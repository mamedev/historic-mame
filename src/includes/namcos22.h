#include "driver.h"
#include "vidhrdw/generic.h"

extern enum namcos22_gametype
{
	NAMCOS22_AIR_COMBAT22,
	NAMCOS22_ALPINE_RACER,
	NAMCOS22_CYBER_COMMANDO,
	NAMCOS22_CYBER_CYCLES,
	NAMCOS22_PROP_CYCLE,
	NAMCOS22_RAVE_RACER,
	NAMCOS22_RIDGE_RACER,
	NAMCOS22_TIME_CRISIS,
	NAMCOS22_VICTORY_LAP
} namcos22_gametype;

#define NAMCOS22_NUM_ROWS 30
#define NAMCOS22_NUM_COLS 40
#define NAMCOS22_ALPHA_GFX 1

extern data32_t *namcos22_cgram;
extern data32_t *namcos22_textram;
extern data32_t *namcos22_polygonram;

READ32_HANDLER( namcos22_cgram_r );
WRITE32_HANDLER( namcos22_cgram_w );
READ32_HANDLER( namcos22_paletteram_r );
WRITE32_HANDLER( namcos22_paletteram_w );
READ32_HANDLER( namcos22_textram_r );
WRITE32_HANDLER( namcos22_textram_w );

VIDEO_UPDATE( namcos22 );

VIDEO_START( namcos22s );
VIDEO_UPDATE( namcos22s );
