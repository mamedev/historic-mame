/***************************************************************************

-----------+---+-----------------+-------------------------
   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-------------------------
0000-3FFF  | R | D D D D D D D D | CPU 1 rom (16k)
0000-1FFF  | R | D D D D D D D D | CPU 2 rom (8k)
0000-0FFF  | R | D D D D D D D D | CPU 3 rom (4k)
-----------+---+-----------------+-------------------------
6800-680F  | W | - - - - D D D D | Audio control
6810-681F  | W | - - - - D D D D | Audio control
-----------+---+-----------------+-------------------------
6820       | W | - - - - - - - D | 0 = Reset IRQ1(latched)
6821       | W | - - - - - - - D | 0 = Reset IRQ2(latched)
6822       | W | - - - - - - - D | 0 = Reset NMI3(latched)
6823       | W | - - - - - - - D | 0 = Reset #2,#3 CPU
6825       | W | - - - - - - - D | custom 53 mode1
6826       | W | - - - - - - - D | custom 53 mode2
6827       | W | - - - - - - - D | custom 53 mode3
-----------+---+-----------------+-------------------------
6830       | W |                 | watchdog reset
-----------+---+-----------------+-------------------------
7000       |R/W| D D D D D D D D | custom 06 Data
7100       |R/W| D D D D D D D D | custom 06 Command
-----------+---+-----------------+-------------------------
8000-87FF  |R/W| D D D D D D D D | 2k playfeild RAM
-----------+---+-----------------+-------------------------
8B80-8BFF  |R/W| D D D D D D D D | 1k sprite RAM (PIC,COL)
9380-93FF  |R/W| D D D D D D D D | 1k sprite RAM (VPOS,HPOS)
9B80-9BFF  |R/W| D D D D D D D D | 1k sprite RAM (FLIP)
-----------+---+-----------------+-------------------------
A000       | W | - - - - - - - D | playfield select
A001       | W | - - - - - - - D | playfield select
A002       | W | - - - - - - - D | Alpha color select
A003       | W | - - - - - - - D | playfield enable
A004       | W | - - - - - - - D | playfield color select
A005       | W | - - - - - - - D | playfield color select
A007       | W | - - - - - - - D | flip video
-----------+---+-----------------+-------------------------
B800-B83F  | W | D D D D D D D D | write EAROM addr,  data
B800       | R | D D D D D D D D | read  EAROM data
B840       | W |         D D D D | write EAROM control
-----------+---+-----------------+-------------------------

Dig Dug memory map (preliminary)

CPU #1:
0000-3fff ROM
CPU #2:
0000-1fff ROM
CPU #3:
0000-0fff ROM
ALL CPUS:
8000-83ff Video RAM
8400-87ff Color RAM
8b80-8bff sprite code/color
9380-93ff sprite position
9b80-9bff sprite control
8800-9fff RAM

read:
6800-6807 dip switches (only bits 0 and 1 are used - bit 0 is DSW1, bit 1 is DSW2)
          dsw1:
            bit 6-7 lives
            bit 3-5 bonus
            bit 0-2 coins per play
		  dsw2: (bootleg version, the original version is slightly different)
		    bit 7 cocktail/upright (1 = upright)
            bit 6 ?
            bit 5 RACK TEST
            bit 4 pause (0 = paused, 1 = not paused)
            bit 3 ?
            bit 2 ?
            bit 0-1 difficulty
7000-     custom IO chip return values
7100      custom IO chip status ($10 = command executed)

write:
6805      sound voice 1 waveform (nibble)
6811-6813 sound voice 1 frequency (nibble)
6815      sound voice 1 volume (nibble)
680a      sound voice 2 waveform (nibble)
6816-6818 sound voice 2 frequency (nibble)
681a      sound voice 2 volume (nibble)
680f      sound voice 3 waveform (nibble)
681b-681d sound voice 3 frequency (nibble)
681f      sound voice 3 volume (nibble)
6820      cpu #1 irq acknowledge/enable
6821      cpu #2 irq acknowledge/enable
6822      cpu #3 nmi acknowledge/enable
6823      if 0, halt CPU #2 and #3
6830      Watchdog reset?
7000-     custom IO chip parameters
7100      custom IO chip command (see machine/galaga.c for more details)
a000-a002 starfield scroll direction/speed (only bit 0 is significant)
a003-a005 starfield blink?
a007      flip screen

Interrupts:
CPU #1 IRQ mode 1
       NMI is triggered by the custom IO chip to signal the CPU to read/write
	       parameters
CPU #2 IRQ mode 1
CPU #3 NMI (@120Hz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *digdug_sharedram;
extern int digdug_reset_r(int offset);
extern int digdug_hiscore_print_r(int offset);
extern int digdug_sharedram_r(int offset);
extern void digdug_sharedram_w(int offset,int data);
extern void digdug_interrupt_enable_1_w(int offset,int data);
extern void digdug_interrupt_enable_2_w(int offset,int data);
extern void digdug_interrupt_enable_3_w(int offset,int data);
extern void digdug_halt_w(int offset,int data);
extern int digdug_customio_r(int offset);
extern void digdug_customio_w(int offset,int data);
extern int digdug_interrupt_1(void);
extern int digdug_interrupt_2(void);
extern int digdug_interrupt_3(void);
extern int digdig_init_machine(const char *gamename);

extern unsigned char *digdug_vlatches;
extern void digdug_cpu_reset_w(int offset, int data);
extern void digdug_vh_latch_w(int offset, int data);
extern int digdug_vh_start(void);
extern void digdug_vh_stop(void);
extern void digdug_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void digdug_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

extern void pengo_sound_w(int offset,int data);
extern int rallyx_sh_start(void);
extern void pengo_sh_update(void);
extern unsigned char *pengo_soundregs;
extern unsigned char digdug_hiscoreloaded;


static struct MemoryReadAddress readmem_cpu1[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_r, &digdug_sharedram },
	{ 0x7100, 0x7100, digdug_customio_r },
	{ 0x7000, 0x700f, MRA_RAM },
        { 0x0000, 0x0000, digdug_reset_r },
	{ 0x0001, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu3[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_r },
	{ 0x0000, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu1[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ 0x6830, 0x6830, MWA_NOP },
	{ 0x7100, 0x7100, digdug_customio_w },
	{ 0x7000, 0x700f, MWA_RAM },
	{ 0x6820, 0x6820, digdug_interrupt_enable_1_w },
	{ 0x6822, 0x6822, digdug_interrupt_enable_3_w },
	{ 0x6823, 0x6823, digdug_halt_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8b80, 0x8bff, MWA_RAM, &spriteram },	/* these three are here just to initialize */
	{ 0x9380, 0x93ff, MWA_RAM, &spriteram_2 },	/* the pointers. The actual writes are */
	{ 0x9b80, 0x9bff, MWA_RAM, &spriteram_3 },	/* handled by digdug_sharedram_w() */
	{ 0x8000, 0x83ff, MWA_RAM, &videoram },	/* dirtybuffer[] handling is not needed because */
	{ 0x8400, 0x87ff, MWA_RAM },	/* characters are redrawn every frame */
	{ 0xa000, 0xa00f, digdug_vh_latch_w, &digdug_vlatches },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ 0x6821, 0x6821, digdug_interrupt_enable_2_w },
	{ 0x6830, 0x6830, MWA_NOP },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xa000, 0xa00f, digdug_vh_latch_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu3[] =
{
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
	{ 0x6822, 0x6822, digdug_interrupt_enable_3_w },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* DSW1 */
		0x98,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x24,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_LEFT, 0, OSD_KEY_CONTROL, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT, 0, OSD_JOY_FIRE, 0, 0 },
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 2, 3, "MOVE LEFT"  },
        { 2, 1, "MOVE RIGHT" },
        { 2, 0, "MOVE UP"    },
        { 2, 2, "MOVE DOWN"  },
        { 2, 5, "FIRE"       },
        { -1 }
};


