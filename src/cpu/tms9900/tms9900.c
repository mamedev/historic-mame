//========================================
//  emulate.c
//
//	C/PPC file for TMS9900 emulation
//
//  by R NABET, based on work by Edward Swartz.
//
// Converted for Mame by M.Coates
//========================================


#include "memory.h"
#include "osd_dbg.h"
#include "tms9900.h"
#include "9900stat.h"

INLINE void execute(UINT16 opcode);

UINT16 fetch(void);
UINT16 decipheraddr(UINT16 opcode);
UINT16 decipheraddrbyte(UINT16 opcode);
void contextswitch(UINT16 addr);

void h0000(UINT16 opcode);
void h0200(UINT16 opcode);
void h0400(UINT16 opcode);
void h0800(UINT16 opcode);
void h1000(UINT16 opcode);
void h2000(UINT16 opcode);
void h4000(UINT16 opcode);

/***************************/
/* Mame Interface Routines */
/***************************/

extern  FILE *errorlog;

TMS9900_Regs 	I;
int 	TMS9900_ICount = 0;
UINT8	lastparity = 0;

#define readword(addr)			((cpu_readmem16((addr)) << 8) + cpu_readmem16((addr)+1))
#define writeword(addr,data)	{ cpu_writemem16((addr), ((data)>>8) );cpu_writemem16(((addr)+1),((data) & 0xff));}

#define readbyte(addr)			(cpu_readmem16(addr))
#define writebyte(addr,data)	{ cpu_writemem16((addr),(data)) ; }

#define READREG(reg)			readword(I.WP+reg)
#define WRITEREG(reg,data)		writeword(I.WP+reg,data)

#define IMASK					(I.STATUS & 0x0F)

//**************************************************************************

void TMS9900_reset(void *param)
{
	I.STATUS = 0;
	I.WP=readword(0);
	I.PC=readword(2);
}

void TMS9900_exit(void)
{
	/* nothing to do ? */
}

int TMS9900_execute(int cycles)
{
	TMS9900_ICount = cycles;

    do
    {

		#ifdef MAME_DEBUG
		{
	        extern int mame_debug;

           	if (mame_debug)
            {
            	setstat();

		     	I.FR0  = READREG(R0);
    		    I.FR1  = READREG(R1);
        		I.FR2  = READREG(R2);
	        	I.FR3  = READREG(R3);
		       	I.FR4  = READREG(R4);
    		    I.FR5  = READREG(R5);
        		I.FR6  = READREG(R6);
	        	I.FR7  = READREG(R7);
		       	I.FR8  = READREG(R8);
    		    I.FR9  = READREG(R9);
        		I.FR10 = READREG(R10);
	        	I.FR11 = READREG(R11);
		       	I.FR12 = READREG(R12);
    		    I.FR13 = READREG(R13);
        		I.FR14 = READREG(R14);
	        	I.FR15 = READREG(R15);

                #if 0		/* Trace */
                if (errorlog)
                {
                	fprintf(errorlog,"> PC %4.4x : %4.4x %4.4x : R=%4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x : T=%d\n",I.PC,I.STATUS,I.WP,I.FR0,I.FR1,I.FR2,I.FR3,I.FR4,I.FR5,I.FR6,I.FR7,I.FR8,I.FR9,I.FR10,I.FR11,I.FR12,I.FR13,I.FR14,I.FR15,TMS9900_ICount);
                }
                #endif

				MAME_Debug();
            }
		}
		#endif

        I.IR = fetch();
      	execute(I.IR);

        /* Fudge : I'll put timings in when it works! */

		TMS9900_ICount -= 12 ;

	} while (TMS9900_ICount > 0);

    return cycles - TMS9900_ICount;
}

void TMS9900_getregs(TMS9900_Regs * regs)
{
	setstat();
    *regs = I;
}

void TMS9900_setregs(TMS9900_Regs * regs)
{
    I = *regs;
    getstat();
}

unsigned TMS9900_getpc(void)
{
	return I.PC;
}

