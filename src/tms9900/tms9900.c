/*******************************************************
 *
 *      Portable TMS9900 emulator
 *
 *      Based on original TI99/4A TMS9900 engine by Ton Brouwer
 *	Adapted for use with DJGPP\MAME by Andy Jones
 *
 *******************************************************/

#define VERBOSE

#ifdef  VERBOSE
#include <stdio.h>
#include "driver.h"
#endif

#include "memory.h"
#include "osd_dbg.h"
#include "tms9900/tms9900.h"
#include "tms9900/tms9900c.h"

#ifndef INLINE
#define INLINE static inline
#endif

extern FILE *errorlog ;

static  TMS9900_Regs I;

int     TMS9900_ICount = 0;

#define RDMEMW(addr)		( ( cpu_readmem16((addr)) << 8 ) + cpu_readmem16((addr)+1))
#define WRMEMW(addr,data)	{ cpu_writemem16((addr), (data)>>8 );cpu_writemem16((addr)+1, (data)&0xff ) ;}
#define RDMEMB(addr)		( cpu_readmem16((addr)) << 8)
#define WRMEMB(addr,data)	{ cpu_writemem16((addr),(data)>>8) ; }

/********************************************************************************/
/* Standard MAME cpu interface functions.                                       */
/********************************************************************************/

void    TMS9900_Reset(void)
{
	I.STATUS=I.IR=0;
	I.WP=RDMEMW(0);
	I.PC=RDMEMW(2);

//	if (errorlog) fprintf(errorlog, "TMS9900.C: Reset : Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;
}

int     TMS9900_Execute(int cycles)
{
        TMS9900_ICount = cycles;

//	if (errorlog) fprintf(errorlog, "TMS9900.C: Execute : Entry : Cycles = %04x\n", cycles ) ;

        do
        {

#ifdef  MAME_DEBUG
		{
		        extern int mame_debug;

                	if (mame_debug) MAME_Debug();
		}
#endif

		I.IR=RDMEMW(I.PC);		/* fetch instruction		*/
//		if (errorlog) fprintf(errorlog, "TMS9900.C: Execute : PC: %04x IR: %04x WP: %04x ST: %04x R0: %04x R1: %04x R2: %04x R3: %04x R4: %04x R5: %04x R6: %04x R7: %04x R8: %04x R9: %04x R10: %04x R11: %04x R12: %04x R13: %04x R14: %04x R15: %04x\n", I.PC, I.IR, I.WP, get_status(), RDMEMW(R0), RDMEMW(R1), RDMEMW(R2), RDMEMW(R3), RDMEMW(R4), RDMEMW(R5), RDMEMW(R6), RDMEMW(R7), RDMEMW(R8), RDMEMW(R9), RDMEMW(R10), RDMEMW(R11), RDMEMW(R12), RDMEMW(R13), RDMEMW(R14), RDMEMW(R15)) ;
//		if (errorlog) fprintf(errorlog, "TMS9900.C: Execute : PC: %04x IR: %04x WP: %04x ST: %04x\n", I.PC, I.IR, I.WP, get_status()) ;
		I.PC+=2;                  	/* update program counter 	*/
		(*opcode_table[I.IR>>8])();	/* execute instruction 		*/

		TMS9900_ICount -= 12 ; /* Fudge : I have no idea of instruction timings */

	} while (TMS9900_ICount > 0);

//	if (errorlog) fprintf(errorlog, "TMS9900.C: Execute : Exit : Cycles = %04x\n", cycles - TMS9900_ICount ) ;

        return cycles - TMS9900_ICount;
}

void    TMS9900_GetRegs(TMS9900_Regs * regs)
{
	I.STATUS = get_status();

//	if (errorlog) fprintf(errorlog, "TMS9900.C: GetRegs : Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;

        *regs = I;
}

void    TMS9900_SetRegs(TMS9900_Regs * regs)
{
        I = *regs;

	set_status (I.STATUS);

//	if (errorlog) fprintf(errorlog, "TMS9900.C: SetRegs : Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;
}

int     TMS9900_GetPC(void)
{
//	if (errorlog) fprintf(errorlog, "TMS9900.C: GetPC : Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;

        return I.PC;
}

