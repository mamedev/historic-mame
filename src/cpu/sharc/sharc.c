/*
   Analog Devices ADSP-2106x SHARC emulator

   Written by Ville Linde

   Portions based on ElSemi's SHARC emulator
*/

#include "driver.h"
#include "sharc.h"
#include "mamedbg.h"

static void sharc_dma_exec(int channel);

enum {
	SHARC_PC=1,		SHARC_PCSTK,	SHARC_MODE1,	SHARC_MODE2,
	SHARC_ASTAT,	SHARC_STKY,		SHARC_IRPTL,	SHARC_IMASK,
	SHARC_IMASKP,	SHARC_USTAT1,	SHARC_USTAT2,	SHARC_LCNTR,
	SHARC_R0,		SHARC_R1,		SHARC_R2,		SHARC_R3,
	SHARC_R4,		SHARC_R5,		SHARC_R6,		SHARC_R7,
	SHARC_R8,		SHARC_R9,		SHARC_R10,		SHARC_R11,
	SHARC_R12,		SHARC_R13,		SHARC_R14,		SHARC_R15,
	SHARC_SYSCON,	SHARC_SYSSTAT,	SHARC_MRF,		SHARC_MRB,
	SHARC_I0,		SHARC_I1,		SHARC_I2,		SHARC_I3,
	SHARC_I4,		SHARC_I5,		SHARC_I6,		SHARC_I7,
	SHARC_I8,		SHARC_I9,		SHARC_I10,		SHARC_I11,
	SHARC_I12,		SHARC_I13,		SHARC_I14,		SHARC_I15,
	SHARC_M0,		SHARC_M1,		SHARC_M2,		SHARC_M3,
	SHARC_M4,		SHARC_M5,		SHARC_M6,		SHARC_M7,
	SHARC_M8,		SHARC_M9,		SHARC_M10,		SHARC_M11,
	SHARC_M12,		SHARC_M13,		SHARC_M14,		SHARC_M15,
	SHARC_L0,		SHARC_L1,		SHARC_L2,		SHARC_L3,
	SHARC_L4,		SHARC_L5,		SHARC_L6,		SHARC_L7,
	SHARC_L8,		SHARC_L9,		SHARC_L10,		SHARC_L11,
	SHARC_L12,		SHARC_L13,		SHARC_L14,		SHARC_L15,
	SHARC_B0,		SHARC_B1,		SHARC_B2,		SHARC_B3,
	SHARC_B4,		SHARC_B5,		SHARC_B6,		SHARC_B7,
	SHARC_B8,		SHARC_B9,		SHARC_B10,		SHARC_B11,
	SHARC_B12,		SHARC_B13,		SHARC_B14,		SHARC_B15,
};

typedef struct {
	UINT32 i[8];
	UINT32 m[8];
	UINT32 b[8];
	UINT32 l[8];
} SHARC_DAG;

typedef union {
	UINT32 r;
	float f;
} SHARC_REG;

typedef struct {
	UINT32 pc;
	UINT32 npc;
	SHARC_REG r[16];
	SHARC_REG reg_alt[16];
	UINT64 mrf;
	UINT64 mrb;

	UINT32 pcstack[32];
	UINT32 lcstack[6];
	UINT32 lastack[6];
	UINT32 lstkp;

	UINT32 faddr;
	UINT32 daddr;
	UINT32 pcstk;
	UINT32 pcstkp;
	UINT32 laddr;
	UINT32 curlcntr;
	UINT32 lcntr;

	/* Data Address Generator (DAG) */
	SHARC_DAG dag1;		// (DM bus)
	SHARC_DAG dag2;		// (PM bus)
	SHARC_DAG dag1_alt;
	SHARC_DAG dag2_alt;

	/* System registers */
	UINT32 mode1;
	UINT32 mode2;
	UINT32 astat;
	UINT32 stky;
	UINT32 irptl;
	UINT32 imask;
	UINT32 imaskp;
	UINT32 ustat1;
	UINT32 ustat2;

	UINT32 syscon;
	UINT32 sysstat;

	UINT32 status_stack[8];
	int status_stkp;

	UINT64 px;

	int (*irq_callback)(int irqline);
	UINT64 opcode;
	void (** opcode_table)(void);
	int idle;
	int irq_active;
	int irq_active_num;
} SHARC_REGS;

