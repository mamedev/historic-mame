/*************************************************************************

	Driver for Midway V-Unit games

**************************************************************************/

#include "wmswolfu.h"


/*----------- defined in vidhrdw/wmsvunit.c -----------*/

extern data16_t *wmsvunit_videoram;
extern data32_t *wmsvunit_textureram;

WRITE32_HANDLER( wmsvunit_dma_queue_w );
READ32_HANDLER( wmsvunit_dma_queue_entries_r );
READ32_HANDLER( wmsvunit_dma_trigger_r );

WRITE32_HANDLER( wmsvunit_page_control_w );
READ32_HANDLER( wmsvunit_page_control_r );

WRITE32_HANDLER( wmsvunit_video_control_w );
READ32_HANDLER( wmsvunit_scanline_r );

WRITE32_HANDLER( wmsvunit_videoram_w );
READ32_HANDLER( wmsvunit_videoram_r );

WRITE32_HANDLER( wmsvunit_paletteram_w );

WRITE32_HANDLER( wmsvunit_textureram_w );
READ32_HANDLER( wmsvunit_textureram_r );

VIDEO_START( wmsvunit );
VIDEO_UPDATE( wmsvunit );
