/*************************************************************************

	Driver for Atari/Midway Seattle hardware games

	driver by Aaron Giles

	Games supported:
		* Wayne Gretzky's 3d Hockey [Atari]
		* San Francisco Rush [Atari]
		* CarnEvil [Midway, 150MHz]
		* NFL Blitz 99 [Midway, 150MHz]
		* Mace [Atari, 200MHz]

	Not working:
		* California Speed [Atari, 150MHz]

	Potentially to be added:
		* Vapor TRX [Atari, 200MHz]
		* Road Burners [Atari]
		* San Francisco Rush Alcatraz Edition [Atari]
		* NFL Blitz [Midway, 150MHz]
		* NFL Blitz 2000 [Midway, 150MHz]
		* Bio Freaks [Midway]
		* Hyperdrive [Midway, 200MHz]
		
	Known bugs:
		* general: Atari games timing is not quite right
		* CarnEvil: lets you set the flash brightness; need to emulate that
		* Blitz99: no sound during movies (FIFO?)
		* Blitz99: random black frames
		* Wayne Gretzky: loses sound occasionally and has to reset it
		* Wayne Gretzky/Mace: random poly glitches (seriously out of whack parameters)
		* California Speed: doesn't boot (will go into test mode -- bad hard disk dump)
		* SF Rush: hangs when trying to start a game (security?)
		* SF Rush: displays "MATH ERR: frexpf(Inf): DOMAIN error" (FP exceptions necessary?)
		* SF Rush: crashes eventually using the DRC

**************************************************************************/

#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "cpu/mips/mips3.h"
#include "sndhrdw/dcs.h"
#include "sndhrdw/cage.h"
#include "machine/idectrl.h"
#include "machine/midwayic.h"
#include "vidhrdw/voodoo.h"


#define TIMER_CLOCK			TIME_IN_HZ(48000000)
#define DMA_SECS_PER_BYTE	TIME_IN_HZ(48000000)


#define LOG_TIMERS			(0)
#define LOG_DMA				(0)


static data32_t *rambase;
static data32_t *rombase;
static data32_t *galileo_regs;

static data32_t pci_bridge_regs[0x40];
static data32_t pci_3dfx_regs[0x40];

static void *timer[4];
static UINT32 timer_start[4];
static UINT8 timer_active[4];

static UINT8 vblank_signalled;
static UINT8 vblank_irq;
static data32_t *vblank_config;
static data32_t *vblank_enable;

static data32_t *asic_reset;

static data8_t pending_analog_read;

static data32_t *generic_speedup;
static data32_t *generic_speedup2;


static void timer_callback(int param);



/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( seattle )
{
	cpu_setbank(1, rambase);
	cpu_setbank(2, rambase);
	cpu_setbank(3, rombase);

	if (mame_find_cpu_index("dcs2") != -1)
	{
		dcs_reset_w(1);
		dcs_reset_w(0);
	}
	else if (mame_find_cpu_index("cage") != -1)
	{
		cage_control_w(0);
		cage_control_w(3);
	}

	ide_controller_reset(0);
	
	timer[0] = timer_alloc(timer_callback);
	timer[1] = timer_alloc(timer_callback);
	timer[2] = timer_alloc(timer_callback);
	timer[3] = timer_alloc(timer_callback);
	
	vblank_irq = 0;
	
	voodoo_reset();
}



/*************************************
 *
 *	IDE interrupts
 *
 *************************************/

static void ide_interrupt(int state)
{
	cpu_set_irq_line(0, 2, state);
}

static struct ide_interface ide_intf =
{
	ide_interrupt
};



/*************************************
 *
 *	I/O ASIC interrupts
 *
 *************************************/

static void ioasic_irq(int state)
{
	cpu_set_irq_line(0, 1, state);
}



/*************************************
 *
 *	VBLANK interrupts
 *
 *************************************/

static void clear_vblank(int param)
{
	logerror("Clearing vblank_irq\n");
	if (vblank_irq)
		cpu_set_irq_line(0, vblank_irq, CLEAR_LINE);
	vblank_signalled = 0;
}


static READ32_HANDLER( vblank_signalled_r )
{
	logerror("%06X:vblank_signalled_r\n", activecpu_get_pc());
	return vblank_signalled ? 0x80 : 0x00;
}


static WRITE32_HANDLER( vblank_enable_w )
{
	logerror("%06X:vblank_enable_w = %08X\n", activecpu_get_pc(), data);
	COMBINE_DATA(vblank_enable);
}


static WRITE32_HANDLER( vblank_config_w )
{
	logerror("%06X:vblank_config_w = %08X\n", activecpu_get_pc(), data);
	COMBINE_DATA(vblank_config);
	if (vblank_irq)
		cpu_set_irq_line(0, vblank_irq, CLEAR_LINE);
	if (*vblank_config & 0x8000)
		vblank_irq = 4;
	else if (*vblank_config & 0x4000)
		vblank_irq = 3;
}


static WRITE32_HANDLER( vblank_clear_w )
{
	logerror("%06X:vblank_clear_w = %08X\n", activecpu_get_pc(), data);
	if (vblank_irq)
		cpu_set_irq_line(0, vblank_irq, CLEAR_LINE);
	vblank_signalled = 0;
}


static INTERRUPT_GEN( assert_vblank )
{
	logerror("Setting IRQ3\n");
	if (*vblank_enable & 0x80)
	{
		if (vblank_irq)
			cpu_set_irq_line(0, vblank_irq, ASSERT_LINE);
		vblank_signalled = 1;
		timer_set(cpu_getscanlinetime(cpu_getscanline() + 1), 0, clear_vblank);
	}
}



/*************************************
 *
 *	CMOS access
 *
 *************************************/

static WRITE32_HANDLER( cmos_w )
{
	data32_t *cmos_base = (data32_t *)generic_nvram;
	COMBINE_DATA(&cmos_base[offset]);
}


static READ32_HANDLER( cmos_r )
{
	data32_t *cmos_base = (data32_t *)generic_nvram;
	return cmos_base[offset];
}



/*************************************
 *
 *	PCI bus writes
 *
 *************************************/

