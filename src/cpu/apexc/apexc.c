/*
	cpu/apexc/apexc.c : APE(X)C CPU emulation

	By Raphael Nabet

	APE(X)C (All Purpose Electronic X-ray Computer) was a computer built by Andrew D. Booth
	and others for the Birkbeck College, in London, which was used to compute cristal
	structure using X-ray diffraction.

	It was one of the APEC series of computer, which were simple electronic computers
	built in the early 1950s for various British Universities.  The HEC (built by
	the British Tabulating Machine Company) and another machine named MAC were based
	on the APEXC.

	References :
	* Andrew D. Booth & Kathleen H. V. Booth : Automatic Digital Calculators, 2nd edition
	(Buttersworth Scientific Publications, 1956)  (referred to as 'Booth&Booth')
	* Kathleen H. V. Booth : Programming for an Automatic Digital Calculator
	(Buttersworth Scientific Publications, 1958)  (referred to as 'Booth')
*/

/*
	Generals specs :
	* 32-bit data word size (10-bit addresses) : uses fixed-point, 2's complement arithmetic
	* CPU has one accumulator (A) and one register (R), plus a Control Register (this is
	  what we would call an "instruction register" nowadays).  No Program Counter, each
	  instruction contains the address of the next instruction (!).
	* memory is composed of 256 circular magnetic tracks of 32 words : only 32 tracks can
	  be accessed at a time (the 16 first ones, plus 16 others chosen by the programmer),
	  and the rotation rate is 3750rpm (62.5 rotations per second).
	* two I/O units : tape reader and tape puncher.  A teletyper was designed to read
	  specially-encoded punched tapes and print decoded text.  (See /systems/apexc.c)
	* machine code has 15 instructions (!), including add, substract, shift, multiply (!),
	  test and branch, input and punch.  A so-called vector mode allow to repeat the same
	  operation 32 times with 32 successive memory locations.  Note the lack of bitwise
	  and/or/xor (!) .
	* 1 kIPS, although memory access times make this figure fairly theorical (drum rotation
	  time : 16ms, which would allow about 60IPS when no optimization is made)
	* there is no indirect addressing whatever, although dynamic modification of opcodes (!)
	  allow to simulate it...
	* a control panel allows operation and debugging of the machine.  (See /systems/apexc.c)

	Conventions :
	Bits are numbered in big-endian order, starting with 1 (!) : bit #1 is the MSB,
	and bit #32 is the LSB

	References :
	* Andrew D. Booth & Kathleen H. V. Booth : Automatic Digital Calculators, 2nd edition
	(Buttersworth Scientific Publications, 1956)
	* Kathleen H. V. Booth : Programming for an Automatic Digital Calculator
	(Buttersworth Scientific Publications, 1958)
*/

