/* this code was hacked out of the fully-featured 6809 disassembler by Sean Riddle */


/* 6809dasm.c - a 6809 opcode disassembler */
/* Version 1.4 1-MAR-95 */
/* Copyright © 1995 Sean Riddle */

/* thanks to Franklin Bowen for bug fixes, ideas */

/* Freely distributable on any medium given all copyrights are retained */
/* by the author and no charge greater than $7.00 is made for obtaining */
/* this software */

/* Please send all bug reports, update ideas and data files to: */
/* sriddle@ionet.net */
#include <stdio.h>
#include <strings.h>
#include "M6809.h"

#ifndef TRUE
#define TRUE         -1
#define FALSE        0
#endif

typedef struct {                                       /* opcode structure */
   int opcode;                                     /* 8-bit opcode value */
   int numoperands;
   char name[6];                                            /* opcode name */
   int mode;                                          /* addressing mode */
   int numcycles;                         /* number of cycles - not used */
} opcodeinfo;

/* 6809 ADDRESSING MODES */
#define INH 0
#define DIR 1
#define IND 2
#define REL 3
#define EXT 4
#define IMM 5
#define LREL 6
#define PG2 7                                    /* PAGE SWITCHES - Page 2 */
#define PG3 8                                                    /* Page 3 */

/* number of opcodes in each page */
#define NUMPG1OPS 223
#define NUMPG2OPS 38
#define NUMPG3OPS 9

int numops[3]={
   NUMPG1OPS,NUMPG2OPS,NUMPG3OPS,
};

char modenames[9][14]={
   "inherent",
   "direct",
   "indexed",
   "relative",
   "extended",
   "immediate",
   "long relative",
   "page 2",
   "page 3",
};

