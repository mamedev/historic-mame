/***************************************************************************

Top Speed / Full Throttle    (c) Taito 1987
-------------------------

David Graves

Sources:		Rastan driver by Jarek Burczynski
			MAME Taito F2 & Z drivers
			Raine source - special thanks to Richard Bush
			  and the Raine Team.

				*****

Top Speed / Full Throttle is the forerunner of the Taito Z system on
which Taito's driving games were based from 1988-91. (You can spot some
similarities with Continental Circus, the first of the TaitoZ games.)

The game hardware has 5 separate layers of graphics - four 64x64 tiled
scrolling background planes of 8x8 tiles (two of which are used for
drawing the road), and a sprite plane.

Taito got round the limitations of the tilemap generator they were using
(which only supports two layers) by using a pair of them.

[Trivia: Taito employed the same trick three years later, this time with
the TC0100SCN in "Thunderfox".]

Top Speed's sprites are 16x8 tiles aggregated through a RAM sprite map
area into 128x128 big sprites. (The TaitoZ system also used this sprite
map system, but moved the sprite map from RAM to ROM.)

Top Speed has twin 68K CPUs which communicate via $10000 bytes of
shared ram.

The first 68000 handles screen, palette and sprites, and the road.

The second 68000 handles inputs/dips, and does data processing in
shared ram to relieve CPUA.

There is also a Z80, which takes over sound duties. Commands are
written to it by the first 68000.


Dumper's info (topspedu)
-------------

Main CPUs: Dual 68000
Sound: YM2151, OKI M5205

Some of the custom Taito chips look like Rastan Hardware

Comments: Note b14-06, and b14-07, are duplicated twice on this board
for some type of hardware graphics reasons. (DG: that's because of
the twin tilemap generator chips. Can someone confirm they are
PC080SN's please...?)

There is a weird chip that is probably a Microcontroller made by Sharp.
Part number: b14-31 - Sharp LH763J-70


TODO Lists
==========

Extra effects to make road "move"? The unknown 0xffff memory
area may relate to this. First 0x800 of this looks as though it
contains per-pixel-row information for the two road tilemaps.
It consists of words from 0xffe0 to 0x001f (-31 to +31).
These change quite a bit.

Currently road tile colors are in the range 0x100-104 (?).
I can't see how these extra offsets would add to this to
produce a per-pixel-row color, since color bytes > 0x104 are
unused parts of the palette.

*Loads* of complaints from the Taito sound system in the log.

CPUA (on all variants) could have a spin_until_int at $63a.

DIPs


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rastan_spriteflip_w );

int  topspeed_vh_start(void);
void topspeed_vh_stop(void);
void topspeed_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE_HANDLER( rastan_adpcm_trigger_w );
WRITE_HANDLER( rastan_c000_w );
WRITE_HANDLER( rastan_d000_w );

static int old_cpua_ctrl = 0xff;
static int ioc220_port = 0;

//static data16_t *topspeed_ram;
extern data16_t *topspeed_spritemap;

static size_t sharedram_size;
static data16_t *sharedram;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static WRITE16_HANDLER( cpua_ctrl_w )	// assumes Z80 sandwiched between 68Ks
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp, no longer necessary */

	/* bit 0 enables cpu B */

	if ((data &0x1)!=(old_cpua_ctrl &0x1))	// perhaps unnecessary but it's often written with same value
		cpu_set_reset_line(2,(data &0x1) ? CLEAR_LINE : ASSERT_LINE);

	/* is there an irq enable ??? */

	old_cpua_ctrl = data;

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",cpu_get_pc(),data);
}


/***********************************************************
				INTERRUPTS
***********************************************************/

/* 68000 A */

void topspeed_interrupt6(int x)
{
	cpu_cause_interrupt(0,6);
}

/* 68000 B */

void topspeed_cpub_interrupt6(int x)
{
	cpu_cause_interrupt(2,6);	// assumes Z80 sandwiched between the 68Ks
}


/***** Routines for particular games *****/

static int topspeed_interrupt(void)
{
	// Unsure how many int6's per frame
	timer_set(TIME_IN_CYCLES(200000-500,0),0, topspeed_interrupt6);
	return 5;
}

static int topspeed_cpub_interrupt(void)
{
	// Unsure how many int6's per frame
	timer_set(TIME_IN_CYCLES(200000-500,0),0, topspeed_cpub_interrupt6);
	return 5;
}



