/*
 *   A quick-hack 6803/6808 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 * History:
 * 990314 HJB
 * Different Dasm functions for 6800/2/3/8 and 63701.
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

enum addr_mode {
	_inh=0, 	/* inherent */
	_rel,		/* relative */
	_im1,		/* immediate byte */
	_im2,		/* immediate word */
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
	eora,	eorb,	illegal,inc,	inca,	incb,	ins,	inx,
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
	"eora",  "eorb",  "*ill",  "inc",   "inca",  "incb",  "ins",   "inx",
	"jmp",   "jsr",   "lda",   "ldb",   "ldd",   "lds",   "ldx",   "lsr",
	"lsra",  "lsrb",  "lsrd",  "mul",   "neg",   "nega",  "negb",  "nop",
	"oim",   "ora",   "orb",   "psha",  "pshb",  "pshx",  "pula",  "pulb",
	"pulx",  "rol",   "rola",  "rolb",  "ror",   "rora",  "rorb",  "rti",
	"rts",   "sba",   "sbca",  "sbcb",  "sec",   "sev",   "sta",   "stb",
	"std",   "sti",   "sts",   "stx",   "suba",  "subb",  "subd",  "swi",
	"sync",  "tab",   "tap",   "tba",   "tim",   "tpa",   "tst",   "tsta",
	"tstb",  "tsx",   "txs",   "asx1",  "asx2",  "xgdx"
};


