/*****************************************************************************
 *
 *	 6502dasm.c
 *	 6502/65c02/6510 disassembler
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include <stdio.h>
#ifdef MAME_DEBUG
#include "driver.h"
#include "mamedbg.h"
#include "m6502.h"

#define OPCODE(A)	cpu_readop(A)
#define ARGBYTE(A)	cpu_readop_arg(A)
#define ARGWORD(A)	cpu_readop_arg(A)+(cpu_readop_arg((A+1) & 0xffff) << 8)

#define RDMEM(A)	cpu_readmem16(A)

enum addr_mode {
	_non=0, 	 /* no additional arguments */
    _imp,        /* implicit */
	_acc,		 /* accumulator */
	_imm,		 /* immediate */
	_adr,		 /* absolute address (jmp,jsr) */
    _abs,        /* absolute */
	_zpg,		 /* zero page */
	_zpx,		 /* zero page + X */
	_zpy,		 /* zero page + Y */
	_zpi,		 /* zero page indirect (65c02) */
	_abx,		 /* absolute + X */
	_aby,		 /* absolute + Y */
	_rel,		 /* relative */
	_idx,		 /* zero page pre indexed */
	_idy,		 /* zero page post indexed */
	_ind,		 /* indirect (jmp) */
	_iax		 /* indirect + X (65c02 jmp) */
};

enum opcodes {
	_adc=0,_and,  _asl,  _bcc,	_bcs,  _beq,  _bit,  _bmi,
	_bne,  _bpl,  _brk,  _bvc,	_bvs,  _clc,  _cld,  _cli,
	_clv,  _cmp,  _cpx,  _cpy,	_dec,  _dex,  _dey,  _eor,
	_inc,  _inx,  _iny,  _jmp,	_jsr,  _lda,  _ldx,  _ldy,
	_lsr,  _nop,  _ora,  _pha,	_php,  _pla,  _plp,  _rol,
	_ror,  _rti,  _rts,  _sbc,	_sec,  _sed,  _sei,  _sta,
	_stx,  _sty,  _tax,  _tay,	_tsx,  _txa,  _txs,  _tya,
	_ill,
/* 65c02 (only) mnemonics */
	_bra,  _stz,  _trb,  _tsb,
/* 6510 + 65c02 mnemonics */
	_anc,  _asr,  _ast,  _arr,	_asx,  _axa,  _dcp,  _dea,
	_dop,  _ina,  _isc,  _lax,	_phx,  _phy,  _plx,  _ply,
	_rla,  _rra,  _sax,  _slo,	_sre,  _sah,  _say,  _ssh,
	_sxh,  _syh,  _top
};


static const char *token[]=
{
	"adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi",
	"bne", "bpl", "brk", "bvc", "bvs", "clc", "cld", "cli",
	"clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor",
	"inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy",
	"lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol",
	"ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta",
	"stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
	"ill",
/* 65c02 mnemonics */
	"bra", "stz", "trb", "tsb",
/* 6510 mnemonics */
	"anc", "asr", "ast", "arr", "asx", "axa", "dcp", "dea",
	"dop", "ina", "isc", "lax", "phx", "phy", "plx", "ply",
	"rla", "rra", "sax", "slo", "sre", "sah", "say", "ssh",
	"sxh", "syh", "top"
};

/* Some really short names for the EA access modes */
#define _0	EA_NONE
#define _V	EA_VALUE
#define _JP EA_ABS_PC
#define _JR EA_REL_PC
#define _RM EA_MEM_RD
#define _WM EA_MEM_WR
#define _RW EA_MEM_RDWR

