/*** TMS34010: Portable Texas Instruments TMS34010 emulator **************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
	 Parts based on code by Aaron Giles

	System dependencies:	int/long must be at least 32 bits
	                        word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "osd_dbg.h"
#include "tms34010.h"
#include "34010ops.h"
#include "driver.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif

/* For now, define the master processor to be CPU #0, the slave to be #1 */
#define CPU_MASTER  0
#define CPU_SLAVE   1

static TMS34010_Regs state;
static int *TMS34010_timer[MAX_CPU] = {0,0,0,0}; /* Display interrupt timer */
static void TMS34010_io_intcallback(int param);

static void (*WFIELD_functions[32]) (unsigned int bitaddr, unsigned int data) =
{
	WFIELD_32, WFIELD_01, WFIELD_02, WFIELD_03, WFIELD_04, WFIELD_05,
	WFIELD_06, WFIELD_07, WFIELD_08, WFIELD_09, WFIELD_10, WFIELD_11,
	WFIELD_12, WFIELD_13, WFIELD_14, WFIELD_15, WFIELD_16, WFIELD_17,
	WFIELD_18, WFIELD_19, WFIELD_20, WFIELD_21, WFIELD_22, WFIELD_23,
	WFIELD_24, WFIELD_25, WFIELD_26, WFIELD_27, WFIELD_28, WFIELD_29,
	WFIELD_30, WFIELD_31
};
static int (*RFIELD_functions[32]) (unsigned int bitaddr) =
{
	RFIELD_32, RFIELD_01, RFIELD_02, RFIELD_03, RFIELD_04, RFIELD_05,
	RFIELD_06, RFIELD_07, RFIELD_08, RFIELD_09, RFIELD_10, RFIELD_11,
	RFIELD_12, RFIELD_13, RFIELD_14, RFIELD_15, RFIELD_16, RFIELD_17,
	RFIELD_18, RFIELD_19, RFIELD_20, RFIELD_21, RFIELD_22, RFIELD_23,
	RFIELD_24, RFIELD_25, RFIELD_26, RFIELD_27, RFIELD_28, RFIELD_29,
	RFIELD_30, RFIELD_31
};

/* public globals */
int	TMS34010_ICount=50000;

/* register definitions and shortcuts */
#define PC (state.pc)
#define N_FLAG    (state.nflag)
#define NOTZ_FLAG (state.notzflag)
#define C_FLAG    (state.cflag)
#define V_FLAG    (state.vflag)
#define P_FLAG    (state.pflag)
#define IE_FLAG   (state.ieflag)
#define FE0_FLAG  (state.fe0flag)
#define FE1_FLAG  (state.fe1flag)
#define AREG(i)   (state.Aregs[i])
#define BREG(i)   (state.Bregs[i])
#define SP        (state.Aregs[15])
#define FW(i)     (state.fw[i])
#define FW_INC(i) (state.fw_inc[i])
#define SRCREG    (((state.op)>>5)&0x0f)
#define DSTREG    ((state.op)&0x0f)
#define PARAM_WORD ( ROPARG() )
#define SKIP_WORD ( PC += (2<<3) )
#define SKIP_LONG ( PC += (4<<3) )
#define PARAM_K (((state.op)>>5)&0x1f)
#define PARAM_N ((state.op)&0x1f)
#define PARAM_REL8 ((signed char) ((state.op)&0x00ff))
#define WFIELD0(a,b) state.F0_write(a,b)
#define WFIELD1(a,b) state.F1_write(a,b)
#define RFIELD0(a)   state.F0_read(a)
#define RFIELD1(a)   state.F1_read(a)
#define WPIXEL(a,b)  state.pixel_write(a,b)
#define RPIXEL(a)    state.pixel_read(a)

/* Implied Operands */
#define SADDR  BREG(0)
#define SPTCH  BREG(1)
#define DADDR  BREG(2)
#define DPTCH  BREG(3)
#define OFFSET BREG(4)
#define WSTART BREG(5)
#define WEND   BREG(6)
#define DYDX   BREG(7)
#define COLOR0 BREG(8)
#define COLOR1 BREG(9)
#define COUNT  BREG(10)
#define INC1   BREG(11)
#define INC2   BREG(12)
#define PATTRN BREG(13)
#define TEMP   BREG(14)

