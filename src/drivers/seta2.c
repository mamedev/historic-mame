/***************************************************************************

						  -= Newer Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


CPU    :	TMP68301*

Custom :	X1-010				Sound: 8 Bit PCM
			DX-101				Sprites
			DX-102 x3

OSC:	50.0000MHz
        32.5304MHz

*	The Toshiba TMP68301 is a 68HC000 + serial I/O, parallel I/O,
	3 timers, address decoder, wait generator, interrupt controller,
	all integrated in a single chip.

-------------------------------------------------------------------------------------------
Ordered by Board		Year	Game									By
-------------------------------------------------------------------------------------------
P0-123A					1996	Wakakusamonogatari Mahjong Yonshimai	Maboroshi Ware
P0-125A ; KE (Namco)	1996	Kosodate Quiz My Angel					Namco
P0-136A ; KL (Namco)	1997	Kosodate Quiz My Angel 2				Namco
P0-142A					1999	Puzzle De Bowling						Nihon System / Moss
P0-142A + extra parts	2000	Penguin Brothers						Subsino
-------------------------------------------------------------------------------------------

TODO:

- Proper emulation of the TMP68301 CPU, in a core file.
- Flip screen support.
- Fix some graphics imperfections (e.g. color depth selection,
  "tilemap" sprites) [all done? - NS]
- I added a kludge involving a -0x10 yoffset, this fixes the lifeline in myangel.
  I didn't find a better way to do it without breaking pzlbowl's title screen.

mj4simai:
- test mode doesn't work correctly, the grid is ok but when you press a key to go to the
  next screen (input test) it stays up a second and then drops back into the game

myangel2:
- before each level, the background image is shown with completely wrong colors. It
  corrects itself when the level starts.

***************************************************************************/

/***************************************************************************

							Penguin Brothers (Japan)

(c)2000 Subsino
Board:	P0-142A
CPU:	TMP68301 (68000 core)

OSC:	50.0000MHz
        32.5304MHz
        28.0000MHz

Chips.:	DX-101
        DX-102 x3
Sound:	X1-010

Notes:	pzlbowl PCB with extra parts:
        28MHz OSC
        2x 62256 SRAM
        74HC00

***************************************************************************/





#include "driver.h"
#include "vidhrdw/generic.h"
#include "seta.h"

/***************************************************************************


				TMP68301 basic emulation + Interrupt Handling


***************************************************************************/

static UINT8 tmp68301_IE[3];		// 3 External Interrupt Lines
static void *tmp68301_timer[3];		// 3 Timers
static data16_t *tmp68301_regs;		// Hardware Registers

static int irq_vector[8];

void tmp68301_update_timer( int i );

int seta2_irq_callback(int int_level)
{
	int vector = irq_vector[int_level];
//	logerror("CPU #0 PC %06X: irq callback returns %04X for level %x\n",activecpu_get_pc(),vector,int_level);
	return vector;
}

void tmp68301_timer_callback(int i)
{
	data16_t TCR	=	tmp68301_regs[(0x200 + i * 0x20)/2];
	data16_t IMR	=	tmp68301_regs[0x94/2];		// Interrupt Mask Register (IMR)
	data16_t ICR	=	tmp68301_regs[0x8e/2+i];	// Interrupt Controller Register (ICR7..9)
	data16_t IVNR	=	tmp68301_regs[0x9a/2];		// Interrupt Vector Number Register (IVNR)

//	logerror("CPU #0 PC %06X: callback timer %04X, j = %d\n",activecpu_get_pc(),i,tcount);

	if	(	(TCR & 0x0004) &&	// INT
			!(IMR & (0x100<<i))
		)
	{
		int level = ICR & 0x0007;

		// Interrupt Vector Number Register (IVNR)
		irq_vector[level]	=	IVNR & 0x00e0;
		irq_vector[level]	+=	4+i;

		cpu_set_irq_line(0,level,HOLD_LINE);
	}

	if (TCR & 0x0080)	// N/1
	{
		// Repeat
		tmp68301_update_timer(i);
	}
	else
	{
		// One Shot
	}
}

