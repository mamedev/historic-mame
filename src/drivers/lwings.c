/***************************************************************************

  Legendary Wings
  Section Z
  Trojan

  Driver provided by Paul Leaman (paull@vortexcomputing.demon.co.uk)

  Please do not send anything large to this address without asking me
  first.

  Trojan contains a third Z80 to drive the game samples. This third
  Z80 outputs the ADPCM data byte at a time to the sound hardware. Since
  this will be expensive to do this extra processor is not emulated.

  Instead, the ADPCM data is lifted directly from the sound ROMS.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void lwings_bankswitch_w(int offset,int data);
int lwings_bankedrom_r(int offset);
int lwings_interrupt(void);

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
void lwings_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char *trojan_scrolly;
extern unsigned char *trojan_scrollx;
extern unsigned char *trojan_bk_scrolly;
extern unsigned char *trojan_bk_scrollx;
int  trojan_vh_start(void);
void trojan_vh_stop(void);
void trojan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void trojan_sound_cmd_w(int offset, int data)
{
       soundlatch_w(offset, data);
       if (data != 0xff && (data & 0x08))
       {
              /*
              I assume that Trojan's ADPCM output is directly derived
              from the sound code. I can't find an output port that
              does this on either the sound board or main board.
              */
              ADPCM_trigger(0, data & 0x07);
        }

#if 0
        if (errorlog)
        {
               fprintf(errorlog, "Sound Code=%02x\n", data);
        }
#endif
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },   /* CODE */
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* CODE */
	{ 0xc000, 0xf7ff, MRA_RAM },
	{ 0xf808, 0xf808, input_port_0_r },
	{ 0xf809, 0xf809, input_port_1_r },
	{ 0xf80a, 0xf80a, input_port_2_r },
	{ 0xf80b, 0xf80b, input_port_3_r },
	{ 0xf80c, 0xf80c, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xddff, MWA_RAM },
	{ 0xde00, 0xdfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xebff, lwings_background_w, &lwings_backgroundram, &lwings_backgroundram_size },
	{ 0xec00, 0xefff, lwings_backgroundattrib_w, &lwings_backgroundattribram },
	{ 0xf000, 0xf3ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0xf400, 0xf7ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0xf800, 0xf801, MWA_RAM, &trojan_scrollx },
	{ 0xf802, 0xf803, MWA_RAM, &trojan_scrolly },
	{ 0xf804, 0xf804, MWA_RAM, &trojan_bk_scrollx },
	{ 0xf805, 0xf805, MWA_RAM, &trojan_bk_scrolly },
	{ 0xf808, 0xf809, MWA_RAM, &lwings_scrolly},
	{ 0xf80a, 0xf80b, MWA_RAM, &lwings_scrollx},
	{ 0xf80c, 0xf80c, soundlatch_w },
	{ 0xf80d, 0xf80d, watchdog_reset_w },
	{ 0xf80e, 0xf80e, lwings_bankswitch_w },
	{ -1 }	/* end of table */
};

/* TROJAN - intercept sound command write for ADPCM samples */

