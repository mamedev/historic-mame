/******************************************************************
Terra Cresta (preliminary)
Nichibutsu 1985
68000 + Z80

driver by Carlos A. Lozano (calb@gsyc.inf.uc3m.es)

TODO:

  - I'm playing samples with a DAC, but they could be ADPCM
  - find correct high-scores table for 'amazon'


Stephh's notes (based on the games M68000 code and some tests) :


1) 'terracr*'

  - Each high-score name is constitued of 10 chars.


2) 'amazon'

  - Each high-score name is constitued of 3 chars followed by 7 0x00.


3) 'amatelas'

  - Each high-score name is constitued of 10 chars.


4) 'horekid*'

  - Each high-score name is constitued of 3 chars.

  - There is a "debug mode" ! To activate it, you need to have cheats ON.
    Set "Debug Mode" Dip Switch to ON and be sure that "Cabinet" Dip Switch
    is set to "Upright". Its features (see below) only affect player 1 !
    Features :
      * invulnerability and infinite time :
          . insert a coin
          . press FAKE button 3 and START1 (player 1 buttons 1 and 2 must NOT be pressed !)
      * level select (there are 32 levels) :
          . insert a coin
          . press FAKE button 3 ("00" will be displayed - this is an hex. display)
          . press FAKE button 3 and player 1 button 1 to increase level
          . press FAKE button 3 and player 1 button 2 to decrease level
          . press START1 to start a game with the selected level
    FAKE button 3 is in fact the same as pressing simultaneously player 2 buttons 1 and 2.
    (I've code this that way because my keyboard doesn't accept too many keys pressed)



Amazon
(c)1986 Nichibutsu

AT-1
                                                16MHz

                  6116    -     -    10    9
                  6116    -     -    12    11   68000-8

 6116
 6116
                                              SW
    15 14 13   clr.12f clr.11f clr.10f        SW

    16   1412M2 XBA


AT-2

 2G    6  7                              8
    4E 4  5                              6116

                                         Z80A   YM3526
      2148 2148
      2148 2148         2148 2148
                                         1 2 3  6116
 22MHz
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

static const data16_t *mpProtData;
static data8_t mAmazonProtCmd;
static data8_t mAmazonProtReg[6];

extern data16_t *amazon_videoram;

extern PALETTE_INIT( amazon );
extern WRITE16_HANDLER( amazon_background_w );
extern WRITE16_HANDLER( amazon_foreground_w );
extern WRITE16_HANDLER( amazon_scrolly_w );
extern WRITE16_HANDLER( amazon_scrollx_w );
extern WRITE16_HANDLER( amazon_flipscreen_w );
extern VIDEO_START( amazon );
extern VIDEO_UPDATE( amazon );

static const data16_t mAmazonProtData[] =
{
	/* default high scores (0x40db4) - wrong data ? */
	0x0000,0x5000,0x5341,0x4b45,0x5349,0x4755,0x5245,
	0x0000,0x4000,0x0e4b,0x4154,0x5544,0x4f4e,0x0e0e,
	0x0000,0x3000,0x414e,0x4b41,0x4b45,0x5544,0x4f4e,
	0x0000,0x2000,0x0e0e,0x4b49,0x5455,0x4e45,0x0e0e,
	0x0000,0x1000,0x0e4b,0x414b,0x4553,0x4f42,0x410e,

	/* code (0x40d92) */
	0x4ef9,0x0000,0x62fa,0x0000,0x4ef9,0x0000,0x805E,0x0000,
	0xc800 /* checksum */
};

static const data16_t mAmatelasProtData[] =
{
	/* default high scores (0x40db4) */
	0x0000,0x5000,0x5341,0x4b45,0x5349,0x4755,0x5245,
	0x0000,0x4000,0x0e4b,0x4154,0x5544,0x4f4e,0x0e0e,
	0x0000,0x3000,0x414e,0x4b41,0x4b45,0x5544,0x4f4e,
	0x0000,0x2000,0x0e0e,0x4b49,0x5455,0x4e45,0x0e0e,
	0x0000,0x1000,0x0e4b,0x414b,0x4553,0x4f42,0x410e,

	/* code (0x40d92) */
	0x4ef9,0x0000,0x632e,0x0000,0x4ef9,0x0000,0x80C2,0x0000,
	0x6100 /* checksum */
};

