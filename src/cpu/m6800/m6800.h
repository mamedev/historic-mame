/*** m6800: Portable 6800 class emulator *************************************/

#ifndef _M6800_H
#define _M6800_H

#include "osd_cpu.h"
#include "memory.h"
#include "cpuintrf.h"

enum {
	M6800_PC=1, M6800_S, M6800_A, M6800_B, M6800_X, M6800_CC,
	M6800_WAI_STATE, M6800_NMI_STATE, M6800_IRQ_STATE };

#define M6800_INT_NONE  0           /* No interrupt required */
#define M6800_INT_IRQ	1			/* Standard IRQ interrupt */
#define M6800_INT_NMI	2			/* NMI interrupt		  */
#define M6800_WAI		8			/* set when WAI is waiting for an interrupt */

#define M6800_IRQ_LINE	0			/* IRQ line number */

/* PUBLIC GLOBALS */
extern int m6800_ICount;

/* PUBLIC FUNCTIONS */
extern void m6800_reset(void *param);
extern void m6800_exit(void);
extern int	m6800_execute(int cycles);
extern unsigned m6800_get_context(void *dst);
extern void m6800_set_context(void *src);
extern unsigned m6800_get_pc(void);
extern void m6800_set_pc(unsigned val);
extern unsigned m6800_get_sp(void);
extern void m6800_set_sp(unsigned val);
extern unsigned m6800_get_reg(int regnum);
extern void m6800_set_reg(int regnum, unsigned val);
extern void m6800_set_nmi_line(int state);
extern void m6800_set_irq_line(int irqline, int state);
extern void m6800_set_irq_callback(int (*callback)(int irqline));
extern void m6800_state_save(void *file);
extern void m6800_state_load(void *file);
extern const char *m6800_info(void *context, int regnum);
extern unsigned m6800_dasm(char *buffer, unsigned pc);

/****************************************************************************
 * For now make the 6801 using the m6800 variables and functions
 ****************************************************************************/
#if HAS_M6801
#define M6801_A 					M6800_A
#define M6801_B 					M6800_B
#define M6801_PC					M6800_PC
#define M6801_S 					M6800_S
#define M6801_X 					M6800_X
#define M6801_CC					M6800_CC
#define M6801_WAI_STATE 			M6800_WAI_STATE
#define M6801_NMI_STATE 			M6800_NMI_STATE
#define M6801_IRQ_STATE 			M6800_IRQ_STATE

#define M6801_INT_NONE              M6800_INT_NONE
#define M6801_INT_IRQ				M6800_INT_IRQ
#define M6801_INT_NMI				M6800_INT_NMI
#define M6801_WAI					M6800_WAI
#define M6801_IRQ_LINE				M6800_IRQ_LINE

#define m6801_ICount				m6800_ICount
extern void m6801_reset(void *param);
extern void m6801_exit(void);
extern int	m6801_execute(int cycles);
extern unsigned m6801_get_context(void *dst);
extern void m6801_set_context(void *src);
extern unsigned m6801_get_pc(void);
extern void m6801_set_pc(unsigned val);
extern unsigned m6801_get_sp(void);
extern void m6801_set_sp(unsigned val);
extern unsigned m6801_get_reg(int regnum);
extern void m6801_set_reg(int regnum, unsigned val);
extern void m6801_set_nmi_line(int state);
extern void m6801_set_irq_line(int irqline, int state);
extern void m6801_set_irq_callback(int (*callback)(int irqline));
extern void m6801_state_save(void *file);
extern void m6801_state_load(void *file);
extern const char *m6801_info(void *context, int regnum);
extern unsigned m6801_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the 6802 using the m6800 variables and functions
 ****************************************************************************/
#if HAS_M6802
#define M6802_A 					M6800_A
#define M6802_B 					M6800_B
#define M6802_PC					M6800_PC
#define M6802_S 					M6800_S
#define M6802_X 					M6800_X
#define M6802_CC					M6800_CC
#define M6802_WAI_STATE 			M6800_WAI_STATE
#define M6802_NMI_STATE 			M6800_NMI_STATE
#define M6802_IRQ_STATE 			M6800_IRQ_STATE

