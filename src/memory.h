/***************************************************************************

	memory.h

	Functions which handle the CPU memory accesses.

***************************************************************************/

#ifndef _MEMORY_H
#define _MEMORY_H

#include "osd_cpu.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Versions of GNU C earlier that 2.7 appear to have problems with the
 * __attribute__ definition of UNUSEDARG, so we act as if it was not a
 * GNU compiler.
 */

#ifdef __GNUC__
#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
#define UNUSEDARG
#else
#define UNUSEDARG __attribute__((__unused__))
#endif
#else
#define UNUSEDARG
#endif


/*
 * Use __builtin_expect on GNU C 3.0 and above
 */
#ifdef __GNUC__
#if (__GNUC__ < 3)
#define UNEXPECTED(exp)	(exp)
#else
#define UNEXPECTED(exp)	 __builtin_expect((exp), 0)
#endif
#else
#define UNEXPECTED(exp)	(exp)
#endif


/***************************************************************************

    Floating-point <-> unsigned converters

***************************************************************************/


INLINE float u2f(UINT32 v)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.vv = v;
	return u.ff;
}

INLINE UINT32 f2u(float f)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.ff = f;
	return u.vv;
}

INLINE float u2d(UINT64 v)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.vv = v;
	return u.dd;
}

INLINE UINT64 d2u(double d)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.dd = d;
	return u.vv;
}


/***************************************************************************

	Parameters

***************************************************************************/

#ifdef MAME_DEBUG
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		1
#elif defined(MESS)
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	1
#define CPUREADOP_SAFETY_FULL		0
#else
#define CPUREADOP_SAFETY_NONE		1
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		0
#endif


	
/***************************************************************************

	Basic type definitions

	These types are used for data access handlers.

***************************************************************************/

/* ----- typedefs for data and offset types ----- */
typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT64			data64_t;
typedef UINT32			offs_t;

/* ----- typedefs for the various common data access handlers ----- */
typedef data8_t			(*read8_handler)  (UNUSEDARG offs_t offset);
typedef void			(*write8_handler) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
typedef data16_t		(*read16_handler) (UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask);
typedef void			(*write16_handler)(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask);
typedef data32_t		(*read32_handler) (UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask);
typedef void			(*write32_handler)(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask);
typedef data64_t		(*read64_handler) (UNUSEDARG offs_t offset, UNUSEDARG data64_t mem_mask);
typedef void			(*write64_handler)(UNUSEDARG offs_t offset, UNUSEDARG data64_t data, UNUSEDARG data64_t mem_mask);
typedef offs_t			(*opbase_handler) (UNUSEDARG offs_t address);

/* ----- this struct contains pointers to the live read/write routines ----- */
struct data_accessors_t
{
	data8_t			(*read_byte)(offs_t offset);
	data16_t		(*read_word)(offs_t offset);
	data32_t		(*read_dword)(offs_t offset);
	data64_t		(*read_qword)(offs_t offset);

	void			(*write_byte)(offs_t offset, data8_t data);
	void			(*write_word)(offs_t offset, data16_t data);
	void			(*write_dword)(offs_t offset, data32_t data);
	void			(*write_qword)(offs_t offset, data64_t data);
};

/* ----- for generic function pointers ----- */
typedef void genf(void);


/***************************************************************************

	Basic macros

***************************************************************************/

/* ----- macros for declaring the various common data access handlers ----- */
#define READ8_HANDLER(name) 	data8_t  name(UNUSEDARG offs_t offset)
#define WRITE8_HANDLER(name) 	void     name(UNUSEDARG offs_t offset, UNUSEDARG data8_t data)
#define READ16_HANDLER(name)	data16_t name(UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask)
#define WRITE16_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask)
#define READ32_HANDLER(name)	data32_t name(UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask)
#define WRITE32_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask)
#define READ64_HANDLER(name)	data64_t name(UNUSEDARG offs_t offset, UNUSEDARG data64_t mem_mask)
#define WRITE64_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data64_t data, UNUSEDARG data64_t mem_mask)
#define OPBASE_HANDLER(name)	offs_t   name(UNUSEDARG offs_t address)

/* ----- macros for accessing bytes and words within larger chunks ----- */
#ifdef LSB_FIRST
	#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE_XOR_LE(a)  	(a)
	#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define BYTE4_XOR_LE(a) 	(a)
	#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define WORD_XOR_LE(a)  	(a)
	#define BYTE8_XOR_BE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
	#define BYTE8_XOR_LE(a) 	(a)
	#define WORD2_XOR_BE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
	#define WORD2_XOR_LE(a)  	(a)
	#define DWORD_XOR_BE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
	#define DWORD_XOR_LE(a)  	(a)
#else
	#define BYTE_XOR_BE(a)  	(a)
	#define BYTE_XOR_LE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE4_XOR_BE(a) 	(a)
	#define BYTE4_XOR_LE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define WORD_XOR_BE(a)  	(a)
	#define WORD_XOR_LE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define BYTE8_XOR_BE(a) 	(a)
	#define BYTE8_XOR_LE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
	#define WORD2_XOR_BE(a)  	(a)
	#define WORD2_XOR_LE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
	#define DWORD_XOR_BE(a)  	(a)
	#define DWORD_XOR_LE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
#endif



/***************************************************************************

	Address array constants

	These apply to values in the array of read/write handlers that is
	declared within each driver.

***************************************************************************/

/* ----- definitions for the flags in the address maps ----- */
#define AM_FLAGS_EXTENDED		0x01					/* this is an extended entry with the below flags in the start field */
#define AM_FLAGS_MATCH_MASK		0x02					/* this entry should have the start/end pair treated as a match/mask pair */
#define AM_FLAGS_END			0x04					/* this is the terminating entry in the array */

/* ----- definitions for the extended flags in the address maps ----- */
#define AMEF_SPECIFIES_SPACE	0x00000001				/* set if the address space is specified */
#define AMEF_SPECIFIES_ABITS	0x00000002				/* set if the number of address space bits is specified */
#define AMEF_SPECIFIES_DBITS	0x00000004				/* set if the databus width is specified */
#define AMEF_SPECIFIES_UNMAP	0x00000008				/* set if the unmap value is specified */

/* ----- definitions for specifying the address space in the extended flags ----- */
#define AMEF_SPACE_SHIFT		8						/* shift to get at the address space */
#define AMEF_SPACE_MASK			(0x0f << AMEF_SPACE_SHIFT) /* mask to get at the address space */
#define AMEF_SPACE(x)			(((x) << AMEF_SPACE_SHIFT) | AMEF_SPECIFIES_SPACE) /* specifies a given address space */

