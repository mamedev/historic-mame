/***************************************************************************

Wiz memory map (preliminary)

This board is similar to a Galaxian board in the way it handles scrolling
and sprites, but the similarities pretty much end there. The most notable
difference is that there are 2 independently scrollable playfields.

Strangely, one of the foreground character banks have a Seibu logo, the
other one a Taito logo. There are other differences as well, so you can't
just switch one for the other.


Main CPU:

0000-BFFF  ROM
C000-C7FF  RAM
D000-D3FF  Video RAM (Foreground)
D400-D7FF  Color RAM (Foreground)
D800-D83F  Scroll RAM (Foreground)
D840-D85F  Sprite RAM 1
E000-E3FF  Video RAM (Background)
E400-E7FF  Color RAM (Background) (I don't think it's used)
E800-E83F  Scroll RAM (Background)
E840-E85F  Sprite RAM 2

I/O read:
f000 DIP SW#1
f008 DIP SW#2
f010 Input Port 1
f010 Input Port 2

I/O write:
c800 Puslated to 1 when a coin is inserted into Chute A
c801 Puslated to 1 when a coin is inserted into Chute B
f000 Sprite bank select
f001 NMI enable
f002 \ (?) Set based on current level (C6E9)
f003 / (?) Possibly palette select
f004 \ Character bank select
f005 /
f800 Sound Command write
f818 (?) Sound (Is there some other sound circuit??)


Sound CPU:

0000-1FFF  ROM
2000-23FF  RAM

I/O read:
7000 Sound Command Read

I/O write:
4000 AY8910 Control Port #1
4001 AY8910 Write Port #1
5000 AY8910 Control Port #2
5001 AY8910 Write Port #2
6000 AY8910 Control Port #3
6001 AY8910 Write Port #3
7000 NMI enable

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *stinger_videoram2;
extern unsigned char *stinger_fg_attributesram,*stinger_bg_attributesram;

void stinger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void stinger_attributes_w(int offset,int data);
void stinger_palettebank_w(int offset,int data);
void stinger_charbank_w(int offset,int data);
void stinger_flipscreen_w(int offset,int data);
void stinger_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);




void sound_command_w(int offset,int data)
{
	if (data == 0x90)
	{
		/* ??? */
	}
	else
		soundlatch_w(0,data);	/* ??? */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd85f, MRA_RAM },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe800, 0xe85f, MRA_RAM },
	{ 0xf000, 0xf000, input_port_0_r },	/* DSW0 */
	{ 0xf008, 0xf008, input_port_1_r },	/* DSW1 */
	{ 0xf010, 0xf010, input_port_2_r },	/* IN0 */
	{ 0xf018, 0xf018, input_port_3_r },	/* IN1 */
	{ 0xf800, 0xf800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd3ff, MWA_RAM, &stinger_videoram2 },
	{ 0xd800, 0xd83f, MWA_RAM, &stinger_fg_attributesram },
	{ 0xd840, 0xd85f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe800, 0xe83f, stinger_attributes_w, &stinger_bg_attributesram },
	{ 0xe840, 0xe85f, MWA_RAM, &spriteram_2 },
	{ 0xf001, 0xf001, interrupt_enable_w },
	{ 0xf002, 0xf003, stinger_palettebank_w },	/* ??? guess! */
	{ 0xf004, 0xf005, stinger_charbank_w },
	{ 0xf006, 0xf007, stinger_flipscreen_w },
	{ 0xf800, 0xf800, sound_command_w },
	{ 0xf808, 0xf808, MWA_NOP },	/* explosion sound trigger; analog? */
	{ 0xf80a, 0xf80a, MWA_NOP },	/* shoot sound trigger; analog? */
	{ 0xf818, 0xf818, MWA_NOP },	/* ??? */
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x3000, 0x3000, interrupt_enable_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x5001, 0x5001, AY8910_write_port_0_w },
	{ 0x6000, 0x6000, AY8910_control_port_1_w },
	{ 0x6001, 0x6001, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( stinger_input_ports )
	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x18, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x70, 0x70, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x60, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/8 Credits" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( scion_input_ports )
	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x18, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 characters */
	3,		/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprintes */
	128,	/* 128 sprites */
	3,		/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 32 },
	{ 1, 0x6000, &charlayout,   0, 32 },
	{ 1, 0x1000, &spritelayout, 0, 32 },
	{ 1, 0x7000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ?? */
	{ 255, 255 },
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
			3072000,	/* 3.072 MHz ? */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz ? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,2	/* ??? */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	stinger_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	stinger_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



ROM_START( stinger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "n1.bin",       0x0000, 0x2000, 0xf2d2790c )
	ROM_LOAD( "n2.bin",       0x2000, 0x2000, 0x8fd2d8d8 )
	ROM_LOAD( "n3.bin",       0x4000, 0x2000, 0xf1794d36 )
	ROM_LOAD( "n4.bin",       0x6000, 0x2000, 0x230ba682 )
	ROM_LOAD( "n5.bin",       0x8000, 0x2000, 0xa03a01da )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7.bin",        0x0000, 0x2000, 0x775489be )
	ROM_LOAD( "8.bin",        0x2000, 0x2000, 0x43c61b3f )
	ROM_LOAD( "9.bin",        0x4000, 0x2000, 0xc9ed8fc7 )
	ROM_LOAD( "10.bin",       0x6000, 0x2000, 0xf6721930 )
	ROM_LOAD( "11.bin",       0x8000, 0x2000, 0xa4404e63 )
	ROM_LOAD( "12.bin",       0xa000, 0x2000, 0xb60fa88c )

	ROM_REGION(0x0300)	/* color PROMs */
	ROM_LOAD( "stinger.a7",   0x0000, 0x0100, 0x52c06fc2 )	/* red component */
	ROM_LOAD( "stinger.b7",   0x0100, 0x0100, 0x9985e575 )	/* green component */
	ROM_LOAD( "stinger.a8",   0x0200, 0x0100, 0x76b57629 )	/* blue component */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6.bin",        0x0000, 0x2000, 0x79757f0c )
ROM_END

ROM_START( scion_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sc1",          0x0000, 0x2000, 0x8dcad575 )
	ROM_LOAD( "sc2",          0x2000, 0x2000, 0xf608e0ba )
	ROM_LOAD( "sc3",          0x4000, 0x2000, 0x915289b9 )
	ROM_LOAD( "4.9j",         0x6000, 0x2000, 0x0f40d002 )
	ROM_LOAD( "5.10j",        0x8000, 0x2000, 0xdc4923b7 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7.10e",        0x0000, 0x2000, 0x223e0d2a )
	ROM_LOAD( "8.12e",        0x2000, 0x2000, 0xd3e39b48 )
	ROM_LOAD( "9.15e",        0x4000, 0x2000, 0x630861b5 )
	ROM_LOAD( "10.10h",       0x6000, 0x2000, 0x0d2a0d1e )
	ROM_LOAD( "11.12h",       0x8000, 0x2000, 0xdc6ef8ab )
	ROM_LOAD( "12.15h",       0xa000, 0x2000, 0xc82c28bf )

	ROM_REGION(0x0300)	/* color PROMs */
	ROM_LOAD( "82s129.7a",    0x0000, 0x0100, 0x2f89d9ea )	/* red component */
	ROM_LOAD( "82s129.7b",    0x0100, 0x0100, 0xba151e6a )	/* green component */
	ROM_LOAD( "82s129.8a",    0x0200, 0x0100, 0xf681ce59 )	/* blue component */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "sc6",         0x0000, 0x2000, 0x09f5f9c1 )
ROM_END

ROM_START( scionc_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1.5j",         0x0000, 0x2000, 0x5aaf571e )
	ROM_LOAD( "2.6j",         0x2000, 0x2000, 0xd5a66ac9 )
	ROM_LOAD( "3.8j",         0x4000, 0x2000, 0x6e616f28 )
	ROM_LOAD( "4.9j",         0x6000, 0x2000, 0x0f40d002 )
	ROM_LOAD( "5.10j",        0x8000, 0x2000, 0xdc4923b7 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7.10e",        0x0000, 0x2000, 0x223e0d2a )
	ROM_LOAD( "8.12e",        0x2000, 0x2000, 0xd3e39b48 )
	ROM_LOAD( "9.15e",        0x4000, 0x2000, 0x630861b5 )
	ROM_LOAD( "10.10h",       0x6000, 0x2000, 0x0d2a0d1e )
	ROM_LOAD( "11.12h",       0x8000, 0x2000, 0xdc6ef8ab )
	ROM_LOAD( "12.15h",       0xa000, 0x2000, 0xc82c28bf )

	ROM_REGION(0x0300)	/* color PROMs */
	ROM_LOAD( "82s129.7a",    0x0000, 0x0100, 0x2f89d9ea )	/* red component */
	ROM_LOAD( "82s129.7b",    0x0100, 0x0100, 0xba151e6a )	/* green component */
	ROM_LOAD( "82s129.8a",    0x0200, 0x0100, 0xf681ce59 )	/* blue component */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6.9f",         0x0000, 0x2000, 0xa66a0ce6 )
ROM_END


static void stinger_decode(void)
{
	static const unsigned char xortable[4][4] =
	{
		{ 0xa0,0x88,0x88,0xa0 },	/* .........00.0... */
		{ 0x88,0x00,0xa0,0x28 },	/* .........00.1... */
		{ 0x80,0xa8,0x20,0x08 },	/* .........01.0... */
		{ 0x28,0x28,0x88,0x88 }		/* .........01.1... */
	};
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0x0000;A < 0xc000;A++)
	{
		int row,col;
		unsigned char src;


		if (A & 0x2040)
		{
			/* not encrypted */
			ROM[A] = RAM[A];
		}
		else
		{
			src = RAM[A];

			/* pick the translation table from bits 3 and 5 */
			row = ((A >> 3) & 1) + (((A >> 5) & 1) << 1);

			/* pick the offset in the table from bits 3 and 5 of the source data */
			col = ((src >> 3) & 1) + (((src >> 5) & 1) << 1);
			/* the bottom half of the translation table is the mirror image of the top */
			if (src & 0x80) col = 3 - col;

			/* decode the opcodes */
			ROM[A] = src ^ xortable[row][col];
		}
	}
}



struct GameDriver stinger_driver =
{
	__FILE__,
	0,
	"stinger",
	"Stinger",
	"1983",
	"Seibu Denshi",
	"Nicola Salmoria",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	stinger_rom,
	0, stinger_decode,
	0,
	0,

	stinger_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver scion_driver =
{
	__FILE__,
	0,
	"scion",
	"Scion",
	"1984",
	"Seibu Denshi",
	"Nicola Salmoria",
	GAME_IMPERFECT_COLORS,
	&machine_driver,
	0,

	scion_rom,
	0, 0,
	0,
	0,

	scion_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver scionc_driver =
{
	__FILE__,
	&scion_driver,
	"scionc",
	"Scion (Cinematronics)",
	"1984",
	"Seibu Denshi [Cinematronics license]",
	"Nicola Salmoria",
	GAME_IMPERFECT_COLORS,
	&machine_driver,
	0,

	scionc_rom,
	0, 0,
	0,
	0,

	scion_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

