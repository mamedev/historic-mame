/*********************************************************************

  usrintrf.h

  Functions used to handle MAME's crude user interface.

*********************************************************************/

#ifndef USRINTRF_H
#define USRINTRF_H

struct DisplayText
{
	const char *text;	/* 0 marks the end of the array */
	int color;	/* see #defines below */
	int x;
	int y;
};

#define UI_COLOR_NORMAL 0	/* white on black text */
#define UI_COLOR_INVERSE 1	/* black on white text */

#define SEL_BITS 12		/* main menu selection mask */
#define SEL_BITS2 4		/* submenu selection masks */
#define SEL_MASK ((1<<SEL_BITS)-1)
#define SEL_MASK2 ((1<<SEL_BITS2)-1)

extern int need_to_clear_bitmap;	/* used to tell updatescreen() to clear the bitmap */

struct GfxElement *builduifont(void);
void pick_uifont_colors(void);
void displaytext(const struct DisplayText *dt,int erase,int update_screen);
void ui_text(const char *buf,int x,int y);
void ui_drawbox(int leftx,int topy,int width,int height);
void ui_displaymessagewindow(const char *text);
void ui_displaymenu(const char **items,const char **subitems,char *flag,int selected,int arrowize_subitem);
int showcopyright(void);
int showgamewarnings(void);
void set_ui_visarea (int xmin, int ymin, int xmax, int ymax);

void init_user_interface(void);
int handle_user_interface(void);

int onscrd_active(void);
int setup_active(void);

void CLIB_DECL usrintf_showmessage(const char *text,...);

#endif
