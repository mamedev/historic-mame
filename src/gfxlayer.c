/*********************************************************************

  gfxlayer.c

  Functions which handle the generic graphics layer system.

*********************************************************************/

#include "driver.h"


/* some globals for the dirty block handling */
short layer_width,layer_height;
short layer_dirty_size;	/* rounded length of the dirty array */
short layer_dirty_shift;	/* pixel of coordinates (x,y) is in the dirty block */
							/* dirty[((y >> 3) << rbshift) + (x >> 3)] */
short layer_dirty_minx,layer_dirty_miny;
				/* coordinates of the top-left corner of the top-left block */
short layer_dirty_maxx,layer_dirty_maxy;
				/* coordinates of the top-left corner of the bottom-right block */


static int readbit(const unsigned char *src,int bitnum)
{
	return (src[bitnum / 8] >> (7 - bitnum % 8)) & 1;
}

void decodetile(struct GfxTileBank *gfxtilebank,int num,const unsigned char *src,const struct GfxTileLayout *gl,const struct MachineLayer *ml)
{
	int plane,x,y;
	unsigned char *dp;



	for (plane = 0;plane < gl->planes;plane++)
	{
		int offs;


		offs = num * gl->charincrement + gl->planeoffset[plane];

		dp = gfxtilebank->tiles[num].pen;

		for (y = 0;y < 8;y++)
		{
			for (x = 0;x < 8;x++)
			{
				int xoffs,yoffs;


				if (plane == 0) *dp = 0;
				else *dp <<= 1;

				xoffs = x;
				yoffs = y;

				if (Machine->orientation & ORIENTATION_FLIP_X)
					xoffs = 7 - xoffs;

				if (Machine->orientation & ORIENTATION_FLIP_Y)
					yoffs = 7 - yoffs;

				if (Machine->orientation & ORIENTATION_SWAP_XY)
				{
					int temp;


					temp = xoffs;
					xoffs = yoffs;
					yoffs = temp;
				}

				*dp += readbit(src,offs + gl->yoffset[yoffs] + gl->xoffset[xoffs]);

				dp++;
			}
		}
	}


	/* fill the pen_usage array with info on the used pens */
	/* and initialize line masks */
	gfxtilebank->tiles[num].pens_used = 0;
	dp = gfxtilebank->tiles[num].pen;

	for (y = 0;y < 8;y++)
	{
		gfxtilebank->tiles[num].mask[y] = 0;

		for (x = 0;x < 8;x++)
		{
			gfxtilebank->tiles[num].pens_used |= 1 << *dp;

			if ((gfxtilebank->transparency_mask & (1 << *dp)) == 0)
				gfxtilebank->tiles[num].mask[y] |= 1 << x;

			dp++;
		}
	}
}

struct GfxTileBank *decodetiles(const struct GfxTileDecodeInfo *di,const struct MachineLayer *ml)
{
	const unsigned char *src;
	const struct GfxTileLayout *gl;
	int c;
	struct GfxTileBank *gfxtilebank;


	gl = di->gfxtilelayout;
	src = Machine->memory_region[di->memory_region]	+ di->start;

	if ((gfxtilebank = malloc(sizeof(struct GfxTileBank))) == 0)
		return 0;

	if ((gfxtilebank->tiles = malloc(gl->total * sizeof(struct GfxTile))) == 0)
	{
		free(gfxtilebank);
		return 0;
	}

	gfxtilebank->total_elements = gl->total;
	gfxtilebank->color_granularity = 1 << gl->planes;
	gfxtilebank->colortable = &Machine->colortable[di->color_codes_start];
	gfxtilebank->total_colors = di->total_color_codes;
	gfxtilebank->transparency_mask = di->transparency_mask;

	for (c = 0;c < gl->total;c++)
		decodetile(gfxtilebank,c,src,gl,ml);

	return gfxtilebank;
}

void freetiles(struct GfxTileBank *gfxtilebank)
{
	if (gfxtilebank)
	{
		free(gfxtilebank->tiles);
		free(gfxtilebank);
	}
}



/* minx and miny are the coordinates (in pixels) of the top left corner of the */
/* 8x8 block to mark. */
void layer_mark_block_dirty(struct GfxLayer *layer,int minx,int miny,int action)
{
	int block;


	if (minx < layer_dirty_minx)
	{
		if (minx <= layer_dirty_minx - 8) return;
		minx = layer_dirty_minx;
	}
	else if (minx > layer_dirty_maxx)
	{
		if (minx >= layer_dirty_maxx + 8) return;
		minx = layer_dirty_maxx;
	}
	if (miny < layer_dirty_miny)
	{
		if (miny <= layer_dirty_miny - 8) return;
		miny = layer_dirty_miny;
	}
	else if (miny > layer_dirty_maxy)
	{
		if (miny >= layer_dirty_maxy + 8) return;
		miny = layer_dirty_maxy;
	}

	block = ((miny >> 3) << layer_dirty_shift) + (minx >> 3);

	if ((minx & 0x07) == 0)
	{
		if ((miny & 0x07) == 0)
		{
			/* totally aligned case */

			switch (action)
			{
				case MARK_ALL_DIRTY:
					layer->dirty[block] = TILE_DIRTY;
					layer->dirtyline[miny >> 3] = 1;
					break;
				case MARK_ALL_NOT_CLEAN:
					layer->dirty[block] = TILE_NOT_CLEAN;
					break;
				case MARK_DIRTY_IF_NOT_CLEAN:
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[miny >> 3] = 1;
					}
					break;
			}
		}
		else
		{
			/* horizontally aligned case */

			switch (action)
			{
				case MARK_ALL_DIRTY:
					layer->dirty[block] = TILE_DIRTY;
					block += 1 << layer_dirty_shift;
					layer->dirty[block] = TILE_DIRTY;
					layer->dirtyline[(miny >> 3)] = 1;
					layer->dirtyline[(miny >> 3) + 1] = 1;
					break;
				case MARK_ALL_NOT_CLEAN:
					layer->dirty[block] = TILE_NOT_CLEAN;
					block += 1 << layer_dirty_shift;
					layer->dirty[block] = TILE_NOT_CLEAN;
					break;
				case MARK_DIRTY_IF_NOT_CLEAN:
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[(miny >> 3)] = 1;
					}
					block += 1 << layer_dirty_shift;
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[(miny >> 3) + 1] = 1;
					}
					break;
			}
		}
	}
	else
	{
		if ((miny & 0x07) == 0)
		{
			/* vertically aligned case */

			switch (action)
			{
				case MARK_ALL_DIRTY:
					layer->dirty[block] = TILE_DIRTY;
					layer->dirty[block + 1] = TILE_DIRTY;
					layer->dirtyline[miny >> 3] = 1;
					break;
				case MARK_ALL_NOT_CLEAN:
					layer->dirty[block] = TILE_NOT_CLEAN;
					layer->dirty[block + 1] = TILE_NOT_CLEAN;
					break;
				case MARK_DIRTY_IF_NOT_CLEAN:
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[miny >> 3] = 1;
					}
					if (layer->dirty[block + 1] == TILE_NOT_CLEAN)
					{
						layer->dirty[block + 1] = TILE_DIRTY;
						layer->dirtyline[miny >> 3] = 1;
					}
					break;
			}
		}
		else
		{
			/* totally unaligned case */

			switch (action)
			{
				case MARK_ALL_DIRTY:
					layer->dirty[block] = TILE_DIRTY;
					layer->dirty[block + 1] = TILE_DIRTY;
					block += 1 << layer_dirty_shift;
					layer->dirty[block] = TILE_DIRTY;
					layer->dirty[block + 1] = TILE_DIRTY;
					layer->dirtyline[miny >> 3] = 1;
					layer->dirtyline[(miny >> 3) + 1] = 1;
					break;
				case MARK_ALL_NOT_CLEAN:
					layer->dirty[block] = TILE_NOT_CLEAN;
					layer->dirty[block + 1] = TILE_NOT_CLEAN;
					block += 1 << layer_dirty_shift;
					layer->dirty[block] = TILE_NOT_CLEAN;
					layer->dirty[block + 1] = TILE_NOT_CLEAN;
					break;
				case MARK_DIRTY_IF_NOT_CLEAN:
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[miny >> 3] = 1;
					}
					if (layer->dirty[block + 1] == TILE_NOT_CLEAN)
					{
						layer->dirty[block + 1] = TILE_DIRTY;
						layer->dirtyline[miny >> 3] = 1;
					}
					block += 1 << layer_dirty_shift;
					if (layer->dirty[block] == TILE_NOT_CLEAN)
					{
						layer->dirty[block] = TILE_DIRTY;
						layer->dirtyline[(miny >> 3) + 1] = 1;
					}
					if (layer->dirty[block + 1] == TILE_NOT_CLEAN)
					{
						layer->dirty[block + 1] = TILE_DIRTY;
						layer->dirtyline[(miny >> 3) + 1] = 1;
					}
					break;
			}
		}
	}
}

