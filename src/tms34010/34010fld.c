/*** TMS34010: Portable TMS34010 emulator ************************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998

    Field Handling

*****************************************************************************/

#include <stdio.h>
#include "osd_dbg.h"
#include "tms34010.h"
#include "driver.h"

#define WFIELDMAC1(m) 													\
	unsigned int shift = bitaddr&0x0f;     								\
	unsigned int loword;				   								\
	unsigned int mask = (m)<<shift;		   								\
	bitaddr &= 0xfffffff0;				   								\
	loword = ((unsigned int) TMS34010_RDMEM_DWORD(bitaddr>>3)&(~mask)); \
	data &= (m);								                        \
	TMS34010_WRMEM_DWORD(bitaddr>>3,(data<<shift)|loword)

void WFIELD_01(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x01);
}
void WFIELD_02(unsigned int bitaddr, unsigned int data) /* this can easily be sped up */
{
	WFIELDMAC1(0x03);
}
void WFIELD_03(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x07);
}
void WFIELD_04(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x0f);
}
void WFIELD_05(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1f);
}
void WFIELD_06(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3f);
}
void WFIELD_07(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7f);
}
void WFIELD_08(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x07)
	{
		WFIELDMAC1(0xff);
	}
	else
	{
		TMS34010_WRMEM(bitaddr>>3,data);
	}
}
void WFIELD_09(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1ff);
}
void WFIELD_10(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3ff);
}
void WFIELD_11(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7ff);
}
void WFIELD_12(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0xfff);
}
void WFIELD_13(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x1fff);
}
void WFIELD_14(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x3fff);
}
void WFIELD_15(unsigned int bitaddr, unsigned int data)
{
	WFIELDMAC1(0x7fff);
}
void WFIELD_16(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		unsigned int loword;
		unsigned int hiword;
		bitaddr &= 0xfffffff0;
		loword = ((unsigned int) TMS34010_RDMEM_WORD(bitaddr>>3)&(0xffff>>(0x10-shift)));
		hiword = ((unsigned int) TMS34010_RDMEM_WORD((bitaddr+16)>>3)&(0xffff<<shift));
		data &= 0xffff;
		TMS34010_WRMEM_WORD( bitaddr>>3,   ((data<<shift)|loword)&0xffff);
		TMS34010_WRMEM_WORD((bitaddr+16)>>3,(data>>(0x10-shift))|hiword);
//				if (errorlog) fprintf(errorlog,"l %04x,h %04x,l %04x,h %04x\n",loword,hiword,((((unsigned int) data)<<shift)|loword)&0xffff,(((unsigned int) data)>>(0x10-shift))|hiword);
	}
	else
	{
		TMS34010_WRMEM_WORD(bitaddr>>3,data);
	}
}
void WFIELD_17(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_18(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_19(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_20(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_21(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_22(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_23(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_24(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_25(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_26(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_27(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_28(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_29(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_30(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_31(unsigned int bitaddr, unsigned int data)
{
}
void WFIELD_32(unsigned int bitaddr, unsigned int data)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		unsigned int loword;
		unsigned int hiword;
		bitaddr &= 0xfffffff0;
		loword = ((unsigned int) TMS34010_RDMEM_DWORD(bitaddr>>3)&(0xffffffff>>(0x20-shift)));
		hiword = ((unsigned int) TMS34010_RDMEM_DWORD((bitaddr+32)>>3)&(0xffffffff<<shift));
		TMS34010_WRMEM_DWORD( bitaddr>>3   ,(data<<shift)|loword);
		TMS34010_WRMEM_DWORD((bitaddr+32)>>3,(data>>(0x20-shift))|hiword);
//				if (errorlog) fprintf(errorlog,"l %08x,h %08x,l %08x,h %08x\n",loword,hiword,(((unsigned int) data)<<shift)|loword,(((unsigned int) data)>>(0x20-shift))|hiword);
	}
	else
	{
		TMS34010_WRMEM_DWORD(bitaddr>>3,data);
	}
}



#define RFIELDMAC1(m)											\
	unsigned int shift = bitaddr&0x0f;						\
	bitaddr &= 0xfffffff0;									\
	return ((TMS34010_RDMEM_DWORD(bitaddr>>3)>>shift)&(m))

int RFIELD_01(unsigned int bitaddr)
{
	RFIELDMAC1(0x01);
}
int RFIELD_02(unsigned int bitaddr)
{
	RFIELDMAC1(0x03);
}
int RFIELD_03(unsigned int bitaddr)
{
	RFIELDMAC1(0x07);
}
int RFIELD_04(unsigned int bitaddr)
{
	RFIELDMAC1(0x0f);
}
int RFIELD_05(unsigned int bitaddr)
{
	RFIELDMAC1(0x1f);
}
int RFIELD_06(unsigned int bitaddr)
{
	RFIELDMAC1(0x3f);
}
int RFIELD_07(unsigned int bitaddr)
{
	RFIELDMAC1(0x7f);
}
int RFIELD_08(unsigned int bitaddr)
{
	if (bitaddr&0x07)
	{
		RFIELDMAC1(0xff);
	}
	else
	{
		return TMS34010_RDMEM(bitaddr>>3);
	}
}
int RFIELD_09(unsigned int bitaddr)
{
	RFIELDMAC1(0x1ff);
}
int RFIELD_10(unsigned int bitaddr)
{
	RFIELDMAC1(0x3ff);
}
int RFIELD_11(unsigned int bitaddr)
{
	RFIELDMAC1(0x7ff);
}
int RFIELD_12(unsigned int bitaddr)
{
	RFIELDMAC1(0xfff);
}
int RFIELD_13(unsigned int bitaddr)
{
	RFIELDMAC1(0x1fff);
}
int RFIELD_14(unsigned int bitaddr)
{
	RFIELDMAC1(0x3fff);
}
int RFIELD_15(unsigned int bitaddr)
{
	RFIELDMAC1(0x7fff);
}
int RFIELD_16(unsigned int bitaddr)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		bitaddr &= 0xfffffff0;
		return ((TMS34010_RDMEM_WORD (bitaddr>>3)   >>      shift) |
			    (TMS34010_RDMEM_WORD ((bitaddr+16)>>3)<<(0x10-shift)))&0xffff;
	}
	else
	{
		return TMS34010_RDMEM_WORD(bitaddr>>3);
	}
}
int RFIELD_17(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_18(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_19(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_20(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_21(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_22(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_23(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_24(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_25(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_26(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_27(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_28(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_29(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_30(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_31(unsigned int bitaddr)
{
	return 0;
}
int RFIELD_32(unsigned int bitaddr)
{
	if (bitaddr&0x0f)
	{
		unsigned int shift = bitaddr&0x0f;
		bitaddr &= 0xfffffff0;
		return (((unsigned int)TMS34010_RDMEM_DWORD (bitaddr>>3))  >>      shift) |
			                  (TMS34010_RDMEM_DWORD ((bitaddr+32)>>3)<<(0x20-shift));
	}
	else
	{
		return TMS34010_RDMEM_DWORD(bitaddr>>3);
	}
}
