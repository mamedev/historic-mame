/*** Tecmo Bowl (c)1987 Tecmo

driver by David Haywood
wip 20/01/2002

Tecmo Bowl was a popular 4 player American Football game with two screens and
attractive graphics

--- Current Issues

Sprites Not Done
Sound Incomplete (what plays the sample roms)

Game Doesn't Behave Correctly
  Most Inputs seem to be ignored once you start a game?
  The Game only does one move / throw then just seems to
  sit there counting the clock down.
  When the clock reaches 0 the game doesn't end.
  Not sure why this is happening, incorrect shared ram?
  incorrect banking? (its from wc90.c / tecmo.c and seems
  quite common to tecmo even if some bits are unused)
  Communication Failure? Protection?  Me Missing Something
  blindingly obvious? NMI Routine of Main CPU?

NMI routine of 6206B (1st CPU) is NEVER called at the moment!

***/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

/* in vidhrdw/tbowl.c */
extern data8_t *tbowl_txvideoram, *tbowl_bgvideoram, *tbowl_bg2videoram;
WRITE_HANDLER (tbowl_bg2videoram_w);
WRITE_HANDLER (tbowl_bgvideoram_w);
WRITE_HANDLER (tbowl_txvideoram_w);

WRITE_HANDLER (tbowl_bg2xscroll_lo); WRITE_HANDLER (tbowl_bg2xscroll_hi);
WRITE_HANDLER (tbowl_bg2yscroll_lo); WRITE_HANDLER (tbowl_bg2yscroll_hi);
WRITE_HANDLER (tbowl_bgxscroll_lo);  WRITE_HANDLER (tbowl_bgxscroll_hi);
WRITE_HANDLER (tbowl_bgyscroll_lo);  WRITE_HANDLER (tbowl_bgyscroll_hi);

VIDEO_START( tbowl );
VIDEO_UPDATE( tbowl );

/*** Banking

note: check this, its borrowed from tecmo.c / wc90.c at the moment and could well be wrong

***/

static WRITE_HANDLER( tbowlb_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	cpu_setbank(1,&RAM[bankaddress]);
}

static WRITE_HANDLER( tbowlc_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);


	bankaddress = 0x10000 + ((data & 0xf8) << 8);


	cpu_setbank(2,&RAM[bankaddress]);
}

/*** Shared Ram Handlers

***/

static unsigned char *shared_ram;

static READ_HANDLER( shared_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( shared_w )
{
	shared_ram[offset] = data;
}

static WRITE_HANDLER( tbowl_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
}


/*** Memory Structures

	Board B is the main board, reading inputs, and in control of the 2 bg layers & text layer etc.
	Board C is the sub board, main job is the sprites
	Board A is for the sound

***/

/* Board B */

static MEMORY_READ_START( readmem_6206B )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM }, /* RAM 1 */
	{ 0xa000, 0xbfff, MRA_RAM }, /* RAM 1 */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_BANK1 }, /* Banked ROM */
	{ 0xf800, 0xfbff, shared_r }, /* RAM 2 */
	{ 0xfc00, 0xfc00, input_port_0_r }, // Player 1 inputs
	{ 0xfc01, 0xfc01, input_port_1_r }, // Player 2 inputs
	{ 0xfc02, 0xfc02, input_port_2_r }, // Player 3 inputs
	{ 0xfc03, 0xfc03, input_port_3_r }, // Player 4 inputs
//	{ 0xfc06, 0xfc06, dummy_r }, // Read During NMI
	{ 0xfc07, 0xfc07, input_port_4_r }, // System inputs
	{ 0xfc08, 0xfc08, input_port_5_r }, // DSW1
	{ 0xfc09, 0xfc09, input_port_6_r }, // DSW2
	{ 0xfc0a, 0xfc0a, input_port_7_r }, // DSW3
MEMORY_END

static MEMORY_WRITE_START( writemem_6206B )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, tbowl_bg2videoram_w, &tbowl_bg2videoram },
	{ 0xc000, 0xdfff, tbowl_bgvideoram_w, &tbowl_bgvideoram },
	{ 0xe000, 0xefff, tbowl_txvideoram_w, &tbowl_txvideoram },
