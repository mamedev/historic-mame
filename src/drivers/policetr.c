/***************************************************************************

	P&P Marketing Police Trainer hardware

	driver by Aaron Giles

	Games supported:
		* Police Trainer

	Known bugs:
		* perspective on the floor in some levels is not drawn correctly

***************************************************************************/

#include "driver.h"
#include "cpu/mips/r3000.h"
#include "machine/eeprom.h"
#include "policetr.h"


/* constants */
#define MASTER_CLOCK	48000000


/* global variables */
data32_t *	policetr_rambase;


/* local variables */
static data32_t *rom_base;

static data32_t control_data;

static data32_t bsmt_reg;
static data32_t bsmt_data_bank;
static data32_t bsmt_data_offset;

static data32_t *speedup_data;
static UINT32 last_cycles;
static UINT32 loop_count;



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static void irq5_gen(int param)
{
	cpu_set_irq_line(0, R3000_IRQ5, ASSERT_LINE);
}


static INTERRUPT_GEN( irq4_gen )
{
	cpu_set_irq_line(0, R3000_IRQ4, ASSERT_LINE);
	timer_set(cpu_getscanlinetime(0), 0, irq5_gen);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ32_HANDLER( port0_r )
{
	return readinputport(0) << 16;
}


static READ32_HANDLER( port1_r )
{
	return (readinputport(1) << 16) | (EEPROM_read_bit() << 29);
}


static READ32_HANDLER( port2_r )
{
	return readinputport(2) << 16;
}



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE32_HANDLER( control_w )
{
	// bit $80000000 = BSMT access/ROM read
	// bit $20000000 = toggled every 64 IRQ4's
	// bit $10000000 = ????
	// bit $00800000 = EEPROM data
	// bit $00400000 = EEPROM clock
	// bit $00200000 = EEPROM enable (on 1)

	COMBINE_DATA(&control_data);

	/* handle EEPROM I/O */
	if (!(mem_mask & 0x00ff0000))
	{
		EEPROM_write_bit(data & 0x00800000);
		EEPROM_set_cs_line((data & 0x00200000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);
	}

	/* log any unknown bits */
	if (data & 0x4f1fffff)
		logerror("%08X: control_w = %08X & %08X\n", activecpu_get_previouspc(), data, ~mem_mask);
}



/*************************************
 *
 *	BSMT2000 I/O
 *
 *************************************/

static WRITE32_HANDLER( bsmt2000_reg_w )
{
	if (control_data & 0x80000000)
		BSMT2000_data_0_w(bsmt_reg, data & 0xffff, mem_mask | 0xffff0000);
	else
		COMBINE_DATA(&bsmt_data_offset);
}


static WRITE32_HANDLER( bsmt2000_data_w )
{
	if (control_data & 0x80000000)
		COMBINE_DATA(&bsmt_reg);
	else
		COMBINE_DATA(&bsmt_data_bank);
}


static READ32_HANDLER( bsmt2000_data_r )
{
	return memory_region(REGION_SOUND1)[bsmt_data_bank * 0x10000 + bsmt_data_offset] << 8;
}



/*************************************
 *
 *	Busy loop optimization
 *
 *************************************/

static WRITE32_HANDLER( speedup_w )
{
	COMBINE_DATA(speedup_data);

	/* see if the PC matches */
	if ((activecpu_get_previouspc() & 0x1fffffff) == 0x1fc028ac)
	{
		UINT32 curr_cycles = cpu_gettotalcycles();

		/* if less than 50 cycles from the last time, count it */
		if (curr_cycles - last_cycles < 50)
		{
			loop_count++;

			/* more than 2 in a row and we spin */
			if (loop_count > 2)
				cpu_spinuntil_int();
		}
		else
			loop_count = 0;

		last_cycles = curr_cycles;
	}
}



/*************************************
 *
 *	EEPROM interface/saving
 *
 *************************************/

struct EEPROM_interface eeprom_interface_policetr =
{
	8,				// address bits	8
	16,				// data bits	16
	"*110",			// read			1 10 aaaaaa
	"*101",			// write		1 01 aaaaaa dddddddddddddddd
	"*111",			// erase		1 11 aaaaaa
	"*10000xxxx",	// lock			1 00 00xxxx
	"*10011xxxx"	// unlock		1 00 11xxxx
};


static NVRAM_HANDLER( policetr )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_policetr);
		if (file)	EEPROM_load(file);
	}
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( main_readmem )
	{ 0x00000000, 0x0001ffff, MRA32_RAM },
	{ 0x00400000, 0x00400003, policetr_video_r },
	{ 0x00600000, 0x00600003, bsmt2000_data_r },
	{ 0x00a00000, 0x00a00003, port0_r },
	{ 0x00a20000, 0x00a20003, port1_r },
	{ 0x00a40000, 0x00a40003, port2_r },
	{ 0x1fc00000, 0x1fdfffff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( main_writemem )
	{ 0x00000000, 0x0001ffff, MWA32_RAM, &policetr_rambase },
	{ 0x00200000, 0x0020000f, policetr_video_w },
	{ 0x00500000, 0x00500003, MWA32_NOP },		// copies ROM here at startup, plus checksum
	{ 0x00700000, 0x00700003, bsmt2000_reg_w },
	{ 0x00800000, 0x00800003, bsmt2000_data_w },
	{ 0x00900000, 0x00900003, policetr_palette_offset_w },
	{ 0x00920000, 0x00920003, policetr_palette_data_w },
	{ 0x00a00000, 0x00a00003, control_w },
	{ 0x00e00000, 0x00e00003, MWA32_NOP },		// watchdog???
	{ 0x1fc00000, 0x1fdfffff, MWA32_ROM, &rom_base },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( policetr )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )		// EEPROM read
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x01, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 70, 10, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 70, 10, 0, 255 )
INPUT_PORTS_END




/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ MASTER_CLOCK/2 },
	{ 11 },
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct r3000_config config =
{
	0,		/* 1 if we have an FPU, 0 otherwise */
	4096,	/* code cache size */
	4096	/* data cache size */
};


MACHINE_DRIVER_START( policetr )

	/* basic machine hardware */
	MDRV_CPU_ADD(R3000BE, MASTER_CLOCK/2)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq4_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(policetr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(400, 240)
	MDRV_VISIBLE_AREA(0, 393, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(policetr)
	MDRV_VIDEO_UPDATE(policetr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(BSMT2000, bsmt2000_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( policetr )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for R3000 */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, 0x56b0b00a )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, 0xca664142 )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, 0xe9ccf3a0 )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, 0xf4acf921 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "pt-u113.bin", 0x00000, 0x20000, 0x7b34d366 )
	ROM_LOAD32_BYTE( "pt-u112.bin", 0x00001, 0x20000, 0x57d059c8 )
	ROM_LOAD32_BYTE( "pt-u111.bin", 0x00002, 0x20000, 0xfb5ce933 )
	ROM_LOAD32_BYTE( "pt-u110.bin", 0x00003, 0x20000, 0x40bd6f60 )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, 0xf267f813 )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, 0x75fe850e )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( policetr )
{
	speedup_data = install_mem_write32_handler(0, 0x00000fc8, 0x00000fcb, speedup_w);
	memcpy(rom_base, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1996, policetr, 0, policetr, policetr, policetr, ROT0, "P&P Marketing", "Police Trainer" )
