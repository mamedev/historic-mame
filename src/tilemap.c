/* tilemap.c

	When the videoram for a tile changes, call tilemap_mark_tile_dirty
	with the appropriate memory offset.

	In the video driver, follow these steps:

	1)	Set each tilemap's scroll registers.

	2)	Call tilemap_update( ALL_TILEMAPS ).

	3)	Call palette_init_used_colors().
		Mark the colors used by sprites.
		Call palette recalc().

	4)	Call tilemap_draw to draw the tilemaps to the screen, from back to front.

	Notes:
	-	tilemap_get_pixmap is not yet supported in the new code.

	-	You can currently configure a tilemap as xscroll + scrolling columns or
		yscroll + scrolling rows, but not both types of scrolling simultaneously.

	-	Optimizations for the new tilemap code (especially tilemap_draw) are not complete.

	To Do:
	-	Screenwise scrolling (orthagonal to row/colscroll); this will require a radically different
		rendering and visibility marking approach.  It's not sufficient to render to a screen-sized
		bitmap as an intermediate step.

	-	ROZ blitting support (could make tilemap_get_pixmap obsolete).  Unfortunately, this can
		also interact with linescroll in complex ways.

	-	Dynamically resizable tilemaps (num rows/cols, tile size) are used by several games.

	-	modulus to next line of pen data should be settable.

	-	Scroll registers should be automatically recomputed when screen flips.

	-	Logical handling of TILEMAP_FRONT|TILEMAP_BACK (but see below).

	-	Consider alternate priority buffer implementations.  It would be nice to be able to render each
		layer in a single pass, handling multiple tile priorities, split tiles, etc. all seamlessly,
		then combine the layers and sprites appropriately as a final step.
*/

#if 1 /* set to 1 to use old tilemap manager, 0 to use new tilemap manager */

#ifndef DECLARE

#include "driver.h"
#include "tilemap.h"
#include "state.h"

struct cached_tile_info {
	const UINT8 *pen_data;
	const UINT32 *pal_data;
	UINT32 pen_usage;
	UINT32 flags;
};

struct tilemap_mask {
	struct osd_bitmap *bitmask;
	int line_offset;
	UINT8 *data;
	UINT8 **data_row;
};

struct tilemap {
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	int *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	int logical_flip_to_cached_flip[4];

	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int memory_offset );

	UINT32 max_memory_offset;
	UINT32 num_tiles;
	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;
	UINT32 cached_tile_width, cached_tile_height;
	UINT32 cached_width, cached_height;

	struct cached_tile_info *cached_tile_info;

	int dx, dx_if_flipped;
	int dy, dy_if_flipped;
	int scrollx_delta, scrolly_delta;

	int enable;
	int attributes;

	int type;
	int transparent_pen;
	unsigned int transmask[4];

	int bNeedRender;

	void (*draw)( int, int );
	void (*draw_opaque)( int, int );
	void (*draw_alpha)( int, int );

	UINT8 *priority,	/* priority for each tile */
		**priority_row;

	UINT8 *visible; /* boolean flag for each tile */

	UINT8 *dirty_vram; /* boolean flag for each tile */

	UINT8 *dirty_pixels;

	int scroll_rows, scroll_cols;
	int *rowscroll, *colscroll;

	int orientation;
	int clip_left,clip_right,clip_top,clip_bottom;

	UINT16 tile_depth, tile_granularity;
	UINT8 *tile_dirty_map;

	/* cached color data */
	struct osd_bitmap *pixmap;
	int pixmap_line_offset;

	struct tilemap_mask *foreground;
	/* for transparent layers, or the front half of a split layer */

	struct tilemap_mask *background;
	/* for the back half of a split layer */

	struct tilemap *next; /* resource tracking */
};


#define SWAP(X,Y) {UINT32 temp=X; X=Y; Y=temp; }

static void draw_tile(
		struct tilemap *tilemap,
		UINT32 cached_indx,
		UINT32 col, UINT32 row
){
	struct osd_bitmap *pixmap = tilemap->pixmap;
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
	const UINT8 *pendata = cached_tile_info->pen_data;
	const UINT32 *paldata = cached_tile_info->pal_data;

	UINT32 flags = cached_tile_info->flags;
	int x, sx = tile_width*col;
	int sy,y1,y2,dy;

	switch( Machine->scrbitmap->depth )
	{
	case 32:
		if( flags&TILE_FLIPY ){
			y1 = tile_height*row+tile_height-1;
			y2 = y1-tile_height;
	 		dy = -1;
	 	}
	 	else {
			y1 = tile_height*row;
			y2 = y1+tile_height;
	 		dy = 1;
	 	}

		if( flags&TILE_FLIPX ){
			tile_width--;
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT32 *dest  = sx + (UINT32 *)pixmap->line[sy];
				for( x=tile_width; x>=0; x-- ) dest[x] = paldata[*pendata++];
			}
		}
		else {
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT32 *dest  = sx + (UINT32 *)pixmap->line[sy];
				for( x=0; x<tile_width; x++ ) dest[x] = paldata[*pendata++];
			}
		}
		break;
	case 15:
	case 16:
		if( flags&TILE_FLIPY ){
			y1 = tile_height*row+tile_height-1;
			y2 = y1-tile_height;
	 		dy = -1;
	 	}
	 	else {
			y1 = tile_height*row;
			y2 = y1+tile_height;
	 		dy = 1;
	 	}

		if( flags&TILE_FLIPX ){
			tile_width--;
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT16 *dest  = sx + (UINT16 *)pixmap->line[sy];
				for( x=tile_width; x>=0; x-- ) dest[x] = paldata[*pendata++];
			}
		}
		else {
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT16 *dest  = sx + (UINT16 *)pixmap->line[sy];
				for( x=0; x<tile_width; x++ ) dest[x] = paldata[*pendata++];
			}
		}
		break;
	case 8:
		if( flags&TILE_FLIPY ){
			y1 = tile_height*row+tile_height-1;
			y2 = y1-tile_height;
	 		dy = -1;
	 	}
	 	else {
			y1 = tile_height*row;
			y2 = y1+tile_height;
	 		dy = 1;
	 	}

		if( flags&TILE_FLIPX ){
			tile_width--;
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT8 *dest  = sx + (UINT8 *)pixmap->line[sy];
				for( x=tile_width; x>=0; x-- ) dest[x] = paldata[*pendata++];
			}
		}
		else {
			for( sy=y1; sy!=y2; sy+=dy ){
				UINT8 *dest  = sx + (UINT8 *)pixmap->line[sy];
				for( x=0; x<tile_width; x++ ) dest[x] = paldata[*pendata++];
			}
		}
		break;
	}
}

static void tmap_render( struct tilemap *tilemap ){
	if( tilemap->bNeedRender ){
		tilemap->bNeedRender = 0;
		if( tilemap->enable ){
			UINT8 *dirty_pixels = tilemap->dirty_pixels;
			const UINT8 *visible = tilemap->visible;
			UINT32 cached_indx = 0;
			UINT32 row,col;

			/* walk over cached rows/cols (better to walk screen coords) */
			for( row=0; row<tilemap->num_cached_rows; row++ ){
				for( col=0; col<tilemap->num_cached_cols; col++ ){
					if( visible[cached_indx] && dirty_pixels[cached_indx] ){
						draw_tile( tilemap, cached_indx, col, row );
						dirty_pixels[cached_indx] = 0;
					}
					cached_indx++;
				} /* next col */
			} /* next row */
		}
	}
}

struct osd_bitmap *tilemap_get_pixmap( struct tilemap * tilemap ){
profiler_mark(PROFILER_TILEMAP_DRAW);
	tmap_render( tilemap );
profiler_mark(PROFILER_END);
	return tilemap->pixmap; /* TBA */
}

void tilemap_set_transparent_pen( struct tilemap *tilemap, int pen ){
	tilemap->transparent_pen = pen;
}

void tilemap_set_transmask( struct tilemap *tilemap, int which, UINT32 penmask ){
	tilemap->transmask[which] = penmask;
}

void tilemap_set_depth( struct tilemap *tilemap, int tile_depth, int tile_granularity )
{
	if( tilemap->tile_dirty_map )
		free( tilemap->tile_dirty_map);
	tilemap->tile_dirty_map = malloc( Machine->drv->total_colors >> tile_granularity);
	if( tilemap->tile_dirty_map )
	{
		tilemap->tile_depth = tile_depth;
		tilemap->tile_granularity = tile_granularity;
	}
}

/***********************************************************************************/
/* some common mappings */

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	/* logical (col,row) -> memory offset */
	return row*num_cols + col;
}
UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	/* logical (col,row) -> memory offset */
	return col*num_rows + row;
}

/*********************************************************************************/

static struct osd_bitmap *create_tmpbitmap( int width, int height, int depth ){
	return osd_alloc_bitmap( width,height,depth );
}

static struct osd_bitmap *create_bitmask( int width, int height ){
	width = (width+7)/8; /* 8 bits per byte */
	return osd_alloc_bitmap( width,height, 8 );
}

/***********************************************************************************/

static int mappings_create( struct tilemap *tilemap ){
	int max_memory_offset = 0;
	UINT32 col,row;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	/* count offsets (might be larger than num_tiles) */
	for( row=0; row<num_logical_rows; row++ ){
		for( col=0; col<num_logical_cols; col++ ){
			UINT32 memory_offset = tilemap->get_memory_offset( col, row, num_logical_cols, num_logical_rows );
			if( memory_offset>max_memory_offset ) max_memory_offset = memory_offset;
		}
	}
	max_memory_offset++;
	tilemap->max_memory_offset = max_memory_offset;
	/* logical to cached (tilemap_mark_dirty) */
	tilemap->memory_offset_to_cached_indx = malloc( sizeof(int)*max_memory_offset );
	if( tilemap->memory_offset_to_cached_indx ){
		/* cached to logical (get_tile_info) */
		tilemap->cached_indx_to_memory_offset = malloc( sizeof(UINT32)*tilemap->num_tiles );
		if( tilemap->cached_indx_to_memory_offset ) return 0; /* no error */
		free( tilemap->memory_offset_to_cached_indx );
	}
	return -1; /* error */
}

static void mappings_dispose( struct tilemap *tilemap ){
	free( tilemap->cached_indx_to_memory_offset );
	free( tilemap->memory_offset_to_cached_indx );
}

static void mappings_update( struct tilemap *tilemap ){
	int logical_flip;
	UINT32 logical_indx, cached_indx;
	UINT32 num_cached_rows = tilemap->num_cached_rows;
	UINT32 num_cached_cols = tilemap->num_cached_cols;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	for( logical_indx=0; logical_indx<tilemap->max_memory_offset; logical_indx++ ){
		tilemap->memory_offset_to_cached_indx[logical_indx] = -1;
	}

	logerror("log size(%dx%d); cach size(%dx%d)\n",
			num_logical_cols,num_logical_rows,
			num_cached_cols,num_cached_rows);

	for( logical_indx=0; logical_indx<tilemap->num_tiles; logical_indx++ ){
		UINT32 logical_col = logical_indx%num_logical_cols;
		UINT32 logical_row = logical_indx/num_logical_cols;
		int memory_offset = tilemap->get_memory_offset( logical_col, logical_row, num_logical_cols, num_logical_rows );
		UINT32 cached_col = logical_col;
		UINT32 cached_row = logical_row;
		if( tilemap->orientation & ORIENTATION_SWAP_XY ) SWAP(cached_col,cached_row)
		if( tilemap->orientation & ORIENTATION_FLIP_X ) cached_col = (num_cached_cols-1)-cached_col;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) cached_row = (num_cached_rows-1)-cached_row;
		cached_indx = cached_row*num_cached_cols+cached_col;
		tilemap->memory_offset_to_cached_indx[memory_offset] = cached_indx;
		tilemap->cached_indx_to_memory_offset[cached_indx] = memory_offset;
	}
	for( logical_flip = 0; logical_flip<4; logical_flip++ ){
		int cached_flip = logical_flip;
		if( tilemap->attributes&TILEMAP_FLIPX ) cached_flip ^= TILE_FLIPX;
		if( tilemap->attributes&TILEMAP_FLIPY ) cached_flip ^= TILE_FLIPY;
#ifndef PREROTATE_GFX
		if( Machine->orientation & ORIENTATION_SWAP_XY ){
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPY;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPX;
		}
		else {
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPX;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPY;
		}
#endif
		if( tilemap->orientation & ORIENTATION_SWAP_XY ){
			cached_flip = ((cached_flip&1)<<1) | ((cached_flip&2)>>1);
		}
		tilemap->logical_flip_to_cached_flip[logical_flip] = cached_flip;
	}
}

/***********************************************************************************/

struct osd_bitmap *priority_bitmap; /* priority buffer (corresponds to screen bitmap) */
int priority_bitmap_line_offset;

static UINT8 flip_bit_table[0x100]; /* horizontal flip for 8 pixels */
static struct tilemap *first_tilemap; /* resource tracking */
static int screen_width, screen_height;
struct tile_info tile_info;

enum {
	TILE_TRANSPARENT,
	TILE_MASKED,
	TILE_OPAQUE
};

/* the following parameters are constant across tilemap_draw calls */
static struct {
	int clip_left, clip_top, clip_right, clip_bottom;
	int source_width, source_height;
	int dest_line_offset,source_line_offset,mask_line_offset;
	int dest_row_offset,source_row_offset,mask_row_offset;
	struct osd_bitmap *screen, *pixmap, *bitmask;
	UINT8 **mask_data_row;
	UINT8 **priority_data_row;
	int tile_priority;
	int tilemap_priority_code;
} blit;

#define MASKROWBYTES(W) (((W)+7)/8)

static void memsetbitmask8( UINT8 *dest, int value, const UINT8 *bitmask, int count ){
/* TBA: combine with memcpybitmask */
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] |= value;
		if( data&0x40 ) dest[1] |= value;
		if( data&0x20 ) dest[2] |= value;
		if( data&0x10 ) dest[3] |= value;
		if( data&0x08 ) dest[4] |= value;
		if( data&0x04 ) dest[5] |= value;
		if( data&0x02 ) dest[6] |= value;
		if( data&0x01 ) dest[7] |= value;
		if( --count == 0 ) break;
		dest+=8;
	}
}