/* ----- definitions for specifying the address bus width in the extended flags ----- */
#define AMEF_ABITS_SHIFT		12						/* shift to get the address bits count */
#define AMEF_ABITS_MASK			(0x3f << AMEF_ABITS_SHIFT) /* mask to get at the address bits count */
#define AMEF_ABITS(n)			(((n) << AMEF_ABITS_SHIFT) | AMEF_SPECIFIES_ABITS) /* specifies a given number of address  */

/* ----- definitions for specifying the data bus width in the extended flags ----- */
#define AMEF_DBITS_SHIFT		18						/* shift to get the data bits count */
#define AMEF_DBITS_MASK			(0x07 << AMEF_DBITS_SHIFT) /* mask to get at the data bits count */
#define AMEF_DBITS(n)			((((n)/8-1) << AMEF_DBITS_SHIFT) | AMEF_SPECIFIES_DBITS) /* specifies a given data bus width */

/* ----- definitions for specifying the unmap value in the extended flags ----- */
#define AMEF_UNMAP_SHIFT		21						/* shift to get the unmap value */
#define AMEF_UNMAP_MASK			(1 << AMEF_UNMAP_SHIFT)	/* mask to get at the unmap value */
#define AMEF_UNMAP(x)			(((x) << AMEF_UNMAP_SHIFT) | AMEF_SPECIFIES_UNMAP) /* specifies a given unmap value */

/* ----- static data access handler constants ----- */
#define STATIC_INVALID			0						/* invalid - should never be used */
#define STATIC_BANK1			1						/* banked memory #1 */
#define STATIC_BANK2			2						/* banked memory #2 */
#define STATIC_BANK3			3						/* banked memory #3 */
#define STATIC_BANK4			4						/* banked memory #4 */
#define STATIC_BANK5			5						/* banked memory #5 */
#define STATIC_BANK6			6						/* banked memory #6 */
#define STATIC_BANK7			7						/* banked memory #7 */
#define STATIC_BANK8			8						/* banked memory #8 */
#define STATIC_BANK9			9						/* banked memory #9 */
#define STATIC_BANK10			10						/* banked memory #10 */
#define STATIC_BANK11			11						/* banked memory #11 */
#define STATIC_BANK12			12						/* banked memory #12 */
#define STATIC_BANK13			13						/* banked memory #13 */
#define STATIC_BANK14			14						/* banked memory #14 */
#define STATIC_BANK15			15						/* banked memory #15 */
#define STATIC_BANK16			16						/* banked memory #16 */
#define STATIC_BANK17			17						/* banked memory #17 */
#define STATIC_BANK18			18						/* banked memory #18 */
#define STATIC_BANK19			19						/* banked memory #19 */
#define STATIC_BANK20			20						/* banked memory #20 */
#define STATIC_BANK21			21						/* banked memory #21 */
#define STATIC_BANK22			22						/* banked memory #22 */
#define STATIC_BANK23			23						/* banked memory #23 */
#define STATIC_BANK24			24						/* banked memory #24 */
/* entries 25-58 are reserved for dynamically allocated internal banks */
#define STATIC_RAM				59						/* RAM - standard reads/writes */
#define STATIC_ROM				60						/* ROM - standard reads, no writes */
#define STATIC_RAMROM			61						/* RAMROM - use for access in encrypted 8-bit systems */
#define STATIC_NOP				62						/* unmapped - all unmapped memory goes here */
#define STATIC_UNMAP			63						/* unmapped - all unmapped memory goes here */
#define STATIC_COUNT			64						/* total number of static handlers */

/* ----- banking constants ----- */
#define MAX_BANKS				58						/* maximum number of banks */
#define MAX_EXPLICIT_BANKS		24						/* maximum number of explicitly-defined banks */
#define STATIC_BANKMAX			(STATIC_RAM - 1)		/* handler constant of last bank */



/***************************************************************************

	Constants for static entries in address map read/write arrays

	The first 32 entries in the address lookup table are reserved for
	"static" handlers. These are internal handlers for RAM, ROM, banks,
	and unmapped areas in the address space. The following definitions 
	are the properly-casted versions of the STATIC_ constants above.

***************************************************************************/

/* 8-bit reads */
#define MRA8_BANK1				((read8_handler)STATIC_BANK1)
#define MRA8_BANK2				((read8_handler)STATIC_BANK2)
#define MRA8_BANK3				((read8_handler)STATIC_BANK3)
#define MRA8_BANK4				((read8_handler)STATIC_BANK4)
#define MRA8_BANK5				((read8_handler)STATIC_BANK5)
#define MRA8_BANK6				((read8_handler)STATIC_BANK6)
#define MRA8_BANK7				((read8_handler)STATIC_BANK7)
#define MRA8_BANK8				((read8_handler)STATIC_BANK8)
#define MRA8_BANK9				((read8_handler)STATIC_BANK9)
#define MRA8_BANK10				((read8_handler)STATIC_BANK10)
#define MRA8_BANK11				((read8_handler)STATIC_BANK11)
#define MRA8_BANK12				((read8_handler)STATIC_BANK12)
#define MRA8_BANK13				((read8_handler)STATIC_BANK13)
#define MRA8_BANK14				((read8_handler)STATIC_BANK14)
#define MRA8_BANK15				((read8_handler)STATIC_BANK15)
#define MRA8_BANK16				((read8_handler)STATIC_BANK16)
#define MRA8_BANK17				((read8_handler)STATIC_BANK17)
#define MRA8_BANK18				((read8_handler)STATIC_BANK18)
#define MRA8_BANK19				((read8_handler)STATIC_BANK19)
#define MRA8_BANK20				((read8_handler)STATIC_BANK20)
#define MRA8_BANK21				((read8_handler)STATIC_BANK21)
#define MRA8_BANK22				((read8_handler)STATIC_BANK22)
#define MRA8_BANK23				((read8_handler)STATIC_BANK23)
#define MRA8_BANK24				((read8_handler)STATIC_BANK24)
#define MRA8_NOP				((read8_handler)STATIC_NOP)
#define MRA8_RAM				((read8_handler)STATIC_RAM)
#define MRA8_ROM				((read8_handler)STATIC_ROM)

