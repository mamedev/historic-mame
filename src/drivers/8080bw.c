/****************************************************************************/
/*                                                                          */
/* 8080bw.c                                                                 */
/*                                                                          */
/*  Michael Strutts, Nicola Salmoria, Tormod Tjaberg, Mirko Buffoni         */
/*  Lee Taylor, Valerio Verrando, Marco Cassili and others                  */
/*                                                                          */
/*                                                                          */
/* Exidy                                                                    */
/* -----                                                                    */
/* Bandido (bandido)                                                        */
/*                                                                          */
/* Midway                                                                   */
/* ------                                                                   */
/* 596 - Sea Wolf (seawolf)                                                 */
/* 597 - Gunfight (gunfight)                                                */
/* 605 - Tornado Baseball (tornbase)                                        */
/* 610 - 280-ZZZAP (zzzap)                                                  */
/* 611 - Amazing Maze (maze)                                                */
/* 612 - Boot Hill (boothill)                                               */
/* 615 - Checkmate                                                          */
/* 618 - Desert Gun                                                         */
/* 619 - Double Play                                                        */
/* 622 - Laguna Racer                                                       */
/* 623 - Guided Missile (gmissile)                                          */
/* 626 - M-4 (m4)                                                           */
/* 630 - Clowns (clowns)                                                    */
/* 640 - Space Walk                                                         */
/* 642 - Extra Innings                                                      */
/* 643 - Shuffleboard                                                       */
/* 644 - Dog Patch (dogpatch)                                               */
/* 645 - Space Encounters (spcenctr)                                        */
/* 652 - Phantom II                                                         */
/* 730 - 4 Player Bowling                                                   */
/* 739 - Space Invaders (invaders)                                          */
/* 742 - Blue Shark                                                         */
/* 851 - Space Invaders Part II (Midway) (invdelux)                         */
/* 852 - Space Invaders Deluxe                                              */
/*                                                                          */
/* Taito                                                                    */
/* -----                                                                    */
/* Space Invaders Part II (Taito) (invadpt2)                                */
/* Space Laser (GPI Intruder)                                               */
/* Galaxy Wars (galxwars)                                                   */
/* Lunar Rescue (lrescue)                                                   */
/* Lupin III (lupin3)                                                       */
/* Space Chaser (schaser)                                                   */
/*                                                                          */
/* Nichibutsu                                                               */
/* ----------                                                               */
/* Rolling Crash - Moon Base (rollingc)                                     */
/*                                                                          */
/* Nintendo                                                                 */
/* --------                                                                 */
/* Heli Fire                                                                */
/* Space Fever                                                              */
/*                                                                          */
/* Universal                                                                */
/* ---------                                                                */
/* Cosmic Monsters (cosmicmo)                                               */
/*                                                                          */
/* Zeltec                                                                   */
/* ------                                                                   */
/* Space Attack II (spaceatt)                                               */
/* Invaders Revenge (invrvnge)                                              */
/*                                                                          */
/* Super Earth Invasion (earthinv)                                          */
/* Destination Earth (desterth)                                             */
/* Space Phantoms (spaceph)                                                 */
/*                                                                          */
/*                                                                          */
/* Notes:                                                                   */
/* 	Space Chaser writes to $c400-$df9f. Color?                              */
/*                                                                          */
/*                                                                          */
/* Known problems:                                                          */
/* --------------                                                           */
/* 	Space Encounter, Sea Wolf need their movement controls fixed.           */
/* 	The accelerator in 280 Zzzap could be handled better.                   */
/*                                                                          */
/* List of Changes                                                          */
/* ---------------                                                          */
/* 	MJC - 22.01.98 - Boot hill controls/memory mappings                     */
/* 	LBO - 5 May 98 - Cleaned up the code, added Al's extra drivers, rotation support */
/*                                                                          */
/****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* in machine/8080bw.c */

int  invaders_shift_data_r(int offset);
int  midbowl_shift_data_r(int offset);
int  midbowl_shift_data_rev_r(int offset);
void invaders_shift_amount_w(int offset,int data);
void invaders_shift_data_w(int offset,int data);
int  invaders_interrupt(void);

int  boothill_shift_data_r(int offset);                  /* MJC 310198 */
int  boothill_port_0_r(int offset);                      /* MJC 310198 */
int  boothill_port_1_r(int offset);                      /* MJC 310198 */
void boothill_sh_port3_w(int offset, int data);          /* HC 4/14/98 */
void boothill_sh_port5_w(int offset, int data);          /* HC 4/14/98 */

int  gray6bit_controller0_r(int offset);
int  gray6bit_controller1_r(int offset);
int  seawolf_shift_data_r(int offset);
int  seawolf_port_0_r(int offset);

/* in video/8080bw.c */

extern unsigned char *invaders_videoram;

void boothill_videoram_w(int offset,int data);   /* MAB */
void invaders_videoram_w(int offset,int data);
void invrvnge_videoram_w(int offset,int data);   /* V.V */
void lrescue_videoram_w(int offset,int data);    /* V.V */
void rollingc_videoram_w(int offset,int data);   /* L.T */

int  invaders_vh_start(void);
void invaders_vh_stop(void);
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void invaders_sh_port3_w(int offset,int data);
void invaders_sh_port4_w(int offset,int data);
void invadpt2_sh_port3_w(int offset,int data);
void invaders_sh_port5_w(int offset,int data);
void invaders_sh_update(void);

static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0xff,0xff,0xff, /* WHITE */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE,CYAN,PURPLE }; /* V.V */

/*******************************************************/
/*                                                     */
/* Midway "Space Invaders"                             */
/*                                                     */
/*******************************************************/

ROM_START( invaders_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h", 0x0000, 0x0800, 0x50d4f038 )
	ROM_LOAD( "invaders.g", 0x0800, 0x0800, 0x0d20437a )
	ROM_LOAD( "invaders.f", 0x1000, 0x0800, 0x5175f639 )
	ROM_LOAD( "invaders.e", 0x1800, 0x0800, 0x1206db68 )
ROM_END

/* invaders, earthinv, spaceatt, invrvnge, invdelux, invadpt2, galxwars, lrescue, */
/* desterth, cosmicmo, spaceph */
static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x57ff, MRA_ROM },
	{ -1 }  /* end of table */
};

/* invaders, earthinv, spaceatt, invdelux, galxwars, desterth, cosmicmo */
static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invaders_videoram_w, &invaders_videoram },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};


/* Catch the write to unmapped I/O port 6 */
void invaders_dummy_write(int offset,int data)
{
}

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};
INPUT_PORTS_START( invaders_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END

static struct Samplesinterface samples_interface =
{
	9       /* 9 channels */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invaders_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20f4],"\x00\x00\x1c",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x20f4],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invaders_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x20f4],2);
		osd_fclose(f);
	}
}

static const char *invaders_sample_names[] =
{
	"*invaders",
	"0.SAM",
	"1.SAM",
	"2.SAM",
	"3.SAM",
	"4.SAM",
	"5.SAM",
	"6.SAM",
	"7.SAM",
	"8.SAM",
	0       /* end of array */
};

struct GameDriver invaders_driver =
{
	__FILE__,
	0,
	"invaders",
	"Space Invaders",
	"1978",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,

	invaders_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invaders_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Space Invaders Part II"                     */
/*                                                     */
/*******************************************************/

ROM_START( invaders2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "pv.01", 0x0000, 0x0800,0x8b00eeba )
	ROM_LOAD( "pv.02", 0x0800, 0x0800,0x0c7adc30 )
	ROM_LOAD( "pv.03", 0x1000, 0x0800,0xb2384c12 )
	ROM_LOAD( "pv.04", 0x1800, 0x0800,0x14411f47 )
	ROM_LOAD( "pv.05", 0x4000, 0x0800,0x2990da7a )
ROM_END

static struct IOReadPort invadpt2_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

/* LT 20-3-1998 */
static struct IOWritePort invadpt2_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invadpt2_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress invadpt2_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invadpt2_input_ports )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* N ? */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END

static struct MachineDriver invadpt2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, invadpt2_writemem, invadpt2_readport, invadpt2_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invadpt2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20f4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x2340], 0xa);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invadpt2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20f4], 0x2);
		/* Save the name */
		osd_fwrite(f,&RAM[0x2340], 0xa);
		osd_fclose(f);
	}
}

