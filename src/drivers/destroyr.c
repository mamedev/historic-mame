/***************************************************************************

Atari Destroyer Driver

***************************************************************************/

#include "driver.h"

extern void destroyr_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

extern int destroyr_wavemod;
extern int destroyr_cursor;

extern UINT8* destroyr_major_obj_ram;
extern UINT8* destroyr_minor_obj_ram;
extern UINT8* destroyr_alpha_num_ram;

static int destroyr_potmask[2];
static int destroyr_potsense[2];
static int destroyr_int_flag;
static int destroyr_attract;
static int destroyr_motor_speed;
static int destroyr_noise;

static UINT8* destroyr_zero_page;


static void destroyr_dial_callback(int dial)
{
	/* Analog inputs come from the player's depth control potentiometer.
	   The voltage is compared to a voltage ramp provided by a discrete
	   analog circuit that conditions the VBLANK signal. When the ramp
	   voltage exceeds the input voltage an NMI signal is generated. The
	   computer then reads the VSYNC data functions to tell where the
	   cursor should be located. */

	if (destroyr_potmask[dial])
	{
		cpu_set_nmi_line(0, PULSE_LINE);

		destroyr_potsense[dial] = 1;
	}
}


static void destroyr_frame_callback(int dummy)
{
	timer_set(cpu_getscanlinetime(0), 0, destroyr_frame_callback);

	destroyr_potsense[0] = 0;
	destroyr_potsense[1] = 0;

	/* PCB supports two dials, but cab has only got one */

	timer_set(cpu_getscanlinetime(readinputport(3)), 0, destroyr_dial_callback);
}


static int destroyr_interrupt(void)
{
	if (destroyr_int_flag)
		return ignore_interrupt();

	destroyr_int_flag = 1;

	return interrupt();
}


static void destroyr_init_machine(void)
{
	timer_set(cpu_getscanlinetime(0), 0, destroyr_frame_callback);

	destroyr_zero_page = auto_malloc(0x100);
}


WRITE_HANDLER( destroyr_ram_w )
{
	destroyr_zero_page[offset & 0xff] = data;
}


WRITE_HANDLER( destroyr_misc_w )
{
	/* bits 0 to 2 connect to the sound circuits */

	destroyr_attract = data & 1;
	destroyr_noise = data & 2;
	destroyr_motor_speed = data & 4;
	destroyr_potmask[0] = data & 8;
	destroyr_wavemod = data & 16;
	destroyr_potmask[1] = data & 32;

	coin_lockout_w(0, !destroyr_attract);
	coin_lockout_w(1, !destroyr_attract);
}


WRITE_HANDLER( destroyr_cursor_load_w )
{
	destroyr_cursor = data;

	watchdog_reset_w(offset, data);
}


WRITE_HANDLER( destroyr_interrupt_ack_w )
{
	destroyr_int_flag = 0;
}


WRITE_HANDLER( destroyr_output_w )
{
	offset &= 15;

	switch (offset)
	{
	case 0:
		set_led_status(0, data & 1);
		break;
	case 1:
		set_led_status(1, data & 1); /* no second LED present on cab */
		break;
	case 2:
		/* bit 0 => songate */
		break;
	case 3:
		/* bit 0 => launch */
		break;
	case 4:
		/* bit 0 => explosion */
		break;
	case 5:
		/* bit 0 => sonar */
		break;
	case 6:
		/* bit 0 => high explosion */
		break;
	case 7:
		/* bit 0 => low explosion */
		break;
	case 8:
		destroyr_misc_w(offset, data);
		break;
	default:
		logerror("unmapped output port %d\n", offset);
		break;
	}
}


READ_HANDLER( destroyr_ram_r )
{
	return destroyr_zero_page[offset & 0xff];
}


READ_HANDLER( destroyr_input_r )
{
	offset &= 15;

	if (offset == 0)
	{
		UINT8 ret = readinputport(0);

		if (destroyr_potsense[0])
			ret |= 4;
		if (destroyr_potsense[1])
			ret |= 8;

		return ret;
	}

	if (offset == 1)
	{
		return readinputport(1);
	}

	logerror("unmapped input port %d\n", offset);

	return 0;
}


READ_HANDLER( destroyr_scanline_r )
{
	return cpu_getscanline();
}