/* companion to mark_block_dirty() which marks a number of horizontally consecutive */
/* blocks dirty. This function is hardwired to MARK_ALL_DIRTY - no other actions available. */
static void layer_mark_blocks_dirty(struct GfxLayer *layer,int minx,int miny,int hcount)
{
	int block;


	if (minx < layer_dirty_minx) minx = layer_dirty_minx;
	else if (minx +  8 * (hcount - 1) > layer_dirty_maxx) minx = layer_dirty_maxx - 8 * (hcount - 1);
	if (miny < layer_dirty_miny) miny = layer_dirty_miny;
	else if (miny > layer_dirty_maxy) miny = layer_dirty_maxy;

	block = ((miny >> 3) << layer_dirty_shift) + (minx >> 3);

	if ((minx & 0x07) == 0)
	{
		if ((miny & 0x07) == 0)
		{
			/* totally aligned case */
			layer->dirtyline[miny >> 3] = 1;
			do
			{
				layer->dirty[block] = TILE_DIRTY;
				block++;
			} while (--hcount > 0);
		}
		else
		{
			/* horizontally aligned case */
			layer->dirtyline[(miny >> 3)] = 1;
			layer->dirtyline[(miny >> 3) + 1] = 1;
			do
			{
				layer->dirty[block] = TILE_DIRTY;
				layer->dirty[block + (1 << layer_dirty_shift)] = TILE_DIRTY;
				block++;
			} while (--hcount > 0);
		}
	}
	else
	{
		if ((miny & 0x07) == 0)
		{
			/* vertically aligned case */
			layer->dirtyline[miny >> 3] = 1;
			do
			{
				layer->dirty[block] = TILE_DIRTY;
				block++;
			} while (--hcount >= 0);	/* mark one block more than requested */
		}
		else
		{
			/* totally unaligned case */
			layer->dirtyline[miny >> 3] = 1;
			layer->dirtyline[(miny >> 3) + 1] = 1;
			do
			{
				layer->dirty[block] = TILE_DIRTY;
				layer->dirty[block + (1 << layer_dirty_shift)] = TILE_DIRTY;
				block++;
			} while (--hcount >= 0);	/* mark one block more than requested */
		}
	}
}

void layer_mark_all_dirty(struct GfxLayer *layer,int action)
{
	int block;
	int x,y;


	if (layer == 0) return;

	switch (action)
	{
		case MARK_ALL_CLEAN:
			for (y = layer_dirty_miny / 8;y <= layer_dirty_maxy / 8;y++)
			{
				block = y << layer_dirty_shift;

				for (x = layer_dirty_minx / 8;x <= layer_dirty_maxx / 8;x++)
					layer->dirty[block + x] = TILE_CLEAN;

				layer->dirtyline[y] = 0;
			}
			break;
		case MARK_ALL_DIRTY:
			for (y = layer_dirty_miny / 8;y <= layer_dirty_maxy / 8;y++)
			{
				block = y << layer_dirty_shift;

				for (x = layer_dirty_minx / 8;x <= layer_dirty_maxx / 8;x++)
					layer->dirty[block + x] = TILE_DIRTY;

				layer->dirtyline[y] = 1;
			}
			break;
		case MARK_ALL_NOT_CLEAN:
			for (y = layer_dirty_miny / 8;y <= layer_dirty_maxy / 8;y++)
			{
				block = y << layer_dirty_shift;

				for (x = layer_dirty_minx / 8;x <= layer_dirty_maxx / 8;x++)
					layer->dirty[block + x] = TILE_NOT_CLEAN;

				layer->dirtyline[y] = 0;
			}
			break;
		case MARK_DIRTY_IF_NOT_CLEAN:
			for (y = layer_dirty_miny / 8;y <= layer_dirty_maxy / 8;y++)
			{
				block = y << layer_dirty_shift;

				for (x = layer_dirty_minx / 8;x <= layer_dirty_maxx / 8;x++)
				{
					if (layer->dirty[block + x] == TILE_NOT_CLEAN)
					{
						layer->dirty[block + x] = TILE_DIRTY;
						layer->dirtyline[y] = 1;
					}
				}
			}
			break;
		case MARK_CLEAN_IF_DIRTY:
			for (y = layer_dirty_miny / 8;y <= layer_dirty_maxy / 8;y++)
			{
				block = y << layer_dirty_shift;

				for (x = layer_dirty_minx / 8;x <= layer_dirty_maxx / 8;x++)
				{
					if (layer->dirty[block + x] == TILE_DIRTY)
						layer->dirty[block + x] = TILE_CLEAN;
				}

				layer->dirtyline[y] = 0;
			}
			break;
	}
}


/* starts from the 8x8 block whose top left corner is at minx,miny, and goes to the */
/* right, counting how many consecutive ones are dirty. Returns 0 if it is not dirty. */
int layer_count_blocks_dirty(struct GfxLayer *layer,int minx,int miny)
{
	int block,maxblock;
	int count;


	if (minx < layer_dirty_minx) minx = layer_dirty_minx;
	else if (minx > layer_dirty_maxx) minx = layer_dirty_maxx;
	if (miny < layer_dirty_miny) miny = layer_dirty_miny;
	else if (miny > layer_dirty_maxy) miny = layer_dirty_maxy;

	block = ((miny >> 3) << layer_dirty_shift) + (minx >> 3);
	maxblock = ((miny >> 3) << layer_dirty_shift) + (layer_dirty_maxx >> 3);
	count = 0;

	if ((minx & 0x07) == 0)
	{
		if ((miny & 0x07) == 0)
		{
			/* totally aligned case */
			while (layer->dirty[block] == TILE_DIRTY)
			{
				count++;
				if (block == maxblock) return count;
				block++;
			}
			return count;
		}
		else
		{
			/* horizontally aligned case */
			while (layer->dirty[block] == TILE_DIRTY ||
					layer->dirty[block + (1 << layer_dirty_shift)] == TILE_DIRTY)
			{
				count++;
				if (block == maxblock) return count;
				block++;
			}
			return count;
		}
	}
	else
	{
		if ((miny & 0x07) == 0)
		{
			/* vertically aligned case */
loop1:
			while (layer->dirty[block] == TILE_DIRTY)
			{
				count++;
				if (block == maxblock) return count;
				block++;
			}
			if (block == maxblock) return count;
			block++;
			if (layer->dirty[block] == TILE_DIRTY)
			{
				count++;
				goto loop1;
			}

			return count;
		}
		else
		{
			/* totally unaligned case */
			while (layer->dirty[block] == TILE_DIRTY ||
					layer->dirty[block + (1 << layer_dirty_shift)] == TILE_DIRTY)
			{
				count++;
				if (block == maxblock) return count;
				block++;
			}
			if (block == maxblock) return count;
			block++;
			if (layer->dirty[block] == TILE_DIRTY ||
					layer->dirty[block + (1 << layer_dirty_shift)] == TILE_DIRTY)
			{
				count++;
				goto loop1;
			}

			return count;
		}
	}

	return 0;
}

