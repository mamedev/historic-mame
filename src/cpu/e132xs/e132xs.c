/********************************************************************
 Hyperstone cpu emulator
 written by Pierpaolo Prazzoli

 All the types are compatible, but they have different IRAM size:
 - Hyperstone E1-32
 - Hyperstone E1-16
 - Hyperstone E1-32X
 - Hyperstone E1-16X
 - Hyperstone E1-32XN
 - Hyperstone E1-32XT
 - Hyperstone E1-16XT
 - Hyperstone E1-32XS
 - Hyperstone E1-16XS
 - Hyperstone E1-32XP (ever released?)
 - Hyperstone E1-32XSB (compatible?)
 - Hyperstone E1-16XSB (compatible?)


 CHANGELOG:

 Tomasz Slanina
 - interrputs after call and before frame are prohibited now
 - emulation of FCR register
 - Floating point opcodes (preliminary)
 - Fixed stack addressing in RET/FRAME opcodes
 - Fixed bug in SET_RS macro
 - Fixed bug in return opcode (S flag)
 - Added C/N flags calculation in add/adc/addi/adds/addsi and some shift opcodes
 - Added writeback to ROL
 - Fixed ROL/SAR/SARD/SHR/SHRD/SHL/SHLD opcode decoding (Local/Global regs)
 - Fixed I and T flag in RET opcode
 - Fixed XX/XM opcodes
 - Fixed MOV opcode, when RD = PC
 - Fixed execute_trap()
 - Fixed ST opcodes, when when RS = SR
 - Added interrupts
 - Fixed I/O addressing

 Pierpaolo Prazzoli
 - Fixed fetch
 - Fixed decode of e132xs_xm opcode
 - Fixed 7 bits difference number in FRAME / RET instructions
 - Some debbugger fixes
 - Added generic registers decode function
 - Some other little fixes.

 MooglyGuy 29/03/2004
    - Changed MOVI to use unsigned values instead of signed, correcting
      an ugly glitch when loading 32-bit immediates.
 Pierpaolo Prazzoli
	- Same fix in get_const

 MooglyGuy - 02/27/04
    - Fixed delayed branching
    - const_val for CALL should always have bit 0 clear

 Pierpaolo Prazzoli - 02/25/04
	- Fixed some wrong addresses to address local registers instead of memory
	- Fixed FRAME and RET instruction
	- Added preliminary I/O space
	- Fixed some load / store instructions

 Pierpaolo Prazzoli - 02/20/04
	- Added execute_exception function
	- Added FL == 0 always interpreted as 16

 Pierpaolo Prazzoli - 02/19/04
	- Changed the reset to use the execute_trap(reset) which should be right to set
	  the initiale state of the cpu
	- Added Trace exception
	- Set of T flag in RET instruction
	- Set I flag in interrupts entries and resetted by a RET instruction
	- Added correct set instruction for SR

 Pierpaolo Prazzoli - 10/26/03
	- Changed get_lrconst to get_const and changed it to use the removed GET_CONST_RR
	  macro.
	- Removed the High flag used in some opcodes, it should be used only in
	  MOV and MOVI instruction.
	- Fixed MOV and MOVI instruction.
	- Set to 1 FP is SR register at reset.
	  (From the doc: A Call, Trap or Software instruction increments the FP and sets FL
	  to 6, thus creating a new stack frame with the length of 6 registers).

 MooglyGuy - 10/25/03
 	- Fixed CALL enough that it at least jumps to the right address, no word
 	  yet as to whether or not it's working enough to return.
 	- Added get_lrconst() to get the const value for the CALL operand, since
 	  apparently using immediate_value() was wrong. The code is ugly, but it
 	  works properly. Vampire 1/2 now gets far enough to try to test its RAM.
 	- Just from looking at it, CALL apparently doesn't frame properly. I'm not
 	  sure about FRAME, but perhaps it doesn't work properly - I'm not entirely
 	  positive. The return address when vamphalf's memory check routine is
 	  called at FFFFFD7E is stored in register L8, and then the RET instruction
 	  at the end of the routine uses L1 as the return address, so that might
 	  provide some clues as to how it works.
 	- I'd almost be willing to bet money that there's no framing at all since
 	  the values in L0 - L15 as displayed by the debugger would change during a
 	  CALL or FRAME operation. I'll look when I'm in the mood.
 	- The mood struck me, and I took a look at SET_L_REG and GET_L_REG.
 	  Apparently no matter what the current frame pointer is they'll always use
 	  local_regs[0] through local_regs[15].

 MooglyGuy - 08/20/03
 	- Added H flag support for MOV and MOVI
 	- Changed init routine to set S flag on boot. Apparently the CPU defaults to
 	  supervisor mode as opposed to user mode when it powers on, as shown by the
 	  vamphalf power-on routines. Makes sense, too, since if the machine booted
 	  in user mode, it would be impossible to get into supervisor mode.

 Pierpaolo Prazzoli	- 08/19/03
	- Added check for D_BIT and S_BIT where PC or SR must or must not be denoted.
	  (movd, divu, divs, ldxx1, ldxx2, stxx1, stxx2, mulu, muls, set, mul
	  call, chk)

 MooglyGuy - 08/17/03
	- Working on support for H flag, nothing quite done yet
	- Added trap Range Error for CHK PC, PC
	- Fixed relative jumps, they have to be taken from the opcode following the
	  jump minstead of the jump opcode itself.

 Pierpaolo Prazzoli	- 08/17/03
	- Fixed get_pcrel() when OP & 0x80 is set.
	- Decremented PC by 2 also in MOV, ADD, ADDI, SUM, SUB and added the check if
	  D_BIT is not set. (when pc is changed they are implicit branch)

 MooglyGuy - 08/17/03
    - Implemented a crude hack to set FL in the SR to 6, since according to the docs
      that's supposed to happen each time a trap occurs, apparently including when
      the processor starts up. The 3rd opcode executed in vamphalf checks to see if
      the FL flag in SR 6, so it's apparently the "correct" behaviour despite the
      docs not saying anything on it. If FL is not 6, the branch falls through and
      encounters a CHK PC, L2, which at that point will always throw a range trap.
      The range trap vector contains 00000000 (CHK PC, PC), which according to the
      docs will always throw a range trap (which would effectively lock the system).
      This revealed a bug: CHK PC, PC apparently does not throw a range trap, which
      needs to be fixed. Now that the "correct" behaviour is hacked in with the FL
      flags, it reveals yet another bug in that the branch is interpreted as being
      +0x8700. This means that the PC then wraps around to 000082B0, give or take
      a few bytes. While it does indeed branch to valid code, I highly doubt that
      this is the desired effect. Check for signed/unsigned relative branch, maybe?

 MooglyGuy - 08/16/03
 	- Fixed the debugger at least somewhat so that it displays hex instead of decimal,
 	  and so that it disassembles opcodes properly.
 	- Fixed e132xs_execute() to increment PC *after* executing the opcode instead of
 	  before. This is probably why vamphalf was booting to fffffff8, but executing at
 	  fffffffa instead.
 	- Changed execute_trap to decrement PC by 2 so that the next opcode isn't skipped
 	  after a trap
 	- Changed execute_br to decrement PC by 2 so that the next opcode isn't skipped
 	  after a branch
 	- Changed e132xs_movi to decrement PC by 2 when G0 (PC) is modified so that the
 	  next opcode isn't skipped after a branch
 	- Changed e132xs_movi to default to a UINT32 being moved into the register
 	  as opposed to a UINT8. This is wrong, the bit width is quite likely to be
 	  dependent on the n field in the Rimm instruction type. However, vamphalf uses
 	  MOVI G0,[FFFF]FBAC (n=$13) since there's apparently no absolute branch opcode.
 	  What kind of CPU is this that it doesn't have an absolute jump in its branch
 	  instructions and you have to use an immediate MOV to do an abs. jump!?
 	- Replaced usage of logerror() with smf's verboselog()

*********************************************************************/

#include "math.h"
#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "e132xs.h"

// set C in adds/addsi/subs/sums
#define SETCARRYS 0
#define MISSIONCRAFT_FLAGS 1

static int e132xs_ICount;

/* Local variables */
static int h_clear;
static int instruction_length;
static int intblock = 0;

void e132xs_chk(void);
void e132xs_movd(void);
void e132xs_divu(void);
void e132xs_divs(void);
void e132xs_xm(void);
void e132xs_mask(void);
void e132xs_sum(void);
void e132xs_sums(void);
void e132xs_cmp(void);
void e132xs_mov(void);
void e132xs_add(void);
void e132xs_adds(void);
void e132xs_cmpb(void);
void e132xs_andn(void);
void e132xs_or(void);
void e132xs_xor(void);
void e132xs_subc(void);
void e132xs_not(void);
void e132xs_sub(void);
void e132xs_subs(void);
void e132xs_addc(void);
void e132xs_and(void);
void e132xs_neg(void);
void e132xs_negs(void);
void e132xs_cmpi(void);
void e132xs_movi(void);
void e132xs_addi(void);
void e132xs_addsi(void);
void e132xs_cmpbi(void);
void e132xs_andni(void);
void e132xs_ori(void);
void e132xs_xori(void);
void e132xs_shrdi(void);
void e132xs_shrd(void);
void e132xs_shr(void);
void e132xs_sardi(void);
void e132xs_sard(void);
void e132xs_sar(void);
void e132xs_shldi(void);
void e132xs_shld(void);
void e132xs_shl(void);
void reserved(void);
void e132xs_testlz(void);
void e132xs_rol(void);
void e132xs_ldxx1(void);
void e132xs_ldxx2(void);
void e132xs_stxx1(void);
void e132xs_stxx2(void);
void e132xs_shri(void);
void e132xs_sari(void);
void e132xs_shli(void);
void e132xs_mulu(void);
void e132xs_muls(void);
void e132xs_set(void);
void e132xs_mul(void);
void e132xs_fadd(void);
void e132xs_faddd(void);
void e132xs_fsub(void);
void e132xs_fsubd(void);
void e132xs_fmul(void);
void e132xs_fmuld(void);
void e132xs_fdiv(void);
void e132xs_fdivd(void);
void e132xs_fcmp(void);
void e132xs_fcmpd(void);
void e132xs_fcmpu(void);
void e132xs_fcmpud(void);
void e132xs_fcvt(void);
void e132xs_fcvtd(void);
void e132xs_extend(void);
void e132xs_do(void);
void e132xs_ldwr(void);
void e132xs_lddr(void);
void e132xs_ldwp(void);
void e132xs_lddp(void);
void e132xs_stwr(void);
void e132xs_stdr(void);
void e132xs_stwp(void);
void e132xs_stdp(void);
void e132xs_dbv(void);
void e132xs_dbnv(void);
void e132xs_dbe(void);
void e132xs_dbne(void);
void e132xs_dbc(void);
void e132xs_dbnc(void);
void e132xs_dbse(void);
void e132xs_dbht(void);
void e132xs_dbn(void);
void e132xs_dbnn(void);
void e132xs_dble(void);
void e132xs_dbgt(void);
void e132xs_dbr(void);
void e132xs_frame(void);
void e132xs_call(void);
void e132xs_bv(void);
void e132xs_bnv(void);
void e132xs_be(void);
void e132xs_bne(void);
void e132xs_bc(void);
void e132xs_bnc(void);
void e132xs_bse(void);
void e132xs_bht(void);
void e132xs_bn(void);
void e132xs_bnn(void);
void e132xs_ble(void);
void e132xs_bgt(void);
void e132xs_br(void);
void e132xs_trap(void);

/* Registers */
enum
{
	E132XS_PC = 1,
	E132XS_SR,
	E132XS_FER,
	E132XS_G3,
	E132XS_G4,
	E132XS_G5,
	E132XS_G6,
	E132XS_G7,
	E132XS_G8,
	E132XS_G9,
	E132XS_G10,
	E132XS_G11,
	E132XS_G12,
	E132XS_G13,
	E132XS_G14,
	E132XS_G15,
	E132XS_G16,
	E132XS_G17,
	E132XS_SP,
	E132XS_UB,
	E132XS_BCR,
	E132XS_TPR,
	E132XS_TCR,
	E132XS_TR,
	E132XS_WCR,
	E132XS_ISR,
	E132XS_FCR,
	E132XS_MCR,
	E132XS_G28,
	E132XS_G29,
	E132XS_G30,
	E132XS_G31,
	E132XS_CL0, E132XS_CL1, E132XS_CL2, E132XS_CL3,
	E132XS_CL4, E132XS_CL5, E132XS_CL6, E132XS_CL7,
	E132XS_CL8, E132XS_CL9, E132XS_CL10,E132XS_CL11,
	E132XS_CL12,E132XS_CL13,E132XS_CL14,E132XS_CL15,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15,
	E132XS_L16, E132XS_L17, E132XS_L18, E132XS_L19,
	E132XS_L20, E132XS_L21, E132XS_L22, E132XS_L23,
	E132XS_L24, E132XS_L25, E132XS_L26, E132XS_L27,
	E132XS_L28, E132XS_L29, E132XS_L30, E132XS_L31,
	E132XS_L32, E132XS_L33, E132XS_L34, E132XS_L35,
	E132XS_L36, E132XS_L37, E132XS_L38, E132XS_L39,
	E132XS_L40, E132XS_L41, E132XS_L42, E132XS_L43,
	E132XS_L44, E132XS_L45, E132XS_L46, E132XS_L47,
	E132XS_L48, E132XS_L49, E132XS_L50, E132XS_L51,
	E132XS_L52, E132XS_L53, E132XS_L54, E132XS_L55,
	E132XS_L56, E132XS_L57, E132XS_L58, E132XS_L59,
	E132XS_L60, E132XS_L61, E132XS_L62, E132XS_L63
};


static UINT8 e132xs_reg_layout[] =
{
	E132XS_PC,  E132XS_SR,  E132XS_FER, E132XS_G3,  -1,
	E132XS_G4,  E132XS_G5,  E132XS_G6,  E132XS_G7,  -1,
	E132XS_G8,  E132XS_G9,  E132XS_G10, E132XS_G11, -1,
	E132XS_G12, E132XS_G13, E132XS_G14,	E132XS_G15, -1,
	E132XS_G16,	E132XS_G17,	E132XS_SP,	E132XS_UB,  -1,
	E132XS_BCR,	E132XS_TPR,	E132XS_TCR,	E132XS_TR,  -1,
	E132XS_WCR,	E132XS_ISR,	E132XS_FCR,	E132XS_MCR, -1,
	E132XS_G28, E132XS_G29,	E132XS_G30,	E132XS_G31, -1,
	E132XS_CL0, E132XS_CL1, E132XS_CL2, E132XS_CL3, -1,
	E132XS_CL4, E132XS_CL5, E132XS_CL6, E132XS_CL7, -1,
	E132XS_CL8, E132XS_CL9, E132XS_CL10,E132XS_CL11,-1,
	E132XS_CL12,E132XS_CL13,E132XS_CL14,E132XS_CL15,-1,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,  -1,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,  -1,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11, -1,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15, -1,
	E132XS_L16, E132XS_L17, E132XS_L18, E132XS_L19, -1,
	E132XS_L20, E132XS_L21, E132XS_L22, E132XS_L23, -1,
	E132XS_L24, E132XS_L25, E132XS_L26, E132XS_L27, -1,
	E132XS_L28, E132XS_L29, E132XS_L30, E132XS_L31, -1,
	E132XS_L32, E132XS_L33, E132XS_L34, E132XS_L35, -1,
	E132XS_L36, E132XS_L37, E132XS_L38, E132XS_L39, -1,
	E132XS_L40, E132XS_L41, E132XS_L42, E132XS_L43, -1,
	E132XS_L44, E132XS_L45, E132XS_L46, E132XS_L47, -1,
	E132XS_L48, E132XS_L49, E132XS_L50, E132XS_L51, -1,
	E132XS_L52, E132XS_L53, E132XS_L54, E132XS_L55, -1,
	E132XS_L56, E132XS_L57, E132XS_L58, E132XS_L59, -1,
	E132XS_L60, E132XS_L61, E132XS_L62, E132XS_L63, 0
};

UINT8 e132xs_win_layout[] =
{
	 0, 0,80, 8, /* register window (top rows) */
	 0, 9,34,13, /* disassembler window (left, middle columns) */
	35, 9,46, 6, /* memory #1 window (right, upper middle) */
	35,16,46, 6, /* memory #2 window (right, lower middle) */
	 0,23,80, 1  /* command line window (bottom row) */
};


/* Internal registers */
typedef struct
{
	UINT32	global_regs[32];
	UINT32	local_regs[64];

	/* internal stuff */
	UINT32	ppc;	// previous pc
	UINT16	op;		// opcode
	UINT8	delay;
	UINT32	delay_pc;
	UINT32	trap_entry; // entry point to get trap address

	int	(*irq_callback)(int irqline);

} e132xs_regs;

static e132xs_regs e132xs;

struct regs_decode
{
	UINT8	src, dst;	    // destination and source register code
	UINT32	src_value;      // current source register value
	UINT32	next_src_value; // current next source register value
	UINT32	dst_value;      // current destination register value
	UINT32	next_dst_value; // current next destination register value
	void	(*set_src_register)(UINT8 reg, UINT32 data);  // set local / global source register
	void	(*set_dst_register)(UINT8 reg, UINT32 data);  // set local / global destination register
	UINT8	src_is_pc;
	UINT8	dst_is_pc;
	UINT8	src_is_sr;
	UINT8	dst_is_sr;
	UINT8   same_src_dst;
	UINT8   same_src_dstf;
	UINT8   same_srcf_dst;
} current_regs;

#define SREG  current_regs.src_value
#define SREGF current_regs.next_src_value
#define DREG  current_regs.dst_value
#define DREGF current_regs.next_dst_value

#define SET_SREG( _data_ )  (*current_regs.set_src_register)(current_regs.src, _data_)
#define SET_SREGF( _data_ ) (*current_regs.set_src_register)(current_regs.src + 1, _data_)
#define SET_DREG( _data_ )  (*current_regs.set_dst_register)(current_regs.dst, _data_)
#define SET_DREGF( _data_ ) (*current_regs.set_dst_register)(current_regs.dst + 1, _data_)

