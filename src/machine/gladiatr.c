

/***************************************************************************

	Taito Gladiator Machine Hardware

	Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
	I/O ports)

***************************************************************************/

#include "driver.h"

static int banka;
//static int port9f;
static unsigned char port9e_c1;
static unsigned char port9f_c1;
static unsigned char port20_c2;
static unsigned char port21_c2;
static unsigned char port9f_active=0;

void gladiatr_bankswitch_w(int offset,int data){
	static int bank1[2] = { 0x10000, 0x12000 };
	static int bank2[2] = { 0x14000, 0x18000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	banka = data;

	cpu_setbank(1,&RAM[bank1[(data & 0x03)]]);
	cpu_setbank(2,&RAM[bank2[(data & 0x03)]]);
}

int gladiatr_bankswitch_r(int offset){
	return banka;
}


void gladiatr_port21_c2w(int offset, int data){
	port21_c2 = data;
}

void gladiatr_port20_c2w(int offset, int data){
	port20_c2 = data;
}

int gladiatr_port20_c2r(int offset){
	return port20_c2;
}

int gladiatr_port21_c2r(int offset){
	return port20_c2;
}

int gladiatr_port9f_c1r(int offset){
	if (port9f_active == 0) return 0;
	return port21_c2;
}

void gladiatr_port9f_c1w(int offset, int data){
	if (data == 0x0a) port9f_active = 1;
	port9e_c1 = data;
	port9f_c1 = data;
}

int gladiatr_port9e_c1r(int offset){
	return port9e_c1;
}

int gladiatr_patch(int offset){
	return 01;
}

void gladiatr_port9e_c1w(int offset, int data){
   //  port9e_c1 = data;
}
