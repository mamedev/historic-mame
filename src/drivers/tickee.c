/***************************************************************************

	Raster Elite Tickee Tickats hardware

	driver by Aaron Giles

	Games supported:
		* Tickee Tickats

	Known bugs:
		* gun sometimes misfires

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "machine/ticket.h"
#include "vidhrdw/tlc34076.h"
#include "tickee.h"


data16_t *tickee_control;


static data16_t *code_rom;


/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( tickee )
{
	/* mirror the ROM into bank 1 */
	cpu_setbank(1, code_rom);

	ticket_dispenser_init(100, 0, 1);

	tlc34076_reset(6);
}



/*************************************
 *
 *	Miscellaneous control bits
 *
 *************************************/

static READ8_HANDLER( port1_r )
{
	return input_port_1_r(offset) | (ticket_dispenser_0_r(0) >> 5) | (ticket_dispenser_1_r(0) >> 6);
}



/*************************************
 *
 *	Miscellaneous control bits
 *
 *************************************/

static WRITE16_HANDLER( tickee_control_w )
{
	data16_t olddata = tickee_control[offset];

	/* offsets:

		2 = palette flash (0 normally, 1 when trigger is pressed)
		3 = ticket motor (bit 3 = 0 for left motor, bit 2 = 0 for right motor)
		6 = lamps? (changing all the time)
	*/

	COMBINE_DATA(&tickee_control[offset]);

	if (offset == 3)
	{
		ticket_dispenser_0_w(0, (data & 8) << 4);
		ticket_dispenser_1_w(0, (data & 4) << 5);
	}

	if (olddata != tickee_control[offset])
		logerror("%08X:tickee_control_w(%d) = %04X (was %04X)\n", activecpu_get_pc(), offset, tickee_control[offset], olddata);
}



/*************************************
 *
 *	Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x02000000, 0x02ffffff) AM_READ(MRA16_BANK1)
	AM_RANGE(0x04000000, 0x04003fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x04100000, 0x041000ff) AM_READ(tlc34076_lsb_r)
	AM_RANGE(0x04200000, 0x0420000f) AM_READ(AY8910_read_port_0_lsb_r)
	AM_RANGE(0x04200100, 0x0420010f) AM_READ(AY8910_read_port_1_lsb_r)
	AM_RANGE(0x04400040, 0x0440004f) AM_READ(input_port_3_word_r)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READ(tms34010_io_register_r)
	AM_RANGE(0xff000000, 0xffffffff) AM_READ(MRA16_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_WRITE(MWA16_RAM) AM_BASE(&tickee_vram)
	AM_RANGE(0x04000000, 0x04003fff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x04100000, 0x041000ff) AM_WRITE(tlc34076_lsb_w)
	AM_RANGE(0x04200000, 0x0420000f) AM_WRITE(AY8910_control_port_0_lsb_w)
	AM_RANGE(0x04200010, 0x0420001f) AM_WRITE(AY8910_write_port_0_lsb_w)
	AM_RANGE(0x04200100, 0x0420010f) AM_WRITE(AY8910_control_port_1_lsb_w)
	AM_RANGE(0x04200110, 0x0420011f) AM_WRITE(AY8910_write_port_1_lsb_w)
	AM_RANGE(0x04400000, 0x0440007f) AM_WRITE(tickee_control_w) AM_BASE(&tickee_control)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_WRITE(tms34010_io_register_w)
	AM_RANGE(0xc0000240, 0xc000025f) AM_WRITE(MWA16_NOP)		/* seems to be a bug in their code */
	AM_RANGE(0xff000000, 0xffffffff) AM_WRITE(MWA16_ROM) AM_BASE(&code_rom)
ADDRESS_MAP_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( tickee )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Game Time/Diff" )
	PORT_DIPSETTING(    0x03, "Very Fast/Very Easy" )
	PORT_DIPSETTING(    0x02, "Fast/Easy" )
	PORT_DIPSETTING(    0x01, "Average/Hard" )
	PORT_DIPSETTING(    0x00, "Slow/Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, "Last Box Tickets" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "25" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x30, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ))

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* right ticket status */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* left ticket status */
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	2,
	40000000/16,
	{ 50, 50 },
	{ input_port_0_r, port1_r },
	{ input_port_2_r, 0 },
	{ 0 },
	{ 0 }
};



/*************************************
 *
 *	34010 configuration
 *
 *************************************/

static struct tms34010_config cpu_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	NULL,							/* write to shiftreg function */
	NULL,							/* read from shiftreg function */
	NULL,							/* display address changed */
	NULL							/* display interrupt callback */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( tickee )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(cpu_config)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (232 - 200)) / (60 * 232))

	MDRV_MACHINE_INIT(tickee)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(tickee)
	MDRV_VIDEO_UPDATE(tickee)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( tickee )
	ROM_REGION( TOBYTE(0x800000), REGION_CPU1, 0 )		/* 34010 dummy region */

	ROM_REGION16_LE( 0x200000, REGION_USER1, ROMREGION_DISPOSE )	/* 34010 code */
	ROM_LOAD16_BYTE( "3.ic4",  0x000000, 0x80000, CRC(5b1e399c) SHA1(681608f06bbaf3d258e9f4768a8a6c5047ad08ec) )
	ROM_LOAD16_BYTE( "2.ic3",  0x000001, 0x80000, CRC(1b26d4bb) SHA1(40266ec0fe5897eba85072e5bb39973d34f97546) )
	ROM_LOAD16_BYTE( "1.ic2",  0x100000, 0x80000, CRC(f7f0309e) SHA1(4a93e0e203f5a340a56b770a40b9ab00e131644d) )
	ROM_LOAD16_BYTE( "4.ic5",  0x100001, 0x80000, CRC(ceb0f559) SHA1(61923fe09e1dfde1eaae297ccbc672bc74a70397) )
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static DRIVER_INIT( tickee )
{
	/* set up code ROMs */
	memcpy(code_rom, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1994, tickee, 0, tickee, tickee, tickee, ROT0, "Raster Elite", "Tickee Tickats" )
