/***************************************************************************

		driver by Phil Stroffolino, Nicola Salmoria, Luca Elia

---------------------------------------------------------------------------
Year + Game					By				CPUs		Sound Chips
---------------------------------------------------------------------------
82	Lasso					SNK				3 x 6502	2 x SN76489
83	Chameleon				Jaleco			2 x 6502	2 x SN76489
84	Wai Wai Jockey Gate-In!	Jaleco/Casio	2 x 6502	2 x SN76489 + DAC
---------------------------------------------------------------------------

Notes:

- unknown CPU speeds (affect game timing)
- Lasso: fire button auto-repeats on high score entry screen (real behavior?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "lasso.h"

/* Shared RAM between Main CPU and sub CPU */

static data8_t *shareram;

static READ_HANDLER( shareram_r )
{
	return shareram[offset];
}
static WRITE_HANDLER( shareram_w )
{
	shareram[offset] = data;
}


/***************************************************************************


								Sound Handling


***************************************************************************/

/* Write to the sound latch and generate an IRQ on the sound CPU */

static WRITE_HANDLER( lasso_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line( 2, 0, PULSE_LINE );
}
static WRITE_HANDLER( chameleo_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line( 1, 0, PULSE_LINE );
}

static READ_HANDLER( sound_status_r )
{
	/*	0x01: chip#0 ready; 0x02: chip#1 ready */
	return 0x03;
}

static data8_t lasso_chip_data;

static WRITE_HANDLER( sound_data_w )
{
	lasso_chip_data =
			((data & 0x80) >> 7) |
			((data & 0x40) >> 5) |
			((data & 0x20) >> 3) |
			((data & 0x10) >> 1) |
			((data & 0x08) << 1) |
			((data & 0x04) << 3) |
			((data & 0x02) << 5) |
			((data & 0x01) << 7);
}

static WRITE_HANDLER( sound_select_w )
{
	if (~data & 0x01)	/* chip #0 */
		SN76496_0_w(0,lasso_chip_data);

	if (~data & 0x02)	/* chip #1 */
		SN76496_1_w(0,lasso_chip_data);
}



/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

/***************************************************************************
								Chameleon
***************************************************************************/

static MEMORY_READ_START( chameleo_readmem )
	{ 0x0000, 0x03ff, MRA_RAM			},	// Work RAM
	{ 0x0400, 0x0bff, MRA_RAM			},	// Tilemap
	{ 0x0c00, 0x0fff, MRA_RAM			},	//
	{ 0x1000, 0x107f, MRA_RAM			},	// Sprites
	{ 0x1080, 0x10ff, MRA_RAM			},	//
	{ 0x1804, 0x1804, input_port_0_r	},	// Player 1
	{ 0x1805, 0x1805, input_port_1_r	},	// Player 2
	{ 0x1806, 0x1806, input_port_2_r	},	// DSW
	{ 0x1807, 0x1807, input_port_3_r	},	// Coins + DSW
	{ 0x4000, 0xbfff, MRA_ROM			},	// ROM
	{ 0xfffa, 0xffff, MRA_ROM			},	// ROM (Mirror)
MEMORY_END
static MEMORY_WRITE_START( chameleo_writemem )
	{ 0x0000, 0x03ff, MWA_RAM						},	// Work RAM
	{ 0x0400, 0x0bff, lasso_videoram_w, &videoram	},	// Tilemap
	{ 0x0c00, 0x0fff, MWA_RAM						},	//
	{ 0x1000, 0x107f, MWA_RAM, &spriteram, &spriteram_size },	// Sprites
	{ 0x1080, 0x10ff, MWA_RAM						},
	{ 0x1800, 0x1800, chameleo_sound_command_w		},	// To Sound CPU
	{ 0x1801, 0x1801, lasso_backcolor_w				},	// Background Color
	{ 0x1802, 0x1802, lasso_gfxbank_w				},	// Flip Screen + Tile Bank
	{ 0x4000, 0xbfff, MWA_ROM						},	// ROM
	{ 0xc428, 0xc429, MWA_NOP						},	// ? written once with $0&41
	{ 0xfffa, 0xffff, MWA_ROM						},	// ROM (Mirror)