/* 8-bit writes */
#define MWA8_BANK1				((write8_handler)STATIC_BANK1)
#define MWA8_BANK2				((write8_handler)STATIC_BANK2)
#define MWA8_BANK3				((write8_handler)STATIC_BANK3)
#define MWA8_BANK4				((write8_handler)STATIC_BANK4)
#define MWA8_BANK5				((write8_handler)STATIC_BANK5)
#define MWA8_BANK6				((write8_handler)STATIC_BANK6)
#define MWA8_BANK7				((write8_handler)STATIC_BANK7)
#define MWA8_BANK8				((write8_handler)STATIC_BANK8)
#define MWA8_BANK9				((write8_handler)STATIC_BANK9)
#define MWA8_BANK10				((write8_handler)STATIC_BANK10)
#define MWA8_BANK11				((write8_handler)STATIC_BANK11)
#define MWA8_BANK12				((write8_handler)STATIC_BANK12)
#define MWA8_BANK13				((write8_handler)STATIC_BANK13)
#define MWA8_BANK14				((write8_handler)STATIC_BANK14)
#define MWA8_BANK15				((write8_handler)STATIC_BANK15)
#define MWA8_BANK16				((write8_handler)STATIC_BANK16)
#define MWA8_BANK17				((write8_handler)STATIC_BANK17)
#define MWA8_BANK18				((write8_handler)STATIC_BANK18)
#define MWA8_BANK19				((write8_handler)STATIC_BANK19)
#define MWA8_BANK20				((write8_handler)STATIC_BANK20)
#define MWA8_BANK21				((write8_handler)STATIC_BANK21)
#define MWA8_BANK22				((write8_handler)STATIC_BANK22)
#define MWA8_BANK23				((write8_handler)STATIC_BANK23)
#define MWA8_BANK24				((write8_handler)STATIC_BANK24)
#define MWA8_NOP				((write8_handler)STATIC_NOP)
#define MWA8_RAM				((write8_handler)STATIC_RAM)
#define MWA8_ROM				((write8_handler)STATIC_ROM)
#define MWA8_RAMROM				((write8_handler)STATIC_RAMROM)

/* 16-bit reads */
#define MRA16_BANK1				((read16_handler)STATIC_BANK1)
#define MRA16_BANK2				((read16_handler)STATIC_BANK2)
#define MRA16_BANK3				((read16_handler)STATIC_BANK3)
#define MRA16_BANK4				((read16_handler)STATIC_BANK4)
#define MRA16_BANK5				((read16_handler)STATIC_BANK5)
#define MRA16_BANK6				((read16_handler)STATIC_BANK6)
#define MRA16_BANK7				((read16_handler)STATIC_BANK7)
#define MRA16_BANK8				((read16_handler)STATIC_BANK8)
#define MRA16_BANK9				((read16_handler)STATIC_BANK9)
#define MRA16_BANK10			((read16_handler)STATIC_BANK10)
#define MRA16_BANK11			((read16_handler)STATIC_BANK11)
#define MRA16_BANK12			((read16_handler)STATIC_BANK12)
#define MRA16_BANK13			((read16_handler)STATIC_BANK13)
#define MRA16_BANK14			((read16_handler)STATIC_BANK14)
#define MRA16_BANK15			((read16_handler)STATIC_BANK15)
#define MRA16_BANK16			((read16_handler)STATIC_BANK16)
#define MRA16_BANK17			((read16_handler)STATIC_BANK17)
#define MRA16_BANK18			((read16_handler)STATIC_BANK18)
#define MRA16_BANK19			((read16_handler)STATIC_BANK19)
#define MRA16_BANK20			((read16_handler)STATIC_BANK20)
#define MRA16_BANK21			((read16_handler)STATIC_BANK21)
#define MRA16_BANK22			((read16_handler)STATIC_BANK22)
#define MRA16_BANK23			((read16_handler)STATIC_BANK23)
#define MRA16_BANK24			((read16_handler)STATIC_BANK24)
#define MRA16_NOP				((read16_handler)STATIC_NOP)
#define MRA16_RAM				((read16_handler)STATIC_RAM)
#define MRA16_ROM				((read16_handler)STATIC_ROM)

/* 16-bit writes */
#define MWA16_BANK1				((write16_handler)STATIC_BANK1)
#define MWA16_BANK2				((write16_handler)STATIC_BANK2)
#define MWA16_BANK3				((write16_handler)STATIC_BANK3)
#define MWA16_BANK4				((write16_handler)STATIC_BANK4)
#define MWA16_BANK5				((write16_handler)STATIC_BANK5)
#define MWA16_BANK6				((write16_handler)STATIC_BANK6)
#define MWA16_BANK7				((write16_handler)STATIC_BANK7)
#define MWA16_BANK8				((write16_handler)STATIC_BANK8)
#define MWA16_BANK9				((write16_handler)STATIC_BANK9)
#define MWA16_BANK10			((write16_handler)STATIC_BANK10)
#define MWA16_BANK11			((write16_handler)STATIC_BANK11)
#define MWA16_BANK12			((write16_handler)STATIC_BANK12)
#define MWA16_BANK13			((write16_handler)STATIC_BANK13)
#define MWA16_BANK14			((write16_handler)STATIC_BANK14)
#define MWA16_BANK15			((write16_handler)STATIC_BANK15)
#define MWA16_BANK16			((write16_handler)STATIC_BANK16)
#define MWA16_BANK17			((write16_handler)STATIC_BANK17)
#define MWA16_BANK18			((write16_handler)STATIC_BANK18)
#define MWA16_BANK19			((write16_handler)STATIC_BANK19)
#define MWA16_BANK20			((write16_handler)STATIC_BANK20)
#define MWA16_BANK21			((write16_handler)STATIC_BANK21)
#define MWA16_BANK22			((write16_handler)STATIC_BANK22)
#define MWA16_BANK23			((write16_handler)STATIC_BANK23)
#define MWA16_BANK24			((write16_handler)STATIC_BANK24)
#define MWA16_NOP				((write16_handler)STATIC_NOP)
#define MWA16_RAM				((write16_handler)STATIC_RAM)
#define MWA16_ROM				((write16_handler)STATIC_ROM)

