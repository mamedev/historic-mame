/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"

static int clock=0;
static int port0e=0;
static int port0f=0;

int arabian_d7f6(int offset)
{
  int pom;
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


int arabian_input_port(int offset)
{
  int pom;

  if (port0f & 0x10)  /* if 1 read the switches */
  {

    switch(offset)
    {
    case 0:
	pom = readinputport(2);
	break;
    case 1:
	pom = readinputport(3);
	break;
    case 2:
	pom = readinputport(4);
	break;
    case 3:
	pom = readinputport(5);
	break;
    case 4:
	pom = readinputport(6);
	break;
    case 5:
	pom = readinputport(7);
	break;
    case 6:
	pom = arabian_d7f6(offset);
	break;
    case 8:
	pom = arabian_d7f8(offset);
	break;
    default:
	pom = RAM[ 0xd7f0 + offset ];    
     break;
    }

  }
  else  /* if bit 4 of AY port 0f==0 then read RAM memory instead of switches */
  {
    pom = RAM[ 0xd7f0 + offset ];
  }

  return pom;
}



void moja(int port, int val)
{
  Z80_Regs regs;
  static int lastr;

  Z80_GetRegs(&regs);


  if (regs.BC.D==0xc800)
  {
    AY8910_control_port_0_w(port,val);
    lastr=val;
  }
  else
  {
    if ( (lastr==0x0e) || (lastr==0x0f) )
    {
      if (lastr==0x0e)
        port0e=val;
      if (lastr==0x0f)
        port0f=val;
    }
    else
      AY8910_write_port_0_w(port,val);
  }

}
