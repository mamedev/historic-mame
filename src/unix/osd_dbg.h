#ifndef _OSD_DBG_H
#define _OSD_DBG_H

#ifdef MAME_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * Note, uclock isn't needed anymore, so if xmame.h was only
 * included for that reason it might as well be omitted now.
 */
#include "xmame.h" /* defines uclock types depending on arch */

/*
 * Specify the format of arguments to win_printf and win_set_title
 * Can be empty if your compiler doesn't support argument checking.
 * If you use GCC you can use the following macro.
 */
#ifdef __GCC__
#define ARGFMT	__attribute__((format(printf,2,3)))
#else
#define ARGFMT
#endif

/*
 * Additional declaration specifier for functions that use variable
 * arguments (...) or C library functions like qsort (WIN32 needs this)
 */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef INVALID
#define INVALID 0xffffffff
#endif

enum {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
	DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE
};

/*
 * Characters used by the window engine to draw background, frames and caption
 * You might want to put some ASCII characters here or the appropriate line
 * characters for your machine.
 */

#if USE_ASCII_CHARSET

#ifndef WIN_EMPTY
#define WIN_EMPTY	' '
#endif
#ifndef CAPTION_L
#define CAPTION_L	'['
#endif
#ifndef CAPTION_R
#define CAPTION_R	']'
#endif
#ifndef FRAME_TL
#define FRAME_TL	'+'
#endif
#ifndef FRAME_BL
#define FRAME_BL	'+'
#endif
#ifndef FRAME_TR
#define FRAME_TR	'+'
#endif
#ifndef FRAME_BR
#define FRAME_BR	'+'
#endif
#ifndef FRAME_V
#define FRAME_V 	'|'
#endif
#ifndef FRAME_H
#define FRAME_H 	'-'
#endif

#else

#ifndef WIN_EMPTY
#define WIN_EMPTY   '°'
#endif
#ifndef CAPTION_L
#define CAPTION_L   '®'
#endif
#ifndef CAPTION_R
#define CAPTION_R   '¯'
#endif
#ifndef FRAME_TL
#define FRAME_TL    'Ú'
#endif
#ifndef FRAME_BL
#define FRAME_BL    'À'
#endif
#ifndef FRAME_TR
#define FRAME_TR    '¿'
#endif
#ifndef FRAME_BR
#define FRAME_BR    'Ù'
#endif
#ifndef FRAME_V
#define FRAME_V     '³'
#endif
#ifndef FRAME_H
#define FRAME_H     'Ä'
#endif

#endif

/***************************************************************************
 *
 * These functions have to be provided by the OS specific code
 *
 ***************************************************************************/
extern void osd_put_screen_char (int ch, int attr, int x, int y);
extern void osd_set_screen_curpos (int x, int y);
extern int /* key */ osd_debug_readkey (void);	/* JB 980103 */

/***************************************************************************
 * Note: I renamed the set_gfx_mode function to avoid a name clash with
 * DOS' allegro.h. The new function should set any mode that is available
 * on the platform and the get_screen_size function should return the
 * resolution that is actually available.
 * The minimum required size is 80x25 characters, anything higher is ok.
 ***************************************************************************/
extern void osd_set_screen_size (unsigned width, unsigned height);
extern void osd_get_screen_size (unsigned *width, unsigned *height);

/***************************************************************************
 * Convenience macro for the CPU cores, this is defined to empty
 * if MAME_DEBUG is not specified, so a CPU core can simply add
 * CALL_MAME_DEBUG; before executing an instruction
 ***************************************************************************/
#define CALL_MAME_DEBUG if( mame_debug ) MAME_Debug()

#else	/* MAME_DEBUG */

#define	CALL_MAME_DEBUG

#endif	/* !MAME_DEBUG */

#endif	/* _OSD_DBG_H */

