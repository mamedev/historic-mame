/***************************************************************************

					-= Seta, Sammy, Visco (SSV) System =-

					driver by	Luca Elia (l.elia@tin.it)


CPU          :		NEC V60

Sound Chip   :		Ensoniq ES5506 (OTTOR2)

Custom Chips :		ST-0004		(Video DAC)
					ST-0005		(Parallel I/O)
					ST-0006		(Video controller)
					ST-0007		(System controller)

Others       :		Battery + MB3790 + LH5168D-10L (NVRAM)
					DX-102				(I/O)
					M62X42B				(RTC)
					ST010
					TA8210				(Audio AMP)
					uPD71051/7001C		(UART)

-----------------------------------------------------------------------------------
Main Board	ROM Board	Year + Game									By
-----------------------------------------------------------------------------------
?			?			93	Super Real Mahjong PIV					Seta
?			STS-0001	93	Dramatic Adventure Quiz Keith & Lucy	Visco
?			?			93	Survival Arts							Sammy
STA-0001B	VISCO-001B	94	Drift Out '94							Visco
?			?			94	Twin Eagle II - The Rescue Mission		Seta
?			P1-102A		95	Mahjong Hyper Reaction					Sammy
?			?			96	Lovely Pop Mahjong Jan Jan Shimasyo		Visco
?			P1-105A		96?	Meosis Magic							Sammy
?			P1-112A		97	Mahjong Hyper Reaction 2				Sammy
?			?			97	Monster Slider							Visco / Datt
?			?			97	Super Real Mahjong P7					Seta
?			?			98	Gourmet Battle Quiz Ryorioh CooKing		Visco
?			P1-112C		98	Pachinko Sexy Reaction					Sammy
-----------------------------------------------------------------------------------

Games not yet dumped:
						?	Kidou Senshi Gundam Final Shooting		Visco / Banpresto
						?	Koi Koi Shimasyo 2 Super Real Hanafuda	Visco
						?	Pachinko Sexy Reaction 2				Sammy
						97	Joryuu Syougi Kyoushitsu				Visco
						00	Vasara									Visco
						01	Vasara 2								Visco


To Do:

- all games	:	CRT controller (resolution+visible area+flip screen?)
				Proper shadows (flickering ones for now). Games use too
				many colors anyway, so the user interface would break
				when using VIDEO_HAS_SHADOWS. Plus I think this actually
				is alpha-blending (mslider). We have an additional problem
				in that case since I'm using a colortable.

- hypreac2	:	communication with other units
				"Warning" size is wrong, so it's kludged to work
				tilemap sprites use the yoffset specified in the sprites-list?
				(see the 8 pixel gap between the backgrounds and the black rows)

- srmp4		:	Backgrounds are offset by $60 pixels, so they're kludged to work

- srmp7		:	Needs interrupts by the sound chip (unsupported yet). Kludged to work.

- mslider   :   V60 issues?  the correct tiles don't always vanish, depending on
                the layout of the 3 tiles you put together sometimes only 2 vanish
                and a blank are off the bottom of the game board flashes instead
                of the correct 3rd tile

- eaglshot  :   GOLF rom board still needs mapping correctly.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "seta.h"


/***************************************************************************


								Interrupts


***************************************************************************/

static UINT8 requested_int;
static data16_t *ssv_irq_vectors;

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	cpu_set_irq_line(0, 0, requested_int ? ASSERT_LINE : CLEAR_LINE);
}

int ssv_irq_callback(int level)
{
	int i;
	for ( i = 0; i <= 7; i++ )
	{
		if (requested_int & (1 << i))
		{
			data16_t vector = ssv_irq_vectors[i * (16/2)] & 7;
			return vector;
		}
	}
	return 0;
}

WRITE16_HANDLER( ssv_irq_ack_w )
{
	int level = ((offset * 2) & 0x70) >> 4;
	requested_int &= ~(1 << level);
	update_irq_state();
}

INTERRUPT_GEN( ssv_interrupt )
{
	requested_int |= 1 << 3;
	update_irq_state();
}


/***************************************************************************


							Coins Lockout / Counter


***************************************************************************/

/*
	drifto94:	c3
	janjans1:	c3
	keithlcy:	c3
	mslider:	c3, 83 in test mode
	ryorioh:	c3

	hypreac2:	80
	hypreact:	80
	meosism:	83
	srmp4:		83, c0 in test mode (where only tilemap sprites are used)
	srmp7:		80
	survarts:	83
	sxyreact:	80
*/
static WRITE16_HANDLER( ssv_lockout_w )
{
//	usrintf_showmessage("%02X",data & 0xff);
	if (ACCESSING_LSB)
	{
		coin_lockout_w(1,~data & 0x01);
		coin_lockout_w(0,~data & 0x02);
		coin_counter_w(1, data & 0x04);
		coin_counter_w(0, data & 0x08);
//		                  data & 0x40?
		ssv_enable_video( data & 0x80);
	}
}

/* Same as above but with inverted lockout lines */
static WRITE16_HANDLER( ssv_lockout_inv_w )
{
//	usrintf_showmessage("%02X",data & 0xff);
	if (ACCESSING_LSB)
	{
		coin_lockout_w(1, data & 0x01);
		coin_lockout_w(0, data & 0x02);
		coin_counter_w(1, data & 0x04);
		coin_counter_w(0, data & 0x08);
//		                  data & 0x40?
		ssv_enable_video( data & 0x80);
	}
}

MACHINE_INIT( ssv )
{
	requested_int = 0;
	cpu_set_irq_callback(0, ssv_irq_callback);
	cpu_setbank(1, memory_region(REGION_USER1));
}


/***************************************************************************


							Non-Volatile RAM


***************************************************************************/

static data16_t *ssv_nvram;
static size_t    ssv_nvram_size;

NVRAM_HANDLER( ssv )
{
	if (read_or_write)
		mame_fwrite(file, ssv_nvram, ssv_nvram_size);
	else
		if (file)
			mame_fread(file, ssv_nvram, ssv_nvram_size);
}


/***************************************************************************


								Memory Maps


***************************************************************************/

//static READ16_HANDLER( fake_r )	{	return ssv_scroll[offset];	}

static data16_t *ssv_input_sel;


#define SSV_READMEM( _ROM  )										\
	{ 0x000000, 0x00ffff, MRA16_RAM				},	/*	RAM		*/	\
	{ 0x100000, 0x13ffff, MRA16_RAM				},	/*	Sprites	*/	\
	{ 0x140000, 0x15ffff, MRA16_RAM				},	/*	Palette	*/	\
	{ 0x160000, 0x17ffff, MRA16_RAM				},	/*			*/	\
	{ 0x1c0000, 0x1c0001, ssv_vblank_r			},	/*	Vblank?	*/	\
/**/{ 0x1c0002, 0x1c007f, MRA16_RAM				},	/*	Scroll	*/	\
	{ 0x210002, 0x210003, input_port_0_word_r	},	/*	DSW		*/	\
	{ 0x210004, 0x210005, input_port_1_word_r	},	/*	DSW		*/	\
	{ 0x210008, 0x210009, input_port_2_word_r	},	/*	P1		*/	\
	{ 0x21000a, 0x21000b, input_port_3_word_r	},	/*	P2		*/	\
	{ 0x21000c, 0x21000d, input_port_4_word_r	},	/*	Coins	*/	\
	{ 0x21000e, 0x21000f, MRA16_NOP				},	/*			*/	\
	{ 0x300000, 0x30007f, ES5506_data_0_word_r	},	/*	Sound	*/	\
	{ _ROM,     0xffffff, MRA16_BANK1			},	/*	ROM		*/	\
/*{ 0x990000, 0x99007f, fake_r	},*/

#define SSV_WRITEMEM														\
	{ 0x000000, 0x00ffff, MWA16_RAM						},	/*	RAM			*/	\
	{ 0x100000, 0x13ffff, MWA16_RAM, &spriteram16		},	/*	Sprites		*/	\
	{ 0x140000, 0x15ffff, paletteram16_xrgb_swap_word_w, &paletteram16	},		\
	{ 0x160000, 0x17ffff, MWA16_RAM						},	/*				*/	\
	{ 0x1c0000, 0x1c007f, ssv_scroll_w, &ssv_scroll		},	/*	Scroll		*/	\
	{ 0x21000e, 0x21000f, ssv_lockout_w					},	/*	Lockout		*/	\
	{ 0x210010, 0x210011, MWA16_NOP						},	/*				*/	\
	{ 0x230000, 0x230071, MWA16_RAM, &ssv_irq_vectors	},	/*	IRQ Vectors	*/	\
	{ 0x240000, 0x240071, ssv_irq_ack_w					},	/*	IRQ Ack.	*/	\
	{ 0x300000, 0x30007f, ES5506_data_0_word_w			},	/*	Sound		*/


/***************************************************************************
								Drift Out '94
***************************************************************************/

static READ16_HANDLER( drifto94_rand_r )
{
	return rand();
}

static READ16_HANDLER( drifto94_482022_r )
{
	return 0x009b;
}

