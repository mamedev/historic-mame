extern void ScreenClear (void);	/* JB 971215 */
extern void ScreenPutString (const char *text, int color, int x, int y);
extern void ScreenSetCursor (int x, int y);
extern int /* key */ readkey (void);
extern void clear_keybuf (void);
extern void SetVideoMode (int mode);
extern int /* pressed */ keypressed (void);
extern void MAME_Debug(void);

#ifdef WIN32
#define GFX_TEXT 0
#define BLACK       0
#define WHITE      15
#define LIGHTGREEN 10
#define LIGHTCYAN  11
#define YELLOW     14
#define RED         4
#define CYAN        7
extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif

#ifdef macintosh
#define GFX_TEXT 0
enum { BLACK, WHITE, LIGHTGREEN, LIGHTCYAN, YELLOW, RED, CYAN };
extern void set_gfx_mode (int mode, int width, int height, int u1, int u2);
#endif	/* macintosh */
