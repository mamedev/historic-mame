/***************************************************************************

Circus memory map

0000-00FF Base Page RAM
0100-01FF Stack RAM
1000-1FFF ROM
2000      Clown Verticle Position
3000      Clown Horizontal Position
4000-43FF Video RAM
8000      Clown Rotation and Audio Controls
F000-FFF7 ROM
FFF8-FFFF Interrupt and Reset Vectors

A000      Control Switches
C000      Option Switches
D000      Paddle Position and Interrupt Reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void circus_clown_x_w(int offset, int data);
void circus_clown_y_w(int offset, int data);
void circus_clown_z_w(int offset, int data);


void crash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void circus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void robotbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int crash_interrupt(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01FF, MRA_RAM },
	{ 0x4000, 0x43FF, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xD000, 0xD000, input_port_2_r },
	{ 0xA000, 0xA000, input_port_0_r },
	{ 0xC000, 0xC000, input_port_1_r }, /* DSW */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x2000, 0x2000, circus_clown_x_w },
	{ 0x3000, 0x3000, circus_clown_y_w },
	{ 0x8000, 0x8000, circus_clown_z_w },
	{ 0x1000, 0x1fff, MWA_ROM },
	{ 0xF000, 0xFFFF, MWA_ROM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x00, "Jumps", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x0C, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Player - 1 Coin" )
	PORT_DIPSETTING(    0x04, "1 Player - 1 Coin" )
	PORT_DIPSETTING(    0x08, "1 Player - 2 Coin" )
	PORT_DIPNAME( 0x10, 0x00, "Top Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Single Line" )
	PORT_DIPSETTING(    0x20, "Super Bonus" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START      /* IN2 - paddle */
	PORT_ANALOG ( 0xff, 115, IPT_PADDLE, 50, 0, 64, 167 )
INPUT_PORTS_END

static unsigned char palette[] = /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0x20, /* YELLOW */
	0x20,0xff,0x20, /* GREEN */
	0x20,0x20,0xFF, /* BLUE */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x20,0x20, /* RED */
};

static unsigned short colortable[] =
{
	0,0,
	0,1,
	0,2,
	0,3,
	0,4,
	0,5
};


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout clownlayout =
{
	16,16,  /* 16*16 characters */
	16,             /* 16 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  16*8, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16   /* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 5 },
	{ 1, 0x0800, &clownlayout, 0, 5 },
	{ -1 } /* end of array */
};



static struct DACinterface dac_interface =
{
	1,
	441000,
	{ 255, 255 },
	{  1,  1 } /* filter rate (rate = Register(ohm)*Capaciter(F)*1000000) */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,        /* Slow down a bit */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	circus_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/***************************************************************************

 Circus

***************************************************************************/

ROM_START( circus_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "CIRCUS.1A", 0x1000, 0x0200, 0xa00e7662 ) /* Code */
	ROM_LOAD( "CIRCUS.2A", 0x1200, 0x0200, 0xd22eb9bc )
	ROM_LOAD( "CIRCUS.3A", 0x1400, 0x0200, 0xe91e9440 )
	ROM_LOAD( "CIRCUS.5A", 0x1600, 0x0200, 0x07a49052 )
	ROM_LOAD( "CIRCUS.6A", 0x1800, 0x0200, 0x91cc58b4 )
	ROM_LOAD( "CIRCUS.7A", 0x1A00, 0x0200, 0xa543ddd3 )
	ROM_LOAD( "CIRCUS.8A", 0x1C00, 0x0200, 0xa95bf207 )
	ROM_LOAD( "CIRCUS.9A", 0x1E00, 0x0200, 0xfddf3b77 )
	ROM_RELOAD(            0xFE00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION(0x0A00)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CIRCUS.1C", 0x0600, 0x0200, 0x452b3c39 )     /* Character Set */
	ROM_LOAD( "CIRCUS.2C", 0x0400, 0x0200, 0xdab9d9e3 )
	ROM_LOAD( "CIRCUS.3C", 0x0200, 0x0200, 0xc0e43dd4 )
	ROM_LOAD( "CIRCUS.4C", 0x0000, 0x0200, 0xe62e88d0 )
	ROM_LOAD( "CIRCUS.14D", 0x0800, 0x0200, 0xb6e09ca0 ) /* Clown */

ROM_END


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0036],"\x00\x00",2) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0036],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0036],2);
		osd_fclose(f);
	}
}


struct GameDriver circus_driver =
{
	__FILE__,
	0,
	"circus",
	"Circus",
	"1977",
	"Exidy",
	"Mike Coates (MAME driver)\nValerio Verrando (high score save)",
	0,
	&machine_driver,

	circus_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload,hisave
};

/***************************************************************************

 Robot Bowl

***************************************************************************/