/* the third byte defines if an opcode is invalid for a CPU */
/* 1 6800/6802/6808, 2 6801/6803, 4 HD63701 */
static UINT8 disasm[0x100][3] = {
	{illegal,_inh, 0}, {nop,	_inh, 0}, {illegal,_inh, 0}, {illegal,_inh, 0}, /* 00 */
	{lsrd,	 _inh, 1}, {asld,	_inh, 1}, {tap,    _inh, 0}, {tpa,	  _inh, 0},
	{inx,	 _inh, 0}, {dex,	_inh, 0}, {clv,    _inh, 0}, {sev,	  _inh, 0},
	{clc,	 _inh, 0}, {sec,	_inh, 0}, {cli,    _inh, 0}, {sti,	  _inh, 0},
	{sba,	 _inh, 0}, {cba,	_inh, 0}, {asx1,   _sx1, 0}, {asx2,   _sx1, 0}, /* 10 */
	{illegal,_inh, 0}, {illegal,_inh, 0}, {tab,    _inh, 0}, {tba,	  _inh, 0},
	{xgdx,	 _inh, 3}, {daa,	_inh, 0}, {illegal,_inh, 0}, {aba,	  _inh, 0},
	{illegal,_inh, 0}, {illegal,_inh, 0}, {illegal,_inh, 0}, {illegal,_inh, 0},
	{bra,	 _rel, 0}, {brn,	_rel, 0}, {bhi,    _rel, 0}, {bls,	  _rel, 0}, /* 20 */
	{bcc,	 _rel, 0}, {bcs,	_rel, 0}, {bne,    _rel, 0}, {beq,	  _rel, 0},
	{bvc,	 _rel, 0}, {bvs,	_rel, 0}, {bpl,    _rel, 0}, {bmi,	  _rel, 0},
	{bge,	 _rel, 0}, {blt,	_rel, 0}, {bgt,    _rel, 0}, {ble,	  _rel, 0},
	{tsx,	 _inh, 0}, {ins,	_inh, 0}, {pula,   _inh, 0}, {pulb,   _inh, 0}, /* 30 */
	{des,	 _inh, 0}, {txs,	_inh, 0}, {psha,   _inh, 0}, {pshb,   _inh, 0},
	{pulx,	 _inh, 1}, {rts,	_inh, 0}, {abx,    _inh, 1}, {rti,	  _inh, 0},
	{pshx,	 _inh, 1}, {mul,	_inh, 1}, {sync,   _inh, 0}, {swi,	  _inh, 0},
	{nega,	 _inh, 0}, {illegal,_inh, 0}, {illegal,_inh, 0}, {coma,   _inh, 0}, /* 40 */
	{lsra,	 _inh, 0}, {illegal,_inh, 0}, {rora,   _inh, 0}, {asra,   _inh, 0},
	{asla,	 _inh, 0}, {rola,	_inh, 0}, {deca,   _inh, 0}, {illegal,_inh, 0},
	{inca,	 _inh, 0}, {tsta,	_inh, 0}, {illegal,_inh, 0}, {clra,   _inh, 0},
	{negb,	 _inh, 0}, {illegal,_inh, 0}, {illegal,_inh, 0}, {comb,   _inh, 0}, /* 50 */
	{lsrb,	 _inh, 0}, {illegal,_inh, 0}, {rorb,   _inh, 0}, {asrb,   _inh, 0},
	{aslb,	 _inh, 0}, {rolb,	_inh, 0}, {decb,   _inh, 0}, {illegal,_inh, 0},
	{incb,	 _inh, 0}, {tstb,	_inh, 0}, {illegal,_inh, 0}, {clrb,   _inh, 0},
	{neg,	 _idx, 0}, {aim,	_imx, 3}, {oim,    _imx, 3}, {com,	  _idx, 0}, /* 60 */
	{lsr,	 _idx, 0}, {eim,	_imx, 3}, {ror,    _idx, 0}, {asr,	  _idx, 0},
	{asl,	 _idx, 0}, {rol,	_idx, 0}, {dec,    _idx, 0}, {tim,	  _imx, 3},
	{inc,	 _idx, 0}, {tst,	_idx, 0}, {jmp,    _idx, 0}, {clr,	  _idx, 0},
	{neg,	 _ext, 0}, {aim,	_imd, 3}, {oim,    _imd, 3}, {com,	  _ext, 0}, /* 70 */
	{lsr,	 _ext, 0}, {eim,	_imd, 3}, {ror,    _ext, 0}, {asr,	  _ext, 0},
	{asl,	 _ext, 0}, {rol,	_ext, 0}, {dec,    _ext, 0}, {tim,	  _imd, 3},
	{inc,	 _ext, 0}, {tst,	_ext, 0}, {jmp,    _ext, 0}, {clr,	  _ext, 0},
	{suba,	 _im1, 0}, {cmpa,	_im1, 0}, {sbca,   _im1, 0}, {subd,   _im2, 1}, /* 80 */
	{anda,	 _im1, 0}, {bita,	_im1, 0}, {lda,    _im1, 0}, {sta,	  _im1, 0},
	{eora,	 _im1, 0}, {adca,	_im1, 0}, {ora,    _im1, 0}, {adda,   _im1, 0},
	{cmpx,	 _im2, 0}, {bsr,	_rel, 0}, {lds,    _im2, 0}, {sts,	  _im2, 0},
	{suba,	 _dir, 0}, {cmpa,	_dir, 0}, {sbca,   _dir, 0}, {subd,   _dir, 1}, /* 90 */
	{anda,	 _dir, 0}, {bita,	_dir, 0}, {lda,    _dir, 0}, {sta,	  _dir, 0},
	{eora,	 _dir, 0}, {adca,	_dir, 0}, {ora,    _dir, 0}, {adda,   _dir, 0},
	{cmpx,	 _dir, 0}, {jsr,	_dir, 0}, {lds,    _dir, 0}, {sts,	  _dir, 0},
	{suba,	 _idx, 0}, {cmpa,	_idx, 0}, {sbca,   _idx, 0}, {subd,   _idx, 1}, /* a0 */
	{anda,	 _idx, 0}, {bita,	_idx, 0}, {lda,    _idx, 0}, {sta,	  _idx, 0},
	{eora,	 _idx, 0}, {adca,	_idx, 0}, {ora,    _idx, 0}, {adda,   _idx, 0},
	{cmpx,	 _idx, 0}, {jsr,	_idx, 0}, {lds,    _idx, 0}, {sts,	  _idx, 0},
	{suba,	 _ext, 0}, {cmpa,	_ext, 0}, {sbca,   _ext, 0}, {subd,   _ext, 1}, /* b0 */
	{anda,	 _ext, 0}, {bita,	_ext, 0}, {lda,    _ext, 0}, {sta,	  _ext, 0},
	{eora,	 _ext, 0}, {adca,	_ext, 0}, {ora,    _ext, 0}, {adda,   _ext, 0},
	{cmpx,	 _ext, 0}, {jsr,	_ext, 0}, {lds,    _ext, 0}, {sts,	  _ext, 0},
	{subb,	 _im1, 0}, {cmpb,	_im1, 0}, {sbcb,   _im1, 0}, {addd,   _im2, 1}, /* c0 */
	{andb,	 _im1, 0}, {bitb,	_im1, 0}, {ldb,    _im1, 0}, {stb,	  _im1, 0},
	{eorb,	 _im1, 0}, {adcb,	_im1, 0}, {orb,    _im1, 0}, {addb,   _im1, 0},
	{ldd,	 _im2, 1}, {std,	_im2, 1}, {ldx,    _im2, 0}, {stx,	  _im2, 0},
	{subb,	 _dir, 0}, {cmpb,	_dir, 0}, {sbcb,   _dir, 0}, {addd,   _dir, 1}, /* d0 */
	{andb,	 _dir, 0}, {bitb,	_dir, 0}, {ldb,    _dir, 0}, {stb,	  _dir, 0},
	{eorb,	 _dir, 0}, {adcb,	_dir, 0}, {orb,    _dir, 0}, {addb,   _dir, 0},
	{ldd,	 _dir, 1}, {std,	_dir, 1}, {ldx,    _dir, 0}, {stx,	  _dir, 0},
	{subb,	 _idx, 0}, {cmpb,	_idx, 0}, {sbcb,   _idx, 0}, {addd,   _idx, 1}, /* e0 */
	{andb,	 _idx, 0}, {bitb,	_idx, 0}, {ldb,    _idx, 0}, {stb,	  _idx, 0},
	{eorb,	 _idx, 0}, {adcb,	_idx, 0}, {orb,    _idx, 0}, {addb,   _idx, 0},
	{ldd,	 _idx, 1}, {std,	_idx, 1}, {ldx,    _idx, 0}, {stx,	  _idx, 0},
	{subb,	 _ext, 0}, {cmpb,	_ext, 0}, {sbcb,   _ext, 0}, {addd,   _ext, 1}, /* f0 */
	{andb,	 _ext, 0}, {bitb,	_ext, 0}, {ldb,    _ext, 0}, {stb,	  _ext, 0},
	{eorb,	 _ext, 0}, {adcb,	_ext, 0}, {orb,    _ext, 0}, {addb,   _ext, 0},
	{ldd,	 _ext, 1}, {std,	_ext, 1}, {ldx,    _ext, 0}, {stx,	  _ext, 0}
};

