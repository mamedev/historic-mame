/***************************************************************************

	cpuintrf.c

	Core CPU interface functions and definitions.

	Cleanup phase 1: split into two pieces
	Cleanup phase 2: simplify CPU core interfaces
	Cleanup phase 3: phase out old interrupt system

***************************************************************************/

#include <signal.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"



/*************************************
 *
 *	Include headers from all CPUs
 *
 *************************************/

#if (HAS_Z80)
#include "cpu/z80/z80.h"
#endif
#if (HAS_8080 || HAS_8085A)
#include "cpu/i8085/i8085.h"
#endif
#if (HAS_M6502 || HAS_M65C02 || HAS_M65SC02 || HAS_M6510 || HAS_M6510T || HAS_M7501 || HAS_M8502 || HAS_N2A03)
#include "cpu/m6502/m6502.h"
#endif
#if (HAS_M4510)
#include "cpu/m6502/m4510.h"
#endif
#if (HAS_M65CE02)
#include "cpu/m6502/m65ce02.h"
#endif
#if (HAS_M6509)
#include "cpu/m6502/m6509.h"
#endif
#if (HAS_H6280)
#include "cpu/h6280/h6280.h"
#endif
#if (HAS_I86)
#include "cpu/i86/i86intf.h"
#endif
#if (HAS_I88)
#include "cpu/i86/i88intf.h"
#endif
#if (HAS_I186)
#include "cpu/i86/i186intf.h"
#endif
#if (HAS_I188)
#include "cpu/i86/i188intf.h"
#endif
#if (HAS_I286)
#include "cpu/i86/i286intf.h"
#endif
#if (HAS_V20 || HAS_V30 || HAS_V33)
#include "cpu/nec/necintrf.h"
#endif
#if (HAS_I8035 || HAS_I8039 || HAS_I8048 || HAS_N7751)
#include "cpu/i8039/i8039.h"
#endif
#if (HAS_I8X41)
#include "cpu/i8x41/i8x41.h"
#endif
#if (HAS_M6800 || HAS_M6801 || HAS_M6802 || HAS_M6803 || HAS_M6808 || HAS_HD63701)
#include "cpu/m6800/m6800.h"
#endif
#if (HAS_M6805 || HAS_M68705 || HAS_HD63705)
#include "cpu/m6805/m6805.h"
#endif
#if (HAS_M6809)
#include "cpu/m6809/m6809.h"
#endif
#if (HAS_HD6309)
#include "cpu/hd6309/hd6309.h"
#endif
#if (HAS_KONAMI)
#include "cpu/konami/konami.h"
#endif
#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
#include "cpu/m68000/m68000.h"
#endif
#if (HAS_T11)
#include "cpu/t11/t11.h"
#endif
#if (HAS_S2650)
#include "cpu/s2650/s2650.h"
#endif
#if (HAS_TMS34010 || HAS_TMS34020)
#include "cpu/tms34010/tms34010.h"
#endif
#if (HAS_TMS9900 || HAS_TMS9940 || HAS_TMS9980 || HAS_TMS9985 || HAS_TMS9989 || HAS_TMS9995 || HAS_TMS99105A || HAS_TMS99110A)
#include "cpu/tms9900/tms9900.h"
#endif
#if (HAS_Z8000)
#include "cpu/z8000/z8000.h"
#endif
#if (HAS_TMS320C10)
#include "cpu/tms32010/tms32010.h"
#endif
#if (HAS_CCPU)
#include "cpu/ccpu/ccpu.h"
#endif
#if (HAS_ADSP2100 || HAS_ADSP2105)
#include "cpu/adsp2100/adsp2100.h"
#endif
#if (HAS_PSXCPU)
#include "cpu/mips/mips.h"
#endif
#if (HAS_ASAP)
#include "cpu/asap/asap.h"
#endif
#if (HAS_UPD7810)
#include "cpu/upd7810/upd7810.h"
#endif


#ifdef MESS

#if (HAS_APEXC)
#include "mess/cpu/apexc/apexc.h"
#endif
#if (HAS_ARM)
#include "mess/cpu/arm/arm.h"
#endif
#if (HAS_CDP1802)
#include "mess/cpu/cdp1802/cdp1802.h"
#endif
#if (HAS_CP1600)
#include "mess/cpu/cp1600/cp1600.h"
#endif
#if (HAS_F8)
#include "mess/cpu/f8/f8.h"
#endif
#if (HAS_G65816)
#include "mess/cpu/g65816/g65816.h"
#endif
#if (HAS_LH5801)
#include "mess/cpu/lh5801/lh5801.h"
#endif
#if (HAS_PDP1)
#include "mess/cpu/pdp1/pdp1.h"
#endif
#if (HAS_SATURN)
#include "mess/cpu/saturn/saturn.h"
#endif
#if (HAS_SC61860)
#include "mess/cpu/sc61860/sc61860.h"
#endif
#if (HAS_SH2)
#include "mess/cpu/sh2/sh2.h"
#endif
#if (HAS_SPC700)
#include "mess/cpu/spc700/spc700.h"
#endif
#if (HAS_Z80GB)
#include "mess/cpu/z80gb/z80gb.h"
#endif
#if (HAS_Z80_MSX)
#include "cpu/z80/z80_msx.h"
#endif

#endif



/*************************************
 *
 *	Debug logging
 *
 *************************************/

#define VERBOSE 0

#define SAVE_STATE_TEST 0
#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *	Macros to help verify active CPU
 *
 *************************************/

#define VERIFY_ACTIVECPU(retval, name)						\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return retval;										\
	}

#define VERIFY_ACTIVECPU_VOID(name)							\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return;												\
	}



/*************************************
 *
 *	Macros to help verify CPU index
 *
 *************************************/

