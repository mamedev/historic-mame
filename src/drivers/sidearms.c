/***************************************************************************

  Sidearms
  ========

  Driver provided by Paul Leaman (paull@vortexcomputing.demon.co.uk)

  Please do not send anything large to this address without asking me
  first.

  There is an additional ROM which seems to contain code for a third Z80,
  however the board only has two. The ROM is related to the missing star
  background. At one point, the code jumps to A000, outside of the ROM
  address space.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *sidearms_bg_scrollx,*sidearms_bg_scrolly;
extern unsigned char *sidearms_bg2_scrollx,*sidearms_bg2_scrolly;
extern unsigned char *sidearms_paletteram;

/* Uses black tiger paging system */
void blktiger_bankswitch_w(int offset,int data);
int blktiger_interrupt(void);

void sidearms_paletteram_w(int offset, int data);
void sidearms_c804_w(int offset, int data);
int  sidearms_vh_start(void);
void sidearms_vh_stop(void);
void sidearms_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void sidearms_vh_screenrefresh(struct osd_bitmap *bitmap);



static void sidearms_bankswitch_w(int offset,int data)
{
	int bankaddress;


	/* bits 0 and 1 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc800, 0xc800, input_port_0_r },
	{ 0xc801, 0xc801, input_port_1_r },
	{ 0xc802, 0xc802, input_port_2_r },
	{ 0xc803, 0xc803, input_port_3_r },
	{ 0xc804, 0xc804, input_port_4_r },
	{ 0xc805, 0xc805, input_port_5_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, sidearms_paletteram_w, &sidearms_paletteram },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc801, 0xc801, sidearms_bankswitch_w },
	{ 0xc802, 0xc802, MWA_NOP },	/* watchdog reset? */
	{ 0xc804, 0xc804, sidearms_c804_w },
	{ 0xc805, 0xc805, MWA_RAM, &sidearms_bg2_scrollx },
	{ 0xc806, 0xc806, MWA_RAM, &sidearms_bg2_scrolly },
	{ 0xc808, 0xc809, MWA_RAM, &sidearms_bg_scrollx },
	{ 0xc80a, 0xc80b, MWA_RAM, &sidearms_bg_scrolly },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },
	{ 0xd800, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};

#ifdef THIRD_CPU
static void pop(int offset,int data)
{
RAM[0xa000] = 0xc3;
RAM[0xa001] = 0x00;
RAM[0xa002] = 0xa0;
//	interrupt_enable_w(offset,data & 0x80);
}

static struct MemoryReadAddress readmem2[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM },
	{ 0xec00, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xebff, MWA_RAM },
	{ 0xec00, 0xefff, MWA_RAM },
	{ 0xf80e, 0xf80e, pop },	/* ROM bank selector? (to appear at 8000) */
	{ -1 }	/* end of table */
};
#endif

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, soundlatch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )	/* I don't think it's really a dip switch */
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "0 (Easiest)")
	PORT_DIPSETTING(    0x06, "1")
	PORT_DIPSETTING(    0x05, "2")
	PORT_DIPSETTING(    0x04, "3")
	PORT_DIPSETTING(    0x03, "4")
	PORT_DIPSETTING(    0x02, "5")
	PORT_DIPSETTING(    0x01, "6")
	PORT_DIPSETTING(    0x00, "7 (Hardest)")
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5")
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "100000" )
	PORT_DIPSETTING(    0x20, "100000 100000" )
	PORT_DIPSETTING(    0x10, "150000 150000" )
	PORT_DIPSETTING(    0x00, "200000 200000" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit")
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit")
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No")
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* not sure, but likely */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	512,    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ 512*256*8+4, 512*256*8+0, 4, 0 },
	{
		      0,       1,       2,       3,       8+0,       8+1,       8+2,       8+3,
		32*16+0, 32*16+1, 32*16+2, 32*16+3, 32*16+8+0, 32*16+8+1, 32*16+8+2, 32*16+8+3,
		64*16+0, 64*16+1, 64*16+2, 64*16+3, 64*16+8+0, 64*16+8+1, 64*16+8+2, 64*16+8+3,
		96*16+0, 96*16+1, 96*16+2, 96*16+3, 96*16+8+0, 96*16+8+1, 96*16+8+2, 96*16+8+3,
	},
	{
		 0*16,  1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
		 8*16,  9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
	},
	256*8	/* every tile takes 256 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/*   start    pointer       colour start   number of colours */
	{ 1, 0x00000, &charlayout,            0, 64 },
	{ 1, 0x08000, &tilelayout,         64*4, 32 },
	{ 1, 0x48000, &spritelayout, 64*4+32*16, 16 },
	{ -1 } /* end of array */
};