static const data16_t mHoreKidProtData[] =
{
	/* N/A */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,

	/* code (0x40dba) */
	0x4e75,0x4e75,0x4e75,0x4e75,0x4e75,0x4e75,0x4e75,0x4e75,
	0x1800 /* checksum */
};

static READ16_HANDLER( horekid_IN2_r )
{
	int data = readinputport(2);

	if (!(data & 0x40))		// FAKE button 3 for "Debug Mode"
	{
		data &=  0x40;
		data |= ~0x30;
	}

	return data;
}

static WRITE16_HANDLER( amazon_sound_w )
{
	soundlatch_w(0,((data & 0x7f) << 1) | 1);
}

static READ_HANDLER( soundlatch_clear_r )
{
	soundlatch_clear_w(0,0);
	return 0;
}

static READ16_HANDLER( amazon_protection_r )
{
	offset = mAmazonProtReg[2];
	if( offset<=0x56 )
	{
		data16_t data;
		data = mpProtData[offset/2];
		if( offset&1 ) return data&0xff;
		return data>>8;
	}
	return 0;
}

static WRITE16_HANDLER( amazon_protection_w )
{
	if( ACCESSING_LSB )
	{
		if( offset==1 )
		{
			mAmazonProtCmd = data;
		}
		else
		{
			if( mAmazonProtCmd>=32 && mAmazonProtCmd<=0x37 )
			{
				mAmazonProtReg[mAmazonProtCmd-0x32] = data;
			}
		}
	}
}



static MEMORY_READ16_START( terracre_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x020000, 0x0201ff, MRA16_RAM },
	{ 0x020200, 0x021fff, MRA16_RAM },
	{ 0x023000, 0x023fff, MRA16_RAM },
	{ 0x024000, 0x024001, input_port_0_word_r },
	{ 0x024002, 0x024003, input_port_1_word_r },
	{ 0x024004, 0x024005, input_port_2_word_r },
	{ 0x024006, 0x024007, input_port_3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( terracre_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x020000, 0x0201ff, MWA16_RAM, &spriteram16 },
	{ 0x020200, 0x021fff, MWA16_RAM },
	{ 0x022000, 0x022fff, amazon_background_w, &amazon_videoram },
	{ 0x023000, 0x023fff, MWA16_RAM },
	{ 0x026000, 0x026001, amazon_flipscreen_w },	/* flip screen & coin counters */
	{ 0x026002, 0x026003, amazon_scrollx_w },
	{ 0x026004, 0x026005, amazon_scrolly_w },
	{ 0x02600c, 0x02600d, amazon_sound_w },
	{ 0x028000, 0x0287ff, amazon_foreground_w, &videoram16 },
MEMORY_END

static MEMORY_READ16_START( amazon_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x040fff, MRA16_RAM },
	{ 0x044000, 0x044001, input_port_0_word_r },
	{ 0x044002, 0x044003, input_port_1_word_r },
	{ 0x044004, 0x044005, input_port_2_word_r },
	{ 0x044006, 0x044007, input_port_3_word_r },
	{ 0x070000, 0x070001, amazon_protection_r },
MEMORY_END

static MEMORY_WRITE16_START( amazon_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x040000, 0x0401ff, MWA16_RAM, &spriteram16 },
	{ 0x040200, 0x040fff, MWA16_RAM },
	{ 0x042000, 0x042fff, amazon_background_w, &amazon_videoram },
	{ 0x046000, 0x046001, amazon_flipscreen_w },	/* flip screen & coin counters */
	{ 0x046002, 0x046003, amazon_scrollx_w },
	{ 0x046004, 0x046005, amazon_scrolly_w },
	{ 0x04600c, 0x04600d, amazon_sound_w },
	{ 0x050000, 0x050fff, amazon_foreground_w, &videoram16 },
	{ 0x070000, 0x070003, amazon_protection_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x04, 0x04, soundlatch_clear_r },
	{ 0x06, 0x06, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_3526 )
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END

static PORT_WRITE_START( sound_writeport_2203 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END

INPUT_PORTS_START( terracre )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x2000, IP_ACTIVE_LOW )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "20k then every 60k" )	// "20000 60000" in the "test mode"
	PORT_DIPSETTING(      0x0008, "30k then every 70k" )	// "30000 70000" in the "test mode"
	PORT_DIPSETTING(      0x0004, "40k then every 80k" )	// "40000 80000" in the "test mode"
	PORT_DIPSETTING(      0x0000, "50k then every 90k" )	// "50000 90000" in the "test mode"
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Flip_Screen ) )	// not in the "test mode"
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Complete Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Base Ship Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( amazon )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x2000, IP_ACTIVE_LOW )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "20k then every 40k" )	// "20000 40000" in the "test mode"
	PORT_DIPSETTING(      0x0008, "50k then every 40k" )	// "50000 40000" in the "test mode"
	PORT_DIPSETTING(      0x0004, "20k then every 70k" )	// "20000 70000" in the "test mode"
	PORT_DIPSETTING(      0x0000, "50k then every 70k" )	// "50000 70000" in the "test mode"
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Flip_Screen ) )	// not in the "test mode"
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Level" )
	PORT_DIPSETTING(      0x4000, "Low" )
	PORT_DIPSETTING(      0x0000, "High" )
	PORT_DIPNAME( 0x8000, 0x8000, "Sprite Test" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( horekid )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "20k then every 60k" )	// "20000 60000" in the "test mode"
	PORT_DIPSETTING(      0x0008, "50k then every 60k" )	// "50000 60000" in the "test mode"
	PORT_DIPSETTING(      0x0004, "20k then every 90k" )	// "20000 90000" in the "test mode"
	PORT_DIPSETTING(      0x0000, "50k then every 90k" )	// "50000 90000" in the "test mode"
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x00c0, "Easy" )
	PORT_DIPSETTING(      0x0080, "Normal" )
	PORT_DIPSETTING(      0x0040, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0xc000, 0xc000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0xc000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )		// "Cabinet" Dip Switch must be set to "Upright" too !
