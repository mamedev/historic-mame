/*** TMS34010: Portable Texas Instruments TMS34010 emulator *****************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
	 Parts based on code by Aaron Giles

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "driver.h"
#include "osd_cpu.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "tms34010.h"
#include "34010ops.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

/* For now, define the master processor to be CPU #0, the slave to be #1 */
#define CPU_MASTER  0
#define CPU_SLAVE   1

static UINT8 tms34010_reg_layout[] = {
	TMS34010_PC, TMS34010_SP, -1,
	TMS34010_A0, TMS34010_B0, -1,
	TMS34010_A1, TMS34010_B1, -1,
	TMS34010_A2, TMS34010_B2, -1,
	TMS34010_A3, TMS34010_B3, -1,
	TMS34010_A4, TMS34010_B4, -1,
	TMS34010_A5, TMS34010_B5, -1,
	TMS34010_A6, TMS34010_B6, -1,
	TMS34010_A7, TMS34010_B7, -1,
	TMS34010_A8, TMS34010_B8, -1,
	TMS34010_A9, TMS34010_B9, -1,
	TMS34010_A10,TMS34010_B10,-1,
	TMS34010_A11,TMS34010_B11,-1,
	TMS34010_A12,TMS34010_B12,-1,
	TMS34010_A13,TMS34010_B13,-1,
	TMS34010_A14,TMS34010_B14, 0
};