#define VERIFY_CPUNUM(retval, name)							\
	if (cpunum < 0 || cpunum >= totalcpu)					\
	{														\
		logerror(#name "() called for invalid cpu num!\n");	\
		return retval;										\
	}

#define VERIFY_CPUNUM_VOID(name)							\
	if (cpunum < 0 || cpunum >= totalcpu)					\
	{														\
		logerror(#name "() called for invalid cpu num!\n");	\
		return;												\
	}



/*************************************
 *
 *	Internal CPU info type
 *
 *************************************/

struct cpuinfo
{
	struct cpu_interface intf; 		/* copy of the interface data */
	int cputype; 					/* type index of this CPU */
	int family; 					/* family index of this CPU */
	void *context;					/* dynamically allocated context buffer */
};



/*************************************
 *
 *	Prototypes for dummy CPU
 *
 *************************************/

static void dummy_init(void);
static void dummy_reset(void *param);
static void dummy_exit(void);
static int dummy_execute(int cycles);
static void dummy_burn(int cycles);
static unsigned dummy_get_context(void *regs);
static void dummy_set_context(void *regs);
static unsigned dummy_get_pc(void);
static void dummy_set_pc(unsigned val);
static unsigned dummy_get_sp(void);
static void dummy_set_sp(unsigned val);
static unsigned dummy_get_reg(int regnum);
static void dummy_set_reg(int regnum, unsigned val);
static void dummy_set_nmi_line(int state);
static void dummy_set_irq_line(int irqline, int state);
static void dummy_set_irq_callback(int (*callback)(int irqline));
static int dummy_ICount;
static const char *dummy_info(void *context, int regnum);
static unsigned dummy_dasm(char *buffer, unsigned pc);



/*************************************
 *
 *	Macros to generate CPU entries
 *
 *************************************/

/* most CPUs use this macro */
#define CPU0(cpu,name,nirq,dirq,oc,i1,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_ICount, oc, i1, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* CPUs which have the _burn function */
#define CPU1(cpu,name,nirq,dirq,oc,i1,datawidth,mem,shift,bits,endian,align,maxinst)	 \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, 				   \
		name##_burn,															   \
		name##_get_context, name##_set_context, 								   \
		name##_get_cycle_table, name##_set_cycle_table, 						   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_ICount, oc, i1, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* CPUs which have the _internal_interrupt function */
#define CPU2(cpu,name,nirq,dirq,oc,i1,datawidth,mem,shift,bits,endian,align,maxinst)	 \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, 				   \
		NULL,																	   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_info, name##_dasm,					   \
		nirq, dirq, &name##_ICount, oc, i1, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}																			   \

/* like CPU0, but CPU has Harvard-architecture like program/data memory */
#define CPU3(cpu,name,nirq,dirq,oc,i1,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_icount, oc, i1, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		cpu##_PGM_OFFSET, cpu_setopbase##mem,									   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* like CPU0, but CPU has internal memory (or I/O ports, timers or similiar) */
#define CPU4(cpu,name,nirq,dirq,oc,i1,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_icount, oc, i1, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, name##_internal_r, name##_internal_w, \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}



/*************************************
 *
 *	The core list of CPU interfaces
 *
 *************************************/

const struct cpu_interface cpuintrf[] =
{
	CPU0(DUMMY,    dummy,	 1,  0,1.00,-1,			    8, 16,	  0,16,LE,1, 1	),
#if (HAS_Z80)
	CPU1(Z80,	   z80, 	 1,255,1.00,Z80_IRQ_INT,    8, 16,	  0,16,LE,1, 4	),
#endif
#if (HAS_8080)
	CPU0(8080,	   i8080,	 4,255,1.00,I8080_INTR,	    8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_8085A)
	CPU0(8085A,    i8085,	 4,255,1.00,I8085_INTR,	    8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6502)
	CPU0(M6502,    m6502,	 1,  0,1.00,M6502_INT_IRQ,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65C02)
	CPU0(M65C02,   m65c02,	 1,  0,1.00,M65C02_INT_IRQ, 8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65SC02)
	CPU0(M65SC02,  m65sc02,  1,  0,1.00,M65SC02_INT_IRQ,8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65CE02)
	CPU0(M65CE02,  m65ce02,  1,  0,1.00,M65CE02_INT_IRQ,8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6509)
	CPU0(M6509,    m6509,	 1,  0,1.00,M6509_INT_IRQ,  8, 20,	  0,20,LE,1, 3	),
#endif
#if (HAS_M6510)
	CPU0(M6510,    m6510,	 1,  0,1.00,M6510_INT_IRQ,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6510T)
	CPU0(M6510T,   m6510t,	 1,  0,1.00,M6510T_INT_IRQ, 8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M7501)
	CPU0(M7501,    m7501,	 1,  0,1.00,M7501_INT_IRQ,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M8502)
	CPU0(M8502,    m8502,	 1,  0,1.00,M8502_INT_IRQ,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_N2A03)
	CPU0(N2A03,    n2a03,	 1,  0,1.00,N2A03_INT_IRQ,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M4510)
	CPU0(M4510,    m4510,	 1,  0,1.00,M4510_INT_IRQ,  8, 20,	  0,20,LE,1, 3	),
#endif
#if (HAS_H6280)
	CPU0(H6280,    h6280,	 3,  0,1.00,-1,			    8, 21,	  0,21,LE,1, 3	),
#endif
#if (HAS_I86)
	CPU0(I86,	   i86, 	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I88)
	CPU0(I88,	   i88, 	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I186)
	CPU0(I186,	   i186,	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I188)
	CPU0(I188,	   i188,	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I286)
	CPU0(I286,	   i286,	 1,  0,1.00,-1000,		    8, 24,	  0,24,LE,1, 5	),
#endif
#if (HAS_V20)
	CPU0(V20,	   v20, 	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_V30)
	CPU0(V30,	   v30, 	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_V33)
	CPU0(V33,	   v33, 	 1,  0,1.00,-1000,		    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I8035)
	CPU0(I8035,    i8035,	 1,  0,1.00,I8035_EXT_INT,  8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8039)
	CPU0(I8039,    i8039,	 1,  0,1.00,I8039_EXT_INT,  8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8048)
	CPU0(I8048,    i8048,	 1,  0,1.00,I8048_EXT_INT,  8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_N7751)
	CPU0(N7751,    n7751,	 1,  0,1.00,N7751_EXT_INT,  8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8X41)
	CPU0(I8X41,    i8x41,	 1,  0,1.00,I8X41_INT_IBF,  8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_M6800)
	CPU0(M6800,    m6800,	 1,  0,1.00,M6800_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6801)
	CPU0(M6801,    m6801,	 1,  0,1.00,M6801_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6802)
	CPU0(M6802,    m6802,	 1,  0,1.00,M6802_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6803)
	CPU0(M6803,    m6803,	 1,  0,1.00,M6803_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6808)
	CPU0(M6808,    m6808,	 1,  0,1.00,M6808_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_HD63701)
	CPU0(HD63701,  hd63701,  1,  0,1.00,HD63701_INT_IRQ,8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_NSC8105)
	CPU0(NSC8105,  nsc8105,  1,  0,1.00,NSC8105_INT_IRQ,8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6805)
	CPU0(M6805,    m6805,	 1,  0,1.00,M6805_INT_IRQ,  8, 16,	  0,11,BE,1, 3	),
#endif
#if (HAS_M68705)
	CPU0(M68705,   m68705,	 1,  0,1.00,M68705_INT_IRQ, 8, 16,	  0,11,BE,1, 3	),
#endif
#if (HAS_HD63705)
	CPU0(HD63705,  hd63705,  8,  0,1.00,HD63705_INT_IRQ,8, 16,	  0,16,BE,1, 3	),
#endif
#if (HAS_HD6309)
	CPU0(HD6309,   hd6309,	 2,  0,1.00,HD6309_INT_IRQ, 8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6809)
	CPU0(M6809,    m6809,	 2,  0,1.00,M6809_INT_IRQ,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_KONAMI)
	CPU0(KONAMI,   konami,	 2,  0,1.00,KONAMI_INT_IRQ, 8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M68000)
	CPU0(M68000,   m68000,	 8, -1,1.00,-1,			   16,24bew,  0,24,BE,2,10	),
#endif
#if (HAS_M68010)
	CPU0(M68010,   m68010,	 8, -1,1.00,-1,			   16,24bew,  0,24,BE,2,10	),
#endif
#if (HAS_M68EC020)
	CPU0(M68EC020, m68ec020, 8, -1,1.00,-1,			   32,24bedw, 0,24,BE,4,10	),
#endif
#if (HAS_M68020)
	CPU0(M68020,   m68020,	 8, -1,1.00,-1, 		   32,32bedw, 0,32,BE,4,10	),
#endif
#if (HAS_T11)
	CPU0(T11,	   t11, 	 4,  0,1.00,-1,			   16,16lew,  0,16,LE,2, 6	),
#endif
#if (HAS_S2650)
	CPU0(S2650,    s2650,	 2,  0,1.00,-1,			    8, 16,	  0,15,LE,1, 3	),
#endif
#if (HAS_TMS34010)
	CPU2(TMS34010, tms34010, 2,  0,1.00,TMS34010_INT1, 16,29lew,  3,29,LE,2,10	),
#endif
#if (HAS_TMS34020)
	CPU2(TMS34020, tms34020, 2,  0,1.00,TMS34020_INT1, 16,29lew,  3,29,LE,2,10	),
#endif
#if (HAS_TMS9900)
	CPU0(TMS9900,  tms9900,  1,  0,1.00,-1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS9940)
	CPU0(TMS9940,  tms9940,  1,  0,1.00,-1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS9980)
	CPU0(TMS9980,  tms9980a, 1,  0,1.00,-1,			    8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9985)
	CPU0(TMS9985,  tms9985,  1,  0,1.00,-1,			    8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9989)
	CPU0(TMS9989,  tms9989,  1,  0,1.00,-1,			    8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9995)
	CPU0(TMS9995,  tms9995,  1,  0,1.00,-1,			    8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS99105A)
	CPU0(TMS99105A,tms99105a,1,  0,1.00,-1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS99110A)
	CPU0(TMS99110A,tms99110a,1,  0,1.00,-1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_Z8000)
	CPU0(Z8000,    z8000,	 2,  0,1.00,Z8000_NVI,	   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS320C10)
	CPU3(TMS320C10,tms320c10,2,  0,1.00,-1,			   16,16bew, -1,16,BE,2, 4	),
#endif
#if (HAS_CCPU)
	CPU3(CCPU,	   ccpu,	 2,  0,1.00,-1,			   16,16bew,  0,15,BE,2, 3	),
#endif
#if (HAS_ADSP2100)
	CPU3(ADSP2100, adsp2100, 4,  0,1.00,-1,			   16,17lew, -1,14,LE,2, 4	),
#endif
#if (HAS_ADSP2105)
	CPU3(ADSP2105, adsp2105, 4,  0,1.00,-1,			   16,17lew, -1,14,LE,2, 4	),
#endif
#if (HAS_PSXCPU)
	CPU0(PSXCPU,   mips,	 8, -1,1.00,MIPS_INT_NONE, 16,32lew,  0,32,LE,4, 4	),
#endif
#if (HAS_ASAP)
	#define asap_ICount asap_icount
	CPU0(ASAP,	   asap,	 1,  0,1.00,-1,			   32,32ledw, 0,32,LE,4, 12 ),
#endif
#if (HAS_UPD7810)
#define upd7810_ICount upd7810_icount
	CPU0(UPD7810,  upd7810,  2,  0,1.00,UPD7810_INTF1,  8, 16,	  0,16,LE,1, 4	),
#endif

#ifdef MESS
#if (HAS_APEXC)
	CPU0(APEXC,    apexc,	 0,  0,1.00,-1,			   32,18bedw, 0,18,LE,1, 1	),
#endif
#if (HAS_ARM)
	CPU0(ARM,	   arm, 	 2,  0,1.00,ARM_FIRQ,	   32,26ledw, 0,26,LE,4, 4	),
#endif
#if (HAS_CDP1802)
#define cdp1802_ICount cdp1802_icount
	CPU0(CDP1802,  cdp1802,  1,  0,1.00,CDP1802_IRQ,    8, 16,	  0,16,BE,1, 3	),
#endif
#if (HAS_CP1600)
#define cp1600_ICount cp1600_icount
	CPU0(CP1600,   cp1600,	 0,  0,1.00,-1,			    8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_F8)
#define f8_ICount f8_icount
	CPU4(F8,	   f8,		 1,  0,1.00,F8_INT_INTR,    8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_G65816)
	CPU0(G65816,  g65816,	 1,  0,1.00,G65816_INT_IRQ, 8, 24,	  0,24,BE,1, 3	),
#endif
#if (HAS_LH5801)
#define lh5801_ICount lh5801_icount
	CPU0(LH5801,   lh5801,	 1,  0,1.00,LH5801_IRQ,	    8, 17,	  0,17,BE,1, 5	),
#endif
#if (HAS_PDP1)
	CPU0(PDP1,	   pdp1,	 0,  0,1.00,-1,			    8, 16,	  0,18,LE,1, 3	),
#endif
#if (HAS_SATURN)
#define saturn_ICount saturn_icount
	CPU0(SATURN,   saturn,	 1,  0,1.00,SATURN_INT_IRQ, 8,20,	  0,20,LE,1, 21 ),
#endif
#if (HAS_SC61860)
	#define sc61860_ICount sc61860_icount
	CPU0(SC61860,  sc61860,  1,  0,1.00,-1,			    8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_SH2)
	CPU4(SH2,	   sh2, 	16,  0,1.00, 0, 		   32,32bedw,   0,32,BE,2, 2  ),
#endif
#if (HAS_SPC700)
	CPU0(SPC700,   spc700,	 0,  0,1.00,-1,			    8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_Z80GB)
	CPU0(Z80GB,    z80gb,	 5,255,1.00,0,			    8, 16,	  0,16,LE,1, 4	),
#endif
#if (HAS_Z80_MSX)
	CPU1(Z80_MSX,  z80_msx,	 1,255,1.00,Z80_IRQ_INT,    8, 16,	  0,16,LE,1, 4	),
#endif
#endif
};



/*************************************
 *
 *	Default debugger window layout
 *
 *************************************/

UINT8 default_win_layout[] =
{
	 0, 0,80, 5,	/* register window (top rows) */
	 0, 5,24,17,	/* disassembler window (left, middle columns) */
	25, 5,55, 8,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1 	/* command line window (bottom row) */
};



/*************************************
 *
 *	Other variables we own
 *
 *************************************/

int activecpu;		/* index of active CPU (or -1) */
int totalcpu;		/* total number of CPUs */

static struct cpuinfo cpu[MAX_CPU];

static int cpu_active_context[CPU_COUNT];
static int cpu_context_stack[4];
static int cpu_context_stack_ptr;



/*************************************
 *
 *	Set a new CPU context
 *
 *************************************/

INLINE void set_cpu_context(int cpunum)
{
	int newfamily = cpu[cpunum].family;
	int oldcontext = cpu_active_context[newfamily];

	/* if we need to change contexts, save the one that was there */
	if (oldcontext != cpunum && oldcontext != -1)
		(*cpu[oldcontext].intf.get_context)(cpu[oldcontext].context);

	/* swap memory spaces */
	activecpu = cpunum;
	memory_set_context(cpunum);

	/* if the new CPU's context is not swapped in, do it now */
	if (oldcontext != cpunum)
	{
		(*cpu[cpunum].intf.set_context)(cpu[cpunum].context);
		cpu_active_context[newfamily] = cpunum;
	}
}



/*************************************
 *
 *	Push/pop to a new CPU context
 *
 *************************************/

void cpuintrf_push_context(int cpunum)
{
	/* push the old context onto the stack */
	cpu_context_stack[cpu_context_stack_ptr++] = activecpu;

	/* do the rest only if this isn't the activecpu */
	if (cpunum != activecpu && cpunum != -1)
		set_cpu_context(cpunum);

	/* this is now the active CPU */
	activecpu = cpunum;
}


void cpuintrf_pop_context(void)
{
	/* push the old context onto the stack */
	int cpunum = cpu_context_stack[--cpu_context_stack_ptr];

	/* do the rest only if this isn't the activecpu */
	if (cpunum != activecpu && cpunum != -1)
		set_cpu_context(cpunum);

	/* this is now the active CPU */
	activecpu = cpunum;
}



/*************************************
 *
 *	Initialize a single CPU
 *
 *************************************/

int cpuintrf_init(void)
{
	int cputype;

	/* verify the order of entries in the cpuintrf[] array */
	for (cputype = 0; cputype < CPU_COUNT; cputype++)
	{
		/* make sure the index in the array matches the current index */
		if (cpuintrf[cputype].cpu_num != cputype)
		{
			printf("CPU #%d [%s] wrong ID %d: check enum CPU_... in src/cpuintrf.h!\n", cputype, cputype_name(cputype), cpuintrf[cputype].cpu_num);
			exit(1);
		}

		/* also reset the active CPU context info */
		cpu_active_context[cputype] = -1;
	}

	/* zap the CPU data structure */
	memset(cpu, 0, sizeof(cpu));
	totalcpu = 0;

	/* reset the context stack */
	memset(cpu_context_stack, -1, sizeof(cpu_context_stack));
	cpu_context_stack_ptr = 0;

	return 0;
}



/*************************************
 *
 *	Initialize a single CPU
 *
 *************************************/

int cpuintrf_init_cpu(int cpunum, int cputype)
{
	char familyname[256];
	int j, size;

	/* fill in the type and interface */
	cpu[cpunum].intf = cpuintrf[cputype];
	cpu[cpunum].cputype = cputype;

	/* determine the family index */
	strcpy(familyname, cputype_core_file(cputype));
	for (j = 0; j < CPU_COUNT; j++)
		if (!strcmp(familyname, cputype_core_file(j)))
		{
			cpu[cpunum].family = j;
			break;
		}

	/* determine the context size */
	size = (*cpu[cpunum].intf.get_context)(NULL);
	if (size == 0)
	{
		/* that can't really be true */
		logerror("CPU #%d claims to need no context buffer!\n", cpunum);
		return 1;
	}

	/* allocate a context buffer for the CPU */
	cpu[cpunum].context = malloc(size);
	if (cpu[cpunum].context == NULL)
	{
		/* that's really bad :( */
		logerror("CPU #%d failed to allocate context buffer (%d bytes)!\n", cpunum, size);
		return 1;
	}

	/* zap the context buffer */
	memset(cpu[cpunum].context, 0, size);

	/* initialize the CPU and stash the context */
	activecpu = cpunum;
	(*cpu[cpunum].intf.init)();
	(*cpu[cpunum].intf.get_context)(cpu[cpunum].context);
	activecpu = -1;

	/* clear out the registered CPU for this family */
	cpu_active_context[cpu[cpunum].family] = -1;

	/* make sure the total includes us */
	totalcpu = cpunum + 1;

	return 0;
}



/*************************************
 *
 *	Exit/free a single CPU
 *
 *************************************/

void cpuintrf_exit_cpu(int cpunum)
{
	/* if the CPU core defines an exit function, call it now */
	if (cpu[cpunum].intf.exit)
		(*cpu[cpunum].intf.exit)();

	/* free the context buffer for that CPU */
	if (cpu[cpunum].context)
		free(cpu[cpunum].context);
	cpu[cpunum].context = NULL;
}



/*************************************
 *
 *	Convert old-style interrupt
 *	number to an IRQ line
 *
 *************************************/

int convert_type_to_irq_line(int cpunum, int num)
{
	int irq_line;

	switch (cpu[cpunum].cputype)
	{
#if (HAS_Z80)
	case CPU_Z80:				irq_line = 0; LOG(("Z80 IRQ\n")); break;
#endif
#if (HAS_8080)
	case CPU_8080:
		switch (num)
		{
		case I8080_INTR:		irq_line = 0; LOG(("I8080 INTR\n")); break;
		default:				irq_line = 0; LOG(("I8080 unknown\n"));
		}
		break;
#endif
#if (HAS_8085A)
	case CPU_8085A:
		switch (num)
		{
		case I8085_INTR:		irq_line = 0; LOG(("I8085 INTR\n")); break;
		case I8085_RST55:		irq_line = 1; LOG(("I8085 RST55\n")); break;
		case I8085_RST65:		irq_line = 2; LOG(("I8085 RST65\n")); break;
		case I8085_RST75:		irq_line = 3; LOG(("I8085 RST75\n")); break;
		default:				irq_line = 0; LOG(("I8085 unknown\n"));
		}
		break;
#endif
#if (HAS_M6502)
	case CPU_M6502: 			irq_line = 0; LOG(("M6502 IRQ\n")); break;
#endif
#if (HAS_M65C02)
	case CPU_M65C02:			irq_line = 0; LOG(("M65C02 IRQ\n")); break;
#endif
#if (HAS_M65SC02)
	case CPU_M65SC02:			irq_line = 0; LOG(("M65SC02 IRQ\n")); break;
#endif
#if (HAS_M65CE02)
	case CPU_M65CE02:			irq_line = 0; LOG(("M65CE02 IRQ\n")); break;
#endif
#if (HAS_M6509)
	case CPU_M6509: 			irq_line = 0; LOG(("M6509 IRQ\n")); break;
#endif
#if (HAS_M6510)
	case CPU_M6510: 			irq_line = 0; LOG(("M6510 IRQ\n")); break;
#endif
#if (HAS_M6510T)
	case CPU_M6510T:			irq_line = 0; LOG(("M6510T IRQ\n")); break;
#endif
#if (HAS_M7501)
	case CPU_M7501: 			irq_line = 0; LOG(("M7501 IRQ\n")); break;
#endif
#if (HAS_M8502)
	case CPU_M8502: 			irq_line = 0; LOG(("M8502 IRQ\n")); break;
#endif
#if (HAS_N2A03)
	case CPU_N2A03: 			irq_line = 0; LOG(("N2A03 IRQ\n")); break;
#endif
#if (HAS_M4510)
	case CPU_M4510: 			irq_line = 0; LOG(("M4510 IRQ\n")); break;
#endif
#if (HAS_H6280)
	case CPU_H6280:
		switch (num)
		{
		case H6280_INT_IRQ1:	irq_line = 0; LOG(("H6280 INT 1\n")); break;
		case H6280_INT_IRQ2:	irq_line = 1; LOG(("H6280 INT 2\n")); break;
		default:				irq_line = 0; LOG(("H6280 unknown\n"));
		}
		break;
#endif
#if (HAS_I86)
	case CPU_I86:				irq_line = 0; LOG(("I86 IRQ\n")); break;
#endif
#if (HAS_I88)
	case CPU_I88:				irq_line = 0; LOG(("I88 IRQ\n")); break;
#endif
#if (HAS_I186)
	case CPU_I186:				irq_line = 0; LOG(("I186 IRQ\n")); break;
#endif
#if (HAS_I188)
	case CPU_I188:				irq_line = 0; LOG(("I188 IRQ\n")); break;
#endif
#if (HAS_I286)
	case CPU_I286:				irq_line = 0; LOG(("I286 IRQ\n")); break;
#endif
#if (HAS_V20)
	case CPU_V20:				irq_line = 0; LOG(("V20 IRQ\n")); break;
#endif
#if (HAS_V30)
	case CPU_V30:				irq_line = 0; LOG(("V30 IRQ\n")); break;
#endif
#if (HAS_V33)
	case CPU_V33:				irq_line = 0; LOG(("V33 IRQ\n")); break;
#endif
#if (HAS_I8035)
	case CPU_I8035: 			irq_line = 0; LOG(("I8035 IRQ\n")); break;
#endif
#if (HAS_I8039)
	case CPU_I8039: 			irq_line = 0; LOG(("I8039 IRQ\n")); break;
#endif
#if (HAS_I8048)
	case CPU_I8048: 			irq_line = 0; LOG(("I8048 IRQ\n")); break;
#endif
#if (HAS_N7751)
	case CPU_N7751: 			irq_line = 0; LOG(("N7751 IRQ\n")); break;
#endif
#if (HAS_M6800)
	case CPU_M6800: 			irq_line = 0; LOG(("M6800 IRQ\n")); break;
#endif
#if (HAS_M6801)
	case CPU_M6801: 			irq_line = 0; LOG(("M6801 IRQ\n")); break;
#endif
#if (HAS_M6802)
	case CPU_M6802: 			irq_line = 0; LOG(("M6802 IRQ\n")); break;
#endif
#if (HAS_M6803)
	case CPU_M6803: 			irq_line = 0; LOG(("M6803 IRQ\n")); break;
#endif
#if (HAS_M6808)
	case CPU_M6808: 			irq_line = 0; LOG(("M6808 IRQ\n")); break;
#endif
#if (HAS_HD63701)
	case CPU_HD63701:			irq_line = 0; LOG(("HD63701 IRQ\n")); break;
#endif
#if (HAS_M6805)
	case CPU_M6805: 			irq_line = 0; LOG(("M6805 IRQ\n")); break;
#endif
#if (HAS_M68705)
	case CPU_M68705:			irq_line = 0; LOG(("M68705 IRQ\n")); break;
#endif
#if (HAS_HD63705)
	case CPU_HD63705:			irq_line = 0; LOG(("HD68705 IRQ\n")); break;
#endif
#if (HAS_HD6309)
	case CPU_HD6309:
		switch (num)
		{
		case HD6309_INT_IRQ:	irq_line = 0; LOG(("M6309 IRQ\n")); break;
		case HD6309_INT_FIRQ:	irq_line = 1; LOG(("M6309 FIRQ\n")); break;
		default:				irq_line = 0; LOG(("M6309 unknown\n"));
		}
		break;
#endif
#if (HAS_M6809)
	case CPU_M6809:
		switch (num)
		{
		case M6809_INT_IRQ: 	irq_line = 0; LOG(("M6809 IRQ\n")); break;
		case M6809_INT_FIRQ:	irq_line = 1; LOG(("M6809 FIRQ\n")); break;
		default:				irq_line = 0; LOG(("M6809 unknown\n"));
		}
		break;
#endif
#if (HAS_KONAMI)
		case CPU_KONAMI:
		switch (num)
		{
		case KONAMI_INT_IRQ:	irq_line = 0; LOG(("KONAMI IRQ\n")); break;
		case KONAMI_INT_FIRQ:	irq_line = 1; LOG(("KONAMI FIRQ\n")); break;
		default:				irq_line = 0; LOG(("KONAMI unknown\n"));
		}
		break;
#endif
#if (HAS_M68000)
	case CPU_M68000:
		switch (num)
		{
		case MC68000_IRQ_1: 	irq_line = 1; LOG(("M68K IRQ1\n")); break;
		case MC68000_IRQ_2: 	irq_line = 2; LOG(("M68K IRQ2\n")); break;
		case MC68000_IRQ_3: 	irq_line = 3; LOG(("M68K IRQ3\n")); break;
		case MC68000_IRQ_4: 	irq_line = 4; LOG(("M68K IRQ4\n")); break;
		case MC68000_IRQ_5: 	irq_line = 5; LOG(("M68K IRQ5\n")); break;
		case MC68000_IRQ_6: 	irq_line = 6; LOG(("M68K IRQ6\n")); break;
		case MC68000_IRQ_7: 	irq_line = 7; LOG(("M68K IRQ7\n")); break;
		default:				irq_line = 0; LOG(("M68K unknown\n"));
		}
		/* until now only auto vector interrupts supported */
		num = MC68000_INT_ACK_AUTOVECTOR;
		break;
#endif
#if (HAS_M68010)
	case CPU_M68010:
		switch (num)
		{
		case MC68010_IRQ_1: 	irq_line = 1; LOG(("M68010 IRQ1\n")); break;
		case MC68010_IRQ_2: 	irq_line = 2; LOG(("M68010 IRQ2\n")); break;
		case MC68010_IRQ_3: 	irq_line = 3; LOG(("M68010 IRQ3\n")); break;
		case MC68010_IRQ_4: 	irq_line = 4; LOG(("M68010 IRQ4\n")); break;
		case MC68010_IRQ_5: 	irq_line = 5; LOG(("M68010 IRQ5\n")); break;
		case MC68010_IRQ_6: 	irq_line = 6; LOG(("M68010 IRQ6\n")); break;
		case MC68010_IRQ_7: 	irq_line = 7; LOG(("M68010 IRQ7\n")); break;
		default:				irq_line = 0; LOG(("M68010 unknown\n"));
		}
		/* until now only auto vector interrupts supported */
		num = MC68000_INT_ACK_AUTOVECTOR;
		break;
#endif
#if (HAS_M68020)
	case CPU_M68020:
		switch (num)
		{
		case MC68020_IRQ_1: 	irq_line = 1; LOG(("M68020 IRQ1\n")); break;
		case MC68020_IRQ_2: 	irq_line = 2; LOG(("M68020 IRQ2\n")); break;
		case MC68020_IRQ_3: 	irq_line = 3; LOG(("M68020 IRQ3\n")); break;
		case MC68020_IRQ_4: 	irq_line = 4; LOG(("M68020 IRQ4\n")); break;
		case MC68020_IRQ_5: 	irq_line = 5; LOG(("M68020 IRQ5\n")); break;
		case MC68020_IRQ_6: 	irq_line = 6; LOG(("M68020 IRQ6\n")); break;
		case MC68020_IRQ_7: 	irq_line = 7; LOG(("M68020 IRQ7\n")); break;
		default:				irq_line = 0; LOG(("M68020 unknown\n"));
		}
		/* until now only auto vector interrupts supported */
		num = MC68000_INT_ACK_AUTOVECTOR;
		break;
#endif
#if (HAS_M68EC020)
	case CPU_M68EC020:
		switch (num)
		{
		case MC68EC020_IRQ_1:	irq_line = 1; LOG(("M68EC020 IRQ1\n")); break;
		case MC68EC020_IRQ_2:	irq_line = 2; LOG(("M68EC020 IRQ2\n")); break;
		case MC68EC020_IRQ_3:	irq_line = 3; LOG(("M68EC020 IRQ3\n")); break;
		case MC68EC020_IRQ_4:	irq_line = 4; LOG(("M68EC020 IRQ4\n")); break;
		case MC68EC020_IRQ_5:	irq_line = 5; LOG(("M68EC020 IRQ5\n")); break;
		case MC68EC020_IRQ_6:	irq_line = 6; LOG(("M68EC020 IRQ6\n")); break;
		case MC68EC020_IRQ_7:	irq_line = 7; LOG(("M68EC020 IRQ7\n")); break;
		default:				irq_line = 0; LOG(("M68EC020 unknown\n"));
		}
		/* until now only auto vector interrupts supported */
		num = MC68000_INT_ACK_AUTOVECTOR;
		break;
#endif
#if (HAS_T11)
	case CPU_T11:
		switch (num)
		{
		case T11_IRQ0:			irq_line = 0; LOG(("T11 IRQ0\n")); break;
		case T11_IRQ1:			irq_line = 1; LOG(("T11 IRQ1\n")); break;
		case T11_IRQ2:			irq_line = 2; LOG(("T11 IRQ2\n")); break;
		case T11_IRQ3:			irq_line = 3; LOG(("T11 IRQ3\n")); break;
		default:				irq_line = 0; LOG(("T11 unknown\n"));
		}
		break;
#endif
#if (HAS_S2650)
	case CPU_S2650: 			irq_line = 0; LOG(("S2650 IRQ\n")); break;
#endif
#if (HAS_TMS34010)
	case CPU_TMS34010:
		switch (num)
		{
		case TMS34010_INT1: 	irq_line = 0; LOG(("TMS34010 INT1\n")); break;
		case TMS34010_INT2: 	irq_line = 1; LOG(("TMS34010 INT2\n")); break;
		default:				irq_line = 0; LOG(("TMS34010 unknown\n"));
		}
		break;
#endif
/*#if (HAS_TMS9900)
	case CPU_TMS9900:	irq_line = 0; LOG(("TMS9900 IRQ\n")); break;
#endif*/
#if (HAS_TMS9900) || (HAS_TMS9940) || (HAS_TMS9980) || (HAS_TMS9985) \
|| (HAS_TMS9989) || (HAS_TMS9995) || (HAS_TMS99105A) || (HAS_TMS99110A)
#if (HAS_TMS9900)
	case CPU_TMS9900:
#endif
#if (HAS_TMS9940)
	case CPU_TMS9940:
#endif
#if (HAS_TMS9980)
	case CPU_TMS9980:
#endif
#if (HAS_TMS9985)
	case CPU_TMS9985:
#endif
#if (HAS_TMS9989)
	case CPU_TMS9989:
#endif
#if (HAS_TMS9995)
	case CPU_TMS9995:
#endif
#if (HAS_TMS99105A)
	case CPU_TMS99105A:
#endif
#if (HAS_TMS99110A)
	case CPU_TMS99110A:
#endif
		irq_line = 0;
		break;
#endif
#if (HAS_Z8000)
	case CPU_Z8000:
		switch (num)
		{
		case Z8000_NVI: 		irq_line = 0; LOG(("Z8000 NVI\n")); break;
		case Z8000_VI:			irq_line = 1; LOG(("Z8000 VI\n")); break;
		default:				irq_line = 0; LOG(("Z8000 unknown\n"));
		}
		break;
#endif
#if (HAS_TMS320C10)
	case CPU_TMS320C10:
		switch (num)
		{
		case TMS320C10_ACTIVE_INT:	irq_line = 0; LOG(("TMS32010 INT\n")); break;
		case TMS320C10_ACTIVE_BIO:	irq_line = 1; LOG(("TMS32010 BIO\n")); break;
		default:					irq_line = 0; LOG(("TMS32010 unknown\n"));
		}
		break;
#endif
#if (HAS_ADSP2100)
	case CPU_ADSP2100:
		switch (num)
		{
		case ADSP2100_IRQ0: 		irq_line = 0; LOG(("ADSP2100 IRQ0\n")); break;
		case ADSP2100_IRQ1: 		irq_line = 1; LOG(("ADSP2100 IRQ1\n")); break;
		case ADSP2100_IRQ2: 		irq_line = 2; LOG(("ADSP2100 IRQ1\n")); break;
		case ADSP2100_IRQ3: 		irq_line = 3; LOG(("ADSP2100 IRQ1\n")); break;
		default:					irq_line = 0; LOG(("ADSP2100 unknown\n"));
		}
		break;
#endif
#if (HAS_PSXCPU)
	case CPU_PSXCPU:
		switch (num)
		{
		case MIPS_IRQ0: 		irq_line = 0; LOG(("MIPS IRQ0\n")); break;
		case MIPS_IRQ1: 		irq_line = 1; LOG(("MIPS IRQ1\n")); break;
		case MIPS_IRQ2: 		irq_line = 2; LOG(("MIPS IRQ2\n")); break;
		case MIPS_IRQ3: 		irq_line = 3; LOG(("MIPS IRQ3\n")); break;
		case MIPS_IRQ4: 		irq_line = 4; LOG(("MIPS IRQ4\n")); break;
		case MIPS_IRQ5: 		irq_line = 5; LOG(("MIPS IRQ5\n")); break;
		default:				irq_line = 0; LOG(("MIPS unknown\n"));
		}
		break;
#endif
	default:
		irq_line = 0;
		LOG(("Please use the new interrupt scheme for your new developments !\n"));
	}
	cpu_irq_line_vector_w(cpunum, irq_line, num);
	return irq_line;
}



/*************************************
 *
 *	Interfaces to the active CPU
 *
 *************************************/

/*--------------------------
 	Adjust/get icount
--------------------------*/

void activecpu_adjust_icount(int delta)
{
	VERIFY_ACTIVECPU_VOID(activecpu_adjust_icount);
	*cpu[activecpu].intf.icount += delta;
}


int activecpu_get_icount(void)
{
	VERIFY_ACTIVECPU(0, activecpu_get_icount);
	return *cpu[activecpu].intf.icount;
}


/*--------------------------
 	Reset banking pointers
--------------------------*/

void activecpu_reset_banking(void)
{
	VERIFY_ACTIVECPU_VOID(activecpu_reset_banking);
	(*cpu[activecpu].intf.set_op_base)(activecpu_get_pc_byte());
}


/*--------------------------
 	IRQ line setting
--------------------------*/

void activecpu_set_irq_line(int irqline, int state)
{
	VERIFY_ACTIVECPU_VOID(activecpu_set_irq_line);
	if (state != INTERNAL_CLEAR_LINE && state != INTERNAL_ASSERT_LINE)
	{
		logerror("activecpu_set_irq_line called when cpu_set_irq_line should have been used!\n");
		return;
	}
	if (irqline == IRQ_LINE_NMI)
		(*cpu[activecpu].intf.set_nmi_line)(state - INTERNAL_CLEAR_LINE);
	else
		(*cpu[activecpu].intf.set_irq_line)(irqline, state - INTERNAL_CLEAR_LINE);
}


/*--------------------------
 	Get/set cycle table
--------------------------*/

void *activecpu_get_cycle_table(int which)
{
	VERIFY_ACTIVECPU(NULL, activecpu_get_cycle_table);
	return (*cpu[activecpu].intf.get_cycle_table)(which);
}


void activecpu_set_cycle_tbl(int which, void *new_table)
{
	VERIFY_ACTIVECPU_VOID(activecpu_set_cycle_tbl);
	(*cpu[activecpu].intf.set_cycle_table)(which, new_table);
}


/*--------------------------
 	Get/set registers
--------------------------*/

unsigned activecpu_get_reg(int regnum)
{
	VERIFY_ACTIVECPU(0, activecpu_get_reg);
	return (*cpu[activecpu].intf.get_reg)(regnum);
}


void activecpu_set_reg(int regnum, unsigned val)
{
	VERIFY_ACTIVECPU_VOID(activecpu_set_reg);
	(*cpu[activecpu].intf.set_reg)(regnum, val);
}


/*--------------------------
 	Get/set PC
--------------------------*/

offs_t activecpu_get_pc(void)
{
	VERIFY_ACTIVECPU(0, activecpu_get_pc);
	return (*cpu[activecpu].intf.get_pc)();
}


offs_t activecpu_get_pc_byte(void)
{
	offs_t base, pc;
	int shift;

	VERIFY_ACTIVECPU(0, activecpu_get_pc_byte);
	shift = cpu[activecpu].intf.address_shift;
	base = cpu[activecpu].intf.pgm_memory_base;
	pc = (*cpu[activecpu].intf.get_pc)();
	return base + ((shift < 0) ? (pc << -shift) : (pc >> shift));
}


void activecpu_set_pc(offs_t val)
{
	VERIFY_ACTIVECPU_VOID(activecpu_get_pc);
	(*cpu[activecpu].intf.set_pc)(val);
}


void activecpu_set_op_base(unsigned val)
{
	VERIFY_ACTIVECPU_VOID(activecpu_set_op_base);
	(*cpu[activecpu].intf.set_op_base)(val);
}


/*--------------------------
 	Get/set SP
--------------------------*/

offs_t activecpu_get_sp(void)
{
	VERIFY_ACTIVECPU(0, activecpu_get_sp);
	return (*cpu[activecpu].intf.get_sp)();
}


void activecpu_set_sp(offs_t val)
{
	VERIFY_ACTIVECPU_VOID(activecpu_set_sp);
	(*cpu[activecpu].intf.set_sp)(val);
}


/*--------------------------
 	Disassembly
--------------------------*/

unsigned activecpu_dasm(char *buffer, unsigned pc)
{
	VERIFY_ACTIVECPU(1, activecpu_dasm);
	return (*cpu[activecpu].intf.cpu_dasm)(buffer, pc);
}


/*--------------------------
 	Register dumps
--------------------------*/

const char *activecpu_flags(void)
{
	VERIFY_ACTIVECPU("", activecpu_flags);
	return (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_FLAGS);
}


const char *activecpu_dump_reg(int regnum)
{
	VERIFY_ACTIVECPU("", activecpu_dump_reg);
	return (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_REG + regnum);
}


/*--------------------------
 	State dumps
--------------------------*/

const char *activecpu_dump_state(void)
{
	static char buffer[1024+1];
	unsigned addr_width = (activecpu_address_bits() + 3) / 4;
	char *dst = buffer;
	const char *src;
	const INT8 *regs;
	int width;

	VERIFY_ACTIVECPU("", activecpu_dump_state);

	dst += sprintf(dst, "CPU #%d [%s]\n", activecpu, activecpu_name());
	width = 0;
	regs = (INT8 *)activecpu_reg_layout();
	while (*regs)
	{
		if (*regs == -1)
		{
			dst += sprintf(dst, "\n");
			width = 0;
		}
		else
		{
			src = activecpu_dump_reg(*regs);
			if (*src)
			{
				if (width + strlen(src) + 1 >= 80)
				{
					dst += sprintf(dst, "\n");
					width = 0;
				}
				dst += sprintf(dst, "%s ", src);
				width += strlen(src) + 1;
			}
		}
		regs++;
	}
	dst += sprintf(dst, "\n%0*X: ", addr_width, activecpu_get_pc());
	activecpu_dasm(dst, activecpu_get_pc());
	strcat(dst, "\n\n");

	return buffer;
}


/*--------------------------
 	Get/set static info
--------------------------*/

#define CPU_FUNC(rettype, name, defresult, result)			\
rettype name(void)	 										\
{ 															\
	VERIFY_ACTIVECPU(defresult, name)						\
	return result;											\
}

CPU_FUNC(int,          activecpu_default_irq_line,   0,  cpu[activecpu].intf.irq_int)
CPU_FUNC(int,          activecpu_default_irq_vector, 0,  cpu[activecpu].intf.default_vector)
CPU_FUNC(unsigned,     activecpu_address_bits,       0,  cpu[activecpu].intf.address_bits)
CPU_FUNC(unsigned,     activecpu_address_mask,       0,  0xffffffffUL >> (32 - cpu[activecpu].intf.address_bits))
CPU_FUNC(int,          activecpu_address_shift,      0,  cpu[activecpu].intf.address_shift)
CPU_FUNC(unsigned,     activecpu_endianess,          0,  cpu[activecpu].intf.endianess)
CPU_FUNC(unsigned,     activecpu_databus_width,      0,  cpu[activecpu].intf.databus_width)
CPU_FUNC(unsigned,     activecpu_align_unit,         0,  cpu[activecpu].intf.align_unit)
CPU_FUNC(unsigned,     activecpu_max_inst_len,       0,  cpu[activecpu].intf.max_inst_len)
CPU_FUNC(const char *, activecpu_name,               "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_NAME))
CPU_FUNC(const char *, activecpu_core_family,        "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_FAMILY))
CPU_FUNC(const char *, activecpu_core_version,       "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_VERSION))
CPU_FUNC(const char *, activecpu_core_file,          "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_FILE))
CPU_FUNC(const char *, activecpu_core_credits,       "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_CREDITS))
CPU_FUNC(const char *, activecpu_reg_layout,         "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_REG_LAYOUT))
CPU_FUNC(const char *, activecpu_win_layout,         "", (*cpu[activecpu].intf.cpu_info)(NULL, CPU_INFO_WIN_LAYOUT))



/*************************************
 *
 *	Interfaces to a specific CPU
 *
 *************************************/

/*--------------------------
 	Execute
--------------------------*/

int cpunum_execute(int cpunum, int cycles)
{
	int ran;
	VERIFY_CPUNUM(0, cpunum_execute);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_op_base)(activecpu_get_pc_byte());
	ran = (*cpu[cpunum].intf.execute)(cycles);
	cpuintrf_pop_context();
	return ran;
}


/*--------------------------
 	Reset and set IRQ ack
--------------------------*/

void cpunum_reset(int cpunum, void *param, int (*irqack)(int))
{
	VERIFY_CPUNUM_VOID(cpunum_reset);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.reset)(param);
	if (irqack)
		(*cpu[cpunum].intf.set_irq_callback)(irqack);
	cpuintrf_pop_context();
}


/*--------------------------
 	Read a byte
--------------------------*/

data8_t cpunum_read_byte(int cpunum, offs_t address)
{
	int result;
	VERIFY_CPUNUM(0, cpunum_read_byte);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.memory_read)(address);
	cpuintrf_pop_context();
	return result;
}


/*--------------------------
 	Write a byte
--------------------------*/

void cpunum_write_byte(int cpunum, offs_t address, data8_t data)
{
	VERIFY_CPUNUM_VOID(cpunum_write_byte);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.memory_write)(address, data);
	cpuintrf_pop_context();
}


/*--------------------------
 	Get context pointer
--------------------------*/

void *cpunum_get_context_ptr(int cpunum)
{
	VERIFY_CPUNUM(NULL, cpunum_get_context_ptr);
	return (cpu_active_context[cpu[cpunum].family] == cpunum) ? NULL : cpu[cpunum].context;
}


/*--------------------------
 	Get/set cycle table
--------------------------*/

void *cpunum_get_cycle_table(int cpunum, int which)
{
	void *result;
	VERIFY_CPUNUM(NULL, cpunum_get_cycle_table);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.get_cycle_table)(which);
	cpuintrf_pop_context();
	return result;
}


void cpunum_set_cycle_tbl(int cpunum, int which, void *new_table)
{
	VERIFY_CPUNUM_VOID(cpunum_set_cycle_tbl);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_cycle_table)(which, new_table);
	cpuintrf_pop_context();
}


/*--------------------------
 	Get/set registers
--------------------------*/

unsigned cpunum_get_reg(int cpunum, int regnum)
{
	unsigned result;
	VERIFY_CPUNUM(0, cpunum_get_reg);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.get_reg)(regnum);
	cpuintrf_pop_context();
	return result;
}


void cpunum_set_reg(int cpunum, int regnum, unsigned val)
{
	VERIFY_CPUNUM_VOID(cpunum_set_reg);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_reg)(regnum, val);
	cpuintrf_pop_context();
}


