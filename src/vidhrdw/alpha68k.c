/***************************************************************************

   Alpha 68k video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int bank_base,flipscreen;
static struct tilemap *fix_tilemap;

/******************************************************************************/

void alpha68k_flipscreen_w(int flip)
{
	flipscreen = flip;
}

void alpha68k_V_video_bank_w(int bank)
{
	bank_base = bank&0xf;
}

WRITE16_HANDLER( alpha68k_paletteram_w )
{
	int newword;
	int r,g,b;

	COMBINE_DATA(paletteram16 + offset);
	newword = paletteram16[offset];

	r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
	g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01);
	b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}

/******************************************************************************/

static void get_tile_info(int tile_index)
{
	int tile  = videoram16[2*tile_index]   &0xff;
	int color = videoram16[2*tile_index+1] &0x0f;

	tile=tile | (bank_base<<8);

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

WRITE16_HANDLER( alpha68k_videoram_w )
{
	/* Doh. */
	if(ACCESSING_LSB)
		if(ACCESSING_MSB)
			videoram16[offset] = data;
		else
			videoram16[offset] = data & 0xff;
	else
		videoram16[offset] = (data >> 8) & 0xff;

	tilemap_mark_tile_dirty(fix_tilemap,offset/2);
}

int alpha68k_vh_start(void)
{
	fix_tilemap = tilemap_create(get_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

/******************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, int j, int pos)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = pos; offs < pos+0x400; offs += 0x40 )
	{
		mx = spriteram16[offs+2+(2*j)]<<1;
		my = spriteram16[offs+3+(2*j)];
		if (my&0x8000) mx++;

		mx=(mx+0x100)&0x1ff;
		my=(my+0x100)&0x1ff;
		mx-=0x100;
		my-=0x100;
		my=0x200 - my;
		my-=0x200;

		if (flipscreen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x40; i+=2) {
			tile  = spriteram16[offs+1+i+(0x800*j)+0x800];
			color = spriteram16[offs  +i+(0x800*j)+0x800]&0x7f;

			fy=tile&0x8000;
			fx=tile&0x4000;
			tile&=0x3fff;

			if (flipscreen) {
				if (fx) fx=0; else fx=1;
				if (fy) fy=0; else fy=1;
			}

			if (color)
				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					0,TRANSPARENCY_PEN,0);

			if (flipscreen)
				my=(my-16)&0x1ff;
			else
				my=(my+16)&0x1ff;
		}
	}
}

/******************************************************************************/

void alpha68k_II_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	static int last_bank=0;

	if (last_bank!=bank_base)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,Machine->pens[2047],&Machine->visible_area);

	draw_sprites(bitmap,1,0x000);
	draw_sprites(bitmap,1,0x400);
	draw_sprites(bitmap,0,0x000);
	draw_sprites(bitmap,0,0x400);
	draw_sprites(bitmap,2,0x000);
	draw_sprites(bitmap,2,0x400);
	tilemap_draw(bitmap,fix_tilemap,0,0);
}

/******************************************************************************/

/*
	Video banking:

	Write to these locations in this order for correct bank:

	20 28 30 for Bank 0
	60 28 30 for Bank 1
	20 68 30 etc
	60 68 30
	20 28 70
	60 28 70
	20 68 70
	60 68 70 for Bank 7

	Actual data values written don't matter!

*/

WRITE16_HANDLER( alpha68k_II_video_bank_w )
{
	static int buffer_28,buffer_60,buffer_68;

	switch (offset) {
		case 0x10: /* Reset */
			bank_base=buffer_28=buffer_60=buffer_68=0;
			return;
		case 0x14:
			buffer_28=1;
			return;
		case 0x18:
			if (buffer_68) {if (buffer_60) bank_base=3; else bank_base=2; }
			if (buffer_28) {if (buffer_60) bank_base=1; else bank_base=0; }
			return;
		case 0x30:
			bank_base=buffer_28=buffer_68=0;
			buffer_60=1;
			return;
		case 0x34:
			buffer_68=1;
			return;
		case 0x38:
			if (buffer_68) {if (buffer_60) bank_base=7; else bank_base=6; }
			if (buffer_28) {if (buffer_60) bank_base=5; else bank_base=4; }
			return;
		case 0x08: /* Graphics flags?  Not related to fix chars anyway */
		case 0x0c:
		case 0x28:
		case 0x2c:
			return;
	}

	logerror("%04x \n",offset);
}

/******************************************************************************/

WRITE16_HANDLER( alpha68k_V_video_control_w )
{
	switch (offset) {
		case 0x08: /* Graphics flags?  Not related to fix chars anyway */
		case 0x0c:
		case 0x28:
		case 0x2c:
			return;
	}
}

static void draw_sprites_V(struct mame_bitmap *bitmap, int j, int s, int e, int fx_mask, int fy_mask, int sprite_mask)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = s; offs < e; offs += 0x40 )
	{
		mx = spriteram16[offs+2+(2*j)]<<1;
		my = spriteram16[offs+3+(2*j)];
		if (my&0x8000) mx++;

		mx=(mx+0x100)&0x1ff;
		my=(my+0x100)&0x1ff;
		mx-=0x100;
		my-=0x100;
		my=0x200 - my;
		my-=0x200;

		if (flipscreen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x40; i+=2) {
			tile  = spriteram16[offs+1+i+(0x800*j)+0x800];
			color = spriteram16[offs  +i+(0x800*j)+0x800]&0xff;

			fx=tile&fx_mask;
			fy=tile&fy_mask;
			tile=tile&sprite_mask;
			if (tile>0x4fff) continue;

			if (flipscreen) {
				if (fx) fx=0; else fx=1;
				if (fy) fy=0; else fy=1;
			}

			if (color)
				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					0,TRANSPARENCY_PEN,0);

			if (flipscreen)
				my=(my-16)&0x1ff;
			else
				my=(my+16)&0x1ff;
		}
	}
}

