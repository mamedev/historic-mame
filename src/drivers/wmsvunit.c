/*************************************************************************

	Driver for Midway V-Unit games

	driver by Aaron Giles

	Games supported:
		* Cruis'n USA (1994)
		* Cruis'n World (1996)

	In the future:
		* War Gods (1996)

	Known bugs:
		* textures for automatic/manual selection get overwritten in Cruis'n World
		* sound reset not working for all games
		* rendering needs to be looked at a little more closely to fix some holes
		* in Cruis'n World attract mode, right side of sky looks like it has wrapped

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "sndhrdw/williams.h"
#include "wmsvunit.h"


static data32_t *ram_base;
static UINT8 cmos_protected;
static UINT16 control_data;

static UINT8 adc_data;
static UINT8 adc_shift;

static data16_t last_port0;
static UINT8 shifter_state;

static void *timer[2];

static data32_t *tms32031_control;




/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( wmsvunit )
{
	williams_dcs_reset_w(1);
	williams_dcs_reset_w(0);

	cpu_setbank(1, memory_region(REGION_USER1));
	memcpy(ram_base, memory_region(REGION_USER1), 0x20000*4);

	timer[0] = timer_alloc(NULL);
	timer[1] = timer_alloc(NULL);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ32_HANDLER( port0_r )
{
	data16_t val = readinputport(0);
	data16_t diff = val ^ last_port0;

	/* make sure the shift controls are mutually exclusive */
	if ((diff & 0x0400) && !(val & 0x0400))
		shifter_state = (shifter_state == 1) ? 0 : 1;
	if ((diff & 0x0800) && !(val & 0x0800))
		shifter_state = (shifter_state == 2) ? 0 : 2;
	if ((diff & 0x1000) && !(val & 0x1000))
		shifter_state = (shifter_state == 4) ? 0 : 4;
	if ((diff & 0x2000) && !(val & 0x2000))
		shifter_state = (shifter_state == 8) ? 0 : 8;
	last_port0 = val;

	val = (val | 0x3c00) ^ (shifter_state << 10);

	return (val << 16) | val;
}


static READ32_HANDLER( port1_r )
{
	return (readinputport(1) << 16) | readinputport(1);
}


static READ32_HANDLER( port2_r )
{
	return (readinputport(2) << 16) | readinputport(2);
}



/*************************************
 *
 *	ADC input ports
 *
 *************************************/

READ32_HANDLER( wmsvunit_adc_r )
{
	if (!(control_data & 0x40))
		return adc_data << adc_shift;
	else
		logerror("adc_r without enabling reads!\n");
	return 0xffffffff;
}


static void adc_ready(int param)
{
	cpu_set_irq_line(0, 3, ASSERT_LINE);
}


WRITE32_HANDLER( wmsvunit_adc_w )
{
	if (!(control_data & 0x20))
	{
		int which = (data >> adc_shift) - 4;
		if (which < 0 || which > 2)
			logerror("adc_w: unexpected which = %02X\n", which + 4);
		adc_data = readinputport(3 + which);
		timer_set(TIME_IN_MSEC(1), 0, adc_ready);
	}
	else
		logerror("adc_w without enabling writes!\n");
}



/*************************************
 *
 *	CMOS access
 *
 *************************************/

static WRITE32_HANDLER( wmsvunit_cmos_protect_w )
{
	cmos_protected = ((data & 0xc00) != 0xc00);
}


static WRITE32_HANDLER( wmsvunit_cmos_w )
{
	if (!cmos_protected)
	{
		data32_t *cmos = (data32_t *)generic_nvram;
		COMBINE_DATA(&cmos[offset]);
	}
}


static READ32_HANDLER( wmsvunit_cmos_r )
{
	data32_t *cmos = (data32_t *)generic_nvram;
	return cmos[offset];
}



/*************************************
 *
 *	System controls
 *
 *************************************/

WRITE32_HANDLER( wmsvunit_control_w )
{
	UINT16 olddata = control_data;
	COMBINE_DATA(&control_data);

	if ((olddata ^ control_data) & 0x0008)
		watchdog_reset_w(0, 0);
//???	williams_dcs_reset_w((~control_data >> 1) & 1);
	if ((olddata ^ control_data) & ~0x00e8)
		logerror("wmsvunit_control_w: old=%04X new=%04X diff=%04X\n", olddata, control_data, olddata ^ control_data);
}


