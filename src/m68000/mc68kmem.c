#include "M68000.h"
#include "driver.h"
#include "cpuintrf.h"
#include "readcpu.h"
#include "osd_dbg.h"
#include <stdio.h>
/* BFV 061298 - added include for instruction timing table */
#include "cycletbl.h"

/* LBO 090597 - added these lines and made them extern in cpudefs.h */
int movem_index1[256];
int movem_index2[256];
int movem_next[256];
UBYTE *actadr;

regstruct regs, lastint_regs;

union flagu intel_flag_lookup[256];
union flagu regflags;

extern int cpu_interrupt(void);
extern void BuildCPU(void);

#define MC68000_interrupt() (cpu_interrupt())

#define ReadMEM(A) (cpu_readmem24(A))
#define WriteMEM(A,V) (cpu_writemem24(A,V))

int MC68000_ICount;
int pending_interrupts;

static int InitStatus=0;

int opcode ;

extern FILE * errorlog;



void m68k_dumpstate(void)
{
   int i;
   CPTR nextpc;
   for(i = 0; i < 8; i++){
      printf("D%d: %08x ", i, regs.d[i].D);
      if ((i & 3) == 3) printf("\n");
   }
   for(i = 0; i < 8; i++){
      printf("A%d: %08x ", i, regs.a[i]);
      if ((i & 3) == 3) printf("\n");
   }
    if (regs.s == 0) regs.usp = regs.a[7];
   if (regs.s && regs.m) regs.msp = regs.a[7];
   if (regs.s && regs.m == 0) regs.isp = regs.a[7];
   printf("USP=%08x ISP=%08x MSP=%08x VBR=%08x SR=%08x\n",
	  regs.usp,regs.isp,regs.msp,regs.vbr,regs.sr);
   printf ("T=%d%d S=%d M=%d N=%d Z=%d V=%d C=%d IMASK=%d\n",
	   regs.t1, regs.t0, regs.s, regs.m,
	   NFLG, ZFLG, VFLG, CFLG, regs.intmask);
   for(i = 0; i < 8; i++){
	printf("FP%d: %g ", i, regs.fp[i]);
      if ((i & 3) == 3) printf("\n");
   }
   printf("N=%d Z=%d I=%d NAN=%d\n",
	  (regs.fpsr & 0x8000000) != 0,
	  (regs.fpsr & 0x4000000) != 0,
	  (regs.fpsr & 0x2000000) != 0,
	  (regs.fpsr & 0x1000000) != 0);
   MC68000_disasm(m68k_getpc(), &nextpc, 1);
   printf("Next PC = 0x%0x\n", nextpc);
}






static void initCPU(void)
{
    int i,j;

    for (i = 0 ; i < 256 ; i++) {
      for (j = 0 ; j < 8 ; j++) {
       if (i & (1 << j)) break;
      }
     movem_index1[i] = j;
     movem_index2[i] = 7-j;
     movem_next[i] = i & (~(1 << j));
    }
    for (i = 0; i < 256; i++) {
         intel_flag_lookup[i].flags.c = !!(i & 1);
         intel_flag_lookup[i].flags.z = !!(i & 64);
         intel_flag_lookup[i].flags.n = !!(i & 128);
         intel_flag_lookup[i].flags.v = 0;
    }

}

void Exception(int nr, CPTR oldpc)
{
	MC68000_ICount -= exception_cycles[nr];

   MakeSR();

   if(!regs.s) {
   	  regs.usp=regs.a[7];
      regs.a[7]=regs.isp;
      regs.s=1;
   }

   regs.a[7] -= 4;
   put_long (regs.a[7], m68k_getpc ());
   regs.a[7] -= 2;
   put_word (regs.a[7], regs.sr);
   m68k_setpc(get_long(regs.vbr + 4*nr));

   regs.t1 = regs.t0 = regs.m = 0;
}

INLINE void Interrupt68k(int level)
{
   int ipl=(regs.sr&0xf00)>>8;
   if(level>ipl)
   {
   	Exception(24+level,0);
   	pending_interrupts &= ~(1 << (level + 24));
   	regs.intmask = level;
   	MakeSR();
   }
}

void Initialisation(void) {
   /* Init 68000 emulator */
   BuildCPU();
   initCPU();
}

void MC68000_Reset(void)
{
if (!InitStatus)
{
	Initialisation();
	InitStatus=1;
}

   regs.a[7]=get_long(0);
   m68k_setpc(get_long(4));

   regs.s = 1;
   regs.m = 0;
   regs.stopped = 0;
   regs.t1 = 0;
   regs.t0 = 0;
   ZFLG = CFLG = NFLG = VFLG = 0;
   regs.intmask = 7;
   regs.vbr = regs.sfc = regs.dfc = 0;
   regs.fpcr = regs.fpsr = regs.fpiar = 0;

   pending_interrupts = 0;
}


void MC68000_SetRegs(MC68000_Regs *src)
{

	regs = src->regs ;

{	int sr = regs.sr ;

	NFLG = (sr >> 3) & 1;
	ZFLG = (sr >> 2) & 1;
	VFLG = (sr >> 1) & 1;
	CFLG = sr & 1;
	pending_interrupts = src->pending_interrupts;
}}

void MC68000_GetRegs(MC68000_Regs *dst)
{
	regs.sr = (regs.sr & 0xfff0) | (NFLG << 3) | (ZFLG << 2) | (VFLG << 1) | CFLG;
	dst->regs = regs;
	dst->pending_interrupts = pending_interrupts;
}


void MC68000_Cause_Interrupt(int level)
{
	if (level >= 1 && level <= 7)
		pending_interrupts |= 1 << (24 + level);
}


void MC68000_Clear_Pending_Interrupts(void)
{
	pending_interrupts &= ~0xff000000;
}


int  MC68000_GetPC(void)
{
	return regs.pc;
}



/* Execute one 68000 instruction */

int MC68000_Execute(int cycles)
{
	if ((pending_interrupts & MC68000_STOP) && !(pending_interrupts & 0xff000000))
		return cycles;

	MC68000_ICount = cycles;
	do
	{
#ifdef MAME_DEBUG
		{
			extern int mame_debug;
			if (mame_debug)
				MAME_Debug();
		}
#endif

		if (pending_interrupts & 0xff000000)
		{
			int level, mask = 0x80000000;
			for (level = 7; level; level--, mask >>= 1)
				if (pending_interrupts & mask)
					break;
			Interrupt68k (level);
		}

{	/* ASG 980413 */
	extern int previouspc;
	previouspc = regs.pc;
}
	#if MC68000ASM_DEBUG
	{
	void MC68000_MiniTrace(unsigned long *d, unsigned long *a, unsigned long pc, unsigned long sr, int icount);
	MakeSR();
	MC68000_MiniTrace(regs.d, regs.a, regs.pc, regs.sr, MC68000_ICount);
	}
	#endif

		#ifdef ASM_MEMORY
			opcode=nextiword_opcode();
		#else
			opcode=nextiword();
		#endif

/* BFV 061298 - removed "MC68000_ICount -= 12;" added index lookup for timing table */
		MC68000_ICount -= cycletbl[opcode];
		cpufunctbl[opcode]();

	}
	while (MC68000_ICount > 0);

   return (cycles - MC68000_ICount);
}








UBYTE *allocmem(long size, UBYTE init_value) {
   UBYTE *p;
   p=(UBYTE *)malloc(size);
   if (!p) {
      printf("No enough memory !\n");
   }
   return(p);
}


