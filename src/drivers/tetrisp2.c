/***************************************************************************

							  -= Tetris Plus 2 =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  TMP68HC000P-12

Video Chips  :	SS91022-03 9428XX001
				GS91022-04 9721PD008
				SS91022-05 9347EX002
				GS91022-05 048 9726HX002

Sound Chips  :	Yamaha YMZ280B-F

Other        :  XILINX XC5210 PQ240C X68710M AKJ9544
				XC7336 PC44ACK9633 A63458A
				NVRAM


To Do:

-	There is a 3rd unimplemented layer capable of rotation (not used by
	the game, can be tested in service mode).
-	Priority RAM is not taken into account.

Notes:

-	The Japan set doesn't seem to have (or use) NVRAM. I can't enter
	a test mode or use the service coin either !?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


UINT16 tetrisp2_systemregs[0x10];
UINT16 rocknms_sub_systemregs[0x10];

UINT16 rockn_protectdata;
UINT16 rockn_adpcmbank;
UINT16 rockn_soundvolume;
UINT32 rockn_timer_ctr, rockn_timer_sub_ctr;

/* Variables defined in vidhrdw: */

extern data16_t *tetrisp2_vram_bg, *tetrisp2_scroll_bg;
extern data16_t *tetrisp2_vram_fg, *tetrisp2_scroll_fg;
extern data16_t *tetrisp2_vram_rot, *tetrisp2_rotregs;

extern data16_t *tetrisp2_priority;

extern data16_t *rocknms_sub_vram_bg, *rocknms_sub_scroll_bg;
extern data16_t *rocknms_sub_vram_fg, *rocknms_sub_scroll_fg;
extern data16_t *rocknms_sub_vram_rot, *rocknms_sub_rotregs;

extern data16_t *rocknms_sub_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( tetrisp2_palette_w );
WRITE16_HANDLER( rocknms_sub_palette_w );
READ16_HANDLER( tetrisp2_priority_r );
WRITE16_HANDLER( tetrisp2_priority_w );
WRITE16_HANDLER( rockn_priority_w );
READ16_HANDLER( rocknms_sub_priority_r );
WRITE16_HANDLER( rocknms_sub_priority_w );

WRITE16_HANDLER( tetrisp2_vram_bg_w );
WRITE16_HANDLER( tetrisp2_vram_fg_w );
WRITE16_HANDLER( tetrisp2_vram_rot_w );

WRITE16_HANDLER( rocknms_sub_vram_bg_w );
WRITE16_HANDLER( rocknms_sub_vram_fg_w );
WRITE16_HANDLER( rocknms_sub_vram_rot_w );

VIDEO_START( tetrisp2 );
VIDEO_UPDATE( tetrisp2 );
VIDEO_START( rockntread );
VIDEO_UPDATE( rockntread );
VIDEO_START( rocknms );
VIDEO_UPDATE( rocknms );


/***************************************************************************


									Sound


***************************************************************************/

static WRITE16_HANDLER( tetrisp2_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		tetrisp2_systemregs[offset] = data;
	}
}

static WRITE16_HANDLER( rocknms_sub_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		rocknms_sub_systemregs[offset] = data;
	}
}

/***************************************************************************


									Sound


***************************************************************************/

static READ16_HANDLER( tetrisp2_sound_r )
{
	return YMZ280B_status_0_r(offset);
}

