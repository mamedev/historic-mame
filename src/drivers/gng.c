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



void gng_bankswitch_w(int offset,int data);
int gng_bankedrom_r(int offset);
int gng_catch_loop_r(int offset); /* JB 970823 */
int gng_interrupt(void);
void gng_init_machine(void);

extern unsigned char *gng_paletteram;
void gng_paletteram_w(int offset,int data);
extern unsigned char *gng_bgvideoram,*gng_bgcolorram;
extern int gng_bgvideoram_size;
extern unsigned char *gng_scrollx,*gng_scrolly;
void gng_bgvideoram_w(int offset,int data);
void gng_bgcolorram_w(int offset,int data);
int gng_vh_start(void);
void gng_vh_stop(void);
void gng_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gng_vh_screenrefresh(struct osd_bitmap *bitmap);

int capcom_sh_start(void);
int capcom_sh_interrupt(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x2fff, MRA_RAM },
	{ 0x4000, 0x5fff, gng_bankedrom_r },
	{ 0x6184, 0x6184, gng_catch_loop_r }, /* JB 970823 */
	{ 0x6000, 0xffff, MRA_ROM },
	{ 0x3c00, 0x3c00, MRA_NOP },    /* watchdog? */
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3003, 0x3003, input_port_3_r },
	{ 0x3004, 0x3004, input_port_4_r },
	{ -1 }	/* end of table */
};

/* JB 970823 - separate diamond run from gng */
static struct MemoryReadAddress diamond_readmem[] =
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
	{ 0x2000, 0x23ff, videoram_w, &videoram, &videoram_size },
	{ 0x2400, 0x27ff, colorram_w, &colorram },
	{ 0x2800, 0x2bff, gng_bgvideoram_w, &gng_bgvideoram, &gng_bgvideoram_size },
	{ 0x2c00, 0x2fff, gng_bgcolorram_w, &gng_bgcolorram },
	{ 0x3c00, 0x3c00, MWA_NOP },   /* watchdog? */
	{ 0x3800, 0x39ff, gng_paletteram_w, &gng_paletteram },
	{ 0x3e00, 0x3e00, gng_bankswitch_w },
	{ 0x1e00, 0x1fff, MWA_RAM, &spriteram, &spriteram_size },
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

static struct TrakPort trak_ports[] =
{
        { -1 }
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
	{ 4, 0x18, "BONUS", { "30000 80000", "20000 70000", "30000 80000 80000", "20000 70000 70000" }, 1 },
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
	1024,	/* 1024 tiles */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },	/* the bitplanes are separated */
        { 0, 1, 2, 3, 4, 5, 6, 7,
            16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	768,	/* 768 sprites */
	4,	/* 4 bits per pixel */
	{ 768*64*8+4, 768*64*8+0, 4, 0 },
        { 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
	    32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   8*8+4*16, 16 },
	{ 1, 0x04000, &tilelayout,          0, 8 },
	{ 1, 0x1c000, &spritelayout,      8*8, 4 },
	{ -1 } /* end of array */
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
			capcom_sh_interrupt,12
		}
	},
	60,
	gng_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 8*8+4*16+16*4,
	gng_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	gng_vh_start,
	gng_vh_stop,
	gng_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	capcom_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

/* JB 970823 - separate diamond run from gng */
static struct MachineDriver diamond_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,			/* 1 Mhz */
			0,
			diamond_readmem,writemem,0,0,
			gng_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			capcom_sh_interrupt,12
		}
	},
	60,
	gng_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 8*8+4*16+16*4,
	gng_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	gng_vh_start,
	gng_vh_stop,
	gng_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	capcom_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gng_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "gg3.bin",  0x8000, 0x8000, 0x019eaa7c )
	ROM_LOAD( "gg4.bin",  0x4000, 0x4000, 0xf74cb35c )	/* 4000-5fff is page 0 */
	ROM_RELOAD(          0x18000, 0x4000 )	/* copy for my convenience */
	ROM_LOAD( "gg5.bin", 0x10000, 0x8000, 0xd39c9516 )	/* page 1, 2, 3 e 4 */

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gg1.bin",  0x00000, 0x4000, 0x0d95960b )	/* characters */
	ROM_LOAD( "gg11.bin", 0x04000, 0x4000, 0x8425662b )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin", 0x08000, 0x4000, 0x332b9d85 )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",  0x0c000, 0x4000, 0xf6433b0f )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",  0x10000, 0x4000, 0x24a66776 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",  0x14000, 0x4000, 0x6e6e3ec0 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",  0x18000, 0x4000, 0x248ddf8b )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "gg17.bin", 0x1c000, 0x4000, 0x1b48124e )	/* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin", 0x20000, 0x4000, 0xc29702c5 )	/* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin", 0x24000, 0x4000, 0x0309f6a7 )	/* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg14.bin", 0x28000, 0x4000, 0x0553b263 )	/* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin", 0x2c000, 0x4000, 0x3e603938 )	/* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin", 0x30000, 0x4000, 0x8ec77b4f )	/* sprites 2 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gg2.bin", 0x0000, 0x8000, 0x0b7edfd2 )   /* Audio CPU is a Z80 */
