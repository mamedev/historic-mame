/* vidhrdw/twin16.c

	Known Issues:
		sprite-tilemap priority isn't right (see Devil's World intro)
		some rogue sprites in Devil's World

	to do:
		8BPP support
		assorted optimizations
*/

#define USE_16BIT

#include "vidhrdw/generic.h"

extern data16_t twin16_custom_vidhrdw;
extern data16_t *twin16_gfx_rom;
extern data16_t *twin16_fixram;
extern data16_t *twin16_sprite_gfx_ram;
extern data16_t *twin16_tile_gfx_ram;
extern void twin16_gfx_decode( void );
extern int twin16_spriteram_process_enable( void );

static int need_process_spriteram;
static data16_t gfx_bank;
static data16_t scrollx[3], scrolly[3];
static data16_t video_register;

enum {
	TWIN16_SCREEN_FLIPY		= 0x01,	/* ? breaks devils world text layer */
	TWIN16_SCREEN_FLIPX		= 0x02,	/* confirmed: Hard Puncher Intro */
	TWIN16_UNKNOWN1			= 0x04,	/* ?Hard Puncher uses this */
	TWIN16_PLANE_ORDER		= 0x08,	/* confirmed: Devil Worlds */
	TWIN16_TILE_FLIPY		= 0x20	/* confirmed? Vulcan Venture */
};

int twin16_vh_start( void ){
	return 0;
}

void twin16_vh_stop( void ){
}

/******************************************************************************************/

WRITE16_HANDLER( fround_gfx_bank_w ){
	COMBINE_DATA(&gfx_bank);
}

WRITE16_HANDLER( twin16_video_register_w ){
	switch( offset ){
		case 0x0: COMBINE_DATA( &video_register ); break;
		case 0x1: COMBINE_DATA( &scrollx[0] ); break;
		case 0x2: COMBINE_DATA( &scrolly[0] ); break;
		case 0x3: COMBINE_DATA( &scrollx[1] ); break;
		case 0x4: COMBINE_DATA( &scrolly[1] ); break;
		case 0x5: COMBINE_DATA( &scrollx[2] ); break;
		case 0x6: COMBINE_DATA( &scrolly[2] ); break;

		case 0x7:
			logerror("unknown video_register write:%d", data );
		break;
	}
}

/******************************************************************************************/