/*
	Machine code:

	Format of a machine instruction :
bits:		1-5			6-10		11-15		16-20		21-25		26-31		32
field:		X address	X address	Y address	Y address	Function	C6			Vector
			(track)		(location)	(track)		(location)

	Meaning of fields :
	X : address of an operand, or immediate, or meaningless, depending on Function
		(When X is meaningless, it should be a duplicate of Y.  Maybe this is because
		X is unintentionnally loaded into the memory address register, and if track # is
		different, we add unneeded track switch delays (this theory is either wrong or
		incomplete, since it cannot be true for B or X))
	Y : address of the next instruction
	Function : code for the actual instruction executed
	C6 : immediate value used by shift, multiply and store operations
	Vector : repeat operation 32 times (on all 32 consecutive locations of a track,
		starting with the location given by the X field)

	Function code :
	#	Mnemonic	C6		Description

	0	Stop

	2	I(y)				Input.  5 digits from tape are moved to the 5 MSBits of R (which
							must be cleared initially).

	4	P(y)				Punch.  5 MSBits of R are punched on tape.

	6	B<(x)>=(y)			Branch.  If A<0, next instruction is read from @x, whereas
							if A>=0, next instruction is read from @y

	8	l (y)		n		Shift left : the 64 bits of A and R are rotated left n times.
		 n

	10	r (y)		64-n	Shift right : the 64 bits of A and R are shifted right n times.
		 n					The sign bit of A is duplicated.

	14	X (x)(y)	33-n	Multiply the contents of *track* x by the last n digits of the
		 n					number in R, sending the 32 MSBs to A and 31 LSBs to R

	16	+c(x)(y)			A <- (x)

	18	-c(x)(y)			A <- -(x)

	20	+(x)(y)				A <- A+(x)

	22	-(x)(y)				A <- A-(x)

	24	T(x)(y)				R <- (x)

	26	R   (x)(y)	32+n	record first or last bits of R in (x).  The remaining bits of x
		 1-n				are unaffected, but "the contents of R are filled with 0s or 1s
							according as the original contents were positive or negative".
		R    (x)(y)	n-1
		 n-32

	28	A   (x)(y)	32+n	Same as 26, except that source is A, and the contents of A are
		 1-n				not affected.

		A    (x)(y)	n-1
		 n-32

	30	S(x)(y)				Block Head switch.  This enables the block of heads specified
							in x to be loaded into the working store.

	Note : Mnemonics use subscripts (!), which I tried to render the best I could.  Also,
	  ">=" is actually one single character.  Last, "1-n" and "n-32" in store mnemonics
	  are the actual sequences "1 *DASH* <number n>" and "<number n> *DASH* 32"
	  (these are NOT formulas with substract signs).

	Note2 : Short-hand notations : X stands for X  , A for A    , and R for R    .
	                                             32         1-32             1-32

	Note3 : Vectors instruction are notated with a subscript 'v' following the basic
	  mnemonic.  For instance :

		A (x)(y), + (x)(y)
		 v         v

	  are the vector counterparts of A(x)(y) and +(x)(y).
*/

/*
	memory interface :

	Data is exchanged on a 1-bit (!) data bus, 10-bit address bus.

	While the bus is 1-bit wide, read/write operation can only be take place on word
	(i.e. 32 bit) boundaries.  However, it is possible to store only the n first bits or
	n last bits of a word, leaving other bits in memory unaffected.

	The LSBits are transferred first, since it allows to perform bit-per-bit add and
	substract.  Otherwise, the CPU would need an additionnal register to store the second
	operand, and it would be probably slower, since the operation could only
	take place after all the data has been transfered.

	Memory operations are synchronous with 2 clocks found on the memory controller :
	* word clock : a pulse on each word boundary (3750rpm*32 -> 2kHz)
	* bit clock : a pulse when a bit is present on the bus (word clock * 32 -> 64kHz)

	CPU operation is synchronous with these clocks, too.  For instance, AU does bit-per-bit
	addition and substraction with a memory operand, synchronously with bit clock,
	starting and stopping on word clock boundaries.  Similar thing with a Fetch operation.

	There is a 10-bit memory location (i.e. address) register on the memory controller.
	It is loaded with the contents of X after when instruction fetch is complete, and
	with the contents of Y when instruction execution is complete, so that the next fetch
	can be executed correctly.
*/

