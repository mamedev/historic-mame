/*******************************************************************************

Equites           (c) 1984 Alpha Denshi Co./Sega
Bull Fighter      (c) 1984 Alpha Denshi Co./Sega
The Koukouyakyuh  (c) 1985 Alpha Denshi Co.
Splendor Blast    (c) 1985 Alpha Denshi Co.
High Voltage      (c) 1985 Alpha Denshi Co.

drivers by Acho A. Tang


Stephh's notes (based on the games M68000 code and some tests) :

0) all games

  - To enter sort of "test mode", bits 0 and 1 need to be ON when the game is reset.
    Acho said that it could be a switch (but I'm not sure of that), and that's why
    I've added a EASY_TEST_MODE compilation switch.


1) 'equites'

  - When in "test mode", press START1 to cycle through next sound, and press START2
    to directly test the inputs and the Dip Switches.
  - When the number of buttons is set to 2, you need to press BOTH BUTTON1 and
    BUTTON2 to have the same effect as BUTTON3.


2) 'bullfgtr'

  - When in "test mode", press START1 to cycle through next sound, and press START2
    to directly test the inputs and the Dip Switches.
  - I'm not sure I understand how the coinage is handled, and so it's hard to make
    a good description. Anyway, the values are correct.


3) 'kouyakyu'

  - When in "test mode", press START1 to cycle through next sound, and press START2
    to directly test the inputs and the Dip Switches.
  - Bit 1 of Dip Switch is only read in combinaison of bit 0 during P.O.S.T. to
    enter the "test mode", but it doesn't add any credit ! That's why I've patched
    the inputs, so you can enter the "test mode" by pressing COIN1 during P.O.S.T.


4) 'splndrbt'

  - When starting a 2 players game, when player 1 game is over, the game enters in
    an infinite loop on displaying the "GAME OVER" message.
  - You can test player 2 by putting 0xff instead of 0x00 at 0x040009 ($9,A6).
  - FYI, what should change the contents of $9,A6 is the routine at 0x000932,
    but I haven't found where this routine could be called 8( 8303 issue ?


5) 'hvoltage'

  - When starting a 2 players game, when player 1 game is over, the game becomes
    buggy on displaying the "GAME OVER" message and you can't start a new game
    anymore.
  - You can test player 2 by putting 0xff instead of 0x00 at 0x040009 ($9,A6).
  - FYI, what should change the contents of $9,A6 is the routine at 0x000fc4,
    but I haven't found where this routine could be called 8( 8404 issue ?

  - There is sort of "debug mode" that you can access if 0x000038.w returns 0x0000
    instead of 0xffff. To enable it, turn HVOLTAGE_HACK to 1 then enable the fake
    Dip Switch.
  - When you are in "debug mode", the Inputs and Dip Switches have special features.
    Here is IMO the full list :

      * pressing IPT_JOYSTICK_DOWN of player 2 freezes the game
      * pressing IPT_JOYSTICK_UP of player 2 unfreezes the game
      * pressing IPT_COIN1 gives invulnerability (the collision routine isn't called)
      * pressing IPT_COIN2 speeds up the game and you don't need to kill the bosses
      * when bit 2 is On, you are given invulnerability (same effect as IPT_COIN1)
      * when bit 3 is On, you don't need to kill the bosses (only the last one)
      * when bit 4 is On ("Lives" Dip Switch set to "5"), some coordonates are displayed
      * when bit 7 is On ("Coinage" Dip Switch set to "A 1/3C B 1/6C" or "A 2/1C B 3/1C"),
        a "band" is displayed at the left of the screen


TO DO :

  - support screen flipping in 'equites' and 'splndrbt'.


Revisions:

xx-xx-2002 (trial release)


Hardware Deficiencies
---------------------

- Lack of 8303/8404 tech info. All MCU results are wild guesses.

  equites_8404rule(unsigned pc, int offset, int data) details:

	    pc: 68000 code address where the program queries the MCU
	offset: 8404 memory offset(in bytes) from where MCU data is read
	  data: fake byte-value to return (negative numbers trigger special conditions)

- Bull Fighter's RGB PROMs need redump. (make-up's included *inaccurate*)
- The Koukouyakyuh's epr-6706.bin needs redump. (fixed ROM included)


Emulation Deficiencies
----------------------

- Equites has a one-frame sprite lag in either the X or Y direction depends on interrupt timing.
- High Voltage and Splendor Blast have unexplanable sprite gaps that require verification.
- MSM5232 clock speed and capacitor values are not known.
- There seems to be a rheostat on Equites' soundboard to adjust the MSM5232's music pitch.
- It hasn't been confirmed whether music tempos are the same across all games.


* Special Thanks to:

  Jarek Burczynski for a superb MSM5232 emulation
  The Equites WIP webmasters(Maccy? Lone Wolf?) for all the vital screenshots and sound clips
  Yasuhiro Ogawa for the correct Equites ROM information


Other unemulated Alpha Denshi games that may use similar hardware:
-----------------------------------------------------------
Year Genre Name             Japanese Name
-----------------------------------------------------------
1984 (SPT) Champion Croquet チャンピオンクロッケー
1985 (SPT) Exciting Soccer2 エキサイティングサッカー2
1985 (???) Tune Pit(?)      チェーンピット
1985 (MAJ) Perfect Janputer パーフェクトジャンピューター


*******************************************************************************/
// Directives

#include "driver.h"
#include "vidhrdw/generic.h"

#define HVOLTAGE_HACK	0
#define EASY_TEST_MODE	0

// Equites Hardware
#define BMPAD 8

/******************************************************************************/
// Imports

// Common Hardware Start
#define EQUITES_ADD_SOUNDBOARD7 \
	MDRV_CPU_ADD(8085A, 5000000) \
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) \
	MDRV_CPU_MEMORY(equites_s_readmem, equites_s_writemem) \
	MDRV_CPU_PORTS(0, equites_s_writeport) \
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 4000) \
	MDRV_SOUND_ADD(MSM5232, equites_5232intf) \
	MDRV_SOUND_ADD(AY8910, equites_8910intf) \
	MDRV_SOUND_ADD(DAC, equites_dacintf)

extern void equites_8404init(void);
extern void equites_8404rule(unsigned pc, int offset, int data);

extern READ16_HANDLER(equites_8404_r);
extern WRITE_HANDLER(equites_5232_w);
extern WRITE_HANDLER(equites_8910control_w);
extern WRITE_HANDLER(equites_8910data_w);
extern WRITE_HANDLER(equites_dac0_w);
extern WRITE_HANDLER(equites_dac1_w);

extern data16_t *equites_8404ram;
extern struct MSM5232interface equites_5232intf;
extern struct AY8910interface equites_8910intf;
extern struct DACinterface equites_dacintf;

static MEMORY_READ_START( equites_s_readmem )
	{ 0x0000, 0xbfff, MRA_ROM }, // sound program
	{ 0xc000, 0xc000, soundlatch_r },
	{ 0xe000, 0xe0ff, MRA_RAM }, // stack and variables
MEMORY_END

static MEMORY_WRITE_START( equites_s_writemem )
	{ 0x0000, 0xbfff, MWA_ROM }, // sound program
	{ 0xc080, 0xc08d, equites_5232_w },
	{ 0xc0a0, 0xc0a0, equites_8910data_w },
	{ 0xc0a1, 0xc0a1, equites_8910control_w },
	{ 0xc0b0, 0xc0b0, MWA_NOP }, // INTR: sync with main melody
	{ 0xc0c0, 0xc0c0, MWA_NOP }, // INTR: sync with specific beats
	{ 0xc0d0, 0xc0d0, equites_dac0_w },
	{ 0xc0e0, 0xc0e0, equites_dac1_w },
	{ 0xc0f8, 0xc0fe, MWA_NOP }, // soundboard I/O, ignored
	{ 0xc0ff, 0xc0ff, soundlatch_clear_w },
	{ 0xe000, 0xe0ff, MWA_RAM }, // stack and variables
MEMORY_END

