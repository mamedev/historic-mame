/***************************************************************************

Lady Bug memory map (preliminary)

0000-5fff ROM
6000-6fff RAM
d000-d3ff video RAM
d400-d7ff color RAM (4 bits wide)

memory mapped ports:

read:
9000      IN0
9001      IN1
9002      DSW1
9003      DSW2
8000      interrupt enable? (toggle)?

*
 * IN0 (all bits are inverted)
 * bit 7 : TILT if this is 0 coins are not accepted
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : FIRE player 1
 * bit 3 : UP player 1
 * bit 2 : RIGHT player 1
 * bit 1 : DOWN player 1
 * bit 0 : LEFT player 1
 *
*
 * IN1 (player input bits are inverted)
 * bit 7 : VBLANK
 * bit 6 : VBLANK inverted
 * bit 5 :
 * bit 4 : FIRE player 2 (TABLE only)
 * bit 3 : UP player 2 (TABLE only)
 * bit 2 : RIGHT player 2 (TABLE only)
 * bit 1 : DOWN player 2 (TABLE only)
 * bit 0 : LEFT player 2 (TABLE only)
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 : DIP SWITCH 8  0 = 5 lives 1 = 3 lives
 * bit 6 : DIP SWITCH 7  Free Play
 * bit 5 : DIP SWITCH 6  TABLE or STANDUP (0 = STANDUP)
 * bit 4 : DIP SWITCH 5  Pause
 * bit 3 : DIP SWITCH 4  RACK TEST
 * bit 2 : DIP SWITCH 3  0 = 3 letter initials 1 = 10 letter initials
 * bit 1 : DIP SWITCH 2\ Difficulty level
 * bit 0 : DIP SWITCH 1/ 11 = 1st (easy) 10 = 2nd 01 = 3rd 00 = 4th (hard)
 *
*
 * DSW2 (all bits are inverted)
 * bit 7 : DIP SWITCH 8\
 * bit 6 : DIP SWITCH 7| Left coin slot
 * bit 5 : DIP SWITCH 6|
 * bit 4 : DIP SWITCH 5/
 * bit 3 : DIP SWITCH 4\
 * bit 2 : DIP SWITCH 3| Right coin slot
 * bit 1 : DIP SWITCH 2|
 * bit 0 : DIP SWITCH 1/
 *                       1111 = 1 coin 1 play
 *                       1110 = 1 coin 2 plays
 *                       1101 = 1 coin 3 plays
 *                       1100 = 1 coin 4 plays
 *                       1011 = 1 coin 5 plays
 *                       1010 = 2 coins 1 play
 *                       1001 = 2 coins 3 plays
 *                       1000 = 3 coins 1 play
 *                       0111 = 3 coins 2 plays
 *                       0110 = 4 coins 1 play
 *                 all others = 1 coin 1 play
 *

write:
7000-73ff sprites
a000      watchdog reset?
b000      sound port 1
c000      sound port 2

interrupts:
There is no vblank interrupt. The vblank status is read from IN1.
Coin insertion in left slot generates an interrupt, in right slot a NMI.

***************************************************************************/

#include "driver.h"
#include "machine.h"
#include "common.h"

int ladybug_IN0_r(int address,int offset);
int ladybug_IN1_r(int address,int offset);
int ladybug_DSW1_r(int address,int offset);
int ladybug_DSW2_r(int address,int offset);
int ladybug_interrupt(void);

int ladybug_videoram_r(int address,int offset);
int ladybug_colorram_r(int address,int offset);
void ladybug_videoram_w(int address,int offset,int data);
void ladybug_colorram_w(int address,int offset,int data);
void ladybug_sprite_w(int address,int offset,int data);
int ladybug_vh_start(void);
void ladybug_vh_stop(void);
void ladybug_vh_screenrefresh(void);

void ladybug_sound1_w(int address,int offset,int data);
void ladybug_sound2_w(int address,int offset,int data);
int ladybug_sh_start(void);
void ladybug_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, ram_r },
	{ 0xd000, 0xd3ff, ladybug_videoram_r },
	{ 0xd400, 0xd7ff, ladybug_colorram_r },
	{ 0x0000, 0x5fff, rom_r },
	{ 0x9000, 0x9000, ladybug_IN0_r },
	{ 0x9001, 0x9001, ladybug_IN1_r },
	{ 0x9002, 0x9002, ladybug_DSW1_r },
	{ 0x9003, 0x9003, ladybug_DSW2_r },
	{ 0x8000, 0x8000, 0 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6fff, ram_w },
	{ 0xd000, 0xd3ff, ladybug_videoram_w },
	{ 0xd400, 0xd7ff, ladybug_colorram_w },
	{ 0x7000, 0x73ff, ladybug_sprite_w },
	{ 0xb000, 0xbfff, ladybug_sound1_w },
	{ 0xc000, 0xcfff, ladybug_sound2_w },
	{ 0xa000, 0xa000, 0 },
	{ 0x0000, 0x5fff, rom_w },
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 0, 0x80, "LIVES", { "5", "3" }, 1 },
	{ 0, 0x03, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 0, 0x04, "INITIALS", { "3 LETTERS", "10 LETTERS" } },
	{ -1 }
};