int layer_is_line_dirty(struct GfxLayer *layer,int miny)
{
	int block;


	if (miny < layer_dirty_miny) miny = layer_dirty_miny;
	else if (miny > layer_dirty_maxy) miny = layer_dirty_maxy;

	block = miny >> 3;

	if ((miny & 0x07) == 0)
	{
		/* aligned case */
		if (layer->dirtyline[block]) return 1;
	}
	else
	{
		/* unaligned case */
		if (layer->dirtyline[block] || layer->dirtyline[block+1]) return 1;
	}

	return 0;
}


/* check if the given tile is totally covered by tiles in the above layers */
INLINE int is_tile_obscured(int layer_num,int minx,int miny)
{
	int i;


	for (i = layer_num - 1;i >= 0;i--)
	{
		int x,y,tile;
		struct GfxLayer *layer = Machine->layer[i];
		int bminy;
		int lminx,lminy;
		int res;


		if (layer->tilemap.flip & TILE_FLIPX)
			minx = layer->tilemap.width - 8 - minx;
		if (layer->tilemap.flip & TILE_FLIPY)
			miny = layer->tilemap.height - 8 - miny;

		minx -= layer->tilemap.scrollx;
		miny -= layer->tilemap.scrolly;

		res = 1;

		do
		{
			lminx = minx;
			if (lminx > -8 && lminx < layer->tilemap.width)
			{
				if (lminx < 0) lminx = 0;
				bminy = miny;
				do
				{
					lminy = bminy;
					if (lminy > -8 && lminy < layer->tilemap.height)
					{
						if (lminy < 0) lminy = 0;
						x = lminx / 8;
						y = lminy / 8;

						tile = y * (layer->tilemap.width / 8) + x;
						if (TILE_TRANSPARENCY(layer->tilemap.tiles[tile]) != TILE_TRANSPARENCY_OPAQUE)
							res = 0;
						if (lminx & 0x07)
						{
							if (TILE_TRANSPARENCY(layer->tilemap.tiles[tile + 1]) != TILE_TRANSPARENCY_OPAQUE)
								res = 0;
						}

						if (lminy & 0x07)
						{
							tile += layer->tilemap.width / 8;
							if (TILE_TRANSPARENCY(layer->tilemap.tiles[tile]) != TILE_TRANSPARENCY_OPAQUE)
								res = 0;

							if (lminx & 0x07)
							{
								if (TILE_TRANSPARENCY(layer->tilemap.tiles[tile + 1]) != TILE_TRANSPARENCY_OPAQUE)
									res = 0;
							}
						}
					}
					bminy += layer->tilemap.height;
				} while (bminy < layer->tilemap.height);
			}
			minx += layer->tilemap.width;
		} while (minx < layer->tilemap.width);

		if (res == 1) return 1;
	}

	return 0;
}



static void draw_tilemap_core8(int layer_num,struct osd_bitmap *bitmap,int x,int y)
{
#define DATA_SIZE unsigned char
	unsigned long *tiles;
	int dx,dy;
	DATA_SIZE *bm,*ebm,*eebm;
	int flipx,flipy;
	struct GfxLayer *layer = Machine->layer[layer_num];
	int lines_to_copy,cols_to_copy;
	int minx,maxx,miny,maxy;


	if (layer == 0) return;

	flipx = layer->tilemap.flip & TILE_FLIPX;
	flipy = layer->tilemap.flip & TILE_FLIPY;

	tiles = layer->tilemap.tiles;

	if (x >= 0)
	{
		minx = x;
		maxx = layer_dirty_maxx + 8;
		if (minx >= maxx) return;
		while (minx <= layer_dirty_minx - 8)
		{
			minx += 8;
			tiles++;
		}
		if ((maxx - minx) & 0x07)
			maxx += 8 - ((maxx - minx) & 0x07);
	}
	else
	{
		minx = layer_dirty_minx;
		maxx = x + layer->tilemap.width;
		if (minx >= maxx) return;
		if ((maxx - minx) & 0x07)
			minx -= 8 - ((maxx - minx) & 0x07);
		tiles += (layer->tilemap.width - (maxx - minx)) / 8;

		while (maxx >= layer_dirty_maxx + 16)
			maxx -= 8;
	}

	if (y >= 0)
	{
		miny = y;
		maxy = layer_dirty_maxy + 8;
		if (miny >= maxy) return;
		while (miny <= layer_dirty_miny - 8)
		{
			miny += 8;
			tiles += (layer->tilemap.width / 8);
		}
		if ((maxy - miny) & 0x07)
			maxy += 8 - ((maxy - miny) & 0x07);
	}
	else
	{
		miny = layer_dirty_miny;
		maxy = y + layer->tilemap.height;
		if (miny >= maxy) return;
		if ((maxy - miny) & 0x07)
			miny -= 8 - ((maxy - miny) & 0x07);
		tiles += (layer->tilemap.width / 8) * (layer->tilemap.height - (maxy - miny)) / 8;

		while (maxy >= layer_dirty_maxy + 16)
			maxy -= 8;
	}


	cols_to_copy = (maxx - minx) / 8;
	lines_to_copy = maxy - miny;

	dy = (DATA_SIZE *)bitmap->line[1] - (DATA_SIZE *)bitmap->line[0];
	bm = (DATA_SIZE *)bitmap->line[0] + dy * miny + minx;
	if (flipy)
	{
		bm += (layer->tilemap.height - 1) * dy;
		miny += layer->tilemap.height - 8;
		dy = -dy;
	}
	dx = 8;
	if (flipx)
	{
		bm += (layer->tilemap.width / 8 - 1) * dx;
		dx = -dx;
	}

	ebm = bm + cols_to_copy * dx;
	eebm = ebm + lines_to_copy * dy;

	while (ebm != eebm)
	{
		if (layer_is_line_dirty(layer,miny))
		{
			int countdirty;


			if (dy > 0)
				minx = (bm - ((DATA_SIZE *)bitmap->line[0]-8-8*dy)) % dy - 8;
			else
				minx = (bm - ((DATA_SIZE *)bitmap->line[0]-8+8*dy)) % -dy - 8;

			countdirty = 0;

			while (bm != ebm)
			{
				if (countdirty == 0)
				{
					countdirty = layer_count_blocks_dirty(layer,minx,miny);
					if (countdirty > 0)
					{
						if (layer_num == 0)
						{
							/* mark screen bitmap dirty */
							layer_mark_blocks_dirty(Machine->dirtylayer,minx,miny,countdirty);
						}
						else
						{
							/* mark tiles in the layer above this one as dirty */
							layer_mark_blocks_dirty(Machine->layer[layer_num - 1],minx,miny,countdirty);
						}
					}
				}

				if (countdirty > 0)
				{
					long tile;


					countdirty--;

					tile = *tiles;

					if (TILE_TRANSPARENCY(tile) != TILE_TRANSPARENCY_TRANSPARENT &&
							is_tile_obscured(layer_num,minx,miny) == 0)
					{
						const unsigned short *paldata;
						const unsigned char *sd;
						const struct GfxTileBank *gfxtilebank;
						int i;


						/* mark all tiles below this one as not clean */
						for (i = layer_num + 1;i < MAX_LAYERS && Machine->layer[i];i++)
							layer_mark_block_dirty(Machine->layer[i],minx,miny,MARK_ALL_NOT_CLEAN);

						gfxtilebank = layer->tilemap.gfxtilebank[TILE_BANK(tile)];

						paldata = &gfxtilebank->colortable[gfxtilebank->color_granularity * TILE_COLOR(tile)];
						sd = gfxtilebank->tiles[TILE_CODE(tile)].pen;

						switch (TILE_TRANSPARENCY(tile))
						{
#define DOALLCOPIES \
	switch (TILE_FLIP(tile) ^ flipx) {\
		case 0:\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd += 8;\
			}\
			break;\
		case TILE_FLIPX:\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd += 8;\
			}\
			break;\
		case TILE_FLIPY:\
			sd += 7*8;\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd -= 8;\
			}\
			break;\
		case (TILE_FLIPX | TILE_FLIPY):\
			sd += 7*8;\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd -= 8;\
			}\
			break;\
	}
#define DOALLCOPIESWITHMASK \
	switch (TILE_FLIP(tile) ^ flipx) {\
		case 0:\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd += 8;\
				mask++;\
			}\
			break;\
		case TILE_FLIPX:\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd += 8;\
				mask++;\
			}\
			break;\
		case TILE_FLIPY:\
			sd += 7*8;\
			mask += 7;\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd -= 8;\
				mask--;\
			}\
			break;\
		case (TILE_FLIPX | TILE_FLIPY):\
			sd += 7*8;\
			mask += 7;\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd -= 8;\
				mask--;\
			}\
			break;\
	}


							case TILE_TRANSPARENCY_TRANSPARENT:
								/* don't draw anything, totally transparent */
								bm += dx;
								break;

							case TILE_TRANSPARENCY_OPAQUE:
