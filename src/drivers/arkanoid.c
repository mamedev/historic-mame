/***************************************************************************

    Arkanoid driver (Preliminary)


    Japanese version support cocktail mode (DSW #7), the others don't.

    Here are the versions we have:

    arkanoid    World version, probably an earlier revision
    arknoidu    USA version, probably a later revision; There has been code
                inserted, NOT patched, so I don't think it's a bootleg
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arknoiuo    USA version, probably an earlier revision
                ROM a75-10.bin should be identical to the real World one.
                (It only differs in the country byte from A75-11.ROM)
                This version works fine with the real MCU ROM
    arkatour    Tournament version
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arknoidj    Japanese version with level selector.
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arkbl2      Bootleg of the early Japanese version.
                The only difference is that the warning text has been replaced
                by "WAIT"
                ROM E2.6F should be identical to the real Japanese one.
                (It only differs in the country byte from A75-11.ROM)
                This version works fine with the real MCU ROM
    arkatayt    Another bootleg of the early Japanese one, more heavily modified
    arkblock    Another bootleg of the early Japanese one, more heavily modified
    arkbloc2    Another bootleg
    arkbl3      Another bootleg of the early Japanese one, more heavily modified
    paddle2     Another bootleg of the early Japanese one, more heavily modified
    arkangc     Game Corporation bootleg with level selector


    Most if not all Arkanoid sets have a bug in their game code. It occurs on the
    final level where the player has to dodge falling objects. The bug resides in
    the collision detection routine which sometimes reads from unmapped addresses
    above $F000. For these addresses it is vital to read zero values, or else the
    player will die for no reason.


Stephh's notes (based on the games Z80 code and some tests) :

1) "Game Corporation" bootlegs and assimilated ones.

  - Region = 0x76 (Japan).
  - All bootlegs are based on a (bootleg) version we don't have.
  - Team credits have been replaced by bootleggers code.

1a) 'arkangc'

  - "(c)  Game Corporation 1986".
  - Displays the "Arkanoid" title but routine to display "BLOCK" with bricks exists.
  - No hardware test and no "NOTICE" screen.
  - All reads from 0xf002 are patched.
  - No reads from 0xd008.
  - "Continue" Dip Switch has been replaced by sort of "Debug" Dip Switch :
      * affects ball speed at start of level (0x06 or 0x08)
      * affects level 2 (same as normal version or same as level 30)
  - You can select your starting level (between 1 and 30)
    but they aren't displayed like in the original Japanese set we have ('arknoidj').
  - Level 30 differs from original Japanese version
  - There seems to be code to edit levels (check code at 0x8082), but the routines
    don't seem to be called anymore.
  - Known bugs :
      * The paddle isn't centered when starting a new life and / or level;
        it doesn't "backup" the paddle position when a life is lost as well
        (I can't tell at the moment if it's an ingame bug or not)
        So the paddle can sometimes appear in the left wall !
      * You are told be to able to select your starting level from level 1 to level 32
        (ingame bug - check code at 0x3425)

1b) 'arkblock'

  - Same as 'arkangc', the only difference is that it displays "BLOCK" with bricks
    instead of displaying the "Arkanoid" title :

      Z:\MAME\dasm>diff arkangc.asm arkbloc2.asm
      8421,8422c8421,8424
      < 32EF: 21 80 03      ld   hl,$0380
      < 32F2: CD D1 20      call $20D1
      ---
      > 32EF: F3            di
      > 32F0: CD 90 7C      call $7C90
      > 32F3: C9            ret
      > 32F4: 14            inc  d

1c) 'arkbloc2'

  - "(c)  Game Corporation 1986".
  - Displays "BLOCK" with bricks.
  - No hardware test and no "NOTICE" screen.
  - All reads from 0xf002 are patched.
  - Reads bit 5 from 0xd008.
  - You can select your starting level (between 1 and 30) but they aren't displayed
    like in the original Japanese set we have ('arknoidj').
  - "Continue" Dip Switch has been replaced by sort of "Debug" Dip Switch :
      * affects ball speed at start of level (0x06 or 0x08)
      * affects level 2 (same as normal version or same as level 30)
  - You can select your starting level (between 1 and 30)
    but they aren't displayed like in the original Japanese set we have ('arknoidj').
  - Level 30 differs from original Japanese version (also differs from 'arkangc')
  - Known bugs :
      * You can go from one side of the screen to the other through the walls
        (I can't tell at the moment if it's an ingame bug or not)
      * You are told be to able to select your starting level from level 1 to level 32
        (ingame bug - check code at 0x3425)

1d) 'arkgcbl'

  - "1986    ARKANOID    1986".
  - Displays the "Arkanoid" title but routine to display "BLOCK" with bricks exists.
  - No hardware test and no "NOTICE" screen.
  - Most reads from 0xf002 are patched. I need to fix the remaining ones (0x8a and 0xff).
  - Reads bits 1 and 5 from 0xd008.
  - "Continue" Dip Switch has been replaced by "Round Select" Dip Switch
    ("debug" functions from 'arkangc' have been patched).
  - Different "Bonus Lives" Dip Switch :
      * "60K 100K 60K+" or "60K" when you start a new game
      * "20K 60K 60K+"  or "20K" when you continue
  - Different "Lives" Dip Switch (check table at 0x9a28)
  - Specific coinage (always 2C_1C)
  - If Dip Switch is set, you can select your starting level (between 1 and 30)
    but they aren't displayed like in the original Japanese set we have ('arknoidj').
  - Same level 30 as original Japanese version
  - Known bugs :
      * You can go from one side of the screen to the other through the walls
        (I can't tell at the moment if it's an ingame bug or not)
      * You are told be to able to select your starting level from level 1 to level 32
        (ingame bug - check code at 0x3425)
      * Sound in "Demo Mode" if 1 coin is inserted (ingame bug - check code at 0x0283)
      * Red square on upper middle left "led" when it is supposed to be yellow (ingame bug)

1e) 'paddle2'

  - Different title, year, and inside texts but routine to display "BLOCK" with bricks exists.
  - No hardware test and no "NOTICE" screen.
  - I need to fix ALL reads from 0xf002.
  - Reads bits 0 to 3 and 5 from 0xd008.
  - "Continue" Dip Switch has been replaced by "Round Select" Dip Switch
    ("debug" functions from 'arkangc' have been patched).
  - No more "Service Mode" Dip Switch (even if code is still there for it).
    This Dip Switch now selects how spinners are handled :
      * bit 2 = 0 => read from 0xd018 only
      * bit 2 = 1 => read from 0xd018 + read from 0xd008
    I set its default to 1 as parts of the game still branch to 0x96b0.
  - Different "Bonus Lives" Dip Switch :
      * "60K 100K 60K+" or "60K" when you start a new game
      * "20K 60K 60K+"  or "20K" when you continue
  - Different "Lives" Dip Switch (check table at 0x9a28)
  - If Dip Switch is set, you can select your starting level (between 1 and 30)
    but they aren't displayed like in the original Japanese set we have ('arknoidj').
  - Levels are based on the ones from "Arkanoid II".
  - Known bugs :
      * You can go from one side of the screen to the other through the walls
        (I can't tell at the moment if it's an ingame bug or not)
      * You can't correctly enter your initials at the end of the game
        (ingame bug ? check code at 0xa23e and difference at 0xa273)
      * On intro and last screen, colour around main ship is yellow instead of red
        (ingame bug due to numerous patches)


TO DO (2006.09.05) :

  - Check the following bootlegs (adresses, routines and Dip Switches) :
      * 'arkatayt'
      * 'arkmcubl'
  - Try to include the bootlegs I found
  - Add notes about main addresses and routines in the Z80


Stephh's log (2006.09.05) :

  - Interverted 'arkblock' and 'arkbloc2' sets for better comparaison
  - Renamed sets :
      * 'arkbl2'   -> 'arkmcubl'
      * 'arkbl3'   -> 'arkgcbl'
  - Changed some games descriptions
  - Removed flags from the following sets :
      * 'arkbloc2' (old 'arkblock')
      * 'arkgcbl'  (old 'arkbl3')
      * 'paddle2'
    This way, even if emulation isn't perfect, people can try them and report bugs.


***************************************************************************/

