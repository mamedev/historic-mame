/*
 *   A quick-hack 6803/6808 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 * History:
 * 990327 HJB
 * Added code to support the debugger set_ea_info function
 * 990314 HJB
 * The disassembler knows about valid opcodes for M6800/1/2/3/8 and HD63701.
 * 990302 HJB
 * Changed the string array into a table of opcode names (or tokens) and
 * argument types. This second try should give somewhat better results.
 * Named the undocumented HD63701YO opcodes $12 and $13 'asx1' and 'asx2',
 * since 'add contents of stack to x register' is what they do.
 *
 */

#include <string.h>
#include <stdio.h>
#include "driver.h"
#include "osd_dbg.h"
#include "m6800.h"

#ifdef MAME_DEBUG

enum addr_mode {
	_inh=0, 	/* inherent */
	_rel,		/* relative */
	_imm,		/* immediate (byte or word) */
	_dir,		/* direct address */
	_imd,		/* HD63701YO: immediate, direct address */
	_ext,		/* extended address */
	_idx,		/* x + byte offset */
	_imx,		/* HD63701YO: immediate, x + byte offset */
	_sx1		/* HD63701YO: undocumented opcodes: byte from (s+1) */
};

enum op_names {
	aba=0,	abx,	adca,	adcb,	adda,	addb,	addd,	aim,
	anda,	andb,	asl,	asla,	aslb,	asld,	asr,	asra,
	asrb,	bcc,	bcs,	beq,	bge,	bgt,	bhi,	bita,
	bitb,	ble,	bls,	blt,	bmi,	bne,	bpl,	bra,
	brn,	bsr,	bvc,	bvs,	cba,	clc,	cli,	clr,
	clra,	clrb,	clv,	cmpa,	cmpb,	cmpx,	com,	coma,
	comb,	daa,	dec,	deca,	decb,	des,	dex,	eim,
	eora,	eorb,	ill,	inc,	inca,	incb,	ins,	inx,
	jmp,	jsr,	lda,	ldb,	ldd,	lds,	ldx,	lsr,
	lsra,	lsrb,	lsrd,	mul,	neg,	nega,	negb,	nop,
	oim,	ora,	orb,	psha,	pshb,	pshx,	pula,	pulb,
	pulx,	rol,	rola,	rolb,	ror,	rora,	rorb,	rti,
	rts,	sba,	sbca,	sbcb,	sec,	sev,	sta,	stb,
	std,	sti,	sts,	stx,	suba,	subb,	subd,	swi,
	sync,	tab,	tap,	tba,	tim,	tpa,	tst,	tsta,
	tstb,	tsx,	txs,	asx1,	asx2,	xgdx
};

static const char *op_name_str[] = {
	"aba",   "abx",   "adca",  "adcb",  "adda",  "addb",  "addd",  "aim",
	"anda",  "andb",  "asl",   "asla",  "aslb",  "asld",  "asr",   "asra",
	"asrb",  "bcc",   "bcs",   "beq",   "bge",   "bgt",   "bhi",   "bita",
	"bitb",  "ble",   "bls",   "blt",   "bmi",   "bne",   "bpl",   "bra",
	"brn",   "bsr",   "bvc",   "bvs",   "cba",   "clc",   "cli",   "clr",
	"clra",  "clrb",  "clv",   "cmpa",  "cmpb",  "cmpx",  "com",   "coma",
	"comb",  "daa",   "dec",   "deca",  "decb",  "des",   "dex",   "eim",
	"eora",  "eorb",  "illegal","inc",   "inca",  "incb",  "ins",   "inx",
	"jmp",   "jsr",   "lda",   "ldb",   "ldd",   "lds",   "ldx",   "lsr",
	"lsra",  "lsrb",  "lsrd",  "mul",   "neg",   "nega",  "negb",  "nop",
	"oim",   "ora",   "orb",   "psha",  "pshb",  "pshx",  "pula",  "pulb",
	"pulx",  "rol",   "rola",  "rolb",  "ror",   "rora",  "rorb",  "rti",
	"rts",   "sba",   "sbca",  "sbcb",  "sec",   "sev",   "sta",   "stb",
	"std",   "sti",   "sts",   "stx",   "suba",  "subb",  "subd",  "swi",
	"sync",  "tab",   "tap",   "tba",   "tim",   "tpa",   "tst",   "tsta",
	"tstb",  "tsx",   "txs",   "asx1",  "asx2",  "xgdx"
};


