/***************************************************************************

  Sega System 16 Video Hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/s16sprit.c"

extern unsigned char *sys16_videoram;
extern unsigned char *sys16_spriteram;
extern unsigned char *sys16_backgroundram;
extern void (* sys16_update_proc)( void );

int sys16_spritesystem;
int sys16_sprxoffset;
int *sys16_obj_bank;

int sys16_tile_bank1;
int sys16_tile_bank0;
int sys16_refreshenable;
int sys16_bg_scrollx, sys16_bg_scrolly;
int sys16_bg_page[4];
int sys16_fg_scrollx, sys16_fg_scrolly;
int sys16_fg_page[4];

int sys16_vh_start( void ){
	int i;

	for( i=0; i<4; i++ ){
		sys16_fg_page[i] = 0;
		sys16_bg_page[i] = 0;
	}

	sys16_update_proc = 0;

	sys16_spritesystem = 1;

	sys16_tile_bank0 = 0;
	sys16_tile_bank1 = 1;

	sys16_fg_scrollx = 0;
	sys16_fg_scrolly = 0;
	sys16_bg_scrollx = 0;
	sys16_bg_scrolly = 0;

	sys16_sprxoffset = 0xb8;
	sys16_refreshenable = 1;

	/* Golden Axe expects the palette to be initialized to black. It never */
	/* touches the locations who control the shadow of the fireball of the red dragon */
	for (i = 0;i < Machine->drv->total_colors;i++)
		palette_change_color(i,0,0,0);

	return 0;
}

void sys16_vh_stop( void ){
}

extern void drawgfxpicture(
	struct osd_bitmap *bitmap,
	const unsigned char *source,
	int width, int screenheight,
	const unsigned short *pal,
	int xflip, int yflip,
	int sx, int sy,
	int zoom,
	const struct rectangle *clip
);

void sys16_draw_sprites( struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned short *base_pal = Machine->gfx[0]->colortable + 1024;
	const unsigned char *base_gfx = Machine->memory_region[2];

	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+64*8;

	if( sys16_spritesystem==1  ){ /* standard sprite hardware */
		do{
			unsigned short attributes = source[4];

			if( ((attributes>>6)&0x3) == priority ){
				const unsigned char *gfx = base_gfx + source[3]*4 + (sys16_obj_bank[(attributes>>8)&0xf] << 17);
				const unsigned short *pal = base_pal + ((attributes&0x3f)<<4);

				int sy = source[0];
				int end_line = sy>>8;
				sy &= 0xff;

				if( end_line == 0xff ) break;

				{
					int sx = source[1] + sys16_sprxoffset;
					int zoom = source[5]&0x3ff;

					int width = source[2];
					int horizontal_flip = width&0x100;
					int vertical_flip = width&0x80;
					width = (width&0x7f)*4;

					if( vertical_flip ){
						width = 512-width;
					}

					if( horizontal_flip ){
						gfx += 4;
						if( vertical_flip ) gfx -= width*2;
					}
					else{
						if( vertical_flip ) gfx -= width; else gfx += width;
					}

					drawgfxpicture(
						bitmap,
						gfx,
						width, end_line - sy,
						pal,
						horizontal_flip, vertical_flip,
						sx, sy,
						zoom,
						clip
					);
				}
			}

			source += 8;
		} while( source<finish );
	}
	else if( sys16_spritesystem==0 ){ /* passing shot */
		do{
			unsigned short attributes = source[5];
			if( ((attributes>>14)&0x3) == priority ){
				int sy = source[1];
				if( sy != 0xffff ){
					int bank = (attributes>>4)&0xf;
					const unsigned short *pal = base_pal + ((attributes>>4)&0x3f0);

					int number = source[2];
					int horizontal_flip = number & 0x8000;

					int zoom = source[4]&0x3ff;

					int sx = source[0] + sys16_sprxoffset;

					int width = source[3];
					int vertical_flip = width&0x80;

					int end_line = sy>>8;
					sy = sy&0xff;

					sy+=2;
					end_line+=2;


					if( vertical_flip ){
						width &= 0x7f;
						width = 0x80-width;
					}
					else{
						width &= 0x7f;
					}

					if( horizontal_flip ){
						bank = (bank-1) & 0xf;
						if( vertical_flip ){
							sx += 5;
						}
						else {
							number += 1-width;
						}
					}

					drawgfxpicture(
						bitmap,
						base_gfx + number*4 + (sys16_obj_bank[bank] << 17),
						width*4, end_line - sy,
						pal,
						horizontal_flip, vertical_flip,
						sx, sy,
						zoom,
						clip
					);
				}
			}

			source += 8;
		}while( source<finish );
	}
}

static void palette_sprites( unsigned short *base ){
	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+64*8;

	if( sys16_spritesystem==1 ){ /* standard sprite hardware */
		do{
			if( (source[0]>>8) == 0xff ) break;
			base[source[4]&0x3f] = 1;
			source+=8;
		}while( source<finish );
	}
	else if( sys16_spritesystem==0 ){ /* passing shot */
		do{
			if( source[1]!=0xffff ) base[(source[5]>>8)&0x3f] = 1;
			source+=8;
		}while( source<finish );
	}
}

