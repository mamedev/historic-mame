/***************************************************************************

  machine\qix.c

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

unsigned char *qix_sharedram;

int qix_sharedram_r(int offset)
{
	return qix_sharedram[offset];
}

void qix_sharedram_w(int offset,int data)
{
	qix_sharedram[offset] = data;
}

void qix_video_firq_w(int offset, int data)
{
	/* generate firq for video cpu */
	struct m6809context *ctxt;

	ctxt = (struct m6809context *)cpucontext[1];
	ctxt->irq = INT_FIRQ;
}

void qix_data_firq_w(int offset, int data)
{
	/* generate firq for data cpu */
	struct m6809context *ctxt;
	
	ctxt = (struct m6809context *)cpucontext[0];
	ctxt->irq = INT_FIRQ;
}

/* Because all the scanlines are drawn at once, this has no real
   meaning. But the game sometimes waits for the scanline to be $0
   and other times for it to be $ff, so we fake it. Ugly, huh?
*/
int qix_scanline_r(int offset)
{
	static unsigned char hack = 0x00;
	
	/* scanline is always $0 or $ff */
	if( hack==0x00 ) hack = 0xff; else hack = 0x00;
	return hack;
}

/*
int m6821_r(int offset)
{
	return 0xff;
}
*/

/* Video CPU ignores external vblank interrupts */
int qix_interrupt_video(void)
{
	return INT_NONE;
}

/* JB 970526 */
int qix_init_machine(const char *gamename)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S;
	return 0;
}