unsigned TMS9900_getreg(int regnum)
{
	switch( regnum )
	{
		case  0: return I.PC;
		case  1: return I.IR;
		case  2: return I.WP;
		case  3: return I.STATUS;
#ifdef MAME_DEBUG
		case  4: return I.FR0;
		case  5: return I.FR1;
		case  6: return I.FR2;
		case  7: return I.FR3;
		case  8: return I.FR4;
		case  9: return I.FR5;
		case 10: return I.FR6;
		case 11: return I.FR7;
		case 12: return I.FR8;
		case 13: return I.FR9;
		case 14: return I.FR10;
		case 15: return I.FR11;
		case 16: return I.FR12;
		case 17: return I.FR13;
		case 18: return I.FR14;
		case 19: return I.FR15;
#endif
	}
	return 0;
}

void TMS9900_setreg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case  0: I.PC = val; break;
		case  1: I.IR = val; break;
		case  2: I.WP = val; break;
		case  3: I.STATUS = val; break;
#ifdef MAME_DEBUG
		case  4: I.FR0 = val; break;
		case  5: I.FR1 = val; break;
		case  6: I.FR2 = val; break;
		case  7: I.FR3 = val; break;
		case  8: I.FR4 = val; break;
		case  9: I.FR5 = val; break;
		case 10: I.FR6 = val; break;
		case 11: I.FR7 = val; break;
		case 12: I.FR8 = val; break;
		case 13: I.FR9 = val; break;
		case 14: I.FR10 = val; break;
		case 15: I.FR11 = val; break;
		case 16: I.FR12 = val; break;
		case 17: I.FR13 = val; break;
		case 18: I.FR14 = val; break;
		case 19: I.FR15 = val; break;
#endif
	}
}

/*
 *  These are just hacked to compile and need properly sorting out!
 */

void TMS9900_set_nmi_line(int state)
{
	/* TMS9900 has no dedicated NMI line? */
}

void TMS9900_set_irq_line(int irqline, int state)
{
	I.irq_state = state;
	if (state == CLEAR_LINE)
	{
		if (errorlog) fprintf(errorlog, "TMS9900.C: set_irq_line(CLEAR): Status = %04x, WP = %04x, PC = %04x\n", I.STATUS, I.WP, I.PC) ;
	}
	else
	{
		int intlevel = (*I.irq_callback)(irqline);
		if (intlevel <= IMASK)
		{
			UINT16 oldwp;

			oldwp=I.WP;
			setstat();					/* Make sure status us uptodate before we store it */
			I.WP=readword(intlevel*4);	/* get new workspace */
			WRITEREG(R13,oldwp);		/* store old WP in new R13 */
			WRITEREG(R14,I.PC);			/* store old PC in new R14 */
			WRITEREG(R15,I.STATUS);		/* store old STATUS in R15 */

			I.PC=readword((intlevel*4)+2);	/* get new PC */
		}
    }
}

void TMS9900_set_irq_callback(int (*callback)(int irqline))
{
	I.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *TMS9900_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	TMS9900_Regs *r = (TMS9900_Regs *)context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context && regnum >= CPU_INFO_PC )
		return buffer[which];

    switch( regnum )
	{
		case CPU_INFO_NAME: return "TMS9900";
		case CPU_INFO_FAMILY: return "Texas Instruments 9900";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Converted for Mame by M.Coates (C/PPC file for TMS9900 emulation by R NABET, based on work by Edward Swartz)";
		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->PC); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%04X", r->WP); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM: r->PC += Dasm9900(buffer[which], r->PC); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "$%04x", READ_WORD(&ROM[r->PC<<1])); r->PC++; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->WP & 0x8000 ? 'L':'.',
				r->WP & 0x4000 ? 'A':'.',
				r->WP & 0x2000 ? 'E':'.',
				r->WP & 0x1000 ? 'C':'.',
				r->WP & 0x0800 ? 'V':'.',
				r->WP & 0x0400 ? 'P':'.',
				r->WP & 0x0200 ? 'X':'.',
				r->WP & 0x0100 ? '?':'.',
				r->WP & 0x0080 ? '?':'.',
				r->WP & 0x0040 ? '?':'.',
				r->WP & 0x0020 ? '?':'.',
				r->WP & 0x0010 ? '?':'.',
				r->WP & 0x0008 ? 'I':'.',
				r->WP & 0x0004 ? 'I':'.',
				r->WP & 0x0002 ? 'I':'.',
				r->WP & 0x0001 ? 'I':'.');
            break;
		case CPU_INFO_REG+ 0: sprintf(buffer[which], "PC:%04X",  r->PC); break;
		case CPU_INFO_REG+ 1: sprintf(buffer[which], "IR:%04X",  r->IR); break;
		case CPU_INFO_REG+ 2: sprintf(buffer[which], "WP:%04X",  r->WP); break;
		case CPU_INFO_REG+ 3: sprintf(buffer[which], "ST:%04X",  r->STATUS); break;