void alpha68k_V_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	static int last_bank=0;

	if (last_bank!=bank_base)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,Machine->pens[4095],&Machine->visible_area);

	/* This appears to be correct priority */
	if (!strcmp(Machine->gamedrv->name,"skyadvnt") || !strcmp(Machine->gamedrv->name,"skyadvnu")) /* Todo */
	{
		draw_sprites_V(bitmap,0,0x07c0,0x0800,0,0x8000,0x7fff);
		draw_sprites_V(bitmap,1,0x0000,0x0800,0,0x8000,0x7fff);
		draw_sprites_V(bitmap,2,0x0000,0x0800,0,0x8000,0x7fff);
		draw_sprites_V(bitmap,0,0x0000,0x07c0,0,0x8000,0x7fff);
	}
	else	/* gangwars */
	{
		draw_sprites_V(bitmap,0,0x07c0,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(bitmap,1,0x0000,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(bitmap,2,0x0000,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(bitmap,0,0x0000,0x07c0,0x8000,0,0x7fff);
	}

	tilemap_draw(bitmap,fix_tilemap,0,0);
}

void alpha68k_V_sb_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	static int last_bank=0;

	if (last_bank!=bank_base)
 		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,Machine->pens[4095],&Machine->visible_area);

	/* This appears to be correct priority */
	draw_sprites_V(bitmap,0,0x07c0,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(bitmap,1,0x0000,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(bitmap,2,0x0000,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(bitmap,0,0x0000,0x07c0,0x4000,0x8000,0x3fff);

	tilemap_draw(bitmap,fix_tilemap,0,0);
}

/******************************************************************************/

static void draw_sprites2(struct mame_bitmap *bitmap, int c,int d)
{
	int offs,mx,my,color,tile,i;

	for (offs = 0x0000; offs < 0x400; offs += 0x20 )
	{
		mx=spriteram16[offs+c];

		my=mx>>8;
		mx=mx&0xff;

		mx=(mx+0x100)&0x1ff;
		my=(my+0x100)&0x1ff;
		mx-=0x110;
		my-=0x100;
		my=0x200 - my;
		my-=0x200;

		for (i=0; i<0x20; i++) {
			tile=spriteram16[offs+d+i];
			color=1;
			tile&=0x3fff;

			if (tile && tile!=0x3000 && tile!=0x26)
				drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					0,0,
					mx+16,my,
					0,TRANSPARENCY_PEN,0);

			my=(my+8)&0xff;
		}
	}
}

void alpha68k_I_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* This appears to be correct priority */
draw_sprites2(bitmap,3,0x0c00);
draw_sprites2(bitmap,2,0x0800);
draw_sprites2(bitmap,1,0x0400);
//
}

/******************************************************************************/

void kyros_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,bit0,bit1,bit2,bit3;

	for (i = 0;i < 256;i++)
	{
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x100] >> 0) & 0x01;
		bit1 = (color_prom[0x100] >> 1) & 0x01;
		bit2 = (color_prom[0x100] >> 2) & 0x01;
		bit3 = (color_prom[0x100] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x200] >> 0) & 0x01;
		bit1 = (color_prom[0x200] >> 1) & 0x01;
		bit2 = (color_prom[0x200] >> 2) & 0x01;
		bit3 = (color_prom[0x200] >> 3) & 0x01;
		*palette++ = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 0x200;

	for (i = 0;i < 256;i++)
	{
		*colortable++ = ((color_prom[0] & 0x0f) << 4) | (color_prom[0x100] & 0x0f);
		color_prom++;
	}
}