//	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )		// duplicated setting
//	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )		// duplicated setting

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x2000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_CHEAT )	// fake button for "Debug Mode" (see read handler)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct GfxLayout char_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{
		4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24
	},
	{
		0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64
	},
	64*16
};

static struct GfxLayout sprite_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{
		 4,  0, RGN_FRAC(1,2)+4,  RGN_FRAC(1,2)+0,
		12,  8, RGN_FRAC(1,2)+12, RGN_FRAC(1,2)+8,
		20, 16, RGN_FRAC(1,2)+20, RGN_FRAC(1,2)+16,
		28, 24, RGN_FRAC(1,2)+28, RGN_FRAC(1,2)+24
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32
	},
	32*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,            0,   1 },
	{ REGION_GFX2, 0, &tile_layout,         1*16,  16 },
	{ REGION_GFX3, 0, &sprite_layout, 1*16+16*16, 256 },
	{ -1 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ 100 }		/* volume */
};

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ???? */
	{ YM2203_VOL(40,20), YM2203_VOL(40,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	2,	/* 2 channels */
	{ 50, 50 }
};

static MACHINE_DRIVER_START( amazon )
	MDRV_CPU_ADD(M68000, 8000000 )
	MDRV_CPU_MEMORY(amazon_readmem,amazon_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_3526)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,128)	/* ??? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1*16+16*16+16*256)

	MDRV_PALETTE_INIT(amazon)
	MDRV_VIDEO_START(amazon)
	MDRV_VIDEO_UPDATE(amazon)

	MDRV_SOUND_ADD(YM3526, ym3526_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ym3526 )
	MDRV_CPU_ADD(M68000, 8000000 )
	MDRV_CPU_MEMORY(terracre_readmem,terracre_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_3526)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,128)	/* ??? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1*16+16*16+16*256)

	MDRV_PALETTE_INIT(amazon)
	MDRV_VIDEO_START(amazon)
	MDRV_VIDEO_UPDATE(amazon)

	MDRV_SOUND_ADD(YM3526, ym3526_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ym2203 )
	MDRV_CPU_ADD(M68000, 8000000) /* 8 MHz?? */
	MDRV_CPU_MEMORY(terracre_readmem,terracre_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_2203)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,128)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1*16+16*16+16*256)

	MDRV_PALETTE_INIT(amazon)
	MDRV_VIDEO_START(amazon)
	MDRV_VIDEO_UPDATE(amazon)

	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

