/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg


              Warlords Memory map and Dip Switches (preliminary}
              ------------------------------------

 Address  R/W  D7 D6 D5 D4 D3 D2 D1 D0   Function
--------------------------------------------------------------------------------------
0000-03FF       D  D  D  D  D  D  D  D   RAM
--------------------------------------------------------------------------------------
0400-07BF       D  D  D  D  D  D  D  D   Screen RAM (8x8 TILES, 32x32 SCREEN)
07C0-07CF       D  D  D  D  D  D  D  D   Motion Object Picture
07D0-07DF       D  D  D  D  D  D  D  D   Motion Object Vert.
07E0-07EF       D  D  D  D  D  D  D  D   Motion Object Horiz.
07F0-07FF             D  D  D  D  D  D   Motion Object Color ????????
--------------------------------------------------------------------------------------
0800       R    D  D  D  D  D  D  D  D   Option Switch 1 (0 = On) (DSW 1)
0801       R    D  D  D  D  D  D  D  D   Option Switch 2 (0 = On) (DSW 2)
--------------------------------------------------------------------------------------
0C00       R    D                        Cocktail Cabinet  (0 = Cocktail)
           R       D                     VBLANK  (1 = VBlank)
           R          D                  SELF TEST
0C01       R    D  D  D                  R,C,L Coin Switches (0 = On)
           R             D               Slam (0 = On)
           R                D            Player 4 Start Switch (0 = On)
           R                   D         Player 3 Start Switch (0 = On)
           R                      D      Player 2 Start Switch (0 = On)
           R                         D   Player 1 Start Switch (0 = On)
--------------------------------------------------------------------------------------
1000-100F  W   D  D  D  D  D  D  D  D    Pokey
1000       R   D  D  D  D  D  D  D  D    Paddle 1 position
1001       R   D  D  D  D  D  D  D  D    Paddle 2 position
1002       R   D  D  D  D  D  D  D  D    Paddle 3 position
1003       R   D  D  D  D  D  D  D  D    Paddle 4 position
100A       R   D  D  D  D  D  D  D  D    Random Number
--------------------------------------------------------------------------------------
1010-10FF  W    D  D  D  D  D  D  D  D   ????
--------------------------------------------------------------------------------------
1800       W                             IRQ Acknowledge
--------------------------------------------------------------------------------------
1C00-1CFF  W    D  D  D  D  D  D  D  D   ????
--------------------------------------------------------------------------------------
4000       W                             WATCHDOG
--------------------------------------------------------------------------------------
5000-7FFF  R                             Program ROM
--------------------------------------------------------------------------------------

1c00-1c02 - coin counters
1c03-1c06 - color

Game Option Settings - J2 (DSW1)
=========================

8   7   6   5   4   3   2   1       Option
------------------------------------------
                        On  On      English
                        On  Off     French
                        Off On      Spanish
                        Off Off     German
                    On              Music at end of each game
                    Off             Music at end of game for new highscore
        On  On                      1 or 2 player game costs 1 credit
        On  Off                     1 player game=1 credit, 2 player=2 credits
        Off Off                     1 or 2 player game costs 2 credits
        Off On                      Not used
-------------------------------------------


Game Price Settings - M2 (DSW2)
========================

8   7   6   5   4   3   2   1       Option
------------------------------------------
                        On  On      Free play
                        On  Off     1 coin for 2 credits
                        Off On      1 coin for 1 credit
                        Off Off     2 coins for 1 credit
                On  On              Right coin mech x 1
                On  Off             Right coin mech x 4
                Off On              Right coin mech x 5
                Off Off             Right coin mech x 6
            On                      Left coin mech x 1
            Off                     Left coin mech x 2
On  On  On                          No bonus coins
On  On  Off                         For every 2 coins, add 1 coin
On  Off On                          For every 4 coins, add 1 coin
On  Off Off                         For every 4 coins, add 2 coins
Off On  On                          For every 5 coins, add 1 coin
------------------------------------------


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


int warlord_pot1_r (int offset);
int warlord_pot2_r (int offset);
int warlord_pot3_r (int offset);
int warlord_pot4_r (int offset);
int warlord_trakball_r (int offset);

void warlord_vh_screenrefresh(struct osd_bitmap *bitmap);
void warlord_paletteram_w (int offset, int data);

extern unsigned char *warlord_paletteram;


