/***************************************************************************

						-= Dynax / Nakanihon Games =-

					driver by	Luca Elia (l.elia@tin.it)


---------------------------------------------------------------------------------------------------
Year + Game				CPU		Sound			Gfx							Misc
---------------------------------------------------------------------------------------------------
89 Sports Match			Z80 	YM2203			Color PROM + 6845 + DYNAX(TC17G032AP-0246)
94 Rong Rong			Z80		YM2413 + M6295	NAKANIHON NL-002
95 Don Den Lover Vol 1	68000	YM2413 + M6295	NAKANIHON NL-005			NVRAM + RTC 72421B 4382
---------------------------------------------------------------------------------------------------


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/dynax.h"


/***************************************************************************


								Interrupts


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

UINT8 dynax_blitter_irq;
UINT8 dynax_sound_irq;
UINT8 dynax_vblank_irq;

/* It runs in IM 0, thus needs an opcode on the data bus */
void sprtmtch_update_irq(void)
{
	int irq	=	((dynax_sound_irq)   ? 0x08 : 0) |
				((dynax_vblank_irq)  ? 0x10 : 0) |
				((dynax_blitter_irq) ? 0x20 : 0) ;
	cpu_set_irq_line_and_vector(0, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xc7 | irq); /* rst $xx */
}

WRITE_HANDLER( sprtmtch_vblank_ack_w )
{
	dynax_vblank_irq = 0;
	sprtmtch_update_irq();
}

WRITE_HANDLER( sprtmtch_blitter_ack_w )
{
	dynax_blitter_irq = 0;
	sprtmtch_update_irq();
}

INTERRUPT_GEN( sprtmtch_vblank_interrupt )
{
	dynax_vblank_irq = 1;
	sprtmtch_update_irq();
}

void sprtmtch_sound_callback(int state)
{
	dynax_sound_irq = state;
	sprtmtch_update_irq();
}

/***************************************************************************


								Memory Maps


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

static WRITE_HANDLER( sprtmtch_coincounter_0_w )
{
	coin_counter_w(0, data & 1);
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, coin counter 0 <- %02X\n", activecpu_get_pc(), data);
}
static WRITE_HANDLER( sprtmtch_coincounter_1_w )
{
	coin_counter_w(1, data & 1);
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, coin counter 1 <- %02X\n", activecpu_get_pc(), data);
}

static READ_HANDLER( ret_ff )	{	return 0xff;	}

static WRITE_HANDLER( sprtmtch_rombank_w )
{
	data8_t *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x10000+0x8000*(data & 0x07)]);
}

static MEMORY_READ_START( sprtmtch_readmem )
	{ 0x0000, 0x6fff, MRA_ROM					},	// ROM
	{ 0x7000, 0x7fff, MRA_RAM					},	// RAM
	{ 0x8000, 0xffff, MRA_BANK1					},	// BANKED ROM
MEMORY_END

static MEMORY_WRITE_START( sprtmtch_writemem )
	{ 0x0000, 0x6fff, MWA_ROM					},	// ROM
	{ 0x7000, 0x7fff, MWA_RAM					},	// RAM
	{ 0x8000, 0xffff, MWA_ROM					},	// BANKED ROM
MEMORY_END

static PORT_READ_START( sprtmtch_readport )
	{ 0x10, 0x10, YM2203_status_port_0_r	},	// YM2203
	{ 0x11, 0x11, YM2203_read_port_0_r		},	// 2 x DSW
	{ 0x20, 0x20, input_port_0_r			},	// P1
	{ 0x21, 0x21, input_port_1_r			},	// P2
	{ 0x22, 0x22, input_port_2_r			},	// Coins
	{ 0x23, 0x23, ret_ff					},	// ?
PORT_END

static PORT_WRITE_START( sprtmtch_writeport )
	{ 0x01, 0x01, sprtmtch_blit_draw_w		},	// Blitter
	{ 0x02, 0x02, dynax_blit_x_w			},	// Destination X
	{ 0x03, 0x03, dynax_blit_y_w			},	// Destination Y
	{ 0x04, 0x04, dynax_blit_addr0_w		},	// Source Address
	{ 0x05, 0x05, dynax_blit_addr1_w		},	//
	{ 0x06, 0x06, dynax_blit_addr2_w		},	//
	{ 0x07, 0x07, dynax_blit_scroll_w		},	// Layers Scroll X & Y
	{ 0x10, 0x10, YM2203_control_port_0_w	},	// YM2203
	{ 0x11, 0x11, YM2203_write_port_0_w		},	//
//	{ 0x12, 0x12, IOWP_NOP					},	// ?? CRT Controller ??
//	{ 0x13, 0x13, IOWP_NOP					},	// ?? CRT Controller ??
	{ 0x30, 0x30, dynax_blit_enable_w		},	// Layers Enable
	{ 0x31, 0x31, sprtmtch_rombank_w		},	// BANK ROM Select
	{ 0x32, 0x32, dynax_blit_dest_w			},	// Destination Layer
	{ 0x33, 0x33, dynax_blit_pen_w			},	// Destination Pen
	{ 0x34, 0x34, dynax_blit_palette01_w	},	// Layers Palettes (Low Bits)
	{ 0x35, 0x35, dynax_blit_palette2_w		},	//
	{ 0x36, 0x36, dynax_blit_backpen_w		},	// Background Color
	{ 0x37, 0x37, sprtmtch_vblank_ack_w		},	// VBlank IRQ Ack
	{ 0x41, 0x41, dynax_flipscreen_w		},	// Flip Screen
	{ 0x42, 0x42, sprtmtch_coincounter_0_w	},	// Coin Counters
	{ 0x43, 0x43, sprtmtch_coincounter_1_w	},	//
	{ 0x44, 0x44, sprtmtch_blitter_ack_w	},	// Blitter IRQ Ack
	{ 0x45, 0x45, dynax_blit_palbank_w		},	// Layers Palettes (High Bit)
PORT_END


/***************************************************************************
							Don Den Lover Vol.1
***************************************************************************/

