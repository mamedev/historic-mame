/*
 *   A quick-hack 6803/6808 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 */

#include <string.h>
#include <stdio.h>

static char *opcode_strings[0x0100] =
{
	"illegal", 	"illegal", 	"illegal", 	"illegal", 	"lsrd", 		"asld", 		"tap",		"tpa",			/*00*/
	"inx",		"dex",		"clv",		"sev",		"clc",		"sec",		"cli",		"sti",
	"sba",		"cba",		"illegal", 	"illegal", 	"illegal", 	"illegal", 	"tab",		"tba",			/*10*/
	"xgdx", 	"daa",		"illegal", 	"aba",		"illegal", 	"illegal", 	"illegal", 	"illegal",
	"bra",		"brn",		"bhi",		"bls",		"bcc",		"bcs",		"bne",		"beq",			/*20*/
	"bvc",		"bvs",		"bpl",		"bmi",		"bge",		"blt",		"bgt",		"ble",
	"tsx",		"ins",		"pula",		"pulb",		"des",		"txs",		"psha",		"pshb",			/*30*/
	"pulx",	 	"rts",		"abx",		"rti",		"pshx",	 	"mul",	 	"sync",		"swi",
	"nega",		"illegal", 	"illegal", 	"coma",		"lsra",		"illegal", 	"rora",		"asra",			/*40*/
	"asla",		"rola",		"deca",		"illegal", 	"inca",		"tsta",		"illegal", 	"clra",
	"negb",		"illegal", 	"illegal", 	"comb",		"lsrb",		"illegal", 	"rorb",		"asrb",			/*50*/
	"aslb",		"rolb",		"decb",		"illegal", 	"incb",		"tstb",		"illegal", 	"clrb",
	"neg_ix",	"aim_ix", 	"oim_ix", 	"com_ix",	"lsr_ix",	"eim_ix", 	"ror_ix",	"asr_ix",		/*60*/
	"asl_ix",	"rol_ix",	"dec_ix",	"tim_ix", 	"inc_ix",	"tst_ix",	"jmp_ix",	"clr_ix",
	"neg_ex",	"aim_di", 	"oim_di", 	"com_ex",	"lsr_ex",	"eim_di", 	"ror_ex",	"asr_ex",		/*70*/
	"asl_ex",	"rol_ex",	"dec_ex",	"tim_di", 	"inc_ex",	"tst_ex",	"jmp_ex",	"clr_ex",
	"suba_im",	"cmpa_im",	"sbca_im",	"subd_im", 	"anda_im",	"bita_im",	"lda_im",	"sta_im",		/*80*/
	"eora_im",	"adca_im",	"ora_im",	"adda_im",	"cmpx_im",	"bsr",		"lds_im",	"sts_im",
	"suba_di",	"cmpa_di",	"sbca_di",	"subd_di", 	"anda_di",	"bita_di",	"lda_di",	"sta_di",		/*90*/
	"eora_di",	"adca_di",	"ora_di",	"adda_di",	"cmpx_di",	"jsr_di",	"lds_di",	"sts_di",
	"suba_ix",	"cmpa_ix",	"sbca_ix",	"subd_ix", 	"anda_ix",	"bita_ix",	"lda_ix",	"sta_ix",		/*A0*/
	"eora_ix",	"adca_ix",	"ora_ix",	"adda_ix",	"cmpx_ix",	"jsr_ix",	"lds_ix",	"sts_ix",
	"suba_ex",	"cmpa_ex",	"sbca_ex",	"subd_ex", 	"anda_ex",	"bita_ex",	"lda_ex",	"sta_ex",		/*B0*/
	"eora_ex",	"adca_ex",	"ora_ex",	"adda_ex",	"cmpx_ex",	"jsr_ex",	"lds_ex",	"sts_ex",
	"subb_im",	"cmpb_im",	"sbcb_im",	"addd_im",	"andb_im",	"bitb_im",	"ldb_im",	"stb_im",		/*C0*/
	"eorb_im",	"adcb_im",	"orb_im",	"addb_im",	"ldd_ex",	"std_im",	"ldx_im",	"stx_im",
	"subb_di",	"cmpb_di",	"sbcb_di",	"addd_di",	"andb_di",	"bitb_di",	"ldb_di",	"stb_di",		/*D0*/
	"eorb_di",	"adcb_di",	"orb_di",	"addb_di",	"ldd_di",	"std_di",	"ldx_di",	"stx_di",
	"subb_ix",	"cmpb_ix",	"sbcb_ix",	"addd_ix",	"andb_ix",	"bitb_ix",	"ldb_ix",	"stb_ix",		/*E0*/
	"eorb_ix",	"adcb_ix",	"orb_ix",	"addb_ix",	"ldd_ix",	"std_ix",	"ldx_ix",	"stx_ix",
	"subb_ex",	"cmpb_ex",	"sbcb_ex",	"addd_ex",	"andb_ex",	"bitb_ex",	"ldb_ex",	"stb_ex",		/*F0*/
	"eorb_ex",	"adcb_ex",	"orb_ex",	"addb_ex",	"ldd_ex",	"std_ex",	"ldx_ex",	"stx_ex"
};


int Dasm6808 (unsigned char *base, char *buf, int pc)
{
	int code = *base++;
	char *opptr = opcode_strings[code];
	char opstr[20];
	char *p, *s;

	strcpy (opstr, opptr);
	p = opstr + 3;
	if (*p != '_')
	{
		if (*++p != '_')
		{
			if ((code >= 0x20 && code <= 0x2f) || code == 0x8d)
			{
				sprintf (buf, "%-5s$%04x", opstr, (pc + 2 + *(signed char *)base) & 0xffff);
				return 2;
			}
			else
			{
				strcpy (buf, opstr);
				return 1;
			}
		}
	}

	s = p - 1;
	*p++ = 0;
	if (*p == 'i')
	{
		if (*++p == 'x')
		{
			if ( s[-1] == 'i' && s[0] == 'm' ) { /* EHC 04-11 Added AIM/OIM/EIM/TIM support */
				sprintf (buf, "%-5s#$%02x,(x+$%02x)", opstr, base[0], base[1] );
				return 3;
			}

			sprintf (buf, "%-5s(x+$%02x)", opstr, *base);
			return 2;
		}
		else
		{
			if (*s == 'x' || *s == 's' || *s == 'd')
			{
				sprintf (buf, "%-5s#$%04x", opstr, (base[0] << 8) + base[1]);
				return 3;
			}
			else
			{
				sprintf (buf, "%-5s#$%02x", opstr, *base);
				return 2;
			}
		}
	}
	else if (*p == 'd')
	{
		if ( s[-1] == 'i' && s[0] == 'm' ) { /* EHC 04-11 Added AIM/OIM/EIM/TIM support */
			sprintf (buf, "%-5s#$%02x,$%02x", opstr, base[0], base[1] );
			return 3;
		}

		sprintf (buf, "%-5s$%02x", opstr, *base);
		return 2;
	}
	else
	{
		sprintf (buf, "%-5s$%04x", opstr, (base[0] << 8) + base[1]);
		return 3;
	}
}