static PORT_WRITE_START( equites_s_writeport )
	{ 0x00e0, 0x00e5, MWA_NOP }, // soundboard I/O, ignored
PORT_END
// Common Hardware End

// Equites Hardware
extern PALETTE_INIT( equites );
extern VIDEO_START( equites );
extern VIDEO_UPDATE( equites );
extern READ16_HANDLER(equites_spriteram_r);
extern WRITE16_HANDLER(equites_charram_w);
extern WRITE16_HANDLER(equites_scrollreg_w);
extern WRITE16_HANDLER(equites_bgcolor_w);

// Splendor Blast Hareware
extern MACHINE_INIT( splndrbt );
extern PALETTE_INIT( splndrbt );
extern VIDEO_START( splndrbt );
extern VIDEO_UPDATE( splndrbt );
extern READ16_HANDLER(splndrbt_bankedchar_r);
extern WRITE16_HANDLER(splndrbt_charram_w);
extern WRITE16_HANDLER(splndrbt_bankedchar_w);
extern WRITE16_HANDLER(splndrbt_selchar0_w);
extern WRITE16_HANDLER(splndrbt_selchar1_w);
extern WRITE16_HANDLER(splndrbt_scrollram_w);
extern WRITE16_HANDLER(splndrbt_bgcolor_w);
extern data16_t *splndrbt_scrollx, *splndrbt_scrolly;

/******************************************************************************/
// Locals

// Equites Hardware
static int disablejoyport2 = 0;

/******************************************************************************/
// Exports

// Common
data16_t *equites_workram;
int equites_id;

// Equites Hardware
int equites_flip;

// Splendor Blast Hardware
int splndrbt_flip;

/******************************************************************************/
// Local Functions

/******************************************************************************/
// Interrupt Handlers

// Equites Hardware
static INTERRUPT_GEN( equites_interrupt )
{
	if (cpu_getiloops())
		cpu_set_irq_line(0, 2, HOLD_LINE);
	else
		cpu_set_irq_line(0, 1, HOLD_LINE);
}

// Splendor Blast Hareware
static INTERRUPT_GEN( splndrbt_interrupt )
{
	if (cpu_getiloops())
		cpu_set_irq_line(0, 1, HOLD_LINE);
	else
		cpu_set_irq_line(0, 2, HOLD_LINE);
}

/******************************************************************************/
// Main CPU Handlers

// Equites Hardware
static READ16_HANDLER(equites_joyport_r)
{
	int data;

	data = readinputport(0);
	if (disablejoyport2) data = (data & 0x80ff) | (data<<8 & 0x7f00);

	return (data);
}

static WRITE16_HANDLER(equites_flip0_w)
{
	if (ACCESSING_LSB) disablejoyport2 = 1;
	if (ACCESSING_MSB) equites_flip = 0;
}

static WRITE16_HANDLER(equites_flip1_w)
{
	if (ACCESSING_LSB) disablejoyport2 = 0;
	if (ACCESSING_MSB) equites_flip = 1;
}

// Splendor Blast Hardware
#if HVOLTAGE_HACK
static READ16_HANDLER(hvoltage_debug_r)
{
	return(readinputport(2));
}
#endif

static WRITE16_HANDLER(splndrbt_flip0_w)
{
	if (ACCESSING_LSB) splndrbt_flip = 0;
	if (ACCESSING_MSB) equites_bgcolor_w(offset, data, 0x00ff);
}

static WRITE16_HANDLER(splndrbt_flip1_w)
{
	if (ACCESSING_LSB) splndrbt_flip = 1;
}
#if 0
static WRITE16_HANDLER(log16_w)
{
	int pc = activecpu_get_pc();

	usrintf_showmessage("%04x: %04x(w)\n", pc, data);
	logerror("%04x: %04x(w)\n", pc, data);
}
#endif
/******************************************************************************/
// Main CPU Memory Map

// Equites Hardware
static MEMORY_READ16_START( equites_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM }, // main program
	{ 0x040000, 0x040fff, MRA16_RAM }, // work RAM
	{ 0x080000, 0x080fff, MRA16_RAM }, // char RAM
	{ 0x0c0000, 0x0c0fff, MRA16_RAM }, // scroll RAM
	{ 0x100000, 0x100fff, equites_spriteram_r }, // sprite RAM
	{ 0x140000, 0x1407ff, equites_8404_r }, // 8404 RAM
	{ 0x180000, 0x180001, input_port_1_word_r }, // MSB: DIP switches
	{ 0x1c0000, 0x1c0001, equites_joyport_r }, // joyport[2211] (shares the same addr with scrollreg)
MEMORY_END

static MEMORY_WRITE16_START( equites_writemem )
	{ 0x000000, 0x00ffff, MWA16_NOP }, // ROM area is written several times (dev system?)
	{ 0x040000, 0x040fff, MWA16_RAM, &equites_workram },
	{ 0x080000, 0x080fff, equites_charram_w, &videoram16 },
	{ 0x0c0000, 0x0c0fff, MWA16_RAM, &spriteram16_2 },
	{ 0x100000, 0x100fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x140000, 0x1407ff, MWA16_RAM, &equites_8404ram },
	{ 0x180000, 0x180001, soundlatch_word_w }, // LSB: sound latch
	{ 0x184000, 0x184001, equites_flip0_w }, // [MMLL] MM: normal screen, LL: use joystick 1 only
	{ 0x188000, 0x188001, MWA16_NOP }, // 8404 control port1
	{ 0x18c000, 0x18c001, MWA16_NOP }, // 8404 control port2
	{ 0x1a4000, 0x1a4001, equites_flip1_w }, // [MMLL] MM: flip screen, LL: use both joysticks
	{ 0x1a8000, 0x1a8001, MWA16_NOP }, // 8404 control port3
	{ 0x1ac000, 0x1ac001, MWA16_NOP }, // 8404 control port4
	{ 0x1c0000, 0x1c0001, equites_scrollreg_w }, // scroll register[XXYY]
	{ 0x380000, 0x380001, equites_bgcolor_w }, // bg color register[CC--]
	{ 0x780000, 0x780001, MWA16_NOP }, // watchdog
MEMORY_END

// Splendor Blast Hardware
static MEMORY_READ16_START( splndrbt_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM }, // main program
	{ 0x040000, 0x040fff, MRA16_RAM }, // work RAM
	{ 0x080000, 0x080001, input_port_0_word_r }, // joyport [2211]
	{ 0x0c0000, 0x0c0001, input_port_1_word_r }, // MSB: DIP switches LSB: not used
	{ 0x100000, 0x100001, MRA16_RAM }, // no read
	{ 0x1c0000, 0x1c0001, MRA16_RAM }, // LSB: watchdog
	{ 0x180000, 0x1807ff, equites_8404_r }, // 8404 RAM
	{ 0x200000, 0x200fff, MRA16_RAM }, // char page 0
	{ 0x201000, 0x201fff, splndrbt_bankedchar_r }, // banked char page 1, 2
	{ 0x400000, 0x400fff, MRA16_RAM }, // scroll RAM 0,1
	{ 0x600000, 0x6001ff, MRA16_RAM }, // sprite RAM 0,1,2
MEMORY_END

