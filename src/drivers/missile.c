/***************************************************************************


      Modified from original schematics...

      MISSILE COMMAND
      ---------------
      HEX        R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
      ---------+-----+------------------------+------------------------
      0000-01FF  R/W   D  D  D  D  D  D  D  D   512 bytes working ram

      0200-05FF  R/W   D  D  D  D  D  D  D  D   3rd color bit region
						                                    of screen ram.
      																          Each bit of every odd byte is the low color
      																          bit for the bottom scanlines
      																          The schematics say that its for the bottom
      																          32 scanlines, although the code only accesses
      																          $401-$5FF for the bottom 8 scanlines...
      																          Pretty wild, huh?

      0600-063F  R/W   D  D  D  D  D  D  D  D   More working ram.

      0640-3FFF  R/W   D  D  D  D  D  D  D  D   2-color bit region of
																								screen ram.
																								Writes to 4 bytes each
																								and mapped to $1900-$FFFF

			1900-FFFF  R/W   D  D                     2-color bit region of
																						    screen ram
			                                          Only accessed with
			                                           LDA ($ZZ,X) and
			                                           STA ($ZZ,X)
			                                          Those instructions take longer
			                                          than 5 cycles.

      ---------+-----+------------------------+------------------------
      4000-400F  R/W   D  D  D  D  D  D  D  D   POKEY ports.
      -----------------------------------------------------------------
      4008       R     D  D  D  D  D  D  D  D   Game Option switches
      -----------------------------------------------------------------
      4800       R     D                        Right coin
      4800       R        D                     Center coin
      4800       R           D                  Left coin
      4800       R              D               1 player start
      4800       R                 D            2 player start
      4800       R                    D         2nd player left fire(cocktail)
      4800       R                       D      2nd player center fire  "
      4800       R                          D   2nd player right fire   "
      ---------+-----+------------------------+------------------------
      4800       R                 D  D  D  D   Horiz trackball displacement
						                                    if ctrld=high.
      4800       R     D  D  D  D               Vert trackball displacement
						                                    if ctrld=high.
      ---------+-----+------------------------+------------------------
      4800       W     D                        Unused ??
      4800       W        D                     screen flip
      4800       W           D                  left coin counter
      4800       W              D               center coin counter
      4800       W                 D            right coin counter
      4800       W                    D         2 player start LED.
      4800       W                       D      1 player start LED.
      4800       W                          D   CTRLD, 0=read switches,
						                                    1= read trackball.
      ---------+-----+------------------------+------------------------
      4900       R     D                        VBLANK read
      4900       R        D                     Self test switch input.
      4900       R           D                  SLAM switch input.
      4900       R              D               Horiz trackball direction input.
      4900       R                 D            Vert trackball direction input.
      4900       R                    D         1st player left fire.
      4900       R                       D      1st player center fire.
      4900       R                          D   1st player right fire.
      ---------+-----+------------------------+------------------------
      4A00       R     D  D  D  D  D  D  D  D   Pricing Option switches.
      4B00-4B07  W                 D  D  D  D   Color RAM.
      4C00       W                              Watchdog.
      4D00       W                              Interrupt acknowledge.
      ---------+-----+------------------------+------------------------
      5000-7FFF  R     D  D  D  D  D  D  D  D   Program.
      ---------+-----+------------------------+------------------------




      MISSILE COMMAND SWITCH SETTINGS (Atari, 1980)
---------------------------------------------


GAME OPTIONS:
(8-position switch at R8)

1   2   3   4   5   6   7   8   Meaning
-------------------------------------------------------------------------
Off Off                         Game starts with 7 cities
On  On                          Game starts with 6 cities
On  Off                         Game starts with 5 cities
Off On                          Game starts with 4 cities
        On                      No bonus credit
        Off                     1 bonus credit for 4 successive coins
            On                  Large trak-ball input
            Off                 Mini Trak-ball input
                On  Off Off     Bonus city every  8000 pts
                On  On  On      Bonus city every 10000 pts
                Off On  On      Bonus city every 12000 pts
                On  Off On      Bonus city every 14000 pts
                Off Off On      Bonus city every 15000 pts
                On  On  Off     Bonus city every 18000 pts
                Off On  Off     Bonus city every 20000 pts
                Off Off Off     No bonus cities
                            On  Upright
                            Off Cocktail



PRICING OPTIONS:
(8-position switch at R10)

1   2   3   4   5   6   7   8   Meaning
-------------------------------------------------------------------------
On  On                          1 coin 1 play
Off On                          Free play
On Off                          2 coins 1 play
Off Off                         1 coin 2 plays
        On  On                  Right coin mech * 1
        Off On                  Right coin mech * 4
        On  Off                 Right coin mech * 5
        Off Off                 Right coin mech * 6
                On              Center coin mech * 1
                Off             Center coin mech * 2
                    On  On      English
                    Off On      French
                    On  Off     German
                    Off Off     Spanish
                            On  ( Unused )
                            Off ( Unused )


******************************************************************************************/



#include "driver.h"
#include "sndhrdw/pokyintf.h"