static UINT8 tms34010_win_layout[] = {
	40, 0,39,17,	/* register window (top right) */
	 0, 0,39,17,	/* disassembler window (left, upper) */
	 0,18,39, 4,	/* memory #1 window (left, middle) */
	40,18,39, 4,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

/* TMS34010 State */
typedef struct
{
#if LSB_FIRST
	INT16 x;
	INT16 y;
#else
	INT16 y;
	INT16 x;
#endif
} XY;

typedef struct
{
    UINT32 op;
    UINT32 pc;
    UINT32 st;        /* Only here so we can display it in the debug window */
    union                       /* The register files are interleaved, so */
    {                           /* that the SP occupies the same location in both */
        INT32 Bregs[241];   	/* Only every 16th entry is actually used */
        XY BregsXY[241];
        struct
        {
            INT32 unused[225];
            union
            {
            	INT32 Aregs[16];
	            XY AregsXY[16];
			} a;
        } a;
    } regs;
    UINT32 nflag;
    UINT32 cflag;
    UINT32 notzflag;  /* So we can just do an assignment to set it */
    UINT32 vflag;
    UINT32 pflag;
    UINT32 ieflag;
    UINT32 fe0flag;
    UINT32 fe1flag;
    UINT32 fw[2];
    UINT32 fw_inc[2];  /* Same as fw[], except when fw = 0, fw_inc = 32 */
    UINT32 IOregs[32];
    void (*F0_write) (UINT32 bitaddr, UINT32 data);
    void (*F1_write) (UINT32 bitaddr, UINT32 data);
     INT32 (*F0_read) (UINT32 bitaddr);
     INT32 (*F1_read) (UINT32 bitaddr);
    UINT32 (*pixel_write)(UINT32 address, UINT32 value);
    UINT32 (*pixel_read)(UINT32 address);
    UINT32 transparency;
    UINT32 window_checking;
     INT32 (*raster_op)(INT32 newpix, INT32 oldpix);
    UINT32 lastpixaddr;
    UINT32 lastpixword;
    UINT32 lastpixwordchanged;
    UINT32 xytolshiftcount1;
    UINT32 xytolshiftcount2;
    UINT16* shiftreg;
    void (*to_shiftreg)  (UINT32 address, UINT16* shiftreg);
    void (*from_shiftreg)(UINT32 address, UINT16* shiftreg);
    UINT8* stackbase;
    UINT32 stackoffs;
    int (*irq_callback)(int irqline);
} TMS34010_Regs;

static TMS34010_Regs state;
static int *TMS34010_timer[MAX_CPU];          /* Display interrupt timer */
static UINT8* stackbase[MAX_CPU] = {0,0,0,0};
static UINT32 stackoffs[MAX_CPU] = {0,0,0,0};
static void (*to_shiftreg  [MAX_CPU])(UINT32, UINT16*) = {0,0,0,0};
static void (*from_shiftreg[MAX_CPU])(UINT32, UINT16*) = {0,0,0,0};

static void TMS34010_io_intcallback(int param);

static void check_interrupt(void);

static void (*wfield_functions[32]) (UINT32 bitaddr, UINT32 data) =
{
	wfield_32, wfield_01, wfield_02, wfield_03, wfield_04, wfield_05,
	wfield_06, wfield_07, wfield_08, wfield_09, wfield_10, wfield_11,
	wfield_12, wfield_13, wfield_14, wfield_15, wfield_16, wfield_17,
	wfield_18, wfield_19, wfield_20, wfield_21, wfield_22, wfield_23,
	wfield_24, wfield_25, wfield_26, wfield_27, wfield_28, wfield_29,
	wfield_30, wfield_31
};
static INT32 (*rfield_functions_z[32]) (UINT32 bitaddr) =
{
	rfield_32  , rfield_z_01, rfield_z_02, rfield_z_03, rfield_z_04, rfield_z_05,
	rfield_z_06, rfield_z_07, rfield_z_08, rfield_z_09, rfield_z_10, rfield_z_11,
	rfield_z_12, rfield_z_13, rfield_z_14, rfield_z_15, rfield_z_16, rfield_z_17,
	rfield_z_18, rfield_z_19, rfield_z_20, rfield_z_21, rfield_z_22, rfield_z_23,
	rfield_z_24, rfield_z_25, rfield_z_26, rfield_z_27, rfield_z_28, rfield_z_29,
	rfield_z_30, rfield_z_31
};
static INT32 (*rfield_functions_s[32]) (UINT32 bitaddr) =
{
	rfield_32  , rfield_s_01, rfield_s_02, rfield_s_03, rfield_s_04, rfield_s_05,
	rfield_s_06, rfield_s_07, rfield_s_08, rfield_s_09, rfield_s_10, rfield_s_11,
	rfield_s_12, rfield_s_13, rfield_s_14, rfield_s_15, rfield_s_16, rfield_s_17,
	rfield_s_18, rfield_s_19, rfield_s_20, rfield_s_21, rfield_s_22, rfield_s_23,
	rfield_s_24, rfield_s_25, rfield_s_26, rfield_s_27, rfield_s_28, rfield_s_29,
	rfield_s_30, rfield_s_31
};

/* public globals */
int	tms34010_ICount;

/* register definitions and shortcuts */
#define PC         (state.pc)
#define ST         (state.st)
#define N_FLAG     (state.nflag)
#define NOTZ_FLAG  (state.notzflag)
#define C_FLAG     (state.cflag)
#define V_FLAG     (state.vflag)
#define P_FLAG     (state.pflag)
#define IE_FLAG    (state.ieflag)
#define FE0_FLAG   (state.fe0flag)
#define FE1_FLAG   (state.fe1flag)
#define AREG(i)    (state.regs.a.a.Aregs[i])
#define AREG_XY(i) (state.regs.a.a.AregsXY[i])
#define AREG_X(i)  (state.regs.a.a.AregsXY[i].x)
#define AREG_Y(i)  (state.regs.a.a.AregsXY[i].y)
#define BREG(i)    (state.regs.Bregs[i])
#define BREG_XY(i) (state.regs.BregsXY[i])
#define BREG_X(i)  (state.regs.BregsXY[i].x)
#define BREG_Y(i)  (state.regs.BregsXY[i].y)
#define SP         (state.regs.a.a.Aregs[15])
#define FW(i)      (state.fw[i])
#define FW_INC(i)  (state.fw_inc[i])
#define ASRCREG  (((state.op)>>5)&0x0f)
#define ADSTREG   ((state.op)    &0x0f)
#define BSRCREG  (((state.op)&0x1e0)>>1)
#define BDSTREG  (((state.op)&0x0f)<<4)
#define SKIP_WORD (PC += (2<<3))
#define SKIP_LONG (PC += (4<<3))
#define PARAM_K   (((state.op)>>5)&0x1f)
#define PARAM_N    ((state.op)&0x1f)
#define PARAM_REL8 ((signed char) ((state.op)&0x00ff))
#define WFIELD0(a,b) state.F0_write(a,b)
#define WFIELD1(a,b) state.F1_write(a,b)
#define RFIELD0(a)   state.F0_read(a)
#define RFIELD1(a)   state.F1_read(a)
#define WPIXEL(a,b)  state.pixel_write(a,b)
#define RPIXEL(a)    state.pixel_read(a)

/* Implied Operands */
#define SADDR	    BREG(0<<4)
#define SADDR_X   BREG_X(0<<4)
#define SADDR_Y   BREG_Y(0<<4)
#define SADDR_XY BREG_XY(0<<4)
#define SPTCH       BREG(1<<4)
#define DADDR       BREG(2<<4)
#define DADDR_X   BREG_X(2<<4)
#define DADDR_Y   BREG_Y(2<<4)
#define DADDR_XY BREG_XY(2<<4)
#define DPTCH       BREG(3<<4)
#define OFFSET      BREG(4<<4)
#define WSTART_X  BREG_X(5<<4)
#define WSTART_Y  BREG_Y(5<<4)
#define WEND_X    BREG_X(6<<4)
#define WEND_Y    BREG_Y(6<<4)
#define DYDX_X    BREG_X(7<<4)
#define DYDX_Y    BREG_Y(7<<4)
#define COLOR0      BREG(8<<4)
#define COLOR1      BREG(9<<4)
#define COUNT       BREG(10<<4)
#define INC1_X    BREG_X(11<<4)
#define INC1_Y    BREG_Y(11<<4)
#define INC2_X    BREG_X(12<<4)
#define INC2_Y    BREG_Y(12<<4)
#define PATTRN      BREG(13<<4)
#define TEMP        BREG(14<<4)

/* set the field widths - shortcut */
INLINE void SET_FW(void)
{
	FW_INC(0) = (FW(0) ? FW(0) : 0x20);
	FW_INC(1) = (FW(1) ? FW(1) : 0x20);

	state.F0_write = wfield_functions[FW(0)];
	state.F1_write = wfield_functions[FW(1)];

	if (FE0_FLAG)
	{
		state.F0_read  = rfield_functions_s[FW(0)];	/* Sign extend */
	}
	else
	{
		state.F0_read  = rfield_functions_z[FW(0)];	/* Zero extend */
	}

	if (FE1_FLAG)
	{
		state.F1_read  = rfield_functions_s[FW(1)];	/* Sign extend */
	}
	else
	{
		state.F1_read  = rfield_functions_z[FW(1)];	/* Zero extend */
	}
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
INLINE UINT32 GET_ST(void)
{
	return (     N_FLAG ? 0x80000000 : 0) |
		   (     C_FLAG ? 0x40000000 : 0) |
		   (  NOTZ_FLAG ? 0 : 0x20000000) |
		   (     V_FLAG ? 0x10000000 : 0) |
		   (     P_FLAG ? 0x02000000 : 0) |
		   (    IE_FLAG ? 0x00200000 : 0) |
		   (   FE0_FLAG ? 0x00000020 : 0) |
		   (   FE1_FLAG ? 0x00000800 : 0) |
		   FW(0) |
		  (FW(1) << 6);
}

/* Break up Status Register into indiviual flags */
INLINE void SET_ST(UINT32 st)
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

	/* interrupts might have been enabled, check it */
	check_interrupt();
}

/* shortcuts for reading opcodes */
INLINE UINT32 ROPCODE (void)
{
	UINT32 pc = TOBYTE(PC);
	PC += (2<<3);
	return cpu_readop16(pc);
}
INLINE INT16 PARAM_WORD (void)
{
	UINT32 pc = TOBYTE(PC);
	PC += (2<<3);
	return cpu_readop_arg16(pc);
}
INLINE INT16 PARAM_WORD_NO_INC (void)
{
    return cpu_readop_arg16(TOBYTE(PC));
}
INLINE INT32 PARAM_LONG_NO_INC (void)
{
	UINT32 pc = TOBYTE(PC);
	return cpu_readop_arg16(pc) | ((UINT32)(UINT16)cpu_readop_arg16(pc+2) << 16);
}
INLINE INT32 PARAM_LONG (void)
{
	INT32 ret = PARAM_LONG_NO_INC();
	PC += (4<<3);
	return ret;
}

/* read memory byte */
INLINE INT8 RBYTE (UINT32 bitaddr)
{
	RFIELDMAC_Z_8;
}

/* write memory byte */
INLINE void WBYTE (UINT32 bitaddr, UINT32 data)
{
    WFIELDMAC_8;
}

/* read memory long */
INLINE INT32 RLONG (UINT32 bitaddr)
{
	RFIELDMAC_32;
}
/* write memory long */
INLINE void WLONG (UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC_32;
}


/* pushes/pops a value from the stack

   These are called millions of times. If you change it, please test effect
   on performance */

INLINE void PUSH (UINT32 data)
{
	UINT8* base;
	SP -= 0x20;
	base = STACKPTR(SP);
	WRITE_WORD(base, (UINT16)data);
	WRITE_WORD(base+2, data >> 16);
}

INLINE INT32 POP (void)
{
	UINT8* base = STACKPTR(SP);
	INT32 ret = READ_WORD(base) + (READ_WORD(base+2) << 16);
	SP += 0x20;
	return ret;
}


/* No Raster Op + No Transparency */
#define WP(m1,m2)  																		\
	UINT32 boundary = 0;	 															\
	UINT32 a = TOBYTE(address&0xfffffff0);												\
	UINT32 shiftcount = (address&m1);													\
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
	UINT32 boundary = 0;	 															\
	UINT32 a = TOBYTE(address&0xfffffff0);												\
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
		UINT32 shiftcount = (address&m1);												\
		state.lastpixword = (state.lastpixword & ~(m2<<shiftcount)) | (value<<shiftcount);	\
		state.lastpixwordchanged = 1;													\
	}						  															\
																						\
	return boundary;