/*
	Instruction timings :


	References : Booth p. 14 for the table below


	Mnemonic					delay in word clock cycles

	I							32

	P							32

	B							0

	l							1 if n>=32 (i.e. C6>=32) (see 4.)
	 n							2 if n<32  (i.e. C6<32)

	r							1 if n<=32 (i.e. C6>=32) (see 4.)
	 n							2 if n>32  (i.e. C6<32)

	X							32

	+c, -c, +, -, T				0

	R    , R    , A   , A		1 (see 1. & 4.)
	  1-n   n-32   1-n   n-32

	track switch				6 (see 2.)

	vector						12 (see 3.)


	(S and stop are missing in the table)


	Note that you must add the fetch delay (at least 1 cycle), and, when applicable, the
	operand read/write delay (at least 1 cycle).


	Notes :

	1.  The delay is applied after the store is done (from the analysis of the example
	 in Booth p.52)

	2.  I guess that the memory controller needs 6 cycles to stabilize whenever track
	  switching occurs, i.e. when X does not refer to the current track, and then when Y
	  does not refer to the same track as X.  This matches various examples in Booth,
	  although it appears that this delay is not applied when X is not read (cf cross-track
	  B in Booth p. 49).
	    However, and here comes the wacky part, analysis of Booth p. 55 shows that
	  no additionnal delay is caused by an X instruction having its X operand
	  on another track.  Maybe, just maybe, this is related to the fact that X does not
	  need to take the word count into account, any word in track is as good as any (yet,
	  this leaves the question of why this optimization could not be applied to vector
	  operations unanswered).

	3.  This is an ambiguous statement.  Analysis of Booth p. 55 shows that
	  an instance of an Av instruction with its destination on another track takes no more
	  than 45 cycles, as follow :
	    * 1 cycle for fetch
	    * 6-cycle delay (at most) before write starts (-> track switch)
	    * 32 memory cycles
	    * 6-cycle delay (at most) after write completion (-> track switch)
	  It appears that the delay associated with the vector mode is not distinguishable from
	  the delay caused by track switch and even the delay associated to the Av instruction.
	    Is there really a specific delay associated with the vector mode ? To know this, we
	  would need to see a vector instruction on the same track as its operands, which is
	  unlikely to be seen (the only reasonnable application I can see is running a '+_v'
	  to compute the checksum of the current track).

	4.  Example in Booth p. 76 ("20/4 A (27/27) (21/2)") seems to imply that
	  when doing a store with a destination on a track other than the track where next
	  instruction is located, the 1-cycle post-store delay is merged with the 6-cycle track
	  switch delay.  (I assume this because there is lots of room on track 21, and if
	  the delays were not merged, it should be easy to move the instruction forward
	  to speed up loop execution time.
	    Similarly, example in Booth p. 49-50 ("4/24 l 32 (5/31)") seems to show that
	  a similar delay merge occurs when doing a shift with the next instruction located on
	  another track.
*/

#include "driver.h"
#include "mamedbg.h"

#include "apexc.h"

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

typedef struct
{
	UINT32 a;	/* accumulator */
	UINT32 r;	/* register */
	UINT32 cr;	/* control register (i.e. instruction register) */
	int ml;		/* memory location (current track in working store, and requested
				word position within track) */
	int working_store;	/* current working store (group of 16 tracks) (1-15) */
	int current_word;	/* current word position within track (0-31) */

	int running;	/* 1 flag : */
				/* running : flag implied by the existence of the stop instruction */
} apexc_regs;

static apexc_regs apexc;

int apexc_ICount;

/* decrement ICount by n */
#define DELAY(n)	{apexc_ICount -= (n); apexc.current_word = (apexc.current_word + (n)) & 0x1f;}


/*
	word accessor functions

	take a 10-bit word address
		5 bits (MSBs) : track address within working store
		5 bits (LSBs) : word position within track

	'special' flag : if true, read first word found in track (used by X instruction only)

	'mask' : one bit is set for each bit to write (used by store instructions)

	memory latency delays are taken into account, but not track switching delays
*/


/* memory access macros */

#define cpu_readmem(address) cpu_readmem18bedw_dword(address << 2)

/* eewww ! - Fortunately, there is no memory mapped I/O, so we can simulate masked write
without danger */
#define cpu_writemem_masked(address, data, mask)										\
	cpu_writemem18bedw_dword(address << 2,												\
								(cpu_readmem18bedw_dword(address << 2) & ~mask) |		\
									(data & mask))

/* compute complete word address (i.e. translate a logical track address (expressed
in current working store) to an absolute track address) */
static int effective_address(int address)
{
	if (address & 0x200)
	{
		address = (address & 0x1FF) | (apexc.working_store) << 9;
	}

	return address;
}

