/*
	Todo:

	Sprite format in Dragon Gun/Locked N Loaded
	Sprite flipscreen in Captain America
	Finish Fighter's History when playable.

*/

#include "driver.h"
#include "vidhrdw/generic.h"

data32_t *deco32_pf1_data,*deco32_pf2_data,*deco32_pf3_data,*deco32_pf4_data;
data32_t *deco32_pf12_control,*deco32_pf34_control;
data32_t *deco32_pf1_colscroll,*deco32_pf2_colscroll,*deco32_pf3_colscroll,*deco32_pf4_colscroll;
data32_t *deco32_pf1_rowscroll,*deco32_pf2_rowscroll,*deco32_pf3_rowscroll,*deco32_pf4_rowscroll;

static struct tilemap *pf1_tilemap,*pf1a_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static int deco32_pf1_bank,deco32_pf2_bank,deco32_pf3_bank,deco32_pf4_bank;
static int deco32_pf2_colourbank,pri,scroll_hack;

/******************************************************************************/

WRITE32_HANDLER( deco32_pf1_data_w )
{
	data32_t oldword=deco32_pf1_data[offset];
	COMBINE_DATA(&deco32_pf1_data[offset]);

	if (oldword!=deco32_pf1_data[offset]) {
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
		if (pf1a_tilemap && offset<0x400)
			tilemap_mark_tile_dirty(pf1a_tilemap,offset);
	}
}

