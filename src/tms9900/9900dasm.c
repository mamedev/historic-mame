/*
	DECODE(*addr,*op,*s,*d,*ts,*td)


	Disassemble instruction at ADDR, print out disassembly.

	Return	OP		= defining bits of instruction (minus s/d/ts/td)
			ADDR	= new address after 'executing'
			S		= source value (address or offset)
			D		= destination value (address or offset)
			TS		= type of source
			TD		= type of dest
*/

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "memory.h"

#define OP(addr) ( ( cpu_readmem16((addr)) << 8 ) + cpu_readmem16((addr)+1))
#define ARG(addr) ( ( cpu_readmem16((addr)) << 8 ) + cpu_readmem16((addr)+1))

/* #define OP(A)   cpu_readop16(A)*/
/*#define ARG(A)  cpu_readop_arg16(A)*/

#define	REG		0
#define	IND		1
#define	ADDR		2
#define	INC		3
#define	IMMED		4
#define	CNT		5
#define	JUMP		6
#define	OFFS		7

const char *regs_9900[5]={"X","Y","U","S","PC"};

int Dasm9900 (char *buffer, int pc)
{
	word	op,s,d,sa,da,addr;
	byte	ts,td;
	char	*inst;

	inst = "";

	addr = pc >> 1 ;
	op=OP(pc);pc=pc+2;

	if (op<0x200)	      	// data
	{
		sprintf(buffer,"DATA");
	}
	else if (op<0x2a0)
	{
		ts=REG;
		s=op&15;
		sa=ARG(pc);pc=pc+2;
		op&=0x1e0;

		switch (op>>5)
		{
			case 0 :inst="LI  "; break;
			case 1 :inst="AI  "; break;
			case 2 :inst="ANDI"; break;
			case 3 :inst="ORI "; break;
			case 4 :inst="CI  "; break;
		}

		sprintf(buffer,"%s R%d,>%04X",inst,s,sa);
	}
	else if (op<0x2e0)
	{
		ts=REG;
		s=op&15;
		op&=0x1e0;

		switch (op>>5)
		{
			case 5 :inst="STWP"; break;
			case 6 :inst="STST"; break;
		}

		sprintf(buffer,"%s R%d",inst,s);
	}
	else if (op<0x320)
	{
		ts=IMMED;
		sa=ARG(pc);pc=pc+2;
		op&=0x1e0;

		switch (op>>5)
		{
			case 7 :inst="LWPI"; break;
			case 8 :inst="LIMI"; break;
		}

		sprintf(buffer,"%s >%04X",inst,sa);
	}
	else if (op<0x400)
	{
		op&=0x1e0;

		switch (op>>5)
		{
			case 9  :inst="DATA"; break;
			case 10 :inst="IDLE"; break;
			case 11 :inst="RSET"; break;
			case 12 :inst="RTWP"; break;
			case 13 :inst="CKON"; break;
			case 14 :inst="CKOF"; break;
			case 15 :inst="LREX"; break;
		}

		sprintf(buffer,"%s",inst);
	}
	else if (op<0x800)
	{
		ts=(op&0x30)>>4;
		s=op&15;
		op&=0x3c0;

		switch (op>>6)
		{
			case 0  :inst="BLWP"; break;
			case 1  :inst="B   "; break;
			case 2  :inst="X   "; break;		/* aaack! */
			case 3  :inst="CLR "; break;
			case 4  :inst="NEG "; break;
			case 5  :inst="INV "; break;
			case 6  :inst="INC "; break;
			case 7  :inst="INCT"; break;
			case 8  :inst="DEC "; break;
			case 9  :inst="DECT"; break;
			case 10 :inst="BL  "; break;
			case 11 :inst="SWPB"; break;
			case 12 :inst="SETO"; break;
			case 13 :inst="ABS "; break;
			case 14 :
			case 15 :inst="DATA"; break;
		}

		switch (ts)
		{
			case REG  : sprintf(buffer,"%s R%d",inst,s); break;
			case IND  : sprintf(buffer,"%s *R%d",inst,s); break;

			case ADDR :
			{
				sa=ARG(pc); pc = pc + 2;

				if (s==0)
				{
					sprintf(buffer,"%s @>%04X",inst,sa);
				}
				else
				{
					sprintf(buffer,"%s @>%04X(R%d)",inst,sa,s);
				}
			}
			break;

			case INC  : sprintf(buffer,"%s *R%d+",inst,s); break;
		}
	}
	else if (op<0xc00)
	{
		ts=REG;
		s=op&15;
		td=CNT;
		d=(op&0xf0)>>4;
		op&=0x700;

		switch (op>>8)
		{
			case 0 :inst="SRA "; break;
			case 1 :inst="SRL "; break;
			case 2 :inst="SLA "; break;
			case 3 :inst="SRC "; break;
		}

		sprintf(buffer,"%s R%d,%d",inst,s,d);
	}
	else if (op<0x1000)
	{
		op&=0x7e0;
		sprintf(buffer,"DATA ...");
	}
	else if (op<0x2000)
	{
		ts=(op<0x1d00 ? JUMP : OFFS);
		s=op&0x00ff;
		op&=0x0f00;

		switch (op>>8)
		{
			case 0  :inst="JMP "; break;
			case 1  :inst="JLT "; break;
			case 2  :inst="JLE "; break;
			case 3  :inst="JEQ "; break;
			case 4  :inst="JHE "; break;
			case 5  :inst="JGT "; break;
			case 6  :inst="JNE "; break;
			case 7  :inst="JNC "; break;
			case 8  :inst="JOC "; break;
			case 9  :inst="JNO "; break;
			case 10 :inst="JL  "; break;
			case 11 :inst="JH  "; break;
			case 12 :inst="JOP "; break;
			case 13 :inst="SBO "; break;
			case 14 :inst="SBZ "; break;
			case 15 :inst="TB  "; break;
		}

		sprintf(buffer,"%s >%04X",inst,(op<0x0d00 ? (pc+((signed char)s)*2) : s));
	}
	else if (op<0x4000 && !(op>=0x3000 && op<0x3800))
	{
		ts=(op&0x30)>>4;
		s=(op&15);
		td=REG;
		d=(op&0x3c0)>>6;
		op&=0x1c00;

		switch (op>>10)
		{
			case 0 :inst="COC "; break;
			case 1 :inst="CZC "; break;
			case 2 :inst="XOR "; break;
			case 3 :inst="XOP "; break;
			case 6 :inst="MPY "; break;
			case 7 :inst="DIV "; break;
		}

		switch (ts)
		{
			case REG  : sprintf(buffer,"%s R%d,R%d",inst,s,d); break;
			case IND  : sprintf(buffer,"%s *R%d,R%d",inst,s,d); break;

			case ADDR :
			{
				sa = ARG(pc) ; pc = pc + 2 ;

				if (s==0)
				{
					sprintf(buffer,"%s @>%04X,R%d",inst,sa,d);
				}
				else
				{
					sprintf(buffer,"%s @>%04X(R%d),R%d",inst,sa,s,d);
				}
			}
			break;

			case INC  : sprintf(buffer,"%s *R%d+,R%d",inst,s,d); break;
		}
	}
	else if (op>=0x3000 && op<0x3800)
	{
		ts=(op&0x30)>>4;
		s=(op&15);
		td=REG;
		d=(op&0x3c0)>>6;

		inst=(((op & 0xfc00)<0x3400) ? "LDCR" : "STCR");

		if (d==0) d=16;
		{
			op&=0x1c00;
		}

		switch (ts)
		{
			case REG  : sprintf(buffer,"%s R%d,%d",inst,s,d); break;
			case IND  : sprintf(buffer,"%s *R%d,%d",inst,s,d); break;

			case ADDR :
			{
				sa=ARG(pc); pc = pc + 2;

				if (s==0)
				{
					sprintf(buffer,"%s @>%04X,%d",inst,sa,d);
				}
				else
				{
					sprintf(buffer,"%s @>%04X(R%d),%d",inst,sa,s,d);
				}
			}
			break;

			case INC  : sprintf(buffer,"%s *R%d+,%d",inst,s,d); break;
		}
	}
	else
	{
		int	os ;

		os = 0;

		ts=(op&0x30)>>4;
		s=(op&15);
		td=(op&0x0c00)>>10;
		d=(op&0x3c0)>>6;

		op&=0xf000;

		switch (op>>12)
		{
			case 4  :inst="SZC "; break;
			case 5  :inst="SZCB"; break;
			case 6  :inst="S   "; break;
			case 7  :inst="SB  "; break;
			case 8  :inst="C   "; break;
			case 9  :inst="CB  "; break;
			case 10 :inst="A   "; break;
			case 11 :inst="AB  "; break;
			case 12 :inst="MOV "; break;
			case 13 :inst="MOVB"; break;
			case 14 :inst="SOC "; break;
			case 15 :inst="SOCB"; break;
		}

		switch (ts)
		{
			case REG  : { sprintf(buffer,"%s R%d",inst,s) ; os = 8 ; } break;//arj//
			case IND  : { sprintf(buffer,"%s *R%d",inst,s); os = 8 ; } break;

			case ADDR :
			{
				sa=ARG(pc); pc = pc + 2 ;

				if (s==0)
				{
					sprintf(buffer,"%s @>%04X",inst,sa); os = 11 ;
				}
				else
				{
					sprintf(buffer,"%s @>%04X(R%d)",inst,sa,s); os = 15 ;
				}
			}
			break;

			case INC  : { sprintf(buffer,"%s *R%d+",inst,s); os = 9 ; } break;
		}

		switch (td)
		{
			case REG  : sprintf(buffer+os,",R%d",d); break;
			case IND  : sprintf(buffer+os,",*R%d",d); break;

			case ADDR :
			{
				da=ARG(pc); pc = pc + 2 ;

				if (d==0)
				{
					sprintf(buffer+os,",@>%04X",da);
				}
				else
				{
					sprintf(buffer+os,",@>%04X(R%d)",da,d);
				}
			}
			break;

			case INC  : sprintf(buffer+os,",*R%d+",d); break;
		}
	}

	return	(pc-(addr+addr));
}

