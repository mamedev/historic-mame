/***************************************************************************

						  -= Puzzle Bancho (Fuuki) =-

					driver by	Luca Elia (eliavit@unina.it)


Main  CPU	:	MC68000
Sound Chips	:	YM2203 YM3812 M6295
Video Chips	:	FI-002K (208pin PQFP, GA2)
				FI-003K (208pin PQFP, GA3)
Other		:	Mitsubishi M60067-0901FP 452100 (208pin PQFP, GA1)

To Do:

- Raster effects (level 5 interrupt is used for that). They involve
  changing the *vertical* scroll value of the layers each scanline
  (when you are about to die, in the solo game).

- The scroll values are generally wrong when flip screen is on.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *pbancho_vram_0, *pbancho_vram_1;
extern data16_t *pbancho_vregs,  *pbancho_unknown, *pbancho_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( pbancho_vram_0_w );
WRITE16_HANDLER( pbancho_vram_1_w );

int  pbancho_vh_start(void);
void pbancho_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void pbancho_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

static WRITE16_HANDLER( pbancho_sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpu_set_nmi_line(1,PULSE_LINE);
		cpu_spinuntil_time(TIME_IN_USEC(50));	// Allow the other CPU to reply
	}
}

static MEMORY_READ16_START( pbancho_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x400000, 0x40ffff, MRA16_RAM					},	// RAM
	{ 0x500000, 0x507fff, MRA16_RAM					},	// Layers
	{ 0x600000, 0x601fff, spriteram16_r				},	// Sprites
	{ 0x608000, 0x609fff, spriteram16_r				},	// Sprites (? Mirror ?)
	{ 0x700000, 0x703fff, MRA16_RAM					},	// Palette
	{ 0x800000, 0x800001, input_port_0_word_r		},	// Buttons (Inputs)
	{ 0x810000, 0x810001, input_port_1_word_r		},	// P1 + P2
	{ 0x880000, 0x880001, input_port_2_word_r		},	// 2 x DSW
	{ 0x8c0000, 0x8c001f, MRA16_RAM					},	// Video Registers
MEMORY_END

static MEMORY_WRITE16_START( pbancho_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	// ROM
	{ 0x400000, 0x40ffff, MWA16_RAM							},	// RAM
	{ 0x500000, 0x501fff, pbancho_vram_1_w, &pbancho_vram_1	},	// Layers
	{ 0x502000, 0x503fff, pbancho_vram_0_w, &pbancho_vram_0	},	//
	{ 0x504000, 0x507fff, MWA16_RAM							},	//
	{ 0x600000, 0x601fff, spriteram16_w, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x608000, 0x609fff, spriteram16_w						},	// Sprites (? Mirror ?)
	{ 0x700000, 0x7017ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701800, 0x703fff, MWA16_RAM							},
	{ 0x8c0000, 0x8c001f, MWA16_RAM, &pbancho_vregs			},	// Video Registers
	{ 0x8a0000, 0x8a0001, pbancho_sound_command_w			},	// To Sound CPU
	{ 0x8d0000, 0x8d0003, MWA16_RAM, &pbancho_unknown		},	//
	{ 0x8e0000, 0x8e0001, MWA16_RAM, &pbancho_priority		},	//
MEMORY_END


/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

static WRITE_HANDLER( pbancho_sound_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x03;

	if (data & ~0x03) 	logerror("CPU #1 - PC %04X: unknown bank bits: %02X\n",cpu_get_pc(),data);

	RAM = &RAM[0x8000 * bank + 0x10000];
	cpu_setbank(1, RAM);
}

static MEMORY_READ_START( pbancho_sound_readmem )
	{ 0x0000, 0x5fff, MRA_ROM		},	// ROM
	{ 0x6000, 0x7fff, MRA_RAM		},	// RAM
	{ 0x8000, 0xffff, MRA_BANK1		},	// Banked ROM
MEMORY_END

static MEMORY_WRITE_START( pbancho_sound_writemem )
	{ 0x0000, 0x5fff, MWA_ROM		},	// ROM
	{ 0x6000, 0x7fff, MWA_RAM		},	// RAM
	{ 0x8000, 0xffff, MWA_ROM		},	// Banked ROM
MEMORY_END

static PORT_READ_START( pbancho_sound_readport )
	{ 0x11, 0x11, soundlatch_r				},	// From Main CPU
	{ 0x50, 0x50, YM3812_status_port_0_r	},	// YM3812
	{ 0x60, 0x60, OKIM6295_status_0_r		},	// M6295
PORT_END

static PORT_WRITE_START( pbancho_sound_writeport )
	{ 0x00, 0x00, pbancho_sound_rombank_w 	},	// ROM Bank
	{ 0x20, 0x20, IOWP_NOP					},	// ?
	{ 0x30, 0x30, IOWP_NOP					},	// ?
	{ 0x40, 0x40, YM2203_control_port_0_w	},	// YM2203
	{ 0x41, 0x41, YM2203_write_port_0_w		},
	{ 0x50, 0x50, YM3812_control_port_0_w	},	// YM3812
	{ 0x51, 0x51, YM3812_write_port_0_w		},
	{ 0x61, 0x61, OKIM6295_data_0_w			},	// M6295
PORT_END


/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( pbancho )

	PORT_START	// IN0 - $800000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - $810000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	// There's code that uses
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	// these unknown bits
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_START	// IN2 - $880000.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easiest" )	// 1
	PORT_DIPSETTING(      0x0010, "Easy"    )	// 2
	PORT_DIPSETTING(      0x001c, "Normal"  )	// 3
	PORT_DIPSETTING(      0x0018, "Hard"    )	// 4
	PORT_DIPSETTING(      0x0004, "Hardest" )	// 5
//	PORT_DIPSETTING(      0x0000, "Normal"  )	// 3
//	PORT_DIPSETTING(      0x000c, "Normal"  )	// 3
//	PORT_DIPSETTING(      0x0014, "Normal"  )	// 3
	PORT_DIPNAME( 0x0060, 0x0060, "Lives (Vs Mode)" )
	PORT_DIPSETTING(      0x0000, "1" )	// 1 1
	PORT_DIPSETTING(      0x0060, "2" )	// 2 3
//	PORT_DIPSETTING(      0x0020, "2" )	// 2 3
	PORT_DIPSETTING(      0x0040, "3" )	// 3 5
	PORT_DIPNAME( 0x0080, 0x0080, "? Senin Mode ?" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Allow Game Selection" )	// "unused" in the manual?
	PORT_DIPSETTING(      0x0200, DEF_STR( Yes ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )	// Why cripple the game!?
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/

/* 16x16x4 */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{	2*4,3*4,   0*4,1*4,   6*4,7*4, 4*4,5*4,
		10*4,11*4, 8*4,9*4, 14*4,15*4, 12*4,13*4	},
	{ STEP16(0,16*4) },
	16*16*4
};

