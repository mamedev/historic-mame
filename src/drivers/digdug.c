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
int digdug_reset_r(int offset);
int digdug_hiscore_print_r(int offset);
int digdug_sharedram_r(int offset);
void digdug_sharedram_w(int offset,int data);
void digdug_interrupt_enable_1_w(int offset,int data);
void digdug_interrupt_enable_2_w(int offset,int data);
void digdug_interrupt_enable_3_w(int offset,int data);
void digdug_halt_w(int offset,int data);
int digdug_customio_r(int offset);
void digdug_customio_w(int offset,int data);
int digdug_customio_data_r(int offset);
void digdug_customio_data_w(int offset,int data);
int digdug_interrupt_1(void);
int digdug_interrupt_2(void);
int digdug_interrupt_3(void);
void digdig_init_machine(void);

extern unsigned char *digdug_vlatches;
void digdug_cpu_reset_w(int offset, int data);
void digdug_vh_latch_w(int offset, int data);
int digdug_vh_start(void);
void digdug_vh_stop(void);
void digdug_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void digdug_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

void pengo_sound_w(int offset,int data);
extern unsigned char *pengo_soundregs;
extern unsigned char digdug_hiscoreloaded;



static struct MemoryReadAddress readmem_cpu1[] =
{
	{ 0x0000, 0x0000, digdug_reset_r },
	{ 0x0001, 0x3fff, MRA_ROM },
	{ 0x7000, 0x700f, digdug_customio_data_r },
	{ 0x7100, 0x7100, digdug_customio_r },
	{ 0x8000, 0x9fff, digdug_sharedram_r, &digdug_sharedram },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x9fff, digdug_sharedram_r },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu3[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x8000, 0x9fff, digdug_sharedram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu1[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x6820, 0x6820, digdug_interrupt_enable_1_w },
	{ 0x6821, 0x6821, digdug_interrupt_enable_2_w },
	{ 0x6822, 0x6822, digdug_interrupt_enable_3_w },
	{ 0x6823, 0x6823, digdug_halt_w },
	{ 0x6825, 0x6827, MWA_NOP },
	{ 0x6830, 0x6830, watchdog_reset_w },
	{ 0x7000, 0x700f, digdug_customio_data_w },
	{ 0x7100, 0x7100, digdug_customio_w },
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ 0x8000, 0x83ff, MWA_RAM, &videoram, &videoram_size },   /* dirtybuffer[] handling is not needed because */
	{ 0x8400, 0x87ff, MWA_RAM },	                          /* characters are redrawn every frame */
	{ 0x8b80, 0x8bff, MWA_RAM, &spriteram, &spriteram_size }, /* these three are here just to initialize */
	{ 0x9380, 0x93ff, MWA_RAM, &spriteram_2 },	          /* the pointers. The actual writes are */
	{ 0x9b80, 0x9bff, MWA_RAM, &spriteram_3 },                /* handled by digdug_sharedram_w() */
	{ 0xa000, 0xa00f, digdug_vh_latch_w, &digdug_vlatches },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x6821, 0x6821, digdug_interrupt_enable_2_w },
	{ 0x6830, 0x6830, watchdog_reset_w },
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ 0xa000, 0xa00f, digdug_vh_latch_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu3[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
	{ 0x6822, 0x6822, digdug_interrupt_enable_3_w },
	{ 0x8000, 0x9fff, digdug_sharedram_w },
	{ -1 }	/* end of table */
};


/* input from the outside world */
INPUT_PORTS_START( digdug_input_ports )
	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x07, 0x01, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x05, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/7 Credits" )
	/* TODO: bonus scores are different for 5 lives */
	PORT_DIPNAME( 0x38, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "10k 40k 40k" )
	PORT_DIPSETTING(    0x10, "10k 50k 50k" )
	PORT_DIPSETTING(    0x30, "20k 60k 60k" )
	PORT_DIPSETTING(    0x08, "20k 70k 70k" )
	PORT_DIPSETTING(    0x28, "10k 40k" )
	PORT_DIPSETTING(    0x18, "20k 60k" )
	PORT_DIPSETTING(    0x38, "10k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0x80, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0xc0, 0x00, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )

	PORT_START	/* FAKE */
	/* The player inputs are not memory mapped, they are handled by an I/O chip. */
	/* These fake input ports are read by digdug_customio_data_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS, 0 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_START2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

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
	{ 1, 0x2000, &spritelayout,         8*2, 64 },
	{ 1, 0x1000, &charlayout2,   64*4 + 8*2, 64 },
	{ -1 } /* end of array */
};


