/***************************************************************************

Lady Bug memory map (preliminary)

0000-5fff ROM
6000-6fff RAM
d000-d3ff video RAM
          d000-d007/d020-d027/d040-d047/d060-d067 contain the column scroll
          registers (not used by Lady Bug)
d400-d7ff color RAM (4 bits wide)

memory mapped ports:

read:
9000      IN0
9001      IN1
9002      DSW0
9003      DSW1
e000      IN2 (not used by Lady Bug)
8000      interrupt enable? (toggle)?
see the input_ports definition below for details on the input bits

write:
7000-73ff sprites
a000      flip screen
b000      sound port 1
c000      sound port 2

interrupts:
There is no vblank interrupt. The vblank status is read from IN1.
Coin insertion in left slot generates a NMI, in right slot an IRQ.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int ladybug_IN0_r(int offset);
int ladybug_IN1_r(int offset);
int ladybug_interrupt(void);

void ladybug_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void ladybug_flipscreen_w(int offset,int data);
void ladybug_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x9001, 0x9001, input_port_1_r },	/* IN1 */
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x8fff, MRA_NOP },
	{ 0x9000, 0x9000, input_port_0_r },	/* IN0 */
	{ 0x9002, 0x9002, input_port_3_r },	/* DSW0 */
	{ 0x9003, 0x9003, input_port_4_r },	/* DSW1 */
	{ 0xd000, 0xd7ff, MRA_RAM },	/* video and color RAM */
	{ 0xe000, 0xe000, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7000, 0x73ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa000, 0xa000, ladybug_flipscreen_w },
	{ 0xb000, 0xbfff, SN76496_0_w },
	{ 0xc000, 0xcfff, SN76496_1_w },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ -1 }	/* end of table */
};



/***************************************************************************

  Lady Bug doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots. Left slot generates a NMI, Right slot an IRQ.

***************************************************************************/
int ladybug_interrupt(void)
{
	if (readinputport(5) & 1)	/* Left Coin */
		return nmi_interrupt();
	else if (readinputport(5) & 2)	/* Right Coin */
		return interrupt();
	else return ignore_interrupt();
}

