/*******************************************************
 *
 *      Portable Signetics 2650 disassembler
 *
 *      Written by Juergen Buchmueller for use with MAME
 *
 *******************************************************/

#include <stdio.h>
#include "memory.h"

#define MNEMONIC_EQUATES

/* handy table to build relative offsets from HR */
static  int     rel[0x100] = {
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
};

typedef char* (*callback) (int addr);
static callback find_symbol = 0;

char *SYM(int addr)
{
static char buff[8+1];
char * s = 0;
	if (find_symbol)
		s = (*find_symbol)(addr);
	if (s)
		return s;
	sprintf(buff, "%04X", addr);
	return buff;
}

/* format an immediate */
char    *IMM(int pc)
{
static  char    buff[32];
	sprintf(buff, "%02X", cpu_readop_arg(pc));
	return buff;
}

#ifdef  MNEMONIC_EQUATES
static  char cc[4] = { 'Z', 'P', 'M', 'A' };

/* format an immediate for PSL */
char    *IMM_PSL(int pc)
{
static  char    buff[32];
char    *p = buff;
int     v = cpu_readop_arg(pc);
	switch (v & 0xc0)
	{
		case 0x40: p += sprintf(p, "P+"); break;
		case 0x80: p += sprintf(p, "M+"); break;
		case 0xc0: p += sprintf(p, "CC+"); break;
	}
	if (v & 0x20)
		p += sprintf(p, "IDC+");
	if (v & 0x10)
		p += sprintf(p, "RS+");
	if (v & 0x08)
		p += sprintf(p, "WC+");
	if (v & 0x04)
		p += sprintf(p, "OVF+");
	if (v & 0x02)
		p += sprintf(p, "COM+");
	if (v & 0x01)
		p += sprintf(p, "C+");
	if (p > buff)
		*--p = '\0';
	return buff;
}

/* format an immediate for PSU */
char    *IMM_PSU(int pc)
{
static  char    buff[32];
char    *p = buff;
int     v = cpu_readop_arg(pc);
	if (v & 0x80)
		p += sprintf(p, "S+");
	if (v & 0x40)
		p += sprintf(p, "F+");
	if (v & 0x20)
		p += sprintf(p, "II+");
	if (v & 0x10)
		p += sprintf(p, "4+");
	if (v & 0x08)
		p += sprintf(p, "3+");
	if (v & 0x04)
		p += sprintf(p, "SP2+");
	if (v & 0x02)
		p += sprintf(p, "SP1+");
	if (v & 0x01)
		p += sprintf(p, "SP0+");
	if (p > buff)
		*--p = '\0';
	return buff;
}
#else
static  char cc[4] = { '0', '1', '2', '3' };
#define IMM_PSL IMM
#define IMM_PSU IMM
#endif

/* format an relative address */
char    *REL(int pc)
{
static char buff[32];
int o = cpu_readop_arg(pc);
	sprintf(buff, "%s%s", (o&0x80)?"*":"", SYM((pc&0x6000)+((pc+1+rel[o])&0x1fff)));
	return buff;
}

/* format an relative address (implicit page 0) */
char    *REL0(int pc)
{
static char buff[32];
int o = cpu_readop_arg(pc);
	sprintf(buff, "%s%s", (o&0x80)?"*":"", SYM((pc+1+rel[o]) & 0x1fff));
	return buff;
}

/* format a destination register and an absolute address */
char    *ABS(int r, int pc)
{
static char buff[32];
int h = cpu_readop_arg(pc);
int l = cpu_readop_arg((pc&0x6000)+((pc+1)&0x1fff));
int a = (pc & 0x6000) + ((h & 0x1f) << 8) + l;
	switch (h >> 5)
	{
		case 0: sprintf(buff, "%d %s", r, SYM(a));      break;
		case 1: sprintf(buff, "0 %s,R%d+", SYM(a), r);  break;
		case 2: sprintf(buff, "0 %s,R%d-", SYM(a), r);  break;
		case 3: sprintf(buff, "0 %s,R%d", SYM(a), r);   break;
		case 4: sprintf(buff, "%d *%s", r, SYM(a));     break;
		case 5: sprintf(buff, "0 *%s,R%d+", SYM(a), r); break;
		case 6: sprintf(buff, "0 *%s,R%d-", SYM(a), r); break;
		case 7: sprintf(buff, "0 *%s,R%d", SYM(a), r);  break;
	}
	return buff;
}