static SHARC_REGS sharc;
static int sharc_icount;

// TODO: cpu_readop doesn't work in non-debug version !!!
//#define ROPCODE(pc)       ((UINT64)((cpu_readop32(pc) | ((UINT64)cpu_readop16(pc+4) << 32))))
#define ROPCODE(pc)			((UINT64)(program_read_word_32le(pc)) | ((UINT64)program_read_word_32le(pc+2) << 16) | ((UINT64)program_read_word_32le(pc+4) << 32))

#define ADDR48_TO_32(x)	(((x & ~0xffff) << 2) | ((x & 0xffff) * 6))

#define DECODE_AND_EXEC_OPCODE() \
	sharc.opcode = ROPCODE(ADDR48_TO_32(sharc.pc)); \
	sharc.opcode_table[(sharc.opcode >> 39) & 0x1ff]();

void decode_and_exec_opcode(void);

static data32_t *internal_ram_block0, *internal_ram_block1;
static int internal_ram_block_size;

/*****************************************************************************/
/* SHARC DMA Controller */

static UINT32 dma_control6;
static UINT32 dma_ii6;
static UINT32 dma_im6;
static UINT32 dma_c6;
static UINT32 dma_cp6;
static UINT32 dma_gp6;
static UINT32 dma_ei6;
static UINT32 dma_em6;
static UINT32 dma_ec6;

/*****************************************************************************/

static UINT32 pm_read32(UINT32 address)
{
	if (address >= 0x20000 && address <= 0x3ffff)
	{
		address = ((address & ~0xffff) << 2) | ((address & 0xffff) * 6);
	}
	else {
		osd_die("SHARC: PM Bus Read %08X at %08X\n", address, sharc.pc);
	}
	return program_read_dword_32le(address);
}

static void pm_write32(UINT32 address, UINT32 data)
{
	if (address >= 0x20000 && address <= 0x3ffff)
	{
		address = ((address & ~0xffff) << 2) | ((address & 0xffff) * 6);
	}
	else {
		osd_die("SHARC: PM Bus Write %08X, %08X at %08X\n", address, data, sharc.pc);
	}
	program_write_dword_32le(address, data);
}

static UINT64 pm_read48(UINT32 address)
{
	UINT64 r = 0;
	if (address >= 0x20000 && address <= 0x3ffff)
	{
		address = ((address & ~0xffff) << 2) | ((address & 0xffff) * 6);
	}
	else {
		osd_die("SHARC: PM Bus Read48 %08X at %08X\n", address, sharc.pc);
	}
	r |= (UINT64)program_read_word_32le(address+0) << 0;
	r |= (UINT64)program_read_word_32le(address+2) << 16;
	r |= (UINT64)program_read_word_32le(address+4) << 32;
	return r;
}

static void pm_write48(UINT32 address, UINT64 data)
{
	if (address >= 0x20000 && address <= 0x3ffff)
	{
		address = ((address & ~0xffff) << 2) | ((address & 0xffff) * 6);
	}
	else {
		osd_die("SHARC: PM Bus Write48 %08X, %04X%08X at %08X\n", address, (UINT16)(data >> 32),(UINT32)(data), sharc.pc);
	}

	program_write_word_32le(address+0, (data >> 0) & 0xffff);
	program_write_word_32le(address+2, (data >> 16) & 0xffff);
	program_write_word_32le(address+4, (data >> 32) & 0xffff);
}

static UINT32 dm_read32(UINT32 address)
{
	return program_read_dword_32le(address << 2);
}

static void dm_write32(UINT32 address, UINT32 data)
{
	program_write_dword_32le(address << 2, data);
}

static READ32_HANDLER(sharc_shortword_r)
{
	if (offset & 0x1)
	{
		return program_read_dword_32le((0x20000 + (offset >> 1)) << 2) >> 16;
	}
	else
	{
		return program_read_dword_32le((0x20000 + (offset >> 1)) << 2) & 0xffff;
	}
}

