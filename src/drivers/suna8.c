/***************************************************************************

							-=  SunA 8 Bit Games =-

					driver by	Luca Elia (eliavit@unina.it)


Main  CPU:		Encrypted Z80 (Epoxy Module)
Sound CPU:		Z80 [Music]  +  Z80 [4 Bit PCM, Optional]
Sound Chips:	AY8910  +  YM3812/YM2203  + DAC x 4 [Optional]


---------------------------------------------------------------------------
Year + Game         Game     PCB         Epoxy CPU    Notes
---------------------------------------------------------------------------
88  Hard Head       KRB-14   60138-0083  S562008      Encryption + Protection
88  Rough Ranger	K030087  ?           S562008
91  Hard Head 2     ?        ?           T568009      Not Working (Encryption)
92  Brick Zone      ?        ?           Yes          Not Working (Encryption)
---------------------------------------------------------------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

extern int suna8_gfx_type; /* specifies format of text layer */

/* Functions defined in vidhrdw: */

WRITE_HANDLER( hardhead_spriteram_w );	// for debug

int  suna8_vh_start(void);
void suna8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


								Main CPU


***************************************************************************/

/***************************************************************************
						Hard Head - Inputs handling
***************************************************************************/

static data8_t *hardhead_ip;

static READ_HANDLER( hardhead_ip_r )
{
	switch (*hardhead_ip)
	{
		case 0:	return readinputport(0);
		case 1:	return readinputport(1);
		case 2:	return readinputport(2);
		case 3:	return readinputport(3);
		default:
			logerror("CPU #0 - PC %04X: Unknown IP read: %02X\n",cpu_get_pc(),hardhead_ip);
			return 0xff;
	}
}

/***************************************************************************
							Hard Head - Protection
***************************************************************************/

static data8_t protection_val;

static READ_HANDLER( hardhead_protection_r )
{
	if (protection_val & 0x80)
		return	((~offset & 0x20)			?	0x20 : 0) |
				((protection_val & 0x04)	?	0x80 : 0) |
				((protection_val & 0x01)	?	0x04 : 0);
	else
		return	((~offset & 0x20)					?	0x20 : 0) |
				(((offset ^ protection_val) & 0x01)	?	0x84 : 0);
}

static WRITE_HANDLER( hardhead_protection_w )
{
	if (data & 0x80)	protection_val = data;
	else				protection_val = offset & 1;
}

/***************************************************************************
							Hard Head - Memory Map
***************************************************************************/

/*
	7654 ----
	---- 3210	ROM Bank
*/
static WRITE_HANDLER( hardhead_bankswitch_w )
{
	UINT8 *pMem = memory_region(REGION_CPU1);
/*
	int bank = data & 0x0f;

	if (data & ~0x0f) 	logerror("CPU #1 - PC %04X: unknown bank bits: %02X\n",cpu_get_pc(),data);

	if (bank < 2)	pMem = &pMem[0x4000 * bank];
	else			pMem = &pMem[0x4000 * (bank-2) + 0x10000];
	cpu_setbank(1, pMem);
*/
	int offs;
	offs = 0x10000 + 0x4000*pMem[0xda80];
	cpu_setbank( 1,&pMem[offs] );
}

static MEMORY_READ_START( hardhead_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1				},	// Banked ROM
	{ 0xc000, 0xd9ff, MRA_RAM				},	// RAM
	{ 0xda00, 0xda00, hardhead_ip_r			},	// Input Ports
	{ 0xda80, 0xda80, soundlatch2_r			},	// From Sound CPU
	{ 0xdd80, 0xddff, hardhead_protection_r	},	// Protection
	{ 0xe000, 0xffff, MRA_RAM				},	// Sprites
MEMORY_END

static MEMORY_WRITE_START( hardhead_writemem )
	{ 0x0000, 0x7fff, MWA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM				},	// Banked ROM
	{ 0xc000, 0xd7ff, MWA_RAM				},	// RAM
	{ 0xd800, 0xd9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xda00, 0xda00, MWA_RAM, &hardhead_ip	},	// Input Port Select
	{ 0xda80, 0xda80, MWA_RAM },//hardhead_bankswitch_w	},	// ROM Banking
	{ 0xdb00, 0xdb00, soundlatch_w			},	// To Sound CPU
	{ 0xdb80, 0xdb80, MWA_NOP				},	// Flip Screen (bit 5) + ?
//	{ 0xdc00, 0xdc00, MWA_NOP				},	// <- R
	{ 0xdc00, 0xdc00, hardhead_bankswitch_w },
	{ 0xdc80, 0xdc80, MWA_NOP				},	// <- R
	{ 0xdd00, 0xdd00, MWA_NOP				},	// <- R
	{ 0xdd80, 0xddff, hardhead_protection_w	},	// Protection
	{ 0xe000, 0xffff, hardhead_spriteram_w, &spriteram	},	// Sprites
MEMORY_END

static PORT_READ_START( hardhead_readport )
	{ 0x00, 0x00, IORP_NOP	},	// ? IRQ Ack
PORT_END

static PORT_WRITE_START( hardhead_writeport )
PORT_END


/***************************************************************************
								Rough Ranger
***************************************************************************/

/*
	76-- ----	Coin Lockout
	--5- ----	Flip Screen
	---4 ----	ROM Bank
	---- 3---
	---- -210	ROM Bank
*/
static WRITE_HANDLER( rranger_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x07;
	if ((~data & 0x10) && (bank >= 4))	bank += 4;

	if (data & ~0xf7) 	logerror("CPU #1 - PC %04X: unknown bank bits: %02X\n",cpu_get_pc(),data);

	RAM = &RAM[0x4000 * bank + 0x10000];

	cpu_setbank(1, RAM);

	flip_screen_set(    data & 0x20);
	coin_lockout_w ( 0,	data & 0x40);
	coin_lockout_w ( 1,	data & 0x80);
}

