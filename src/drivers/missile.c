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

void missile_init_machine(void);
int  missile_r(int offset);
void missile_w(int offset, int data);

int missile_trakball_r(int data);

int  missile_vh_start(void);
void missile_vh_stop(void);
void missile_vh_update(struct osd_bitmap *bitmap);



static struct POKEYinterface interface =
{
	1,	/* 1 chip */
	FREQ_17_APPROX,	/* 1.7 Mhz */
	255,
	USE_CLIP,  /* EEA was NO_CLIP */
	/* The 8 pot handlers */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ input_port_3_r },
};


int missile_sh_start(void)
{
	return pokey_sh_start (&interface);
}


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



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x18, 0x00, IPT_UNUSED )	/* trackball input, handled in machine/missile.c */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_SERVICE , "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_DIPNAME (0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x03, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x02, "Free Play" )
	PORT_DIPNAME (0x0c, 0x00, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x04, "*4" )
	PORT_DIPSETTING (   0x08, "*5" )
	PORT_DIPSETTING (   0x0c, "*6" )
	PORT_DIPNAME (0x10, 0x00, "Center Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x10, "*2" )
	PORT_DIPNAME (0x60, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "English" )
	PORT_DIPSETTING (   0x20, "French" )
	PORT_DIPSETTING (   0x40, "German" )
	PORT_DIPSETTING (   0x60, "Spanish" )
	PORT_DIPNAME (0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (   0x80, "Off" )
	PORT_DIPSETTING (   0x00, "On" )

	PORT_START	/* IN3 */
	PORT_DIPNAME (0x03, 0x00, "Cities", IP_KEY_NONE )
	PORT_DIPSETTING (   0x02, "4" )
	PORT_DIPSETTING (   0x01, "5" )
	PORT_DIPSETTING (   0x03, "6" )
	PORT_DIPSETTING (   0x00, "7" )
	PORT_DIPNAME (0x04, 0x04, "Bonus Credit for 4 Coins", IP_KEY_NONE )
	PORT_DIPSETTING (   0x04, "No" )
	PORT_DIPSETTING (   0x00, "Yes" )
	PORT_DIPNAME (0x08, 0x08, "Trackball Size", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Large" )
	PORT_DIPSETTING (   0x08, "Mini" )
	PORT_DIPNAME (0x70, 0x00, "Bonus City", IP_KEY_NONE )
	PORT_DIPSETTING (   0x10, "8000" )
	PORT_DIPSETTING (   0x70, "10000" )
	PORT_DIPSETTING (   0x60, "12000" )
	PORT_DIPSETTING (   0x50, "14000" )
	PORT_DIPSETTING (   0x40, "15000" )
	PORT_DIPSETTING (   0x30, "18000" )
	PORT_DIPSETTING (   0x20, "20000" )
	PORT_DIPSETTING (   0x00, "None" )
	PORT_DIPNAME (0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Upright" )
	PORT_DIPSETTING (   0x80, "Cocktail" )

	PORT_START	/* FAKE */
	PORT_ANALOG ( 0x0f, 0x0, IPT_TRACKBALL_Y | IPF_CENTER | IPF_REVERSE, 50, 7, 0, 0)

	PORT_START	/* FAKE */
	PORT_ANALOG ( 0x0f, 0x0, IPT_TRACKBALL_X | IPF_CENTER, 50, 7, 0, 0)
INPUT_PORTS_END



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
			interrupt, 4  /* EEA was 1 */
		}
	},
	60,
	10,
 	missile_init_machine,

	/* video hardware */
	256, 231, { 0, 255, 0, 230 },
	0,
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	missile_vh_start,
	missile_vh_stop,
	missile_vh_update,

	/* sound hardware */
	0,
	missile_sh_start,
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



static int hiload(void)
{
	void *f;
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x002C],"\x47\x4A\x4C", 3) == 0 &&
			memcmp(&RAM[0x0044],"\x50\x69\x00", 3) == 0){

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0){
			osd_fread(f,&RAM[0x002C],6*8);
			osd_fclose(f);
		}
		return 1;
	}else
		return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0){
		osd_fwrite(f,&RAM[0x002C],6*8);
		osd_fclose(f);
	}
}



struct GameDriver missile_driver =
{
	"Missile Command",
	"missile",
	"Ray Giarratana\nMarco Cassili\nEric Anschuetz",  /* EEA */
	&machine_driver,

	missile_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
