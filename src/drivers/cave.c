/***************************************************************************

							  -= Cave Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  MC68000

Sound CPU    :  Z80 [Optional]

Sound Chips  :	YMZ280B or
                OKIM6295 x (1|2) + YM2203 / YM2151 [Optional]

Other        :  93C46 EEPROM


-----------------------------------------------------------------------------------
Year + Game			License		PCB			Tilemaps		Sprites			Other
-----------------------------------------------------------------------------------
94	Mazinger Z		Banpresto	?			038 9335EX706	013 9341E7009	Z80
94	PowerInstinct 2 Atlus		ATG02?		038 9429WX709?	?				Z80
95	Metamoqester	Banpresto	BP947A		038 9437WX711	013 9346E7002	Z80
95	Sailor Moon		Banpresto	BP945A		038 9437WX711	013 9346E7002	Z80
95	Donpachi		Atlus		AT-C01DP-2	038 9429WX727	013 8647-01		NMK 112
96	Air Gallet		Banpresto	BP962A		038 9437WX711	013 9346E7002	Z80
96	Hotdog Storm	Marble		?			?								Z80
97	Dodonpachi		Atlus		ATC03D2 	?
98	Dangun Feveron	Nihon Sys.	CV01    	?
98	ESP Ra.De.		Atlus		ATC04		?
98	Uo Poko			Jaleco		CV02		?
99	Guwange			Atlus		ATC05		?
-----------------------------------------------------------------------------------

To Do:

- Sprite lag in some games (e.g. metmqstr). The sprites chip probably
  generates interrupts (unknown_irq)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "cpu/z80/z80.h"
#include "cave.h"

/***************************************************************************


						Interrupt Handling Routines


***************************************************************************/

static UINT8 vblank_irq;
static UINT8 sound_irq;
static UINT8 unknown_irq;

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	if (vblank_irq || sound_irq || unknown_irq)
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 1, CLEAR_LINE);
}

/* Called once/frame to generate the VBLANK interrupt */
static INTERRUPT_GEN( cave_interrupt )
{
	vblank_irq = 1;
	update_irq_state();
}

static INTERRUPT_GEN( mazinger_interrupt )
{
	unknown_irq = 1;
	cave_interrupt();
}

static INTERRUPT_GEN( metmqstr_interrupt )
{
	switch( cpu_getiloops() )
	{
		case 0:		// VBlank
			vblank_irq = 1;
			update_irq_state();
			break;
		default:
		case 1:		// Sprites?
			unknown_irq = 1;
			update_irq_state();
			break;
	}
}

/* Called by the YMZ280B to set the IRQ state */
static void sound_irq_gen(int state)
{
	sound_irq = (state != 0);
	update_irq_state();
}


/*	Level 1 irq routines:

	Game		|first read	| bit==0->routine +	|
				|offset:	| read this offset	|

	ddonpach	4,0			0 -> vblank + 4		1 -> rte	2 -> like 0		read sound
	dfeveron	0			0 -> vblank + 4		1 -> + 6	-				read sound
	uopoko		0			0 -> vblank + 4		1 -> + 6	-				read sound
	esprade		0			0 -> vblank + 4		1 -> rte	2 must be 0		read sound
	guwange		0			0 -> vblank + 6,4	1 -> + 6,4	2 must be 0		read sound
	mazinger	0			0 -> vblank + 4		rest -> scroll + 6
*/


/* Reads the cause of the interrupt and clears the state */

static READ16_HANDLER( cave_irq_cause_r )
{
	int result = 0x0003;

	if (vblank_irq)		result ^= 0x01;
	if (unknown_irq)	result ^= 0x02;

	if (offset == 4/2)	vblank_irq = 0;
	if (offset == 6/2)	unknown_irq = 0;

	update_irq_state();

	return result;
}

/* Is ddonpach different? This is probably wrong but it works */

static READ16_HANDLER( ddonpach_irq_cause_r )
{
	int result = 0x0007;

	if (vblank_irq)		result ^= 0x01;
//	if (unknown_irq)	result ^= 0x02;

	if (offset == 0/2)	vblank_irq = 0;
//	if (offset == 4/2)	unknown_irq = 0;

	update_irq_state();

	return result;
}


/***************************************************************************


							Sound Handling Routines


***************************************************************************/

/*	We need a FIFO buffer for sailormn, where the inter-CPUs
	communication is *really* tight */
struct
{
	int len;
	data8_t data[32];
}	soundbuf;

//static data8_t sound_flag1, sound_flag2;

static READ_HANDLER( soundflags_r )
{
	// bit 2 is low: can read command (lo)
	// bit 3 is low: can read command (hi)
//	return	(sound_flag1 ? 0 : 4) |
//			(sound_flag2 ? 0 : 8) ;
return 0;
}

static READ16_HANDLER( soundflags_ack_r )
{
	// bit 0 is low: can write command
	// bit 1 is low: can read answer
//	return	((sound_flag1 | sound_flag2) ? 1 : 0) |
//			((soundbuf.len>0        ) ? 0 : 2) ;

return		((soundbuf.len>0        ) ? 0 : 2) ;
}

/* Main CPU: write a 16 bit sound latch and generate a NMI on the sound CPU */
static WRITE16_HANDLER( sound_cmd_w )
{
//	sound_flag1 = 1;
//	sound_flag2 = 1;
	soundlatch_word_w(offset,data,mem_mask);
	cpu_set_nmi_line(1, PULSE_LINE);
	cpu_spinuntil_time(TIME_IN_USEC(50));	// Allow the other cpu to reply
}

/* Sound CPU: read the low 8 bits of the 16 bit sound latch */
static READ_HANDLER( soundlatch_lo_r )
{
//	sound_flag1 = 0;
	return soundlatch_word_r(offset,0) & 0xff;
}

/* Sound CPU: read the high 8 bits of the 16 bit sound latch */
static READ_HANDLER( soundlatch_hi_r )
{
//	sound_flag2 = 0;
	return soundlatch_word_r(offset,0) >> 8;
}

/* Main CPU: read the latch written by the sound CPU (acknowledge) */
static READ16_HANDLER( soundlatch_ack_r )
{
	if (soundbuf.len>0)
	{
		data8_t data = soundbuf.data[0];
		memmove(soundbuf.data,soundbuf.data+1,(32-1)*sizeof(soundbuf.data[0]));
		soundbuf.len--;
		return data;
	}
	else
	{	logerror("CPU #1 - PC %04X: Sound Buffer 2 Underflow Error\n",activecpu_get_pc());
		return 0xff;	}
}


/* Sound CPU: write latch for the main CPU (acknowledge) */
static WRITE_HANDLER( soundlatch_ack_w )
{
	soundbuf.data[soundbuf.len] = data;
	if (soundbuf.len<32)
		soundbuf.len++;
	else
		logerror("CPU #1 - PC %04X: Sound Buffer 2 Overflow Error\n",activecpu_get_pc());
}



/* Handles writes to the YMZ280B */
static WRITE16_HANDLER( cave_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

/* Handles reads from the YMZ280B */
static READ16_HANDLER( cave_sound_r )
{
	return YMZ280B_status_0_r(offset);
}


/***************************************************************************


									EEPROM


***************************************************************************/

static data8_t cave_default_eeprom_type1[16] =	{0x00,0x0C,0x11,0x0D,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x11,0x11,0xFF,0xFF,0xFF,0xFF};  /* DFeveron, Guwange */
static data8_t cave_default_eeprom_type2[16] =	{0x00,0x0C,0xFF,0xFB,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  /* Esprade, DonPachi, DDonPachi */
static data8_t cave_default_eeprom_type3[16] =	{0x00,0x03,0x08,0x00,0xFF,0xFF,0xFF,0xFF,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF};  /* UoPoko */
static data8_t cave_default_eeprom_type4[16] =	{0xF3,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  /* Hotdog Storm */
static data8_t cave_default_eeprom_type5[16] =	{0xED,0xFF,0x00,0x00,0x12,0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};  /* Mazinger Z (6th byte is country code) */
static data8_t cave_default_eeprom_type6[18] =	{0xa5,0x00,0xa5,0x00,0xa5,0x00,0xa5,0x00,0xa5,0x01,0xa5,0x01,0xa5,0x04,0xa5,0x01,0xa5,0x02};	/* Sailor Moon (last byte is country code) */
// Air Gallet. Byte 1f is the country code (0==JAPAN,U.S.A,EUROPE,HONGKONG,TAIWAN,KOREA)
static data8_t cave_default_eeprom_type7[48] =	{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
												 0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
												 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff};

static data8_t *cave_default_eeprom;
static int cave_default_eeprom_length;

READ16_HANDLER( cave_input1_r )
{
	return readinputport(1) | ((EEPROM_read_bit() & 0x01) << 11);
}

READ16_HANDLER( guwange_input1_r )
{
	return readinputport(1) | ((EEPROM_read_bit() & 0x01) << 7);
}

WRITE16_HANDLER( cave_eeprom_msb_w )
{
	if (data & ~0xfe00)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_MSB )  // even address
	{
		coin_lockout_w(1,~data & 0x8000);
		coin_lockout_w(0,~data & 0x4000);
		coin_counter_w(1, data & 0x2000);
		coin_counter_w(0, data & 0x1000);

		// latch the bit
		EEPROM_write_bit(data & 0x0800);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE );
	}
}

WRITE16_HANDLER( sailormn_eeprom_msb_w )
{
	sailormn_tilebank_w    ( data &  0x0100 );
	cave_eeprom_msb_w(offset,data & ~0x0100,mem_mask);
}

WRITE16_HANDLER( hotdogst_eeprom_msb_w )
{
	if ( ACCESSING_MSB )  // even address
	{
		// latch the bit
		EEPROM_write_bit(data & 0x0800);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0400) ? CLEAR_LINE: ASSERT_LINE );
	}
}

WRITE16_HANDLER( cave_eeprom_lsb_w )
{
	if (data & ~0x00ef)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_LSB )  // odd address
	{
		coin_lockout_w(1,~data & 0x0008);
		coin_lockout_w(0,~data & 0x0004);
		coin_counter_w(1, data & 0x0002);
		coin_counter_w(0, data & 0x0001);

		// latch the bit
		EEPROM_write_bit(data & 0x80);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE );
	}
}

/*	- No coin lockouts
	- Writing 0xcf00 shouldn't send a 1 bit to the eeprom	*/
WRITE16_HANDLER( metmqstr_eeprom_msb_w )
{
	if (data & ~0xff00)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_MSB )  // even address
	{
		coin_counter_w(1, data & 0x2000);
		coin_counter_w(0, data & 0x1000);

		if (~data & 0x0100)
		{
			// latch the bit
			EEPROM_write_bit(data & 0x0800);

			// reset line asserted: reset.
			EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

			// clock line asserted: write latch or select next bit to read
			EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE );
		}
	}
}

NVRAM_HANDLER( cave )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			if (cave_default_eeprom)	/* Set the EEPROM to Factory Defaults */
				EEPROM_set_data(cave_default_eeprom,cave_default_eeprom_length);
		}
	}
}


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

/*	Lines starting with an empty comment in the following MemoryReadAddress
	 arrays are there for debug (e.g. the game does not read from those ranges
	AFAIK)	*/

/***************************************************************************
								Dangun Feveron
***************************************************************************/

static MEMORY_READ16_START( dfeveron_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x300002, 0x300003, cave_sound_r			},	// YMZ280
/**/{ 0x400000, 0x407fff, MRA16_RAM				},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA16_RAM				},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA16_RAM				},	// Layer 0
/**/{ 0x600000, 0x607fff, MRA16_RAM				},	// Layer 1
/**/{ 0x708000, 0x708fff, MRA16_RAM				},	// Palette
/**/{ 0x710000, 0x710fff, MRA16_RAM				},	// ?
	{ 0x800000, 0x800007, cave_irq_cause_r		},	// IRQ Cause
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// Layer 1 Control
	{ 0xb00000, 0xb00001, input_port_0_word_r	},	// Inputs
	{ 0xb00002, 0xb00003, cave_input1_r			},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( dfeveron_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x300003, cave_sound_w					},	// YMZ280
	{ 0x400000, 0x407fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA16_RAM						},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x607fff, cave_vram_1_w, &cave_vram_1	},	// Layer 1
	{ 0x708000, 0x708fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0x710c00, 0x710fff, MWA16_RAM						},	// ?
	{ 0x800000, 0x80007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0x900000, 0x900005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA16_RAM, &cave_vctrl_1		},	// Layer 1 Control
	{ 0xc00000, 0xc00001, cave_eeprom_msb_w				},	// EEPROM