/* read word */
static UINT32 word_read(int address, int special)
{
	UINT32 result;

	/* compute absolute track address */
	address = effective_address(address);

	if (special)
	{
		/* ignore word position in x - use current position instead */
		address = (address & ~ 0x1f) | apexc.current_word;
	}
	else
	{
		/* wait for requested word to appear under the heads */
		DELAY(((address /*& 0x1f*/) - apexc.current_word) & 0x1f);
	}

	/* read 32 bits */
#if 0
	/* note that the APEXC reads LSBits first */
	result = 0;
	for (i=0; i<31; i++)
	{
		/*if (mask & (1 << i))*/
			result |= bit_read((address << 5) | i) << i;
	}
#else
	result = cpu_readmem(address);
#endif

	/* read takes one memory cycle */
	DELAY(1);

	return result;
}

/* write word (or part of a word, according to mask) */
static void word_write(int address, UINT32 data, UINT32 mask)
{
	/* compute absolute track address */
	address = effective_address(address);

	/* wait for requested word to appear under the heads */
	DELAY(((address /*& 0x1f*/) - apexc.current_word) & 0x1f);

	/* write 32 bits according to mask */
#if 0
	/* note that the APEXC reads LSBits first */
	for (i=0; i<31; i++)
	{
		if (mask & (1 << i))
			bit_write((address << 5) | i, (data >> i) & 1);
	}
#else
	cpu_writemem_masked(address, data, mask);
#endif

	/* write takes one memory cycle (2, actually, but the 2nd cycle is taken into
	account in execute) */
	DELAY(1);
}

/*
	I/O accessors

	no address is used, these functions just punch or read 5 bits
*/

static int papertape_read(void)
{
	return cpu_readport16bedw(0) & 0x1f;
}

static void papertape_punch(int data)
{
	cpu_writeport16bedw(0, data);
}

/*
	now for emulation code
*/

/*
	set the memory location (i.e. address) register, and compute the associated delay
*/
INLINE int load_ml(int address, int vector)
{
	int delay;

	/* additionnal delay appears if we switch tracks */
	if (((apexc.ml & 0x3E0) != (address & 0x3E0)) /*|| vector*/)
		delay = 6;	/* if tracks are different, delay to allow for track switching */
	else
		delay = 0;	/* else, no problem */

	apexc.ml = address;	/* save ml */

	return delay;
}

