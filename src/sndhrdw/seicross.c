#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


static int seicross_portB_r(int offset)
{
	return readinputport(3);
}


static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1536000000,	/* 1.536 MHZ ?? */
	{ 255 },
	{  },
	{ seicross_portB_r },
	{  },
	{  }
};



int seicross_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