static WRITE32_HANDLER(sharc_shortword_w)
{
	if (offset & 0x1)
	{
		UINT32 old_data = program_read_dword_32le((0x20000 + (offset >> 1)) << 2);
		program_write_dword_32le((0x20000 + (offset >> 1)) << 2, (data << 16) | (old_data & 0xffff));
	}
	else
	{
		UINT32 old_data = program_read_dword_32le((0x20000 + (offset >> 1)) << 2);
		program_write_dword_32le((0x20000 + (offset >> 1)) << 2, (data & 0xffff) | (old_data & 0xffff0000));
	}
}

/* IOP registers */
static READ32_HANDLER(sharc_iop_r)
{
	printf("SHARC: sharc_iop_r %08X, %08X\n", offset, mem_mask);
	return 0;
}

static WRITE32_HANDLER(sharc_iop_w)
{
	switch(offset)
	{
		case 0x1c:
			dma_control6 = data;
			if (data & 0x1) {
				sharc_dma_exec(6);
			}
			return;
		case 0x40:		dma_ii6 = data; return;
		case 0x41:		dma_im6 = data; return;
		case 0x42:		dma_c6 = data; return;
		case 0x43:		dma_cp6 = data; return;
		case 0x44:		dma_gp6 = data; return;
		case 0x45:		dma_ei6 = data; return;
		case 0x46:		dma_em6 = data; return;
		case 0x47:		dma_ec6 = data; return;
	}
	printf("SHARC: sharc_iop_w %08X, %08X, %08X\n", data, offset, mem_mask);
}

/*****************************************************************************/

static void sharc_dma_exec(int channel)
{
	int i;
	UINT32 src, dst;
	int chen, tran, dtype, pmode, mswf, master, ishake, intio, ext, flsh;
	if (channel != 6) {
		osd_die("SHARC: DMA channels other than 6 not supported yet (%d)\n", channel);
	}

	chen = (dma_control6 >> 1) & 0x1;
	tran = (dma_control6 >> 2) & 0x1;
	dtype = (dma_control6 >> 5) & 0x1;
	pmode = (dma_control6 >> 6) & 0x3;
	mswf = (dma_control6 >> 8) & 0x1;
	master = (dma_control6 >> 9) & 0x1;
	ishake = (dma_control6 >> 10) & 0x1;
	intio = (dma_control6 >> 11) & 0x1;
	ext = (dma_control6 >> 12) & 0x1;
	flsh = (dma_control6 >> 13) & 0x1;

	printf("IMask = %08X\n", sharc.imask);
	printf("DMA Channel 6 op\n");
	printf("   Chaining %s\n", chen ? "enabled" : "disabled");
	printf("   %s ext. memory\n", tran ? "Write to" : "Read from");
	printf("   Data type %s\n", dtype ? "data" : "instructions");
	printf("   Packing mode %d\n", pmode);
	printf("   MSWF %d\n", mswf);
	printf("   Extern %d\n", ext);
	printf("\n");
	printf("   Internal Index: %08X\n", dma_ii6);
	printf("   Internal Modifier: %08X\n", dma_im6);
	printf("   Internal Count: %08X\n", dma_c6);
	printf("   Chain Pointer: %08X\n", dma_cp6);
	printf("   General Purpose: %08X\n", dma_gp6);
	printf("   External Index: %08X\n", dma_ei6);
	printf("   External Modifier: %08X\n", dma_em6);
	printf("   External Count: %08X\n", dma_ec6);

	src = dma_ei6;
	dst = dma_ii6;
	for (i=0; i < dma_ec6; i+=6)
	{
		UINT64 data = (UINT64)dm_read32(src+0) << 0 | (UINT64)dm_read32(src+1) << 8 | (UINT64)dm_read32(src+2) << 16 |
					  (UINT64)dm_read32(src+3) << 24 | (UINT64)dm_read32(src+4) << 32 | (UINT64)dm_read32(src+5) << 40;
		pm_write48(0x20000 + (dst & 0x1ffff), data);
		src += dma_em6*6;
		dst += dma_im6;
	}

	/* DMA interrupt */
	if (sharc.imask & (1 << (channel+10)))
	{
		sharc.irq_active = 1;
		sharc.irq_active_num = channel+10;
	}
}