static MEMORY_WRITE16_START( splndrbt_writemem )
	{ 0x000000, 0x00ffff, MWA16_NOP },
	{ 0x040000, 0x040fff, MWA16_RAM, &equites_workram }, // work RAM
	{ 0x0c0000, 0x0c0001, splndrbt_flip0_w }, // [MMLL] MM: bg color register, LL: normal screen
	{ 0x0c4000, 0x0c4001, MWA16_NOP }, // 8404 control port1
	{ 0x0c8000, 0x0c8001, MWA16_NOP }, // 8404 control port2
	{ 0x0cc000, 0x0cc001, splndrbt_selchar0_w }, // select active char map
	{ 0x0e0000, 0x0e0001, splndrbt_flip1_w }, // [MMLL] MM: not used, LL: flip screen
	{ 0x0e4000, 0x0e4001, MWA16_NOP }, // 8404 control port3
	{ 0x0e8000, 0x0e8001, MWA16_NOP }, // 8404 control port4
	{ 0x0ec000, 0x0ec001, splndrbt_selchar1_w }, // select active char map
	{ 0x100000, 0x100001, MWA16_RAM, &splndrbt_scrollx }, // scrollx
	{ 0x140000, 0x140001, soundlatch_word_w }, // LSB: sound command
	{ 0x1c0000, 0x1c0001, MWA16_RAM, &splndrbt_scrolly }, // scrolly
	{ 0x180000, 0x1807ff, MWA16_RAM, &equites_8404ram }, // 8404 RAM
	{ 0x200000, 0x200fff, splndrbt_charram_w, &videoram16, &videoram_size }, // char RAM page 0
	{ 0x201000, 0x201fff, splndrbt_bankedchar_w }, // banked char RAM page 1,2
	{ 0x400000, 0x400fff, splndrbt_scrollram_w, &spriteram16_2 }, // scroll RAM 0,1
	{ 0x600000, 0x6001ff, MWA16_RAM, &spriteram16, &spriteram_size }, // sprite RAM 0,1,2
MEMORY_END

/******************************************************************************/
// Common Port Map

#define EQUITES_PLAYER_INPUT_LSB( button1, button2, button3, start ) \
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, button1 ) \
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, button2 ) \
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, button3 ) \
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, start )

#define EQUITES_PLAYER_INPUT_MSB( button1, button2, button3, start ) \
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, button1 | IPF_COCKTAIL ) \
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, button2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, button3 | IPF_COCKTAIL ) \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, start )

/******************************************************************************/
// Equites Port Map

INPUT_PORTS_START( equites )
	PORT_START
	EQUITES_PLAYER_INPUT_LSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START1 )
	EQUITES_PLAYER_INPUT_MSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_COIN2 )
#if EASY_TEST_MODE
	PORT_SERVICE( 0x0300, IP_ACTIVE_HIGH )
#endif
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Buttons" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0800, "3" )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR ( Lives ) )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPNAME( 0xc000, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xc000, "A 2C/1C B 3C/1C" )
	PORT_DIPSETTING(      0x0000, "A 1C/1C B 2C/1C" )
	PORT_DIPSETTING(      0x8000, "A 1C/2C B 1C/4C" )
	PORT_DIPSETTING(      0x4000, "A 1C/3C B 1C/6C" )
INPUT_PORTS_END

/******************************************************************************/
// Bull Fighter Port Map

INPUT_PORTS_START( bullfgtr )
	PORT_START
	EQUITES_PLAYER_INPUT_LSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START1 )
	EQUITES_PLAYER_INPUT_MSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_COIN2 )
#if EASY_TEST_MODE
	PORT_SERVICE( 0x0300, IP_ACTIVE_HIGH )
#endif
	PORT_DIPNAME( 0x0c00, 0x0000, "Game Time" )
	PORT_DIPSETTING(      0x0c00, "3:00" )
	PORT_DIPSETTING(      0x0800, "2:00" )
	PORT_DIPSETTING(      0x0000, "1:30" )
	PORT_DIPSETTING(      0x0400, "1:00" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x9000, 0x0000, DEF_STR( Coinage ) )
//	PORT_DIPSETTING(      0x9000, "A 1C/1C B 1C/1C" )		// More than 1 credit per player needed
	PORT_DIPSETTING(      0x0000, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(      0x8000, "A 1C/1C B 1C/4C" )
	PORT_DIPSETTING(      0x1000, "A 1C/2C B 1C/3C" )
INPUT_PORTS_END

/******************************************************************************/
// Koukouyakyuh Port Map

INPUT_PORTS_START( kouyakyu )
	PORT_START
	EQUITES_PLAYER_INPUT_LSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START1 )
	EQUITES_PLAYER_INPUT_MSB( IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_START2 )

	PORT_START
//	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_COIN1 )
//	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0300, IP_ACTIVE_HIGH, IPT_COIN1 )
#if EASY_TEST_MODE
	PORT_SERVICE( 0x0300, IP_ACTIVE_HIGH )
#endif
	PORT_DIPNAME( 0x0c00, 0x0000, "Game Points" )
	PORT_DIPSETTING(      0x0800, "3000" )
	PORT_DIPSETTING(      0x0400, "4000" )
	PORT_DIPSETTING(      0x0000, "5000" )
	PORT_DIPSETTING(      0x0c00, "7000" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x9000, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x9000, "1C/1C (2C per player)" )
	PORT_DIPSETTING(      0x0000, "1C/1C (1C per player)" )
	PORT_DIPSETTING(      0x8000, "1C/1C (1C for 2 players)" )
	PORT_DIPSETTING(      0x1000, "1C/3C (1C per player)" )
INPUT_PORTS_END

/******************************************************************************/
// Splendor Blast Port Map

INPUT_PORTS_START( splndrbt )
	PORT_START
	EQUITES_PLAYER_INPUT_LSB( IPT_BUTTON1, IPT_BUTTON2, IPT_UNKNOWN, IPT_START1 )
	EQUITES_PLAYER_INPUT_MSB( IPT_BUTTON1, IPT_BUTTON2, IPT_UNKNOWN, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_COIN2 )
#if EASY_TEST_MODE
	PORT_SERVICE( 0x0300, IP_ACTIVE_HIGH )
#endif
	PORT_DIPNAME( 0x0c00, 0x0000, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(      0x0400, "Easy" )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0c00, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc000, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xc000, "A 2C/1C B 3C/1C" )
	PORT_DIPSETTING(      0x0000, "A 1C/1C B 2C/1C" )
	PORT_DIPSETTING(      0x4000, "A 1C/2C B 1C/4C" )
	PORT_DIPSETTING(      0x8000, "A 1C/3C B 1C/6C" )
INPUT_PORTS_END

/******************************************************************************/
// High Voltage Port Map

INPUT_PORTS_START( hvoltage )
	PORT_START
	EQUITES_PLAYER_INPUT_LSB( IPT_BUTTON1, IPT_BUTTON2, IPT_UNKNOWN, IPT_START1 )
	EQUITES_PLAYER_INPUT_MSB( IPT_BUTTON1, IPT_BUTTON2, IPT_UNKNOWN, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_COIN2 )
#if EASY_TEST_MODE
	PORT_SERVICE( 0x0300, IP_ACTIVE_HIGH )
#endif
#if HVOLTAGE_HACK
	PORT_DIPNAME( 0x0400, 0x0000, "Invulnerability" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0000, "Need to kill Bosses" )
	PORT_DIPSETTING(      0x0800, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
#else
	PORT_DIPNAME( 0x0c00, 0x0000, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(      0x0400, "Easy" )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0c00, "Hardest" )
#endif
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR ( Lives ) )			// See notes
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR ( Bonus_Life ) )
	PORT_DIPSETTING(      0x0000, "50k, 100k then every 100k" )
	PORT_DIPSETTING(      0x2000, "50k, 200k then every 100k" )
	PORT_DIPNAME( 0xc000, 0x0000, DEF_STR( Coinage ) )			// See notes
	PORT_DIPSETTING(      0xc000, "A 2C/1C B 3C/1C" )
	PORT_DIPSETTING(      0x0000, "A 1C/1C B 2C/1C" )
	PORT_DIPSETTING(      0x4000, "A 1C/2C B 1C/4C" )
	PORT_DIPSETTING(      0x8000, "A 1C/3C B 1C/6C" )

