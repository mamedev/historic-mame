/* IBM/Motorola PowerPC 4xx/6xx Emulator */

#include "driver.h"
#include "ppc.h"
#include "mamedbg.h"

#define RT				((op >> 21) & 0x1f)
#define RS				((op >> 21) & 0x1f)
#define RA				((op >> 16) & 0x1f)
#define RB				((op >> 11) & 0x1f)
#define RC				((op >> 6) & 0x1f)
#define MB				((op >> 6) & 0x1f)
#define ME				((op >> 1) & 0x1f)
#define SH				((op >> 11) & 0x1f)
#define BO				((op >> 21) & 0x1f)
#define BI				((op >> 16) & 0x1f)
#define CRFD			((op >> 23) & 0x7)
#define CRFA			((op >> 18) & 0x7)
#define FXM				((op >> 12) & 0xff)
#define SPR				(((op >> 16) & 0x1f) | ((op >> 6) & 0x3e0))

#define SIMM16			(INT32)(INT16)(op & 0xffff)
#define UIMM16			(UINT32)(op & 0xffff)

#define RCBIT			(op & 0x1)
#define OEBIT			(op & 0x400)
#define AABIT			(op & 0x2)
#define LKBIT			(op & 0x1)

#define REG(x)			(ppc.r[x])
#define LR				(ppc.lr)
#define CTR				(ppc.ctr)
#define XER				(ppc.xer)
#define CR(x)			(ppc.cr[x])
#define MSR				(ppc.msr)
#define SRR0			(ppc.srr0)
#define SRR1			(ppc.srr1)
#define EVPR			(ppc.evpr)
#define EXIER			(ppc.exier)
#define EXISR			(ppc.exisr)

#define BITMASK_0(n)	(UINT32)(((UINT64)1 << n) - 1)
#define CRBIT(x)		((ppc.cr[x / 4] & (1 << (3 - (x % 4)))) ? 1 : 0)
#define _BIT(n)			(1 << (n))
#define GET_ROTATE_MASK(mb,me)		(ppc_rotate_mask[mb][me])
#define ADD_CA(r,a,b)		((UINT32)r < (UINT32)a)
#define SUB_CA(r,a,b)		(!((UINT32)a < (UINT32)b))
#define ADD_OV(r,a,b)		((~((a) ^ (b)) & ((a) ^ (r))) & 0x80000000)
#define SUB_OV(r,a,b)		(( ((a) ^ (b)) & ((a) ^ (r))) & 0x80000000)

#define XER_SO			0x80000000
#define XER_OV			0x40000000
#define XER_CA			0x20000000

#define MSR_WE			0x00040000
#define MSR_CE			0x00020000
#define MSR_ILE			0x00010000
#define MSR_EE			0x00008000
#define MSR_PR			0x00004000
#define MSR_ME			0x00001000
#define MSR_DE			0x00000200
#define MSR_PE			0x00000008
#define MSR_PX			0x00000004
#define MSR_LE			0x00000001

#define BYTE_REVERSE16(x)	(((x >> 8) & 0xff) | ((x << 8) & 0xff00))
#define BYTE_REVERSE32(x)	(((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | ((x << 24) & 0xff000000))

typedef struct {
	UINT32 cr;
	UINT32 da;
	UINT32 sa;
	UINT32 ct;
	UINT32 cc;
} DMA_REGS;

typedef struct {
	UINT8 spls;
	UINT8 sphs;
	UINT16 brd;
	UINT8 spctl;
	UINT8 sprc;
	UINT8 sptc;
	UINT8 sprb;
	UINT8 sptb;
} SPU_REGS;



typedef struct {
	UINT32 r[32];
	UINT32 pc;
	UINT32 npc;

	UINT32 lr;
	UINT32 ctr;
	UINT32 xer;
	UINT32 msr;
	UINT8 cr[8];
	UINT32 pvr;
	UINT32 srr0;
	UINT32 srr1;

	UINT32 spr[1024];

	UINT32 evpr;
	UINT32 exier;
	UINT32 exisr;
	UINT32 bear;
	UINT32 besr;
	UINT32 iocr;
	UINT32 br[8];

	SPU_REGS spu;
	DMA_REGS dma[4];
	UINT32 dmasr;

	int reserved;
	UINT32 reserved_address;

	UINT64 tb;			/* 56-bit timebase register */

	int (*irq_callback)(int irqline);
} PPC_REGS;



