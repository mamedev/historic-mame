#include "driver.h"
#include "vidhrdw/generic.h"

static int bg_tilemap_baseaddr;
#define bg_tilemap_ram_size (32*32*2)
static unsigned char *bg_tilemap_ram_buffer;

static int shadows_visible = 0; /* toggles rapidly to fake translucency in ikari warriors */

#define GFX_CHARS			0
#define GFX_TILES			1
#define GFX_SPRITES			2
#define GFX_BIGSPRITES		3

void snk_adpcm_play( int which );

static void adpcm_test( void ){
	if( osd_key_pressed( OSD_KEY_Q ) ){
		while( osd_key_pressed( OSD_KEY_Q ) );
		snk_adpcm_play(0);
	}
	if( osd_key_pressed( OSD_KEY_W ) ){
		while( osd_key_pressed( OSD_KEY_W ) );
		snk_adpcm_play(1);
	}
	if( osd_key_pressed( OSD_KEY_E ) ){
		while( osd_key_pressed( OSD_KEY_E ) );
		snk_adpcm_play(2);
	}
	if( osd_key_pressed( OSD_KEY_R) ){
		while( osd_key_pressed( OSD_KEY_R ) );
		snk_adpcm_play(3);
	}
}

void snk_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom){
	int i;
	for( i=0; i<1024; i++ ){
		int bit0,bit1,bit2,bit3;

		colortable[i] = i;

		bit0 = (color_prom[0*1024] >> 0) & 0x01;
		bit1 = (color_prom[0*1024] >> 1) & 0x01;
		bit2 = (color_prom[0*1024] >> 2) & 0x01;
		bit3 = (color_prom[0*1024] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[1*1024] >> 0) & 0x01;
		bit1 = (color_prom[1*1024] >> 1) & 0x01;
		bit2 = (color_prom[1*1024] >> 2) & 0x01;
		bit3 = (color_prom[1*1024] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[2*1024] >> 0) & 0x01;
		bit1 = (color_prom[2*1024] >> 1) & 0x01;
		bit2 = (color_prom[2*1024] >> 2) & 0x01;
		bit3 = (color_prom[2*1024] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}

void ikari_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom){
	int i;
	snk_vh_convert_color_prom( palette, colortable, color_prom);

	palette += 6*3;
	/*
		pen#6 is used for translucent shadows;
		we'll just make it dark grey for now
	*/
	for( i=0; i<256; i+=8 ){
		palette[i*3+0] = 14;
		palette[i*3+1] = 14;
		palette[i*3+2] = 14;
	}
}

int snk_vh_start( void ){
	bg_tilemap_ram_buffer = malloc( bg_tilemap_ram_size );
	if( bg_tilemap_ram_buffer ){
		tmpbitmap = osd_create_bitmap( 512, 512 );
		if( tmpbitmap ){
			memset( bg_tilemap_ram_buffer, 1, bg_tilemap_ram_size  );

			if( //strcmp(Machine->gamedrv->name, "psychos")==0 ||
				strcmp(Machine->gamedrv->name, "ikarijp")==0 ||
				strcmp(Machine->gamedrv->name, "ikarijpb")==0 ){
				bg_tilemap_baseaddr = 0xd000; /* special case */
			}
			else {
				bg_tilemap_baseaddr = 0xd800;
			}
			shadows_visible = 1;
			return 0;
		}
		free( bg_tilemap_ram_buffer );
	}
	return 1;
}

void snk_vh_stop( void ){
	osd_free_bitmap( tmpbitmap );
	free( bg_tilemap_ram_buffer );
}

static void ikari_draw_background( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	const struct GfxElement *gfx = Machine->gfx[GFX_TILES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][bg_tilemap_baseaddr];

	int offs;
	for( offs=0; offs<bg_tilemap_ram_size; offs+=2 ){
		int tile_number = source[offs];
		unsigned char attributes = source[offs+1];

		if( tile_number!=bg_tilemap_ram_buffer[offs] ||
			attributes != bg_tilemap_ram_buffer[offs+1] ){

			int sx = ((offs/2)%32)*16;
			int sy = ((offs/2)/32)*16;

			bg_tilemap_ram_buffer[offs] = tile_number;
			bg_tilemap_ram_buffer[offs+1] = attributes;

			tile_number+=256*(attributes&0x3);

			drawgfx(tmpbitmap,gfx,
				tile_number,
				(attributes>>4), /* color */
				0,0, /* no flip */
				sx,sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	{
		struct rectangle clip = Machine->drv->visible_area;
		clip.min_y += 16;
		clip.max_y -= 16;
		copyscrollbitmap(bitmap,tmpbitmap,
			1,&xscroll,1,&yscroll,
			&clip,
			TRANSPARENCY_NONE,0);
	}
}

static void ikari_draw_text( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[GFX_CHARS];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xf800];

	int offs;
	for( offs = 0;offs <0x400; offs++ ){
		int tile_number = source[offs];
		if( source[offs] != 0x20 ){ /* skip spaces */
			int sx = 20+(offs % 32)*8;
			int sy = (offs / 32)*8+16;

			drawgfx(bitmap,gfx,
				tile_number+256,
				8, /* color */
				0,0, /* no flip */
				sx,sy,
				clip,TRANSPARENCY_PEN,15);
		}
	}
}

static void ikari_draw_status( struct osd_bitmap *bitmap ){
	/*	this is drawn above and below the main display */

	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[GFX_CHARS];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xfc00];

	int offs;
	for( offs = 0; offs<64; offs++ ){
		int tile_number = source[offs+30*32]+256;
		int sx = 20+(offs % 32)*8;
		int sy = (offs / 32)*8;

		drawgfx(bitmap,gfx,
			tile_number,
			8, /* color */
			0,0, /* no flip */
			sx,sy,
			clip,TRANSPARENCY_NONE,0);

		tile_number = source[offs]+256;
		sy += 34*8;

		drawgfx(bitmap,gfx,
			tile_number,
			8, /* color */
			0,0, /* no flip */
			sx,sy,
			clip,TRANSPARENCY_NONE,0);
	}
}

static void ikari_draw_sprites_16x16( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	int transp_mode = shadows_visible?TRANSPARENCY_PEN:TRANSPARENCY_PENS;
	int transp_param = shadows_visible?7:(1<<7)|(1<<6);

	int which;
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe800];

	struct rectangle clip = Machine->drv->visible_area;
	clip.min_y += 16;
	clip.max_y -= 16;

	for( which = 0; which < 50*4; which+=4 ){
		int attributes = source[which+3]; /* YBBX.CCCC */
		int tile_number = source[which+1] + ((attributes&0x60)<<3);
		int sx =   8 - xscroll + source[which];
		int sy =  16 + yscroll - source[which+2]+256;
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		drawgfx(bitmap,Machine->gfx[GFX_SPRITES],
			tile_number,
			attributes&0x0f, /* color */
			0,0, /* flip */
			(sx&0x1ff),(sy&0x1ff),
			&clip,transp_mode,transp_param);
	}
}

static void ikari_draw_sprites_32x32( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	int transp_mode = shadows_visible?TRANSPARENCY_PEN:TRANSPARENCY_PENS;
	int transp_param = shadows_visible?7:(1<<7)|(1<<6);

	int which;
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe000];

	struct rectangle clip = Machine->drv->visible_area;
	clip.min_y += 16;
	clip.max_y -= 16;

	for( which = 0; which < 25*4; which+=4 ){
		int attributes = source[which+3];
		int tile_number = source[which+1];
		int sx = -8 - xscroll + source[which];
		int sy = 0 + yscroll - source[which+2] + 256;
		if( attributes&0x40 ) tile_number += 256;
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		drawgfx( bitmap,Machine->gfx[GFX_BIGSPRITES],
			tile_number,
			attributes&0xf, /* color */
			0,0, /* flip */
			(sx&0x1ff),(sy&0x1ff),
			&clip,transp_mode,transp_param );
	}
}

