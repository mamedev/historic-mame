/*** T-11: Portable DEC T-11 emulator ******************************************/

#ifndef _T11_H
#define _T11_H

#include "memory.h"
#include "osd_cpu.h"

/* T-11 Registers */
typedef struct
{
	PAIR	reg[8];
	PAIR	psw;
	int 	op;
	int 	pending_interrupts;
	UINT8	*bank[8];
#if NEW_INTERRUPT_SYSTEM
	INT8	irq_state[4];
	int 	(*irq_callback)(int irqline);
#endif
} t11_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

#define T11_INT_NONE    -1      /* No interrupt requested */
#define T11_IRQ0        0      /* IRQ0 */
#define T11_IRQ1		1	   /* IRQ1 */
#define T11_IRQ2		2	   /* IRQ2 */
#define T11_IRQ3		3	   /* IRQ3 */

#define T11_RESERVED    0x000   /* Reserved vector */
#define T11_TIMEOUT     0x004   /* Time-out/system error vector */
#define T11_ILLINST     0x008   /* Illegal and reserved instruction vector */
#define T11_BPT         0x00C   /* BPT instruction vector */
#define T11_IOT         0x010   /* IOT instruction vector */
#define T11_PWRFAIL     0x014   /* Power fail vector */
#define T11_EMT         0x018   /* EMT instruction vector */
#define T11_TRAP        0x01C   /* TRAP instruction vector */

/* PUBLIC GLOBALS */
extern int  t11_ICount;


/* PUBLIC FUNCTIONS */
extern void t11_reset(void *param);
extern void t11_exit(void);
extern int t11_execute(int cycles);    /* NS 970908 */
extern void t11_setregs(t11_Regs *Regs);
extern void t11_getregs(t11_Regs *Regs);
extern unsigned t11_getpc(void);
extern unsigned t11_getreg(int regnum);
extern void t11_setreg(int regnum, unsigned val);
extern void t11_set_nmi_line(int state);
extern void t11_set_irq_line(int irqline, int state);
extern void t11_set_irq_callback(int (*callback)(int irqline));
extern const char *t11_info(void *context, int regnum);

extern void t11_SetBank(int banknum, unsigned char *address);

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
#define T11_RDMEM(A) ((unsigned)cpu_readmem16lew(A))
#define T11_RDMEM_WORD(A) ((unsigned)cpu_readmem16lew_word(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define T11_WRMEM(A,V) (cpu_writemem16lew(A,V))
#define T11_WRMEM_WORD(A,V) (cpu_writemem16lew_word(A,V))

#ifdef MAME_DEBUG
extern int mame_debug;
extern int DasmT11(unsigned char *pBase, char *buffer, int pc);
#endif

#endif /* _T11_H */