#if 0
static WRITE16_HANDLER( ddenlovr_oki_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 3) * 0x40000);
}
#endif

static READ16_HANDLER( ddenlovr_gfxrom_r )
{
	data8_t *ROM	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	UINT32 address	=	dynax_blit_address - 0x200000;	// why?

	if (address >= size)
	{
		address %= size;
		logerror("CPU#0 PC %06X: Error, Blitter address %06X out of range\n", activecpu_get_pc(), address);
	}

	dynax_blit_address++;

	return ROM[address];
}

static WRITE16_HANDLER( ddenlovr_blit_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
//logerror("CPU#0 PC %06X: Blitter %x <- %02X\n", activecpu_get_pc(), offset, data);
		switch(offset)
		{
		case 0:
			dynax_blit_reg = data;
			break;

		case 1:
			switch(dynax_blit_reg)
			{
			case 0x00:
				//?
				break;
			case 0x05:
				//?
				break;

			case 0x0d:
				dynax_blit_addr0_w(0,data&0xff);
				break;
			case 0x0e:
				dynax_blit_addr1_w(0,data&0xff);
				break;
			case 0x0f:
				dynax_blit_addr2_w(0,data&0xff);
				break;

			case 0x14:
			case 0x54:
				dynax_blit_x_w(0,data&0xff);
				if (dynax_blit_reg & 0x40)	dynax_blit_x |= 0x100;
				break;

			case 0x02:
			case 0x42:
				dynax_blit_y_w(0,data&0xff);
				if (dynax_blit_reg & 0x40)	dynax_blit_y |= 0x100;
				break;

			case 0x24:
				drawgfx(	Machine->scrbitmap, Machine->uifont,
							dynax_blit_address,
							0,
							0, 0,
							dynax_blit_x, dynax_blit_y,
							&Machine->visible_area, TRANSPARENCY_PEN,0x00	);
				break;
			}
		}
	}
}

READ16_HANDLER( ddenlovr_special_r )
{
	return readinputport(2) | (rand() & 0x00c0);
}

static WRITE16_HANDLER( ddenlovr_coincounter_0_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(0, data & 1);
	else
		logerror("CPU#0 PC %06X: Error, MSB of coin counter 0 written\n", activecpu_get_pc());
}
static WRITE16_HANDLER( ddenlovr_coincounter_1_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(1, data & 1);
	else
		logerror("CPU#0 PC %06X: Error, MSB of coin counter 1 written\n", activecpu_get_pc());
}

