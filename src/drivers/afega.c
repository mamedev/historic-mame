/***************************************************************************

							  -= Afega Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU	:	M68000
Video Chips	:	AFEGA AFI-GFSK  (68 Pin PLCC)
				AFEGA AFI-GFLK (208 Pin PQFP)

Sound CPU	:	Z80
Sound Chips	:	M6295 (AD-65)  +  YM2151 (BS901)  +  YM3014 (BS90?)

---------------------------------------------------------------------------
Year + Game						Notes
---------------------------------------------------------------------------
97 Red Hawk                     US Version of Stagger 1
98 Sen Jin - Guardian Storm		Some text missing (protection, see service mode)
98 Stagger I
98 Bubble 2000                  By Tuning, but it seems to be the same HW
---------------------------------------------------------------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *afega_vram_0, *afega_scroll_0;
extern data16_t *afega_vram_1, *afega_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( afega_vram_0_w );
WRITE16_HANDLER( afega_vram_1_w );
WRITE16_HANDLER( afega_palette_w );

PALETTE_INIT( grdnstrm );

VIDEO_START( afega );
VIDEO_UPDATE( afega );
VIDEO_UPDATE( bubl2000 );


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

WRITE16_HANDLER( afega_soundlatch_w )
{
	if (ACCESSING_LSB && Machine->sample_rate)
		soundlatch_w(0,data&0xff);
}

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/

static MEMORY_READ16_START( afega_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0x080000, 0x080001, input_port_0_word_r		},	// Buttons
	{ 0x080002, 0x080003, input_port_1_word_r		},	// P1 + P2
	{ 0x080004, 0x080005, input_port_2_word_r		},	// 2 x DSW
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	// Palette
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	// Scroll
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	//
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	// Layer 0
/**/{ 0x092000, 0x093fff, MRA16_RAM					},	// ?
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	// Layer 1
	{ 0x3c0000, 0x3c7fff, MRA16_RAM					},	// RAM
	{ 0x3c8000, 0x3c8fff, MRA16_RAM					},	// Sprites
	{ 0x3c9000, 0x3cffff, MRA16_RAM					},	// RAM
	{ 0xff8000, 0xff8fff, MRA16_BANK1				},	// Sprites Mirror
MEMORY_END

static MEMORY_WRITE16_START( afega_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x080000, 0x08001d, MWA16_RAM						},	//
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	// To Sound CPU
	{ 0x080020, 0x087fff, MWA16_RAM						},	//
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	// Palette
	{ 0x088600, 0x08bfff, MWA16_RAM						},	//
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	// Scroll
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	//
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	//
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	// Layer 0
	{ 0x092000, 0x093fff, MWA16_RAM						},	// ?
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	// Layer 1
	{ 0x3c0000, 0x3c7fff, MWA16_RAM						},	// RAM
	{ 0x3c8000, 0x3c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x3c9000, 0x3cffff, MWA16_RAM						},	// RAM
	{ 0xff8000, 0xff8fff, MWA16_BANK1				},	// Sprites Mirror
MEMORY_END

/* redhawk has main ram / sprites in a different location */

static MEMORY_READ16_START( redhawk_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0x080000, 0x080001, input_port_0_word_r		},	// Buttons
	{ 0x080002, 0x080003, input_port_1_word_r		},	// P1 + P2
	{ 0x080004, 0x080005, input_port_2_word_r		},	// 2 x DSW
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	// Palette
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	// Scroll
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	//
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	// Layer 0
/**/{ 0x092000, 0x093fff, MRA16_RAM					},	// ?
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	// Layer 1
	{ 0x0c0000, 0x0c7fff, MRA16_RAM					},	// RAM
	{ 0x0c8000, 0x0c8fff, MRA16_RAM					},	// Sprites
	{ 0x0c9000, 0x0cffff, MRA16_RAM					},	// RAM
	{ 0xff8000, 0xff8fff, MRA16_BANK1				},	// Sprites Mirror
MEMORY_END

