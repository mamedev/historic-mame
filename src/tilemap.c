#include "driver.h"
#include "tilemap.h"

#define USE_FATMASKS 1

/*
	Usage Notes:

	When the videoram for a tile changes, call tilemap_mark_tile_dirty
	with the appropriate tile_index.

	In the video driver, follow these steps:

	1)	set each tilemap's scroll registers

	2)	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors);
		call tilemap_update for each tilemap.
		call tilemap_mark_palette
		mark the colors used by sprites.

	3)	call palette recalc.  If the palette manager has compressed the palette,
		call tilemap_mark_all_pixels_dirty for each tilemap.

	4)	call tilemap_render for each tilemap.

	5)	call tilemap_draw to draw the tilemaps to the screen, from back to front

********************************************************************************/

static int orientation, screen_width, screen_height;
struct tile_info tile_info;
static int *pen_refcount;

#define SWAP(X,Y) {int temp=X; X=Y; Y=temp; }


void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip ){
	int left = clip->min_x;
	int top = clip->min_y;
	int right = clip->max_x+1;
	int bottom = clip->max_y+1;

	tilemap->clip = *clip;

	if( orientation & ORIENTATION_SWAP_XY ){
		SWAP(left,top)
		SWAP(right,bottom)
	}
	if( orientation & ORIENTATION_FLIP_X ){
		SWAP(left,right)
		left = screen_width-left;
		right = screen_width-right;
	}
	if( orientation & ORIENTATION_FLIP_Y ){
		SWAP(top,bottom)
		top = screen_width-top;
		bottom = screen_width-bottom;
	}

	tilemap->clip_left = left;
	tilemap->clip_right = right;
	tilemap->clip_top = top;
	tilemap->clip_bottom = bottom;
}

static void use_color( int base_pen, unsigned int pen_usage ){
	while( pen_usage ){
		if( pen_usage&1 ) pen_refcount[base_pen]++;
		base_pen++;
		pen_usage >>= 1;
	}
}

static void unuse_color( int base_pen, unsigned int pen_usage ){
	while( pen_usage ){
		if( pen_usage&1 ) pen_refcount[base_pen]--;
		base_pen++;
		pen_usage >>= 1;
	}
}

int tilemap_start( void ){
	int total_colors = Machine->drv->total_colors;
	orientation = Machine->orientation;
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;
	pen_refcount = (int *)malloc(sizeof(int)*total_colors);
	if( pen_refcount ){
		int pen;
		for( pen=0; pen<total_colors; pen++ ) pen_refcount[pen] = 0;
		return 0;
	}
	return 1; /* error */
}

void tilemap_stop( void ){
	free( pen_refcount );
}

void tilemap_mark_palette( void ){
	if( palette_used_colors ){ /* this needn't be called for static palette games */
		int pen;
		for( pen=0; pen<Machine->drv->total_colors; pen++ ){
			if( pen_refcount[pen] ) palette_used_colors[pen] = PALETTE_COLOR_USED;
		}
	}
}

/***********************************************************************************/

#if USE_FATMASKS

#define MASKROWBYTES(W) W
#define FATMASK_OPAQUE			0x00
#define FATMASK_TRANSPARENT		0xFF
/*
	memcpy0 is a low level renderer, analogous to memcpy, but doing masking.
	fatmask has one byte per source pixel.
*/
static void memcpy0( unsigned char *dest, const unsigned char *source,
				const unsigned char *fatmask, int numbytes ){
	const unsigned char *finish = source+numbytes;
	const unsigned char *finishw = finish-(numbytes&0x3);

	while( source<finishw ){
		int m = *(int *)fatmask;
		if( m ){
			*(int *)dest = ( *(int *)dest & m ) | ( *(int *)source & (~m) );
		}
		else{
			*(int *)dest = *(int *)source;
		}
		dest += 4; source += 4; fatmask += 4;
	}

	while( source<finish ){
		if( *fatmask==0 ) *dest = *source;
		source++; dest++;  fatmask++;
	}
}
#else
#define MASKROWBYTES(W) (((W)+7)/8)