#define SRC_IS_PC      current_regs.src_is_pc
#define DST_IS_PC      current_regs.dst_is_pc
#define SRC_IS_SR      current_regs.src_is_sr
#define DST_IS_SR      current_regs.dst_is_sr
#define SAME_SRC_DST   current_regs.same_src_dst
#define SAME_SRC_DSTF  current_regs.same_src_dstf
#define SAME_SRCF_DST  current_regs.same_srcf_dst

#if 1

/* Opcodes table */
static void (*e132xs_op[0x100])(void) = {
	e132xs_chk,	 e132xs_chk,  e132xs_chk,   e132xs_chk,		/* CHK - CHKZ - NOP */
	e132xs_movd, e132xs_movd, e132xs_movd,  e132xs_movd,	/* MOVD - RET */
	e132xs_divu, e132xs_divu, e132xs_divu,  e132xs_divu,	/* DIVU */
	e132xs_divs, e132xs_divs, e132xs_divs,  e132xs_divs,	/* DIVS */
	e132xs_xm,	 e132xs_xm,   e132xs_xm,    e132xs_xm,		/* XMx - XXx */
	e132xs_mask, e132xs_mask, e132xs_mask,  e132xs_mask,	/* MASK */
	e132xs_sum,  e132xs_sum,  e132xs_sum,   e132xs_sum,		/* SUM */
	e132xs_sums, e132xs_sums, e132xs_sums,  e132xs_sums,	/* SUMS */
	e132xs_cmp,  e132xs_cmp,  e132xs_cmp,   e132xs_cmp,		/* CMP */
	e132xs_mov,  e132xs_mov,  e132xs_mov,   e132xs_mov,		/* MOV */
	e132xs_add,  e132xs_add,  e132xs_add,   e132xs_add,		/* ADD */
	e132xs_adds, e132xs_adds, e132xs_adds,  e132xs_adds,	/* ADDS */
	e132xs_cmpb, e132xs_cmpb, e132xs_cmpb,  e132xs_cmpb,	/* CMPB */
	e132xs_andn, e132xs_andn, e132xs_andn,  e132xs_andn,	/* ANDN */
	e132xs_or,   e132xs_or,   e132xs_or,    e132xs_or,		/* OR */
	e132xs_xor,  e132xs_xor,  e132xs_xor,   e132xs_xor,		/* XOR */
	e132xs_subc, e132xs_subc, e132xs_subc,  e132xs_subc,	/* SUBC */
	e132xs_not,  e132xs_not,  e132xs_not,   e132xs_not,		/* NOT */
	e132xs_sub,  e132xs_sub,  e132xs_sub,   e132xs_sub,		/* SUB */
	e132xs_subs, e132xs_subs, e132xs_subs,  e132xs_subs,	/* SUBS */
	e132xs_addc, e132xs_addc, e132xs_addc,  e132xs_addc,	/* ADDC */
	e132xs_and,  e132xs_and,  e132xs_and,   e132xs_and,		/* AND */
	e132xs_neg,  e132xs_neg,  e132xs_neg,   e132xs_neg,		/* NEG */
	e132xs_negs, e132xs_negs, e132xs_negs,  e132xs_negs,	/* NEGS */
	e132xs_cmpi, e132xs_cmpi, e132xs_cmpi,  e132xs_cmpi,	/* CMPI */
	e132xs_movi, e132xs_movi, e132xs_movi,  e132xs_movi,	/* MOVI */
	e132xs_addi, e132xs_addi, e132xs_addi,  e132xs_addi,	/* ADDI */
	e132xs_addsi,e132xs_addsi,e132xs_addsi, e132xs_addsi,	/* ADDSI */
	e132xs_cmpbi,e132xs_cmpbi,e132xs_cmpbi, e132xs_cmpbi,	/* CMPBI */
	e132xs_andni,e132xs_andni,e132xs_andni, e132xs_andni,	/* ANDNI */
	e132xs_ori,  e132xs_ori,  e132xs_ori,   e132xs_ori,		/* ORI */
	e132xs_xori, e132xs_xori, e132xs_xori,  e132xs_xori,	/* XORI */
	e132xs_shrdi,e132xs_shrdi,e132xs_shrd,  e132xs_shr,		/* SHRDI, SHRD, SHR */
	e132xs_sardi,e132xs_sardi,e132xs_sard,  e132xs_sar,		/* SARDI, SARD, SAR */
	e132xs_shldi,e132xs_shldi,e132xs_shld,  e132xs_shl,		/* SHLDI, SHLD, SHL */
	reserved,    reserved,	  e132xs_testlz,e132xs_rol,		/* RESERVED, TESTLZ, ROL */
	e132xs_ldxx1,e132xs_ldxx1,e132xs_ldxx1, e132xs_ldxx1,	/* LDxx.D/A/IOD/IOA */
	e132xs_ldxx2,e132xs_ldxx2,e132xs_ldxx2, e132xs_ldxx2,	/* LDxx.N/S */
	e132xs_stxx1,e132xs_stxx1,e132xs_stxx1,e132xs_stxx1,	/* STxx.D/A/IOD/IOA */
	e132xs_stxx2,e132xs_stxx2,e132xs_stxx2,e132xs_stxx2,	/* STxx.N/S */
	e132xs_shri, e132xs_shri, e132xs_shri,  e132xs_shri,	/* SHRI */
	e132xs_sari, e132xs_sari, e132xs_sari,  e132xs_sari,	/* SARI */
	e132xs_shli, e132xs_shli, e132xs_shli,  e132xs_shli,	/* SHLI */
	reserved,    reserved,    reserved,     reserved,		/* RESERVED */
	e132xs_mulu, e132xs_mulu, e132xs_mulu,  e132xs_mulu,	/* MULU */
	e132xs_muls, e132xs_muls, e132xs_muls,  e132xs_muls,	/* MULS */
	e132xs_set,  e132xs_set,  e132xs_set,   e132xs_set,		/* SETxx - SETADR - FETCH */
	e132xs_mul,  e132xs_mul,  e132xs_mul,   e132xs_mul,		/* MUL */
	e132xs_fadd, e132xs_faddd,e132xs_fsub,  e132xs_fsubd,	/* FADD, FADDD, FSUB, FSUBD */
	e132xs_fmul, e132xs_fmuld,e132xs_fdiv,  e132xs_fdivd,	/* FMUL, FMULD, FDIV, FDIVD */
	e132xs_fcmp, e132xs_fcmpd,e132xs_fcmpu, e132xs_fcmpud,	/* FCMP, FCMPD, FCMPU, FCMPUD */
	e132xs_fcvt, e132xs_fcvtd,e132xs_extend,e132xs_do,		/* FCVT, FCVTD, EXTEND, DO */
	e132xs_ldwr, e132xs_ldwr, e132xs_lddr,  e132xs_lddr,	/* LDW.R, LDD.R */
	e132xs_ldwp, e132xs_ldwp, e132xs_lddp,  e132xs_lddp,	/* LDW.P, LDD.P */
	e132xs_stwr, e132xs_stwr, e132xs_stdr,  e132xs_stdr,	/* STW.R, STD.R */
	e132xs_stwp, e132xs_stwp, e132xs_stdp,  e132xs_stdp,	/* STW.P, STD.P */
	e132xs_dbv,  e132xs_dbnv, e132xs_dbe,   e132xs_dbne,	/* DBV, DBNV, DBE, DBNE */
	e132xs_dbc,  e132xs_dbnc, e132xs_dbse,  e132xs_dbht,	/* DBC, DBNC, DBSE, DBHT */
	e132xs_dbn,  e132xs_dbnn, e132xs_dble,  e132xs_dbgt,	/* DBN, DBNN, DBLE, DBGT */
	e132xs_dbr,  e132xs_frame,e132xs_call,  e132xs_call,	/* DBR, FRAME, CALL */
	e132xs_bv,   e132xs_bnv,  e132xs_be,    e132xs_bne,		/* BV, BNV, BE, BNE */
	e132xs_bc,   e132xs_bnc,  e132xs_bse,   e132xs_bht,		/* BC, BNC, BSE, BHT */
	e132xs_bn,   e132xs_bnn,  e132xs_ble,   e132xs_bgt,		/* BN, BNN, BLE, BGT */
 	e132xs_br,   e132xs_trap, e132xs_trap,  e132xs_trap		/* BR, TRAPxx - TRAP */
};

#else

static void (*e132xs_op[0x100])(void) = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, e132xs_movi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

/* Return the entry point for a determinated trap */
UINT32 get_trap_addr(UINT8 trapno)
{
	UINT32 addr;
	if( e132xs.trap_entry == 0xffffff00 ) /* @ MEM3 */
	{
		addr = trapno * 4;
	}
	else
	{
		addr = (63 - trapno) * 4;
	}
	addr |= e132xs.trap_entry;

	return addr;
}

/* Return the entry point for a determinated emulated code (the one for "extend" opcode is reserved) */
UINT32 get_emu_code_addr(UINT8 num) /* num is OP */
{
	UINT32 addr;
	if( e132xs.trap_entry == 0xffffff00 ) /* @ MEM3 */
	{
		addr = (e132xs.trap_entry - 0x100) | (num << 4);
	}
	else
	{
		addr = e132xs.trap_entry | (0x10c | ((0x0f - num) << 4));
	}
	return addr;
}

void e132xs_set_trap_entry(int which)
{
	switch( which )
	{
		case E132XS_ENTRY_MEM0:
			e132xs.trap_entry = 0x00000000;
			break;

		case E132XS_ENTRY_MEM1:
			e132xs.trap_entry = 0x40000000;
			break;

		case E132XS_ENTRY_MEM2:
			e132xs.trap_entry = 0x80000000;
			break;

		case E132XS_ENTRY_MEM3:
			e132xs.trap_entry = 0xffffff00;
			break;

		case E132XS_ENTRY_IRAM:
			e132xs.trap_entry = 0xc0000000;
			break;

		default:
			logerror("Set entry point to a reserved value: %d\n", which);
			break;
	}
}

#define OP				e132xs.op

#define PPC				e132xs.ppc //previous pc
#define PC				e132xs.global_regs[0] //Program Counter
#define SR				e132xs.global_regs[1] //Status Register
#define FER				e132xs.global_regs[2] //Floating-Point Exception Register
// 03 - 15	General Purpose Registers
// 16 - 17	Reserved
#define SP				e132xs.global_regs[18] //Stack Pointer
#define UB				e132xs.global_regs[19] //Upper Stack Bound
#define BCR				e132xs.global_regs[20] //Bus Control Register
#define TPR				e132xs.global_regs[21] //Timer Prescaler Register
#define TCR				e132xs.global_regs[22] //Timer Compare Register
#define TR				e132xs.global_regs[23] //Timer Register
#define WCR				e132xs.global_regs[24] //Watchdog Compare Register
#define ISR				e132xs.global_regs[25] //Input Status Register
#define FCR				e132xs.global_regs[26] //Function Control Register
#define MCR				e132xs.global_regs[27] //Memory Control Register
// 28 - 31	Reserved

/* SR flags */
#define GET_C					( SR & 0x00000001)      // bit 0 //CARRY
#define GET_Z					((SR & 0x00000002)>>1)  // bit 1 //ZERO
#define GET_N					((SR & 0x00000004)>>2)  // bit 2 //NEGATIVE
#define GET_V					((SR & 0x00000008)>>3)  // bit 3 //OVERFLOW
#define GET_M					((SR & 0x00000010)>>4)  // bit 4 //CACHE-MODE
#define GET_H					((SR & 0x00000020)>>5)  // bit 5 //HIGHGLOBAL
// bit 6 RESERVED (always 0)
#define GET_I					((SR & 0x00000080)>>7)  // bit 7 //INTERRUPT-MODE
#define GET_FTE					((SR & 0x00001f00)>>8)  // bits 12 - 8 	//Floating-Point Trap Enable
#define GET_FRM					((SR & 0x00006000)>>13) // bits 14 - 13 //Floating-Point Rounding Mode
#define GET_L					((SR & 0x00008000)>>15) // bit 15 //INTERRUPT-LOCK
#define GET_T					((SR & 0x00010000)>>16) // bit 16 //TRACE-MODE
#define GET_P					((SR & 0x00020000)>>17) // bit 17 //TRACE PENDING
#define GET_S					((SR & 0x00040000)>>18) // bit 18 //SUPERVISOR STATE
#define GET_ILC					((SR & 0x00180000)>>19) // bits 20 - 19 //INSTRUCTION-LENGTH
/* if FL is zero it is always interpreted as 16 */
#define GET_FL					((SR & 0x01e00000) ? ((SR & 0x01e00000)>>21) : 16) // bits 24 - 21 //FRAME LENGTH
#define GET_FP					((SR & 0xfe000000)>>25) // bits 31 - 25 //FRAME POINTER

#define SET_C(val)				(SR = (SR & ~0x00000001) | (val))
#define SET_Z(val)				(SR = (SR & ~0x00000002) | ((val) << 1))
#define SET_N(val)				(SR = (SR & ~0x00000004) | ((val) << 2))
#define SET_V(val)				(SR = (SR & ~0x00000008) | ((val) << 3))
#define SET_M(val)				(SR = (SR & ~0x00000010) | ((val) << 4))
#define SET_H(val)				(SR = (SR & ~0x00000020) | ((val) << 5))
#define SET_I(val)				(SR = (SR & ~0x00000080) | ((val) << 7))
#define SET_FTE(val)			(SR = (SR & ~0x00001f00) | ((val) << 8))
#define	SET_FRM(val)			(SR = (SR & ~0x00006000) | ((val) << 13))
#define SET_L(val)				(SR = (SR & ~0x00008000) | ((val) << 15))
#define SET_T(val)				(SR = (SR & ~0x00010000) | ((val) << 16))
#define SET_P(val)				(SR = (SR & ~0x00020000) | ((val) << 17))
#define SET_S(val)				(SR = (SR & ~0x00040000) | ((val) << 18))
#define SET_ILC(val)			(SR = (SR & ~0x00180000) | ((val) << 19))
#define SET_FL(val)				(SR = (SR & ~0x01e00000) | ((val) << 21))
#define SET_FP(val)				(SR = (SR & ~0xfe000000) | ((val) << 25))

#define SET_PC(val)				PC = ((val) & 0xfffffffe) //PC(0) = 0
#define SET_SP(val)				SP = ((val) & 0xfffffffc) //SP(0) = SP(1) = 0
#define SET_UB(val)				UB = ((val) & 0xfffffffc) //UB(0) = UB(1) = 0

#define SET_LOW_SR(val)			(SR = (SR & 0xffff0000) | ((val) & 0x0000ffff)) // when SR is addressed, only low 16 bits can be changed


#define CHECK_C(x) 				(SR = (SR & ~0x00000001) | (((x) & (((UINT64)1) << 32)) ? 1 : 0 ))
#define CHECK_VADD(x,y,z)		(SR = (SR & ~0x00000008) | ((((x) ^ (z)) & ((y) ^ (z)) & 0x80000000) ? 8: 0))
#define CHECK_VADD3(x,y,w,z)	(SR = (SR & ~0x00000008) | ((((x) ^ (z)) & ((y) ^ (z)) & ((w) ^ (z)) & 0x80000000) ? 8: 0))
#define CHECK_VSUB(x,y,z)		(SR = (SR & ~0x00000008) | ((((z) ^ (y)) & ((y) ^ (x)) & 0x80000000) ? 8: 0))


/* FER flags */
#define GET_ACCRUED				(FER & 0x0000001f) //bits  4 - 0 //Floating-Point Accrued Exceptions
#define GET_ACTUAL				(FER & 0x00001f00) //bits 12 - 8 //Floating-Point Actual  Exceptions
//other bits are reversed, in particular 7 - 5 for the operating system.
//the user program can only changes the above 2 flags

UINT32 get_global_register(UINT8 code)
{
	if( code >= 16 )
	{
		switch( code )
		{
		case 16:
		case 17:
		case 28:
		case 29:
		case 30:
		case 31:
			printf("read _Reserved_ Global Register %d @ %08X\n",code,PC);
			break;

		case BCR_REGISTER:
			printf("read write-only BCR register @ %08X\n",PC);
			return 0;

		case TPR_REGISTER:
			printf("read write-only TPR register @ %08X\n",PC);
			return 0;

		case FCR_REGISTER:
			printf("read write-only FCR register @ %08X\n",PC);
			return 0;

		case MCR_REGISTER:
			printf("read write-only MCR register @ %08X\n",PC);
			return 0;
		}
	}

	/* TODO: if PC is used in a delay instruction, the delayed PC is used */

	return e132xs.global_regs[code];
}

void set_global_register(UINT8 code, UINT32 val)
{
	//TODO: add correct FER set instruction

	if( code == PC_REGISTER )
	{
		SET_PC(val);
	}
	else if( code == SR_REGISTER )
	{
		SET_LOW_SR(val); // only a RET instruction can change the full content of SR

		SR &= ~0x40; //reserved bit 6 always zero
	}
	else
	{
		if( code != ISR_REGISTER )
			e132xs.global_regs[code] = val;
		else
			logerror("Written to ISR register. PC = %08X\n", PC);

		//are these set only when privilege bit is set?
		if( code >= 16 )
		{
			switch( code )
			{
			case 18:
				SET_SP(val);
				break;

			case 19:
				SET_UB(val);
				break;

			case ISR_REGISTER:
				printf("written %08X to read-only ISR register\n",val);
				break;

			case 22:
//				printf("written %08X to TCR register\n",val);
				break;

			case 23:
//				printf("written %08X to TR register\n",val);
				break;

			case 24:
//				printf("written %08X to WCR register\n",val);
				break;

			case 16:
			case 17:
			case 28:
			case 29:
			case 30:
			case 31:
				printf("written %08X to _Reserved_ Global Register %d\n",val,code);
				break;

			case BCR_REGISTER:
				break;

			case TPR_REGISTER:
				//printf("written %08X to TPR register\n",val);
				break;

			case FCR_REGISTER:
				switch((val & 0x3000)>>12)
				{
				case 0:
					printf("IO3 interrupt mode\n");
					break;
				case 1:
					printf("IO3 timing mode\n");
					break;
				case 2:
					printf("watchdog mode\n");
					break;
				case 3:
					//printf("IO3 standard mode\n");
					break;
				}
				break;

			case MCR_REGISTER:
				// bits 14..12 EntryTableMap
				e132xs_set_trap_entry((val & 0x7000) >> 12);

				break;
			}
		}
	}
}