#define M6802_INT_NONE              M6800_INT_NONE
#define M6802_INT_IRQ				M6800_INT_IRQ
#define M6802_INT_NMI				M6800_INT_NMI
#define M6802_WAI					M6800_WAI
#define M6802_IRQ_LINE				M6800_IRQ_LINE

#define m6802_ICount				m6800_ICount
extern void m6802_reset(void *param);
extern void m6802_exit(void);
extern int	m6802_execute(int cycles);
extern unsigned m6802_get_context(void *dst);
extern void m6802_set_context(void *src);
extern unsigned m6802_get_pc(void);
extern void m6802_set_pc(unsigned val);
extern unsigned m6802_get_sp(void);
extern void m6802_set_sp(unsigned val);
extern unsigned m6802_get_reg(int regnum);
extern void m6802_set_reg(int regnum, unsigned val);
extern void m6802_set_nmi_line(int state);
extern void m6802_set_irq_line(int irqline, int state);
extern void m6802_set_irq_callback(int (*callback)(int irqline));
extern void m6802_state_save(void *file);
extern void m6802_state_load(void *file);
extern const char *m6802_info(void *context, int regnum);
extern unsigned m6802_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the 6803 using the m6800 variables and functions
 ****************************************************************************/
#if HAS_M6803
#define M6803_A 					M6800_A
#define M6803_B 					M6800_B
#define M6803_PC					M6800_PC
#define M6803_S 					M6800_S
#define M6803_X 					M6800_X
#define M6803_CC					M6800_CC
#define M6803_WAI_STATE 			M6800_WAI_STATE
#define M6803_NMI_STATE 			M6800_NMI_STATE
#define M6803_IRQ_STATE 			M6800_IRQ_STATE

#define M6803_INT_NONE              M6800_INT_NONE
#define M6803_INT_IRQ				M6800_INT_IRQ
#define M6803_INT_NMI				M6800_INT_NMI
#define M6803_WAI					M6800_WAI
#define M6803_IRQ_LINE				M6800_IRQ_LINE

#define m6803_ICount				m6800_ICount
extern void m6803_reset(void *param);
extern void m6803_exit(void);
extern int	m6803_execute(int cycles);
extern unsigned m6803_get_context(void *dst);
extern void m6803_set_context(void *src);
extern unsigned m6803_get_pc(void);
extern void m6803_set_pc(unsigned val);
extern unsigned m6803_get_sp(void);
extern void m6803_set_sp(unsigned val);
extern unsigned m6803_get_reg(int regnum);
extern void m6803_set_reg(int regnum, unsigned val);
extern void m6803_set_nmi_line(int state);
extern void m6803_set_irq_line(int irqline, int state);
extern void m6803_set_irq_callback(int (*callback)(int irqline));
extern void m6803_state_save(void *file);
extern void m6803_state_load(void *file);
extern const char *m6803_info(void *context, int regnum);
extern unsigned m6803_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the 6808 using the m6800 variables and functions
 ****************************************************************************/
#if HAS_M6808
#define M6808_A 					M6800_A
#define M6808_B 					M6800_B
#define M6808_PC					M6800_PC
#define M6808_S 					M6800_S
#define M6808_X 					M6800_X
#define M6808_CC					M6800_CC
#define M6808_WAI_STATE 			M6800_WAI_STATE
#define M6808_NMI_STATE 			M6800_NMI_STATE
#define M6808_IRQ_STATE 			M6800_IRQ_STATE

#define M6808_INT_NONE              M6800_INT_NONE
#define M6808_INT_IRQ               M6800_INT_IRQ
#define M6808_INT_NMI               M6800_INT_NMI
#define M6808_WAI                   M6800_WAI
#define M6808_IRQ_LINE              M6800_IRQ_LINE