/* set the field widths - shortcut */
INLINE void SET_FW(void)
{
	FW_INC(0) = (FW(0) ? FW(0) : 0x20);
	FW_INC(1) = (FW(1) ? FW(1) : 0x20);
	state.F0_write = WFIELD_functions[FW(0)];
	state.F1_write = WFIELD_functions[FW(1)];
	state.F0_read  = RFIELD_functions[FW(0)];
	state.F1_read  = RFIELD_functions[FW(1)];
}
	
/* Intialize Status to 0x0010 */
INLINE void RESET_ST(void)
{
	N_FLAG = C_FLAG = V_FLAG = P_FLAG = IE_FLAG = FE0_FLAG = FE1_FLAG = 0;
	NOTZ_FLAG = 1;
	FW(0) = 0x10;
	FW(1) = 0;
	SET_FW();
}

/* Combine indiviual flags into the Status Register */
INLINE unsigned int GET_ST(void)
{
	return (N_FLAG    ? 0x80000000 : 0) |
		   (C_FLAG    ? 0x40000000 : 0) |
		   (NOTZ_FLAG ? 0 : 0x20000000) |
		   (V_FLAG    ? 0x10000000 : 0) |
		   (P_FLAG    ? 0x02000000 : 0) |
		   (IE_FLAG   ? 0x00200000 : 0) |
		   (FE0_FLAG  ? 0x00000020 : 0) |
		   (FE1_FLAG  ? 0x00000800 : 0) |
		   FW(0) |
		  (FW(1) << 6);
}

/* Break up Status Register into indiviual flags */
INLINE void SET_ST(unsigned int st)
{
	N_FLAG    =    st & 0x80000000;
	C_FLAG    =    st & 0x40000000;
	NOTZ_FLAG =  !(st & 0x20000000);
	V_FLAG    =    st & 0x10000000;
	P_FLAG    =    st & 0x02000000;
	IE_FLAG   =    st & 0x00200000;
	FE0_FLAG  =    st & 0x00000020;
	FE1_FLAG  =    st & 0x00000800;
	FW(0)     =    st & 0x1f;
	FW(1)     =   (st >> 6) & 0x1f;
	SET_FW();
}

/* shortcuts for reading opcodes */
INLINE int ROPCODE (void)
{
	int pc = PC>>3;
	PC += (2<<3);
	return cpu_readop16(pc);
}
INLINE int ROPARG (void)
{
	int pc = PC>>3;
	PC += (2<<3);
	return cpu_readop_arg16(pc);
}
INLINE int PARAM_LONG (void)
{
	int lo = ROPARG();
	return lo | (ROPARG() << 16);
}
/* read memory byte */
INLINE int RBYTE (int bitaddr)
{
	return RFIELD_08 (bitaddr);
}

/* write memory byte */
INLINE void WBYTE (int bitaddr, int data)
{
	WFIELD_08 (bitaddr,data);
}

//* read memory long */
INLINE int RLONG (int bitaddr)
{
	return RFIELD_32 (bitaddr);
}
/* write memory long */
INLINE void WLONG (int bitaddr,int data)
{
	WFIELD_32 (bitaddr,data);
}


/* pushes/pops a value from the stack */
INLINE void PUSH (int val)
{
	SP -= 0x20;
	WLONG (SP, val);
	COPY_ASP;
}

INLINE int POP (void)
{
	int result = RLONG (SP);
	SP += 0x20;
	COPY_ASP;
	return result;
}


/* No Raster Op + No Transparency */
#define WP(m1,m2)  																		\
	int boundary = 0;	 																\
	int a = (address&0xfffffff0)>>3;													\
	int shiftcount = (address&m1);														\
	if (state.lastpixaddr != a)															\
	{																					\
		if (state.lastpixaddr != INVALID_PIX_ADDRESS)									\
		{																				\
			TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);					\
			boundary = 1;																\
		}																				\
		state.lastpixword = TMS34010_RDMEM_WORD(a);										\
		state.lastpixaddr = a;															\
	}																					\
																						\
	/* TODO: plane masking */															\
																						\
	value &= m2;																		\
	state.lastpixword = (state.lastpixword & ~(m2<<shiftcount)) | (value<<shiftcount);	\
																						\
	return boundary;


