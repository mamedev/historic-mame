/***************************************************************************

Eggs memory map (preliminary)

0000-07ff RAM
1000-13ff Video RAM
1800-181f Sprites
1800-1bff Mirror address of video RAM, but x and y coordinates are swapped
3000-7fff ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



void eggs_mirrorvideoram_w(int offset,int data);
void eggs_vh_screenrefresh(struct osd_bitmap *bitmap);


int pop(int offs)
{
	int res;


	res = readinputport(2);

	return res;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x2000, 0x2000, input_port_2_r },
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x13ff, MRA_RAM },
	{ 0x3000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* reset/interrupt vectors */
	{ 0x2002, 0x2002, input_port_0_r },
	{ 0x2003, 0x2003, input_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1800, 0x181f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1800, 0x1bff, eggs_mirrorvideoram_w },
	{ 0x3000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, OSD_KEY_3 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, IPB_VBLANK },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x01, "LIVES", { "5", "3" }, 1 },
	{ 2, 0x02, "SW2", { "0", "1" } },
	{ 2, 0x04, "SW3", { "0", "1" } },
	{ 2, 0x08, "SW4", { "0", "1" } },
	{ 2, 0x10, "SW5", { "0", "1" } },
	{ 2, 0x20, "SW6", { "0", "1" } },
	{ 2, 0x40, "SW7", { "0", "1" } },
	{ 2, 0x80, "SW8", { "0", "1" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x0000, &spritelayout, 0, 1 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0x94,0x00,0xd8,   /* darkpurple */
	0xd8,0x00,0x00,   /* darkred    */
	0xf8,0x64,0xd8,   /* pink       */
	0x00,0xd8,0x00,   /* darkgreen  */
	0x00,0xf8,0xd8,   /* darkcyan   */
	0xd8,0xd8,0x94,   /* darkyellow */
	0xd8,0xf8,0xd8,   /* darkwhite  */
	0xf8,0x94,0x44,   /* orange     */
	0x00,0x00,0xd8,   /* blue   */
	0xf8,0x00,0x00,   /* red    */
	0xff,0x00,0xff,   /* purple */
	0x00,0xf8,0x00,   /* green  */
	0x00,0xff,0xff,   /* cyan   */
	0xf8,0xf8,0x00,   /* yellow */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, darkpurple, darkred, pink, darkgreen, darkcyan, darkyellow,
		darkwhite, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	black, darkred,   blue,       darkyellow, yellow, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,	   green, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,	         red, green,     orange,     yellow,
	black, darkwhite, red,        pink,        green, darkcyan,  red,        darkwhite
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	eggs_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( eggs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "e14.bin", 0x3000, 0x1000, 0x996dbebf )
	ROM_LOAD( "d14.bin", 0x4000, 0x1000, 0xbbbce334 )
	ROM_LOAD( "c14.bin", 0x5000, 0x1000, 0xc89bff1d )
	ROM_LOAD( "b14.bin", 0x6000, 0x1000, 0x86e4f94e )
	ROM_LOAD( "a14.bin", 0x7000, 0x1000, 0x9beb93e9 )
	ROM_RELOAD(          0xf000, 0x1000 )	/* for reset/interrupt vectors */

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g12.bin",  0x0000, 0x1000, 0x4beb2eab )
	ROM_LOAD( "g10.bin",  0x1000, 0x1000, 0x61460352 )
	ROM_LOAD( "h12.bin",  0x2000, 0x1000, 0x9c23f42b )
	ROM_LOAD( "h10.bin",  0x3000, 0x1000, 0x77b53ac7 )
	ROM_LOAD( "j12.bin",  0x4000, 0x1000, 0x27ab70f5 )
	ROM_LOAD( "j10.bin",  0x5000, 0x1000, 0x8786e8c4 )
ROM_END



struct GameDriver eggs_driver =
{
	"Eggs",
	"eggs",
	"NICOLA SALMORIA",
	&machine_driver,

	eggs_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	0, 0
};
