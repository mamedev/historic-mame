/*************************************************************************

 Exidy Vertigo hardware

 The Vertigo vector CPU consists of four AMD 2901 bit slice
 processors, logic to control microcode program flow and a digital
 vector generator. The microcode for the bit slice CPUs is stored in 13
 bipolar proms for a total of 512 52bit wide micro instructions. The
 microcode not only crontrols the 2901s but also loading and storing
 of operands and results, program flow control and vector generation.

 +----+----+----+----+----+----+-------+-----+-----+------+-----+----+---+
 |VUC |VUC |VUC |VUC |VUC |VUC |  VUC  | VUC | VUC | VUC  | VUC |VUC |VUC| labels
 | 10 | 13 | 9  | 8  | 7  | 6  |   5   |  12 |  11 |  2   |  1  | 4  | 3 |
 +----+----+----+----+----+----+-------+-----+-----+------+-----+----+---+
 |    |    |    |    |    |    |       |     |     |      |     |PR5/|R5/| schematics
 |J5/4|G5/1|K5/5|L5/6|M5/7|N5/8| P5/9  |H5/2 |HJ5/3|S5/12 |T5/13| 10 |11 |
 +----+----+----+----+----+----+-------+-----+-----+------+-----+----+---+
 55 44|4444|4444|3333|3333|3322|2 2 2 2|2 222|11 11|1 1 11|110 0|0000|0000
 21 98|7654|3210|9876|5432|1098|7 6 5 4|3 210|98 76|5 4 32|109 8|7654|3210

    xx|xxxx|aaaa|bbbb|iiii|iiii|i c m r|r ooo|ii oo|  j jj|jjj m|mmmm|mmmm
    54|3210|3210|3210|8765|4321|0 n r s|w fff|ff aa|  p 43|210 a|aaaa|aaaa
                                    e e|r 210|10 10|  o   |    8|7654|3210
                                    q l i             s
                                        t
                                        e
 x:    address for 64 words of 16 bit wide SRAM
 a:    A register index
 b:    B register index
 i:    2901 instruction
 cn:   carry bit
 mreq, rsel, rwrite: signals for memory access
 of:   vector generator
 if:   vector RAM/ROM data select
 oa:   vector RAM/ROM address select
 jpos: jump condition inverted
 j:    jump condition and type
 m:    jump address

 Variables, enums and defines are named as in the schematics (pp. 6, 7)
 where possible.

*************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vertigo.h"


/*************************************
 *
 *  Global variables
 *
 *************************************/

UINT16 *vertigo_vectorram;


/*************************************
 *
 *  Typedefs
 *
 *************************************/

typedef struct _am2901
{
	int ram[16]; /* internal ram */
	int i;		 /* instruction */
	int a;		 /* A address to int. ram */
	int b;		 /* B address to int. ram */
	int d;		 /* direct data D input */
	int q;		 /* Q register */
	int f;		 /* F ALU result */
	int y;		 /* Y output */
	int c;		 /* carry bit */
} am2901;

typedef struct _vgen
{
	int sreg;		 /* Vector Generator shift register */
	int l1;			 /* VG latch 1 adder operand only */
	int l2;			 /* VG latch 2 adder operand only */
	int c_v;		 /* VG vertical position counter */
	int c_h;		 /* VG horizontal position counter */
	int c_l;		 /* VG length counter */
	int adder_s;	 /* VG slope generator result and B input */
	int adder_a;	 /* VG slope generator A input */
	int color;		 /* VG color */
	int intensity;	 /* VG intensity */
	int brez;		 /* VG h/v-counters enable */
	int vfin;		 /* VG drawing yes/no */
	int hud1;		 /* VG h-counter up or down (stored in L1) */
	int hud2;		 /* VG h-counter up or down (stored in L2) */
	int vud1;		 /* VG v-counter up or down (stored in L1) */
	int vud2;		 /* VG v-counter up or down (stored in L2) */
	int hc1;		 /* VG use h- or v-counter in L1 mode */
	int ven;		 /* VG state of transistor which controls intensity output */
} vgen;

typedef struct _vertigo_vproc_state
{
	am2901 bs;		 /* 2901 state */
	vgen vg;		 /* vertigo's vector generator */
	UINT16 sram[64]; /* external sram */
	UINT16 ramlatch; /* latch between 2901 and sram */
	UINT16 rom_adr;	 /* vector ROM/RAM address latch */
	int pc;			 /* program counter */
	int ret;		 /* return address */

} vertigo_vproc_state;