/*--------------------------
 	Get/set PC
--------------------------*/

offs_t cpunum_get_pc(int cpunum)
{
	offs_t result;
	VERIFY_CPUNUM(0, cpunum_get_pc);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.get_pc)();
	cpuintrf_pop_context();
	return result;
}


offs_t cpunum_get_pc_byte(int cpunum)
{
	offs_t base, pc;
	int shift;

	VERIFY_CPUNUM(0, cpunum_get_pc_byte);
	shift = cpu[cpunum].intf.address_shift;
	base = cpu[cpunum].intf.pgm_memory_base;
	cpuintrf_push_context(cpunum);
	pc = (*cpu[cpunum].intf.get_pc)();
	cpuintrf_pop_context();
	return base + ((shift < 0) ? (pc << -shift) : (pc >> shift));
}


void cpunum_set_pc(int cpunum, offs_t val)
{
	VERIFY_CPUNUM_VOID(cpunum_get_pc);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_pc)(val);
	cpuintrf_pop_context();
}


void cpunum_set_op_base(int cpunum, unsigned val)
{
	VERIFY_CPUNUM_VOID(cpunum_set_op_base);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_op_base)(val);
	cpuintrf_pop_context();
}


/*--------------------------
 	Get/set SP
--------------------------*/

offs_t cpunum_get_sp(int cpunum)
{
	offs_t result;
	VERIFY_CPUNUM(0, cpunum_get_sp);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.get_sp)();
	cpuintrf_pop_context();
	return result;
}