#define DOCOPY \
	bm[0] = paldata[sd[0]];\
	bm[1] = paldata[sd[1]];\
	bm[2] = paldata[sd[2]];\
	bm[3] = paldata[sd[3]];\
	bm[4] = paldata[sd[4]];\
	bm[5] = paldata[sd[5]];\
	bm[6] = paldata[sd[6]];\
	bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	bm[0] = paldata[sd[7]];\
	bm[1] = paldata[sd[6]];\
	bm[2] = paldata[sd[5]];\
	bm[3] = paldata[sd[4]];\
	bm[4] = paldata[sd[3]];\
	bm[5] = paldata[sd[2]];\
	bm[6] = paldata[sd[1]];\
	bm[7] = paldata[sd[0]];

								DOALLCOPIES

#undef DOCOPY
#undef DOCOPY_FLIPX
								break;

							case TILE_TRANSPARENCY_PEN:
{
	unsigned char *mask;
	unsigned char m;

	mask = gfxtilebank->tiles[TILE_CODE(tile)].mask;

#define DOCOPY \
	m = *mask;\
	if (m & 0x01) bm[0] = paldata[sd[0]];\
	if (m & 0x02) bm[1] = paldata[sd[1]];\
	if (m & 0x04) bm[2] = paldata[sd[2]];\
	if (m & 0x08) bm[3] = paldata[sd[3]];\
	if (m & 0x10) bm[4] = paldata[sd[4]];\
	if (m & 0x20) bm[5] = paldata[sd[5]];\
	if (m & 0x40) bm[6] = paldata[sd[6]];\
	if (m & 0x80) bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	m = *mask;\
	if (m & 0x01) bm[7] = paldata[sd[0]];\
	if (m & 0x02) bm[6] = paldata[sd[1]];\
	if (m & 0x04) bm[5] = paldata[sd[2]];\
	if (m & 0x08) bm[4] = paldata[sd[3]];\
	if (m & 0x10) bm[3] = paldata[sd[4]];\
	if (m & 0x20) bm[2] = paldata[sd[5]];\
	if (m & 0x40) bm[1] = paldata[sd[6]];\
	if (m & 0x80) bm[0] = paldata[sd[7]];

								DOALLCOPIESWITHMASK

#undef DOCOPY
#undef DOCOPY_FLIPX
}
								break;

							case TILE_TRANSPARENCY_COLOR:
#define DOCOPY \
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[0]])) == 0) bm[0] = paldata[sd[0]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[1]])) == 0) bm[1] = paldata[sd[1]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[2]])) == 0) bm[2] = paldata[sd[2]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[3]])) == 0) bm[3] = paldata[sd[3]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[4]])) == 0) bm[4] = paldata[sd[4]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[5]])) == 0) bm[5] = paldata[sd[5]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[6]])) == 0) bm[6] = paldata[sd[6]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[7]])) == 0) bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[0]])) == 0) bm[7] = paldata[sd[0]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[1]])) == 0) bm[6] = paldata[sd[1]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[2]])) == 0) bm[5] = paldata[sd[2]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[3]])) == 0) bm[4] = paldata[sd[3]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[4]])) == 0) bm[3] = paldata[sd[4]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[5]])) == 0) bm[2] = paldata[sd[5]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[6]])) == 0) bm[1] = paldata[sd[6]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[7]])) == 0) bm[0] = paldata[sd[7]];

								DOALLCOPIES

#undef DOCOPY
#undef DOCOPY_FLIPX
								break;
						}

						bm = bm - 8 * dy + dx;
					}
					else bm += dx;
				}
				else bm += dx;

				tiles++;

				minx += dx;
			}

			bm = bm - cols_to_copy * dx + 8 * dy;
			tiles = tiles - cols_to_copy + layer->tilemap.width / 8;
		}
		else
		{
			bm += 8 * dy;
			tiles += layer->tilemap.width / 8;
		}

		ebm += 8 * dy;

		miny += (dy > 0) ? 8 : -8;
	}
#undef DATA_SIZE
}

