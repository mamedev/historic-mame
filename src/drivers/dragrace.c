/***************************************************************************

Atari Drag Race Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( dragrace );
extern VIDEO_UPDATE( dragrace );

extern UINT8* dragrace_playfield_ram;
extern UINT8* dragrace_position_ram;

static unsigned dragrace_misc_flags;

static int dragrace_gear[2];


static void dragrace_frame_callback(int dummy)
{
	int i;

	for (i = 0; i < 2; i++)
	{
		switch (readinputport(5 + i))
		{
		case 0x01: dragrace_gear[i] = 1; break;
		case 0x02: dragrace_gear[i] = 2; break;
		case 0x04: dragrace_gear[i] = 3; break;
		case 0x08: dragrace_gear[i] = 4; break;
		case 0x10: dragrace_gear[i] = 0; break;
		}
	}
}


static MACHINE_INIT( dragrace )
{
	timer_pulse(cpu_getscanlinetime(0), 0, dragrace_frame_callback);
}


WRITE_HANDLER( dragrace_misc_w )
{
	UINT32 mask = 1 << offset;

	if (data & 1)
	{
		dragrace_misc_flags |= mask;
	}
	else
	{
		dragrace_misc_flags &= ~mask;
	}

	set_led_status(0, dragrace_misc_flags & 0x00008000);
	set_led_status(1, dragrace_misc_flags & 0x80000000);
}


READ_HANDLER( dragrace_input_r )
{
	int val = readinputport(2);

	UINT8 maskA = 1 << (offset % 8);
	UINT8 maskB = 1 << (offset / 8);

	int i;

	for (i = 0; i < 2; i++)
	{
		int in = readinputport(i);

		if (dragrace_gear[i] != 0)
		{
			in &= ~(1 << dragrace_gear[i]);
		}

		if (in & maskA)
		{
			val |= 1 << i;
		}
	}

	return (val & maskB) ? 0xFF : 0x7F;
}


READ_HANDLER( dragrace_steering_r )
{
	int bitA[2];
	int bitB[2];

	int i;

	for (i = 0; i < 2; i++)
	{
		int dial = readinputport(3 + i);

		bitA[i] = ((dial + 1) / 2) & 1;
		bitB[i] = ((dial + 0) / 2) & 1;
	}

	return
		(bitA[0] << 0) | (bitB[0] << 1) |
		(bitA[1] << 2) | (bitB[1] << 3);
}


READ_HANDLER( dragrace_scanline_r )
{
	return (cpu_getscanline() ^ 0xf0) | 0x0f;
}


static MEMORY_READ_START( dragrace_readmem )
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x0800, 0x083f, dragrace_input_r },
	{ 0x0c00, 0x0c00, dragrace_steering_r },
	{ 0x0d00, 0x0d00, dragrace_scanline_r },
	{ 0x1000, 0x1fff, MRA_ROM }, /* program */
	{ 0xf800, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( dragrace_writemem )
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0900, 0x091f, dragrace_misc_w },
	{ 0x0a00, 0x0aff, MWA_RAM, &dragrace_playfield_ram },
	{ 0x0b00, 0x0bff, MWA_RAM, &dragrace_position_ram },
	{ 0x0e00, 0x0e00, MWA_NOP }, /* watchdog (disabled in service mode) */
	{ 0x1000, 0x1fff, MWA_ROM }, /* program */
	{ 0xf800, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( dragrace )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 4 */
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0xc0, 0x80, "Extended Play" )
	PORT_DIPSETTING( 0x00, "6.9 seconds" )
	PORT_DIPSETTING( 0x80, "5.9 seconds" )
	PORT_DIPSETTING( 0x40, "4.9 seconds" )
	PORT_DIPSETTING( 0xc0, "Never" )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 4 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x80, "Number Of Heats" )
	PORT_DIPSETTING( 0xc0, "3" )
	PORT_DIPSETTING( 0x80, "4" )
	PORT_DIPSETTING( 0x00, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* IN0 connects here */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) /* IN1 connects here */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL_V | IPF_PLAYER1, 25, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL_V | IPF_PLAYER2, 25, 10, 0, 0 )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Player 1 Gear 1",  KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1, "Player 1 Gear 2",  KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1, "Player 1 Gear 3",  KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1, "Player 1 Gear 4",  KEYCODE_V, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1, "Player 1 Neutral", KEYCODE_B, IP_JOY_DEFAULT )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Player 2 Gear 1",  KEYCODE_G, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "Player 2 Gear 2",  KEYCODE_H, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "Player 2 Gear 3",  KEYCODE_J, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "Player 2 Gear 4",  KEYCODE_K, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2, "Player 2 Neutral", KEYCODE_L, IP_JOY_DEFAULT )
