/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"

static int clock=0;

int arabian_c200(int offset)
{

  static unsigned int i;

  i=0x0;
  return i;
}


int arabian_d7f6(int offset)
{
  int pom;

/*
  Z80_Regs regs;
  Z80_GetRegs(&regs);
  pom2=regs.PC.D;

  switch( pom2 )
  {
  case 0x15aa:
      pom=1;
      break;
  case 0x15b1:
      pom=0;
      break;
  case 0x158e:
      pom=0;
      break;
  case 0x2d0c:
      pom=0;
      break;
  default:
      pom=RAM[0xd7f6];
      break;
  }
*/
  pom = ( (clock & 0xf0) >> 4) ;
  return pom;
}

int arabian_d7f8(int offset)
{
  int pom;

  pom = clock & 0x0f ;
  return pom;
}

int arabian_interrupt(void)
{
  clock = (clock+1) & 0xff;
  return 0;
}
