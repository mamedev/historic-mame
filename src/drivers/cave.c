/***************************************************************************

							  -= Cave Games =-

				driver by	Luca Elia (eliavit@unina.it)


CPU:	MC68000
Sound:	YMZ280B
Other:	EEPROM

---------------------------------------------------------------------------
Game					Year	PCB		License		Issues / Notes
---------------------------------------------------------------------------
Donpachi				1995						- Not Dumped -
Dodonpachi				1997						- Not Dumped -
Dangun Feveron (Japan)	1998	CV01    Nihon System Inc.
ESP Ra.De.	   (Japan)	1998	ATC04	Atlus
Uo Poko        (Japan)	1998	CV02	Jaleco
Guwange					1999						- Not Dumped -
---------------------------------------------------------------------------

	- Note: press F2 to enter service mode: there are no DSWs -

To Do:

- Sound
- Alignment issues between sprites and layers: see uopoko!


***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

/* Variables that vidhrdw has access to */

/* Variables defined in vidhrdw */
extern unsigned char *cave_videoregs;

extern unsigned char *cave_vram_0, *cave_vctrl_0;
extern unsigned char *cave_vram_1, *cave_vctrl_1;
extern unsigned char *cave_vram_2, *cave_vctrl_2;

extern unsigned char *cave_soundram;

/* Functions defined in vidhrdw */
WRITE_HANDLER( cave_vram_0_w );
WRITE_HANDLER( cave_vram_1_w );
WRITE_HANDLER( cave_vram_2_w );

WRITE_HANDLER( cave_soundram_w );

int  dfeveron_vh_start(void);
int  esprade_vh_start(void);
int  uopoko_vh_start(void);
void dfeveron_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void cave_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



/***************************************************************************


								Memory Maps


***************************************************************************/


static struct EEPROM_interface eeprom_interface =
{
	6,				// address bits
	16,				// data bits
	"0110",			// read command
	"0101",			// write command
	0,				// erase command
	0,				// lock command
	"0100110000" 	// unlock command
};

WRITE_HANDLER( cave_eeprom_w )
{
	if ( (data & 0xFF000000) == 0 )  // even address
	{
		// latch the bit
		EEPROM_write_bit(data & 0x0800);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE );
	}
}

void cave_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file) EEPROM_load(file);
		else usrintf_showmessage("You MUST initialize NVRAM in service mode");
	}
}


READ_HANDLER( cave_inputs_r )
{
	switch (offset)
	{
		case 0:
			return readinputport(0);

		case 2:
			return	readinputport(1) |
					( (EEPROM_read_bit() & 0x01) << 11 );

		default:	return 0;
	}
}

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/


/***************************************************************************
								Dangun Feveron
***************************************************************************/

static struct MemoryReadAddress dfeveron_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA_BANK1					},	// RAM
	{ 0x300000, 0x300003, MRA_BANK2					},	// From sound
/**/{ 0x400000, 0x407fff, MRA_BANK3					},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA_BANK4					},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA_BANK5					},	// Layer 0 (size?)
/**/{ 0x600000, 0x607fff, MRA_BANK6					},	// Layer 1 (size?)
/**/{ 0x708000, 0x708fff, MRA_BANK7					},	// Palette
/**/{ 0x710000, 0x710fff, MRA_BANK8					},	// ?
//	{ 0x800000, 0x800079, MRA_BANK9					},	// ?
/**/{ 0x900000, 0x900005, MRA_BANK10				},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA_BANK11				},	// Layer 1 Control
	{ 0xb00000, 0xb00003, cave_inputs_r				},	// Inputs + EEPROM
/**/{ 0xc00000, 0xc00001, MRA_BANK12				},	//
	{ -1 }
};