#if HVOLTAGE_HACK
	/* Fake port to handle debug mode */
	PORT_START
	PORT_DIPNAME( 0xffff, 0xffff, "Debug Mode" )
	PORT_DIPSETTING(      0xffff, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#endif
INPUT_PORTS_END

/******************************************************************************/
// Graphics Layouts

// Equites Hardware
static struct GfxLayout eq_charlayout =
{
	8, 8,
	256,
	2,
	{ 0, 4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout eq_tilelayout =
{
	16, 16,
	256,
	3,
	{ 0, 0x4000*8, 0x4000*8+4 },
	{
	  128*1+3, 128*1+2, 128*1+1, 128*1+0,
	  128*2+3, 128*2+2, 128*2+1, 128*2+0,
	  128*3+3, 128*3+2, 128*3+1, 128*3+0,
	        3,       2,       1,       0
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};

static struct GfxLayout eq_spritelayout =
{
	16, 14,
	256,
	3,
	{ 0, 0x4000*8, 0x4000*8+4 },
	{
	        3,       2,       1,       0,
	  128*1+3, 128*1+2, 128*1+1, 128*1+0,
	  128*2+3, 128*2+2, 128*2+1, 128*2+0,
	  128*3+3, 128*3+2, 128*3+1, 128*3+0
	},
	{ 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8 },
	64*8
};

static struct GfxDecodeInfo equites_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &eq_charlayout,     0, 32 }, // chars
	{ REGION_GFX2, 0, &eq_tilelayout,   128, 16 }, // tile set0
	{ REGION_GFX3, 0, &eq_tilelayout,   128, 16 }, // tile set1
	{ REGION_GFX4, 0, &eq_spritelayout, 256, 16 }, // sprite set0
	{ REGION_GFX5, 0, &eq_spritelayout, 256, 16 }, // sprite set1
	{ -1 } // end of array
};

// Splendor Blast Hardware
static struct GfxLayout sp_charlayout =
{
	8, 8,
	512,
	2,
	{ 0, 4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout sp_tilelayout =
{
	16,16,
	256,
	2,
	{ 0, 4 },
	{
	  16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  48*8+3, 48*8+2, 48*8+1, 48*8+0,
	       3,      2,      1,      0
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};

static struct GfxLayout sp_spritelayout =
{
	32,32,
	128,
	3,
	{ 0, 0x8000*8, 0x8000*8+4 },
	{
	  0*8+3, 0*8+2, 0*8+1, 0*8+0, 1*8+3, 1*8+2, 1*8+1, 1*8+0,
	  2*8+3, 2*8+2, 2*8+1, 2*8+0, 3*8+3, 3*8+2, 3*8+1, 3*8+0,
	  4*8+3, 4*8+2, 4*8+1, 4*8+0, 5*8+3, 5*8+2, 5*8+1, 5*8+0,
	  6*8+3, 6*8+2, 6*8+1, 6*8+0, 7*8+3, 7*8+2, 7*8+1, 7*8+0
	},
	{
	  0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8,
	  8*8*8, 9*8*8,10*8*8,11*8*8,12*8*8,13*8*8,14*8*8,15*8*8,
	 31*8*8,30*8*8,29*8*8,28*8*8,27*8*8,26*8*8,25*8*8,24*8*8,
	 23*8*8,22*8*8,21*8*8,20*8*8,19*8*8,18*8*8,17*8*8,16*8*8
	},
	8*32*8
};

static struct GfxDecodeInfo splndrbt_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sp_charlayout,     0, 256/4 }, // 512 4-color chars
	{ REGION_GFX2, 0, &sp_tilelayout,   256, 256/4 }, // 256 4-color tiles
	{ REGION_GFX3, 0, &sp_tilelayout,   256, 256/4 }, // 256 4-color tiles
	{ REGION_GFX4, 0, &sp_spritelayout, 512, 256/8 }, // 256 8-color sprites
	{ -1 } // end of array
};

/******************************************************************************/
// Hardware Definitions

static MACHINE_DRIVER_START( equites )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000/2) // OSC: 12Mhz
	MDRV_CPU_MEMORY(equites_readmem, equites_writemem)
	MDRV_CPU_VBLANK_INT(equites_interrupt, 2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION | VIDEO_UPDATE_BEFORE_VBLANK)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256 +BMPAD*2, 256 +BMPAD*2)
	MDRV_VISIBLE_AREA(0 +BMPAD, 256-1 +BMPAD, 24 +BMPAD, 232-1 +BMPAD)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(384)
	MDRV_GFXDECODE(equites_gfxdecodeinfo)

	MDRV_PALETTE_INIT(equites)
	MDRV_VIDEO_START(equites)
	MDRV_VIDEO_UPDATE(equites)

	/* sound hardware */
	EQUITES_ADD_SOUNDBOARD7

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( splndrbt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000/2) // OSC: 12Mhz
	MDRV_CPU_MEMORY(splndrbt_readmem, splndrbt_writemem)
	MDRV_CPU_VBLANK_INT(splndrbt_interrupt, 2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_MACHINE_INIT(splndrbt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 64, 256-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1536)
	MDRV_GFXDECODE(splndrbt_gfxdecodeinfo)

	MDRV_PALETTE_INIT(splndrbt)
	MDRV_VIDEO_START(splndrbt)
	MDRV_VIDEO_UPDATE(splndrbt)

	/* sound hardware */
	EQUITES_ADD_SOUNDBOARD7

MACHINE_DRIVER_END

/******************************************************************************/
// Equites ROM Map

ROM_START( equites )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // 68000 ROMs
	ROM_LOAD16_BYTE( "ep1", 0x00001, 0x2000, 0x6a4fe5f7 )
	ROM_LOAD16_BYTE( "ep5", 0x00000, 0x2000, 0x00faa3eb )
	ROM_LOAD16_BYTE( "epr-ep2.12d", 0x04001, 0x2000, 0x0C1BC2E7 )
	ROM_LOAD16_BYTE( "epr-ep6.12b", 0x04000, 0x2000, 0xBBED3DCC )
	ROM_LOAD16_BYTE( "epr-ep3.10d", 0x08001, 0x2000, 0x5F2D059A )
	ROM_LOAD16_BYTE( "epr-ep7.10b", 0x08000, 0x2000, 0xA8F6B8AA )
	ROM_LOAD16_BYTE( "ep4",  0x0c001, 0x2000, 0xb636e086 )
	ROM_LOAD16_BYTE( "ep8",  0x0c000, 0x2000, 0xd7ee48b0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs
	ROM_LOAD( "ev1.1m", 0x00000, 0x2000, 0x43FAAF2E )
	ROM_LOAD( "ev2.1l", 0x02000, 0x2000, 0x09E6702D )
	ROM_LOAD( "ev3.1k", 0x04000, 0x2000, 0x10FF140B )
	ROM_LOAD( "ev4.1h", 0x06000, 0x2000, 0xB7917264 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) // chars
	ROM_LOAD( "ep9",  0x00000, 0x1000, 0x0325be11 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) // tile set1
	ROM_LOAD_NIB_HIGH( "eb5.7h",  0x00000, 0x2000, 0xCBEF7DA5 )
	ROM_LOAD         ( "eb5.7h",  0x02000, 0x2000, 0xCBEF7DA5 )
	ROM_LOAD         ( "eb1.7f",  0x04000, 0x2000, 0x9A236583 )
	ROM_LOAD         ( "eb2.8f",  0x06000, 0x2000, 0xF0FB6355 )

	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE ) // tile set2
	ROM_LOAD_NIB_HIGH( "eb6.8h",  0x00000, 0x2000, 0x1E5E5475 )
	ROM_LOAD         ( "eb6.8h",  0x02000, 0x2000, 0x1E5E5475 )
	ROM_LOAD         ( "eb3.10f", 0x04000, 0x2000, 0xDBD0044B )
	ROM_LOAD         ( "eb4.11f", 0x06000, 0x2000, 0xF8F8E600 )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE ) // sprite set1
	ROM_LOAD_NIB_HIGH( "es5.5h",     0x00000, 0x2000, 0xD5B82E6A )
	ROM_LOAD         ( "es5.5h",     0x02000, 0x2000, 0xD5B82E6A )
	ROM_LOAD         ( "es1.5f",     0x04000, 0x2000, 0xCF81A2CD )
	ROM_LOAD         ( "es2.4f",     0x06000, 0x2000, 0xAE3111D8 )

	ROM_REGION( 0x8000, REGION_GFX5, ROMREGION_DISPOSE ) // sprite set2
	ROM_LOAD_NIB_HIGH( "es6.4h",     0x00000, 0x2000, 0xCB4F5DA9 )
	ROM_LOAD         ( "es6.4h",     0x02000, 0x2000, 0xCB4F5DA9 )
	ROM_LOAD         ( "es3.2f",     0x04000, 0x2000, 0x3D44F815 )
	ROM_LOAD         ( "es4.1f",     0x06000, 0x2000, 0x16E6D18A )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMs
	ROM_LOAD( "bprom.3a",  0x00000, 0x100, 0x2FCDF217 ) // R
	ROM_LOAD( "bprom.1a",  0x00100, 0x100, 0xD7E6CD1F ) // G
	ROM_LOAD( "bprom.2a",  0x00200, 0x100, 0xE3D106E8 ) // B

	ROM_REGION( 0x0100, REGION_USER1, 0 ) // CLUT(same PROM x 4)
	ROM_LOAD( "bprom.6b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.7b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.9b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.10b", 0x0000, 0x100, 0x6294CDDF )

	ROM_REGION( 0x0020, REGION_USER2, 0 ) // MSM5232 PROM?
	ROM_LOAD( "bprom.3h",  0x00000, 0x020, 0x33B98466 )
ROM_END

ROM_START( equitess )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // 68000 ROMs
	ROM_LOAD16_BYTE( "epr-ep1.13d", 0x00001, 0x2000, 0xC6EDF1CD )
	ROM_LOAD16_BYTE( "epr-ep5.13b", 0x00000, 0x2000, 0xC11F0759 )
	ROM_LOAD16_BYTE( "epr-ep2.12d", 0x04001, 0x2000, 0x0C1BC2E7 )
	ROM_LOAD16_BYTE( "epr-ep6.12b", 0x04000, 0x2000, 0xBBED3DCC )
	ROM_LOAD16_BYTE( "epr-ep3.10d", 0x08001, 0x2000, 0x5F2D059A )
	ROM_LOAD16_BYTE( "epr-ep7.10b", 0x08000, 0x2000, 0xA8F6B8AA )
	ROM_LOAD16_BYTE( "epr-ep4.9d",  0x0c001, 0x2000, 0x956A06BD )
	ROM_LOAD16_BYTE( "epr-ep8.9b",  0x0c000, 0x2000, 0x4C78D60D )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs
	ROM_LOAD( "ev1.1m", 0x00000, 0x2000, 0x43FAAF2E )
	ROM_LOAD( "ev2.1l", 0x02000, 0x2000, 0x09E6702D )
	ROM_LOAD( "ev3.1k", 0x04000, 0x2000, 0x10FF140B )
	ROM_LOAD( "ev4.1h", 0x06000, 0x2000, 0xB7917264 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) // chars
	ROM_LOAD( "epr-ep0.3e",  0x00000, 0x1000, 0x3F5A81C3 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) // tile set1
	ROM_LOAD_NIB_HIGH( "eb5.7h",  0x00000, 0x2000, 0xCBEF7DA5 )
	ROM_LOAD         ( "eb5.7h",  0x02000, 0x2000, 0xCBEF7DA5 )
	ROM_LOAD         ( "eb1.7f",  0x04000, 0x2000, 0x9A236583 )
	ROM_LOAD         ( "eb2.8f",  0x06000, 0x2000, 0xF0FB6355 )

	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE ) // tile set2
	ROM_LOAD_NIB_HIGH( "eb6.8h",  0x00000, 0x2000, 0x1E5E5475 )
	ROM_LOAD         ( "eb6.8h",  0x02000, 0x2000, 0x1E5E5475 )
	ROM_LOAD         ( "eb3.10f", 0x04000, 0x2000, 0xDBD0044B )
	ROM_LOAD         ( "eb4.11f", 0x06000, 0x2000, 0xF8F8E600 )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE ) // sprite set1
	ROM_LOAD_NIB_HIGH( "es5.5h",     0x00000, 0x2000, 0xD5B82E6A )
	ROM_LOAD         ( "es5.5h",     0x02000, 0x2000, 0xD5B82E6A )
	ROM_LOAD         ( "es1.5f",     0x04000, 0x2000, 0xCF81A2CD )
	ROM_LOAD         ( "es2.4f",     0x06000, 0x2000, 0xAE3111D8 )

	ROM_REGION( 0x8000, REGION_GFX5, ROMREGION_DISPOSE ) // sprite set2
	ROM_LOAD_NIB_HIGH( "es6.4h",     0x00000, 0x2000, 0xCB4F5DA9 )
	ROM_LOAD         ( "es6.4h",     0x02000, 0x2000, 0xCB4F5DA9 )
	ROM_LOAD         ( "es3.2f",     0x04000, 0x2000, 0x3D44F815 )
	ROM_LOAD         ( "es4.1f",     0x06000, 0x2000, 0x16E6D18A )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMs
	ROM_LOAD( "bprom.3a",  0x00000, 0x100, 0x2FCDF217 ) // R
	ROM_LOAD( "bprom.1a",  0x00100, 0x100, 0xD7E6CD1F ) // G
	ROM_LOAD( "bprom.2a",  0x00200, 0x100, 0xE3D106E8 ) // B

	ROM_REGION( 0x0100, REGION_USER1, 0 ) // CLUT(same PROM x 4)
	ROM_LOAD( "bprom.6b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.7b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.9b",  0x0000, 0x100, 0x6294CDDF )
	ROM_LOAD( "bprom.10b", 0x0000, 0x100, 0x6294CDDF )

	ROM_REGION( 0x0020, REGION_USER2, 0 ) // MSM5232 PROM?
	ROM_LOAD( "bprom.3h",  0x00000, 0x020, 0x33B98466 )
