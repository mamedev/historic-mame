/*** TMS34010: Portable TMS34010 emulator ***********************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1988
	 originally based on code by Aaron Giles

	System dependencies:	long must be at least 32 bits
	                        word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/

#ifndef _TMS34010_H
#define _TMS34010_H

#include "memory.h"

/****************************************************************************/
/* sizeof(char)=1, sizeof(short)=2, sizeof(long)>=4                         */
/****************************************************************************/

/* TMS34010 Registers */
typedef struct
{
	unsigned int op;
	unsigned int pc;
	unsigned int st;        /* Only here so we can display it in the debug window */
	unsigned int nflag;
	unsigned int cflag;
	unsigned int notzflag;  /* So we can just do an assignment to set it */
	unsigned int vflag;
	unsigned int pflag;
	unsigned int ieflag;
	unsigned int fe0flag;
	unsigned int fe1flag;
	unsigned int fw[2];
	unsigned int fw_inc[2];  /* Same as fw[], except when fw = 0, fw_inc = 32 */
	int Aregs[16];
	int Bregs[16];
	unsigned int IOregs[32];
	void (*F0_write) (unsigned int bitaddr, unsigned int data);
	void (*F1_write) (unsigned int bitaddr, unsigned int data);
	int (*F0_read) (unsigned int bitaddr);
	int (*F1_read) (unsigned int bitaddr);
    int (*pixel_write)(unsigned int address, unsigned int value);
    int (*pixel_read)(unsigned int address);
	int transparency;
	int window_checking;
	int (*raster_op)(int newpix, int oldpix);
	unsigned int lastpixaddr;
	unsigned int lastpixword;
	int lastpixwordchanged;
	int xytolshiftcount1;
	int xytolshiftcount2;
	unsigned int shiftreg;
} TMS34010_Regs;

#define INVALID_PIX_ADDRESS  0xffffffe0  /* This is the reset vector, this should be ok */

#ifndef INLINE
#define INLINE static inline
#endif

/* Interrupt bits */
#define TMS34010_INT_NONE    0x0000
#define TMS34010_INT1        0x0002    /* External Interrupt 1 */
#define TMS34010_INT2        0x0004    /* External Interrupt 2 */
#define TMS34010_NMI         0x0100    /* NMI Interrupt */
#define TMS34010_HI          0x0200    /* Host Interrupt */
#define TMS34010_DI          0x0400    /* Display Interrupt */
#define TMS34010_WV          0x0800    /* Window Violation Interrupt */

/* PUBLIC FUNCTIONS */
TMS34010_Regs* TMS34010_GetState(void);
void TMS34010_SetRegs(TMS34010_Regs *Regs);
void TMS34010_GetRegs(TMS34010_Regs *Regs);
unsigned TMS34010_GetPC(void);
void TMS34010_Reset(void);
int TMS34010_Execute(int cycles);
void TMS34010_Cause_Interrupt(int type);
void TMS34010_Clear_Pending_Interrupts(void);

void TMS34010_SetBank(int banknum, unsigned char *address);

void TMS34010_State_Save(int cpunum, void *f);
void TMS34010_State_Load(int cpunum, void *f);

void TMS34010_HSTADRL_w (int offset, int data);
void TMS34010_HSTADRH_w (int offset, int data);
void TMS34010_HSTDATA_w (int offset, int data);
int  TMS34010_HSTDATA_r (int offset);
void TMS34010_HSTCTLH_w (int offset, int data);

/* PUBLIC GLOBALS */
extern int	TMS34010_ICount;


#define TOBYTE(bitaddr) ((unsigned int) (bitaddr)>>3)

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
#define TMS34010_RDMEM(A) ((unsigned)cpu_readmem29(A))
#define TMS34010_RDMEM_WORD(A) ((unsigned)cpu_readmem29_word(A))
#define TMS34010_RDMEM_DWORD(A) ((unsigned)cpu_readmem29_dword(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define TMS34010_WRMEM(A,V) (cpu_writemem29(A,(V)&0xff))
#define TMS34010_WRMEM_WORD(A,V) (cpu_writemem29_word(A,(V)&0xffff))
#define TMS34010_WRMEM_DWORD(A,V) (cpu_writemem29_dword(A,V))

/****************************************************************************/
/* I/O constants and function prototypes 									*/
/****************************************************************************/
#define REG_HESYNC		0
#define REG_HEBLNK		1
#define REG_HSBLNK		2
#define REG_HTOTAL		3
#define REG_VESYNC		4
#define REG_VEBLNK		5
#define REG_VSBLNK		6
#define REG_VTOTAL		7
#define REG_DPYCTL		8
#define REG_DPYSTRT		9
#define REG_DPYINT		10
#define REG_CONTROL		11
#define REG_HSTDATA		12
#define REG_HSTADRL		13
#define REG_HSTADRH		14
#define REG_HSTCTLL		15
#define REG_HSTCTLH		16
#define REG_INTENB		17
#define REG_INTPEND		18
#define REG_CONVSP		19
#define REG_CONVDP		20
#define REG_PSIZE		21
#define REG_PMASK		22
#define REG_DPYTAP		27
#define REG_HCOUNT		28
#define REG_VCOUNT		29
#define REG_DPYADR		30
#define REG_REFCNT		31

