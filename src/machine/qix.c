/***************************************************************************

  machine\qix.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"



extern unsigned char cpucontext[MAX_CPU][100];	/* enough to accomodate the cpu status */

struct m6809context
{
	m6809_Regs	regs;
	int	icount;
	int iperiod;
	int	irq;
};

unsigned char *qix_sharedram;

static int lastcheck2;
extern int yield_cpu;		/* JB 970824 */
extern int saved_icount;	/* JB 970824 */


/* JB 970824 - don't interrupt when we're just yielding */
int qix_data_interrupt(void)
{
	if( yield_cpu )
		return INT_NONE;
	else
		return INT_IRQ;
}

int qix_sharedram_r_1(int offset)
{
	word pc;

	pc = cpu_getpc();
	/* JB 970824 - trap all occurrences where the main cpu is waiting for the video */
	/*             cpu and yield immediately                                        */
	if(pc==0xdd54 || pc==0xcf16 || pc==0xdd5a)
	{
		yield_cpu = TRUE;
		saved_icount = cpu_geticount();
		cpu_seticount (0);
	}
	/* JB 970824 - waiting for an IRQ */
	else if(pc==0xdd4b && RAM[0x8520])
		cpu_seticount (0);

	/* JB 970825 - these loop optimizations work, but incorrectly affect the gameplay */
	/*               (even with speed throttling, play is too fast)                   */
	#if 0
	if(pc==0xdd6b || pc==0xdd73 || pc==0xdd62)
	{
		yield_cpu = TRUE;
		saved_icount = cpu_geticount();
		cpu_seticount (0);
	}
	#endif

	return qix_sharedram[offset];
}

int qix_sharedram_r_2(int offset)
{
	/* ASG - 970726 - CPU 2 sits and spins on offset 37D */
	if (offset == 0x37d)
	{
		int count = cpu_geticount ();

		/* this is a less tight loop; stop if less than 60 cycles apart */
		if (lastcheck2 < count + 60 && cpu_getpc () == 0xc894)
		{
			cpu_seticount (0);
			lastcheck2 = 0x7fffffff;
		}
		else
			lastcheck2 = count;
	}
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



void qix_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S;/* | M6809_FAST_U;*/

	/* Reset the checking flags */
	lastcheck2 = 0x7fffffff;
}