static MEMORY_READ16_START( drifto94_readmem )
	{ 0x480000, 0x480000, MRA16_NOP				},	// ?
	{ 0x482022, 0x482023, drifto94_482022_r		},	// ?? protection?
	{ 0x482042, 0x482043, MRA16_NOP				},	// ?? protection?
	{ 0x510000, 0x510001, drifto94_rand_r		},	// ??
	{ 0x520000, 0x520001, drifto94_rand_r		},	// ??
	{ 0x580000, 0x5807ff, MRA16_RAM				},	// NVRAM
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( drifto94_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP				},	// ? 1 at the start
//	{ 0x260000, 0x260001, MWA16_NOP				},	// ? c at the start
	{ 0x400000, 0x47ffff, MWA16_RAM				},	// ?
	{ 0x480000, 0x480000, MWA16_NOP				},	// ?
	{ 0x482000, 0x485fff, MWA16_NOP				},	// ?
	{ 0x500000, 0x500001, MWA16_NOP				},	// ??
	{ 0x580000, 0x5807ff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size	},	// NVRAM
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Hyper Reaction
***************************************************************************/

/*
	The game prints "backup ram ok" and there is code to test some ram
	at 0x580000-0x5bffff. The test is skipped and this ram isn't used
	though. I guess it's either a left-over or there are different
	version with some battery backed RAM (which would indeed be on the
	rom-board, AFAIK)
*/

static READ16_HANDLER( hypreact_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0001)	return readinputport(5);
	if (input_sel & 0x0002)	return readinputport(6);
	if (input_sel & 0x0004)	return readinputport(7);
	if (input_sel & 0x0008)	return readinputport(8);
	logerror("CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( hypreact_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	// Watchdog
//	{ 0x280000, 0x280001, MRA16_NOP					},	// ? read at the start, value not used
	{ 0xc00000, 0xc00001, hypreact_input_r			},	// Inputs
	{ 0xc00006, 0xc00007, MRA16_RAM					},	//
	{ 0xc00008, 0xc00009, MRA16_NOP					},	//
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( hypreact_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP					},	// ? 5 at the start
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	// Inverted lockout lines
//	{ 0x260000, 0x260001, MWA16_NOP					},	// ? ff at the start
	{ 0xc00006, 0xc00007, MWA16_RAM, &ssv_input_sel	},	// Inputs
	{ 0xc00008, 0xc00009, MWA16_NOP					},	//
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Hyper Reaction 2
***************************************************************************/

static MEMORY_READ16_START( hypreac2_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	// Watchdog
//	{ 0x280000, 0x280001, MRA16_NOP					},	// ? read at the start, value not used
	{ 0x500000, 0x500001, hypreact_input_r			},	// Inputs
	{ 0x500002, 0x500003, hypreact_input_r			},	// (again?)
//	  0x540000, 0x540003  communication with another unit
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( hypreac2_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP					},	// ? 5 at the start
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	// Inverted lockout lines
//	{ 0x260000, 0x260001, MWA16_NOP					},	// ? ff at the start
	{ 0x520000, 0x520001, MWA16_RAM, &ssv_input_sel	},	// Inputs
//	  0x540000, 0x540003  communication with other units
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Jan Jan Simasyo
***************************************************************************/

static READ16_HANDLER( srmp4_input_r );

static MEMORY_READ16_START( janjans1_readmem )
	{ 0x210006, 0x210007, MRA16_NOP					},
	{ 0x800002, 0x800003, srmp4_input_r				},	// Inputs
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( janjans1_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP					},	// ? 1 at the start
//	{ 0x260000, 0x260001, MWA16_NOP					},	// ? 0,6c,60
	{ 0x800000, 0x800000, MWA16_RAM, &ssv_input_sel	},	// Inputs
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Keith & Lucy
***************************************************************************/

static MEMORY_READ16_START( keithlcy_readmem )
	{ 0x21000e, 0x21000f, MRA16_NOP			},	//
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( keithlcy_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP			},	// ? 1 at the start
//	{ 0x260000, 0x260001, MWA16_NOP			},	// ? c at the start
	{ 0x210010, 0x210011, MWA16_NOP			},	//
	{ 0x400000, 0x47ffff, MWA16_RAM			},	// ?
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Meosis Magic
***************************************************************************/

static MEMORY_READ16_START( meosism_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	// Watchdog
//	{ 0x280000, 0x280001, MRA16_NOP				},	// ? read once, value not used
	{ 0x580000, 0x58ffff, MRA16_RAM				},	// NVRAM
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( meosism_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP				},	// ? 5 at the start
//	{ 0x260000, 0x260001, MWA16_NOP				},	// ? ff at the start
//	{ 0x500004, 0x500005, MWA16_NOP				},	// ? 0,58,18
	{ 0x580000, 0x58ffff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size	},	// NVRAM
	SSV_WRITEMEM
MEMORY_END

/***************************************************************************
								Monster Slider
***************************************************************************/

static MEMORY_READ16_START( mslider_readmem )
	{ 0x010000, 0x01ffff, MRA16_RAM			},	// More RAM
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( mslider_writemem )
	{ 0x010000, 0x01ffff, MWA16_RAM			},	// More RAM
//	{ 0x210002, 0x210003, MWA16_NOP			},	// ? 1 at the start
//	{ 0x260000, 0x260001, MWA16_NOP			},	// ? c at the start
	{ 0x400000, 0x47ffff, MWA16_RAM			},	// ?
//	{ 0x500000, 0x500001, MWA16_NOP			},	// ? ff at the start
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
					Gourmet Battle Quiz Ryorioh CooKing
***************************************************************************/

static MEMORY_READ16_START( ryorioh_readmem )
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( ryorioh_writemem )
	{ 0x210000, 0x210001, watchdog_reset16_w	},	// Watchdog
//	{ 0x210002, 0x210003, MWA16_NOP				},	// ? 1 at the start
//	{ 0x260000, 0x260001, MWA16_NOP				},	// ? 0,c at the start
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Super Real Mahjong PIV
***************************************************************************/

static READ16_HANDLER( srmp4_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0002)	return readinputport(5);
	if (input_sel & 0x0004)	return readinputport(6);
	if (input_sel & 0x0008)	return readinputport(7);
	if (input_sel & 0x0010)	return readinputport(8);
	logerror("CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( srmp4_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	// Watchdog
	{ 0xc0000a, 0xc0000b, srmp4_input_r				},	// Inputs
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( srmp4_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP					},	// ? 1,5 at the start
//	{ 0x260000, 0x260001, MWA16_NOP					},	// ? 8 at the start
	{ 0xc0000e, 0xc0000f, MWA16_RAM, &ssv_input_sel	},	// Inputs
	{ 0xc00010, 0xc00011, MWA16_NOP					},	//
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Super Real Mahjong P7
***************************************************************************/

/*
	Interrupts aren't supported by the chip emulator yet
	(lev 5 in this case, I guess)
*/
static READ16_HANDLER( srmp7_irqv_r )
{
	return 0x0080;
}

static WRITE16_HANDLER( srmp7_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		int bank = 0x400000 * (data & 1);
		ES5506_voice_bank_0_w(2, bank);
		ES5506_voice_bank_0_w(3, bank);
	}
//	usrintf_showmessage("%04X",data);
}

static READ16_HANDLER( srmp7_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0002)	return readinputport(5);
	if (input_sel & 0x0004)	return readinputport(6);
	if (input_sel & 0x0008)	return readinputport(7);
	if (input_sel & 0x0010)	return readinputport(8);
	logerror("CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( srmp7_readmem )
	{ 0x010000, 0x050faf, MRA16_RAM				},	// More RAM
	{ 0x210000, 0x210001, watchdog_reset16_r	},	// Watchdog
	{ 0x300076, 0x300077, srmp7_irqv_r			},	// Sound
//	  0x540000, 0x540003, related to lev 5 irq?
	{ 0x600000, 0x600001, srmp7_input_r			},	// Inputs
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( srmp7_writemem )
	{ 0x010000, 0x050faf, MWA16_RAM					},	// More RAM
//	{ 0x210002, 0x210003, MWA16_NOP					},	// ? 0,4 at the start
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	// Coin Counters / Lockouts
//	{ 0x260000, 0x260001, MWA16_NOP					},	// ? 8 at the start, 28, 40 (seems related to 21000e writes)
//	  0x540000, 0x540003, related to lev 5 irq?
	{ 0x580000, 0x580001, srmp7_sound_bank_w		},	// Sound Bank
	{ 0x680000, 0x680001, MWA16_RAM, &ssv_input_sel	},	// Inputs
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Survival Arts
***************************************************************************/

static MEMORY_READ16_START( survarts_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	// Watchdog
//	{ 0x290000, 0x290001, MRA16_NOP				},	// ?
//	{ 0x2a0000, 0x2a0001, MRA16_NOP				},	// ?
	{ 0x500008, 0x500009, input_port_5_word_r	},	// Extra Buttons
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( survarts_writemem )
//	{ 0x210002, 0x210003, MWA16_NOP				},	// ? 0,4 at the start
//	{ 0x260000, 0x260001, MWA16_NOP				},	// ? 0,8 at the start
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Pachinko Sexy Reaction
***************************************************************************/

static data16_t serial;

static READ16_HANDLER( sxyreact_ballswitch_r )
{
	return readinputport(5);
}

static READ16_HANDLER( sxyreact_dial_r )
{
	return ((serial >> 1) & 0x80);
}

static WRITE16_HANDLER( sxyreact_dial_w )
{
	if (ACCESSING_LSB)
	{
		static int old;

		if (data & 0x20)
			serial = readinputport(6) & 0xff;

		if ( (old & 0x40) && !(data & 0x40) )	// $40 -> $00
			serial <<= 1;						// shift 1 bit

		old = data;
	}
}

static WRITE16_HANDLER( sxyreact_motor_w )
{
//	usrintf_showmessage("%04X",data);	// 8 = motor on; 0 = motor off
}

static MEMORY_READ16_START( sxyreact_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	// Watchdog
	{ 0x500002, 0x500003, sxyreact_ballswitch_r	},	// ?
	{ 0x500004, 0x500005, sxyreact_dial_r		},	// Dial Value (serial)
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( sxyreact_writemem )
//	{ 0x210002, 0x210002, MWA16_NOP				},	// ? 1 at the start
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w		},	// Inverted lockout lines
//	{ 0x260000, 0x260001, MWA16_NOP				},	// ? ff at the start
	{ 0x520000, 0x520001, sxyreact_dial_w		},	// Dial Value (advance 1 bit)
	{ 0x520004, 0x520005, sxyreact_motor_w		},	// Dial Motor?
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Twin Eagle II
***************************************************************************/

static MEMORY_READ16_START( twineag2_readmem )
	{ 0x010000, 0x03ffff, MRA16_RAM				},	// More RAM
	{ 0x482022, 0x482023, drifto94_482022_r		},	// ?? protection ??
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( twineag2_writemem )
	{ 0x010000, 0x03ffff, MWA16_RAM				},	// More RAM
	SSV_WRITEMEM
MEMORY_END



/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Drift Out '94
***************************************************************************/

INPUT_PORTS_START( drifto94 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Sound Test" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(      0x000c, "11 (0)" )
	PORT_DIPSETTING(      0x0008, "10 (1)" )
	PORT_DIPSETTING(      0x0004, "01 (0)" )
	PORT_DIPSETTING(      0x0000, "00 (2)" )
	PORT_DIPNAME( 0x0010, 0x0010, "Music Volume" )
	PORT_DIPSETTING(      0x0000, "Quiet" )
	PORT_DIPSETTING(      0x0010, "Loud" )
	PORT_DIPNAME( 0x0020, 0x0020, "Sound Volume" )
	PORT_DIPSETTING(      0x0000, "Quiet" )
	PORT_DIPSETTING(      0x0020, "Loud" )
	PORT_DIPNAME( 0x0040, 0x0040, "Save Best Time" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Hyper Reaction
***************************************************************************/

INPUT_PORTS_START( hypreact )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Half Coins To Continue" )
	PORT_DIPSETTING(      0x0040, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy"    )
	PORT_DIPSETTING(      0x000c, "Normal"  )
	PORT_DIPSETTING(      0x0004, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Keyboard" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN2 - $210008 (used in joystick mode)
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )

	PORT_START	// IN3 - $21000a (used in joystick mode)
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "Tsumo", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN        )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN        )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )	// service coin & bet
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN5 - $c00000(0)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1                              )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6 - $c00000(1)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $c00000(2)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8 - $c00000(3)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Hyper Reaction 2
***************************************************************************/

INPUT_PORTS_START( hypreac2 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Half Coins To Continue" )
	PORT_DIPSETTING(      0x0040, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Keyboard" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, "Communication 1" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Communication 2" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN5 - $500000(0)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1                              )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6 - $500000(1)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $500000(2)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8 - $500000(3)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Jan Jan Simasyo
***************************************************************************/

INPUT_PORTS_START( janjans1 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Mini Game" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, "Initial Score" )
	PORT_DIPSETTING(      0x0020, "1000" )
	PORT_DIPSETTING(      0x0030, "1500" )
	PORT_DIPSETTING(      0x0010, "2000" )
	PORT_DIPSETTING(      0x0000, "3000" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Communication" )
//	PORT_DIPSETTING(      0x0080, "unused" )
	PORT_DIPSETTING(      0x00c0, "None" )
	PORT_DIPSETTING(      0x0040, "Board 1 (Main)" )
	PORT_DIPSETTING(      0x0000, "Board 2 (Sub)" )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN5 - $800002(0)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6 - $800002(1)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $800002(2)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8 - $800002(3)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Keith & Lucy
***************************************************************************/

INPUT_PORTS_START( keithlcy )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"	)	// 15 sec
	PORT_DIPSETTING(      0x0003, "Normal"	)	// 12
	PORT_DIPSETTING(      0x0001, "Hard"	)	// 10
	PORT_DIPSETTING(      0x0000, "Hardest"	)	// 8
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0030, "100k" )	//100
	PORT_DIPSETTING(      0x0020, "150k" )	//150
	PORT_DIPSETTING(      0x0010, "200k" )	//100
	PORT_DIPSETTING(      0x0000, "200k?" )	//200
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Meosis Magic
***************************************************************************/

INPUT_PORTS_START( meosism )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0003, "1 Medal/1 Credit" )
	PORT_DIPSETTING(      0x0001, "1 Medal/5 Credits" )
	PORT_DIPSETTING(      0x0002, "1 Medal/10 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Medal/20 Credits" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Attendant Pay" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Medals Payout" )
	PORT_DIPSETTING(      0x0010, "400" )
	PORT_DIPSETTING(      0x0000, "800" )
	PORT_DIPNAME( 0x0020, 0x0020, "Max Credits" )
	PORT_DIPSETTING(      0x0020, "5000" )
	PORT_DIPSETTING(      0x0000, "9999" )
	PORT_DIPNAME( 0x0040, 0x0040, "Hopper" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Reel Speed" )
	PORT_DIPSETTING(      0x0080, "Low" )
	PORT_DIPSETTING(      0x0000, "High" )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0003, 0x0003, "Game Rate" )
	PORT_DIPSETTING(      0x0000, "80%" )
	PORT_DIPSETTING(      0x0002, "85%" )
	PORT_DIPSETTING(      0x0003, "90%" )
	PORT_DIPSETTING(      0x0001, "95%" )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Controls" )
//	PORT_DIPSETTING(      0x0020, "Simple") )
	PORT_DIPSETTING(      0x0000, "Complex" )
	PORT_DIPNAME( 0x0040, 0x0000, "Coin Sensor" )
	PORT_DIPSETTING(      0x0040, "Active High" )
	PORT_DIPSETTING(      0x0000, "Active Low" )
	PORT_DIPNAME( 0x0080, 0x0080, "Hopper Sensor" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON4        )	//bet
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        )	//stop/r
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        )	//stop/c
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        )	//stop/l
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	//no
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )	//yes
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_START1         )	//start
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN        )	//-

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//-
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	//test
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//-
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE3 )	//payout
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//-
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_TILT     )	//reset
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//-
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )	//-

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )	//service coin
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )	//analyzer
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON5  )	//max bet
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Monster Slider
***************************************************************************/