typedef struct {
	int code;
	int subcode;
	void (* handler)(UINT32);
} PPC_OPCODE;



static int ppc_icount;
static PPC_REGS ppc;
static UINT32 ppc_rotate_mask[32][32];

#define ROPCODE(pc)		cpu_readop32(pc)

INLINE void SET_CR0(INT32 rd)
{
	if( rd < 0 ) {
		CR(0) = 0x8;
	} else if( rd > 0 ) {
		CR(0) = 0x4;
	} else {
		CR(0) = 0x2;
	}

	if( XER & XER_SO )
		CR(0) |= 0x1;
}

INLINE void SET_ADD_OV(UINT32 rd, UINT32 ra, UINT32 rb)
{
	if( ADD_OV(rd, ra, rb) )
		XER |= XER_SO | XER_OV;
	else
		XER &= ~XER_OV;
}

INLINE void SET_SUB_OV(UINT32 rd, UINT32 ra, UINT32 rb)
{
	if( SUB_OV(rd, ra, rb) )
		XER |= XER_SO | XER_OV;
	else
		XER &= ~XER_OV;
}

INLINE void SET_ADD_CA(UINT32 rd, UINT32 ra, UINT32 rb)
{
	if( ADD_CA(rd, ra, rb) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;
}

INLINE void SET_SUB_CA(UINT32 rd, UINT32 ra, UINT32 rb)
{
	if( SUB_CA(rd, ra, rb) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;
}

INLINE UINT32 check_condition_code(UINT32 bo, UINT32 bi)
{
	UINT32 ctr_ok;
	UINT32 condition_ok;
	UINT32 bo0 = (bo & 0x10) ? 1 : 0;
	UINT32 bo1 = (bo & 0x08) ? 1 : 0;
	UINT32 bo2 = (bo & 0x04) ? 1 : 0;
	UINT32 bo3 = (bo & 0x02) ? 1 : 0;

	if( bo2 == 0 )
		--CTR;

	ctr_ok = bo2 | ((CTR != 0) ^ bo3);
	condition_ok = bo0 | (CRBIT(bi) ^ (~bo1 & 0x1));

	return ctr_ok && condition_ok;
}

/*********************************************************************/

INLINE void ppc_set_spr(int spr, UINT32 value)
{
	switch(spr)
	{
		case SPR_LR:		LR = value; break;
		case SPR_CTR:		CTR = value; break;
		case SPR_XER:		XER = value; break;
		case SPR_EVPR:		EVPR = value & 0xffff0000; break;
		case SPR_SRR0:		ppc.srr0 = value; break;
		case SPR_SRR1:		ppc.srr1 = value; break;
		case SPR_ESR:		ppc.spr[SPR_ESR] = value; break;
		case SPR_TCR:		ppc.spr[SPR_TCR] = value; break;
		case SPR_ICCR:		ppc.spr[SPR_ICCR] = value; break;
		case SPR_DCCR:		ppc.spr[SPR_DCCR] = value; break;
		case SPR_TBHI:		ppc.tb &= 0xffffffff; ppc.tb |= (UINT64)value << 32; break;
		case SPR_TBLO:		ppc.tb &= U64(0xffffffff00000000); ppc.tb |= value; break;
		case SPR_PIT:		ppc.spr[SPR_PIT] = value; break;
		case SPR_SPRG0:		ppc.spr[SPR_SPRG0] = value; break;
		case SPR_SPRG1:		ppc.spr[SPR_SPRG1] = value; break;
		case SPR_SPRG2:		ppc.spr[SPR_SPRG2] = value; break;
		case SPR_SPRG3:		ppc.spr[SPR_SPRG3] = value; break;

		default:
			osd_die("ppc: set_spr: Unimplemented SPR %X\n", spr);
			break;
	}
}

INLINE UINT32 ppc_get_spr(int spr)
{
	switch(spr)
	{
		case SPR_LR:		return LR;
		case SPR_CTR:		return CTR;
		case SPR_XER:		return XER;
		case SPR_EVPR:		return EVPR;
		case SPR_SRR0:		return ppc.srr0;
		case SPR_SRR1:		return ppc.srr1;
		case SPR_ESR:		return ppc.spr[SPR_ESR];
		case SPR_TCR:		return ppc.spr[SPR_TCR];
		case SPR_ICCR:		return ppc.spr[SPR_ICCR];
		case SPR_DCCR:		return ppc.spr[SPR_DCCR];
		case SPR_TBHI:		return ((ppc.tb >> 32) & 0xffffff);
		case SPR_TBLO:		return (ppc.tb & 0xffffffff);
		case SPR_PIT:		return ppc.spr[SPR_PIT];
		case SPR_SPRG0:		return ppc.spr[SPR_SPRG0];
		case SPR_SPRG1:		return ppc.spr[SPR_SPRG1];
		case SPR_SPRG2:		return ppc.spr[SPR_SPRG2];
		case SPR_SPRG3:		return ppc.spr[SPR_SPRG3];
		case SPR_PVR:		return ppc.pvr;

		default:
			osd_die("ppc: get_spr: Unimplemented SPR %X\n", spr);
			break;
	}
}

INLINE void ppc_set_msr(UINT32 value)
{
	/* TODO */
	if( value & 0x8000 )
		logerror("ppc: External IRQ enable\n");

	if( value & (MSR_ILE | MSR_LE) )
		osd_die("ppc: set_msr: little_endian mode not supported !\n");
	MSR = value;
}

INLINE UINT32 ppc_get_msr(void)
{
	return MSR;
}

INLINE void ppc_set_cr(UINT32 value)
{
	CR(0) = (value >> 28) & 0xf;
	CR(1) = (value >> 24) & 0xf;
	CR(2) = (value >> 20) & 0xf;
	CR(3) = (value >> 16) & 0xf;
	CR(4) = (value >> 12) & 0xf;
	CR(5) = (value >> 8) & 0xf;
	CR(6) = (value >> 4) & 0xf;
	CR(7) = (value >> 0) & 0xf;
}

INLINE UINT32 ppc_get_cr(void)
{
	return CR(0) << 28 | CR(1) << 24 | CR(2) << 20 | CR(3) << 16 | CR(4) << 12 | CR(5) << 8 | CR(6) << 4 | CR(7);
}

/***********************************************************************/

static void (* optable19[1024])(UINT32);
static void (* optable31[1024])(UINT32);
static void (* optable59[1024])(UINT32);
static void (* optable63[1024])(UINT32);
static void (* optable[64])(UINT32);

#if (HAS_PPC403)
#include "ppc403.c"
#endif


INLINE UINT8 READ8(UINT32 a)
{
	if( a >= 0x40000000 && a <= 0x4000000f ) {		/* Serial Port */
		return ppc403_spu_r(a);
	} else {
		return program_read_byte_32be(a);
	}
}

INLINE UINT16 READ16(UINT32 a)
{
	if( a & 0x1 ) {
		osd_die("ppc: Unaligned read16 %08X\n", a);
	} else {
		return program_read_word_32be(a);
	}
}

INLINE UINT32 READ32(UINT32 a)
{
	if( a & 0x3 ) {
		osd_die("ppc: Unaligned read32 %08X\n", a);
	} else {
		return program_read_dword_32be(a);
	}
}

INLINE void WRITE8(UINT32 a, UINT8 d)
{
	if( a >= 0x40000000 && a <= 0x4000000f ) {		/* Serial Port */
		ppc403_spu_w(a, d);
	} else {
		program_write_byte_32be(a, d);
	}
}

INLINE void WRITE16(UINT32 a, UINT16 d)
{
	if( a & 0x1 ) {
		osd_die("ppc: Unaligned write16 %08X, %04X\n", a, d);
	} else {
		program_write_word_32be(a, d);
	}
}

INLINE void WRITE32(UINT32 a, UINT32 d)
{
	if( a & 0x3 ) {
		osd_die("ppc: Unaligned write32 %08X, %08X\n", a, d);
	} else {
		if( ppc.reserved ) {
			if( a == ppc.reserved_address ) {
				ppc.reserved = 0;
			}
		}
		program_write_dword_32be(a, d);
	}
}

/********************************************************************/

#include "ppc_ops.c"
#include "ppc_ops.h"

/* Initialization and shutdown */

void ppc_init(void)
{
	int i,j;

	for( i=0; i < 64; i++ ) {
		optable[i] = ppc_invalid;
	}
	for( i=0; i < 1024; i++ ) {
		optable19[i] = ppc_invalid;
		optable31[i] = ppc_invalid;
		optable59[i] = ppc_invalid;
		optable63[i] = ppc_invalid;
	}

	/* Fill the opcode tables */
	for( i=0; i < (sizeof(ppc_opcode_common) / sizeof(PPC_OPCODE)); i++ ) {

		switch(ppc_opcode_common[i].code)
		{
			case 19:	
				optable19[ppc_opcode_common[i].subcode] = ppc_opcode_common[i].handler;
				break;

			case 31:
				optable31[ppc_opcode_common[i].subcode] = ppc_opcode_common[i].handler;
				break;

			case 59:
			case 63:
				break;

			default:
				optable[ppc_opcode_common[i].code] = ppc_opcode_common[i].handler;
		}

	}

	/* Calculate rotate mask table */
	for( i=0; i < 32; i++ ) {
		for( j=0; j < 32; j++ ) {
			UINT32 mask;
			int mb = i;
			int me = j;
			mask = ((UINT32)0xFFFFFFFF >> mb) ^ ((me >= 31) ? 0 : ((UINT32)0xFFFFFFFF >> (me + 1)));
			if( mb > me ) 
				mask = ~mask;

			ppc_rotate_mask[i][j] = mask;
		}
	}
}

static void ppc403_init(void)
{
	ppc_init();

	/* PPC403 specific opcodes */
	optable31[454] = ppc_dccci;
	optable31[486] = ppc_dcread;
	optable31[262] = ppc_icbt;
	optable31[966] = ppc_iccci;
	optable31[998] = ppc_icread;
	optable31[323] = ppc_mfdcr;
	optable31[451] = ppc_mtdcr;
	optable19[51] = ppc_rfci;
	optable31[131] = ppc_wrtee;
	optable31[163] = ppc_wrteei;
}

static void ppc403_exit(void)
{

}

static void ppc_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(PPC_REGS *)dst = ppc;
}


static void ppc_set_context(void *src)
{
	/* copy the context */
	if (src)
		ppc = *(PPC_REGS *)src;

	change_pc(ppc.pc);
}

/*******************************************************************/

/* Debugger definitions */

static UINT8 ppc_reg_layout[] =
{
	PPC_PC,			PPC_MSR,		-1,
	PPC_CR,			PPC_LR,			-1,
	PPC_CTR,		PPC_XER,		-1,
	PPC_R0,		 	PPC_R16,		-1,
	PPC_R1, 		PPC_R17,		-1,
	PPC_R2, 		PPC_R18,		-1,
	PPC_R3, 		PPC_R19,		-1,
	PPC_R4, 		PPC_R20,		-1,
	PPC_R5, 		PPC_R21,		-1,
	PPC_R6, 		PPC_R22,		-1,
	PPC_R7, 		PPC_R23,		-1,
	PPC_R8,			PPC_R24,		-1,
	PPC_R9,			PPC_R25,		-1,
	PPC_R10,		PPC_R26,		-1,
	PPC_R11,		PPC_R27,		-1,
	PPC_R12,		PPC_R28,		-1,
	PPC_R13,		PPC_R29,		-1,
	PPC_R14,		PPC_R30,		-1,
	PPC_R15,		PPC_R31,		0
};

static UINT8 ppc_win_layout[] =
{
	 0, 0,34,33,	/* register window (top rows) */
	35, 0,45,14,	/* disassembler window (left colums) */
	35,15,45, 3,	/* memory #2 window (right, lower middle) */
	35,19,45, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

static offs_t ppc_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	return ppc_dasm_one(buffer, pc, ROPCODE(pc));
#else
	sprintf(buffer, "$%08X", ROPCODE(pc));
	return 4;
#endif
}


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void ppc_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + PPC_PC:				ppc.pc = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_MSR:			ppc_set_msr(info->i);				 	break;
		case CPUINFO_INT_REGISTER + PPC_CR:				ppc_set_cr(info->i);			 		break;
		case CPUINFO_INT_REGISTER + PPC_LR:				LR = info->i;							break;
		case CPUINFO_INT_REGISTER + PPC_CTR:			CTR = info->i;							break;
		case CPUINFO_INT_REGISTER + PPC_XER:			XER = info->i;						 	break;

		case CPUINFO_INT_REGISTER + PPC_R0:				ppc.r[0] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R1:				ppc.r[1] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R2:				ppc.r[2] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R3:				ppc.r[3] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R4:				ppc.r[4] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R5:				ppc.r[5] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R6:				ppc.r[6] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R7:				ppc.r[7] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R8:				ppc.r[8] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R9:				ppc.r[9] = info->i;						break;
		case CPUINFO_INT_REGISTER + PPC_R10:			ppc.r[10] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R11:			ppc.r[11] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R12:			ppc.r[12] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R13:			ppc.r[13] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R14:			ppc.r[14] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R15:			ppc.r[15] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R16:			ppc.r[16] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R17:			ppc.r[17] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R18:			ppc.r[18] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R19:			ppc.r[19] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R20:			ppc.r[20] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R21:			ppc.r[21] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R22:			ppc.r[22] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R23:			ppc.r[23] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R24:			ppc.r[24] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R25:			ppc.r[25] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R26:			ppc.r[26] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R27:			ppc.r[27] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R28:			ppc.r[28] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R29:			ppc.r[29] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R30:			ppc.r[30] = info->i;					break;
		case CPUINFO_INT_REGISTER + PPC_R31:			ppc.r[31] = info->i;					break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:					ppc.irq_callback = info->irqcallback;	break;
	}
}

