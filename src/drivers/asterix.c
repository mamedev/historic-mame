/***************************************************************************

Asterix

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"

int asterix_vh_start(void);
void asterix_vh_stop(void);
void asterix_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
WRITE16_HANDLER( asterix_spritebank_w );

static unsigned char cur_control2;
static int init_eeprom_count;

static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"111000",		/*  read command */
	"111100",		/* write command */
	"1100100000000",/* erase command */
	"1100000000000",/* lock command */
	"1100110000000" /* unlock command */
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
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( control1_r )
{
	int res;

	/* bit 8  is EEPROM data */
	/* bit 9  is EEPROM ready */
	/* bit 10 is service button */
	res = (EEPROM_read_bit()<<8) | input_port_1_word_r(0,0);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xfbff;
	}

	return res;
}



static READ16_HANDLER( control2_r )
{
	return cur_control2;
}

static WRITE16_HANDLER( control2_w )
{
	if (ACCESSING_LSB)
	{
		cur_control2 = data;
		/* bit 0 is data */
		/* bit 1 is cs (active low) */
		/* bit 2 is clock (active high) */

		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

		/* bit 5 is select tile bank */
		K054157_set_tile_bank((data & 0x20) >> 5);
	}
}

static int asterix_interrupt(void)
{
	if (K054157_is_IRQ_enabled())
		return 5;       /* ??? All irqs have the same vector, and the
                           mask used is 0 or 7 */
	return ignore_interrupt();
}

static READ16_HANDLER( asterix_sound_r )
{
	return K053260_0_r(2 + offset);
}

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static WRITE_HANDLER( sound_arm_nmi_w )
{
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(5),0,nmi_callback);
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

// Check the routine at 7f30 in the ead version.
// You're not supposed to laugh.
// This emulation is grossly overkill but hey, I'm having fun.

static data16_t prot[2];

static WRITE16_HANDLER( protection_w )
{
	COMBINE_DATA(prot+offset);

	if (offset == 1)
	{
		UINT32 cmd = (prot[0] << 16) | prot[1];
		switch (cmd >> 24)
		{
		case 0x64:
		{
			UINT32 param1 = (cpu_readmem24bew_word(cmd & 0xffffff) << 16)
				| cpu_readmem24bew_word((cmd & 0xffffff) + 2);
			UINT32 param2 = (cpu_readmem24bew_word((cmd & 0xffffff) + 4) << 16)
				| cpu_readmem24bew_word((cmd & 0xffffff) + 6);

			switch (param1 >> 24)
			{
			case 0x22:
			{
				int size = param2 >> 24;
				param1 &= 0xffffff;
				param2 &= 0xffffff;
				while(size >= 0)
				{
					cpu_writemem24bew_word(param2, cpu_readmem24bew_word(param1));
					param1 += 2;
					param2 += 2;
					size--;
				}
				break;
			}
			}
			break;
		}
		}
	}
}



static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x107fff, MRA16_RAM },			// Main RAM.
	{ 0x180000, 0x1807ff, K053245_word_r },		// Sprites
	{ 0x180800, 0x180fff, MRA16_RAM },
	{ 0x200000, 0x20000f, K053244_word_r },
	{ 0x280000, 0x280fff, MRA16_RAM },
	{ 0x300000, 0x30001f, K053244_lsb_r },
	{ 0x380000, 0x380001, input_port_0_word_r },
	{ 0x380002, 0x380003, control1_r },
	{ 0x380200, 0x380203, asterix_sound_r },	// 053260
	{ 0x380600, 0x380601, MRA16_NOP },			// Watchdog
	{ 0x400000, 0x400fff, K054157_ram_half_word_r },	// Graphic planes
	{ 0x420000, 0x421fff, K054157_rom_word_r },		// Passthrough to tile roms
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x180000, 0x1807ff, K053245_word_w },
	{ 0x180800, 0x180fff, MWA16_RAM },	// extra RAM, or mirror for the above?
	{ 0x200000, 0x20000f, K053244_word_w },
	{ 0x280000, 0x280fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x300000, 0x30001f, K053244_lsb_w },
	{ 0x380100, 0x380101, control2_w },
	{ 0x380200, 0x380203, K053260_0_lsb_w },
	{ 0x380300, 0x380301, sound_irq_w },
	{ 0x380400, 0x380401, asterix_spritebank_w },
	{ 0x380500, 0x38051f, K053251_lsb_w },
	{ 0x380600, 0x380601, MWA16_NOP },			// Watchdog
	{ 0x380700, 0x380707, K054157_b_word_w },
	{ 0x380800, 0x380803, protection_w },
	{ 0x400000, 0x400fff, K054157_ram_half_word_w },
	{ 0x440000, 0x44003f, K054157_word_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfa00, 0xfa2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa2f, K053260_0_w },
	{ 0xfc00, 0xfc00, sound_arm_nmi_w },
	{ 0xfe00, 0xfe00, YM2151_register_port_0_w },