/* LT 24-11-1997 */
/* LT 20-3-1998 UPDATED */
struct GameDriver invadpt2_driver =
{
	__FILE__,
	0,
	"invadpt2",
	"Space Invaders Part II (Taito)",
	"1980",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	0,
	&invadpt2_machine_driver,

	invaders2_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invadpt2_input_ports,

	0,palette, 0,
	ORIENTATION_ROTATE_270,

	invadpt2_hiload, invadpt2_hisave
};

/*******************************************************/
/*                                                     */
/* ?????? "Super Earth Invasion"                       */
/*                                                     */
/*******************************************************/

ROM_START( earthinv_rom )
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "earthinv.h", 0x0000, 0x0800, 0xec409c20 )
	ROM_LOAD( "earthinv.g", 0x0800, 0x0800, 0x46a1b083 )
	ROM_LOAD( "earthinv.f", 0x1000, 0x0800, 0xfcb8d54c )
	ROM_LOAD( "earthinv.e", 0x1800, 0x0800, 0x38dff747 )
ROM_END

INPUT_PORTS_START( earthinv_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/1 Credit" )
INPUT_PORTS_END

struct GameDriver earthinv_driver =
{
	__FILE__,
	&invaders_driver,
	"earthinv",
	"Super Earth Invasion",
	"1980",
	"bootleg",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nMarco Cassili",
	0,
	&machine_driver,

	earthinv_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	earthinv_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* ?????? "Space Attack II"                            */
/*                                                     */
/*******************************************************/

ROM_START( spaceatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "spaceatt.h", 0x0000, 0x0800, 0x607fceab )
	ROM_LOAD( "spaceatt.g", 0x0800, 0x0800, 0x47146b7c )
	ROM_LOAD( "spaceatt.f", 0x1000, 0x0800, 0xf7306b0a )
	ROM_LOAD( "spaceatt.e", 0x1800, 0x0800, 0xb362b53c )
ROM_END

INPUT_PORTS_START( spaceatt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

struct GameDriver spaceatt_driver =
{
	__FILE__,
	&invaders_driver,
	"spaceatt",
	"Space Attack II",
	"1980",
	"Video Games, Ltd.",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,

	spaceatt_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spaceatt_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};

/*******************************************************/
/*                                                     */
/* Zenitone Microsec "Invaders Revenge"                */
/*                                                     */
/*******************************************************/

ROM_START( invrvnge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invrvnge.h", 0x0000, 0x0800, 0xe6a7b0ab )
	ROM_LOAD( "invrvnge.g", 0x0800, 0x0800, 0xc3355da1 )
	ROM_LOAD( "invrvnge.f", 0x1000, 0x0800, 0x81b0bac0 )
	ROM_LOAD( "invrvnge.e", 0x1800, 0x0800, 0xe90b9c6f )
ROM_END

static struct MemoryWriteAddress invrvnge_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invrvnge_videoram_w, &invaders_videoram },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invrvnge_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

static struct MachineDriver invrvnge_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, invrvnge_writemem, readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invrvnge_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2003],"\xce\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2019],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void invrvnge_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2019],3);
		osd_fclose(f);
	}
}

struct GameDriver invrvnge_driver =
{
	__FILE__,
	0,
	"invrvnge",
	"Invader's Revenge",
	"????",
	"Zenitone Microsec",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nMarco Cassili (high score save)",
	0,
	&invrvnge_machine_driver,

	invrvnge_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invrvnge_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invrvnge_hiload, invrvnge_hisave
};


/*******************************************************/
/*                                                     */
/* Midway "Space Invaders Deluxe"                      */
/*                                                     */
/*******************************************************/

ROM_START( invdelux_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invdelux.h", 0x0000, 0x0800, 0x5b009eba )
	ROM_LOAD( "invdelux.g", 0x0800, 0x0800, 0x5f0db2f5 )
	ROM_LOAD( "invdelux.f", 0x1000, 0x0800, 0x0204561c )
	ROM_LOAD( "invdelux.e", 0x1800, 0x0800, 0x8a364d1a )
	ROM_LOAD( "invdelux.d", 0x4000, 0x0800, 0x5239e87f )
ROM_END

static struct IOReadPort invdelux_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invdelux_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* N ? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END


static struct MachineDriver invdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, writemem, invdelux_readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invdelux_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20f4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x2340], 0xa);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invdelux_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20f4], 0x2);
		/* Save the name */
		osd_fwrite(f,&RAM[0x2340], 0xa);
		osd_fclose(f);
	}
}

struct GameDriver invdelux_driver =
{
	__FILE__,
	&invadpt2_driver,
	"invdelux",
	"Space Invaders Deluxe (Midway)",
	"1980",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&invdelux_machine_driver,

	invdelux_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invdelux_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invdelux_hiload, invdelux_hisave
};


/*******************************************************/
/*                                                     */
/* ?????? "Astro Laser"                                */
/*                                                     */
/*******************************************************/

ROM_START( astlaser_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "1.u36", 0x0000, 0x0800, 0xeada1922 )
	ROM_LOAD( "2.u35", 0x0800, 0x0800, 0x2a011857 )
	ROM_LOAD( "3.u34", 0x1000, 0x0800, 0x77693c09 )
	ROM_LOAD( "4.u33", 0x1800, 0x0800, 0xe83fc1e1 )
ROM_END

struct GameDriver astlaser_driver =
{
	__FILE__,
	0,
	"astlaser",
	"Astro Laser",
	"1980",
	"?????",
	"The Space Invaders Team",
	0,
	&invdelux_machine_driver,

	astlaser_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invdelux_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Game Plan "Intruder"                                */
/*                                                     */
/*******************************************************/

ROM_START( intruder_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "LA01", 0x0000, 0x0800, 0x1317576b )
	ROM_LOAD( "LA02", 0x0800, 0x0800, 0x64dc1868 )
	ROM_LOAD( "LA03", 0x1000, 0x0800, 0x10ce7edc )
	ROM_LOAD( "LA04", 0x1800, 0x0800, 0x2c72977e )
ROM_END

struct GameDriver intruder_driver =
{
	__FILE__,
	&astlaser_driver,
	"intruder",
	"Intruder",
	"1980",
	"Game Plan, Inc. (Taito)",
	"The Space Invaders Team",
	0,
	&invdelux_machine_driver,

	intruder_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invdelux_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Taito "Galaxy Wars"                                 */
/*                                                     */
/*******************************************************/

ROM_START( galxwars_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "galxwars.0", 0x0000, 0x0400, 0x3ad5521b )
	ROM_LOAD( "galxwars.1", 0x0400, 0x0400, 0xce40024c )
	ROM_LOAD( "galxwars.2", 0x0800, 0x0400, 0xe3dff58d )
	ROM_LOAD( "galxwars.3", 0x0c00, 0x0400, 0x93490efd )
	ROM_LOAD( "galxwars.4", 0x4000, 0x0400, 0x83edc2fd )
	ROM_LOAD( "galxwars.5", 0x4400, 0x0400, 0xc1234a3d )
ROM_END

INPUT_PORTS_START( galxwars_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

static int galxwars_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2000],"\x07\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2004],7);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void galxwars_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2004],7);
		osd_fclose(f);
	}
}

struct GameDriver galxwars_driver =
{
	__FILE__,
	0,
	"galxwars",
	"Galaxy Wars",
	"1979",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,

	galxwars_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	galxwars_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	galxwars_hiload, galxwars_hisave
};

/*******************************************************/
/*                                                     */
/* Taito "Lunar Rescue"                                */
/*                                                     */
/*******************************************************/

ROM_START( lrescue_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lrescue.1", 0x0000, 0x0800, 0x9fa38d01 )
	ROM_LOAD( "lrescue.2", 0x0800, 0x0800, 0x3d00e2b8 )
	ROM_LOAD( "lrescue.3", 0x1000, 0x0800, 0x2fc56073 )
	ROM_LOAD( "lrescue.4", 0x1800, 0x0800, 0xc2170fa3 )
	ROM_LOAD( "lrescue.5", 0x4000, 0x0800, 0x238ce80c )
	ROM_LOAD( "lrescue.6", 0x4800, 0x0800, 0x464afa4a )
ROM_END

/* lrescue, invadpt2, spaceph */
static struct MemoryWriteAddress lrescue_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( lrescue_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

static struct MachineDriver lrescue_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, lrescue_writemem, readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int lrescue_hiload(void)     /* V.V */ /* Whole function */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20F4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x20CF], 0xa);
			/* Load the high score length */
			osd_fread(f,&RAM[0x20DB], 0x1);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */

}