static WRITE32_HANDLER( wmsvunit_sound_w )
{
	logerror("Sound W = %02X\n", data & 0xff);
	williams_dcs_data_w(data & 0xff);
}



/*************************************
 *
 *	TMS32031 I/O accesses
 *
 *************************************/

READ32_HANDLER( tms32031_control_r )
{
	/* watch for accesses to the timers */
	if (offset == 0x24 || offset == 0x34)
	{
		/* timer is clocked at 100ns */
		int which = (offset >> 4) & 1;
		INT32 result = timer_timeelapsed(timer[which]) * 10000000.;
		return result;
	}

	/* log anything else except the memory control register */
	if (offset != 0x64)
		logerror("%06X:tms32031_control_r(%02X)\n", activecpu_get_pc(), offset);

	return tms32031_control[offset];
}


WRITE32_HANDLER( tms32031_control_w )
{
	COMBINE_DATA(&tms32031_control[offset]);

	/* ignore changes to the memory control register */
	if (offset == 0x64)
		;

	/* watch for accesses to the timers */
	else if (offset == 0x20 || offset == 0x30)
	{
		int which = (offset >> 4) & 1;
		if (data & 0x40)
			timer_adjust(timer[which], TIME_NEVER, 0, TIME_NEVER);
	}
	else
		logerror("%06X:tms32031_control_w(%02X) = %08X\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *	Serial number access
 *
 *************************************/

static READ32_HANDLER( crusnwld_serial_status_r )
{
	int status = wms_serial_status();
	return (port1_r(offset, mem_mask) & 0x7fff7fff) | (status << 31) | (status << 15);
}


static READ32_HANDLER( crusnwld_serial_data_r )
{
	return wms_serial_data_r() << 16;
}


static WRITE32_HANDLER( crusnwld_serial_data_w )
{
	if ((data & 0xf0000) == 0x10000)
		wms_serial_reset();
	wms_serial_data_w(data >> 16);
}



/*************************************
 *
 *	Some kind of protection-like
 *	device
 *
 *************************************/

/* values from offset 3, 6, and 10 must add up to 0x904752a2 */
static UINT16 bit_index;
static UINT32 bit_data[0x10] =
{
	0x3017c636,0x3017c636,0x3017c636,0x3017c636,
	0x3017c636,0x3017c636,0x3017c636,0x3017c636,
	0x3017c636,0x3017c636,0x3017c636,0x3017c636,
	0x3017c636,0x3017c636,0x3017c636,0x3017c636
};


static READ32_HANDLER( bit_data_r )
{
	data32_t *cmos_base = (data32_t *)generic_nvram;
	int bit = (bit_data[bit_index / 32] >> (31 - (bit_index % 32))) & 1;
	bit_index = (bit_index + 1) % 512;
	return bit ? cmos_base[offset] : ~cmos_base[offset];
}


static WRITE32_HANDLER( bit_reset_w )
{
	bit_index = 0;
}



/*************************************
 *
 *	Memory maps
 *
 *************************************/

#define ADDR_RANGE(s,e) ((s)*4), ((e)*4+3)

static MEMORY_READ32_START( readmem )
	{ ADDR_RANGE(0x000000, 0x01ffff), MRA32_RAM },
	{ ADDR_RANGE(0x400000, 0x41ffff), MRA32_RAM },
	{ ADDR_RANGE(0x808000, 0x80807f), tms32031_control_r },
	{ ADDR_RANGE(0x809800, 0x809fff), MRA32_RAM },
	{ ADDR_RANGE(0x900000, 0x97ffff), wmsvunit_videoram_r },
	{ ADDR_RANGE(0x980000, 0x980000), wmsvunit_dma_queue_entries_r },
	{ ADDR_RANGE(0x980020, 0x980020), wmsvunit_scanline_r },
	{ ADDR_RANGE(0x980040, 0x980040), wmsvunit_page_control_r },
	{ ADDR_RANGE(0x980080, 0x980080), MRA32_NOP },
	{ ADDR_RANGE(0x980082, 0x980083), wmsvunit_dma_trigger_r },
	{ ADDR_RANGE(0x990000, 0x990000), MRA32_NOP },	// link PAL (low 4 bits must == 4)
	{ ADDR_RANGE(0x991030, 0x991030), port1_r },
//	{ ADDR_RANGE(0x991050, 0x991050), MRA32_RAM },	// seems to be another port
	{ ADDR_RANGE(0x991060, 0x991060), port0_r },
	{ ADDR_RANGE(0x992000, 0x992000), port2_r },
	{ ADDR_RANGE(0x993000, 0x993000), wmsvunit_adc_r },
	{ ADDR_RANGE(0x997000, 0x997000), MRA32_NOP },	// communications
	{ ADDR_RANGE(0x9c0000, 0x9c1fff), wmsvunit_cmos_r },
	{ ADDR_RANGE(0x9e0000, 0x9e7fff), MRA32_RAM },
	{ ADDR_RANGE(0xa00000, 0xbfffff), wmsvunit_textureram_r },
	{ ADDR_RANGE(0xc00000, 0xffffff), MRA32_BANK1 },
MEMORY_END


static MEMORY_WRITE32_START( writemem )
	{ ADDR_RANGE(0x000000, 0x01ffff), MWA32_RAM, &ram_base },
	{ ADDR_RANGE(0x400000, 0x41ffff), MWA32_RAM },
	{ ADDR_RANGE(0x600000, 0x600000), wmsvunit_dma_queue_w },
	{ ADDR_RANGE(0x808000, 0x80807f), tms32031_control_w, &tms32031_control },
	{ ADDR_RANGE(0x809800, 0x809fff), MWA32_RAM },
	{ ADDR_RANGE(0x900000, 0x97ffff), wmsvunit_videoram_w, (data32_t **)&wmsvunit_videoram },
	{ ADDR_RANGE(0x980020, 0x98002b), wmsvunit_video_control_w },
	{ ADDR_RANGE(0x980040, 0x980040), wmsvunit_page_control_w },
	{ ADDR_RANGE(0x980080, 0x980080), MWA32_NOP },
	{ ADDR_RANGE(0x993000, 0x993000), wmsvunit_adc_w },
	{ ADDR_RANGE(0x994000, 0x994000), wmsvunit_control_w },
	{ ADDR_RANGE(0x995000, 0x995000), MWA32_NOP },	// force feedback?
	{ ADDR_RANGE(0x995020, 0x995020), wmsvunit_cmos_protect_w },
	{ ADDR_RANGE(0x997000, 0x997000), MWA32_NOP },	// link communications
	{ ADDR_RANGE(0x9a0000, 0x9a0000), wmsvunit_sound_w },
	{ ADDR_RANGE(0x9c0000, 0x9c1fff), wmsvunit_cmos_w, (data32_t **)&generic_nvram, &generic_nvram_size },
	{ ADDR_RANGE(0x9e0000, 0x9e7fff), wmsvunit_paletteram_w, &paletteram32 },
	{ ADDR_RANGE(0xa00000, 0xbfffff), wmsvunit_textureram_w, &wmsvunit_textureram },
	{ ADDR_RANGE(0xc00000, 0xffffff), MWA32_ROM },
MEMORY_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( crusnusa )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Enter", KEYCODE_F2, IP_JOY_NONE ) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* 4th */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* 3rd */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* 2nd */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* 1st */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* radio */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* view 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* view 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )	/* view 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )	/* view 4 */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0000, "Link Status" )
	PORT_DIPSETTING(      0x0000, "Master" )
	PORT_DIPSETTING(      0x0001, "Slave" )
	PORT_DIPNAME( 0x0002, 0x0002, "Link???" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Linking" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Freeze" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Cabinet ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Upright ))
	PORT_DIPSETTING(      0x0000, "Sitdown" )
	PORT_DIPNAME( 0x0040, 0x0040, "Enable Motion" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0100, 0x0100, "Coin Counters" )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0xfe00, 0xf800, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0xfe00, "USA-1" )
	PORT_DIPSETTING(      0xfa00, "USA-3" )
	PORT_DIPSETTING(      0xfc00, "USA-7" )
	PORT_DIPSETTING(      0xf800, "USA-8" )
	PORT_DIPSETTING(      0xf600, "Norway-1" )
	PORT_DIPSETTING(      0xee00, "Australia-1" )
	PORT_DIPSETTING(      0xea00, "Australia-2" )
	PORT_DIPSETTING(      0xec00, "Australia-3" )
	PORT_DIPSETTING(      0xe800, "Australia-4" )
	PORT_DIPSETTING(      0xde00, "Swiss-1" )
	PORT_DIPSETTING(      0xda00, "Swiss-2" )
	PORT_DIPSETTING(      0xdc00, "Swiss-3" )
	PORT_DIPSETTING(      0xce00, "Belgium-1" )
	PORT_DIPSETTING(      0xca00, "Belgium-2" )
	PORT_DIPSETTING(      0xcc00, "Belgium-3" )
	PORT_DIPSETTING(      0xbe00, "French-1" )
	PORT_DIPSETTING(      0xba00, "French-2" )
	PORT_DIPSETTING(      0xbc00, "French-3" )
	PORT_DIPSETTING(      0xb800, "French-4" )
	PORT_DIPSETTING(      0xb600, "Hungary-1" )
	PORT_DIPSETTING(      0xae00, "Taiwan-1" )
	PORT_DIPSETTING(      0xaa00, "Taiwan-2" )
	PORT_DIPSETTING(      0xac00, "Taiwan-3" )
	PORT_DIPSETTING(      0x9e00, "UK-1" )
	PORT_DIPSETTING(      0x9a00, "UK-2" )
	PORT_DIPSETTING(      0x9c00, "UK-3" )
	PORT_DIPSETTING(      0x8e00, "Finland-1" )
	PORT_DIPSETTING(      0x7e00, "German-1" )
	PORT_DIPSETTING(      0x7a00, "German-2" )
	PORT_DIPSETTING(      0x7c00, "German-3" )
	PORT_DIPSETTING(      0x7800, "German-4" )
	PORT_DIPSETTING(      0x7600, "Denmark-1" )
	PORT_DIPSETTING(      0x6e00, "Japan-1" )
	PORT_DIPSETTING(      0x6a00, "Japan-2" )
	PORT_DIPSETTING(      0x6c00, "Japan-3" )
	PORT_DIPSETTING(      0x5e00, "Italy-1" )
	PORT_DIPSETTING(      0x5a00, "Italy-2" )
	PORT_DIPSETTING(      0x5c00, "Italy-3" )
	PORT_DIPSETTING(      0x4e00, "Sweden-1" )
	PORT_DIPSETTING(      0x3e00, "Canada-1" )
	PORT_DIPSETTING(      0x3a00, "Canada-2" )
	PORT_DIPSETTING(      0x3c00, "Canada-3" )
	PORT_DIPSETTING(      0x3600, "General-1" )
	PORT_DIPSETTING(      0x3200, "General-3" )
	PORT_DIPSETTING(      0x3400, "General-5" )
	PORT_DIPSETTING(      0x3000, "General-7" )
	PORT_DIPSETTING(      0x2e00, "Austria-1" )
	PORT_DIPSETTING(      0x2a00, "Austria-2" )
	PORT_DIPSETTING(      0x2c00, "Austria-3" )
	PORT_DIPSETTING(      0x2800, "Austria-4" )
	PORT_DIPSETTING(      0x1e00, "Spain-1" )
	PORT_DIPSETTING(      0x1a00, "Spain-2" )
	PORT_DIPSETTING(      0x1c00, "Spain-3" )
	PORT_DIPSETTING(      0x1800, "Spain-4" )
	PORT_DIPSETTING(      0x0e00, "Netherland-1" )

	PORT_START		/* wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 20, 0x10, 0xf0 )

	PORT_START		/* gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 25, 20, 0x00, 0xff )

	PORT_START		/* brake pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2, 25, 20, 0x00, 0xff )
INPUT_PORTS_END



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( wmsvunit )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS32031, 50000000)
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(wmsvunit)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 432)
	MDRV_VISIBLE_AREA(0, 511, 0, 399)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(wmsvunit)
	MDRV_VIDEO_UPDATE(wmsvunit)

	/* sound hardware */
	MDRV_IMPORT_FROM(williams_dcs_sound)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( crusnusa )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy 32C031 region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "cusa.u2",  ADSP2100_SIZE + 0x000000, 0x80000, 0xb9338332 )
	ROM_LOAD( "cusa.u3",  ADSP2100_SIZE + 0x100000, 0x80000, 0xcd8325d6 )
	ROM_LOAD( "cusa.u4",  ADSP2100_SIZE + 0x200000, 0x80000, 0xfab457f3 )
	ROM_LOAD( "cusa.u5",  ADSP2100_SIZE + 0x300000, 0x80000, 0xbecc92f4 )
	ROM_LOAD( "cusa.u6",  ADSP2100_SIZE + 0x400000, 0x80000, 0xa9f915d3 )
	ROM_LOAD( "cusa.u7",  ADSP2100_SIZE + 0x500000, 0x80000, 0x424f0bbc )
	ROM_LOAD( "cusa.u8",  ADSP2100_SIZE + 0x600000, 0x80000, 0x03c28199 )
	ROM_LOAD( "cusa.u9",  ADSP2100_SIZE + 0x700000, 0x80000, 0x24ba6371 )

	ROM_REGION32_LE( 0xa00000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "cusa-l41.u10", 0x000000, 0x80000, 0xeb9372d1 )
	ROM_LOAD32_BYTE( "cusa-l41.u11", 0x000001, 0x80000, 0x76f3cd40 )
	ROM_LOAD32_BYTE( "cusa-l41.u12", 0x000002, 0x80000, 0x9021a376 )
	ROM_LOAD32_BYTE( "cusa-l41.u13", 0x000003, 0x80000, 0x1687c932 )
	ROM_LOAD32_BYTE( "cusa.u14",     0x200000, 0x80000, 0x6a4ae622 )
	ROM_LOAD32_BYTE( "cusa.u15",     0x200001, 0x80000, 0x1a0ad3b7 )
	ROM_LOAD32_BYTE( "cusa.u16",     0x200002, 0x80000, 0x799d4dd6 )
	ROM_LOAD32_BYTE( "cusa.u17",     0x200003, 0x80000, 0x3d68b660 )
	ROM_LOAD32_BYTE( "cusa.u18",     0x400000, 0x80000, 0x9e8193fb )
	ROM_LOAD32_BYTE( "cusa.u19",     0x400001, 0x80000, 0x0bf60cde )
	ROM_LOAD32_BYTE( "cusa.u20",     0x400002, 0x80000, 0xc07f68f0 )
	ROM_LOAD32_BYTE( "cusa.u21",     0x400003, 0x80000, 0xb0264aed )
	ROM_LOAD32_BYTE( "cusa.u22",     0x600000, 0x80000, 0xad137193 )
	ROM_LOAD32_BYTE( "cusa.u23",     0x600001, 0x80000, 0x842449b0 )
	ROM_LOAD32_BYTE( "cusa.u24",     0x600002, 0x80000, 0x0b2275be )
	ROM_LOAD32_BYTE( "cusa.u25",     0x600003, 0x80000, 0x2b9fe68f )
	ROM_LOAD32_BYTE( "cusa.u26",     0x800000, 0x80000, 0xae56b871 )
	ROM_LOAD32_BYTE( "cusa.u27",     0x800001, 0x80000, 0x2d977a8e )
	ROM_LOAD32_BYTE( "cusa.u28",     0x800002, 0x80000, 0xcffa5fb1 )
	ROM_LOAD32_BYTE( "cusa.u29",     0x800003, 0x80000, 0xcbe52c60 )
