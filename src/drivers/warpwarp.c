/***************************************************************************

Warp Warp memory map (preliminary)

  Memory Map figured out by Chris Hardy (chrish@kcbbs.gen.nz)
  Initial Driver code by Mirko


0000-37FF ROM		Code
4800-4FFF ROM		Graphics rom which must be included in memory space

memory mapped ports:

read:

All Read ports only use bit 0

C000      Coin slot 2
C001      ???
C002      Start P1
C003      Start P2
C004      Fire
C005      Test Mode
C006      ???
C007      Coin Slot 2
C020-C027 Dipswitch 1->8 in bit 0

C010      Analog Joystick
			 0->62 = DOWN
			 63->110 = LEFT
			 111->166 = RIGHT
			 167->255 = UP

write:

C003			WatchDog reset (ignore)
C036			re-enable interrupts (ignore)
C010			Sound Port (??)
C020			Sound Port 2 (??)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *warpwarp_bulletsram;
void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap);

int warpwarp_input_c000_7_r(int offset);
int warpwarp_input_c020_27_r(int offset);
int warpwarp_input_controller_r(int offset);
int warpwarp_interrupt();


static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x83FF, MRA_RAM },
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x47FF, MRA_RAM },
	{ 0x4800, 0x4FFF, MRA_ROM },
        { 0xc000, 0xc007, warpwarp_input_c000_7_r },
        { 0xc010, 0xc010, warpwarp_input_controller_r },
        { 0xc020, 0xc027, warpwarp_input_c020_27_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83FF, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0xC000, 0xC001, MWA_RAM, &warpwarp_bulletsram },

	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4800, 0x4FFF, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct InputPort input_ports[] =
{
	{	/* DSW1 */
		0x0D,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ /* Controls get mapped to correct read location in Machine.c code */
		0xFF,
		{ OSD_KEY_3, 0, OSD_KEY_1, OSD_KEY_2, OSD_KEY_CONTROL },
		{ 0        , 0, 0        , 0        , OSD_JOY_FIRE2}
	},
	{
		0x00,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN }
	},
	{	/* Test setting */
		0x1,
		{ 0},
		{ 0}
	},
	{ -1 }	/* end of table */
};


static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { -1 }
};



static struct DSW dsw[] =
{
	{ 0, 0x03, "CREDITS", { "FREEPLAY", "1 COIN 1 CREDIT","1 COIN 2 CREDITS","2 COINS 1 CREDIT" } },
	{ 0, 0x0C, "NO FIGHTERS", { "2", "3", "4", "5" } },
/* The bonus setting changes depending on the no of lives */
	{ 0, 0x30, "BONUS", {	"SETTING 1","SETTING 2","SETTING 3","SETTING 4", } },
	{ 0, 0x40, "DEMO SOUND", { "ON", "OFF" } },
	{ 1, 1<<5, "TEST MODE", { "ON", "OFF" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 2 bits per pixel */
	{ 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* bits are packed in groups of four */
	8*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	1,	/* 1 bits per pixel */
	{ 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 23 * 8, 22 * 8, 21 * 8, 20 * 8, 19 * 8, 18 * 8, 17 * 8, 16 * 8 ,
           7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{  0, 1, 2, 3, 4, 5, 6, 7 ,
         8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 16 },
	{ 1, 0x0000, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0, 0, 0,
	0, 165, 255,
	231, 231, 0,
	165, 0, 0,
	181, 181, 181,
	239, 0, 239,
	247, 0, 156,
	255, 0, 0,
	255, 132, 0,
	255, 181, 115,
	255, 255, 255,
	255, 0, 255,
	0, 255, 255,
	0, 0, 255,
	255, 0, 0
};

enum
{
	black, white, yellow
};

static unsigned char colortable[] =
{
	0,0,
	0,1,
	0,2,
	0,3,
	0,4,
	0,5,
	0,6,
	0,7,
	0,8,
	0,9,
	0,9,
	0,10,
	0,11,
	0,12,
	0,13,
	0,14,
	0,15
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2048000,	/* 3 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
  	31*8, 34*8, { 0*8, 31*8-1, 0*8, 34*8-1 },
	gfxdecodeinfo,

	sizeof(palette)/3 ,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	warpwarp_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



ROM_START( warpwarp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "warp_2r.bin",  0x0000, 0x1000, 0x9a51d92b )
	ROM_LOAD( "warp_2m.bin",  0x1000, 0x1000, 0xdeb96ecf )
	ROM_LOAD( "warp_1p.bin",  0x2000, 0x1000, 0x30ad0f77 )
	ROM_LOAD( "warp_1t.bin",  0x3000, 0x0800, 0x3426c0b0 )
	ROM_LOAD( "warp_s12.bin", 0x4800, 0x0800, 0xb0468bf8 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "warp_s12.bin", 0x0000, 0x0800, 0xb0468bf8 )
ROM_END



struct GameDriver warpwarp_driver =
{
	"Warp Warp",
	"warpwarp",
	"CHRIS HARDY",
	&machine_driver,

	warpwarp_rom,
	0, 0,
        0,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	0, 0
};

