/***************************************************************************

  Tumblepop (World)     (c) 1991 Data East Corporation
  Tumblepop (Japan)     (c) 1991 Data East Corporation
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 1)
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 2)


  Bootleg sound is not quite correct yet (Nothing on bootleg 2).

  Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"

VIDEO_START( tumblep );
VIDEO_UPDATE( tumblep );
VIDEO_UPDATE( tumblepb );

WRITE16_HANDLER( tumblep_pf1_data_w );
WRITE16_HANDLER( tumblep_pf2_data_w );
WRITE16_HANDLER( tumblep_control_0_w );

extern data16_t *tumblep_pf1_data,*tumblep_pf2_data;

/******************************************************************************/

static WRITE16_HANDLER( tumblep_oki_w )
{
	OKIM6295_data_0_w(0,data&0xff);
    /* STUFF IN OTHER BYTE TOO..*/
}

static READ16_HANDLER( tumblep_prot_r )
{
	return ~0;
}

static WRITE16_HANDLER( tumblep_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

/******************************************************************************/

static READ16_HANDLER( tumblepop_controls_r )
{
 	switch (offset<<1)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 2: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
		case 8: /* Credits */
			return readinputport(2);
		case 10: /* ? */
		case 12:
        	return 0;
	}

	return ~0;
}

static READ16_HANDLER( tumblep_pf1_data_r ) { return tumblep_pf1_data[offset]; }
static READ16_HANDLER( tumblep_pf2_data_r ) { return tumblep_pf2_data[offset]; }

/******************************************************************************/

static MEMORY_READ16_START( tumblepop_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0x320000, 0x320fff, tumblep_pf1_data_r },
	{ 0x322000, 0x322fff, tumblep_pf2_data_r },
MEMORY_END

static MEMORY_WRITE16_START( tumblepop_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x100001, tumblep_sound_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM, &spriteram16 },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( tumblepopb_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, tumblep_prot_r },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tumblepopb_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x100001, tumblep_oki_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x160000, 0x160807, MWA16_RAM, &spriteram16 }, /* Bootleg sprite buffer */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

/* Physical memory map (21 bits) */
static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, MRA_NOP }, /* This board only has 1 oki chip */
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP }, /* YM2203 - this board doesn't have one */
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, MWA_NOP },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( tumblep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout tcharlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tlayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 256, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tlayout,     512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &tlayout,     256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface2 =
{
	1,          /* 1 chip */
	{ 7757 },   /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 70 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* May not be correct, there is another crystal near the ym2151 */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static MACHINE_DRIVER_START( tumblepop )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepop_readmem,tumblepop_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280, 32220000/8)	/* Custom chip 45; Audio section crystal is 32.220 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblep)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tumblepb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepopb_readmem,tumblepopb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblepb)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( tumblep )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hl00-1.f12", 0x00000, 0x40000, 0xfd697c1b )
	ROM_LOAD16_BYTE("hl01-1.f13", 0x00001, 0x40000, 0xd5a62a3f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, 0xa5cab888 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, 0xdfceaa26 )	// encrypted

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, 0x01b81da0 )
ROM_END

ROM_START( tumblepj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hk00-1.f12", 0x00000, 0x40000, 0x2d3e4d3d )
	ROM_LOAD16_BYTE("hk01-1.f13", 0x00001, 0x40000, 0x56912a00 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, 0xa5cab888 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, 0xdfceaa26 )	// encrypted

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, 0x01b81da0 )
ROM_END

ROM_START( tumblepb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.12", 0x00000, 0x40000, 0x0c984703 )
	ROM_LOAD16_BYTE( "thumbpop.13", 0x00001, 0x40000, 0x864c4053 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, 0x0795aab4 )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, 0xad58df43 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, 0xfabbf15d )
ROM_END

ROM_START( tumblep2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.2", 0x00000, 0x40000, 0x34b016e1 )
	ROM_LOAD16_BYTE( "thumbpop.3", 0x00001, 0x40000, 0x89501c71 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, 0x0795aab4 )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, 0xad58df43 )

 	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, 0xe81ffa09 )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, 0x8c879cfe )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, 0xfabbf15d )
ROM_END


/******************************************************************************/


static DRIVER_INIT( tumblep )
{
	deco56_decrypt(REGION_GFX1);
}

static DRIVER_INIT( tumblepb )
{
	data8_t *rom = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);
	int i;

	/* gfx data is in the wrong order */
	for (i = 0;i < len;i++)
	{
		if ((i & 0x20) == 0)
		{
			int t = rom[i]; rom[i] = rom[i + 0x20]; rom[i + 0x20] = t;
		}
	}
	/* low/high half are also swapped */
	for (i = 0;i < len/2;i++)
	{
		int t = rom[i]; rom[i] = rom[i + len/2]; rom[i + len/2] = t;
	}
}


/******************************************************************************/

GAME( 1991, tumblep,  0,       tumblepop, tumblep, tumblep,  ROT0, "Data East Corporation", "Tumble Pop (World)" )
GAME( 1991, tumblepj, tumblep, tumblepop, tumblep, tumblep,  ROT0, "Data East Corporation", "Tumble Pop (Japan)" )
GAMEX(1991, tumblepb, tumblep, tumblepb,  tumblep, tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 1)", GAME_IMPERFECT_SOUND )
GAMEX(1991, tumblep2, tumblep, tumblepb,  tumblep, tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 2)", GAME_IMPERFECT_SOUND )