static struct DSW digdug_dsw[] =
{
	{ 0, 0xc0, "LIVES", { "1", "2", "3", "5" } },
 	{ 0, 0x38, "BONUS", { "NONE", "20K 70K 70K", "10K 50K 50K", "20K 60K", "10K 40K 40K", "10K 40K", "20K 60K 60K", "10K" }, 1 },
	{ 1, 0x03, "RANK", { "A", "C", "B", "D" }, 1 },
	{ -1 }
};


static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	128,	/* 128 characters */
	1,		/* 1 bit per pixel */
	{ 0 },	/* one bitplane */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },      /* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },   /* characters are rotated 90 degrees */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },   /* bits are packed in groups of four */
	16*8	       /* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	        /* 16*16 sprites */
	256,	        /* 256 sprites */
	2,	        /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
			7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,            0,  8 },
	{ 1, 0x2000, &spritelayout,         8*2, 32 },
	{ 1, 0x1000, &charlayout2,   32*4 + 8*2, 64 },
	{ -1 } /* end of array */
};



/* the palette used -- these are all guesses until we can get the color PROMs */
static unsigned char digdug_palette[] =
{
	0x00, 0x00, 0x00,	/* 00 black */
	0xff, 0x00, 0x00,	/* 01 red */
	0xde, 0x97, 0x47,	/* 02 brown */
	0xff, 0xb8, 0xde,	/* 03 pink */
	0x97, 0x47, 0x21,	/* 04 dk brown */
	0x00, 0xff, 0xde,	/* 05 cyan */
	0x47, 0x97, 0xde,	/* 06 lt blue */
	0xff, 0x47, 0x00,	/* 07 orange */
	0xff, 0x97, 0x21,	/* 08 lt orange */
	0xff, 0xff, 0x00,	/* 09 yellow */
	0xb8, 0xb8, 0xb8,	/* 0A gray */
	0x21, 0x21, 0xde,	/* 0B dk blue */
	0x21, 0xde, 0x21,	/* 0C green */
	0x47, 0xb8, 0x97,	/* 0D bluish */
	0xde, 0xde, 0x97,	/* 0E tan */
	0xff, 0xff, 0xff,	/* 0F white */
	0x97, 0x97, 0xff, /* 10 lavender */
	0xb8, 0xb8, 0x47, /* 11 dk tan */
	0x67, 0xb8, 0x67, /* 12 gray green */
	0x00, 0x47, 0x00, /* 13 dk green */
	0x47, 0x47, 0x47, /* 14 dk gray */
	0xb8, 0x00, 0xff, /* 15 purple */
};

