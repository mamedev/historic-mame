/***************************************************************************

Atari Super Bug Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( superbug );
extern VIDEO_UPDATE( superbug );

extern int superbug_car_rot;
extern int superbug_skid_in;
extern int superbug_crash_in;
extern int superbug_arrow_off;
extern int superbug_flash;

extern WRITE_HANDLER( superbug_vert_w );
extern WRITE_HANDLER( superbug_horz_w );

extern UINT8* superbug_alpha_num_ram;
extern UINT8* superbug_playfield_ram;

static int superbug_steer_dir;
static int superbug_steer_flag;
static int superbug_attract;
static int superbug_motor_snd;
static int superbug_crash_snd;
static int superbug_skid_snd;

static UINT8 superbug_gear = 1;
static UINT8 superbug_dial = 0;


static void superbug_frame_callback(int dummy)
{
	signed char delta = readinputport(2) - superbug_dial;

	if (delta < 0)
	{
		superbug_steer_flag = 1;
		superbug_steer_dir = 0;
	}
	if (delta > 0)
	{
		superbug_steer_flag = 1;
		superbug_steer_dir = 1;
	}

	superbug_dial += delta;

	switch (readinputport(3) & 15)
	{
	case 1: superbug_gear = 1; break;
	case 2: superbug_gear = 2; break;
	case 4: superbug_gear = 3; break;
	case 8: superbug_gear = 4; break;
	}

	superbug_arrow_off = 0;
}


static MACHINE_INIT( superbug )
{
	timer_pulse(cpu_getscanlinetime(0), 0, superbug_frame_callback);
}


static READ_HANDLER( superbug_input_r )
{
	UINT8 input0 = readinputport(0);
	UINT8 input7 = readinputport(1);

	if (!superbug_steer_dir)
		input0 |= 0x04;
	if (!superbug_steer_flag)
		input7 |= 0x04;
	if (superbug_skid_in)
		input0 |= 0x40;
	if (superbug_crash_in)
		input7 |= 0x40;
	if (superbug_gear == 1)
		input7 |= 0x02;
	if (superbug_gear == 2)
		input0 |= 0x01;
	if (superbug_gear == 3)
		input7 |= 0x01;

	return
		(((input0 >> offset) & 1) << 0) |
		(((input7 >> offset) & 1) << 7);
}

static READ_HANDLER( superbug_dip_r )
{
	return (readinputport(4) >> (2 * offset)) & 3;
}

static WRITE_HANDLER( superbug_motor_snd_w )
{
	superbug_motor_snd = data & 0xF;
}

static WRITE_HANDLER( superbug_crash_snd_w )
{
	superbug_crash_snd = data >> 4;
}

static WRITE_HANDLER( superbug_skid_snd_w )
{
	superbug_skid_snd = 1;
}

static WRITE_HANDLER( superbug_steer_reset_w )
{
	superbug_steer_flag = 0;
}

static WRITE_HANDLER( superbug_crash_reset_w )
{
	superbug_crash_in = 0;
}

static WRITE_HANDLER( superbug_skid_reset_w )
{
	superbug_skid_in = 0;
}

static WRITE_HANDLER( superbug_asr_w )
{
	/* sound stuff */
}

static WRITE_HANDLER( superbug_arrow_off_w )
{
	superbug_arrow_off = 1;
}

static WRITE_HANDLER( superbug_car_rot_w )
{
	superbug_car_rot = data;
}

static WRITE_HANDLER( superbug_misc_w )
{
	set_led_status(0, offset & 1); /* start button lamp */
	set_led_status(1, offset & 8); /* track select button lamp */

	superbug_attract = offset & 2;
	superbug_flash = offset & 4;

	coin_lockout_w(0, !superbug_attract);
	coin_lockout_w(1, !superbug_attract);
}