#ifdef MAME_DEBUG
		case CPU_INFO_REG+ 4: sprintf(buffer[which], "R0 :%04X",  r->FR0); break;
		case CPU_INFO_REG+ 5: sprintf(buffer[which], "R1 :%04X",  r->FR1); break;
		case CPU_INFO_REG+ 6: sprintf(buffer[which], "R2 :%04X",  r->FR2); break;
		case CPU_INFO_REG+ 7: sprintf(buffer[which], "R3 :%04X",  r->FR3); break;
		case CPU_INFO_REG+ 8: sprintf(buffer[which], "R4 :%04X",  r->FR4); break;
		case CPU_INFO_REG+ 9: sprintf(buffer[which], "R5 :%04X",  r->FR5); break;
		case CPU_INFO_REG+10: sprintf(buffer[which], "R6 :%04X",  r->FR6); break;
		case CPU_INFO_REG+11: sprintf(buffer[which], "R7 :%04X",  r->FR7); break;
		case CPU_INFO_REG+12: sprintf(buffer[which], "R8 :%04X",  r->FR8); break;
		case CPU_INFO_REG+13: sprintf(buffer[which], "R9 :%04X",  r->FR9); break;
		case CPU_INFO_REG+14: sprintf(buffer[which], "R10:%04X",  r->FR10); break;
		case CPU_INFO_REG+15: sprintf(buffer[which], "R11:%04X",  r->FR11); break;
		case CPU_INFO_REG+16: sprintf(buffer[which], "R12:%04X",  r->FR12); break;
		case CPU_INFO_REG+17: sprintf(buffer[which], "R13:%04X",  r->FR13); break;
		case CPU_INFO_REG+18: sprintf(buffer[which], "R14:%04X",  r->FR14); break;
		case CPU_INFO_REG+19: sprintf(buffer[which], "R15:%04X",  r->FR15); break;
#endif
    }
	return buffer[which];
}
//**************************************************************************

void writeCRU(int CRUAddr, int Number, UINT16 Value)
{
	int count;

	if (errorlog) fprintf(errorlog,"PC %4.4x Write CRU %x for %x = %x\n",I.PC,CRUAddr,Number,Value);

	/* Write Number bits from CRUAddr */

    for(count=0;count<Number;count++)
    {
    	cpu_writeport(CRUAddr+count,(Value & 0x01));
        Value >>= 1;
    }
}

UINT16 readCRU(int CRUAddr, int Number)
{
	static int BitMask[] =
    {
    	0, /* filler - saves a subtract to find mask */
    	0x0100,0x0300,0x0700,0x0F00,0x1F00,0x3F00,0x7F00,0xFF00,
    	0x01FF,0x03FF,0x07FF,0x0FFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF
    };

    int Offset,Location,Value;

	if (errorlog) fprintf(errorlog,"Read CRU %x for %x\n",CRUAddr,Number);

    Location = CRUAddr >> 3;
    Offset   = CRUAddr & 07;

    if (Number <= 8)
    {
    	/* Read 16 Bits */
	    Value = (cpu_readport(Location+1) << 8) + cpu_readport(Location);

        /* Allow for Offset */
	    Value >>= Offset;

        /* Mask out what we want */
      	Value = (Value << 8) & BitMask[Number];

        /* And update */
  		return (Value>>8);
    }
    else
    {
    	/* Read 24 Bits */
	    Value = (cpu_readport(Location+2) << 16)
		      + (cpu_readport(Location+1) << 8)
              + cpu_readport(Location);

        /* Allow for Offset */
	    Value >>= Offset;

        /* Mask out what we want */
    	Value &= BitMask[Number];

        /* and Update */
    	return Value;
    }
}

//**************************************************************************

// fetch : read one word at * PC, and increment PC.

UINT16 fetch(void)
{
  register UINT16 value = readword(I.PC);
  I.PC += 2;
  return value;
}

