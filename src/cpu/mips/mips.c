/*
 * MIPS emulator for the MAME project written by smf
 *
 * Work in progress, there is still much to do.
 *
 */

#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "mips.h"

#define EXC_INT ( 0 )
#define EXC_TLBL ( 2 )
#define EXC_TLBS ( 3 )
#define EXC_ADEL ( 4 )
#define EXC_ADES ( 5 )
#define EXC_SYS ( 8 )
#define EXC_BP ( 9 )
#define EXC_RI ( 10 )
#define EXC_OVF ( 12 )

#define CP0_RANDOM ( 1 )
#define CP0_BADVADDR ( 8 )
#define CP0_SR ( 12 )
#define CP0_CAUSE ( 13 )
#define CP0_EPC ( 14 )

#define SR_IEC ( 1L << 0 )
#define SR_KUC ( 1L << 1 )
#define SR_ISC ( 1L << 16 )
#define SR_SWC ( 1L << 17 )
#define SR_TS  ( 1L << 21 )
#define SR_BEV ( 1L << 22 )
#define SR_RE ( 1L << 25 )

#define CAUSE_BD ( 0x80000000 )

/* there are more registers but the debugger interface is limited to 127 */
static UINT8 mips_reg_layout[] =
{
	MIPS_PC, MIPS_OC, -1,
	MIPS_HI, MIPS_LO, -1,
	-1,
	MIPS_R0, MIPS_R1, -1,
	MIPS_R2, MIPS_R3, -1,
	MIPS_R4, MIPS_R5, -1,
	MIPS_R6, MIPS_R7, -1,
	MIPS_R8, MIPS_R9, -1,
	MIPS_R10, MIPS_R11, -1,
	MIPS_R12, MIPS_R13, -1,
	MIPS_R14, MIPS_R15, -1,
	MIPS_R16, MIPS_R17, -1,
	MIPS_R18, MIPS_R19, -1,	
	MIPS_R20, MIPS_R21, -1,
	MIPS_R22, MIPS_R23, -1,
	MIPS_R24, MIPS_R25, -1,
	MIPS_R26, MIPS_R27, -1,
	MIPS_R28, MIPS_R29, -1,
	MIPS_R30, MIPS_R31, -1,
	-1,
	MIPS_CP0R0, MIPS_CP0R1, -1,
	MIPS_CP0R2, MIPS_CP0R3, -1,
	MIPS_CP0R4, MIPS_CP0R5, -1,
	MIPS_CP0R6, MIPS_CP0R7, -1,
	MIPS_CP0R8, MIPS_CP0R9, -1,
	MIPS_CP0R10, MIPS_CP0R11, -1,
	MIPS_CP0R12, MIPS_CP0R13, -1,
	MIPS_CP0R14, MIPS_CP0R15, -1,
	MIPS_CP0R16, MIPS_CP0R17, -1,
	MIPS_CP0R18, MIPS_CP0R19, -1,
	MIPS_CP0R20, MIPS_CP0R21, -1,
	MIPS_CP0R22, MIPS_CP0R23, -1,
	MIPS_CP0R24, MIPS_CP0R25, -1,
	MIPS_CP0R26, MIPS_CP0R27, -1,
	MIPS_CP0R28, MIPS_CP0R29, -1,
	MIPS_CP0R30, MIPS_CP0R31, 0
};

static UINT8 mips_win_layout[] = {
	45, 0,35,13,	/* register window (top right) */
	 0, 0,44,13,	/* disassembler window (left, upper) */
	 0,14,44, 8,	/* memory #1 window (left, middle) */
	45,14,35, 8,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

typedef struct
{
	UINT32 op;
	UINT32 pc;
	UINT32 nextop;
	UINT32 nextpc;
	UINT32 irq;
	UINT32 hi;
	UINT32 lo;
	UINT32 r[ 32 ];
	UINT32 cp0r[ 32 ];
} mips_cpu_context;

mips_cpu_context mips;
UINT32 context_pc = 0;

int mips_ICount = 0;

static void mips_exception( int exception )
{
	mips.cp0r[ CP0_CAUSE ] = ( mips.cp0r[ CP0_CAUSE ] & 0xffffff83 ) | ( exception << 2 );
	if( mips.pc != mips.nextpc - 4 )
	{
		mips.cp0r[ CP0_EPC ] = mips.pc - 4;
		mips.cp0r[ CP0_CAUSE ] |= CAUSE_BD;
	}
	else
	{
		mips.cp0r[ CP0_EPC ] = mips.pc;
		mips.cp0r[ CP0_CAUSE ] &= ~CAUSE_BD;
	}
	mips.cp0r[ CP0_SR ] = ( mips.cp0r[ CP0_SR ] & 0xffffffc0 ) | ( ( mips.cp0r[ CP0_SR ] << 2 ) & 0x3f );
	if( mips.cp0r[ CP0_SR ] & SR_BEV )
	{
		mips_set_pc( 0xbfc00180 );
	}
	else
	{
		mips_set_pc( 0x80000080 );
	}
}

void mips_stop( void )
{
#ifdef MAME_DEBUG
	extern int debug_key_pressed;
	debug_key_pressed = 1;
	CALL_MAME_DEBUG;
#endif
}

INLINE void mips_set_nextpc( UINT32 adr )
{
	mips.pc = mips.nextpc;
	mips.op = mips.nextop;
	if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 3 ) ) != 0 )
	{
		mips.cp0r[ CP0_BADVADDR ] = adr;
		mips_exception( EXC_ADEL );
	}
	else
	{
		change_pc32( adr );
		mips.nextpc = adr;
		mips.nextop = cpu_readop32( mips.nextpc );
	}
}

