/***************************************************************************

Burgertime memory map (preliminary)

MAIN BOARD:

0000-07ff RAM
0c00-0c1f palette
1000-13ff Video RAM
1400-17ff Attributes RAM
1800-181f Sprite ram
b000-ffff ROM

read:
4000      IN0
4001      IN1
4002      coin
4003      DSW1
4004      DSW2

write
4000      Coinbox enable
4001      not used
4002      flip screen
4003      command to sound board / trigger interrupt on sound board
4004      Map number
5005      ?

IN0  Player 1 Joystick
7\
6 |
5 |
4 |  Pepper
3 |  Down
2 |  Up
1 |  Left
0/   Right

IN1  Player 2 Joystick (TABLE only)
7\
6 |
5 |
4 |  Pepper
3 |  Down
2 |  Up
1 |  Left
0/   Right

Coin slot
7\   Coin Right side
6 |  Coin Left Side
5 |
4 |
3 |
2 |  Tilt  (must be set to 1)
1 |  Player 2 start
0/   Player 1 start

DSW1
7    HVBlank input toggle (???)
6    TABLE or UPRIGHT cabinet select (0 = UPRIGHT)
5\   Diagnostic bit 2
4/   Diagnostic bit 1
3\
2/   Credit base for slot 2
1\
0/   Credit base for slot 1

DSW2
7
6
5
4    Pepper awarded at end level?
3    4 or 6 pursuers
2\   Select bonus chef award
1/
0    3 or 5 chefs


interrupts:
A NMI causes reset.


SOUND BOARD:
0000-03ff RAM
f000-ffff ROM

read:
a000      command from CPU board / interrupt acknowledge

write:
2000      8910 #1  write
4000      8910 #1  control
6000      8910 #2  write
8000      8910 #2  control
c000      NMI enable

interrupts:
NMI used for timing (music etc), clocked at (I think) 8V
IRQ triggered by commands sent by the main CPU.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int btime_DSW1_r(int offset);
extern int btime_init_machine(const char *gamename);
extern int btime_interrupt(void);

extern void btime_background_w(int offset,int data);
extern void btime_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void btime_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int btime_soundcommand_r(int offset);
extern void btime_soundcommand_w(int offset,int data);
extern void btime_sh_control_port1_w(int offset,int data);
extern void btime_sh_control_port2_w(int offset,int data);
extern void btime_sh_write_port1_w(int offset,int data);
extern void btime_sh_write_port2_w(int offset,int data);
extern void btime_sh_interrupt_enable_w(int offset,int data);
extern int btime_sh_interrupt(void);
extern int btime_sh_start(void);
extern void btime_sh_stop(void);
extern void btime_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4003, 0x4003, btime_DSW1_r },	/* DSW1 */
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x181f, MRA_RAM },
	{ 0xb000, 0xffff, MRA_ROM },
	{ 0x4000, 0x4000, input_port_0_r },	/* IN0 */
	{ 0x4001, 0x4001, input_port_1_r },	/* IN1 */
	{ 0x4002, 0x4002, input_port_2_r },	/* coin */
	{ 0x4004, 0x4004, input_port_4_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x181f, MWA_RAM, &spriteram },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x4003, 0x4003, btime_soundcommand_w },
	{ 0x4004, 0x4004, btime_background_w },
	{ 0xb000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xa000, 0xa000, btime_soundcommand_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, btime_sh_write_port1_w },
	{ 0x4000, 0x4000, btime_sh_control_port1_w },
	{ 0x6000, 0x6000, btime_sh_write_port2_w },
	{ 0x8000, 0x8000, btime_sh_control_port2_w },
	{ 0xc000, 0xc000, btime_sh_interrupt_enable_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0},
		{ 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{	/* IN2 */
		0x3f,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, OSD_KEY_4, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x3f,
		{ 0, 0, 0, 0, OSD_KEY_F1, OSD_KEY_F2, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 4, 0x01, "LIVES", { "5", "3" }, 1 },
	{ 4, 0x06, "BONUS", { "30000", "20000", "15000", "10000" }, 1 },
/*	{ 4, 0x06, "BONUS", { "50000", "40000", "30000", "20000" }, 1 },*/
	{ 4, 0x08, "PURSUERS", { "6", "4" }, 1 },
	{ 4, 0x10, "END OF LEVEL PEPPER", { "YES", "NO" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 1024*8*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*8 sprites */
	128,    /* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 128*16*16, 128*2*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,  /* 16*16 characters */
	16,    /* 16 characters */
	3,	/* 3 bits per pixel */
	{ 0, 64*16*16, 64*2*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* actually every char set uses 1 color, not 2 - but the second here is */
	/* used to obtain two different colors in the dip switch menu */
	{ 1, 0x0000, &charlayout,   0, 2 },	/* char set #1 */
	{ 1, 0x3000, &charlayout,   0, 1 },	/* char set #2 */
	{ 1, 0x6000, &charlayout2,  8, 1 },	/* background tiles */
	{ 1, 0x0000, &spritelayout, 0, 1 },	/* sprites */
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0xff,0x00,0xd0,0xc0,0xf8,0xc7,0xe1,0xd4,
	0xff,0x52,0x07,0x3f,0x00,0xf8,0xc0,0x38
};



const struct MachineDriver btime_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			btime_interrupt,12	/* 12 interrupts per frame */
		},
		{
			CPU_M6502,
			500000,	/* 500 khz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			btime_sh_interrupt,14	/* 14 (??) interrupts per frame */
		}
	},
	60,
	input_ports,dsw,
	btime_init_machine,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16,2*8,
	color_prom,btime_vh_convert_color_prom,0,0,
	1,11,
	0x00,0x01,
	8*13,8*16,0x00,
	0,
	generic_vh_start,
	generic_vh_stop,
	btime_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	btime_sh_start,
	btime_sh_stop,
	btime_sh_update
};
