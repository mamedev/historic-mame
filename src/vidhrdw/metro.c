/***************************************************************************

							  -= Metro Games =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows the background
		W		shows the middleground
		E		shows the foreground
		A		shows the sprites

		Keys can be used together!


							[ 3 Scrolling Layers ]

		There is memory for a huge layer, but the actual tilemap
		is a smaller window (of fixed size) carved from anywhere
		inside that layer.

		Tile Size:				  8 x 8 x 4  (later games can switch to
											  8 x 8 x 8 at run time)
		Big Layer Size:			2048 x 2048

		Tilemap Window Size:	 512 x  256

		The tile codes in memory do not map directly to tiles. They
		are indexes into a table (with 0x200 entries) that defines
		a virtual set of tiles for the 3 layers. Each entry in that
		table adds 16 tiles to the set of available tiles, and decides
		their color code.

		Tile code with their msbit set are different as they mean:
		draw a tile filled with a single color (0-1ff)


							[ 512? Zooming Sprites ]

		The sprites are NOT tile based: the "tile" size can vary from
		8 to 64 (independently for width and height) with an 8 pixel
		granularity. The "tile" address is a multiple of 8x8 pixels.

		Each sprite can be shrinked to ~1/4 or enlarged to ~32x following
		an exponential curve of sizes (with one zoom value for both width
		and height)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables that driver has access to: */

data16_t *metro_priority;
data16_t *metro_screenctrl;
data16_t *metro_scroll;
data16_t *metro_tiletable;
data16_t *metro_vram_0, *metro_vram_1, *metro_vram_2;
data16_t *metro_window;



/***************************************************************************


								Palette


***************************************************************************/

WRITE16_HANDLER( paletteram16_GGGGGRRRRRBBBBBx_word_w )
{
	int r,g,b;

	data = COMBINE_DATA(&paletteram16[offset]);

	b = (data >>  1) & 0x1f;
	r = (data >>  6) & 0x1f;
	g = (data >> 11) & 0x1f;

	palette_change_color(offset,(r << 3) | (r >> 2),(g << 3) | (g >> 2),(b << 3) | (b >> 2));
}


/***************************************************************************


						Tilemaps: Tiles Set & Window


***************************************************************************/

/*
 Tilemaps get repeatedly dirtied. We avoid calling tilemap_mark_all_tiles_dirty
 so many times here, and do it once in the screen refresh routine using 3 flags
 in the following variable
*/
static int tmap_dirty;


/* Dirty all tilemaps when the tiles set changes */
WRITE16_HANDLER( metro_tiletable_w )
{
	data16_t olddata = metro_tiletable[offset];
	data16_t newdata = COMBINE_DATA( &metro_tiletable[offset] );
	if ( newdata != olddata )	tmap_dirty |= 1|2|4;
}

/* Dirty the relevant tilemap when its window changes */
WRITE16_HANDLER( metro_window_w )
{
	data16_t olddata = metro_window[offset];
	data16_t newdata = COMBINE_DATA( &metro_window[offset] );
	if ( newdata != olddata )	tmap_dirty |= 1 << (offset/2);
}


/***************************************************************************


							Tilemaps: Rendering


***************************************************************************/

static struct tilemap *tilemap_0, *tilemap_1, *tilemap_2;

/* A 2048 x 2048 virtual tilemap */

#define BIG_NX		(0x100)
#define BIG_NY		(0x100)

/* A smaller 512 x 256 window defines the actual tilemap */

#define WIN_NX		(0x40)
#define WIN_NY		(0x20)