INPUT_PORTS_END


static struct GfxLayout dragrace_tile_layout1 =
{
	16, 16,   /* width, height */
	0x40,     /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100      /* increment */
};


static struct GfxLayout dragrace_tile_layout2 =
{
	16, 16,   /* width, height */
	0x20,     /* total         */
	2,        /* planes        */
	{         /* plane offsets */
		0x0000, 0x2000
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100      /* increment */
};


static struct GfxDecodeInfo dragrace_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &dragrace_tile_layout1, 0, 4 },
	{ REGION_GFX2, 0, &dragrace_tile_layout2, 8, 2 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( dragrace )
{
	palette_set_color(0, 0xFF, 0xFF, 0xFF);   /* 2 color tiles */
	palette_set_color(1, 0x00, 0x00, 0x00);
	palette_set_color(2, 0x00, 0x00, 0x00);
	palette_set_color(3, 0xFF, 0xFF, 0xFF);
	palette_set_color(4, 0x00, 0x00, 0x00);
	palette_set_color(5, 0x00, 0x00, 0x00);
	palette_set_color(6, 0xFF, 0xFF, 0xFF);
	palette_set_color(7, 0xFF, 0xFF, 0xFF);
	palette_set_color(8, 0xFF, 0xFF, 0xFF);   /* 4 color tiles */
	palette_set_color(9, 0xB0, 0xB0, 0xB0);
	palette_set_color(10,0x5F, 0x5F, 0x5F);
	palette_set_color(11,0x00, 0x00, 0x00);
	palette_set_color(12,0xFF, 0xFF, 0xFF);
	palette_set_color(13,0x5F, 0x5F, 0x5F);
	palette_set_color(14,0xB0, 0xB0, 0xB0);
	palette_set_color(15,0x00, 0x00, 0x00);
}


static MACHINE_DRIVER_START( dragrace )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 12096000 / 12)
	MDRV_CPU_MEMORY(dragrace_readmem, dragrace_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_MACHINE_INIT(dragrace)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(dragrace_gfx_decode_info)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(dragrace)
	MDRV_VIDEO_START(dragrace)
	MDRV_VIDEO_UPDATE(dragrace)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( dragrace )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "8513.c1", 0x1000, 0x0800, CRC(543bbb30) SHA1(646a41d1124c8365f07a93de38af007895d7d263) )
	ROM_LOAD( "8514.a1", 0x1800, 0x0800, CRC(ad218690) SHA1(08ba5f4fa4c75d8dad1a7162888d44b3349cbbe4) )
	ROM_RELOAD(          0xF800, 0x0800 )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )   /* 2 color tiles */
	ROM_LOAD( "8519dr.j0", 0x000, 0x200, CRC(aa221ba0) SHA1(450acbf349d77a790a25f3e303c31b38cc426a38) )
	ROM_LOAD( "8521dr.k0", 0x200, 0x200, CRC(0cb33f12) SHA1(d50cb55391aec03e064eecad1624d50d4c30ccab) )
	ROM_LOAD( "8520dr.r0", 0x400, 0x200, CRC(ee1ae6a7) SHA1(83491095260c8b7c616ff17ec1e888d05620f166) )

	ROM_REGION( 0x800, REGION_GFX2, ROMREGION_DISPOSE )   /* 4 color tiles */
	ROM_LOAD( "8515dr.e0", 0x000, 0x200, CRC(9510a59e) SHA1(aea0782b919279efe55a07007bd55a16f7f59239) )
	ROM_LOAD( "8517dr.h0", 0x200, 0x200, CRC(8b5bff1f) SHA1(fdcd719c66bff7c4b9f3d56d1e635259dd8add61) )
	ROM_LOAD( "8516dr.l0", 0x400, 0x200, CRC(d1e74af1) SHA1(f55a3bfd7d152ac9af128697f55c9a0c417779f5) )
	ROM_LOAD( "8518dr.n0", 0x600, 0x200, CRC(b1369028) SHA1(598a8779982d532c9f34345e793a79fcb29cac62) )
ROM_END


GAMEX( 1977, dragrace, 0, dragrace, dragrace, 0, 0, "Atari", "Drag Race", GAME_NO_SOUND )
