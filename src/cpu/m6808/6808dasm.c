/*
 *   A quick-hack 6803/6808 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 * History:
 * 99/03/02 HJB
 * Changed the string array into a table of opcode names (or tokens) and
 * argument types. This second try should give somewhat better results ;)
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

static unsigned char disasm[0x100][2] = {
	{illegal,_inh}, {nop,	 _inh}, {illegal,_inh}, {illegal,_inh}, /* 00 */
	{lsrd,	 _inh}, {asld,	 _inh}, {tap,	 _inh}, {tpa,	 _inh},
	{inx,	 _inh}, {dex,	 _inh}, {clv,	 _inh}, {sev,	 _inh},
	{clc,	 _inh}, {sec,	 _inh}, {cli,	 _inh}, {sti,	 _inh},
	{sba,	 _inh}, {cba,	 _inh}, {asx1,	 _sx1}, {asx2,	 _sx1}, /* 10 */
	{illegal,_inh}, {illegal,_inh}, {tab,	 _inh}, {tba,	 _inh},
	{xgdx,	 _inh}, {daa,	 _inh}, {illegal,_inh}, {aba,	 _inh},
	{illegal,_inh}, {illegal,_inh}, {illegal,_inh}, {illegal,_inh},
	{bra,	 _rel}, {brn,	 _rel}, {bhi,	 _rel}, {bls,	 _rel}, /* 20 */
	{bcc,	 _rel}, {bcs,	 _rel}, {bne,	 _rel}, {beq,	 _rel},
	{bvc,	 _rel}, {bvs,	 _rel}, {bpl,	 _rel}, {bmi,	 _rel},
	{bge,	 _rel}, {blt,	 _rel}, {bgt,	 _rel}, {ble,	 _rel},
	{tsx,	 _inh}, {ins,	 _inh}, {pula,	 _inh}, {pulb,	 _inh}, /* 30 */
	{des,	 _inh}, {txs,	 _inh}, {psha,	 _inh}, {pshb,	 _inh},
	{pulx,	 _inh}, {rts,	 _inh}, {abx,	 _inh}, {rti,	 _inh},
	{pshx,	 _inh}, {mul,	 _inh}, {sync,	 _inh}, {swi,	 _inh},
	{nega,	 _inh}, {illegal,_inh}, {illegal,_inh}, {coma,	 _inh}, /* 40 */
	{lsra,	 _inh}, {illegal,_inh}, {rora,	 _inh}, {asra,	 _inh},
	{asla,	 _inh}, {rola,	 _inh}, {deca,	 _inh}, {illegal,_inh},
	{inca,	 _inh}, {tsta,	 _inh}, {illegal,_inh}, {clra,	 _inh},
	{negb,	 _inh}, {illegal,_inh}, {illegal,_inh}, {comb,	 _inh}, /* 50 */
	{lsrb,	 _inh}, {illegal,_inh}, {rorb,	 _inh}, {asrb,	 _inh},
	{aslb,	 _inh}, {rolb,	 _inh}, {decb,	 _inh}, {illegal,_inh},
	{incb,	 _inh}, {tstb,	 _inh}, {illegal,_inh}, {clrb,	 _inh},
	{neg,	 _idx}, {aim,	 _imx}, {oim,	 _imx}, {com,	 _idx}, /* 60 */
	{lsr,	 _idx}, {eim,	 _imx}, {ror,	 _idx}, {asr,	 _idx},
	{asl,	 _idx}, {rol,	 _idx}, {dec,	 _idx}, {tim,	 _imx},
	{inc,	 _idx}, {tst,	 _idx}, {jmp,	 _idx}, {clr,	 _idx},
	{neg,	 _ext}, {aim,	 _imd}, {oim,	 _imd}, {com,	 _ext}, /* 70 */
	{lsr,	 _ext}, {eim,	 _imd}, {ror,	 _ext}, {asr,	 _ext},
	{asl,	 _ext}, {rol,	 _ext}, {dec,	 _ext}, {tim,	 _imd},
	{inc,	 _ext}, {tst,	 _ext}, {jmp,	 _ext}, {clr,	 _ext},
	{suba,	 _im1}, {cmpa,	 _im1}, {sbca,	 _im1}, {subd,	 _im2}, /* 80 */
	{anda,	 _im1}, {bita,	 _im1}, {lda,	 _im1}, {sta,	 _im1},
	{eora,	 _im1}, {adca,	 _im1}, {ora,	 _im1}, {adda,	 _im1},
	{cmpx,	 _im2}, {bsr,	 _rel}, {lds,	 _im2}, {sts,	 _im2},
	{suba,	 _dir}, {cmpa,	 _dir}, {sbca,	 _dir}, {subd,	 _dir}, /* 90 */
	{anda,	 _dir}, {bita,	 _dir}, {lda,	 _dir}, {sta,	 _dir},
	{eora,	 _dir}, {adca,	 _dir}, {ora,	 _dir}, {adda,	 _dir},
	{cmpx,	 _dir}, {jsr,	 _dir}, {lds,	 _dir}, {sts,	 _dir},
	{suba,	 _idx}, {cmpa,	 _idx}, {sbca,	 _idx}, {subd,	 _idx}, /* a0 */
	{anda,	 _idx}, {bita,	 _idx}, {lda,	 _idx}, {sta,	 _idx},
	{eora,	 _idx}, {adca,	 _idx}, {ora,	 _idx}, {adda,	 _idx},
	{cmpx,	 _idx}, {jsr,	 _idx}, {lds,	 _idx}, {sts,	 _idx},
	{suba,	 _ext}, {cmpa,	 _ext}, {sbca,	 _ext}, {subd,	 _ext}, /* b0 */
	{anda,	 _ext}, {bita,	 _ext}, {lda,	 _ext}, {sta,	 _ext},
	{eora,	 _ext}, {adca,	 _ext}, {ora,	 _ext}, {adda,	 _ext},
	{cmpx,	 _ext}, {jsr,	 _ext}, {lds,	 _ext}, {sts,	 _ext},
	{subb,	 _im1}, {cmpb,	 _im1}, {sbcb,	 _im1}, {addd,	 _im2}, /* c0 */
	{andb,	 _im1}, {bitb,	 _im1}, {ldb,	 _im1}, {stb,	 _im1},
	{eorb,	 _im1}, {adcb,	 _im1}, {orb,	 _im1}, {addb,	 _im1},
	{ldd,	 _ext}, {std,	 _im2}, {ldx,	 _im2}, {stx,	 _im2},
	{subb,	 _dir}, {cmpb,	 _dir}, {sbcb,	 _dir}, {addd,	 _dir}, /* d0 */
	{andb,	 _dir}, {bitb,	 _dir}, {ldb,	 _dir}, {stb,	 _dir},
	{eorb,	 _dir}, {adcb,	 _dir}, {orb,	 _dir}, {addb,	 _dir},
	{ldd,	 _dir}, {std,	 _dir}, {ldx,	 _dir}, {stx,	 _dir},
	{subb,	 _idx}, {cmpb,	 _idx}, {sbcb,	 _idx}, {addd,	 _idx}, /* e0 */
	{andb,	 _idx}, {bitb,	 _idx}, {ldb,	 _idx}, {stb,	 _idx},
	{eorb,	 _idx}, {adcb,	 _idx}, {orb,	 _idx}, {addb,	 _idx},
	{ldd,	 _idx}, {std,	 _idx}, {ldx,	 _idx}, {stx,	 _idx},
	{subb,	 _ext}, {cmpb,	 _ext}, {sbcb,	 _ext}, {addd,	 _ext}, /* f0 */
	{andb,	 _ext}, {bitb,	 _ext}, {ldb,	 _ext}, {stb,	 _ext},
	{eorb,	 _ext}, {adcb,	 _ext}, {orb,	 _ext}, {addb,	 _ext},
	{ldd,	 _ext}, {std,	 _ext}, {ldx,	 _ext}, {stx,	 _ext}
};

/* some macros to keep things short */
#define OP      cpu_readop(pc)
#define ARG1    cpu_readop_arg(pc+1)
#define ARG2    cpu_readop_arg(pc+2)
#define ARGW	(cpu_readop_arg(pc+1)<<8) + cpu_readop_arg(pc+2)

int Dasm6808 (char *buf, int pc)
{
	int code = cpu_readop(pc);
	const char *opstr = op_name_str[disasm[code][0]];

    switch( disasm[code][1] ) {
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
