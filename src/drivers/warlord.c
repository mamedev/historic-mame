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
#include "sndhrdw/pokyintf.h"


int warlord_pots(int offset);
int warlord_trakball_r (int offset);

void warlord_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void warlord_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x0c00, 0x0c00, input_port_0_r },	/* IN0 */
	{ 0x0c01, 0x0c01, input_port_1_r },	/* IN1 */
	{ 0x0800, 0x0800, input_port_4_r },	/* DSW1 */
	{ 0x0801, 0x0801, input_port_5_r },	/* DSW2 */
	{ 0x1000, 0x100f, warlord_pots },   /* Read the 4 pokey pot values & the random # gen */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07bf, videoram_w, &videoram, &videoram_size },
	{ 0x07c0, 0x07ff, MWA_RAM, &spriteram },
	{ 0x1000, 0x100f, pokey1_w },
	{ 0x1010, 0x10ff, MWA_NOP },
	{ 0x1800, 0x1800, MWA_NOP },
	{ 0x1c00, 0x1cff, MWA_NOP },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x5000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x20,
 		{ 0, 0, 0, 0, 0, OSD_KEY_F2, IPB_VBLANK, OSD_KEY_C },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, OSD_KEY_3, 0 },
                { OSD_JOY_FIRE, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN3 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x00,
                { 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x02,
                { 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        {
          X_AXIS,
          1,
          1.0,
          warlord_trakball_r
        },
        { -1 }
};


static struct KEYSet keys[] =
{
        { 1, 0, "PL1 FIRE / START" },
        { 1, 1, "PL2 FIRE / START" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 5, 0x03, "COINS PER CRED", { "FREE PLAY", "1CO=2CR", "1CO=1CR", "2CO=1CR" } },
	{ 5, 0x0C, "RIGHT COIN MULTIPLIER", { "X1", "X4", "X5", "X6" } },
	{ 5, 0x10, "LEFT COIN MULTIPLIER", { "X1", "X2" } },
	{ 4, 0x30, "CRED PER PLAY", { "1&2UP=1", "1UP=1,2UP=2", "N/A", "1&2UP=2" } },
	{ 4, 0x04, "END MUSIC", { "ALWAYS", "HISCORE" } },
	{ 4, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	128,	/* 128 characters */
	2,	/* 2 bits per pixel */
	{ 128*8*8 , 0 },	/* bitplane separation */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ -1 } /* end of array */
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
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16, 2*4,
	warlord_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	warlord_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	pokey1_sh_start,
	pokey_sh_stop,
	pokey_sh_update
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



static int hiload(const char *name)
{
	return 0;
}



static void hisave(const char *name)
{

}



struct GameDriver warlord_driver =
{
	"Warlords",
	"warlord",
	"LEE TAYLOR\nJOHN CLEGG",
	&machine_driver,

	warlord_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
