/***************************************************************************

  Tumblepop (World)     (c) 1991 Data East Corporation
  Tumblepop (Japan)     (c) 1991 Data East Corporation
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 1)
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 2)
  Jump Kids	            (c) 1993 Comad
  Fancy World           (c) 1995 Unico

  Bootleg sound is not quite correct yet (Nothing on bootleg 2).

  If you reset the game while pressing START1 and START2, "VER 0.00 JAPAN"
  is put into tile ram then MAME crashes !

  One of the Jump Kids Sprite roms is bad, same with
  the Sound CPU code, there's one unknown ROM.

  Emulation by Bryan McPhail, mish@tendril.co.uk


Stephh's notes (based on the games M68000 code and some tests) :

1) 'tumblep*' and 'jumpkids'

  - I don't understand the interest of the "Remove Monsters" Dip Switch :
    as I haven't found a way to "end" a level, I guess that it was used to
    test the backgrounds and the "platforms".

  - The "Edit Levels" Dip Switch allows you to add/delete monsters and
    change their position.

    Notes (for 'tumblep', 'tumblepj', 'tumblep2') :
      * "worlds" and levels are 0-based (00-09 & 00-09) :

          World      Name
            0      America
            1      Brazil
            2      Asia
            3      Soviet
            4      Europe
            5      Egypt
            6      Australia
            7      Antartica
            8      Stratosphere
            9      Space

      * As levels x-9 and 9-x are only constitued of a "big boss", you can't
        edit them !
      * All data is stored within the range 0x02b8c8-0x02d2c9, but it should be
        extended to 0x02ebeb (and perhaps 0x02ffff). TO BE CONFIRMED !
      * Once your levels are ready, turn the Dip Switch OFF and reset the game.
      * Of course, there is no possibility to save the levels when you exit
        MAME, nor the way to reload the default ones 8(

    Additional notes (for 'tumblepb') :
      * All data is stored within the range 0x02b8c8-0x02d2c9, but it should be
        extended to 0x02ebeb (and perhaps 0x02ebff). TO BE CONFIRMED !

    Additional notes (for 'jumpkids') :
      * As there are only 9 "worlds", editing "world" 9 ("Space") might cause
        unpredictable weird results !
      * The "worlds" names are the same, but the background is different :

          World      Name            Background
            0      America         Stadium
            1      Brazil          Beach
            2      Asia            Planet
            3      Soviet          Prehistoric Ages
            4      Europe          Castle
            5      Egypt           Pyramids
            6      Australia       Lunar base
            7      Antartica       Bridge
            8      Stratosphere    ???
            9      Space           DOES NOT EXIST !

        As I'm not sure of the description of the background, feel free to
        improve the previous list.
      * All data is stored within the range 0x02776e-0x029207, but it should be
        extended to 0x02ab29 (and perhaps 0x02ab49). TO BE CONFIRMED !


2) 'fncywrld'

  - I'm not sure about the release date of this game :
      * on the title screen, it ALWAYS displays 1996
      * when "Language" Dip Switch is set to "English", there is a (c) 1996 "warning"
        screen, but when it is set to "Korean", there is a (c) 1995 "warning" screen !

  - I don't understand the interest of the "Remove Monsters" Dip Switch :
    as I haven't found a way to "end" a level, I guess that it was used to
    test the backgrounds and the "platforms".

  - The "Edit Levels" Dip Switch allows you to add/delete monsters and
    change their position.

    This needs more investigation to get similar infos to the ones for the other
    games in the driver.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"

#define TUMBLEP_HACK	0
#define FNCYWLD_HACK	0

VIDEO_START( tumblep );
VIDEO_START( fncywld );
VIDEO_UPDATE( tumblep );
VIDEO_UPDATE( tumblepb );
VIDEO_UPDATE( jumpkids );
VIDEO_UPDATE( fncywld );

WRITE16_HANDLER( tumblep_pf1_data_w );
WRITE16_HANDLER( tumblep_pf2_data_w );
WRITE16_HANDLER( fncywld_pf1_data_w );
WRITE16_HANDLER( fncywld_pf2_data_w );
WRITE16_HANDLER( tumblep_control_0_w );

extern data16_t *tumblep_pf1_data,*tumblep_pf2_data;

/******************************************************************************/