void warlord_led_w(int offset,int data)
{
	osd_led_w(offset,~data >> 7);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x0800, 0x0800, input_port_2_r },	/* DSW1 */
	{ 0x0801, 0x0801, input_port_3_r },	/* DSW2 */
	{ 0x0c00, 0x0c00, input_port_0_r },	/* IN0 */
	{ 0x0c01, 0x0c01, input_port_1_r },	/* IN1 */
	{ 0x1000, 0x100f, pokey1_r },		/* Read the 4 paddle values & the random # gen */
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },		/* for the reset / interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07bf, videoram_w, &videoram, &videoram_size },
	{ 0x07c0, 0x07ff, MWA_RAM, &spriteram },
	{ 0x1000, 0x100f, pokey1_w },
	{ 0x1800, 0x1800, MWA_NOP },
	{ 0x1c00, 0x1c02, coin_counter_w },
	{ 0x1c03, 0x1c06, warlord_led_w },	/* 4 start lights */
	{ 0x4000, 0x4000, watchdog_reset_w },
	{ 0x5000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME (0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Upright" )
	PORT_DIPSETTING (   0x80, "Cocktail" )

	PORT_START	/* IN1 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "1p Button/Start", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "2p Button/Start", OSD_KEY_ALT, IP_JOY_NONE, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3, "3p Button/Start", OSD_KEY_SPACE, IP_JOY_NONE, 0 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4, "4p Button/Start", OSD_KEY_L, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN2 */
	PORT_DIPNAME (0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "English" )
	PORT_DIPSETTING (   0x01, "French" )
	PORT_DIPSETTING (   0x02, "Spanish" )
	PORT_DIPSETTING (   0x03, "German" )
	PORT_DIPNAME (0x04, 0x00, "Music", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "End of game" )
	PORT_DIPSETTING (   0x04, "High score only" )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME (0x60, 0x00, "Credits", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "1p/2p = 1 credit" )
	PORT_DIPSETTING (   0x20, "1p = 1, 2p = 2" )
	PORT_DIPSETTING (   0x60, "1p/2p = 2 credits" )

	PORT_START	/* IN3 */
	PORT_DIPNAME (0x03, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Free Play" )
	PORT_DIPSETTING (   0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x03, "2 Coins/1 Credit" )
	PORT_DIPNAME (0x0c, 0x00, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x04, "*4" )
	PORT_DIPSETTING (   0x08, "*5" )
	PORT_DIPSETTING (   0x0c, "*6" )
	PORT_DIPNAME (0x10, 0x00, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x10, "*2" )
	PORT_DIPNAME (0xe0, 0x00, "Bonus Coins", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "None" )
	PORT_DIPSETTING (   0x20, "3 credits/2 coins" )
	PORT_DIPSETTING (   0x40, "5 credits/4 coins" )
	PORT_DIPSETTING (   0x60, "6 credits/4 coins" )
	PORT_DIPSETTING (   0x80, "6 credits/5 coins" )

	PORT_START	/* IN4 - fake to control player 1 paddle */
	PORT_ANALOG ( 0xff, 0x80, IPT_PADDLE, 50, 32, 0x1d, 0xcb )

	PORT_START	/* IN5 - fake to control players 2-4 with keyboard */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2, "2p Right", OSD_KEY_A, IP_JOY_NONE, 0 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2, "2p Right", OSD_KEY_S, IP_JOY_NONE, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER3, "3p Right", OSD_KEY_V, IP_JOY_NONE, 0 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER3, "3p Right", OSD_KEY_B, IP_JOY_NONE, 0 )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER4, "4p Right", OSD_KEY_O, IP_JOY_NONE, 0 )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER4, "4p Right", OSD_KEY_P, IP_JOY_NONE, 0 )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	128,	/* 128 characters */
	2,	/* 2 bits per pixel */
	{ 0, 128*8*8 },	/* bitplane separation */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x0000, &charlayout,  12, 1 },
	{ -1 } /* end of array */
};



static struct POKEYinterface pokey_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz??? */
	255,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ input_port_4_r },
	{ warlord_pot2_r },
	{ warlord_pot3_r },
	{ warlord_pot4_r },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ 0 },
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16, 16,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	warlord_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( warlord_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "037154.1m", 0x5000, 0x0800, 0x69a5fadb )
	ROM_LOAD( "037153.1k", 0x5800, 0x0800, 0x13ee094a )
	ROM_LOAD( "037158.1j", 0x6000, 0x0800, 0x038996f3 )
	ROM_LOAD( "037157.1h", 0x6800, 0x0800, 0xa259de59 )
	ROM_LOAD( "037156.1e", 0x7000, 0x0800, 0x363914bd )
	ROM_LOAD( "037155.1d", 0x7800, 0x0800, 0x4880f13a )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "037159.6e", 0x0000, 0x0800, 0x98aea2be )
ROM_END



struct GameDriver warlord_driver =
{
	"Warlords",
	"warlord",
	"Lee Taylor\nJohn Clegg\nBrad Oliver (additional code)",
	&machine_driver,

	warlord_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