MEMORY_END

/***************************************************************************
								Lasso
***************************************************************************/

static MEMORY_READ_START( lasso_readmem )
	{ 0x0000, 0x03ff, MRA_RAM			},	// Work RAM
	{ 0x0400, 0x0bff, MRA_RAM			},	// Tilemap
	{ 0x0c00, 0x0c7f, MRA_RAM			},	// Sprites
	{ 0x1000, 0x17ff, shareram_r		},	// Shared RAM (17f0 on CPU1 maps to 07f0 on CPU2)
	{ 0x1804, 0x1804, input_port_0_r	},	// Player 1
	{ 0x1805, 0x1805, input_port_1_r	},	// Player 2
	{ 0x1806, 0x1806, input_port_2_r	},	// DSW
	{ 0x1807, 0x1807, input_port_3_r	},	// Coins
	{ 0x8000, 0xffff, MRA_ROM			},	// ROM
MEMORY_END
static MEMORY_WRITE_START( lasso_writemem )
	{ 0x0000, 0x03ff, MWA_RAM						},	// ROM
	{ 0x0400, 0x0bff, lasso_videoram_w, &videoram	},	// Tilemap
	{ 0x0c00, 0x0c7f, MWA_RAM, &spriteram, &spriteram_size },	// Sprites
	{ 0x1000, 0x17ff, shareram_w					},	// Shared RAM
	{ 0x1800, 0x1800, lasso_sound_command_w			},	// To Sound CPU
	{ 0x1801, 0x1801, lasso_backcolor_w				},	// Background Color
	{ 0x1802, 0x1802, lasso_gfxbank_w				},	// Flip Screen + Tile Bank
	{ 0x1806, 0x1806, MWA_NOP						},	// ? spurious write
	{ 0x8000, 0xffff, MWA_ROM						},	// ROM
MEMORY_END

/***************************************************************************
							Wai Wai Jockey Gate-In!
***************************************************************************/

static MEMORY_READ_START( wwjgtin_readmem )
	{ 0x0000, 0x07ff, MRA_RAM			},	// Work RAM
	{ 0x0800, 0x0fff, MRA_RAM			},	// Tilemap
	{ 0x1000, 0x10ff, MRA_RAM			},	// Sprites
	{ 0x1804, 0x1804, input_port_0_r	},	// Player 1
	{ 0x1805, 0x1805, input_port_1_r	},	// Player 2
	{ 0x1806, 0x1806, input_port_2_r	},	// DSW
	{ 0x1807, 0x1807, input_port_3_r	},	// Coins + DSW
	{ 0x5000, 0xbfff, MRA_ROM			},	// ROM
	{ 0xfffa, 0xffff, MRA_ROM			},	// ROM (Mirror)
MEMORY_END
static MEMORY_WRITE_START( wwjgtin_writemem )
	{ 0x0000, 0x07ff, MWA_RAM						},	// Work RAM
	{ 0x0800, 0x0fff, lasso_videoram_w, &videoram	},	// Tilemap
	{ 0x1000, 0x10ff, MWA_RAM, &spriteram, &spriteram_size },	// Sprites
	{ 0x1800, 0x1800, chameleo_sound_command_w		},	// To Sound CPU
	{ 0x1801, 0x1801, lasso_backcolor_w				},	// Background Color
	{ 0x1802, 0x1802, wwjgtin_gfxbank_w				},	// Flip Screen + Tile Bank
	{ 0x1c00, 0x1c03, wwjgtin_lastcolor_w			},	// Last 4 Colors
	{ 0x1c04, 0x1c07, MWA_RAM, &wwjgtin_scroll		},	// Scroll (For The Tilemap in ROM)
	{ 0x5000, 0xbfff, MWA_ROM						},	// ROM
	{ 0xfffa, 0xffff, MWA_ROM						},	// ROM (Mirror)
MEMORY_END



/***************************************************************************


						Memory Maps - Sub CPU (blitter)


***************************************************************************/