opcodeinfo pg1opcodes[NUMPG1OPS]={                           /* page 1 ops */
	{0,1,"NEG",DIR,6},
	{3,1,"COM",DIR,6},
	{4,1,"LSR",DIR,6},
	{6,1,"ROR",DIR,6},
	{7,1,"ASR",DIR,6},
	{8,1,"ASL",DIR,6},
	{9,1,"ROL",DIR,6},
	{10,1,"DEC",DIR,6},
	{12,1,"INC",DIR,6},
	{13,1,"TST",DIR,6},
	{14,1,"JMP",DIR,3},
	{15,1,"CLR",DIR,6},

	{16,1,"page2",PG2,0},
	{17,1,"page3",PG3,0},
	{18,0,"NOP",INH,2},
	{19,0,"SYNC",INH,4},
	{22,2,"LBRA",LREL,5},
	{23,2,"LBSR",LREL,9},
	{25,0,"DAA",INH,2},
	{26,1,"ORCC",IMM,3},
	{28,1,"ANDCC",IMM,3},
	{29,0,"SEX",INH,2},
	{30,1,"EXG",IMM,8},
	{31,1,"TFR",IMM,6},

	{32,1,"BRA",REL,3},
	{33,1,"BRN",REL,3},
	{34,1,"BHI",REL,3},
	{35,1,"BLS",REL,3},
	{36,1,"BCC",REL,3},
	{37,1,"BCS",REL,3},
	{38,1,"BNE",REL,3},
	{39,1,"BEQ",REL,3},
	{40,1,"BVC",REL,3},
	{41,1,"BVS",REL,3},
	{42,1,"BPL",REL,3},
	{43,1,"BMI",REL,3},
	{44,1,"BGE",REL,3},
	{45,1,"BLT",REL,3},
	{46,1,"BGT",REL,3},
	{47,1,"BLE",REL,3},

	{48,1,"LEAX",IND,2},
	{49,1,"LEAY",IND,2},
	{50,1,"LEAS",IND,2},
	{51,1,"LEAU",IND,2},
	{52,1,"PSHS",INH,5},
	{53,1,"PULS",INH,5},
	{54,1,"PSHU",INH,5},
	{55,1,"PULU",INH,5},
	{57,0,"RTS",INH,5},
	{58,0,"ABX",INH,3},
	{59,0,"RTI",INH,6},
	{60,1,"CWAI",IMM,20},
	{61,0,"MUL",INH,11},
	{63,0,"SWI",INH,19},

	{64,0,"NEGA",INH,2},
	{67,0,"COMA",INH,2},
	{68,0,"LSRA",INH,2},
	{70,0,"RORA",INH,2},
	{71,0,"ASRA",INH,2},
	{72,0,"ASLA",INH,2},
	{73,0,"ROLA",INH,2},
	{74,0,"DECA",INH,2},
	{76,0,"INCA",INH,2},
	{77,0,"TSTA",INH,2},
	{79,0,"CLRA",INH,2},

	{80,0,"NEGB",INH,2},
	{83,0,"COMB",INH,2},
	{84,0,"LSRB",INH,2},
	{86,0,"RORB",INH,2},
	{87,0,"ASRB",INH,2},
	{88,0,"ASLB",INH,2},
	{89,0,"ROLB",INH,2},
	{90,0,"DECB",INH,2},
	{92,0,"INCB",INH,2},
	{93,0,"TSTB",INH,2},
	{95,0,"CLRB",INH,2},

	{96,1,"NEG",IND,6},
	{99,1,"COM",IND,6},
	{100,1,"LSR",IND,6},
	{102,1,"ROR",IND,6},
	{103,1,"ASR",IND,6},
	{104,1,"ASL",IND,6},
	{105,1,"ROL",IND,6},
	{106,1,"DEC",IND,6},
	{108,1,"INC",IND,6},
	{109,1,"TST",IND,6},
	{110,1,"JMP",IND,3},
	{111,1,"CLR",IND,6},

	{112,2,"NEG",EXT,7},
	{115,2,"COM",EXT,7},
	{116,2,"LSR",EXT,7},
	{118,2,"ROR",EXT,7},
	{119,2,"ASR",EXT,7},
	{120,2,"ASL",EXT,7},
	{121,2,"ROL",EXT,7},
	{122,2,"DEC",EXT,7},
	{124,2,"INC",EXT,7},
	{125,2,"TST",EXT,7},
	{126,2,"JMP",EXT,4},
	{127,2,"CLR",EXT,7},

	{128,1,"SUBA",IMM,2},
	{129,1,"CMPA",IMM,2},
	{130,1,"SBCA",IMM,2},
	{131,2,"SUBD",IMM,4},
	{132,1,"ANDA",IMM,2},
	{133,1,"BITA",IMM,2},
	{134,1,"LDA",IMM,2},
	{136,1,"EORA",IMM,2},
	{137,1,"ADCA",IMM,2},
	{138,1,"ORA",IMM,2},
	{139,1,"ADDA",IMM,2},
	{140,2,"CMPX",IMM,4},
	{141,1,"BSR",REL,7},
	{142,2,"LDX",IMM,3},

	{144,1,"SUBA",DIR,4},
	{145,1,"CMPA",DIR,4},
	{146,1,"SBCA",DIR,4},
	{147,1,"SUBD",DIR,6},
	{148,1,"ANDA",DIR,4},
	{149,1,"BITA",DIR,4},
	{150,1,"LDA",DIR,4},
	{151,1,"STA",DIR,4},
	{152,1,"EORA",DIR,4},
	{153,1,"ADCA",DIR,4},
	{154,1,"ORA",DIR,4},
	{155,1,"ADDA",DIR,4},
	{156,1,"CPX",DIR,6},
	{157,1,"JSR",DIR,7},
	{158,1,"LDX",DIR,5},
	{159,1,"STX",DIR,5},

	{160,1,"SUBA",IND,4},
	{161,1,"CMPA",IND,4},
	{162,1,"SBCA",IND,4},
	{163,1,"SUBD",IND,6},
	{164,1,"ANDA",IND,4},
	{165,1,"BITA",IND,4},
	{166,1,"LDA",IND,4},
	{167,1,"STA",IND,4},
	{168,1,"EORA",IND,4},
	{169,1,"ADCA",IND,4},
	{170,1,"ORA",IND,4},
	{171,1,"ADDA",IND,4},
	{172,1,"CPX",IND,6},
	{173,1,"JSR",IND,7},
	{174,1,"LDX",IND,5},
	{175,1,"STX",IND,5},

	{176,2,"SUBA",EXT,5},
	{177,2,"CMPA",EXT,5},
	{178,2,"SBCA",EXT,5},
	{179,2,"SUBD",EXT,7},
	{180,2,"ANDA",EXT,5},
	{181,2,"BITA",EXT,5},
	{182,2,"LDA",EXT,5},
	{183,2,"STA",EXT,5},
	{184,2,"EORA",EXT,5},
	{185,2,"ADCA",EXT,5},
	{186,2,"ORA",EXT,5},
	{187,2,"ADDA",EXT,5},
	{188,2,"CPX",EXT,7},
	{189,2,"JSR",EXT,8},
	{190,2,"LDX",EXT,6},
	{191,2,"STX",EXT,6},

	{192,1,"SUBB",IMM,2},
	{193,1,"CMPB",IMM,2},
	{194,1,"SBCB",IMM,2},
	{195,2,"ADDD",IMM,4},
	{196,1,"ANDB",IMM,2},
	{197,1,"BITB",IMM,2},
	{198,1,"LDB",IMM,2},
	{200,1,"EORB",IMM,2},
	{201,1,"ADCB",IMM,2},
	{202,1,"ORB",IMM,2},
	{203,1,"ADDB",IMM,2},
	{204,2,"LDD",IMM,3},
	{206,2,"LDU",IMM,3},

	{208,1,"SUBB",DIR,4},
	{209,1,"CMPB",DIR,4},
	{210,1,"SBCB",DIR,4},
	{211,1,"ADDD",DIR,6},
	{212,1,"ANDB",DIR,4},
	{213,1,"BITB",DIR,4},
	{214,1,"LDB",DIR,4},
	{215,1,"STB",DIR,4},
	{216,1,"EORB",DIR,4},
	{217,1,"ADCB",DIR,4},
	{218,1,"ORB",DIR,4},
	{219,1,"ADDB",DIR,4},
	{220,1,"LDD",DIR,5},
	{221,1,"STD",DIR,5},
	{222,1,"LDU",DIR,5},
	{223,1,"STU",DIR,5},

	{224,1,"SUBB",IND,4},
	{225,1,"CMPB",IND,4},
	{226,1,"SBCB",IND,4},
	{227,1,"ADDD",IND,6},
	{228,1,"ANDB",IND,4},
	{229,1,"BITB",IND,4},
	{230,1,"LDB",IND,4},
	{231,1,"STB",IND,4},
	{232,1,"EORB",IND,4},
	{233,1,"ADCB",IND,4},
	{234,1,"ORB",IND,4},
	{235,1,"ADDB",IND,4},
	{236,1,"LDD",IND,5},
	{237,1,"STD",IND,5},
	{238,1,"LDU",IND,5},
	{239,1,"STU",IND,5},

	{240,2,"SUBB",EXT,5},
	{241,2,"CMPB",EXT,5},
	{242,2,"SBCB",EXT,5},
	{243,2,"ADDD",EXT,7},
	{244,2,"ANDB",EXT,5},
	{245,2,"BITB",EXT,5},
	{246,2,"LDB",EXT,5},
	{247,2,"STB",EXT,5},
	{248,2,"EORB",EXT,5},
	{249,2,"ADCB",EXT,5},
	{250,2,"ORB",EXT,5},
	{251,2,"ADDB",EXT,5},
	{252,2,"LDD",EXT,6},
	{253,2,"STD",EXT,6},
	{254,2,"LDU",EXT,6},
	{255,2,"STU",EXT,6},
};