/*
	execute one instruction

	TODO :
	* test !!!

	NOTE :
	* I do not know whether we should fetch instructions at the beginning or the end of the
	instruction cycle.  Either solution is roughly equivalent to the other, but changes
	the control panel operation (and I know virtually nothing on the control panel).
	Currently, I fetch after the executing the instruction, so that the one may enter
	an instruction into the control register with the control panel, then execute it.
	This solution make timing simulation much simpler, too.
*/
static void execute(void)
{
	int x, y, function, c6, vector;	/* instruction fields */
	int i = 0;			/* misc counter */
	int has_operand;	/* true if instruction is an AU operation with an X operand */
	static const char has_operand_table[32] =	/* table for has_operand - one entry for each function code */
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 
	};
	int delay1;	/* pre-operand-access delay */
	int delay2;	/* post-operation delay */
	int delay3;	/* pre-operand-fetch delay */

	/* first isolate the instruction fields */
	x = (apexc.cr >> 22) & 0x3FF;
	y = (apexc.cr >> 12) & 0x3FF;
	function = (apexc.cr >> 7) & 0x1F;
	c6 = (apexc.cr >> 1) & 0x3F;
	vector = apexc.cr & 1;

	function &= 0x1E;	/* this is a mere guess - the LSBit is reserved for future additions */

	/* determinates if we need to read an operand*/
	has_operand = has_operand_table[function];

	if (has_operand)
	{
		/* load ml with X */
		delay1 = load_ml(x, vector);
		/* burn pre-operand-access delay if needed */
		if (delay1)
		{
			DELAY(delay1);
		}
	}

	delay2 = 0;	/* default */

	do
	{
		switch (function)
		{
		case 0:
			/* stop */

			apexc.running = FALSE;

			/* BTW, I don't know whether stop loads y into ml or not, and whether
			subsequent fetch is done */
			break;

		case 2:
			/* I */
			/* I do not know whether the CPU does an OR or whatever, but since docs say that
			the 5 bits must be cleared initially, an OR kind of makes sense */
			apexc.r |= papertape_read() << 27;
			DELAY(32);	/* no idea whether this should be counted as an absolute delay 
						or as a value in delay2 */
			break;

		case 4:
			/* P */
			papertape_punch((apexc.r >> 27) & 0x1f);
			DELAY(32);	/* no idea whether this should be counted as an absolute delay 
						or as a value in delay2 */
			break;

		case 6:
			/* B<(x)>=(y) */
			/* I have no idea what we should do if the vector bit is set */
			if (apexc.a & 0x80000000UL)
			{
				/* load ml with X */
				delay1 = load_ml(x, vector);
				/* burn pre-fetch delay if needed */
				if (delay1)
				{
					DELAY(delay1);
				}
				/* and do fetch at X */
				goto special_fetch;
			}
			/* else, the instruction ends with a normal fetch */
			break;

		case 8:
			/* l_n */
			delay2 = (c6 & 0x20) ? 1 : 2;	/* if more than 32 shifts, it takes more time */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			while (c6 != 0)
			{
				int shifted_bit = 0;

				/* shift and increment c6 */
				shifted_bit = apexc.r & 1;
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a >>= 1;
				if (shifted_bit)
					apexc.a |= 0x80000000UL;

				c6 = (c6+1) & 0x3f;
			}

			break;

		case 10:
			/* r_n */
			delay2 = (c6 & 0x20) ? 1 : 2;	/* if more than 32 shifts, it takes more time */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			while (c6 != 0)
			{
				/* shift and increment c6 */
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a = ((INT32) apexc.a) >> 1;

				c6 = (c6+1) & 0x3f;
			}

			break;

		case 12:
			/* unused function code.  I assume this results into a NOP, for lack of any
			specific info... */

			break;

		case 14:
			/* X_n(x) */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			/* algorithm found in Booth&Booth, p. 45-48 */
			apexc.a = 0;
			while (1)
			{
				int shifted_bit = 0;

				/* note we read word at current word position */
				if (shifted_bit && ! (apexc.r & 1))
					apexc.a += word_read(x, 1);
				else if ((! shifted_bit) && (apexc.r & 1))
					apexc.a -= word_read(x, 1);
				else
					/* Even if we do not read anything, the loop still takes 1 cycle of
					the memory word clock. */
					/* Anyway, maybe we still read the data even if we do not use it. */
					DELAY(1);

				/* exit if c6 reached 32 ("c6 & 0x20" is simpler to implement and
				essentially equivalent, so this is most likely the actual implementation) */
				if (c6 & 0x20)
					break;

				/* else increment c6 and  shift */
				c6 = (c6+1) & 0x3f;

				/* shift */
				shifted_bit = apexc.r & 1;
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a = ((INT32) apexc.a) >> 1;
			}

			//DELAY(32);	/* mmmh... we have already counted 32 wait states */
			/* actually, if (n < 32) (which is an untypical case), we do not have 32 wait
			states.  Question is : do we really have 32 wait states if (n < 32), or is
			the timing table incomplete ? */
			break;

		case 16:
			/* +c(x) */
			apexc.a = + word_read(apexc.ml, 0);
			break;

		case 18:
			/* -c(x) */
			apexc.a = - word_read(apexc.ml, 0);
			break;

		case 20:
			/* +(x) */
			apexc.a += word_read(apexc.ml, 0);
			break;

		case 22:
			/* -(x) */
			apexc.a -= word_read(apexc.ml, 0);
			break;

		case 24:
			/* T(x) */
			apexc.r = word_read(apexc.ml, 0);
			break;

		case 26:
			/* R_(1-n)(x) & R_(n-32)(x) */

			{
				UINT32 mask;

				if (c6 & 0x20)
					mask = 0xFFFFFFFFUL << (64 - c6);
				else
					mask = 0xFFFFFFFFUL >> c6;

				word_write(apexc.ml, apexc.r, mask);
			}

			apexc.r = (apexc.r & 0x80000000UL) ? 0xFFFFFFFFUL : 0;

			delay2 = 1;
			break;

		case 28:
			/* A_(1-n)(x) & A_(n-32)(x) */

			{
				UINT32 mask;

				if (c6 & 0x20)
					mask = 0xFFFFFFFFUL << (64 - c6);
				else
					mask = 0xFFFFFFFFUL >> c6;

				word_write(apexc.ml, apexc.a, mask);
			}

			delay2 = 1;
			break;

		case 30:
			/* S(x) */
			apexc.working_store = (x >> 5) & 0xf;	/* or is it (x >> 6) ? */
			DELAY(32);	/* no idea what the value is...  All I know is that it takes much
						more time than track switching (which takes 6 cycles) */
			break;
		}
		if (vector)
			/* increment word position in vector operations */
			apexc.ml = (apexc.ml & 0x3E0) | ((apexc.ml + 1) & 0x1F);
	} while (vector && has_operand && (++i < 32));	/* iterate 32 times if vector bit is set */
													/* the has_operand is a mere guess */

	/* load ml with Y */
	delay3 = load_ml(y, 0);

	/* compute max(delay2, delay3) */
	if (delay2 > delay3)
		delay3 = delay2;

	/* burn pre-fetch delay if needed */
	if (delay3)
	{
		DELAY(delay3);
	}

	/* entry point after a successful Branch (which alters the normal instruction sequence,
	in order not to load ml with Y) */
