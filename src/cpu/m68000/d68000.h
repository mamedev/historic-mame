#ifndef D68000__HEADER
#define D68000__HEADER

/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                DEBABELIZER
 *                                Version 1.0
 *
 * A portable Motorola M68000 disassembler.
 * Copyright 1999 Karl Stenerud.  All rights reserved.
 *
 * This code is freeware and may be freely used as long as this copyright
 * notice remains unaltered in the source code and any binary files
 * containing this code in compiled form.
 *
 * The latest version of this code can be obtained at:
 * http://milliways.scas.bcit.bc.ca/~karl/musashi
 */


/* ======================================================================== */
/* ============================= INSTRUCTIONS ============================= */
/* ======================================================================== */
/* 1. edit d68kconf.h and modify according to your needs.
 * 2. Implement in your host program the functions defined in
 *    "FUNCTIONS CALLED BY THE DISASSEMBLER" located later in this file.
 * 3. Your first call to m68000_disassemble will initialize the disassembler.
 */


/* ======================================================================== */
/* ================= FUNCTIONS CALLED BY THE DISASSEMBLER ================= */
/* ======================================================================== */

/* You will have to implement these functions */

/* read memory */
int m68000_read_memory_32 (int address);
int m68000_read_memory_16 (int address);
int m68000_read_memory_8  (int address);



/* ======================================================================== */
/* ====================== FUNCTIONS TO ACCESS THE CPU ===================== */
/* ======================================================================== */

/* Disassemble 1 instructionat pc.  Stores disassembly in str_buff and returns
 * the size of the instruction in bytes.
 */
int m68000_disassemble(char* str_buff, int pc);



/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* D68000__HEADER */
