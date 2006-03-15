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
#include "cpu/rsp/rsp.h"
#include "streams.h"
#include "sound/custom.h"

/* defined in vidhrdw/n64.c */
extern VIDEO_START( n64 );
extern VIDEO_UPDATE( n64 );
extern void rdp_process_list(void);

UINT32 *rdram;
UINT32 *rsp_imem;
UINT32 *rsp_dmem;

#define SP_INTERRUPT	0x1
#define SI_INTERRUPT	0x2
#define AI_INTERRUPT	0x4
#define VI_INTERRUPT	0x8
#define PI_INTERRUPT	0x10
#define DP_INTERRUPT	0x20

// MIPS Interface
static UINT32 mi_version;
static UINT32 mi_interrupt = 0;
static UINT32 mi_intr_mask = 0;

static void signal_rcp_interrupt(int interrupt)
{
	if (mi_intr_mask & interrupt)
	{
		mi_interrupt |= interrupt;

		cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
	}
}

static void clear_rcp_interrupt(int interrupt)
{
	mi_interrupt &= ~interrupt;

	//if (!mi_interrupt)
	{
		cpunum_set_input_line(0, INPUT_LINE_IRQ0, CLEAR_LINE);
	}
}

static READ32_HANDLER( mi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:			// MI_VERSION_REG
			return mi_version;

		case 0x08/4:			// MI_INTR_REG
			return mi_interrupt;

		case 0x0c/4:			// MI_INTR_MASK_REG
			return mi_intr_mask;

		default:
			logerror("mi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

static WRITE32_HANDLER( mi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// MI_INIT_MODE_REG
			if (data & 0x0800)
			{
				clear_rcp_interrupt(DP_INTERRUPT);
			}
			break;

		case 0x04/4:		// MI_VERSION_REG
			mi_version = data;
			break;

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
			logerror("mi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// RSP Interface
#define SP_STATUS_HALT			0x0001
#define SP_STATUS_BROKE			0x0002
#define SP_STATUS_DMABUSY		0x0004
#define SP_STATUS_DMAFULL		0x0008
#define SP_STATUS_IOFULL		0x0010
#define SP_STATUS_SSTEP			0x0020
#define SP_STATUS_INT_ON_BRK	0x0040
#define SP_STATUS_SIGNAL0		0x0080
#define SP_STATUS_SIGNAL1		0x0100
#define SP_STATUS_SIGNAL2		0x0200
#define SP_STATUS_SIGNAL3		0x0400
#define SP_STATUS_SIGNAL4		0x0800
#define SP_STATUS_SIGNAL5		0x1000
#define SP_STATUS_SIGNAL6		0x2000
#define SP_STATUS_SIGNAL7		0x4000

static UINT32 rsp_sp_status = 0;
//static UINT32 cpu_sp_status = SP_STATUS_HALT;
static UINT32 sp_mem_addr;
static UINT32 sp_dram_addr;
static int sp_dma_length;
static int sp_dma_count;
static int sp_dma_skip;

static UINT32 sp_semaphore;

static void sp_dma(int direction)
{
	UINT8 *src, *dst;
	int i;

	if (sp_dma_length == 0)
	{
		return;
	}

	if (sp_mem_addr & 0x3)
	{
		fatalerror("sp_dma: sp_mem_addr unaligned: %08X\n", sp_mem_addr);
	}
	if (sp_dram_addr & 0x3)
	{
		fatalerror("sp_dma: sp_dram_addr unaligned: %08X\n", sp_dram_addr);
	}

	if (sp_dma_count > 0)
	{
		fatalerror("sp_dma: dma_count = %d\n", sp_dma_count);
	}
	if (sp_dma_skip > 0)
	{
		fatalerror("sp_dma: dma_skip = %d\n", sp_dma_skip);
	}

	if ((sp_mem_addr & 0xfff) + (sp_dma_length+1) > 0x1000)
	{
		fatalerror("sp_dma: dma out of memory area: %08X, %08X\n", sp_mem_addr, sp_dma_length+1);
	}

	if (direction == 0)		// RDRAM -> I/DMEM
	{
		src = (UINT8*)&rdram[sp_dram_addr / 4];
		dst = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];

		for (i=0; i <= sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

		/*dst = (sp_mem_addr & 0x1000) ? (UINT8*)rsp_imem : (UINT8*)rsp_dmem;
        for (i=0; i <= sp_dma_length; i++)
        {
            dst[BYTE4_XOR_BE(sp_mem_addr+i) & 0xfff] = src[BYTE4_XOR_BE(i)];
        }*/
	}
	else					// I/DMEM -> RDRAM
	{
		src = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];
		dst = (UINT8*)&rdram[sp_dram_addr / 4];

		for (i=0; i <= sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

		/*src = (sp_mem_addr & 0x1000) ? (UINT8*)rsp_imem : (UINT8*)rsp_dmem;
        for (i=0; i <= sp_dma_length; i++)
        {
            dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(sp_mem_addr+i) & 0xfff];
        }*/
	}
}




void sp_set_status(UINT32 status)
{
	if (status & 0x1)
	{
		cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
		rsp_sp_status |= SP_STATUS_HALT;
	}
	if (status & 0x2)
	{
		rsp_sp_status |= SP_STATUS_BROKE;

		if (rsp_sp_status & SP_STATUS_INT_ON_BRK)
		{
			signal_rcp_interrupt(SP_INTERRUPT);
		}
	}
}

static READ32_HANDLER( sp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// SP_MEM_ADDR_REG
			return sp_mem_addr;

		case 0x04/4:		// SP_DRAM_ADDR_REG
			return sp_dram_addr;

		case 0x08/4:		// SP_RD_LEN_REG
			return (sp_dma_skip << 20) | (sp_dma_count << 12) | sp_dma_length;

		case 0x10/4:		// SP_STATUS_REG
			return rsp_sp_status;

		case 0x14/4:		// SP_DMA_FULL_REG
			return 0;

		case 0x18/4:		// SP_DMA_BUSY_REG
			return 0;

		case 0x1c/4:		// SP_SEMAPHORE_REG
			return sp_semaphore;

		default:
			logerror("sp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

static WRITE32_HANDLER( sp_reg_w )
{
	if ((offset & 0x10000) == 0)
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:		// SP_MEM_ADDR_REG
				sp_mem_addr = data;
				break;

			case 0x04/4:		// SP_DRAM_ADDR_REG
				sp_dram_addr = data & 0xffffff;
				break;

			case 0x08/4:		// SP_RD_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(0);
				break;

			case 0x0c/4:		// SP_WR_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(1);
				break;

			case 0x10/4:		// SP_STATUS_REG
			{
				if (data & 0x00000001)		// clear halt
				{
					cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
					rsp_sp_status &= ~SP_STATUS_HALT;
				}
				if (data & 0x00000002)		// set halt
				{
					cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
					rsp_sp_status |= SP_STATUS_HALT;
				}
				if (data & 0x00000004) rsp_sp_status &= ~SP_STATUS_BROKE;		// clear broke
				if (data & 0x00000008)		// clear interrupt
				{
					clear_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000010)		// set interrupt
				{
					signal_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000020) rsp_sp_status &= ~SP_STATUS_SSTEP;		// clear single step
				if (data & 0x00000040) rsp_sp_status |= SP_STATUS_SSTEP;		// set single step
				if (data & 0x00000080) rsp_sp_status &= ~SP_STATUS_INT_ON_BRK;	// clear interrupt on break
				if (data & 0x00000100) rsp_sp_status |= SP_STATUS_INT_ON_BRK;	// set interrupt on break
				if (data & 0x00000200) rsp_sp_status &= ~SP_STATUS_SIGNAL0;		// clear signal 0
				if (data & 0x00000400) rsp_sp_status |= SP_STATUS_SIGNAL0;		// set signal 0
				if (data & 0x00000800) rsp_sp_status &= ~SP_STATUS_SIGNAL1;		// clear signal 1
				if (data & 0x00001000) rsp_sp_status |= SP_STATUS_SIGNAL1;		// set signal 1
				if (data & 0x00002000) rsp_sp_status &= ~SP_STATUS_SIGNAL2;		// clear signal 2
				if (data & 0x00004000) rsp_sp_status |= SP_STATUS_SIGNAL2;		// set signal 2
				if (data & 0x00008000) rsp_sp_status &= ~SP_STATUS_SIGNAL3;		// clear signal 3
				if (data & 0x00010000) rsp_sp_status |= SP_STATUS_SIGNAL3;		// set signal 3
				if (data & 0x00020000) rsp_sp_status &= ~SP_STATUS_SIGNAL4;		// clear signal 4
				if (data & 0x00040000) rsp_sp_status |= SP_STATUS_SIGNAL4;		// set signal 4
				if (data & 0x00080000) rsp_sp_status &= ~SP_STATUS_SIGNAL5;		// clear signal 5
				if (data & 0x00100000) rsp_sp_status |= SP_STATUS_SIGNAL5;		// set signal 5
				if (data & 0x00200000) rsp_sp_status &= ~SP_STATUS_SIGNAL6;		// clear signal 6
				if (data & 0x00400000) rsp_sp_status |= SP_STATUS_SIGNAL6;		// set signal 6
				if (data & 0x00800000) rsp_sp_status &= ~SP_STATUS_SIGNAL7;		// clear signal 7
				if (data & 0x01000000) rsp_sp_status |= SP_STATUS_SIGNAL7;		// set signal 7
				break;
			}

			case 0x1c/4:		// SP_SEMAPHORE_REG
				sp_semaphore = data;
		//      printf("sp_semaphore = %08X\n", sp_semaphore);
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
		}
	}
	else
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:		// SP_PC_REG
				cpunum_set_info_int(1, CPUINFO_INT_PC, 0x04000000 | (data & 0x1fff));
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
		}
	}
}

UINT32 sp_read_reg(UINT32 reg)
{
	switch (reg)
	{
		//case 4:       return rsp_sp_status;
		default:	return sp_reg_r(reg, 0x00000000);
	}
}

void sp_write_reg(UINT32 reg, UINT32 data)
{
	switch (reg)
	{
		default:	sp_reg_w(reg, data, 0x00000000); break;
	}
}

// RDP Interface
UINT32 dp_start;
UINT32 dp_end;
UINT32 dp_current;
UINT32 dp_status = 0;

#define DP_STATUS_XBUS_DMA		0x01
#define DP_STATUS_FREEZE		0x02
#define DP_STATUS_FLUSH			0x04

void dp_full_sync(void)
{
	signal_rcp_interrupt(DP_INTERRUPT);
}

READ32_HANDLER( dp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// DP_START_REG
			return dp_start;

		case 0x04/4:		// DP_END_REG
			return dp_end;

		case 0x08/4:		// DP_CURRENT_REG
			return dp_current;

		case 0x0c/4:		// DP_STATUS_REG
			return dp_status;

		default:
			logerror("dp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( dp_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// DP_START_REG
			dp_start = data;
			dp_current = dp_start;
			break;

		case 0x04/4:		// DP_END_REG
			dp_end = data;
			rdp_process_list();
			break;

		case 0x0c/4:		// DP_STATUS_REG
			if (data & 0x00000001)	dp_status &= ~DP_STATUS_XBUS_DMA;
			if (data & 0x00000002)	dp_status |= DP_STATUS_XBUS_DMA;
			if (data & 0x00000004)	dp_status &= ~DP_STATUS_FREEZE;
			if (data & 0x00000008)	dp_status |= DP_STATUS_FREEZE;
			if (data & 0x00000010)	dp_status &= ~DP_STATUS_FLUSH;
			if (data & 0x00000020)	dp_status |= DP_STATUS_FLUSH;
			break;

		default:
			logerror("dp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}



// Video Interface
UINT32 vi_width;
UINT32 vi_origin;
UINT32 vi_control;
UINT32 vi_burst, vi_vsync, vi_hsync, vi_leap, vi_hstart, vi_vstart;
UINT32 vi_intr, vi_vburst, vi_xscale, vi_yscale;

static READ32_HANDLER( vi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:		// VI_ORIGIN_REG
			return vi_origin;

		case 0x08/4:		// VI_WIDTH_REG
			return vi_width;

		case 0x0c/4:
			return vi_intr;

		case 0x10/4:		// VI_CURRENT_REG
		{
			return cpu_getscanline();
		}

		case 0x14/4:		// VI_BURST_REG
			return vi_burst;

		case 0x18/4:		// VI_V_SYNC_REG
			return vi_vsync;

		case 0x1c/4:		// VI_H_SYNC_REG
			return vi_hsync;

		case 0x20/4:		// VI_LEAP_REG
			return vi_leap;

		case 0x24/4:		// VI_H_START_REG
			return vi_hstart;

		case 0x28/4:		// VI_V_START_REG
			return vi_vstart;

		case 0x2c/4:		// VI_V_BURST_REG
			return vi_vburst;

		case 0x30/4:		// VI_X_SCALE_REG
			return vi_xscale;

		case 0x34/4:		// VI_Y_SCALE_REG
			return vi_yscale;

		default:
			logerror("vi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}
	return 0;
}

static WRITE32_HANDLER( vi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// VI_CONTROL_REG
			if ((vi_control & 0x40) != (data & 0x40))
			{
				set_visible_area(0, vi_width-1, 0, (data & 0x40) ? 479 : 239);
			}
			vi_control = data;
			break;

		case 0x04/4:		// VI_ORIGIN_REG
			vi_origin = data & 0xffffff;
			break;

		case 0x08/4:		// VI_WIDTH_REG
			if (vi_width != data && data > 0)
			{
				set_visible_area(0, data-1, 0, (vi_control & 0x40) ? 479 : 239);
			}
			vi_width = data;
			break;

		case 0x0c/4:		// VI_INTR_REG
			vi_intr = data;
			break;

		case 0x10/4:		// VI_CURRENT_REG
			clear_rcp_interrupt(VI_INTERRUPT);
			break;

		case 0x14/4:		// VI_BURST_REG
			vi_burst = data;
			break;

		case 0x18/4:		// VI_V_SYNC_REG
			vi_vsync = data;
			break;

		case 0x1c/4:		// VI_H_SYNC_REG
			vi_hsync = data;
			break;

		case 0x20/4:		// VI_LEAP_REG
			vi_leap = data;
			break;

		case 0x24/4:		// VI_H_START_REG
			vi_hstart = data;
			break;

		case 0x28/4:		// VI_V_START_REG
			vi_vstart = data;
			break;

		case 0x2c/4:		// VI_V_BURST_REG
			vi_vburst = data;
			break;

		case 0x30/4:		// VI_X_SCALE_REG
			vi_xscale = data;
			break;

		case 0x34/4:		// VI_Y_SCALE_REG
			vi_yscale = data;
			break;

		default:
			logerror("vi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// Audio Interface
static UINT32 ai_dram_addr;
static UINT32 ai_len;
static UINT32 ai_control = 0;
static int ai_dacrate;
static int ai_bitrate;
static UINT32 ai_status = 0;

static int audio_dma_active = 0;
static void *audio_timer;

#define SOUNDBUFFER_LENGTH		0x80000
#define AUDIO_DMA_DEPTH		2

typedef struct
{
	UINT32 address;
	UINT32 length;
} AUDIO_DMA;

static AUDIO_DMA audio_fifo[AUDIO_DMA_DEPTH];
static int audio_fifo_wpos = 0;
static int audio_fifo_rpos = 0;
static int audio_fifo_num = 0;

static INT16 *soundbuffer[2];
static INT64 sb_dma_ptr = 0;
static INT64 sb_play_ptr = 0;

static void audio_fifo_push(UINT32 address, UINT32 length)
{
	if (audio_fifo_num == AUDIO_DMA_DEPTH)
	{
		printf("audio_fifo_push: tried to push to full DMA FIFO!!!\n");
	}

	audio_fifo[audio_fifo_wpos].address = address;
	audio_fifo[audio_fifo_wpos].length = length;

	audio_fifo_wpos++;
	audio_fifo_num++;

	if (audio_fifo_wpos >= AUDIO_DMA_DEPTH)
	{
		audio_fifo_wpos = 0;
	}

	if (audio_fifo_num >= AUDIO_DMA_DEPTH)
	{
		ai_status |= 0x80000001;	// FIFO full
		signal_rcp_interrupt(AI_INTERRUPT);
	}
}

static void audio_fifo_pop(void)
{
	audio_fifo_rpos++;
	audio_fifo_num--;

	if (audio_fifo_num < 0)
	{
		fatalerror("audio_fifo_pop: FIFO underflow!\n");
	}

	if (audio_fifo_rpos >= AUDIO_DMA_DEPTH)
	{
		audio_fifo_rpos = 0;
	}

	if (audio_fifo_num < AUDIO_DMA_DEPTH)
	{
		ai_status &= ~0x80000001;	// FIFO not full
	}

	ai_len = 0;
}

static AUDIO_DMA *audio_fifo_get_top(void)
{
	if (audio_fifo_num > 0)
	{
		return &audio_fifo[audio_fifo_rpos];
	}
	else
	{
		return NULL;
	}
}

static void start_audio_dma(void)
{
	UINT16 *ram = (UINT16*)rdram;
	int i;
	double time, frequency;
	AUDIO_DMA *current = audio_fifo_get_top();

	frequency = (double)48681812 / (double)(ai_dacrate+1);
	time = (double)(current->length / 4) / frequency;

	timer_adjust(audio_timer, TIME_IN_SEC(time), 0, 0);

	//sb_play_ptr += current->length;

	for (i=0; i < current->length; i+=4)
	{
		//INT16 l = program_read_word_32be(current->address + i + 0);
		//INT16 r = program_read_word_32be(current->address + i + 2);
		INT16 l = ram[(current->address + i + 0) / 2];
		INT16 r = ram[(current->address + i + 2) / 2];

		soundbuffer[0][sb_dma_ptr] = l;
		soundbuffer[1][sb_dma_ptr] = r;

		sb_dma_ptr++;
		if (sb_dma_ptr >= SOUNDBUFFER_LENGTH)
		{
			sb_dma_ptr = 0;
		}
	}
}

static void audio_timer_callback(int param)
{
	audio_fifo_pop();

	// keep playing if there's another DMA queued
	if (audio_fifo_get_top() != NULL)
	{
		start_audio_dma();
	}
	else
	{
		audio_dma_active = 0;
	}
}

static READ32_HANDLER( ai_reg_r )
{
	switch (offset)
	{
		case 0x04/4:		// AI_LEN_REG
		{
			return ai_len;
		}

		case 0x0c/4:		// AI_STATUS_REG
			return ai_status;

		default:
			logerror("ai_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

static WRITE32_HANDLER( ai_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// AI_DRAM_ADDR_REG
//          printf("ai_dram_addr = %08X at %08X\n", data, activecpu_get_pc());
			ai_dram_addr = data & 0xffffff;
			break;

		case 0x04/4:		// AI_LEN_REG
//          printf("ai_len = %08X at %08X\n", data, activecpu_get_pc());
			ai_len = data & 0x3ffff;		// Hardware v2.0 has 18 bits, v1.0 has 15 bits
			//audio_dma();
			audio_fifo_push(ai_dram_addr, ai_len);

			// if no DMA transfer is active, start right away
			if (!audio_dma_active)
			{
				start_audio_dma();
			}
			break;

		case 0x08/4:		// AI_CONTROL_REG
//          printf("ai_control = %08X at %08X\n", data, activecpu_get_pc());
			ai_control = data;
			break;

		case 0x0c/4:
			clear_rcp_interrupt(AI_INTERRUPT);
			break;

		case 0x10/4:		// AI_DACRATE_REG
//          printf("ai_dacrate = %08X\n", data);
			ai_dacrate = data & 0x3fff;
			break;

		case 0x14/4:		// AI_BITRATE_REG
//          printf("ai_bitrate = %08X\n", data);
			ai_bitrate = data & 0xf;
			break;

		default:
			logerror("ai_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// Peripheral Interface

static UINT32 pi_dram_addr, pi_cart_addr;
static UINT32 pi_first_dma = 1;

static READ32_HANDLER( pi_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// PI_DRAM_ADDR_REG
			return pi_dram_addr;

		case 0x04/4:		// PI_CART_ADDR_REG
			return pi_cart_addr;

		case 0x10/4:		// PI_STATUS_REG
			return 0;

		default:
			logerror("pi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
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
			fatalerror("PI_RD_LEN_REG: %08X\n", data);
			break;
		}

		case 0x0c/4:		// PI_WR_LEN_REG
		{
			int i;
			UINT32 dma_length = (data + 1);

			//printf("PI DMA: %08X to %08X, length %08X\n", pi_cart_addr, pi_dram_addr, dma_length);

			if (pi_dram_addr != 0xffffffff)
			{
				for (i=0; i < dma_length; i++)
				{
					/*UINT32 d = program_read_dword_32be(pi_cart_addr);
                    program_write_dword_32be(pi_dram_addr, d);
                    pi_cart_addr += 4;
                    pi_dram_addr += 4;*/

					UINT8 b = program_read_byte_32be(pi_cart_addr);
					program_write_byte_32be(pi_dram_addr, b);
					pi_cart_addr += 1;
					pi_dram_addr += 1;
				}
			}
			signal_rcp_interrupt(PI_INTERRUPT);

			if (pi_first_dma)
			{
				// TODO: CIC-6105 has different address...
				program_write_dword_32be(0x00000318, 0x400000);
				pi_first_dma = 0;
			}

			break;
		}

		case 0x10/4:		// PI_STATUS_REG
		{
			if (data & 0x2)
			{
				clear_rcp_interrupt(PI_INTERRUPT);
			}
			break;
		}

		default:
			logerror("pi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

// RDRAM Interface
static READ32_HANDLER( ri_reg_r )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

static WRITE32_HANDLER( ri_reg_w )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

// Serial Interface
UINT32 pif_ram[0x40/4];
UINT32 pif_cmd[0x40/4];
UINT32 si_dram_addr = 0;
UINT32 si_pif_addr = 0;
UINT32 si_status = 0;

static void handle_pif(void)
{
	int i;
	if ((pif_cmd[0xf] & 0xff) == 0x01)		// only handle the command if the last byte is 1
	{
		for (i=0; i < 5; i++)
		{
			UINT32 d1 = pif_cmd[(i*2) + 0];
		//  UINT32 d2 = pif_cmd[(i*2) + 1];

			UINT8 command = (d1 >> 24) & 0xff;
			UINT8 bytes_to_send = (d1 >> 16) & 0xff;
			UINT8 bytes_to_recv = (d1 >> 8) & 0xff;
			UINT8 cmd_type = (d1 & 0xff);

			if (command == 0xff)		// new command
			{
				switch (cmd_type)
				{
					case 0x00:		// read status
					{
						if (bytes_to_send != 1 || bytes_to_recv != 3)
						{
				//          osd_die("handle_pif: read status (bytes to send %d, bytes to receive %d)\n", bytes_to_send, bytes_to_recv);
						}

						if (i==0)
						{
							pif_ram[(i*2) + 1] &= 0x000000ff;
							pif_ram[(i*2) + 1] |= 0x05000200;
						}
						else
						{
							// not connected
							pif_ram[(i*2) + 0] |= 0x00008000;
							pif_ram[(i*2) + 1] = 0xffffffff;
						}

						break;
					}

					case 0x01:		// read button values
					{
						UINT16 buttons = 0;
						INT8 x = 0, y = 0;
						if (bytes_to_send != 1 || bytes_to_recv != 4)
						{
							fatalerror("handle_pif: read button values (bytes to send %d, bytes to receive %d)\n", bytes_to_send, bytes_to_recv);
						}

						if (i==0)
						{
							buttons = readinputport((i*3) + 0);
							x = readinputport((i*3) + 1) - 128;
							y = readinputport((i*3) + 2) - 128;
						}
						pif_ram[(i*2) + 1] = (buttons << 16) | ((UINT8)(x) << 8) | (UINT8)(y);
						break;
					}

					case 0xff:		// reset
					{
						break;
					}

					default:
					{
						printf("handle_pif: unknown/unimplemented command %02X\n", cmd_type);
					}
				}
			}
			else if (command == 0xfe)	// end of commands
			{
				return;
			}
			else if (command == 0x00)	// empty
			{

			}
			else
			{
				printf("handle_pif: unknown command %02X\n", command);

				pif_ram[(i*2) + 0] |= 0x00008000;
				pif_ram[(i*2) + 1] = 0xffffffff;
			}
		}

		pif_ram[0xf] &= 0xffffff00;
	}
}

static void pif_dma(int direction)
{
	int i;
	UINT32 *src, *dst;

	if (si_dram_addr & 0x3)
	{
		fatalerror("pif_dma: si_dram_addr unaligned: %08X\n", si_dram_addr);
	}

	if (direction)		// RDRAM -> PIF RAM
	{
		src = &rdram[(si_dram_addr & 0x1fffffff) / 4];
		dst = pif_ram;

		for (i=0; i < 64; i+=4)
		{
			*dst++ = *src++;
		}

		memcpy(pif_cmd, pif_ram, 0x40);
	}
	else				// PIF RAM -> RDRAM
	{
		handle_pif();

		src = pif_ram;
		dst = &rdram[(si_dram_addr & 0x1fffffff) / 4];

		for (i=0; i < 64; i+=4)
		{
			*dst++ = *src++;
		}
	}

	si_status |= 0x1000;
	signal_rcp_interrupt(SI_INTERRUPT);
}

static READ32_HANDLER( si_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// SI_DRAM_ADDR_REG
			return si_dram_addr;

		case 0x18/4:		// SI_STATUS_REG
			return si_status;
	}
	return 0;
}

static WRITE32_HANDLER( si_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// SI_DRAM_ADDR_REG
			si_dram_addr = data;
	//      printf("si_dram_addr = %08X\n", si_dram_addr);
			break;

		case 0x04/4:		// SI_PIF_ADDR_RD64B_REG
			// PIF RAM -> RDRAM
			si_pif_addr = data;
			pif_dma(0);
			break;

		case 0x10/4:		// SI_PIF_ADDR_WR64B_REG
			// RDRAM -> PIF RAM
			si_pif_addr = data;
			pif_dma(1);
			break;

		case 0x18/4:		// SI_STATUS_REG
			si_status &= ~0x1000;
			clear_rcp_interrupt(SI_INTERRUPT);
			break;

		default:
			logerror("si_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}
}

static READ32_HANDLER( pif_ram_r )
{
	return pif_ram[offset];
}

static WRITE32_HANDLER( pif_ram_w )
{
//  printf("pif_ram_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	pif_ram[offset] = data;

	signal_rcp_interrupt(SI_INTERRUPT);
}

static READ32_HANDLER( unknown_r )
{
	//return 0xff0fffff;
	if (offset == 0)
	{
		return (readinputport(3) << 16) | 0xffff;
	}
	else if (offset == 1)
	{
		return (readinputport(4) << 16) | 0xffff;
	}
	return 0;
}

static ADDRESS_MAP_START( n64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&rdram)				// RDRAM
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_SHARE(1)					// RSP DMEM
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_SHARE(2)					// RSP IMEM
	AM_RANGE(0x04040000, 0x040fffff) AM_READWRITE(sp_reg_r, sp_reg_w)	// RSP
	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(dp_reg_r, dp_reg_w)	// RDP
	AM_RANGE(0x04300000, 0x043fffff) AM_READWRITE(mi_reg_r, mi_reg_w)	// MIPS Interface
	AM_RANGE(0x04400000, 0x044fffff) AM_READWRITE(vi_reg_r, vi_reg_w)	// Video Interface
	AM_RANGE(0x04500000, 0x045fffff) AM_READWRITE(ai_reg_r, ai_reg_w)	// Audio Interface
	AM_RANGE(0x04600000, 0x046fffff) AM_READWRITE(pi_reg_r, pi_reg_w)	// Peripheral Interface
	AM_RANGE(0x04700000, 0x047fffff) AM_READWRITE(ri_reg_r, ri_reg_w)	// RDRAM Interface
	AM_RANGE(0x04800000, 0x048fffff) AM_READWRITE(si_reg_r, si_reg_w)	// Serial Interface
	AM_RANGE(0x10000000, 0x13ffffff) AM_ROM AM_REGION(REGION_USER2, 0)	// Cartridge
	AM_RANGE(0x1fc00000, 0x1fc007bf) AM_ROM AM_REGION(REGION_USER1, 0)	// PIF ROM
	AM_RANGE(0x1fc007c0, 0x1fc007ff) AM_READWRITE(pif_ram_r, pif_ram_w)

	AM_RANGE(0xc0800000, 0xc08fffff) AM_READWRITE(unknown_r, MWA32_NOP)
	AM_RANGE(0xd0000000, 0xd08fffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( rsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_BASE(&rsp_dmem) AM_SHARE(1)
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_BASE(&rsp_imem) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( aleck64 )
	PORT_START_TAG("P1")
		PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)			// Button A
		PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)			// Button B
		PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)			// Button Z
		PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START1 ) 							// Start
		PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)		// Joypad Up
		PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)	// Joypad Down
		PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)	// Joypad Left
		PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)	// Joypad Right
		PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)			// Pan Left
		PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1)			// Pan Right
		PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1)			// C Button Up
		PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_PLAYER(1)			// C Button Down
		PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_PLAYER(1)			// C Button Left
		PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_PLAYER(1)			// C Button Right

	PORT_START_TAG("P1_ANALOG_X")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START_TAG("P1_ANALOG_Y")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0xff,0x00) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START
		PORT_DIPNAME( 0x8000, 0x8000, "DIPSW1 #8" )
		PORT_DIPSETTING( 0x8000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x4000, 0x4000, "DIPSW1 #7" )
		PORT_DIPSETTING( 0x4000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x2000, 0x2000, "DIPSW1 #6" )
		PORT_DIPSETTING( 0x2000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x1000, 0x1000, "DIPSW1 #5" )
		PORT_DIPSETTING( 0x1000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0800, 0x0800, "DIPSW1 #4" )
		PORT_DIPSETTING( 0x0800, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0400, 0x0400, "DIPSW1 #3" )
		PORT_DIPSETTING( 0x0400, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0200, 0x0200, "DIPSW1 #2" )
		PORT_DIPSETTING( 0x0200, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0100, 0x0100, "DIPSW1 #1" )
		PORT_DIPSETTING( 0x0100, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0080, 0x0080, "Test Mode" )
		PORT_DIPSETTING( 0x0080, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0040, 0x0040, "DIPSW2 #7" )
		PORT_DIPSETTING( 0x0040, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0020, 0x0020, "DIPSW2 #6" )
		PORT_DIPSETTING( 0x0020, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0010, 0x0010, "DIPSW2 #5" )
		PORT_DIPSETTING( 0x0010, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0008, 0x0008, "DIPSW2 #4" )
		PORT_DIPSETTING( 0x0008, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0004, 0x0004, "DIPSW2 #3" )
		PORT_DIPSETTING( 0x0004, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0002, 0x0002, "DIPSW2 #2" )
		PORT_DIPSETTING( 0x0002, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0001, 0x0001, "DIPSW2 #1" )
		PORT_DIPSETTING( 0x0001, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )

	PORT_START
		PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
		PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_7)
		PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

static int audio_playing = 0;

void n64_sh_update( void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length )
{
	INT64 play_step;
	double frequency;
	int i;

	frequency = (double)48681812 / (double)(ai_dacrate+1);
	play_step = (UINT32)((frequency / (double)Machine->sample_rate) * 65536.0);

	//printf("dma_ptr = %08X, play_ptr = %08X%08X\n", (UINT32)sb_dma_ptr, (UINT32)(sb_play_ptr >> 32),(UINT32)sb_play_ptr);
	//printf("frequency = %f, step = %08X\n", frequency, (UINT32)play_step);

	if (!audio_playing)
	{
		if (sb_dma_ptr >= 0x1000)
		{
			audio_playing = 1;
		}
		else
		{
			return;
		}
	}

	for (i = 0; i < length; i++)
	{
		short mix[2];
		mix[0] = soundbuffer[0][sb_play_ptr >> 16];
		mix[1] = soundbuffer[1][sb_play_ptr >> 16];

		sb_play_ptr += play_step;

		if ((sb_play_ptr >> 16) >= SOUNDBUFFER_LENGTH)
		{
			sb_play_ptr = 0;
		}

		// Update the buffers
		outputs[0][i] = (stream_sample_t)mix[0];
		outputs[1][i] = (stream_sample_t)mix[1];
	}
}

static void *n64_sh_start(int clock, const struct CustomSound_interface *config)
{
	stream_create( 0, 2, 44100, NULL, n64_sh_update );

	return auto_malloc(1);
}




/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	8192,				/* data cache size */
	62500000			/* system clock */
};

static INTERRUPT_GEN( n64_vblank )
{
	signal_rcp_interrupt(VI_INTERRUPT);
}

static UINT16 crc_seed = 0x3f;

MACHINE_RESET( aleck64 )
{
	int i;
	UINT64 boot_checksum;
	UINT32 *pif_rom	= (UINT32*)memory_region(REGION_USER1);
	UINT32 *cart = (UINT32*)memory_region(REGION_USER2);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_DRC_OPTIONS, MIPS3DRC_FASTEST_OPTIONS + MIPS3DRC_STRICT_VERIFY);

		/* configure fast RAM regions for DRC */
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 0);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x00000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x003fffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rdram);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	soundbuffer[0] = auto_malloc(SOUNDBUFFER_LENGTH * sizeof(INT16));
	soundbuffer[1] = auto_malloc(SOUNDBUFFER_LENGTH * sizeof(INT16));
	audio_timer = timer_alloc(audio_timer_callback);
	timer_adjust(audio_timer, TIME_NEVER, 0, 0);

	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);

	// bootcode differs between CIC-chips, so we can use its checksum to detect the CIC-chip
	boot_checksum = 0;
	for (i=0x40; i < 0x1000; i+=4)
	{
		boot_checksum += cart[i/4]+i;
	}

	if (boot_checksum == U64(0x000000d057e84864))
	{
		// CIC-NUS-6101
		printf("CIC-NUS-6101 detected\n");
		crc_seed = 0x3f;
	}
	else if (boot_checksum == U64(0x000000d6499e376b))
	{
		// CIC-NUS-6103
		printf("CIC-NUS-6103 detected\n");
		crc_seed = 0x78;
	}
	else if (boot_checksum == U64(0x0000011a4a1604b6))
	{
		// CIC-NUS-6105
		printf("CIC-NUS-6105 detected\n");
		crc_seed = 0x91;
	}
	else if (boot_checksum == U64(0x000000d6d5de4ba0))
	{
		// CIC-NUS-6106
		printf("CIC-NUS-6106 detected\n");
		crc_seed = 0x85;
	}
	else
	{
		printf("Unknown BootCode Checksum %08X%08X\n", (UINT32)(boot_checksum>>32),(UINT32)(boot_checksum));
	}


	// The PIF Boot ROM is not dumped, the following code simulates it

	// clear all registers
	for (i=1; i < 32; i++)
	{
		*pif_rom++ = 0x00000000 | 0 << 21 | 0 << 16 | i << 11 | 0x20;		// ADD ri, r0, r0
	}

	// R20 <- 0x00000001
	*pif_rom++ = 0x34000000 | 20 << 16 | 0x0001;					// ORI r20, r0, 0x0001

	// R22 <- 0x0000003F
	*pif_rom++ = 0x34000000 | 22 << 16 | crc_seed;					// ORI r22, r0, 0x003f

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

	// Zelda and DK64 need these
	*pif_rom++ = 0x3c000000 | 9 << 16 | 0xa400;
	*pif_rom++ = 0x34000000 | 9 << 21 | 9 << 16 | 0x1ff0;
	*pif_rom++ = 0x3c000000 | 11 << 16 | 0xa400;
	*pif_rom++ = 0x3c000000 | 31 << 16 | 0xffff;
	*pif_rom++ = 0x34000000 | 31 << 21 | 31 << 16 | 0xffff;

	*pif_rom++ = 0x3c000000 | 1 << 16 | 0xa400;						// LUI r1, 0xa400
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0040;			// ORI r1, r1, 0x0040
	*pif_rom++ = 0x00000000 | 1 << 21 | 0x8;						// JR r1
}