static void memcpybitmask( unsigned char *dest, const unsigned char *source,
				const unsigned char *bitmask, int count ){
	for(;;){
		unsigned char data = *bitmask++;
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
#endif

#define TILE_TRANSPARENT	0
#define TILE_MASKED			1
#define TILE_OPAQUE			2

/* the following parameters are constant across tilemap_draw calls */
static struct {
	int clip_left, clip_top, clip_right, clip_bottom;
	int source_width, source_height;

	int dest_line_offset,source_line_offset,mask_line_offset;
	int dest_row_offset,source_row_offset,mask_row_offset;
	struct osd_bitmap *screen, *pixmap, *bitmask;

	unsigned char **mask_data_row;
	char **priority_data_row, **visible_row;
	unsigned char priority;
} blit;

#define TILE_WIDTH	8
#define TILE_HEIGHT	8
static void draw8x8( int xpos, int ypos )
#include "tmapdraw.c"
static void mark_visible8x8( int xpos, int ypos )
#include "tmapmvis.c"
#undef TILE_HEIGHT
#undef TILE_WIDTH

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
static void draw16x16( int xpos, int ypos )
#include "tmapdraw.c"
static void mark_visible16x16( int xpos, int ypos )
#include "tmapmvis.c"
#undef TILE_HEIGHT
#undef TILE_WIDTH

#define TILE_WIDTH	32
#define TILE_HEIGHT	32
static void draw32x32( int xpos, int ypos )
#include "tmapdraw.c"
static void mark_visible32x32( int xpos, int ypos )
#include "tmapmvis.c"
#undef TILE_HEIGHT
#undef TILE_WIDTH

/***********************************************************************************/

static void dispose_tile_info( struct tilemap *tilemap ){
	if( tilemap->pendata ) free( tilemap->pendata );
	if( tilemap->paldata ) free( tilemap->paldata );
	if( tilemap->pen_usage ) free( tilemap->pen_usage );
	if( tilemap->priority ) free( tilemap->priority );
	if( tilemap->visible ) free( tilemap->visible );
	if( tilemap->dirty_vram ) free( tilemap->dirty_vram );
	if( tilemap->dirty_pixels ) free( tilemap->dirty_pixels );
	if( tilemap->flags ) free( tilemap->flags );
	if( tilemap->priority_row ) free( tilemap->priority_row );
	if( tilemap->visible_row ) free( tilemap->visible_row );
}

static int create_tile_info( struct tilemap *tilemap ){
	int num_tiles = tilemap->num_tiles;
	int num_cols = tilemap->num_cols;
	int num_rows = tilemap->num_rows;

	tilemap->pendata = malloc( sizeof( unsigned char *)*num_tiles );
	tilemap->paldata = malloc( sizeof( unsigned short *)*num_tiles );
	tilemap->pen_usage = malloc( sizeof( unsigned int )*num_tiles );

	tilemap->priority = malloc( num_tiles );
	tilemap->visible = malloc( num_tiles );
	tilemap->dirty_vram = malloc( num_tiles );
	tilemap->dirty_pixels = malloc( num_tiles );
	tilemap->flags = malloc( num_tiles );

	tilemap->priority_row = malloc( sizeof(char *)*num_rows );
	tilemap->visible_row = malloc( sizeof(char *)*num_rows );

	if( tilemap->pendata && tilemap->paldata && tilemap->pen_usage &&
		tilemap->priority && tilemap->visible &&
		tilemap->dirty_vram && tilemap->dirty_pixels &&
		tilemap->flags &&
		tilemap->priority_row && tilemap->visible_row )
	{
		int tile_index,row;

		for( row=0; row<num_rows; row++ ){
			tilemap->priority_row[row] = tilemap->priority+num_cols*row;
			tilemap->visible_row[row] = tilemap->visible+num_cols*row;
		}

		for( tile_index=0; tile_index<num_tiles; tile_index++ ){
			tilemap->paldata[tile_index] = Machine->colortable;
			tilemap->pen_usage[tile_index] = 0;
		}

		memset( tilemap->priority, 0, num_tiles );
		memset( tilemap->visible, 0, num_tiles );
		memset( tilemap->dirty_vram, 1, num_tiles );
		memset( tilemap->dirty_pixels, 1, num_tiles );

		return 1; /* done */
	}
	dispose_tile_info( tilemap );
	return 0; /* error */
}

static int create_pixmap( struct tilemap *tilemap ){
	int error = 0;

	tilemap->scrolled = 1;

	if( tilemap->scroll_rows ){
		tilemap->rowscroll = malloc(sizeof(int)*tilemap->scroll_rows );
		if( tilemap->rowscroll ){
			memset( tilemap->rowscroll, 0, sizeof(int)*tilemap->scroll_rows );
		}
		else {
			error = 1;
		}
	}
	else {
		tilemap->rowscroll = 0;
	}

	if( tilemap->scroll_cols ){
		tilemap->colscroll = malloc(sizeof(int)*tilemap->scroll_cols );
		if( tilemap->colscroll ){
			memset( tilemap->colscroll, 0, sizeof(int)*tilemap->scroll_cols );
		}
		else {
			error = 1;
		}
	}
	else {
		tilemap->colscroll = 0;
	}

	if( error ==0 ){
		tilemap->pixmap = osd_create_bitmap( tilemap->width, tilemap->height );
		if( tilemap->pixmap ){
			tilemap->pixmap_line_offset = tilemap->pixmap->line[1] - tilemap->pixmap->line[0];
			return 1; /* done */
		}
		if( tilemap->colscroll ) free( tilemap->colscroll );
		if( tilemap->rowscroll ) free( tilemap->rowscroll );
	}
	return 0; /* error */
}

static void dispose_pixmap( struct tilemap *tilemap ){
	osd_free_bitmap( tilemap->pixmap );
	if( tilemap->colscroll ) free( tilemap->colscroll );
	if( tilemap->rowscroll ) free( tilemap->rowscroll );
}

static unsigned char **new_mask_data_table( unsigned char *mask_data, int num_cols, int num_rows ){
	unsigned char **mask_data_row = malloc(num_rows * sizeof(unsigned char *));
	if( mask_data_row ){
		int row;
		for( row = 0; row<num_rows; row++ ) mask_data_row[row] = mask_data + num_cols*row;
	}
	return mask_data_row;
}

static int create_fg_mask( struct tilemap *tilemap ){
	if( tilemap->type==0 ) return 1;

	tilemap->fg_mask_data = malloc( tilemap->num_tiles );
	if( tilemap->fg_mask_data ){
		tilemap->fg_mask_data_row = new_mask_data_table( tilemap->fg_mask_data, tilemap->num_cols, tilemap->num_rows );
		if( tilemap->fg_mask_data_row ){
			tilemap->fg_mask = osd_create_bitmap( MASKROWBYTES(tilemap->width), tilemap->height );
			if( tilemap->fg_mask ){
				tilemap->fg_mask_line_offset = tilemap->fg_mask->line[1] - tilemap->fg_mask->line[0];
				return 1; /* done */
			}
			free( tilemap->fg_mask_data_row );
		}
		free( tilemap->fg_mask_data );
	}
	return 0; /* error */
}

static int create_bg_mask( struct tilemap *tilemap ){
	if( (tilemap->type & TILEMAP_SPLIT)==0 ) return 1;

	tilemap->bg_mask_data = malloc( tilemap->num_tiles );
	if( tilemap->bg_mask_data ){
		tilemap->bg_mask_data_row = new_mask_data_table( tilemap->bg_mask_data, tilemap->num_cols, tilemap->num_rows );
		if( tilemap->bg_mask_data_row ){
			tilemap->bg_mask = osd_create_bitmap( MASKROWBYTES(tilemap->width), tilemap->height );
			if( tilemap->bg_mask ){
				tilemap->bg_mask_line_offset = tilemap->bg_mask->line[1] - tilemap->bg_mask->line[0];
				return 1; /* done */
			}
			free( tilemap->bg_mask_data_row );
		}
		free( tilemap->bg_mask_data );
	}
	return 0; /* error */
}

static void dispose_fg_mask( struct tilemap *tilemap ){
	if( tilemap->type!=0 ){
		osd_free_bitmap( tilemap->fg_mask );
		free( tilemap->fg_mask_data_row );
		free( tilemap->fg_mask_data );
	}
}

static void dispose_bg_mask( struct tilemap *tilemap ){
	if( tilemap->type & TILEMAP_SPLIT ){
		osd_free_bitmap( tilemap->bg_mask );
		free( tilemap->bg_mask_data_row );
		free( tilemap->bg_mask_data );
	}
}

/***********************************************************************************/

struct tilemap *tilemap_create(
		int type,
		int tile_width, int tile_height,
		int num_cols, int num_rows,
		int scroll_rows, int scroll_cols ){

	struct tilemap *tilemap = (struct tilemap *)malloc( sizeof( struct tilemap ) );
	if( tilemap ){
		if( orientation & ORIENTATION_SWAP_XY ){
			SWAP( tile_width, tile_height )
			SWAP( num_cols,num_rows )
			SWAP( scroll_rows, scroll_cols )
		}

		memset( tilemap, 0, sizeof( struct tilemap ) );
		{
			const struct rectangle *clip = &Machine->drv->visible_area;
			tilemap_set_clip( tilemap, clip );
		}

		if( tile_width==8 ){
			if( tile_height==8 ){
				tilemap->mark_visible = mark_visible8x8;
				tilemap->draw = draw8x8;
			}
		}
		else if( tile_width==16 ){
			if( tile_height==16 ){
				tilemap->mark_visible = mark_visible16x16;
				tilemap->draw = draw16x16;
			}
		}
		else if( tile_width==32 ){
			if( tile_height==32 ){
				tilemap->mark_visible = mark_visible32x32;
				tilemap->draw = draw32x32;
			}
		}

		if( tilemap->mark_visible && tilemap->draw ){
			tilemap->type = type;

			tilemap->tile_width = tile_width;
			tilemap->tile_height = tile_height;
			tilemap->width = tile_width*num_cols;
			tilemap->height = tile_height*num_rows;

			tilemap->num_rows = num_rows;
			tilemap->num_cols = num_cols;
			tilemap->num_tiles = num_cols*num_rows;

			tilemap->scroll_rows = scroll_rows;
			tilemap->scroll_cols = scroll_cols;

			tilemap->transparent_pen = -1; /* default (this is supplied by video driver) */

			if( create_pixmap( tilemap ) ){
				if( create_fg_mask( tilemap ) ){
					if( create_bg_mask( tilemap ) ){
						if( create_tile_info( tilemap ) ){
							return tilemap;
						}
						dispose_bg_mask( tilemap );
					}
					dispose_fg_mask( tilemap );
				}
				dispose_pixmap( tilemap );
			}
		}
		free( tilemap );
	}
	return 0; /* error */
}

void tilemap_dispose( struct tilemap *tilemap ){
	if( tilemap ){
		dispose_tile_info( tilemap );
		dispose_bg_mask( tilemap );
		dispose_fg_mask( tilemap );
		dispose_pixmap( tilemap );
		free( tilemap );
	}
}

/***********************************************************************************/

static void draw_tile(
		struct osd_bitmap *pixmap,
		int col, int row, int tile_width, int tile_height,
		const unsigned char *pendata, const unsigned short *paldata,
		unsigned char flags )
{
	int x, sx = tile_width*col;
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
			unsigned char *dest  = pixmap->line[sy]+sx;
			for( x=tile_width; x>=0; x-- ) dest[x] = paldata[*pendata++];
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
			unsigned char *dest  = pixmap->line[sy]+sx;
			for( x=0; x<tile_width; x++ ) dest[x] = paldata[*pendata++];
		}
	}
}