static void draw_tilemap_core16(int layer_num,struct osd_bitmap *bitmap,int x,int y)
{
#define DATA_SIZE unsigned short
	unsigned long *tiles;
	int dx,dy;
	DATA_SIZE *bm,*ebm,*eebm;
	int flipx,flipy;
	struct GfxLayer *layer = Machine->layer[layer_num];
	int lines_to_copy,cols_to_copy;
	int minx,maxx,miny,maxy;


	if (layer == 0) return;

	flipx = layer->tilemap.flip & TILE_FLIPX;
	flipy = layer->tilemap.flip & TILE_FLIPY;

	tiles = layer->tilemap.tiles;

	if (x >= 0)
	{
		minx = x;
		maxx = layer_dirty_maxx + 8;
		if (minx >= maxx) return;
		while (minx <= layer_dirty_minx - 8)
		{
			minx += 8;
			tiles++;
		}
		if ((maxx - minx) & 0x07)
			maxx += 8 - ((maxx - minx) & 0x07);
	}
	else
	{
		minx = layer_dirty_minx;
		maxx = x + layer->tilemap.width;
		if (minx >= maxx) return;
		if ((maxx - minx) & 0x07)
			minx -= 8 - ((maxx - minx) & 0x07);
		tiles += (layer->tilemap.width - (maxx - minx)) / 8;

		while (maxx >= layer_dirty_maxx + 16)
			maxx -= 8;
	}

	if (y >= 0)
	{
		miny = y;
		maxy = layer_dirty_maxy + 8;
		if (miny >= maxy) return;
		while (miny <= layer_dirty_miny - 8)
		{
			miny += 8;
			tiles += (layer->tilemap.width / 8);
		}
		if ((maxy - miny) & 0x07)
			maxy += 8 - ((maxy - miny) & 0x07);
	}
	else
	{
		miny = layer_dirty_miny;
		maxy = y + layer->tilemap.height;
		if (miny >= maxy) return;
		if ((maxy - miny) & 0x07)
			miny -= 8 - ((maxy - miny) & 0x07);
		tiles += (layer->tilemap.width / 8) * (layer->tilemap.height - (maxy - miny)) / 8;

		while (maxy >= layer_dirty_maxy + 16)
			maxy -= 8;
	}


	cols_to_copy = (maxx - minx) / 8;
	lines_to_copy = maxy - miny;

	dy = (DATA_SIZE *)bitmap->line[1] - (DATA_SIZE *)bitmap->line[0];
	bm = (DATA_SIZE *)bitmap->line[0] + dy * miny + minx;
	if (flipy)
	{
		bm += (layer->tilemap.height - 1) * dy;
		miny += layer->tilemap.height - 8;
		dy = -dy;
	}
	dx = 8;
	if (flipx)
	{
		bm += (layer->tilemap.width / 8 - 1) * dx;
		dx = -dx;
	}

	ebm = bm + cols_to_copy * dx;
	eebm = ebm + lines_to_copy * dy;

	while (ebm != eebm)
	{
		if (layer_is_line_dirty(layer,miny))
		{
			int countdirty;


			if (dy > 0)
				minx = (bm - ((DATA_SIZE *)bitmap->line[0]-8-8*dy)) % dy - 8;
			else
				minx = (bm - ((DATA_SIZE *)bitmap->line[0]-8+8*dy)) % -dy - 8;

			countdirty = 0;

			while (bm != ebm)
			{
				if (countdirty == 0)
				{
					countdirty = layer_count_blocks_dirty(layer,minx,miny);
					if (countdirty > 0)
					{
						if (layer_num == 0)
						{
							/* mark screen bitmap dirty */
							layer_mark_blocks_dirty(Machine->dirtylayer,minx,miny,countdirty);
						}
						else
						{
							/* mark tiles in the layer above this one as dirty */
							layer_mark_blocks_dirty(Machine->layer[layer_num - 1],minx,miny,countdirty);
						}
					}
				}

				if (countdirty > 0)
				{
					long tile;


					countdirty--;

					tile = *tiles;

					if (TILE_TRANSPARENCY(tile) != TILE_TRANSPARENCY_TRANSPARENT &&
							is_tile_obscured(layer_num,minx,miny) == 0)
					{
						const unsigned short *paldata;
						const unsigned char *sd;
						const struct GfxTileBank *gfxtilebank;
						int i;


						/* mark all tiles below this one as not clean */
						for (i = layer_num + 1;i < MAX_LAYERS && Machine->layer[i];i++)
							layer_mark_block_dirty(Machine->layer[i],minx,miny,MARK_ALL_NOT_CLEAN);

						gfxtilebank = layer->tilemap.gfxtilebank[TILE_BANK(tile)];

						paldata = &gfxtilebank->colortable[gfxtilebank->color_granularity * TILE_COLOR(tile)];
						sd = gfxtilebank->tiles[TILE_CODE(tile)].pen;

						switch (TILE_TRANSPARENCY(tile))
						{
#define DOALLCOPIES \
	switch (TILE_FLIP(tile) ^ flipx) {\
		case 0:\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd += 8;\
			}\
			break;\
		case TILE_FLIPX:\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd += 8;\
			}\
			break;\
		case TILE_FLIPY:\
			sd += 7*8;\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd -= 8;\
			}\
			break;\
		case (TILE_FLIPX | TILE_FLIPY):\
			sd += 7*8;\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd -= 8;\
			}\
			break;\
	}
#define DOALLCOPIESWITHMASK \
	switch (TILE_FLIP(tile) ^ flipx) {\
		case 0:\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd += 8;\
				mask++;\
			}\
			break;\
		case TILE_FLIPX:\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd += 8;\
				mask++;\
			}\
			break;\
		case TILE_FLIPY:\
			sd += 7*8;\
			mask += 7;\
			for (i = 8;i > 0;i--) {\
				DOCOPY\
				bm += dy;\
				sd -= 8;\
				mask--;\
			}\
			break;\
		case (TILE_FLIPX | TILE_FLIPY):\
			sd += 7*8;\
			mask += 7;\
			for (i = 8;i > 0;i--) {\
				DOCOPY_FLIPX\
				bm += dy;\
				sd -= 8;\
				mask--;\
			}\
			break;\
	}


							case TILE_TRANSPARENCY_TRANSPARENT:
								/* don't draw anything, totally transparent */
								bm += dx;
								break;

							case TILE_TRANSPARENCY_OPAQUE:
#define DOCOPY \
	bm[0] = paldata[sd[0]];\
	bm[1] = paldata[sd[1]];\
	bm[2] = paldata[sd[2]];\
	bm[3] = paldata[sd[3]];\
	bm[4] = paldata[sd[4]];\
	bm[5] = paldata[sd[5]];\
	bm[6] = paldata[sd[6]];\
	bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	bm[0] = paldata[sd[7]];\
	bm[1] = paldata[sd[6]];\
	bm[2] = paldata[sd[5]];\
	bm[3] = paldata[sd[4]];\
	bm[4] = paldata[sd[3]];\
	bm[5] = paldata[sd[2]];\
	bm[6] = paldata[sd[1]];\
	bm[7] = paldata[sd[0]];

								DOALLCOPIES

#undef DOCOPY
#undef DOCOPY_FLIPX
								break;

							case TILE_TRANSPARENCY_PEN:
{
	unsigned char *mask;
	unsigned char m;

	mask = gfxtilebank->tiles[TILE_CODE(tile)].mask;

#define DOCOPY \
	m = *mask;\
	if (m & 0x01) bm[0] = paldata[sd[0]];\
	if (m & 0x02) bm[1] = paldata[sd[1]];\
	if (m & 0x04) bm[2] = paldata[sd[2]];\
	if (m & 0x08) bm[3] = paldata[sd[3]];\
	if (m & 0x10) bm[4] = paldata[sd[4]];\
	if (m & 0x20) bm[5] = paldata[sd[5]];\
	if (m & 0x40) bm[6] = paldata[sd[6]];\
	if (m & 0x80) bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	m = *mask;\
	if (m & 0x01) bm[7] = paldata[sd[0]];\
	if (m & 0x02) bm[6] = paldata[sd[1]];\
	if (m & 0x04) bm[5] = paldata[sd[2]];\
	if (m & 0x08) bm[4] = paldata[sd[3]];\
	if (m & 0x10) bm[3] = paldata[sd[4]];\
	if (m & 0x20) bm[2] = paldata[sd[5]];\
	if (m & 0x40) bm[1] = paldata[sd[6]];\
	if (m & 0x80) bm[0] = paldata[sd[7]];

								DOALLCOPIESWITHMASK

#undef DOCOPY
#undef DOCOPY_FLIPX
}
								break;

							case TILE_TRANSPARENCY_COLOR:
#define DOCOPY \
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[0]])) == 0) bm[0] = paldata[sd[0]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[1]])) == 0) bm[1] = paldata[sd[1]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[2]])) == 0) bm[2] = paldata[sd[2]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[3]])) == 0) bm[3] = paldata[sd[3]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[4]])) == 0) bm[4] = paldata[sd[4]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[5]])) == 0) bm[5] = paldata[sd[5]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[6]])) == 0) bm[6] = paldata[sd[6]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[7]])) == 0) bm[7] = paldata[sd[7]];
#define DOCOPY_FLIPX \
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[0]])) == 0) bm[7] = paldata[sd[0]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[1]])) == 0) bm[6] = paldata[sd[1]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[2]])) == 0) bm[5] = paldata[sd[2]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[3]])) == 0) bm[4] = paldata[sd[3]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[4]])) == 0) bm[3] = paldata[sd[4]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[5]])) == 0) bm[2] = paldata[sd[5]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[6]])) == 0) bm[1] = paldata[sd[6]];\
	if ((gfxtilebank->transparency_mask & (1<<paldata[sd[7]])) == 0) bm[0] = paldata[sd[7]];

								DOALLCOPIES

