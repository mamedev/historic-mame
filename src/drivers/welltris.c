/*******************************************************************************
 Welltris (c)1991 Video System
********************************************************************************
 hardware is similar to aerofgt.c but with slightly different sprites, sound,
 and an additional 'pixel' layer used for the backdrops

 Driver by David Haywood
 Thanks to the authors of aerofgt.c and fromance.c on which most of this is
 based
********************************************************************************
OW-13 CPU

CPU  : MC68000P10
Sound: Z80 YM2610 YM3016
OSC  : 14.31818MHz (X1), 12.000MHz (X2),
       8.000MHz (X3), 20.0000MHz (OSC1)

ROMs:
j1.7 - Main programs (271000 compatible onetime)
j2.8 /

lh532j11.9  - Data
lh532j10.10 /

3.144 - Sound program (27c1000)

lh534j09.123 - Samples
lh534j10.124 |
lh534j11.126 /

lh534j12.77 - BG chr.

046.93 - OBJ chr.
048.94 /

PALs (16L8):
ow13-1.1
ow13-2.2
ow13-3.97
ow13-4.115

Custom chips:
V-SYSTEM C7-01 GGA
VS8905 6620 9039 ABBA
V-SYSTEM VS8904 GGB
V-SYSTEM VS8803 6082 9040 EBBB

********************************************************************************

 its impossible to know what some of the video registers do due to lack of
 evidence (bg palette has a selector, but i'm not sure which ... test mode
 colours use different palette on rgb test

 sound needs hooking up

 dipswitches need naming (they're listed in testmode so this is just a case of
 finding the time)

*******************************************************************************/


#include "driver.h"


data16_t *welltris_spriteram;
size_t welltris_spriteram_size;
data16_t *welltris_pixelram;
data16_t *welltris_charvideoram;

WRITE16_HANDLER( welltris_palette_bank_w );
WRITE16_HANDLER( welltris_gfxbank_w );
WRITE16_HANDLER( welltris_charvideoram_w );
int welltris_vh_start(void);
void welltris_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

static MEMORY_READ16_START( welltris_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x17ffff, MRA16_ROM },
	{ 0x800000, 0x81ffff, MRA16_RAM }, /* Graph_1 & 2*/
	{ 0xff8000, 0xffbfff, MRA16_RAM }, /* Work */
	{ 0xffc000, 0xffc3ff, MRA16_RAM }, /* Sprite */
	{ 0xffd000, 0xffdfff, MRA16_RAM }, /* Char */
	{ 0xffe000, 0xffefff, MRA16_RAM }, /* Palette */

	{ 0xfff000, 0xfff001, input_port_1_word_r }, /* Bottom Controls */
	{ 0xfff002, 0xfff003, input_port_2_word_r }, /* Top Controls */
	{ 0xfff004, 0xfff005, input_port_3_word_r }, /* Left Side Ctrls */
	{ 0xfff006, 0xfff007, input_port_4_word_r }, /* Right Side Ctrls */

	{ 0xfff008, 0xfff009, input_port_0_word_r }, /* Coinage, Start Buttons etc. */  /* Bit 5 Tested at start of irq 1 */
	{ 0xfff00a, 0xfff00b, input_port_5_word_r }, /* P3+P4 Coin + Start Buttons */
	{ 0xfff00c, 0xfff00d, input_port_6_word_r }, /* DSW0 Coinage */
	{ 0xfff00e, 0xfff00f, input_port_7_word_r }, /* DSW1 Game Options */
MEMORY_END

static MEMORY_WRITE16_START( welltris_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x17ffff, MWA16_ROM },
	{ 0x800000, 0x81ffff, MWA16_RAM, &welltris_pixelram },
	{ 0xff8000, 0xffbfff, MWA16_RAM },
	{ 0xffc000, 0xffc3ff, MWA16_RAM, &welltris_spriteram, &welltris_spriteram_size },
	{ 0xffd000, 0xffdfff, welltris_charvideoram_w, &welltris_charvideoram},
	{ 0xffe000, 0xffefff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },

	{ 0xfff000, 0xfff001, welltris_palette_bank_w },
	{ 0xfff002, 0xfff003, welltris_gfxbank_w },
//	{ 0xfff004, 0xfff005, ?? },
//	{ 0xfff006, 0xfff007, ?? },
//	{ 0xfff00c, 0xfff00d, ?? },
//	{ 0xfff00e, 0xfff00f, ?? },
MEMORY_END

/* Sound needs hooking up */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
PORT_END



INPUT_PORTS_START( welltris )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Test */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Service */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIPSW 1-1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIPSW 1-2" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIPSW 1-3" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIPSW 1-4" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 1-5" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIPSW 1-6" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIPSW 1-7" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIPSW 1-8" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIPSW 2-1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIPSW 2-2" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIPSW 2-3" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIPSW 2-4" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 2-5" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIPSW 2-6" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIPSW 2-7" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIPSW 2-8" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout welltris_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout welltris_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, RGN_FRAC(1,2)+1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+3*4, RGN_FRAC(1,2)+2*4,
			5*4, 4*4, 7*4, 6*4, RGN_FRAC(1,2)+5*4, RGN_FRAC(1,2)+4*4, RGN_FRAC(1,2)+7*4, RGN_FRAC(1,2)+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static struct GfxDecodeInfo welltris_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &welltris_charlayout,   0, 64 },
	{ REGION_GFX2, 0, &welltris_spritelayout, 0x700, 64 },
	{ -1 } /* end of array */
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static const struct MachineDriver machine_driver_welltris =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			20000000/2,	/* 10 MHz */
			welltris_readmem,welltris_writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			8000000/2,	/* 4 MHz ??? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 256, { 0, 351, 0, 239 },
	welltris_gfxdecodeinfo,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	welltris_vh_start,
	0,
    welltris_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface,
		}
	}
};

ROM_START( welltris )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "j2.8",			0x000000, 0x20000, 0x68ec5691 )
	ROM_LOAD16_BYTE( "j1.7",			0x000001, 0x20000, 0x1598ea2c )
	/* Space */
	ROM_LOAD16_BYTE( "lh532j10.10",		0x100000, 0x40000, 0x1187c665 )
	ROM_LOAD16_BYTE( "lh532j11.9",		0x100001, 0x40000, 0x18eda9e5 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "3.144",        0x00000, 0x20000, 0xae8f763e )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE ) /* CHAR Tiles */
	ROM_LOAD( "lh534j12.77",          0x000000, 0x80000, 0xb61a8b74 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPRITE Tiles */
	ROM_LOAD( "046.93",          0x000000, 0x40000, 0x31d96d77 )
	ROM_LOAD( "048.94",          0x040000, 0x40000, 0xbb4643da )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "lh534j09.123",          0x00000, 0x80000, 0x6c2ce9a5 )
	ROM_LOAD( "lh534j10.124",          0x80000, 0x80000, 0xe3682221 )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "lh534j11.126",          0x00000, 0x80000, 0xbf85fb0d )
ROM_END

GAMEX( 1991, welltris, 0,        welltris, welltris, 0, ROT0,   "Video System Co.", "Welltris (Japan)", GAME_NO_COCKTAIL | GAME_NO_SOUND )