ROM_END


ROM_START( crusnu40 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy 32C031 region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "cusa.u2",  ADSP2100_SIZE + 0x000000, 0x80000, 0xb9338332 )
	ROM_LOAD( "cusa.u3",  ADSP2100_SIZE + 0x100000, 0x80000, 0xcd8325d6 )
	ROM_LOAD( "cusa.u4",  ADSP2100_SIZE + 0x200000, 0x80000, 0xfab457f3 )
	ROM_LOAD( "cusa.u5",  ADSP2100_SIZE + 0x300000, 0x80000, 0xbecc92f4 )
	ROM_LOAD( "cusa.u6",  ADSP2100_SIZE + 0x400000, 0x80000, 0xa9f915d3 )
	ROM_LOAD( "cusa.u7",  ADSP2100_SIZE + 0x500000, 0x80000, 0x424f0bbc )
	ROM_LOAD( "cusa.u8",  ADSP2100_SIZE + 0x600000, 0x80000, 0x03c28199 )
	ROM_LOAD( "cusa.u9",  ADSP2100_SIZE + 0x700000, 0x80000, 0x24ba6371 )

	ROM_REGION32_LE( 0xa00000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "cusa-l4.u10",  0x000000, 0x80000, 0x7526d8bf )
	ROM_LOAD32_BYTE( "cusa-l4.u11",  0x000001, 0x80000, 0xbfc691b9 )
	ROM_LOAD32_BYTE( "cusa-l4.u12",  0x000002, 0x80000, 0x059c2234 )
	ROM_LOAD32_BYTE( "cusa-l4.u13",  0x000003, 0x80000, 0x39e0ff7d )
	ROM_LOAD32_BYTE( "cusa.u14",     0x200000, 0x80000, 0x6a4ae622 )
	ROM_LOAD32_BYTE( "cusa.u15",     0x200001, 0x80000, 0x1a0ad3b7 )
	ROM_LOAD32_BYTE( "cusa.u16",     0x200002, 0x80000, 0x799d4dd6 )
	ROM_LOAD32_BYTE( "cusa.u17",     0x200003, 0x80000, 0x3d68b660 )
	ROM_LOAD32_BYTE( "cusa.u18",     0x400000, 0x80000, 0x9e8193fb )
	ROM_LOAD32_BYTE( "cusa.u19",     0x400001, 0x80000, 0x0bf60cde )
	ROM_LOAD32_BYTE( "cusa.u20",     0x400002, 0x80000, 0xc07f68f0 )
	ROM_LOAD32_BYTE( "cusa.u21",     0x400003, 0x80000, 0xb0264aed )
	ROM_LOAD32_BYTE( "cusa.u22",     0x600000, 0x80000, 0xad137193 )
	ROM_LOAD32_BYTE( "cusa.u23",     0x600001, 0x80000, 0x842449b0 )
	ROM_LOAD32_BYTE( "cusa.u24",     0x600002, 0x80000, 0x0b2275be )
	ROM_LOAD32_BYTE( "cusa.u25",     0x600003, 0x80000, 0x2b9fe68f )
	ROM_LOAD32_BYTE( "cusa.u26",     0x800000, 0x80000, 0xae56b871 )
	ROM_LOAD32_BYTE( "cusa.u27",     0x800001, 0x80000, 0x2d977a8e )
	ROM_LOAD32_BYTE( "cusa.u28",     0x800002, 0x80000, 0xcffa5fb1 )
	ROM_LOAD32_BYTE( "cusa.u29",     0x800003, 0x80000, 0xcbe52c60 )
