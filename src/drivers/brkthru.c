/***************************************************************************
Break Thru Doc. Data East (1986)

Sound: YM2203 and YM3526

UNK-1.1    (16Kb)  Code (4000-7FFF)
UNK-1.2    (32Kb)  Main 6809 (8000-FFFF)
UNK-1.3    (32Kb)  Mapped (2000-3FFF)
UNK-1.4    (32Kb)  Mapped (2000-3FFF)

UNK-1.5    (32Kb)  Sound 6809 (8000-FFFF)

Background has 4 banks, with 256 16x16x8 tiles in each bank.
UNK-1.6    (32Kb)  GFX Background
UNK-1.7    (32Kb)  GFX Background
UNK-1.8    (32Kb)  GFX Background

Sprites ROMs haven't been dumped, but are probably the same format as the background.
UNK-1.9    (32Kb)  GFX Sprites
UNK-1.10   (32Kb)  GFX Sprites
UNK-1.11   (32Kb)  GFX Sprites

Text has 256 8x8x4 characters.
UNK-1.12   (8Kb)   GFX Text

**************************************************************************
Memory Map for Main CPU by Carlos A. Lozano
**************************************************************************

MAIN CPU
0000-03FF                                   W                   Plane0
0400-0BFF                                  R/W                  RAM
0C00-0FFF                                   W                   Plane2(Background)
1000-10FF                                   W                   Plane1(sprites)
1100-17FF                                  R/W                  RAM
1800-180F                                  R/W                  In/Out
1810-1FFF                                  R/W                  RAM (unmapped?)
2000-3FFF                                   R                   ROM Mapped(*)
4000-7FFF                                   R                   ROM(UNK-1.1)
8000-FFFF                                   R                   ROM(UNK-1.2)

Interrupts: Reset, NMI, IRQ
The test routine is at F000

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"

unsigned char *brkthru_nmi_enable; /* needs to be tracked down */
extern unsigned char *brkthru_videoram;
extern unsigned int brkthru_videoram_size;

void brkthru_1800_w(int offset,int data);
int brkthru_vh_start(void);
void brkthru_vh_stop(void);
void brkthru_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void brkthru_vh_screenrefresh(struct osd_bitmap *bitmap);


static int nmi_enable;