#include "driver.h"
#include "arkanoid.h"
#include "sound/ay8910.h"

int arkanoid_bootleg_id;


/***************************************************************************/

/* Memory Maps */

static ADDRESS_MAP_START( arkanoid_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITE(arkanoid_d008_w)	/* gfx bank, flip screen etc. */
	AM_RANGE(0xd00c, 0xd00c) AM_READ(arkanoid_68705_input_0_r)  /* mainly an input port, with 2 bits from the 68705 */
	AM_RANGE(0xd010, 0xd010) AM_READWRITE(input_port_1_r, watchdog_reset_w)
	AM_RANGE(0xd018, 0xd018) AM_READWRITE(arkanoid_Z80_mcu_r, arkanoid_Z80_mcu_w)  /* input from the 68705 */
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(arkanoid_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe840, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_READNOP	/* fixes instant death in final level */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bootleg_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITE(arkanoid_d008_w)	/* gfx bank, flip screen etc. */
	AM_RANGE(0xd00c, 0xd00c) AM_READ(input_port_0_r)
	AM_RANGE(0xd010, 0xd010) AM_READWRITE(input_port_1_r, watchdog_reset_w)
	AM_RANGE(0xd018, 0xd018) AM_READ(arkanoid_input_2_r) AM_WRITENOP
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(arkanoid_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe840, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_READNOP	/* fixes instant death in final level */
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE(arkanoid_68705_portA_r, arkanoid_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_READ(arkanoid_input_2_r)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(arkanoid_68705_portC_r, arkanoid_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(arkanoid_68705_ddrA_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(arkanoid_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_RAM
	AM_RANGE(0x0080, 0x07ff) AM_ROM
ADDRESS_MAP_END


/***************************************************************************/


/* Input Ports */

#define ARKNOI_IN0\
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input from the 68705, some bootlegs need it to be 1 */\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* input from the 68705 */

#define ARKNOI_IN1\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL\
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define ARKNOI_SPINNERS\
	PORT_START_TAG("IN2")      /* Spinner Player 1 */\
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15)\
	PORT_START_TAG("IN3")      /* Spinner Player 2  */\
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_COCKTAIL

INPUT_PORTS_START( arkanoid )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K 60K 60K+" )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

/* These are the input ports of the real Japanese ROM set                        */
/* 'Block' uses the these ones as well. The Tayto bootleg is different           */
/*  in coinage and # of lives.                                                   */

INPUT_PORTS_START( arknoidj )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K 60K 60K+" )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/* Is the same as arkanoij, but the Coinage,
  Lives and Bonus_Life dips are different */
INPUT_PORTS_START( arkatayt )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "60K 100K 60K+" )
	PORT_DIPSETTING(    0x00, "60K" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END


INPUT_PORTS_START( arkangc )
	PORT_INCLUDE(arknoidj)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Ball Speed" )            /* Speed at 0xc462 (code at 0x18aa) - Also affects level 2 (code at 0x7b82) */
	PORT_DIPSETTING(    0x01, DEF_STR( Normal ) )       /* 0xc462 = 0x06 - Normal level 2 */
	PORT_DIPSETTING(    0x00, "Faster" )                /* 0xc462 = 0x08 - Level 2 same as level 30 */
INPUT_PORTS_END

INPUT_PORTS_START( arkgcbl )
	PORT_INCLUDE(arknoidj)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x00, "Round Select" )          /* Check code at 0x7bc2 - Speed at 0xc462 (code at 0x18aa) */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )          /* 0xc462 = 0x06 */
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )           /* 0xc462 = 0x06 */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )  /* "ld a,$60" at 0x93bd and "ld a,$20" at 0x9c7f and 0x9c9b */
	PORT_DIPSETTING(    0x10, "60K 100K 60K+" )        /* But "20K 60K 60K+" when continue */
	PORT_DIPSETTING(    0x00, "60K" )                  /* But "20K" when continue */
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )       /* Always 2C_1C - check code at 0x7d5e */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( paddle2 )
	PORT_INCLUDE(arknoidj)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x00, "Round Select" )          /* Check code at 0x7bc2 - Speed at 0xc462 (code at 0x18aa) */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )          /* 0xc462 = 0x06 */
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )           /* 0xc462 = 0x06 */
	PORT_DIPNAME( 0x04, 0x04, "Controls ?" )            /* Check code at 0x96a1 and read notes */
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Alternate ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )  /* "ld a,$60" at 0x93bd and "ld a,$20" at 0x9c7f and 0x9c9b */
	PORT_DIPSETTING(    0x10, "60K 100K 60K+" )        /* But "20K 60K 60K+" when continue */
	PORT_DIPSETTING(    0x00, "60K" )                  /* But "20K" when continue */
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
INPUT_PORTS_END


