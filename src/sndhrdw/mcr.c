#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw/dac.h"
#include "sndhrdw/5220intf.h"

int mcr_sound_ram_size;

static unsigned char *mcr_sound_ram;


static struct AY8910interface interface =
{
	2,	/* 2 chips */
	2000000,	/* 2 MHZ ?? */
	{ 0x20ff, 0x20ff },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_intf =
{
	1,
	441000,
	{ 255 },
	{ 0 },
};

static struct TMS5220interface tms_intf =
{
	640000,
	192,
	0
};


int mcr_sh_start(void)
{
	return AY8910_sh_start(&interface);
}


int rampage_sh_start (void)
{
	if (!(mcr_sound_ram = malloc (mcr_sound_ram_size)))
		return 1;
	cpu_setbank(1, mcr_sound_ram);
	return DAC_sh_start (&dac_intf);
}

void rampage_sh_stop (void)
{
	DAC_sh_stop();
	if (mcr_sound_ram)
		free (mcr_sound_ram);
	mcr_sound_ram = 0;
}

int sarge_sh_start (void)
{
	return DAC_sh_start (&dac_intf);
}

int spyhunt_sh_start(void)
{
	if (!(mcr_sound_ram = malloc (mcr_sound_ram_size)))
		return 1;
	cpu_setbank(1, mcr_sound_ram);
	if (DAC_sh_start(&dac_intf))
		return 1;
	return AY8910_sh_start(&interface);
}


void spyhunt_sh_stop(void)
{
	AY8910_sh_stop ();
	DAC_sh_stop ();
}


void spyhunt_sh_update(void)
{
	AY8910_sh_update ();
	DAC_sh_update ();
}


int dotron_sh_start(void)
{
	if (tms5220_sh_start(&tms_intf))
		return 1;
	return AY8910_sh_start(&interface);
}


void dotron_sh_stop(void)
{
	AY8910_sh_stop ();
	tms5220_sh_stop ();
}


void dotron_sh_update(void)
{
	AY8910_sh_update ();
	tms5220_sh_update ();
}
