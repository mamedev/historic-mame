/***************************************************************************

GHOST AND GOBLINS HARDWARE.  ( Doc By Roberto Ventura )

The hardware is similar to 1942's and other Capcom games.
It seems that it's adapted by software to run on a standard (horizontal)
CRT display.

-ROM CONTENTS.

GG1.bin  = Character display. (unlike 1942 this is bit complemented)
GG2.bin  = Sound ROM.
GG3.bin  = CPU main chunk,fixed in 8000h-ffffh.
GG4.bin  = CPU paged in.Upmost 2000h fixed in 6000h-7fffh.
GG5.bin  = CPU paged in 4000h-5fffh.
GG6.bin  = background set 2-3   plane 3
GG7.bin  = background set 0-1   plane 3
GG8.bin  = background set 2-3   plane 2
GG9.bin  = background set 0-1   plane 2
GG10.bin = background set 2-3   plane 1
GG11.bin = background set 0-1   plane 1
GG12.bin = sprites set 2        planes 3-4
GG13.bin = sprites set 1        planes 3-4
GG14.bin = sprites set 0        planes 3-4
GG15.bin = sprites set 2        planes 1-2
GG16.bin = sprites set 1        planes 1-2
GG17.bin = sprites set 0        planes 1-2

Note: the plane order is important because the game's palette is
not stored in hardware,therefore there is only a way to match a bit
with the corresponding colour.

In other ROM sets I've found a different in-rom arrangement of banks.
(16k ROMs instead of 32k)

-MEMORY MAP:

0000-1DFF = Work RAM

1e00-1f7f = Sprites

2000-23ff = Characters video ram.
2400-27ff = Characters attributes.

2800-2bff = Background video ram.
2c00-2fff = Background attributes.

3000-37ff = Input ports.
3800-3fff = Output ports.

4000-5fff = Bank switched.
6000-7fff = GG4 8k upmost.  (fixed)
8000-ffff = GG3.            (fixed)

-CHARACTER TILE FORMAT.

Attribute description:

                76543210
                    ^^^^ Palette selector.
                  ^ Page selector.
                ^^ ^ Unknown/unused.

Two 256 tiles pages are available for characters.

Sixteen 4 colours palettes are available for characters.


-BACKGROUND TILE FORMAT:

Both scroll and attributes map consist of 4 pages which wrap up/down
and left/right.

Attribute description:

                76543210
                     ^^^ Palette selector
                    ^ Tile priority
                   ^ Flip X
                  ^ Flip Y? (should be present.)
                ^^ Tile page selector.

When the priority bit is set the tile can overlap sprites.

Eight 8 colours palettes are available for background.

Four 256 tiles pages are available for background.

-SPRITE FORMAT:

There hardware is capable to display 96 sprites,they use 4 bytes each,in order:
1) Sprite number.
2) Sprite attribute.
3) Y pos.
4) X pos.

Sprite attribute description:

                76543210
                       ^ X "Sprite clipping"
                      ^ Unknown/Unused
                     ^ Flip X
                    ^Flip Y
                  ^^ Palette selector
                ^^ Sprite page selector

I've called bit 1 "Sprite clipping" bit because this does not act as a
MSB,it's set when the sprite reaches either left or right border,
according to the MSB bit of the X position (rough scroll) selects two
border display columns,where sprites horizontal movement is limited
by a 7 bit offset (rest of X position).

Four 16 colours palettes are available for sprites.

Three 256 sprites pages are available.

The sprite priority is inversed: the last sprite,increasing memory,
has the higher priority.

INPUT:

3000 = Coin and start buttons
3001 = Controls and joystick 1.
3002 = Controls and joystick 2.
3003 = DIP 0
3004 = DIP 1

OUTPUT:

3800-38ff = Palette HI: 2 nibbles:red and green.
3900-39ff = Palette LO: 1 nibble:blue (low nibble set to zero)

        The palette depth is 12 bits (4096 colours).
        Each object (scroll,characters,sprites) has it's own palette space.

        00-3f:  background palettes. (8x8 colours)
        40-7f:  sprites palettes. (4*16 colours)
        80-bf:  characters palettes (16*4 colours)
        c0-ff:  duplicate of character palettes ?? (16*4 colours)

        note:   character and sprite palettes are in reversed order,the
                transparent colour is the last colour,not colour zero.

3a00 = sound output?

3b08 = scroll X
3b09 = scroll X MSB
3b0a = scroll Y
3b0b = scroll Y MSB

3c00 = watchdog???

3d00 = ???
3d01 = ???
3d02 = ???
3d03 = ???

3e00 = page selector (valid pages are 0,1,2,3 and 4)




***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern void gng_bankswitch_w(int offset,int data);
extern int gng_bankedrom_r(int offset);
extern int gng_interrupt(void);

extern unsigned char *gng_paletteram;
extern void gng_paletteram_w(int offset,int data);
extern unsigned char *gng_bgvideoram,*gng_bgcolorram;
extern unsigned char *gng_scrollx,*gng_scrolly;
extern void gng_bgvideoram_w(int offset,int data);
extern void gng_bgcolorram_w(int offset,int data);
int gng_vh_start(void);
void gng_vh_stop(void);
extern void gng_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void gng_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int gng_init_machine(const char *gamename);
extern int c1942_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x2fff, MRA_RAM },
	{ 0x4000, 0x5fff, gng_bankedrom_r },
	{ 0x6000, 0xffff, MRA_ROM },
	{ 0x3c00, 0x3c00, MRA_NOP },    /* watchdog? */
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3003, 0x3003, input_port_3_r },
	{ 0x3004, 0x3004, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1dff, MWA_RAM },
	{ 0x2000, 0x23ff, videoram_w, &videoram },
	{ 0x2400, 0x27ff, colorram_w, &colorram },
	{ 0x2800, 0x2bff, gng_bgvideoram_w, &gng_bgvideoram },
	{ 0x2c00, 0x2fff, gng_bgcolorram_w, &gng_bgcolorram },
	{ 0x3c00, 0x3c00, MWA_NOP },   /* watchdog? */
	{ 0x3800, 0x39ff, gng_paletteram_w, &gng_paletteram },
	{ 0x3e00, 0x3e00, gng_bankswitch_w },
	{ 0x1e00, 0x1fff, MWA_RAM, &spriteram },
	{ 0x3b08, 0x3b09, MWA_RAM, &gng_scrollx },
	{ 0x3b0a, 0x3b0b, MWA_RAM, &gng_scrolly },
	{ 0x3a00, 0x3a00, sound_command_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, sound_command_latch_r },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, AY8910_control_port_0_w },
	{ 0xe001, 0xe001, AY8910_write_port_0_w },
	{ 0xe002, 0xe002, AY8910_control_port_1_w },
	{ 0xe003, 0xe003, AY8910_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xdf,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_F2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
	{ 1, 3, "MOVE UP" },
	{ 1, 1, "MOVE LEFT"  },
	{ 1, 0, "MOVE RIGHT" },
	{ 1, 2, "MOVE DOWN" },
	{ 1, 4, "FIRE" },
	{ 1, 5, "JUMP" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "7", "5", "4", "3" }, 1 },
	{ 4, 0x18, "BONUS", { "30000 800000", "20000 800000", "30000 80000 80000", "20000 70000 70000" }, 1 },
	{ 4, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "EASY", "NORMAL" }, 1 },
	{ 3, 0x20, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	256,	/* 256 tiles */
	3,	/* 3 bits per pixel */
	{ 0x8000*8, 0x4000*8, 0 },	/* the bitplanes are separated */
        { 0, 1, 2, 3, 4, 5, 6, 7,
            16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0x4000*8+4, 0x4000*8+0, 4, 0 },
        { 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
	    32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the remapped color table and dynamically build the real one. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};


#define TOTAL_COLORS 186

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout, 8*8+4*16+16*4, 3 },	/* not used by the game, here only for the dip switch menu */
	{ 1, 0x00000, &charlayout,      8*8+4*16, 16 },
	{ 1, 0x04000, &tilelayout,             0, 8 },
	{ 1, 0x06000, &tilelayout,             0, 8 },
	{ 1, 0x10000, &tilelayout,             0, 8 },
	{ 1, 0x12000, &tilelayout,             0, 8 },
	{ 1, 0x1c000, &spritelayout,         8*8, 4 },
	{ 1, 0x24000, &spritelayout,         8*8, 4 },
	{ 1, 0x2c000, &spritelayout,         8*8, 4 },
	{ 0, 0,       &fakelayout, 8*8+4*16+16*4+3*4, TOTAL_COLORS },
	{ -1 } /* end of array */
};



