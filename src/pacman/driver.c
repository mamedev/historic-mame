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

int pacman_IN0_r(int address,int offset);
int pacman_IN1_r(int address,int offset);
int pacman_DSW1_r(int address,int offset);
void pacman_interrupt_enable_w(int address,int offset,int data);
int pacman_init_machine(const char *gamename);
int pacman_interrupt(void);
void pacman_out(byte Port,byte Value);

int pengo_videoram_r(int address,int offset);
int pengo_colorram_r(int address,int offset);
void pengo_videoram_w(int address,int offset,int data);
void pengo_colorram_w(int address,int offset,int data);
void pengo_spritecode_w(int address,int offset,int data);
void pengo_spritepos_w(int address,int offset,int data);
int pengo_vh_start(void);
void pengo_vh_stop(void);
void pengo_vh_screenrefresh(void);

void pengo_sound_enable_w(int address,int offset,int data);
void pengo_sound_w(int address,int offset,int data);
void pengo_sh_update(void);



static struct MemoryReadAddress pacman_readmem[] =
{
	{ 0x4c00, 0x4fff, ram_r },
	{ 0x4000, 0x43ff, pengo_videoram_r },
	{ 0x4400, 0x47ff, pengo_colorram_r },
	{ 0x0000, 0x3fff, rom_r },
	{ 0x5000, 0x503f, pacman_IN0_r },
	{ 0x5040, 0x507f, pacman_IN1_r },
	{ 0x5080, 0x50bf, pacman_DSW1_r },
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress mspacman_readmem[] =
{
	{ 0x4c00, 0x4fff, ram_r },
	{ 0x4000, 0x43ff, pengo_videoram_r },
	{ 0x4400, 0x47ff, pengo_colorram_r },
	{ 0x0000, 0x3fff, rom_r },
	{ 0x8000, 0x9fff, rom_r },
	{ 0x5000, 0x503f, pacman_IN0_r },
	{ 0x5040, 0x507f, pacman_IN1_r },
	{ 0x5080, 0x50bf, pacman_DSW1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pacman_writemem[] =
{
	{ 0x4c00, 0x4fff, ram_w },					/* note that the sprite codes */
	{ 0x4ff0, 0x4fff, pengo_spritecode_w },	/* overlap standard memory. */
	{ 0x4000, 0x43ff, pengo_videoram_w },
	{ 0x4400, 0x47ff, pengo_colorram_w },
	{ 0x5040, 0x505f, pengo_sound_w },
	{ 0x5060, 0x506f, pengo_spritepos_w },
	{ 0xc000, 0xc3ff, pengo_videoram_w },	/* mirror address for video ram, */
	{ 0xc400, 0xc7ef, pengo_colorram_w },	/* used to display HIGH SCORE and CREDITS */
	{ 0x5000, 0x5000, pacman_interrupt_enable_w },
	{ 0x50c0, 0x50c0, 0 },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5007, 0 },
	{ 0x0000, 0x3fff, rom_w },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress mspacman_writemem[] =
{
	{ 0x4c00, 0x4fff, ram_w },					/* note that the sprite codes */
	{ 0x4ff0, 0x4fff, pengo_spritecode_w },	/* overlap standard memory. */
	{ 0x4000, 0x43ff, pengo_videoram_w },
	{ 0x4400, 0x47ff, pengo_colorram_w },
	{ 0x5040, 0x505f, pengo_sound_w },
	{ 0x5060, 0x506f, pengo_spritepos_w },
	{ 0xc000, 0xc3ff, pengo_videoram_w },	/* mirror address for video ram, */
	{ 0xc400, 0xc7ef, pengo_colorram_w },	/* used to display HIGH SCORE and CREDITS */
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x50c0, 0x50c0, 0 },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5007, 0 },
	{ 0x0000, 0x3fff, rom_w },
	{ 0x8000, 0x9fff, rom_w },
	{ -1 }	/* end of table */
};



static struct DSW pacdsw[] =
{
	{ 0, 0x0c, "LIVES", { "1", "2", "3", "5" } },
	{ 0, 0x30, "BONUS", { "10000", "15000", "20000", "NONE" } },
	{ 0, 0x40, "DIFFICULTY", { "HARD", "NORMAL" }, 1 },
	{ 0, 0x80, "GHOST NAMES", { "ALTERNATE", "NORMAL" }, 1 },
	{ -1 }
};
static struct DSW mspacdsw[] =
{
	{ 0, 0x0c, "LIVES", { "1", "2", "3", "5" } },
	{ 0, 0x30, "BONUS", { "10000", "15000", "20000", "NONE" } },
	{ 0, 0x40, "DIFFICULTY", { "HARD", "NORMAL" }, 1 },
	{ -1 }
};



static struct RomModule genericrom[] =
{
	/* code */
	{ "%s.6e", 0x00000, 0x1000 },
	{ "%s.6f", 0x01000, 0x1000 },
	{ "%s.6h", 0x02000, 0x1000 },
	{ "%s.6j", 0x03000, 0x1000 },
	/* gfx */
	{ "%s.5e", 0x10000, 0x1000 },
	{ "%s.5f", 0x11000, 0x1000 },
	{ 0 }	/* end of table */
};
static struct RomModule pacmodrom[] =
{
	{ "6e.mod",    0x0000, 0x1000 },
	{ "pacman.6f", 0x1000, 0x1000 },
	{ "6h.mod",    0x2000, 0x1000 },
	{ "pacman.6j", 0x3000, 0x1000 },
	{ 0 }	/* end of table */
};
static struct RomModule mspacrom[] =
{
	/* code */
	{ "boot1", 0x00000, 0x1000 },
	{ "boot2", 0x01000, 0x1000 },
	{ "boot3", 0x02000, 0x1000 },
	{ "boot4", 0x03000, 0x1000 },
	{ "boot5", 0x08000, 0x1000 },
	{ "boot6", 0x09000, 0x1000 },
	/* gfx */
	{ "5e",    0x10000, 0x1000 },
	{ "5f",    0x11000, 0x1000 },
	{ 0 }	/* end of table */
};
static struct RomModule piranharom[] =
{
	{ "pr1.cpu", 0x0000, 0x1000 },
	{ "pr2.cpu", 0x1000, 0x1000 },
	{ "pr3.cpu", 0x2000, 0x1000 },
	{ "pr4.cpu", 0x3000, 0x1000 },
	{ 0 }	/* end of table */
};



static struct RomModule piranhagfx[] =
{
	{ "pr5.cpu", 0x0000, 0x0800 },
	{ "pr7.cpu", 0x0800, 0x0800 },
	{ "pr6.cpu", 0x1000, 0x0800 },
	{ "pr8.cpu", 0x1800, 0x0800 },
	{ 0 }	/* end of table */
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	4,	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	4,	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
			7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	64*8	/* every sprite takes 64 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,   0, 31 },
	{ 0x11000, &spritelayout, 0, 31 },
	{ -1 } /* end of array */
};



static unsigned char pacpalette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0x00,0x00,	/* UNUSED */
	0x00,0xff,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x00,0x00,0x00,	/* UNUSED */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0x00,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xff,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* WHITE */
};

