/***************************************************************************

Game				Video Hardware
====				==============
Master Boy			Unknown
Big Karnak			Type 1
Master Boy 2		Unknown
Splash!				Splash! specific
Thunder Hoop		Type 1
Squash				Type 1
World Rally			Type 1
Glass				Unknown
Strike Back			Unknown
Target Hits			Unknown
Biomechanical Toy	Type 1
Alligator Hunt		Type 2
World Rally 2		Unknown
Salter				Unknown
Touch & Go			Type 2
Maniac Square		Type 1
Snow Board			Type 2
Bang				Unknown

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

/*============================================================================
						SPLASH! Video Hardware
  ============================================================================*/

unsigned char *splash_vregs;
unsigned char *splash_videoram;
unsigned char *splash_spriteram;
unsigned char *splash_pixelram;

static struct tilemap *screen[2];
static struct osd_bitmap *screen2;

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

/*
	Tile format
	-----------

	Screen 0: (64*32, 8x8 tiles)

	Byte | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | sprite code (low 8 bits)
	  0  | ----xxxx -------- | sprite code (high 4 bits)
	  0  | xxxx---- -------- | color

	Screen 1: (32*32, 16x16 tiles)

	Byte | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- -------x | flip y
	  0  | -------- ------x- | flip x
	  0  | -------- xxxxxx-- | sprite code (low 6 bits)
	  0  | ----xxxx -------- | sprite code (high 4 bits)
	  0  | xxxx---- -------- | color
*/

static void get_tile_info_splash_screen0(int tile_index)
{
	int data = READ_WORD(&splash_videoram[2*tile_index]);
	int attr = data >> 8;
	int code = data & 0xff;

	SET_TILE_INFO(0, code + ((0x20 + (attr & 0x0f)) << 8), (attr & 0xf0) >> 4);
}

static void get_tile_info_splash_screen1(int tile_index)
{
	int data = READ_WORD(&splash_videoram[0x1000 + 2*tile_index]);
	int attr = data >> 8;
	int code = data & 0xff;

	tile_info.flags = TILE_FLIPXY(code & 0x03);

	SET_TILE_INFO(1, (code >> 2) + ((0x30 + (attr & 0x0f)) << 6), (attr & 0xf0) >> 4);
}

/***************************************************************************

	Memory Handlers

***************************************************************************/

READ_HANDLER( splash_vram_r )
{
	return READ_WORD(&splash_videoram[offset]);
}

WRITE_HANDLER( splash_vram_w )
{
	COMBINE_WORD_MEM(&splash_videoram[offset],data);
	tilemap_mark_tile_dirty(screen[offset >> 12],(offset & 0x0fff) >> 1);
}

READ_HANDLER( splash_pixelram_r )
{
	return READ_WORD(&splash_pixelram[offset]);
}

WRITE_HANDLER( splash_pixelram_w )
{
	int sx,sy,color;

	COMBINE_WORD_MEM(&splash_pixelram[offset],data);

	sx = (offset >> 1) & 0x1ff;	//(offset/2) % 512;
	sy = (offset >> 10);		//(offset/2) / 512;

	color = READ_WORD(&splash_pixelram[offset]);

	plot_pixel(screen2, sx-9, sy, Machine->pens[0x300 + (color & 0xff)]);
}