static void draw_mask(
		struct osd_bitmap *mask,
		int col, int row, int tile_width, int tile_height,
		const unsigned char *pendata, unsigned int transmask,
		unsigned char flags )
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
#if USE_FATMASKS
			unsigned char *mask_dest  = mask->line[sy]+sx;
			for( x=tile_width; x>=0; x-- ){
				unsigned char p = *pendata++;
				mask_dest[x] = ((1<<p)&transmask)?FATMASK_TRANSPARENT:FATMASK_OPAQUE;
			}
#else
			unsigned char *mask_dest  = mask->line[sy]+sx/8;
			for( x=tile_width/8; x>=0; x-- ){
				unsigned char data = 0;
				for( bit=0; bit<8; bit++ ){
					unsigned char p = *pendata++;
					data = (data>>1)|(((1<<p)&transmask)?0x00:0x80);
				}
				mask_dest[x] = data;
			}
#endif
		}
	}
	else {
		for( sy=y1; sy!=y2; sy+=dy ){
#if USE_FATMASKS
			unsigned char *mask_dest  = mask->line[sy]+sx;
			for( x=0; x<tile_width; x++ ){
				unsigned char p = *pendata++;
				mask_dest[x] = ((1<<p)&transmask)?FATMASK_TRANSPARENT:FATMASK_OPAQUE;
			}
#else
			unsigned char *mask_dest  = mask->line[sy]+sx/8;
			for( x=0; x<tile_width/8; x++ ){
				unsigned char data = 0;
				for( bit=0; bit<8; bit++ ){
					unsigned char p = *pendata++;
					data = (data<<1)|(((1<<p)&transmask)?0x00:0x01);
				}
				mask_dest[x] = data;
			}
#endif
		}
	}
}