static void pci_bridge_w(UINT8 reg, UINT8 type, data32_t data)
{
	pci_bridge_regs[reg] = data;
	logerror("%06X:PCI bridge write: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, data);
}


static void pci_3dfx_w(UINT8 reg, UINT8 type, data32_t data)
{
	pci_3dfx_regs[reg] = data;
	
	switch (reg)
	{
		case 0x04:		/* address register */
			if (data != 0x08000000)
				logerror("3dfx not mapped where we expect it!\n");
			break;
		
		case 0x10:		/* initEnable register */
			voodoo_set_init_enable(data);
			break;
	}
	logerror("%06X:PCI 3dfx write: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, data);
}



/*************************************
 *
 *	PCI bus reads
 *
 *************************************/

static data32_t pci_bridge_r(UINT8 reg, UINT8 type)
{
	data32_t result = pci_bridge_regs[reg];
	
	logerror("%06X:PCI bridge read: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, result);
	
	return result;
}


static data32_t pci_3dfx_r(UINT8 reg, UINT8 type)
{
	data32_t result = pci_3dfx_regs[reg];

	switch (reg)
	{
		case 0:		/* ID register: 0x0001 = SST-1, 0x121a = 3dfx */
			result = 0x0001121a;
			break;
	}

	logerror("%06X:PCI 3dfx read: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, result);
	
	return result;
}



/*************************************
 *
 *	Galileo timers & interrupts
 *
 *************************************/

static void update_galileo_irqs(void)
{
	if (galileo_regs[0xc18/4] & galileo_regs[0xc1c/4])
	{
		logerror("Galileo IRQ asserted\n");
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	}
	else
	{
		logerror("Galileo IRQ cleared\n");
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}


static void timer_callback(int which)
{
	logerror("timer %d fired\n", which);
	
	/* copy the start value from the registers */
	timer_start[which] = galileo_regs[0x850/4 + which];
	if (which != 0)
		timer_start[which] &= 0xffffff;
	
	/* if we're a timer, adjust the timer to fire again */
	if (galileo_regs[0x864/4] & (2 << (2 * which)))
		timer_adjust(timer[which], TIMER_CLOCK * timer_start[which], which, 0);
	else
		timer_active[which] = 0;
	
	/* trigger the interrupt */
	galileo_regs[0xc18/4] |= 0x100 << which;
	update_galileo_irqs();
}



/*************************************
 *
 *	Galileo DMA handler
 *
 *************************************/

static int dma_fetch_next(int which)
{
	offs_t address = 0;
	data32_t data;
	
	/* no-op for unchained mode */
	if (!(galileo_regs[0x840/4 + which] & 0x200))
		address = galileo_regs[0x830/4 + which];
	
	/* if we hit the end address, signal an interrupt */
	if (address == 0)
	{
		if (galileo_regs[0x840/4 + which] & 0x400)
		{
			galileo_regs[0xc18/4] |= 0x10 << which;
			update_galileo_irqs();
		}
		return 0;
	}
	
	/* fetch the byte count */
	data = cpunum_read_byte(0, address++);
	data |= cpunum_read_byte(0, address++) << 8;
	data |= cpunum_read_byte(0, address++) << 16;
	data |= cpunum_read_byte(0, address++) << 24;
	galileo_regs[0x800/4 + which] = data;
	
	/* fetch the source address */
	data = cpunum_read_byte(0, address++);
	data |= cpunum_read_byte(0, address++) << 8;
	data |= cpunum_read_byte(0, address++) << 16;
	data |= cpunum_read_byte(0, address++) << 24;
	galileo_regs[0x810/4 + which] = data;
	
	/* fetch the dest address */
	data = cpunum_read_byte(0, address++);
	data |= cpunum_read_byte(0, address++) << 8;
	data |= cpunum_read_byte(0, address++) << 16;
	data |= cpunum_read_byte(0, address++) << 24;
	galileo_regs[0x820/4 + which] = data;
	
	/* fetch the next record address */
	data = cpunum_read_byte(0, address++);
	data |= cpunum_read_byte(0, address++) << 8;
	data |= cpunum_read_byte(0, address++) << 16;
	data |= cpunum_read_byte(0, address++) << 24;
	galileo_regs[0x830/4 + which] = data;
	
//	logerror("DMA Fetch Record: bytes=%08X src=%08X dst=%08X nextrec=%08X\n", 
//		galileo_regs[0x800/4 + which],
//		galileo_regs[0x810/4 + which],
//		galileo_regs[0x820/4 + which],
//		galileo_regs[0x830/4 + which]);
	return 1;
}

static void perform_dma(int which);

static void dma_finished_callback(int which)
{
//	logerror("DMA%d finished\n", which);
	galileo_regs[0x840/4 + which] &= ~0x4000;
	galileo_regs[0x840/4 + which] &= ~0x1000;
	
	/* interrupt? */
	if (!(galileo_regs[0x840/4 + which] & 0x400))
	{
		galileo_regs[0xc18/4] |= 0x10 << which;
		update_galileo_irqs();
	}
	
	/* chain? */
	if (dma_fetch_next(which))
		perform_dma(which);
}


static void perform_dma(int which)
{
	offs_t srcaddr = galileo_regs[0x810/4 + which];
	offs_t dstaddr = galileo_regs[0x820/4 + which];
	data32_t bytesleft = galileo_regs[0x800/4 + which] & 0xffff;
	int srcinc, dstinc, i;
	
	/* determine src/dst inc */
	switch ((galileo_regs[0x840/4 + which] >> 2) & 3)
	{
		default:
		case 0:		srcinc = 1;		break;
		case 1:		srcinc = -1;	break;
		case 2:		srcinc = 0;		break;
	}
	switch ((galileo_regs[0x840/4 + which] >> 4) & 3)
	{
		default:
		case 0:		dstinc = 1;		break;
		case 1:		dstinc = -1;	break;
		case 2:		dstinc = 0;		break;
	}
	
	if (LOG_DMA)
		logerror("Performing DMA%d: src=%08X dst=%08X bytes=%04X sinc=%d dinc=%d\n", which, srcaddr, dstaddr, bytesleft, srcinc, dstinc);

	/* special case: transfer ram to voodoo */
	if (bytesleft % 4 == 0 && srcaddr % 4 == 0 && srcaddr < 0x007fffff && dstaddr >= 0x08000000 && dstaddr < 0x09000000)
	{
		data32_t *src = &rambase[srcaddr/4];
		bytesleft /= 4;
		
		/* transfer to registers */
		if (dstaddr < 0x8400000)
		{
			dstaddr = (dstaddr & 0x3fffff) / 4;
			for (i = 0; i < bytesleft; i++)
			{
				voodoo_regs_w(dstaddr, *src, 0);
				src += srcinc;
				dstaddr += dstinc;
			}
		}
		
		/* transfer to framebuf */
		else if (dstaddr < 0x8800000)
		{
			dstaddr = (dstaddr & 0x3fffff) / 4;
			for (i = 0; i < bytesleft; i++)
			{
				voodoo_framebuf_w(dstaddr, *src, 0);
				src += srcinc;
				dstaddr += dstinc;
			}
		}
		
		/* transfer to textureram */
		else
		{
			dstaddr = (dstaddr & 0x7fffff) / 4;
			for (i = 0; i < bytesleft; i++)
			{
				voodoo_textureram_w(dstaddr, *src, 0);
				src += srcinc;
				dstaddr += dstinc;
			}
		}
	}

	/* standard transfer */
	else
	{
		for (i = 0; i < bytesleft; i++)
		{
			cpunum_write_byte(0, dstaddr, cpunum_read_byte(0, srcaddr));
			srcaddr += srcinc;
			dstaddr += dstinc;
		}
	}
	
	/* set a timer for the end */
	galileo_regs[0x840/4 + which] |= 0x4000;
	if (bytesleft > 0x100)
		timer_set(DMA_SECS_PER_BYTE * bytesleft, which, dma_finished_callback);
	else
		dma_finished_callback(which);
}



/*************************************
 *
 *	Galileo system controller
 *
 *************************************/

/*
	0x000 = CPU interface configuration
	
	0x008 = RAS[1:0] low decode address
	0x010 = RAS[1:0] high decode address
	0x018 = RAS[3:2] low decode address
	0x020 = RAS[3:2] high decode address
	0x028 = CS[2:0] low decode address
	0x030 = CS[2:0] high decode address
	0x038 = CS[3] & boot CS low decode address
	0x040 = CS[3] & boot CS high decode address
	0x048 = PCI I/O low decode address
	0x050 = PCI I/O high decode address
	0x058 = PCI memory low decode address
	0x060 = PCI memory high decode address
	0x068 = internal space decode
	0x070 = bus error address low processor
	0x078 = bus error address high processor
	
	0x400 = RAS[0] low decode address
	0x404 = RAS[0] high decode address
	0x408 = RAS[1] low decode address
	0x40c = RAS[1] high decode address
	0x410 = RAS[2] low decode address
	0x414 = RAS[2] high decode address
	0x418 = RAS[3] low decode address
	0x41c = RAS[3] high decode address
	0x420 = CS[0] low decode address
	0x424 = CS[0] high decode address
	0x428 = CS[1] low decode address
	0x42c = CS[1] high decode address
	0x430 = CS[2] low decode address
	0x434 = CS[2] high decode address
	0x438 = CS[3] low decode address
	0x43c = CS[3] high decode address
	0x440 = boot CS low decode address
	0x444 = boot CS high decode address
	0x448 = DRAM configuration
	0x44c = DRAM bank 0 parameters
	0x450 = DRAM bank 1 parameters
	0x454 = DRAM bank 2 parameters
	0x458 = DRAM bank 3 parameters
	0x45c = device bank 0 parameters
	0x460 = device bank 1 parameters
	0x464 = device bank 2 parameters
	0x468 = device bank 3 parameters
	0x46c = device boot bank parameters
	0x470 = address decode error

	0x800 = channel 0 DMA byte count
	0x804 = channel 1 DMA byte count
	0x808 = channel 2 DMA byte count
	0x80c = channel 3 DMA byte count
	0x810 = channel 0 DMA source address
	0x814 = channel 1 DMA source address
	0x818 = channel 2 DMA source address
	0x81c = channel 3 DMA source address
	0x820 = channel 0 DMA destination address
	0x824 = channel 1 DMA destination address
	0x828 = channel 2 DMA destination address
	0x82c = channel 3 DMA destination address
	0x830 = channel 0 next record pointer
	0x834 = channel 1 next record pointer
	0x838 = channel 2 next record pointer
	0x83c = channel 3 next record pointer
	0x840 = channel 0 control
	0x844 = channel 1 control
	0x848 = channel 2 control
	0x84c = channel 3 control
	0x850 = timer/counter 0
	0x854 = timer/counter 1
	0x858 = timer/counter 2
	0x85c = timer/counter 3
	0x860 = DMA arbiter control
	0x864 = timer/counter control
	
	0xc00 = PCI internal command
	0xc04 = PCI internal time out & retry
	0xc08 = PCI internal RAS[1:0] bank size
	0xc0c = PCI internal RAS[3:2] bank size
	0xc10 = PCI internal CS[2:0] bank size
	0xc14 = PCI internal CS[3] & boot CS bank size
	0xc18 = interrupt cause
	0xc1c = CPU interrupt mask
	0xc24 = PCI interrupt mask
	0xc28 = SErr mask
	0xc34 = interrupt acknowledge
	0xcf8 = configuration address
	0xcfc = configuration data
*/

static READ32_HANDLER( galileo_r )
{
	data32_t result = galileo_regs[offset];
	
	/* switch off the offset for special cases */
	switch (offset)
	{
		case 0x850/4:		/* timer/counter 0 count */
		case 0x854/4:		/* timer/counter 1 count */
		case 0x858/4:		/* timer/counter 2 count */
		case 0x85c/4:		/* timer/counter 3 count */
		{
			int which = offset % 4;
			
			/* eat some time for those which poll this register */
			activecpu_eat_cycles(100);
			
			result = timer_start[which];
			if (timer_active[which])
			{
				UINT32 elapsed = (UINT32)(timer_timeelapsed(timer[which]) / TIMER_CLOCK);
				result = (result > elapsed) ? (result - elapsed) : 0;
			}
			if (LOG_TIMERS)
				logerror("%06X:hires_timer_r = %08X\n", activecpu_get_pc(), result);
			break;
		}
		
		case 0xc00/4:		/* PCI internal command */
			// code at 40188 loops until this returns non-zero in bit 0
			result = 0x0001;
			break;
	
		case 0xcfc/4:		/* configuration data */
		{
			int bus = (galileo_regs[0xcf8/4] >> 16) & 0xff;
			int unit = (galileo_regs[0xcf8/4] >> 11) & 0x1f;
			int func = (galileo_regs[0xcf8/4] >> 8) & 7;
			int reg = (galileo_regs[0xcf8/4] >> 2) & 0x3f;
			int type = galileo_regs[0xcf8/4] & 3;
			
			/* unit 0 is the PCI bridge */
			if (unit == 0 && func == 0)
				result = pci_bridge_r(reg >> 2, type);
			
			/* unit 6 is the 3dfx card */
			else if (unit == 6 && func == 0)
				result = pci_3dfx_r(reg, type);
			
			/* anything else, just log */
			else
				logerror("%06X:PCIBus read: bus %d unit %d func %d reg %d type %d = %08X\n", activecpu_get_pc(), bus, unit, func, reg, type, result);
			break;
		}

		case 0x864/4:		/* timer/counter control */
			break;
		
		default:
			logerror("%06X:Galileo read from offset %03X = %08X\n", activecpu_get_pc(), offset*4, result);
			break;
	}
	
	return result;
}


static WRITE32_HANDLER( galileo_w )
{
	UINT32 oldata = galileo_regs[offset];
	COMBINE_DATA(&galileo_regs[offset]);
	
	/* switch off the offset for special cases */
	switch (offset)
	{
		case 0x840/4:		/* DMA channel 0 control */
		case 0x844/4:		/* DMA channel 1 control */
		case 0x848/4:		/* DMA channel 2 control */
		case 0x84c/4:		/* DMA channel 3 control */
		{
			int which = offset % 4;

			/* keep the read only activity bit */
			galileo_regs[offset] &= ~0x4000;
			galileo_regs[offset] |= (oldata & 0x4000);

			/* fetch next record */
			if (data & 0x2000)
				dma_fetch_next(which);
			galileo_regs[offset] &= ~0x2000;
			
			/* if enabling, start the DMA */
			if (!(oldata & 0x1000) && (data & 0x1000))
				perform_dma(which);
			break;
		}
	
		case 0x850/4:		/* timer/counter 0 reset value */
		case 0x854/4:		/* timer/counter 1 reset value */
		case 0x858/4:		/* timer/counter 2 reset value */
		case 0x85c/4:		/* timer/counter 3 reset value */
		{
			int which = offset % 4;
			
			if (which != 0)
				data &= 0xffffff;
			if (!timer_active[which])
				timer_start[which] = data;
			if (LOG_TIMERS)
				logerror("%06X:timer/counter %d count = %08X [start=%08X]\n", activecpu_get_pc(), offset % 4, data, timer_start[which]);
			break;
		}
		
		case 0x864/4:		/* timer/counter control */
		{
			int which, mask;
			
			if (LOG_TIMERS)
				logerror("%06X:timer/counter control = %08X\n", activecpu_get_pc(), data);
			for (which = 0, mask = 0x01; which < 4; which++, mask <<= 2)
			{
				if (!timer_active[which] && (data & mask))
				{
					timer_active[which] = 1;
					timer_adjust(timer[which], TIMER_CLOCK * timer_start[which], which, 0);
					if (LOG_TIMERS)
						logerror("Adjusted timer to fire in %f secs\n", TIMER_CLOCK * timer_start[which]);
				}
				else if (timer_active[which] && !(data & mask))
				{
					UINT32 elapsed = (UINT32)(timer_timeelapsed(timer[which]) / TIMER_CLOCK);
					timer_active[which] = 0;
					timer_start[which] = (timer_start[which] > elapsed) ? (timer_start[which] - elapsed) : 0;
					timer_adjust(timer[which], TIME_NEVER, which, 0);
					if (LOG_TIMERS)
						logerror("Disabled timer\n");
				}
			}
			break;
		}
		
		case 0xc18/4:		/* IRQ clear */
			logerror("%06X:Galileo write to IRQ clear = %08X & %08X\n", offset*4, data, ~mem_mask);
			galileo_regs[offset] = oldata & data;
			update_galileo_irqs();
			break;
		
		case 0xcf8/4:		/* configuration address */
			break;

		case 0xcfc/4:		/* configuration data */
		{
			int bus = (galileo_regs[0xcf8/4] >> 16) & 0xff;
			int unit = (galileo_regs[0xcf8/4] >> 11) & 0x1f;
			int func = (galileo_regs[0xcf8/4] >> 8) & 7;
			int reg = (galileo_regs[0xcf8/4] >> 2) & 0x3f;
			int type = galileo_regs[0xcf8/4] & 3;

			/* unit 0 is the PCI bridge */
			if (unit == 0 && func == 0)
				pci_bridge_w(reg >> 2, type, data);
			
			/* unit 6 is the 3dfx card */
			else if (unit == 6 && func == 0)
				pci_3dfx_w(reg, type, data);
			
			/* anything else, just log */
			else
				logerror("%06X:PCIBus write: bus %d unit %d func %d reg %d type %d = %08X\n", activecpu_get_pc(), bus, unit, func, reg, type, data);
			break;
		}
		
		default:
			logerror("%06X:Galileo write to offset %03X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, ~mem_mask);
			break;
	}
}



/*************************************
 *
 *	Misc accesses
 *
 *************************************/

static WRITE32_HANDLER( seattle_watchdog_w )
{
	activecpu_eat_cycles(100);
}


static WRITE32_HANDLER( asic_reset_w )
{
	COMBINE_DATA(asic_reset);
	if (!(*asic_reset & 0x0002))
		midway_ioasic_reset();
}


static WRITE32_HANDLER( asic_fifo_w )
{
	midway_ioasic_fifo_w(data);
}



/*************************************
 *
 *	Misc unknown accesses
 *
 *************************************/

static READ32_HANDLER( unknown1_r )
{
logerror("%06X:unknown1_r\n", activecpu_get_pc());
	// code at 1FC10248 loops until this returns non-zero in bit 6
	return 0x0040;
}



/*************************************
 *
 *	Analog input handling
 *
 *************************************/

static READ32_HANDLER( analog_port_r )
{
	return pending_analog_read;
}


static WRITE32_HANDLER( analog_port_w )
{
	if (data < 8 || data > 15)
		logerror("%08X:Unexpected analog port select = %08X\n", activecpu_get_pc(), data);
	pending_analog_read = readinputport(4 + (data & 7));
}



/*************************************
 *
 *	CarnEvil gun handling
 *
 *************************************/

INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = (((readinputport(4 + player * 2) & 0xff) << 4) * Machine->visible_area.max_x) / 0xfff;
	*y = (((readinputport(5 + player * 2) & 0xff) << 2) * Machine->visible_area.max_y) / 0x3ff;
}


static VIDEO_UPDATE( carnevil )
{
	int beamx, beamy;
	
	/* first do common video update */
	video_update_voodoo(bitmap, cliprect);
	
	/* now draw the crosshairs */
	get_crosshair_xy(0, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect);
	get_crosshair_xy(1, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect);
}


static READ32_HANDLER( carnevil_gun_r )
{
	data32_t result = 0;

	switch (offset)
	{
		case 0:		/* low 8 bits of X */
			result = (readinputport(4) << 4) & 0xff;
			break;
		
		case 1:		/* upper 4 bits of X */
			result = (readinputport(4) >> 4) & 0x0f;
			result |= (readinputport(8) & 0x03) << 4;
			result |= 0x40;
			break;
			
		case 2:		/* low 8 bits of Y */
			result = (readinputport(5) << 2) & 0xff;
			break;
		
		case 3:		/* upper 4 bits of Y */
			result = (readinputport(5) >> 6) & 0x03;
			break;
			
		case 4:		/* low 8 bits of X */
			result = (readinputport(6) << 4) & 0xff;
			break;
		
		case 5:		/* upper 4 bits of X */
			result = (readinputport(6) >> 4) & 0x0f;
			result |= (readinputport(8) & 0x30);
			result |= 0x40;
			break;
			
		case 6:		/* low 8 bits of Y */
			result = (readinputport(7) << 2) & 0xff;
			break;
		
		case 7:		/* upper 4 bits of Y */
			result = (readinputport(7) >> 6) & 0x03;
			break;
	}
	return result;
}


static WRITE32_HANDLER( carnevil_gun_w )
{
	logerror("carnevil_gun_w(%d) = %02X\n", offset, data);
}



/*************************************
 *
 *	Speedups
 *
 *************************************/

static READ32_HANDLER( generic_speedup_r )
{
	activecpu_eat_cycles(100);
	return *generic_speedup;
}


static WRITE32_HANDLER( generic_speedup_w )
{
	activecpu_eat_cycles(100);
	COMBINE_DATA(generic_speedup);
}


static READ32_HANDLER( generic_speedup2_r )
{
	activecpu_eat_cycles(100);
	return *generic_speedup2;
}



/*************************************
 *
 *	Memory maps
 *
 *************************************/

static MEMORY_READ32_START( seattle_readmem )
	{ 0x00000000, 0x007fffff, MRA32_BANK1 },
	{ 0x80000000, 0x807fffff, MRA32_BANK2 },
	{ 0x88000000, 0x883fffff, voodoo_regs_r },
	{ 0x88400000, 0x887fffff, voodoo_framebuf_r },
	{ 0x9fc00000, 0x9fc7ffff, MRA32_BANK3 },
	{ 0xa0000000, 0xa07fffff, MRA32_RAM },			// wg3dh only has 4MB; sfrush, blitz99 8MB
//{ 0xa1000000, 0xa100000f, MRA32_RAM },
//{ 0xa4000000, 0xa400000f, MRA32_RAM },
//{ 0xa6000000, 0xa600000f, MRA32_RAM },
	{ 0xa8000000, 0xa83fffff, voodoo_regs_r },
	{ 0xa8400000, 0xa87fffff, voodoo_framebuf_r },
	{ 0xaa000000, 0xaa0003ff, ide_controller32_0_r },
	{ 0xaa00040c, 0xaa00040f, MRA32_NOP },			// IDE-related, but annoying
	{ 0xaa000f00, 0xaa000f07, ide_bus_master32_0_r },
	{ 0xac000000, 0xac000fff, galileo_r },
	{ 0xb6000000, 0xb600003f, midway_ioasic_r },
	{ 0xb6100000, 0xb611ffff, cmos_r },
	{ 0xb7300000, 0xb7300003, MRA32_RAM },
	{ 0xb7400000, 0xb7400003, MRA32_RAM },
	{ 0xb7500000, 0xb7500003, vblank_signalled_r },
	{ 0xb7600000, 0xb7600003, unknown1_r },
	{ 0xb7900000, 0xb7900003, MRA32_NOP },			// very noisy -- status LEDs?
	{ 0xb7f00000, 0xb7f00003, MRA32_RAM },
	{ 0xbfc00000, 0xbfc7ffff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( seattle_writemem )
	{ 0x00000000, 0x007fffff, MWA32_BANK1 },
	{ 0x80000000, 0x807fffff, MWA32_BANK2 },
	{ 0x88000000, 0x883fffff, voodoo_regs_w },
	{ 0x88400000, 0x887fffff, voodoo_framebuf_w },
	{ 0x88800000, 0x88ffffff, voodoo_textureram_w },
	{ 0x9fc00000, 0x9fc7ffff, MWA32_ROM },
	{ 0xa0000000, 0xa07fffff, MWA32_RAM, &rambase },			// wg3dh only has 4MB
//{ 0xa1000000, 0xa100000f, MWA32_RAM },
//{ 0xa4000000, 0xa400000f, MWA32_RAM },
//{ 0xa6000000, 0xa600000f, MWA32_RAM },
	{ 0xa8000000, 0xa83fffff, voodoo_regs_w, &voodoo_regs },
	{ 0xa8400000, 0xa87fffff, voodoo_framebuf_w },
	{ 0xa8800000, 0xa8ffffff, voodoo_textureram_w },
	{ 0xaa000000, 0xaa0003ff, ide_controller32_0_w },
	{ 0xaa000f00, 0xaa000f07, ide_bus_master32_0_w },
	{ 0xac000000, 0xac000fff, galileo_w, &galileo_regs },
	{ 0xb3000000, 0xb3000003, asic_fifo_w },
	{ 0xb6000000, 0xb600003f, midway_ioasic_w },
	{ 0xb6100000, 0xb611ffff, cmos_w, (data32_t **)&generic_nvram, &generic_nvram_size },
	{ 0xb7100000, 0xb7100003, seattle_watchdog_w },
	{ 0xb7300000, 0xb7300003, vblank_enable_w, &vblank_enable },
	{ 0xb7400000, 0xb7400003, vblank_config_w, &vblank_config },
	{ 0xb7700000, 0xb7700003, vblank_clear_w },
	{ 0xb7800000, 0xb7800003, MWA32_NOP },						// unknown
	{ 0xb7900000, 0xb7900003, MWA32_NOP },						// very noisy -- status LEDs?
	{ 0xb7f00000, 0xb7f00003, asic_reset_w, &asic_reset },
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM, &rombase },
MEMORY_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( wg3dh )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown0040" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* 3d cam */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( mace )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x0000, "Resolution" )
	PORT_DIPSETTING(      0x8000, "Low" )
	PORT_DIPSETTING(      0x0000, "Medium" )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* 3d cam */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( sfrush )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown0040" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )	/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )	/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )	/* abort */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 	/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* reverse */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )	/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )	/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* view 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* view 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )	/* view 3 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )	/* music */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )	/* track 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )	/* track 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )	/* track 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER3 )	/* track 4 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	/* 4th gear */
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX(0x2000, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2, 25, 100, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 100, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )
INPUT_PORTS_END


INPUT_PORTS_START( calspeed )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )	/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )	/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )	/* abort */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 	/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* reverse */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )	/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )	/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* view 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* view 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )	/* view 3 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )	/* music */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )	/* track 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )	/* track 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )	/* track 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER3 )	/* track 4 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	/* 4th gear */
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX(0x2000, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2, 25, 100, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 100, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )
INPUT_PORTS_END