INPUT_PORTS_START( mslider )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, "Rounds (Vs Mode)" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
					Gourmet Battle Quiz Ryorioh CooKing
***************************************************************************/

INPUT_PORTS_START( ryorioh )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7*" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 2-0&1*" )
	PORT_DIPSETTING(      0x0002, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
							Super Real Mahjong PIV
***************************************************************************/

INPUT_PORTS_START( srmp4 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty )  )
	PORT_DIPSETTING(      0x0007, "1" )
	PORT_DIPSETTING(      0x0006, "2" )
	PORT_DIPSETTING(      0x0005, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0003, "5" )
	PORT_DIPSETTING(      0x0002, "6" )
	PORT_DIPSETTING(      0x0001, "7" )
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN5 - $c0000a(0)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6 - $c0000a(1)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $c0000a(2)
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8 - $c0000a(3)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Super Real Mahjong P7
***************************************************************************/

INPUT_PORTS_START( srmp7 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Re-cloth" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, "1 (Easy)" )
	PORT_DIPSETTING(      0x0005, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0007, "4" )
	PORT_DIPSETTING(      0x0003, "5" )
	PORT_DIPSETTING(      0x0002, "6" )
	PORT_DIPSETTING(      0x0001, "7" )
	PORT_DIPSETTING(      0x0000, "8 (Hard)" )
	PORT_DIPNAME( 0x0008, 0x0008, "Kuitan" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN  )	// tested
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN6 - $600000(0)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7 - $600000(1)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8 - $600000(2)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN5 - $600000(3)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Survival Arts
***************************************************************************/

INPUT_PORTS_START( survarts )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, "6 Coins/4 Credits" )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
//	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0050, "6 Coins/4 Credits" )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
//	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2*" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, "Unknown 2-4&5*" )
	PORT_DIPSETTING(      0x0010, "0" )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Unknown 2-6&7*" )
	PORT_DIPSETTING(      0x0040, "0" )
	PORT_DIPSETTING(      0x00c0, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x0000, "3" )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	// IN5 - $500008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Pachinko Sexy Reaction
