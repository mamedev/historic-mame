#include "driver.h"
#include "vidhrdw/generic.h"


data16_t armedf_vreg;

data16_t *terraf_text_videoram;
data16_t *armedf_bg_videoram;
data16_t *armedf_fg_videoram;
static data16_t armedf_fg_scrollx,armedf_fg_scrolly;

data16_t terraf_scroll_msb;

static struct tilemap *bg_tilemap, *fg_tilemap, *tx_tilemap;

static int scroll_type,sprite_offy;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 terraf_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	if (col < 32)
		return 32*(31-row)+col;
	else
		return 32*(31-row)+(col-32)+0x800;
}


static void terraf_get_tx_tile_info(int tile_index)
{
	int attributes = terraf_text_videoram[tile_index+0x400]&0xff;
	int tile_number = terraf_text_videoram[tile_index]&0xff;
	SET_TILE_INFO(0,tile_number + 256 * (attributes & 0x3),attributes >> 4);
}

static void get_tx_tile_info(int tile_index)
{
	int attributes = terraf_text_videoram[tile_index+0x800]&0xff;
	int tile_number = terraf_text_videoram[tile_index]&0xff;
	SET_TILE_INFO(0,tile_number + 256 * (attributes & 0x3),attributes >> 4);
}

static void get_fg_tile_info( int tile_index )
{
	int data = armedf_fg_videoram[tile_index];
	SET_TILE_INFO( 1, data&0x7ff, data>>11 );
}