/*****************************************************************************/

void sharc_set_flag_input(int flag_num, int state)
{
	if (flag_num >= 0 && flag_num < 4)
	{
		// Check if flag is set to input in MODE2 (bit == 0)
		if ((sharc.mode2 & (1 << (flag_num+16))) == 0)
		{
			sharc.astat &= ~(1 << (flag_num+19));
			sharc.astat |= (state != 0) ? (1 << (flag_num+19)) : 0;
		}
		else
		{
			osd_die("sharc_set_flag_input: flag %d is set output!", flag_num);
		}
	}
}

static offs_t sharc_dasm(char *buffer, offs_t pc)
{
	int npc = ADDR48_TO_32(pc);
	UINT64 op = ROPCODE(npc);
#ifdef MAME_DEBUG
	sharc_dasm_one(buffer, pc, op);
#else
	sprintf(buffer, "$%04X%08X", (UINT32)((op >> 32) & 0xffff), (UINT32)op);
#endif
	return 1;
}

#include "sharcops.c"
#include "sharcops.h"

static void sharc_init(void)
{
	sharc.opcode_table = sharc_op;
	internal_ram_block_size = 0x20000;
}

static void sharc_reset(void *param)
{
	sharc_config *config = param;
	sharc.pc = 0x20004;
	sharc.npc = sharc.pc + 1;
	sharc.idle = 0;

	switch(config->boot_mode)
	{
		case BOOT_MODE_EPROM:
			dma_ii6 = 0x20000;
			dma_im6 = 1;
			dma_c6 = 0x100;
			dma_ei6 = 0x400000;
			dma_em6 = 1;
			dma_ec6 = 0x600;
			sharc_dma_exec(6);
			break;

		case BOOT_MODE_HOST:
			break;

		default:
			osd_die("SHARC: Unimplemented boot mode %d\n", config->boot_mode);
	}
}

static void sharc_exit(void)
{
	/* TODO */
}

static void sharc_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(SHARC_REGS *)dst = sharc;
}

static void sharc_set_context(void *src)
{
	/* copy the context */
	if (src)
		sharc = *(SHARC_REGS *)src;

	change_pc(sharc.pc);
}

static void sharc_set_irq_line(int irqline, int state)
{
	/* TODO */
	if (state)
	{
		if (sharc.imask & (1 << (8-irqline)))
		{
			sharc.irq_active = 1;
			sharc.irq_active_num = 8-irqline;
		}
	}
}

static void check_interrupts(void)
{
	if (sharc.imask & (1 << sharc.irq_active_num) && sharc.mode1 & 0x1000)
	{
		PUSH_PC(sharc.npc);

		sharc.irptl |= 1 << sharc.irq_active_num;

		if (sharc.irq_active_num >= 6 && sharc.irq_active_num <= 8)
		{
			PUSH_STATUS_REG(sharc.astat);
			PUSH_STATUS_REG(sharc.mode1);
		}

		sharc.npc = 0x20000 + (sharc.irq_active_num * 0x4);
		/* TODO: alter IMASKP */

		sharc.irq_active = 0;
	}
}