static void memcpybitmask8( UINT8 *dest, const UINT8 *source, const UINT8 *bitmask, int count ){
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

static void memcpybitmask16( UINT16 *dest, const UINT16 *source, const UINT8 *bitmask, int count ){
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

static void memcpybitmask32( UINT32 *dest, const UINT32 *source, const UINT8 *bitmask, int count ){
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

static void blend16( UINT16 *dest, const UINT16 *source, int count ){
	for(;;){
		*dest = alpha_blend16(*dest, *source);
		if( --count == 0 ) break;
		source++;
		dest++;
	}
}

static void blendbitmask16( UINT16 *dest, const UINT16 *source, const UINT8 *bitmask, int count ){
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = alpha_blend16(dest[0], source[0]);
		if( data&0x40 ) dest[1] = alpha_blend16(dest[1], source[1]);
		if( data&0x20 ) dest[2] = alpha_blend16(dest[2], source[2]);
		if( data&0x10 ) dest[3] = alpha_blend16(dest[3], source[3]);
		if( data&0x08 ) dest[4] = alpha_blend16(dest[4], source[4]);
		if( data&0x04 ) dest[5] = alpha_blend16(dest[5], source[5]);
		if( data&0x02 ) dest[6] = alpha_blend16(dest[6], source[6]);
		if( data&0x01 ) dest[7] = alpha_blend16(dest[7], source[7]);
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

static void blend32( UINT32 *dest, const UINT32 *source, int count ){
	for(;;){
		*dest = alpha_blend32(*dest, *source);
		if( --count == 0 ) break;
		source++;
		dest++;
	}
}

static void blendbitmask32( UINT32 *dest, const UINT32 *source, const UINT8 *bitmask, int count ){
	for(;;){
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = alpha_blend32(dest[0], source[0]);
		if( data&0x40 ) dest[1] = alpha_blend32(dest[1], source[1]);
		if( data&0x20 ) dest[2] = alpha_blend32(dest[2], source[2]);
		if( data&0x10 ) dest[3] = alpha_blend32(dest[3], source[3]);
		if( data&0x08 ) dest[4] = alpha_blend32(dest[4], source[4]);
		if( data&0x04 ) dest[5] = alpha_blend32(dest[5], source[5]);
		if( data&0x02 ) dest[6] = alpha_blend32(dest[6], source[6]);
		if( data&0x01 ) dest[7] = alpha_blend32(dest[7], source[7]);
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

#define DEPTH 8
#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##8x8x8BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##16x16x8BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##32x32x8BPP args body
#include "tilemap.c"
#undef DEPTH

#define DEPTH 16
#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define blend blend16
#define blendbitmask blendbitmask16
#define DECLARE(function,args,body) static void function##8x8x16BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define DECLARE(function,args,body) static void function##16x16x16BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define DECLARE(function,args,body) static void function##32x32x16BPP args body
#include "tilemap.c"
#undef DEPTH
#undef blend
#undef blendbitmask

#define DEPTH 32
#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define blend blend32
#define blendbitmask blendbitmask32
#define DECLARE(function,args,body) static void function##8x8x32BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define DECLARE(function,args,body) static void function##16x16x32BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define DECLARE(function,args,body) static void function##32x32x32BPP args body
#include "tilemap.c"
#undef DEPTH
#undef blend
#undef blendbitmask

/*********************************************************************************/

static void mask_dispose( struct tilemap_mask *mask ){
	if( mask ){
		free( mask->data_row );
		free( mask->data );
		osd_free_bitmap( mask->bitmask );
		free( mask );
	}
}

static struct tilemap_mask *mask_create( struct tilemap *tilemap ){
	struct tilemap_mask *mask = malloc( sizeof(struct tilemap_mask) );
	if( mask ){
		mask->data = malloc( tilemap->num_tiles );
		mask->data_row = malloc( tilemap->num_cached_rows * sizeof(UINT8 *) );
		mask->bitmask = create_bitmask( tilemap->cached_width, tilemap->cached_height );
		if( mask->data && mask->data_row && mask->bitmask ){
			int row;
			for( row=0; row<tilemap->num_cached_rows; row++ ){
				mask->data_row[row] = mask->data + row*tilemap->num_cached_cols;
			}
			mask->line_offset = mask->bitmask->line[1] - mask->bitmask->line[0];
			return mask;
		}
	}
	mask_dispose( mask );
	return NULL;
}

/***********************************************************************************/

static void install_draw_handlers( struct tilemap *tilemap ){
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	tilemap->draw = tilemap->draw_opaque = tilemap->draw_alpha = NULL;
	switch( Machine->scrbitmap->depth )
	{
	case 32:
		if( tile_width==8 && tile_height==8 ){
			tilemap->draw = draw8x8x32BPP;
			tilemap->draw_opaque = draw_opaque8x8x32BPP;
			tilemap->draw_alpha = draw_alpha8x8x32BPP;
		}
		else if( tile_width==16 && tile_height==16 ){
			tilemap->draw = draw16x16x32BPP;
			tilemap->draw_opaque = draw_opaque16x16x32BPP;
			tilemap->draw_alpha = draw_alpha16x16x32BPP;
		}
		else if( tile_width==32 && tile_height==32 ){
			tilemap->draw = draw32x32x32BPP;
			tilemap->draw_opaque = draw_opaque32x32x32BPP;
			tilemap->draw_alpha = draw_alpha32x32x32BPP;
		}
		break;
	case 15:
		if( tile_width==8 && tile_height==8 ){
			tilemap->draw = draw8x8x16BPP;
			tilemap->draw_opaque = draw_opaque8x8x16BPP;
			tilemap->draw_alpha = draw_alpha8x8x16BPP;
		}
		else if( tile_width==16 && tile_height==16 ){
			tilemap->draw = draw16x16x16BPP;
			tilemap->draw_opaque = draw_opaque16x16x16BPP;
			tilemap->draw_alpha = draw_alpha16x16x16BPP;
		}
		else if( tile_width==32 && tile_height==32 ){
			tilemap->draw = draw32x32x16BPP;
			tilemap->draw_opaque = draw_opaque32x32x16BPP;
			tilemap->draw_alpha = draw_alpha32x32x16BPP;
		}
		break;
	case 16:
		if( tile_width==8 && tile_height==8 ){
			tilemap->draw = draw8x8x16BPP;
			tilemap->draw_opaque = draw_opaque8x8x16BPP;
			tilemap->draw_alpha = draw8x8x16BPP;
		}
		else if( tile_width==16 && tile_height==16 ){
			tilemap->draw = draw16x16x16BPP;
			tilemap->draw_opaque = draw_opaque16x16x16BPP;
			tilemap->draw_alpha = draw16x16x16BPP;
		}
		else if( tile_width==32 && tile_height==32 ){
			tilemap->draw = draw32x32x16BPP;
			tilemap->draw_opaque = draw_opaque32x32x16BPP;
			tilemap->draw_alpha = draw32x32x16BPP;
		}
		break;
	case 8:
		if( tile_width==8 && tile_height==8 ){
			tilemap->draw = draw8x8x8BPP;
			tilemap->draw_opaque = draw_opaque8x8x8BPP;
			tilemap->draw_alpha = draw8x8x8BPP;
		}
		else if( tile_width==16 && tile_height==16 ){
			tilemap->draw = draw16x16x8BPP;
			tilemap->draw_opaque = draw_opaque16x16x8BPP;
			tilemap->draw_alpha = draw16x16x8BPP;
		}
		else if( tile_width==32 && tile_height==32 ){
			tilemap->draw = draw32x32x8BPP;
			tilemap->draw_opaque = draw_opaque32x32x8BPP;
			tilemap->draw_alpha = draw32x32x8BPP;
		}
		break;
	}
}

/***********************************************************************************/

static void tilemap_reset(void)
{
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

int tilemap_init( void ){
	UINT32 value, data, bit;
	for( value=0; value<0x100; value++ ){
		data = 0;
		for( bit=0; bit<8; bit++ ) if( (value>>bit)&1 ) data |= 0x80>>bit;
		flip_bit_table[value] = data;
	}
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;
	first_tilemap = 0;
	state_save_register_func_postload(tilemap_reset);
	priority_bitmap = create_tmpbitmap( screen_width, screen_height, 8 );
	if( priority_bitmap ){
		priority_bitmap_line_offset = priority_bitmap->line[1] - priority_bitmap->line[0];
		return 0;
	}
	return -1;
}

void tilemap_close( void ){
	while( first_tilemap ){
		struct tilemap *next = first_tilemap->next;
		tilemap_dispose( first_tilemap );
		first_tilemap = next;
	}
	osd_free_bitmap( priority_bitmap );
}

/***********************************************************************************/

struct tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height, /* in pixels */
	int num_cols, int num_rows /* in tiles */
){
	struct tilemap *tilemap = calloc( 1,sizeof( struct tilemap ) );
	if( tilemap ){
		int num_tiles = num_cols*num_rows;
		tilemap->num_logical_cols = num_cols;
		tilemap->num_logical_rows = num_rows;
		if( Machine->orientation & ORIENTATION_SWAP_XY ){
		logerror("swap!!\n" );
			SWAP( tile_width, tile_height )
			SWAP( num_cols,num_rows )
		}
		tilemap->num_cached_cols = num_cols;
		tilemap->num_cached_rows = num_rows;
		tilemap->num_tiles = num_tiles;
		tilemap->cached_tile_width = tile_width;
		tilemap->cached_tile_height = tile_height;
		tilemap->cached_width = tile_width*num_cols;
		tilemap->cached_height = tile_height*num_rows;
		tilemap->tile_get_info = tile_get_info;
		tilemap->get_memory_offset = get_memory_offset;
		tilemap->orientation = Machine->orientation;
		tilemap->enable = 1;
		tilemap->type = type;
		tilemap->scroll_rows = 1;
		tilemap->scroll_cols = 1;
		tilemap->transparent_pen = -1;
		tilemap->tile_depth = 0;
		tilemap->tile_granularity = 0;
		tilemap->tile_dirty_map = 0;
		tilemap->cached_tile_info = calloc( num_tiles, sizeof(struct cached_tile_info) );
		tilemap->priority = calloc( num_tiles,1 );
		tilemap->visible = calloc( num_tiles,1 );
		tilemap->dirty_vram = malloc( num_tiles );
		tilemap->dirty_pixels = malloc( num_tiles );
		tilemap->rowscroll = calloc(tilemap->cached_height,sizeof(int));
		tilemap->colscroll = calloc(tilemap->cached_width,sizeof(int));
		tilemap->priority_row = malloc( sizeof(UINT8 *)*num_rows );
		tilemap->pixmap = create_tmpbitmap( tilemap->cached_width, tilemap->cached_height, Machine->scrbitmap->depth );
		tilemap->foreground = mask_create( tilemap );
		tilemap->background = (type & TILEMAP_SPLIT)?mask_create( tilemap ):NULL;
		if( tilemap->cached_tile_info &&
			tilemap->priority && tilemap->visible &&
			tilemap->dirty_vram && tilemap->dirty_pixels &&
			tilemap->rowscroll && tilemap->colscroll &&
			tilemap->priority_row &&
			tilemap->pixmap && tilemap->foreground &&
			((type&TILEMAP_SPLIT)==0 || tilemap->background) &&
			(mappings_create( tilemap )==0)
		){
			UINT32 row;
			for( row=0; row<num_rows; row++ ){
				tilemap->priority_row[row] = tilemap->priority+num_cols*row;
			}
			install_draw_handlers( tilemap );
			mappings_update( tilemap );
			tilemap_set_clip( tilemap, &Machine->visible_area );
			memset( tilemap->dirty_vram, 1, num_tiles );
			memset( tilemap->dirty_pixels, 1, num_tiles );
			tilemap->pixmap_line_offset = tilemap->pixmap->line[1] - tilemap->pixmap->line[0];
			tilemap->next = first_tilemap;
			first_tilemap = tilemap;
			return tilemap;
		}
		tilemap_dispose( tilemap );
	}
	return 0;
}

void tilemap_dispose( struct tilemap *tilemap ){
	if( tilemap==first_tilemap ){
		first_tilemap = tilemap->next;
	}
	else {
		struct tilemap *prev = first_tilemap;
		while( prev->next != tilemap ) prev = prev->next;
		prev->next =tilemap->next;
	}

	free( tilemap->cached_tile_info );
	free( tilemap->priority );
	free( tilemap->visible );
	free( tilemap->dirty_vram );
	free( tilemap->dirty_pixels );
	free( tilemap->rowscroll );
	free( tilemap->colscroll );
	free( tilemap->priority_row );
	osd_free_bitmap( tilemap->pixmap );
	mask_dispose( tilemap->foreground );
	mask_dispose( tilemap->background );
	mappings_dispose( tilemap );
	free( tilemap );
}

/***********************************************************************************/

static void unregister_pens( struct cached_tile_info *cached_tile_info, int num_pens ){
	if( palette_used_colors ){
		const UINT32 *pal_data = cached_tile_info->pal_data;
		if( pal_data ){
			UINT32 pen_usage = cached_tile_info->pen_usage;
			if( pen_usage ){
				palette_decrease_usage_count(
					pal_data-Machine->remapped_colortable,
					pen_usage,
					PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
			}
			else {
				palette_decrease_usage_countx(
					pal_data-Machine->remapped_colortable,
					num_pens,
					cached_tile_info->pen_data,
					PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
			}
			cached_tile_info->pal_data = NULL;
		}
	}
}

static void register_pens( struct cached_tile_info *cached_tile_info, int num_pens ){
	if (palette_used_colors){
		UINT32 pen_usage = cached_tile_info->pen_usage;
		if( pen_usage ){
			palette_increase_usage_count(
				cached_tile_info->pal_data-Machine->remapped_colortable,
				pen_usage,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
		else {
			palette_increase_usage_countx(
				cached_tile_info->pal_data-Machine->remapped_colortable,
				num_pens,
				cached_tile_info->pen_data,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
	}
}

/***********************************************************************************/

void tilemap_set_enable( struct tilemap *tilemap, int enable ){
	tilemap->enable = enable?1:0;
}

void tilemap_set_flip( struct tilemap *tilemap, int attributes ){
	if( tilemap==ALL_TILEMAPS ){
		tilemap = first_tilemap;
		while( tilemap ){
			tilemap_set_flip( tilemap, attributes );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->attributes!=attributes ){
		tilemap->attributes = attributes;
		tilemap->orientation = Machine->orientation;
		if( attributes&TILEMAP_FLIPY ){
			tilemap->orientation ^= ORIENTATION_FLIP_Y;
			tilemap->scrolly_delta = tilemap->dy_if_flipped;
		}
		else {
			tilemap->scrolly_delta = tilemap->dy;
		}
		if( attributes&TILEMAP_FLIPX ){
			tilemap->orientation ^= ORIENTATION_FLIP_X;
			tilemap->scrollx_delta = tilemap->dx_if_flipped;
		}
		else {
			tilemap->scrollx_delta = tilemap->dx;
		}

		mappings_update( tilemap );
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip ){
	int left,top,right,bottom;
	if( clip ){
		left = clip->min_x;
		top = clip->min_y;
		right = clip->max_x+1;
		bottom = clip->max_y+1;
		if( tilemap->orientation & ORIENTATION_SWAP_XY ){
			SWAP(left,top)
			SWAP(right,bottom)
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X ){
			SWAP(left,right)
			left = screen_width-left;
			right = screen_width-right;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y ){
			SWAP(top,bottom)
			top = screen_height-top;
			bottom = screen_height-bottom;
		}
	}
	else {
		left = 0;
		top = 0;
		right = tilemap->cached_width;
		bottom = tilemap->cached_height;
	}
	tilemap->clip_left = left;
	tilemap->clip_right = right;
	tilemap->clip_top = top;
	tilemap->clip_bottom = bottom;
//	logerror("clip: %d,%d,%d,%d\n", left,top,right,bottom );
}

/***********************************************************************************/

void tilemap_set_scroll_cols( struct tilemap *tilemap, int n ){
	if( tilemap->orientation & ORIENTATION_SWAP_XY ){
		if (tilemap->scroll_rows != n){
			tilemap->scroll_rows = n;
		}
	}
	else {
		if (tilemap->scroll_cols != n){
			tilemap->scroll_cols = n;
		}
	}
}

void tilemap_set_scroll_rows( struct tilemap *tilemap, int n ){
	if( tilemap->orientation & ORIENTATION_SWAP_XY ){
		if (tilemap->scroll_cols != n){
			tilemap->scroll_cols = n;
		}
	}
	else {
		if (tilemap->scroll_rows != n){
			tilemap->scroll_rows = n;
		}
	}
}

/***********************************************************************************/

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int memory_offset ){
	if( memory_offset<tilemap->max_memory_offset ){
		int cached_indx = tilemap->memory_offset_to_cached_indx[memory_offset];
		if( cached_indx>=0 ){
			tilemap->dirty_vram[cached_indx] = 1;
		}
	}
}

void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap ){
	if( tilemap==ALL_TILEMAPS ){
		tilemap = first_tilemap;
		while( tilemap ){
			tilemap_mark_all_tiles_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else {
		memset( tilemap->dirty_vram, 1, tilemap->num_tiles );
	}
}

static void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap ){
	if( tilemap==ALL_TILEMAPS ){
		tilemap = first_tilemap;
		while( tilemap ){
			tilemap_mark_all_pixels_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else {
		/* invalidate all offscreen tiles */
		UINT32 cached_tile_indx;
		UINT32 num_pens = tilemap->cached_tile_width*tilemap->cached_tile_height;
		for( cached_tile_indx=0; cached_tile_indx<tilemap->num_tiles; cached_tile_indx++ ){
			if( !tilemap->visible[cached_tile_indx] ){
				unregister_pens( &tilemap->cached_tile_info[cached_tile_indx], num_pens );
				tilemap->dirty_vram[cached_tile_indx] = 1;
			}
		}
		memset( tilemap->dirty_pixels, 1, tilemap->num_tiles );
	}
}

void tilemap_dirty_palette( const UINT8 *dirty_pens ) {
	UINT32 *color_base = Machine->remapped_colortable;
	struct tilemap *tilemap = first_tilemap;
	while( tilemap )
	{
		if( !tilemap->tile_dirty_map)
			tilemap_mark_all_pixels_dirty( tilemap );
		else
		{
			UINT8 *dirty_map = tilemap->tile_dirty_map;
			int i, j, pen, row, col;
			int step = 1 << tilemap->tile_granularity;
			int count = 1 << tilemap->tile_depth;
			int limit = Machine->drv->total_colors - count;
//			UINT32 num_pens = tilemap->cached_tile_width*tilemap->cached_tile_height;
			pen = 0;
			for( i=0; i<limit; i+=step )
			{
				for( j=0; j<count; j++ )
					if( dirty_pens[i+j] )
					{
						dirty_map[pen++] = 1;
						goto next;
					}
				dirty_map[pen++] = 0;
			next:
				;
			}

			i = 0;
			for( row=0; row<tilemap->num_cached_rows; row++ ){
				for( col=0; col<tilemap->num_cached_cols; col++ ){
					if (!tilemap->dirty_vram[i] && !tilemap->dirty_pixels[i])
					{
						struct cached_tile_info *cached_tile = tilemap->cached_tile_info+i;
						j = (cached_tile->pal_data - color_base) >> tilemap->tile_granularity;
						if( dirty_map[j] )
						{
							if( tilemap->visible[i] )
								draw_tile( tilemap, i, col, row );
							else
								tilemap->dirty_pixels[i] = 1;
						}
					}
					i++;
				}
			}
		}

		tilemap = tilemap->next;
	}
}

/***********************************************************************************/

static int draw_bitmask(
		struct osd_bitmap *mask,
		UINT32 col, UINT32 row,
		UINT32 tile_width, UINT32 tile_height,
		const UINT8 *maskdata,
		UINT32 flags )
{
	int is_opaque = 1, is_transparent = 1;
	int x,sx = tile_width*col;
	int sy,y1,y2,dy;

	if( maskdata==TILEMAP_BITMASK_TRANSPARENT ) return TILE_TRANSPARENT;
	if( maskdata==TILEMAP_BITMASK_OPAQUE) return TILE_OPAQUE;

	if( flags&TILE_FLIPY ){
		y1 = tile_height*row+tile_height-1;
		y2 = y1-tile_height;
 		dy = -1;
 	}
 	else {
		y1 = tile_height*row;
		y2 = y1+tile_height;
 		dy = 1;
 	}

	if( flags&TILE_FLIPX ){
		tile_width--;
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=tile_width/8; x>=0; x-- ){
				UINT8 data = flip_bit_table[*maskdata++];
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=0; x<tile_width/8; x++ ){
				UINT8 data = *maskdata++;
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}

	if( is_transparent ) return TILE_TRANSPARENT;
	if( is_opaque ) return TILE_OPAQUE;
	return TILE_MASKED;
}

static int draw_color_mask(
	struct osd_bitmap *mask,
	UINT32 col, UINT32 row,
	UINT32 tile_width, UINT32 tile_height,
	const UINT8 *pendata,
	const UINT16 *clut,
	int transparent_color,
	UINT32 flags )
{
	int is_opaque = 1, is_transparent = 1;

	int x,bit,sx = tile_width*col;
	int sy,y1,y2,dy;

	if( flags&TILE_FLIPY ){
		y1 = tile_height*row+tile_height-1;
		y2 = y1-tile_height;
 		dy = -1;
 	}
 	else {
		y1 = tile_height*row;
		y2 = y1+tile_height;
 		dy = 1;
 	}

	if( flags&TILE_FLIPX ){
		tile_width--;
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=tile_width/8; x>=0; x-- ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = data>>1;
					if( clut[pen]!=transparent_color ) data |=0x80;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=0; x<tile_width/8; x++ ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = data<<1;
					if( clut[pen]!=transparent_color ) data |=0x01;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}
	if( is_transparent ) return TILE_TRANSPARENT;
	if( is_opaque ) return TILE_OPAQUE;
	return TILE_MASKED;
}

static int draw_pen_mask(
	struct osd_bitmap *mask,
	UINT32 col, UINT32 row,
	UINT32 tile_width, UINT32 tile_height,
	const UINT8 *pendata,
	int transparent_pen,
	UINT32 flags )
{
	int is_opaque = 1, is_transparent = 1;

	int x,bit,sx = tile_width*col;
	int sy,y1,y2,dy;

	if( flags&TILE_FLIPY ){
		y1 = tile_height*row+tile_height-1;
		y2 = y1-tile_height;
 		dy = -1;
 	}
 	else {
		y1 = tile_height*row;
		y2 = y1+tile_height;
 		dy = 1;
 	}

	if( flags&TILE_FLIPX ){
		tile_width--;
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=tile_width/8; x>=0; x-- ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = data>>1;
					if( pen!=transparent_pen ) data |=0x80;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=0; x<tile_width/8; x++ ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = data<<1;
					if( pen!=transparent_pen ) data |=0x01;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				mask_dest[x] = data;
			}
		}
	}
	if( is_transparent ) return TILE_TRANSPARENT;
	if( is_opaque ) return TILE_OPAQUE;
	return TILE_MASKED;
}

static void draw_mask(
	struct osd_bitmap *mask,
	UINT32 col, UINT32 row,
	UINT32 tile_width, UINT32 tile_height,
	const UINT8 *pendata,
	UINT32 transmask,
	UINT32 flags )
{
	int x,bit,sx = tile_width*col;
	int sy,y1,y2,dy;

	if( flags&TILE_FLIPY ){
		y1 = tile_height*row+tile_height-1;
		y2 = y1-tile_height;
 		dy = -1;
 	}
 	else {
		y1 = tile_height*row;
		y2 = y1+tile_height;
 		dy = 1;
 	}

	if( flags&TILE_FLIPX ){
		tile_width--;
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=tile_width/8; x>=0; x-- ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = data>>1;
					if( !((1<<pen)&transmask) ) data |= 0x80;
				}
				mask_dest[x] = data;
			}
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
			UINT8 *mask_dest  = mask->line[sy]+sx/8;
			for( x=0; x<tile_width/8; x++ ){
				UINT32 data = 0;
				for( bit=0; bit<8; bit++ ){
					UINT32 pen = *pendata++;
					data = (data<<1);
					if( !((1<<pen)&transmask) ) data |= 0x01;
				}
				mask_dest[x] = data;
			}
		}
	}
}

static void render_mask( struct tilemap *tilemap, UINT32 cached_indx ){
	const struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
	UINT32 col = cached_indx%tilemap->num_cached_cols;
	UINT32 row = cached_indx/tilemap->num_cached_cols;
	UINT32 type = tilemap->type;

	UINT32 transparent_pen = tilemap->transparent_pen;
	UINT32 *transmask = tilemap->transmask;
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;

	UINT32 pen_usage = cached_tile_info->pen_usage;
	const UINT8 *pen_data = cached_tile_info->pen_data;
	UINT32 flags = cached_tile_info->flags;

	if( type & TILEMAP_BITMASK ){
		tilemap->foreground->data_row[row][col] =
			draw_bitmask( tilemap->foreground->bitmask,col, row,
				tile_width, tile_height,tile_info.mask_data, flags );
	}
	else if( type & TILEMAP_SPLIT ){
		UINT32 pen_mask = (transparent_pen<0)?0:(1<<transparent_pen);
		if( flags&TILE_IGNORE_TRANSPARENCY ){
			tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
			tilemap->background->data_row[row][col] = TILE_OPAQUE;
		}
		else if( pen_mask == pen_usage ){ /* totally transparent */
			tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
			tilemap->background->data_row[row][col] = TILE_TRANSPARENT;
		}
		else {
			UINT32 fg_transmask = transmask[(flags>>2)&3];
			UINT32 bg_transmask = (~fg_transmask)|pen_mask;
			if( (pen_usage & fg_transmask)==0 ){ /* foreground totally opaque */
				tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
				tilemap->background->data_row[row][col] = TILE_TRANSPARENT;
			}
			else if( (pen_usage & bg_transmask)==0 ){ /* background totally opaque */
				tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
				tilemap->background->data_row[row][col] = TILE_OPAQUE;
			}
			else if( (pen_usage & ~bg_transmask)==0 ){ /* background transparent */
				draw_mask( tilemap->foreground->bitmask,
					col, row, tile_width, tile_height,
					pen_data, fg_transmask, flags );
				tilemap->foreground->data_row[row][col] = TILE_MASKED;
				tilemap->background->data_row[row][col] = TILE_TRANSPARENT;
			}
			else if( (pen_usage & ~fg_transmask)==0 ){ /* foreground transparent */
				draw_mask( tilemap->background->bitmask,
					col, row, tile_width, tile_height,
					pen_data, bg_transmask, flags );
				tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
				tilemap->background->data_row[row][col] = TILE_MASKED;
			}
			else { /* split tile - opacity in both foreground and background */
				draw_mask( tilemap->foreground->bitmask,
					col, row, tile_width, tile_height,
					pen_data, fg_transmask, flags );
				draw_mask( tilemap->background->bitmask,
					col, row, tile_width, tile_height,
					pen_data, bg_transmask, flags );
				tilemap->foreground->data_row[row][col] = TILE_MASKED;
				tilemap->background->data_row[row][col] = TILE_MASKED;
			}
		}
	}
	else if( type==TILEMAP_TRANSPARENT ){
		if( pen_usage ){
			UINT32 fg_transmask = 1 << transparent_pen;
		 	if( flags&TILE_IGNORE_TRANSPARENCY ) fg_transmask = 0;
			if( pen_usage == fg_transmask ){
				tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
			}
			else if( pen_usage & fg_transmask ){
				draw_mask( tilemap->foreground->bitmask,
					col, row, tile_width, tile_height,
					pen_data, fg_transmask, flags );
				tilemap->foreground->data_row[row][col] = TILE_MASKED;
			}
			else {
				tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
			}
		}
		else {
			tilemap->foreground->data_row[row][col] =
				draw_pen_mask(
					tilemap->foreground->bitmask,
					col, row, tile_width, tile_height,
					pen_data,
					transparent_pen,
					flags
				);
		}
	}
	else if( type==TILEMAP_TRANSPARENT_COLOR ){
		tilemap->foreground->data_row[row][col] =
			draw_color_mask(
				tilemap->foreground->bitmask,
				col, row, tile_width, tile_height,
				pen_data,
				Machine->game_colortable +
					(cached_tile_info->pal_data - Machine->remapped_colortable),
				transparent_pen,
				flags
			);
	}
	else {
		tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
	}
}

static void update_tile_info( struct tilemap *tilemap ){
	int *logical_flip_to_cached_flip = tilemap->logical_flip_to_cached_flip;
	UINT32 num_pens = tilemap->cached_tile_width*tilemap->cached_tile_height;
	UINT32 num_tiles = tilemap->num_tiles;
	UINT32 cached_indx;
	UINT8 *visible = tilemap->visible;
	UINT8 *dirty_vram = tilemap->dirty_vram;
	UINT8 *dirty_pixels = tilemap->dirty_pixels;
	tile_info.flags = 0;
	tile_info.priority = 0;
	for( cached_indx=0; cached_indx<num_tiles; cached_indx++ ){
		if( visible[cached_indx] && dirty_vram[cached_indx] ){
			struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
			UINT32 memory_offset = tilemap->cached_indx_to_memory_offset[cached_indx];
			unregister_pens( cached_tile_info, num_pens );
			tilemap->tile_get_info( memory_offset );
			{
				UINT32 flags = tile_info.flags;
				cached_tile_info->flags = (flags&0xfc)|logical_flip_to_cached_flip[flags&0x3];
			}
			cached_tile_info->pen_usage = tile_info.pen_usage;
			cached_tile_info->pen_data = tile_info.pen_data;
			cached_tile_info->pal_data = tile_info.pal_data;
			tilemap->priority[cached_indx] = tile_info.priority;
			register_pens( cached_tile_info, num_pens );
			dirty_pixels[cached_indx] = 1;
			dirty_vram[cached_indx] = 0;
			render_mask( tilemap, cached_indx );
		}
	}
}

static void update_visible( struct tilemap *tilemap ){
	// temporary hack
	memset( tilemap->visible, 1, tilemap->num_tiles );

#if 0
	int yscroll = scrolly[0];
	int row0, y0;

	int xscroll = scrollx[0];
	int col0, x0;

	if( yscroll>=0 ){
		row0 = yscroll/tile_height;
		y0 = -(yscroll%tile_height);
	}
	else {
		yscroll = tile_height-1-yscroll;
		row0 = num_rows - yscroll/tile_height;
		y0 = (yscroll+1)%tile_height;
		if( y0 ) y0 = y0-tile_height;
	}

	if( xscroll>=0 ){
		col0 = xscroll/tile_width;
		x0 = -(xscroll%tile_width);
	}
	else {
		xscroll = tile_width-1-xscroll;
		col0 = num_cols - xscroll/tile_width;
		x0 = (xscroll+1)%tile_width;
		if( x0 ) x0 = x0-tile_width;
	}

	{
		int ypos = y0;
		int row = row0;
		while( ypos<screen_height ){
			int xpos = x0;
			int col = col0;
			while( xpos<screen_width ){
				process_visible_tile( col, row );
				col++;
				if( col>=num_cols ) col = 0;
				xpos += tile_width;
			}
			row++;
			if( row>=num_rows ) row = 0;
			ypos += tile_height;
		}
	}
#endif
}

void tilemap_update( struct tilemap *tilemap ){
profiler_mark(PROFILER_TILEMAP_UPDATE);
	if( tilemap==ALL_TILEMAPS ){
		tilemap = first_tilemap;
		while( tilemap ){
			tilemap_update( tilemap );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->enable ){
		tilemap->bNeedRender = 1;
		update_visible( tilemap );
		update_tile_info( tilemap );
	}
profiler_mark(PROFILER_END);
}

/***********************************************************************************/

void tilemap_set_scrolldx( struct tilemap *tilemap, int dx, int dx_if_flipped ){
	tilemap->dx = dx;
	tilemap->dx_if_flipped = dx_if_flipped;
	tilemap->scrollx_delta = ( tilemap->attributes & TILEMAP_FLIPX )?dx_if_flipped:dx;
}

void tilemap_set_scrolldy( struct tilemap *tilemap, int dy, int dy_if_flipped ){
	tilemap->dy = dy;
	tilemap->dy_if_flipped = dy_if_flipped;
	tilemap->scrolly_delta = ( tilemap->attributes & TILEMAP_FLIPY )?dy_if_flipped:dy;
}

void tilemap_set_scrollx( struct tilemap *tilemap, int which, int value ){
	value = tilemap->scrollx_delta-value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY ){
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value ){
			tilemap->colscroll[which] = value;
		}
	}
	else {
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value ){
			tilemap->rowscroll[which] = value;
		}
	}
}
void tilemap_set_scrolly( struct tilemap *tilemap, int which, int value ){
	value = tilemap->scrolly_delta - value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY ){
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value ){
			tilemap->rowscroll[which] = value;
		}
	}
	else {
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value ){
			tilemap->colscroll[which] = value;
		}
	}
}
/***********************************************************************************/

void tilemap_draw( struct osd_bitmap *dest, struct tilemap *tilemap, UINT32 flags, UINT32 priority ){
	int xpos,ypos;
profiler_mark(PROFILER_TILEMAP_DRAW);
	tmap_render( tilemap );

	if( tilemap->enable ){
		void (*draw)( int, int );

		int rows = tilemap->scroll_rows;
		const int *rowscroll = tilemap->rowscroll;
		int cols = tilemap->scroll_cols;
		const int *colscroll = tilemap->colscroll;

		int left = tilemap->clip_left;
		int right = tilemap->clip_right;
		int top = tilemap->clip_top;
		int bottom = tilemap->clip_bottom;

		int tile_height = tilemap->cached_tile_height;

		blit.screen = dest;
		blit.dest_line_offset = dest->line[1] - dest->line[0];

		blit.pixmap = tilemap->pixmap;
		blit.source_line_offset = tilemap->pixmap_line_offset;

		if( tilemap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY) ){
			draw = tilemap->draw_opaque;
		}
		else {
			if( flags & TILEMAP_ALPHA )
				draw = tilemap->draw_alpha;
			else
				draw = tilemap->draw;

			if( flags&TILEMAP_BACK ){
				blit.bitmask = tilemap->background->bitmask;
				blit.mask_line_offset = tilemap->background->line_offset;
				blit.mask_data_row = tilemap->background->data_row;
			}
			else {
				blit.bitmask = tilemap->foreground->bitmask;
				blit.mask_line_offset = tilemap->foreground->line_offset;
				blit.mask_data_row = tilemap->foreground->data_row;
			}

			blit.mask_row_offset = tile_height*blit.mask_line_offset;
		}

		if( dest->depth==16 || dest->depth==15 ){
			blit.dest_line_offset /= 2;
			blit.source_line_offset /= 2;
		} else if( dest->depth==32 ){
			blit.dest_line_offset /= 4;
			blit.source_line_offset /= 4;
		}


		blit.source_row_offset = tile_height*blit.source_line_offset;
		blit.dest_row_offset = tile_height*blit.dest_line_offset;

		blit.priority_data_row = tilemap->priority_row;
		blit.source_width = tilemap->cached_width;
		blit.source_height = tilemap->cached_height;
		blit.tile_priority = flags&0xf;
		blit.tilemap_priority_code = priority;

		if( rows == 1 && cols == 1 ){ /* XY scrolling playfield */
			int scrollx = rowscroll[0];
			int scrolly = colscroll[0];

			if( scrollx < 0 ){
				scrollx = blit.source_width - (-scrollx) % blit.source_width;
			}
			else {
				scrollx = scrollx % blit.source_width;
			}

			if( scrolly < 0 ){
				scrolly = blit.source_height - (-scrolly) % blit.source_height;
			}
			else {
				scrolly = scrolly % blit.source_height;
			}

	 		blit.clip_left = left;
	 		blit.clip_top = top;
	 		blit.clip_right = right;
	 		blit.clip_bottom = bottom;

			for(
				ypos = scrolly - blit.source_height;
				ypos < blit.clip_bottom;
				ypos += blit.source_height
			){
				for(
					xpos = scrollx - blit.source_width;
					xpos < blit.clip_right;
					xpos += blit.source_width
				){
					draw( xpos,ypos );
				}
			}
		}
		else if( rows == 1 ){ /* scrolling columns + horizontal scroll */
			int col = 0;
			int colwidth = blit.source_width / cols;
			int scrollx = rowscroll[0];

			if( scrollx < 0 ){
				scrollx = blit.source_width - (-scrollx) % blit.source_width;
			}
			else {
				scrollx = scrollx % blit.source_width;
			}

			blit.clip_top = top;
			blit.clip_bottom = bottom;

			while( col < cols ){
				int cons = 1;
				int scrolly = colscroll[col];

	 			/* count consecutive columns scrolled by the same amount */
				if( scrolly != TILE_LINE_DISABLED ){
					while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;

					if( scrolly < 0 ){
						scrolly = blit.source_height - (-scrolly) % blit.source_height;
					}
					else {
						scrolly %= blit.source_height;
					}

					blit.clip_left = col * colwidth + scrollx;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - blit.source_height;
						ypos < blit.clip_bottom;
						ypos += blit.source_height
					){
						draw( scrollx,ypos );
					}

					blit.clip_left = col * colwidth + scrollx - blit.source_width;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx - blit.source_width;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - blit.source_height;
						ypos < blit.clip_bottom;
						ypos += blit.source_height
					){
						draw( scrollx - blit.source_width,ypos );
					}
				}
				col += cons;
			}
		}
		else if( cols == 1 ){ /* scrolling rows + vertical scroll */
			int row = 0;
			int rowheight = blit.source_height / rows;
			int scrolly = colscroll[0];
			if( scrolly < 0 ){
				scrolly = blit.source_height - (-scrolly) % blit.source_height;
			}
			else {
				scrolly = scrolly % blit.source_height;
			}
			blit.clip_left = left;
			blit.clip_right = right;
			while( row < rows ){
				int cons = 1;
				int scrollx = rowscroll[row];
				/* count consecutive rows scrolled by the same amount */
				if( scrollx != TILE_LINE_DISABLED ){
					while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
					if( scrollx < 0){
						scrollx = blit.source_width - (-scrollx) % blit.source_width;
					}
					else {
						scrollx %= blit.source_width;
					}
					blit.clip_top = row * rowheight + scrolly;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - blit.source_width;
						xpos < blit.clip_right;
						xpos += blit.source_width
					){
						draw( xpos,scrolly );
					}
					blit.clip_top = row * rowheight + scrolly - blit.source_height;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly - blit.source_height;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - blit.source_width;
						xpos < blit.clip_right;
						xpos += blit.source_width
					){
						draw( xpos,scrolly - blit.source_height );
					}
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}

/***********************************************************************************/

#else // DECLARE
/*
	The following procedure body is #included several times by
	tilemap.c to implement a suite of tilemap_draw subroutines.

	The constants TILE_WIDTH and TILE_HEIGHT are different in
	each instance of this code, allowing arithmetic shifts to
	be used by the compiler instead of multiplies/divides.

	This routine should be fairly optimal, for C code, though of
	course there is room for improvement.

	It renders pixels one row at a time, skipping over runs of totally
	transparent tiles, and calling custom blitters to handle runs of
	masked/totally opaque tiles.
*/

DECLARE( draw, (int xpos, int ypos),
{
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ){ /* do nothing if totally clipped */
		DATA_TYPE *dest_baseaddr = xpos + (DATA_TYPE *)blit.screen->line[y1];
		DATA_TYPE *dest_next;

		int priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_HEIGHT;
		UINT8 *priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		UINT8 *priority_bitmap_next;

		int priority = blit.tile_priority;
		const DATA_TYPE *source_baseaddr;
		const DATA_TYPE *source_next;
		const UINT8 *mask_baseaddr;
		const UINT8 *mask_next;

		int c1;
		int c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];
		mask_baseaddr = blit.bitmask->line[y1];

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */

		y = y1;
		y_next = TILE_HEIGHT*(y1/TILE_HEIGHT) + TILE_HEIGHT;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*blit.dest_line_offset;
			priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_line_offset;
			source_next = source_baseaddr + dy*blit.source_line_offset;
			mask_next = mask_baseaddr + dy*blit.mask_line_offset;
		}

		for(;;){
			int row = y/TILE_HEIGHT;
			UINT8 *mask_data = blit.mask_data_row[row];
			UINT8 *priority_data = blit.priority_data_row[row];

			int tile_type;
			int prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1;
			int x_end;

			int column;
			for( column=c1; column<=c2; column++ ){
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = mask_data[column];

				if( tile_type!=prev_tile_type ){
					x_end = column*TILE_WIDTH;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT ){
						if( prev_tile_type == TILE_MASKED ){
							int count = (x_end+7)/8 - x_start/8;
							const UINT8 *mask0 = mask_baseaddr + x_start/8;
							const DATA_TYPE *source0 = source_baseaddr + (x_start&0xfff8);
							DATA_TYPE *dest0 = dest_baseaddr + (x_start&0xfff8);
							UINT8 *pmap0 = priority_bitmap_baseaddr + (x_start&0xfff8);
							int i = y;
							for(;;){
								memcpybitmask( dest0, source0, mask0, count );
								memsetbitmask8( pmap0, tilemap_priority_code, mask0, count );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								mask0 += blit.mask_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
						else { /* TILE_OPAQUE */
							int num_pixels = x_end - x_start;
							DATA_TYPE *dest0 = dest_baseaddr+x_start;
							const DATA_TYPE *source0 = source_baseaddr+x_start;
							UINT8 *pmap0 = priority_bitmap_baseaddr + x_start;
							int i = y;
							for(;;){
								memcpy( dest0, source0, num_pixels*sizeof(DATA_TYPE) );
								memset( pmap0, tilemap_priority_code, num_pixels );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;

			y = y_next;
			y_next += TILE_HEIGHT;

			if( y_next>=y2 ){
				y_next = y2;
			}
			else {
				dest_next += blit.dest_row_offset;
				priority_bitmap_next += priority_bitmap_row_offset;
				source_next += blit.source_row_offset;
				mask_next += blit.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})

DECLARE( draw_opaque, (int xpos, int ypos),
{
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;
	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ){ /* do nothing if totally clipped */
		UINT8 *priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		int priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_HEIGHT;

		int priority = blit.tile_priority;
		DATA_TYPE *dest_baseaddr = xpos + (DATA_TYPE *)blit.screen->line[y1];
		DATA_TYPE *dest_next;
		const DATA_TYPE *source_baseaddr;
		const DATA_TYPE *source_next;

		int c1;
		int c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */

		y = y1;
		y_next = TILE_HEIGHT*(y1/TILE_HEIGHT) + TILE_HEIGHT;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*blit.dest_line_offset;
			source_next = source_baseaddr + dy*blit.source_line_offset;
		}

		for(;;){
			int row = y/TILE_HEIGHT;
			UINT8 *priority_data = blit.priority_data_row[row];

			int tile_type;
			int prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1;
			int x_end;

			int column;
			for( column=c1; column<=c2; column++ ){
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = TILE_OPAQUE;

				if( tile_type!=prev_tile_type ){
					x_end = column*TILE_WIDTH;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT ){
						/* TILE_OPAQUE */
						int num_pixels = x_end - x_start;
						DATA_TYPE *dest0 = dest_baseaddr+x_start;
						UINT8 *pmap0 = priority_bitmap_baseaddr+x_start;
						const DATA_TYPE *source0 = source_baseaddr+x_start;
						int i = y;
						for(;;){
							memcpy( dest0, source0, num_pixels*sizeof(DATA_TYPE) );
							memset( pmap0, tilemap_priority_code, num_pixels );
							if( ++i == y_next ) break;

							dest0 += blit.dest_line_offset;
							pmap0 += priority_bitmap_line_offset;
							source0 += blit.source_line_offset;
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr += priority_bitmap_row_offset;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;

			y = y_next;
			y_next += TILE_HEIGHT;

			if( y_next>=y2 ){
				y_next = y2;
			}
			else {
				dest_next += blit.dest_row_offset;
				source_next += blit.source_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})

#if DEPTH >= 16
DECLARE( draw_alpha, (int xpos, int ypos),
{
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ){ /* do nothing if totally clipped */
		DATA_TYPE *dest_baseaddr = xpos + (DATA_TYPE *)blit.screen->line[y1];
		DATA_TYPE *dest_next;

		int priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_HEIGHT;
		UINT8 *priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		UINT8 *priority_bitmap_next;

		int priority = blit.tile_priority;
		const DATA_TYPE *source_baseaddr;
		const DATA_TYPE *source_next;
		const UINT8 *mask_baseaddr;
		const UINT8 *mask_next;

		int c1;
		int c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];
		mask_baseaddr = blit.bitmask->line[y1];

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */

		y = y1;
		y_next = TILE_HEIGHT*(y1/TILE_HEIGHT) + TILE_HEIGHT;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*blit.dest_line_offset;
			priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_line_offset;
			source_next = source_baseaddr + dy*blit.source_line_offset;
			mask_next = mask_baseaddr + dy*blit.mask_line_offset;
		}

		for(;;){
			int row = y/TILE_HEIGHT;
			UINT8 *mask_data = blit.mask_data_row[row];
			UINT8 *priority_data = blit.priority_data_row[row];

			int tile_type;
			int prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1;
			int x_end;

			int column;
			for( column=c1; column<=c2; column++ ){
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = mask_data[column];

				if( tile_type!=prev_tile_type ){
					x_end = column*TILE_WIDTH;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT ){
						if( prev_tile_type == TILE_MASKED ){
							int count = (x_end+7)/8 - x_start/8;
							const UINT8 *mask0 = mask_baseaddr + x_start/8;
							const DATA_TYPE *source0 = source_baseaddr + (x_start&0xfff8);
							DATA_TYPE *dest0 = dest_baseaddr + (x_start&0xfff8);
							UINT8 *pmap0 = priority_bitmap_baseaddr + (x_start&0xfff8);
							int i = y;
							for(;;){
								blendbitmask( dest0, source0, mask0, count );
								memsetbitmask8( pmap0, tilemap_priority_code, mask0, count );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								mask0 += blit.mask_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
						else { /* TILE_OPAQUE */
							int num_pixels = x_end - x_start;
							DATA_TYPE *dest0 = dest_baseaddr+x_start;
							const DATA_TYPE *source0 = source_baseaddr+x_start;
							UINT8 *pmap0 = priority_bitmap_baseaddr + x_start;
							int i = y;
							for(;;){
								blend( dest0, source0, num_pixels );
								memset( pmap0, tilemap_priority_code, num_pixels );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;

			y = y_next;
			y_next += TILE_HEIGHT;

			if( y_next>=y2 ){
				y_next = y2;
			}
			else {
				dest_next += blit.dest_row_offset;
				priority_bitmap_next += priority_bitmap_row_offset;
				source_next += blit.source_row_offset;
				mask_next += blit.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})
#endif

#undef TILE_WIDTH
#undef TILE_HEIGHT
#undef DATA_TYPE
#undef memcpybitmask
#undef DECLARE

#endif /* DECLARE */

#else

/*******************************************************************************************/
/*-----------------------------------------------------------------------------------------*/
/*******************************************************************************************/

/* WIP code for new tilemap implementation follows */

#ifndef DECLARE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "tilemap.h"
#include "state.h"

enum
{
	WHOLLY_TRANSPARENT,
	PARTIALLY_TRANSPARENT,
	WHOLLY_OPAQUE
};

#define SWAP(X,Y) { temp=X; X=Y; Y=temp; }

#define NUM_BUCKETS (0x1000)

struct osd_bitmap *priority_bitmap; /* 8bpp, 1 to 1 with screen bitmap */

static UINT8 flip_bit_table[0x100]; /* flip table for 8 bit value; 0x80 <-> 0x01, 0x40<->0x02, etc. */
static struct tilemap *first_tilemap; /* resource tracking */
static int screen_width, screen_height;
struct tile_info tile_info;

struct draw_param
{
	int mask_offset;
	int tile_priority;
	int tilemap_priority_code;
	int clip_left, clip_top, clip_right, clip_bottom;
	struct osd_bitmap *screen;
} blit;

/***********************************************************************************/

/* DrawClippedTranspTile8,DrawClippedTranspTile16,DrawClippedTranspTile32,
 * DrawClippedOpaqueTile8,DrawClippedOpaqueTile16,DrawClippedOpaqueTile32
 *
 *	These routines draw a single tile clipping it to the screen bounds defined in 'blit'
 *	They have room for optimizations.
 */

static void DrawClippedOpaqueTile8(
		const UINT8 *pColorData,
		int sx, int sy, int tile_width, int tile_height )
{
	int skip = 0;
	int priority_code = blit.tilemap_priority_code;
	int count;
	UINT8 *pDest;
	int clip;
	UINT8 *pPri;

	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		pColorData += tile_width*clip;
		tile_height -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		skip += clip;
		tile_width -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		sx += clip;
		skip += clip;
		pColorData += clip;
		tile_width -= clip;
	}

	while( tile_height-- )
	{
		pDest = sx + (UINT8 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		while( count-- )
		{
			*pDest++ = *pColorData++;
			*pPri++ |= priority_code;
		}
		pColorData += skip;
	}
}

static void DrawClippedOpaqueTile16(
		const UINT16 *pColorData,
		int sx, int sy, int tile_width, int tile_height )
{
	int skip = 0;
	int priority_code = blit.tilemap_priority_code;
	int count;
	UINT16 *pDest;
	int clip;
	UINT8 *pPri;

	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		pColorData += tile_width*clip;
		tile_height -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		skip += clip;
		tile_width -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		sx += clip;
		skip += clip;
		pColorData += clip;
		tile_width -= clip;
	}
	while( tile_height-- )
	{
		pDest = sx+(UINT16 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		while( count-- )
		{
			*pDest++ = *pColorData++;
			*pPri++ |= priority_code;
		}
		pColorData += skip;
	}
}

static void DrawClippedOpaqueTile32(
		const UINT32 *pColorData,
		int sx, int sy, int tile_width, int tile_height )
{
	int skip = 0;
	int priority_code = blit.tilemap_priority_code;
	int count;
	UINT32 *pDest;
	int clip;
	UINT8 *pPri;

	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		pColorData += tile_width*clip;
		tile_height -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		skip += clip;
		tile_width -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		sx += clip;
		skip += clip;
		pColorData += clip;
		tile_width -= clip;
	}
	while( tile_height-- )
	{
		pDest = sx+(UINT32 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		while( count-- )
		{
			*pDest++ = *pColorData++;
			*pPri++ |= priority_code;
		}
		pColorData += skip;
	}
}

/***********************************************************************************/

void DrawClippedTranspTile8(
		const UINT8 *pColorData,
		int sx, int sy, int tile_width, int tile_height,
		const UINT8 *pMask )
{
	int priority_code = blit.tilemap_priority_code;
	int tile_pitch = tile_width;
	unsigned int x = 0;
	unsigned int xpos;
	UINT8 *pDest;
	UINT8 *pPri;
	int clip;
	unsigned int count;

	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		tile_width -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		x += clip;
		tile_width -= clip;
	}
	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		tile_height -= clip;
		clip *= tile_pitch;
		pColorData += clip;
		pMask += clip/8;
	}
	while( tile_height-- )
	{
		pDest = sx + (UINT8 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		xpos = x;
		while( count-- )
		{
			if( pMask[xpos/8]&(0x80>>(xpos&7)) )
			{
				pDest[xpos] = pColorData[xpos];
				pPri[xpos] |= priority_code;
			}
			xpos++;
		}
		pMask += tile_pitch/8;
		pColorData += tile_pitch;
	}
}

void DrawClippedTranspTile16(
		const UINT16 *pColorData,
		int sx, int sy, int tile_width, int tile_height,
		const UINT8 *pMask )
{
	int priority_code = blit.tilemap_priority_code;
	int tile_pitch = tile_width;
	unsigned int x = 0;
	unsigned int xpos;
	UINT16 *pDest;
	UINT8 *pPri;
	int clip;
	unsigned int count;

	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		tile_width -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		x += clip;
		tile_width -= clip;
	}
	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		tile_height -= clip;
		clip *= tile_pitch;
		pColorData += clip;
		pMask += clip/8;
	}
	while( tile_height-- )
	{
		pDest = sx + (UINT16 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		xpos = x;
		while( count-- )
		{
			if( pMask[xpos/8]&(0x80>>(xpos&7)) )
			{
				pDest[xpos] = pColorData[xpos];
				pPri[xpos] |= priority_code;
			}
			xpos++;
		}
		pMask += tile_pitch/8;
		pColorData += tile_pitch;
	}
}

void DrawClippedTranspTile32(
		const UINT32 *pColorData,
		int sx, int sy, int tile_width, int tile_height,
		const UINT8 *pMask )
{
	int priority_code = blit.tilemap_priority_code;
	int tile_pitch = tile_width;
	unsigned int x = 0;
	unsigned int xpos;
	UINT32 *pDest;
	UINT8 *pPri;
	int clip;
	unsigned int count;

	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		tile_width -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		x += clip;
		tile_width -= clip;
	}
	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		tile_height -= clip;
		clip *= tile_pitch;
		pColorData += clip;
		pMask += clip/8;
	}
	while( tile_height-- )
	{
		pDest = sx + (UINT32 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		xpos = x;
		while( count-- )
		{
			if( pMask[xpos/8]&(0x80>>(xpos&7)) )
			{
				pDest[xpos] = pColorData[xpos];
				pPri[xpos] |= priority_code;
			}
			xpos++;
		}
		pMask += tile_pitch/8;
		pColorData += tile_pitch;
	}
}

/***********************************************************************************/

/* DrawClippedAlphaTranspTile16,DrawClippedAlphaTranspTile32,
 * DrawClippedAlphaOpaqueTile16,DrawClippedAlphaOpaqueTile32
 *
 *	These routines blend a single tile clipping it to the screen bounds defined in 'blit'
 *	They have room for optimizations.
 */

static void DrawClippedAlphaOpaqueTile16(
		const UINT16 *pColorData,
		int sx, int sy, int tile_width, int tile_height )
{
	int skip = 0;
	int priority_code = blit.tilemap_priority_code;
	int count;
	UINT16 *pDest;
	int clip;
	UINT8 *pPri;

	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		pColorData += tile_width*clip;
		tile_height -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		skip += clip;
		tile_width -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		sx += clip;
		skip += clip;
		pColorData += clip;
		tile_width -= clip;
	}
	while( tile_height-- )
	{
		pDest = sx+(UINT16 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		while( count-- )
		{
			*pDest = alpha_blend16(*pDest, *pColorData);
			pDest++;
			pColorData++;
			*pPri++ |= priority_code;
		}
		pColorData += skip;
	}
}

static void DrawClippedAlphaOpaqueTile32(
		const UINT32 *pColorData,
		int sx, int sy, int tile_width, int tile_height )
{
	int skip = 0;
	int priority_code = blit.tilemap_priority_code;
	int count;
	UINT32 *pDest;
	int clip;
	UINT8 *pPri;

	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		pColorData += tile_width*clip;
		tile_height -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		skip += clip;
		tile_width -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		sx += clip;
		skip += clip;
		pColorData += clip;
		tile_width -= clip;
	}
	while( tile_height-- )
	{
		pDest = sx+(UINT32 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		while( count-- )
		{
			*pDest = alpha_blend32(*pDest, *pColorData);
			pDest++;
			pColorData++;
			*pPri++ |= priority_code;
		}
		pColorData += skip;
	}
}

/***********************************************************************************/


void DrawClippedAlphaTranspTile16(
		const UINT16 *pColorData,
		int sx, int sy, int tile_width, int tile_height,
		const UINT8 *pMask )
{
	int priority_code = blit.tilemap_priority_code;
	int tile_pitch = tile_width;
	unsigned int x = 0;
	unsigned int xpos;
	UINT16 *pDest;
	UINT8 *pPri;
	int clip;
	unsigned int count;

	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		tile_width -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		x += clip;
		tile_width -= clip;
	}
	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		tile_height -= clip;
		clip *= tile_pitch;
		pColorData += clip;
		pMask += clip/8;
	}
	while( tile_height-- )
	{
		pDest = sx + (UINT16 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		xpos = x;
		while( count-- )
		{
			if( pMask[xpos/8]&(0x80>>(xpos&7)) )
			{
				pDest[xpos] = alpha_blend16(pDest[xpos], pColorData[xpos]);
				pPri[xpos] |= priority_code;
			}
			xpos++;
		}
		pMask += tile_pitch/8;
		pColorData += tile_pitch;
	}
}

void DrawClippedAlphaTranspTile32(
		const UINT32 *pColorData,
		int sx, int sy, int tile_width, int tile_height,
		const UINT8 *pMask )
{
	int priority_code = blit.tilemap_priority_code;
	int tile_pitch = tile_width;
	unsigned int x = 0;
	unsigned int xpos;
	UINT32 *pDest;
	UINT8 *pPri;
	int clip;
	unsigned int count;

	clip = sx+tile_width-blit.clip_right;
	if( clip>0 )
	{ /* clip right */
		tile_width -= clip;
	}
	clip = sy+tile_height-blit.clip_bottom;
	if( clip>0 )
	{ /* clip bottom */
		tile_height -= clip;
	}
	clip = blit.clip_left - sx;
	if( clip>0 )
	{ /* clip left */
		x += clip;
		tile_width -= clip;
	}
	clip = blit.clip_top - sy;
	if( clip>0 )
	{ /* clip top */
		sy += clip;
		tile_height -= clip;
		clip *= tile_pitch;
		pColorData += clip;
		pMask += clip/8;
	}
	while( tile_height-- )
	{
		pDest = sx + (UINT32 *)blit.screen->line[sy++];
		pPri = &priority_bitmap->line[sy][sx];
		count = tile_width;
		xpos = x;
		while( count-- )
		{
			if( pMask[xpos/8]&(0x80>>(xpos&7)) )
			{
				pDest[xpos] = alpha_blend32(pDest[xpos], pColorData[xpos]);
				pPri[xpos] |= priority_code;
			}
			xpos++;
		}
		pMask += tile_pitch/8;
		pColorData += tile_pitch;
	}
}

/***********************************************************************************/

typedef UINT32 *bitarray;

static void *bitarray_create( UINT32 size )
{
	bitarray a;

	size = (size+31)/32; /* round up to groups of 32 bits */
	size = size*4; /* number of bytes to allocate */
	a = malloc( size );
	if( a )
	{
		memset( a, 0x00, size );
	}
	return a;
}

static int bitarray_get( bitarray a, UINT32 bit )
{
	return (a[bit/32]>>(bit&0x1f))&1;
}

static void bitarray_wipe( bitarray a, int size )
{
	size = (size+31)/32;
	size = size*4;
	memset( a, 0x00, size );
}

static void bitarray_iterate(
		bitarray a,
		UINT32 size,
		void *pData,
		void (*callback)( void *, UINT32 ) )
{
	UINT32 i,bit,data,base;
	size = (size+31)/32;
	for( i=0; i<size; i++ )
	{
		data = a[i];
		if( data )
		{
			base = i*32;
			for( bit=0; bit<32; bit++ )
			{
				if( data&1 ) callback( pData, base+bit );
				data >>= 1;
			}
		}
	}
}

static void bitarray_set_range( bitarray a, UINT32 indx, UINT32 count )
{
	while( count-- )
	{
		a[indx/32] |= 1<<(indx&0x1f);
		indx++;
	}
}

/***********************************************************************************/

struct tilemap
{
	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int memory_offset );

	struct cached_tile **cached_tile;
	struct cached_tile **bucket;
	struct cached_tile *freelist;

	/* screen <-> logical tile mapping */
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	INT32 *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	UINT8 logical_flip_to_cached_flip[4];
	UINT32 max_memory_offset;

	UINT32 num_tiles, pens_per_tile, cached_tile_size;
	UINT32 front_offset, back_offset;

	UINT16 tile_depth, tile_granularity;
	UINT8 *tile_dirty_map;

	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;
	UINT32 cached_tile_width, cached_tile_height;
	UINT32 cached_width, cached_height;

	void (*draw_transparent)( struct tilemap *tilemap, INT32 xpos, INT32 ypos );
	void (*draw_opaque)( struct tilemap *tilemap, INT32 xpos, INT32 ypos );
	void (*draw_alpha)( struct tilemap *tilemap, INT32 xpos, INT32 ypos );
	void (*mark_visible)( struct tilemap *tilemap, INT32 xpos, INT32 ypos );

	bitarray visible;

	INT32 dx, dx_if_flipped, scrollx_delta;
	UINT32 scroll_rows, *rowscroll;

	INT32 dy, dy_if_flipped, scrolly_delta;
	UINT32 scroll_cols, *colscroll;

	UINT32 clip_left,clip_right,clip_top,clip_bottom;
	UINT32 orientation;

	UINT32 transmask[4];
	INT32 transparent_pen;

	struct tilemap *next; /* resource tracking */

	UINT8 enable;
	UINT8 attributes;
	UINT8 type;
};

struct cached_tile {
	struct tile_info tile_info;
	UINT32 refcount;
	struct cached_tile *next; /* next tile in hashtable bucket */
	UINT16 bPaintOK; /* boolean flag */
/*
	DATA_TYPE color[pens_per_tile]; // cached colors for tile

	UINT8 front_transparency; // WHOLLY_TRANSPARENT,PARTIALLY_TRANSPARENT,WHOLLY_OPAQUE
	UINT8 back_bitmask[pens_per_tile/8];

	// back transparency is used only for split tiles
	UINT8 back_transparency; // WHOLLY_TRANSPARENT,PARTIALLY_TRANSPARENT,WHOLLY_OPAQUE
	UINT8 front_bitmask[pens_per_tile/8];
*/
};

struct osd_bitmap *tilemap_get_pixmap( struct tilemap * tilemap )
{
	return NULL; //tilemap->pixmap; /* TBA */
}

/* populate tile bitmask for TILEMAP_BITMASK */
static void compute_transparency_bitmask( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT8 data, *dest;
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	int flip = tilemap->logical_flip_to_cached_flip[cached_tile->tile_info.flags&0x3];
	UINT8 *dest_baseaddr = tilemap->front_offset + (UINT8 *)cached_tile;
	int is_opaque = 1, is_transparent = 1;
	int x,y;
	int y1,y2,dy;
	const UINT8 *maskdata = cached_tile->tile_info.mask_data;

	if( maskdata==TILEMAP_BITMASK_TRANSPARENT )
	{
		dest_baseaddr[0] = WHOLLY_TRANSPARENT;
		return;
	}
	if( maskdata==TILEMAP_BITMASK_OPAQUE)
	{
		dest_baseaddr[0] = WHOLLY_OPAQUE;
		return;
	}
	if( flip&TILE_FLIPY )
	{
		y1 = tile_height-1;
		y2 = -1;
 		dy = -1;
 	}
 	else
 	{
		y1 = 0;
		y2 = tile_height;
 		dy = 1;
 	}
	if( flip&TILE_FLIPX )
	{
		for( y=y1; y!=y2; y+=dy )
		{
			dest  = dest_baseaddr + (y*tile_width)/8 + 1;
			for( x=tile_width/8-1; x>=0; x-- )
			{ /* right to left */
				data = flip_bit_table[*maskdata++];
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				dest[x] = data;
			}
		}
	}
	else {
		for( y=y1; y!=y2; y+=dy )
		{
			dest  = dest_baseaddr + (y*tile_width)/8 + 1;
			for( x=0; x<tile_width/8; x++ )
			{ /* left to right */
				data = *maskdata++;
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				dest[x] = data;
			}
		}
	}
	if( is_transparent )
	{
		dest_baseaddr[0] = WHOLLY_TRANSPARENT;
	}
	else if( is_opaque )
	{
		dest_baseaddr[0] = WHOLLY_OPAQUE;
	}
	else
	{
		dest_baseaddr[0] = PARTIALLY_TRANSPARENT;
	}
}

/* populate tile bitmask for TILEMAP_TRANSPARENT_COLOR */
static void compute_transparency_color( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	const UINT16 *clut = Machine->game_colortable + (cached_tile->tile_info.pal_data - Machine->remapped_colortable);
	UINT8 *dest_baseaddr = tilemap->front_offset + (UINT8 *)cached_tile;
	int transparent_color = tilemap->transparent_pen;
	int flags = cached_tile->tile_info.flags;
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	int flip = tilemap->logical_flip_to_cached_flip[flags&0x3];
	const UINT8 *pendata = cached_tile->tile_info.pen_data;
	int is_opaque = 1, is_transparent = 1;
	int x,y,bit;
	int y1,y2,dy;
	UINT32 data;
	UINT8 *dest;
	UINT32 pen;

	if( flip&TILE_FLIPY )
	{
		y1 = tile_height-1;
		y2 = -1;
 		dy = -1;
 	}
 	else {
		y1 = 0;
		y2 = tile_height;
 		dy = 1;
 	}

	if( flip&TILE_FLIPX )
	{
		for( y=y1; y!=y2; y+=dy )
		{
			dest  = dest_baseaddr + y*tile_width/8 + 1;
			for( x=tile_width/8-1; x>=0; x-- )
			{
				data = 0;
				for( bit=0; bit<8; bit++ )
				{
					pen = *pendata++;
					data = data>>1;
					if( clut[pen]!=transparent_color ) data |=0x80;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				dest[x] = data;
			}
		}
	}
	else {
		for( y=y1; y!=y2; y+=dy )
		{
			dest  = dest_baseaddr + y*tile_width/8 + 1;
			for( x=0; x<tile_width/8; x++ )
			{
				data = 0;
				for( bit=0; bit<8; bit++ )
				{
					pen = *pendata++;
					data = data<<1;
					if( clut[pen]!=transparent_color ) data |=0x01;
				}
				if( data!=0x00 ) is_transparent = 0;
				if( data!=0xff ) is_opaque = 0;
				dest[x] = data;
			}
		}
	}
	if( is_transparent )
	{
		dest_baseaddr[0] = WHOLLY_TRANSPARENT;
	}
	else if( is_opaque )
	{
		dest_baseaddr[0] = WHOLLY_OPAQUE;
	}
	else
	{
		dest_baseaddr[0] = PARTIALLY_TRANSPARENT;
	}
}

/* populate tile bitmask for TILEMAP_TRANSPARENT */
static void compute_transparency_pen( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT8 *dest_baseaddr = tilemap->front_offset + (UINT8 *)cached_tile;
	UINT32 pen_usage = cached_tile->tile_info.pen_usage;
	int transparent_pen = tilemap->transparent_pen;
	int flags = cached_tile->tile_info.flags;
	UINT32 fg_transmask;
	int tile_width;
	int tile_height;
	int flip;
	const UINT8 *pendata;
	int is_opaque, is_transparent;
	int x,y,bit;
	int y1,y2,dy;
	UINT32 data;
	UINT8 *dest;
	UINT32 pen;

	if( pen_usage )
	{
		fg_transmask = 1 << transparent_pen;
	 	if( flags&TILE_IGNORE_TRANSPARENCY )
	 	{
	 		fg_transmask = 0;
	 	}
		if( pen_usage == fg_transmask )
		{
			dest_baseaddr[0] = WHOLLY_TRANSPARENT;
			return;
		}
		else if( (pen_usage & fg_transmask)==0 )
		{
			dest_baseaddr[0] = WHOLLY_OPAQUE;
			return;
		}
	}
	{
		tile_width = tilemap->cached_tile_width;
		tile_height = tilemap->cached_tile_height;
		flip = tilemap->logical_flip_to_cached_flip[flags&0x3];
		pendata = cached_tile->tile_info.pen_data;
		is_opaque = 1;
		is_transparent = 1;

		if( flip&TILE_FLIPY )
		{
			y1 = tile_height-1;
			y2 = -1;
 			dy = -1;
 		}
 		else
 		{
			y1 = 0;
			y2 = tile_height;
 			dy = 1;
 		}

		if( flip&TILE_FLIPX )
		{
			for( y=y1; y!=y2; y+=dy )
			{
				dest  = dest_baseaddr + y*tile_width/8 + 1;
				for( x=tile_width/8-1; x>=0; x-- )
				{
					data = 0;
					for( bit=0; bit<8; bit++ )
					{
						pen = *pendata++;
						data = data>>1;
						if( pen!=transparent_pen ) data |=0x80;
					}
					if( data!=0x00 ) is_transparent = 0;
					if( data!=0xff ) is_opaque = 0;
					dest[x] = data;
				}
			}
		}
		else
		{
			for( y=y1; y!=y2; y+=dy )
			{
				dest  = dest_baseaddr + y*tile_width/8 + 1;
				for( x=0; x<tile_width/8; x++ )
				{
					data = 0;
					for( bit=0; bit<8; bit++ )
					{
						pen = *pendata++;
						data = data<<1;
						if( pen!=transparent_pen ) data |=0x01;
					}
					if( data!=0x00 ) is_transparent = 0;
					if( data!=0xff ) is_opaque = 0;
					dest[x] = data;
				}
			}
		}

		if( is_transparent )
		{
			dest_baseaddr[0] = WHOLLY_TRANSPARENT;
		}
		else if( is_opaque )
		{
			dest_baseaddr[0] = WHOLLY_OPAQUE;
		}
		else
		{
			dest_baseaddr[0] = PARTIALLY_TRANSPARENT;
		}
	}
}

/* populate tile bitmask for TILEMAP_SPLIT */
static void compute_split(
		struct tilemap *tilemap, struct cached_tile *cached_tile,
		UINT8 *dest_baseaddr, UINT32 transparent_pens )
{
	UINT32 pen_usage = cached_tile->tile_info.pen_usage;

	if( (pen_usage & transparent_pens) == 0 )
	{
		dest_baseaddr[0] = WHOLLY_OPAQUE;
		return;
	}
	else if( (pen_usage & ~transparent_pens) == 0 )
	{
		dest_baseaddr[0] = WHOLLY_TRANSPARENT;
		return;
	}
	else
	{
		int tile_width = tilemap->cached_tile_width;
		int tile_height = tilemap->cached_tile_height;
		int flip = tilemap->logical_flip_to_cached_flip[cached_tile->tile_info.flags&0x3];
		int x,y,bit;
		int y1,y2,dy;
		const UINT8 *pendata = cached_tile->tile_info.pen_data;

		dest_baseaddr[0] = PARTIALLY_TRANSPARENT;

		if( flip&TILE_FLIPY )
		{
			y1 = tile_height-1;
			y2 = -1;
	 		dy = -1;
	 	}
	 	else
	 	{
			y1 = 0;
			y2 = tile_height;
	 		dy = 1;
	 	}

		if( flip&TILE_FLIPX )
		{
			for( y=y1; y!=y2; y+=dy )
			{
				UINT8 *dest  = dest_baseaddr + y*tile_width/8 + 1;
				for( x=tile_width/8-1; x>=0; x-- )
				{
					UINT8 data = 0;
					for( bit=0; bit<8; bit++ )
					{
						UINT32 pen = *pendata++;
						data >>= 1;
						if( !((1<<pen)&transparent_pens) ) data |= 0x80; /* opaque pixel */
					}
					dest[x] = data;
				}
			}
		}
		else
		{
			for( y=y1; y!=y2; y+=dy )
			{
				UINT8 *dest  = dest_baseaddr + y*tile_width/8 + 1;
				for( x=0; x<tile_width/8; x++ )
				{
					UINT8 data = 0;
					for( bit=0; bit<8; bit++ )
					{
						UINT32 pen = *pendata++;
						data <<= 1;
						if( !((1<<pen)&transparent_pens) ) data |= 0x01;
					}
					dest[x] = data;
				}
			}
		}
	}
}

/* calculate bitmask for tile */
static void calculate_mask( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT32 transparent_pen = tilemap->transparent_pen;
	UINT32 *transmask = tilemap->transmask;
//	UINT32 pen_usage = cached_tile->tile_info.pen_usage;
	UINT32 front_mask, back_mask;
	int flags;
	UINT32 pen_mask;

	if( tilemap->type & TILEMAP_SPLIT )
	{
		flags = cached_tile->tile_info.flags;
		if( flags & TILE_IGNORE_TRANSPARENCY )
		{
			front_mask = 0;
			back_mask = ~0;
		}
		else
		{
			pen_mask = (transparent_pen<0)?0:(1<<transparent_pen);
			front_mask = transmask[(flags>>2)&3]; /* select mask */
			back_mask = (~front_mask)|pen_mask; /* 1 indicates a transparent pen */
		}

		compute_split(
			tilemap, cached_tile,
			tilemap->front_offset + (UINT8 *)cached_tile,
			front_mask );

		compute_split(
			tilemap, cached_tile,
			tilemap->back_offset + (UINT8 *)cached_tile,
			back_mask );
	}
	else
	{
		switch( tilemap->type )
		{
		case TILEMAP_BITMASK:
			compute_transparency_bitmask( tilemap, cached_tile );
			break;

		case TILEMAP_TRANSPARENT:
			compute_transparency_pen( tilemap, cached_tile );
			break;

		case TILEMAP_TRANSPARENT_COLOR:
			compute_transparency_color( tilemap, cached_tile );
			break;

		case TILEMAP_OPAQUE:
			break;

		default:
			printf( "unknown tilemap type %d\n", tilemap->type );
			exit(1);
		}
	}
}

/***********************************************************************************/

static void paint_tile8( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT8 *color = sizeof(struct cached_tile) + (UINT8 *)cached_tile;
	const UINT8 *pen_data = cached_tile->tile_info.pen_data;
	const UINT32 *pal_data = cached_tile->tile_info.pal_data;
	int pitch = tilemap->cached_tile_width;
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	int flip = tilemap->logical_flip_to_cached_flip[cached_tile->tile_info.flags&0x3];
	int x;

	if( flip&TILE_FLIPY )
	{
		pen_data += pitch*(tile_height-1);
		pitch = -pitch;
	}

	if( flip&TILE_FLIPX )
	{
		while( tile_height-- )
		{
			for( x=tile_width-1; x>=0; x-- )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	else
	{
		while( tile_height-- )
		{
			for( x=0; x<tile_width; x++ )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	cached_tile->bPaintOK = 1;
}

static void paint_tile16( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT16 *color = (UINT16 *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
	const UINT8 *pen_data = cached_tile->tile_info.pen_data;
	const UINT32 *pal_data = cached_tile->tile_info.pal_data;
	int pitch = tilemap->cached_tile_width;
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	int flip = tilemap->logical_flip_to_cached_flip[cached_tile->tile_info.flags&0x3];
	int x;

	if( flip&TILE_FLIPY )
	{
		pen_data += pitch*(tile_height-1);
		pitch = -pitch;
	}

	if( flip&TILE_FLIPX )
	{
		while( tile_height-- )
		{
			for( x=tile_width-1; x>=0; x-- )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	else
	{
		while( tile_height-- )
		{
			for( x=0; x<tile_width; x++ )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	cached_tile->bPaintOK = 1;
}

static void paint_tile32( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT32 *color = (UINT32 *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
	const UINT8 *pen_data = cached_tile->tile_info.pen_data;
	const UINT32 *pal_data = cached_tile->tile_info.pal_data;
	int pitch = tilemap->cached_tile_width;
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;
	int flip = tilemap->logical_flip_to_cached_flip[cached_tile->tile_info.flags&0x3];
	int x;

	if( flip&TILE_FLIPY )
	{
		pen_data += pitch*(tile_height-1);
		pitch = -pitch;
	}

	if( flip&TILE_FLIPX )
	{
		while( tile_height-- )
		{
			for( x=tile_width-1; x>=0; x-- )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	else
	{
		while( tile_height-- )
		{
			for( x=0; x<tile_width; x++ )
			{
				*color++ = pal_data[pen_data[x]];
			}
			pen_data += pitch;
		}
	}
	cached_tile->bPaintOK = 1;
}

/***********************************************************************************/

void tilemap_set_transparent_pen( struct tilemap *tilemap, int pen )
{
	tilemap->transparent_pen = pen;
}

void tilemap_set_transmask( struct tilemap *tilemap, int which, UINT32 penmask )
{
	tilemap->transmask[which] = penmask;
}

void tilemap_set_depth( struct tilemap *tilemap, int tile_depth, int tile_granularity )
{
	if( tilemap->tile_dirty_map )
		free( tilemap->tile_dirty_map);
	tilemap->tile_dirty_map = malloc( Machine->drv->total_colors >> tile_granularity);
	if( tilemap->tile_dirty_map )
	{
		tilemap->tile_depth = tile_depth;
		tilemap->tile_granularity = tile_granularity;
	}
}

/***********************************************************************************/

/* init_cached_tiles initializes hashtable buckets and logical cached tile pointers */
static int init_cached_tiles( struct tilemap *tilemap )
{
	tilemap->freelist = NULL;
	tilemap->cached_tile = malloc( tilemap->num_tiles * sizeof(struct cached_tile *) );
	if( tilemap->cached_tile )
	{
		tilemap->bucket = malloc( NUM_BUCKETS * sizeof(struct cached_tile *) );
		if( tilemap->bucket )
		{
			struct cached_tile **cached_tile = tilemap->cached_tile;
			int i;
			for( i=0; i<tilemap->num_tiles; i++ )
			{
				cached_tile[i] = NULL;
			}
			for( i=0; i<NUM_BUCKETS; i++ )
			{
				tilemap->bucket[i] = NULL;
			}
			return 0; /* no error */
		}
		free( tilemap->cached_tile );
	}
	return -1; /* error */
}

static void free_cached_tile_list( struct cached_tile *cached_tile )
{
	while( cached_tile )
	{
		struct cached_tile *next = cached_tile->next;
		free( cached_tile );
		cached_tile = next;
	}
}

static void dispose_cached_tiles( struct tilemap *tilemap )
{
	int i;
	for( i=0; i<NUM_BUCKETS; i++ )
	{
		free_cached_tile_list( tilemap->bucket[i] );
	}
	free_cached_tile_list( tilemap->freelist );
	free( tilemap->cached_tile );
}

/***********************************************************************************/
/* some common mappings */

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	(void)num_rows;
	return row*num_cols + col;
}
UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	(void)num_cols;
	return col*num_rows + row;
}

/*********************************************************************************/

static int mappings_create( struct tilemap *tilemap )
{
	int max_memory_offset = 0;
	UINT32 col,row;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	/* count offsets (might be larger than num_tiles) */
	for( row=0; row<num_logical_rows; row++ )
	{
		for( col=0; col<num_logical_cols; col++ )
		{
			UINT32 memory_offset = tilemap->get_memory_offset( col, row, num_logical_cols, num_logical_rows );
			if( memory_offset>max_memory_offset ) max_memory_offset = memory_offset;
		}
	}
	max_memory_offset++;
	tilemap->max_memory_offset = max_memory_offset;
	/* logical to cached (tilemap_mark_dirty) */
	tilemap->memory_offset_to_cached_indx = malloc( sizeof(UINT32)*max_memory_offset );
	if( tilemap->memory_offset_to_cached_indx )
	{
		/* cached to logical (get_tile_info) */
		tilemap->cached_indx_to_memory_offset = malloc( sizeof(UINT32)*tilemap->num_tiles );
		if( tilemap->cached_indx_to_memory_offset ) return 0; /* no error */
		free( tilemap->memory_offset_to_cached_indx );
	}
	return -1; /* error */
}

static void mappings_dispose( struct tilemap *tilemap )
{
	free( tilemap->cached_indx_to_memory_offset );
	free( tilemap->memory_offset_to_cached_indx );
}

static void mappings_update( struct tilemap *tilemap )
{
	UINT32 temp;
	int logical_flip;
	UINT32 logical_indx, cached_indx;
	UINT32 num_cached_rows = tilemap->num_cached_rows;
	UINT32 num_cached_cols = tilemap->num_cached_cols;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	for( logical_indx=0; logical_indx<tilemap->max_memory_offset; logical_indx++ )
	{
		tilemap->memory_offset_to_cached_indx[logical_indx] = -1;
	}

	for( logical_indx=0; logical_indx<tilemap->num_tiles; logical_indx++ )
	{
		UINT32 logical_col = logical_indx%num_logical_cols;
		UINT32 logical_row = logical_indx/num_logical_cols;
		int memory_offset = tilemap->get_memory_offset( logical_col, logical_row, num_logical_cols, num_logical_rows );
		UINT32 cached_col = logical_col;
		UINT32 cached_row = logical_row;
		if( tilemap->orientation & ORIENTATION_SWAP_XY ) SWAP(cached_col,cached_row)
		if( tilemap->orientation & ORIENTATION_FLIP_X ) cached_col = (num_cached_cols-1)-cached_col;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) cached_row = (num_cached_rows-1)-cached_row;
		cached_indx = cached_row*num_cached_cols+cached_col;
		tilemap->memory_offset_to_cached_indx[memory_offset] = cached_indx;
		tilemap->cached_indx_to_memory_offset[cached_indx] = memory_offset;
	}
	for( logical_flip = 0; logical_flip<4; logical_flip++ )
	{
		int cached_flip = logical_flip;
		if( tilemap->attributes&TILEMAP_FLIPX ) cached_flip ^= TILE_FLIPX;
		if( tilemap->attributes&TILEMAP_FLIPY ) cached_flip ^= TILE_FLIPY;
#ifndef PREROTATE_GFX
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPY;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPX;
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPX;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPY;
		}
#endif
		if( tilemap->orientation & ORIENTATION_SWAP_XY )
		{
			cached_flip = ((cached_flip&1)<<1) | ((cached_flip&2)>>1);
		}
		tilemap->logical_flip_to_cached_flip[logical_flip] = cached_flip;
	}
}

/***********************************************************************************/

#define DEPTH 8
#define paint_tile paint_tile8
#define DATA_TYPE UINT8
#define DrawClippedOpaqueTile DrawClippedOpaqueTile8
#define DrawClippedTranspTile DrawClippedTranspTile8

#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DECLARE(function,args,body) static void function##8x8x8BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DECLARE(function,args,body) static void function##16x16x8BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DECLARE(function,args,body) static void function##32x32x8BPP args body
#include "tilemap.c"

/* undefine */
#undef DEPTH
#undef paint_tile
#undef DATA_TYPE
#undef DrawClippedOpaqueTile
#undef DrawClippedTranspTile

#define DEPTH 16
#define paint_tile paint_tile16
#define DATA_TYPE UINT16
#define DrawClippedOpaqueTile DrawClippedOpaqueTile16
#define DrawClippedTranspTile DrawClippedTranspTile16
#define DrawClippedAlphaOpaqueTile DrawClippedAlphaOpaqueTile16
#define DrawClippedAlphaTranspTile DrawClippedAlphaTranspTile16

#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DECLARE(function,args,body) static void function##8x8x16BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DECLARE(function,args,body) static void function##16x16x16BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DECLARE(function,args,body) static void function##32x32x16BPP args body
#include "tilemap.c"

/* undefine */
#undef DEPTH
#undef paint_tile
#undef DATA_TYPE
#undef DrawClippedOpaqueTile
#undef DrawClippedTranspTile
#undef DrawClippedAlphaOpaqueTile
#undef DrawClippedAlphaTranspTile

#define DEPTH 32
#define paint_tile paint_tile32
#define DATA_TYPE UINT32
#define DrawClippedOpaqueTile DrawClippedOpaqueTile32
#define DrawClippedTranspTile DrawClippedTranspTile32
#define DrawClippedAlphaOpaqueTile DrawClippedAlphaOpaqueTile32
#define DrawClippedAlphaTranspTile DrawClippedAlphaTranspTile32

#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DECLARE(function,args,body) static void function##8x8x32BPP args body
#include "tilemap.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DECLARE(function,args,body) static void function##16x16x32BPP args body
#include "tilemap.c"

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
#define DECLARE(function,args,body) static void function##32x32x32BPP args body
#include "tilemap.c"

/***********************************************************************************/

static int install_draw_handlers( struct tilemap *tilemap )
{
	int tile_width = tilemap->cached_tile_width;
	int tile_height = tilemap->cached_tile_height;

	int bpp = Machine->scrbitmap->depth;

	if( tile_width==8 && tile_height==8 )
	{
		switch(bpp)
		{
		case 8:
			tilemap->draw_transparent = draw_transparent8x8x8BPP;
			tilemap->draw_opaque = draw_opaque8x8x8BPP;
			tilemap->draw_alpha = 0;
			break;
		case 15:
		case 16:
			tilemap->draw_transparent = draw_transparent8x8x16BPP;
			tilemap->draw_opaque = draw_opaque8x8x16BPP;
			tilemap->draw_alpha = draw_alpha8x8x16BPP;
			break;
		case 32:
			tilemap->draw_transparent = draw_transparent8x8x32BPP;
			tilemap->draw_opaque = draw_opaque8x8x32BPP;
			tilemap->draw_alpha = draw_alpha8x8x32BPP;
			break;
		}
		tilemap->mark_visible = mark_visible8x8x8BPP;
	}
	else if( tile_width==16 && tile_height==16 )
	{
		switch(bpp)
		{
		case 8:
			tilemap->draw_transparent = draw_transparent16x16x8BPP;
			tilemap->draw_opaque = draw_opaque16x16x8BPP;
			tilemap->draw_alpha = 0;
			break;
		case 15:
		case 16:
			tilemap->draw_transparent = draw_transparent16x16x16BPP;
			tilemap->draw_opaque = draw_opaque16x16x16BPP;
			tilemap->draw_alpha = draw_alpha16x16x16BPP;
			break;
		case 32:
			tilemap->draw_transparent = draw_transparent16x16x32BPP;
			tilemap->draw_opaque = draw_opaque16x16x32BPP;
			tilemap->draw_alpha = draw_alpha16x16x32BPP;
			break;
		}
		tilemap->mark_visible = mark_visible16x16x8BPP;
	}
	else if( tile_width==32 && tile_height==32 )
	{
		switch(bpp)
		{
		case 8:
			tilemap->draw_transparent = draw_transparent32x32x8BPP;
			tilemap->draw_opaque = draw_opaque32x32x8BPP;
			tilemap->draw_alpha = 0;
			break;
		case 15:
		case 16:
			tilemap->draw_transparent = draw_transparent32x32x16BPP;
			tilemap->draw_opaque = draw_opaque32x32x16BPP;
			tilemap->draw_alpha = draw_alpha32x32x16BPP;
			break;
		case 32:
			tilemap->draw_transparent = draw_transparent32x32x32BPP;
			tilemap->draw_opaque = draw_opaque32x32x32BPP;
			tilemap->draw_alpha = draw_alpha32x32x32BPP;
			break;
		}
		tilemap->mark_visible = mark_visible32x32x8BPP;
	}
	else
	{
		printf( "no draw handlers exists for this tile size (%dx%d)", tile_width, tile_height );
		return -1;
	}
	return 0;
}

/***********************************************************************************/

static void tilemap_reset(void)
{
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

int tilemap_init( void )
{
	UINT32 value, data, bit;

	for( value=0; value<0x100; value++ )
	{
		data = 0;
		for( bit=0; bit<8; bit++ )
		{
			if( (value>>bit)&1 ) data |= 0x80>>bit;
		}
		flip_bit_table[value] = data;
	}
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;
	first_tilemap = 0;
	state_save_register_func_postload(tilemap_reset);
	priority_bitmap = osd_alloc_bitmap( screen_width,screen_height,8 );
	if( priority_bitmap )
	{
		return 0;
	}
	return -1;
}

void tilemap_close( void )
{
	while( first_tilemap )
	{
		struct tilemap *next = first_tilemap->next;
		tilemap_dispose( first_tilemap );
		first_tilemap = next;
	}
	osd_free_bitmap( priority_bitmap );
}

/***********************************************************************************/

struct tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height, /* in pixels */
	int num_cols, int num_rows /* in tiles */ )
{
	UINT32 temp;
	int num_tiles;

	struct tilemap *tilemap = calloc( 1,sizeof( struct tilemap ) );
	if( tilemap )
	{
		num_tiles = num_cols*num_rows;
		tilemap->num_logical_cols = num_cols;
		tilemap->num_logical_rows = num_rows;
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			SWAP( tile_width, tile_height )
			SWAP( num_cols,num_rows )
		}
		tilemap->num_cached_cols = num_cols;
		tilemap->num_cached_rows = num_rows;
		tilemap->num_tiles = num_tiles;
		tilemap->cached_tile_width = tile_width;
		tilemap->cached_tile_height = tile_height;
		tilemap->pens_per_tile = tile_width*tile_height;

		tilemap->cached_tile_size = sizeof(struct cached_tile);

		/* reserve space for cached color data */
		switch(Machine->scrbitmap->depth)
		{
		case 8:
			tilemap->cached_tile_size += tilemap->pens_per_tile*sizeof(UINT8);
			break;
		case 15:
		case 16:
			tilemap->cached_tile_size += tilemap->pens_per_tile*sizeof(UINT16);
			break;
		case 32:
			tilemap->cached_tile_size += tilemap->pens_per_tile*sizeof(UINT32);
			break;
		}

		tilemap->front_offset = tilemap->cached_tile_size;
		if( type != TILEMAP_OPAQUE )
		{
			tilemap->cached_tile_size += tilemap->pens_per_tile/8+1;
		}

		tilemap->back_offset = tilemap->cached_tile_size;
		if( type & TILEMAP_SPLIT )
		{
			tilemap->cached_tile_size += tilemap->pens_per_tile/8+1;
		}

		tilemap->cached_width = tile_width*num_cols;
		tilemap->cached_height = tile_height*num_rows;
		tilemap->tile_get_info = tile_get_info;
		tilemap->get_memory_offset = get_memory_offset;
		tilemap->orientation = Machine->orientation;
		tilemap->enable = 1;
		tilemap->type = type;

		tilemap->scroll_rows = 1;
		tilemap->scroll_cols = 1;

		tilemap->transparent_pen = -1;

		tilemap->tile_depth = 0;
		tilemap->tile_granularity = 0;
		tilemap->tile_dirty_map = 0;

		init_cached_tiles( tilemap );

		tilemap->visible = bitarray_create( num_tiles );

		tilemap->rowscroll = calloc(tilemap->cached_height,sizeof(UINT32));
		tilemap->colscroll = calloc(tilemap->cached_width,sizeof(UINT32));

		if( tilemap->visible && tilemap->rowscroll && tilemap->colscroll &&
			(mappings_create( tilemap )==0) )
		{
			install_draw_handlers( tilemap );
			mappings_update( tilemap );
			tilemap_set_clip( tilemap, &Machine->visible_area );

			tilemap->next = first_tilemap;
			first_tilemap = tilemap;
			return tilemap;
		}
		tilemap_dispose( tilemap );
	}
	return 0;
}

void tilemap_dispose( struct tilemap *tilemap )
{
	if( tilemap==first_tilemap )
	{
		first_tilemap = tilemap->next;
	}
	else
	{
		struct tilemap *prev = first_tilemap;
		while( prev->next != tilemap ) prev = prev->next;
		prev->next =tilemap->next;
	}

	dispose_cached_tiles( tilemap );

	/* dispose bitarrays */
	free( tilemap->visible );

	/* dispose scroll registers */
	free( tilemap->rowscroll );
	free( tilemap->colscroll );

	/* dispose logical->cached mappings */
	mappings_dispose( tilemap );

	free( tilemap );
}

/***********************************************************************************/

static void unregister_pens( struct tile_info *tileinfo, int num_pens )
{
	if( palette_used_colors){
		UINT32 pen_usage = tileinfo->pen_usage;
		if( pen_usage )
		{
			palette_decrease_usage_count(
				tileinfo->pal_data-Machine->remapped_colortable,
				pen_usage,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
		else
		{
			palette_decrease_usage_countx(
				tileinfo->pal_data-Machine->remapped_colortable,
				num_pens,
				tileinfo->pen_data,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
	}
}

static void register_pens( struct tile_info *tileinfo, int num_pens )
{
	if( palette_used_colors ){
		UINT32 pen_usage = tileinfo->pen_usage;
		if( pen_usage )
		{
			palette_increase_usage_count(
				tileinfo->pal_data-Machine->remapped_colortable,
				pen_usage,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
		else
		{
			palette_increase_usage_countx(
				tileinfo->pal_data-Machine->remapped_colortable,
				num_pens,
				tileinfo->pen_data,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
	}
}

/***********************************************************************************/

void tilemap_set_enable( struct tilemap *tilemap, int enable ){
	tilemap->enable = enable?1:0;
}

void tilemap_set_flip( struct tilemap *tilemap, int attributes )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_set_flip( tilemap, attributes );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->attributes!=attributes )
	{
		tilemap->attributes = attributes;
		tilemap->orientation = Machine->orientation;
		if( attributes&TILEMAP_FLIPY ){
			tilemap->orientation ^= ORIENTATION_FLIP_Y;
			tilemap->scrolly_delta = tilemap->dy_if_flipped;
		}
		else
		{
			tilemap->scrolly_delta = tilemap->dy;
		}
		if( attributes&TILEMAP_FLIPX )
		{
			tilemap->orientation ^= ORIENTATION_FLIP_X;
			tilemap->scrollx_delta = tilemap->dx_if_flipped;
		}
		else
		{
			tilemap->scrollx_delta = tilemap->dx;
		}

		mappings_update( tilemap );
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip )
{
	UINT32 temp;
	int left,top,right,bottom;
	if( clip )
	{
		left = clip->min_x;
		top = clip->min_y;
		right = clip->max_x+1;
		bottom = clip->max_y+1;
		if( tilemap->orientation & ORIENTATION_SWAP_XY )
		{
			SWAP(left,top)
			SWAP(right,bottom)
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			SWAP(left,right)
			left = screen_width-left;
			right = screen_width-right;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			SWAP(top,bottom)
			top = screen_height-top;
			bottom = screen_height-bottom;
		}
	}
	else
	{
		left = 0;
		top = 0;
		right = tilemap->cached_width;
		bottom = tilemap->cached_height;
	}
	tilemap->clip_left = left;
	tilemap->clip_right = right;
	tilemap->clip_top = top;
	tilemap->clip_bottom = bottom;
}

/***********************************************************************************/

void tilemap_set_scroll_cols( struct tilemap *tilemap, int n )
{
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if( tilemap->scroll_rows != n )
		{
			tilemap->scroll_rows = n;
		}
	}
	else
	{
		if( tilemap->scroll_cols != n )
		{
			tilemap->scroll_cols = n;
		}
	}
}

void tilemap_set_scroll_rows( struct tilemap *tilemap, int n )
{
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if (tilemap->scroll_cols != n)
		{
			tilemap->scroll_cols = n;
		}
	}
	else
	{
		if( tilemap->scroll_rows != n )
		{
			tilemap->scroll_rows = n;
		}
	}
}

/***********************************************************************************/

/*	new_cached_tile returns an unitialized cached tile structure.
**	if the freelist isn't empty, an already-malloced tile is returned.
**	otherwise, a new cached tile is malloced and returned.
*/
static struct cached_tile *new_cached_tile( struct tilemap *tilemap )
{
	struct cached_tile *cached_tile = tilemap->freelist;
	if( cached_tile )
	{
		tilemap->freelist = cached_tile->next;
	}
	else
	{
		cached_tile = malloc( tilemap->cached_tile_size );
		if( !cached_tile ) printf( "can't malloc cached_tile" );
	}
	return cached_tile;
}

/*	free_cached_tile adds the cached tile to the freelist, so that it's memory
**	can be efficiently reused */
static void cached_tile_free( struct tilemap *tilemap, struct cached_tile *cached_tile )
{
	UINT32 indx = (cached_tile->tile_info.tile_number)%NUM_BUCKETS;

	struct cached_tile *prev = NULL;
	struct cached_tile *curr = tilemap->bucket[indx];

	/* we must remove from the associated bucket's linked list */
	while( curr!=cached_tile )
	{
		prev = curr;
		curr = curr->next;
	}

	if( prev )
	{ /* this wasn't the first item in the bucket */
		prev->next = curr->next;
	}
	else
	{ /* we removed the bucket's head */
		tilemap->bucket[indx] = curr->next;
	}

	cached_tile->next = tilemap->freelist;
	tilemap->freelist = cached_tile;
}

/***********************************************************************************/

static void mark_tile_dirty( struct tilemap *tilemap, int indx )
{
	struct cached_tile *cached_tile = tilemap->cached_tile[indx];
	if( cached_tile )
	{
		if( cached_tile->refcount )
		{
			cached_tile->refcount--;
		}
		else
		{
			unregister_pens( &cached_tile->tile_info, tilemap->pens_per_tile );
			cached_tile_free( tilemap, cached_tile );
			/*
				note: we may wish to keep the tile cached even though it is no longer
				present in the tilemap, so that if it reoccurs we can more quickly
				fetch it.
			*/
		}
		tilemap->cached_tile[indx] = NULL;
	}
}

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int memory_offset )
{
	if( memory_offset < tilemap->max_memory_offset )
	{
		int indx = tilemap->memory_offset_to_cached_indx[memory_offset];
		if( indx>=0 ) mark_tile_dirty( tilemap, indx );
	}
	else
	{
		logerror( "tilemap_mark_tile_dirty: bad param\n" );
	}
}

void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_mark_all_tiles_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else
	{
		int indx;
		for( indx=0; indx<tilemap->num_tiles; indx++ )
		{
			mark_tile_dirty( tilemap, indx );
		}
	}
}

static void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_mark_all_pixels_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else
	{
		/* the dynamic palette has been remapped. */
		int indx;
		for( indx=0; indx<tilemap->num_tiles; indx++ )
		{
			if( bitarray_get( tilemap->visible, indx ) )
			{
				/*	mark visible tiles invalid */
				struct cached_tile *cached_tile = tilemap->cached_tile[indx];
				if( cached_tile ) cached_tile->bPaintOK = 0;
			}
			else
			{
				/* mark any invisible tiles dirty */
				mark_tile_dirty( tilemap, indx );
			}
		}
	}
}

void tilemap_dirty_palette( const UINT8 *dirty_pens )
{
	UINT32 *color_base = Machine->remapped_colortable;
	struct tilemap *tilemap = first_tilemap;
	while( tilemap )
	{
		if( !tilemap->tile_dirty_map)
			tilemap_mark_all_pixels_dirty( tilemap );
		else
		{
			UINT8 *dirty_map = tilemap->tile_dirty_map;
			int i, j, pen;
			int step = 1 << tilemap->tile_granularity;
			int count = 1 << tilemap->tile_depth;
			int limit = Machine->drv->total_colors - count;
			pen = 0;
			for( i=0; i<limit; i+=step )
			{
				for( j=0; j<count; j++ )
					if( dirty_pens[i+j] )
					{
						dirty_map[pen++] = 1;
						goto next;
					}
				dirty_map[pen++] = 0;
			next:
				;
			}

			for( i=0; i<tilemap->num_tiles; i++ )
			{
				struct cached_tile *cached_tile = tilemap->cached_tile[i];
				if (cached_tile && cached_tile->bPaintOK)
				{
					j = (cached_tile->tile_info.pal_data - color_base) >> tilemap->tile_granularity;
					if( dirty_map[j] )
						cached_tile->bPaintOK = 0;
				}
			}
		}

		tilemap = tilemap->next;
	}
}

/***********************************************************************************/

void tilemap_set_scrolldx( struct tilemap *tilemap, int dx, int dx_if_flipped )
{
	tilemap->dx = dx;
	tilemap->dx_if_flipped = dx_if_flipped;
	tilemap->scrollx_delta = ( tilemap->attributes & TILEMAP_FLIPX )?dx_if_flipped:dx;
}

void tilemap_set_scrolldy( struct tilemap *tilemap, int dy, int dy_if_flipped )
{
	tilemap->dy = dy;
	tilemap->dy_if_flipped = dy_if_flipped;
	tilemap->scrolly_delta = ( tilemap->attributes & TILEMAP_FLIPY )?dy_if_flipped:dy;
}

void tilemap_set_scrollx( struct tilemap *tilemap, int which, int value )
{
	value = tilemap->scrollx_delta-value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value ){
			tilemap->colscroll[which] = value;
		}
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value )
		{
			tilemap->rowscroll[which] = value;
		}
	}
}
void tilemap_set_scrolly( struct tilemap *tilemap, int which, int value )
{
	value = tilemap->scrolly_delta - value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value )
		{
			tilemap->rowscroll[which] = value;
		}
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value )
		{
			tilemap->colscroll[which] = value;
		}
	}
}

static void update_visible( struct tilemap *tilemap )
{
	int xpos,ypos;
	UINT32 scrollx, scrolly;
	UINT32 rows = tilemap->scroll_rows;
	UINT32 cols = tilemap->scroll_cols;
	const UINT32 *rowscroll = tilemap->rowscroll;
	const UINT32 *colscroll = tilemap->colscroll;
	UINT32 left = tilemap->clip_left;
	UINT32 right = tilemap->clip_right;
	UINT32 top = tilemap->clip_top;
	UINT32 bottom = tilemap->clip_bottom;
//	UINT32 tile_height = tilemap->cached_tile_height;
	UINT32 col, colwidth;
	UINT32 row, rowheight;
	UINT32 cons;

	bitarray_wipe( tilemap->visible, tilemap->num_tiles );

	if( rows == 1 && cols == 1 )
	{ /* XY scrolling playfield */
		scrollx = rowscroll[0]%tilemap->cached_width;
		scrolly = colscroll[0]%tilemap->cached_height;
		blit.clip_left = left;
	 	blit.clip_top = top;
	 	blit.clip_right = right;
	 	blit.clip_bottom = bottom;
		for( ypos = scrolly - tilemap->cached_height;
			ypos < blit.clip_bottom;
			ypos += tilemap->cached_height )
		{
			for( xpos = scrollx - tilemap->cached_width;
				xpos < blit.clip_right;
				xpos += tilemap->cached_width )
			{
				tilemap->mark_visible( tilemap, xpos, ypos );
			}
		}
	}
	else if( rows == 1 )
	{ /* scrolling columns + horizontal scroll */
		col = 0;
		colwidth = tilemap->cached_width / cols;
		scrollx = rowscroll[0]%tilemap->cached_width;
		blit.clip_top = top;
		blit.clip_bottom = bottom;
		while( col < cols )
		{
			cons = 1;
			scrolly = colscroll[col];
			/* count consecutive columns scrolled by the same amount */
			if( scrolly != TILE_LINE_DISABLED )
			{
				while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;
				scrolly %= tilemap->cached_height;
				blit.clip_left = col * colwidth + scrollx;
				if (blit.clip_left < left) blit.clip_left = left;
				blit.clip_right = (col + cons) * colwidth + scrollx;
				if (blit.clip_right > right) blit.clip_right = right;
				for( ypos = scrolly - tilemap->cached_height;
					ypos < blit.clip_bottom;
					ypos += tilemap->cached_height )
				{
					tilemap->mark_visible( tilemap, scrollx,ypos );
				}
				blit.clip_left = col * colwidth + scrollx - tilemap->cached_width;
				if (blit.clip_left < left) blit.clip_left = left;
				blit.clip_right = (col + cons) * colwidth + scrollx - tilemap->cached_width;
				if (blit.clip_right > right) blit.clip_right = right;
				for( ypos = scrolly - tilemap->cached_height;
					ypos < blit.clip_bottom;
					ypos += tilemap->cached_height )
				{
					tilemap->mark_visible( tilemap, scrollx - tilemap->cached_width,ypos );
				}
			}
			col += cons;
		}
	}
	else if( cols == 1 )
	{ /* scrolling rows + vertical scroll */
		row = 0;
		rowheight = tilemap->cached_height / rows;
		scrolly = colscroll[0] % tilemap->cached_height;
		blit.clip_left = left;
		blit.clip_right = right;
		while( row < rows )
		{
			cons = 1;
			scrollx = rowscroll[row];
			/* count consecutive rows scrolled by the same amount */
			if( scrollx != TILE_LINE_DISABLED )
			{
				while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
				scrollx %= tilemap->cached_width;
				blit.clip_top = row * rowheight + scrolly;
				if (blit.clip_top < top) blit.clip_top = top;
				blit.clip_bottom = (row + cons) * rowheight + scrolly;
				if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
				for( xpos = scrollx - tilemap->cached_width;
					xpos < blit.clip_right;
					xpos += tilemap->cached_width )
				{
					tilemap->mark_visible( tilemap, xpos,scrolly );
				}
				blit.clip_top = row * rowheight + scrolly - tilemap->cached_height;
				if (blit.clip_top < top) blit.clip_top = top;
				blit.clip_bottom = (row + cons) * rowheight + scrolly - tilemap->cached_height;
				if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
				for( xpos = scrollx - tilemap->cached_width;
					xpos < blit.clip_right;
					xpos += tilemap->cached_width )
				{
					tilemap->mark_visible( tilemap, xpos,scrolly - tilemap->cached_height );
				}
			}
			row += cons;
		}
	}
}

/*	get_cached_tile takes as input pointers to a tilemap and tile_info description.
**
**	If an identical tile is cached, the cached tile's refcount is incremented and
**	a pointer to it is returned.
**
**	Otherwise, a new cached_tile structure is allocated, initialized, and returned.
*/
static struct cached_tile *get_cached_tile( struct tilemap *tilemap )
{
	UINT32 indx = (tile_info.tile_number)%NUM_BUCKETS;
	struct cached_tile *cached_tile = tilemap->bucket[indx];
	while( cached_tile )
	{ /* walk through all members of this bucket */
		if( memcmp( &cached_tile->tile_info, &tile_info, sizeof(struct tile_info) )==0 )
		{
			cached_tile->refcount++;
			return cached_tile;
		}
		cached_tile = cached_tile->next;
	}
	cached_tile = new_cached_tile( tilemap ); /* malloc */
	if( cached_tile )
	{
		cached_tile->next = tilemap->bucket[indx];
		tilemap->bucket[indx] = cached_tile;
		cached_tile->tile_info = tile_info;
		cached_tile->refcount = 0;
		register_pens( &tile_info, tilemap->pens_per_tile );
		cached_tile->bPaintOK = 0;
		calculate_mask( tilemap, cached_tile );
	}
	return cached_tile;
}

static void update_tile( void *data, UINT32 indx )
{
	struct tilemap *tilemap = data;
	if( tilemap->cached_tile[indx]==NULL )
	{
		UINT32 memory_offset = tilemap->cached_indx_to_memory_offset[indx];
		tilemap->tile_get_info( memory_offset );
		tilemap->cached_tile[indx] = get_cached_tile( (struct tilemap *)data );
	}
}

void tilemap_update( struct tilemap *tilemap )
{
profiler_mark(PROFILER_TILEMAP_UPDATE);
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_update( tilemap );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->enable )
	{
		update_visible( tilemap );
		memset( &tile_info, 0x00, sizeof(struct tile_info) );
		bitarray_iterate( tilemap->visible, tilemap->num_tiles, tilemap, update_tile );
	}
profiler_mark(PROFILER_END);
}

/***********************************************************************************/

void tilemap_draw(
		struct osd_bitmap *dest,
		struct tilemap *tilemap,
		UINT32 flags,
		UINT32 priority )
{
	int xpos,ypos;
	UINT32 cons;
	UINT32 scrollx, scrolly;
	UINT32 rows, cols;
	const UINT32 *rowscroll, *colscroll;
	void (*draw)( struct tilemap *, INT32, INT32 );
	int left, right, top, bottom;
	int tile_height;
	UINT32 col, colwidth;
	UINT32 row, rowheight;

profiler_mark(PROFILER_TILEMAP_DRAW);
	if( tilemap->enable )
	{
		rows = tilemap->scroll_rows;
		rowscroll = tilemap->rowscroll;
		cols = tilemap->scroll_cols;
		colscroll = tilemap->colscroll;

		left = tilemap->clip_left;
		right = tilemap->clip_right;
		top = tilemap->clip_top;
		bottom = tilemap->clip_bottom;

		tile_height = tilemap->cached_tile_height;

		blit.screen = dest;

		blit.tile_priority = flags&0xf;
		blit.tilemap_priority_code = priority;

		if( tilemap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY) )
		{
			draw = tilemap->draw_opaque;
		}
		else if( flags & TILEMAP_ALPHA )
		{
			draw = tilemap->draw_alpha;
		}
		else
		{
			draw = tilemap->draw_transparent;
		}

		if( flags&TILEMAP_BACK )
		{
			blit.mask_offset = tilemap->back_offset;
		}
		else
		{
			blit.mask_offset = tilemap->front_offset;
		}

		if( rows == 1 && cols == 1 )
		{ /* XY scrolling playfield */
			scrollx = rowscroll[0] % tilemap->cached_width;
			scrolly = colscroll[0] % tilemap->cached_height;
	 		blit.clip_left = left;
	 		blit.clip_top = top;
	 		blit.clip_right = right;
	 		blit.clip_bottom = bottom;
			for(
				ypos = scrolly - tilemap->cached_height;
				ypos < blit.clip_bottom;
				ypos += tilemap->cached_height )
			{
				for(
					xpos = scrollx - tilemap->cached_width;
					xpos < blit.clip_right;
					xpos += tilemap->cached_width )
				{
					draw( tilemap,xpos,ypos );
				}
			}
		}
		else if( rows == 1 )
		{ /* scrolling columns + horizontal scroll */
			col = 0;
			colwidth = tilemap->cached_width / cols;
			scrollx = rowscroll[0] % tilemap->cached_width;
			blit.clip_top = top;
			blit.clip_bottom = bottom;
			while( col < cols )
			{
				cons = 1;
				scrolly = colscroll[col];
	 			/* count consecutive columns scrolled by the same amount */
				if( scrolly != TILE_LINE_DISABLED ){
					while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;
					scrolly %= tilemap->cached_height;
					blit.clip_left = col * colwidth + scrollx;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx;
					if (blit.clip_right > right) blit.clip_right = right;
					for( ypos = scrolly - tilemap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tilemap->cached_height )
					{
						draw( tilemap,scrollx,ypos );
					}
					blit.clip_left = col * colwidth + scrollx - tilemap->cached_width;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx - tilemap->cached_width;
					if (blit.clip_right > right) blit.clip_right = right;
					for( ypos = scrolly - tilemap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tilemap->cached_height )
					{
						draw( tilemap,scrollx - tilemap->cached_width,ypos );
					}
				}
				col += cons;
			}
		}
		else if( cols == 1 )
		{ /* scrolling rows + vertical scroll */
			row = 0;
			rowheight = tilemap->cached_height / rows;
			scrolly = colscroll[0] % tilemap->cached_height;
			blit.clip_left = left;
			blit.clip_right = right;
			while( row < rows )
			{
				cons = 1;
				scrollx = rowscroll[row];
				/* count consecutive rows scrolled by the same amount */
				if( scrollx != TILE_LINE_DISABLED )
				{
					while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
					scrollx %= tilemap->cached_width;
					blit.clip_top = row * rowheight + scrolly;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for( xpos = scrollx - tilemap->cached_width;
						xpos < blit.clip_right;
						xpos += tilemap->cached_width )
					{
						draw( tilemap,xpos,scrolly );
					}
					blit.clip_top = row * rowheight + scrolly - tilemap->cached_height;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly - tilemap->cached_height;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for( xpos = scrollx - tilemap->cached_width;
						xpos < blit.clip_right;
						xpos += tilemap->cached_width )
					{
						draw( tilemap,xpos,scrolly - tilemap->cached_height );
					}
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}

/* tilemap_debug is not part of the tilemap API.  It is exclusive to the new tilemap code. */
void tilemap_debug( struct tilemap *tilemap )
{
	int i;
	logerror( "tilemap debug:\n" );
	logerror( "num tiles: %d\n", tilemap->num_tiles );
	logerror( "pens per tile: %d\n", tilemap->num_tiles );
	logerror( "cached tile size: %d\n", tilemap->cached_tile_size );
	logerror( "front offset: %d\n", tilemap->front_offset );
	logerror( "back offset: %d\n", tilemap->back_offset );

	logerror( "cached_tile_width: %d\n", tilemap->cached_tile_width );
	logerror( "cached_tile_height: %d\n", tilemap->cached_tile_height );
	logerror( "cached_width: %d\n", tilemap->cached_width );
	logerror( "cached_height: %d\n", tilemap->cached_height );
	logerror( "type: %d\n", tilemap->type );

	for( i=0; i<tilemap->num_tiles; i++ )
	{
		struct cached_tile *tile = tilemap->cached_tile[i];
		logerror( "%04x) ", i );
		if( tile )
		{
			logerror( "refcount=%d\n, painted:%d", tile->refcount,tile->bPaintOK );
		}
		else
		{
			logerror( "NULL\n" );
		}
	}
}

/***********************************************************************************/

#else /* DECLARE */
/*
	The following procedure body is #included several times by
	tilemap.c to implement a suite of tilemap_draw subroutines.

	The constants TILE_WIDTH and TILE_HEIGHT are different in
	each instance of this code, allowing arithmetic shifts to
	be used by the compiler instead of multiplies/divides.

	The drawing routines below are not optimized!
*/
DECLARE( draw_opaque, (struct tilemap *tilemap, INT32 xpos, INT32 ypos),
{
	UINT32 row;
	UINT32 col;
	int indx;
	struct cached_tile *cached_tile;
	int sx0;
	int sy0;
	DATA_TYPE *pColorData;

	/* screen coordinates to tilemap coordinates */
	int x0 = blit.clip_left		- xpos;
	int x1 = blit.clip_right	- xpos;
	int y0 = blit.clip_top		- ypos;
	int y1 = blit.clip_bottom	- ypos;

	/* clip to tilemap bounds */
	if( x0<0 ) x0 = 0;
	if( y0<0 ) y0 = 0;
	if( x1>tilemap->cached_width )	x1 = tilemap->cached_width;
	if( y1>tilemap->cached_height )	y1 = tilemap->cached_height;

	if( x1>x0 && y1>y0 )
	{
		/* only proceed if not totally clipped */
		x0 = ((UINT32)x0)/TILE_WIDTH;
		y0 = ((UINT32)y0)/TILE_HEIGHT;
		x1 = ((UINT32)(x1+TILE_WIDTH-1))/TILE_WIDTH;
		y1 = ((UINT32)(y1+TILE_HEIGHT-1))/TILE_HEIGHT;

		sy0 = ypos+y0*TILE_HEIGHT;
		for( row=y0; row<y1; row++ )
		{
			sx0 = xpos+x0*TILE_WIDTH;
			indx = row*tilemap->num_cached_cols+x0;

			for( col=x0; col<x1; col++ )
			{
				cached_tile = tilemap->cached_tile[indx++];
				if( cached_tile->tile_info.priority == blit.tile_priority )
				{
					if( !cached_tile->bPaintOK )
					{
						paint_tile( tilemap, cached_tile );
					}
					pColorData = (DATA_TYPE *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
					DrawClippedOpaqueTile( pColorData, sx0, sy0, TILE_WIDTH, TILE_HEIGHT );
				}
				sx0 += TILE_WIDTH;
			} /* col */
			sy0 += TILE_HEIGHT;
		} /* row */
	} /* clip */
})

DECLARE( draw_transparent, (struct tilemap *tilemap, INT32 xpos, INT32 ypos),
{
	UINT32 row;
	UINT32 col;
	int indx;
	struct cached_tile *cached_tile;
	int sx0;
	int sy0;
	DATA_TYPE *pColorData;
	const UINT8 *pMask;
	int mask_offset;

	/* screen coordinates to tilemap coordinates */
	int x0 = blit.clip_left - xpos;
	int y0 = blit.clip_top - ypos;
	int x1 = blit.clip_right - xpos;
	int y1 = blit.clip_bottom - ypos;

	/* clip to tilemap bounds */
	if( x0<0 ) x0 = 0;
	if( y0<0 ) y0 = 0;
	if( x1>tilemap->cached_width ) x1 = tilemap->cached_width;
	if( y1>tilemap->cached_height ) y1 = tilemap->cached_height;

	if( x1>x0 && y1>y0 )
	{ /* only proceed if not totally clipped */
		mask_offset = blit.mask_offset;

		x0 = ((UINT32)x0)/TILE_WIDTH;
		y0 = ((UINT32)y0)/TILE_HEIGHT;
		x1 = ((UINT32)(x1+TILE_WIDTH-1))/TILE_WIDTH;
		y1 = ((UINT32)(y1+TILE_HEIGHT-1))/TILE_HEIGHT;

		sy0 = ypos+y0*TILE_HEIGHT;
		for( row=y0; row<y1; row++ )
		{
			sx0 = xpos+x0*TILE_WIDTH;
			indx = row*tilemap->num_cached_cols+x0;

			for( col=x0; col<x1; col++ )
			{
				cached_tile = tilemap->cached_tile[indx++];
				if( cached_tile->tile_info.priority == blit.tile_priority )
				{
					pMask = mask_offset+(UINT8 *)cached_tile;
					/* first byte is mask type; subsequant bytes are a packed bitmask */
					switch( *pMask )
					{
					case WHOLLY_OPAQUE:
					// TBA: use function pointer for tile drawing
					// The function pointer can automatically dispatch
					// to the required behavior avoiding this case statement and
					// cached_tile->bPaintOK conditionals.
						if( !cached_tile->bPaintOK )
						{
							paint_tile( tilemap, cached_tile );
						}
						pColorData = (DATA_TYPE *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
						DrawClippedOpaqueTile( pColorData, sx0, sy0, TILE_WIDTH, TILE_HEIGHT );
						// note that it's faster not to bother with clipping.
						// We are drawing rectangular chunks of tiles, so only the edges are
						// potentially clipped; interior tiles could be drawn with a faster
						// routine that doesn't check blit bounds.
						break;

					case PARTIALLY_TRANSPARENT:
						if( !cached_tile->bPaintOK )
						{
							paint_tile( tilemap, cached_tile );
						}
						pMask++; /* skip past mask type */
						pColorData = (DATA_TYPE *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
						DrawClippedTranspTile( pColorData, sx0, sy0, TILE_WIDTH, TILE_HEIGHT, pMask );
						break;
					} /* mask type */
				} /* tile priority */
				sx0 += TILE_WIDTH;
			} /* col */
			sy0 += TILE_HEIGHT;
		} /* row */
	} /* clip */
})

#if DEPTH >= 16
DECLARE( draw_alpha, (struct tilemap *tilemap, INT32 xpos, INT32 ypos),
{
	UINT32 row;
	UINT32 col;
	int indx;
	struct cached_tile *cached_tile;
	int sx0;
	int sy0;
	DATA_TYPE *pColorData;
	const UINT8 *pMask;
	int mask_offset;

	/* screen coordinates to tilemap coordinates */
	int x0 = blit.clip_left - xpos;
	int y0 = blit.clip_top - ypos;
	int x1 = blit.clip_right - xpos;
	int y1 = blit.clip_bottom - ypos;

	/* clip to tilemap bounds */
	if( x0<0 ) x0 = 0;
	if( y0<0 ) y0 = 0;
	if( x1>tilemap->cached_width ) x1 = tilemap->cached_width;
	if( y1>tilemap->cached_height ) y1 = tilemap->cached_height;

	if( x1>x0 && y1>y0 )
	{ /* only proceed if not totally clipped */
		mask_offset = blit.mask_offset;

		x0 = ((UINT32)x0)/TILE_WIDTH;
		y0 = ((UINT32)y0)/TILE_HEIGHT;
		x1 = ((UINT32)(x1+TILE_WIDTH-1))/TILE_WIDTH;
		y1 = ((UINT32)(y1+TILE_HEIGHT-1))/TILE_HEIGHT;

		sy0 = ypos+y0*TILE_HEIGHT;
		for( row=y0; row<y1; row++ )
		{
			sx0 = xpos+x0*TILE_WIDTH;
			indx = row*tilemap->num_cached_cols+x0;

			for( col=x0; col<x1; col++ )
			{
				cached_tile = tilemap->cached_tile[indx++];
				if( cached_tile->tile_info.priority == blit.tile_priority )
				{
					pMask = mask_offset+(UINT8 *)cached_tile;
					/* first byte is mask type; subsequant bytes are a packed bitmask */
					switch( *pMask )
					{
					case WHOLLY_OPAQUE:
					// TBA: use function pointer for tile drawing
					// The function pointer can automatically dispatch
					// to the required behavior avoiding this case statement and
					// cached_tile->bPaintOK conditionals.
						if( !cached_tile->bPaintOK )
						{
							paint_tile( tilemap, cached_tile );
						}
						pColorData = (DATA_TYPE *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
						DrawClippedAlphaOpaqueTile( pColorData, sx0, sy0, TILE_WIDTH, TILE_HEIGHT );
						// note that it's faster not to bother with clipping.
						// We are drawing rectangular chunks of tiles, so only the edges are
						// potentially clipped; interior tiles could be drawn with a faster
						// routine that doesn't check blit bounds.
						break;

					case PARTIALLY_TRANSPARENT:
						if( !cached_tile->bPaintOK )
						{
							paint_tile( tilemap, cached_tile );
						}
						pMask++; /* skip past mask type */
						pColorData = (DATA_TYPE *)(sizeof(struct cached_tile) + (UINT8 *)cached_tile);
						DrawClippedAlphaTranspTile( pColorData, sx0, sy0, TILE_WIDTH, TILE_HEIGHT, pMask );
						break;
					} /* mask type */
				} /* tile priority */
				sx0 += TILE_WIDTH;
			} /* col */
			sy0 += TILE_HEIGHT;
		} /* row */
	} /* clip */
})
#endif

/* we don't need separate 8, 16 and 32 bit versions of visibility marking */
#if DEPTH == 8
DECLARE( mark_visible, (struct tilemap *tilemap, INT32 xpos, INT32 ypos),
{
	/* screen coordinates to tilemap coordinates */
	int x0 = blit.clip_left - xpos;
	int y0 = blit.clip_top - ypos;
	int x1 = blit.clip_right - xpos;
	int y1 = blit.clip_bottom - ypos;

	/* clip to tilemap bounds */
	if( x0<0 ) x0 = 0;
	if( y0<0 ) y0 = 0;
	if( x1>tilemap->cached_width ) x1 = tilemap->cached_width;
	if( y1>tilemap->cached_height ) y1 = tilemap->cached_height;

	if( x1>x0 && y1>y0 )
	{ /* only proceed if not totally clipped */
		x0 = ((UINT32)x0)/TILE_WIDTH;
		y0 = ((UINT32)y0)/TILE_HEIGHT;
		x1 = ((UINT32)(x1+TILE_WIDTH-1))/TILE_WIDTH;
		y1 = ((UINT32)(y1+TILE_HEIGHT-1))/TILE_HEIGHT;

		x1 -= x0; /* x1: num visible cols */
		x0 += tilemap->num_cached_cols*y0; /* x0:indx */

		y1 -= y0; /* y1: num visible rows */
		while( y1-- )
		{
			bitarray_set_range( tilemap->visible, x0, x1 );
			x0 += tilemap->num_cached_cols; /* next row */
		}
	}
})
#endif

#undef TILE_WIDTH
#undef TILE_HEIGHT
#undef DECLARE

#endif /* DECLARE */

#endif
