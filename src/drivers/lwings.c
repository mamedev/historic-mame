/***************************************************************************

  Legendary Wings
  Section Z
  Trojan

  Driver provided by Paul Leaman (paull@vortexcomputing.demon.co.uk)

  Please do not send anything large to this address without asking me
  first.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void lwings_bankswitch_w(int offset,int data);
void lwings_paletteram_w(int offset,int data);
int lwings_bankedrom_r(int offset);
int lwings_interrupt(void);

extern unsigned char *lwings_paletteram;
extern int lwings_paletteram_size;
extern unsigned char *lwings_backgroundram;
extern unsigned char *lwings_backgroundattribram;
extern int lwings_backgroundram_size;
extern unsigned char *lwings_scrolly;
extern unsigned char *lwings_scrollx;
extern unsigned char *lwings_palette_bank;
void lwings_background_w(int offset,int data);
void lwings_backgroundattrib_w(int offset,int data);
void lwings_palette_bank_w(int offset,int data);
int  lwings_vh_start(void);
void lwings_vh_stop(void);
void lwings_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *trojan_scrolly;
extern unsigned char *trojan_scrollx;
extern unsigned char *trojan_bk_scrolly;
extern unsigned char *trojan_bk_scrollx;
int  trojan_vh_start(void);
void trojan_vh_stop(void);
void trojan_vh_screenrefresh(struct osd_bitmap *bitmap);

/*
Call this function from somewhere to trigger the ADPCM. I can't find
the output port.
*/

void trojan_adpcm_w(int offset, int data)
{
    ADPCM_trigger(0, data&0x07);
}