static unsigned char color_prom[] =
{
	/* 5N - palette */
	0x00,0x2f,0xf6,0x1e,0x28,0x0d,0x36,0x04,0x80,0x5b,0x07,0xa4,0x52,0x5a,0x65,0x00,
	0x00,0x07,0x2f,0x28,0xe8,0xf6,0x36,0x1f,0x65,0x14,0x0a,0xdf,0xd8,0xd0,0x84,0x00,
	/* 1C - sprites */
	0x0f,0x01,0x05,0x0c,0x0f,0x01,0x06,0x05,0x0f,0x01,0x03,0x05,0x0f,0x00,0x06,0x05,
	0x0f,0x08,0x09,0x0a,0x0f,0x01,0x06,0x07,0x0f,0x00,0x00,0x00,0x0f,0x01,0x06,0x04,
	0x0f,0x01,0x00,0x05,0x0f,0x01,0x0f,0x05,0x0f,0x00,0x04,0x00,0x0f,0x06,0x07,0x0b,
	0x0f,0x05,0x03,0x05,0x0f,0x01,0x03,0x08,0x0f,0x01,0x03,0x08,0x0f,0x00,0x03,0x05,
	0x0f,0x05,0x03,0x08,0x0f,0x0e,0x0b,0x0d,0x0f,0x05,0x08,0x01,0x0f,0x01,0x05,0x03,
	0x0f,0x09,0x07,0x02,0x0f,0x06,0x01,0x0d,0x0f,0x06,0x03,0x09,0x0f,0x06,0x03,0x0b,
	0x0f,0x06,0x03,0x01,0x0f,0x07,0x03,0x05,0x0f,0x0d,0x05,0x01,0x0f,0x0d,0x05,0x03,
	0x0f,0x04,0x03,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/* 2N - playfield */
	0x00,0x06,0x08,0x01,0x00,0x02,0x08,0x0a,0x06,0x01,0x01,0x03,0x01,0x03,0x03,0x05,
	0x03,0x05,0x05,0x07,0x02,0x06,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x09,0x08,0x0b,0x00,0x02,0x08,0x0a,0x09,0x0b,0x0c,0x0e,0x0c,0x0e,0x09,0x01,
	0x09,0x01,0x07,0x03,0x02,0x06,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x09,0x08,0x0b,0x00,0x02,0x08,0x0a,0x09,0x0b,0x0c,0x09,0x0c,0x09,0x00,0x0d,
	0x00,0x0d,0x09,0x0c,0x02,0x06,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x09,0x08,0x0e,0x00,0x02,0x08,0x0a,0x09,0x0e,0x05,0x0e,0x05,0x0e,0x0c,0x0e,
	0x0c,0x0e,0x07,0x0e,0x02,0x06,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};



/* waveforms for the audio hardware */
static unsigned char sound_prom[] =
{
	0x07,0x09,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0e,0x0e,0x0d,0x0d,0x0c,0x0b,0x0a,0x09,
	0x07,0x05,0x04,0x03,0x02,0x01,0x01,0x00,0x00,0x00,0x01,0x01,0x02,0x03,0x04,0x05,
	0x07,0x09,0x0a,0x0b,0x07,0x0d,0x0d,0x07,0x0e,0x07,0x0d,0x0d,0x07,0x0b,0x0a,0x09,
	0x07,0x05,0x07,0x03,0x07,0x01,0x07,0x00,0x07,0x00,0x07,0x01,0x07,0x03,0x07,0x05,
	0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0b,0x0d,0x0e,0x0d,0x0c,0x0a,0x08,0x08,0x08,0x0a,0x0c,0x0d,0x0e,0x0d,0x0b,0x08,
	0x04,0x02,0x01,0x02,0x03,0x05,0x07,0x07,0x07,0x05,0x03,0x02,0x01,0x02,0x04,0x07,
	0x07,0x0a,0x0c,0x0d,0x0e,0x0d,0x0c,0x0a,0x07,0x04,0x02,0x01,0x00,0x01,0x02,0x04,
	0x07,0x0b,0x0d,0x0e,0x0d,0x0b,0x07,0x03,0x01,0x00,0x01,0x03,0x07,0x0e,0x07,0x00,
	0x07,0x0e,0x0c,0x09,0x0c,0x0e,0x0a,0x07,0x0c,0x0f,0x0d,0x08,0x0a,0x0b,0x07,0x02,
	0x08,0x0d,0x09,0x04,0x05,0x07,0x02,0x00,0x03,0x08,0x05,0x01,0x03,0x06,0x03,0x01,
	0x07,0x08,0x0a,0x0c,0x0e,0x0d,0x0c,0x0c,0x0b,0x0a,0x08,0x07,0x05,0x06,0x07,0x08,
	0x08,0x09,0x0a,0x0b,0x09,0x08,0x06,0x05,0x04,0x04,0x03,0x02,0x04,0x06,0x08,0x09,
	0x0a,0x0c,0x0c,0x0a,0x07,0x07,0x08,0x0b,0x0d,0x0e,0x0d,0x0a,0x06,0x05,0x05,0x07,
	0x09,0x09,0x08,0x04,0x01,0x00,0x01,0x03,0x06,0x07,0x07,0x04,0x02,0x02,0x04,0x07
};



