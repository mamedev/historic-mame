#include "driver.h"
#include "vidhrdw/generic.h"
#include "state.h"

static struct tilemap *bg_tilemap, *fg_tilemap;
static int flipscreen;

UINT8 ninjakun_io_8000_ctrl[4];
static UINT8 old_scroll;

/*******************************************************************************
 Tilemap Callbacks
*******************************************************************************/

static void get_fg_tile_info(int tile_index){
	unsigned int tile_number = videoram[tile_index] & 0xFF;
	unsigned char attr  = videoram[tile_index+0x400];
	tile_number += (attr & 0x20) << 3; /* bank */
	SET_TILE_INFO(0,tile_number,(attr&0xf));
}

static void get_bg_tile_info(int tile_index){
	unsigned int tile_number = videoram[tile_index+0x800] & 0xFF;
	unsigned char attr  = videoram[tile_index+0xc00];
	tile_number += (attr & 0xC0) << 2; /* bank */
	SET_TILE_INFO(1,tile_number,(attr&0xf));
}

WRITE_HANDLER( ninjakid_fg_videoram_w ){
	videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset&0x3ff);
}

WRITE_HANDLER( ninjakid_bg_videoram_w ){
	videoram[0x800+offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset&0x3ff);
}

/******************************************************************************/

WRITE_HANDLER( ninjakun_flipscreen_w ){
	flipscreen = data?(TILEMAP_FLIPX|TILEMAP_FLIPY):0;
	tilemap_set_flip( ALL_TILEMAPS,flipscreen );
}

READ_HANDLER( ninjakun_io_8000_r ){
	switch( offset ){
	case 0: /* control */
		return AY8910_read_port_0_r( 0 );

	case 1: /* input read */
		switch( ninjakun_io_8000_ctrl[0] ){
		case 0xf:
			return readinputport(4);
		case 0xe:
			return readinputport(3);
		default:
			return ninjakun_io_8000_ctrl[1];
		}
		break;

	case 2: /* control */
		return AY8910_read_port_1_r( 0 );

	case 3: /* data */
		return ninjakun_io_8000_ctrl[3];
	}

//	logerror("PC=%04x; RAM[0x800%d]\n",cpu_get_pc(),offset);
	return 0xFF;
}

static void handle_scrolly( UINT8 new_scroll ){
/*	HACK!!!
**
**	New rows are always written at fixed locations above and below the background
**	tilemaps, rather than at the logical screen boundaries with respect to scrolling.
**
**  I don't know how this is handled by the actual NinjaKun hardware, but the
**	following is a crude approximation, yielding a playable game.
*/
	int old_row = old_scroll/8;
	int new_row = new_scroll/8;
	int i;
	if( new_scroll!=old_scroll ){
		tilemap_set_scrolly( bg_tilemap, 0, new_scroll&7 );

		if ((new_row == ((old_row - 1) & 0xff)) || ((!old_row) && (new_row == 0x1f)))
		{
			for( i=0x400-0x21; i>=0; i-- ){
				ninjakid_bg_videoram_w( i+0x20, videoram[0x800+i] );
			}
		}
		else if ((new_row == ((old_row + 1) & 0xff)) || ((old_row == 0x1f) && (!new_row)))
		{
			for( i=0x20; i<0x400; i++ ){
				ninjakid_bg_videoram_w( i-0x20, videoram[0x800+i] );
			}
		}

		old_scroll = new_scroll;
	}
}


WRITE_HANDLER( ninjakun_io_8000_w ){
	switch( offset ){
	case 0x0: /* control#1 */
		ninjakun_io_8000_ctrl[0] = data;
		AY8910_control_port_0_w( 0, data );
		break;

	case 0x1: /* data#1 */
		ninjakun_io_8000_ctrl[1] = data;
		switch( ninjakun_io_8000_ctrl[0] ){
		default:
			AY8910_write_port_0_w( 0,data );
			break;
		}
		break;

	case 0x2: /* control#2 */
		ninjakun_io_8000_ctrl[2] = data;
		AY8910_control_port_1_w( 0, data );
		break;

	case 0x3: /* data#2 */
		ninjakun_io_8000_ctrl[3] = data;
		switch( ninjakun_io_8000_ctrl[2] ){
		case 0xf:
			handle_scrolly( data );
			break;
		case 0xe:
			tilemap_set_scrollx( bg_tilemap, 0, data-8 );
			break;
		default:
			AY8910_write_port_1_w( 0,data );
		}
		break;
	}
}

/*******************************************************************************
 Video Hardware Functions
********************************************************************************
 vh_start / vh_stop / vh_refresh
*******************************************************************************/

int ninjakid_vh_start( void ){
    fg_tilemap = tilemap_create( get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32 );
	bg_tilemap = tilemap_create( get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32 );
	tilemap_set_transparent_pen( fg_tilemap,0 );

	/* Save State Support */

	state_save_register_UINT8 ("NK_Video", 0, "ninjakun_io_8000_ctrl", ninjakun_io_8000_ctrl, 4);
	state_save_register_int   ("NK_Video", 0, "flipscreen", &flipscreen);
	state_save_register_UINT8 ("NK_Video", 0, "old_scroll", &old_scroll, 1);

	return 0;
}

static void draw_sprites( struct osd_bitmap *bitmap ){
	const UINT8 *source = spriteram;
	const UINT8 *finish = source+0x800;

	const struct rectangle *clip = &Machine->visible_area;
	const struct GfxElement *gfx = Machine->gfx[2];

	while( source<finish ){
		int tile_number = source[0];
		int sx = source[1];
		int sy = source[2];
		int attr = source[3];
		int flipx = attr&0x10;
		int flipy = attr&0x20;
		int color = 0;

		if( flipscreen ){
			sx = 240-sx;
			sy = 240-sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(
			bitmap,
			gfx,
			tile_number,
			color,
			flipx,flipy,
			sx,sy,
			clip,
			TRANSPARENCY_PEN,0
		);

		source+=0x20;
	}
}

void ninjakid_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	tilemap_update( ALL_TILEMAPS );
	palette_recalc();
	tilemap_draw( bitmap,bg_tilemap,0,0 );
	draw_sprites( bitmap );
	tilemap_draw( bitmap,fg_tilemap,0,0 );
}
