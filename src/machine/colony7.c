/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"

extern unsigned char *defender_bank_base;
extern unsigned char *defender_bank_ram;
extern int defender_io_r(int offset);

void williams_sh_w(int offset,int data);
void williams_palette_w(int offset,int data);

void colony7_io_w (int offset,int data);


void colony7_bank_select_w (int offset,int data); /* MB 112497 */


void colony7_init_machine (void)
{
	/* set the optimization flags */
	m6809_Flags = M6809_FAST_NONE;

	/* initialize the banks */
	colony7_bank_select_w (0,0);
}


/***************************************************************************

        Colony7-specific routines

***************************************************************************/

/*
 *  Colony7 Select a bank
 *  There is just data in bank 0
 */
void colony7_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0xc000 };

	/* set bank address */
	cpu_setbank(1,&RAM[bank[data&7]]);
	if( bank[data] < 0x10000 ){
		/* i/o area map */
		cpu_setbankhandler_r(1,defender_io_r );
		cpu_setbankhandler_w(1,colony7_io_w );
	}else{
		/* bank rom map */
		cpu_setbankhandler_r(1,MRA_BANK1);
		cpu_setbankhandler_w(1,MWA_ROM);
	}
}



/*
 *  Defender Write at C000-CFFF
 */

void colony7_io_w (int offset,int data)
{
	defender_bank_base[offset] = data;

	/* WatchDog */
	if (offset == 0x03fc)
		return;

	/* Palette */
	if (offset < 0x10)
		williams_palette_w (offset, data);

	/* Sound */
	if (offset == 0x0c02 && data < 0x20)
		williams_sh_w (offset,data);
}
