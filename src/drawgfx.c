#include "driver.h"


/* LBO */
#ifdef LSB_FIRST
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#endif

#define TA

#ifdef ALIGN_INTS /* GSL 980108 read/write nonaligned dword routine for ARM processor etc */

INLINE int read_dword(void *address)
{
	if ((long)address & 3)
	{
#ifdef LSB_FIRST  /* little endian version */
  		return ( *((unsigned char *)address) +
				(*((unsigned char *)address+1) << 8)  +
				(*((unsigned char *)address+2) << 16) +
				(*((unsigned char *)address+3) << 24) );
#else             /* big endian version */
  		return ( *((unsigned char *)address+3) +
				(*((unsigned char *)address+2) << 8)  +
				(*((unsigned char *)address+1) << 16) +
				(*((unsigned char *)address)   << 24) );
#endif
	}
	else
		return *(int *)address;
}


INLINE void write_dword(void *address, int data)
{
  	if ((long)address & 3)
	{
#ifdef LSB_FIRST
    		*((unsigned char *)address) =    data;
    		*((unsigned char *)address+1) = (data >> 8);
    		*((unsigned char *)address+2) = (data >> 16);
    		*((unsigned char *)address+3) = (data >> 24);
#else
    		*((unsigned char *)address+3) =  data;
    		*((unsigned char *)address+2) = (data >> 8);
    		*((unsigned char *)address+1) = (data >> 16);
    		*((unsigned char *)address)   = (data >> 24);
#endif
		return;
  	}
  	else
		*(int *)address = data;
}
#else
#define read_dword(address) *(int *)address
#define write_dword(address,data) *(int *)address=data
#endif



static int readbit(const unsigned char *src,int bitnum)
{
	return (src[bitnum / 8] >> (7 - bitnum % 8)) & 1;
}


void decodechar(struct GfxElement *gfx,int num,const unsigned char *src,const struct GfxLayout *gl)
{
	int plane,x,y;
	unsigned char *dp;



	for (plane = 0;plane < gl->planes;plane++)
	{
		int offs;


		offs = num * gl->charincrement + gl->planeoffset[plane];

		for (y = 0;y < gfx->height;y++)
		{
			dp = gfx->gfxdata->line[num * gfx->height + y];

			for (x = 0;x < gfx->width;x++)
			{
				int xoffs,yoffs;


				if (plane == 0) dp[x] = 0;
				else dp[x] <<= 1;

				xoffs = x;
				yoffs = y;

				if (Machine->orientation & ORIENTATION_FLIP_X)
					xoffs = gfx->width-1 - xoffs;

				if (Machine->orientation & ORIENTATION_FLIP_Y)
					yoffs = gfx->height-1 - yoffs;

				if (Machine->orientation & ORIENTATION_SWAP_XY)
				{
					int temp;


					temp = xoffs;
					xoffs = yoffs;
					yoffs = temp;
				}

				dp[x] += readbit(src,offs + gl->yoffset[yoffs] + gl->xoffset[xoffs]);
			}
		}
	}


	if (gfx->pen_usage)
	{
		/* fill the pen_usage array with info on the used pens */
		gfx->pen_usage[num] = 0;

		for (y = 0;y < gfx->height;y++)
		{
			dp = gfx->gfxdata->line[num * gfx->height + y];

			for (x = 0;x < gfx->width;x++)
			{
				gfx->pen_usage[num] |= 1 << dp[x];
			}
		}
	}
}


struct GfxElement *decodegfx(const unsigned char *src,const struct GfxLayout *gl)
{
	int c;
	struct osd_bitmap *bm;
	struct GfxElement *gfx;


	if ((gfx = malloc(sizeof(struct GfxElement))) == 0)
		return 0;
	memset(gfx,0,sizeof(struct GfxElement));

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		gfx->width = gl->height;
		gfx->height = gl->width;

