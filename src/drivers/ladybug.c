/***************************************************************************

Lady Bug memory map (preliminary)

0000-5fff ROM
6000-6fff RAM
d000-d3ff video RAM
          d000-d007/d020-d027/d040-d047/d060-d067 contain the column scroll
          registers (not used by Lady Bug)
d400-d7ff color RAM (4 bits wide)

memory mapped ports:

read:
9000      IN0
9001      IN1
9002      DSW1
9003      DSW2
e000      IN2
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
 * IN2 (all bits are inverted)
 * bit 7 :
 * bit 6 :
 * bit 5 :
 * bit 4 : BOMB player 2 (TABLE only)
 * bit 3 :
 * bit 2 :
 * bit 1 :
 * bit 0 : BOMB player 1
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
#include "vidhrdw/generic.h"



int ladybug_IN0_r(int offset);
int ladybug_IN1_r(int offset);
int ladybug_interrupt(void);

void ladybug_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void ladybug_vh_screenrefresh(struct osd_bitmap *bitmap);

void ladybug_sound1_w(int offset,int data);
void ladybug_sound2_w(int offset,int data);
int ladybug_sh_start(void);
void ladybug_sh_stop(void);
void ladybug_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x9001, 0x9001, input_port_1_r },	/* IN1 */
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },	/* video and color RAM */
//	{ 0x9000, 0x9000, ladybug_IN0_r },	/* IN0 */
	{ 0x9000, 0x9000, input_port_0_r },	/* IN0 */
	{ 0x9002, 0x9002, input_port_2_r },	/* DSW1 */
	{ 0x9003, 0x9003, input_port_3_r },	/* DSW2 */
	{ 0xe000, 0xe000, input_port_4_r },	/* IN2 */
	{ 0x8000, 0x8fff, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0x7000, 0x73ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb000, 0xbfff, ladybug_sound1_w },
	{ 0xc000, 0xcfff, ladybug_sound2_w },
	{ 0xa000, 0xafff, MWA_NOP },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_RIGHT, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_RIGHT, OSD_JOY_UP,
				OSD_JOY_FIRE1, 0, 0, 0 }
	},
	{	/* IN1 */
		0x7f,
		{ 0, 0, 0, 0, 0, 0, IPB_VBLANK, IPB_VBLANK },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xdf,
		{ 0, 0, 0, OSD_KEY_F1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_ALT, 0, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_FIRE2, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 3, "MOVE UP" },
        { 0, 0, "MOVE LEFT"  },
        { 0, 2, "MOVE RIGHT" },
        { 0, 1, "MOVE DOWN" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x80, "LIVES", { "5", "3" }, 1 },
	{ 2, 0x03, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 2, 0x04, "INITIALS", { "3 LETTERS", "10 LETTERS" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 1, 0 },	/* the two bitplanes are packed in two consecutive bits */
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 8*16+14, 8*16+12, 8*16+10, 8*16+8, 8*16+6, 8*16+4, 8*16+2, 8*16+0,
			14, 12, 10, 8, 6, 4, 2, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0,  8 },
	{ 1, 0x2000, &spritelayout, 4*8, 16 },
	{ -1 } /* end of array */
};



static unsigned char ladybug_color_prom[] =
{
	/* palette */
	0xF5,0x90,0x41,0x54,0x94,0x11,0x80,0x65,0x05,0xD4,0x01,0x00,0xB1,0xA0,0x00,0xF5,
	0x04,0xB1,0x00,0x15,0x11,0x25,0x90,0xD0,0xA0,0x90,0x15,0x84,0xB5,0x04,0x04,0x04,
	/* sprite color lookup table */
	0x00,0x59,0x33,0xB8,0x00,0xD4,0xA3,0x8D,0x00,0x2C,0x63,0xDD,0x00,0x22,0x38,0x1D,
	0x00,0x93,0x3A,0xDD,0x00,0xE2,0x38,0xDD,0x00,0x82,0x3A,0xD8,0x00,0x22,0x68,0x1D
};

static unsigned char snapjack_color_prom[] =
{
	/* palette */
	0xF5,0x05,0x54,0xC1,0xC4,0x94,0x84,0x24,0xD0,0x90,0xA1,0x00,0x31,0x50,0x25,0xF5,
	0x90,0x31,0x05,0x25,0x05,0x94,0x30,0x41,0x05,0x94,0x61,0x30,0x94,0x50,0x05,0xA5,
	/* sprite color lookup table */
	0x00,0x9D,0x11,0xB8,0x00,0x79,0x62,0x18,0x00,0x9E,0x25,0xDA,0x00,0xD7,0xA3,0x79,
	0x00,0xDE,0x29,0x74,0x00,0xD4,0x75,0x9D,0x00,0xAD,0x86,0x97,0x00,0x5A,0x4C,0x17
};

