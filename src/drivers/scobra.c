/***************************************************************************

Super Cobra memory map (preliminary)

0000-5fff ROM (Lost Tomb: 0000-6fff)
8000-87ff RAM
8800-8bff Video RAM
9000-90ff Object RAM
  9000-903f  screen attributes
  9040-905f  sprites
  9060-907f  bullets
  9080-90ff  unused?

read:
b000      Watchdog Reset
9800      IN0
9801      IN1
9802      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT
 * bit 1 : SHOOT 2 player 1
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : SHOOT 1 player 2 (TABLE only)
 * bit 2 : SHOOT 2 player 2 (TABLE only)
 * bit 1 : nr of lives  0 = 3  1 = 5
 * bit 0 : allow continue 0 = NO  1 = YES
*
 * IN2 (all bits are inverted)
 * bit 7 : protection check?
 * bit 6 : DOWN player 1
 * bit 5 : protection check?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/ (00 = 99 credits!)
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
a801      interrupt enable
a802      coin counter
a803      ? (POUT1)
a804      stars on
a805      ? (POUT2)
a806      screen vertical flip
a807      screen horizontal flip
a000      To AY-3-8910 port A (commands for the audio CPU)
a001      bit 3 = trigger interrupt on audio CPU

TODO:
	Need correct color PROMs for:
		Super Bond
		Anteater
		Lost Tomb
		Strategy X
		Armored Car

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galaxian_flipx_w(int offset,int data);
void galaxian_flipy_w(int offset,int data);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
int scramble_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int scramble_vh_interrupt(void);
void scramble_background_w(int offset, int data);	/* MJC 051297 */

int scramble_portB_r(int offset);
int frogger_portB_r(int offset);
void scramble_sh_irqtrigger_w(int offset,int data);

int rescue_vh_start(void);							/* MJC */
int minefield_vh_start(void);
int calipso_vh_start(void);

void scobra_init_machine(void)
{
	/* we must start with NMI interrupts disabled, otherwise some games */
	/* (e.g. Lost Tomb, Rescue) will not pass the startup test. */
	interrupt_enable_w(0,0);
}

int moonwar2_IN0_r (int offset)
{
	int sign;
	int delta;

	delta = readinputport(3);

	sign = (delta & 0x80) >> 3;
	delta &= 0x0f;

	return ((readinputport(0) & 0xe0) | delta | sign );
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x9000, 0x907f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x9800, 0x9800, input_port_0_r },	/* IN0 */
	{ 0x9801, 0x9801, input_port_1_r },	/* IN1 */
	{ 0x9802, 0x9802, input_port_2_r },	/* IN2 */
	{ 0xb000, 0xb000, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x903f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0xa000, 0xa000, soundlatch_w },
	{ 0xa001, 0xa001, scramble_sh_irqtrigger_w },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa803, 0xa803, scramble_background_w },
	{ 0xa804, 0xa804, galaxian_stars_w },
	{ 0xa806, 0xa806, galaxian_flipx_w },
	{ 0xa807, 0xa807, galaxian_flipy_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress stratgyx_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x9000, 0x93ff, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x9800, 0x9800, watchdog_reset_r},
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa004, 0xa004, input_port_2_r },	/* IN2 */
	{ 0xa008, 0xa008, input_port_1_r },	/* IN1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress stratgyx_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x883f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x8840, 0x885f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8860, 0x887f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x8880, 0x88ff, MWA_NOP},
	{ 0xa800, 0xa800, soundlatch_w },
	{ 0xa804, 0xa804, scramble_sh_irqtrigger_w },
	{ 0xb004, 0xb004, interrupt_enable_w },
//	{ 0xb002, 0xb002, galaxian_stars_w },
//	{ 0xb006, 0xb006, MRW_NOP },    /* writes to these two a lot, always 0 */
//	{ 0xb008, 0xb008, MRW_NOP },    /* writes to these two a lot, always 0 */
	{ 0xb00c, 0xb00c, galaxian_flipy_w },
	{ 0xb00e, 0xb00e, galaxian_flipx_w },
	{ 0xa80c, 0xa80c, scramble_background_w }, /* may be wrong! */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress hustler_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x9000, 0x907f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ 0xd000, 0xd000, input_port_0_r },	/* IN0 */
	{ 0xd008, 0xd008, input_port_1_r },	/* IN1 */
	{ 0xd010, 0xd010, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress hustler_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x903f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0xa802, 0xa802, galaxian_flipx_w },
	{ 0xa804, 0xa804, interrupt_enable_w },
	{ 0xa806, 0xa806, galaxian_flipy_w },
	{ 0xa80e, 0xa80e, MWA_NOP },	/* coin counters */
	{ 0xe000, 0xe000, soundlatch_w },
	{ 0xe008, 0xe008, scramble_sh_irqtrigger_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress moonwar2_readmem[] =
{
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x9000, 0x907f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x9800, 0x9800, moonwar2_IN0_r },	/* IN0 */
	{ 0x9801, 0x9801, input_port_1_r },	/* IN1 */
	{ 0x9802, 0x9802, input_port_2_r },	/* IN2 */
	{ 0xb000, 0xb000, watchdog_reset_r },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress hustler_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress hustler_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort hustler_sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort hustler_sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( scobra_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x01, "Yes" )
	PORT_DIPNAME( 0x02, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* identical to scobra apart from the number of lives */
INPUT_PORTS_START( scobrak_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x01, "Yes" )
	PORT_DIPNAME( 0x02, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( stratgyx_input_ports )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credit" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "99" )

	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN3 */
	PORT_DIPNAME( 0xff, 0x00, "stuff", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x80, "80" )
	PORT_DIPSETTING(    0xff, "ff" )
INPUT_PORTS_END

INPUT_PORTS_START( armorcar_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( moonwar2_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* the spinner */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) //
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) //

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "Free Play" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) //
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) //
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) //
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) //
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) //

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/4 Credits" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 - dummy port for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 25, 0, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( darkplnt_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tazmania_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( calipso_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) //
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY ) //
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) //
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) //
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY ) //
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) //
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) //

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Lives", IP_KEY_NONE ) //
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 ) //
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 ) //
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) //
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 ) //
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) //
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY ) //
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) //
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( anteater_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( rescue_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x02, 0x02, "Starting Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( minefld_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x02, 0x02, "Starting Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x02, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Coin A 1/1 Coin B 1/1" )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( losttomb_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "Free Play" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x00, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( superbon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "Free Play" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x06, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x00, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 9", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( hustler_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_BITX(    0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* it's just 1 pixel, but we use 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 17*8*8 },	/* point to letter "A" */
	{ 3, 0, 0, 0, 0, 0, 0 },	/* I "know" that this bit of the */
	{ 1*8 },						/* graphics ROMs is 1 */
	0	/* no use */
};
static struct GfxLayout armorcar_bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* 4*1 line, I think - 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2, 2, 0, 0, 0 },	/* I "know" that this bit of the */
	{ 8 },						/* graphics ROMs is 1 */
	0	/* no use */
};

static struct GfxLayout calipso_charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 1024*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout calipso_spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 256*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 1 },	/* 1 color code instead of 2, so all */
											/* shots will be yellow */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo armorcar_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &armorcar_bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo calipso_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &calipso_charlayout,     0, 8 },
	{ 1, 0x0000, &calipso_spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 1 },	/* 1 color code instead of 2, so all */
											/* shots will be yellow */
	{ -1 } /* end of array */
};