ROM_END


ROM_START( crusnu21 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy 32C031 region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "cusa.u2",  ADSP2100_SIZE + 0x000000, 0x80000, 0xb9338332 )
	ROM_LOAD( "cusa.u3",  ADSP2100_SIZE + 0x100000, 0x80000, 0xcd8325d6 )
	ROM_LOAD( "cusa.u4",  ADSP2100_SIZE + 0x200000, 0x80000, 0xfab457f3 )
	ROM_LOAD( "cusa.u5",  ADSP2100_SIZE + 0x300000, 0x80000, 0xbecc92f4 )
	ROM_LOAD( "cusa.u6",  ADSP2100_SIZE + 0x400000, 0x80000, 0xa9f915d3 )
	ROM_LOAD( "cusa.u7",  ADSP2100_SIZE + 0x500000, 0x80000, 0x424f0bbc )
	ROM_LOAD( "cusa.u8",  ADSP2100_SIZE + 0x600000, 0x80000, 0x03c28199 )
	ROM_LOAD( "cusa.u9",  ADSP2100_SIZE + 0x700000, 0x80000, 0x24ba6371 )

	ROM_REGION32_LE( 0xa00000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "cusa-l21.u10", 0x000000, 0x80000, 0xbb759945 )
	ROM_LOAD32_BYTE( "cusa-l21.u11", 0x000001, 0x80000, 0x4d2da096 )
	ROM_LOAD32_BYTE( "cusa-l21.u12", 0x000002, 0x80000, 0x4b66fe5e )
	ROM_LOAD32_BYTE( "cusa-l21.u13", 0x000003, 0x80000, 0xa165359f )
	ROM_LOAD32_BYTE( "cusa.u14",     0x200000, 0x80000, 0x6a4ae622 )
	ROM_LOAD32_BYTE( "cusa.u15",     0x200001, 0x80000, 0x1a0ad3b7 )
	ROM_LOAD32_BYTE( "cusa.u16",     0x200002, 0x80000, 0x799d4dd6 )
	ROM_LOAD32_BYTE( "cusa.u17",     0x200003, 0x80000, 0x3d68b660 )
	ROM_LOAD32_BYTE( "cusa.u18",     0x400000, 0x80000, 0x9e8193fb )
	ROM_LOAD32_BYTE( "cusa.u19",     0x400001, 0x80000, 0x0bf60cde )
	ROM_LOAD32_BYTE( "cusa.u20",     0x400002, 0x80000, 0xc07f68f0 )
	ROM_LOAD32_BYTE( "cusa.u21",     0x400003, 0x80000, 0xb0264aed )
	ROM_LOAD32_BYTE( "cusa.u22",     0x600000, 0x80000, 0xad137193 )
	ROM_LOAD32_BYTE( "cusa.u23",     0x600001, 0x80000, 0x842449b0 )
	ROM_LOAD32_BYTE( "cusa.u24",     0x600002, 0x80000, 0x0b2275be )
	ROM_LOAD32_BYTE( "cusa.u25",     0x600003, 0x80000, 0x2b9fe68f )
	ROM_LOAD32_BYTE( "cusa.u26",     0x800000, 0x80000, 0xae56b871 )
	ROM_LOAD32_BYTE( "cusa.u27",     0x800001, 0x80000, 0x2d977a8e )
	ROM_LOAD32_BYTE( "cusa.u28",     0x800002, 0x80000, 0xcffa5fb1 )
	ROM_LOAD32_BYTE( "cusa.u29",     0x800003, 0x80000, 0xcbe52c60 )