MEMORY_END


/***************************************************************************
								Dodonpachi
***************************************************************************/

static MEMORY_READ16_START( ddonpach_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x300002, 0x300003, cave_sound_r			},	// YMZ280
/**/{ 0x400000, 0x407fff, MRA16_RAM				},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA16_RAM				},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA16_RAM				},	// Layer 0
/**/{ 0x600000, 0x607fff, MRA16_RAM				},	// Layer 1
/**/{ 0x700000, 0x70ffff, MRA16_RAM				},	// Layer 2
	{ 0x800000, 0x800007, ddonpach_irq_cause_r	},	// IRQ Cause
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 2 Control
/**/{ 0xc00000, 0xc0ffff, MRA16_RAM				},	// Palette
	{ 0xd00000, 0xd00001, input_port_0_word_r	},	// Inputs
	{ 0xd00002, 0xd00003, cave_input1_r			},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( ddonpach_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM							},	// RAM
	{ 0x300000, 0x300003, cave_sound_w						},	// YMZ280
	{ 0x400000, 0x407fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA16_RAM							},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w,     &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x607fff, cave_vram_1_w,     &cave_vram_1	},	// Layer 1
	{ 0x700000, 0x70ffff, cave_vram_2_8x8_w, &cave_vram_2	},	// Layer 2
	{ 0x800000, 0x80007f, MWA16_RAM, &cave_videoregs		},	// Video Regs
	{ 0x900000, 0x900005, MWA16_RAM, &cave_vctrl_0			},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA16_RAM, &cave_vctrl_1			},	// Layer 1 Control
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_2			},	// Layer 2 Control
	{ 0xc00000, 0xc0ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xe00000, 0xe00001, cave_eeprom_msb_w					},	// EEPROM
MEMORY_END


/***************************************************************************
									Donpachi
***************************************************************************/

READ16_HANDLER( donpachi_videoregs_r )
{
	switch( offset )
	{
		case 0:
		case 1:
		case 2:
		case 3:	return cave_irq_cause_r(offset,0);

		default:	return 0x0000;
	}
}

#if 0
WRITE16_HANDLER( donpachi_videoregs_w )
{
	COMBINE_DATA(&cave_videoregs[offset]);

	switch( offset )
	{
//		case 0x78/2:	watchdog_reset16_w(0,0);	break;
	}
}
#endif

static WRITE16_HANDLER( nmk_oki6295_bankswitch_w )
{
	if (Machine->sample_rate == 0)	return;

	if (ACCESSING_LSB)
	{
		/* The OKI6295 ROM space is divided in four banks, each one indepentently
		   controlled. The sample table at the beginning of the addressing space is
		   divided in four pages as well, banked together with the sample data. */

		#define TABLESIZE 0x100
		#define BANKSIZE 0x10000

		int chip	=	offset / 4;
		int banknum	=	offset % 4;

		unsigned char *rom	=	memory_region(REGION_SOUND1 + chip);
		int size			=	memory_region_length(REGION_SOUND1 + chip) - 0x40000;

		int bankaddr		=	(data * BANKSIZE) % size;	// % used: size is not a power of 2

		/* copy the samples */
		memcpy(rom + banknum * BANKSIZE,rom + 0x40000 + bankaddr,BANKSIZE);

		/* and also copy the samples address table (only for chip #1) */
		if (chip==1)
		{
			rom += banknum * TABLESIZE;
			memcpy(rom,rom + 0x40000 + bankaddr,TABLESIZE);
		}
	}
}

static MEMORY_READ16_START( donpachi_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM					},	// RAM
	{ 0x200000, 0x207fff, MRA16_RAM					},	// Layer 1
	{ 0x300000, 0x307fff, MRA16_RAM					},	// Layer 0
	{ 0x400000, 0x407fff, MRA16_RAM					},	// Layer 2
	{ 0x500000, 0x507fff, MRA16_RAM					},	// Sprites
	{ 0x508000, 0x50ffff, MRA16_RAM					},	// Sprites?
/**/{ 0x600000, 0x600005, MRA16_RAM					},	// Layer 0 Control
/**/{ 0x700000, 0x700005, MRA16_RAM					},	// Layer 1 Control
/**/{ 0x800000, 0x800005, MRA16_RAM					},	// Layer 2 Control
	{ 0x900000, 0x90007f, donpachi_videoregs_r		},	// Video Regs
/**/{ 0xa08000, 0xa08fff, MRA16_RAM					},	// Palette
	{ 0xb00000, 0xb00001, OKIM6295_status_0_lsb_r	},	// M6295
	{ 0xb00010, 0xb00011, OKIM6295_status_1_lsb_r	},	//
	{ 0xc00000, 0xc00001, input_port_0_word_r		},	// Inputs
	{ 0xc00002, 0xc00003, cave_input1_r				},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( donpachi_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM							},	// RAM
	{ 0x200000, 0x207fff, cave_vram_1_w,     &cave_vram_1	},	// Layer 1
	{ 0x300000, 0x307fff, cave_vram_0_w,     &cave_vram_0	},	// Layer 0
	{ 0x400000, 0x407fff, cave_vram_2_8x8_w, &cave_vram_2	},	// Layer 2
	{ 0x500000, 0x507fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x508000, 0x50ffff, MWA16_RAM							},	// Sprites?
	{ 0x600000, 0x600005, MWA16_RAM, &cave_vctrl_1			},	// Layer 1 Control
	{ 0x700000, 0x700005, MWA16_RAM, &cave_vctrl_0			},	// Layer 0 Control
	{ 0x800000, 0x800005, MWA16_RAM, &cave_vctrl_2			},	// Layer 2 Control
	{ 0x900000, 0x90007f, MWA16_RAM, &cave_videoregs		},	// Video Regs
	{ 0xa08000, 0xa08fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xb00000, 0xb00003, OKIM6295_data_0_lsb_w				},	// M6295
	{ 0xb00010, 0xb00013, OKIM6295_data_1_lsb_w				},	//
	{ 0xb00020, 0xb0002f, nmk_oki6295_bankswitch_w			},	//
	{ 0xd00000, 0xd00001, cave_eeprom_msb_w					},	// EEPROM
MEMORY_END


/***************************************************************************
									Esprade
***************************************************************************/

static MEMORY_READ16_START( esprade_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x300002, 0x300003, cave_sound_r			},	// YMZ280
/**/{ 0x400000, 0x407fff, MRA16_RAM				},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA16_RAM				},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA16_RAM				},	// Layer 0
/**/{ 0x600000, 0x607fff, MRA16_RAM				},	// Layer 1
/**/{ 0x700000, 0x707fff, MRA16_RAM				},	// Layer 2
	{ 0x800000, 0x800007, cave_irq_cause_r		},	// IRQ Cause
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 2 Control
/**/{ 0xc00000, 0xc0ffff, MRA16_RAM				},	// Palette
	{ 0xd00000, 0xd00001, input_port_0_word_r	},	// Inputs
	{ 0xd00002, 0xd00003, cave_input1_r			},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( esprade_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x300003, cave_sound_w					},	// YMZ280
	{ 0x400000, 0x407fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA16_RAM						},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x607fff, cave_vram_1_w, &cave_vram_1	},	// Layer 1
	{ 0x700000, 0x707fff, cave_vram_2_w, &cave_vram_2	},	// Layer 2
	{ 0x800000, 0x80007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0x900000, 0x900005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA16_RAM, &cave_vctrl_1		},	// Layer 1 Control
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_2		},	// Layer 2 Control
	{ 0xc00000, 0xc0ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xe00000, 0xe00001, cave_eeprom_msb_w				},	// EEPROM
MEMORY_END


/***************************************************************************
									Guwange
***************************************************************************/

static MEMORY_READ16_START( guwange_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x300000, 0x300007, cave_irq_cause_r		},	// IRQ Cause
/**/{ 0x400000, 0x407fff, MRA16_RAM				},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA16_RAM				},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA16_RAM				},	// Layer 0
/**/{ 0x600000, 0x607fff, MRA16_RAM				},	// Layer 1
/**/{ 0x700000, 0x707fff, MRA16_RAM				},	// Layer 2
	{ 0x800002, 0x800003, cave_sound_r			},	// YMZ280
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 2 Control
/**/{ 0xc00000, 0xc0ffff, MRA16_RAM				},	// Palette
	{ 0xd00010, 0xd00011, input_port_0_word_r	},	// Inputs
	{ 0xd00012, 0xd00013, guwange_input1_r		},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( guwange_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x30007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0x400000, 0x407fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA16_RAM						},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x607fff, cave_vram_1_w, &cave_vram_1	},	// Layer 1
	{ 0x700000, 0x707fff, cave_vram_2_w, &cave_vram_2	},	// Layer 2
	{ 0x800000, 0x800003, cave_sound_w					},	// YMZ280
	{ 0x900000, 0x900005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA16_RAM, &cave_vctrl_1		},	// Layer 1 Control
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_2		},	// Layer 2 Control
	{ 0xc00000, 0xc0ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xd00010, 0xd00011, cave_eeprom_lsb_w				},	// EEPROM
//	{ 0xd00012, 0xd00013, MWA16_NOP				},	// ?
//	{ 0xd00014, 0xd00015, MWA16_NOP				},	// ? $800068 in dfeveron ?
MEMORY_END


/***************************************************************************
								Hotdog Storm
***************************************************************************/

static MEMORY_READ16_START( hotdogst_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x300000, 0x30ffff, MRA16_RAM				},	// RAM
/**/{ 0x408000, 0x408fff, MRA16_RAM				},	// Palette
/**/{ 0x880000, 0x887fff, MRA16_RAM				},	// Layer 0
/**/{ 0x900000, 0x907fff, MRA16_RAM				},	// Layer 1
/**/{ 0x980000, 0x987fff, MRA16_RAM				},	// Layer 2
	{ 0xa80000, 0xa80007, cave_irq_cause_r		},	// IRQ Cause
//	{ 0xa8006e, 0xa8006f, soundlatch_ack_r		},	// From Sound CPU
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xb80000, 0xb80005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xc00000, 0xc00005, MRA16_RAM				},	// Layer 2 Control
	{ 0xc80000, 0xc80001, input_port_0_word_r	},	// Inputs
	{ 0xc80002, 0xc80003, cave_input1_r			},	// Inputs + EEPROM
/**/{ 0xf00000, 0xf07fff, MRA16_RAM				},	// Sprites
/**/{ 0xf08000, 0xf0ffff, MRA16_RAM				},	// Sprites?
MEMORY_END

static MEMORY_WRITE16_START( hotdogst_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x300000, 0x30ffff, MWA16_RAM						},	// RAM
	{ 0x408000, 0x408fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0x880000, 0x887fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x900000, 0x907fff, cave_vram_1_w, &cave_vram_1	},	// Layer 1
	{ 0x980000, 0x987fff, cave_vram_2_w, &cave_vram_2	},	// Layer 2
	{ 0xa8006e, 0xa8006f, sound_cmd_w					},	// To Sound CPU
	{ 0xa80000, 0xa8007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0xb80000, 0xb80005, MWA16_RAM, &cave_vctrl_1		},	// Layer 1 Control
	{ 0xc00000, 0xc00005, MWA16_RAM, &cave_vctrl_2		},	// Layer 2 Control
	{ 0xd00000, 0xd00001, hotdogst_eeprom_msb_w			},	// EEPROM
	{ 0xd00002, 0xd00003, MWA16_NOP						},	// ???
	{ 0xf00000, 0xf07fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0xf08000, 0xf0ffff, MWA16_RAM						},	// Sprites?
