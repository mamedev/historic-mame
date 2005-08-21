/* LSI Logic LSI53C810A PCI to SCSI I/O Processor */

#include "driver.h"
#include "scsidev.h"

#define DMA_MAX_ICOUNT	512		/* Maximum number of DMA Scripts opcodes to run */

static struct {
	UINT8 scntl0;
	UINT8 scntl1;
	UINT8 scntl2;
	UINT8 scntl3;
	UINT8 scid;
	UINT8 istat;
	UINT8 dstat;
	UINT8 dien;
	UINT8 dcntl;
	UINT8 dmode;
	UINT32 dsa;
	UINT32 dsp;
	UINT32 dsps;
	UINT32 dcmd;
	UINT8 sien0;
	UINT8 sien1;
	UINT8 stime0;
	UINT8 respid;
	UINT8 stest1;
	UINT8 scratch_a[4];
	UINT8 scratch_b[4];
	int dma_icount;
	int halted;

	struct
	{
		void *data;		// device's "this" pointer
		pSCSIDispatch handler;	// device's handler routine
	} devices[8];

	UINT32 (* fetch)(UINT32 dsp);
	void (* irq_callback)(void);
	void (* dma_callback)(UINT32, UINT32, int, int);
} lsi810;

static void (* dma_opcode[256])(void);


INLINE UINT32 FETCH(void)
{
	UINT32 r = lsi810.fetch(lsi810.dsp);
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
	if(lsi810.dma_callback != NULL) {
		lsi810.dma_callback(src, dst, count, 1);
	}
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
	lsi810.halted = 1;
}

static void dmaop_block_move(void)
{
	UINT32 address;
	UINT32 count;

	address = FETCH();
	if (lsi810.dcmd & 0x20000000)
		address = lsi810.fetch(address);

	if (lsi810.dcmd & 0x10000000)
		osd_die("LSI53C810: Table indirect not implemented\n");

	count = lsi810.dcmd & 0x00ffffff;

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_block_move not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_block_move not implemented\n");
	}
}

static void dmaop_select(void)
{
	UINT32 operand;

	operand = FETCH();

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_select not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_select not implemented\n");
	}
}

static void dmaop_wait_disconnect(void)
{
	UINT32 operand;

	operand = FETCH();

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_wait_disconnect not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_wait_disconnect not implemented\n");
	}
}

static void dmaop_wait_reselect(void)
{
	UINT32 operand;

	operand = FETCH();

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_wait_reselect not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_wait_reselect not implemented\n");
	}
}

static void dmaop_set(void)
{
	UINT32 operand;

	operand = FETCH();

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_set not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_set not implemented\n");
	}
}

static void dmaop_clear(void)
{
	UINT32 operand;

	operand = FETCH();

	if (lsi810.scntl0 & 0x01)
	{
		/* target mode */
		osd_die("LSI53C810: dmaop_clear not implemented\n");
	}
	else
	{
		/* initiator mode */
		osd_die("LSI53C810: dmaop_clear not implemented\n");
	}
}

static void dmaop_move_from_sfbr(void)
{
	osd_die("LSI53C810: dmaop_move_from_sfbr not implemented\n");
}

static void dmaop_move_to_sfbr(void)
{
	osd_die("LSI53C810: dmaop_move_to_sfbr not implemented\n");
}

static void dmaop_read_modify_write(void)
{
	osd_die("LSI53C810: dmaop_read_modify_write not implemented\n");
}

static void dmaop_jump(void)
{
	UINT32 dsps, dest;

	dsps = FETCH();

	/* relative or absolute addressing? */
	if (lsi810.dcmd & 0x08000000)
		dest = dsps + ((lsi810.dcmd & 0x00FFFFFF) | ((lsi810.dcmd & 0x00800000) ? 0xFF000000 : 0));
	else
		dest = dsps;

	osd_die("LSI53C810: dmaop_jump not implemented\n");
}