/*************************************
 *
 *  Statics
 *
 *************************************/

static vertigo_vproc_state vs;
static UINT16 *vertigo_vectorrom;
static UINT64 *mcode;


/*************************************
 *
 *  Macros and enums
 *
 *************************************/

#define V_ADDPOINT(h,v,c,i) \
	vector_add_point (((h) & 0x7ff) << 14, (0x6ff - ((v) & 0x7ff)) << 14, VECTOR_COLOR444(c), (i))

#define MC_X	  ((mcode[vs.pc] >> 44) & 0x3f)
#define MC_A	  ((mcode[vs.pc] >> 40) & 0xf)
#define MC_B	  ((mcode[vs.pc] >> 36) & 0xf)
#define MC_INST	  ((mcode[vs.pc] >> 27) & 0777)
#define MC_CN	  ((mcode[vs.pc] >> 26) & 0x1)
#define MC_MREQ	  ((mcode[vs.pc] >> 25) & 0x1)
#define MC_RSEL	  (MC_RWRITE & ((mcode[vs.pc] >> 24) & 0x1))
#define MC_RWRITE ((mcode[vs.pc] >> 23) & 0x1)
#define MC_RAMACC (MC_RWRITE+MC_RSEL)
#define MC_OF	  ((mcode[vs.pc] >> 20) & 0x7)
#define MC_IF	  ((mcode[vs.pc] >> 18) & 0x3)
#define MC_OA	  ((mcode[vs.pc] >> 16) & 0x3)
#define MC_JPOS	  ((mcode[vs.pc] >> 14) & 0x1)
#define MC_JMP	  ((mcode[vs.pc] >> 12) & 0x3)
#define MC_JCON	  ((mcode[vs.pc] >> 9) & 0x7)
#define MC_MA	  ( mcode[vs.pc] & 0x1ff)

#define ADD(r,s,c)	(((r)  + (s) + (c)) & 0xffff)
#define SUBR(r,s,c) ((~(r) + (s) + (c)) & 0xffff)
#define SUBS(r,s,c) (((r) + ~(s) + (c)) & 0xffff)
#define OR(r,s)		((r) | (s))
#define AND(r,s)	((r) & (s))
#define NOTRS(r,s)	(~(r) & (s))
#define EXOR(r,s)	((r) ^ (s))
#define EXNOR(r,s)	(~((r) ^ (s)))

/* possible values for MC_DST */
enum {
	QREG = 0,
	NOP,
	RAMA,
	RAMF,
	RAMQD,
	RAMD,
	RAMQU,
	RAMU
};

/* possible values for MC_RAMACC */
enum {
	S_RAMW = 0,
	S_RAMR,
	S_RAM0
};

/* possible values for MC_IF */
enum {
	S_ROMDE = 0,
	S_RAMDE
};

/* possible values for MC_OA */
enum {
	S_SREG = 0,
	S_ROMA,
	S_RAMD
};

/* possible values for MC_JMP */
enum {
	S_JBK = 0,
	S_CALL,
	S_OPT,
	S_RETURN
};

/* possible values for MC_JCON */
enum {
	S_MSB = 1,
	S_FEQ0,
	S_Y10,
	S_VFIN,
	S_FPOS,
	S_INTL4
};


/********************************************
 *
 *  4 x AM2901 bit slice processors
 *  Q3 and IN3 are hardwired
 *
 ********************************************/

