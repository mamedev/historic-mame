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

/* Massage the C core's names a bit for MAME.
 * The rest of the functions are in m68kmame.c
 */
#ifndef A68KEM

#define MC68000_ICount   m68k_clks_left

#define MC68000_reset	 m68k_pulse_reset
#define MC68000_execute  m68k_execute
#define MC68000_getpc	 m68k_peek_pc

#define MC68000_set_irq_callback m68k_set_int_ack_callback

#else

extern int MC68000_ICount;

extern void MC68000_reset(void *param);
extern int  MC68000_execute(int cycles);
extern void MC68000_set_irq_callback(int (*callback)(int irqline));

extern unsigned MC68000_getpc(void);

#endif /* A68KEM */

extern void MC68000_exit(void);
extern void MC68000_setregs(MC68000_Regs *src);
extern void MC68000_getregs(MC68000_Regs *dst);
extern unsigned MC68000_getreg(int regnum);
extern void MC68000_setreg(int regnum, unsigned val);
extern void MC68000_set_nmi_line(int state);
extern void MC68000_set_irq_line(int irqline, int state);
extern const char *MC68000_info(void *context, int regnum);

/****************************************************************************
 * For now define the MC68010 to use MC68000 variables and functions
 ****************************************************************************/
#define MC68010_INT_NONE                MC68000_INT_NONE
#define MC68010_ICount                  MC68000_ICount
#define MC68010_setregs                 MC68000_setregs
#define MC68010_getregs                 MC68000_getregs
#define MC68010_reset                   MC68000_reset
#define MC68010_exit                    MC68000_exit
#define MC68010_execute                 MC68000_execute
#define MC68010_getpc                   MC68000_getpc
#define MC68010_getreg					MC68000_getreg
#define MC68010_setreg					MC68000_setreg
#define MC68010_set_nmi_line            MC68000_set_nmi_line
#define MC68010_set_irq_line            MC68000_set_irq_line
#define MC68010_set_irq_callback        MC68000_set_irq_callback
const char *MC68010_info(void *context, int regnum);

/****************************************************************************
 * For now define the MC68020 to use MC68000 variables and functions
 ****************************************************************************/
#define MC68020_INT_NONE				MC68000_INT_NONE
#define MC68020_ICount					MC68000_ICount
#define MC68020_setregs 				MC68000_setregs
#define MC68020_getregs 				MC68000_getregs
#define MC68020_reset					MC68000_reset
#define MC68020_exit					MC68000_exit
#define MC68020_execute 				MC68000_execute
#define MC68020_getpc					MC68000_getpc
#define MC68020_getreg					MC68000_getreg
#define MC68020_setreg					MC68000_setreg
#define MC68020_set_nmi_line            MC68000_set_nmi_line
#define MC68020_set_irq_line			MC68000_set_irq_line
#define MC68020_set_irq_callback		MC68000_set_irq_callback
const char *MC68020_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int mame_debug;
extern void MAME_Debug(void);
extern int Dasm68000(char *buffer, int pc);
#endif /* MAME_DEBUG */

#endif /* M68000__HEADER */