special_fetch:

	/* fetch current instruction into control register */
	apexc.cr = word_read(apexc.ml, 0);
}




void apexc_reset(void *param)
{
	/* mmmh...  I don't know what happens on reset with an actual APEXC. */

	apexc.working_store = 1;	/* mere guess */
	apexc.current_word = 0;		/* well, we do have to start somewhere... */

	/* next two lines are just the product of my bold fantasy */
	apexc.cr = 0;				/* first instruction executed will be a stop */
	apexc.running = TRUE;		/* this causes the CPU to load the instruction at 0/0,
								which enables easy booting (just press run on the panel) */
}

void apexc_exit(void)
{
}

unsigned apexc_get_context(void *dst)
{
	if (dst)
		* ((apexc_regs*) dst) = apexc;
	return sizeof(apexc_regs);
}

void apexc_set_context(void *src)
{
	if (src)
	{
		apexc = * ((apexc_regs*)src);
	}
}

/* no PC - return memory location register instead, this should be equivalent unless
executed in the midst of an instruction */
unsigned apexc_get_pc(void)
{
	return effective_address(apexc.ml);
}

void apexc_set_pc(unsigned val)
{
	/* keep address 9 LSBits - 10th bit depends on whether we are accessing the permanent
	or a switchable track group */
	apexc.ml = val & 0x1ff;
	if (val & 0x1e00)
	{	/* we are accessing a switchable track group */
		apexc.ml |= 0x200;	/* set 10th bit */

		if (((val >> 9) & 0xf) != apexc.working_store)
		{	/* we need to do a store switch */
			apexc.working_store = ((val >> 9) & 0xf);
		}
	}
	
}

/* no SP */
unsigned apexc_get_sp(void)
{
	return 0U;
}

void apexc_set_sp(unsigned val)
{
	(void) val;
}

/* no NMI line */
void apexc_set_nmi_line(int state)
{
	(void) state;
}

