/*	video hardware for Taito Grand Champion */

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 grchamp_videoreg0;
UINT8 grchamp_player_xpos;
UINT8 grchamp_player_ypos;
int grchamp_collision;

static UINT8 grchamp_tile_number;
static UINT8 grchamp_rain_xpos;
static UINT8 grchamp_rain_ypos;
static int palette_bank;
static struct osd_bitmap *headlight_bitmap;
static struct osd_bitmap *work_bitmap;

UINT8 grchamp_vreg1[0x10];	/* background control registers */
UINT8 *grchamp_videoram;	/* background tilemaps */
UINT8 *grchamp_radar;		/* bitmap for radar */

struct tilemap *tilemap[3];

WRITE_HANDLER( grchamp_player_xpos_w )
{
	grchamp_player_xpos = data;
}

WRITE_HANDLER( grchamp_player_ypos_w )
{
	grchamp_player_ypos = data;
}

WRITE_HANDLER( grchamp_tile_select_w )
{
	/* tile select: bits 4..7:rain; bits 0..3:player car */
	grchamp_tile_number = data;
}

WRITE_HANDLER( grchamp_rain_xpos_w )
{
	grchamp_rain_xpos = data;
}

WRITE_HANDLER( grchamp_rain_ypos_w )
{
	grchamp_rain_ypos = data;
}

void grchamp_convert_color_prom( UINT8 *palette, UINT16 *colortable, const UINT8 *color_prom )
{
	int i;
	for( i=0; i<0x20; i++ )
	{
		UINT8 data = *color_prom++;
		int bit0,bit1,bit2;
		/* red component */
		bit0 = (data >> 0) & 0x01;
		bit1 = (data >> 1) & 0x01;
		bit2 = (data >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (data >> 3) & 0x01;
		bit1 = (data >> 4) & 0x01;
		bit2 = (data >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (data >> 6) & 0x01;
		bit1 = (data >> 7) & 0x01;
		*(palette++) = 0x4f * bit0 + 0xa8 * bit1;
	}

	for( i=0; i<0x20; i++ )
	{
		int data = *color_prom++;
		int r = (data&4)?0:1;
		int g = (data&2)?0:1;
		int b = (data&1)?0:1;
		int intensity = (data&0x08)?0x55:0xff;
		*palette++ = r*intensity;
		*palette++ = g*intensity;
		*palette++ = b*intensity;
	}
	/* add a fake entry for fog */
	*palette++ = 0x55;
	*palette++ = 0x55;
	*palette++ = 0x55;

	memset( palette, 0x00, 3*3 );
}

WRITE_HANDLER( grchamp_videoram_w )
{
	if( grchamp_videoram[offset]!=data )
	{
		grchamp_videoram[offset] = data;
		tilemap_mark_tile_dirty( tilemap[offset/0x800], offset%0x800 );
	}
}

static void get_bg0_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset];
	SET_TILE_INFO(1,tile_number,palette_bank)
}

static void get_bg1_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset+0x800]+256;
	SET_TILE_INFO(1,tile_number,palette_bank)
}

static void get_bg2_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset+0x800*2]+256*2;
	SET_TILE_INFO(1,tile_number,0)
}

static UINT32 get_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	int offset = (31-row)*32;
	offset += 31-(col%32);
	if( col/32 ) offset += 0x400;
	return offset;
}

int grchamp_vh_start( void )
{
	headlight_bitmap = osd_alloc_bitmap( 64,128,Machine->scrbitmap->depth );
	if( headlight_bitmap ){
		work_bitmap = osd_alloc_bitmap( 32,32,Machine->scrbitmap->depth );
		if( work_bitmap ){
			tilemap[0] = tilemap_create(get_bg0_tile_info,get_memory_offset,TILEMAP_OPAQUE,8,8,64,32);
			tilemap[1] = tilemap_create(get_bg1_tile_info,get_memory_offset,TILEMAP_TRANSPARENT,8,8,64,32);
			tilemap[2] = tilemap_create(get_bg2_tile_info,get_memory_offset,TILEMAP_TRANSPARENT,8,8,64,32);
			if( tilemap[0] && tilemap[1] && tilemap[2] )
			{
				tilemap_set_transparent_pen( tilemap[1], 0 );
				tilemap_set_transparent_pen( tilemap[2], 0 );
				return 0;
			}
			osd_free_bitmap( work_bitmap );
		}
		osd_free_bitmap( headlight_bitmap );
	}
	return 1;
}

