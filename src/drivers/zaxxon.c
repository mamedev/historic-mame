/***************************************************************************

Zaxxon memory map (preliminary)

0000-1fff ROM 3
2000-3fff ROM 2
4000-4fff ROM 1
6000-67ff RAM 1
6800-6fff RAM 2
8000-83ff Video RAM
a000-a0ff sprites

read:
c000      IN0
c001      IN1
c002      DSW1
c003      DSW2
c100      IN2

*
 * IN0 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 1
 * bit 3 : UP player 1
 * bit 2 : DOWN player 1
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 2 (TABLE only)
 * bit 3 : UP player 2 (TABLE only)
 * bit 2 : DOWN player 2 (TABLE only)
 * bit 1 : LEFT player 2 (TABLE only)
 * bit 0 : RIGHT player 2 (TABLE ony)
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : CREDIT
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : ?
 * bit 3 : START 2
 * bit 2 : START 1
 * bit 1 : ?
 * bit 0 : ?
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 :  COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 6 :  sound  0 = off  1 = on
 * bit 5 :\ lives
 * bit 4 :/ 00 = infinite  01 = 4  10 = 5  11 = 3
 * bit 3 :  ?
 * bit 2 :  ?
 * bit 1 :\ bonus
 * bit 0 :/ 00 = 40000  01 = 20000  10 = 30000  11 = 10000
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 :  COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 6 :  sound  0 = off  1 = on
 * bit 5 :\ lives
 * bit 4 :/ 00 = infinite  01 = 4  10 = 5  11 = 3
 * bit 3 :  ?
 * bit 2 :  ?
 * bit 1 :\ bonus
 * bit 0 :/ 00 = 40000  01 = 20000  10 = 30000  11 = 10000
 *
*
 * DSW2 (bits NOT inverted)
 * bit 7 :\
 * bit 6 :|  right coin slot
 * bit 5 :|
 * bit 4 :/
 * bit 3 :\
 * bit 2 :|  left coin slot
 * bit 1 :|
 * bit 0 :/
 *

write:
c000-c002 ?
c006      ?
ff3c-ff3f ?
fff0      interrupt enable
fff1      ?
fff8-fff9 background playfield position
fffa-fffb ?
fffe      ?

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI causes a ROM/RAM test.

***************************************************************************/

#include "driver.h"


extern int zaxxon_IN2_r(int offset);

extern unsigned char *zaxxon_videoram;
extern unsigned char *zaxxon_colorram;
extern unsigned char *zaxxon_spriteram;
extern unsigned char *zaxxon_background_position;
extern void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void zaxxon_videoram_w(int offset,int data);
extern void zaxxon_colorram_w(int offset,int data);
extern int  zaxxon_vh_start(void);
extern void zaxxon_vh_stop(void);
extern void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc100, 0xc100, zaxxon_IN2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc003, 0xc003, input_port_4_r },
	{ 0x0000, 0x4fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, zaxxon_videoram_w, &zaxxon_videoram },
	{ 0xa000, 0xa0ff, MWA_RAM, &zaxxon_spriteram },
	{ 0xfff0, 0xfff0, interrupt_enable_w },
	{ 0xfff8, 0xfff9, MWA_RAM, &zaxxon_background_position },
	{ 0x0000, 0x4fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, OSD_KEY_1, OSD_KEY_2,
				0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x33,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "INFINITE", "4", "5", "3" }, 1 },
	{ 3, 0x03, "BONUS", { "40000", "20000", "30000", "10000" }, 1 },
	{ 3, 0x40, "SOUND", { "OFF", "ON" } },
	{ 3, 0x04, "SW3", { "OFF", "ON" } },
	{ 3, 0x08, "SW4", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 103*8, 102*8, 101*8, 100*8, 99*8, 98*8, 97*8, 96*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,  0, 16 },	/* characters */
	{ 1, 0x1000, &spritelayout, 0, 16 },	/* sprites */
	{ 1, 0x7000, &charlayout2,  0, 16 },	/* background graphics */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0xdb,0x00,	/* UNUSED */
	0x00,0xdb,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x88,0x88,0x88,	/* UNUSED */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xff,0x00,0xdb,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,
		UNUSED2,YELLOW,UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,GREY};

static unsigned char colortable[] =
{
	0,1,2,3,4,5,6,7,
	0,2,3,4,5,6,7,8,
	0,3,4,5,6,7,8,9,
	0,4,5,6,7,8,9,10,
	0,5,6,7,8,9,10,11,
	0,6,7,8,9,10,11,12,
	0,7,8,9,10,11,12,13,
	0,8,9,10,11,12,13,14,
	0,9,10,11,12,13,14,15,
	0,10,11,12,13,14,15,1,
	0,11,12,13,14,15,1,2,
	0,12,13,14,15,1,2,3,
	0,13,14,15,1,2,3,4,
	0,14,15,1,2,3,4,5,
	0,15,1,2,3,4,5,6,
	0,15,1,2,3,4,5,6
};



const struct MachineDriver zaxxon_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
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
	80,97,
	0x00,0x01,
	8*13,8*16,0x05,
	0,
	zaxxon_vh_start,
	zaxxon_vh_stop,
	zaxxon_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