		if ((bm = osd_create_bitmap(gl->total * gfx->height,gfx->width)) == 0)
		{
			free(gfx);
			return 0;
		}
	}
	else
	{
		gfx->width = gl->width;
		gfx->height = gl->height;

		if ((bm = osd_create_bitmap(gfx->width,gl->total * gfx->height)) == 0)
		{
			free(gfx);
			return 0;
		}
	}

	gfx->total_elements = gl->total;
	gfx->color_granularity = 1 << gl->planes;
	gfx->gfxdata = bm;

	gfx->pen_usage = 0; /* need to make sure this is NULL if the next test fails) */
	if (gfx->color_granularity <= 32)	/* can't handle more than 32 pens */
		gfx->pen_usage = malloc(gfx->total_elements * sizeof(int));
		/* no need to check for failure, the code can work without pen_usage */

	for (c = 0;c < gl->total;c++)
		decodechar(gfx,c,src,gl);

	return gfx;
}


void freegfx(struct GfxElement *gfx)
{
	if (gfx)
	{
		free(gfx->pen_usage);
		osd_free_bitmap(gfx->gfxdata);
		free(gfx);
	}
}




INLINE void blockmove_transpen_noremap8(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		UINT8 *dstdata,int dstmodulo,
		int transpen)
{
	UINT8 *end;
	int trans4;
	UINT32 *sd4;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	trans4 = transpen * 0x01010101;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = *(srcdata++);
			if (col != transpen) *dstdata = col;
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			UINT32 col4;

			if ((col4 = *(sd4++)) != trans4)
			{
				UINT32 xod4;

				xod4 = col4 ^ trans4;
				if( (xod4&0x000000ff) && (xod4&0x0000ff00) &&
					(xod4&0x00ff0000) && (xod4&0xff000000) )
				{
					write_dword((UINT32 *)dstdata,col4);
				}
				else
				{
					if (xod4 & 0xff000000) dstdata[BL3] = col4 >> 24;
					if (xod4 & 0x00ff0000) dstdata[BL2] = col4 >> 16;
					if (xod4 & 0x0000ff00) dstdata[BL1] = col4 >>  8;
					if (xod4 & 0x000000ff) dstdata[BL0] = col4;
				}
			}
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = *(srcdata++);
			if (col != transpen) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
}

INLINE void blockmove_transpen_noremap_flipx8(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		UINT8 *dstdata,int dstmodulo,
		int transpen)
{
	UINT8 *end;
	int trans4;
	UINT32 *sd4;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;
	srcdata -= 3;

	trans4 = transpen * 0x01010101;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (col != transpen) *dstdata = col;
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			UINT32 col4;

			if ((col4 = *(sd4--)) != trans4)
			{
				UINT32 xod4;

				xod4 = col4 ^ trans4;
				if (xod4 & 0x000000ff) dstdata[BL3] = col4;
				if (xod4 & 0x0000ff00) dstdata[BL2] = col4 >>  8;
				if (xod4 & 0x00ff0000) dstdata[BL1] = col4 >> 16;
				if (xod4 & 0xff000000) dstdata[BL0] = col4 >> 24;
			}
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (col != transpen) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
}


INLINE void blockmove_transpen_noremap16(
		const UINT16 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		UINT16 *dstdata,int dstmodulo,
		int transpen)
{
	UINT16 *end;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			int col;

			col = *(srcdata++);
			if (col != transpen) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
}

INLINE void blockmove_transpen_noremap_flipx16(
		const UINT16 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		UINT16 *dstdata,int dstmodulo,
		int transpen)
{
	UINT16 *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			int col;

			col = *(srcdata--);
			if (col != transpen) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
}


#define DATA_TYPE UINT8
#define DECLARE(function,args,body) INLINE void function##8 args body
#define BLOCKMOVE(function,args) \
	if (flipx) blockmove_##function##_flipx##8 args ; \
	else blockmove_##function##8 args
#include "drawgfxl.c"
#undef DATA_TYPE
#undef DECLARE
#undef BLOCKMOVE

#define DATA_TYPE UINT16
#define DECLARE(function,args,body) INLINE void function##16 args body
#define BLOCKMOVE(function,args) \
	if (flipx) blockmove_##function##_flipx##16 args ; \
	else blockmove_##function##16 args
#include "drawgfxl.c"
#undef DATA_TYPE
#undef DECLARE
#undef BLOCKMOVE



