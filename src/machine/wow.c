/* Bally interrupt system's */

#include "driver.h"
#include "z80.h"

extern int interrupt(void);

/* Standard machine stuff */

void colour_register_w(int offset, int data)
{
	/* Colour registers are not implemented yet */
}

void paging_register_w(int offset, int data)
{
	/* Don't know what this does - ignored at the moment */
}

void scanline_interrupt_w(int offset, int data)
{
	/* A write to 0F triggers an interrupt at that scanline */

	if (errorlog) fprintf(errorlog,"Scanline interrupt set to %02x\n",data);
}


/* Gorf uses IM 0 and IM 2 */

int Gorf_Interrupt  = 0;

void gorf_interrupt_w(int offset, int data)
{
	Gorf_Interrupt = (data & 0x10);
}

int gorf_interrupt(void)
{
	int res;
	Z80_Regs regs;

    /* Get normal answer */
    res = interrupt();

    /* if interrupt mode 0, override with CALL 00f0 */
    Z80_GetRegs(&regs);
	if ((regs.IM==0) && (res!=Z80_IGNORE_INT))
		if (Gorf_Interrupt==0x10) res = 0xCD00F0;

  	return res;
}