/***********************************************************************************/

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int col, int row ){
	if( orientation & ORIENTATION_SWAP_XY ) SWAP(col,row)
	if( orientation & ORIENTATION_FLIP_X ) col = tilemap->num_cols-1-col;
	if( orientation & ORIENTATION_FLIP_Y ) row = tilemap->num_rows-1-row;

	tilemap->dirty_vram[row*tilemap->num_cols + col] = 1;
}

void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap ){
	memset( tilemap->dirty_pixels, 1, tilemap->num_tiles );
}

/***********************************************************************************/

void tilemap_render( struct tilemap *tilemap ){
	int type = tilemap->type;
	int transparent_pen = tilemap->transparent_pen;
	unsigned int *transmask = tilemap->transmask;

	int tile_width = tilemap->tile_width;
	int tile_height = tilemap->tile_height;

	char *dirty_pixels = tilemap->dirty_pixels;
	char *visible = tilemap->visible;
	int tile_index = 0;
	int row,col;

	for( row=0; row<tilemap->num_rows; row++ ){
		for( col=0; col<tilemap->num_cols; col++ ){
			if( dirty_pixels[tile_index] && visible[tile_index] ){
				unsigned int pen_usage = tilemap->pen_usage[tile_index];
				const unsigned char *pendata = tilemap->pendata[tile_index];
				unsigned char flags = tilemap->flags[tile_index];

				draw_tile(
					tilemap->pixmap,
					col, row, tile_width, tile_height,
					pendata,
					tilemap->paldata[tile_index],
					flags );

				if( type & TILEMAP_SPLIT ){
					int pen_mask = (transparent_pen<0)?0:(1<<transparent_pen);
					if( pen_mask == pen_usage ){ /* totally transparent */
						tilemap->fg_mask_data_row[row][col] = TILE_TRANSPARENT;
						tilemap->bg_mask_data_row[row][col] = TILE_TRANSPARENT;
					}
					else {
						unsigned int fg_transmask = transmask[(flags>>2)&3];
						unsigned int bg_transmask = (~fg_transmask)|pen_mask;
						if( (pen_usage & fg_transmask)==0 ){ /* foreground totally opaque */
							tilemap->fg_mask_data_row[row][col] = TILE_OPAQUE;
							tilemap->bg_mask_data_row[row][col] = TILE_TRANSPARENT;
						}
						else if( (pen_usage & bg_transmask)==0 ){ /* background totally opaque */
							tilemap->fg_mask_data_row[row][col] = TILE_TRANSPARENT;
							tilemap->bg_mask_data_row[row][col] = TILE_OPAQUE;
						}
						else if( (pen_usage & ~bg_transmask)==0 ){ /* background transparent */
							draw_mask( tilemap->fg_mask,
								col, row, tile_width, tile_height,
								pendata, fg_transmask, flags );
							tilemap->fg_mask_data_row[row][col] = TILE_MASKED;
							tilemap->bg_mask_data_row[row][col] = TILE_TRANSPARENT;
						}
						else if( (pen_usage & ~fg_transmask)==0 ){ /* foreground transparent */
							draw_mask( tilemap->bg_mask,
								col, row, tile_width, tile_height,
								pendata, bg_transmask, flags );
							tilemap->fg_mask_data_row[row][col] = TILE_TRANSPARENT;
							tilemap->bg_mask_data_row[row][col] = TILE_MASKED;
						}
						else { /* split tile - opacity in both foreground and background */
							draw_mask( tilemap->fg_mask,
								col, row, tile_width, tile_height,
								pendata, fg_transmask, flags );
							draw_mask( tilemap->bg_mask,
								col, row, tile_width, tile_height,
								pendata, bg_transmask, flags );
							tilemap->fg_mask_data_row[row][col] = TILE_MASKED;
							tilemap->bg_mask_data_row[row][col] = TILE_MASKED;
						}
					}
			 	}
			 	else if( type==TILEMAP_TRANSPARENT ){
			 		unsigned int fg_transmask = 1 << transparent_pen;
			 	 	if( pen_usage == fg_transmask ){
						tilemap->fg_mask_data_row[row][col] = TILE_TRANSPARENT;
					}
					else if( pen_usage & fg_transmask ){
						draw_mask( tilemap->fg_mask,
							col, row, tile_width, tile_height,
							pendata, fg_transmask, flags );
						tilemap->fg_mask_data_row[row][col] = TILE_MASKED;
					}
					else {
						tilemap->fg_mask_data_row[row][col] = TILE_OPAQUE;
					}
			 	}
				dirty_pixels[tile_index] = 0;
			}
			tile_index++;
		} /* next col */
	} /* next row */
}

