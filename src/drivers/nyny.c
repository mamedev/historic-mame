/****

New York, New York (c) 1980 Sigma


memory map main cpu (m6809)

fedcba98
--------
000xxxxx  we1   $0000 8k (bitmap)
100xxxxx  we1   $8000 8k (ram)

010xxxxx  we2   $4000 8k (bitmap)
110xxxxx  we2   $C000 8k (ram)

001xxxxx  we3   $2000 16k x 3bits (colour)

011xxxxx  we4   $6000 16k x 3bits (colour)

10100000  SRAM  $A000
10100001  CRTC  $A100
10100010  PIA   $A200
10100011  SOUND $A300 one latch for read one for write same address

10101xxx  ROM7  $A800
10110xxx  ROM6  $B000
10111xxx  ROM5  $B800

11100xxx  ROM4  $E000
11101xxx  ROM3  $E800
11110xxx  ROM2  $F000
11111xxx  ROM1  $F800

****/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"

void nyny_init_palette(unsigned char *obsolete,unsigned short *game_colortable,const unsigned char *color_prom);
void nyny_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
int nyny_vh_start(void);
void nyny_vh_stop(void);

unsigned char *nyny_videoram ;
unsigned char *nyny_colourram ;


static unsigned char pia1_ca1 = 0 ;
static unsigned char dac_volume = 0 ;
static unsigned char dac_enable = 0 ;

READ_HANDLER( nyny_videoram0_r );
WRITE_HANDLER( nyny_videoram0_w );
READ_HANDLER( nyny_videoram1_r );
WRITE_HANDLER( nyny_videoram1_w );

READ_HANDLER( nyny_colourram0_r );
WRITE_HANDLER( nyny_colourram0_w );
READ_HANDLER( nyny_colourram1_r );
WRITE_HANDLER( nyny_colourram1_w );
WRITE_HANDLER( nyny_flipscreen_w ) ;



static unsigned char *nvram;
static size_t nvram_size;

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file,nvram,nvram_size);
	else
	{
		if (file)
			osd_fread(file,nvram,nvram_size);
		else
			memset(nvram,0x00,nvram_size);
	}
}



int nyny_interrupt(void)
{
	/* this is not accurate */
	/* pia1_ca1 should be toggled by output of LS123 */
	pia1_ca1 ^= 0x80 ;

	/* update for coin irq */
	pia_0_ca1_w(0,input_port_5_r(0)&0x01);
	pia_0_ca2_w(0,input_port_6_r(0)&0x01);

	return 0 ;
}

/***************************************************************************
	6821 PIA handlers
***************************************************************************/

void cpu0_irq(int state)
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE);
}


static READ_HANDLER( pia1_ca1_r )
{
	return pia1_ca1;
}


static WRITE_HANDLER( pia1_porta_w )
{
	/* bits 0-7 control a timer (low 8 bits) - is this for a starfield? */
}

static WRITE_HANDLER( pia1_portb_w )
{
	/* bits 0-3 control a timer (high 4 bits) - is this for a starfield? */
	/* bit 4 enables the starfield? */

	/* bits 5-7 go to the music board */
	soundlatch2_w(0,(data & 0x60) >> 5);
	cpu_set_irq_line(2,M6802_IRQ_LINE,(data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
}

static struct pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, input_port_5_r, 0, input_port_6_r, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ cpu0_irq, 0
};

static struct pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, pia1_ca1_r, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia1_porta_w, pia1_portb_w, nyny_flipscreen_w, 0,
	/*irqs   : A/B             */ 0, 0
};

void nyny_init_machine(void)
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_ALTERNATE_ORDERING, &pia1_intf);
	pia_reset();
}



WRITE_HANDLER( ay8910_porta_w )
{
	/* dac sounds like crap most likely bad implementation */
	dac_volume = data ;
	DAC_1_data_w( 0, dac_enable * dac_volume ) ;
}

WRITE_HANDLER( ay8910_portb_w )
{
	int v = (data & 7) << 5 ;
	DAC_0_data_w( 0, v ) ;

	dac_enable = ( data & 8 ) >> 3 ;
	DAC_1_data_w( 0, dac_enable * dac_volume ) ;
}

WRITE_HANDLER( shared_w_irq )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(1,M6802_IRQ_LINE,HOLD_LINE);
}


static unsigned char snd_w = 0;

READ_HANDLER( snd_answer_r )
{
	return snd_w;
}

