/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



unsigned char *taito_dsw23_select;

static int bank;



int taito_dsw23_r(int offset)
{
	if (*taito_dsw23_select == 0x0e) return readinputport(5);
	else if (*taito_dsw23_select == 0x0f) return readinputport(6);
	else if (errorlog) fprintf(errorlog,"d40e = %02x\n",*taito_dsw23_select);

	return 0;
}



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



int elevator_protection_r(int offset)
{
	return 0;
}



int elevator_unknown_r(int offset)
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
