/***************************************************************************

GI Joe

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "state.h"


VIDEO_START( gijoe );
VIDEO_UPDATE( gijoe );

static data16_t cur_control2;
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

#if 0
static void eeprom_init(void)
{
	EEPROM_init(&eeprom_interface);
	init_eeprom_count = 0;
}
#endif

static NVRAM_HANDLER( gijoe )
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
	res = (EEPROM_read_bit()<<8) | input_port_0_word_r(0,0);

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

static INTERRUPT_GEN( gijoe_interrupt )
{
	switch (cpu_getiloops())
	{
	case 0:
		if (K054157_is_IRQ_enabled())
			cpu_set_irq_line(0, 5, HOLD_LINE);       /* ??? */
	case 1:
		if (K054157_is_IRQ_enabled() && (cur_control2 & 0x0020))
			cpu_set_irq_line(0, 6, HOLD_LINE);       /* ??? */
	}
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
	cpu_set_irq_line(1, 0, HOLD_LINE);
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
	{ 0x122000, 0x123fff, K054157_ram_word_w },
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
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_START3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_START4 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )  // EEPROM data
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_SPECIAL )  // EEPROM ready (always 1)
	PORT_BITX(0x0800, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_COIN4 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_SERVICE2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_SERVICE3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_DIPNAME( 0x0080, 0x0000, "Sound" )
	PORT_DIPSETTING(      0x0080, "Mono" )
	PORT_DIPSETTING(      0x0000, "Stereo" )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_DIPNAME( 0x8000, 0x8000, "Coin mechanism" )
	PORT_DIPSETTING(      0x8000, "Common" )
	PORT_DIPSETTING(      0x0000, "Independant" )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_DIPNAME( 0x0080, 0x0000, "Players" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
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
	{ REGION_SOUND1 },
	{ { 100, 100 } },
	{ 0 },
	{ sound_nmi }
};

static MACHINE_DRIVER_START( gijoe )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* Confirmed */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(gijoe_interrupt,2) /* ? */

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Amuse & confirmed. z80e */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	
	MDRV_NVRAM_HANDLER(gijoe)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(gijoe)
	MDRV_VIDEO_UPDATE(gijoe)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END


ROM_START( gijoe )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "069eab03.rom", 0x000000,  0x40000, CRC(dd2d533f) SHA1(6fc9f7a8fc89155ef2b9ee43fe5e456d9b574f8c) )
	ROM_LOAD16_BYTE( "069eab02.rom", 0x000001,  0x40000, CRC(6bb11c87) SHA1(86581d24f73f2e837f1d4fc5f1f2188f610c50b6) )
	ROM_LOAD16_BYTE( "069a12",       0x080000,  0x40000, CRC(75a7585c) SHA1(443d6dee99edbe81ab1b7289e6cad403fe01cc0d) )
	ROM_LOAD16_BYTE( "069a11",       0x080001,  0x40000, CRC(3153e788) SHA1(fde4543eac707ef24b431e64011cf0f923d4d3ac) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "069a01", 0x000000, 0x010000, CRC(74172b99) SHA1(f5e0e0d43317454fdacd3df7cd3035fcae4aef68) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "069a10", 0x000000, 0x100000, CRC(4c6743ee) SHA1(fa94fbfb55955fdb40705e79b49103676961d919) )
	ROM_LOAD( "069a09", 0x100000, 0x100000, CRC(e6e36b05) SHA1(fecad503f2c285b2b0312e888c06dd6e87f95a07) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "069a08", 0x000000, 0x100000, CRC(325477d4) SHA1(140c57b0ac9e5cf702d788f416408a5eeb5d6d3c) )
	ROM_LOAD( "069a05", 0x100000, 0x100000, CRC(c4ab07ed) SHA1(dc806eff00937d9465b1726fae8fdc3022464a28) )
	ROM_LOAD( "069a07", 0x200000, 0x100000, CRC(ccaa3971) SHA1(16989cbbd65fe1b41c4a85fea02ba1e9880818a9) )
	ROM_LOAD( "069a06", 0x300000, 0x100000, CRC(63eba8e1) SHA1(aa318d356c2580765452106ea0d2228273a90523) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "069a04", 0x000000, 0x200000, CRC(11d6dcd6) SHA1(04cbff9f61cd8641db538db809ddf20da29fd5ac) )
ROM_END

ROM_START( gijoeu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE("069b03", 0x000000,  0x40000, CRC(25ff77d2) SHA1(bea2ae975718806698fd35ef1217bd842b2b69ec) )
	ROM_LOAD16_BYTE("069b02", 0x000001,  0x40000, CRC(31cced1c) SHA1(3df1def671966b3c3d8117ac1b68adeeef9d98c0) )
	ROM_LOAD16_BYTE("069a12", 0x080000,  0x40000, CRC(75a7585c) SHA1(443d6dee99edbe81ab1b7289e6cad403fe01cc0d) )
	ROM_LOAD16_BYTE("069a11", 0x080001,  0x40000, CRC(3153e788) SHA1(fde4543eac707ef24b431e64011cf0f923d4d3ac) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "069a01", 0x000000, 0x010000, CRC(74172b99) SHA1(f5e0e0d43317454fdacd3df7cd3035fcae4aef68) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "069a10", 0x000000, 0x100000, CRC(4c6743ee) SHA1(fa94fbfb55955fdb40705e79b49103676961d919) )
	ROM_LOAD( "069a09", 0x100000, 0x100000, CRC(e6e36b05) SHA1(fecad503f2c285b2b0312e888c06dd6e87f95a07) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "069a08", 0x000000, 0x100000, CRC(325477d4) SHA1(140c57b0ac9e5cf702d788f416408a5eeb5d6d3c) )
	ROM_LOAD( "069a05", 0x100000, 0x100000, CRC(c4ab07ed) SHA1(dc806eff00937d9465b1726fae8fdc3022464a28) )
	ROM_LOAD( "069a07", 0x200000, 0x100000, CRC(ccaa3971) SHA1(16989cbbd65fe1b41c4a85fea02ba1e9880818a9) )
	ROM_LOAD( "069a06", 0x300000, 0x100000, CRC(63eba8e1) SHA1(aa318d356c2580765452106ea0d2228273a90523) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "069a04", 0x000000, 0x200000, CRC(11d6dcd6) SHA1(04cbff9f61cd8641db538db809ddf20da29fd5ac) )
ROM_END


static DRIVER_INIT( gijoe )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);

	state_save_register_UINT16("main", 0, "control2", &cur_control2, 1);
}

GAME( 1992, gijoe,  0,     gijoe, gijoe, gijoe, ROT0, "Konami", "GI Joe (World)")
GAME( 1992, gijoeu, gijoe, gijoe, gijoe, gijoe, ROT0, "Konami", "GI Joe (US)")