INPUT_PORTS_START( tetrsark )
	/* most of these probably just aren't connected rather than being dipswitches */
	PORT_START_TAG("IN0")
	/*
    PORT_DIPNAME( 0x01, 0x01, "0" )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    */
	PORT_START_TAG("IN1")

	PORT_START_TAG("IN2")

	PORT_START_TAG("IN3")

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) // or up? it rotates the piece.
	/* coinage? */
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) // or up? it rotates the piece.
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) // WTF? it does't work
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) // WTF? it does't work
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


/***************************************************************************/

/* Graphics Layouts */

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 2*4096*8*8, 4096*8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 64 },
	// sprites use the same characters above, but are 16x8
	{ -1 }
};

/* Sound Interfaces */

static struct AY8910interface ay8910_interface =
{
	input_port_5_r,
	input_port_4_r
};

/* Machine Drivers */

static MACHINE_DRIVER_START( arkanoid )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, 6000000)	// 6 MHz ???
	MDRV_CPU_PROGRAM_MAP(arkanoid_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD_TAG("mcu", M68705, 500000)	// .5 MHz (don't know really how fast, but it doesn't need to even be this fast)
	MDRV_CPU_PROGRAM_MAP(mcu_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)					// 100 CPU slices per second to synchronize between the MCU and the main CPU

	MDRV_MACHINE_START(arkanoid)
	MDRV_MACHINE_RESET(arkanoid)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(arkanoid)
	MDRV_VIDEO_UPDATE(arkanoid)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bootleg )
	MDRV_IMPORT_FROM(arkanoid)

	// basic machine hardware
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bootleg_map, 0)

	MDRV_CPU_REMOVE("mcu")
