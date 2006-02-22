/* 'Aleck64' and similar boards */
/* N64 based hardware */
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

// MIPS Interface
static UINT32 mi_intr_mask = 0;

static READ32_HANDLER( mi_reg_r )
{
	switch (offset)
	{
		case 0x0c/4:			// MI_INTR_MASK_REG
			return mi_intr_mask;

	}

	return 0;
}

static WRITE32_HANDLER( mi_reg_w )
{
	switch (offset)
	{
		case 0x0c/4:		// MI_INTR_MASK_REG
		{
			if (data & 0x0001) mi_intr_mask &= ~0x1;		// clear SP mask
			if (data & 0x0002) mi_intr_mask |= 0x1;			// set SP mask
			if (data & 0x0004) mi_intr_mask &= ~0x2;		// clear SI mask
			if (data & 0x0008) mi_intr_mask |= 0x2;			// set SI mask
			if (data & 0x0010) mi_intr_mask &= ~0x4;		// clear AI mask
			if (data & 0x0020) mi_intr_mask |= 0x4;			// set AI mask
			if (data & 0x0040) mi_intr_mask &= ~0x8;		// clear VI mask
			if (data & 0x0080) mi_intr_mask |= 0x8;			// set VI mask
			if (data & 0x0100) mi_intr_mask &= ~0x10;		// clear PI mask
			if (data & 0x0200) mi_intr_mask |= 0x10;		// set PI mask
			if (data & 0x0400) mi_intr_mask &= ~0x20;		// clear DP mask
			if (data & 0x0800) mi_intr_mask |= 0x20;		// set DP mask
			break;
		}

		default:
			logerror("mi_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	}
}

// Video Interface
static READ32_HANDLER( vi_reg_r )
{
	switch (offset)
	{
		case 0x10/4:		// VI_CURRENT_REG
			return cpu_getscanline();

		default:
			logerror("vi_reg_r: %08X, %08X\n", offset, mem_mask);
	}
	return 0;
}

static WRITE32_HANDLER( vi_reg_w )
{
	switch (offset)
	{
		default:
			logerror("vi_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	}
}

// Peripheral Interface

static UINT32 pi_dram_addr, pi_cart_addr;

static READ32_HANDLER( pi_reg_r )
{
	switch (offset)
	{
		case 0x10/4:		// PI_STATUS_REG
			return 0;

	}
	return 0;
}

static WRITE32_HANDLER( pi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// PI_DRAM_ADDR_REG
		{
			pi_dram_addr = data;
			break;
		}

		case 0x04/4:		// PI_CART_ADDR_REG
		{
			pi_cart_addr = data;
			break;
		}

		case 0x08/4:		// PI_RD_LEN_REG
		{
			break;
		}

		case 0x0c/4:		// PI_WR_LEN_REG
		{
			int i;
			UINT32 dma_length = (data + 1) / 4;

			for (i=0; i < dma_length; ++i)
			{
				UINT32 d = program_read_dword_32be(pi_cart_addr);
				program_write_dword_32be(pi_dram_addr, d);
				pi_cart_addr += 4;
				pi_dram_addr += 4;
			}

			break;
		}
	}
}

// RDRAM Interface
static READ32_HANDLER( ri_reg_r )
{
	return 0;
}

static WRITE32_HANDLER( ri_reg_w )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	}
}

static ADDRESS_MAP_START( aleck64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM				// RDRAM
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM				// RSP DMEM
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM				// RSP IMEM
	AM_RANGE(0x04300000, 0x043fffff) AM_READWRITE(mi_reg_r, mi_reg_w)	// MIPS Interface
	AM_RANGE(0x04400000, 0x044fffff) AM_READWRITE(vi_reg_r, vi_reg_w)	// Video Interface
	AM_RANGE(0x04600000, 0x046fffff) AM_READWRITE(pi_reg_r, pi_reg_w)	// Peripheral Interface
	AM_RANGE(0x04700000, 0x047fffff) AM_READWRITE(ri_reg_r, ri_reg_w)	// RDRAM Interface
	AM_RANGE(0x10000000, 0x10ffffff) AM_ROM AM_REGION(REGION_USER2, 0)	// Cartridge
	AM_RANGE(0x1fc00000, 0x1fc007bf) AM_ROM AM_REGION(REGION_USER1, 0)	// PIF ROM
	AM_RANGE(0x1fc007c0, 0x1fc007ff) AM_RAM								// PIF RAM
ADDRESS_MAP_END

INPUT_PORTS_START( aleck64 )
INPUT_PORTS_END

/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	16384				/* data cache size */
};

MACHINE_RESET( aleck64 )
{
	int i;
	UINT32 *pif_rom	= (UINT32*)memory_region(REGION_USER1);

	// The PIF Boot ROM is not dumped, the following code simulates it

	// clear all registers
	for (i=1; i < 32; i++)
	{
		*pif_rom++ = 0x00000000 | 0 << 21 | 0 << 16 | i << 11 | 0x20;		// ADD ri, r0, r0
	}

	// R20 <- 0x00000001
	*pif_rom++ = 0x34000000 | 20 << 16 | 0x0001;					// ORI r20, r0, 0x0001

	// R22 <- 0x0000003F
	*pif_rom++ = 0x34000000 | 22 << 16 | 0x003f;					// ORI r22, r0, 0x003f

	// R29 <- 0xA4001FF0
	*pif_rom++ = 0x3c000000 | 29 << 16 | 0xa400;					// LUI r29, 0xa400
	*pif_rom++ = 0x34000000 | 29 << 21 | 29 << 16 | 0x1ff0;			// ORI r29, r29, 0x1ff0

	// clear CP0 registers
	for (i=0; i < 32; i++)
	{
		*pif_rom++ = 0x40000000 | 4 << 21 | 0 << 16 | i << 11;		// MTC2 cp0ri, r0
	}

	// Random <- 0x0000001F
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x001f;
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 1 << 11;			// MTC2 Random, r1

	// Status <- 0x70400004
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x7040;						// LUI r1, 0x7040
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0004;			// ORI r1, r1, 0x0004
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 12 << 11;			// MTC2 Status, r1

	// PRId <- 0x00000B00
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x0b00;						// ORI r1, r0, 0x0b00
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 15 << 11;			// MTC2 PRId, r1

	// Config <- 0x0006E463
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0006;						// LUI r1, 0x0006
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0xe463;			// ORI r1, r1, 0xe463
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 16 << 11;			// MTC2 Config, r1

	// (0xa4300004) <- 0x01010101
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0101;						// LUI r1, 0x0101
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0101;			// ORI r1, r1, 0x0101
	*pif_rom++ = 0x3c000000 | 3 << 16 | 0xa430;						// LUI r3, 0xa430
	*pif_rom++ = 0xac000000 | 3 << 21 | 1 << 16 | 0x0004;			// SW r1, 0x0004(r3)

	// Copy 0xb0000000...1fff -> 0xa4000000...1fff
	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0400;						// ORI r3, r0, 0x0400
	*pif_rom++ = 0x3c000000 | 4 << 16 | 0xb000;						// LUI r4, 0xb000
	*pif_rom++ = 0x3c000000 | 5 << 16 | 0xa400;						// LUI r5, 0xa400
	*pif_rom++ = 0x8c000000 | 4 << 21 | 1 << 16;					// LW r1, 0x0000(r4)
	*pif_rom++ = 0xac000000 | 5 << 21 | 1 << 16;					// SW r1, 0x0000(r5)
	*pif_rom++ = 0x20000000 | 4 << 21 | 4 << 16 | 0x0004;			// ADDI r4, r4, 0x0004
	*pif_rom++ = 0x20000000 | 5 << 21 | 5 << 16 | 0x0004;			// ADDI r5, r5, 0x0004
	*pif_rom++ = 0x20000000 | 3 << 21 | 3 << 16 | 0xffff;			// ADDI r3, r3, -1
	*pif_rom++ = 0x14000000 | 3 << 21 | 0 << 16 | 0xfffa;			// BNE r3, r0, -6
	*pif_rom++ = 0x00000000;

	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0000;						// ORI r3, r0, 0x0000
	*pif_rom++ = 0x34000000 | 4 << 16 | 0x0000;						// ORI r4, r0, 0x0000
	*pif_rom++ = 0x34000000 | 5 << 16 | 0x0000;						// ORI r5, r0, 0x0000

	*pif_rom++ = 0x3c000000 | 1 << 16 | 0xa400;						// LUI r1, 0xa400
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0040;			// ORI r1, r1, 0x0040
	*pif_rom++ = 0x00000000 | 1 << 21 | 0x8;						// JR r1
}

MACHINE_DRIVER_START( aleck64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 14318180*4)  /* actually VR4300 ?? speed */
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(aleck64_map, 0)

	MDRV_MACHINE_RESET( aleck64 )

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
}

