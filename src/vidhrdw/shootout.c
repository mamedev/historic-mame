/*
	Video Hardware for Shoot Out
	prom GB09.K6 may be related to background tile-sprite priority
*/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *background, *foreground;
extern unsigned char *shootout_textram;

static void get_bg_tile_info(int tile_index){
	int attributes = videoram[tile_index+0x400]; /* CCCC -TTT */
	int tile_number = videoram[tile_index] + 256*(attributes&7);
	int color = attributes>>4;
	SET_TILE_INFO(2,tile_number,color )
}

static void get_fg_tile_info(int tile_index){
	int attributes = shootout_textram[tile_index+0x400]; /* CCCC --TT */
	int tile_number = shootout_textram[tile_index] + 256*(attributes&0x3);
	int color = attributes>>4;
	SET_TILE_INFO(0,tile_number,color )
}

WRITE_HANDLER( shootout_videoram_w ){
	if( videoram[offset]!=data ){
		videoram[offset] = data;
		tilemap_mark_tile_dirty( background, offset&0x3ff );
	}
}
WRITE_HANDLER( shootout_textram_w ){
	if( shootout_textram[offset]!=data ){
		shootout_textram[offset] = data;
		tilemap_mark_tile_dirty( foreground, offset&0x3ff );
	}
}

int shootout_vh_start( void ){
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	foreground = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	if( background && foreground ){
		tilemap_set_transparent_pen( foreground, 0 );
		return 0;
	}
	return 1; /* error */
}

static void draw_sprites( struct osd_bitmap *bitmap, int bank_bits ){
	static int bFlicker;
	const struct GfxElement *gfx = Machine->gfx[1];
	const struct rectangle *clip = &Machine->visible_area;
	const UINT8 *source = spriteram+127*4;
	int count;

	bFlicker = !bFlicker;

	for( count=0; count<128; count++ ){
		int attributes = source[1];
		/*
		    76543210
			xxx-----	bank
			---x----	vertical size
			----x---	priority
			-----x--	horizontal flip
			------x-	flicker
			-------x	enable
		*/
		if ( attributes & 0x01 ){ /* visible */
			if( bFlicker || (attributes&0x02)==0 ){
				int priority_mask = (attributes&0x08)?0xaa:0;
				int sx = (240 - source[2])&0xff;
				int sy = (240 - source[0])&0xff;
				int number = source[3] | ((attributes<<bank_bits)&0x700);
				int flipx = (attributes & 0x04);

				if( attributes & 0x10 ){ /* double height */
					number = number&(~1);
					sy -= 16;
					pdrawgfx(bitmap,gfx,
						number,
						0 /*color*/,
						flipx,0 /*flipy*/,
						sx,sy,
						clip,TRANSPARENCY_PEN,0,
						priority_mask);

					number++;
					sy += 16;
				}

				pdrawgfx(bitmap,gfx,
						number,
						0 /*color*/,
						flipx,0 /*flipy*/,
						sx,sy,
						clip,TRANSPARENCY_PEN,0,
						priority_mask);
				}
		}
		source -= 4;
	}
}

static void draw_sprites2( struct osd_bitmap *bitmap ){
}

void shootout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
//	mark_sprite_colors();
	palette_recalc();
	fillbitmap(priority_bitmap,0,NULL);

	tilemap_draw(bitmap,background,0,0);
	tilemap_draw(bitmap,foreground,0,1);
	draw_sprites( bitmap,3/*bank bits */ );
}

void shootouj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
//	mark_sprite_colors();
	palette_recalc();
	fillbitmap(priority_bitmap,0,NULL);

	tilemap_draw(bitmap,background,0,1);
	tilemap_draw(bitmap,foreground,0,2);
	draw_sprites( bitmap,2/*bank bits*/ );
}