INPUT_PORTS_START( ladybug_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_UNUSED )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x40, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( snapjack_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_UNUSED  )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "unused1?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "unused2?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	/* coinage is slightly different from Lady Bug and Cosmic Avenger */
	PORT_DIPNAME( 0x0f, 0x0f, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x05, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x09, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x04 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x50, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x60, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x90, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x04 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( cavenger_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_DIPNAME( 0x30, 0x00, "Initial High Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "8000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 1, 0 },	/* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14,
			8*16+0, 8*16+2, 8*16+4, 8*16+6, 8*16+8, 8*16+10, 8*16+12, 8*16+14 },
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout spritelayout2 =
{
	8,8,	/* 8*8 sprites */
	512,	/* 512 sprites */
	2,	/* 2 bits per pixel */
	{ 1, 0 },	/* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0,  8 },
	{ 1, 0x2000, &spritelayout,  4*8, 16 },
	{ 1, 0x2000, &spritelayout2, 4*8, 16 },
	{ -1 } /* end of array */
};


static unsigned char ladybug_color_prom[] =
{
	/* palette */
	0xF5,0x90,0x41,0x54,0x94,0x11,0x80,0x65,0x05,0xD4,0x01,0x00,0xB1,0xA0,0x00,0xF5,
	0x04,0xB1,0x00,0x15,0x11,0x25,0x90,0xD0,0xA0,0x90,0x15,0x84,0xB5,0x04,0x04,0x04,
	/* sprite color lookup table */
	0x00,0x59,0x33,0xB8,0x00,0xD4,0xA3,0x8D,0x00,0x2C,0x63,0xDD,0x00,0x22,0x38,0x1D,
	0x00,0x93,0x3A,0xDD,0x00,0xE2,0x38,0xDD,0x00,0x82,0x3A,0xD8,0x00,0x22,0x68,0x1D
};

static unsigned char snapjack_color_prom[] =
{
	/* palette */
	0xF5,0x05,0x54,0xC1,0xC4,0x94,0x84,0x24,0xD0,0x90,0xA1,0x00,0x31,0x50,0x25,0xF5,
	0x90,0x31,0x05,0x25,0x05,0x94,0x30,0x41,0x05,0x94,0x61,0x30,0x94,0x50,0x05,0xA5,
	/* sprite color lookup table */
	0x00,0x9D,0x11,0xB8,0x00,0x79,0x62,0x18,0x00,0x9E,0x25,0xDA,0x00,0xD7,0xA3,0x79,
	0x00,0xDE,0x29,0x74,0x00,0xD4,0x75,0x9D,0x00,0xAD,0x86,0x97,0x00,0x5A,0x4C,0x17
};

static unsigned char cavenger_color_prom[] =
{
	/* palette */
	0xF5,0xC4,0xD0,0xB1,0xD4,0x90,0x45,0x44,0x00,0x54,0x91,0x94,0x25,0x21,0x65,0xF5,
	0x21,0x00,0x25,0xD0,0xB1,0x90,0xD4,0xD4,0x25,0xB1,0xC4,0x90,0x65,0xD4,0x00,0x00,
	/* sprite color lookup table */
	0x00,0x78,0xA3,0xB5,0x00,0x8C,0x79,0x64,0x00,0xC3,0xEE,0xDD,0x00,0x3C,0xA2,0x4A,
	0x00,0x87,0xBA,0xDE,0x00,0x2A,0xAE,0xBB,0x00,0x8C,0xC2,0xB7,0x00,0xAC,0xE2,0x1D
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	4000000,	/* 4 MHz */
	{ 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			ladybug_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	32,4*24,
	ladybug_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	ladybug_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ladybug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lb1.cpu", 0x0000, 0x1000, 0x00e5eaaf )
	ROM_LOAD( "lb2.cpu", 0x1000, 0x1000, 0x758e9c98 )
	ROM_LOAD( "lb3.cpu", 0x2000, 0x1000, 0x4295ccd7 )
	ROM_LOAD( "lb4.cpu", 0x3000, 0x1000, 0xad30c2b6 )
	ROM_LOAD( "lb5.cpu", 0x4000, 0x1000, 0xc4da41d6 )
	ROM_LOAD( "lb6.cpu", 0x5000, 0x1000, 0x18aaf1ec )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lb9.vid",  0x0000, 0x1000, 0x80bd96ef )
	ROM_LOAD( "lb10.vid", 0x1000, 0x1000, 0xec7c93c8 )
	ROM_LOAD( "lb8.cpu",  0x2000, 0x1000, 0x2d4d4821 )
	ROM_LOAD( "lb7.cpu",  0x3000, 0x1000, 0xf685434d )
ROM_END

ROM_START( snapjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sj2a.bin", 0x0000, 0x1000, 0x37ef057b )
	ROM_LOAD( "sj2b.bin", 0x1000, 0x1000, 0x5f6a17c6 )
	ROM_LOAD( "sj2c.bin", 0x2000, 0x1000, 0x3cf098fc )
	ROM_LOAD( "sj2d.bin", 0x3000, 0x1000, 0x06fa91f2 )
	ROM_LOAD( "sj2e.bin", 0x4000, 0x1000, 0x135d2527 )
	ROM_LOAD( "sj2f.bin", 0x5000, 0x1000, 0x734f0213 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sj2i.bin", 0x0000, 0x1000, 0xffa6a2ec )
	ROM_LOAD( "sj2j.bin", 0x1000, 0x1000, 0x2506c6f0 )
	ROM_LOAD( "sj2h.bin", 0x2000, 0x1000, 0xdd2fa07f )
	ROM_LOAD( "sj2g.bin", 0x3000, 0x1000, 0x888dec19 )
ROM_END

ROM_START( cavenger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1", 0x0000, 0x1000, 0x8851691b )
	ROM_LOAD( "2", 0x1000, 0x1000, 0xfc637e8b )
	ROM_LOAD( "3", 0x2000, 0x1000, 0x46fcdaba )
	ROM_LOAD( "4", 0x3000, 0x1000, 0xbc747536 )
	ROM_LOAD( "5", 0x4000, 0x1000, 0x25f39d9f )
	ROM_LOAD( "6", 0x5000, 0x1000, 0x38961b88 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9", 0x0000, 0x1000, 0x6522fd1a )
	ROM_LOAD( "0", 0x1000, 0x1000, 0x6132e3d8 )
	ROM_LOAD( "8", 0x2000, 0x1000, 0x366d7ec1 )
/*	ROM_LOAD( "7", 0x3000, 0x1000, 0x )	empty socket */
ROM_END



static int ladybug_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6073],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0x608b],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6073],3*9);
			osd_fread(f,&RAM[0xd380],13*9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void ladybug_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x6073],3*9);
		osd_fwrite(f,&RAM[0xd380],13*9);
		osd_fclose(f);
	}
}



static int cavenger_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x6025],"\x01\x00\x00",3) == 0) &&
		(memcmp(&RAM[0x6063],"\x0A\x15\x28",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x6025],0x41);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void cavenger_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x6025],0x41);
		osd_fclose(f);
	}

}

static int snapjack_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x6A94],"\x01\x00\x00",3) == 0) &&
		(memcmp(&RAM[0x6AA0],"\x01\x00\x00\x1E",4) == 0) &&
		(memcmp(&RAM[0x6AD2],"\x0A\x15\x24",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x6A94],0x41);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void snapjack_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x6A94],0x41);
		osd_fclose(f);
	}

}



struct GameDriver ladybug_driver =
{
	__FILE__,
	0,
	"ladybug",
	"Lady Bug",
	"1981",
	"Universal",
	"Nicola Salmoria",
	0,
	&machine_driver,

	ladybug_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ladybug_input_ports,

	ladybug_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	ladybug_hiload, ladybug_hisave
};

struct GameDriver snapjack_driver =
{
	__FILE__,
	0,
	"snapjack",
	"Snap Jack",
	"1981?",
	"Universal",
	"Nicola Salmoria (MAME driver)\nMike Balfour (high score save)",
	0,
	&machine_driver,

	snapjack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	snapjack_input_ports,

	snapjack_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	snapjack_hiload, snapjack_hisave
};

struct GameDriver cavenger_driver =
{
	__FILE__,
	0,
	"cavenger",
	"Cosmic Avenger",
	"1981",
	"Universal",
	"Nicola Salmoria (MAME driver)\nMike Balfour (high score save)",
	0,
	&machine_driver,

	cavenger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	cavenger_input_ports,

	cavenger_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cavenger_hiload, cavenger_hisave
};