WRITE32_HANDLER( deco32_pf2_data_w )
{
	data32_t oldword=deco32_pf2_data[offset];
	COMBINE_DATA(&deco32_pf2_data[offset]);

	if (oldword!=deco32_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE32_HANDLER( deco32_pf3_data_w )
{
	data32_t oldword=deco32_pf3_data[offset];
	COMBINE_DATA(&deco32_pf3_data[offset]);

	if (oldword!=deco32_pf3_data[offset])
		tilemap_mark_tile_dirty(pf3_tilemap,offset);
}

WRITE32_HANDLER( deco32_pf4_data_w )
{
	data32_t oldword=deco32_pf4_data[offset];
	COMBINE_DATA(&deco32_pf4_data[offset]);

	if (oldword!=deco32_pf4_data[offset])
		tilemap_mark_tile_dirty(pf4_tilemap,offset);
}

/******************************************************************************/

WRITE32_HANDLER( deco32_unknown_w )
{
	/* ???  Captain America writes either 0xc8 or 0xca to here.  When 0xc8
	pf2 seems to need a scroll hack to be in line - see the intro sequence!? */
	scroll_hack=data;
}

WRITE32_HANDLER( deco32_pri_w )
{
	pri=data;
}

WRITE32_HANDLER( deco32_paletteram_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	r = (paletteram32[offset] >> 0) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	b = (paletteram32[offset] >>16) & 0xff;

	palette_set_color(offset,r,g,b);
}

/******************************************************************************/

static void captaven_drawsprites(struct mame_bitmap *bitmap, data32_t *spritedata, int gfxbank)
{
	int offs;

	/*
		Word 0:
			0x8000:	Y flip
			0x4000: X flip
			0x2000:	Flash (Sprite toggles on/off every frame)
			0x1fff:	Y value
		Word 1:
			0xffff: X value
		Word 2:
			0xf000:	Block height
			0x0f00: Block width
			0x00c0: Unused?
			0x0020: Priority
			0x001f: Colour
		Word 3:
			0xffff:	Sprite value
	*/

	for (offs = 0x400-4;offs >=0;offs -= 4)
	{
		int sx,sy,sprite,colour,fx,fy,x_mult,y_mult,w,h,x,y,prival;

		sy = spritedata[offs+0];
		sprite = spritedata[offs+3] & 0xffff;

		if (sy==0x00000108 && !sprite)
			continue; //fix!!!!!

		if (spritedata[offs+2]&0x20)
			prival=0;
		else
			prival=2;

		sx = spritedata[offs+1];

		if ((sy&0x2000) && (cpu_getcurrentframe() & 1)) continue;

		colour = (spritedata[offs+2] >>0) & 0x1f;

		h = (spritedata[offs+2]&0xf000)>>12;
		w = (spritedata[offs+2]&0x0f00)>> 8;
		fx = !(spritedata[offs+0]&0x4000);
		fy = !(spritedata[offs+0]&0x8000);

		sx = sx & 0x01ff;
		sy = sy & 0x01ff;
		if (sx >= 320) sx -= 512;
		if (sy >= 256) sy -= 512;

		if (flip_screen) {
			sy=304-sy;
			sx=240-sx;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
		}

		if (fx) { x_mult=-16; sx+=16*w; } else { x_mult=16; sx-=16; }
		if (fy) { y_mult=-16; sy+=16*h; } else { y_mult=16; sy-=16; }

		for (x=0; x<w; x++) {
			for (y=0; y<h; y++) {
				pdrawgfx(bitmap,Machine->gfx[gfxbank],
						sprite + y + h * x,
						colour,
						fx,fy,
						sx + x_mult * (w-x),sy + y_mult * (h-y),
						&Machine->visible_area,TRANSPARENCY_PEN,0,prival);
			}
		}
	}
}

static void fghthist_drawsprites(struct mame_bitmap *bitmap, data32_t *spritedata, int gfxbank)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spritedata[offs+1] & 0xffff;

		y = spritedata[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spritedata[offs+2];
		colour = (x >>9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
//		x = 304 - x;
//		y = 240 - y;

//		if (x>320 || x<-16) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		mult=+16;

		if (fx) fx=0; else fx=1;
		if (fy) fy=0; else fy=1;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[gfxbank],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static void dragngun_drawsprites(struct mame_bitmap *bitmap, data32_t *spritedata, int gfxbank)
{
	int offs;

	for (offs = 0;offs < 0x800;offs += 4)
	{
		int sx,sy,sprite,colour,fx,fy,inc,flash,x_mult,y_mult,w,h,x,y;


		sy = spritedata[offs+3];
		sprite = ((spritedata[offs+4]&0xff)<<8)|((spritedata[offs+5]&0xff)<<0);

		if (sy==0x00000108 && !sprite)
			continue; //fix!!!!!

		sx = spritedata[offs+2];

		flash=sy&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		colour = (spritedata[offs+2] >>0) & 0x1f;

//		multi = (1 << ((spritedata[offs+2] & 0x6000) >> 13)) - 1;	/* 1x, 2x, 4x, 8x height */

		h=1;//(spritedata[offs+2]&0xf000)>>12;
		w=1;//(spritedata[offs+2]&0x0f00)>> 8;
		fx = !(spritedata[offs+0]&0x4000);
		fy = 0;//spritedata[offs+0]&0x2000;

//continue on bit 8000 not set??

		sx = sx & 0x01ff;
		sy = sy & 0x01ff;
		if (sx >= 320) sx -= 512;
		if (sy >= 256) sy -= 512;
//		sx = 304 - sx;
//		sy = 240 - sy;

//		if (x>320 || x<-16) continue;

//		sprite &= ~multi;
		if (fy) {
			inc = -1;
//			sprite += h;//multi;
		}
		else
		{
//			sprite += h;//multi;
			inc = 1;
		}

		if (fx) x_mult=-16; else x_mult=16;

		if (flip_screen) {
			sy=304-sy;
			sx=240-sx;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			y_mult=-16;
		}
		else y_mult=16;

//]		fy=1;

//multi=1;

//	if (multi==1) y-=16;

		sy-=16;
		if (fx) sx+=16*w; else sx-=16;
//		sx-=16;

		for (x=0; x<w; x++) {
			for (y=0; y<h; y++) {
				drawgfx(bitmap,Machine->gfx[gfxbank],
						sprite + y * inc + h * x,
						colour,
						fx,fy,
						sx + x_mult * (w-x),sy + y_mult * (h-y),// - (multi*16),
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

/******************************************************************************/

static void get_pf1_tile_info(int tile_index)
{
	int tile=deco32_pf1_data[tile_index];
	SET_TILE_INFO(0,tile&0xfff,(tile>>12)&0xf,0)
}

static void get_pf1a_tile_info(int tile_index)
{
	int tile=deco32_pf1_data[tile_index];
	SET_TILE_INFO(1,tile&0xfff,(tile>>12)&0xf,0)
}

static void get_pf2_tile_info(int tile_index)
{
	int tile=deco32_pf2_data[tile_index];
	SET_TILE_INFO(1,(tile&0xfff)|deco32_pf2_bank,((tile >> 12)&0xf)+deco32_pf2_colourbank,0)
}

static void get_pf3_tile_info(int tile_index)
{
	int tile=deco32_pf3_data[tile_index];
	SET_TILE_INFO(2,(tile&0xfff)|deco32_pf3_bank,(tile >> 12)&0xf,0)
}

static void get_pf4_tile_info(int tile_index)
{
	int tile=deco32_pf4_data[tile_index];
	SET_TILE_INFO(2,(tile&0xfff)|deco32_pf4_bank,(tile >> 12)&0xf,0)
}

/* Captain America tilemap chip 2 has different banking and colour from normal */
static void get_ca_pf3_tile_info(int tile_index)
{
	int tile=deco32_pf3_data[tile_index];
	SET_TILE_INFO(2,(tile&0x3fff)+deco32_pf3_bank,(tile >> 14)&3,0)
}

static void get_ll_pf3_tile_info(int tile_index)
{
	int tile=deco32_pf3_data[tile_index];
	SET_TILE_INFO(2,(tile&0x0fff)|deco32_pf3_bank,(tile >> 12)&3,0)
}

static void get_ll_pf4_tile_info(int tile_index)
{
	int tile=deco32_pf4_data[tile_index];
	SET_TILE_INFO(2,(tile&0x0fff)|deco32_pf4_bank,(tile >> 12)&3,0)
}

VIDEO_START( captaven )
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf1a_tilemap =tilemap_create(get_pf1a_tile_info,   tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf3_tilemap = tilemap_create(get_ca_pf3_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if (!pf1_tilemap || !pf1a_tilemap ||!pf2_tilemap || !pf3_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf1a_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);

	deco32_pf2_colourbank=16;

	return 0;
}

VIDEO_START( fghthist )
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf3_tilemap = tilemap_create(get_pf3_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf4_tilemap = tilemap_create(get_pf4_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);
	pf1a_tilemap =0;

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);

	deco32_pf2_colourbank=0;

	return 0;
}

VIDEO_START( dragngun )
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf3_tilemap = tilemap_create(get_ll_pf3_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf4_tilemap = tilemap_create(get_ll_pf4_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);
	pf1a_tilemap =0;

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);

	deco32_pf2_colourbank=0;

	return 0;
}

VIDEO_START( tattass )
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf3_tilemap = tilemap_create(get_ll_pf3_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pf4_tilemap = tilemap_create(get_ll_pf4_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,32);
	pf1a_tilemap =0;

	return 0;
}

/******************************************************************************/

VIDEO_EOF( captaven )
{
	memcpy(buffered_spriteram32,spriteram32,spriteram_size);
}

#if 0
static void print_debug_info()
{
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	int j,trueorientation;
	char buf[64];

	trueorientation = Machine->orientation;
	Machine->orientation = ROT0;

	sprintf(buf,"%04X %04X %04X %04X",deco32_pf12_control[0],deco32_pf12_control[1],deco32_pf12_control[2],deco32_pf12_control[3]);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,40,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",deco32_pf12_control[4],deco32_pf12_control[5],deco32_pf12_control[6],deco32_pf12_control[7]);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,48,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"%04X %04X %04X %04X",deco32_pf34_control[0],deco32_pf34_control[1],deco32_pf34_control[2],deco32_pf34_control[3]);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,60,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",deco32_pf34_control[4],deco32_pf34_control[5],deco32_pf34_control[6],deco32_pf34_control[7]);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,68,0,TRANSPARENCY_NONE,0);
}
#endif

static void deco32_setup_scroll(struct tilemap *pf_tilemap, data16_t height, data8_t control0, data8_t control1, data16_t sy, data16_t sx, data32_t *rowdata, data32_t *coldata)
{
	int rows,offs;

	/* Colscroll - not fully supported yet! */
	if (control1&0x20 && coldata)
		sy+=coldata[0];

	/* Rowscroll enable */
	if (control1&0x40 && rowdata) {
		tilemap_set_scroll_cols(pf_tilemap,1);
		tilemap_set_scrolly( pf_tilemap,0, sy );

		/* Several different rowscroll styles */
		switch ((control0>>3)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}
		if (height<rows) rows/=2; /* 8x8 tile layers have half as many lines as 16x16 */

		tilemap_set_scroll_rows(pf_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf_tilemap,offs, sx + rowdata[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf_tilemap,1);
		tilemap_set_scroll_cols(pf_tilemap,1);
		tilemap_set_scrollx( pf_tilemap, 0, sx );
		tilemap_set_scrolly( pf_tilemap, 0, sy );
	}
}

VIDEO_UPDATE( captaven )
{
	int pf1_enable,pf2_enable,pf3_enable;
	static int last_pf3_bank;

	flip_screen_set(deco32_pf12_control[0]&0x80);
	tilemap_set_flip(ALL_TILEMAPS,flip_screen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	deco32_setup_scroll(pf1_tilemap, 256,(deco32_pf12_control[5]>>0)&0xff,(deco32_pf12_control[6]>>0)&0xff,deco32_pf12_control[2],deco32_pf12_control[1],deco32_pf1_rowscroll,deco32_pf1_rowscroll+0x200);
	deco32_setup_scroll(pf1a_tilemap,512,(deco32_pf12_control[5]>>0)&0xff,(deco32_pf12_control[6]>>0)&0xff,deco32_pf12_control[2],deco32_pf12_control[1],deco32_pf1_rowscroll,deco32_pf1_rowscroll+0x200);
	if (scroll_hack==0xc8)
		deco32_setup_scroll(pf2_tilemap,512,(deco32_pf12_control[5]>>8)&0xff,(deco32_pf12_control[6]>>8)&0xff,deco32_pf12_control[4]-0x60,deco32_pf12_control[3],deco32_pf2_rowscroll,deco32_pf2_rowscroll+0x200);
	else
		deco32_setup_scroll(pf2_tilemap, 512,(deco32_pf12_control[5]>>8)&0xff,(deco32_pf12_control[6]>>8)&0xff,deco32_pf12_control[4],deco32_pf12_control[3],deco32_pf2_rowscroll,deco32_pf2_rowscroll+0x200);
	deco32_setup_scroll(pf3_tilemap, 512,(deco32_pf34_control[5]>>0)&0xff,(deco32_pf34_control[6]>>0)&0xff,deco32_pf34_control[4],deco32_pf34_control[3],deco32_pf3_rowscroll,deco32_pf3_rowscroll+0x200);

	/* PF1 & PF2 only have enough roms for 1 bank */
	deco32_pf1_bank=0;//(deco32_pf12_control[7]>> 4)&0xf;
	deco32_pf2_bank=0;//(deco32_pf12_control[7]>>12)&0xf;
	deco32_pf3_bank=(deco32_pf34_control[7]>> 4)&0xf;

	if (deco32_pf34_control[7]&0x0020) deco32_pf3_bank=0x4000; else deco32_pf3_bank=0;
	if (deco32_pf3_bank!=last_pf3_bank) tilemap_mark_all_tiles_dirty(pf3_tilemap);
	last_pf3_bank=deco32_pf3_bank;

	pf1_enable=deco32_pf12_control[5]&0x0080;
	pf2_enable=deco32_pf12_control[5]&0x8000;
	pf3_enable=deco32_pf34_control[5]&0x0080;

	fillbitmap(priority_bitmap,0,cliprect);
	if ((pri&1)==0) {
		if (pf3_enable)
			tilemap_draw(bitmap,cliprect,pf3_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		else
			fillbitmap(bitmap,get_black_pen(),cliprect);

		if (pf2_enable)
			tilemap_draw(bitmap,cliprect,pf2_tilemap,0,1);
	} else {
		if (pf2_enable)
			tilemap_draw(bitmap,cliprect,pf2_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		else
			fillbitmap(bitmap,get_black_pen(),cliprect);

		if (pf3_enable)
			tilemap_draw(bitmap,cliprect,pf3_tilemap,0,1);
	}

	captaven_drawsprites(bitmap,buffered_spriteram32,3);

	if (pf1_enable) {
		/* PF1 can be in 8x8 mode or 16x16 mode */
		if (deco32_pf12_control[6]&0x80)
			tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
		else
			tilemap_draw(bitmap,cliprect,pf1a_tilemap,0,0);
	}

//	print_debug_info();
}

VIDEO_UPDATE( fghthist )
{
	tilemap_set_scrollx( pf1_tilemap,0, deco32_pf12_control[1] );
	tilemap_set_scrolly( pf1_tilemap,0, deco32_pf12_control[2] );
	tilemap_set_scrollx( pf2_tilemap,0, deco32_pf12_control[3] );
	tilemap_set_scrolly( pf2_tilemap,0, deco32_pf12_control[4] );
	tilemap_set_scrollx( pf3_tilemap,0, deco32_pf34_control[1] );
	tilemap_set_scrolly( pf3_tilemap,0, deco32_pf34_control[2] );
	tilemap_set_scrollx( pf4_tilemap,0, deco32_pf34_control[3] );
	tilemap_set_scrolly( pf4_tilemap,0, deco32_pf34_control[4] );

	/* Tilemap graphics banking */
//	if ((((deco32_pf12_control[7]>>12)&0x7)<<12)!=deco32_pf2_bank)
//		tilemap_mark_all_tiles_dirty(pf3_tilemap);
//	if ((((deco32_pf34_control[7]>> 5)&0x7)<<12)!=deco32_pf3_bank)
//		tilemap_mark_all_tiles_dirty(pf3_tilemap);
//	if ((((deco32_pf34_control[7]>>13)&0x7)<<12)!=deco32_pf4_bank)
//		tilemap_mark_all_tiles_dirty(pf4_tilemap);

	deco32_pf2_bank=0x1000;//((deco32_pf12_control[7]>>12)&0x7)<<12;
	deco32_pf3_bank=0;//((deco32_pf34_control[7]>> 5)&0x7)<<12;
	deco32_pf4_bank=0;//((deco32_pf34_control[7]>>13)&0x7)<<12;

//	tilemap_set_enable(pf1_tilemap,deco32_pf12_control[5]&0x0080);
//	tilemap_set_enable(pf2_tilemap,deco32_pf12_control[5]&0x8000);
//	tilemap_set_enable(pf3_tilemap,deco32_pf34_control[5]&0x0080);
//	tilemap_set_enable(pf4_tilemap,deco32_pf34_control[5]&0x8000);

	tilemap_draw(bitmap,cliprect,pf4_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf3_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	fghthist_drawsprites(bitmap,spriteram32,3);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);

//	print_debug_info();
}

VIDEO_UPDATE( dragngun )
{
	tilemap_set_scrollx( pf1_tilemap,0, deco32_pf12_control[1] );
	tilemap_set_scrolly( pf1_tilemap,0, deco32_pf12_control[2] );
	tilemap_set_scrollx( pf2_tilemap,0, deco32_pf12_control[3] );
	tilemap_set_scrolly( pf2_tilemap,0, deco32_pf12_control[4] );
	tilemap_set_scrollx( pf3_tilemap,0, deco32_pf34_control[1] );
	tilemap_set_scrolly( pf3_tilemap,0, deco32_pf34_control[2] );
	tilemap_set_scrollx( pf4_tilemap,0, deco32_pf34_control[3] );
	tilemap_set_scrolly( pf4_tilemap,0, deco32_pf34_control[4] );

	/* Tilemap graphics banking */
	if ((((deco32_pf12_control[7]>>12)&0x7)<<12)!=deco32_pf2_bank)
		tilemap_mark_all_tiles_dirty(pf3_tilemap);
	if ((((deco32_pf34_control[7]>> 5)&0x7)<<12)!=deco32_pf3_bank)
		tilemap_mark_all_tiles_dirty(pf3_tilemap);
	if ((((deco32_pf34_control[7]>>13)&0x7)<<12)!=deco32_pf4_bank)
		tilemap_mark_all_tiles_dirty(pf4_tilemap);
	deco32_pf3_bank=((deco32_pf12_control[7]>>12)&0x7)<<12;
	deco32_pf3_bank=((deco32_pf34_control[7]>> 5)&0x7)<<12;
	deco32_pf4_bank=((deco32_pf34_control[7]>>13)&0x7)<<12;

//	tilemap_set_enable(pf1_tilemap,deco32_pf12_control[5]&0x0080);
//	tilemap_set_enable(pf2_tilemap,deco32_pf12_control[5]&0x8000);
//	tilemap_set_enable(pf3_tilemap,deco32_pf34_control[5]&0x0080);
//	tilemap_set_enable(pf4_tilemap,deco32_pf34_control[5]&0x8000);

	tilemap_draw(bitmap,cliprect,pf4_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf3_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
/*


ram at 0x220000

  Bit 0x400 - use upper sprite bank, else lower bank


	0x1388	start of sprite routine









  */



	dragngun_drawsprites(bitmap,spriteram32,3);
//	dragngun_drawsprites_2(bitmap,spriteram32_2,3);

//	print_debug_info();
}

VIDEO_UPDATE( tattass )
{
//	tilemap_draw(bitmap,pf1_tilemap,0,0);
	int offs;

	for (offs=0; offs<0x2000/4; offs++) {
		int tile;

		tile=deco32_pf1_data[offs];
		if (tile)
			tile+=28;
		drawgfx(bitmap,Machine->uifont,tile,0,0,0,(offs%64)*8,(offs/64)*8,0,TRANSPARENCY_NONE,0);
	}
}