static void draw_background(struct osd_bitmap *bitmap){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;

	int scrollx = 320+sys16_bg_scrollx;
	int scrolly = 256-sys16_bg_scrolly;

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for( page=0; page < 4; page++ ){
		unsigned short *source = ((unsigned short *)sys16_backgroundram) + sys16_bg_page[page]*0x800;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				int x = startx+col;
				int y = starty+row;
				if( x>320 ) x-=1024;
				if( y>224 ) y-=512;
				if( x>-8 && x<320 && y>-8 && y<224 ){
					unsigned short data = *source;
					int tile_number = data&0xfff;
					tile_number += 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
					drawgfx( bitmap, gfx,
						tile_number,
						(data>>6)&0x7f, /* color */
						0, 0, x, y,
						clip, TRANSPARENCY_NONE, 0);
				}
				source++;
			}
		}
	} /* next page */
}

static void palette_background(unsigned char *base){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;

	int page;
	for (page=0; page < 4; page++){
		unsigned char *source = sys16_backgroundram +
			sys16_bg_page[page]*0x1000;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				unsigned short data = READ_WORD( &source[0] );
				int tile_number = data&0xfff;
				tile_number += 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
				base[(data >> 6)%128] |= gfx->pen_usage[tile_number & mask];
				source+=2;
			}
		}
	} /* next page */
}

static void draw_foreground(struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;
	int scrollx = 320+sys16_fg_scrollx;
	int scrolly = 256-sys16_fg_scrolly;

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for (page=0; page < 4; page++){
		unsigned short *source = ((unsigned short *)sys16_backgroundram) + sys16_fg_page[page]*0x800;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				unsigned short data = *source++;
				if( data>>15 == priority ){
					int x = startx+col;
					int y = starty+row;
					if( x>320 ) x-=1024;
					if( y>224 ) y-=512;
					if( x>-8 && x<320 && y>-8 && y<224 ){
						int tile_number = data&0xfff;
						tile_number += 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
						if( tile_number ){
							drawgfx( bitmap, gfx,
								tile_number,
								(data>>6)&0x7f, /* color */
								0, 0, x, y,
								clip, TRANSPARENCY_PEN, 0);
						}
					}
				}
			}
		}
	} /* next page */
}

static void palette_foreground(unsigned char *base){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;

	int page;

	for (page=0; page < 4; page++){
		unsigned char *source = sys16_backgroundram + sys16_fg_page[page]*0x1000;
		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				unsigned short data = READ_WORD ( &source[0] );
				int tile_number = data&0xfff;
				tile_number += 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
				base[(data >> 6)%128] |= gfx->pen_usage[tile_number & mask];
				source+=2;
			}
		}
	} /* next page */
}

static void draw_text(struct osd_bitmap *bitmap){
	const struct GfxElement *gfx = Machine->gfx[0];
	const struct rectangle *clip = &Machine->drv->visible_area;

	int sx,sy;
	for (sx = 0; sx < 40; sx++) for (sy = 0; sy < 28; sy++){
			int offs = (sy * 64 + sx + (64-40)) << 1;
			int data = READ_WORD(&sys16_videoram[offs]);
			int tile_number = data&0x1ff;
			if( tile_number ){ /* skip spaces */
				drawgfx(bitmap,gfx,
							tile_number,
							(data >> 9)%8, /* color */
							0,0, /* no flip*/
							8*sx,8*sy,
							clip,
							TRANSPARENCY_PEN,0);
			}
	}
}

static void palette_text(unsigned char *base){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;
	int sx,sy;
	for (sx = 0; sx < 40; sx++) for (sy = 0; sy < 28; sy++){
			int offs = (sy * 64 + sx + (64-40)) << 1;
			int data = READ_WORD(&sys16_videoram[offs]);
			int tile_number = data&0x1ff;
			if( tile_number ){ /* skip spaces */
				base[(data >> 9)%8] |= gfx->pen_usage[tile_number & mask] & 0xfe;
			}
	}
}

static void refresh_palette( void ){
	unsigned char *pal = &palette_used_colors[0];

	/* compute palette usage */
	unsigned char palette_map[128];
	unsigned short sprite_map[64];
	int i,j;

	memset (palette_map, 0, sizeof (palette_map));
	memset (sprite_map, 0, sizeof (sprite_map));

	palette_background( palette_map );
	palette_foreground( palette_map );
	palette_text( palette_map );
	palette_sprites( sprite_map );

	/* expand the results */
	for( i = 0; i < 128; i++ ){
		int usage = palette_map[i];
		if (usage){
			for (j = 0; j < 8; j++)
				if (usage & (1 << j))
					pal[j] = PALETTE_COLOR_USED;
				else
					pal[j] = PALETTE_COLOR_UNUSED;
		}
		else {
			memset (pal, PALETTE_COLOR_UNUSED, 8);
		}
		pal += 8;
	}
	for (i = 0; i < 64; i++){
		if ( sprite_map[i] ){
			pal[0] = PALETTE_COLOR_UNUSED;
			for (j = 1; j < 15; j++) pal[j] = PALETTE_COLOR_USED;
			pal[15] = PALETTE_COLOR_UNUSED;
		}
		else {
			memset( pal, PALETTE_COLOR_UNUSED, 16 );
		}
		pal += 16;
	}

	palette_recalc ();
}

void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	if( sys16_update_proc ) sys16_update_proc();

	if( sys16_refreshenable ){
		refresh_palette();

		draw_background(bitmap);

		sys16_draw_sprites(bitmap,1);

		draw_foreground(bitmap,0);

		sys16_draw_sprites(bitmap,2);

		draw_foreground(bitmap,1);

		draw_text(bitmap);

		sys16_draw_sprites(bitmap,3);
	}
}
