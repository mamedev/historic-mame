 /**************************************************************************\
 *						Microchip PIC16C5x Emulator							*
 *																			*
 *					  Copyright (C) 2003+ Tony La Porta						*
 *				   Originally written for the MAME project.					*
 *																			*
 *																			*
 *		Addressing architecture is based on the Harvard addressing scheme.	*
 *																			*
 \**************************************************************************/

#ifndef _PIC16C5X_H
#define _PIC16C5X_H


#include "osd_cpu.h"
#include "cpuintrf.h"
#include "memory.h"


/**************************************************************************
 *	Internal Clock divisor
 *
 *	External Clock is divided internally by 4 for the instruction cycle
 *	times. (Each instruction cycle passes through 4 machine states).
 */

#define PIC16C5x_CLOCK_DIVIDER		4


enum {
	PIC16C5x_PC=1, PIC16C5x_STK0, PIC16C5x_STK1, PIC16C5x_FSR,
	PIC16C5x_W,    PIC16C5x_ALU,  PIC16C5x_STR,  PIC16C5x_OPT,
	PIC16C5x_TMR0, PIC16C5x_PRTA, PIC16C5x_PRTB, PIC16C5x_PRTC,
	PIC16C5x_WDT,  PIC16C5x_TRSA, PIC16C5x_TRSB, PIC16C5x_TRSC,
	PIC16C5x_PSCL
};


/****************************************************************************
 *	Function to configure the CONFIG register. This is actually hard-wired
 *	during ROM programming, so should be called in the driver INIT, with
 *	the value if known (available in HEX dumps of the ROM).
 */

void pic16c5x_config(int data);


/****************************************************************************
 *	Read the state of the T0 Clock input signal
 */

#define PIC16C5x_T0		0x10
#define PIC16C5x_T0_In (io_read_byte_8(PIC16C5x_T0))


/****************************************************************************
 *	Input a word from given I/O port
 */

#define PIC16C5x_In(Port) ((UINT8)io_read_byte_8((Port)))


/****************************************************************************
 *	Output a word to given I/O port
 */

#define PIC16C5x_Out(Port,Value) (io_write_byte_8((Port),Value))



/****************************************************************************
 *	Read a word from given RAM memory location
 */

#define PIC16C5x_RAM_RDMEM(A) ((UINT8)data_read_byte_8(A))


/****************************************************************************
 *	Write a word to given RAM memory location
 */

#define PIC16C5x_RAM_WRMEM(A,V) (data_write_byte_8(A,V))



/****************************************************************************
 *	PIC16C5X_RDOP() is identical to PIC16C5X_RDMEM() except it is used for
 *	reading opcodes. In case of system with memory mapped I/O, this function
 *	can be used to greatly speed up emulation
 */

#define PIC16C5x_RDOP(A) (cpu_readop16((A)<<1))


/****************************************************************************
 *	PIC16C5X_RDOP_ARG() is identical to PIC16C5X_RDOP() except it is used
 *	for reading opcode arguments. This difference can be used to support systems
 *	that use different encoding mechanisms for opcodes and opcode arguments
 */

#define PIC16C5x_RDOP_ARG(A) (cpu_readop_arg16((A)<<1))




#if (HAS_PIC16C54)
/****************************************************************************
 *	PIC16C54
 ****************************************************************************/
#define pic16C54_icount			pic16C5x_icount
#define PIC16C54_RESET_VECTOR	0x1ff

#define PIC16C54_PROGRAM_MEMORY_READ			\
	AM_RANGE(0x000, 0x1ff) AM_READ(MRA16_ROM)

#define PIC16C54_PROGRAM_MEMORY_WRITE			\
	AM_RANGE(0x000, 0x1ff) AM_WRITE(MWA16_ROM)

#define PIC16C54_DATA_MEMORY_READ				\
	AM_RANGE(0x00, 0x1f) AM_READ(MRA8_RAM)

#define PIC16C54_DATA_MEMORY_WRITE				\
	AM_RANGE(0x00, 0x1f) AM_WRITE(MWA8_RAM)

void pic16C54_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C55)
/****************************************************************************
 *	PIC16C55
 ****************************************************************************/
#define pic16C55_icount			pic16C5x_icount
#define PIC16C55_RESET_VECTOR	0x1ff

#define PIC16C55_PROGRAM_MEMORY_READ			\
	AM_RANGE(0x000, 0x1ff) AM_READ(MRA16_ROM)

#define PIC16C55_PROGRAM_MEMORY_WRITE			\
	AM_RANGE(0x000, 0x1ff) AM_WRITE(MWA16_ROM)

#define PIC16C55_DATA_MEMORY_READ				\
	AM_RANGE(0x00, 0x1f) AM_READ(MRA8_RAM)

#define PIC16C55_DATA_MEMORY_WRITE				\
	AM_RANGE(0x00, 0x1f) AM_WRITE(MWA8_RAM)

void pic16C55_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C56)
/****************************************************************************
 *	PIC16C56
 ****************************************************************************/
#define pic16C56_icount			pic16C5x_icount
#define PIC16C56_RESET_VECTOR	0x3ff

#define PIC16C56_PROGRAM_MEMORY_READ			\
	AM_RANGE(0x000, 0x3ff) AM_READ(MRA16_ROM)

#define PIC16C56_PROGRAM_MEMORY_WRITE			\
	AM_RANGE(0x000, 0x3ff) AM_WRITE(MWA16_ROM)

#define PIC16C56_DATA_MEMORY_READ				\
	AM_RANGE(0x00, 0x1f) AM_READ(MRA8_RAM)

#define PIC16C56_DATA_MEMORY_WRITE				\
	AM_RANGE(0x00, 0x1f) AM_WRITE(MWA8_RAM)

void pic16C56_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C57)
/****************************************************************************
 *	PIC16C57
 ****************************************************************************/
#define pic16C57_icount			pic16C5x_icount
#define PIC16C57_RESET_VECTOR	0x7ff

#define PIC16C57_PROGRAM_MEMORY_READ			\
	AM_RANGE(0x000, 0x7ff) AM_READ(MRA16_ROM)

#define PIC16C57_PROGRAM_MEMORY_WRITE			\
	AM_RANGE(0x000, 0x7ff) AM_WRITE(MWA16_ROM)

#define PIC16C57_DATA_MEMORY_READ				\
	AM_RANGE(0x00, 0x7f) AM_READ(MRA8_RAM)

#define PIC16C57_DATA_MEMORY_WRITE				\
	AM_RANGE(0x00, 0x7f) AM_WRITE(MWA8_RAM)

void pic16C57_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C58)
/****************************************************************************
 *	PIC16C58
 ****************************************************************************/
#define pic16C58_icount			pic16C5x_icount
#define PIC16C58_RESET_VECTOR	0x7ff

#define PIC16C57_PROGRAM_MEMORY_READ			\
	AM_RANGE(0x000, 0x7ff) AM_READ(MRA16_ROM)

#define PIC16C57_PROGRAM_MEMORY_WRITE			\
	AM_RANGE(0x000, 0x7ff) AM_WRITE(MWA16_ROM)

#define PIC16C57_DATA_MEMORY_READ				\
	AM_RANGE(0x00, 0x7f) AM_READ(MRA8_RAM)

#define PIC16C57_DATA_MEMORY_WRITE				\
	AM_RANGE(0x00, 0x7f) AM_WRITE(MWA8_RAM)

void pic16C58_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif

#endif	/* _PIC16C5X_H */