ROM_END

ROM_START( gngcross_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "gg3.bin",  0x8000, 0x8000, 0x019eaa7c )
	ROM_LOAD( "gg4.bin",  0x4000, 0x4000, 0xf74cb35c )	/* 4000-5fff is page 0 */
	ROM_RELOAD(          0x18000, 0x4000 )	/* copy for my convenience */
	ROM_LOAD( "gg5.bin", 0x10000, 0x8000, 0xd39c9516 )	/* page 1, 2, 3 e 4 */

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gg1.bin",  0x00000, 0x4000, 0x0d95960b )	/* characters */
	ROM_LOAD( "gg11.bin", 0x04000, 0x4000, 0x8425662b )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin", 0x08000, 0x4000, 0x332b9d85 )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",  0x0c000, 0x4000, 0xf6433b0f )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",  0x10000, 0x4000, 0x24a66776 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",  0x14000, 0x4000, 0x6e6e3ec0 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",  0x18000, 0x4000, 0x248ddf8b )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "gg17.bin", 0x1c000, 0x4000, 0xb59f663d )	/* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin", 0x20000, 0x4000, 0xc29702c5 )	/* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin", 0x24000, 0x4000, 0x0309f6a7 )	/* sprites 2 Plane 1-2 */
	ROM_LOAD( "gg14.bin", 0x28000, 0x4000, 0xce3dc76f )	/* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin", 0x2c000, 0x4000, 0x3e603938 )	/* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin", 0x30000, 0x4000, 0x8ec77b4f )	/* sprites 2 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gg2.bin", 0x0000, 0x8000, 0x0b7edfd2 )   /* Audio CPU is a Z80 */
ROM_END

ROM_START( diamond_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "d5",  0x00000, 0x8000, 0x89e5a985 )
	ROM_LOAD( "d3",  0x08000, 0x8000, 0x38d5bcc9 )
	ROM_LOAD( "d5o", 0x10000, 0x8000, 0x06f68aa8 )
	ROM_LOAD( "d3o", 0x18000, 0x4000, 0x76c09ea4 )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d1",  0x00000, 0x4000, 0x7da60000 )	/* characters */
	ROM_LOAD( "d11", 0x04000, 0x4000, 0xc592534e )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "d10", 0x08000, 0x4000, 0x2c5520ed )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "d9",  0x0c000, 0x4000, 0x0e971c4b )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "d8",  0x10000, 0x4000, 0x1505a1c7 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "d7",  0x14000, 0x4000, 0x5cfe0000 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "d6",  0x18000, 0x4000, 0x7428a122 )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "d17", 0x1c000, 0x4000, 0x821a03ee )	/* sprites 0 Plane 1-2 */
	/* empty space for unused sprites 1 Plane 1-2 */
	/* empty space for unused sprites 2 Plane 1-2 */
	ROM_LOAD( "d14", 0x28000, 0x4000, 0x465e000e )	/* sprites 0 Plane 3-4 */
	/* empty space for unused sprites 1 Plane 3-4 */
	/* empty space for unused sprites 2 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "d2", 0x0000, 0x8000, 0x0b7edfd2 )   /* Audio CPU is a Z80 */
ROM_END



static int gng_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x152c],"\x00\x01\x00\x00",4) == 0 &&
			memcmp(&RAM[0x156b],"\x00\x01\x00\x00",4) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			int offs;


			fread(&RAM[0x1518],1,9*10,f);
			offs = RAM[0x1518] * 256 + RAM[0x1519];
			RAM[0x00d0] = RAM[offs];
			RAM[0x00d1] = RAM[offs+1];
			RAM[0x00d2] = RAM[offs+2];
			RAM[0x00d3] = RAM[offs+3];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void gng_hisave(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x1518],1,9*10,f);
		fclose(f);
	}
}



static int diamond_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* We're just going to blast the hi score table into ROM and be done with it */
        if (memcmp(&RAM[0xC10E],"KLE",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
                        fread(&RAM[0xC10E],1,0x80,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void diamond_hisave(const char *name)
{
	FILE *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		/* The RAM location of the hi score table */
                fwrite(&RAM[0x105F],1,0x80,f);
		fclose(f);
	}

}



struct GameDriver gng_driver =
{
	"Ghosts'n Goblins",
	"gng",
	"ROBERTO VENTURA\nMIRKO BUFFONI\nNICOLA SALMORIA\nGABRIO SECCO",
	&machine_driver,

	gng_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gng_hiload, gng_hisave
};

struct GameDriver gngcross_driver =
{
	"Ghosts'n Goblins (Cross)",
	"gngcross",
	"ROBERTO VENTURA\nMIRKO BUFFONI\nNICOLA SALMORIA\nGABRIO SECCO",
	&machine_driver,

	gngcross_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gng_hiload, gng_hisave
};

struct GameDriver diamond_driver =
{
	"Diamond Run",
	"diamond",
	"ROBERTO VENTURA\nMIRKO BUFFONI\nNICOLA SALMORIA\nMIKE BALFOUR",
	&diamond_machine_driver,	/* JB 970823 - separate machine driver for D.R. */

	diamond_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	diamond_hiload, diamond_hisave
};