static int sharc_execute(int num_cycles)
{
	sharc_icount = num_cycles;

	if(sharc.idle && sharc.irq_active == 0) {
		sharc_icount = 0;
		CALL_MAME_DEBUG;
	}
	if(sharc.irq_active != 0)
	{
		sharc.idle = 0;
		check_interrupts();
	}

	while(sharc_icount > 0 && !sharc.idle)
	{
		sharc.pc = sharc.npc;
		sharc.npc++;
		CALL_MAME_DEBUG;
		DECODE_AND_EXEC_OPCODE();

		systemreg_latency_op();

		/* handle looping */
		if(sharc.pc == (sharc.laddr & 0xffffff)) {
			int cond = (sharc.laddr >> 24) & 0x1f;

			/* loop type */
			switch((sharc.laddr >> 30) & 0x3)
			{
				case 0:		/* arithmetic condition-based */
					if(DO_CONDITION_CODE(cond)) {
						POP_LOOP();
						POP_PC();
					} else {
						sharc.npc = TOP_PC();
					}
					break;
				case 1:		/* counter-based, length 1 */
				case 2:		/* counter-based, length 2 */
				case 3:		/* counter-based, length >2 */
					sharc.lcstack[sharc.lstkp]--;
					sharc.curlcntr--;
					if(sharc.curlcntr == 0) {
						POP_LOOP();
						POP_PC();
					} else {
						sharc.npc = TOP_PC();
					}
					break;
			}
		}

		sharc_icount--;
	}
	return num_cycles - sharc_icount;
}

/*****************************************************************************/

/* Debugger definitions */

static UINT8 sharc_reg_layout[] =
{
	SHARC_PC,		SHARC_PCSTK,	-1,
	SHARC_IMASK,	SHARC_ASTAT,	-1,
	SHARC_LCNTR,	SHARC_SYSSTAT,	-1,
	SHARC_R0,		SHARC_R8,		-1,
	SHARC_R1,		SHARC_R9,		-1,
	SHARC_R2,		SHARC_R10,		-1,
	SHARC_R3,		SHARC_R11,		-1,
	SHARC_R4,		SHARC_R12,		-1,
	SHARC_R5,		SHARC_R13,		-1,
	SHARC_R6,		SHARC_R14,		-1,
	SHARC_R7,		SHARC_R15,		0
};