static void dmaop_call(void)
{
	UINT32 dsps, dest;

	dsps = FETCH();

	/* relative or absolute addressing? */
	if (lsi810.dcmd & 0x08000000)
		dest = dsps + ((lsi810.dcmd & 0x00FFFFFF) | ((lsi810.dcmd & 0x00800000) ? 0xFF000000 : 0));
	else
		dest = dsps;

	osd_die("LSI53C810: dmaop_call not implemented\n");
}

static void dmaop_return(void)
{
	UINT32 dsps, dest;

	dsps = FETCH();

	/* relative or absolute addressing? */
	if (lsi810.dcmd & 0x08000000)
		dest = dsps + ((lsi810.dcmd & 0x00FFFFFF) | ((lsi810.dcmd & 0x00800000) ? 0xFF000000 : 0));
	else
		dest = dsps;

	osd_die("LSI53C810: dmaop_return not implemented\n");
}

static void dmaop_store(void)
{
	osd_die("LSI53C810: dmaop_store not implemented\n");
}

static void dmaop_load(void)
{
	osd_die("LSI53C810: dmaop_load not implemented\n");
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
		case 0x00:		/* SCNTL0 */
			return lsi810.scntl0;
		case 0x01:		/* SCNTL1 */
			return lsi810.scntl1;
		case 0x02:		/* SCNTL2 */
			return lsi810.scntl2;
		case 0x03:		/* SCNTL3 */
			return lsi810.scntl3;
		case 0x04:		/* SCID */
			return lsi810.scid;
		case 0x0c:		/* DSTAT */
			return lsi810.dstat;
		case 0x10:		/* DSA [7-0] */
			return lsi810.dsa & 0xff;
		case 0x11:		/* DSA [15-8] */
			return (lsi810.dsa >> 8) & 0xff;
		case 0x12:		/* DSA [23-16] */
			return (lsi810.dsa >> 16) & 0xff;
		case 0x13:		/* DSA [31-24] */
			return (lsi810.dsa >> 24) & 0xff;
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
		case 0x34:		/* SCRATCH A */
		case 0x35:
		case 0x36:
		case 0x37:
			return lsi810.scratch_a[reg % 4];
		case 0x39:		/* DIEN */
			return lsi810.dien;
		case 0x3b:		/* DCNTL */
			return lsi810.dcntl;
		case 0x40:		/* SIEN0 */
			return lsi810.sien0;
		case 0x41:		/* SIEN1 */
			return lsi810.sien1;
		case 0x48:		/* STIME0 */
			return lsi810.stime0;
		case 0x4a:		/* RESPID */
			return lsi810.respid;
		case 0x4d:		/* STEST1 */
			return lsi810.stest1;
		case 0x5c:		/* SCRATCH B */
		case 0x5d:
		case 0x5e:
		case 0x5f:
			return lsi810.scratch_b[reg % 4];

		default:
			osd_die("LSI53C810: reg_r: Unknown reg %02X\n", reg);
	}

	return 0;
}

