/***************************************************************************

Vanguard memory map (preliminary)

Note that this memory map is very similar to Nibbler

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM
1000-1fff Character mapped RAM
4000-bfff ROM

read:
3104      IN0
3105      IN1
3106      DSW ??
3107      IN2

write
3200      y scroll register
3300      x scroll register

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *vanguard_videoram2;
extern unsigned char *vanguard_characterram;

extern int vanguard_interrupt(void);
void vanguard_videoram2_w(int offset,int data);
void vanguard_scrollx_w (int offset,int data);
void vanguard_scrolly_w (int offset,int data);
int vanguard_vh_start(void);
void vanguard_vh_stop(void);
extern void vanguard_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void vanguard_characterram_w(int offset,int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x3104, 0x3104, input_port_0_r },	/* IN0 */
	{ 0x3105, 0x3105, input_port_1_r },	/* IN1 */
	{ 0x3106, 0x3106, input_port_2_r },	/* DSW ?? */
	{ 0x3107, 0x3107, input_port_3_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, videoram_w, &videoram },
	{ 0x0800, 0x0bff, vanguard_videoram2_w, &vanguard_videoram2 },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, vanguard_characterram_w, &vanguard_characterram },
	{ 0x3300, 0x3300, vanguard_scrollx_w },
	{ 0x3200, 0x3200, vanguard_scrolly_w },
	{ 0x4000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_D, OSD_KEY_E, OSD_KEY_F, OSD_KEY_S,
			OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_S, OSD_KEY_W, OSD_KEY_D, OSD_KEY_A,
			OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW ?? */
		0x00,
		{ 0, 0, 0, 0, OSD_KEY_F1, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x04, "DIFFICULTY", { "EASY", "HARD" } },
	{ -1 }
};



struct GfxLayout vanguard_charlayout =
{
        8,8,    /* 8*8 characters */
        256,    /* 256 characters */
        2,      /* 2 bits per pixel */
        { 0, 256*8*8 }, /* the two bitplanes are separated */
        { 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xf000, &vanguard_charlayout,  0, 16 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &charlayout2,        	0, 16 },
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
	black, darkcyan,  red,        darkwhite,
	black, darkred,   blue,       darkyellow,
	black, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,
	black, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,
	black, green,     orange,     yellow,
	black, darkwhite, red,        pink,
	black, darkcyan,  red,        darkwhite
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
			vanguard_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	vanguard_vh_screenrefresh,

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

ROM_START( vanguard_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000 )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000 )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000 )
	ROM_LOAD( "sk4_ic10.bin", 0x7000, 0x1000 )
	ROM_LOAD( "sk4_ic13.bin", 0x8000, 0x1000 )
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000 )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000 )
	ROM_LOAD( "sk4_ic16.bin", 0xb000, 0x1000 )
	ROM_LOAD( "sk4_ic13.bin", 0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800 )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800 )
ROM_END



struct GameDriver vanguard_driver =
{
	"vanguard",
	&machine_driver,

	vanguard_rom,
	0, 0,

	input_ports, dsw,

	0, palette, colortable,
	0, 10,
	0x06, 0x04,
	8*13, 8*16, 0x00,

	0, 0
};
