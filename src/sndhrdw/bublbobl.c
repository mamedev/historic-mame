#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	1536000000,	/* 1.536000000 MHZ????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int bublbobl_sh_interrupt(void)
{
	if (pending_commands) return nmi_interrupt();
	else return interrupt();
}



int bublbobl_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
