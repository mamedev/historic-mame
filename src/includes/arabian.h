/***************************************************************************

	Sun Electronics Arabian hardware

	driver by Dan Boris

***************************************************************************/

#include "driver.h"


/*----------- defined in vidhrdw/arabian.c -----------*/

extern UINT8 arabian_video_control;
extern UINT8 arabian_flip_screen;

int arabian_vh_start(void);
void arabian_vh_stop(void);
void arabian_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void arabian_vh_convert_color_prom(unsigned char *obsolete, unsigned short *colortable, const unsigned char *color_prom);

WRITE_HANDLER( arabian_blitter_w );
WRITE_HANDLER( arabian_videoram_w );