/* Some really short names for the EA access modes */
#define _0	EA_NONE
#define _JP EA_ABS_PC
#define _JR EA_REL_PC
#define _RM EA_MEM_RD
#define _WM EA_MEM_WR
#define _RW EA_MEM_RDWR

/* and the data element sizes */
#define _B  EA_UINT8
#define _W	EA_UINT16

/*
 * This table defines the opcodes:
 * byte meaning
 * 0	token (menmonic)
 * 1	addressing mode
 * 2	EA access mode
 * 3	EA access size
 * 4	invalid opcode for 1:6800/6802/6808, 2:6801/6803, 4:HD63701
 */

static UINT8 table[0x100][5] = {
	{ill, _inh,_0 ,0 ,7}, {nop, _inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, /* 00 */
	{lsrd,_inh,_0 ,0 ,1}, {asld,_inh,_0 ,0 ,1}, {tap, _inh,_0 ,0 ,0}, {tpa, _inh,_0 ,0 ,0},
	{inx, _inh,_0 ,0 ,0}, {dex, _inh,_0 ,0 ,0}, {clv, _inh,_0 ,0 ,0}, {sev, _inh,_0 ,0 ,0},
	{clc, _inh,_0 ,0 ,0}, {sec, _inh,_0 ,0 ,0}, {cli, _inh,_0 ,0 ,0}, {sti, _inh,_0 ,0 ,0},
	{sba, _inh,_0 ,0 ,0}, {cba, _inh,_0 ,0 ,0}, {asx1,_sx1,_RM,_B,0}, {asx2,_sx1,_RM,_B,0}, /* 10 */
	{ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, {tab, _inh,_0 ,0 ,0}, {tba, _inh,_0 ,0 ,0},
	{xgdx,_inh,_0 ,0 ,3}, {daa, _inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {aba, _inh,_0 ,0 ,0},
	{ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7},
	{bra, _rel,_JR,0 ,0}, {brn, _rel,_JR,0 ,0}, {bhi, _rel,_JR,0 ,0}, {bls, _rel,_JR,0 ,0}, /* 20 */
	{bcc, _rel,_JR,0 ,0}, {bcs, _rel,_JR,0 ,0}, {bne, _rel,_JR,0 ,0}, {beq, _rel,_JR,0 ,0},
	{bvc, _rel,_JR,0 ,0}, {bvs, _rel,_JR,0 ,0}, {bpl, _rel,_JR,0 ,0}, {bmi, _rel,_JR,0 ,0},
	{bge, _rel,_JR,0 ,0}, {blt, _rel,_JR,0 ,0}, {bgt, _rel,_JR,0 ,0}, {ble, _rel,_JR,0 ,0},
	{tsx, _inh,_0 ,0 ,0}, {ins, _inh,_0 ,0 ,0}, {pula,_inh,_0 ,0 ,0}, {pulb,_inh,_0 ,0 ,0}, /* 30 */
	{des, _inh,_0 ,0 ,0}, {txs, _inh,_0 ,0 ,0}, {psha,_inh,_0 ,0 ,0}, {pshb,_inh,_0 ,0 ,0},
	{pulx,_inh,_0 ,0 ,1}, {rts, _inh,_0 ,0 ,0}, {abx, _inh,_0 ,0 ,1}, {rti, _inh,_0 ,0 ,0},
	{pshx,_inh,_0 ,0 ,1}, {mul, _inh,_0 ,0 ,1}, {sync,_inh,_0 ,0 ,0}, {swi, _inh,_0 ,0 ,0},
	{nega,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, {coma,_inh,_0 ,0 ,0}, /* 40 */
	{lsra,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {rora,_inh,_0 ,0 ,0}, {asra,_inh,_0 ,0 ,0},
	{asla,_inh,_0 ,0 ,0}, {rola,_inh,_0 ,0 ,0}, {deca,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7},
	{inca,_inh,_0 ,0 ,0}, {tsta,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {clra,_inh,_0 ,0 ,0},
	{negb,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {ill, _inh,_0 ,0 ,7}, {comb,_inh,_0 ,0 ,0}, /* 50 */
	{lsrb,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {rorb,_inh,_0 ,0 ,0}, {asrb,_inh,_0 ,0 ,0},
	{aslb,_inh,_0 ,0 ,0}, {rolb,_inh,_0 ,0 ,0}, {decb,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7},
	{incb,_inh,_0 ,0 ,0}, {tstb,_inh,_0 ,0 ,0}, {ill, _inh,_0 ,0 ,7}, {clrb,_inh,_0 ,0 ,0},
	{neg, _idx,_RW,_B,0}, {aim, _imx,_RW,_B,3}, {oim, _imx,_RW,_B,3}, {com, _idx,_RW,_B,0}, /* 60 */
	{lsr, _idx,_RW,_B,0}, {eim, _imx,_RW,_B,3}, {ror, _idx,_RW,_B,0}, {asr, _idx,_RW,_B,0},
	{asl, _idx,_RW,_B,0}, {rol, _idx,_RW,_B,0}, {dec, _idx,_RW,_B,0}, {tim, _imx,_RM,_B,3},
	{inc, _idx,_RW,_B,0}, {tst, _idx,_RM,_B,0}, {jmp, _idx,_JP,0 ,0}, {clr, _idx,_WM,_B,0},
	{neg, _ext,_RW,_B,0}, {aim, _imd,_RW,_B,3}, {oim, _imd,_RW,_B,3}, {com, _ext,_RW,_B,0}, /* 70 */
	{lsr, _ext,_RW,_B,0}, {eim, _imd,_RW,_B,3}, {ror, _ext,_RW,_B,0}, {asr, _ext,_RW,_B,0},
	{asl, _ext,_RW,_B,0}, {rol, _ext,_RW,_B,0}, {dec, _ext,_RW,_B,0}, {tim, _imd,_RM,_B,3},
	{inc, _ext,_RW,_B,0}, {tst, _ext,_RM,_B,0}, {jmp, _ext,_JP,0 ,0}, {clr, _ext,_WM,_B,0},
	{suba,_imm,_0 ,_B,0}, {cmpa,_imm,_0 ,_B,0}, {sbca,_imm,_0 ,_B,0}, {subd,_imm,_0 ,_W,1}, /* 80 */
	{anda,_imm,_0 ,_B,0}, {bita,_imm,_0 ,_B,0}, {lda, _imm,_0 ,_B,0}, {sta, _imm,_0 ,_B,0},
	{eora,_imm,_0 ,_B,0}, {adca,_imm,_0 ,_B,0}, {ora, _imm,_0 ,_B,0}, {adda,_imm,_0 ,_B,0},
	{cmpx,_imm,_0 ,_W,0}, {bsr, _rel,_JR,0 ,0}, {lds, _imm,_0 ,_W,0}, {sts, _imm,_0 ,_W,0},
	{suba,_dir,_RM,_B,0}, {cmpa,_dir,_RM,_B,0}, {sbca,_dir,_RM,_B,0}, {subd,_dir,_RM,_W,1}, /* 90 */
	{anda,_dir,_RM,_B,0}, {bita,_dir,_RM,_B,0}, {lda, _dir,_RM,_B,0}, {sta, _dir,_WM,_B,0},
	{eora,_dir,_RM,_B,0}, {adca,_dir,_RM,_B,0}, {ora, _dir,_RM,_B,0}, {adda,_dir,_RM,_B,0},
	{cmpx,_dir,_RM,_W,0}, {jsr, _dir,_JP,0 ,0}, {lds, _dir,_RM,_W,0}, {sts, _dir,_WM,_W,0},
	{suba,_idx,_RM,_B,0}, {cmpa,_idx,_RM,_B,0}, {sbca,_idx,_RM,_B,0}, {subd,_idx,_RM,_W,1}, /* a0 */
	{anda,_idx,_RM,_B,0}, {bita,_idx,_RM,_B,0}, {lda, _idx,_RM,_B,0}, {sta, _idx,_WM,_B,0},
	{eora,_idx,_RM,_B,0}, {adca,_idx,_RM,_B,0}, {ora, _idx,_RM,_B,0}, {adda,_idx,_RM,_B,0},
	{cmpx,_idx,_RM,_W,0}, {jsr, _idx,_JP,0 ,0}, {lds, _idx,_RM,_W,0}, {sts, _idx,_WM,_W,0},
	{suba,_ext,_RM,_B,0}, {cmpa,_ext,_RM,_B,0}, {sbca,_ext,_RM,_B,0}, {subd,_ext,_RM,_W,1}, /* b0 */
	{anda,_ext,_RM,_B,0}, {bita,_ext,_RM,_B,0}, {lda, _ext,_RM,_B,0}, {sta, _ext,_WM,_B,0},
	{eora,_ext,_RM,_B,0}, {adca,_ext,_RM,_B,0}, {ora, _ext,_RM,_B,0}, {adda,_ext,_RM,_B,0},
	{cmpx,_ext,_RM,_W,0}, {jsr, _ext,_JP,0 ,0}, {lds, _ext,_RM,_W,0}, {sts, _ext,_WM,_W,0},
	{subb,_imm,_0 ,0 ,0}, {cmpb,_imm,_0 ,0 ,0}, {sbcb,_imm,_0 ,0 ,0}, {addd,_imm,_0 ,0 ,1}, /* c0 */
	{andb,_imm,_0 ,0 ,0}, {bitb,_imm,_0 ,0 ,0}, {ldb, _imm,_0 ,0 ,0}, {stb, _imm,_0 ,0 ,0},
	{eorb,_imm,_0 ,0 ,0}, {adcb,_imm,_0 ,0 ,0}, {orb, _imm,_0 ,0 ,0}, {addb,_imm,_0 ,0 ,0},
	{ldd, _imm,_0 ,0 ,1}, {std, _imm,_0 ,0 ,1}, {ldx, _imm,_0 ,0 ,0}, {stx, _imm,_0 ,0 ,0},
	{subb,_dir,_RM,_B,0}, {cmpb,_dir,_RM,_B,0}, {sbcb,_dir,_RM,_B,0}, {addd,_dir,_RM,_W,1}, /* d0 */
	{andb,_dir,_RM,_B,0}, {bitb,_dir,_RM,_B,0}, {ldb, _dir,_RM,_B,0}, {stb, _dir,_WM,_B,0},
	{eorb,_dir,_RM,_B,0}, {adcb,_dir,_RM,_B,0}, {orb, _dir,_RM,_B,0}, {addb,_dir,_RM,_B,0},
	{ldd, _dir,_RM,_W,1}, {std, _dir,_RM,_W,1}, {ldx, _dir,_RM,_W,0}, {stx, _dir,_WM,_W,0},
	{subb,_idx,_RM,_B,0}, {cmpb,_idx,_RM,_B,0}, {sbcb,_idx,_RM,_B,0}, {addd,_idx,_RM,_W,1}, /* e0 */
	{andb,_idx,_RM,_B,0}, {bitb,_idx,_RM,_B,0}, {ldb, _idx,_RM,_B,0}, {stb, _idx,_WM,_B,0},
	{eorb,_idx,_RM,_B,0}, {adcb,_idx,_RM,_B,0}, {orb, _idx,_RM,_B,0}, {addb,_idx,_RM,_B,0},
	{ldd, _idx,_RM,_W,1}, {std, _idx,_RM,_W,1}, {ldx, _idx,_RM,_W,0}, {stx, _idx,_WM,_W,0},
	{subb,_ext,_RM,_B,0}, {cmpb,_ext,_RM,_B,0}, {sbcb,_ext,_RM,_B,0}, {addd,_ext,_RM,_W,1}, /* f0 */
	{andb,_ext,_RM,_B,0}, {bitb,_ext,_RM,_B,0}, {ldb, _ext,_RM,_B,0}, {stb, _ext,_WM,_B,0},
	{eorb,_ext,_RM,_B,0}, {adcb,_ext,_RM,_B,0}, {orb, _ext,_RM,_B,0}, {addb,_ext,_RM,_B,0},
	{ldd, _ext,_RM,_W,1}, {std, _ext,_RM,_W,1}, {ldx, _ext,_RM,_W,0}, {stx, _ext,_WM,_W,0}
};

/* some macros to keep things short */
#define OP      cpu_readop(pc)
#define ARG1    cpu_readop_arg(pc+1)
#define ARG2    cpu_readop_arg(pc+2)
#define ARGW	(cpu_readop_arg(pc+1)<<8) + cpu_readop_arg(pc+2)

unsigned Dasm680x (int subtype, char *buf, unsigned pc)
{
	int invalid_mask;
	int code = cpu_readop(pc);
	const char *opstr, *symbol, *symbol2;
	UINT8 opcode, args, access, size, invalid;

	switch( subtype )
	{
		case 6800: case 6802: case 6808:
			invalid_mask = 1;
			break;
		case 6801: case 6803:
			invalid_mask = 2;
			break;
		default:
			invalid_mask = 4;
	}

	opcode = table[code][0];
	args = table[code][1];
	access = table[code][2];
	size = table[code][3];
	invalid = table[code][4];

	if ( invalid & invalid_mask )	/* invalid for this cpu type ? */
	{
		strcpy(buf, "illegal");
		return 1;
	}

	buf += sprintf(buf, "%-5s", op_name_str[opcode]);

	switch( args )
	{
        case _rel:  /* relative */
			symbol = set_ea_info( 0, pc, ARG1+2, access );
			sprintf (buf, "%s", symbol);
			return 2;
		case _imm:	/* immediate (byte or word) */
			if( size == _B )
			{
				symbol = set_ea_info( 0, ARG1, EA_UINT8, EA_VALUE );
				sprintf (buf, "#%s", symbol);
                return 2;
			}
			symbol = set_ea_info( 0, ARGW, EA_UINT16, EA_VALUE );
			sprintf (buf, "#%s", symbol);
			return 3;
        case _idx:  /* indexed + byte offset */
			symbol = set_ea_info( 0, (m6800_get_reg(M6800_X) + ARG1), size, access);
            sprintf (buf, "(x+$%02x)", ARG1 );
            return 2;
        case _imx:  /* immediate, indexed + byte offset */
			symbol = set_ea_info( 1, ARG1, EA_UINT8, EA_VALUE);
			symbol2 = set_ea_info( 0, (m6800_get_reg(M6800_X) + ARG2), EA_UINT8, access);
			sprintf (buf, "#%s,(x+$%02x)", symbol, ARG2 );
			return 3;
		case _dir:	/* direct address */
			symbol = set_ea_info( 1, ARG1, EA_UINT8, access);
			sprintf (buf, "%s", symbol );
			return 2;
		case _imd:	/* immediate, direct address */
            symbol = set_ea_info( 1, ARG1, EA_UINT8, EA_VALUE );
			symbol2 = set_ea_info( 0, ARG2, EA_UINT8, access);
			sprintf (buf, "#%s,%s", symbol, symbol2);
            return 3;
		case _ext:	/* extended address */
			symbol = set_ea_info( 0, ARGW, EA_UINT16, access);
			sprintf (buf, "%s", symbol);
			return 3;
		case _sx1:	/* byte from address (s + 1) */
			symbol = set_ea_info( 0, m6800_get_reg(M6800_S), EA_UINT16, access);
            sprintf (buf, "(s+1)");
            return 1;
        default:
			return 1;
	}
}

#endif