/*
	7--- ----	1 -> Garbled title (another romset?)
	-654 ----
	---- 3---	1 -> No sound (soundlatch full?)
	---- -2--
	---- --1-	1 -> Interlude screens
	---- ---0
*/
static READ_HANDLER( rranger_soundstatus_r )
{
	return 0x02;
}

static MEMORY_READ_START( rranger_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1				},	// Banked ROM
	{ 0xc000, 0xc000, watchdog_reset_r		},	// Watchdog (Tested!)
	{ 0xc002, 0xc002, input_port_0_r		},	// P1 (Inputs)
	{ 0xc003, 0xc003, input_port_1_r		},	// P2
	{ 0xc004, 0xc004, rranger_soundstatus_r	},	// Latch Status?
	{ 0xc200, 0xc200, MRA_NOP				},	// Protection?
	{ 0xc280, 0xc280, input_port_2_r		},	// DSW 1
	{ 0xc2c0, 0xc2c0, input_port_3_r		},	// DSW 2
	{ 0xc600, 0xdfff, MRA_RAM				},	// RAM
	{ 0xe000, 0xffff, MRA_RAM				},	// Sprites
MEMORY_END

static MEMORY_WRITE_START( rranger_writemem )
	{ 0x0000, 0x7fff, MWA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM				},	// Banked ROM
	{ 0xc000, 0xc000, soundlatch_w			},	// To Sound CPU
	{ 0xc002, 0xc002, rranger_bankswitch_w	},	// ROM Banking
	{ 0xc200, 0xc200, MWA_NOP				},	// Protection?
	{ 0xc280, 0xc280, MWA_NOP				},	// ? NMI Ack
	{ 0xc600, 0xc7ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xc800, 0xdfff, MWA_RAM				},	// RAM
	{ 0xe000, 0xffff, hardhead_spriteram_w, &spriteram	},	// Sprites
MEMORY_END

static PORT_READ_START( rranger_readport )
	{ 0x00, 0x00, IORP_NOP	},	// ? IRQ Ack
PORT_END

static PORT_WRITE_START( rranger_writeport )
PORT_END

/***************************************************************************
								Brick Zone
***************************************************************************/

						/* --- PRELIMINARY --- */

static MEMORY_READ_START( brickzn_readmem )
	{ 0x0000, 0xdfff, MRA_ROM	},	// ROM
	{ 0xe000, 0xffff, MRA_RAM	},	// Sprites
MEMORY_END

static MEMORY_WRITE_START( brickzn_writemem )
	{ 0x0000, 0xdfff, MWA_ROM			},	// ROM
	{ 0xe000, 0xffff, hardhead_spriteram_w, &spriteram	},	// Sprites
MEMORY_END

static PORT_READ_START( brickzn_readport )
PORT_END

static PORT_WRITE_START( brickzn_writeport )
PORT_END


/***************************************************************************
								Hard Head 2
***************************************************************************/

						/* --- PRELIMINARY --- */

static MEMORY_READ_START( hardhea2_readmem )
	{ 0x0000, 0xdfff, MRA_ROM	},	// ROM
	{ 0xe000, 0xffff, MRA_RAM	},	// Sprites
MEMORY_END

static MEMORY_WRITE_START( hardhea2_writemem )
	{ 0x0000, 0xdfff, MWA_ROM			},	// ROM
	{ 0xe000, 0xffff, hardhead_spriteram_w, &spriteram	},	// Sprites
MEMORY_END

static PORT_READ_START( hardhea2_readport )
PORT_END

static PORT_WRITE_START( hardhea2_writeport )
PORT_END


/***************************************************************************


								Sound CPU(s)


***************************************************************************/

/***************************************************************************
								Hard Head
***************************************************************************/