ROM_END


ROM_START( crusnwld )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy 32C031 region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "cwld.u2",  ADSP2100_SIZE + 0x000000, 0x80000, 0x7a233c89 )
	ROM_LOAD( "cwld.u3",  ADSP2100_SIZE + 0x100000, 0x80000, 0xbe9a5ff0 )
	ROM_LOAD( "cwld.u4",  ADSP2100_SIZE + 0x200000, 0x80000, 0x69f02d84 )
	ROM_LOAD( "cwld.u5",  ADSP2100_SIZE + 0x300000, 0x80000, 0x9d0b9071 )
	ROM_LOAD( "cwld.u6",  ADSP2100_SIZE + 0x400000, 0x80000, 0xdf28f492 )
	ROM_LOAD( "cwld.u7",  ADSP2100_SIZE + 0x500000, 0x80000, 0x0128913e )
	ROM_LOAD( "cwld.u8",  ADSP2100_SIZE + 0x600000, 0x80000, 0x5127c08e )
	ROM_LOAD( "cwld.u9",  ADSP2100_SIZE + 0x700000, 0x80000, 0x84cdc781 )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "cwld_l23.u10", 0x0000000, 0x100000, 0x956e0642 )
	ROM_LOAD32_BYTE( "cwld_l23.u11", 0x0000001, 0x100000, 0xb4ed2929 )
	ROM_LOAD32_BYTE( "cwld_l23.u12", 0x0000002, 0x100000, 0xcd12528e )
	ROM_LOAD32_BYTE( "cwld_l23.u13", 0x0000003, 0x100000, 0xb096d211 )
	ROM_LOAD32_BYTE( "cwld.u14",     0x0400000, 0x100000, 0xee815091 )
	ROM_LOAD32_BYTE( "cwld.u15",     0x0400001, 0x100000, 0xe2da7bf1 )
	ROM_LOAD32_BYTE( "cwld.u16",     0x0400002, 0x100000, 0x05a7ad2f )
	ROM_LOAD32_BYTE( "cwld.u17",     0x0400003, 0x100000, 0xd6278c0c )
	ROM_LOAD32_BYTE( "cwld.u18",     0x0800000, 0x100000, 0xe2dc2733 )
	ROM_LOAD32_BYTE( "cwld.u19",     0x0800001, 0x100000, 0x5223a070 )
	ROM_LOAD32_BYTE( "cwld.u20",     0x0800002, 0x100000, 0xdb535625 )
	ROM_LOAD32_BYTE( "cwld.u21",     0x0800003, 0x100000, 0x92a080e8 )
	ROM_LOAD32_BYTE( "cwld.u22",     0x0c00000, 0x100000, 0x77c56318 )
	ROM_LOAD32_BYTE( "cwld.u23",     0x0c00001, 0x100000, 0x6b920fc7 )
	ROM_LOAD32_BYTE( "cwld.u24",     0x0c00002, 0x100000, 0x83485401 )
	ROM_LOAD32_BYTE( "cwld.u25",     0x0c00003, 0x100000, 0x0dad97a9 )