#define m6808_ICount                m6800_ICount
extern void m6808_reset(void *param);
extern void m6808_exit(void);
extern int	m6808_execute(int cycles);
extern unsigned m6808_get_context(void *dst);
extern void m6808_set_context(void *src);
extern unsigned m6808_get_pc(void);
extern void m6808_set_pc(unsigned val);
extern unsigned m6808_get_sp(void);
extern void m6808_set_sp(unsigned val);
extern unsigned m6808_get_reg(int regnum);
extern void m6808_set_reg(int regnum, unsigned val);
extern void m6808_set_nmi_line(int state);
extern void m6808_set_irq_line(int irqline, int state);
extern void m6808_set_irq_callback(int (*callback)(int irqline));
extern void m6808_state_save(void *file);
extern void m6808_state_load(void *file);
extern const char *m6808_info(void *context, int regnum);
extern unsigned m6808_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the HD63701 using the m6800 variables and functions
 ****************************************************************************/
#if HAS_HD63701
#define HD63701_A					 M6800_A
#define HD63701_B					 M6800_B
#define HD63701_PC					 M6800_PC
#define HD63701_S					 M6800_S
#define HD63701_X					 M6800_X
#define HD63701_CC					 M6800_CC
#define HD63701_WAI_STATE			 M6800_WAI_STATE
#define HD63701_NMI_STATE			 M6800_NMI_STATE
#define HD63701_IRQ_STATE			 M6800_IRQ_STATE

#define HD63701_INT_NONE             M6800_INT_NONE
#define HD63701_INT_IRQ 			 M6800_INT_IRQ
#define HD63701_INT_NMI 			 M6800_INT_NMI
#define HD63701_WAI 				 M6800_WAI
#define HD63701_IRQ_LINE			 M6800_IRQ_LINE

#define hd63701_ICount				 m6800_ICount
extern void hd63701_reset(void *param);
extern void hd63701_exit(void);
extern int	hd63701_execute(int cycles);
extern unsigned hd63701_get_context(void *dst);
extern void hd63701_set_context(void *src);
extern unsigned hd63701_get_pc(void);
extern void hd63701_set_pc(unsigned val);
extern unsigned hd63701_get_sp(void);
extern void hd63701_set_sp(unsigned val);
extern unsigned hd63701_get_reg(int regnum);
extern void hd63701_set_reg(int regnum, unsigned val);
extern void hd63701_set_nmi_line(int state);
extern void hd63701_set_irq_line(int irqline, int state);
extern void hd63701_set_irq_callback(int (*callback)(int irqline));
extern void hd63701_state_save(void *file);
extern void hd63701_state_load(void *file);
extern const char *hd63701_info(void *context, int regnum);
extern unsigned hd63701_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6800_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6800_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6800_RDOP() is identical to M6800_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6800_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6800_RDOP_ARG() is identical to M6800_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6800_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6800_Flags	*/
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6800_FAST_S is set, bytes are pulled 		*/
/* directly from the RAM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6800_Flags;
#define M6800_FAST_NONE 0x00	/* no memory optimizations */
#define M6800_FAST_S	0x02	/* stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef	MAME_DEBUG
extern unsigned Dasm680x(int subtype, char *buf, unsigned pc);
#endif


/* By default, on a port write port bits which are not set as output in the DDR */
/* are set to 1. If you need to know the DDR for e.g. port 1, do */
/* m6803_internal_registers_r(M6801_DDR1) */
#define M6803_DDR1	0x00
#define M6803_DDR2	0x01

#define M6803_PORT1 0x100
#define M6803_PORT2 0x101

#define HD63701_DDR1 M6803_DDR1
#define HD63701_DDR2 M6803_DDR2

#define HD63701_PORT1 M6803_PORT1
#define HD63701_PORT2 M6803_PORT2

int m6803_internal_registers_r(int offset);
void m6803_internal_registers_w(int offset,int data);
int hd63701_internal_registers_r(int offset);
void hd63701_internal_registers_w(int offset,int data);

#endif /* _M6800_H */