#define METRO_TILEMAP(_n_) \
static void get_tile_info_##_n_( int tile_index ) \
{ \
	data16_t code; \
	int      table_index; \
	UINT32   tile; \
\
	/* The actual tile index depends on the window */ \
	tile_index	=	((tile_index / WIN_NX + metro_window[_n_ * 2 + 0] / 8) % BIG_NY) * BIG_NX + \
					((tile_index % WIN_NX + metro_window[_n_ * 2 + 1] / 8) % BIG_NX); \
\
	/* Fetch the code */ \
	code			=	metro_vram_##_n_[ tile_index ]; \
\
	/* Use it as an index into the tiles set table */ \
	table_index		=	( (code & 0x1ff0) >> 4 ) * 2; \
	tile			=	(metro_tiletable[table_index + 0] << 16 ) + \
						 metro_tiletable[table_index + 1]; \
\
	if (code & 0x8000) /* Special: draw a tile of a single color (e.g. not from the gfx ROMs) */ \
		SET_TILE_INFO( 1, code & 0x000f, ((code & 0x0ff0) >> 4) + 0x100) \
	else \
		SET_TILE_INFO( 0, (tile & 0xfffff) + (code & 0xf), ((tile & 0x0ff00000) >> 20) + 0x100 ); \
} \
\
\
WRITE16_HANDLER( metro_vram_##_n_##_w ) \
{ \
	data16_t olddata = metro_vram_##_n_[offset]; \
	data16_t newdata = COMBINE_DATA(&metro_vram_##_n_[offset]); \
	if ( newdata != olddata ) \
	{ \
		/* Account for the window */ \
		int col		=	(offset % BIG_NX) - (metro_window[_n_ * 2 + 1] / 8); \
		int row		=	(offset / BIG_NY) - (metro_window[_n_ * 2 + 0] / 8); \
		if	( (col >= 0) && (col < WIN_NX) && \
			  (row >= 0) && (row < WIN_NY) ) \
			tilemap_mark_tile_dirty(tilemap_##_n_, row * WIN_NX + col ); \
	} \
}

/* 3 Tilemaps */
METRO_TILEMAP(0)
METRO_TILEMAP(1)
METRO_TILEMAP(2)

/* PRELIMINARY support for 8 bit layers in balcube */
static void get_tile_info_2_8bit( int tile_index )
{
	data16_t code;
	int      table_index;
	UINT32   tile;

	tile_index	=	((tile_index / WIN_NX + metro_window[2 * 2 + 0] / 8) % BIG_NY) * BIG_NX + \
					((tile_index % WIN_NX + metro_window[2  * 2 + 1] / 8) % BIG_NX); \

	code			=	metro_vram_2[ tile_index ]; \
	table_index		=	( (code & 0x1ff0) >> 4 ) * 2; \
	tile			=	(metro_tiletable[table_index + 0] << 16 ) + \
						 metro_tiletable[table_index + 1]; \

	if (tile & 0x8000)
		SET_TILE_INFO( 0, (tile & 0xfffff) + (code & 0xf), ((tile & 0x0ff00000) >> 20) + 0x100 )
	else
		SET_TILE_INFO( 2, (tile & 0xfffff)/2 + (code & 0xf), (tile & 0x01f00000) >> 20 )
}



/***************************************************************************


							Video Init Routines


***************************************************************************/

/*
 Sprites are not tile based, so we decode their graphics at runtime.

 We can't do it at startup because drawgfx requires the tiles to be
 pre-rotated to support vertical games, and that, in turn, requires
 the tile's sizes to be known at startup - which we don't!

 The following variables are needed to support just that..
 for the sake of efficiency we use an hash table, where each entry
 holds the sprite dimensions ("word" field) and a pointer to its
 decoded graphics ("gfx" field).

 When drawing a sprite, we use the low bits of its tile code as an
 index into the hash table, to see if we already decoded its graphics
 (and with the same size).

 If not, we do the (computationally expensive) decoding step.
*/

typedef struct { data16_t word;	struct GfxElement *gfx; }	sprite_t;
static sprite_t *sprite;

int metro_sprite_xoffs, metro_sprite_yoffs;

/*
 This layout is used to decode the sprites' graphics. Some fields have
 to be filled at runtime
*/

static struct GfxLayout sprite_layout =
{
	0,0,	// filled in later
	1,
	4,
	{ STEP4(0,1) },
	{ 1*4,0*4,3*4,2*4,5*4,4*4,7*4,6*4, 9*4,8*4,11*4,10*4,13*4,12*4,15*4,14*4,
	  17*4,16*4,19*4,18*4,21*4,20*4,23*4,22*4, 25*4,24*4,27*4,26*4,29*4,28*4,31*4,30*4,
	  33*4,32*4,35*4,34*4,37*4,36*4,39*4,38*4, 41*4,40*4,43*4,42*4,45*4,44*4,47*4,46*4,
	  49*4,48*4,51*4,50*4,53*4,52*4,55*4,54*4, 57*4,56*4,59*4,58*4,61*4,60*4,63*4,62*4 },
	{ 0 },	// filled in later
	0		// unused
};



int metro_vh_start_14100(void)
{

	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );

	tilemap_2 = tilemap_create(	get_tile_info_2, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );

	sprite = (sprite_t *) malloc(sizeof(sprite_t) * 0x10000);

	if (tilemap_0 && tilemap_1 && tilemap_2 && sprite)
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,15);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,15);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,15);

		metro_sprite_xoffs	=	0 + ((Machine->drv->screen_width == 360) ? 0x34c : 0x362);
		metro_sprite_yoffs	=	0x190;

		memset(sprite, 0, sizeof(sprite_t) * 0x10000);

		return 0;
	}
	else return 1;
}

