/***************************************************************************

Amidar memory map (preliminary)

0000-3fff ROM (Amidar US version and Turtles: 0000-4fff)
8000-87ff RAM
9000-93ff Video RAM
9800-983f Screen attributes
9840-985f sprites

read:
a800      watchdog reset
b000      IN0
b010      IN1
b020      IN2
b820      DSW (not Turtles)
see the input_ports definition below for details on the input bits

write:
a008      interrupt enable
a010/a018 flip screen X & Y (I don't know which is which)
a030      coin counter A
a038      coin counter B
b800      To AY-3-8910 port A (commands for the audio CPU)
b810      bit 3 = interrupt trigger on audio CPU


SOUND BOARD:
0000-1fff ROM
8000-83ff RAM

write:
9000      ?
9080      ?

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control
20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void amidar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void amidar_updatehook00(int offset);
void amidar_updatehook01(int offset);
void amidar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int scramble_portB_r(int offset);
void scramble_sh_irqtrigger_w(int offset,int data);

void amidar_coina_w (int offset, int data)
{
	coin_counter_w (0, data);
	coin_counter_w (0, 0);
}

void amidar_coinb_w (int offset, int data)
{
	coin_counter_w (1, data);
	coin_counter_w (1, 0);
}

static struct MemoryReadAddress amidar_readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x985f, MRA_RAM },
	{ 0xa800, 0xa800, watchdog_reset_r },
	{ 0xb000, 0xb000, input_port_0_r },	/* IN0 */
	{ 0xb010, 0xb010, input_port_1_r },	/* IN1 */
	{ 0xb020, 0xb020, input_port_2_r },	/* IN2 */
	{ 0xb820, 0xb820, input_port_3_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram00_w, &videoram00 },
	{ 0x9800, 0x983f, videoram01_w, &videoram01 },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_NOP },
	{ 0xa008, 0xa008, interrupt_enable_w },
	{ 0xa010, 0xa010, MWA_RAM, &flip_screen_x },
	{ 0xa018, 0xa018, MWA_RAM, &flip_screen_y },
	{ 0xa030, 0xa030, amidar_coina_w },
	{ 0xa038, 0xa038, amidar_coinb_w },
	{ 0xb800, 0xb800, soundlatch_w },
	{ 0xb810, 0xb810, scramble_sh_irqtrigger_w },
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
{ 0x9000, 0x9000, MWA_NOP },
{ 0x9080, 0x9080, MWA_NOP },
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