static struct CustomSound_interface n64_sound_interface =
{
	n64_sh_start
};

MACHINE_DRIVER_START( aleck64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 93750000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(n64_map, 0)
	MDRV_CPU_VBLANK_INT( n64_vblank, 1 )

	MDRV_CPU_ADD(RSP, 62500000)
	MDRV_CPU_PROGRAM_MAP(rsp_map, 0)

	MDRV_MACHINE_RESET( aleck64 )

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(640, 525)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(n64)
	MDRV_VIDEO_UPDATE(n64)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(n64_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(0, "right", 1.00)
MACHINE_DRIVER_END

DRIVER_INIT( aleck64 )
{
	UINT8 *rom = memory_region(REGION_USER2);

	rom[0x67c] = 0;
	rom[0x67d] = 0;
	rom[0x67e] = 0;
	rom[0x67f] = 0;
}

ROM_START( 11beat )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x800, REGION_USER1, 0 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x4000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zhaj.u3", 0x000000, 0x0800000,  CRC(95258ba2) SHA1(0299b8fb9a8b1b24428d0f340f6bf1cfaf99c672) )
ROM_END

ROM_START( mtetrisc )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	ROM_REGION32_BE( 0x800, REGION_USER1, 0 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x4000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zcaj.u4", 0x000000, 0x1000000,  CRC(ec4563fc) SHA1(4d5a30873a5850cf4cd1c0bdbe24e1934f163cd0) )

	ROM_REGION32_BE( 0x100000, REGION_USER3, 0 )
	ROM_LOAD ( "tet-01m.u5", 0x000000, 0x100000,  CRC(f78f859b) SHA1(b07c85e0453869fe43792f42081f64a5327e58e6) )

	ROM_REGION32_BE( 0x80, REGION_USER4, 0 )
	ROM_LOAD ( "at24c01.u34", 0x000000, 0x80,  CRC(ba7e503f) SHA1(454aa4fdde7d8694d1affaf25cd750fa678686bb) )
ROM_END

GAME( 1998, 11beat,   0,  aleck64, aleck64, aleck64, ROT0, "Hudson", "Eleven Beat", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, mtetrisc, 0,  aleck64, aleck64, aleck64, ROT0, "Capcom", "Magical Tetris Challenge (981009 Japan)", GAME_NOT_WORKING|GAME_NO_SOUND )