static unsigned char cavenger_color_prom[] =
{
	/* palette */
	0xF5,0xC4,0xD0,0xB1,0xD4,0x90,0x45,0x44,0x00,0x54,0x91,0x94,0x25,0x21,0x65,0xF5,
	0x21,0x00,0x25,0xD0,0xB1,0x90,0xD4,0xD4,0x25,0xB1,0xC4,0x90,0x65,0xD4,0x00,0x00,
	/* sprite color lookup table */
	0x00,0x78,0xA3,0xB5,0x00,0x8C,0x79,0x64,0x00,0xC3,0xEE,0xDD,0x00,0x3C,0xA2,0x4A,
	0x00,0x87,0xBA,0xDE,0x00,0x2A,0xAE,0xBB,0x00,0x8C,0xC2,0xB7,0x00,0xAC,0xE2,0x1D
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			ladybug_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 4*8, 28*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	32,4*24,
	ladybug_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	ladybug_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	ladybug_sh_start,
	ladybug_sh_stop,
	ladybug_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ladybug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lb1.cpu", 0x0000, 0x1000, 0x00e5eaaf )
	ROM_LOAD( "lb2.cpu", 0x1000, 0x1000, 0x758e9c98 )
	ROM_LOAD( "lb3.cpu", 0x2000, 0x1000, 0x4295ccd7 )
	ROM_LOAD( "lb4.cpu", 0x3000, 0x1000, 0xad30c2b6 )
	ROM_LOAD( "lb5.cpu", 0x4000, 0x1000, 0xc4da41d6 )
	ROM_LOAD( "lb6.cpu", 0x5000, 0x1000, 0x18aaf1ec )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lb9.vid",  0x0000, 0x1000, 0x80bd96ef )
	ROM_LOAD( "lb10.vid", 0x1000, 0x1000, 0xec7c93c8 )
	ROM_LOAD( "lb8.cpu",  0x2000, 0x1000, 0x2d4d4821 )
	ROM_LOAD( "lb7.cpu",  0x3000, 0x1000, 0xf685434d )
ROM_END

ROM_START( snapjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sj2a.bin", 0x0000, 0x1000, 0x37ef057b )
	ROM_LOAD( "sj2b.bin", 0x1000, 0x1000, 0x5f6a17c6 )
	ROM_LOAD( "sj2c.bin", 0x2000, 0x1000, 0x3cf098fc )
	ROM_LOAD( "sj2d.bin", 0x3000, 0x1000, 0x06fa91f2 )
	ROM_LOAD( "sj2e.bin", 0x4000, 0x1000, 0x135d2527 )
	ROM_LOAD( "sj2f.bin", 0x5000, 0x1000, 0x734f0213 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sj2i.bin", 0x0000, 0x1000, 0xffa6a2ec )
	ROM_LOAD( "sj2j.bin", 0x1000, 0x1000, 0x2506c6f0 )
	ROM_LOAD( "sj2h.bin", 0x2000, 0x1000, 0xdd2fa07f )
	ROM_LOAD( "sj2g.bin", 0x3000, 0x1000, 0x888dec19 )
ROM_END

ROM_START( cavenger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1", 0x0000, 0x1000, 0x8851691b )
	ROM_LOAD( "2", 0x1000, 0x1000, 0xfc637e8b )
	ROM_LOAD( "3", 0x2000, 0x1000, 0x46fcdaba )
	ROM_LOAD( "4", 0x3000, 0x1000, 0xbc747536 )
	ROM_LOAD( "5", 0x4000, 0x1000, 0x25f39d9f )
	ROM_LOAD( "6", 0x5000, 0x1000, 0x38961b88 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9", 0x0000, 0x1000, 0x6522fd1a )
	ROM_LOAD( "0", 0x1000, 0x1000, 0x6132e3d8 )
	ROM_LOAD( "8", 0x2000, 0x1000, 0x366d7ec1 )
/*	ROM_LOAD( "7", 0x3000, 0x1000, 0x )	empty socket */
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6073],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0x608b],"\x01\x00\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6073],1,3*9,f);
			fread(&RAM[0xd380],1,13*9,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x6073],1,3*9,f);
		fwrite(&RAM[0xd380],1,13*9,f);
		fclose(f);
	}
}



struct GameDriver ladybug_driver =
{
	"Lady Bug",
	"ladybug",
	"NICOLA SALMORIA",
	&machine_driver,

	ladybug_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	ladybug_color_prom, 0, 0,
	8*13, 8*30,

	hiload, hisave
};

struct GameDriver snapjack_driver =
{
	"Snap Jack",
	"snapjack",
	"NICOLA SALMORIA",
	&machine_driver,

	snapjack_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	snapjack_color_prom, 0, 0,
	8*13, 8*30,

	0, 0
};

struct GameDriver cavenger_driver =
{
	"Cosmic Avenger",
	"cavenger",
	"NICOLA SALMORIA",
	&machine_driver,

	cavenger_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	cavenger_color_prom, 0, 0,
	8*13, 8*30,

	0, 0
};
