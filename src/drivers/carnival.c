/***************************************************************************

Carnival memory map (preliminary)

0000-3fff ROM mirror image (not used, apart from startup and NMI)
4000-7fff ROM
e000-e3ff Video RAM + other
e400-e7ff RAM
e800-efff Character RAM

I/O ports:
read:
00        IN0
          bit 4 = ?

01        IN1
          bit 3 = vblank
          bit 4 = LEFT
          bit 5 = RIGHT

02        IN2
          bit 4 = START 1
          bit 5 = FIRE

03        IN3
          bit 3 = COIN (must reset the CPU to make the game acknowledge it)
          bit 5 = START 2

write:
01		  bit 0 = fire gun
 		  bit 1 = hit object     	* See Port 2
          bit 2 = duck 1
          bit 3 = duck 2
          bit 4 = duck 3
          bit 5 = hit pipe
          bit 6 = bonus
          bit 7 = hit background

02        bit 2 = Switch effect for hit object - Bear
          bit 3 = Music On/Off
          bit 4 = ? (may be used as signal to sound processor)
          bit 5 = Ranking (not implemented)

08        ?

40        This should be a port to select the palette bank, however it is
          never accessed by Carnival.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


int carnival_IN1_r(int offset);
int carnival_interrupt(void);

extern unsigned char *carnival_characterram;
void carnival_characterram_w(int offset,int data);
void carnival_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void carnival_vh_screenrefresh(struct osd_bitmap *bitmap);

void carnival_sh_port1_w(int offset, int data);			/* MJC */
void carnival_sh_port2_w(int offset, int data);			/* MJC */

void carnival_colour_bank_w(int offset, int data);		/* MJC */

static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, carnival_characterram_w, &carnival_characterram },
	{ 0x4000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ -1 }	/* end of table */
};


static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, carnival_sh_port1_w },						/* MJC */
  	{ 0x02, 0x02, carnival_sh_port2_w },						/* MJC */
    { 0x40, 0x40, carnival_colour_bank_w },						/* MJC */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START ( input_ports )

		PORT_START	/* Dipswitch + Joystick? */
        PORT_DIPNAME ( 0x04, 0x00, "Bit 4", IP_KEY_NONE )
        PORT_DIPSETTING(     0x00, "0" )
        PORT_DIPSETTING(     0x04, "1" )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
        PORT_BIT( 0xCB, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START /* IN1 */
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

        PORT_START /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )

        PORT_START /* IN3 */
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
				"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 18 )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE,
				"Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xe800, &charlayout, 0, 32 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};



static unsigned char carnival_color_prom[] =
{
	/* U49: palette */
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60
};



