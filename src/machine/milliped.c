/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int milliped_IN_r(int offset)
{
    int res;


    res = readinputport(offset);

    if ( cpu_geticount() > 4000 )
        return (res & 0x70) | (readtrakport(offset) & 0x8f);
    else
        return res;
}
