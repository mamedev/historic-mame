/******************************************************************************

  'Face' LINDA board
 driver by Paul Priest + David Haywood

*******************************************************************************

 Games on this Hardware

 Magical Cat Adventure (c)1993 Wintechno
 Nostradamus (c)1993 Face

*******************************************************************************

 Hardware Overview:

 Board Name : LINDA5 (Magical Cat) / LINDA25 (Nostradamus)

 Main CPU: 68000-16
 Sound CPU: Z80
 Sound Chip: YMF286-K

 Custom: FACE FX1037 x1
         038 x2 (As in Cave)

*******************************************************************************

Stephh's notes (based on the games M68000 code and some tests) :

1) "mcatadv*'

  - Player 1 Button 3 is only used in the "test" mode :
      * to select "OBJECT ROM CHECK"
      * in "BG ROM", to change the background number

  - Do NOT trust the "NORMAL TESTMODE" for the system inputs !

  - The Japan version has extra GFX/anims and it's harder than the other set.

2) 'nost*'

  - When entering the "test mode", you need to press SERVICE1 to cycle through
    the different screens.

*******************************************************************************

 todo:

 Flip Screen

*******************************************************************************

 trivia:

 Magical Cat Adventure tests for 'MASICAL CAT ADVENTURE' in RAM on start-up
 and will write it there if not found, expecting a reset, great engrish ;-)

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_UPDATE( mcatadv );
VIDEO_START( mcatadv );
VIDEO_EOF( mcatadv );
VIDEO_UPDATE( nost );
VIDEO_START( nost );

WRITE16_HANDLER( mcatadv_videoram1_w );
WRITE16_HANDLER( mcatadv_videoram2_w );
data16_t* mcatadv_videoram1, *mcatadv_videoram2;
data16_t* mcatadv_scroll, *mcatadv_scroll2;
data16_t* mcatadv_vidregs;


/*** Main CPU ***/

static READ16_HANDLER( mcatadv_dsw_r )
{
	return readinputport(2+offset) << 8;
}

static WRITE16_HANDLER( mcat_soundlatch_w )
{
	soundlatch_w(0, data);
	cpu_set_nmi_line(1,PULSE_LINE);
}

static READ16_HANDLER( mcat_wd_r )
{
	watchdog_reset_r(0);
	return 0xc00;
}


static MEMORY_READ16_START( mcatadv_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },

	{ 0x200000, 0x200005, MRA16_RAM },
	{ 0x300000, 0x300005, MRA16_RAM },

	{ 0x180018, 0x18001f, MRA16_NOP }, // ?

	{ 0x400000, 0x401fff, MRA16_RAM }, // Tilemap 0
	{ 0x500000, 0x501fff, MRA16_RAM }, // Tilemap 1

	{ 0x600000, 0x601fff, MRA16_RAM },
	{ 0x602000, 0x602fff, MRA16_RAM },

	{ 0x700000, 0x707fff, MRA16_RAM }, // Sprites
	{ 0x708000, 0x70ffff, MRA16_RAM }, // Tests more than is needed?

	{ 0x800000, 0x800001, input_port_0_word_r },	// Inputs
	{ 0x800002, 0x800003, input_port_1_word_r },	// Inputs
	{ 0xa00000, 0xa00003, mcatadv_dsw_r },		// Dip Switches

	{ 0xb00000, 0xb0000f, MRA16_RAM },

	{ 0xb0001e, 0xb0001f, mcat_wd_r }, // MCAT Only
	{ 0xc00000, 0xc00001, soundlatch2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( mcatadv_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x200005, MWA16_RAM, &mcatadv_scroll },
	{ 0x300000, 0x300005, MWA16_RAM, &mcatadv_scroll2 },

	{ 0x400000, 0x401fff, mcatadv_videoram1_w, &mcatadv_videoram1 }, // Tilemap 0
	{ 0x500000, 0x501fff, mcatadv_videoram2_w, &mcatadv_videoram2 }, // Tilemap 1

	{ 0x600000, 0x601fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },
	{ 0x602000, 0x602fff, MWA16_RAM }, // Bigger than needs to be?

	{ 0x700000, 0x707fff, MWA16_RAM, &spriteram16, &spriteram_size }, // Sprites, two halves for double buffering
	{ 0x708000, 0x70ffff, MWA16_RAM }, // Tests more than is needed?

	{ 0x900000, 0x900001, MWA16_RAM }, //?
	{ 0xb00000, 0xb0000f, MWA16_RAM, &mcatadv_vidregs },
	{ 0xb00018, 0xb00019, watchdog_reset16_w }, // NOST Only
	{ 0xc00000, 0xc00001, mcat_soundlatch_w },
MEMORY_END

/*** Sound ***/

static WRITE_HANDLER ( mcatadv_sound_bw_w )
{
	data8_t *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1,rom + data * 0x4000);
}


