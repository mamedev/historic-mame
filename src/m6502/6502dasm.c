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
#include "memory.h"

#define RDOP(A) cpu_readop(A)
#define RDBYTE(A) cpu_readop_arg(A)
#define RDWORD(A) cpu_readop_arg(A)+(cpu_readop_arg((A+1) & 0xffff) << 8)

enum addr_mode {
	_non=0, 	 /* no additional arguments */
	_acc,		 /* accumulator */
	_imp,		 /* implicit */
	_imm,		 /* immediate */
	_abs,		 /* absolute */
	_zpg,		 /* zero page */
	_zpx,		 /* zero page + X */
	_zpy,		 /* zero page + Y */
	_zpi,		 /* zero page indirect (65c02) */
	_abx,		 /* absolute + X */
	_aby,		 /* absolute + Y */
	_rel,		 /* relative */
	_idx,		 /* zero page pre indexed */
	_idy,		 /* zero page post indexed */
	_ind,		 /* indirect */
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

static const unsigned char op6502[512]=
{
  _brk,_imp, _ora,_idx, _ill,_non, _ill,_non, _ill,_non, _ora,_zpg, _asl,_zpg, _ill,_non, /* 00 */
  _php,_imp, _ora,_imm, _asl,_acc, _ill,_non, _ill,_non, _ora,_abs, _asl,_abs, _ill,_non,
  _bpl,_rel, _ora,_idy, _ill,_non, _ill,_non, _ill,_non, _ora,_zpx, _asl,_zpx, _ill,_non, /* 10 */
  _clc,_imp, _ora,_aby, _ill,_non, _ill,_non, _ill,_non, _ora,_abx, _asl,_abx, _ill,_non,
  _jsr,_abs, _and,_idx, _ill,_non, _ill,_non, _bit,_zpg, _and,_zpg, _rol,_zpg, _ill,_non, /* 20 */
  _plp,_imp, _and,_imm, _rol,_acc, _ill,_non, _bit,_abs, _and,_abs, _rol,_abs, _ill,_non,
  _bmi,_rel, _and,_idy, _ill,_non, _ill,_non, _ill,_non, _and,_zpx, _rol,_zpx, _ill,_non, /* 30 */
  _sec,_imp, _and,_aby, _ill,_non, _ill,_non, _ill,_non, _and,_abx, _rol,_abx, _ill,_non,
  _rti,_imp, _eor,_idx, _ill,_non, _ill,_non, _ill,_non, _eor,_zpg, _lsr,_zpg, _ill,_non, /* 40 */
  _pha,_imp, _eor,_imm, _lsr,_acc, _ill,_non, _jmp,_abs, _eor,_abs, _lsr,_abs, _ill,_non,
  _bvc,_rel, _eor,_idy, _ill,_non, _ill,_non, _ill,_non, _eor,_zpx, _lsr,_zpx, _ill,_non, /* 50 */
  _cli,_imp, _eor,_aby, _ill,_non, _ill,_non, _ill,_non, _eor,_abx, _lsr,_abx, _ill,_non,
  _rts,_imp, _adc,_idx, _ill,_non, _ill,_non, _ill,_non, _adc,_zpg, _ror,_zpg, _ill,_non, /* 60 */
  _pla,_imp, _adc,_imm, _ror,_acc, _ill,_non, _jmp,_ind, _adc,_abs, _ror,_abs, _ill,_non,
  _bvs,_rel, _adc,_idy, _ill,_non, _ill,_non, _ill,_non, _adc,_zpx, _ror,_zpx, _ill,_non, /* 70 */
  _sei,_imp, _adc,_aby, _ill,_non, _ill,_non, _ill,_non, _adc,_abx, _ror,_abx, _ill,_non,
  _ill,_non, _sta,_idx, _ill,_non, _ill,_non, _sty,_zpg, _sta,_zpg, _stx,_zpg, _ill,_non, /* 80 */
  _dey,_imp, _ill,_non, _txa,_imp, _ill,_non, _sty,_abs, _sta,_abs, _stx,_abs, _ill,_non,
  _bcc,_rel, _sta,_idy, _ill,_non, _ill,_non, _sty,_zpx, _sta,_zpx, _stx,_zpy, _ill,_non, /* 90 */
  _tya,_imp, _sta,_aby, _txs,_imp, _ill,_non, _ill,_non, _sta,_abx, _ill,_non, _ill,_non,
  _ldy,_imm, _lda,_idx, _ldx,_imm, _ill,_non, _ldy,_zpg, _lda,_zpg, _ldx,_zpg, _ill,_non, /* a0 */
  _tay,_imp, _lda,_imm, _tax,_imp, _ill,_non, _ldy,_abs, _lda,_abs, _ldx,_abs, _ill,_non,
  _bcs,_rel, _lda,_idy, _ill,_non, _ill,_non, _ldy,_zpx, _lda,_zpx, _ldx,_zpy, _ill,_non, /* b0 */
  _clv,_imp, _lda,_aby, _tsx,_imp, _ill,_non, _ldy,_abx, _lda,_abx, _ldx,_aby, _ill,_non,
  _cpy,_imm, _cmp,_idx, _ill,_non, _ill,_non, _cpy,_zpg, _cmp,_zpg, _dec,_zpg, _ill,_non, /* c0 */
  _iny,_imp, _cmp,_imm, _dex,_imp, _ill,_non, _cpy,_abs, _cmp,_abs, _dec,_abs, _ill,_non,
  _bne,_rel, _cmp,_idy, _ill,_non, _ill,_non, _ill,_non, _cmp,_zpx, _dec,_zpx, _ill,_non, /* d0 */
  _cld,_imp, _cmp,_aby, _ill,_non, _ill,_non, _ill,_non, _cmp,_abx, _dec,_abx, _ill,_non,
  _cpx,_imm, _sbc,_idx, _ill,_non, _ill,_non, _cpx,_zpg, _sbc,_zpg, _inc,_zpg, _ill,_non, /* e0 */
  _inx,_imp, _sbc,_imm, _nop,_imp, _ill,_non, _cpx,_abs, _sbc,_abs, _inc,_abs, _ill,_non,
  _beq,_rel, _sbc,_idy, _ill,_non, _ill,_non, _ill,_non, _sbc,_zpx, _inc,_zpx, _ill,_non, /* f0 */
  _sed,_imp, _sbc,_aby, _ill,_non, _ill,_non, _ill,_non, _sbc,_abx, _inc,_abx, _ill,_non
};

static const unsigned char op65c02[512]=
{
  _brk,_imp, _ora,_idx, _ill,_non, _ill,_non, _tsb,_zpg, _ora,_zpg, _asl,_zpg, _ill,_non, /* 00 */
  _php,_imp, _ora,_imm, _asl,_acc, _ill,_non, _tsb,_abs, _ora,_abs, _asl,_abs, _ill,_non,
  _bpl,_rel, _ora,_idy, _ora,_zpi, _ill,_non, _trb,_zpg, _ora,_zpx, _asl,_zpx, _ill,_non, /* 10 */
  _clc,_imp, _ora,_aby, _ina,_imp, _ill,_non, _tsb,_abs, _ora,_abx, _asl,_abx, _ill,_non,
  _jsr,_abs, _and,_idx, _ill,_non, _ill,_non, _bit,_zpg, _and,_zpg, _rol,_zpg, _ill,_non, /* 20 */
  _plp,_imp, _and,_imm, _rol,_acc, _ill,_non, _bit,_abs, _and,_abs, _rol,_abs, _ill,_non,
  _bmi,_rel, _and,_idy, _and,_zpi, _ill,_non, _bit,_zpx, _and,_zpx, _rol,_zpx, _ill,_non, /* 30 */
  _sec,_imp, _and,_aby, _dea,_imp, _ill,_non, _bit,_abx, _and,_abx, _rol,_abx, _ill,_non,
  _rti,_imp, _eor,_idx, _ill,_non, _ill,_non, _ill,_non, _eor,_zpg, _lsr,_zpg, _ill,_non, /* 40 */
  _pha,_imp, _eor,_imm, _lsr,_acc, _ill,_non, _jmp,_abs, _eor,_abs, _lsr,_abs, _ill,_non,
  _bvc,_rel, _eor,_idy, _eor,_zpi, _ill,_non, _ill,_non, _eor,_zpx, _lsr,_zpx, _ill,_non, /* 50 */
  _cli,_imp, _eor,_aby, _phy,_imp, _ill,_non, _ill,_non, _eor,_abx, _lsr,_abx, _ill,_non,
  _rts,_imp, _adc,_idx, _ill,_non, _ill,_non, _stz,_zpg, _adc,_zpg, _ror,_zpg, _ill,_non, /* 60 */
  _pla,_imp, _adc,_imm, _ror,_acc, _ill,_non, _jmp,_ind, _adc,_abs, _ror,_abs, _ill,_non,
  _bvs,_rel, _adc,_idy, _adc,_zpi, _ill,_non, _stz,_zpx, _adc,_zpx, _ror,_zpx, _ill,_non, /* 70 */
  _sei,_imp, _adc,_aby, _ply,_imp, _ill,_non, _jmp,_iax, _adc,_abx, _ror,_abx, _ill,_non,
  _bra,_rel, _sta,_idx, _ill,_non, _ill,_non, _sty,_zpg, _sta,_zpg, _stx,_zpg, _ill,_non, /* 80 */
  _dey,_imp, _bit,_imm, _txa,_imp, _ill,_non, _sty,_abs, _sta,_abs, _stx,_abs, _ill,_non,
  _bcc,_rel, _sta,_idy, _sta,_zpi, _ill,_non, _sty,_zpx, _sta,_zpx, _stx,_zpy, _ill,_non, /* 90 */
  _tya,_imp, _sta,_aby, _txs,_imp, _ill,_non, _stz,_abs, _sta,_abx, _ill,_non, _ill,_non,
  _ldy,_imm, _lda,_idx, _ldx,_imm, _ill,_non, _ldy,_zpg, _lda,_zpg, _ldx,_zpg, _ill,_non, /* a0 */
  _tay,_imp, _lda,_imm, _tax,_imp, _ill,_non, _ldy,_abs, _lda,_abs, _ldx,_abs, _ill,_non,
  _bcs,_rel, _lda,_idy, _lda,_zpi, _ill,_non, _ldy,_zpx, _lda,_zpx, _ldx,_zpy, _ill,_non, /* b0 */
  _clv,_imp, _lda,_aby, _tsx,_imp, _ill,_non, _ldy,_abx, _lda,_abx, _ldx,_aby, _ill,_non,
  _cpy,_imm, _cmp,_idx, _ill,_non, _ill,_non, _cpy,_zpg, _cmp,_zpg, _dec,_zpg, _ill,_non, /* c0 */
  _iny,_imp, _cmp,_imm, _dex,_imp, _ill,_non, _cpy,_abs, _cmp,_abs, _dec,_abs, _ill,_non,
  _bne,_rel, _cmp,_idy, _cmp,_zpi, _ill,_non, _ill,_non, _cmp,_zpx, _dec,_zpx, _ill,_non, /* d0 */
  _cld,_imp, _cmp,_aby, _phx,_imp, _ill,_non, _ill,_non, _cmp,_abx, _dec,_abx, _ill,_non,
  _cpx,_imm, _sbc,_idx, _ill,_non, _ill,_non, _cpx,_zpg, _sbc,_zpg, _inc,_zpg, _ill,_non, /* e0 */
  _inx,_imp, _sbc,_imm, _nop,_imp, _ill,_non, _cpx,_abs, _sbc,_abs, _inc,_abs, _ill,_non,
  _beq,_rel, _sbc,_idy, _sbc,_zpi, _ill,_non, _ill,_non, _sbc,_zpx, _inc,_zpx, _ill,_non, /* f0 */
  _sed,_imp, _sbc,_aby, _plx,_imp, _ill,_non, _ill,_non, _sbc,_abx, _inc,_abx, _ill,_non
};

static const unsigned char op6510[512]=
{
  _brk,_imp, _ora,_idx, _ill,_non, _slo,_idx, _dop,_imp, _ora,_zpg, _asl,_zpg, _slo,_zpg, /* 00 */
  _php,_imp, _ora,_imm, _asl,_acc, _anc,_imm, _top,_imp, _ora,_abs, _asl,_abs, _slo,_abs,
  _bpl,_rel, _ora,_idy, _nop,_imp, _slo,_idy, _dop,_imp, _ora,_zpx, _asl,_zpx, _slo,_zpx, /* 10 */
  _clc,_imp, _ora,_aby, _ill,_non, _slo,_aby, _top,_imp, _ora,_abx, _asl,_abx, _slo,_abx,
  _jsr,_abs, _and,_idx, _ill,_non, _rla,_idx, _bit,_zpg, _and,_zpg, _rol,_zpg, _rla,_zpg, /* 20 */
  _plp,_imp, _and,_imm, _rol,_acc, _anc,_imm, _bit,_abs, _and,_abs, _rol,_abs, _rla,_abs,
  _bmi,_rel, _and,_idy, _ill,_non, _rla,_idy, _dop,_imp, _and,_zpx, _rol,_zpx, _rla,_zpx, /* 30 */
  _sec,_imp, _and,_aby, _nop,_imp, _rla,_aby, _top,_imp, _and,_abx, _rol,_abx, _rla,_abx,
  _rti,_imp, _eor,_idx, _ill,_non, _sre,_idx, _dop,_imp, _eor,_zpg, _lsr,_zpg, _sre,_zpg, /* 40 */
  _pha,_imp, _eor,_imm, _lsr,_acc, _asr,_imm, _jmp,_abs, _eor,_abs, _lsr,_abs, _sre,_abs,
  _bvc,_rel, _eor,_idy, _ill,_non, _sre,_idy, _dop,_imp, _eor,_zpx, _lsr,_zpx, _sre,_zpx, /* 50 */
  _cli,_imp, _eor,_aby, _nop,_imp, _sre,_aby, _top,_imp, _eor,_abx, _lsr,_abx, _sre,_abx,
  _rts,_imp, _adc,_idx, _ill,_non, _rra,_idx, _dop,_imp, _adc,_zpg, _ror,_zpg, _rra,_zpg, /* 60 */
  _pla,_imp, _adc,_imm, _ror,_acc, _arr,_imm, _jmp,_ind, _adc,_abs, _ror,_abs, _rra,_abs,
  _bvs,_rel, _adc,_idy, _ill,_non, _rra,_idy, _dop,_imp, _adc,_zpx, _ror,_zpx, _rra,_zpx, /* 70 */
  _sei,_imp, _adc,_aby, _nop,_imp, _rra,_aby, _top,_imp, _adc,_abx, _ror,_abx, _rra,_abx,
  _dop,_imp, _sta,_idx, _dop,_imp, _sax,_idx, _sty,_zpg, _sta,_zpg, _stx,_zpg, _sax,_zpg, /* 80 */
  _dey,_imp, _dop,_imp, _txa,_imp, _axa,_imm, _sty,_abs, _sta,_abs, _stx,_abs, _sax,_abs,
  _bcc,_rel, _sta,_idy, _ill,_non, _say,_idy, _sty,_zpx, _sta,_zpx, _stx,_zpy, _sax,_zpx, /* 90 */
  _tya,_imp, _sta,_aby, _txs,_imp, _ssh,_aby, _syh,_abx, _sta,_abx, _ill,_non, _sah,_aby,
  _ldy,_imm, _lda,_idx, _ldx,_imm, _lax,_idx, _ldy,_zpg, _lda,_zpg, _ldx,_zpg, _lax,_zpg, /* a0 */
  _tay,_imp, _lda,_imm, _tax,_imp, _lax,_imm, _ldy,_abs, _lda,_abs, _ldx,_abs, _lax,_abs,
  _bcs,_rel, _lda,_idy, _ill,_non, _lax,_idy, _ldy,_zpx, _lda,_zpx, _ldx,_zpy, _lax,_zpx, /* b0 */
  _clv,_imp, _lda,_aby, _tsx,_imp, _ast,_aby, _ldy,_abx, _lda,_abx, _ldx,_aby, _lax,_abx,
  _cpy,_imm, _cmp,_idx, _dop,_imp, _dcp,_idx, _cpy,_zpg, _cmp,_zpg, _dec,_zpg, _dcp,_zpg, /* c0 */
  _iny,_imp, _cmp,_imm, _dex,_imp, _asx,_imm, _cpy,_abs, _cmp,_abs, _dec,_abs, _dcp,_abs,
  _bne,_rel, _cmp,_idy, _ill,_non, _dcp,_idy, _dop,_imp, _cmp,_zpx, _dec,_zpx, _dcp,_zpx, /* d0 */
  _cld,_imp, _cmp,_aby, _nop,_imp, _dcp,_aby, _top,_imp, _cmp,_abx, _dec,_abx, _dcp,_abx,
  _cpx,_imm, _sbc,_idx, _dop,_imp, _isc,_idx, _cpx,_zpg, _sbc,_zpg, _inc,_zpg, _isc,_zpg, /* e0 */
  _inx,_imp, _sbc,_imm, _nop,_imp, _sbc,_imm, _cpx,_abs, _sbc,_abs, _inc,_abs, _isc,_abs,
  _beq,_rel, _sbc,_idy, _ill,_non, _isc,_idy, _dop,_imp, _sbc,_zpx, _inc,_zpx, _isc,_zpx, /* f0 */
  _sed,_imp, _sbc,_aby, _nop,_imp, _isc,_aby, _top,_imp, _sbc,_abx, _inc,_abx, _isc,_abx
};

/*****************************************************************************
 *	Disassemble a single command and return the number of bytes it uses.
 *****************************************************************************/
int Dasm6502(char *buffer, int pc)
{
extern int M6502_Type;
int PC, OP, opc, arg;

	PC = pc;
	OP = RDOP(PC++) << 1;

	switch (M6502_Type)
	{
		case 1:
			opc =op65c02[OP];
            arg = op65c02[OP+1];
			break;
		case 2:
			opc =op6510[OP];
            arg = op6510[OP+1];
            break;
		default:
			opc =op6502[OP];
            arg = op6502[OP+1];
            break;
	}

	switch(arg)
	{
		case _acc:
			sprintf(buffer,"%-5sa", token[opc]);
			break;

		case _imp:
			sprintf(buffer,"%s", token[opc]);
			break;

		case _rel:
			sprintf(buffer,"%-5s$%04X", token[opc], (PC + 1 + (signed char)RDBYTE(PC)) & 0xffff);
			PC+=1;
			break;

		case _imm:
			sprintf(buffer,"%-5s#$%02X", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _zpg:
			sprintf(buffer,"%-5s$%02X", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _zpx:
			sprintf(buffer,"%-5s$%02X,x", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _zpy:
			sprintf(buffer,"%-5s$%02X,y", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _idx:
			sprintf(buffer,"%-5s($%02X,x)", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _idy:
			sprintf(buffer,"%-5s($%02X),y", token[opc], RDBYTE(PC));
			PC+=1;
			break;
		case _zpi:
			sprintf(buffer,"%-5s($%02X)", token[opc], RDBYTE(PC));
            PC+=1;
            break;

		case _abs:
			sprintf(buffer,"%-5s$%04X", token[opc], RDWORD(PC));
			PC+=2;
			break;
		case _abx:
			sprintf(buffer,"%-5s$%04X,x", token[opc], RDWORD(PC));
			PC+=2;
			break;
		case _aby:
			sprintf(buffer,"%-5s$%04X,y", token[opc], RDWORD(PC));
			PC+=2;
			break;
		case _ind:
			sprintf(buffer,"%-5s($%04X)", token[opc], RDWORD(PC));
			PC+=2;
			break;
		case _iax:
			sprintf(buffer,"%-5s($%04X),X", token[opc], RDWORD(PC));
			PC+=2;
            break;

		default:
			sprintf(buffer,"%-5s$%02X", token[opc], OP >> 1);
	}
	return PC - pc;
}