void grchamp_vh_stop( void )
{
	osd_free_bitmap( headlight_bitmap );
	osd_free_bitmap( work_bitmap );
}

static void draw_text( struct osd_bitmap *bitmap )
{
	const struct rectangle *clip = &Machine->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];
	const UINT8 *source = videoram;
	int bank = (grchamp_videoreg0&0x20)?256:0;
	int offs;
	for( offs=0; offs<0x400; offs++ )
	{
		int col = offs%32;
		int row = offs/32;
		int scroll = colorram[col*2]-1;
		int attributes = colorram[col*2+1];
		int tile_number = source[offs];

		drawgfx( bitmap, gfx,
			bank + tile_number,
			attributes,
			0,0, /* no flip */
			8*col,
			(8*row-scroll)&0xff,
			clip,
			TRANSPARENCY_PEN, 0 );
	}
}

static void draw_background( struct osd_bitmap *bitmap )
{
	int dx = -48;
	int dy = 16;
	int attributes = grchamp_vreg1[0x3];
		/*	----xxxx	Analog Tachometer output
		**	---x----	palette select
		**	--x-----	enables msb of bg#3 xscroll
		**	xx------	unused
		*/

	int color = (attributes&0x10)?1:0;
	if( color!=palette_bank )
	{
		palette_bank = color;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}

	tilemap_set_scrollx( tilemap[0], 0, dx-(grchamp_vreg1[0x0]+grchamp_vreg1[0x1]*256) );
	tilemap_set_scrolly( tilemap[0], 0, dy - grchamp_vreg1[0x2] );
	tilemap_set_scrollx( tilemap[1], 0, dx-(grchamp_vreg1[0x5]+grchamp_vreg1[0x6]*256) );
	tilemap_set_scrolly( tilemap[1], 0, dy - grchamp_vreg1[0x7] );
	tilemap_set_scrollx( tilemap[2], 0, dx-(grchamp_vreg1[0x9]+ ((attributes&0x20)?256:(grchamp_vreg1[0xa]*256))));
	tilemap_set_scrolly( tilemap[2], 0, dy - grchamp_vreg1[0xb] );
	tilemap_update(ALL_TILEMAPS);

	palette_recalc();

	tilemap_draw(bitmap,tilemap[0],0,0);
	tilemap_draw(bitmap,tilemap[1],0,0);
	tilemap_draw(bitmap,tilemap[2],0,0);
}

static void draw_player_car( struct osd_bitmap *bitmap )
{
	drawgfx( bitmap,
		Machine->gfx[2],
		grchamp_tile_number&0xf,
		1, /* color = red */
		0,0, /* flip */
		256-grchamp_player_xpos,
		240-grchamp_player_ypos,
		&Machine->visible_area,
		TRANSPARENCY_PEN, 0 );
}

static int collision_check( struct osd_bitmap *bitmap, int which )
{
	int bgcolor = Machine->pens[0];
	int sprite_transp = Machine->pens[0x24];
	const struct rectangle *clip = &Machine->visible_area;
	int y0 = 240-grchamp_player_ypos;
	int x0 = 256-grchamp_player_xpos;
	int x,y;
	int result = 0;

	if( which==0 )
	{
		/* draw the current player sprite into a work bitmap */
		drawgfx( work_bitmap, Machine->gfx[2],
			grchamp_tile_number&0xf,
			1, /* color */
			0,0,
			0,0,
			0,
			TRANSPARENCY_NONE, 0 );
	}

	for( y = 0; y <32; y++ )
	{
		for( x = 0; x<32; x++ )
		{
			if( read_pixel(work_bitmap,x,y) != sprite_transp ){
				int sx = x+x0;
				int sy = y+y0;
				if( sx >= clip->min_x && sx <= clip->max_x &&
					sy >= clip->min_y && sy <= clip->max_y )
				{
					if( read_pixel(bitmap, sx, sy) != bgcolor )
					{
						result = 1; /* flag collision */

						/*	wipe this pixel, so collision checks with the
						**	next layer work */
						plot_pixel( bitmap, sx, sy, bgcolor );
					}
				}
			}
        }
	}
	return result?(1<<which):0;
}