static struct MemoryWriteAddress trojan_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xddff, MWA_RAM },
	{ 0xde00, 0xdfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xebff, lwings_background_w, &lwings_backgroundram, &lwings_backgroundram_size },
	{ 0xec00, 0xefff, lwings_backgroundattrib_w, &lwings_backgroundattribram },
	{ 0xf000, 0xf3ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0xf400, 0xf7ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0xf800, 0xf801, MWA_RAM, &trojan_scrollx },
	{ 0xf802, 0xf803, MWA_RAM, &trojan_scrolly },
	{ 0xf804, 0xf804, MWA_RAM, &trojan_bk_scrollx },
	{ 0xf805, 0xf805, MWA_RAM, &trojan_bk_scrolly },
	{ 0xf808, 0xf809, MWA_RAM, &lwings_scrolly},
	{ 0xf80a, 0xf80b, MWA_RAM, &lwings_scrollx},
	{ 0xf80c, 0xf80c, trojan_sound_cmd_w },
	{ 0xf80d, 0xf80d, watchdog_reset_w },
	{ 0xf80e, 0xf80e, lwings_bankswitch_w },
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
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright 1 Joystick" )
	PORT_DIPSETTING(    0x02, "Upright 2 Joysticks" )
	PORT_DIPSETTING(    0x03, "Cocktail" )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, "Bonus Life", IP_KEY_NONE )
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
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
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
	{ 0x30000*8, 0x20000*8, 0x10000*8, 0x00000*8  },  /* the bitplanes are separated */
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
	{ 1, 0x00000, &charlayout,   512, 16 },	/* colors 512-575 */
	{ 1, 0x10000, &tilelayout,     0,  8 },	/* colors   0-127 */
	{ 1, 0x50000, &spritelayout, 384,  8 },	/* colors 384-511 */
 	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(100,255), YM2203_VOL(100,255) },
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
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
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
	ROM_LOAD( "6c_lw01.bin", 0x00000, 0x8000, 0x664f6939 )
	ROM_LOAD( "7c_lw02.bin", 0x10000, 0x8000, 0x5506f9b8 )
	ROM_LOAD( "9c_lw03.bin", 0x18000, 0x8000, 0x45a255a0 )

	ROM_REGION(0x70000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9h_lw05.bin", 0x00000, 0x4000, 0x0bf1930f )	/* characters */
	ROM_LOAD( "3e_lw14.bin", 0x10000, 0x8000, 0xf796da02 )	/* tiles */
	ROM_LOAD( "1e_lw08.bin", 0x18000, 0x8000, 0xcc211065 )
	ROM_LOAD( "3d_lw13.bin", 0x20000, 0x8000, 0x4dd132db )
	ROM_LOAD( "1d_lw07.bin", 0x28000, 0x8000, 0x5fee27d2 )
	ROM_LOAD( "3b_lw12.bin", 0x30000, 0x8000, 0xbebb9519 )
	ROM_LOAD( "1b_lw06.bin", 0x38000, 0x8000, 0x2bd30a6f )
	ROM_LOAD( "3f_lw15.bin", 0x40000, 0x8000, 0x27502d44 )
	ROM_LOAD( "1f_lw09.bin", 0x48000, 0x8000, 0xe7f59523 )
	ROM_LOAD( "3j_lw17.bin", 0x50000, 0x8000, 0xabcd8c3f )	/* sprites */
	ROM_LOAD( "1j_lw11.bin", 0x58000, 0x8000, 0x7f560e0a )
	ROM_LOAD( "3h_lw16.bin", 0x60000, 0x8000, 0xfe4fb851 )
	ROM_LOAD( "1h_lw10.bin", 0x68000, 0x8000, 0xbce45b9e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0x314df447 )
ROM_END

ROM_START( lwingsjp_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "a_06c.rom",   0x00000, 0x8000, 0x32da996c )
	ROM_LOAD( "a_07c.rom",   0x10000, 0x8000, 0x5717e44b )
	ROM_LOAD( "9c_lw03.bin", 0x18000, 0x8000, 0x45a255a0 )

	ROM_REGION(0x70000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9h_lw05.bin", 0x00000, 0x4000, 0x0bf1930f )	/* characters */
	ROM_LOAD( "b_03e.rom",   0x10000, 0x8000, 0x017ac406 )	/* tiles */
	ROM_LOAD( "b_01e.rom",   0x18000, 0x8000, 0xc34943cf )
	ROM_LOAD( "b_03d.rom",   0x20000, 0x8000, 0x346ad050 )
	ROM_LOAD( "b_01d.rom",   0x28000, 0x8000, 0xadc849d6 )
	ROM_LOAD( "b_03b.rom",   0x30000, 0x8000, 0x97c116c3 )
	ROM_LOAD( "b_01b.rom",   0x38000, 0x8000, 0x31c18a63 )
	ROM_LOAD( "b_03f.rom",   0x40000, 0x8000, 0xb2962e04 )
	ROM_LOAD( "b_01f.rom",   0x48000, 0x8000, 0x871dc5eb )
	ROM_LOAD( "b_03j.rom",   0x50000, 0x8000, 0x6c24efb0 )	/* sprites */
	ROM_LOAD( "b_01j.rom",   0x58000, 0x8000, 0xf3715fe3 )
	ROM_LOAD( "b_03h.rom",   0x60000, 0x8000, 0x17d60134 )
	ROM_LOAD( "b_01h.rom",   0x68000, 0x8000, 0xd92720a7 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0x314df447 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


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
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xce00],13*7);
		osd_fclose(f);
	}
}