#if (HAS_PPC403)
void ppc403_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + 5)
	{
		ppc403_set_irq_line(state-CPUINFO_INT_INPUT_STATE, info->i);
		return;
	}
	switch(state)
	{
		default:	ppc_set_info(state, info);		break;
	}
}
#endif


void ppc_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(ppc);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
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

		case CPUINFO_INT_PC:	/* intentional fallthrough */
		case CPUINFO_INT_REGISTER + PPC_PC:				info->i = ppc.pc;						break;
		case CPUINFO_INT_REGISTER + PPC_MSR:			info->i = ppc_get_msr();				break;
		case CPUINFO_INT_REGISTER + PPC_CR:				info->i = ppc_get_cr();					break;
		case CPUINFO_INT_REGISTER + PPC_LR:				info->i = LR;							break;
		case CPUINFO_INT_REGISTER + PPC_CTR:			info->i = CTR;							break;
		case CPUINFO_INT_REGISTER + PPC_XER:			info->i = XER;							break;

		case CPUINFO_INT_REGISTER + PPC_R0:				info->i = ppc.r[0];						break;
		case CPUINFO_INT_REGISTER + PPC_R1:				info->i = ppc.r[1];						break;
		case CPUINFO_INT_REGISTER + PPC_R2:				info->i = ppc.r[2];						break;
		case CPUINFO_INT_REGISTER + PPC_R3:				info->i = ppc.r[3];						break;
		case CPUINFO_INT_REGISTER + PPC_R4:				info->i = ppc.r[4];						break;
		case CPUINFO_INT_REGISTER + PPC_R5:				info->i = ppc.r[5];						break;
		case CPUINFO_INT_REGISTER + PPC_R6:				info->i = ppc.r[6];						break;
		case CPUINFO_INT_REGISTER + PPC_R7:				info->i = ppc.r[7];						break;
		case CPUINFO_INT_REGISTER + PPC_R8:				info->i = ppc.r[8];						break;
		case CPUINFO_INT_REGISTER + PPC_R9:				info->i = ppc.r[9];						break;
		case CPUINFO_INT_REGISTER + PPC_R10:			info->i = ppc.r[10];					break;
		case CPUINFO_INT_REGISTER + PPC_R11:			info->i = ppc.r[11];					break;
		case CPUINFO_INT_REGISTER + PPC_R12:			info->i = ppc.r[12];					break;
		case CPUINFO_INT_REGISTER + PPC_R13:			info->i = ppc.r[13];					break;
		case CPUINFO_INT_REGISTER + PPC_R14:			info->i = ppc.r[14];					break;
		case CPUINFO_INT_REGISTER + PPC_R15:			info->i = ppc.r[15];					break;
		case CPUINFO_INT_REGISTER + PPC_R16:			info->i = ppc.r[16];					break;
		case CPUINFO_INT_REGISTER + PPC_R17:			info->i = ppc.r[17];					break;
		case CPUINFO_INT_REGISTER + PPC_R18:			info->i = ppc.r[18];					break;
		case CPUINFO_INT_REGISTER + PPC_R19:			info->i = ppc.r[19];					break;
		case CPUINFO_INT_REGISTER + PPC_R20:			info->i = ppc.r[20];					break;
		case CPUINFO_INT_REGISTER + PPC_R21:			info->i = ppc.r[21];					break;
		case CPUINFO_INT_REGISTER + PPC_R22:			info->i = ppc.r[22];					break;
		case CPUINFO_INT_REGISTER + PPC_R23:			info->i = ppc.r[23];					break;
		case CPUINFO_INT_REGISTER + PPC_R24:			info->i = ppc.r[24];					break;
		case CPUINFO_INT_REGISTER + PPC_R25:			info->i = ppc.r[25];					break;
		case CPUINFO_INT_REGISTER + PPC_R26:			info->i = ppc.r[26];					break;
		case CPUINFO_INT_REGISTER + PPC_R27:			info->i = ppc.r[27];					break;
		case CPUINFO_INT_REGISTER + PPC_R28:			info->i = ppc.r[28];					break;
		case CPUINFO_INT_REGISTER + PPC_R29:			info->i = ppc.r[29];					break;
		case CPUINFO_INT_REGISTER + PPC_R30:			info->i = ppc.r[30];					break;
		case CPUINFO_INT_REGISTER + PPC_R31:			info->i = ppc.r[31];					break;



		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = ppc403_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = ppc_get_context;		break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = ppc_set_context;		break;
		case CPUINFO_PTR_INIT:							info->init = ppc403_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = ppc403_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = ppc403_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = ppc403_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = ppc_dasm;			break;
		case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = ppc.irq_callback;	break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &ppc_icount;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = ppc_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = ppc_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "PPC403"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "PowerPC"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2004"); break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s = cpuintrf_temp_str(), " "); break;

		case CPUINFO_STR_REGISTER + PPC_PC:				sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", ppc.pc); break;
		case CPUINFO_STR_REGISTER + PPC_MSR:			sprintf(info->s = cpuintrf_temp_str(), "MSR: %08X", ppc_get_msr()); break;
		case CPUINFO_STR_REGISTER + PPC_CR:				sprintf(info->s = cpuintrf_temp_str(), "CR: %08X", ppc_get_cr()); break;
		case CPUINFO_STR_REGISTER + PPC_LR:				sprintf(info->s = cpuintrf_temp_str(), "LR: %08X", LR); break;
		case CPUINFO_STR_REGISTER + PPC_CTR:			sprintf(info->s = cpuintrf_temp_str(), "CTR: %08X", CTR); break;
		case CPUINFO_STR_REGISTER + PPC_XER:			sprintf(info->s = cpuintrf_temp_str(), "XER: %08X", XER); break;

		case CPUINFO_STR_REGISTER + PPC_R0:				sprintf(info->s = cpuintrf_temp_str(), "R0: %08X", ppc.r[0]); break;
		case CPUINFO_STR_REGISTER + PPC_R1:				sprintf(info->s = cpuintrf_temp_str(), "R1: %08X", ppc.r[1]); break;
		case CPUINFO_STR_REGISTER + PPC_R2:				sprintf(info->s = cpuintrf_temp_str(), "R2: %08X", ppc.r[2]); break;
		case CPUINFO_STR_REGISTER + PPC_R3:				sprintf(info->s = cpuintrf_temp_str(), "R3: %08X", ppc.r[3]); break;
		case CPUINFO_STR_REGISTER + PPC_R4:				sprintf(info->s = cpuintrf_temp_str(), "R4: %08X", ppc.r[4]); break;
		case CPUINFO_STR_REGISTER + PPC_R5:				sprintf(info->s = cpuintrf_temp_str(), "R5: %08X", ppc.r[5]); break;
		case CPUINFO_STR_REGISTER + PPC_R6:				sprintf(info->s = cpuintrf_temp_str(), "R6: %08X", ppc.r[6]); break;
		case CPUINFO_STR_REGISTER + PPC_R7:				sprintf(info->s = cpuintrf_temp_str(), "R7: %08X", ppc.r[7]); break;
		case CPUINFO_STR_REGISTER + PPC_R8:				sprintf(info->s = cpuintrf_temp_str(), "R8: %08X", ppc.r[8]); break;
		case CPUINFO_STR_REGISTER + PPC_R9:				sprintf(info->s = cpuintrf_temp_str(), "R9: %08X", ppc.r[9]); break;
		case CPUINFO_STR_REGISTER + PPC_R10:			sprintf(info->s = cpuintrf_temp_str(), "R10: %08X", ppc.r[10]); break;
		case CPUINFO_STR_REGISTER + PPC_R11:			sprintf(info->s = cpuintrf_temp_str(), "R11: %08X", ppc.r[11]); break;
		case CPUINFO_STR_REGISTER + PPC_R12:			sprintf(info->s = cpuintrf_temp_str(), "R12: %08X", ppc.r[12]); break;
		case CPUINFO_STR_REGISTER + PPC_R13:			sprintf(info->s = cpuintrf_temp_str(), "R13: %08X", ppc.r[13]); break;
		case CPUINFO_STR_REGISTER + PPC_R14:			sprintf(info->s = cpuintrf_temp_str(), "R14: %08X", ppc.r[14]); break;
		case CPUINFO_STR_REGISTER + PPC_R15:			sprintf(info->s = cpuintrf_temp_str(), "R15: %08X", ppc.r[15]); break;
		case CPUINFO_STR_REGISTER + PPC_R16:			sprintf(info->s = cpuintrf_temp_str(), "R16: %08X", ppc.r[16]); break;
		case CPUINFO_STR_REGISTER + PPC_R17:			sprintf(info->s = cpuintrf_temp_str(), "R17: %08X", ppc.r[17]); break;
		case CPUINFO_STR_REGISTER + PPC_R18:			sprintf(info->s = cpuintrf_temp_str(), "R18: %08X", ppc.r[18]); break;
		case CPUINFO_STR_REGISTER + PPC_R19:			sprintf(info->s = cpuintrf_temp_str(), "R19: %08X", ppc.r[19]); break;
		case CPUINFO_STR_REGISTER + PPC_R20:			sprintf(info->s = cpuintrf_temp_str(), "R20: %08X", ppc.r[20]); break;
		case CPUINFO_STR_REGISTER + PPC_R21:			sprintf(info->s = cpuintrf_temp_str(), "R21: %08X", ppc.r[21]); break;
		case CPUINFO_STR_REGISTER + PPC_R22:			sprintf(info->s = cpuintrf_temp_str(), "R22: %08X", ppc.r[22]); break;
		case CPUINFO_STR_REGISTER + PPC_R23:			sprintf(info->s = cpuintrf_temp_str(), "R23: %08X", ppc.r[23]); break;
		case CPUINFO_STR_REGISTER + PPC_R24:			sprintf(info->s = cpuintrf_temp_str(), "R24: %08X", ppc.r[24]); break;
		case CPUINFO_STR_REGISTER + PPC_R25:			sprintf(info->s = cpuintrf_temp_str(), "R25: %08X", ppc.r[25]); break;
		case CPUINFO_STR_REGISTER + PPC_R26:			sprintf(info->s = cpuintrf_temp_str(), "R26: %08X", ppc.r[26]); break;
		case CPUINFO_STR_REGISTER + PPC_R27:			sprintf(info->s = cpuintrf_temp_str(), "R27: %08X", ppc.r[27]); break;
		case CPUINFO_STR_REGISTER + PPC_R28:			sprintf(info->s = cpuintrf_temp_str(), "R28: %08X", ppc.r[28]); break;
		case CPUINFO_STR_REGISTER + PPC_R29:			sprintf(info->s = cpuintrf_temp_str(), "R29: %08X", ppc.r[29]); break;
		case CPUINFO_STR_REGISTER + PPC_R30:			sprintf(info->s = cpuintrf_temp_str(), "R30: %08X", ppc.r[30]); break;
		case CPUINFO_STR_REGISTER + PPC_R31:			sprintf(info->s = cpuintrf_temp_str(), "R31: %08X", ppc.r[31]); break;
	}
}

#if (HAS_PPC403)
void ppc403_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 5;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = ppc403_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = ppc403_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = ppc403_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = ppc403_execute;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "PPC403"); break;

		default:	ppc_get_info(state, info);		break;
	}
}
#endif
