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
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int btime_DSW1_r(int offset);
extern int btime_interrupt(void);

extern void btime_background_w(int offset,int data);
extern void btime_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void btime_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void btime_sh_interrupt_enable_w(int offset,int data);
extern int btime_sh_interrupt(void);
extern int btime_sh_start(void);



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
	{ 0x4003, 0x4003, sound_command_w },
	{ 0x4004, 0x4004, btime_background_w },
	{ 0xb000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xa000, 0xa000, sound_command_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
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


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "JUMP" },
        { -1 }
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



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			readmem,writemem,0,0,
			btime_interrupt,12	/* 12 interrupts per frame */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			500000,	/* 500 khz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			btime_sh_interrupt,14	/* 14 (??) interrupts per frame */
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16,2*8,
	btime_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	btime_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	btime_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( btime_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ab05a1.12b", 0xb000, 0x1000 )
	ROM_LOAD( "ab04.9b",    0xc000, 0x1000 )
	ROM_LOAD( "ab06.13b",   0xd000, 0x1000 )
	ROM_LOAD( "ab05.10b",   0xe000, 0x1000 )
	ROM_LOAD( "ab07.15b",   0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x7800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ab8.13k",    0x0000, 0x1000 )	/* charset #1 */
	ROM_LOAD( "ab10.10k",   0x1000, 0x1000 )
	ROM_LOAD( "ab12.7k",    0x2000, 0x1000 )
	ROM_LOAD( "ab9.15k",    0x3000, 0x1000 )	/* charset #2 */
	ROM_LOAD( "ab11.12k",   0x4000, 0x1000 )
	ROM_LOAD( "ab13.9k" ,   0x5000, 0x1000 )
	ROM_LOAD( "ab02.4b",    0x6000, 0x0800 )	/* charset #3 */
	ROM_LOAD( "ab01.3b",    0x6800, 0x0800 )
	ROM_LOAD( "ab00.1b",    0x7000, 0x0800 )

	ROM_REGION(0x0800)	/* background graphics */
	ROM_LOAD( "ab03.6b",    0x0000, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ab14.12h",   0xf000, 0x1000 )
ROM_END

ROM_START( btimea_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "aa04.9b",    0xc000, 0x1000 )
	ROM_LOAD( "aa06.13b",   0xd000, 0x1000 )
	ROM_LOAD( "aa05.10b",   0xe000, 0x1000 )
	ROM_LOAD( "aa07.15b",   0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x7800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "aa8.13k",    0x0000, 0x1000 )	/* charset #1 */
	ROM_LOAD( "aa10.10k",   0x1000, 0x1000 )
	ROM_LOAD( "aa12.7k",    0x2000, 0x1000 )
	ROM_LOAD( "aa9.15k",    0x3000, 0x1000 )	/* charset #2 */
	ROM_LOAD( "aa11.12k",   0x4000, 0x1000 )
	ROM_LOAD( "aa13.9k" ,   0x5000, 0x1000 )
	ROM_LOAD( "aa02.4b",    0x6000, 0x0800 )	/* charset #3 */
	ROM_LOAD( "aa01.3b",    0x6800, 0x0800 )
	ROM_LOAD( "aa00.1b",    0x7000, 0x0800 )

	ROM_REGION(0x0800)	/* background graphics */
	ROM_LOAD( "aa03.6b",    0x0000, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "aa14.12h",   0xf000, 0x1000 )
ROM_END



static unsigned btime_decode(int A)
{
	/* the encryption is a simple bit rotation: 76543210 -> 65342710      */
	/* it is not known, however, when it must be applied. Only opcodes at */
	/* addresses with this bit pattern:                                   */
	/* xxxx xxx1 xxxx x1xx                                                */
	/* _can_ (but not necessarily are) be encrypted.                      */

	switch (A)
	{
		case 0xb105:
		case 0xb116:
		case 0xb11e:
		case 0xb12c:
		case 0xb13d:
		case 0xb146:
		case 0xb14d:
		case 0xb154:
		case 0xb15e:
		case 0xb175:
		case 0xb18f:
		case 0xb195:
		case 0xb19d:
		case 0xb1a4:
		case 0xb1bd:
		case 0xb1c4:
		case 0xb1cf:
		case 0xb1d4:
		case 0xb1dd:
		case 0xb1f4:
		case 0xb31c:
		case 0xb326:
		case 0xb336:
		case 0xb347:
		case 0xb34f:
		case 0xb35d:
		case 0xb396:
		case 0xb39e:
		case 0xb3ad:
		case 0xb3af:
		case 0xb3bc:
		case 0xb3cc:
		case 0xb3dd:
		case 0xb3ed:
		case 0xb3f7:
		case 0xb507:
		case 0xb50c:
		case 0xb516:
		case 0xb525:
		case 0xb52d:
		case 0xb537:
		case 0xb53c:
		case 0xb544:
		case 0xb55d:
		case 0xb567:
		case 0xb56c:
		case 0xb56f:
		case 0xb575:
		case 0xb57d:
		case 0xb587:
		case 0xb58c:
		case 0xb594:
		case 0xb59e:
		case 0xb5ac:
		case 0xb5b6:
		case 0xb5be:
		case 0xb5d4:
		case 0xb5df:
		case 0xb5e4:
		case 0xb5e6:
		case 0xb5fd:
		case 0xb5ff:
		case 0xb707:
		case 0xb70d:
		case 0xb715:
		case 0xb71f:
		case 0xb73d:
		case 0xb744:
		case 0xb746:
		case 0xb74c:
		case 0xb755:
		case 0xb757:
		case 0xb76d:
		case 0xb786:
		case 0xb7d6:
		case 0xb7dc:
		case 0xb7fe:
		case 0xb90e:
		case 0xb916:
		case 0xb91e:
		case 0xb927:
		case 0xb92d:
		case 0xb92f:
		case 0xb937:
		case 0xb93f:
		case 0xb947:
		case 0xb965:
		case 0xb96f:
		case 0xb974:
		case 0xb97e:
		case 0xb986:
		case 0xb98c:
		case 0xb99f:
		case 0xb9a5:
		case 0xb9bc:
		case 0xb9ce:
		case 0xb9f6:
		case 0xbb0d:
		case 0xc105:
		case 0xc10e:
		case 0xc115:
		case 0xc15e:
		case 0xc16c:
		case 0xc17f:
		case 0xc187:
		case 0xc18d:
		case 0xc1b5:
		case 0xc1bd:
		case 0xc1bf:
		case 0xc1de:
		case 0xc1ed:
		case 0xc1f5:
		case 0xc347:
		case 0xc34d:
		case 0xc355:
		case 0xc35c:
		case 0xc35f:
		case 0xc365:
		case 0xc36e:
		case 0xc377:
		case 0xc37c:
		case 0xc386:
		case 0xc396:
		case 0xc3a4:
		case 0xc3ac:
		case 0xc3b7:
		case 0xc3bc:
		case 0xc3d6:
		case 0xc504:
		case 0xc50c:
		case 0xc51e:
		case 0xc525:
		case 0xc52e:
		case 0xc534:
		case 0xc537:
		case 0xc53d:
		case 0xc546:
		case 0xc54c:
		case 0xc54f:
		case 0xc557:
		case 0xc55e:
		case 0xc56e:
		case 0xc57d:
		case 0xc587:
		case 0xc58c:
		case 0xc594:
		case 0xc596:
		case 0xc59c:
		case 0xc5a6:
		case 0xc5bd:
		case 0xc7dd:
		case 0xc7ee:
		case 0xc7fd:
		case 0xc7ff:
		case 0xc906:
		case 0xc92f:
		case 0xc937:
		case 0xc93c:
		case 0xc946:
		case 0xc955:
		case 0xc95d:
		case 0xc966:
		case 0xc96f:
		case 0xc976:
		case 0xc97f:
		case 0xc986:
		case 0xc9b5:
		case 0xc9dd:
		case 0xcb17:
		case 0xcb1e:
		case 0xcb34:
		case 0xcb44:
		case 0xcb56:
		case 0xcb5e:
		case 0xcb6e:
		case 0xcb76:
		case 0xcb7e:
		case 0xcb8c:
		case 0xcb9e:
		case 0xcbad:
		case 0xcbb5:
		case 0xcbe4:
		case 0xcbe7:
		case 0xcd0e:
		case 0xcd14:
		case 0xcd1e:
		case 0xcd2d:
		case 0xcd3e:
		case 0xcd47:
		case 0xcd4d:
		case 0xcd56:
		case 0xcd6f:
		case 0xcd7c:
		case 0xcd8e:
		case 0xcda4:
		case 0xcdaf:
		case 0xcdb5:
		case 0xcdbf:
		case 0xcdc5:
		case 0xcdc7:
		case 0xcdce:
		case 0xcdd4:
		case 0xd105:
		case 0xd117:
		case 0xd11d:
		case 0xd124:
		case 0xd137:
		case 0xd13c:
		case 0xd15d:
		case 0xd197:
		case 0xd19f:
		case 0xd1a7:
		case 0xd1d6:
		case 0xd1dc:
		case 0xd1e4:
		case 0xd1ff:
		case 0xd306:
		case 0xd30d:
		case 0xd334:
		case 0xd345:
		case 0xd347:
		case 0xd36c:
		case 0xd394:
		case 0xd3ac:
		case 0xd3ae:
		case 0xd3b4:
		case 0xd3bf:
		case 0xd3c4:
		case 0xd3de:
		case 0xd3e6:
		case 0xd3ec:
		case 0xd3ee:
		case 0xd3ff:
		case 0xd50f:
		case 0xd526:
		case 0xd52c:
		case 0xd53c:
		case 0xd557:
		case 0xd55d:
		case 0xd56d:
		case 0xd57e:
		case 0xd58f:
		case 0xd597:
		case 0xd5a6:
		case 0xd5af:
		case 0xd5bf:
		case 0xd5ce:
		case 0xd5df:
		case 0xd5e7:
		case 0xd5f6:
		case 0xd72d:
		case 0xd734:
		case 0xd744:
		case 0xd757:
		case 0xd764:
		case 0xd78d:
		case 0xd90f:
		case 0xd92e:
		case 0xd937:
		case 0xd94d:
		case 0xd94f:
		case 0xd956:
		case 0xd95e:
		case 0xd965:
		case 0xd9b7:
		case 0xd9c5:
		case 0xd9cc:
		case 0xd9d7:
		case 0xd9f7:
		case 0xdb1d:
		case 0xdb45:
		case 0xdb4d:
		case 0xdb57:
		case 0xdb6d:
		case 0xdb77:
		case 0xdb8c:
		case 0xdba5:
		case 0xdbaf:
		case 0xdbb5:
		case 0xdbb7:
		case 0xdbbd:
		case 0xdbc5:
		case 0xdbd7:
		case 0xdbe5:
		case 0xdbed:
		case 0xdd1c:
		case 0xdd2e:
		case 0xdd47:
		case 0xdd6d:
		case 0xdd8d:
		case 0xdda4:
		case 0xddae:
		case 0xddb6:
		case 0xddbd:
		case 0xddd7:
		case 0xdddc:
		case 0xddde:
		case 0xddee:
		case 0xddfd:
		case 0xdf0f:
		case 0xdf15:
		case 0xdf1c:
		case 0xdf36:
		case 0xdf3d:
		case 0xdf67:
		case 0xdf75:
		case 0xdf87:
		case 0xdf8d:
		case 0xdf9f:
		case 0xdfa5:
		case 0xdfb7:
		case 0xdfbd:
		case 0xdfcf:
		case 0xdfd6:
		case 0xe10d:
		case 0xe127:
		case 0xe12e:
		case 0xe134:
		case 0xe13f:
		case 0xe157:
		case 0xe15c:
		case 0xe167:
		case 0xe16d:
		case 0xe17e:
		case 0xe18f:
		case 0xe1b5:
		case 0xe1be:
		case 0xe1e5:
		case 0xe1fe:
		case 0xe305:
		case 0xe315:
		case 0xe32d:
		case 0xe347:
		case 0xe355:
		case 0xe357:
		case 0xe35d:
		case 0xe36c:
		case 0xe376:
		case 0xe38d:
		case 0xe38f:
		case 0xe395:
		case 0xe3a4:
		case 0xe3ae:
		case 0xe3c5:
		case 0xe3c7:
		case 0xe3cd:
		case 0xe3dc:
		case 0xe3e6:
		case 0xe3fd:
		case 0xe3ff:
		case 0xe51c:
		case 0xe524:
		case 0xe53e:
		case 0xe546:
		case 0xe567:
		case 0xe56c:
		case 0xe586:
		case 0xe5ac:
		case 0xe5b5:
		case 0xe5be:
		case 0xe5c7:
		case 0xe5cc:
		case 0xe5ce:
		case 0xe704:
		case 0xe70f:
		case 0xe72e:
		case 0xe737:
		case 0xe746:
		case 0xe75e:
		case 0xe764:
		case 0xe775:
		case 0xe784:
		case 0xe794:
		case 0xe7bf:
		case 0xe7e7:
		case 0xe90f:
		case 0xe916:
		case 0xe947:
		case 0xe95e:
		case 0xe966:
		case 0xe96f:
		case 0xe97f:
		case 0xe986:
		case 0xe994:
		case 0xe99d:
		case 0xe9a4:
		case 0xe9ad:
		case 0xe9f5:
		case 0xeb26:
		case 0xeb35:
		case 0xeb75:
		case 0xeb77:
		case 0xeb7d:
		case 0xebad:
		case 0xebbd:
		case 0xebc7:
		case 0xebff:
		case 0xed2d:
		case 0xed3e:
		case 0xed56:
		case 0xf1af:
		case 0xf1b4:
		case 0xf1b6:
		case 0xf1bf:
		case 0xf1c5:
		case 0xf1dc:
		case 0xf1e4:
		case 0xf1ec:
		case 0xf306:
		case 0xf317:
		case 0xf31e:
		case 0xf34d:
		case 0xf35c:
		case 0xf36c:
		case 0xf37d:
		case 0xf387:
		case 0xf38c:
		case 0xf38e:
		case 0xf395:
		case 0xf39d:
		case 0xf39f:
		case 0xf3ae:
		case 0xf3b4:
		case 0xf3c6:
		case 0xf507:
		case 0xf50f:
		case 0xf51d:
		case 0xf51f:
		case 0xf524:
		case 0xf52c:
		case 0xf52e:
		case 0xf535:
		case 0xf537:
		case 0xf54c:
		case 0xf564:
		case 0xf576:
		case 0xf57f:
		case 0xf5c7:
		case 0xf5de:
		case 0xf5ee:
		case 0xf707:
		case 0xf70f:
		case 0xf714:
		case 0xf736:
		case 0xf73d:
		case 0xf73f:
		case 0xf745:
		case 0xf747:
		case 0xf74d:
		case 0xf74f:
		case 0xf756:
		case 0xf767:
		case 0xf76c:
		case 0xf775:
		case 0xf77d:
		case 0xf796:
		case 0xf79e:
		case 0xf7ad:
		case 0xf7b4:
		case 0xf7c7:
		case 0xf7dc:
		case 0xf7f7:
		case 0xf7fd:
		case 0xff1c:
		case 0xff1e:
		case 0xff3f:
			return (RAM[A] & 0x13) | ((RAM[A] & 0x80) >> 5) | ((RAM[A] & 0x64) << 1)
					| ((RAM[A] & 0x08) << 2);
			break;

#ifdef ONELEVEL
		case 0xdb47:
		case 0xdb48:
		case 0xdb49:
			return 0xea;
			break;
#endif

		default:
			return RAM[A];
			break;
	}
}

static unsigned btimea_decode(int A)
{
	/* the encryption is a simple bit rotation: 76543210 -> 65342710      */
	/* it is not known, however, when it must be applied. Only opcodes at */
	/* addresses with this bit pattern:                                   */
	/* xxxx xxx1 xxxx x1xx                                                */
	/* _can_ (but not necessarily are) be encrypted.                      */

	switch (A)
	{
		case 0xc12e:
		case 0xc13c:
		case 0xc14f:
		case 0xc157:
		case 0xc15d:
		case 0xc185:
		case 0xc18d:
		case 0xc18f:
		case 0xc1ae:
		case 0xc1bd:
		case 0xc1c5:
		case 0xc1de:
		case 0xc1fc:
		case 0xc317:
		case 0xc31d:
		case 0xc325:
		case 0xc32c:
		case 0xc32f:
		case 0xc335:
		case 0xc33e:
		case 0xc347:
		case 0xc34c:
		case 0xc356:
		case 0xc366:
		case 0xc374:
		case 0xc37c:
		case 0xc387:
		case 0xc38c:
		case 0xc50c:
		case 0xc56c:
		case 0xc56e:
		case 0xc576:
		case 0xc57d:
		case 0xc586:
		case 0xc58c:
		case 0xc58f:
		case 0xc595:
		case 0xc597:
		case 0xc59d:
		case 0xc5a7:
		case 0xc5ac:
		case 0xc5b6:
		case 0xc5c4:
		case 0xc5c6:
		case 0xc5d7:
		case 0xc70d:
		case 0xc71e:
		case 0xc72d:
		case 0xc72f:
		case 0xc736:
		case 0xc74d:
		case 0xc74f:
		case 0xc75c:
		case 0xc764:
		case 0xc767:
		case 0xc78c:
		case 0xc795:
		case 0xc79f:
		case 0xc7c6:
		case 0xc7d7:
		case 0xc7dd:
		case 0xc7ec:
		case 0xc7f5:
		case 0xc91d:
		case 0xc927:
		case 0xc94e:
		case 0xc965:
		case 0xc976:
		case 0xc97f:
		case 0xc985:
		case 0xc98c:
		case 0xc997:
		case 0xc99e:
		case 0xc9bc:
		case 0xc9bf:
		case 0xc9cc:
		case 0xc9e5:
		case 0xcb06:
		case 0xcb0d:
		case 0xcb1e:
		case 0xcb34:
		case 0xcb3e:
		case 0xcb47:
		case 0xcb55:
		case 0xcb57:
		case 0xcb5d:
		case 0xcb6c:
		case 0xcb74:
		case 0xcb7d:
		case 0xcb7f:
		case 0xcb8d:
		case 0xcba4:
		case 0xcbb4:
		case 0xcbb6:
		case 0xcbce:
		case 0xcbee:
		case 0xcbf7:
		case 0xcbfc:
		case 0xcd36:
		case 0xcd3e:
		case 0xcd44:
		case 0xcd54:
		case 0xcd5d:
		case 0xcd76:
		case 0xcd87:
		case 0xcd8d:
		case 0xcd8f:
		case 0xcd95:
		case 0xcd9d:
		case 0xcda7:
		case 0xcdad:
		case 0xcdb6:
		case 0xcf3d:
		case 0xcf46:
		case 0xcf57:
		case 0xcf8d:
		case 0xcf9c:
		case 0xcfd7:
		case 0xcfef:
		case 0xcff7:
		case 0xd106:
		case 0xd11e:
		case 0xd125:
		case 0xd12c:
		case 0xd13f:
		case 0xd16e:
		case 0xd177:
		case 0xd1ad:
		case 0xd1b6:
		case 0xd1cd:
		case 0xd1d5:
		case 0xd1e5:
		case 0xd1e7:
		case 0xd1fe:
		case 0xd30c:
		case 0xd31f:
		case 0xd326:
		case 0xd32c:
		case 0xd32e:
		case 0xd336:
		case 0xd347:
		case 0xd3d5:
		case 0xd3de:
		case 0xd3e5:
		case 0xd3f4:
		case 0xd3fc:
		case 0xd51f:
		case 0xd535:
		case 0xd537:
		case 0xd53c:
		case 0xd53e:
		case 0xd56f:
		case 0xd584:
		case 0xd594:
		case 0xd5b7:
		case 0xd5be:
		case 0xd796:
		case 0xd7a5:
		case 0xd7be:
		case 0xd7c4:
		case 0xd7cf:
		case 0xd7d5:
		case 0xd7f5:
		case 0xd906:
		case 0xd917:
		case 0xd93f:
		case 0xd94e:
		case 0xd954:
		case 0xd95e:
		case 0xd97f:
		case 0xd987:
		case 0xd994:
		case 0xd996:
		case 0xd99e:
		case 0xd9be:
		case 0xd9e4:
		case 0xd9ed:
		case 0xd9ef:
		case 0xdb35:
		case 0xdb3f:
		case 0xdb5e:
		case 0xdb65:
		case 0xdb67:
		case 0xdb6c:
		case 0xdb6e:
		case 0xdb74:
		case 0xdb7d:
		case 0xdb87:
		case 0xdb95:
		case 0xdbc4:
		case 0xdbdf:
		case 0xdbe6:
		case 0xdbef:
		case 0xdbf4:
		case 0xdd04:
		case 0xdd14:
		case 0xdd1c:
		case 0xdd1e:
		case 0xdd65:
		case 0xdd6e:
		case 0xdd86:
		case 0xdd95:
		case 0xdd9d:
		case 0xddd7:
		case 0xdddd:
		case 0xddec:
		case 0xdf34:
		case 0xdf4e:
		case 0xdf54:
		case 0xdf6e:
		case 0xdfbc:
		case 0xdfd6:
		case 0xe107:
		case 0xe116:
		case 0xe11e:
		case 0xe12e:
		case 0xe13d:
		case 0xe15c:
		case 0xe166:
		case 0xe16d:
		case 0xe174:
		case 0xe196:
		case 0xe1a4:
		case 0xe1ae:
		case 0xe1c6:
		case 0xe1ee:
		case 0xe1fd:
		case 0xe1ff:
		case 0xe31f:
		case 0xe334:
		case 0xe357:
		case 0xe36c:
		case 0xe38f:
		case 0xe3a4:
		case 0xe3c7:
		case 0xe3dc:
		case 0xe3ff:
		case 0xe516:
		case 0xe55d:
		case 0xe56e:
		case 0xe57f:
		case 0xe586:
		case 0xe5b4:
		case 0xe5be:
		case 0xe5ce:
		case 0xe5d7:
		case 0xe5dc:
		case 0xe5ff:
		case 0xe717:
		case 0xe726:
		case 0xe737:
		case 0xe73e:
		case 0xe746:
		case 0xe76f:
		case 0xe786:
		case 0xe7be:
		case 0xe7cd:
		case 0xe93c:
		case 0xe93f:
		case 0xe94c:
		case 0xe965:
		case 0xe98c:
		case 0xe98f:
		case 0xe99c:
		case 0xe9af:
		case 0xe9e7:
		case 0xe9ed:
		case 0xe9f7:
		case 0xe9fe:
		case 0xeb07:
		case 0xeb0c:
		case 0xeb2f:
		case 0xeb34:
		case 0xeb37:
		case 0xeb3e:
		case 0xeb44:
		case 0xeb4c:
		case 0xeb77:
		case 0xeb7e:
		case 0xeb87:
		case 0xeb8c:
		case 0xebb5:
		case 0xebd5:
		case 0xefff:
		case 0xf107:
		case 0xf10f:
		case 0xf115:
		case 0xf117:
		case 0xf146:
		case 0xf14c:
		case 0xf15d:
		case 0xf164:
		case 0xf194:
		case 0xf196:
		case 0xf1ae:
		case 0xf1b6:
		case 0xf1bc:
		case 0xf1c6:
		case 0xf1d4:
		case 0xf1dc:
		case 0xf1ed:
		case 0xf1fd:
		case 0xf305:
		case 0xf30f:
		case 0xf317:
		case 0xf324:
		case 0xf32f:
		case 0xf35e:
		case 0xf365:
		case 0xf36d:
		case 0xf36f:
		case 0xf376:
		case 0xf37c:
		case 0xf38d:
		case 0xf3a5:
		case 0xf3b7:
		case 0xf3cc:
		case 0xf504:
		case 0xf515:
		case 0xf51e:
		case 0xf52f:
		case 0xf535:
		case 0xf53f:
		case 0xf555:
		case 0xf564:
		case 0xf577:
		case 0xf57e:
		case 0xf584:
		case 0xf586:
		case 0xf58c:
		case 0xf58e:
		case 0xf597:
		case 0xf59c:
		case 0xf5ad:
		case 0xf5b6:
		case 0xf5be:
		case 0xf5d7:
		case 0xf5df:
		case 0xf5ec:
		case 0xf5ee:
		case 0xf5f5:
			return (RAM[A] & 0x13) | ((RAM[A] & 0x80) >> 5) | ((RAM[A] & 0x64) << 1)
					| ((RAM[A] & 0x08) << 2);
			break;
		default:
			return RAM[A];
			break;
	}
}



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0036],"\x00\x80\x02",3) == 0 &&
			memcmp(&RAM[0x0042],"\x50\x48\00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0036],1,6*6,f);
			RAM[0x0033] = RAM[0x0036];
			RAM[0x0034] = RAM[0x0037];
			RAM[0x0035] = RAM[0x0038];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x0036],1,6*6,f);
		fclose(f);
	}
}



struct GameDriver btime_driver =
{
	"Burger Time (Midway)",
	"btime",
	"KEVIN BRISLEY\nMIRKO BUFFONI\nNICOLA SALMORIA",
	&machine_driver,

	btime_rom,
	0, btime_decode,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,	/* numbers */
		0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* letters */
		0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24 },
	0x00, 0x01,
	8*13, 8*16, 0x00,

	hiload, hisave
};

struct GameDriver btimea_driver =
{
	"Burger Time (Data East)",
	"btimea",
	"KEVIN BRISLEY\nMIRKO BUFFONI\nNICOLA SALMORIA",
	&machine_driver,

	btimea_rom,
	0, btimea_decode,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,	/* numbers */
		0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* letters */
		0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24 },
	0x00, 0x01,
	8*13, 8*16, 0x00,

	hiload, hisave
};
