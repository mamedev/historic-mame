/*
 *   A quick-hack 68(7)05 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 */

#include <string.h>

#ifdef MAME_DEBUG

#include <stdio.h>
#include "cpuintrf.h"
#include "mamedbg.h"
#include "m6805.h"

enum addr_mode {
	_imp=0, 	/* implicit */
	_btr,		/* bit test and relative */
	_bit,		/* bit set/clear */
	_rel,		/* relative */
	_imm,		/* immediate */
	_dir,		/* direct address */
	_ext,		/* extended address */
	_idx,		/* indexed */
	_ix1,		/* indexed + byte offset */
	_ix2		/* indexed + word offset */
};

enum op_names {
	adca=0, adda,	anda,	asl,	asla,	aslx,	asr,	asra,
	asrx,	bcc,	bclr,	bcs,	beq,	bhcc,	bhcs,	bhi,
	bih,	bil,	bita,	bls,	bmc,	bmi,	bms,	bne,
	bpl,	bra,	brclr,	brn,	brset,	bset,	bsr,	clc,
	cli,	clr,	clra,	clrx,	cmpa,	com,	coma,	comx,
	cpx,	dec,	deca,	decx,	eora,	ill,	inc,	inca,
	incx,	jmp,	jsr,	lda,	ldx,	lsr,	lsra,	lsrx,
	neg,	nega,	negx,	nop,	ora,	rol,	rola,	rolx,
	ror,	rora,	rorx,	rsp,	rti,	rts,	sbca,	sec,
	sei,	sta,	stx,	suba,	swi,	tax,	tst,	tsta,
	tstx,	txa
};

static const char *op_name_str[] = {
	"adca", "adda", "anda", "asl",  "asla", "aslx", "asr",  "asra",
	"asrx", "bcc",  "bclr", "bcs",  "beq",  "bhcc", "bhcs", "bhi",
	"bih",  "bil",  "bita", "bls",  "bmc",  "bmi",  "bms",  "bne",
	"bpl",  "bra",  "brclr","brn",  "brset","bset", "bsr",  "clc",
	"cli",  "clr",  "clra", "clrx", "cmpa", "com",  "coma", "comx",
	"cpx",  "dec",  "deca", "decx", "eora", "*ill", "inc",  "inca",
	"incx", "jmp",  "jsr",  "lda",  "ldx",  "lsr",  "lsra", "lsrx",
	"neg",  "nega", "negx", "nop",  "ora",  "rol",  "rola", "rolx",
	"ror",  "rora", "rorx", "rsp",  "rti",  "rts",  "sbca", "sec",
	"sei",  "sta",  "stx",  "suba", "swi",  "tax",  "tst",  "tsta",
	"tstx", "txa"
};

#define _0	0,0
#define _jr 0,EA_REL_PC
#define _jp 0,EA_REL_PC
#define _rm EA_UINT8,EA_MEM_RD
#define _wm EA_UINT8,EA_MEM_WR
#define _rw EA_UINT8,EA_MEM_RDWR