#undef DOCOPY
#undef DOCOPY_FLIPX
								break;
						}

						bm = bm - 8 * dy + dx;
					}
					else bm += dx;
				}
				else bm += dx;

				tiles++;

				minx += dx;
			}

			bm = bm - cols_to_copy * dx + 8 * dy;
			tiles = tiles - cols_to_copy + layer->tilemap.width / 8;
		}
		else
		{
			bm += 8 * dy;
			tiles += layer->tilemap.width / 8;
		}

		ebm += 8 * dy;

		miny += (dy > 0) ? 8 : -8;
	}
#undef DATA_SIZE
}

void update_tile_layer(int layer_num,struct osd_bitmap *bitmap)
{
	struct GfxLayer *layer = Machine->layer[layer_num];


	if (bitmap->depth != 16)
	{
		draw_tilemap_core8(layer_num,bitmap,
				layer->tilemap.scrollx,layer->tilemap.scrolly);
		draw_tilemap_core8(layer_num,bitmap,
				layer->tilemap.scrollx - layer->tilemap.width,layer->tilemap.scrolly);
		draw_tilemap_core8(layer_num,bitmap,
				layer->tilemap.scrollx,layer->tilemap.scrolly - layer->tilemap.height);
		draw_tilemap_core8(layer_num,bitmap,
				layer->tilemap.scrollx - layer->tilemap.width,layer->tilemap.scrolly - layer->tilemap.height);
	}
	else
	{
		draw_tilemap_core16(layer_num,bitmap,
				layer->tilemap.scrollx,layer->tilemap.scrolly);
		draw_tilemap_core16(layer_num,bitmap,
				layer->tilemap.scrollx - layer->tilemap.width,layer->tilemap.scrolly);
		draw_tilemap_core16(layer_num,bitmap,
				layer->tilemap.scrollx,layer->tilemap.scrolly - layer->tilemap.height);
		draw_tilemap_core16(layer_num,bitmap,
				layer->tilemap.scrollx - layer->tilemap.width,layer->tilemap.scrolly - layer->tilemap.height);
	}

	layer_mark_all_dirty(layer,MARK_CLEAN_IF_DIRTY);
}



static void set_tile_attributes_core(int layer_num,struct osd_bitmap *bitmap,int x,int y,int attr)
{
	int tile;
	int bank,code,color;
	int transparency;
	struct GfxLayer *layer = Machine->layer[layer_num];
	int minx,miny,i;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;


		temp = x;
		x = y;
		y = temp;

		attr = (attr & ~TILE_FLIP_MASK)
				| ((attr & TILE_FLIPX) ? TILE_FLIPY : 0) | ((attr & TILE_FLIPY) ? TILE_FLIPX : 0);
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = layer->tilemap.width / 8 - 1 - x;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = layer->tilemap.height / 8 - 1 - y;

	if (x < 0 || x >= layer->tilemap.width / 8 || y < 0 || y >= layer->tilemap.height / 8)
		return;

	tile = y * (layer->tilemap.width / 8) + x;

	bank = TILE_BANK(attr);
	code = TILE_CODE(attr);
	color = TILE_COLOR(attr);
	transparency = TILE_TRANSPARENCY(attr);

	/* calculate transparency */
	{
		int transmask;
		struct GfxTileBank *gfxtilebank = layer->tilemap.gfxtilebank[bank];


		switch (transparency)
		{
			case TILE_TRANSPARENCY_TRANSPARENT:
			default:
				transmask = 0xffffffff;
				break;

			case TILE_TRANSPARENCY_OPAQUE:
				transmask = 0;
				break;

			case TILE_TRANSPARENCY_PEN:
				transmask = gfxtilebank->transparency_mask;
				break;

			case TILE_TRANSPARENCY_COLOR:
				{
					const unsigned short *paldata;


					paldata = &gfxtilebank->colortable[gfxtilebank->color_granularity * color];

					transmask = 0;
					for (i = gfxtilebank->color_granularity - 1;i >= 0;i--)
					{
						if (gfxtilebank->transparency_mask & (1 << paldata[i]))
							transmask |= 1 << i;
					}
				}
				break;
		}

		if ((gfxtilebank->tiles[code].pens_used & transmask) == 0)
			/* character is totally opaque, can disable transparency */
			attr = (attr & ~TILE_TRANSPARENCY_MASK) | TILE_TRANSPARENCY_OPAQUE;
		else if ((gfxtilebank->tiles[code].pens_used & ~transmask) == 0)
		{
			/* if the tile was already transparent we can quit, no need to redraw. */
			if (TILE_TRANSPARENCY(layer->tilemap.tiles[tile]) == TILE_TRANSPARENCY_TRANSPARENT)
				return;

			/* character is totally transparent, no need to draw */
			attr = (attr & ~TILE_TRANSPARENCY_MASK) | TILE_TRANSPARENCY_TRANSPARENT;
		}
	}


	layer->tilemap.tiles[tile] = attr;

	minx = 8*x;
	if (layer->tilemap.flip & TILE_FLIPX)
		minx = layer->tilemap.width-8-minx;

	miny = 8*y;
	if (layer->tilemap.flip & TILE_FLIPY)
		miny = layer->tilemap.height-8-miny;

	minx += layer->tilemap.scrollx;
	miny += layer->tilemap.scrolly;

/* TODO: a possible additional optimization is to only mark the tile as dirty */
/* if it is not fully covered by other layers. However, to handle that we should */
/* pospone the tile[] array update to after all the layers know their scroll offsets. */
	layer_mark_block_dirty(layer,minx,miny,MARK_ALL_DIRTY);
	layer_mark_block_dirty(layer,minx-layer->tilemap.width,miny,MARK_ALL_DIRTY);
	layer_mark_block_dirty(layer,minx,miny-layer->tilemap.height,MARK_ALL_DIRTY);
	layer_mark_block_dirty(layer,minx-layer->tilemap.width,miny-layer->tilemap.height,MARK_ALL_DIRTY);

	/* if the tile is not opaque, mark tiles in the bottom layer as dirty if they */
	/* need to be redrawn before redrawing it. */
	if (TILE_TRANSPARENCY(attr) != TILE_TRANSPARENCY_OPAQUE)
	{
		i = MAX_LAYERS - 1;
		while (Machine->layer[i] == 0) i--;	/* find the bottom layer */
		layer_mark_block_dirty(Machine->layer[i],minx,miny,MARK_DIRTY_IF_NOT_CLEAN);
		layer_mark_block_dirty(Machine->layer[i],minx-layer->tilemap.width,miny,MARK_DIRTY_IF_NOT_CLEAN);
		layer_mark_block_dirty(Machine->layer[i],minx,miny-layer->tilemap.height,MARK_DIRTY_IF_NOT_CLEAN);
		layer_mark_block_dirty(Machine->layer[i],minx-layer->tilemap.width,miny-layer->tilemap.height,MARK_DIRTY_IF_NOT_CLEAN);
	}
}