struct GameDriver lwings_driver =
{
	__FILE__,
	0,
	"lwings",
	"Legendary Wings (US)",
	"1986",
	"Capcom",
	"Paul Leaman\nMarco Cassili (dip switches)",
	0,
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
	__FILE__,
	&lwings_driver,
	"lwingsjp",
	"Legendary Wings (Japan)",
	"1986",
	"Capcom",
	"Paul Leaman\nMarco Cassili (dip switches)",
	0,
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
	ROM_LOAD( "9h_sz05.bin", 0x00000, 0x4000, 0xb8520000 )	/* characters */
	ROM_LOAD( "3e_SZ14.bin", 0x10000, 0x8000, 0xd40d5373 )	/* tiles */
	ROM_LOAD( "1e_SZ08.bin", 0x18000, 0x8000, 0xdfcbf461 )
	ROM_LOAD( "3d_SZ13.bin", 0x20000, 0x8000, 0xb5d7a0ad )
	ROM_LOAD( "1d_SZ07.bin", 0x28000, 0x8000, 0x031e915a )
	ROM_LOAD( "3b_SZ12.bin", 0x30000, 0x8000, 0x8081a4e9 )
	ROM_LOAD( "1b_SZ06.bin", 0x38000, 0x8000, 0x0324323a )
	ROM_LOAD( "3f_SZ15.bin", 0x40000, 0x8000, 0xa332ea3c )
	ROM_LOAD( "1f_SZ09.bin", 0x48000, 0x8000, 0x27b80a1c )
	ROM_LOAD( "3j_sz17.bin", 0x50000, 0x8000, 0x1a7d3f2b )	/* sprites */
	ROM_LOAD( "1j_sz11.bin", 0x58000, 0x8000, 0xfd420ebc )
	ROM_LOAD( "3h_sz16.bin", 0x60000, 0x8000, 0x3a32ae7c )
	ROM_LOAD( "1h_sz10.bin", 0x68000, 0x8000, 0x750adac0 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, 0x44b6a7dc )
ROM_END

struct GameDriver sectionz_driver =
{
	__FILE__,
	0,
	"sectionz",
	"Section Z",
	"1985",
	"Capcom",
	"Paul Leaman\nMarco Cassili (dip switches)",
	0,
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
	{ 0x20000*8+4, 0x20000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout bktilelayout_trojan =
{
	16,16,	/* 16*16 sprites */
	512,   /* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0x08000*8+0, 0x08000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo_trojan[] =
{
	{ 1, 0x00000, &charlayout,          768, 16 },	/* colors 768-831 */
	{ 1, 0x10000, &tilelayout,          256,  8 },	/* colors 256-383 */
	{ 1, 0x50000, &spritelayout_trojan, 640,  8 },	/* colors 640-767 */
	{ 1, 0x90000, &bktilelayout_trojan,   0,  8 },	/* colors   0-127 */
	{ -1 } /* end of array */
};


/*
ADPCM is driven by Z80 continuously outputting to a port. The following
table is lifted from the code.

Sample 5 doesn't play properly.
*/

ADPCM_SAMPLES_START(trojan_samples)
	ADPCM_SAMPLE(0x00, 0x00a7, (0x0aa9-0x00a7)*2 )
	ADPCM_SAMPLE(0x01, 0x0aa9, (0x12ab-0x0aa9)*2 )
	ADPCM_SAMPLE(0x02, 0x12ab, (0x17ad-0x12ab)*2 )
	ADPCM_SAMPLE(0x03, 0x17ad, (0x22af-0x17ad)*2 )
	ADPCM_SAMPLE(0x04, 0x22af, (0x2db1-0x22af)*2 )
	ADPCM_SAMPLE(0x05, 0x2db1, (0x310a-0x2db1)*2 )
	ADPCM_SAMPLE(0x06, 0x310a, (0x3cb3-0x310a)*2 )
ADPCM_SAMPLES_END

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	4000,                   /* 4000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};



static struct MachineDriver trojan_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,trojan_writemem,0,0,
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
	60, 2500,	/* frames per second, vblank duration */
				/* hand tuned to get rid of sprite lag */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo_trojan,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
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
	ROM_LOAD( "TB04.bin", 0x00000, 0x8000, 0x0d14b378 )
	ROM_LOAD( "TB06.bin", 0x10000, 0x8000, 0xf790b4a2 )
	ROM_LOAD( "TB05.bin", 0x18000, 0x8000, 0x8c67a865 )

	ROM_REGION(0xa0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "TB03.bin", 0x00000, 0x4000, 0x6f676329 )	/* characters */
	ROM_LOAD( "TB13.bin", 0x10000, 0x8000, 0x488526a9 )	/* tiles */
	ROM_LOAD( "TB09.bin", 0x18000, 0x8000, 0x941934f9 )
	ROM_LOAD( "TB12.bin", 0x20000, 0x8000, 0xca0be2e1 )
	ROM_LOAD( "TB08.bin", 0x28000, 0x8000, 0x67b20808 )
	ROM_LOAD( "TB11.bin", 0x30000, 0x8000, 0x689f2af9 )
	ROM_LOAD( "TB07.bin", 0x38000, 0x8000, 0x167327d9 )
	ROM_LOAD( "TB14.bin", 0x40000, 0x8000, 0x343d4711 )
	ROM_LOAD( "TB10.bin", 0x48000, 0x8000, 0x7ebd6a7d )
	ROM_LOAD( "TB18.bin", 0x50000, 0x8000, 0xcbeaef2c )	/* sprites */
	ROM_LOAD( "TB16.bin", 0x58000, 0x8000, 0xb276f758 )
	ROM_LOAD( "TB17.bin", 0x60000, 0x8000, 0xc7ad3a3d )
	ROM_LOAD( "TB15.bin", 0x68000, 0x8000, 0x9db50319 )
	ROM_LOAD( "TB22.bin", 0x70000, 0x8000, 0xa28832a2 )
	ROM_LOAD( "TB20.bin", 0x78000, 0x8000, 0x0156ce22 )
	ROM_LOAD( "TB21.bin", 0x80000, 0x8000, 0x3c8ece14 )
	ROM_LOAD( "TB19.bin", 0x88000, 0x8000, 0x5034e812 )
	ROM_LOAD( "TB25.bin", 0x90000, 0x8000, 0x6592283e )	/* Bk Tiles */
	ROM_LOAD( "TB24.bin", 0x98000, 0x8000, 0x24efb007 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "TB02.bin", 0x0000, 0x8000, 0x04af8521 )

	ROM_REGION(0x08000)	/* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "TB01.bin", 0x0000, 0x4000, 0xdc22d83e )

	ROM_REGION(0x08000)
	ROM_LOAD( "TB23.bin", 0x00000, 0x08000, 0x4e586e86 )  /* Tile Map */
ROM_END

ROM_START( trojanj_rom )
	ROM_REGION(0x20000)     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "Troj-04.rom", 0x00000, 0x8000, 0xa3d48798 )
	ROM_LOAD( "Troj-06.rom", 0x10000, 0x8000, 0x3d7a2112 )
	ROM_LOAD( "TB05.bin", 0x18000, 0x8000, 0x8c67a865 )

