/***************************************************************************

Space Panic memory map

0000-3FFF ROM
4000-5BFF Video RAM (Bitmap)
5C00-5FFF RAM
6000-601F Sprite Controller
          4 bytes per sprite

          byte 1 - 80 = ?
                   40 = Rotate sprite left/right
                   3F = Sprite Number (Conversion to my layout via table)

          byte 2 - X co-ordinate
          byte 3 - Y co-ordinate

          byte 4 - 08 = Switch sprite bank
                   07 = Colour

6800      IN1 - See input port setup for mappings
6802	  DSW
6803      IN0

7800      80 = Flash Screen?

I/O ports:
read:

write:

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void panic_videoram_w(int offset,int data);
extern int panic_interrupt(void);

extern unsigned char *panic_videoram;

extern void panic_vh_screenrefresh(struct osd_bitmap *bitmap);

static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x5FFF, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
    { 0x6803, 0x6803, input_port_0_r },
    { 0x6800, 0x6800, input_port_1_r },
    { 0x6802, 0x6802, input_port_2_r }, /* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x5C00, 0x5FFF, MWA_RAM },
	{ 0x4000, 0x5BFF, panic_videoram_w, &panic_videoram },
	{ 0x6000, 0x601F, MWA_RAM, &spriteram },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0xFF,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, OSD_KEY_3, OSD_KEY_4 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0xFF,
		{ OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP, 0, 0, OSD_KEY_ALT },
		{ OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP, 0, 0, OSD_JOY_FIRE2 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x20, "LIVES", { "3", "4" } },
	{ 2, 0x10, "BONUS", { "3000", "5000" } },
	{ 2, 0x08, "CABINET", { "UPRIGHT", "COCKTAIL" } },
	{ 2, 0x07, "LEFT SLOT", { "1 1", "1 2", "1 3", "1 4", "1 5", "2 3", "? ?", "? ?" } },
	{ 2, 0xC0, "RIGHT SLOT", { "2 1", "1 1", "1 2", "1 3" } },
	{ -1 }
};



/* Space Panic doesn't have character mapped */
/* graphics, this definition is here only    */
/* only for the dip switch menu              */

static struct GfxLayout charlayout =
{
	7,8, /* 7*8 characters */
	36,	 /* 36 characters */
	1,	 /* 1 bit per pixel */
	{ 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*7	/* every char takes 7 consecutive bytes */
};

// Main Sprites

static struct GfxLayout spritelayout0 =
{
	16,16,	/* 16*16 sprites */
	48 ,	/* 64 sprites */
	2,	    /* 2 bits per pixel */
	{ 0, 4096*8 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
  	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0, 7, 6, 5, 4, 3, 2, 1, 0 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

// Man Top

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	16 ,	/* 16 sprites */
	2,	    /* 2 bits per pixel */
	{ 0, 4096*8 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0, 7, 6, 5, 4, 3, 2, 1, 0,  },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 751, &charlayout, 0, 8 },
	{ 1, 0x0A00, &spritelayout0, 0, 8 },	// Monsters
	{ 1, 0x0200, &spritelayout0, 0, 8 },    // Monsters eating Man
	{ 1, 0x0800, &spritelayout1, 0, 8 },    // Man
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black  */
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
	black, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	black, yellow,    red,        white,        /* 1 Drop Monster */
	black, green,     purple,     white,        /* 3 Drop Monster */
	black, red,       white,      yellow,       /* Man */
	black, white,     white,      white,        /* Not Used? */
	black, blue,      green,      white,        /* 2 Drop Monster */
	black, white,     white,      white,        /* Not Used? */
	black, white,     white,      white,        /* Not Used? */
	black, white,     white,      white,        /* Not Used? */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,	/* 2 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			panic_interrupt,2
		}
	},
	60,
	0,

	/* video hardware */
//	32*8, 32*8, { 5*8, 30*8-1, 0*8, 32*8-1 },
  	32*8, 32*8, { 6*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	panic_vh_screenrefresh,

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

ROM_START( panic_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "spcpanic.1", 0x0000, 0x0800)         /* Code */
	ROM_LOAD( "spcpanic.2", 0x0800, 0x0800)
	ROM_LOAD( "spcpanic.3", 0x1000, 0x0800)
	ROM_LOAD( "spcpanic.4", 0x1800, 0x0800)
	ROM_LOAD( "spcpanic.5", 0x2000, 0x0800)
	ROM_LOAD( "spcpanic.6", 0x2800, 0x0800)
	ROM_LOAD( "spcpanic.7", 0x3000, 0x0800)
    ROM_LOAD( "spcpanic.8", 0x3800, 0x0800)         /* Dunno */

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */

	ROM_LOAD( "spcpanic.9", 0x0000, 0x0800 )
	ROM_LOAD( "spcpanic.10", 0x0800, 0x0800 )
	ROM_LOAD( "spcpanic.12", 0x1000, 0x0800 )
	ROM_LOAD( "spcpanic.11", 0x1800, 0x0800 )
ROM_END


struct GameDriver panic_driver =
{
	"panic",
	&machine_driver,

	panic_rom,
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