// contextswitch : performs a BLWP, ie change I.PC, I.WP, and save I.PC, I.WP and STAT...
void contextswitch(UINT16 addr)
{
  UINT16 oldWP,oldpc;

  oldWP = I.WP;
  oldpc = I.PC;

  I.WP = readword(addr) & ~1;
  I.PC = readword(addr+2) & ~1;

  WRITEREG(R13, oldWP);
  WRITEREG(R14, oldpc);
  setstat();
  WRITEREG(R15, I.STATUS);
}

// decipheraddr : compute and return the effective adress in word instructions.
//
// NOTA : the LSB is always ignored in word adresses,
// but we do not set to 0 because of XOP...
UINT16 decipheraddr(UINT16 opcode)
{
  register UINT16 ts = opcode & 0x30;
  register UINT16 reg = opcode & 0xF;

  reg += reg;

  if (ts == 0)
    return(reg + I.WP);
  else if (ts == 0x10)
    return(readword(reg + I.WP));
  else if (ts == 0x20)
  {
	register UINT16 imm;

    imm = fetch();
    if (reg)
    {
      return(readword(reg + I.WP) + imm);
    }
    else
      return(imm);
  }
  else /*if (ts == 0x30)*/
  {
	register UINT16 reponse;

    reg += I.WP;      // reg now contains effective address

    reponse = readword(reg);
    writeword(reg, reponse+2); // we increment register content
    return(reponse);
  }
}

// decipheraddrbyte : compute and return the effective adress in byte instructions.
UINT16 decipheraddrbyte(UINT16 opcode)
{
  register UINT16 ts = opcode & 0x30;
  register UINT16 reg = opcode & 0xF;

  reg += reg;

  if (ts == 0)
    return(reg + I.WP);
  else if (ts == 0x10)
    return(readword(reg + I.WP));
  else if (ts == 0x20)
  {
	register UINT16 imm;

    imm = fetch();
    if (reg)
    {
      return(readword(reg + I.WP) + imm);
    }
    else
      return(imm);
  }
  else /*if (ts == 0x30)*/
  {
	register UINT16 reponse;

    reg += I.WP;      // reg now contains effective address

    reponse = readword(reg);
    writeword(reg, reponse+1); // we increment register content
    return(reponse);
  }
}


//*************************************************************************


//==========================================================================
//       Data instructions,                                      >0000->01FF
//==========================================================================
void h0000(UINT16 opcode)
{
  #pragma unused (opcode)
}


//==========================================================================
//       Immediate, Control instructions,                        >0200->03FF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               LI, AI, ANDI, ORI,
//       ----------------------------------              CI, STI.WP, STST,
//       |      o p c o d e     |0| reg # |              LIMI, LI.WPI, IDLE,
//       ----------------------------------              RSET, RTI.WP, CKON,
//                                                       CKOF, LREX
//==========================================================================
void h0200(UINT16 opcode)
{
  register UINT16 addr;
  register UINT16 value;	// used for anything

  addr = opcode & 0xF;
  addr = ((addr + addr) + I.WP) & ~1;

  switch ((opcode & 0x1e0) >> 5)
  {
    case 0:    // LI
      // LI --     Load Immediate
      // *Reg = *PC+
      value = fetch();
      writeword(addr, value);
      setst_lae(value);
      break;
    case 1:    // AI
      // AI --     Add Immediate
      // *Reg += *PC+
      value = fetch();
      wadd(addr, value);
      break;
    case 2:    // ANDI
      // ANDI --     AND Immediate
      // *Reg &= *PC+
      value = fetch();
      value = readword(addr) & value;
      writeword(addr, value);
      setst_lae(value);
      break;
    case 3:    // ORI
      // ORI --     OR Immediate
      // *Reg |= *PC+
      value = fetch();
      value = readword(addr) | value;
      writeword(addr, value);
      setst_lae(value);
      break;
    case 4:    // CI
      // CI --     Compare Immediate
      // status = (*Reg-*PC+)
      value = fetch();
      setst_c_lae(value, readword(addr));
      break;
    case 5:  // STI.WP
      // STI.WP --     STore Workspace Pointer
      // *Reg = I.WP
      writeword(addr, I.WP);
      break;
    case 6:  // STST
      // STST --     STore STatus register
      // *Reg = STAT
      setstat();
      writeword(addr, I.STATUS);
      break;
    case 7: // LI.WPI
      // LI.WPI --     Load Workspace Pointer Immediate
      // I.WP = *PC+
      I.WP = fetch();
      break;
    case 8:  // LIMI
      // LIMI --     Load Interrupt Mask Immediate
      // status&15 |= (*PC+)&15
      value = fetch();
      I.STATUS = (I.STATUS & ~ 0xF) | (value & 0xF);
//    handle9901();
      break;
    case 12:  // RTI.WP
      // RTI.WP --     Return with Workspace Pointer
      // I.WP = R13, I.PC = R14, I.STATUS = R15
      I.STATUS = READREG(R15);
      getstat();
      I.PC = READREG(R14);
      I.WP = READREG(R13);
//    handle9901();
      break;
  }
}