/* this is NOT the original color PROM - it's the Super Cobra one */
static unsigned char wrong_color_prom[] =
{
	0x00,0xF6,0x07,0xF0,0x00,0x80,0x3F,0xC7,0x00,0xFF,0x07,0x27,0x00,0xFF,0xC9,0x39,
	0x00,0x3C,0x07,0xF0,0x00,0x27,0x29,0xFF,0x00,0xC7,0x17,0xF6,0x00,0xC7,0x39,0x3F
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 Mhz */
	{ 0x30ff, 0x30ff },
	{ soundlatch_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};

static struct AY8910interface hustler_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 Mhz */
	{ 0x30ff },
	{ soundlatch_r },
	{ frogger_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/* same as the above, the only difference is in gfxdecodeinfo to have long bullets */
static struct MachineDriver armorcar_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	armorcar_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/* Rescue and Minefield have extra colours, and custom video initialise */
/* routines to set up the graduated colour backgound they use     * MJC */

static struct MachineDriver rescue_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64+128,8*4+2*2,	/* 32 for the characters, 64 for the stars, 128 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	rescue_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver minefield_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64+128,8*4+2*2,	/* 32 for the characters, 64 for the stars, 128 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	minefield_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};




static struct MachineDriver stratgyx_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			stratgyx_readmem,stratgyx_writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};


static struct MachineDriver hustler_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			hustler_readmem,hustler_writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			hustler_sound_readmem,hustler_sound_writemem,hustler_sound_readport,hustler_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&hustler_ay8910_interface
		}
	}
};

static struct MachineDriver calipso_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	calipso_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	calipso_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/* same as machine_driver, but with moonwar2_readmem for the spinner */
static struct MachineDriver moonwar2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			moonwar2_readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scobra_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};
/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scobra_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "scobra2c.bin", 0x0000, 0x1000, 0x0fe64f76 , 0xe15ade38 )
	ROM_LOAD( "scobra2e.bin", 0x1000, 0x1000, 0x205664e2 , 0xa270e44d )
	ROM_LOAD( "scobra2f.bin", 0x2000, 0x1000, 0x59e8525e , 0xbdd70346 )
	ROM_LOAD( "scobra2h.bin", 0x3000, 0x1000, 0x303ac596 , 0xdca5ec31 )
	ROM_LOAD( "scobra2j.bin", 0x4000, 0x1000, 0x156d771d , 0x0d8f6b6e )
	ROM_LOAD( "scobra2l.bin", 0x5000, 0x1000, 0xbc79c629 , 0x6f80f3a9 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scobra5f.bin", 0x0000, 0x0800, 0x4b2a202a , 0x64d113b4 )
	ROM_LOAD( "scobra5h.bin", 0x0800, 0x0800, 0xaa5bbcd1 , 0xa96316d3 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "scobra.clr", 0x0000, 0x0020, 0xa0a0f682 , 0xfd35c561 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* the ROMs were bad - I took the ones from the Konami version */
	ROM_LOAD( "scobra5c.bin", 0x0000, 0x0800, 0x5b7ffd15 , 0xd4346959 )
	ROM_LOAD( "scobra5d.bin", 0x0800, 0x0800, 0xc1522792 , 0xcc025d95 )
	ROM_LOAD( "scobra5e.bin", 0x1000, 0x0800, 0xc4b1e3e7 , 0x1628c53f )
ROM_END

ROM_START( scobrak_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x1000, 0x71fcefba , 0xa0744b3f )
	ROM_LOAD( "2e", 0x1000, 0x1000, 0xd8edcd97 , 0x8e7245cd )
	ROM_LOAD( "2f", 0x2000, 0x1000, 0xd884517c , 0x47a4e6fb )
	ROM_LOAD( "2h", 0x3000, 0x1000, 0x81707f54 , 0x7244f21c )
	ROM_LOAD( "2j", 0x4000, 0x1000, 0xe9ac3850 , 0xe1f8a801 )
	ROM_LOAD( "2l", 0x5000, 0x1000, 0x3b37371b , 0xd52affde )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800, 0x4b2a202a , 0x64d113b4 )
	ROM_LOAD( "5h", 0x0800, 0x0800, 0xaa5bbcd1 , 0xa96316d3 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "scobra.clr", 0x0000, 0x0020, 0xa0a0f682 , 0xfd35c561 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0x5b7ffd15 , 0xd4346959 )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0xc1522792 , 0xcc025d95 )
	ROM_LOAD( "5e", 0x1000, 0x0800, 0xc4b1e3e7 , 0x1628c53f )
