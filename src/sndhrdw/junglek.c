#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


static int sndnmi_disable = 1;

static int taito_dsw_2_r(int offset)
{
	return readinputport(5);
}

static int taito_dsw_3_r(int offset)
{
	return readinputport(6);
}

static void sndnmi_msk( int offset , int data )
{
	sndnmi_disable = data & 0x01;
}

int junglek_sh_interrupt(void)
{
	static int count;

	count++;
	if (count>=8){
		count = 0;
		return 0xff;
	}
	else
	{
		if (pending_commands && !sndnmi_disable) return Z80_NMI_INT;
		else return Z80_IGNORE_INT;
	}
}



static struct AY8910interface interface =
{
	4,	/* 4 chips */
	1,	/* 1 update per video frame (low quality) */
	1789750000,	/* 1.78975 MHZ ???? */
	{ 255, 255, 255, 255 },
	{ taito_dsw_2_r },		/* port Aread */
	{ taito_dsw_3_r },		/* port Bread */
	{ 0,0,0,0},				/* port Awrite */
	{ 0,0,0,sndnmi_msk }	/* port Bwrite */
};



int junglek_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