/***********************************************************************************/

void tilemap_draw( struct osd_bitmap *dest, struct tilemap *tilemap, int priority ){
	void (*draw)( int, int ) = tilemap->draw;

	int rows = tilemap->scroll_rows;
	const int *rowscroll = tilemap->rowscroll;
	int cols = tilemap->scroll_cols;
	const int *colscroll = tilemap->colscroll;

	int left = tilemap->clip_left;
	int right = tilemap->clip_right;
	int top = tilemap->clip_top;
	int bottom = tilemap->clip_bottom;

	int tile_height = tilemap->tile_height;

	if( tilemap->type==0 ){
		copyscrollbitmap(dest,tilemap->pixmap,rows,rowscroll,cols,colscroll,&tilemap->clip,TRANSPARENCY_NONE,0);
		return;
	}

	blit.screen = dest;
	blit.dest_line_offset = dest->line[1] - dest->line[0];
	blit.dest_row_offset = tile_height*blit.dest_line_offset;

	blit.pixmap = tilemap->pixmap;
	blit.source_line_offset = tilemap->pixmap_line_offset;
	blit.source_row_offset = tile_height*blit.source_line_offset;

	if( priority&TILEMAP_BACK ){
		blit.bitmask = tilemap->bg_mask;
		blit.mask_line_offset = tilemap->bg_mask_line_offset;
		blit.mask_data_row = tilemap->bg_mask_data_row;
	}
	else {
		blit.bitmask = tilemap->fg_mask;
		blit.mask_line_offset = tilemap->fg_mask_line_offset;
		blit.mask_data_row = tilemap->fg_mask_data_row;
	}

	blit.priority_data_row = tilemap->priority_row;
	blit.mask_row_offset = tile_height*blit.mask_line_offset;
	blit.source_width = tilemap->width;
	blit.source_height = tilemap->height;
	blit.priority = priority&0xf;

	if( rows == 0 && cols == 0 ){ /* no scrolling */
 		blit.clip_left = left;
 		blit.clip_top = top;
 		blit.clip_right = right;
 		blit.clip_bottom = bottom;

		draw( 0,0 );
	}
	else if( rows == 0 ){ /* scrolling columns */
		int col,colwidth;

		colwidth = blit.source_width / cols;

		blit.clip_top = top;
		blit.clip_bottom = bottom;

		col = 0;
		while( col < cols ){
			int cons,scroll;

 			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while( col + cons < cols &&	colscroll[col + cons] == scroll ) cons++;

			if (scroll < 0) scroll = blit.source_height - (-scroll) % blit.source_height;
			else scroll %= blit.source_height;

			blit.clip_left = col * colwidth;
			if (blit.clip_left < left) blit.clip_left = left;
			blit.clip_right = (col + cons) * colwidth;
			if (blit.clip_right > right) blit.clip_right = right;

			draw( 0,scroll );
			draw( 0,scroll - blit.source_height );

			col += cons;
		}
	}
	else if( cols == 0 ){ /* scrolling rows */
		int row,rowheight;

		rowheight = blit.source_height / rows;

		blit.clip_left = left;
		blit.clip_right = right;

		row = 0;
		while( row < rows ){
			int cons,scroll;

			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while( row + cons < rows &&	rowscroll[row + cons] == scroll ) cons++;

			if (scroll < 0) scroll = blit.source_width - (-scroll) % blit.source_width;
			else scroll %= blit.source_width;

			blit.clip_top = row * rowheight;
			if (blit.clip_top < top) blit.clip_top = top;
			blit.clip_bottom = (row + cons) * rowheight;
			if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

			draw( scroll,0 );
			draw( scroll - blit.source_width,0 );

			row += cons;
		}
	}
	else if( rows == 1 && cols == 1 ){ /* XY scrolling playfield */
		int scrollx,scrolly;

		if (rowscroll[0] < 0) scrollx = blit.source_width - (-rowscroll[0]) % blit.source_width;
		else scrollx = rowscroll[0] % blit.source_width;

		if (colscroll[0] < 0) scrolly = blit.source_height - (-colscroll[0]) % blit.source_height;
		else scrolly = colscroll[0] % blit.source_height;

 		blit.clip_left = left;
 		blit.clip_top = top;
 		blit.clip_right = right;
 		blit.clip_bottom = bottom;

		draw( scrollx,scrolly );
		draw( scrollx,scrolly - blit.source_height );
		draw( scrollx - blit.source_width,scrolly );
		draw( scrollx - blit.source_width,scrolly - blit.source_height );
	}
	else if( rows == 1 ){ /* scrolling columns + horizontal scroll */
		int col,colwidth;
		int scrollx;

		if (rowscroll[0] < 0) scrollx = blit.source_width - (-rowscroll[0]) % blit.source_width;
		else scrollx = rowscroll[0] % blit.source_width;

		colwidth = blit.source_width / cols;

		blit.clip_top = top;
		blit.clip_bottom = bottom;

		col = 0;
		while( col < cols ){
			int cons,scroll;

 			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while( col + cons < cols &&	colscroll[col + cons] == scroll ) cons++;

			if (scroll < 0) scroll = blit.source_height - (-scroll) % blit.source_height;
			else scroll %= blit.source_height;

			blit.clip_left = col * colwidth + scrollx;
			if (blit.clip_left < left) blit.clip_left = left;
			blit.clip_right = (col + cons) * colwidth + scrollx;
			if (blit.clip_right > right) blit.clip_right = right;

			draw( scrollx,scroll );
			draw( scrollx,scroll - blit.source_height );

			blit.clip_left = col * colwidth + scrollx - blit.source_width;
			if (blit.clip_left < left) blit.clip_left = left;
			blit.clip_right = (col + cons) * colwidth + scrollx - blit.source_width;
			if (blit.clip_right > right) blit.clip_right = right;

			draw( scrollx - blit.source_width,scroll );
			draw( scrollx - blit.source_width,scroll - blit.source_height );

			col += cons;
		}
	}
	else if( cols == 1 ){ /* scrolling rows + vertical scroll */
		int row,rowheight;
		int scrolly;

		if (colscroll[0] < 0) scrolly = blit.source_height - (-colscroll[0]) % blit.source_height;
		else scrolly = colscroll[0] % blit.source_height;

		rowheight = blit.source_height / rows;

		blit.clip_left = left;
		blit.clip_right = right;

		row = 0;
		while( row < rows ){
			int cons,scroll;

			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while (row + cons < rows &&	rowscroll[row + cons] == scroll) cons++;

			if (scroll < 0) scroll = blit.source_width - (-scroll) % blit.source_width;
			else scroll %= blit.source_width;

			blit.clip_top = row * rowheight + scrolly;
			if (blit.clip_top < top) blit.clip_top = top;
			blit.clip_bottom = (row + cons) * rowheight + scrolly;
			if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

			draw( scroll,scrolly );
			draw( scroll - blit.source_width,scrolly );

			blit.clip_top = row * rowheight + scrolly - blit.source_height;
			if (blit.clip_top < top) blit.clip_top = top;
			blit.clip_bottom = (row + cons) * rowheight + scrolly - blit.source_height;
			if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

			draw( scroll,scrolly - blit.source_height );
			draw( scroll - blit.source_width,scrolly - blit.source_height );

			row += cons;
		}
	}
}

