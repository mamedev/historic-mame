/***************************************************************************

Legionnaire (c) Tad 1992
-----------

David Graves

Made from MAME D-con and Toki drivers (by Bryan McPhail, Jarek Parchanski)


BK3 charsets
------------

The GFX roms contain two odd sets of 256 16x16 tiles marked as BK3.
It is not known which area includes tilemaps accessing the two BK3
charsets. The 0x104000 area appears to be extra paletteram?

Note that one BK3 is at end of GFX3. The other was scrambled in the
char gfx, and so has a separate gfxdecode.


TODO
----

Foreground tiles screwy (screen after character selection screen).
Tile selection for 'working' layers may be wrong too - we are only
accessing half the available 0x2000 tiles in each gfx set.

Need 16 px off top of vis area?

Unemulated protection (in attract demo things get weird; not playable)

Inputs - all ok?



Preliminary COP MCU memory map
------------------------------

0x400-0x5ff   Protection related
0x600-0x6ff   Includes standard screen control words
0x700-0x7ff   Includes standard Seibu sound system


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

WRITE16_HANDLER( legionna_background_w );
WRITE16_HANDLER( legionna_foreground_w );
WRITE16_HANDLER( legionna_midground_w );
WRITE16_HANDLER( legionna_text_w );
WRITE16_HANDLER( legionna_control_w );

int legionna_vh_start(void);
void legionna_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

extern data16_t *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;
static data16_t *mcu_ram;

static WRITE16_HANDLER( legionna_paletteram16_w )	/* xBBBBxRRRRxGGGGx */
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	a = paletteram16[offset];

	r = (a >> 1) & 0x0f;
	g = (a >> 6) & 0x0f;
	b = (a >> 11) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(offset,r,g,b);
}

/* Mcu reads in attract in game demo

Guess the 0x400-0x5ff area of the COP is protection related.

CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003546 unknown MCU write offset: 0262 data: 02c4
CPU0 PC 00354a unknown MCU write offset: 0252 data: 0004
CPU0 PC 00355c unknown MCU write offset: 0263 data: 0000
CPU0 PC 003560 unknown MCU write offset: 0253 data: 0004
CPU0 PC 003568 unknown MCU write offset: 0280 data: a180
CPU0 PC 00356e unknown MCU write offset: 0280 data: a980
CPU0 PC 003574 unknown MCU write offset: 0280 data: b100
CPU0 PC 00357a unknown MCU write offset: 0280 data: b900
CPU0 PC 003580 unknown MCU read offset: 02c4
CPU0 PC 003588 unknown MCU read offset: 02c2
CPU0 PC 003594 unknown MCU read offset: 02c1
CPU0 PC 0035a0 unknown MCU read offset: 02c3
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003422 unknown MCU write offset: 0280 data: 138e
CPU0 PC 003428 unknown MCU read offset: 02da
CPU0 PC 00342e unknown MCU read offset: 02d8
CPU0 PC 00346c unknown MCU write offset: 0280 data: 3bb0
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 987c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 003306 unknown MCU write offset: 0280 data: 8100
CPU0 PC 00330c unknown MCU write offset: 0280 data: 8900
*/

static READ16_HANDLER( mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		case (0x470/2):	/* read PC $110a, could be some sort of control word:
				sometimes a bit is changed then it's poked back in... */
			return (rand() &0xffff);

		case (0x482/2):	/* read PC $3594 */
			return (rand() &0xffff);

		case (0x484/2):	/* read PC $3588 */
			return (rand() &0xffff);

		case (0x486/2):	/* read PC $35a0 */
			return (rand() &0xffff);

		case (0x488/2):	/* read PC $3580 */
			return (rand() &0xffff);

		case (0x5b0/2):	/* bit 15 is branched on a few times in the $3300 area */
			return (rand() &0xffff);

		case (0x5b4/2):	/* read and stored in ram before +0x5b0 bit 15 tested */
			return (rand() &0xffff);

		/* Non-protection reads */

		case (0x708/2):	/* seibu sound: these three around $b10 on */
			return seibu_main_word_r(2,0);

		case (0x70c/2):
			return seibu_main_word_r(3,0);

		case (0x714/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):	/* code at $b00 sticks waiting for bit 6 hi */
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):	/* code at $f4a reads this 4 times in _weird_ fashion */
			return input_port_0_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
logerror("CPU0 PC %06x unknown MCU read offset: %04x\n",cpu_getpreviouspc(),offset);

	return mcu_ram[offset];
}

