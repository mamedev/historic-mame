/*
**	Video Driver for Taito Samurai (1985)
*/

#include "driver.h"
#include "vidhrdw/generic.h"

/*
** prototypes
*/
WRITE_HANDLER( tsamurai_bgcolor_w );
WRITE_HANDLER( tsamurai_textbank1_w );
WRITE_HANDLER( tsamurai_textbank2_w );
WRITE_HANDLER( tsamurai_scrolly_w );
WRITE_HANDLER( tsamurai_scrollx_w );

WRITE_HANDLER( tsamurai_bg_videoram_w );
WRITE_HANDLER( tsamurai_fg_videoram_w );

void tsamurai_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void tsamurai_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
int tsamurai_vh_start( void );

/*
** variables
*/
unsigned char *tsamurai_videoram;
static int bgcolor;
static int textbank1, textbank2;

static struct tilemap *background, *foreground;


/*
** color prom decoding
*/

void tsamurai_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	unsigned char attributes = tsamurai_videoram[2*tile_index+1];
	int tile_number = tsamurai_videoram[2*tile_index];
	tile_number += (( attributes & 0xc0 ) >> 6 ) * 256;	 /* legacy */
	tile_number += (( attributes & 0x20 ) >> 5 ) * 1024; /* Mission 660 add-on*/
	SET_TILE_INFO(
			0,
			tile_number,
			attributes & 0x1f,
			0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile_number = videoram[tile_index];
	if (textbank1 & 0x01) tile_number += 256; /* legacy */
	if (textbank2 & 0x01) tile_number += 512; /* Mission 660 add-on */
	SET_TILE_INFO(
			1,
			tile_number,
			colorram[((tile_index&0x1f)*2)+1] & 0x1f,
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int tsamurai_vh_start(void)
{
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	foreground = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!background || !foreground)
		return 1;

	tilemap_set_transparent_pen(background,0);
	tilemap_set_transparent_pen(foreground,0);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( tsamurai_scrolly_w )
{
	tilemap_set_scrolly( background, 0, data );
}

WRITE_HANDLER( tsamurai_scrollx_w )
{
	tilemap_set_scrollx( background, 0, data );
}

WRITE_HANDLER( tsamurai_bgcolor_w )
{
	bgcolor = data;
}

WRITE_HANDLER( tsamurai_textbank1_w )
{
	if( textbank1!=data )
	{
		textbank1 = data;
		tilemap_mark_all_tiles_dirty( foreground );
	}
}

WRITE_HANDLER( tsamurai_textbank2_w )
{
	if( textbank2!=data )
	{
		textbank2 = data;
		tilemap_mark_all_tiles_dirty( foreground );
	}
}

WRITE_HANDLER( tsamurai_bg_videoram_w )
{
	if( tsamurai_videoram[offset]!=data )
	{
		tsamurai_videoram[offset]=data;
		offset = offset/2;
		tilemap_mark_tile_dirty(background,offset);
	}
}
WRITE_HANDLER( tsamurai_fg_videoram_w )
{
	if( videoram[offset]!=data )
	{
		videoram[offset]=data;
		tilemap_mark_tile_dirty(foreground,offset);
	}
}
WRITE_HANDLER( tsamurai_fg_colorram_w )
{
	if( colorram[offset]!=data )
	{
		colorram[offset]=data;
		if (offset & 1)
		{
			int col = offset/2;
			int row;
			for (row = 0;row < 32;row++)
				tilemap_mark_tile_dirty(foreground,32*row+col);
		}
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap )
{
	struct GfxElement *gfx = Machine->gfx[2];
	const struct rectangle *clip = &Machine->visible_area;
	const unsigned char *source = spriteram+32*4-4;
	const unsigned char *finish = spriteram; /* ? */
	static int flicker;
	flicker = 1-flicker;

	while( source>=finish )
	{
		int attributes = source[2]; /* bit 0x10 is usually, but not always set */

		int sx = source[3] - 16;
		int sy = 240-source[0];
		int sprite_number = source[1];
		int color = attributes&0x1f;
		//color = 0x2d - color; nunchakun fix?
		if( sy<-16 ) sy += 256;

		if( flip_screen )
		{
			drawgfx( bitmap,gfx,
				sprite_number&0x7f,
				color,
				1,(sprite_number&0x80)?0:1,
				256-32-sx,256-32-sy,
				clip,TRANSPARENCY_PEN,0 );
		}
		else
		{
			drawgfx( bitmap,gfx,
				sprite_number&0x7f,
				color,
				0,sprite_number&0x80,
				sx,sy,
				clip,TRANSPARENCY_PEN,0 );
		}

		source -= 4;
	}
}

void tsamurai_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
	int i;

/* Do the column scroll used for the "660" logo on the title screen */
	tilemap_set_scroll_cols(foreground, 32);
	for (i = 0 ; i < 32 ; i++)
	{
		tilemap_set_scrolly(foreground, i, colorram[i*2]);
	}
/* end of column scroll code */

	/*
		This following isn't particularly efficient.  We'd be better off to
		dynamically change every 8th palette to the background color, so we
		could draw the background as an opaque tilemap.

		Note that the background color register isn't well understood
		(screenshots would be helpful)
	*/
	fillbitmap(bitmap,Machine->pens[bgcolor],&Machine->visible_area);
	tilemap_draw(bitmap,background,0,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,foreground,0,0);
}

/***************************************************************************

VS Gong Fight runs on older hardware

***************************************************************************/

int vsgongf_color;

WRITE_HANDLER( vsgongf_color_w )
{
	if( vsgongf_color != data )
	{
		vsgongf_color = data;
		tilemap_mark_all_tiles_dirty( foreground );
	}
}

static void get_vsgongf_tile_info(int tile_index)
{
	int tile_number = videoram[tile_index];
	int color = vsgongf_color&0x1f;
	if( textbank1 ) tile_number += 0x100;
	SET_TILE_INFO(
			1,
			tile_number,
			color,
			0)
}

int vsgongf_vh_start(void)
{
	foreground = tilemap_create(get_vsgongf_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	if (!foreground) return 1;
	return 0;
}

void vsgongf_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
	static int k;
	if( keyboard_pressed( KEYCODE_Q ) ){
		while( keyboard_pressed( KEYCODE_Q ) ){}
		k++;
		vsgongf_color = k;
		tilemap_mark_all_tiles_dirty( foreground );
	}

	tilemap_draw(bitmap,foreground,0,0);
	draw_sprites(bitmap);
}