opcodeinfo pg2opcodes[NUMPG2OPS]={                       /* page 2 ops 10xx*/
	{33,3,"LBRN",LREL,5},
	{34,3,"LBHI",LREL,5},
	{35,3,"LBLS",LREL,5},
	{36,3,"LBCC",LREL,5},
	{37,3,"LBCS",LREL,5},
	{38,3,"LBNE",LREL,5},
	{39,3,"LBEQ",LREL,5},
	{40,3,"LBVC",LREL,5},
	{41,3,"LBVS",LREL,5},
	{42,3,"LBPL",LREL,5},
	{43,3,"LBMI",LREL,5},
	{44,3,"LBGE",LREL,5},
	{45,3,"LBLT",LREL,5},
	{46,3,"LBGT",LREL,5},
	{47,3,"LBLE",LREL,5},
	{63,2,"SWI2",INH,20},
	{131,3,"CMPD",IMM,5},
	{140,3,"CMPY",IMM,5},
	{142,3,"LDY",IMM,4},
	{147,2,"CMPD",DIR,7},
	{156,2,"CMPY",DIR,7},
	{158,2,"LDY",DIR,6},
	{159,2,"STY",DIR,6},
	{163,2,"CMPD",IND,7},
	{172,2,"CMPY",IND,7},
	{174,2,"LDY",IND,6},
	{175,2,"STY",IND,6},
	{179,3,"CMPD",EXT,8},
	{188,3,"CMPY",EXT,8},
	{190,3,"LDY",EXT,7},
	{191,3,"STY",EXT,7},
	{206,3,"LDS",IMM,4},
	{222,2,"LDS",DIR,6},
	{223,2,"STS",DIR,6},
	{238,2,"LDS",IND,6},
	{239,2,"STS",IND,6},
	{254,3,"LDS",EXT,7},
	{255,3,"STS",EXT,7},
};