static WRITE16_HANDLER( tetrisp2_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

static READ16_HANDLER( rockn_adpcmbank_r )
{
	return ((rockn_adpcmbank & 0xf0ff) | (rockn_protectdata << 8));
}

static WRITE16_HANDLER( rockn_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	rockn_adpcmbank = data;
	bank = ((data & 0x001f) >> 2);

	if (bank > 7)
	{
		usrintf_showmessage("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0c00000 * bank)], 0x0c00000);
}


static WRITE16_HANDLER( rockn2_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	char banktable[9][3]=
	{
		{  0,  1,  2 },		// bank $00
		{  3,  4,  5 },		// bank $04
		{  6,  7,  8 },		// bank $08
		{  9, 10, 11 },		// bank $0c
		{ 12, 13, 14 },		// bank $10
		{ 15, 16, 17 },		// bank $14
		{ 18, 19, 20 },		// bank $18
		{  0,  0,  0 },		// bank $1c
		{  0,  5, 14 },		// bank $20
	};

	rockn_adpcmbank = data;
	bank = ((data & 0x003f) >> 2);

	if (bank > 8)
	{
		usrintf_showmessage("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][0] )], 0x0400000);
	memcpy(&SNDROM[0x0800000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][1] )], 0x0400000);
	memcpy(&SNDROM[0x0c00000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][2] )], 0x0400000);
}

static READ16_HANDLER( rockn_soundvolume_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( rockn_soundvolume_w )
{
	rockn_soundvolume = data;
}

/***************************************************************************


								Protection


***************************************************************************/

static READ16_HANDLER( tetrisp2_ip_1_word_r )
{
	return	( readinputport(1) &  0xfcff ) |
			(           rand() & ~0xfcff ) |
			(      1 << (8 + (rand()&1)) );
}


/***************************************************************************


									NVRAM


***************************************************************************/

static data16_t *tetrisp2_nvram;
static size_t tetrisp2_nvram_size;

NVRAM_HANDLER( tetrisp2 )
{
	if (read_or_write)
		mame_fwrite(file,tetrisp2_nvram,tetrisp2_nvram_size);
	else
	{
		if (file)
			mame_fread(file,tetrisp2_nvram,tetrisp2_nvram_size);
		else
		{
			/* fill in the default values */
			memset(tetrisp2_nvram,0,tetrisp2_nvram_size);
		}
	}
}


/* The game only ever writes even bytes and reads odd bytes */
READ16_HANDLER( tetrisp2_nvram_r )
{
	return	( (tetrisp2_nvram[offset] >> 8) & 0x00ff ) |
			( (tetrisp2_nvram[offset] << 8) & 0xff00 ) ;
}

READ16_HANDLER( rockn_nvram_r )
{
	return	tetrisp2_nvram[offset];
}

WRITE16_HANDLER( tetrisp2_nvram_w )
{
	COMBINE_DATA(&tetrisp2_nvram[offset]);
}



/***************************************************************************





***************************************************************************/
static UINT16 rocknms_main2sub;
static UINT16 rocknms_sub2main;

READ16_HANDLER( rocknms_main2sub_r )
{
	return rocknms_main2sub;
}

WRITE16_HANDLER( rocknms_main2sub_w )
{
	if (ACCESSING_LSB)
		rocknms_main2sub = (data ^ 0xffff);
}

READ16_HANDLER( rocknms_port_0_r )
{
	return ((readinputport(0) & 0xfffc ) | (rocknms_sub2main & 0x0003));
}

WRITE16_HANDLER( rocknms_sub2main_w )
{
	if (ACCESSING_LSB)
		rocknms_sub2main = (data ^ 0xffff);
}


WRITE16_HANDLER( tetrisp2_coincounter_w )
{
	coin_counter_w( 0, (data & 0x0001));
}


/***************************************************************************


								Memory Map


***************************************************************************/

static MEMORY_READ16_START( tetrisp2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x400000, 0x403fff, MRA16_RAM				},	// Foreground
	{ 0x404000, 0x407fff, MRA16_RAM				},	// Background
	{ 0x408000, 0x409fff, MRA16_RAM				},	// ???
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x650000, 0x651fff, MRA16_RAM				},	// Rotation (mirror)
	{ 0x800002, 0x800003, tetrisp2_sound_r		},	// Sound
	{ 0x900000, 0x903fff, tetrisp2_nvram_r	    },	// NVRAM
	{ 0x904000, 0x907fff, tetrisp2_nvram_r	    },	// NVRAM (mirror)
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, tetrisp2_ip_1_word_r	},	// Inputs & protection
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( tetrisp2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_w, &tetrisp2_priority	},	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x400000, 0x403fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x404000, 0x407fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x408000, 0x409fff, MWA16_RAM									},	// ???
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x650000, 0x651fff, tetrisp2_vram_rot_w						},	// Rotation (mirror)
	{ 0x800000, 0x800003, tetrisp2_sound_w							},	// Sound
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0x904000, 0x907fff, tetrisp2_nvram_w							},	// NVRAM (mirror)
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, tetrisp2_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rockn1_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x400000, 0x403fff, MRA16_RAM				},	// Foreground
	{ 0x404000, 0x407fff, MRA16_RAM				},	// Background
	{ 0x408000, 0x409fff, MRA16_RAM				},	// ???
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x900000, 0x903fff, rockn_nvram_r		    },	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rockn1_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x400000, 0x403fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x404000, 0x407fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x408000, 0x409fff, MWA16_RAM									},	// ???
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_w							},	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, tetrisp2_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rockn2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM				},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM				},	// Background
	{ 0x808000, 0x809fff, MRA16_RAM				},	// ???
	{ 0x900000, 0x903fff, rockn_nvram_r		    },	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rockn2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x804000, 0x807fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x808000, 0x809fff, MWA16_RAM									},	// ???
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn2_adpcmbank_w					    },	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, tetrisp2_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rocknms_main_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, MRA16_RAM				},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
//	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM				},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM				},	// Background
//	{ 0x808000, 0x809fff, MRA16_RAM				},	// ???
	{ 0x900000, 0x903fff, rockn_nvram_r	    	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, rocknms_port_0_r	    },	// Inputs & MAIN <- SUB Communication
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rocknms_main_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
//	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x804000, 0x807fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
//	{ 0x808000, 0x809fff, MWA16_RAM									},	// ???
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_w							},	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xa00000, 0xa00001, rocknms_main2sub_w	                    },	// MAIN -> SUB Communication
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, tetrisp2_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rocknms_sub_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM					},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM					},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM					},	// Work RAM
	{ 0x200000, 0x23ffff, rocknms_sub_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM					},	// Palette