ROM_START( terracre )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "2a_15b.rom",   0x0000, 0x4000, 0x604c3b11 )
	ROM_LOAD( "2a_17b.rom",   0x4000, 0x4000, 0xaffc898d )
	ROM_LOAD( "2a_18b.rom",   0x8000, 0x4000, 0x302dc0ab )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

ROM_START( terracrb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "2a_15b.rom",   0x0000, 0x4000, 0x604c3b11 )
	ROM_LOAD( "dg.12",        0x4000, 0x4000, 0x9e9b3808 )
	ROM_LOAD( "2a_18b.rom",   0x8000, 0x4000, 0x302dc0ab )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

ROM_START( terracra )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k to sound cpu */
	ROM_LOAD( "tc2a_15b.bin", 0x0000, 0x4000, 0x790ddfa9 )
	ROM_LOAD( "tc2a_17b.bin", 0x4000, 0x4000, 0xd4531113 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END

ROM_START( amazon )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code (main CPU) */
	ROM_LOAD16_BYTE( "11.4d",	0x00000, 0x8000,0x6c7f85c5 )
	ROM_LOAD16_BYTE( "9.4b",	0x00001, 0x8000,0xe1b7a989 )
	ROM_LOAD16_BYTE( "12.6d",	0x10000, 0x8000,0x4de8a3ee )
	ROM_LOAD16_BYTE( "10.6b",	0x10001, 0x8000,0xd86bad81 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound) */
	ROM_LOAD( "1.15b",	0x00000, 0x4000, 0x55a8b5e7 )
	ROM_LOAD( "2.17b",	0x04000, 0x4000, 0x427a7cca )
	ROM_LOAD( "3.18b",	0x08000, 0x4000, 0xb8cceaf7 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumerics */
	ROM_LOAD( "8.16g",	0x0000, 0x2000, 0x0cec8644 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13.15f",	0x00000, 0x8000, 0x415ff4d9 )
	ROM_LOAD( "14.17f",	0x08000, 0x8000, 0x492b5c48 )
	ROM_LOAD( "15.18f",	0x10000, 0x8000, 0xb1ac0b9d )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "4.6e",	0x0000, 0x4000, 0xf77ced7a )
	ROM_LOAD( "5.7e",	0x4000, 0x4000, 0x16ef1465 )
	ROM_LOAD( "6.6g",	0x8000, 0x4000, 0x936ec941 )
	ROM_LOAD( "7.7g",	0xc000, 0x4000, 0x66dd718e )

	ROM_REGION( 0x2000, REGION_USER1, 0 ) /* unknown, mostly text */
	ROM_LOAD( "16.18g",	0x0000, 0x2000, 0x1d8d592b )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "clr.10f", 0x000, 0x100, 0x6440b341 ) /* red */
	ROM_LOAD( "clr.11f", 0x100, 0x100, 0x271e947f ) /* green */
	ROM_LOAD( "clr.12f", 0x200, 0x100, 0x7d38621b ) /* blue */
	ROM_LOAD( "2g",		 0x300, 0x100, 0x44ca16b9 ) /* clut */
	ROM_LOAD( "4e",		 0x400, 0x100, 0x035f2c7b ) /* ctable */
ROM_END

ROM_START( amatelas )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code (main CPU) */
	ROM_LOAD16_BYTE( "a11.4d",	0x00000, 0x8000,0x3d226d0b )
	ROM_LOAD16_BYTE( "a9.4b",	0x00001, 0x8000,0xe2a0d21d )
	ROM_LOAD16_BYTE( "a12.6d",	0x10000, 0x8000,0xe6607c51 )
	ROM_LOAD16_BYTE( "a10.6b",	0x10001, 0x8000,0xdbc1f1b4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound) */
	ROM_LOAD( "1.15b",	0x00000, 0x4000, 0x55a8b5e7 )
	ROM_LOAD( "2.17b",	0x04000, 0x4000, 0x427a7cca )
	ROM_LOAD( "3.18b",	0x08000, 0x4000, 0xb8cceaf7 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumerics */
	ROM_LOAD( "a8.16g",	0x0000, 0x2000, 0xaeba2102 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13.15f",	0x00000, 0x8000, 0x415ff4d9 )
	ROM_LOAD( "14.17f",	0x08000, 0x8000, 0x492b5c48 )
	ROM_LOAD( "15.18f",	0x10000, 0x8000, 0xb1ac0b9d )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "4.6e",	0x0000, 0x4000, 0xf77ced7a )
	ROM_LOAD( "a5.7e",	0x4000, 0x4000, 0x5fbf9a16 )
	ROM_LOAD( "6.6g",	0x8000, 0x4000, 0x936ec941 )
	ROM_LOAD( "7.7g",	0xc000, 0x4000, 0x66dd718e )

	ROM_REGION( 0x2000, REGION_USER1, 0 ) /* unknown, mostly text */
	ROM_LOAD( "16.18g",	0x0000, 0x2000, 0x1d8d592b )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "clr.10f", 0x000, 0x100, 0x6440b341 ) /* red */
	ROM_LOAD( "clr.11f", 0x100, 0x100, 0x271e947f ) /* green */
	ROM_LOAD( "clr.12f", 0x200, 0x100, 0x7d38621b ) /* blue */
	ROM_LOAD( "2g",		 0x300, 0x100, 0x44ca16b9 ) /* clut */
	ROM_LOAD( "4e",		 0x400, 0x100, 0x035f2c7b ) /* ctable */
ROM_END

ROM_START( horekid )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code (main CPU) */
	ROM_LOAD16_BYTE( "horekid.03",	0x00000, 0x8000, 0x90ec840f )
	ROM_LOAD16_BYTE( "horekid.01",	0x00001, 0x8000, 0xa282faf8 )
	ROM_LOAD16_BYTE( "horekid.04",	0x10000, 0x8000, 0x375c0c50 )
	ROM_LOAD16_BYTE( "horekid.02",	0x10001, 0x8000, 0xee7d52bb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound) */
	ROM_LOAD( "horekid.09",	0x0000, 0x4000,0x49cd3b81 )
	ROM_LOAD( "horekid.10",	0x4000, 0x4000,0xc1eaa938 )
	ROM_LOAD( "horekid.11",	0x8000, 0x4000,0x0a2bc702 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumerics */
	ROM_LOAD( "horekid.16",	0x0000, 0x2000, 0x104b77cc )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "horekid.05",	0x00000, 0x8000, 0xda25ae10 )
	ROM_LOAD( "horekid.06",	0x08000, 0x8000, 0x616e4321 )
	ROM_LOAD( "horekid.07",	0x10000, 0x8000, 0x8c7d2be2 )
	ROM_LOAD( "horekid.08",	0x18000, 0x8000, 0xa0066b02 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "horekid.12",	0x00000, 0x8000, 0xa3caa07a )
	ROM_LOAD( "horekid.13",	0x08000, 0x8000, 0x0e48ff8e )
	ROM_LOAD( "horekid.14",	0x10000, 0x8000, 0xe300747a )
	ROM_LOAD( "horekid.15",	0x18000, 0x8000, 0x51105741 )

	ROM_REGION( 0x2000, REGION_USER1, 0 ) /* unknown, mostly text */
	ROM_LOAD( "horekid.17",	0x0000, 0x2000, 0x1d8d592b )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "kid_prom.10f", 0x000, 0x100, 0xca13ce23 ) /* red */
	ROM_LOAD( "kid_prom.11f", 0x100, 0x100, 0xfb44285a ) /* green */
	ROM_LOAD( "kid_prom.12f", 0x200, 0x100, 0x40d41237 ) /* blue */
	ROM_LOAD( "kid_prom.2g",  0x300, 0x100, 0x4b9be0ed ) /* clut */
	ROM_LOAD( "kid_prom.4e",  0x400, 0x100, 0xe4fb54ee ) /* ctable */