/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int splash_vh_start(void)
{
	screen[0] = tilemap_create(get_tile_info_splash_screen0,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	screen[1] = tilemap_create(get_tile_info_splash_screen1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	screen2 = bitmap_alloc (512, 256);

	if (!screen[0] || !screen[1] || !screen[2])
		return 1;

	screen[0]->transparent_pen = 0;
	screen[1]->transparent_pen = 0;

	tilemap_set_scrollx(screen[0], 0, 4);

	return 0;
}

/***************************************************************************

	Sprites

***************************************************************************/

/*
	Sprite Format
	-------------

	Byte | Bit(s)   | Description
	-----+-76543210-+--------------------------
	  0  | xxxxxxxx | sprite number (low 8 bits)
	  1  | xxxxxxxx | y position
	  2  | xxxxxxxx | x position (low 8 bits)
	  3  | ----xxxx | sprite number (high 4 bits)
	  3  | --xx---- | unknown
	  3  | -x------ | flip x
	  3  | x------- | flip y
	  4  | ----xxxx | sprite color
	  4  | -xxx---- | unknown
	  4  | x------- | x position (high bit)
*/

static void splash_draw_sprites(struct osd_bitmap *bitmap)
{
	int i;
	const struct GfxElement *gfx = Machine->gfx[1];

	for (i = 0; i < 0x800; i += 8){
		int sx = READ_WORD(&splash_spriteram[i+4]) & 0xff;
		int sy = 256 - (READ_WORD(&splash_spriteram[i+2]) & 0xff);
		int attr = READ_WORD(&splash_spriteram[i+6]) & 0xff;
		int attr2 = READ_WORD(&splash_spriteram[i+0x800]) >> 8;
		int number = (READ_WORD(&splash_spriteram[i]) & 0xff) + (attr & 0xf)*256;

		if (attr2 & 0x80) sx += 256;

		drawgfx(bitmap,gfx,number,
			0x10 + (attr2 & 0x0f),attr & 0x40,attr & 0x80,
			sx-8,sy-16,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

void splash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* set scroll registers */
	tilemap_set_scrolly(screen[0], 0, READ_WORD(&splash_vregs[0]));
	tilemap_set_scrolly(screen[1], 0, READ_WORD(&splash_vregs[2]));

	tilemap_update(ALL_TILEMAPS);

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	copybitmap(bitmap,screen2,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	tilemap_draw(bitmap,screen[1],0);
	splash_draw_sprites(bitmap);
	tilemap_draw(bitmap,screen[0],0);
}


/*============================================================================
					Gaelco Type 1 Video Hardware
  ============================================================================*/

unsigned char *gaelco_vregs;
unsigned char *gaelco_videoram;
unsigned char *gaelco_spriteram;

int sprite_count[4];
int *sprite_table[4];
static struct tilemap *pant[2];

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

/*
	Tile format
	-----------

	Screen 0 & 1: (32*32, 16x16 tiles)

	Byte | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- -------x | flip x
	  0  | -------- ------x- | flip y
	  0  | -------- xxxxxx-- | code (low 6 bits)
	  0  | xxxxxxxx -------- | code (high 8 bits)
	  2  | -------- --xxxxxx | color
	  2	 | -------- xx------ | priority
	  2  | xxxxxxxx -------- | not used
*/

static void get_tile_info_gaelco_screen0(int tile_index)
{
	int data = READ_WORD(&gaelco_videoram[tile_index << 2]);
	int data2 = READ_WORD(&gaelco_videoram[(tile_index << 2) + 2]);
	int code = ((data & 0xfffc) >> 2);

	tile_info.flags = TILE_FLIPYX(data & 0x03);
	tile_info.priority = (data2 >> 6) & 0x03;

	SET_TILE_INFO(1, 0x4000 + code, data2 & 0x3f);
}


static void get_tile_info_gaelco_screen1(int tile_index)
{
	int data = READ_WORD(&gaelco_videoram[0x1000 + (tile_index << 2)]);
	int data2 = READ_WORD(&gaelco_videoram[0x1000 + (tile_index << 2) + 2]);
	int code = ((data & 0xfffc) >> 2);

	tile_info.flags = TILE_FLIPYX(data & 0x03);
	tile_info.priority = (data2 >> 6) & 0x03;

	SET_TILE_INFO(1, 0x4000 + code, data2 & 0x3f);
}

/***************************************************************************

	Memory Handlers

***************************************************************************/

READ_HANDLER( gaelco_vram_r )
{
	return READ_WORD(&gaelco_videoram[offset]);
}

WRITE_HANDLER( gaelco_vram_w )
{
	COMBINE_WORD_MEM(&gaelco_videoram[offset],data);
	tilemap_mark_tile_dirty(pant[offset >> 12],(offset & 0x0fff) >> 2);
}

/***************************************************************************

	Start/Stop the video hardware emulation.

***************************************************************************/

void gaelco_vh_stop(void)
{
	int i;

	for (i = 0; i < 4; i++){
		if (sprite_table[i])
			free(sprite_table[i]);
		sprite_table[i] = NULL;
	}
}

int gaelco_vh_start(void)
{
	int i;

	pant[0] = tilemap_create(get_tile_info_gaelco_screen0,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pant[1] = tilemap_create(get_tile_info_gaelco_screen1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if (!pant[0] || !pant[1])
		return 1;

	pant[0]->transparent_pen = 0;
	pant[1]->transparent_pen = 0;

	for (i = 0; i < 4; i++){
		sprite_table[i] = malloc(512*sizeof(int));
		if (!sprite_table[i]){
			gaelco_vh_stop();
			return 1;
		}
	}

	return 0;
}

/***************************************************************************

	Sprites

***************************************************************************/

static void gaelco_sort_sprites(void)
{
	int i;

	sprite_count[0] = 0;
	sprite_count[1] = 0;
	sprite_count[2] = 0;
	sprite_count[3] = 0;

	for (i = 6; i < 0x1000 - 6; i += 8){
		int priority = (READ_WORD(&gaelco_spriteram[i]) & 0x3000) >> 12;

		/* save sprite number in the proper array for later */
		sprite_table[priority][sprite_count[priority]] = i;
		sprite_count[priority]++;
	}
}

/*
	Sprite Format
	-------------

	Byte | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | y position
	  0  | -----xxx -------- | not used
	  0  | ----x--- -------- | sprite size
	  0  | --xx---- -------- | sprite priority
	  0  | -x------ -------- | flipx
	  0  | x------- -------- | flipy
	  2  | xxxxxxxx xxxxxxxx | not used
	  4  | -------x xxxxxxxx | x position
	  4  | -xxxxxx- -------- | sprite color
	  6	 | -------- ------xx | sprite code (8x8 cuadrant)
	  6  | xxxxxxxx xxxxxx-- | sprite code
*/

static void gaelco_draw_sprites(struct osd_bitmap *bitmap, int pri)
{
	int j, x, y, ex, ey;
	const struct GfxElement *gfx = Machine->gfx[0];

	static int x_offset[2] = {0x0,0x2};
	static int y_offset[2] = {0x0,0x1};

	for (j = 0; j < sprite_count[pri]; j++){
		int i = sprite_table[pri][j];
		int sx = READ_WORD(&gaelco_spriteram[i+4]) & 0x01ff;
		int sy = 256 - (READ_WORD(&gaelco_spriteram[i]) & 0x00ff);
		int number = READ_WORD(&gaelco_spriteram[i+6]);
		int color = (READ_WORD(&gaelco_spriteram[i+4]) & 0x7e00) >> 9;
		int attr = (READ_WORD(&gaelco_spriteram[i]) & 0xfe00) >> 9;

		int xflip = attr & 0x20;
		int yflip = attr & 0x40;
		int spr_size;

		if (attr & 0x04){
			spr_size = 1;
		}
		else{
			spr_size = 2;
			number &= (~3);
		}

		sy -= 0x10;
		if (sy < 0) sy += 0x100;

		for (y = 0; y < spr_size; y++){
			for (x = 0; x < spr_size; x++){

				ex = xflip ? (spr_size-1-x) : x;
				ey = yflip ? (spr_size-1-y) : y;

				drawgfx(bitmap,gfx,number + x_offset[ex] + y_offset[ey],
						color,xflip,yflip,
						sx-0x0f+x*8,sy+y*8,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

void maniacsq_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* set scroll registers */
	tilemap_set_scrolly(pant[0], 0, READ_WORD(&gaelco_vregs[0]));
	tilemap_set_scrollx(pant[0], 0, READ_WORD(&gaelco_vregs[2])+4);
	tilemap_set_scrolly(pant[1], 0, READ_WORD(&gaelco_vregs[4]));
	tilemap_set_scrollx(pant[1], 0, READ_WORD(&gaelco_vregs[6]));

	tilemap_update(ALL_TILEMAPS);
	gaelco_sort_sprites();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);


	fillbitmap( bitmap, Machine->pens[0], &Machine->visible_area );

	tilemap_draw(bitmap,pant[1],3);
	tilemap_draw(bitmap,pant[0],3);
	gaelco_draw_sprites(bitmap,3);
	tilemap_draw(bitmap,pant[1],2);
	tilemap_draw(bitmap,pant[0],2);
	gaelco_draw_sprites(bitmap,2);
	tilemap_draw(bitmap,pant[1],1);
	tilemap_draw(bitmap,pant[0],1);
	gaelco_draw_sprites(bitmap,1);
	tilemap_draw(bitmap,pant[1],0);
	tilemap_draw(bitmap,pant[0],0);
	gaelco_draw_sprites(bitmap,0);
}

void bigkarnk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* set scroll registers */
	tilemap_set_scrolly(pant[0], 0, READ_WORD(&gaelco_vregs[0]));
	tilemap_set_scrollx(pant[0], 0, READ_WORD(&gaelco_vregs[2])+4);
	tilemap_set_scrolly(pant[1], 0, READ_WORD(&gaelco_vregs[4]));
	tilemap_set_scrollx(pant[1], 0, READ_WORD(&gaelco_vregs[6]));

	tilemap_update(ALL_TILEMAPS);
	gaelco_sort_sprites();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);


	fillbitmap( bitmap, Machine->pens[0], &Machine->visible_area );

	tilemap_draw(bitmap,pant[1],3);
	tilemap_draw(bitmap,pant[0],3);
	gaelco_draw_sprites(bitmap,3);
	tilemap_draw(bitmap,pant[1],2);
	tilemap_draw(bitmap,pant[0],2);
	gaelco_draw_sprites(bitmap,2);
	tilemap_draw(bitmap,pant[1],1);
	tilemap_draw(bitmap,pant[0],1);
	gaelco_draw_sprites(bitmap,1);
	tilemap_draw(bitmap,pant[1],0);
	tilemap_draw(bitmap,pant[0],0);
	gaelco_draw_sprites(bitmap,0);
}