/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( topspeed_ioc_r )
{
	UINT16 steer = 0;

	if (input_port_4_word_r(0) & 0x8)	/* pressing down */
		steer = 0xff40;

	if (input_port_4_word_r(0) & 0x2)	/* pressing right */
		steer = 0x007f;

	if (input_port_4_word_r(0) & 0x1)	/* pressing left */
		steer = 0xff80;

	/* To allow hiscore input we must let you return to
	   continuous input type while you press up */

	if (input_port_4_word_r(0) & 0x4)	/* pressing up */
		steer = input_port_5_word_r(0);

	switch (offset)
	{
		case 0x00:
			{
				switch (ioc220_port & 0xf)
				{
					case 0x00:
						return input_port_2_word_r(0);	/* DSW A */

					case 0x01:
						return input_port_3_word_r(0);	/* DSW B */

					case 0x02:
						return input_port_0_word_r(0);	/* IN0 */

					case 0x03:
						return input_port_1_word_r(0);	/* IN1 */

					case 0x0c:
						return steer &0xff;

					case 0x0d:
						return steer >> 8;
				}

logerror("CPU #1 PC %06x: warning - read unmapped ioc220 port %02x\n",cpu_get_pc(),ioc220_port);
					return 0;
			}

		case 0x01:
			return ioc220_port;
	}

	return 0x00;	// keep compiler happy
}

static WRITE16_HANDLER( topspeed_ioc_w )
{
	switch (offset)
	{
		case 0x00:
			// write to ioc [coin lockout etc.?]
			break;

		case 0x01:
			ioc220_port = data &0xff;
	}
}

static READ16_HANDLER( topspeed_unknown_r )
{
	return 0x55;
}


/*****************************************************
				SOUND
*****************************************************/

static WRITE_HANDLER( topspeed_bankswitch_w )	// assumes Z80 sandwiched between 68Ks
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

#ifdef MAME_DEBUG
	if (banknum>3) logerror("CPU#3 (Z80) switch to ROM bank %06x: should only happen if Z80 prg rom is 128K!\n",banknum);
#endif
	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/


static MEMORY_READ16_START( topspeed_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_r },	// this block of ram seems to be all shared
	{ 0x500000, 0x503fff, paletteram16_word_r },
	{ 0x7e0000, 0x7e0001, MRA16_NOP },
	{ 0x7e0002, 0x7e0003, taitosound_comm16_lsb_r },
	{ 0x800000, 0x80ffff, MRA16_RAM },	// unknown, road related?
	{ 0xa00000, 0xa0ffff, PC080SN_word_0_r },	/* tilemaps */
	{ 0xb00000, 0xb0ffff, PC080SN_word_1_r },	/* tilemaps */
	{ 0xd00000, 0xd00fff, MRA16_RAM },	/* sprite ram */
	{ 0xe00000, 0xe0ffff, MRA16_RAM },	/* sprite map */
MEMORY_END

static MEMORY_WRITE16_START( topspeed_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x500000, 0x503fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x600002, 0x600003, cpua_ctrl_w },
	{ 0x7e0000, 0x7e0001, taitosound_port16_lsb_w },
	{ 0x7e0002, 0x7e0003, taitosound_comm16_lsb_w },
	{ 0x800000, 0x80ffff, MWA16_RAM },	// unknown, road related?
	{ 0xa00000, 0xa0ffff, PC080SN_word_0_w },
	{ 0xa20000, 0xa20003, PC080SN_yscroll_word_0_w },
	{ 0xa40000, 0xa40003, PC080SN_xscroll_word_0_w },
	{ 0xa50000, 0xa50003, PC080SN_ctrl_word_0_w },
	{ 0xb00000, 0xb0ffff, PC080SN_word_1_w },
	{ 0xb20000, 0xb20003, PC080SN_yscroll_word_1_w },
	{ 0xb40000, 0xb40003, PC080SN_xscroll_word_1_w },
	{ 0xb50000, 0xb50003, PC080SN_ctrl_word_1_w },
	{ 0xd00000, 0xd00fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xe00000, 0xe0ffff, MWA16_RAM, &topspeed_spritemap },
MEMORY_END

static MEMORY_READ16_START( topspeed_cpub_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_r },
	{ 0x880000, 0x880003, topspeed_ioc_r },
	{ 0x900202, 0x900203, topspeed_unknown_r },	/* cockpit status ? */
MEMORY_END

static MEMORY_WRITE16_START( topspeed_cpub_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x400000, 0X40ffff, sharedram_w, &sharedram },
	{ 0x880000, 0x880003, topspeed_ioc_w },