/***************************************************************************
								Lasso
***************************************************************************/

static MEMORY_READ_START( lasso_coprocessor_readmem )
	{ 0x0000, 0x07ff, MRA_RAM				},	// Shared RAM
	{ 0x2000, 0x3fff, MRA_RAM				},	// Video RAM
	{ 0x8000, 0x8fff, MRA_ROM				},	// ROM
	{ 0xf000, 0xffff, MRA_ROM				},	// ROM (Mirror)
MEMORY_END
static MEMORY_WRITE_START( lasso_coprocessor_writemem )
	{ 0x0000, 0x07ff, MWA_RAM, &shareram	},	// Shared RAM (code is executed from here!)
	{ 0x2000, 0x3fff, MWA_RAM, &lasso_vram	},	// Video RAM
	{ 0x8000, 0x8fff, MWA_ROM				},	// ROM
	{ 0xf000, 0xffff, MWA_ROM				},	// ROM (Mirror)
MEMORY_END



/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

/***************************************************************************
								Chameleon
***************************************************************************/

static MEMORY_READ_START( chameleo_sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// Work RAM
	{ 0x1000, 0x1fff, MRA_ROM			},	// ROM
	{ 0x6000, 0x7fff, MRA_ROM			},	//
	{ 0xb004, 0xb004, sound_status_r	},	// Sound
	{ 0xb005, 0xb005, soundlatch_r		},	//
	{ 0xfffa, 0xffff, MRA_ROM			},	// ROM (Mirror)
MEMORY_END
static MEMORY_WRITE_START( chameleo_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM			},	// Work RAM
	{ 0x1000, 0x1fff, MWA_ROM			},	// ROM
	{ 0x6000, 0x7fff, MWA_ROM			},	//
	{ 0xb000, 0xb000, sound_data_w		},	// Sound
	{ 0xb001, 0xb001, sound_select_w	},	//
	{ 0xfffa, 0xffff, MWA_ROM			},	// ROM (Mirror)
MEMORY_END

/***************************************************************************
								Lasso
***************************************************************************/

static MEMORY_READ_START( lasso_sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// Work RAM
	{ 0x5000, 0x7fff, MRA_ROM			},	// ROM
	{ 0xb004, 0xb004, sound_status_r	},	// Sound
	{ 0xb005, 0xb005, soundlatch_r		},	// From Main CPU
	{ 0xf000, 0xffff, MRA_ROM			},	// ROM (mirror)
MEMORY_END
static MEMORY_WRITE_START( lasso_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM			},	// Work RAM
	{ 0x5000, 0x7fff, MWA_ROM			},	// ROM
	{ 0xb000, 0xb000, sound_data_w		},	// Sound
	{ 0xb001, 0xb001, sound_select_w	},	//
	{ 0xf000, 0xffff, MWA_ROM			},	// ROM (mirror)
MEMORY_END

/***************************************************************************
							Wai Wai Jockey Gate-In!
***************************************************************************/

static MEMORY_READ_START( wwjgtin_sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// Work RAM
	{ 0x5000, 0x7fff, MRA_ROM			},	// ROM
	{ 0xb004, 0xb004, sound_status_r	},	// Sound
	{ 0xb005, 0xb005, soundlatch_r		},	// From Main CPU
	{ 0xfffa, 0xffff, MRA_ROM			},	// ROM (mirror)
MEMORY_END
static MEMORY_WRITE_START( wwjgtin_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM			},	// Work RAM
	{ 0x5000, 0x7fff, MWA_ROM			},	// ROM
	{ 0xb000, 0xb000, sound_data_w		},	// Sound
	{ 0xb001, 0xb001, sound_select_w	},	//
	{ 0xb003, 0xb003, DAC_0_data_w		},	//
	{ 0xfffa, 0xffff, MWA_ROM			},	// ROM (mirror)
MEMORY_END



/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Chameleon
***************************************************************************/

INPUT_PORTS_START( chameleo )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x30, "5" )
//	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX(0,        0x20, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_JOY_NONE, IP_KEY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

/***************************************************************************
								Lasso
***************************************************************************/

INPUT_PORTS_START( lasso )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* lasso */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* shoot */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2	| IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
//	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Show Instructions" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )

	PORT_START /* 1807 */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

/***************************************************************************
							Wai Wai Jockey Gate-In!
***************************************************************************/

INPUT_PORTS_START( wwjgtin )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
//	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x01, "50k" )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2  )
INPUT_PORTS_END


/***************************************************************************


								Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x2 =
{
	8,8,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout layout_16x16x2 =
{
	16,16,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{	RGN_FRAC(1,4),RGN_FRAC(3,4),RGN_FRAC(0,4),RGN_FRAC(2,4)	},
	{	STEP8(0,1), STEP8(8*8*1,1)	},
	{	STEP8(0,8), STEP8(8*8*2,8)	},
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x2,	0,	16	},	// [0] Tiles
	{ REGION_GFX1, 0, &layout_16x16x2,	0,	16	},	// [1] Sprites
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_wwjgtin[] =
{
	{ REGION_GFX1, 0, &layout_8x8x2,	0,		16	},	// [0] Tiles
	{ REGION_GFX1, 0, &layout_16x16x2,	0,		16	},	// [1] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,	4*0x10,	16	},	// [2] Tiles (for the tilemap in ROM)
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/

/* IRQ = VBlank, NMI = Coin Insertion */

int lasso_interrupt( void )
{
	static int old;
	int new;

	// VBlank
	if (cpu_getiloops() == 0)
		return interrupt();

	// Coins
	new = ~readinputport(3) & 0x30;

	if ( ((new & 0x10) && !(old & 0x10)) ||
		 ((new & 0x20) && !(old & 0x20)) )
	{
		old = new;
		return nmi_interrupt();
	}

	old = new;
	return ignore_interrupt();
}


static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 2000000, 2000000 },	/* ? MHz */
	{ 100, 100 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

/***************************************************************************
								Chameleon
***************************************************************************/

static const struct MachineDriver machine_driver_chameleo =
{
	{
		{
			CPU_M6502,
			2000000,	/* 2 MHz (?) */
			chameleo_readmem, chameleo_writemem, 0,0,
			lasso_interrupt, 2,		/* IRQ = VBlank, NMI = Coin Insertion */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			600000,		/* ?? (controls music tempo) */
			chameleo_sound_readmem,chameleo_sound_writemem, 0,0,
			ignore_interrupt, 1,	/* IRQ by Main CPU, no NMI */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1, /* CPU slices */
	0, /* init machine */

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 255-16-1 },
	gfxdecodeinfo,
	0x40,0x40,
	lasso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	lasso_vh_start,
	0,
	chameleo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_SN76496,	&sn76496_interface	}
	}
};

/***************************************************************************
								Lasso
***************************************************************************/

static const struct MachineDriver machine_driver_lasso =
{
	{
		{
			CPU_M6502,
			2000000,	/* 2 MHz (?) */
			lasso_readmem,lasso_writemem, 0,0,
			lasso_interrupt, 2,		/* IRQ = VBlank, NMI = Coin Insertion */
		},
		{
			CPU_M6502,
			2000000,	/* 2 MHz (?) */
			lasso_coprocessor_readmem,lasso_coprocessor_writemem, 0,0,
			ignore_interrupt, 1,	/* No IRQ */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			600000,		/* ?? (controls music tempo) */
			lasso_sound_readmem,lasso_sound_writemem, 0,0,
			ignore_interrupt, 1,	/* IRQ by Main CPU, no NMI */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100, /* CPU slices */
	0, /* init machine */

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 255-16-1 },
	gfxdecodeinfo,
	0x40,0x40,
	lasso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	lasso_vh_start,
	0,
	lasso_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_SN76496,	&sn76496_interface	}
	}
};