/* 16x16x8 */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(0,1) },
	{	2*4,3*4,   0*4,1*4,   6*4,7*4, 4*4,5*4,
		10*4,11*4, 8*4,9*4, 14*4,15*4, 12*4,13*4	},
	{ STEP16(0,16*4) },
	16*16*4
};

static struct GfxDecodeInfo pbancho_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x4400, 0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x0400, 0x40 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_16x16x4, 0x0000, 0x40 }, // [2] Layer 1
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static void soundirq(int state)
{
	cpu_set_irq_line(1, 0, state);
}

static struct YM2203interface pbancho_ym2203_intf =
{
	1,
	4000000,		/* ? */
	{ YM2203_VOL(15,15) },
	{ 0 },			/* Port A Read  */
	{ 0 },			/* Port B Read  */
	{ 0 },			/* Port A Write */
	{ 0 },			/* Port B Write */
	{ 0 }			/* IRQ handler  */
};

static struct YM3812interface pbancho_ym3812_intf =
{
	1,
	4000000,		/* ? */
	{ 15 },
	{ soundirq },	/* IRQ Line */
};

static struct OKIM6295interface pbancho_m6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 85 }
};

/*
	- Interrupts -

	Lev 1:	Sets bit 5 of $400010. Prints "credit .." with sprites.
	Lev 2:	Sets bit 7 of $400010. Clears $8c0012.
			It seems unused by the game.
	Lev 3:	VBlank.
	Lev 5:	Programmable to happen on a raster line. Used to do raster
			effects when you are about to die

*/
#define INTERRUPTS_NUM	(256)
int pbancho_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 1:	return 1;
		case 0:	return 3;	// VBlank
		default:
			if ( (pbancho_vregs[0x1c/2] & 0xff) == (INTERRUPTS_NUM-1 - cpu_getiloops()) )
				return 5;
			else
				return 0;
	}
}