void set_local_register(UINT8 code, UINT32 val)
{
	UINT8 new_code = (code + GET_FP) % 64;

	e132xs.local_regs[new_code] = val;
}

#define GET_ABS_L_REG(code)			e132xs.local_regs[code]
#define SET_L_REG(code, val)	    set_local_register(code, val)
#define SET_ABS_L_REG(code, val)	e132xs.local_regs[code] = val
#define GET_G_REG(code)				get_global_register(code)
#define SET_G_REG(code, val)	    set_global_register(code, val)

#define S_BIT					((OP & 0x100) >> 8)
#define N_BIT					S_BIT
#define D_BIT					((OP & 0x200) >> 9)
#define N_VALUE					((N_BIT << 4) | (OP & 0x0f))
#define DST_CODE				((OP & 0xf0) >> 4)
#define SRC_CODE				(OP & 0x0f)
#define SIGN_BIT(val)			((val & 0x80000000) >> 31)

#define LOCAL  1

static void decode_source(int local, int hflag)
{
	UINT8 code = current_regs.src;

	if(local)
	{
		current_regs.set_src_register = set_local_register;

		code = (current_regs.src + GET_FP) % 64; // registers offset by frame pointer
		SREG = e132xs.local_regs[code];

		code = (current_regs.src + 1 + GET_FP) % 64;

		SREGF = e132xs.local_regs[code];
	}
	else
	{
		current_regs.set_src_register = set_global_register;

		if(hflag)
			current_regs.src += hflag * 16;

		SREG = e132xs.global_regs[current_regs.src];

		/* bound safe */
		if(code != 15 && code != 31)
			SREGF = e132xs.global_regs[current_regs.src + 1];

		/* TODO: if PC is used in a delay instruction, the delayed PC should be used */

		if(current_regs.src == PC_REGISTER)
			SRC_IS_PC = 1;
		else if(current_regs.src == SR_REGISTER)
			SRC_IS_SR = 1;
		else if( current_regs.src == BCR_REGISTER || current_regs.src == TPR_REGISTER ||
				 current_regs.src == FCR_REGISTER || current_regs.src == MCR_REGISTER )
		{
			SREG = 0; // write-only registers
		}
		else if( current_regs.src == ISR_REGISTER )
			printf("read src ISR. PC = %08X\n",PPC);
	}
}

static void decode_dest(int local, int hflag)
{
	UINT8 code = current_regs.dst;

	if(local)
	{
		current_regs.set_dst_register = set_local_register;

		code = (current_regs.dst + GET_FP) % 64; // registers offset by frame pointer
		DREG = e132xs.local_regs[code];

		code = (current_regs.dst + 1 + GET_FP) % 64;

		DREGF = e132xs.local_regs[code];
	}
	else
	{
		current_regs.set_dst_register = set_global_register;

		if(hflag)
			current_regs.dst += hflag * 16;

		DREG = e132xs.global_regs[current_regs.dst];

		/* bound safe */
		if(code != 15 && code != 31)
			DREGF = e132xs.global_regs[current_regs.dst + 1];

		if(current_regs.dst == PC_REGISTER)
			DST_IS_PC = 1;
		else if(current_regs.dst == SR_REGISTER)
			DST_IS_SR = 1;
		else if( current_regs.src == ISR_REGISTER )
			printf("read dst ISR. PC = %08X\n",PPC);
	}
}

static void decode_registers(void)
{
	memset(&current_regs, 0, sizeof(struct regs_decode));

	switch((OP & 0xff00) >> 8)
	{
		// RR decode
		case 0x00: case 0x01: case 0x02: case 0x03: // CHK - CHKZ - NOP
		case 0x04: case 0x05: case 0x06: case 0x07: // MOVD - RET
		case 0x08: case 0x09: case 0x0a: case 0x0b: // DIVU
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // DIVS
		case 0x20: case 0x21: case 0x22: case 0x23: // CMP
		case 0x28: case 0x29: case 0x2a: case 0x2b: // ADD
		case 0x2c: case 0x2d: case 0x2e: case 0x2f: // ADDS
		case 0x30: case 0x31: case 0x32: case 0x33: // CMPB
		case 0x34: case 0x35: case 0x36: case 0x37: // ANDN
		case 0x38: case 0x39: case 0x3a: case 0x3b: // OR
		case 0x3c: case 0x3d: case 0x3e: case 0x3f: // XOR
		case 0x40: case 0x41: case 0x42: case 0x43: // SUBC
		case 0x44: case 0x45: case 0x46: case 0x47: // NOT
		case 0x48: case 0x49: case 0x4a: case 0x4b: // SUB
		case 0x4c: case 0x4d: case 0x4e: case 0x4f: // SUBS
		case 0x50: case 0x51: case 0x52: case 0x53: // ADDC
		case 0x54: case 0x55: case 0x56: case 0x57: // AND
		case 0x58: case 0x59: case 0x5a: case 0x5b: // NEG
		case 0x5c: case 0x5d: case 0x5e: case 0x5f: // NEGS
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: // MULU
		case 0xb4: case 0xb5: case 0xb6: case 0xb7: // MULS
		case 0xbc: case 0xbd: case 0xbe: case 0xbf: // MUL
		// RRlim decode
		case 0x10: case 0x11: case 0x12: case 0x13: // XMx - XXx
		// RRconst decode
		case 0x14: case 0x15: case 0x16: case 0x17: // MASK
		case 0x18: case 0x19: case 0x1a: case 0x1b: // SUM
		case 0x1c: case 0x1d: case 0x1e: case 0x1f: // SUMS
		// RRdis decode
		case 0x90: case 0x91: case 0x92: case 0x93: // LDxx.D/A/IOD/IOA
		case 0x94: case 0x95: case 0x96: case 0x97: // LDxx.N/S
		case 0x98: case 0x99: case 0x9a: case 0x9b: // STxx.D/A/IOD/IOA
		case 0x9c: case 0x9d: case 0x9e: case 0x9f: // STxx.N/S

			current_regs.src = SRC_CODE;
			current_regs.dst = DST_CODE;
			decode_source(S_BIT, 0);
			decode_dest(D_BIT, 0);

			if( SRC_CODE == DST_CODE && S_BIT == D_BIT )
				SAME_SRC_DST = 1;

			if( SRC_CODE == (DST_CODE + 1) && S_BIT == D_BIT )
				SAME_SRC_DSTF = 1;

			if( (SRC_CODE + 1) == DST_CODE && S_BIT == D_BIT )
				SAME_SRCF_DST = 1;

			break;

		// RR decode with H flag
		case 0x24: case 0x25: case 0x26: case 0x27: // MOV

			current_regs.src = SRC_CODE;
			current_regs.dst = DST_CODE;
			decode_source(S_BIT, GET_H);
			decode_dest(D_BIT, GET_H);

			if(GET_H)
				if(S_BIT == 0 && D_BIT == 0)
					printf("MOV with hflag and 2 GRegs! PC = %08X\n",PPC);

			break;

		// Rimm decode
		case 0x60: case 0x61: case 0x62: case 0x63: // CMPI
		case 0x68: case 0x69: case 0x6a: case 0x6b: // ADDI
		case 0x6c: case 0x6d: case 0x6e: case 0x6f: // ADDSI
		case 0x70: case 0x71: case 0x72: case 0x73: // CMPBI
		case 0x74: case 0x75: case 0x76: case 0x77: // ANDNI
		case 0x78: case 0x79: case 0x7a: case 0x7b: // ORI
		case 0x7c: case 0x7d: case 0x7e: case 0x7f: // XORI
		// Rn decode
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: // SHRI
		case 0xa4: case 0xa5: case 0xa6: case 0xa7: // SARI
		case 0xa8: case 0xa9: case 0xaa: case 0xab: // SHLI
		case 0xb8: case 0xb9: case 0xba: case 0xbb: // SETxx - SETADR - FETCH

			current_regs.dst = DST_CODE;
			decode_dest(D_BIT, 0);

			break;

		// Rimm decode with H flag
		case 0x64: case 0x65: case 0x66: case 0x67: // MOVI

			current_regs.dst = DST_CODE;
			decode_dest(D_BIT, GET_H);

			break;

		// Ln decode
		case 0x80: case 0x81: // SHRDI
		case 0x84: case 0x85: // SARDI
		case 0x88: case 0x89: // SHLDI

			current_regs.dst = DST_CODE;
			decode_dest(LOCAL, 0);

			break;

		// LL decode
		case 0x82: // SHRD
		case 0x83: // SHR
		case 0x86: // SARD
		case 0x87: // SAR
		case 0x8a: // SHLD
		case 0x8b: // SHL
		case 0x8e: // TESTLZ
		case 0x8f: // ROL
		case 0xc0: // FADD
		case 0xc1: // FADDD
		case 0xc2: // FSUB
		case 0xc3: // FSUBD
		case 0xc4: // FMUL
		case 0xc5: // FMULD
		case 0xc6: // FDIV
		case 0xc7: // FDIVD
		case 0xc8: // FCMP
		case 0xc9: // FCMPD
		case 0xca: // FCMPU
		case 0xcb: // FCMPUD
		case 0xcc: // FCVT
		case 0xcd: // FCVTD
		case 0xcf: // DO
		case 0xed: // FRAME
		// LLext decode
		case 0xce: // EXTEND

			current_regs.src = SRC_CODE;
			current_regs.dst = DST_CODE;
			decode_source(LOCAL, 0);
			decode_dest(LOCAL, 0);

			if( SRC_CODE == DST_CODE )
				SAME_SRC_DST = 1;

			if( SRC_CODE == (DST_CODE + 1) )
				SAME_SRC_DSTF = 1;

			break;

		// LR decode
		case 0xd0: case 0xd1: // LDW.R
		case 0xd2: case 0xd3: // LDD.R
		case 0xd4: case 0xd5: // LDW.P
		case 0xd6: case 0xd7: // LDD.P
		case 0xd8: case 0xd9: // STW.R
		case 0xda: case 0xdb: // STD.R
		case 0xdc: case 0xdd: // STW.P
		case 0xde: case 0xdf: // STD.P
		// LRconst decode
		case 0xee: case 0xef: // CALL

			current_regs.src = SRC_CODE;
			current_regs.dst = DST_CODE;
			decode_source(S_BIT, 0);
			decode_dest(LOCAL, 0);

			if( (SRC_CODE + 1) == DST_CODE && S_BIT == LOCAL )
				SAME_SRCF_DST = 1;

			break;
/*

		// PCrel decode
		case 0xe0: // DBV
		case 0xe1: // DBNV
		case 0xe2: // DBE
		case 0xe3: // DBNE
		case 0xe4: // DBC
		case 0xe5: // DBNC
		case 0xe6: // DBSE
		case 0xe7: // DBHT
		case 0xe8: // DBN
		case 0xe9: // DBNN
		case 0xea: // DBLE
		case 0xeb: // DBGT
		case 0xec: // DBR
		case 0xf0: // BV
		case 0xf1: // BNV
		case 0xf2: // BE
		case 0xf3: // BNE
		case 0xf4: // BC
		case 0xf5: // BNC
		case 0xf6: // BSE
		case 0xf7: // BHT
		case 0xf8: // BN
		case 0xf9: // BNN
		case 0xfa: // BLE
		case 0xfb: // BGT
		case 0xfc: // BR
			break;

		// PCadr decode
		case 0xfd: case 0xfe: case 0xff: // TRAPxx - TRAP
			break;

		// RESERVED
		case 0x8c: case 0x8d:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			break;
*/
	}
}

UINT32 immediate_value(void)
{
	UINT16 imm1, imm2;
	UINT32 ret;

	switch( N_VALUE )
	{
		case 0:	case 1:  case 2:  case 3:  case 4:  case 5:  case 6:  case 7: case 8:
		case 9:	case 10: case 11: case 12: case 13: case 14: case 15: case 16:
			return N_VALUE;

		case 17:
			instruction_length = 3;
			imm1 = READ_OP(PC);
			PC += 2;
			imm2 = READ_OP(PC);
			PC += 2;
			ret = (imm1 << 16) | imm2;
			return ret;

		case 18:
			instruction_length = 2;
			imm1 = READ_OP(PC);
			PC += 2;
			ret = imm1;
			return ret;

		case 19:
			instruction_length = 2;
			imm1 = READ_OP(PC);
			PC += 2;
			ret = 0xffff0000 | imm1;
			return ret;

		case 20:
			return 32;	// bit 5 = 1, others = 0

		case 21:
			return 64;	// bit 6 = 1, others = 0

		case 22:
			return 128; // bit 7 = 1, others = 0

		case 23:
			return 0x80000000; // bit 31 = 1, others = 0 (2 at the power of 31)

		case 24:
			return -8;

		case 25:
			return -7;

		case 26:
			return -6;

		case 27:
			return -5;

		case 28:
			return -4;

		case 29:
			return -3;

		case 30:
			return -2;

		case 31:
			return -1;
	}
	return 0; //it should never be executed
}

INT32 get_const(void)
{
	INT32 const_val;
	UINT16 imm1;

	instruction_length = 2;
	imm1 = READ_OP(PC);
	PC += 2;

	if( E_BIT(imm1) )
	{
		UINT16 imm2;

		instruction_length = 3;
		imm2 = READ_OP(PC);
		PC += 2;

		const_val = imm2;
		const_val |= ((imm1 & 0x3fff) << 16);

		if( S_BIT_CONST(imm1) )
		{
			const_val |= 0xc0000000;
		}
	}
	else
	{
		const_val = imm1 & 0x3fff;

		if( S_BIT_CONST(imm1) )
		{
			const_val |= 0xffffc000;
		}
	}
	return const_val;
}

INT32 get_pcrel(void)
{
	INT32 ret;

	if( OP & 0x80 )
	{
		UINT16 next;

		instruction_length = 2;
		next = READ_OP(PC);
		PC += 2;

		ret = (OP & 0x7f) << 16;

		ret |= (next & 0xfffe);

		if( next & 1 )
			ret |= 0xff800000;
	}
	else
	{
		ret = OP & 0x7e;

		if( OP & 1 )
			ret |= 0xffffff80;
	}

	return ret;
}

INT32 get_dis(UINT32 val)
{
	INT32 ret;

	if( E_BIT(val) )
	{
		UINT16 next;

		instruction_length = 3;
		next = READ_OP(PC);
		PC += 2;

		ret = next;
		ret |= ((val & 0xfff) << 16);

		if( S_BIT_CONST(val) )
		{
			ret |= 0xf0000000;
		}
	}
	else
	{
		ret = val & 0xfff;
		if( S_BIT_CONST(val) )
		{
			ret |= 0xfffff000;
		}
	}
	return ret;
}

void execute_br(INT32 rel)
{
	PPC = PC;
	PC += rel;
	SET_M(0);

	//TODO: change when there's latency
//	if target is on-chip cache
	e132xs_ICount -= 2;
//	else 2 + memory read latency

}

void execute_dbr(INT32 rel)
{
	e132xs.delay_pc = PC + rel;
	e132xs.delay    = DELAY_TAKEN;

	//TODO: change when there's latency
//	if target in on-chip cache
	e132xs_ICount -= 1;
//	else 1 + memory read latency cycles exceeding (dealy instruction cycles - 1)
}


void execute_trap(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(instruction_length & 3);

	oldSR = SR;

	SET_FL(6);
	SET_FP(reg);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	PPC = PC;
	PC = addr;

//	printf("TRAP! PPC = %08X PC = %08X\n",PPC-2,PC-2);

	e132xs_ICount -= 2;	// TODO: + delay...
}


void execute_int(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(instruction_length & 3);

	oldSR = SR;

	SET_FL(2);
	SET_FP(reg);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);
	SET_I(1);

	PPC = PC;
	PC = addr;

	e132xs_ICount -= 2;	// TODO: + delay...
}

/* TODO: mask Parity Error and Extended Overflow exceptions */
void execute_exception(UINT32 addr)
{
	UINT8 reg;
	UINT32 oldSR;
	reg = GET_FP + GET_FL;

	SET_ILC(instruction_length & 3);

	oldSR = SR;

	SET_FP(reg);
	SET_FL(2);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, oldSR);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	PPC = PC;
	PC = addr;

	printf("EXCEPTION! PPC = %08X PC = %08X\n",PPC-2,PC-2);
	e132xs_ICount -= 2;	// TODO: + delay...
}

static void e132xs_init(void)
{
	int cpu = cpu_getactivecpu();

	//TODO: add other stuffs

	state_save_register_UINT32("E132XS", cpu, "PC",  &PC,  1);
	state_save_register_UINT32("E132XS", cpu, "SR",  &SR,  1);
	state_save_register_UINT32("E132XS", cpu, "FER", &FER, 1);
	state_save_register_UINT32("E132XS", cpu, "SP",  &SP,  1);
	state_save_register_UINT32("E132XS", cpu, "UB",  &UB,  1);
	state_save_register_UINT32("E132XS", cpu, "BCR", &BCR, 1);
	state_save_register_UINT32("E132XS", cpu, "TPR", &TPR, 1);
	state_save_register_UINT32("E132XS", cpu, "TCR", &TCR, 1);
	state_save_register_UINT32("E132XS", cpu, "TR",  &TR,  1);
	state_save_register_UINT32("E132XS", cpu, "WCR", &WCR, 1);
	state_save_register_UINT32("E132XS", cpu, "ISR", &ISR, 1);
	state_save_register_UINT32("E132XS", cpu, "FCR", &FCR, 1);
	state_save_register_UINT32("E132XS", cpu, "MCR", &MCR, 1);

	state_save_register_int("E132XS", cpu, "h_clear", &h_clear);
	state_save_register_int("E132XS", cpu, "instruction_length", &instruction_length);
}

