/***************************************************************************

GI Joe

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"


int gijoe_vh_start(void);
void gijoe_vh_stop(void);
void gijoe_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int cur_control2;
static int init_eeprom_count;

static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static void eeprom_init(void)
{
	EEPROM_init(&eeprom_interface);
	init_eeprom_count = 0;
}

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 2720;
	}
}

static READ16_HANDLER( control1_r )
{
	int res;

	/* bit 8  is EEPROM data */
	/* bit 9  is EEPROM ready */
	/* bit 11 is service button */
	res = (EEPROM_read_bit()<<8) | input_port_0_word_r(0);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7ff;
	}

	return res;
}

static READ16_HANDLER( control2_r )
{
	return cur_control2;
}

static WRITE16_HANDLER( control2_w )
{
	if(ACCESSING_LSB) {
		/* bit 0  is data */
		/* bit 1  is cs (active low) */
		/* bit 2  is clock (active high) */
		/* bit 5  is enable irq 6 */

		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);
		cur_control2 = data;

		/* bit 6 = enable sprite ROM reading */
		K053246_set_OBJCHA_line((data & 0x0040) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static int gijoe_interrupt(void)
{
	switch (cpu_getiloops())
	{
	case 0:
		if (K054157_is_IRQ_enabled())
			return 5;       /* ??? */
	case 1:
		if (K054157_is_IRQ_enabled() && (cur_control2 & 0x0020))
			return 6;       /* ??? */
	}
	return ignore_interrupt();
}



static WRITE16_HANDLER( sound_cmd_w )
{
	if(ACCESSING_LSB) {
		data &= 0xff;
		soundlatch_w(0, data);
		if(!Machine->sample_rate)
			if(data == 0xfc || data == 0xfe)
				soundlatch2_w(0, 0x7f);
	}
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, Z80_IRQ_INT, HOLD_LINE);
}

static READ16_HANDLER( sound_status_r )
{
	return soundlatch2_r(0);
}

static void sound_nmi(void)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x100fff, K053247_word_r },			// Sprites
	{ 0x120000, 0x121fff, K054157_ram_word_r },		// Graphic planes
	{ 0x130000, 0x131fff, K054157_rom_word_r },		// Passthrough to tile roms
	{ 0x180000, 0x18ffff, MRA16_RAM },			// Main RAM.  Spec. 180000-1803ff, 180400-187fff
	{ 0x190000, 0x190fff, MRA16_RAM },
	{ 0x1c0014, 0x1c0015, sound_status_r },
	{ 0x1e0000, 0x1e0001, input_port_2_word_r },
	{ 0x1e0002, 0x1e0003, input_port_3_word_r },
	{ 0x1e4000, 0x1e4001, input_port_1_word_r },
	{ 0x1e4002, 0x1e4003, control1_r },
	{ 0x1e8000, 0x1e8001, control2_r },
	{ 0x1f0000, 0x1f0001, K053246_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x100fff, K053247_word_w },
	{ 0x110000, 0x110007, K053246_word_w },
	{ 0x120000, 0x121fff, K054157_ram_word_w },
	{ 0x130000, 0x131fff, MWA16_ROM },
	{ 0x160000, 0x160007, K054157_b_word_w },
	{ 0x170000, 0x170001, MWA16_NOP },			// Watchdog
	{ 0x180000, 0x18ffff, MWA16_RAM },
	{ 0x190000, 0x190fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x1a0000, 0x1a001f, K053251_lsb_w },
	{ 0x1b0000, 0x1b003f, K054157_word_w },
	{ 0x1c000c, 0x1c000d, sound_cmd_w },
	{ 0x1d0000, 0x1d0001, sound_irq_w },
	{ 0x1e8000, 0x1e8001, control2_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xebff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xfa2f, K054539_0_r },
	{ 0xfc02, 0xfc02, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xebff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xfa2f, K054539_0_w },
	{ 0xfc00, 0xfc00, soundlatch2_w },
MEMORY_END

