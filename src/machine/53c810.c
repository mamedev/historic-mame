/* LSI Logic LSI53C810A PCI to SCSI I/O Processor */

#include "driver.h"

#define DMA_MAX_ICOUNT	512		/* Maximum number of DMA Scripts opcodes to run */

static UINT8 *ram;

static struct {
	UINT8 istat;
	UINT8 dstat;
	UINT8 dien;
	UINT8 dcntl;
	UINT8 dmode;
	UINT32 dsp;
	UINT32 dsps;
	UINT32 dcmd;
	int dma_icount;

	void (* irq_callback)(void);
} lsi810;

#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

static void (* dma_opcode[256])(void);


INLINE UINT32 FETCH(void)
{
	UINT32 r = BYTE_REVERSE32(*(UINT32*)&ram[lsi810.dsp ^ 4]);
	lsi810.dsp += 4;
	return r;
}

static void dmaop_invalid(void)
{
	osd_die("LSI53C810: Invalid SCRIPTS DMA opcode %08X at %08X\n", lsi810.dcmd, lsi810.dsp);
}

static void dmaop_move_memory(void)
{
	UINT32 src = FETCH();
	UINT32 dst = FETCH();
	int count;

	count = lsi810.dcmd & 0xffffff;
	logerror("LSI53C810: Unimplemented MOVE MEMORY %08X, %08X, %d\n", src, dst, count);
}

static void dmaop_interrupt(void)
{
	if(lsi810.dcmd & 0x100000) {
		osd_die("LSI53C810: INTFLY opcode not implemented\n");
	}
	lsi810.dsps = FETCH();

	lsi810.istat |= 0x1;	/* DMA interrupt pending */
	lsi810.dstat |= 0x4;	/* SIR (SCRIPTS Interrupt Instruction Received) */

	if(lsi810.irq_callback != NULL) {
		lsi810.irq_callback();
	}

	lsi810.dma_icount = 0;
}



static void dma_exec(void)
{
	lsi810.dma_icount = DMA_MAX_ICOUNT;

	while(lsi810.dma_icount > 0)
	{
		int op;
		lsi810.dcmd = FETCH();

		op = (lsi810.dcmd >> 24) & 0xff;
		dma_opcode[op]();
		
		lsi810.dma_icount--;
	}
}

UINT8 lsi53c810_reg_r(int reg)
{
	switch(reg)
	{
		case 0x0c:		/* DSTAT */
			return lsi810.dstat;
		case 0x14:		/* ISTAT */
			return lsi810.istat;
		case 0x2c:		/* DSP [7-0] */
			return lsi810.dsp & 0xff;
		case 0x2d:		/* DSP [15-8] */
			return (lsi810.dsp >> 8) & 0xff;
		case 0x2e:		/* DSP [23-16] */
			return (lsi810.dsp >> 16) & 0xff;
		case 0x2f:		/* DSP [31-24] */
			return (lsi810.dsp >> 24) & 0xff;
		case 0x39:		/* DIEN */
			return lsi810.dien;
		case 0x3b:		/* DCNTL */
			return lsi810.dcntl;

		default:
			osd_die("LSI53C810: reg_r: Unknown reg %02X\n", reg);
	}
	
	return 0;
}

void lsi53c810_reg_w(int reg, UINT8 value)
{
	switch(reg)
	{
		case 0x14:		/* ISTAT */
			lsi810.istat = value;
			break;
		case 0x2c:		/* DSP [7-0] */
			lsi810.dsp &= 0xffffff00;
			lsi810.dsp |= value;
			break;
		case 0x2d:		/* DSP [15-8] */
			lsi810.dsp &= 0xffff00ff;
			lsi810.dsp |= value << 8;
			break;
		case 0x2e:		/* DSP [23-16] */
			lsi810.dsp &= 0xff00ffff;
			lsi810.dsp |= value << 16;
			break;
		case 0x2f:		/* DSP [31-24] */
			lsi810.dsp &= 0x00ffffff;
			lsi810.dsp |= value << 24;
			if((lsi810.dmode & 0x1) == 0) {
				dma_exec();
			}
			break;
		case 0x38:		/* DMODE */
			lsi810.dmode = value;
			break;
		case 0x39:		/* DIEN */
			lsi810.dien = value;
			break;
		case 0x3b:		/* DCNTL */
			lsi810.dcntl = value;

			if(lsi810.dcntl & 0x14)			/* single-step & start DMA */
			{
				int op;
				lsi810.dcmd = FETCH();
				op = (lsi810.dcmd >> 24) & 0xff;
				dma_opcode[op]();

				lsi810.istat |= 0x1;	/* DMA interrupt pending */
				lsi810.dstat |= 0x8;	/* SSI (Single Step Interrupt) */
				if(lsi810.irq_callback != NULL) {
					lsi810.irq_callback();
				}
			}
			else if(lsi810.dcntl & 0x04)	/* manual start DMA */
			{
				dma_exec();
			}
			break;

		default:
			osd_die("LSI53C810: reg_w: Unknown reg %02X, %02X\n", reg, value);
	}
}

static void add_opcode(UINT8 op, UINT8 mask, void *handler)
{
	int i;
	for(i=0; i < 256; i++) {
		if((i & mask) == op) {
			dma_opcode[i] = handler;
		}
	}
}

void lsi53c810_init(UINT8 *mem, void (*irq_callback)(void))
{
	int i;
	memset(&lsi810, 0, sizeof(lsi810));
	for(i = 0; i < 256; i++)
	{
		dma_opcode[i] = dmaop_invalid;
	}

	ram = (UINT8*)mem;
	lsi810.irq_callback = irq_callback;

	add_opcode(0xc0, 0xfe, dmaop_move_memory);
	add_opcode(0x98, 0xf8, dmaop_interrupt);
}