/* Ghosts 'n Goblins doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. Here is the list of all the colors is uses. */
static unsigned char gng_color_prom[2*TOTAL_COLORS] =
{
	/* total: 186 colors (2 bytes per color) */
	0x00,0x00,
	0xff,0xf0,0x88,0x80,0xff,0x00,0x88,0x00,0xf0,0x00,0x80,0x00,	/* for the dipswitch menu */
	          0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x08,0x00,0x09,0x00,
	0x11,0x00,0x20,0x00,0x21,0x00,0x31,0x00,0x32,0x00,0x35,0x00,0x36,0x00,0x42,0x00,
	0x43,0x00,0x46,0x00,0x48,0x00,0x50,0x00,0x52,0x00,0x53,0x00,0x54,0x00,0x57,0x00,
	0x60,0x00,0x63,0x00,0x64,0x00,0x65,0x00,0x68,0x00,0x74,0x00,0x75,0x00,0x79,0x00,
	0x80,0x00,0x84,0x00,0x85,0x00,0x8a,0x00,0x90,0x00,0x94,0x00,0x95,0x00,0x99,0x00,
	0xa0,0x00,0xa5,0x00,0xb0,0x00,0xb3,0x00,0xb5,0x00,0xc0,0x00,0xc5,0x00,0xc7,0x00,
	0xe0,0x00,0xec,0x00,0xf5,0x00,0xf7,0x00,0x11,0x10,0x12,0x10,0x24,0x10,0x42,0x10,
	0x03,0x20,0x22,0x20,0x33,0x20,0x43,0x20,0x64,0x20,0x75,0x20,0x01,0x30,0x04,0x30,
	0x33,0x30,0x42,0x30,0x43,0x30,0x44,0x30,0x46,0x30,0x54,0x30,0x64,0x30,0x75,0x30,
	0x86,0x30,0xa0,0x30,0xe5,0x30,0x02,0x40,0x04,0x40,0x05,0x40,0x33,0x40,0x42,0x40,
	0x44,0x40,0x53,0x40,0x55,0x40,0x58,0x40,0x64,0x40,0x65,0x40,0x75,0x40,0x76,0x40,
	0x7b,0x40,0x86,0x40,0x97,0x40,0xa6,0x40,0xa7,0x40,0xa8,0x40,0xb7,0x40,0xc0,0x40,
	0xf4,0x40,0x03,0x50,0x05,0x50,0x44,0x50,0x55,0x50,0x64,0x50,0x65,0x50,0x66,0x50,
	0x68,0x50,0x6a,0x50,0x76,0x50,0x87,0x50,0x8a,0x50,0x97,0x50,0xa8,0x50,0x00,0x60,
	0x07,0x60,0x44,0x60,0x55,0x60,0x66,0x60,0x6b,0x60,0x75,0x60,0x76,0x60,0x77,0x60,
	0x87,0x60,0x9b,0x60,0xa8,0x60,0xa9,0x60,0xb9,0x60,0xf0,0x60,0x00,0x70,0x66,0x70,
	0x77,0x70,0x86,0x70,0x87,0x70,0x88,0x70,0x98,0x70,0xa9,0x70,0xac,0x70,0x00,0x80,
	0x04,0x80,0x33,0x80,0x50,0x80,0x66,0x80,0x77,0x80,0x7f,0x80,0x88,0x80,0xa9,0x80,
	0xca,0x80,0xf8,0x80,0x05,0x90,0x0a,0x90,0x63,0x90,0x88,0x90,0x99,0x90,0xa8,0x90,
	0xaa,0x90,0x05,0xa0,0x08,0xa0,0x45,0xa0,0x88,0xa0,0x99,0xa0,0xb9,0xa0,0xcc,0xa0,
	0xd8,0xa0,0x06,0xb0,0x57,0xb0,0x75,0xb0,0xbb,0xb0,0x08,0xc0,0x09,0xc0,0x69,0xc0,
	0x6a,0xc0,0x85,0xc0,0x9f,0xc0,0xaa,0xc0,0xcc,0xc0,0xee,0xd0,0xa7,0xe0,0x5f,0xf0,
	0x66,0xf0,0x8f,0xf0,0x99,0xf0,0xda,0xf0
};