void ikari_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh){
	const unsigned char *ram = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	shadows_visible = !shadows_visible;

	adpcm_test();

	{
		int bg_attributes = ram[0xc900];
		int bg_scroll_x = 24-ram[0xc800] - ((bg_attributes&0x01)?256:0);
		int bg_scroll_y = 16-ram[0xc880] - ((bg_attributes&0x02)?256:0);
		ikari_draw_background( bitmap, bg_scroll_x, bg_scroll_y );
	}

	{
		int sprite_attributes = ram[0xcd00];

		int spr16_scroll_x =    ram[0xca00] + ((sprite_attributes&0x04)?256:0);
		int spr16_scroll_y = 16+ram[0xca80] + ((sprite_attributes&0x10)?256:0);

		int spr32_scroll_x =    ram[0xcb00] + ((sprite_attributes&0x08)?256:0);
		int spr32_scroll_y = 16+ram[0xcb80] + ((sprite_attributes&0x20)?256:0);

		ikari_draw_sprites_16x16( bitmap, spr16_scroll_x, spr16_scroll_y );
		ikari_draw_sprites_32x32( bitmap, spr32_scroll_x, spr32_scroll_y );
	}

	ikari_draw_text( bitmap );
	ikari_draw_status( bitmap );
}

static void tnk3_draw_background( struct osd_bitmap *bitmap, int scrollx, int scrolly ){
	const struct GfxElement *gfx = Machine->gfx[GFX_TILES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xd800];

	int sx, sy;
	for( sy=0; sy<512; sy+=8 ){
		for( sx=0; sx<512; sx+=8 ){
		/* for now, just redraw everything every time */
			unsigned char attributes = source[1];
			int tile_number = source[0] + 256*((attributes>>4)&0x3);

			drawgfx( tmpbitmap,gfx,
				tile_number,
				(attributes&0xf),
				0,0,
				sx,sy,
				0,TRANSPARENCY_NONE,0);

			source+=2;
		}
	}

	{
		struct rectangle clip = Machine->drv->visible_area;
		clip.min_y += 16;
		clip.max_y -= 16;
		copyscrollbitmap(bitmap,tmpbitmap,
			1,&scrollx,1,&scrolly,
			&clip,
			TRANSPARENCY_NONE,0);
	}
}