INPUT_PORTS_START( carnevil )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA 1" )
	PORT_DIPSETTING(      0x003c, "USA 2" )
	PORT_DIPSETTING(      0x003a, "USA 3" )
	PORT_DIPSETTING(      0x0038, "USA 4" )
	PORT_DIPSETTING(      0x0036, "USA 5" )
	PORT_DIPSETTING(      0x0034, "USA 6" )
	PORT_DIPSETTING(      0x0032, "USA 7" )
	PORT_DIPSETTING(      0x0030, "USA ECA" )
	PORT_DIPSETTING(      0x002e, "France 1" )
	PORT_DIPSETTING(      0x002c, "France 2" )
	PORT_DIPSETTING(      0x002a, "France 3" )
	PORT_DIPSETTING(      0x0028, "France 4" )
	PORT_DIPSETTING(      0x0026, "France 5" )
	PORT_DIPSETTING(      0x0024, "France 6" )
	PORT_DIPSETTING(      0x0022, "France 7" )
	PORT_DIPSETTING(      0x0020, "France ECA" )
	PORT_DIPSETTING(      0x001e, "German 1" )
	PORT_DIPSETTING(      0x001c, "German 2" )
	PORT_DIPSETTING(      0x001a, "German 3" )
	PORT_DIPSETTING(      0x0018, "German 4" )
	PORT_DIPSETTING(      0x0016, "German 5" )
