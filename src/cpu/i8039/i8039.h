/**************************************************************************
 *                      Intel 8039 Portable Emulator                      *
 *                                                                        *
 *                   Copyright (C) 1997 by Mirko Buffoni                  *
 *  Based on the original work (C) 1997 by Dan Boris, an 8048 emulator    *
 *     You are not allowed to distribute this software commercially       *
 *        Please, notify me, if you make any changes to this file         *
 **************************************************************************/

#ifndef _I8039_H
#define _I8039_H

#ifndef INLINE
#define INLINE static inline
#endif

#include "osd_cpu.h"

typedef struct
{
	PAIR	PC; 			/* HJB */
	UINT8	A, SP, PSW;
	UINT8	RAM[128];
	UINT8	bus, f1;		/* Bus data, and flag1 */

	int 	pending_irq,irq_executing, masterClock, regPtr;
	UINT8	t_flag, timer, timerON, countON, xirq_en, tirq_en;
	UINT16	A11, A11ff;
	int 	irq_state;
	int 	(*irq_callback)(int irqline);
} I8039_Regs;

extern int i8039_ICount;		/* T-state count						  */

/* HJB 01/05/99 changed to positive values to use pending_irq as a flag */
#define I8039_IGNORE_INT    0   /* Ignore interrupt                     */
#define I8039_EXT_INT		1	/* Execute a normal extern interrupt	*/
#define I8039_TIMER_INT 	2	/* Execute a Timer interrupt			*/
#define I8039_COUNT_INT 	4	/* Execute a Counter interrupt			*/

void i8039_reset(void *param);			/* Reset processor & registers	*/
void i8039_exit(void);					/* Shut down CPU emulation		*/
int i8039_execute(int cycles);			/* Execute cycles T-States - returns number of cycles actually run */
void i8039_setregs(I8039_Regs *Regs);	/* Set registers				*/
void i8039_getregs(I8039_Regs *Regs);	/* Get registers				*/
unsigned i8039_getpc(void); 			/* Get program counter			*/
unsigned i8039_getreg(int regnum);	   	/* Get a specific register      */
void i8039_setreg(int regnum, unsigned val);	/* Set a specific register      */
void i8039_set_nmi_line(int state);
void i8039_set_irq_line(int irqline, int state);
void i8039_set_irq_callback(int (*callback)(int irqline));
const char *i8039_info(void *context, int regnum);

/*   This handling of special I/O ports should be better for actual MAME
 *   architecture.  (i.e., define access to ports { I8039_p1, I8039_p1, dkong_out_w })
 */

#if OLDPORTHANDLING
		UINT8	 I8039_port_r(UINT8 port);
		void	 I8039_port_w(UINT8 port, UINT8 data);
		UINT8	 I8039_test_r(UINT8 port);
		void	 I8039_test_w(UINT8 port, UINT8 data);
		UINT8	 I8039_bus_r(void);
		void	 I8039_bus_w(UINT8 data);
#else
        #define  I8039_p0	0x100   /* Not used */
        #define  I8039_p1	0x101
        #define  I8039_p2	0x102
        #define  I8039_p4	0x104
        #define  I8039_p5	0x105
        #define  I8039_p6	0x106
        #define  I8039_p7	0x107
        #define  I8039_t0	0x110
        #define  I8039_t1	0x111
        #define  I8039_bus	0x120
#endif

/**************************************************************************
 *   For now make the I8035 using the I8039 variables and functions
 **************************************************************************/
#define I8035_IGNORE_INT		I8039_IGNORE_INT
#define I8035_EXT_INT           I8039_EXT_INT
#define I8035_TIMER_INT         I8039_TIMER_INT
#define I8035_COUNT_INT         I8039_COUNT_INT
#define i8035_ICount			i8039_ICount
#define i8035_reset 			i8039_reset
#define i8035_exit				i8039_exit
#define i8035_execute			i8039_execute
#define i8035_setregs			i8039_setregs
#define i8035_getregs			i8039_getregs
#define i8035_getpc 			i8039_getpc
#define i8035_getreg 			i8039_getreg
#define i8035_setreg 			i8039_setreg
#define i8035_set_nmi_line		i8039_set_nmi_line
#define i8035_set_irq_line		i8039_set_irq_line
#define i8035_set_irq_callback	i8039_set_irq_callback
const char *i8035_info(void *context, int regnum);

/**************************************************************************
 *   For now make the I8048 using the I8039 variables and functions
 **************************************************************************/
#define I8048_IGNORE_INT        I8039_IGNORE_INT
#define I8048_EXT_INT           I8039_EXT_INT
#define I8048_TIMER_INT         I8039_TIMER_INT
#define I8048_COUNT_INT         I8039_COUNT_INT
#define i8048_ICount			i8039_ICount
#define i8048_reset 			i8039_reset
#define i8048_exit				i8039_exit
#define i8048_execute			i8039_execute
#define i8048_setregs			i8039_setregs
#define i8048_getregs			i8039_getregs
#define i8048_getpc 			i8039_getpc
#define i8048_getreg 			i8039_getreg
#define i8048_setreg 			i8039_setreg
#define i8048_set_nmi_line		i8039_set_nmi_line
#define i8048_set_irq_line		i8039_set_irq_line
#define i8048_set_irq_callback	i8039_set_irq_callback
const char *i8048_info(void *context, int regnum);

/**************************************************************************
 *   For now make the N7751 using the I8039 variables and functions
 **************************************************************************/
#define N7751_IGNORE_INT        I8039_IGNORE_INT
#define N7751_EXT_INT           I8039_EXT_INT
#define N7751_TIMER_INT         I8039_TIMER_INT
#define N7751_COUNT_INT         I8039_COUNT_INT
#define n7751_ICount			i8039_ICount
#define n7751_reset 			i8039_reset
#define n7751_exit				i8039_exit
#define n7751_execute			i8039_execute
#define n7751_setregs			i8039_setregs
#define n7751_getregs			i8039_getregs
#define n7751_getpc 			i8039_getpc
#define n7751_getreg 			i8039_getreg
#define n7751_setreg 			i8039_setreg
#define n7751_set_nmi_line		i8039_set_nmi_line
#define n7751_set_irq_line		i8039_set_irq_line
#define n7751_set_irq_callback	i8039_set_irq_callback
const char *n7751_info(void *context, int regnum);

#include "memory.h"

/*
 *	 Input a UINT8 from given I/O port
 */
#define I8039_In(Port) ((UINT8)cpu_readport(Port))


/*
 *	 Output a UINT8 to given I/O port
 */
#define I8039_Out(Port,Value) (cpu_writeport(Port,Value))


/*
 *	 Read a UINT8 from given memory location
 */
#define I8039_RDMEM(A) ((unsigned)cpu_readmem16(A))


/*
 *	 Write a UINT8 to given memory location
 */
#define I8039_WRMEM(A,V) (cpu_writemem16(A,V))


/*
 *   I8039_RDOP() is identical to I8039_RDMEM() except it is used for reading
 *   opcodes. In case of system with memory mapped I/O, this function can be
 *   used to greatly speed up emulation
 */
#define I8039_RDOP(A) ((unsigned)cpu_readop(A))


/*
 *   I8039_RDOP_ARG() is identical to I8039_RDOP() except it is used for reading
 *   opcode arguments. This difference can be used to support systems that
 *   use different encoding mechanisms for opcodes and opcode arguments
 */
#define I8039_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

#ifdef  MAME_DEBUG
int     Dasm8039(char * dst, unsigned char* addr);
#endif

#endif  /* _I8039_H */