/***************************************************************************
							Wai Wai Jockey Gate-In!
***************************************************************************/

static const struct MachineDriver machine_driver_wwjgtin =
{
	{
		{
			CPU_M6502,
			2000000,	/* 2 MHz (?) */
			wwjgtin_readmem, wwjgtin_writemem, 0,0,
			lasso_interrupt, 2,		/* IRQ = VBlank, NMI = Coin Insertion */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			600000,		/* ?? (controls music tempo) */
			wwjgtin_sound_readmem,wwjgtin_sound_writemem, 0,0,
			ignore_interrupt, 1,	/* IRQ by Main CPU, no NMI */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1, /* CPU slices */
	0, /* init machine */

	/* video hardware */
	256, 256, { 0+8, 256-1-8, 0+16, 256-1-16 },	// Smaller visible area?
	gfxdecodeinfo_wwjgtin,	// Has 1 additional layer
	0x40+1, 4*16 + 16*16,	// Reserve 1 color for black
	wwjgtin_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	wwjgtin_vh_start,
	0,
	wwjgtin_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_SN76496,	&sn76496_interface	},
		{	SOUND_DAC,		&dac_interface		}
	}
};


/***************************************************************************


								Game Drivers


***************************************************************************/

/***************************************************************************

								Chameleon

Jaleco Ltd. (Japan Leisure Ltd.)  1983
Chameleon.
4/30/99

Hardware Specs: Dual 6502-11 CPUS

Sound: Two TI SN76489 PSGs

***************************************************************************/