static MEMORY_READ_START( mcatadv_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM		},	// ROM
	{ 0x4000, 0xbfff, MRA_BANK1		},	// ROM
	{ 0xc000, 0xdfff, MRA_RAM		},	// RAM
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r		},
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r		},
MEMORY_END

static MEMORY_WRITE_START( mcatadv_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM		},	// ROM
	{ 0x4000, 0xbfff, MWA_ROM		},	// ROM
	{ 0xc000, 0xdfff, MWA_RAM		},	// RAM
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xf000, 0xf000, mcatadv_sound_bw_w },
MEMORY_END

static PORT_READ_START( mcatadv_sound_readport )
	{ 0x80, 0x80, soundlatch_r },
PORT_END

static PORT_WRITE_START( mcatadv_sound_writeport )
	{ 0x80, 0x80, soundlatch2_w },
PORT_END


static MEMORY_READ_START( nost_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM		},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1		},	// ROM
	{ 0xc000, 0xdfff, MRA_RAM		},	// RAM
MEMORY_END

static MEMORY_WRITE_START( nost_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM		},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM		},	// ROM
	{ 0xc000, 0xdfff, MWA_RAM		},	// RAM
MEMORY_END

static PORT_READ_START( nost_sound_readport )
	{ 0x04, 0x05, YM2610_status_port_0_A_r },
	{ 0x06, 0x07, YM2610_status_port_0_B_r },
	{ 0x80, 0x80, soundlatch_r },
PORT_END

static PORT_WRITE_START( nost_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x40, 0x40, mcatadv_sound_bw_w },
	{ 0x80, 0x80, soundlatch2_w },
PORT_END

/*** Inputs ***/

INPUT_PORTS_START( mcatadv )
	PORT_START	// IN0
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )	// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )	// "Jump"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )	// See notes
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )	// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )	// "Jump"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// DSW1
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	*/
	PORT_START	// DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Energy" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, "Upright 1 Player" )
	PORT_DIPSETTING(    0xc0, "Upright 2 Players" )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
//	PORT_DIPSETTING(    0x00, "Upright 2 Players" )		// duplicated setting (NEVER tested)
INPUT_PORTS_END

INPUT_PORTS_START( nost )
	PORT_START	// IN0
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 2 in "test mode"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 3 in "test mode"
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )					// "test" 3 in "test mode"
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN )					// Must be LOW or startup freezes !
	PORT_BIT( 0xf400, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 2 in "test mode"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )					// Button 3 in "test mode"
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// DSW1
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "500k 1000k" )
	PORT_DIPSETTING(    0xc0, "800k 1500k" )
	PORT_DIPSETTING(    0x40, "1000k 2000k" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START	// DSW2
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/*** GFX Decode ***/

static struct GfxLayout mcatadv_tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
	  32*8+0,32*8+4,32*8+8,32*8+12,32*8+16,32*8+20,32*8+24,32*8+28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	 64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
	128*8
};

static struct GfxDecodeInfo mcatadv_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &mcatadv_tiles16x16x4_layout, 0, 0x200 },
	{ REGION_GFX3, 0, &mcatadv_tiles16x16x4_layout, 0, 0x200 },
	{ -1 }
};