static void draw_rain( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[4];
	const struct rectangle *clip = &Machine->visible_area;
	int tile_number = grchamp_tile_number>>4;
	if( tile_number ){
		int scrollx = grchamp_rain_xpos;
		int scrolly = grchamp_rain_ypos;
		int sx,sy;
		for( sy=0; sy<256; sy+=16 ){
			for( sx=0; sx<256; sx+=16 ){
				drawgfx( bitmap, gfx,
					tile_number, 0,
					0,0,
					(sx+scrollx)&0xff,(sy+scrolly)&0xff,
					clip,
					TRANSPARENCY_PEN, 0 );
			}
		}
	}
}

static void draw_fog( struct osd_bitmap *bitmap, int bFog ){
	int x0 = 256-grchamp_player_xpos-64;
	int y0 = 240-grchamp_player_ypos-64;
	int color = Machine->pens[bFog?0x40:0x00];

	copybitmap(
		headlight_bitmap, /* dest */
		bitmap, /* source */
		0,0, /* flipx,flipy */
		-x0,-y0, /* sx,sy */
		0, /* clip */
		TRANSPARENCY_NONE,0 );

	fillbitmap( bitmap,color,0 );
}

static void draw_headlights( struct osd_bitmap *bitmap, int bFog )
{
	int sx, sy, color;
	int x0 = 256-grchamp_player_xpos-64;
	int y0 = 240-grchamp_player_ypos-64;
	const UINT8 *source = memory_region( REGION_GFX4 );
	int x,y,bit;
	if( !bFog ) source += 0x400;
	for( y=0; y<128; y++ )
	{
		for( x=0; x<64; x+=8 )
		{
			int data = *source++;
			if( data )
			{
				for( bit=0; bit<8; bit++ )
				{
					if( data&0x80 ){
						sx = x0+x+bit;
						sy = y0+y;
						if( sx>=0 && sy>=0 && sx<=255 && sy<=255 )
						{
							color = read_pixel( headlight_bitmap, x+bit, y );
							plot_pixel( bitmap, sx,sy, color );
						}
					}
					data <<= 1;
				}
			}
		}
	}
}

static void draw_radar( struct osd_bitmap *bitmap ){
	const UINT8 *source = grchamp_radar;
	int color = Machine->pens[3];
	int offs;
	for( offs=0; offs<0x400; offs++ ){
		int data = source[offs];
		if( data ){
			int x = (offs%32)*8;
			int y = (offs/32)+16;
			int bit;
			for( bit=0; bit<8; bit++ ){
				if( data&0x80 ) plot_pixel( bitmap, x+bit, y, color );
				data <<= 1;
			}
		}
	}
}

static void draw_tachometer( struct osd_bitmap *bitmap ){
/*
	int value = grchamp_vreg1[0x03]&0xf;
	int i;
	for( i=0; i<value; i++ ){
		drawgfx( bitmap, Machine->uifont,
			'*',
			0,
			0,0,
			i*6+32,64,
			0,
			TRANSPARENCY_NONE, 0 );
	}
*/
}

static void draw_sprites( struct osd_bitmap *bitmap, int bFog ){
	const struct GfxElement *gfx = Machine->gfx[3];
	const struct rectangle *clip = &Machine->visible_area;
	int bank = (grchamp_videoreg0&0x20)?0x40:0x00;
	const UINT8 *source = spriteram;
	const UINT8 *finish = source+0x40;
	while( source<finish ){
		int sx = source[3];
		int sy = 240-source[0];
		int color = bFog?8:source[2];
		int code = source[1];
		drawgfx( bitmap, gfx,
			bank + (code&0x3f),
			color,
			code&0x40,code&0x80,
			sx,sy,
			clip,
			TRANSPARENCY_PEN, 0 );
		source += 4;
	}
}

void grchamp_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh ){
	int bFog = grchamp_videoreg0&0x40;

	draw_background( bitmap ); /* 3 layers */

	grchamp_collision = collision_check( bitmap,0 );
	draw_sprites( bitmap, 0 ); /* computer cars */
	grchamp_collision |= collision_check( bitmap,1 );

	draw_player_car( bitmap );

	if( grchamp_videoreg0&(0x10|0x40) ){
		draw_fog( bitmap,bFog ); /* grey fog / black tunnel darkness */
	}

	/* fog covered sprites look like black shadows */
	if( bFog ) draw_sprites( bitmap, bFog );

	/* paint the visible area exposed by headlights shape */
	if( grchamp_videoreg0&(0x10|0x40) ){
		draw_headlights( bitmap,bFog );
	}

	draw_rain( bitmap );
	draw_text( bitmap );
	if( grchamp_videoreg0&0x80 ) draw_radar( bitmap );
	draw_tachometer( bitmap );
}
