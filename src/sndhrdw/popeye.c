#include "driver.h"
#include "sndhrdw/8910intf.h"



static int dswbit;


int popeye_sh_interrupt(void)
{
	AY8910_update();

	/* the CPU needs 2 interrupts per frame, the handler is called 20 times */
	if (cpu_getiloops() % 10 == 0) return nmi_interrupt();
	else return ignore_interrupt();
}



static void popeye_portB_w(int offset,int data)
{
	dswbit = data / 2;
}



static int popeye_portA_r(int offset)
{
	int res;


	res = input_port_3_r(offset);
	res |= (input_port_4_r(offset) << (7-dswbit)) & 0x80;

	return res;
}



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	20,	/* 20 updates per video frame (good quality) (the frame rate is 30fps) */
	1832727040,	/* 1.832727040 MHZ ????? */
	{ 255 },
	{ popeye_portA_r },
	{ 0 },
	{ 0 },
	{ popeye_portB_w }
};



int popeye_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
