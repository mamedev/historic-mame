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

#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#define MEM1DEFAULT         0x0000
#define MEM2DEFAULT 		0x8000
#define COLOR_TITLE 		YELLOW
#define COLOR_LINE			LIGHTCYAN
#define COLOR_REGS			WHITE
#define COLOR_DASM          WHITE
#define COLOR_FLAG			WHITE
#define COLOR_CMDS			WHITE
#define COLOR_BRK_EXEC		YELLOW
#define COLOR_BRK_DATA		(YELLOW+BLUE*16)
#define COLOR_BRK_REGS		(YELLOW+BLUE*16)
#define COLOR_PC			(WHITE+BLUE*16) /* MB 980103 */
#ifndef UNIX
#define COLOR_CURSOR		(WHITE+RED*16)	/* MB 980103 */
#else
#define COLOR_CURSOR		MAGENTA
#endif
#define COLOR_HELP			(WHITE+BLUE*16)
#define COLOR_CODE			WHITE
#define COLOR_CHANGES		LIGHTCYAN
#define COLOR_MEM1			WHITE
#define COLOR_MEM2			WHITE
#define COLOR_ERROR 		(YELLOW+RED*16)
#define COLOR_PROMPT		CYAN

#endif /* _MAMEDBG_H */