static WRITE16_HANDLER( tumblep_oki_w )
{
	OKIM6295_data_0_w(0,data&0xff);
    /* STUFF IN OTHER BYTE TOO..*/
}

static READ16_HANDLER( tumblep_prot_r )
{
	return ~0;
}

static WRITE16_HANDLER( tumblep_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

/******************************************************************************/

static READ16_HANDLER( tumblepop_controls_r )
{
 	switch (offset<<1)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 2: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
		case 8: /* Credits */
			return readinputport(2);
		case 10: /* ? */
		case 12:
        	return 0;
	}

	return ~0;
}

/******************************************************************************/

static MEMORY_READ16_START( tumblepop_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0x320000, 0x320fff, MRA16_RAM },
	{ 0x322000, 0x322fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tumblepop_writemem )
#if TUMBLEP_HACK
	{ 0x000000, 0x07ffff, MWA16_RAM },	// To write levels modifications
#else
	{ 0x000000, 0x07ffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, tumblep_sound_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM, &spriteram16 },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( tumblepopb_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, tumblep_prot_r },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tumblepopb_writemem )
#if TUMBLEP_HACK
	{ 0x000000, 0x07ffff, MWA16_RAM },	// To write levels modifications
#else
	{ 0x000000, 0x07ffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, tumblep_oki_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x160000, 0x1607ff, MWA16_RAM, &spriteram16 }, /* Bootleg sprite buffer */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( fncywld_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x100001, YM2151_status_port_0_lsb_r },
	{ 0x100002, 0x100003, MRA16_NOP }, // ym?
	{ 0x100004, 0x100005, OKIM6295_status_0_lsb_r },
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x320000, 0x321fff, MRA16_RAM },
	{ 0x322000, 0x323fff, MRA16_RAM },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM }, // RAM
MEMORY_END

static MEMORY_WRITE16_START( fncywld_writemem )
#if FNCYWLD_HACK
	{ 0x000000, 0x0fffff, MWA16_RAM },	// To write levels modifications
#else
	{ 0x000000, 0x0fffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, YM2151_register_port_0_lsb_w },
	{ 0x100002, 0x100003, YM2151_data_port_0_lsb_w },
	{ 0x100004, 0x100005, OKIM6295_data_0_lsb_w },
	{ 0x140000, 0x140fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x160000, 0x1607ff, MWA16_RAM, &spriteram16 }, /* sprites */
	{ 0x160800, 0x16080f, MWA16_RAM }, /* goes slightly past the end of spriteram? */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x321fff, fncywld_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x323fff, fncywld_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
	{ 0xff0000, 0xffffff, MWA16_RAM }, // RAM
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

/* Physical memory map (21 bits) */
static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, MRA_NOP }, /* This board only has 1 oki chip */
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP }, /* YM2203 - this board doesn't have one */
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, MWA_NOP },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( tumblep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
#if TUMBLEP_HACK
	PORT_DIPNAME( 0x08, 0x08, "Remove Monsters" )
#else
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )		// See notes
#endif
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#if TUMBLEP_HACK
	PORT_DIPNAME( 0x04, 0x04, "Edit Levels" )
#else
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )		// See notes
#endif
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( fncywld )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )		// duplicated setting
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )	// to be confirmed
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Language" )			// only seems to the title screen
	PORT_DIPSETTING(    0x04, "English" )
	PORT_DIPSETTING(    0x00, "Korean" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )	// to be confirmed
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
#if FNCYWLD_HACK
	PORT_DIPNAME( 0x08, 0x08, "Remove Monsters" )