void tmp68301_update_timer( int i )
{
	data16_t TCR	=	tmp68301_regs[(0x200 + i * 0x20)/2];
	data16_t MAX1	=	tmp68301_regs[(0x204 + i * 0x20)/2];
	data16_t MAX2	=	tmp68301_regs[(0x206 + i * 0x20)/2];

	int max = 0;
	double duration = 0;

	timer_adjust(tmp68301_timer[i],TIME_NEVER,i,0);

	// timers 1&2 only
	switch( (TCR & 0x0030)>>4 )						// MR2..1
	{
	case 1:
		max = MAX1;
		break;
	case 2:
		max = MAX2;
		break;
	}

	switch ( (TCR & 0xc000)>>14 )					// CK2..1
	{
	case 0:	// System clock (CLK)
		if (max)
		{
			int scale = (TCR & 0x3c00)>>10;			// P4..1
			if (scale > 8) scale = 8;
			duration = Machine->drv->cpu[0].cpu_clock;
			duration /= 1 << scale;
			duration /= max;
		}
		break;
	}

//	logerror("CPU #0 PC %06X: TMP68301 Timer %d, duration %lf, max %04X\n",activecpu_get_pc(),i,duration,max);

	if (!(TCR & 0x0002))				// CS
	{
		if (duration)
			timer_adjust(tmp68301_timer[i],TIME_IN_HZ(duration),i,0);
		else
			logerror("CPU #0 PC %06X: TMP68301 error, timer %d duration is 0\n",activecpu_get_pc(),i);
	}
}

void tmp68301_init(void)
{
	int i;
	for (i = 0; i < 3; i++)
		tmp68301_timer[i] = timer_alloc(tmp68301_timer_callback);
}

MACHINE_INIT( seta2 )
{
	cpu_set_irq_callback(0, seta2_irq_callback);
	tmp68301_init();
}

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	int i;

	/* Take care of external interrupts */

	data16_t IMR	=	tmp68301_regs[0x94/2];		// Interrupt Mask Register (IMR)
	data16_t IVNR	=	tmp68301_regs[0x9a/2];		// Interrupt Vector Number Register (IVNR)

	for (i = 0; i < 3; i++)
	{
		if	(	(tmp68301_IE[i]) &&
				!(IMR & (1<<i))
			)
		{
			data16_t ICR	=	tmp68301_regs[0x80/2+i];	// Interrupt Controller Register (ICR0..2)

			// Interrupt Controller Register (ICR0..2)
			int level = ICR & 0x0007;

			// Interrupt Vector Number Register (IVNR)
			irq_vector[level]	=	IVNR & 0x00e0;
			irq_vector[level]	+=	i;

			tmp68301_IE[i] = 0;		// Interrupts are edge triggerred

			cpu_set_irq_line(0,level,HOLD_LINE);
		}
	}
}

WRITE16_HANDLER( tmp68301_regs_w )
{
	COMBINE_DATA(&tmp68301_regs[offset]);

	if (!ACCESSING_LSB)	return;

//	logerror("CPU #0 PC %06X: TMP68301 Reg %04X<-%04X & %04X\n",activecpu_get_pc(),offset*2,data,mem_mask^0xffff);

	switch( offset * 2 )
	{
		// Timers
		case 0x200:
		case 0x220:
		case 0x240:
		{
			int i = ((offset*2) >> 5) & 3;

			tmp68301_update_timer( i );
		}
		break;
	}
}

INTERRUPT_GEN( seta2_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			/* VBlank is connected to INT0 (external interrupts pin 0) */
			tmp68301_IE[0] = 1;
			update_irq_state();
			break;
	}
}

/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

WRITE16_HANDLER( seta2_sound_bank_w )
{
	if (ACCESSING_LSB && Machine->sample_rate)
	{
		data8_t *ROM = memory_region( REGION_SOUND1 );
		int banks = (memory_region_length( REGION_SOUND1 ) - 0x100000) / 0x20000;
		if (data >= banks)
		{
			logerror("CPU #0 PC %06X: invalid sound bank %04X\n",activecpu_get_pc(),data);
			data %= banks;
		}
		memcpy(ROM + offset * 0x20000, ROM + 0x100000 + data * 0x20000, 0x20000);
	}
}


/***************************************************************************
                      Wakakusamonogatari Mahjong Yonshimai
***************************************************************************/

static int keyboard_row;