void metro_vh_stop(void)
{
	if (sprite)
	{
		int i;
		for (i = 0; i < 0x10000; i++)
			freegfx(sprite[i].gfx);
		free(sprite);
	}
}

const struct GameDriver driver_balcube;

int metro_vh_start_14220(void)
{

	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );

	if (Machine->gamedrv			==	&driver_balcube ||
		Machine->gamedrv->clone_of	==	&driver_balcube )
	{
		/* 8 bit tiles for this layer */
		tilemap_2 = tilemap_create(	get_tile_info_2_8bit, tilemap_scan_rows,
									TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );
	}
	else
	{
		tilemap_2 = tilemap_create(	get_tile_info_2, tilemap_scan_rows,
									TILEMAP_TRANSPARENT, 8,8, WIN_NX,WIN_NY );
	}

	sprite = (sprite_t *) malloc(sizeof(sprite_t) * 0x10000);

	if (tilemap_0 && tilemap_1 && tilemap_2 && sprite)
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,15);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,15);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,15);

		metro_sprite_xoffs	=	-2 + ((Machine->drv->screen_width == 360) ? 0x34c : 0x362);
		metro_sprite_yoffs	=	0x190;

		tilemap_set_scrolldx(tilemap_0, -2, 2);
		tilemap_set_scrolldx(tilemap_1, -2, 2);
		tilemap_set_scrolldx(tilemap_2, -2, 2);

		memset(sprite, 0, sizeof(sprite_t) * 0x10000);

		return 0;
	}
	else return 1;
}

void balcube_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int color, pen;

	for( color = 0; color < 0x200; color++ )
		for( pen = 0; pen < 256; pen++ )
			colortable[0x2000 + color * 256 + pen] = (color * 16 + pen) % 0x2000;
}


/***************************************************************************


								Sprites Drawing


		Offset:		Bits:					Value:

		0.w			fedc b--- ---- ----		Priority (0 = Max)
					---- -a98 7654 3210		X

		2.w			fedc ba-- ---- ----		Zoom (Both X & Y)
					---- --98 7654 3210		Y

		4.w			f--- ---- ---- ----		Flip X
					-e-- ---- ---- ----		Flip Y
					--dc b--- ---- ----		Size X *
					---- -a98 ---- ----		Size Y *
					---- ---- 7654 ----		Color
					---- ---- ---- 3210		Code High Bits **

		6.w									Code Low Bits  **

*  8 pixel increments
** 8x8 pixel increments

***************************************************************************/

/* Draw sprites whose priority falls in the min_pri..max_pri range */