static MEMORY_READ_START( hardhead_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	// ROM
	{ 0xc000, 0xc7ff, MRA_RAM					},	// RAM
	{ 0xc800, 0xc800, YM3812_status_port_0_r 	},	// ? unsure
	{ 0xd800, 0xd800, soundlatch_r				},	// From Main CPU
MEMORY_END

static MEMORY_WRITE_START( hardhead_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	// ROM
	{ 0xc000, 0xc7ff, MWA_RAM					},	// RAM
	{ 0xd000, 0xd000, soundlatch2_w				},	// To PCM CPU??
	{ 0xa000, 0xa000, YM3812_control_port_0_w	},	// YM3812
	{ 0xa001, 0xa001, YM3812_write_port_0_w		},
	{ 0xa002, 0xa002, AY8910_control_port_0_w	},	// AY8910
	{ 0xa003, 0xa003, AY8910_write_port_0_w		},
MEMORY_END

static PORT_READ_START( hardhead_sound_readport )
	{ 0x01, 0x01, IORP_NOP	},	// ? IRQ Ack
PORT_END

static PORT_WRITE_START( hardhead_sound_writeport )
PORT_END



/***************************************************************************
								Rough Ranger
***************************************************************************/

static MEMORY_READ_START( rranger_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	// ROM
	{ 0xc000, 0xc7ff, MRA_RAM					},	// RAM
	{ 0xd800, 0xd800, soundlatch_r				},	// From Main CPU
MEMORY_END

static MEMORY_WRITE_START( rranger_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	// ROM
	{ 0xc000, 0xc7ff, MWA_RAM					},	// RAM
	{ 0xd000, 0xd000, soundlatch2_w				},	//
	{ 0xa000, 0xa000, YM2203_control_port_0_w	},	// YM2203
	{ 0xa001, 0xa001, YM2203_write_port_0_w		},
	{ 0xa002, 0xa002, YM2203_control_port_1_w	},	// AY8910
	{ 0xa003, 0xa003, YM2203_write_port_1_w		},
MEMORY_END

static PORT_READ_START( rranger_sound_readport )
PORT_END

static PORT_WRITE_START( rranger_sound_writeport )
PORT_END


/***************************************************************************
								Brick Zone
***************************************************************************/

static MEMORY_READ_START( brickzn_sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM				},	// ROM
	{ 0xe000, 0xe7ff, MRA_RAM				},	// RAM
MEMORY_END

static MEMORY_WRITE_START( brickzn_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM					},	// ROM
	{ 0xc000, 0xc000, YM3812_control_port_0_w	},	// YM3812
	{ 0xc001, 0xc001, YM3812_write_port_0_w		},
	{ 0xc002, 0xc002, AY8910_control_port_0_w	},	// AY8910
	{ 0xc003, 0xc003, AY8910_write_port_0_w		},
	{ 0xe000, 0xe7ff, MWA_RAM					},	// RAM
	{ 0xf000, 0xf000, soundlatch2_w				},	// To PCM CPU
MEMORY_END

static PORT_READ_START( brickzn_sound_readport )
PORT_END

static PORT_WRITE_START( brickzn_sound_writeport )
PORT_END


/* PCM Z80 , 4 DACs (4 bits per sample), NO RAM !! */

static MEMORY_READ_START( brickzn_pcm_readmem )
	{ 0x0000, 0xffff, MRA_ROM	},	// ROM
MEMORY_END
static MEMORY_WRITE_START( brickzn_pcm_writemem )
	{ 0x0000, 0xffff, MWA_ROM	},	// ROM
MEMORY_END


static WRITE_HANDLER( brickzn_pcm_w )
{
	DAC_data_w( offset & 3, (data & 0xf) * 0x11 );
}

static PORT_READ_START( brickzn_pcm_readport )
	{ 0x00, 0x00, soundlatch2_r		},	// From Sound CPU
PORT_END
static PORT_WRITE_START( brickzn_pcm_writeport )
	{ 0x00, 0x03, brickzn_pcm_w			},	// 4 x DAC
PORT_END



/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Hard Head
***************************************************************************/

INPUT_PORTS_START( hardhead )

	PORT_START	// IN0 - Player 1 - $da00.b (ip = 0)
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1  )

	PORT_START	// IN1 - Player 2 - $da00.b (ip = 1)
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_START	// IN2 - DSW 1 - $da00.b (ip = 2)
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0e, "No Bonus" )
	PORT_DIPSETTING(    0x0c, "10k" )
	PORT_DIPSETTING(    0x0a, "20k" )
	PORT_DIPSETTING(    0x08, "50k" )
	PORT_DIPSETTING(    0x06, "50k, every 50k" )
	PORT_DIPSETTING(    0x04, "100k, every 50k" )
	PORT_DIPSETTING(    0x02, "100k, every 100k" )
	PORT_DIPSETTING(    0x00, "200k, every 100k" )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x80, 0x80, "Game Mode" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Power Gauge Stable" )

	PORT_START	// IN3 - DSW 2 - $da00.b (ip = 3)
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Play Together" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "Easiest" )
	PORT_DIPSETTING(    0xc0, "Very Easy" )
	PORT_DIPSETTING(    0xa0, "Easy" )
	PORT_DIPSETTING(    0x80, "Moderate" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Harder" )
	PORT_DIPSETTING(    0x20, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************
								Rough Ranger
***************************************************************************/

INPUT_PORTS_START( rranger )

	PORT_START	// IN0 - Player 1 - $c002.b
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1  )

	PORT_START	// IN1 - Player 2 - $c003.b
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_START	// IN2 - DSW 1 - $c280.b
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x38, "No Bonus" )
	PORT_DIPSETTING(    0x30, "10K" )
	PORT_DIPSETTING(    0x28, "30K" )
	PORT_DIPSETTING(    0x20, "50K" )
	PORT_DIPSETTING(    0x18, "50K, Every 50K" )
	PORT_DIPSETTING(    0x10, "100K, Every 50K" )
	PORT_DIPSETTING(    0x08, "100K, Every 100K" )
	PORT_DIPSETTING(    0x00, "100K, Every 200K" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0x40, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START	// IN3 - DSW 2 - $c2c0.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Play Together" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
								Brick Zone
***************************************************************************/

						/* --- PRELIMINARY --- */

INPUT_PORTS_START( brickzn )

	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1  )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_START	// IN2 - DSW 1
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN3 - DSW 2
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
								Hard Head 2
***************************************************************************/

						/* --- PRELIMINARY --- */

INPUT_PORTS_START( hardhea2 )

	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1  )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_START	// IN2 - DSW 1
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN3 - DSW 2
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************


								Graphics Layouts


***************************************************************************/

/* 8x8x4 tiles (2 bitplanes per ROM) */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2) + 0, RGN_FRAC(1,2) + 4, 0, 4 },
	{ 3,2,1,0, 11,10,9,8},
	{ STEP8(0,16) },
	8*8*2
};

static struct GfxDecodeInfo suna8_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0, 16 }, // [0] Sprites
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/


/***************************************************************************
								Hard Head
***************************************************************************/

static struct AY8910interface hardhead_ay8910_interface =
{
	1,
	1500000,	/* ? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface hardhead_ym3812_interface =
{
	1,
	4000000,	/* ? */
	{ 50 },
	{  0 },
};

