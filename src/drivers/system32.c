/*
**	V60 + z80
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

static unsigned char irq_status;

static void irq_raise(int level)
{
	logerror("irq: raising %d\n", level);
	irq_status |= (1 << level);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static int irq_callback(int irqline)
{
	int i;
	for(i=7; i>=0; i--)
		if(irq_status & (1 << i)) {
			logerror("irq: taking irq level %d\n", i);
			return i;
		}
	return 0;
}

static WRITE16_HANDLER(irq_ack_w)
{
	if(ACCESSING_MSB) {
		irq_status &= data >> 8;
		logerror("irq: clearing %02x -> %02x\n", data >> 8, irq_status);
		if(!irq_status)
			cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

static void irq_init(void)
{
	irq_status = 0;
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}



static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx",	/* unlock command */
	0
};

static NVRAM_HANDLER( system32 )
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

static READ16_HANDLER(system32_eeprom_r)
{
	return (EEPROM_read_bit() << 7) | input_port_0_r(0);
}

static WRITE16_HANDLER(system32_eeprom_w)
{
	if(ACCESSING_LSB) {
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static READ16_HANDLER(ga2_protection_r)
{
	static const char *prot =
		"wake up! GOLDEN AXE The Revenge of Death-Adder! ";
	fprintf(stderr, "Protection read %06x %04x\n", 0xa00100 + offset*2, mem_mask);
	return prot[offset];
}

static MEMORY_READ16_START( ga2_readmem )
	{ 0x000000, 0x17ffff, MRA16_ROM },
	{ 0x200000, 0x23ffff, MRA16_RAM },
	{ 0x300000, 0x31ffff, MRA16_RAM },
	{ 0x400000, 0x407fff, MRA16_RAM },
	{ 0x608000, 0x60ffff, MRA16_RAM },
	{ 0x700000, 0x701fff, MRA16_RAM },
	{ 0xa00100, 0xa0015f, ga2_protection_r },
	{ 0xc0000a, 0xc0000b, system32_eeprom_r },
	{ 0xf00000, 0xffffff, MRA16_BANK1 }, // High rom mirror
MEMORY_END

static MEMORY_WRITE16_START( ga2_writemem )
	{ 0x000000, 0x17ffff, MWA16_ROM },
	{ 0x200000, 0x23ffff, MWA16_RAM },
	{ 0x300000, 0x31ffff, MWA16_RAM }, // Tilemaps
	{ 0x400000, 0x407fff, MWA16_RAM }, // Sprites
	{ 0x608000, 0x60ffff, MWA16_RAM }, // Palettes
	{ 0x700000, 0x701fff, MWA16_RAM }, // Shared ram with the z80
	{ 0xc00006, 0xc00007, system32_eeprom_w },
	{ 0xd00006, 0xd00007, irq_ack_w },
	{ 0xf00000, 0xffffff, MWA16_ROM },
MEMORY_END

static PORT_READ_START( system32_readport )
PORT_END

static PORT_WRITE_START( system32_writeport )
PORT_END

static MACHINE_INIT( system32 )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();
}

static INTERRUPT_GEN( system32_interrupt )
{
	if(cpu_getiloops())
		irq_raise(1);
	else
		irq_raise(0);
}

INPUT_PORTS_START( ga2 )
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x7c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
INPUT_PORTS_END

ROM_START( ga2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD( "epr14961.b", 0x000000, 0x20000, 0xd9cd8885 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD16_WORD( "epr14958.b", 0x080000, 0x20000, 0x0be324a3 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr15148.b", 0x100000, 0x40000, 0xc477a9fd )
	ROM_LOAD16_BYTE( "epr15147.b", 0x100001, 0x40000, 0x1bb676ea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr14945", 0x00000,  0x10000,  0x4781d4cb )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr14948", 0x000000, 0x200000, 0x75050d4a )
	ROM_LOAD( "mpr14947", 0x200000, 0x200000, 0xb53e62f4 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "mpr14949", 0x000000, 0x200000, 0x152c716c )
	ROM_LOAD( "mpr14951", 0x200000, 0x200000, 0xfdb1a534 )
	ROM_LOAD( "mpr14953", 0x400000, 0x200000, 0x33bd1c15 )
	ROM_LOAD( "mpr14955", 0x600000, 0x200000, 0xe42684aa )
	ROM_LOAD( "mpr14950", 0x800000, 0x200000, 0x15fd0026 )
	ROM_LOAD( "mpr14952", 0xa00000, 0x200000, 0x96f96613 )
	ROM_LOAD( "mpr14954", 0xc00000, 0x200000, 0x39b2ac9e )
	ROM_LOAD( "mpr14956", 0xe00000, 0x200000, 0x298fca50 )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "mpr14944", 0x000000, 0x100000, 0xfd4d4b86 )
	ROM_LOAD( "mpr14942", 0x100000, 0x100000, 0xa89b0e90 )
	ROM_LOAD( "mpr14943", 0x200000, 0x100000, 0x24d40333 )
ROM_END

VIDEO_START( system32 )
{
	return 0;
}

VIDEO_STOP( system32 )
{
}

VIDEO_UPDATE( system32 )
{
	fillbitmap(bitmap, 0, 0);
}

static MACHINE_DRIVER_START( ga2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V60, 8000000) // Reality is 16MHz
	MDRV_CPU_MEMORY(ga2_readmem,ga2_writemem)
	MDRV_CPU_VBLANK_INT(system32_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100 /*DEFAULT_60HZ_VBLANK_DURATION*/)

	MDRV_MACHINE_INIT(system32)
	MDRV_NVRAM_HANDLER(system32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(16384)

	MDRV_VIDEO_START(system32)
	MDRV_VIDEO_UPDATE(system32)
MACHINE_DRIVER_END

GAME( 1992, ga2, 0, ga2, ga2, 0, ROT0, "Sega", "Golden Axe 2" )