ROM_END

/******************************************************************************/
// Koukouyakyuh ROM Map

ROM_START( kouyakyu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  // 68000 ROMs
	ROM_LOAD16_BYTE( "epr-6704.bin", 0x00001, 0x2000, 0xc7ac2292 )
	ROM_LOAD16_BYTE( "epr-6707.bin", 0x00000, 0x2000, 0x9cb2962e )
	ROM_LOAD16_BYTE( "epr-6705.bin", 0x04001, 0x2000, 0x985327cb )
	ROM_LOAD16_BYTE( "epr-6708.bin", 0x04000, 0x2000, 0xf8863dc5 )
	ROM_LOAD16_BYTE( "epr-6706.bin", 0x08001, 0x2000, BADCRC( 0x79e94cd2 ) )	// was bad, manually patched
	ROM_LOAD16_BYTE( "epr-6709.bin", 0x08000, 0x2000, 0xf41cb58c )
	ROM_FILL(                        0x0c000, 0x4000, 0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs
	ROM_LOAD( "epr-6703.bin", 0x00000, 0x2000, 0xfbff3a86 )
	ROM_LOAD( "epr-6702.bin", 0x02000, 0x2000, 0x27ddf031 )
	ROM_LOAD( "epr-6701.bin", 0x04000, 0x2000, 0x3c83588a )
	ROM_LOAD( "epr-6700.bin", 0x06000, 0x2000, 0xee579266 )
	ROM_LOAD( "epr-6699.bin", 0x08000, 0x2000, 0x9bfa4a72 )
	ROM_LOAD( "epr-6698.bin", 0x0a000, 0x2000, 0x7adfd1ff )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) // chars
	ROM_LOAD( "epr-6710.bin", 0x00000, 0x1000, 0xaccda190 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) // tile set1
	ROM_LOAD_NIB_HIGH( "epr-6695.bin", 0x00000, 0x2000, 0x22bea465 )
	ROM_LOAD         ( "epr-6695.bin", 0x02000, 0x2000, 0x22bea465 )
	ROM_LOAD         ( "epr-6689.bin", 0x04000, 0x2000, 0x53bf7587 )
	ROM_LOAD         ( "epr-6688.bin", 0x06000, 0x2000, 0xceb76c5b )

	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE ) // tile set2
	ROM_LOAD_NIB_HIGH( "epr-6694.bin", 0x00000, 0x2000, 0x51a7345e )
	ROM_LOAD         ( "epr-6694.bin", 0x02000, 0x2000, 0x51a7345e )
	ROM_LOAD         ( "epr-6687.bin", 0x04000, 0x2000, 0x9c1f49df )
	ROM_LOAD         ( "epr-6686.bin", 0x06000, 0x2000, 0x3d9e516f )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE ) // sprite set1
	ROM_LOAD_NIB_HIGH( "epr-6696.bin", 0x00000, 0x2000, 0x0625f48e )
	ROM_LOAD         ( "epr-6696.bin", 0x02000, 0x2000, 0x0625f48e )
	ROM_LOAD         ( "epr-6690.bin", 0x04000, 0x2000, 0xa142a11d )
	ROM_LOAD         ( "epr-6691.bin", 0x06000, 0x2000, 0xb640568c )

	ROM_REGION( 0x8000, REGION_GFX5, ROMREGION_DISPOSE ) // sprite set2
	ROM_LOAD_NIB_HIGH( "epr-6697.bin", 0x00000, 0x2000, 0xf18afabe )
	ROM_LOAD         ( "epr-6697.bin", 0x02000, 0x2000, 0xf18afabe )
	ROM_LOAD         ( "epr-6692.bin", 0x04000, 0x2000, 0xb91d8172 )
	ROM_LOAD         ( "epr-6693.bin", 0x06000, 0x2000, 0x874e3acc )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMS
	ROM_LOAD( "pr6627.bpr",  0x00000, 0x100, 0x5ec5480d ) // R
	ROM_LOAD( "pr6629.bpr",  0x00100, 0x100, 0x29c7a393 ) // G
	ROM_LOAD( "pr6628.bpr",  0x00200, 0x100, 0x8af247a4 ) // B

	ROM_REGION( 0x0100, REGION_USER1, 0 ) // CLUT(same PROM x 4)
	ROM_LOAD( "pr6630a.bpr", 0x0000, 0x100, 0xd6e202da )
	ROM_LOAD( "pr6630b.bpr", 0x0000, 0x100, 0xd6e202da )
	ROM_LOAD( "pr6630c.bpr", 0x0000, 0x100, 0xd6e202da )
	ROM_LOAD( "pr6630d.bpr", 0x0000, 0x100, 0xd6e202da )

	ROM_REGION( 0x0020, REGION_USER2, 0 ) // MSM5232 PROM?(identical to bprom.3h in Equites)
	ROM_LOAD( "pr.bpr",      0x00000, 0x020, 0x33b98466 )
