/***************************************************************************

								  -= Shocking =-

					driver by	Luca Elia (eliavit@unina.it)


Main  CPU    :  MC68000
Video Chips  :	?
Sound Chips  :	OKI M6295
Other        :  ?

- Service mode just shows the menu with mangled graphics (sprites,
  but the charset they used is in the tiles ROMs!)
  Is it an unfinished left over ?

- Screen & sprites flipping: not used!?

- Are priorities correct ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *shocking_vram_0,   *shocking_vram_1;
extern data16_t *shocking_scroll_0, *shocking_scroll_1;
extern data16_t *shocking_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( shocking_vram_0_w );
WRITE16_HANDLER( shocking_vram_1_w );

int  shocking_vh_start(void);
void shocking_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


								Memory Maps


***************************************************************************/

static WRITE16_HANDLER( shocking_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		int bank = data & 3;
		unsigned char *dst	= memory_region(REGION_SOUND1);
		unsigned char *src	= dst + 0x80000 + 0x20000 * bank;
		memcpy(dst + 0x20000, src, 0x20000);
	}
}

static MEMORY_READ16_START( shocking_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0xff0000, 0xffffff, MRA16_RAM					},	// RAM
	{ 0x800000, 0x800001, input_port_0_word_r		},	// P1 + P2
	{ 0x800018, 0x800019, input_port_1_word_r		},	// Coins
	{ 0x80001a, 0x80001b, input_port_2_word_r		},	// DSW1
	{ 0x80001c, 0x80001d, input_port_3_word_r		},	// DSW2
	{ 0x800188, 0x800189, OKIM6295_status_0_lsb_r	},	// Sound
	{ 0x900000, 0x903fff, MRA16_RAM					},	// Palette
	{ 0x908000, 0x90bfff, MRA16_RAM					},	// Layer 1
	{ 0x90c000, 0x90ffff, MRA16_RAM					},	// Layer 0
	{ 0x910000, 0x910fff, MRA16_RAM					},	// Sprites
MEMORY_END

static MEMORY_WRITE16_START( shocking_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM					},	// ROM
	{ 0xff0000, 0xffffff, MWA16_RAM					},	// RAM
	{ 0x800030, 0x800031, MWA16_NOP					},	// ? (value: don't care)
	{ 0x800100, 0x800101, MWA16_NOP					},	// ? $9100
	{ 0x800102, 0x800103, MWA16_NOP					},	// ? $9080
	{ 0x800104, 0x800105, MWA16_NOP					},	// ? $90c0
	{ 0x80010a, 0x80010b, MWA16_NOP					},	// ? $9000
	{ 0x80010c, 0x80010f, MWA16_RAM, &shocking_scroll_1	},	// Scrolling
	{ 0x800114, 0x800117, MWA16_RAM, &shocking_scroll_0	},	//
	{ 0x800154, 0x800155, MWA16_RAM, &shocking_priority	},	// Priority
	{ 0x800180, 0x800181, shocking_sound_bank_w		},	// Sound
	{ 0x800188, 0x800189, OKIM6295_data_0_lsb_w		},	//
	{ 0x8001fe, 0x8001ff, MWA16_NOP					},	// ? 0 (during int)
	{ 0x900000, 0x903fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x908000, 0x90bfff, shocking_vram_1_w, &shocking_vram_1		},	// Layer 1
	{ 0x90c000, 0x90ffff, shocking_vram_0_w, &shocking_vram_0		},	// Layer 0
	{ 0x910000, 0x910fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( shocking )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $800019.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $80001b.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )		// rest unused
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $80001d.b
	PORT_DIPNAME( 0x0007, 0x0007, "Unknown 2-0&1&2" )
	PORT_DIPSETTING(      0x0004, "0" )
	PORT_DIPSETTING(      0x0005, "1" )
	PORT_DIPSETTING(      0x0006, "2" )
	PORT_DIPSETTING(      0x0007, "3" )
	PORT_DIPSETTING(      0x0003, "4" )
	PORT_DIPSETTING(      0x0002, "5" )
	PORT_DIPSETTING(      0x0001, "6" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

INPUT_PORTS_END




/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 16x16x4 */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(0,4) },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

/* 16x16x8 */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 6*8,4*8, 2*8,0*8, 7*8,5*8, 3*8,1*8 },
	{ STEP8(0,1),STEP8(8*8,1) },
	{ STEP16(0,16*8) },
	16*16*8
};


static struct GfxDecodeInfo shocking_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x1000, 0x10 }, // [0] Layers
	{ REGION_GFX2, 0, &layout_16x16x4, 0x0000, 0x20 }, // [1] Sprites
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct OKIM6295interface shocking_okim6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static const struct MachineDriver machine_driver_shocking =
{
	{
		{
			CPU_M68000,
			12000000,	/* ? */
			shocking_readmem, shocking_writemem,0,0,
			m68_level2_irq, 1
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x180, 0xe0, { 0, 0x180-1-4, 0, 0xe0-1 },
	shocking_gfxdecodeinfo,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	shocking_vh_start,
	0,
	shocking_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_OKIM6295, &shocking_okim6295_intf }
	},
};



/***************************************************************************


								ROMs Loading


***************************************************************************/

ROM_START( shocking )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "shocking.u33", 0x000000, 0x040000, 0x8a155521 )
	ROM_LOAD16_BYTE( "shocking.u32", 0x000001, 0x040000, 0xc4998c10 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "shocking.u67", 0x000000, 0x080000, 0xe30fb2c4, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "shocking.u68", 0x000002, 0x080000, 0x7d702538, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "shocking.u69", 0x000004, 0x080000, 0x97447fec, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "shocking.u70", 0x000006, 0x080000, 0x1b1f7895, ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "shocking.u20", 0x000000, 0x040000, 0x124699d0 )
	ROM_LOAD( "shocking.u21", 0x040000, 0x040000, 0x4eea29a2 )
	ROM_LOAD( "shocking.u22", 0x080000, 0x040000, 0xd6db0388 )
	ROM_LOAD( "shocking.u23", 0x0c0000, 0x040000, 0x1fa33b2e )

	ROM_REGION( 0x080000 * 2, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "shocking.131", 0x000000, 0x080000, 0xd0a1bb8c )
	ROM_RELOAD(               0x080000, 0x080000             )

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1997, shocking, 0, shocking, shocking, 0, ROT0_16BIT, "Yun Sung", "Shocking", GAME_NO_COCKTAIL )
