/*
driver:yiear.c

YIE AR KUNG-FU hardware description
enrique.sanchez@cs.us.es

Main CPU:    Motorola 6809
Sound chip:  ???

Normal 6809 IRQs must be generated each video frame (60 fps).
The 6809 NMI vector is present, though I don't know what's its use.

ROM files

D12_8.BIN (16K)  -- ROM $8000-$BFFF   \___ 6809 code
D14_7.BIN (16K)  -- ROM $C000-$FFFF   /

G16_1.BIN (8K)   -- Background tiles; bitplanes 0,1
G15_2.BIN (8K)   -- Background tiles; bitplanes 2,3

G06_3.BIN (16K)  -- Sprites; bitplanes 0,1 (part 1)
G05_4.BIN (16K)  -- Sprites; bitplanes 0,1 (part 2)
G04_5.BIN (16K)  -- Sprites; bitplanes 2,3 (part 1)
G03_6.BIN (16K)  -- Sprites; bitplanes 2,3 (part 2)

A12_9.BIN (8K)   -- Sound related??

Memory map

0000 - 3FFF   = RAM
4000&4f00     various, not clear which of the two locations does what
              but it seems 4000 is used for interrupt enable, and
			  4f00 for everything else.
              bit 0 flip screen
              bit 1 ?
              bit 2 interrupt enable/acknowledge
              bit 3 coin cointer 1
              bit 4 coin cointer 1
4800          = ?
4900          = ?
4A00          = ?
4B00          = ?
4C00          = DIP SWITCH 1
4D00          = DIP SWITCH 2
4E00		  = COIN,START
4E01          = JOY1
4E02		  = JOY2
4E03		  = DIP SWITCH 3
5000 - 57FF   = RAM (includes sprite attribute zone mapped in it)
5030 - 51AF   = SPRITE ATTRIBUTES
5800 - 5FFF   = BACKGROUND ATTRIBUTES
6000 - 7FFF   = Not used.
8000 - FFFF   = Code ROM.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"

void yiear_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void yiear_vh_screenrefresh(struct osd_bitmap *bitmap);
void yiear_videoram_w(int offset,int data);
void yiear_4f00_w(int offset,int data);

extern unsigned char *yiear_soundport;
extern const char *yiear_sample_names[];
void yiear_audio_out_w( int offset, int val );
int yiear_sh_start( void );
void yiear_sh_update(void);



void yiear_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;
}

static void yiear_interrupt_enable_w(int offset,int data)
{
	/* bit 2 is interrupt enable */
	interrupt_enable_w(offset,data & 0x04);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x4E00, 0x4E00, input_port_0_r },	/* coin,start */
	{ 0x4E01, 0x4E01, input_port_1_r },	/* joy1 */
	{ 0x4E02, 0x4E02, input_port_2_r },	/* joy2 */
	{ 0x4C00, 0x4C00, input_port_3_r },	/* misc */
	{ 0x4D00, 0x4D00, input_port_4_r },	/* test mode */
	{ 0x4E03, 0x4E03, input_port_5_r },	/* coins per play */
	{ 0x5000, 0x5fff, MRA_RAM },
	{ 0x8000, 0xFFFF, MRA_ROM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x4000, yiear_interrupt_enable_w },
	{ 0x4f00, 0x4f00, yiear_4f00_w },
	{ 0x5030, 0x51AF, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5607, 0x5607, yiear_audio_out_w, &yiear_soundport },
	{ 0x5000, 0x57FF, MWA_RAM },	/* sprites and audio are in this area */
	{ 0x5800, 0x5FFF, videoram_w, &videoram, &videoram_size },
	{ 0x8000, 0xFFFF, MWA_ROM },
	{ -1 } /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "30000 80000" )
	PORT_DIPSETTING(    0x00, "40000 90000" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown DSW1 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown DSW1 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown DSW2 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW2 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown DSW2 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown DSW2 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown DSW2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown DSW2 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	/* 0x00 gives invalid */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	512,	/* 512 characters */
	4,		/* 4 bits per pixel */
	{ 4, 0, 512*16*8+4, 512*16*8+0 },		/* plane */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },		/* x */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },	/* y */
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16 by 16 */
	512,	/* 512 sprites */
	4,		/* 4 bits per pixel */
	{ 512*64*8+4, 512*64*8+0, 4, 0 },	/* plane offsets */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 1 },
	{ 1, 0x4000, &spritelayout, 16, 1 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	0x00,0x49,0xE0,0xFD,0x06,0xA7,0x5D,0x2E,0x3F,0xAE,0xBF,0xB7,0x3D,0x72,0xAD,0xFF,
	0x00,0xF5,0x5E,0x66,0xA5,0x70,0xFE,0x2E,0x29,0x20,0x00,0x80,0xA0,0xE3,0xAD,0xFF
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,	/* 1.25 Mhz */
			0,			/* memory region */
			readmem,	/* MemoryReadAddress */
			writemem,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			interrupt,	/* interrupt routine */
			1			/* interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,			/* GfxDecodeInfo * */

	32,						/* total colors */
	32,						/* color table length */
	yiear_vh_convert_color_prom,						/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,						/* vh_init routine */
	generic_vh_start,		/* vh_start routine */
	generic_vh_stop,		/* vh_stop routine */
	yiear_vh_screenrefresh,	/* vh_update routine */

	/* sound hardware */
	0,					/* sh_init routine */
	yiear_sh_start,		/* sh_start routine */
	0,					/* sh_stop routine */
	yiear_sh_update		/* sh_update routine */
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( yiear_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D12_8.BIN", 0x8000, 0x4000, 0x53e66644 )
	ROM_LOAD( "D14_7.BIN", 0xC000, 0x4000, 0xd5f1fb77 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "G16_1.BIN", 0x00000, 0x2000, 0x12fd01fd )
	ROM_LOAD( "G15_2.BIN", 0x02000, 0x2000, 0xf889b63f )
	ROM_LOAD( "G06_3.BIN", 0x04000, 0x4000, 0x5c60737a )
	ROM_LOAD( "G05_4.BIN", 0x08000, 0x4000, 0xe9a3d4d9 )
	ROM_LOAD( "G04_5.BIN", 0x0c000, 0x4000, 0x35ca933a )
	ROM_LOAD( "G03_6.BIN", 0x10000, 0x4000, 0x9e25b6cb )
ROM_END



static int hiload(void)
{
	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x5520],"\x00\x36\x70",3) == 0) &&
		(memcmp(&RAM[0x55A9],"\x10\x10\x10",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Read the scores */
                        osd_fread(f,&RAM[0x5520],14*10);
			/* reset score at top */
			memcpy(&RAM[0x521C],&RAM[0x5520],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the scores */
                osd_fwrite(f,&RAM[0x5520],14*10);
		osd_fclose(f);
	}

}



struct GameDriver yiear_driver =
{
	"Yie Ar Kung Fu (Konami)",
	"yiear",
	"Enrique Sanchez\nPhilip Stroffolino\nMike Balfour (high score)\nTim Lindquist (color info)\nKevin Estep (sound info)\nMarco Cassili",
	&machine_driver,

	yiear_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	yiear_sample_names,	/* Sample names */
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