static void lrescue_hisave(void)    /* V.V */ /* Whole function */
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20F4],0x02);
		/* Save the name */
		osd_fwrite(f,&RAM[0x20CF],0xa);
		/* Save the high score length */
		osd_fwrite(f,&RAM[0x20DB],0x1);
		osd_fclose(f);
	}
}

struct GameDriver lrescue_driver =
{
	__FILE__,
	0,
	"lrescue",
	"Lunar Rescue",
	"1979",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&lrescue_machine_driver,

	lrescue_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	lrescue_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	lrescue_hiload, lrescue_hisave  /* V.V */
};

/*******************************************************/
/*                                                     */
/* ?????? "Destination Earth"                          */
/*                                                     */
/*******************************************************/

ROM_START( desterth_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "36_h.bin", 0x0000, 0x0800, 0x386b443d )
	ROM_LOAD( "35_g.bin", 0x0800, 0x0800, 0x838c80be )
	ROM_LOAD( "34_f.bin", 0x1000, 0x0800, 0x1fc49074 )
	ROM_LOAD( "33_e.bin", 0x1800, 0x0800, 0x7bb1a60f )
	ROM_LOAD( "32_d.bin", 0x4000, 0x0800, 0x5a2d5195 )
	ROM_LOAD( "31_c.bin", 0x4800, 0x0800, 0x8c9da1e1 )
	ROM_LOAD( "42_b.bin", 0x5000, 0x0800, 0xcea0bb2c )
ROM_END

