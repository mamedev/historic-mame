/*** TMS34010: Portable TMS34010 emulator ************************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998

    Field Handling

*****************************************************************************/

#include <stdio.h>
#include "osd_dbg.h"
#include "osd_cpu.h"
#include "tms34010.h"
#include "34010ops.h"
#include "driver.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif

#define WFIELDMAC1(MASK,MAX) 														\
	UINT32 shift = bitaddr&0x0f;     												\
	UINT32 old;				   														\
	bitaddr = (bitaddr&0xfffffff0)>>3; 												\
																					\
	if (shift >= MAX)																\
	{																				\
		old = ((UINT32) TMS34010_RDMEM_DWORD(bitaddr)&~((MASK)<<shift)); 			\
		TMS34010_WRMEM_DWORD(bitaddr,((data&(MASK))<<shift)|old);					\
	}																				\
	else																			\
	{																				\
		old = ((UINT32) TMS34010_RDMEM_WORD (bitaddr)&~((MASK)<<shift)); 			\
		TMS34010_WRMEM_WORD (bitaddr,((data&(MASK))<<shift)|old);			   		\
	}

void WFIELD_01(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x01,16);
}
void WFIELD_02(UINT32 bitaddr, UINT32 data) /* this can easily be sped up */
{
	WFIELDMAC1(0x03,15);
}
void WFIELD_03(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x07,14);
}
void WFIELD_04(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x0f,13);
}
void WFIELD_05(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x1f,12);
}
void WFIELD_06(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x3f,11);
}
void WFIELD_07(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x7f,10);
}
void WFIELD_08(UINT32 bitaddr, UINT32 data)
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
void WFIELD_09(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x1ff,8);
}
void WFIELD_10(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x3ff,7);
}
void WFIELD_11(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x7ff,6);
}
void WFIELD_12(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0xfff,5);
}
void WFIELD_13(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x1fff,4);
}
void WFIELD_14(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x3fff,3);
}
void WFIELD_15(UINT32 bitaddr, UINT32 data)
{
	WFIELDMAC1(0x7fff,2);
}
void WFIELD_16(UINT32 bitaddr, UINT32 data)
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
void WFIELD_17(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"17-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_18(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"18-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_19(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"19-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_20(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"20-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_21(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"21-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_22(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"22-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_23(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"23-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_24(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"24-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_25(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"25-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_26(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"26-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_27(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"27-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_28(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"28-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_29(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"29-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_30(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"30-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_31(UINT32 bitaddr, UINT32 data)
{
	if (errorlog) fprintf(errorlog,"31-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
}
void WFIELD_32(UINT32 bitaddr, UINT32 data)
{
	if (bitaddr&0x0f)
	{
		UINT32 shift = bitaddr&0x0f;
		UINT32 old;
		UINT32 hiword;
		bitaddr &= 0xfffffff0;
		old =    ((UINT32) TMS34010_RDMEM_DWORD (bitaddr>>3)    &(0xffffffff>>(0x20-shift)));
		hiword = ((UINT32) TMS34010_RDMEM_DWORD((bitaddr+32)>>3)&(0xffffffff<<shift));
		TMS34010_WRMEM_DWORD( bitaddr    >>3,(data<<      shift) |old);
		TMS34010_WRMEM_DWORD((bitaddr+32)>>3,(data>>(0x20-shift))|hiword);
	}
	else
	{
		TMS34010_WRMEM_DWORD(bitaddr>>3,data);
	}
}



#define RFIELDMAC1(MASK,MAX)									\
	UINT32 shift = bitaddr&0x0f;							    \
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

int RFIELD_01(UINT32 bitaddr)
{
	RFIELDMAC1(0x01,16);
}
int RFIELD_02(UINT32 bitaddr)
{
	RFIELDMAC1(0x03,15);
}
int RFIELD_03(UINT32 bitaddr)
{
	RFIELDMAC1(0x07,14);
}
int RFIELD_04(UINT32 bitaddr)
{
	RFIELDMAC1(0x0f,13);
}
int RFIELD_05(UINT32 bitaddr)
{
	RFIELDMAC1(0x1f,12);
}
int RFIELD_06(UINT32 bitaddr)
{
	RFIELDMAC1(0x3f,11);
}
int RFIELD_07(UINT32 bitaddr)
{
	RFIELDMAC1(0x7f,10);
}
int RFIELD_08(UINT32 bitaddr)
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
int RFIELD_09(UINT32 bitaddr)
{
	RFIELDMAC1(0x1ff,8);
}
int RFIELD_10(UINT32 bitaddr)
{
	RFIELDMAC1(0x3ff,7);
}
int RFIELD_11(UINT32 bitaddr)
{
	RFIELDMAC1(0x7ff,6);
}
int RFIELD_12(UINT32 bitaddr)
{
	RFIELDMAC1(0xfff,5);
}
int RFIELD_13(UINT32 bitaddr)
{
	RFIELDMAC1(0x1fff,4);
}
int RFIELD_14(UINT32 bitaddr)
{
	RFIELDMAC1(0x3fff,3);
}
int RFIELD_15(UINT32 bitaddr)
{
	RFIELDMAC1(0x7fff,2);
}
int RFIELD_16(UINT32 bitaddr)
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
int RFIELD_17(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"17-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_18(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"18-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_19(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"19-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_20(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"20-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_21(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"21-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_22(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"22-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_23(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"23-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_24(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"24-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_25(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"25-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_26(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"26-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_27(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"27-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_28(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"28-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_29(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"29-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_30(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"30-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_31(UINT32 bitaddr)
{
	if (errorlog) fprintf(errorlog,"31-bit fields are not implemented!\n");
#ifdef MAME_DEBUG
	debug_key_pressed=1;
#endif
	return 0;
}
int RFIELD_32(UINT32 bitaddr)
{
	if (bitaddr&0x0f)
	{
		UINT32 shift = bitaddr&0x0f;
		bitaddr &= 0xfffffff0;
		return (((UINT32)TMS34010_RDMEM_DWORD  (bitaddr    >>3))>>      shift) |
			            (TMS34010_RDMEM_DWORD ((bitaddr+32)>>3) <<(0x20-shift));
	}
	else
	{
		return TMS34010_RDMEM_DWORD(bitaddr>>3);
	}
}
