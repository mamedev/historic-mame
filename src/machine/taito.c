/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


void taito_init_machine(void)
{
	cpu_setbank(1,&RAM[0x6000]);
}



void taito_bankswitch_w(int offset,int data)
{
	if (data & 0x80) { cpu_setbank(1,&RAM[0x10000]); }
	else cpu_setbank(1,&RAM[0x6000]);
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