void lsi53c810_reg_w(int reg, UINT8 value)
{
	switch(reg)
	{
		case 0x00:		/* SCNTL0 */
			lsi810.scntl0 = value;
			break;
		case 0x01:		/* SCNTL1 */
			lsi810.scntl1 = value;
			break;
		case 0x02:		/* SCNTL2 */
			lsi810.scntl2 = value;
			break;
		case 0x03:		/* SCNTL3 */
			lsi810.scntl3 = value;
			break;
		case 0x04:		/* SCID */
			lsi810.scid = value;
			break;
		case 0x10:		/* DSA [7-0] */
			lsi810.dsa &= 0xffffff00;
			lsi810.dsa |= value;
			break;
		case 0x11:		/* DSA [15-8] */
			lsi810.dsa &= 0xffff00ff;
			lsi810.dsa |= value << 8;
			break;
		case 0x12:		/* DSA [23-16] */
			lsi810.dsa &= 0xff00ffff;
			lsi810.dsa |= value << 16;
			break;
		case 0x13:		/* DSA [31-24] */
			lsi810.dsa &= 0x00ffffff;
			lsi810.dsa |= value << 24;
			break;
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
			lsi810.halted = 0;
			if((lsi810.dmode & 0x1) == 0 && !lsi810.halted) {
				dma_exec();
			}
			break;
		case 0x34:		/* SCRATCH A */
		case 0x35:
		case 0x36:
		case 0x37:
			lsi810.scratch_a[reg % 4] = value;
			break;
		case 0x38:		/* DMODE */
			lsi810.dmode = value;
			break;
		case 0x39:		/* DIEN */
			lsi810.dien = value;
			break;
		case 0x3b:		/* DCNTL */
			lsi810.dcntl = value;

			if(lsi810.dcntl & 0x14 && !lsi810.halted)		/* single-step & start DMA */
			{
				int op;
				lsi810.dcmd = FETCH();
				op = (lsi810.dcmd >> 24) & 0xff;
				dma_opcode[op]();

				lsi810.istat |= 0x3;	/* DMA interrupt pending */
				lsi810.dstat |= 0x8;	/* SSI (Single Step Interrupt) */
				if(lsi810.irq_callback != NULL) {
					lsi810.irq_callback();
				}
			}
			else if(lsi810.dcntl & 0x04 && !lsi810.halted)	/* manual start DMA */
			{
				dma_exec();
			}
			break;
		case 0x40:		/* SIEN0 */
			lsi810.sien0 = value;
			break;
		case 0x41:		/* SIEN1 */
			lsi810.sien1 = value;
			break;
		case 0x48:		/* STIME0 */
			lsi810.stime0 = value;
			break;
		case 0x4a:		/* RESPID */
			lsi810.respid = value;
			break;
		case 0x4d:		/* STEST1 */
			lsi810.stest1 = value;
			break;
		case 0x5c:		/* SCRATCH B */
		case 0x5d:
		case 0x5e:
		case 0x5f:
			lsi810.scratch_b[reg % 4] = value;
			break;

		default:
			osd_die("LSI53C810: reg_w: Unknown reg %02X, %02X\n", reg, value);
	}
}

static void add_opcode(UINT8 op, UINT8 mask, void (* handler)(void))
{
	int i;
	for(i=0; i < 256; i++) {
		if((i & mask) == op) {
			dma_opcode[i] = handler;
		}
	}
}

void lsi53c810_init(UINT32 (*fetch)(UINT32 dsp), void (*irq_callback)(void), void (*dma_callback)(UINT32,UINT32,int,int))
{
	int i;
	memset(&lsi810, 0, sizeof(lsi810));
	for(i = 0; i < 256; i++)
	{
		dma_opcode[i] = dmaop_invalid;
	}

	lsi810.fetch = fetch;
	lsi810.irq_callback = irq_callback;
	lsi810.dma_callback = dma_callback;

	add_opcode(0x00, 0xc0, dmaop_block_move);
	add_opcode(0x40, 0xf8, dmaop_select);
	add_opcode(0x48, 0xf8, dmaop_wait_disconnect);
	add_opcode(0x50, 0xf8, dmaop_wait_reselect);
	add_opcode(0x58, 0xf8, dmaop_set);
	add_opcode(0x60, 0xf8, dmaop_clear);
	add_opcode(0x68, 0xf8, dmaop_move_from_sfbr);
	add_opcode(0x70, 0xf8, dmaop_move_to_sfbr);
	add_opcode(0x78, 0xf8, dmaop_read_modify_write);
	add_opcode(0x80, 0xf8, dmaop_jump);
	add_opcode(0x88, 0xf8, dmaop_call);
	add_opcode(0x90, 0xf8, dmaop_return);
	add_opcode(0x98, 0xf8, dmaop_interrupt);
	add_opcode(0xc0, 0xfe, dmaop_move_memory);
	add_opcode(0xe0, 0xed, dmaop_store);
	add_opcode(0xe1, 0xed, dmaop_load);
}