//	PORT_DIPSETTING(      0x0014, "German 5" )
//	PORT_DIPSETTING(      0x0012, "German 5" )
	PORT_DIPSETTING(      0x0010, "German ECA" )
	PORT_DIPSETTING(      0x000e, "U.K. 1" )
	PORT_DIPSETTING(      0x000c, "U.K. 2" )
	PORT_DIPSETTING(      0x000a, "U.K. 3" )
	PORT_DIPSETTING(      0x0008, "U.K. 4" )
	PORT_DIPSETTING(      0x0006, "U.K. 5" )
	PORT_DIPSETTING(      0x0004, "U.K. 6" )
	PORT_DIPSETTING(      0x0002, "U.K. 7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0100, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0600, 0x0400, "Resolution" )
//	PORT_DIPSETTING(      0x0600, "0" )
//	PORT_DIPSETTING(      0x0200, "Medium" )
	PORT_DIPSETTING(      0x0400, "Low" )
//	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, "45 MHz" )
	PORT_DIPSETTING(      0x0800, "47 MHz" )
	PORT_DIPSETTING(      0x1000, "49 MHz" )
	PORT_DIPSETTING(      0x1800, "51 MHz" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x2000, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power Up Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0780, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 70, 10, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 70, 10, 0, 255 )

	PORT_START				/* fake switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
INPUT_PORTS_END


INPUT_PORTS_START( blitz99 )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA 1" )
	PORT_DIPSETTING(      0x003c, "USA 2" )
	PORT_DIPSETTING(      0x003a, "USA 3" )
	PORT_DIPSETTING(      0x0038, "USA 4" )
	PORT_DIPSETTING(      0x0036, "USA 5" )
	PORT_DIPSETTING(      0x0034, "USA 6" )
	PORT_DIPSETTING(      0x0032, "USA 7" )
	PORT_DIPSETTING(      0x0030, "USA ECA" )
	PORT_DIPSETTING(      0x002e, "France 1" )
	PORT_DIPSETTING(      0x002c, "France 2" )
	PORT_DIPSETTING(      0x002a, "France 3" )
	PORT_DIPSETTING(      0x0028, "France 4" )
	PORT_DIPSETTING(      0x0026, "France 5" )
	PORT_DIPSETTING(      0x0024, "France 6" )
	PORT_DIPSETTING(      0x0022, "France 7" )
	PORT_DIPSETTING(      0x0020, "France ECA" )
	PORT_DIPSETTING(      0x001e, "German 1" )
	PORT_DIPSETTING(      0x001c, "German 2" )
	PORT_DIPSETTING(      0x001a, "German 3" )
	PORT_DIPSETTING(      0x0018, "German 4" )
	PORT_DIPSETTING(      0x0016, "German 5" )
//	PORT_DIPSETTING(      0x0014, "German 5" )
//	PORT_DIPSETTING(      0x0012, "German 5" )
	PORT_DIPSETTING(      0x0010, "German ECA" )
	PORT_DIPSETTING(      0x000e, "U.K. 1 ECA" )
	PORT_DIPSETTING(      0x000c, "U.K. 2 ECA" )
	PORT_DIPSETTING(      0x000a, "U.K. 3 ECA" )
	PORT_DIPSETTING(      0x0008, "U.K. 4" )
	PORT_DIPSETTING(      0x0006, "U.K. 5" )
	PORT_DIPSETTING(      0x0004, "U.K. 6 ECA" )
	PORT_DIPSETTING(      0x0002, "U.K. 7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0100, 0x0100, "Joysticks" )
	PORT_DIPSETTING(      0x0100, "8-Way" )
	PORT_DIPSETTING(      0x0000, "49-Way" )
	PORT_DIPNAME( 0x0600, 0x0200, "Resolution" )
//	PORT_DIPSETTING(      0x0600, "0" )
	PORT_DIPSETTING(      0x0200, "Medium" )
	PORT_DIPSETTING(      0x0400, "Low" )
//	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1800, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x1800, "0" )
	PORT_DIPSETTING(      0x0800, "1" )
	PORT_DIPSETTING(      0x1000, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x2000, 0x0000, "Players" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power Up Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static struct mips3_config config =
{
	16384,	/* code cache size */
	16384	/* data cache size */
};

