/***************************************************************************

              Centipede Memory map and Dip Switches
              -------------------------------------

Memory map for Centipede directly from the Atari schematics (1981).

 Address  R/W  D7 D6 D5 D4 D3 D2 D1 D0   Function
--------------------------------------------------------------------------------------
0000-03FF       D  D  D  D  D  D  D  D   RAM
--------------------------------------------------------------------------------------
0400-07BF       D  D  D  D  D  D  D  D   Playfield RAM
07C0-07CF       D  D  D  D  D  D  D  D   Motion Object Picture
07D0-07DF       D  D  D  D  D  D  D  D   Motion Object Vert.
07E0-07EF       D  D  D  D  D  D  D  D   Motion Object Horiz.
07F0-07FF             D  D  D  D  D  D   Motion Object Color
--------------------------------------------------------------------------------------
0800       R    D  D  D  D  D  D  D  D   Option Switch 1 (0 = On)
0801       R    D  D  D  D  D  D  D  D   Option Switch 2 (0 = On)
--------------------------------------------------------------------------------------
0C00       R    D           D  D  D  D   Horizontal Mini-Track Ball tm Inputs
           R       D                     VBLANK  (1 = VBlank)
           R          D                  Self-Test  (0 = On)
           R             D               Cocktail Cabinet  (1 = Cocktail)
0C01       R    D  D  D                  R,C,L Coin Switches (0 = On)
           R             D               SLAM  (0 = On)
           R                D            Player 2 Fire Switch (0 = On)
           R                   D         Player 1 Fire Switch (0 = On)
           R                      D      Player 2 Start Switch (0 = On)
           R                         D   Player 1 Start Switch (0 = On)

0C02       R    D           D  D  D  D   Vertical Mini-Track Ball tm Inputs
0C03       R    D  D  D  D               Player 1 Joystick (R,L,Down,Up)
           R                D  D  D  D   Player 2 Joystick   (0 = On)
--------------------------------------------------------------------------------------
1000-100F R/W   D  D  D  D  D  D  D  D   Custom Audio Chip
1404       W                D  D  D  D   Playfield Color RAM
140C       W                D  D  D  D   Motion Object Color RAM
--------------------------------------------------------------------------------------
1600       W    D  D  D  D  D  D  D  D   EA ROM Address & Data Latch
1680       W                D  D  D  D   EA ROM Control Latch
1700       R    D  D  D  D  D  D  D  D   EA ROM Read Data
--------------------------------------------------------------------------------------
1800       W                             IRQ Acknowledge
--------------------------------------------------------------------------------------
1C00       W    D                        Left Coin Counter (1 = On)
1C01       W    D                        Center Coin Counter (1 = On)
1C02       W    D                        Right Coin Counter (1 = On)
1C03       W    D                        Player 1 Start LED (0 = On)
1C04       W    D                        Player 2 Start LED (0 = On)
1C07       W    D                        Track Ball Flip Control (0 = Player 1)
--------------------------------------------------------------------------------------
2000       W                             WATCHDOG
2400       W                             Clear Mini-Track Ball Counters
--------------------------------------------------------------------------------------
2000-3FFF  R                             Program ROM
--------------------------------------------------------------------------------------

-EA ROM is an Erasable Reprogrammable rom to save the top 3 high scores
  and other stuff.


 Dip switches at N9 on the PCB

 8    7    6    5    4    3    2    1    Option
-------------------------------------------------------------------------------------
                              On   On    English $
                              On   Off   German
                              Off  On    French
                              Off  Off   Spanish
-------------------------------------------------------------------------------------
                    On   On              2 lives per game
                    On   Off             3 lives per game $
                    Off  On              4 lives per game
                    Off  Off             5 lives per game
-------------------------------------------------------------------------------------
                                         Bonus life granted at every:
          On   On                        10,000 points
          On   Off                       12.000 points $
          Off  On                        15,000 points
          Off  Off                       20,000 points
-------------------------------------------------------------------------------------
     On                                  Hard game difficulty
     Off                                 Easy game difficulty $
-------------------------------------------------------------------------------------
On                                       1-credit minimum $
Off                                      2-credit minimum
-------------------------------------------------------------------------------------

$ = Manufacturer's suggested settings


 Dip switches at N8 on the PCB

 8    7    6    5    4    3    2    1    Option
-------------------------------------------------------------------------------------
                              On   On    Free play
                              On   Off   1 coin for 2 credits
                              Off  On    1 coin for 1 credit $
                              Off  Off   2 coins for 1 credit
-------------------------------------------------------------------------------------
                    On   On              Right coin mech X 1 $
                    On   Off             Right coin mech X 4
                    Off  On              Right coin mech X 5
                    Off  Off             Right coin mech X 6
-------------------------------------------------------------------------------------
               On                        Left coin mech X 1 $
               Off                       Left coin mech X 2
-------------------------------------------------------------------------------------
On   On   On                             No bonus coins $
On   On   Off                            For every 2 coins inserted, game logic
                                          adds 1 more coin
On   Off  On                             For every 4 coins inserted, game logic
                                          adds 1 more coin
On   Off  Off                            For every 4 coins inserted, game logic
                                          adds 2 more coin
Off  On   On                             For every 5 coins inserted, game logic
                                          adds 1 more coin
Off  On   Off                            For every 3 coins inserted, game logic
                                          adds 1 more coin
-------------------------------------------------------------------------------------
$ = Manufacturer's suggested settings

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokyintf.h"


int centiped_IN0_r(int offset);

int centiped_trakball_x(int data);
int centiped_trakball_y(int data);

extern unsigned char *centiped_charpalette,*centiped_spritepalette;
void centiped_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void centiped_vh_screenrefresh(struct osd_bitmap *bitmap);
void centiped_vh_charpalette_w(int offset, int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x0c00, 0x0c00, centiped_IN0_r },	/* IN0 */
	{ 0x0c01, 0x0c01, input_port_1_r },	/* IN1 */
	{ 0x0c02, 0x0c02, input_trak_1_r },	/* IN2 */
	{ 0x0c03, 0x0c03, input_port_3_r },	/* IN3 */
	{ 0x0800, 0x0800, input_port_4_r },	/* DSW1 */
	{ 0x0801, 0x0801, input_port_5_r },	/* DSW2 */
	{ 0x1000, 0x100f, pokey1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07bf, videoram_w, &videoram, &videoram_size },
	{ 0x07c0, 0x07ff, MWA_RAM, &spriteram },
	{ 0x1000, 0x100f, pokey1_w },
	{ 0x1404, 0x1407, centiped_vh_charpalette_w, &centiped_charpalette },
	{ 0x140c, 0x140f, MWA_RAM, &centiped_spritepalette },
	{ 0x1800, 0x1800, MWA_NOP },
	{ 0x1c00, 0x1c07, MWA_NOP },
	{ 0x1680, 0x1680, MWA_NOP },
	{ 0x2000, 0x2000, MWA_NOP },
	{ 0x2000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x20,
		{ 0, 0, 0, 0, 0, OSD_KEY_F2, IPB_VBLANK, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_CONTROL, OSD_KEY_CONTROL, 0, 0, OSD_KEY_3, 0 },
		{ 0, 0, OSD_JOY_FIRE, OSD_JOY_FIRE, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN3 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT }
	},
	{	/* DSW1 */
		0x54,
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

static struct TrakPort trak_ports[] = {
  {
    X_AXIS,
    1,
    1.0,
    centiped_trakball_x
  },
  {
    Y_AXIS,
    1,
    1.0,
    centiped_trakball_y
  },
  { -1 }
};


static struct KEYSet keys[] =
{
        { 3, 0, "MOVE UP" },
        { 3, 2, "MOVE LEFT"  },
        { 3, 3, "MOVE RIGHT" },
        { 3, 1, "MOVE DOWN" },
        { 1, 2, "PL1 FIRE" },
        { 1, 3, "PL2 FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x0c, "LIVES", { "2", "3", "4", "5" } },
	{ 4, 0x30, "BONUS", { "10000", "12000", "15000", "20000" } },
	{ 4, 0x40, "DIFFICULTY", { "HARD", "EASY" }, 1 },
	{ 4, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 256*8*8, 0 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,8,	/* 16*8 sprites */
	128,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 128*16*8, 0 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   4, 1 },
	{ 1, 0x0000, &spritelayout, 0, 1 },
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
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16, 2*4,
	centiped_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	centiped_vh_screenrefresh,

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

ROM_START( centiped_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "centiped.307", 0x2000, 0x0800, 0x2409fc03 )
	ROM_LOAD( "centiped.308", 0x2800, 0x0800, 0x209922dd )
	ROM_LOAD( "centiped.309", 0x3000, 0x0800, 0x57cee11e )
	ROM_LOAD( "centiped.310", 0x3800, 0x0800, 0xb959c639 )
	ROM_RELOAD(         0xf800, 0x0800 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "centiped.211", 0x0000, 0x0800, 0x704a2608 )
	ROM_LOAD( "centiped.212", 0x0800, 0x0800, 0xc9016e3f )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0002],"\x43\x65\x01",3) == 0 &&
			memcmp(&RAM[0x0017],"\x02\x21\x01",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0002],1,6*8,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x0002],1,6*8,f);
		fclose(f);
	}
}



struct GameDriver centiped_driver =
{
	"Centipede",
	"centiped",
	"IVAN MACKINTOSH\nEDWARD MASSEY\nPETE RITTWAGE\nNICOLA SALMORIA\nMIRKO BUFFONI",
	&machine_driver,

	centiped_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
