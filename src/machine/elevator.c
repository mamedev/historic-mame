/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int bank;

int elevator_init_machine(const char *gamename)
{
	bank = 0x00;

#if 0
	/* patch the roms to remove protection check */
	RAM[0x33c6] = 0;
	RAM[0x33c7] = 0;
	RAM[0x33c8] = 0;

	/* remove ROM checkusm check as well */
	RAM[0x34be] = 0xc9;


	/* avoid lockups, but this is only a kludge which removes portions of the code */
	RAM[0x0073] = 0xc9;
	RAM[0x007f] = 0xc9;
#endif
	return 0;
}

unsigned char *elevator_protection;

int elevator_protection_r(int offset)
{
	int data;

	data = (int)(*elevator_protection);
	/*********************************************************************/
	/*  elevator action , fast entry 52h , must be return 17h            */
	/*  (preliminary version)                                            */
	data = data - 0x3b;
	/*********************************************************************/
	if (errorlog) fprintf(errorlog,"Protection entry:%02x , return:%02x\n" , *elevator_protection , data );
	return data;
}


int elevator_protection_t_r(int offset)
{
	return 0xff;
}



void elevatob_bankswitch_w(int offset,int data)
{
	if ((data & 0x80) != bank)
	{
		if (data & 0x80) memcpy(&RAM[0x7000],&RAM[0xf000],0x1000);
		else memcpy(&RAM[0x7000],&RAM[0xe000],0x1000);

		bank = data & 0x80;
	}
}