static const UINT8 op6502[256][3] =
{
  {_brk,_imp,_0 }, {_ora,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 00 */
  {_ill,_non,_0 }, {_ora,_zpg,_RM}, {_asl,_zpg,_RW}, {_ill,_non,_0 },
  {_php,_imp,_0 }, {_ora,_imm,_V }, {_asl,_acc,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_ora,_abs,_RM}, {_asl,_abs,_RW}, {_ill,_non,_0 },
  {_bpl,_rel,_JR}, {_ora,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 10 */
  {_ill,_non,_0 }, {_ora,_zpx,_RM}, {_asl,_zpx,_RW}, {_ill,_non,_0 },
  {_clc,_imp,_0 }, {_ora,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_ora,_abx,_RM}, {_asl,_abx,_RW}, {_ill,_non,_0 },
  {_jsr,_adr,_JP}, {_and,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 20 */
  {_bit,_zpg,_RM}, {_and,_zpg,_RM}, {_rol,_zpg,_RW}, {_ill,_non,_0 },
  {_plp,_imp,_0 }, {_and,_imm,_V }, {_rol,_acc,_0 }, {_ill,_non,_0 },
  {_bit,_abs,_RM}, {_and,_abs,_RM}, {_rol,_abs,_RW}, {_ill,_non,_0 },
  {_bmi,_rel,_JR}, {_and,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 30 */
  {_ill,_non,_0 }, {_and,_zpx,_RM}, {_rol,_zpx,_RW}, {_ill,_non,_0 },
  {_sec,_imp,_0 }, {_and,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_and,_abx,_RM}, {_rol,_abx,_RW}, {_ill,_non,_0 },
  {_rti,_imp,_0 }, {_eor,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 40 */
  {_ill,_non,_0 }, {_eor,_zpg,_RM}, {_lsr,_zpg,_RW}, {_ill,_non,_0 },
  {_pha,_imp,_0 }, {_eor,_imm,_V }, {_lsr,_acc,_0 }, {_ill,_non,_0 },
  {_jmp,_adr,_JP}, {_eor,_abs,_RM}, {_lsr,_abs,_RW}, {_ill,_non,_0 },
  {_bvc,_rel,_JR}, {_eor,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 50 */
  {_ill,_non,_0 }, {_eor,_zpx,_RM}, {_lsr,_zpx,_RW}, {_ill,_non,_0 },
  {_cli,_imp,_0 }, {_eor,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_eor,_abx,_RM}, {_lsr,_abx,_RW}, {_ill,_non,_0 },
  {_rts,_imp,_0 }, {_adc,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 60 */
  {_ill,_non,_0 }, {_adc,_zpg,_RM}, {_ror,_zpg,_RW}, {_ill,_non,_0 },
  {_pla,_imp,_0 }, {_adc,_imm,_V }, {_ror,_acc,_0 }, {_ill,_non,_0 },
  {_jmp,_ind,_JP}, {_adc,_abs,_RM}, {_ror,_abs,_RW}, {_ill,_non,_0 },
  {_bvs,_rel,_JR}, {_adc,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 70 */
  {_ill,_non,_0 }, {_adc,_zpx,_RM}, {_ror,_zpx,_RW}, {_ill,_non,_0 },
  {_sei,_imp,_0 }, {_adc,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_adc,_abx,_RM}, {_ror,_abx,_RW}, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_sta,_idx,_WM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 80 */
  {_sty,_zpg,_WM}, {_sta,_zpg,_WM}, {_stx,_zpg,_WM}, {_ill,_non,_0 },
  {_dey,_imp,_0 }, {_ill,_non,_0 }, {_txa,_imp,_0 }, {_ill,_non,_0 },
  {_sty,_abs,_WM}, {_sta,_abs,_WM}, {_stx,_abs,_WM}, {_ill,_non,_0 },
  {_bcc,_rel,_JR}, {_sta,_idy,_WM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 90 */
  {_sty,_zpx,_WM}, {_sta,_zpx,_WM}, {_stx,_zpy,_WM}, {_ill,_non,_0 },
  {_tya,_imp,_0 }, {_sta,_aby,_WM}, {_txs,_imp,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_sta,_abx,_WM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ldy,_imm,_V }, {_lda,_idx,_RM}, {_ldx,_imm,_V }, {_ill,_non,_0 }, /* a0 */
  {_ldy,_zpg,_RM}, {_lda,_zpg,_RM}, {_ldx,_zpg,_RM}, {_ill,_non,_0 },
  {_tay,_imp,_0 }, {_lda,_imm,_V }, {_tax,_imp,_0 }, {_ill,_non,_0 },
  {_ldy,_abs,_RM}, {_lda,_abs,_RM}, {_ldx,_abs,_RM}, {_ill,_non,_0 },
  {_bcs,_rel,_JR}, {_lda,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* b0 */
  {_ldy,_zpx,_RM}, {_lda,_zpx,_RM}, {_ldx,_zpy,_RM}, {_ill,_non,_0 },
  {_clv,_imp,_0 }, {_lda,_aby,_RM}, {_tsx,_imp,_0 }, {_ill,_non,_0 },
  {_ldy,_abx,_RM}, {_lda,_abx,_RM}, {_ldx,_aby,_RM}, {_ill,_non,_0 },
  {_cpy,_imm,_V }, {_cmp,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* c0 */
  {_cpy,_zpg,_RM}, {_cmp,_zpg,_RM}, {_dec,_zpg,_RW}, {_ill,_non,_0 },
  {_iny,_imp,_0 }, {_cmp,_imm,_V }, {_dex,_imp,_0 }, {_ill,_non,_0 },
  {_cpy,_abs,_RM}, {_cmp,_abs,_RM}, {_dec,_abs,_RW}, {_ill,_non,_0 },
  {_bne,_rel,_JR}, {_cmp,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* d0 */
  {_ill,_non,_0 }, {_cmp,_zpx,_RM}, {_dec,_zpx,_RW}, {_ill,_non,_0 },
  {_cld,_imp,_0 }, {_cmp,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_cmp,_abx,_RM}, {_dec,_abx,_RW}, {_ill,_non,_0 },
  {_cpx,_imm,_V }, {_sbc,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* e0 */
  {_cpx,_zpg,_RM}, {_sbc,_zpg,_RM}, {_inc,_zpg,_RW}, {_ill,_non,_0 },
  {_inx,_imp,_0 }, {_sbc,_imm,_V }, {_nop,_imp,_0 }, {_ill,_non,_0 },
  {_cpx,_abs,_RM}, {_sbc,_abs,_RM}, {_inc,_abs,_RW}, {_ill,_non,_0 },
  {_beq,_rel,_JR}, {_sbc,_idy,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* f0 */
  {_ill,_non,_0 }, {_sbc,_zpx,_RM}, {_inc,_zpx,_RW}, {_ill,_non,_0 },
  {_sed,_imp,_0 }, {_sbc,_aby,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_sbc,_abx,_RM}, {_inc,_abx,_RW}, {_ill,_non,_0 }
};

static const UINT8 op65c02[256][3] =
{
  {_brk,_imp,_0 }, {_ora,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 00 */
  {_tsb,_zpg,_0 }, {_ora,_zpg,_RM}, {_asl,_zpg,_RW}, {_ill,_non,_0 },
  {_php,_imp,_0 }, {_ora,_imm,_V }, {_asl,_acc,_RW}, {_ill,_non,_0 },
  {_tsb,_abs,_RM}, {_ora,_abs,_RM}, {_asl,_abs,_RW}, {_ill,_non,_0 },
  {_bpl,_rel,_JR}, {_ora,_idy,_RM}, {_ora,_zpi,_RM}, {_ill,_non,_0 }, /* 10 */
  {_trb,_zpg,_RM}, {_ora,_zpx,_RM}, {_asl,_zpx,_RW}, {_ill,_non,_0 },
  {_clc,_imp,_0 }, {_ora,_aby,_RM}, {_ina,_imp,_0 }, {_ill,_non,_0 },
  {_tsb,_abs,_RM}, {_ora,_abx,_RM}, {_asl,_abx,_RW}, {_ill,_non,_0 },
  {_jsr,_adr,_0 }, {_and,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 20 */
  {_bit,_zpg,_RM}, {_and,_zpg,_RM}, {_rol,_zpg,_RW}, {_ill,_non,_0 },
  {_plp,_imp,_0 }, {_and,_imm,_V }, {_rol,_acc,_0 }, {_ill,_non,_0 },
  {_bit,_abs,_RM}, {_and,_abs,_RM}, {_rol,_abs,_RW}, {_ill,_non,_0 },
  {_bmi,_rel,_JR}, {_and,_idy,_RM}, {_and,_zpi,_RM}, {_ill,_non,_0 }, /* 30 */
  {_bit,_zpx,_RM}, {_and,_zpx,_RM}, {_rol,_zpx,_RW}, {_ill,_non,_0 },
  {_sec,_imp,_0 }, {_and,_aby,_RM}, {_dea,_imp,_0 }, {_ill,_non,_0 },
  {_bit,_abx,_RM}, {_and,_abx,_RM}, {_rol,_abx,_RW}, {_ill,_non,_0 },
  {_rti,_imp,_0 }, {_eor,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 40 */
  {_ill,_non,_0 }, {_eor,_zpg,_RM}, {_lsr,_zpg,_RW}, {_ill,_non,_0 },
  {_pha,_imp,_0 }, {_eor,_imm,_V }, {_lsr,_acc,_0 }, {_ill,_non,_0 },
  {_jmp,_adr,_JP}, {_eor,_abs,_RM}, {_lsr,_abs,_RW}, {_ill,_non,_0 },
  {_bvc,_rel,_JR}, {_eor,_idy,_RM}, {_eor,_zpi,_RM}, {_ill,_non,_0 }, /* 50 */
  {_ill,_non,_0 }, {_eor,_zpx,_RM}, {_lsr,_zpx,_RW}, {_ill,_non,_0 },
  {_cli,_imp,_0 }, {_eor,_aby,_RM}, {_phy,_imp,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_eor,_abx,_RM}, {_lsr,_abx,_RW}, {_ill,_non,_0 },
  {_rts,_imp,_0 }, {_adc,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 60 */
  {_stz,_zpg,_WM}, {_adc,_zpg,_RM}, {_ror,_zpg,_RW}, {_ill,_non,_0 },
  {_pla,_imp,_0 }, {_adc,_imm,_V }, {_ror,_acc,_0 }, {_ill,_non,_0 },
  {_jmp,_ind,_JP}, {_adc,_abs,_RM}, {_ror,_abs,_RW}, {_ill,_non,_0 },
  {_bvs,_rel,_JR}, {_adc,_idy,_RM}, {_adc,_zpi,_RM}, {_ill,_non,_0 }, /* 70 */
  {_stz,_zpx,_WM}, {_adc,_zpx,_RM}, {_ror,_zpx,_RW}, {_ill,_non,_0 },
  {_sei,_imp,_0 }, {_adc,_aby,_RM}, {_ply,_imp,_0 }, {_ill,_non,_0 },
  {_jmp,_iax,_JP}, {_adc,_abx,_RM}, {_ror,_abx,_RW}, {_ill,_non,_0 },
  {_bra,_rel,_JR}, {_sta,_idx,_WM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* 80 */
  {_sty,_zpg,_WM}, {_sta,_zpg,_WM}, {_stx,_zpg,_WM}, {_ill,_non,_0 },
  {_dey,_imp,_0 }, {_bit,_imm,_V }, {_txa,_imp,_0 }, {_ill,_non,_0 },
  {_sty,_abs,_WM}, {_sta,_abs,_WM}, {_stx,_abs,_WM}, {_ill,_non,_0 },
  {_bcc,_rel,_JR}, {_sta,_idy,_WM}, {_sta,_zpi,_WM}, {_ill,_non,_0 }, /* 90 */
  {_sty,_zpx,_WM}, {_sta,_zpx,_WM}, {_stx,_zpy,_WM}, {_ill,_non,_0 },
  {_tya,_imp,_0 }, {_sta,_aby,_WM}, {_txs,_imp,_0 }, {_ill,_non,_0 },
  {_stz,_abs,_WM}, {_sta,_abx,_WM}, {_stz,_abx,_WM}, {_ill,_non,_0 },
  {_ldy,_imm,_V }, {_lda,_idx,_RM}, {_ldx,_imm,_V }, {_ill,_non,_0 }, /* a0 */
  {_ldy,_zpg,_RM}, {_lda,_zpg,_RM}, {_ldx,_zpg,_RM}, {_ill,_non,_0 },
  {_tay,_imp,_0 }, {_lda,_imm,_V }, {_tax,_imp,_0 }, {_ill,_non,_0 },
  {_ldy,_abs,_RM}, {_lda,_abs,_RM}, {_ldx,_abs,_RM}, {_ill,_non,_0 },
  {_bcs,_rel,_JR}, {_lda,_idy,_RM}, {_lda,_zpi,_RM}, {_ill,_non,_0 }, /* b0 */
  {_ldy,_zpx,_RM}, {_lda,_zpx,_RM}, {_ldx,_zpy,_RM}, {_ill,_non,_0 },
  {_clv,_imp,_0 }, {_lda,_aby,_RM}, {_tsx,_imp,_0 }, {_ill,_non,_0 },
  {_ldy,_abx,_RM}, {_lda,_abx,_RM}, {_ldx,_aby,_RM}, {_ill,_non,_0 },
  {_cpy,_imm,_V }, {_cmp,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* c0 */
  {_cpy,_zpg,_RM}, {_cmp,_zpg,_RM}, {_dec,_zpg,_RW}, {_ill,_non,_0 },
  {_iny,_imp,_0 }, {_cmp,_imm,_V }, {_dex,_imp,_0 }, {_ill,_non,_0 },
  {_cpy,_abs,_RM}, {_cmp,_abs,_RM}, {_dec,_abs,_RW}, {_ill,_non,_0 },
  {_bne,_rel,_JR}, {_cmp,_idy,_RM}, {_cmp,_zpi,_RM}, {_ill,_non,_0 }, /* d0 */
  {_ill,_non,_0 }, {_cmp,_zpx,_RM}, {_dec,_zpx,_RW}, {_ill,_non,_0 },
  {_cld,_imp,_0 }, {_cmp,_aby,_RM}, {_phx,_imp,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_cmp,_abx,_RM}, {_dec,_abx,_RW}, {_ill,_non,_0 },
  {_cpx,_imm,_V }, {_sbc,_idx,_RM}, {_ill,_non,_0 }, {_ill,_non,_0 }, /* e0 */
  {_cpx,_zpg,_RM}, {_sbc,_zpg,_RM}, {_inc,_zpg,_RW}, {_ill,_non,_0 },
  {_inx,_imp,_0 }, {_sbc,_imm,_V }, {_nop,_imp,_0 }, {_ill,_non,_0 },
  {_cpx,_abs,_RM}, {_sbc,_abs,_RM}, {_inc,_abs,_RW}, {_ill,_non,_0 },
  {_beq,_rel,_JR}, {_sbc,_idy,_RM}, {_sbc,_zpi,_RM}, {_ill,_non,_0 }, /* f0 */
  {_ill,_non,_0 }, {_sbc,_zpx,_RM}, {_inc,_zpx,_RW}, {_ill,_non,_0 },
  {_sed,_imp,_0 }, {_sbc,_aby,_RM}, {_plx,_imp,_0 }, {_ill,_non,_0 },
  {_ill,_non,_0 }, {_sbc,_abx,_RM}, {_inc,_abx,_RW}, {_ill,_non,_0 }
};

static const UINT8 op6510[256][3] =
{
  {_brk,_imp,_0 }, {_ora,_idx,_RM}, {_ill,_non,_0 }, {_slo,_idx,_RW}, /* 00 */
  {_dop,_imp,_0 }, {_ora,_zpg,_RM}, {_asl,_zpg,_RW}, {_slo,_zpg,_RW},
  {_php,_imp,_0 }, {_ora,_imm,_V }, {_asl,_acc,_0 }, {_anc,_imm,_V },
  {_top,_imp,_0 }, {_ora,_abs,_RM}, {_asl,_abs,_RW}, {_slo,_abs,_RW},
  {_bpl,_rel,_JR}, {_ora,_idy,_RM}, {_nop,_imp,_0 }, {_slo,_idy,_RW}, /* 10 */
  {_dop,_imp,_0 }, {_ora,_zpx,_RM}, {_asl,_zpx,_RW}, {_slo,_zpx,_RW},
  {_clc,_imp,_0 }, {_ora,_aby,_RM}, {_ill,_non,_0 }, {_slo,_aby,_RW},
  {_top,_imp,_0 }, {_ora,_abx,_RM}, {_asl,_abx,_RW}, {_slo,_abx,_RW},
  {_jsr,_adr,_JP}, {_and,_idx,_RM}, {_ill,_non,_0 }, {_rla,_idx,_RW}, /* 20 */
  {_bit,_zpg,_RM}, {_and,_zpg,_RM}, {_rol,_zpg,_RW}, {_rla,_zpg,_RW},
  {_plp,_imp,_0 }, {_and,_imm,_V }, {_rol,_acc,_0 }, {_anc,_imm,_V },
  {_bit,_abs,_RM}, {_and,_abs,_RM}, {_rol,_abs,_RW}, {_rla,_abs,_RW},
  {_bmi,_rel,_JR}, {_and,_idy,_RM}, {_ill,_non,_0 }, {_rla,_idy,_RW}, /* 30 */
  {_dop,_imp,_0 }, {_and,_zpx,_RM}, {_rol,_zpx,_RW}, {_rla,_zpx,_RW},
  {_sec,_imp,_0 }, {_and,_aby,_RM}, {_nop,_imp,_0 }, {_rla,_aby,_RW},
  {_top,_imp,_0 }, {_and,_abx,_RM}, {_rol,_abx,_RW}, {_rla,_abx,_RW},
  {_rti,_imp,_0 }, {_eor,_idx,_RM}, {_ill,_non,_0 }, {_sre,_idx,_RW}, /* 40 */
  {_dop,_imp,_0 }, {_eor,_zpg,_RM}, {_lsr,_zpg,_RW}, {_sre,_zpg,_RW},
  {_pha,_imp,_0 }, {_eor,_imm,_V }, {_lsr,_acc,_0 }, {_asr,_imm,_V },
  {_jmp,_adr,_JP}, {_eor,_abs,_RM}, {_lsr,_abs,_RW}, {_sre,_abs,_RW},
  {_bvc,_rel,_JR}, {_eor,_idy,_RM}, {_ill,_non,_0 }, {_sre,_idy,_RW}, /* 50 */
  {_dop,_imp,_0 }, {_eor,_zpx,_RM}, {_lsr,_zpx,_RW}, {_sre,_zpx,_RW},
  {_cli,_imp,_0 }, {_eor,_aby,_RM}, {_nop,_imp,_0 }, {_sre,_aby,_RW},
  {_top,_imp,_0 }, {_eor,_abx,_RM}, {_lsr,_abx,_RW}, {_sre,_abx,_RW},
  {_rts,_imp,_0 }, {_adc,_idx,_RM}, {_ill,_non,_0 }, {_rra,_idx,_RW}, /* 60 */
  {_dop,_imp,_0 }, {_adc,_zpg,_RM}, {_ror,_zpg,_RW}, {_rra,_zpg,_RW},
  {_pla,_imp,_0 }, {_adc,_imm,_V }, {_ror,_acc,_0 }, {_arr,_imm,_V },
  {_jmp,_ind,_JP}, {_adc,_abs,_RM}, {_ror,_abs,_RW}, {_rra,_abs,_RW},
  {_bvs,_rel,_JR}, {_adc,_idy,_RM}, {_ill,_non,_0 }, {_rra,_idy,_RW}, /* 70 */
  {_dop,_imp,_0 }, {_adc,_zpx,_RM}, {_ror,_zpx,_RW}, {_rra,_zpx,_RW},
  {_sei,_imp,_0 }, {_adc,_aby,_RM}, {_nop,_imp,_0 }, {_rra,_aby,_RW},
  {_top,_imp,_0 }, {_adc,_abx,_RM}, {_ror,_abx,_RW}, {_rra,_abx,_RW},
  {_dop,_imp,_0 }, {_sta,_idx,_WM}, {_dop,_imp,_0 }, {_sax,_idx,_WM}, /* 80 */
  {_sty,_zpg,_WM}, {_sta,_zpg,_WM}, {_stx,_zpg,_WM}, {_sax,_zpg,_WM},
  {_dey,_imp,_0 }, {_dop,_imp,_0 }, {_txa,_imp,_0 }, {_axa,_imm,_V },
  {_sty,_abs,_WM}, {_sta,_abs,_WM}, {_stx,_abs,_WM}, {_sax,_abs,_WM},
  {_bcc,_rel,_JR}, {_sta,_idy,_WM}, {_ill,_non,_0 }, {_say,_idy,_WM}, /* 90 */
  {_sty,_zpx,_WM}, {_sta,_zpx,_WM}, {_stx,_zpy,_WM}, {_sax,_zpx,_WM},
  {_tya,_imp,_0 }, {_sta,_aby,_WM}, {_txs,_imp,_0 }, {_ssh,_aby,_WM},
  {_syh,_abx,_0 }, {_sta,_abx,_WM}, {_ill,_non,_0 }, {_sah,_aby,_WM},
  {_ldy,_imm,_V }, {_lda,_idx,_RM}, {_ldx,_imm,_V }, {_lax,_idx,_RM}, /* a0 */
  {_ldy,_zpg,_RM}, {_lda,_zpg,_RM}, {_ldx,_zpg,_RM}, {_lax,_zpg,_RM},
  {_tay,_imp,_0 }, {_lda,_imm,_V }, {_tax,_imp,_0 }, {_lax,_imm,_V },
  {_ldy,_abs,_RM}, {_lda,_abs,_RM}, {_ldx,_abs,_RM}, {_lax,_abs,_RM},
  {_bcs,_rel,_JR}, {_lda,_idy,_RM}, {_ill,_non,_0 }, {_lax,_idy,_RM}, /* b0 */
  {_ldy,_zpx,_RM}, {_lda,_zpx,_RM}, {_ldx,_zpy,_RM}, {_lax,_zpx,_RM},
  {_clv,_imp,_0 }, {_lda,_aby,_RM}, {_tsx,_imp,_0 }, {_ast,_aby,_RM},
  {_ldy,_abx,_RM}, {_lda,_abx,_RM}, {_ldx,_aby,_RM}, {_lax,_abx,_RM},
  {_cpy,_imm,_V }, {_cmp,_idx,_RM}, {_dop,_imp,_0 }, {_dcp,_idx,_RW}, /* c0 */
  {_cpy,_zpg,_RM}, {_cmp,_zpg,_RM}, {_dec,_zpg,_RW}, {_dcp,_zpg,_RW},
  {_iny,_imp,_0 }, {_cmp,_imm,_V }, {_dex,_imp,_0 }, {_asx,_imm,_V },
  {_cpy,_abs,_RM}, {_cmp,_abs,_RM}, {_dec,_abs,_RW}, {_dcp,_abs,_RW},
  {_bne,_rel,_JR}, {_cmp,_idy,_RM}, {_ill,_non,_0 }, {_dcp,_idy,_RW}, /* d0 */
  {_dop,_imp,_0 }, {_cmp,_zpx,_RM}, {_dec,_zpx,_RW}, {_dcp,_zpx,_RW},
  {_cld,_imp,_0 }, {_cmp,_aby,_RM}, {_nop,_imp,_0 }, {_dcp,_aby,_RW},
  {_top,_imp,_0 }, {_cmp,_abx,_RM}, {_dec,_abx,_RW}, {_dcp,_abx,_RW},
  {_cpx,_imm,_V }, {_sbc,_idx,_RM}, {_dop,_imp,_0 }, {_isc,_idx,_RW}, /* e0 */
  {_cpx,_zpg,_RM}, {_sbc,_zpg,_RM}, {_inc,_zpg,_RW}, {_isc,_zpg,_RW},
  {_inx,_imp,_0 }, {_sbc,_imm,_V }, {_nop,_imp,_0 }, {_sbc,_imm,_V },
  {_cpx,_abs,_RM}, {_sbc,_abs,_RM}, {_inc,_abs,_RW}, {_isc,_abs,_RW},
  {_beq,_rel,_JR}, {_sbc,_idy,_RM}, {_ill,_non,_0 }, {_isc,_idy,_RW}, /* f0 */
  {_dop,_imp,_0 }, {_sbc,_zpx,_RM}, {_inc,_zpx,_RW}, {_isc,_zpx,_RW},
  {_sed,_imp,_0 }, {_sbc,_aby,_RM}, {_nop,_imp,_0 }, {_isc,_aby,_RW},
  {_top,_imp,_0 }, {_sbc,_abx,_RM}, {_inc,_abx,_RW}, {_isc,_abx,_RW}
};

/*****************************************************************************
 *	Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned Dasm6502(char *buffer, unsigned pc)
{
	char *dst = buffer;
	const char *symbol;
	INT8 offset;
	unsigned PC = pc;
	UINT16 addr, ea;
	UINT8 op, opc, arg, acc, value;

	op = OPCODE(pc++);

	switch ( m6502_get_reg(M6502_SUBTYPE) )
	{
#if HAS_M65C02
		case CPU_M65C02:
			opc = op65c02[op][0];
			arg = op65c02[op][1];
			acc = op65c02[op][2];
			break;
#endif
#if HAS_M6510
		case CPU_M6510:
			opc = op6510[op][0];
			arg = op6510[op][1];
			acc = op6510[op][2];
            break;
#endif
        default:
			opc = op6502[op][0];
			arg = op6502[op][1];
			acc = op6502[op][2];
            break;
	}

	dst += sprintf(dst, "%-5s", token[opc]);

    switch(arg)
	{
	case _imp:
		break;

	case _acc:
		dst += sprintf(dst,"a");
		break;

	case _rel:
		offset = (INT8)ARGBYTE(pc++);
		symbol = set_ea_info( 0, pc, offset, acc );
		dst += sprintf(dst,"%s", symbol);
		break;

	case _imm:
		value = ARGBYTE(pc++);
		symbol = set_ea_info( 0, value, EA_UINT8, acc );
		dst += sprintf(dst,"#%s", symbol);
		break;

	case _zpg:
		addr = ARGBYTE(pc++);
		symbol = set_ea_info( 0, addr, EA_UINT8, acc );
		dst += sprintf(dst,"$%02X", addr);
		break;

	case _zpx:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_X)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, acc );
        dst += sprintf(dst,"$%02X,x", addr);
		break;

	case _zpy:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_Y)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, acc );
        dst += sprintf(dst,"$%02X,y", addr);
		break;

	case _idx:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_X)) & 0xff;
		ea = RDMEM(ea) + (RDMEM((ea+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"($%02X,x)", addr);
		break;

	case _idy:
		addr = ARGBYTE(pc++);
        ea = (RDMEM(addr) + (RDMEM((addr+1) & 0xff) << 8) + m6502_get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"($%02X),y", addr);
		break;

	case _zpi:
		addr = ARGBYTE(pc++);
        ea = RDMEM(addr) + (RDMEM((addr+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"($%02X)", addr);
		break;

	case _adr:
		addr = ARGWORD(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, acc );
		dst += sprintf(dst,"%s", symbol);
		break;

	case _abs:
		addr = ARGWORD(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, acc );
		dst += sprintf(dst,"%s", symbol);
		break;

	case _abx:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (addr + m6502_get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"$%04X,x", addr);
		break;

	case _aby:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (addr + m6502_get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"$%04X,y", addr);
		break;

	case _ind:
		addr = ARGWORD(pc);
		pc += 2;
        ea = ARGWORD(addr);
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"($%04X)", addr);
		break;

	case _iax:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (ARGWORD(addr) + m6502_get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, acc );
        dst += sprintf(dst,"($%04X),X", addr);
		break;

	default:
		dst += sprintf(dst,"$%02X", op);
	}
	return pc - PC;
}

#endif