static void tnk3_draw_text( struct osd_bitmap *bitmap ){	const struct GfxElement *gfx = Machine->gfx[GFX_CHARS];
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xf800];

	int offs;
	for( offs = 0;offs <0x400; offs++ ){
		int tile_number = source[offs];

		int sx = 20+(offs % 32)*8;
		int sy = (offs / 32)*8+16;

		drawgfx(bitmap,gfx,
			tile_number+0x100,
			8, /* color */
			0,0, /* no flip */
			sx,sy,
			clip,
			TRANSPARENCY_PEN,15 );
	}
}

static void tnk3_draw_status( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xfc00];

	int offs;
	for( offs = 0; offs<64; offs++ ){
		int tile_number = source[offs+30*32]+256;
		int sx = 20+(offs % 32)*8;
		int sy = (offs / 32)*8;

		drawgfx(bitmap,Machine->gfx[GFX_CHARS],
			tile_number,
			8, /* color */
			0,0, /* no flip */
			sx,sy,
			clip,TRANSPARENCY_NONE,0);

		tile_number = source[offs]+256;
		sy += 34*8;

		drawgfx(bitmap,Machine->gfx[GFX_CHARS],
			tile_number,
			8, /* color */
			0,0, /* no flip */
			sx,sy,
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

void tnk3_draw_sprites( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xd000];

	int which;

	struct rectangle clip = Machine->drv->visible_area;
	clip.min_y += 16;
	clip.max_y -= 16;

	for( which = 0; which < 48*4; which+=4 ){
		int attributes = source[which+3]; /* YB?X.CCCC */
		int tile_number = source[which+1] + ((attributes&0x40)?256:0);
		int sx = source[which] +   ((attributes&0x10)?256:0) - xscroll;
		int sy = source[which+2] + ((attributes&0x80)?256:0) - yscroll;
		int color = attributes&0x0f;
		int xflip = 0;
		int yflip = 0;

		drawgfx(bitmap,Machine->gfx[GFX_SPRITES],
			tile_number,
			color,
			0,yflip,
			sx,(256-sy)&0x1ff,
			&clip,TRANSPARENCY_PEN,7);
	}
}

void tnk3_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	const unsigned char *ram = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	int attrs = ram[0xc800];
	int x1 = ram[0xc900] + ((attrs&0x08)?256:0);
	int y1 = ram[0xca00] + ((attrs&0x01)?256:0);
	int x2 = ram[0xcb00] + ((attrs&0x10)?256:0);
	int y2 = ram[0xcc00] + ((attrs&0x02)?256:0);

	adpcm_test();

	if( osd_key_pressed( OSD_KEY_Q ) ){ /* debug code: show palette */
		int i,x,y;
		for( i=0; i<1024; i++ ){
			int x0 = (i%32)*7;
			int y0 = (i/32)*7+8;
			int c = Machine->pens[i];
			for( x=0; x<8; x++ ) for( y=0; y<8; y++ ){
				bitmap->line[y+y0][x+x0] = c;
			}
		}
		return;
	}

	tnk3_draw_background( bitmap, -x1, y1+32 );
	tnk3_draw_sprites( bitmap, x2, -y2 );
	tnk3_draw_text( bitmap );
	tnk3_draw_status( bitmap );
}