INPUT_PORTS_START( gijoe )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_DIPNAME( 0x0004, 0x0004, "Bit 2a" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bit 3a" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Bit 4a" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Bit 5a" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Bit 6a" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Bit 7a" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Bit 2b" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Bit 4b" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Bit 5b" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Bit 6b" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Bit 7b" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )  // EEPROM data
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_SPECIAL )  // EEPROM ready (always 1)
	PORT_BITX(0x0800, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_DIPNAME( 0x0004, 0x0004, "Bit 2c" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bit 3c" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Bit 4c" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Bit 5c" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Bit 6c" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Bit 7c" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_SERVICE2 )
	PORT_DIPNAME( 0x0400, 0x0400, "Bit 2d" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Bit 3d" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Bit 4d" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Bit 5d" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Bit 6d" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Bit 7d" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_DIPNAME( 0x0080, 0x0080, "Bit 7e" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_DIPNAME( 0x8000, 0x8000, "Bit 7f" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_DIPNAME( 0x0080, 0x0080, "Bit 7g" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_DIPNAME( 0x8000, 0x8000, "Bit 7h" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K054539interface k054539_interface =
{
	1,			/* 1 chip */
	48000,
	REGION_SOUND1,
	{ sound_nmi }
};

static struct MachineDriver machine_driver_gijoe =
{
	{
		{
			CPU_M68000,
			16000000,	/* Probable */
			readmem, writemem, 0, 0,
			gijoe_interrupt, 2 /* ? */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			8000000,	/* Amuse. z80e? */
			sound_readmem, sound_writemem, 0, 0,
			0, 0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gijoe_vh_start,
	gijoe_vh_stop,
	gijoe_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K054539,
			&k054539_interface
		}
	},

	nvram_handler
};


ROM_START( gijoe )
	ROM_REGION( 0x100000, REGION_CPU1 )
	ROM_LOAD_EVEN( "069eab03.rom", 0x000000,  0x40000, 0xdd2d533f )
	ROM_LOAD_ODD ( "069eab02.rom", 0x000000,  0x40000, 0x6bb11c87 )
	ROM_LOAD_EVEN( "069a12",       0x080000,  0x40000, 0x75a7585c )
	ROM_LOAD_ODD ( "069a11",       0x080000,  0x40000, 0x3153e788 )

	ROM_REGION( 0x010000, REGION_CPU2 )
	ROM_LOAD( "069a01", 0x000000, 0x010000, 0x74172b99 )

	ROM_REGION( 0x200000, REGION_GFX1 )
	ROM_LOAD( "069a10", 0x000000, 0x100000, 0x4c6743ee )
	ROM_LOAD( "069a09", 0x100000, 0x100000, 0xe6e36b05 )

	ROM_REGION( 0x400000, REGION_GFX2 )
	ROM_LOAD( "069a08", 0x000000, 0x100000, 0x325477d4 )
	ROM_LOAD( "069a05", 0x100000, 0x100000, 0xc4ab07ed )
	ROM_LOAD( "069a07", 0x200000, 0x100000, 0xccaa3971 )
	ROM_LOAD( "069a06", 0x300000, 0x100000, 0x63eba8e1 )

	ROM_REGION( 0x200000, REGION_SOUND1 )
	ROM_LOAD( "069a04", 0x000000, 0x200000, 0x11d6dcd6 )
ROM_END

ROM_START( gijoeu )
	ROM_REGION( 0x100000, REGION_CPU1 )
	ROM_LOAD_EVEN("069b03", 0x000000,  0x40000, 0x25ff77d2 )
	ROM_LOAD_ODD ("069b02", 0x000000,  0x40000, 0x31cced1c )
	ROM_LOAD_EVEN("069a12", 0x080000,  0x40000, 0x75a7585c )
	ROM_LOAD_ODD ("069a11", 0x080000,  0x40000, 0x3153e788 )

	ROM_REGION( 0x010000, REGION_CPU2 )
	ROM_LOAD( "069a01", 0x000000, 0x010000, 0x74172b99 )

	ROM_REGION( 0x200000, REGION_GFX1 )
	ROM_LOAD( "069a10", 0x000000, 0x100000, 0x4c6743ee )
	ROM_LOAD( "069a09", 0x100000, 0x100000, 0xe6e36b05 )

	ROM_REGION( 0x400000, REGION_GFX2 )
	ROM_LOAD( "069a08", 0x000000, 0x100000, 0x325477d4 )
	ROM_LOAD( "069a05", 0x100000, 0x100000, 0xc4ab07ed )
	ROM_LOAD( "069a07", 0x200000, 0x100000, 0xccaa3971 )
	ROM_LOAD( "069a06", 0x300000, 0x100000, 0x63eba8e1 )

	ROM_REGION( 0x200000, REGION_SOUND1 )
	ROM_LOAD( "069a04", 0x000000, 0x200000, 0x11d6dcd6 )
ROM_END


static void init_gijoe(void)
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}

GAMEX( 1992, gijoe,  0,     gijoe, gijoe, gijoe, ROT0, "Konami", "GI Joe (World)", GAME_NOT_WORKING )
GAMEX( 1992, gijoeu, gijoe, gijoe, gijoe, gijoe, ROT0, "Konami", "GI Joe (US)", GAME_NOT_WORKING )