void brkthru_1803_w(int offset, int data)
{
	/* bit 0 = NMI enable */
	nmi_enable = ~data & 1;

	/* bit 1 = ? maybe IRQ acknowledge */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },		/* Plane 0: Text */
	{ 0x0400, 0x0bff, MRA_RAM },
	{ 0x0c00, 0x0fff, MRA_RAM },		/* Plane 2  Background */
	{ 0x1000, 0x10ff, MRA_RAM },		/* Plane 1: Sprites */
	{ 0x1100, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1800, input_port_0_r },	/* player controls, player start */
	{ 0x1801, 0x1801, input_port_1_r },	/* cocktail player controls */
	{ 0x1802, 0x1802, input_port_3_r },	/* DSW 0 */
	{ 0x1803, 0x1803, input_port_2_r },	/* coin input & DSW */
	{ 0x2000, 0x3fff, MRA_BANK1 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM, &brkthru_videoram, &brkthru_videoram_size },
	{ 0x0400, 0x0bff, MWA_RAM },
	{ 0x0c00, 0x0fff, videoram_w, &videoram, &videoram_size },
	{ 0x1000, 0x10ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1100, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1801, brkthru_1800_w },	/* bg scroll and color, ROM bank selection, flip screen */
	{ 0x1802, 0x1802, soundlatch_w },
	{ 0x1803, 0x1803, brkthru_1803_w },	/* NMI enable, + ? */
	{ 0x2000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

#ifdef TRY_SOUND
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif


int brkthru_interrupt(void)
{
	if (cpu_getiloops() == 0)
	{
		if (nmi_enable) return nmi_interrupt();
	}
	else
	{
		/* generate IRQ on coin insertion */
		if ((readinputport(2) & 0xe0) != 0xe0) return interrupt();
	}

	return ignore_interrupt();
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* used only by the self test */

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX(0,        0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE, 0)
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin C", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 512*8*8+4, 0, 4 },	/* plane offset */
	{ 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout1 =
{
	16,16,	/* 16*16 tiles */
	128,	/* 128 tiles */
	3,	/* 3 bits per pixel */
	{ 0x4000*8+4, 0, 4 },	/* plane offset */
	{ 0, 1, 2, 3, 1024*8*8+0, 1024*8*8+1, 1024*8*8+2, 1024*8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+1024*8*8+0, 16*8+1024*8*8+1, 16*8+1024*8*8+2, 16*8+1024*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout2 =
{
	16,16,	/* 16*16 tiles */
	128,	/* 128 tiles */
	3,	/* 3 bits per pixel */
	{ 0x3000*8+0, 0, 4 },	/* plane offset */
	{ 0, 1, 2, 3, 1024*8*8+0, 1024*8*8+1, 1024*8*8+2, 1024*8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+1024*8*8+0, 16*8+1024*8*8+1, 16*8+1024*8*8+2, 16*8+1024*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 1 },
	{ 1, 0x02000, &tilelayout1, 8+8*8, 16 },
	{ 1, 0x03000, &tilelayout2, 8+8*8, 16 },
	{ 1, 0x0a000, &tilelayout1, 8+8*8, 16 },
	{ 1, 0x0b000, &tilelayout2, 8+8*8, 16 },
	{ 1, 0x12000, &tilelayout1, 8+8*8, 16 },
	{ 1, 0x13000, &tilelayout2, 8+8*8, 16 },
	{ 1, 0x1a000, &tilelayout1, 8+8*8, 16 },
	{ 1, 0x1b000, &tilelayout2, 8+8*8, 16 },
	{ 1, 0x22000, &spritelayout,    8, 8 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* brkthru.13 - palette red and green component */
	0x00,0x0F,0xCF,0xFC,0x00,0xF0,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x67,0x0F,0x00,0x78,0xCD,0x89,0xEE,0x00,0xCC,0x4F,0x00,0x88,0x88,0x60,0xEF,
	0x00,0x4A,0x7F,0x00,0x78,0x9A,0x9B,0xDC,0x00,0x4A,0x7F,0x00,0x5B,0xEE,0x9D,0xDD,
	0x00,0x08,0x0F,0x00,0x88,0xCC,0x75,0xEE,0x00,0x67,0x0F,0x00,0x78,0xCD,0x89,0xEE,
	0x00,0x7A,0xBF,0x00,0x88,0xDE,0x75,0xFF,0x00,0x0C,0x0F,0x00,0xFF,0x89,0xDF,0xFF,
	0xAD,0xCE,0x8A,0xEF,0x00,0x95,0xC6,0xF0,0xAD,0xDF,0x79,0x8F,0x43,0x82,0xB2,0xDD,
	0xAD,0x8C,0x9F,0x36,0xF4,0x94,0x2A,0x00,0x77,0xAE,0x9C,0x00,0x88,0x9B,0x7B,0x00,
	0x00,0xFF,0xBB,0x88,0x48,0x16,0x91,0xCC,0x68,0x33,0xAF,0x9C,0x69,0x12,0xCC,0xBB,
	0x99,0x90,0x00,0x91,0xF0,0x0A,0x8C,0xEE,0xF0,0xCF,0xAF,0x8C,0x7E,0x90,0x00,0xEE,
	0xDC,0xB4,0x82,0xB0,0xCC,0x55,0x47,0xFD,0xDC,0xC9,0x94,0x86,0x65,0x19,0x77,0xFF,
	0x00,0x67,0xDE,0xBF,0xB0,0xEE,0x00,0xFF,0x00,0x0F,0x00,0x4F,0xAA,0xBB,0xEE,0xFF,
	0x00,0xD8,0x00,0x1F,0xF2,0xFF,0xFF,0xFF,0x00,0x0F,0xCF,0xFF,0xDF,0xCE,0xBA,0xFF,
	0x7A,0x67,0x56,0x68,0x00,0x57,0xB3,0xAA,0x7A,0x67,0x56,0x68,0x00,0x57,0xB3,0xAA,
	/* brkthru.14 - palette blue component */
	0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x09,0x00,0x00,0x07,0x0B,0x08,0x0F,0x00,0x0D,0x02,0x00,0x09,0x05,0x07,0x0E,
	0x00,0x02,0x02,0x00,0x04,0x05,0x07,0x0B,0x00,0x02,0x02,0x00,0x01,0x0F,0x04,0x0D,
	0x00,0x0B,0x05,0x00,0x07,0x0C,0x01,0x0E,0x00,0x09,0x00,0x00,0x07,0x0B,0x0B,0x0F,
	0x00,0x01,0x0A,0x00,0x08,0x0F,0x01,0x0F,0x00,0x00,0x00,0x00,0x06,0x08,0x04,0x0F,
	0x07,0x07,0x03,0x0C,0x00,0x00,0x00,0x00,0x07,0x0A,0x00,0x06,0x03,0x0F,0x0F,0x0D,
	0x07,0x06,0x01,0x00,0x05,0x03,0x02,0x00,0x07,0x07,0x06,0x08,0x07,0x00,0x06,0x00,
	0x00,0x0F,0x0B,0x08,0x05,0x0E,0x0F,0x0C,0x08,0x03,0x00,0x00,0x02,0x08,0x0C,0x0B,
	0x09,0x0A,0x00,0x07,0x00,0x00,0x04,0x0E,0x00,0x08,0x01,0x04,0x03,0x0E,0x00,0x0E,
	0x0C,0x0B,0x02,0x0E,0x0B,0x05,0x02,0x0F,0x0C,0x00,0x04,0x0F,0x0B,0x06,0x08,0x0F,
	0x00,0x07,0x0B,0x06,0x06,0x0E,0x0E,0x0F,0x00,0x00,0x0F,0x04,0x0A,0x0D,0x0E,0x0F,
	0x00,0x0D,0x0F,0x0A,0x09,0x0F,0x0F,0x0F,0x00,0x00,0x07,0x01,0x09,0x0F,0x0F,0x0F,
	0x01,0x02,0x00,0x04,0x00,0x01,0x01,0x0A,0x01,0x02,0x00,0x04,0x00,0x01,0x01,0x0A,
};



static struct MachineDriver brkthru_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz ? */
			0,
			readmem,writemem,0,0,
			brkthru_interrupt,2
#ifdef TRY_SOUND
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1
#endif
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,8+8*8+16*8,
	brkthru_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	brkthru_vh_start,
	brkthru_vh_stop,
	brkthru_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( brkthru_rom )
	ROM_REGION(0x20000)     /* 64k for main CPU + 64k for banked ROMs */
	ROM_LOAD( "brkthru.1", 0x04000, 0x4000, 0x1badf6d3 )
	ROM_LOAD( "brkthru.2", 0x08000, 0x8000, 0x463d0ead )
	ROM_LOAD( "brkthru.4", 0x10000, 0x8000, 0xc76c007a )
	ROM_LOAD( "brkthru.3", 0x18000, 0x8000, 0x3273003d )

	ROM_REGION(0x3a000)	/* temporary space for graphics */
	ROM_LOAD( "brkthru.12", 0x00000, 0x2000, 0x1b793a0b )	/* characters */
	/* background */
	/* we do a lot of scatter loading here, to place the data in a format */
	/* which can be decoded by MAME's standard functions */
	ROM_LOAD( "brkthru.7",  0x02000, 0x4000, 0xac8d4d91 )	/* bitplanes 1,2 for bank 1,2 */
	ROM_CONTINUE(           0x0a000, 0x4000 )		/* bitplanes 1,2 for bank 3,4 */
	ROM_LOAD( "brkthru.6",  0x12000, 0x4000, 0xf4d950bd )	/* bitplanes 1,2 for bank 5,6 */
	ROM_CONTINUE(           0x1a000, 0x4000 )		/* bitplanes 1,2 for bank 7,8 */
	ROM_LOAD( "brkthru.8",  0x06000, 0x1000, 0x20ca0802 )	/* bitplane 3 for bank 1,2 */
	ROM_CONTINUE(           0x08000, 0x1000 )
	ROM_CONTINUE(           0x0e000, 0x1000 )		/* bitplane 3 for bank 3,4 */
	ROM_CONTINUE(           0x10000, 0x1000 )
	ROM_CONTINUE(           0x16000, 0x1000 )		/* bitplane 3 for bank 5,6 */
	ROM_CONTINUE(           0x18000, 0x1000 )
	ROM_CONTINUE(           0x1e000, 0x1000 )		/* bitplane 3 for bank 7,8 */
	ROM_CONTINUE(           0x20000, 0x1000 )
	/* sprites */
	ROM_LOAD( "brkthru.9",  0x22000, 0x8000, 0xe7ca5c28 )
	ROM_LOAD( "brkthru.10", 0x2a000, 0x8000, 0x50691d89 )
	ROM_LOAD( "brkthru.11", 0x32000, 0x8000, 0x92f174b5 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "brkthru.5", 0x8000, 0x8000, 0x364efd26 )
ROM_END



struct GameDriver brkthru_driver =
{
	"Break Thru",
	"brkthru",
	"Phil Stroffolino (MAME driver)\nCarlos Lozano (hardware info)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)\nMarco Cassili",
	&brkthru_machine_driver,

	brkthru_rom,
	0, 0,
	0,
	0,

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