ROM_END

ROM_START( horekidb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code (main CPU) */
	ROM_LOAD16_BYTE( "knhhd5", 0x00000, 0x8000, 0x786619c7 )
	ROM_LOAD16_BYTE( "knhhd7", 0x00001, 0x8000, 0x3bbb475b )
	ROM_LOAD16_BYTE( "horekid.04",	0x10000, 0x8000, 0x375c0c50 )
	ROM_LOAD16_BYTE( "horekid.02",	0x10001, 0x8000, 0xee7d52bb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound) */
	ROM_LOAD( "horekid.09",	0x0000, 0x4000,0x49cd3b81 )
	ROM_LOAD( "horekid.10",	0x4000, 0x4000,0xc1eaa938 )
	ROM_LOAD( "horekid.11",	0x8000, 0x4000,0x0a2bc702 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* alphanumerics */
	ROM_LOAD( "horekid.16",	0x0000, 0x2000, 0x104b77cc )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "horekid.05",	0x00000, 0x8000, 0xda25ae10 )
	ROM_LOAD( "horekid.06",	0x08000, 0x8000, 0x616e4321 )
	ROM_LOAD( "horekid.07",	0x10000, 0x8000, 0x8c7d2be2 )
	ROM_LOAD( "horekid.08",	0x18000, 0x8000, 0xa0066b02 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "horekid.12",	0x00000, 0x8000, 0xa3caa07a )
	ROM_LOAD( "horekid.13",	0x08000, 0x8000, 0x0e48ff8e )
	ROM_LOAD( "horekid.14",	0x10000, 0x8000, 0xe300747a )
	ROM_LOAD( "horekid.15",	0x18000, 0x8000, 0x51105741 )

	ROM_REGION( 0x2000, REGION_USER1, 0 ) /* unknown, mostly text */
	ROM_LOAD( "horekid.17",	0x0000, 0x2000, 0x1d8d592b )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "kid_prom.10f", 0x000, 0x100, 0xca13ce23 ) /* red */
	ROM_LOAD( "kid_prom.11f", 0x100, 0x100, 0xfb44285a ) /* green */
	ROM_LOAD( "kid_prom.12f", 0x200, 0x100, 0x40d41237 ) /* blue */
	ROM_LOAD( "kid_prom.2g",  0x300, 0x100, 0x4b9be0ed ) /* clut */
	ROM_LOAD( "kid_prom.4e",  0x400, 0x100, 0xe4fb54ee ) /* ctable */