void tilemap_update( struct tilemap *tilemap ){
	if( tilemap->scrolled ){
		void (*mark_visible)( int, int ) = tilemap->mark_visible;

		int rows = tilemap->scroll_rows;
		const int *rowscroll = tilemap->rowscroll;
		int cols = tilemap->scroll_cols;
		const int *colscroll = tilemap->colscroll;

		int left = tilemap->clip_left;
		int right = tilemap->clip_right;
		int top = tilemap->clip_top;
		int bottom = tilemap->clip_bottom;

		blit.source_width = tilemap->width;
		blit.source_height = tilemap->height;
		blit.visible_row = tilemap->visible_row;

		memset( tilemap->visible, 0, tilemap->num_tiles );

		if( rows == 0 && cols == 0 ){ /* no scrolling */
	 		blit.clip_left = left;
	 		blit.clip_top = top;
	 		blit.clip_right = right;
	 		blit.clip_bottom = bottom;

			mark_visible( 0,0 );
		}
		else if( rows == 0 ){ /* scrolling columns */
			int col,colwidth;

			colwidth = blit.source_width / cols;

			blit.clip_top = top;
			blit.clip_bottom = bottom;

			col = 0;
			while( col < cols ){
				int cons,scroll;

	 			/* count consecutive columns scrolled by the same amount */
				scroll = colscroll[col];
				cons = 1;
				while( col + cons < cols &&	colscroll[col + cons] == scroll ) cons++;

				if (scroll < 0) scroll = blit.source_height - (-scroll) % blit.source_height;
				else scroll %= blit.source_height;

				blit.clip_left = col * colwidth;
				if (blit.clip_left < left) blit.clip_left = left;
				blit.clip_right = (col + cons) * colwidth;
				if (blit.clip_right > right) blit.clip_right = right;

				mark_visible( 0,scroll );
				mark_visible( 0,scroll - blit.source_height );

				col += cons;
			}
		}
		else if( cols == 0 ){ /* scrolling rows */
			int row,rowheight;

			rowheight = blit.source_height / rows;

			blit.clip_left = left;
			blit.clip_right = right;

			row = 0;
			while( row < rows ){
				int cons,scroll;

				/* count consecutive rows scrolled by the same amount */
				scroll = rowscroll[row];
				cons = 1;
				while( row + cons < rows &&	rowscroll[row + cons] == scroll ) cons++;

				if (scroll < 0) scroll = blit.source_width - (-scroll) % blit.source_width;
				else scroll %= blit.source_width;

				blit.clip_top = row * rowheight;
				if (blit.clip_top < top) blit.clip_top = top;
				blit.clip_bottom = (row + cons) * rowheight;
				if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

				mark_visible( scroll,0 );
				mark_visible( scroll - blit.source_width,0 );

				row += cons;
			}
		}
		else if( rows == 1 && cols == 1 ){ /* XY scrolling playfield */
			int scrollx,scrolly;

			if (rowscroll[0] < 0) scrollx = blit.source_width - (-rowscroll[0]) % blit.source_width;
			else scrollx = rowscroll[0] % blit.source_width;

			if (colscroll[0] < 0) scrolly = blit.source_height - (-colscroll[0]) % blit.source_height;
			else scrolly = colscroll[0] % blit.source_height;

	 		blit.clip_left = left;
	 		blit.clip_top = top;
	 		blit.clip_right = right;
	 		blit.clip_bottom = bottom;

			mark_visible( scrollx,scrolly );
			mark_visible( scrollx,scrolly - blit.source_height );
			mark_visible( scrollx - blit.source_width,scrolly );
			mark_visible( scrollx - blit.source_width,scrolly - blit.source_height );
		}
		else if( rows == 1 ){ /* scrolling columns + horizontal scroll */
			int col,colwidth;
			int scrollx;

			if (rowscroll[0] < 0) scrollx = blit.source_width - (-rowscroll[0]) % blit.source_width;
			else scrollx = rowscroll[0] % blit.source_width;

			colwidth = blit.source_width / cols;

			blit.clip_top = top;
			blit.clip_bottom = bottom;

			col = 0;
			while( col < cols ){
				int cons,scroll;

	 			/* count consecutive columns scrolled by the same amount */
				scroll = colscroll[col];
				cons = 1;
				while( col + cons < cols &&	colscroll[col + cons] == scroll ) cons++;

				if (scroll < 0) scroll = blit.source_height - (-scroll) % blit.source_height;
				else scroll %= blit.source_height;

				blit.clip_left = col * colwidth + scrollx;
				if (blit.clip_left < left) blit.clip_left = left;
				blit.clip_right = (col + cons) * colwidth + scrollx;
				if (blit.clip_right > right) blit.clip_right = right;

				mark_visible( scrollx,scroll );
				mark_visible( scrollx,scroll - blit.source_height );

				blit.clip_left = col * colwidth + scrollx - blit.source_width;
				if (blit.clip_left < left) blit.clip_left = left;
				blit.clip_right = (col + cons) * colwidth + scrollx - blit.source_width;
				if (blit.clip_right > right) blit.clip_right = right;

				mark_visible( scrollx - blit.source_width,scroll );
				mark_visible( scrollx - blit.source_width,scroll - blit.source_height );

				col += cons;
			}
		}
		else if( cols == 1 ){ /* scrolling rows + vertical scroll */
			int row,rowheight;
			int scrolly;

			if (colscroll[0] < 0) scrolly = blit.source_height - (-colscroll[0]) % blit.source_height;
			else scrolly = colscroll[0] % blit.source_height;

			rowheight = blit.source_height / rows;

			blit.clip_left = left;
			blit.clip_right = right;

			row = 0;
			while( row < rows ){
				int cons,scroll;

				/* count consecutive rows scrolled by the same amount */
				scroll = rowscroll[row];
				cons = 1;
				while (row + cons < rows &&	rowscroll[row + cons] == scroll) cons++;

				if (scroll < 0) scroll = blit.source_width - (-scroll) % blit.source_width;
				else scroll %= blit.source_width;

				blit.clip_top = row * rowheight + scrolly;
				if (blit.clip_top < top) blit.clip_top = top;
				blit.clip_bottom = (row + cons) * rowheight + scrolly;
				if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

				mark_visible( scroll,scrolly );
				mark_visible( scroll - blit.source_width,scrolly );

				blit.clip_top = row * rowheight + scrolly - blit.source_height;
				if (blit.clip_top < top) blit.clip_top = top;
				blit.clip_bottom = (row + cons) * rowheight + scrolly - blit.source_height;
				if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;

				mark_visible( scroll,scrolly - blit.source_height );
				mark_visible( scroll - blit.source_width,scrolly - blit.source_height );

				row += cons;
			}
		}

		tilemap->scrolled = 0;
	}

	{
		int tile_index;
		char *visible = tilemap->visible;
		char *dirty_vram = tilemap->dirty_vram;
		char *dirty_pixels = tilemap->dirty_pixels;

		int tile_height = tilemap->tile_height;

		unsigned char **pendata = tilemap->pendata;
		unsigned short **paldata = tilemap->paldata;
		unsigned int *pen_usage = tilemap->pen_usage;

		tile_info.flags = 0;
		tile_info.priority = 0;

		for( tile_index=0; tile_index<tilemap->num_tiles; tile_index++ ){
			if( visible[tile_index] && dirty_vram[tile_index] ){
				int row = tile_index/tilemap->num_cols;
				int col = tile_index%tilemap->num_cols;
				if( orientation & ORIENTATION_FLIP_Y ) row = tilemap->num_rows-1-row;
				if( orientation & ORIENTATION_FLIP_X ) col = tilemap->num_cols-1-col;
				if( orientation & ORIENTATION_SWAP_XY ) SWAP(col,row)

				unuse_color( (paldata[tile_index]-Machine->colortable), pen_usage[tile_index] );

				tilemap->tile_get_info( col, row );

				if( orientation & ORIENTATION_SWAP_XY ){
					tile_info.flags =
						(tile_info.flags&0xfc) |
						((tile_info.flags&1)<<1) | ((tile_info.flags&2)>>1);
				}

				pen_usage[tile_index] = tile_info.pen_usage;
				pendata[tile_index] = tile_info.pen_data;
				paldata[tile_index] = tile_info.pal_data;
				tilemap->flags[tile_index] = tile_info.flags;
				tilemap->priority[tile_index] = tile_info.priority;

				use_color( (paldata[tile_index]-Machine->colortable), pen_usage[tile_index] );

				dirty_pixels[tile_index] = 1;
				dirty_vram[tile_index] = 0;
			}
		}
	}
}

