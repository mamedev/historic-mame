/***************************************************************************

Pengo memory map (preliminary)

0000-7fff ROM
8000-83ff Video RAM
8400-87ff Color RAM
8800-8fff RAM

memory mapped ports:

read:
9000      DSW2
9040      DSW1
9080      IN1
90c0      IN0

*
 * IN0 (all bits are inverted)
 * bit 7 : PUSH player 1
 * bit 6 : CREDIT
 * bit 5 : COIN B
 * bit 4 : COIN A
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : DOWN player 1
 * bit 0 : UP player 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : PUSH player 2 (TABLE only)
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : TEST SWITCH
 * bit 3 : RIGHT player 2 (TABLE only)
 * bit 2 : LEFT player 2 (TABLE only)
 * bit 1 : DOWN player 2 (TABLE only)
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 : DIP SWITCH 8\ difficulty level
 * bit 6 : DIP SWITCH 7/ 00 = Hardest  01 = Hard  10 = Medium  11 = Easy
 * bit 5 : DIP SWITCH 6  RACK TEST
 * bit 4 : DIP SWITCH 5\ nr of lives
 * bit 3 : DIP SWITCH 4/ 00 = 5  01 = 4  10 = 3  11 = 2
 * bit 2 : DIP SWITCH 3  TABLE or UPRIGHT cabinet select (0 = UPRIGHT)
 * bit 1 : DIP SWITCH 2  Attract mode sounds  1 = off  0 = on
 * bit 0 : DIP SWITCH 1  Bonus  0 = 30000  1 = 50000
 *
*
 * DSW2 (all bits are inverted)
 * bit 7 : DIP SWITCH 8  not used
 * bit 6 : DIP SWITCH 7  not used
 * bit 5 : DIP SWITCH 6  not used
 * bit 4 : DIP SWITCH 5  not used
 * bit 3 : DIP SWITCH 4  not used
 * bit 2 : DIP SWITCH 3  not used
 * bit 1 : DIP SWITCH 2  not used
 * bit 0 : DIP SWITCH 1  not used
 *

write:
8ff2-8ffd 6 pairs of two bytes:
          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
		  X flip (bit 1); the second byte the color
9005      sound voice 1 waveform (nibble)
9011-9013 sound voice 1 frequency (nibble)
9015      sound voice 1 volume (nibble)
900a      sound voice 2 waveform (nibble)
9016-9018 sound voice 2 frequency (nibble)
901a      sound voice 2 volume (nibble)
900f      sound voice 3 waveform (nibble)
901b-901d sound voice 3 frequency (nibble)
901f      sound voice 3 volume (nibble)
9022-902d Sprite coordinates, x/y pairs for 6 sprites
9040      interrupt enable
9041      sound enable
9042      palette bank selector
9043      flip screen
9044-9045 coin counters
9046      color lookup table bank selector
9047      character/sprite bank selector
9070      watchdog reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *pengo_soundregs;
extern void pengo_gfxbank_w(int offset,int data);
extern void pengo_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int pengo_vh_start(void);
extern void pengo_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void pengo_sound_enable_w(int offset,int data);
extern void pengo_sound_w(int offset,int data);
extern void pengo_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x8fff, MRA_RAM },	/* video and color RAM, scratchpad RAM, sprite codes */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x90c0, 0x90ff, input_port_0_r },	/* IN0 */
	{ 0x9080, 0x90bf, input_port_1_r },	/* IN1 */
	{ 0x9040, 0x907f, input_port_2_r },	/* DSW1 */
	{ 0x9000, 0x903f, input_port_3_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8800, 0x8fef, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0x9000, 0x901f, pengo_sound_w, &pengo_soundregs },
	{ 0x8ff0, 0x8fff, MWA_RAM, &spriteram},
	{ 0x9020, 0x902f, MWA_RAM, &spriteram_2 },
	{ 0x9040, 0x9040, interrupt_enable_w },
	{ 0x9070, 0x9070, MWA_NOP },
	{ 0x9041, 0x9041, pengo_sound_enable_w },
	{ 0x9047, 0x9047, pengo_gfxbank_w },
	{ 0x9042, 0x9046, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				0, 0, OSD_KEY_3, OSD_KEY_CONTROL },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				0, 0, 0, OSD_JOY_FIRE }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_F2, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xb0,
		{ 0, 0, 0, 0, 0, OSD_KEY_F1, 0, 0 },
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
	{ 2, 0x18, "LIVES", { "5", "4", "3", "2" }, 1 },
	{ 2, 0x01, "BONUS", { "30000", "50000" } },
	{ 2, 0xc0, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 2, 0x02, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
			7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	64*8	/* every sprite takes 64 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 32 },	/* first bank */
	{ 1, 0x1000, &spritelayout,    0, 32 },
	{ 1, 0x2000, &charlayout,   4*32, 32 },	/* second bank */
	{ 1, 0x3000, &spritelayout, 4*32, 32 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0xF6,0x07,0x38,0xC9,0xF8,0x3F,0xEF,0x6F,0x16,0x2F,0x7F,0xF0,0x36,0xDB,0xC6,
	0x00,0xF6,0xD8,0xF0,0xF8,0x16,0x07,0x2F,0x36,0x3F,0x7F,0x28,0x32,0x38,0xEF,0xC6,
	/* color lookup table */
	0x00,0x00,0x00,0x00,0x00,0x05,0x03,0x01,0x00,0x05,0x02,0x01,0x00,0x05,0x06,0x01,
	0x00,0x05,0x07,0x01,0x00,0x05,0x0A,0x01,0x00,0x05,0x0B,0x01,0x00,0x05,0x0C,0x01,
	0x00,0x05,0x0D,0x01,0x00,0x05,0x04,0x01,0x00,0x03,0x06,0x01,0x00,0x03,0x02,0x01,
	0x00,0x03,0x07,0x01,0x00,0x03,0x05,0x01,0x00,0x02,0x03,0x01,0x00,0x00,0x00,0x00,
	0x00,0x08,0x03,0x01,0x00,0x09,0x02,0x05,0x00,0x08,0x05,0x0D,0x04,0x04,0x04,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x02,0x00,0x03,0x03,0x03,
	0x00,0x06,0x06,0x06,0x00,0x07,0x07,0x07,0x00,0x0A,0x0A,0x0A,0x00,0x0B,0x0B,0x0B,
	0x00,0x01,0x01,0x01,0x00,0x05,0x05,0x05,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x00,0x00,0x00,0x00,0x03,0x07,0x0D,0x00,0x0C,0x0F,0x0B,0x00,0x0C,0x0E,0x0B,
	0x00,0x0C,0x06,0x0B,0x00,0x0C,0x07,0x0B,0x00,0x0C,0x03,0x0B,0x00,0x0C,0x08,0x0B,
	0x00,0x0C,0x0D,0x0B,0x00,0x0C,0x04,0x0B,0x00,0x0C,0x09,0x0B,0x00,0x0C,0x05,0x0B,
	0x00,0x0C,0x02,0x0B,0x00,0x0C,0x0B,0x02,0x00,0x08,0x0C,0x02,0x00,0x08,0x0F,0x02,
	0x00,0x03,0x02,0x01,0x00,0x02,0x0F,0x03,0x00,0x0F,0x0E,0x02,0x00,0x0E,0x07,0x0F,
	0x00,0x07,0x06,0x0E,0x00,0x06,0x05,0x07,0x00,0x05,0x00,0x06,0x00,0x00,0x0B,0x05,
	0x00,0x0B,0x0C,0x00,0x00,0x0C,0x0D,0x0B,0x00,0x0D,0x08,0x0C,0x00,0x08,0x09,0x0D,
	0x00,0x09,0x0A,0x08,0x00,0x0A,0x01,0x09,0x00,0x01,0x04,0x0A,0x00,0x04,0x03,0x01
};



/* waveforms for the audio hardware */
static unsigned char samples[8*32] =
{
	0x00,0x00,0x00,0x00,0x00,0x77,0x77,0x00,0x00,0x88,0x88,0x88,0x00,0x00,0x00,0x00,
	0x77,0x77,0x77,0x00,0x88,0x88,0x00,0x00,0x00,0x00,0x77,0x77,0x00,0x00,0x88,0x88,

	0xff,0x11,0x22,0x33,0xff,0x55,0x55,0xff,0x66,0xff,0x55,0x55,0xff,0x33,0x22,0x11,
	0xff,0xdd,0xff,0xbb,0xff,0x99,0xff,0x88,0xff,0x88,0xff,0x99,0xff,0xbb,0xff,0xdd,

	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,

	0x00,0x00,0x00,0x88,0x00,0x00,0x77,0x77,0x88,0x88,0x00,0x00,0x00,0x77,0x77,0x77,
	0x88,0x00,0x00,0x88,0x77,0x77,0x00,0x00,0x00,0x00,0x77,0x00,0x88,0x88,0x88,0x00,

	0xff,0x22,0x44,0x55,0x66,0x55,0x44,0x22,0xff,0xcc,0xaa,0x99,0x88,0x99,0xaa,0xcc,
	0xff,0x33,0x55,0x66,0x55,0x33,0xff,0xbb,0x99,0x88,0x99,0xbb,0xff,0x66,0xff,0x88,

	0xff,0x66,0x44,0x11,0x44,0x66,0x22,0xff,0x44,0x77,0x55,0x00,0x22,0x33,0xff,0xaa,
	0x00,0x55,0x11,0xcc,0xdd,0xff,0xaa,0x88,0xbb,0x00,0xdd,0x99,0xbb,0xee,0xbb,0x99,

	0xff,0x00,0x22,0x44,0x66,0x55,0x44,0x44,0x33,0x22,0x00,0xff,0xdd,0xee,0xff,0x00,
	0x00,0x11,0x22,0x33,0x11,0x00,0xee,0xdd,0xcc,0xcc,0xbb,0xaa,0xcc,0xee,0x00,0x11,

	0x22,0x44,0x44,0x22,0xff,0xff,0x00,0x33,0x55,0x66,0x55,0x22,0xee,0xdd,0xdd,0xff,
	0x11,0x11,0x00,0xcc,0x99,0x88,0x99,0xbb,0xee,0xff,0xff,0xcc,0xaa,0xaa,0xcc,0xff
};



const struct MachineDriver pengo_driver =
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
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	32,4*64,
	color_prom,pengo_vh_convert_color_prom,0,0,
	'0','A',
	0x01,0x18,
	8*11,8*20,0x16,
	0,
	pengo_vh_start,
	generic_vh_stop,
	pengo_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	0,
	0,
	pengo_sh_update
};