ROM_END

ROM_START( scobrab_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_2c.bin", 0x0000, 0x0800, 0x9eab7bb3 , 0xaeddf391 )
	ROM_LOAD( "vid_2e.bin", 0x0800, 0x0800, 0x3b6585fd , 0x72b57eb7 )
	ROM_LOAD( "vid_2f.bin", 0x1000, 0x0800, 0x3336090a , 0xa26ded8c )
	ROM_LOAD( "vid_2h.bin", 0x1800, 0x0800, 0xed206de8 , 0xdf1a0519 )
	ROM_LOAD( "vid_2j_l.bin", 0x2000, 0x0800, 0xbefdffe5 , 0x2db3e68c )
	ROM_LOAD( "vid_2l_l.bin", 0x2800, 0x0800, 0x9aebadbb , 0xa40158db )
	ROM_LOAD( "vid_2m_l.bin", 0x3000, 0x0800, 0x2a29599d , 0xb9e07c80 )
	ROM_LOAD( "vid_2p_l.bin", 0x3800, 0x0800, 0x06119c0b , 0x96ea7388 )
	ROM_LOAD( "vid_2j_u.bin", 0x4000, 0x0800, 0xf35e2d38 , 0x97aefb83 )
	ROM_LOAD( "vid_2l_u.bin", 0x4800, 0x0800, 0x220f5a25 , 0x72254b10 )
	ROM_LOAD( "vid_2m_u.bin", 0x5000, 0x0800, 0xef190401 , 0xd3b91f19 )
	ROM_LOAD( "vid_2p_u.bin", 0x5800, 0x0800, 0xcd60c228 , 0x1bcc7875 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_5f.bin", 0x0000, 0x0800, 0x4b2a202a , 0x64d113b4 )
	ROM_LOAD( "vid_5h.bin", 0x0800, 0x0800, 0xaa5bbcd1 , 0xa96316d3 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "scobra.clr", 0x0000, 0x0020, 0xa0a0f682 , 0xfd35c561 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin", 0x0000, 0x0800, 0xb5c45422 , 0xdeeb0dd3 )
	ROM_LOAD( "snd_5d.bin", 0x0800, 0x0800, 0xaa50e11a , 0x872c1a74 )
	ROM_LOAD( "snd_5e.bin", 0x1000, 0x0800, 0xb7b4dd96 , 0xccd7a110 )
ROM_END

ROM_START( stratgyx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c.cpu", 0x0000, 0x1000, 0xfd8e7706 , 0xf2aaaf2b )
	ROM_LOAD( "2e.cpu", 0x1000, 0x1000, 0xb66185fd , 0x5873fdc8 )
	ROM_LOAD( "2f.cpu", 0x2000, 0x1000, 0x02b94693 , 0x532d604f )
	ROM_LOAD( "2h.cpu", 0x3000, 0x1000, 0xdaf466b4 , 0x82b1d95e )
	ROM_LOAD( "2j.cpu", 0x4000, 0x1000, 0x437c8c5c , 0x66e84cde )
	ROM_LOAD( "2l.cpu", 0x5000, 0x1000, 0xf195e343 , 0x62b032d0 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f.cpu", 0x0000, 0x0800, 0xe1c5eea1 , 0xf4aa5ddd )
	ROM_LOAD( "5h.cpu", 0x0800, 0x0800, 0xc842da6c , 0x548e4635 )

	ROM_REGION(0x0020)	/* color prom */

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "5c.snd", 0x0000, 0x1000, 0xac5c8db8 , 0x713a5db8 )
	ROM_LOAD( "5d.snd", 0x1000, 0x1000, 0x03f78e87 , 0x46079411 )
ROM_END

ROM_START( stratgyb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "st3", 0x0000, 0x1000, 0xf5bef1a8 , 0xc2f7268c )
	ROM_LOAD( "st4", 0x1000, 0x1000, 0xd161e28b , 0x0dded1ef )
	ROM_LOAD( "st5", 0x2000, 0x1000, 0x21cd4e83 , 0x849e2504 )
	ROM_LOAD( "st6", 0x3000, 0x1000, 0x8a8756f7 , 0x8a64069b )
	ROM_LOAD( "st7", 0x4000, 0x1000, 0x2e84d152 , 0x78b9b898 )
	ROM_LOAD( "st8", 0x5000, 0x1000, 0x0d2736f1 , 0x20bae414 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "st9", 0x0000, 0x0800, 0xe2bcee5e , 0x7121b679 )
	ROM_LOAD( "st10", 0x0800, 0x0800, 0xc939da93 , 0xd105ad91 )

	ROM_REGION(0x0020)	/* color prom */

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "5c.snd", 0x0000, 0x1000, 0xac5c8db8 , 0x713a5db8 )
	ROM_LOAD( "5d.snd", 0x1000, 0x1000, 0x03f78e87 , 0x46079411 )
ROM_END

ROM_START( armorcar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu.2c", 0x0000, 0x1000, 0x81345184 , 0x0d7bfdfb )
	ROM_LOAD( "cpu.2e", 0x1000, 0x1000, 0x503fa409 , 0x76463213 )
	ROM_LOAD( "cpu.2f", 0x2000, 0x1000, 0xdd06ed0e , 0x2cc6d5f0 )
	ROM_LOAD( "cpu.2h", 0x3000, 0x1000, 0x1cab9ae7 , 0x61278dbb )
	ROM_LOAD( "cpu.2j", 0x4000, 0x1000, 0x0906ebe6 , 0xfb158d8c )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu.5f", 0x0000, 0x0800, 0x5a5bc7f1 , 0x8a3da4d1 )
	ROM_LOAD( "cpu.5h", 0x0800, 0x0800, 0x73b9f0db , 0x85bdb113 )

	ROM_REGION(0x0020)	/* color prom */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sound.5c", 0x0000, 0x0800, 0x926e6408 , 0x54ee7753 )
	ROM_LOAD( "sound.5d", 0x0800, 0x0800, 0x2afc8972 , 0x5218fec0 )
ROM_END

ROM_START( moonwar2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mw2.2c", 0x0000, 0x1000, 0xdd4e216e , 0x7c11b4d9 )
	ROM_LOAD( "mw2.2e", 0x1000, 0x1000, 0x4fd203bc , 0x1b6362be )
	ROM_LOAD( "mw2.2f", 0x2000, 0x1000, 0x6487bc59 , 0x4fd8ba4b )
	ROM_LOAD( "mw2.2h", 0x3000, 0x1000, 0x95d0a376 , 0x56879f0d )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mw2.5f", 0x0000, 0x0800, 0x8a0a55b2 , 0xc5fa1aa0 )
	ROM_LOAD( "mw2.5h", 0x0800, 0x0800, 0x3d364ee0 , 0xa6ccc652 )

	ROM_REGION(0x20)	/* color prom */
	ROM_LOAD( "mw2.clr", 0x0000, 0x20, 0x00517b73 , 0x99614c6c )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "mw2.5c", 0x0000, 0x0800, 0xc66f1aed , 0xc26231eb )
	ROM_LOAD( "mw2.5d", 0x0800, 0x0800, 0x5ed01ec6 , 0xbb48a646 )
ROM_END

ROM_START( darkplnt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "drkplt2c.dat", 0x0000, 0x1000, 0x71eb388b , 0x5a0ca559 )
	ROM_LOAD( "drkplt2e.dat", 0x1000, 0x1000, 0xca7e0382 , 0x52e2117d )
	ROM_LOAD( "drkplt2g.dat", 0x2000, 0x1000, 0x31600f5a , 0x4093219c )
	ROM_LOAD( "drkplt2j.dat", 0x3000, 0x1000, 0x3af0013c , 0xb974c78d )
	ROM_LOAD( "drkplt2k.dat", 0x4000, 0x1000, 0xbdb18695 , 0x71a37385 )
	ROM_LOAD( "drkplt2l.dat", 0x5000, 0x1000, 0x1876af34 , 0x5ad25154 )
	ROM_LOAD( "drkplt2m.dat", 0x6000, 0x1000, 0x99be094e , 0x8d2f0122 )
	ROM_LOAD( "drkplt2p.dat", 0x7000, 0x1000, 0xc3e7f2d5 , 0x2d66253b )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "drkplt5f.dat", 0x0000, 0x0800, 0x8d47bb07 , 0x2af0ee66 )
	ROM_LOAD( "drkplt5h.dat", 0x0800, 0x0800, 0x0551adfb , 0x66ef3225 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "6e.cpu", 0x0000, 0x0020, 0xef110007 , 0x86b6e124 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c.snd", 0x0000, 0x1000, 0xa8bcba7a , 0x672b9454 )
ROM_END

ROM_START( tazmania_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c.cpu", 0x0000, 0x1000, 0xf1e5b623 , 0x932c5a06 )
	ROM_LOAD( "2e.cpu", 0x1000, 0x1000, 0x57a62bb0 , 0xef17ce65 )
	ROM_LOAD( "2f.cpu", 0x2000, 0x1000, 0xd60bd211 , 0x43c7c39d )
	ROM_LOAD( "2h.cpu", 0x3000, 0x1000, 0x7071301b , 0xbe829694 )
	ROM_LOAD( "2j.cpu", 0x4000, 0x1000, 0xd641c641 , 0x6e197271 )
	ROM_LOAD( "2k.cpu", 0x5000, 0x1000, 0x4512af78 , 0xa1eb453b )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f.cpu", 0x0000, 0x0800, 0x0abd9559 , 0x2c5b612b )
	ROM_LOAD( "5h.cpu", 0x0800, 0x0800, 0x7daaccb4 , 0x3f5ff3ac )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "colr6f.cpu", 0x0000, 0x0020, 0x4df3f3a1 , 0xfce333c7 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rom0.snd", 0x0000, 0x0800, 0x292ad1dc , 0xb8d741f1 )
ROM_END

ROM_START( calipso_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "calipso.2c", 0x0000, 0x1000, 0xb6209dfa , 0x0fcb703c )
	ROM_LOAD( "calipso.2e", 0x1000, 0x1000, 0x9d67ce59 , 0xc6622f14 )
	ROM_LOAD( "calipso.2f", 0x2000, 0x1000, 0x86c21320 , 0x7bacbaba )
	ROM_LOAD( "calipso.2h", 0x3000, 0x1000, 0x64c56a2b , 0xa3a8111b )
	ROM_LOAD( "calipso.2j", 0x4000, 0x1000, 0x505b3077 , 0xfcbd7b9e )
	ROM_LOAD( "calipso.2l", 0x5000, 0x1000, 0x50993eab , 0xf7630cab )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "calipso.5f", 0x0000, 0x2000, 0x18a04e00 , 0xfd4252e9 )
	ROM_LOAD( "calipso.5h", 0x2000, 0x2000, 0x44f067e4 , 0x1663a73a )

	ROM_REGION(0x20)	/* color prom */
	ROM_LOAD( "calipso.clr", 0x0000, 0x20, 0xc55f36a5 , 0x01165832 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "calipso.5c", 0x0000, 0x0800, 0x368fc8d1 , 0x9cbc65ab )
	ROM_LOAD( "calipso.5d", 0x0800, 0x0800, 0x749e9608 , 0xa225ee3b )
ROM_END

ROM_START( anteater_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ra1-2c", 0x0000, 0x1000, 0x3a99be31 , 0x58bc9393 )
	ROM_LOAD( "ra1-2e", 0x1000, 0x1000, 0xb46154bb , 0x574fc6f6 )
	ROM_LOAD( "ra1-2f", 0x2000, 0x1000, 0xd467888d , 0x2f7c1fe5 )
	ROM_LOAD( "ra1-2h", 0x3000, 0x1000, 0xfbcffc91 , 0xae8a5da3 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ra6-5f", 0x1000, 0x0800, 0x5a8a7eb6 , 0x4c3f8a08 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "ra6-5h", 0x1800, 0x0800, 0x491cbf24 , 0xb30c7c9f )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x0020)	/* color prom */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ra4-5c", 0x0000, 0x0800, 0xa8a615f0 , 0x87300b4f )
	ROM_LOAD( "ra4-5d", 0x0800, 0x0800, 0x2ff74b03 , 0xaf4e5ffe )
ROM_END

ROM_START( rescue_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rb15acpu.bin", 0x0000, 0x1000, 0x2d7f947b , 0xd7e654ba )
	ROM_LOAD( "rb15bcpu.bin", 0x1000, 0x1000, 0x09fbbe11 , 0xa93ea158 )
	ROM_LOAD( "rb15ccpu.bin", 0x2000, 0x1000, 0x7d76d748 , 0x058cd3d0 )
	ROM_LOAD( "rb15dcpu.bin", 0x3000, 0x1000, 0x592c8dd0 , 0xd6505742 )
	ROM_LOAD( "rb15ecpu.bin", 0x4000, 0x1000, 0x9c059431 , 0x604df3a4 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rb15fcpu.bin", 0x1000, 0x0800, 0xb55f561f , 0x4489d20c )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "rb15hcpu.bin", 0x1800, 0x0800, 0x7a13c447 , 0x5512c547 )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x0020)	/* color prom */
	/* This is the original color PROM, however the colors are not entirely correct - */
	/* there is some additional hardware coloring the sky and sea. */
	ROM_LOAD( "rescue.clr", 0x0000, 0x0020, 0x4b086088 , 0x40c6bcbd )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rb15csnd.bin", 0x0000, 0x0800, 0xfd8c78cc , 0x8b24bf17 )
	ROM_LOAD( "rb15dsnd.bin", 0x0800, 0x0800, 0xfdbb116f , 0xd96e4fb3 )
ROM_END

ROM_START( minefld_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ma22c", 0x0000, 0x1000, 0x76549dee , 0x1367a035 )
	ROM_LOAD( "ma22e", 0x1000, 0x1000, 0x080495f0 , 0x68946d21 )
	ROM_LOAD( "ma22f", 0x2000, 0x1000, 0xe0129b76 , 0x7663aee5 )
	ROM_LOAD( "ma22h", 0x3000, 0x1000, 0xc8b5487f , 0x9787475d )
	ROM_LOAD( "ma22j", 0x4000, 0x1000, 0x31277229 , 0x2ceceb54 )
	ROM_LOAD( "ma22l", 0x5000, 0x1000, 0xbb011a9d , 0x85138fc9 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ma15f", 0x1000, 0x0800, 0xa801b143 , 0x9f703006 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "ma15h", 0x1800, 0x0800, 0x818f5e89 , 0xed0dccb1 )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x0020)	/* color prom */
	/* This is the original color PROM, however the colors are not entirely correct - */
	/* there is some additional hardware coloring the sky and sea. */
	ROM_LOAD( "minefld.clr", 0x0000, 0x0020, 0xbf91db37 , 0x1877368e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ma15c", 0x0000, 0x0800, 0x5ddc5d4c , 0x8bef736b )
	ROM_LOAD( "ma15d", 0x0800, 0x0800, 0x73ef5b99 , 0xf67b3f97 )
ROM_END

ROM_START( losttomb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x1000, 0x55248120 , 0xd6176d2c )
	ROM_LOAD( "2e", 0x1000, 0x1000, 0x8d5d9663 , 0xa5f55f4a )
	ROM_LOAD( "2f", 0x2000, 0x1000, 0xb53e6390 , 0x0169fa3c )
	ROM_LOAD( "2h-easy", 0x3000, 0x1000, 0x18d580a9 , 0x054481b6 )
	ROM_LOAD( "2j", 0x4000, 0x1000, 0x5d21f893 , 0x249ee040 )
	ROM_LOAD( "2l", 0x5000, 0x1000, 0x030788df , 0xc7d2e608 )
	ROM_LOAD( "2m", 0x6000, 0x1000, 0x24205f0e , 0xbc4bc5b1 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x1000, 0x0800, 0x5be70b13 , 0x61f137e7 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "5h", 0x1800, 0x0800, 0xd6e32f73 , 0x5581de5f )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "ltprom", 0x0000, 0x0020, 0xca063056 , 0x1108b816 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0xa28e0d60 , 0xb899be2a )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0x70ea19ea , 0x6907af31 )
ROM_END

ROM_START( losttmbh_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x1000, 0x55248120 , 0xd6176d2c )
	ROM_LOAD( "2e", 0x1000, 0x1000, 0x8d5d9663 , 0xa5f55f4a )
	ROM_LOAD( "2f", 0x2000, 0x1000, 0xb53e6390 , 0x0169fa3c )
	ROM_LOAD( "lthard", 0x3000, 0x1000, 0x30bdb65d , 0xe32cbf0e )
	ROM_LOAD( "2j", 0x4000, 0x1000, 0x5d21f893 , 0x249ee040 )
	ROM_LOAD( "2l", 0x5000, 0x1000, 0x030788df , 0xc7d2e608 )
	ROM_LOAD( "2m", 0x6000, 0x1000, 0x24205f0e , 0xbc4bc5b1 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x1000, 0x0800, 0x5be70b13 , 0x61f137e7 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "5h", 0x1800, 0x0800, 0xd6e32f73 , 0x5581de5f )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "ltprom", 0x0000, 0x0020, 0xca063056 , 0x1108b816 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0xa28e0d60 , 0xb899be2a )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0x70ea19ea , 0x6907af31 )
ROM_END

ROM_START( superbon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2d.cpu", 0x0000, 0x1000, 0x00028b0e , 0x60c0ba18 )
	ROM_LOAD( "2e.cpu", 0x1000, 0x1000, 0x83c736e7 , 0xddcf44bf )
	ROM_LOAD( "2f.cpu", 0x2000, 0x1000, 0xb4e5df69 , 0xbb66c2d5 )
	ROM_LOAD( "2h.cpu", 0x3000, 0x1000, 0x71e68756 , 0x74f4f04d )
	ROM_LOAD( "2j.cpu", 0x4000, 0x1000, 0xbc7207b8 , 0x78effb08 )
	ROM_LOAD( "2l.cpu", 0x5000, 0x1000, 0x2fae95f2 , 0xe9dcecbd )
	ROM_LOAD( "2m.cpu", 0x6000, 0x1000, 0x84791ba5 , 0x3ed0337e )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f.cpu", 0x0000, 0x0800, 0xfd44d404 , 0x5b9d4686 )
	ROM_LOAD( "5h.cpu", 0x0800, 0x0800, 0x177c7dfc , 0x58c29927 )

	ROM_REGION(0x0020)	/* color prom */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c.snd", 0x0000, 0x0800, 0xa28e0d60 , 0xb899be2a )
	ROM_LOAD( "5d.snd", 0x0800, 0x0800, 0x70e419ec , 0x80640a04 )
ROM_END

ROM_START( hustler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "hustler.1", 0x0000, 0x1000, 0x1264f260 , 0x94479a3e )
	ROM_LOAD( "hustler.2", 0x1000, 0x1000, 0xc6af76a7 , 0x3cc67bcc )
	ROM_LOAD( "hustler.3", 0x2000, 0x1000, 0x78fb91dd , 0x9422226a )
	/* 3000-3fff space for diagnostics ROM */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hustler.5", 0x0000, 0x0800, 0x47866ee0 , 0x0bdfad0e )
	ROM_LOAD( "hustler.4", 0x0800, 0x0800, 0x1bb72927 , 0x8e062177 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "hustler.clr", 0x0000, 0x0020, 0x99723f42 , 0xaa1f7f5e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "hustler.6", 0x0000, 0x0800, 0x20f7835f , 0x7a946544 )
	ROM_LOAD( "hustler.7", 0x0800, 0x0800, 0xb122b54e , 0x3db57351 )
ROM_END

ROM_START( pool_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a", 0x0000, 0x1000, 0xffc0df06 , 0xb7eb50c0 )
	ROM_LOAD( "b", 0x1000, 0x1000, 0x60452fcd , 0x988fe1c5 )
	ROM_LOAD( "c", 0x2000, 0x1000, 0x342d1a2b , 0x7b8de793 )
	/* 3000-3fff space for diagnostics ROM */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hustler.5", 0x0000, 0x0800, 0x47866ee0 , 0x0bdfad0e )
	ROM_LOAD( "hustler.4", 0x0800, 0x0800, 0x1bb72927 , 0x8e062177 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "hustler.clr", 0x0000, 0x0020, 0x99723f42 , 0xaa1f7f5e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "hustler.6", 0x0000, 0x0800, 0x20f7835f , 0x7a946544 )
	ROM_LOAD( "hustler.7", 0x0800, 0x0800, 0xb122b54e , 0x3db57351 )
ROM_END



static int bit(int i,int n)
{
	return ((i >> n) & 1);
}


static void anteater_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0x9bf;
		j |= ( bit(i,4) ^ bit(i,9) ^ ( bit(i,2) & bit (i,10) ) ) << 6;
		j |= ( bit(i,2) ^ bit(i,10) ) << 9;
		j |= ( bit(i,0) ^ bit(i,6) ^ 1 ) << 10;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void rescue_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0xa7f;
		j |= ( bit(i,3) ^ bit(i,10) ) << 7;
		j |= ( bit(i,1) ^ bit(i,7) ) << 8;
		j |= ( bit(i,0) ^ bit(i,8) ) << 10;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void minefld_decode(void)
{
	/*
	*   Code To Decode Minefield by Mike Balfour and Nicola Salmoria
	*/
	int i,j;
	unsigned char *RAM;


	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0xd5f;
		j |= ( bit(i,3) ^ bit(i,7) ) << 5;
		j |= ( bit(i,2) ^ bit(i,9) ^ ( bit(i,0) & bit(i,5) ) ^
				( bit(i,3) & bit(i,7) & ( bit(i,0) ^ bit(i,5) ))) << 7;
		j |= ( bit(i,0) ^ bit(i,5) ^ ( bit(i,3) & bit(i,7) ) ) << 9;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void losttomb_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0xa7f;
		j |= ( (bit(i,1) & bit(i,8)) | ((1 ^ bit(i,1)) & (bit(i,10)))) << 7;
		j |= ( bit(i,7) ^ (bit(i,1) & ( bit(i,7) ^ bit(i,10) ))) << 8;
		j |= ( (bit(i,1) & bit(i,7)) | ((1 ^ bit(i,1)) & (bit(i,8)))) << 10;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void superbon_decode(void)
{
	/*
	*   Code rom deryption worked out by hand by Chris Hardy.
	*/
	int i;
	unsigned char *RAM;


	RAM = Machine->memory_region[0];

	for (i = 0;i < 0x1000;i++)
	{
		/* Code is encrypted depending on bit 7 and 9 of the address */
		switch (i & 0x280)
		{
			case 0x000:
				RAM[i] ^= 0x92;
				break;
			case 0x080:
				RAM[i] ^= 0x82;
				break;
			case 0x200:
				RAM[i] ^= 0x12;
				break;
			case 0x280:
				RAM[i] ^= 0x10;
				break;
		}
	}
}


static void hustler_decode(void)
{
	int A;


	for (A = 0;A < 0x4000;A++)
	{
		unsigned char xormask;
		int adr[8];
		int i;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


		for (i = 0;i < 8;i++)
			adr[i] = (~A >> i) & 1;

		xormask = 0;
		if (adr[0] ^ adr[1]) xormask |= 0x01;
		if (adr[3] ^ adr[6]) xormask |= 0x02;
		if (adr[4] ^ adr[5]) xormask |= 0x04;
		if (adr[0] ^ adr[2]) xormask |= 0x08;
		if (adr[2] ^ adr[3]) xormask |= 0x10;
		if (adr[1] ^ adr[5]) xormask |= 0x20;
		if (adr[0] ^ adr[7]) xormask |= 0x40;
		if (adr[4] ^ adr[6]) xormask |= 0x80;

		RAM[A] ^= ~xormask;
	}

	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	{
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


		for (A = 0;A < 0x0800;A++)
			RAM[A] = (RAM[A] & 0xfc) | ((RAM[A] & 1) << 1) | ((RAM[A] & 2) >> 1);
	}
}



static int scobra_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	/* Wait for machine initialization to be done. */
	if (memcmp(&RAM[0x8200], "\x00\x00\x01", 3) != 0) return 0;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* Load and set hiscore table. */
		osd_fread(f,&RAM[0x8200],3*10);
		/* copy high score */
		memcpy(&RAM[0x80A8],&RAM[0x8200],3);

		osd_fclose(f);
	}

	return 1;
}

static void scobra_hisave(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Write hiscore table. */
		osd_fwrite(f,&RAM[0x8200],3*10);
		osd_fclose(f);
	}
}


static int armorcar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	/* Wait for machine initialization to be done. */
	if ((memcmp(&RAM[0x8113], "\x02\x04\x40", 3) != 0) &&
	    (memcmp(&RAM[0x814C], "WHP", 3) != 0))
	return 0;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* Load and set hiscore table. */
		osd_fread(f,&RAM[0x8113],6*10);
		osd_fclose(f);
	}

	return 1;
}

static void armorcar_hisave(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Write hiscore table. */
		osd_fwrite(f,&RAM[0x8113],6*10);
		osd_fclose(f);
	}
}


static int moonwar2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x80f2], "\x02\x04\x30", 3) == 0 &&
			memcmp(&RAM[0x8128], "\x00\x25\x40", 3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x80f2],6*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void moonwar2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
		osd_fwrite(f,&RAM[0x80f2],6*10);
		osd_fclose(f);
	}
}