void set_tile_layer_attributes(int layer_num,struct osd_bitmap *bitmap,int scrollx,int scrolly,int flipx,int flipy,unsigned long global_attr_mask,unsigned long global_attr)
{
	int x,y;
	struct GfxLayer *layer = Machine->layer[layer_num];
	struct GfxTileCompose *compose;


	if (layer == 0) return;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;


		if (flipx) flipx = TILE_FLIPY;
		else flipx = 0;
		if (flipy) flipy = TILE_FLIPX;
		else flipy = 0;

		temp = scrollx;
		scrollx = scrolly;
		scrolly = temp;
	}
	else
	{
		if (flipx) flipx = TILE_FLIPX;
		else flipx = 0;
		if (flipy) flipy = TILE_FLIPY;
		else flipy = 0;
	}
	if (!(Machine->orientation & ORIENTATION_FLIP_X) ^ !flipx)
		scrollx = bitmap->width - layer->tilemap.width - scrollx;
	if (!(Machine->orientation & ORIENTATION_FLIP_Y) ^ !flipy)
		scrolly = bitmap->height - layer->tilemap.height - scrolly;

	if (scrollx < 0)
	{
		do
		{
			scrollx += layer->tilemap.width;
		} while (scrollx < 0);
	}
	if (scrollx >= layer->tilemap.width)
	{
		do
		{
			scrollx -= layer->tilemap.width;
		} while (scrollx >= layer->tilemap.width);
	}
	if (scrolly < 0)
	{
		do
		{
			scrolly += layer->tilemap.height;
		} while (scrolly < 0);
	}
	if (scrolly >= layer->tilemap.height)
	{
		do
		{
			scrolly -= layer->tilemap.height;
		} while (scrolly >= layer->tilemap.height);
	}

	if (layer->tilemap.scrollx != scrollx || layer->tilemap.scrolly != scrolly ||
			layer->tilemap.flip != (flipx | flipy) ||
			layer->tilemap.globalattr != global_attr)
	{
		int i;


		layer->tilemap.scrollx = scrollx;
		layer->tilemap.scrolly = scrolly;
		layer->tilemap.flip = (flipx | flipy);
		if (layer->tilemap.globalattr != global_attr)
		{
			layer->tilemap.globalattr = global_attr;
			for (i = (layer->tilemap.width / 8) * (layer->tilemap.height / 8) - 1;i >= 0;i--)
				layer->tilemap.tiles[i] = (layer->tilemap.tiles[i] & ~global_attr_mask) | global_attr;
		}

		layer_mark_all_dirty(layer,MARK_ALL_DIRTY);

		/* mark tiles in the bottom layer as dirty if they need to be refreshed */
		i = MAX_LAYERS - 1;
		while (Machine->layer[i] == 0) i--;	/* find the bottom layer */
		layer_mark_all_dirty(Machine->layer[i],MARK_DIRTY_IF_NOT_CLEAN);
	}


	compose = layer->tilemap.gfxtilecompose;

	if (compose == 0)
	{
		for (y = 0;y < layer->tilemap.virtualheight;y++)
		{
			for (x = 0;x < layer->tilemap.virtualwidth;x++)
			{
				if (layer->tilemap.virtualdirty[y * layer->tilemap.virtualwidth + x])
				{
					layer->tilemap.virtualdirty[y * layer->tilemap.virtualwidth + x] = 0;

					set_tile_attributes_core(layer_num,bitmap,x,y,
							layer->tilemap.virtualtiles[y * layer->tilemap.virtualwidth + x] | global_attr);
				}
			}
		}
	}
	else
	{
/* TODO: support compose->bankxoffset[] and compose->bankyoffset[] */
		for (y = 0;y < layer->tilemap.virtualheight;y++)
		{
			for (x = 0;x < layer->tilemap.virtualwidth;x++)
			{
				if (layer->tilemap.virtualdirty[y * layer->tilemap.virtualwidth + x])
				{
					int code,attr,xx,yy;


					layer->tilemap.virtualdirty[y * layer->tilemap.virtualwidth + x] = 0;

					attr = (layer->tilemap.virtualtiles[y * layer->tilemap.virtualwidth + x] & ~global_attr_mask) | global_attr;
					code = TILE_CODE(attr) * compose->multiplier;
					attr = TILE_ATTR(attr);

					switch (TILE_FLIP(attr))
					{
						case 0:
							for (yy = 0;yy < compose->height;yy++)
							{
								for (xx = 0;xx < compose->width;xx++)
								{
									set_tile_attributes_core(layer_num,bitmap,
											compose->width * x + xx,compose->height * y + yy,
											MAKE_TILE_CODEATTR(code + compose->xoffset[xx] + compose->yoffset[yy],attr));
								}
							}
							break;
						case TILE_FLIPX:
							for (yy = 0;yy < compose->height;yy++)
							{
								for (xx = 0;xx < compose->width;xx++)
								{
									set_tile_attributes_core(layer_num,bitmap,
											compose->width * x + compose->width - 1 - xx,compose->height * y + yy,
											MAKE_TILE_CODEATTR(code + compose->xoffset[xx] + compose->yoffset[yy],attr));
								}
							}
							break;
						case TILE_FLIPY:
							for (yy = 0;yy < compose->height;yy++)
							{
								for (xx = 0;xx < compose->width;xx++)
								{
									set_tile_attributes_core(layer_num,bitmap,
											compose->width * x + xx,compose->height * y + compose->height - 1 - yy,
											MAKE_TILE_CODEATTR(code + compose->xoffset[xx] + compose->yoffset[yy],attr));
								}
							}
							break;
						case (TILE_FLIPX | TILE_FLIPY):
							for (yy = 0;yy < compose->height;yy++)
							{
								for (xx = 0;xx < compose->width;xx++)
								{
									set_tile_attributes_core(layer_num,bitmap,
											compose->width * x + compose->width - 1 - xx,compose->height * y + compose->height - 1 - yy,
											MAKE_TILE_CODEATTR(code + compose->xoffset[xx] + compose->yoffset[yy],attr));
								}
							}
							break;
					}
				}
			}
		}
	}
}