static void get_bg_tile_info( int tile_index )
{
	int data = armedf_bg_videoram[tile_index];
	SET_TILE_INFO( 2, data&0x3ff, data>>11 );
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int terraf_vh_start(void)
{
	scroll_type = 0;
	sprite_offy = 128;

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(terraf_get_tx_tile_info,terraf_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(tx_tilemap,0xf);

	tilemap_set_scrollx(tx_tilemap,0,-128);

	return 0;
}

int cclimbr2_vh_start(void)
{
	scroll_type = 2;
	sprite_offy = 0;

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(terraf_get_tx_tile_info,terraf_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(tx_tilemap,0xf);

	tilemap_set_scrollx(tx_tilemap,0,-128);

	return 0;
}

int kodure_vh_start(void)
{
	scroll_type = 2;
	sprite_offy = 128;

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(terraf_get_tx_tile_info,terraf_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(tx_tilemap,0xf);

	tilemap_set_scrollx(tx_tilemap,0,-128);

	return 0;
}

int armedf_vh_start(void)
{
	scroll_type = 1;
	sprite_offy = 128;

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(tx_tilemap,0xf);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( terraf_text_videoram_w )
{
	int oldword = terraf_text_videoram[offset];
	COMBINE_DATA(&terraf_text_videoram[offset]);
	if (oldword != terraf_text_videoram[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0xbff);
}

WRITE16_HANDLER( armedf_text_videoram_w )
{
	int oldword = terraf_text_videoram[offset];
	COMBINE_DATA(&terraf_text_videoram[offset]);
	if (oldword != terraf_text_videoram[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0x7ff);
}


WRITE16_HANDLER( armedf_fg_videoram_w )
{
	int oldword = armedf_fg_videoram[offset];
	COMBINE_DATA(&armedf_fg_videoram[offset]);
	if (oldword != armedf_fg_videoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( armedf_bg_videoram_w )
{
	int oldword = armedf_bg_videoram[offset];
	COMBINE_DATA(&armedf_bg_videoram[offset]);
	if (oldword != armedf_bg_videoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE16_HANDLER( terraf_fg_scrollx_w )
{
	if (ACCESSING_MSB)
		armedf_fg_scrollx = data >> 8;
}

static int waiting_msb;

WRITE16_HANDLER( terraf_fg_scrolly_w )
{
	if (ACCESSING_MSB)
	{
		if (waiting_msb)
		{
			terraf_scroll_msb = data >> 8;
			waiting_msb = 0;
		}
		else
			armedf_fg_scrolly = data >> 8;
	}
}

WRITE16_HANDLER( terraf_fg_scroll_msb_arm_w )
{
	if (ACCESSING_MSB)
		waiting_msb = 1;
}

WRITE16_HANDLER( armedf_fg_scrollx_w )
{
	COMBINE_DATA(&armedf_fg_scrollx);
}

WRITE16_HANDLER( armedf_fg_scrolly_w )
{
	COMBINE_DATA(&armedf_fg_scrolly);
}

WRITE16_HANDLER( armedf_bg_scrollx_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( armedf_bg_scrolly_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(bg_tilemap,0,scroll);
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap, int priority )
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int code = buffered_spriteram16[offs+1]; /* ??YX?TTTTTTTTTTT */
		int flipx = code & 0x2000;
		int flipy = code & 0x1000;
		int color = (buffered_spriteram16[offs+2]>>8)&0x1f;
		int sx = buffered_spriteram16[offs+3];
		int sy = sprite_offy+240-(buffered_spriteram16[offs+0]&0x1ff);

		if (((buffered_spriteram16[offs+0] & 0x3000) >> 12) == priority)
		{
			drawgfx(bitmap,Machine->gfx[3],
				code & 0xfff,
				color,
 				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}
}

static void mark_sprite_colors( void )
{
	int offs;
	int i;
	char flag[32];

	for (i = 0;i < 32;i++) flag[i] = 0;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int color = (buffered_spriteram16[offs+2]>>8)&0x1f;
		flag[color] = 1;
	}

	{
		unsigned char *pen_ptr = &palette_used_colors[Machine->drv->gfxdecodeinfo[3].color_codes_start];
		int pen;
		for (i = 0;i < 32;i++)
		{
			if (flag[i])
			{
				for (pen = 0;pen < 0xf;pen++)
					pen_ptr[pen] |= PALETTE_COLOR_VISIBLE;
			}
			pen_ptr += 16;
		}
	}
}


void armedf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int sprite_enable = armedf_vreg & 0x200;

	tilemap_set_enable( bg_tilemap, armedf_vreg&0x800 );
	tilemap_set_enable( fg_tilemap, armedf_vreg&0x400 );
	tilemap_set_enable( tx_tilemap, armedf_vreg&0x100 );

	switch (scroll_type)
	{
		case	0:		/* terra force */
			tilemap_set_scrollx( fg_tilemap, 0, armedf_fg_scrolly + ((terraf_scroll_msb>>4)&3)*256 );
			tilemap_set_scrolly( fg_tilemap, 0, armedf_fg_scrollx + ((terraf_scroll_msb)&3)*256 );
			break;

		case	1:		/* armed formation */
			tilemap_set_scrollx( fg_tilemap, 0, armedf_fg_scrollx );
			tilemap_set_scrolly( fg_tilemap, 0, armedf_fg_scrolly );
			break;

		case	2:		/* kodure ookami, crazy climber 2 */
			{
				int scrollx,scrolly;

				/* scrolling is handled by the protection mcu */
				scrollx = (terraf_text_videoram[13] & 0xff) | (terraf_text_videoram[14] << 8);
				scrolly = (terraf_text_videoram[11] & 0xff) | (terraf_text_videoram[12] << 8);
				tilemap_set_scrollx( fg_tilemap, 0, scrollx);
				tilemap_set_scrolly( fg_tilemap, 0, scrolly);
			}
			break;
	}

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprite_colors();
	palette_recalc();

	if( armedf_vreg & 0x0800 )
		tilemap_draw( bitmap, bg_tilemap, 0 ,0);
	else
		fillbitmap( bitmap, palette_transparent_pen, 0 ); /* disabled bg_tilemap - all black? */

	if( sprite_enable ) draw_sprites( bitmap, 2 );
	tilemap_draw( bitmap, fg_tilemap, 0 ,0);
	if( sprite_enable ) draw_sprites( bitmap, 1 );
	tilemap_draw( bitmap, tx_tilemap, 0 ,0);
	if( sprite_enable ) draw_sprites( bitmap, 0 );
}

void armedf_eof_callback(void)
{
	buffer_spriteram16_w(0,0,0);
}