void cpunum_set_sp(int cpunum, offs_t val)
{
	VERIFY_CPUNUM_VOID(cpunum_set_sp);
	cpuintrf_push_context(cpunum);
	(*cpu[cpunum].intf.set_sp)(val);
	cpuintrf_pop_context();
}


/*--------------------------
 	Disassembly
--------------------------*/

unsigned cpunum_dasm(int cpunum, char *buffer, unsigned pc)
{
	unsigned result;
	VERIFY_CPUNUM(1, cpunum_dasm);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.cpu_dasm)(buffer, pc);
	cpuintrf_pop_context();
	return result;
}


/*--------------------------
 	Register dumps
--------------------------*/

const char *cpunum_flags(int cpunum)
{
	const char *result;
	VERIFY_CPUNUM("", cpunum_flags);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_FLAGS);
	cpuintrf_pop_context();
	return result;
}


const char *cpunum_dump_reg(int cpunum, int regnum)
{
	const char *result;
	VERIFY_CPUNUM("", cpunum_dump_reg);
	cpuintrf_push_context(cpunum);
	result = (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_REG + regnum);
	cpuintrf_pop_context();
	return result;
}


/*--------------------------
 	State dumps
--------------------------*/

const char *cpunum_dump_state(int cpunum)
{
	static char buffer[1024+1];
	VERIFY_CPUNUM("", cpunum_dump_state);
	cpuintrf_push_context(cpunum);
	strcpy(buffer, activecpu_dump_state());
	cpuintrf_pop_context();
	return buffer;
}