//	{ 0x500000, 0x50ffff, MRA16_RAM					},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM					},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM					},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM					},	// Background
//	{ 0x808000, 0x809fff, MRA16_RAM					},	// ???
	{ 0x900000, 0x907fff, MRA16_RAM					},	// NVRAM
//	{ 0xbe0000, 0xbe0001, MRA16_NOP					},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, rocknms_main2sub_r		},	// MAIN -> SUB Communication
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r		},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rocknms_sub_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM										},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16_2, &spriteram_2_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM										},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM										},	// Work RAM
	{ 0x200000, 0x23ffff, rocknms_sub_priority_w, &rocknms_sub_priority	},	// Priority
	{ 0x300000, 0x31ffff, rocknms_sub_palette_w, &paletteram16_2		},	// Palette
//	{ 0x500000, 0x50ffff, MWA16_RAM										},	// Line
	{ 0x600000, 0x60ffff, rocknms_sub_vram_rot_w, &rocknms_sub_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, rocknms_sub_vram_fg_w, &rocknms_sub_vram_fg	},	// Foreground
	{ 0x804000, 0x807fff, rocknms_sub_vram_bg_w, &rocknms_sub_vram_bg	},	// Background
//	{ 0x808000, 0x809fff, MWA16_RAM										},	// ???
	{ 0x900000, 0x907fff, MWA16_RAM										},	// NVRAM
	{ 0xb00000, 0xb00001, rocknms_sub2main_w							},	// MAIN <- SUB Communication
	{ 0xb20000, 0xb20001, MWA16_NOP										},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &rocknms_sub_scroll_fg				},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &rocknms_sub_scroll_bg				},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP										},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &rocknms_sub_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, rocknms_sub_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP										},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP										},	// Lev 2 irq ack
	{ 0xbe0002, 0xbe0003, rocknms_sub2main_w							},	// MAIN <- SUB Communication (mirror)
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
							Tetris Plus 2 (World)
***************************************************************************/

INPUT_PORTS_START( tetrisp2 )

	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
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
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPNAME( 0x0800, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0800, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPNAME( 0x1000, 0x1000, "F.B.I Logo" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
							Tetris Plus 2 (Japan)
***************************************************************************/


INPUT_PORTS_START( teplus2j )

	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused button

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused button

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

/*
	The code for checking the "service mode" and "free play" DSWs
	is (deliberately?) bugged in this set
*/
	PORT_START	// IN2 - $be0008.w
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
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNUSED  ) // Language dip
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )	// F.B.I. Logo (in the USA set?)
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
							Rock'n Tread (Japan)
***************************************************************************/


INPUT_PORTS_START( rockn1 )
	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
	PORT_BITX(    0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BITX(    0x0100, 0x0100, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0200, 0x0200, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0400, 0x0400, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x2000, 0x2000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( rocknms )
	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
	PORT_BITX(    0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BITX(    0x0100, 0x0100, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0200, 0x0200, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0400, 0x0400, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x2000, 0x2000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 8x8x8 tiles */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

/* 16x16x8 tiles */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};

static struct GfxDecodeInfo tetrisp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ -1 }
};

static struct GfxDecodeInfo rocknms_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ REGION_GFX5, 0, &layout_8x8x8,   0x8000, 0x10 }, // [0] Sprites
	{ REGION_GFX6, 0, &layout_16x16x8, 0x9000, 0x10 }, // [1] Background
	{ REGION_GFX7, 0, &layout_16x16x8, 0xa000, 0x10 }, // [2] Rotation
	{ REGION_GFX8, 0, &layout_8x8x8,   0xe000, 0x10 }, // [3] Foreground
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }	// irq
};