MEMORY_END


/***************************************************************************
								Mazinger Z
***************************************************************************/

static MEMORY_READ16_START( mazinger_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
/**/{ 0x200000, 0x207fff, MRA16_RAM				},	// Sprites
/**/{ 0x208000, 0x20ffff, MRA16_RAM				},	// Sprites?
	{ 0x300000, 0x300007, cave_irq_cause_r		},	// IRQ Cause
	{ 0x30006e, 0x30006f, soundlatch_ack_r		},	// From Sound CPU
/**/{ 0x404000, 0x407fff, MRA16_RAM				},	// Layer 1
/**/{ 0x504000, 0x507fff, MRA16_RAM				},	// Layer 0
/**/{ 0x600000, 0x600005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0x700000, 0x700005, MRA16_RAM				},	// Layer 0 Control
	{ 0x800000, 0x800001, input_port_0_word_r	},	// Inputs
	{ 0x800002, 0x800003, cave_input1_r			},	// Inputs + EEPROM
/**/{ 0xc08000, 0xc0ffff, MRA16_RAM				},	// Palette
	{ 0xd00000, 0xd7ffff, MRA16_BANK1			},	// ROM
MEMORY_END

static MEMORY_WRITE16_START( mazinger_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM							},	// RAM
	{ 0x200000, 0x207fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x208000, 0x20ffff, MWA16_RAM							},	// Sprites?
	{ 0x300068, 0x300069, watchdog_reset16_w				},	// Watchdog
	{ 0x30006e, 0x30006f, sound_cmd_w						},	// To Sound CPU
	{ 0x300000, 0x30007f, MWA16_RAM, &cave_videoregs		},	// Video Regs
	{ 0x400000, 0x407fff, cave_vram_1_8x8_w, &cave_vram_1	},	// Layer 1
	{ 0x500000, 0x507fff, cave_vram_0_8x8_w, &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x600005, MWA16_RAM, &cave_vctrl_1			},	// Layer 1 Control
	{ 0x700000, 0x700005, MWA16_RAM, &cave_vctrl_0			},	// Layer 0 Control
	{ 0x900000, 0x900001, cave_eeprom_msb_w					},	// EEPROM
	{ 0xc08000, 0xc0ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xd00000, 0xd7ffff, MWA16_ROM							},	// ROM
MEMORY_END


/***************************************************************************
								Metamoqester
***************************************************************************/

static MEMORY_READ16_START( metmqstr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x17ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x27ffff, MRA16_ROM				},	// ROM
	{ 0x408000, 0x408fff, MRA16_RAM				},	// Palette
	{ 0x600000, 0x600001, watchdog_reset16_r	},	// Watchdog?
	{ 0x880000, 0x887fff, MRA16_RAM				},	// Layer 2
	{ 0x888000, 0x88ffff, MRA16_RAM				},	//
	{ 0x900000, 0x907fff, MRA16_RAM				},	// Layer 1
	{ 0x908000, 0x90ffff, MRA16_RAM				},	//
	{ 0x980000, 0x987fff, MRA16_RAM				},	// Layer 0
	{ 0x988000, 0x98ffff, MRA16_RAM				},	//
	{ 0xa80000, 0xa80007, cave_irq_cause_r		},	// IRQ Cause
	{ 0xa8006c, 0xa8006d, soundflags_ack_r		},	// Communication
	{ 0xa8006e, 0xa8006f, soundlatch_ack_r		},	// From Sound CPU
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xb80000, 0xb80005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xc00000, 0xc00005, MRA16_RAM				},	// Layer 2 Control
	{ 0xc80000, 0xc80001, input_port_0_word_r	},	// Inputs
	{ 0xc80002, 0xc80003, cave_input1_r			},	// Inputs + EEPROM
	{ 0xf00000, 0xf07fff, MRA16_RAM				},	// Sprites
	{ 0xf08000, 0xf0ffff, MRA16_RAM				},	// RAM
MEMORY_END

static MEMORY_WRITE16_START( metmqstr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x17ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x27ffff, MWA16_ROM						},	// ROM
	{ 0x408000, 0x408fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0x880000, 0x887fff, cave_vram_2_w, &cave_vram_2	},	// Layer 2
	{ 0x888000, 0x88ffff, MWA16_RAM						},	//
	{ 0x900000, 0x907fff, cave_vram_1_w, &cave_vram_1	},	// Layer 1
	{ 0x908000, 0x90ffff, MWA16_RAM						},	//
	{ 0x980000, 0x987fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x988000, 0x98ffff, MWA16_RAM						},	//
	{ 0xa80068, 0xa80069, watchdog_reset16_w			},	// Watchdog?
	{ 0xa8006c, 0xa8006d, MWA16_NOP						},	// ?
	{ 0xa8006e, 0xa8006f, sound_cmd_w					},	// To Sound CPU
	{ 0xa80000, 0xa8007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_2		},	// Layer 2 Control
	{ 0xb80000, 0xb80005, MWA16_RAM, &cave_vctrl_1		},	// Layer 1 Control
	{ 0xc00000, 0xc00005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0xd00000, 0xd00001, metmqstr_eeprom_msb_w			},	// EEPROM
	{ 0xf00000, 0xf07fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0xf08000, 0xf0ffff, MWA16_RAM						},	// RAM
MEMORY_END


/***************************************************************************
								Power Instinct 2
***************************************************************************/

READ16_HANDLER( pwrinst2_eeprom_r )
{
	return ~8 + ((EEPROM_read_bit() & 1) ? 8 : 0);
}

INLINE void vctrl_w(data16_t *VCTRL, UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask)
{
	if ( offset == 4/2 )
	{
		switch( data & 0x000f )
		{
			case 1:	data = (data & ~0x000f) | 0;	break;
			case 2:	data = (data & ~0x000f) | 1;	break;
			case 4:	data = (data & ~0x000f) | 2;	break;
			default:
			case 8:	data = (data & ~0x000f) | 3;	break;
		}
	}
	COMBINE_DATA(&VCTRL[offset]);
}
WRITE16_HANDLER( pwrinst2_vctrl_0_w )	{ vctrl_w(cave_vctrl_0, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_1_w )	{ vctrl_w(cave_vctrl_1, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_2_w )	{ vctrl_w(cave_vctrl_2, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_3_w )	{ vctrl_w(cave_vctrl_3, offset, data, mem_mask); }

static MEMORY_READ16_START( pwrinst2_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM					},	// ROM
	{ 0x400000, 0x40ffff, MRA16_RAM					},	// RAM
	{ 0x500000, 0x500001, input_port_0_word_r		},	// Inputs
	{ 0x500002, 0x500003, input_port_1_word_r		},	//
	{ 0x800000, 0x807fff, MRA16_RAM					},	// Layer 0
	{ 0x880000, 0x887fff, MRA16_RAM					},	// Layer 1
	{ 0x900000, 0x907fff, MRA16_RAM					},	// Layer 2
	{ 0x980000, 0x987fff, MRA16_RAM					},	// Layer 3
	{ 0xa00000, 0xa07fff, MRA16_RAM					},	// Sprites
	{ 0xa08000, 0xa0ffff, MRA16_RAM					},	// Sprites?
	{ 0xa10000, 0xa1ffff, MRA16_RAM					},	// Sprites?
/**/{ 0xb00000, 0xb00005, MRA16_RAM					},	// Layer 0 Control
/**/{ 0xb80000, 0xb80005, MRA16_RAM					},	// Layer 1 Control
/**/{ 0xc00000, 0xc00005, MRA16_RAM					},	// Layer 2 Control
/**/{ 0xc80000, 0xc80005, MRA16_RAM					},	// Layer 3 Control
	{ 0xa80000, 0xa8007f, donpachi_videoregs_r		},	// Video Regs
	{ 0xd80000, 0xd80001, MRA16_NOP					},	// ? From Sound CPU
	{ 0xe80000, 0xe80001, pwrinst2_eeprom_r			},	// EEPROM
	{ 0xf00000, 0xf04fff, MRA16_RAM					},	// Palette
MEMORY_END

static MEMORY_WRITE16_START( pwrinst2_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM							},	// ROM
	{ 0x400000, 0x40ffff, MWA16_RAM							},	// RAM
	{ 0x700000, 0x700001, cave_eeprom_msb_w					},	// EEPROM
	{ 0x800000, 0x807fff, cave_vram_0_w,     &cave_vram_0	},	// Layer 0
	{ 0x880000, 0x887fff, cave_vram_1_w,     &cave_vram_1	},	// Layer 1
	{ 0x900000, 0x907fff, cave_vram_2_w,     &cave_vram_2	},	// Layer 2
	{ 0x980000, 0x987fff, cave_vram_3_8x8_w, &cave_vram_3	},	// Layer 3
	{ 0xa00000, 0xa07fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0xa08000, 0xa0ffff, MWA16_RAM							},	// Sprites?
	{ 0xa10000, 0xa1ffff, MWA16_RAM							},	// Sprites?
	{ 0xa80000, 0xa8007f, MWA16_RAM, &cave_videoregs		},	// Video Regs
	{ 0xb00000, 0xb00005, pwrinst2_vctrl_0_w, &cave_vctrl_0			},	// Layer 0 Control
	{ 0xb80000, 0xb80005, pwrinst2_vctrl_1_w, &cave_vctrl_1			},	// Layer 1 Control
	{ 0xc00000, 0xc00005, pwrinst2_vctrl_2_w, &cave_vctrl_2			},	// Layer 2 Control
	{ 0xc80000, 0xc80005, pwrinst2_vctrl_3_w, &cave_vctrl_3			},	// Layer 3 Control
	{ 0xe00000, 0xe00001, sound_cmd_w						},	// To Sound CPU
	{ 0xf00000, 0xf04fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
MEMORY_END


/***************************************************************************
								Sailorm Moon
***************************************************************************/

static READ16_HANDLER( sailormn_input0_r )
{
//	watchdog_reset16_r(0,0);	// written too rarely for mame.
	return readinputport(0);
}

/*
	sailormn and agallet wait for bit 2 of $b80001 to go 1 -> 0.
	It must happen once per frame as agallet uses this to show
	the copyright notice screen for ~8.5s
*/
static READ16_HANDLER( sailormn_irq_cause_r )
{
	data16_t irq_cause = cave_irq_cause_r(offset,mem_mask);

	if (offset == 0)
	{
		irq_cause &= ~4;
		irq_cause |= (cpu_getvblank()?0:4);
	}

	return irq_cause;
}

static READ16_HANDLER( agallet_irq_cause_r )
{
	data16_t irq_cause = cave_irq_cause_r(offset,mem_mask);

	if (offset == 0)
	{
		irq_cause &= ~4;
		irq_cause |= (cpu_getvblank()?0:4);

// Speed hack for agallet
		if ((activecpu_get_pc() == 0xcdca) && (irq_cause & 4))
			cpu_spinuntil_int();
	}

	return irq_cause;
}

static MEMORY_READ16_START( sailormn_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x110000, 0x110001, MRA16_RAM				},	// (agallet)
	{ 0x200000, 0x3fffff, MRA16_ROM				},	// ROM
	{ 0x400000, 0x407fff, MRA16_RAM				},	// (agallet)
	{ 0x408000, 0x40bfff, MRA16_RAM				},	// Palette
	{ 0x40c000, 0x40ffff, MRA16_RAM				},	// (agallet)
	{ 0x410000, 0x410001, MRA16_RAM				},	// (agallet)
	{ 0x500000, 0x507fff, MRA16_RAM				},	// Sprites
	{ 0x508000, 0x50ffff, MRA16_RAM				},	// Sprites?
	{ 0x510000, 0x510001, MRA16_RAM				},	// (agallet)
	{ 0x600000, 0x600001, sailormn_input0_r		},	// Inputs + Watchdog!
	{ 0x600002, 0x600003, cave_input1_r			},	// Inputs + EEPROM
	{ 0x800000, 0x887fff, MRA16_RAM				},	// Layer 0
	{ 0x880000, 0x887fff, MRA16_RAM				},	// Layer 1
	{ 0x900000, 0x907fff, MRA16_RAM				},	// Layer 2
	{ 0x908000, 0x908001, MRA16_RAM				},	// (agallet)
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0xa80000, 0xa80005, MRA16_RAM				},	// Layer 1 Control
/**/{ 0xb00000, 0xb00005, MRA16_RAM				},	// Layer 2 Control
	{ 0xb80000, 0xb80007, sailormn_irq_cause_r	},	// IRQ Cause (bit 2 tested!)
	{ 0xb8006c, 0xb8006d, soundflags_ack_r		},	// Communication
	{ 0xb8006e, 0xb8006f, soundlatch_ack_r		},	// From Sound CPU
MEMORY_END

static MEMORY_WRITE16_START( sailormn_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM							},	// RAM
	{ 0x110000, 0x110001, MWA16_RAM							},	// (agallet)
	{ 0x200000, 0x3fffff, MWA16_ROM							},	// ROM
	{ 0x400000, 0x407fff, MWA16_RAM							},	// (agallet)
	{ 0x408000, 0x40bfff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0x40c000, 0x40ffff, MWA16_RAM							},	// (agallet)
	{ 0x410000, 0x410001, MWA16_RAM							},	// (agallet)
	{ 0x500000, 0x507fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x508000, 0x50ffff, MWA16_RAM							},	// Sprites?
	{ 0x510000, 0x510001, MWA16_RAM							},	// (agallet)
	{ 0x700000, 0x700001, sailormn_eeprom_msb_w				},	// EEPROM
	{ 0x800000, 0x807fff, cave_vram_0_w, &cave_vram_0		},	// Layer 0
	{ 0x880000, 0x887fff, cave_vram_1_w, &cave_vram_1		},	// Layer 1
	{ 0x900000, 0x907fff, cave_vram_2_w, &cave_vram_2		},	// Layer 2
	{ 0x908000, 0x908001, MWA16_RAM							},	// (agallet)
	{ 0xa00000, 0xa00005, MWA16_RAM, &cave_vctrl_0			},	// Layer 0 Control
	{ 0xa80000, 0xa80005, MWA16_RAM, &cave_vctrl_1			},	// Layer 1 Control
	{ 0xb00000, 0xb00005, MWA16_RAM, &cave_vctrl_2			},	// Layer 2 Control
	{ 0xb8006e, 0xb8006f, sound_cmd_w						},	// To Sound CPU
	{ 0xb80000, 0xb8007f, MWA16_RAM, &cave_videoregs		},	// Video Regs
MEMORY_END


/***************************************************************************
									Uo Poko
***************************************************************************/

static MEMORY_READ16_START( uopoko_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x300002, 0x300003, cave_sound_r			},	// YMZ280
/**/{ 0x400000, 0x407fff, MRA16_RAM				},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA16_RAM				},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA16_RAM				},	// Layer 0
	{ 0x600000, 0x600007, cave_irq_cause_r		},	// IRQ Cause
/**/{ 0x700000, 0x700005, MRA16_RAM				},	// Layer 0 Control
/**/{ 0x800000, 0x80ffff, MRA16_RAM				},	// Palette
	{ 0x900000, 0x900001, input_port_0_word_r	},	// Inputs
	{ 0x900002, 0x900003, cave_input1_r			},	// Inputs + EEPROM
MEMORY_END

static MEMORY_WRITE16_START( uopoko_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x300003, cave_sound_w					},	// YMZ280
	{ 0x400000, 0x407fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA16_RAM						},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0	},	// Layer 0
	{ 0x600000, 0x60007f, MWA16_RAM, &cave_videoregs	},	// Video Regs
	{ 0x700000, 0x700005, MWA16_RAM, &cave_vctrl_0		},	// Layer 0 Control
	{ 0x800000, 0x80ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },	// Palette
	{ 0xa00000, 0xa00001, cave_eeprom_msb_w				},	// EEPROM
