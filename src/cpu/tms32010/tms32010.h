/****************************************************************************
 *					Texas Instruments TMS320C10 DSP Emulator				*
 *																			*
 *						Copyright (C) 1999 by Quench						*
 *		You are not allowed to distribute this software commercially.		*
 *						Written for the MAME project.						*
 *																			*
 *				Note: This is a word based microcontroller. 				*
 ****************************************************************************/

#ifndef _TMS320C10_H
#define _TMS320C10_H

/*************************************************************************
 * If your compiler doesn't know about inlined functions, uncomment this *
 *************************************************************************/

/* #define INLINE static */

#include "osd_cpu.h"
#include "cpuintrf.h"

enum {
	TMS320C10_PC=1, TMS320C10_SP, TMS320C10_STR, TMS320C10_ACC,
	TMS320C10_PREG, TMS320C10_TREG, TMS320C10_AR0, TMS320C10_AR1,
	TMS320C10_STK0, TMS320C10_STK1, TMS320C10_STK2, TMS320C10_STK3
};

extern	int tms320c10_icount;		/* T-state count */

#define TMS320C10_DATA_OFFSET	0x0000
#define TMS320C10_PGM_OFFSET	0x8000

#define TMS320C10_ACTIVE_INT  0 	/* Activate INT external interrupt		 */
#define TMS320C10_ACTIVE_BIO  1 	/* Activate BIO for use with BIOZ inst	 */
#define TMS320C10_IGNORE_BIO  -1	/* Inhibit BIO polled external interrupt */
#define TMS320C10_PENDING	  0x80000000
#define TMS320C10_NOT_PENDING 0
#define TMS320C10_INT_NONE	  -1

#define  TMS320C10_ADDR_MASK  0x0fff	/* TMS320C10 can only address 0x0fff */
										/* however other TMS320C1x devices	 */
										/* can address up to 0xffff (incase  */
										/* their support is ever added).	 */

void tms320c10_init(void);
void tms320c10_reset  (void *param);			/* Reset processor & registers	*/
void tms320c10_exit(void);						/* Shutdown CPU core			*/
int tms320c10_execute(int cycles);				/* Execute cycles T-States -	*/
												/* returns number of cycles actually run */
unsigned tms320c10_get_context(void *dst);		/* Get registers			*/
void tms320c10_set_context(void *src);			/* Set registers			*/
unsigned tms320c10_get_reg(int regnum); 		/* Get specific register	*/
void tms320c10_set_reg(int regnum, unsigned val);/* Set specific register	*/
void tms320c10_set_irq_line(int irqline, int state);
void tms320c10_set_irq_callback(int (*callback)(int irqline));
const char *tms320c10_info(void *context, int regnum);
unsigned tms320c10_dasm(char *buffer, unsigned pc);

#include "memory.h"

/*	 Input a word from given I/O port
 */
#define TMS320C10_In(Port) (cpu_readport16bew_word((Port)<<1))


/*	 Output a word to given I/O port
 */
#define TMS320C10_Out(Port,Value) (cpu_writeport16bew_word((Port<<1),Value))


/*	 Read a word from given ROM memory location
 */
#define TMS320C10_ROM_RDMEM(A) (cpu_readmem16bew_word(((A)<<1)+TMS320C10_PGM_OFFSET))

/*	 Write a word to given ROM memory location
 */
#define TMS320C10_ROM_WRMEM(A,V) (cpu_writemem16bew_word(((A)<<1)+TMS320C10_PGM_OFFSET,V))

/*
 * Read a word from given RAM memory location
 * The following adds 8000h to the address, since MAME doesnt support
 * RAM and ROM living in the same address space. RAM really starts at
 * address 0 and are word entities.
 */
#define TMS320C10_RAM_RDMEM(A) (cpu_readmem16bew_word(((A)<<1)+TMS320C10_DATA_OFFSET))

/*	 Write a word to given RAM memory location
	 The following adds 8000h to the address, since MAME doesnt support
	 RAM and ROM living in the same address space. RAM really starts at
	 address 0 and word entities.
 */
#define TMS320C10_RAM_WRMEM(A,V) (cpu_writemem16bew_word(((A)<<1)+TMS320C10_DATA_OFFSET,V))

/*	 TMS320C10_RDOP() is identical to TMS320C10_RDMEM() except it is used for reading
 *	 opcodes. In case of system with memory mapped I/O, this function can be
 *	 used to greatly speed up emulation
 */
#define TMS320C10_RDOP(A) (cpu_readop16(((A)<<1)+TMS320C10_PGM_OFFSET))

/*
 * TMS320C10_RDOP_ARG() is identical to TMS320C10_RDOP() except it is used
 * for reading opcode arguments. This difference can be used to support systems
 * that use different encoding mechanisms for opcodes and opcode arguments
 */
#define TMS320C10_RDOP_ARG(A) (cpu_readop_arg16(((A)<<1)+TMS320C10_PGM_OFFSET))

#ifdef	MAME_DEBUG
extern unsigned Dasm32010(char *buffer, unsigned pc);
#endif

#endif	/* _TMS320C10_H */