/* Raster Op + No Transparency */
#define WP_R(m1,m2)  																	\
	UINT32 oldpix;																		\
	UINT32 boundary = 0;	 															\
	UINT32 a = TOBYTE(address&0xfffffff0);												\
	UINT32 shiftcount = (address&m1);													\
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
	UINT32 oldpix;																		\
	UINT32 boundary = 0;	 															\
	UINT32 a = TOBYTE(address&0xfffffff0);												\
	UINT32 shiftcount = (address&m1);													\
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
static UINT32 write_pixel_1 (UINT32 address, UINT32 value) { WP(0x0f,0x01); }
static UINT32 write_pixel_2 (UINT32 address, UINT32 value) { WP(0x0e,0x03); }
static UINT32 write_pixel_4 (UINT32 address, UINT32 value) { WP(0x0c,0x0f); }
static UINT32 write_pixel_8 (UINT32 address, UINT32 value) { WP(0x08,0xff); }
static UINT32 write_pixel_16(UINT32 address, UINT32 value)
{
	/* TODO: plane masking */

	TMS34010_WRMEM_WORD(TOBYTE(address&0xfffffff0), value);
	return 1;
}


/* No Raster Op + Transparency */
static UINT32 write_pixel_t_1 (UINT32 address, UINT32 value) { WP_T(0x0f,0x01); }
static UINT32 write_pixel_t_2 (UINT32 address, UINT32 value) { WP_T(0x0e,0x03); }
static UINT32 write_pixel_t_4 (UINT32 address, UINT32 value) { WP_T(0x0c,0x0f); }
static UINT32 write_pixel_t_8 (UINT32 address, UINT32 value) { WP_T(0x08,0xff); }
static UINT32 write_pixel_t_16(UINT32 address, UINT32 value)
{
	/* TODO: plane masking */

	/* Transparency checking */
	if (value)
	{
		TMS34010_WRMEM_WORD(TOBYTE(address&0xfffffff0), value);
	}

	return 1;
}


/* Raster Op + No Transparency */
static UINT32 write_pixel_r_1 (UINT32 address, UINT32 value) { WP_R(0x0f,0x01); }
static UINT32 write_pixel_r_2 (UINT32 address, UINT32 value) { WP_R(0x0e,0x03); }
static UINT32 write_pixel_r_4 (UINT32 address, UINT32 value) { WP_R(0x0c,0x0f); }
static UINT32 write_pixel_r_8 (UINT32 address, UINT32 value) { WP_R(0x08,0xff); }
static UINT32 write_pixel_r_16(UINT32 address, UINT32 value)
{
	/* TODO: plane masking */

	UINT32 a = TOBYTE(address&0xfffffff0);

	TMS34010_WRMEM_WORD(a, state.raster_op(value, TMS34010_RDMEM_WORD(a)));

	return 1;
}


/* Raster Op + Transparency */
static UINT32 write_pixel_r_t_1 (UINT32 address, UINT32 value) { WP_R_T(0x0f,0x01); }
static UINT32 write_pixel_r_t_2 (UINT32 address, UINT32 value) { WP_R_T(0x0e,0x03); }
static UINT32 write_pixel_r_t_4 (UINT32 address, UINT32 value) { WP_R_T(0x0c,0x0f); }
static UINT32 write_pixel_r_t_8 (UINT32 address, UINT32 value) { WP_R_T(0x08,0xff); }
static UINT32 write_pixel_r_t_16(UINT32 address, UINT32 value)
{
	/* TODO: plane masking */

	UINT32 a = TOBYTE(address&0xfffffff0);
	value = state.raster_op(value, TMS34010_RDMEM_WORD(a));

	/* Transparency checking */
	if (value)
	{
		TMS34010_WRMEM_WORD(a, value);
	}

	return 1;
}



