/* vidhrdw/twin16.c */

/*
	Known Issues:
		sprite-tilemap priority isn't right (see Devil's World intro)
		some consolidation of the video rendering code should be possible
		cocktail support isn't implemented
		sprite rendering should be optimized
		some rogue sprites in Devil's World (probably CPU emulation related)
		the sprite processor (used for protection) handling is a bit clumsy, and
		partly tied to videorefresh
*/

#include "vidhrdw/generic.h"

enum {
	TWIN16_GFX_ROW_FLIP		= 1,
	TWIN16_GFX_CUSTOM		= 2,
	TWIN16_GFX_TILE_FLIP	= 4
};


static UINT16 video_register[8];
static int need_process_spriteram;
static UINT16 fround_gfx_bank;

extern int twin16_gfx_type;
extern UINT16 *twin16_gfx_rom;
extern unsigned char *twin16_fixram;
extern unsigned char *twin16_sprite_gfx_ram;
extern unsigned char *twin16_tile_gfx_ram;
extern void twin16_gfx_decode( void );
extern int twin16_spriteram_process_enable( void );

/******************************************************************************************/

void cuebrick_decode( void ){
	twin16_gfx_decode();
	twin16_gfx_type = 0;
}

void fround_decode( void ){
	twin16_gfx_decode();
	twin16_gfx_type = TWIN16_GFX_CUSTOM;
}

void gradius2_decode( void ){
	twin16_gfx_decode();
	twin16_gfx_type = TWIN16_GFX_ROW_FLIP;
}

void devilw_twin16_decode( void ){
	twin16_gfx_decode();
	twin16_gfx_type = TWIN16_GFX_ROW_FLIP|TWIN16_GFX_TILE_FLIP;
}

/******************************************************************************************/

void fround_gfx_bank_w( int offset, int data ){
	fround_gfx_bank = COMBINE_WORD(fround_gfx_bank,data);
}

void twin16_video_register_w( int offset, int data ){
	COMBINE_WORD_MEM( &video_register[offset/2], data );
}

/******************************************************************************************/