enum
{	black,red,brown,pink,dkbrown,cyan,ltblue,orange,ltorange,yellow,gray,dkblue,green,bluish,tan,white,
	lavender,dktan,graygreen,dkgreen,dkgray,purple };

/* the color table used -- these are all guesses until we can get the color PROMs */
static unsigned char digdug_color_table[] =
{
	/* alphanumeric colors */
	black,black,
	black,white,
	black,green,
	black,yellow,
	dkblue,dkblue,
	black,red,
	black,yellow,
	black,white,

	/* sprite colors * 32 */
	black,red,white,ltblue,			/* 00 - main dude */
	black,orange,yellow,white,		/* 01 - orange dudes */
	black,red,green,white,			/* 02 - dragon dudes */
	black,black,yellow,white,		/* 03 - floating eyes */
	black,tan,brown,dkbrown,		/* 04 - rocks */
	black,red,orange,yellow,		/* 05 - fire breath */
	black,black,black,black,		/* 06 - black circle behind main dude */
	1,2,3,0,
	black,red,green,white,			/* 08 - pumper */
	black,red,green,white,			/* 09 - namco symbol */
	black,black,ltblue,white,		/* 0a - score bubbles */
	black,yellow,orange,pink,		
	black,white,green,white,		/* 0c - ready text/pre-breathing dragon */
	black,orange,green,white,		/* 0d - watermelon */
	black,ltorange,green,orange,	/* 0e - carrot */
	black,green,dkgreen,white,		/* 0f - pickle */
	black,white,dkgreen,green,		/* 10 - radish */
	black,purple,pink,ltblue,		/* 11 - squash */
	black,white,gray,red,			/* 12 - shroom */
	black,orange,white,green,		/* 13 - tomato */
	black,orange,ltorange,green,	/* 14 - orange thing */
	black,yellow,red,ltblue,		/* 15 - key */
	1,2,3,0,
	1,2,3,0,
	black,yellow,green,orange,		/* 18 - lg flower */
	black,yellow,green,white,		/* 19 - flowers */
	black,dkblue,white,orange,		/* 1A - dig dug big sprite */
	black,dkblue,white,green,		/* 1B - frygar big sprite */
	black,dkblue,green,orange,		/* 1C - pooka big sprite */
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,

	/* playfield colors * 64 */	
	black,ltorange,dkblue,yellow,			/* levels 1-4 */
	black,white,dkblue,red,
	ltorange,yellow,orange,ltorange,
	orange,ltorange,red,orange,
	red,orange,orange,red,
	white,yellow,dkblue,black,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	black,tan,dkblue,ltblue,				/* levels 5-8 */
	black,white,dkblue,red,
	tan,ltblue,brown,tan,
	brown,tan,ltorange,yellow,
	ltorange,yellow,red,ltorange,
	white,yellow,dkblue,black,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	black,dktan,dkblue,lavender,			/* levels 9-12 */
	black,white,dkblue,red,
	dktan,lavender,dkbrown,dktan,
	dkbrown,dktan,dkgreen,graygreen,
	dkgreen,graygreen,gray,dkgray,
	white,yellow,dkblue,black,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	black,ltorange,dkblue,yellow,
	black,white,dkblue,red,
	ltorange,yellow,orange,ltorange,
	orange,ltorange,red,orange,
	red,orange,orange,red,
	white,yellow,dkblue,black,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
	1,2,3,0,
};


