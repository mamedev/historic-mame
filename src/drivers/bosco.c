/***************************************************************************

bosco memory map (preliminary)

CPU #1:
0000-3fff ROM
CPU #2:
0000-1fff ROM
CPU #3:
0000-1fff ROM
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
7100      custom IO chip command (see machine/bosco.c for more details)
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

extern unsigned char *bosco_sharedram;
int  bosco_reset_r(int offset);
int  bosco_hiscore_print_r(int offset);
int  bosco_sharedram_r(int offset);
void bosco_sharedram_w(int offset,int data);
int  bosco_dsw_r(int offset);
void bosco_interrupt_enable_1_w(int offset,int data);
void bosco_interrupt_enable_2_w(int offset,int data);
void bosco_interrupt_enable_3_w(int offset,int data);
void bosco_halt_w(int offset,int data);
int  bosco_customio_r_1(int offset);
int  bosco_customio_r_2(int offset);
void bosco_customio_w_1(int offset,int data);
void bosco_customio_w_2(int offset,int data);
int  bosco_customio_data_r_1(int offset);
int  bosco_customio_data_r_2(int offset);
void bosco_customio_data_w_1(int offset,int data);
void bosco_customio_data_w_2(int offset,int data);
int  bosco_interrupt_1(void);
int  bosco_interrupt_2(void);
int  bosco_interrupt_3(void);
void bosco_init_machine(void);

void bosco_cpu_reset_w(int offset, int data);
int  bosco_vh_start(void);
void bosco_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bosco_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

extern unsigned char *bosco_videoram2,*bosco_colorram2;
extern unsigned char *bosco_radarx,*bosco_radary,*bosco_radarattr;
extern int bosco_radarram_size;
extern unsigned char *bosco_scrollx,*bosco_scrolly;
extern unsigned char *bosco_starcontrol,*bosco_staronoff;
extern unsigned char *bosco_flipvideo,*bosco_starblink;
void bosco_videoram2_w(int offset,int data);
void bosco_colorram2_w(int offset,int data);
int  bosco_vh_start(void);
void bosco_vh_stop(void);
void bosco_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void pengo_sound_w(int offset,int data);
int  bosco_sh_start(void);
void bosco_sh_stop(void);
extern unsigned char *pengo_soundregs;
extern unsigned char bosco_hiscoreloaded;
extern int  HiScore;


static struct MemoryReadAddress readmem_cpu1[] =
{
	{ 0x7800, 0x9fff, bosco_sharedram_r, &bosco_sharedram },
	{ 0x6800, 0x6807, bosco_dsw_r },
	{ 0x7000, 0x700f, bosco_customio_data_r_1 },
	{ 0x7100, 0x7100, bosco_customio_r_1 },
	{ 0x0000, 0x3fff, MRA_ROM },

	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x9000, 0x900f, bosco_customio_data_r_2 },
	{ 0x9100, 0x9100, bosco_customio_r_2 },
	{ 0x7800, 0x9fff, bosco_sharedram_r },
	{ 0x6800, 0x6807, bosco_dsw_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu3[] =
{
	{ 0x7800, 0x9fff, bosco_sharedram_r },
	{ 0x6800, 0x6807, bosco_dsw_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu1[] =
{
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, bosco_videoram2_w, &bosco_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, bosco_colorram2_w, &bosco_colorram2 },

	{ 0x7800, 0x9fff, bosco_sharedram_w },

	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
	{ 0x7000, 0x700f, bosco_customio_data_w_1 },
	{ 0x7100, 0x7100, bosco_customio_w_1 },
	{ 0x6820, 0x6820, bosco_interrupt_enable_1_w },
	{ 0x6822, 0x6822, bosco_interrupt_enable_3_w },
	{ 0x6823, 0x6823, bosco_halt_w },
/*	{ 0x6830, 0x6830, watchdog_reset_w },	*/
	{ 0x0000, 0x3fff, MWA_ROM },

	{ 0x83d4, 0x83df, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8bd4, 0x8bdf, MWA_RAM, &spriteram_2 },	                /* the pointers. */
	{ 0x83f4, 0x83ff, MWA_RAM, &bosco_radarx, &bosco_radarram_size },	/* ditto */
	{ 0x8bf4, 0x8bff, MWA_RAM, &bosco_radary },
	{ 0x9810, 0x9810, MWA_RAM, &bosco_scrollx },
	{ 0x9820, 0x9820, MWA_RAM, &bosco_scrolly },
	{ 0x9830, 0x9830, MWA_RAM, &bosco_starcontrol },
	{ 0x9840, 0x9840, MWA_RAM, &bosco_staronoff },
	{ 0x9870, 0x9870, MWA_RAM, &bosco_flipvideo },
	{ 0x9874, 0x9875, MWA_RAM, &bosco_starblink },
	{ 0x9804, 0x980f, MWA_RAM, &bosco_radarattr },

	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x8000, 0x83ff, videoram_w },
	{ 0x8400, 0x87ff, bosco_videoram2_w },
	{ 0x8800, 0x8bff, colorram_w },
	{ 0x8c00, 0x8fff, bosco_colorram2_w },

	{ 0x9000, 0x900f, bosco_customio_data_w_2 },
	{ 0x9100, 0x9100, bosco_customio_w_2 },
	{ 0x7800, 0x9fff, bosco_sharedram_w },
	{ 0x6821, 0x6821, bosco_interrupt_enable_2_w },