//	{ 0xf000, 0xf000, unknown_write },* written during start-up, not again */
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, shared_w, &shared_ram }, /* check */
	{ 0xfc00, 0xfc00, tbowlb_bankswitch_w },
//	{ 0xfc01, 0xfc01, unknown_write }, /* written during start-up, not again */
//	{ 0xfc02, 0xfc02, unknown_write }, /* written during start-up, not again */
	{ 0xfc0d, 0xfc0d, tbowl_sound_command_w }, /* not sure, used quite a bit */
//	{ 0xfc05, 0xfc05, unknown_write }, /* no idea */
//	{ 0xfc08, 0xfc08, unknown_write }, /* hardly uesd .. */
//	{ 0xfc0a, 0xfc0a, unknown_write }, /* hardly uesd .. */
	{ 0xfc10, 0xfc10, tbowl_bg2xscroll_lo },
	{ 0xfc11, 0xfc11, tbowl_bg2xscroll_hi },
	{ 0xfc12, 0xfc12, tbowl_bg2yscroll_lo },
	{ 0xfc13, 0xfc13, tbowl_bg2yscroll_hi },
	{ 0xfc14, 0xfc14, tbowl_bgxscroll_lo },
	{ 0xfc15, 0xfc15, tbowl_bgxscroll_hi },
	{ 0xfc16, 0xfc16, tbowl_bgyscroll_lo },
	{ 0xfc17, 0xfc17, tbowl_bgyscroll_hi },
MEMORY_END

/* Board C */

static MEMORY_READ_START( readmem_6206C )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM }, /* not read? */
	{ 0xf000, 0xf7ff, MRA_BANK2 }, /* Banked ROM */
	{ 0xf800, 0xfbff, shared_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_6206C )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, shared_w },
	{ 0xfc00, 0xfc00, tbowlc_bankswitch_w },
	{ 0xfc01, 0xfc01, MWA_NOP }, /* ? */
	{ 0xfc02, 0xfc02, MWA_NOP }, /* ? */
	{ 0xfc03, 0xfc03, MWA_NOP }, /* ? */
	{ 0xfc06, 0xfc06, MWA_NOP }, /* ? */
MEMORY_END

/* Board A */

static MEMORY_READ_START( readmem_6206A )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe010, 0xe010, soundlatch_r },

MEMORY_END

static MEMORY_WRITE_START( writemem_6206A )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, YM3812_control_port_0_w },
	{ 0xd001, 0xd001, YM3812_write_port_0_w },
	{ 0xd800, 0xd800, YM3812_control_port_1_w },
	{ 0xd801, 0xd801, YM3812_write_port_1_w },
//	{ 0xe000, 0xe000, unknown_write },
//	{ 0xe001, 0xe001, unknown_write },
//	{ 0xe002, 0xe002, unknown_write },
//	{ 0xe003, 0xe003, unknown_write },
//	{ 0xe004, 0xe004, unknown_write },
//	{ 0xe005, 0xe005, unknown_write },
//	{ 0xe006, 0xe006, unknown_write },
//	{ 0xe007, 0xe007, unknown_write },
/* rest of sound is probably similar to tecmo.c */
MEMORY_END