MACHINE_DRIVER_START( seattle_flagstaff_common )
	
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", R5000LE, 48000000*3)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_MEMORY(seattle_readmem,seattle_writemem)
	MDRV_CPU_VBLANK_INT(assert_vblank,1)
	
	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seattle)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 384)
	MDRV_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(voodoo_1x4mb)
	MDRV_VIDEO_STOP(voodoo)
	MDRV_VIDEO_UPDATE(voodoo)
	
	/* sound hardware */
MACHINE_DRIVER_END


MACHINE_DRIVER_START( seattle150 )
	MDRV_IMPORT_FROM(seattle_flagstaff_common)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(dcs2_audio)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( seattle200 )
	MDRV_IMPORT_FROM(seattle150)
	MDRV_CPU_REPLACE("main", R5000LE, 48000000*4)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( carnevil )
	MDRV_IMPORT_FROM(seattle150)
	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VIDEO_UPDATE(carnevil)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( blitz99 )
	MDRV_IMPORT_FROM(seattle150)
	MDRV_CPU_REPLACE("dcs2", ADSP2115, 12000000)	/* NOT confirmed, just a guess */
MACHINE_DRIVER_END


MACHINE_DRIVER_START( flagstaff )
	MDRV_IMPORT_FROM(seattle_flagstaff_common)
	MDRV_CPU_REPLACE("main", R5000LE, 48000000*4)

	/* video hardware */
	MDRV_VIDEO_START(voodoo_2x4mb)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(cage_seattle)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( wg3dh )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( ADSP2100_SIZE + 0x408000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "wg3dh_11.u95", ADSP2100_SIZE + 0x000000, 0x8000, CRC(c589458c) SHA1(0cf970a35910a74cdcf3bd8119bfc0c693e19b00) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "wg3dh_12.u32", 0x000000, 0x80000, CRC(15e4cea2) SHA1(72c0db7dc53ce645ba27a5311b5ce803ad39f131) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "wg3dh.chd", 0, MD5(424dbda376e8c45ec873b79194bdb924) )