static struct MemoryWriteAddress dfeveron_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM									},	// ROM
	{ 0x100000, 0x10ffff, MWA_BANK1									},	// RAM
	{ 0x300000, 0x300003, cave_soundram_w, &cave_soundram			},	// To Sound
	{ 0x400000, 0x407fff, MWA_BANK3, &spriteram, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA_BANK4									},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0				},	// Layer 0 (size?)
	{ 0x600000, 0x607fff, cave_vram_1_w, &cave_vram_1				},	// Layer 1 (size?)
	{ 0x708000, 0x708fff, paletteram_xGGGGGRRRRRBBBBB_word_w, &paletteram },	// Palette
	{ 0x710c00, 0x710fff, MWA_BANK8									},	// ?
	{ 0x800000, 0x800079, MWA_BANK9,  &cave_videoregs				},	// Video Regs?
	{ 0x900000, 0x900005, MWA_BANK10, &cave_vctrl_0					},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA_BANK11, &cave_vctrl_1					},	// Layer 1 Control
	{ 0xc00000, 0xc00001, cave_eeprom_w								},	// EEPROM
	{ -1 }
};







/***************************************************************************
									Esprade
***************************************************************************/


static struct MemoryReadAddress esprade_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA_BANK1					},	// RAM
	{ 0x300000, 0x300003, MRA_BANK2					},	// From sound
/**/{ 0x400000, 0x407fff, MRA_BANK3					},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA_BANK4					},	// Sprites?
/**/{ 0x500000, 0x507fff, MRA_BANK5					},	// Layer 0 (size?)
/**/{ 0x600000, 0x607fff, MRA_BANK6					},	// Layer 1 (size?)
/**/{ 0x700000, 0x707fff, MRA_BANK7					},	// Layer 2 (size?)
//	{ 0x800000, 0x800079, MRA_BANK8					},	// ?
/**/{ 0x900000, 0x900005, MRA_BANK9					},	// Layer 0 Control
/**/{ 0xa00000, 0xa00005, MRA_BANK10				},	// Layer 1 Control
/**/{ 0xb00000, 0xb00005, MRA_BANK11				},	// Layer 2 Control
/**/{ 0xc00000, 0xc0ffff, MRA_BANK12				},	// Palette
	{ 0xd00000, 0xd00003, cave_inputs_r				},	// Inputs + EEPROM
/**/{ 0xe00000, 0xe00001, MRA_BANK13				},	//
	{ -1 }
};

static struct MemoryWriteAddress esprade_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM									},	// ROM
	{ 0x100000, 0x10ffff, MWA_BANK1									},	// RAM
	{ 0x300000, 0x300003, cave_soundram_w, &cave_soundram			},	// To Sound
	{ 0x400000, 0x407fff, MWA_BANK3, &spriteram, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA_BANK4									},	// Sprites?
	{ 0x500000, 0x507fff, cave_vram_0_w, &cave_vram_0				},	// Layer 0 (size?)
	{ 0x600000, 0x607fff, cave_vram_1_w, &cave_vram_1				},	// Layer 1 (size?)
	{ 0x700000, 0x707fff, cave_vram_2_w, &cave_vram_2				},	// Layer 2 (size?)
	{ 0x800000, 0x800079, MWA_BANK8,  &cave_videoregs				},	// Video Regs?
	{ 0x900000, 0x900005, MWA_BANK9,  &cave_vctrl_0					},	// Layer 0 Control
	{ 0xa00000, 0xa00005, MWA_BANK10, &cave_vctrl_1					},	// Layer 1 Control
	{ 0xb00000, 0xb00005, MWA_BANK11, &cave_vctrl_2					},	// Layer 2 Control
	{ 0xc00000, 0xc0ffff, paletteram_xGGGGGRRRRRBBBBB_word_w, &paletteram },	// Palette
	{ 0xe00000, 0xe00001, cave_eeprom_w								},	// EEPROM
	{ -1 }
};





/***************************************************************************
								Uo Poko
***************************************************************************/

static struct MemoryReadAddress uopoko_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA_BANK1					},	// RAM
	{ 0x300000, 0x300003, MRA_BANK2					},	// From sound