static void kyros_draw_sprites(struct mame_bitmap *bitmap, int c,int d)
{
	int offs,mx,my,color,tile,i,bank,fy;

	for (offs = 0x0000; offs < 0x400; offs += 0x20 )
	{
		mx=spriteram16[offs+c];
		my=0x100-(mx>>8);
		mx=mx&0xff;

		for (i=0; i<0x20; i++) {
			tile=spriteram16[offs+d+i];
			color=(tile&0x4000)>>14;
			fy=tile&0x1000;
			bank=((tile>>10)&0x3)+((tile&0x8000)?4:0);
			tile=(tile&0x3ff)+((tile&0x2000)?0x400:0);

			drawgfx(bitmap,Machine->gfx[bank],
					tile,
					color,
					0,fy,
					mx,my,
					0,TRANSPARENCY_PEN,0);

			my=(my+8)&0xff;
		}
	}
}

void kyros_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	kyros_draw_sprites(bitmap,2,0x0800);
	kyros_draw_sprites(bitmap,3,0x0c00);
	kyros_draw_sprites(bitmap,1,0x0400);
}

/******************************************************************************/

static void sstingry_draw_sprites(struct mame_bitmap *bitmap, int c,int d)
{
	int offs,mx,my,color,tile,i,bank,fx,fy;

	for (offs = 0x0000; offs < 0x400; offs += 0x20 )
	{
		mx=spriteram16[offs+c];
		my=0x100-(mx>>8);
		mx=mx&0xff;

		for (i=0; i<0x20; i++) {
			tile=spriteram16[offs+d+i];
			color=0; //bit 0x4000
			fy=tile&0x1000;
			fx=0;
			tile=(tile&0xfff);
			bank=tile/0x400;

			drawgfx(bitmap,Machine->gfx[bank],
					tile&0x3ff,
					color,
					fx,fy,
					mx,my,
					0,TRANSPARENCY_PEN,0);

			my=(my+8)&0xff;
		}
	}
}

void sstingry_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	sstingry_draw_sprites(bitmap,2,0x0800);
	sstingry_draw_sprites(bitmap,3,0x0c00);
	sstingry_draw_sprites(bitmap,1,0x0400);
}

/******************************************************************************/

static void get_kouyakyu_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=videoram16[offs]&0xff;
	int color=videoram16[offs+1]&0xf;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

WRITE16_HANDLER( kouyakyu_video_w )
{
	COMBINE_DATA(videoram16+offset);
	tilemap_mark_tile_dirty( fix_tilemap, offset/2 );
}

int kouyakyu_vh_start(void)
{
	fix_tilemap = tilemap_create(get_kouyakyu_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

void kouyakyu_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	fillbitmap(bitmap,1,&Machine->visible_area);

sstingry_draw_sprites(bitmap,2,0x0800);
sstingry_draw_sprites(bitmap,3,0x0c00);
sstingry_draw_sprites(bitmap,1,0x0400);

	tilemap_draw(bitmap,fix_tilemap,0,0);
}