#define RP(m1,m2)  											\
	/* TODO: Plane masking */								\
	return (TMS34010_RDMEM_WORD(TOBYTE(address&0xfffffff0)) >> (address&m1)) & m2;

static UINT32 read_pixel_1 (UINT32 address) { RP(0x0f,0x01) }
static UINT32 read_pixel_2 (UINT32 address) { RP(0x0e,0x03) }
static UINT32 read_pixel_4 (UINT32 address) { RP(0x0c,0x0f) }
static UINT32 read_pixel_8 (UINT32 address) { RP(0x08,0xff) }
static UINT32 read_pixel_16(UINT32 address)
{
	/* TODO: Plane masking */
	return TMS34010_RDMEM_WORD(TOBYTE(address&0xfffffff0));
}


#define FINISH_PIX_OP												\
	if (state.lastpixaddr != INVALID_PIX_ADDRESS)					\
	{																\
		TMS34010_WRMEM_WORD(state.lastpixaddr, state.lastpixword);	\
	}																\
	state.lastpixaddr = INVALID_PIX_ADDRESS;						\
	P_FLAG = 0;


static UINT32 write_pixel_shiftreg (UINT32 address, UINT32 value)
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

static UINT32 read_pixel_shiftreg (UINT32 address)
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
static INT32 raster_op_1(INT32 newpix, INT32 oldpix)
{
	/*  S AND D -> D */
	return newpix & oldpix;
}
static INT32 raster_op_2(INT32 newpix, INT32 oldpix)
{
	/*  S AND ~D -> D */
	return newpix & ~oldpix;
}
static INT32 raster_op_3(INT32 newpix, INT32 oldpix)
{
	/*  0 -> D */
	return 0;
}
static INT32 raster_op_4(INT32 newpix, INT32 oldpix)
{
	/*  S OR ~D -> D */
	return newpix | ~oldpix;
}
static INT32 raster_op_5(INT32 newpix, INT32 oldpix)
{
	/* FIXME!!! Not sure about this one? */
	/*  S XNOR D -> D */
	return ~(newpix ^ oldpix);
}
static INT32 raster_op_6(INT32 newpix, INT32 oldpix)
{
	/*  ~D -> D */
	return ~oldpix;
}
static INT32 raster_op_7(INT32 newpix, INT32 oldpix)
{
	/*  S NOR D -> D */
	return ~(newpix | oldpix);
}
static INT32 raster_op_8(INT32 newpix, INT32 oldpix)
{
	/*  S OR D -> D */
	return newpix | oldpix;
}
static INT32 raster_op_9(INT32 newpix, INT32 oldpix)
{
	/*  D -> D */
	return oldpix;
}
static INT32 raster_op_10(INT32 newpix, INT32 oldpix)
{
	/*  S XOR D -> D */
	return newpix ^ oldpix;
}
static INT32 raster_op_11(INT32 newpix, INT32 oldpix)
{
	/*  ~S AND D -> D */
	return ~newpix & oldpix;
}
static INT32 raster_op_12(INT32 newpix, INT32 oldpix)
{
	/*  1 -> D */
	return 0xffff;
}
static INT32 raster_op_13(INT32 newpix, INT32 oldpix)
{
	/*  ~S OR D -> D */
	return ~newpix | oldpix;
}
static INT32 raster_op_14(INT32 newpix, INT32 oldpix)
{
	/*  S NAND D -> D */
	return ~(newpix & oldpix);
}
static INT32 raster_op_15(INT32 newpix, INT32 oldpix)
{
	/*  ~S -> D */
	return ~newpix;
}
static INT32 raster_op_16(INT32 newpix, INT32 oldpix)
{
	/*  S + D -> D */
	return newpix + oldpix;
}
static INT32 raster_op_17(INT32 newpix, INT32 oldpix)
{
	/*  S + D -> D with Saturation*/
	INT32 max = (UINT32)0xffffffff>>(32-IOREG(REG_PSIZE));
	INT32 res = newpix + oldpix;
	return (res > max) ? max : res;
}
static INT32 raster_op_18(INT32 newpix, INT32 oldpix)
{
	/*  D - S -> D */
	return oldpix - newpix;
}
static INT32 raster_op_19(INT32 newpix, INT32 oldpix)
{
	/*  D - S -> D with Saturation */
	INT32 res = oldpix - newpix;
	return (res < 0) ? 0 : res;
}
static INT32 raster_op_20(INT32 newpix, INT32 oldpix)
{
	/*  MAX(S,D) -> D */
	return ((oldpix > newpix) ? oldpix : newpix);
}
static INT32 raster_op_21(INT32 newpix, INT32 oldpix)
{
	/*  MIN(S,D) -> D */
	return ((oldpix > newpix) ? newpix : oldpix);
}


/****************************************************************************
 * Reset the CPU emulation
 ****************************************************************************/