static void draw_text( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->visible_area;
	const data16_t *source = twin16_fixram;
	int i;

	int tile_flipx = 0;
	int tile_flipy = 0;
	//if( video_register&TWIN16_SCREEN_FLIPY ) tile_flipy = !tile_flipy;
	if( video_register&TWIN16_SCREEN_FLIPX ) tile_flipx = !tile_flipx;

	for( i=0; i<64*64; i++ ){
		int code = source[i];

		int sx = (i%64)*8;
		int sy = (i/64)*8;

		if( video_register&TWIN16_SCREEN_FLIPY ) sy = 256-8 - sy;
		if( video_register&TWIN16_SCREEN_FLIPX ) sx = 320-8 - sx;

		drawgfx( bitmap, Machine->gfx[0],
			code&0x1ff, /* tile number */
			(code>>9)&0xf, /* color */
			tile_flipx,tile_flipy,
			sx,sy,
			clip,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************************/

static void draw_sprite( /* slow slow slow, but it's ok for now */
	struct osd_bitmap *bitmap,
	const UINT16 *pen_data,
	const UINT32 *pal_data,
	int xpos, int ypos,
	int width, int height,
	int flipx, int flipy ){

	int x,y;
	if( xpos>=320 ) xpos -= 512;
	if( ypos>=256 ) ypos -= 512;

	if( xpos+width>=0 && xpos<320 && ypos+height>16 && ypos<256-16 )
	{
		if (bitmap->depth == 16)
		{
			for( y=0; y<height; y++ )
			{
				int sy = (flipy)?(ypos+height-1-y):(ypos+y);
				if( sy>=16 && sy<256-16 )
				{
					UINT16 *dest = (UINT16 *)bitmap->line[sy];
					for( x=0; x<width; x++ )
					{
						int sx = (flipx)?(xpos+width-1-x):(xpos+x);
						if( sx>=0 && sx<320 )
						{
							UINT16 pen = pen_data[x/4];
							switch( x%4 )
							{
							case 0: pen = pen>>12; break;
							case 1: pen = (pen>>8)&0xf; break;
							case 2: pen = (pen>>4)&0xf; break;
							case 3: pen = pen&0xf; break;
							}
							if( pen ) dest[sx] = pal_data[pen];
						}
					}
				}
				pen_data += width/4;
			}
		}
		else
		{
			for( y=0; y<height; y++ )
			{
				int sy = (flipy)?(ypos+height-1-y):(ypos+y);
				if( sy>=16 && sy<256-16 )
				{
					UINT8 *dest = (UINT8 *)bitmap->line[sy];
					for( x=0; x<width; x++ )
					{
						int sx = (flipx)?(xpos+width-1-x):(xpos+x);
						if( sx>=0 && sx<320 )
						{
							UINT16 pen = pen_data[x/4];
							switch( x%4 )
							{
							case 0: pen = pen>>12; break;
							case 1: pen = (pen>>8)&0xf; break;
							case 2: pen = (pen>>4)&0xf; break;
							case 3: pen = pen&0xf; break;
							}
							if( pen ) dest[sx] = pal_data[pen];
						}
					}
				}
				pen_data += width/4;
			}
		}
	}
}

void twin16_spriteram_process( void ){
	data16_t dx = scrollx[0];
	data16_t dy = scrolly[0];

	const data16_t *source = &spriteram16[0x0000];
	const data16_t *finish = &spriteram16[0x1800];

	memset( &spriteram16[0x1800], 0, 0x800 );
	while( source<finish ){
		data16_t priority = source[0];
		if( priority & 0x8000 ){
			data16_t *dest = &spriteram16[0x1800 + 4*(priority&0xff)];

			INT32 xpos = (0x10000*source[4])|source[5];
			INT32 ypos = (0x10000*source[6])|source[7];

			data16_t attributes = source[2]&0x03ff; /* scale,size,color */
			if( priority & 0x0200 ) attributes |= 0x4000;
			attributes |= 0x8000;

			dest[0] = source[3]; /* gfx data */
			dest[1] = ((xpos>>8) - dx)&0xffff;
			dest[2] = ((ypos>>8) - dy)&0xffff;
			dest[3] = attributes;
		}
		source += 0x50/2;
	}
	need_process_spriteram = 0;
}

/*
 * Sprite Format
 * ----------------------------------
 *
 * Word | Bit(s)           | Use
 * -----+-fedcba9876543210-+----------------
 *   0  | --xxxxxxxxxxxxxx | code
 * -----+------------------+
 *   1  | -------xxxxxxxxx | ypos
 * -----+------------------+
 *   2  | -------xxxxxxxxx | xpos
 * -----+------------------+
 *   3  | x--------------- | enble
 *   3  | -x-------------- | priority?
 *   3  | ------x--------- | yflip?
 *   3  | -------x-------- | xflip
 *   3  | --------xx------ | height
 *   3  | ----------xx---- | width
 *   3  | ------------xxxx | color
 */

static void draw_sprites( struct osd_bitmap *bitmap, int pri ){
	int count = 0;
	const data16_t *source = 0x1800+spriteram16;
	const data16_t *finish = source + 0x800;
	pri = pri?0x4000:0x0000;

	while( source<finish ){
		data16_t attributes = source[3];
		data16_t code = source[0];

		if( code!=0xffff && (attributes&0x8000) && (attributes&0x4000)==pri ){
			int xpos = source[1];
			int ypos = source[2];

			const UINT32 *pal_data = Machine->pens+((attributes&0xf)+0x10)*16;
			int height	= 16<<((attributes>>6)&0x3);
			int width	= 16<<((attributes>>4)&0x3);
			const UINT16 *pen_data = 0;
			int flipy = attributes&0x0200;
			int flipx = attributes&0x0100;

			if( twin16_custom_vidhrdw ){
				pen_data = twin16_gfx_rom + 0x80000;
			}
			else {
				switch( (code>>12)&0x3 ){ /* bank select */
					case 0:
					pen_data = twin16_gfx_rom;
					break;

					case 1:
					pen_data = twin16_gfx_rom + 0x40000;
					break;

					case 2:
					pen_data = twin16_gfx_rom + 0x80000;
					if( code&0x4000 ) pen_data += 0x40000;
					break;

					case 3:
					pen_data = twin16_sprite_gfx_ram;
					break;
				}
				code &= 0xfff;
			}
			pen_data += code*0x40;

			if( video_register&TWIN16_SCREEN_FLIPY ){
				ypos = 256-ypos-height;
				flipy = !flipy;
			}
			if( video_register&TWIN16_SCREEN_FLIPX ){
				xpos = 320-xpos-width;
				flipx = !flipx;
			}

			//if( sprite_which==count || !keyboard_pressed( KEYCODE_B ) )
			draw_sprite( bitmap, pen_data, pal_data, xpos, ypos, width, height, flipx, flipy );
		}
			count++;
		source += 4;
	}
}

static void show_video_register( struct osd_bitmap *bitmap ){
#if 0
	int n;
	for( n=0; n<4; n++ ){
		drawgfx( bitmap, Machine->uifont,
			"0123456789abcdef"[(video_register>>(12-4*n))&0xf],
			0,
			0,0,
			n*6+8,16,
			0,TRANSPARENCY_NONE,0);
	}
#endif
}

static void draw_layer( struct osd_bitmap *bitmap, int opaque ){
	const data16_t *gfx_base;
	const data16_t *source = videoram16;
	int i, y1, y2, yd;
	int bank_table[4];
	int dx, dy, palette;
	int tile_flipx = 0; // video_register&TWIN16_TILE_FLIPX;
	int tile_flipy = video_register&TWIN16_TILE_FLIPY;

	if( ((video_register&TWIN16_PLANE_ORDER)?1:0) != opaque ){
		source += 0x1000;
		dx = scrollx[2];
		dy = scrolly[2];
		palette = 1;
	}
	else {
		source += 0x0000;
		dx = scrollx[1];
		dy = scrolly[1];
		palette = 0;
	}

	if( twin16_custom_vidhrdw ){
		gfx_base = twin16_gfx_rom;
		bank_table[3] = (gfx_bank>>(4*3))&0xf;
		bank_table[2] = (gfx_bank>>(4*2))&0xf;
		bank_table[1] = (gfx_bank>>(4*1))&0xf;
		bank_table[0] = (gfx_bank>>(4*0))&0xf;
	}
	else {
		gfx_base = twin16_tile_gfx_ram;
		bank_table[0] = 0;
		bank_table[1] = 1;
		bank_table[2] = 2;
		bank_table[3] = 3;
	}

	if( video_register&TWIN16_SCREEN_FLIPX ){
		dx = 256-dx-64;
		tile_flipx = !tile_flipx;
	}

	if( video_register&TWIN16_SCREEN_FLIPY ){
		dy = 256-dy;
		tile_flipy = !tile_flipy;
	}

	if( tile_flipy ){
		y1 = 7; y2 = -1; yd = -1;
	}
	else {
		y1 = 0; y2 = 8; yd = 1;
	}

	for( i=0; i<64*64; i++ ){
		int sx = (i%64)*8;
		int sy = (i/64)*8;
		int xpos,ypos;

		if( video_register&TWIN16_SCREEN_FLIPX ) sx = 63*8 - sx;
		if( video_register&TWIN16_SCREEN_FLIPY ) sy = 63*8 - sy;

		xpos = (sx-dx)&0x1ff;
		ypos = (sy-dy)&0x1ff;
		if( xpos>=320 ) xpos -= 512;
		if( ypos>=256 ) ypos -= 512;

		if(  xpos>-8 && ypos>8 && xpos<320 && ypos<256-16 ){
			int code = source[i];
			/*
				xxx-------------	color
				---xx-----------	tile bank
				-----xxxxxxxxxxx	tile number
			*/
			const data16_t *gfx_data = gfx_base + (code&0x7ff)*16 + bank_table[(code>>11)&0x3]*0x8000;
			int color = (code>>13);
			UINT32 *pal_data = Machine->pens + 16*(0x20+color+8*palette);

			{
				int y;
				data16_t data;
				int pen;

				if( tile_flipx )
				{
					if( opaque )
					{
						if (bitmap->depth == 16)
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								dest[7] = pal_data[(data>>4*3)&0xf];
								dest[6] = pal_data[(data>>4*2)&0xf];
								dest[5] = pal_data[(data>>4*1)&0xf];
								dest[4] = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								dest[3] = pal_data[(data>>4*3)&0xf];
								dest[2] = pal_data[(data>>4*2)&0xf];
								dest[1] = pal_data[(data>>4*1)&0xf];
								dest[0] = pal_data[(data>>4*0)&0xf];
							}
						}
						else
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT8 *dest = ((UINT8 *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								dest[7] = pal_data[(data>>4*3)&0xf];
								dest[6] = pal_data[(data>>4*2)&0xf];
								dest[5] = pal_data[(data>>4*1)&0xf];
								dest[4] = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								dest[3] = pal_data[(data>>4*3)&0xf];
								dest[2] = pal_data[(data>>4*2)&0xf];
								dest[1] = pal_data[(data>>4*1)&0xf];
								dest[0] = pal_data[(data>>4*0)&0xf];
							}
						}
					}
					else
					{
						if (bitmap->depth == 16)
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT8 *dest = ((UINT8 *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[7] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[6] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[5] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[4] = pal_data[pen];
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[3] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[2] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[1] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[0] = pal_data[pen];
								}
							}
						}
						else
						{
							for( y=y1; y!=y2; y+=yd )
							{
								data16_t *dest = ((data16_t *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[7] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[6] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[5] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[4] = pal_data[pen];
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[3] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[2] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[1] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[0] = pal_data[pen];
								}
							}
						}
					}
				}
				else
				{
					if( opaque )
					{
						if (bitmap->depth == 16)
						{
							for( y=y1; y!=y2; y+=yd )
							{
								data16_t *dest = ((data16_t *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];
							}
						}
						else
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT8 *dest = ((UINT8 *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];
							}
						}
					}
					else
					{
						if (bitmap->depth == 16)
						{
							for( y=y1; y!=y2; y+=yd )
							{
								data16_t *dest = ((data16_t *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[0] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[1] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[2] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[3] = pal_data[pen];
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[4] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[5] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[6] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[7] = pal_data[pen];
								}
							}
						}
						else
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT8 *dest = ((UINT8 *)bitmap->line[ypos+y])+xpos;
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[0] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[1] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[2] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[3] = pal_data[pen];
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) dest[4] = pal_data[pen];
									pen = (data>>4*2)&0xf; if( pen ) dest[5] = pal_data[pen];
									pen = (data>>4*1)&0xf; if( pen ) dest[6] = pal_data[pen];
									pen = (data>>4*0)&0xf; if( pen ) dest[7] = pal_data[pen];
								}
							}
						}
					}
				}
			}
		}
	}
}

