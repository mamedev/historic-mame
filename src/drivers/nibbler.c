/***************************************************************************

Nibbler memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM (playfield 1)
0800-0cff Video RAM (playfield 2)
0d00-0fff Color RAM (4 bits per playfield)
1000-1fff Character generator RAM
3000-bfff ROM

read:
2104      IN0
2105      IN1
2106      DSW
2107      IN2

*
 * IN0 (bits NOT inverted)
 * bit 7 : LEFT player 1
 * bit 6 : RIGHT player 1
 * bit 5 : UP player 1
 * bit 4 : DOWN player 1
 * bit 3 : ?\
 * bit 2 : ?| debug
 * bit 1 : ?| commands?
 * bit 0 : ?/
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : LEFT player 2 (TABLE only)
 * bit 6 : RIGHT player 2 (TABLE only)
 * bit 5 : UP player 2 (TABLE only)
 * bit 4 : DOWN player 2 (TABLE only)
 * bit 3 : ?\
 * bit 2 : ?| debug
 * bit 1 : ?| commands?
 * bit 0 : ?/
 *
*
 * DSW (bits NOT inverted)
 * bit 7 :
 * bit 6 :
 * bit 5 : Free Play
 * bit 4 : RACK TEST
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :
 * bit 1 :\ nr of lives
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 6
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : ?
 * bit 4 : ?
 * bit 3 : ?
 * bit 2 : ?
 * bit 1 : ?
 * bit 0 : ?
 *

write:
2000-2001 ?
2100-2103 ?
2200      ?
2300      ?

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

***************************************************************************/

#include "driver.h"



extern int nibbler_interrupt(void);

extern unsigned char *nibbler_videoram1;
extern unsigned char *nibbler_videoram2;
extern unsigned char *nibbler_colorram;
extern unsigned char *nibbler_characterram;
extern void nibbler_videoram1_w(int offset,int data);
extern void nibbler_videoram2_w(int offset,int data);
extern void nibbler_colorram_w(int offset,int data);
extern void nibbler_characterram_w(int offset,int data);
extern int nibbler_vh_start(void);
extern void nibbler_vh_stop(void);
extern void nibbler_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0xbfff, MRA_ROM },
	{ 0x2104, 0x2104, input_port_0_r },	/* IN0 */
	{ 0x2105, 0x2105, input_port_1_r },	/* IN1 */
	{ 0x2106, 0x2106, input_port_2_r },	/* DSW */
	{ 0x2107, 0x2107, input_port_3_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, nibbler_videoram1_w, &nibbler_videoram1 },
	{ 0x0800, 0x0bff, nibbler_videoram2_w, &nibbler_videoram2 },
	{ 0x0c00, 0x0fff, nibbler_colorram_w, &nibbler_colorram },
	{ 0x1000, 0x1fff, nibbler_characterram_w, &nibbler_characterram },
	{ 0x3000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, 0, 0, OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, OSD_JOY_DOWN, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT },
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
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



struct GfxLayout nibbler_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xf000, &nibbler_charlayout, 0, 16 },	/* the game dynamically modifies this */
	{ 1, 0x0800, &charlayout2,        0, 16 },
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



const struct MachineDriver nibbler_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			nibbler_interrupt,1
		}
	},
	60,
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,10,
	0x06,0x04,
	8*13,8*16,0x00,
	0,
	nibbler_vh_start,
	nibbler_vh_stop,
	nibbler_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
