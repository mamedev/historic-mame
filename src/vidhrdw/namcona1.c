/*	Namco System NA1/2 Video Hardware */
#include "vidhrdw/generic.h"

void namcona1_vh_stop( void ); /* forward reference */

/* public variables */
data16_t *namcona1_vreg;
data16_t *namcona1_scroll;
data16_t *namcona1_workram;

extern int cgang_hack( void );

static char *dirtychar;
static char dirtygfx;
static data16_t *shaperam;
static data16_t *cgram;

static struct tilemap *tilemap[4];
static int tilemap_palette_bank[4];

static const data16_t *tilemap_videoram;
static int tilemap_color;

static void tilemap_get_info( int tile_index )
{
	static UINT8 mask_data[8];
	data16_t *source;

	int data = tilemap_videoram[tile_index];
	int tile = data&0xfff;
	SET_TILE_INFO( 0,tile,tilemap_color );

#ifdef LSB_FIRST
/* hack for tilemap manager */
	source = shaperam+4*tile;
	mask_data[0] = source[0]>>8;
	mask_data[1] = source[0]&0xff;
	mask_data[2] = source[1]>>8;
	mask_data[3] = source[1]&0xff;
	mask_data[4] = source[2]>>8;
	mask_data[5] = source[2]&0xff;
	mask_data[6] = source[3]>>8;
	mask_data[7] = source[3]&0xff;
	tile_info.mask_data = mask_data;
#else
	tile_info.mask_data = (UINT8 *)(shaperam+4*tile);
#endif
}

/*************************************************************************/

WRITE16_HANDLER( namcona1_videoram_w )
{
	COMBINE_DATA( &videoram16[offset] );
	tilemap_mark_tile_dirty( tilemap[offset/0x1000], offset&0xfff );
}

READ16_HANDLER( namcona1_videoram_r )
{
	return videoram16[offset];
}

/*************************************************************************/

READ16_HANDLER( namcona1_paletteram_r )
{
	return paletteram16[offset];
}

WRITE16_HANDLER( namcona1_paletteram_w )
{
	int r,g,b;
	COMBINE_DATA( &paletteram16[offset] );
	/* -RRRRRGGGGGBBBBB */
	r = (paletteram16[offset]>>10)&0x1f;
	g = (paletteram16[offset]>>5)&0x1f;
	b = paletteram16[offset]&0x001f;

	palette_change_color( offset,
		(r<<3)|(r>>2),
		(g<<3)|(g>>2),
		(b<<3)|(b>>2));
}

/*************************************************************************/

static struct GfxLayout shape_layout =
{
	8,8,
	0x1000,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
#ifdef LSB_FIRST
	{ 8*1,8*0,8*3,8*2,8*5,8*4,8*7,8*6 },
#else
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
#endif
	8*8
};

static struct GfxLayout cg_layout =
{
	8,8,
	0x1000,
	8, /* 8BPP */
	{ 0,1,2,3,4,5,6,7 },
#ifdef LSB_FIRST
	{ 8*1,8*0,8*3,8*2,8*5,8*4,8*7,8*6 },
#else
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
#endif
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7 },
	64*8
};

READ16_HANDLER( namcona1_gfxram_r )
{
	data16_t type = namcona1_vreg[0x0c/2];
	if( type == 0x03 ){
		if( offset<0x4000 )
		{
			return shaperam[offset];
		}
	}
	else if( type == 0x02 )
	{
		return cgram[offset];
	}
	return 0x0000;
}

WRITE16_HANDLER( namcona1_gfxram_w )
{
	data16_t type = namcona1_vreg[0x0c/2];
	data16_t old_word;
	if( type == 0x03 ){
		if( offset<0x4000 )
		{
			old_word = shaperam[offset];
			COMBINE_DATA( &shaperam[offset] );
			if( shaperam[offset]!=old_word )
			{
				dirtygfx = 1;
				dirtychar[offset/4] = 1;
			}
		}
	}
	else if( type == 0x02 ){
		old_word = cgram[offset];
		COMBINE_DATA( &cgram[offset] );
		if( cgram[offset]!=old_word )
		{
			dirtygfx = 1;
			dirtychar[offset/0x20] = 1;
		}
	}
}

static void update_gfx( void )
{
	if( dirtygfx )
	{
		int i;
		for( i = 0;i < 0x1000;i++ )
		{
			if( dirtychar[i] )
			{
				dirtychar[i] = 0;
				decodechar(Machine->gfx[0],i,(UINT8 *)cgram,&cg_layout);
				decodechar(Machine->gfx[1],i,(UINT8 *)shaperam,&shape_layout);
			}
		}
		dirtygfx = 0;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}
}