/* Stolen from Psikyo.c */
static void sound_irq( int irq )
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}
struct YM2610interface mcatadv_ym2610_interface =
{
	1,
	16000000/2,	/* ? */
	{ MIXERG(8,MIXER_GAIN_4x,MIXER_PAN_CENTER) }, // volume of AY-3-8910 voices
	{ 0 },	/* A_r */
	{ 0 },	/* B_r */
	{ 0 },	/* A_w */
	{ 0 },	/* B_w */
	{ sound_irq },	/* irq */
	{ REGION_SOUND1 },	/* delta_t */
	{ REGION_SOUND1 },	/* adpcm */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static MACHINE_INIT( mcatadv )
{
	watchdog_reset_r(0);
}


static MACHINE_DRIVER_START( mcatadv )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(mcatadv_readmem,mcatadv_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 28000000/4) // Guess, 3.5MHz is too slow and CPU comms fail reporting U9 bad.
	MDRV_CPU_MEMORY(mcatadv_sound_readmem,mcatadv_sound_writemem)
	MDRV_CPU_PORTS(mcatadv_sound_readport,mcatadv_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(mcatadv_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000/2)

	MDRV_MACHINE_INIT(mcatadv)

	MDRV_VIDEO_START(mcatadv)
	MDRV_VIDEO_EOF(mcatadv) // Buffer Spriteram
	MDRV_VIDEO_UPDATE(mcatadv)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, mcatadv_ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nost )
	MDRV_IMPORT_FROM( mcatadv )

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(nost_sound_readmem,nost_sound_writemem)
	MDRV_CPU_PORTS(nost_sound_readport,nost_sound_writeport)

	MDRV_VIDEO_START(nost)
	MDRV_VIDEO_EOF(mcatadv) // Buffer Spriteram
	MDRV_VIDEO_UPDATE(nost)
MACHINE_DRIVER_END


static DRIVER_INIT( mcatadv )
{
	data8_t *z80rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1, z80rom + 0x4000);
}


ROM_START( mcatadv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "mca-u30e", 0x00000, 0x80000, 0xc62fbb65 )
	ROM_LOAD16_BYTE( "mca-u29e", 0x00001, 0x80000, 0xcf21227c )

	ROM_REGION( 0x030000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "u9.bin", 0x00000, 0x20000, 0xfda05171 )
	ROM_RELOAD( 0x10000, 0x20000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "mca-u82.bin", 0x000000, 0x100000, 0x5f01d746 )
	ROM_LOAD16_BYTE( "mca-u83.bin", 0x000001, 0x100000, 0x4e1be5a6 )
	ROM_LOAD16_BYTE( "mca-u84.bin", 0x200000, 0x080000, 0xdf202790 )
	ROM_LOAD16_BYTE( "mca-u85.bin", 0x200001, 0x080000, 0xa85771d2 )
	ROM_LOAD16_BYTE( "mca-u86e",    0x400000, 0x080000, 0x017bf1da )
	ROM_LOAD16_BYTE( "mca-u87e",    0x400001, 0x080000, 0xbc9dc9b9 )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "mca-u58.bin", 0x000000, 0x080000, 0x3a8186e2 )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "mca-u60.bin", 0x000000, 0x100000, 0xc8942614 )
	ROM_LOAD( "mca-u61.bin", 0x100000, 0x100000, 0x51af66c9 )
	ROM_LOAD( "mca-u100",    0x200000, 0x080000, 0xb273f1b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mca-u53.bin", 0x00000, 0x80000, 0x64c76e05 )
ROM_END

ROM_START( mcatadvj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "u30.bin", 0x00000, 0x80000, 0x05762f42 )
	ROM_LOAD16_BYTE( "u29.bin", 0x00001, 0x80000, 0x4c59d648 )

	ROM_REGION( 0x030000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "u9.bin", 0x00000, 0x20000, 0xfda05171 )
	ROM_RELOAD( 0x10000, 0x20000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "mca-u82.bin", 0x000000, 0x100000, 0x5f01d746 )
	ROM_LOAD16_BYTE( "mca-u83.bin", 0x000001, 0x100000, 0x4e1be5a6 )
	ROM_LOAD16_BYTE( "mca-u84.bin", 0x200000, 0x080000, 0xdf202790 )
	ROM_LOAD16_BYTE( "mca-u85.bin", 0x200001, 0x080000, 0xa85771d2 )
	ROM_LOAD16_BYTE( "u86.bin",     0x400000, 0x080000, 0x2d3725ed )
	ROM_LOAD16_BYTE( "u87.bin",     0x400001, 0x080000, 0x4ddefe08 )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "mca-u58.bin", 0x000000, 0x080000, 0x3a8186e2 )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "mca-u60.bin", 0x000000, 0x100000, 0xc8942614 )
	ROM_LOAD( "mca-u61.bin", 0x100000, 0x100000, 0x51af66c9 )
	ROM_LOAD( "u100.bin",    0x200000, 0x080000, 0xe2c311da )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mca-u53.bin", 0x00000, 0x80000, 0x64c76e05 )