ROM_START( 11beat )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x800, REGION_USER1, 0 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x1000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zhaj.u3", 0x000000, 0x0800000,  CRC(95258ba2) SHA1(0299b8fb9a8b1b24428d0f340f6bf1cfaf99c672) )
ROM_END

ROM_START( mtetrisc )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x800, REGION_USER1, 0 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x1000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zcaj.u4", 0x000000, 0x1000000,  CRC(ec4563fc) SHA1(4d5a30873a5850cf4cd1c0bdbe24e1934f163cd0) )

	ROM_REGION32_BE( 0x100000, REGION_USER3, 0 )
	ROM_LOAD ( "tet-01m.u5", 0x000000, 0x100000,  CRC(f78f859b) SHA1(b07c85e0453869fe43792f42081f64a5327e58e6) )

	ROM_REGION32_BE( 0x80, REGION_USER4, 0 )
	ROM_LOAD ( "at24c01.u34", 0x000000, 0x80,  CRC(ba7e503f) SHA1(454aa4fdde7d8694d1affaf25cd750fa678686bb) )
ROM_END


GAME( 1998, 11beat,   0,  aleck64, aleck64, aleck64, ROT0, "Hudson", "Eleven Beat", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, mtetrisc,   0,  aleck64, aleck64, aleck64, ROT0, "Capcom", "Magical Tetris Challenge (981009 Japan)", GAME_NOT_WORKING|GAME_NO_SOUND )