ROM_END


ROM_START( mace )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( ADSP2100_SIZE + 0x408000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "mace-snd.u95", ADSP2100_SIZE + 0x000000, 0x8000, CRC(c589458c) SHA1(0cf970a35910a74cdcf3bd8119bfc0c693e19b00) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "maceboot.u32", 0x000000, 0x80000, CRC(effe3ebc) SHA1(7af3ca3580d6276ffa7ab8b4c57274e15ee6bcbb) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "mace.chd", 0, BAD_DUMP MD5(08d8982d818ec70554e2b66e4efcb4bc) )
ROM_END


ROM_START( sfrush )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( 0x040000, REGION_CPU2, 0 )		/* RAM for TMS320C31 */

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "hdboot.u32", 0x000000, 0x80000, CRC(39a35f1b) SHA1(c46d83448399205d38e6e41dd56abbc362254254) )

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )	/* TMS320C31 boot ROM */
	ROM_LOAD32_BYTE( "sndboot.u69", 0x000000, 0x080000, CRC(7e52cdc7) SHA1(f735063e19d2ca672cef6d761a2a47df272e8c59) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 )	/* TMS320C31 sound ROMs */
	ROM_LOAD32_WORD( "sfrush.u62",  0x400000, 0x200000, CRC(5d66490e) SHA1(bd39ea3b45d44cae6ca5890f365653326bbecd2d) )
	ROM_LOAD32_WORD( "sfrush.u61",  0x400002, 0x200000, CRC(f3a00ee8) SHA1(c1ac780efc32b2e30522d7cc3e6d92e7daaadddd) )
	ROM_LOAD32_WORD( "sfrush.u53",  0x800000, 0x200000, CRC(71f8ddb0) SHA1(c24bef801f43bae68fda043c4356e8cf1298ca97) )
	ROM_LOAD32_WORD( "sfrush.u49",  0x800002, 0x200000, CRC(dfb0a54c) SHA1(ed34f9485f7a7e5bb73bf5c6428b27548e12db12) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "sfrush.chd", 0, MD5(7a77addb141fc11fd5ca63850382e0d1) )
