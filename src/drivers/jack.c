/***************************************************************************

Jack the Giant Killer memory map (preliminary)

b400 - sound command? 0x1d = self-test?

Sound CPU appears to run in interrupt mode 1

Known problems:
	* Sound doesn't work. No clue why.
	* The palette seems to be wrong when you die. Should the beans change color?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *jack_paletteram;

void jack_paletteram_w(int offset,int data);
void jack_vh_screenrefresh(struct osd_bitmap *bitmap);

void jack_sh_command_w(int offset,int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,0xff);
}


/* I'm using a reworking of the Scramble timer here - it could be TOTALLY WRONG */
static int jack_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

int jack_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 5120;

	last_totalcycles = current_totalcycles;

	return jack_timer[clock/256];
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xc000, 0xffff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x62ff, MRA_RAM },
	{ 0xb500, 0xb500, input_port_0_r },
	{ 0xb501, 0xb501, input_port_1_r },
	{ 0xb502, 0xb502, input_port_2_r },
	{ 0xb503, 0xb503, input_port_3_r },
	{ 0xb504, 0xb504, input_port_4_r },
	{ 0xb505, 0xb505, input_port_5_r },
//	{ 0xb506, 0xb506, b506_r },
	{ 0xb000, 0xb07f, MRA_RAM },
	{ 0xb800, 0xbfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x62ff, MWA_RAM },
	{ 0xb000, 0xb07f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb400, 0xb400, jack_sh_command_w },
	{ 0xb600, 0xb61f, jack_paletteram_w, &jack_paletteram },
	{ 0xb800, 0xbbff, videoram_w, &videoram, &videoram_size },
	{ 0xbc00, 0xbfff, colorram_w, &colorram },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Every 10000" )
	PORT_DIPSETTING(    0x20, "10000 Only" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Start Level 1" )
	PORT_DIPSETTING(    0x40, "Start Level 5" )
	PORT_DIPNAME( 0x80, 0x00, "Number of Beans", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "x1" )
	PORT_DIPSETTING(    0x80, "x2" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BITX ( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BITX ( 0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x80, "On" )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* ?? */

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* ?? */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 1024*8*8 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 16 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 8 },
	{ -1 } /* end of array */
};



/* Sound stuff */
static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz?????? */
	{ 0x20ff },
	{ soundlatch_r },
	{ jack_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 32,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	jack_vh_screenrefresh,

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

ROM_START( jack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jgk.j8", 0x0000, 0x1000, 0x27e0c818 )
	ROM_LOAD( "jgk.j6", 0x1000, 0x1000, 0x97ab32d9 )
	ROM_LOAD( "jgk.j7", 0x2000, 0x1000, 0x4622cc4a )
	ROM_LOAD( "jgk.j5", 0x3000, 0x1000, 0xbfda4dce )
	ROM_LOAD( "jgk.j3", 0xc000, 0x1000, 0xd4cad17e )
	ROM_LOAD( "jgk.j4", 0xd000, 0x1000, 0x9c7a8886 )
	ROM_LOAD( "jgk.j2", 0xe000, 0x1000, 0xe1862056 )
	ROM_LOAD( "jgk.j1", 0xf000, 0x1000, 0x832bfd35 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jgk.j12",  0x0000, 0x1000, 0x6d506420 )
	ROM_LOAD( "jgk.j13",  0x1000, 0x1000, 0x31b999a7 )
	ROM_LOAD( "jgk.j11",  0x2000, 0x1000, 0x08957111 )
	ROM_LOAD( "jgk.j10",  0x3000, 0x1000, 0xecc2d6ac )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "jgk.j9", 0x0000, 0x1000, 0x7b878165 )
ROM_END



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x04506],"\x41\x41\x41",3) == 0 &&
			memcmp(&RAM[0x4560],"\x41\x41\x41",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4500],9*11);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4500],9*11);
		osd_fclose(f);
	}
}



struct GameDriver jack_driver =
{
	"Jack the Giant Killer",
	"jack",
	"Brad Oliver",
	&machine_driver,

	jack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