/* some macros to keep things short */
#define OP      cpu_readop(pc)
#define ARG1    cpu_readop_arg(pc+1)
#define ARG2    cpu_readop_arg(pc+2)
#define ARGW	(cpu_readop_arg(pc+1)<<8) + cpu_readop_arg(pc+2)

int Dasm680x (int subtype, char *buf, int pc)
{
	int cputype;
	int code = cpu_readop(pc);
	const char *opstr;

	switch( subtype )
	{
		case 6800: case 6802: case 6808:
			cputype = 1;
			break;
		case 6801: case 6803:
			cputype = 2;
			break;
		default:
			cputype = 4;
	}

    if ( disasm[code][2] & cputype )    /* invalid for this cpu type ? */
		code = 0;						/* code 0 is illegal */
	opstr = op_name_str[disasm[code][0]];

    switch( disasm[code][1] )
	{
        case _rel:  /* relative */
			sprintf (buf, "%-5s$%04x", opstr, (pc + 2 + (signed char)ARG1) & 0xffff);
			return 2;
		case _im1:	/* immediate byte */
			sprintf (buf, "%-5s#$%02x", opstr, ARG1 );
			return 2;
		case _im2:	/* immediate word */
			sprintf (buf, "%-5s#$%04x", opstr, ARGW );
			return 3;
        case _idx:  /* indexed + byte offset */
			sprintf (buf, "%-5s(x+$%02x)", opstr, ARG1 );
            return 2;
        case _imx:  /* immediate, indexed + byte offset */
			sprintf (buf, "%-5s#$%02x,(x+$%02x)", opstr, ARG1, ARG2 );
			return 3;
		case _dir:	/* direct address */
			sprintf (buf, "%-5s$%02x", opstr, ARG1 );
			return 2;
		case _imd:	/* immediate, direct address */
			sprintf (buf, "%-5s#$%02x,$%02x", opstr, ARG1, ARG2 );
            return 3;
		case _ext:	/* extended address */
			sprintf (buf, "%-5s$%04x", opstr, ARGW );
			return 3;
		case _sx1:	/* byte from address (s + 1) */
			sprintf (buf, "%-5s(s+1)", opstr );
            return 1;
        default:
			strcpy (buf, opstr);
			return 1;
	}
}