static MEMORY_WRITE16_START( redhawk_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x080000, 0x08001d, MWA16_RAM						},	//
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	// To Sound CPU
	{ 0x080020, 0x087fff, MWA16_RAM						},	//
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	// Palette
	{ 0x088600, 0x08bfff, MWA16_RAM						},	//
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	// Scroll
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	//
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	//
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	// Layer 0
	{ 0x092000, 0x093fff, MWA16_RAM						},	// ?
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	// Layer 1
	{ 0x0c0000, 0x0c7fff, MWA16_RAM						},	// RAM
	{ 0x0c8000, 0x0c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x0c9000, 0x0cffff, MWA16_RAM						},	// RAM
	{ 0xff8000, 0xff8fff, MWA16_BANK1				},	// Sprites Mirror
MEMORY_END


/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

static MEMORY_READ_START( afega_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MRA_RAM					},	// RAM
	{ 0xf800, 0xf800, soundlatch_r				},	// From Main CPU
	{ 0xf809, 0xf809, YM2151_status_port_0_r	},	// YM2151
	{ 0xf80a, 0xf80a, OKIM6295_status_0_r		},	// M6295
MEMORY_END

static MEMORY_WRITE_START( afega_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MWA_RAM					},	// RAM
	{ 0xf808, 0xf808, YM2151_register_port_0_w	},	// YM2151
	{ 0xf809, 0xf809, YM2151_data_port_0_w		},	//
	{ 0xf80a, 0xf80a, OKIM6295_data_0_w			},	// M6295
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Stagger I
***************************************************************************/

INPUT_PORTS_START( stagger1 )
	PORT_START	// IN0 - $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $080004.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
							Sen Jin - Guardian Storm
***************************************************************************/

INPUT_PORTS_START( grdnstrm )
	PORT_START	// IN0 - $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $080004.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bombs" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
								Stagger I
***************************************************************************/

INPUT_PORTS_START( bubl2000 )
	PORT_START	// IN0 - $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $080004.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Free Credit" )
	PORT_DIPSETTING(      0x0080, "500k" )
	PORT_DIPSETTING(      0x00c0, "800k" )
	PORT_DIPSETTING(      0x0040, "1000k" )
	PORT_DIPSETTING(      0x0000, "1500k" )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
//	PORT_DIPSETTING(      0x0000, "Disabled" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
//	PORT_DIPSETTING(      0x0000, "Disabled" )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4)	},
	{ STEP8(0,8*4)	},
	8*8*4
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x4_2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(0,2),1), STEP4(RGN_FRAC(1,2),1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};


static struct GfxDecodeInfo grdnstrm_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 256*1, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 256*3, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,   256*2, 16 }, // [2] Layer 1
	{ -1 }
};

static struct GfxDecodeInfo stagger1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4_2, 256*1, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,   256*0, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,     256*2, 16 }, // [2] Layer 1
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface afega_ym2151_intf =
{
	1,
	4000000,	/* ? */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct OKIM6295interface afega_m6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 70 }
};

INTERRUPT_GEN( interrupt_afega )
{
	switch ( cpu_getiloops() )
	{
		case 0:		irq2_line_hold();	break;
		case 1:		irq4_line_hold();	break;
	}
}

MACHINE_INIT( afega )
{
	/* Sprites Mirror required due to bug in the game code ( movem.w instead of movem.l ) */
	cpu_setbank( 1, spriteram16 );
}