/*--------------------------
 	Get/set static info
--------------------------*/

#define CPUNUM_FUNC(rettype, name, defresult, result)		\
rettype name(int cpunum)									\
{ 															\
	VERIFY_CPUNUM(defresult, name)							\
	return result;											\
}

CPUNUM_FUNC(int,          cpunum_default_irq_line,   0,  cpu[cpunum].intf.irq_int)
CPUNUM_FUNC(int,          cpunum_default_irq_vector, 0,  cpu[cpunum].intf.default_vector)
CPUNUM_FUNC(unsigned,     cpunum_address_bits,       0,  cpu[cpunum].intf.address_bits)
CPUNUM_FUNC(unsigned,     cpunum_address_mask,       0,  0xffffffffUL >> (32 - cpu[cpunum].intf.address_bits))
CPUNUM_FUNC(int,          cpunum_address_shift,      0,  cpu[cpunum].intf.address_shift)
CPUNUM_FUNC(unsigned,     cpunum_endianess,          0,  cpu[cpunum].intf.endianess)
CPUNUM_FUNC(unsigned,     cpunum_databus_width,      0,  cpu[cpunum].intf.databus_width)
CPUNUM_FUNC(unsigned,     cpunum_align_unit,         0,  cpu[cpunum].intf.align_unit)
CPUNUM_FUNC(unsigned,     cpunum_max_inst_len,       0,  cpu[cpunum].intf.max_inst_len)
CPUNUM_FUNC(const char *, cpunum_name,               "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_NAME))
CPUNUM_FUNC(const char *, cpunum_core_family,        "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_FAMILY))
CPUNUM_FUNC(const char *, cpunum_core_version,       "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_VERSION))
CPUNUM_FUNC(const char *, cpunum_core_file,          "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_FILE))
CPUNUM_FUNC(const char *, cpunum_core_credits,       "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_CREDITS))
CPUNUM_FUNC(const char *, cpunum_reg_layout,         "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_REG_LAYOUT))
CPUNUM_FUNC(const char *, cpunum_win_layout,         "", (*cpu[cpunum].intf.cpu_info)(NULL, CPU_INFO_WIN_LAYOUT))



/*************************************
 *
 *	Static info about a type of CPU
 *
 *************************************/

#define CPUTYPE_FUNC(rettype, name, defresult, result)		\
rettype name(int cputype)									\
{ 															\
	cputype &= ~CPU_FLAGS_MASK;								\
	if (cputype >= 0 && cputype < CPU_COUNT)				\
		return result;										\
	else													\
		logerror(#name "() called with invalid cpu type!\n");\
	return defresult;										\
}

CPUTYPE_FUNC(int,          cputype_default_irq_line,   0,  cpuintrf[cputype].irq_int)
CPUTYPE_FUNC(int,          cputype_default_irq_vector, 0,  cpuintrf[cputype].default_vector)
CPUTYPE_FUNC(unsigned,     cputype_address_bits,       0,  cpuintrf[cputype].address_bits)
CPUTYPE_FUNC(unsigned,     cputype_address_mask,       0,  0xffffffffUL >> (32 - cpuintrf[cputype].address_bits))
CPUTYPE_FUNC(int,          cputype_address_shift,      0,  cpuintrf[cputype].address_shift)
CPUTYPE_FUNC(unsigned,     cputype_endianess,          0,  cpuintrf[cputype].endianess)
CPUTYPE_FUNC(unsigned,     cputype_databus_width,      0,  cpuintrf[cputype].databus_width)
CPUTYPE_FUNC(unsigned,     cputype_align_unit,         0,  cpuintrf[cputype].align_unit)
CPUTYPE_FUNC(unsigned,     cputype_max_inst_len,       0,  cpuintrf[cputype].max_inst_len)
CPUTYPE_FUNC(const char *, cputype_name,               "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_NAME))
CPUTYPE_FUNC(const char *, cputype_core_family,        "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_FAMILY))
CPUTYPE_FUNC(const char *, cputype_core_version,       "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_VERSION))
CPUTYPE_FUNC(const char *, cputype_core_file,          "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_FILE))
CPUTYPE_FUNC(const char *, cputype_core_credits,       "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_CREDITS))
CPUTYPE_FUNC(const char *, cputype_reg_layout,         "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_REG_LAYOUT))
CPUTYPE_FUNC(const char *, cputype_win_layout,         "", (*cpuintrf[cputype].cpu_info)(NULL, CPU_INFO_WIN_LAYOUT))



/*************************************
 *
 *	Dump states of all CPUs
 *
 *************************************/

void cpu_dump_states(void)
{
	int cpunum;

	for (cpunum = 0; cpunum < totalcpu; cpunum++)
		puts(cpunum_dump_state(cpunum));
	fflush(stdout);
}



/*************************************
 *
 *	Dummy CPU definition
 *
 *************************************/

static void dummy_init(void) { }
static void dummy_reset(void *param) { }
static void dummy_exit(void) { }
static int dummy_execute(int cycles) { return cycles; }
static void dummy_burn(int cycles) { }
static unsigned dummy_get_context(void *regs) { return 0; }
static void dummy_set_context(void *regs) { }
static unsigned dummy_get_pc(void) { return 0; }
static void dummy_set_pc(unsigned val) { }
static unsigned dummy_get_sp(void) { return 0; }
static void dummy_set_sp(unsigned val) { }
static unsigned dummy_get_reg(int regnum) { return 0; }
static void dummy_set_reg(int regnum, unsigned val) { }
static void dummy_set_nmi_line(int state) { }
static void dummy_set_irq_line(int irqline, int state) { }
static void dummy_set_irq_callback(int (*callback)(int irqline)) { }

static const char *dummy_info(void *context, int regnum)
{
	switch (regnum)
	{
		case CPU_INFO_NAME: return "Dummy";
		case CPU_INFO_FAMILY: return "no CPU";
		case CPU_INFO_VERSION: return "0.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "The MAME team.";
	}
	return "";
}

static unsigned dummy_dasm(char *buffer, unsigned pc)
{
	strcpy(buffer, "???");
	return 1;
}



/*************************************
 *
 *	68000 reset kludge
 *
 *************************************/

#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
void cpu_set_m68k_reset(int cpunum, void (*resetfn)(void))
{
	void m68k_set_reset_instr_callback(void (*callback)(void));

	if ( 1
#if (HAS_M68000)
		&& cpu[cpunum].cputype != CPU_M68000
#endif
#if (HAS_M68010)
		&& cpu[cpunum].cputype != CPU_M68010
#endif
#if (HAS_M68020)
		&& cpu[cpunum].cputype != CPU_M68020
#endif
#if (HAS_M68EC020)
		&& cpu[cpunum].cputype != CPU_M68EC020
#endif
		)
	{
		logerror("Trying to set m68k reset vector on non-68k cpu\n");
		exit(1);
	}

	cpuintrf_push_context(cpunum);
	m68k_set_reset_instr_callback(resetfn);
	cpuintrf_pop_context();
}
#endif
