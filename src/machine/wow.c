/* Bally interrupt system's */

#include "driver.h"
#include "z80.h"

extern int interrupt(void);
extern int Z80_IRQ;

/* Standard machine stuff */

void colour_register_w(int offset, int data)
{
	/* Colour registers are not implemented yet */
}

void paging_register_w(int offset, int data)
{
	/* Don't know what this does - ignored at the moment */
}


/*
 * Scanline Interrupt System **
 */

int NextScanInt=0;
int CurrentScan=0;
int InterruptFlag=0;
int Controller1=32;
int GorfDelay;

void wow_interrupt_enable_w(int offset, int data)
{
	InterruptFlag = data;

    if (data == 0) Z80_IRQ=Z80_IGNORE_INT;

    if (data & 0x10)
	{
		GorfDelay =(CurrentScan + 10) & 0xFF;

    	if (errorlog) fprintf(errorlog,"Gorf Delay set to %02x\n",GorfDelay);
    }

	if (errorlog) fprintf(errorlog,"Interrupt flag set to %02x\n",data);
}

void wow_interrupt_w(int offset, int data)
{
	/* A write to 0F triggers an interrupt at that scanline */

	if (errorlog) fprintf(errorlog,"Scanline interrupt set to %02x\n",data);

    NextScanInt = data;
}

int wow_interrupt(void)
{
	int res=Z80_IGNORE_INT;

    CurrentScan++;

    if (CurrentScan == 230)
	{
		CurrentScan = 0;

    	/*
		 * Seawolf2 needs to emulate a rotary port
         *
         * Checked each flyback, takes 1 second to traverse screen
         */

        if ((osd_key_pressed(OSD_KEY_RIGHT)) && (Controller1 > 0))
			Controller1--;

		if ((osd_key_pressed(OSD_KEY_LEFT)) && (Controller1 < 63))
			Controller1++;
    }

    /* Scanline interrupt enabled ? */

    if ((InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
    {
		res = interrupt();
    }

    return res;
}

int gorf_interrupt(void)
{
	int res=Z80_IGNORE_INT;

    CurrentScan++;

    if (CurrentScan == 256)
	{
		CurrentScan=0;
    }

    /* Scanline interrupt enabled ? */

    if ((InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
    {
		res = interrupt();
    }

    /* Gorf Unknown Interrupts */

    if ((InterruptFlag & 0x10) && (CurrentScan==GorfDelay))
	{
   	    res = interrupt() & 0xF0;
	}

    return res;
}

/*
 * Seawolf2 uses rotary controllers on input ports 10 + 11
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button. Controller values are calculated in the
 * interrupt routine, and just formatted & returned here.
 *
 * I only emulate one controller at the moment!
 *
 * The controller looks like it returns Grays binary,
 * so I use a table to translate my simple counter into it!
 */

static const int ControllerTable[64] = {
    0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
    12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
    24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
    20 , 21 , 23 , 22 , 18 , 19 , 17 , 16 ,
    48 , 49 , 51 , 50 , 54 , 55 , 53 , 52 ,
    60 , 61 , 63 , 62 , 58 , 59 , 57 , 56 ,
    40 , 41 , 43 , 42 , 46 , 47 , 45 , 44 ,
    36 , 37 , 39 , 38 , 34 , 35 , 33 , 32
};

int seawolf2_controller1_r(int offset)
{
    return input_port_0_r(0) + ControllerTable[Controller1];
}