	ROM_REGION(0xa0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "TB03.bin", 0x00000, 0x4000, 0x6f676329 )	/* characters */
	ROM_LOAD( "TB13.bin", 0x10000, 0x8000, 0x488526a9 )	/* tiles */
	ROM_LOAD( "TB09.bin", 0x18000, 0x8000, 0x941934f9 )
	ROM_LOAD( "TB12.bin", 0x20000, 0x8000, 0xca0be2e1 )
	ROM_LOAD( "TB08.bin", 0x28000, 0x8000, 0x67b20808 )
	ROM_LOAD( "TB11.bin", 0x30000, 0x8000, 0x689f2af9 )
	ROM_LOAD( "TB07.bin", 0x38000, 0x8000, 0x167327d9 )
	ROM_LOAD( "TB14.bin", 0x40000, 0x8000, 0x343d4711 )
	ROM_LOAD( "TB10.bin", 0x48000, 0x8000, 0x7ebd6a7d )
	ROM_LOAD( "TB18.bin", 0x50000, 0x8000, 0xcbeaef2c )	/* sprites */
	ROM_LOAD( "TB16.bin", 0x58000, 0x8000, 0xb276f758 )
	ROM_LOAD( "TB17.bin", 0x60000, 0x8000, 0xc7ad3a3d )
	ROM_LOAD( "TB15.bin", 0x68000, 0x8000, 0x9db50319 )
	ROM_LOAD( "TB22.bin", 0x70000, 0x8000, 0xa28832a2 )
	ROM_LOAD( "TB20.bin", 0x78000, 0x8000, 0x0156ce22 )
	ROM_LOAD( "TB21.bin", 0x80000, 0x8000, 0x3c8ece14 )
	ROM_LOAD( "TB19.bin", 0x88000, 0x8000, 0x5034e812 )
	ROM_LOAD( "TB25.bin", 0x90000, 0x8000, 0x6592283e )	/* Bk Tiles */
	ROM_LOAD( "TB24.bin", 0x98000, 0x8000, 0x24efb007 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "TB02.bin", 0x0000, 0x8000, 0x04af8521 )

