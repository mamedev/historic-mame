/***************************************************************************

The Pit memory map (preliminary)


Main CPU:

0000-4fff ROM
8000-83ff RAM
8800-8bff Color RAM
9000-93ff Video RAM
9840-985f Sprite RAM

Read:

a000      Input Port 1
a800      Input Port 2
b000      DIP Switches
b800      Watchdog Reset

Write:

b000      NMI Enable
b003      ? (0 during demo mode, 1 otherwise)
b006      Flip Screen X
b007      Flip Screen Y
b800      Sound Command


Sound CPU:

0000-07ff ROM
3800-3bff RAM

Read:

281c-281d ???

Port I/O Read:

8f  AY8910 Read Port


Port I/O Write:

00  Clear Sound Command
8e  AY8910 Control Port
8f  AY8910 Write Port


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void thepit_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void thepit_vh_screenrefresh(struct osd_bitmap *bitmap);
void thepit_flipx_w(int offset,int data);
void thepit_flipy_w(int offset,int data);


static void soundlatch_clear_w(int offset, int data)
{
    // Is there a better way to clear the latch?
    // Doing it this way generates a lot of log entries
    soundlatch_w(0,0);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x8800, 0x8bff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, MWA_NOP }, // Probably unused
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_NOP }, // Probably unused
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb003, 0xb003, MWA_NOP }, // ???
	{ 0xb006, 0xb006, thepit_flipx_w, &flip_screen_x },
	{ 0xb007, 0xb007, thepit_flipy_w, &flip_screen_y },
	{ 0xb800, 0xb800, soundlatch_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x281c, 0x281d, MWA_NOP },  // ???
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
	{ 0x8e, 0x8e, AY8910_control_port_0_w },
	{ 0x8f, 0x8f, AY8910_write_port_0_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
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
	PORT_DIPNAME( 0x03, 0x00, "Coinage (P1/P2)", IP_KEY_NONE )
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



static unsigned char color_prom[] =
{
    0x00,0x38,0x17,0xf0,0x00,0x06,0xdb,0xf0,0x00,0x37,0x07,0xf7,0x00,0x37,0x07,0xd7,
    0x00,0xf0,0xf7,0xd7,0x00,0xe8,0x8a,0x88,0x00,0x37,0xd7,0x07,0x00,0x07,0xe8,0x37
};



static struct AY8910interface ay8910_interface =
{
	1,      /* 1 chip */
	14318000/8,     /* ? */
	{ 0x60ff }, /* ? */
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
			18432000/6,     /* 3.072 Mhz ? */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,     /* ? */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
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
	ROM_LOAD( "p38b", 0x0000, 0x1000, 0xe4348d92 )
	ROM_LOAD( "p39b", 0x1000, 0x1000, 0x8d281512 )
	ROM_LOAD( "p40b", 0x2000, 0x1000, 0x8123601b )
	ROM_LOAD( "p41b", 0x3000, 0x1000, 0x8d962efe )
	ROM_LOAD( "p33b", 0x4000, 0x1000, 0x04776851 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "p8",   0x0000, 0x0800, 0x878e996c )
	ROM_LOAD( "p9",   0x0800, 0x0800, 0x57841574 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "p30",  0x0000, 0x0800, 0xc00d291d )
ROM_END



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


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



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8039],0x0f);
		osd_fwrite(f,&RAM[0x8280],0x20);
		osd_fclose(f);
	}
}



struct GameDriver thepit_driver =
{
	"The Pit",
	"thepit",
	"Zsolt Vasvari",
	&machine_driver,

	thepit_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
