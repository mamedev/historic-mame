/***************************************************************************

	D-Con video hardware.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *dcon_back_data,*dcon_fore_data,*dcon_mid_data,*dcon_scroll_ram,*dcon_textram;

static struct tilemap *background_layer,*foreground_layer,*midground_layer,*text_layer;
static int dcon_enable;

/******************************************************************************/

WRITE16_HANDLER( dcon_control_w )
{
	if (ACCESSING_LSB)
	{
		dcon_enable=data;
		if ((dcon_enable&4)==4)
			tilemap_set_enable(foreground_layer,0);
		else
			tilemap_set_enable(foreground_layer,1);

		if ((dcon_enable&2)==2)
			tilemap_set_enable(midground_layer,0);
		else
			tilemap_set_enable(midground_layer,1);

		if ((dcon_enable&1)==1)
			tilemap_set_enable(background_layer,0);
		else
			tilemap_set_enable(background_layer,1);
	}
}

WRITE16_HANDLER( dcon_background_w )
{
	int oldword = dcon_back_data[offset];
	COMBINE_DATA(&dcon_back_data[offset]);
	if (oldword != dcon_back_data[offset])
		tilemap_mark_tile_dirty(background_layer,offset);
}

WRITE16_HANDLER( dcon_foreground_w )
{
	int oldword = dcon_fore_data[offset];
	COMBINE_DATA(&dcon_fore_data[offset]);
	if (oldword != dcon_fore_data[offset])
		tilemap_mark_tile_dirty(foreground_layer,offset);
}

WRITE16_HANDLER( dcon_midground_w )
{
	int oldword = dcon_mid_data[offset];
	COMBINE_DATA(&dcon_mid_data[offset]);
	if (oldword != dcon_mid_data[offset])
		tilemap_mark_tile_dirty(midground_layer,offset);
}

WRITE16_HANDLER( dcon_text_w )
{
	int oldword = dcon_textram[offset];
	COMBINE_DATA(&dcon_textram[offset]);
	if (oldword != dcon_textram[offset])
		tilemap_mark_tile_dirty(text_layer,offset);
}

static void get_back_tile_info(int tile_index)
{
	int tile=dcon_back_data[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void get_fore_tile_info(int tile_index)
{
	int tile=dcon_fore_data[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
}

static void get_mid_tile_info(int tile_index)
{
	int tile=dcon_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO(
			3,
			tile,
			color,
			0)
}

static void get_text_tile_info(int tile_index)
{
	int tile = dcon_textram[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

int dcon_vh_start(void)
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}

static void draw_sprites(struct mame_bitmap *bitmap,int pri)
{
	int offs,fx,fy,x,y,color,sprite;
	int dx,dy,ax,ay;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		if ((spriteram16[offs+0]&0x8000)!=0x8000) continue;
		sprite = spriteram16[offs+1];
		if ((sprite>>14)!=pri) continue;
		sprite &= 0x3fff;

		y = spriteram16[offs+3];
		x = spriteram16[offs+2];

		if (x&0x8000) x=0-(0x200-(x&0x1ff));
		else x&=0x1ff;
		if (y&0x8000) y=0-(0x200-(y&0x1ff));
		else y&=0x1ff;

		color = spriteram16[offs+0]&0x3f;
		fx = 0; /* To do */
		fy = 0; /* To do */
		dy=((spriteram16[offs+0]&0x0380)>>7)+1;
		dx=((spriteram16[offs+0]&0x1c00)>>10)+1;

		for (ax=0; ax<dx; ax++)
			for (ay=0; ay<dy; ay++) {
				drawgfx(bitmap,Machine->gfx[4],
				sprite++,
				color,fx,fy,x+ax*16,y+ay*16,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
			}
	}
}

void dcon_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer,0, dcon_scroll_ram[0] );
	tilemap_set_scrolly( background_layer,0, dcon_scroll_ram[1] );
	tilemap_set_scrollx( midground_layer, 0, dcon_scroll_ram[2] );
	tilemap_set_scrolly( midground_layer, 0, dcon_scroll_ram[3] );
	tilemap_set_scrollx( foreground_layer,0, dcon_scroll_ram[4] );
	tilemap_set_scrolly( foreground_layer,0, dcon_scroll_ram[5] );

	if ((dcon_enable&1)!=1)
		tilemap_draw(bitmap,background_layer,0,0);
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	draw_sprites(bitmap,2);
	tilemap_draw(bitmap,midground_layer,0,0);
	draw_sprites(bitmap,1);
	tilemap_draw(bitmap,foreground_layer,0,0);
	draw_sprites(bitmap,0);
	draw_sprites(bitmap,3);
	tilemap_draw(bitmap,text_layer,0,0);
}
