/********************************************************************

Mazinger Z

driver by Nicola Salmoria

TODO:
- EEPROM interface is slightly wrong, it's probably something minor but
  the game doesn't recover the data it writes.

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"



void mazinger_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	palette_recalc();

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	for (offs = 0;offs < (spriteram_size >> 1);offs += 8)
	{
		int sx = spriteram16[offs];
		int sy = spriteram16[offs+1];
		int code = spriteram16[offs+3];
		drawgfx(bitmap,Machine->uifont,
				code,
				0,
				0,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
}



static READ16_HANDLER( rom16_r )
{
	data16_t *rom16 = (data16_t *)memory_region(REGION_USER1);
	return rom16[offset];
}

/***************************************************************************

  EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,			/* address bits */
	16,			/* data bits */
	"0110",		/*  read command */
	"0101",		/* write command */
	0,			/* erase command */
	"1000000000",	/* lock command */
	"01001100000"	/* unlock command */
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

static READ16_HANDLER( port1_r )
{
	int bit;

	bit = EEPROM_read_bit() << 11;

	return (input_port_1_word_r(0) & 0xf7ff) | bit;
}

static WRITE16_HANDLER( eeprom_w )
{
	if (ACCESSING_MSB)
	{
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_write_bit(data & 0x0800);
		EEPROM_set_clock_line((data & 0x0400) ? CLEAR_LINE : ASSERT_LINE);
	}
}



static READ16_HANDLER( pip )
{
	return rand() & 0xffff;
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x300000, 0x300001, pip },
	{ 0x404000, 0x407fff, MRA16_RAM },
	{ 0x504000, 0x507fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, port1_r },
	{ 0xc08000, 0xc0ffff, MRA16_RAM },
	{ 0xd00000, 0xd7ffff, rom16_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x20ffff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x404000, 0x407fff, MWA16_RAM },
	{ 0x504000, 0x507fff, MWA16_RAM },
	{ 0x900000, 0x900001, eeprom_w },
	{ 0xc08000, 0xc0ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd7ffff, MWA16_ROM },
MEMORY_END



INPUT_PORTS_START( mazinger )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout charlayout2 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 4*4, 5*4, 8*4, 9*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 16 },
	{ REGION_GFX2, 0, &charlayout2, 0, 16 },
	{ REGION_GFX3, 0, &tilelayout,  0, 16 },
	{ REGION_GFX4, 0, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,              	/* 1 chip */
	{ 32000000/8/165 },	/* 24242Hz frequency? */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }			/* volume */
};



static const struct MachineDriver machine_driver_mazinger =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000, /* 16 MHz ? (there seems to be only a 32MHz crystal) */
			readmem,writemem,0,0,
			m68_level1_irq,1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	512, 256, { 0*8, 48*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16384, 16384,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	mazinger_vh_screenrefresh,

	0,0,0,0,
	{
		/* there's also a YM2203 */
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	},

	nvram_handler
};



ROM_START( mazinger )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mzp-0.u24",     0x00000, 0x80000, 0x43a4279f )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )		/* 68000 code (mapped at d00000) */
	ROM_LOAD16_WORD_SWAP( "mzp-1.924",     0x00000, 0x80000, 0xdb40acba )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "mzs.u21",     0x00000, 0x20000, 0xc5b4f7ed )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bp943a-1.u60", 0x000000, 0x200000, 0x46327415 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp943a-0.u63", 0x000000, 0x200000, 0xc1fed98a )	// FIXED BITS (xxxxxxxx00000000)

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bp943a-3.u55", 0x000000, 0x080000, 0x9c4957dd )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "bp943a-2.u56", 0x000000, 0x200000, 0x97e13959 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bp943a-4.u64", 0x000000, 0x080000, 0x3fc7f29a )
ROM_END


GAMEX( 1994, mazinger, 0, mazinger, mazinger, 0, ROT90_16BIT, "Banpresto", "Mazinger Z", GAME_NO_SOUND | GAME_NO_COCKTAIL )