static MEMORY_READ_START( superbug_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0200, 0x0207, superbug_input_r },
	{ 0x0240, 0x0243, superbug_dip_r },
	{ 0x0400, 0x041f, MRA_RAM },
	{ 0x0500, 0x05ff, MRA_RAM },
	{ 0x0800, 0x1fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( superbug_writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
	{ 0x0100, 0x0100, superbug_vert_w },
	{ 0x0120, 0x0120, superbug_horz_w },
	{ 0x0140, 0x0140, superbug_crash_reset_w },
	{ 0x0160, 0x0160, superbug_skid_reset_w },
	{ 0x0180, 0x0180, superbug_car_rot_w },
	{ 0x01a0, 0x01a0, superbug_steer_reset_w },
	{ 0x01c0, 0x01c0, watchdog_reset_w },
	{ 0x01e0, 0x01e0, superbug_arrow_off_w },
	{ 0x0220, 0x0220, superbug_asr_w },
	{ 0x0260, 0x026f, superbug_misc_w },
	{ 0x0280, 0x0280, superbug_motor_snd_w },
	{ 0x02a0, 0x02a0, superbug_crash_snd_w },
	{ 0x02c0, 0x02c0, superbug_skid_snd_w },
	{ 0x0400, 0x041f, MWA_RAM, &superbug_alpha_num_ram },
	{ 0x0500, 0x05ff, MWA_RAM, &superbug_playfield_ram },
	{ 0x0800, 0x1fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( superbug )

	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON7, "Hiscore Reset", KEYCODE_H, IP_JOY_DEFAULT )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_BUTTON6, "Track Select", KEYCODE_SPACE, IP_JOY_DEFAULT )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 25, 10, 0, 0 )


	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Gear 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "Gear 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4, "Gear 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5, "Gear 4", KEYCODE_V, IP_JOY_DEFAULT )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ))
	PORT_DIPSETTING( 0x03, DEF_STR( 2C_1C ))
	PORT_DIPSETTING( 0x02, DEF_STR( 1C_1C ))
	PORT_DIPSETTING( 0x01, DEF_STR( 1C_2C ))
	PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0c, 0x08, "Play Time" )
	PORT_DIPSETTING( 0x00, "60 seconds" )
	PORT_DIPSETTING( 0x04, "90 seconds" )
	PORT_DIPSETTING( 0x08, "120 seconds" )
	PORT_DIPSETTING( 0x0c, "150 seconds" )
	PORT_DIPNAME( 0x30, 0x20, "Extended Play" )
	PORT_DIPSETTING( 0x10, "Liberal" )
	PORT_DIPSETTING( 0x20, "Medium" )
	PORT_DIPSETTING( 0x30, "Conservative" )
	PORT_DIPSETTING( 0x00, "Never" )
	PORT_DIPNAME( 0xc0, 0x00, "Language" )
	PORT_DIPSETTING( 0xc0, "German" )
	PORT_DIPSETTING( 0x80, "Spanish" )
	PORT_DIPSETTING( 0x40, "Frensh" )
	PORT_DIPSETTING( 0x00, "English" )

INPUT_PORTS_END


static struct GfxLayout superbug_charlayout =
{
	16, 16, /* width, height */
	32,     /* total         */
	1,      /* planes        */
	{ 0 },	/* plane offsets */
	{
		0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17,
		0x1C, 0x1D, 0x1E, 0x1F, 0x04, 0x05, 0x06, 0x07
	},
	{
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0C0, 0x0E0,
		0x100, 0x120, 0x140, 0x160, 0x180, 0x1A0, 0x1C0, 0x1E0
	},
	0x200    /* increment */
};


static struct GfxLayout superbug_tilelayout =
{
	16, 16, /* width, height */
	64,     /* total         */
	1,      /* planes        */
	{ 0 },  /* plane offsets */
	{
		0x07, 0x06, 0x05, 0x04, 0x0F, 0x0E, 0x0D, 0x0C,
		0x17, 0x16, 0x15, 0x14, 0x1F, 0x1E, 0x1D, 0x1C
	},
	{
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0C0, 0x0E0,
		0x100, 0x120, 0x140, 0x160, 0x180, 0x1A0, 0x1C0, 0x1E0
	},
	0x200   /* increment */
};