MACHINE_DRIVER_END


/***************************************************************************/

/* ROMs */

ROM_START( arkanoid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-01-1.rom", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a75-11.rom",   0x8000, 0x8000, CRC(eafd7191) SHA1(d2f8843b716718b1de209e97a874e8ce600f3f87) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkanoid.uc",  0x0000, 0x0800, CRC(515d77b6) SHA1(a302937683d11f663abd56a2fd7c174374e4d7fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoidu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-19.bin",   0x0000, 0x8000, CRC(d3ad37d7) SHA1(a172a1ef5bb83ee2d8ed2842ef8968af19ad411e) )
	ROM_LOAD( "a75-18.bin",   0x8000, 0x8000, CRC(cdc08301) SHA1(05f54353cc8333af14fa985a2764960e20e8161a) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arknoidu.uc",  0x0000, 0x0800, BAD_DUMP CRC(de518e47) SHA1(b8eddd1c566505fb69e3d1207c7a9720dfb9f503)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoiuo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-01-1.rom", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a75-10.rom",   0x8000, 0x8000, CRC(a1769e15) SHA1(fbb45731246a098b29eb08de5d63074b496aaaba) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkanoid.uc",  0x0000, 0x0800, CRC(515d77b6) SHA1(a302937683d11f663abd56a2fd7c174374e4d7fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkatour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "t_ark1.bin",   0x0000, 0x8000, CRC(e3b8faf5) SHA1(4c09478fa41881fa89ee6afb676aeb780f17ac2e) )
	ROM_LOAD( "t_ark2.bin",   0x8000, 0x8000, CRC(326aca4d) SHA1(5a194b7a0361236d471b24905dc6434372f81252) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkatour.uc",  0x0000, 0x0800, BAD_DUMP CRC(d3249559) SHA1(b1542764450016614e9e03cedd6a2f1e59961789)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "t_ark3.bin",   0x00000, 0x8000, CRC(5ddea3cf) SHA1(58f16515898b7cc2697bf7663a60d9ca0db6da95) )
	ROM_LOAD( "t_ark4.bin",   0x08000, 0x8000, CRC(5fcf2e85) SHA1(f721f0afb0550cc64bff26681856a7576398d9b5) )
	ROM_LOAD( "t_ark5.bin",   0x10000, 0x8000, CRC(7b76b192) SHA1(a68aa08717646a6c322cf3455df07f50df9e9f33) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoidj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-21.rom",   0x0000, 0x8000, CRC(bf0455fc) SHA1(250522b84b9f491c3f4efc391bf6aa6124361369) )
	ROM_LOAD( "a75-22.rom",   0x8000, 0x8000, CRC(3a2688d3) SHA1(9633a661352def3d85f95ca830f6d761b0b5450e) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arknoidj.uc",  0x0000, 0x0800, BAD_DUMP CRC(0a4abef6) SHA1(fdce0b7a2eab7fd4f1f4fc3b93120b1ebc16078e)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkmcubl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "e1.6d",        0x0000, 0x8000, CRC(dd4f2b72) SHA1(399a8636030a702dafc1da926f115df6f045bef1) )
	ROM_LOAD( "e2.6f",        0x8000, 0x8000, CRC(bbc33ceb) SHA1(e9b6fef98d0d20e77c7a1c25eff8e9a8c668a258) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "68705p3.6i",   0x0000, 0x0800, CRC(389a8cfb) SHA1(9530c051b61b5bdec7018c6fdc1ea91288a406bd) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkangc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkgc.1",      0x0000, 0x8000, CRC(c54232e6) SHA1(beb759cee68009a06824b755d2aa26d7d436b5b0) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkblock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ark-6.bin",    0x0000, 0x8000, CRC(0be015de) SHA1(f4209085b59d2c96a62ac9657c7bf097da55362b) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkbloc2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "block01.bin",  0x0000, 0x8000, CRC(5be667e1) SHA1(fbc5c97d836c404a2e6c007c3836e36b52ae75a1) )
	ROM_LOAD( "block02.bin",  0x8000, 0x8000, CRC(4f883ef1) SHA1(cb090a57fc75f17a3e2ba637f0e3ec93c1d02cea) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkgcbl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkanunk.1",   0x0000, 0x8000, CRC(b0f73900) SHA1(2c9a36cc1d2a3f33ec81d63c1c325554b818d2d3) )
	ROM_LOAD( "arkanunk.2",   0x8000, 0x8000, CRC(9827f297) SHA1(697874e73e045eb5a7bf333d7310934b239c0adf) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( paddle2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "paddle2.16",   0x0000, 0x8000, CRC(a286333c) SHA1(0b2c9cb0df236f327413d0c541453e1ba979ea38) )
	ROM_LOAD( "paddle2.17",   0x8000, 0x8000, CRC(04c2acb5) SHA1(7ce8ba31224f705b2b6ed0200404ef5f8f688001) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkatayt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic81.3f",   0x0000, 0x8000, CRC(6e0a2b6f) SHA1(5227d7a944cb1e815f60ec87a67f7462870ff9fe) )
	ROM_LOAD( "ic82.5f",   0x8000, 0x8000, CRC(5a97dd56) SHA1(b71c7b5ced2b0eebbcc5996dd21a1bb1c2da4819) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-ic33.2c",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "2-ic34.3c",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "3-ic35.5c",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "ic73.11e",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "ic74.12e",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "ic75.13e",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

/* the other Dr. Korea game (Hexa, hexa.c) also appears to be derived from Arkanoid hardware */

ROM_START( tetrsark )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )		/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "ic17.1",      0x00000, 0x8000, CRC(1a505eda) SHA1(92f171a12cf0c326d29c244514718df04b998426) )
	ROM_LOAD( "ic16.2",      0x08000, 0x8000, CRC(157bc4df) SHA1(b2c704148e7e3ca61ab51308ee0d66ea1088bff3) ) // it doens't care if this is here??

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic64.3",      0x00000, 0x8000, CRC(c3e9b290) SHA1(6e99520606c654e531dbeb9a598cfbb443c24dff) )
	ROM_LOAD( "ic63.4",      0x08000, 0x8000, CRC(de9a368f) SHA1(ffbb2479200648da3f3e7ab7cebcdb604f6dfb3d) )
	ROM_LOAD( "ic62.5",      0x10000, 0x8000, CRC(c8e80a00) SHA1(4bee4c36ee768ae68ebc64e639fdc43f61c74f92) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END


/* Driver Initialization */

static void arkanoid_bootleg_init( void )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf002, 0xf002, 0, 0, arkanoid_bootleg_f002_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd018, 0xd018, 0, 0, arkanoid_bootleg_d018_w );
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd008, 0xd008, 0, 0, arkanoid_bootleg_d008_r );
}

static DRIVER_INIT( arkangc )
{
	arkanoid_bootleg_id = ARKANGC;
	arkanoid_bootleg_init();
}

static DRIVER_INIT( arkblock )
{
	arkanoid_bootleg_id = ARKBLOCK;
	arkanoid_bootleg_init();
}

static DRIVER_INIT( arkbloc2 )
{
	arkanoid_bootleg_id = ARKBLOC2;
	arkanoid_bootleg_init();
}

static DRIVER_INIT( arkgcbl )
{
	arkanoid_bootleg_id = ARKGCBL;
	arkanoid_bootleg_init();
}

static DRIVER_INIT( paddle2 )
{
	arkanoid_bootleg_id = PADDLE2;
	arkanoid_bootleg_init();
}


static DRIVER_INIT( tetrsark )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	int x;

	for (x=0;x<0x8000;x++)
	{
		ROM[x]=ROM[x]^0x94;
	}
}


/* Game Drivers */

GAME( 1986, arkanoid, 0,        arkanoid, arkanoid, 0,        ROT90, "Taito Corporation Japan", "Arkanoid (World)", GAME_SUPPORTS_SAVE )
GAME( 1986, arknoidu, arkanoid, arkanoid, arkanoid, 0,        ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US)", GAME_SUPPORTS_SAVE )
GAME( 1986, arknoiuo, arkanoid, arkanoid, arkanoid, 0,        ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US, older)", GAME_SUPPORTS_SAVE )
GAME( 1986, arknoidj, arkanoid, arkanoid, arknoidj, 0,        ROT90, "Taito Corporation", "Arkanoid (Japan)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkmcubl, arkanoid, arkanoid, arknoidj, 0,        ROT90, "bootleg", "Arkanoid (bootleg with MCU)", GAME_NOT_WORKING )
GAME( 1986, arkangc,  arkanoid, bootleg,  arkangc,  arkangc,  ROT90, "bootleg", "Arkanoid (Game Corporation bootleg)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkblock, arkanoid, bootleg,  arkangc,  arkblock, ROT90, "bootleg", "Block (Game Corporation bootleg, set 1)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkbloc2, arkanoid, bootleg,  arkangc,  arkbloc2, ROT90, "bootleg", "Block (Game Corporation bootleg, set 2)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkgcbl,  arkanoid, bootleg,  arkgcbl,  arkgcbl,  ROT90, "bootleg", "Arkanoid (bootleg on Block hardware)", GAME_SUPPORTS_SAVE )
GAME( 1988, paddle2,  arkanoid, bootleg,  paddle2,  paddle2,  ROT90, "bootleg", "Paddle 2 (bootleg on Block hardware)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkatayt, arkanoid, bootleg,  arkatayt, 0,        ROT90, "bootleg", "Arkanoid (Tayto bootleg)", GAME_SUPPORTS_SAVE )
GAME( 1987, arkatour, arkanoid, arkanoid, arkanoid, 0,        ROT90, "Taito America Corporation (Romstar license)", "Tournament Arkanoid (US)", GAME_SUPPORTS_SAVE )
GAME( 19??, tetrsark, 0,        bootleg,  tetrsark, tetrsark, ROT0,  "D.R. Korea", "Tetris (D.R. Korea)", GAME_SUPPORTS_SAVE | GAME_WRONG_COLORS )

