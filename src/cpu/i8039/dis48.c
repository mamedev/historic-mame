/*
** $Id:$
**
** File: dis48.c -- disassembler for 8048/8039 microcontroller
**
** NOTE: this file is '#include'd by cpu48.c with the symbol DIS48_INCLUDE
** 	defined.
**
** This file is Copyright 1996 Michael Cuddy, Fen's Ende Sofware.
** Redistribution is allowed in source and binary form as long as both
** forms are distributed together with the file 'README'.  This copyright
** notice must also accompany the files.
**
** This software should be considered a small token to all of the
** emulator authors for thier dilligence in preserving our Arcade and
** Computer history.
**
** Michael Cuddy, Fen's Ende Software.
** 11/25/1996
**
*/
#include <stdio.h>
#include <string.h>

#include "8039dasm.c"

int main(int argc,char *argv[])
{
  FILE *F;
  long Counter;
  int len, offset;
  int N,I;
  byte Buf[16];
  char S[128];

  if(argc<2)
  {
    puts("MCS-48 Disassembler 1.1 by Michael Cuddy, Fen's Ende Software (C)1996");
    puts("Usage: dis48 <input file> [ <start-addr> [ <len> ] ]");
    return(0);
  }

  if(!(F=fopen(argv[1],"rb")))
  { printf("\n%s: Can't open file %s\n",argv[0],argv[1]);return(1);
  }
      argv++; argc--;
  if (argv[1]) {
      offset = strtol(argv[1],NULL,0);
      printf("offset = %d (%s)\n",offset,argv[1]);
      argv++; argc--;
  } else {offset = 0; }
  if (argv[1]) {
      len = strtol(argv[1],NULL,0);
      printf("len = %d, %s\n",len,argv[1]);
      argv++; argc--;
  } else { len = 0x7FFFFFFF; }

  Counter=0L;N=0;
  if (fseek(F,offset,0) != 0) {
      fprintf(stderr,"Error seeking to offset %d\n",offset);
      exit(1);
  }
  Counter = offset;
  while(N+=fread(Buf+N,1,16-N,F))
  {
    int ii;
    if(N<16) memset(Buf+N,0,16-N);
    I=Dasm8039(S,Buf);
    printf("%08lX: ", Counter);
    for (ii = 0; ii < I; ii++) {
	printf("%02.2x ",Buf[ii]);
    }
    for (; ii < 4; ii++) { printf("   "); }
    printf("\t%s\n",S);
    Counter+=I;N-=I;
    len -= I; if (len <= 0) break;
    if(N) memcpy(Buf,Buf+16-N,N);
  }

  fclose(F);return(0);
}
