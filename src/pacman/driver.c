/***************************************************************************

Pac Man memory map (preliminary)

0000-3fff ROM
4000-43ff Video RAM
4400-47ff Color RAM
4c00-4fff RAM
8000-9fff ROM (Ms Pac Man only)

memory mapped ports:

read:
5000      IN0
5040      IN1
5080      DSW1


*
 * IN0 (all bits are inverted)
 * bit 7 : CREDIT
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : RACK TEST
 * bit 3 : DOWN player 1
 * bit 2 : RIGHT player 1
 * bit 1 : LEFT player 1
 * bit 0 : UP player 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : COCKTAIL or UPRIGHT cabinet (1 = UPRIGHT)
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : TEST SWITCH (not Crush Roller)
 * bit 3 : DOWN player 2 (TABLE only)
 * bit 2 : RIGHT player 2 (TABLE only)
 * bit 1 : LEFT player 2 (TABLE only)
 * bit 0 : UP player 2 (TABLE ony)
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 :  (PacMan only) selects the names for the ghosts
 *          1 = Normal 0 = Alternate
 * bit 6 :  difficulty level
 *          1 = Normal  0 = Harder
 * bit 5 :\ bonus pac at xx000 pts
 * bit 4 :/ 00 = 10000  01 = 15000  10 = 20000  11 = none
 * bit 3 :\ nr of lives
 * bit 2 :/ 00 = 1  01 = 2  10 = 3  11 = 5
 * bit 1 :\ play mode
 * bit 0 :/ 00 = free play   01 = 1 coin 1 credit
 *          10 = 1 coin 2 credits   11 = 2 coins 1 credit
 *

write:
4ff2-4ffd 6 pairs of two bytes:
          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
		  X flip (bit 1); the second byte the color
5000      interrupt enable
5001      sound enable
5002      ????
5003      flip screen
5004      1 player start lamp
5005      2 players start lamp
5006      related to the credits. don't know what it was used for.
5007      coin counter (pulsing it would increase the counter on the machine)
5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
5045      sound voice 1 waveform (nibble)
5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
504a      sound voice 2 waveform (nibble)
504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
504f      sound voice 3 waveform (nibble)
5050-5054 sound voice 1 frequency (nibbles)
5055      sound voice 1 volume (nibble)
5056-5059 sound voice 2 frequency (nibbles)
505a      sound voice 2 volume (nibble)
505b-505e sound voice 3 frequency (nibbles)
505f      sound voice 3 volume (nibble)
5062-506d Sprite coordinates, x/y pairs for 6 sprites
50c0      Watchdog reset

I/O ports:
OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "driver.h"
#include "machine.h"
#include "common.h"


extern int pacman_init_machine(const char *gamename);
extern int pacman_interrupt(void);

extern unsigned char *pengo_videoram;
extern unsigned char *pengo_colorram;
extern unsigned char *pengo_spritecode;
extern unsigned char *pengo_spritepos;
extern unsigned char *pengo_soundregs;
extern void pengo_videoram_w(int offset,int data);
extern void pengo_colorram_w(int offset,int data);
extern void pacman_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int pengo_vh_start(void);
extern void pengo_vh_stop(void);
extern void pengo_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void pengo_sound_enable_w(int offset,int data);
extern void pengo_sound_w(int offset,int data);
extern void pengo_sh_update(void);



static struct MemoryReadAddress pacman_readmem[] =
{
	{ 0x4c00, 0x4fff, MRA_RAM },	/* includeing sprite codes at 4ff0-4fff */
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress mspacman_readmem[] =
{
	{ 0x4c00, 0x4fff, MRA_RAM },	/* includeing sprite codes at 4ff0-4fff */
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_ROM },
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pacman_writemem[] =
{
	{ 0x4c00, 0x4fef, MWA_RAM },
	{ 0x4000, 0x43ff, pengo_videoram_w, &pengo_videoram },
	{ 0x4400, 0x47ff, pengo_colorram_w, &pengo_colorram },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x4ff0, 0x4fff, MWA_RAM, &pengo_spritecode},
	{ 0x5060, 0x506f, MWA_RAM, &pengo_spritepos },
	{ 0xc000, 0xc3ff, pengo_videoram_w },	/* mirror address for video ram, */
	{ 0xc400, 0xc7ef, pengo_colorram_w },	/* used to display HIGH SCORE and CREDITS */
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x50c0, 0x50c0, MWA_NOP },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5007, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress mspacman_writemem[] =
{
	{ 0x4c00, 0x4fef, MWA_RAM },
	{ 0x4000, 0x43ff, pengo_videoram_w, &pengo_videoram },
	{ 0x4400, 0x47ff, pengo_colorram_w, &pengo_colorram },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x4ff0, 0x4fff, MWA_RAM, &pengo_spritecode},
	{ 0x5060, 0x506f, MWA_RAM, &pengo_spritepos },
	{ 0xc000, 0xc3ff, pengo_videoram_w },	/* mirror address for video ram, */
	{ 0xc400, 0xc7ef, pengo_colorram_w },	/* used to display HIGH SCORE and CREDITS */
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x50c0, 0x50c0, MWA_NOP },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5007, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN,
				OSD_KEY_F1, 0, 0, OSD_KEY_3 },
		{ OSD_JOY_UP, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN,
				0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_F2, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xe9,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW pacdsw[] =
{
	{ 2, 0x0c, "LIVES", { "1", "2", "3", "5" } },
	{ 2, 0x30, "BONUS", { "10000", "15000", "20000", "NONE" } },
	{ 2, 0x40, "DIFFICULTY", { "HARD", "NORMAL" }, 1 },
	{ 2, 0x80, "GHOST NAMES", { "ALTERNATE", "NORMAL" }, 1 },
	{ -1 }
};
static struct DSW mspacdsw[] =
{
	{ 2, 0x0c, "LIVES", { "1", "2", "3", "5" } },
	{ 2, 0x30, "BONUS", { "10000", "15000", "20000", "NONE" } },
	{ 2, 0x40, "DIFFICULTY", { "HARD", "NORMAL" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4},	/* the two bitplanes for 4 pixels are packed into one byte */
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
	{ 0x10000, &charlayout,   0, 32 },
	{ 0x11000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0x07,0x66,0xEF,0x00,0xF8,0xEA,0x6F,0x00,0x3F,0x00,0xC9,0x38,0xAA,0xAF,0xF6,
	/* color lookup table */
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x01,0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x03,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x05,0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x07,
	0x00,0x00,0x00,0x00,0x00,0x0B,0x01,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0E,0x00,0x01,0x0C,0x0F,
	0x00,0x0E,0x00,0x0B,0x00,0x0C,0x0B,0x0E,0x00,0x0C,0x0F,0x01,0x00,0x00,0x00,0x00,
	0x00,0x01,0x02,0x0F,0x00,0x07,0x0C,0x02,0x00,0x09,0x06,0x0F,0x00,0x0D,0x0C,0x0F,
	0x00,0x05,0x03,0x09,0x00,0x0F,0x0B,0x00,0x00,0x0E,0x00,0x0B,0x00,0x0E,0x00,0x0B,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0E,0x01,0x00,0x0F,0x0B,0x0E,0x00,0x0E,0x00,0x0F
};



/* waveforms for the audio hardware */
static unsigned char samples[8*32] =
{
	0xff,0x11,0x22,0x33,0x44,0x55,0x55,0x66,0x66,0x66,0x55,0x55,0x44,0x33,0x22,0x11,
	0xff,0xdd,0xcc,0xbb,0xaa,0x99,0x99,0x88,0x88,0x88,0x99,0x99,0xaa,0xbb,0xcc,0xdd,

	0xff,0x44,0x66,0x66,0x55,0x33,0x11,0x22,0x33,0x33,0x22,0x11,0xee,0xcc,0xbb,0xdd,
	0xff,0x11,0x33,0x22,0x00,0xdd,0xcc,0xbb,0xbb,0xcc,0xdd,0xbb,0x99,0x88,0x88,0xaa,

	0xff,0x22,0x44,0x55,0x66,0x55,0x44,0x22,0xff,0xcc,0xaa,0x99,0x88,0x99,0xaa,0xcc,
	0xff,0x33,0x55,0x66,0x55,0x33,0xff,0xbb,0x99,0x88,0x99,0xbb,0xff,0x66,0xff,0x88,

	0xff,0x55,0x33,0x00,0x33,0x55,0x11,0xee,0x33,0x66,0x44,0xff,0x11,0x22,0xee,0xaa,
	0xff,0x44,0x00,0xcc,0xdd,0xff,0xaa,0x88,0xbb,0x00,0xdd,0x99,0xbb,0xee,0xbb,0x99,

	0x88,0x00,0x77,0xff,0x99,0x00,0x66,0xff,0xaa,0x00,0x55,0xff,0xbb,0x00,0x44,0xff,
	0xcc,0x00,0x33,0xff,0xdd,0x00,0x22,0xff,0xee,0x00,0x11,0xff,0xff,0x00,0x00,0xff,

	0xff,0x00,0xee,0x11,0xdd,0x22,0xcc,0x33,0xbb,0x44,0xaa,0x55,0x99,0x66,0x88,0x77,
	0x88,0x77,0x99,0x66,0xaa,0x55,0xbb,0x44,0xcc,0x33,0xdd,0x22,0xee,0x11,0xff,0x00,

	0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
	0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00,0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,

	0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
	0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77
};



const struct MachineDriver pacman_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz. Is this correct for Pac Man? */
	60,
	pacman_readmem,pacman_writemem,0,writeport,
	input_ports,pacdsw,
	pacman_init_machine,
	pacman_interrupt,

	/* video hardware */
	224,288,
	gfxdecodeinfo,
	16,4*32,
	color_prom,pacman_vh_convert_color_prom,0,0,
	'0','A',
	0x0f,0x09,
	8*11,8*20,0x01,
	0,
	pengo_vh_start,
	pengo_vh_stop,
	pengo_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	0,
	0,
	pengo_sh_update
};



const struct MachineDriver mspacman_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz. Is this correct for Pac Man? */
	60,
	mspacman_readmem,mspacman_writemem,0,0,
	input_ports,mspacdsw,
	pacman_init_machine,
	pacman_interrupt,

	/* video hardware */
	224,288,
	gfxdecodeinfo,
	16,4*32,
	color_prom,pacman_vh_convert_color_prom,0,0,
	'0','A',
	0x0f,0x09,
	8*11,8*20,0x01,
	0,
	pengo_vh_start,
	pengo_vh_stop,
	pengo_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	0,
	0,
	pengo_sh_update
};
