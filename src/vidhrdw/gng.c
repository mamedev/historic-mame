/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "tilemap.h"

extern unsigned char *spriteram;
extern int spriteram_size;

unsigned char *gng_fgvideoram,*gng_fgcolorram;
unsigned char *gng_bgvideoram,*gng_bgcolorram;
static struct tilemap *bg_tilemap,*fg_tilemap;
static int flipscreen;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static const unsigned char *videoram1, *videoram2;
static int gfxbank;

static void fg_tilemap_preupdate(void){
	gfxbank = 0;
	videoram1 = gng_fgvideoram;
	videoram2 = gng_fgcolorram;
}

static void get_fg_tile_info( int col, int row ){
	int tile_index = row*32+col;
	unsigned char attributes = videoram2[tile_index];
	SET_TILE_INFO(gfxbank,videoram1[tile_index] + ((attributes & 0xc0) << 2),attributes & 0x0f)
	tile_info.flags = TILE_FLIPYX((attributes & 0x30) >> 4);
}

static void bg_tilemap_preupdate(void){
	gfxbank = 1;
	videoram1 = gng_bgvideoram;
	videoram2 = gng_bgcolorram;
}

static void get_bg_tile_info( int col, int row ){
	int tile_index = col*32+row;
	unsigned char attributes = videoram2[tile_index];
	SET_TILE_INFO(gfxbank,videoram1[tile_index] + ((attributes & 0xc0) << 2),attributes & 0x07)
	tile_info.flags = TILE_FLIPYX((attributes & 0x30) >> 4) | TILE_SPLIT((attributes & 0x08) >> 3);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
void gng_vh_stop(void){
	tilemap_dispose(bg_tilemap);
	tilemap_dispose(fg_tilemap);
	tilemap_stop();
}

int gng_vh_start(void){
	if (tilemap_start() == 0){
		fg_tilemap = tilemap_create(
			TILEMAP_TRANSPARENT,
			8,8, /* tile width, tile height */
			32,32, /* number of columns, number of rows */
			0,0	/* scroll rows, scroll columns */
		);

		bg_tilemap = tilemap_create(TILEMAP_SPLIT,16,16,32,32,1,1);

		if (fg_tilemap && bg_tilemap){
			fg_tilemap->tile_get_info = get_fg_tile_info;
			fg_tilemap->transparent_pen = 3;

			bg_tilemap->tile_get_info = get_bg_tile_info;
			bg_tilemap->transmask[0] = 0xff; /* split type 0 is totally transparent in front half */
			bg_tilemap->transmask[1] = 0x01; /* split type 1 has pen 1 transparent in front half */

			return 0;
		}
	}
	gng_vh_stop();
	return 1;
}

void gng_fgvideoram_w(int offset,int data){
	if (gng_fgvideoram[offset] != data){
		gng_fgvideoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset%32,offset/32);
	}
}

void gng_fgcolorram_w(int offset,int data){
	if (gng_fgcolorram[offset] != data){
		gng_fgcolorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset%32,offset/32);
	}
}

void gng_bgvideoram_w(int offset,int data){
	if (gng_bgvideoram[offset] != data){
		gng_bgvideoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/32,offset%32);
	}
}

void gng_bgcolorram_w(int offset,int data){
	if (gng_bgcolorram[offset] != data){
		gng_bgcolorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/32,offset%32);
	}
}



void gng_bgscrollx_w(int offset,int data){
	static unsigned char scrollx[2];
	scrollx[offset] = data;
	tilemap_set_scrollx( bg_tilemap, 0, -(scrollx[0] + 256 * scrollx[1]) );
}

void gng_bgscrolly_w(int offset,int data){
	static unsigned char scrolly[2];
	scrolly[offset] = data;
	tilemap_set_scrolly( bg_tilemap, 0, -(scrolly[0] + 256 * scrolly[1]) );
}


void gng_flipscreen_w(int offset,int data){
	if (flipscreen != (~data & 1)){
		flipscreen = ~data & 1;
		tilemap_mark_all_pixels_dirty(bg_tilemap);
		tilemap_mark_all_pixels_dirty(fg_tilemap);
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap){
	const struct GfxElement *gfx = Machine->gfx[2];
	const struct rectangle *clip = &Machine->drv->visible_area;
	int offs;
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4){
		unsigned char attributes = spriteram[offs+1];
		int sx = spriteram[offs + 3] - 0x100 * (attributes & 0x01);
		int sy = spriteram[offs + 2];
		int flipx = attributes & 0x04;
		int flipy = attributes & 0x08;

		if (flipscreen){
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,gfx,
				spriteram[offs] + ((attributes<<2) & 0x300),
				(attributes >> 4) & 3,
				flipx,flipy,
				sx,sy,
				clip,TRANSPARENCY_PEN,15);
	}
}

void gng_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	bg_tilemap_preupdate(); tilemap_update(bg_tilemap);
	fg_tilemap_preupdate(); tilemap_update(fg_tilemap);

	tilemap_render(bg_tilemap);
	tilemap_render(fg_tilemap);

	tilemap_draw(bitmap,bg_tilemap,TILEMAP_BACK);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,bg_tilemap,TILEMAP_FRONT);
	tilemap_draw(bitmap,fg_tilemap,0);
}