//==========================================================================
//       Single-operand instructions,                            >0400->07FF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               BLI.WP, B, X, CLR,
//       ----------------------------------              NEG, INV, INC, INCT,
//       |      o p c o d e   |TS |   S   |              DEC, DECT, BL, SI.WPB,
//       ----------------------------------              SETO, ABS
//
//==========================================================================
void h0400(UINT16 opcode)
{
  register UINT16 addr = decipheraddr(opcode) & ~1;
  register UINT16 value;	// used for anything

  switch ((opcode &0x3C0) >> 6)
  {
    case 0: // BLI.WP
      // BLI.WP --     Branch with Workspace Pointer
      // Result: I.WP = *S+, I.PC = *S
      //         New R13=old I.WP, New R14=Old PC, New R15=Old STAT
      contextswitch(addr);
      break;
   case 1:  // B
     // B --       Branch
     // I.PC = S
      I.PC = addr;
      break;
    case 2: // X
      // X --       eXecute
      // Executes instruction *S
      execute(readword(addr));
      break;
    case 3: // CLR
      // CLR --       CLeaR
      // *S = 0
      writeword(addr, 0);
      break;
    case 4: // NEG
      // NEG --       NEGate
      // *S = -*S
	  value = - (INT16) readword(addr);
      if (value)
        I.STATUS &= ~ ST_C;
      else
        I.STATUS |= ST_C;
      setst_laeo(value);
      writeword(addr, value);
      break;
    case 5: // INV
      // INV --       INVert
      // *S = ~*S
      value = ~ readword(addr);
      writeword(addr, value);
      setst_lae(value);
      break;
    case 6: // INC
      // INC --       INCrement
      // (*S)++
      wadd(addr, 1);
      break;
    case 7: // INCT
      // INCT --       INCrement by Two
      // (*S) +=2
      wadd(addr, 2);
      break;
    case 8: // DEC
      // DEC --       DECrement
      // (*S)--
      wsub(addr, 1);
      break;
    case 9: // DECT
      // DECT --       DECrement by Two
      // (*S) -= 2
      wsub(addr, 2);
      break;
    case 10:    // BL
      // BL --       Branch and Link
      // IP=S, R11=old IP
      WRITEREG(R11, I.PC);
      I.PC = addr;
      break;
    case 11:    // SI.WPB
      // SI.WPB --       SWaP Bytes
      // *S = swab(*S)
      value = readword(addr);
      value = logical_right_shift(value, 8) | (value << 8);
      writeword(addr, value);
      break;
    case 12:    // SETO
      // SETO --       SET Ones
      // *S = #$FFFF
      writeword(addr, 0xFFFF);
      break;
    case 13:    // ABS
      // ABS --       ABSolute value
      // *S = |*S|
      // clearing ST_C seems to be necessary, although ABS will never set it.
      I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);
      value = readword(addr);

	  if (((INT16) value) > 0)
        I.STATUS |=  ST_L | ST_A;
	  else if (((INT16) value) < 0)
      {
        I.STATUS |= ST_L;
        if (value == 0x8000)
          I.STATUS |= ST_O;
		writeword(addr, - ((INT16) value));
      }
      else
        I.STATUS |= ST_E;

      break;
  }
}


