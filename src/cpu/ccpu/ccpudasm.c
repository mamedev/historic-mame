/* extra functions */

/*
 * disassemble() appends a disassembly of the specified address to
 * the end of the specified text buffer.
 */
void disassemble (char *buffer, unsigned int maxlen, unsigned int address)
{
  char ambuffer [ 40 ];   /* text buffer for addressing mode values */
  char opbuffer [ 40 ];   /* text buffer for opcodes etc. */
  char flbuffer [ 80 ];   /* text buffer for flag dump */
  CINEBYTE opcode;
  CINEBYTE opsize = 1;
  int iter = 0;

  /* which opcode in opcode table? (includes immediate data) */
  opcode = CCPU_FETCH (address);

  /* build addressing mode (value to opcode) */

	memset(ambuffer, 0, sizeof(ambuffer));
  switch (amodes [ opcode_table [ opcode ].ot_amode ].ad_amode) {

  /* 4-bit immediate value */
  case AIM4:
  case IM4:
    sprintf (ambuffer, "#$%X", opcode & 0x0F);
    break;

  /* 4-bit extended immediate value */
  case AIM4X:
    sprintf (ambuffer, "#$%X", (opcode & 0x0F) << 8);
    break;

  /* 8-bit immediate value */
  case AIM8:
    sprintf (ambuffer, "#$%02X", CCPU_FETCH (address + 1));
    opsize ++; /* required second byte for value */
    break;

  /* [m] -- indirect through memory */
  case AINDM:
    sprintf (ambuffer, "AINDM/Error");
    break;

  /* [i] -- indirect through 'I' */
  case AIRG:
  case IRG:
    sprintf (ambuffer, "[i]");
    break;

  /* extended 12-bit immediate value */
  case AIMX:
    sprintf (ambuffer, "AIMX/Error");
    break;

  /* no special params */
  case ACC:
  case AXLT:
  case IMP:
    ambuffer [ 0 ] = '\0';
    break;

  /* 12-bit immediate value */
  case IM12:
    sprintf (ambuffer, "#$%03X",
	      (CCPU_FETCH (address) & 0x0F) +
	      (CCPU_FETCH (address + 1) & 0xF0) +
	      ((CCPU_FETCH (address + 1) & 0x0F) << 8));
    opsize ++;
    break;

  /* display address of direct addressing modes */
  case ADIR:
  case DIR:
    sprintf (ambuffer, "$%X", opcode & 0x0F);
    break;

  /* jump through J register */
  case JUMP:
    sprintf (ambuffer, "<jump>");
    break;

  /* extended jump */
  case JUMPX:
    sprintf (ambuffer, "JUMPX/Error");
    break;

  } /* switch on addressing mode */

  /* build flags dump */
  sprintf (flbuffer,
	    "A=%03X B=%03X I=%03XZ J=%03X P=%X " \
	    "A0=%02X %02X N%X O%X %c%c%c%c",
	    register_A, register_B,
	    register_I << 1,               /* use the <<1 for Zonn style */
	    register_J, register_P, GETA0(),
	    GETFC() /* ? 'C' : 'c' */,
	    cmp_new, cmp_old,
	    ((state == state_A) || (state == state_AA)) ? 'A' : ' ',
	    (state == state_AA) ? 'A' : ' ',
	    ((state == state_B) || (state == state_BB)) ? 'B' : ' ',
	    (state == state_BB) ? 'B' : ' ');

  /* build the final output string. Format is as follows:
   * ADDRESS: OPCODE-ROM-DUMP\tOPCODE VALUE\tFLAGS
   */

  /* append complete opcode ROM dump */
	memset(opbuffer, 0, sizeof(opbuffer));
  for (iter = 0; iter < opsize; iter++) {
    sprintf (opbuffer + (iter * 3), " %02X", CCPU_FETCH (address + iter));
  }

  /* create final output */
  sprintf (buffer, "%04X:%-6s : %-4s %-6s : %s",
	    address, opbuffer,
	    opcodes [ opcode_table [ opcode ].ot_opcode ].od_name,
	    ambuffer,
	    flbuffer);

  return;
}