static MEMORY_READ16_START( ddenlovr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0xff0000, 0xffffff, MRA16_RAM					},	// RAM
	{ 0xe00086, 0xe00087, ddenlovr_gfxrom_r			},	// Video Chip
	{ 0xe00100, 0xe00101, input_port_0_word_r		},	// P1?
	{ 0xe00102, 0xe00103, input_port_1_word_r		},	// P2?
	{ 0xe00104, 0xe00105, ddenlovr_special_r		},	// Coins + ?
	{ 0xe00200, 0xe00201, input_port_3_word_r		},	// DSW
	{ 0xe00500, 0xe0051f, MRA16_RAM 				},	// NVRAM?
	{ 0xe00700, 0xe00701, OKIM6295_status_0_lsb_r	},	// Sound
MEMORY_END

static MEMORY_WRITE16_START( ddenlovr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0xff0000, 0xffffff, MWA16_RAM						},	// RAM
	{ 0xe00080, 0xe00083, ddenlovr_blit_w				},	// Video Chip
	{ 0xe00308, 0xe00309, ddenlovr_coincounter_0_w		},	// Coin Counters
	{ 0xe0030c, 0xe0030d, ddenlovr_coincounter_1_w		},	//
	{ 0xe00400, 0xe00401, YM2413_register_port_0_lsb_w	},	// Sound
	{ 0xe00402, 0xe00403, YM2413_data_port_0_lsb_w		},	//
	{ 0xe00500, 0xe0051f, MWA16_RAM 					},	// NVRAM?
	{ 0xe00302, 0xe00303, MWA16_NOP						},	// ?
	{ 0xe00700, 0xe00701, OKIM6295_data_0_lsb_w 		},	//
	{ 0xd00000, 0xd017ff, MWA16_RAM 					},	// Palette?
MEMORY_END


/***************************************************************************
								Rong Rong
***************************************************************************/

data8_t rongrong_select,rongrong_select2;

READ_HANDLER( rongrong_input_r )
{
	if (!(rongrong_select & 0x01))	return readinputport(3);
	if (!(rongrong_select & 0x02))	return readinputport(4);
	if (!(rongrong_select & 0x04))	return readinputport(0);
	if (!(rongrong_select & 0x08))	return readinputport(1);
	return 0xff;
}

READ_HANDLER( rongrong_input2_r )
{
	switch( rongrong_select2 )
	{
		case 0x00:	return 0xff;
		case 0x01:	return 0xff;
		case 0x02:	return readinputport(2);
	}
	return 0xff;
}

WRITE_HANDLER( rongrong_select_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	rongrong_select = data;
	cpu_setbank(1, &RAM[0x10000 + 0x8000 * (rongrong_select & 0x0f)]);
}

WRITE_HANDLER( rongrong_select2_w )
{
	rongrong_select2 = data;
}

static MEMORY_READ_START( rongrong_readmem )
	{ 0x0000, 0x5fff, MRA_ROM					},	// ROM
	{ 0x6000, 0x7fff, MRA_RAM					},	// RAM
	{ 0x8000, 0xffff, MRA_BANK1					},	// ROM (Banked)
MEMORY_END

static MEMORY_WRITE_START( rongrong_writemem )
	{ 0x0000, 0x5fff, MWA_ROM					},	// ROM
	{ 0x6000, 0x7fff, MWA_RAM					},	// RAM
	{ 0x8000, 0xffff, MWA_ROM					},	// ROM (Banked)
MEMORY_END

static PORT_READ_START( rongrong_readport )
	{ 0x1c, 0x1c, rongrong_input_r		},	//
	{ 0x40, 0x40, OKIM6295_status_0_r	},	//
	{ 0xa2, 0xa2, rongrong_input2_r		},	//
	{ 0xa3, 0xa3, rongrong_input2_r		},	//
PORT_END

static PORT_WRITE_START( rongrong_writeport )
	{ 0x1e, 0x1e, rongrong_select_w		},	//
	{ 0x40, 0x40, OKIM6295_data_0_w		},	//
	{ 0xa0, 0xa0, rongrong_select2_w	},	//
PORT_END
/*
0&1: video chip
	14 x
	54
	02 y
	42

1e input select,1c input read
	3e=dsw1	3d=dsw2
a0 input select,a2 input read (protection?)
	0=?	1=?	2=coins(from a3)
*/


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