int namcona1_vh_start( void )
{
	int i;
	for( i=0; i<4; i++ )
	{
		tilemap[i] = tilemap_create(
			tilemap_get_info,
			tilemap_scan_rows,
			TILEMAP_BITMASK,8,8,64,32 );

		if( tilemap[i]==NULL ) return 1; /* error */
		tilemap_palette_bank[i] = -1;
	}

	shaperam = malloc( 0x1000*4*2 );
	cgram = malloc( 0x1000*0x20*2 );
	dirtychar = malloc( 0x1000 );

	if( shaperam && cgram && dirtychar )
	{
		struct GfxElement *gfx0 = decodegfx( (UINT8 *)cgram,&cg_layout );
		struct GfxElement *gfx1 = decodegfx( (UINT8 *)shaperam,&shape_layout );

		if( gfx0 && gfx1 )
		{
			gfx0->colortable = Machine->remapped_colortable;
			gfx0->total_colors = Machine->drv->color_table_len/256;
			Machine->gfx[0] = gfx0;

			gfx1->colortable = Machine->remapped_colortable;
			gfx1->total_colors = Machine->drv->color_table_len/2;
			Machine->gfx[1] = gfx1;

			return 0;
		}
	}

	namcona1_vh_stop();
	return -1; /* error */
}

void namcona1_vh_stop( void )
{
	free( shaperam );
	free( cgram );
	free( dirtychar );
}

/*************************************************************************/

static void pdraw_masked_tile(
		struct osd_bitmap *bitmap,
		int code,
		int color,
		int sx, int sy,
		int flipx, int flipy,
		int priority )
{
	/*
	**	custom blitter for drawing a masked 8x8x8BPP tile
	**	- doesn't yet handle screen orientation (needed particularly for F/A, a vertical game)
	**	- assumes there is an 8 pixel overdraw region on the screen for clipping
	*/
	const struct GfxElement *gfx = Machine->gfx[0];
	const struct GfxElement *mask = Machine->gfx[1];

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if( sx > -8 &&
		sy > -8 &&
		sx < bitmap->width &&
		sy < bitmap->height ) /* all-or-nothing clip */
	{
		const UINT32 *paldata = &gfx->colortable[gfx->color_granularity * color];
		data8_t *gfx_addr = gfx->gfxdata + code * gfx->char_modulo;
		int gfx_pitch = gfx->line_modulo;

		data8_t *mask_addr = mask->gfxdata + code * mask->char_modulo;
		int mask_pitch = mask->line_modulo;

		int x,y;

		if( Machine->color_depth == 8 )
		{
			for( y=0; y<8; y++ )
			{
				int ypos = sy+(flipy?7-y:y);
				data8_t *pri = (data8_t *)priority_bitmap->line[ypos];
				data8_t *dest = bitmap->line[ypos];
				if( flipx )
				{
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{
							if( priority>=pri[-x] ) dest[-x] = paldata[gfx_addr[x]];
							pri[-x] = 0xff;
						}
					}
				}
				else
				{
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{
							if( priority>=pri[x] ) dest[x] = paldata[gfx_addr[x]];
							pri[x] = 0xff;
						}
					}
				}
				gfx_addr += gfx_pitch;
				mask_addr += mask_pitch;
			}
		}
		else
		{ /* 16 bit color */
			for( y=0; y<8; y++ )
			{
				int ypos = sy+(flipy?7-y:y);
				data8_t *pri = (data8_t *)priority_bitmap->line[ypos];
				UINT16 *dest = (UINT16 *)bitmap->line[ypos];
				if( flipx )
				{
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[-x] ) dest[-x] = paldata[gfx_addr[x]];
							pri[-x] = 0xff;
						}
					}
				}
				else
				{
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[x] ) dest[x] = paldata[gfx_addr[x]];
							pri[x] = 0xff;
						}
					}
				}
				gfx_addr += gfx_pitch;
				mask_addr += mask_pitch;
			}
		}
	}
}

static const data8_t pri_mask[8] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f };