void tms34010_reset(void *param)
{
	int i;

	memset (&state, 0, sizeof (state));
	state.lastpixaddr = INVALID_PIX_ADDRESS;
	for (i = 0; i < MAX_CPU; i ++)
	{
		TMS34010_timer[i] = 0;
	}

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

	if (stackbase[cpu_getactivecpu()] == 0)
	{
		if (errorlog) fprintf(errorlog, "Stack Base not set on CPU #%d\n", cpu_getactivecpu());
	}

	state.stackbase = stackbase[cpu_getactivecpu()] - stackoffs[cpu_getactivecpu()];

	state.to_shiftreg   = to_shiftreg  [cpu_getactivecpu()];
	state.from_shiftreg = from_shiftreg[cpu_getactivecpu()];
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void tms34010_exit(void)
{
	/* nothing to do ? */
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned tms34010_get_context(void *dst)
{
    if( dst )
		*(TMS34010_Regs*)dst = state;
    return sizeof(TMS34010_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void tms34010_set_context(void *src)
{
    if( src )
        state = *(TMS34010_Regs*)src;
    change_pc29(PC)
    check_interrupt();
}

/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned tms34010_get_pc(void)
{
    return PC;
}


/****************************************************************************
 * Set program counter
 ****************************************************************************/
void tms34010_set_pc(unsigned val)
{
    PC = val;
    change_pc29(PC)
}


/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned tms34010_get_sp(void)
{
    return SP;
}


/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void tms34010_set_sp(unsigned val)
{
    SP = val;
}


/****************************************************************************
 * Return a specific register
 ****************************************************************************/
unsigned tms34010_get_reg(int regnum)
{
    switch( regnum )
    {
        case TMS34010_PC:  return PC;
        case TMS34010_SP:  return SP;
        case TMS34010_ST:  return ST;
        case TMS34010_A0:  return AREG( 0);
        case TMS34010_A1:  return AREG( 1);
        case TMS34010_A2:  return AREG( 2);
        case TMS34010_A3:  return AREG( 3);
        case TMS34010_A4:  return AREG( 4);
        case TMS34010_A5:  return AREG( 5);
        case TMS34010_A6:  return AREG( 6);
        case TMS34010_A7:  return AREG( 7);
        case TMS34010_A8:  return AREG( 8);
        case TMS34010_A9:  return AREG( 9);
        case TMS34010_A10: return AREG(10);
        case TMS34010_A11: return AREG(11);
        case TMS34010_A12: return AREG(12);
        case TMS34010_A13: return AREG(13);
        case TMS34010_A14: return AREG(14);
        case TMS34010_B0:  return BREG( 0<<4);
        case TMS34010_B1:  return BREG( 1<<4);
        case TMS34010_B2:  return BREG( 2<<4);
        case TMS34010_B3:  return BREG( 3<<4);
        case TMS34010_B4:  return BREG( 4<<4);
        case TMS34010_B5:  return BREG( 5<<4);
        case TMS34010_B6:  return BREG( 6<<4);
        case TMS34010_B7:  return BREG( 7<<4);
        case TMS34010_B8:  return BREG( 8<<4);
        case TMS34010_B9:  return BREG( 9<<4);
        case TMS34010_B10: return BREG(10<<4);
        case TMS34010_B11: return BREG(11<<4);
        case TMS34010_B12: return BREG(12<<4);
        case TMS34010_B13: return BREG(13<<4);
        case TMS34010_B14: return BREG(14<<4);
/* TODO: return contents of [SP + wordsize * (CPU_SP_CONTENTS-regnum)] */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = SP + 4 * (REG_SP_CONTENTS - regnum);
				return cpu_readmem29_dword( offset >> 3 );
			}
    }
    return 0;
}


/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void tms34010_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
        case TMS34010_PC:  PC = val; break;
        case TMS34010_SP:  SP = val; break;
        case TMS34010_ST:  ST = val; break;
        case TMS34010_A0:  AREG( 0) = val; break;
        case TMS34010_A1:  AREG( 1) = val; break;
        case TMS34010_A2:  AREG( 2) = val; break;
        case TMS34010_A3:  AREG( 3) = val; break;
        case TMS34010_A4:  AREG( 4) = val; break;
        case TMS34010_A5:  AREG( 5) = val; break;
        case TMS34010_A6:  AREG( 6) = val; break;
        case TMS34010_A7:  AREG( 7) = val; break;
        case TMS34010_A8:  AREG( 8) = val; break;
        case TMS34010_A9:  AREG( 9) = val; break;
        case TMS34010_A10: AREG(10) = val; break;
        case TMS34010_A11: AREG(11) = val; break;
        case TMS34010_A12: AREG(12) = val; break;
        case TMS34010_A13: AREG(13) = val; break;
        case TMS34010_A14: AREG(14) = val; break;
        case TMS34010_B0:  BREG( 0<<4) = val; break;
        case TMS34010_B1:  BREG( 1<<4) = val; break;
        case TMS34010_B2:  BREG( 2<<4) = val; break;
        case TMS34010_B3:  BREG( 3<<4) = val; break;
        case TMS34010_B4:  BREG( 4<<4) = val; break;
        case TMS34010_B5:  BREG( 5<<4) = val; break;
        case TMS34010_B6:  BREG( 6<<4) = val; break;
        case TMS34010_B7:  BREG( 7<<4) = val; break;
        case TMS34010_B8:  BREG( 8<<4) = val; break;
        case TMS34010_B9:  BREG( 9<<4) = val; break;
        case TMS34010_B10: BREG(10<<4) = val; break;
        case TMS34010_B11: BREG(11<<4) = val; break;
        case TMS34010_B12: BREG(12<<4) = val; break;
        case TMS34010_B13: BREG(13<<4) = val; break;
        case TMS34010_B14: BREG(14<<4) = val; break;
/* TODO: set contents of [SP + wordsize * (CPU_SP_CONTENTS-regnum)] */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = SP + 4 * (REG_SP_CONTENTS - regnum);
				cpu_writemem29_word( offset >> 3, val ); /* ??? */
			}
	}
}


TMS34010_Regs* TMS34010_GetState(void)
{
    return &state;
}


/****************************************************************************
 * Set NMI line state
 ****************************************************************************/
void tms34010_set_nmi_line(int linestate)
{
	/* Does not apply: the NMI is an internal interrupt for the TMS34010 */
}