void metro_draw_sprites(struct osd_bitmap *bitmap, int min_pri, int max_pri)
{
	const int region		=	REGION_GFX1;

	unsigned char *base_gfx	=	memory_region(region);
	unsigned char *gfx_max	=	base_gfx + memory_region_length(region);

	int max_x				=	Machine->drv->screen_width;
	int max_y				=	Machine->drv->screen_height;

	data16_t *src			=	spriteram16 + (spriteram_size-8)/2;
	data16_t *end			=	spriteram16;

	min_pri = (~min_pri & 0x1f) << (16-5);
	max_pri = (~max_pri & 0x1f) << (16-5);

	if (min_pri > max_pri)
	{
		int temp = min_pri;
		min_pri = max_pri;
		max_pri = temp;
	}

	for ( ; src >= end; src -= 8/2 )
	{
		int x,y, attr,code, flipx,flipy, zoom, pri;
		unsigned char *gfxdata;

		/* Exponential zoom table extracted from daitoride */
		const int zoomtable[0x40] =
		{	0xAAC,0x800,0x668,0x554,0x494,0x400,0x390,0x334,
			0x2E8,0x2AC,0x278,0x248,0x224,0x200,0x1E0,0x1C8,
			0x1B0,0x198,0x188,0x174,0x164,0x154,0x148,0x13C,
			0x130,0x124,0x11C,0x110,0x108,0x100,0x0F8,0x0F0,
			0x0EC,0x0E4,0x0DC,0x0D8,0x0D4,0x0CC,0x0C8,0x0C4,
			0x0C0,0x0BC,0x0B8,0x0B4,0x0B0,0x0AC,0x0A8,0x0A4,
			0x0A0,0x09C,0x098,0x094,0x090,0x08C,0x088,0x080,
			0x078,0x070,0x068,0x060,0x058,0x050,0x048,0x040	};

		x					=	src[ 0 ];
		pri					=	x & 0xf800;
		if ( pri == 0xf800 )					continue;
		if ( (pri<min_pri) || (pri>max_pri) )	continue;
		y					=	src[ 1 ];
		attr				=	src[ 2 ];
		code				=	src[ 3 ];

		flipx				=	attr & 0x8000;
		flipy				=	attr & 0x4000;

		zoom				=	zoomtable[(y & 0xfc00) >> 10] << (16-8);

		x					=	(x & 0x07ff) - metro_sprite_xoffs;
		y					=	(y & 0x03ff) - metro_sprite_yoffs;

		sprite_layout.width			=	(( (attr >> 11) & 0x7 ) + 1 ) * 8;
		sprite_layout.height		=	(( (attr >>  8) & 0x7 ) + 1 ) * 8;

		gfxdata		=	base_gfx + (8*8*4/8) * (((attr & 0x000f) << 16) + code);

		/* Bound checking */
		if ( (gfxdata + sprite_layout.width * sprite_layout.height - 1) >= gfx_max )
			continue;

		/* Decode the sprite's graphics when needed */
		if ( (sprite[code].gfx == 0) || ((sprite[code].word ^ attr) & 0x3f0f) )
		{
			int i,yoffs;

			for (i=0,yoffs=0; i<sprite_layout.height; i++,yoffs+=sprite_layout.width*4)
				sprite_layout.yoffset[i] = yoffs;

			sprite[code].word	=	attr;
			freegfx(sprite[code].gfx);
			sprite[code].gfx	=	decodegfx(gfxdata,&sprite_layout);

			sprite[code].gfx->colortable   = Machine->remapped_colortable;
			sprite[code].gfx->total_colors = 0x200;
		}

		if (flip_screen)
		{
			flipx = !flipx;		x = max_x - x - sprite_layout.width;
			flipy = !flipy;		y = max_y - y - sprite_layout.height;
		}

		drawgfxzoom(	bitmap,sprite[code].gfx,
						0,
						((attr & 0xf0)>>4) + 0x1f0,
						flipx, flipy,
						x, y,
						&Machine->visible_area, TRANSPARENCY_PEN, 0xf,
						zoom, zoom	);
#if 0
{	/* Display priority + zoom on each sprite */
	struct DisplayText dt[2];	char buf[80];
	sprintf(buf, "%02X %02X",((src[ 0 ] & 0xf800) >> 11)^0x1f,((src[ 1 ] & 0xfc00) >> 10) );
    dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
    dt[0].x = x;    dt[0].y = y;    dt[1].text = 0; /* terminate array */
	displaytext(Machine->scrbitmap,dt);		}
#endif
	}
}


void metro_mark_sprites_colors(void)
{
	int count = 0;
	int pen,col,colmask[16];

	unsigned int *pen_usage	=	Machine->gfx[0]->pen_usage;
	int total_elements		=	Machine->gfx[0]->total_elements;

	int max_x				=	Machine->visible_area.max_x;
	int max_y				=	Machine->visible_area.max_y;

	data16_t *src			=	spriteram16 + (spriteram_size-8)/2;
	data16_t *end			=	spriteram16;

	memset(colmask, 0, sizeof(colmask));

	for ( ; src >= end; src -= 8/2 )
	{
		int x				=	src[ 0 ];
		int y				=	src[ 1 ];
		int attr			=	src[ 2 ];
		int code			=	src[ 3 ];

		int n				=	( ( (attr >> 11) & 0x7 ) + 1 ) *
								( ( (attr >>  8) & 0x7 ) + 1 );

		int color			=	( (attr & 0xf0) >> 4 ) /*+ 0x1f0*/;

		if ((x & 0xf800) == 0xf800)		continue;

		code				+=	(attr & 0x000f) << 16;

		x					=	(x & 0x07ff) - metro_sprite_xoffs;
		y					=	(y & 0x03ff) - metro_sprite_yoffs;

		if ((x > max_x) || (y > max_y))	continue;

		while (n--)
			colmask[color] |= pen_usage[(code++) % total_elements];
	}

	for (col = 0; col < 16; col++)
	 for (pen = 0; pen < 15; pen++)	// pen 15 is transparent
	  if (colmask[col] & (1 << pen))
	  {	palette_used_colors[16 * (col + 0x1f0) + pen] = PALETTE_COLOR_USED;
		count++;	}

#if 0
{	char buf[8];	sprintf(buf,"%d",count);	usrintf_showmessage(buf);	}
#endif
}


