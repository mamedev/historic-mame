/*******************************************************************

Rolling Thunder Video Hardware

*******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


data8_t *rthunder_videoram1, *rthunder_videoram2, *rthunder_spriteram;

static int tilebank;
static int xscroll[4], yscroll[4];	/* scroll + priority */

static struct tilemap *tilemap[4];

static int backcolor;
static const UINT8 *tile_address_prom;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Rolling Thunder has two palette PROMs (512x8 and 512x4) and two 2048x8
  lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( namcos86 )
{
	int i;
	int totcolors,totlookup;


	totcolors = Machine->drv->total_colors;
	totlookup = Machine->drv->color_table_len;

	for (i = 0;i < totcolors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[totcolors] >> 0) & 0x01;
		bit1 = (color_prom[totcolors] >> 1) & 0x01;
		bit2 = (color_prom[totcolors] >> 2) & 0x01;
		bit3 = (color_prom[totcolors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/* tiles lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *color_prom++;

	/* sprites lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *(color_prom++) + totcolors/2;

	/* color_prom now points to the beginning of the tile address decode PROM */

	tile_address_prom = color_prom;	/* we'll need this at run time */
}




/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void get_tile_info(int tile_index,int layer,data8_t *vram)
{
	int attr = vram[2*tile_index + 1];
	int tile_offs;
	if (layer & 2)
		tile_offs = ((tile_address_prom[((layer & 1) << 4) + (attr & 0x03)] & 0xe0) >> 5) * 0x100;
	else
		tile_offs = ((tile_address_prom[((layer & 1) << 4) + ((attr & 0x03) << 2)] & 0x0e) >> 1) * 0x100 + tilebank * 0x800;

	SET_TILE_INFO(
			(layer & 2) ? 1 : 0,
			vram[2*tile_index] + tile_offs,
			attr,
			0)
}

static void get_tile_info0(int tile_index) { get_tile_info(tile_index,0,&rthunder_videoram1[0x0000]); }
static void get_tile_info1(int tile_index) { get_tile_info(tile_index,1,&rthunder_videoram1[0x1000]); }
static void get_tile_info2(int tile_index) { get_tile_info(tile_index,2,&rthunder_videoram2[0x0000]); }
static void get_tile_info3(int tile_index) { get_tile_info(tile_index,3,&rthunder_videoram2[0x1000]); }


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( namcos86 )
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!tilemap[0] || !tilemap[1] || !tilemap[2] || !tilemap[3])
		return 1;

	tilemap_set_transparent_pen(tilemap[0],7);
	tilemap_set_transparent_pen(tilemap[1],7);
	tilemap_set_transparent_pen(tilemap[2],7);
	tilemap_set_transparent_pen(tilemap[3],7);

	spriteram = rthunder_spriteram + 0x1800;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( rthunder_videoram1_r )
{
	return rthunder_videoram1[offset];
}

WRITE_HANDLER( rthunder_videoram1_w )
{
	if (rthunder_videoram1[offset] != data)
	{
		rthunder_videoram1[offset] = data;
		tilemap_mark_tile_dirty(tilemap[offset/0x1000],(offset & 0xfff)/2);
	}
}

READ_HANDLER( rthunder_videoram2_r )
{
	return rthunder_videoram2[offset];
}

WRITE_HANDLER( rthunder_videoram2_w )
{
	if (rthunder_videoram2[offset] != data)
	{
		rthunder_videoram2[offset] = data;
		tilemap_mark_tile_dirty(tilemap[2+offset/0x1000],(offset & 0xfff)/2);
	}
}

WRITE_HANDLER( rthunder_tilebank_select_w )
{
	int bit = BIT(offset,10);
	if (tilebank != bit)
	{
		tilebank = bit;
		tilemap_mark_all_tiles_dirty(tilemap[0]);
		tilemap_mark_all_tiles_dirty(tilemap[1]);
	}
}

static void scroll_w(int layer,int offset,int data)
{
	switch (offset)
	{
		case 0:
			xscroll[layer] = (xscroll[layer]&0xff)|(data<<8);
			break;
		case 1:
			xscroll[layer] = (xscroll[layer]&0xff00)|data;
			break;
		case 2:
			yscroll[layer] = data;
			break;
	}
}

WRITE_HANDLER( rthunder_scroll0_w )
{
	scroll_w(0,offset,data);
}
WRITE_HANDLER( rthunder_scroll1_w )
{
	scroll_w(1,offset,data);
}
WRITE_HANDLER( rthunder_scroll2_w )
{
	scroll_w(2,offset,data);
}
WRITE_HANDLER( rthunder_scroll3_w )
{
	scroll_w(3,offset,data);
}

WRITE_HANDLER( rthunder_backcolor_w )
{
	backcolor = data;
}


static int copy_sprites;

READ_HANDLER( rthunder_spriteram_r )
{
	return rthunder_spriteram[offset];
}

WRITE_HANDLER( rthunder_spriteram_w )
{
	rthunder_spriteram[offset] = data;

	/* a write to this offset tells the sprite chip to buffer the sprite list */
	if (offset == 0x1ff2)
		copy_sprites = 1;
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int sprite_priority)
{
	const data8_t *source = &spriteram[0];
	const data8_t *finish = &spriteram[0x0800-16];	/* the last is NOT a sprite */

	int sprite_xoffs = spriteram[0x07f5] - 256 * (spriteram[0x07f4] & 1);
	int sprite_yoffs = spriteram[0x07f7];

	int bank_sprites = Machine->gfx[2]->total_elements / 8;

	while( source<finish )
	{
/*
	source[10] S-FT -BBB
	source[11] TTTT TTTT
	source[12] CCCC CCCX
	source[13] XXXX XXXX
	source[14] PPPT -S-F
	source[15] YYYY YYYY
*/
		int priority = (source[14] & 0xe0) >> 5;
		if (priority == sprite_priority)
		{
			static int gfx_offs[2][2] =
			{
				{ 0, 1 },
				{ 2, 3 }
			};
			int attr1 = source[10];
			int attr2 = source[14];
			int color = source[12];
			int sx = source[13] + (color & 0x01)*256;
			int sy = 240 - source[15];
			int flipx = (attr1 & 0x20) >> 5;
			int flipy = (attr2 & 0x01);
			int sizex = (attr1 & 0x80) >> 7;
			int sizey = (attr2 & 0x04) >> 2;
			int sprite = (source[11] & 0xff)*4;
			int sprite_bank = attr1 & 7;
			int x,y;

			sprite &= bank_sprites-1;
			sprite += sprite_bank * bank_sprites;
			if ((attr1 & 0x10) && !sizex) sprite += 1;
			if ((attr2 & 0x10) && !sizey) sprite += 2;
			color = color >> 1;

			sx += sprite_xoffs;
			sy -= sprite_yoffs;

			sy -= 16 * sizey;

			if (flip_screen)
			{
				sx = 496 - 16 * sizex - sx;
				sy = 240 - 16 * sizey - sy;
				flipx ^= 1;
				flipy ^= 1;
			}

			for (y = 0;y <= sizey;y++)
			{
				for (x = 0;x <= sizex;x++)
				{
					drawgfx( bitmap, Machine->gfx[2],
						sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
						color,
						flipx,flipy,
						-67 + ((sx + 16*x) & 0x1ff),
						1 + ((sy + 16*y) & 0xff),
						cliprect,TRANSPARENCY_PEN,0xf);
				}
			}
		}

		source+=16;
	}
}


static void set_scroll(int layer)
{
	int xdisp[4] = { 20, 18, 21, 19 };
	int scrollx,scrolly;

	scrollx = xscroll[layer] + xdisp[layer];
	scrolly = yscroll[layer] + 9;
	if (flip_screen)
	{
		scrollx = -scrollx;
		scrolly = -scrolly;
	}
	tilemap_set_scrollx(tilemap[layer], 0, scrollx);
	tilemap_set_scrolly(tilemap[layer], 0, scrolly);
}


VIDEO_UPDATE( namcos86 )
{
	int layer;

	/* flip screen is embedded in the sprite control registers */
	/* can't use flip_screen_set() because the visible area is asymmetrical */
	flip_screen = spriteram[0x07f6] & 1;
	tilemap_set_flip(ALL_TILEMAPS,flip_screen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	set_scroll(0);
	set_scroll(1);
	set_scroll(2);
	set_scroll(3);

	fillbitmap(bitmap,Machine->gfx[0]->colortable[8*backcolor+7],cliprect);

	for (layer = 0;layer < 8;layer++)
	{
		int i;

		for (i = 3;i >= 0;i--)
		{
			if (((xscroll[i] & 0x0e00) >> 9) == layer)
				tilemap_draw(bitmap,cliprect,tilemap[i],0,0);
		}

		draw_sprites(bitmap,cliprect,layer);
	}
}


VIDEO_EOF( namcos86 )
{
	if (copy_sprites)
	{
		int i,j;

		for (i = 0;i < 0x800;i += 16)
		{
			for (j = 10;j < 16;j++)
				spriteram[i+j] = spriteram[i+j - 6];
		}

		copy_sprites = 0;
	}
}