/**/{ 0x400000, 0x407fff, MRA_BANK3					},	// Sprites
/**/{ 0x408000, 0x40ffff, MRA_BANK4					},	// Sprites?
/**/{ 0x500000, 0x501fff, MRA_BANK5					},	// Layer 0 (size?)
//	{ 0x600000, 0x60006f, MRA_BANK6					},	// ?
/**/{ 0x700000, 0x700005, MRA_BANK7					},	// Layer 0 Control
/**/{ 0x800000, 0x80ffff, MRA_BANK8					},	// Palette
	{ 0x900000, 0x900003, cave_inputs_r				},	// Inputs + EEPROM
/**/{ 0xa00000, 0xa00001, MRA_BANK9					},	//
	{ -1 }
};

static struct MemoryWriteAddress uopoko_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM									},	// ROM
	{ 0x100000, 0x10ffff, MWA_BANK1									},	// RAM
	{ 0x300000, 0x300003, cave_soundram_w, &cave_soundram			},	// To Sound
	{ 0x400000, 0x407fff, MWA_BANK3, &spriteram, &spriteram_size	},	// Sprites
	{ 0x408000, 0x40ffff, MWA_BANK4									},	// Sprites?
	{ 0x500000, 0x501fff, cave_vram_0_w, &cave_vram_0				},	// Layer 0 (size?)
	{ 0x600000, 0x60006f, MWA_BANK6,  &cave_videoregs				},	// Video Regs?
	{ 0x700000, 0x700005, MWA_BANK7,  &cave_vctrl_0					},	// Layer 0 Control
	{ 0x800000, 0x80ffff, paletteram_xGGGGGRRRRRBBBBB_word_w, &paletteram },	// Palette
	{ 0xa00000, 0xa00001, cave_eeprom_w								},	// EEPROM
	{ -1 }
};









/***************************************************************************


								Input Ports


***************************************************************************/

/*
	dfeveron config menu:
	101624.w -> 8,a6	preferences
	101626.w -> c,a6	(1:coin<<4|credit) <<8 | (2:coin<<4|credit)
*/

INPUT_PORTS_START( dfeveron )

	PORT_START	// IN0 - $b00000
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN1, 1)
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? exit service mode
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $b00002
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT_IMPULSE(  0x0100, IP_ACTIVE_LOW, IPT_COIN2, 1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	//         0x0800  eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END







/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 16x16x4 tiles */
static struct GfxLayout layout_4bit =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
	 64*4,65*4,66*4,67*4,68*4,69*4,70*4,71*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
	 16*32,17*32,18*32,19*32,20*32,21*32,22*32,23*32},
	16*16*4
};

/* 16x16x8 tiles */
static struct GfxLayout layout_8bit =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{8,9,10,11, 0,1,2,3},
	{0*4,1*4,4*4,5*4,8*4,9*4,12*4,13*4,
	 128*4,129*4,132*4,133*4,136*4,137*4,140*4,141*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64,
	 16*64,17*64,18*64,19*64,20*64,21*64,22*64,23*64},
	16*16*8
};


#if 0
/* 16x16x4 Zooming Sprites - No need to decode them, but if we do,
   to have a look, we must remember the data has been converted from
   4 bits/pixel to 1 byte per pixel, for the sprite manager */

static struct GfxLayout layout_spritemanager =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{4,5,6,7},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
	 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,
	 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
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
	   A vh_init_palette function is provided as well, for sprites */

	{ REGION_GFX1, 0, &layout_4bit,          0x4400, 0x40 }, // [0] Tiles
	{ REGION_GFX2, 0, &layout_4bit,          0x4400, 0x40 }, // [1] Tiles
//	{ REGION_GFX3, 0, &layout_4bit,          0x4400, 0x40 }, // [2] Tiles
//	{ REGION_GFX4, 0, &layout_spritemanager, 0x0000, 0x40 }, // [3] 4 Bit Sprites
	{ -1 }
};


/***************************************************************************
								Esprade
***************************************************************************/

static struct GfxDecodeInfo esprade_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8bit,          0x4000, 0x40 }, // [0] Tiles
	{ REGION_GFX2, 0, &layout_8bit,          0x4000, 0x40 }, // [1] Tiles
	{ REGION_GFX3, 0, &layout_8bit,          0x4000, 0x40 }, // [2] Tiles