INPUT_PORTS_START( sprtmtch )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 10)
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 10)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )	// Time
	PORT_DIPSETTING(    0x00, "1 (Easy)" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x05, "6" )
	PORT_DIPSETTING(    0x06, "7" )
	PORT_DIPSETTING(    0x07, "8 (Hard)" )
	PORT_DIPNAME( 0x18, 0x10, "Vs Time" )
	PORT_DIPSETTING(    0x18, "8 s" )
	PORT_DIPSETTING(    0x10, "10 s" )
	PORT_DIPSETTING(    0x08, "12 s" )
	PORT_DIPSETTING(    0x00, "14 s" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
							Don Den Lover Vol.1
***************************************************************************/

INPUT_PORTS_START( ddenlovr )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins + ?
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x00f0, IP_ACTIVE_HIGH, IPT_SPECIAL  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN3 - DSW
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5*" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6*" )	// 6&7
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7*" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Rong Rong
***************************************************************************/

INPUT_PORTS_START( rongrong )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 10)
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 10)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//?
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4*" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Lives?" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x03, 0x03, "Lives?" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(    0x0c, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x30, 0x30, "Unknown 2-4&5*" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "3'" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************


								Machine Drivers


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

static struct YM2203interface ym2203_intf =
{
	1,
	22000000 / 8,					/* 2.75MHz */
	{ YM2203_VOL(100,100) },
	{ input_port_3_r },				/* Port A Read: DSW */
	{ input_port_4_r },				/* Port B Read: DSW */
	{ 0 },							/* Port A Write */
	{ 0 },							/* Port B Write */
	{ sprtmtch_sound_callback },	/* IRQ handler */
};

static MACHINE_DRIVER_START( sprtmtch )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_MEMORY(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_PORTS(sprtmtch_readport,sprtmtch_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)	// needs alpha blending
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+8, 256-1-8)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(sprtmtch)
	MDRV_VIDEO_UPDATE(sprtmtch)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_intf)
MACHINE_DRIVER_END


/***************************************************************************
							Don Den Lover Vol.1
***************************************************************************/

static struct YM2413interface ym2413_intf =
{
	1,
	3579545,	/* ???? */
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static struct OKIM6295interface okim6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 50 }
};

static MACHINE_DRIVER_START( ddenlovr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000 / 2)
	MDRV_CPU_MEMORY(ddenlovr_readmem,ddenlovr_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x800)

	MDRV_VIDEO_START(dynax)
	MDRV_VIDEO_UPDATE(dynax)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2413, ym2413_intf)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf)
MACHINE_DRIVER_END


/***************************************************************************
								Rong Rong
***************************************************************************/

static MACHINE_DRIVER_START( rongrong )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* ? */
	MDRV_CPU_MEMORY(rongrong_readmem,rongrong_writemem)
	MDRV_CPU_PORTS(rongrong_readport,rongrong_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x800)

	MDRV_VIDEO_START(dynax)
	MDRV_VIDEO_UPDATE(dynax)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2413, ym2413_intf)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************
Sports Match
Dynax 1989

                     5563
                     3101
        SW2 SW1
                             3103
         YM2203              3102
                     16V8
                     Z80         DYNAX
         22MHz

           6845
                         53462
      17G                53462
      18G                53462
                         53462
                         53462
                         53462

- Note: to enter hardware test mode keep start1 pressed during power-up.

***************************************************************************/

ROM_START( sprtmtch )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "3101.3d", 0x00000, 0x08000, 0xd8fa9638 )
	ROM_CONTINUE(        0x30000, 0x08000 )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* Gfx Data (Do not dispose) */
	ROM_LOAD( "3102.6c", 0x00000, 0x20000, 0x46f90e59 )
	ROM_LOAD( "3103.5c", 0x20000, 0x20000, 0xad29d7bd )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "18g", 0x000, 0x200, 0xdcc4e0dd )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "17g", 0x200, 0x200, 0x5443ebfb )
ROM_END

ROM_START( drgpunch )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2401.3d", 0x00000, 0x08000, 0xb310709c )
	ROM_CONTINUE(        0x10000, 0x08000 )
	ROM_LOAD( "2402.6d", 0x30000, 0x10000, 0xd21ed237 )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )	/* Gfx Data (Do not dispose) */
	ROM_LOAD( "2403.6c", 0x00000, 0x20000, 0xb936f202 )
	ROM_LOAD( "2404.5c", 0x20000, 0x20000, 0x2ee0683a )
	ROM_LOAD( "2405.3c", 0x40000, 0x20000, 0xaefbe192 )
	ROM_LOAD( "2406.1c", 0x60000, 0x20000, 0xe137f96e )
	ROM_LOAD( "2407.6a", 0x80000, 0x20000, 0xf3f1b065 )
	ROM_LOAD( "2408.5a", 0xa0000, 0x20000, 0x3a91e2b9 )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.18g", 0x000, 0x200, 0x9adccc33 )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.17g", 0x200, 0x200, 0x324fa9cf )