static void tdfever_draw_background( struct osd_bitmap *bitmap,
		int xscroll, int yscroll )
{
	const struct GfxElement *gfx = Machine->gfx[GFX_TILES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xd000];

	int offs;
	for( offs=0; offs<bg_tilemap_ram_size; offs+=2 ){
		int tile_number = source[offs];
		unsigned char attributes = source[offs+1];

		if( tile_number!=bg_tilemap_ram_buffer[offs] ||
			attributes != bg_tilemap_ram_buffer[offs+1] ){

			int sx = ((offs/2)%32)*16;
			int sy = ((offs/2)/32)*16;

			int color = (attributes>>4); /* color */

			bg_tilemap_ram_buffer[offs] = tile_number;
			bg_tilemap_ram_buffer[offs+1] = attributes;

			tile_number+=256*(attributes&0xf);

			drawgfx(tmpbitmap,gfx,
				tile_number,
				color,
				0,0, /* no flip */
				sx,sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	{
		struct rectangle clip = Machine->drv->visible_area;
		copyscrollbitmap(bitmap,tmpbitmap,
			1,&xscroll,1,&yscroll,
			&clip,
			TRANSPARENCY_NONE,0);
	}
}

static void tdfever_draw_sprites( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	const struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe000];

	int which;

	struct rectangle clip = Machine->drv->visible_area;

	for( which = 0; which < 32*4; which+=4 ){
		int attributes = source[which+3];
		int tile_number = source[which+1] + 8*(attributes&0x60);

		int sx = - xscroll + source[which];
		int sy = yscroll - source[which+2];
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		sx &= 0x1ff; if( sx>512-32 ) sx-=512;
		sy &= 0x1ff; if( sy>512-32 ) sy-=512;

		drawgfx(bitmap,gfx,
			tile_number,
			(attributes&0xf), /* color */
			0,0, /* no flip */
			sx,sy,
			&clip,TRANSPARENCY_PEN,15);
	}
}

static void tdfever_draw_text( struct osd_bitmap *bitmap, int attributes ){
	int bank = attributes>>4;
	int color = attributes&0xf;

	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[GFX_CHARS];

	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xf800];

	int offs;

	int bank_offset = bank*256;

	for( offs = 0;offs <0x800; offs++ ){
		int tile_number = source[offs];
		int sx = (offs % 32)*8;
		int sy = (offs / 32)*8;

		if( source[offs] != 0x20 ){
			drawgfx(bitmap,gfx,
				tile_number + bank_offset,
				color,
				0,0, /* no flip */
				sx,sy,
				clip,TRANSPARENCY_PEN,15);
		}
	}
}

void tdfever_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	const unsigned char *ram = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	adpcm_test();

	{
		unsigned char bg_attributes = ram[0xc880];
		int bg_scroll_x = -30 - ram[0xc800] - ((bg_attributes&0x01)?256:0);
		int bg_scroll_y = 141 - ram[0xc840] - ((bg_attributes&0x02)?256:0);
		tdfever_draw_background( bitmap, bg_scroll_x, bg_scroll_y );
	}

	{
		unsigned char sprite_attributes = ram[0xc900];
		int sprite_scroll_x =   65 + ram[0xc980] + ((sprite_attributes&0x80)?256:0);
		int sprite_scroll_y = -135 + ram[0xc9c0] + ((sprite_attributes&0x40)?256:0);
		tdfever_draw_sprites( bitmap, sprite_scroll_x, sprite_scroll_y );
	}

	{
		unsigned char text_attributes = ram[0xc8c0];
		tdfever_draw_text( bitmap, text_attributes );
	}
}