/***************************************************************************

  Draw graphic elements in the specified bitmap.

  transparency == TRANSPARENCY_NONE - no transparency.
  transparency == TRANSPARENCY_PEN - bits whose _original_ value is == transparent_color
                                     are transparent. This is the most common kind of
									 transparency.
  transparency == TRANSPARENCY_PENS - as above, but transparent_color is a mask of
  									 transparent pens.
  transparency == TRANSPARENCY_COLOR - bits whose _remapped_ value is == Machine->pens[transparent_color]
                                     are transparent. This is used by e.g. Pac Man.
  transparency == TRANSPARENCY_THROUGH - if the _destination_ pixel is == transparent_color,
                                     the source pixel is drawn over it. This is used by
									 e.g. Jr. Pac Man to draw the sprites when the background
									 has priority over them.

***************************************************************************/

void drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	if (dest->depth != 16)
		drawgfx_core8(dest,gfx,code,color,flipx,flipy,sx,sy,clip,transparency,transparent_color);
	else
		drawgfx_core16(dest,gfx,code,color,flipx,flipy,sx,sy,clip,transparency,transparent_color);
}


/***************************************************************************

  Use drawgfx() to copy a bitmap onto another at the given position.
  This function will very likely change in the future.

***************************************************************************/
void copybitmap(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	if (dest->depth != 16)
		copybitmap_core8(dest,src,flipx,flipy,sx,sy,clip,transparency,transparent_color);
	else
		copybitmap_core16(dest,src,flipx,flipy,sx,sy,clip,transparency,transparent_color);
}


void copybitmapzoom(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scalex,int scaley)
{
	static struct GfxElement mygfx =
	{
		0,0,0,	/* filled in later */
		1,1,0,1
	};
unsigned short hacktable[256];
int i;

	mygfx.width = src->width;
	mygfx.height = src->height;
	mygfx.gfxdata = src;
mygfx.colortable = hacktable;
for (i = 0;i < 256;i++) hacktable[i] = i;
	drawgfxzoom(dest,&mygfx,0,0,flipx,flipy,sx,sy,clip,transparency,transparent_color,scalex,scaley);	/* ASG 971011 */
}