static unsigned char diamond_color_prom[2*TOTAL_COLORS] =
{
	/* total: 85 colors (2 bytes per color) */
	0x00,0x00,
	0x88,0x80,0xff,0xf0,0x88,0x00,0xff,0x00,0x80,0x00,0xf0,0x00,	/* for the dipswitch menu */
	          0x01,0x00,0x05,0x00,0x07,0x00,0x09,0x00,0x0a,0x00,0x0b,0x00,0x0c,0x00,
	0x0d,0x00,0x0f,0x00,0x17,0x00,0x19,0x00,0x20,0x00,0x60,0x00,0x77,0x00,0x80,0x00,
	0x8f,0x00,0x99,0x00,0xa0,0x00,0xa9,0x00,0xaa,0x00,0xb0,0x00,0xb7,0x00,0xbb,0x00,
	0xc0,0x00,0xca,0x00,0xcc,0x00,0xd0,0x00,0xdd,0x00,0xea,0x00,0xec,0x00,0xee,0x00,
	0xf0,0x00,0xf8,0x00,0xfc,0x00,0xff,0x00,0x00,0x10,0x01,0x10,0x20,0x10,0xca,0x10,
	0xcc,0x10,0x00,0x20,0x01,0x20,0x54,0x40,0xa3,0x40,0x66,0x60,0x87,0x60,0xb4,0x60,
	0xc6,0x60,0x77,0x70,0x00,0x80,0x88,0x80,0x98,0x80,0xc5,0x80,0x99,0x90,0x00,0xa0,
	0xba,0xa0,0xd6,0xa0,0x00,0xb0,0x20,0xb0,0xbb,0xb0,0x00,0xc0,0x04,0xc0,0x08,0xc0,
	0xcc,0xc0,0xdc,0xc0,0xe7,0xc0,0x0a,0xd0,0xdd,0xd0,0x0c,0xe0,0x0e,0xe0,0xee,0xe0,
	0xfc,0xe0,0x00,0xf0,0x08,0xf0,0x0f,0xf0,0x8f,0xf0,0xe8,0xf0,0xff,0xf0
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,			/* 1 Mhz */
			0,
			readmem,writemem,0,0,
			gng_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60,
	gng_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	TOTAL_COLORS,8*8+4*16+16*4+3*4+TOTAL_COLORS,
	gng_vh_convert_color_prom,

	0,
	gng_vh_start,
	gng_vh_stop,
	gng_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	c1942_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gng_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "gg4.bin", 0x4000, 0x4000 )    /* 4000-5fff is page 0 */
	ROM_LOAD( "gg3.bin", 0x8000, 0x8000 )
        ROM_LOAD( "gg5.bin", 0x10000, 0x8000 )   /* page 1, 2, 3 e 4 */
	ROM_LOAD( "gg4.bin", 0x18000, 0x4000 )    /* 4000-5fff is page 0 */

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gg1.bin",  0x00000, 0x4000 )	/* characters */
	ROM_LOAD( "gg11.bin", 0x04000, 0x4000 )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg9.bin",  0x08000, 0x4000 )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg7.bin",  0x0c000, 0x4000 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg10.bin", 0x10000, 0x4000 )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg8.bin",  0x14000, 0x4000 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg6.bin",  0x18000, 0x4000 )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "gg17.bin", 0x1c000, 0x4000 )	/* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg14.bin", 0x20000, 0x4000 )	/* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg16.bin", 0x24000, 0x4000 )	/* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg13.bin", 0x28000, 0x4000 )	/* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg15.bin", 0x2c000, 0x4000 )	/* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg12.bin", 0x30000, 0x4000 )	/* sprites 2 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gg2.bin", 0x0000, 0x8000 )   /* Audio CPU is a Z80 */