static struct RomModule rom[] =
{
	/* code */
	{ "lb1.cpu", 0x00000, 0x1000 },
	{ "lb2.cpu", 0x01000, 0x1000 },
	{ "lb3.cpu", 0x02000, 0x1000 },
	{ "lb4.cpu", 0x03000, 0x1000 },
	{ "lb5.cpu", 0x04000, 0x1000 },
	{ "lb6.cpu", 0x05000, 0x1000 },
	/* gfx */
	{ "lb9.vid",  0x10000, 0x1000 },
	{ "lb10.vid", 0x11000, 0x1000 },
	{ "lb8.cpu",  0x12000, 0x1000 },
	{ "lb7.cpu",  0x13000, 0x1000 },
	{ 0 }	/* end of table */
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	512*8*8,	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	1,	/* the two bitplanes are packed in two consecutive bits */
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 8*16+14, 8*16+12, 8*16+10, 8*16+8, 8*16+6, 8*16+4, 8*16+2, 8*16+0,
			14, 12, 10, 8, 6, 4, 2, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,   0, 7 },
	{ 0x12000, &spritelayout, 8, 23 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x49,0x00,	/* LTRED */
	0x00,0x92,0x00,	/* DKGRN */
	0xdb,0x92,0x00,	/* ORANGE */
	0x00,0xdb,0x00,	/* GREEN */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xdb,0x92,0x49,	/* ORANGE2 */
	0x49,0xdb,0x49,	/* GRN2 */
	0xdb,0xdb,0x49,	/* YELLOW2 */
	0xdb,0x00,0x92,	/* PURPLE2 */
	0x92,0x92,0x92,	/* GRAY */
	0x92,0xdb,0x92,	/* LTGRN */
	0x00,0x00,0xdb,	/* BLUE */
	0x92,0x49,0xdb,	/* PURPLE */
	0x00,0x92,0xdb,	/* LTBLUE */
	0x92,0x92,0xdb,	/* LTPURPLE */
	0xdb,0x92,0xdb,	/* PINK */
	0x92,0xdb,0xdb,	/* CYAN */
	0xdb,0xdb,0xdb	/* WHITE */
};

enum {BLACK,RED,LTRED,DKGRN,ORANGE,GREEN,YELLOW,ORANGE2,GRN2,YELLOW2,
		PURPLE2,GRAY,LTGRN,BLUE,PURPLE,LTBLUE,LTPURPLE,PINK,CYAN,WHITE};

static unsigned char colortable[] =
{
	/* characters */
	BLACK,LTPURPLE,PINK,GRN2,	/* maze walls, doors */
	BLACK,RED,GREEN,YELLOW,		/* Lady Bug */
	BLACK,CYAN,WHITE,GRAY,		/* logo on title screen; CREDIT #; coin; dots */
	BLACK,WHITE,GRAY,ORANGE2,	/* top scores */
	BLACK,GREEN,LTGRN,DKGRN,	/* score */
	BLACK,GRN2,LTBLUE,PINK,		/* 2x 3x 5x; timer; PART # */
	BLACK,WHITE,YELLOW,PINK,	/* EXTRA; skull; timer */
	BLACK,BLACK,LTRED,PINK,		/* SPECIAL */

	/* sprites */
	BLACK,GREEN,RED,YELLOW,		/* Lady Bug; cucumber; parsley; red peper */
	BLACK,GREEN,PURPLE,WHITE,	/* 1st level monster; egg plant */
	BLACK,GREEN,PURPLE2,WHITE,	/* 2nd level monster; sweet potato */
	BLACK,YELLOW,ORANGE,WHITE,	/* 4th level monster */
	BLACK,LTGRN,GREEN,WHITE,	/* 3rd level monster; japanese radish; turnip; celery; horseradish */
	BLACK,YELLOW,ORANGE,WHITE,	/* 5th level monster; pumpkin; bamboo shoot; potato */
	BLACK,LTGRN,ORANGE,YELLOW,	/* 6th level monster; onion */
	BLACK,YELLOW,ORANGE,WHITE,	/* 7th level monster */
	BLACK,GREEN,CYAN,GRN2,		/* 8th level monster */
	BLACK,LTGRN,WHITE,YELLOW,	/* dead Lady Bug; chinese cabbage */
	BLACK,YELLOW2,ORANGE,WHITE,	/* mushroom */
	BLACK,GREEN,ORANGE,LTPURPLE,	/* carrot */
	BLACK,GREEN,RED,WHITE,		/* radish; tomato */
	BLACK,GREEN,BLUE,WHITE,
	BLACK,GREEN,YELLOW,WHITE,
	BLACK,YELLOW2,ORANGE,LTPURPLE	/* scores */
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
};



const struct MachineDriver ladybug_driver =
{
	"ladybug",
	rom,
	/* basic machine hardware */
	4000000,	/* 4 Mhz */
	60,
	readmem,
	writemem,
	dsw, { 0xdf , 0xff },
	0,
	ladybug_interrupt,
	0,

	/* video hardware */
	256,256,
	gfxdecodeinfo,
	palette,sizeof(palette)/3,
	colortable,sizeof(colortable)/4,
	0,10,
	0x02,0x06,
	8*13,8*30,0x08,
	0,
	ladybug_vh_start,
	ladybug_vh_stop,
	ladybug_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	ladybug_sh_start,
	0,
	0,
	0,
	ladybug_sh_update
};
