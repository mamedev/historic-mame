#ifndef _OSD_DBG_H
#define _OSD_DBG_H

extern void ScreenClear (void); /* JB 971215 */
extern void ScreenPutChar (int ch, int attr, int x, int y);
extern void ScreenPutString (const char *text, int color, int x, int y);

/* Note: y first (row), then x (column) !! */
extern void ScreenSetCursor (int y, int x);
extern int /* key */ osd_debug_readkey (void);	/* JB 980103 */

extern void MAME_Debug(void);

/* Where did that come from before? I couldn't find it for DOS either ... */
enum {
	BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
	DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };

#if defined WIN32 || defined macintosh || defined UNIX
#define GFX_TEXT 0

extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif

#endif	/* _OSD_DBG_H */