static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	32,			/* gain adjustment */
	255			/* playback volume */
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
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	digdig_init_machine,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	32,8*2+64*4+64*4,
	digdug_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	digdug_vh_start,
	digdug_vh_stop,
	digdug_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( digdug_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "136007.101", 0x0000, 0x1000, 0x530a8d1c )
	ROM_LOAD( "136007.102", 0x1000, 0x1000, 0x3e4a1cb6 )
	ROM_LOAD( "136007.103", 0x2000, 0x1000, 0x2a1e5ce2 )
	ROM_LOAD( "136007.104", 0x3000, 0x1000, 0xc6c1f5e1 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136007.108", 0x0000, 0x0800, 0xaf5e219e )
	ROM_LOAD( "136007.115", 0x1000, 0x1000, 0x87be0000 )
	ROM_LOAD( "136007.116", 0x2000, 0x1000, 0xff914d75 )
	ROM_LOAD( "136007.117", 0x3000, 0x1000, 0x0b276793 )
	ROM_LOAD( "136007.118", 0x4000, 0x1000, 0x494d7f6d )
	ROM_LOAD( "136007.119", 0x5000, 0x1000, 0xfd468dcc )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "136007.105", 0x0000, 0x1000, 0xbdb75bb1 )
	ROM_LOAD( "136007.106", 0x1000, 0x1000, 0xa3ac5198 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "136007.107", 0x0000, 0x1000, 0xc7bdef23 )

	ROM_REGION(0x01000)	/* 4k for the playfield graphics */
	ROM_LOAD( "136007.114", 0x0000, 0x1000, 0xcce929b7 )
ROM_END

ROM_START( digdugnm_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "136007.101", 0x0000, 0x1000, 0x530a8d1c )
	ROM_LOAD( "136007.102", 0x1000, 0x1000, 0x3e4a1cb6 )
	ROM_LOAD( "136007.103", 0x2000, 0x1000, 0x2a1e5ce2 )
	ROM_LOAD( "dd1.4b",     0x3000, 0x1000, 0xaaddfff7 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dd1.9",      0x0000, 0x0800, 0x81dba5a5 )
	ROM_LOAD( "dd1.11",     0x1000, 0x1000, 0xd6a41d2e )
	ROM_LOAD( "136007.116", 0x2000, 0x1000, 0xff914d75 )
	ROM_LOAD( "dd1.14",     0x3000, 0x1000, 0xa3d074d0 )
	ROM_LOAD( "136007.118", 0x4000, 0x1000, 0x494d7f6d )
	ROM_LOAD( "136007.119", 0x5000, 0x1000, 0xfd468dcc )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "dd1.5b", 0x0000, 0x1000, 0xe09b56bb )
	ROM_LOAD( "dd1.6b", 0x1000, 0x1000, 0xee615c91 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "136007.107", 0x0000, 0x1000, 0xc7bdef23 )

	ROM_REGION(0x01000)	/* 4k for the playfield graphics */
	ROM_LOAD( "dd1.10b", 0x0000, 0x1000, 0x581d2bb7 )
ROM_END



static int hiload(void)
{
   void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


   /* check if the hi score table has already been initialized (works for Namco & Atari) */
   if (RAM[0x89b1] == 0x35 && RAM[0x89b4] == 0x35)
   {
      if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
      {
         osd_fread(f,&RAM[0x89a0],37);
         osd_fclose(f);
         digdug_hiscoreloaded = 1;
      }

      return 1;
   }
   else
      return 0; /* we can't load the hi scores yet */
}


static void hisave(void)
{
   void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


   if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
   {
      osd_fwrite(f,&RAM[0x89a0],37);
      osd_fclose(f);
   }
}


struct GameDriver digdug_driver =
{
	__FILE__,
	0,
	"digdug",
	"Dig Dug (Atari)",
	"1982",
	"[Namco] (Atari license)",
	"Aaron Giles\nMartin Scragg\nNicola Salmoria\nMirko Buffoni\nAlan J McCormick",
	0,
	&machine_driver,

	digdug_rom,
	0, 0,
	0,
	sound_prom,	/* sound_prom */

	digdug_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver digdugnm_driver =
{
	__FILE__,
	&digdug_driver,
	"digdugnm",
	"Dig Dug (Namco)",
	"1982",
	"Namco",
	"Aaron Giles\nMartin Scragg\nNicola Salmoria\nMirko Buffoni\nAlan J McCormick",
	0,
	&machine_driver,

	digdugnm_rom,
	0, 0,
	0,
	sound_prom,	/* sound_prom */

	digdug_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
