/*******************************************************************

Rolling Thunder Video Hardware

*******************************************************************/

#include "driver.h"
#include "tilemap.h"

void rt_stop_mcu_timer( void );


#define GFX_TILES1	0
#define GFX_TILES2	1
#define GFX_SPRITES	2

unsigned char *rthunder_videoram;
extern unsigned char *spriteram;

static int tilebank;
static int xscroll[4], yscroll[4];

static struct tilemap *tilemap[4];


void rthunder_tilebank_w( int offset, int data ){
	if( tilebank!=data ){
		tilebank = data;
		tilemap_mark_all_tiles_dirty(tilemap[0]);
		tilemap_mark_all_tiles_dirty(tilemap[1]);
	}
}

void rthunder_scroll_w( int layer, int offset, int data )
{
	int xdisp[4] = { 20,18,21,19 };
	switch( offset )
	{
		case 0:
			xscroll[layer] = (xscroll[layer]&0xff)|(data<<8);
			tilemap_set_scrollx(tilemap[layer],0,-xscroll[layer]-xdisp[layer]);
//tilemap_set_scrollx(tilemap[layer],0,xscroll[layer]+xdisp[layer]-224);
			break;
		case 1:
			xscroll[layer] = (xscroll[layer]&0xff00)|data;
			tilemap_set_scrollx(tilemap[layer],0,-xscroll[layer]-xdisp[layer]);
//tilemap_set_scrollx(tilemap[layer],0,xscroll[layer]+xdisp[layer]-224);
			break;
		case 2:
			yscroll[layer] = data;
			tilemap_set_scrolly(tilemap[layer],0,-yscroll[layer]-25);
//tilemap_set_scrolly(tilemap[layer],0,yscroll[layer]-7);
			break;
	}
}

/*******************************************************************/

void rthunder_vh_convert_color_prom( unsigned char *palette,
	unsigned short *colortable,
	const unsigned char *color_prom ){

	int i;

	for( i = 0; i<256; i++ ){
		unsigned char byte0 = color_prom[i];
		unsigned char byte1 = color_prom[i+512];

		unsigned char red = byte0&0x0f;
		unsigned char blue = byte1;
		unsigned char green = byte0>>4;

		*(palette++) = red | (red<<4);
		*(palette++) = green | (green<<4);
		*(palette++) = blue | (blue<<4);
	}

	color_prom += 0x400;

	for( i=0; i<4096; i++ ){
		colortable[i] = *color_prom++;
	}

}




/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static unsigned char *videoram;
static int gfx_num,tile_offset;

static void tilemap0_preupdate(void)
{
	videoram = &rthunder_videoram[0x0000];
	gfx_num = GFX_TILES1;
	tile_offset = 0*0x400 + tilebank * 0x800;
}

static void tilemap1_preupdate(void)
{
	videoram = &rthunder_videoram[0x1000];
	gfx_num = GFX_TILES1;
	tile_offset = 1*0x400 + tilebank * 0x800;
}

static void tilemap2_preupdate(void)
{
	videoram = &rthunder_videoram[0x2000];
	gfx_num = GFX_TILES2;
	tile_offset = 0*0x400;
}

static void tilemap3_preupdate(void)
{
	videoram = &rthunder_videoram[0x3000];
	gfx_num = GFX_TILES2;
	tile_offset = 1*0x400;
}