ROM_END

/******************************************************************************/
// Bull Fighter ROM Map

ROM_START( bullfgtr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // 68000 ROMs
	ROM_LOAD16_BYTE( "m_d13.bin",  0x00001, 0x2000, 0x7c35dd4b )
	ROM_LOAD16_BYTE( "m_b13.bin",  0x00000, 0x2000, 0xc4adddce )
	ROM_LOAD16_BYTE( "m_d12.bin",  0x04001, 0x2000, 0x5d51be2b )
	ROM_LOAD16_BYTE( "m_b12.bin",  0x04000, 0x2000, 0xd98390ef )
	ROM_LOAD16_BYTE( "m_dd10.bin", 0x08001, 0x2000, 0x21875752 )
	ROM_LOAD16_BYTE( "m_b10.bin",  0x08000, 0x2000, 0x9d84f678 )
	ROM_FILL(                      0x0c000, 0x4000, 0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs
	ROM_LOAD( "s_m1.bin", 0x00000, 0x2000, 0x2a8e6fcf )
	ROM_LOAD( "s_l2.bin", 0x02000, 0x2000, 0x026e1533 )
	ROM_LOAD( "s_k1.bin", 0x04000, 0x2000, 0x51ee751c )
	ROM_LOAD( "s_h1.bin", 0x06000, 0x2000, 0x62c7a25b )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) // chars
	ROM_LOAD( "m_e4.bin", 0x000000, 0x1000, 0xc6894c9a )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) // tile set1
	ROM_LOAD_NIB_HIGH( "l_h7.bin",  0x00000, 0x2000, 0x6d05e9f2 )
	ROM_LOAD         ( "l_h7.bin",  0x02000, 0x2000, 0x6d05e9f2 )
	ROM_LOAD         ( "l_f7.bin",  0x04000, 0x2000, 0x4352d069 )
	ROM_LOAD         ( "l_f9.bin",  0x06000, 0x2000, 0x24edfd7d )

	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE ) // tile set2
	ROM_LOAD_NIB_HIGH( "l_h9.bin",  0x00000, 0x2000, 0x016340ae )
	ROM_LOAD         ( "l_h9.bin",  0x02000, 0x2000, 0x016340ae )
	ROM_LOAD         ( "l_f10.bin", 0x04000, 0x2000, 0x4947114e )
	ROM_LOAD         ( "l_f12.bin", 0x06000, 0x2000, 0xfa296cb3 )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE ) // sprite set1
	ROM_LOAD_NIB_HIGH( "l_h6.bin",  0x00000, 0x2000, 0x48394389 )
	ROM_LOAD         ( "l_h6.bin",  0x02000, 0x2000, 0x48394389 )
	ROM_LOAD         ( "l_f6.bin",  0x04000, 0x2000, 0x7c69b473 )
	ROM_LOAD         ( "l_f4.bin",  0x06000, 0x2000, 0xc3dc713f )

	ROM_REGION( 0x8000, REGION_GFX5, ROMREGION_DISPOSE ) // sprite set2
	ROM_LOAD_NIB_HIGH( "l_h4.bin",  0x00000, 0x2000, 0x141409ec )
	ROM_LOAD         ( "l_h4.bin",  0x02000, 0x2000, 0x141409ec )
	ROM_LOAD         ( "l_f3.bin",  0x04000, 0x2000, 0x883f93fd )
	ROM_LOAD         ( "l_f1.bin",  0x06000, 0x2000, 0x578ace7b )

	// all color PROMs of current dump are bad and have wrong sizes
	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMs
	ROM_LOAD( "m_a3.bin", 0x00000, 0x100, BADCRC(0x8203ee60) ) // R (made up, dump was bad)
	ROM_LOAD( "m_a1.bin", 0x00100, 0x100, BADCRC(0x2eb1a3de) ) // G (made up, dump was bad)
	ROM_LOAD( "m_a2.bin", 0x00200, 0x100, BADCRC(0x2e769d4c) ) // B (made up, dump was bad)

	ROM_REGION( 0x0100, REGION_USER1, 0 ) // CLUT(same PROM x 4)
	ROM_LOAD( "l_b6.bin",  0x0000, 0x100, 0x8835a069 )
	ROM_LOAD( "l_b7.bin",  0x0000, 0x100, 0x8835a069 )
	ROM_LOAD( "l_b9.bin",  0x0000, 0x100, 0x8835a069 )
	ROM_LOAD( "l_b10.bin", 0x0000, 0x100, 0x8835a069 )

	ROM_REGION( 0x0020, REGION_USER2, 0 ) // MSM5232 PROMs?(identical to bprom.3h in Equites)
	ROM_LOAD( "s_h3.bin",  0x00000, 0x020, 0x33b98466 )
ROM_END

/******************************************************************************/
// Splendor Blast ROM Map