/* no IRQ line */
void apexc_set_irq_line(int irqline, int state)
{
	(void) irqline;
	(void) state;
}

void apexc_set_irq_callback(int (*callback)(int irqline))
{
	(void) callback;
}

unsigned apexc_get_reg(int regnum)
{
	switch (regnum)
	{
		case APEXC_CR:
			return apexc.cr;
		case APEXC_A:
			return apexc.a;
		case APEXC_R:
			return apexc.r;
		case APEXC_ML:
			return apexc.ml;
		case APEXC_WS:
			return apexc.working_store;
		case APEXC_STATE:
			return apexc.running ? TRUE : FALSE;
		case APEXC_ML_FULL:
			return effective_address(apexc.ml);
	}
	return 0;
}

void apexc_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case APEXC_CR:
			apexc.cr = val;
			break;
		case APEXC_A:
			apexc.a = val;
			break;
		case APEXC_R:
			apexc.r = val;
			break;
		case APEXC_ML:
			apexc.ml = val;
			break;
		case APEXC_WS:
			apexc.working_store = val;
			break;
		case APEXC_STATE:
			apexc.running = val ? TRUE : FALSE;
	}
}

const char *apexc_info(void *context, int regnum)
{
	static const UINT8 apexc_reg_layout[] =
	{
		APEXC_CR, -1,
		APEXC_A, APEXC_R, -1,
		APEXC_ML, APEXC_WS, -1,
		APEXC_STATE, 0
	};

	/* OK, I have no idea what would be the best layout */
	static const UINT8 apexc_win_layout[] =
	{
		48, 0,32,13,	/* register window (top right) */
		 0, 0,47,13,	/* disassembler window (top left) */
		 0,14,47, 8,	/* memory #1 window (left, middle) */
		48,14,32, 8,	/* memory #2 window (right, middle) */
		 0,23,80, 1 	/* command line window (bottom rows) */
	};

	static char buffer[16][47 + 1];
	static int which = 0;
	apexc_regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if (! context)
		r = &apexc;

	switch (regnum)
	{
	case CPU_INFO_REG + APEXC_CR:
		sprintf(buffer[which], "CR:%08X", r->cr);
		break;
	case CPU_INFO_REG + APEXC_A:
		sprintf(buffer[which], "A :%08X", r->a);
		break;
	case CPU_INFO_REG + APEXC_R: 
		sprintf(buffer[which], "R :%08X", r->r);
		break;
	case CPU_INFO_REG + APEXC_ML:
		sprintf(buffer[which], "ML:%03X", r->ml);
		break;
	case CPU_INFO_REG + APEXC_WS:
		sprintf(buffer[which], "WS:%01X", r->working_store);
		break;
	case CPU_INFO_REG + APEXC_STATE:
		sprintf(buffer[which], "CPU state:%01X", r->running ? TRUE : FALSE);
		break;
	case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c", (r->running) ? 'R' : 'S');
		break;
	case CPU_INFO_NAME:
		return "APEXC";
	case CPU_INFO_FAMILY:
		return "APEC";
	case CPU_INFO_VERSION:
		return "1.0";
	case CPU_INFO_FILE:
		return __FILE__;
	case CPU_INFO_CREDITS:
		return "Raphael Nabet";
	case CPU_INFO_REG_LAYOUT:
		return (const char *) apexc_reg_layout;
	case CPU_INFO_WIN_LAYOUT:
		return (const char *) apexc_win_layout;
	}
	return buffer[which];
}

unsigned apexc_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmAPEXC(buffer,pc);
#else
	sprintf(buffer, "$%08X", cpu_readop(pc));
	return 1;
#endif
}

int apexc_execute(int cycles)
{
	apexc_ICount = cycles;

	do
	{
		CALL_MAME_DEBUG;

		if (apexc.running)
			execute();
		else
		{
			DELAY(apexc_ICount);	/* burn cycles once for all */
		}
	} while (apexc_ICount > 0);

	return cycles - apexc_ICount;
}