ROM_START( robotbowl_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROBOTBWL.1A", 0xF000, 0x0200, 0x4cdc172c ) /* Code */
	ROM_LOAD( "ROBOTBWL.2A", 0xF200, 0x0200, 0x35c12993 )
	ROM_LOAD( "ROBOTBWL.3A", 0xF400, 0x0200, 0x80e1b1c1 )
	ROM_LOAD( "ROBOTBWL.5A", 0xF600, 0x0200, 0xa14ca8e4 )
	ROM_LOAD( "ROBOTBWL.6A", 0xF800, 0x0200, 0x6d84dbc2 )
	ROM_LOAD( "ROBOTBWL.7A", 0xFA00, 0x0200, 0xd7a66e30 )
	ROM_LOAD( "ROBOTBWL.8A", 0xFC00, 0x0200, 0xd2d9c2fd )
	ROM_LOAD( "ROBOTBWL.9A", 0xFE00, 0x0200, 0x2f416fa5 )

	ROM_REGION(0x0A00)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ROBOTBWL.1C", 0x0600, 0x0200, 0x0a3e3466 )     /* Character Set */
	ROM_LOAD( "ROBOTBWL.2C", 0x0400, 0x0200, 0xefaa87b4 )
	ROM_LOAD( "ROBOTBWL.3C", 0x0200, 0x0200, 0xdfb1d383 )
	ROM_LOAD( "ROBOTBWL.4C", 0x0000, 0x0200, 0x0efd8a75 )
	ROM_LOAD( "ROBOTBWL.14D", 0x0800, 0x0020, 0xab9c2424 ) /* Ball */

ROM_END

INPUT_PORTS_START( robotbowl_input_ports )

	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0,"Hook Right", OSD_KEY_X, 0, 0 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0,"Hook Left", OSD_KEY_Z, 0, 0 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x60, 0x00, "Bowl Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x40, "7" )
	PORT_DIPSETTING(    0x60, "9" )
	PORT_DIPNAME( 0x18, 0x08, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Player - 1 Coin" )
	PORT_DIPSETTING(    0x08, "1 Player - 1 Coin" )
	PORT_DIPSETTING(    0x10, "1 Player - 2 Coin" )
	PORT_DIPNAME( 0x04, 0x04, "Beer Frame", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END

static struct GfxLayout robotlayout =
{
	8,8,  /* 16*16 characters */
	1,      /* 1 character */
	1,      /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8
};

static struct GfxDecodeInfo robotbowl_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 5 },
	{ 1, 0x0800, &robotlayout, 0, 5 },
	{ -1 } /* end of array */
};

static void robotbowl_decode(void)
{
	/* PROM is reversed, fix it! */

	int Count;

    for(Count=31;Count>=0;Count--) Machine->memory_region[1][0x800+Count]^=0xFF;
}

static struct MachineDriver robotbowl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1411125,        /* 11.289 / 8 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	robotbowl_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	robotbowl_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

struct GameDriver robotbwl_driver =
{
	__FILE__,
	0,
	"robotbwl",
	"Robot Bowl",
	"1977",
	"Exidy",
	"Mike Coates",
	0,
	&robotbowl_machine_driver,

	robotbowl_rom,
	robotbowl_decode, 0,
	0,
	0,      /* sound_prom */

	robotbowl_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0,0
};

/***************************************************************************

 Crash

***************************************************************************/

ROM_START( crash_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "CRASH.A1", 0x1000, 0x0200, 0xb43221da ) /* Code */
	ROM_LOAD( "CRASH.A2", 0x1200, 0x0200, 0x7eef07ef )
	ROM_LOAD( "CRASH.A3", 0x1400, 0x0200, 0xdb35fe5d )
	ROM_LOAD( "CRASH.A4", 0x1600, 0x0200, 0x99c59ef9 )
	ROM_LOAD( "CRASH.A5", 0x1800, 0x0200, 0xdbd7928f )
	ROM_LOAD( "CRASH.A6", 0x1A00, 0x0200, 0x7f79c719 )
	ROM_LOAD( "CRASH.A7", 0x1C00, 0x0200, 0x26c42afa )
	ROM_LOAD( "CRASH.A8", 0x1E00, 0x0200, 0xd79d983b )
	ROM_RELOAD(            0xFE00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION(0x0A00)      /* temporary space for graphics */
	ROM_LOAD( "CRASH.C1", 0x0600, 0x0200, 0xc05ac420 )  /* Character Set */
	ROM_LOAD( "CRASH.C2", 0x0400, 0x0200, 0xd82dddbb )
	ROM_LOAD( "CRASH.C3", 0x0200, 0x0200, 0xa595e3c7 )
	ROM_LOAD( "CRASH.C4", 0x0000, 0x0200, 0xdbccbdbc )
	ROM_LOAD( "CRASH.D14",0x0800, 0x0200, 0xbd1e3d80 ) /* Cars */
ROM_END

INPUT_PORTS_START( crash_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0C, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x10, 0x00, "Top Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static int crash_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (RAM[0x004B] != 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x000F],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void crash_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x000F],2);
		osd_fclose(f);
	}
}

static struct MachineDriver crash_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1411125,        /* 11.289 / 8 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,2
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	crash_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

struct GameDriver crash_driver =
{
	__FILE__,
	0,
	"crash",
	"Crash",
	"1979",
	"Exidy",
	"Mike Coates",
	0,
	&crash_machine_driver,

	crash_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	crash_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	crash_hiload,crash_hisave
};