static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x7fff, MRA_ROM },   /* CODE */
        { 0x8000, 0xbfff, MRA_BANK1 },  /* CODE */
        { 0xc000, 0xdfff, MRA_RAM },
        { 0xe000, 0xf7ff, MRA_RAM },
        { 0xf808, 0xf808, input_port_0_r },
        { 0xf809, 0xf809, input_port_1_r },
        { 0xf80a, 0xf80a, input_port_2_r },
        { 0xf80b, 0xf80b, input_port_3_r },
        { 0xf80c, 0xf80c, input_port_4_r },

	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0xc000, 0xddff, MWA_RAM },
        { 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
        { 0xe400, 0xe7ff, colorram_w, &colorram },
        { 0xe800, 0xebff, lwings_background_w, &lwings_backgroundram, &lwings_backgroundram_size },
        { 0xec00, 0xefff, lwings_backgroundattrib_w, &lwings_backgroundattribram },
        { 0xde00, 0xdf7f, MWA_RAM, &spriteram, &spriteram_size },
        { 0xf000, 0xf7ff, lwings_paletteram_w, &lwings_paletteram, &lwings_paletteram_size },
        { 0xf800, 0xf801, MWA_RAM, &trojan_scrollx },
        { 0xf802, 0xf803, MWA_RAM, &trojan_scrolly },
        { 0xf804, 0xf804, MWA_RAM, &trojan_bk_scrollx },
        { 0xf805, 0xf805, MWA_RAM, &trojan_bk_scrolly },
        { 0xf80c, 0xf80c, soundlatch_w },
        { 0xf80e, 0xf80e, lwings_bankswitch_w },
        { 0xf808, 0xf809, MWA_RAM, &lwings_scrolly},
        { 0xf80a, 0xf80b, MWA_RAM, &lwings_scrollx},
        { 0xf80d, 0xf80d, MWA_RAM }, /* Watchdog (same as lower byte scroll y) */
        { 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
        { 0xc000, 0xc7ff, MRA_RAM },
        { 0xc800, 0xc800, soundlatch_r },
        { 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
        { 0xc000, 0xc7ff, MWA_RAM },
        { 0xe000, 0xe000, AY8910_control_port_0_w },
        { 0xe001, 0xe001, AY8910_write_port_0_w },
        { 0xe002, 0xe002, AY8910_control_port_1_w },
        { 0xe003, 0xe003, AY8910_write_port_1_w },
        { 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( sectionz_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Screen Inversion", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x01, "Yes" )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x38, 0x38, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "20000 50000" )
	PORT_DIPSETTING(    0x18, "20000 60000" )
	PORT_DIPSETTING(    0x28, "20000 70000" )
	PORT_DIPSETTING(    0x08, "30000 60000" )
	PORT_DIPSETTING(    0x30, "30000 70000" )
	PORT_DIPSETTING(    0x10, "30000 80000" )
	PORT_DIPSETTING(    0x20, "40000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*	PORT_DIPSETTING(    0x80, "???" )	probably unused */
	PORT_DIPSETTING(    0xc0, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( lwings_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Unknown 1/2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy?" )
	PORT_DIPSETTING(    0x06, "Normal?" )
	PORT_DIPSETTING(    0x04, "Difficult?" )
	PORT_DIPSETTING(    0x00, "Very Difficult?" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPNAME( 0xe0, 0xe0, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0xe0, "20000 50000" )
	PORT_DIPSETTING(    0x60, "20000 60000" )
	PORT_DIPSETTING(    0xa0, "20000 70000" )
	PORT_DIPSETTING(    0x20, "30000 60000" )
	PORT_DIPSETTING(    0xc0, "30000 70000" )
	PORT_DIPSETTING(    0x40, "30000 80000" )
	PORT_DIPSETTING(    0x80, "40000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
INPUT_PORTS_END

INPUT_PORTS_START( trojan_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright 1 Joystick" )
	PORT_DIPSETTING(    0x02, "Upright 2 Joysticks" )
	PORT_DIPSETTING(    0x03, "Cocktail" )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x0c, "20000 70000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x1c, "30000 60000" )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x14, "30000 80000" )
	PORT_DIPSETTING(    0x04, "40000 80000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xe0, 0xe0, "Starting Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
/* 0x00 and 0x20 start at level 6 */

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Yes" )
	PORT_DIPSETTING(    0x00, "No" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
        1024,   /* 1024 characters */
	2,	/* 2 bits per pixel */
        { 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
        2048,   /* 2048 tiles */
        4,      /* 4 bits per pixel */
        { 0x30000*8, 0x00000*8, 0x10000*8, 0x20000*8  },  /* the bitplanes are separated */
        { 0, 1, 2, 3, 4, 5, 6, 7,
            16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
        1024,   /* 1024 sprites */
	4,	/* 4 bits per pixel */
        { 0x10000*8+4, 0x10000*8+0, 4, 0 },
        { 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
	    32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};




static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /*   start    pointer     colour start   number of colours */
        { 1, 0x00000, &charlayout,           0,   16 },
        { 1, 0x10000, &tilelayout,        16*4,    8 },
        { 1, 0x50000, &spritelayout, 16*4+8*16,    8 },
 	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(100,0x20ff), YM2203_VOL(100,0x20ff) },
	{ 0 },
	{ 0 },
        { 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
                        lwings_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
        32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
        256,               /* 256 colours */
        8*16+16*4+8*16,   /* Colour table length (tiles+char+sprites) */
        0,

        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
        lwings_vh_start,
        lwings_vh_stop,
        lwings_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( lwings_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_lw01.bin",  0x00000, 0x8000, 0x664f6939 )
	ROM_LOAD( "7c_lw02.bin",  0x10000, 0x8000, 0x5506f9b8 )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, 0x45a255a0 )

        ROM_REGION(0x70000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9h_lw05.bin", 0x00000, 0x4000, 0x0bf1930f )  /* characters */
	ROM_LOAD( "3b_lw12.bin", 0x10000, 0x8000, 0xbebb9519 )     /* tiles */
	ROM_LOAD( "1b_lw06.bin", 0x18000, 0x8000, 0x2bd30a6f )     /* tiles */
	ROM_LOAD( "3d_lw13.bin", 0x20000, 0x8000, 0x4dd132db )     /* tiles */
	ROM_LOAD( "1d_lw07.bin", 0x28000, 0x8000, 0x5fee27d2 )     /* tiles */
	ROM_LOAD( "3e_lw14.bin", 0x30000, 0x8000, 0xf796da02 )     /* tiles */
	ROM_LOAD( "1e_lw08.bin", 0x38000, 0x8000, 0xcc211065 )     /* tiles */
	ROM_LOAD( "3f_lw15.bin", 0x40000, 0x8000, 0x27502d44 )     /* tiles */
	ROM_LOAD( "1f_lw09.bin", 0x48000, 0x8000, 0xe7f59523 )     /* tiles */
	ROM_LOAD( "3j_lw17.bin", 0x50000, 0x8000, 0xabcd8c3f )     /* sprites */
	ROM_LOAD( "1j_lw11.bin", 0x58000, 0x8000, 0x7f560e0a )     /* sprites */
	ROM_LOAD( "3h_lw16.bin", 0x60000, 0x8000, 0xfe4fb851 )     /* sprites */
	ROM_LOAD( "1h_lw10.bin", 0x68000, 0x8000, 0xbce45b9e )     /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0x314df447 )
ROM_END

ROM_START( lwingsjp_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "a_06c.rom",  0x00000, 0x8000, 0x32da996c )
	ROM_LOAD( "a_07c.rom",  0x10000, 0x8000, 0x5717e44b )
	ROM_LOAD( "a_09c.rom",  0x18000, 0x8000, 0x45a255a0 )

	ROM_REGION(0x70000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_09h.rom", 0x00000, 0x4000, 0x0bf1930f )  /* characters */
	ROM_LOAD( "b_03b.rom", 0x10000, 0x8000, 0x97c116c3 )     /* tiles */
	ROM_LOAD( "b_01b.rom", 0x18000, 0x8000, 0x31c18a63 )     /* tiles */
	ROM_LOAD( "b_03d.rom", 0x20000, 0x8000, 0x346ad050 )     /* tiles */
	ROM_LOAD( "b_01d.rom", 0x28000, 0x8000, 0xadc849d6 )     /* tiles */
	ROM_LOAD( "b_03e.rom", 0x30000, 0x8000, 0x017ac406 )     /* tiles */
	ROM_LOAD( "b_01e.rom", 0x38000, 0x8000, 0xc34943cf )     /* tiles */
	ROM_LOAD( "b_03f.rom", 0x40000, 0x8000, 0xb2962e04 )     /* tiles */
	ROM_LOAD( "b_01f.rom", 0x48000, 0x8000, 0x871dc5eb )     /* tiles */
	ROM_LOAD( "b_03j.rom", 0x50000, 0x8000, 0x6c24efb0 )     /* sprites */
	ROM_LOAD( "b_01j.rom", 0x58000, 0x8000, 0xf3715fe3 )     /* sprites */
	ROM_LOAD( "b_03h.rom", 0x60000, 0x8000, 0x17d60134 )     /* sprites */
	ROM_LOAD( "b_01h.rom", 0x68000, 0x8000, 0xd92720a7 )     /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a_11e.rom", 0x0000, 0x8000, 0x314df447 )
ROM_END



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xce00],"\x00\x30\x00",3) == 0 &&
                        memcmp(&RAM[0xce4e],"\x00\x08\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xce00],13*7);
			RAM[0xce97] = RAM[0xce00];
			RAM[0xce98] = RAM[0xce01];
			RAM[0xce99] = RAM[0xce02];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xce00],13*7);
		osd_fclose(f);
	}
}



struct GameDriver lwings_driver =
{
	"Legendary Wings",
	"lwings",
	"Paul Leaman\nMarco Cassili (dip switches)",
	&machine_driver,

	lwings_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	lwings_input_ports,

	NULL, 0, 0,
	ORIENTATION_ROTATE_270,
	hiload, hisave
};

struct GameDriver lwingsjp_driver =
{
	"Legendary Wings (Japanese)",
	"lwingsjp",
	"Paul Leaman\nMarco Cassili (dip switches)",
	&machine_driver,

	lwingsjp_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	lwings_input_ports,

	NULL, 0, 0,
	ORIENTATION_ROTATE_270,
	hiload, hisave
};

/***************************************************************

 Section Z
 =========

   Exactly the same hardware as legendary wings, apart from the
   graphics orientation.

***************************************************************/


ROM_START( sectionz_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_sz01.bin",  0x00000, 0x8000, 0x0ad0dfbe )
	ROM_LOAD( "7c_sz02.bin",  0x10000, 0x8000, 0xee7e960e )
	ROM_LOAD( "9c_sz03.bin",  0x18000, 0x8000, 0x7d3980a1 )

	ROM_REGION(0x70000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9h_sz05.bin", 0x00000, 0x4000, 0xb8520000 )     /* characters */
	ROM_LOAD( "3b_SZ12.bin", 0x10000, 0x8000, 0x8081a4e9 )     /* tiles */
	ROM_LOAD( "1b_SZ06.bin", 0x18000, 0x8000, 0x0324323a )     /* tiles */
	ROM_LOAD( "3d_SZ13.bin", 0x20000, 0x8000, 0xb5d7a0ad )     /* tiles */
	ROM_LOAD( "1d_SZ07.bin", 0x28000, 0x8000, 0x031e915a )     /* tiles */
	ROM_LOAD( "3e_SZ14.bin", 0x30000, 0x8000, 0xd40d5373 )     /* tiles */
	ROM_LOAD( "1e_SZ08.bin", 0x38000, 0x8000, 0xdfcbf461 )     /* tiles */
	ROM_LOAD( "3f_SZ15.bin", 0x40000, 0x8000, 0xa332ea3c )     /* tiles */
	ROM_LOAD( "1f_SZ09.bin", 0x48000, 0x8000, 0x27b80a1c )     /* tiles */
	ROM_LOAD( "3j_sz17.bin", 0x50000, 0x8000, 0x1a7d3f2b )     /* sprites */
	ROM_LOAD( "1j_sz11.bin", 0x58000, 0x8000, 0xfd420ebc )     /* sprites */
	ROM_LOAD( "3h_sz16.bin", 0x60000, 0x8000, 0x3a32ae7c )     /* sprites */
	ROM_LOAD( "1h_sz10.bin", 0x68000, 0x8000, 0x750adac0 )     /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, 0x44b6a7dc )
ROM_END

struct GameDriver sectionz_driver =
{
	"Section Z",
	"sectionz",
	"Paul Leaman\nMarco Cassili (dip switches)",
	&machine_driver,

	sectionz_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sectionz_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};


/***************************************************************

 Trojan
 ======

   Similar to Legendary Wings apart from:
   1) More sprites
   2) 3rd Z80
   3) Different palette layout
   4) Third Background tile layer

***************************************************************/


static struct GfxLayout spritelayout_trojan = /* LWings, with more sprites */
{
	16,16,	/* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 0x20000*8+4, 0x20000*8+0, 4, 0},
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout bktilelayout_trojan = /* LWings, with more sprites */
{
	16,16,	/* 16*16 sprites */
	512,   /* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0x08000*8+4, 0x08000*8+0, 4, 0},
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo_trojan[] =
{
	/*   start    pointer     colour start    number of colours */
	{ 1, 0x00000, &charlayout,                        0, 16 },
	{ 1, 0x10000, &tilelayout,                     16*4,  8 },
	{ 1, 0x50000, &spritelayout_trojan,       16*4+8*16,  8 },
	{ 1, 0x90000, &bktilelayout_trojan,  16*4+8*16+8*16,  8 },
	{ -1 } /* end of array */
};


/*
ADPCM is driven by Z80 continuously outputting to a port. The following
table is lifted from the code.

Sample 5 doesn't play properly.

*/

ADPCM_SAMPLES_START(trojan_samples)
	ADPCM_SAMPLE(0x01, 0x00a7, (0x0aa9-0x00a7)*2 )
	ADPCM_SAMPLE(0x02, 0x0aa9, (0x12ab-0x0aa9)*2 )
	ADPCM_SAMPLE(0x03, 0x12ab, (0x17ad-0x12ab)*2 )
	ADPCM_SAMPLE(0x04, 0x17ad, (0x22af-0x17ad)*2 )
	ADPCM_SAMPLE(0x05, 0x22af, (0x2db1-0x22af)*2 )
	ADPCM_SAMPLE(0x06, 0x2db1, (0x310a-0x2db1)*2 )
	ADPCM_SAMPLE(0x07, 0x310a, (0x3cb3-0x310a)*2 )
ADPCM_SAMPLES_END

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	4000,       /* 4000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};



static struct MachineDriver machine_driver_trojan =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			lwings_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo_trojan,
	256,               /* 256 colours */
	8*16+16*4+8*16+8*16,   /* Colour table length (tiles+char+sprites) */
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	trojan_vh_start,
	trojan_vh_stop,
	trojan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface,
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

ROM_START( trojan_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "TB04.bin",  0x00000, 0x8000, 0x0d14b378 )
	ROM_LOAD( "TB06.bin",  0x10000, 0x8000, 0xf790b4a2 )
	ROM_LOAD( "TB05.bin",  0x18000, 0x8000, 0x8c67a865 )

	ROM_REGION(0xa0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "TB03.bin",   0x00000, 0x4000, 0x6f676329 )  /* characters */
	ROM_LOAD( "TB11.bin",   0x10000, 0x8000, 0x689f2af9 )     /* tiles */
	ROM_LOAD( "TB07.bin",   0x18000, 0x8000, 0x167327d9 )     /* tiles */
	ROM_LOAD( "TB12.bin",   0x20000, 0x8000, 0xca0be2e1 )     /* tiles */
	ROM_LOAD( "TB08.bin",   0x28000, 0x8000, 0x67b20808 )     /* tiles */
	ROM_LOAD( "TB13.bin",   0x30000, 0x8000, 0x488526a9 )     /* tiles */
	ROM_LOAD( "TB09.bin",   0x38000, 0x8000, 0x941934f9 )     /* tiles */
	ROM_LOAD( "TB14.bin",   0x40000, 0x8000, 0x343d4711 )     /* tiles */
	ROM_LOAD( "TB10.bin",   0x48000, 0x8000, 0x7ebd6a7d )     /* tiles */
	ROM_LOAD( "TB18.bin",   0x50000, 0x8000, 0xcbeaef2c )     /* sprites */
	ROM_LOAD( "TB16.bin",   0x58000, 0x8000, 0xb276f758 )     /* sprites */
	ROM_LOAD( "TB17.bin",   0x60000, 0x8000, 0xc7ad3a3d )     /* sprites */
	ROM_LOAD( "TB15.bin",   0x68000, 0x8000, 0x9db50319 )     /* sprites */
	ROM_LOAD( "TB22.bin",   0x70000, 0x8000, 0xa28832a2 )     /* sprites */
	ROM_LOAD( "TB20.bin",   0x78000, 0x8000, 0x0156ce22 )     /* sprites */
	ROM_LOAD( "TB21.bin",   0x80000, 0x8000, 0x3c8ece14 )     /* sprites */
	ROM_LOAD( "TB19.bin",   0x88000, 0x8000, 0x5034e812 )     /* sprites */
	ROM_LOAD( "TB25.BIN",   0x90000, 0x8000, 0x6592283e )    /* Bk Tiles */
	ROM_LOAD( "TB24.BIN",   0x98000, 0x8000, 0x24efb007 )    /* Bk Tiles */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "TB02.bin", 0x0000, 0x8000, 0x04af8521 )

	ROM_REGION(0x08000)     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "TB01.bin", 0x0000, 0x8000, 0xbc22d83e )

	ROM_REGION(0x08000)
	ROM_LOAD( "TB23.BIN", 0x00000, 0x08000, 0x4e586e86 )  /* Tile Map */
ROM_END

ROM_START( trojanj_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "Troj-04.rom",  0x00000, 0x8000, 0xa3d48798 )
	ROM_LOAD( "Troj-06.rom",  0x10000, 0x8000, 0x3d7a2112 )
	ROM_LOAD( "Troj-05.rom",  0x18000, 0x8000, 0x8c67a865 )

	ROM_REGION(0xa0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "Troj-03.rom",   0x00000, 0x4000, 0x6f676329 )  /* characters */
	ROM_LOAD( "Troj-11.rom",   0x10000, 0x8000, 0x689f2af9 )     /* tiles */
	ROM_LOAD( "Troj-07.rom",   0x18000, 0x8000, 0x167327d9 )     /* tiles */
	ROM_LOAD( "Troj-12.rom",   0x20000, 0x8000, 0xca0be2e1 )     /* tiles */
	ROM_LOAD( "Troj-08.rom",   0x28000, 0x8000, 0x67b20808 )     /* tiles */
	ROM_LOAD( "Troj-13.rom",   0x30000, 0x8000, 0x488526a9 )     /* tiles */
	ROM_LOAD( "Troj-09.rom",   0x38000, 0x8000, 0x941934f9 )     /* tiles */
	ROM_LOAD( "Troj-14.rom",   0x40000, 0x8000, 0x343d4711 )     /* tiles */
	ROM_LOAD( "Troj-10.rom",   0x48000, 0x8000, 0x7ebd6a7d )     /* tiles */
	ROM_LOAD( "Troj-18.rom",   0x50000, 0x8000, 0xcbeaef2c )     /* sprites */
	ROM_LOAD( "Troj-16.rom",   0x58000, 0x8000, 0xb276f758 )     /* sprites */
	ROM_LOAD( "Troj-17.rom",   0x60000, 0x8000, 0xc7ad3a3d )     /* sprites */
	ROM_LOAD( "Troj-15.rom",   0x68000, 0x8000, 0x9db50319 )     /* sprites */
	ROM_LOAD( "Troj-22.rom",   0x70000, 0x8000, 0xa28832a2 )     /* sprites */
	ROM_LOAD( "Troj-20.rom",   0x78000, 0x8000, 0x0156ce22 )     /* sprites */
	ROM_LOAD( "Troj-21.rom",   0x80000, 0x8000, 0x3c8ece14 )     /* sprites */
	ROM_LOAD( "Troj-19.rom",   0x88000, 0x8000, 0x5034e812 )     /* sprites */
	ROM_LOAD( "Troj-25.rom",   0x90000, 0x8000, 0x6592283e )    /* Bk Tiles */
	ROM_LOAD( "Troj-24.rom",   0x98000, 0x8000, 0x24efb007 )    /* Bk Tiles */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "Troj-02.rom", 0x0000, 0x8000, 0x04af8521 )

	ROM_REGION(0x08000)     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "Troj-01.rom", 0x0000, 0x4000, 0xdc22d83e )

	ROM_REGION(0x08000)
	ROM_LOAD( "Troj-23.rom", 0x00000, 0x08000, 0x4e586e86 )  /* Tile Map */
ROM_END


struct GameDriver trojan_driver =
{
	"Trojan",
	"trojan",
	"Paul Leaman\nPhil Stroffolino\nMarco Cassili (dip switches)",
	&machine_driver_trojan,

	trojan_rom,
	0, 0,
	0,
	(void *)trojan_samples, /* sound_prom */

	trojan_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver trojanj_driver =
{
	"Trojan (Japanese)",
	"trojanj",
	"Paul Leaman\nPhil Stroffolino\nMarco Cassili (dip switches)",
	&machine_driver_trojan,

	trojanj_rom,
	0, 0,
	0,
	(void *)trojan_samples, /* sound_prom */

	trojan_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

