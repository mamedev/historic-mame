/***************************************************************************

Bomb Jack memory map (preliminary)

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-83ff RAM 0
8400-87ff RAM 1
8800-8bff RAM 2
8c00-8fff RAM 3
9000-93ff Video RAM (RAM 4)
9400-97ff Color RAM (RAM 4)
c000-dfff ROM 4

memory mapped ports:
read:
b000      IN0
b001      IN1
b002      IN2
b003      watchdog reset?
b004      DSW1
b005      DSW2

write:
9820-987f sprites
9a00      ?
9c00-9cff palette
9e00      background image selector
b000      interrupt enable
b800      ?

***************************************************************************/

#include "driver.h"


extern unsigned char *bombjack_videoram;
extern unsigned char *bombjack_colorram;
extern unsigned char *bombjack_spriteram;
extern unsigned char *bombjack_paletteram;
extern void bombjack_videoram_w(int offset,int data);
extern void bombjack_colorram_w(int offset,int data);
extern void bombjack_paletteram_w(int offset,int data);
extern void bombjack_background_w(int offset,int data);
extern void bombjack_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int bombjack_vh_start(void);
extern void bombjack_vh_stop(void);
extern void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },	/* including video and color RAM */
	{ 0xb003, 0xb003, MRA_NOP },
	{ 0xb000, 0xb000, input_port_0_r },	/* player 1 input */
	{ 0xb001, 0xb001, input_port_1_r },	/* player 2 input */
	{ 0xb002, 0xb002, input_port_2_r },	/* coin */
	{ 0xb004, 0xb004, input_port_3_r },	/* DSW1 */
	{ 0xb005, 0xb005, input_port_4_r },	/* DSW2 */
	{ 0xc000, 0xdfff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x93ff, bombjack_videoram_w, &bombjack_videoram },
	{ 0x9400, 0x97ff, bombjack_colorram_w, &bombjack_colorram },
	{ 0x9820, 0x987f, MWA_RAM, &bombjack_spriteram },
	{ 0x9c00, 0x9cff, bombjack_paletteram_w, &bombjack_paletteram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0x9e00, 0x9e00, bombjack_background_w },
	{ 0xb800, 0xb800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xc0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};



static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "3", "4", "5", "2" } },
	{ 4, 0x18, "DIFFICULTY 1", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ 4, 0x60, "DIFFICULTY 2", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ 4, 0x80, "SPECIAL", { "EASY", "HARD" } },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" } },
	{ 4, 0x07, "INITIAL HIGH SCORE", { "10000", "100000", "30000", "50000", "100000", "50000", "100000", "50000" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 2*512*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	32,	/* 32 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 87*8, 86*8, 85*8, 84*8, 83*8, 82*8, 81*8, 80*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
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



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,   256, 16 },	/* characters */
	{ 1, 0x3000, &charlayout2,   256, 16 },	/* background tiles */
	{ 1, 0x9000, &spritelayout1, 256, 16 },	/* normal sprites */
	{ 1, 0xa000, &spritelayout2, 256, 16 },	/* large sprites */
	{ 0, 0,      &fakelayout,      0, 256 },
	{ -1 } /* end of array */
};



const struct MachineDriver bombjack_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0, 32*8-1 },
	gfxdecodeinfo,
	256,256+8*16,
	0,bombjack_vh_convert_color_prom,0,0,
	48,65,
	5,0,
	8*13,8*16,2,
	0,
	bombjack_vh_start,
	bombjack_vh_stop,
	bombjack_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