static void gwar_draw_sprites_16x16( struct osd_bitmap *bitmap, int xscroll, int yscroll, int priority ){
	const struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe800] + 32*4*priority;
	int which;

	struct rectangle clip = Machine->drv->visible_area;
	clip.min_y += 16;
	clip.max_y -= 16;

	for( which=0; which<32*4; which+=4 ){
		int attributes = source[which+3]; /* YBBX.BCCC */
		int tile_number = source[which+1];
		int sx = -xscroll + source[which];
		int sy =  yscroll - source[which+2];
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		if( attributes&0x08 ) tile_number += 256;
		if( attributes&0x20 ) tile_number += 512;
		if( attributes&0x40 ) tile_number += 1024;

		sx &= 0x1ff; if( sx>512-16 ) sx-=512;
		sy = (-sy)&0x1ff; if( sy>512-16 ) sy-=512;

		drawgfx(bitmap,gfx,
			tile_number,
			(attributes&7), /* color */
			0,1, /* flip */
			sx,sy,
			&clip,TRANSPARENCY_PEN,15 );
	}
}

void gwar_draw_sprites_32x32( struct osd_bitmap *bitmap, int xscroll, int yscroll ){
	const struct GfxElement *gfx = Machine->gfx[GFX_BIGSPRITES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe000];

	struct rectangle clip = Machine->drv->visible_area;
	int which;
	for( which=0; which<32*4; which+=4 ){
		int attributes = source[which+3];
		int tile_number = source[which+1] + 8*(attributes&0x60);

		int sx = - xscroll + source[which];
		int sy = yscroll - source[which+2];
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		sx = (sx&0x1ff);
		sy = ((-sy)&0x1ff);
		if( sx>512-32 ) sx-=512;
		if( sy>512-32 ) sy-=512;

		drawgfx(bitmap,gfx,
			tile_number,
			(attributes&0xf), /* color */
			0,1, /* no flip */
			sx,sy,
			&clip,TRANSPARENCY_PEN,15);
	}
}

void gwar_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	const unsigned char *ram = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	unsigned char bg_attributes = ram[0xc880];
	adpcm_test();

	{
		int bg_scroll_x = - ram[0xc800] - ((bg_attributes&0x01)?256:0);
		int bg_scroll_y = 16 - ram[0xc840] - ((bg_attributes&0x02)?256:0);
		tdfever_draw_background( bitmap, bg_scroll_x, bg_scroll_y );
	}

	{
		int sp16_x = 15 + ram[0xc900]+((bg_attributes&0x10)?256:0);
		int sp16_y = 9 + ram[0xc940]+((bg_attributes&0x40)?256:0);

		int sp32_x = 31 + ram[0xc980]+((bg_attributes&0x20)?256:0);
		int sp32_y = 16 + ram[0xc9c0]+((bg_attributes&0x80)?256:0);

		/* this isn't quite right */
		gwar_draw_sprites_16x16( bitmap, sp16_x, sp16_y, 1 );
		gwar_draw_sprites_32x32( bitmap, sp32_x, sp32_y );
		gwar_draw_sprites_16x16( bitmap, sp16_x, sp16_y, 0 );
	}

	{
		unsigned char text_attributes = ram[0xc8c0];
		tdfever_draw_text( bitmap, text_attributes );
	}
}

