/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/system16.h"
#include "vidhrdw/s16sprit.c"

//#include "vgeneric.h"
//#include "msystem16.h"
//#include "vs16sprit.c"

/***************************************************************************

  system16 games don't have a color PROM
  Instead, 4096 bytes of RAM are mapped to specify colors.
  2 bytes are used for each color, yielding 2048 color entries.
  1024 of these are used for sprites, and 1024 are used for the backgrounds.

  Graphics use 4 bitplanes.

  Since MAME doesn't yet support 16-bit color,
  we define a reduced 3x3x2 color space.

***************************************************************************/

void system16_paletterefresh(void)
{
	unsigned char *dirty = system16_colordirty;
	int index;

	for( index=0; index<s16_paletteram_size/2; index++ ){
		if( dirty[index] ){
			int offset = index*2;
			int palette = READ_WORD (&system16_paletteram[offset]);

			/*
				byte0		byte1
				GBGR BBBB	GGGG RRRR
				5444 3210	3210 3210
			*/
#if 0
			/* extract color components and scale to 0..255 */
			int red		= ( byte1 & 0xf )<<4;
			int green	= ( byte1 & 0xf0 );
			int blue	= ( byte0 & 0xf )<<4;

			/* map to our reduced palette: BBGGGRRR */
			int color = ((red >> 5) & 0x07) + ((green >> 2) & 0x38) + (blue & 0xc0);

			Machine->gfx[0]->colortable[index] = Machine->pens[color];
#endif
			int red		= ( palette & 0xf );
			int green	= ( palette >> 4 ) & 0x0f;
			int blue	= ( palette >> 8 ) & 0x0f;

			red = (red << 4) + red;
			green = (green << 4) + green;
			blue = (blue << 4) + blue;

			setgfxcolorentry (Machine->gfx[3], index, red, green, blue);
			dirty[index] = 0;
		}
	}
}

void system16_backgroundrefresh(struct osd_bitmap *bitmap)
{
	const struct GfxElement *gfx = Machine->gfx[2];

	int page;

	int scrollx = 320+scrollX[BG_OFFS];
	int scrolly = 256-scrollY[BG_OFFS];

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for (page=0; page < 4; page++){
		const unsigned char *source = system16_backgroundram + bg_pages[page]*0x1000;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				int x = startx+col;
				int y = starty+row;
				if( x>320 ) x-=1024;
				if( y>224 ) y-=512;
				if( x>-8 && x<320 && y>-8 && y<224 ){
					int tile = READ_WORD( &source[0] );
					drawgfx( bitmap, gfx,
						tile + system16_background_bank*0x1000,
						(tile >> 6),
						0, 0, x, y,
						&Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
				}
				source+=2;
			}
		}
	} /* next page */
}

void system16_foregroundrefresh(struct osd_bitmap *bitmap, int priority )
{
	const struct GfxElement *gfx = Machine->gfx[2];

	int page;
	int scrollx = 320+scrollX[FG_OFFS];
	int scrolly = 256-scrollY[FG_OFFS];

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for (page=0; page < 4; page++){
		const unsigned char *source = system16_backgroundram + fg_pages[page]*0x1000;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				int tile = READ_WORD ( &source[0] );
				if( tile>>15 == priority ){
					int x = startx+col;
					int y = starty+row;
					if( x>320 ) x-=1024;
					if( y>224 ) y-=512;
					if( x>-8 && x<320 && y>-8 && y<224 ){
						if( tile )
						 drawgfx( bitmap, gfx,
							tile+(priority?8192:0),/*hack for altered beast */
							(tile >> 6),
							0, 0, x, y,
							&Machine->drv->visible_area, TRANSPARENCY_PEN, 0);
					}
				}
				source+=2;
			}
		}
	} /* next page */
}

void system16_textrefresh(struct osd_bitmap *bitmap)
{
	int sx,sy;
	for (sx = 0; sx < 40; sx++) for (sy = 0; sy < 28; sy++){
			int offs = (sy * 64 + sx + (64-40)) << 1;
			int tile = READ_WORD(&system16_videoram[offs]);
			if( tile ){
				drawgfx(bitmap,Machine->gfx[0],
							tile & 0x1ff,
							tile >> 9,
							0,0, // xflip, yflip
							8*sx,8*sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
	}
}

void system16_spriterefresh(struct osd_bitmap *bitmap, int priority)
{
	struct sys16_sprite_info *sprite = sys16_sprite;
	struct sys16_sprite_info *finish = sprite+64;

	do{
		if( sprite->end_line == 0xff ) break;
		if( sprite->priority == priority ) DrawSprite( bitmap, sprite );
		sprite++;
	}while( sprite<finish );
}


void system16_vh_screenrefresh(struct osd_bitmap *bitmap)
{
		system16_paletterefresh();

	if (system16_refreshenable_r(0))
	{
		system16_backgroundrefresh(bitmap);

		system16_spriterefresh(bitmap,1);

		system16_foregroundrefresh(bitmap,0);

		system16_spriterefresh(bitmap,2);

		system16_foregroundrefresh(bitmap,1);

		system16_textrefresh(bitmap);

		system16_spriterefresh(bitmap,3);
	}
}



