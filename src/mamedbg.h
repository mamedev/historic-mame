#ifndef _MAMEDBG_H
#define _MAMEDBG_H

#ifndef macintosh
#ifndef WIN32
#ifndef UNIX
	#define __INLINE__ static __inline__	/* keep allegro.h happy */
	#include <allegro.h>
	#undef __INLINE__
#endif
#endif
#endif

#include "driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef macintosh /* JB 980208 */
#define uclock_t clock_t
#define	uclock clock
#define UCLOCKS_PER_SEC CLOCKS_PER_SEC
#endif

#ifdef UNIX
#include "xmame.h" /* defines uclock types depending on arch */
#endif

#include "osdepend.h"
#include "osd_dbg.h"
#include "cpuintrf.h"
#if (HAS_Z80)
#include "cpu/z80/z80.h"
#endif
#if (HAS_8080 || HAS_8085A)
#include "cpu/i8085/i8085.h"
#endif
#if (HAS_M6502 || HAS_M65C02 || HAS_M6510)
#include "cpu/m6502/m6502.h"
#endif
#if (HAS_H6280)
#include "cpu/h6280/h6280.h"
#endif
#if (HAS_I86)
#include "cpu/i86/i86intrf.h"
#endif
#if (HAS_I8035 || HAS_I8039 || HAS_I8048 || HAS_N7751)
#include "cpu/i8039/i8039.h"
#endif
#if (HAS_M6800 || HAS_M6801 || HAS_M6802 || HAS_M6803 || HAS_M6808 || HAS_HD63701)
#include "cpu/m6800/m6800.h"
#endif
#if (HAS_M6805 || HAS_M68705 || HAS_HD63705)
#include "cpu/m6805/m6805.h"
#endif
#if (HAS_M6309 || HAS_M6809)
#include "cpu/m6809/m6809.h"
#endif
#if (HAS_M68000 || defined HAS_M68010 || HAS_M68020)
#include "cpu/m68000/m68000.h"
#endif
#if (HAS_T11)
#include "cpu/t11/t11.h"
#endif
#if (HAS_S2650)
#include "cpu/s2650/s2650.h"
#endif
#if (HAS_TMS34010)
#include "cpu/tms34010/tms34010.h"
#endif
#if (HAS_TMS9900)
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
#if (HAS_PDP1)
#include "cpu/pdp1/pdp1.h"
#endif


#define MEM1DEFAULT 		0x0000
#define MEM2DEFAULT 		0x8000
#define COLOR_TITLE 		YELLOW
#define COLOR_LINE			LIGHTCYAN
#define COLOR_REGS			WHITE
#define COLOR_DASM          WHITE
#define COLOR_FLAG			WHITE
#define COLOR_CMDS			WHITE
#define COLOR_BREAKPOINT	YELLOW
#define COLOR_PC			(WHITE+BLUE*16) /* MB 980103 */
#ifndef UNIX
#define COLOR_CURSOR		(WHITE+RED*16)	/* MB 980103 */
#else
#define COLOR_CURSOR		MAGENTA
#endif
#define COLOR_COMMANDINFO	(BLACK+LIGHTGRAY*16)	/* MB 980201 */
#define COLOR_CODE			WHITE
#define COLOR_CHANGES		LIGHTCYAN
#define COLOR_MEM1			WHITE
#define COLOR_MEM2			WHITE
#define COLOR_ERROR 		(YELLOW+RED*16)
#define COLOR_PROMPT		CYAN

#endif /* _MAMEDBG_H */