//	{ REGION_GFX4, 0, &layout_spritemanager, 0x0000, 0x40 }, // [3] 8 Bit Sprites
	{ -1 }
};



/***************************************************************************
								Uo Poko
***************************************************************************/

static struct GfxDecodeInfo uopoko_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8bit,          0x4000, 0x40 }, // [0] Tiles
//	{ REGION_GFX4, 0, &layout_spritemanager, 0x0000, 0x40 }, // [1] 8 Bit Sprites
	{ -1 }
};




/***************************************************************************


								Machine Drivers


***************************************************************************/

#define cave_INTERRUPT_NUM 1
int cave_interrupt(void)
{
	switch (cpu_getiloops())
	{
		case 0:		return 1;
		default:	return ignore_interrupt();
	}
}





/***************************************************************************
								Dangun Feveron
***************************************************************************/

static struct MachineDriver machine_driver_dfeveron =
{
	{
		{
			CPU_M68000,
			16000000,
			dfeveron_readmem, dfeveron_writemem,0,0,
			cave_interrupt, cave_INTERRUPT_NUM
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	dfeveron_gfxdecodeinfo,
	0x800, 0x8000,	/* $8000 palette entries for consistency with the other games */
	dfeveron_vh_init_palette,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dfeveron_vh_start,
	0,
	cave_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{0}
	},

	cave_nvram_handler
};





/***************************************************************************
								Esprade
***************************************************************************/


static struct MachineDriver machine_driver_esprade =
{
	{
		{
			CPU_M68000,
			16000000,
			esprade_readmem, esprade_writemem,0,0,
			cave_interrupt, cave_INTERRUPT_NUM
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	esprade_gfxdecodeinfo,
	0x8000, 0x8000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	esprade_vh_start,
	0,
	cave_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{0}
	},

	cave_nvram_handler
};



/***************************************************************************
								Uo Poko
***************************************************************************/

static struct MachineDriver machine_driver_uopoko =
{
	{
		{
			CPU_M68000,
			16000000,
			uopoko_readmem, uopoko_writemem,0,0,
			cave_interrupt, cave_INTERRUPT_NUM
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	uopoko_gfxdecodeinfo,
	0x8000, 0x8000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	uopoko_vh_start,
	0,
	cave_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{0}
	},

	cave_nvram_handler
};





/***************************************************************************


								ROMs Loading


***************************************************************************/

/* 4 bits -> 8 bits */
static void unpack_sprites(void)
{
	const int region		=	REGION_GFX4;	// sprites

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


/* 2 pages of 4 bits -> 8 bits */
static void esprade_unpack_sprites(void)
{
	const int region		=	REGION_GFX4;	// sprites

	unsigned char *src		=	memory_region(region);
	unsigned char *dst		=	memory_region(region) + memory_region_length(region);

	while(src < dst)
	{
		unsigned char data1 = src[0];
		unsigned char data2 = src[1];

		src[0] = (data1 & 0xf0) + (data2 & 0x0f);
		src[1] = ((data1 & 0x0f)<<4) + ((data2 & 0xf0)>>4);

		src += 2;
	}
}




/***************************************************************************

								Dangun Feveron

Board:	CV01
OSC:	28.0, 16.0, 16.9 MHz


***************************************************************************/

ROM_START( dfeveron )

	ROM_REGION( 0x100000, REGION_CPU1 )		/* 68000 Code */
	ROM_LOAD_EVEN( "cv01-u34.bin", 0x000000, 0x080000, 0xbe87f19d )
	ROM_LOAD_ODD ( "cv01-u33.bin", 0x000000, 0x080000, 0xe53a7db3 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "cv01-u50.bin", 0x000000, 0x200000, 0x7a344417 )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "cv01-u49.bin", 0x000000, 0x200000, 0xd21cdda7 )

//	ROM_REGION( 0x200000, REGION_GFX3 | REGIONFLAG_DISPOSE )	/* Layer 3 */
//	empty