//	{ 0x900000, 0x90002f, topspeed_unknown_w },	/* cockpit motors ? */
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, rastan_adpcm_trigger_w },
	{ 0xc000, 0xc000, rastan_c000_w },
	{ 0xd000, 0xd000, rastan_d000_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( topspeed )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// main brake key

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	// nitro
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	// gear shift lo/hi
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// main accel key

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit ?" )
	PORT_DIPSETTING(    0x02, "Standard Cockpit ?" )
	PORT_DIPSETTING(    0x01, "Upright / Steering Lock ?" )
	PORT_DIPSETTING(    0x00, "Upright / No Steering Lock ?" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* fake inputs, used for steering */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 13, 3, 0xff7f, 0x80)
INPUT_PORTS_END

INPUT_PORTS_START( topspedu )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// main brake key

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	// nitro
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	// gear shift lo/hi
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// main accel key

	PORT_START /* DSW A, coinage varies between countries */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit ?" )
	PORT_DIPSETTING(    0x02, "Standard Cockpit ?" )
	PORT_DIPSETTING(    0x01, "Upright / Steering Lock ?" )
	PORT_DIPSETTING(    0x00, "Upright / No Steering Lock ?" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* fake inputs, used for steering */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 13, 3, 0xff7f, 0x80)
INPUT_PORTS_END

INPUT_PORTS_START( fullthrl )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// main brake key

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	// nitro
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	// gear shift lo/hi
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// main accel key

	PORT_START /* DSW A, coinage varies between countries */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit ?" )
	PORT_DIPSETTING(    0x02, "Standard Cockpit ?" )
	PORT_DIPSETTING(    0x01, "Upright / Steering Lock ?" )
	PORT_DIPSETTING(    0x00, "Upright / No Steering Lock ?" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* fake inputs, used for steering */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 13, 3, 0xff7f, 0x80)
INPUT_PORTS_END

/**************************************************************
				GFX DECODING
**************************************************************/

static struct GfxLayout tile16x8_layout =
{
	16,8,	/* 16*8 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo topspeed_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },		/* sprites & playfield */
	// Road Lines gfxdecodable ?
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2151 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irq_handler },
	{ topspeed_bankswitch_w }
};


static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,       /* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 60 }
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static struct MachineDriver machine_driver_topspeed =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			topspeed_readmem,topspeed_writemem,0,0,
			topspeed_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_readmem, z80_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2151 */
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			topspeed_cpub_readmem,topspeed_cpub_writemem,0,0,
			topspeed_cpub_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	topspeed_gfxdecodeinfo,
	8192, 8192,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	topspeed_vh_start,
	topspeed_vh_stop,
	topspeed_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};



/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( topspeed )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b14-67-1", 0x00000, 0x10000, 0x23f17616 )
	ROM_LOAD16_BYTE( "b14-68-1", 0x00001, 0x10000, 0x835659d9 )
	ROM_LOAD16_BYTE( "b14-54",   0x80000, 0x20000, 0x172924d5 )	/* 4 data roms */
	ROM_LOAD16_BYTE( "b14-52",   0x80001, 0x20000, 0xe1b5b2a1 )
	ROM_LOAD16_BYTE( "b14-55",   0xc0000, 0x20000, 0xa1f15499 )
	ROM_LOAD16_BYTE( "b14-53",   0xc0001, 0x20000, 0x04a04f5f )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-69",   0x00000, 0x10000, 0xd652e300 )
	ROM_LOAD16_BYTE( "b14-70",   0x00001, 0x10000, 0xb720592b )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25", 0x00000, 0x04000, 0x9eab28ef )
	ROM_CONTINUE(       0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07",   0x00000, 0x20000, 0xc6025fff )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06",   0x00001, 0x20000, 0xb4e2536e )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROMX_LOAD( "b14-48", 0x000003, 0x20000, 0x30c7f265, ROM_SKIP(7) )	/* OBJ, bitplane 3 */
	ROMX_LOAD( "b14-49", 0x100003, 0x20000, 0x32ba4265, ROM_SKIP(7) )
	ROMX_LOAD( "b14-50", 0x000007, 0x20000, 0xec1ef311, ROM_SKIP(7) )
	ROMX_LOAD( "b14-51", 0x100007, 0x20000, 0x35041c5f, ROM_SKIP(7) )

	ROMX_LOAD( "b14-44", 0x000002, 0x20000, 0x9f6c030e, ROM_SKIP(7) )	/* OBJ, bitplane 2 */
	ROMX_LOAD( "b14-45", 0x100002, 0x20000, 0x63e4ce03, ROM_SKIP(7) )
	ROMX_LOAD( "b14-46", 0x000006, 0x20000, 0xd489adf2, ROM_SKIP(7) )
	ROMX_LOAD( "b14-47", 0x100006, 0x20000, 0xb3a1f75b, ROM_SKIP(7) )

	ROMX_LOAD( "b14-40", 0x000001, 0x20000, 0xfa2a3cb3, ROM_SKIP(7) )	/* OBJ, bitplane 1 */
	ROMX_LOAD( "b14-41", 0x100001, 0x20000, 0x09455a14, ROM_SKIP(7) )
	ROMX_LOAD( "b14-42", 0x000005, 0x20000, 0xab51f53c, ROM_SKIP(7) )
	ROMX_LOAD( "b14-43", 0x100005, 0x20000, 0x1e6d2b38, ROM_SKIP(7) )

	ROMX_LOAD( "b14-36", 0x000000, 0x20000, 0x20a7c1b8, ROM_SKIP(7) )	/* OBJ, bitplane 0 */
	ROMX_LOAD( "b14-37", 0x100000, 0x20000, 0x801b703b, ROM_SKIP(7) )
	ROMX_LOAD( "b14-38", 0x000004, 0x20000, 0xde0c213e, ROM_SKIP(7) )
	ROMX_LOAD( "b14-39", 0x100004, 0x20000, 0x798c28c5, ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b14-30",  0x00000, 0x10000, 0xdccb0c7f )	/* road gfx ?? */

// One dump has this 0x10000 long, but just contains the same stuff repeated 8 times //
	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "b14-31",  0x0000,  0x2000,  0x5c6b013d )	/* microcontroller ? */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28",  0x00000, 0x10000, 0xdf11d0ae )
	ROM_LOAD( "b14-29",  0x10000, 0x10000, 0x7ad983e7 )