enum {BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,
		UNUSED2,YELLOW,UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,WHITE};
#define UNUSED BLACK

static unsigned char paccolortable[] =
{
	BLACK,BLACK,BLACK,BLACK,		/* 0x00 Background in intermissions */
	BLACK,WHITE,BLUE,RED,			/* 0x01 - SHADOW  "BLINKY" */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x02 Unused */
	BLACK,WHITE,BLUE,PINK,			/* 0x03 - SPEEDY  "PINKY" */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x04 Unused */
	BLACK,WHITE,BLUE,CYAN,			/* 0x05 - BASHFUL  "INKY" */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x06 Unused */
	BLACK,WHITE,BLUE,DKORANGE,		/* 0x07 - POKEY  "CLYDE"; Ms Pac Man 4th maze */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x08 Unused */
	BLACK,BLUE,RED,YELLOW,			/* 0x09 Lives left; Bird; Pac Man */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x0a Unused */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x0b Unused */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x0c Unused */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x0d Unused */
	BLACK,WHITE,BLACK,LTORANGE,		/* 0x0e BONUS PAC-MAN for xx000 PTS */
	BLACK,RED,GREEN,WHITE,			/* 0x0f White text; Strawberry */
	BLACK,LTORANGE,BLACK,BLUE,		/* 0x10 Background; Maze walls & dots */
	BLACK,GREEN,BLUE,LTORANGE,		/* 0x11 Blue ghosts */
	BLACK,GREEN,WHITE,RED,			/* 0x12 White ghosts */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x13 Unused */
	BLACK,RED,BROWN,WHITE,			/* 0x14 Cherry; Apple; Ms Pac Man 3rd maze */
	BLACK,DKORANGE,GREEN,BROWN,		/* 0x15 Orange */
	BLACK,YELLOW,DKCYAN,WHITE,		/* 0x16 Bell; Key; Ms Pac Man 2nd maze */
	BLACK,DKGREEN,GREEN,WHITE,		/* 0x17 Grape; Ms Pac Man pear */
	BLACK,CYAN,PINK,YELLOW,			/* 0x18 Barn door; Points when eating ghost */
									/*      Ms Pac Man 5th maze */
	BLACK,WHITE,BLUE,BLACK,			/* 0x19 Ghost eyes */
	BLACK,LTORANGE,UNUSED,UNUSED,	/* 0x1a Dots around starting position */
	BLACK,LTORANGE,BLACK,BLUE,		/* 0x1b Tunnel */
	UNUSED,UNUSED,UNUSED,UNUSED,	/* 0x1c Unused */
	BLACK,WHITE,LTORANGE,RED,		/* 0x1d Ghost without sheet (2nd intermission) */
									/*      Ms Pac Man 1st maze */
	BLACK,WHITE,BLUE,LTORANGE,		/* 0x1e Ghost without sheet (3rd intermission) */
	BLACK,LTORANGE,BLACK,WHITE		/* 0x1f 10 pts, 50 pts; White maze walls when level complete */
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
	"pacman",
	genericrom,
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz. Is this correct for Pac Man? */
	60,
	pacman_readmem,
	pacman_writemem,
	pacdsw, { 0xe9 },
	pacman_init_machine,
	pacman_interrupt,
	pacman_out,

	/* video hardware */
	224,288,
	gfxdecodeinfo,
	pacpalette,sizeof(pacpalette)/3,
	paccolortable,sizeof(paccolortable)/4,
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
	0,
	0,
	pengo_sh_update
};



const struct MachineDriver mspacman_driver =
{
	"mspacman",
	mspacrom,
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz. Is this correct for Pac Man? */
	60,
	mspacman_readmem,
	mspacman_writemem,
	mspacdsw, { 0xe9 },
	pacman_init_machine,
	interrupt,
	0,

	/* video hardware */
	224,288,
	gfxdecodeinfo,
	pacpalette,sizeof(pacpalette)/3,
	paccolortable,sizeof(paccolortable)/4,
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
	0,
	0,
	pengo_sh_update
};