/*	{ 0x6830, 0x6830, watchdog_reset_w },	*/
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu3[] =
{
	{ 0x8000, 0x83ff, videoram_w },
	{ 0x8400, 0x87ff, bosco_videoram2_w },
	{ 0x8800, 0x8bff, colorram_w },
	{ 0x8c00, 0x8fff, bosco_colorram2_w },

	{ 0x7800, 0x9fff, bosco_sharedram_w },
	{ 0x6800, 0x681f, pengo_sound_w },
	{ 0x6822, 0x6822, bosco_interrupt_enable_3_w },
/*	{ 0x6830, 0x6830, watchdog_reset_w },	*/
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( bosco_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	/* TODO: bonus scores are different for 5 lives */
	PORT_DIPNAME( 0x38, 0x08, "Bonus Fighter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "15K 50K" )
	PORT_DIPSETTING(    0x38, "20K 70K" )
	PORT_DIPSETTING(    0x08, "10K 50K 50K" )
	PORT_DIPSETTING(    0x10, "15K 50K 50K" )
	PORT_DIPSETTING(    0x18, "15K 70K 70K" )
	PORT_DIPSETTING(    0x20, "20K 70K 70K" )
	PORT_DIPSETTING(    0x28, "40K 100K 100K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0x80, "Fighters", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Player" )
	PORT_DIPSETTING(    0x01, "2 Players" )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x06, "Medium" )
	PORT_DIPSETTING(    0x04, "Hardest" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x08, "Yes" )
	PORT_DIPNAME( 0x10, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Test ????", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
	/* The player inputs are not memory mapped, they are handled by an I/O chip. */
	/* These fake input ports are read by galaga_customio_data_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS, 0 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_IMPULSE | IPF_COCKTAIL,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS, 0 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* FAKE */
	/* the button here is used to trigger the sound in the test screen */
	PORT_BITX(0x03, IP_ACTIVE_LOW, IPT_BUTTON1,     0, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_START1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_START2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END


INPUT_PORTS_START( bosconm_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	/* TODO: bonus scores are different for 5 lives */
	PORT_DIPNAME( 0x38, 0x08, "Bonus Fighter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "15K 50K" )
	PORT_DIPSETTING(    0x38, "20K 70K" )
	PORT_DIPSETTING(    0x08, "10K 50K 50K" )
	PORT_DIPSETTING(    0x10, "15K 50K 50K" )
	PORT_DIPSETTING(    0x18, "15K 70K 70K" )
	PORT_DIPSETTING(    0x20, "20K 70K 70K" )
	PORT_DIPSETTING(    0x28, "40K 100K 100K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0x80, "Fighters", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Player" )
	PORT_DIPSETTING(    0x01, "2 Players" )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x06, "Medium" )
	PORT_DIPSETTING(    0x04, "Hardest" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Test ????", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
	/* The player inputs are not memory mapped, they are handled by an I/O chip. */
	/* These fake input ports are read by galaga_customio_data_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS, 0 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_IMPULSE | IPF_COCKTAIL,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS, 0 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* FAKE */
	/* the button here is used to trigger the sound in the test screen */
	PORT_BITX(0x03, IP_ACTIVE_LOW, IPT_BUTTON1,     0, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_START1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_START2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },      /* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },   /* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },   /* characters are rotated 90 degrees */
	16*8	       /* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	        /* 16*16 sprites */
	64,	        /* 128 sprites */
	2,	        /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3  },
	{ 0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8,
			32 * 8, 33 * 8, 34 * 8, 35 * 8, 36 * 8, 37 * 8, 38 * 8, 39 * 8 },
	64*8	/* every sprite takes 64 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 64 },
	{ 1, 0x1000, &spritelayout, 64*4, 64 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* PROM 6B Palette */
	0xF6,0x07,0x1F,0x3F,0xC4,0xDF,0xF8,0xD8,0x0B,0x28,0xC3,0x51,0x26,0x0D,0xA4,0x00,
	0xA4,0x0D,0x1F,0x3F,0xC4,0xDF,0xF8,0xD8,0x0B,0x28,0xC3,0x51,0x26,0x07,0xF6,0x00,

	/* PROM 4M Lookup Table */
	0x0F,0x0F,0x0F,0x0F,0x0F,0x01,0x00,0x0E,0x0F,0x02,0x06,0x01,0x0F,0x05,0x04,0x01,
	0x0F,0x06,0x07,0x02,0x0F,0x02,0x07,0x08,0x0F,0x0C,0x01,0x0B,0x0F,0x03,0x09,0x01,
	0x0F,0x00,0x0E,0x01,0x0F,0x00,0x01,0x02,0x0F,0x0E,0x00,0x0C,0x0F,0x07,0x0E,0x0D,
	0x0F,0x0E,0x03,0x0D,0x0F,0x00,0x00,0x07,0x0F,0x0D,0x00,0x06,0x0F,0x09,0x0B,0x04,
	0x0F,0x09,0x0B,0x09,0x0F,0x09,0x0B,0x0B,0x0F,0x0D,0x05,0x0E,0x0F,0x09,0x0B,0x01,
	0x0F,0x09,0x04,0x01,0x0F,0x09,0x0B,0x05,0x0F,0x09,0x0B,0x0D,0x0F,0x09,0x09,0x01,
	0x0F,0x0D,0x07,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0E,
	0x00,0x0D,0x0F,0x0D,0x0F,0x0F,0x0F,0x0D,0x0F,0x0F,0x0F,0x0E,0x0F,0x0D,0x0F,0x07,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0D,0x0E,0x00,0x0F,0x0F,0x0F,0x0E,0x0F,0x0B,0x09,0x04,
	0x0F,0x09,0x0B,0x09,0x0F,0x09,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,
	0x00,0x00,0x00,0x0E,0x0F,0x0D,0x0F,0x0D,0x0F,0x0F,0x0F,0x0D,0x0F,0x0F,0x0F,0x0E,
	0x0F,0x0F,0x0F,0x07,0x0F,0x0F,0x0F,0x02,0x0F,0x0F,0x0F,0x07,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0E,0x0E,0x0F,0x01,0x0F,0x01,0x0F,0x09,0x03,0x07,0x0F,0x02,0x07,0x09,
	0x0F,0x0C,0x05,0x01,0x0D,0x0D,0x0F,0x0F,0x03,0x03,0x0F,0x0F,0x09,0x09,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0D,0x0F,0x0F,0x0F,0x06,0x0F,0x0F,0x0F,0x05,0x0F,0x03,0x0F,0x03,
	0x0F,0x03,0x0F,0x05,0x0F,0x0F,0x0F,0x03,0x0F,0x0F,0x03,0x05,0x0F,0x03,0x0F,0x0F
};




/* waveforms for the audio hardware */
static unsigned char sound_prom[] =
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



static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	32,			/* gain adjustment */
	255			/* playback volume */
};


static struct Samplesinterface samples_interface =
{
	3	/* 3 channels */
};


static struct CustomSound_interface custom_interface =
{
	bosco_sh_start,
	bosco_sh_stop,
	0
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
			bosco_interrupt_1,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			2,	/* memory region #2 */
			readmem_cpu2,writemem_cpu2,0,0,
			bosco_interrupt_2,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			3,	/* memory region #3 */
			readmem_cpu3,writemem_cpu3,0,0,
			bosco_interrupt_3,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	bosco_init_machine,

	/* video hardware */
	36*8, 28*8, { 0*8+3, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	32+64,64*4+64*4,	/* 32 for the characters, 64 for the stars */
	bosco_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	bosco_vh_start,
	bosco_vh_stop,
	bosco_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		},
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};





/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bosco_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "2300.3n", 0x0000, 0x1000, 0xebaf78dd )
	ROM_LOAD( "2400.3m", 0x1000, 0x1000, 0x829da7c9 )
	ROM_LOAD( "2500.3l", 0x2000, 0x1000, 0x14f87820 )
	ROM_LOAD( "2600.3k", 0x3000, 0x1000, 0xa4a8c286 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5300.5d", 0x0000, 0x1000, 0x1610cd08 )
	ROM_LOAD( "5200.5e", 0x1000, 0x1000, 0xb04edf54 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "2700.3j", 0x0000, 0x1000, 0xb85fb57d )
	ROM_LOAD( "2800.3h", 0x1000, 0x1000, 0xf773c773 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "2900.3e", 0x0000, 0x1000, 0x6b74d2ca )

	ROM_REGION(0x3000)	/* ROMs for digitised speech */
	ROM_LOAD( "4900.5n", 0x0000, 0x1000, 0x409e4312 )
	ROM_LOAD( "5000.5m", 0x1000, 0x1000, 0x01fce73c )
	ROM_LOAD( "5100.5l", 0x2000, 0x1000, 0x0c0be19b )
ROM_END

ROM_START( bosconm_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU  */
	ROM_LOAD( "bos1_1.bin",  0x0000, 0x1000, 0x345df511 )
	ROM_LOAD( "bos1_2.bin",  0x1000, 0x1000, 0xf9998bad )
	ROM_LOAD( "bos1_3.bin",  0x2000, 0x1000, 0x810c3c4e )
	ROM_LOAD( "bos1_4b.bin", 0x3000, 0x1000, 0x5cf9f585 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5300.5d", 0x0000, 0x1000, 0x1610cd08 )
	ROM_LOAD( "5200.5e", 0x1000, 0x1000, 0xb04edf54 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "bos1_5c.bin", 0x0000, 0x1000, 0x36dceeb2 )
	ROM_LOAD( "2800.3h",     0x1000, 0x1000, 0xf773c773 )

	ROM_REGION(0x10000)	/* 64k for the third CPU  */
	ROM_LOAD( "2900.3e", 0x0000, 0x1000, 0x6b74d2ca )

	ROM_REGION(0x3000)	/* ROMs for digitised speech */
	ROM_LOAD( "4900.5n", 0x0000, 0x1000, 0x409e4312 )
	ROM_LOAD( "5000.5m", 0x1000, 0x1000, 0x01fce73c )
	ROM_LOAD( "5100.5l", 0x2000, 0x1000, 0x0c0be19b )
ROM_END

static const char *bosco_sample_names[] =
{
	"*bosco",
	"MIDBANG.SAM",
	"BIGBANG.SAM",
	"SHOT.SAM",
	0	/* end of array */
};


static int hiload(void)
{
	void *f;
	int		i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8bd3],"\x18",1) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8BC5],15);
			osd_fread(f,&RAM[0x8BE4],16);
			osd_fread(f,&RAM[0x885C],4);
			osd_fread(f,&RAM[0x8060],8);
			osd_fclose(f);
		}
		HiScore = 0;
		for (i = 0; i < 3; i++)
		{
			HiScore = HiScore * 10 + RAM[0x8065 + i];
			/* Set colour RAM to show large values if they are not zero */
			if (RAM[0x8065 + i] != 0)
				RAM[0x8865 + i] = 0x62;
		}
		for (i = 0; i < 4; i++)
		{
			HiScore = HiScore * 10 + RAM[0x8060 + i];
		}
		if (HiScore == 0)
			HiScore = 20000;
		bosco_hiscoreloaded = 1;
		return 1;
	}
	return 0; /* we can't load the hi scores yet */
}


static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8BC5],15);
		osd_fwrite(f,&RAM[0x8BE4],16);
		osd_fwrite(f,&RAM[0x885C],4);
		osd_fwrite(f,&RAM[0x8060],8);
		osd_fclose(f);
	}
}


struct GameDriver bosco_driver =
{
	__FILE__,
	0,
	"bosco",
	"Bosconian (Midway)",
	"1981",
	"[Namco] (Midway license)",
	"Martin Scragg\nAaron Giles\nPete Grounds\nSidney Brown\nKurt Mahan (color info)\nNicola Salmoria\nMirko Buffoni",
	0,
	&machine_driver,

	bosco_rom,
	0, 0,
	bosco_sample_names,
	sound_prom,	/* sound_prom */

	bosco_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver bosconm_driver =
{
	__FILE__,
	&bosco_driver,
	"bosconm",
	"Bosconian (Namco)",
	"1981",
	"Namco",
	"Martin Scragg\nAaron Giles\nPete Grounds\nSidney Brown\nKurt Mahan (color info)\nNicola Salmoria\nMirko Buffoni",
	0,
	&machine_driver,

	bosconm_rom,
	0, 0,
	bosco_sample_names,
	sound_prom,	/* sound_prom */

	bosconm_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