/* Sidearms has a 2048 color palette RAM, but it doesn't seem to modify it */
/* dynamically. The color space is 4x4x4, and the number of unique colors is */
/* greater than 256; however, ignoring the least significant bit of the blue */
/* component, we can squeeze them into 250 colors with no appreciable loss. */
static unsigned char color_table[251*2] =
{
	0x00,0x00,	/* non transparent black */
	0x00,0x00,0x00,0x04,0x00,0x06,0x00,0x09,0x00,0x0b,0x00,0x0d,0x00,0x0f,0x01,0x05,
	0x01,0x06,0x02,0x06,0x03,0x05,0x03,0x07,0x04,0x00,0x04,0x03,0x04,0x05,0x04,0x06,
	0x04,0x08,0x05,0x00,0x05,0x05,0x05,0x07,0x05,0x09,0x06,0x09,0x06,0x0b,0x06,0x0c,
	0x07,0x00,0x07,0x06,0x07,0x09,0x07,0x0b,0x07,0x0e,0x08,0x00,0x08,0x0b,0x08,0x0d,
	0x08,0x0f,0x09,0x00,0x09,0x0a,0x09,0x0c,0x09,0x0f,0x0a,0x00,0x0a,0x0d,0x0a,0x0f,
	0x0b,0x0e,0x0c,0x00,0x0c,0x0c,0x0c,0x0f,0x0e,0x00,0x0e,0x0f,0x0f,0x00,0x0f,0x0f,
	0x10,0x05,0x11,0x01,0x11,0x05,0x22,0x02,0x24,0x00,0x24,0x03,0x33,0x01,0x33,0x02,
	0x34,0x03,0x34,0x06,0x35,0x03,0x35,0x04,0x36,0x06,0x40,0x00,0x40,0x05,0x44,0x02,
	0x44,0x04,0x45,0x00,0x45,0x03,0x45,0x04,0x45,0x06,0x45,0x09,0x46,0x00,0x46,0x02,
	0x46,0x04,0x47,0x06,0x47,0x08,0x49,0x0b,0x50,0x00,0x55,0x02,0x55,0x04,0x56,0x05,
	0x56,0x06,0x56,0x08,0x57,0x0a,0x57,0x0d,0x58,0x08,0x60,0x00,0x60,0x06,0x62,0x00,
	0x63,0x00,0x63,0x06,0x64,0x00,0x65,0x05,0x66,0x04,0x66,0x06,0x67,0x05,0x67,0x06,
	0x67,0x08,0x68,0x00,0x68,0x04,0x68,0x06,0x68,0x08,0x6a,0x0b,0x70,0x00,0x74,0x00,
	0x74,0x02,0x74,0x09,0x76,0x00,0x76,0x05,0x77,0x04,0x77,0x06,0x78,0x07,0x78,0x08,
	0x79,0x07,0x79,0x08,0x79,0x0c,0x7a,0x0b,0x7a,0x0c,0x7b,0x0b,0x7c,0x0f,0x80,0x00,
	0x80,0x08,0x82,0x00,0x83,0x00,0x84,0x00,0x84,0x08,0x85,0x00,0x85,0x03,0x85,0x09,
	0x86,0x00,0x86,0x0a,0x87,0x00,0x87,0x05,0x88,0x00,0x88,0x05,0x88,0x06,0x88,0x08,
	0x89,0x07,0x8a,0x09,0x8b,0x00,0x8b,0x06,0x90,0x00,0x93,0x0b,0x95,0x00,0x95,0x03,
	0x98,0x0c,0x99,0x07,0x99,0x09,0x99,0x0a,0x9a,0x09,0x9a,0x0a,0x9b,0x09,0x9b,0x0b,
	0x9c,0x0d,0xa4,0x00,0xa4,0x04,0xa4,0x0a,0xa5,0x00,0xa5,0x0a,0xa6,0x00,0xa6,0x09,
	0xa7,0x0c,0xa9,0x06,0xaa,0x07,0xaa,0x09,0xaa,0x0a,0xaa,0x0e,0xab,0x09,0xac,0x0b,
	0xad,0x00,0xad,0x08,0xad,0x0d,0xb0,0x00,0xb4,0x00,0xb5,0x03,0xb6,0x00,0xb7,0x00,
	0xb7,0x05,0xb8,0x00,0xb9,0x04,0xba,0x06,0xbb,0x09,0xbb,0x0b,0xbc,0x0b,0xbc,0x0c,
	0xbc,0x0f,0xbd,0x0b,0xbe,0x0f,0xc0,0x00,0xc5,0x00,0xc5,0x06,0xc5,0x0a,0xc6,0x00,
	0xc6,0x0c,0xc7,0x0a,0xc8,0x0c,0xc9,0x00,0xc9,0x0f,0xcb,0x08,0xcc,0x00,0xcc,0x09,
	0xcc,0x0c,0xcc,0x0e,0xce,0x0d,0xce,0x0e,0xcf,0x0a,0xd0,0x00,0xd6,0x00,0xd6,0x04,
	0xd7,0x00,0xd7,0x05,0xd8,0x00,0xd8,0x05,0xd9,0x07,0xda,0x00,0xdd,0x0b,0xdd,0x0d,
	0xe0,0x00,0xe6,0x00,0xe6,0x08,0xe7,0x00,0xe7,0x0a,0xe7,0x0e,0xe8,0x00,0xe9,0x00,
	0xea,0x00,0xec,0x00,0xed,0x0a,0xee,0x00,0xee,0x0e,0xf0,0x00,0xf0,0x0f,0xf7,0x0c,
	0xf8,0x00,0xf8,0x04,0xf8,0x06,0xf8,0x0b,0xf9,0x00,0xf9,0x07,0xfa,0x00,0xfa,0x06,
	0xfa,0x08,0xfb,0x00,0xfb,0x07,0xfb,0x09,0xfc,0x00,0xfc,0x0a,0xff,0x00,0xff,0x09,
	0xff,0x0a,0xff,0x0f
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3500000,	/* 3.5 MHz ? (hand tuned) */
	{ YM2203_VOL(100,0x20ff), YM2203_VOL(100,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static struct MachineDriver sidearms_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2203 */
		},
#ifdef THIRD_CPU
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			4,	/* memory region #4 */
			readmem2,writemem2,0,0,
			nmi_interrupt,1
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(color_table)/2,
	64*4+32*16+16*16,   /* Colour table length */
	sidearms_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	sidearms_vh_start,
	sidearms_vh_stop,
	sidearms_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



ROM_START( sidearms_rom )
	ROM_REGION(0x20000)     /* 64k for code + banked ROMs images */
	ROM_LOAD( "SA03.BIN", 0x00000, 0x08000, 0x22bb6721 )  /* CODE */
	ROM_LOAD( "SA02.BIN", 0x10000, 0x08000, 0x9ae56dc9 )  /* 0+1 */
	ROM_LOAD( "SA01.BIN", 0x18000, 0x08000, 0xd334d882 )  /* 2+3 */

	ROM_REGION(0x88000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "SA04.BIN", 0x00000, 0x8000, 0xe0d00000 )  /* characters */
	ROM_LOAD( "SA15.BIN", 0x08000, 0x8000, 0xa0101f3e ) /* tiles */
	ROM_LOAD( "SA17.BIN", 0x10000, 0x8000, 0x89f8efec ) /* tiles */
	ROM_LOAD( "SA19.BIN", 0x18000, 0x8000, 0x31c9b645 ) /* tiles */
	ROM_LOAD( "SA21.BIN", 0x20000, 0x8000, 0xceb77765 ) /* tiles */
	ROM_LOAD( "SA16.BIN", 0x28000, 0x8000, 0xd2d2105e ) /* tiles */
	ROM_LOAD( "SA18.BIN", 0x30000, 0x8000, 0x184ae81c ) /* tiles */
	ROM_LOAD( "SA20.BIN", 0x38000, 0x8000, 0xf8089804 ) /* tiles */
	ROM_LOAD( "SA22.BIN", 0x40000, 0x8000, 0xecfb6047 ) /* tiles */
	ROM_LOAD( "SA10.BIN", 0x48000, 0x8000, 0xd21e2362 ) /* sprites */
	ROM_LOAD( "SA12.BIN", 0x50000, 0x8000, 0xe2f99cc3 ) /* sprites */
	ROM_LOAD( "SA06.BIN", 0x58000, 0x8000, 0xc805167b ) /* sprites */
	ROM_LOAD( "SA08.BIN", 0x60000, 0x8000, 0x778f5043 ) /* sprites */
	ROM_LOAD( "SA11.BIN", 0x68000, 0x8000, 0x63102ab8 ) /* sprites */
	ROM_LOAD( "SA13.BIN", 0x70000, 0x8000, 0x863bc3f1 ) /* sprites */
	ROM_LOAD( "SA07.BIN", 0x78000, 0x8000, 0x770e34ca ) /* sprites */
	ROM_LOAD( "SA09.BIN", 0x80000, 0x8000, 0x70aa3e6c ) /* sprites */

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "SA05.BIN", 0x0000, 0x8000, 0xc8e97c53 )

	ROM_REGION(0x08000) /* 32k tile map */
	ROM_LOAD( "SA14.BIN", 0x0000, 0x8000, 0x4c6ef4e6 )

#ifdef THIRD_CPU
	ROM_REGION(0x10000) /* 64k for CPU */
	ROM_LOAD( "SA23.BIN", 0x0000, 0x8000, 0x70faa88e )
#endif
ROM_END

ROM_START( sidearjp_rom )
	ROM_REGION(0x20000)     /* 64k for code + banked ROMs images */
	ROM_LOAD( "a_15e.rom", 0x00000, 0x08000, 0x405f5869 )  /* CODE */
	ROM_LOAD( "a_14e.rom", 0x10000, 0x08000, 0x9ae56dc9 )  /* 0+1 */
	ROM_LOAD( "a_12e.rom", 0x18000, 0x08000, 0xd334d882 )  /* 2+3 */

	ROM_REGION(0x88000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_10j.rom", 0x00000, 0x8000, 0xe0d00000 )  /* characters */
	ROM_LOAD( "b_13d.rom", 0x08000, 0x8000, 0xa0101f3e ) /* tiles */
	ROM_LOAD( "b_13e.rom", 0x10000, 0x8000, 0x89f8efec ) /* tiles */
	ROM_LOAD( "b_13f.rom", 0x18000, 0x8000, 0x31c9b645 ) /* tiles */
	ROM_LOAD( "b_13g.rom", 0x20000, 0x8000, 0xceb77765 ) /* tiles */
	ROM_LOAD( "b_14d.rom", 0x28000, 0x8000, 0xd2d2105e ) /* tiles */
	ROM_LOAD( "b_14e.rom", 0x30000, 0x8000, 0x184ae81c ) /* tiles */
	ROM_LOAD( "b_14f.rom", 0x38000, 0x8000, 0xf8089804 ) /* tiles */
	ROM_LOAD( "b_14g.rom", 0x40000, 0x8000, 0xecfb6047 ) /* tiles */
	ROM_LOAD( "b_11b.rom", 0x48000, 0x8000, 0xd21e2362 ) /* sprites */
	ROM_LOAD( "b_13b.rom", 0x50000, 0x8000, 0xe2f99cc3 ) /* sprites */
	ROM_LOAD( "b_11a.rom", 0x58000, 0x8000, 0xc805167b ) /* sprites */
	ROM_LOAD( "b_13a.rom", 0x60000, 0x8000, 0x778f5043 ) /* sprites */
	ROM_LOAD( "b_12b.rom", 0x68000, 0x8000, 0x63102ab8 ) /* sprites */
	ROM_LOAD( "b_14b.rom", 0x70000, 0x8000, 0x863bc3f1 ) /* sprites */
	ROM_LOAD( "b_12a.rom", 0x78000, 0x8000, 0x770e34ca ) /* sprites */
	ROM_LOAD( "b_14a.rom", 0x80000, 0x8000, 0x70aa3e6c ) /* sprites */

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom", 0x0000, 0x8000, 0xc8e97c53 )

	ROM_REGION(0x08000) /* 32k tile map */
	ROM_LOAD( "b_03d.rom", 0x0000, 0x8000, 0x4c6ef4e6 )

#ifdef THIRD_CPU
	ROM_REGION(0x10000) /* 64k for CPU */
	ROM_LOAD( "b_11j.rom", 0x0000, 0x8000, 0x70faa88e )
#endif
ROM_END


static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xe680],"\x00\x00\x00\x01\x00\x00\x00\x00",8) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        osd_fread(f,&RAM[0xe680],16*5);

                        memcpy(&RAM[0xe600], &RAM[0xe680], 8);
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
                osd_fwrite(f,&RAM[0xe680],16*5);
		osd_fclose(f);
	}
}



struct GameDriver sidearms_driver =
{
	"Sidearms",
	"sidearms",
	"Paul Leaman (MAME driver)\nNicola Salmoria (additional code)\nGerrit Van Goethem (high score save)",
	&sidearms_machine_driver,

	sidearms_rom,
	0,0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_table, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver sidearjp_driver =
{
	"Sidearms (Japanese)",
	"sidearjp",
	"Paul Leaman (MAME driver)\nNicola Salmoria (additional code)\nGerrit Van Goethem (high score save)",
	&sidearms_machine_driver,

	sidearjp_rom,
	0,0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_table, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};