ROM_END


ROM_START( calspeed )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( ADSP2100_SIZE + 0x408000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "mace-snd.u95", ADSP2100_SIZE + 0x000000, 0x8000, NO_DUMP CRC(c589458d) SHA1(f735063e19d2ca672cef6d761a2a47df272e8c59) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "caspd1_2.u32", 0x000000, 0x80000, CRC(0a235e4e) SHA1(b352f10fad786260b58bd344b5002b6ea7aaf76d) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "calspeed.chd", 0, BAD_DUMP MD5(dc8c919af86a1ab88a0b05ea2b6c74b3) )
ROM_END


ROM_START( blitz99 )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( ADSP2100_SIZE + 0x408000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "blitz99.u95", ADSP2100_SIZE + 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "blitz99.u32", 0x000000, 0x80000, CRC(777119b2) SHA1(40d255181c2f3a787919c339e83593fd506779a5) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "blitz99.chd", 0, BAD_DUMP MD5(678d10f1089539b026d6e5570ea8749b) )
ROM_END


ROM_START( carnevil )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )		/* dummy R5000 region */
	
	ROM_REGION( ADSP2100_SIZE + 0x408000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "sound102.u95", ADSP2100_SIZE + 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "boot.u32", 0x000000, 0x80000, CRC(82c07f2e) SHA1(fa51c58022ce251c53bad12fc6ffadb35adb8162) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "carnevil.chd", 0, BAD_DUMP MD5(6eafae86091c0a915cf8cfdc3d73adc2) )
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static void init_common(int ioasic, int serialnum)
{
	/* initialize the subsystems */
	ide_controller_init(0, &ide_intf);
	midway_ioasic_init(ioasic, serialnum, 80, ioasic_irq);

	/* copy the boot ROM into its home location */
	memcpy(rombase, memory_region(REGION_USER1), memory_region_length(REGION_USER1));

	/* set the fastest DRC options, but strict verification */	
	mips3drc_set_options(0, MIPS3DRC_FASTEST_OPTIONS + MIPS3DRC_STRICT_VERIFY);
}


