/***************************************************************************

Pac Man memory map (preliminary)

0000-3fff ROM
4000-43ff Video RAM
4400-47ff Color RAM
4c00-4fff RAM
8000-9fff ROM (Ms Pac Man and Ponpoko only)
a000-bfff ROM (Ponpoko only)

memory mapped ports:

read:
5000      IN0
5040      IN1
5080      DSW 1
50c0	  DSW 2 (Ponpoko only)
see the input_ports definition below for details on the input bits

write:
4ff0-4fff 8 pairs of two bytes:
          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
		  X flip (bit 1); the second byte the color
5000      interrupt enable
5001      sound enable
5002      ????
5003      flip screen
5004      1 player start lamp
5005      2 players start lamp
5006      related to the credits. don't know what it was used for.
5007      coin counter
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
5060-506f Sprite coordinates, x/y pairs for 8 sprites
50c0      Watchdog reset

I/O ports:
OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void pacman_init_machine(void);
int pacman_interrupt(void);

int pacman_vh_start(void);
void pengo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void pengo_updatehook0(int offset);
void pengo_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *pengo_soundregs;
void pengo_sound_enable_w(int offset,int data);
void pengo_sound_w(int offset,int data);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x4c00, 0x4fff, MRA_RAM },	/* including sprite codes at 4ff0-4fff */
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ 0x50c0, 0x50ff, input_port_3_r },	/* DSW2 */
	{ 0x8000, 0xbfff, MRA_ROM },	/* Ms. Pac Man / Ponpoko only */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram00_w, &videoram00 },
	{ 0x4400, 0x47ff, videoram01_w, &videoram01 },
	{ 0x4c00, 0x4fef, MWA_RAM },
	{ 0x4ff0, 0x4fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5002, MWA_NOP },
	{ 0x5003, 0x5003, MWA_RAM, &flip_screen },
 	{ 0x5004, 0x5005, osd_led_w },
 	{ 0x5006, 0x5006, MWA_NOP },
 	{ 0x5007, 0x5007, coin_counter_w },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x5060, 0x506f, MWA_RAM, &spriteram_2 },
	{ 0x50c0, 0x50c0, watchdog_reset_w },
	{ 0x8000, 0xbfff, MWA_ROM },	/* Ms. Pac Man / Ponpoko only */
	{ 0xc000, 0xc3ff, videoram00_w },	/* mirror address for video ram, */
	{ 0xc400, 0xc7ef, videoram01_w },	/* used to display HIGH SCORE and CREDITS */
	{ 0xffff, 0xffff, MWA_NOP },		/* Eyes writes to this location to simplify code */
	{ -1 }	/* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },	/* Pac Man only */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( pacman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "Never" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Ghost Names", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Alternate" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
INPUT_PORTS_END

/* Ms. Pac Man input ports are identical to Pac Man, the only difference is */
/* the missing Ghost Names dip switch. */
INPUT_PORTS_START( mspacman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "Never" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( crush_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x10, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x10, "Cocktail" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, "First Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Teleport holes", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( ponpoko_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	/* The 2nd player controls are used even in upright mode */
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_DIPNAME( 0x0f, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "A 3/1 B 3/1" )
	PORT_DIPSETTING(    0x0e, "A 3/1 B 1/2" )
	PORT_DIPSETTING(    0x0f, "A 3/1 B 1/4" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x0d, "A 2/1 B 1/1" )
	PORT_DIPSETTING(    0x07, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x0b, "A 2/1 B 1/5" )
	PORT_DIPSETTING(    0x0c, "A 2/1 B 1/6" )
	PORT_DIPSETTING(    0x01, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 4/5" )
	PORT_DIPSETTING(    0x05, "A 1/1 B 2/3" )
	PORT_DIPSETTING(    0x0a, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x08, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x09, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x03, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 5", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( eyes_input_ports )
    PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(0x10, "Off" )
	PORT_DIPSETTING(0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000" )
	PORT_DIPSETTING(    0x20, "75000" )
	PORT_DIPSETTING(    0x10, "100000" )
	PORT_DIPSETTING(    0x00, "125000" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )  /* Not accessed */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x1000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};


static struct GfxTileLayout tilelayout =
{
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};


static struct GfxTileDecodeInfo gfxtiledecodeinfo[] =
{
	{ 1, 0x0000, &tilelayout,   0, 32, 0 },
	{ -1 } /* end of array */
};



static struct MachineLayer machine_layers[MAX_LAYERS] =
{
	{
		LAYER_TILE,
		36*8,28*8,
		gfxtiledecodeinfo,
		0,
		pengo_updatehook0,pengo_updatehook0,0,0
	}
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
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,writeport,
			pacman_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	pacman_init_machine,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	16, 4*32,
	pengo_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	machine_layers,
	pacman_vh_start,
	generic_vh_stop,
	pengo_vh_screenrefresh,

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

ROM_START( pacman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacman.6e", 0x0000, 0x1000, 0x8200be38 )
	ROM_LOAD( "pacman.6f", 0x1000, 0x1000, 0xd800bc8a )
	ROM_LOAD( "pacman.6h", 0x2000, 0x1000, 0xd800986c )
	ROM_LOAD( "pacman.6j", 0x3000, 0x1000, 0xbca63c60 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacman.5e", 0x0000, 0x1000, 0x45346da8 )
	ROM_LOAD( "pacman.5f", 0x1000, 0x1000, 0x0f80461c )
ROM_END

ROM_START( namcopac_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "namcopac.6e", 0x0000, 0x1000, 0x86002ca0 )
	ROM_LOAD( "namcopac.6f", 0x1000, 0x1000, 0xd700205a )
	ROM_LOAD( "namcopac.6h", 0x2000, 0x1000, 0xd70098ec )
	ROM_LOAD( "namcopac.6j", 0x3000, 0x1000, 0x2700e81e )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "namcopac.5e", 0x0000, 0x1000, 0x45346da8 )
	ROM_LOAD( "namcopac.5f", 0x1000, 0x1000, 0x0f80461c )
ROM_END

ROM_START( pacmanjp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "prg1", 0x0000, 0x0800, 0xd577c995 )
	ROM_LOAD( "prg2", 0x0800, 0x0800, 0xac8977ad )
	ROM_LOAD( "prg3", 0x1000, 0x0800, 0xda22d9ac )
	ROM_LOAD( "prg4", 0x1800, 0x0800, 0xfdde6526 )
	ROM_LOAD( "prg5", 0x2000, 0x0800, 0x31818df5 )
	ROM_LOAD( "prg6", 0x2800, 0x0800, 0xa67f1599 )
	ROM_LOAD( "prg7", 0x3000, 0x0800, 0xc56be265 )
	ROM_LOAD( "prg8", 0x3800, 0x0800, 0x61950c2f )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chg1", 0x0000, 0x0800, 0x2a2a6b44 )
	ROM_LOAD( "chg2", 0x0800, 0x0800, 0xb4a4608a )
	ROM_LOAD( "chg3", 0x1000, 0x0800, 0x971c3192 )
	ROM_LOAD( "chg4", 0x1800, 0x0800, 0x7864778e )
ROM_END

ROM_START( pacmod_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacmanh.6e", 0x0000, 0x1000, 0x8200b62a )
	ROM_LOAD( "pacmanh.6f", 0x1000, 0x1000, 0xd800bc8a )
	ROM_LOAD( "pacmanh.6h", 0x2000, 0x1000, 0xd80098fc )
	ROM_LOAD( "pacmanh.6j", 0x3000, 0x1000, 0xbea73c61 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacmanh.5e", 0x0000, 0x1000, 0xcd1138b9 )
	ROM_LOAD( "pacmanh.5f", 0x1000, 0x1000, 0x0f80461c )
ROM_END

ROM_START( pacplus_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacplus.6e", 0x0000, 0x1000, 0x8200be38 )
	ROM_LOAD( "pacplus.6f", 0x1000, 0x1000, 0xd800bc8a )
	ROM_LOAD( "pacplus.6h", 0x2000, 0x1000, 0xd800986c )
	ROM_LOAD( "pacplus.6j", 0x3000, 0x1000, 0xbca63c60 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacplus.5e", 0x0000, 0x1000, 0xd635f515 )
	ROM_LOAD( "pacplus.5f", 0x1000, 0x1000, 0x58751f9d )
ROM_END

ROM_START( hangly_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "hangly.6e", 0x0000, 0x1000, 0x1b05a9d7 )
	ROM_LOAD( "hangly.6f", 0x1000, 0x1000, 0xa1fff4c3 )
	ROM_LOAD( "hangly.6h", 0x2000, 0x1000, 0xb7e9ae83 )
	ROM_LOAD( "hangly.6j", 0x3000, 0x1000, 0xe29b0d5d )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hangly.5e", 0x0000, 0x1000, 0x45346da8 )
	ROM_LOAD( "hangly.5f", 0x1000, 0x1000, 0x0f80461c )
ROM_END

ROM_START( puckman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "puckman.6e", 0x0000, 0x1000, 0xec1056cc )
	ROM_LOAD( "puckman.6f", 0x1000, 0x1000, 0xd800bc8a )
	ROM_LOAD( "puckman.6h", 0x2000, 0x1000, 0x73409a0c )
	ROM_LOAD( "puckman.6j", 0x3000, 0x1000, 0x1f99fa09 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "puckman.5e", 0x0000, 0x1000, 0x45346da8 )
	ROM_LOAD( "puckman.5f", 0x1000, 0x1000, 0x0f80461c )
ROM_END

ROM_START( piranha_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pr1.cpu", 0x0000, 0x1000, 0xafe7d1ef )
	ROM_LOAD( "pr2.cpu", 0x1000, 0x1000, 0xd800bc8a )
	ROM_LOAD( "pr3.cpu", 0x2000, 0x1000, 0xa999a679 )
	ROM_LOAD( "pr4.cpu", 0x3000, 0x1000, 0x7c3cd3da )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pr5.cpu", 0x0000, 0x0800, 0xbbb52019 )
	ROM_LOAD( "pr7.cpu", 0x0800, 0x0800, 0xb1b07de2 )
	ROM_LOAD( "pr6.cpu", 0x1000, 0x0800, 0x8cd0b26c )
	ROM_LOAD( "pr8.cpu", 0x1800, 0x0800, 0xb44bf835 )
ROM_END

ROM_START( mspacman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "boot1", 0x0000, 0x1000, 0xbefc1968 )
	ROM_LOAD( "boot2", 0x1000, 0x1000, 0xee8a0d34 )
	ROM_LOAD( "boot3", 0x2000, 0x1000, 0xd16668e8 )
	ROM_LOAD( "boot4", 0x3000, 0x1000, 0x0652d280 )
	ROM_LOAD( "boot5", 0x8000, 0x1000, 0x5723a645 )
	ROM_LOAD( "boot6", 0x9000, 0x1000, 0xefd154c7 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e",    0x0000, 0x1000, 0x02d51d73 )
	ROM_LOAD( "5f",    0x1000, 0x1000, 0x26da1654 )
ROM_END

ROM_START( mspacatk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mspacatk.1", 0x0000, 0x1000, 0xbefc1968 )
	ROM_LOAD( "mspacatk.2", 0x1000, 0x1000, 0xe800e6f4 )
	ROM_LOAD( "mspacatk.3", 0x2000, 0x1000, 0xd16668e8 )
	ROM_LOAD( "mspacatk.4", 0x3000, 0x1000, 0x0652d280 )
	ROM_LOAD( "mspacatk.5", 0x8000, 0x1000, 0xf98d457b )
	ROM_LOAD( "mspacatk.6", 0x9000, 0x1000, 0x33f15633 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e",    0x0000, 0x1000, 0x02d51d73 )
	ROM_LOAD( "5f",    0x1000, 0x1000, 0x26da1654 )
ROM_END

ROM_START( crush_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "CR1", 0x0000, 0x0800, 0x00f93d3d )
	ROM_LOAD( "CR5", 0x0800, 0x0800, 0x13f49f60 )
	ROM_LOAD( "CR2", 0x1000, 0x0800, 0xa4dfa051 )
	ROM_LOAD( "CR6", 0x1800, 0x0800, 0x934c3836 )
	ROM_LOAD( "CR3", 0x2000, 0x0800, 0x2c2fe2f3 )
	ROM_LOAD( "CR7", 0x2800, 0x0800, 0x6154f01e )
	ROM_LOAD( "CR4", 0x3000, 0x0800, 0x14031ae5 )
	ROM_LOAD( "CR8", 0x3800, 0x0800, 0xa9ada1a1 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CRA", 0x0000, 0x0800, 0xab2e4160 )
	ROM_LOAD( "CRC", 0x0800, 0x0800, 0x09082f80 )
	ROM_LOAD( "CRB", 0x1000, 0x0800, 0x80f4e38a )
	ROM_LOAD( "CRD", 0x1800, 0x0800, 0x49c458f6 )
ROM_END

ROM_START( ponpoko_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ppoko1.bin", 0x0000, 0x1000, 0x31c72f35 )
	ROM_LOAD( "ppoko2.bin", 0x1000, 0x1000, 0xcd981984 )
	ROM_LOAD( "ppoko3.bin", 0x2000, 0x1000, 0xeba2e5ba )
	ROM_LOAD( "ppoko4.bin", 0x3000, 0x1000, 0x4c240e52 )
	ROM_LOAD( "ppoko5.bin", 0x8000, 0x1000, 0xe4781ed8 )
	ROM_LOAD( "ppoko6.bin", 0x9000, 0x1000, 0x2cd69040 )
	ROM_LOAD( "ppoko7.bin", 0xa000, 0x1000, 0xcc5420c8 )
	ROM_LOAD( "ppoko8.bin", 0xb000, 0x1000, 0xe09979bf )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ppoko9.bin",  0x0000, 0x1000, 0x5b359d43 )
	ROM_LOAD( "ppoko10.bin", 0x1000, 0x1000, 0xe3fe3e40 )
ROM_END

ROM_START( eyes_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D7", 0x0000, 0x1000, 0xf98019b2 )
	ROM_LOAD( "E7", 0x1000, 0x1000, 0xa1f59e25 )
	ROM_LOAD( "F7", 0x2000, 0x1000, 0x03bd58d3 )
	ROM_LOAD( "H7", 0x3000, 0x1000, 0x5125cc69 )

	ROM_REGION(0x2000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "D5", 0x0000, 0x1000, 0x8555fbeb )
	ROM_LOAD( "E5", 0x1000, 0x1000, 0x73ec4d68 )
ROM_END



static void ponpoko_decode(void)
{
	int i, j;
	unsigned char *RAM, temp;

	/* The gfx data is swapped wrt the other Pac Man hardware games. */
	/* Here we revert it to the usual format. */

	/* Characters */
	RAM = Machine->memory_region[1];
	for (i = 0; i < 0x1000; i += 0x10)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}

	/* Sprites */
	for (; i < 0x2000; i += 0x20)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x18];
			RAM[i+j+0x18] = RAM[i+j+0x10];
			RAM[i+j+0x10] = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}
}


static void eyes_decode(void)
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	/* Data lines D3 and D5 swapped */
	RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] =  (RAM[i] & 0xc0) | ((RAM[i] & 0x08) << 2) |
				  (RAM[i] & 0x10) | ((RAM[i] & 0x20) >> 2) | (RAM[i] & 0x07);
	}


	/* Graphics ROMs */

	/* Data lines D4 and D6 and address lines A0 and A2 are swapped */
	RAM = Machine->memory_region[1];
	for (i = 0; i < 0x2000; i += 8)
	{
		int j;
		unsigned char swapbuffer[8];

		for (j = 0; j < 8; j++)
		{
			swapbuffer[j] = RAM[i + (j >> 2) + (j & 2) + ((j & 1) << 2)];
		}

		for (j = 0; j < 8; j++)
		{
			char ch = swapbuffer[j];

			RAM[i + j] = (ch & 0x80) | ((ch & 0x10) << 2) |
				         (ch & 0x20) | ((ch & 0x40) >> 2) | (ch & 0x0f);
		}
	}
}



/* waveforms for the audio hardware */
static unsigned char sound_prom[] =
{
	0x07,0x09,0x0A,0x0B,0x0C,0x0D,0x0D,0x0E,0x0E,0x0E,0x0D,0x0D,0x0C,0x0B,0x0A,0x09,
	0x07,0x05,0x04,0x03,0x02,0x01,0x01,0x00,0x00,0x00,0x01,0x01,0x02,0x03,0x04,0x05,
	0x07,0x0C,0x0E,0x0E,0x0D,0x0B,0x09,0x0A,0x0B,0x0B,0x0A,0x09,0x06,0x04,0x03,0x05,
	0x07,0x09,0x0B,0x0A,0x08,0x05,0x04,0x03,0x03,0x04,0x05,0x03,0x01,0x00,0x00,0x02,
	0x07,0x0A,0x0C,0x0D,0x0E,0x0D,0x0C,0x0A,0x07,0x04,0x02,0x01,0x00,0x01,0x02,0x04,
	0x07,0x0B,0x0D,0x0E,0x0D,0x0B,0x07,0x03,0x01,0x00,0x01,0x03,0x07,0x0E,0x07,0x00,
	0x07,0x0D,0x0B,0x08,0x0B,0x0D,0x09,0x06,0x0B,0x0E,0x0C,0x07,0x09,0x0A,0x06,0x02,
	0x07,0x0C,0x08,0x04,0x05,0x07,0x02,0x00,0x03,0x08,0x05,0x01,0x03,0x06,0x03,0x01,
	0x00,0x08,0x0F,0x07,0x01,0x08,0x0E,0x07,0x02,0x08,0x0D,0x07,0x03,0x08,0x0C,0x07,
	0x04,0x08,0x0B,0x07,0x05,0x08,0x0A,0x07,0x06,0x08,0x09,0x07,0x07,0x08,0x08,0x07,
	0x07,0x08,0x06,0x09,0x05,0x0A,0x04,0x0B,0x03,0x0C,0x02,0x0D,0x01,0x0E,0x00,0x0F,
	0x00,0x0F,0x01,0x0E,0x02,0x0D,0x03,0x0C,0x04,0x0B,0x05,0x0A,0x06,0x09,0x07,0x08,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
};



static unsigned char pacplus_color_prom[] =
{
	/* palette */
	0x00,0x3F,0x07,0xEF,0xF8,0x6F,0x38,0xC9,0xAF,0xAA,0x20,0xD5,0xBF,0x5D,0xED,0xF6,
	/* color lookup table */
	0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x02,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x03,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x05,
	0x00,0x00,0x00,0x00,0x00,0x07,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x08,0x00,0x01,0x0B,0x0F,
	0x00,0x08,0x00,0x09,0x00,0x06,0x08,0x07,0x00,0x0F,0x08,0x0F,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x02,0x0D,0x00,0x0F,0x0A,0x06,0x00,0x01,0x0D,0x0C,0x00,0x0B,0x0F,0x0D,
	0x00,0x04,0x03,0x01,0x00,0x0F,0x07,0x00,0x00,0x08,0x00,0x09,0x00,0x08,0x00,0x09,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x08,0x02,0x00,0x0F,0x07,0x08,0x00,0x08,0x00,0x0F
};

static unsigned char pacman_color_prom[] =
{
	/* palette */
	0x00,0x07,0x66,0xEF,0x00,0xF8,0xEA,0x6F,0x00,0x3F,0x00,0xC9,0x38,0xAA,0xAF,0xF6,
	/* color lookup table */
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x01,0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x03,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x05,0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x07,
	0x00,0x00,0x00,0x00,0x00,0x0B,0x01,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0E,0x00,0x01,0x0C,0x0F,
	0x00,0x0E,0x00,0x0B,0x00,0x0C,0x0B,0x0E,0x00,0x0C,0x0F,0x01,0x00,0x00,0x00,0x00,
	0x00,0x01,0x02,0x0F,0x00,0x07,0x0C,0x02,0x00,0x09,0x06,0x0F,0x00,0x0D,0x0C,0x0F,
	0x00,0x05,0x03,0x09,0x00,0x0F,0x0B,0x00,0x00,0x0E,0x00,0x0B,0x00,0x0E,0x00,0x0B,
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0E,0x01,0x00,0x0F,0x0B,0x0E,0x00,0x0E,0x00,0x0F
};

static unsigned char crush_color_prom[] =
{
	/* palette */
	0x00,0x07,0x66,0xEF,0x00,0xF8,0xEA,0x6F,0x00,0x3F,0x00,0xC9,0x38,0xAA,0xAF,0xF6,
	/* color lookup table */
	0x00,0x00,0x00,0x00,0x00,0x0f,0x0b,0x01,0x00,0x0f,0x0b,0x03,0x00,0x0f,0x0b,0x0f,
	0x00,0x0f,0x0b,0x07,0x00,0x0f,0x0b,0x05,0x00,0x0f,0x0b,0x0c,0x00,0x0f,0x0b,0x09,
	0x00,0x05,0x0b,0x07,0x00,0x0b,0x01,0x09,0x00,0x05,0x0b,0x01,0x00,0x02,0x05,0x01,
	0x00,0x02,0x0b,0x01,0x00,0x05,0x0b,0x09,0x00,0x0c,0x01,0x07,0x00,0x01,0x0c,0x0f,
	0x00,0x0f,0x00,0x0b,0x00,0x0c,0x05,0x0f,0x00,0x0f,0x0b,0x0e,0x00,0x0f,0x0b,0x0d,
	0x00,0x01,0x09,0x0f,0x00,0x09,0x0c,0x09,0x00,0x09,0x05,0x0f,0x00,0x05,0x0c,0x0f,
	0x00,0x01,0x07,0x0b,0x00,0x0f,0x0b,0x00,0x00,0x0f,0x00,0x0b,0x00,0x0b,0x05,0x09,
	0x00,0x0b,0x0c,0x0f,0x00,0x0b,0x07,0x09,0x00,0x02,0x0b,0x00,0x00,0x02,0x0b,0x07
};


static void copytoscreen(int mem, int len, int screen, int direction)
{
	char buf[10];
	int hi;

	hi =      (RAM[mem + direction*3] & 0x0f) +
		      (RAM[mem + direction*3] >> 4)   * 10 +
		      (RAM[mem + direction*2] & 0x0f) * 100 +
		      (RAM[mem + direction*2] >> 4)   * 1000 +
		      (RAM[mem + direction*1] & 0x0f) * 10000 +
		      (RAM[mem + direction*1] >> 4)   * 100000;

	if (len == 4)
	{
		hi += (RAM[mem + 0*direction] & 0x0f) * 1000000 +
		      (RAM[mem + 0*direction] >> 4)   * 10000000;
	}

	if (hi)
	{
		sprintf(buf,"%8d",hi);
		if (buf[2] != ' ') videoram00_w(screen + direction*0, buf[2]-'0');
		if (buf[3] != ' ') videoram00_w(screen + direction*1, buf[3]-'0');
		if (buf[4] != ' ') videoram00_w(screen + direction*2, buf[4]-'0');
		if (buf[5] != ' ') videoram00_w(screen + direction*3, buf[5]-'0');
		if (buf[6] != ' ') videoram00_w(screen + direction*4, buf[6]-'0');
		                   videoram00_w(screen + direction*5, buf[7]-'0');
	}
}

static int pacman_hiload(void)
{
	static int resetcount;


	/* during a reset, leave time to the game to clear the screen */
	if (++resetcount < 60) return 0;

	/* wait for "HIGH SCORE" to be on screen */
	if (memcmp(&RAM[0x43d1],"\x48\x47",2) == 0)
	{
		void *f;


		resetcount = 0;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4e88],4);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4e8b, 4, 0x3f2, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void pacman_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4e88],4);
		osd_fclose(f);
	}
}



static int crush_hiload(void)
{
	static int resetcount;


	/* during a reset, leave time to the game to clear the screen */
	if (++resetcount < 60) return 0;

	/* wait for "HI SCORE" to be on screen */
	if (memcmp(&RAM[0x43d0],"\x53\x40",2) == 0)
	{
		void *f;


		resetcount = 0;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4c80],3);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4c83, 3, 0x3f3, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void crush_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4c80],3);
		osd_fclose(f);
	}
}



static int eyes_hiload(void)
{
	if (memcmp(&RAM[0x4d30],"\x90\x52\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4cf7],0x3c);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4cfd, 3, 0x3f2, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void eyes_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4cf7],0x3c);
		osd_fclose(f);
	}
}


static int ponpoko_hiload(void)
{
	if (memcmp(&RAM[0x406c],"\x0f\x0f\x0f\x0f\x0f\x00",6) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4e5a],0x14);
			memcpy(&RAM[0x4c40], &RAM[0x4e5a], 3);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4e59, 3, 0x6c, 1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void ponpoko_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4e5a],0x14);
		osd_fclose(f);
	}
}


#define GAMEDRIVER(NAME, BASENAME, ORIENT, DECODE, DESC, CREDITS)        \
																		 \
struct GameDriver NAME##_driver =										 \
{																		 \
	DESC,																 \
	#NAME,																 \
	CREDITS,															 \
	&machine_driver,													 \
																		 \
	NAME##_rom,															 \
	DECODE, 0,														     \
	0,																	 \
	sound_prom,	/* sound_prom */										 \
																		 \
	BASENAME##_input_ports,												 \
																		 \
	BASENAME##_color_prom, 0, 0,										 \
	ORIENT,																 \
																		 \
	BASENAME##_hiload, BASENAME##_hisave								 \
};


#define mspacman_hiload      pacman_hiload
#define pacplus_hiload       pacman_hiload
#define mspacman_hisave      pacman_hisave
#define pacplus_hisave       pacman_hisave

#define pacplus_input_ports  pacman_input_ports

#define mspacman_color_prom  pacman_color_prom
#define ponpoko_color_prom   pacman_color_prom  /* Probably Correct */
#define eyes_color_prom      pacman_color_prom  /* Wrong !!! */


#define BASE_CREDITS "Allard van der Bas (original code)\nNicola Salmoria (MAME driver)"


GAMEDRIVER(pacman,   pacman,   ORIENTATION_ROTATE_90, 0, "Pac Man (Midway)", BASE_CREDITS)

GAMEDRIVER(namcopac, pacman,   ORIENTATION_ROTATE_90, 0, "Pac Man (Namco)", BASE_CREDITS)

GAMEDRIVER(pacmanjp, pacman,   ORIENTATION_ROTATE_90, 0, "Pac Man (Namco, alternate)", BASE_CREDITS)

GAMEDRIVER(pacmod,   pacman,   ORIENTATION_ROTATE_90, 0, "Pac Man (modified)", BASE_CREDITS)

GAMEDRIVER(pacplus,  pacplus,  ORIENTATION_ROTATE_90, 0, "Pac Man with Pac Man Plus graphics", BASE_CREDITS)

GAMEDRIVER(hangly,   pacman,   ORIENTATION_ROTATE_90, 0, "Hangly Man", BASE_CREDITS)

GAMEDRIVER(puckman,  pacman,   ORIENTATION_ROTATE_90, 0, "Puck Man", BASE_CREDITS)

GAMEDRIVER(piranha,  mspacman, ORIENTATION_ROTATE_90, 0, "Piranha", BASE_CREDITS)

GAMEDRIVER(mspacman, mspacman, ORIENTATION_ROTATE_90, 0, "Ms. Pac Man", BASE_CREDITS)

GAMEDRIVER(mspacatk, mspacman, ORIENTATION_ROTATE_90, 0, "Miss Pac Plus", BASE_CREDITS)

GAMEDRIVER(crush,    crush,    ORIENTATION_ROTATE_90, 0, "Crush Roller", BASE_CREDITS"\nGary Walton (color info)\nSimon Walls (color info)")

GAMEDRIVER(ponpoko,  ponpoko,  ORIENTATION_DEFAULT,   ponpoko_decode, "Ponpoko", BASE_CREDITS)

GAMEDRIVER(eyes,     eyes,     ORIENTATION_ROTATE_90, eyes_decode, "Eyes", "Zsolt Vasvari\n"BASE_CREDITS)