static void mark_used_colors( void ){
	const data16_t *source, *finish;
	unsigned char used[0x40], *used_ptr;
	memset( used, 0, 0x40 );

	/* text layer */
	used_ptr = &used[0x00];
	source = twin16_fixram;
	finish = source + 0x1000;
	while( source<finish ){
		int code = *source++;
		used_ptr[(code>>9)&0xf] = 1;
	}

	/* sprites */
	used_ptr = &used[0x10];
	source = 0x1800+spriteram16;
	finish = source + 0x800;
	while( source<finish ){
		data16_t attributes = source[3];
		data16_t code = source[0];
		if( code!=0xffff && (attributes&0x8000) ) used_ptr[attributes&0xf] = 1;
		source++;
	}

	/* plane#0 */
	used_ptr = &used[0x20];
	source = videoram16;
	finish = source+0x1000;
	while( source<finish ){
		int code = *source++;
		used_ptr[code>>13] = 1;
	}

	/* plane#1 */
	used_ptr = &used[0x28];
	source = 0x1000+videoram16;
	finish = source+0x1000;
	while( source<finish ){
		int code = *source++;
		used_ptr[code>>13] = 1;
	}

	{
		int i;
		memset( palette_used_colors, PALETTE_COLOR_UNUSED, 0x400 );
		for( i=0; i<0x40; i++ ){
			if( used[i] ){
				memset( &palette_used_colors[i*16], PALETTE_COLOR_VISIBLE, 0x10 );
			}
		}
	}
}

void twin16_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	if( twin16_spriteram_process_enable() && need_process_spriteram ) twin16_spriteram_process();
	need_process_spriteram = 1;

	mark_used_colors();
	palette_recalc();

	draw_layer( bitmap,1 );
	draw_sprites( bitmap, 1 );
	draw_layer( bitmap,0 );
	draw_sprites( bitmap, 0 );
	draw_text( bitmap );

	show_video_register( bitmap );
}