//==========================================================================
//       Shift instructions,                                     >0800->0BFF
//       AND my own instructions,                                >0C00->0FFF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               SRA, SRL, SLA, SRC
//       ----------------------------------              ------------------
//       |  o p c o d e   |   C   |   W   |              DSR, KEYS, SPRI,
//       ----------------------------------              TRAN, INT1, BRK,
//                                                       TIDSR, KEYSLOW,
//                                                       SCREEN
//==========================================================================
void h0800(UINT16 opcode)
{
  register UINT16 addr;
  register UINT16 cnt = (opcode & 0xF0) >> 4;
  register UINT16 value;

  addr = (opcode & 0xF);
  addr = ((addr+addr) + I.WP) & ~1;

  if (cnt == 0)
    cnt = READREG(0) & 0xF;

  // ONLY shift now
  switch ((opcode & 0x700) >> 8)
  {
    case 0:  // SRA
      // SRA --       Shift Right Arithmetic
      // *W >>= C   (*W is filled on the left with a copy of the sign bit)
      value = setst_sra_laec(readword(addr), cnt);
      writeword(addr, value);
      break;
    case 1:  // SRL
      // SRL --       Shift Right Logical
      // *W >>= C   (*W is filled on the left with 0)
      value = setst_srl_laec(readword(addr), cnt);
      writeword(addr, value);
      break;
    case 2: // SLA
      // SLA --       Shift Left Arithmetic
      // *W <<= C
      value = setst_sla_laeco(readword(addr), cnt);
      writeword(addr, value);
      break;
    case 3: // SRC
      // SRC --       Shift Right Circular
      // *W = rightcircularshift(*W, C)
      value = setst_src_laec(readword(addr), cnt);
      writeword(addr, value);
      break;
  }
}


//==========================================================================
//       Jump, CRU bit instructions,                             >1000->1FFF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               JMP, JLT, JLE, JEQ,
//       ----------------------------------              JHE, JGT, JNE, JNC,
//       |   o p c o d e  | signed offset |              JOC, JNO, JL,JH,JOP
//       ----------------------------------              ---------------------
//                                                       SBO, SBZ, TB
//
//==========================================================================
void h1000(UINT16 opcode)
{
  // we convert 8 bit signed word offset to a 16 bit effective word offset.
  register INT16 offset = ((INT8) opcode);


  switch ((opcode & 0xF00) >> 8)
  {
    case 0:  // JMP
      // JMP --    Unconditional jump
      // I.PC += offset
      I.PC += (offset + offset);
      break;
    case 1: // JLT
      // JLT --    Jump if Less Than (arithmetic)
      // if (A==0 && EQ==0), I.PC += offset
      if (! (I.STATUS & (ST_A | ST_E)))
        I.PC += (offset + offset);
      break;
    case 2: // JLE
      // JLE --    Jump if Lower or Equal (logical)
      // if (L==0 || EQ==1), I.PC += offset
      if ((! (I.STATUS & ST_L)) || (I.STATUS & ST_E))
        I.PC += (offset + offset);
      break;
    case 3: // JEQ
      // JEQ --    Jump if Equal
      // if (EQ==1), I.PC += offset
      if (I.STATUS & ST_E)
        I.PC += (offset + offset);
      break;
	      case 4: // JHE
      // JHE --    Jump if Higher or Equal (logical)
      // if (L==1 || EQ==1), I.PC += offset
      if  (I.STATUS & (ST_L | ST_E))
        I.PC += (offset + offset);
      break;
    case 5: // JGT
      // JGT --    Jump if Greater Than (arithmetic)
      // if (A==1), I.PC += offset
      if (I.STATUS & ST_A)
        I.PC += (offset + offset);
      break;
    case 6: // JNE
      // JNE --    Jump if Not Equal
      // if (EQ==0), I.PC += offset
      if (! (I.STATUS & ST_E))
        I.PC += (offset + offset);
      break;
    case 7: // JNC
      // JNC --    Jump if No Carry
      // if (C==0), I.PC += offset
      if (! (I.STATUS & ST_C))
        I.PC += (offset + offset);
      break;
    case 8: // JOC
      // JOC --    Jump On Carry
      // if (C==1), I.PC += offset
      if (I.STATUS & ST_C)
        I.PC += (offset + offset);
      break;
    case 9: // JNO
      // JNO --    Jump if No Overflow
      // if (OV==0), I.PC += offset
      if (! (I.STATUS & ST_O))
        I.PC += (offset + offset);
      break;
    case 10: // JL
      // JL --      Jump if Lower (logical)
      // if (L==0 && EQ==0), I.PC += offset
      if (! (I.STATUS & (ST_L | ST_E)))
        I.PC += (offset + offset);
      break;
    case 11: // JH
      // JH --      Jump if Higher (logical)
      // if (L==1 && EQ==0), I.PC += offset
      if ((I.STATUS & ST_L) && ! (I.STATUS & ST_E))
        I.PC += (offset + offset);
      break;
    case 12: // JOP
      // JOP --    Jump On (Odd) Parity
      // if (P==1), I.PC += offset
      {
        // let's set ST_P
        int i;
		UINT8 a;

        a = lastparity;

        I.STATUS &= ~ ST_P;

        for (i=0; i<8; i++)   // 8 bits to test
        {
          if (a & 1)      // If current bit is set
            I.STATUS ^= ST_P; // we change ST_P bit
          a >>= 1;        // Next bit.
        }
      }

      if (I.STATUS & ST_P)
        I.PC += (offset + offset);

      break;
    case 13: // SBO
      // SBO --    Set Bit to One
      // CRU Bit = 1
      writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 1);
      break;
    case 14: // SBZ
      // SBZ --    Set Bit to Zero
      // CRU Bit = 0
      writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 0);
      break;
    case 15: // TB
      // TB --      Test Bit
      // EQ = (CRU Bit == 1)
      setst_e(readCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1) & 1, 1);
      break;
  }
}