ROM_END

ROM_START( topspedu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE     ( "b14-23", 0x00000, 0x10000, 0xdd0307fd )
	ROM_LOAD16_BYTE     ( "b14-24", 0x00001, 0x10000, 0xacdf08d4 )
	ROM_LOAD16_WORD_SWAP( "b14-05", 0x80000, 0x80000, 0x6557e9d8 )	/* data rom */

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-26", 0x00000, 0x10000, 0x659dc872 )
	ROM_LOAD16_BYTE( "b14-56", 0x00001, 0x10000, 0xd165cf1b )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25", 0x00000, 0x04000, 0x9eab28ef )
	ROM_CONTINUE(       0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07", 0x00000, 0x20000, 0xc6025fff )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06", 0x00001, 0x20000, 0xb4e2536e )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b14-01", 0x00000, 0x80000, 0x84a56f37 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b14-02", 0x00001, 0x80000, 0x6889186b )
	ROM_LOAD32_BYTE( "b14-03", 0x00002, 0x80000, 0xd1ed9e71 )
	ROM_LOAD32_BYTE( "b14-04", 0x00003, 0x80000, 0xb63f0519 )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b14-30", 0x00000, 0x10000, 0xdccb0c7f )	/* road gfx ?? */

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "b14-31", 0x0000,  0x2000,  0x5c6b013d )	/* microcontroller ? */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28", 0x00000, 0x10000, 0xdf11d0ae )
	ROM_LOAD( "b14-29", 0x10000, 0x10000, 0x7ad983e7 )
ROM_END

ROM_START( fullthrl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE     ( "b14-67", 0x00000, 0x10000, 0x284c943f )
	ROM_LOAD16_BYTE     ( "b14-68", 0x00001, 0x10000, 0x54cf6196 )
	ROM_LOAD16_WORD_SWAP( "b14-05", 0x80000, 0x80000, 0x6557e9d8 )	/* data rom */

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-69", 0x00000, 0x10000, 0xd652e300 )
	ROM_LOAD16_BYTE( "b14-71", 0x00001, 0x10000, 0xf7081727 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25", 0x00000, 0x04000, 0x9eab28ef )
	ROM_CONTINUE(       0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07", 0x00000, 0x20000, 0xc6025fff )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06", 0x00001, 0x20000, 0xb4e2536e )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b14-01", 0x00000, 0x80000, 0x84a56f37 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b14-02", 0x00001, 0x80000, 0x6889186b )
	ROM_LOAD32_BYTE( "b14-03", 0x00002, 0x80000, 0xd1ed9e71 )
	ROM_LOAD32_BYTE( "b14-04", 0x00003, 0x80000, 0xb63f0519 )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b14-30", 0x00000, 0x10000, 0xdccb0c7f )	/* road gfx ?? */

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "b14-31", 0x0000,  0x2000,  0x5c6b013d )	/* microcontroller ? */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28", 0x00000, 0x10000, 0xdf11d0ae )
	ROM_LOAD( "b14-29", 0x10000, 0x10000, 0x7ad983e7 )
ROM_END


void init_topspeed(void)
{
//	taitosnd_setz80_soundcpu( 2 );
	old_cpua_ctrl = 0xff;
}


GAME( 1987, topspeed, 0,        topspeed, topspeed, topspeed, ROT0, "Taito Corporation Japan", "Top Speed (World)" )
GAME( 1987, topspedu, topspeed, topspeed, topspedu, topspeed, ROT0, "Taito America Corporation (Romstar license)", "Top Speed (US)" )
GAME( 1987, fullthrl, topspeed, topspeed, fullthrl, topspeed, ROT0, "Taito Corporation", "Full Throttle (Japan)" )

