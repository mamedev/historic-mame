/*
    Driver For DECO   ASTRO FIGHTER

    Initial Version

    Lee Taylor 28/11/1997


To Do!!
       Sound!
       Correct Colours (cuurent are best guess from memory}
       There is a upright/cocktail dip switch, but I don't know where it is

Also....
        I know there must be at least one other rom set for this game
        I have played one that stoped between waves to show the next enemy
*/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *astrof_color;


int  astrof_vh_start(void);
void astrof_vh_stop(void);
void astrof_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void astrof_videoram_w(int offset,int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x8003, 0x8003, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },	/* IN1 */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x4000, 0x5fff, astrof_videoram_w, &videoram, &videoram_size },
	{ 0x8003, 0x8003, MWA_RAM, &astrof_color },
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



/***************************************************************************

  Astro Fighter doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/
static int astrof_interrupt(void)
{
	if (readinputport(2) & 1)	/* Coin */
		return nmi_interrupt();
	else return ignore_interrupt();
}


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
/* Player 1 Controls */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
/* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credits" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, "None" )

	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END


static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xa0,0xa0,0xa0, /* ?????? */
	0x00,0xff,0x00, /* GREEN */
	0x00,0xff,0x00, /* GREEN */
	0xff,0x00,0xff, /* PURPLE */
	0x00,0xff,0xff, /* CYAN */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0xff,0x00  /* YELLOW */

};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			astrof_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	256, 256,                               /* screen_width, screen_height */
	{ 0, 256-1, 0, 256-1 },                 /* struct rectangle visible_area */

	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	astrof_vh_start,
	astrof_vh_stop,
	astrof_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};





/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( astrof_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kei2", 0xd000, 0x0400, 0x9dd4c3bc )
	ROM_LOAD( "keii", 0xd400, 0x0400, 0xc5cecd8e )
	ROM_LOAD( "kei0", 0xd800, 0x0400, 0xdd55dbfd )
	ROM_LOAD( "ke9",  0xdc00, 0x0400, 0xbefe8e64 )
	ROM_LOAD( "ke8",  0xe000, 0x0400, 0x68650103 )
	ROM_LOAD( "ke7",  0xe400, 0x0400, 0x1bf36e8f )
	ROM_LOAD( "ke6",  0xe800, 0x0400, 0xb07844ec )
	ROM_LOAD( "ke5",  0xec00, 0x0400, 0x6af05984 )
	ROM_LOAD( "ke4",  0xf000, 0x0400, 0xedcb5d21 )
	ROM_LOAD( "ke3",  0xf400, 0x0400, 0x9fff5faf )
	ROM_LOAD( "ke2",  0xf800, 0x0400, 0x7d41186b )
	ROM_LOAD( "kei",  0xfc00, 0x0400, 0x4e92741a )
ROM_END

ROM_START( astrof2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "afii.6", 0xd000, 0x0800, 0x63a20e32 )
	ROM_LOAD( "afii.5", 0xd800, 0x0800, 0x6fbe2d94 )
	ROM_LOAD( "afii.4", 0xe000, 0x0800, 0x84586f8c )
	ROM_LOAD( "afii.3", 0xe800, 0x0800, 0x0f581d68 )
	ROM_LOAD( "afii.2", 0xf000, 0x0800, 0x8dca028e )
	ROM_LOAD( "afii.1", 0xf800, 0x0800, 0xf85d14a5 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0000],"\x0c",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0084],2);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0084],2);
		osd_fclose(f);
	}
}



struct GameDriver astrof_driver =
{
	__FILE__,
	0,
	"astrof",
	"Astro Fighter (set 1)",
	"1980",
	"Data East",
	"Lee Taylor\nLucy Anne Taylor(Who`s birth 27/11/1997 made this driver possible)\nSanteri Saarimaa (high score save)",
	0,
	&machine_driver,

	astrof_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver astrof2_driver =
{
	__FILE__,
	&astrof_driver,
	"astrof2",
	"Astro Fighter (set 2)",
	"1980",
	"Data East",
	"Lee Taylor\nLucy Anne Taylor(Who`s birth 27/11/1997 made this driver possible)\nSanteri Saarimaa (high score save)",
	0,
	&machine_driver,

	astrof2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
