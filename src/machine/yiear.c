/***************************************************************************

  machine\yiear.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#ifdef __MWERKS__
	#include "m6809.h"
#else
	#include "m6809/m6809.h"
#endif

extern unsigned char cpucontext[MAX_CPU][100];	/* enough to accomodate the cpu status */

struct m6809context
{
	m6809_Regs	regs;
	int	icount;
	int iperiod;
	int	irq;
};

void yiear_firq_w(int offset, int data)
{
	struct m6809context *ctxt;
	
	ctxt = (struct m6809context *)cpucontext[0];
	ctxt->irq = INT_FIRQ;
}

int yiear_init_machine(const char *gamename)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S;
	return 0;
}