/* 32-bit reads */
#define MRA32_BANK1				((read32_handler)STATIC_BANK1)
#define MRA32_BANK2				((read32_handler)STATIC_BANK2)
#define MRA32_BANK3				((read32_handler)STATIC_BANK3)
#define MRA32_BANK4				((read32_handler)STATIC_BANK4)
#define MRA32_BANK5				((read32_handler)STATIC_BANK5)
#define MRA32_BANK6				((read32_handler)STATIC_BANK6)
#define MRA32_BANK7				((read32_handler)STATIC_BANK7)
#define MRA32_BANK8				((read32_handler)STATIC_BANK8)
#define MRA32_BANK9				((read32_handler)STATIC_BANK9)
#define MRA32_BANK10			((read32_handler)STATIC_BANK10)
#define MRA32_BANK11			((read32_handler)STATIC_BANK11)
#define MRA32_BANK12			((read32_handler)STATIC_BANK12)
#define MRA32_BANK13			((read32_handler)STATIC_BANK13)
#define MRA32_BANK14			((read32_handler)STATIC_BANK14)
#define MRA32_BANK15			((read32_handler)STATIC_BANK15)
#define MRA32_BANK16			((read32_handler)STATIC_BANK16)
#define MRA32_BANK17			((read32_handler)STATIC_BANK17)
#define MRA32_BANK18			((read32_handler)STATIC_BANK18)
#define MRA32_BANK19			((read32_handler)STATIC_BANK19)
#define MRA32_BANK20			((read32_handler)STATIC_BANK20)
#define MRA32_BANK21			((read32_handler)STATIC_BANK21)
#define MRA32_BANK22			((read32_handler)STATIC_BANK22)
#define MRA32_BANK23			((read32_handler)STATIC_BANK23)
#define MRA32_BANK24			((read32_handler)STATIC_BANK24)
#define MRA32_NOP				((read32_handler)STATIC_NOP)
#define MRA32_RAM				((read32_handler)STATIC_RAM)
#define MRA32_ROM				((read32_handler)STATIC_ROM)

/* 32-bit writes */
#define MWA32_BANK1				((write32_handler)STATIC_BANK1)
#define MWA32_BANK2				((write32_handler)STATIC_BANK2)
#define MWA32_BANK3				((write32_handler)STATIC_BANK3)
#define MWA32_BANK4				((write32_handler)STATIC_BANK4)
#define MWA32_BANK5				((write32_handler)STATIC_BANK5)
#define MWA32_BANK6				((write32_handler)STATIC_BANK6)
#define MWA32_BANK7				((write32_handler)STATIC_BANK7)
#define MWA32_BANK8				((write32_handler)STATIC_BANK8)
#define MWA32_BANK9				((write32_handler)STATIC_BANK9)
#define MWA32_BANK10			((write32_handler)STATIC_BANK10)
#define MWA32_BANK11			((write32_handler)STATIC_BANK11)
#define MWA32_BANK12			((write32_handler)STATIC_BANK12)
#define MWA32_BANK13			((write32_handler)STATIC_BANK13)
#define MWA32_BANK14			((write32_handler)STATIC_BANK14)
#define MWA32_BANK15			((write32_handler)STATIC_BANK15)
#define MWA32_BANK16			((write32_handler)STATIC_BANK16)
#define MWA32_BANK17			((write32_handler)STATIC_BANK17)
#define MWA32_BANK18			((write32_handler)STATIC_BANK18)
#define MWA32_BANK19			((write32_handler)STATIC_BANK19)
#define MWA32_BANK20			((write32_handler)STATIC_BANK20)
#define MWA32_BANK21			((write32_handler)STATIC_BANK21)
#define MWA32_BANK22			((write32_handler)STATIC_BANK22)
#define MWA32_BANK23			((write32_handler)STATIC_BANK23)
#define MWA32_BANK24			((write32_handler)STATIC_BANK24)
#define MWA32_NOP				((write32_handler)STATIC_NOP)
#define MWA32_RAM				((write32_handler)STATIC_RAM)
#define MWA32_ROM				((write32_handler)STATIC_ROM)

/* 64-bit reads */
#define MRA64_BANK1				((read64_handler)STATIC_BANK1)
#define MRA64_BANK2				((read64_handler)STATIC_BANK2)
#define MRA64_BANK3				((read64_handler)STATIC_BANK3)
#define MRA64_BANK4				((read64_handler)STATIC_BANK4)
#define MRA64_BANK5				((read64_handler)STATIC_BANK5)
#define MRA64_BANK6				((read64_handler)STATIC_BANK6)
#define MRA64_BANK7				((read64_handler)STATIC_BANK7)
#define MRA64_BANK8				((read64_handler)STATIC_BANK8)
#define MRA64_BANK9				((read64_handler)STATIC_BANK9)
#define MRA64_BANK10			((read64_handler)STATIC_BANK10)
#define MRA64_BANK11			((read64_handler)STATIC_BANK11)
#define MRA64_BANK12			((read64_handler)STATIC_BANK12)
#define MRA64_BANK13			((read64_handler)STATIC_BANK13)
#define MRA64_BANK14			((read64_handler)STATIC_BANK14)
#define MRA64_BANK15			((read64_handler)STATIC_BANK15)
#define MRA64_BANK16			((read64_handler)STATIC_BANK16)
#define MRA64_BANK17			((read64_handler)STATIC_BANK17)
#define MRA64_BANK18			((read64_handler)STATIC_BANK18)
#define MRA64_BANK19			((read64_handler)STATIC_BANK19)
#define MRA64_BANK20			((read64_handler)STATIC_BANK20)
#define MRA64_BANK21			((read64_handler)STATIC_BANK21)
#define MRA64_BANK22			((read64_handler)STATIC_BANK22)
#define MRA64_BANK23			((read64_handler)STATIC_BANK23)
#define MRA64_BANK24			((read64_handler)STATIC_BANK24)
#define MRA64_NOP				((read64_handler)STATIC_NOP)
#define MRA64_RAM				((read64_handler)STATIC_RAM)
#define MRA64_ROM				((read64_handler)STATIC_ROM)

/* 64-bit writes */
#define MWA64_BANK1				((write64_handler)STATIC_BANK1)
#define MWA64_BANK2				((write64_handler)STATIC_BANK2)
#define MWA64_BANK3				((write64_handler)STATIC_BANK3)
#define MWA64_BANK4				((write64_handler)STATIC_BANK4)
#define MWA64_BANK5				((write64_handler)STATIC_BANK5)
#define MWA64_BANK6				((write64_handler)STATIC_BANK6)
#define MWA64_BANK7				((write64_handler)STATIC_BANK7)
#define MWA64_BANK8				((write64_handler)STATIC_BANK8)
#define MWA64_BANK9				((write64_handler)STATIC_BANK9)
#define MWA64_BANK10			((write64_handler)STATIC_BANK10)
#define MWA64_BANK11			((write64_handler)STATIC_BANK11)
#define MWA64_BANK12			((write64_handler)STATIC_BANK12)
#define MWA64_BANK13			((write64_handler)STATIC_BANK13)
#define MWA64_BANK14			((write64_handler)STATIC_BANK14)
#define MWA64_BANK15			((write64_handler)STATIC_BANK15)
#define MWA64_BANK16			((write64_handler)STATIC_BANK16)
#define MWA64_BANK17			((write64_handler)STATIC_BANK17)
#define MWA64_BANK18			((write64_handler)STATIC_BANK18)
#define MWA64_BANK19			((write64_handler)STATIC_BANK19)
#define MWA64_BANK20			((write64_handler)STATIC_BANK20)
#define MWA64_BANK21			((write64_handler)STATIC_BANK21)
#define MWA64_BANK22			((write64_handler)STATIC_BANK22)
#define MWA64_BANK23			((write64_handler)STATIC_BANK23)
#define MWA64_BANK24			((write64_handler)STATIC_BANK24)
#define MWA64_NOP				((write64_handler)STATIC_NOP)
#define MWA64_RAM				((write64_handler)STATIC_RAM)
#define MWA64_ROM				((write64_handler)STATIC_ROM)