/*** Input Ports

Haze's notes :

There are controls for 4 players, each player has 4 directions and 2 buttons as
well as coin and start.  The service switch acts as inserting a coin for all
4 players, the dipswitches are listed in the manual


Steph's notes (2002.02.12) :

I haven't found any manual, so my notes only rely on the Z80 code ...

  -- Inputs --

According to the Z80 code, here is the list of controls for each player :

  - NO START button (you need to press BUTTON1n to start a game for player n)
  - 4 or 8 directions (I can't tell for the moment, so I've chosen 8 directions)
  - 2 buttons (I can't tell for the moment what they do)

There are also 1 coin slot and a "Service" button for each player.

1 "credit" will mean <<time defined by the "Player Time" Dip Switch>>.

COINn adds one COIN for player n. When the number of coins fit the "Coinage"
Dip Switch, 1 "credit" will be added.

SERVICEn adds 1 "credit" for player n.

There is also a GENERAL "Service" switch that adds 1 "credit" for ALL players.
I've mapped it to the F1 key. If you have a better key and/or a better
description for it, feel free to change it.

  -- Dip Switches --

According to the Z80 code, what is called "Difficulty" Dip Switch (DSW 1
bits 0 and 1) doesn't seem to be tested. BTW, where did you find such info ?

I haven't been able to determine yet the effect(s) of DSW3 bits 2 and 3.
Could it be the "Difficulty" you mentioned that is HERE instead of DSW1
bits 0 and 1 ? I'll try to have another look when the sprites stuff is finished.

***/


#define TBOWL_PLAYER_INPUT(_n_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START##_n_ )


