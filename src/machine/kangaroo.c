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
#include "Z80.h"

static int clock=0;


/* I have no idea what the security chip is nor whether it really does,
   this just seems to do the trick -V-
*/

int kangaroo_sec_chip_r(int offset)
{
  clock = (clock << 1) + 1;
  clock&=0xff;
  return (clock);
}

void kangaroo_sec_chip_w(int offset, int val)
{
  clock = val & 0xff;

}

/* This is not really necessary ? after all it does nothing ;) -V-
*/
int kangaroo_interrupt(void)
{
  /* clock = (clock+1) & 0xff; */
  return 0;
}