static struct Samplesinterface samples_interface =
{
	8	/* 8 channels */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1933560,	/* 1.93356 Mhz ???? */
			0,
			readmem,writemem,readport,writeport,
			carnival_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	64,32*2,
	carnival_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	carnival_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( carnival_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "651u33.cpu", 0x0000, 0x0400, 0x661fa2ef )
	ROM_RELOAD(             0x4000, 0x0400 )
	ROM_LOAD( "652u32.cpu", 0x4400, 0x0400, 0x5172c958 )
	ROM_LOAD( "653u31.cpu", 0x4800, 0x0400, 0x3771ff13 )
	ROM_LOAD( "654u30.cpu", 0x4c00, 0x0400, 0x6b0caeb6 )
	ROM_LOAD( "655u29.cpu", 0x5000, 0x0400, 0x15283224 )
	ROM_LOAD( "656u28.cpu", 0x5400, 0x0400, 0xec2fd83b )
	ROM_LOAD( "657u27.cpu", 0x5800, 0x0400, 0x9a30851e )
	ROM_LOAD( "658u26.cpu", 0x5c00, 0x0400, 0x914e4bf2 )
	ROM_LOAD( "659u8.cpu",  0x6000, 0x0400, 0x22c6547c )
	ROM_LOAD( "660u7.cpu",  0x6400, 0x0400, 0xba76241e )
	ROM_LOAD( "661u6.cpu",  0x6800, 0x0400, 0x3bab3df7 )
	ROM_LOAD( "662u5.cpu",  0x6c00, 0x0400, 0xc315eb83 )
	ROM_LOAD( "663u4.cpu",  0x7000, 0x0400, 0x434d30f7 )
	ROM_LOAD( "664u3.cpu",  0x7400, 0x0400, 0x75f1b261 )
	ROM_LOAD( "665u2.cpu",  0x7800, 0x0400, 0xecdd5165 )
	ROM_LOAD( "666u1.cpu",  0x7c00, 0x0400, 0x24809182 )
ROM_END



static const char *carnival_sample_names[] =					/* MJC */
{
	"C1.SAM",	/* Fire Gun 1 */
	"C2.SAM",	/* Hit Target */
	"C3.SAM",	/* Duck 1 */
	"C4.SAM",	/* Duck 2 */
	"C5.SAM",	/* Duck 3 */
	"C6.SAM",	/* Hit Pipe */
	"C7.SAM",	/* BONUS */
	"C8.SAM",	/* Hit bonus box */
    "C9.SAM",	/* Fire Gun 2 */
    "C10.SAM",	/* Hit Bear */
	0       	/* end of array */
};



static int carnival_hiload(void)
{
	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xE397],"\x00\x00\x00",3) == 0) &&
		(memcmp(&RAM[0xE5A2],"   ",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Read the scores */
            osd_fread(f,&RAM[0xE397],2*30);
			/* Read the initials */
            osd_fread(f,&RAM[0xE5A2],9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void carnival_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the scores */
        osd_fwrite(f,&RAM[0xE397],2*30);
		/* Save the initials */
        osd_fwrite(f,&RAM[0xE5A2],9);
		osd_fclose(f);
	}

}

struct GameDriver carnival_driver =
{
	"Carnival",
	"carnival",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\nMIKE BALFOUR",
	&machine_driver,

	carnival_rom,
	0, 0,
	carnival_sample_names,										/* MJC */
	0,	/* sound_prom */

	input_ports,

	carnival_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	carnival_hiload, carnival_hisave
};

/****************************************************************************
 * Pulsar
 ****************************************************************************/

ROM_START( pulsar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "790.u33", 0x0000, 0x0400, 0xbc154259 )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "791.u32", 0x4400, 0x0400, 0x8bc8bf1a )
	ROM_LOAD( "792.u31", 0x4800, 0x0400, 0x5b6fc8a9 )
	ROM_LOAD( "793.u30", 0x4c00, 0x0400, 0xb3b44342 )
	ROM_LOAD( "794.u29", 0x5000, 0x0400, 0xbe5d6fd1 )
	ROM_LOAD( "795.u28", 0x5400, 0x0400, 0x0b53ae73 )
	ROM_LOAD( "796.u27", 0x5800, 0x0400, 0xc3cfc4c5 )
	ROM_LOAD( "797.u26", 0x5c00, 0x0400, 0x9efb41b3 )
	ROM_LOAD( "798.u8",  0x6000, 0x0400, 0xcdae0cc0 )
	ROM_LOAD( "799.u7",  0x6400, 0x0400, 0x9617dc67 )
	ROM_LOAD( "800.u6",  0x6800, 0x0400, 0xb1ed255f )
	ROM_LOAD( "801.u5",  0x6C00, 0x0400, 0x140f4fff )
	ROM_LOAD( "802.u4",  0x7000, 0x0400, 0x181f535d )
	ROM_LOAD( "803.u3",  0x7400, 0x0400, 0x1ca50c4d )
	ROM_LOAD( "804.u2",  0x7800, 0x0400, 0x75482596 )
	ROM_LOAD( "805.u1",  0x7C00, 0x0400, 0x4215bd79 )
ROM_END

static unsigned char pulsar_color_prom[] =
{
	/* U49: palette */
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91
};

struct GameDriver pulsar_driver =
{
	"Pulsar",
	"pulsar",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\n",
	&machine_driver,

	pulsar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	pulsar_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/****************************************************************************
 * Invinco / Head On 2
 ****************************************************************************/

ROM_START( invhd2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "271b.u33", 0x0000, 0x0400, 0x1557248b )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "272b.u32", 0x4400, 0x0400, 0xa290a8dc )
	ROM_LOAD( "273b.u31", 0x4800, 0x0400, 0x0f5df4cf )
	ROM_LOAD( "274b.u30", 0x4c00, 0x0400, 0xfc5b979d )
	ROM_LOAD( "275b.u29", 0x5000, 0x0400, 0x6dd0c104 )
	ROM_LOAD( "276b.u28", 0x5400, 0x0400, 0x1e1eadf0 )
	ROM_LOAD( "277b.u27", 0x5800, 0x0400, 0xad1d5307 )
	ROM_LOAD( "278b.u26", 0x5c00, 0x0400, 0xe409f8d3 )
	ROM_LOAD( "279b.u8",  0x6000, 0x0400, 0x436f5afb )
	ROM_LOAD( "280b.u7",  0x6400, 0x0400, 0x71d90e01 )
	ROM_LOAD( "281b.u6",  0x6800, 0x0400, 0x37fa0188 )
	ROM_LOAD( "282b.u5",  0x6c00, 0x0400, 0xb83213f2 )
	ROM_LOAD( "283b.u4",  0x7000, 0x0400, 0xff7803c4 )
	ROM_LOAD( "284b.u3",  0x7400, 0x0400, 0x53f1cd8b )
	ROM_LOAD( "285b.u2",  0x7800, 0x0400, 0x58a0fe0e )
	ROM_LOAD( "286b.u1",  0x7C00, 0x0400, 0x8460e06c )
ROM_END