static MACHINE_DRIVER_START( stagger1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,10000000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(afega_readmem,afega_writemem)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(afega_sound_readmem,afega_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(afega)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(stagger1_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(afega)
	MDRV_VIDEO_UPDATE(afega)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, afega_ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, afega_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( redhawk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(stagger1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(redhawk_readmem,redhawk_writemem)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( grdnstrm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(afega_readmem,afega_writemem)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(afega_sound_readmem,afega_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(afega)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(grdnstrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)
	MDRV_COLORTABLE_LENGTH(768 + 16*256)

	MDRV_PALETTE_INIT(grdnstrm)
	MDRV_VIDEO_START(afega)
	MDRV_VIDEO_UPDATE(afega)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, afega_ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, afega_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bubl2000 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(grdnstrm)

	/* video hardware */
	MDRV_VIDEO_UPDATE(bubl2000)
MACHINE_DRIVER_END

/***************************************************************************


								ROMs Loading


***************************************************************************/

/* Address lines scrambling */

static void decryptcode( int a23, int a22, int a21, int a20, int a19, int a18, int a17, int a16, int a15, int a14, int a13, int a12,
	int a11, int a10, int a9, int a8, int a7, int a6, int a5, int a4, int a3, int a2, int a1, int a0 )
{
	int i;
	data8_t *RAM = memory_region( REGION_CPU1 );
	size_t  size = memory_region_length( REGION_CPU1 );
	data8_t *buffer = malloc( size );

	if( buffer )
	{
		memcpy( buffer, RAM, size );
		for( i = 0; i < size; i++ )
		{
			RAM[ i ] = buffer[ BITSWAP24( i, a23, a22, a21, a20, a19, a18, a17, a16, a15, a14, a13, a12,
				a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0 ) ];
		}
		free( buffer );
	}
}

/***************************************************************************

									Stagger I
(AFEGA 1998)

Parts:

1 MC68HC000P10
1 Z80
2 Lattice ispLSI 1032E

***************************************************************************/

ROM_START( stagger1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2.bin", 0x000000, 0x020000, 0x8555929b )
	ROM_LOAD16_BYTE( "3.bin", 0x000001, 0x020000, 0x5b0b63ac )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, 0x5d8cf28e )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "7.bin", 0x00000, 0x80000, 0x048f7683 )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x80000, 0x051d4a77 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x4 */
	ROM_LOAD( "4.bin", 0x00000, 0x80000, 0x46463d36 )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	// Unused

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, 0xe911ce33 )
ROM_END


/***************************************************************************

							Red Hawk (c)1997 Afega



    6116         ym2145(?)  MSM6295   5    4MHz
    1
    Z80        pLSI1032    4
                                      76C88
       6116   76C256                  76C88
       6116   76C256
    2  76C256 76C256
    3  76C256 76C256

    68000-10        6116
                    6116
   SW1                             6
   SW21                pLSI1032    7
    12MHz

***************************************************************************/

ROM_START( redhawk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2", 0x000000, 0x020000, 0x3ef5f326 )
	ROM_LOAD16_BYTE( "3", 0x000001, 0x020000, 0x9b3a10ef )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, 0x5d8cf28e )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "6", 0x000001, 0x080000, 0x61560164 )
	ROM_LOAD16_BYTE( "7", 0x000000, 0x080000, 0x66a8976d )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "4", 0x000000, 0x080000, 0xd6427b8a )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	// Unused

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, 0xe911ce33 )
ROM_END

static DRIVER_INIT( redhawk )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 16, 15, 14, 17, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

/***************************************************************************

							Sen Jin - Guardian Storm

(C) Afega 1998

CPU: 68HC000FN10 (68000, 68 pin PLCC)
SND: Z84C000FEC (Z80, 44 pin PQFP), AD-65 (44 pin PQFP, Probably OKI M6295),
     BS901 (Probably YM2151 or YM3812, 24 pin DIP), BS901 (possibly YM3014 or similar? 16 pin DIP)
OSC: 12.000MHz (near 68000), 4.000MHz (Near Z84000)
RAM: LH52B256 x 8, 6116 x 7
DIPS: 2 x 8 position

Other Chips: AFEGA AFI-GFSK (68 pin PLCC, located next to 68000)
             AFEGA AFI-GFLK (208 pin PQFP)

ROMS:
GST-01.U92   27C512, \
GST-02.U95   27C2000  > Sound Related, all located near Z80
GST-03.U4    27C512  /

GST-04.112   27C2000 \
GST-05.107   27C2000  /Main Program

GST-06.C13   read as 27C160  label = AF1-SP (Sprites?)
GST-07.C08   read as 27C160  label = AF1=B2 (Backgrounds?)
GST-08.C03   read as 27C160  label = AF1=B1 (Backgrounds?)

***************************************************************************/

ROM_START( grdnstrm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gst-04.112", 0x000000, 0x040000, 0x922c931a )
	ROM_LOAD16_BYTE( "gst-05.107", 0x000001, 0x040000, 0xd22ca2dc )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "gst-01.u92", 0x00000, 0x10000, 0x5d8cf28e )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD( "gst-06.c13", 0x000000, 0x200000, 0x7d4d4985 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "gst-07.c08", 0x000000, 0x200000, 0xd68588c2 )
	ROM_LOAD( "gst-08.c03", 0x200000, 0x200000, 0xf8b200a8 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "gst-03.u4",  0x00000, 0x10000, 0xa1347297 )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "gst-02.u95", 0x00000, 0x40000, 0xe911ce33 )
ROM_END

static DRIVER_INIT( grdnstrm )
{
	data16_t *RAM = (data16_t *)memory_region( REGION_CPU1 );

	decryptcode( 23, 22, 21, 20, 19, 18, 16, 17, 14, 15, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );

	/* Patch Protection (that supplies some 68k code seen in the
	   2760-29cf range */
	RAM[0x0027a/2] = 0x4e71;
	RAM[0x02760/2] = 0x4e75;
}


/***************************************************************************

							Bubble 2000 (c)1998 Tuning

Bubble 2000
Tuning, 1998

CPU   : TMP68HC000P-10 (68000)
SOUND : Z840006 (Z80, 44 pin QFP), YM2151, OKI M6295
OSC   : 4.000MHZ, 12.000MHz
DIPSW : 8 position (x2)
RAM   : 6116 (x5, gfx related?) 6116 (x1, sound program ram), 6116 (x1, near rom3)
        64256 (x4, gfx related?), 62256 (x2, main program ram), 6264 (x2, gfx related?)
PALs/PROMs: None
Custom: Unknown 208 pin QFP labelled LTC2 (Graphics generator)
        Unknown 68 pin PLCC labelled LTC1 (?, near rom 2 and rom 3)
ROMs  :

Filename    Type        Possible Use
----------------------------------------------
rom01.92    27C512      Sound Program
rom02.95    27C020      Oki Samples
rom03.4     27C512      ? (located near rom 1 and 2 and near LTC1)
rom04.1     27C040   \
rom05.3     27C040    |
rom06.6     27C040    |
rom07.9     27C040    | Gfx
rom08.11    27C040    |
rom09.14    27C040    |
rom12.2     27C040    |
rom13.7     27C040   /

rom10.112   27C040   \  Main Program
rom11.107   27C040   /


Developers......
                More info reqd? or a redump?
                Email me....
                theguru@emuunlim.com


***************************************************************************/

ROM_START( bubl2000 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom10.112", 0x00000, 0x20000, 0x87f960d7 )
	ROM_LOAD16_BYTE( "rom11.107", 0x00001, 0x20000, 0xb386041a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom01.92", 0x00000, 0x10000, 0x5d8cf28e ) /* same as the other games on this driver */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "rom08.11", 0x000000, 0x040000, 0x519dfd82 )
	ROM_LOAD16_BYTE( "rom09.14", 0x000001, 0x040000, 0x04fcb5c6 )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "rom06.6",  0x000000, 0x080000, 0xac1aabf5 )
	ROM_LOAD( "rom07.9",  0x080000, 0x080000, 0x69aff769 )
	ROM_LOAD( "rom13.7",  0x100000, 0x080000, 0x3a5b7226 )
	ROM_LOAD( "rom04.1",  0x180000, 0x080000, 0x46acd054 )
	ROM_LOAD( "rom05.3",  0x200000, 0x080000, 0x37deb6a1 )
	ROM_LOAD( "rom12.2",  0x280000, 0x080000, 0x1fdc59dd )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "rom03.4",  0x00000, 0x10000, 0xf4c15588 )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom02.95", 0x00000, 0x40000, 0x859a86e5 )
ROM_END

static DRIVER_INIT( bubl2000 )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 13, 14, 15, 16, 17, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1998, stagger1, 0,        stagger1, stagger1, 0,        ROT270, "Afega", "Stagger I (Japan)",                GAME_NOT_WORKING )
GAMEX( 1997, redhawk,  stagger1, redhawk,  stagger1, redhawk,  ROT270, "Afega", "Red Hawk (US)", GAME_NOT_WORKING )
GAMEX( 1998, grdnstrm, 0,        grdnstrm, grdnstrm, grdnstrm, ROT270, "Afega", "Sen Jin - Guardian Storm (Korea)", GAME_NOT_WORKING )
GAMEX( 1998, bubl2000, 0,        bubl2000, bubl2000, bubl2000, ROT0,   "Tuning", "Bubble 2000", GAME_IMPERFECT_GRAPHICS )
