/***************************************************************************

Mario Bros memory map (preliminary):

0000-5fff ROM
6000-6fff RAM
7000-73ff ?
7400-77ff Video RAM
f000-ffff ROM

read:
7c00      IN0
7c80      IN1
7f80      DSW

*
 * IN0 (bits NOT inverted)
 * bit 7 : TEST
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : JUMP player 1
 * bit 3 : ? DOWN player 1 ?
 * bit 2 : ? UP player 1 ?
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : JUMP player 2
 * bit 3 : ? DOWN player 2 ?
 * bit 2 : ? UP player 2 ?
 * bit 1 : LEFT player 2
 * bit 0 : RIGHT player 2
 *
*
 * DSW (bits NOT inverted)
 * bit 7 : \ difficulty
 * bit 6 : / 00 = easy  01 = medium  10 = hard  11 = hardest
 * bit 5 : \ bonus
 * bit 4 : / 00 = 20000  01 = 30000  10 = 40000  11 = none
 * bit 3 : \ coins per play
 * bit 2 : /
 * bit 1 : \ 00 = 3 lives  01 = 4 lives
 * bit 0 : / 10 = 5 lives  11 = 6 lives
 *

write:
7d00      ?
7d80      ?
7e00      ?
7e80-7e83 ?
7e84      interrupt enable
7e85      ?
7f00-7f06 ?


I/O ports

write:
00        ?

***************************************************************************/

#include "driver.h"


extern unsigned char *mario_videoram;
extern unsigned char *mario_spriteram;
extern void mario_videoram_w(int offset,int data);
extern void mario_gfxbank_w(int offset,int data);
extern int  mario_vh_start(void);
extern void mario_vh_stop(void);
extern void mario_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7f80, 0x7f80, input_port_2_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &mario_spriteram },
	{ 0x7400, 0x77ff, mario_videoram_w, &mario_videoram },
	{ 0x7e80, 0x7e80, mario_gfxbank_w },
	{ 0x7e84, 0x7e84, interrupt_enable_w },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, OSD_KEY_1, OSD_KEY_2, OSD_KEY_F1 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 },
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_X, OSD_KEY_Z, 0, 0, OSD_KEY_SPACE, OSD_KEY_3, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x30, "BONUS", { "20000", "30000", "40000", "NONE" } },
	{ 2, 0xc0, "DIFFICULTY", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 256*16*16, 2*256*16*16 },	/* the bitplanes are separated */
	{ 256*16*8+7, 256*16*8+6, 256*16*8+5, 256*16*8+4, 256*16*8+3, 256*16*8+2, 256*16*8+1, 256*16*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },	/* the two halves of the sprite are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,      0, 16 },
	{ 0x12000, &spritelayout, 4*16, 16 },
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
	/* chars */
	0,1,2,3,
	0,2,3,4,
	0,3,4,5,
	0,4,5,6,
	0,5,6,7,
	0,6,7,8,
	0,7,8,9,
	0,8,9,10,
	0,9,10,11,
	0,10,11,12,
	0,11,12,13,
	0,12,13,14,
	0,13,14,15,
	0,14,15,1,
	0,15,1,2,
	0,15,1,2,

	/* sprites */
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,
	0,12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14
};



const struct MachineDriver mario_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	readmem,writemem,0,0,
	input_ports,dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,17,
	1,11,
	8*13,8*16,0,
	0,
	mario_vh_start,
	mario_vh_stop,
	mario_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