opcodeinfo pg3opcodes[NUMPG3OPS]={                      /* page 3 ops 11xx */
	{63,1,"SWI3",INH,20},
	{131,3,"CMPU",IMM,5},
	{140,3,"CMPS",IMM,5},
	{147,2,"CMPU",DIR,7},
	{156,2,"CMPS",DIR,7},
	{163,2,"CMPU",IND,7},
	{172,2,"CMPS",IND,7},
	{179,3,"CMPU",EXT,8},
	{188,3,"CMPS",EXT,8},
};

opcodeinfo *pgpointers[3]={
   pg1opcodes,pg2opcodes,pg3opcodes,
};

const char *regs_6809[5]={"X","Y","U","S","PC"};
const char *teregs[16]={"D","X","Y","U","S","PC","inv","inv","A","B","CC",
      "DP","inv","inv","inv","inv"};

static char *hexstring (int address)
{
	static char labtemp[10];
	sprintf (labtemp, "$%04hX", address);
	return labtemp;
}

int Dasm6809 (char *buffer, int pc)
{
	int i, j, k, page, opcode, numoperands, mode;
	unsigned char operandarray[4];
	char *opname;
	int p = 0;

	buffer[0] = 0;
	opcode = M6809_RDOP(pc+(p++));
	for (i = 0; i < numops[0]; i++)
		if (pg1opcodes[i].opcode == opcode)
			break;

	if (i < numops[0])
	{
		if (pg1opcodes[i].mode >= PG2)
		{
			opcode = M6809_RDOP(pc+(p++));
			page = pg1opcodes[i].mode - PG2 + 1;          /* get page # */
			for (k = 0; k < numops[page]; k++)
				if (opcode == pgpointers[page][k].opcode)
					break;

			if (k != numops[page])
			{                 /* opcode found */
				numoperands = pgpointers[page][k].numoperands - 1;
				for (j = 0; j < numoperands; j++)
					operandarray[j] = M6809_RDOP_ARG(pc+(p++));
				mode = pgpointers[page][k].mode;
				opname = pgpointers[page][k].name;
				if (mode != IND)
					sprintf (buffer + strlen (buffer), "%-6s", opname);
				goto printoperands;
         }
         else
         {               /* not found in alternate page */
         	strcpy (buffer, "Illegal Opcode");
         	return 2;
         }
		}
		else
		{                                /* page 1 opcode */
			numoperands = pg1opcodes[i].numoperands;
			for (j = 0; j < numoperands; j++)
				operandarray[j] = M6809_RDOP_ARG(pc+(p++));
			mode = pg1opcodes[i].mode;
			opname = pg1opcodes[i].name;
			if (mode != IND)
				sprintf (buffer + strlen (buffer), "%-6s", opname);
			goto printoperands;
		}
	}
	else
	{
   	strcpy (buffer, "Illegal Opcode");
   	return 1;
   }

printoperands:
	pc += p;
{
   int rel, pb, offset=0, reg, pb2;
   int comma;
   int printdollar;                  /* print a leading $? before address */

   printdollar = FALSE;

   if ((opcode != 0x1f) && (opcode != 0x1e))
   {
      switch (mode)
      {                              /* print before operands */
         case IMM:
            strcat (buffer, "#");
         case DIR:
         case EXT:
            printdollar = TRUE;
            break;
         default:
            break;
      }
   }

   switch (mode)
   {
      case REL:                                          /* 8-bit relative */
         rel = operandarray[0];
         sprintf (buffer + strlen (buffer), hexstring ((short) (pc + ((rel < 128) ? rel : rel - 256))));
         break;

      case LREL:                                   /* 16-bit long relative */
         rel = (operandarray[0] << 8) + operandarray[1];
         sprintf (buffer + strlen (buffer), hexstring (pc + ((rel < 32768) ? rel : rel - 65536)));
         break;

      case IND:                                  /* indirect- many flavors */
         pb = operandarray[0];
         reg = (pb >> 5) & 0x3;
         pb2 = pb & 0x8f;
         if ((pb2 == 0x88) || (pb2 == 0x8c))
         {                    /* 8-bit offset */

            /* KW 11/05/98 Fix of indirect opcodes      */

            /*  offset = M6809_RDOP_ARG(pc+(p++));      */

            offset = M6809_RDOP_ARG(pc);
            p++;

            /* KW 11/05/98 Fix of indirect opcodes      */

            if (offset > 127)                            /* convert to signed */
               offset = offset - 256;
            if (pb == 0x8c)
               reg = 4;
            sprintf (buffer + strlen (buffer), "%-6s", opname);
            if ((pb & 0x90) == 0x90)
               strcat (buffer, "[");
            if (pb == 0x8c)
               sprintf (buffer + strlen (buffer), "%s,%s", hexstring (offset), regs_6809[reg]);
            else if (offset >= 0)
               sprintf (buffer + strlen (buffer), "$%02X,%s", offset, regs_6809[reg]);
            else
               sprintf (buffer + strlen (buffer), "-$%02X,%s", -offset, regs_6809[reg]);
            if (pb == 0x8c)
               sprintf (buffer + strlen (buffer), " ; ($%04X)", offset + pc);
         }
         else if ((pb2 == 0x89) || (pb2 == 0x8d) || (pb2 == 0x8f))
         { /* 16-bit */

            /* KW 11/05/98 Fix of indirect opcodes      */

            /*  offset = M6809_RDOP_ARG(pc+(p++)) << 8; */
            /*  offset += M6809_RDOP_ARG(pc+(p++));     */

            offset = M6809_RDOP_ARG(pc) << 8;
            offset += M6809_RDOP_ARG(pc+1);
            p+=2;

            /* KW 11/05/98 Fix of indirect opcodes      */

            if ((pb != 0x8f) && (offset > 32767))
               offset = offset - 65536;
            offset &= 0xffff;
            if (pb == 0x8d)
               reg = 4;
            sprintf (buffer + strlen (buffer), "%-6s", opname);
            if ((pb&0x90) == 0x90)
               strcat (buffer, "[");
            if (pb == 0x8d)
               sprintf (buffer + strlen (buffer), "%s,%s", hexstring (offset), regs_6809[reg]);
            else if (offset >= 0)
               sprintf (buffer + strlen (buffer), "$%04X,%s", offset, regs_6809[reg]);
            else
               sprintf (buffer + strlen (buffer), "-$%04X,%s", offset, regs_6809[reg]);
            if (pb == 0x8d)
               sprintf (buffer + strlen (buffer), " ; ($%04X)", offset + pc);
         }
         else if (pb & 0x80)
         {
            sprintf (buffer + strlen (buffer), "%-6s", opname);
            if ((pb & 0x90) == 0x90)
               strcat (buffer, "[");
            if ((pb & 0x8f) == 0x80)
               sprintf (buffer + strlen (buffer), ",%s+", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x81)
               sprintf (buffer + strlen (buffer), ",%s++", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x82)
               sprintf (buffer + strlen (buffer), ",-%s", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x83)
               sprintf (buffer + strlen (buffer), ",--%s", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x84)
               sprintf (buffer + strlen (buffer), ",%s", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x85)
               sprintf (buffer + strlen (buffer), "B,%s", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x86)
               sprintf (buffer + strlen (buffer), "A,%s", regs_6809[reg]);
            else if ((pb & 0x8f) == 0x8b)
               sprintf (buffer + strlen (buffer), "D,%s", regs_6809[reg]);
         }
         else
         {                                          /* 5-bit offset */
            offset = pb & 0x1f;
            if (offset > 15)
               offset = offset - 32;
            sprintf (buffer + strlen (buffer), "%-6s", opname);
            sprintf (buffer + strlen (buffer), "%s,%s", hexstring (offset), regs_6809[reg]);
         }
         if ((pb & 0x90) == 0x90)
            strcat (buffer, "]");
         break;

      default:
         if ((opcode == 0x1f) || (opcode == 0x1e))
         {                   /* TFR/EXG */
            sprintf (buffer + strlen (buffer), "%s,%s", teregs[ (operandarray[0] >> 4) & 0xf], teregs[operandarray[0] & 0xf]);
         }
         else if ((opcode == 0x34) || (opcode == 0x36))
         {              /* PUSH */
            comma = FALSE;
            if (operandarray[0] & 0x80)
            {
               strcat (buffer, "PC");
               comma = TRUE;
            }
            if (operandarray[0] & 0x40)
            {
               if (comma)
                  strcat (buffer, ",");
               if ((opcode == 0x34) || (opcode == 0x35))
                  strcat (buffer, "U");
               else
                  strcat (buffer, "S");
               comma = TRUE;
            }
            if (operandarray[0] & 0x20)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "Y");
               comma = TRUE;
            }
            if (operandarray[0] & 0x10)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "X");
               comma = TRUE;
            }
            if (operandarray[0] & 0x8)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "DP");
               comma = TRUE;
            }
            if (operandarray[0] & 0x4)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "B");
               comma = TRUE;
            }
            if (operandarray[0] & 0x2)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "A");
               comma = TRUE;
            }
            if (operandarray[0] & 0x1)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "CC");
            }
         }
         else if ((opcode == 0x35) || (opcode == 0x37))
         {              /* PULL */
            comma = FALSE;
            if (operandarray[0] & 0x1)
            {
               strcat (buffer, "CC");
               comma = TRUE;
            }
            if (operandarray[0] & 0x2)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "A");
               comma = TRUE;
            }
            if (operandarray[0] & 0x4)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "B");
               comma = TRUE;
            }
            if (operandarray[0] & 0x8)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "DP");
               comma = TRUE;
            }
            if (operandarray[0] & 0x10)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "X");
               comma = TRUE;
            }
            if (operandarray[0] & 0x20)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "Y");
               comma = TRUE;
            }
            if (operandarray[0] & 0x40)
            {
               if (comma)
                  strcat (buffer, ",");
               if ((opcode == 0x34)|| (opcode == 0x35))
                  strcat (buffer, "U");
               else
                  strcat (buffer, "S");
               comma = TRUE;
            }
            if (operandarray[0] & 0x80)
            {
               if (comma)
                  strcat (buffer, ",");
               strcat (buffer, "PC");
					strcat (buffer, " ; (PUL? PC=RTS)");
            }
         }
         else
         {
            if (numoperands == 2)
            {
               strcat (buffer + strlen (buffer), hexstring ((operandarray[0] << 8) + operandarray[1]));
            }
            else
            {
               if (printdollar)
                  strcat (buffer, "$");
               for (i = 0; i < numoperands; i++)
                  sprintf (buffer + strlen (buffer), "%02X", operandarray[i]);
            }
         }
         break;
   }
}

	return p;
}