void set_tile_attributes_fast(int layer_num,int tile,int attr)
{
	struct GfxLayer *layer = Machine->layer[layer_num];


	if (layer->tilemap.virtualtiles[tile] != attr)
	{
		layer->tilemap.virtualtiles[tile] = attr;
		layer->tilemap.virtualdirty[tile] = 1;
	}
}

struct GfxLayer *create_tile_layer(struct MachineLayer *ml)
{
	struct GfxLayer *layer;
	int i,len,virtuallen;


/* TODO: this part should only be done once since it initializes the globals */
{
	int minx,miny,maxx,maxy;


	/* determine the size of the dirty buffer */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		layer_width = Machine->drv->screen_height;
		layer_height = Machine->drv->screen_width;
		minx = Machine->drv->visible_area.min_y;
		maxx = Machine->drv->visible_area.max_y;
		miny = Machine->drv->visible_area.min_x;
		maxy = Machine->drv->visible_area.max_x;
	}
	else
	{
		layer_width = Machine->drv->screen_width;
		layer_height = Machine->drv->screen_height;
		minx = Machine->drv->visible_area.min_x;
		maxx = Machine->drv->visible_area.max_x;
		miny = Machine->drv->visible_area.min_y;
		maxy = Machine->drv->visible_area.max_y;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		int temp;


		temp = minx;
		minx = layer_width - 1 - maxx;
		maxx = layer_width - 1 - temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		int temp;


		temp = miny;
		miny = layer_height - 1 - maxy;
		maxy = layer_height - 1 - temp;
	}

	layer_dirty_minx = minx & ~0x07;
	layer_dirty_maxx = maxx & ~0x07;
	layer_dirty_miny = miny & ~0x07;
	layer_dirty_maxy = maxy & ~0x07;

	layer_dirty_shift = 0;
	for (i = (layer_width - 1) >> 3;i != 0;i >>= 1)
		layer_dirty_shift++;

	layer_dirty_size = (1 << layer_dirty_shift) * (layer_height >> 3) * sizeof(unsigned char);
}


	if ((layer = malloc(sizeof(struct GfxLayer))) != 0)
	{
		layer->dirty = malloc(layer_dirty_size);
		layer->dirtyline = malloc((layer_height + 7) / 8);
		if (layer->dirty && layer->dirtyline)
		{
			/* first of all, mark everything clean */
			memset(layer->dirty,TILE_CLEAN,layer_dirty_size);
			memset(layer->dirtyline,0,(layer_height + 7) / 8);
			/* then mark the *visible* region dirty */
			layer_mark_all_dirty(layer,MARK_ALL_DIRTY);
		}


		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			layer->tilemap.width = ml->height;
			layer->tilemap.height = ml->width;
		}
		else
		{
			layer->tilemap.width = ml->width;
			layer->tilemap.height = ml->height;
		}

		layer->tilemap.virtualwidth = ml->width / 8;
		layer->tilemap.virtualheight = ml->height / 8;
		layer->tilemap.gfxtilecompose = ml->gfxtilecompose;
		if (ml->gfxtilecompose)
		{
			layer->tilemap.virtualwidth /= ml->gfxtilecompose->width;
			layer->tilemap.virtualheight /= ml->gfxtilecompose->height;
		}
		layer->tilemap.flip = 0;

		len = (ml->width / 8) * (ml->height / 8);
		virtuallen = layer->tilemap.virtualwidth * layer->tilemap.virtualheight;
		layer->tilemap.tiles = malloc(len * sizeof(unsigned long));
		layer->tilemap.virtualtiles = malloc(virtuallen * sizeof(unsigned long));
		layer->tilemap.virtualdirty = malloc(virtuallen * sizeof(unsigned char));
		for (i = 0;i < MAX_TILE_BANKS;i++)
			layer->tilemap.gfxtilebank[i] = 0;

		if (layer->tilemap.tiles == 0 || layer->tilemap.virtualtiles == 0 || layer->tilemap.virtualdirty == 0)
		{
			free_tile_layer(layer);
			return 0;
		}
		else
		{
			if (ml->gfxtiledecodeinfo)
			{
				i = 0;
				while (i < MAX_TILE_BANKS && ml->gfxtiledecodeinfo[i].memory_region != -1)
				{
					if ((layer->tilemap.gfxtilebank[i] = decodetiles(&ml->gfxtiledecodeinfo[i],ml)) == 0)
					{
						free_tile_layer(layer);
						return 0;
					}

					i++;
				}
			}

			memset(layer->tilemap.tiles,0,len * sizeof(unsigned long));
			memset(layer->tilemap.virtualtiles,0,virtuallen * sizeof(unsigned long));
			memset(layer->tilemap.virtualdirty,1,virtuallen * sizeof(unsigned char));
		}
	}

	return layer;
}

void free_tile_layer(struct GfxLayer *layer)
{
	int i;


	if (layer == 0) return;

	free(layer->dirty);
	free(layer->dirtyline);

	free(layer->tilemap.tiles);
	for (i = 0;i < MAX_TILE_BANKS;i++)
		freetiles(layer->tilemap.gfxtilebank[i]);

	free(layer);
}

void layer_mark_rectangle_dirty(struct GfxLayer *layer,int minx,int maxx,int miny,int maxy)
{
	int y;


	if (layer == 0) return;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;


		temp = minx;
		minx = miny;
		miny = temp;

		temp = maxx;
		maxx = maxy;
		maxy = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		int temp;


		temp = minx;
		minx = layer_width - 1 - maxx;
		maxx = layer_width - 1 - temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		int temp;


		temp = miny;
		miny = layer_height - 1 - maxy;
		maxy = layer_height - 1 - temp;
	}

	if (minx < layer_dirty_minx) minx = layer_dirty_minx;
	if (miny < layer_dirty_miny) miny = layer_dirty_miny;
	if (maxx > layer_dirty_maxx) maxx = layer_dirty_maxx;
	if (maxy > layer_dirty_maxy) maxy = layer_dirty_maxy;

	for (y = miny & ~0x07;y <= maxy;y += 8)
	{
		layer_mark_blocks_dirty(layer,minx,y,(maxx - minx) / 8 + 1);
	}
}

void layer_mark_rectangle_dirty_norotate(struct GfxLayer *layer,int minx,int maxx,int miny,int maxy)
{
	int y;


	if (layer == 0) return;

	if (minx < layer_dirty_minx) minx = layer_dirty_minx;
	if (miny < layer_dirty_miny) miny = layer_dirty_miny;
	if (maxx > layer_dirty_maxx) maxx = layer_dirty_maxx;
	if (maxy > layer_dirty_maxy) maxy = layer_dirty_maxy;

	for (y = miny & ~0x07;y <= maxy;y += 8)
	{
		layer_mark_blocks_dirty(layer,minx,y,(((maxx + 7) & ~0x07) - (minx & ~0x07)) / 8 + 1);
	}
}

void layer_mark_full_screen_dirty(void)
{
	int i;
	extern int bitmap_dirty;	/* in mame.c */

	/* mame.c passes this to the drivers (*vh_update)() */
	bitmap_dirty = 1;

	i = MAX_LAYERS - 1;
	while (i >= 0 && Machine->layer[i] == 0) i--;	/* find the bottom layer */

	if (i >= 0)
		layer_mark_all_dirty(Machine->layer[i],MARK_ALL_DIRTY);
}
