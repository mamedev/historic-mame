/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static unsigned char voltable[255];


void taito_init_machine(void)
{
	int i,j;
	int weight[8],totweight;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* set the default ROM bank (many games only have one bank and */
	/* never write to the bank selector register) */
	cpu_setbank(1,&RAM[0x6000]);


	/* reproduce the resistor ladder for the DAC output

	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+-------- out
	*/

	totweight = 0;
	for (i = 0;i < 8;i++)
	{
		weight[i] = 75600 / (30 + (7-i) * 15);
		totweight += weight[i];
	}

	for (i = 0;i < 8;i++)
		weight[i] = (255 * weight[i] + totweight / 2) / totweight;

	for (i = 0;i < 256;i++)
	{
		voltable[i] = 0;

		for (j = 0;j < 8;j++)
		{
			if ((i >> j) & 1)
				voltable[i] += weight[j];
		}
	}
}



void taito_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (data & 0x80) { cpu_setbank(1,&RAM[0x10000]); }
	else cpu_setbank(1,&RAM[0x6000]);
}



void taito_digital_out(int offset,int data)
{
	DAC_data_w(0,voltable[data]);
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
return 0;
	return data;
}


int elevator_protection_t_r(int offset)
{
	return 0xff;
}