void rockn_timer_level4_callback(int param)
{
	static UINT16 rockn_timer_old = 0;
	UINT16 rockn_timer;
	int timer;

	rockn_timer = tetrisp2_systemregs[0x0c];

	if (rockn_timer != rockn_timer_old)
	{
		rockn_timer_old = rockn_timer;
		rockn_timer_ctr = 0;
	}

	rockn_timer_ctr++;

	timer = (200 + (4094 - rockn_timer) * 100);

	if (!(rockn_timer_ctr % timer)) cpu_set_irq_line(0, 4, HOLD_LINE);
}

void rockn_timer_sub_level4_callback(int param)
{
	static UINT16 rockn_timer_sub_old = 0;
	UINT16 rockn_timer_sub;
	int timer;

	rockn_timer_sub = rocknms_sub_systemregs[0x0c];

	if (rockn_timer_sub != rockn_timer_sub_old)
	{
		rockn_timer_sub_old = rockn_timer_sub;
		rockn_timer_sub_ctr = 0;
	}

	rockn_timer_sub_ctr++;

	timer = (200 + (4094 - rockn_timer_sub) * 100);

	if (!(rockn_timer_sub_ctr % timer)) cpu_set_irq_line(1, 4, HOLD_LINE);
}

void rockn_timer_level1_callback(int param)
{
	cpu_set_irq_line(0, 1, HOLD_LINE);
}

void rockn_timer_sub_level1_callback(int param)
{
	cpu_set_irq_line(1, 1, HOLD_LINE);
}

DRIVER_INIT( rockn_timer )
{
	rockn_timer_ctr = 0;
	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_level1_callback);
	timer_pulse(TIME_IN_USEC(5), 0, rockn_timer_level4_callback);
}

DRIVER_INIT( rockn1 )
{
	init_rockn_timer();
	rockn_protectdata = 1;
}

DRIVER_INIT( rockn2 )
{
	init_rockn_timer();
	rockn_protectdata = 2;
}

DRIVER_INIT( rocknms )
{
	init_rockn_timer();

	rockn_timer_sub_ctr = 0;
	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_sub_level1_callback);
	timer_pulse(TIME_IN_USEC(5), 0, rockn_timer_sub_level4_callback);

	rockn_protectdata = 3;
}

DRIVER_INIT( rockn3 )
{
	init_rockn_timer();
	rockn_protectdata = 4;
}

static MACHINE_DRIVER_START( tetrisp2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(tetrisp2_readmem,tetrisp2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(tetrisp2)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rockn1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rockn1_readmem,rockn1_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rockn2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rockn2_readmem,rockn2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rocknms )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rocknms_main_readmem,rocknms_main_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rocknms_sub_readmem,rocknms_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8, 3)
	MDRV_SCREEN_SIZE(0x140, 0xe0*2)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0*2-1)
	MDRV_GFXDECODE(rocknms_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(rocknms)
	MDRV_VIDEO_UPDATE(rocknms)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

								Tetris Plus 2

(C) Jaleco 1996

TP-97222
96019 EB-00-20117-0
MDK332V-0

BRIEF HARDWARE OVERVIEW

Toshiba TMP68HC000P-12
Yamaha YMZ280B-F
OSC: 12.000MHz, 48.000MHz, 16.9344MHz

Listing of custom chips. (Some on scan are hard to read).

IC38	JALECO SS91022-03 9428XX001
IC31	JALECO SS91022-05 9347EX002
IC32	JALECO GS91022-05    048  9726HX002
IC30	JALECO GS91022-04 9721PD008
IC39	XILINX XC5210 PQ240C X68710M AKJ9544
IC49	XILINX XC7336 PC44ACK9633 A63458A

***************************************************************************/

ROM_START( tetrisp2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "t2p_04.rom", 0x000000, 0x080000, 0xe67f9c51 )
	ROM_LOAD16_BYTE( "t2p_01.rom", 0x000001, 0x080000, 0x5020a4ed )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, 0x06f7dc64 )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, 0x3e613bed )
	/* If t2p_m01&2 from this board were correctly read, since they
	   hold the same data of the above but with swapped halves, it
	   means they had to invert the top bit of the "page select"
	   register in the sprite's hardware on this board! */

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, 0x16f7093c )
	ROM_LOAD( "96019-04.6",  0x400000, 0x100000, 0xb849dec9 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, 0xb849dec9 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, 0x34dd1bad )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, 0xa8a61954 )