//==========================================================================
//       General and One-Register instructions                   >2000->3FFF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               COC, CZC, XOR,
//       ----------------------------------              LDCR, STCR, XOP,
//       |   opcode   |   D   |TS |   S   |              MPY, DIV
//       ----------------------------------
//
//==========================================================================
void h2000(UINT16 opcode)
{
  register UINT16 dest = (opcode & 0x3C0) >> 6;
  register UINT16 src;
  register UINT16 value;

  if (opcode < 0x3000 || opcode >= 0x3800)  // not LDCR and STCR
  {
    src = decipheraddr(opcode);

    switch ((opcode & 0x1C00) >> 10)
    {
      case 0:   // COC
        // COC --       Compare Ones Corresponding
        // status E bit = (S&D == S)
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        value = readword(src);
//      setst_lae(value);   // only l&a are useful
        setst_e(value & readword(dest), value);
        break;
      case 1:   // CZC
        // CZC --       Compare Zeroes Corresponding
        // status E bit = (S&~D == S)
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        value = readword(src);
//      setst_lae(value);   // only l&a are useful
        setst_e(value & (~ readword(dest)), value);
        break;
      case 2:   // XOR
        // XOR --       eXclusive OR
        // D ^= S
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        value = readword(dest) ^ readword(src);
        setst_lae(value);
        writeword(dest,value);
        break;
      case 3:   // XOP
        // XOP --       eXtended OPeration
        // I.WP = *(40h+D), I.PC = *(42h+D)
        // New R13=old I.WP, New R14=Old IP, New R15=Old STAT
        // New R11=S
        // Xop bit set
        contextswitch(0x40 + (dest << 2));
        I.STATUS |= ST_X;
        WRITEREG(R11, src);
        break;
      /*case 4:*/   // LDCR is implemented elsewhere
      /*case 5:*/   // STCR is implemented elsewhere
      case 6: // MPY
        // MPY --       MultiPlY  (unsigned)
        // Results:  D:D+1 = D*S
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        {
		  UINT32 prod = readword(dest) * readword(src);
          writeword(dest, prod >> 16);
          writeword(dest+2, prod);
        }
        break;
      case 7: // DIV
        // DIV --       DIVide  (unsigned)
        // D = D/S  D+1 = D%S
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        {
		  UINT16 d = readword(src);
		  UINT16 hi = readword(dest);
		  UINT32 divq = (((UINT32) hi) << 16) | readword(dest+2);

          if (d <= hi)
            I.STATUS |= ST_O;
          else
          {
            writeword(dest, divq/d);
            writeword(dest+2, divq%d);
            I.STATUS &= ~ST_O;
          }
        }
    }
  }
  else  // LDCR and STCR
  {
    if (dest == 0)
      dest = 16;

    if (dest <= 8)
      src = decipheraddrbyte(opcode);
    else
      src = decipheraddr(opcode) & ~1;

    if (opcode < 0x3400)
    {   // LDCR
      // LDCR --       LoaD into CRu
      // CRU R12--CRU R12+D-1 set to S
      if (dest <= 8)
      {
        value = readbyte(src);
        setst_byte_laep(value);
        writeCRU(((READREG(R12) & 0x1FFE) >> 1), dest, value);
      }
      else
      {
        value = readword(src);
        setst_lae(value);
        writeCRU(((READREG(R12) & 0x1FFE) >> 1), dest, value);
      }
    }
    else
    {   // STCR
      // STCR --       Store from CRu
      // S = CRU R12--CRU R12+D-1
      if (dest<=8)
      {
        value = readCRU(((READREG(R12) & 0x1FFE) >> 1), dest);
        setst_byte_laep(value);
        writebyte(src, value);
      }
      else
      {
        value = readCRU(((READREG(R12) & 0x1FFE) >> 1), dest);
        setst_lae(value);
        writeword(src, value);
      }
    }
  }
}