/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void tms34010_set_irq_line(int irqline, int linestate)
{
	LOG((errorlog, "TMS34010#%d set irq line %d state %d\n", cpu_getactivecpu(), irqline, linestate));
	if (linestate != CLEAR_LINE)
	{
		/* set the pending interrupt */
		switch (irqline)
		{
		case 0:
			IOREG(REG_INTPEND) |= TMS34010_INT1;
			break;
		case 1:
			IOREG(REG_INTPEND) |= TMS34010_INT2;
			break;
        }

        check_interrupt();
	}
}

void tms34010_set_irq_callback(int (*callback)(int irqline))
{
	state.irq_callback = callback;
}

void tms34010_internal_interrupt(int type)
{
	LOG((errorlog, "TMS34010#%d set internal interrupt $%04x\n", cpu_getactivecpu(), type));
    IOREG(REG_INTPEND) |= type;
	check_interrupt();
}

/* Generate pending interrupts. Do NOT inline this function on DJGPP,
   it causes a slowdown */
static void check_interrupt(void)
{
	int vector=0;
	int irqline = -1;

	if (!IOREG(REG_INTPEND))
	{
		/* No interrupts pending, get out quickly */
		return;
	}

    if (IOREG(REG_INTPEND) & TMS34010_NMI)
	{
		LOG((errorlog, "TMS34010#%d takes NMI\n", cpu_getactivecpu()));
        IOREG(REG_INTPEND) &= ~TMS34010_NMI;

		if (!(IOREG(REG_HSTCTLH) & 0x0200))  /* NMI mode bit */
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
		if (!IE_FLAG)
		{
			/* Global interrupt disable */
			return;
		}

		if ((IOREG(REG_INTPEND) & TMS34010_HI) &&
			(IOREG(REG_INTENB)  & TMS34010_HI))
		{
			LOG((errorlog, "TMS34010#%d takes HI\n", cpu_getactivecpu()));
            vector = 0xfffffec0;
        }
		else
		if ((IOREG(REG_INTPEND) & TMS34010_DI) &&
			(IOREG(REG_INTENB)  & TMS34010_DI))
		{
			LOG((errorlog, "TMS34010#%d takes DI\n", cpu_getactivecpu()));
            vector = 0xfffffea0;
        }
		else
		if ((IOREG(REG_INTPEND) & TMS34010_WV) &&
			(IOREG(REG_INTENB)  & TMS34010_WV))
		{
			LOG((errorlog, "TMS34010#%d takes WV\n", cpu_getactivecpu()));
            vector = 0xfffffe80;
        }
		else
		if ((IOREG(REG_INTPEND) & TMS34010_INT1) &&
			(IOREG(REG_INTENB)	& TMS34010_INT1))
		{
			LOG((errorlog, "TMS34010#%d takes INT1\n", cpu_getactivecpu()));
            vector = 0xffffffc0;
			irqline = 0;
        }
		else
		if ((IOREG(REG_INTPEND) & TMS34010_INT2) &&
			(IOREG(REG_INTENB)  & TMS34010_INT2))
		{
			LOG((errorlog, "TMS34010#%d takes INT2\n", cpu_getactivecpu()));
            vector = 0xffffffa0;
			irqline = 1;
        }

		if (vector)
		{
            PUSH(PC);
			PUSH(GET_ST());
			RESET_ST();
			PC = RLONG(vector);
            change_pc29(PC);

			if (irqline >= 0)
				(void)(*state.irq_callback)(irqline);
        }
	}
}


