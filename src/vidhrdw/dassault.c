/***************************************************************************

   Desert Assault Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *dassault_pf1_data,*dassault_pf2_data;
data16_t *dassault_pf3_data,*dassault_pf4_data;
data16_t *dassault_pf4_rowscroll, *dassault_pf2_rowscroll;

static struct tilemap *pf1_tilemap,*pf2_tilemap;
static struct tilemap *pf3_tilemap,*pf4_tilemap;
static struct tilemap *pf1_8x8_tilemap,*pf4_8x8_tilemap;
static int playfield_priority;

static struct mame_bitmap *sprite_priority_bitmap;

static data16_t dassault_control_0[8];
static data16_t dassault_control_1[8];

static int dassault_pf1_bank,dassault_pf2_bank,dassault_pf3_bank,dassault_pf4_bank;

/******************************************************************************/

WRITE16_HANDLER( dassault_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}

/******************************************************************************/

static void dassault_pdrawgfx(struct mame_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,UINT32 pri_mask, UINT32 sprite_mask)
{
	int ox;
	int oy;
	int cx;
	int x_index,y_index;
	int x,y;
	int cy;

	const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
	int source_base = (code % gfx->total_elements) * gfx->height;

	/* check bounds */
	ox = sx;
	oy = sy;

	if (sx>319 || sy>247 || sx<-15 || sy<-7)
		return;

	if (sy<0) sy=0;
	if (sx<0) sx=0;
	if (sx>319) cx=319;
	else cx=ox+16;

	cy=(sy-oy);

	if (flipy) y_index=15-cy; else y_index=cy;

	for( y=0; y<16-cy; y++ )
	{
		UINT8 *source = gfx->gfxdata + ((source_base+y_index) * gfx->line_modulo);
		UINT32 *destb = (UINT32 *)dest->line[sy];
		UINT8 *pri = priority_bitmap->line[sy];
		UINT8 *spri = sprite_priority_bitmap->line[sy];

		if (flipx) { source+=15-(sx-ox); x_index=-1; } else { x_index=1; source+=(sx-ox); }

		for (x=sx; x<cx; x++)
		{
			int c = *source;
			if( c != transparent_color )
			{
				if (pri_mask>pri[x] && sprite_mask>spri[x]) {
					if (transparency == TRANSPARENCY_ALPHA)
						destb[x] = alpha_blend32(destb[x], pal[c]);
					else
						destb[x] = pal[c];
					pri[x] |= pri_mask;//pri_or;
				}
				spri[x]|=sprite_mask;
			}
			source+=x_index;
		}

		sy++;
		if (sy>247)
			return;
		if (flipy) y_index--; else y_index++;
	}
}

static void dassault_drawsprites(struct mame_bitmap *bitmap, int pf_priority)
{
	int x,y,sprite,colour,multi,fx,fy,inc,flash;
	int offs, bank, gfxbank;
	const data16_t *spritebase;

	/* Have to loop over the two sprite sources */
	for (bank=0; bank<2; bank++)
	{
		for (offs = 0x800-4;offs >= 0; offs -= 4)
		{
			int trans=TRANSPARENCY_PEN, pmask=0;

			/* Draw the main spritebank after the other one */
			if (bank==0)
			{
				spritebase=buffered_spriteram16;
				gfxbank=3;
			}
			else
			{
				spritebase=buffered_spriteram16_2;
				gfxbank=4;
			}

			sprite = spritebase[offs+1] & 0x7fff;
			if (!sprite) continue;

			x = spritebase[offs+2];

			/* Alpha on chip 2 only */
			if (bank==1 && x&0xc000)
				trans=TRANSPARENCY_ALPHA;

			y = spritebase[offs];
			flash=y&0x1000;
			if (flash && (cpu_getcurrentframe() & 1)) continue;
			colour = (x >> 9) &0x1f;
			if (y&0x8000) colour+=32;

			fx = y & 0x2000;
			fy = y & 0x4000;
			multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

			x = x & 0x01ff;
			y = y & 0x01ff;
			if (x >= 320) x -= 512;
			if (y >= 256) y -= 512;
			x = 304 - x;
			y = 240 - y;

			if (x>320) continue; /* Speedup */

			sprite &= ~multi;
			if (fy)
				inc = -1;
			else
			{
				sprite += multi;
				inc = 1;
			}

			/* Priority */
			switch (pf_priority&3) {
			case 0:
				if (bank==0) {
					if (spritebase[offs+2]&0x8000) pmask=8;
					else if (spritebase[offs+2]&0x4000) pmask=32;
					else pmask=128;
				} else {
					if (spritebase[offs+2]&0x8000) pmask=64; /* Check */
					else pmask=64;
				}
				break;

			case 1:
				if (bank==0) {
					if (spritebase[offs+2]&0x8000) pmask=8;
					else if (spritebase[offs+2]&0x4000) pmask=32;
					else pmask=128;
				} else {
					if (spritebase[offs+2]&0x8000) pmask=16; /* Check */
					else pmask=16;
				}
				break;

			case 2: /* Unused */
			case 3:
				if (bank==0) {
					if (spritebase[offs+2]&0x8000) pmask=8;
					else if (spritebase[offs+2]&0x4000) pmask=32;
					else pmask=128;
				} else {
					if (spritebase[offs+2]&0x8000) pmask=64; /* Check */
					else pmask=64;
				}
				break;
			}

			while (multi >= 0)
			{
				dassault_pdrawgfx(bitmap,Machine->gfx[gfxbank],
						sprite - multi * inc,
						colour,
						fx,fy,
						x,y - 16 * multi,
						&Machine->visible_area,trans,0,pmask,1<<bank);

				multi--;
			}
		}
	}
}