***************************************************************************/

INPUT_PORTS_START( sxyreact )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_BIT(     0x0038, IP_ACTIVE_LOW, IPT_UNUSED )
//	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
//	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
//	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
//	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
//	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
//	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Credits To Play" )
	PORT_DIPSETTING(      0x0040, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0080, 0x0080, "Buy Balls With Credits" )	// press start
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Difficulty?" )
	PORT_DIPSETTING(      0x0008, "Easy?" )
	PORT_DIPSETTING(      0x000c, "Normal?" )
	PORT_DIPSETTING(      0x0004, "Hard?" )
	PORT_DIPSETTING(      0x0000, "Hardest?" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Dial" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )	// -> ball sensor on
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// (player 2, only shown in test mode)

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED   )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, "Test Advance", KEYCODE_F1, IP_JOY_DEFAULT )
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN5 - $500002
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH,  IPT_SERVICE2 )	// ball switch on -> handle motor off

	PORT_START	// IN6 - $500004
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE, 15, 15, 0, 0xcf, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END


/***************************************************************************
								Twin Eagle II
***************************************************************************/

INPUT_PORTS_START( twineag2 )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Storm Blade
***************************************************************************/

INPUT_PORTS_START( stmblade )
	PORT_START	// IN0 - $210002
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Rapid Fire" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1 - $210004
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0040, "600000" )
	PORT_DIPSETTING(      0x0000, "800000" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN2 - $210008
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3 - $21000a
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4 - $21000c
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END

/***************************************************************************
                       Change Air Blade
***************************************************************************/

INPUT_PORTS_START( cairblad )
	PORT_START	// IN0
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Rapid Fire" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN1
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0040, "600000" )
	PORT_DIPSETTING(      0x0000, "800000" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	// IN3
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN4
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END

/***************************************************************************


							Graphics Layouts


***************************************************************************/

/*	16 x 8 tiles. Depth is 8 bits, but can be decreased to 6 (and maybe
	less) at runtime.	*/

static struct GfxLayout layout_16x8x8 =
{
	16,8,
	RGN_FRAC(1,4),
	8,
	{	RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8, RGN_FRAC(0,4)+0	},
	{	STEP8(0,1), STEP8(16,1)	},
	{	STEP8(0,16*2)	},
	16*8*2
};

static struct GfxDecodeInfo ssv_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x8x8, 0, 0x8000/64 * 2 }, // Sprites
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ REGION_SOUND3 },
	{ REGION_SOUND4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
	{ 0 }
};

/* Average clock cycles per instruction (12?) */
#define AVERAGE_CPI		(12)

#define CLOCK_12MHz			(12000000 / AVERAGE_CPI)
#define CLOCK_16MHz			(16000000 / AVERAGE_CPI)
/* When clock is unknown */
#define CLOCK_12MHz_UNK		CLOCK_12MHz
#define CLOCK_16MHz_UNK		CLOCK_16MHz

/***************************************************************************

	Some games (e.g. hypreac2) oddly map the high bits of the tile code
	to the gfx roms: arranging the roms accordingly would waste tens of
	megabytes. So we use a look-up table.

	We also need to set up game	specific offsets for sprites and layers
	(at least until the CRT	controlled will be emulated).

***************************************************************************/

void init_ssv(void)
{
	int i;
	for (i = 0; i < 16; i++)
		ssv_tile_code[i]	=	( (i & 8) ? (1 << 16) : 0 ) +
								( (i & 4) ? (2 << 16) : 0 ) +
								( (i & 2) ? (4 << 16) : 0 ) +
								( (i & 1) ? (8 << 16) : 0 ) ;
	ssv_enable_video(1);
	ssv_special = 0;
}

void hypreac2_init(void)
{
	int i;
	for (i = 0; i < 16; i++)
		ssv_tile_code[i]	=	(i << 16);

	ssv_enable_video(1);
	ssv_special = 1;
}