/***************************************************************************

	Address space array type definitions

	Note that the data access handlers are not passed the actual address
	where the operation takes place, but the offset from the beginning
	of the block they are assigned to. This makes handling of mirror
	addresses easier, and makes the handlers a bit more "object oriented".
	If you handler needs to read/write the main memory area, provide a
	"base" pointer: it will be initialized by the main engine to point to
	the beginning of the memory block assigned to the handler. You may
	also provide a pointer to "size": it will be set to the length of
	the memory area processed by the handler.

***************************************************************************/

/* ----- a union of all the different read handler types ----- */
union read_handlers_t
{
	genf *				handler;
	read8_handler		handler8;
	read16_handler		handler16;
	read32_handler		handler32;
	read64_handler		handler64;
};

/* ----- a union of all the different write handler types ----- */
union write_handlers_t
{
	genf *				handler;
	write8_handler		handler8;
	write16_handler		handler16;
	write32_handler		handler32;
	write64_handler		handler64;
};

/* ----- a generic address map type ----- */
struct address_map_t
{
	UINT32				flags;				/* flags and additional info about this entry */
	offs_t				start, end;			/* start/end (or mask/match) values */
	offs_t				mirror;				/* mirror bits */
	offs_t				mask;				/* mask bits */
	union read_handlers_t read;				/* read handler callback */
	union write_handlers_t write;			/* write handler callback */
	void *				memory;				/* pointer to memory backing this entry */
	UINT32				share;				/* index of a shared memory block */
	void **				base;				/* receives pointer to memory (optional) */
	size_t *			size;				/* receives size of area in bytes (optional) */
};

/* ----- structs to contain internal data ----- */
struct address_space_t
{
	offs_t				addrmask;			/* address mask */
	UINT8 *				readlookup;			/* read table lookup */
	UINT8 *				writelookup;		/* write table lookup */
	struct handler_data_t *readhandlers;	/* read handlers */
	struct handler_data_t *writehandlers;	/* write handlers */
	struct data_accessors_t *accessors;		/* pointers to the data access handlers */ 
};



/***************************************************************************

	Address map array constructors

***************************************************************************/

/* ----- a typedef for pointers to these functions ----- */
typedef struct address_map_t *(*construct_map_t)(struct address_map_t *map);

/* use this to declare external references to a machine driver */
#define ADDRESS_MAP_EXTERN(_name)										\
struct address_map_t *construct_map_##_name(struct address_map_t *map)	\

/* ----- macros for starting, ending, and setting map flags ----- */
#define ADDRESS_MAP_START(_name,_space,_bits)							\
struct address_map_t *construct_map_##_name(struct address_map_t *map)	\
{																		\
	typedef read##_bits##_handler _rh_t;								\
	typedef write##_bits##_handler _wh_t;								\
	_rh_t read;															\
	_wh_t write;														\
	data##_bits##_t **base;												\
																		\
	(void)read; (void)write; (void)base;								\
	map->flags = AM_FLAGS_EXTENDED;										\
	map->start = AMEF_DBITS(_bits) | AMEF_SPACE(_space);				\

#define ADDRESS_MAP_FLAGS(_flags)										\
	map++;																\
	map->flags = AM_FLAGS_EXTENDED;										\
	map->start = (_flags);												\

#define ADDRESS_MAP_END													\
	map++;																\
	map->flags = AM_FLAGS_END;											\
	return map;															\
}																		\

/* ----- each map entry begins with one of these ----- */
#define AM_RANGE(_start,_end)											\
	map++;																\
	map->flags = 0;														\
	map->start = (_start);												\
	map->end = (_end);													\

#define AM_SPACE(_match,_mask)											\
	map++;																\
	map->flags = AM_FLAGS_MATCH_MASK;									\
	map->start = (_match);												\
	map->end = (_mask);													\

/* ----- these are optional entries after each map entry ----- */
#define AM_MASK(_mask)													\
	map->mask = (_mask);												\

#define AM_MIRROR(_mirror)												\
	map->mirror = (_mirror);											\

#define AM_READ(_handler)												\
	map->read.handler = (genf *)(read = _handler);						\

#define AM_WRITE(_handler)												\
	map->write.handler = (genf *)(write = _handler);					\

#define AM_REGION(_region, _offs)										\
	map->memory = memory_region(_region) + _offs;						\

#define AM_SHARE(_index)												\
	map->share = _index;												\

#define AM_BASE(_base)													\
	map->base = (void **)(base = _base);								\

#define AM_SIZE(_size)													\
	map->size = _size;													\

/* ----- common shortcuts ----- */
#define AM_READWRITE(_read,_write)			AM_READ(_read) AM_WRITE(_write)
#define AM_ROM								AM_READ((_rh_t)STATIC_ROM)
#define AM_RAM								AM_READWRITE((_rh_t)STATIC_RAM, (_wh_t)STATIC_RAM)
#define AM_ROMBANK(_bank)					AM_READ((_rh_t)(STATIC_BANK1 + (_bank) - 1))
#define AM_RAMBANK(_bank)					AM_READWRITE((_rh_t)(STATIC_BANK1 + (_bank) - 1), (_wh_t)(STATIC_BANK1 + (_bank) - 1))
#define AM_NOP								AM_READWRITE((_rh_t)STATIC_NOP, (_wh_t)STATIC_NOP)
#define AM_READNOP							AM_READ((_rh_t)STATIC_NOP)
#define AM_WRITENOP							AM_WRITE((_wh_t)STATIC_NOP)



/***************************************************************************

	Address map array helper macros

***************************************************************************/

/* ----- macros for identifying address map struct markers ----- */
#define IS_AMENTRY_EXTENDED(ma)				(((ma)->flags & AM_FLAGS_EXTENDED) != 0)
#define IS_AMENTRY_MATCH_MASK(ma)			(((ma)->flags & AM_FLAGS_MATCH_MASK) != 0)
#define IS_AMENTRY_END(ma)					(((ma)->flags & AM_FLAGS_END) != 0)