/* No Raster Op + Transparency */
#define WP_T(m1,m2)  																	\
	int boundary = 0;	 																\
	int a = (address&0xfffffff0)>>3;													\
	if (state.lastpixaddr != a)															\
	{																					\
		if (state.lastpixaddr != INVALID_PIX_ADDRESS)									\
		{																				\
			if (state.lastpixwordchanged)												\
			{																			\
				TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);				\
			}																			\
			boundary = 1;																\
		}																				\
		state.lastpixword = TMS34010_RDMEM_WORD(a);										\
		state.lastpixaddr = a;															\
		state.lastpixwordchanged = 0;													\
	}																					\
																						\
	/* TODO: plane masking */															\
																						\
	value &= m2;																		\
	if (value)																			\
	{																					\
		int shiftcount = (address&m1);													\
		state.lastpixword = (state.lastpixword & ~(m2<<shiftcount)) | (value<<shiftcount);	\
		state.lastpixwordchanged = 1;													\
	}						  															\
																						\
	return boundary;


/* Raster Op + No Transparency */
#define WP_R(m1,m2)  																	\
	int oldpix;																			\
	int boundary = 0;	 																\
	int a = (address&0xfffffff0)>>3;													\
	int shiftcount = (address&m1);														\
	if (state.lastpixaddr != a)															\
	{																					\
		if (state.lastpixaddr != INVALID_PIX_ADDRESS)									\
		{																				\
			TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);					\
			boundary = 1;																\
		}																				\
		state.lastpixword = TMS34010_RDMEM_WORD(a);										\
		state.lastpixaddr = a;															\
	}																					\
																						\
	/* TODO: plane masking */															\
																						\
	oldpix = (state.lastpixword >> shiftcount) & m2;									\
	value = state.raster_op(value & m2, oldpix) & m2;									\
																						\
	state.lastpixword = (state.lastpixword & ~(m2<<shiftcount)) | (value<<shiftcount);	\
																						\
	return boundary;


/* Raster Op + Transparency */
#define WP_R_T(m1,m2)  																	\
	int oldpix;																			\
	int boundary = 0;	 																\
	int a = (address&0xfffffff0)>>3;													\
	int shiftcount = (address&m1);														\
	if (state.lastpixaddr != a)															\
	{																					\
		if (state.lastpixaddr != INVALID_PIX_ADDRESS)									\
		{																				\
			if (state.lastpixwordchanged)												\
			{																			\
				TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);				\
			}																			\
			boundary = 1;																\
		}																				\
		state.lastpixword = TMS34010_RDMEM_WORD(a);										\
		state.lastpixaddr = a;															\
		state.lastpixwordchanged = 0;													\
	}																					\
																						\
	/* TODO: plane masking */															\
																						\
	oldpix = (state.lastpixword >> shiftcount) & m2;									\
	value = state.raster_op(value & m2, oldpix) & m2;									\
																						\
	if (value)																			\
	{																					\
		state.lastpixword = (state.lastpixword & ~(m2<<shiftcount)) | (value<<shiftcount);	\
		state.lastpixwordchanged = 1;													\
	}						  															\
																						\
	return boundary;


/* These functions return 'true' on word boundary, 'false' otherwise */

/* No Raster Op + No Transparency */
static int write_pixel_1 (unsigned int address, unsigned int value) { WP(0x0f,0x01); }
static int write_pixel_2 (unsigned int address, unsigned int value) { WP(0x0e,0x03); }
static int write_pixel_4 (unsigned int address, unsigned int value) { WP(0x0c,0x0f); }
static int write_pixel_8 (unsigned int address, unsigned int value) { WP(0x08,0xff); }
static int write_pixel_16(unsigned int address, unsigned int value)
{
	// TODO: plane masking

	TMS34010_WRMEM_WORD((address&0xfffffff0)>>3, value);		
	return 1;
}


