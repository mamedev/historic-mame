/****************************************************************************
 *					Texas Instruments TMS320C10 DSP Emulator				*
 *																			*
 *						Copyright (C) 1999 by Quench						*
 *		You are not allowed to distribute this software commercially.		*
 *						Written for the MAME project.						*
 *																			*
 *				Note: This is a word based microcontroller.					*
 ****************************************************************************/

#ifndef _TMS320C10_H
#define _TMS320C10_H

/*************************************************************************
 * If your compiler doesn't know about inlined functions, uncomment this *
 *************************************************************************/

/* #define INLINE static */

#include "osd_cpu.h"
#include "cpuintrf.h"


#ifndef INLINE
#define INLINE static inline
#endif
typedef struct
{
		UINT16	PC;
		INT32	ACC, Preg;
		INT32	ALU;
		UINT16	Treg;
		UINT16	AR[2], STACK[4], STR;
		int		pending_irq, BIO_pending_irq;
#if NEW_INTERRUPT_SYSTEM
		int		irq_state;
		int		(*irq_callback)(int irqline);
#endif

} TMS320C10_Regs;


extern	int TMS320C10_ICount;		/* T-state count */

#define TMS320C10_ACTIVE_INT  0		/* Activate INT external interrupt		 */
#define TMS320C10_ACTIVE_BIO  1		/* Activate BIO for use with BIOZ inst	 */
#define TMS320C10_IGNORE_BIO  -1	/* Inhibit BIO polled external interrupt */
#define	TMS320C10_PENDING	  0x80000000
#define TMS320C10_NOT_PENDING 0
#define TMS320C10_INT_NONE	  -1

#define  TMS320C10_ADDR_MASK  0x0fff	/* TMS320C10 can only address 0x0fff */
										/* however other TMS320C1x devices	 */
										/* can address up to 0xffff (incase  */
										/* their support is ever added).	 */

unsigned TMS320C10_GetPC  (void);				  /* Get program counter	*/
void	 TMS320C10_GetRegs(TMS320C10_Regs *Regs); /* Get registers			*/
void	 TMS320C10_SetRegs(TMS320C10_Regs *Regs); /* Set registers			*/
void	 TMS320C10_Reset  (void);			/* Reset processor & registers	*/
int		 TMS320C10_Execute(int cycles);		/* Execute cycles T-States - 	*/
											/* returns number of cycles actually run */

#if NEW_INTERRUPT_SYSTEM
void TMS320C10_set_nmi_line(int state);
void TMS320C10_set_irq_line(int irqline, int state);
void TMS320C10_set_irq_callback(int (*callback)(int irqline));
#else
void	TMS320C10_Cause_Interrupts(int type);
void	TMS320C10_Clear_Pending_Interrupts(void);
#endif


#include "memory.h"

/*	 Input a word from given I/O port
 */
#define TMS320C10_In(Port) ((UINT16)cpu_readport(Port))


/*	 Output a word to given I/O port
 */
#define TMS320C10_Out(Port,Value) (cpu_writeport(Port,Value))


/*	 Read a word from given ROM memory location
 * #define TMS320C10_ROM_RDMEM(A) READ_WORD(&ROM[(A<<1)])
 */
#define TMS320C10_ROM_RDMEM(A) (unsigned)((cpu_readmem16((A<<1))<<8) | cpu_readmem16(((A<<1)+1)))

/*	 Write a word to given ROM memory location
 * #define TMS320C10_ROM_WRMEM(A,V) WRITE_WORD(&ROM[(A<<1)],V)
 */
#define TMS320C10_ROM_WRMEM(A,V) { cpu_writemem16(((A<<1)+1),(V&0xff)); cpu_writemem16((A<<1),((V>>8)&0xff)); }


/*	 Read a word from given RAM memory location
	 The following adds 8000h to the address, since MAME doesnt support
	 RAM and ROM living in the same address space. RAM really starts at
	 address 0 and are word entities.
 * #define TMS320C10_RAM_RDMEM(A) ((unsigned)cpu_readmem16lew_word(((A<<1)|0x8000)))
 */
#define TMS320C10_RAM_RDMEM(A) (unsigned)((cpu_readmem16((A<<1)|0x8000)<<8) | cpu_readmem16(((A<<1)|0x8001)))


/*	 Write a word to given RAM memory location
	 The following adds 8000h to the address, since MAME doesnt support
	 RAM and ROM living in the same address space. RAM really starts at
	 address 0 and word entities.
 * #define TMS320C10_RAM_WRMEM(A,V) (cpu_writemem16lew_word(((A<<1)|0x8000),V))
 */
#define TMS320C10_RAM_WRMEM(A,V) { cpu_writemem16(((A<<1)|0x8001),(V&0x0ff)); cpu_writemem16(((A<<1)|0x8000),((V>>8)&0x0ff)); }


/*	 TMS320C10_RDOP() is identical to TMS320C10_RDMEM() except it is used for reading
 *	 opcodes. In case of system with memory mapped I/O, this function can be
 *	 used to greatly speed up emulation
 */
#define TMS320C10_RDOP(A) (unsigned)((cpu_readop((A<<1))<<8) | cpu_readop(((A<<1)+1)))


/*	 TMS320C10_RDOP_ARG() is identical to TMS320C10_RDOP() except it is used for reading
 *	 opcode arguments. This difference can be used to support systems that
 *	 use different encoding mechanisms for opcodes and opcode arguments
 */
#define TMS320C10_RDOP_ARG(A) (unsigned)((cpu_readop_arg((A<<1))<<8) | cpu_readop_arg(((A<<1)+1)))

#ifdef	MAME_DEBUG
int		Dasm32010(char * dst, unsigned char* addr);
#endif

#endif	/* _TMS320C10_H */