static void e132xs_reset(void *param)
{
	//TODO: Add different reset initializations for BCR, MCR, FCR, TPR

	memset(&e132xs, 0, sizeof(e132xs_regs));

	h_clear = 0;
	instruction_length = 0;

	e132xs_set_trap_entry(E132XS_ENTRY_MEM3); /* default entry point @ MEM3 */

	BCR = ~0;
	MCR = ~0;
	FCR = ~0;

	TPR = 0x8000000;

	PC = get_trap_addr(RESET);

	//is reset right?
	SET_FP(0);
	SET_FL(2);

	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);

	SET_L_REG(0, (PC & 0xfffffffe) | GET_S);
	SET_L_REG(1, SR);

	e132xs_ICount -= 2;
}

static void e132xs_exit(void)
{
	/* nothing */
}

static int e132xs_execute(int cycles)
{
	e132xs_ICount = cycles;

	do
	{
		TR++; //hack!
		PPC = PC;	/* copy PC to previous PC */

		CALL_MAME_DEBUG;

		OP = READ_OP(PC);

		PC += 2;

		if(GET_H)
		{
			h_clear = 1;
		}

		if(e132xs_op[(OP & 0xff00) >> 8] == NULL)
		{
			osd_die("Opcode %02X @ %08X\n", (OP & 0xff00) >> 8, PC);
		}

		instruction_length = 1;

		decode_registers();

		e132xs_op[(OP & 0xff00) >> 8]();

		SET_ILC(instruction_length & 3);

		if(h_clear == 1)
		{
			SET_H(0);
			h_clear = 0;
		}

		if( GET_T && GET_P && !e132xs.delay_pc ) /* Not in a Delayed Branch instructions */
		{
			UINT32 addr = get_trap_addr(TRACE_EXCEPTION);
			execute_exception(addr);
		}

		if( e132xs.delay == DELAY_EXECUTE )
		{
			PC = e132xs.delay_pc;
			e132xs.delay_pc = 0;
			e132xs.delay = NO_DELAY;
		}

		if( e132xs.delay == DELAY_TAKEN )
		{
			e132xs.delay = DELAY_EXECUTE;
		}

		if(intblock>0)
			intblock--;

	} while( e132xs_ICount > 0 );

	return cycles - e132xs_ICount;  //TODO: check this
}

static void e132xs_get_context(void *regs)
{
	/* copy the context */
	if( regs )
		*(e132xs_regs *)regs = e132xs;
}

static void e132xs_set_context(void *regs)
{
	/* copy the context */
	if (regs)
		e132xs = *(e132xs_regs *)regs;

	//TODO: other to do? check interrupt?
}

/*
	IRQ lines :
		0 - IO2 	(trap 48)
		1 - IO1 	(trap 49)
		2 - INT4 	(trap 50)
		3 - INT3	(trap 51)
		4 - INT2	(trap 52)
		5 - INT1	(trap 53)
		6 - IO3		(trap 54)
		7 - TIMER	(trap 55)
*/
static void set_irq_line(int irqline, int state)
{
	/* Interrupt-Lock flag isn't set */

	if(intblock)
		return;

	if( !GET_L && state )
	{
		int execint=0;
		switch(irqline)
		{
			case 0:	 if( (FCR&(1<<6)) && (!(FCR&(1<<4))) ) execint=1; // IO2
				break;
			case 1:	 if( (FCR&(1<<2)) && (!(FCR&(1<<0))) ) execint=1; // IO1
				break;
			case 2:	 if( !(FCR&(1<<31)) ) execint=1; //  int 4
				break;
			case 3:	 if( !(FCR&(1<<30)) ) execint=1; //  int 3
				break;
			case 4:	 if( !(FCR&(1<<29)) ) execint=1; //  int 2
				break;
			case 5:	 if( !(FCR&(1<<28)) ) execint=1; //  int 1
				break;
			case 6:	 if( (FCR&(1<<10)) && (!(FCR&(1<<8))) ) execint=1; // IO3
				break;
			case 7:	 if( !(FCR&(1<<23)) ) execint=1; //  timer
				break;
		}
		if(execint)
		{
			execute_int(get_trap_addr(irqline+48));
			(*e132xs.irq_callback)(irqline);
		}
	}
}

static offs_t e132xs_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	return dasm_e132xs( buffer, pc, GET_H );
#else
	sprintf(buffer, "$%08x", READ_OP(pc));
	return 1;
#endif
}


/* Preliminary floating point opcodes */

// taken from i960 core
static float u2f(UINT32 v)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.vv = v;
	return u.ff;
}

static UINT32 f2u(float f)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.ff = f;
	return u.vv;
}

static float u2d(UINT64 v)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.vv = v;
	return u.dd;
}

static UINT64 d2u(double d)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.dd = d;
	return u.vv;
}




void e132xs_fadd(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	val2+=val1;
	SET_DREG(f2u(val2));
}

void e132xs_faddd(void)
{
	double val1,val2;
	UINT64 v1,v2;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	val2+=val1;
	v1=d2u(val2);
	SET_DREG(HI32_32_64(v1));
	SET_DREGF(LO32_32_64(v1));
}

void e132xs_fsub(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	val2-=val1;
	SET_DREG(f2u(val2));
}

void e132xs_fsubd(void)
{
	double val1,val2;
	UINT64 v1,v2;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	val2-=val1;
	v1=d2u(val2);
	SET_DREG(HI32_32_64(v1));
	SET_DREGF(LO32_32_64(v1));
}

void e132xs_fmul(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	val2*=val1;
	SET_DREG(f2u(val2));
}

void e132xs_fmuld(void)
{
	double val1,val2;
	UINT64 v1,v2;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	val2*=val1;
	v1=d2u(val2);
	SET_DREG(HI32_32_64(v1));
	SET_DREGF(LO32_32_64(v1));
}

void e132xs_fdiv(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	if(val1==0)
		printf("DIV0 !\n");
	else
		val2/=val1;
	SET_DREG(f2u(val2));
}

void e132xs_fdivd(void)
{
	double val1,val2;
	UINT64 v1,v2;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	if(val1==0)
		printf("DIVD0 !\n");
	else
		val2+=val1;
	v1=d2u(val2);
	SET_DREG(HI32_32_64(v1));
	SET_DREGF(LO32_32_64(v1));
}

void e132xs_fcmp(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	int unordered=((isunordered(val1,val2))?1:0);
	SET_Z(((val1==val2)?1:0)&(unordered^1));
	SET_N(((val2<val1)?1:0)|unordered);
	SET_C(((val2<val1)?1:0)&(unordered^1));
	SET_V(unordered);
	// exception when unordered !
}

void e132xs_fcmpd(void)
{
	UINT64 v1,v2;
	int unordered;
	double val1,val2;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	unordered=((isunordered(val1,val2))?1:0);
	SET_Z(((val1==val2)?1:0)&(unordered^1));
	SET_N(((val2<val1)?1:0)|unordered);
	SET_C(((val2<val1)?1:0)&(unordered^1));
	SET_V(unordered);
	// exception when unordered !

}

void e132xs_fcmpu(void)
{
	float val1=u2f(SREG);
	float val2=u2f(DREG);
	int unordered=((isunordered(val1,val2))?1:0);
	SET_Z(((val1==val2)?1:0)&(unordered^1));
	SET_N(((val2<val1)?1:0)|unordered);
	SET_C(((val2<val1)?1:0)&(unordered^1));
	SET_V(unordered);
}

void e132xs_fcmpud(void)
{
	UINT64 v1,v2;
	double val1,val2;
	int unordered;
	v1=COMBINE_U64_U32_U32(SREG,SREGF);
	v2=COMBINE_U64_U32_U32(DREG,DREGF);
	val1=u2d(v1);
	val2=u2d(v2);
	unordered=((isunordered(val1,val2))?1:0);
	SET_Z(((val1==val2)?1:0)&(unordered^1));
	SET_N(((val2<val1)?1:0)|unordered);
	SET_C(((val2<val1)?1:0)&(unordered^1));
	SET_V(unordered);
}

void e132xs_fcvt(void)
{
	double val1=u2d(COMBINE_U64_U32_U32(SREG,SREGF));
	float val2=(float) val1;
	SET_DREG(f2u(val2));
}

void e132xs_fcvtd(void)
{
	float val1=u2f(SREG);
	double val2=(double)val1;
	UINT64 tmp=d2u(val2);
	SET_DREG(HI32_32_64(tmp));
	SET_DREGF(LO32_32_64(tmp));
}


/* Opcodes */

void e132xs_chk(void)
{
	UINT32 addr = get_trap_addr(RANGE_ERROR);

	if( SRC_IS_SR )
	{
		if( DREG == 0 )
			execute_exception(addr);
	}
	else
	{
		if( SRC_IS_PC )
		{
			if( DREG >= SREG )
				execute_exception(addr);
		}
		else
		{
			if( DREG > SREG )
				execute_exception(addr);
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_movd(void)
{
	if( DST_IS_PC ) // Rd denotes PC
	{
		// RET instruction

		UINT8 old_s, old_l;
		INT8 difference; // really it's 7 bits

		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR in RET instruction. PC = %08X\n", PC);
		}
		else
		{
			old_s = GET_S;
			old_l = GET_L;
			PPC = PC;

			PC = SET_PC(SREG);
			SR = (SREGF & 0xffe00000) | ((SREG & 0x01) << 18 ) | (SREGF & 0x3ffff);

			instruction_length = 0; // undefined

			if( (!old_s && GET_S) || (!GET_S && !old_l && GET_L))
			{
				UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
				execute_exception(addr);
			}

			difference = GET_FP - ((SP & 0x1fc) >> 2);

			/* convert to 8 bits */
			if(difference > 63)
				difference = (INT8)(difference|0x80);
			else if( difference < -64 )
				difference = difference & 0x7f;

			if( difference < 0 ) //else it's finished
			{
				do
				{
					SP -= 4;
					SET_ABS_L_REG(((SP & 0xfc) >> 2), READ_W(SP));
					difference++;

				} while(difference != 0);
			}
		}

		//TODO: no 1!
		e132xs_ICount -= 1;
	}
	else if( SRC_IS_SR ) // Rd doesn't denote PC and Rs denotes SR
	{
		SET_DREG(0);
		SET_DREGF(0);
		SET_Z(1);
		SET_N(0);

		e132xs_ICount -= 2;
	}
	else // Rd doesn't denote PC and Rs doesn't denote SR
	{
		UINT64 tmp;

		SET_DREG(SREG);
		SET_DREGF(SREGF);

		tmp = COMBINE_U64_U32_U32(SREG, SREGF);
		SET_Z( tmp == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(SREG) );

		e132xs_ICount -= 2;
	}
}

void e132xs_divu(void)
{
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted the same register code in e132xs_divu instruction. PC = %08X\n", PC);
	}
	else
	{
		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR as source register in e132xs_divu instruction. PC = %08X\n", PC);
		}
		else
		{
			UINT64 dividend;

			dividend = COMBINE_U64_U32_U32(DREG, DREGF);

			if( SREG == 0 )
			{
				//Rd//Rdf -> undefined
				//Z -> undefined
				//N -> undefined
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_exception(addr);
			}
			else
			{
				UINT32 quotient, remainder;

				/* TODO: add quotient overflow */
				quotient = dividend / SREG;
				remainder = dividend % SREG;

				SET_DREG(remainder);
				SET_DREGF(quotient);

				SET_Z( quotient == 0 ? 1 : 0 );
				SET_N( SIGN_BIT(quotient) );
				SET_V(0);
			}
		}
	}

	e132xs_ICount -= 36;
}

void e132xs_divs(void)
{
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted the same register code in e132xs_divs instruction. PC = %08X\n", PC);
	}
	else
	{
		if( SRC_IS_PC || SRC_IS_SR )
		{
			logerror("Denoted PC or SR as source register in e132xs_divs instruction. PC = %08X\n", PC);
		}
		else
		{
			INT64 dividend;

			dividend = (INT64) COMBINE_64_32_32(DREG, DREGF);

			if( SREG == 0 || (DREG & 0x80000000) )
			{
				//Rd//Rdf -> undefined
				//Z -> undefined
				//N -> undefined
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_exception(addr);
			}
			else
			{
				INT32 quotient, remainder;

				/* TODO: add quotient overflow */
				quotient = dividend / ((INT32)(SREG));
				remainder = dividend % ((INT32)(SREG));

				SET_DREG(remainder);
				SET_DREGF(quotient);

				SET_Z( quotient == 0 ? 1 : 0 );
				SET_N( SIGN_BIT(quotient) );
				SET_V(0);
			}
		}
	}

	e132xs_ICount -= 36;
}

void e132xs_xm(void)
{
	UINT32 val, lim;
	UINT16 next_source;
	UINT8 x_code;

	instruction_length = 2;
	next_source = READ_OP(PC);
	PC += 2;

	x_code = X_CODE(next_source);

	if( E_BIT(next_source) )
	{
		UINT16 next_source_2;

		instruction_length = 3;
		next_source_2 = READ_OP(PC);
		PC += 2;

		lim = ((next_source & 0xfff) << 16) | next_source_2;
	}
	else
	{
		lim = next_source & 0xfff;
	}

	decode_registers();

	if( SRC_IS_SR || DST_IS_SR || DST_IS_PC )
	{
		logerror("Denoted PC or SR in e132xs_xm. PC = %08X\n", PC);
	}
	else
	{
		val = SREG;

		switch( x_code )
		{
			case 0:
			case 1:
			case 2:
			case 3:
				if( !SRC_IS_PC && (val > lim) )
				{
					UINT32 addr = get_trap_addr(RANGE_ERROR);
					execute_exception(addr);
				}
				else if( SRC_IS_PC && (val >= lim) )
				{
					UINT32 addr = get_trap_addr(RANGE_ERROR);
					execute_exception(addr);
				}
				else
				{
					val <<= x_code;
				}

				break;

			case 4:
			case 5:
			case 6:
			case 7:
				x_code -= 4;
				val <<= x_code;

				break;
		}

		SET_DREG(val);
	}

	e132xs_ICount -= 1;
}

void e132xs_mask(void)
{
	UINT32 const_val;

	const_val = get_const();

	decode_registers();

	DREG = SREG & const_val;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_sum(void)
{
	UINT32 const_val;
	UINT64 tmp;

	const_val = get_const();

	decode_registers();

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(SREG) + (UINT64)(const_val);
	CHECK_C(tmp);
	CHECK_VADD(SREG,const_val,tmp);

	DREG = SREG + const_val;

	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_sums(void)
{
	INT32 const_val, res;
	INT64 tmp;

	const_val = get_const();

	decode_registers();

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(SREG)) + (INT64)(const_val);
	CHECK_VADD(SREG,const_val,tmp);

//#if SETCARRYS
//	CHECK_C(tmp);
//#endif

	res = (INT32)(SREG) + const_val;

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	e132xs_ICount -= 1;

	if( GET_V && !SRC_IS_SR )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

void e132xs_cmp(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	if( DREG == SREG )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) DREG < (INT32) SREG )
		SET_N(1);
	else
		SET_N(0);

	tmp = (UINT64)(DREG) - (UINT64)(SREG);
	CHECK_VSUB(SREG,DREG,tmp);

	if( DREG < SREG )
		SET_C(1);
	else
		SET_C(0);

	e132xs_ICount -= 1;
}

