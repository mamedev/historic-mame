#ifndef _OSD_DBG_H
#define _OSD_DBG_H

extern void ScreenClear (void); /* JB 971215 */
extern void ScreenPutChar (int ch, int attr, int x, int y);
extern void ScreenPutString (const char *text, int color, int x, int y);

/* Note: y first (row), then x (column) !! */
extern void ScreenSetCursor (int y, int x);
extern int /* key */ osd_debug_readkey (void);	/* JB 980103 */

enum {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
    DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };

#ifdef MAME_DEBUG
/***************************************************************************
 *
 * The following functions are defined in mamedbg.c
 *
 ***************************************************************************/
/* What EA address to set with debug_ea_info (origin) */
enum {
    EA_DST,
    EA_SRC
};

/* Size of the data element accessed (or the immediate value) */
enum {
	EA_DEFAULT,
	EA_INT8,
	EA_UINT8,
	EA_INT16,
	EA_UINT16,
	EA_INT32,
	EA_UINT32,
	EA_SIZE
};

/* Access modes for effective addresses to debug_ea_info */
enum {
    EA_NONE,        /* no EA mode */
    EA_VALUE,       /* immediate value */
	EA_ABS_PC,		/* change PC absolute (JMP or CALL type opcodes) */
	EA_REL_PC,		/* change PC relative (BRA or JR type opcodes) */
	EA_MEM_RD,		/* read memory */
	EA_MEM_WR,		/* write memory */
	EA_MEM_RDWR,	/* read then write memory */
	EA_PORT_RD, 	/* read i/o port */
	EA_PORT_WR, 	/* write i/o port */
    EA_COUNT
};

/*
 * This function can (should) be called by a disassembler to set
 * information for the debugger. It sets the address, size and type
 * of a memory or port access, an absolute or relative branch or
 * an immediate value and at the same time returns a string that
 * contains a literal hex string for that address.
 * Later it could also return a symbol for that address and access.
 */
extern const char *set_ea_info( int what, unsigned address, int size, int access );

/* Startup and shutdown functions; called from cpu_run */
extern void mame_debug_init(void);
extern void mame_debug_exit(void);

/* If this flag is set, a CPU core should call MAME_Debug from it's execution loop */
extern int mame_debug;

/* This is the main entry into the mame debugger */
extern void MAME_Debug(void);

#define CALL_MAME_DEBUG if( mame_debug ) MAME_Debug()

#if defined WIN32 || defined macintosh || defined UNIX
#define GFX_TEXT 0
extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif

#else	/* MAME_DEBUG */

#define	CALL_MAME_DEBUG

#endif	/* !MAME_DEBUG */

#endif	/* _OSD_DBG_H */