static WRITE16_HANDLER( mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		// 61a bit 0 probably flipscreen
		// 61c probably layer disables, like Dcon
		// 620 - 62a scroll control;  is there a layer priority switch...?

		case (0x620/2):
		{
			legionna_scrollram16[0] = mcu_ram[offset];
			break;
		}
		case (0x622/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x624/2):
		{
			legionna_scrollram16[2] = mcu_ram[offset];
			break;
		}
		case (0x626/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x628/2):
		{
			legionna_scrollram16[4] = mcu_ram[offset];
			break;
		}
		case (0x62a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		case (0x700/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x704/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x710/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x718/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
		default:
logerror("CPU0 PC %06x unknown MCU write offset: %04x data: %04x\n",cpu_getpreviouspc(),offset,data);
	}
}

/*****************************************************************************/

static MEMORY_READ16_START( legionna_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, mcu_r },	/* COP mcu */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x1027ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102800, 0x1037ff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette mirror ? */
//	{ 0x104000, 0x10401f, MRA16_RAM },	/* debugging... */
//	{ 0x104200, 0x1043ff, MRA16_RAM },	/* BK3 (1) tilemap ??? */
//	{ 0x104600, 0x1047ff, MRA16_RAM },	/* BK3 (2) tilemap ??? */
//	{ 0x104800, 0x10481f, MRA16_RAM },	/* ??? */

	{ 0x105000, 0x105fff, MRA16_RAM },	/* spriteram */
	{ 0x106000, 0x106fff, MRA16_RAM },
	{ 0x107000, 0x107fff, MRA16_RAM },	/* palette */
	{ 0x108000, 0x113fff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( legionna_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x101000, 0x1017ff, legionna_background_w, &legionna_back_data },
	{ 0x101800, 0x101fff, legionna_foreground_w, &legionna_fore_data },
	{ 0x102000, 0x1027ff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102800, 0x1037ff, legionna_text_w, &legionna_textram },

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MWA16_RAM },
//	{ 0x104000, 0x104fff, legionna_paletteram16_w },
//	{ 0x104000, 0x10401f, MWA16_RAM },
//	{ 0x104200, 0x1043ff, MWA16_RAM },
//	{ 0x104600, 0x1047ff, MWA16_RAM },
//	{ 0x104800, 0x10481f, MWA16_RAM },

	{ 0x105000, 0x105fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x106000, 0x106fff, MWA16_RAM },	/* is this used outside inits ?? */
	{ 0x107000, 0x107fff, legionna_paletteram16_w, &paletteram16 },	/* palette xRRRRxGGGGxBBBBx ? */
	{ 0x108000, 0x113fff, MWA16_RAM },
MEMORY_END


/*****************************************************************************/

INPUT_PORTS_START( legionna )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0400, 0x0400, "Extend" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
//	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/*****************************************************************************/

/* There's something weird with the tilemap gfx for Legionnaire. It looks
   from the contents of the tilemap layers that Tad stuck to 1 word per
   tile with top four bits as color bank. This leaves only 0xfff tiles to
   address, but the bg gfx set has twice this many. It also seems to be
   three sets in one, with the last just comprising 0xff tiles.
   The char gfx is also weird, it looks to need various tile sizes and
   decoding schemes. */

static struct GfxLayout legionna_charlayout =
{
	8,8,
	RGN_FRAC(1,4),	/* other half is BK3, decoded in char2layout */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout legionna_char2layout =
{
	16,16,
	256,	/* Can't use RGN_FRAC as (1,16) not supported */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8,
	  1024*16*8 +3,  1024*16*8 +2,  1024*16*8 +1, 1024*16*8 +0,
	  1024*16*8 +11, 1024*16*8 +10, 1024*16*8 +9, 1024*16*8 +8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  512*16*8 +0*16, 512*16*8 +1*16, 512*16*8 +2*16, 512*16*8 +3*16,
	  512*16*8 +4*16, 512*16*8 +5*16, 512*16*8 +6*16, 512*16*8 +7*16 },
	16*8
};

static struct GfxLayout legionna_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout legionna_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxDecodeInfo legionna_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &legionna_charlayout,   48*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &legionna_char2layout,  32*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,   16*16, 16 },
	{ REGION_GFX2, 0, &legionna_spritelayout , 0*16, 8*16 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);


/*****************************************************************************/

static const struct MachineDriver machine_driver_legionna =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			20000000/2, 	/* ??? */
			legionna_readmem,legionna_writemem,0,0,
			m68_level4_irq,1 /* VBL */
		},
		{
			SEIBU_SOUND_SYSTEM_CPU(14318180/4)
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	seibu_sound_init_1, /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },	// maybe topy is 2*8 ??

	legionna_gfxdecodeinfo,
	128*16, 0,
	0,
	VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM,
	0,
	legionna_vh_start,
	0,
	legionna_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( legionna )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1",   0x00000, 0x20000, 0x9e2d3ec8 )
	ROM_LOAD32_BYTE( "2",   0x00001, 0x20000, 0x35c8a28f )
	ROM_LOAD32_BYTE( "3",   0x00002, 0x20000, 0x553fc7c0 )
	ROM_LOAD32_BYTE( "4",   0x00003, 0x20000, 0x91fd4648 )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "6",   0x00000, 0x08000, 0xfe7b8d06 )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff ?? */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",   0x000000, 0x10000, 0x88e26809 )	/* chars */
	ROM_LOAD( "8",   0x010000, 0x10000, 0x06e35407 )	/* chars */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, 0xd35602f5 )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, 0x351d3917 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "back",     0x000000, 0x100000, 0x58280989 )	/* tiles... (a BK3 set at end) */

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* for BK3 decode */
	ROM_COPY( REGION_GFX1, 0x00000, 0x00000, 0x20000)

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "5",   0x00000, 0x20000, 0x21d09bde )
ROM_END



static void init_legionna(void)
{
	data8_t *gfx = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1)/2;
	int a,i;

	for (i = 0; i < len/2; i++)
	{
		a = gfx[i];
		gfx[i] = gfx[i + len/2];
		gfx[i+len/2] = a;

		a = gfx[i+len];
		gfx[i+len] = gfx[i + len/2 + len];
		gfx[i + len/2 +len] = a;
	}
}



GAMEX( 1992, legionna, 0, legionna, legionna, legionna, ROT0, "Tad (Fabtek license)", "Legionnaire (US)", GAME_UNEMULATED_PROTECTION )