static void am2901x4 (am2901 *bs)
{
	switch (bs->i & 077)
	{
	case 000: bs->f = ADD(bs->ram[bs->a], bs->q, bs->c); break;
	case 001: bs->f = ADD(bs->ram[bs->a], bs->ram[bs->b], bs->c); break;
	case 002: bs->f = ADD(0, bs->q, bs->c); break;
	case 003: bs->f = ADD(0, bs->ram[bs->b], bs->c); break;
	case 004: bs->f = ADD(0, bs->ram[bs->a], bs->c); break;
	case 005: bs->f = ADD(bs->d, bs->ram[bs->a], bs->c); break;
	case 006: bs->f = ADD(bs->d, bs->q, bs->c); break;
	case 007: bs->f = ADD(bs->d, 0, bs->c); break;

	case 010: bs->f = SUBR(bs->ram[bs->a], bs->q, bs->c); break;
	case 011: bs->f = SUBR(bs->ram[bs->a], bs->ram[bs->b], bs->c); break;
	case 012: bs->f = SUBR(0, bs->q, bs->c); break;
	case 013: bs->f = SUBR(0, bs->ram[bs->b], bs->c); break;
	case 014: bs->f = SUBR(0, bs->ram[bs->a], bs->c); break;
	case 015: bs->f = SUBR(bs->d, bs->ram[bs->a], bs->c); break;
	case 016: bs->f = SUBR(bs->d, bs->q, bs->c); break;
	case 017: bs->f = SUBR(bs->d, 0, bs->c); break;

	case 020: bs->f = SUBS(bs->ram[bs->a], bs->q, bs->c); break;
	case 021: bs->f = SUBS(bs->ram[bs->a], bs->ram[bs->b], bs->c); break;
	case 022: bs->f = SUBS(0, bs->q, bs->c); break;
	case 023: bs->f = SUBS(0, bs->ram[bs->b], bs->c); break;
	case 024: bs->f = SUBS(0, bs->ram[bs->a], bs->c); break;
	case 025: bs->f = SUBS(bs->d, bs->ram[bs->a], bs->c); break;
	case 026: bs->f = SUBS(bs->d, bs->q, bs->c); break;
	case 027: bs->f = SUBS(bs->d, 0, bs->c); break;

	case 030: bs->f = OR(bs->ram[bs->a], bs->q); break;
	case 031: bs->f = OR(bs->ram[bs->a], bs->ram[bs->b]); break;
	case 032: bs->f = OR(0, bs->q); break;
	case 033: bs->f = OR(0, bs->ram[bs->b]); break;
	case 034: bs->f = OR(0, bs->ram[bs->a]); break;
	case 035: bs->f = OR(bs->d, bs->ram[bs->a]); break;
	case 036: bs->f = OR(bs->d, bs->q); break;
	case 037: bs->f = OR(bs->d, 0); break;

	case 040: bs->f = AND(bs->ram[bs->a], bs->q); break;
	case 041: bs->f = AND(bs->ram[bs->a], bs->ram[bs->b]); break;
	case 042: bs->f = AND(0, bs->q); break;
	case 043: bs->f = AND(0, bs->ram[bs->b]); break;
	case 044: bs->f = AND(0, bs->ram[bs->a]); break;
	case 045: bs->f = AND(bs->d, bs->ram[bs->a]); break;
	case 046: bs->f = AND(bs->d, bs->q); break;
	case 047: bs->f = AND(bs->d, 0); break;

	case 050: bs->f = NOTRS(bs->ram[bs->a], bs->q); break;
	case 051: bs->f = NOTRS(bs->ram[bs->a], bs->ram[bs->b]); break;
	case 052: bs->f = NOTRS(0, bs->q); break;
	case 053: bs->f = NOTRS(0, bs->ram[bs->b]); break;
	case 054: bs->f = NOTRS(0, bs->ram[bs->a]); break;
	case 055: bs->f = NOTRS(bs->d, bs->ram[bs->a]); break;
	case 056: bs->f = NOTRS(bs->d, bs->q); break;
	case 057: bs->f = NOTRS(bs->d, 0); break;

	case 060: bs->f = EXOR(bs->ram[bs->a], bs->q); break;
	case 061: bs->f = EXOR(bs->ram[bs->a], bs->ram[bs->b]); break;
	case 062: bs->f = EXOR(0, bs->q); break;
	case 063: bs->f = EXOR(0, bs->ram[bs->b]); break;
	case 064: bs->f = EXOR(0, bs->ram[bs->a]); break;
	case 065: bs->f = EXOR(bs->d, bs->ram[bs->a]); break;
	case 066: bs->f = EXOR(bs->d, bs->q); break;
	case 067: bs->f = EXOR(bs->d, 0); break;

	case 070: bs->f = EXNOR(bs->ram[bs->a], bs->q); break;
	case 071: bs->f = EXNOR(bs->ram[bs->a], bs->ram[bs->b]); break;
	case 072: bs->f = EXNOR(0, bs->q); break;
	case 073: bs->f = EXNOR(0, bs->ram[bs->b]); break;
	case 074: bs->f = EXNOR(0, bs->ram[bs->a]); break;
	case 075: bs->f = EXNOR(bs->d, bs->ram[bs->a]); break;
	case 076: bs->f = EXNOR(bs->d, bs->q); break;
	case 077: bs->f = EXNOR(bs->d, 0); break;
	}

	switch (bs->i >> 6)
	{
	case QREG:
		bs->q = bs->f;
		bs->y = bs->f;
		break;
	case NOP:
		bs->y = bs->f;
		break;
	case RAMA:
		bs->y = bs->ram[bs->a];
		bs->ram[bs->b] = bs->f;
		break;
	case RAMF:
		bs->y = bs->f;
		bs->ram[bs->b] = bs->f;
		break;
	case RAMQD:
		bs->y = bs->f;
		bs->q = (bs->q >> 1) & 0x7fff;			/* Q3 is low */
		bs->ram[bs->b] = (bs->f >> 1) | 0x8000; /* IN3 is high! */
		break;
	case RAMD:
		bs->y = bs->f;
		bs->ram[bs->b] = (bs->f >> 1) | 0x8000; /* IN3 is high! */
		break;
	case RAMQU:
		bs->y = bs->f;
		bs->ram[bs->b] = (bs->f << 1) & 0xffff;
		bs->q = (bs->q << 1) & 0xffff;
		break;
	case RAMU:
		bs->y = bs->f;
		bs->ram[bs->b] = (bs->f << 1) & 0xffff;
		break;
	}
}


