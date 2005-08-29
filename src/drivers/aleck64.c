/* 'Aleck64' and similar boards */
/* N64 based hardware */
/* Skeleton Driver */

/*

If you want to boot eleven beat on any n64 emu ?(tested on nemu, 1964
and project64) patch the rom :
write 0 to offset $67b,$67c,$67d,$67e

*/

/*

Eleven Beat World Tournament
Hudson Soft, 1998

This game runs on Nintendo 64-based hardware which is manufactured by Seta.
It's very similar to the hardware used for 'Magical Tetris Featuring Mickey'
(Seta E90 main board) except the game software is contained in a cart that
plugs into a slot on the main board. The E92 board also has more RAM than
the E90 board.
The carts are not compatible with standard N64 console carts.

PCB Layout
----------

          Seta E92 Mother PCB
         |---------------------------------------------|
       --|     VOL_POT                                 |
       |R|TA8139S                                      |
  RCA  --|  TA8201         BU9480                      |
 AUDIO   |                                             |
 PLUGS --|           AMP-NUS                           |
       |L|                                             |
       --|                 BU9480                      |
         |  TD62064                                    |
         |           UPD555  4050                      |
         |                                             |
         |    AD813  DSW1    TC59S1616AFT-10           |
         |J          DSW2    TC59S1616AFT-10           |
         |A       4.9152MHz                            |
         |M                                            |
         |M                                            |
         |A                    SETA                    |
         |                     ST-0043                 |
         |      SETA                          NINTENDO |
         |      ST-0042                       CPU-NUS A|
         |                                             |
         |                                             |
         |                                             |
         |                     14.31818MHz             |
         |X                                            |
         |   MAX232                            NINTENDO|
         |X          RDRAM18-NUS  RDRAM18-NUS  RCP-NUS |
         |           RDRAM18-NUS  RDRAM18-NUS          |
         |X   LVX125                                   |
         |                     14.705882MHz            |
         |X  PIF-NUS                                   |
         |            -------------------------------  |
         |   O        |                             |  |
         |            -------------------------------  |
         |---------------------------------------------|

Notes:
      Hsync      : 15.73kHz
      VSync      : 60Hz
      O          : Push-button reset switch
      X          : Connectors for special (Aleck64) digital joysticks
      CPU-NUS A  : Labelled on the PCB as "VR4300"

The cart contains:
                   CIC-NUS-5101: Boot protection chip
                   BK4D-NUS    : Similar to the save chip used in N64 console carts
                   NUS-ZHAJ.U3 : 64Mbit 28 pin DIP serial MASKROM

      - RCA audio plugs output stereo sound. Regular mono sound is output
        via the standard JAMMA connector also.

      - ALL components are listed for completeness. However, many are power or
        logic devices that most people need not be concerned about :-)

*/