ROM_END


/***************************************************************************

							Tetris Plus 2 (Japan)

(c)1997 Jaleco / The Tetris Company

TP-97222
96019 EB-00-20117-0

CPU:	68000-12
Sound:	YMZ280B-F
OSC:	12.000MHz
		48.0000MHz
		16.9344MHz

Custom:	SS91022-03
		GS91022-04
		GS91022-05
		SS91022-05

***************************************************************************/

ROM_START( teplus2j )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tet2-4v2.2", 0x000000, 0x080000, 0x5bfa32c8 )	// v2.2
	ROM_LOAD16_BYTE( "tet2-1v2.2", 0x000001, 0x080000, 0x919116d0 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, 0x06f7dc64 )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, 0x3e613bed )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, 0x16f7093c )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, 0xb849dec9 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, 0x34dd1bad )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, 0xa8a61954 )

ROM_END


/***************************************************************************

							Rock'n Tread 1 (Japan)
							Rock'n Tread 2 (Japan)
							Rock'n MegaSession (Japan)
							Rock'n 3 (Japan)
							Rock'n 4 (Japan)

(c)1997 Jaleco

CPU:	68000-12
Sound:	YMZ280B-F
OSC:	12.000MHz
		48.0000MHz
		16.9344MHz

Custom:	SS91022-03
		GS91022-04
		GS91022-05
		SS91022-05

***************************************************************************/

