/*************************************************************************
 Universal Cheeky Mouse Driver
 (c)Lee Taylor May/June 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distributed as part of any commerical work.
**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"




void cheekymouse_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cheekymouse_sprite_w(int offset, int data);


static struct MemoryReadAddress cheekymouse_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM},
	{ 0x3000, 0x33ff, MRA_RAM},
	{ 0x3800, 0x3bff, MRA_RAM},	/* screen RAM */
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress cheekymouse_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ -1 }	/* end of table */
};

static struct IOReadPort cheekymouse_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort cheekymouse_writeport[] =
{
	{ 0x00, 0x3f, cheekymouse_sprite_w },
	{ -1 }	/* end of table */
};


int cheekymouse_interrupt(void)
{
	if (readinputport(2) & 1)	/* Left Coin */
		return nmi_interrupt();
	else return interrupt();
}


INPUT_PORTS_START( cheekymouse_input_ports )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x0c, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x04,IP_ACTIVE_LOW,IPT_BUTTON1,"?",OSD_KEY_A,IP_JOY_DEFAULT,0)
	PORT_BITX(0x08,IP_ACTIVE_LOW,IPT_BUTTON1,"?",OSD_KEY_S,IP_JOY_DEFAULT,0)
	PORT_BITX(0x10,IP_ACTIVE_LOW,IPT_BUTTON1,"?",OSD_KEY_D,IP_JOY_DEFAULT,0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin  causes a NMI, */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END



static struct GfxLayout cheekymouse_charlayout =
{
	8,8,	/* 16*16 sprites */
	256,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout cheekymouse_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*32*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo cheekymouse_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &cheekymouse_charlayout,    0,  8 },
	{ 1, 0x1000, &cheekymouse_spritelayout,  0,  8 },
	{ -1 } /* end of array */
};


static struct MachineDriver cheekymouse_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1081600,
			0,
			cheekymouse_readmem,cheekymouse_writemem,
			cheekymouse_readport, cheekymouse_writeport,
			cheekymouse_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 32*8, { 0*8, 32*8-1, 4*8, 28*8-1 },
	cheekymouse_gfxdecodeinfo,
	32,32,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	cheekymouse_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cheekymouse_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cm03.c5",       0x0000, 0x0800, 0x1ad0cb40 )
	ROM_LOAD( "cm04.c6",       0x0800, 0x0800, 0x2238f607 )
	ROM_LOAD( "cm05.c7",       0x1000, 0x0800, 0x4169eba8 )
	ROM_LOAD( "cm06.c8",       0x1800, 0x0800, 0x7031660c )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cm01.c1",       0x0000, 0x0800, 0x26f73bd7 )
	ROM_LOAD( "cm02.c2",       0x0800, 0x0800, 0x885887c3 )
	ROM_LOAD( "cm07.n5",       0x1000, 0x0800, 0x2738c88d )
	ROM_LOAD( "cm08.n6",       0x1800, 0x0800, 0xb3fbd4ac )

	ROM_REGION(0x0060)	/* PROMs */
	ROM_LOAD( "cm.m8",         0x0000, 0x0020, 0x2386bc68 )
	ROM_LOAD( "cm.m9",         0x0020, 0x0020, 0xdb9c59a5 )
	ROM_LOAD( "cm.p3",         0x0040, 0x0020, 0x6ac41516 )
ROM_END



struct GameDriver cheekyms_driver =
{
	__FILE__,
	0,
	"cheekyms",
	"Cheeky Mouse",
	"1980?",
	"Universal",
	"Lee Taylor\nChris Moore",
	GAME_NOT_WORKING | GAME_WRONG_COLORS,
	&cheekymouse_machine_driver,
	0,

	cheekymouse_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	cheekymouse_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