static MEMORY_READ_START( destroyr_readmem )
	{ 0x0000, 0x0fff, destroyr_ram_r },
	{ 0x1000, 0x1fff, destroyr_input_r },
	{ 0x2000, 0x2fff, input_port_2_r },
	{ 0x6000, 0x6fff, destroyr_scanline_r },
	{ 0x7000, 0x77ff, MRA_NOP }, /* missing translation ROMs */
	{ 0x7800, 0x7fff, MRA_ROM }, /* program */
	{ 0xf800, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( destroyr_writemem )
	{ 0x0000, 0x0fff, destroyr_ram_w },
	{ 0x1000, 0x1fff, destroyr_output_w },
	{ 0x3000, 0x30ff, MWA_RAM, &destroyr_alpha_num_ram },
	{ 0x4000, 0x401f, MWA_RAM, &destroyr_major_obj_ram },
	{ 0x5000, 0x5000, destroyr_cursor_load_w },
	{ 0x5001, 0x5001, destroyr_interrupt_ack_w },
	{ 0x5002, 0x5007, MWA_RAM, &destroyr_minor_obj_ram },
	{ 0x7000, 0x77ff, MWA_NOP }, /* missing translation ROMs */
	{ 0x7800, 0x7fff, MWA_ROM }, /* program */
	{ 0xf800, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( destroyr )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* call 7400 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* potsense1 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* potsense2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_DIPNAME( 0xc0, 0x80, "Extended Play" )
	PORT_DIPSETTING( 0x40, "1500 points" )
	PORT_DIPSETTING( 0x80, "2500 points" )
	PORT_DIPSETTING( 0xc0, "3500 points" )
	PORT_DIPSETTING( 0x00, "never" )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* actually a lever */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* IN2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, "Play Time" )
	PORT_DIPSETTING( 0x00, "50 seconds" )
	PORT_DIPSETTING( 0x04, "75 seconds" )
	PORT_DIPSETTING( 0x08, "100 seconds" )
	PORT_DIPSETTING( 0x0c, "125 seconds" )
	PORT_DIPNAME( 0x30, 0x00, "Language" ) /* requires translation ROMs */
	PORT_DIPSETTING( 0x30, "German" )
	PORT_DIPSETTING( 0x20, "French" )
	PORT_DIPSETTING( 0x10, "Spanish" )
	PORT_DIPSETTING( 0x00, "English" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN3 */
	PORT_ANALOG( 0xff, 0x00, IPT_PADDLE_V | IPF_REVERSE, 30, 10, 0, 160)
INPUT_PORTS_END


static struct GfxLayout destroyr_alpha_num_layout =
{
	8, 8,     /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80      /* increment */
};

static struct GfxLayout destroyr_minor_object_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
	  0x004, 0x005, 0x006, 0x007, 0x00C, 0x00D, 0x00E, 0x00F,
	  0x014, 0x015, 0x016, 0x017, 0x01C, 0x01D, 0x01E, 0x01F
	},
	{
	  0x000, 0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0,
	  0x100, 0x120, 0x140, 0x160, 0x180, 0x1a0, 0x1c0, 0x1e0
	},
	0x200     /* increment */
};

static struct GfxLayout destroyr_major_object_layout =
{
	64, 16,   /* width, height */
	4,        /* total         */
	2,        /* planes        */
	{ 1, 0 }, /* plane offsets */
	{
	  0x0004, 0x0006, 0x2004, 0x2006, 0x000C, 0x000E, 0x200C, 0x200E,
	  0x0014, 0x0016, 0x2014, 0x2016, 0x001C, 0x001E, 0x201C, 0x201E,
	  0x0024, 0x0026, 0x2024, 0x2026, 0x002C, 0x002E, 0x202C, 0x202E,
	  0x0034, 0x0036, 0x2034, 0x2036, 0x003C, 0x003E, 0x203C, 0x203E,
	  0x0044, 0x0046, 0x2044, 0x2046, 0x004C, 0x004E, 0x204C, 0x204E,
	  0x0054, 0x0056, 0x2054, 0x2056, 0x005C, 0x005E, 0x205C, 0x205E,
	  0x0064, 0x0066, 0x2064, 0x2066, 0x006C, 0x006E, 0x206C, 0x206E,
	  0x0074, 0x0076, 0x2074, 0x2076, 0x007C, 0x007E, 0x207C, 0x207E
	},
	{
	  0x0000, 0x0080, 0x0100, 0x0180, 0x0200, 0x0280, 0x0300, 0x0380,
	  0x0400, 0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780
	},
	0x0800     /* increment */
};

