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
#include "M6809/M6809.h"



void gng_bankswitch_w(int offset,int data);
int gng_bankedrom_r(int offset);
void gng_init_machine(void);

extern unsigned char *gng_bgvideoram,*gng_bgcolorram;
extern int gng_bgvideoram_size;
extern unsigned char *gng_scrollx,*gng_scrolly;
void gng_bgvideoram_w(int offset,int data);
void gng_bgcolorram_w(int offset,int data);
void gng_flipscreen_w(int offset,int data);
int gng_vh_start(void);
void gng_vh_stop(void);
void gng_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void gng_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	static int bank[] = { 0x10000, 0x12000, 0x14000, 0x16000, 0x04000, 0x18000 };
	cpu_setbank (1, &RAM[bank[data]]);
}



void gng_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_NONE;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3003, 0x3003, input_port_3_r },
	{ 0x3004, 0x3004, input_port_4_r },
	{ 0x3c00, 0x3c00, MRA_NOP },    /* watchdog? */
	{ 0x4000, 0x5fff, MRA_BANK1 },
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1dff, MWA_RAM },
	{ 0x1e00, 0x1fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x23ff, videoram_w, &videoram, &videoram_size },
	{ 0x2400, 0x27ff, colorram_w, &colorram },
	{ 0x2800, 0x2bff, gng_bgvideoram_w, &gng_bgvideoram, &gng_bgvideoram_size },
	{ 0x2c00, 0x2fff, gng_bgcolorram_w, &gng_bgcolorram },
	{ 0x3800, 0x38ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0x3900, 0x39ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0x3a00, 0x3a00, soundlatch_w },
	{ 0x3b08, 0x3b09, MWA_RAM, &gng_scrollx },
	{ 0x3b0a, 0x3b0b, MWA_RAM, &gng_scrolly },
	{ 0x3c00, 0x3c00, MWA_NOP },   /* watchdog? */
	{ 0x3d00, 0x3d00, gng_flipscreen_w },
	{ 0x3e00, 0x3e00, gng_bankswitch_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( gng_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage affects", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coin A" )
	PORT_DIPSETTING(    0x00, "Coin B" )
	PORT_DIPNAME( 0x20, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000 70000 70000" )
	PORT_DIPSETTING(    0x10, "30000 80000 80000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* identical to gng, but the "unknown" dip switch is Invulnerability */
INPUT_PORTS_START( gngjap_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage affects", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coin A" )
	PORT_DIPSETTING(    0x00, "Coin B" )
	PORT_DIPNAME( 0x20, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000 70000 70000" )
	PORT_DIPSETTING(    0x10, "30000 80000 80000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( diamond_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	 /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x04, "*2" )
	PORT_DIPSETTING(    0x08, "*3" )
	PORT_DIPSETTING(    0x0c, "*4" )
	PORT_DIPNAME( 0x30, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown DSW1 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown DSW2 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW2 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown DSW2 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW2 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x10, "*2" )
	PORT_DIPSETTING(    0x20, "*3" )
	PORT_DIPSETTING(    0x30, "*4" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown DSW2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown DSW2 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


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
	{ 1, 0x00000, &charlayout,   128, 16 },	/* use colors 128-195 */
	{ 1, 0x04000, &tilelayout,     0, 8 },	/* use colors 0-63 */
	{ 1, 0x1c000, &spritelayout,  64, 4 },	/* use colors 64-127 */
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
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
			CPU_M6809,
			1500000,			/* 1.5 Mhz ? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, 2500,	/* frames per second, vblank duration */
				/* hand tuned to get rid of sprite lag */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	gng_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	192, 192,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	gng_vh_start,
	gng_vh_stop,
	gng_vh_screenrefresh,

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

ROM_START( gng_rom )
	ROM_REGION(0x18000)	/* 64k for code * 5 pages */
	ROM_LOAD( "gg3.bin",  0x8000, 0x8000, 0x019eaa7c )
	ROM_LOAD( "gg4.bin",  0x4000, 0x4000, 0xf74cb35c )	/* 4000-5fff is page 0 */
	ROM_LOAD( "gg5.bin", 0x10000, 0x8000, 0xd39c9516 )	/* page 1, 2, 3 and 4 */

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
	ROM_REGION(0x18000)	/* 64k for code * 5 pages */
	ROM_LOAD( "gg3.bin",  0x8000, 0x8000, 0x019eaa7c )
	ROM_LOAD( "gg4.bin",  0x4000, 0x4000, 0xf74cb35c )	/* 4000-5fff is page 0 */
	ROM_LOAD( "gg5.bin", 0x10000, 0x8000, 0xd39c9516 )	/* page 1, 2, 3 and 4 */

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gg1.bin",     0x00000, 0x4000, 0x0d95960b )	/* characters */
	ROM_LOAD( "gg11.bin",    0x04000, 0x4000, 0x8425662b )	/* tiles 0-1 Plane 1*/
	ROM_LOAD( "gg10.bin",    0x08000, 0x4000, 0x332b9d85 )	/* tiles 2-3 Plane 1*/
	ROM_LOAD( "gg9.bin",     0x0c000, 0x4000, 0xf6433b0f )	/* tiles 0-1 Plane 2*/
	ROM_LOAD( "gg8.bin",     0x10000, 0x4000, 0x24a66776 )	/* tiles 2-3 Plane 2*/
	ROM_LOAD( "gg7.bin",     0x14000, 0x4000, 0x6e6e3ec0 )	/* tiles 0-1 Plane 3*/
	ROM_LOAD( "gg6.bin",     0x18000, 0x4000, 0x248ddf8b )	/* tiles 2-3 Plane 3*/
	ROM_LOAD( "cross17.bin", 0x1c000, 0x4000, 0xb59f663d )	/* sprites 0 Plane 1-2 */
	ROM_LOAD( "gg16.bin",    0x20000, 0x4000, 0xc29702c5 )	/* sprites 1 Plane 1-2 */
	ROM_LOAD( "gg15.bin",    0x24000, 0x4000, 0x0309f6a7 )	/* sprites 2 Plane 1-2 */
	ROM_LOAD( "cross14.bin", 0x28000, 0x4000, 0xce3dc76f )	/* sprites 0 Plane 3-4 */
	ROM_LOAD( "gg13.bin",    0x2c000, 0x4000, 0x3e603938 )	/* sprites 1 Plane 3-4 */
	ROM_LOAD( "gg12.bin",    0x30000, 0x4000, 0x8ec77b4f )	/* sprites 2 Plane 3-4 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gg2.bin", 0x0000, 0x8000, 0x0b7edfd2 )   /* Audio CPU is a Z80 */
ROM_END

ROM_START( gngjap_rom )
	ROM_REGION(0x18000)	/* 64k for code * 5 pages */
	ROM_LOAD( "8n.rom",   0x8000, 0x8000, 0xe787d6cf )
	ROM_LOAD( "10n.rom",  0x4000, 0x4000, 0x6ae8f4b8 )	/* 4000-5fff is page 0 */
	ROM_LOAD( "12n.rom", 0x10000, 0x8000, 0x0d9a17cc )	/* page 1, 2, 3 and 4 */

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

ROM_START( diamond_rom )
	ROM_REGION(0x1a000)	/* 64k for code * 6 pages (is it really 6?) */
	ROM_LOAD( "d5",       0x0000, 0x8000, 0x89e5a985 )
	ROM_LOAD( "d3",       0x8000, 0x8000, 0x38d5bcc9 )
	ROM_LOAD( "d3o",      0x4000, 0x2000, 0x76c09ea4 )	/* 4000-5fff is page 0 */
	ROM_CONTINUE(        0x18000, 0x2000 )
	ROM_LOAD( "d5o",     0x10000, 0x8000, 0x06f68aa8 )	/* page 1, 2, 3 and 4 */

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



static int gng_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x152c],"\x00\x01\x00\x00",4) == 0 &&
			memcmp(&RAM[0x156b],"\x00\x01\x00\x00",4) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int offs;


			osd_fread(f,&RAM[0x1518],9*10);
			offs = RAM[0x1518] * 256 + RAM[0x1519];
			RAM[0x00d0] = RAM[offs];
			RAM[0x00d1] = RAM[offs+1];
			RAM[0x00d2] = RAM[offs+2];
			RAM[0x00d3] = RAM[offs+3];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void gng_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x1518],9*10);
		osd_fclose(f);
	}
}