#define AM_EXTENDED_FLAGS(ma)				((ma)->start)



/***************************************************************************

	Address map lookup constants

	These apply to values in the internal lookup table.

***************************************************************************/

/* ----- address spaces ----- */
#define ADDRESS_SPACES			3						/* maximum number of address spaces */
#define ADDRESS_SPACE_PROGRAM	0						/* program address space */
#define ADDRESS_SPACE_DATA		1						/* data address space */
#define ADDRESS_SPACE_IO		2						/* I/O address space */

/* ----- address map lookup table definitions ----- */
#define SUBTABLE_COUNT			64						/* number of slots reserved for subtables */
#define SUBTABLE_BASE			(256-SUBTABLE_COUNT)	/* first index of a subtable */
#define ENTRY_COUNT				(SUBTABLE_BASE)			/* number of legitimate (non-subtable) entries */
#define SUBTABLE_ALLOC			8						/* number of subtables to allocate at a time */

/* ----- bit counts ----- */
#define SPARSE_THRESH			20						/* number of address bits above which we use sparse memory */
#define LEVEL1_BITS				18						/* number of address bits in the level 1 table */
#define LEVEL2_BITS				(32 - LEVEL1_BITS)		/* number of address bits in the level 2 table */

/* ----- other address map constants ----- */
#define MAX_ADDRESS_MAP_SIZE	256						/* maximum entries in an address map */
#define MAX_MEMORY_BLOCKS		1024					/* maximum memory blocks we can track */
#define MAX_SHARED_POINTERS		256						/* maximum number of shared pointers in memory maps */
#define MEMORY_BLOCK_SIZE		65536					/* size of allocated memory blocks */



/***************************************************************************

	Address map lookup macros

	These are used for accessing the internal lookup table.

***************************************************************************/

/* ----- table lookup helpers ----- */
#define LEVEL1_INDEX(a)			((a) >> LEVEL2_BITS)
#define LEVEL2_INDEX(e,a)		((1 << LEVEL1_BITS) + (((e) - SUBTABLE_BASE) << LEVEL2_BITS) + ((a) & ((1 << LEVEL2_BITS) - 1)))



/***************************************************************************

	Function prototypes for core readmem/writemem routines

***************************************************************************/

/* ----- declare program address space handlers ----- */
data8_t program_read_byte_8(offs_t address);
void program_write_byte_8(offs_t address, data8_t data);

data8_t program_read_byte_16be(offs_t address);
data16_t program_read_word_16be(offs_t address);
void program_write_byte_16be(offs_t address, data8_t data);
void program_write_word_16be(offs_t address, data16_t data);
   
data8_t program_read_byte_16le(offs_t address);
data16_t program_read_word_16le(offs_t address);
void program_write_byte_16le(offs_t address, data8_t data);
void program_write_word_16le(offs_t address, data16_t data);

data8_t program_read_byte_32be(offs_t address);
data16_t program_read_word_32be(offs_t address);
data32_t program_read_dword_32be(offs_t address);
void program_write_byte_32be(offs_t address, data8_t data);
void program_write_word_32be(offs_t address, data16_t data);
void program_write_dword_32be(offs_t address, data32_t data);
   
data8_t program_read_byte_32le(offs_t address);
data16_t program_read_word_32le(offs_t address);
data32_t program_read_dword_32le(offs_t address);
void program_write_byte_32le(offs_t address, data8_t data);
void program_write_word_32le(offs_t address, data16_t data);
void program_write_dword_32le(offs_t address, data32_t data);
   
data8_t program_read_byte_64be(offs_t address);
data16_t program_read_word_64be(offs_t address);
data32_t program_read_dword_64be(offs_t address);
data64_t program_read_qword_64be(offs_t address);
void program_write_byte_64be(offs_t address, data8_t data);
void program_write_word_64be(offs_t address, data16_t data);
void program_write_dword_64be(offs_t address, data32_t data);
void program_write_qword_64be(offs_t address, data64_t data);
   
data8_t program_read_byte_64le(offs_t address);
data16_t program_read_word_64le(offs_t address);
data32_t program_read_dword_64le(offs_t address);
data64_t program_read_qword_64le(offs_t address);
void program_write_byte_64le(offs_t address, data8_t data);
void program_write_word_64le(offs_t address, data16_t data);
void program_write_dword_64le(offs_t address, data32_t data);
void program_write_qword_64le(offs_t address, data64_t data);

/* ----- declare data address space handlers ----- */
data8_t data_read_byte_8(offs_t address);
void data_write_byte_8(offs_t address, data8_t data);

data8_t data_read_byte_16be(offs_t address);
data16_t data_read_word_16be(offs_t address);
void data_write_byte_16be(offs_t address, data8_t data);
void data_write_word_16be(offs_t address, data16_t data);
   
data8_t data_read_byte_16le(offs_t address);
data16_t data_read_word_16le(offs_t address);
void data_write_byte_16le(offs_t address, data8_t data);
void data_write_word_16le(offs_t address, data16_t data);

data8_t data_read_byte_32be(offs_t address);
data16_t data_read_word_32be(offs_t address);
data32_t data_read_dword_32be(offs_t address);
void data_write_byte_32be(offs_t address, data8_t data);
void data_write_word_32be(offs_t address, data16_t data);
void data_write_dword_32be(offs_t address, data32_t data);
   
data8_t data_read_byte_32le(offs_t address);
data16_t data_read_word_32le(offs_t address);
data32_t data_read_dword_32le(offs_t address);
void data_write_byte_32le(offs_t address, data8_t data);
void data_write_word_32le(offs_t address, data16_t data);
void data_write_dword_32le(offs_t address, data32_t data);
   
data8_t data_read_byte_64be(offs_t address);
data16_t data_read_word_64be(offs_t address);
data32_t data_read_dword_64be(offs_t address);
data64_t data_read_qword_64be(offs_t address);
void data_write_byte_64be(offs_t address, data8_t data);
void data_write_word_64be(offs_t address, data16_t data);
void data_write_dword_64be(offs_t address, data32_t data);
void data_write_qword_64be(offs_t address, data64_t data);
   
data8_t data_read_byte_64le(offs_t address);
data16_t data_read_word_64le(offs_t address);
data32_t data_read_dword_64le(offs_t address);
data64_t data_read_qword_64le(offs_t address);
void data_write_byte_64le(offs_t address, data8_t data);
void data_write_word_64le(offs_t address, data16_t data);
void data_write_dword_64le(offs_t address, data32_t data);
void data_write_qword_64le(offs_t address, data64_t data);

