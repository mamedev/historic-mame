/***************************************************************************

Centipede memory map (preliminary)

0000-0200 RAM
0400-07ff Video RAM (07c0-07ff Sprites)
2000-3fff ROM

read:
0800      DSW1
0801      DSW2
0c00      trackball x + other things
0c01      IN0
0c02      trackball y
0c03      IN1
100a      POKEY random number generator

write
1000-1008 POKEY
1800      ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int centiped_init_machine(const char *gamename);
extern int centiped_rand_r(int offset);

extern void milliped_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void milliped_pokey1_w(int offset,int data);
extern int milliped_sh_start(void);
extern void milliped_sh_stop(void);
extern void milliped_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0200, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x0c00, 0x0c00, input_port_0_r },	/* ??? */
	{ 0x0c01, 0x0c01, input_port_1_r },	/* IN0 */
	{ 0x0c02, 0x0c02, input_port_2_r },	/* ??? */
	{ 0x0c03, 0x0c03, input_port_3_r },	/* IN1 */
	{ 0x0800, 0x0800, input_port_4_r },	/* DSW1 */
	{ 0x0801, 0x0801, input_port_5_r },	/* DSW2 */
	{ 0x100a, 0x100a, centiped_rand_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0200, MWA_RAM },
	{ 0x0400, 0x07bf, videoram_w, &videoram },
	{ 0x07c0, 0x07ff, MWA_RAM, &spriteram },
	{ 0x1000, 0x1008, milliped_pokey1_w },
	{ 0x2000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* ??? */
		0x60,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_CONTROL, OSD_KEY_CONTROL, 0, 0, OSD_KEY_3, 0 },
		{ 0, 0, OSD_JOY_FIRE, OSD_JOY_FIRE, 0, 0, 0, 0 }
	},
	{	/* ??? */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT }
	},
	{	/* DSW1 */
		0x54,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 4, 0x0c, "LIVES", { "2", "3", "4", "5" } },
	{ 4, 0x30, "BONUS", { "10000", "12000", "15000", "20000" } },
	{ 4, 0x40, "DIFFICULTY", { "HARD", "EASY" }, 1 },
	{ 4, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,8,	/* 16*8 sprites */
	128,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*8 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
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
	black, darkred,   blue,       darkyellow,
	black, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,
	black, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,
	black, green,     orange,     yellow,
	black, darkwhite, red,        pink,
	black, darkcyan,  red,        darkwhite
};



const struct MachineDriver centiped_driver =
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
	input_ports,dsw,
	centiped_init_machine,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0x60,0x41,
	0x06,0x04,
	8*13,8*16,0x00,
	0,
	generic_vh_start,
	generic_vh_stop,
	milliped_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	milliped_sh_start,
	milliped_sh_stop,
	milliped_sh_update
};