static void draw_sprites( struct osd_bitmap *bitmap )
{
	int which;
	const data16_t *source = spriteram16;
	if( namcona1_vreg[0x22/2]&1 ) source += 0x400;

	for( which=0; which<0x100; which++ )
	{ /* 256 sprites */
		data16_t ypos = source[0];	/* FHHH---- YYYYYYYY */
		data16_t tile = source[1];	/* ----TTTT TTTTTTTT */
		data16_t color = source[2];	/* FSWW---- CCCC-PPP	flipx, shadow, width, color, pri*/
		data16_t xpos = source[3];	/* -------X XXXXXXXX */
		/*
			unmapped bits likely include xscale and yscale
		*/

		int priority = pri_mask[color&0x7];
		int width = ((color>>12)&0x3)+1;
		int height = ((ypos>>12)&0x7)+1;
		int flipy = ypos&0x8000;
		int flipx = color&0x8000;
		int row,col;

		int skip = 0;

		if( !skip )
		for( row=0; row<height; row++ )
		{
			int sy = (ypos&0x1ff)-30;
			if( flipy )
			{
				sy += (height-1-row)*8;
			}
			else
			{
				sy += row*8;
			}
			for( col=0; col<width; col++ )
			{
				int sx = (xpos&0x1ff)-10;
				if( flipx )
				{
					sx += (width-1-col)*8;
				}
				else
				{
					sx+=col*8;
				}
				pdraw_masked_tile(
					bitmap,
					tile + row*64+col,
					(color>>4)&0xf,
					((sx+16)&0x1ff)-8,
					((sy+8)&0x1ff)-8,
					flipx,flipy,
					priority );
			}
		}
		source += 4;
	}
}

static int count_scroll_rows( const data16_t *source )
{
	if( source[0x00]==0 ) return 0;
	if( source[0x1f]==0 ) return 1;
	if( source[0xff]==0 ) return 0x20;
	return 0x100;
}

void namcona1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;
	int priority;
//	int flipscreen = namcona1_vreg[0x98/2];
	/* cocktail not yet implemented */

	if( keyboard_pressed( KEYCODE_Z ) )
	{
		while( keyboard_pressed( KEYCODE_Z ) ){}
		for( i=0; i<0x100/2; i++ )
		{
			if( i%8 == 0 ) logerror( "\n%02x: ",i );
			if( namcona1_vreg[i] )
			{
				logerror( "%04x ", namcona1_vreg[i] );
			}
			else {
				logerror( "---- " );
			}
		}
		logerror( "\n" );
	}

	update_gfx();
	palette_init_used_colors();

	for( i=0; i<4; i++ )
	{
		const data16_t *source = namcona1_scroll+0x200*i;
		int scroll_rows, scrolly;

		if( cgang_hack() )
		{
			/* Scrolling isn't well understood.  Each layer may
			 * be configured to one of the following modes:
			 * - no scrolling
			 * - simple XY scrolling
			 * - row/quarter scrolling?
			 * - line scroll
			 *
			 * There is also a ROZ feature not yet hooked up.
			 *
			 * The following hack fixes Cosmo Gang:
			 */
			scroll_rows = 0;
			scrolly = 0x41e0;
		}
		else
		{
			scroll_rows = count_scroll_rows( source );
			scrolly = source[0x100];
		}

		tilemap_set_scrolly( tilemap[i], 0, scrolly-0x41e0 );

		if( scroll_rows )
		{
			int adjust = i*2+0x41c6;
			int line;
			tilemap_set_scroll_rows( tilemap[i], scroll_rows );
			for( line=0; line<scroll_rows; line++ ){
				int value = source[line]-adjust;
				tilemap_set_scrollx( tilemap[i], line, value );
			}
		}
		else {
			tilemap_set_scroll_rows( tilemap[i], 1 );
			tilemap_set_scrollx( tilemap[i], 0, -8 );
		}

		tilemap_videoram = i*0x1000+(data16_t *)videoram16;
		tilemap_color = namcona1_vreg[0x58+i]&0xf;
		if( tilemap_color!=tilemap_palette_bank[i] ){
			tilemap_mark_all_tiles_dirty( tilemap[i] );
			tilemap_palette_bank[i] = tilemap_color;
		}
		tilemap_update( tilemap[i] );
	}

	palette_recalc();

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap( bitmap,Machine->pens[0],&Machine->visible_area ); /* ? */

	for( priority = 0; priority<8; priority++ )
	{
		int which;
		for( which=3; which>=0; which-- )
		{
			if( (which!=0 || !keyboard_pressed( KEYCODE_Q ) ) &&
				(which!=1 || !keyboard_pressed( KEYCODE_W ) ) &&
				(which!=2 || !keyboard_pressed( KEYCODE_E ) ) &&
				(which!=3 || !keyboard_pressed( KEYCODE_R ) ) )
			{

				if( namcona1_vreg[0x50+which] == priority )
				{
					tilemap_draw(
						bitmap,
						tilemap[which],
						0,
						pri_mask[priority] );
				}
			}
		}
	}

	if( !keyboard_pressed( KEYCODE_T ) ) draw_sprites( bitmap );
	/* Q,W,E,R,T are debug keys hooked up to hide individual
	 *tilemap layers or sprites.
	 */
}