ROM_END


/***************************************************************************

Don Den Lover Vol 1
(C) Dynax Inc 1995

CPU: TMP68HC000N-12
SND: OKI M6295, YM2413 (18 pin DIL), YMZ284-D (16 pin DIL. This chip is in place where a 40 pin chip is marked on PCB,
                                     possibly a replacement for some other 40 pin YM chip?)
OSC: 28.636MHz (near large GFX chip), 24.000MHz (near CPU)
DIPS: 1 x 8 Position switch. DIP info is in Japanese !
RAM: 1 x Toshiba TC5588-35, 2 x Toshiba TC55257-10, 5 x OKI M514262-70

OTHER:
Battery
RTC 72421B   4382 (18 pin DIL)
3 X PAL's (2 on daughter-board at locations 2E & 2D, 1 on main board near CPU at location 4C)
GFX Chip - NAKANIHON NL-005 (208 pin, square, surface-mounted)
Controls: 8 Way Joystick plus 2 Buttons.

ROMS: (All located on a daughter-board)

1133H.1C	27C020   \
1134H.1A	27C020   / MAIN PROGRAM?

1131H.1F	27C040   \
1132H.1E	27C040   / SOUND?

1135H.3H	27C040   -\
1136H.3F	27C040    |
1137H.3E	27C040    | GFX?
1138H.3D	27C040    |
1139H.3C	27C040    /

***************************************************************************/

ROM_START( ddenlovr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "1134h.1a", 0x000000, 0x040000, 0x43accdff )
	ROM_LOAD16_BYTE( "1133h.1c", 0x000001, 0x040000, 0x361bf7b6 )

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* Gfx Data (Do not dispose) */
	ROM_LOAD( "1135h.3h", 0x000000, 0x080000, 0xee143d8e )
	ROM_LOAD( "1136h.3f", 0x080000, 0x080000, 0x58a662be )
	ROM_LOAD( "1137h.3e", 0x100000, 0x080000, 0xf96e0708 )
	ROM_LOAD( "1138h.3d", 0x180000, 0x080000, 0x633cff33 )
	ROM_LOAD( "1139h.3c", 0x200000, 0x080000, 0xbe1189ca )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "1131h.1f", 0x000000, 0x080000, 0x32f68241 )	// 4 x $40000
	ROM_LOAD( "1132h.1e", 0x080000, 0x080000, 0x2de6363d )	//
ROM_END


/***************************************************************************

								Rong Rong

Here are the proms for Nakanihon's Rong Rong
It's a quite nice Puzzle game.
The CPU don't have any numbers on it except for this:
Nakanihon
NL-002
3J3  JAPAN
For the sound it uses A YM2413

***************************************************************************/

ROM_START( rongrong )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "rr_8002g.rom", 0x00000, 0x80000, 0x9a5d2885 )
	ROM_RELOAD(               0x10000, 0x80000             )

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* Gfx Data (Do not dispose) */
	ROM_LOAD( "rr_8003.rom",  0x000000, 0x80000, 0xf57192e5 )
	ROM_LOAD( "rr_8004.rom",  0x080000, 0x80000, 0xc8c0b5cb )
	ROM_LOAD( "rr_8005g.rom", 0x100000, 0x80000, 0x11c7a23c )
	ROM_LOAD( "rr_8006g.rom", 0x180000, 0x80000, 0xf3de77e6 )
	ROM_LOAD( "rr_8007g.rom", 0x200000, 0x80000, 0x38a8caa3 )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rr_8001w.rom", 0x00000, 0x40000, 0x8edc87a2 )
ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAME ( 1989, sprtmtch, 0,        sprtmtch, sprtmtch, 0, ROT0, "Log+Dynax (Fabtek license)", "Sports Match" )
GAME ( 1989, drgpunch, sprtmtch, sprtmtch, sprtmtch, 0, ROT0, "Log+Dynax", "Dragon Punch (Japan)" )

/* TESTDRIVERS */
GAMEX( 1995, ddenlovr, 0, ddenlovr, ddenlovr, 0, ROT0, "Dynax",     "Don Den Lover Vol 1", GAME_NOT_WORKING )
GAMEX( 1994, rongrong, 0, rongrong, rongrong, 0, ROT0, "Nakanihon", "Rong Rong",           GAME_NOT_WORKING )