//==========================================================================
//       Two-Operand instructions                               >4000->FFFF
//--------------------------------------------------------------------------
//
//         0 1 2 3-4 5 6 7+8 9 A B-C D E F               SZC, SZCB, S, SB,
//       ----------------------------------              C, CB, A, AB, MOV,
//       |opcode|B|TD |   D   |TS |   S   |              MOVB, SOC, SOCB
//       ----------------------------------
//
//==========================================================================
void h4000(UINT16 opcode)
{
  register UINT16 src;
  register UINT16 dest;
  register UINT16 value;

  if (opcode & 0x1000)
  {   // byte instruction
    src = decipheraddrbyte(opcode);
    dest = decipheraddrbyte(opcode >> 6);
  }
  else
  {   // word instruction
    src = decipheraddr(opcode) & ~1;
    dest = decipheraddr(opcode >> 6) & ~1;
  }

  switch ((opcode >> 12) &0x000F)   // ((opcode & 0xF000) >> 12)
  {
    case 4: // SZC
      // SZC --       Set Zeros Corresponding
      // D &= ~S
      value = readword(dest) & (~ readword(src));
      setst_lae(value);
      writeword(dest, value);
      break;
    case 5: // SZCB
      // SZCB --       Set Zeros Corresponding, Byte
      // D &= ~S
      value = readbyte(dest) & (~ readbyte(src));
      setst_byte_laep(value);
      writebyte(dest, value);
      break;
    case 6: // S
      // S --       Subtract
      // D -= S
      value = setst_sub_laeco(readword(dest), readword(src));
      writeword(dest, value);
      break;
    case 7: // SB
      // SB --       Subtract, Byte
      // D -= S
      value = setst_subbyte_laecop(readbyte(dest), readbyte(src));
	  writebyte(dest, value);
      break;
    case 8: // C
      // C --       Compare
      // status = (D - S)
      setst_c_lae(readword(dest), readword(src));
      break;
    case 9: // CB
      // CB --       Compare Bytes
      // I.STATUS = (D - S)
      value = readbyte(src);
      setst_c_lae(readbyte(dest)<<8, value<<8);
      lastparity = value;
      break;
    case 10: // A
      // A --       Add
      // D += S
      value = setst_add_laeco(readword(dest), readword(src));
      writeword(dest, value);
      break;
    case 11: // AB
      // AB --       Add, Byte
      // D += S
      value = setst_addbyte_laecop(readbyte(dest), readbyte(src));
      writebyte(dest, value);
      break;
    case 12: // MOV
      // MOV --       MOVe
      // Results:  D = S
      value = readword(src);
      setst_lae(value);
      writeword(dest, value);
      break;
    case 13: // MOVB
      // MOVB --       MOVe Bytes
      // Results:  D = S
      value = readbyte(src);
      setst_byte_laep(value);
      writebyte(dest, value);
      break;
    case 14: // SOC
      // SOC --       Set Ones Corresponding
      // D |= S
      value = readword(dest) | readword(src);
      setst_lae(value);
      writeword(dest, value);
      break;
    case 15: // SOCB
      // SOCB --       Set Ones Corresponding, Byte
      // D |= S
      value = readbyte(dest) | readbyte(src);
      setst_byte_laep(value);
      writebyte(dest, value);
      break;
  }
}


INLINE void execute(UINT16 opcode)
{
  static void (* jumptable[128])(UINT16) =
  {
    &h0000,&h0200,&h0400,&h0400,&h0800,&h0800,&h0800,&h0800,
    &h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
    &h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
    &h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000
  };

  (* jumptable[opcode >> 9])(opcode);
}