/* ----- declare I/O address space handlers ----- */
data8_t io_read_byte_8(offs_t address);
void io_write_byte_8(offs_t address, data8_t data);

data8_t io_read_byte_16be(offs_t address);
data16_t io_read_word_16be(offs_t address);
void io_write_byte_16be(offs_t address, data8_t data);
void io_write_word_16be(offs_t address, data16_t data);
   
data8_t io_read_byte_16le(offs_t address);
data16_t io_read_word_16le(offs_t address);
void io_write_byte_16le(offs_t address, data8_t data);
void io_write_word_16le(offs_t address, data16_t data);

data8_t io_read_byte_32be(offs_t address);
data16_t io_read_word_32be(offs_t address);
data32_t io_read_dword_32be(offs_t address);
void io_write_byte_32be(offs_t address, data8_t data);
void io_write_word_32be(offs_t address, data16_t data);
void io_write_dword_32be(offs_t address, data32_t data);
   
data8_t io_read_byte_32le(offs_t address);
data16_t io_read_word_32le(offs_t address);
data32_t io_read_dword_32le(offs_t address);
void io_write_byte_32le(offs_t address, data8_t data);
void io_write_word_32le(offs_t address, data16_t data);
void io_write_dword_32le(offs_t address, data32_t data);
   
data8_t io_read_byte_64be(offs_t address);
data16_t io_read_word_64be(offs_t address);
data32_t io_read_dword_64be(offs_t address);
data64_t io_read_qword_64be(offs_t address);
void io_write_byte_64be(offs_t address, data8_t data);
void io_write_word_64be(offs_t address, data16_t data);
void io_write_dword_64be(offs_t address, data32_t data);
void io_write_qword_64be(offs_t address, data64_t data);
   
data8_t io_read_byte_64le(offs_t address);
data16_t io_read_word_64le(offs_t address);
data32_t io_read_dword_64le(offs_t address);
data64_t io_read_qword_64le(offs_t address);
void io_write_byte_64le(offs_t address, data8_t data);
void io_write_word_64le(offs_t address, data16_t data);
void io_write_dword_64le(offs_t address, data32_t data);
void io_write_qword_64le(offs_t address, data64_t data);



/***************************************************************************

	Function prototypes for core memory functions

***************************************************************************/

/* ----- memory setup function ----- */
int			memory_init(void);
void		memory_shutdown(void);
void		memory_set_context(int activecpu);

/* ----- address map functions ----- */
const struct address_map_t *memory_get_map(int cpunum, int spacenum);

/* ----- opcode base control ---- */
opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function);
void		memory_set_opbase(offs_t offset);

/* ----- separate opcode/data encryption helpers ---- */
void		memory_set_opcode_base(int cpunum, void *base);

/* ----- return a base pointer to memory ---- */
void *		memory_get_read_ptr(int cpunum, int spacenum, offs_t offset);
void *		memory_get_write_ptr(int cpunum, int spacenum, offs_t offset);

/* ----- memory banking ----- */
void		memory_set_bankptr(int banknum, void *base);

/* ----- dynamic address space mapping ----- */
void		memory_set_debugger_access(int debugger);

/* ----- dynamic address space mapping ----- */
data8_t *	memory_install_read8_handler  (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_handler handler);
data16_t *	memory_install_read16_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read16_handler handler);
data32_t *	memory_install_read32_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read32_handler handler);
data64_t *	memory_install_read64_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read64_handler handler);
data8_t *	memory_install_write8_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write8_handler handler);
data16_t *	memory_install_write16_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write16_handler handler);
data32_t *	memory_install_write32_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write32_handler handler);
data64_t *	memory_install_write64_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write64_handler handler);

data8_t *	memory_install_read8_matchmask_handler  (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read8_handler handler);
data16_t *	memory_install_read16_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read16_handler handler);
data32_t *	memory_install_read32_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read32_handler handler);
data64_t *	memory_install_read64_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read64_handler handler);
data8_t *	memory_install_write8_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write8_handler handler);
data16_t *	memory_install_write16_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write16_handler handler);
data32_t *	memory_install_write32_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write32_handler handler);
data64_t *	memory_install_write64_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write64_handler handler);


/***************************************************************************

	Global variables

***************************************************************************/

extern UINT8 			opcode_entry;				/* current entry for opcode fetching */
extern UINT8 *			opcode_base;				/* opcode ROM base */
extern UINT8 *			opcode_arg_base;			/* opcode RAM base */
extern offs_t			opcode_mask;				/* mask to apply to the opcode address */
extern offs_t			opcode_memory_min;			/* opcode memory minimum */
extern offs_t			opcode_memory_max;			/* opcode memory maximum */
extern struct address_space_t address_space[];		/* address spaces */
extern struct address_map_t *construct_map_0(struct address_map_t *map);



/***************************************************************************

	Helper macros and inlines

***************************************************************************/

/* ----- 16/32-bit memory accessing ----- */
#define COMBINE_DATA(varptr)		(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))

/* ----- 16-bit memory accessing ----- */
#define ACCESSING_LSB16				((mem_mask & 0x00ff) == 0)
#define ACCESSING_MSB16				((mem_mask & 0xff00) == 0)
#define ACCESSING_LSB				ACCESSING_LSB16
#define ACCESSING_MSB				ACCESSING_MSB16

/* ----- 32-bit memory accessing ----- */
#define ACCESSING_LSW32				((mem_mask & 0x0000ffff) == 0)
#define ACCESSING_MSW32				((mem_mask & 0xffff0000) == 0)
#define ACCESSING_LSB32				((mem_mask & 0x000000ff) == 0)
#define ACCESSING_MSB32				((mem_mask & 0xff000000) == 0)

/* ----- opcode range safety checks ----- */
#if CPUREADOP_SAFETY_NONE
#define address_is_unsafe(A)		(0)
#elif CPUREADOP_SAFETY_PARTIAL
#define address_is_unsafe(A)		(UNEXPECTED((A) > opcode_memory_max))
#elif CPUREADOP_SAFETY_FULL
#define address_is_unsafe(A)		((UNEXPECTED((A) < opcode_memory_min) || UNEXPECTED((A) > opcode_memory_max)))
#else
#error Must set either CPUREADOP_SAFETY_NONE, CPUREADOP_SAFETY_PARTIAL or CPUREADOP_SAFETY_FULL
#endif