ROM_START( splndrbt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // 68000 ROMs(16k x 4)
	ROM_LOAD16_BYTE( "1.16a", 0x00001, 0x4000, 0x4bf4b047 )
	ROM_LOAD16_BYTE( "2.16c", 0x00000, 0x4000, 0x27acb656 )
	ROM_LOAD16_BYTE( "3.15a", 0x08001, 0x4000, 0x5b182189 )
	ROM_LOAD16_BYTE( "4.15c", 0x08000, 0x4000, 0xcde99613 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs(8k x 4)
	ROM_LOAD( "1_v.1m", 0x00000, 0x2000, 0x1b3a6e42 )
	ROM_LOAD( "2_v.1l", 0x02000, 0x2000, 0x2a618c72 )
	ROM_LOAD( "3_v.1k", 0x04000, 0x2000, 0xbbee5346 )
	ROM_LOAD( "4_v.1h", 0x06000, 0x2000, 0x10f45af4 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) // 512 chars(8k x 1)
	ROM_LOAD( "10.8c",  0x00000, 0x2000, 0x501887d4 )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE ) // 256 tiles(16k x 1)
	ROM_LOAD( "8.14m",  0x00000, 0x4000, 0xc2c86621 )

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE ) // 256 tiles(16k x 1)
	ROM_LOAD( "9.12m",  0x00000, 0x4000, 0x4f7da6ff )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE ) // 256 sprites
	ROM_LOAD_NIB_HIGH( "6.18n", 0x00000, 0x2000, 0xaa72237f )
	ROM_CONTINUE     (          0x04000, 0x2000 )
	ROM_LOAD         ( "6.18n", 0x02000, 0x2000, 0xaa72237f )
	ROM_CONTINUE     (          0x06000, 0x2000 )
	ROM_LOAD         ( "5.18m", 0x08000, 0x4000, 0x5f618b39 )
	ROM_LOAD         ( "7.17m", 0x0c000, 0x4000, 0xabdd8483 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMs
	ROM_LOAD( "r.3a",   0x00000, 0x100, 0xca1f08ce ) // R
	ROM_LOAD( "g.1a",   0x00100, 0x100, 0x66f89177 ) // G
	ROM_LOAD( "b.2a",   0x00200, 0x100, 0xd14318bc ) // B

	ROM_REGION( 0x0500, REGION_USER1, 0 ) // CLUT(256bytes x 5)
	ROM_LOAD( "2.8k",   0x00000, 0x100, 0xe1770ad3 )
	ROM_LOAD( "s5.15p", 0x00100, 0x100, 0x7f6cf709 )
	ROM_LOAD( "s3.8l",  0x00200, 0x100, 0x1314b0b5 ) // unknown
	ROM_LOAD( "1.9j",   0x00300, 0x100, 0xf5b9b777 ) // unknown
	ROM_LOAD( "4.7m",   0x00400, 0x100, 0x12cbcd2c ) // unknown

	ROM_REGION( 0x2000, REGION_USER2, 0 ) // unknown(8k x 1)
	ROM_LOAD( "0.8h",   0x00000, 0x2000, 0x12681fb5 )

	ROM_REGION( 0x0020, REGION_USER3, 0 ) // MSM5232 PROMs?(identical to bprom.3h in Equites)
	ROM_LOAD( "3h.bpr", 0x00000, 0x020, 0x33b98466 )
ROM_END

/******************************************************************************/
// High Voltage ROM Map

ROM_START( hvoltage )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // 68000 ROMs(16k x 4)
	ROM_LOAD16_BYTE( "1.16a", 0x00001, 0x4000, 0x82606e3b )
	ROM_LOAD16_BYTE( "2.16c", 0x00000, 0x4000, 0x1d74fef2 )
	ROM_LOAD16_BYTE( "3.15a", 0x08001, 0x4000, 0x677abe14 )
	ROM_LOAD16_BYTE( "4.15c", 0x08000, 0x4000, 0x8aab5a20 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) // Z80 ROMs(8k x 3)
	ROM_LOAD( "5_v.1l", 0x00000, 0x4000, 0xed9bb6ea )
	ROM_LOAD( "6_v.1h", 0x04000, 0x4000, 0xe9542211 )
	ROM_LOAD( "7_v.1e", 0x08000, 0x4000, 0x44d38554 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) // 512 chars(8k x 1)
	ROM_LOAD( "5.8c",   0x00000, 0x2000, 0x656d53cd )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE ) // 256 tiles(16k x 1)
	ROM_LOAD( "9.14m",  0x00000, 0x4000, 0x506a0989 )

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE ) // 256 tiles(16k x 1)
	ROM_LOAD( "10.12m", 0x00000, 0x4000, 0x98f87d4f )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE ) // 256 sprites
	ROM_LOAD_NIB_HIGH( "8.18n", 0x00000, 0x2000, 0x725acae5 )
	ROM_CONTINUE     (          0x04000, 0x2000 )
	ROM_LOAD         ( "8.18n", 0x02000, 0x2000, 0x725acae5 )
	ROM_CONTINUE     (          0x06000, 0x2000 )
	ROM_LOAD         ( "6.18m", 0x08000, 0x4000, 0x9baf2c68 )
	ROM_LOAD         ( "7.17m", 0x0c000, 0x4000, 0x12d25fb1 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) // RGB PROMs
	ROM_LOAD( "r.3a",   0x00000, 0x100, 0x98eccbf6 ) // R
	ROM_LOAD( "g.1a",   0x00100, 0x100, 0xfab2ed23 ) // G
	ROM_LOAD( "b.2a",   0x00200, 0x100, 0x7274961b ) // B

	ROM_REGION( 0x0500, REGION_USER1, 0 ) // CLUT(256bytes x 5)
	ROM_LOAD( "2.8k",   0x00000, 0x100, 0x685f4e44 )
	ROM_LOAD( "s5.15p", 0x00100, 0x100, 0xb09bcc73 )
	ROM_LOAD( "3.8l",   0x00200, 0x100, 0x1314b0b5 ) // unknown(identical to s3.8l in Splendor Blast)
	ROM_LOAD( "1.9j",   0x00300, 0x100, 0xf5b9b777 ) // unknown(identical to 1.9j in Splendor Blast)
	ROM_LOAD( "4.7m",   0x00400, 0x100, 0x12cbcd2c ) // unknown(identical to 4.7m in Splendor Blast)

	ROM_REGION( 0x2000, REGION_USER2, 0 ) // unknown(8k x 1, identical to 0.8h in Splendor Blast )
	ROM_LOAD( "0.8h",   0x00000, 0x2000, 0x12681fb5 )

	ROM_REGION( 0x0020, REGION_USER3, 0 ) // MSM5232 PROMs?(identical to bprom.3h in Equites)
	ROM_LOAD( "3h.bpr", 0x00000, 0x020, 0x33b98466 )
ROM_END

/******************************************************************************/
// Initializations

static void equites_init_common(void)
{
	equites_flip = 0;

	equites_8404init();
}

static void splndrbt_init_common(void)
{
	equites_8404init();
}

static DRIVER_INIT( equites )
{
	equites_id = 0x8400;

	equites_init_common();

	equites_8404rule(0x094a, 0x007, 0x04); // 8404 test
	equites_8404rule(0x094e, 0x049, 0x00); // 8404 test

	equites_8404rule(0x0c76, 0x00b, 0x04); // death
	equites_8404rule(0x0c7e, 0x0cf, 0x0c); // 2nd jmp hi
	equites_8404rule(0x0c84, 0x0d1, 0xae); // 2nd jmp lo
	equites_8404rule(0x0c8c, 0x0d3, 0x0c); // 1st jmp hi
	equites_8404rule(0x0c92, 0x0d5, 0x9c); // 1st jmp lo

	equites_8404rule(0x92a6, 0x00f, 0x04); // respawn
	equites_8404rule(0x92b0, 0x27d,-0x40); // 2nd jmp hi
	equites_8404rule(0x92b6, 0x27f,-0x04); // 2nd jmp lo
	equites_8404rule(0x92b0, 0x281,-0x50); // 1st jmp hi
	equites_8404rule(0x92b6, 0x283,-0x05); // 1st jmp lo

	equites_8404rule(0x8f06, 0x013, 0x04); // ENT
	equites_8404rule(0x8f0c, 0x481,-0x10); // ENT jmpaddr hi
	equites_8404rule(0x8f12, 0x483,-0x01); // ENT jmpaddr lo

	equites_8404rule(0x915e, 0x017, 0x04); // EXT
	equites_8404rule(0x9164, 0x47d,-0x20); // scroll y
	equites_8404rule(0x916a, 0x47f,-0x02); // player y
	equites_8404rule(0x9170, 0x481,-0x30); // exit location hi
	equites_8404rule(0x9176, 0x483,-0x03); // exit location lo
}

