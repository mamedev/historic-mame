/***************************************************************************

The Pit/Round Up memory map (preliminary)


Main CPU:

0000-4fff ROM
8000-83ff RAM
8800-8bff Color RAM
8c00-8fff Mirror for above
9000-93ff Video RAM
9400-97ff Mirror for above
9800-983f Attributes RAM
9840-985f Sprite RAM

Read:

a000      Input Port 0
a800      Input Port 1
b000      DIP Switches
b800      Watchdog Reset

Write:

b000      NMI Enable
b002      Marked as LOCKOUT, but I can't find where it goes to
b003	  Sound Enable
b006      Flip Screen X
b007      Flip Screen Y
b800      Sound Command


Sound CPU:

0000-0fff ROM  (0000-07ff in The Pit)
3800-3bff RAM


Port I/O Read:

8f  AY8910 Read Port


Port I/O Write:

00  Reset Sound Command
8e  AY8910 Control Port
8f  AY8910 Write Port


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *galaxian_attributesram;
void galaxian_attributes_w(int offset,int data);

void thepit_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void thepit_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void thepit_flipx_w(int offset,int data);
void thepit_flipy_w(int offset,int data);
int  thepit_input_port_0_r(int offset);
void thepit_sound_enable_w(int offset, int data);
void thepit_AY8910_0_w(int offset, int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x8800, 0x98ff, MRA_RAM },
	{ 0xa000, 0xa000, thepit_input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, colorram_w },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x98ff, MWA_NOP }, // Probably unused
	{ 0xa000, 0xa000, MWA_NOP }, // Not hooked up according to the schematics
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb001, 0xb001, MWA_NOP }, // Unused, but initialized
	{ 0xb002, 0xb002, MWA_NOP }, // See memory map
	{ 0xb003, 0xb003, thepit_sound_enable_w },
	{ 0xb004, 0xb005, MWA_NOP }, // Unused, but initialized
	{ 0xb006, 0xb006, thepit_flipx_w, &flip_screen_x },
	{ 0xb007, 0xb007, thepit_flipy_w, &flip_screen_y },
	{ 0xb800, 0xb800, soundlatch_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x3800, 0x3bff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x8f, 0x8f, AY8910_read_port_0_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x8e, 0x8f, thepit_AY8910_0_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( thepit_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Coinage P1/P2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1Cr/2Cr" )
	PORT_DIPSETTING(    0x01, "2Cr/3Cr" )
	PORT_DIPSETTING(    0x02, "2Cr/4Cr" )
	PORT_DIPSETTING(    0x03, "Free Play" )
	PORT_DIPNAME( 0x04, 0x00, "Game Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPNAME( 0x08, 0x00, "Time Limit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Long" )
	PORT_DIPSETTING(    0x08, "Short" )
	PORT_DIPNAME( 0x10, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( roundup_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life At", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10,000" )
	PORT_DIPSETTING(    0x20, "30,000" )
	PORT_DIPNAME( 0x40, 0x40, "Gly Boys Wake Up", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 0x0800*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,     /* 64 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 0x0800*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,      /* 1 chip */
	18432000/12,     /* 1.536Mhz */
	{ 255 },
	{ soundlatch_r },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,     /* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			10000000/4,     /* 2.5 Mhz */
			3,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,32,
	thepit_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	thepit_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( thepit_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "p38b",       0x0000, 0x1000, 0xe4348d92 )
	ROM_LOAD( "p39b",       0x1000, 0x1000, 0x8d281512 )
	ROM_LOAD( "p40b",       0x2000, 0x1000, 0x8123601b )
	ROM_LOAD( "p41b",       0x3000, 0x1000, 0x8d962efe )
	ROM_LOAD( "p33b",       0x4000, 0x1000, 0x04776851 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "p8",         0x0000, 0x0800, 0x878e996c )
	ROM_LOAD( "p9",         0x0800, 0x0800, 0x57841574 )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "pitclr.ic4", 0x0000, 0x0020, 0x480d8e59 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "p30",        0x0000, 0x0800, 0xc01d290d )
ROM_END

ROM_START( roundup_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "roundup.u38",0x0000, 0x1000, 0xcb0ec56c )
	ROM_LOAD( "roundup.u39",0x1000, 0x1000, 0xc327c76b )
	ROM_LOAD( "roundup.u40",0x2000, 0x1000, 0x3f57c585 )
	ROM_LOAD( "roundup.u41",0x3000, 0x1000, 0x3bd6e3ce )
	ROM_LOAD( "roundup.u33",0x4000, 0x1000, 0xde80606e )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "roundup.u10",0x0000, 0x0800, 0xbc970d0d )
	ROM_LOAD( "roundup.u9", 0x0800, 0x0800, 0xe9351d81 )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "roundup.clr",0x0000, 0x0020, 0x480d8e59 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "roundup.u30",0x0000, 0x0800, 0xf51031e2 )
	ROM_LOAD( "roundup.u31",0x0800, 0x0800, 0xfcb31285 )
ROM_END


static int thepit_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8297],"\x16",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8039],0x0f);
			osd_fread(f,&RAM[0x8280],0x20);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void thepit_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8039],0x0f);
		osd_fwrite(f,&RAM[0x8280],0x20);
		osd_fclose(f);
	}
}


static int roundup_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (videoram_r(0x0181) == 0x00 &&
		videoram_r(0x01a1) == 0x24)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int pos;

			osd_fread(f,&RAM[0x8050],0x03);
			osd_fclose(f);

			/* Copy it to the screen and remove leading zeroes */
			videoram_w(0x0221, RAM[0x8050] >> 4);
			videoram_w(0x0201, RAM[0x8050] & 0x0f);
			videoram_w(0x01e1, RAM[0x8051] >> 4);
			videoram_w(0x01c1, RAM[0x8051] & 0x0f);
			videoram_w(0x01a1, RAM[0x8052] >> 4);
			videoram_w(0x0181, RAM[0x8052] & 0x0f);

			for (pos = 0x0221; pos >= 0x01a1; pos -= 0x20 )
			{
				if (videoram_r(pos) != 0x00) break;

				videoram_w(pos, 0x24);
			}
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void roundup_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8050],0x03);
		osd_fclose(f);
	}
}



struct GameDriver thepit_driver =
{
	__FILE__,
	0,
	"thepit",
	"The Pit",
	"1982",
	"Centuri",
	"Zsolt Vasvari",
	0,
	&machine_driver,

	thepit_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	thepit_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	thepit_hiload, thepit_hisave
};


struct GameDriver roundup_driver =
{
	__FILE__,
	0,
	"roundup",
	"Round-Up",
	"1981",
	"Amenip/Centuri",
	"Zsolt Vasvari",
	0,
	&machine_driver,

	roundup_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	roundup_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	roundup_hiload, roundup_hisave
};
