/***************************************************************************

Targ memory map

0000-07ff RAM
0800-3fff ROM (Targ: 1800-3fff)
4000-43ff Video ram
4800-4fff Character generator RAM
f800-ffff ROM

read:
5100      DSW (inverted)

5101      IN0 (inverted)
		  bit 7  coin 1 (must activate together with IN1 bit 6)

5105      IN0 (inverted)
          bit 0  start 1
		  bit 1  start 2
		  bit 2  right
		  bit 3  left
		  bit 4  fire
		  bit 5  up
		  bit 6  down
          bit 7  unused

5103      IN1
          bit 5  coin 2 (must activate together with DSW bit 0)
          bit 6  coin 1 (must activate together with IN0 bit 7)
          other bits ?
5200      ?
5201      ?
5202      ?
5203      ?

write:
5000      Sprite #1 X position
5040      Sprite #1 Y position
5080      Sprite #2 X position
50c0      Sprite #2 Y position

5100      Sprite number  bits 0-3 Sprite #1  4-7 Sprite #2

5200-5203  Probably has something to do with the colors?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* These are defined in vidhrdw/exidy.c */
extern unsigned char *exidy_characterram;
extern unsigned char *exidy_color_lookup;
void exidy_characterram_w(int offset,int data);
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *exidy_sprite_no;
extern unsigned char *exidy_sprite_enable;
extern unsigned char *exidy_sprite1_xpos;
extern unsigned char *exidy_sprite1_ypos;
extern unsigned char *exidy_sprite2_xpos;
extern unsigned char *exidy_sprite2_ypos;

unsigned char targ_sprite_enable;

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x07ff, MRA_RAM },
    { 0x0800, 0x3fff, MRA_ROM },
    { 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
    { 0x5103, 0x5103, input_port_2_r }, /* IN1 */
    { 0x5105, 0x5105, input_port_3_r }, /* IN2 */
    { 0xF800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x07ff, MWA_RAM },
    { 0x0800, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
//    { 0x5000, 0x5000, targ_x_pos_change, &exidy_sprite1_xpos },
//    { 0x5040, 0x5040, targ_y_pos_change, &exidy_sprite1_ypos },
    { 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
    { 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },

	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( targ_input_ports )
    PORT_START      /* DSW0 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )	/* upright/cocktail switch? */
    PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "10P/1 C 50P Coin/6 Cs" )
    PORT_DIPSETTING(    0x02, "2x10P/1 C 50P Coin/3 Cs" )
    PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Credit" )
    PORT_DIPSETTING(    0x04, "Extended Play" )
    PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x00, "1C/1C (no display)" )
    PORT_DIPSETTING(    0x18, "1 Coin/2 Credits" )
    PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x60, "2" )
    PORT_DIPSETTING(    0x40, "3" )
    PORT_DIPSETTING(    0x20, "4" )
    PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "Quarters" )
    PORT_DIPSETTING(    0x00, "Pence" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START  /* IN1 */
    PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

    PORT_START  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* identical to Targ, the only difference is the additional Language dip switch */
INPUT_PORTS_START( spectar_input_ports )
    PORT_START      /* DSW0 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )	/* upright/cocktail switch? */
    PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "10P/1 C 50P Coin/6 Cs" )
    PORT_DIPSETTING(    0x02, "2x10P/1 C 50P Coin/3 Cs" )
    PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Credit" )
    PORT_DIPSETTING(    0x04, "Extended Play" )
    PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x00, "1C/1C (no display)" )
    PORT_DIPSETTING(    0x18, "1 Coin/2 Credits" )
    PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x60, "2" )
    PORT_DIPSETTING(    0x40, "3" )
    PORT_DIPSETTING(    0x20, "4" )
    PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "Quarters" )
    PORT_DIPSETTING(    0x00, "Pence" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START  /* IN1 */
    PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "English" )
    PORT_DIPSETTING(    0x01, "French" )
    PORT_DIPSETTING(    0x02, "German" )
    PORT_DIPSETTING(    0x03, "Spanish" )
    PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

    PORT_START  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	32,	/* 32 sprites (the other Exidy games have more) */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,       0, 4*2 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, 4*2*2, 2*2 },	/* Sprites */
	{ -1 } /* end of array */
};