static DRIVER_INIT( bullfgtr )
{
	equites_id = 0x8401;

	equites_init_common();

	equites_8404rule(0x0e7a, 0x601, 0x00); // boot up
	equites_8404rule(0x3da4, 0x201, 0x0c); // goal in
}

static DRIVER_INIT( kouyakyu )
{
	equites_id = 0x8500;

	equites_init_common();

	equites_8404rule(0x5582, 0x603, 0x05); // home run
}

static DRIVER_INIT( splndrbt )
{
	equites_id = 0x8510;

	splndrbt_init_common();

	equites_8404rule(0x06f8, 0x007, 0x04); // 8404 test
	equites_8404rule(0x06fc, 0x049, 0x00); // 8404 test

	equites_8404rule(0x12dc, 0x01b, 0x04); // guard point
	equites_8404rule(0x12e4, 0x01f, 0x04); // guard point

	equites_8404rule(0x0dc2, 0x00b, 0x04); // guard point (start race)
	equites_8404rule(0x0dd4, 0x5e1, 0x00); // no. of addresses to look up - 1
	equites_8404rule(0x0dd8, 0x5e3, 0x0c); // race start/respawn addr hi
	equites_8404rule(0x0dde, 0x5e5, 0x32); // race start/respawn addr lo

	equites_8404rule(0x1268, 0x023, 0x04); // guard point

	equites_8404rule(0x1298, 0x25f,-0x0c); // stage number?

	// game params. (180261-18027f)->(40060-4006f)
	equites_8404rule(0x12a0, 0x261, 0x0a); // max speed hi
	equites_8404rule(0x12a0, 0x263, 0x00); // max speed lo
	equites_8404rule(0x12a0, 0x265, 0x00); // accel hi
	equites_8404rule(0x12a0, 0x267, 0x10); // accel lo
	equites_8404rule(0x12a0, 0x269, 0x0c); // max turbo speed hi
	equites_8404rule(0x12a0, 0x26b, 0x00); // max turbo speed lo
	equites_8404rule(0x12a0, 0x26d, 0x00); // turbo accel hi
	equites_8404rule(0x12a0, 0x26f, 0x20); // turbo accel lo
	equites_8404rule(0x12a0, 0x271,-0x09); // random enemy spwan list
	equites_8404rule(0x12a0, 0x273,-0x09); // .
	equites_8404rule(0x12a0, 0x275,-0x09); // .
	equites_8404rule(0x12a0, 0x277,-0x09); // .
	equites_8404rule(0x12a0, 0x279,-0x09); // .
	equites_8404rule(0x12a0, 0x27b,-0x09); // .
	equites_8404rule(0x12a0, 0x27d,-0x09); // .
	equites_8404rule(0x12a0, 0x27f,-0x09); // .

	equites_8404rule(0x500e, 0x281,-0x08); // power-up's (random?)
	equites_8404rule(0x500e, 0x283,-0x08); // power-up's (random?)

	equites_8404rule(0x132e, 0x285,-0xa0); // object spawn table addr hi
	equites_8404rule(0x1334, 0x287,-0x0a); // object spawn table addr lo

	equites_8404rule(0x739a, 0x289,-0xb0); // level section table addr hi
	equites_8404rule(0x73a0, 0x28b,-0x0b); // level section table addr lo

	equites_8404rule(0x0912, 0x017, 0x04); // guard point
	equites_8404rule(0x0b4c, 0x013, 0x04); // guard point

	equites_8404rule(0x0bfc, 0x00f, 0x04); // guard point (miss/no gas/end level)
	equites_8404rule(0x0c08, 0x5e1, 0x05); // no. of addresses to look up - 1
	equites_8404rule(0x0c0c, 0x5e3,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5e5,-0x07); // game over/respawn addr lo
	equites_8404rule(0x0c0c, 0x5e7,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5e9,-0x07); // game over/respawn addr lo
	equites_8404rule(0x0c0c, 0x5eb,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5ed,-0x07); // game over/respawn addr lo
	equites_8404rule(0x0c0c, 0x5ef,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5f1,-0x07); // game over/respawn addr lo
	equites_8404rule(0x0c0c, 0x5f3,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5f5,-0x07); // game over/respawn addr lo
	equites_8404rule(0x0c0c, 0x5f7,-0x70); // game over/respawn addr hi
	equites_8404rule(0x0c12, 0x5f9,-0x07); // game over/respawn addr lo
}

static DRIVER_INIT( hvoltage )
{
	int i;

#if HVOLTAGE_HACK
	install_mem_read16_handler(0, 0x000038, 0x000039, hvoltage_debug_r);
#endif

	equites_id = 0x8511;

	splndrbt_init_common();

	equites_8404rule(0x0b18, 0x007, 0x04); // 8404 test
	equites_8404rule(0x0b1c, 0x049, 0x00); // 8404 test

	for(i=0x07; i<0x47; i+=4) equites_8404rule(0x0c64, i, 0xff); // checksum

	equites_8404rule(0x0df6, 0x00f, 0x04); // 1st gateway (init)
	equites_8404rule(0x0e02, 0x247, 0x01); // no. of addresses to look up - 1
	equites_8404rule(0x0e06, 0x249, 0x10); // addr hi
	equites_8404rule(0x0e0c, 0x24b, 0x12); // addr lo
	equites_8404rule(0x0e06, 0x24d, 0x19); // addr hi
	equites_8404rule(0x0e0c, 0x24f, 0x96); // addr lo

	equites_8404rule(0x10fc, 0x017, 0x04); // 2nd gateway (intro)
	equites_8404rule(0x111e, 0x6a5, 0x00); // no. of addresses to look up - 1
	equites_8404rule(0x1122, 0x6a7, 0x11); // addr hi
	equites_8404rule(0x1128, 0x6a9, 0xa4); // addr lo

	equites_8404rule(0x0f86, 0x013, 0x04); // 3rd gateway (miss)
	equites_8404rule(0x0f92, 0x491, 0x03); // no. of addresses to look up - 1
	equites_8404rule(0x0f96, 0x493,-0x60); // addr hi
	equites_8404rule(0x0f9c, 0x495,-0x06); // addr lo
	equites_8404rule(0x0f96, 0x497,-0x60); // addr hi
	equites_8404rule(0x0f9c, 0x499,-0x06); // addr lo
	equites_8404rule(0x0f96, 0x49b,-0x60); // addr hi
	equites_8404rule(0x0f9c, 0x49d,-0x06); // addr lo
	equites_8404rule(0x0f96, 0x49f,-0x60); // addr hi
	equites_8404rule(0x0f9c, 0x4a1,-0x06); // addr lo
}

/******************************************************************************/

// Game Entries

// Equites Hardware
GAMEX( 1984, equites,  0,        equites,  equites,  equites,  ROT90, "Alpha Denshi Co.",                "Equites", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1984, equitess, equites,  equites,  equites,  equites,  ROT90, "Alpha Denshi Co. (Sega license)", "Equites (Sega)", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1984, bullfgtr, 0,        equites,  bullfgtr, bullfgtr, ROT90, "Alpha Denshi Co. (Sega license)", "Bull Fighter", GAME_UNEMULATED_PROTECTION | GAME_WRONG_COLORS )
GAMEX( 1985, kouyakyu, 0,        equites,  kouyakyu, kouyakyu, ROT0,  "Alpha Denshi Co.",                "The Koukouyakyuh", GAME_UNEMULATED_PROTECTION )

// Splendor Blast Hardware
GAMEX( 1985, splndrbt, 0,        splndrbt, splndrbt, splndrbt, ROT0,  "Alpha Denshi Co.", "Splendor Blast", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1985, hvoltage, 0,        splndrbt, hvoltage, hvoltage, ROT0,  "Alpha Denshi Co.", "High Voltage", GAME_UNEMULATED_PROTECTION )

/******************************************************************************/