static void psychos_draw_sprites_16x16( struct osd_bitmap *bitmap, int xscroll, int yscroll, int priority ){
	const struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const unsigned char *source = &Machine->memory_region[Machine->drv->cpu[0].memory_region][0xe800] + 32*4*priority;
	int which;

	struct rectangle clip = Machine->drv->visible_area;
	clip.min_y += 16;
	clip.max_y -= 16;

	for( which=0; which<32*4; which+=4 ){
		int attributes = source[which+3]; /* YBBX.BCCC */
		int tile_number = source[which+1];
		int sx = -xscroll + source[which];
		int sy =  yscroll - source[which+2];
		if( attributes&0x10 ) sx += 256;
		if( attributes&0x80 ) sy -= 256;

		if( attributes&0x08 ) tile_number += 256;
		if( attributes&0x20 ) tile_number += 512;
		//if( attributes&0x40 ) tile_number += 1024;

		sx &= 0x1ff; if( sx>512-16 ) sx-=512;
		sy = (-sy)&0x1ff; if( sy>512-16 ) sy-=512;

		drawgfx(bitmap,gfx,
			tile_number,
			(attributes&7), /* color */
			0,1, /* flip */
			sx,sy,
			&clip,TRANSPARENCY_PEN,15 );
	}
}

static void print( struct osd_bitmap *bitmap, int num, int row ){
	char *digit = "0123456789abcdef";
	drawgfx( bitmap,Machine->uifont,
		digit[num>>4],
		0,
		0,0, /* no flip */
		8+row*8,8,
		0,TRANSPARENCY_NONE,0);
	drawgfx( bitmap,Machine->uifont,
		digit[num%4],
		0,
		0,0, /* no flip */
		8+row*8,8+8,
		0,TRANSPARENCY_NONE,0);
}

/*
	if( osd_key_pressed( OSD_KEY_M ) ){
		FILE *f = fopen("ram1","wb");
		if( f ){
			fwrite( Machine->memory_region[0], 0x10000, 1, f );
			fclose( f );
		}
		while( osd_key_pressed( OSD_KEY_M ) );
	}
*/

void psychos_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	const unsigned char *ram = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char bg_attributes = ram[0xc880];

	adpcm_test();

	{
		int bg_scroll_x = - ram[0xc800] - ((bg_attributes&0x01)?256:0);
		int bg_scroll_y = 16 - ram[0xc840] - ((bg_attributes&0x02)?256:0);
		tdfever_draw_background( bitmap, bg_scroll_x, bg_scroll_y );
	}

	{
		unsigned char sprite_attributes = ram[0xca80];

		int sp16_x = 15+ ram[0xc900] + ((bg_attributes&0x01)?256:0);//((sprite_attributes&0x40)?256:0);
		int sp16_y = 16+/*9 +*/ ram[0xc940] + ((sprite_attributes&0x10)?256:0);
		int sp32_x = 31 + ram[0xc980] + ((bg_attributes&0x01)?256:0);//((sprite_attributes&0x80)?256:0);
		int sp32_y = 16 + ram[0xc9c0] + ((sprite_attributes&0x20)?256:0);

		psychos_draw_sprites_16x16( bitmap, sp16_x, sp16_y, 1 );
		gwar_draw_sprites_32x32( bitmap, sp32_x, sp32_y );
		psychos_draw_sprites_16x16( bitmap, sp16_x, sp16_y, 0 );
	}

	{
		unsigned char text_attributes = ram[0xc8c0];
		tdfever_draw_text( bitmap, text_attributes );
	}

return;
	print( bitmap, ram[0xc880]&(~3), 2 ); // bg attrs
	print( bitmap, ram[0xc8c0], 3 ); // text attrs!

	print( bitmap, ram[0xca00], 0+10 );
	print( bitmap, ram[0xca40], 1+10 );
	print( bitmap, ram[0xca80], 2+10 ); // sprite attrs
	print( bitmap, ram[0xcac0], 3+10 ); // 02

	print( bitmap, ram[0xcd00], 0+15 );
	print( bitmap, ram[0xcd40], 1+15 );
	print( bitmap, ram[0xcd80], 2+15 ); // attrs
	print( bitmap, ram[0xcdc0], 3+15 );

	print( bitmap, ram[0xcf00], 0+20 );
	print( bitmap, ram[0xcf40], 1+20 );
	print( bitmap, ram[0xcf80], 2+20 );
	print( bitmap, ram[0xcfc0], 3+20 );
}