INPUT_PORTS_START( desterth_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

static int desterth_hiload(void)     /* V.V */ /* Whole function */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x07",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20F4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x20CF], 0xa);
			/* Load the high score length */
			osd_fread(f,&RAM[0x20DB], 0x1);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

struct GameDriver desterth_driver =
{
	__FILE__,
	&lrescue_driver,
	"desterth",
	"Destination Earth",
	"1979",
	"bootleg",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,

	desterth_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	desterth_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	desterth_hiload, lrescue_hisave
};

/*******************************************************/
/*                                                     */
/* Universal "Cosmic Monsters"                         */
/*                                                     */
/*******************************************************/

ROM_START( cosmicmo_rom )  /* L.T */
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cosmicmo.1", 0x0000, 0x0400,0x3ccca2e4 )
	ROM_LOAD( "cosmicmo.2", 0x0400, 0x0400,0xc8ecbfbc )
	ROM_LOAD( "cosmicmo.3", 0x0800, 0x0400,0xd9e97659 )
	ROM_LOAD( "cosmicmo.4", 0x0c00, 0x0400,0xa1c2ead0 )
	ROM_LOAD( "cosmicmo.5", 0x4000, 0x0400,0xff83f719 )
	ROM_LOAD( "cosmicmo.6", 0x4400, 0x0400,0x79df7b39 )
	ROM_LOAD( "cosmicmo.7", 0x4800, 0x0400,0x95dd75a7 )
ROM_END

INPUT_PORTS_START( cosmicmo_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
INPUT_PORTS_END

struct GameDriver cosmicmo_driver =
{
	__FILE__,
	&invaders_driver,
	"cosmicmo",
	"Cosmic Monsters",
	"1979",
	"Universal",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	0,
	&machine_driver,

	cosmicmo_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	cosmicmo_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};


/*******************************************************/
/*                                                     */
/* Zilec "Space Phantoms"                              */
/*                                                     */
/*******************************************************/

ROM_START( spaceph_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sv01.bin", 0x0000, 0x0400, 0x5e769e98 )
	ROM_LOAD( "sv02.bin", 0x0400, 0x0400, 0xaaacc506 )
	ROM_LOAD( "sv03.bin", 0x0800, 0x0400, 0x734dec43 )
	ROM_LOAD( "sv04.bin", 0x0c00, 0x0400, 0xf16661e4 )
	ROM_LOAD( "sv05.bin", 0x1000, 0x0400, 0x02ba1408 )
	ROM_LOAD( "sv06.bin", 0x1400, 0x0400, 0xcab9ea59 )
	ROM_LOAD( "sv07.bin", 0x1800, 0x0400, 0xc8f1c563 )
	ROM_LOAD( "sv08.bin", 0x1c00, 0x0400, 0x923d6d79 )
	ROM_LOAD( "sv09.bin", 0x4000, 0x0400, 0xae6f21e9 )
	ROM_LOAD( "sv10.bin", 0x4400, 0x0400, 0x1176e23a )
	ROM_LOAD( "sv11.bin", 0x4800, 0x0400, 0xbbb49a42 )
	ROM_LOAD( "sv12.bin", 0x4c00, 0x0400, 0xca632a7b )
ROM_END

INPUT_PORTS_START( spaceph_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "35000" )
	PORT_DIPSETTING(    0x02, "25000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt */
	PORT_DIPNAME( 0x08, 0x00, "Bonus Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Fire */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
INPUT_PORTS_END

/* LT 3-12-1997 */
struct GameDriver spaceph_driver =
{
	__FILE__,
	0,
	"spaceph",
	"Space Phantoms",
	"????",
	"Zilec Games",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan\nMarco Cassili",
	0,
	&lrescue_machine_driver,

	spaceph_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spaceph_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Nichibutsu "Rolling Crash"                          */
/*                                                     */
/*******************************************************/

ROM_START( rollingc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rc01.bin", 0x0000, 0x0400, 0x6fcfc861 )
	ROM_LOAD( "rc02.bin", 0x0400, 0x0400, 0xaa7205d8 )
	ROM_LOAD( "rc03.bin", 0x0800, 0x0400, 0xb1d8f7c8 )
	ROM_LOAD( "rc04.bin", 0x0c00, 0x0400, 0x2637f611 )
	ROM_LOAD( "rc05.bin", 0x1000, 0x0400, 0x82c0fb70 )
	ROM_LOAD( "rc06.bin", 0x1400, 0x0400, 0x52da00c8 )
	ROM_LOAD( "rc07.bin", 0x1800, 0x0400, 0xf0c569f7 )
	ROM_LOAD( "rc08.bin", 0x1c00, 0x0400, 0x033ac1a8 )
	ROM_LOAD( "rc09.bin", 0x4000, 0x0800, 0x4c3b8fe3 )
	ROM_LOAD( "rc10.bin", 0x4800, 0x0800, 0xceebaefd )
	ROM_LOAD( "rc11.bin", 0x5000, 0x0800, 0x68228d74 )
	ROM_LOAD( "rc12.bin", 0x5800, 0x0800, 0xdd2c4d68 )
ROM_END

static struct MemoryReadAddress rollingc_readmem[] =
{
//      { 0x2000, 0x2002, MRA_RAM },
//      { 0x2003, 0x2003, hack },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0xa400, 0xbfff, MRA_RAM },
	{ 0xe400, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rollingc_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, rollingc_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0xa400, 0xbfff, MWA_RAM },
	{ 0xe400, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};
INPUT_PORTS_START( rollingc_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) /* Game Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) /* Game Select */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 1 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt ? */
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) /* Player 2 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) /* Player 2 Controls */
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

static unsigned char rollingc_palette[] =
{
	0xff,0xff,0xff, /* WHITE */
	0xa0,0xa0,0xa0, /* ?????? */
	0x00,0xff,0x00, /* GREEN */
	0x00,0xa0,0xa0, /* GREEN */
	0xff,0x00,0xff, /* PURPLE */
	0x00,0xff,0xff, /* CYAN */
	0xa0,0x00,0xa0, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0xff,0x00, /* YELLOW */
	0x80,0x40,0x20, /* ?????? */
	0x80,0x80,0x00, /* GREEN */
	0x00,0x80,0x80, /* GREEN */
	0xa0,0xa0,0xff, /* PURPLE */
	0xa0,0xff,0x80, /* CYAN */
	0xff,0xff,0x00, /* WHITE */
	0xff,0x00,0xa0, /* RED */
	0x00,0x00,0x00  /* BLACK */
};

static struct MachineDriver rollingc_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			rollingc_readmem,rollingc_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(rollingc_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

/* LT 3-12-1997 */
struct GameDriver rollingc_driver =
{
	__FILE__,
	0,
	"rollingc",
	"Rolling Crash / Moon Base",
	"1979",
	"Nichibutsu",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan",    /*L.T */
	0,
	&rollingc_machine_driver,

	rollingc_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	rollingc_input_ports,

	0,rollingc_palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Exidy "Bandido"                                     */
/*                                                     */
/*******************************************************/

ROM_START( bandido_rom )                                                                                /* MJC */
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "BAF1-3", 0x0000, 0x0400, 0xc62cf9c2 )
	ROM_LOAD( "BAF2-1", 0x0400, 0x0400, 0xcc34ebda )
	ROM_LOAD( "BAG1-1", 0x0800, 0x0400, 0x23f48c36 )
	ROM_LOAD( "BAG2-1", 0x0C00, 0x0400, 0x2d832c93 )
	ROM_LOAD( "BAH1-1", 0x1000, 0x0400, 0x7be81f76 )
	ROM_LOAD( "BAH2-1", 0x1400, 0x0400, 0x853c7eca )
	ROM_LOAD( "BAI1-1", 0x1800, 0x0400, 0x0a903e90 )
	ROM_LOAD( "BAI2-1", 0x1C00, 0x0400, 0x7158d760 )
	ROM_LOAD( "BAJ1-1", 0x2000, 0x0400, 0xd563f205 )
	ROM_LOAD( "BAJ2-2", 0x2400, 0x0400, 0x4d14b20a )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

    ROM_REGION(0x0800)                  /* Sound 8035 + 76477 Sound Generator */
    ROM_LOAD( "BASND.U2", 0x0000, 0x0400, 0x0 )
#endif

ROM_END

static struct MemoryReadAddress bandido_readmem[] =
{
	{ 0x0000, 0x27ff, MRA_ROM },
	{ 0x4200, 0x7FFF, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bandido_writemem[] =
{
	{ 0x4200, 0x5dff, boothill_videoram_w, &invaders_videoram },
	{ 0x5E00, 0x7fff, MWA_RAM },
	{ 0x0000, 0x27ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort bandido_readport[] =                                   /* MJC */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ 0x04, 0x04, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort bandido_writeport[] =                 /* MJC */
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

/* All of the controls/dips for cocktail mode are as per the schematic */
/* BUT a coffee table version was never manufactured and support was   */
/* probably never completed.                                           */
/* e.g. cocktail players button will give 6 credits!                   */

INPUT_PORTS_START( bandido_input_ports )                        /* MJC */

	PORT_START      /* 00 Main Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )

	PORT_START      /* 01 Player 2 Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* 02 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* Marked for   */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* Expansion    */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* on Schematic */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* 04 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail (SEE NOTES)" )
INPUT_PORTS_END

static struct MachineDriver bandido_machine_driver =                    /* MJC */
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			bandido_readmem, bandido_writemem, bandido_readport, bandido_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

struct GameDriver bandido_driver =                                                              /* MJC */
{
	__FILE__,
	0,
	"bandido",
	"Bandido",
	"1980",
	"Exidy",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nMirko Buffoni\nValerio Verrando\nMike Coates\n",
	0,
	&bandido_machine_driver,

	bandido_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	bandido_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

ROM_START( boothill_rom )
	ROM_REGION(0x10000)     /* 64k for code */
    ROM_LOAD( "romh.cpu", 0x0000, 0x0800, 0xec5fd9d3 )
    ROM_LOAD( "romg.cpu", 0x0800, 0x0800, 0x4cfcd35e )
    ROM_LOAD( "romf.cpu", 0x1000, 0x0800, 0x61543dac )
    ROM_LOAD( "rome.cpu", 0x1800, 0x0800, 0xe00189af )
ROM_END

static struct MemoryWriteAddress boothill_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, boothill_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort boothill_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, boothill_port_0_r },
	{ 0x01, 0x01, boothill_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort boothill_writeport[] =                                /* MJC 310198 */
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ 0x03, 0x03, boothill_sh_port3_w },
	{ 0x05, 0x05, boothill_sh_port5_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( boothill_input_ports )                                       /* MJC 310198 */
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START      /* IN0 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )              /* Fire */

	PORT_START      /* IN1 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )              /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )                    /* Fire */

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  - 1 Player" )
	PORT_DIPSETTING(    0x01, "1 Coin  - 2 Players" )
	PORT_DIPSETTING(    0x02, "2 Coins - 1 Player" )
	PORT_DIPNAME( 0x0C, 0x00, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "64" )
	PORT_DIPSETTING(    0x04, "74" )
	PORT_DIPSETTING(    0x08, "84" )
	PORT_DIPSETTING(    0x0C, "94" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE )                   /* Test */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

    PORT_START                                                                                          /* Player 2 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE | IPF_PLAYER2, 100, 7, 1, 255, 0, 0, 0, 0, 1 )

    PORT_START                                                                                          /* Player 1 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE, 100, 7, 1, 255, OSD_KEY_Z, OSD_KEY_A, 0, 0, 1 )
INPUT_PORTS_END

static struct MachineDriver boothill_machine_driver =                   /* MJC 310198 */
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, boothill_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static const char *boothill_sample_names[] =
{
	"*boothill", /* in case we ever find any bootlegs hehehe */
	"addcoin.sam",
	"endgame.sam",
	"gunshot.sam",
	"killed.sam",
	0       /* end of array */
};

struct GameDriver boothill_driver =                                                     /* MJC 310198 */
{
	__FILE__,
	0,
	"boothill",
	"Boot Hill",
	"1977",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili\nMike Balfour\nMike Coates\n\n(added samples)\n Howie Cohen \n Douglas Silfen \n Eric Lundquist",
	0,
	&boothill_machine_driver,

	boothill_rom,
	0, 0,
	boothill_sample_names,
	0,      /* sound_prom */

	boothill_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser"                                */
/*                                                     */
/*******************************************************/

ROM_START( schaser_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rt13.bin", 0x0000, 0x0400, 0x8f508fda )
	ROM_LOAD( "rt14.bin", 0x0400, 0x0400, 0xc45e9dae )
	ROM_LOAD( "rt15.bin", 0x0800, 0x0400, 0x222fdee9 )
	ROM_LOAD( "rt16.bin", 0x0c00, 0x0400, 0xfac11b19 )
	ROM_LOAD( "rt17.bin", 0x1000, 0x0400, 0xa7e3889b )
	ROM_LOAD( "rt18.bin", 0x1400, 0x0400, 0x1a453dcf )
	ROM_LOAD( "rt19.bin", 0x1800, 0x0400, 0x711544dd )
	ROM_LOAD( "rt20.bin", 0x1c00, 0x0400, 0xec27aa4b )
	ROM_LOAD( "rt21.bin", 0x4000, 0x0400, 0x1965c393 )
	ROM_LOAD( "rt22.bin", 0x4400, 0x0400, 0x2d0388f7 )
ROM_END

INPUT_PORTS_START( schaser_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt  */
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

/* LT 20-2-1998 */
struct GameDriver schaser_driver =
{
	__FILE__,
	0,
	"schaser",
	"Space Chaser",
	"1980",
	"Taito",
	"Lee Taylor\n",
	0,
	&lrescue_machine_driver,

	schaser_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	schaser_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Space Encounters"                           */
/*                                                     */
/* To Do : 'trench' video overlay                      */
/*                                                     */
/*******************************************************/

ROM_START( spcenctr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "4M33.CPU", 0x0000, 0x0800,0xe5255fef )
	ROM_LOAD( "4M32.CPU", 0x0800, 0x0800,0xc361eda7 )
	ROM_LOAD( "4M31.CPU", 0x1000, 0x0800,0x9fc06fb2 )
	ROM_LOAD( "4M30.CPU", 0x1800, 0x0800,0x9e8cbddc )
	ROM_LOAD( "4M29.CPU", 0x4000, 0x0800,0xf8daad3a )
	ROM_LOAD( "4M28.CPU", 0x4800, 0x0800,0x569c2aa8 )
	ROM_LOAD( "4M27.CPU", 0x5000, 0x0800,0xd2562fae )
	ROM_LOAD( "4M26.CPU", 0x5800, 0x0800,0x04b5b7d3 )
ROM_END

static struct IOWritePort spcenctr_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort spcenctr_readport[] =
{
	{ 0x00, 0x00, gray6bit_controller0_r }, /* These 2 ports use Gray's binary encoding */
	{ 0x01, 0x01, gray6bit_controller1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( spcenctr_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_X, 25, 0, 0x01, 0x3e) /* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )    /* fire */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_Y, 25, 0, 0x01, 0x3e) /* 6 bit vert encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2000,4000,8000" )
	PORT_DIPSETTING(    0x01, "3000,6000,12000" )
	PORT_DIPSETTING(    0x02, "4000,8000,16000" )
	PORT_DIPSETTING(    0x03, "5000,10000,20000" )
	PORT_DIPNAME( 0x0c, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin / 1 Play" )
	PORT_DIPSETTING(    0x04, "2 Coins / 1 Play" )
	PORT_DIPSETTING(    0x08, "1 Coin / 3 Plays" )
	PORT_DIPSETTING(    0x0c, "2 Coins / 3 Plays" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus/Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Bonus" )
	PORT_DIPSETTING(    0x30, "No Bonus" )
	PORT_DIPSETTING(    0x20, "Cross Hatch" )
	PORT_DIPSETTING(    0x10, "Ram/Rom Test" )
	PORT_DIPNAME( 0xc0, 0xC0, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45 Sec" )
	PORT_DIPSETTING(    0x40, "60 Sec" )
	PORT_DIPSETTING(    0x80, "75 Sec" )
	PORT_DIPSETTING(    0xC0, "90 Sec" )
INPUT_PORTS_END

static struct MachineDriver spcenctr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, spcenctr_readport, spcenctr_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver spcenctr_driver =
{
	__FILE__,
	0,
	"spcenctr",
	"Space Encounters",
	"1980",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&spcenctr_machine_driver,

	spcenctr_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spcenctr_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invdelux_hiload, invdelux_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Clowns"                                     */
/*                                                     */
/*******************************************************/

ROM_START( clowns_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "H2.CPU", 0x0000, 0x0400, 0x1393b03d )
	ROM_LOAD( "G2.CPU", 0x0400, 0x0400, 0xa7b119c7 )
	ROM_LOAD( "F2.CPU", 0x0800, 0x0400, 0xebc03102 )
	ROM_LOAD( "E2.CPU", 0x0c00, 0x0400, 0x4cabcfc1 )
	ROM_LOAD( "D2.CPU", 0x1000, 0x0400, 0xf2954e57 )
	ROM_LOAD( "C2.CPU", 0x1400, 0x0400, 0x503b9331 )
ROM_END

/*
 * Clowns (EPROM version)
 */
INPUT_PORTS_START( clowns_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1C/1P, 2C/2P" )
	PORT_DIPSETTING(    0x01, "1C/1P, 1C/2P" )
	PORT_DIPSETTING(    0x02, "2C/1P, 2C/2P" )
	PORT_DIPSETTING(    0x03, "2C/1P, 4C/2P" )
	PORT_DIPNAME( 0x0C, 0x0C, "Jumps", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2/game" )
	PORT_DIPSETTING(    0x04, "3/game" )
	PORT_DIPSETTING(    0x08, "4/game" )
	PORT_DIPSETTING(    0x0C, "5/game" )
	PORT_DIPNAME( 0x10, 0x00, "Balloon Resets", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Each row" )
	PORT_DIPSETTING(    0x10, "All rows" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Jump", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "4000" )
	PORT_DIPNAME( 0x40, 0x00, "Springboard", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x40, "Alignment On")
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x80, "Test Mode")
INPUT_PORTS_END

static struct MachineDriver clowns_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, invdelux_readport, spcenctr_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver clowns_driver =
{
	__FILE__,
	0,
	"clowns",
	"Clowns",
	"1978",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&clowns_machine_driver,

	clowns_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	clowns_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "Guided Missile"                             */
/*                                                     */
/*******************************************************/

ROM_START( gmissile_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "gm_623.h", 0x0000, 0x0800, 0x3f23e571 )
	ROM_LOAD( "gm_623.g", 0x0800, 0x0800, 0xd193bded )
	ROM_LOAD( "gm_623.f", 0x1000, 0x0800, 0x9e921c20 )
	ROM_LOAD( "gm_623.e", 0x1800, 0x0800, 0x255c2470 )
ROM_END

INPUT_PORTS_START( gmissile_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin  - 1 Player" )
	PORT_DIPSETTING(    0x02, "1 Coin  - 2 Players" )
	PORT_DIPSETTING(    0x01, "2 Coins - 1 Player" )
	PORT_DIPSETTING(    0x00, "2 Coins - 3 Players" )
	PORT_DIPNAME( 0x0C, 0x0C, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x04, "80" )
	PORT_DIPSETTING(    0x0C, "90" )
	PORT_DIPNAME( 0x30, 0x00, "Extra Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "500" )
	PORT_DIPSETTING(    0x20, "700" )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )
INPUT_PORTS_END

struct GameDriver gmissile_driver =
{
	__FILE__,
	0,
	"gmissile",
	"Guided Missile",
	"1977",
	"Midway",
	"Lee Taylor\n",
	0,
	&boothill_machine_driver,

	gmissile_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	gmissile_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

ROM_START( seawolf_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "SW0041.H", 0x0000, 0x0400, 0xca098b6f )
	ROM_LOAD( "SW0042.G", 0x0400, 0x0400, 0xfcd42f68 )
	ROM_LOAD( "SW0043.F", 0x0800, 0x0400, 0xa7b7e2af )
	ROM_LOAD( "SW0044.E", 0x0c00, 0x0400, 0xd2aa5148 )
ROM_END

static struct IOReadPort seawolf_readport[] =
{
	{ 0x00, 0x00, seawolf_shift_data_r },
	{ 0x01, 0x01, seawolf_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort seawolf_writeport[] =
{
	{ 0x03, 0x03, invaders_shift_data_w },
	{ 0x04, 0x04, invaders_shift_amount_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( seawolf_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0x1f, 0x01, IPT_PADDLE, 100, 0, 0x01, 0xfe)
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0xc0, 0xc0, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x40, "70" )
	PORT_DIPSETTING(    0x80, "80" )
	PORT_DIPSETTING(    0xc0, "90" )

	PORT_START      /* IN1 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  - 1 Player" )
	PORT_DIPSETTING(    0x04, "2 Coins  - 1 Player" ) //
	PORT_DIPSETTING(    0x08, "1 Coin - 2 Players" ) //
	PORT_DIPSETTING(    0x0c, "2 Coins - 3 Players" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_TILT ) // Reset High Scores
	PORT_DIPNAME( 0xe0, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x20, "2000" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x60, "4000" )
	PORT_DIPSETTING(    0x80, "5000" )
	PORT_DIPSETTING(    0xa0, "6000" )
	PORT_DIPSETTING(    0xc0, "7000" )
	PORT_DIPSETTING(    0xe0, "8000" )
INPUT_PORTS_END

static struct MachineDriver seawolf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,seawolf_readport,seawolf_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver seawolf_driver =
{
	__FILE__,
	0,
	"seawolf",
	"Sea Wolf",
	"1976",
	"Midway",
	"Lee Taylor\n",
	0,
	&seawolf_machine_driver,

	seawolf_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	seawolf_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

ROM_START( gunfight_rom)
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "7609H.BIN", 0x0000, 0x0400, 0x74032d05 )
	ROM_LOAD( "7609G.BIN", 0x0400, 0x0400, 0x96a82fca )
	ROM_LOAD( "7609F.BIN", 0x0800, 0x0400, 0x4d46f1b4 )
	ROM_LOAD( "7609E.BIN", 0x0c00, 0x0400, 0x1153af95 )
ROM_END

static struct IOWritePort gunfight_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( gunfight_input_ports )
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START      /* IN0 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )              /* Fire */

	PORT_START      /* IN1 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )              /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )                    /* Fire */

#ifdef NOTDEF
	PORT_START      /* IN2 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x0C, 0x00, "Plays", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time", IP_KEY_NONE ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin - 1 Player" )
	PORT_DIPSETTING(    0x40, "1 Coin - 2 Players" )
	PORT_DIPSETTING(    0x80, "1 Coin - 3 Players" )
	PORT_DIPSETTING(    0xc0, "1 Coin - 4 Players" )
#endif

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin" )
	PORT_DIPSETTING(    0x01, "2 Coins" )
	PORT_DIPSETTING(    0x02, "3 Coins" )
	PORT_DIPSETTING(    0x03, "4 Coins" )
	PORT_DIPNAME( 0x0C, 0x00, "Plays", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time", IP_KEY_NONE ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

    PORT_START                                                                                          /* Player 2 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE | IPF_PLAYER2, 100, 7, 1, 255, 0, 0, 0, 0, 1 )

    PORT_START                                                                                          /* Player 1 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE, 100, 7, 1, 255, OSD_KEY_Z, OSD_KEY_A, 0, 0, 1 )
INPUT_PORTS_END

static struct MachineDriver gunfight_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,boothill_readport,gunfight_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver gunfight_driver =
{
	__FILE__,
	0,
	"gunfight",
	"Gunfight",
	"1975",
	"Midway",
	"Lee Taylor\n",
	0,
	&gunfight_machine_driver,

	gunfight_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	gunfight_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "280 ZZZAP"                                  */
/*                                                     */
/*******************************************************/

ROM_START( zzzap_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "zzzaph", 0x0000, 0x0400, 0xb41c108e )
	ROM_LOAD( "zzzapg", 0x0400, 0x0400, 0xd9dbdfc5 )
	ROM_LOAD( "zzzapf", 0x0800, 0x0400, 0xb201f13d )
	ROM_LOAD( "zzzape", 0x0c00, 0x0400, 0x3e746ae4 )
	ROM_LOAD( "zzzapd", 0x1000, 0x0400, 0xbcdfbd65 )
	ROM_LOAD( "zzzapc", 0x1400, 0x0400, 0x5d3e3644 )
ROM_END

static struct IOWritePort zzzap_writeport[] =
{
	/*{ 0x02, 0x02, zzzap_snd2_w}, */
	/*{ 0x05, 0x05, zzzap_snd5_w}, */
	{ 0x03, 0x03, invaders_shift_data_w },
	{ 0x04, 0x04, invaders_shift_amount_w },
	/*{ 0x07, 0x07, zzzap_wdog }, */
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( zzzap_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* 4 bit accel */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Crude approximation using 2 buttons */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_TOGGLE ) /* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START      /* IN1 - Steering Wheel */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE | IPF_REVERSE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin / 1 Play" )
	PORT_DIPSETTING(    0x01, "2 Coins / 1 Play" )
	PORT_DIPSETTING(    0x02, "1 Coin / 3 Plays" )
	PORT_DIPSETTING(    0x03, "2 Coins / 3 Plays" )
	PORT_DIPNAME( 0x0c, 0x00, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "80 Sec" )
	PORT_DIPSETTING(    0x04, "Test" )
	PORT_DIPSETTING(    0x08, "99 Sec" )
	PORT_DIPSETTING(    0x0c, "60 Sec" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Score >= 2.5" )
	PORT_DIPSETTING(    0x10, "Score >= 2" )
	PORT_DIPSETTING(    0x20, "None" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0xc0, 0x00, "Language", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "English")
	PORT_DIPSETTING(    0x40, "German")
	PORT_DIPSETTING(    0x80, "French")
	PORT_DIPSETTING(    0xc0, "Spanish")
INPUT_PORTS_END

static struct MachineDriver zzzap_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,zzzap_writeport,
			invaders_interrupt, 2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

struct GameDriver zzzap_driver =
{
	__FILE__,
	0,
	"280zzzap",
	"280-ZZZAP",
	"1976",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&zzzap_machine_driver,

	zzzap_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	zzzap_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Taito "Lupin III"                                   */
/*                                                     */
/*******************************************************/

ROM_START( lupin3_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lp12.bin", 0x0000, 0x0800, 0xe2966d2a )
	ROM_LOAD( "lp13.bin", 0x0800, 0x0800, 0x86e37465 )
	ROM_LOAD( "lp14.bin", 0x1000, 0x0800, 0x365c8a54 )
	ROM_LOAD( "lp15.bin", 0x1800, 0x0800, 0xd962c436 )
	ROM_LOAD( "lp16.bin", 0x4000, 0x0800, 0x3258ce90 )
	ROM_LOAD( "lp17.bin", 0x4800, 0x0800, 0x4178a84a )
	ROM_LOAD( "lp18.bin", 0x5000, 0x0800, 0xfe15569b )
ROM_END

static struct MemoryWriteAddress lupin3_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};
INPUT_PORTS_START( lupin3_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,  IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,  IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "TT" )
	PORT_DIPNAME( 0x08, 0x00, "Check", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPNAME( 0x10, 0x00, "Export", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Export" )
	PORT_DIPSETTING(    0x10, "Japan" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Check", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x80, "No Crush" )
INPUT_PORTS_END


static struct MachineDriver lupin3_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,lupin3_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver lupin3_driver =
{
	__FILE__,
	0,
	"lupin3",
	"Lupin III",
	"1980",
	"Taito",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&lupin3_machine_driver,

	lupin3_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	lupin3_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*******************************************************/
/*                                                     */
/* Nintendo "Heli Fire"                                */
/*                                                     */
/*******************************************************/

ROM_START( helifire_rom )                                                                                /* MJC */
ROM_REGION(0x10000)             /* 64k for code */
        ROM_LOAD( "tub.f1b", 0x0000, 0x0400, 0x78696e01 )
        ROM_LOAD( "tub.f2b", 0x0400, 0x0400, 0x9d67bfa9 )
        ROM_LOAD( "tub.g1b", 0x0800, 0x0400, 0xd422f2ea )
        ROM_LOAD( "tub.g2b", 0x0C00, 0x0400, 0xaedbe96d )
        ROM_LOAD( "tub.h1b", 0x1000, 0x0400, 0x3002ea6e )
        ROM_LOAD( "tub.h2b", 0x1400, 0x0400, 0x830d55e1 )
        ROM_LOAD( "tub.i1b", 0x1800, 0x0400, 0x89f92b4b )
        ROM_LOAD( "tub.i2b", 0x1C00, 0x0400, 0x0c01fe39 )
        ROM_LOAD( "tub.j1b", 0x2000, 0x0400, 0x99a598cd )
        ROM_LOAD( "tub.j2b", 0x2400, 0x0400, 0x687c46a8 )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

        ROM_REGION(0x0800)   /* Sound 8035 + 76477 Sound Generator */
        ROM_LOAD( "TUB.SND", 0x0000, 0x0400, 0x0 )
#endif
ROM_END

INPUT_PORTS_START( helifire_input_ports )

        PORT_START      /* 00 Main Controls */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY  )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

        PORT_START      /* 01 Player 2 Controls */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

        PORT_START      /* Start and Coin Buttons */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Marked for   */
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Expansion    */
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* on Schematic */
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

        PORT_START      /* DSW */
        PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPSETTING(    0x02, "5" )
        PORT_DIPSETTING(    0x03, "6" )
        PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "5000")
        PORT_DIPSETTING(    0x04, "6000")
        PORT_DIPSETTING(    0x08, "8000")
        PORT_DIPSETTING(    0x00, "10000")
        PORT_DIPNAME( 0x10, 0x00, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1 Coin/1 Credit")
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit")
        PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END

struct GameDriver helifire_driver =
{
	__FILE__,
	0,
	"helifire",
	"Heli Fire",
	"1980",
	"Nintendo",
	"The Space Invaders Team",
	0,
	&bandido_machine_driver,

	helifire_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	helifire_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*******************************************************/
/*                                                     */
/* Nintendo "Space Fever (Color)"                      */
/*                                                     */
/*******************************************************/


ROM_START( spacefev_rom )                                                                                /* MJC */
ROM_REGION(0x10000)             /* 64k for code */
        ROM_LOAD( "TSF.F1", 0x0000, 0x0400, 0x63752d87 )
        ROM_LOAD( "TSF.F2", 0x0400, 0x0400, 0xa6251123 )
        ROM_LOAD( "TSF.G1", 0x0800, 0x0400, 0x7ddec16a )
        ROM_LOAD( "TSF.G2", 0x0C00, 0x0400, 0xf4ec1bdc )
        ROM_LOAD( "TSF.H1", 0x1000, 0x0400, 0x95cb6bb7 )
        ROM_LOAD( "TSF.H2", 0x1400, 0x0400, 0x4024227c )
        ROM_LOAD( "TSF.I1", 0x1800, 0x0400, 0x523d359b )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

       ROM_REGION(0x0800)    /* Sound 8035 + 76477 Sound Generator */
       ROM_LOAD( "TSF.SND", 0x0000, 0x0400, 0x0 )
#endif

ROM_END

INPUT_PORTS_START( spacefev_input_ports )

	PORT_START      /* 00 Main Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* 01 Player 2 Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BITX(0x08, 0x00, 0, "Start Game A", OSD_KEY_Q, IP_JOY_NONE, 0 )
	PORT_BITX(0x10, 0x00, 0, "Start Game B", OSD_KEY_W, IP_JOY_NONE, 0 )
	PORT_BITX(0x20, 0x00, 0, "Start Game C", OSD_KEY_E, IP_JOY_NONE, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* If on low the game doesn't start */

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
//	PORT_DIPNAME( 0xfc, 0x00, "Unknown", IP_KEY_NONE )
//	PORT_DIPSETTING(    0x00, "On" )
//	PORT_DIPSETTING(    0xfc, "Off" )
INPUT_PORTS_END

struct GameDriver spacefev_driver =
{
	__FILE__,
	0,
	"spacefev",
	"Space Fever",
	"1980",
	"Nintendo",
	"The Space Invaders Team",
	0,
	&bandido_machine_driver,

	spacefev_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	spacefev_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*******************************************************/
/*                                                     */
/* Taito "Polaris"                                     */
/*                                                     */
/*******************************************************/


ROM_START( polaris_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ps-01", 0x0000, 0x0800, 0x5aa7c985 )
	ROM_LOAD( "ps-09", 0x0800, 0x0800, 0x77363310 )
	ROM_LOAD( "ps-08", 0x1000, 0x0800, 0x00769af6 )
	ROM_LOAD( "ps-04", 0x1800, 0x0800, 0x00a0c624 )
	ROM_LOAD( "ps-05", 0x4000, 0x0800, 0x3602c8f6 )
	ROM_LOAD( "ps-10", 0x4800, 0x0800, 0x7eebdc49 )
ROM_END

static struct MemoryReadAddress polaris_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress polaris_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, boothill_videoram_w, &invaders_videoram },
	{ 0x4000, 0x4fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOReadPort polaris_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort polaris_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( polaris_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )   // Tilt
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )


	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END


static struct MachineDriver polaris_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			polaris_readmem,polaris_writemem,polaris_readport,polaris_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};


struct GameDriver polaris_driver =
{
	__FILE__,
	0,
	"polaris",
	"Polaris",
	"1980",
	"Taito",
	"Space Invaders Team",
	0,
	&polaris_machine_driver,

	polaris_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	polaris_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "Laguna Racer"                               */
/*                                                     */
/*******************************************************/


ROM_START( lagunar_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lagunar.h", 0x0000, 0x0800, 0xba4fa2e9 )
	ROM_LOAD( "lagunar.g", 0x0800, 0x0800, 0x2805c827 )
	ROM_LOAD( "lagunar.f", 0x1000, 0x0800, 0x7d1db18b )
	ROM_LOAD( "lagunar.e", 0x1800, 0x0800, 0xae6e4a0c )
ROM_END

INPUT_PORTS_START( lagunar_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* 4 bit accel */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Crude approximation using 2 buttons */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_TOGGLE ) /* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START      /* IN1 - Steering Wheel */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE | IPF_REVERSE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin / 1 Play" )
	PORT_DIPSETTING(    0x01, "1 Coins / 2 Plays" )
	PORT_DIPSETTING(    0x02, "2 Coins / 1 Play" )
	PORT_DIPSETTING(    0x03, "2 Coins / 3 Plays" )
	PORT_DIPNAME( 0x0c, 0x0c, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45 Sec" )
	PORT_DIPSETTING(    0x04, "60 Sec" )
	PORT_DIPSETTING(    0x08, "75 Sec" )
	PORT_DIPSETTING(    0x0c, "90 Sec" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "350" )
	PORT_DIPSETTING(    0x10, "400" )
	PORT_DIPSETTING(    0x20, "450" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0xc0, 0x00, "Test Modes", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x40, "RAM/ROM")
	PORT_DIPSETTING(    0x80, "Steering")
	PORT_DIPSETTING(    0xc0, "No Extended Play")
INPUT_PORTS_END

struct GameDriver lagunar_driver =
{
	__FILE__,
	0,
	"lagunar",
	"Laguna Racer",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&zzzap_machine_driver,

	lagunar_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	lagunar_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_90,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "M-4"                                        */
/*                                                     */
/*******************************************************/


ROM_START( m4_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "m4.h", 0x0000, 0x0800, 0x108cb28c )
	ROM_LOAD( "m4.g", 0x0800, 0x0800, 0xd14beb73 )
	ROM_LOAD( "m4.f", 0x1000, 0x0800, 0x835e5ea2 )
	ROM_LOAD( "m4.e", 0x1800, 0x0800, 0x728dfb2b )
ROM_END

static struct IOReadPort m4_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( m4_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_UP | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_DOWN | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 ) /* Left trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 ) /* Left reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_UP | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )  /* 1 player */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_DOWN | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON3 ) /* Right trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON4 ) /* Right reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START2 )  /* 2 player */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  / Player" )
	PORT_DIPSETTING(    0x01, "1 Coin  / 2 Players" )
	PORT_DIPSETTING(    0x02, "2 Coins / Player" )
	PORT_DIPSETTING(    0x03, "2 Coins / 2 Players" )
	PORT_DIPNAME( 0x0C, 0x0C, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x04, "70" )
	PORT_DIPSETTING(    0x08, "80" )
	PORT_DIPSETTING(    0x0C, "90" )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct MachineDriver m4_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, m4_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver m4_driver =
{
	__FILE__,
	0,
	"m4",
	"M-4",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&m4_machine_driver,

	m4_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	m4_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Phantom II"                                 */
/*                                                     */
/* To Do : clouds                                      */
/*                                                     */
/*******************************************************/


ROM_START( phantom2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "phantom2.h", 0x0000, 0x0800, 0xb79c5ae8 )
	ROM_LOAD( "phantom2.g", 0x0800, 0x0800, 0xe26cf606 )
	ROM_LOAD( "phantom2.f", 0x1000, 0x0800, 0xbca599dd )
	ROM_LOAD( "phantom2.e", 0x1800, 0x0800, 0x86ac9c28 )
ROM_END

static struct IOReadPort phantom2_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( phantom2_input_ports )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )                    /* Fire */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x01, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  - 1 Player" )
	PORT_DIPSETTING(    0x01, "1 Coin  - 2 Players" )
	PORT_DIPNAME( 0x06, 0x06, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45sec 20sec 20" )
	PORT_DIPSETTING(    0x02, "60sec 25sec 25" )
	PORT_DIPSETTING(    0x04, "75sec 30sec 30" )
	PORT_DIPSETTING(    0x06, "90sec 35sec 35" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE )                   /* Test */

INPUT_PORTS_END

static struct MachineDriver phantom2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, phantom2_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver phantom2_driver =
{
	__FILE__,
	0,
	"phantom2",
	"Phantom II",
	"1979",
	"Midway",
	"The Space Invaders Team",
	0,
	&phantom2_machine_driver,

	phantom2_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	phantom2_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Dog Patch"                                  */
/*                                                     */
/*******************************************************/


static struct IOReadPort dogpatch_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort dogpatch_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ 0x03, 0x03, invaders_dummy_write },
	{ 0x04, 0x04, invaders_dummy_write },
	{ 0x05, 0x05, invaders_dummy_write },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( dogpatch_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_X, 25, 0, 0x01, 0x3e) /* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "# Cans", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPSETTING(    0x01, "20" )
	PORT_DIPSETTING(    0x10, "15" )
	PORT_DIPSETTING(    0x11, "10" )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1C / 1P" )
	PORT_DIPSETTING(    0x04, "2C / 1P" )
	PORT_DIPSETTING(    0x08, "1C / 2P" )
	PORT_DIPSETTING(    0x0c, "2C / 3P" )
	PORT_DIPNAME( 0x10, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 extra cans" )
	PORT_DIPSETTING(    0x10, "3 extra cans" )
	PORT_DIPNAME( 0x20, 0x20, "Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPNAME( 0xc0, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "275 Pts")
	PORT_DIPSETTING(    0x01, "225 Pts")
	PORT_DIPSETTING(    0x10, "175 Pts")
	PORT_DIPSETTING(    0x11, "150 Pts")
INPUT_PORTS_END

static struct MachineDriver dogpatch_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,dogpatch_readport,dogpatch_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

ROM_START( dogpatch_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "dogpatch.h", 0x0000, 0x0800, 0x8ccfbdc1 )
	ROM_LOAD( "dogpatch.g", 0x0800, 0x0800, 0xb0a54573 )
	ROM_LOAD( "dogpatch.f", 0x1000, 0x0800, 0x482757fb )
	ROM_LOAD( "dogpatch.e", 0x1800, 0x0800, 0x53eb8b79 )
ROM_END

struct GameDriver dogpatch_driver =
{
	__FILE__,
	0,
	"dogpatch",
	"Dog Patch",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&dogpatch_machine_driver,

	dogpatch_rom,
	0, 0,
	0, 0,      /* sound_prom */

	dogpatch_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "4 Player Bowling"                           */
/*                                                     */
/*******************************************************/


static struct IOReadPort midwbowl_readport[] =
{
	{ 0x01, 0x01, midbowl_shift_data_r },
	{ 0x02, 0x02, input_port_0_r },				/* dip switch */
	{ 0x03, 0x03, midbowl_shift_data_rev_r },
	{ 0x04, 0x04, input_port_1_r },				/* coins / switches */
	{ 0x05, 0x05, input_port_2_r },				/* ball vert */
	{ 0x06, 0x06, input_port_3_r },				/* ball horz */
	{ -1 }  /* end of table */
};

static struct IOWritePort midwbowl_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

ROM_START( midwbowl_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "h.cpu", 0x0000, 0x0800, 0x5600082e )
	ROM_LOAD( "g.cpu", 0x0800, 0x0800, 0x796cd88e )
	ROM_LOAD( "f.cpu", 0x1000, 0x0800, 0x9d8d4183 )
	ROM_LOAD( "e.cpu", 0x1800, 0x0800, 0x1b418df5 )
	ROM_LOAD( "d.cpu", 0x4000, 0x0800, 0x122f0db1 )
ROM_END

INPUT_PORTS_START( midwbowl_input_ports )
	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "German" )
	PORT_DIPSETTING(    0x03, "German" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1C/1P" )
	PORT_DIPSETTING(    0x10, "2C/1P" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* effects button 1 */
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x80, "Test Mode")

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN5 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 3, 0, 0)

	PORT_START      /* IN6 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_X, 10, 3, 0, 0)
INPUT_PORTS_END

static struct MachineDriver midwbowl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz */
			0,
			readmem, boothill_writemem, midwbowl_readport, midwbowl_writeport,
			invaders_interrupt, 2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
};

struct GameDriver midwbowl_driver =
{
	__FILE__,
	0,
	"bowler",
	"4 Player Bowling",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&midwbowl_machine_driver,

	midwbowl_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	midwbowl_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_90,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Blue Shark"                                 */
/*                                                     */
/*******************************************************/


ROM_START( blueshrk_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "blueshrk.h", 0x0000, 0x0800, 0x06bc9fde )
	ROM_LOAD( "blueshrk.g", 0x0800, 0x0800, 0xf263595b )
	ROM_LOAD( "blueshrk.f", 0x1000, 0x0800, 0x9d262302 )
ROM_END

INPUT_PORTS_START( blueshrk_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0x1f, 0x01, IPT_PADDLE, 100, 0, 0x01, 0xfe)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x01, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  - 1 Player" )
	PORT_DIPSETTING(    0x01, "2 Coins  - 1 Player" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x60, 0x00, "Replay", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x20, "14000" )
	PORT_DIPSETTING(    0x40, "18000" )
	PORT_DIPSETTING(    0x60, "22000" )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x80, "Test Mode")
INPUT_PORTS_END

static struct MachineDriver blueshrk_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, writemem, readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
};

struct GameDriver blueshrk_driver =
{
	__FILE__,
	0,
	"blueshrk",
	"Blue Shark",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&blueshrk_machine_driver,

	blueshrk_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	blueshrk_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};


/*******************************************************/
/*                                                     */
/* Midway "Extra Innings"                              */
/*                                                     */
/*******************************************************/


ROM_START( einnings_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "EI.H", 0x0000, 0x0800, 0xbabc2556 )
	ROM_LOAD( "EI.G", 0x0800, 0x0800, 0x1671923d )
	ROM_LOAD( "EI.F", 0x1000, 0x0800, 0x1a16c90a )
	ROM_LOAD( "EI.E", 0x1800, 0x0800, 0x8cb3c289 )
	ROM_LOAD( "EI.B", 0x5000, 0x0800, 0xb25116b9 )
ROM_END

/*
 * The cocktail version has independent bat, pitch, and field controls
 * while the upright version ties the pairs of inputs together through
 * jumpers in the wiring harness.
 */
INPUT_PORTS_START( einnings_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )			/* home bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )	/* home fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT)	/* home fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT)	/* home pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT)	/* home pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN)  /* home pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP)    /* home pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )			/* vistor bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )	/* vistor fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT) 	/* visitor fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT)	/* visitor pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT)	/* visitor pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN)  /* visitor pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP)    /* visitor pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x07, 0x00, "Coins / Inning", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "1c / inning")
	PORT_DIPSETTING(    0x02, "2c / inning")
	PORT_DIPSETTING(    0x01, "1c/1inning 2c/3 innings")
	PORT_DIPNAME( 0x40, 0x40, "Test", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x40, "Off")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

struct GameDriver einnings_driver =
{
	__FILE__,
	0,
	"einnings",
	"Extra Innings",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&boothill_machine_driver,

	einnings_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	einnings_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Double Play"                                */
/*                                                     */
/*******************************************************/


ROM_START( dplay_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "dplay619.h", 0x0000, 0x0800, 0xa5e4d700 )
	ROM_LOAD( "dplay619.g", 0x0800, 0x0800, 0xd7a41b56 )
	ROM_LOAD( "dplay619.f", 0x1000, 0x0800, 0xcc87b48b )
	ROM_LOAD( "dplay619.e", 0x1800, 0x0800, 0x741f64f3 )
ROM_END


struct GameDriver dplay_driver =
{
	__FILE__,
	0,
	"dplay",
	"Double Play",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&boothill_machine_driver,

	dplay_rom,
	0, 0,
	0, 0,      /* sound_prom */

	einnings_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};


/*******************************************************/
/*                                                     */
/* Midway "Amazing Maze"                               */
/*                                                     */
/*******************************************************/

ROM_START( maze_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h", 0x0000, 0x0800, 0xa96063c2 )
	ROM_LOAD( "invaders.g", 0x0800, 0x0800, 0x784e056a )
ROM_END

INPUT_PORTS_START( maze_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02,"On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct MachineDriver maze_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

/* L.T 11/3/1998 */
struct GameDriver maze_driver =
{
	__FILE__,
	0,
	"maze",
	"The Amazing Maze Game",
	"1976",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&maze_machine_driver,

	maze_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	maze_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Tornado Baseball"                           */
/*                                                     */
/*******************************************************/

ROM_START( tornbase_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "tb.h", 0x0000, 0x0800, 0x00ee84b4 )
	ROM_LOAD( "tb.g", 0x0800, 0x0800, 0xb9fedbe4 )
	ROM_LOAD( "tb.f", 0x1000, 0x0800, 0xd512b59c )
ROM_END

INPUT_PORTS_START( tornbase_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_START      /* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x78, 0x40, "Coins / Rounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 / 1" )
	PORT_DIPSETTING(    0x08, "2 / 1" )
	PORT_DIPSETTING(    0x10, "3 / 1" )
	PORT_DIPSETTING(    0x18, "4 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0x28, "2 / 2" )
	PORT_DIPSETTING(    0x30, "3 / 2" )
	PORT_DIPSETTING(    0x38, "4 / 2" )
	PORT_DIPSETTING(    0x40, "1 / 4" )
	PORT_DIPSETTING(    0x48, "2 / 4" )
	PORT_DIPSETTING(    0x50, "3 / 4" )
	PORT_DIPSETTING(    0x58, "4 / 4" )
	PORT_DIPSETTING(    0x60, "1 / 9" )
	PORT_DIPSETTING(    0x68, "2 / 9" )
	PORT_DIPSETTING(    0x70, "3 / 9" )
	PORT_DIPSETTING(    0x78, "4 / 9" )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct MachineDriver tornbase_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

/* LT 20-2-1998 */
struct GameDriver tornbase_driver =
{
	__FILE__,
	0,
	"tornbase",
	"Tornado Baseball",
	"1976",
	"Midway",
	"Lee Taylor\n",
	0,
	&tornbase_machine_driver,

	tornbase_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	tornbase_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