ROM_END

DRIVER_INIT( amazon )
{
	mpProtData = mAmazonProtData;
}

DRIVER_INIT( amatelas )
{
	mpProtData = mAmatelasProtData;
}

DRIVER_INIT( horekid )
{
	mpProtData = mHoreKidProtData;
	install_mem_read16_handler(0, 0x44004, 0x44005, horekid_IN2_r);
}

/*    YEAR, NAME,   PARENT,     MACHINE, INPUT,    INIT,     MONITOR,  COMPANY,      FULLNAME, FLAGS */
GAME( 1985, terracre, 0,        ym3526,  terracre, 0,        ROT270,  "Nichibutsu", "Terra Cresta (YM3526 set 1)" )
GAME( 1985, terracrb, terracre, ym3526,  terracre, 0,        ROT270,  "Nichibutsu", "Terra Cresta (YM3526 set 2)" )
GAME( 1985, terracra, terracre, ym2203,  terracre, 0,        ROT270,  "Nichibutsu", "Terra Cresta (YM2203)" )
GAME( 1986, amazon,   0,        amazon,  amazon,   amazon,   ROT270,  "Nichibutsu", "Soldier Girl Amazon" )
GAME( 1986, amatelas, amazon,   amazon,  amazon,   amatelas, ROT270,  "Nichibutsu", "Sei Senshi Amatelass" )
GAME( 1987, horekid,  0,        amazon,  horekid,  horekid,  ROT270,  "Nichibutsu", "Kid no Hore Hore Daisakusen" )
GAME( 1987, horekidb, horekid,  amazon,  horekid,  horekid,  ROT270,  "bootleg", "Kid no Hore Hore Daisakusen (bootleg)" )