static int diamond_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* We're just going to blast the hi score table into ROM and be done with it */
        if (memcmp(&RAM[0xC10E],"KLE",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC10E],0x80);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void diamond_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* The RAM location of the hi score table */
		osd_fwrite(f,&RAM[0x1200],0x80);
		osd_fclose(f);
	}

}



struct GameDriver gng_driver =
{
	__FILE__,
	0,
	"gng",
	"Ghosts'n Goblins (US)",
	"1985",
	"Capcom",
	"Roberto Ventura\nMirko Buffoni\nNicola Salmoria\nGabrio Secco\nMarco Cassili",
	0,
	&machine_driver,

	gng_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gng_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gng_hiload, gng_hisave
};

struct GameDriver gngcross_driver =
{
	__FILE__,
	&gng_driver,
	"gngcross",
	"Ghosts'n Goblins (Cross)",
	"1985",
	"Capcom",
	"Roberto Ventura\nMirko Buffoni\nNicola Salmoria\nGabrio Secco\nMarco Cassili",
	0,
	&machine_driver,

	gngcross_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gng_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gng_hiload, gng_hisave
};

struct GameDriver gngjap_driver =
{
	__FILE__,
	&gng_driver,
	"gngjap",
	"Makai-mura (Japan)",
	"1985",
	"Capcom",
	"Roberto Ventura\nMirko Buffoni\nNicola Salmoria\nGabrio Secco\nMarco Cassili",
	0,
	&machine_driver,

	gngjap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gngjap_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gng_hiload, gng_hisave
};

struct GameDriver diamond_driver =
{
	__FILE__,
	0,
	"diamond",
	"Diamond Run",
	"1989",
	"KH Video",
	"Roberto Ventura\nMirko Buffoni\nNicola Salmoria\nMike Balfour",
	0,
	&machine_driver,

	diamond_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	diamond_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	diamond_hiload, diamond_hisave
};
