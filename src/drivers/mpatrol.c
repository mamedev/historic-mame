/***************************************************************************

Moon Patrol memory map (preliminary)

0000-3fff ROM
8000-83ff Video RAM
8400-87ff Color RAM
e000-e7ff RAM


read:
8800      protection
d000      IN0
d001      IN1
d002      IN2
d003      DSW1
d004      DSW2

write:
c820-c87f sprites
c8a0-c8ff sprites
d000-d001 ?

I/O ports
write:
1c-1f     scroll registers
40        background #1 x position
60        background #1 y position
80        background #2 x position
a0        background #2 y position
c0        background control?


There's an interesting problem with this game: it is designed to run on an
horizontal monitor, but the display in MAME is narrow and tall. The reason
is that the real board doesn't produce square pixels, but rectangular ones
whose width is almost twice their height.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



void mpatrol_scroll_w(int offset,int data);
void mpatrol_bg1xpos_w(int offset,int data);
void mpatrol_bg1ypos_w(int offset,int data);
void mpatrol_bg2xpos_w(int offset,int data);
void mpatrol_bg2ypos_w(int offset,int data);
void mpatrol_bgcontrol_w(int offset,int data);
int mpatrol_vh_start(void);
void mpatrol_vh_stop(void);
void mpatrol_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mpatrol_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void mpatrol_io_w(int offset, int value);
int mpatrol_io_r(int offset);
void mpatrol_adpcm_reset_w(int offset,int value);
void mpatrol_sound_cmd_w(int offset, int value);

void mpatrol_adpcm_int(int data);


/* this looks like some kind of protection. The game does strange things */
/* if a read from this address doesn't return the value it expects. */
int mpatrol_protection_r(int offset)
{
	Z80_Regs regs;


	if (errorlog) fprintf(errorlog,"%04x: read protection\n",cpu_getpc());
	Z80_GetRegs(&regs);
	return regs.DE.B.l;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xd000, 0xd000, input_port_0_r },     /* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },     /* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },     /* IN2 */
	{ 0xd003, 0xd003, input_port_3_r },     /* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },     /* DSW2 */
	{ 0x8800, 0x8800, mpatrol_protection_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0xc820, 0xc87f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8a0, 0xc8ff, MWA_RAM, &spriteram_2 },
	{ 0xd000, 0xd001, mpatrol_sound_cmd_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0x1c, 0x1f, mpatrol_scroll_w },
	{ 0x40, 0x40, mpatrol_bg1xpos_w },
	{ 0x60, 0x60, mpatrol_bg1ypos_w },
	{ 0x80, 0x80, mpatrol_bg2xpos_w },
	{ 0xa0, 0xa0, mpatrol_bg2ypos_w },
	{ 0xc0, 0xc0, mpatrol_bgcontrol_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0801, 0x0802, MSM5205_data_w },
	{ 0x9000, 0x9000, MWA_NOP },    /* IACK */
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( mpatrol_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
/* coin input must be active for ? frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 17)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "10000 30000 50000" )
	PORT_DIPSETTING(    0x08, "20000 40000 60000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, "None" )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x90, "7 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/8 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
/* 0x80 gives 1 Coin/1 Credit */
/*	PORT_DIPNAME( 0x30, 0x30, "Coin A Mode 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Free" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B Mode 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )*/

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Sector Selection", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* Identical to mpatrol, the only difference is the number of lives */
INPUT_PORTS_START( mpatrolw_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
/* coin input must be active for ? frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 17)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "10000 30000 50000" )
	PORT_DIPSETTING(    0x08, "20000 40000 60000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, "None" )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x90, "7 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/8 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
/* 0x80 gives 1 Coin/1 Credit */
/*	PORT_DIPNAME( 0x30, 0x30, "Coin A Mode 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Free" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B Mode 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )*/

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Sector Selection", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 128*16*16 },       /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bgcharlayout =
{
	32,32,  /* 32*32 characters (actually, it is just 1 big 256x64 image) */
	8,      /* 8 characters */
	2,      /* 2 bits per pixel */
	{ 4, 0 },       /* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3, 2*8+0, 2*8+1, 2*8+2, 2*8+3, 3*8+0, 3*8+1, 3*8+2, 3*8+3,
			4*8+0, 4*8+1, 4*8+2, 4*8+3, 5*8+0, 5*8+1, 5*8+2, 5*8+3, 6*8+0, 6*8+1, 6*8+2, 6*8+3, 7*8+0, 7*8+1, 7*8+2, 7*8+3 },
	{ 0*512, 1*512, 2*512, 3*512, 4*512, 5*512, 6*512, 7*512, 8*512, 9*512, 10*512, 11*512, 12*512, 13*512, 14*512, 15*512,
			16*512, 17*512, 18*512, 19*512, 20*512, 21*512, 22*512, 23*512, 24*512, 25*512, 26*512, 27*512, 28*512, 29*512, 30*512, 31*512 },
	8*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,               0, 64 },
	{ 1, 0x2000, &spritelayout,          64*4, 16 },
	{ 1, 0x4000, &bgcharlayout, 64*4+16*4+0*4,  1 },	/* top half */
	{ 1, 0x4800, &bgcharlayout, 64*4+16*4+0*4,  1 },	/* bottom half */
	{ 1, 0x5000, &bgcharlayout, 64*4+16*4+1*4,  1 },	/* top half */
	{ 1, 0x5800, &bgcharlayout, 64*4+16*4+1*4,  1 },	/* bottom half */
	{ 1, 0x6000, &bgcharlayout, 64*4+16*4+2*4,  1 },	/* top half */
	{ 1, 0x6800, &bgcharlayout, 64*4+16*4+2*4,  1 },	/* bottom half */
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* 2A - character palette */
	0x00,0xE8,0xFF,0x0F,0xC8,0x0F,0x3F,0xC8,0xE8,0x01,0x0F,0xC8,0xE8,0x28,0x87,0x00,
	0x00,0x67,0x00,0x5C,0x67,0x01,0x00,0x00,0xC8,0x87,0x01,0xE8,0xE8,0x87,0x00,0x00,
	0x00,0xFF,0x00,0x00,0xC8,0x01,0x0F,0x00,0xE8,0x87,0x00,0x00,0x9D,0x87,0x91,0xE8,
	0x9D,0x0F,0x3F,0x9D,0xE8,0x01,0x0F,0x9D,0x9D,0x01,0x0F,0x00,0x00,0x01,0xBD,0xE8,
	0x00,0x67,0xBD,0xE8,0x00,0x07,0xBD,0xE8,0x00,0x67,0xBD,0xE8,0x00,0x67,0xBD,0x01,
	0x00,0x67,0xE8,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/* 1M - background palette */
	0x00,0x20,0x00,0x70,0xC0,0x20,0x00,0x70,0x00,0x20,0x00,0x70,0xA0,0x20,0x00,0x70,
	0x00,0x00,0x77,0x70,0xC0,0x00,0x77,0x70,0x00,0x00,0x77,0x70,0xA0,0x00,0x77,0x70,
	/* 1c1j - sprite palette */
	0x00,0x01,0xC6,0x37,0xB8,0xC0,0x38,0x80,0xFF,0xF8,0x98,0x50,0x47,0xE8,0x6F,0x18,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/* 2hx - sprite lookup table */
	0x00,0x01,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x04,0x02,0x05,0x00,0x00,0x00,0x00,
	0x00,0x05,0x06,0x07,0x00,0x00,0x00,0x00,0x00,0x07,0x08,0x09,0x00,0x00,0x00,0x00,
	0x00,0x0A,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x0E,0x05,0x00,0x00,0x00,0x00,
	0x00,0x05,0x03,0x0F,0x00,0x00,0x00,0x00,0x00,0x09,0x01,0x05,0x00,0x00,0x00,0x00,
	0x00,0x01,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01,0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x04,0x0D,0x05,0x00,0x00,0x00,0x00,
	0x00,0x05,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	910000,	/* .91 MHZ ?? */
	{ 160, 160 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0, mpatrol_adpcm_reset_w }
};

static struct MSM5205interface msm5205_interface =
{
	2,			/* 2 chips */
	4000,       /* 4000Hz playback */
	mpatrol_adpcm_int,	/* interrupt function */
	{ 255, 255 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,        /* 3.072 Mhz ? */
			0,
			readmem,writemem,0,writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,        /* 1.0 Mhz ? */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* interrupts are generated by the ADPCM hardware */
		}
	},
	57, 1790,	/* accurate frequency, measured on a real board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	128+32+32,64*4+16*4+3*4,
	mpatrol_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mpatrol_vh_start,
	mpatrol_vh_stop,
	mpatrol_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mpatrol_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mp-a.3m", 0x0000, 0x1000, 0x0440639c )
	ROM_LOAD( "mp-a.3l", 0x1000, 0x1000, 0x15d6639c )
	ROM_LOAD( "mp-a.3k", 0x2000, 0x1000, 0x2f2a0bf4 )
	ROM_LOAD( "mp-a.3j", 0x3000, 0x1000, 0xf5377887 )

	ROM_REGION(0x7000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mp-e.3e", 0x0000, 0x1000, 0xefe9bb1d )       /* chars */
	ROM_LOAD( "mp-e.3f", 0x1000, 0x1000, 0x796d8525 )
	ROM_LOAD( "mp-b.3m", 0x2000, 0x1000, 0xfe518a23 )       /* sprites */
	ROM_LOAD( "mp-b.3n", 0x3000, 0x1000, 0x974b35c3 )
	ROM_LOAD( "mp-e.3l", 0x4000, 0x1000, 0x48b86bb0 )       /* background graphics */
	ROM_LOAD( "mp-e.3k", 0x5000, 0x1000, 0x48d8cace )
	ROM_LOAD( "mp-e.3h", 0x6000, 0x1000, 0x89ce19a8 )

	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mp-snd.1a", 0xf000, 0x1000, 0x506d76fb )
ROM_END

ROM_START( mpatrolw_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mpw-a.3m", 0x0000, 0x1000, 0x138439c6 )
	ROM_LOAD( "mpw-a.3l", 0x1000, 0x1000, 0x0c1bc43b )
	ROM_LOAD( "mpw-a.3k", 0x2000, 0x1000, 0x56b4c738 )
	ROM_LOAD( "mpw-a.3j", 0x3000, 0x1000, 0x9e598a75 )

	ROM_REGION(0x7000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mpw-e.3e", 0x0000, 0x1000, 0xc8e818a2 )       /* chars */
	ROM_LOAD( "mpw-e.3f", 0x1000, 0x1000, 0x080d9163 )
	ROM_LOAD( "mp-b.3m",  0x2000, 0x1000, 0xfe518a23 )       /* sprites */
	ROM_LOAD( "mp-b.3n",  0x3000, 0x1000, 0x974b35c3 )
	ROM_LOAD( "mp-e.3l",  0x4000, 0x1000, 0x48b86bb0 )       /* background graphics */
	ROM_LOAD( "mp-e.3k",  0x5000, 0x1000, 0x48d8cace )
	ROM_LOAD( "mp-e.3h",  0x6000, 0x1000, 0x89ce19a8 )

	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mp-snd.1a", 0xf000, 0x1000, 0x506d76fb )
ROM_END

ROM_START( mranger_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mp-a.3m", 0x0000, 0x1000, 0x0440639c )
	ROM_LOAD( "mr-a.3l", 0x1000, 0x1000, 0xf25e07f8 )
	ROM_LOAD( "mr-a.3k", 0x2000, 0x1000, 0x6e686d92 )
	ROM_LOAD( "mr-a.3j", 0x3000, 0x1000, 0x39412cd3 )

	ROM_REGION(0x7000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mp-e.3e", 0x0000, 0x1000, 0xefe9bb1d )       /* chars */
	ROM_LOAD( "mp-e.3f", 0x1000, 0x1000, 0x796d8525 )
	ROM_LOAD( "mp-b.3m", 0x2000, 0x1000, 0xfe518a23 )       /* sprites */
	ROM_LOAD( "mp-b.3n", 0x3000, 0x1000, 0x974b35c3 )
	ROM_LOAD( "mp-e.3l", 0x4000, 0x1000, 0x48b86bb0 )       /* background graphics */
	ROM_LOAD( "mp-e.3k", 0x5000, 0x1000, 0x48d8cace )
	ROM_LOAD( "mp-e.3h", 0x6000, 0x1000, 0x89ce19a8 )

	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mp-snd.1a", 0xf000, 0x1000, 0x506d76fb )
ROM_END



static int hiload(void)
{
	static int loop = 0;


	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (loop == 0)
	{
		memset(&RAM[0x0e008],0xff,44);
		loop = 1;
	}

	if (RAM[0x0e008] == 0 && RAM[0x0e04b] == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0e008],44);
			osd_fclose(f);
		}

		loop = 0;
		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;

	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0e008],44);
		osd_fclose(f);
	}
}



struct GameDriver mpatrol_driver =
{
	__FILE__,
	0,
	"mpatrol",
	"Moon Patrol",
	"1982",
	"Irem",
	"Nicola Salmoria\nChris Hardy\nValerio Verrando\nTim Lindquist (color info)\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,

	mpatrol_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mpatrol_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver mpatrolw_driver =
{
	__FILE__,
	&mpatrol_driver,
	"mpatrolw",
	"Moon Patrol (Williams)",
	"1982",
	"Irem (Williams license)",
	"Nicola Salmoria\nChris Hardy\nValerio Verrando\nTim Lindquist (color info)\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,

	mpatrolw_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mpatrolw_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver mranger_driver =
{
	__FILE__,
	&mpatrol_driver,
	"mranger",
	"Moon Ranger",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nChris Hardy (hardware info)\nTim Lindquist (color info)\nAaron Giles (sound)\nValerio Verrando (high score save)\nMarco Cassili",
	0,
	&machine_driver,

	mranger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mpatrol_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