ROM_END


ROM_START( crusnw13 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy 32C031 region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "cwld.u2",  ADSP2100_SIZE + 0x000000, 0x80000, 0x7a233c89 )
	ROM_LOAD( "cwld.u3",  ADSP2100_SIZE + 0x100000, 0x80000, 0xbe9a5ff0 )
	ROM_LOAD( "cwld.u4",  ADSP2100_SIZE + 0x200000, 0x80000, 0x69f02d84 )
	ROM_LOAD( "cwld.u5",  ADSP2100_SIZE + 0x300000, 0x80000, 0x9d0b9071 )
	ROM_LOAD( "cwld.u6",  ADSP2100_SIZE + 0x400000, 0x80000, 0xdf28f492 )
	ROM_LOAD( "cwld.u7",  ADSP2100_SIZE + 0x500000, 0x80000, 0x0128913e )
	ROM_LOAD( "cwld.u8",  ADSP2100_SIZE + 0x600000, 0x80000, 0x5127c08e )
	ROM_LOAD( "cwld.u9",  ADSP2100_SIZE + 0x700000, 0x80000, 0x84cdc781 )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "cwld_l13.u10", 0x0000000, 0x100000, BADCRC( 0x956bac74 ) )
	ROM_LOAD32_BYTE( "cwld_l13.u11", 0x0000001, 0x100000, 0xb0c0a462 )
	ROM_LOAD32_BYTE( "cwld_l13.u12", 0x0000002, 0x100000, 0x5e7c566b )
	ROM_LOAD32_BYTE( "cwld_l13.u13", 0x0000003, 0x100000, 0x46886e9c )
	ROM_LOAD32_BYTE( "cwld.u14",     0x0400000, 0x100000, 0xee815091 )
	ROM_LOAD32_BYTE( "cwld.u15",     0x0400001, 0x100000, 0xe2da7bf1 )
	ROM_LOAD32_BYTE( "cwld.u16",     0x0400002, 0x100000, 0x05a7ad2f )
	ROM_LOAD32_BYTE( "cwld.u17",     0x0400003, 0x100000, 0xd6278c0c )
	ROM_LOAD32_BYTE( "cwld.u18",     0x0800000, 0x100000, 0xe2dc2733 )
	ROM_LOAD32_BYTE( "cwld.u19",     0x0800001, 0x100000, 0x5223a070 )
	ROM_LOAD32_BYTE( "cwld.u20",     0x0800002, 0x100000, 0xdb535625 )
	ROM_LOAD32_BYTE( "cwld.u21",     0x0800003, 0x100000, 0x92a080e8 )
	ROM_LOAD32_BYTE( "cwld.u22",     0x0c00000, 0x100000, 0x77c56318 )
	ROM_LOAD32_BYTE( "cwld.u23",     0x0c00001, 0x100000, 0x6b920fc7 )
	ROM_LOAD32_BYTE( "cwld.u24",     0x0c00002, 0x100000, 0x83485401 )
	ROM_LOAD32_BYTE( "cwld.u25",     0x0c00003, 0x100000, 0x0dad97a9 )
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static DRIVER_INIT( crusnusa )
{
	williams_dcs_init();
	adc_shift = 24;
}