#else
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )		// See notes
#endif
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#if FNCYWLD_HACK
	PORT_DIPNAME( 0x04, 0x04, "Edit Levels" )
#else
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )		// See notes
#endif
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout tcharlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tlayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 256, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tlayout,     512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &tlayout,     256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo fncywld_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 0x400, 0x40 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tlayout,     0x400, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &tlayout,     0x200, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 0x40 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface2 =
{
	1,          /* 1 chip */
	{ 7757 },   /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 70 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* May not be correct, there is another crystal near the ym2151 */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct OKIM6295interface fncy_okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

static struct YM2151interface fncy_ym2151_interface =
{
	1,
	32220000/9,
	{ YM3012_VOL(20,MIXER_PAN_LEFT,20,MIXER_PAN_RIGHT) },
	{ 0 }
};

static MACHINE_DRIVER_START( tumblep )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepop_readmem,tumblepop_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280, 32220000/8)	/* Custom chip 45; Audio section crystal is 32.220 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblep)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tumblepb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepopb_readmem,tumblepopb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblepb)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jumpkids )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepopb_readmem,tumblepopb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* z80? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(jumpkids)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fncywld )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(fncywld_readmem,fncywld_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(fncywld_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(fncywld)
	MDRV_VIDEO_UPDATE(fncywld)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, fncy_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, fncy_okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( tumblep )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hl00-1.f12", 0x00000, 0x40000, 0xfd697c1b )
	ROM_LOAD16_BYTE("hl01-1.f13", 0x00001, 0x40000, 0xd5a62a3f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, 0xa5cab888 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, 0xdfceaa26 )	// encrypted

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, 0x01b81da0 )
ROM_END

ROM_START( tumblepj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hk00-1.f12", 0x00000, 0x40000, 0x2d3e4d3d )
	ROM_LOAD16_BYTE("hk01-1.f13", 0x00001, 0x40000, 0x56912a00 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, 0xa5cab888 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, 0xdfceaa26 )	// encrypted

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, 0x01b81da0 )
ROM_END

ROM_START( tumblepb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.12", 0x00000, 0x40000, 0x0c984703 )
	ROM_LOAD16_BYTE( "thumbpop.13", 0x00001, 0x40000, 0x864c4053 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, 0x0795aab4 )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, 0xad58df43 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, 0xfabbf15d )
ROM_END

ROM_START( tumblep2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.2", 0x00000, 0x40000, 0x34b016e1 )
	ROM_LOAD16_BYTE( "thumbpop.3", 0x00001, 0x40000, 0x89501c71 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, 0x0795aab4 )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, 0xad58df43 )

 	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, 0xfabbf15d )
ROM_END

ROM_START( jumpkids )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "23.15c", 0x00000, 0x40000, 0x6ba11e91 )
	ROM_LOAD16_BYTE( "24.16c", 0x00001, 0x40000, 0x5795d98b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "23.3c", 0x00000, 0x10000, BADCRC(0xd7dbbd8c) ) // bad

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "30.15j", 0x00000, 0x40000, 0x44b9a089 )
	ROM_LOAD16_BYTE( "29.13j", 0x00001, 0x40000, 0x3f98ec69 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "25.1g",  0x00000, 0x40000, 0x176ae857 )
	ROM_LOAD16_BYTE( "28.1l",  0x00001, 0x40000, BADCRC(0xdc35c5a0) ) // bad
	ROM_LOAD16_BYTE( "26.2g",  0x80000, 0x40000, 0xe8b34980 )
	ROM_LOAD16_BYTE( "27.1j",  0x80001, 0x40000, 0x3918dda3 )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) /* ? */
	ROM_LOAD( "21.1c", 0x00000, 0x80000, 0xe5094f75 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "22.2c", 0x00000, 0x20000, 0xfae44fbf )
ROM_END