static READ16_HANDLER( mj4simai_p1_r )
{
	switch (keyboard_row)
	{
		case 0x01: return readinputport(3);
		case 0x02: return readinputport(4);
		case 0x04: return readinputport(5);
		case 0x08: return readinputport(6);
		case 0x10: return readinputport(7);
		default:   logerror("p1_r with keyboard_row = %02x\n",keyboard_row); return 0xffff;
	}
}

static WRITE16_HANDLER( mj4simai_keyboard_w )
{
	if (ACCESSING_LSB)
		keyboard_row = data & 0xff;
}

static MEMORY_READ16_START( mj4simai_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_ROM					},	// RAM
	{ 0x600000, 0x600001, mj4simai_p1_r				},	// P1
	{ 0x600002, 0x600003, mj4simai_p1_r				},	// P2, but I'm using P1 again
	{ 0x600006, 0x600007, watchdog_reset16_r		},	// Watchdog
	{ 0x600100, 0x600101, input_port_2_word_r		},	//
	{ 0x600300, 0x600301, input_port_0_word_r		},	// DSW 1
	{ 0x600302, 0x600303, input_port_1_word_r		},	// DSW 2
	{ 0xb00000, 0xb03fff, seta_sound_word_r 		},	// Sound
	{ 0xc00000, 0xc3ffff, MRA16_RAM					},	// Sprites
	{ 0xc40000, 0xc4ffff, MRA16_RAM					},	// Palette
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	// TMP68301 Registers
MEMORY_END

static MEMORY_WRITE16_START( mj4simai_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x600004, 0x600005, mj4simai_keyboard_w			},	// select keyboard row to read
	{ 0x600200, 0x600201, MWA16_NOP						},	// Leds? Coins?
	{ 0x600300, 0x60030f, seta2_sound_bank_w			},	// Samples Banks
	{ 0xb00000, 0xb03fff, seta_sound_word_w 			},	// Sound
	{ 0xc00000, 0xc3ffff, MWA16_RAM, &spriteram16,  &spriteram_size	},	// Sprites
	{ 0xc40000, 0xc4ffff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xc60000, 0xc6003f, seta2_vregs_w, &seta2_vregs	},	// Video Registers
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	// TMP68301 Registers
MEMORY_END


/***************************************************************************
							Kosodate Quiz My Angel
***************************************************************************/

static MEMORY_READ16_START( myangel_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_ROM					},	// RAM
	{ 0x700000, 0x700001, input_port_2_word_r		},	// P1
	{ 0x700002, 0x700003, input_port_3_word_r		},	// P2
	{ 0x700004, 0x700005, input_port_4_word_r		},	// Coins
	{ 0x700006, 0x700007, watchdog_reset16_r		},	// Watchdog
	{ 0x700300, 0x700301, input_port_0_word_r		},	// DSW 1
	{ 0x700302, 0x700303, input_port_1_word_r		},	// DSW 2
	{ 0xb00000, 0xb03fff, seta_sound_word_r 		},	// Sound
	{ 0xc00000, 0xc3ffff, MRA16_RAM					},	// Sprites
	{ 0xc40000, 0xc4ffff, MRA16_RAM					},	// Palette
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	// TMP68301 Registers
MEMORY_END

static MEMORY_WRITE16_START( myangel_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x700200, 0x700201, MWA16_NOP						},	// Leds? Coins?
	{ 0x700310, 0x70031f, seta2_sound_bank_w			},	// Samples Banks
	{ 0xb00000, 0xb03fff, seta_sound_word_w 			},	// Sound
	{ 0xc00000, 0xc3ffff, MWA16_RAM, &spriteram16,  &spriteram_size	},	// Sprites
	{ 0xc40000, 0xc4ffff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xc60000, 0xc6003f, seta2_vregs_w, &seta2_vregs	},	// Video Registers
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	// TMP68301 Registers
MEMORY_END


/***************************************************************************
							Kosodate Quiz My Angel 2
***************************************************************************/

static MEMORY_READ16_START( myangel2_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_ROM					},	// RAM
	{ 0x600000, 0x600001, input_port_2_word_r		},	// P1
	{ 0x600002, 0x600003, input_port_3_word_r		},	// P2
	{ 0x600004, 0x600005, input_port_4_word_r		},	// Coins
	{ 0x600006, 0x600007, watchdog_reset16_r		},	// Watchdog
	{ 0x600300, 0x600301, input_port_0_word_r		},	// DSW 1
	{ 0x600302, 0x600303, input_port_1_word_r		},	// DSW 2
	{ 0xb00000, 0xb03fff, seta_sound_word_r 		},	// Sound
	{ 0xd00000, 0xd3ffff, MRA16_RAM					},	// Sprites
	{ 0xd40000, 0xd4ffff, MRA16_RAM					},	// Palette
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	// TMP68301 Registers
MEMORY_END