INLINE void mips_advance_pc( void )
{
	mips.pc = mips.nextpc;
	mips.op = mips.nextop;
	mips.nextpc += 4;
	mips.nextop = cpu_readop32( mips.nextpc );
}

void mips_reset( void *param )
{
	mips.cp0r[ CP0_RANDOM ] = 63;
	mips.cp0r[ CP0_SR ] = ( mips.cp0r[ CP0_SR ] & ~( SR_TS | SR_SWC | SR_KUC | SR_IEC ) ) | SR_BEV;
	mips_set_pc( 0xbfc00000 );
}

void mips_exit( void )
{
}

int mips_execute( int cycles )
{
	mips_ICount = cycles;
	do
	{
		mips.op = cpu_readop32(mips.pc);
		mips.pc += 4;

        CALL_MAME_DEBUG;

		switch( INS_OP( mips.op ) )
		{
		case OP_SPECIAL:
			switch( INS_FUNCT( mips.op ) )
			{
			case FUNCT_SLL:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RT( mips.op ) ] << INS_SHAMT( mips.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRL:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RT( mips.op ) ] >> INS_SHAMT( mips.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRA:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = (INT32)mips.r[ INS_RT( mips.op ) ] >> INS_SHAMT( mips.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SLLV:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RT( mips.op ) ] << ( mips.r[ INS_RS( mips.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRLV:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RT( mips.op ) ] >> ( mips.r[ INS_RS( mips.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRAV:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = (INT32)mips.r[ INS_RT( mips.op ) ] >> ( mips.r[ INS_RS( mips.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_JR:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips_set_nextpc( mips.r[ INS_RS( mips.op ) ] );
				}
				break;
			case FUNCT_JALR:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.nextpc + 4;
				}
				mips_set_nextpc( mips.r[ INS_RS( mips.op ) ] );
				break;
			case FUNCT_SYSCALL:
				mips_exception( EXC_SYS );
				break;
			case FUNCT_BREAK:
				mips_exception( EXC_BP );
				break;
			case FUNCT_MFHI:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.hi;
				}
				mips_advance_pc();
				break;
			case FUNCT_MTHI:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips.hi = mips.r[ INS_RS( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_MFLO:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.lo;
				}
				mips_advance_pc();
				break;
			case FUNCT_MTLO:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips.lo = mips.r[ INS_RS( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_MULT:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					INT64 res;
					res = MUL_64_32_32( (INT32)mips.r[ INS_RS( mips.op ) ], (INT32)mips.r[ INS_RT( mips.op ) ] );
					mips.lo = LO32_32_64( res );
					mips.hi = HI32_32_64( res );
					mips_advance_pc();
				}
				break;
			case FUNCT_MULTU:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					UINT64 res;
					res = MUL_U64_U32_U32( (INT32)mips.r[ INS_RS( mips.op ) ], (INT32)mips.r[ INS_RT( mips.op ) ] );
					mips.lo = LO32_U32_U64( res );
					mips.hi = HI32_U32_U64( res );
					mips_advance_pc();
				}
				break;
			case FUNCT_DIV:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips.lo = (INT32)mips.r[ INS_RS( mips.op ) ] / (INT32)mips.r[ INS_RT( mips.op ) ];
					mips.hi = (INT32)mips.r[ INS_RS( mips.op ) ] % (INT32)mips.r[ INS_RT( mips.op ) ];
					mips_advance_pc();
				}
				break;
			case FUNCT_DIVU:
				if( INS_RD( mips.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips.lo = mips.r[ INS_RS( mips.op ) ] / mips.r[ INS_RT( mips.op ) ];
					mips.hi = mips.r[ INS_RS( mips.op ) ] % mips.r[ INS_RT( mips.op ) ];
					mips_advance_pc();
				}
				break;
			case FUNCT_ADD:
				{
					UINT32 res;
					res = mips.r[ INS_RS( mips.op ) ] + mips.r[ INS_RT( mips.op ) ];
					if( (INT32)( ( mips.r[ INS_RS( mips.op ) ] & mips.r[ INS_RT( mips.op ) ] & ~res ) | ( ~mips.r[ INS_RS( mips.op ) ] & ~mips.r[ INS_RT( mips.op ) ] & res ) ) < 0 )
					{
						mips_exception( EXC_OVF );
					}
					else
					{
						if( INS_RD( mips.op ) != 0 )
						{
							mips.r[ INS_RD( mips.op ) ] = res;
						}
						mips_advance_pc();
					}
				}
				break;
			case FUNCT_ADDU:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] + mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_SUB:
				{
					UINT32 res;
					res = mips.r[ INS_RS( mips.op ) ] - mips.r[ INS_RT( mips.op ) ];
					if( (INT32)( ( mips.r[ INS_RS( mips.op ) ] & ~mips.r[ INS_RT( mips.op ) ] & ~res ) | ( ~mips.r[ INS_RS( mips.op ) ] & mips.r[ INS_RT( mips.op ) ] & res ) ) < 0 )
					{
						mips_exception( EXC_OVF );
					}
					else
					{
						if( INS_RD( mips.op ) != 0 )
						{
							mips.r[ INS_RD( mips.op ) ] = res;
						}
						mips_advance_pc();
					}
				}
				break;
			case FUNCT_SUBU:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] - mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_AND:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] & mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_OR:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] | mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_XOR:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] ^ mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_NOR:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = ~( mips.r[ INS_RS( mips.op ) ] | mips.r[ INS_RT( mips.op ) ] );
				}
				mips_advance_pc();
				break;
			case FUNCT_SLT:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = (INT32)mips.r[ INS_RS( mips.op ) ] < (INT32)mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_SLTU:
				if( INS_RD( mips.op ) != 0 )
				{
					mips.r[ INS_RD( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] < mips.r[ INS_RT( mips.op ) ];
				}
				mips_advance_pc();
				break;
			default:
				mips_exception( EXC_RI );
				break;
			}
			break;
		case OP_REGIMM:
			switch( INS_RT( mips.op ) )
			{
			case RT_BLTZ:
				if( (INT32)mips.r[ INS_RS( mips.op ) ] < 0 )
				{
					mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BGEZ:
				if( (INT32)mips.r[ INS_RS( mips.op ) ] >= 0 )
				{
					mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BLTZAL:
				mips.r[ 31 ] = mips.nextpc + 4;
				if( (INT32)mips.r[ INS_RS( mips.op ) ] < 0 )
				{
					mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BGEZAL:
				mips.r[ 31 ] = mips.nextpc + 4;
				if( (INT32)mips.r[ INS_RS( mips.op ) ] >= 0 )
				{
					mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			}
			break;
		case OP_J:
			mips_set_nextpc( ( mips.nextpc & 0xF0000000 ) + ( INS_TARGET( mips.op ) << 2 ) );
			break;
		case OP_JAL:
			mips.r[ 31 ] = mips.nextpc + 4;
			mips_set_nextpc( ( mips.nextpc & 0xF0000000 ) + ( INS_TARGET( mips.op ) << 2 ) );
			break;
		case OP_BEQ:
			if( mips.r[ INS_RS( mips.op ) ] == mips.r[ INS_RT( mips.op ) ] )
			{
				mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BNE:
			if( mips.r[ INS_RS( mips.op ) ] != mips.r[ INS_RT( mips.op ) ] )
			{
				mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BLEZ:
			if( INS_RT( mips.op ) != 0 )
			{
				mips_exception( EXC_RI );
			}
			else if( (INT32)mips.r[ INS_RS( mips.op ) ] <= 0 )
			{
				mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BGTZ:
			if( INS_RT( mips.op ) != 0 )
			{
				mips_exception( EXC_RI );
			}
			else if( (INT32)mips.r[ INS_RS( mips.op ) ] > 0 )
			{
				mips_set_nextpc( mips.nextpc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_ADDI:
			{
				UINT32 res,imm;
				imm = MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				res = mips.r[ INS_RS( mips.op ) ] + imm;
				if( (INT32)( ( mips.r[ INS_RS( mips.op ) ] & imm & ~res ) | ( ~mips.r[ INS_RS( mips.op ) ] & ~imm & res ) ) < 0 )
				{
					mips_exception( EXC_OVF );
				}
				else
				{
					if( INS_RT( mips.op ) != 0 )
					{
						mips.r[ INS_RT( mips.op ) ] = res;
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_ADDIU:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
			}
			mips_advance_pc();
			break;
		case OP_SLTI:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = (INT32)mips.r[ INS_RS( mips.op ) ] < MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
			}
			mips_advance_pc();
			break;
		case OP_SLTIU:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] < (unsigned)MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
			}
			mips_advance_pc();
			break;
		case OP_ANDI:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] & INS_IMMEDIATE( mips.op );
			}
			mips_advance_pc();
			break;
		case OP_ORI:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] | INS_IMMEDIATE( mips.op );
			}
			mips_advance_pc();
			break;
		case OP_XORI:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = mips.r[ INS_RS( mips.op ) ] ^ INS_IMMEDIATE( mips.op );
			}
			mips_advance_pc();
			break;
		case OP_LUI:
			if( INS_RT( mips.op ) != 0 )
			{
				mips.r[ INS_RT( mips.op ) ] = INS_IMMEDIATE( mips.op ) << 16;
			}
			mips_advance_pc();
			break;
		case OP_COP0:
			switch( INS_RS( mips.op ) )
			{
			case RS_MFC:
				mips.r[ INS_RT( mips.op ) ] = mips.cp0r[ INS_RD( mips.op ) ];
				mips_advance_pc();
				break;
			case RS_CFC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_MTC:
				mips.cp0r[ INS_RD( mips.op ) ] = mips.r[ INS_RT( mips.op ) ];
				mips_advance_pc();
				break;
			case RS_CTC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_BC:
				mips_stop();
				switch( INS_RT( mips.op ) )
				{
				case RT_BCF:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				case RT_BCT:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			default:
				switch( INS_CO( mips.op ) )
				{
				case 1:
					switch( INS_CF( mips.op ) )
					{
					case CF_RFE:
						mips.cp0r[ CP0_SR ] = ( mips.cp0r[ CP0_SR ] & 0xffffffe0 ) | ( ( mips.cp0r[ CP0_SR ] >> 2 ) & 0x1f );
						mips_advance_pc();
						break;
					default:
						/* not implemented */
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				default:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			}
			break;
		case OP_COP1:
			switch( INS_RS( mips.op ) )
			{
			case RS_MFC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_CFC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_MTC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_CTC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_BC:
				mips_stop();
				switch( INS_RT( mips.op ) )
				{
				case RT_BCF:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				case RT_BCT:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			default:
				mips_stop();
				switch( INS_CO( mips.op ) )
				{
				case 1:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				default:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			}
			break;
		case OP_COP2:
			switch( INS_RS( mips.op ) )
			{
			case RS_MFC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_CFC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_MTC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_CTC:
				/* not implemented */
				mips_stop();
				mips_advance_pc();
				break;
			case RS_BC:
				mips_stop();
				switch( INS_RT( mips.op ) )
				{
				case RT_BCF:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				case RT_BCT:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			default:
				mips_stop();
				switch( INS_CO( mips.op ) )
				{
				case 1:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				default:
					/* not implemented */
					mips_stop();
					mips_advance_pc();
					break;
				}
				break;
			}
			break;
		case OP_LB:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADEL );
				}
				else
				{	
					mips.r[ INS_RT( mips.op ) ] = MIPS_BYTE_EXTEND( cpu_readmem32lew( adr ) );
					mips_advance_pc();
				}
			}
			break;
		case OP_LH:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 1 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADEL );
				}
				else
				{
					mips.r[ INS_RT( mips.op ) ] = MIPS_WORD_EXTEND( cpu_readmem32lew_word( adr ) );
					mips_advance_pc();
				}
			}
			break;
		case OP_LWL:
			mips_stop();
			/* not implemented */
			mips_advance_pc();
			break;
		case OP_LW:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 3 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADEL );
				}
				else
				{
					mips.r[ INS_RT( mips.op ) ] = cpu_readmem32lew_dword( adr );
					mips_advance_pc();
				}
			}
			break;
		case OP_LBU:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADEL );
				}
				else
				{
					mips.r[ INS_RT( mips.op ) ] = cpu_readmem32lew( adr );
					mips_advance_pc();
				}
			}
			break;
		case OP_LHU:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 1 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADEL );
				}
				else
				{
					mips.r[ INS_RT( mips.op ) ] = cpu_readmem32lew_word( adr );
					mips_advance_pc();
				}
			}
			break;
		case OP_LWR:
			mips_stop();
			/* not implemented */
			mips_advance_pc();
			break;
		case OP_SB:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADES );
				}
				else
				{
					cpu_writemem32lew( adr, mips.r[ INS_RT( mips.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SH:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 1 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADES );
				}
				else
				{
					cpu_writemem32lew_word( adr, mips.r[ INS_RT( mips.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SWL:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_SW:
			if( mips.cp0r[ CP0_SR ] & SR_RE || mips.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* not implemented */
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mips.r[ INS_RS( mips.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mips.op ) );
				if( ( adr & ( ( ( mips.cp0r[ CP0_SR ] & 0x02 ) << 31 ) | 3 ) ) != 0 )
				{
					mips.cp0r[ CP0_BADVADDR ] = adr;
					mips_exception( EXC_ADES );
				}
				else
				{
					cpu_writemem32lew_dword( adr, mips.r[ INS_RT( mips.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SWR:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_LWC1:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_LWC2:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_SWC1:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_SWC2:
			/* not implemented */
			mips_stop();
			mips_advance_pc();
			break;
		default:
			mips_exception( EXC_RI );
			break;
		}
		mips_ICount--;
	} while( mips_ICount > 0 );

	return cycles - mips_ICount;
}

unsigned mips_get_context( void *dst )
{
	if( dst )
	{
		*(mips_cpu_context *)dst = mips;
	}
	return sizeof( mips_cpu_context );
}

void mips_set_context( void *src )
{
	if( src )
	{
		mips = *(mips_cpu_context *)src;
		change_pc32( mips.nextpc );
	}
}

unsigned mips_get_pc( void )
{
	return mips.pc;
}

unsigned mips_get_op_counter( void )
{
	return mips.nextpc;
}

void mips_set_pc( unsigned val )
{
	mips_set_nextpc( val );
	mips_advance_pc();
}

unsigned mips_get_sp(void)
{
	/* because there is no hardware stack and the pipeline causes the cpu to execute the
	instruction after a subroutine call before the subroutine there is little chance of
	cmd_step_over() in mamedbg.c working. */
	return 0;
}

void mips_set_sp( unsigned val )
{
	/* no hardware stack */
}

unsigned mips_get_reg( int regnum )
{
    switch( regnum )
    {
	case MIPS_PC:		return mips.pc;
	case MIPS_OC:		return mips.nextpc;
	case MIPS_HI:		return mips.hi;
	case MIPS_LO:		return mips.lo;
	case MIPS_R0:		return mips.r[ 0 ];
	case MIPS_R1:		return mips.r[ 1 ];
	case MIPS_R2:		return mips.r[ 2 ];
	case MIPS_R3:		return mips.r[ 3 ];
	case MIPS_R4:		return mips.r[ 4 ];
	case MIPS_R5:		return mips.r[ 5 ];
	case MIPS_R6:		return mips.r[ 6 ];
	case MIPS_R7:		return mips.r[ 7 ];
	case MIPS_R8:		return mips.r[ 8 ];
	case MIPS_R9:		return mips.r[ 9 ];
	case MIPS_R10:		return mips.r[ 10 ];
	case MIPS_R11:		return mips.r[ 11 ];
	case MIPS_R12:		return mips.r[ 12 ];
	case MIPS_R13:		return mips.r[ 13 ];
	case MIPS_R14:		return mips.r[ 14 ];
	case MIPS_R15:		return mips.r[ 15 ];
	case MIPS_R16:		return mips.r[ 16 ];
	case MIPS_R17:		return mips.r[ 17 ];
	case MIPS_R18:		return mips.r[ 18 ];
	case MIPS_R19:		return mips.r[ 19 ];
	case MIPS_R20:		return mips.r[ 20 ];
	case MIPS_R21:		return mips.r[ 21 ];
	case MIPS_R22:		return mips.r[ 22 ];
	case MIPS_R23:		return mips.r[ 23 ];
	case MIPS_R24:		return mips.r[ 24 ];
	case MIPS_R25:		return mips.r[ 25 ];
	case MIPS_R26:		return mips.r[ 26 ];
	case MIPS_R27:		return mips.r[ 27 ];
	case MIPS_R28:		return mips.r[ 28 ];
	case MIPS_R29:		return mips.r[ 29 ];
	case MIPS_R30:		return mips.r[ 30 ];
	case MIPS_R31:		return mips.r[ 31 ];
	case MIPS_CP0R0:	return mips.cp0r[ 0 ];
	case MIPS_CP0R1:	return mips.cp0r[ 1 ];
	case MIPS_CP0R2:	return mips.cp0r[ 2 ];
	case MIPS_CP0R3:	return mips.cp0r[ 3 ];
	case MIPS_CP0R4:	return mips.cp0r[ 4 ];
	case MIPS_CP0R5:	return mips.cp0r[ 5 ];
	case MIPS_CP0R6:	return mips.cp0r[ 6 ];
	case MIPS_CP0R7:	return mips.cp0r[ 7 ];
	case MIPS_CP0R8:	return mips.cp0r[ 8 ];
	case MIPS_CP0R9:	return mips.cp0r[ 9 ];
	case MIPS_CP0R10:	return mips.cp0r[ 10 ];
	case MIPS_CP0R11:	return mips.cp0r[ 11 ];
	case MIPS_CP0R12:	return mips.cp0r[ 12 ];
	case MIPS_CP0R13:	return mips.cp0r[ 13 ];
	case MIPS_CP0R14:	return mips.cp0r[ 14 ];
	case MIPS_CP0R15:	return mips.cp0r[ 15 ];
	case MIPS_CP0R16:	return mips.cp0r[ 16 ];
	case MIPS_CP0R17:	return mips.cp0r[ 17 ];
	case MIPS_CP0R18:	return mips.cp0r[ 18 ];
	case MIPS_CP0R19:	return mips.cp0r[ 19 ];
	case MIPS_CP0R20:	return mips.cp0r[ 20 ];
	case MIPS_CP0R21:	return mips.cp0r[ 21 ];
	case MIPS_CP0R22:	return mips.cp0r[ 22 ];
	case MIPS_CP0R23:	return mips.cp0r[ 23 ];
	case MIPS_CP0R24:	return mips.cp0r[ 24 ];
	case MIPS_CP0R25:	return mips.cp0r[ 25 ];
	case MIPS_CP0R26:	return mips.cp0r[ 26 ];
	case MIPS_CP0R27:	return mips.cp0r[ 27 ];
	case MIPS_CP0R28:	return mips.cp0r[ 28 ];
	case MIPS_CP0R29:	return mips.cp0r[ 29 ];
	case MIPS_CP0R30:	return mips.cp0r[ 30 ];
	case MIPS_CP0R31:	return mips.cp0r[ 31 ];
    }
    return 0;
}

void mips_set_reg( int regnum, unsigned val )
{
	switch( regnum )
	{
	case MIPS_PC:		mips_set_pc( val );	break;
	case MIPS_OC:		mips_set_nextpc( val );	break;
	case MIPS_HI:		mips.hi = val;		break;
	case MIPS_LO:		mips.lo = val;		break;
	case MIPS_R0:		mips.r[ 0 ] = val;	break;
	case MIPS_R1:		mips.r[ 1 ] = val;	break;
	case MIPS_R2:		mips.r[ 2 ] = val;	break;
	case MIPS_R3:		mips.r[ 3 ] = val;	break;
	case MIPS_R4:		mips.r[ 4 ] = val;	break;
	case MIPS_R5:		mips.r[ 5 ] = val;	break;
	case MIPS_R6:		mips.r[ 6 ] = val;	break;
	case MIPS_R7:		mips.r[ 7 ] = val;	break;
	case MIPS_R8:		mips.r[ 8 ] = val;	break;
	case MIPS_R9:		mips.r[ 9 ] = val;	break;
	case MIPS_R10:		mips.r[ 10 ] = val;	break;
	case MIPS_R11:		mips.r[ 11 ] = val;	break;
	case MIPS_R12:		mips.r[ 12 ] = val;	break;
	case MIPS_R13:		mips.r[ 13 ] = val;	break;
	case MIPS_R14:		mips.r[ 14 ] = val;	break;
	case MIPS_R15:		mips.r[ 15 ] = val;	break;
	case MIPS_R16:		mips.r[ 16 ] = val;	break;
	case MIPS_R17:		mips.r[ 17 ] = val;	break;
	case MIPS_R18:		mips.r[ 18 ] = val;	break;
	case MIPS_R19:		mips.r[ 19 ] = val;	break;
	case MIPS_R20:		mips.r[ 20 ] = val;	break;
	case MIPS_R21:		mips.r[ 21 ] = val;	break;
	case MIPS_R22:		mips.r[ 22 ] = val;	break;
	case MIPS_R23:		mips.r[ 23 ] = val;	break;
	case MIPS_R24:		mips.r[ 24 ] = val;	break;
	case MIPS_R25:		mips.r[ 25 ] = val;	break;
	case MIPS_R26:		mips.r[ 26 ] = val;	break;
	case MIPS_R27:		mips.r[ 27 ] = val;	break;
	case MIPS_R28:		mips.r[ 28 ] = val;	break;
	case MIPS_R29:		mips.r[ 29 ] = val;	break;
	case MIPS_R30:		mips.r[ 30 ] = val;	break;
	case MIPS_R31:		mips.r[ 31 ] = val;	break;
	case MIPS_CP0R0:	mips.cp0r[ 0 ] = val;	break;
	case MIPS_CP0R1:	mips.cp0r[ 1 ] = val;	break;
	case MIPS_CP0R2:	mips.cp0r[ 2 ] = val;	break;
	case MIPS_CP0R3:	mips.cp0r[ 3 ] = val;	break;
	case MIPS_CP0R4:	mips.cp0r[ 4 ] = val;	break;
	case MIPS_CP0R5:	mips.cp0r[ 5 ] = val;	break;
	case MIPS_CP0R6:	mips.cp0r[ 6 ] = val;	break;
	case MIPS_CP0R7:	mips.cp0r[ 7 ] = val;	break;
	case MIPS_CP0R8:	mips.cp0r[ 8 ] = val;	break;
	case MIPS_CP0R9:	mips.cp0r[ 9 ] = val;	break;
	case MIPS_CP0R10:	mips.cp0r[ 10 ] = val;	break;
	case MIPS_CP0R11:	mips.cp0r[ 11 ] = val;	break;
	case MIPS_CP0R12:	mips.cp0r[ 12 ] = val;	break;
	case MIPS_CP0R13:	mips.cp0r[ 13 ] = val;	break;
	case MIPS_CP0R14:	mips.cp0r[ 14 ] = val;	break;
	case MIPS_CP0R15:	mips.cp0r[ 15 ] = val;	break;
	case MIPS_CP0R16:	mips.cp0r[ 16 ] = val;	break;
	case MIPS_CP0R17:	mips.cp0r[ 17 ] = val;	break;
	case MIPS_CP0R18:	mips.cp0r[ 18 ] = val;	break;
	case MIPS_CP0R19:	mips.cp0r[ 19 ] = val;	break;
	case MIPS_CP0R20:	mips.cp0r[ 20 ] = val;	break;
	case MIPS_CP0R21:	mips.cp0r[ 21 ] = val;	break;
	case MIPS_CP0R22:	mips.cp0r[ 22 ] = val;	break;
	case MIPS_CP0R23:	mips.cp0r[ 23 ] = val;	break;
	case MIPS_CP0R24:	mips.cp0r[ 24 ] = val;	break;
	case MIPS_CP0R25:	mips.cp0r[ 25 ] = val;	break;
	case MIPS_CP0R26:	mips.cp0r[ 26 ] = val;	break;
	case MIPS_CP0R27:	mips.cp0r[ 27 ] = val;	break;
	case MIPS_CP0R28:	mips.cp0r[ 28 ] = val;	break;
	case MIPS_CP0R29:	mips.cp0r[ 29 ] = val;	break;
	case MIPS_CP0R30:	mips.cp0r[ 30 ] = val;	break;
	case MIPS_CP0R31:	mips.cp0r[ 31 ] = val;	break;
	}
}

void mips_set_nmi_line( int state )
{
	switch( state )
	{
		case CLEAR_LINE:
			return;
		case ASSERT_LINE:
			return;
		default:
			return;
	}
}

void mips_set_irq_line( int irqline, int state )
{
	switch( state )
	{
		case CLEAR_LINE:
			return;
		case ASSERT_LINE:
			return;
		default:
			return;
	}
}

void mips_set_irq_callback(int (*callback)(int irqline))
{
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/

const char *mips_info( void *context, int regnum )
{
	static char buffer[ 64 ][ 47 + 1 ];
	static int which = 0;
	mips_cpu_context *r = context;

	which = ++which % 64;
	buffer[ which ][ 0 ] = '\0';
	if( !context )
	{
		static mips_cpu_context tmp;
		mips_get_context( &tmp );
		r = &tmp;
	}

	switch( regnum )
	{
	case CPU_INFO_REG + MIPS_PC:		sprintf( buffer[ which ], "pc      :%08x", r->pc );				break;
	case CPU_INFO_REG + MIPS_OC:		sprintf( buffer[ which ], "oc      :%08x", r->nextpc );			break;
	case CPU_INFO_REG + MIPS_HI:		sprintf( buffer[ which ], "hi      :%08x", r->hi );				break;
	case CPU_INFO_REG + MIPS_LO:		sprintf( buffer[ which ], "lo      :%08x", r->lo );				break;
	case CPU_INFO_REG + MIPS_R0:		sprintf( buffer[ which ], "zero    :%08x", r->r[ 0 ] );			break;
	case CPU_INFO_REG + MIPS_R1:		sprintf( buffer[ which ], "at      :%08x", r->r[ 1 ] );			break;
	case CPU_INFO_REG + MIPS_R2:		sprintf( buffer[ which ], "v0      :%08x", r->r[ 2 ] );			break;
	case CPU_INFO_REG + MIPS_R3:		sprintf( buffer[ which ], "v1      :%08x", r->r[ 3 ] );			break;
	case CPU_INFO_REG + MIPS_R4:		sprintf( buffer[ which ], "a0      :%08x", r->r[ 4 ] );			break;
	case CPU_INFO_REG + MIPS_R5:		sprintf( buffer[ which ], "a1      :%08x", r->r[ 5 ] );			break;
	case CPU_INFO_REG + MIPS_R6:		sprintf( buffer[ which ], "a2      :%08x", r->r[ 6 ] );			break;
	case CPU_INFO_REG + MIPS_R7:		sprintf( buffer[ which ], "a3      :%08x", r->r[ 7 ] );			break;
	case CPU_INFO_REG + MIPS_R8:		sprintf( buffer[ which ], "t0      :%08x", r->r[ 8 ] );			break;
	case CPU_INFO_REG + MIPS_R9:		sprintf( buffer[ which ], "t1      :%08x", r->r[ 9 ] );			break;
	case CPU_INFO_REG + MIPS_R10:		sprintf( buffer[ which ], "t2      :%08x", r->r[ 10 ] );		break;
	case CPU_INFO_REG + MIPS_R11:		sprintf( buffer[ which ], "t3      :%08x", r->r[ 11 ] );		break;
	case CPU_INFO_REG + MIPS_R12:		sprintf( buffer[ which ], "t4      :%08x", r->r[ 12 ] );		break;
	case CPU_INFO_REG + MIPS_R13:		sprintf( buffer[ which ], "t5      :%08x", r->r[ 13 ] );		break;
	case CPU_INFO_REG + MIPS_R14:		sprintf( buffer[ which ], "t6      :%08x", r->r[ 14 ] );		break;
	case CPU_INFO_REG + MIPS_R15:		sprintf( buffer[ which ], "t7      :%08x", r->r[ 15 ] );		break;
	case CPU_INFO_REG + MIPS_R16:		sprintf( buffer[ which ], "s0      :%08x", r->r[ 16 ] );		break;
	case CPU_INFO_REG + MIPS_R17:		sprintf( buffer[ which ], "s1      :%08x", r->r[ 17 ] );		break;
	case CPU_INFO_REG + MIPS_R18:		sprintf( buffer[ which ], "s2      :%08x", r->r[ 18 ] );		break;
	case CPU_INFO_REG + MIPS_R19:		sprintf( buffer[ which ], "s3      :%08x", r->r[ 19 ] );		break;
	case CPU_INFO_REG + MIPS_R20:		sprintf( buffer[ which ], "s4      :%08x", r->r[ 20 ] );		break;
	case CPU_INFO_REG + MIPS_R21:		sprintf( buffer[ which ], "s5      :%08x", r->r[ 21 ] );		break;
	case CPU_INFO_REG + MIPS_R22:		sprintf( buffer[ which ], "s6      :%08x", r->r[ 22 ] );		break;
	case CPU_INFO_REG + MIPS_R23:		sprintf( buffer[ which ], "s7      :%08x", r->r[ 23 ] );		break;
	case CPU_INFO_REG + MIPS_R24:		sprintf( buffer[ which ], "t8      :%08x", r->r[ 24 ] );		break;
	case CPU_INFO_REG + MIPS_R25:		sprintf( buffer[ which ], "t9      :%08x", r->r[ 25 ] );		break;
	case CPU_INFO_REG + MIPS_R26:		sprintf( buffer[ which ], "k0      :%08x", r->r[ 26 ] );		break;
	case CPU_INFO_REG + MIPS_R27:		sprintf( buffer[ which ], "k1      :%08x", r->r[ 27 ] );		break;
	case CPU_INFO_REG + MIPS_R28:		sprintf( buffer[ which ], "gp      :%08x", r->r[ 28 ] );		break;
	case CPU_INFO_REG + MIPS_R29:		sprintf( buffer[ which ], "sp      :%08x", r->r[ 29 ] );		break;
	case CPU_INFO_REG + MIPS_R30:		sprintf( buffer[ which ], "fp      :%08x", r->r[ 30 ] );		break;
	case CPU_INFO_REG + MIPS_R31:		sprintf( buffer[ which ], "ra      :%08x", r->r[ 31 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R0:		sprintf( buffer[ which ], "Index   :%08x", r->cp0r[ 0 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R1:		sprintf( buffer[ which ], "Random  :%08x", r->cp0r[ 1 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R2:		sprintf( buffer[ which ], "EntryLo :%08x", r->cp0r[ 2 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R3:		sprintf( buffer[ which ], "cp0r3   :%08x", r->cp0r[ 3 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R4:		sprintf( buffer[ which ], "Context :%08x", r->cp0r[ 4 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R5:		sprintf( buffer[ which ], "cp0r5   :%08x", r->cp0r[ 5 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R6:		sprintf( buffer[ which ], "cp0r6   :%08x", r->cp0r[ 6 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R7:		sprintf( buffer[ which ], "cp0r7   :%08x", r->cp0r[ 7 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R8:		sprintf( buffer[ which ], "BadVAddr:%08x", r->cp0r[ 8 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R9:		sprintf( buffer[ which ], "cp0r9   :%08x", r->cp0r[ 9 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R10:	sprintf( buffer[ which ], "EntryHi :%08x", r->cp0r[ 10 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R11:	sprintf( buffer[ which ], "cp0r11  :%08x", r->cp0r[ 11 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R12:	sprintf( buffer[ which ], "SR      :%08x", r->cp0r[ 12 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R13:	sprintf( buffer[ which ], "Cause   :%08x", r->cp0r[ 13 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R14:	sprintf( buffer[ which ], "EPC     :%08x", r->cp0r[ 14 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R15:	sprintf( buffer[ which ], "PRId    :%08x", r->cp0r[ 15 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R16:	sprintf( buffer[ which ], "cp0r16  :%08x", r->cp0r[ 16 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R17:	sprintf( buffer[ which ], "cp0r17  :%08x", r->cp0r[ 17 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R18:	sprintf( buffer[ which ], "cp0r18  :%08x", r->cp0r[ 18 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R19:	sprintf( buffer[ which ], "cp0r19  :%08x", r->cp0r[ 19 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R20:	sprintf( buffer[ which ], "cp0r20  :%08x", r->cp0r[ 20 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R21:	sprintf( buffer[ which ], "cp0r21  :%08x", r->cp0r[ 21 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R22:	sprintf( buffer[ which ], "cp0r22  :%08x", r->cp0r[ 22 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R23:	sprintf( buffer[ which ], "cp0r23  :%08x", r->cp0r[ 23 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R24:	sprintf( buffer[ which ], "cp0r24  :%08x", r->cp0r[ 24 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R25:	sprintf( buffer[ which ], "cp0r25  :%08x", r->cp0r[ 25 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R26:	sprintf( buffer[ which ], "cp0r26  :%08x", r->cp0r[ 26 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R27:	sprintf( buffer[ which ], "cp0r27  :%08x", r->cp0r[ 27 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R28:	sprintf( buffer[ which ], "cp0r28  :%08x", r->cp0r[ 28 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R29:	sprintf( buffer[ which ], "cp0r29  :%08x", r->cp0r[ 29 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R30:	sprintf( buffer[ which ], "cp0r30  :%08x", r->cp0r[ 30 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R31:	sprintf( buffer[ which ], "cp0r31  :%08x", r->cp0r[ 31 ] );		break;
	case CPU_INFO_FLAGS:		return "";
	case CPU_INFO_NAME:			return "PSX";
	case CPU_INFO_FAMILY:		return "MIPS";
	case CPU_INFO_VERSION:		return "1.0";
	case CPU_INFO_FILE:			return __FILE__;
	case CPU_INFO_CREDITS:		return "Copyright 2000 smf";
	case CPU_INFO_REG_LAYOUT:	return (const char*)mips_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char*)mips_win_layout;
	}
	return buffer[ which ];
}

unsigned mips_dasm( char *buffer, UINT32 pc )
{
	unsigned ret;
	change_pc32( pc );
#ifdef MAME_DEBUG
	ret = DasmMIPS( buffer, pc );
#else
	sprintf( buffer, "$%08x", cpu_readop32( pc ) );
	ret = 4;
#endif
	change_pc32( mips.nextpc );
	return ret;
}
