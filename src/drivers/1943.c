/*

TODO: 1943 is almost identical to GunSmoke (one more scrolling playfield). We
      should merge the two drivers.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



extern unsigned char *c1943_scrollx;
extern unsigned char *c1943_scrolly;
extern unsigned char *c1943_bgscrolly;
void c1943_c804_w(int offset,int data);	/* in vidhrdw/c1943.c */
void c1943_d806_w(int offset,int data);	/* in vidhrdw/c1943.c */
void c1943_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void c1943_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int c1943_vh_start(void);
void c1943_vh_stop(void);



/* this is a protection check. The game crashes (thru a jump to 0x8000) */
/* if a read from this address doesn't return the value it expects. */
static int c1943_protection_r(int offset)
{
	Z80_Regs regs;


	Z80_GetRegs(&regs);
	if (errorlog) fprintf(errorlog,"protection read, PC: %04x Result:%02x\n",cpu_getpc(),regs.BC.B.h);
	return regs.BC.B.h;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xc007, 0xc007, c1943_protection_r },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, c1943_c804_w },	/* ROM bank switch, screen flip */
	{ 0xc806, 0xc806, watchdog_reset_w },
	{ 0xc807, 0xc807, MWA_NOP }, 	/* protection chip write (we don't emulate it) */
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &c1943_scrolly },
	{ 0xd802, 0xd802, MWA_RAM, &c1943_scrollx },
	{ 0xd803, 0xd804, MWA_RAM, &c1943_bgscrolly },
	{ 0xd806, 0xd806, c1943_d806_w },	/* sprites, bg1, bg2 enable */
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* actually, this is VBLANK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "1 (Easiest)" )
	PORT_DIPSETTING(    0x0e, "2" )
	PORT_DIPSETTING(    0x0d, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0b, "5" )
	PORT_DIPSETTING(    0x0a, "6" )
	PORT_DIPSETTING(    0x09, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPSETTING(    0x06, "10" )
	PORT_DIPSETTING(    0x05, "11" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x03, "13" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPSETTING(    0x01, "15" )
	PORT_DIPSETTING(    0x00, "16 (Hardest)" )
	PORT_DIPNAME( 0x10, 0x10, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout fgtilelayout =
{
	32,32,  /* 32*32 tiles */
	512,    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ 512*256*8+4, 512*256*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	{ 192*8+8+3, 192*8+8+2, 192*8+8+1, 192*8+8+0, 192*8+3, 192*8+2, 192*8+1, 192*8+0,
			128*8+8+3, 128*8+8+2, 128*8+8+1, 128*8+8+0, 128*8+3, 128*8+2, 128*8+1, 128*8+0,
			64*8+8+3, 64*8+8+2, 64*8+8+1, 64*8+8+0, 64*8+3, 64*8+2, 64*8+1, 64*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	256*8	/* every tile takes 256 consecutive bytes */
};
static struct GfxLayout bgtilelayout =
{
	32,32,  /* 32*32 tiles */
	128,    /* 128 tiles */
	4,      /* 4 bits per pixel */
	{ 128*256*8+4, 128*256*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	{ 192*8+8+3, 192*8+8+2, 192*8+8+1, 192*8+8+0, 192*8+3, 192*8+2, 192*8+1, 192*8+0,
			128*8+8+3, 128*8+8+2, 128*8+8+1, 128*8+8+0, 128*8+3, 128*8+2, 128*8+1, 128*8+0,
			64*8+8+3, 64*8+8+2, 64*8+8+1, 64*8+8+0, 64*8+3, 64*8+2, 64*8+1, 64*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	256*8	/* every tile takes 256 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,                  0, 32 },
	{ 1, 0x08000, &fgtilelayout,             32*4, 16 },
	{ 1, 0x48000, &bgtilelayout,       32*4+16*16, 16 },
	{ 1, 0x58000, &spritelayout, 32*4+16*16+16*16, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz */
	{ YM2203_VOL(100,255), YM2203_VOL(100,255) },
	{ 0 },
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
			6000000,	/* 6 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,32*4+16*16+16*16+16*16,
	c1943_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	c1943_vh_start,
	c1943_vh_stop,
	c1943_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( c1943_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943.01", 0x00000, 0x08000, 0xc2349190 )
	ROM_LOAD( "1943.02", 0x10000, 0x10000, 0x7f84dc22 )
	ROM_LOAD( "1943.03", 0x20000, 0x10000, 0x727252b4 )

	ROM_REGION(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943.04", 0x00000, 0x8000, 0xa31cdad4 )	/* characters */
	ROM_LOAD( "1943.15", 0x08000, 0x8000, 0x3b682598 )	/* bg tiles */
	ROM_LOAD( "1943.16", 0x10000, 0x8000, 0xb9d72251 )
	ROM_LOAD( "1943.17", 0x18000, 0x8000, 0xf8b4585a )
	ROM_LOAD( "1943.18", 0x20000, 0x8000, 0xd50b5d99 )
	ROM_LOAD( "1943.19", 0x28000, 0x8000, 0x5f3e4214 )
	ROM_LOAD( "1943.20", 0x30000, 0x8000, 0xa9842054 )
	ROM_LOAD( "1943.21", 0x38000, 0x8000, 0x948cd1ba )
	ROM_LOAD( "1943.22", 0x40000, 0x8000, 0x76e6bdc4 )
	ROM_LOAD( "1943.24", 0x48000, 0x8000, 0xe485625f )	/* fg tiles */
	ROM_LOAD( "1943.25", 0x50000, 0x8000, 0x402e1da0 )
	ROM_LOAD( "1943.06", 0x58000, 0x8000, 0xf811ba7d )	/* sprites */
	ROM_LOAD( "1943.07", 0x60000, 0x8000, 0xa796e820 )
	ROM_LOAD( "1943.08", 0x68000, 0x8000, 0x867972c1 )
	ROM_LOAD( "1943.09", 0x70000, 0x8000, 0xe87975d1 )
	ROM_LOAD( "1943.10", 0x78000, 0x8000, 0xbf07571d )
	ROM_LOAD( "1943.11", 0x80000, 0x8000, 0xe33ec696 )
	ROM_LOAD( "1943.12", 0x88000, 0x8000, 0xb1ba27ca )
	ROM_LOAD( "1943.13", 0x90000, 0x8000, 0x7d843cf0 )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "BMPROM.01", 0x0000, 0x0100, 0x9f5b0f03 )	/* red component */
	ROM_LOAD( "BMPROM.02", 0x0100, 0x0100, 0x75090d01 )	/* green component */
	ROM_LOAD( "BMPROM.03", 0x0200, 0x0100, 0xc66c030a )	/* blue component */
	ROM_LOAD( "BMPROM.05", 0x0300, 0x0100, 0xe7ff020d )	/* char lookup table */
	ROM_LOAD( "BMPROM.10", 0x0400, 0x0100, 0x31f90f0b )	/* foreground lookup table */
	ROM_LOAD( "BMPROM.09", 0x0500, 0x0100, 0x3e740202 )	/* foreground palette bank */
	ROM_LOAD( "BMPROM.12", 0x0600, 0x0100, 0xf2390703 )	/* background lookup table */
	ROM_LOAD( "BMPROM.11", 0x0700, 0x0100, 0x94b80000 )	/* background palette bank */
	ROM_LOAD( "BMPROM.08", 0x0800, 0x0100, 0xafa10f0f )	/* sprite lookup table */
	ROM_LOAD( "BMPROM.07", 0x0900, 0x0100, 0x6d7d0201 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05", 0x00000, 0x8000, 0xd52cd1ee )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943.14", 0x0000, 0x8000, 0x9f128a0c )	/* front background */
	ROM_LOAD( "1943.23", 0x8000, 0x8000, 0xf96f6429 )	/* back background */
ROM_END

ROM_START( c1943jap_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943jap.001", 0x00000, 0x08000, 0x73e25a22 )
	ROM_LOAD( "1943jap.002", 0x10000, 0x10000, 0xf4a7037f )
	ROM_LOAD( "1943jap.003", 0x20000, 0x10000, 0xed95ab3d )

	ROM_REGION(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943.04", 0x00000, 0x8000, 0xa31cdad4 )	/* characters */
	ROM_LOAD( "1943.15", 0x08000, 0x8000, 0x3b682598 )	/* bg tiles */
	ROM_LOAD( "1943.16", 0x10000, 0x8000, 0xb9d72251 )
	ROM_LOAD( "1943.17", 0x18000, 0x8000, 0xf8b4585a )
	ROM_LOAD( "1943.18", 0x20000, 0x8000, 0xd50b5d99 )
	ROM_LOAD( "1943.19", 0x28000, 0x8000, 0x5f3e4214 )
	ROM_LOAD( "1943.20", 0x30000, 0x8000, 0xa9842054 )
	ROM_LOAD( "1943.21", 0x38000, 0x8000, 0x948cd1ba )
	ROM_LOAD( "1943.22", 0x40000, 0x8000, 0x76e6bdc4 )
	ROM_LOAD( "1943.24", 0x48000, 0x8000, 0xe485625f )	/* fg tiles */
	ROM_LOAD( "1943.25", 0x50000, 0x8000, 0x402e1da0 )
	ROM_LOAD( "1943.06", 0x58000, 0x8000, 0xf811ba7d )	/* sprites */
	ROM_LOAD( "1943.07", 0x60000, 0x8000, 0xa796e820 )
	ROM_LOAD( "1943.08", 0x68000, 0x8000, 0x867972c1 )
	ROM_LOAD( "1943.09", 0x70000, 0x8000, 0xe87975d1 )
	ROM_LOAD( "1943.10", 0x78000, 0x8000, 0xbf07571d )
	ROM_LOAD( "1943.11", 0x80000, 0x8000, 0xe33ec696 )
	ROM_LOAD( "1943.12", 0x88000, 0x8000, 0xb1ba27ca )
	ROM_LOAD( "1943.13", 0x90000, 0x8000, 0x7d843cf0 )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "BMPROM.01", 0x0000, 0x0100, 0x9f5b0f03 )	/* red component */
	ROM_LOAD( "BMPROM.02", 0x0100, 0x0100, 0x75090d01 )	/* green component */
	ROM_LOAD( "BMPROM.03", 0x0200, 0x0100, 0xc66c030a )	/* blue component */
	ROM_LOAD( "BMPROM.05", 0x0300, 0x0100, 0xe7ff020d )	/* char lookup table */
	ROM_LOAD( "BMPROM.10", 0x0400, 0x0100, 0x31f90f0b )	/* foreground lookup table */
	ROM_LOAD( "BMPROM.09", 0x0500, 0x0100, 0x3e740202 )	/* foreground palette bank */
	ROM_LOAD( "BMPROM.12", 0x0600, 0x0100, 0xf2390703 )	/* background lookup table */
	ROM_LOAD( "BMPROM.11", 0x0700, 0x0100, 0x94b80000 )	/* background palette bank */
	ROM_LOAD( "BMPROM.08", 0x0800, 0x0100, 0xafa10f0f )	/* sprite lookup table */
	ROM_LOAD( "BMPROM.07", 0x0900, 0x0100, 0x6d7d0201 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05", 0x00000, 0x8000, 0xd52cd1ee )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943.14", 0x0000, 0x8000, 0x9f128a0c )	/* front background */
	ROM_LOAD( "1943.23", 0x8000, 0x8000, 0xf96f6429 )	/* back background */
ROM_END

ROM_START( c1943kai_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943kai.01", 0x00000, 0x08000, 0xaf0dbc29 )
	ROM_LOAD( "1943kai.02", 0x10000, 0x10000, 0x6762b9c6 )
	ROM_LOAD( "1943kai.03", 0x20000, 0x10000, 0x40740b6c )

	ROM_REGION(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943kai.04", 0x00000, 0x8000, 0xd5473d21 )	/* characters */
	ROM_LOAD( "1943kai.15", 0x08000, 0x8000, 0x3b682598 )	/* bg tiles */
	ROM_LOAD( "1943kai.16", 0x10000, 0x8000, 0xada0672a )
	ROM_LOAD( "1943kai.17", 0x18000, 0x8000, 0x30a3ab21 )
	ROM_LOAD( "1943kai.18", 0x20000, 0x8000, 0xcac83bf6 )
	ROM_LOAD( "1943kai.19", 0x28000, 0x8000, 0x5f3e4214 )
	ROM_LOAD( "1943kai.20", 0x30000, 0x8000, 0x6af8d78a )
	ROM_LOAD( "1943kai.21", 0x38000, 0x8000, 0x76d36635 )
	ROM_LOAD( "1943kai.22", 0x40000, 0x8000, 0x29613a69 )
	ROM_LOAD( "1943kai.24", 0x48000, 0x8000, 0xe4aa3382 )	/* fg tiles */
	ROM_LOAD( "1943kai.25", 0x50000, 0x8000, 0xaee0eafc )
	ROM_LOAD( "1943kai.06", 0x58000, 0x8000, 0x16a25870 )	/* sprites */
	ROM_LOAD( "1943kai.07", 0x60000, 0x8000, 0x4749d641 )
	ROM_LOAD( "1943kai.08", 0x68000, 0x8000, 0x337ab0e4 )
	ROM_LOAD( "1943kai.09", 0x70000, 0x8000, 0xf0d0a3d4 )
	ROM_LOAD( "1943kai.10", 0x78000, 0x8000, 0xdf8a9d28 )
	ROM_LOAD( "1943kai.11", 0x80000, 0x8000, 0x7ee315e5 )
	ROM_LOAD( "1943kai.12", 0x88000, 0x8000, 0x2bc1a45b )
	ROM_LOAD( "1943kai.13", 0x90000, 0x8000, 0x46851283 )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "BMK01.BIN", 0x0000, 0x0100, 0xf1940302 )	/* red component */
	ROM_LOAD( "BMK02.BIN", 0x0100, 0x0100, 0x8e1e0a00 )	/* green component */
	ROM_LOAD( "BMK03.BIN", 0x0200, 0x0100, 0xdc96070c )	/* blue component */
	ROM_LOAD( "BMK05.BIN", 0x0300, 0x0100, 0x2ba30b0b )	/* char lookup table */
	ROM_LOAD( "BMK10.BIN", 0x0400, 0x0100, 0x8052050c )	/* foreground lookup table */
	ROM_LOAD( "BMK09.BIN", 0x0500, 0x0100, 0x4c850203 )	/* foreground palette bank */
	ROM_LOAD( "BMK12.BIN", 0x0600, 0x0100, 0xb5500e04 )	/* background lookup table */
	ROM_LOAD( "BMK11.BIN", 0x0700, 0x0100, 0xafc40102 )	/* background palette bank */
	ROM_LOAD( "BMK08.BIN", 0x0800, 0x0100, 0xb98b0f0f )	/* sprite lookup table */
	ROM_LOAD( "BMK07.BIN", 0x0900, 0x0100, 0x907a0300 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943kai.05", 0x00000, 0x8000, 0x3cf49db4 )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943kai.14", 0x0000, 0x8000, 0xc0d1e091 )	/* front background */
	ROM_LOAD( "1943kai.23", 0x8000, 0x8000, 0x4ff43b94 )	/* back background */
ROM_END



static int c1943_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe600],"\x00\x00\x00\x02\x00\x00\x00\x00\x1D\x0A\x0E\x24\x24\x24\x24\x24",16) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			// High score table.
			osd_fread(f,&RAM[0xe600],0x60);

			// High score.
			osd_fread(f,&RAM[0xe110],8);

			// High score screen.
			osd_fread(f,&RAM[0xd1be],1);
			osd_fread(f,&RAM[0xd1de],1);
			osd_fread(f,&RAM[0xd1fe],1);
			osd_fread(f,&RAM[0xd21e],1);
			osd_fread(f,&RAM[0xd23e],1);
			osd_fread(f,&RAM[0xd25e],1);
			osd_fread(f,&RAM[0xd27e],1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void c1943_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		// High score table.
		osd_fwrite(f,&RAM[0xe600],0x60);

		// High score.
		osd_fwrite(f,&RAM[0xe110],8);

		// High score screen.
		osd_fwrite(f,&RAM[0xd1be],1);
		osd_fwrite(f,&RAM[0xd1de],1);
		osd_fwrite(f,&RAM[0xd1fe],1);
		osd_fwrite(f,&RAM[0xd21e],1);
		osd_fwrite(f,&RAM[0xd23e],1);
		osd_fwrite(f,&RAM[0xd25e],1);
		osd_fwrite(f,&RAM[0xd27e],1);

		osd_fclose(f);
	}
}


static int c1943kai_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe600],"\x00\x00\x02\x00\x00\x00\x00\x00\x1D\x0A\x0E\x24\x24\x24\x24\x24",16) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			// High score table.
			osd_fread(f,&RAM[0xe600],0x60);

			// High score.
			osd_fread(f,&RAM[0xe110],8);

			// High score screen.
			osd_fread(f,&RAM[0xd1be],1);
			osd_fread(f,&RAM[0xd1de],1);
			osd_fread(f,&RAM[0xd1fe],1);
			osd_fread(f,&RAM[0xd21e],1);
			osd_fread(f,&RAM[0xd23e],1);
			osd_fread(f,&RAM[0xd25e],1);
			osd_fread(f,&RAM[0xd27e],1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void c1943kai_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		// High score table.
		osd_fwrite(f,&RAM[0xe600],0x60);

		// High score.
		osd_fwrite(f,&RAM[0xe110],8);

		// High score screen.
		osd_fwrite(f,&RAM[0xd1be],1);
		osd_fwrite(f,&RAM[0xd1de],1);
		osd_fwrite(f,&RAM[0xd1fe],1);
		osd_fwrite(f,&RAM[0xd21e],1);
		osd_fwrite(f,&RAM[0xd23e],1);
		osd_fwrite(f,&RAM[0xd25e],1);
		osd_fwrite(f,&RAM[0xd27e],1);

		osd_fclose(f);
	}
}



struct GameDriver c1943_driver =
{
	__FILE__,
	0,
	"1943",
	"1943 (US)",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)\nJeff Johnson (high score save)",
	0,
	&machine_driver,

	c1943_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943_hiload, c1943_hisave
};

struct GameDriver c1943jap_driver =
{
	__FILE__,
	&c1943_driver,
	"1943jap",
	"1943 (Japan)",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)\nJeff Johnson (high score save)",
	0,
	&machine_driver,

	c1943jap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943_hiload, c1943_hisave
};

struct GameDriver c1943kai_driver =
{
	__FILE__,
	0,
	"1943kai",
	"1943 Kai",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)\nJeff Johnson (high score save)\nGerrit Van Goethem (high score fix)",
	0,
	&machine_driver,

	c1943kai_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943kai_hiload, c1943kai_hisave
};