static void draw_layer(
	struct osd_bitmap *bitmap,
	const UINT16 *source,
	const UINT16 *gfx_base,
	int scrollx, int scrolly,
	const int *bank_table,
	int palette,
	int opaque
){
	int i;

	int y1, y2, dy;
	if( twin16_gfx_type&TWIN16_GFX_TILE_FLIP ){
		y1 = 7; y2 = -1; dy = -1;
	}
	else {
		y1 = 0; y2 = 8; dy = 1;
	}

	for( i=0; i<64*64; i++ ){
		int sx = (i%64)*8;
		int sy = (i/64)*8;
		int xpos,ypos;
		if( twin16_gfx_type&TWIN16_GFX_ROW_FLIP ) sy = 63*8 - sy;

		xpos = (sx-scrollx)&0x1ff;
		ypos = (sy-scrolly)&0x1ff;
		if( xpos>=320 ) xpos -= 512;
		if( ypos>=256 ) ypos -= 512;

		if( xpos>-8 && ypos>8 && xpos<320 && ypos<256-16 ){
			int code = source[i];
			/*
				xxx-------------	color
				---xx-----------	tile bank
				-----xxxxxxxxxxx	tile number
			*/
			const UINT16 *gfx_data = gfx_base + (code&0x7ff)*16 + bank_table[(code>>11)&0x3]*0x8000;
			int color = (code>>13);
			UINT16 *pal_data = Machine->pens + 16*(0x20+color+8*palette);

			{
				int y;
				UINT16 data;
				int pen;

				if( opaque )
				for( y=y1; y!=y2; y+=dy ){
					UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
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
				else
				for( y=y1; y!=y2; y+=dy ){
					UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
					data = *gfx_data++;
					if( data ){
						pen = (data>>4*3)&0xf; if( pen ) dest[0] = pal_data[pen];
						pen = (data>>4*2)&0xf; if( pen ) dest[1] = pal_data[pen];
						pen = (data>>4*1)&0xf; if( pen ) dest[2] = pal_data[pen];
						pen = (data>>4*0)&0xf; if( pen ) dest[3] = pal_data[pen];
					}
					data = *gfx_data++;
					if( data ){
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

static void draw_text( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const UINT16 *source = (UINT16 *)twin16_fixram;
	int i;


	for( i=0; i<64*64; i++ ){
		int code = source[i];

		int sx = (i%64)*8;
		int sy = (i/64)*8;

		if( twin16_gfx_type&TWIN16_GFX_ROW_FLIP ) sy = 31*8 - sy;

		drawgfx( bitmap, Machine->gfx[0],
			code&0x1ff,(code>>9)&0xf,
			0,0,
			sx,sy,
			clip,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************************/

static void draw_sprite( /* slow slow slow, but it's ok for now */
	struct osd_bitmap *bitmap,
	const UINT16 *pen_data,
	const UINT16 *pal_data,
	int xpos, int ypos,
	int width, int height,
	int flipx, int flipy ){

	int x,y;
	if( xpos>=320 ) xpos -= 512;
	if( ypos>=256 ) ypos -= 512;

	if( xpos+width>=0 && xpos<320 && ypos+height>16 && ypos<256-16 )

	for( y=0; y<height; y++ ){
		int sy = (flipy)?(ypos+height-1-y):(ypos+y);
		if( sy>=16 && sy<256-16 ){
			UINT16 *dest = (UINT16 *)bitmap->line[sy];
			for( x=0; x<width; x++ ){
				int sx = (flipx)?(xpos+width-1-x):(xpos+x);
				if( sx>=0 && sx<320 ){
					UINT16 pen = pen_data[x/4];
					switch( x%4 ){
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

void twin16_spriteram_process( void ){
	UINT16 scrollx = video_register[1];
	UINT16 scrolly = video_register[2];

	const UINT16 *source = (const UINT16 *)&spriteram[0x0000];
	const UINT16 *finish = (const UINT16 *)&spriteram[0x3000];

	memset( &spriteram[0x3000], 0, 0x800 );
	while( source<finish ){
		UINT16 priority = source[0];
		if( priority & 0x8000 ){
			UINT16 *dest = (UINT16 *)&spriteram[0x3000 + 8*(priority&0xff)];

			INT32 xpos = (0x10000*source[4])|source[5];
			INT32 ypos = (0x10000*source[6])|source[7];

			UINT16 attributes = source[2]&0x03ff; /* scale,size,color */
			if( priority & 0x0200 ) attributes |= 0x4000;
			attributes |= 0x8000;

			dest[0] = source[3]; /* gfx data */
			dest[1] = ((xpos>>8) - scrollx)&0xffff;
			dest[2] = ((ypos>>8) - scrolly)&0xffff;
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
	const UINT16 *source = (UINT16 *)(spriteram+0x3000);
	const UINT16 *finish = source + 0x800;
	pri = pri?0x4000:0x0000;

	while( source<finish ){
		UINT16 attributes = source[3];
		UINT16 code = source[0];

		if( code!=0xffff && (attributes&0x8000) && (attributes&0x4000)==pri ){
			int xpos = (INT16)source[1];
			int ypos = (INT16)source[2];

			const UINT16 *pal_data = Machine->pens+((attributes&0xf)+0x10)*16;
			int height	= 16<<((attributes>>6)&0x3);
			int width	= 16<<((attributes>>4)&0x3);
			const UINT16 *pen_data=0;
			int flipy = attributes&0x0200;
			int flipx = attributes&0x0100;

			if( twin16_gfx_type&TWIN16_GFX_CUSTOM ){
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
					pen_data = (UINT16 *)twin16_sprite_gfx_ram;
					break;
				}
				code &= 0xfff;
			}
			pen_data += code*0x40;

			if( twin16_gfx_type&TWIN16_GFX_ROW_FLIP ){
				ypos = 256-ypos-height;
				flipy = !flipy;
			}
			else {
				xpos = xpos&0x1ff;
				ypos = ypos&0x1ff;
			}

			draw_sprite( bitmap, pen_data, pal_data, xpos, ypos, width, height, flipx, flipy );
		}
		source += 4;
	}
}

/*						drawgfx( bitmap, Machine->uifont,
							"0123456789abcdef"[dat>>4],0,
							0,0,
							xpos,ypos,
							&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
						drawgfx( bitmap, Machine->uifont,
							"0123456789abcdef"[dat&0xf],0,
							0,0,
							xpos+6,ypos,
							&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
*/


static void show_vreg( struct osd_bitmap *bitmap ){
/*
	int i,n;

	for( i=0; i<8; i++ ){
		for( n=0; n<4; n++ ){
			drawgfx( bitmap, Machine->uifont,
				"0123456789abcdef"[(video_register[i]>>(12-4*n))&0xf],
				0,
				0,0,
				n*6+8,i*8+16,
				0,TRANSPARENCY_NONE,0);
		}
	}
	*/
	if( keyboard_pressed( KEYCODE_S ) ){
		FILE *f = fopen("spriteram","wb");
		fwrite( spriteram, 1, 0x4000, f );
		fclose(f);
		while( keyboard_pressed( KEYCODE_S ) ){}
	}
}

void twin16_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	const int bank_table[4] = { 0,1,2,3 };
	int scrollx, scrolly;
	int palette, offs;

	palette_recalc();

	if( twin16_spriteram_process_enable() && need_process_spriteram ) twin16_spriteram_process();

	{ /* background */
		if( twin16_gfx_type&TWIN16_GFX_ROW_FLIP ){
			scrollx = video_register[3];
			scrolly = 256-video_register[4];
			palette = 0;
			offs = 0x0000;
		}
		else {
			scrollx = video_register[5];
			scrolly = video_register[6];
			palette = 1;
			offs = 0x1000;
		}

		draw_layer(
			bitmap,
			offs + (UINT16 *)videoram,
			(UINT16 *)twin16_tile_gfx_ram,
			scrollx,scrolly,
			bank_table,
			palette,
			1
		);
	}

	draw_sprites( bitmap, 1 );

	if( !keyboard_pressed( KEYCODE_H ) ){ /* foreground */
		if( twin16_gfx_type&TWIN16_GFX_ROW_FLIP ){
			scrollx = video_register[5];
			scrolly = 256-video_register[6];
			palette = 1;
			offs = 0x1000;
		}
		else {
			scrollx = video_register[3];
			scrolly = video_register[4];
			palette = 0;
			offs = 0x0000;
		}

		draw_layer(
			bitmap,
			offs + (UINT16 *)videoram,
			(UINT16 *)twin16_tile_gfx_ram,
			scrollx,scrolly,
			bank_table,
			palette,
			0
		);
	}

	draw_sprites( bitmap, 0 );
	draw_text( bitmap );
	show_vreg( bitmap );

	need_process_spriteram = 1;
}

void fround_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	int bank_table[4];
	bank_table[0] = (fround_gfx_bank>>(4*0))&0xf;
	bank_table[1] = (fround_gfx_bank>>(4*1))&0xf;
	bank_table[2] = (fround_gfx_bank>>(4*2))&0xf;
	bank_table[3] = (fround_gfx_bank>>(4*3))&0xf;

	palette_recalc();

	if( twin16_spriteram_process_enable() && need_process_spriteram ) twin16_spriteram_process();

	draw_layer(
		bitmap,
		0x1000 + (UINT16 *)videoram,
		twin16_gfx_rom,
		video_register[5],video_register[6],
		bank_table,
		1, /* palette select */
		1 /* opaque */
	);

	draw_sprites( bitmap, 1 );

	draw_layer(
		bitmap,
		0x0000 + (UINT16 *)videoram,
		twin16_gfx_rom,
		video_register[3],video_register[4],
		bank_table,
		0, /* palette select */
		0 /* opaque */
	);

	draw_sprites( bitmap, 0 );

	draw_text( bitmap );
	show_vreg( bitmap );

	need_process_spriteram = 1;
}