WRITE_HANDLER( snd_answer_w )
{
	snd_w = data;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, nyny_videoram0_r }, // WE1 8k
	{ 0x2000, 0x3fff, nyny_colourram0_r }, // WE3
	{ 0x4000, 0x5fff, nyny_videoram1_r }, // WE2
	{ 0x6000, 0x7fff, nyny_colourram1_r }, // WE4
	{ 0x8000, 0x9fff, MRA_RAM }, // WE1 8k
	{ 0xa000, 0xa007, MRA_RAM }, // SRAM
	{ 0xa204, 0xa207, pia_0_r },
	{ 0xa208, 0xa20b, pia_1_r },
	{ 0xa300, 0xa300, snd_answer_r },
	{ 0xa800, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM }, // WE2
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, nyny_videoram0_w }, // WE1
	{ 0x2000, 0x3fff, nyny_colourram0_w }, // WE3
	{ 0x4000, 0x5fff, nyny_videoram1_w }, // WE2
	{ 0x6000, 0x7fff, nyny_colourram1_w }, // WE4
	{ 0x8000, 0x9fff, MWA_RAM }, // WE1
	{ 0xa000, 0xa007, MWA_RAM, &nvram, &nvram_size }, // SRAM (coin counter, shown when holding F2)
	{ 0xa204, 0xa207, pia_0_w },
	{ 0xa208, 0xa20b, pia_1_w },
	{ 0xa300, 0xa300, shared_w_irq },
	{ 0xa100, 0xa100, crtc6845_address_w },
	{ 0xa101, 0xa101, crtc6845_register_w },
	{ 0xa800, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM }, // WE2
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x9001, 0x9001, soundlatch_r },
	{ 0xa000, 0xa001, input_port_4_r },
	{ 0xb000, 0xb000, AY8910_read_port_0_r },
	{ 0xb002, 0xb002, AY8910_read_port_1_r },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x9001, 0x9001, snd_answer_w },
	{ 0xb000, 0xb000, AY8910_write_port_0_w },
	{ 0xb001, 0xb001, AY8910_control_port_0_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xb003, 0xb003, AY8910_control_port_1_w },
	{ 0xd000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound2_readmem )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x9000, 0x9000, soundlatch2_r },
	{ 0xa000, 0xa000, AY8910_read_port_2_r },
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound2_writemem )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_write_port_2_w },
	{ 0xa001, 0xa001, AY8910_control_port_2_w },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( nyny )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) 	/* PIA0 PA0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* PIA0 PA1 */
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	/* PIA0 PA2 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* PIA0 PA3 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL ) /* PIA0 PA4 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )	/* PIA0 PA5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )	/* PIA0 PA6 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL ) /* PIA0 PB0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL ) /* PIA0 PB1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )	/* PIA0 PB2 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )	/* PIA0 PB3 */

	PORT_START	/* SW1 - port 2*/
	PORT_DIPNAME( 0x03, 0x03, "Bombs from UFO (scr 3+) " )
	PORT_DIPSETTING(	0x03, "9" )
	PORT_DIPSETTING(	0x02, "12" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Bombs from UFO (scr 1-2) " )
	PORT_DIPSETTING(	0x04, "6" )
	PORT_DIPSETTING(	0x00, "9" )
	PORT_DIPNAME( 0x80, 0x80, "Voice Volume " )
	PORT_DIPSETTING(	0x80, "High" )
	PORT_DIPSETTING(	0x00, "Low" )

	PORT_START	/* SW2 - port 3*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "No Replays" )
	PORT_DIPSETTING(	0x10, "5000 Points" )
	PORT_DIPSETTING(	0x00, "10000 Points" )
	PORT_DIPSETTING(	0x08, "15000 Points" )
	PORT_DIPNAME( 0x40, 0x40, "Extra Missile Base")
	PORT_DIPSETTING(	0x00, "3000 Points" )
	PORT_DIPSETTING(	0x40, "5000 Points" )
	PORT_DIPNAME( 0x80, 0x80, "Extra Missile Mode" )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPSETTING(	0x00, "No Extra Base" )

	PORT_START	/* SW3 - port 4*/
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1c, 0x00, "Vertical Screen Position" )
	PORT_DIPSETTING(	0x00, "Neutral" )
	PORT_DIPSETTING(	0x04, "+1" )
	PORT_DIPSETTING(	0x08, "+2" )
	PORT_DIPSETTING(	0x0c, "+3" )
	PORT_DIPSETTING(	0x1c, "-1" )
	PORT_DIPSETTING(	0x18, "-2" )
	PORT_DIPSETTING(	0x14, "-3" )
	PORT_DIPNAME( 0xe0, 0x00, "Horizontal Screen Position" )
	PORT_DIPSETTING(	0x00, "Neutral" )
	PORT_DIPSETTING(	0x60, "+1" )
	PORT_DIPSETTING(	0x40, "+2" )
	PORT_DIPSETTING(	0x20, "+3" )
	PORT_DIPSETTING(	0xe0, "-1" )
	PORT_DIPSETTING(	0xc0, "-2" )
	PORT_DIPSETTING(	0xa0, "-3" )

	PORT_START  /* Connected to PIA1 CA1 input - port 5 */
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START  /* Connected to PIA1 CA2 input - port 6 */
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_SERVICE1 )
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1000000,	/* 1 MHz */
	{ 25, 25, 3 },
	{ 0, input_port_2_r, 0 },
	{ 0, input_port_3_r, 0 },
	{ ay8910_porta_w, 0 },
	{ ay8910_portb_w, 0 }
};

static struct DACinterface dac_interface =
{
	2,
	{ 25, 25 }
};