static MEMORY_WRITE16_START( myangel2_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x600200, 0x600201, MWA16_NOP						},	// Leds? Coins?
	{ 0x600300, 0x60030f, seta2_sound_bank_w			},	// Samples Banks
	{ 0xb00000, 0xb03fff, seta_sound_word_w 			},	// Sound
	{ 0xd00000, 0xd3ffff, MWA16_RAM, &spriteram16,  &spriteram_size	},	// Sprites
	{ 0xd40000, 0xd4ffff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xd60000, 0xd6003f, seta2_vregs_w, &seta2_vregs	},	// Video Registers
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	// TMP68301 Registers
MEMORY_END


/***************************************************************************
								Puzzle De Bowling
***************************************************************************/

/*	The game checks for a specific sequence of values read from here.
	Patching out the check from ROM seems to work... */
READ16_HANDLER( pzlbowl_protection_r )
{
	return 0;
}

READ16_HANDLER( pzlbowl_coins_r )
{
	return readinputport(4) | (rand() & 0x80 );
}

WRITE16_HANDLER( pzlbowl_coin_counter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,data & 0x10);
		coin_counter_w(1,data & 0x20);
	}
}

static MEMORY_READ16_START( pzlbowl_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_ROM					},	// RAM
	{ 0x400300, 0x400301, input_port_0_word_r		},	// DSW 1
	{ 0x400302, 0x400303, input_port_1_word_r		},	// DSW 2
	{ 0x500000, 0x500001, input_port_2_word_r		},	// P1
	{ 0x500002, 0x500003, input_port_3_word_r		},	// P2
	{ 0x500004, 0x500005, pzlbowl_coins_r			},	// Coins + Protection?
	{ 0x500006, 0x500007, watchdog_reset16_r		},	// Watchdog
	{ 0x700000, 0x700001, pzlbowl_protection_r		},	// Protection
	{ 0x800000, 0x83ffff, MRA16_RAM					},	// Sprites
	{ 0x840000, 0x84ffff, MRA16_RAM					},	// Palette
	{ 0x900000, 0x903fff, seta_sound_word_r 		},	// Sound
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	// TMP68301 Registers
MEMORY_END

static MEMORY_WRITE16_START( pzlbowl_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x400300, 0x40030f, seta2_sound_bank_w			},	// Samples Banks
	{ 0x500004, 0x500005, pzlbowl_coin_counter_w		},	// Coins Counter
	{ 0x800000, 0x83ffff, MWA16_RAM, &spriteram16,  &spriteram_size	},	// Sprites
	{ 0x840000, 0x84ffff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x860000, 0x86003f, seta2_vregs_w, &seta2_vregs	},	// Video Registers
	{ 0x900000, 0x903fff, seta_sound_word_w 			},	// Sound
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	// TMP68301 Registers
MEMORY_END


/***************************************************************************
							Penguin Bros
***************************************************************************/

static MEMORY_READ16_START( penbros_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_ROM					},	// RAM
	{ 0x210000, 0x23ffff, MRA16_ROM					},	// RAM
	{ 0x300000, 0x30ffff, MRA16_RAM					},	// RAM
	{ 0x500300, 0x500301, input_port_0_word_r		},	// DSW 1
	{ 0x500302, 0x500303, input_port_1_word_r		},	// DSW 2
	{ 0x600000, 0x600001, input_port_2_word_r		},	// P1
	{ 0x600002, 0x600003, input_port_3_word_r		},	// P2
	{ 0x600004, 0x600005, input_port_4_word_r		},	// Coins
	{ 0x600006, 0x600007, watchdog_reset16_r		},	// Watchdog
//	{ 0x700000, 0x700001, pzlbowl_protection_r		},	// Protection
	{ 0xb00000, 0xb3ffff, MRA16_RAM					},	// Sprites
	{ 0xb40000, 0xb4ffff, MRA16_RAM					},	// Palette
	{ 0xa00000, 0xa03fff, seta_sound_word_r 		},	// Sound
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	// TMP68301 Registers
MEMORY_END