ROM_END

ROM_START( nost )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "nos-pe-u.bin", 0x00000, 0x80000, 0x4b080149 )
	ROM_LOAD16_BYTE( "nos-po-u.bin", 0x00001, 0x80000, 0x9e3cd6d9 )

	ROM_REGION( 0x050000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "nos-ps.u9", 0x00000, 0x40000, 0x832551e9 )
	ROM_RELOAD( 0x10000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "nos-se-0.u82", 0x000000, 0x100000, 0x9d99108d )
	ROM_LOAD16_BYTE( "nos-so-0.u83", 0x000001, 0x100000, 0x7df0fc7e )
	ROM_LOAD16_BYTE( "nos-se-1.u84", 0x200000, 0x100000, 0xaad07607 )
	ROM_LOAD16_BYTE( "nos-so-1.u85", 0x200001, 0x100000, 0x83d0012c )
	ROM_LOAD16_BYTE( "nos-se-2.u86", 0x400000, 0x080000, 0xd99e6005 )
	ROM_LOAD16_BYTE( "nos-so-2.u87", 0x400001, 0x080000, 0xf60e8ef3 )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "nos-b0-0.u58", 0x000000, 0x100000, 0x0214b0f2 )
	ROM_LOAD( "nos-b0-1.u59", 0x100000, 0x080000, 0x3f8b6b34 )

	ROM_REGION( 0x180000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "nos-b1-0.u60", 0x000000, 0x100000, 0xba6fd0c7 )
	ROM_LOAD( "nos-b1-1.u61", 0x100000, 0x080000, 0xdabd8009 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "nossn-00.u53", 0x00000, 0x100000, 0x3bd1bcbc )
ROM_END

ROM_START( nostj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* M68000 */
	ROM_LOAD16_BYTE( "nos-pe-j.u30", 0x00000, 0x80000, 0x4b080149 )
	ROM_LOAD16_BYTE( "nos-po-j.u29", 0x00001, 0x80000, 0x7fe241de )

	ROM_REGION( 0x050000, REGION_CPU2, 0 ) /* Z80-A */
	ROM_LOAD( "nos-ps.u9", 0x00000, 0x40000, 0x832551e9 )
	ROM_RELOAD( 0x10000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "nos-se-0.u82", 0x000000, 0x100000, 0x9d99108d )
	ROM_LOAD16_BYTE( "nos-so-0.u83", 0x000001, 0x100000, 0x7df0fc7e )
	ROM_LOAD16_BYTE( "nos-se-1.u84", 0x200000, 0x100000, 0xaad07607 )
	ROM_LOAD16_BYTE( "nos-so-1.u85", 0x200001, 0x100000, 0x83d0012c )
	ROM_LOAD16_BYTE( "nos-se-2.u86", 0x400000, 0x080000, 0xd99e6005 )
	ROM_LOAD16_BYTE( "nos-so-2.u87", 0x400001, 0x080000, 0xf60e8ef3 )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* BG0 */
	ROM_LOAD( "nos-b0-0.u58", 0x000000, 0x100000, 0x0214b0f2 )
	ROM_LOAD( "nos-b0-1.u59", 0x100000, 0x080000, 0x3f8b6b34 )

	ROM_REGION( 0x180000, REGION_GFX3, 0 ) /* BG1 */
	ROM_LOAD( "nos-b1-0.u60", 0x000000, 0x100000, 0xba6fd0c7 )
	ROM_LOAD( "nos-b1-1.u61", 0x100000, 0x080000, 0xdabd8009 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "nossn-00.u53", 0x00000, 0x100000, 0x3bd1bcbc )
ROM_END

GAMEX( 1993, mcatadv,  0,       mcatadv, mcatadv, mcatadv, ROT0,   "Wintechno", "Magical Cat Adventure", GAME_NO_COCKTAIL )
GAMEX( 1993, mcatadvj, mcatadv, mcatadv, mcatadv, mcatadv, ROT0,   "Wintechno", "Magical Cat Adventure (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1993, nost,     0,       nost,    nost,    mcatadv, ROT270, "Face",      "Nostradamus", GAME_NO_COCKTAIL )
GAMEX( 1993, nostj,    nost,    nost,    nost,    mcatadv, ROT270, "Face",      "Nostradamus (Japan)", GAME_NO_COCKTAIL )