static UINT8 sharc_win_layout[] =
{
	 0,16,34,17,	/* register window (top rows) */
	 0, 0,80,15,	/* disassembler window (left colums) */
	35,16,45, 2,	/* memory #2 window (right, lower middle) */
	35,19,45, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/**************************************************************************
 * Internal memory map
 **************************************************************************/

static ADDRESS_MAP_START( adsp21062_internal, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0003ff) AM_READWRITE(sharc_iop_r, sharc_iop_w)
	AM_RANGE(0x020000, 0x02ffff) AM_RAM		/* shut up the debug for now */
	AM_RANGE(0x080000, 0x09ffff) AM_RAM AM_BASE(&internal_ram_block0)
	AM_RANGE(0x0a0000, 0x0bffff) AM_RAM AM_BASE(&internal_ram_block1) AM_SHARE(1)
	AM_RANGE(0x0c0000, 0x0dffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x0e0000, 0x0fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x100000, 0x1fffff) AM_READWRITE(sharc_shortword_r, sharc_shortword_w)
ADDRESS_MAP_END

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void sharc_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + SHARC_PC:			sharc.pc = info->i;						break;

		case CPUINFO_INT_REGISTER + SHARC_R0:			sharc.r[0].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R1:			sharc.r[1].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R2:			sharc.r[2].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R3:			sharc.r[3].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R4:			sharc.r[4].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R5:			sharc.r[5].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R6:			sharc.r[6].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R7:			sharc.r[7].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R8:			sharc.r[8].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R9:			sharc.r[9].r = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_R10:			sharc.r[10].r = info->i;				break;
		case CPUINFO_INT_REGISTER + SHARC_R11:			sharc.r[11].r = info->i;				break;
		case CPUINFO_INT_REGISTER + SHARC_R12:			sharc.r[12].r = info->i;				break;
		case CPUINFO_INT_REGISTER + SHARC_R13:			sharc.r[13].r = info->i;				break;
		case CPUINFO_INT_REGISTER + SHARC_R14:			sharc.r[14].r = info->i;				break;
		case CPUINFO_INT_REGISTER + SHARC_R15:			sharc.r[15].r = info->i;				break;

		case CPUINFO_INT_REGISTER + SHARC_I0:			sharc.dag1.i[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I1:			sharc.dag1.i[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I2:			sharc.dag1.i[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I3:			sharc.dag1.i[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I4:			sharc.dag1.i[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I5:			sharc.dag1.i[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I6:			sharc.dag1.i[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I7:			sharc.dag1.i[7] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I8:			sharc.dag2.i[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I9:			sharc.dag2.i[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I10:			sharc.dag2.i[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I11:			sharc.dag2.i[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I12:			sharc.dag2.i[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I13:			sharc.dag2.i[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I14:			sharc.dag2.i[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + SHARC_I15:			sharc.dag2.i[7] = info->i;					break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:					sharc.irq_callback = info->irqcallback;	break;
	}
}

#if (HAS_ADSP21062)
void adsp21062_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + 5)
	{
		sharc_set_irq_line(state-CPUINFO_INT_INPUT_STATE, info->i);
		return;
	}
	switch(state)
	{
		default:	sharc_set_info(state, info);		break;
	}
}
#endif



void sharc_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(sharc);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 32;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 40;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:			info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + SHARC_PC:			info->i = sharc.pc;						break;
		case CPUINFO_INT_REGISTER + SHARC_PCSTK:		info->i = sharc.pcstk;					break;
		case CPUINFO_INT_REGISTER + SHARC_MODE1:		info->i = sharc.mode1;					break;
		case CPUINFO_INT_REGISTER + SHARC_MODE2:		info->i = sharc.mode2;					break;
		case CPUINFO_INT_REGISTER + SHARC_ASTAT:		info->i = sharc.astat;					break;
		case CPUINFO_INT_REGISTER + SHARC_IRPTL:		info->i = sharc.irptl;					break;
		case CPUINFO_INT_REGISTER + SHARC_IMASK:		info->i = sharc.imask;					break;
		case CPUINFO_INT_REGISTER + SHARC_USTAT1:		info->i = sharc.ustat1;					break;
		case CPUINFO_INT_REGISTER + SHARC_USTAT2:		info->i = sharc.ustat2;					break;

		case CPUINFO_INT_REGISTER + SHARC_R0:			info->i = sharc.r[0].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R1:			info->i = sharc.r[1].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R2:			info->i = sharc.r[2].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R3:			info->i = sharc.r[3].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R4:			info->i = sharc.r[4].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R5:			info->i = sharc.r[5].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R6:			info->i = sharc.r[6].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R7:			info->i = sharc.r[7].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R8:			info->i = sharc.r[8].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R9:			info->i = sharc.r[9].r;					break;
		case CPUINFO_INT_REGISTER + SHARC_R10:			info->i = sharc.r[10].r;				break;
		case CPUINFO_INT_REGISTER + SHARC_R11:			info->i = sharc.r[11].r;				break;
		case CPUINFO_INT_REGISTER + SHARC_R12:			info->i = sharc.r[12].r;				break;
		case CPUINFO_INT_REGISTER + SHARC_R13:			info->i = sharc.r[13].r;				break;
		case CPUINFO_INT_REGISTER + SHARC_R14:			info->i = sharc.r[14].r;				break;
		case CPUINFO_INT_REGISTER + SHARC_R15:			info->i = sharc.r[15].r;				break;

		case CPUINFO_INT_REGISTER + SHARC_I0:			info->i = sharc.dag1.i[0];					break;
		case CPUINFO_INT_REGISTER + SHARC_I1:			info->i = sharc.dag1.i[1];					break;
		case CPUINFO_INT_REGISTER + SHARC_I2:			info->i = sharc.dag1.i[2];					break;
		case CPUINFO_INT_REGISTER + SHARC_I3:			info->i = sharc.dag1.i[3];					break;
		case CPUINFO_INT_REGISTER + SHARC_I4:			info->i = sharc.dag1.i[4];					break;
		case CPUINFO_INT_REGISTER + SHARC_I5:			info->i = sharc.dag1.i[5];					break;
		case CPUINFO_INT_REGISTER + SHARC_I6:			info->i = sharc.dag1.i[6];					break;
		case CPUINFO_INT_REGISTER + SHARC_I7:			info->i = sharc.dag1.i[7];					break;
		case CPUINFO_INT_REGISTER + SHARC_I8:			info->i = sharc.dag2.i[0];					break;
		case CPUINFO_INT_REGISTER + SHARC_I9:			info->i = sharc.dag2.i[1];					break;
		case CPUINFO_INT_REGISTER + SHARC_I10:			info->i = sharc.dag2.i[2];					break;
		case CPUINFO_INT_REGISTER + SHARC_I11:			info->i = sharc.dag2.i[3];					break;
		case CPUINFO_INT_REGISTER + SHARC_I12:			info->i = sharc.dag2.i[4];					break;
		case CPUINFO_INT_REGISTER + SHARC_I13:			info->i = sharc.dag2.i[5];					break;
		case CPUINFO_INT_REGISTER + SHARC_I14:			info->i = sharc.dag2.i[6];					break;
		case CPUINFO_INT_REGISTER + SHARC_I15:			info->i = sharc.dag2.i[7];					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = sharc_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = sharc_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = sharc_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = sharc_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = sharc_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = sharc_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = sharc_dasm;			break;
		case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = sharc.irq_callback;	break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &sharc_icount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = sharc_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = sharc_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "SHARC"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2004 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s = cpuintrf_temp_str(), " "); break;

		case CPUINFO_STR_REGISTER + SHARC_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", sharc.pc); break;
		case CPUINFO_STR_REGISTER + SHARC_PCSTK:		sprintf(info->s = cpuintrf_temp_str(), "PCSTK: %08X", sharc.pcstk); break;
		case CPUINFO_STR_REGISTER + SHARC_MODE1:		sprintf(info->s = cpuintrf_temp_str(), "MODE1: %08X", sharc.mode1); break;
		case CPUINFO_STR_REGISTER + SHARC_MODE2:		sprintf(info->s = cpuintrf_temp_str(), "MODE2: %08X", sharc.mode2); break;
		case CPUINFO_STR_REGISTER + SHARC_ASTAT:		sprintf(info->s = cpuintrf_temp_str(), "ASTAT: %08X", sharc.astat); break;
		case CPUINFO_STR_REGISTER + SHARC_IRPTL:		sprintf(info->s = cpuintrf_temp_str(), "IRPTL: %08X", sharc.irptl); break;
		case CPUINFO_STR_REGISTER + SHARC_IMASK:		sprintf(info->s = cpuintrf_temp_str(), "IMASK: %08X", sharc.imask); break;
		case CPUINFO_STR_REGISTER + SHARC_USTAT1:		sprintf(info->s = cpuintrf_temp_str(), "USTAT1: %08X", sharc.ustat1); break;
		case CPUINFO_STR_REGISTER + SHARC_USTAT2:		sprintf(info->s = cpuintrf_temp_str(), "USTAT2: %08X", sharc.ustat2); break;

		case CPUINFO_STR_REGISTER + SHARC_R0:			sprintf(info->s = cpuintrf_temp_str(), "R0: %08X", (UINT32)sharc.r[0].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R1:			sprintf(info->s = cpuintrf_temp_str(), "R1: %08X", (UINT32)sharc.r[1].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R2:			sprintf(info->s = cpuintrf_temp_str(), "R2: %08X", (UINT32)sharc.r[2].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R3:			sprintf(info->s = cpuintrf_temp_str(), "R3: %08X", (UINT32)sharc.r[3].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R4:			sprintf(info->s = cpuintrf_temp_str(), "R4: %08X", (UINT32)sharc.r[4].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R5:			sprintf(info->s = cpuintrf_temp_str(), "R5: %08X", (UINT32)sharc.r[5].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R6:			sprintf(info->s = cpuintrf_temp_str(), "R6: %08X", (UINT32)sharc.r[6].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R7:			sprintf(info->s = cpuintrf_temp_str(), "R7: %08X", (UINT32)sharc.r[7].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R8:			sprintf(info->s = cpuintrf_temp_str(), "R8: %08X", (UINT32)sharc.r[8].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R9:			sprintf(info->s = cpuintrf_temp_str(), "R9: %08X", (UINT32)sharc.r[9].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R10:			sprintf(info->s = cpuintrf_temp_str(), "R10: %08X", (UINT32)sharc.r[10].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R11:			sprintf(info->s = cpuintrf_temp_str(), "R11: %08X", (UINT32)sharc.r[11].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R12:			sprintf(info->s = cpuintrf_temp_str(), "R12: %08X", (UINT32)sharc.r[12].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R13:			sprintf(info->s = cpuintrf_temp_str(), "R13: %08X", (UINT32)sharc.r[13].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R14:			sprintf(info->s = cpuintrf_temp_str(), "R14: %08X", (UINT32)sharc.r[14].r); break;
		case CPUINFO_STR_REGISTER + SHARC_R15:			sprintf(info->s = cpuintrf_temp_str(), "R15: %08X", (UINT32)sharc.r[15].r); break;

		case CPUINFO_STR_REGISTER + SHARC_I0:			sprintf(info->s = cpuintrf_temp_str(), "I0: %08X", (UINT32)sharc.dag1.i[0]); break;
		case CPUINFO_STR_REGISTER + SHARC_I1:			sprintf(info->s = cpuintrf_temp_str(), "I1: %08X", (UINT32)sharc.dag1.i[1]); break;
		case CPUINFO_STR_REGISTER + SHARC_I2:			sprintf(info->s = cpuintrf_temp_str(), "I2: %08X", (UINT32)sharc.dag1.i[2]); break;
		case CPUINFO_STR_REGISTER + SHARC_I3:			sprintf(info->s = cpuintrf_temp_str(), "I3: %08X", (UINT32)sharc.dag1.i[3]); break;
		case CPUINFO_STR_REGISTER + SHARC_I4:			sprintf(info->s = cpuintrf_temp_str(), "I4: %08X", (UINT32)sharc.dag1.i[4]); break;
		case CPUINFO_STR_REGISTER + SHARC_I5:			sprintf(info->s = cpuintrf_temp_str(), "I5: %08X", (UINT32)sharc.dag1.i[5]); break;
		case CPUINFO_STR_REGISTER + SHARC_I6:			sprintf(info->s = cpuintrf_temp_str(), "I6: %08X", (UINT32)sharc.dag1.i[6]); break;
		case CPUINFO_STR_REGISTER + SHARC_I7:			sprintf(info->s = cpuintrf_temp_str(), "I7: %08X", (UINT32)sharc.dag1.i[7]); break;
		case CPUINFO_STR_REGISTER + SHARC_I8:			sprintf(info->s = cpuintrf_temp_str(), "I8: %08X", (UINT32)sharc.dag2.i[0]); break;
		case CPUINFO_STR_REGISTER + SHARC_I9:			sprintf(info->s = cpuintrf_temp_str(), "I9: %08X", (UINT32)sharc.dag2.i[1]); break;
		case CPUINFO_STR_REGISTER + SHARC_I10:			sprintf(info->s = cpuintrf_temp_str(), "I10: %08X", (UINT32)sharc.dag2.i[2]); break;
		case CPUINFO_STR_REGISTER + SHARC_I11:			sprintf(info->s = cpuintrf_temp_str(), "I11: %08X", (UINT32)sharc.dag2.i[3]); break;
		case CPUINFO_STR_REGISTER + SHARC_I12:			sprintf(info->s = cpuintrf_temp_str(), "I12: %08X", (UINT32)sharc.dag2.i[4]); break;
		case CPUINFO_STR_REGISTER + SHARC_I13:			sprintf(info->s = cpuintrf_temp_str(), "I13: %08X", (UINT32)sharc.dag2.i[5]); break;
		case CPUINFO_STR_REGISTER + SHARC_I14:			sprintf(info->s = cpuintrf_temp_str(), "I14: %08X", (UINT32)sharc.dag2.i[6]); break;
		case CPUINFO_STR_REGISTER + SHARC_I15:			sprintf(info->s = cpuintrf_temp_str(), "I15: %08X", (UINT32)sharc.dag2.i[7]); break;
	}
}

#if (HAS_ADSP21062)
void adsp21062_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = adsp21062_set_info;		break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = construct_map_adsp21062_internal; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "ADSP21062"); break;

		default:	sharc_get_info(state, info);		break;
	}
}
#endif