MEMORY_END



INPUT_PORTS_START( asterix )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )  // EEPROM data
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNUSED )  // EEPROM ready (always 1)
	PORT_BITX(0x0400, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xf800, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct K053260_interface k053260_interface =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(70,MIXER_PAN_LEFT), MIXER(70,MIXER_PAN_RIGHT) } },
	{ 0 }
};

static struct MachineDriver machine_driver_asterix =
{
	{
		{
			CPU_M68000,
			12000000,
			readmem, writemem, 0, 0,
			asterix_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			8000000,
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
	2048, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_NEEDS_6BITS_PER_GUN,
	0,
	asterix_vh_start,
	asterix_vh_stop,
	asterix_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface
		}
	},

	nvram_handler
};


ROM_START( asterix )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "aster8c.bin", 0x000000,  0x20000, 0x61d6621d )
	ROM_LOAD16_BYTE( "aster8d.bin", 0x000001,  0x20000, 0x53aac057 )
	ROM_LOAD16_BYTE( "aster7c.bin", 0x080000,  0x20000, 0x8223ebdc )
	ROM_LOAD16_BYTE( "aster7d.bin", 0x080001,  0x20000, 0x9f351828 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "aster5f.bin", 0x000000, 0x010000,  0xd3d0d77b  )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "aster16k.bin", 0x000000, 0x080000, 0xb9da8e9c )
	ROM_LOAD( "aster12k.bin", 0x080000, 0x080000, 0x7eb07a81 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "aster7k.bin", 0x000000, 0x200000, 0xc41278fe )
	ROM_LOAD( "aster3k.bin", 0x200000, 0x200000, 0x32efdbc4 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "aster1e.bin", 0x000000, 0x200000, 0x6df9ec0e )
ROM_END

ROM_START( astrxeac )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "asterix.8c",  0x000000,  0x20000, 0x0ccd1feb )
	ROM_LOAD16_BYTE( "asterix.8d",  0x000001,  0x20000, 0xb0805f47 )
	ROM_LOAD16_BYTE( "aster7c.bin", 0x080000,  0x20000, 0x8223ebdc )
	ROM_LOAD16_BYTE( "aster7d.bin", 0x080001,  0x20000, 0x9f351828 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "aster5f.bin", 0x000000, 0x010000,  0xd3d0d77b  )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "aster16k.bin", 0x000000, 0x080000, 0xb9da8e9c )
	ROM_LOAD( "aster12k.bin", 0x080000, 0x080000, 0x7eb07a81 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "aster7k.bin", 0x000000, 0x200000, 0xc41278fe )
	ROM_LOAD( "aster3k.bin", 0x200000, 0x200000, 0x32efdbc4 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "aster1e.bin", 0x000000, 0x200000, 0x6df9ec0e )
ROM_END

ROM_START( astrxeaa )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "068eaa01.8c", 0x000000,  0x20000, 0x85b41d8e )
	ROM_LOAD16_BYTE( "068eaa02.8d", 0x000001,  0x20000, 0x8e886305 )
	ROM_LOAD16_BYTE( "aster7c.bin", 0x080000,  0x20000, 0x8223ebdc )
	ROM_LOAD16_BYTE( "aster7d.bin", 0x080001,  0x20000, 0x9f351828 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "aster5f.bin", 0x000000, 0x010000,  0xd3d0d77b  )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "aster16k.bin", 0x000000, 0x080000, 0xb9da8e9c )
	ROM_LOAD( "aster12k.bin", 0x080000, 0x080000, 0x7eb07a81 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "aster7k.bin", 0x000000, 0x200000, 0xc41278fe )
	ROM_LOAD( "aster3k.bin", 0x200000, 0x200000, 0x32efdbc4 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "aster1e.bin", 0x000000, 0x200000, 0x6df9ec0e )
ROM_END


static void init_asterix(void)
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);

#if 0
	*(data16_t *)(memory_region(REGION_CPU1) + 0x07f34) = 0x602a;
	*(data16_t *)(memory_region(REGION_CPU1) + 0x00008) = 0x0400;
#endif
}


GAME( 1992, asterix,  0,       asterix, asterix, asterix, ROT0, "Konami", "Asterix (World ver. EAD)" )
GAME( 1992, astrxeac, asterix, asterix, asterix, asterix, ROT0, "Konami", "Asterix (World ver. EAC)" )
GAME( 1992, astrxeaa, asterix, asterix, asterix, asterix, ROT0, "Konami", "Asterix (World ver. EAA)" )