/* No Raster Op + Transparency */
static int write_pixel_t_1 (unsigned int address, unsigned int value) { WP_T(0x0f,0x01); }
static int write_pixel_t_2 (unsigned int address, unsigned int value) { WP_T(0x0e,0x03); }
static int write_pixel_t_4 (unsigned int address, unsigned int value) { WP_T(0x0c,0x0f); }
static int write_pixel_t_8 (unsigned int address, unsigned int value) { WP_T(0x08,0xff); }
static int write_pixel_t_16(unsigned int address, unsigned int value)
{
	// TODO: plane masking

	// Transparency checking
	if (value)
	{
		TMS34010_WRMEM_WORD((address&0xfffffff0)>>3, value);		
	}

	return 1;
}


/* Raster Op + No Transparency */
static int write_pixel_r_1 (unsigned int address, unsigned int value) { WP_R(0x0f,0x01); }
static int write_pixel_r_2 (unsigned int address, unsigned int value) { WP_R(0x0e,0x03); }
static int write_pixel_r_4 (unsigned int address, unsigned int value) { WP_R(0x0c,0x0f); }
static int write_pixel_r_8 (unsigned int address, unsigned int value) { WP_R(0x08,0xff); }
static int write_pixel_r_16(unsigned int address, unsigned int value)
{
	// TODO: plane masking

	int a = (address&0xfffffff0)>>3;

	TMS34010_WRMEM_WORD(a, state.raster_op(value, TMS34010_RDMEM_WORD(a)));

	return 1;
}


/* Raster Op + Transparency */
static int write_pixel_r_t_1 (unsigned int address, unsigned int value) { WP_R_T(0x0f,0x01); }
static int write_pixel_r_t_2 (unsigned int address, unsigned int value) { WP_R_T(0x0e,0x03); }
static int write_pixel_r_t_4 (unsigned int address, unsigned int value) { WP_R_T(0x0c,0x0f); }
static int write_pixel_r_t_8 (unsigned int address, unsigned int value) { WP_R_T(0x08,0xff); }
static int write_pixel_r_t_16(unsigned int address, unsigned int value)
{
	// TODO: plane masking

	int a = (address&0xfffffff0)>>3;
	value = state.raster_op(value, TMS34010_RDMEM_WORD(a));

	// Transparency checking
	if (value)
	{
		TMS34010_WRMEM_WORD(a, value);		
	}

	return 1;
}



#define RP(m1,m2)  											\
	/* TODO: Plane masking */								\
	return (TMS34010_RDMEM_WORD((address&0xfffffff0)>>3) >> (address&m1)) & m2;

static int read_pixel_1 (unsigned int address) { RP(0x0f,0x01) }
static int read_pixel_2 (unsigned int address) { RP(0x0e,0x03) }
static int read_pixel_4 (unsigned int address) { RP(0x0c,0x0f) }
static int read_pixel_8 (unsigned int address) { RP(0x08,0xff) }
static int read_pixel_16(unsigned int address)
{
	// TODO: Plane masking
	return TMS34010_RDMEM_WORD((address&0xfffffff0)>>3);	
}


#define FINISH_PIX_OP												\
	if (state.lastpixaddr != INVALID_PIX_ADDRESS)					\
	{																\
		TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);	\
	}																\
	state.lastpixaddr = INVALID_PIX_ADDRESS;						\
	P_FLAG = 0;


static int write_pixel_shiftreg (unsigned int address, unsigned int value)
{
	if (state.from_shiftreg)
	{
		state.from_shiftreg(address, &state.shiftreg[0]);		
	}
	else
	{
		if (errorlog)
		{
			fprintf(errorlog, "From ShiftReg function not set. PC = %08X\n", PC);			
		}
	}
	return 1;
}

static int read_pixel_shiftreg (unsigned int address)
{
	if (state.to_shiftreg)
	{
		state.to_shiftreg(address, &state.shiftreg[0]);		
	}
	else
	{
		if (errorlog)
		{
			fprintf(errorlog, "To ShiftReg function not set. PC = %08X\n", PC);			
		}
	}
	return state.shiftreg[0];
}

/* includes the static function prototypes and the master opcode table */
#include "34010tbl.c"

/* includes the actual opcode implementations */
#include "34010ops.c"