/*

Magical Tetris Challenge Featuring Mickey
Capcom, 1998

This game runs on Nintendo 64-based hardware which is manufactured
by Seta. On bootup, it has the usual Capcom message....


Magical Tetris Challenge
        Featuring Mickey

       981009

        JAPAN


On bootup, they also mention 'T.L.S' (Temporary Landing System), which seems
to be the hardware system, designed by Arika Co. Ltd.


PCB Layout
----------

          Seta E90 Main PCB Rev. B
         |--------------------------------------------|
       --|     VOL_POT                       *        |
       |R|TA8139S                        TET-01M.U5   |
  RCA  --|  TA8201         BU9480                     |
 AUDIO   |                                SETA        |
 PLUGS --|           AMP-NUS              ST-0039     |
       |L|                      42.95454MHz           |
       --|                 BU9480                     |
         |  TD62064                   QS32X384        |
         |           UPD555  4050            QS32X384 |
         |                                            |
         |    AD813                                   |
         |J                                           |
         |A            4.9152MHz                      |
         |M                                           |
         |M                                           |
         |A                    SETA                   |
         |      SETA           ST-0035                |
         |      ST-0042                    NINTENDO   |
         |                     MX8330      CPU-NUS A  |
         |                     14.31818MHz            |
         |                                            |
         |X       AT24C01.U34  NINTENDO               |
         |                     RDRAM18-NUS            |
         |X                                           |
         |   MAX232            NINTENDO     NINTENDO  |
         |X          LT1084    RDRAM18-NUS  RCP-NUS   |
         |    LVX125     MX8330                       |
         |X  PIF-NUS       14.31818MHz                |
         |   O   CIC-NUS-5101  BK4D-NUS   NUS-CZAJ.U4 |
         |--------------------------------------------|

Notes:
      Hsync      : 15.73kHz
      VSync      : 60Hz
      *          : Unpopulated socket for 8M - 32M 42 pin DIP MASKROM
      O          : Push-button reset switch
      X          : Connectors for special (Aleck64?) digital joysticks
      CPU-NUS A  : Labelled on the PCB as "VR4300"
      BK4D-NUS   : Similar to the save chip used in N64 console carts

      ROMs
      ----
      TET-01M.U5 : 8Mbit 42 pin MASKROM
      NUS-CZAJ.U4: 128Mbit 28 pin DIP serial MASKROM
      AT24C01.U34: 128bytes x 8 bit serial EEPROM

      - RCA audio plugs output stereo sound. Regular mono sound is output
        via the standard JAMMA connector also.

      - ALL components are listed for completeness. However, many are power or
        logic devices that most people need not be concerned about :-)

      - The Seta/N64 Aleck64 hardware is similar also, but instead of the hich capacity
        serial MASKROM being on the main board, it's in a cart that plugs into a slot.

*/

#include "driver.h"
#include "cpu/mips/mips3.h"

/* video */
VIDEO_START(aleck64)
{
	return 0;
}

VIDEO_UPDATE(aleck64)
{

}

static ADDRESS_MAP_START( aleck64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

INPUT_PORTS_START( aleck64 )
INPUT_PORTS_END

/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	16384				/* data cache size */
};

MACHINE_DRIVER_START( aleck64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 14318180*4)  /* actually VR4300 ?? speed */
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(aleck64_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024, 1024)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(aleck64)
	MDRV_VIDEO_UPDATE(aleck64)
MACHINE_DRIVER_END

DRIVER_INIT( aleck64 )
{
	UINT8 *ROM = memory_region(REGION_USER1);
	memory_set_bankptr(1,&ROM[0x000000]);
}

ROM_START( 11beat )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x0800000, REGION_USER1, 0 )
	ROM_LOAD ( "nus-zhaj.u3", 0x000000, 0x0800000,  CRC(95258ba2) SHA1(0299b8fb9a8b1b24428d0f340f6bf1cfaf99c672) )
ROM_END

ROM_START( mtetrisc )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x1000000, REGION_USER1, 0 )
	ROM_LOAD ( "nus-zcaj.u4", 0x000000, 0x1000000,  CRC(ec4563fc) SHA1(4d5a30873a5850cf4cd1c0bdbe24e1934f163cd0) )

	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD ( "tet-01m.u5", 0x000000, 0x100000,  CRC(f78f859b) SHA1(b07c85e0453869fe43792f42081f64a5327e58e6) )

	ROM_REGION32_BE( 0x80, REGION_USER3, 0 )
	ROM_LOAD ( "at24c01.u34", 0x000000, 0x80,  CRC(ba7e503f) SHA1(454aa4fdde7d8694d1affaf25cd750fa678686bb) )
ROM_END


GAMEX( 1998, 11beat,   0,  aleck64, aleck64, aleck64, ROT0, "Hudson", "Eleven Beat", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1998, mtetrisc,   0,  aleck64, aleck64, aleck64, ROT0, "Capcom", "Magical Tetris Challenge (981009 Japan)", GAME_NOT_WORKING|GAME_NO_SOUND )