/* execute instructions on this CPU until icount expires */
int tms34010_execute(int cycles)
{
	/* Get out if CPU is halted. Absolutely no interrupts must be taken!!! */
	if (IOREG(REG_HSTCTLH) & 0x8000)
	{
		return cycles;
	}

	tms34010_ICount = cycles;
	change_pc29(PC)
	do
	{
		#ifdef	MAME_DEBUG
		if (mame_debug) { state.st = GET_ST(); MAME_Debug(); }
		#endif
		state.op = ROPCODE ();
		(*opcode_table[state.op >> 4])();

		#ifdef	MAME_DEBUG
		if (mame_debug) { state.st = GET_ST(); MAME_Debug(); }
		#endif
		state.op = ROPCODE ();
		(*opcode_table[state.op >> 4])();

		#ifdef	MAME_DEBUG
		if (mame_debug) { state.st = GET_ST(); MAME_Debug(); }
		#endif
		state.op = ROPCODE ();
		(*opcode_table[state.op >> 4])();

		#ifdef	MAME_DEBUG
		if (mame_debug) { state.st = GET_ST(); MAME_Debug(); }
		#endif
		state.op = ROPCODE ();
		(*opcode_table[state.op >> 4])();

		tms34010_ICount -= 4 * TMS34010_AVGCYCLES;

	} while (tms34010_ICount > 0);

	return cycles - tms34010_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *tms34010_info(void *context, int regnum)
{
	static char buffer[40][63+1];
	static int which = 0;
	TMS34010_Regs *r = context;

	which = ++which % 40;
	buffer[which][0] = '\0';
	if( !context )
		r = &state;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "TMS34010";
		case CPU_INFO_FAMILY: return "Texas Instruments 34010";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998\nParts based on code by Aaron Giles";
		case CPU_INFO_REG_LAYOUT: return (const char *)tms34010_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)tms34010_win_layout;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->st & 0x80000000 ? 'N':'.',
				r->st & 0x40000000 ? 'C':'.',
				r->st & 0x20000000 ? 'Z':'.',
				r->st & 0x10000000 ? 'V':'.',
				r->st & 0x08000000 ? '?':'.',
				r->st & 0x04000000 ? '?':'.',
				r->st & 0x02000000 ? 'P':'.',
				r->st & 0x01000000 ? '?':'.',
				r->st & 0x00800000 ? '?':'.',
				r->st & 0x00400000 ? '?':'.',
				r->st & 0x00200000 ? 'I':'.',
				r->st & 0x00100000 ? '?':'.',
				r->st & 0x00080000 ? '?':'.',
				r->st & 0x00040000 ? '?':'.',
				r->st & 0x00020000 ? '?':'.',
				r->st & 0x00010000 ? '?':'.',
				r->st & 0x00008000 ? '?':'.',
				r->st & 0x00004000 ? '?':'.',
				r->st & 0x00002000 ? '?':'.',
				r->st & 0x00001000 ? '?':'.',
				r->st & 0x00000800 ? 'E':'.',
				r->st & 0x00000400 ? 'F':'.',
				r->st & 0x00000200 ? 'F':'.',
				r->st & 0x00000100 ? 'F':'.',
				r->st & 0x00000080 ? 'F':'.',
				r->st & 0x00000040 ? 'F':'.',
				r->st & 0x00000020 ? 'E':'.',
				r->st & 0x00000010 ? 'F':'.',
				r->st & 0x00000008 ? 'F':'.',
				r->st & 0x00000004 ? 'F':'.',
				r->st & 0x00000002 ? 'F':'.',
				r->st & 0x00000001 ? 'F':'.');
            break;
		case CPU_INFO_REG+TMS34010_PC: sprintf(buffer[which], "PC :%08X", r->pc); break;
		case CPU_INFO_REG+TMS34010_SP: sprintf(buffer[which], "SP :%08X", r->regs.a.a.Aregs[15]); break;
		case CPU_INFO_REG+TMS34010_ST: sprintf(buffer[which], "ST :%08X", r->st); break;
		case CPU_INFO_REG+TMS34010_A0: sprintf(buffer[which], "A0 :%08X", r->regs.a.a.Aregs[ 0]); break;
		case CPU_INFO_REG+TMS34010_A1: sprintf(buffer[which], "A1 :%08X", r->regs.a.a.Aregs[ 1]); break;
		case CPU_INFO_REG+TMS34010_A2: sprintf(buffer[which], "A2 :%08X", r->regs.a.a.Aregs[ 2]); break;
		case CPU_INFO_REG+TMS34010_A3: sprintf(buffer[which], "A3 :%08X", r->regs.a.a.Aregs[ 3]); break;
		case CPU_INFO_REG+TMS34010_A4: sprintf(buffer[which], "A4 :%08X", r->regs.a.a.Aregs[ 4]); break;
		case CPU_INFO_REG+TMS34010_A5: sprintf(buffer[which], "A5 :%08X", r->regs.a.a.Aregs[ 5]); break;
		case CPU_INFO_REG+TMS34010_A6: sprintf(buffer[which], "A6 :%08X", r->regs.a.a.Aregs[ 6]); break;
		case CPU_INFO_REG+TMS34010_A7: sprintf(buffer[which], "A7 :%08X", r->regs.a.a.Aregs[ 7]); break;
		case CPU_INFO_REG+TMS34010_A8: sprintf(buffer[which], "A8 :%08X", r->regs.a.a.Aregs[ 8]); break;
		case CPU_INFO_REG+TMS34010_A9: sprintf(buffer[which], "A9 :%08X", r->regs.a.a.Aregs[ 9]); break;
		case CPU_INFO_REG+TMS34010_A10: sprintf(buffer[which],"A10:%08X", r->regs.a.a.Aregs[10]); break;
		case CPU_INFO_REG+TMS34010_A11: sprintf(buffer[which],"A11:%08X", r->regs.a.a.Aregs[11]); break;
		case CPU_INFO_REG+TMS34010_A12: sprintf(buffer[which],"A12:%08X", r->regs.a.a.Aregs[12]); break;
		case CPU_INFO_REG+TMS34010_A13: sprintf(buffer[which],"A13:%08X", r->regs.a.a.Aregs[13]); break;
		case CPU_INFO_REG+TMS34010_A14: sprintf(buffer[which],"A14:%08X", r->regs.a.a.Aregs[14]); break;
		case CPU_INFO_REG+TMS34010_B0: sprintf(buffer[which], "B0 :%08X", r->regs.Bregs[ 0<<4]); break;
		case CPU_INFO_REG+TMS34010_B1: sprintf(buffer[which], "B1 :%08X", r->regs.Bregs[ 1<<4]); break;
		case CPU_INFO_REG+TMS34010_B2: sprintf(buffer[which], "B2 :%08X", r->regs.Bregs[ 2<<4]); break;
		case CPU_INFO_REG+TMS34010_B3: sprintf(buffer[which], "B3 :%08X", r->regs.Bregs[ 3<<4]); break;
		case CPU_INFO_REG+TMS34010_B4: sprintf(buffer[which], "B4 :%08X", r->regs.Bregs[ 4<<4]); break;
		case CPU_INFO_REG+TMS34010_B5: sprintf(buffer[which], "B5 :%08X", r->regs.Bregs[ 5<<4]); break;
		case CPU_INFO_REG+TMS34010_B6: sprintf(buffer[which], "B6 :%08X", r->regs.Bregs[ 6<<4]); break;
		case CPU_INFO_REG+TMS34010_B7: sprintf(buffer[which], "B7 :%08X", r->regs.Bregs[ 7<<4]); break;
		case CPU_INFO_REG+TMS34010_B8: sprintf(buffer[which], "B8 :%08X", r->regs.Bregs[ 8<<4]); break;
		case CPU_INFO_REG+TMS34010_B9: sprintf(buffer[which], "B9 :%08X", r->regs.Bregs[ 9<<4]); break;
		case CPU_INFO_REG+TMS34010_B10: sprintf(buffer[which],"B10:%08X", r->regs.Bregs[10<<4]); break;
		case CPU_INFO_REG+TMS34010_B11: sprintf(buffer[which],"B11:%08X", r->regs.Bregs[11<<4]); break;
		case CPU_INFO_REG+TMS34010_B12: sprintf(buffer[which],"B12:%08X", r->regs.Bregs[12<<4]); break;
		case CPU_INFO_REG+TMS34010_B13: sprintf(buffer[which],"B13:%08X", r->regs.Bregs[13<<4]); break;
		case CPU_INFO_REG+TMS34010_B14: sprintf(buffer[which],"B14:%08X", r->regs.Bregs[14<<4]); break;
	}
	return buffer[which];
}