/* Raster operations */
static int raster_op_1(int newpix, int oldpix)
{
	/*  S AND D -> D */
	return newpix & oldpix;
}
static int raster_op_2(int newpix, int oldpix)
{
	/*  S AND ~D -> D */
	return newpix & ~oldpix;
}
static int raster_op_3(int newpix, int oldpix)
{
	/*  0 -> D */
	return 0;
}
static int raster_op_4(int newpix, int oldpix)
{
	/*  S OR ~D -> D */
	return newpix | ~oldpix;
}
static int raster_op_5(int newpix, int oldpix)
{
	/* FIXME!!! Not sure about this one? */
	/*  S XNOR D -> D */
	return ~(newpix ^ oldpix);
}
static int raster_op_6(int newpix, int oldpix)
{
	/*  ~D -> D */
	return ~oldpix;
}
static int raster_op_7(int newpix, int oldpix)
{
	/*  S NOR D -> D */
	return ~(newpix | oldpix);
}
static int raster_op_8(int newpix, int oldpix)
{
	/*  S OR D -> D */
	return newpix | oldpix;
}
static int raster_op_9(int newpix, int oldpix)
{
	/*  D -> D */
	return oldpix;
}
static int raster_op_10(int newpix, int oldpix)
{
	/*  S XOR D -> D */
	return newpix ^ oldpix;
}
static int raster_op_11(int newpix, int oldpix)
{
	/*  ~S AND D -> D */
	return ~newpix & oldpix;
}
static int raster_op_12(int newpix, int oldpix)
{
	/*  1 -> D */
	return 0xffff;
}
static int raster_op_13(int newpix, int oldpix)
{
	/*  ~S OR D -> D */
	return ~newpix | oldpix;
}
static int raster_op_14(int newpix, int oldpix)
{
	/*  S NAND D -> D */
	return ~(newpix & oldpix);
}
static int raster_op_15(int newpix, int oldpix)
{
	/*  ~S -> D */
	return ~newpix;
}
static int raster_op_16(int newpix, int oldpix)
{
	/*  S + D -> D */
	return newpix + oldpix;
}
static int raster_op_17(int newpix, int oldpix)
{
	/*  S + D -> D with Saturation*/
	int max = (unsigned int)0xffffffff>>(32-IOREG(REG_PSIZE));
	int res = newpix + oldpix;
	return (res > max) ? max : res;
}
static int raster_op_18(int newpix, int oldpix)
{
	/*  D - S -> D */
	return oldpix - newpix;
}
static int raster_op_19(int newpix, int oldpix)
{
	/*  D - S -> D with Saturation */
	int res = oldpix - newpix;
	return (res < 0) ? 0 : res;
}
static int raster_op_20(int newpix, int oldpix)
{
	/*  MAX(S,D) -> D */
	return ((oldpix > newpix) ? oldpix : newpix);
}
static int raster_op_21(int newpix, int oldpix)
{
	/*  MIN(S,D) -> D */
	return ((oldpix > newpix) ? newpix : oldpix);
}