	ROM_REGION( 0x800000 * 2, REGION_GFX4 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "cv01-u25.bin", 0x000000, 0x400000, 0xa6f6a95d )
	ROM_LOAD( "cv01-u26.bin", 0x400000, 0x400000, 0x32edb62a )

	ROM_REGION( 0x400000, REGION_SOUND1 )	/* Samples */
	ROM_LOAD( "cv01-u19.bin", 0x000000, 0x400000, 0x5f5514da )

ROM_END


void init_dfeveron(void)
{
	unpack_sprites();
}




/***************************************************************************

									Esprade

ATC04
OSC:	28.0, 16.0, 16.9 MHz

***************************************************************************/


ROM_START( esprade )

	ROM_REGION( 0x100000, REGION_CPU1 )		/* 68000 Code */
	ROM_LOAD_EVEN( "u42.bin", 0x000000, 0x080000, 0x0718c7e5 )
	ROM_LOAD_ODD ( "u41.bin", 0x000000, 0x080000, 0xdef30539 )

	ROM_REGION( 0x800000, REGION_GFX1 | REGIONFLAG_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, 0xe7ca6936 )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, 0xf53bd94f )

	ROM_REGION( 0x800000, REGION_GFX2 | REGIONFLAG_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, 0xe7abe7b4 )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, 0x51a0f391 )

	ROM_REGION( 0x400000, REGION_GFX3 | REGIONFLAG_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, 0x0b9b875c )

	ROM_REGION( 0x1000000, REGION_GFX4 )		/* Sprites (do not dispose) */
	ROM_LOAD_GFX_EVEN( "u63.bin", 0x000000, 0x400000, 0x2f2fe92c )
	ROM_LOAD_GFX_ODD ( "u64.bin", 0x000000, 0x400000, 0x491a3da4 )
	ROM_LOAD_GFX_EVEN( "u65.bin", 0x800000, 0x400000, 0x06563efe )
	ROM_LOAD_GFX_ODD ( "u66.bin", 0x800000, 0x400000, 0x7bbe4cfc )

	ROM_REGION( 0x400000, REGION_SOUND1 )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, 0xf54b1cab )

ROM_END


void init_esprade(void)
{
	esprade_unpack_sprites();
}




/***************************************************************************

									Uo Poko
Board: CV02
OSC:	28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( uopoko )

	ROM_REGION( 0x100000, REGION_CPU1 )		/* 68000 Code */
	ROM_LOAD_EVEN( "u26j.bin", 0x000000, 0x080000, 0xe7eec050 )
	ROM_LOAD_ODD ( "u25j.bin", 0x000000, 0x080000, 0x68cb6211 )

	ROM_REGION( 0x400000, REGION_GFX1 | REGIONFLAG_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u49.bin", 0x000000, 0x400000, 0x12fb11bb )

//	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )	/* Layer 2 */
//	empty

//	ROM_REGION( 0x200000, REGION_GFX3 | REGIONFLAG_DISPOSE )	/* Layer 3 */
//	empty

	ROM_REGION( 0x400000 * 2, REGION_GFX4 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "u33.bin", 0x000000, 0x400000, 0x5d142ad2 )

	ROM_REGION( 0x400000, REGION_SOUND1 )	/* Samples */
	ROM_LOAD( "u4.bin", 0x000000, 0x200000, 0xa2d0d755 )

ROM_END


void init_uopoko(void)
{
	unpack_sprites();
}




/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1998, dfeveron, 0, dfeveron, dfeveron, dfeveron, ROT270_16BIT, "Cave (Nihon System license)", "Dangun Feveron (Japan)", GAME_NO_SOUND )
GAMEX( 1998, esprade,  0, esprade,  dfeveron, esprade,  ROT270_16BIT, "Atlus/Cave", "ESP Ra.De. (Japan)", GAME_NO_SOUND )
GAMEX( 1998, uopoko,   0, uopoko,   dfeveron, uopoko,   ROT0_16BIT,   "Cave (Jaleco license)", "Uo Poko (Japan)", GAME_NO_SOUND )
