/*** TMS34010: Portable TMS34010 emulator ************************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998

    Field Handling

*****************************************************************************/

#include <stdio.h>
#include "osd_dbg.h"
#include "tms34010.h"
#include "34010ops.h"
#include "driver.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif

#define WFIELDMAC1(MASK,MAX) 														\
	unsigned int shift = bitaddr&0x0f;     											\
	unsigned int old;				   												\
	bitaddr = (bitaddr&0xfffffff0)>>3; 												\
																					\
	if (shift >= MAX)																\
	{																				\
		old = ((unsigned int) TMS34010_RDMEM_DWORD(bitaddr)&~((MASK)<<shift)); 		\
		TMS34010_WRMEM_DWORD(bitaddr,((data&(MASK))<<shift)|old);					\
	}																				\
	else																			\
	{																				\
		old = ((unsigned int) TMS34010_RDMEM_WORD (bitaddr)&~((MASK)<<shift)); 		\
		TMS34010_WRMEM_WORD (bitaddr,((data&(MASK))<<shift)|old);			   		\
	}

void WFIELD_01(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x01,16);
}
void WFIELD_02(unsigned int bitaddr, unsigned int data) /* this can easily be sped up */
{
	WFIELDMAC1(0x03,15);
}
void WFIELD_03(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x07,14);
}
void WFIELD_04(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x0f,13);
}
void WFIELD_05(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1f,12);
}
void WFIELD_06(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3f,11);
}
void WFIELD_07(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7f,10);
}
void WFIELD_08(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x07)
	{
		WFIELDMAC1(0xff,9);
	}
	else
	{
		TMS34010_WRMEM(bitaddr>>3,data);
	}
}
void WFIELD_09(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1ff,8);
}
void WFIELD_10(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3ff,7);
}
void WFIELD_11(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7ff,6);
}
void WFIELD_12(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0xfff,5);
}
void WFIELD_13(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1fff,4);
}
void WFIELD_14(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3fff,3);
}
void WFIELD_15(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7fff,2);
}
void WFIELD_16(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x0f)
	{
		WFIELDMAC1(0xffff,1);
	}
	else
	{
		TMS34010_WRMEM_WORD(bitaddr>>3,data);
	}
}
void WFIELD_17(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"17-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_18(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"18-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_19(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"19-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_20(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"20-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_21(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"21-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_22(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"22-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_23(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"23-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_24(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"24-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_25(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"25-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_26(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"26-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_27(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"27-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_28(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"28-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_29(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"29-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_30(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"30-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_31(unsigned int bitaddr, unsigned int data)
{
	if (errorlog) fprintf(errorlog,"31-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_32(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		unsigned int old;
		unsigned int hiword;
		bitaddr &= 0xfffffff0;
		old = ((unsigned int) TMS34010_RDMEM_DWORD(bitaddr>>3)&(0xffffffff>>(0x20-shift)));
		hiword = ((unsigned int) TMS34010_RDMEM_DWORD((bitaddr+32)>>3)&(0xffffffff<<shift));
		TMS34010_WRMEM_DWORD( bitaddr>>3   ,(data<<shift)|old);
		TMS34010_WRMEM_DWORD((bitaddr+32)>>3,(data>>(0x20-shift))|hiword);
	}
	else
	{
		TMS34010_WRMEM_DWORD(bitaddr>>3,data);
	}
}



#define RFIELDMAC1(MASK,MAX)									\
	unsigned int shift = bitaddr&0x0f;							\
	bitaddr = (bitaddr&0xfffffff0)>>3; 							\
																\
	if (shift >= MAX)											\
	{															\
		return ((TMS34010_RDMEM_DWORD(bitaddr)>>shift)&(MASK));	\
	}															\
	else														\
	{															\
		return ((TMS34010_RDMEM_WORD (bitaddr)>>shift)&(MASK));	\
	}

int RFIELD_01(unsigned int bitaddr)
{
	RFIELDMAC1(0x01,16);
}
int RFIELD_02(unsigned int bitaddr)
{
	RFIELDMAC1(0x03,15);
}
int RFIELD_03(unsigned int bitaddr)
{
	RFIELDMAC1(0x07,14);
}
int RFIELD_04(unsigned int bitaddr)
{
	RFIELDMAC1(0x0f,13);
}
int RFIELD_05(unsigned int bitaddr)
{
	RFIELDMAC1(0x1f,12);
}
int RFIELD_06(unsigned int bitaddr)
{
	RFIELDMAC1(0x3f,11);
}
int RFIELD_07(unsigned int bitaddr)
{
	RFIELDMAC1(0x7f,10);
}
int RFIELD_08(unsigned int bitaddr)
{
	if (bitaddr&0x07)
	{
		RFIELDMAC1(0xff,9);
	}
	else
	{
		return TMS34010_RDMEM(bitaddr>>3);
	}
}
int RFIELD_09(unsigned int bitaddr)
{
	RFIELDMAC1(0x1ff,8);
}
int RFIELD_10(unsigned int bitaddr)
{
	RFIELDMAC1(0x3ff,7);
}
int RFIELD_11(unsigned int bitaddr)
{
	RFIELDMAC1(0x7ff,6);
}
int RFIELD_12(unsigned int bitaddr)
{
	RFIELDMAC1(0xfff,5);
}
int RFIELD_13(unsigned int bitaddr)
{
	RFIELDMAC1(0x1fff,4);
}
int RFIELD_14(unsigned int bitaddr)
{
	RFIELDMAC1(0x3fff,3);
}
int RFIELD_15(unsigned int bitaddr)
{
	RFIELDMAC1(0x7fff,2);
}
int RFIELD_16(unsigned int bitaddr)
{
	if (bitaddr&0x0f)
	{
		RFIELDMAC1(0xffff,1);
	}
	else
	{
		return TMS34010_RDMEM_WORD(bitaddr>>3);
	}
}
int RFIELD_17(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"17-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_18(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"18-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_19(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"19-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_20(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"20-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_21(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"21-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_22(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"22-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_23(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"23-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_24(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"24-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_25(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"25-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_26(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"26-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_27(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"27-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_28(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"28-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_29(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"29-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_30(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"30-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_31(unsigned int bitaddr)
{
	if (errorlog) fprintf(errorlog,"31-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_32(unsigned int bitaddr)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		bitaddr &= 0xfffffff0;
		return (((unsigned int)TMS34010_RDMEM_DWORD  (bitaddr>>3))   >>      shift) |
			                  (TMS34010_RDMEM_DWORD ((bitaddr+32)>>3)<<(0x20-shift));
	}
	else
	{
		return TMS34010_RDMEM_DWORD(bitaddr>>3);
	}
}