static DRIVER_INIT( crusnwld )
{
	williams_dcs_init();
	adc_shift = 16;

	/* valid values are 450 or 460 */
	wms_serial_generate(450);
	install_mem_read32_handler(0, ADDR_RANGE(0x991030, 0x991030), crusnwld_serial_status_r);
	install_mem_read32_handler(0, ADDR_RANGE(0x996000, 0x996000), crusnwld_serial_data_r);
	install_mem_write32_handler(0, ADDR_RANGE(0x996000, 0x996000), crusnwld_serial_data_w);

	/* install strange protection device */
	install_mem_read32_handler(0, ADDR_RANGE(0x9d0000, 0x9d1fff), bit_data_r);
	install_mem_write32_handler(0, ADDR_RANGE(0x9d0000, 0x9d0000), bit_reset_w);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1994, crusnusa,        0, wmsvunit, crusnusa, crusnusa, ROT0, "Midway", "Cruis'n USA (rev L4.1)" )
GAME( 1994, crusnu40, crusnusa, wmsvunit, crusnusa, crusnusa, ROT0, "Midway", "Cruis'n USA (rev L4.0)" )
GAME( 1994, crusnu21, crusnusa, wmsvunit, crusnusa, crusnusa, ROT0, "Midway", "Cruis'n USA (rev L2.1)" )
GAME( 1996, crusnwld,        0, wmsvunit, crusnusa, crusnwld, ROT0, "Midway", "Cruis'n World (rev L2.3)" )
GAME( 1996, crusnw13, crusnwld, wmsvunit, crusnusa, crusnwld, ROT0, "Midway", "Cruis'n World (rev L1.3)" )