/********************************************
 *
 *  Vector Generator
 *
 *  This part of the hardware draws vectors
 *  under control of the bit slice processors.
 *  It is just a bunch of counters, latches
 *  and DACs.
 *
 ********************************************/

static void vertigo_vgen (vgen *vg)
{
	if (vg->c_l & 0x800)
	{
		vg->vfin = 1;
		vg->c_l = (vg->c_l+1) & 0xfff;

		if ((vg->c_l & 0x800) == 0)
		{
			vg->brez = 0;
			vg->vfin = 0;
		}

		if (vg->brez) /* H/V counter enabled */
		{
			/* Depending on MSB of adder only one or both
               counters are de-/incremented. This is all
               defined by the shift register which is
               latched in bits 12-15 of L1/L2.
            */
			if (vg->adder_s & 0x800)
			{
				if (vg->hc1)
					vg->c_h += vg->hud1? -1: 1;
				else
					vg->c_v += vg->vud1? -1: 1;
				vg->adder_a = vg->l1;
			}
			else
			{
				vg->c_h += vg->hud2? -1: 1;
				vg->c_v += vg->vud2? -1: 1;
				vg->adder_a = vg->l2;
			}

			/* H/V counters are 12 bit */
			vg->c_v &= 0xfff;
			vg->c_h &= 0xfff;
		}

		vg->adder_s = (vg->adder_s + vg->adder_a) & 0xfff;
	}

	/* BREZ also controls the a transistor which modulates the
       intensity DAC output. There is a shift register in between
       which may be used to introduce some delay. The schematics
       don't show to which output VEN is connected, so I left the
       SR out. At least in Top Gunner any additional delay
       produces garbled output */

	if (vg->brez ^ vg->ven)
	{
		if (vg->ven)
		{
			V_ADDPOINT (vg->c_h, vg->c_v, vg->color, vg->intensity);
		}
		else
		{
			V_ADDPOINT (vg->c_h, vg->c_v, 0, 0);
		}
		vg->ven = vg->brez;
	}
}

/*************************************
 *
 *  Vector processor initialization
 *
 *************************************/

void vertigo_vproc_init(void)
{
	vertigo_vectorrom = (UINT16 *)memory_region(REGION_USER1);
	mcode = (UINT64 *)memory_region(REGION_PROMS);
	memset(&vs, 0, sizeof(vs));
}


/*************************************
 *
 *  Vector processor
 *
 *************************************/