#if 0
static unsigned char color_prom[] =
{
	/* 5N - palette */
	0xF6,0x07,0x3F,0x27,0x2F,0xC7,0xF8,0xED,0x16,0x38,0x21,0xD8,0xC4,0xC0,0xA0,0x00,
	0xF6,0x07,0x3F,0x27,0x00,0xC7,0xF8,0xE8,0x00,0x38,0x00,0xD8,0xC5,0xC0,0x00,0x00,
	/* 2N - chars */
	0x0F,0x00,0x00,0x06,0x0F,0x0D,0x01,0x00,0x0F,0x02,0x0C,0x0D,0x0F,0x0B,0x01,0x00,
	0x0F,0x01,0x00,0x01,0x0F,0x00,0x00,0x02,0x0F,0x00,0x00,0x03,0x0F,0x00,0x00,0x05,
	0x0F,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0F,0x0B,0x07,0x06,0x0F,0x06,0x0B,0x07,0x0F,0x07,0x06,0x0B,0x0F,0x0F,0x0F,0x01,
	0x0F,0x0F,0x0B,0x0F,0x0F,0x02,0x0F,0x0F,0x0F,0x06,0x06,0x0B,0x0F,0x06,0x0B,0x0B,
	/* 1C - sprites */
	0x0F,0x08,0x0E,0x02,0x0F,0x05,0x0B,0x0C,0x0F,0x00,0x0B,0x01,0x0F,0x01,0x0B,0x02,
	0x0F,0x08,0x0D,0x02,0x0F,0x06,0x01,0x04,0x0F,0x09,0x01,0x05,0x0F,0x07,0x0B,0x01,
	0x0F,0x01,0x06,0x0B,0x0F,0x01,0x0B,0x00,0x0F,0x01,0x02,0x00,0x0F,0x00,0x01,0x06,
	0x0F,0x00,0x00,0x06,0x0F,0x03,0x0B,0x09,0x0F,0x06,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
#endif




/* waveforms for the audio hardware */
static unsigned char samples[8*32] =
{
	0xff,0x11,0x22,0x33,0x44,0x55,0x55,0x66,0x66,0x66,0x55,0x55,0x44,0x33,0x22,0x11,
	0xff,0xdd,0xcc,0xbb,0xaa,0x99,0x99,0x88,0x88,0x88,0x99,0x99,0xaa,0xbb,0xcc,0xdd,

	0xff,0x11,0x22,0x33,0xff,0x55,0x55,0xff,0x66,0xff,0x55,0x55,0xff,0x33,0x22,0x11,
	0xff,0xdd,0xff,0xbb,0xff,0x99,0xff,0x88,0xff,0x88,0xff,0x99,0xff,0xbb,0xff,0xdd,

	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,

	0x33,0x55,0x66,0x55,0x44,0x22,0x00,0x00,0x00,0x22,0x44,0x55,0x66,0x55,0x33,0x00,
	0xcc,0xaa,0x99,0xaa,0xbb,0xdd,0xff,0xff,0xff,0xdd,0xbb,0xaa,0x99,0xaa,0xcc,0xff,

	0xff,0x22,0x44,0x55,0x66,0x55,0x44,0x22,0xff,0xcc,0xaa,0x99,0x88,0x99,0xaa,0xcc,
	0xff,0x33,0x55,0x66,0x55,0x33,0xff,0xbb,0x99,0x88,0x99,0xbb,0xff,0x66,0xff,0x88,

	0xff,0x66,0x44,0x11,0x44,0x66,0x22,0xff,0x44,0x77,0x55,0x00,0x22,0x33,0xff,0xaa,
	0x00,0x55,0x11,0xcc,0xdd,0xff,0xaa,0x88,0xbb,0x00,0xdd,0x99,0xbb,0xee,0xbb,0x99,

	0xff,0x00,0x22,0x44,0x66,0x55,0x44,0x44,0x33,0x22,0x00,0xff,0xdd,0xee,0xff,0x00,
	0x00,0x11,0x22,0x33,0x11,0x00,0xee,0xdd,0xcc,0xcc,0xbb,0xaa,0xcc,0xee,0x00,0x11,

	0x22,0x44,0x44,0x22,0xff,0xff,0x00,0x33,0x55,0x66,0x55,0x22,0xee,0xdd,0xdd,0xff,
	0x11,0x11,0x00,0xcc,0x99,0x88,0x99,0xbb,0xee,0xff,0xff,0xcc,0xaa,0xaa,0xcc,0xff,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			0,
			readmem_cpu1,writemem_cpu1,0,0,
			digdug_interrupt_1,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			2,	/* memory region #2 */
			readmem_cpu2,writemem_cpu2,0,0,
			digdug_interrupt_2,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			3,	/* memory region #3 */
			readmem_cpu3,writemem_cpu3,0,0,
			digdug_interrupt_3,2
		}
	},
	60,
	digdig_init_machine,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	32,8*2+32*4+64*4,
	0,

	0,
	digdug_vh_start,
	digdug_vh_stop,
	digdug_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	rallyx_sh_start,
	0,
	pengo_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( digdugnm_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "dd1.1b", 0x0000, 0x1000 )
	ROM_LOAD( "dd1.2",  0x1000, 0x1000 )
	ROM_LOAD( "dd1.3",  0x2000, 0x1000 )
	ROM_LOAD( "dd1.4b", 0x3000, 0x1000 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dd1.9", 0x0000, 0x0800 )
	ROM_LOAD( "dd1.11", 0x1000, 0x1000 )
	ROM_LOAD( "dd1.15", 0x2000, 0x1000 )
	ROM_LOAD( "dd1.14", 0x3000, 0x1000 )
	ROM_LOAD( "dd1.13", 0x4000, 0x1000 )
	ROM_LOAD( "dd1.12", 0x5000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "dd1.5b", 0x0000, 0x1000 )
	ROM_LOAD( "dd1.6b", 0x1000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "dd1.7", 0x0000, 0x1000 )

	ROM_REGION(0x01000)	/* 4k for the playfield graphics */
	ROM_LOAD( "dd1.10b", 0x0000, 0x1000 )
ROM_END


ROM_START( digdug_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "136007.101", 0x0000, 0x1000 )
	ROM_LOAD( "136007.102", 0x1000, 0x1000 )
	ROM_LOAD( "136007.103", 0x2000, 0x1000 )
	ROM_LOAD( "136007.104", 0x3000, 0x1000 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136007.108", 0x0000, 0x0800 )
	ROM_LOAD( "136007.115", 0x1000, 0x1000 )
	ROM_LOAD( "136007.116", 0x2000, 0x1000 )
	ROM_LOAD( "136007.117", 0x3000, 0x1000 )
	ROM_LOAD( "136007.118", 0x4000, 0x1000 )
	ROM_LOAD( "136007.119", 0x5000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "136007.105", 0x0000, 0x1000 )
	ROM_LOAD( "136007.106", 0x1000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "136007.107", 0x0000, 0x1000 )

	ROM_REGION(0x01000)	/* 4k for the playfield graphics */
	ROM_LOAD( "136007.114", 0x0000, 0x1000 )
ROM_END


static int hiload(const char *name)
{
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];

   /* check if the hi score table has already been initialized (works for Namco & Atari) */
   if (RAM[0x89b1] == 0x35 && RAM[0x89b4] == 0x35)
   {
      if ((f = fopen(name,"rb")) != 0)
      {
         fread(&RAM[0x89a0],1,37,f);
         fclose(f);
         digdug_hiscoreloaded = 1;
      }

      return 1;
   }
   else
      return 0; /* we can't load the hi scores yet */
}


static void hisave(const char *name)
{
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];

   if ((f = fopen(name,"wb")) != 0)
   {
      fwrite(&RAM[0x89a0],1,37,f);
      fclose(f);
   }
}


struct GameDriver digdugnm_driver =
{
	"Dig Dug - Namco",
	"digdugnm",
	"AARON GILES\nMARTIN SCRAGG\nNICOLA SALMORIA\nMIRKO BUFFONI",
	&machine_driver,

	digdugnm_rom,
	0, 0,
	0,

	input_ports, trak_ports, digdug_dsw, keys,

	0, digdug_palette, digdug_color_table,

	8*11, 8*20,

	hiload, hisave
};


struct GameDriver digdug_driver =
{
	"Dig Dug - Atari",
	"digdug",
	"AARON GILES\nMARTIN SCRAGG\nNICOLA SALMORIA\nMIRKO BUFFONI",
	&machine_driver,

	digdug_rom,
	0, 0,
	0,

	input_ports, trak_ports, digdug_dsw, keys,

	0, digdug_palette, digdug_color_table,

	8*11, 8*20,

	hiload, hisave
};

