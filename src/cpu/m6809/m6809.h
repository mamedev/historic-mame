/*** m6809: Portable 6809 emulator ******************************************/

#ifndef _M6809_H
#define _M6809_H

#include "memory.h"
#include "osd_cpu.h"

/* 6809 Registers */
typedef struct
{
	PAIR	pc; 	/* Program counter */
	PAIR	u, s;	/* Stack pointers */
	PAIR	x, y;	/* Index registers */
	PAIR	d;		/* Accumulatora a and b */
	UINT8	dp; 	/* Direct Page register */
	UINT8	cc;
	INT8	int_state;	/* SYNC and CWAI flags */
    INT8    nmi_state;
    INT8    irq_state[2];
	int 	(*irq_callback)(int irqline);
	int 	extra_cycles; /* cycles used up by interrupts */
} m6809_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

/* flag bits in the cc register */
#define CC_C	0x01		/* Carry */
#define CC_V	0x02		/* Overflow */
#define CC_Z	0x04		/* Zero */
#define CC_N	0x08		/* Negative */
#define CC_II	0x10		/* Inhibit IRQ */
#define CC_H	0x20		/* Half (auxiliary) carry */
#define CC_IF	0x40		/* Inhibit FIRQ */
#define CC_E	0x80		/* entire state pushed */


#define M6809_INT_NONE	0	/* No interrupt required */
#define M6809_INT_IRQ	1	/* Standard IRQ interrupt */
#define M6809_INT_FIRQ	2	/* Fast IRQ */
#define M6809_INT_NMI	4	/* NMI */	/* NS 970909 */
#define M6809_IRQ_LINE	0	/* IRQ line number */
#define M6809_FIRQ_LINE 1   /* FIRQ line number */

/* PUBLIC GLOBALS */
extern int  m6809_ICount;


/* PUBLIC FUNCTIONS */
extern void m6809_reset(void *param);
extern void m6809_exit(void);
extern int m6809_execute(int cycles);  /* NS 970908 */
extern void m6809_setregs(m6809_Regs *Regs);
extern void m6809_getregs(m6809_Regs *Regs);
extern unsigned m6809_getpc(void);
extern unsigned m6809_getreg(int regnum);
extern void m6809_setreg(int regnum, unsigned val);
extern void m6809_set_nmi_line(int state);
extern void m6809_set_irq_line(int irqline, int state);
extern void m6809_set_irq_callback(int (*callback)(int irqline));
extern const char *m6809_info(void *context,int regnum);

/****************************************************************************/
/* For now the 6309 is using the functions of the 6809						*/
/****************************************************************************/
#define M6309_INT_NONE                  M6809_INT_NONE
#define M6309_INT_IRQ					M6809_INT_IRQ
#define M6309_INT_FIRQ					M6809_INT_FIRQ
#define M6309_INT_NMI					M6809_INT_NMI
#define M6309_IRQ_LINE					M6809_IRQ_LINE
#define M6309_FIRQ_LINE 				M6809_FIRQ_LINE
#define m6309_ICount					m6809_ICount
#define m6309_reset 					m6809_reset
#define m6309_exit						m6809_exit
#define m6309_execute					m6809_execute
#define m6309_setregs					m6809_setregs
#define m6309_getregs					m6809_getregs
#define m6309_getpc 					m6809_getpc
#define m6309_getreg					m6809_getreg
#define m6309_setreg					m6809_setreg
#define m6309_set_nmi_line				m6809_set_nmi_line
#define m6309_set_irq_line				m6809_set_irq_line
#define m6309_set_irq_callback			m6809_set_irq_callback
extern const char *m6309_info(void *context,int regnum);

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6809_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6809_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6809_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6809_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6809_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6809_FAST_OP is set, opcodes are fetched     */
/* directly from the ROM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6809_Flags;
#define M6809_FAST_NONE	0x00	/* no memory optimizations */
#define M6809_FAST_S	0x02	/* stack */
#define M6809_FAST_U	0x04	/* user stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6809 (char *buffer, int pc);
#endif

#endif /* _M6809_H */
