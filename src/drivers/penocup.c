/* Peno Cup?

no idea if this is the real title, I just see a large Peno Cup logo in the 2/3 roms

looks like some kind of ping pong / tennis game?

this is a BAD dump


 ___________________________________________________
|        __     _________ __________   __________  |
|__      |_|   |UM62256D| |UM62256D|   |UM62256D|  |
  |    LN358N  |________| |________|   |________|  |
 _|            _________ _________     __________  |
|_  _1_______  |TI     | |TI     |     |UM62256D|  |
|_  |27C020 |  |52C1HXW| |52ALROW|     |________|  |
|_  |_______|  |       | |       |     __________  |
|__            |       | |       |     |UM62256D|  |
 _| __         |_______| |_______|     |________|  |
|_  |_| 74HC74                                     |
|_  OKI        74LS244   _3_______  _5_______      |
|_  M6295      74LS244   |27C040 |  |27C040 |      |
|_    57C55LK            |_______|  |_______|      |
|_             ________  _2_______  _4_______      |
|_    57C55LK  |GM76C28| |27C040 |  |27C040 |      |
|_             |_______| |_______|  |_______|      |
|_    74LS244  ________                            |
|_             |GM76C28| PAL16L8B PAL16L8B 74LS245 |
|_             |_______|                           |
|_                       74LS244  74LS373N 74LS373 |
|_    74LS244  PIC16C84                            |
|_                       74HC273N 74LS373N 74LS245 |
|_             74HC74B1              _____________ |
  |                            OSC   |NEC V30    | |
 _|   74LS244  74LS14N 74HC74  16MHz |D70116C-10 | |
|__________________________________________________|

The PCB is Spanish and manufacured by Gamart.

ROMs 2, 3, 4 & 5 are bad. :(

*/

/*

maybe its possible to reconstruct most the code from the
3 dumps and emulate the hardware at least

would help if the NEC dasm was fixed...

*/

#include "driver.h"

VIDEO_START(penocup)
{
	return 0;
}

VIDEO_UPDATE(penocup)
{

}


static ADDRESS_MAP_START( penocup_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x20000, 0xfffff) AM_READWRITE(MRA8_BANK1, MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( penocup_io, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END


INPUT_PORTS_START(penocup)
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static INTERRUPT_GEN( penocup_irq ) /* right? */
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( penocup )
	/* basic machine hardware */
	MDRV_CPU_ADD(V30, 8000000)
	MDRV_CPU_PROGRAM_MAP(penocup_map, 0)
	MDRV_CPU_IO_MAP(penocup_io,0)
	MDRV_CPU_VBLANK_INT(penocup_irq,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024,1024)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(penocup)
	MDRV_VIDEO_UPDATE(penocup)
MACHINE_DRIVER_END

ROM_START( penocup )

	/* dump a */
	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "27c040_dump_a.2", 0x000000, 0x080000, BAD_DUMP CRC(791d68c8) SHA1(641c989d50e95ac3ff7c87d148cfab44abbdc774) )
	ROM_LOAD16_BYTE( "27c040_dump_a.3", 0x000001, 0x080000, BAD_DUMP CRC(00c81241) SHA1(899d4d1566f5f5d2967b6a8ec7dca60833846bbe) )
	ROM_LOAD16_BYTE( "27c040_dump_a.4", 0x100000, 0x080000, BAD_DUMP CRC(11af50f6) SHA1(1e5b6cc5c5a6c1ec302b2de7ce40c9ebfb349b46) )
	ROM_LOAD16_BYTE( "27c040_dump_a.5", 0x100001, 0x080000, BAD_DUMP CRC(f6b87231) SHA1(3db461c0858c207e8a3dfd822c99d28e3a26b4ee) )

	/* dump b */
	ROM_REGION( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "27c040_dump_b.2", 0x000000, 0x080000, BAD_DUMP CRC(df1f2618) SHA1(7c6abb7a6ec55c49b95809f003d217f1ea758729) )
	ROM_LOAD16_BYTE( "27c040_dump_b.3", 0x000001, 0x080000, BAD_DUMP CRC(0292ca69) SHA1(fe3b0e78d9e946d8f8a86e8246e5a94483f44ce1) )
	ROM_LOAD16_BYTE( "27c040_dump_b.4", 0x100000, 0x080000, BAD_DUMP CRC(f6bcadc6) SHA1(d8c61c207175d67f4229103696dc2a4447af2ba4) )
	ROM_LOAD16_BYTE( "27c040_dump_b.5", 0x100001, 0x080000, BAD_DUMP CRC(b872747c) SHA1(24d2aa2603a71cdfd3d45608177bb60ab7cfe8a2) )

	/* dump c */
	ROM_REGION( 0x200000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "27c040_dump_c.2", 0x000000, 0x080000, BAD_DUMP CRC(be70adc7) SHA1(fe439caa54856c75ef310e456a7e61b15321031d) )
	ROM_LOAD16_BYTE( "27c040_dump_c.3", 0x000001, 0x080000, BAD_DUMP CRC(8e3b3396) SHA1(f47243041b9283712e34ea58fa2456c35785c5ee) )
	ROM_LOAD16_BYTE( "27c040_dump_c.4", 0x100000, 0x080000, BAD_DUMP CRC(34ab75e9) SHA1(779f03139b336cdc46f4d00bf3fd9e6de79942e2) )
	ROM_LOAD16_BYTE( "27c040_dump_c.5", 0x100001, 0x080000, BAD_DUMP CRC(3e7b3533) SHA1(433439c2b8a8e54bb20fc3c1690d3f183c6fa6f6) )

	/* these were the same in each dump..*/
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* not verified if this is correct yet, seems very empty, maybe protected */
	ROM_LOAD( "pic16c84.rom", 0x000000, 0x4280,  CRC(900f2ef8) SHA1(08f206fe52f413437436e4b0d2b4ec310767446c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "27c020.1", 0x000000, 0x040000,  CRC(e2c4fe95) SHA1(da349035cc348db220a1e12b4c2a6021e2168425) )
ROM_END

static DRIVER_INIT (penocup)
{
	unsigned char *ROM1 = memory_region(REGION_USER1);
	unsigned char *ROM2 = memory_region(REGION_USER2);
	unsigned char *ROM3 = memory_region(REGION_USER3);

	UINT32 count;

	for (count = 0;count <0x200000;count++)
	{
		if ((ROM1[count] != ROM2[count]) || (ROM2[count] != ROM3[count]))
		{
			printf("Non-Matching addr: 0x%06x DumpA: %02x  DumpB: %02x  DumpC: %02x\n", count, ROM1[count],ROM2[count],ROM3[count]);
		}
	}


	cpu_setbank(1,&ROM1[0x120000]);



}

GAMEX(199?, penocup, 0,        penocup, penocup, penocup, ROT0,  "Gamart?", "Peno Cup?", GAME_NOT_WORKING|GAME_NO_SOUND )
