/***************************************************************************

   Funky Jet Video emulation - Bryan McPhail, mish@tendril.co.uk

   Cut & paste from Super Burgertime for now! :)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *funkyjet_pf2_data16;
data16_t *funkyjet_pf1_data16;
data16_t *funkyjet_pf1_row16;
size_t funkyjet_pf2_data16_size;
size_t funkyjet_pf1_data16_size;
static unsigned char *funkyjet_pf1_dirty;
static unsigned char *funkyjet_pf2_dirty;
static struct osd_bitmap *funkyjet_pf1_bitmap;
static struct osd_bitmap *funkyjet_pf2_bitmap;

static data16_t funkyjet_control16_0[8];
static int offsetx[4],offsety[4];

/******************************************************************************/

static void funkyjet_update_palette(void)
{
	int offs,color,code,i,pal_base;
	int colmask[16];
    unsigned int *pen_usage;

	palette_init_used_colors();

	pen_usage=Machine->gfx[1]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0; color < 16; color++)
		colmask[color] = 0;
	for (offs = 0; offs < (funkyjet_pf2_data16_size >> 1); offs++)
	{
		code = funkyjet_pf2_data16[offs];
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= pen_usage[code];
	}

	for (color = 0; color < 16; color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	pen_usage=Machine->gfx[0]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0; color < 16; color++)
		colmask[color] = 0;
	for (offs = 0; offs < funkyjet_pf1_data16_size; offs++)
	{
		code = funkyjet_pf1_data16[offs];
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Sprites */
	pen_usage=Machine->gfx[2]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0; offs < (0x800 >> 1); offs += 4)
	{
		int x,y,sprite,multi;

		sprite = spriteram16[offs+1] & 0x1fff;
		if (!sprite) continue;

		y = spriteram16[offs];
		x = spriteram16[offs+2];
		color = (x >> 9) & 0xf;

		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		sprite &= ~multi;

		while (multi >= 0)
		{
			colmask[color] |= pen_usage[(sprite + multi)&0x1fff];
			multi--;
		}
	}

	for (color = 0; color < 16; color++)
	{
		for (i = 1; i < 16; i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		memset(funkyjet_pf2_dirty, 1, funkyjet_pf2_data16_size >> 1);
		memset(funkyjet_pf1_dirty, 1, funkyjet_pf1_data16_size >> 1);
	}
}

static void funkyjet_drawsprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0; offs < (0x800 >> 1); offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash;

		sprite = spriteram16[offs+1] & 0x1fff;
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >> 9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y - 16 * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static void funkyjet_pf1_update(void)
{
	int offs,mx,my,color,tile;

	mx = -1;
	my = 0;
	for (offs = 0; offs < (0x2000 >> 1); offs++)
	{
		mx++;
		if (mx == 64)
		{
			mx = 0;
			my++;
		}

		if (funkyjet_pf1_dirty[offs])
		{
			funkyjet_pf1_dirty[offs] = 0;
			tile = funkyjet_pf1_data16[offs];
			color = (tile & 0xf000) >> 12;

			drawgfx(funkyjet_pf1_bitmap,Machine->uifont, //gfx[0],
					tile & 0x0fff,
					color,
					0,0,
					8*mx,8*my,
					0,TRANSPARENCY_NONE,0);
		}
	}
}

static void funkyjet_pf2_update(void)
{
	int offs, mx, my, color, tile, quarter;

	quarter = funkyjet_pf2_data16_size / 4;

	for (offs = 0, mx = -1, my = 0; offs < (funkyjet_pf2_data16_size >> 1); offs++)
	{
		if (offs % quarter == 0)
		{
			mx = -1;
			my = 0;
		}

        mx++;
		if (mx == 32)
		{
			mx = 0;
			my++;
		}

		if (funkyjet_pf2_dirty[offs])
		{
			funkyjet_pf2_dirty[offs] = 0;
			tile = funkyjet_pf2_data16[offs];
			color = (tile & 0xf000) >> 12;

			drawgfx(funkyjet_pf2_bitmap,Machine->gfx[1],
					tile & 0x0fff,
					color,
					0,0,
					16*mx + offsetx[quarter],16*my + offsety[quarter],
					0,TRANSPARENCY_NONE,0);
		}
	}
}

/******************************************************************************/

void funkyjet_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int scrollx,scrolly;

	funkyjet_update_palette();
	funkyjet_pf1_update();
	funkyjet_pf2_update();

	/* Background */
	scrollx = -funkyjet_control16_0[3];
	scrolly = -funkyjet_control16_0[4];
 	copyscrollbitmap(bitmap,funkyjet_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Sprites */
//	funkyjet_drawsprites(bitmap);

	/* Foreground */
	scrollx = -funkyjet_control16_0[1];
	scrolly = -funkyjet_control16_0[2];

	/* 'Fake' rowscroll, used only in the end game message */
	if (funkyjet_control16_0[6] == 0xc0)
		scrollx = -funkyjet_pf1_row16[4];
	copyscrollbitmap(bitmap,funkyjet_pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	funkyjet_drawsprites(bitmap);
}

/******************************************************************************/

WRITE16_HANDLER( funkyjet_pf2_data16_w )
{
	data16_t oldword = funkyjet_pf2_data16[offset];
	COMBINE_DATA(&funkyjet_pf2_data16[offset]);

	if (oldword != funkyjet_pf2_data16[offset])
	{
		funkyjet_pf2_dirty[offset] = 1;
	}
}

WRITE16_HANDLER( funkyjet_pf1_data16_w )
{
	data16_t oldword = funkyjet_pf1_data16[offset];
	COMBINE_DATA(&funkyjet_pf1_data16[offset]);

	if (oldword != funkyjet_pf1_data16[offset])
	{
		funkyjet_pf1_dirty[offset] = 1;
	}
}

READ16_HANDLER( funkyjet_pf1_data16_r )
{
	return funkyjet_pf1_data16[offset];
}

READ16_HANDLER( funkyjet_pf2_data16_r )
{
	return funkyjet_pf2_data16[offset];
}

WRITE16_HANDLER( funkyjet_control16_0_w )
{
	COMBINE_DATA(&funkyjet_control16_0[offset]);
}

/******************************************************************************/

void funkyjet_vh_stop (void)
{
	bitmap_free(funkyjet_pf2_bitmap);
	bitmap_free(funkyjet_pf1_bitmap);
	free(funkyjet_pf1_dirty);
	free(funkyjet_pf2_dirty);
}

int funkyjet_vh_start(void)
{
	/* Allocate bitmaps */
	if ((funkyjet_pf1_bitmap = bitmap_alloc(512,512)) == 0) {
		funkyjet_vh_stop ();
		return 1;
	}

	if ((funkyjet_pf2_bitmap = bitmap_alloc(1024,512)) == 0) {
		funkyjet_vh_stop ();
		return 1;
	}

	funkyjet_pf1_dirty = malloc(funkyjet_pf1_data16_size >> 1);
	funkyjet_pf2_dirty = malloc(funkyjet_pf2_data16_size >> 1);
	memset(funkyjet_pf2_dirty,1,funkyjet_pf2_data16_size >> 1);
	memset(funkyjet_pf1_dirty,1,funkyjet_pf1_data16_size >> 1);

	offsetx[0] = 0;
	offsetx[1] = 0;
	offsetx[2] = 512;
	offsetx[3] = 512;
	offsety[0] = 0;
	offsety[1] = 256;
	offsety[2] = 0;
	offsety[3] = 256;

	return 0;
}