INPUT_PORTS_START( amidar_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000 70000" )
	PORT_DIPSETTING(    0x04, "50000 80000" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x08, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0e, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x80, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0xe0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disable All Coins" )
INPUT_PORTS_END

/* absolutely identical to amidar, the only difference is the BONUS dip switch */
INPUT_PORTS_START( amidarjp_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000 50000" )
	PORT_DIPSETTING(    0x04, "50000 50000" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x08, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0e, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x80, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0xe0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disable All Coins" )
INPUT_PORTS_END

/* similar to Amidar, dip swtiches are different and port 3, which in Amidar */
/* selects coins per credit, is not used. */
INPUT_PORTS_START( turtles_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "126" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* same as Turtles, but dip switches are different. */
INPUT_PORTS_START( turpin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x03, "126" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



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

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static struct GfxTileLayout tilelayout =
{
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxTileDecodeInfo gfxtiledecodeinfo[] =
{
	{ 1, 0x0000, &tilelayout, 0, 8, 0 },
	{ -1 } /* end of array */
};



static struct MachineLayer machine_layers[MAX_LAYERS] =
{
	{
		LAYER_TILE,
		32*8,32*8,
		gfxtiledecodeinfo,
		0,
		amidar_updatehook00,amidar_updatehook01,0,0
	}
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



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			amidar_readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,32,
	amidar_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	machine_layers,
	generic_vh_start,
	generic_vh_stop,
	amidar_vh_screenrefresh,

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

ROM_START( amidar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "amidarus.2c", 0x0000, 0x1000, 0xd04355e9 )
	ROM_LOAD( "amidarus.2e", 0x1000, 0x1000, 0xaaf3b9f9 )
	ROM_LOAD( "amidarus.2f", 0x2000, 0x1000, 0x65dbb08f )
	ROM_LOAD( "amidarus.2h", 0x3000, 0x1000, 0x42647e74 )
	ROM_LOAD( "amidarus.2j", 0x4000, 0x1000, 0x80994cd9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "amidarus.5f", 0x0000, 0x0800, 0xa114cb34 )
	ROM_LOAD( "amidarus.5h", 0x0800, 0x0800, 0x0d1cafd2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c", 0x0000, 0x1000, 0x2fd961f7 )
	ROM_LOAD( "amidarus.5d", 0x1000, 0x1000, 0xfa4bd265 )
ROM_END

ROM_START( amidarjp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "amidar.2c", 0x0000, 0x1000, 0x061cf902 )
	ROM_LOAD( "amidar.2e", 0x1000, 0x1000, 0x68948086 )
	ROM_LOAD( "amidar.2f", 0x2000, 0x1000, 0x89747538 )
	ROM_LOAD( "amidar.2h", 0x3000, 0x1000, 0xa1bc1266 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "amidar.5f", 0x0000, 0x0800, 0x1e31bc69 )
	ROM_LOAD( "amidar.5h", 0x0800, 0x0800, 0x8b6560db )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "amidar.5c", 0x0000, 0x1000, 0xcdfaa8be )
	ROM_LOAD( "amidar.5d", 0x1000, 0x1000, 0xc84cdcfc )
ROM_END

ROM_START( amigo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2732.a1", 0x0000, 0x1000, 0x43d647a4 )
	ROM_LOAD( "2732.a2", 0x1000, 0x1000, 0x3f1c500a )
	ROM_LOAD( "2732.a3", 0x2000, 0x1000, 0xa698c12c )
	ROM_LOAD( "2732.a4", 0x3000, 0x1000, 0x3c6d621d )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2716.a6", 0x0000, 0x0800, 0xe09ed6c8 )
	ROM_LOAD( "2716.a5", 0x0800, 0x0800, 0x3355a22f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "2732.a7", 0x0000, 0x1000, 0x2fd961f7 )
	ROM_LOAD( "2732.a8", 0x1000, 0x1000, 0xfa4bd265 )
ROM_END

ROM_START( turtles_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "turt_vid.2c", 0x0000, 0x1000, 0xd64b683b )
	ROM_LOAD( "turt_vid.2e", 0x1000, 0x1000, 0x694c07b6 )
	ROM_LOAD( "turt_vid.2f", 0x2000, 0x1000, 0x07385210 )
	ROM_LOAD( "turt_vid.2h", 0x3000, 0x1000, 0xb71c1118 )
	ROM_LOAD( "turt_vid.2j", 0x4000, 0x1000, 0x2f33ea7b )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "turt_vid.5h", 0x0000, 0x0800, 0x077a3a46 )
	ROM_LOAD( "turt_vid.5f", 0x0800, 0x0800, 0xd1107d84 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c", 0x0000, 0x1000, 0x90ecc748 )
	ROM_LOAD( "turt_snd.5d", 0x1000, 0x1000, 0xad8bffad )
ROM_END

ROM_START( turpin_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "M1",          0x0000, 0x1000, 0x805f1577 )
	ROM_LOAD( "M2",          0x1000, 0x1000, 0x694f9789 )
	ROM_LOAD( "M3",          0x2000, 0x1000, 0x1b08134e )
	ROM_LOAD( "turt_vid.2h", 0x3000, 0x1000, 0xb71c1118 )
	ROM_LOAD( "M5",          0x4000, 0x1000, 0x112be471 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "turt_vid.5h", 0x0000, 0x0800, 0x077a3a46 )
	ROM_LOAD( "turt_vid.5f", 0x0800, 0x0800, 0xd1107d84 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c", 0x0000, 0x1000, 0x90ecc748 )
	ROM_LOAD( "turt_snd.5d", 0x1000, 0x1000, 0xad8bffad )
ROM_END



static unsigned char amidar_color_prom[] =
{
	/* palette */
	0x00,0x07,0xC0,0xB6,0x00,0x38,0xC5,0x67,0x00,0x30,0x07,0x3F,0x00,0x07,0x30,0x3F,
	0x00,0x3F,0x30,0x07,0x00,0x38,0x67,0x3F,0x00,0xFF,0x07,0xDF,0x00,0xF8,0x07,0xFF
};

static unsigned char turtles_color_prom[] =
{
	/* palette */
	0x00,0xC0,0x57,0xFF,0x00,0x66,0xF2,0xFE,0x00,0x2D,0x12,0xBF,0x00,0x2F,0x7D,0xB8,
	0x00,0x72,0xD2,0x06,0x00,0x94,0xFF,0xE8,0x00,0x54,0x2F,0xF6,0x00,0x24,0xBF,0xC6
};



static int amidar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8200],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x821b],"\x00\x00\x01",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8200],3*10);
			RAM[0x80a8] = RAM[0x8200];
			RAM[0x80a9] = RAM[0x8201];
			RAM[0x80aa] = RAM[0x8202];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static int turtles_hiload(void) /* V.V */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* same as Amidar, but the high score table is initialized with zeros */
	/* a working quick-and-dirty solution is to update the top high score */
	/* and the whole table at different times */
	/* further study of the game code may provide a cleaner solution */

	static int first_pass = 0;
	static unsigned char top_score[] = { 0, 0, 0 };

	if (first_pass == 0)
	{
		void *f;

			if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
			{
				osd_fread(f,top_score,3);
				osd_fclose(f);
			}
		first_pass = 1;
	}

	if ((memcmp(&RAM[0x80A0],"\xc0\xc0\x00",3) == 0))
	{
		RAM[0x80a8] = top_score[0];
		RAM[0x80a9] = top_score[1];
		RAM[0x80aa] = top_score[2];
		return 0;
	} /* continuously updating top high score - really dirty solution */

	else if (memcmp(&RAM[0x80A0],"\xc6\xc6\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8200],3*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void amidar_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8200],3*10);
		osd_fclose(f);
	}
}



struct GameDriver amidar_driver =
{
	__FILE__,
	0,
	"amidar",
	"Amidar (US)",
	"1982",
	"Konami (Stern license)",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	0,
	&machine_driver,

	amidar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	amidar_input_ports,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver amidarjp_driver =
{
	__FILE__,
	&amidar_driver,
	"amidarjp",
	"Amidar (Japan)",
	"1981",
	"Konami",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	0,
	&machine_driver,

	amidarjp_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	amidarjp_input_ports,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver amigo_driver =
{
	__FILE__,
	&amidar_driver,
	"amigo",
	"Amigo",
	"1982",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)\nDavid Winter (game driver)",
	0,
	&machine_driver,

	amigo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	amidar_input_ports,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver turtles_driver =
{
	__FILE__,
	0,
	"turtles",
	"Turtles",
	"1981",
	"Stern",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)\nValerio Verrando (high score save)",
	0,
	&machine_driver,

	turtles_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	turtles_input_ports,

	turtles_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	turtles_hiload, amidar_hisave
};

struct GameDriver turpin_driver =
{
	__FILE__,
	&turtles_driver,
	"turpin",
	"Turpin",
	"1981",
	"Sega",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)\nValerio Verrando (high score save)",
	0,
	&machine_driver,

	turpin_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	turpin_input_ports,

	turtles_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	turtles_hiload, amidar_hisave
};
