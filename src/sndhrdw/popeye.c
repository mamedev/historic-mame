#include "driver.h"
#include "sndhrdw/8910intf.h"



static int dswbit;


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
	1536000000,	/* 1.536000000 MHZ ????? */
	{ 255 },
	{ popeye_portA_r },
	{ },
	{ },
	{ popeye_portB_w }
};



int popeye_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
