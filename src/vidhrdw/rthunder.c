/*******************************************************************

Rolling Thunder Video Hardware

*******************************************************************/

#include "driver.h"

void rt_stop_mcu_timer( void );


#define GFX_TILES		0
#define GFX_SPRITES	1

extern unsigned char *videoram;
extern unsigned char *spriteram;
extern unsigned char *dirtybuffer;

static int tilebank;
static int xscroll[4], yscroll[4];

static struct osd_bitmap *pixmap[4];
#define LAYER_SIZE (32*64)

void rthunder_tilebank_w( int offset, int data ){
	if( tilebank!=data ){
		tilebank = data;
		memset( dirtybuffer, 1, LAYER_SIZE*2 );
	}
}

void rthunder_scroll_w( int layer, int offset, int data ){
	switch( offset ){
		case 0: xscroll[layer] = (xscroll[layer]&0xff)|(data<<8); break;
		case 1: xscroll[layer] = (xscroll[layer]&0xff00)|data; break;
		case 2: yscroll[layer] = data; break;
	}
}

/*******************************************************************/

void rthunder_vh_convert_color_prom( unsigned char *palette,
	unsigned short *colortable,
	const unsigned char *color_prom ){

	int i;

	for( i=0; i<4096; i++ ){
		colortable[i] = *color_prom++;
	}

	for( i = 0; i<256; i++ ){
		unsigned char byte0 = color_prom[i];
		unsigned char byte1 = color_prom[i+256];

		unsigned char red = byte0&0x0f;
		unsigned char blue = byte1;
		unsigned char green = byte0>>4;

		*(palette++) = red | (red<<4);
		*(palette++) = green | (green<<4);
		*(palette++) = blue | (blue<<4);
	}
}

int rthunder_vh_start( void ){
	pixmap[0] = osd_create_bitmap(64*8,32*8);
	if( pixmap[0] ){
		pixmap[1] = osd_create_bitmap(64*8,32*8);
		if( pixmap[1] ){
			pixmap[2] = osd_create_bitmap(64*8,32*8);
			if( pixmap[2] ){
				pixmap[3] = osd_create_bitmap(64*8,32*8);
				if( pixmap[3] ){
					dirtybuffer = malloc(LAYER_SIZE*4);
					if( dirtybuffer ){
						memset( dirtybuffer, 1, LAYER_SIZE*4 );
						return 0;
					}
					osd_free_bitmap( pixmap[3] );
				}
				osd_free_bitmap( pixmap[2] );
			}
			osd_free_bitmap( pixmap[1] );
		}
		osd_free_bitmap( pixmap[0] );
	}
	return 1;
}

void rthunder_vh_stop( void ){
	free( dirtybuffer );
	osd_free_bitmap( pixmap[0] );
	osd_free_bitmap( pixmap[1] );
	osd_free_bitmap( pixmap[2] );
	osd_free_bitmap( pixmap[3] );

	rt_stop_mcu_timer();
}

static void draw_sprites( struct osd_bitmap *bitmap, int sprite_priority ){
	/* note: sprites don't yet clip at the top of the screen properly */

	struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const struct rectangle *clip = &Machine->drv->visible_area;

	const unsigned char *source = &spriteram[0x1400];
	const unsigned char *finish = &spriteram[0x1c00];

	while( source<finish ){
/*
	source[4]	S--T -TTT
	source[5]	-TTT TTTT
	source[6]   CCCC CCCX
	source[7]	XXXX XXXX
	source[8]	PP-T -S--
	source[9]   YYYY YYYY
*/
		unsigned char priority = source[8];
		if( priority>>6 == sprite_priority ) {
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

			if( attrs&0x10 ) sprite_number+=1;
			if( priority&0x10 ) sprite_number+=2;
			color = color>>1;

			if( sx>512-32 ) sx -= 512;

			if( !tall ) sy+=16;
			if( flip && !wide ) sx-=16;
			for( row=0; row<=tall; row++ ){
				for( col=0; col<=wide; col++ ){
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
		source+=16;
	}
}

static void update_layer( struct osd_bitmap *bitmap, unsigned char *source,
	unsigned char *dirty, int tile_offset, int opaque ){

	struct GfxElement *gfx = Machine->gfx[GFX_TILES];

	int offs;
	for( offs=0; offs<LAYER_SIZE; offs++ ){
		if( dirty[offs] ){
			int sx = (offs%64)*8;
			int sy = (offs/64)*8;
			unsigned char tile_number = source[offs*2];
			unsigned char color = source[(offs*2) | 1];

			dirty[offs] = 0;

			if( !opaque ){
				int x,y;
				for( y=sy; y<sy+8; y++ )
				for( x=sx; x<sx+8; x++ )
					bitmap->line[y][x] = 0;//palette_transparent_pen;
			}
			drawgfx( bitmap, gfx,
				tile_number + tile_offset + (color&0x03)*256,					color,
				0,0, // no flip
				sx,sy,
				0,
				opaque?TRANSPARENCY_NONE:TRANSPARENCY_PEN, 0x7 );
		}
	}
}

void rthunder_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	const struct rectangle *clip = &Machine->drv->visible_area;

	{
		int scrollx = -xscroll[0]-20;
		int scrolly = -yscroll[0]-25;
		update_layer( pixmap[0], &videoram[0x0000], &dirtybuffer[LAYER_SIZE*0], 0*0x400+tilebank*0x800, 1 );
		copyscrollbitmap(bitmap,pixmap[0],
			1,&scrollx, 1,&scrolly,
			clip,TRANSPARENCY_NONE,0);
	}

	draw_sprites( bitmap, 1 );

	{
		int scrollx = -xscroll[0]-20;
		int scrolly = -yscroll[0]-25;
		update_layer( pixmap[1], &videoram[0x1000], &dirtybuffer[LAYER_SIZE*1], 1*0x400+tilebank*0x800, 0 );
		copyscrollbitmap(bitmap,pixmap[1],
			1,&scrollx, 1,&scrolly,
			clip,TRANSPARENCY_PEN,0);//palette_transparent_pen);
	}

	draw_sprites( bitmap, 2 );

	{
		int scrollx = -xscroll[0]-20;
		int scrolly = -yscroll[0]-25;
		update_layer( pixmap[2], &videoram[0x2000], &dirtybuffer[LAYER_SIZE*2], 4*0x400, 0 );
		copyscrollbitmap(bitmap,pixmap[2],
			1,&scrollx, 1,&scrolly,
			clip,TRANSPARENCY_PEN,0);//palette_transparent_pen);
	}

	draw_sprites( bitmap, 3 );

	{
		int scrollx = -xscroll[3]-20;
		int scrolly = -yscroll[3]-25;
		update_layer( pixmap[3], &videoram[0x3000], &dirtybuffer[LAYER_SIZE*3], 5*0x400, 0 );
		copyscrollbitmap(bitmap,pixmap[3],
			1,&scrollx, 1,&scrolly,
			clip,TRANSPARENCY_PEN,0);//palette_transparent_pen);
	}
}
