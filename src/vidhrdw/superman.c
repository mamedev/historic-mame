/***************************************************************************

  vidhrdw/superman.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

size_t supes_videoram_size;
size_t supes_attribram_size;

data16_t *supes_videoram;
data16_t *supes_attribram;

static int tilemask;

int superman_vh_start (void)
{
	tilemask = 0x3fff;
	return 0;
}

int ballbros_vh_start (void)
{
	tilemask = 0x0fff;
	return 0;
}

void superman_vh_stop (void)
{
}

/*************************************
 *
 *		Foreground RAM
 *
 *************************************/

WRITE16_HANDLER( supes_attribram_w )
{
//   data16_t oldword = supes_attribram[offset];
   COMBINE_DATA (&supes_attribram[offset]);

//   if (oldword != supes_attribram[offset])
//   {
//   }
}

READ16_HANDLER( supes_attribram_r )
{
   return supes_attribram[offset];
}



/*************************************
 *
 *		Background RAM
 *
 *************************************/

WRITE16_HANDLER( supes_videoram_w )
{
//   data16_t oldword = supes_videoram[offset];
   COMBINE_DATA (&supes_videoram[offset]);

//   if (oldword != supes_videoram[offset])
//   {
//   }
}

READ16_HANDLER( supes_videoram_r )
{
   return supes_videoram[offset];
}


void superman_update_palette(int bankbase)
{
	unsigned short palette_map[32]; /* range of color table is 0-31 */
	int i;

	memset (palette_map, 0, sizeof (palette_map));

	/* Find colors used in the background tile plane */
	for (i = 0; i < 0x400/2; i += 0x40/2)
	{
		int i2;

		for (i2 = i; i2 < (i + 0x40/2); i2 += 2/2)
		{
			int tile;
			int color;

			color = 0;

			tile = supes_videoram[0x800/2 + bankbase + i2] & tilemask;
			if (tile)
				color = supes_videoram[0xc00/2 + bankbase + i2] >> 11;

			palette_map[color] |= Machine->gfx[0]->pen_usage[tile];

		}
	}

	/* Find colors used in the sprite plane */
	for (i = 0x3fe/2; i >= 0; i -= 2/2)
	{
		int tile;
		int color;

		color = 0;

		tile = supes_videoram[i + bankbase] & tilemask;
		if (tile)
			color = supes_videoram[0x400/2 + bankbase + i] >> 11;

		palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
	}


	/* Now tell the palette system about those colors */

	palette_init_used_colors();

	for (i = 0;i < 32;i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_VISIBLE;
		}
	}

	/* background */
	palette_used_colors[0x1f0] = PALETTE_COLOR_VISIBLE;

	palette_recalc();
}

void superman_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;
	int bankbase;
	int attribfix;
	int cocktail;

	/* set bank base */
	if (supes_attribram[0x602 / 2] & 0x40)
		bankbase = 0x2000 / 2;
	else
		bankbase = 0x0000 / 2;

	/* attribute fix */
	attribfix = ( (supes_attribram[0x604 / 2] & 0xff) << 8 ) |
				( (supes_attribram[0x606 / 2] & 0xff) << 16 );

	/* cocktail mode */
	cocktail = supes_attribram[0x600 / 2] & 0x40;

	/* update palette */
	superman_update_palette ( bankbase );

	fillbitmap(bitmap,Machine->pens[0x1f0],&Machine->visible_area);

	/* Refresh the background tile plane */
	for (i = 0; i < 0x400/2; i += 0x40/2)
	{
		UINT32 x1, y1;
		int i2;

		x1 = supes_attribram[0x408/2 + (i>>1)] | (attribfix & 0x100);
		y1 = supes_attribram[0x400/2 + (i>>1)];

		attribfix >>= 1;

		for (i2 = i; i2 < (i + 0x40/2); i2 += 2/2)
		{
			int tile;

			tile = supes_videoram[0x800/2 + bankbase + i2] & tilemask;
			if (tile)
			{
				int x, y;

				x = ((x1 + ((i2 & 0x01) << 4)) + 16) & 0x1ff;
				if ( !cocktail )
					y = (265 - (y1 - ((i2 & 0x1e) << 3))    ) & 0xff;
				else
					y = (      (y1 - ((i2 & 0x1e) << 3)) - 7) & 0xff;

//				if ((x > 0) && (y > 0) && (x < 388) && (y < 272))
				{
					UINT32 flipx = supes_videoram[0x800/2 + bankbase + i2] & 0x8000;
					UINT32 flipy = supes_videoram[0x800/2 + bankbase + i2] & 0x4000;
					UINT32 color = supes_videoram[0xc00/2 + bankbase + i2] >> 11;

					if (cocktail)
					{
						flipx ^= 0x8000;
						flipy ^= 0x4000;
					}

					/* Some tiles are transparent, e.g. the gate, so we use TRANSPARENCY_PEN */
					drawgfx(bitmap,Machine->gfx[0],
						tile,
						color,
						flipx,flipy,
						x,y,
						&Machine->visible_area,
						TRANSPARENCY_PEN,0);
				}
			}
		}
	}

	/* Refresh the sprite plane */
	for (i = 0x3fe/2; i >= 0; i -= 2/2)
	{
		int sprite;

		sprite = supes_videoram[i + bankbase] & tilemask;

		if (sprite)
		{
			int x, y;

			x = (supes_videoram [0x400/2 + bankbase + i] + 16) & 0x1ff;

			if (!cocktail)
				y = (250 - supes_attribram[i]) & 0xff;
			else
				y = (10  + supes_attribram[i]) & 0xff;

//			if ((x > 0) && (y > 0) && (x < 388) && (y < 272))
			{
				int flipx = supes_videoram[          bankbase + i] & 0x8000;
				int flipy = supes_videoram[          bankbase + i] & 0x4000;
				int color = supes_videoram[0x400/2 + bankbase + i] >> 11;

				if (cocktail){
					flipx ^= 0x8000;
					flipy ^= 0x4000;
				}

				drawgfx(bitmap,Machine->gfx[0],
					sprite,
					color,
					flipx,flipy,
					x,y,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0);
			}
		}
	}
}