static const struct MachineDriver machine_driver_pbancho =
{
	{
		{
			CPU_M68000,
			16000000,
			pbancho_readmem, pbancho_writemem,0,0,
			pbancho_interrupt, INTERRUPTS_NUM
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* ? */
			pbancho_sound_readmem,  pbancho_sound_writemem,
			pbancho_sound_readport, pbancho_sound_writeport,
			ignore_interrupt, 1	/* IRQ by YM3812; NMI by main CPU */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 256, { 0, 320-1, 0, 256-16-1 },
	pbancho_gfxdecodeinfo,
	0x400*3, 0x40*16 + 0x40*256 + 0x40*16,	/* Sprites, 8 bit layer, 4 bit layer */
	pbancho_vh_init_palette,				/* For the 8 bit layer */
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pbancho_vh_start,
	0,
	pbancho_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_YM2203,   &pbancho_ym2203_intf },
		{ SOUND_YM3812,   &pbancho_ym3812_intf },
		{ SOUND_OKIM6295, &pbancho_m6295_intf  }
	},
};


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

							Gyakuten!! Puzzle Bancho

(c)1996 Fuuki
FG-1C AI AM-2

CPU  : TMP68HC000P-16
Sound: Z80 YM2203 YM3812 M6295
OSC  : 32.00000MHz(OSC1) 28.64000MHz(OSC2) 12.000MHz(Xtal1)

ROMs:
rom2.no1 - Main program (even)(27c4000)
rom1.no2 - Main program (odd) (27c4000)

rom25.no3 - Samples (27c2001)
rom24.no4 - Sound program (27c010)

rom11.61 - Graphics (Mask, read as 27c160)
rom15.59 |
rom20.58 |
rom3.60  /

Custom chips:
FI-002K (208pin PQFP, GA2)
FI-003K (208pin PQFP, GA3)

Others:
Mitsubishi M60067-0901FP 452100 (208pin PQFP, GA1)

***************************************************************************/

ROM_START( pbancho )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom2.no1", 0x000000, 0x080000, 0x1b4fd178 )	// 1xxxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "rom1.no2", 0x000001, 0x080000, 0x9cf510a5 )	// 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom24.no4", 0x00000, 0x08000, 0xdfbfdb81 )
	ROM_CONTINUE(          0x10000, 0x18000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x4 Sprites */
	ROM_LOAD( "rom20.58", 0x000000, 0x200000, 0x4dad0a2e )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 Tiles */
	ROM_LOAD( "rom11.61", 0x000000, 0x200000, 0x7f1213b9 )
	ROM_LOAD( "rom15.59", 0x200000, 0x200000, 0xb83dcb70 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "rom3.60",  0x000000, 0x200000, 0xa50a3c1b )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom25.no3", 0x000000, 0x040000, 0xa7bfb5ea )

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1996, pbancho, 0, pbancho, pbancho, 0, ROT0_16BIT, "Fuuki", "Gyakuten!! Puzzle Bancho (Japan)", GAME_IMPERFECT_GRAPHICS )