static int calipso_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x81cb], "\x01\x04\x80", 3) == 0 &&
			memcmp(&RAM[0x8201], "\x00\x12\x00", 3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x81cb],6*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void calipso_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
		osd_fwrite(f,&RAM[0x81cb],6*10);
		osd_fclose(f);
	}
}


static int anteater_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x80ef], "\x01\x04\x80", 3) == 0 &&
			memcmp(&RAM[0x8125], "\x00\x12\x00", 3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x80ef],6*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void anteater_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
		osd_fwrite(f,&RAM[0x80ef],6*10);
		osd_fclose(f);
	}
}



struct GameDriver scobra_driver =
{
	__FILE__,
	0,
	"scobra",
	"Super Cobra (Stern)",
	"1981",
	"Stern",
	"Nicola Salmoria (MAME driver)\nValerio Verrando (high score)\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,

	scobra_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	scobra_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scobra_hiload, scobra_hisave
};

struct GameDriver scobrak_driver =
{
	__FILE__,
	&scobra_driver,
	"scobrak",
	"Super Cobra (Konami)",
	"1981",
	"Konami",
	"Nicola Salmoria (MAME driver)\nValerio Verrando (high score)\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,

	scobrak_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	scobrak_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scobra_hiload, scobra_hisave
};

struct GameDriver scobrab_driver =
{
	__FILE__,
	&scobra_driver,
	"scobrab",
	"Super Cobra (bootleg)",
	"1981",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nValerio Verrando (high score)\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,