/*********************************************************/
// there are 4 levels of interupt + reset
// PC vector and WP location are stored as follows
// 0000 PC,WP Reset (level 0)
// 0004 PC,WP Interupt level 1
// 0008 PC,WP Interupt level 2
// 000C PC,WP Interupt level 3
// 0010 PC,WP Interupt level 4
// Lower 4 bits of the status register are the interupt mask
// Only interupts with a value lower than or equal to the mask are enabled
// So if the mask is set to 3 only interupts 1,2 and 3 occur. Level 0 is always enabled.
// When the CPU is interupted -
// WP set to appropriate location
// PC set to appropriate vector
// Old WP stored in R13
// Old PC stored in R14
// Old ST stored in R15
// Interupt mask decremented by 1 (this will stop the cpu being interepted unless its higher priority)
/*********************************************************/

void    TMS9900_Cause_Interrupt(int intlevel)
{
	if (intlevel <= IMASK)
	{

		word oldwp;

		oldwp=I.WP;
		I.STATUS = get_status();	/* Make sure status us uptodate before we store it */
		I.WP=RDMEMW(intlevel*4);	/* get new workspace */
		WRMEMW(R13,oldwp);        	/* store old WP in new R13 */
		WRMEMW(R14,I.PC);		/* store old PC in new R14 */
		WRMEMW(R15,I.STATUS);		/* store old STATUS in R15 */

		set_mask((IMASK&0xF)-1);	/* Decrement the interupt mask */

		I.STATUS = get_status();	/* Update the status again */

		I.PC=RDMEMW((intlevel*4)+2);	/* get new PC */
	}
}

