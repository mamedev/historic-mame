#ifndef M68000__HEADER
#define M68000__HEADER

/* Choose between the x86 assembler and C version of the CPU context */
#ifdef A68KEM

/* Use the x86 assembly core */
typedef struct
{
    int   d[8];             /* 0x0004 8 Data registers */
	int   a[8];             /* 0x0024 8 Address registers */

    int   usp;              /* 0x0044 Stack registers (They will overlap over reg_a7) */
    int   isp;              /* 0x0048 */

    int   sr_high;     		/* 0x004C System registers */
    int   ccr;              /* 0x0050 CCR in Intel Format */
    int   x_carry;			/* 0x0054 Extended Carry */

    int   pc;            	/* 0x0058 Program Counter */

    int   IRQ_level;        /* 0x005C IRQ level you want the MC68K process (0=None)  */

    /* Backward compatible with C emulator - Only set in Debug compile */

    int   sr;

#if NEW_INTERRUPT_SYSTEM
	int (*irq_callback)(int irqline);
#endif /* NEW_INTERRUPT_SYSTEM */

    int vbr;                /* Vector Base Register.  Will be used in 68010 */
    int sfc;                /* Source Function Code.  Will be used in 68010 */
    int dfc;                /* Destination Function Code.  Will be used in 68010 */

} m68k_cpu_context;


#else

/* Use the C core */
#include "m68k.h"

#endif /* A68KEM */




/* The MAME API for MC68000 */

#define MC68000_INT_NONE 0
#define MC68000_IRQ_1    1
#define MC68000_IRQ_2    2
#define MC68000_IRQ_3    3
#define MC68000_IRQ_4    4
#define MC68000_IRQ_5    5
#define MC68000_IRQ_6    6
#define MC68000_IRQ_7    7

#define MC68000_INT_ACK_AUTOVECTOR    -1
#define MC68000_INT_ACK_SPURIOUS      -2

#define MC68000_CPU_MODE_68000 1
#define MC68000_CPU_MODE_68010 2
#define MC68000_CPU_MODE_68020 4



#define Dasm68000        m68k_disassemble

typedef struct {

	m68k_cpu_context regs;

}	MC68000_Regs;

/*
extern m68k_cpu_context regs;
*/

void MC68000_SetRegs(MC68000_Regs *src);
void MC68000_GetRegs(MC68000_Regs *dst);
void MC68000_Reset(void);
int  MC68000_Execute(int cycles);
int  MC68000_GetPC(void);

extern int MC68000_ICount;

#if NEW_INTERRUPT_SYSTEM

void MC68000_set_nmi_line(int state);
void MC68000_set_irq_line(int irqline, int state);
void MC68000_set_irq_callback(int (*callback)(int irqline));

#else

void MC68000_Cause_Interrupt(int level)
void MC68000_ClearPendingInterrupts(void);

#endif /* NEW_INTERRUPT_SYSTEM */




/* Massage the C core's names a bit for MAME.
 * The rest of the functions are in m68kmame.c
 */
#ifndef A68KEM

#define MC68000_Reset    m68k_pulse_reset
#define MC68000_Execute  m68k_execute
#define MC68000_GetPC    m68k_peek_pc

#define MC68000_ICount   m68k_clks_left

#define MC68000_set_irq_callback m68k_set_int_ack_callback

#endif /* A68KEM */


#endif /* M68000__HEADER */