	ROM_REGION(0x08000)	/* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "TB01.bin", 0x0000, 0x4000, 0xdc22d83e )

	ROM_REGION(0x08000)
	ROM_LOAD( "TB23.bin", 0x00000, 0x08000, 0x4e586e86 )  /* Tile Map */
ROM_END



static int trojan_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xce97],"\x00\x23\x60",3) == 0)
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
	else return 0;  /* we can't load the hi scores yet */
}

static void trojan_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xce00],13*7);
		osd_fclose(f);
	}
}



struct GameDriver trojan_driver =
{
	__FILE__,
	0,
	"trojan",
	"Trojan (US)",
	"1986",
	"Capcom (Romstar license)",
	"Paul Leaman\nPhil Stroffolino\nMarco Cassili (dip switches)",
	0,
	&trojan_machine_driver,

	trojan_rom,
	0, 0,
	0,
	(void *)trojan_samples, /* sound_prom */

	trojan_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	trojan_hiload, trojan_hisave
};

struct GameDriver trojanj_driver =
{
	__FILE__,
	&trojan_driver,
	"trojanj",
	"Trojan (Japan)",
	"1986",
	"Capcom",
	"Paul Leaman\nPhil Stroffolino\nMarco Cassili (dip switches)",
	0,
	&trojan_machine_driver,

	trojanj_rom,
	0, 0,
	0,
	(void *)trojan_samples, /* sound_prom */

	trojan_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	trojan_hiload, trojan_hisave
};