static const struct MachineDriver machine_driver_hardhead =
{
	{
		{
			CPU_Z80,
			4000000,					/* ? */
			hardhead_readmem, hardhead_writemem,
			hardhead_readport,hardhead_writeport,
			interrupt, 1	/* No NMI */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			hardhead_sound_readmem, hardhead_sound_writemem,
			hardhead_sound_readport,hardhead_sound_writeport,
			interrupt, 4	/* No NMI */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	suna8_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna8_vh_start,
	0,
	suna8_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM3812,	&hardhead_ym3812_interface	},
		{	SOUND_AY8910,	&hardhead_ay8910_interface	}
	}
};



/***************************************************************************
								Rough Ranger
***************************************************************************/

/* 2203 + 8910 */
static struct YM2203interface rranger_ym2203_interface =
{
	2,
	3000000,	/* ? */
	{ YM2203_VOL(50,50), YM2203_VOL(50,50) },
	{ 0,0 },	/* Port A Read  */
	{ 0,0 },	/* Port B Read  */
	{ 0,0 },	/* Port A Write */
	{ 0,0 },	/* Port B Write */
	{ 0,0 }		/* IRQ handler  */
};

static const struct MachineDriver machine_driver_rranger =
{
	{
		{
			CPU_Z80,
			4000000,					/* ? */
			rranger_readmem,  rranger_writemem,
			rranger_readport, rranger_writeport,
			interrupt, 1	/* IRQ & NMI ! */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			rranger_sound_readmem,  rranger_sound_writemem,
			rranger_sound_readport, rranger_sound_writeport,
			interrupt, 4	/* NMI = retn */
		}

		/* Is there a 3rd CPU for PCM here ? */

	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	suna8_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna8_vh_start,
	0,
	suna8_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2203,	&rranger_ym2203_interface	},
	}
};




/***************************************************************************
								Brick Zone
***************************************************************************/

static struct AY8910interface brickzn_ay8910_interface =
{
	1,
	1500000,	/* ? */
	{ 33 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface brickzn_ym3812_interface =
{
	1,
	4000000,	/* ? */
	{ 33 },
	{  0 },
};

static struct DACinterface brickzn_dac_interface =
{
	4,
	{	MIXER(33,MIXER_PAN_LEFT), MIXER(33,MIXER_PAN_RIGHT),
		MIXER(33,MIXER_PAN_LEFT), MIXER(33,MIXER_PAN_RIGHT)	}
};

static const struct MachineDriver machine_driver_brickzn =
{
	{
		{
			CPU_Z80,
			4000000,					/* ? */
			brickzn_readmem, brickzn_writemem,
			brickzn_readport,brickzn_writeport,
			interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			brickzn_sound_readmem, brickzn_sound_writemem,
			brickzn_sound_readport,brickzn_sound_writeport,
			interrupt, 1	/* No NMI */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			brickzn_pcm_readmem, brickzn_pcm_writemem,
			brickzn_pcm_readport,brickzn_pcm_writeport,
			ignore_interrupt, 1	/* No interrupts */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	suna8_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna8_vh_start,
	0,
	suna8_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM3812,	&brickzn_ym3812_interface	},
		{	SOUND_AY8910,	&brickzn_ay8910_interface	},
		{	SOUND_DAC,		&brickzn_dac_interface		},
	}
};


/***************************************************************************
								Hard Head 2
***************************************************************************/

static const struct MachineDriver machine_driver_hardhea2 =
{
	{
		{
			CPU_Z80,	/* SUNA T568009 */
			4000000,					/* ? */
			hardhea2_readmem, hardhea2_writemem,
			hardhea2_readport,hardhea2_writeport,
			interrupt, 1
		},
		/* The sound section is identical to brickzn's */
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			brickzn_sound_readmem, brickzn_sound_writemem,
			brickzn_sound_readport,brickzn_sound_writeport,
			interrupt, 1	/* No NMI */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,					/* ? */
			brickzn_pcm_readmem, brickzn_pcm_writemem,
			brickzn_pcm_readport,brickzn_pcm_writeport,
			ignore_interrupt, 1	/* No interrupts */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	suna8_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna8_vh_start,
	0,
	suna8_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM3812,	&brickzn_ym3812_interface	},
		{	SOUND_AY8910,	&brickzn_ay8910_interface	},
		{	SOUND_DAC,		&brickzn_dac_interface		},
	}
};


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