static struct GfxLayout superbug_carXYlayout =
{
	32, 32, /* width, height */
	4,      /* total         */
	1,      /* planes        */
	{ 0 },	/* plane offsets */
	{
		0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C,
		0x44, 0x4C, 0x54, 0x5C, 0x64, 0x6C, 0x74, 0x7C,
		0x84, 0x8C, 0x94, 0x9C, 0xA4, 0xAC, 0xB4, 0xBC,
		0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC
	},
	{
		0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700,
		0x0800, 0x0900, 0x0A00, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00,
		0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700,
		0x1800, 0x1900, 0x1A00, 0x1B00, 0x1C00, 0x1D00, 0x1E00, 0x1F00
	},
	0x001    /* increment */
};


static struct GfxLayout superbug_carYXlayout =
{
	32, 32, /* width, height */
	4,      /* total         */
	1,      /* planes        */
	{ 0 },	/* plane offsets */
	{
		0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700,
		0x0800, 0x0900, 0x0A00, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00,
		0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700,
		0x1800, 0x1900, 0x1A00, 0x1B00, 0x1C00, 0x1D00, 0x1E00, 0x1F00
	},
	{
		0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C,
		0x44, 0x4C, 0x54, 0x5C, 0x64, 0x6C, 0x74, 0x7C,
		0x84, 0x8C, 0x94, 0x9C, 0xA4, 0xAC, 0xB4, 0xBC,
		0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC
	},
	0x001    /* increment */
};


static struct GfxDecodeInfo superbug_gfx_decode_info[] =
{
	{ 1, 0, &superbug_charlayout, 0x40, 0x04 },
	{ 2, 0, &superbug_tilelayout, 0x00, 0x40 },
	{ 3, 0, &superbug_carXYlayout, 0x40, 0x02 },
	{ 3, 0, &superbug_carYXlayout, 0x40, 0x02 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( superbug )
{
	int i;

	/* set up palette for the tilemap */

	for (i = 0; i < 64; i++)
	{
		UINT8 color = 0;

		if (!(i & 1))
		{
			if (i & 2) color += 0x5B;
			if (i & 4) color += 0xA4;
		}

		if (i & 8)
		{
			color ^= 0xFF;
		}

		palette_set_color(i, color, color, color);
	}

	/* set up palette for alpha numerics and sprites */

	palette_set_color(64+0, 0x00, 0x00, 0x00);
	palette_set_color(64+1, 0xFF, 0xFF, 0xFF);
	palette_set_color(64+2, 0xFF, 0xFF, 0xFF);
	palette_set_color(64+3, 0x00, 0x00, 0x00);
}


static MACHINE_DRIVER_START( superbug )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6800, 12096000 / 16)
	MDRV_CPU_MEMORY(superbug_readmem, superbug_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse, 540)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(superbug)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(superbug_gfx_decode_info)
	MDRV_PALETTE_LENGTH(64 + 4)

	MDRV_PALETTE_INIT(superbug)
	MDRV_VIDEO_UPDATE(superbug)
	MDRV_VIDEO_START(superbug)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( superbug )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "9121.d1", 0x0800, 0x0800, 0x350df308 )
	ROM_LOAD( "9122.c1", 0x1000, 0x0800, 0xeb6e3e37 )
	ROM_LOAD( "9123.a1", 0x1800, 0x0800, 0xf42c6bbe )
	ROM_RELOAD(          0xF800, 0x0800 )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE ) /* alpha numerics */
	ROM_LOAD( "9124.m3", 0x0000, 0x0400, 0xf8af8dd5 )
	ROM_LOAD( "9471.n3", 0x0400, 0x0400, 0x52250698 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "9126.f5", 0x0000, 0x0400, 0xee695137 )
	ROM_LOAD( "9472.h5", 0x0400, 0x0400, 0x5ddb80ac )
	ROM_LOAD( "9127.e5", 0x0800, 0x0400, 0xbe1386b4 )
	ROM_RELOAD(          0x0C00, 0x0400 )

	ROM_REGION( 0x400, REGION_GFX3, ROMREGION_DISPOSE ) /* car sprites */
	ROM_LOAD( "9125.k6", 0x0000, 0x0400, 0xa3c835df )

	ROM_REGION( 0x100, REGION_PROMS, 0 )                /* sync prom, not used */
	ROM_LOAD( "9114.m5", 0x0000, 0x0100, 0xb8094b4c )

ROM_END


GAMEX( 1977, superbug, 0, superbug, superbug, 0, ROT270, "Atari", "Super Bug",  GAME_NO_SOUND )