/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void TMS34010_SetRegs(TMS34010_Regs *Regs)
{
	state = *Regs;
	change_pc29(PC)
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void TMS34010_GetRegs(TMS34010_Regs *Regs)
{
	*Regs = state;
}

/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned TMS34010_GetPC(void)
{
	return PC;
}


TMS34010_Regs* TMS34010_GetState(void)
{
	return &state;
}


void TMS34010_Reset(void)
{
	int i;
	extern unsigned char *RAM;
	memset (&state, 0, sizeof (state));
	state.lastpixaddr = INVALID_PIX_ADDRESS;					
	PC = RLONG(0xffffffe0);
	change_pc29(PC)
	RESET_ST();

	state.shiftreg = malloc(SHIFTREG_SIZE);

	/* The slave CPU starts out halted */
	if (cpu_getactivecpu() == CPU_SLAVE)
	{
		IOREG(REG_HSTCTLH) = 0x8000;
		cpu_halt(cpu_getactivecpu(), 0);
	}
}


void TMS34010_Cause_Interrupt(int type)
{
	/* NONE = 0 */
	IOREG(REG_INTPEND) |= type;
}


void TMS34010_Clear_Pending_Interrupts(void)
{
	/* This doesn't apply */
}


/* Generate interrupts */
static void Interrupt(void)
{
	int take=0;

	if (IOREG(REG_INTPEND) & TMS34010_NMI)
	{
		IOREG(REG_INTPEND) &= ~TMS34010_NMI;

		if (!(IOREG(REG_HSTCTLH) & 0x0200))  // NMI mode bit
		{
			PUSH(PC);
			PUSH(GET_ST());					
		}
		RESET_ST();
		PC = RLONG(0xfffffee0);
        change_pc29(PC);
	}
	else
	{
		if ((IOREG(REG_INTPEND) & TMS34010_HI) &&
			(IOREG(REG_INTENB)  & TMS34010_HI))
		{
			take = 0xfffffec0;		
		}
		else
		if ((IOREG(REG_INTPEND) & TMS34010_DI)) /* This was only generated if enabled */
		{
			take = 0xfffffea0;		
		}
		else
		if ((IOREG(REG_INTPEND) & TMS34010_WV) &&
			(IOREG(REG_INTENB)  & TMS34010_WV))
		{
			take = 0xfffffe80;		
		}
		else
		if ((IOREG(REG_INTPEND) & TMS34010_INT1) &&
			(IOREG(REG_INTENB)  & TMS34010_INT1))
		{
			take = 0xffffffc0;		
		}
		else
		if ((IOREG(REG_INTPEND) & TMS34010_INT2) &&
			(IOREG(REG_INTENB)  & TMS34010_INT2))
		{
			take = 0xffffffa0;		
		}

		if (take)
		{
			PUSH(PC);
			PUSH(GET_ST());
			RESET_ST();
			PC = RLONG(take);
			change_pc29(PC);
		}
	}
}



/* execute instructions on this CPU until icount expires */
int TMS34010_Execute(int cycles)
{
	/* Get out if CPU is halted. Absolutely no interrupts must be taken!!! */
	if (IOREG(REG_HSTCTLH) & 0x8000)
	{
		return cycles;		
	}

	TMS34010_ICount = cycles;
	change_pc29(PC)
	do
	{
		/* Quickly reject the cases when there are no pending interrupts
		   or they are disabled (as in an interrupt service routine) */
		if (IOREG(REG_INTPEND) &&
			(IE_FLAG || (IOREG(REG_INTPEND) & TMS34010_NMI)))
		{
			Interrupt();			
		}

#ifdef	MAME_DEBUG
{
	extern int mame_debug;
	if (mame_debug)
	{
		state.st = GET_ST();
		MAME_Debug();		
	}
}
#endif
		state.op = ROPCODE ();
		(*opcode_table[state.op >> 4])();

		TMS34010_ICount -= 5;

	} while (TMS34010_ICount > 0);

	return cycles - TMS34010_ICount;
}


/****************************************************************************/
/* I/O Function prototypes 									*/
/****************************************************************************/

static int (*pixel_write_ops[4][5])(unsigned int, unsigned int)	=
{
	{write_pixel_1,     write_pixel_2,     write_pixel_4,     write_pixel_8,     write_pixel_16},
	{write_pixel_r_1,   write_pixel_r_2,   write_pixel_r_4,   write_pixel_r_8,   write_pixel_r_16},
	{write_pixel_t_1,   write_pixel_t_2,   write_pixel_t_4,   write_pixel_t_8,   write_pixel_t_16},
	{write_pixel_r_t_1, write_pixel_r_t_2, write_pixel_r_t_4, write_pixel_r_t_8, write_pixel_r_t_16}
};

static int (*pixel_read_ops[5])(unsigned int address) =
{
	read_pixel_1, read_pixel_2, read_pixel_4, read_pixel_8, read_pixel_16
};


static void set_pixel_function(void)
{
	int i1,i2;

	if (IOREG(REG_DPYCTL) & 0x0800)
	{
		/* Shift Register Transfer */
		state.pixel_write = write_pixel_shiftreg;
		state.pixel_read  = read_pixel_shiftreg;
		return;
	}

	switch (IOREG(REG_PSIZE))
	{
	default:
	case 0x01: i2 = 0; break;
	case 0x02: i2 = 1; break;
	case 0x04: i2 = 2; break;
	case 0x08: i2 = 3; break;
	case 0x10: i2 = 4; break;
	}

	if (state.transparency)
	{
		if (state.raster_op)
		{
			i1 = 3;			
		}
		else
		{
			i1 = 2;
		}
	}
	else
	{
		if (state.raster_op)
		{
			i1 = 1;			
		}
		else
		{
			i1 = 0;
		}		
	}

	state.pixel_write = pixel_write_ops[i1][i2];
	state.pixel_read  = pixel_read_ops [i2];
}


static int (*raster_ops[32]) (int newpix, int oldpix) =
{
	           0, raster_op_1 , raster_op_2 , raster_op_3,
	raster_op_4 , raster_op_5 , raster_op_6 , raster_op_7,
	raster_op_8 , raster_op_9 , raster_op_10, raster_op_11,
	raster_op_12, raster_op_13, raster_op_14, raster_op_15,
	raster_op_16, raster_op_17, raster_op_18, raster_op_19,
	raster_op_20, raster_op_21,            0,            0,
	           0,            0,            0,            0,
	           0,            0,            0,            0,
};

static void set_raster_op(void)
{
	state.raster_op = raster_ops[(IOREG(REG_CONTROL) >> 10) & 0x1f];
}

void TMS34010_io_register_w(int reg, int data)
{
	int cpu;
							
	/* Set register */
	reg >>= 1;
	IOREG(reg) = data;		

	switch (reg)
	{
	case REG_DPYINT:
		/* Set display interrupt timer */
		cpu = cpu_getactivecpu();
		if (TMS34010_timer[cpu])
		{
			timer_remove(TMS34010_timer[cpu]);		
		}
		TMS34010_timer[cpu] = timer_set(cpu_getscanlinetime(data), cpu, TMS34010_io_intcallback);	
		break;

	case REG_CONTROL:
		state.transparency = data & 0x20;
		state.window_checking = (data >> 6) & 0x03;
		set_raster_op();
		set_pixel_function();
		break;

	case REG_PSIZE:
		set_pixel_function();

		switch (data)
		{
		default:
		case 0x01: state.xytolshiftcount2 = 0; break;
		case 0x02: state.xytolshiftcount2 = 1; break;
		case 0x04: state.xytolshiftcount2 = 2; break;
		case 0x08: state.xytolshiftcount2 = 3; break;
		case 0x10: state.xytolshiftcount2 = 4; break;
		}
		break;

	case REG_PMASK:
		if (data && errorlog) fprintf(errorlog, "Plane masking NOT supported. PC=%08X\n", cpu_getpc());
		break;

	case REG_DPYCTL:
		set_pixel_function();
		break;

	case REG_HSTCTLH:
		if (data & 0x8000)
		{
			/* CPU is halting itself, stop execution right away */
			TMS34010_ICount = 0;
		}
		cpu_halt(cpu_getactivecpu(), !(data & 0x8000));

		if (data & 0x0100)
		{
			/* NMI issued */
			cpu_cause_interrupt(cpu_getactivecpu(), TMS34010_NMI);
		}
		break;

	case REG_CONVDP:
		state.xytolshiftcount1 = ~data & 0x0f;
		break;
	}

	//if (errorlog)
	//{
	//	fprintf(errorlog, "TMS34010 io write. Reg #%02X=%04X - PC: %04X\n",
	//			reg, IOREG(reg), cpu_getpc());
	//}
}

int TMS34010_io_register_r(int reg)
{
	reg >>=1;
	switch (reg)
	{
	case REG_VCOUNT:
		return cpu_getscanline();

	case REG_HCOUNT:
		return cpu_gethorzbeampos();
	}

	return IOREG(reg);
}

void TMS34010_set_shiftreg_functions(int cpu,
									 void (*to_shiftreg  )(unsigned int, unsigned short*),
									 void (*from_shiftreg)(unsigned int, unsigned short*))
{
	TMS34010_Regs* context = cpu_getcontext(cpu);
	context->to_shiftreg   = to_shiftreg;		
	context->from_shiftreg = from_shiftreg;		
}

int TMS34010_io_display_blanked(int cpu)
{
	TMS34010_Regs* context = cpu_getcontext(cpu);
	return (!(context->IOregs[REG_DPYCTL] & 0x8000));
}

int TMS34010_get_DPYSTRT(int cpu)
{
	TMS34010_Regs* context = cpu_getcontext(cpu);
	return context->IOregs[REG_DPYSTRT];
}

static void TMS34010_io_intcallback(int param)
{
	TMS34010_Regs* context = cpu_getcontext(param);

	/* Reset timer for next frame */
	double interval = TIME_IN_HZ (Machine->drv->frames_per_second);
	TMS34010_timer[param] = timer_set(interval, param, TMS34010_io_intcallback);	

	/* This is not 100% accurate, but faster */
	if (context->IOregs[REG_INTENB] & TMS34010_DI)
	{
		cpu_cause_interrupt(param, TMS34010_DI);
	}
}


void TMS34010_State_Save(int cpunum, void *f)
{
	osd_fwrite(f,cpu_getcontext(cpunum),sizeof(state));
	osd_fwrite(f,&TMS34010_ICount,sizeof(int));
	osd_fwrite(f,state.shiftreg,sizeof(SHIFTREG_SIZE));
}

void TMS34010_State_Load(int cpunum, void *f)
{
	/* Don't reload the following */
	unsigned short* shiftreg_save = state.shiftreg;
	void (*to_shiftreg_save)  (unsigned int, unsigned short*) = state.to_shiftreg;
	void (*from_shiftreg_save)(unsigned int, unsigned short*) = state.from_shiftreg;

	osd_fread(f,cpu_getcontext(cpunum),sizeof(state));
	osd_fread(f,&TMS34010_ICount,sizeof(int));
	change_pc29(PC);
	SET_FW();
	TMS34010_io_register_w(REG_DPYINT<<1,IOREG(REG_DPYINT));
	set_raster_op();
	set_pixel_function();

	state.shiftreg      = shiftreg_save;
	state.to_shiftreg   = to_shiftreg_save;
	state.from_shiftreg = from_shiftreg_save;

	osd_fread(f,state.shiftreg,sizeof(SHIFTREG_SIZE));
}


/* Host interface */
static TMS34010_Regs* slavecontext;			

void TMS34010_HSTADRL_w (int offset, int data)
{
	slavecontext = cpu_getcontext(CPU_SLAVE);

    SLAVE_IOREG(REG_HSTADRL) = data & 0xfff0;
}

void TMS34010_HSTADRH_w (int offset, int data)
{
	slavecontext = cpu_getcontext(CPU_SLAVE);

    SLAVE_IOREG(REG_HSTADRH) = data;
}

void TMS34010_HSTDATA_w (int offset, int data)
{
	unsigned int addr;

	memorycontextswap (CPU_SLAVE);				
												
	addr = (SLAVE_IOREG(REG_HSTADRH) << 16) | SLAVE_IOREG(REG_HSTADRL);

    TMS34010_WRMEM_WORD(addr>>3, data);

	/* Postincrement? */
	if (SLAVE_IOREG(REG_HSTCTLH) & 0x0800)
	{
		addr += 0x10;
		SLAVE_IOREG(REG_HSTADRH) = addr >> 16;
		SLAVE_IOREG(REG_HSTADRL) = addr & 0xffff;
	}
												
	memorycontextswap (CPU_MASTER);				
	change_pc29(PC);							
}

int  TMS34010_HSTDATA_r (int offset)
{
	unsigned int addr;
	int data;									
												
	memorycontextswap (CPU_SLAVE);				
												
	addr = (SLAVE_IOREG(REG_HSTADRH) << 16) | SLAVE_IOREG(REG_HSTADRL);

	/* Preincrement? */
	if (SLAVE_IOREG(REG_HSTCTLH) & 0x1000)
	{
		addr += 0x10;
		SLAVE_IOREG(REG_HSTADRH) = addr >> 16;
		SLAVE_IOREG(REG_HSTADRL) = addr & 0xffff;
	}
	
    data = TMS34010_RDMEM_WORD(addr>>3);
												
	memorycontextswap (CPU_MASTER);				
	change_pc29(PC);							
	 											
	return data;
}

void TMS34010_HSTCTLH_w (int offset, int data)
{
	slavecontext = cpu_getcontext(CPU_SLAVE);

    SLAVE_IOREG(REG_HSTCTLH) = data;

	cpu_halt(CPU_SLAVE, !(data & 0x8000));

	if (data & 0x0100)
	{
		/* Issue NMI */
		cpu_cause_interrupt(CPU_SLAVE, TMS34010_NMI);
	}
}