static unsigned char inv_hd2_color_prom[] =
{
	/* U49: palette */
	/* note: changed 0x01 to 0x21 to avoid black on black. Maybe the PROM was not read correctly */
	0xC1,0x81,0x41,0xC1,0xC1,0xC1,0xC1,0xC1,
	0x81,0xC1,0x21,0xC1,0x41,0x81,0x81,0x81,
	0xC1,0x81,0x21,0x41,0xC1,0x81,0x41,0x81,
	0xC1,0xC1,0xC1,0xC1,0xC1,0xC1,0xC1,0xC1,
};

struct GameDriver invho2_driver =
{
	"Invinco / Head On 2",
	"invho2",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\n",
	&machine_driver,

	invhd2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	inv_hd2_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/****************************************************************************
 * Sega Space Attack
 ****************************************************************************/

ROM_START( sspaceat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "139.u27", 0x0000, 0x0400, 0xc3bde565 )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "140.u26", 0x4400, 0x0400, 0xf7953eeb )
	ROM_LOAD( "141.u25", 0x4800, 0x0400, 0xac45484b )
	ROM_LOAD( "142.u24", 0x4c00, 0x0400, 0xc3bdcf07 )
	ROM_LOAD( "143.u23", 0x5000, 0x0400, 0x71d29b2c )
	ROM_LOAD( "144.u22", 0x5400, 0x0400, 0x479e9940 )
	ROM_LOAD( "145.u21", 0x5800, 0x0400, 0x47dbfeef )
	ROM_LOAD( "146.u20", 0x5c00, 0x0400, 0xdd530af1 )
ROM_END

static struct IOReadPort sspaceat_readport[] =
{
   	{ 0x04, 0x04, input_port_0_r },
  	{ 0x01, 0x01, input_port_1_r },
   	{ 0x08, 0x08, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct MachineDriver sspaceattack_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1933560,	/* 1.93356 Mhz ???? */
			0,
			readmem,writemem,sspaceat_readport,writeport,
			carnival_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	64,32*2,
	carnival_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	carnival_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



INPUT_PORTS_START( sspaceat_input_ports )
		PORT_START	/* Dip Switch */
        PORT_DIPNAME( 0x0e, 0x0e, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0e, "3" )
        PORT_DIPSETTING(    0x0c, "4" )
        PORT_DIPSETTING(    0x0a, "5" )
        PORT_DIPSETTING(    0x06, "6" )
/*
        PORT_DIPSETTING(    0x00, "4" )
        PORT_DIPSETTING(    0x04, "4" )
        PORT_DIPSETTING(    0x08, "4" )
        PORT_DIPSETTING(    0x02, "5" )
*/

        PORT_START /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

        PORT_START
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
				"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 18 )
INPUT_PORTS_END



static unsigned char sspaceat_color_prom[] =
{
	/* Guessed palette! */
	0x60,0x40,0x40,0x20,0xC0,0x80,0xA0,0xE0,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

struct GameDriver sspaceat_driver =
{
	"Sega Space Attack",
	"sspaceat",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\n",
	&sspaceattack_machine_driver,

	sspaceat_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sspaceat_input_ports,

	sspaceat_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/****************************************************************************
 * Invinco
 ****************************************************************************/

ROM_START( invinco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "310a.u27", 0x0000, 0x0400, 0x0982135c )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "311a.u26", 0x4400, 0x0400, 0x17d6221c )
	ROM_LOAD( "312a.u25", 0x4800, 0x0400, 0x0280d0e8 )
	ROM_LOAD( "313a.u24", 0x4c00, 0x0400, 0xe9d6530e )
	ROM_LOAD( "314a.u23", 0x5000, 0x0400, 0x31f350ab )
	ROM_LOAD( "315a.u22", 0x5400, 0x0400, 0x0c6fffb1 )
	ROM_LOAD( "316a.u21", 0x5800, 0x0400, 0x9b5dfb21 )
	ROM_LOAD( "317a.u20", 0x5c00, 0x0400, 0x6b868418 )
	ROM_LOAD( "318a.uxx", 0x6000, 0x0400, 0x8c7227ea )
ROM_END

INPUT_PORTS_START( invinco_input_ports )
	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0xFc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 18 )
INPUT_PORTS_END

static unsigned char invinco_color_prom[] =
{
	/* Selected palette from Invinco/Head On 2 colour prom */

	0xC1,0x81,0x21,0x41,0xC1,0x81,0x41,0x81,
	0xC1,0x81,0x21,0x41,0xC1,0x81,0x41,0x81,
	0xC1,0x81,0x21,0x41,0xC1,0x81,0x41,0x81,
	0xC1,0x81,0x21,0x41,0xC1,0x81,0x41,0x81
};

struct GameDriver invinco_driver =
{
	"Invinco",
	"invinco",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\n",
	&sspaceattack_machine_driver,

	invinco_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	invinco_input_ports,

	invinco_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};
