/***************************************************************************
  vidhrdw/mole.c
  Functions to emulate the video hardware of Mole Attack!.
  Mole Attack's Video hardware is essentially two banks of 512 characters.
  The program uses a single byte to indicate which character goes in each location,
  and uses a control location (0x8400) to select the character sets
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int tile_bank;
static UINT16 *tile_data;
#define NUM_ROWS 25
#define NUM_COLS 40
#define NUM_TILES (NUM_ROWS*NUM_COLS)

PALETTE_INIT( moleattack ){
	int i;
	for( i=0; i<8; i++ ){
		int r,g,b;
		r = (i&1)?0xff:0x00;
		g = (i&4)?0xff:0x00;
		b = (i&2)?0xff:0x00;
		palette_set_color(i,r,g,b);
	}
}

VIDEO_START( moleattack ){
	tile_data = (UINT16 *)auto_malloc( NUM_TILES*sizeof(UINT16) );
	if( !tile_data )
		return 1;
	dirtybuffer = auto_malloc( NUM_TILES );
	if( !dirtybuffer )
		return 1;
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	if( !tmpbitmap )
		return 1;
	memset( dirtybuffer, 1, NUM_TILES );
	return 0;
}

WRITE_HANDLER( moleattack_videoram_w ){
	if( offset<NUM_TILES ){
		if( tile_data[offset]!=data ){
			dirtybuffer[offset] = 1;
			tile_data[offset] = data | (tile_bank<<8);
		}
	}
	else if( offset==0x3ff ){ /* hack!  erase screen */
		memset( dirtybuffer, 1, NUM_TILES );
		memset( tile_data, 0, NUM_TILES*sizeof(UINT16) );
	}
}

WRITE_HANDLER( moleattack_tilesetselector_w ){
	tile_bank = data;
}

VIDEO_UPDATE( moleattack ){
	int offs;

	if( get_vh_global_attribute_changed() )
		memset( dirtybuffer, 1, NUM_TILES );

	for( offs=0; offs<NUM_TILES; offs++ ){
		if( dirtybuffer[offs] ){
			UINT16 code = tile_data[offs];
			drawgfx( tmpbitmap, Machine->gfx[(code&0x200)?1:0],
				code&0x1ff,
				0, /* color */
				0,0, /* no flip */
				(offs%NUM_COLS)*8, /* xpos */
				(offs/NUM_COLS)*8, /* ypos */
				0, /* no clip */
				TRANSPARENCY_NONE,0 );

			dirtybuffer[offs] = 0;
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
