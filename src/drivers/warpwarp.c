/***************************************************************************

Warp Warp memory map (preliminary)

0000-37ff ROM

memory mapped ports:

read:
5000      IN0
5040      IN1
5080      DSW1

write:
c003      interrupt_enable

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap);

static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x7FFF, MRA_RAM },
	{ 0x0000, 0x37ff, MRA_ROM },
        { 0xc000, 0xc000, input_port_0_r },
        { 0xc001, 0xc001, input_port_1_r },
        { 0xc002, 0xc002, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x7FFF, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram },
	{ 0x8400, 0x87ff, videoram_w, &videoram },
	{ 0xc003, 0xc003, interrupt_enable_w },
        { 0xc000, 0xc03f, MWA_RAM },
	{ 0x0000, 0x37ff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN,
				OSD_KEY_F1, 0, 0, OSD_KEY_3 },
		{ OSD_JOY_UP, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN,
				0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_F2, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xe9,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x0c, "LIVES", { "1", "2", "3", "5" } },
	{ 2, 0x30, "BONUS", { "10000", "15000", "20000", "NONE" } },
	{ 2, 0x40, "DIFFICULTY", { "HARD", "NORMAL" }, 1 },
	{ 2, 0x80, "GHOST NAMES", { "ALTERNATE", "NORMAL" }, 1 },
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
	{ 1, 0x0000, &charlayout,   0, 2 },
	{ 1, 0x0000, &spritelayout, 0, 2 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, white
};

static unsigned char colortable[] =
{
	black, white,
        black, white,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
  	32*8, 32*8, { 6*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
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
	ROM_LOAD( "warp_2r.bin", 0x0000, 0x1000)         /* Code */
	ROM_LOAD( "warp_2m.bin", 0x1000, 0x1000)
	ROM_LOAD( "warp_1p.bin", 0x2000, 0x1000)
	ROM_LOAD( "warp_1t.bin", 0x3000, 0x0800)

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "warp_s12.bin", 0x0000, 0x0800 )
ROM_END


struct GameDriver warpwarp_driver =
{
	"warpwarp",
	&machine_driver,

	warpwarp_rom,
	0, 0,
        0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	            /* numbers */
	  0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
	  0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0, 3,
	8*13, 8*16, 2,

	0, 0
};