const unsigned char disasm[0x100][4] = {
	{brset,_btr,_rm}, {brclr,_btr,_rm}, {brset,_btr,_rm}, {brclr,_btr,_rm}, /* 00 */
	{brset,_btr,_rm}, {brclr,_btr,_rm}, {brset,_btr,_rm}, {brclr,_btr,_rm},
	{brset,_btr,_rm}, {brclr,_btr,_rm}, {brset,_btr,_rm}, {brclr,_btr,_rm},
	{brset,_btr,_rm}, {brclr,_btr,_rm}, {brset,_btr,_rm}, {brclr,_btr,_rm},
	{bset, _bit,_wm}, {bclr, _bit,_wm}, {bset, _bit,_wm}, {bclr, _bit,_wm}, /* 10 */
	{bset, _bit,_wm}, {bclr, _bit,_wm}, {bset, _bit,_wm}, {bclr, _bit,_wm},
	{bset, _bit,_wm}, {bclr, _bit,_wm}, {bset, _bit,_wm}, {bclr, _bit,_wm},
	{bset, _bit,_wm}, {bclr, _bit,_wm}, {bset, _bit,_wm}, {bclr, _bit,_wm},
	{bra,  _rel,_jr}, {brn,  _rel,_jr}, {bhi,  _rel,_jr}, {bls,  _rel,_jr}, /* 20 */
	{bcc,  _rel,_jr}, {bcs,  _rel,_jr}, {bne,  _rel,_jr}, {beq,  _rel,_jr},
	{bhcc, _rel,_jr}, {bhcs, _rel,_jr}, {bpl,  _rel,_jr}, {bmi,  _rel,_jr},
	{bmc,  _rel,_jr}, {bms,  _rel,_jr}, {bil,  _rel,_jr}, {bih,  _rel,_jr},
	{neg,  _dir,_rw}, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {com,  _dir,_rw}, /* 30 */
	{lsr,  _dir,_rw}, {ill,  _imp,_0 }, {ror,  _dir,_rw}, {asr,  _dir,_rw},
	{asl,  _dir,_rw}, {rol,  _dir,_rw}, {dec,  _dir,_rw}, {ill,  _imp,_0 },
	{inc,  _dir,_rw}, {tst,  _dir,_rm}, {ill,  _imp,_0 }, {clr,  _dir,_wm},
	{nega, _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {coma, _imp,_0 }, /* 40 */
	{lsra, _imp,_0 }, {ill,  _imp,_0 }, {rora, _imp,_0 }, {asra, _imp,_0 },
	{asla, _imp,_0 }, {rola, _imp,_0 }, {deca, _imp,_0 }, {ill,  _imp,_0 },
	{inca, _imp,_0 }, {tsta, _imp,_0 }, {ill,  _imp,_0 }, {clra, _imp,_0 },
	{negx, _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {comx, _imp,_0 }, /* 50 */
	{lsrx, _imp,_0 }, {ill,  _imp,_0 }, {rorx, _imp,_0 }, {asrx, _imp,_0 },
	{aslx, _imp,_0 }, {rolx, _imp,_0 }, {decx, _imp,_0 }, {ill,  _imp,_0 },
	{incx, _imp,_0 }, {tstx, _imp,_0 }, {ill,  _imp,_0 }, {clrx, _imp,_0 },
	{neg,  _ix1,_rw}, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {com,  _ix1,_rw}, /* 60 */
	{lsr,  _ix1,_rw}, {ill,  _imp,_0 }, {ror,  _ix1,_rw}, {asr,  _ix1,_rw},
	{asl,  _ix1,_rw}, {rol,  _ix1,_rw}, {dec,  _ix1,_rw}, {ill,  _imp,_0 },
	{inc,  _ix1,_rw}, {tst,  _ix1,_rm}, {jmp,  _ix1,_jp}, {clr,  _ix1,_wm},
	{neg,  _idx,_rw}, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {com,  _idx,_rw}, /* 70 */
	{lsr,  _idx,_rw}, {ill,  _imp,_0 }, {ror,  _idx,_rw}, {asr,  _idx,_rw},
	{asl,  _idx,_rw}, {rol,  _idx,_rw}, {dec,  _idx,_rw}, {ill,  _imp,_0 },
	{inc,  _idx,_rw}, {tst,  _idx,_rm}, {jmp,  _idx,_jp}, {clr,  _idx,_wm},
	{rti,  _imp,_0 }, {rts,  _imp,_0 }, {ill,  _imp,_0 }, {swi,  _imp,_0 }, /* 80 */
	{ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 },
	{ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 },
	{ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 },
	{ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, /* 90 */
	{ill,  _imp,_0 }, {ill,  _imp,_0 }, {ill,  _imp,_0 }, {tax,  _imp,_0 },
	{clc,  _imp,_0 }, {sec,  _imp,_0 }, {cli,  _imp,_0 }, {sei,  _imp,_0 },
	{rsp,  _imp,_0 }, {nop,  _imp,_0 }, {ill,  _imp,_0 }, {txa,  _imp,_0 },
	{suba, _imm,_0 }, {cmpa, _imm,_0 }, {sbca, _imm,_0 }, {cpx,  _imm,_0 }, /* a0 */
	{anda, _imm,_0 }, {bita, _imm,_0 }, {lda,  _imm,_0 }, {ill,  _imp,_0 },
	{eora, _imm,_0 }, {adca, _imm,_0 }, {ora,  _imm,_0 }, {adda, _imm,_0 },
	{ill,  _imp,_0 }, {bsr,  _rel,_jr}, {ldx,  _imm,_0 }, {ill,  _imp,_0 },
	{suba, _dir,_rm}, {cmpa, _dir,_rm}, {sbca, _dir,_rm}, {cpx,  _dir,_rm}, /* b0 */
	{anda, _dir,_rm}, {bita, _dir,_rm}, {lda,  _dir,_rm}, {sta,  _dir,_wm},
	{eora, _dir,_rm}, {adca, _dir,_rm}, {ora,  _dir,_rm}, {adda, _dir,_rm},
	{jmp,  _dir,_rm}, {jsr,  _dir,_jp}, {ldx,  _dir,_rm}, {stx,  _dir,_wm},
	{suba, _ext,_rm}, {cmpa, _ext,_rm}, {sbca, _ext,_rm}, {cpx,  _ext,_rm}, /* c0 */
	{anda, _ext,_rm}, {bita, _ext,_rm}, {lda,  _ext,_rm}, {sta,  _ext,_wm},
	{eora, _ext,_rm}, {adca, _ext,_rm}, {ora,  _ext,_rm}, {adda, _ext,_rm},
	{jmp,  _ext,_jp}, {jsr,  _ext,_jp}, {ldx,  _ext,_rm}, {stx,  _ext,_wm},
	{suba, _ix2,_rm}, {cmpa, _ix2,_rm}, {sbca, _ix2,_rm}, {cpx,  _ix2,_rm}, /* d0 */
	{anda, _ix2,_rm}, {bita, _ix2,_rm}, {lda,  _ix2,_rm}, {sta,  _ix2,_wm},
	{eora, _ix2,_rm}, {adca, _ix2,_rm}, {ora,  _ix2,_rm}, {adda, _ix2,_rm},
	{jmp,  _ix2,_jp}, {jsr,  _ix2,_jp}, {ldx,  _ix2,_rm}, {stx,  _ix2,_wm},
	{suba, _ix1,_rm}, {cmpa, _ix1,_rm}, {sbca, _ix1,_rm}, {cpx,  _ix1,_rm}, /* e0 */
	{anda, _ix1,_rm}, {bita, _ix1,_rm}, {lda,  _ix1,_rm}, {sta,  _ix1,_wm},
	{eora, _ix1,_rm}, {adca, _ix1,_rm}, {ora,  _ix1,_rm}, {adda, _ix1,_rm},
	{jmp,  _ix1,_jp}, {jsr,  _ix1,_jp}, {ldx,  _ix1,_rm}, {stx,  _ix1,_wm},
	{suba, _idx,_rm}, {cmpa, _idx,_rm}, {sbca, _idx,_rm}, {cpx,  _idx,_rm}, /* f0 */
	{anda, _idx,_rm}, {bita, _idx,_rm}, {lda,  _idx,_rm}, {sta,  _idx,_wm},
	{eora, _idx,_rm}, {adca, _idx,_rm}, {ora,  _idx,_rm}, {adda, _idx,_rm},
	{jmp,  _idx,_jp}, {jsr,  _idx,_jp}, {ldx,  _idx,_rm}, {stx,  _idx,_wm}
};

#if 0
static char *opcode_strings[0x0100] =
{
	"brset0", 	"brclr0", 	"brset1", 	"brclr1", 	"brset2", 	"brclr2",	"brset3",	"brclr3",		/*00*/
	"brset4",	"brclr4",	"brset5",	"brclr5",	"brset6",	"brclr6",	"brset7",	"brclr7",
	"bset0",	"bclr0",	"bset1", 	"bclr1", 	"bset2", 	"bclr2", 	"bset3",	"bclr3",		/*10*/
	"bset4", 	"bclr4",	"bset5", 	"bclr5",	"bset6", 	"bclr6", 	"bset7", 	"bclr7",
	"bra",		"brn",		"bhi",		"bls",		"bcc",		"bcs",		"bne",		"beq",			/*20*/
	"bhcc",		"bhcs",		"bpl",		"bmi",		"bmc",		"bms",		"bil",		"bih",
	"neg_di",   "illegal",  "illegal",  "com_di",   "lsr_di",   "illegal",  "ror_di",   "asr_di",       /*30*/
	"asl_di",	"rol_di",	"dec_di",	"illegal", 	"inc_di",	"tst_di",	"illegal", 	"clr_di",
	"nega",		"illegal", 	"illegal", 	"coma",		"lsra",		"illegal", 	"rora",		"asra",			/*40*/
	"asla",		"rola",		"deca",		"illegal", 	"inca",		"tsta",		"illegal", 	"clra",
	"negx",		"illegal", 	"illegal", 	"comx",		"lsrx",		"illegal", 	"rorx",		"asrx",			/*50*/
	"aslx",		"rolx",		"decx",		"illegal", 	"incx",		"tstx",		"illegal", 	"clrx",
	"neg_ix1",	"illegal", 	"illegal", 	"com_ix1",	"lsr_ix1",	"illegal", 	"ror_ix1",	"asr_ix1",		/*60*/
	"asl_ix1",	"rol_ix1",	"dec_ix1",	"illegal", 	"inc_ix1",	"tst_ix1",	"jmp_ix1",	"clr_ix1",
	"neg_ix",	"illegal", 	"illegal", 	"com_ix",	"lsr_ix",	"illegal", 	"ror_ix",	"asr_ix",		/*70*/
	"asl_ix",	"rol_ix",	"dec_ix",	"illegal", 	"inc_ix",	"tst_ix",	"jmp_ix",	"clr_ix",
	"rti",		"rts",		"illegal",	"swi",		"illegal",	"illegal",	"illegal",	"illegal",		/*80*/
	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",
	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"tax",			/*90*/
	"clc",		"sec",		"cli",		"sei",		"rsp",		"nop",		"illegal",	"txa",
	"suba_im",	"cmpa_im",	"sbca_im",	"cpx_im", 	"anda_im",	"bita_im",	"lda_im",	"illegal",		/*A0*/
	"eora_im",	"adca_im",	"ora_im",	"adda_im",	"illegal",	"bsr",		"ldx_im",	"illegal",
	"suba_di",	"cmpa_di",	"sbca_di",	"cpx_di", 	"anda_di",	"bita_di",	"lda_di",	"sta_di",		/*B0*/
	"eora_di",	"adca_di",	"ora_di",	"adda_di",	"jmp_di",	"jsr_di",	"ldx_di",	"stx_di",
	"suba_ex",	"cmpa_ex",	"sbca_ex",	"cpx_ex", 	"anda_ex",	"bita_ex",	"lda_ex",	"sta_ex",		/*C0*/
	"eora_ex",	"adca_ex",	"ora_ex",	"adda_ex",	"jmp_ex",	"jsr_ex",	"ldx_ex",	"stx_ex",
	"suba_ix2",	"cmpa_ix2",	"sbca_ix2",	"cpx_ix2", 	"anda_ix2",	"bita_ix2",	"lda_ix2",	"sta_ix2",		/*D0*/
	"eora_ix2",	"adca_ix2",	"ora_ix2",	"adda_ix2",	"jmp_ix2",	"jsr_ix2",	"ldx_ix2",	"stx_ix2",
	"suba_ix1",	"cmpa_ix1",	"sbca_ix1",	"cpx_ix1", 	"anda_ix1",	"bita_ix1",	"lda_ix1",	"sta_ix1",		/*E0*/
	"eora_ix1",	"adca_ix1",	"ora_ix1",	"adda_ix1",	"jmp_ix1",	"jsr_ix1",	"ldx_ix1",	"stx_ix1",
	"suba_ix",	"cmpa_ix",	"sbca_ix",	"cpx_ix", 	"anda_ix",	"bita_ix",	"lda_ix",	"sta_ix",		/*F0*/
	"eora_ix",	"adca_ix",	"ora_ix",	"adda_ix",	"jmp_ix",	"jsr_ix",	"ldx_ix",	"stx_ix"
};
#endif

unsigned Dasm6805 (char *buf, unsigned pc)
{
	const char *sym1, *sym2;
    int code, bit;
	unsigned addr, ea, size, access;

	code = cpu_readop(pc);
	size = disasm[code][2];
	access = disasm[code][3];

    buf += sprintf(buf, "%-5s", op_name_str[disasm[code][0]]);

	switch( disasm[code][1] )
	{
	case _btr:	/* bit test and relative branch */
		bit = (code >> 1) & 7;
		ea = cpu_readop_arg(pc+1);
		sym1 = set_ea_info(1, ea, EA_UINT8, EA_MEM_RD);
		sym2 = set_ea_info(0, pc + 3, (INT8)cpu_readop_arg(pc+2), EA_REL_PC);
		sprintf (buf, "%d,%s,%s", bit, sym1, sym2);
		return 3;
	case _bit:	/* bit test */
		bit = (code >> 1) & 7;
		ea = cpu_readop_arg(pc+1);
		sym1 = set_ea_info(1, ea, EA_UINT8, EA_MEM_RD);
		sprintf (buf, "%d,%s", bit, sym1);
		return 2;
	case _rel:	/* relative */
		sym1 = set_ea_info(0, pc + 2, (INT8)cpu_readop_arg(pc+1), access);
		sprintf (buf, "%s", sym1);
		return 2;
	case _imm:	/* immediate */
		sym1 = set_ea_info(0, cpu_readop_arg(pc+1), EA_UINT8, EA_VALUE);
		sprintf (buf, "#%s", sym1);
		return 2;
	case _dir:	/* direct (zero page address) */
		addr = cpu_readop_arg(pc+1);
		ea = addr;
		sym1 = set_ea_info(1, ea, EA_UINT8, EA_MEM_RD);
        sprintf (buf, "%s", sym1);
		return 2;
	case _ext:	/* extended (16 bit address) */
		addr = (cpu_readop_arg(pc+1) << 8) + cpu_readop_arg(pc+2);
		ea = addr;
		sym1 = set_ea_info(1, ea, EA_UINT16, EA_MEM_RD);
		sprintf (buf, "%s", sym1);
		return 3;
	case _idx:	/* indexed */
		ea = m6805_get_reg(M6805_X);
		set_ea_info(0, ea, EA_UINT8, EA_MEM_RD);
		sprintf (buf, "(x)");
		return 1;
	case _ix1:	/* indexed + byte (zero page) */
		addr = cpu_readop_arg(pc+1);
		ea = (addr + cpu_get_reg(M6805_X)) & 0xffff;
        sym1 = set_ea_info(0, ea, size, access);
		sprintf (buf, "(x+$%02x)", addr);
		return 2;
	case _ix2:	/* indexed + word (16 bit address) */
		addr = (cpu_readop_arg(pc+1) << 8) + cpu_readop_arg(pc+2);
		ea = (addr + cpu_get_reg(M6805_X)) & 0xffff;
		sym1 = set_ea_info(0, ea, size, access);
		sprintf (buf, "(x+$%04x)", addr);
		return 3;
    default:    /* implicit */
		return 1;
    }
}

#endif
