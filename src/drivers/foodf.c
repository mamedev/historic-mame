/***************************************************************************

	Atari Food Fight hardware

	driver by Aaron Giles

	Games supported:
		* Food Fight

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	Function                           Address        R/W  DATA
	-------------------------------------------------------------
	Program ROM                        000000-00FFFF  R    D0-D15
	Program RAM                        014000-01BFFF  R/W  D0-D15
	Motion Object RAM                  01C000-01CFFF  R/W  D0-D15

	Motion Objects:
	  Vertical Position                xxxx00              D0-D7
	  Horizontal Position              xxxx00              D8-D15
	  Picture                          xxxx10              D0-D7
	  Color                            xxxx10              D8-D13
	  VFlip                            xxxx10              D14
	  HFlip                            xxxx10              D15

	Playfield                          800000-8007FF  R/W  D0-D15
	  Picture                          xxxxx0              D0-D7+D15
	  Color                            xxxxx0              D8-D13

	NVRAM                              900000-9001FF  R/W  D0-D3
	Analog In                          940000-940007  R    D0-D7
	Analog Out                         944000-944007  W

	Coin 1 (Digital In)                948000         R    D0
	Coin 2                                            R    D1
	Start 1                                           R    D2
	Start 2                                           R    D3
	Coin Aux                                          R    D4
	Throw 1                                           R    D5
	Throw 2                                           R    D6
	Test                                              R    D7

	PFFlip                             948000         W    D0
	Update                                            W    D1
	INT3RST                                           W    D2
	INT4RST                                           W    D3
	LED 1                                             W    D4
	LED 2                                             W    D5
	COUNTERL                                          W    D6
	COUNTERR                                          W    D7

	Color RAM                          950000-9503FF  W    D0-D7
	Recall                             954000         W
	Watchdog                           958000         W
	Audio 1                            A40000-A4001F  R/W  D0-D7
	Audio 0                            A80000-A8001F  R/W  D0-D7
	Audio 2                            AC0000-AC001F  R/W  D0-D7

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

WRITE16_HANDLER( foodf_playfieldram_w );
WRITE16_HANDLER( foodf_paletteram_w );

void foodf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 whichport = 0;
static data16_t *nvram;



/*************************************
 *
 *	NVRAM handler
 *
 *************************************/

static void nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, nvram, 512);
	else if (file)
		osd_fread(file, nvram, 512);
	else
		memset(nvram, 0xff, 512);
}



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static void delayed_interrupt(int param)
{
	cpu_cause_interrupt(0, 2);
}


static int interrupt_gen(void)
{
	/* INT 2 once per frame in addition to... */
	if (cpu_getiloops() == 0)
		timer_set(TIME_IN_USEC(100), 0, delayed_interrupt);

	/* INT 1 on the 32V signal */
	return 1;
}



/*************************************
 *
 *	Analog inputs
 *
 *************************************/

static READ16_HANDLER( analog_r )
{
	return readinputport(whichport);
}


static WRITE16_HANDLER( analog_w )
{
	whichport = offset ^ 3;
}



/*************************************
 *
 *	POKEY I/O
 *
 *************************************/

static READ16_HANDLER( pokey1_word_r ) { return pokey1_r(offset); }
static READ16_HANDLER( pokey2_word_r ) { return pokey2_r(offset); }
static READ16_HANDLER( pokey3_word_r ) { return pokey3_r(offset); }

static WRITE16_HANDLER( pokey1_word_w ) { if (ACCESSING_LSB) pokey1_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey2_word_w ) { if (ACCESSING_LSB) pokey2_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey3_word_w ) { if (ACCESSING_LSB) pokey3_w(offset, data & 0xff); }



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x014000, 0x01cfff, MRA16_RAM },
	{ 0x800000, 0x8007ff, MRA16_RAM },
	{ 0x900000, 0x9001ff, MRA16_RAM },
	{ 0x940000, 0x940007, analog_r },
	{ 0x948000, 0x948001, input_port_4_word_r },
	{ 0x94c000, 0x94c001, MRA16_NOP }, /* Used from PC 0x776E */
	{ 0x958000, 0x958001, MRA16_NOP },
	{ 0xa40000, 0xa4001f, pokey1_word_r },
	{ 0xa80000, 0xa8001f, pokey2_word_r },
	{ 0xac0000, 0xac001f, pokey3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x014000, 0x01bfff, MWA16_RAM },
	{ 0x01c000, 0x01cfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x8007ff, foodf_playfieldram_w, &videoram16, &videoram_size },
	{ 0x900000, 0x9001ff, MWA16_RAM, &nvram },
	{ 0x944000, 0x944007, analog_w },
	{ 0x948000, 0x948001, MWA16_NOP },
	{ 0x950000, 0x9501ff, foodf_paletteram_w, &paletteram16 },
	{ 0x954000, 0x954001, MWA16_NOP },
	{ 0x958000, 0x958001, MWA16_NOP },
	{ 0xa40000, 0xa4001f, pokey1_word_w },
	{ 0xa80000, 0xa8001f, pokey2_word_w },
	{ 0xac0000, 0xac001f, pokey3_word_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( foodf )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_REVERSE, 100, 10, 0, 255 )

	PORT_START	/* IN1 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 10, 0, 255 )

	PORT_START	/* IN2 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_REVERSE, 100, 10, 0, 255 )

	PORT_START	/* IN3 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 10, 0, 255 )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16
};


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), 0 },
	{ 8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 64 },	/* characters 8x8 */
	{ REGION_GFX2, 0, &spritelayout, 0, 64 },	/* sprites & playfield */
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	3,	/* 3 chips */
	600000,	/* .6 MHz */
	{ 33, 33, 33 },
	/* The 8 pot handlers */
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static const struct MachineDriver machine_driver_foodf =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			6000000,	/* 6 MHz */
			readmem,writemem,0,0,
			interrupt_gen,4
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	foodf_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	},

	nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( foodf )
	ROM_REGION( 0x10000, REGION_CPU1 )	/* 64k for 68000 code */
	ROM_LOAD_EVEN( "foodf.9c",     0x00000, 0x02000, 0xef92dc5c )
	ROM_LOAD_ODD ( "foodf.8c",     0x00000, 0x02000, 0xdfc3d5a8 )
	ROM_LOAD_EVEN( "foodf.9d",     0x04000, 0x02000, 0xea596480 )
	ROM_LOAD_ODD ( "foodf.8d",     0x04000, 0x02000, 0x64b93076 )
	ROM_LOAD_EVEN( "foodf.9e",     0x08000, 0x02000, 0x95159a3e )
	ROM_LOAD_ODD ( "foodf.8e",     0x08000, 0x02000, 0xe6cff1b1 )
	ROM_LOAD_EVEN( "foodf.9f",     0x0c000, 0x02000, 0x608690c9 )
	ROM_LOAD_ODD ( "foodf.8f",     0x0c000, 0x02000, 0x17828dbb )

	ROM_REGION( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "foodf.6lm",    0x0000, 0x2000, 0xc13c90eb )

	ROM_REGION( 0x4000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "foodf.4d",     0x0000, 0x2000, 0x8870e3d6 )
	ROM_LOAD( "foodf.4e",     0x2000, 0x2000, 0x84372edf )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1982, foodf, 0, foodf, foodf, 0, ROT0, "Atari", "Food Fight", GAME_NO_COCKTAIL )