static void get_tile_info(int col,int row)
{
	int tile_index = 2*(64*row+col);
	unsigned char attr = videoram[tile_index + 1];
	SET_TILE_INFO(gfx_num,tile_offset + videoram[tile_index] + ((attr & 0x03) << 8),attr)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void rthunder_vh_stop(void)
{
	tilemap_dispose(tilemap[0]);
	tilemap_dispose(tilemap[1]);
	tilemap_dispose(tilemap[2]);
	tilemap_dispose(tilemap[3]);

	rt_stop_mcu_timer();
}

int rthunder_vh_start(void)
{
	tilemap[0] = tilemap_create(TILEMAP_TRANSPARENT,8,8,64,32,1,1);
	tilemap[1] = tilemap_create(TILEMAP_TRANSPARENT,8,8,64,32,1,1);
	tilemap[2] = tilemap_create(TILEMAP_TRANSPARENT,8,8,64,32,1,1);
	tilemap[3] = tilemap_create(TILEMAP_TRANSPARENT,8,8,64,32,1,1);

	if (tilemap[0] && tilemap[1] && tilemap[2] && tilemap[3])
	{
		tilemap[0]->tile_get_info = get_tile_info;
		tilemap[1]->tile_get_info = get_tile_info;
		tilemap[2]->tile_get_info = get_tile_info;
		tilemap[3]->tile_get_info = get_tile_info;
		tilemap[0]->transparent_pen = 7;
		tilemap[1]->transparent_pen = 7;
		tilemap[2]->transparent_pen = 7;
		tilemap[3]->transparent_pen = 7;

		return 0;
	}

	rthunder_vh_stop();

	return 1;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

int rthunder_videoram_r(int offset)
{
	return rthunder_videoram[offset];
}

void rthunder_videoram_w(int offset,int data)
{
	if (rthunder_videoram[offset] != data)
	{
		rthunder_videoram[offset] = data;
		tilemap_mark_tile_dirty(tilemap[offset/0x1000],(offset/2)%64,((offset%0x1000)/2)/64);
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap, int sprite_priority ){
	/* note: sprites don't yet clip at the top of the screen properly */

	struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const struct rectangle *clip = &Machine->drv->visible_area;

	const unsigned char *source = &spriteram[0x1400];
	const unsigned char *finish = &spriteram[0x1c00];

	while( source<finish ){
/*
	source[4]	S--T -TTT
	source[5]	-TTT TTTT
	source[6]   CCCC CCCX
	source[7]	XXXX XXXX
	source[8]	PP-T -S--
	source[9]   YYYY YYYY
*/
		unsigned char priority = source[8];
		if( priority>>6 == sprite_priority ) {
			unsigned char attrs = source[4];
			unsigned char color = source[6];
			int sx = source[7] + (color&1)*256; /* need adjust for left clip */
			int sy = 192-source[9];
			int flip = attrs&0x20; /* horizontal flip */
			int tall = (priority&0x04)?1:0;
			int wide = (attrs&0x80)?1:0;
			int sprite_bank = attrs&7;
			int sprite_number = ((source[5]&0x7f)+(sprite_bank<<7))*4;
			int row,col;

			if( attrs&0x10 ) sprite_number+=1;
			if( priority&0x10 ) sprite_number+=2;
			color = color>>1;

			if( sx>512-32 ) sx -= 512;

			if( !tall ) sy+=16;
			if( flip && !wide ) sx-=16;
			for( row=0; row<=tall; row++ ){
				for( col=0; col<=wide; col++ ){
					drawgfx( bitmap, gfx,
						sprite_number+2*row+col,
						color,
						flip,0,
						sx+16*(flip?1-col:col),
						sy+row*16,
						clip,
						TRANSPARENCY_PEN, 0xf );
				}
			}
		}
		source+=16;
	}
}

void rthunder_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap0_preupdate(); tilemap_update(tilemap[0]);
	tilemap1_preupdate(); tilemap_update(tilemap[1]);
	tilemap2_preupdate(); tilemap_update(tilemap[2]);
	tilemap3_preupdate(); tilemap_update(tilemap[3]);

	tilemap_render(tilemap[0]);
	tilemap_render(tilemap[1]);
	tilemap_render(tilemap[2]);
	tilemap_render(tilemap[3]);

	/* the background color can be changed but I don't know to which address it's mapped */
	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
	draw_sprites(bitmap,0);
	tilemap_draw(bitmap,tilemap[0],0);
	draw_sprites(bitmap,1);
	tilemap_draw(bitmap,tilemap[1],0);
	draw_sprites(bitmap,2);
	tilemap_draw(bitmap,tilemap[2],0);
	draw_sprites(bitmap,3);
	tilemap_draw(bitmap,tilemap[3],0);
}