void tilemap_set_scrollx( struct tilemap *tilemap, int which, int value ){
	if( orientation & ORIENTATION_SWAP_XY ){
		if( orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( orientation & ORIENTATION_FLIP_Y ) value = -screen_height-value;
		if( tilemap->colscroll[which]!=value ){
			tilemap->scrolled = 1;
			tilemap->colscroll[which] = value;
		}
	}
	else {
		if( orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( orientation & ORIENTATION_FLIP_X ) value = -screen_width-value;
		if( tilemap->rowscroll[which]!=value ){
			tilemap->scrolled = 1;
			tilemap->rowscroll[which] = value;
		}
	}
}
void tilemap_set_scrolly( struct tilemap *tilemap, int which, int value ){
	if( orientation & ORIENTATION_SWAP_XY ){
		if( orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( orientation & ORIENTATION_FLIP_X ) value = -screen_width-value;
		if( tilemap->rowscroll[which]!=value ){
			tilemap->scrolled = 1;
			tilemap->rowscroll[which] = value;
		}
	}
	else {
		if( orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( orientation & ORIENTATION_FLIP_Y ) value = -screen_height-value;
		if( tilemap->colscroll[which]!=value ){
			tilemap->scrolled = 1;
			tilemap->colscroll[which] = value;
		}
	}
}