/* IO registers accessor */
#define IOREG(reg)	(state.IOregs[reg])
#define PBH (IOREG(REG_CONTROL) & 0x0100)
#define PBV (IOREG(REG_CONTROL) & 0x0200)

/* Callback prototypes */

struct TMS34010_io_interface
{
    int cpu;                         /* Which CPU are we causing interrupts on */
};


/* Writes to the 34010 io */
void TMS34010_io_register_w(int offset, int data);

/* Reads from the 34010 io */
int TMS34010_io_register_r(int offset);

/* Checks whether the display is inhibited */
int TMS34010_io_display_blanked(int cpu);

int TMS34010_get_DPYSTRT(int cpu);

void WFIELD_01(unsigned int bitaddr, unsigned int data);
void WFIELD_02(unsigned int bitaddr, unsigned int data);
void WFIELD_03(unsigned int bitaddr, unsigned int data);
void WFIELD_04(unsigned int bitaddr, unsigned int data);
void WFIELD_05(unsigned int bitaddr, unsigned int data);
void WFIELD_06(unsigned int bitaddr, unsigned int data);
void WFIELD_07(unsigned int bitaddr, unsigned int data);
void WFIELD_08(unsigned int bitaddr, unsigned int data);
void WFIELD_09(unsigned int bitaddr, unsigned int data);
void WFIELD_10(unsigned int bitaddr, unsigned int data);
void WFIELD_11(unsigned int bitaddr, unsigned int data);
void WFIELD_12(unsigned int bitaddr, unsigned int data);
void WFIELD_13(unsigned int bitaddr, unsigned int data);
void WFIELD_14(unsigned int bitaddr, unsigned int data);
void WFIELD_15(unsigned int bitaddr, unsigned int data);
void WFIELD_16(unsigned int bitaddr, unsigned int data);
void WFIELD_17(unsigned int bitaddr, unsigned int data);
void WFIELD_18(unsigned int bitaddr, unsigned int data);
void WFIELD_19(unsigned int bitaddr, unsigned int data);
void WFIELD_20(unsigned int bitaddr, unsigned int data);
void WFIELD_21(unsigned int bitaddr, unsigned int data);
void WFIELD_22(unsigned int bitaddr, unsigned int data);
void WFIELD_23(unsigned int bitaddr, unsigned int data);
void WFIELD_24(unsigned int bitaddr, unsigned int data);
void WFIELD_25(unsigned int bitaddr, unsigned int data);
void WFIELD_26(unsigned int bitaddr, unsigned int data);
void WFIELD_27(unsigned int bitaddr, unsigned int data);
void WFIELD_28(unsigned int bitaddr, unsigned int data);
void WFIELD_29(unsigned int bitaddr, unsigned int data);
void WFIELD_30(unsigned int bitaddr, unsigned int data);
void WFIELD_31(unsigned int bitaddr, unsigned int data);
void WFIELD_32(unsigned int bitaddr, unsigned int data);

int RFIELD_01(unsigned int bitaddr);
int RFIELD_02(unsigned int bitaddr);
int RFIELD_03(unsigned int bitaddr);
int RFIELD_04(unsigned int bitaddr);
int RFIELD_05(unsigned int bitaddr);
int RFIELD_06(unsigned int bitaddr);
int RFIELD_07(unsigned int bitaddr);
int RFIELD_08(unsigned int bitaddr);
int RFIELD_09(unsigned int bitaddr);
int RFIELD_10(unsigned int bitaddr);
int RFIELD_11(unsigned int bitaddr);
int RFIELD_12(unsigned int bitaddr);
int RFIELD_13(unsigned int bitaddr);
int RFIELD_14(unsigned int bitaddr);
int RFIELD_15(unsigned int bitaddr);
int RFIELD_16(unsigned int bitaddr);
int RFIELD_17(unsigned int bitaddr);
int RFIELD_18(unsigned int bitaddr);
int RFIELD_19(unsigned int bitaddr);
int RFIELD_20(unsigned int bitaddr);
int RFIELD_21(unsigned int bitaddr);
int RFIELD_22(unsigned int bitaddr);
int RFIELD_23(unsigned int bitaddr);
int RFIELD_24(unsigned int bitaddr);
int RFIELD_25(unsigned int bitaddr);
int RFIELD_26(unsigned int bitaddr);
int RFIELD_27(unsigned int bitaddr);
int RFIELD_28(unsigned int bitaddr);
int RFIELD_29(unsigned int bitaddr);
int RFIELD_30(unsigned int bitaddr);
int RFIELD_31(unsigned int bitaddr);
int RFIELD_32(unsigned int bitaddr);

#endif /* _TMS34010_H */