INPUT_PORTS_START( tbowl )
	PORT_START	/* player 1 inputs (0xfc00) */
	TBOWL_PLAYER_INPUT(1)

	PORT_START	/* player 2 inputs (0xfc01) */
	TBOWL_PLAYER_INPUT(2)

	PORT_START	/* player 3 inputs (0xfc02) */
	TBOWL_PLAYER_INPUT(3)

	PORT_START	/* player 4 inputs (0xfc03) */
	TBOWL_PLAYER_INPUT(4)

	PORT_START	/* system inputs (0xfc07 -> 0x80f9) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Service (General)", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 (0xfc08 -> 0xffb4) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING (   0x01, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING (   0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING (   0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING (   0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING (   0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xf8, 0xb8, "Time (Players)" )
	PORT_DIPSETTING (   0x00, "7:00" )
	PORT_DIPSETTING (   0x08, "6:00" )
	PORT_DIPSETTING (   0x10, "5:00" )
	PORT_DIPSETTING (   0x18, "4:30" )
	PORT_DIPSETTING (   0x20, "3:40" )
	PORT_DIPSETTING (   0x28, "3:20" )
	PORT_DIPSETTING (   0x30, "3:00" )
	PORT_DIPSETTING (   0x38, "2:50" )
	PORT_DIPSETTING (   0x40, "2:40" )
	PORT_DIPSETTING (   0x48, "2:30" )
	PORT_DIPSETTING (   0x50, "2:20" )
	PORT_DIPSETTING (   0x58, "2:10" )
	PORT_DIPSETTING (   0x60, "2:00" )
	PORT_DIPSETTING (   0x68, "1:55" )
	PORT_DIPSETTING (   0x70, "1:50" )
	PORT_DIPSETTING (   0x78, "1:45" )
	PORT_DIPSETTING (   0x80, "1:40" )
	PORT_DIPSETTING (   0x88, "1:35" )
	PORT_DIPSETTING (   0x90, "1:25" )
	PORT_DIPSETTING (   0x98, "1:20" )
	PORT_DIPSETTING (   0xa0, "1:15" )
	PORT_DIPSETTING (   0xa8, "1:10" )
	PORT_DIPSETTING (   0xb0, "1:05" )
	PORT_DIPSETTING (   0xb8, "1:00" )
	PORT_DIPSETTING (   0xc0, "0:55" )
	PORT_DIPSETTING (   0xc8, "0:50" )
	PORT_DIPSETTING (   0xd0, "0:45" )
	PORT_DIPSETTING (   0xd8, "0:40" )
	PORT_DIPSETTING (   0xe0, "0:35" )
	PORT_DIPSETTING (   0xe8, "0:30" )
	PORT_DIPSETTING (   0xf0, "0:25" )
//	PORT_DIPSETTING (   0xf8, "1:00" )

	PORT_START	/* DSW2 (0xfc09 -> 0xffb5) */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty (unused ?)" )	// To be checked again
	PORT_DIPSETTING (   0x00, "0x00" )
	PORT_DIPSETTING (   0x01, "0x01" )
	PORT_DIPSETTING (   0x02, "0x02" )
	PORT_DIPSETTING (   0x03, "0x03" )
	PORT_DIPNAME( 0x0c, 0x0c, "Extra Time (Players)" )	// For multiple "credits"
	PORT_DIPSETTING (   0x00, "0:30" )
	PORT_DIPSETTING (   0x04, "0:20" )
	PORT_DIPSETTING (   0x08, "0:10" )
	PORT_DIPSETTING (   0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Timer Speed" )
	PORT_DIPSETTING (   0x00, "56/60" )
	PORT_DIPSETTING (   0x10, "51/60" )
	PORT_DIPSETTING (   0x30, "47/60" )
	PORT_DIPSETTING (   0x20, "42/60" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )	// Check code at 0x0393
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Hi-Score Reset" )		// Only if P1 buttons 1 and 2 are pressed during P.O.S.T. !
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START	/* DSW3 (0xfc0a -> 0xffb6) */
	PORT_DIPNAME( 0x03, 0x03, "Time (Quarter)" )
	PORT_DIPSETTING (   0x00, "8:00" )
	PORT_DIPSETTING (   0x01, "5:00" )
	PORT_DIPSETTING (   0x03, "4:00" )
	PORT_DIPSETTING (   0x02, "3:00" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown (in 0x8126.w)" )	// Check code at 0x6e16
	PORT_DIPSETTING (   0x00, "0x00 = 0x54f3" )
	PORT_DIPSETTING (   0x04, "0x04 = 0x54e1" )
	PORT_DIPSETTING (   0x08, "0x08 = 0x54cf" )
	PORT_DIPSETTING (   0x0c, "0x0c = 0x54bd" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* same as 'tbowl', but different "Quarter Time" Dip Switch
   ("3:00" and "4:00" are inverted) */
INPUT_PORTS_START( tbowlj )
	PORT_START	/* player 1 inputs (0xfc00) */
	TBOWL_PLAYER_INPUT(1)

	PORT_START	/* player 2 inputs (0xfc01) */
	TBOWL_PLAYER_INPUT(2)

	PORT_START	/* player 3 inputs (0xfc02) */
	TBOWL_PLAYER_INPUT(3)

	PORT_START	/* player 4 inputs (0xfc03) */
	TBOWL_PLAYER_INPUT(4)

	PORT_START	/* system inputs (0xfc07 -> 0x80f9) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Service (General)", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 (0xfc08 -> 0xffb4) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING (   0x01, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING (   0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING (   0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING (   0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING (   0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xf8, 0xb8, "Time (Players)" )
	PORT_DIPSETTING (   0x00, "7:00" )
	PORT_DIPSETTING (   0x08, "6:00" )
	PORT_DIPSETTING (   0x10, "5:00" )
	PORT_DIPSETTING (   0x18, "4:30" )
	PORT_DIPSETTING (   0x20, "3:40" )
	PORT_DIPSETTING (   0x28, "3:20" )
	PORT_DIPSETTING (   0x30, "3:00" )
	PORT_DIPSETTING (   0x38, "2:50" )
	PORT_DIPSETTING (   0x40, "2:40" )
	PORT_DIPSETTING (   0x48, "2:30" )
	PORT_DIPSETTING (   0x50, "2:20" )
	PORT_DIPSETTING (   0x58, "2:10" )
	PORT_DIPSETTING (   0x60, "2:00" )
	PORT_DIPSETTING (   0x68, "1:55" )
	PORT_DIPSETTING (   0x70, "1:50" )
	PORT_DIPSETTING (   0x78, "1:45" )
	PORT_DIPSETTING (   0x80, "1:40" )
	PORT_DIPSETTING (   0x88, "1:35" )
	PORT_DIPSETTING (   0x90, "1:25" )
	PORT_DIPSETTING (   0x98, "1:20" )
	PORT_DIPSETTING (   0xa0, "1:15" )
	PORT_DIPSETTING (   0xa8, "1:10" )
	PORT_DIPSETTING (   0xb0, "1:05" )
	PORT_DIPSETTING (   0xb8, "1:00" )
	PORT_DIPSETTING (   0xc0, "0:55" )
	PORT_DIPSETTING (   0xc8, "0:50" )
	PORT_DIPSETTING (   0xd0, "0:45" )
	PORT_DIPSETTING (   0xd8, "0:40" )
	PORT_DIPSETTING (   0xe0, "0:35" )
	PORT_DIPSETTING (   0xe8, "0:30" )
	PORT_DIPSETTING (   0xf0, "0:25" )
//	PORT_DIPSETTING (   0xf8, "1:00" )

	PORT_START	/* DSW2 (0xfc09 -> 0xffb5) */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty (unused ?)" )	// To be checked again
	PORT_DIPSETTING (   0x00, "0x00" )
	PORT_DIPSETTING (   0x01, "0x01" )
	PORT_DIPSETTING (   0x02, "0x02" )
	PORT_DIPSETTING (   0x03, "0x03" )
	PORT_DIPNAME( 0x0c, 0x0c, "Extra Time (Players)" )	// For multiple "credits"
	PORT_DIPSETTING (   0x00, "0:30" )
	PORT_DIPSETTING (   0x04, "0:20" )
	PORT_DIPSETTING (   0x08, "0:10" )
	PORT_DIPSETTING (   0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Timer Speed" )
	PORT_DIPSETTING (   0x00, "56/60" )
	PORT_DIPSETTING (   0x10, "51/60" )
	PORT_DIPSETTING (   0x30, "47/60" )
	PORT_DIPSETTING (   0x20, "42/60" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )	// Check code at 0x0393
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Hi-Score Reset" )		// Only if P1 buttons 1 and 2 are pressed during P.O.S.T. !
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START	/* DSW3 (0xfc0a -> 0xffb6) */
	PORT_DIPNAME( 0x03, 0x03, "Time (Quarter)" )
	PORT_DIPSETTING (   0x00, "8:00" )
	PORT_DIPSETTING (   0x01, "5:00" )
	PORT_DIPSETTING (   0x02, "4:00" )
	PORT_DIPSETTING (   0x03, "3:00" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown (in 0x8126.w)" )	// Check code at 0x6e37
	PORT_DIPSETTING (   0x00, "0x00 = 0x5414" )
	PORT_DIPSETTING (   0x04, "0x04 = 0x5402" )
	PORT_DIPSETTING (   0x08, "0x08 = 0x54f0" )
	PORT_DIPSETTING (   0x0c, "0x0c = 0x54de" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*** Graphic Decodes

***/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,	/* tile size */
	RGN_FRAC(1,1),	/* number of tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4,},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32},
	128*8	/* offset to next tile */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   256, 16 },
	{ REGION_GFX2, 0, &spritelayout, 768, 16 },
	{ REGION_GFX2, 0, &spritelayout, 512, 16 },
	{ REGION_GFX3, 0, &spritelayout, 0,   16 },
	{ -1 } /* end of array */
};

/*** Sound Bits

There should also be something for playing the samples (roms 2+3)

*/

static void irqhandler(int linestate)
{
	cpu_set_irq_line(2,0,linestate);
}

static struct YM3526interface ym3812_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz */
	{ 80, 80 },		/* volume */
	{ irqhandler }
};

/*** Machine Driver

there are 3 boards, each with a cpu, boards b and c contain
NEC D70008AC-8's which is just a Z80, board a (the sound board)
has an actual Z80 chip

Sound Hardware should be 2 YM3812's, this isn't done yet

The game is displayed on 2 monitors

***/

static MACHINE_DRIVER_START( tbowl )

	/* CPU on Board '6206B' */
	MDRV_CPU_ADD(Z80, 8000000) /* NEC D70008AC-8 (Z80 Clone) */
	MDRV_CPU_MEMORY(readmem_6206B,writemem_6206B)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	/* CPU on Board '6206C' */
	MDRV_CPU_ADD(Z80, 8000000) /* NEC D70008AC-8 (Z80 Clone) */
	MDRV_CPU_MEMORY(readmem_6206C,writemem_6206C)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	/* CPU on Board '6206A' */
	MDRV_CPU_ADD(Z80, 4000000) /* Actual Z80 */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_6206A,writemem_6206A)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tbowl)
	MDRV_VIDEO_UPDATE(tbowl)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	/* something for the samples? */
MACHINE_DRIVER_END


/* Board Layout from readme.txt

6206A
+-----------------------------------+
|                                   |
|                                   |
|                                   |
|       Z80                         |
|                                   |
|       1                           |
|                                   |
| 3                                 |
|                                   |
| 2                                 |
+-----------------------------------+

6206B
+-----------------------------------+
|                                   |
|                                   |
|                                   |
|   10          6                   |
|   11          7                   |
|   12          8                   |
|   13          9                   |
|                                   |
|                                   |
|                   NEC D70008AC-8  |
|                       4           |
|                       5           |
| 14                                |
| 15                                |
|                                   |
+-----------------------------------+

6206C
+-----------------------------------+
|                                   |
|                                   |
|                                   |
|   D70008AC-8                      |
|   24                              |
|   25                              |
|                                   |
|                   20  16          |
|                   21  17          |
|                   22  18          |
|                   23  19          |
+-----------------------------------+

*/

/*** Rom Loading ***

we currently have two dumps, one appears to be a world/us version, the
other is clearly a Japan version as it displays a regional warning

there is also a bad dump which for reference has the following roms
different to the world dump

	"24.rom" 0x10000 0x39a2d923 (code)
	"25.rom" 0x10000 0x9a0a9cd6 (code / data)

	"21.rom" 0x10000 0x93651858 (gfx)
	"22.rom" 0x10000 0xee7561d9 (gfx)
	"23.rom" 0x10000 0x46b3c186 (gfx)

this fails its rom check so I assume its corrupt

***/


ROM_START( tbowl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* NEC D70008AC-8 (Z80 Clone) */
	ROM_LOAD( "4.11b",	    0x00000, 0x08000, 0xdb8a4f5d )
	ROM_LOAD( "6206b.5",	0x10000, 0x10000, 0x133c5c11 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* NEC D70008AC-8 (Z80 Clone) */
	ROM_LOAD( "6206c.24",	0x00000, 0x10000, 0x040c8138 )
	ROM_LOAD( "6206c.25",	0x10000, 0x10000, 0x92c3cef5 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Z80 */
	ROM_LOAD( "6206a.1",	0x00000, 0x08000, 0x4370207b )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Characters inc. Alphanumerics */
	ROM_LOAD16_BYTE( "14.13l",	    0x00000, 0x08000, 0xf9cf60b9 )
	ROM_LOAD16_BYTE( "15.15l",	    0x00001, 0x08000, 0xa23f6c53 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* BG GFX */
	ROM_LOAD16_BYTE( "6206b.6",	    0x40001, 0x10000, 0xb9615ffa )
	ROM_LOAD16_BYTE( "6206b.8",	    0x40000, 0x10000, 0x6389c719 )
	ROM_LOAD16_BYTE( "6206b.7",	    0x00001, 0x10000, 0xd139c397 )
	ROM_LOAD16_BYTE( "6206b.9",	    0x00000, 0x10000, 0x975ded4c )
	ROM_LOAD16_BYTE( "6206b.10",    0x60001, 0x10000, 0x9b4fa82e )
	ROM_LOAD16_BYTE( "6206b.12",    0x60000, 0x10000, 0x7d0030f6 )
	ROM_LOAD16_BYTE( "6206b.11",    0x20001, 0x10000, 0x06bf07bb )
	ROM_LOAD16_BYTE( "6206b.13",    0x20000, 0x10000, 0x4ad72c16 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* SPR GFX */
	ROM_LOAD16_BYTE( "6206c.16",	0x00001, 0x10000, 0x1a2fb925 )
	ROM_LOAD16_BYTE( "6206c.20",	0x00000, 0x10000, 0x70bb38a3 )
	ROM_LOAD16_BYTE( "6206c.17",	0x20001, 0x10000, 0xde16bc10 )
	ROM_LOAD16_BYTE( "6206c.21",	0x20000, 0x10000, 0x41b2a910 )
	ROM_LOAD16_BYTE( "6206c.18",	0x40001, 0x10000, 0x0684e188 )
	ROM_LOAD16_BYTE( "6206c.22",	0x40000, 0x10000, 0xcf660ebc )
	ROM_LOAD16_BYTE( "6206c.19",	0x60001, 0x10000, 0x71795604 )
	ROM_LOAD16_BYTE( "6206c.23",	0x60000, 0x10000, 0x97fba168 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "6206a.2",	0x00000, 0x10000, 0x1e9e5936 )
	ROM_LOAD( "6206a.3",	0x10000, 0x10000, 0x3aa24744 )
ROM_END

ROM_START( tbowlj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* NEC D70008AC-8 (Z80 Clone) */
	ROM_LOAD( "6206b.4",	0x00000, 0x08000, 0x7ed3eff7 )
	ROM_LOAD( "6206b.5",	0x10000, 0x10000, 0x133c5c11 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* NEC D70008AC-8 (Z80 Clone) */
	ROM_LOAD( "6206c.24",	0x00000, 0x10000, 0x040c8138 )
	ROM_LOAD( "6206c.25",	0x10000, 0x10000, 0x92c3cef5 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Z80 */
	ROM_LOAD( "6206a.1",	0x00000, 0x08000, 0x4370207b )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Characters inc. Alphanumerics */
	ROM_LOAD16_BYTE( "6206b.14",	0x00000, 0x08000, 0xcf99d0bf )
	ROM_LOAD16_BYTE( "6206b.15",	0x00001, 0x08000, 0xd69248cf )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* BG GFX */
	ROM_LOAD16_BYTE( "6206b.6",	    0x40001, 0x10000, 0xb9615ffa )
	ROM_LOAD16_BYTE( "6206b.8",	    0x40000, 0x10000, 0x6389c719 )
	ROM_LOAD16_BYTE( "6206b.7",	    0x00001, 0x10000, 0xd139c397 )
	ROM_LOAD16_BYTE( "6206b.9",	    0x00000, 0x10000, 0x975ded4c )
	ROM_LOAD16_BYTE( "6206b.10",    0x60001, 0x10000, 0x9b4fa82e )
	ROM_LOAD16_BYTE( "6206b.12",    0x60000, 0x10000, 0x7d0030f6 )
	ROM_LOAD16_BYTE( "6206b.11",    0x20001, 0x10000, 0x06bf07bb )
	ROM_LOAD16_BYTE( "6206b.13",    0x20000, 0x10000, 0x4ad72c16 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* SPR GFX */
	ROM_LOAD16_BYTE( "6206c.16",	0x00001, 0x10000, 0x1a2fb925 )
	ROM_LOAD16_BYTE( "6206c.20",	0x00000, 0x10000, 0x70bb38a3 )
	ROM_LOAD16_BYTE( "6206c.17",	0x20001, 0x10000, 0xde16bc10 )
	ROM_LOAD16_BYTE( "6206c.21",	0x20000, 0x10000, 0x41b2a910 )
	ROM_LOAD16_BYTE( "6206c.18",	0x40001, 0x10000, 0x0684e188 )
	ROM_LOAD16_BYTE( "6206c.22",	0x40000, 0x10000, 0xcf660ebc )
	ROM_LOAD16_BYTE( "6206c.19",	0x60001, 0x10000, 0x71795604 )
	ROM_LOAD16_BYTE( "6206c.23",	0x60000, 0x10000, 0x97fba168 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "6206a.2",	0x00000, 0x10000, 0x1e9e5936 )
	ROM_LOAD( "6206a.3",	0x10000, 0x10000, 0x3aa24744 )
ROM_END

GAME( 1987, tbowl,    0,        tbowl,    tbowl,    0, ROT0,  "Tecmo", "Tecmo Bowl (World?)" )
GAME( 1987, tbowlj,   tbowl,    tbowl,    tbowlj,   0, ROT0,  "Tecmo", "Tecmo Bowl (Japan)" )