									Hard Head

Location  Type    File ID  Checksum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
L5       27C256     P1       1327   [ main program ]
K5       27C256     P2       50B1   [ main program ]
J5       27C256     P3       CF73   [ main program ]
I5       27C256     P4       DE86   [ main program ]
D5       27C256     P5       94D1   [  background  ]
A5       27C256     P6       C3C7   [ motion obj.  ]
L7       27C256     P7       A7B8   [ main program ]
K7       27C256     P8       5E53   [ main program ]
J7       27C256     P9       35FC   [ main program ]
I7       27C256     P10      8F9A   [ main program ]
D7       27C256     P11      931C   [  background  ]
A7       27C256     P12      2EED   [ motion obj.  ]
H9       27C256     P13      5CD2   [ snd program  ]
M9       27C256     P14      5576   [  sound data  ]

Note:  Game   No. KRB-14
       PCB    No. 60138-0083

Main processor  -  Custom security block (battery backed) CPU No. S562008

Sound processor -  Z80
                -  YM3812
                -  AY-3-8910

***************************************************************************/

ROM_START( hardhead )
	ROM_REGION( 0x48000, REGION_CPU1, 0 ) /* Main Z80 Code */
	ROM_LOAD( "p1",  0x00000, 0x8000, 0xc6147926 )	// 1988,9,14
	ROM_LOAD( "p2",			0x10000, 0x8000, 0xfaa2cf9a )
	ROM_LOAD( "p3",			0x18000, 0x8000, 0x3d24755e )
	ROM_LOAD( "p4",			0x20000, 0x8000, 0x0241ac79 )
	ROM_LOAD( "p7",			0x28000, 0x8000, 0xbeba8313 )
	ROM_LOAD( "p8",			0x30000, 0x8000, 0x211a9342 )
	ROM_LOAD( "p9",			0x38000, 0x8000, 0x2ad430c4 )
	ROM_LOAD( "p10",		0x40000, 0x8000, 0xb6894517 )

//	ROM_LOAD( "p3",  0x10000, 0x8000, 0x3d24755e )
//	ROM_LOAD( "p4",  0x18000, 0x8000, 0x0241ac79 )
//	ROM_LOAD( "p7",  0x20000, 0x8000, 0xbeba8313 )	// FIXED BITS (xxxxxxxx0xxxxxxx)
//	ROM_LOAD( "p8",  0x28000, 0x8000, 0x211a9342 )
//	ROM_LOAD( "p9",  0x30000, 0x8000, 0x2ad430c4 )
//	ROM_LOAD( "p10", 0x38000, 0x8000, 0xb6894517 )
//	ROM_LOAD( "p2",  0x40000, 0x8000, 0xfaa2cf9a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "p13", 0x0000, 0x8000, 0x493c0b41 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT )	/* tiles */
	ROM_LOAD( "p5",  0x00000, 0x8000, 0xe9aa6fba )
	ROM_RELOAD(      0x08000, 0x8000             )
	ROM_LOAD( "p6",  0x10000, 0x8000, 0x15d5f5dd )
	ROM_RELOAD(      0x18000, 0x8000             )
	ROM_LOAD( "p11", 0x20000, 0x8000, 0x055f4c29 )
	ROM_RELOAD(      0x28000, 0x8000             )
	ROM_LOAD( "p12", 0x30000, 0x8000, 0x9582e6db )
	ROM_RELOAD(      0x38000, 0x8000             )

	ROM_REGION( 0x8000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "p14", 0x0000, 0x8000, 0x41314ac1 )
ROM_END

ROM_START( hardheab )
	ROM_REGION( 0x48000, REGION_CPU1, 0 ) /* Main CPU Z80 code */
	ROM_LOAD( "9_1_6l.rom",	0x00000, 0x8000, 0x750e6aee )
	ROM_LOAD( "p2",			0x10000, 0x8000, 0xfaa2cf9a )
	ROM_LOAD( "p3",			0x18000, 0x8000, 0x3d24755e )
	ROM_LOAD( "p4",			0x20000, 0x8000, 0x0241ac79 )
	ROM_LOAD( "p7",			0x28000, 0x8000, 0xbeba8313 )
	ROM_LOAD( "p8",			0x30000, 0x8000, 0x211a9342 )
	ROM_LOAD( "p9",			0x38000, 0x8000, 0x2ad430c4 )
	ROM_LOAD( "p10",		0x40000, 0x8000, 0xb6894517 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound Z80 Code */
	ROM_LOAD( "p13", 0x0000, 0x8000, 0x493c0b41 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "p5",  0x00000, 0x8000, 0xe9aa6fba )
	ROM_RELOAD(      0x08000, 0x8000             )
	ROM_LOAD( "p6",  0x10000, 0x8000, 0x15d5f5dd )
	ROM_RELOAD(      0x18000, 0x8000             )
	ROM_LOAD( "p11", 0x20000, 0x8000, 0x055f4c29 )
	ROM_RELOAD(      0x28000, 0x8000             )
	ROM_LOAD( "p12", 0x30000, 0x8000, 0x9582e6db )
	ROM_RELOAD(      0x38000, 0x8000             )

	ROM_REGION( 0x8000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "1_14_11m.rom",	0x0000, 0x8000, 0x41314ac1 )
ROM_END

/***************************************************************************

								Rough Ranger (v2.0)

(SunA 1988)
K030087

 24MHz    6  7  8  9  - 10 11 12 13   sw1  sw2



   6264
   5    6116
   4    6116                         6116
   3    6116                         14
   2    6116                         Z80A
   1                        6116     8910
                 6116  6116          2203
                                     15
 Epoxy CPU
                            6116

***************************************************************************/

ROM_START( rranger )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "1",  0x00000, 0x8000, 0x4fb4f096 )	// V 2.0 1988,4,15
	ROM_LOAD( "2",  0x10000, 0x8000, 0xff65af29 )
	ROM_LOAD( "3",  0x18000, 0x8000, 0x64e09436 )
	ROM_LOAD( "r4", 0x30000, 0x8000, 0x4346fae6 )
	ROM_CONTINUE(   0x20000, 0x8000             )
	ROM_LOAD( "r5", 0x38000, 0x8000, 0x6a7ca1c3 )
	ROM_CONTINUE(   0x28000, 0x8000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "14", 0x0000, 0x8000, 0x11c83aa1 )

	ROM_REGION( 0x8000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "15", 0x0000, 0x8000, 0x28c2c87e )
	/* Is there code for a 3rd CPU for PCM in the first (missing) half of this ROM ? */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT )	/* tiles */
	ROM_LOAD( "6",  0x00000, 0x8000, 0x57543643 )
	ROM_LOAD( "7",  0x08000, 0x8000, 0x9f35dbfa )
	ROM_LOAD( "8",  0x10000, 0x8000, 0xf400db89 )
	ROM_LOAD( "9",  0x18000, 0x8000, 0xfa2a11ea )
	ROM_LOAD( "10", 0x20000, 0x8000, 0x42c4fdbf )
	ROM_LOAD( "11", 0x28000, 0x8000, 0x19037a7b )
	ROM_LOAD( "12", 0x30000, 0x8000, 0xc59c0ec7 )
	ROM_LOAD( "13", 0x38000, 0x8000, 0x9809fee8 )