int missile_init_machine(const char *gamename);
int  missile_r(int offset);
void missile_w(int offset, int data);

int missile_trakball_r(int data);

int  missile_vh_start(void);
void missile_vh_stop(void);
void missile_vh_update(struct osd_bitmap *bitmap);





static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xFFFF, missile_r },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xFFFF, missile_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{ /* IN0  - 4800, all inverted */
	/*
	 80 = right coin
	 40 = center coin
	 20 = left coin
	 10 = 1 player start
	 08 = 2 player start
	 04 = 2nd player left fire (cocktail)
	 02 = 2nd player center fire (cocktail)
	 01 = 2nd player right fire (cocktail)
	 */
		0xFF,
		{OSD_KEY_D, OSD_KEY_S, OSD_KEY_A, OSD_KEY_2, OSD_KEY_1, OSD_KEY_3, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0 }
	},


	{ /* IN1 - 4900, all inverted */
	/*
	 80 = vbl read
	 40 = self test
	 20 = SLAM switch
	 10 = horiz trackball input
	 08 = vertical trackball input
	 04 = 1st player left fire
	 02 = 1st player center fire
	 01 = 1st player right fire
	*/

		0x67,
		{ OSD_KEY_D, OSD_KEY_S, OSD_KEY_A, 0, 0, OSD_KEY_F6, OSD_KEY_F5, IPB_VBLANK},
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},


	{	/* IN2  - 4A00 - Pricing Option switches, all inverted */
			0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},


	{	/* IN3  4008 Game Option switches, all inverted */
		0x00,
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
          missile_trakball_r
        },
        {
          Y_AXIS,
          1,
          1.0,
          missile_trakball_r
        },
        { -1 }
};


static struct KEYSet keys[] =
{
        { 1, 2, "LEFT FIRE" },
        { 1, 1, "CENTER FIRE" },
        { 1, 0, "RIGHT FIRE" },
        { -1 }
};





static struct DSW dsw[] =
{
	{ 2, 0x60, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ 3, 0x03, "NUMBER OF CITIES", { "6", "4", "5", "7" } },
/* 	{ 3, 0x08, "TRACKBALL", { "LARGE", "MINI" } }, not very useful */
	{ 3, 0x70, "BONUS AT", { "10000", "12000", "14000", "15000", "18000", "20000", "8000", "0" } },
/* 	{ 3, 0x80, "MODEL", { "UPRIGHT", "COCKTAIL" } }, */
	{ -1 }
};





/* Missile Command has only minimal character mapped graphics, this definition is here */
/* mainly for the dip switch menu */
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	50,	/* 50 characters */
	1,	/* 1 bit per pixel */
	{ 0 },

	{ 0, 1, 2, 3, 4, 5, 6, 7 },		/* characters are upside down */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8														/* every char takes 8 consecutive bytes */
};




static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x731A, &charlayout,     0, 0x10 },
	{ -1 } /* end of array */
};




static unsigned char palette[] =
{
	0, 0, 0,					/* extra black for Macs */
	0xFF,0xFF,0xFF,   /* white   */
	0xFF,0xFF,0x00,   /* yellow  */
	0xFF,0x00,0xFF,   /* magenta  */
	0xFF,0x00,0x00,   /* red    */
	0x00,0xFF,0xFF,   /* cyan    */
	0x00,0xFF,0x00,   /* green   */
	0x00,0x00,0xFF,   /* blue  */
	0x00,0x00,0x00    /* black */
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
			interrupt, 1
		}
	},
	60,
	missile_init_machine,

	/* video hardware */
	256, 231, { 0, 255, 0, 230 },
	gfxdecodeinfo,
	sizeof(palette)/3, 0,
	0,
	0,
	missile_vh_start,
	missile_vh_stop,
	missile_vh_update,

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

ROM_START( missile_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "035820.02", 0x5000, 0x0800, 0x899d091b )
	ROM_LOAD( "035821.02", 0x5800, 0x0800, 0x25543e0a )
	ROM_LOAD( "035822.02", 0x6000, 0x0800, 0x8067194f )
	ROM_LOAD( "035823.02", 0x6800, 0x0800, 0xfc0f6b13 )
	ROM_LOAD( "035824.02", 0x7000, 0x0800, 0xa3e9d74d )
	ROM_LOAD( "035825.02", 0x7800, 0x0800, 0x6050ea56 )
	ROM_RELOAD(            0xF800, 0x0800 )		/* for interrupt vectors  */
ROM_END



static int hiload(const char *name)
{
	FILE *f;
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x002C],"\x47\x4A\x4C", 3) == 0 &&
			memcmp(&RAM[0x0044],"\x50\x69\x00", 3) == 0){

		if ((f = fopen(name,"rb")) != 0){
			fread(&RAM[0x002C],1,6*8,f);
			fclose(f);
		}
		return 1;
	}else
		return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0){
		fwrite(&RAM[0x002C],1,6*8,f);
		fclose(f);
	}
}



struct GameDriver missile_driver =
{
	"Missile Command",
	"missile",
	"RAY GIARRATANA",
	&machine_driver,

	missile_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, 0,

	8*13, 8*12,

	hiload, hisave
};
