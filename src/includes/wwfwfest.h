extern data16_t *wwfwfest_fg0_videoram, *wwfwfest_bg0_videoram, *wwfwfest_bg1_videoram;
extern int wwfwfest_pri;
extern int wwfwfest_bg0_scrollx, wwfwfest_bg0_scrolly, wwfwfest_bg1_scrollx, wwfwfest_bg1_scrolly;

int wwfwfest_vh_start(void);
void wwfwfest_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
WRITE16_HANDLER( wwfwfest_fg0_videoram_w );
WRITE16_HANDLER( wwfwfest_bg0_videoram_w );
WRITE16_HANDLER( wwfwfest_bg1_videoram_w );
