/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6809.h"

static int tutbankaddress = 0x10000;
static int tut_bank = 0;


int tutankhm_interrupt(void)
{
	static int count;

	count = (count + 1) % 2;

	if (count) return INT_IRQ;
	else return INT_NONE;
}

void tutankhm_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S;    /* thanks to Dave Dahl for suggestion */
}


int tut_rnd_r( int offset )
{
	return ( rand() & 0xff );
}


void tut_bankselect_w(int offset,int data)
{
        if (tut_bank == data) return;

	tutbankaddress = 0x10000 + ( data * 0x1000 );
	tut_bank = data;
}


int tut_bankedrom_r(int offset)
{
	return RAM[tutbankaddress + offset];
}