/***************************************************************************

  Copy a bitmap onto another with scroll and wraparound.
  This function supports multiple independently scrolling rows/columns.
  "rows" is the number of indepentently scrolling rows. "rowscroll" is an
  array of integers telling how much to scroll each row. Same thing for
  "cols" and "colscroll".
  If the bitmap cannot scroll in one direction, set rows or columns to 0.
  If the bitmap scrolls as a whole, set rows and/or cols to 1.
  Bidirectional scrolling is, of course, supported only if the bitmap
  scrolls as a whole in at least one direction.

***************************************************************************/
void copyscrollbitmap(struct osd_bitmap *dest,struct osd_bitmap *src,
		int rows,const int *rowscroll,int cols,const int *colscroll,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	int srcwidth,srcheight,destwidth,destheight;


	if (rows == 0 && cols == 0)
	{
		copybitmap(dest,src,0,0,0,0,clip,transparency,transparent_color);
		return;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		srcwidth = src->height;
		srcheight = src->width;
		destwidth = dest->height;
		destheight = dest->width;
	}
	else
	{
		srcwidth = src->width;
		srcheight = src->height;
		destwidth = dest->width;
		destheight = dest->height;
	}

	if (rows == 0)
	{
		/* scrolling columns */
		int col,colwidth;
		struct rectangle myclip;


		colwidth = srcwidth / cols;

		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		col = 0;
		while (col < cols)
		{
			int cons,scroll;


			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while (col + cons < cols &&	colscroll[col + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = srcheight - (-scroll) % srcheight;
			else scroll %= srcheight;

			myclip.min_x = col * colwidth;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,0,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,0,scroll - srcheight,&myclip,transparency,transparent_color);

			col += cons;
		}
	}
	else if (cols == 0)
	{
		/* scrolling rows */
		int row,rowheight;
		struct rectangle myclip;


		rowheight = srcheight / rows;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;

		row = 0;
		while (row < rows)
		{
			int cons,scroll;


			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while (row + cons < rows &&	rowscroll[row + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = srcwidth - (-scroll) % srcwidth;
			else scroll %= srcwidth;

			myclip.min_y = row * rowheight;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,0,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - srcwidth,0,&myclip,transparency,transparent_color);

			row += cons;
		}
	}
	else if (rows == 1 && cols == 1)
	{
		/* XY scrolling playfield */
		int scrollx,scrolly,sx,sy;


		if (rowscroll[0] < 0) scrollx = srcwidth - (-rowscroll[0]) % srcwidth;
		else scrollx = rowscroll[0] % srcwidth;

		if (colscroll[0] < 0) scrolly = srcheight - (-colscroll[0]) % srcheight;
		else scrolly = colscroll[0] % srcheight;

		for (sx = scrollx - srcwidth;sx < destwidth;sx += srcwidth)
			for (sy = scrolly - srcheight;sy < destheight;sy += srcheight)
				copybitmap(dest,src,0,0,sx,sy,clip,transparency,transparent_color);
	}
	else if (rows == 1)
	{
		/* scrolling columns + horizontal scroll */
		int col,colwidth;
		int scrollx;
		struct rectangle myclip;


		if (rowscroll[0] < 0) scrollx = srcwidth - (-rowscroll[0]) % srcwidth;
		else scrollx = rowscroll[0] % srcwidth;

		colwidth = srcwidth / cols;

		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		col = 0;
		while (col < cols)
		{
			int cons,scroll;


			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while (col + cons < cols &&	colscroll[col + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = srcheight - (-scroll) % srcheight;
			else scroll %= srcheight;

			myclip.min_x = col * colwidth + scrollx;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1 + scrollx;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,scrollx,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scrollx,scroll - srcheight,&myclip,transparency,transparent_color);

			myclip.min_x = col * colwidth + scrollx - srcwidth;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1 + scrollx - srcwidth;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,scrollx - srcwidth,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scrollx - srcwidth,scroll - srcheight,&myclip,transparency,transparent_color);

			col += cons;
		}
	}
	else if (cols == 1)
	{
		/* scrolling rows + vertical scroll */
		int row,rowheight;
		int scrolly;
		struct rectangle myclip;


		if (colscroll[0] < 0) scrolly = srcheight - (-colscroll[0]) % srcheight;
		else scrolly = colscroll[0] % srcheight;

		rowheight = srcheight / rows;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;

		row = 0;
		while (row < rows)
		{
			int cons,scroll;


			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while (row + cons < rows &&	rowscroll[row + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = srcwidth - (-scroll) % srcwidth;
			else scroll %= srcwidth;

			myclip.min_y = row * rowheight + scrolly;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1 + scrolly;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,scrolly,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - srcwidth,scrolly,&myclip,transparency,transparent_color);

			myclip.min_y = row * rowheight + scrolly - srcheight;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1 + scrolly - srcheight;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,scrolly - srcheight,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - srcwidth,scrolly - srcheight,&myclip,transparency,transparent_color);

			row += cons;
		}
	}
}


/* fill a bitmap using the specified pen */
void fillbitmap(struct osd_bitmap *dest,int pen,const struct rectangle *clip)
{
	int sx,sy,ex,ey,y;
	struct rectangle myclip;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		if (clip)
		{
			myclip.min_x = clip->min_y;
			myclip.max_x = clip->max_y;
			myclip.min_y = clip->min_x;
			myclip.max_y = clip->max_x;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		if (clip)
		{
			int temp;


			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	sx = 0;
	ex = dest->width - 1;
	sy = 0;
	ey = dest->height - 1;

	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	/* ASG 980211 */
	if (dest->depth == 16)
	{
		if ((pen >> 8) == (pen & 0xff))
		{
			for (y = sy;y <= ey;y++)
				memset(&dest->line[y][sx*2],pen&0xff,(ex-sx+1)*2);
		}
		else
		{
			for (y = sy;y <= ey;y++)
			{
				unsigned short *p = (unsigned short *)&dest->line[y][sx*2];
				int x;
				for (x = sx;x <= ex;x++)
					*p++ = pen;
			}
		}
	}
	else
	{
		for (y = sy;y <= ey;y++)
			memset(&dest->line[y][sx],pen,ex-sx+1);
	}
}


void drawgfxzoom( struct osd_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scalex, int scaley)
{
	struct rectangle myclip;


	/* only support TRANSPARENCY_PEN and TRANSPARENCY_COLOR */
	if (transparency != TRANSPARENCY_PEN && transparency != TRANSPARENCY_COLOR)
		return;

	if (transparency == TRANSPARENCY_COLOR)
		transparent_color = Machine->pens[transparent_color];


	/*
	scalex and scaley are 16.16 fixed point numbers
	1<<15 : shrink to 50%
	1<<16 : uniform scale
	1<<17 : double to 200%
	*/


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		temp = scalex;
		scalex = scaley;
		scaley = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest_bmp->width - ((gfx->width * scalex + 0x7fff) >> 16) - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest_bmp->width-1 - clip->max_x;
			myclip.max_x = dest_bmp->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest_bmp->height - ((gfx->height * scaley + 0x7fff) >> 16) - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest_bmp->height-1 - clip->max_y;
			myclip.max_y = dest_bmp->height-1 - temp;
			clip = &myclip;
		}
	}


	/* ASG 980209 -- added 16-bit version */
	if (dest_bmp->depth != 16)
	{
		if( gfx && gfx->colortable )
		{
			const unsigned short *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
			struct osd_bitmap *source_bmp = gfx->gfxdata;
			int source_base = (code % gfx->total_elements) * gfx->height;

			int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
			int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;

			/* compute sprite increment per screen pixel */
			int dx = (gfx->width<<16)/sprite_screen_width;
			int dy = (gfx->height<<16)/sprite_screen_height;

			int ex = sx+sprite_screen_width;
			int ey = sy+sprite_screen_height;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = (sprite_screen_width-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = (sprite_screen_height-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
				int y;

				/* case 1: TRANSPARENCY_PEN */
				if (transparency == TRANSPARENCY_PEN)
				{
					for( y=sy; y<ey; y++ )
					{
						unsigned char *source = source_bmp->line[source_base+(y_index>>16)];
						unsigned char *dest = dest_bmp->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if( c != transparent_color ) dest[x] = pal[c];
							x_index += dx;
						}

						y_index += dy;
					}
				}

				/* case 2: TRANSPARENCY_COLOR */
				else if (transparency == TRANSPARENCY_COLOR)
				{
					for( y=sy; y<ey; y++ )
					{
						unsigned char *source = source_bmp->line[source_base+(y_index>>16)];
						unsigned char *dest = dest_bmp->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = pal[source[x_index>>16]];
							if( c != transparent_color ) dest[x] = c;
							x_index += dx;
						}

						y_index += dy;
					}
				}
			}

		}
	}

	/* ASG 980209 -- new 16-bit part */
	else
	{
		if( gfx && gfx->colortable )
		{
			const unsigned short *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
			struct osd_bitmap *source_bmp = gfx->gfxdata;
			int source_base = (code % gfx->total_elements) * gfx->height;

			int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
			int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;

			/* compute sprite increment per screen pixel */
			int dx = (gfx->width<<16)/sprite_screen_width;
			int dy = (gfx->height<<16)/sprite_screen_height;

			int ex = sx+sprite_screen_width;
			int ey = sy+sprite_screen_height;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = (sprite_screen_width-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = (sprite_screen_height-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
				int y;

				/* case 1: TRANSPARENCY_PEN */
				if (transparency == TRANSPARENCY_PEN)
				{
					for( y=sy; y<ey; y++ )
					{
						unsigned char *source = source_bmp->line[source_base+(y_index>>16)];
						unsigned short *dest = (unsigned short *)dest_bmp->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if( c != transparent_color ) dest[x] = pal[c];
							x_index += dx;
						}

						y_index += dy;
					}
				}

				/* case 2: TRANSPARENCY_COLOR */
				else if (transparency == TRANSPARENCY_COLOR)
				{
					for( y=sy; y<ey; y++ )
					{
						unsigned char *source = source_bmp->line[source_base+(y_index>>16)];
						unsigned short *dest = (unsigned short *)dest_bmp->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = pal[source[x_index>>16]];
							if( c != transparent_color ) dest[x] = c;
							x_index += dx;
						}

						y_index += dy;
					}
				}
			}
		}
	}
}