void e132xs_mov(void)
{
	if( !GET_S && current_regs.dst >= 16 )
	{
		UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
		execute_exception(addr);
	}

	SET_DREG(SREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( SREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(SREG) );

	e132xs_ICount -= 1;
}


void e132xs_add(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(SREG) + (UINT64)(DREG);
	CHECK_C(tmp);
	CHECK_VADD(SREG,DREG,tmp);

	DREG = SREG + DREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_adds(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(SREG)) + (INT64)((INT32)(DREG));

	CHECK_VADD(SREG,DREG,tmp);

//#if SETCARRYS
//	CHECK_C(tmp);
//#endif

	res = (INT32)(SREG) + (INT32)(DREG);

	SET_DREG(res);
	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

void e132xs_cmpb(void)
{
	SET_Z( (DREG & SREG) == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_andn(void)
{
	DREG = DREG & ~SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_or(void)
{
	DREG = DREG | SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_xor(void)
{
	DREG = DREG ^ SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_subc(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
	{
		tmp = (UINT64)(DREG) - (UINT64)(GET_C);
		CHECK_VSUB(GET_C,DREG,tmp);
	}
	else
	{
		tmp = (UINT64)(DREG) - ((UINT64)(SREG) + (UINT64)(GET_C));
		//CHECK!
		CHECK_VSUB(SREG + GET_C,DREG,tmp);
	}

	CHECK_C(tmp);

	if( SRC_IS_SR )
	{
		DREG = DREG - GET_C;
	}
	else
	{
		DREG = DREG - (SREG + GET_C);
	}

	SET_DREG(DREG);

	SET_Z( GET_Z & (DREG == 0 ? 1 : 0) );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_not(void)
{
	SET_DREG(~SREG);
	SET_Z( ~SREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_sub(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (UINT64)(DREG) - (UINT64)(SREG);
	CHECK_C(tmp);
	CHECK_VSUB(SREG,DREG,tmp);

	DREG = DREG - SREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_subs(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = (INT64)((INT32)(DREG)) - (INT64)((INT32)(SREG));

//#ifdef SETCARRYS
//	CHECK_C(tmp);
//#endif

	CHECK_VSUB(SREG,DREG,tmp);

	res = (INT32)(DREG) - (INT32)(SREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

void e132xs_addc(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
	{
		tmp = (UINT64)(DREG) + (UINT64)(GET_C);
		CHECK_VADD(DREG,GET_C,tmp);
	}
	else
	{
		tmp = (UINT64)(SREG) + (UINT64)(DREG) + (UINT64)(GET_C);

		//CHECK!
		//CHECK_VADD1: V = (DREG == 0x7FFF) && (C == 1);
		//OVERFLOW = CHECK_VADD1(DREG, C, DREG+C) | CHECK_VADD(SREG, DREG+C, SREG+DREG+C)
		/* check if DREG + GET_C overflows */
//		if( (DREG == 0x7FFFFFFF) && (GET_C == 1) )
//			SET_V(1);
//		else
//			CHECK_VADD(SREG,DREG + GET_C,tmp);

		CHECK_VADD3(SREG,DREG,GET_C,tmp);
	}

	CHECK_C(tmp);

	if( SRC_IS_SR )
		DREG = DREG + GET_C;
	else
		DREG = SREG + DREG + GET_C;

	SET_DREG(DREG);
	SET_Z( GET_Z & (DREG == 0 ? 1 : 0) );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_and(void)
{
	DREG = DREG & SREG;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_neg(void)
{
	UINT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = -(UINT64)(SREG);
	CHECK_C(tmp);
	CHECK_VSUB(SREG,0,tmp);

	DREG = -SREG;

	SET_DREG(DREG);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_negs(void)
{
	INT32 res;
	INT64 tmp;

	if( SRC_IS_SR )
		SREG = GET_C;

	tmp = -(INT64)((INT32)(SREG));
	CHECK_VSUB(SREG,0,tmp);

//#if SETCARRYS
//	CHECK_C(tmp);
//#endif

	res = -(INT32)(SREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );


	e132xs_ICount -= 1;

	if( GET_V && !SRC_IS_SR ) //trap doesn't occur when source is SR
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

void e132xs_cmpi(void)
{
	UINT32 imm;
	UINT64 tmp;

	imm = immediate_value();

	decode_registers();

	tmp = (UINT64)(DREG) - (UINT64)(imm);
	CHECK_VSUB(imm,DREG,tmp);

	if( DREG == imm )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) DREG < (INT32) imm )
		SET_N(1);
	else
		SET_N(0);

	if( DREG < imm )
		SET_C(1);
	else
		SET_C(0);

	e132xs_ICount -= 1;
}

void e132xs_movi(void)
{
	UINT32 imm;

	imm = immediate_value();

	if( !GET_S && current_regs.dst >= 16 )
	{
		UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
		execute_exception(addr);
	}

	SET_DREG(imm);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( imm == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(imm) );

#if MISSIONCRAFT_FLAGS
	SET_V(0); // or V undefined ?
#endif

	e132xs_ICount -= 1;
}

void e132xs_addi(void)
{
	UINT32 imm;
	UINT64 tmp;

	if( N_VALUE )
		imm = immediate_value();
	else
		imm = GET_C & ((GET_Z == 0 ? 1 : 0) | (DREG & 0x01));

	decode_registers();

	tmp = (UINT64)(imm) + (UINT64)(DREG);
	CHECK_C(tmp);
	CHECK_VADD(imm,DREG,tmp);

	DREG = imm + DREG;
	SET_DREG(DREG);

	if( DST_IS_PC )
		SET_M(0);

	SET_Z( DREG == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(DREG) );

	e132xs_ICount -= 1;
}

void e132xs_addsi(void)
{
	INT32 imm, res;
	INT64 tmp;

	if( N_VALUE )
		imm = immediate_value();
	else
		imm = GET_C & ((GET_Z == 0 ? 1 : 0) | (DREG & 0x01));

	decode_registers();

	tmp = (INT64)(imm) + (INT64)((INT32)(DREG));
	CHECK_VADD(imm,DREG,tmp);

//#if SETCARRYS
//	CHECK_C(tmp);
//#endif

	res = imm + (INT32)(DREG);

	SET_DREG(res);

	SET_Z( res == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(res) );

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_exception(addr);
	}
}

void e132xs_cmpbi(void)
{
	UINT32 imm;

	if( N_VALUE )
	{
		if( N_VALUE == 31 )
		{
			imm = 0x7fffffff; // bit 31 = 0, others = 1
		}
		else
		{
			imm = immediate_value();
			decode_registers();
		}

		SET_Z( (DREG & imm) == 0 ? 1 : 0 );
	}
	else
	{
		if( (DREG & 0xff000000) == 0 || (DREG & 0x00ff0000) == 0 ||
			(DREG & 0x0000ff00) == 0 || (DREG & 0x000000ff) == 0 )
			SET_Z(1);
		else
			SET_Z(0);
	}

	e132xs_ICount -= 1;
}

void e132xs_andni(void)
{
	UINT32 imm;

	if( N_VALUE == 31 )
		imm = 0x7fffffff; // bit 31 = 0, others = 1
	else
		imm = immediate_value();

	decode_registers();

	DREG = DREG & ~imm;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_ori(void)
{
	UINT32 imm;

	imm = immediate_value();

	decode_registers();

	DREG = DREG | imm;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_xori(void)
{
	UINT32 imm;

	imm = immediate_value();

	decode_registers();

	DREG = DREG ^ imm;

	SET_DREG(DREG);
	SET_Z( DREG == 0 ? 1 : 0 );

	e132xs_ICount -= 1;
}

void e132xs_shrdi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;

	high_order = DREG;
	low_order  = DREGF;

	val = COMBINE_U64_U32_U32(high_order, low_order);

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	high_order = HI32_32_64(val);
	low_order  = LO32_32_64(val);

	SET_DREG(high_order);
	SET_DREGF(low_order);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	e132xs_ICount -= 2;
}


void e132xs_shrd(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in e132xs_shrd. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		val = COMBINE_U64_U32_U32(high_order, low_order);

		if( n )
			SET_C((val >> (n - 1)) & 1);
		else
			SET_C(0);

		val >>= n;

		high_order = HI32_32_64(val);
		low_order  = LO32_32_64(val);

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	e132xs_ICount -= 2;
}

void e132xs_shr(void)
{
	UINT32 ret;
	UINT8 n;

	n = SREG & 0x1f;
	ret = DREG;

	if( n )
		SET_C((ret >> (n - 1)) & 1);
	else
		SET_C(0);

	ret >>= n;

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	e132xs_ICount -= 1;
}

void e132xs_sardi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 sign_bit;

	high_order = DREG;
	low_order  = DREGF;

	val = COMBINE_64_32_32(high_order, low_order);

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	sign_bit = val >> 63;
	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (U64(0x8000000000000000) >> i);
		}
	}

	high_order = val >> 32;
	low_order  = val & 0xffffffff;

	SET_DREG(high_order);
	SET_DREGF(low_order);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	e132xs_ICount -= 2;
}

void e132xs_sard(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	UINT8 n, sign_bit;

	n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in e132xs_sard. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		val = COMBINE_64_32_32(high_order, low_order);

		if( n )
			SET_C((val >> (n - 1)) & 1);
		else
			SET_C(0);

		sign_bit = val >> 63;

		val >>= n;

		if( sign_bit )
		{
			int i;
			for( i = 0; i < n; i++ )
			{
				val |= (U64(0x8000000000000000) >> i);
			}
		}

		high_order = val >> 32;
		low_order  = val & 0xffffffff;

		SET_DREG(high_order);
		SET_DREGF(low_order);
		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	e132xs_ICount -= 2;
}

void e132xs_sar(void)
{
	UINT32 ret;
	UINT8 n, sign_bit;

	n = SREG & 0x1f;
	ret = DREG;
	sign_bit = (ret & 0x80000000) >> 31;

	if( n )
		SET_C((ret >> (n - 1)) & 1);
	else
		SET_C(0);

	ret >>= n;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < n; i++ )
		{
			ret |= (0x80000000 >> i);
		}
	}

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	e132xs_ICount -= 1;
}

void e132xs_shldi(void)
{
	UINT32 low_order, high_order, tmp;
	UINT64 val, mask;

	high_order = DREG;
	low_order  = DREGF;

	val  = COMBINE_U64_U32_U32(high_order, low_order);
	mask = ((((UINT64)1) << (32 - N_VALUE)) - 1) ^ 0xffffffff;
	tmp  = high_order << N_VALUE;

	if( ((high_order & mask) && (!(tmp & 0x80000000))) ||
			(((high_order & mask) ^ mask) && (tmp & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	val <<= N_VALUE;

	high_order = HI32_32_64(val);
	low_order  = LO32_32_64(val);

	SET_DREG(high_order);
	SET_DREGF(low_order);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(high_order) );

	e132xs_ICount -= 2;
}

void e132xs_shld(void)
{
	UINT32 low_order, high_order, tmp, n;
	UINT64 val, mask;

	n = SREG & 0x1f;

	// result undefined if Ls denotes the same register as Ld or Ldf
	if( SAME_SRC_DST || SAME_SRC_DSTF )
	{
		logerror("Denoted same registers in e132xs_shld. PC = %08X\n", PC);
	}
	else
	{
		high_order = DREG;
		low_order  = DREGF;

		mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;

		val = COMBINE_U64_U32_U32(high_order, low_order);
		tmp = high_order << n;

		if( ((high_order & mask) && (!(tmp & 0x80000000))) ||
				(((high_order & mask) ^ mask) && (tmp & 0x80000000)) )
			SET_V(1);
		else
			SET_V(0);

		val <<= n;

		high_order = HI32_32_64(val);
		low_order  = LO32_32_64(val);

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( val == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	e132xs_ICount -= 2;
}

void e132xs_shl(void)
{
	UINT32 base, ret, n;
	UINT64 mask;

	n    = SREG & 0x1f;
	base = DREG;
	mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;
	ret  = base << n;

	if( ((base & mask) && (!(ret & 0x80000000))) ||
			(((base & mask) ^ mask) && (ret & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	SET_DREG(ret);
	SET_Z( ret == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(ret) );

	e132xs_ICount -= 1;
}

void reserved(void)
{
	logerror("Executed Reserved opcode. PC = %08X OP = %04X\n", PC, OP);
}

void e132xs_testlz(void)
{
	UINT8 zeros = 0;
	UINT32 code, mask;

	code = SREG;

	for( mask = 0x80000000; ; mask >>= 1 )
	{
		if( code & mask )
			break;
		else
			zeros++;

		if( zeros == 32 )
			break;
	}

	SET_DREG(zeros);

	e132xs_ICount -= 2;
}

void e132xs_rol(void)
{
	UINT32 val, base;
	UINT8 n;
	UINT64 mask;

	n = SREG & 0x1f;

	val = base = DREG;

	mask = ((((UINT64)1) << (32 - n)) - 1) ^ 0xffffffff;

	while( n > 0 )
	{
		val = (val << 1) | ((val & 0x80000000) >> 31);
		n--;
	}

#ifdef MISSIONCRAFT_FLAGS

	if( ((base & mask) && (!(val & 0x80000000))) ||
			(((base & mask) ^ mask) && (val & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

#endif

	SET_DREG(val);

	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	e132xs_ICount -= 1;
}

//TODO: add trap error
void e132xs_ldxx1(void)
{
	UINT32 load;
	UINT16 next_op;
	INT32 dis;

	instruction_length = 2;
	next_op = READ_OP(PC);
	PC += 2;

	dis = get_dis(next_op);

	decode_registers(); // re-decode because PC has changed

	if( DST_IS_SR )
	{
		switch( DD(next_op) )
		{
			case 0: // LDBS.A

				load = READ_B(dis);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);

				break;

			case 1: // LDBU.A

				load = READ_B(dis);
				SET_SREG(load);

				break;

			case 2:

				load = READ_HW(dis & ~1);

				if( dis & 1 ) // LDHS.A
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;
				}
				else          // LDHU.A
				{
					/* nothing more */
				}

				SET_SREG(load);

				break;

			case 3:

				if( (dis & 3) == 3 )      // LDD.IOA
				{
					load = IO_READ_W(dis & ~3);
					SET_SREG(load);

					load = IO_READ_W((dis & ~3) + 4);
					SET_SREGF(load);

					e132xs_ICount -= 1; // extra cycle
				}
				else if( (dis & 3) == 2 ) // LDW.IOA
				{
					load = IO_READ_W(dis & ~3);
					SET_SREG(load);
				}
				else if( (dis & 3) == 1 ) // LDD.A
				{
					load = READ_W(dis & ~1);
					SET_SREG(load);

					load = READ_W((dis & ~1) + 4);
					SET_SREGF(load);

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // LDW.A
				{
					load = READ_W(dis & ~1);
					SET_SREG(load);
				}

				break;
		}
	}
	else
	{
		switch( DD(next_op) )
		{
			case 0: // LDBS.D

				load = READ_B(DREG + dis);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);

				break;

			case 1: // LDBU.D

				load = READ_B(DREG + dis);
				SET_SREG(load);

				break;

			case 2:

				load = READ_HW(DREG + (dis & ~1));

				if( dis & 1 ) // LDHS.D
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;
				}
				else          // LDHU.D
				{
					/* nothing more */
				}

				SET_SREG(load);

				break;

			case 3:

				if( (dis & 3) == 3 )      // LDD.IOD
				{
					load = IO_READ_W(DREG + (dis & ~3));
					SET_SREG(load);

					load = IO_READ_W(DREG + (dis & ~3) + 4);
					SET_SREGF(load);

					e132xs_ICount -= 1; // extra cycle
				}
				else if( (dis & 3) == 2 ) // LDW.IOD
				{
					load = IO_READ_W(DREG + (dis & ~3));
					SET_SREG(load);
				}
				else if( (dis & 3) == 1 ) // LDD.D
				{
					load = READ_W(DREG + (dis & ~1));
					SET_SREG(load);

					load = READ_W(DREG + (dis & ~1) + 4);
					SET_SREGF(load);

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // LDW.D
				{
					load = READ_W(DREG + (dis & ~1));
					SET_SREG(load);
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_ldxx2(void)
{
	UINT32 load;
	UINT16 next_op;
	INT32 dis;

	instruction_length = 2;
	next_op = READ_OP(PC);
	PC += 2;

	dis = get_dis(next_op);

	decode_registers(); // re-decode because PC has changed

	if( DST_IS_PC || DST_IS_SR )
	{
		logerror("Denoted PC or SR in e132xs_ldxx2. PC = %08X\n", PC);
	}
	else
	{
		switch( DD(next_op) )
		{
			case 0: // LDBS.N

				load = READ_B(DREG);
				load |= (load & 0x80) ? 0xffffff00 : 0;
				SET_SREG(load);
				SET_DREG(DREG + dis);

				break;

			case 1: // LDBU.N

				load = READ_B(DREG);
				SET_SREG(load);
				SET_DREG(DREG + dis);

				break;

			case 2:

				load = READ_HW(DREG);

				if( dis & 1 ) // LDHS.N
				{
					load |= (load & 0x8000) ? 0xffff0000 : 0;
				}
				else          // LDHU.N
				{
					/* nothing more */
				}

				SET_SREG(load);
				SET_DREG(DREG + (dis & ~1));

				break;

			case 3:

				if( (dis & 3) == 3 )      // LDW.S
				{
					osd_die("Executed LDW.S instruction. PC = %08X\n", PC);
					/* TODO */
					SET_DREG(DREG + (dis & ~3));

					e132xs_ICount -= 2; // extra cycles
				}
				else if( (dis & 3) == 2 ) // Reserved
				{
					logerror("Executed Reserved instruction in e132xs_ldxx2. PC = %08X\n", PC);
				}
				else if( (dis & 3) == 1 ) // LDD.N
				{
					load = READ_W(DREG);
					SET_SREG(load);

					load = READ_W(DREG + 4);
					SET_SREGF(load);

					SET_DREG(DREG + (dis & ~1));

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // LDW.N
				{
					load = READ_W(DREG);
					SET_SREG(load);
					SET_DREG(DREG + (dis & ~1));
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

//TODO: add trap error
void e132xs_stxx1(void)
{
	UINT16 next_op;
	INT32 dis;

	instruction_length = 2;
	next_op = READ_OP(PC);
	PC += 2;

	dis = get_dis(next_op);

	decode_registers(); // re-decode because PC has changed

	if( SRC_IS_SR )
		SREG = SREGF = 0;

	if( DST_IS_SR )
	{
		switch( DD(next_op) )
		{
			case 0: // STBS.A

				/* TODO: missing trap on range error */
				WRITE_B(dis, SREG & 0xff);

				break;

			case 1: // STBU.A

				WRITE_B(dis, SREG & 0xff);

				break;

			case 2:

				WRITE_HW(dis & ~1, SREG & 0xffff);

				if( dis & 1 ) // STHS.A
				{
					/* TODO: missing trap on range error */
				}
				else          // STHU.A
				{
					/* nothing more */
				}

				break;

			case 3:

				if( (dis & 3) == 3 )      // STD.IOA
				{
					IO_WRITE_W(dis & ~3, SREG);
					IO_WRITE_W((dis & ~3) + 4, SREGF);

					e132xs_ICount -= 1; // extra cycle
				}
				else if( (dis & 3) == 2 ) // STW.IOA
				{
					IO_WRITE_W(dis & ~3, SREG);
				}
				else if( (dis & 3) == 1 ) // STD.A
				{
					WRITE_W(dis & ~1, SREG);
					WRITE_W((dis & ~1) + 4, SREGF);

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // STW.A
				{
					WRITE_W(dis & ~1, SREG);
				}

				break;
		}
	}
	else
	{
		switch( DD(next_op) )
		{
			case 0: // STBS.D

				/* TODO: missing trap on range error */
				WRITE_B(DREG + dis, SREG & 0xff);

				break;

			case 1: // STBU.D

				WRITE_B(DREG + dis, SREG & 0xff);

				break;

			case 2:

				WRITE_HW(DREG + (dis & ~1), SREG & 0xffff);

				if( dis & 1 ) // STHS.D
				{
					/* TODO: missing trap on range error */
				}
				else          // STHU.D
				{
					/* nothing more */
				}

				break;

			case 3:

				if( (dis & 3) == 3 )      // STD.IOD
				{
					IO_WRITE_W(DREG + (dis & ~3), SREG);
					IO_WRITE_W(DREG + (dis & ~3) + 4, SREGF);

					e132xs_ICount -= 1; // extra cycle
				}
				else if( (dis & 3) == 2 ) // STW.IOD
				{
					IO_WRITE_W(DREG + (dis & ~3), SREG);
				}
				else if( (dis & 3) == 1 ) // STD.D
				{
					WRITE_W(DREG + (dis & ~1), SREG);
					WRITE_W(DREG + (dis & ~1) + 4, SREGF);

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // STW.D
				{
					WRITE_W(DREG + (dis & ~1), SREG);
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_stxx2(void)
{
	UINT16 next_op;
	INT32 dis;

	instruction_length = 2;
	next_op = READ_OP(PC);
	PC += 2;

	dis = get_dis(next_op);

	decode_registers(); // re-decode because PC has changed

	if( SRC_IS_SR )
		SREG = SREGF = 0;

	if( DST_IS_PC || DST_IS_SR )
	{
		logerror("Denoted PC or SR in e132xs_stxx2. PC = %08X\n", PC);
	}
	else
	{
		switch( DD( next_op ) )
		{
			case 0: // STBS.N

				/* TODO: missing trap on range error */
				WRITE_B(DREG, SREG & 0xff);
				SET_DREG(DREG + dis);

				break;

			case 1: // STBU.N

				WRITE_B(DREG, SREG & 0xff);
				SET_DREG(DREG + dis);

				break;

			case 2:

				WRITE_HW(DREG, SREG & 0xffff);
				SET_DREG(DREG + (dis & ~1));

				if( dis & 1 ) // STHS.N
				{
					/* TODO: missing trap on range error */
				}
				else          // STHU.N
				{
					/* nothing more */
				}

				break;

			case 3:

				if( (dis & 3) == 3 )      // STW.S
				{
					osd_die("Executed STW.S instruction. PC = %08X\n", PC);
					/* TODO */
					SET_DREG(DREG + (dis & ~3));

					e132xs_ICount -= 2; // extra cycles

				}
				else if( (dis & 3) == 2 ) // Reserved
				{
					logerror("Executed Reserved instruction in e132xs_stxx2. PC = %08X\n", PC);
				}
				else if( (dis & 3) == 1 ) // STD.N
				{
					WRITE_W(DREG, SREG);
					SET_DREG(DREG + (dis & ~1));

					if( SAME_SRCF_DST )
						WRITE_W(DREG + 4, SREGF + (dis & ~1));  // because DREG == SREGF and DREG has been incremented
					else
						WRITE_W(DREG + 4, SREGF);

					e132xs_ICount -= 1; // extra cycle
				}
				else                      // STW.N
				{
					WRITE_W(DREG, SREG);
					SET_DREG(DREG + (dis & ~1));
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_shri(void)
{
	UINT32 val;

	val = DREG;

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	SET_DREG(val);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	e132xs_ICount -= 1;
}

void e132xs_sari(void)
{
	UINT32 val;
	UINT8 sign_bit;

	val = DREG;
	sign_bit = (val & 0x80000000) >> 31;

	if( N_VALUE )
		SET_C((val >> (N_VALUE - 1)) & 1);
	else
		SET_C(0);

	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (0x80000000 >> i);
		}
	}

	SET_DREG(val);
	SET_Z( val == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val) );

	e132xs_ICount -= 1;
}

void e132xs_shli(void)
{
	UINT32 val, val2;
	UINT64 mask;

	val  = DREG;
	mask = ((((UINT64)1) << (32 - N_VALUE)) - 1) ^ 0xffffffff;
	val2 = val << N_VALUE;

	if( ((val & mask) && (!(val2 & 0x80000000))) ||
			(((val & mask) ^ mask) && (val2 & 0x80000000)) )
		SET_V(1);
	else
		SET_V(0);

	SET_DREG(val2);
	SET_Z( val2 == 0 ? 1 : 0 );
	SET_N( SIGN_BIT(val2) );

	e132xs_ICount -= 1;
}

void e132xs_mulu(void)
{
	UINT32 low_order, high_order;
	UINT64 double_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in e132xs_mulu instruction. PC = %08X\n", PC);
	}
	else
	{
		double_word = SREG * DREG;

		low_order = double_word & 0xffffffff;
		high_order = double_word >> 32;

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( double_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	if(SREG <= 0xffff && DREG <= 0xffff)
		e132xs_ICount -= 4;
	else
		e132xs_ICount -= 6;
}

void e132xs_muls(void)
{
	UINT32 low_order, high_order;
	INT64 double_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in e132xs_muls instruction. PC = %08X\n", PC);
	}
	else
	{
		double_word = (INT32)(SREG) * (INT32)(DREG);
		low_order = double_word & 0xffffffff;
		high_order = double_word >> 32;

		SET_DREG(high_order);
		SET_DREGF(low_order);

		SET_Z( double_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(high_order) );
	}

	if((SREG >= 0xffff8000 && SREG <= 0x7fff) && (DREG >= 0xffff8000 && DREG <= 0x7fff))
		e132xs_ICount -= 4;
	else
		e132xs_ICount -= 6;
}

void e132xs_set(void)
{
	int n = N_VALUE;

	if( DST_IS_PC )
	{
		logerror("Denoted PC in e132xs_set. PC = %08X\n", PC);
	}
	else if( DST_IS_SR )
	{
		//TODO: add fetch opcode when there's the pipeline

		//TODO: no 1!
		e132xs_ICount -= 1;
	}
	else
	{
		switch( n )
		{
			// SETADR
			case 0:
			{
				UINT32 val;
				val =  (SP & 0xfffffe00) | (GET_FP << 2);

				//plus carry into bit 9
				val += (( (SP & 0x100) && (SIGN_BIT(SR) == 0) ) ? 1 : 0);

				SET_DREG(val);

				break;
			}
			// Reserved
			case 1:
			case 16:
			case 17:
			case 19:
				logerror("Used reserved N value (%d) in e132xs_set. PC = %08X\n", n, PC);
				break;

			// SETxx
			case 2:
				SET_DREG(1);
				break;

			case 3:
				SET_DREG(0);
				break;

			case 4:
				if( GET_N || GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 5:
				if( !GET_N && !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 6:
				if( GET_N )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 7:
				if( !GET_N )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 8:
				if( GET_C || GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 9:
				if( !GET_C && !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 10:
				if( GET_C )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 11:
				if( !GET_C )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 12:
				if( GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 13:
				if( !GET_Z )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 14:
				if( GET_V )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 15:
				if( !GET_V )
				{
					SET_DREG(1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 18:
				SET_DREG(-1);
				break;

			case 20:
				if( GET_N || GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 21:
				if( !GET_N && !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 22:
				if( GET_N )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 23:
				if( !GET_N )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 24:
				if( GET_C || GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 25:
				if( !GET_C && !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 26:
				if( GET_C )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 27:
				if( !GET_C )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 28:
				if( GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 29:
				if( !GET_Z )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 30:
				if( GET_V )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;

			case 31:
				if( !GET_V )
				{
					SET_DREG(-1);
				}
				else
				{
					SET_DREG(0);
				}

				break;
		}

		e132xs_ICount -= 1;
	}
}

void e132xs_mul(void)
{
	UINT32 single_word;

	// PC or SR aren't denoted, else result is undefined
	if( SRC_IS_PC || SRC_IS_SR || DST_IS_PC || DST_IS_SR  )
	{
		logerror("Denoted PC or SR in e132xs_mul instruction. PC = %08X\n", PC);
	}
	else
	{
		single_word = (SREG * DREG) & 0xffffffff; // only the low-order word is taken

		SET_DREG(single_word);

		SET_Z( single_word == 0 ? 1 : 0 );
		SET_N( SIGN_BIT(single_word) );
	}

	if((SREG >= 0xffff8000 && SREG <= 0x7fff) && (DREG >= 0xffff8000 && DREG <= 0x7fff))
		e132xs_ICount -= 3;
	else
		e132xs_ICount -= 5;
}



void e132xs_extend(void)
{
	//TODO: add locks, overflow error and other things
	UINT16 ext_opcode;
	UINT32 vals, vald;

	instruction_length = 2;
	ext_opcode = READ_OP(PC);
	PC += 2;

	decode_registers();

	vals = SREG;
	vald = DREG;

	switch( ext_opcode )
	{
		// signed or unsigned multiplication, single word product
		case EMUL:
		{
			UINT32 result;

			result = vals * vald;
			SET_G_REG(15, result);

			break;
		}
		// unsigned multiplication, double word product
		case EMULU:
		{
			UINT64 result;

			result = vals * vald;
			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiplication, double word product
		case EMULS:
		{
			INT64 result;

			result = (INT32)(vals) * (INT32)(vald);
			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiply/add, single word product sum
		case EMAC:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) + ((INT32)(vals) * (INT32)(vald));
			SET_G_REG(15, result);

			break;
		}
		// signed multiply/add, double word product sum
		case EMACD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + ((INT32)(vals) * (INT32)(vald));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed multiply/substract, single word product difference
		case EMSUB:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) - ((INT32)(vals) * (INT32)(vald));
			SET_G_REG(15, result);

			break;
		}
		// signed multiply/substract, double word product difference
		case EMSUBD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) - ((INT32)(vals) * (INT32)(vald));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// signed half-word multiply/add, single word product sum
		case EHMAC:
		{
			INT32 result;

			result = (INT32)GET_G_REG(15) + ((INT32)((vald & 0xffff0000) >> 16) * (INT32)((vals & 0xffff0000) >> 16)) + ((INT32)(vald & 0xffff) * (INT32)(vals & 0xffff));
			SET_G_REG(15, result);

			break;
		}
		// signed half-word multiply/add, double word product sum
		case EHMACD:
		{
			INT64 result;

			result = (INT64)COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + ((INT32)((vald & 0xffff0000) >> 16) * (INT32)((vals & 0xffff0000) >> 16)) + ((INT32)(vald & 0xffff) * (INT32)(vals & 0xffff));

			vals = result >> 32;
			vald = result & 0xffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		// half-word complex multiply
		case EHCMULD:
		{
			UINT32 result;

			result = (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		// half-word complex multiply/add
		case EHCMACD:
		{
			UINT32 result;

			result = GET_G_REG(14) + (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = GET_G_REG(15) + (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract
		// Ls is not used and should denote the same register as Ld
		case EHCSUMD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + GET_G_REG(15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - GET_G_REG(15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract with fixed point adjustment
		// Ls is not used and should denote the same register as Ld
		case EHCFFTD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		// half-word (complex) add/substract with fixed point adjustment and shift
		// Ls is not used and should denote the same register as Ld
		case EHCFFTSD:
		{
			UINT32 result;

			result = (((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) + (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(14, result);

			result = (((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) - (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(15, result);

			break;
		}
		default:
			logerror("Executed Illegal extended opcode (%x). PC = %08X\n", ext_opcode, PC);
			break;
	}

	e132xs_ICount -= 1; //TODO: with the latency it can change
}

void e132xs_do(void)
{
	osd_die("Executed e132xs_do instruction. PC = %08X\n", PC);
}

void e132xs_ldwr(void)
{
	SET_SREG(READ_W(DREG));

	e132xs_ICount -= 1;
}

void e132xs_lddr(void)
{
	SET_SREG(READ_W(DREG));
	SET_SREGF(READ_W(DREG + 4));

	e132xs_ICount -= 2;
}

void e132xs_ldwp(void)
{
	SET_SREG(READ_W(DREG));
	SET_DREG(DREG + 4);

	e132xs_ICount -= 1;
}

void e132xs_lddp(void)
{
	SET_SREG(READ_W(DREG));
	SET_SREGF(READ_W(DREG + 4));
	SET_DREG(DREG + 8);

	e132xs_ICount -= 2;
}

void e132xs_stwr(void)
{
	if( SRC_IS_SR )
		SREG = 0;

	WRITE_W(DREG, SREG);

	e132xs_ICount -= 1;
}

void e132xs_stdr(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	WRITE_W(DREG, SREG);
	WRITE_W(DREG + 4, SREGF);

	e132xs_ICount -= 2;
}

void e132xs_stwp(void)
{
	if( SRC_IS_SR )
		SREG = 0;

	WRITE_W(DREG, SREG);
	SET_DREG(DREG + 4);

	e132xs_ICount -= 1;
}

void e132xs_stdp(void)
{
	if( SRC_IS_SR )
		SREG = SREGF = 0;

	WRITE_W(DREG, SREG);
	SET_DREG(DREG + 8);

	if( SAME_SRCF_DST )
		WRITE_W(DREG + 4, SREGF + 8); // because DREG == SREGF and DREG has been incremented
	else
		WRITE_W(DREG + 4, SREGF);

	e132xs_ICount -= 2;
}

void e132xs_dbv(void)
{
	INT32 newPC = get_pcrel();
	if( GET_V )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnv(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_V )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbe(void) //or DBZ
{
	INT32 newPC = get_pcrel();
	if( GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbne(void) //or DBNZ
{
	INT32 newPC = get_pcrel();
	if( !GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbc(void) //or DBST
{
	INT32 newPC = get_pcrel();
	if( GET_C )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnc(void) //or DBHE
{
	INT32 newPC = get_pcrel();
	if( !GET_C )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbse(void)
{
	INT32 newPC = get_pcrel();
	if( GET_C || GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbht(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_C && !GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbn(void) //or DBLT
{
	INT32 newPC = get_pcrel();
	if( GET_N )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnn(void) //or DBGE
{
	INT32 newPC = get_pcrel();
	if( !GET_N )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dble(void)
{
	INT32 newPC = get_pcrel();
	if( GET_N || GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbgt(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_N && !GET_Z )
		execute_dbr(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_dbr(void)
{
	INT32 newPC = get_pcrel();
	execute_dbr(newPC);
}

void e132xs_frame(void)
{
	INT8 difference; // really it's 7 bits
	UINT8 realfp = GET_FP - SRC_CODE;

	SET_FP(realfp);
	SET_FL(DST_CODE);
	SET_M(0);

	difference = ((SP & 0x1fc) >> 2) + (64 - 10) - (realfp + GET_FL);

	/* convert to 8 bits */
	if(difference > 63)
		difference = (INT8)(difference|0x80);
	else if( difference < -64 )
		difference = difference & 0x7f;

	if( difference < 0 ) // else it's finished
	{
		UINT8 tmp_flag;

		tmp_flag = ( SP >= UB ? 1 : 0 );

		do
		{
			WRITE_W(SP, GET_ABS_L_REG((SP & 0xfc) >> 2));
			SP += 4;
			difference++;

		} while(difference != 0);

		if( tmp_flag )
		{
			UINT32 addr = get_trap_addr(FRAME_ERROR);
			execute_exception(addr);
		}
	}

	//TODO: no 1!
	e132xs_ICount -= 1;
}

void e132xs_call(void)
{
	INT32 const_val;

	const_val = get_const() & ~0x01;

	decode_registers();

	if( SRC_IS_SR )
		SREG = 0;

	if( !DST_CODE )
		current_regs.dst = 16;

	const_val += SREG;

	SET_ILC(instruction_length & 3);

	SET_DREG((PC & 0xfffffffe) | GET_S);
	SET_DREGF(SR);

	SET_FP(GET_FP + current_regs.dst);

	SET_FL(6); //default value for call
	SET_M(0);

	PPC = PC;
	PC = const_val;

	intblock = 2;

	//TODO: add interrupt locks, errors, ....

	//TODO: no 1!
	e132xs_ICount -= 1;
}

void e132xs_bv(void)
{
	INT32 newPC = get_pcrel();
	if( GET_V )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bnv(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_V )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_be(void) //or BZ
{
	INT32 newPC = get_pcrel();
	if( GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bne(void) //or BNZ
{
	INT32 newPC = get_pcrel();
	if( !GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bc(void) //or BST
{
	INT32 newPC = get_pcrel();
	if( GET_C )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bnc(void) //or BHE
{
	INT32 newPC = get_pcrel();
	if( !GET_C )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bse(void)
{
	INT32 newPC = get_pcrel();
	if( GET_C || GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bht(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_C && !GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bn(void) //or BLT
{
	INT32 newPC = get_pcrel();
	if( GET_N )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bnn(void) //or BGE
{
	INT32 newPC = get_pcrel();
	if( !GET_N )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_ble(void)
{
	INT32 newPC = get_pcrel();
	if( GET_N || GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_bgt(void)
{
	INT32 newPC = get_pcrel();
	if( !GET_N && !GET_Z )
		execute_br(newPC);
	else
		e132xs_ICount -= 1;
}

void e132xs_br(void)
{
	INT32 newPC = get_pcrel();
	execute_br(newPC);
}


void e132xs_trap(void)
{
	UINT8 code, trapno;
	UINT32 addr;

	trapno = (OP & 0xfc) >> 2;

	addr = get_trap_addr(trapno);
	code = ((OP & 0x300) >> 6) | (OP & 0x03);

	switch( code )
	{
		case TRAPLE:
			if( GET_N || GET_Z )
				execute_trap(addr);

			break;

		case TRAPGT:
			if( !GET_N && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPLT:
			if( GET_N )
				execute_trap(addr);

			break;

		case TRAPGE:
			if( !GET_N )
				execute_trap(addr);

			break;

		case TRAPSE:
			if( GET_C || GET_Z )
				execute_trap(addr);

			break;

		case TRAPHT:
			if( !GET_C && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPST:
			if( GET_C )
				execute_trap(addr);

			break;

		case TRAPHE:
			if( !GET_C )
				execute_trap(addr);

			break;

		case TRAPE:
			if( GET_Z )
				execute_trap(addr);

			break;

		case TRAPNE:
			if( !GET_Z )
				execute_trap(addr);

			break;

		case TRAPV:
			if( GET_V )
				execute_trap(addr);

			break;

		case TRAP:
			execute_trap(addr);

			break;
	}

	e132xs_ICount -= 1;
}


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void e132xs_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + E132XS_PC:			PC = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_SR:			SR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_FER:			FER = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_G3:			e132xs.global_regs[3] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G4:			e132xs.global_regs[4] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G5:			e132xs.global_regs[5] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G6:			e132xs.global_regs[6] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G7:			e132xs.global_regs[7] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G8:			e132xs.global_regs[8] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G9:			e132xs.global_regs[9] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G10:			e132xs.global_regs[10] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G11:			e132xs.global_regs[11] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G12:			e132xs.global_regs[12] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G13:			e132xs.global_regs[13] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G14:			e132xs.global_regs[14] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G15:			e132xs.global_regs[15] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G16:			e132xs.global_regs[16] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G17:			e132xs.global_regs[17] = info->i;		break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + E132XS_SP:			SP  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_UB:			UB  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_BCR:			BCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TPR:			TPR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TCR:			TCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_TR:			TR  = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_WCR:			WCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_ISR:			ISR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_FCR:			FCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_MCR:			MCR = info->i;							break;
		case CPUINFO_INT_REGISTER + E132XS_G28:			e132xs.global_regs[28] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G29:			e132xs.global_regs[29] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G30:			e132xs.global_regs[30] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_G31:			e132xs.global_regs[31] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_CL0:			e132xs.local_regs[(0 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL1:			e132xs.local_regs[(1 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL2:			e132xs.local_regs[(2 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL3:			e132xs.local_regs[(3 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL4:			e132xs.local_regs[(4 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL5:			e132xs.local_regs[(5 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL6:			e132xs.local_regs[(6 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL7:			e132xs.local_regs[(7 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL8:			e132xs.local_regs[(8 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL9:			e132xs.local_regs[(9 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL10:		e132xs.local_regs[(10 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL11:		e132xs.local_regs[(11 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL12:		e132xs.local_regs[(12 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL13:		e132xs.local_regs[(13 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL14:		e132xs.local_regs[(14 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_CL15:		e132xs.local_regs[(15 + GET_FP) % 64] = info->i; break;
		case CPUINFO_INT_REGISTER + E132XS_L0:			e132xs.local_regs[0] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L1:			e132xs.local_regs[1] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L2:			e132xs.local_regs[2] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L3:			e132xs.local_regs[3] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L4:			e132xs.local_regs[4] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L5:			e132xs.local_regs[5] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L6:			e132xs.local_regs[6] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L7:			e132xs.local_regs[7] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L8:			e132xs.local_regs[8] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L9:			e132xs.local_regs[9] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L10:			e132xs.local_regs[10] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L11:			e132xs.local_regs[11] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L12:			e132xs.local_regs[12] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L13:			e132xs.local_regs[13] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L14:			e132xs.local_regs[14] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L15:			e132xs.local_regs[15] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L16:			e132xs.local_regs[16] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L17:			e132xs.local_regs[17] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L18:			e132xs.local_regs[18] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L19:			e132xs.local_regs[19] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L20:			e132xs.local_regs[20] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L21:			e132xs.local_regs[21] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L22:			e132xs.local_regs[22] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L23:			e132xs.local_regs[23] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L24:			e132xs.local_regs[24] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L25:			e132xs.local_regs[25] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L26:			e132xs.local_regs[26] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L27:			e132xs.local_regs[27] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L28:			e132xs.local_regs[28] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L29:			e132xs.local_regs[29] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L30:			e132xs.local_regs[30] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L31:			e132xs.local_regs[31] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L32:			e132xs.local_regs[32] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L33:			e132xs.local_regs[33] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L34:			e132xs.local_regs[34] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L35:			e132xs.local_regs[35] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L36:			e132xs.local_regs[36] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L37:			e132xs.local_regs[37] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L38:			e132xs.local_regs[38] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L39:			e132xs.local_regs[39] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L40:			e132xs.local_regs[40] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L41:			e132xs.local_regs[41] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L42:			e132xs.local_regs[42] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L43:			e132xs.local_regs[43] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L44:			e132xs.local_regs[44] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L45:			e132xs.local_regs[45] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L46:			e132xs.local_regs[46] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L47:			e132xs.local_regs[47] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L48:			e132xs.local_regs[48] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L49:			e132xs.local_regs[49] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L50:			e132xs.local_regs[50] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L51:			e132xs.local_regs[51] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L52:			e132xs.local_regs[52] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L53:			e132xs.local_regs[53] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L54:			e132xs.local_regs[54] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L55:			e132xs.local_regs[55] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L56:			e132xs.local_regs[56] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L57:			e132xs.local_regs[57] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L58:			e132xs.local_regs[58] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L59:			e132xs.local_regs[59] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L60:			e132xs.local_regs[60] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L61:			e132xs.local_regs[61] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L62:			e132xs.local_regs[62] = info->i;		break;
		case CPUINFO_INT_REGISTER + E132XS_L63:			e132xs.local_regs[63] = info->i;		break;

		/* --- the following bits of info are set as pointers to info->i or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:					e132xs.irq_callback = info->irqcallback;	break;

		case CPUINFO_INT_INPUT_STATE + 0:				set_irq_line(0, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 1:				set_irq_line(1, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 2:				set_irq_line(2, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 3:				set_irq_line(3, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 4:				set_irq_line(4, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 5:				set_irq_line(5, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 6:				set_irq_line(6, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 7:				set_irq_line(7, info->i);					break;
	}
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

void e132xs_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(e132xs_regs);			break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 8;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 6;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 36;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 13;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:					/* not implemented */					break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = PPC;							break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + E132XS_PC:			info->i =  PC;							break;
		case CPUINFO_INT_REGISTER + E132XS_SR:			info->i =  SR;							break;
		case CPUINFO_INT_REGISTER + E132XS_FER:			info->i =  FER;							break;
		case CPUINFO_INT_REGISTER + E132XS_G3:			info->i =  e132xs.global_regs[3];		break;
		case CPUINFO_INT_REGISTER + E132XS_G4:			info->i =  e132xs.global_regs[4];		break;
		case CPUINFO_INT_REGISTER + E132XS_G5:			info->i =  e132xs.global_regs[5];		break;
		case CPUINFO_INT_REGISTER + E132XS_G6:			info->i =  e132xs.global_regs[6];		break;
		case CPUINFO_INT_REGISTER + E132XS_G7:			info->i =  e132xs.global_regs[7];		break;
		case CPUINFO_INT_REGISTER + E132XS_G8:			info->i =  e132xs.global_regs[8];		break;
		case CPUINFO_INT_REGISTER + E132XS_G9:			info->i =  e132xs.global_regs[9];		break;
		case CPUINFO_INT_REGISTER + E132XS_G10:			info->i =  e132xs.global_regs[10];		break;
		case CPUINFO_INT_REGISTER + E132XS_G11:			info->i =  e132xs.global_regs[11];		break;
		case CPUINFO_INT_REGISTER + E132XS_G12:			info->i =  e132xs.global_regs[12];		break;
		case CPUINFO_INT_REGISTER + E132XS_G13:			info->i =  e132xs.global_regs[13];		break;
		case CPUINFO_INT_REGISTER + E132XS_G14:			info->i =  e132xs.global_regs[14];		break;
		case CPUINFO_INT_REGISTER + E132XS_G15:			info->i =  e132xs.global_regs[15];		break;
		case CPUINFO_INT_REGISTER + E132XS_G16:			info->i =  e132xs.global_regs[16];		break;
		case CPUINFO_INT_REGISTER + E132XS_G17:			info->i =  e132xs.global_regs[17];		break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + E132XS_SP:			info->i =  SP;							break;
		case CPUINFO_INT_REGISTER + E132XS_UB:			info->i =  UB;							break;
		case CPUINFO_INT_REGISTER + E132XS_BCR:			info->i =  BCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TPR:			info->i =  TPR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TCR:			info->i =  TCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_TR:			info->i =  TR;							break;
		case CPUINFO_INT_REGISTER + E132XS_WCR:			info->i =  WCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_ISR:			info->i =  ISR;							break;
		case CPUINFO_INT_REGISTER + E132XS_FCR:			info->i =  FCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_MCR:			info->i =  MCR;							break;
		case CPUINFO_INT_REGISTER + E132XS_G28:			info->i =  e132xs.global_regs[28];		break;
		case CPUINFO_INT_REGISTER + E132XS_G29:			info->i =  e132xs.global_regs[29];		break;
		case CPUINFO_INT_REGISTER + E132XS_G30:			info->i =  e132xs.global_regs[30];		break;
		case CPUINFO_INT_REGISTER + E132XS_G31:			info->i =  e132xs.global_regs[31];		break;
		case CPUINFO_INT_REGISTER + E132XS_CL0:			info->i =  e132xs.local_regs[(0 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL1:			info->i =  e132xs.local_regs[(1 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL2:			info->i =  e132xs.local_regs[(2 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL3:			info->i =  e132xs.local_regs[(3 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL4:			info->i =  e132xs.local_regs[(4 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL5:			info->i =  e132xs.local_regs[(5 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL6:			info->i =  e132xs.local_regs[(6 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL7:			info->i =  e132xs.local_regs[(7 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL8:			info->i =  e132xs.local_regs[(8 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL9:			info->i =  e132xs.local_regs[(9 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL10:		info->i =  e132xs.local_regs[(10 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL11:		info->i =  e132xs.local_regs[(11 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL12:		info->i =  e132xs.local_regs[(12 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL13:		info->i =  e132xs.local_regs[(13 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL14:		info->i =  e132xs.local_regs[(14 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_CL15:		info->i =  e132xs.local_regs[(15 + GET_FP) % 64]; break;
		case CPUINFO_INT_REGISTER + E132XS_L0:			info->i =  e132xs.local_regs[0];		break;
		case CPUINFO_INT_REGISTER + E132XS_L1:			info->i =  e132xs.local_regs[1];		break;
		case CPUINFO_INT_REGISTER + E132XS_L2:			info->i =  e132xs.local_regs[2];		break;
		case CPUINFO_INT_REGISTER + E132XS_L3:			info->i =  e132xs.local_regs[3];		break;
		case CPUINFO_INT_REGISTER + E132XS_L4:			info->i =  e132xs.local_regs[4];		break;
		case CPUINFO_INT_REGISTER + E132XS_L5:			info->i =  e132xs.local_regs[5];		break;
		case CPUINFO_INT_REGISTER + E132XS_L6:			info->i =  e132xs.local_regs[6];		break;
		case CPUINFO_INT_REGISTER + E132XS_L7:			info->i =  e132xs.local_regs[7];		break;
		case CPUINFO_INT_REGISTER + E132XS_L8:			info->i =  e132xs.local_regs[8];		break;
		case CPUINFO_INT_REGISTER + E132XS_L9:			info->i =  e132xs.local_regs[9];		break;
		case CPUINFO_INT_REGISTER + E132XS_L10:			info->i =  e132xs.local_regs[10];		break;
		case CPUINFO_INT_REGISTER + E132XS_L11:			info->i =  e132xs.local_regs[11];		break;
		case CPUINFO_INT_REGISTER + E132XS_L12:			info->i =  e132xs.local_regs[12];		break;
		case CPUINFO_INT_REGISTER + E132XS_L13:			info->i =  e132xs.local_regs[13];		break;
		case CPUINFO_INT_REGISTER + E132XS_L14:			info->i =  e132xs.local_regs[14];		break;
		case CPUINFO_INT_REGISTER + E132XS_L15:			info->i =  e132xs.local_regs[15];		break;
		case CPUINFO_INT_REGISTER + E132XS_L16:			info->i =  e132xs.local_regs[16];		break;
		case CPUINFO_INT_REGISTER + E132XS_L17:			info->i =  e132xs.local_regs[17];		break;
		case CPUINFO_INT_REGISTER + E132XS_L18:			info->i =  e132xs.local_regs[18];		break;
		case CPUINFO_INT_REGISTER + E132XS_L19:			info->i =  e132xs.local_regs[19];		break;
		case CPUINFO_INT_REGISTER + E132XS_L20:			info->i =  e132xs.local_regs[20];		break;
		case CPUINFO_INT_REGISTER + E132XS_L21:			info->i =  e132xs.local_regs[21];		break;
		case CPUINFO_INT_REGISTER + E132XS_L22:			info->i =  e132xs.local_regs[22];		break;
		case CPUINFO_INT_REGISTER + E132XS_L23:			info->i =  e132xs.local_regs[23];		break;
		case CPUINFO_INT_REGISTER + E132XS_L24:			info->i =  e132xs.local_regs[24];		break;
		case CPUINFO_INT_REGISTER + E132XS_L25:			info->i =  e132xs.local_regs[25];		break;
		case CPUINFO_INT_REGISTER + E132XS_L26:			info->i =  e132xs.local_regs[26];		break;
		case CPUINFO_INT_REGISTER + E132XS_L27:			info->i =  e132xs.local_regs[27];		break;
		case CPUINFO_INT_REGISTER + E132XS_L28:			info->i =  e132xs.local_regs[28];		break;
		case CPUINFO_INT_REGISTER + E132XS_L29:			info->i =  e132xs.local_regs[29];		break;
		case CPUINFO_INT_REGISTER + E132XS_L30:			info->i =  e132xs.local_regs[30];		break;
		case CPUINFO_INT_REGISTER + E132XS_L31:			info->i =  e132xs.local_regs[31];		break;
		case CPUINFO_INT_REGISTER + E132XS_L32:			info->i =  e132xs.local_regs[32];		break;
		case CPUINFO_INT_REGISTER + E132XS_L33:			info->i =  e132xs.local_regs[33];		break;
		case CPUINFO_INT_REGISTER + E132XS_L34:			info->i =  e132xs.local_regs[34];		break;
		case CPUINFO_INT_REGISTER + E132XS_L35:			info->i =  e132xs.local_regs[35];		break;
		case CPUINFO_INT_REGISTER + E132XS_L36:			info->i =  e132xs.local_regs[36];		break;
		case CPUINFO_INT_REGISTER + E132XS_L37:			info->i =  e132xs.local_regs[37];		break;
		case CPUINFO_INT_REGISTER + E132XS_L38:			info->i =  e132xs.local_regs[38];		break;
		case CPUINFO_INT_REGISTER + E132XS_L39:			info->i =  e132xs.local_regs[39];		break;
		case CPUINFO_INT_REGISTER + E132XS_L40:			info->i =  e132xs.local_regs[40];		break;
		case CPUINFO_INT_REGISTER + E132XS_L41:			info->i =  e132xs.local_regs[41];		break;
		case CPUINFO_INT_REGISTER + E132XS_L42:			info->i =  e132xs.local_regs[42];		break;
		case CPUINFO_INT_REGISTER + E132XS_L43:			info->i =  e132xs.local_regs[43];		break;
		case CPUINFO_INT_REGISTER + E132XS_L44:			info->i =  e132xs.local_regs[44];		break;
		case CPUINFO_INT_REGISTER + E132XS_L45:			info->i =  e132xs.local_regs[45];		break;
		case CPUINFO_INT_REGISTER + E132XS_L46:			info->i =  e132xs.local_regs[46];		break;
		case CPUINFO_INT_REGISTER + E132XS_L47:			info->i =  e132xs.local_regs[47];		break;
		case CPUINFO_INT_REGISTER + E132XS_L48:			info->i =  e132xs.local_regs[48];		break;
		case CPUINFO_INT_REGISTER + E132XS_L49:			info->i =  e132xs.local_regs[49];		break;
		case CPUINFO_INT_REGISTER + E132XS_L50:			info->i =  e132xs.local_regs[50];		break;
		case CPUINFO_INT_REGISTER + E132XS_L51:			info->i =  e132xs.local_regs[51];		break;
		case CPUINFO_INT_REGISTER + E132XS_L52:			info->i =  e132xs.local_regs[52];		break;
		case CPUINFO_INT_REGISTER + E132XS_L53:			info->i =  e132xs.local_regs[53];		break;
		case CPUINFO_INT_REGISTER + E132XS_L54:			info->i =  e132xs.local_regs[54];		break;
		case CPUINFO_INT_REGISTER + E132XS_L55:			info->i =  e132xs.local_regs[55];		break;
		case CPUINFO_INT_REGISTER + E132XS_L56:			info->i =  e132xs.local_regs[56];		break;
		case CPUINFO_INT_REGISTER + E132XS_L57:			info->i =  e132xs.local_regs[57];		break;
		case CPUINFO_INT_REGISTER + E132XS_L58:			info->i =  e132xs.local_regs[58];		break;
		case CPUINFO_INT_REGISTER + E132XS_L59:			info->i =  e132xs.local_regs[59];		break;
		case CPUINFO_INT_REGISTER + E132XS_L60:			info->i =  e132xs.local_regs[60];		break;
		case CPUINFO_INT_REGISTER + E132XS_L61:			info->i =  e132xs.local_regs[61];		break;
		case CPUINFO_INT_REGISTER + E132XS_L62:			info->i =  e132xs.local_regs[62];		break;
		case CPUINFO_INT_REGISTER + E132XS_L63:			info->i =  e132xs.local_regs[63];		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = e132xs_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = e132xs_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = e132xs_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = e132xs_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = e132xs_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = e132xs_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = e132xs_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = e132xs_dasm;		break;
		case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = e132xs.irq_callback;				break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &e132xs_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = e132xs_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = e132xs_win_layout;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "E1-32XS"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Hyperstone E1-32XS"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.1"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright Pierpaolo Prazzoli and Ryan Holtz"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c FTE:%X FRM:%X ILC:%d FL:%d FP:%d",
				GET_S ? 'S':'.',
				GET_P ? 'P':'.',
				GET_T ? 'T':'.',
				GET_L ? 'L':'.',
				GET_I ? 'I':'.',
				e132xs.global_regs[1] & 0x00040 ? '?':'.',
				GET_H ? 'H':'.',
				GET_M ? 'M':'.',
				GET_V ? 'V':'.',
				GET_N ? 'N':'.',
				GET_Z ? 'Z':'.',
				GET_C ? 'C':'.',
				GET_FTE,
				GET_FRM,
				GET_ILC,
				GET_FL,
				GET_FP);
			break;

		case CPUINFO_STR_REGISTER + E132XS_PC:  		sprintf(info->s = cpuintrf_temp_str(), "PC  :%08X", e132xs.global_regs[0]); break;
		case CPUINFO_STR_REGISTER + E132XS_SR:  		sprintf(info->s = cpuintrf_temp_str(), "SR  :%08X", e132xs.global_regs[1]); break;
		case CPUINFO_STR_REGISTER + E132XS_FER: 		sprintf(info->s = cpuintrf_temp_str(), "FER :%08X", e132xs.global_regs[2]); break;
		case CPUINFO_STR_REGISTER + E132XS_G3:  		sprintf(info->s = cpuintrf_temp_str(), "G3  :%08X", e132xs.global_regs[3]); break;
		case CPUINFO_STR_REGISTER + E132XS_G4:  		sprintf(info->s = cpuintrf_temp_str(), "G4  :%08X", e132xs.global_regs[4]); break;
		case CPUINFO_STR_REGISTER + E132XS_G5:  		sprintf(info->s = cpuintrf_temp_str(), "G5  :%08X", e132xs.global_regs[5]); break;
		case CPUINFO_STR_REGISTER + E132XS_G6:  		sprintf(info->s = cpuintrf_temp_str(), "G6  :%08X", e132xs.global_regs[6]); break;
		case CPUINFO_STR_REGISTER + E132XS_G7:  		sprintf(info->s = cpuintrf_temp_str(), "G7  :%08X", e132xs.global_regs[7]); break;
		case CPUINFO_STR_REGISTER + E132XS_G8:  		sprintf(info->s = cpuintrf_temp_str(), "G8  :%08X", e132xs.global_regs[8]); break;
		case CPUINFO_STR_REGISTER + E132XS_G9:  		sprintf(info->s = cpuintrf_temp_str(), "G9  :%08X", e132xs.global_regs[9]); break;
		case CPUINFO_STR_REGISTER + E132XS_G10: 		sprintf(info->s = cpuintrf_temp_str(), "G10 :%08X", e132xs.global_regs[10]); break;
		case CPUINFO_STR_REGISTER + E132XS_G11: 		sprintf(info->s = cpuintrf_temp_str(), "G11 :%08X", e132xs.global_regs[11]); break;
		case CPUINFO_STR_REGISTER + E132XS_G12: 		sprintf(info->s = cpuintrf_temp_str(), "G12 :%08X", e132xs.global_regs[12]); break;
		case CPUINFO_STR_REGISTER + E132XS_G13: 		sprintf(info->s = cpuintrf_temp_str(), "G13 :%08X", e132xs.global_regs[13]); break;
		case CPUINFO_STR_REGISTER + E132XS_G14: 		sprintf(info->s = cpuintrf_temp_str(), "G14 :%08X", e132xs.global_regs[14]); break;
		case CPUINFO_STR_REGISTER + E132XS_G15: 		sprintf(info->s = cpuintrf_temp_str(), "G15 :%08X", e132xs.global_regs[15]); break;
		case CPUINFO_STR_REGISTER + E132XS_G16: 		sprintf(info->s = cpuintrf_temp_str(), "G16 :%08X", e132xs.global_regs[16]); break;
		case CPUINFO_STR_REGISTER + E132XS_G17: 		sprintf(info->s = cpuintrf_temp_str(), "G17 :%08X", e132xs.global_regs[17]); break;
		case CPUINFO_STR_REGISTER + E132XS_SP:  		sprintf(info->s = cpuintrf_temp_str(), "SP  :%08X", e132xs.global_regs[18]); break;
		case CPUINFO_STR_REGISTER + E132XS_UB:  		sprintf(info->s = cpuintrf_temp_str(), "UB  :%08X", e132xs.global_regs[19]); break;
		case CPUINFO_STR_REGISTER + E132XS_BCR: 		sprintf(info->s = cpuintrf_temp_str(), "BCR :%08X", e132xs.global_regs[20]); break;
		case CPUINFO_STR_REGISTER + E132XS_TPR: 		sprintf(info->s = cpuintrf_temp_str(), "TPR :%08X", e132xs.global_regs[21]); break;
		case CPUINFO_STR_REGISTER + E132XS_TCR: 		sprintf(info->s = cpuintrf_temp_str(), "TCR :%08X", e132xs.global_regs[22]); break;
		case CPUINFO_STR_REGISTER + E132XS_TR:  		sprintf(info->s = cpuintrf_temp_str(), "TR  :%08X", e132xs.global_regs[23]); break;
		case CPUINFO_STR_REGISTER + E132XS_WCR: 		sprintf(info->s = cpuintrf_temp_str(), "WCR :%08X", e132xs.global_regs[24]); break;
		case CPUINFO_STR_REGISTER + E132XS_ISR: 		sprintf(info->s = cpuintrf_temp_str(), "ISR :%08X", e132xs.global_regs[25]); break;
		case CPUINFO_STR_REGISTER + E132XS_FCR: 		sprintf(info->s = cpuintrf_temp_str(), "FCR :%08X", e132xs.global_regs[26]); break;
		case CPUINFO_STR_REGISTER + E132XS_MCR: 		sprintf(info->s = cpuintrf_temp_str(), "MCR :%08X", e132xs.global_regs[27]); break;
		case CPUINFO_STR_REGISTER + E132XS_G28: 		sprintf(info->s = cpuintrf_temp_str(), "G28 :%08X", e132xs.global_regs[28]); break;
		case CPUINFO_STR_REGISTER + E132XS_G29: 		sprintf(info->s = cpuintrf_temp_str(), "G29 :%08X", e132xs.global_regs[29]); break;
		case CPUINFO_STR_REGISTER + E132XS_G30: 		sprintf(info->s = cpuintrf_temp_str(), "G30 :%08X", e132xs.global_regs[30]); break;
		case CPUINFO_STR_REGISTER + E132XS_G31: 		sprintf(info->s = cpuintrf_temp_str(), "G31 :%08X", e132xs.global_regs[31]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL0:  		sprintf(info->s = cpuintrf_temp_str(), "CL0 :%08X", e132xs.local_regs[(0 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL1:  		sprintf(info->s = cpuintrf_temp_str(), "CL1 :%08X", e132xs.local_regs[(1 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL2:  		sprintf(info->s = cpuintrf_temp_str(), "CL2 :%08X", e132xs.local_regs[(2 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL3:  		sprintf(info->s = cpuintrf_temp_str(), "CL3 :%08X", e132xs.local_regs[(3 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL4:  		sprintf(info->s = cpuintrf_temp_str(), "CL4 :%08X", e132xs.local_regs[(4 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL5:  		sprintf(info->s = cpuintrf_temp_str(), "CL5 :%08X", e132xs.local_regs[(5 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL6:  		sprintf(info->s = cpuintrf_temp_str(), "CL6 :%08X", e132xs.local_regs[(6 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL7:  		sprintf(info->s = cpuintrf_temp_str(), "CL7 :%08X", e132xs.local_regs[(7 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL8:  		sprintf(info->s = cpuintrf_temp_str(), "CL8 :%08X", e132xs.local_regs[(8 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL9:  		sprintf(info->s = cpuintrf_temp_str(), "CL9 :%08X", e132xs.local_regs[(9 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL10: 		sprintf(info->s = cpuintrf_temp_str(), "CL10:%08X", e132xs.local_regs[(10 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL11: 		sprintf(info->s = cpuintrf_temp_str(), "CL11:%08X", e132xs.local_regs[(11 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL12: 		sprintf(info->s = cpuintrf_temp_str(), "CL12:%08X", e132xs.local_regs[(12 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL13: 		sprintf(info->s = cpuintrf_temp_str(), "CL13:%08X", e132xs.local_regs[(13 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL14: 		sprintf(info->s = cpuintrf_temp_str(), "CL14:%08X", e132xs.local_regs[(14 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_CL15: 		sprintf(info->s = cpuintrf_temp_str(), "CL15:%08X", e132xs.local_regs[(15 + GET_FP) % 64]); break;
		case CPUINFO_STR_REGISTER + E132XS_L0:  		sprintf(info->s = cpuintrf_temp_str(), "L0  :%08X", e132xs.local_regs[0]); break;
		case CPUINFO_STR_REGISTER + E132XS_L1:  		sprintf(info->s = cpuintrf_temp_str(), "L1  :%08X", e132xs.local_regs[1]); break;
		case CPUINFO_STR_REGISTER + E132XS_L2:  		sprintf(info->s = cpuintrf_temp_str(), "L2  :%08X", e132xs.local_regs[2]); break;
		case CPUINFO_STR_REGISTER + E132XS_L3:  		sprintf(info->s = cpuintrf_temp_str(), "L3  :%08X", e132xs.local_regs[3]); break;
		case CPUINFO_STR_REGISTER + E132XS_L4:  		sprintf(info->s = cpuintrf_temp_str(), "L4  :%08X", e132xs.local_regs[4]); break;
		case CPUINFO_STR_REGISTER + E132XS_L5:  		sprintf(info->s = cpuintrf_temp_str(), "L5  :%08X", e132xs.local_regs[5]); break;
		case CPUINFO_STR_REGISTER + E132XS_L6:  		sprintf(info->s = cpuintrf_temp_str(), "L6  :%08X", e132xs.local_regs[6]); break;
		case CPUINFO_STR_REGISTER + E132XS_L7:  		sprintf(info->s = cpuintrf_temp_str(), "L7  :%08X", e132xs.local_regs[7]); break;
		case CPUINFO_STR_REGISTER + E132XS_L8:  		sprintf(info->s = cpuintrf_temp_str(), "L8  :%08X", e132xs.local_regs[8]); break;
		case CPUINFO_STR_REGISTER + E132XS_L9:  		sprintf(info->s = cpuintrf_temp_str(), "L9  :%08X", e132xs.local_regs[9]); break;
		case CPUINFO_STR_REGISTER + E132XS_L10: 		sprintf(info->s = cpuintrf_temp_str(), "L10 :%08X", e132xs.local_regs[10]); break;
		case CPUINFO_STR_REGISTER + E132XS_L11: 		sprintf(info->s = cpuintrf_temp_str(), "L11 :%08X", e132xs.local_regs[11]); break;
		case CPUINFO_STR_REGISTER + E132XS_L12: 		sprintf(info->s = cpuintrf_temp_str(), "L12 :%08X", e132xs.local_regs[12]); break;
		case CPUINFO_STR_REGISTER + E132XS_L13: 		sprintf(info->s = cpuintrf_temp_str(), "L13 :%08X", e132xs.local_regs[13]); break;
		case CPUINFO_STR_REGISTER + E132XS_L14: 		sprintf(info->s = cpuintrf_temp_str(), "L14 :%08X", e132xs.local_regs[14]); break;
		case CPUINFO_STR_REGISTER + E132XS_L15: 		sprintf(info->s = cpuintrf_temp_str(), "L15 :%08X", e132xs.local_regs[15]); break;
		case CPUINFO_STR_REGISTER + E132XS_L16: 		sprintf(info->s = cpuintrf_temp_str(), "L16 :%08X", e132xs.local_regs[16]); break;
		case CPUINFO_STR_REGISTER + E132XS_L17: 		sprintf(info->s = cpuintrf_temp_str(), "L17 :%08X", e132xs.local_regs[17]); break;
		case CPUINFO_STR_REGISTER + E132XS_L18: 		sprintf(info->s = cpuintrf_temp_str(), "L18 :%08X", e132xs.local_regs[18]); break;
		case CPUINFO_STR_REGISTER + E132XS_L19: 		sprintf(info->s = cpuintrf_temp_str(), "L19 :%08X", e132xs.local_regs[19]); break;
		case CPUINFO_STR_REGISTER + E132XS_L20: 		sprintf(info->s = cpuintrf_temp_str(), "L20 :%08X", e132xs.local_regs[20]); break;
		case CPUINFO_STR_REGISTER + E132XS_L21: 		sprintf(info->s = cpuintrf_temp_str(), "L21 :%08X", e132xs.local_regs[21]); break;
		case CPUINFO_STR_REGISTER + E132XS_L22: 		sprintf(info->s = cpuintrf_temp_str(), "L22 :%08X", e132xs.local_regs[22]); break;
		case CPUINFO_STR_REGISTER + E132XS_L23: 		sprintf(info->s = cpuintrf_temp_str(), "L23 :%08X", e132xs.local_regs[23]); break;
		case CPUINFO_STR_REGISTER + E132XS_L24: 		sprintf(info->s = cpuintrf_temp_str(), "L24 :%08X", e132xs.local_regs[24]); break;
		case CPUINFO_STR_REGISTER + E132XS_L25: 		sprintf(info->s = cpuintrf_temp_str(), "L25 :%08X", e132xs.local_regs[25]); break;
		case CPUINFO_STR_REGISTER + E132XS_L26: 		sprintf(info->s = cpuintrf_temp_str(), "L26 :%08X", e132xs.local_regs[26]); break;
		case CPUINFO_STR_REGISTER + E132XS_L27: 		sprintf(info->s = cpuintrf_temp_str(), "L27 :%08X", e132xs.local_regs[27]); break;
		case CPUINFO_STR_REGISTER + E132XS_L28: 		sprintf(info->s = cpuintrf_temp_str(), "L28 :%08X", e132xs.local_regs[28]); break;
		case CPUINFO_STR_REGISTER + E132XS_L29: 		sprintf(info->s = cpuintrf_temp_str(), "L29 :%08X", e132xs.local_regs[29]); break;
		case CPUINFO_STR_REGISTER + E132XS_L30: 		sprintf(info->s = cpuintrf_temp_str(), "L30 :%08X", e132xs.local_regs[30]); break;
		case CPUINFO_STR_REGISTER + E132XS_L31: 		sprintf(info->s = cpuintrf_temp_str(), "L31 :%08X", e132xs.local_regs[31]); break;
		case CPUINFO_STR_REGISTER + E132XS_L32: 		sprintf(info->s = cpuintrf_temp_str(), "L32 :%08X", e132xs.local_regs[32]); break;
		case CPUINFO_STR_REGISTER + E132XS_L33: 		sprintf(info->s = cpuintrf_temp_str(), "L33 :%08X", e132xs.local_regs[33]); break;
		case CPUINFO_STR_REGISTER + E132XS_L34: 		sprintf(info->s = cpuintrf_temp_str(), "L34 :%08X", e132xs.local_regs[34]); break;
		case CPUINFO_STR_REGISTER + E132XS_L35: 		sprintf(info->s = cpuintrf_temp_str(), "L35 :%08X", e132xs.local_regs[35]); break;
		case CPUINFO_STR_REGISTER + E132XS_L36: 		sprintf(info->s = cpuintrf_temp_str(), "L36 :%08X", e132xs.local_regs[36]); break;
		case CPUINFO_STR_REGISTER + E132XS_L37: 		sprintf(info->s = cpuintrf_temp_str(), "L37 :%08X", e132xs.local_regs[37]); break;
		case CPUINFO_STR_REGISTER + E132XS_L38: 		sprintf(info->s = cpuintrf_temp_str(), "L38 :%08X", e132xs.local_regs[38]); break;
		case CPUINFO_STR_REGISTER + E132XS_L39: 		sprintf(info->s = cpuintrf_temp_str(), "L39 :%08X", e132xs.local_regs[39]); break;
		case CPUINFO_STR_REGISTER + E132XS_L40: 		sprintf(info->s = cpuintrf_temp_str(), "L40 :%08X", e132xs.local_regs[40]); break;
		case CPUINFO_STR_REGISTER + E132XS_L41: 		sprintf(info->s = cpuintrf_temp_str(), "L41 :%08X", e132xs.local_regs[41]); break;
		case CPUINFO_STR_REGISTER + E132XS_L42: 		sprintf(info->s = cpuintrf_temp_str(), "L42 :%08X", e132xs.local_regs[42]); break;
		case CPUINFO_STR_REGISTER + E132XS_L43: 		sprintf(info->s = cpuintrf_temp_str(), "L43 :%08X", e132xs.local_regs[43]); break;
		case CPUINFO_STR_REGISTER + E132XS_L44: 		sprintf(info->s = cpuintrf_temp_str(), "L44 :%08X", e132xs.local_regs[44]); break;
		case CPUINFO_STR_REGISTER + E132XS_L45: 		sprintf(info->s = cpuintrf_temp_str(), "L45 :%08X", e132xs.local_regs[45]); break;
		case CPUINFO_STR_REGISTER + E132XS_L46: 		sprintf(info->s = cpuintrf_temp_str(), "L46 :%08X", e132xs.local_regs[46]); break;
		case CPUINFO_STR_REGISTER + E132XS_L47: 		sprintf(info->s = cpuintrf_temp_str(), "L47 :%08X", e132xs.local_regs[47]); break;
		case CPUINFO_STR_REGISTER + E132XS_L48: 		sprintf(info->s = cpuintrf_temp_str(), "L48 :%08X", e132xs.local_regs[48]); break;
		case CPUINFO_STR_REGISTER + E132XS_L49: 		sprintf(info->s = cpuintrf_temp_str(), "L49 :%08X", e132xs.local_regs[49]); break;
		case CPUINFO_STR_REGISTER + E132XS_L50: 		sprintf(info->s = cpuintrf_temp_str(), "L50 :%08X", e132xs.local_regs[50]); break;
		case CPUINFO_STR_REGISTER + E132XS_L51: 		sprintf(info->s = cpuintrf_temp_str(), "L51 :%08X", e132xs.local_regs[51]); break;
		case CPUINFO_STR_REGISTER + E132XS_L52: 		sprintf(info->s = cpuintrf_temp_str(), "L52 :%08X", e132xs.local_regs[52]); break;
		case CPUINFO_STR_REGISTER + E132XS_L53: 		sprintf(info->s = cpuintrf_temp_str(), "L53 :%08X", e132xs.local_regs[53]); break;
		case CPUINFO_STR_REGISTER + E132XS_L54: 		sprintf(info->s = cpuintrf_temp_str(), "L54 :%08X", e132xs.local_regs[54]); break;
		case CPUINFO_STR_REGISTER + E132XS_L55: 		sprintf(info->s = cpuintrf_temp_str(), "L55 :%08X", e132xs.local_regs[55]); break;
		case CPUINFO_STR_REGISTER + E132XS_L56: 		sprintf(info->s = cpuintrf_temp_str(), "L56 :%08X", e132xs.local_regs[56]); break;
		case CPUINFO_STR_REGISTER + E132XS_L57: 		sprintf(info->s = cpuintrf_temp_str(), "L57 :%08X", e132xs.local_regs[57]); break;
		case CPUINFO_STR_REGISTER + E132XS_L58: 		sprintf(info->s = cpuintrf_temp_str(), "L58 :%08X", e132xs.local_regs[58]); break;
		case CPUINFO_STR_REGISTER + E132XS_L59: 		sprintf(info->s = cpuintrf_temp_str(), "L59 :%08X", e132xs.local_regs[59]); break;
		case CPUINFO_STR_REGISTER + E132XS_L60: 		sprintf(info->s = cpuintrf_temp_str(), "L60 :%08X", e132xs.local_regs[60]); break;
		case CPUINFO_STR_REGISTER + E132XS_L61: 		sprintf(info->s = cpuintrf_temp_str(), "L61 :%08X", e132xs.local_regs[61]); break;
		case CPUINFO_STR_REGISTER + E132XS_L62: 		sprintf(info->s = cpuintrf_temp_str(), "L62 :%08X", e132xs.local_regs[62]); break;
		case CPUINFO_STR_REGISTER + E132XS_L63: 		sprintf(info->s = cpuintrf_temp_str(), "L63 :%08X", e132xs.local_regs[63]); break;
	}
}