ROM_START( fncywld )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "01_fw02.bin", 0x000000, 0x080000, 0xecb978c1 )
	ROM_LOAD16_BYTE( "02_fw03.bin", 0x000001, 0x080000, 0x2d233b42 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE  )
	ROM_LOAD16_BYTE( "05_fw06.bin",  0x00000, 0x40000, 0xe141ecdc )
	ROM_LOAD16_BYTE( "06_fw07.bin",  0x00001, 0x40000, 0x0058a812 )
	ROM_LOAD16_BYTE( "03_fw04.bin",  0x80000, 0x40000, 0x6ad38c14 )
	ROM_LOAD16_BYTE( "04_fw05.bin",  0x80001, 0x40000, 0xb8d079a6 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "08_fw09.bin", 0x00000, 0x40000, 0xa4a00de9 )
	ROM_LOAD16_BYTE( "07_fw08.bin", 0x00001, 0x40000, 0xb48cd1d4 )
	ROM_LOAD16_BYTE( "10_fw11.bin", 0x80000, 0x40000, 0xf21bab48 )
	ROM_LOAD16_BYTE( "09_fw10.bin", 0x80001, 0x40000, 0x6aea8e0f )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "00_fw01.bin", 0x000000, 0x040000, 0xb395fe01 )
ROM_END

/******************************************************************************/

void tumblep_patch_code(UINT16 offset)
{
	/* A hack which enables all Dip Switches effects */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	RAM[(offset + 0)/2] = 0x0240;
	RAM[(offset + 2)/2] = 0xffff;	// andi.w  #$f3ff, D0
}


static void tumblepb_gfx1_decrypt(void)
{
	data8_t *rom = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);
	int i;

	/* gfx data is in the wrong order */
	for (i = 0;i < len;i++)
	{
		if ((i & 0x20) == 0)
		{
			int t = rom[i]; rom[i] = rom[i + 0x20]; rom[i + 0x20] = t;
		}
	}
	/* low/high half are also swapped */
	for (i = 0;i < len/2;i++)
	{
		int t = rom[i]; rom[i] = rom[i + len/2]; rom[i + len/2] = t;
	}
}

static DRIVER_INIT( tumblep )
{
	deco56_decrypt(REGION_GFX1);

	#if TUMBLEP_HACK
	tumblep_patch_code(0x000132);
	#endif
}

static DRIVER_INIT( tumblepb )
{
	tumblepb_gfx1_decrypt();

	#if TUMBLEP_HACK
	tumblep_patch_code(0x000132);
	#endif
}

static DRIVER_INIT( jumpkids )
{
	tumblepb_gfx1_decrypt();

	#if TUMBLEP_HACK
	tumblep_patch_code(0x00013a);
	#endif
}

static DRIVER_INIT( fncywld )
{
	#if FNCYWLD_HACK
	/* This is a hack to allow you to use the extra features
         of the 2 first "Unused" Dip Switch (see notes above). */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	RAM[0x0005fa/2] = 0x4e71;
	RAM[0x00060a/2] = 0x4e71;
	#endif

	tumblepb_gfx1_decrypt();
}


/******************************************************************************/

GAME( 1991, tumblep,  0,       tumblep,   tumblep,  tumblep,  ROT0, "Data East Corporation", "Tumble Pop (World)" )
GAME( 1991, tumblepj, tumblep, tumblep,   tumblep,  tumblep,  ROT0, "Data East Corporation", "Tumble Pop (Japan)" )
GAMEX(1991, tumblepb, tumblep, tumblepb,  tumblep,  tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 1)", GAME_IMPERFECT_SOUND )
GAMEX(1991, tumblep2, tumblep, tumblepb,  tumblep,  tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 2)", GAME_IMPERFECT_SOUND )
GAMEX(1993, jumpkids, 0,       jumpkids,  tumblep,  jumpkids, ROT0, "Comad", "Jump Kids", GAME_NO_SOUND )
GAME (1996, fncywld,  0,       fncywld,   fncywld,  fncywld,  ROT0, "Unico", "Fancy World - Earth of Crisis" ) // game says 1996, testmode 1995?