/******************************************************************************/

static UINT32 back_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

static void get_pf1_tile_info(int tile_index)
{
	int tile=dassault_pf1_data[tile_index];

	SET_TILE_INFO(1,(tile&0xfff)|dassault_pf1_bank,tile>>12,0)
}

static void get_pf2_tile_info(int tile_index)
{
	int tile=dassault_pf2_data[tile_index];

	SET_TILE_INFO(2,(tile&0xfff)|dassault_pf2_bank,(tile>>12)+16,0)
}

static void get_pf3_tile_info(int tile_index)
{
	int tile=dassault_pf3_data[tile_index];

	SET_TILE_INFO(2,(tile&0xfff)|dassault_pf3_bank,tile>>12,0)
}

static void get_pf4_tile_info(int tile_index)
{
	int tile=dassault_pf4_data[tile_index];

	SET_TILE_INFO(1,(tile&0xfff)|dassault_pf4_bank,(tile>>12)+16,0)
	if ((dassault_control_1[6]&0x100) && tile&0x8000) /* May apply to other tilemaps as well */
		tile_info.flags=TILE_FLIPX;
	else
		tile_info.flags=0;
}

static void get_pf1_tile_info8x8(int tile_index)
{
	int tile=dassault_pf1_data[tile_index];

	SET_TILE_INFO(0,(tile&0xfff)|dassault_pf1_bank,tile>>12,0)
}

static void get_pf4_tile_info8x8(int tile_index)
{
	int tile=dassault_pf4_data[tile_index];

	SET_TILE_INFO(0,(tile&0xfff)|dassault_pf4_bank,(tile>>12)+16,0)
}

/******************************************************************************/

WRITE16_HANDLER( dassault_pf1_data_w )
{
	data16_t oldword=dassault_pf1_data[offset];
	COMBINE_DATA(&dassault_pf1_data[offset]);
	if (oldword!=dassault_pf1_data[offset]) {
		if (offset<0x800)
			tilemap_mark_tile_dirty(pf1_tilemap,offset);
		tilemap_mark_tile_dirty(pf1_8x8_tilemap,offset);
	}
}