ROM_START( rockn1 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BADCRC(0xad90f2a3) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BADCRC(0xeef37ad6) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BADCRC(0x261b99a0) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x100000, BADCRC(0x5551717f) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x080000, BADCRC(0x918663a8) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ajrt00ma.shp", 0x0000000, 0x0400000, BADCRC(0xc354f753) ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) // BANK AREA
	ROM_LOAD( "ajrt01ma.shp", 0x1000000, 0x0400000, BADCRC(0x5b42999e) ) // bank 0
	ROM_LOAD( "ajrt02ma.shp", 0x1400000, 0x0400000, BADCRC(0x8306f302) ) // bank 0
	ROM_LOAD( "ajrt03ma.shp", 0x1800000, 0x0400000, BADCRC(0x3fda842c) ) // bank 0
	ROM_LOAD( "ajrt04ma.shp", 0x1c00000, 0x0400000, BADCRC(0x86d4f289) ) // bank 1
	ROM_LOAD( "ajrt05ma.shp", 0x2000000, 0x0400000, BADCRC(0xf8dbf47d) ) // bank 1
	ROM_LOAD( "ajrt06ma.shp", 0x2400000, 0x0400000, BADCRC(0x525aff97) ) // bank 1
	ROM_LOAD( "ajrt07ma.shp", 0x2800000, 0x0400000, BADCRC(0x5bd8bb95) ) // bank 2
	ROM_LOAD( "ajrt08ma.shp", 0x2c00000, 0x0400000, BADCRC(0x304c1643) ) // bank 2
	ROM_LOAD( "ajrt09ma.shp", 0x3000000, 0x0400000, BADCRC(0x78c22c56) ) // bank 2
	ROM_LOAD( "ajrt10ma.shp", 0x3400000, 0x0400000, BADCRC(0xd5e8d8a5) ) // bank 3
	ROM_LOAD( "ajrt13ma.shp", 0x3800000, 0x0400000, BADCRC(0x569ef4dd) ) // bank 3
	ROM_LOAD( "ajrt14ma.shp", 0x3c00000, 0x0400000, BADCRC(0xaae8d59c) ) // bank 3
	ROM_LOAD( "ajrt15ma.shp", 0x4000000, 0x0400000, BADCRC(0x9ec1459b) ) // bank 4
	ROM_LOAD( "ajrt16ma.shp", 0x4400000, 0x0400000, BADCRC(0xb26f9a81) ) // bank 4

ROM_END


ROM_START( rockn2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BADCRC(0x369750fd) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x3e0000, BADCRC(0x13de1054) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BADCRC(0x9f35b2cf) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x200000, BADCRC(0x4d9989ff) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x080000, BADCRC(0x6e724e73) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd25.bin", 0x0000000, 0x0400000, BADCRC(0x4e9611a3) ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "snd01.bin", 0x1000000, 0x0400000, BADCRC(0xec600f13) ) // bank 0
	ROM_LOAD( "snd02.bin", 0x1400000, 0x0400000, BADCRC(0x8306f302) ) // bank 0
	ROM_LOAD( "snd03.bin", 0x1800000, 0x0400000, BADCRC(0x3fda842c) ) // bank 0
	ROM_LOAD( "snd04.bin", 0x1c00000, 0x0400000, BADCRC(0x86d4f289) ) // bank 1
	ROM_LOAD( "snd05.bin", 0x2000000, 0x0400000, BADCRC(0xf8dbf47d) ) // bank 1
	ROM_LOAD( "snd06.bin", 0x2400000, 0x0400000, BADCRC(0x06f7bd63) ) // bank 1
	ROM_LOAD( "snd07.bin", 0x2800000, 0x0400000, BADCRC(0x22f042f6) ) // bank 2
	ROM_LOAD( "snd08.bin", 0x2c00000, 0x0400000, BADCRC(0xdd294d8e) ) // bank 2
	ROM_LOAD( "snd09.bin", 0x3000000, 0x0400000, BADCRC(0x8fedee6e) ) // bank 2
	ROM_LOAD( "snd10.bin", 0x3400000, 0x0400000, BADCRC(0x01292f11) ) // bank 3
	ROM_LOAD( "snd11.bin", 0x3800000, 0x0400000, BADCRC(0x20dc76ba) ) // bank 3
	ROM_LOAD( "snd12.bin", 0x3c00000, 0x0400000, BADCRC(0x11fff0bc) ) // bank 3
	ROM_LOAD( "snd13.bin", 0x4000000, 0x0400000, BADCRC(0x2367dd18) ) // bank 4
	ROM_LOAD( "snd14.bin", 0x4400000, 0x0400000, BADCRC(0x75ced8c0) ) // bank 4
	ROM_LOAD( "snd15.bin", 0x4800000, 0x0400000, BADCRC(0xaeaca380) ) // bank 4
	ROM_LOAD( "snd16.bin", 0x4c00000, 0x0400000, BADCRC(0x21d50e32) ) // bank 5
	ROM_LOAD( "snd17.bin", 0x5000000, 0x0400000, BADCRC(0xde785a2a) ) // bank 5
	ROM_LOAD( "snd18.bin", 0x5400000, 0x0400000, BADCRC(0x18cabb1e) ) // bank 5
	ROM_LOAD( "snd19.bin", 0x5800000, 0x0400000, BADCRC(0x33c89e53) ) // bank 6
	ROM_LOAD( "snd20.bin", 0x5c00000, 0x0400000, BADCRC(0x89c1b088) ) // bank 6
	ROM_LOAD( "snd21.bin", 0x6000000, 0x0400000, BADCRC(0x13db74bd) ) // bank 6

ROM_END


ROM_START( rockn3 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BADCRC(0x31895ef5) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BADCRC(0x3fa0a3fa) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BADCRC(0xe01bf471) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x200000, BADCRC(0x4e146de5) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x080000, BADCRC(0x8100039e) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rt3rd001.bin", 0x0000000, 0x0400000, BADCRC(0xe2f69042) ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) 		 // BANK AREA
	ROM_LOAD( "rt3rd002.bin", 0x1000000, 0x0400000, BADCRC(0xb328b18f) ) // bank 0
	ROM_LOAD( "rt3rd003.bin", 0x1400000, 0x0400000, BADCRC(0xf46438e3) ) // bank 0
	ROM_LOAD( "rt3rd004.bin", 0x1800000, 0x0400000, BADCRC(0xb979e887) ) // bank 0
	ROM_LOAD( "rt3rd005.bin", 0x1c00000, 0x0400000, BADCRC(0x0bb2c212) ) // bank 1
	ROM_LOAD( "rt3rd006.bin", 0x2000000, 0x0400000, BADCRC(0x3116e437) ) // bank 1
	ROM_LOAD( "rt3rd007.bin", 0x2400000, 0x0400000, BADCRC(0x26b37ef6) ) // bank 1
	ROM_LOAD( "rt3rd008.bin", 0x2800000, 0x0400000, BADCRC(0x1dd3f4e3) ) // bank 2
	ROM_LOAD( "rt3rd009.bin", 0x2c00000, 0x0400000, BADCRC(0xa1b03d67) ) // bank 2
	ROM_LOAD( "rt3rd010.bin", 0x3000000, 0x0400000, BADCRC(0x35107aac) ) // bank 2
	ROM_LOAD( "rt3rd011.bin", 0x3400000, 0x0400000, BADCRC(0x059ec592) ) // bank 3
	ROM_LOAD( "rt3rd012.bin", 0x3800000, 0x0400000, BADCRC(0x84d4badb) ) // bank 3
	ROM_LOAD( "rt3rd013.bin", 0x3c00000, 0x0400000, BADCRC(0x4527a9b7) ) // bank 3
	ROM_LOAD( "rt3rd014.bin", 0x4000000, 0x0400000, BADCRC(0xbfa4b7ce) ) // bank 4
	ROM_LOAD( "rt3rd015.bin", 0x4400000, 0x0400000, BADCRC(0xa2ccd2ce) ) // bank 4
	ROM_LOAD( "rt3rd016.bin", 0x4800000, 0x0400000, BADCRC(0x95baf678) ) // bank 4
	ROM_LOAD( "rt3rd017.bin", 0x4c00000, 0x0400000, BADCRC(0x5883c84b) ) // bank 5
	ROM_LOAD( "rt3rd018.bin", 0x5000000, 0x0400000, BADCRC(0xf92098ce) ) // bank 5
	ROM_LOAD( "rt3rd019.bin", 0x5400000, 0x0400000, BADCRC(0xdbb2c228) ) // bank 5
	ROM_LOAD( "rt3rd020.bin", 0x5800000, 0x0400000, BADCRC(0x9efdae1c) ) // bank 6
	ROM_LOAD( "rt3rd021.bin", 0x5c00000, 0x0400000, BADCRC(0x5f301b83) ) // bank 6

ROM_END


ROM_START( rockn4 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BADCRC(0xf2c9ff1c) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BADCRC(0x5ca2228e) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BADCRC(0xead41e79) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x200000, BADCRC(0xeb16fc67) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x100000, BADCRC(0x37d50259) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd25.bin", 0x0000000, 0x0400000, BADCRC(0x918ea8eb) ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "snd01.bin", 0x1000000, 0x0400000, BADCRC(0xc548e51e) ) // bank 0
	ROM_LOAD( "snd02.bin", 0x1400000, 0x0400000, BADCRC(0xffda0253) ) // bank 0
	ROM_LOAD( "snd03.bin", 0x1800000, 0x0400000, BADCRC(0x1f813af5) ) // bank 0
	ROM_LOAD( "snd04.bin", 0x1c00000, 0x0400000, BADCRC(0x035c4ff3) ) // bank 1
	ROM_LOAD( "snd05.bin", 0x2000000, 0x0400000, BADCRC(0x0f01f7b0) ) // bank 1
	ROM_LOAD( "snd06.bin", 0x2400000, 0x0400000, BADCRC(0x31574b1c) ) // bank 1
	ROM_LOAD( "snd07.bin", 0x2800000, 0x0400000, BADCRC(0x388e2c91) ) // bank 2
	ROM_LOAD( "snd08.bin", 0x2c00000, 0x0400000, BADCRC(0x6e7e3f23) ) // bank 2
	ROM_LOAD( "snd09.bin", 0x3000000, 0x0400000, BADCRC(0x39fa512f) ) // bank 2

ROM_END


ROM_START( rocknms )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live_m.bin", 0x000000, 0x100000, BADCRC(0x857483da) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live_s.bin", 0x000000, 0x100000, BADCRC(0x630dcb2e) )

	ROM_REGION( 0x0800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr_m.im1", 0x000000, 0x400000, BADCRC(0x1caad02a) )
	ROM_LOAD32_WORD( "spr_m.im2", 0x000002, 0x400000, BADCRC(0x520152dc) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr_m.bin", 0x000000, 0x200000, BADCRC(0x1ca30e3f) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot_m.bin", 0x000000, 0x200000, BADCRC(0x1f29b622) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc_m.bin", 0x000000, 0x080000, BADCRC(0xa4717579) )

	ROM_REGION( 0x0800000, REGION_GFX5, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr_s.im1", 0x000000, 0x400000, BADCRC(0xeea4699c) )
	ROM_LOAD32_WORD( "spr_s.im2", 0x000002, 0x400000, BADCRC(0x2c02a1ec) )

	ROM_REGION( 0x200000, REGION_GFX6, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr_s.bin", 0x000000, 0x200000, BADCRC(0xa00dfdaa) )

	ROM_REGION( 0x200000, REGION_GFX7, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot_s.bin", 0x000000, 0x200000, BADCRC(0xc76eff18) )

	ROM_REGION( 0x080000, REGION_GFX8, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc_s.bin", 0x000000, 0x080000, BADCRC(0x26a8bcaa) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rtdx001.bin", 0x0000000, 0x0400000, BADCRC(0x8bafae71) ) // COMMON AREA
	ROM_FILL(                0x0400000, 0x0c00000, 0xffffffff ) 		// BANK AREA
	ROM_LOAD( "rtdx002.bin", 0x1000000, 0x0400000, BADCRC(0xeec0589b) ) // bank 0
	ROM_LOAD( "rtdx003.bin", 0x1400000, 0x0400000, BADCRC(0x564aa972) ) // bank 0
	ROM_LOAD( "rtdx004.bin", 0x1800000, 0x0400000, BADCRC(0x940302d0) ) // bank 0
	ROM_LOAD( "rtdx005.bin", 0x1c00000, 0x0400000, BADCRC(0x766db7f8) ) // bank 1
	ROM_LOAD( "rtdx006.bin", 0x2000000, 0x0400000, BADCRC(0x3a3002f9) ) // bank 1
	ROM_LOAD( "rtdx007.bin", 0x2400000, 0x0400000, BADCRC(0x06b04df9) ) // bank 1
	ROM_LOAD( "rtdx008.bin", 0x2800000, 0x0400000, BADCRC(0xda74305e) ) // bank 2
	ROM_LOAD( "rtdx009.bin", 0x2c00000, 0x0400000, BADCRC(0xb5a0aa48) ) // bank 2
	ROM_LOAD( "rtdx010.bin", 0x3000000, 0x0400000, BADCRC(0x0fd4a088) ) // bank 2
	ROM_LOAD( "rtdx011.bin", 0x3400000, 0x0400000, BADCRC(0x33c89e53) ) // bank 3
	ROM_LOAD( "rtdx012.bin", 0x3800000, 0x0400000, BADCRC(0xf9256a3f) ) // bank 3
	ROM_LOAD( "rtdx013.bin", 0x3c00000, 0x0400000, BADCRC(0xb0a09f3e) ) // bank 3
	ROM_LOAD( "rtdx014.bin", 0x4000000, 0x0400000, BADCRC(0xd5cee673) ) // bank 4
	ROM_LOAD( "rtdx015.bin", 0x4400000, 0x0400000, BADCRC(0xb394aa8a) ) // bank 4
	ROM_LOAD( "rtdx016.bin", 0x4800000, 0x0400000, BADCRC(0x6c791501) ) // bank 4
	ROM_LOAD( "rtdx017.bin", 0x4c00000, 0x0400000, BADCRC(0xfe80159e) ) // bank 5
	ROM_LOAD( "rtdx018.bin", 0x5000000, 0x0400000, BADCRC(0x142c1159) ) // bank 5
	ROM_LOAD( "rtdx019.bin", 0x5400000, 0x0400000, BADCRC(0xcc595d85) ) // bank 5
	ROM_LOAD( "rtdx020.bin", 0x5800000, 0x0400000, BADCRC(0x82b085a3) ) // bank 6
	ROM_LOAD( "rtdx021.bin", 0x5c00000, 0x0400000, BADCRC(0xdd5e9680) ) // bank 6

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1997, tetrisp2, 0,        tetrisp2, tetrisp2, 0, ROT0, "Jaleco / The Tetris Company", "Tetris Plus 2 (World?)" )
GAME( 1997, teplus2j, tetrisp2, tetrisp2, teplus2j, 0, ROT0, "Jaleco / The Tetris Company", "Tetris Plus 2 (Japan)" )

GAME( 1999,  rockn1, 0, rockn1,  rockn1,  rockn1, ROT270, "Jaleco", "Rock'n Tread 1 (Japan)" )
GAME( 1999,  rockn2, 0, rockn2,  rockn1,  rockn2, ROT270, "Jaleco", "Rock'n Tread 2 (Japan)" )
GAMEX(1999, rocknms, 0,rocknms, rocknms, rocknms, ROT270, "Jaleco", "Rock'n MegaSession (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1999,  rockn3, 0, rockn2,  rockn1,  rockn3, ROT270, "Jaleco", "Rock'n 3 (Japan)" )
GAME( 2000,  rockn4, 0, rockn2,  rockn1,  rockn3, ROT270, "Jaleco (PCCWJ)", "Rock'n 4 (Japan prototype version)" )