void vertigo_vproc(int cycles, int irq4)
{
	int jcond;

	if (irq4) vector_clear_list();

	profiler_mark(PROFILER_USER1);

	while (cycles--)
	{
		/* Load data */
		switch (MC_IF)
		{
		case S_RAMDE:
			vs.bs.d = vs.ramlatch;
			break;
		case S_ROMDE:
			if (vs.rom_adr < 0x2000)
				vs.bs.d = vertigo_vectorram[vs.rom_adr & 0xfff];
			else
				vs.bs.d = vertigo_vectorrom[vs.rom_adr & 0x7fff];
			break;
		}

		switch(MC_RAMACC)
		{
		case S_RAMR:
			vs.bs.d = vs.sram[MC_X];
			break;
		case S_RAMW:
			/* Data can be transferred between vector ROM/RAM
               and SRAM without going through the 2901 */
			vs.sram[MC_X] = vs.bs.d;
			break;
		}

		/* Setup and call 2901 */
		vs.bs.i = MC_INST;
		vs.bs.a = MC_A;
		vs.bs.b = MC_B;
		vs.bs.c = MC_CN;

		am2901x4 (&vs.bs);

		/* Store data */
		switch (MC_OA)
		{
		case S_RAMD:
			vs.ramlatch = vs.bs.y;
			if (MC_RAMACC==S_RAMW && MC_IF==S_RAMDE)
				vs.sram[MC_X] = vs.ramlatch;
			break;
		case S_ROMA:
			vs.rom_adr = vs.bs.y;
			break;
		case S_SREG:
			/* FPOS is shifted into sreg */
			vs.vg.sreg = (vs.vg.sreg >> 1) | ((vs.bs.f >> 9) & 4);
			break;
		default:
			break;
		}

		/* Vector generator setup */
		switch (MC_OF)
		{
		case 0:
			vs.vg.color = vs.bs.y & 0xfff;
			break;
		case 1:
			vs.vg.intensity = vs.bs.y & 0xff;
			break;
		case 2:
			vs.vg.l1 = vs.bs.y & 0xfff;
			vs.vg.adder_s = 0;
			vs.vg.adder_a = vs.vg.l2;
			vs.vg.hud1 = vs.vg.sreg & 1;
			vs.vg.vud1 = vs.vg.sreg & 2;
			vs.vg.hc1  = vs.vg.sreg & 4;
			vs.vg.brez = 1;
			break;
		case 3:
			vs.vg.l2 = vs.bs.y & 0xfff;
			vs.vg.adder_s = (vs.vg.adder_s + vs.vg.adder_a) & 0xfff;
			if (vs.vg.adder_s & 0x800)
				vs.vg.adder_a = vs.vg.l1;
			else
				vs.vg.adder_a = vs.vg.l2;
			vs.vg.hud2 = vs.vg.sreg & 1;
			vs.vg.vud2 = vs.vg.sreg & 2;
			break;
		case 4:
			vs.vg.c_v = vs.bs.y & 0xfff;
			break;
		case 5:
			vs.vg.c_h = vs.bs.y & 0xfff;
			break;
		case 6:
			/* Loading the c_l counter starts
             * the vector counters if MSB is set
             */
			vs.vg.c_l = vs.bs.y & 0xfff;
			break;
		}

		vertigo_vgen (&vs.vg);

		/* Microcode program flow */
		switch (MC_JCON)
		{
		case 0:
			jcond = 1;
			break;
		case S_MSB:
			/* ALU most significant bit */
			jcond = (vs.bs.f >> 15) & 1;
			break;
		case S_FEQ0:
			/* ALU is 0 */
			jcond = (vs.bs.f==0)? 1 : 0;
			break;
		case S_Y10:
			jcond = (vs.bs.y >> 10) & 1;
			break;
		case S_VFIN:
			jcond = vs.vg.vfin;
			break;
		case S_FPOS:
			/* FPOS is bit 11 */
			jcond = (vs.bs.f >> 11) & 1;
			break;
		case S_INTL4:
			jcond = irq4;
			break;
		case 7:
			jcond = 1;
			break;
		default:
			jcond = 1;
			break;
		}

		jcond ^= MC_JPOS;

		if (jcond)
		{
			/* Except for JBK, address bit 8 isn't changed
               in program flow. */
			switch (MC_JMP)
			{
			case S_JBK:
				/* JBK is the only jump where MA8 is used */
				vs.pc = MC_MA;
				break;
			case S_CALL:
				/* call and store return address */
				vs.ret = (vs.pc + 1) & 0xff;
				vs.pc = (vs.pc & 0x100) | (MC_MA & 0xff);
				break;
			case S_OPT:
				/* OPT is used for microcode jump tables. The first
                   four address bits are defined by bits 12-15
                   of 2901 input (D) */
				vs.pc = (vs.pc & 0x100) | (MC_MA & 0xf0) | ((vs.bs.d >> 12) & 0xf);
				break;
			case S_RETURN:
				/* return from call */
				vs.pc = (vs.pc & 0x100) | vs.ret;
				break;
			}
		}
		else
		{
			vs.pc = (vs.pc & 0x100) | ((vs.pc + 1) & 0xff);
		}
	}

	profiler_mark(PROFILER_END);
}