/***************************************************************************



								Screen Drawing


***************************************************************************/

void metro_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layers_ctrl = -1;

	data16_t screenctrl = *metro_screenctrl;


	/* Black background color ? */
	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);


	/*	Screen Control Register:

		f--- ---- ---- ----
		-edc ---- ---- ----		Layer 0,1,2 Enable
		---- ba98 7654 32--
		---- ---- ---- --1-		Blank Screen
		---- ---- ---- ---0		Flip  Screen	*/
	if (screenctrl & 2)	return;
	tilemap_set_enable(tilemap_0, screenctrl & 0x0100);
	tilemap_set_enable(tilemap_1, screenctrl & 0x0200);
	tilemap_set_enable(tilemap_2, screenctrl & 0x0400);
	flip_screen_set(screenctrl & 1);


	/* Dirty the flagged tilemaps */
	if (tmap_dirty & 1)	{ tilemap_mark_all_tiles_dirty(tilemap_0); tmap_dirty &= ~1; }
	if (tmap_dirty & 2)	{ tilemap_mark_all_tiles_dirty(tilemap_1); tmap_dirty &= ~2; }
	if (tmap_dirty & 4)	{ tilemap_mark_all_tiles_dirty(tilemap_2); tmap_dirty &= ~4; }


	/* Scrolling */
	tilemap_set_scrolly(tilemap_0, 0, metro_scroll[0x0/2] - metro_window[0x0/2] );
	tilemap_set_scrollx(tilemap_0, 0, metro_scroll[0x2/2] - metro_window[0x2/2] );
	tilemap_set_scrolly(tilemap_1, 0, metro_scroll[0x4/2] - metro_window[0x4/2] );
	tilemap_set_scrollx(tilemap_1, 0, metro_scroll[0x6/2] - metro_window[0x6/2] );
	tilemap_set_scrolly(tilemap_2, 0, metro_scroll[0x8/2] - metro_window[0x8/2] );
	tilemap_set_scrollx(tilemap_2, 0, metro_scroll[0xa/2] - metro_window[0xa/2] );

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;

#if 0
{	char buf[1024];
	sprintf(buf, "%X %X %X %X %X %X %X %X\n"
				 "%X %X %X %X %X %X\n"
				 "%X %X %X %X %X %X",

				metro_priority[0x00/2], metro_priority[0x02/2],
				metro_priority[0x04/2], metro_priority[0x06/2],
				metro_priority[0x08/2],
				metro_priority[0x10/2], metro_priority[0x12/2],
				*metro_screenctrl,

				metro_scroll[0x02/2], metro_scroll[0x00/2],
				metro_scroll[0x06/2], metro_scroll[0x04/2],
				metro_scroll[0x0a/2], metro_scroll[0x08/2],

				metro_window[0x02/2], metro_window[0x00/2],
				metro_window[0x06/2], metro_window[0x04/2],
				metro_window[0x0a/2], metro_window[0x08/2] );

	usrintf_showmessage(buf);	}
#endif
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	metro_mark_sprites_colors();

	palette_recalc();

	if (layers_ctrl & 1)	tilemap_draw(bitmap,tilemap_2, 0, 0);
	if (layers_ctrl & 2)	tilemap_draw(bitmap,tilemap_1, 0, 0);
	if (metro_priority[0x02/2] & 0x0100)	// tilemap 0 over sprites
	{
		if (layers_ctrl & 8)	metro_draw_sprites(bitmap, 0x00, 0x1f);
		if (layers_ctrl & 4)	tilemap_draw(bitmap,tilemap_0, 0, 0);
	}
	else
	{
		if (layers_ctrl & 4)	tilemap_draw(bitmap,tilemap_0, 0, 0);
		if (layers_ctrl & 8)	metro_draw_sprites(bitmap, 0x00, 0x1f);
	}
}
