/***************************************************************************

	Game Driver for Video System Mahjong series and Pipe Dream.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/04 -
	and Bryan McPhail, Nicola Salmoria, Aaron Giles

***************************************************************************/

#include "driver.h"


/*----------- defined in vidhrdw/fromance.c -----------*/

int fromance_vh_start(void);
void fromance_vh_stop(void);
void fromance_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void pipedrm_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

WRITE_HANDLER( fromance_crtc_data_w );
WRITE_HANDLER( fromance_crtc_register_w );

WRITE_HANDLER( fromance_gfxreg_w );

WRITE_HANDLER( fromance_scroll_w );

READ_HANDLER( fromance_paletteram_r );
WRITE_HANDLER( fromance_paletteram_w );

READ_HANDLER( fromance_videoram_r );
WRITE_HANDLER( fromance_videoram_w );


