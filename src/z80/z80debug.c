/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                              Z80Debug.c                              ***/
/***                                                                      ***/
/*** This file contains a simple single step debugger which is called     ***/
/*** when DEBUG is #defined and Z80_Trace is true                         ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#ifdef DEBUG

#include "Z80.h"
#include <stdio.h>
#include <string.h>
#include "Z80Dasm.h"

void Z80_Debug (Z80_Regs *R)
{
 byte buf[5];
 char flags[]="SZ.H.VNC";
 char S[20],f[9];
 int i;
 for (i=0;i<5;++i) { buf[i]=Z80_RDMEM((R->PC.D+i)&0xFFFF); }
 Z80_Dasm (buf,S);
 for (i=strlen(S);i<19;++i) S[i]=' ';
 S[19]='\0';
 for (i=7;i>=0;--i) { f[7-i]=(R->AF.D&(1<<i))? flags[7-i]:'.'; }
 f[8]='\0';
 printf ("%04X: %sAT SP: %04X %04X  FLAGS: %s\n",R->PC.D,S,
          Z80_RDMEM(R->SP.D)+Z80_RDMEM((R->SP.D+1)&0xFFFF)*256,
          Z80_RDMEM((R->SP.D+2)&0xFFFF)+Z80_RDMEM((R->SP.D+3)&0xFFFF)*256,f);
 printf ("%25sAF:%04X BC:%04X DE:%04X HL:%04X IX:%04X IY:%04X\n",
           "",R->AF.D,R->BC.D,R->DE.D,R->HL.D,R->IX.D,R->IY.D);
}

#endif
