#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *wc90b_shared, *wc90b_palette;

unsigned char *wc90b_tile_colorram, *wc90b_tile_videoram;
unsigned char *wc90b_tile_colorram2, *wc90b_tile_videoram2;
unsigned char *wc90b_scroll1xlo, *wc90b_scroll1xhi;
unsigned char *wc90b_scroll2xlo, *wc90b_scroll2xhi;
unsigned char *wc90b_scroll1ylo, *wc90b_scroll1yhi;
unsigned char *wc90b_scroll2ylo, *wc90b_scroll2yhi;

int wc90b_tile_videoram_size;
int wc90b_tile_videoram_size2;

static int palettedirty = 1;

static unsigned char *dirtybuffer1 = 0, *dirtybuffer2 = 0;
static struct osd_bitmap *tmpbitmap1 = 0,*tmpbitmap2 = 0;

int wc90b_vh_start( void ) {

	if ( generic_vh_start() )
		return 1;

	if ( ( dirtybuffer1 = malloc( wc90b_tile_videoram_size ) ) == 0 ) {
		return 1;
	}

	memset( dirtybuffer1, 1, wc90b_tile_videoram_size );

	if ( ( tmpbitmap1 = osd_new_bitmap(4*Machine->drv->screen_width,2*Machine->drv->screen_height,Machine->scrbitmap->depth ) ) == 0 ){
		free( dirtybuffer1 );
		generic_vh_stop();
		return 1;
	}

	if ( ( dirtybuffer2 = malloc( wc90b_tile_videoram_size2 ) ) == 0 ) {
		osd_free_bitmap(tmpbitmap1);
		free(dirtybuffer1);
		generic_vh_stop();
		return 1;
	}

	memset( dirtybuffer2, 1, wc90b_tile_videoram_size2 );

	if ( ( tmpbitmap2 = osd_new_bitmap( 4*Machine->drv->screen_width,2*Machine->drv->screen_height,Machine->scrbitmap->depth ) ) == 0 ){
		osd_free_bitmap(tmpbitmap1);
		free(dirtybuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	// Free the generic bitmap and allocate one twice as wide
	free( tmpbitmap );

	if ( ( tmpbitmap = osd_new_bitmap( 2*Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth ) ) == 0 ){
		osd_free_bitmap(tmpbitmap1);
		osd_free_bitmap(tmpbitmap2);
		free(dirtybuffer);
		free(dirtybuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}

void wc90b_vh_stop ( void ) {
	free( dirtybuffer1 );
	free( dirtybuffer2 );
	osd_free_bitmap( tmpbitmap1 );
	osd_free_bitmap( tmpbitmap2 );
	generic_vh_stop();
}

int wc90b_tile_videoram_r ( int offset ) {
	return wc90b_tile_videoram[offset];
}

void wc90b_tile_videoram_w( int offset, int v ) {
	dirtybuffer1[offset] = 1;
	wc90b_tile_videoram[offset] = v;
}

int wc90b_tile_colorram_r ( int offset ) {
	return wc90b_tile_colorram[offset];
}

void wc90b_tile_colorram_w( int offset, int v ) {
	dirtybuffer1[offset] = 1;
	wc90b_tile_colorram[offset] = v;
}

int wc90b_tile_videoram2_r ( int offset ) {
	return wc90b_tile_videoram2[offset];
}

void wc90b_tile_videoram2_w( int offset, int v ) {
	dirtybuffer2[offset] = 1;
	wc90b_tile_videoram2[offset] = v;
}

int wc90b_tile_colorram2_r ( int offset ) {
	return wc90b_tile_colorram2[offset];
}

void wc90b_tile_colorram2_w( int offset, int v ) {
	dirtybuffer2[offset] = 1;
	wc90b_tile_colorram2[offset] = v;
}

int wc90b_shared_r ( int offset ) {
	return wc90b_shared[offset];
}

void wc90b_shared_w( int offset, int v ) {
	wc90b_shared[offset] = v;
}

int wc90b_palette_r ( int offset ) {
	return wc90b_palette[offset];
}

void wc90b_palette_w( int offset, int v ) {
	palettedirty = 1;
	wc90b_palette[offset] = v;
}

static void wc90b_draw_sprites( struct osd_bitmap *bitmap, int priority ){
  int offs;

  /* draw all visible sprites of specified priority */
	for ( offs = spriteram_size - 8;offs >= 0;offs -= 8 ){

		if ( ( ~( spriteram[offs+3] >> 6 ) & 3 ) == priority ) {

			if ( spriteram[offs+1] > 16 ) { /* visible */
				int code = ( spriteram[offs+3] & 0x3f ) << 4;
				int bank = spriteram[offs+0];
				int flags = spriteram[offs+4];

				code += ( bank & 0xf0 ) >> 4;
				code <<= 2;
				code += ( bank & 0x0f ) >> 2;

				drawgfx( bitmap,Machine->gfx[ 17 ], code,
						flags >> 4, /* color */
						bank&1, /* flipx */
						bank&2, /* flipy */
						spriteram[offs + 2], /* sx */
						240 - spriteram[offs + 1], /* sy */
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0 );
			}
		}
	}
}

void wc90b_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs, i;
	int scrollx, scrolly;

	if ( palettedirty ) {
		for (i = 0;i < 4*16*16;i++)
		{
			int blue = wc90b_palette[ 2*i ] & 0x0f;
			int green = (wc90b_palette[2*i+1] & 0xf0) >> 4;
			int red = wc90b_palette[2*i+1] & 0x0f;

			red = ( red << 4 ) + red;
			green = ( green << 4 ) + green;
			blue = ( blue << 4 ) + blue;

			if ( !red && !green && !blue && ( i %16 ) != 15 ) red = 0x20;

			setgfxcolorentry (Machine->gfx[17], i, red, green, blue);
		}
		palettedirty = 0;
	}

	wc90b_draw_sprites( bitmap, 3 );

	for ( offs = wc90b_tile_videoram_size2 - 1; offs >= 0; offs-- ) {
		int sx, sy, tile, gfx;

		if ( dirtybuffer2[offs] ) {

			dirtybuffer2[offs] = 0;

			sx = ( offs % 64 );
			sy = ( offs / 64 );

			tile = wc90b_tile_videoram2[offs];
			gfx = 9 + ( wc90b_tile_colorram2[offs] & 3 ) + ( ( wc90b_tile_colorram2[offs] >> 1 ) & 4 );

			drawgfx(tmpbitmap2,Machine->gfx[ gfx ],
					tile,
					wc90b_tile_colorram2[offs] >> 4,
					0,0,
					sx*16,sy*16,
					0,TRANSPARENCY_NONE,0);
		}
	}

	scrollx = -wc90b_scroll2xlo[0] - 256 * ( wc90b_scroll2xhi[0] & 3 );
	scrolly = -wc90b_scroll2ylo[0] - 256 * ( wc90b_scroll2yhi[0] & 1 );

	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	wc90b_draw_sprites( bitmap, 2 );

	for ( offs = wc90b_tile_videoram_size - 1; offs >= 0; offs-- ) {
		int sx, sy, tile, gfx;

		if ( dirtybuffer1[offs] ) {

			dirtybuffer1[offs] = 0;

			sx = ( offs % 64 );
			sy = ( offs / 64 );

			tile = wc90b_tile_videoram[offs];
			gfx = 1 + ( wc90b_tile_colorram[offs] & 3 ) + ( ( wc90b_tile_colorram[offs] >> 1 ) & 4 );

			drawgfx(tmpbitmap1,Machine->gfx[ gfx ],
					tile,
					wc90b_tile_colorram[offs] >> 4,
					0,0,
					sx*16,sy*16,
					0,TRANSPARENCY_NONE,0);
		}
	}

	scrollx = -wc90b_scroll1xlo[0] - 256 * ( wc90b_scroll1xhi[0] & 3 );
	scrolly = -wc90b_scroll1ylo[0] - 256 * ( wc90b_scroll1yhi[0] & 1 );

	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	wc90b_draw_sprites( bitmap, 1 );

	for ( offs = videoram_size - 1; offs >= 0; offs-- ) {
		if ( dirtybuffer[offs] ) {
			int sx, sy;

			dirtybuffer[offs] = 0;

			sx = (offs % 64);
			sy = (offs / 64);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ( ( colorram[offs] & 0x07 ) << 8 ),
					colorram[offs] >> 4,
					0,0,
					sx*8,sy*8,
					0,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap( bitmap, tmpbitmap, 0, 0, 0, 0,&Machine->drv->visible_area, TRANSPARENCY_COLOR, 0 );

	wc90b_draw_sprites( bitmap, 0 );
}
