/* CHANGELOG
        97/04/xx        renamed the arabian.c and modified it to suit
                        kangaroo. -V-
*/

/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

static int kangaroo_clock=0;


/* I have no idea what the security chip is nor whether it really does,
   this just seems to do the trick -V-
*/

READ_HANDLER( kangaroo_sec_chip_r )
{
/*  kangaroo_clock = (kangaroo_clock << 1) + 1; */
  kangaroo_clock++;
  return (kangaroo_clock & 0x0f);
}

WRITE_HANDLER( kangaroo_sec_chip_w )
{
/*  kangaroo_clock = val & 0x0f; */
}