ROM_START( chameleo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "chamel4.bin", 0x4000, 0x2000, 0x97379c47 )
	ROM_LOAD( "chamel5.bin", 0x6000, 0x2000, 0x0a2cadfd )
	ROM_LOAD( "chamel6.bin", 0x8000, 0x2000, 0xb023c354 )
	ROM_LOAD( "chamel7.bin", 0xa000, 0x2000, 0xa5a03375 )
	ROM_RELOAD(              0xe000, 0x2000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "chamel3.bin", 0x1000, 0x1000, 0x52eab9ec )
	ROM_LOAD( "chamel2.bin", 0x6000, 0x1000, 0x81dcc49c )
	ROM_LOAD( "chamel1.bin", 0x7000, 0x1000, 0x96031d3b )
	ROM_RELOAD(              0xf000, 0x1000             )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chamel8.bin", 0x0800, 0x800, 0xdc67916b )	/* Tiles   */
	ROM_CONTINUE(            0x1800, 0x800             )	/* Sprites */
	ROM_CONTINUE(            0x0000, 0x800             )
	ROM_CONTINUE(            0x1000, 0x800             )
	ROM_LOAD( "chamel9.bin", 0x2800, 0x800, 0x6b559bf1 )	/* 2nd bitplane */
	ROM_CONTINUE(            0x3800, 0x800             )
	ROM_CONTINUE(            0x2000, 0x800             )
	ROM_CONTINUE(            0x3000, 0x800             )

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "chambprm.bin", 0x00, 0x20, 0xe3ad76df )
	ROM_LOAD( "chamaprm.bin", 0x20, 0x20, 0xc7063b54 )
ROM_END

/***************************************************************************

								Lasso

USES THREE 6502 CPUS

CHIP #  POSITION  TYPE
-----------------------
WMA     IC19      2732   DAUGHTER BD	sound cpu
WMB     IC18       "      "				sound data
WMC     IC17       "      "				sound data
WM5     IC45       "     CONN BD		bitmap coprocessor
82S123  IC69              "
82S123  IC70              "
WM4     IC22      2764   BOTTOM BD		main cpu
WM3     IC21       "      "				main cpu
WM2     IC66       "      "				graphics
WM1     IC65       "      "				graphics

***************************************************************************/