ROM_END

ROM_START( sranger )
	ROM_REGION( 0x48000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "r1",	0x00000, 0x8000, 0x4eef1ede )
	ROM_LOAD( "r2",	0x10000, 0x8000, 0xff65af29 )
	ROM_LOAD( "r3",	0x18000, 0x8000, 0x64e09436 )
	ROM_LOAD( "r4",	0x30000, 0x8000, 0x4346fae6 )
	ROM_CONTINUE(   0x20000, 0x8000             )
	ROM_LOAD( "r5",	0x38000, 0x8000, 0x6a7ca1c3 )
	ROM_CONTINUE(   0x28000, 0x8000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "r14",	0x00000, 0x8000, 0x11c83aa1 ) /* sound program */

	ROM_REGION( 0x08000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "r15",	0x00000, 0x8000, 0x28c2c87e ) /* sound data */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "r6",		0x00000, 0x8000, 0x4f11fef3 )
	ROM_LOAD( "r7",		0x08000, 0x8000, 0x9f35dbfa )
	ROM_LOAD( "r8",		0x10000, 0x8000, 0xf400db89 )
	ROM_LOAD( "r9",		0x18000, 0x8000, 0xfa2a11ea )
	ROM_LOAD( "r10",	0x20000, 0x8000, 0x1b204d6b )
	ROM_LOAD( "r11",	0x28000, 0x8000, 0x19037a7b )
	ROM_LOAD( "r12",	0x30000, 0x8000, 0xc59c0ec7 )
	ROM_LOAD( "r13",	0x38000, 0x8000, 0x9809fee8 )
ROM_END

ROM_START( srangerb )
	ROM_REGION( 0x48000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "r1bt",	0x00000, 0x08000, 0x40635e7c )
	ROM_LOAD( "r2",		0x10000, 0x08000, 0xff65af29 )
	ROM_LOAD( "r3",		0x18000, 0x08000, 0x64e09436 )
	ROM_LOAD( "r4",	0x30000, 0x8000, 0x4346fae6 )
	ROM_CONTINUE(   0x20000, 0x8000             )
	ROM_LOAD( "r5",	0x38000, 0x8000, 0x6a7ca1c3 )
	ROM_CONTINUE(   0x28000, 0x8000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "r14",	0x00000, 0x8000, 0x11c83aa1 ) /* sound program */

	ROM_REGION( 0x08000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "r15",	0x00000, 0x8000, 0x28c2c87e ) /* sound data */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "r6",		0x00000, 0x8000, 0x4f11fef3 )
	ROM_LOAD( "r7",		0x08000, 0x8000, 0x9f35dbfa )
	ROM_LOAD( "r8",		0x10000, 0x8000, 0xf400db89 )
	ROM_LOAD( "r9",		0x18000, 0x8000, 0xfa2a11ea )
	ROM_LOAD( "r10",	0x20000, 0x8000, 0x1b204d6b )
	ROM_LOAD( "r11",	0x28000, 0x8000, 0x19037a7b )
	ROM_LOAD( "r12",	0x30000, 0x8000, 0xc59c0ec7 )
	ROM_LOAD( "r13",	0x38000, 0x8000, 0x9809fee8 )
ROM_END

/***************************************************************************

								Brick Zone (v5.0)

SUNA ELECTRONICS IND CO., LTD

CPU Z0840006PSC (ZILOG)

Chrystal : 24.000 Mhz

Sound CPU : Z084006PSC (ZILOG) + AY3-8910A

Warning ! This game has a 'SUNA' protection block :-(

***************************************************************************/

