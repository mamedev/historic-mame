/***************************************************************************

Pepper II memory map (preliminary)

0000-03ff RAM
4000-43ff Video ram
6000-6fff Character generator RAM (512)
8000-ffff ROM

read:
5100      DSW (inverted)
          bit 0  coin 2 (NOT inverted) (must activate together with IN1 bit 5)
          bit 1-2  bonus
          bit 3-4  coins per play
          bit 5-6  lives
          bit 7  US/UK coins
5101      IN0 (inverted)
          bit 0  start 1
		  bit 1  start 2
		  bit 2  right
		  bit 3  left
		  bit 4  fire (Dog Button)
		  bit 5  up
		  bit 6  down
		  bit 7  coin 1 (must activate together with IN1 bit 6)
5103      IN1

      bit 2  Must be 1 to have collision enabled?
               Probably Collision between sprite and background.
               Press the key 7 to test

			bit 4  Probably Collision flag between sprites
               Press the key 8 to test

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
5101      Sprites bank
              bit 7  Bank for sprite #1  (Not sure!)
              bit 6  Bank for sprite #2  (Sure, see the bird in Mouse Trap)

5200-5203  Probably has something to do with the colors?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *venture_characterram;
extern void venture_characterram_w(int offset,int data);
extern void venture_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *venture_sprite_no;
extern unsigned char *venture_sprite_enable;


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
	{ 0x5103, 0x5103, input_port_2_r },	/* IN1 */
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram },
	{ 0x5100, 0x5100, MWA_RAM, &venture_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &venture_sprite_enable },
	{ 0x6000, 0x6fff, venture_characterram_w, &venture_characterram },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* DSW */
		0xb0,
		{ OSD_KEY_4, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_RIGHT, OSD_KEY_LEFT,
				OSD_KEY_CONTROL, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_3 },
		{ 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT,
				OSD_JOY_FIRE, OSD_JOY_UP, OSD_JOY_DOWN, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, OSD_KEY_7, 0, OSD_KEY_8, OSD_KEY_4, OSD_KEY_3, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x60, "LIVES", { "2", "3", "4", "5" } },
	{ 0, 0x06, "BONUS", { "20000", "30000", "40000", "50000" } },
	{ -1 }
};



struct GfxLayout pepper2_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	43,	/* 43 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*4,	/* 8 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xdc42, &charlayout,         0, 1 },	/* not used by the game, here only for the dip switch menu */
	{ 0, 0x6000, &pepper2_charlayout, 0, 4 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout,      10, 2 },  /* Sprites */
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
	black, yellow,
	black, blue,
	black, green,
	black, red,
	black, white,
	black, cyan,
	black, purple,

};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
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
	venture_vh_screenrefresh,

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

ROM_START( pepper2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "main_12a", 0x9000, 0x1000 )
	ROM_LOAD( "main_11a", 0xA000, 0x1000 )
	ROM_LOAD( "main_10a", 0xB000, 0x1000 )
	ROM_LOAD( "main_9a",  0xC000, 0x1000 )
	ROM_LOAD( "main_8a",  0xD000, 0x1000 )
	ROM_LOAD( "main_7a",  0xE000, 0x1000 )
	ROM_LOAD( "main_6a",  0xF000, 0x1000 )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "main_11d", 0x0000, 0x0800 )
ROM_END





struct GameDriver pepper2_driver =
{
	"Pepper II",
	"pepper2",
	"MARC LAFONTAINE",
	&machine_driver,

	pepper2_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	0,0
};