ROM_START( lasso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6502 code (main cpu) */
	ROM_LOAD( "wm3",       0x8000, 0x2000, 0xf93addd6 )
	ROM_RELOAD(            0xc000, 0x2000)
	ROM_LOAD( "wm4",       0xe000, 0x2000, 0x77719859 )
	ROM_RELOAD(            0xa000, 0x2000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 6502 code (lasso image blitter) */
	ROM_LOAD( "wm5",       0xf000, 0x1000, 0x7dc3ff07 )
	ROM_RELOAD(            0x8000, 0x1000)

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 6502 code (sound) */
	ROM_LOAD( "wmc",       0x5000, 0x1000, 0x8b4eb242 )
	ROM_LOAD( "wmb",       0x6000, 0x1000, 0x4658bcb9 )
	ROM_LOAD( "wma",       0x7000, 0x1000, 0x2e7de3e9 )
	ROM_RELOAD(            0xf000, 0x1000 )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1",       0x0000, 0x0800, 0x7db77256 )	/* Tiles   */
	ROM_CONTINUE(          0x1000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(          0x0800, 0x0800             )
	ROM_CONTINUE(          0x1800, 0x0800             )
	ROM_LOAD( "wm2",       0x2000, 0x0800, 0x9e7d0b6f )	/* 2nd bitplane */
	ROM_CONTINUE(          0x3000, 0x0800             )
	ROM_CONTINUE(          0x2800, 0x0800             )
	ROM_CONTINUE(          0x3800, 0x0800             )

	ROM_REGION( 0x40, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.69", 0x0000, 0x0020, 0x1eabb04d )
	ROM_LOAD( "82s123.70", 0x0020, 0x0020, 0x09060f8c )
ROM_END

/***************************************************************************

							Wai Wai Jockey Gate-In!

(c)1984 Jaleco (developed by Casio?)
GI-8422A (top) / GI-8423A (bottom)

CPU  : M6502(x2, each board has 1)
Sound: SN76489(x2) DAC?
OSC  : 18.000MHz (on bottom BD)

ROMs:
(Top BD)
ic49.1 - Graphics (2764)
ic48.2 |
ic47.3 |
ic46.4 /

ic5.5  - 6502 Program (27128)
ic2.6  /

1.bpr - Color PROM (ic81, MB7051)
2.bpr - Color PROM (ic80, MB7051)

(Bottom BD)
ic81.7 - Graphics (27128)
ic82.8 /

ic59.9 - 6502 Program (27128)

Boards:

							GI-8422A
	N82S16N			LS174	2764 (1)	DIPSW1	LS240
	N82S16N	LS74	LS151	2764 (2)	DIPSW2	LS240
	N82S16N	LS86	LS374	2764 (3)	LS283	LS240
	N82S16N	LS86	LS151	2764 (4)	LS283	LS240	LS14
	N82S16N	LS163	LS374				LS283	LS374	LS157
	N82S16N	LS163	LS04	LS136		LS283	LS374	LS138
	LS08	LS158	LS157	LS20		LS283	LS374
	LS08	LS158	LS157	LS138		LS283	LS374
	LS02			LS04	LS00		LS174	LS374
	MSM2128-20		LS138	LS155		LS32	LS374
	27128 (5)		LS138	LS155		LS08	LS174	MB7051 (1)
					LS138	LS04		LS10	LS174	MB7051 (2)
					LS244				LS107	LS273	SN76489
	27128 (6)		LS244	M6502		LS121	LS02	SN76489
					LS245				HA17555	LS32

							GI-8423A
	LS244	LS273		LS155	27128 (9)		LS244	LS244
	LS174	LS374		MSM2128-15					M6502	LS393
	LS125	LS374
										LS121	LS138		LS393
	LS157	N82S09		LS157	LS393	LS04	LS00
	LS273	LS273		LS157	LS10	LS74	LS10		LS157
	LS240	LS174		LS193	LS139	LS04	LS32		LS194
	LS283	LS273		LS86	LS139	LS08	27128 (8)	LS194
	LS283	LS377		LS174	LS157	LS20				LS194
	LS157	LS377		LS273	LS157	LS174	27128 (7)	LS194
	LS157	LS08		LS244	LS283	LS08	LS02		LS74
	LS157	LS86		LS244	LS283	LS00	LS27		LS368
	LS157	MSM2128-15	LS244	LS86	LS74	LS161		LS74
	LS157	MSM2128-15	LS244	LS86	LS10	LS161		LS107
	LS157	MSM2128-15	LS244	LS86	LS04	LS161		LS04
	LS157				LS244	LS86	LS74	LS161		OSC(18MHz)

Inputs:

DIPSW1				1	2	3	4	5	6	7	8		Parts side				Solder side
Coin 1		1cn/1cr		off	off	off						GND			1	2		GND
			1cn/2cr		off	off	on						GND			3	4		GND
			1cn/3cr		off	on	off						+5V			5	6		+5V
			1cn/6cr		off	on	on						+5V			7	8		+5V
			2cn/1cr		on	off	off						2p Down		9	10		2p Up
														2p Right	11	12		2p Left
DIPSW2				1	2	3	4	5	6	7	8		2p Jump		13	14		2p Whip
Coin 2		1cn/1cr	off									(not used)	15	16		(not used)
			2cn/1cr	on									1p Down		17	18		1p Up
Extra Play	20000			off							1p Right	19	20		1p Left
			50000			on							1p Jump		21	22		1p Whip
Attract Snd	Yes							On				(not used)	23	24		(not used)
			No							Off				Coin 1		25	26		1p Start
														GND			27	28		Coin 2
														GND			29	30		2p Start
														Green		31	32		Red
														Sync		33	34		Blue
														(not used)	35	36		Speaker
														+12V		37	38		+12V
														+12V		39	40		+12V
														GND			41	42		GND
														GND			43	44		GND

***************************************************************************/

ROM_START( wwjgtin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "ic2.6", 0x4000, 0x4000, 0x744ba45b )
	ROM_LOAD( "ic5.5", 0x8000, 0x4000, 0xaf751614 )
	ROM_RELOAD(        0xc000, 0x4000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "ic59.9", 0x4000, 0x4000, 0x2ecb4d98 )
	ROM_RELOAD(         0xc000, 0x4000             )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic81.7", 0x0000, 0x800, 0xa27f1a63 )	/* Tiles   */
	ROM_CONTINUE(       0x2000, 0x800             )	/* Sprites */
	ROM_CONTINUE(       0x0800, 0x800             )
	ROM_CONTINUE(       0x2800, 0x800             )
	ROM_CONTINUE(       0x1000, 0x800             )
	ROM_CONTINUE(       0x3000, 0x800             )
	ROM_CONTINUE(       0x1800, 0x800             )
	ROM_CONTINUE(       0x3800, 0x800             )
	ROM_LOAD( "ic82.8", 0x4000, 0x800, 0xea2862b3 )	/* 2nd bitplane */
	ROM_CONTINUE(       0x6000, 0x800             )	/* Sprites */
	ROM_CONTINUE(       0x4800, 0x800             )
	ROM_CONTINUE(       0x6800, 0x800             )
	ROM_CONTINUE(       0x5000, 0x800             )
	ROM_CONTINUE(       0x7000, 0x800             )
	ROM_CONTINUE(       0x5800, 0x800             )
	ROM_CONTINUE(       0x7800, 0x800             )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )	/* Gfx */
	ROM_LOAD( "ic47.3", 0x0000, 0x2000, 0x40594c59 )	// 1xxxxxxxxxxxx = 0xFF
	ROM_LOAD( "ic46.4", 0x2000, 0x2000, 0xd1921348 )

	ROM_REGION( 0x4000, REGION_GFX3, 0 )	/* TILEMAP */
	ROM_LOAD( "ic48.2", 0x0000, 0x2000, 0xa4a7df77 )
	ROM_LOAD( "ic49.1", 0x2000, 0x2000, 0xe480fbba )	// FIXED BITS (1111xxxx)

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "2.bpr", 0x00, 0x20, 0x79adda5d )
	ROM_LOAD( "1.bpr", 0x20, 0x20, 0xc1a93cc8 )
ROM_END



/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1982, lasso,    0, lasso,    lasso,    0, ROT90, "SNK",            "Lasso"                   )
GAME( 1983, chameleo, 0, chameleo, chameleo, 0, ROT0,  "Jaleco",         "Chameleon"               )
GAME( 1984, wwjgtin,  0, wwjgtin,  wwjgtin,  0, ROT0,  "Jaleco / Casio", "Wai Wai Jockey Gate-In!" )
