#include "driver.h"
#include "sprite.h"

/*
	This file replaces s16sprit.c, and should be included in the makefile.

	This is a general-purpose renderer for streamed scaled sprites/

	Sega System16 video hardware currently makes use of it,
	and it should be useful for Nemesis hardware as well.

	still to do:
	- optimize (especially the non-zoomed case)
	- fix stray pixel glitches
*/

struct sprite_info sprite_info;

#define SWAP(X,Y) { int temp = X; X = Y; Y = temp; }

void draw_sprite( struct osd_bitmap *bitmap ){
	if( sprite_info.dest_h==0 || sprite_info.dest_w<=0 ) return;

	{
		int screen_width = bitmap->width;
		int screen_height = bitmap->height;
		int orientation = Machine->orientation;

		int source_row_offset = sprite_info.source_row_offset;
		int source_rows = sprite_info.source_h;

		int x1,y1,x2,y2;
		int dx,dy;
		int tx0 = 0,ty0 = 0;
		int scalex = (sprite_info.source_w<<16)/sprite_info.dest_w;
		int scaley = (sprite_info.source_h<<16)/sprite_info.dest_h;

		if( orientation & ORIENTATION_SWAP_XY ){
			SWAP(sprite_info.dest_x,sprite_info.dest_y)
			SWAP(sprite_info.dest_w,sprite_info.dest_h)
			SWAP(sprite_info.source_w,sprite_info.source_h)
		}

		if( orientation & ORIENTATION_FLIP_X ){
			sprite_info.dest_x = screen_width - sprite_info.dest_x - sprite_info.dest_w;
			if( orientation & ORIENTATION_SWAP_XY ){
				sprite_info.flags ^= SPRITE_FLIPY;
			}
			else {
				sprite_info.flags ^= SPRITE_FLIPX;
			}
		}

		if( orientation & ORIENTATION_FLIP_Y ){
			sprite_info.dest_y = screen_height - sprite_info.dest_y - sprite_info.dest_h;
			if( orientation & ORIENTATION_SWAP_XY ){
				sprite_info.flags ^= SPRITE_FLIPX;
			}
			else {
				sprite_info.flags ^= SPRITE_FLIPY;
			}
		}

		if( sprite_info.flags & SPRITE_FLIPX ){
			sprite_info.source_baseaddr += source_row_offset - 1;
			dx = -1;
		}
		else {
			dx = 1;
		}

		if( sprite_info.flags & SPRITE_FLIPY ){
			sprite_info.source_baseaddr += source_row_offset*(source_rows-1);
			dy = -1;
		}
		else {
			dy = 1;
		}

		x1= sprite_info.dest_x;
		y1 = sprite_info.dest_y;
		x2 = sprite_info.dest_w + x1;
		y2 = sprite_info.dest_h + y1;

		dx *= scalex;
		dy *= scaley;

		if( x1<0 ){
			tx0 = (-x1)*((orientation & ORIENTATION_SWAP_XY)?dy:dx);
			x1 = 0;
		}

		if( y1<0 ){
			ty0 = (-y1)*((orientation & ORIENTATION_SWAP_XY)?dx:dy);
			y1 = 0;
		}

		if( x2>screen_width ){
			x2 = screen_width;
		}

		if( y2>screen_height ){
			y2 = screen_height;
		}

		if( x1<x2 && y1<y2 ){
			const unsigned short *pal_data = sprite_info.pal_data;
			int x,y;
			const unsigned char *source;
			int tx,ty;

			if( Machine->orientation & ORIENTATION_SWAP_XY ){
				ty = ty0;
				for( y=y1; y<y2; y++ ){
					unsigned char *dest = bitmap->line[y];
					tx = tx0;
					for( x=x1; x<x2; x++ ){
						unsigned char pen;
						source = sprite_info.source_baseaddr+sprite_info.source_row_offset*(tx>>16);
						pen = source[ty>>16];
						tx += dy;
						if( pen ) dest[x] = pal_data[pen];
					}
					ty += dx;
				}
			}

			else {
				ty = ty0;
				for( y=y1; y<y2; y++ ){
					unsigned char *dest = bitmap->line[y];
					tx = tx0;
					source = sprite_info.source_baseaddr+sprite_info.source_row_offset*(ty>>16);
					for( x=x1; x<x2; x++ ){
						unsigned char pen = source[tx>>16];
						tx += dx;
						if( pen ) dest[x] = pal_data[pen];
					}
					ty += dy;
				}
			}
		}
	}
}