unsigned tms34010_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasm34010(buffer,pc);
#else
	sprintf( buffer, "$%04X", cpu_readop16(pc>>3) );
	return 2;
#endif
}


static UINT32 (*pixel_write_ops[4][5])(UINT32, UINT32)	=
{
	{write_pixel_1,     write_pixel_2,     write_pixel_4,     write_pixel_8,     write_pixel_16},
	{write_pixel_r_1,   write_pixel_r_2,   write_pixel_r_4,   write_pixel_r_8,   write_pixel_r_16},
	{write_pixel_t_1,   write_pixel_t_2,   write_pixel_t_4,   write_pixel_t_8,   write_pixel_t_16},
	{write_pixel_r_t_1, write_pixel_r_t_2, write_pixel_r_t_4, write_pixel_r_t_8, write_pixel_r_t_16}
};

static UINT32 (*pixel_read_ops[5])(UINT32 address) =
{
	read_pixel_1, read_pixel_2, read_pixel_4, read_pixel_8, read_pixel_16
};


static void set_pixel_function(void)
{
	UINT32 i1,i2;

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


static INT32 (*raster_ops[32]) (INT32 newpix, INT32 oldpix) =
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
		if (data && errorlog) fprintf(errorlog, "Plane masking not supported. PC=%08X\n", cpu_get_pc());
		break;

	case REG_DPYCTL:
		set_pixel_function();
		break;

	case REG_HSTCTLH:
		if (data & 0x8000)
		{
			/* CPU is halting itself, stop execution right away */
			tms34010_ICount = 0;
		}
		cpu_halt(cpu_getactivecpu(), !(data & 0x8000));

		if (data & 0x0100)
		{
			/* NMI issued */
            cpu_generate_internal_interrupt(cpu_getactivecpu(), TMS34010_NMI);
        }
		break;

	case REG_CONVDP:
		state.xytolshiftcount1 = (~data & 0x0f);
		break;

	case REG_INTENB:
		if (IOREG(REG_INTENB) & IOREG(REG_INTPEND)) check_interrupt();
		break;
	}

	/*if (errorlog)
	{
		fprintf(errorlog, "TMS34010 io write. Reg #%02X=%04X - PC: %04X\n",
				reg, IOREG(reg), cpu_get_pc());
	}*/
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

#define FINDCONTEXT(cpu, context)       \
	if (cpu_is_saving_context(cpu))     \
	{                                   \
		context = cpu_getcontext(cpu);  \
	}                                   \
	else                                \
	{                                   \
		context = &state;               \
	}                                   \


void TMS34010_set_stack_base(int cpu, UINT8* stackbase_p, UINT32 stackoffs_p)
{
	stackbase[cpu] = stackbase_p;
	stackoffs[cpu] = stackoffs_p;
}

void TMS34010_set_shiftreg_functions(int cpu,
									 void (*to_shiftreg_p  )(UINT32, UINT16*),
									 void (*from_shiftreg_p)(UINT32, UINT16*))
{
	to_shiftreg  [cpu] = to_shiftreg_p;
	from_shiftreg[cpu] = from_shiftreg_p;
}

int TMS34010_io_display_blanked(int cpu)
{
	TMS34010_Regs* context;
	FINDCONTEXT(cpu, context);
	return (!(context->IOregs[REG_DPYCTL] & 0x8000));
}

int TMS34010_get_DPYSTRT(int cpu)
{
	TMS34010_Regs* context;
	FINDCONTEXT(cpu, context);
	return context->IOregs[REG_DPYSTRT];
}

static void TMS34010_io_intcallback(int param)
{
    /* Reset timer for next frame */
	double interval = TIME_IN_HZ (Machine->drv->frames_per_second);
	TMS34010_timer[param] = timer_set(interval, param, TMS34010_io_intcallback);
    cpu_generate_internal_interrupt(param, TMS34010_DI);
}


void TMS34010_State_Save(int cpunum, void *f)
{
	TMS34010_Regs* context;
	FINDCONTEXT(cpunum, context);
	osd_fwrite(f,context,sizeof(state));
	osd_fwrite(f,&tms34010_ICount,sizeof(tms34010_ICount));
	osd_fwrite(f,state.shiftreg,sizeof(SHIFTREG_SIZE));
}

void TMS34010_State_Load(int cpunum, void *f)
{
	/* Don't reload the following */
	unsigned short* shiftreg_save = state.shiftreg;
	void (*to_shiftreg_save)  (UINT32, UINT16*) = state.to_shiftreg;
	void (*from_shiftreg_save)(UINT32, UINT16*) = state.from_shiftreg;

	TMS34010_Regs* context;
	FINDCONTEXT(cpunum, context);

	osd_fread(f,context,sizeof(state));
	osd_fread(f,&tms34010_ICount,sizeof(tms34010_ICount));
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

    TMS34010_WRMEM_WORD(TOBYTE(addr), data);

	/* Postincrement? */
	if (SLAVE_IOREG(REG_HSTCTLH) & 0x0800)
	{
		addr += 0x10;
		SLAVE_IOREG(REG_HSTADRH) = addr >> 16;
		SLAVE_IOREG(REG_HSTADRL) = (UINT16)addr;
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
		SLAVE_IOREG(REG_HSTADRL) = (UINT16)addr;
	}

    data = TMS34010_RDMEM_WORD(TOBYTE(addr));

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
        cpu_generate_internal_interrupt(CPU_SLAVE, TMS34010_NMI);
    }
}