ROM_START( brickzn )
	ROM_REGION( 0x50000 * 2, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "brickzon.009", 0x00000, 0x08000, 0x1ea68dea )	// V5.0 1992,3,3
	ROM_RELOAD(               0x50000, 0x08000             )
	ROM_LOAD( "brickzon.008", 0x10000, 0x20000, 0xc61540ba )
	ROM_RELOAD(               0x60000, 0x20000             )
	ROM_LOAD( "brickzon.007", 0x30000, 0x20000, 0xceed12f1 )
	ROM_RELOAD(               0x80000, 0x20000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Music Z80 Code */
	ROM_LOAD( "brickzon.010", 0x00000, 0x10000, 0x4eba8178 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* PCM Z80 Code */
	ROM_LOAD( "brickzon.011", 0x00000, 0x10000, 0x6c54161a )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT )	/* tiles */
	ROM_LOAD( "brickzon.001", 0x00000, 0x20000, 0x6970ada9 )
	ROM_LOAD( "brickzon.002", 0x20000, 0x20000, 0x241f0659 )
	ROM_LOAD( "brickzon.003", 0x40000, 0x20000, 0x2e4f194b )
	ROM_LOAD( "brickzon.004", 0x60000, 0x20000, 0x2be5f335 )
	ROM_LOAD( "brickzon.005", 0x80000, 0x20000, 0x118f8392 )
	ROM_LOAD( "brickzon.006", 0xa0000, 0x20000, 0xbbf31081 )
ROM_END

/***************************************************************************

								Brick Zone (v3.0)

(c) 1992 Suna Electronics

2 * Z80B

AY-3-8910
YM3812

24 MHz crystal

Large epoxy(?) module near the cpu's.

***************************************************************************/

ROM_START( brickzn3 )
	ROM_REGION( 0x50000 * 2, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "39",           0x00000, 0x08000, 0x043380bd )	// V3.0 1992,1,23
	ROM_RELOAD(               0x50000, 0x08000             )
	ROM_LOAD( "38",           0x10000, 0x20000, 0xe16216e8 )
	ROM_RELOAD(               0x60000, 0x20000             )
	ROM_LOAD( "brickzon.007", 0x30000, 0x20000, 0xceed12f1 )
	ROM_RELOAD(               0x80000, 0x20000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Music Z80 Code */
	ROM_LOAD( "brickzon.010", 0x00000, 0x10000, 0x4eba8178 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* PCM Z80 Code */
	ROM_LOAD( "brickzon.011", 0x00000, 0x10000, 0x6c54161a )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT )	/* tiles */
	ROM_LOAD( "brickzon.001", 0x00000, 0x20000, 0x6970ada9 )
	ROM_LOAD( "32",           0x20000, 0x20000, 0x32dbf2dd )
	ROM_LOAD( "brickzon.003", 0x40000, 0x20000, 0x2e4f194b )
	ROM_LOAD( "brickzon.004", 0x60000, 0x20000, 0x2be5f335 )
	ROM_LOAD( "35",           0x80000, 0x20000, 0xb463dfcf )
	ROM_LOAD( "brickzon.006", 0xa0000, 0x20000, 0xbbf31081 )
ROM_END

/* !! BRICKZN3 !! */

static int is_special(int i)
{
	if (i & 0x400)
	{
		switch ( i & 0xf )
		{
			case 0x1:
			case 0x5:	return 1;
			default:	return 0;
		}
	}
	else
	{
		switch ( i & 0xf )
		{
			case 0x3:
			case 0x7:
			case 0x8:
			case 0xc:	return 1;
			default:	return 0;
		}
	}
}


/***************************************************************************

								Hard Head 2 (v2.0)

These ROMS are all 27C512

ROM 1 is at Location 1N
ROM 2 ..............1o
ROM 3 ..............1Q
ROM 4...............3N
ROM 5.............. 4N
ROM 6...............4o
ROM 7...............4Q
ROM 8...............6N
ROM 10..............H5
ROM 11..............i5
ROM 12 .............F7
ROM 13..............H7
ROM 15..............N10

These ROMs are 27C256

ROM 9...............F5
ROM 14..............C8

Game uses 2 Z80B processors and a Custom Sealed processor (assumed)
Labeled "SUNA T568009"

Sound is a Yamaha YM3812 and a  AY-3-8910A

3 RAMS are 6264LP- 10   and 5) HM6116K-90 rams  (small package)

24 mhz

***************************************************************************/

ROM_START( hardhea2 )
	ROM_REGION( 0x50000 * 2, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "hrd-hd9",  0x00000, 0x08000, 0x69c4c307 )	// V 2.0 1991,2,12
	ROM_RELOAD(           0x50000, 0x08000             )
	ROM_LOAD( "hrd-hd10", 0x10000, 0x10000, 0x77ec5b0a )
	ROM_RELOAD(           0x60000, 0x10000             )
	ROM_LOAD( "hrd-hd11", 0x20000, 0x10000, 0x12af8f8e )
	ROM_RELOAD(           0x70000, 0x10000             )
	ROM_LOAD( "hrd-hd12", 0x30000, 0x10000, 0x35d13212 )
	ROM_RELOAD(           0x80000, 0x10000             )
	ROM_LOAD( "hrd-hd13", 0x40000, 0x10000, 0x3225e7d7 )
	ROM_RELOAD(           0x90000, 0x10000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Music Z80 Code */
	ROM_LOAD( "hrd-hd14", 0x00000, 0x08000, 0x79a3be51 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* PCM Z80 Code */
	ROM_LOAD( "hrd-hd15", 0x00000, 0x10000, 0xbcbd88c3 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_INVERT )	/* tiles */
	ROM_LOAD( "hrd-hd1",  0x00000, 0x10000, 0x7e7b7a58 )
	ROM_LOAD( "hrd-hd2",  0x10000, 0x10000, 0x303ec802 )
	ROM_LOAD( "hrd-hd5",  0x20000, 0x10000, 0xf738c0af )
	ROM_LOAD( "hrd-hd6",  0x30000, 0x10000, 0xbf90d3ca )
	ROM_LOAD( "hrd-hd3",  0x40000, 0x10000, 0x3353b2c7 )
	ROM_LOAD( "hrd-hd4",  0x50000, 0x10000, 0xdbc1f9c1 )
	ROM_LOAD( "hrd-hd7",  0x60000, 0x10000, 0x992ce8cb )
	ROM_LOAD( "hrd-hd8",  0x70000, 0x10000, 0x359597a4 )
ROM_END


void init_hardhead(void)
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int i;

	for (i = 0; i < 0x8000; i++)
	{
		data8_t x = RAM[i];
		if (!   ( (i & 0x800) && ((~i & 0x400) ^ ((i & 0x4000)>>4)) )	)
		{
			x	=	x ^ 0x58;
			x	=	(((x & (1<<0))?1:0)<<0) |
					(((x & (1<<1))?1:0)<<1) |
					(((x & (1<<2))?1:0)<<2) |
					(((x & (1<<4))?1:0)<<3) |
					(((x & (1<<3))?1:0)<<4) |
					(((x & (1<<5))?1:0)<<5) |
					(((x & (1<<6))?1:0)<<6) |
					(((x & (1<<7))?1:0)<<7);
		}
		RAM[i] = x;
	}
}

void init_brickzn(void)
{
	data8_t	*RAM	=	memory_region(REGION_CPU1);
	size_t	size	=	memory_region_length(REGION_CPU1)/2;
	int i;

	memory_set_opcode_base(0,RAM + size);

	/* Opcodes */

	for (i = 0; i < 0x8000; i++)
	{
		int encry;
		data8_t x = RAM[i];

		data8_t mask = 0x90;

		switch ( i & 0xf )
		{
			case 0x0:
			case 0x1:
			case 0x2:
				if (i & 0x40)	mask = 0x10;
				encry = 1;
				break;

			case 0x3:
				if (i & 0x40)	encry = 0;
				else
					if (i & 0x400)	encry = 1;
					else			encry = 2;
				break;

			case 0x7:
			case 0x8:
				encry = 2;
				break;

			case 0xc:
//				if (i & 0x400)	encry = 1;
//				else			encry = 0;
encry = 0;
				break;

			case 0xd:
			case 0xe:
			case 0xf:
				mask = 0x10;
				encry = 1;
				break;

			default:
				encry = 1;
		}

		switch (encry)
		{
			case 1:
				x	^=	mask;
				x	=	(((x & (1<<1))?1:0)<<0) |
						(((x & (1<<0))?1:0)<<1) |
						(((x & (1<<6))?1:0)<<2) |
						(((x & (1<<5))?1:0)<<3) |
						(((x & (1<<4))?1:0)<<4) |
						(((x & (1<<3))?1:0)<<5) |
						(((x & (1<<2))?1:0)<<6) |
						(((x & (1<<7))?1:0)<<7);
				break;

			case 2:
				x	^=	mask;
				x	=	(((x & (1<<0))?1:0)<<0) |	// swap
						(((x & (1<<1))?1:0)<<1) |
						(((x & (1<<6))?1:0)<<2) |
						(((x & (1<<5))?1:0)<<3) |
						(((x & (1<<4))?1:0)<<4) |
						(((x & (1<<3))?1:0)<<5) |
						(((x & (1<<2))?1:0)<<6) |
						(((x & (1<<7))?1:0)<<7);
				break;
		}

		RAM[i + size] = x;
	}


	/* Data */

	for (i = 0; i < 0x8000; i++)
	{
		data8_t x = RAM[i];

		if ( !is_special(i) )
		{
			x	^=	0x10;
			x	=	(((x & (1<<1))?1:0)<<0) |
					(((x & (1<<0))?1:0)<<1) |
					(((x & (1<<6))?1:0)<<2) |
					(((x & (1<<5))?1:0)<<3) |
					(((x & (1<<4))?1:0)<<4) |
					(((x & (1<<3))?1:0)<<5) |
					(((x & (1<<2))?1:0)<<6) |
					(((x & (1<<7))?1:0)<<7);
		}

		RAM[i] = x;
	}
}

void init_hardhea2(void)
{
	data8_t	*RAM	=	memory_region(REGION_CPU1);
	size_t	size	=	memory_region_length(REGION_CPU1)/2;
	int i;

	memory_set_opcode_base(0,RAM + size);

	for (i = 0; i < 0x8000; i++)
	{
		data8_t x = RAM[i];

		x	^=	0x45;
		x	=	(((x & (1<<0))?1:0)<<0) |
				(((x & (1<<1))?1:0)<<1) |
				(((x & (1<<2))?1:0)<<2) |
				(((x & (1<<4))?1:0)<<3) |
				(((x & (1<<3))?1:0)<<4) |
				(((x & (1<<7))?1:0)<<5) |
				(((x & (1<<6))?1:0)<<6) |
				(((x & (1<<5))?1:0)<<7);
		RAM[i+size] = x;
	}

	for (i = 0; i < 0x8000; i++)
	{
		data8_t x = RAM[i];

		x	^=	0x41;
		x	=	(((x & (1<<0))?1:0)<<0) |
				(((x & (1<<1))?1:0)<<1) |
				(((x & (1<<2))?1:0)<<2) |
				(((x & (1<<3))?1:0)<<3) |
				(((x & (1<<4))?1:0)<<4) |
				(((x & (1<<7))?1:0)<<5) |
				(((x & (1<<5))?1:0)<<6) |
				(((x & (1<<6))?1:0)<<7);
		RAM[i] = x;
	}
	suna8_gfx_type = 1;
}

static void init_hardheab( void )
{
	/* patch ROM checksum (ROM1 fails self test) */
	memory_region( REGION_CPU1 )[0x1e5b] = 0xAF;
	suna8_gfx_type = 1;
}

static void init_rranger( void )
{
	suna8_gfx_type = 0;
}

/*           rom       parent   machine	  inp		init */
GAMEX( 1988, rranger,  0,       rranger,  rranger,  rranger,  ROT0_16BIT, "SunA", "Rough Ranger (v2.0, Sharp Image license)",GAME_NO_COCKTAIL )
GAMEX( 1988, sranger,  0,       rranger,  rranger,	rranger,  ROT0_16BIT, "SunA", "Super Ranger",			GAME_NO_COCKTAIL )
GAMEX( 1988, srangerb, sranger, rranger,  rranger,	rranger,  ROT0_16BIT, "bootleg", "Super Ranger (bootleg)", GAME_NO_COCKTAIL )
GAMEX( 1988, hardhead, 0,       hardhead, hardhead, hardhead, ROT0_16BIT, "SunA", "Hard Head",          	GAME_NO_COCKTAIL )
GAMEX( 1988, hardheab, hardhead,hardhead, hardhead,	hardheab, ROT0_16BIT, "bootleg", "Hard Head (bootleg)",	GAME_NO_COCKTAIL )
GAMEX( 1991, hardhea2, 0,       hardhea2, hardhea2, hardhea2, ROT0_16BIT, "SunA", "Hard Head 2 (v2.0)", 	GAME_NOT_WORKING  )
GAMEX( 1992, brickzn,  0,       brickzn,  brickzn,  brickzn,  ROT0_16BIT, "SunA", "Brick Zone (v5.0)",  	GAME_NOT_WORKING  )
GAMEX( 1992, brickzn3, brickzn, brickzn,  brickzn,  brickzn,  ROT0_16BIT, "SunA", "Brick Zone (v3.0)",  	GAME_NOT_WORKING  )
