extern void ScreenClear (void);	/* JB 971215 */
extern void ScreenPutString (const char *text, int color, int x, int y);
extern void ScreenSetCursor (int x, int y);
extern int /* key */ osd_debug_readkey (void);	/* JB 980103 */
extern void MAME_Debug(void);

#ifdef WIN32
#define GFX_TEXT 0
enum {	BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,	/* JB 980208 */
		DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };

extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif

#ifdef macintosh
#define GFX_TEXT 0
enum {	BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,	/* JB 980208 */
		DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };

extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif	/* macintosh */