/* ----- generic memory access ----- */
INLINE data8_t  program_read_byte (offs_t offset) { return (*address_space[ADDRESS_SPACE_PROGRAM].accessors->read_byte)(offset); }
INLINE data16_t program_read_word (offs_t offset) { return (*address_space[ADDRESS_SPACE_PROGRAM].accessors->read_word)(offset); }
INLINE data32_t program_read_dword(offs_t offset) { return (*address_space[ADDRESS_SPACE_PROGRAM].accessors->read_dword)(offset); }
INLINE data64_t program_read_qword(offs_t offset) { return (*address_space[ADDRESS_SPACE_PROGRAM].accessors->read_qword)(offset); }

INLINE void	program_write_byte (offs_t offset, data8_t  data) { (*address_space[ADDRESS_SPACE_PROGRAM].accessors->write_byte)(offset, data); }
INLINE void	program_write_word (offs_t offset, data16_t data) { (*address_space[ADDRESS_SPACE_PROGRAM].accessors->write_word)(offset, data); }
INLINE void	program_write_dword(offs_t offset, data32_t data) { (*address_space[ADDRESS_SPACE_PROGRAM].accessors->write_dword)(offset, data); }
INLINE void	program_write_qword(offs_t offset, data64_t data) { (*address_space[ADDRESS_SPACE_PROGRAM].accessors->write_qword)(offset, data); }

INLINE data8_t  data_read_byte (offs_t offset) { return (*address_space[ADDRESS_SPACE_DATA].accessors->read_byte)(offset); }
INLINE data16_t data_read_word (offs_t offset) { return (*address_space[ADDRESS_SPACE_DATA].accessors->read_word)(offset); }
INLINE data32_t data_read_dword(offs_t offset) { return (*address_space[ADDRESS_SPACE_DATA].accessors->read_dword)(offset); }
INLINE data64_t data_read_qword(offs_t offset) { return (*address_space[ADDRESS_SPACE_DATA].accessors->read_qword)(offset); }

INLINE void	data_write_byte (offs_t offset, data8_t  data) { (*address_space[ADDRESS_SPACE_DATA].accessors->write_byte)(offset, data); }
INLINE void	data_write_word (offs_t offset, data16_t data) { (*address_space[ADDRESS_SPACE_DATA].accessors->write_word)(offset, data); }
INLINE void	data_write_dword(offs_t offset, data32_t data) { (*address_space[ADDRESS_SPACE_DATA].accessors->write_dword)(offset, data); }
INLINE void	data_write_qword(offs_t offset, data64_t data) { (*address_space[ADDRESS_SPACE_DATA].accessors->write_qword)(offset, data); }

INLINE data8_t  io_read_byte (offs_t offset) { return (*address_space[ADDRESS_SPACE_IO].accessors->read_byte)(offset); }
INLINE data16_t io_read_word (offs_t offset) { return (*address_space[ADDRESS_SPACE_IO].accessors->read_word)(offset); }
INLINE data32_t io_read_dword(offs_t offset) { return (*address_space[ADDRESS_SPACE_IO].accessors->read_dword)(offset); }
INLINE data64_t io_read_qword(offs_t offset) { return (*address_space[ADDRESS_SPACE_IO].accessors->read_qword)(offset); }

INLINE void	io_write_byte (offs_t offset, data8_t  data) { (*address_space[ADDRESS_SPACE_IO].accessors->write_byte)(offset, data); }
INLINE void	io_write_word (offs_t offset, data16_t data) { (*address_space[ADDRESS_SPACE_IO].accessors->write_word)(offset, data); }
INLINE void	io_write_dword(offs_t offset, data32_t data) { (*address_space[ADDRESS_SPACE_IO].accessors->write_dword)(offset, data); }
INLINE void	io_write_qword(offs_t offset, data64_t data) { (*address_space[ADDRESS_SPACE_IO].accessors->write_qword)(offset, data); }

/* ----- safe opcode and opcode argument reading ----- */
data8_t		cpu_readop_safe(offs_t offset);
data16_t	cpu_readop16_safe(offs_t offset);
data32_t	cpu_readop32_safe(offs_t offset);
data64_t	cpu_readop64_safe(offs_t offset);
data8_t		cpu_readop_arg_safe(offs_t offset);
data16_t	cpu_readop_arg16_safe(offs_t offset);
data32_t	cpu_readop_arg32_safe(offs_t offset);
data64_t	cpu_readop_arg64_safe(offs_t offset);

/* ----- unsafe opcode and opcode argument reading ----- */
#define cpu_opptr_unsafe(A)			((void *)&opcode_base[(A) & opcode_mask])
#define cpu_readop_unsafe(A)		(opcode_base[(A) & opcode_mask])
#define cpu_readop16_unsafe(A)		(*(data16_t *)&opcode_base[(A) & opcode_mask])
#define cpu_readop32_unsafe(A)		(*(data32_t *)&opcode_base[(A) & opcode_mask])
#define cpu_readop64_unsafe(A)		(*(data64_t *)&opcode_base[(A) & opcode_mask])
#define cpu_readop_arg_unsafe(A)	(opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg16_unsafe(A)	(*(data16_t *)&opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg32_unsafe(A)	(*(data32_t *)&opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg64_unsafe(A)	(*(data64_t *)&opcode_arg_base[(A) & opcode_mask])

/* ----- opcode and opcode argument reading ----- */
INLINE void *   cpu_opptr(offs_t A)			{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_opptr_unsafe(A); }
INLINE data8_t  cpu_readop(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_unsafe(A); }
INLINE data16_t cpu_readop16(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop16_unsafe(A); }
INLINE data32_t cpu_readop32(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop32_unsafe(A); }
INLINE data64_t cpu_readop64(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop64_unsafe(A); }
INLINE data8_t  cpu_readop_arg(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg_unsafe(A); }
INLINE data16_t cpu_readop_arg16(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg16_unsafe(A); }
INLINE data32_t cpu_readop_arg32(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg32_unsafe(A); }
INLINE data64_t cpu_readop_arg64(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg64_unsafe(A); }

/* ----- bank switching for CPU cores ----- */
#define change_pc(pc)																	\
do {																					\
	if (address_space[ADDRESS_SPACE_PROGRAM].readlookup[LEVEL1_INDEX((pc) & address_space[ADDRESS_SPACE_PROGRAM].addrmask)] != opcode_entry)	\
		memory_set_opbase(pc);															\
} while (0)																				\

/* ----- forces the next branch to generate a call to the opbase handler ----- */
#define catch_nextBranch()			(opcode_entry = 0xff)

/* ----- bank switching macro ----- */
#define cpu_setbank(bank, base) memory_set_bankptr(bank, base)


#ifdef __cplusplus
}
#endif

#endif	/* !_MEMORY_H */