static struct MachineDriver machine_driver_nyny =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1400000,	/* 1.40 MHz */
			readmem, writemem, 0, 0,
			nyny_interrupt,2 /* game doesn't use video based irqs it's polling based */
		},
		{
			CPU_M6802,
			4000000/4,	/* 1 MHz */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt,0
		},
		{
			CPU_M6802,
			4000000/4,	/* 1 MHz */
			sound2_readmem, sound2_writemem, 0, 0,
			ignore_interrupt,0
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices per frame  */
	nyny_init_machine,
	/* video hardware */
	256, 256, { 0, 255, 4, 251 },	/* visible_area - just a guess */
	0,
	8, 0,
	nyny_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	nyny_vh_start,
	nyny_vh_stop,
	nyny_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	},

	nvram_handler
};

/***************************************************************************
  Game driver(s)
***************************************************************************/


ROM_START( nyny )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "nyny01s.100",  0xa800, 0x800, 0xa2b76eca )
	ROM_LOAD( "nyny02s.099",  0xb000, 0x800, 0xef2d4dae )
	ROM_LOAD( "nyny03s.098",  0xb800, 0x800, 0x2734c229 )
	ROM_LOAD( "nyny04s.097",  0xe000, 0x800, 0xbd94087f )
	ROM_LOAD( "nyny05s.096",  0xe800, 0x800, 0x248b22c4 )
	ROM_LOAD( "nyny06s.095",  0xf000, 0x800, 0x8c073052 )
	ROM_LOAD( "nyny07s.094",  0xf800, 0x800, 0xd49d7429 )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "nyny08.093",   0xd000, 0x800, 0x19ddb6c3 )
	ROM_RELOAD(               0xd800, 0x800 ) /*  needed high bit not wired */
	ROM_LOAD( "nyny09.092",   0xe000, 0x800, 0xa359c6f1 )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "nyny10.091",   0xf000, 0x800, 0xa72a70fa )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	ROM_LOAD( "nyny11.snd",   0xf800, 0x800, 0x650450fc )
ROM_END

ROM_START( nynyg )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "gny1.cpu",     0xa800, 0x800, 0xfb5b8f17 )
	ROM_LOAD( "gny2.cpu",     0xb000, 0x800, 0xd248dd93 )
	ROM_LOAD( "gny3.cpu",     0xb800, 0x800, 0x223a9d09 )
	ROM_LOAD( "gny4.cpu",     0xe000, 0x800, 0x7964ec1f )
	ROM_LOAD( "gny5.cpu",     0xe800, 0x800, 0x4799dcfc )
	ROM_LOAD( "gny6.cpu",     0xf000, 0x800, 0x4839d4d2 )
	ROM_LOAD( "gny7.cpu",     0xf800, 0x800, 0xb7564c5b )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "gny8.cpu",     0xd000, 0x800, 0xe0bf7d00 )
	ROM_RELOAD(               0xd800, 0x800 ) /* reload needed high bit not wired */
	ROM_LOAD( "gny9.cpu",     0xe000, 0x800, 0x639bc81a )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "gny10.cpu",    0xf000, 0x800, 0x73764021 )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	/* The original dump of this ROM was bad [FIXED BITS (x1xxxxxx)] */
	/* Since what's left is identical to the Sigma version, I'm assuming it's the same. */
	ROM_LOAD( "nyny11.snd",   0xf800, 0x800, 0x650450fc )
ROM_END

ROM_START( arcadia )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "ar-01",        0xa800, 0x800, 0x7b7e8f27 )
	ROM_LOAD( "ar-02",        0xb000, 0x800, 0x81d9e172 )
	ROM_LOAD( "ar-03",        0xb800, 0x800, 0x2c5feb05 )
	ROM_LOAD( "ar-04",        0xe000, 0x800, 0x66fcbd7f )
	ROM_LOAD( "ar-05",        0xe800, 0x800, 0xb2320e20 )
	ROM_LOAD( "ar-06",        0xf000, 0x800, 0x27b79cc0 )
	ROM_LOAD( "ar-07",        0xf800, 0x800, 0xbe77a477 )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "ar-08",        0xd000, 0x800, 0x38569b25 )
	ROM_RELOAD(               0xd800, 0x800 ) /*  needed high bit not wired */
	ROM_LOAD( "nyny09.092",   0xe000, 0x800, 0xa359c6f1 )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "nyny10.091",   0xf000, 0x800, 0xa72a70fa )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	ROM_LOAD( "ar-11",        0xf800, 0x800, 0x208f4488 )
ROM_END


GAMEX( 1980, nyny,    0,    nyny, nyny, 0, ROT270, "Sigma Ent. Inc.", "New York New York", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1980, nynyg,   nyny, nyny, nyny, 0, ROT270, "Sigma Ent. Inc. (Gottlieb license)", "New York New York (Gottlieb)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1980, arcadia, nyny, nyny, nyny, 0, ROT270, "Sigma Ent. Inc.", "Waga Seishun no Arcadia", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
