/***************************************************************************

Jump Bug memory map (preliminary)

0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
5000-50ff Object RAM
  5000-503f  screen attributes
  5040-505f  sprites
  5060-507f  bullets?
  5080-50ff  unused?
8000-afff ROM

read:
6000      IN0
6800      IN1
7000      IN2 ?
*
 * IN0 (all bits are inverted)
 * bit 7 : DOWN player 1
 * bit 6 : UP player 1
 * bit 5 : ?
 * bit 4 : SHOOT player 1
 * bit 3 : LEFT player 1
 * bit 2 : RIGHT player 1
 * bit 1 : DOWN player 2
 * bit 0 : COIN A
*
 * IN1 (bits 2-7 are inverted)
 * bit 7 : UP player 2
 * bit 6 : Difficulty: 1 Easy, 0 Hard (according to JBE v0.5)
 * bit 5 : COIN B
 * bit 4 : SHOOT player 2
 * bit 3 : LEFT player 2
 * bit 2 : RIGHT player 2
 * bit 1 : START 2 player
 * bit 0 : START 1 player
*
 * DSW (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : ?
 * bit 3 : \ coins: 11; coin a 1 credit, coin b 6 credits
 * bit 2 : /        10; coin a 1/2 credits, coin b 3 credits credits
                    01; coin a 1/2 credits, coin b 1/2
                    00; coin a 1 credit, coin b 1 credit
 * bit 1 : \ lives: 11; 5 cars, 10; 4 cars,
 * bit 0 : /        01; 3 cars, 00; Free Play


write:
5800      8910 write port
5900      8910 control port
7001      interrupt enable
7002      coin counter ????
7003      ?
7004      stars on (?)
7005      ?
7006      screen vertical flip ????
7007      screen horizontal flip ????

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int cclimber_sh_start(void);



extern unsigned char *scramble_attributesram;
extern unsigned char *scramble_bulletsram;
extern void scramble_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void scramble_attributes_w(int offset,int data);
extern void scramble_stars_w(int offset,int data);
extern int scramble_vh_start(void);
extern void scramble_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xafff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* IN2 */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram },
	{ 0x5000, 0x503f, scramble_attributes_w, &scramble_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7004, 0x7004, scramble_stars_w },
	{ 0x5900, 0x5900, AY8910_control_port_0_w },
	{ 0x5800, 0x5800, AY8910_write_port_0_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0xafff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_DOWN, OSD_KEY_UP },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_DOWN, OSD_JOY_UP }
	},
	{	/* IN1 */
		0x40,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, OSD_KEY_3, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x01,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "UNLIMITED", "3", "4", "5" } },
	{ 1, 0x40, "DIFFICULTY", { "HARD", "EASY" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	2,	/* 2 bits per pixel */
	{ 0, 768*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	192,	/* 192 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 192*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the color table */
static struct GfxLayout starslayout =
{
	1,1,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 0, 0,      &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	scramble_vh_convert_color_prom,

	0,
	scramble_vh_start,
	generic_vh_stop,
	scramble_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	cclimber_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jumpbug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1", 0x0000, 0x1000 )
	ROM_LOAD( "jb2", 0x1000, 0x1000 )
	ROM_LOAD( "jb3", 0x2000, 0x1000 )
	ROM_LOAD( "jb4", 0x3000, 0x1000 )
	ROM_LOAD( "jb5", 0x8000, 0x1000 )
	ROM_LOAD( "jb6", 0x9000, 0x1000 )
	ROM_LOAD( "jb7", 0xa000, 0x1000 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jbi", 0x0000, 0x0800 )
	ROM_LOAD( "jbj", 0x0800, 0x0800 )
	ROM_LOAD( "jbk", 0x1000, 0x0800 )
	ROM_LOAD( "jbl", 0x1800, 0x0800 )
	ROM_LOAD( "jbm", 0x2000, 0x0800 )
	ROM_LOAD( "jbn", 0x2800, 0x0800 )
ROM_END

ROM_START( jbugsega_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1.prg", 0x0000, 0x4000 )
	ROM_LOAD( "jb2.prg", 0x8000, 0x2800 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jb3.gfx", 0x0000, 0x3000 )
ROM_END



struct GameDriver jumpbug_driver =
{
	"jumpbug",
	&machine_driver,

	jumpbug_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,0x109,0x10a,0x10b,0x10c,	/* letters */
		0x10d,0x10e,0x10f,0x110,0x111,0x112,0x113,0x114,0x115,0x116,0x117,0x118,0x119 },
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver jbugsega_driver =
{
	"jbugsega",
	&machine_driver,

	jbugsega_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,0x109,0x10a,0x10b,0x10c,	/* letters */
		0x10d,0x10e,0x10f,0x110,0x111,0x112,0x113,0x114,0x115,0x116,0x117,0x118,0x119 },
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};
