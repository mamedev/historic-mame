/***************************************************************************

  most Capcom games have a similar sound hardware.
  This sound driver is used by 1942, Commando, Ghosts 'n Goblins,
  Exed Exes, Son Son, Gunsmoke, and others.

***************************************************************************/

#include "driver.h"
#include "sndhrdw/generic.h"
/* #include "sndhrdw/8910intf.h" */
#include "sndhrdw/2203intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 0x20ff, 0x20ff },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface OPNinterface =
{
	2,			/* 2 chips */
	3000000,	/* 3.0 MHZ ? */
	{ YM2203_VOL(100,0x20ff), YM2203_VOL(100,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


int capcom_sh_start(void)
{
	return AY8910_sh_start(&interface);
}

int capcomOPN_sh_start(void)
{
	return YM2203_sh_start(&OPNinterface);
}