DRIVER_INIT( drifto94 )	{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( hypreact )	{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf7;	}
DRIVER_INIT( hypreac2 )	{	hypreac2_init();	// different
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf8;	}
DRIVER_INIT( janjans1 )	{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( keithlcy )	{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf1;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( meosism )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( mslider )		{	init_ssv();
								ssv_sprites_offsx =-16;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +8;	ssv_tilemap_offsy = -0xf1;	}
DRIVER_INIT( ryorioh )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( srmp4 )		{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( srmp7 )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = -0xf;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( survarts )	{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( sxyreact )	{	hypreac2_init();	// different
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( twineag2 )	{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( stmblade )	{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf8;	}


static MACHINE_DRIVER_START( ssv )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", V60, CLOCK_12MHz)
	MDRV_CPU_VBLANK_INT(ssv_interrupt,1)	/* Vblank */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we use cpu_getvblank */

	MDRV_MACHINE_INIT(ssv)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(0x180, 0x100)
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
	MDRV_GFXDECODE(ssv_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH((0x8000/64)*256*2)

	MDRV_PALETTE_INIT(ssv)
	MDRV_VIDEO_UPDATE(ssv)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5506, es5506_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( drifto94 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_16MHz)
	MDRV_CPU_MEMORY(drifto94_readmem, drifto94_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hypreact )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(hypreact_readmem, hypreact_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(8, 0x148-1, 16, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hypreac2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(hypreac2_readmem, hypreac2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 8, 0xf8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( janjans1 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(janjans1_readmem, janjans1_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( keithlcy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(keithlcy_readmem, keithlcy_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( meosism )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(meosism_readmem, meosism_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mslider )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(mslider_readmem, mslider_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ryorioh )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(ryorioh_readmem, ryorioh_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( srmp4 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz)
	MDRV_CPU_MEMORY(srmp4_readmem, srmp4_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf4-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( srmp7 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(srmp7_readmem, srmp7_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( survarts )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_16MHz)
	MDRV_CPU_MEMORY(survarts_readmem, survarts_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf4-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sxyreact )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(sxyreact_readmem, sxyreact_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( twineag2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_12MHz_UNK)
	MDRV_CPU_MEMORY(twineag2_readmem, twineag2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( stmblade )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_REPLACE("main", V60, CLOCK_16MHz)
	MDRV_CPU_MEMORY(drifto94_readmem, drifto94_writemem)

	MDRV_NVRAM_HANDLER(ssv)
	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x158-1, 2, 0xf0-1)
MACHINE_DRIVER_END



/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

						Drift Out '94 - The hard order

----------------------
System SSV (STA-0001B)
----------------------
CPU  : NEC D70615GD-16-S (V60)
Sound: Ensoniq ES5506 (OTTOR2)
OSC  : 42.9545MHz(X2) 48.0000MHz(X3)

Custom chips:
ST-0004 (Video DAC?)
ST-0005 (Parallel I/O?)
ST-0006 (Video controller)
ST-0007 (System controller)

Program Work RAM  : 256Kbitx2 (expandable to 1Mx2)
Object Work RAM   : 1Mbitx2
Color Palette RAM : 256Kbitx3 (expandable to 1Mx3)

-------------------------
SSV Subboard (VISCO-001B)
-------------------------
ROMs:
visco-33.bin - Main programs (27c4000)
visco-37.bin /

vg003-19.u26 - Data? (mask, read as 27c160)

vg003-17.u22 - Samples (mask, read as 27c160)
vg003-18.u15 /

vg003-01.a0 - Graphics (mask, read as 27c160)
vg003-05.a1 |
vg003-09.a2 |
vg009-13.a3 |
vg009-02.b0 |
vg003-06.b1 |
vg003-10.b2 |
vg003-14.b3 |
vg003-03.c0 |
vg003-07.c1 |
vg003-11.c2 |
vg003-15.c3 |
vg003-04.d0 |
vg003-08.d1 |
vg003-12.d2 |
vg003-16.d3 /

GAL:
vg003-22.u29 (16V8)

Custom chip:
ST010 (maybe D78C10?)

Others:
Lithium battery + MB3790 + LH5168D-10L

***************************************************************************/

ROM_START( drifto94 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "vg003-19.u26", 0x000000, 0x200000, 0x238e5e2b )	// "SoundDriverV1.1a"
	ROM_LOAD16_BYTE( "visco-37.bin", 0x200000, 0x080000, 0x78fa3ccb )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "visco-33.bin", 0x200001, 0x080000, 0x88351146 )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "vg003-01.a0", 0x0000000, 0x200000, 0x2812aa1a )
	ROM_LOAD( "vg003-05.a1", 0x0200000, 0x200000, 0x1a1dd910 )
	ROM_LOAD( "vg003-09.a2", 0x0400000, 0x200000, 0x198f1c06 )
	ROM_LOAD( "vg003-13.a3", 0x0600000, 0x200000, 0xb45b2267 )

	ROM_LOAD( "vg003-02.b0", 0x0800000, 0x200000, 0xd7402027 )
	ROM_LOAD( "vg003-06.b1", 0x0a00000, 0x200000, 0x518c509f )
	ROM_LOAD( "vg003-10.b2", 0x0c00000, 0x200000, 0xc1ee9d8b )
	ROM_LOAD( "vg003-14.b3", 0x0e00000, 0x200000, 0x645b672b )

	ROM_LOAD( "vg003-03.c0", 0x1000000, 0x200000, 0x1ca7163d )
	ROM_LOAD( "vg003-07.c1", 0x1200000, 0x200000, 0x2ff113bb )
	ROM_LOAD( "vg003-11.c2", 0x1400000, 0x200000, 0xf924b105 )
	ROM_LOAD( "vg003-15.c3", 0x1600000, 0x200000, 0x83623b01 )

	ROM_LOAD( "vg003-04.d0", 0x1800000, 0x200000, 0x6be9bc62 )
	ROM_LOAD( "vg003-08.d1", 0x1a00000, 0x200000, 0xa7113cdb )
	ROM_LOAD( "vg003-12.d2", 0x1c00000, 0x200000, 0xac0fd855 )
	ROM_LOAD( "vg003-16.d3", 0x1e00000, 0x200000, 0x1a5fd312 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "vg003-17.u22", 0x000000, 0x200000, 0x6f9294ce )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "vg003-18.u15", 0x000000, 0x200000, 0x511b3e93 )
ROM_END


/***************************************************************************

					(Mahjong) Hyper Reaction (Japan)

(c)1995 Sammy, SSV system

P1-102A (ROM board)

***************************************************************************/

ROM_START( hypreact )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "s14-1-02.u2", 0x000000, 0x080000, 0xd90a383c )
	ROM_LOAD16_BYTE( "s14-1-01.u1", 0x000001, 0x080000, 0x80481401 )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s14-1-07.u7",  0x0000000, 0x200000, 0x6c429fd0 )
	ROM_LOAD( "s14-1-05.u13", 0x0200000, 0x200000, 0x2ff72f98 )
	ROM_LOAD( "s14-1-06.u10", 0x0400000, 0x200000, 0xf470ec42 )

	ROM_LOAD( "s14-1-10.u6",  0x0600000, 0x200000, 0xfdd706ba )
	ROM_LOAD( "s14-1-08.u12", 0x0800000, 0x200000, 0x5bb9bb0d )
	ROM_LOAD( "s14-1-09.u9",  0x0a00000, 0x200000, 0xd1dda65f )

	ROM_LOAD( "s14-1-13.u8",  0x0c00000, 0x200000, 0x971caf11 )
	ROM_LOAD( "s14-1-11.u14", 0x0e00000, 0x200000, 0x6d8e7bae )
	ROM_LOAD( "s14-1-12.u11", 0x1000000, 0x200000, 0x233a8e23 )

	ROM_FILL(                 0x1200000, 0x600000, 0          )

//	The chip seems to use REGION1 too, but produces no sound from there.

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s14-1-04.u4", 0x000000, 0x200000, 0xa5955336 )
	ROM_LOAD16_WORD_SWAP( "s14-1-03.u5", 0x200000, 0x200000, 0x283a6ec2 )
ROM_END


/***************************************************************************

					(Mahjong) Hyper Reaction 2 (Japan)

(c)1997 Sammy,SSV system

P1-112A (ROM board)

***************************************************************************/

ROM_START( hypreac2 )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "u2.bin",  0x000000, 0x080000, 0x05c93266 )
	ROM_LOAD16_BYTE( "u1.bin",  0x000001, 0x080000, 0x80cf9e59 )
	ROM_LOAD16_BYTE( "u47.bin", 0x100000, 0x080000, 0xa3e9bfee )
	ROM_LOAD16_BYTE( "u46.bin", 0x100001, 0x080000, 0x68c41235 )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s16-1-16.u6",  0x0000000, 0x400000, 0xb308ac34 )
	ROM_LOAD( "s16-1-15.u9",  0x0400000, 0x400000, 0x2c8e381e )
	ROM_LOAD( "s16-1-14.u12", 0x0800000, 0x200000, 0xafe9d187 )

	ROM_LOAD( "s16-1-10.u7",  0x0a00000, 0x400000, 0x86a10cbd )
	ROM_LOAD( "s16-1-09.u10", 0x0e00000, 0x400000, 0x6b8e4d92 )
	ROM_LOAD( "s16-1-08.u13", 0x1200000, 0x200000, 0xb355f45d )

	ROM_LOAD( "s16-1-13.u8",  0x1400000, 0x400000, 0x89869af2 )
	ROM_LOAD( "s16-1-12.u11", 0x1800000, 0x400000, 0x87d9c748 )
	ROM_LOAD( "s16-1-11.u14", 0x1c00000, 0x200000, 0x70b3c0a0 )

	ROM_FILL(                 0x1e00000,0x0a00000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s16-1-06.u41", 0x000000, 0x400000, 0x626e8a81 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s16-1-07.u42", 0x000000, 0x400000, 0x42bcb41b )
ROM_END


/***************************************************************************

							Jan Jan Simasyo (Japan)

(c)1996 Visco, SSV System

***************************************************************************/

ROM_START( janjans1 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "jj1-data.bin", 0x000000, 0x200000, 0x6734537e )
	ROM_LOAD16_BYTE( "jj1-prol.bin", 0x200000, 0x080000, 0x4231d928 )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "jj1-proh.bin", 0x200001, 0x080000, 0x651383c6 )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "jj1-a0.bin", 0x0000000, 0x400000, 0x39bbbc46 )
	ROM_LOAD( "jj1-a1.bin", 0x0400000, 0x400000, 0x26020133 )
	ROM_LOAD( "jj1-a2.bin", 0x0800000, 0x200000, 0xe993251e )

	ROM_LOAD( "jj1-b0.bin", 0x0a00000, 0x400000, 0x8ee66b0a )
	ROM_LOAD( "jj1-b1.bin", 0x0e00000, 0x400000, 0x048719b3 )
	ROM_LOAD( "jj1-b2.bin", 0x1200000, 0x200000, 0x6e95af3f )

	ROM_LOAD( "jj1-c0.bin", 0x1400000, 0x400000, 0x9df28afc )
	ROM_LOAD( "jj1-c1.bin", 0x1800000, 0x400000, 0xeb470ed3 )
	ROM_LOAD( "jj1-c2.bin", 0x1c00000, 0x200000, 0xaaf72c2d )

	ROM_LOAD( "jj1-d0.bin", 0x1e00000, 0x400000, 0x2b3bd591 )
	ROM_LOAD( "jj1-d1.bin", 0x2200000, 0x400000, 0xf24c0d36 )
	ROM_LOAD( "jj1-d2.bin", 0x2600000, 0x200000, 0x481b3be8 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "jj1-snd0.bin", 0x000000, 0x200000, 0x4f7d620a )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "jj1-snd1.bin", 0x000000, 0x200000, 0x9b3a7ae5 )
ROM_END


/***************************************************************************

				Dramatic Adventure Quiz Keith & Lucy (Japan)

(c)1993 Visco, SSV system

STS-0001 (ROM board)

***************************************************************************/

ROM_START( keithlcy )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "vg002-07.u28", 0x000000, 0x100000, 0x57f80ff5 )	// "SETA SoundDriver"
	ROM_LOAD16_BYTE( "kl-p0l.u26",   0x100000, 0x080000, 0xd7b177fb )
	ROM_LOAD16_BYTE( "kl-p0h.u27",   0x100001, 0x080000, 0x9de7add4 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "vg002-01.u13", 0x000000, 0x200000, 0xb44d85b2 )
	ROM_LOAD( "vg002-02.u16", 0x200000, 0x200000, 0xaa05fd14 )
	ROM_LOAD( "vg002-03.u21", 0x400000, 0x200000, 0x299a8a7d )
	ROM_LOAD( "vg002-04.u34", 0x600000, 0x200000, 0xd3633f9b )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "vg002-05.u29", 0x000000, 0x200000, 0x66aecd79 )
	ROM_LOAD16_WORD_SWAP( "vg002-06.u33", 0x200000, 0x200000, 0x75d8c8ea )
ROM_END


/***************************************************************************

						Meosis Magic (Japan, BET?)

(c)1996 Sammy, SSV System

P1-105A

Custom:		DX-102 (I/O)
Others:		M62X42B (RTC?)
			64k SRAM (Back up)
			Ni-Cd Battery

***************************************************************************/

ROM_START( meosism )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "s15-2-2.u47", 0x000000, 0x080000, 0x2ab0373f )
	ROM_LOAD16_BYTE( "s15-2-1.u46", 0x000001, 0x080000, 0xa4bce148 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s15-1-7.u7", 0x000000, 0x200000, 0xec5023cb )
	ROM_LOAD( "s15-1-8.u6", 0x200000, 0x200000, 0xf04b0836 )
	ROM_LOAD( "s15-1-5.u9", 0x400000, 0x200000, 0xc0414b97 )
	ROM_LOAD( "s15-1-6.u8", 0x600000, 0x200000, 0xd721aeb6 )

//	The chip seems to use REGION1 too, but produces no sound from there.

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s15-1-4.u45", 0x000000, 0x200000, 0x0c6738a7 )
	ROM_LOAD16_WORD_SWAP( "s15-1-3.u43", 0x200000, 0x200000, 0xd7e83178 )
ROM_END


/***************************************************************************

							Monster Slider (Japan)

(c)1997 Visco/PATT, System SSV

ms-pl.bin - V60 main program (27c4000, low)
ms-ph.bin - V60 main program (27c4000, high)

ms-snd0.bin \
            |- sound data (read as 27c160)
ms-snd1.bin /

ms-a0.bin \
ms-b0.bin |- Graphics (read as 27c160)
ms-c0.bin /

ms-a1.bin \
ms-b1.bin |- Graphics (27c4100)
ms-c1.bin /

vg001-14 \
         |- (GAL16V8. not dumped)
vg001-15 /

Other parts:	uPD71051
				OSC 8.0000MHz

***************************************************************************/

ROM_START( mslider )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "ms-pl.bin", 0x000000, 0x080000, 0x70b2a05d )
	ROM_LOAD16_BYTE( "ms-ph.bin", 0x000001, 0x080000, 0x34a64e9f )

	ROM_REGION( 0xa00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ms-a0.bin", 0x000000, 0x200000, 0x7ed38ccc )
	ROM_LOAD( "ms-a1.bin", 0x200000, 0x080000, 0x83f5995f )

	ROM_LOAD( "ms-b0.bin", 0x280000, 0x200000, 0xfaa076e1 )
	ROM_LOAD( "ms-b1.bin", 0x480000, 0x080000, 0xef9748db )

	ROM_LOAD( "ms-c0.bin", 0x500000, 0x200000, 0xf9d3e052 )
	ROM_LOAD( "ms-c1.bin", 0x700000, 0x080000, 0x7f910c5a )

	ROM_FILL(              0x780000, 0x280000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ms-snd0.bin", 0x000000, 0x200000, 0xcda6e3a5 )
	ROM_LOAD16_WORD_SWAP( "ms-snd1.bin", 0x200000, 0x200000, 0x8f484b35 )
ROM_END


/***************************************************************************

					Gourmet Battle Quiz Ryorioh CooKing (Japan)

(c)1998 Visco, SSV System

***************************************************************************/

ROM_START( ryorioh )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD( "ryorioh.dat",      0x000000, 0x200000, 0xd1335a6a )
	ROM_LOAD16_BYTE( "ryorioh.l", 0x200000, 0x080000, 0x9ad60e7d )
	ROM_RELOAD(                   0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "ryorioh.h", 0x200001, 0x080000, 0x0655fcff )
	ROM_RELOAD(                   0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ryorioh.a0", 0x0000000, 0x400000, 0xf76ee003 )
	ROM_LOAD( "ryorioh.a1", 0x0400000, 0x400000, 0xca44d66d )

	ROM_LOAD( "ryorioh.b0", 0x0800000, 0x400000, 0xdaa134f4 )
	ROM_LOAD( "ryorioh.b1", 0x0c00000, 0x400000, 0x7611697c )

	ROM_LOAD( "ryorioh.c0", 0x1000000, 0x400000, 0x20eb49cf )
	ROM_LOAD( "ryorioh.c1", 0x1400000, 0x400000, 0x1370c75e )

	ROM_LOAD( "ryorioh.d0", 0x1800000, 0x400000, 0xffa14ef1 )
	ROM_LOAD( "ryorioh.d1", 0x1c00000, 0x400000, 0xae6055e8 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "ryorioh.snd", 0x000000, 0x200000, 0x7bd38b76 )
ROM_END


/***************************************************************************

							Super Real Mahjong PIV

(c)SETA 1993, System SSV

CPU        : V60 (12MHz)
Sound      : Ensoniq OTTO
Work RAM   : 256Kbit (expandable to 1Mbitx2. SRMP7 requires this)
Object RAM : 1Mbitx2
Palette RAM: 256Kbitx3 (expandable to 1Mbitx3)

sx001-01.a0 \
sx001-02.b0 |
sx001-03.c0 |
sx001-04.a1 |
sx001-05.b1 |- Graphics (16M Mask)
sx001-06.c1 |
sx001-07.a2 |
sx001-08.b2 |
sx001-09.c2 /

sx001-10.sd0 - Sound - 16M Mask

sx001-11.prl - Main program (low)  - 27c040
sx001-12.prh - Main program (high) - 27c040

Custom chips
ST-0004 (Video DAC)
ST-0005 (Parallel I/O)
ST-0006 (Video controller - 32768 palettes from 24bit color)
ST-0007 (System controller)

***************************************************************************/

ROM_START( srmp4 )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "sx001-11.prl", 0x000000, 0x080000, 0xdede3e64 )
	ROM_LOAD16_BYTE( "sx001-12.prh", 0x000001, 0x080000, 0x739c53c3 )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx001-01.a0", 0x0000000, 0x200000, 0x94ee9203 )
	ROM_LOAD( "sx001-04.a1", 0x0200000, 0x200000, 0x38c9c49a )
	ROM_LOAD( "sx001-07.a2", 0x0400000, 0x200000, 0xee66021e )

	ROM_LOAD( "sx001-02.b0", 0x0600000, 0x200000, 0xadffb598 )
	ROM_LOAD( "sx001-05.b1", 0x0800000, 0x200000, 0x4c400a38 )
	ROM_LOAD( "sx001-08.b2", 0x0a00000, 0x200000, 0x36efd52c )

	ROM_LOAD( "sx001-03.c0", 0x0c00000, 0x200000, 0x4336b037 )
	ROM_LOAD( "sx001-06.c1", 0x0e00000, 0x200000, 0x6fe7229e )
	ROM_LOAD( "sx001-09.c2", 0x1000000, 0x200000, 0x91dd8218 )

	ROM_FILL(                0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "sx001-10.sd0", 0x000000, 0x200000, 0x45409ef1 )
	ROM_RELOAD(                           0x200000, 0x200000             )
ROM_END


/***************************************************************************

							Super Real Mahjong P7 (Japan)

(c)1997 Seta, SSV system

***************************************************************************/

ROM_START( srmp7 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sx015-10.dat", 0x000000, 0x200000, 0xfad3ac6a )
	ROM_LOAD16_BYTE( "sx015-07.pr0", 0x200000, 0x080000, 0x08d7f841 )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "sx015-08.pr1", 0x200001, 0x080000, 0x90307825 )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx015-26.a0", 0x0000000, 0x400000, 0xa997be9d )
	ROM_LOAD( "sx015-25.a1", 0x0400000, 0x400000, 0x29ac4211 )
	ROM_LOAD( "sx015-24.a2", 0x0800000, 0x400000, 0xb8fea3da )
	ROM_LOAD( "sx015-23.a3", 0x0c00000, 0x400000, 0x9ec0b81e )

	ROM_LOAD( "sx015-22.b0", 0x1000000, 0x400000, 0x62c3df07 )
	ROM_LOAD( "sx015-21.b1", 0x1400000, 0x400000, 0x55b8a431 )
	ROM_LOAD( "sx015-20.b2", 0x1800000, 0x400000, 0xe84a64d7 )
	ROM_LOAD( "sx015-19.b3", 0x1c00000, 0x400000, 0x994b5063 )

	ROM_LOAD( "sx015-18.c0", 0x2000000, 0x400000, 0x72d43fd4 )
	ROM_LOAD( "sx015-17.c1", 0x2400000, 0x400000, 0xfdfd82f1 )
	ROM_LOAD( "sx015-16.c2", 0x2800000, 0x400000, 0x86aa314b )
	ROM_LOAD( "sx015-15.c3", 0x2c00000, 0x400000, 0x11f50e16 )

	ROM_LOAD( "sx015-14.d0", 0x3000000, 0x400000, 0x186f83fa )
	ROM_LOAD( "sx015-13.d1", 0x3400000, 0x400000, 0xea6e5329 )
	ROM_LOAD( "sx015-12.d2", 0x3800000, 0x400000, 0x80336523 )
	ROM_LOAD( "sx015-11.d3", 0x3c00000, 0x400000, 0x134c8e28 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-06.s0", 0x000000, 0x200000, 0x0d5a206c )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-05.s1", 0x000000, 0x200000, 0xbb8cebe2 )

	ROM_REGION16_BE( 0x800000, REGION_SOUND3, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-04.s2", 0x000000, 0x200000, 0xf6e933df )
	ROM_LOAD16_BYTE( "sx015-02.s4", 0x400000, 0x200000, 0x6567bc3e )

	ROM_REGION16_BE( 0x800000, REGION_SOUND4, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-03.s3", 0x000000, 0x200000, 0x5b51ab21 )
	ROM_LOAD16_BYTE( "sx015-01.s5", 0x400000, 0x200000, 0x481b00ed )
ROM_END


/***************************************************************************

         Survival Arts

 Manufacturer: Sammy USA
 System Type: System SSV

 ----------------------
 System SSV (STA-0001)
 ----------------------
 CPU  : NEC D70615GD-16 (V60)
 Sound: Ensoniq ES5506 (OTTOR2)
 OSC  : 42.9545MHz(X2) 48.0000MHz(X3)

 Custom chips:
 ST-0004 (Video DAC)
 ST-0005 (Parallel I/O)
 ST-0006 (Video controller)
 ST-0007 (System controller)

 Program Work RAM  : 256Kbitx2 (expandable to 1Mx2)
 Object Work RAM   : 1Mbitx2
 Color Palette RAM : 256Kbitx3 (expandable to 1Mx3)

 -------------------------
 SSV Subboard (SAM-5127)
 -------------------------
 ROMs:
 USA-PR-H.u3 - V60 Program (27C4001)
 USA-PR-L.u4 /

 si001-10.s0 - Samples (16M-Mask)
 si001-12.s2 /

 si001-11.s1 - Samples (8M-Mask)
 si001-13.s3 /

 si001-01.a0 - Graphics (16M-Mask)
 si001-04.a1 |
 si001-05.a2 |
 si001-02.b0 |
 si001-05.b1 |
 si001-07.b2 |
 si001-03.c0 |
 si001-06.c1 |
 si001-09.c2 /

 Empty Sockets:
 DATA --- 16M-Mask
 A3     |
 B3     |
 C3     |
 D0-D3 /

 GAL:
 si003-14.u5 (16V8B)

 MISC:
 Pin Header for P3 (used)
 Pin Header for P4 (unused)

***************************************************************************/

ROM_START( survarts )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "usa-pr-l.u4", 0x000000, 0x080000, 0xfa328673 )
	ROM_LOAD16_BYTE( "usa-pr-h.u3", 0x000001, 0x080000, 0x6bee2635 )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "si001-01.a0", 0x0000000, 0x200000, 0x8b38fbab )
	ROM_LOAD( "si001-04.a1", 0x0200000, 0x200000, 0x34248b54 )
	ROM_LOAD( "si001-07.a2", 0x0400000, 0x200000, 0x497d6151 )

	ROM_LOAD( "si001-02.b0", 0x0600000, 0x200000, 0xcb4a2dbd )
	ROM_LOAD( "si001-05.b1", 0x0800000, 0x200000, 0x8f092381 )
	ROM_LOAD( "si001-08.b2", 0x0a00000, 0x200000, 0x182b88c4 )

	ROM_LOAD( "si001-03.c0", 0x0c00000, 0x200000, 0x92fdf652 )
	ROM_LOAD( "si001-06.c1", 0x0e00000, 0x200000, 0x9a62f532 )
	ROM_LOAD( "si001-09.c2", 0x1000000, 0x200000, 0x0955e393 )

	ROM_FILL(                0x1200000, 0x600000, 0          )

//	The chip seems to use REGION1 too, but produces no sound from there.

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si001-10.s0", 0x000000, 0x200000, 0xb0c3425d )	// FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD16_WORD_SWAP( "si001-11.s1", 0x100000, 0x200000, 0x6bf621ca )	// FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD16_WORD_SWAP( "si001-12.s2", 0x200000, 0x200000, 0xd51a92b8 )	// FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD16_WORD_SWAP( "si001-13.s3", 0x300000, 0x100000, 0x8f1a34bb )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(                        0x300000, 0x100000             )
ROM_END


/***************************************************************************

						Pachinko Sexy Reaction (Japan)

(c)1998 Sammy, SSV system

P1-112C (ROM board)

Chips:	DX-102 x2
		uPD7001C (ADC?)
		64k NVRAM

***************************************************************************/

ROM_START( sxyreact )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "ac414e00.u2",  0x000000, 0x080000, 0xd5dd7593 )
	ROM_LOAD16_BYTE( "ac413e00.u1",  0x000001, 0x080000, 0xf46aee4a )
	ROM_LOAD16_BYTE( "ac416e00.u47", 0x100000, 0x080000, 0xe0f7bba9 )
	ROM_LOAD16_BYTE( "ac415e00.u46", 0x100001, 0x080000, 0x92de1b5f )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ac1401m0.u6",  0x0000000, 0x400000, 0x0b7b693c )
	ROM_LOAD( "ac1402m0.u9",  0x0400000, 0x400000, 0x9d593303 )
	ROM_LOAD( "ac1403m0.u12", 0x0800000, 0x200000, 0xaf433eca )

	ROM_LOAD( "ac1404m0.u7",  0x0a00000, 0x400000, 0xcdda2ccb )
	ROM_LOAD( "ac1405m0.u10", 0x0e00000, 0x400000, 0xe5e7a5df )
	ROM_LOAD( "ac1406m0.u13", 0x1200000, 0x200000, 0xc7053409 )

	ROM_LOAD( "ac1407m0.u8",  0x1400000, 0x400000, 0x28c83d5e )
	ROM_LOAD( "ac1408m0.u11", 0x1800000, 0x400000, 0xc45bab47 )
	ROM_LOAD( "ac1409m0.u14", 0x1c00000, 0x200000, 0xbe1c66c2 )

	ROM_FILL(                 0x1e00000, 0xa00000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1410m0.u41", 0x000000, 0x400000, 0x2a880afc )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1411m0.u42", 0x200000, 0x200000, 0x2ba4ca43 )
	ROM_CONTINUE( 0x000000, 0x200000 )	// this will go in region 3

	// a few sparse samples are played from here
	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND2, 0x000000,    0x200000, 0x200000 )
ROM_END


/***************************************************************************

								Twin Eagle II

***************************************************************************/

ROM_START( twineag2 )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sx002_12", 0x000000, 0x200000, 0x846044dc )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx002_01", 0x0000000, 0x200000, 0x6d6896b5 )
	ROM_LOAD( "sx002_02", 0x0200000, 0x200000, 0x3f47e97a )
	ROM_LOAD( "sx002_03", 0x0400000, 0x200000, 0x544f18bf )

	ROM_LOAD( "sx002_04", 0x0600000, 0x200000, 0x58c270e2 )
	ROM_LOAD( "sx002_05", 0x0800000, 0x200000, 0x3c310229 )
	ROM_LOAD( "sx002_06", 0x0a00000, 0x200000, 0x46d5b1f3 )

	ROM_LOAD( "sx002_07", 0x0c00000, 0x200000, 0xc30fa397 )
	ROM_LOAD( "sx002_08", 0x0e00000, 0x200000, 0x64edcefa )
	ROM_LOAD( "sx002_09", 0x1000000, 0x200000, 0x51527c56 )

	ROM_FILL(             0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "sx002_10", 0x000000, 0x200000, 0xb0669dfa )
	ROM_LOAD16_WORD_SWAP( "sx002_11", 0x200000, 0x200000, 0xb8dd621a )
ROM_END


/***************************************************************************

								Storm Blade
CPU  : NEC D70615GD-16-S (V60)
Sound: Ensoniq ES5506 (OTTOR2)


Rom board  001B
SSV mother board

U37, U33 = 27c040
U22, U41, U35, U25, U21, U11, U7  = 16 MEG MASK ROMS
U32, U18, U4 = 4 MEG MASK ROMS
U26 = 8 MEG MASK ROM

There is a battery on the rom board @ BT1 (battery # CR2032 - 3 volts)

***************************************************************************/


ROM_START( stmblade )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sb-pd0.u26",  0x000000, 0x100000, 0x91c4fbf7 )
	ROM_LOAD16_BYTE( "s-blade.u37", 0x200000, 0x080000, 0xa6a42cc7 )
	ROM_RELOAD(                     0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "s-blade.u33", 0x200001, 0x080000, 0x16104ca6 )
	ROM_RELOAD(                     0x300001, 0x080000             )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sb-a0.u41", 0x0000000, 0x200000, 0x2a327b51 )
	ROM_LOAD( "sb-a1.u35", 0x0200000, 0x200000, 0x246f6f28 )
	ROM_LOAD( "sb-a2.u32", 0x0400000, 0x080000, 0x2049acf3 )
	ROM_LOAD( "sb-b0.u25", 0x0600000, 0x200000, 0xb3aa3e68 )
	ROM_LOAD( "sb-b1.u21", 0x0800000, 0x200000, 0xe95b38e7 )
	ROM_LOAD( "sb-b2.u18", 0x0a00000, 0x080000, 0xd080e620 )
	ROM_LOAD( "sb-c0.u11", 0x0c00000, 0x200000, 0x825dd8f1 )
	ROM_LOAD( "sb-c1.u7",  0x0e00000, 0x200000, 0x744afcd7 )
	ROM_LOAD( "sb-c2.u4",  0x1000000, 0x080000, 0xfd1d2a92 )
	ROM_FILL(              0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sb-snd0.u22", 0x000000, 0x200000, 0x4efd605b )
ROM_END

/***************************************************************************

						Change Air Blade (Japan)

Change Air Blade
Sammy, 1999

ROM board for use with System SSV Main Board
PCB No: P1-112C

Fairly sparsely populated board containing not much except....

RAM   : 6262 (x1)
OTHER : 3.6V Ni-Cd Battery
PALs  : (x1, labelled AC412G00)

ROMs  : (Filename  = ROM Label)
        (Extension = PCB Location)
------------------------------
AC1801M01.U6    32M Mask
AC1802M01.U9    32M Mask

AC1805M01.U8    32M Mask
AC1806M01.U11   32M Mask

AC1803M01.U7    32M Mask
AC1804M01.U10   32M Mask

AC1807M01.U41   32M Mask
AC1810E01.U32   27C160

Developers:
           More Info Reqd? Redump needed? Email me....
           theguru@emuunlim.com

***************************************************************************/

ROM_START( cairblad )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "ac1810e0.u32",  0x000000, 0x200000, 0x13a0b4c2  ) // AC1810E01.U32    27C160

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ac1801m0.u6",  0x0000000, 0x400000, 0x1b2b6943  ) // AC1801M01.U6    32M Mask
	ROM_LOAD( "ac1802m0.u9",  0x0400000, 0x400000, 0xe053b087  ) // AC1802M01.U9    32M Mask

	ROM_LOAD( "ac1803m0.u7",  0x0800000, 0x400000, 0x45484866  ) // AC1803M01.U7    32M Mask
	ROM_LOAD( "ac1804m0.u10", 0x0c00000, 0x400000, 0x5e0b2285  ) // AC1804M01.U10   32M Mask

	ROM_LOAD( "ac1805m0.u8",  0x1000000, 0x400000, 0x19771f43  ) // AC1805M01.U8    32M Mask
	ROM_LOAD( "ac1806m0.u11", 0x1400000, 0x400000, 0x816b97dc  ) // AC1806M01.U11   32M Mask

	ROM_FILL(                 0x1800000, 0x800000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1410m0.u41", 0x000000, 0x400000, 0xecf1f255  ) // AC1807M01.U41   32M Mask
ROM_END

/***************************************************************************

						Eagle Shot Golf
Eagle Shot Golf
Sammy, 199x

Lower PCB
PCB Number: GOLF ROM PCB
RAM       : HM514400 (x8)
PALs      : GAL16V8 (x2) labelled SI3-11 & SI3-12
OTHER     : NEC D4701AC
            Controls probably trackball, has 6 pin connector hooked up to a
            4584 Logic IC. Joystick appears to be used also for selecting
            stance, club and direction.

ROMs      : U18 & U20 are used for main program.
            All rest are 16M Mask
            U23 & U24 are sound related, all others for GFX.

***************************************************************************/

ROM_START( eaglshot )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "si003-10.u20",  0x000001, 0x080000, 0xc8872e48 )
	ROM_LOAD16_BYTE( "si003-09.u18",  0x000000, 0x080000, 0x219c71ce )

	ROM_REGION( 0x0c00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "si003-04.u10", 0x0000000, 0x200000, 0x4c65d1a1 )
	ROM_LOAD( "si003-02.u12", 0x0200000, 0x200000, 0x92b4d50d )
	ROM_LOAD( "si003-05.u30", 0x0400000, 0x200000, 0xdaf52d56 )
	ROM_LOAD( "si003-03.u11", 0x0600000, 0x200000, 0x6ede4012 )
	ROM_LOAD( "si003-01.u13", 0x0800000, 0x200000, 0xd7df0d52 )
	ROM_LOAD( "si003-06.u31", 0x0a00000, 0x200000, 0x449f9ae5 )

	ROM_REGION16_BE( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-07.u23", 0x000000, 0x200000, 0x81679fd6 )

	ROM_REGION16_BE( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-08.u24", 0x000000, 0x200000, 0xd0122ba2 )
ROM_END

ROM_START( eaglshta )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "si003-10.prh",  0x000001, 0x080000, 0x2060c304 )
	ROM_LOAD16_BYTE( "si003-09.prl",  0x000000, 0x080000, 0x36989004 )

	ROM_REGION( 0x0c00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "si003-04.u10", 0x0000000, 0x200000, 0x4c65d1a1 )
	ROM_LOAD( "si003-02.u12", 0x0200000, 0x200000, 0x92b4d50d )
	ROM_LOAD( "si003-05.u30", 0x0400000, 0x200000, 0xdaf52d56 )
	ROM_LOAD( "si003-03.u11", 0x0600000, 0x200000, 0x6ede4012 )
	ROM_LOAD( "si003-01.u13", 0x0800000, 0x200000, 0xd7df0d52 )
	ROM_LOAD( "si003-06.u31", 0x0a00000, 0x200000, 0x449f9ae5 )

	ROM_REGION16_BE( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-07.u23", 0x000000, 0x200000, 0x81679fd6 )

	ROM_REGION16_BE( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-08.u24", 0x000000, 0x200000, 0xd0122ba2 )
ROM_END

/***************************************************************************


								Game Drivers


***************************************************************************/

//     year   rom       clone     machine   inputs    init      monitor manufacturer          title                                           flags

GAMEX( 1993,  srmp4,    0,        srmp4,    srmp4,    srmp4,    ROT0,   "Seta",               "Super Real Mahjong PIV (Japan)",               GAME_NO_COCKTAIL )
GAMEX( 1997,  srmp7,    0,        srmp7,    srmp7,    srmp7,    ROT0,   "Seta",               "Super Real Mahjong P7 (Japan)",                GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )

GAMEX( 1993,  survarts, 0,        survarts, survarts, survarts, ROT0,   "Sammy (American)",   "Survival Arts (USA)",                          GAME_NO_COCKTAIL )
GAMEX( 1995,  hypreact, 0,        hypreact, hypreact, hypreact, ROT0,   "Sammy",              "Mahjong Hyper Reaction (Japan)",               GAME_NO_COCKTAIL | GAME_NOT_WORKING )
GAMEX( 1996?, meosism,  0,        meosism,  meosism,  meosism,  ROT0,   "Sammy",              "Meosis Magic (Japan)",                         GAME_NO_COCKTAIL )
GAMEX( 1997,  hypreac2, 0,        hypreac2, hypreac2, hypreac2, ROT0,   "Sammy",              "Mahjong Hyper Reaction 2 (Japan)",             GAME_NO_COCKTAIL )
GAMEX( 1998,  sxyreact, 0,        sxyreact, sxyreact, sxyreact, ROT0,   "Sammy",              "Pachinko Sexy Reaction (Japan)",               GAME_NO_COCKTAIL )
GAMEX( 1999,  cairblad, 0,        sxyreact, cairblad, sxyreact, ROT270, "Sammy",              "Change Air Blade (Japan)",                     GAME_NO_COCKTAIL )

GAMEX( 1993,  keithlcy, 0,        keithlcy, keithlcy, keithlcy, ROT0,   "Visco",              "Dramatic Adventure Quiz Keith & Lucy (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1994,  drifto94, 0,        drifto94, drifto94, drifto94, ROT0,   "Visco",              "Drift Out '94 - The Hard Order (Japan)",       GAME_NO_COCKTAIL )
GAMEX( 1996,  janjans1, 0,        janjans1, janjans1, janjans1, ROT0,   "Visco",              "Lovely Pop Mahjong Jan Jan Shimasyo (Japan)",  GAME_NO_COCKTAIL )
GAMEX( 1996,  stmblade, 0,        stmblade, stmblade, stmblade, ROT270, "Visco",              "Storm Blade (US)",                             GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
GAMEX( 1998,  ryorioh,  0,        ryorioh,  ryorioh,  ryorioh,  ROT0,   "Visco",              "Gourmet Battle Quiz Ryorioh CooKing (Japan)",  GAME_NO_COCKTAIL )

// Games not working properly:

GAMEX( 1997,  mslider,  0,        mslider,  mslider,  mslider,  ROT0,   "Visco / Datt Japan", "Monster Slider (Japan)",                       GAME_NO_COCKTAIL ) // game logic?

// Games not working at all:

// Breaks immediately (on the first interrupt the core should use ISP!)
GAMEX( 1994,  twineag2, 0,        twineag2, twineag2, twineag2, ROT270, "Seta",               "Twin Eagle II - The Rescue Mission",           GAME_NO_COCKTAIL | GAME_NOT_WORKING )
GAMEX( 1994,  eaglshot, 0,        survarts, survarts, survarts, ROT0,   "Sammy",   			  "Eagle Shot Golf",                              GAME_NO_COCKTAIL | GAME_NOT_WORKING)
GAMEX( 1994,  eaglshta, eaglshot, survarts, survarts, survarts, ROT0,   "Sammy",   			  "Eagle Shot Golf (alt)",                        GAME_NO_COCKTAIL | GAME_NOT_WORKING)