	scobrab_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	scobra_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scobra_hiload, scobra_hisave
};

struct GameDriver stratgyx_driver =
{
	__FILE__,
	0,
	"stratgyx",
	"Strategy X",
	"1981",
	"Stern",
	"Lee Taylor",
	GAME_WRONG_COLORS,
	&stratgyx_machine_driver,

	stratgyx_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	stratgyx_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver stratgyb_driver =
{
	__FILE__,
	&stratgyx_driver,
	"stratgyb",
	"Strong X",
	"1982",
	"bootleg",
	"Lee Taylor",
	GAME_WRONG_COLORS,
	&stratgyx_machine_driver,

	stratgyb_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	stratgyx_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver armorcar_driver =
{
	__FILE__,
	0,
	"armorcar",
	"Armored Car",
	"1981",
	"Stern",
	"Nicola Salmoria (MAME driver)\nMike Balfour (high score save)",
	GAME_WRONG_COLORS,
	&armorcar_machine_driver,

	armorcar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	armorcar_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	armorcar_hiload, armorcar_hisave
};

struct GameDriver moonwar2_driver =
{
	__FILE__,
	0,
	"moonwar2",
	"Moon War II",
	"1981",
	"Stern",
	"Nicola Salmoria (MAME driver)\nBrad Oliver (additional code)",
	0,
	&moonwar2_machine_driver,

	moonwar2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	moonwar2_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	moonwar2_hiload, moonwar2_hisave
};

struct GameDriver darkplnt_driver =
{
	__FILE__,
	0,
	"darkplnt",
	"Dark Planet",
	"1982",
	"Stern",
	"Mike Balfour",
	GAME_NOT_WORKING,
	&stratgyx_machine_driver,

	darkplnt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	darkplnt_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_180,

	0,0
};

struct GameDriver tazmania_driver =
{
	__FILE__,
	0,
	"tazmania",
	"Tazz-Mania",
	"1982",
	"Stern",
	"Chris Hardy\nChris Moore (high score save)",
	0,
	&machine_driver,

	tazmania_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tazmania_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	anteater_hiload, anteater_hisave
};

struct GameDriver calipso_driver =
{
	__FILE__,
	0,
	"calipso",
	"Calipso",
	"1982",
	"[Stern] (Tago license)",
	"Nicola Salmoria (MAME driver)\nBrad Oliver (additional code)",
	0,
	&calipso_machine_driver,

	calipso_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	calipso_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	calipso_hiload, calipso_hisave
};

struct GameDriver anteater_driver =
{
	__FILE__,
	0,
	"anteater",
	"Ant Eater",
	"1982",
	"[Stern] (Tago license)",
	"James R. Twine\nChris Hardy\nMirko Buffoni\nFabio Buffoni",
	GAME_WRONG_COLORS,
	&machine_driver,

	anteater_rom,
	anteater_decode, 0,
	0,
	0,	/* sound_prom */

	anteater_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	anteater_hiload, anteater_hisave
};

struct GameDriver rescue_driver =
{
	__FILE__,
	0,
	"rescue",
	"Rescue",
	"1982",
	"Stern",
	"James R. Twine\nChris Hardy\nMirko Buffoni\nFabio Buffoni\nAlan J. McCormick\nMike Coates (Background)",
	0,
	&rescue_machine_driver,

	rescue_rom,
	rescue_decode, 0,
	0,
	0,	/* sound_prom */

	rescue_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver minefld_driver =
{
	__FILE__,
	0,
	"minefld",
	"Minefield",
	"1983",
	"Stern",
	"Nicola Salmoria (MAME driver)\nAl Kossow (color info)\nMike Coates (Background)",
	0,
	&minefield_machine_driver,

	minefld_rom,
	minefld_decode, 0,
	0,
	0,	/* sound_prom */

	minefld_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver losttomb_driver =
{
	__FILE__,
	0,
	"losttomb",
	"Lost Tomb (easy)",
	"1982",
	"Stern",
	"Nicola Salmoria\nJames R. Twine\nMirko Buffoni\nFabio Buffoni",
	0,
	&machine_driver,

	losttomb_rom,
	losttomb_decode, 0,
	0,
	0,	/* sound_prom */

	losttomb_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver losttmbh_driver =
{
	__FILE__,
	&losttomb_driver,
	"losttmbh",
	"Lost Tomb (hard)",
	"1982",
	"Stern",
	"Nicola Salmoria\nJames R. Twine\nMirko Buffoni\nFabio Buffoni",
	0,
	&machine_driver,

	losttmbh_rom,
	losttomb_decode, 0,
	0,
	0,	/* sound_prom */

	losttomb_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver superbon_driver =
{
	__FILE__,
	0,
	"superbon",
	"Super Bond",
	"1982?",
	"bootleg",
	"Chris Hardy",
	GAME_WRONG_COLORS,
	&machine_driver,

	superbon_rom,
	superbon_decode, 0,
	0,
	0,	/* sound_prom */

	superbon_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0,0,
};

struct GameDriver hustler_driver =
{
	__FILE__,
	0,
	"hustler",
	"Video Hustler",
	"1981",
	"Konami",
	"Nicola Salmoria",
	0,
	&hustler_machine_driver,

	hustler_rom,
	hustler_decode, 0,
	0,
	0,	/* sound_prom */

	hustler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

/* not working due to different encryption */
struct GameDriver pool_driver =
{
	__FILE__,
	&hustler_driver,
	"pool",
	"Pool",
	"????",
	"?????",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&hustler_machine_driver,

	pool_rom,
	hustler_decode, 0,
	0,
	0,	/* sound_prom */

	hustler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