/* Targ doesn't have a color PROM; colors are changed by the means of 8x3 */
/* dip switches on the board. Here are the colors they map to. */
static unsigned char targ_palette[] =
{
	               	/* color   use                */
	0x00,0x00,0xFF,	/* blue    background         */
	0xFF,0x00,0x00,	/* red     characters   0- 63 */
	0xFF,0xFF,0xFF,	/* white   characters  64-127 */
	0xFF,0xFF,0x00,	/* yellow  characters 128-191 */
	0x00,0xFF,0xFF,	/* cyan    characters 192-255 */
	0x00,0xFF,0xFF,	/* cyan    not used           */
	0xFF,0xFF,0xFF,	/* white   bullet sprite      */
	0x00,0xFF,0x00,	/* green   wummel sprite      */
};

/* Spectar has different colors */
static unsigned char spectar_palette[] =
{
	               	/* color   use                */
	0x00,0x00,0xFF,	/* blue    background         */
	0xFF,0x00,0x00,	/* red     characters   0- 63 */
	0xFF,0xFF,0xFF,	/* white   characters  64-127 */
	0x00,0xFF,0x00,	/* green   characters 128-191 */
	0x00,0xFF,0x00,	/* green   characters 192-255 */
	0x00,0xFF,0x00,	/* green   not used           */
	0xFF,0xFF,0x00,	/* yellow  bullet sprite      */
	0x00,0xFF,0x00,	/* green   wummel sprite      */
};


static unsigned char colortable[] =
{
	/* all entries are duplicated to fit in the existing Exidy video driver */

	/* characters */
	0,1,0,1,
	0,2,0,2,
	0,3,0,3,
	0,4,0,4,

	/* sprites - reversed to fit in the existing Exidy driver */
	0,7,0,7,
	0,6,0,6,
};


static unsigned char targ_color_lookup[] =
{
	/* 0x00-0x3F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	/* 0x40-0x7F */
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	/* 0x80-0xBF */
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	/* 0xC0-0xFF */
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
};



void targ_init_machine(void)
{

	/* Set color lookup table to point to us */
    exidy_sprite_enable=&targ_sprite_enable;

    /* Targ does not have a sprite enable register so we have to fake it out */
    targ_sprite_enable=0x10;
    exidy_color_lookup=targ_color_lookup;
}



static struct MachineDriver targ_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            704000,    /* .7Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
	},
    60,
    1,
    targ_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
    sizeof(targ_palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( targ_rom )
	ROM_REGION(0x10000)	/* 64k for code */
    ROM_LOAD( "targ10a1", 0x1800, 0x0800, 0x2ef6836a )
    ROM_LOAD( "targ09a1", 0x2000, 0x0800, 0x98556db9 )
    ROM_LOAD( "targ08a1", 0x2800, 0x0800, 0xb8a37e39 )
    ROM_LOAD( "targ07a4", 0x3000, 0x0800, 0x9bdfac7b )
    ROM_LOAD( "targ06a3", 0x3800, 0x0800, 0x84a5a66f )
    ROM_RELOAD(           0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "targ11d1", 0x0000, 0x0400, 0x3f892727 )
ROM_END

ROM_START( spectar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
    ROM_LOAD( "spl12a1", 0x0800, 0x0800, 0xefa4d9f2 )
    ROM_LOAD( "spl11a1", 0x1000, 0x0800, 0xe39df787 )
    ROM_LOAD( "spl10a1", 0x1800, 0x0800, 0x266a1f3c )
    ROM_LOAD( "spl9a1",  0x2000, 0x0800, 0x720689a0 )
    ROM_LOAD( "spl8a1",  0x2800, 0x0800, 0x3c62f338 )
    ROM_LOAD( "spl7a1",  0x3000, 0x0800, 0x024dbb5f )
    ROM_LOAD( "spl6a1",  0x3800, 0x0800, 0x408e5a4e )
    ROM_RELOAD(          0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "splaud",  0x0000, 0x0400, 0x7d2aa084 )	/* this is actually not used (all FF) */
    ROM_CONTINUE(        0x0000, 0x0400 )	/* overwrite with the real one */
ROM_END



struct GameDriver targ_driver =
{
	"Targ",
	"targ",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	&targ_machine_driver,

	targ_rom,
	0, 0,
	0,
	0,

	0, targ_input_ports, 0 ,0, 0,

	0, targ_palette, colortable,

	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver spectar_driver =
{
	"Spectar",
	"spectar",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	&targ_machine_driver,

	spectar_rom,
	0, 0,
	0,
	0,

	0, spectar_input_ports, 0 ,0, 0,

	0, spectar_palette, colortable,

	ORIENTATION_DEFAULT,

	0,0
};