static DRIVER_INIT( wg3dh )
{
	dcs2_init(0x3839);
	init_common(MIDWAY_IOASIC_STANDARD, 450);
	
	/* speedups */
	install_mem_read32_handler(0, 0x80115e00, 0x80115e03, generic_speedup_r);
	generic_speedup = &rambase[0x115e00/4];
}


static DRIVER_INIT( mace )
{
	dcs2_init(0x3839);
	init_common(MIDWAY_IOASIC_MACE, 450);
	
	/* no obvious speedups */
}


static DRIVER_INIT( calspeed )
{
	dcs2_init(0x39c0);
	init_common(MIDWAY_IOASIC_CALSPEED, 450);

	/* set up the analog inputs */
	install_mem_read32_handler(0, 0xb6c00010, 0xb6c00013, analog_port_r);
	install_mem_write32_handler(0, 0xb6c00010, 0xb6c00013, analog_port_w);
/*
cpu #0 (PC=80014048): unmapped memory dword write to B6C0001C = 00000007 & FFFFFFFF
cpu #0 (PC=80014048): unmapped memory dword write to B6C00004 = 00000004 & FFFFFFFF
*/
}


static DRIVER_INIT( sfrush )
{
	cage_init(REGION_USER2, 0x5236);
	init_common(MIDWAY_IOASIC_STANDARD, 315);

	/* set up the analog inputs */
	install_mem_read32_handler(0, 0xb4000000, 0xb4000003, analog_port_r);
	install_mem_write32_handler(0, 0xb4000000, 0xb4000003, analog_port_w);
	
	/* speedups */
	install_mem_read32_handler(0, 0x8012498c, 0x8012498f, generic_speedup_r);
	generic_speedup = &rambase[0x12498c/4];
	install_mem_read32_handler(0, 0x80120000, 0x80120003, generic_speedup2_r);
	generic_speedup2 = &rambase[0x120000/4];
}


static DRIVER_INIT( blitz99 )
{
	dcs2_init(0x0afb);
	init_common(MIDWAY_IOASIC_BLITZ99, 520/* or 481 or 484 */);

	/* speedups */
	install_mem_write32_handler(0, 0x802502bc, 0x802502bf, generic_speedup_w);
	generic_speedup = &rambase[0x2502bc/4];
}


static DRIVER_INIT( carnevil )
{
	dcs2_init(0x0af7);
	init_common(MIDWAY_IOASIC_CARNEVIL, 528/* or 469 or 486 */);

	/* set up the gun */
	install_mem_read32_handler(0, 0xb6800000, 0xb680001f, carnevil_gun_r);
	install_mem_write32_handler(0, 0xb6800000, 0xb680001f, carnevil_gun_w);

	/* speedups */
	install_mem_write32_handler(0, 0x801a2bac, 0x801a2baf, generic_speedup_w);
	generic_speedup = &rambase[0x1a2bac/4];
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME ( 1996, wg3dh,    0,        seattle150, wg3dh,    wg3dh,    ROT0, "Atari Games", "Wayne Gretzky's 3D Hockey" )
GAME ( 1996, mace,     0,        seattle200, mace,     mace,     ROT0, "Atari Games", "Mace: The Dark Age" )
GAMEX( 1996, sfrush,   0,        flagstaff,  sfrush,   sfrush,   ROT0, "Atari Games", "San Francisco Rush", GAME_NOT_WORKING )
GAMEX( 1996, calspeed, 0,        seattle150, calspeed, calspeed, ROT0, "Atari Games", "California Speed", GAME_NOT_WORKING )

GAME ( 1998, carnevil, 0,        carnevil,   carnevil, carnevil, ROT0, "Midway Games", "CarnEvil" )
GAME ( 1998, blitz99,  0,        blitz99,    blitz99,  blitz99,  ROT0, "Midway Games", "NFL Blitz '99" )