MEMORY_END



/***************************************************************************


						Memory Maps - Sound CPU (Optional)


***************************************************************************/

/***************************************************************************
								Hotdog Storm
***************************************************************************/

WRITE_HANDLER( hotdogst_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x0f;
	if ( data & ~0x0f )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	cpu_setbank(2, &RAM[ 0x4000 * bank ]);
}

WRITE_HANDLER( hotdogst_okibank_w )
{
	data8_t *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0x3;
	int bank2 = (data >> 4) & 0x3;
	if (Machine->sample_rate == 0)	return;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static MEMORY_READ_START( hotdogst_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK2	},	// ROM (Banked)
	{ 0xe000, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( hotdogst_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM	},	// ROM (Banked)
	{ 0xe000, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( hotdogst_sound_readport )
	{ 0x30, 0x30, soundlatch_lo_r			},	// From Main CPU
	{ 0x40, 0x40, soundlatch_hi_r			},	//
	{ 0x50, 0x50, YM2203_status_port_0_r	},	// YM2203
	{ 0x51, 0x51, YM2203_read_port_0_r		},	//
	{ 0x60, 0x60, OKIM6295_status_0_r		},	// M6295
PORT_END

static PORT_WRITE_START( hotdogst_sound_writeport )
	{ 0x00, 0x00, hotdogst_rombank_w		},	// ROM bank
	{ 0x50, 0x50, YM2203_control_port_0_w	},	// YM2203
	{ 0x51, 0x51, YM2203_write_port_0_w		},	//
	{ 0x60, 0x60, OKIM6295_data_0_w			},	// M6295
	{ 0x70, 0x70, hotdogst_okibank_w		},	// Samples bank
PORT_END

/***************************************************************************
								Mazinger Z
***************************************************************************/

WRITE_HANDLER( mazinger_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x07;
	if ( data & ~0x07 )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	cpu_setbank(2, &RAM[ 0x4000 * bank ]);
}

static MEMORY_READ_START( mazinger_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK2	},	// ROM (Banked)
	{ 0xc000, 0xc7ff, MRA_RAM	},	// RAM
	{ 0xf800, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( mazinger_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM	},	// ROM (Banked)
	{ 0xc000, 0xc7ff, MWA_RAM	},	// RAM
	{ 0xf800, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( mazinger_sound_readport )
	{ 0x30, 0x30, soundlatch_lo_r			},	// From Main CPU
	{ 0x52, 0x52, YM2203_status_port_0_r	},	// YM2203
PORT_END

static PORT_WRITE_START( mazinger_sound_writeport )
	{ 0x00, 0x00, mazinger_rombank_w		},	// ROM bank
	{ 0x10, 0x10, soundlatch_ack_w			},	// To Main CPU
	{ 0x50, 0x50, YM2203_control_port_0_w	},	// YM2203
	{ 0x51, 0x51, YM2203_write_port_0_w		},	//
	{ 0x70, 0x70, OKIM6295_data_0_w			},	// M6295
	{ 0x74, 0x74, hotdogst_okibank_w		},	// Samples bank
PORT_END


/***************************************************************************
								Metamoqester
***************************************************************************/

WRITE_HANDLER( metmqstr_rombank_w )
{
	data8_t *ROM = memory_region(REGION_CPU2);
	int bank = data & 0xf;
	if ( bank != data )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank >= 2)	bank += 2;
	cpu_setbank(1, &ROM[ 0x4000 * bank ]);
}

WRITE_HANDLER( metmqstr_okibank0_w )
{
	data8_t *ROM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0x7;
	int bank2 = (data >> 4) & 0x7;
	if (Machine->sample_rate == 0)	return;
	memcpy(ROM + 0x20000 * 0, ROM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(ROM + 0x20000 * 1, ROM + 0x40000 + 0x20000 * bank2, 0x20000);
}

WRITE_HANDLER( metmqstr_okibank1_w )
{
	data8_t *ROM = memory_region(REGION_SOUND2);
	int bank1 = (data >> 0) & 0x7;
	int bank2 = (data >> 4) & 0x7;
	if (Machine->sample_rate == 0)	return;
	memcpy(ROM + 0x20000 * 0, ROM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(ROM + 0x20000 * 1, ROM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static MEMORY_READ_START( metmqstr_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK1	},	// ROM (Banked)
	{ 0xe000, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( metmqstr_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM	},	// ROM (Banked)
	{ 0xe000, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( metmqstr_sound_readport )
	{ 0x20, 0x20, soundflags_r				},	// Communication
	{ 0x30, 0x30, soundlatch_lo_r			},	// From Main CPU
	{ 0x40, 0x40, soundlatch_hi_r			},	//
	{ 0x51, 0x51, YM2151_status_port_0_r	},	// YM2151
PORT_END

static PORT_WRITE_START( metmqstr_sound_writeport )
	{ 0x00, 0x00, metmqstr_rombank_w		},	// Rom Bank
	{ 0x50, 0x50, YM2151_register_port_0_w	},	// YM2151
	{ 0x51, 0x51, YM2151_data_port_0_w		},	//
	{ 0x60, 0x60, OKIM6295_data_0_w			},	// M6295 #0
	{ 0x70, 0x70, metmqstr_okibank0_w		},	// Samples Bank #0
	{ 0x80, 0x80, OKIM6295_data_1_w			},	// M6295 #1
	{ 0x90, 0x90, metmqstr_okibank1_w		},	// Samples Bank #1
PORT_END


/***************************************************************************
								Power Instinct 2
***************************************************************************/

// TODO : FIX SAMPLES TABLE BEING OVERWRITTEN IN DONPACHI
static WRITE_HANDLER( pwrinst2_okibank_w )
{
	/* The OKI6295 ROM space is divided in four banks, each one indepentently
	   controlled. The sample table at the beginning of the addressing space is
	   divided in four pages as well, banked together with the sample data. */

	#define TABLESIZE 0x100
	#define BANKSIZE 0x10000

	int chip	=	offset / 4;
	int banknum	=	offset % 4;

	unsigned char *rom	=	memory_region(REGION_SOUND1 + chip);
	int size			=	memory_region_length(REGION_SOUND1 + chip) - 0x40000;

	int bankaddr		=	data * BANKSIZE;

	if (Machine->sample_rate == 0)	return;

	if (bankaddr >= size)
	{
		bankaddr %= size;
logerror("CPU #1 - PC %06X: chip %d bank %X<-%02X\n",activecpu_get_pc(),chip,banknum,data);
	}

	/* copy the samples */
	if (banknum == 0)		/* skip table */
		memcpy(rom + banknum * BANKSIZE+0x400,rom + 0x40000 + bankaddr+0x400,BANKSIZE-0x400);
	else
		memcpy(rom + banknum * BANKSIZE,rom + 0x40000 + bankaddr,BANKSIZE);

	/* and also copy the samples address table (only for chip #1) */
	rom += banknum * TABLESIZE;
	memcpy(rom,rom + 0x40000 + bankaddr,TABLESIZE);
}

WRITE_HANDLER( pwrinst2_rombank_w )
{
	data8_t *ROM = memory_region(REGION_CPU2);
	int bank = data & 0x07;
	if ( data & ~0x07 )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 2)	bank+=1;
	cpu_setbank(1, &ROM[ 0x4000 * bank ]);
}

static MEMORY_READ_START( pwrinst2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM	},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1	},	// ROM (Banked)
	{ 0xe000, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( pwrinst2_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM	},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM	},	// ROM (Banked)
	{ 0xe000, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( pwrinst2_sound_readport )
	{ 0x00, 0x00, OKIM6295_status_0_r		},	// M6295
	{ 0x08, 0x08, OKIM6295_status_1_r		},	//
	{ 0x40, 0x40, YM2203_status_port_0_r	},	// YM2203
	{ 0x41, 0x41, YM2203_read_port_0_r		},	//
	{ 0x60, 0x60, soundlatch_hi_r			},	// From Main CPU
	{ 0x70, 0x70, soundlatch_lo_r			},	//
PORT_END

static PORT_WRITE_START( pwrinst2_sound_writeport )
	{ 0x00, 0x00, OKIM6295_data_0_w			},	// M6295
	{ 0x08, 0x08, OKIM6295_data_1_w			},	//
	{ 0x10, 0x17, pwrinst2_okibank_w		},	// Samples bank
	{ 0x40, 0x40, YM2203_control_port_0_w	},	// YM2203
	{ 0x41, 0x41, YM2203_write_port_0_w		},	//
//	{ 0x50, 0x50, IOWP_NOP		},	// ?? volume
//	{ 0x51, 0x51, IOWP_NOP		},	// ?? volume
	{ 0x80, 0x80, pwrinst2_rombank_w		},	// ROM bank
PORT_END


/***************************************************************************
								Sailorm Moon
***************************************************************************/

static data8_t *mirror_ram;
static READ_HANDLER( mirror_ram_r )
{
	return mirror_ram[offset];
}
static WRITE_HANDLER( mirror_ram_w )
{
	mirror_ram[offset] = data;
}

WRITE_HANDLER( sailormn_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x1f;
	if ( data & ~0x1f )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	cpu_setbank(1, &RAM[ 0x4000 * bank ]);
}

WRITE_HANDLER( sailormn_okibank0_w )
{
	data8_t *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	if (Machine->sample_rate == 0)	return;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

WRITE_HANDLER( sailormn_okibank1_w )
{
	data8_t *RAM = memory_region(REGION_SOUND2);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	if (Machine->sample_rate == 0)	return;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static MEMORY_READ_START( sailormn_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM					},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK1					},	// ROM (Banked)
	{ 0xc000, 0xdfff, mirror_ram_r				},	// RAM
	{ 0xe000, 0xffff, mirror_ram_r				},	// Mirrored RAM (agallet)
MEMORY_END

static MEMORY_WRITE_START( sailormn_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM					},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM					},	// ROM (Banked)
	{ 0xc000, 0xdfff, mirror_ram_w, &mirror_ram	},	// RAM
	{ 0xe000, 0xffff, mirror_ram_w				},	// Mirrored RAM (agallet)
MEMORY_END

static PORT_READ_START( sailormn_sound_readport )
	{ 0x20, 0x20, soundflags_r				},	// Communication
	{ 0x30, 0x30, soundlatch_lo_r			},	// From Main CPU
	{ 0x40, 0x40, soundlatch_hi_r			},	//
	{ 0x51, 0x51, YM2151_status_port_0_r	},	// YM2151
	{ 0x60, 0x60, OKIM6295_status_0_r		},	// M6295 #0
	{ 0x80, 0x80, OKIM6295_status_1_r		},	// M6295 #1
PORT_END

static PORT_WRITE_START( sailormn_sound_writeport )
	{ 0x00, 0x00, sailormn_rombank_w		},	// Rom Bank
	{ 0x10, 0x10, soundlatch_ack_w			},	// To Main CPU
	{ 0x50, 0x50, YM2151_register_port_0_w	},	// YM2151
	{ 0x51, 0x51, YM2151_data_port_0_w		},	//
	{ 0x60, 0x60, OKIM6295_data_0_w			},	// M6295 #0
	{ 0x70, 0x70, sailormn_okibank0_w		},	// Samples Bank #0
	{ 0x80, 0x80, OKIM6295_data_1_w			},	// M6295 #1
	{ 0xc0, 0xc0, sailormn_okibank1_w		},	// Samples Bank #1
PORT_END



/***************************************************************************


								Input Ports


***************************************************************************/

/*
	dfeveron config menu:
	101624.w -> 8,a6	preferences
	101626.w -> c,a6	(1:coin<<4|credit) <<8 | (2:coin<<4|credit)
*/

/* Most games use this */
INPUT_PORTS_START( cave )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN1, 6)
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? exit service mode
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN2, 6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/* Different layout */
INPUT_PORTS_START( guwange )
	PORT_START	// IN0 - Player 1 & 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	// IN1 - Coins
	PORT_BIT_IMPULSE(  0x0001, IP_ACTIVE_LOW, IPT_COIN1, 6)
	PORT_BIT_IMPULSE(  0x0002, IP_ACTIVE_LOW, IPT_COIN2, 6)
	PORT_BITX( 0x0004, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* "normal" layout but with 4 buttons */
INPUT_PORTS_START( metmqstr )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN1, 6)
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN2, 6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/

/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{STEP4(0,1)},
	{STEP8(0,4)},
	{STEP8(0,4*8)},
	8*8*4
};

/* 8x8x6 tiles (in a 8x8x8 layout) */
static struct GfxLayout layout_8x8x6 =
{
	8,8,
	RGN_FRAC(1,1),
	6,
	{8,9, 0,1,2,3},
	{0*4,1*4,4*4,5*4,8*4,9*4,12*4,13*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64},
	8*8*8
};

/* 8x8x6 tiles (4 bits in one rom, 2 bits in the other,
   unpacked in 2 pages of 4 bits) */
static struct GfxLayout layout_8x8x6_2 =
{
	8,8,
	RGN_FRAC(1,2),
	6,
	{RGN_FRAC(1,2)+2,RGN_FRAC(1,2)+3, STEP4(0,1)},
	{STEP8(0,4)},
	{STEP8(0,4*8)},
	8*8*4
};

/* 8x8x8 tiles */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{8,9,10,11, 0,1,2,3},
	{0*4,1*4,4*4,5*4,8*4,9*4,12*4,13*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64},
	8*8*8
};

#if 0
/* 16x16x8 Zooming Sprites - No need to decode them */
static struct GfxLayout layout_sprites =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};
#endif

/***************************************************************************
								Dangun Feveron
***************************************************************************/

static struct GfxDecodeInfo dfeveron_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
	   the second half for tiles. We use $8000 virtual colors instead
	   for consistency with games having $8000 real colors.
	   A vh_init_palette function is thus needed for sprites */

//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4400, 0x40 }, // [1] Layer 1
	{ -1 }
};

/***************************************************************************
								Dodonpachi
***************************************************************************/

static struct GfxDecodeInfo ddonpach_gfxdecodeinfo[] =
{
	/* Layers 0&1 are 4 bit deep and use the first 16 of every 256
	   colors for any given color code (a vh_init_palette function
	   is provided for these layers, filling the 8000-83ff entries
	   in the color table). Layer 2 uses the whole 256 for any given
	   color code and the 4000-7fff range in the color table.	*/

//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x8000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x8000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x8,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								Donpachi
***************************************************************************/

static struct GfxDecodeInfo donpachi_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
	   the second half for tiles. We use $8000 virtual colors instead
	   for consistency with games having $8000 real colors.
	   A vh_init_palette function is thus needed for sprites */

//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4400, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x4400, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								Esprade
***************************************************************************/

static struct GfxDecodeInfo esprade_gfxdecodeinfo[] =
{
//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x8,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x8,	0x4000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x8,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								Hotdog Storm
***************************************************************************/

static struct GfxDecodeInfo hotdogst_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
	   the second half for tiles. We use $8000 virtual colors instead
	   for consistency with games having $8000 real colors.
	   A vh_init_palette function is needed for sprites */

//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								Mazinger Z
***************************************************************************/

static struct GfxDecodeInfo mazinger_gfxdecodeinfo[] =
{
	/*	Sprites are 4 bit deep.
		Layer 0 is 4 bit deep.
		Layer 1 uses 64 color palettes, but the game only fills the
		first 16 colors of each palette, Indeed, the gfx data in ROM
		is empty in the top 4 bits. Additionally even if there are
		$40 color codes, only $400 colors are addressable.
		A vh_init_palette is thus needed for sprites and layer 0.	*/

//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x6,	0x4400, 0x40 }, // [1] Layer 1
	{ -1 }
};


/***************************************************************************
								Power Instinct 2
***************************************************************************/

static struct GfxDecodeInfo pwrinst2_gfxdecodeinfo[] =
{
//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x1800, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x0800, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x1000, 0x40 }, // [2] Layer 2
	{ REGION_GFX5, 0, &layout_8x8x4,	0x2000, 0x40 }, // [3] Layer 3
	{ -1 }
};


/***************************************************************************
								Sailor Moon
***************************************************************************/

static struct GfxDecodeInfo sailormn_gfxdecodeinfo[] =
{
	/* 4 bit sprites ? */
//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4800, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x6_2,	0x4c00, 0x40 }, // [2] Layer 2
	{ -1 }
};


/***************************************************************************
								Uo Poko
***************************************************************************/

static struct GfxDecodeInfo uopoko_gfxdecodeinfo[] =
{
//    REGION_GFX1										// Sprites
	{ REGION_GFX2, 0, &layout_8x8x8,	0x4000, 0x40 }, // [0] Layer 0
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

MACHINE_INIT( cave )
{
	soundbuf.len = 0;
}

/* start with the watchdog armed */
MACHINE_INIT( cave_watchdog )
{
	machine_init_cave();
	watchdog_reset16_w(0,0,0);
}

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ sound_irq_gen }
};

/*	X1 = 32 MHz, OKI: / 165 mode A ; / 132 mode B */
static struct OKIM6295interface metmqstr_okim6295_intf =
{
	2,
	{ 32000000 / 16 / 132,	32000000 / 16 / 132	},
	{ REGION_SOUND1,		REGION_SOUND2		},
	{ 100,					100					}
};

static struct OKIM6295interface okim6295_intf_16kHz_16kHz =
{
	2,
	{ 16000, 16000 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 100 }
};

static struct OKIM6295interface okim6295_intf_8kHz_16kHz =
{
	2,
	{ 8000, 16000 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 100 }
};

static struct OKIM6295interface okim6295_intf_8kHz =
{
	1,
	{ 8000 },           /* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_intf_4MHz =
{
	1,
	16000000/4, /* ? */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ irqhandler }, /* irq handler */
	{ 0 } /* port_write */
};

static struct YM2203interface ym2203_intf_4MHz =
{
	1,
	4000000,	/* ? */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


/***************************************************************************
								Dangun Feveron
***************************************************************************/

static MACHINE_DRIVER_START( dfeveron )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(dfeveron_readmem,dfeveron_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(dfeveron_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_2_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************
								Dodonpachi
***************************************************************************/

static MACHINE_DRIVER_START( ddonpach )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(ddonpach_readmem,ddonpach_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(ddonpach_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH(0x8000 + 0x40*16)	// $400 extra entries for layers 1&2

	MDRV_PALETTE_INIT(ddonpach)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************
									Donpachi
***************************************************************************/

static MACHINE_DRIVER_START( donpachi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(donpachi_readmem,donpachi_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(donpachi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_8kHz_16kHz)
MACHINE_DRIVER_END


/***************************************************************************
								Esprade
***************************************************************************/

static MACHINE_DRIVER_START( esprade )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(esprade_readmem,esprade_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(esprade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************
									Guwange
***************************************************************************/

static MACHINE_DRIVER_START( guwange )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(guwange_readmem,guwange_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(esprade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************
								Hotdog Storm
***************************************************************************/

static MACHINE_DRIVER_START( hotdogst )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(hotdogst_readmem,hotdogst_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(hotdogst_sound_readmem,hotdogst_sound_writemem)
	MDRV_CPU_PORTS(hotdogst_sound_readport,hotdogst_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 240-1)
	MDRV_GFXDECODE(hotdogst_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_8kHz)
MACHINE_DRIVER_END


/***************************************************************************
								Mazinger Z
***************************************************************************/

static MACHINE_DRIVER_START( mazinger )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(mazinger_readmem,mazinger_writemem)
	MDRV_CPU_VBLANK_INT(mazinger_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	// Bidirectional communication
	MDRV_CPU_MEMORY(mazinger_sound_readmem,mazinger_sound_writemem)
	MDRV_CPU_PORTS(mazinger_sound_readport,mazinger_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave_watchdog)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 240-1)
	MDRV_GFXDECODE(mazinger_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(mazinger)
	MDRV_VIDEO_START(cave_2_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_8kHz)
MACHINE_DRIVER_END


/***************************************************************************
								Metamoqester
***************************************************************************/

static MACHINE_DRIVER_START( metmqstr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000 / 2)
	MDRV_CPU_MEMORY(metmqstr_readmem,metmqstr_writemem)
	MDRV_CPU_VBLANK_INT(metmqstr_interrupt,2)

	MDRV_CPU_ADD(Z80,32000000 / 4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(metmqstr_sound_readmem,metmqstr_sound_writemem)
	MDRV_CPU_PORTS(metmqstr_sound_readport,metmqstr_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave_watchdog)	/* start with the watchdog armed */
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 240)
	MDRV_VISIBLE_AREA(0x7d, 0x7d + 0x180-1, 0, 240-1)
	MDRV_GFXDECODE(donpachi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_intf_4MHz)	// 32/8 ?
	MDRV_SOUND_ADD(OKIM6295, metmqstr_okim6295_intf)
MACHINE_DRIVER_END


/***************************************************************************
								Power Instinct 2
***************************************************************************/

/*	X1 = 12 MHz, X2 = 28 MHz, X3 = 16 MHz. OKI: / 165 mode A ; / 132 mode B */

//	sound1 is wrong!! (it seems to require only ~1 ipf on the z80?)
static struct OKIM6295interface okim6295_intf_pwrinst2 =
{
	2,
	{ 16000000 / 8 / 132,		16000000 / 8 / 132	},
	{ REGION_SOUND1, 			REGION_SOUND2		},
	{ 0, 	/*<-wrong */		50					}
};

static struct YM2203interface ym2203_intf_pwrinst2 =
{
	1,
	16000000 / 4,	/* ? */
	{ YM2203_VOL(100,100) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static MACHINE_DRIVER_START( pwrinst2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(pwrinst2_readmem,pwrinst2_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80,16000000 / 2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(pwrinst2_sound_readmem,pwrinst2_sound_writemem)
	MDRV_CPU_PORTS(pwrinst2_sound_readport,pwrinst2_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 240)
	MDRV_VISIBLE_AREA(0x72, 0x72 + 0x140-1, 0, 240-1)
	MDRV_GFXDECODE(pwrinst2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x5000/2)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_VIDEO_START(cave_4_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_intf_pwrinst2)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_pwrinst2)
MACHINE_DRIVER_END


/***************************************************************************
						Sailor Moon / Air Gallet
***************************************************************************/

static MACHINE_DRIVER_START( sailormn )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(sailormn_readmem,sailormn_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000)
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	// Bidirectional Communication
	MDRV_CPU_MEMORY(sailormn_sound_readmem,sailormn_sound_writemem)
	MDRV_CPU_PORTS(sailormn_sound_readport,sailormn_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	// we use cpu_getvblank
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320+1, 240)
	MDRV_VISIBLE_AREA(0+1, 320+1-1, 0, 240-1)
	MDRV_GFXDECODE(sailormn_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(sailormn)	// 4 bit sprites, 6 bit tiles
	MDRV_VIDEO_START(sailormn_3_layers)	/* Layer 2 has 1 banked ROM */
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_16kHz_16kHz)
MACHINE_DRIVER_END


/***************************************************************************
								Uo Poko
***************************************************************************/

static MACHINE_DRIVER_START( uopoko )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(uopoko_readmem,uopoko_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(uopoko_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_1_layer)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/

/* 4 bits -> 8 bits. Even and odd pixels are swapped */
static void unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	const unsigned int len	=	memory_region_length(region);
	unsigned char *src		=	memory_region(region) + len / 2 - 1;
	unsigned char *dst		=	memory_region(region) + len - 1;

	while(dst > src)
	{
		unsigned char data = *src--;
		/* swap even and odd pixels */
		*dst-- = data >> 4;		*dst-- = data & 0xF;
	}
}


/* 4 bits -> 8 bits. Even and odd pixels and even and odd words, are swapped */
static void ddonpach_unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	const unsigned int len	=	memory_region_length(region);
	unsigned char *src		=	memory_region(region) + len / 2 - 1;
	unsigned char *dst		=	memory_region(region) + len - 1;

	while(dst > src)
	{
		unsigned char data1= *src--;
		unsigned char data2= *src--;
		unsigned char data3= *src--;
		unsigned char data4= *src--;

		/* swap even and odd pixels, and even and odd words */
		*dst-- = data2 & 0xF;		*dst-- = data2 >> 4;
		*dst-- = data1 & 0xF;		*dst-- = data1 >> 4;
		*dst-- = data4 & 0xF;		*dst-- = data4 >> 4;
		*dst-- = data3 & 0xF;		*dst-- = data3 >> 4;
	}
}


/* 2 pages of 4 bits -> 8 bits */
static void esprade_unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	unsigned char *src		=	memory_region(region);
	unsigned char *dst		=	memory_region(region) + memory_region_length(region);

	while(src < dst)
	{
		unsigned char data1 = src[0];
		unsigned char data2 = src[1];

		src[0] = ((data1 & 0x0f)<<4) + (data2 & 0x0f);
		src[1] = (data1 & 0xf0) + ((data2 & 0xf0)>>4);

		src += 2;
	}
}

/***************************************************************************

								Air Gallet

Banpresto
Runs on identical board to Sailor Moon (several sockets unpopulated)

PCB: BP945A (overstamped with BP962A)
CPU: TMP68HC000P16 (68000, 64 pin DIP)
SND: Z84C0008PEC (Z80, 40 pin DIP), OKI M6295 x 2, YM2151, YM3012
OSC: 28.000MHz, 16.000MHz
RAM: 62256 x 8, NEC 424260 x 2, 6264 x 5

Other Chips:
SGS Thomson ST93C46CB1 (EEPROM)
PALS (same as Sailor Moon, not dumped):
      18CV8 label SMBG
      18CV8 label SMZ80
      18CV8 label SMCPU
      GAL16V8 (located near BP962A.U47)

GFX:  038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button to access test mode.

ROMS:
BP962A.U9	27C040		Sound Program
BP962A.U45	27C240		Main Program
BP962A.U47	23C16000	Sound
BP962A.U48	23C16000	Sound
BP962A.U53	23C16000	GFX
BP962A.U54	23C16000	GFX
BP962A.U57	23C16000	GFX
BP962A.U65	23C16000	GFX
BP962A.U76	23C16000	GFX
BP962A.U77	23C16000	GFX

***************************************************************************/

ROM_START( agallet )	// Shows "Taiwan Only" on the copyright notice screen.
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bp962a.u45", 0x000000, 0x080000, 0x24815046 )
	//empty

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bp962a.u9",  0x00000, 0x08000, 0x06caddbe )	// 1xxxxxxxxxxxxxxxxxx = 0xFF
	ROM_CONTINUE(           0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bp962a.u76", 0x000000, 0x200000, 0x858da439 )
	ROM_LOAD( "bp962a.u77", 0x200000, 0x200000, 0xea2ba35e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp962a.u53", 0x000000, 0x100000, 0xfcd9a107 )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp962a.u54", 0x000000, 0x200000, 0x0cfa3409 )

	ROM_REGION( (1*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bp962a.u57", 0x000000, 0x200000, 0x6d608957 )
	/* 2 bit part */
	ROM_LOAD( "bp962a.u65", 0x200000, 0x100000, 0x135fcf9a )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x200000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp962a.u48", 0x040000, 0x200000, 0xae00a1ce )	// 16 x $20000, FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x240000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp962a.u47", 0x040000, 0x200000, 0x6d4e9737 )	// 16 x $20000, FIRST AND SECOND HALF IDENTICAL
ROM_END


/***************************************************************************

								Dangun Feveron

Board:	CV01
OSC:	28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( dfeveron )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "cv01-u34.bin", 0x000000, 0x080000, 0xbe87f19d )
	ROM_LOAD16_BYTE( "cv01-u33.bin", 0x000001, 0x080000, 0xe53a7db3 )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "cv01-u25.bin", 0x000000, 0x400000, 0xa6f6a95d )
	ROM_LOAD( "cv01-u26.bin", 0x400000, 0x400000, 0x32edb62a )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "cv01-u50.bin", 0x000000, 0x200000, 0x7a344417 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "cv01-u49.bin", 0x000000, 0x200000, 0xd21cdda7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "cv01-u19.bin", 0x000000, 0x400000, 0x5f5514da )
ROM_END


/***************************************************************************

								Dodonpachi (Japan)

PCB:	AT-C03 D2
CPU:	MC68000-16
Sound:	YMZ280B
OSC:	28.0000MHz
		16.0000MHz
		16.9MHz (16.9344MHz?)

***************************************************************************/

ROM_START( ddonpach )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u27.bin", 0x000000, 0x080000, 0x2432ff9b )
	ROM_LOAD16_BYTE( "u26.bin", 0x000001, 0x080000, 0x4f3a914a )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2, do not dispose */
	ROM_LOAD( "u50.bin", 0x000000, 0x200000, 0x14b260ec )
	ROM_LOAD( "u51.bin", 0x200000, 0x200000, 0xe7ba8cce )
	ROM_LOAD( "u52.bin", 0x400000, 0x200000, 0x02492ee0 )
	ROM_LOAD( "u53.bin", 0x600000, 0x200000, 0xcb4c10f0 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u60.bin", 0x000000, 0x200000, 0x903096a7 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u61.bin", 0x000000, 0x200000, 0xd89b7631 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u62.bin", 0x000000, 0x200000, 0x292bfb6b )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u6.bin", 0x000000, 0x200000, 0x9dfdafaf )
	ROM_LOAD( "u7.bin", 0x200000, 0x200000, 0x795b17d5 )
ROM_END


/***************************************************************************

								Donpachi

CPU:          TMP68HC000-16
VOICE:        M6295 x2
OSC:          28.000/16.000/4.220MHz
BOARD #:      AT-C01DP-2
CUSTOM:       ATLUS 8647-01 013
              038 9429WX727 x3
              NMK 112 (Sound)

---------------------------------------------------
 filenames          devices       kind
---------------------------------------------------
 PRG.U29            27C4096       68000 main prg.
 U58.BIN            27C020        gfx   data
 ATDP.U32           57C8200       M6295 data
 ATDP.U33           57C16200      M6295 data
 ATDP.U44           57C16200      gfx   data
 ATDP.U45           57C16200      gfx   data
 ATDP.U54           57C8200       gfx   data
 ATDP.U57           57C8200       gfx   data

***************************************************************************/

ROM_START( donpachi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "prg.u29",     0x00000, 0x80000, 0x6be14af6 )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "atdp.u44", 0x000000, 0x200000, 0x7189e953 )
	ROM_LOAD( "atdp.u45", 0x200000, 0x200000, 0x6984173f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atdp.u54", 0x000000, 0x100000, 0x6bda6b66 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atdp.u57", 0x000000, 0x100000, 0x0a0e72b9 )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u58.bin", 0x000000, 0x040000, 0x285379ff )

	ROM_REGION( 0x240000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u33", 0x040000, 0x200000, 0xd749de00 )

	ROM_REGION( 0x340000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u32", 0x040000, 0x100000, 0x0d89fcca )
	ROM_LOAD( "atdp.u33", 0x140000, 0x200000, 0xd749de00 )
ROM_END

ROM_START( donpachk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "prgk.u26",    0x00000, 0x80000, 0xbbaf4c8b )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "atdp.u44", 0x000000, 0x200000, 0x7189e953 )
	ROM_LOAD( "atdp.u45", 0x200000, 0x200000, 0x6984173f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atdp.u54", 0x000000, 0x100000, 0x6bda6b66 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atdp.u57", 0x000000, 0x100000, 0x0a0e72b9 )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u58.bin", 0x000000, 0x040000, 0x285379ff )

	ROM_REGION( 0x240000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u33", 0x040000, 0x200000, 0xd749de00 )

	ROM_REGION( 0x340000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u32", 0x040000, 0x100000, 0x0d89fcca )
	ROM_LOAD( "atdp.u33", 0x140000, 0x200000, 0xd749de00 )
ROM_END


/***************************************************************************

									Esprade

ATC04
OSC:	28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( esprade )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42_i.bin", 0x000000, 0x080000, 0x3b510a73 )
	ROM_LOAD16_BYTE( "u41_i.bin", 0x000001, 0x080000, 0x97c1b649 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, 0x2f2fe92c )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, 0x491a3da4 )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, 0x06563efe )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, 0x7bbe4cfc )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, 0xe7ca6936 )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, 0xf53bd94f )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, 0xe7abe7b4 )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, 0x51a0f391 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, 0x0b9b875c )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, 0xf54b1cab )
ROM_END

ROM_START( espradej )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42_ver2.bin", 0x000000, 0x080000, 0x75d03c42 )
	ROM_LOAD16_BYTE( "u41_ver2.bin", 0x000001, 0x080000, 0x734b3ef0 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, 0x2f2fe92c )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, 0x491a3da4 )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, 0x06563efe )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, 0x7bbe4cfc )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, 0xe7ca6936 )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, 0xf53bd94f )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, 0xe7abe7b4 )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, 0x51a0f391 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, 0x0b9b875c )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, 0xf54b1cab )
ROM_END

ROM_START( espradeo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42.bin", 0x000000, 0x080000, 0x0718c7e5 )
	ROM_LOAD16_BYTE( "u41.bin", 0x000001, 0x080000, 0xdef30539 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, 0x2f2fe92c )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, 0x491a3da4 )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, 0x06563efe )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, 0x7bbe4cfc )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, 0xe7ca6936 )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, 0xf53bd94f )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, 0xe7abe7b4 )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, 0x51a0f391 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, 0x0b9b875c )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, 0xf54b1cab )
ROM_END


/***************************************************************************

								Guwange (Japan)

PCB:	ATC05
CPU:	MC68000-16
Sound:	YMZ280B
OSC:	28.0000MHz
		16.0000MHz
		16.9MHz

***************************************************************************/

ROM_START( guwange )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gu-u0127.bin", 0x000000, 0x080000, 0xf86b5293 )
	ROM_LOAD16_BYTE( "gu-u0129.bin", 0x000001, 0x080000, 0x6c0e3b93 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u083.bin", 0x0000000, 0x800000, 0xadc4b9c4 )
	ROM_LOAD16_BYTE( "u082.bin", 0x0000001, 0x800000, 0x3d75876c )
	ROM_LOAD16_BYTE( "u086.bin", 0x1000000, 0x400000, 0x188e4f81 )
	ROM_RELOAD(                  0x1800000, 0x400000 )
	ROM_LOAD16_BYTE( "u085.bin", 0x1000001, 0x400000, 0xa7d5659e )
	ROM_RELOAD(                  0x1800001, 0x400000 )
//sprite bug fix?
//	ROM_FILL(                    0x1800000, 0x800000, 0xff )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u101.bin", 0x000000, 0x800000, 0x0369491f )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u10102.bin", 0x000000, 0x400000, 0xe28d6855 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u10103.bin", 0x000000, 0x400000, 0x0fe91b8e )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u0462.bin", 0x000000, 0x400000, 0xb3d75691 )
ROM_END


/***************************************************************************

								Hot Dog Storm
Marble 1996

6264 6264 MP7 6264 6264 MP6 6264 6264 MP5  32MHz
                                                6264
                                                6264
                                                  MP4
                                                  MP3
                                                       93C46
                                      68257
                                      68257 68000-12
                                             YM2203
                                          Z80
  MP8 MP9
  68257
  68257                         U19    MP1      6296

***************************************************************************/

ROM_START( hotdogst )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "mp3u29", 0x00000, 0x80000, 0x1f4e5479 )
	ROM_LOAD16_BYTE( "mp4u28", 0x00001, 0x80000, 0x6f1c3c4b )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "mp2u19", 0x00000, 0x08000, 0xff979ebe )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(       0x10000, 0x38000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "mp9u55", 0x000000, 0x200000, 0x258d49ec )
	ROM_LOAD( "mp8u54", 0x200000, 0x200000, 0xbdb4d7b8 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "mp7u56", 0x00000, 0x80000, 0x87c21c50 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "mp6u61", 0x00000, 0x80000, 0x4dafb288 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "mp5u64", 0x00000, 0x80000, 0x9b26458c )

	ROM_REGION( 0xc0000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "mp1u65", 0x40000, 0x80000, 0x4868be1b )	// 1xxxxxxxxxxxxxxxxxx = 0xFF, 4 x 0x20000
ROM_END


/***************************************************************************

								Mazinger Z

Banpresto 1994

U63               038               62256
                  9335EX706         62256
3664                            62256  62256
3664                                U924      32MHz
                                    U24
U60               038             68000
                  9335EX706
3664                                U21   YM2203  92E422
3664                                Z80
                                    3664
                  013
                  9341E7009
U56
U55

62256 62256      514260  514260     U64         M6295

***************************************************************************/

ROM_START( mazinger )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mzp-0.u24", 0x00000, 0x80000, 0x43a4279f )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )		/* 68000 code (mapped at d00000) */
	ROM_LOAD16_WORD_SWAP( "mzp-1.924", 0x00000, 0x80000, 0xdb40acba )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "mzs.u21", 0x00000, 0x08000, 0xc5b4f7ed )
	ROM_CONTINUE(        0x10000, 0x18000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, ROMREGION_ERASEFF )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "bp943a-2.u56", 0x000000, 0x200000, 0x97e13959 )
	ROM_LOAD( "bp943a-3.u55", 0x200000, 0x080000, 0x9c4957dd )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp943a-1.u60", 0x000000, 0x200000, 0x46327415 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp943a-0.u63", 0x000000, 0x200000, 0xc1fed98a )	// FIXED BITS (xxxxxxxx00000000)

	ROM_REGION( 0x0c0000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp943a-4.u64", 0x040000, 0x080000, 0x3fc7f29a )	// 4 x $20000
ROM_END


/***************************************************************************

								Metamoqester

[Ninja Master (World version)?]
(C) 1995 Banpresto

PCB: BP947A
CPU: MC68HC000P16 (68000, 64 pin DIP)
SND: Z0840008PSC (Z80, 40 pin DIP), AD-65 x 2 (= OKI M6295), YM2151, CY5002 (= YM3012)
OSC: 32.000 MHz
RAM: LGS GM76C88ALFW-15 x 9 (28 pin SOP), LGS GM71C4260AJ70 x 2 (40 pin SOJ)
     Hitachi HM62256LFP-12T x 2 (40 pin SOJ)

Other Chips:
AT93C46 (EEPROM)
PAL (not dumped, located near 68000): ATF16V8 x 1

GFX:  (Same GFX chips as "Sailor Moon")

      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button labelled SW1 to access test mode.

ROMS:
BP947A.U37	16M Mask	\ Oki Samples
BP947A.U42	16M Mask	/

BP947A.U46	16M Mask	\
BP947A.U47	16M Mask	|
BP947A.U48	16M Mask	|
BP947A.U49	16M Mask	| GFX
BP947A.U50	16M Mask	|
BP947A.U51	16M Mask	|
BP947A.U52	16M Mask	/

BP947A.U20	27C020		  Sound PRG

BP947A.U25	27C240		\
BP947A.U28	27C240		| Main PRG
BP947A.U29	27C240		/

***************************************************************************/

ROM_START( metmqstr )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bp947a.u25", 0x000000, 0x80000, 0x0a5c3442 )
	ROM_LOAD16_WORD_SWAP( "bp947a.u28", 0x100000, 0x80000, 0x8c55decf )
	ROM_LOAD16_WORD_SWAP( "bp947a.u29", 0x200000, 0x80000, 0xcf0f3f3b )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "bp947a.u20",  0x00000, 0x08000, 0xa4a36170 )
	ROM_CONTINUE(            0x10000, 0x38000             )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bp947a.u49", 0x000000, 0x200000, 0x09749531 )
	ROM_LOAD( "bp947a.u50", 0x200000, 0x200000, 0x19cea8b2 )
	ROM_LOAD( "bp947a.u51", 0x400000, 0x200000, 0xc19bed67 )
	ROM_LOAD( "bp947a.u52", 0x600000, 0x200000, 0x70c64875 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp947a.u48", 0x000000, 0x100000, 0x04ff6a3d )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp947a.u47", 0x000000, 0x100000, 0x0de42827 )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "bp947a.u46", 0x000000, 0x100000, 0x0f9c906e )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u42", 0x040000, 0x100000, 0x2ce8ff2a )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u37", 0x040000, 0x100000, 0xc3077c8f )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )
ROM_END


/***************************************************************************

							Power Instinct 2

©1994 Atlus
CPU: 68000, Z80
Sound: YM2203, AR17961 (x2)
Custom: NMK 112 (sound?), Atlus 8647-01  013, 038 (x4)
X1 = 12 MHz
X2 = 28 MHz
X3 = 16 MHz

***************************************************************************/

ROM_START( pwrinst2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "g02.u45", 0x000000, 0x80000, 0x7b33bc43 )
	ROM_LOAD16_BYTE( "g02.u44", 0x000001, 0x80000, 0x8f6f6637 )
	ROM_LOAD16_BYTE( "g02.u43", 0x100000, 0x80000, 0x178e3d24 )
	ROM_LOAD16_BYTE( "g02.u42", 0x100001, 0x80000, 0xa0b4ee99 )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "g02.u3a", 0x00000, 0x0c000, 0xebea5e1e )
	ROM_CONTINUE(        0x10000, 0x14000             )

	ROM_REGION( 0xe00000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "g02.u61", 0x000000, 0x200000, 0x91e30398 )
	ROM_LOAD( "g02.u62", 0x200000, 0x200000, 0xd9455dd7 )
	ROM_LOAD( "g02.u63", 0x400000, 0x200000, 0x4d20560b )
	ROM_LOAD( "g02.u64", 0x600000, 0x200000, 0xb17b9b6e )
	ROM_LOAD( "g02.u65", 0x800000, 0x200000, 0x08541878 )
	ROM_LOAD( "g02.u66", 0xa00000, 0x200000, 0xbecf2a36 )
	ROM_LOAD( "g02.u67", 0xc00000, 0x200000, 0x52fe2b8b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "g02.u89", 0x000000, 0x100000, 0x373e1f73 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "g02.u78", 0x000000, 0x200000, 0x1eca63d2 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "g02.u81", 0x000000, 0x100000, 0x8a3ff685 )

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "g02.82a", 0x000000, 0x080000, 0x4b3567d6 )

	ROM_REGION( 0x440000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u53", 0x040000, 0x200000, 0xc4bdd9e0 )
	ROM_LOAD( "g02.u54", 0x240000, 0x200000, 0x1357d50e )

	ROM_REGION( 0x440000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u55", 0x040000, 0x200000, 0x2d102898 )
	ROM_LOAD( "g02.u56", 0x240000, 0x200000, 0x9ff50dda )
ROM_END


/***************************************************************************

								Sailor Moon

(C) 1995 Banpresto
PCB: BP945A
CPU: TMP68HC000P16 (68000, 64 pin DIP)
SND: Z84C0008PEC (Z80, 40 pin DIP), OKI M6295 x 2, YM2151, YM3012
OSC: 28.000MHz, 16.000MHz
RAM: NEC 43256 x 8, NEC 424260 x 2, Sanyo LC3664 x 5

Other Chips:
SGS Thomson ST93C46CB1 (EEPROM?)
PALS (not dumped):
      18CV8 label SMBG
      18CV8 label SMZ80
      18CV8 label SMCPU
      GAL16V8 (located near BPSM-U47)

GFX:  038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button to access test mode.

ROMS:
BP945A.U9	27C040		Sound Program
BP945A.U45	27C240		Main Program
BPSM.U46	23C16000	Main Program?
BPSM.U47	23C4000		Sound?
BPSM.U48	23C16000	Sound?
BPSM.U53	23C16000	GFX
BPSM.U54	23C16000	GFX
BPSM.U57	23C16000	GFX
BPSM.U58	23C16000	GFX
BPSM.U59	23C16000	GFX
BPSM.U60	23C16000	GFX
BPSM.U61	23C16000	GFX
BPSM.U62	23C16000	GFX
BPSM.U63	23C16000	GFX
BPSM.U64	23C16000	GFX
BPSM.U65	23C16000	GFX
BPSM.U76	23C16000	GFX
BPSM.U77	23C16000	GFX

***************************************************************************/

ROM_START( sailormn )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bpsm945a.u45", 0x000000, 0x080000, 0x898c9515 )
	ROM_LOAD16_WORD_SWAP( "bpsm.u46",     0x200000, 0x200000, 0x32084e80 )

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bpsm945a.u9",  0x00000, 0x08000, 0x438de548 )
	ROM_CONTINUE(             0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bpsm.u76", 0x000000, 0x200000, 0xa243a5ba )
	ROM_LOAD( "bpsm.u77", 0x200000, 0x200000, 0x5179a4ac )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bpsm.u53", 0x000000, 0x200000, 0xb9b15f83 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bpsm.u54", 0x000000, 0x200000, 0x8f00679d )

	ROM_REGION( (5*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bpsm.u57", 0x000000, 0x200000, 0x86be7b63 )
	ROM_LOAD( "bpsm.u58", 0x200000, 0x200000, 0xe0bba83b )
	ROM_LOAD( "bpsm.u62", 0x400000, 0x200000, 0xa1e3bfac )
	ROM_LOAD( "bpsm.u61", 0x600000, 0x200000, 0x6a014b52 )
	ROM_LOAD( "bpsm.u60", 0x800000, 0x200000, 0x992468c0 )
	/* 2 bit part */
	ROM_LOAD( "bpsm.u65", 0xa00000, 0x200000, 0xf60fb7b5 )
	ROM_LOAD( "bpsm.u64", 0xc00000, 0x200000, 0x6559d31c )
	ROM_LOAD( "bpsm.u63", 0xe00000, 0x100000, 0xd57a56b4 )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(         0xe00000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u48", 0x040000, 0x200000, 0x498e4ed1 )	// 16 x $20000

	ROM_REGION( 0x240000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u47", 0x040000, 0x080000, 0x0f2901b9 )	// 4 x $20000
	ROM_RELOAD(           0x0c0000, 0x080000             )
	ROM_RELOAD(           0x140000, 0x080000             )
	ROM_RELOAD(           0x1c0000, 0x080000             )
ROM_END

ROM_START( sailormo )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smprg.u45",    0x000000, 0x080000, 0x234f1152 )
	ROM_LOAD16_WORD_SWAP( "bpsm.u46",     0x200000, 0x200000, 0x32084e80 )

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bpsm945a.u9",  0x00000, 0x08000, 0x438de548 )
	ROM_CONTINUE(             0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bpsm.u76", 0x000000, 0x200000, 0xa243a5ba )
	ROM_LOAD( "bpsm.u77", 0x200000, 0x200000, 0x5179a4ac )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bpsm.u53", 0x000000, 0x200000, 0xb9b15f83 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bpsm.u54", 0x000000, 0x200000, 0x8f00679d )

	ROM_REGION( (5*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bpsm.u57", 0x000000, 0x200000, 0x86be7b63 )
	ROM_LOAD( "bpsm.u58", 0x200000, 0x200000, 0xe0bba83b )
	ROM_LOAD( "bpsm.u62", 0x400000, 0x200000, 0xa1e3bfac )
	ROM_LOAD( "bpsm.u61", 0x600000, 0x200000, 0x6a014b52 )
	ROM_LOAD( "bpsm.u60", 0x800000, 0x200000, 0x992468c0 )
	/* 2 bit part */
	ROM_LOAD( "bpsm.u65", 0xa00000, 0x200000, 0xf60fb7b5 )
	ROM_LOAD( "bpsm.u64", 0xc00000, 0x200000, 0x6559d31c )
	ROM_LOAD( "bpsm.u63", 0xe00000, 0x100000, 0xd57a56b4 )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(         0xe00000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u48", 0x040000, 0x200000, 0x498e4ed1 )	// 16 x $20000

	ROM_REGION( 0x240000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u47", 0x040000, 0x080000, 0x0f2901b9 )	// 4 x $20000
	ROM_RELOAD(           0x0c0000, 0x080000             )
	ROM_RELOAD(           0x140000, 0x080000             )
	ROM_RELOAD(           0x1c0000, 0x080000             )
ROM_END


/***************************************************************************

									Uo Poko
Board: CV02
OSC:	28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( uopoko )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u26j.bin", 0x000000, 0x080000, 0xe7eec050 )
	ROM_LOAD16_BYTE( "u25j.bin", 0x000001, 0x080000, 0x68cb6211 )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "u33.bin", 0x000000, 0x400000, 0x5d142ad2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u49.bin", 0x000000, 0x400000, 0x12fb11bb )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u4.bin", 0x000000, 0x200000, 0xa2d0d755 )
ROM_END






/***************************************************************************


	Drivers Init Routines - Rom decryption/unpacking, global vars etc.


***************************************************************************/

/* Tiles are 6 bit, 4 bits stored in one rom, 2 bits in the other.
   Expand the 2 bit part into a 4 bit layout, so we can decode it */
void sailormn_unpack_tiles( const int region )
{
	unsigned char *src		=	memory_region(region) + (memory_region_length(region)/4)*3 - 1;
	unsigned char *dst		=	memory_region(region) + (memory_region_length(region)/4)*4 - 2;

	while(src <= dst)
	{
		unsigned char data = src[0];

		dst[0] = ((data & 0x03) << 4) + ((data & 0x0c) >> 2);
		dst[1] = ((data & 0x30) >> 0) + ((data & 0xc0) >> 6);

		src -= 1;
		dst -= 2;
	}
}

DRIVER_INIT( agallet )
{
	sailormn_unpack_tiles( REGION_GFX4 );

	cave_default_eeprom = cave_default_eeprom_type7;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type7);
	unpack_sprites();
	cave_spritetype = 0;	// "normal" sprites

//	Speed Hack
	install_mem_read16_handler(0, 0xb80000, 0xb80001, agallet_irq_cause_r);
}

DRIVER_INIT( dfeveron )
{
	cave_default_eeprom = cave_default_eeprom_type1;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type1);
	unpack_sprites();
	cave_spritetype = 0;	// "normal" sprites
}

DRIVER_INIT( ddonpach )
{
	cave_default_eeprom = cave_default_eeprom_type2;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type2);
	ddonpach_unpack_sprites();
	cave_spritetype = 1;	// "different" sprites (no zooming?)
}

DRIVER_INIT( esprade )
{
	cave_default_eeprom = cave_default_eeprom_type2;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type2);
	esprade_unpack_sprites();
	cave_spritetype = 0;	// "normal" sprites

#if 0		//ROM PATCH
	{
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0x118A/2] = 0x4e71;			//palette fix	118A: 5548				SUBQ.W	#2,A0		--> NOP
	}
#endif
}

DRIVER_INIT( guwange )
{
	cave_default_eeprom = cave_default_eeprom_type1;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type1);
	esprade_unpack_sprites();
	cave_spritetype = 0;	// "normal" sprites
}

DRIVER_INIT( hotdogst )
{
	cave_default_eeprom = cave_default_eeprom_type4;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type4);
	unpack_sprites();
	cave_spritetype = 2;	// "normal" sprites with different position handling
}

DRIVER_INIT( mazinger )
{
	unsigned char *buffer;
	data8_t *src = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);

	/* decrypt sprites */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i ^ 0xdf88] = src[BITSWAP24(i,23,22,21,20,19,9,7,3,15,4,17,14,18,2,16,5,11,8,6,13,1,10,12,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}

	cave_default_eeprom = cave_default_eeprom_type5;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type5);
	unpack_sprites();
	cave_spritetype = 2;	// "normal" sprites with different position handling

	/* setup extra ROM */
	cpu_setbank(1,memory_region(REGION_USER1));
}


DRIVER_INIT( metmqstr )
{
	cave_default_eeprom = 0;
	cave_default_eeprom_length = 0;
	unpack_sprites();
	cave_spritetype = 2;	// "normal" sprites with different position handling
}


DRIVER_INIT( pwrinst2 )
{
	cave_default_eeprom = 0;
	cave_default_eeprom_length = 0;

//	To do: Decrypt sprites

	unpack_sprites();
	cave_spritetype = 1;	// "different" sprites (no zooming?)
}

DRIVER_INIT( sailormn )
{
	unsigned char *buffer;
	data8_t *src = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);

	/* decrypt sprites */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i ^ 0x950c4] = src[BITSWAP24(i,23,22,21,20,15,10,12,6,11,1,13,3,16,17,2,5,14,7,18,8,4,19,9,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}

	sailormn_unpack_tiles( REGION_GFX4 );

	cave_default_eeprom = cave_default_eeprom_type6;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type6);
	unpack_sprites();
	cave_spritetype = 2;	// "normal" sprites with different position handling
}

DRIVER_INIT( uopoko )
{
	cave_default_eeprom = cave_default_eeprom_type3;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type4);
	unpack_sprites();
	cave_spritetype = 0;	// "normal" sprites
}


/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1994, mazinger, 0,        mazinger, cave,     mazinger, ROT90,  "Banpresto/Dynamic Pl. Toei Animation", "Mazinger Z"                 )
GAME( 1995, donpachi, 0,        donpachi, cave,     ddonpach, ROT270, "Atlus/Cave",                           "DonPachi (Japan)"           )
GAME( 1995, donpachk, donpachi, donpachi, cave,     ddonpach, ROT270, "Atlus/Cave",                           "DonPachi (Korea)"           )
GAME( 1995, metmqstr, 0,        metmqstr, metmqstr, metmqstr, ROT0,   "Banpresto/Pandorabox",                 "Metamoqester"               )
GAME( 1995, sailormn, 0,        sailormn, cave,     sailormn, ROT0,   "Banpresto",                            "Pretty Soldier Sailor Moon (95/03/22B)" )
GAME( 1995, sailormo, sailormn, sailormn, cave,     sailormn, ROT0,   "Banpresto",                            "Pretty Soldier Sailor Moon (95/03/22)" )
GAME( 1996, agallet,  0,        sailormn, cave,     agallet,  ROT270, "Banpresto / Gazelle",                  "Air Gallet (Taiwan)"        )
GAME( 1996, hotdogst, 0,        hotdogst, cave,     hotdogst, ROT90,  "Marble",                               "Hotdog Storm"               )
GAME( 1997, ddonpach, 0,        ddonpach, cave,     ddonpach, ROT270, "Atlus/Cave",                           "DoDonPachi (Japan)"         )
GAME( 1998, dfeveron, 0,        dfeveron, cave,     dfeveron, ROT270, "Cave (Nihon System license)",          "Dangun Feveron (Japan)"     )
GAME( 1998, esprade,  0,        esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (International Ver 1998 4/22)" )
GAME( 1998, espradej, esprade,  esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (Japan Ver 1998 4/21)" )
GAME( 1998, espradeo, esprade,  esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (Japan Ver 1998 4/14)" )
GAME( 1998, uopoko,   0,        uopoko,   cave,     uopoko,   ROT0,   "Cave (Jaleco license)",                "Uo Poko (Japan)"            )
GAME( 1999, guwange,  0,        guwange,  guwange,  guwange,  ROT270, "Atlus/Cave",                           "Guwange (Japan)"            )

/* Games not working properly: */
GAMEX(1994, pwrinst2, 0,        pwrinst2, metmqstr, pwrinst2, ROT0,   "Atlus/Cave",                           "Power Instinct 2 (USA)", GAME_NOT_WORKING )