ROM_END

ROM_START( diamond_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "d5",  0x00000, 0x8000 )
	ROM_LOAD( "d3",  0x08000, 0x8000 )
        ROM_LOAD( "d5o", 0x10000, 0x8000 )
	ROM_LOAD( "d3o", 0x18000, 0x4000 )

	ROM_REGION(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d1",  0x00000, 0x4000 )	/* characters */
	ROM_LOAD( "d11", 0x04000, 0x4000 )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "d9",  0x08000, 0x4000 )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "d7",  0x0c000, 0x4000 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "d10", 0x10000, 0x4000 )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "d8",  0x14000, 0x4000 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "d6",  0x18000, 0x4000 )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "d17", 0x1c000, 0x4000 )	/* sprites 0 Plane 1-2 */
	ROM_LOAD( "d14", 0x20000, 0x4000 )	/* sprites 0 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "d2", 0x0000, 0x8000 )   /* Audio CPU is a Z80 */
ROM_END



#if 0
static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xee00],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0xee4e],"\x00\x08\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0xee00],1,13*7,f);
			RAM[0xee97] = RAM[0xee00];
			RAM[0xee98] = RAM[0xee01];
			RAM[0xee99] = RAM[0xee02];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0xee00],1,13*7,f);
		fclose(f);
	}
}
#endif


struct GameDriver gng_driver =
{
	"Ghosts'n Goblins",
	"gng",
	"ROBERTO VENTURA\nMIRKO BUFFONI\nNICOLA SALMORIA\nGABRIO SECCO",
	&machine_driver,

	gng_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	gng_color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0, 1,
	8*13, 8*16, 2,

	0, 0
};

struct GameDriver diamond_driver =
{
	"Diamond Run",
	"diamond",
	"ROBERTO VENTURA\nMIRKO BUFFONI\nNICOLA SALMORIA",
	&machine_driver,

	diamond_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	diamond_color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0, 1,
	8*13, 8*16, 2,

	0, 0
};