void    TMS9900_Clear_Pending_Interrupts(void)
{
	if (errorlog) fprintf(errorlog, "TMS9900.C: Clear_Pending_Interrupts : Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;
}

/********************************************************************************/
/* Low opcodes jump redirection                                                 */
/********************************************************************************/

void TAB2(void)
{
	(*low_opcode_table[I.IR>>5])();
}

/********************************************************************************/
/* This part provides the operands for the different types of opcodes.          */
/********************************************************************************/

void typeIoperands(void)
{
	get_source_operand();
	get_dest_operand();
	if (BYTE_OPERATION)
        {
//		if (errorlog) fprintf(errorlog, "TMS9900.C: TypeIoperand : Byte operation\n" ) ;

		SourceVal=RDMEMB(SourceOpAdr);
	        DestVal=RDMEMB(DestOpAdr);
	}
	else
	{
		SourceOpAdr&=0xFFFE;
		DestOpAdr&=0xFFFE;

		SourceVal=RDMEMW(SourceOpAdr);
	        DestVal=RDMEMW(DestOpAdr);
	}
}

void typeIIIoperands(void)
{
	get_source_operand();
	SourceVal=RDMEMW(SourceOpAdr&0xFFFE);
	DestOpAdr=I.WP+((I.IR>>5)&0x1e);
	DestVal=RDMEMW(DestOpAdr);
}

void typeIVoperands(void)
{
	get_source_operand();
	SourceVal=RDMEMW(SourceOpAdr&=0xFFFE);
	DestVal=(I.IR>>6)&0xF;
	CRUBaseAdr = ((RDMEMW(R12)&0x1FFE)>>1);
}

void typeVoperands(void)
{
	SourceOpAdr=I.WP+((I.IR&0xF)<<1);
	SourceVal=RDMEMW(SourceOpAdr);
	DestVal=(I.IR>>4)&0xF;
	if (!DestVal)
	{
		DestVal=RDMEMW(I.WP)&0xF;
		if (!DestVal) DestVal=16;
	}
}

void typeVIoperands(void)
{
	get_source_operand();
	SourceVal=RDMEMW(SourceOpAdr&=0xFFFE);
}

void typeVIIIoperands(void)
{
	SourceOpAdr=I.WP+((I.IR&0xF)<<1);
	SourceVal=RDMEMW(SourceOpAdr);
	DestVal=RDMEMW(I.PC);
	I.PC+=2;
}

void typeIXoperands(void)
{
	SourceOpAdr=I.WP+((I.IR&0xF)<<1);
}

void typeXoperands(void)
{
	SourceVal=RDMEMW(I.PC);
	I.PC+=2;
}

/********************************************************************************/
/* Decode the source operand. Use address modification indicated by bit 2 and 3 */
/* of the instruction register.                                                 */
/********************************************************************************/

void get_source_operand(void)
{
	switch((I.IR>>4)&3)
	{

 /* WS Register direct - (Contents of register) */

	    case 0: 	SourceOpAdr=I.WP+((I.IR&0xF)<<1);
			break;

 /* Indirect WS Register (Contents of memory location pointed to by register)*/

	    case 1: 	SourceOpAdr=RDMEMW(I.WP+((I.IR&0xF)<<1));
			break;

/* Direct (Contents of memory address specified) */
/* Indexed ('Direct' address + contents of WS register - when not R0) */

	    case 2: 	SourceOpAdr=RDMEMW(I.PC); I.PC+=2;
			if (I.IR&0xF) SourceOpAdr+=RDMEMW(I.WP+((I.IR&0xF)<<1));
			break;

/* Indirect WS Register Auto Increment (as '1' but increment register contents afterwards) */

	    case 3:	SourceOpAdr=RDMEMW(I.WP+((I.IR&0xF)<<1));
			WRMEMW(I.WP+((I.IR&0xF)<<1),SourceOpAdr+((BYTE_OPERATION) ? 1:2));
			break;
	}

//	if (errorlog) fprintf(errorlog, "TMS9900.C: get_source_operand : Address Mode = %1x, Srce Address = %04x\n", (I.IR>>4)&3, SourceOpAdr ) ;

}

void get_dest_operand(void)
{
	switch((I.IR>>10)&3)
	{
	case 0: 	DestOpAdr=I.WP+((I.IR>>5)&0x1E); /* Register direct */
			break;
	case 1: 	DestOpAdr=RDMEMW(I.WP+((I.IR>>5)&0x1E));
			break;
	case 2: 	DestOpAdr=RDMEMW(I.PC); I.PC+=2;
			if (I.IR&0x3c0) DestOpAdr+=RDMEMW(I.WP+((I.IR>>5)&0x1E));
			break;
	case 3: 	DestOpAdr=RDMEMW(I.WP+((I.IR>>5)&0x1E));
			WRMEMW(I.WP+((I.IR>>5)&0x1E),DestOpAdr+((BYTE_OPERATION) ? 1:2));
			break;
	}

//	if (errorlog) fprintf(errorlog, "TMS9900.C: get_dest_operand : Address Mode = %1x, Dest Address = %04x\n", (I.IR>>10)&3, DestOpAdr ) ;
}

/********************************************************************************/
/* Instruction bodies.                                                          */
/********************************************************************************/

void SRA()
{
	typeVoperands();
	if (DestVal>1) SourceVal=(signed short int)SourceVal>>(DestVal-1);
	CY=SourceVal&1;
	SourceVal=(signed short int)SourceVal>>1;
	set_status(SourceVal);
	WRMEMW(SourceOpAdr,SourceVal);
	TMS9900_ICount -= TMS9900_CYCLES_SRA ;
}

void SRL()
{
	typeVoperands();
	if (DestVal>1) SourceVal>>=DestVal-1;
	CY=SourceVal&1;
	SourceVal>>=1;
	set_status(SourceVal);
	WRMEMW(SourceOpAdr,SourceVal);
}

void SLA()
{
	typeVoperands();
	OV=((((signed short int)0x8000>>DestVal) &SourceVal)!=0);
	if (DestVal>1) SourceVal<<=DestVal-1;
	CY=((SourceVal&0x8000)!=0);
	SourceVal<<=1;
	set_status(SourceVal);
	WRMEMW(SourceOpAdr,SourceVal);
}

void SRC()
{
	typeVoperands();
	SourceVal=(word)(SourceVal>>DestVal)|(SourceVal<<(16-DestVal));
	CY=((SourceVal&0x8000)!=0);
	set_status(SourceVal);
	WRMEMW(SourceOpAdr,SourceVal);
}

void SZC()
{
	typeIoperands();
	DestVal&=~SourceVal;
	set_status(DestVal);
	WRMEMW(DestOpAdr,DestVal);
}

void SZCB()
{
	typeIoperands();
	DestVal&=(~SourceVal|0xFF);
	set_status(DestVal&0xFF00);
	set_parity(DestVal);
	WRMEMB(DestOpAdr,DestVal);
}

void S()
{
	typeIoperands();
	DestVal=add(DestVal,1+~SourceVal);
	set_status(DestVal);
	WRMEMW(DestOpAdr,DestVal);
}

void SB()
{
	typeIoperands();
	DestVal=(DestVal&0xFF)|add(DestVal&0xFF00,1+~(SourceVal&0xFF00));
	set_status(DestVal&0xFF00);
	set_parity(DestVal);
	WRMEMB(DestOpAdr,DestVal);
}

void C()
{
	typeIoperands();
	compare(SourceVal,DestVal);
}

void CB()
{
	typeIoperands();
	compare(SourceVal&0xFF00,DestVal&0xFF00);
	set_parity(SourceVal);
}

void A()
{
	typeIoperands();
	DestVal=add(SourceVal,DestVal);
	set_status(DestVal);
	WRMEMW(DestOpAdr,DestVal);
}

void AB()
{
	typeIoperands();
	SourceVal=add(SourceVal&0xFF00,DestVal&0xFF00);
	set_status(SourceVal&0xFF00);
	set_parity(SourceVal);
	WRMEMB(DestOpAdr,DestVal);
}

void MOV()
{
	typeIoperands();
	set_status(SourceVal);
	WRMEMW(DestOpAdr,SourceVal);
}

void MOVB()
{
	typeIoperands();
	set_status(SourceVal&0xFF00);
	set_parity(SourceVal);
	WRMEMB(DestOpAdr,SourceVal);
}

void SOC()
{
	typeIoperands();
	DestVal|=SourceVal;
	set_status(DestVal);
	WRMEMW(DestOpAdr,DestVal);
}

void SOCB()
{
	typeIoperands();
	DestVal|=SourceVal&0xFF00;
	set_status(DestVal&0xFF00);
	set_parity(DestVal);
	WRMEMB(DestOpAdr,DestVal);
}

void JMP() { 				do_jmp(); }
void JLT() { if (AGT==0 && EQ==0) 	do_jmp(); }
void JLE() { if (LGT==FALSE || EQ==1) 	do_jmp(); }
void JEQ() { if (EQ==1) 		do_jmp(); }
void JHE() { if (LGT==1 || EQ==1) 	do_jmp(); }
void JGT() { if (AGT==1) 		do_jmp(); }
void JNE() { if (EQ==0) 		do_jmp(); }
void JNC() { if (CY==0) 		do_jmp(); }
void JOC() { if (CY==1) 		do_jmp(); }
void JNO() { if (OV==0) 		do_jmp(); }
void JL()  { if (LGT==0 && EQ==0) 	do_jmp(); }
void JH()  { if (LGT==1) 		do_jmp(); }
void JOP() { if (OP==1) 		do_jmp(); }

void do_jmp(void)
{
	I.PC+=(I.IR&0xFF)<<1;
	if (I.IR&0x0080) I.PC-=0x200;
}

void SBO()
{
	CRUBaseAdr = ((RDMEMW(R12)&0x1FFE)>>1);

	if (errorlog) fprintf(errorlog, "TMS9900.C: PC = %04x : SBO : BaseAddress = %04x, Offset = %x\n", I.PC-2, CRUBaseAdr, (I.IR&0xFF)) ;
}

void SBZ()
{
	CRUBaseAdr = ((RDMEMW(R12)&0x1FFE)>>1);

	if (errorlog) fprintf(errorlog, "TMS9900.C: PC = %04x : SBZ : BaseAddress = %04x, Offset = %x\n", I.PC-2, CRUBaseAdr, (I.IR&0xFF)) ;
}

void TB()
{
	CRUBaseAdr = ((RDMEMW(R12)&0x1FFE)>>1);

	if (errorlog) fprintf(errorlog, "TMS9900.C: PC = %04x : TB : BaseAddress = %04x, Offset = %x\n", I.PC-2, CRUBaseAdr, (I.IR&0xFF)) ;

	EQ=0;
}

void COC()
{
	typeIIIoperands();
	EQ=( (SourceVal&~DestVal)==0 );
}

void CZC()
{
	typeIIIoperands();
	EQ=( (SourceVal&DestVal)==0 );
}

void XOR()
{
	typeIIIoperands();
	DestVal^=SourceVal;
	set_status(DestVal);
	WRMEMW(DestOpAdr,DestVal);
}

void XOP()
{
	word oldwp,oldpc;
	typeIVoperands();
	oldwp=I.WP;
	oldpc=I.PC;
	I.WP=RDMEMW(0x40+(DestVal<<2));
	I.PC=RDMEMW(0x42+(DestVal<<2));
	WRMEMW(R11,SourceVal);
	WRMEMW(R13,oldwp);
	WRMEMW(R14,oldpc);
	I.STATUS = get_status();
	WRMEMW(R15,I.STATUS);
	X=1;
}

void LDCR()
{
	EQ=0;
	typeIVoperands();
	set_status(SourceVal);

	if (errorlog) fprintf(errorlog, "TMS9900.C: PC = %04x : LDCR : Value = %04x, Count = %x -> BaseAddress = %04x\n", I.PC-2, SourceVal, DestVal, CRUBaseAdr ) ;
}

void STCR()
{
	typeIVoperands();
//	WRMEMW(SourceOpAdr,0xFFFF);
	WRMEMW(SourceOpAdr,0x0E00);
	set_status(SourceVal);

	if (errorlog) fprintf(errorlog, "TMS9900.C: PC = %04x : STCR : Address = %04x, Count = %x <- BaseAddress = %04x\n", I.PC-2, SourceOpAdr, DestVal, CRUBaseAdr ) ;
}

void MPY()
{
	unsigned long int m;
	typeIIIoperands();
	m=(unsigned long int)SourceVal*(unsigned long int)DestVal;
	WRMEMW(DestOpAdr,(word)(m>>16));
	WRMEMW(DestOpAdr+2,(word)(m&0xFFFF));
}

void DIV()
{
	unsigned long int m;
	typeIIIoperands();
	m=((unsigned long int)(DestVal)<<16)+
	  (unsigned long int)RDMEMW(DestOpAdr+2);
	OV=(SourceVal<=DestVal);
	if ((SourceVal)&&!OV)
	{
		WRMEMW(DestOpAdr,(word)(m/SourceVal));
		WRMEMW(DestOpAdr+2,(word)(m%SourceVal));
	}
}

void LI()
{
	typeVIIIoperands();
	WRMEMW(SourceOpAdr,DestVal);
	set_status(DestVal);
}

void AI()
{
	typeVIIIoperands();
	DestVal=add(SourceVal,DestVal);
	WRMEMW(SourceOpAdr,DestVal);
	set_status(DestVal);
}

void ANDI()
{
	typeVIIIoperands();
	DestVal&=SourceVal;
	WRMEMW(SourceOpAdr,DestVal);
	set_status(DestVal);
}

void ORI()
{
	typeVIIIoperands();
	DestVal|=SourceVal;
	WRMEMW(SourceOpAdr,DestVal);
	set_status(DestVal);
}

void CI()
{
	typeVIIIoperands();
	compare(SourceVal,DestVal);
}

void STWP()
{
	typeIXoperands();
	WRMEMW(SourceOpAdr,I.WP);
}

void STST()
{
	typeIXoperands();
	I.STATUS = get_status();
	WRMEMW(SourceOpAdr,I.STATUS);
}

void LWPI()
{
	typeXoperands();
	I.WP=SourceVal;
}

void LIMI()
{
	typeXoperands();
	set_mask(SourceVal);
}

void RTWP()
{
	I.PC = RDMEMW(R14);
	I.STATUS = RDMEMW(R15);
	I.WP = RDMEMW(R13);

	set_status(I.STATUS);				//Done for interupt stuff
	set_parity(I.STATUS);				//Done for interupt stuff
	set_mask(I.STATUS);				//Done for interupt stuff
}

void BLWP()
{
	word oldwp;
	I.STATUS = get_status();
	typeVIoperands();
	oldwp=I.WP;
	I.WP=SourceVal;			/* get new workspace */
	WRMEMW(R13,oldwp);        	/* store old WP in new R13 */
	WRMEMW(R14,I.PC);		/* store old PC in new R14 */
	WRMEMW(R15,I.STATUS);		/* store old STATUS in R15 */
	I.PC=RDMEMW(SourceOpAdr+2);	/* get new PC :-) */
}

void B()
{
	typeVIoperands();
	I.PC=SourceOpAdr;
}

void eX()
{
	typeVIoperands();
	I.IR=SourceVal;
        (*opcode_table[I.IR>>8])();	/* execute instruction */
}

void CLR()
{
	typeVIoperands();
	WRMEMW(SourceOpAdr,0);
}

void NEG()
{
        typeVIoperands();
	DestVal=add(~SourceVal,1);
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void INV()
{
	typeVIoperands();
	DestVal=~SourceVal;
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void INC()
{
	typeVIoperands();
	DestVal=add(SourceVal,1);
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void INCT()
{
	typeVIoperands();
	DestVal=add(SourceVal,2);
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void DEC()
{
	typeVIoperands();
	DestVal=add(SourceVal,0xFFFF);
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void DECT()
{
	typeVIoperands();
	DestVal=add(SourceVal,0xFFFE);
	set_status(DestVal);
	WRMEMW(SourceOpAdr,DestVal);
}

void BL()
{
	typeVIoperands();
	WRMEMW(R11,I.PC);
	I.PC=SourceOpAdr;
}

void SWPB()
{
	typeVIoperands();
	SourceVal=((SourceVal&0xFF)<<8) | (SourceVal>>8);
	WRMEMW(SourceOpAdr,SourceVal);
}

void SETO()
{
	typeVIoperands();
	WRMEMW(SourceOpAdr,0xFFFF);
}

void ABSv()
{
	typeVIoperands();
	set_status(SourceVal);
	OV=(SourceVal==0x8000);
	if (SourceVal&0x8000) SourceVal=~SourceVal+1;
	WRMEMW(SourceOpAdr,SourceVal);
}

void IDLE()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - IDLE at address >%04x\n",I.PC-2);
}

void RSET()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - RSET at address >%04x\n",I.PC-2);
}

void CKON()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - CKON at address >%04x\n",I.PC-2);
}

void CKOF()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - CKOF at address >%04x\n",I.PC-2);
}

void LREX()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - LREX at address >%04x\n",I.PC-2);
}

void ILL()
{
	if (errorlog) fprintf(errorlog, "\nTMS9900.C - Illegal opcode %04x at address >%04x\n",I.IR,I.PC-2);
}

/********************************************************************************/
/* This part provides the status register handling                              */
/********************************************************************************/

word get_status(void)
{
	return (LGT<<15)|(AGT<<14)|(EQ<<13)|(CY<<12)|(OV<<11)|(OP<<10)|(X<<9)|(IMASK&0xf);
}

void set_status(word value)
{
	EQ=(value==0);
	LGT=!EQ;
	AGT=LGT && (!(value&SIGN));
}

void set_parity(word value)
{
	OP= ((PARCHECK >> (value>>12)) ^ (PARCHECK >> ((value>>8)&15))) &1;
}

void set_mask(word value)
{
	IMASK = value&0xf;
}

/********************************************************************************/
/* Support functions                                                            */
/********************************************************************************/

void compare(word source,word dest)
{
	LGT=(source>dest);
	EQ=(source==dest);
	AGT=((signed short int)source>(signed short int)dest);
}

word add(word a, word b)
{
	word c;
	c=a+b;
	CY=(((a&b)&SIGN)!=0) || (((a&SIGN) != (b&SIGN)) &&((c&SIGN)==0));
	OV=((~a&~b&c&SIGN) || (a&b&~c&SIGN))!=0;
	return(c);
}