/* format an (branch) absolute address */
char    *ADR(int pc)
{
static char buff[32];
int h = cpu_readop_arg(pc);
int l = cpu_readop_arg((pc&0x6000)+((pc+1)&0x1fff));
int a = ((h & 0x7f) << 8) + l;
	if (h & 0x80)
		sprintf(buff, "*%s", SYM(a));
	else
		sprintf(buff, "%s", SYM(a));
	return buff;
}

/* disassemble one instruction at PC into buff. return byte size of instr */
int     Dasm2650(char * buff, int PC)
{
int pc = PC;
int op = cpu_readop(pc);
int rv = op & 3;
	pc += 1;
	switch (op)
	{
		case 0x00: case 0x01: case 0x02: case 0x03:
			sprintf(buff, "LODZ,%d", rv);
			break;
		case 0x04: case 0x05: case 0x06: case 0x07:
			sprintf(buff, "LODI,%d %s", rv, IMM(pc)); pc+=1;
			break;
		case 0x08: case 0x09: case 0x0a: case 0x0b:
			sprintf(buff, "LODR,%d %s", rv, REL(pc)); pc+=1;
			break;
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			sprintf(buff, "LODA,%s", ABS(rv,pc)); pc+=2;
			break;
		case 0x10: case 0x11:
			sprintf(buff, "****   %02X",op);
			break;
		case 0x12:
			sprintf(buff, "SPSU");
			break;
		case 0x13:
			sprintf(buff, "SPSL");
			break;
		case 0x14: case 0x15: case 0x16: case 0x17:
			sprintf(buff, "RETC   %c", cc[rv]);
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
			sprintf(buff, "BCTR,%c %s", cc[rv], REL(pc));
			pc+=1;
			break;
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			sprintf(buff, "BCTA,%c %s", cc[rv], ADR(pc));
			pc+=2;
			break;
		case 0x20: case 0x21: case 0x22: case 0x23:
			sprintf(buff, "EORZ,%d", rv);
			break;
		case 0x24: case 0x25: case 0x26: case 0x27:
			sprintf(buff, "EORI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0x28: case 0x29: case 0x2a: case 0x2b:
			sprintf(buff, "EORR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			sprintf(buff, "EORA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0x30: case 0x31: case 0x32: case 0x33:
			sprintf(buff, "REDC,%d", rv);
			break;
		case 0x34: case 0x35: case 0x36: case 0x37:
			sprintf(buff, "RETE   %c", cc[rv]);
			break;
		case 0x38: case 0x39: case 0x3a: case 0x3b:
			sprintf(buff, "BSTR,%c %s", cc[rv], REL(pc));
			pc+=1;
			break;
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			sprintf(buff, "BSTA,%c %s", cc[rv], ADR(pc));
			pc+=2;
			break;
		case 0x40:
			sprintf(buff, "HALT");
			break;
		case 0x41: case 0x42: case 0x43:
			sprintf(buff, "ANDZ,%d", rv);
			break;
		case 0x44: case 0x45: case 0x46: case 0x47:
			sprintf(buff, "ANDI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0x48: case 0x49: case 0x4a: case 0x4b:
			sprintf(buff, "ANDR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x4c: case 0x4d: case 0x4e: case 0x4f:
			sprintf(buff, "ANDA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0x50: case 0x51: case 0x52: case 0x53:
			sprintf(buff, "RRR,%d", rv);
			break;
		case 0x54: case 0x55: case 0x56: case 0x57:
			sprintf(buff, "REDE,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0x58: case 0x59: case 0x5a: case 0x5b:
			sprintf(buff, "BRNR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x5c: case 0x5d: case 0x5e: case 0x5f:
			sprintf(buff, "BRNA,%d %s", rv, ADR(pc));
			pc+=2;
			break;
		case 0x60: case 0x61: case 0x62: case 0x63:
			sprintf(buff, "IORZ,%d", rv);
			break;
		case 0x64: case 0x65: case 0x66: case 0x67:
			sprintf(buff, "IORI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0x68: case 0x69: case 0x6a: case 0x6b:
			sprintf(buff, "IORR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x6c: case 0x6d: case 0x6e: case 0x6f:
			sprintf(buff, "IORA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0x70: case 0x71: case 0x72: case 0x73:
			sprintf(buff, "REDD,%d", rv);
			break;
		case 0x74:
			sprintf(buff, "CPSU   %s", IMM_PSU(pc));
			pc+=1;
			break;
		case 0x75:
			sprintf(buff, "CPSL   %s", IMM_PSL(pc));
			pc+=1;
			break;
		case 0x76:
			sprintf(buff, "PPSU   %s", IMM_PSU(pc));
			pc+=1;
			break;
		case 0x77:
			sprintf(buff, "PPSL   %s", IMM_PSL(pc));
			pc+=1;
			break;
		case 0x78: case 0x79: case 0x7a: case 0x7b:
			sprintf(buff, "BSNR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			sprintf(buff, "BSNA,%d %s", rv, ADR(pc));
			pc+=2;
			break;
		case 0x80: case 0x81: case 0x82: case 0x83:
			sprintf(buff, "ADDZ,%d", rv);
			break;
		case 0x84: case 0x85: case 0x86: case 0x87:
			sprintf(buff, "ADDI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0x88: case 0x89: case 0x8a: case 0x8b:
			sprintf(buff, "ADDR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			sprintf(buff, "ADDA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0x90: case 0x91:
			sprintf(buff, "****   %02X",op);
			break;
		case 0x92:
			sprintf(buff, "LPSU");
			break;
		case 0x93:
			sprintf(buff, "LPSL");
			break;
		case 0x94: case 0x95: case 0x96: case 0x97:
			sprintf(buff, "DAR,%d", rv);
			break;
		case 0x98: case 0x99: case 0x9a:
			sprintf(buff, "BCFR,%c %s", cc[rv], REL(pc));
			pc+=1;
			break;
		case 0x9b:
			sprintf(buff, "ZBRR   %s", REL0(pc));
			pc+=1;
			break;
		case 0x9c: case 0x9d: case 0x9e:
			sprintf(buff, "BCFA,%c %s", cc[rv], ADR(pc));
			pc+=2;
			break;
		case 0x9f:
			sprintf(buff, "BXA    %s", ADR(pc));
			pc+=2;
			break;
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
			sprintf(buff, "SUBZ,%d", rv);
			break;
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
			sprintf(buff, "SUBI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
			sprintf(buff, "SUBR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0xac: case 0xad: case 0xae: case 0xaf:
			sprintf(buff, "SUBA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
			sprintf(buff, "WRTC,%d", rv);
			break;
		case 0xb4:
			sprintf(buff, "TPSU   %s", IMM_PSU(pc));
			pc+=1;
			break;
		case 0xb5:
			sprintf(buff, "TPSL   %s", IMM_PSL(pc));
			pc+=1;
			break;
		case 0xb6: case 0xb7:
			sprintf(buff, "****   %02X", op);
			break;
		case 0xb8: case 0xb9: case 0xba:
			sprintf(buff, "BSFR,%c %s", cc[rv], REL(pc));
			pc+=1;
			break;
		case 0xbb:
			sprintf(buff, "ZBSR   %s", REL0(pc));
			pc+=1;
			break;
		case 0xbc: case 0xbd: case 0xbe:
			sprintf(buff, "BSFA,%c %s", cc[rv], ADR(pc));
			pc+=2;
			break;
		case 0xbf:
			sprintf(buff, "BSXA   %s", ADR(pc));
			pc+=2;
			break;
		case 0xc0:
			sprintf(buff, "NOP");
			break;
		case 0xc1: case 0xc2: case 0xc3:
			sprintf(buff, "STRZ,%d", rv);
			break;
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
			sprintf(buff, "****   %02X", op);
			break;
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
			sprintf(buff, "STRR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			sprintf(buff, "STRA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
			sprintf(buff, "RRL,%d", rv);
			break;
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
			sprintf(buff, "WRTE,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
			sprintf(buff, "BIRR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0xdc: case 0xdd: case 0xde: case 0xdf:
			sprintf(buff, "BIRA,%d %s", rv, ADR(pc));
			pc+=2;
			break;
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
			sprintf(buff, "COMZ,%d", rv);
			break;
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
			sprintf(buff, "COMI,%d %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
			sprintf(buff, "COMR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0xec: case 0xed: case 0xee: case 0xef:
			sprintf(buff, "COMA,%s", ABS(rv,pc));
			pc+=2;
			break;
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
			sprintf(buff, "WRTD,%d", rv);
			break;
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
			sprintf(buff, "TMI,%d  %s", rv, IMM(pc));
			pc+=1;
			break;
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
			sprintf(buff, "BDRR,%d %s", rv, REL(pc));
			pc+=1;
			break;
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			sprintf(buff, "BDRA,%d %s", rv, ADR(pc));
			pc+=2;
			break;
	}
	return pc - PC;
}
