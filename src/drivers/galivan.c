/***************************************************************************
					Galivan
			    (C) 1985 Nihon Bussan

				    driver by

			Luca Elia (eliavit@unina.it)
			Olivier Galibert

			Interesting locations (main cpu)
			--------------------------------

0009	(ROM) if 0 there's an on screen display of this debug info:
	1)e218-9(yscroll)	2)e216-17(xscroll)	3)e3a9(player status)
	4)e5f5-6 (timer?)	5)e2a7	6)e5ef	7)ff5f

e123 yscroll	e124 xscroll e1d1	screen address e1d3-f2 string (like "insert coin")
e215	port 40 (rom bank)
e216	port 41 (scroll x lo)	<- max 0x7fff
e217	port 42 (scroll x hi)	<- gfx flags in high nibble
e218	port 43 (scroll y lo)	<- max 0x7fff
e219	port 44 (scroll y hi)
e21a	port 45 (sound command)
e21f-2e <- moved left one byte, last gets 0. e21f old value->e21a (port 45)
e22f-34	ip 0(cpl'd)/ip 0 (bits just gone high)/ip 1....
e235-6 DSW1-2(cpl'd)	e237-8 1P Coin-Play	e239-a 2P Coin-Play
e23c	Difficulty		e23d	Flip Screen		e23e	Start lives
e23f-41	1st bonus	e243-45	2nd bonus	e247	Demo sounds
e248	Cabinet	e24d	Credits	e24f	counter(60Hz)
e251	bit7->service	e252	routine selector(from 8fa, or fcd)
	0x1d->start1P
e280	x_scroll?	e284&5	Lives		e286	energy

			Interesting routines (main cpu)
			-------------------------------

271	Rom test	327	End tests	5d2	Reads input ports 0-2
692	Reads DSW's	777	clears screen and e1d3 string
7e5	prints the ASCII string pointed by DE to HL. FE means next byte is
	new attribute, FF means next two bytes are the new HL value.
	0 means end of string and new PC is HL.
961	Sets e215-a (ports 40-45 values)	a9b	background initialisation
c21	draws the info layer	fab	calls routine number (e252) from those in fcd.
149c	clears c5d bytes from HL.	14a7	fills the e662-ffff area

----------------------------------------------------------------------------

			Interesting routines (sound cpu)
			--------------------------------

108&	prepare for a DAC sound (sound command 1f-2d). Data is at 5000:
11f	offset:	0		1		2		3
	meaning:	DAC		Delay		Address-0x5000
	Sounds end with a 0x80

			Interesting locations (sound cpu)
			---------------------------------

c00d	counter

----------------------------------------------------------------------------

					TO DO
					-----

- Player can't get off lifts (maybe a missing button?)
- Find the color proms and check color attributes
- Find out how layers are enabled\disabled  and changed priority
- Tune the sound section
- Achieve pixel accurate positioning of gfx


***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

void galivan_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galivan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void galivan_scrollx_w(int offset,int data);
void galivan_scrolly_w(int offset,int data);
void galivan_gfxbank_w(int offset,int data);
void galivan_init_machine(void);
int galivan_vh_start(void);
void galivan_vh_stop(void);

extern unsigned char *foreground_ram;
extern int foreground_ram_size;

void galivan_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,(data<<1)|1);
}

int galivan_sound_command_r(int offset)
{
	int data;

	data = soundlatch_r(offset);
	soundlatch_clear_w(0,0);
	return data;
}



/* Main cpu data */

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_RAM },

	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
 	/* comment line if you want to change 0x0009 rom location */
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd800, 0xdbff, videoram_w, &videoram, &videoram_size },
	{ 0xdc00, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xe0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe100, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};


/* Main cpu ports */

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x40, 0x40, galivan_gfxbank_w },
	{ 0x41, 0x42, galivan_scrollx_w },
	{ 0x43, 0x44, galivan_scrolly_w },
	{ 0x45, 0x45, galivan_sound_command_w },
//	{ 0x46, 0x46, IOWP_NOP },
//	{ 0x47, 0x47, IOWP_NOP },
	{ -1 }	/* end of table */
};


/* Sound cpu data */

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }
};


/* Sound cpu ports */

static struct IOReadPort sound_readport[] =
{
//	{ 0x04, 0x04, IORP_NOP }, /* value read and *discarded* */
	{ 0x06, 0x06, galivan_sound_command_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x03, DAC_data_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( galivan_input_ports )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )

	PORT_START  /* IN2 - TEST, COIN, START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x20, 0x20, 0, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "First Bonus" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x08, 0x08, "Second Bonus" )
	PORT_DIPSETTING(    0x08, "60000 after first" )
	PORT_DIPSETTING(    0x00, "90000 after first" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW1 7 (Unknown)" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START	/* IN4 - DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW2 6 (Unknown)" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW2 7 (Unused?)" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )	/* unused? */

INPUT_PORTS_END


/* galivan gfx layouts */

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static struct GfxLayout tilelayout =
{
	16,16,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 4,0,12,8,20,16,28,24,36,32,44,40,52,48,60,56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*16*4
};

static struct GfxLayout spritelayout =
{
	16,16,
	512,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 4+512*64*8, 0+512*64*8, 12, 8, 12+512*64*8, 8+512*64*8,
	  20, 16, 20+512*64*8, 16+512*64*8, 28, 24, 28+512*64*8, 24+512*64*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,            0,  8 },
	{ 1, 0x04000, &tilelayout,         8*16, 16 },
	{ 1, 0x24000, &spritelayout, 8*16+16*16, 64 },
	{ -1 }
};


static struct YM3526interface YM3526_interface =
{
	1,
	4000000,
	{ 0xff },
	0
};


static struct DACinterface dac_interface =
{
	2,
	{ 0xff, 0xff }
};


static struct MachineDriver galivan_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* ?? Hz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ?? Hz */
			4,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0,
			interrupt,7250  /* timed interrupt, ?? Hz */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	galivan_init_machine, /* init machine */

	/* video hardware */
	32*8, 32*8, { 0, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 8*16+16*16+64*16,
	galivan_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galivan_vh_start,
	galivan_vh_stop,
	galivan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&YM3526_interface
		},

		{
			SOUND_DAC,
			&dac_interface
		}

	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galivan_rom )
	ROM_REGION(0x14000)		/* Region 0 - main cpu code */
	ROM_LOAD( "gv1.1b",      0x00000, 0x08000, 0x5e480bfc )
	ROM_LOAD( "gv2.3b",      0x08000, 0x04000, 0x0d1b3538 )
	ROM_LOAD( "gv3.4b",      0x10000, 0x04000, 0x82f0c5e6 ) /* 2 banks at c000 */

	ROM_REGION_DISPOSE(0x34000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "gv4.13d",     0x00000, 0x04000, 0x162490b4 ) /* chars */
	ROM_LOAD( "gv7.14f",     0x04000, 0x08000, 0xeaa1a0db ) /* tiles */
	ROM_LOAD( "gv8.15f",     0x0c000, 0x08000, 0xf174a41e )
	ROM_LOAD( "gv9.17f",     0x14000, 0x08000, 0xedc60f5d )
	ROM_LOAD( "gv10.19f",    0x1c000, 0x08000, 0x41f27fca )
	ROM_LOAD( "gv14.4f",     0x24000, 0x08000, 0x03e2229f ) /* sprites */
	ROM_LOAD( "gv13.1f",     0x2c000, 0x08000, 0xbca9e66b )

	ROM_REGION(0x8000)		/* Region 2 - background */
	ROM_LOAD( "gv6.19d",     0x00000, 0x04000, 0xda38168b )
	ROM_LOAD( "gv5.17d",     0x04000, 0x04000, 0x22492d2a )

	ROM_REGION(0x0500)		/* Region 3 - color proms */
	ROM_LOAD( "mb7114e.9f",  0x0000, 0x0100, 0xde782b3e )	/* red */
	ROM_LOAD( "mb7114e.10f", 0x0100, 0x0100, 0x0ae2a857 )	/* green */
	ROM_LOAD( "mb7114e.11f", 0x0200, 0x0100, 0x7ba8b9d1 )	/* blue */
	ROM_LOAD( "mb7114e.2d",  0x0300, 0x0100, 0x75466109 )	/* sprite lookup table */
	ROM_LOAD( "mb7114e.7f",  0x0400, 0x0100, 0x06538736 )	/* sprite palette bank */

	ROM_REGION(0x10000)		/* Region 4 - sound cpu code */
	ROM_LOAD( "gv11.14b",    0x00000, 0x04000, 0x05f1a0e3 )
	ROM_LOAD( "gv12.15b",    0x04000, 0x08000, 0x5b7a0d6d )
ROM_END


/* Ten entries, 13 bytes each:  3 bytes - score/10 (BCD)
					 10 bytes - name (ASCII)		*/
static int galivan_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the high scores table has already been initialized */
	if ((memcmp(&RAM[0xe14f], "\x00\x01\x50\x4B", 4) == 0)&&
	    (memcmp(&RAM[0xe1cd], "\x54\x45\x52\x20", 4) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f, &RAM[0xe14f], 13*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the high scores yet */
}


static void galivan_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f, &RAM[0xe14f], 13*10);
		osd_fclose(f);
	}
}



struct GameDriver galivan_driver =
{
	__FILE__,
	0,
	"galivan",
	"Galivan",
	"1985",
	"Nichibutsu",
	"Luca Elia\nOlivier Galibert",
	GAME_IMPERFECT_COLORS,
	&galivan_machine_driver,
	0,

	galivan_rom,
	0, 0,
	0,
	0,

	galivan_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_270,

	galivan_hiload, galivan_hisave
};