WRITE16_HANDLER( dassault_pf2_data_w )
{
	data16_t oldword=dassault_pf2_data[offset];
	COMBINE_DATA(&dassault_pf2_data[offset]);
	if (oldword!=dassault_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( dassault_pf3_data_w )
{
	data16_t oldword=dassault_pf3_data[offset];
	COMBINE_DATA(&dassault_pf3_data[offset]);
	if (oldword!=dassault_pf3_data[offset])
		tilemap_mark_tile_dirty(pf3_tilemap,offset);
}

WRITE16_HANDLER( dassault_pf4_data_w )
{
	data16_t oldword=dassault_pf4_data[offset];
	COMBINE_DATA(&dassault_pf4_data[offset]);
	if (oldword!=dassault_pf4_data[offset]) {
		if (offset<0x800)
			tilemap_mark_tile_dirty(pf4_tilemap,offset);
		tilemap_mark_tile_dirty(pf4_8x8_tilemap,offset);
	}
}

WRITE16_HANDLER( dassault_control_0_w )
{
	COMBINE_DATA(&dassault_control_0[offset]);
}

WRITE16_HANDLER( dassault_control_1_w )
{
	COMBINE_DATA(&dassault_control_1[offset]);
}

WRITE16_HANDLER( dassault_priority_w )
{
	playfield_priority=data&0xff;
}

/******************************************************************************/

VIDEO_START( dassault )
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info,       back_scan,        TILEMAP_TRANSPARENT,16,16,64,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,       back_scan,        TILEMAP_OPAQUE,     16,16,64,32);
	pf3_tilemap = tilemap_create(get_pf3_tile_info,       back_scan,        TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_pf4_tile_info,       back_scan,        TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_8x8_tilemap = tilemap_create(get_pf4_tile_info8x8,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	pf1_8x8_tilemap = tilemap_create(get_pf1_tile_info8x8,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	sprite_priority_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap || !pf4_8x8_tilemap || !pf1_8x8_tilemap || !sprite_priority_bitmap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	tilemap_set_transparent_pen(pf1_8x8_tilemap,0);
	tilemap_set_transparent_pen(pf4_8x8_tilemap,0);

	alpha_set_level(0x80);

	return 0;
}

/******************************************************************************/

VIDEO_UPDATE( dassault )
{
	static int last_pf1_bank,last_pf3_bank,last_pf4_bank,last_pf2_bank;
	int pf23_control,pf14_control,offs;

	/* These two tilemaps don't change size within the game */
	tilemap_set_enable(pf2_tilemap,dassault_control_0[5]&0x8000);
	tilemap_set_enable(pf3_tilemap,dassault_control_0[5]&0x0080);
	if (dassault_control_1[6]&0x80) {
		tilemap_set_enable(pf1_8x8_tilemap,dassault_control_1[5]&0x80);
		tilemap_set_enable(pf1_tilemap,0);
	} else {
		tilemap_set_enable(pf1_tilemap,dassault_control_1[5]&0x80);
		tilemap_set_enable(pf1_8x8_tilemap,0);
	}
	if (dassault_control_1[6]&0x8000) {
		tilemap_set_enable(pf4_8x8_tilemap,dassault_control_1[5]&0x8000);
		tilemap_set_enable(pf4_tilemap,0);
	} else {
		tilemap_set_enable(pf4_tilemap,dassault_control_1[5]&0x8000);
		tilemap_set_enable(pf4_8x8_tilemap,0);
	}

	/* Setup scrolling */
	if (dassault_control_0[5]&0x4000) {
		/* PF2 has rowscroll - todo abstract this and support rowscroll for all
		playfields (though in this game this is the only use */
		tilemap_set_scroll_rows(pf2_tilemap,32);
		for (offs=0; offs<32; offs++)
			tilemap_set_scrollx( pf2_tilemap,offs, dassault_control_0[3] + dassault_pf2_rowscroll[offs] );
		tilemap_set_scrolly( pf2_tilemap,0, dassault_control_0[4] );
	} else {
		tilemap_set_scroll_rows(pf2_tilemap,1);
		tilemap_set_scrollx( pf2_tilemap,0, dassault_control_0[3] );
		tilemap_set_scrolly( pf2_tilemap,0, dassault_control_0[4] );
	}
	tilemap_set_scrollx( pf3_tilemap,0, dassault_control_0[1] );
	tilemap_set_scrolly( pf3_tilemap,0, dassault_control_0[2] );
	tilemap_set_scrollx( pf4_tilemap,0, dassault_control_1[3] );
	tilemap_set_scrolly( pf4_tilemap,0, dassault_control_1[4] );
	tilemap_set_scrollx( pf1_tilemap,0, dassault_control_1[1] );
	tilemap_set_scrolly( pf1_tilemap,0, dassault_control_1[2] );
	tilemap_set_scrollx( pf1_8x8_tilemap,0, dassault_control_1[1] );
	tilemap_set_scrolly( pf1_8x8_tilemap,0, dassault_control_1[2] );
	tilemap_set_scrollx( pf4_8x8_tilemap,0, dassault_control_1[3] );
	tilemap_set_scrolly( pf4_8x8_tilemap,0, dassault_control_1[4] );

	/* Handle gfx rom switching, then update playfields */
	pf23_control=dassault_control_0[7];
	pf14_control=dassault_control_1[7];

	/* Playfield 2 */
	switch (pf23_control>>8) {
		case 0x01: dassault_pf2_bank=0x0000; break;
		case 0x11: dassault_pf2_bank=0x1000; break;
		case 0x21: dassault_pf2_bank=0x2000; break;
		case 0x31: dassault_pf2_bank=0x3000; break;
		default:   dassault_pf2_bank=0x0000; break;
	}
	if (last_pf2_bank!=dassault_pf2_bank)
		tilemap_mark_all_tiles_dirty(pf2_tilemap);
	last_pf2_bank=dassault_pf2_bank;

	/* Playfield 3 */
	switch (pf23_control&0xff) {
		case 0x01: dassault_pf3_bank=0x0000; break;
		case 0x11: dassault_pf3_bank=0x1000; break;
		case 0x21: dassault_pf3_bank=0x2000; break;
		case 0x31: dassault_pf3_bank=0x3000; break;
		default:   dassault_pf3_bank=0x0000; break;
	}
	if (last_pf3_bank!=dassault_pf3_bank)
		tilemap_mark_all_tiles_dirty(pf3_tilemap);
	last_pf3_bank=dassault_pf3_bank;

	/* Playfield 4 */
	switch (pf14_control>>8) {
		case 0x01: dassault_pf4_bank=0x0000; break;
		case 0x11: dassault_pf4_bank=0x1000; break;
		case 0x81: dassault_pf4_bank=0x8000; break;
		default:   dassault_pf4_bank=0x0000; break;
	}
	if (last_pf4_bank!=dassault_pf4_bank) {
		tilemap_mark_all_tiles_dirty(pf4_8x8_tilemap);
		tilemap_mark_all_tiles_dirty(pf4_tilemap);
	}
	last_pf4_bank=dassault_pf4_bank;

	/* Playfield 1 */
	switch (pf14_control&0xff) {
		case 0x01: dassault_pf1_bank=0x0000; break;
		case 0x11: dassault_pf1_bank=0x1000; break;
		case 0x21: dassault_pf1_bank=0x2000; break;
		case 0x81: dassault_pf1_bank=0x8000; break;
		default:   dassault_pf1_bank=0x0000; break;
	}
	if (last_pf1_bank!=dassault_pf1_bank) {
		tilemap_mark_all_tiles_dirty(pf1_8x8_tilemap);
		tilemap_mark_all_tiles_dirty(pf1_tilemap);
	}
	last_pf1_bank=dassault_pf1_bank;

	/* Draw playfields/update priority bitmap */
	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(sprite_priority_bitmap,0,NULL);
	if (dassault_control_0[5]&0x8000)
		tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	else {
		fillbitmap(bitmap,Machine->pens[3072],&Machine->visible_area);
		fillbitmap(priority_bitmap,0,&Machine->visible_area);
	}

	/* The middle playfields can be swapped priority-wise */
	if ((playfield_priority&3)==0) {
		tilemap_draw(bitmap,cliprect,pf4_8x8_tilemap,0,2);
		tilemap_draw(bitmap,cliprect,pf4_tilemap,0,2);
		tilemap_draw(bitmap,cliprect,pf3_tilemap,0,16);
	} else if ((playfield_priority&3)==1) {
		tilemap_draw(bitmap,cliprect,pf3_tilemap,0,2);
		tilemap_draw(bitmap,cliprect,pf4_8x8_tilemap,0,64);
		tilemap_draw(bitmap,cliprect,pf4_tilemap,0,64);
	} else if ((playfield_priority&3)==3) {
		tilemap_draw(bitmap,cliprect,pf3_tilemap,0,2);
		tilemap_draw(bitmap,cliprect,pf4_8x8_tilemap,0,16);
		tilemap_draw(bitmap,cliprect,pf4_tilemap,0,16);
	} else {
		/* Unused */
	}

	/* Draw sprites - two sprite generators, with selectable priority */
	dassault_drawsprites(bitmap,playfield_priority);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf1_8x8_tilemap,0,0);
}

/******************************************************************************/
