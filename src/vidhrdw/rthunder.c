/*******************************************************************

Rolling Thunder Video Hardware

*******************************************************************/

#include "driver.h"


#define GFX_TILES1	0
#define GFX_TILES2	1
#define GFX_SPRITES	2

unsigned char *rthunder_videoram;
extern unsigned char *spriteram;

static int tilebank;
static int xscroll[4], yscroll[4];

static struct tilemap *tilemap[4];

static int flipscreen;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Rolling Thunder has two palette PROMs (512x8 and 512x4) and two 2048x8
  lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

***************************************************************************/
void rthunder_vh_convert_color_prom( unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom )
{
	int i;
	int totcolors,totlookup;


	totcolors = Machine->drv->total_colors;
	totlookup = Machine->drv->color_table_len;

	for (i = 0;i < totcolors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[totcolors] >> 0) & 0x01;
		bit1 = (color_prom[totcolors] >> 1) & 0x01;
		bit2 = (color_prom[totcolors] >> 2) & 0x01;
		bit3 = (color_prom[totcolors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/* tiles lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *color_prom++;

	/* sprites lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *(color_prom++) + totcolors/2;
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

int rthunder_vh_start(void)
{
	tilemap[0] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[2] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[3] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);

	if (tilemap[0] && tilemap[1] && tilemap[2] && tilemap[3])
	{
		tilemap[0]->transparent_pen = 7;
		tilemap[1]->transparent_pen = 7;
		tilemap[2]->transparent_pen = 7;
		tilemap[3]->transparent_pen = 7;
		tilemap_set_scroll_rows(tilemap[0],1);
		tilemap_set_scroll_cols(tilemap[0],1);
		tilemap_set_scroll_rows(tilemap[1],1);
		tilemap_set_scroll_cols(tilemap[1],1);
		tilemap_set_scroll_rows(tilemap[2],1);
		tilemap_set_scroll_cols(tilemap[2],1);
		tilemap_set_scroll_rows(tilemap[3],1);
		tilemap_set_scroll_cols(tilemap[3],1);

		return 0;
	}

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

void rthunder_tilebank_select_0(int offset,int data)
{
	if (tilebank != 0)
	{
		tilebank = 0;
		tilemap_mark_all_tiles_dirty(tilemap[0]);
		tilemap_mark_all_tiles_dirty(tilemap[1]);
	}
}

void rthunder_tilebank_select_1(int offset,int data)
{
	if (tilebank != 1)
	{
		tilebank = 1;
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
			if (flipscreen)
				tilemap_set_scrollx(tilemap[layer],0,xscroll[layer]+xdisp[layer]-224);
			else
				tilemap_set_scrollx(tilemap[layer],0,-xscroll[layer]-xdisp[layer]);
			break;
		case 1:
			xscroll[layer] = (xscroll[layer]&0xff00)|data;
			if (flipscreen)
				tilemap_set_scrollx(tilemap[layer],0,xscroll[layer]+xdisp[layer]-224);
			else
				tilemap_set_scrollx(tilemap[layer],0,-xscroll[layer]-xdisp[layer]);
			break;
		case 2:
			yscroll[layer] = data;
			if (flipscreen)
				tilemap_set_scrolly(tilemap[layer],0,yscroll[layer]-7);
			else
				tilemap_set_scrolly(tilemap[layer],0,-yscroll[layer]-25);
			break;
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap, int sprite_priority )
{
	/* note: sprites don't yet clip at the top of the screen properly */

	struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const struct rectangle *clip = &Machine->drv->visible_area;

	const unsigned char *source = &spriteram[0x0000];
	const unsigned char *finish = &spriteram[0x0800-16];	/* the last is NOT a sprite */

	while( source<finish )
	{
/*
	source[4]	S--T -TTT
	source[5]	-TTT TTTT
	source[6]   CCCC CCCX
	source[7]	XXXX XXXX
	source[8]	PP-T -S--
	source[9]   YYYY YYYY
*/
		unsigned char priority = source[8];
		if( priority>>6 == sprite_priority )
		{
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

			if ((attrs & 0x10) && !wide) sprite_number += 1;
			if ((priority & 0x10) && !tall) sprite_number += 2;
			color = color>>1;

			if( sx>512-32 ) sx -= 512;

			if( !tall ) sy+=16;
			if( flip && !wide ) sx-=16;

			for( row=0; row<=tall; row++ )
			{
				for( col=0; col<=wide; col++ )
				{
					if (flipscreen)
					{
						drawgfx( bitmap, gfx,
							sprite_number+2*row+col,
							color,
							!flip,1,
							272-(sx+16*(flip?1-col:col)),
							208-(sy+row*16),
							clip,
							TRANSPARENCY_PEN, 0xf );
					}
					else
					{
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
		}
		source+=16;
	}
}

void rthunder_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	flipscreen = spriteram[0x07f6] & 1;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	tilemap0_preupdate(); tilemap_update(tilemap[0]);
	tilemap1_preupdate(); tilemap_update(tilemap[1]);
	tilemap2_preupdate(); tilemap_update(tilemap[2]);
	tilemap3_preupdate(); tilemap_update(tilemap[3]);

	tilemap_render(ALL_TILEMAPS);

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

void wndrmomo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	flipscreen = spriteram[0x07f6] & 1;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	tilemap0_preupdate(); tilemap_update(tilemap[0]);
	tilemap1_preupdate(); tilemap_update(tilemap[1]);
	tilemap2_preupdate(); tilemap_update(tilemap[2]);
	tilemap3_preupdate(); tilemap_update(tilemap[3]);

	tilemap_render(ALL_TILEMAPS);

	/* the background color can be changed but I don't know to which address it's mapped */
	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
	draw_sprites(bitmap,0);
	tilemap_draw(bitmap,tilemap[2],0);
	draw_sprites(bitmap,1);
	tilemap_draw(bitmap,tilemap[0],0);
	draw_sprites(bitmap,2);
	tilemap_draw(bitmap,tilemap[3],0);
	draw_sprites(bitmap,3);
	tilemap_draw(bitmap,tilemap[1],0);
}
