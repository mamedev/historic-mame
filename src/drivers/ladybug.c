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
#include "vidhrdw/generic.h"



extern int ladybug_IN0_r(int offset);
extern int ladybug_IN1_r(int offset);
extern int ladybug_interrupt(void);

extern void ladybug_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void ladybug_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void ladybug_sound1_w(int offset,int data);
extern void ladybug_sound2_w(int offset,int data);
extern int ladybug_sh_start(void);
extern void ladybug_sh_stop(void);
extern void ladybug_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x9001, 0x9001, ladybug_IN1_r },	/* IN1 */
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },	/* video and color RAM */
	{ 0x9000, 0x9000, ladybug_IN0_r },	/* IN0 */
	{ 0x9002, 0x9002, input_port_2_r },	/* DSW1 */
	{ 0x9003, 0x9003, input_port_3_r },	/* DSW2 */
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
				0, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_RIGHT, OSD_JOY_UP,
				0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
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



static unsigned char color_prom[] =
{
	/* palette */
	0xF5,0x90,0x41,0x54,0x94,0x11,0x80,0x65,0x05,0xD4,0x01,0x00,0xB1,0xA0,0x00,0xF5,
	0x04,0xB1,0x00,0x15,0x11,0x25,0x90,0xD0,0xA0,0x90,0x15,0x84,0xB5,0x04,0x04,0x04,
	/* sprite color lookup table */
	0x00,0x59,0x33,0xB8,0x00,0xD4,0xA3,0x8D,0x00,0x2C,0x63,0xDD,0x00,0x22,0x38,0x1D,
	0x00,0x93,0x3A,0xDD,0x00,0xE2,0x38,0xDD,0x00,0x82,0x3A,0xD8,0x00,0x22,0x68,0x1D
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
	ROM_LOAD( "lb1.cpu",  0x0000, 0x1000 )
	ROM_LOAD( "lb2.cpu",  0x1000, 0x1000 )
	ROM_LOAD( "lb3.cpu",  0x2000, 0x1000 )
	ROM_LOAD( "lb4.cpu",  0x3000, 0x1000 )
	ROM_LOAD( "lb5.cpu",  0x4000, 0x1000 )
	ROM_LOAD( "lb6.cpu",  0x5000, 0x1000 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lb9.vid",  0x0000, 0x1000 )
	ROM_LOAD( "lb10.vid", 0x1000, 0x1000 )
	ROM_LOAD( "lb8.cpu",  0x2000, 0x1000 )
	ROM_LOAD( "lb7.cpu",  0x3000, 0x1000 )
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

	color_prom, 0, 0,
	8*13, 8*30,

	hiload, hisave
};