static MEMORY_WRITE16_START( penbros_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x30ffff, MWA16_RAM						},	// RAM
	{ 0x210000, 0x23ffff, MWA16_RAM						},	// RAM
	{ 0x500300, 0x50030f, seta2_sound_bank_w			},	// Samples Banks
	{ 0x600004, 0x600005, pzlbowl_coin_counter_w		},	// Coins Counter
	{ 0xb00000, 0xb3ffff, MWA16_RAM, &spriteram16,  &spriteram_size	},	// Sprites
	{ 0xb40000, 0xb4ffff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xb60000, 0xb6003f, seta2_vregs_w, &seta2_vregs	},	// Video Registers
	{ 0xa00000, 0xa03fff, seta_sound_word_w 			},	// Sound
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	// TMP68301 Registers
MEMORY_END



/***************************************************************************

								Input Ports

***************************************************************************/

/***************************************************************************
                      Wakakusamonogatari Mahjong Yonshimai
***************************************************************************/

INPUT_PORTS_START( mj4simai )
	PORT_START	// IN0 - $600300.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Tumo Pin" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $600302.w
	PORT_DIPNAME( 0x0007, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0001, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPSETTING(      0x0007, "5" )
	PORT_DIPSETTING(      0x0006, "6" )
	PORT_DIPSETTING(      0x0005, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Select Girl" )
	PORT_DIPSETTING(      0x0010, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Com Put" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $600100.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $600000(0)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1                              )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $600000(1)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_2,        IP_JOY_NONE )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN5 - $600000(2)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6 - $600000(3)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $600000(4)
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "Last",   KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "Take",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "Double", KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "Big",    KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, 0, "Small",  KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT(  0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Kosodate Quiz My Angel
***************************************************************************/

INPUT_PORTS_START( myangel )
	PORT_START	// IN0 - $700300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $700302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Push Start To Freeze (Cheat)", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $700000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $700002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $700004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Kosodate Quiz My Angel 2
***************************************************************************/

INPUT_PORTS_START( myangel2 )
	PORT_START	// IN0 - $600300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $600302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $600000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $600002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $600004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Puzzle De Bowling
***************************************************************************/

INPUT_PORTS_START( pzlbowl )
	PORT_START	// IN0 - $400300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0030, "Easy" )
	PORT_DIPSETTING(      0x0038, "Normal" )
//	PORT_DIPSETTING(      0x0028, "Normal" )
//	PORT_DIPSETTING(      0x0020, "Normal" )
//	PORT_DIPSETTING(      0x0018, "Normal" )
//	PORT_DIPSETTING(      0x0010, "Normal" )
//	PORT_DIPSETTING(      0x0008, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x0040, "1" )
	PORT_DIPSETTING(      0x00c0, "2" )
//	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0080, "3" )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $400302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
//	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
//	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Join In" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPSETTING(      0x0080, "Japanese" )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $500000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $500002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $500004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )	// unused, test mode shows it
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// Protection?

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Penguin Bros
***************************************************************************/

INPUT_PORTS_START( penbros )
	PORT_START	// IN0 - $500300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $500302.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0008, "5" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0010, "150k and 500k" )
	PORT_DIPSETTING(      0x0030, "200k and 700k" )
	PORT_DIPSETTING(      0x0000, "Every 250k" )	// no extra life after the one at 1500k
	PORT_DIPSETTING(      0x0020, "None" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x00c0, "2" )
	PORT_DIPSETTING(      0x0040, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0000, "5" )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $600000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )	// unsure if used
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $600002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )	// unsure if used
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $600004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )	// unused, test mode shows it
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x4_lo =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{	RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static struct GfxLayout layout_8x8x4_hi =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{	RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{	RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

/*	Tiles are 8x8x8, but the hardware is additionally able to discard
	some bitplanes and use the low 4 bits only, or the high 4 bits only	*/
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4_lo,	0,		0x8000/16 }, // [0] Sprites
	{ REGION_GFX1, 0, &layout_8x8x4_hi,	0,		0x8000/16 }, // [1]	""
	{ REGION_GFX1, 0, &layout_8x8x8,	0x8000,	0x8000/16 }, // [2]	""
	{ -1 }
};


/***************************************************************************

								Machine Drivers

***************************************************************************/

static struct x1_010_interface x1_010_sound_intf_16MHz =
{
	50000000/3,	/* clock */
	YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT),	/* volume */
	0x0000,		/* address */
};


static MACHINE_DRIVER_START( mj4simai )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,32530400 / 2)			/* !! TMP68301 !! */
	MDRV_CPU_MEMORY(mj4simai_readmem,mj4simai_writemem)
	MDRV_CPU_VBLANK_INT(seta2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seta2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 0x200)
	MDRV_VISIBLE_AREA(0x40, 0x1c0-1, 0x80, 0x170-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH((0x8000/16)*16 + (0x8000/16)*256)

	MDRV_PALETTE_INIT(seta2)
	MDRV_VIDEO_START(seta2)
	MDRV_VIDEO_UPDATE(seta2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(X1_010, x1_010_sound_intf_16MHz)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( myangel )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(myangel_readmem,myangel_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x178-1, 0x00, 0xf0-1)

	MDRV_VIDEO_START(seta2_offset)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( myangel2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(myangel2_readmem,myangel2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x178-1, 0x00, 0xf0-1)

	MDRV_VIDEO_START(seta2_offset)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pzlbowl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(pzlbowl_readmem,pzlbowl_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0x10, 0x190-1, 0x100, 0x1f0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( penbros )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(penbros_readmem,penbros_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x140-1, 0x80, 0x160-1)
MACHINE_DRIVER_END


/***************************************************************************

								ROMs Loading

***************************************************************************/


ROM_START( mj4simai )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "ll.u2",       0x000000, 0x080000, 0x7be9c781 )
	ROM_LOAD16_BYTE( "lh1.u3",      0x000001, 0x080000, 0x82aa3f72 )
	ROM_LOAD16_BYTE( "hl.u4",       0x100000, 0x080000, 0x226063b7 )
	ROM_LOAD16_BYTE( "hh.u5",       0x100001, 0x080000, 0x23aaf8df )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "cha-03.u16",  0x0000000, 0x400000, 0xd367429a )
	ROM_LOAD( "cha-04.u18",  0x0400000, 0x400000, 0x7f2008c3 )
	ROM_LOAD( "cha-05.u15",  0x0800000, 0x400000, 0xe94ec40a )
	ROM_LOAD( "cha-06.u17",  0x0c00000, 0x400000, 0x5cb0b3a9 )
	ROM_LOAD( "cha-01.u21",  0x1000000, 0x400000, 0x35f47b37 )
	ROM_LOAD( "cha-02.u22",  0x1400000, 0x400000, 0xf6346860 )
	ROM_FILL(                0x1800000, 0x800000, 0 )	/* 6bpp instead of 8bpp */

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "cha-07.u32",  0x100000, 0x400000, 0x817519ee )
ROM_END

ROM_START( myangel )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kq1-prge.u2", 0x000000, 0x080000, 0x6137d4c0 )
	ROM_LOAD16_BYTE( "kq1-prgo.u3", 0x000001, 0x080000, 0x4aad10d8 )
	ROM_LOAD16_BYTE( "kq1-tble.u4", 0x100000, 0x080000, 0xe332a514 )
	ROM_LOAD16_BYTE( "kq1-tblo.u5", 0x100001, 0x080000, 0x760cab15 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kq1-cg2.u20", 0x000000, 0x200000, 0x80b4e8de )
	ROM_LOAD( "kq1-cg0.u16", 0x200000, 0x200000, 0xf8ae9a05 )
	ROM_LOAD( "kq1-cg3.u19", 0x400000, 0x200000, 0x9bdc35c9 )
	ROM_LOAD( "kq1-cg1.u15", 0x600000, 0x200000, 0x23bd7ea4 )
	ROM_LOAD( "kq1-cg6.u22", 0x800000, 0x200000, 0xb25acf12 )
	ROM_LOAD( "kq1-cg4.u18", 0xa00000, 0x200000, 0xdca7f8f2 )
	ROM_LOAD( "kq1-cg7.u21", 0xc00000, 0x200000, 0x9f48382c )
	ROM_LOAD( "kq1-cg5.u17", 0xe00000, 0x200000, 0xa4bc4516 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kq1-snd.u32", 0x100000, 0x200000, 0x8ca1b449 )
ROM_END

ROM_START( myangel2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kqs1ezpr.u2", 0x000000, 0x080000, 0x2469aac2 )
	ROM_LOAD16_BYTE( "kqs1ozpr.u3", 0x000001, 0x080000, 0x6336375c )
	ROM_LOAD16_BYTE( "kqs1e-tb.u4", 0x100000, 0x080000, 0xe759b4cc )
	ROM_LOAD16_BYTE( "kqs1o-tb.u5", 0x100001, 0x080000, 0xb6168737 )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kqs1-cg4.u20", 0x0000000, 0x200000, 0xd1802241 )
	ROM_LOAD( "kqs1-cg0.u16", 0x0200000, 0x400000, 0xc21a33a7 )
	ROM_LOAD( "kqs1-cg5.u19", 0x0600000, 0x200000, 0xd86cf19c )
	ROM_LOAD( "kqs1-cg1.u15", 0x0800000, 0x400000, 0xdca799ba )
	ROM_LOAD( "kqs1-cg6.u22", 0x0c00000, 0x200000, 0x3f08886b )
	ROM_LOAD( "kqs1-cg2.u18", 0x0e00000, 0x400000, 0xf7f92c7e )
	ROM_LOAD( "kqs1-cg7.u21", 0x1200000, 0x200000, 0x2c977904 )
	ROM_LOAD( "kqs1-cg3.u17", 0x1400000, 0x400000, 0xde3b2191 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kqs1-snd.u32", 0x100000, 0x400000, 0x792a6b49 )
ROM_END

ROM_START( pzlbowl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kup-u06.i03", 0x000000, 0x080000, 0x314e03ac )
	ROM_LOAD16_BYTE( "kup-u07.i03", 0x000001, 0x080000, 0xa0423a04 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kuc-u38.i00", 0x000000, 0x400000, 0x3db24172 )
	ROM_LOAD( "kuc-u39.i00", 0x400000, 0x400000, 0x9b26619b )
	ROM_LOAD( "kuc-u40.i00", 0x800000, 0x400000, 0x7e49a2cf )
	ROM_LOAD( "kuc-u41.i00", 0xc00000, 0x400000, 0x2febf19b )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kus-u18.i00", 0x100000, 0x400000, 0xe2b1dfcf )
ROM_END

ROM_START( penbros )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "u06.bin", 0x000000, 0x080000, 0x7bbdffac )
	ROM_LOAD16_BYTE( "u07.bin", 0x000001, 0x080000, 0xd50cda5f )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "u38.bin", 0x000000, 0x400000, 0x4247b39e )
	ROM_LOAD( "u39.bin", 0x400000, 0x400000, 0xf9f07faf )
	ROM_LOAD( "u40.bin", 0x800000, 0x400000, 0xdc9e0a96 )
	ROM_FILL(            0xc00000, 0x400000, 0 )	/* 6bpp instead of 8bpp */

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "u18.bin", 0x100000, 0x200000, 0xde4e65e2 )
ROM_END



DRIVER_INIT( pzlbowl )
{
	data16_t *ROM = (data16_t *)memory_region( REGION_CPU1 );

	/* Patch out the protection check */
	ROM[0x01d6/2] = 0x4e73;		// trap #0 routine becomes rte
}



GAMEX( 1996, mj4simai, 0, mj4simai, mj4simai, 0,        ROT0, "Maboroshi Ware",      "Wakakusamonogatari Mahjong Yonshimai (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1996, myangel,  0, myangel,  myangel,  0,        ROT0, "Namco",               "Kosodate Quiz My Angel (Japan)",   GAME_NO_COCKTAIL )
GAMEX( 1997, myangel2, 0, myangel2, myangel2, 0,        ROT0, "Namco",               "Kosodate Quiz My Angel 2 (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
GAMEX( 1999, pzlbowl,  0, pzlbowl,  pzlbowl,  pzlbowl,  ROT0, "Nihon System / Moss", "Puzzle De Bowling (Japan)",        GAME_NO_COCKTAIL )
GAMEX( 2000, penbros,  0, penbros,  penbros,  0,  		ROT0, "Subsino",             "Penguin Brothers (Japan)",         GAME_NO_COCKTAIL )
