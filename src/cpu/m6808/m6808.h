/*** m6808: Portable 6808 emulator ******************************************/

#ifndef _M6808_H
#define _M6808_H

#include "osd_cpu.h"
#include "memory.h"
#include "cpuintrf.h"

/* 6808 Registers */
typedef struct
{
	PAIR	pc; 		/* Program counter */
	PAIR	s;			/* Stack pointer */
	PAIR	x;			/* Index register */
	PAIR	d;			/* Accumulators */
	UINT8	cc; 		/* Condition codes */
	UINT8	wai_state;	/* WAI opcode state */
	INT8	nmi_state;
	INT8	irq_state[2];
    int     (*irq_callback)(int irqline);
}	m6808_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

#define M6808_INT_NONE  0           /* No interrupt required */
#define M6808_INT_IRQ	1 			/* Standard IRQ interrupt */
#define M6808_INT_NMI	2			/* NMI interrupt          */
#define M6808_INT_OCI	4			/* Output Compare interrupt (timer) */
#define M6808_WAI		8			/* set when WAI is waiting for an interrupt */

#define M6808_IRQ_LINE	0			/* IRQ line number */
#define M6808_OCI_LINE	1			/* OCI line number */

/* PUBLIC GLOBALS */
extern int m6808_ICount;

/* PUBLIC FUNCTIONS */
extern void m6808_reset(void *param);
extern void m6808_exit(void);
extern int  m6808_execute(int cycles);             /* MB */
extern void m6808_setregs(m6808_Regs *Regs);
extern void m6808_getregs(m6808_Regs *Regs);
extern unsigned m6808_getpc(void);
extern unsigned m6808_getreg(int regnum);
extern void m6808_setreg(int regnum, unsigned val);
extern void m6808_set_nmi_line(int state);
extern void m6808_set_irq_line(int irqline, int state);
extern void m6808_set_irq_callback(int (*callback)(int irqline));
extern const char *m6808_info(void *context, int regnum);

/****************************************************************************
 * For now make the 6800 using the m6808 variables and functions
 ****************************************************************************/
#define m6800_Regs					m6808_Regs
#define M6800_INT_NONE				M6808_INT_NONE
#define M6800_INT_IRQ				M6808_INT_IRQ
#define M6800_INT_NMI				M6808_INT_NMI
#define M6800_INT_OCI				M6808_INT_OCI
#define M6800_WAI					M6808_WAI
#define M6800_IRQ_LINE				M6808_IRQ_LINE
#define M6800_OCI_LINE				M6808_OCI_LINE

#define m6800_ICount				m6808_ICount
#define m6800_reset 				m6808_reset
#define m6800_exit					m6808_exit
extern int m6800_execute(int cycles);
#define m6800_setregs				m6808_setregs
#define m6800_getregs				m6808_getregs
#define m6800_getpc 				m6808_getpc
#define m6800_getreg				m6808_getreg
#define m6800_setreg				m6808_setreg
#define m6800_set_nmi_line			m6808_set_nmi_line
#define m6800_set_irq_line			m6808_set_irq_line
#define m6800_set_irq_callback		m6808_set_irq_callback
extern const char *m6800_info(void *context, int regnum);

/****************************************************************************
 * For now make the 6802 using the m6808 variables and functions
 ****************************************************************************/
#define m6802_Regs					m6808_Regs
#define M6802_INT_NONE              M6808_INT_NONE
#define M6802_INT_IRQ				M6808_INT_IRQ
#define M6802_INT_NMI				M6808_INT_NMI
#define M6802_INT_OCI				M6808_INT_OCI
#define M6802_WAI					M6808_WAI
#define M6802_IRQ_LINE				M6808_IRQ_LINE
#define M6802_OCI_LINE				M6808_OCI_LINE

#define m6802_ICount				m6808_ICount
#define m6802_reset                 m6808_reset
#define m6802_exit                  m6808_exit
extern int m6802_execute(int cycles);
#define m6802_setregs				m6808_setregs
#define m6802_getregs				m6808_getregs
#define m6802_getpc 				m6808_getpc
#define m6802_getreg				m6808_getreg
#define m6802_setreg				m6808_setreg
#define m6802_set_nmi_line			m6808_set_nmi_line
#define m6802_set_irq_line			m6808_set_irq_line
#define m6802_set_irq_callback		m6808_set_irq_callback
extern const char *m6802_info(void *context, int regnum);

/****************************************************************************
 * For now make the 6803 using the m6808 variables and functions
 ****************************************************************************/
#define m6803_Regs					m6808_Regs
#define M6803_INT_NONE              M6808_INT_NONE
#define M6803_INT_IRQ				M6808_INT_IRQ
#define M6803_INT_NMI				M6808_INT_NMI
#define M6803_INT_OCI				M6808_INT_OCI
#define M6803_WAI					M6808_WAI
#define M6803_IRQ_LINE				M6808_IRQ_LINE
#define M6803_OCI_LINE				M6808_OCI_LINE

#define m6803_ICount				m6808_ICount
#define m6803_reset 				m6808_reset
#define m6803_exit					m6808_exit
extern int m6803_execute(int cycles);
#define m6803_setregs               m6808_setregs
#define m6803_getregs               m6808_getregs
#define m6803_getpc                 m6808_getpc
#define m6803_getreg				m6808_getreg
#define m6803_setreg				m6808_setreg
#define m6803_set_nmi_line          m6808_set_nmi_line
#define m6803_set_irq_line			m6808_set_irq_line
#define m6803_set_irq_callback		m6808_set_irq_callback
extern const char *m6803_info(void *context, int regnum);

/****************************************************************************
 * For now make the HD63701 using the m6808 variables and functions
 ****************************************************************************/
#define hd63701_Regs				m6808_Regs
#define HD63701_INT_NONE            M6808_INT_NONE
#define HD63701_INT_IRQ 			M6808_INT_IRQ
#define HD63701_INT_NMI 			M6808_INT_NMI
#define HD63701_INT_OCI 			M6808_INT_OCI
#define HD63701_WAI 				M6808_WAI
#define HD63701_IRQ_LINE			M6808_IRQ_LINE
#define HD63701_OCI_LINE			M6808_OCI_LINE

#define hd63701_ICount              m6808_ICount
#define hd63701_reset               m6808_reset
#define hd63701_exit                m6808_exit
extern int hd63701_execute(int cycles);
#define hd63701_setregs 			m6808_setregs
#define hd63701_getregs 			m6808_getregs
#define hd63701_getpc				m6808_getpc
#define hd63701_getreg				m6808_getreg
#define hd63701_setreg				m6808_setreg
#define hd63701_set_nmi_line        m6808_set_nmi_line
#define hd63701_set_irq_line		m6808_set_irq_line
#define hd63701_set_irq_callback	m6808_set_irq_callback
extern const char *hd63701_info(void *context, int regnum);

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6808_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6808_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6808_RDOP() is identical to M6808_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6808_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6808_RDOP_ARG() is identical to M6808_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6808_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6808_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6808_FAST_S is set, bytes are pulled 		*/
/* directly from the RAM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6808_Flags;
#define M6808_FAST_NONE	0x00	/* no memory optimizations */
#define M6808_FAST_S	0x02	/* stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef	MAME_DEBUG
extern	int mame_debug;
extern	int Dasm6800(char *buf, int pc);
extern  int Dasm6802(char *buf, int pc);
extern	int Dasm6803(char *buf, int pc);
extern  int Dasm6808(char *buf, int pc);
extern	int Dasm63701(char *buf, int pc);
#endif

#endif /* _M6808_H */