static struct GfxLayout destroyr_waves_layout =
{
	64, 2,    /* width, height */
	2,        /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
	  0x03, 0x02, 0x01, 0x00, 0x0B, 0x0A, 0x09, 0x08,
	  0x13, 0x12, 0x11, 0x10, 0x1B, 0x1A, 0x19, 0x18,
	  0x23, 0x22, 0x21, 0x20, 0x2B, 0x2A, 0x29, 0x28,
	  0x33, 0x32, 0x31, 0x30, 0x3B, 0x3A, 0x39, 0x38,
	  0x43, 0x42, 0x41, 0x40, 0x4B, 0x4A, 0x49, 0x48,
	  0x53, 0x52, 0x51, 0x50, 0x5B, 0x5A, 0x59, 0x58,
	  0x63, 0x62, 0x61, 0x60, 0x6B, 0x6A, 0x69, 0x68,
	  0x73, 0x72, 0x71, 0x70, 0x7B, 0x7A, 0x79, 0x78
	},
	{
	  0x00, 0x80
	},
	0x04     /* increment */
};


static struct GfxDecodeInfo destroyr_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &destroyr_alpha_num_layout, 4, 2 },
	{ REGION_GFX2, 0, &destroyr_minor_object_layout, 4, 2 },
	{ REGION_GFX3, 0, &destroyr_major_object_layout, 0, 4 },
	{ REGION_GFX4, 0, &destroyr_waves_layout, 4, 2 },
	{ -1 } /* end of array */
};


static void destroyr_init_palette(unsigned char *game_palette, unsigned short *game_colortable, const unsigned char *color_prom)
{
	static UINT8 palette[] =
	{
		0x00, 0x00, 0x00,   /* major objects */
		0x55, 0x55, 0x55,
		0xB0, 0xB0, 0xB0,
		0xFF ,0xFF, 0xFF,
		0x00, 0x00, 0x00,   /* alpha numericals, waves, minor objects */
		0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00,   /* cursor */
		0x78, 0x78, 0x78
	};

	memcpy(game_palette, palette, sizeof palette);
}


static const struct MachineDriver machine_driver_destroyr =
{
	/* basic machine hardware */
	{
		{
			CPU_M6800,
			12096000 / 16,
			destroyr_readmem, destroyr_writemem, 0, 0,
			destroyr_interrupt, 4
		}
	},
	60,
	(int) ((22.5 * 1000000) / (262.5 * 60) + 0.5),
	1,
	destroyr_init_machine,

	/* video hardware */
	256, 240, { 0, 255, 0, 239 },
	destroyr_gfx_decode_info,
	8, 0,
	destroyr_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	0,
	0,
	destroyr_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};


ROM_START( destroyr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	               /* program code */
	ROM_LOAD( "30146-01.c3", 0x7800, 0x0800, 0xe560c712 )
	ROM_RELOAD(              0xF800, 0x0800 )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )   /* alpha numericals */
	ROM_LOAD( "30135-01.p4", 0x0000, 0x0400, 0x184824cf )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )   /* minor objects */
	ROM_LOAD( "30132-01.f4", 0x0000, 0x0400, 0xe09d3d55 )

	ROM_REGION( 0x0800, REGION_GFX3, ROMREGION_DISPOSE )   /* major objects */
	ROM_LOAD( "30134-01.p8", 0x0000, 0x0400, 0x6259e007 )
	ROM_LOAD( "30133-01.n8", 0x0400, 0x0400, 0x108d3e2c )

	ROM_REGION( 0x0020, REGION_GFX4, ROMREGION_DISPOSE )   /* waves */
	ROM_LOAD( "30136-01.k2", 0x0000, 0x0020, BADCRC( 0xe6741ac3 ))

	ROM_REGION( 0x0100, REGION_USER1, 0 )                  /* sync (unused) */
	ROM_LOAD( "30131-01.m1", 0x0000, 0x0100, 0xb8094b4c )
ROM_END


GAMEX( 1977, destroyr, 0, destroyr, destroyr, 0, ORIENTATION_FLIP_X, "Atari", "Destroyer", GAME_NO_SOUND )
