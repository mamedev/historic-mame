/*
	Interface routine for 68kem <-> Mame

*/

#include "driver.h"
#include "M68000.H"
#include "osd_dbg.h"

#ifdef WIN32
#define CONVENTION __cdecl
#else
#define CONVENTION
#endif

extern void CONVENTION M68KRUN(void);
extern void CONVENTION M68KRESET(void);


/********************************************/
/* Interface routines to link Mame -> 68KEM */
/********************************************/

void MC68000_Reset(void)
{
	memset(&regs,0,sizeof(regs));

    regs.a[7] = regs.isp = get_long(0);
    regs.pc   = get_long(4) & 0xffffff;
    regs.sr_high = 0x27;
   	regs.sr = 0x2700;

    M68KRESET();
}


int  MC68000_Execute(int cycles)
{
	if (regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

  	MC68000_ICount = cycles;

#ifdef MAME_DEBUG

    do
    {
        #if 0				/* Trace */

        if (errorlog)
        {
			int mycount, areg, dreg;

            areg = dreg = 0;
	        for (mycount=7;mycount>=0;mycount--)
            {
            	areg = areg + regs.a[mycount];
                dreg = dreg + regs.d[mycount];
            }

           	fprintf(errorlog,"=> %8x %8x ",areg,dreg);
            fprintf(errorlog,"%6x %4x %d\n",regs.pc,regs.sr,MC68000_ICount);
        }
        #endif

		{
			extern int mame_debug;

			if (mame_debug)
            {
				MAME_Debug();
            }
		}

		M68KRUN();
    }
	while (MC68000_ICount >= 0);

#else

	M68KRUN();

#endif /* MAME_DEBUG */

    return (cycles - MC68000_ICount);
}


static regstruct Myregs;

void MC68000_SetRegs(MC68000_Regs *src)
{
 	regs = src->regs;
}

void MC68000_GetRegs(MC68000_Regs *dst)
{
  	dst->regs = regs;
}

void MC68000_Cause_Interrupt(int level)
{
	if (level >= 1 && level <= 7)
        regs.IRQ_level |= 1 << (level-1);
}

void MC68000_Clear_Pending_Interrupts(void)
{
	regs.IRQ_level = 0;
}

int  MC68000_GetPC(void)
{
    return regs.pc;
}

#ifdef MAME_DEBUG

	/* Need cycle table, to make identical to C core */

	#include "cycletbl.h"

#endif

