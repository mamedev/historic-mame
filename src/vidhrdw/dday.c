/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *dday_videoram2;
unsigned char *dday_videoram3;
static int control = 0;

static unsigned char *searchlight_image;
static int searchlight_flipx;
static int searchlight_enable = 0;

void dday_sound_enable(int enabled);


/* LBO */
#ifdef LSB_FIRST
#define intelLong(x) (x)
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
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


static void drawgfx_shadow(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,
		unsigned char* shadow_mask, int mask_flip,
		unsigned char* layer_mask, int layer)
{
	int ox,oy,ex,ey,y,start;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	int col;
	int *sd4;
	int trans4,col4;
	int f,shadow=0,l;


	code %= gfx->total_elements;
	color %= gfx->total_colors;


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	start = code * gfx->height + (sy-oy);

	if (gfx->colortable)	/* remap colors */
	{
		const unsigned short *paldata;	/* ASG 980209 */

		paldata = &gfx->colortable[gfx->color_granularity * color];

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				if (flipx)	/* X flip */
				{
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						if (mask_flip)
						{
							for( bm += sx ; bm <= bme-7 ; bm+=8)
							{
								shadow = *(shadow_mask++);
								l = *(layer_mask++);

								if (((l & 0x01) >> 0) == layer)  bm[0] = paldata[sd[0]+((shadow & 0x80) << 1)];
								if (((l & 0x02) >> 1) == layer)  bm[1] = paldata[sd[1]+((shadow & 0x40) << 2)];
								if (((l & 0x04) >> 2) == layer)  bm[2] = paldata[sd[2]+((shadow & 0x20) << 3)];
								if (((l & 0x08) >> 3) == layer)  bm[3] = paldata[sd[3]+((shadow & 0x10) << 4)];
								if (((l & 0x10) >> 4) == layer)  bm[4] = paldata[sd[4]+((shadow & 0x08) << 5)];
								if (((l & 0x20) >> 5) == layer)  bm[5] = paldata[sd[5]+((shadow & 0x04) << 6)];
								if (((l & 0x40) >> 6) == layer)  bm[6] = paldata[sd[6]+((shadow & 0x02) << 7)];
								if (((l & 0x80) >> 7) == layer)  bm[7] = paldata[sd[7]+((shadow & 0x01) << 8)];
								sd+=8;
							}
						}
						else
						{
							for( bm += sx ; bm <= bme-7 ; bm+=8)
							{
								shadow = *(shadow_mask++);
								l = *(layer_mask++);

								if (((l & 0x01) >> 0) == layer)  bm[0] = paldata[sd[0]+((shadow & 0x01) << 8)];
								if (((l & 0x02) >> 1) == layer)  bm[1] = paldata[sd[1]+((shadow & 0x02) << 7)];
								if (((l & 0x04) >> 2) == layer)  bm[2] = paldata[sd[2]+((shadow & 0x04) << 6)];
								if (((l & 0x08) >> 3) == layer)  bm[3] = paldata[sd[3]+((shadow & 0x08) << 5)];
								if (((l & 0x10) >> 4) == layer)  bm[4] = paldata[sd[4]+((shadow & 0x10) << 4)];
								if (((l & 0x20) >> 5) == layer)  bm[5] = paldata[sd[5]+((shadow & 0x20) << 3)];
								if (((l & 0x40) >> 6) == layer)  bm[6] = paldata[sd[6]+((shadow & 0x40) << 2)];
								if (((l & 0x80) >> 7) == layer)  bm[7] = paldata[sd[7]+((shadow & 0x80) << 1)];
								sd+=8;
							}
						}
						start+=1;
					}
				}
				break;

			case TRANSPARENCY_PEN:
				trans4 = transparent_color * 0x01010101;
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd4 = (int *)(gfx->gfxdata->line[start] + gfx->width -1 - (sx-ox) -3);
						f = 0;
						if (mask_flip)
						{
							for( bm += sx ; bm <= bme-3 ; bm+=4, f^=1 )
							{
								if (f)
								{
									shadow <<= 4;
								}
								else
								{
									shadow = *(shadow_mask++);
								}
								if ((col4=read_dword(sd4)) != trans4){
									col = (col4>>24)&0xff;
									if (col != transparent_color) bm[BL0] = paldata[col+((shadow & 0x80) << 1)];
									col = (col4>>16)&0xff;
									if (col != transparent_color) bm[BL1] = paldata[col+((shadow & 0x40) << 2)];
									col = (col4>>8)&0xff;
									if (col != transparent_color) bm[BL2] = paldata[col+((shadow & 0x20) << 3)];
									col = col4&0xff;
									if (col != transparent_color) bm[BL3] = paldata[col+((shadow & 0x10) << 4)];
								}
								sd4--;
							}
						}
						else
						{
							for( bm += sx ; bm <= bme-3 ; bm+=4, f^=1 )
							{
								if (f)
								{
									shadow >>= 4;
								}
								else
								{
									shadow = *(shadow_mask++);
								}
								if ((col4=read_dword(sd4)) != trans4){
									col = (col4>>24)&0xff;
									if (col != transparent_color) bm[BL0] = paldata[col+((shadow & 0x01) << 8)];
									col = (col4>>16)&0xff;
									if (col != transparent_color) bm[BL1] = paldata[col+((shadow & 0x02) << 7)];
									col = (col4>>8)&0xff;
									if (col != transparent_color) bm[BL2] = paldata[col+((shadow & 0x04) << 6)];
									col = col4&0xff;
									if (col != transparent_color) bm[BL3] = paldata[col+((shadow & 0x08) << 5)];
								}
								sd4--;
							}
						}
						start+=1;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd4 = (int *)(gfx->gfxdata->line[start] + (sx-ox));
						f = 0;
						if (mask_flip)
						{
							for( bm += sx ; bm <= bme-3 ; bm+=4, f^=1 )
							{
								if (f)
								{
									shadow <<= 4;
								}
								else
								{
									shadow = *(shadow_mask++);
								}
								if ((col4=read_dword(sd4)) != trans4){
									col = col4&0xff;
									if (col != transparent_color) bm[BL0] = paldata[col+((shadow & 0x80) << 1)];
									col = (col4>>8)&0xff;
									if (col != transparent_color) bm[BL1] = paldata[col+((shadow & 0x40) << 2)];
									col = (col4>>16)&0xff;
									if (col != transparent_color) bm[BL2] = paldata[col+((shadow & 0x20) << 3)];
									col = (col4>>24)&0xff;
									if (col != transparent_color) bm[BL3] = paldata[col+((shadow & 0x10) << 4)];
								}
								sd4++;
							}
						}
						else
						{
							for( bm += sx ; bm <= bme-3 ; bm+=4, f^=1 )
							{
								if (f)
								{
									shadow >>= 4;
								}
								else
								{
									shadow = *(shadow_mask++);
								}
								if ((col4=read_dword(sd4)) != trans4){
									col = col4&0xff;
									if (col != transparent_color) bm[BL0] = paldata[col+((shadow & 0x01) << 8)];
									col = (col4>>8)&0xff;
									if (col != transparent_color) bm[BL1] = paldata[col+((shadow & 0x02) << 7)];
									col = (col4>>16)&0xff;
									if (col != transparent_color) bm[BL2] = paldata[col+((shadow & 0x04) << 6)];
									col = (col4>>24)&0xff;
									if (col != transparent_color) bm[BL3] = paldata[col+((shadow & 0x08) << 5)];
								}
								sd4++;
							}
						}
						start+=1;
					}
				}
				break;
		}
	}
}


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void dday_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,total;

	total = Machine->drv->total_colors/2;

	for (i = 0;i < total;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[total] >> 0) & 0x01;
		bit1 = (color_prom[total] >> 1) & 0x01;
		bit2 = (color_prom[total] >> 2) & 0x01;
		bit3 = (color_prom[total] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*total] >> 0) & 0x01;
		bit1 = (color_prom[2*total] >> 1) & 0x01;
		bit2 = (color_prom[2*total] >> 2) & 0x01;
		bit3 = (color_prom[2*total] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette[0] = r;
		palette[1] = g;
		palette[2] = b;

		/* darker version for searchlight */
		palette[3*256  ] = r >> 3;
		palette[3*256+1] = g >> 3;
		palette[3*256+2] = b >> 3;

		palette += 3;

		color_prom++;
	}


	/* HACK!!! This table is handgenerated, but it matches the screenshot.
	   I have no clue how it really works */

	colortable[0*4+0] = 0;
	colortable[0*4+1] = 1;
	colortable[0*4+2] = 21;
	colortable[0*4+3] = 2;

	colortable[1*4+0] = 4;
	colortable[1*4+1] = 5;
	colortable[1*4+2] = 3;
	colortable[1*4+3] = 7;

	colortable[2*4+0] = 8;
	colortable[2*4+1] = 21;
	colortable[2*4+2] = 10;
	colortable[2*4+3] = 3;

	colortable[3*4+0] = 8;
	colortable[3*4+1] = 21;
	colortable[3*4+2] = 10;
	colortable[3*4+3] = 3;

	colortable[4*4+0] = 16;
	colortable[4*4+1] = 17;
	colortable[4*4+2] = 18;
	colortable[4*4+3] = 7;

	colortable[5*4+0] = 29;
	colortable[5*4+1] = 21;
	colortable[5*4+2] = 22;
	colortable[5*4+3] = 27;

	colortable[6*4+0] = 29;
	colortable[6*4+1] = 21;
	colortable[6*4+2] = 26;
	colortable[6*4+3] = 27;

	colortable[7*4+0] = 29;
	colortable[7*4+1] = 2;
	colortable[7*4+2] = 4;
	colortable[7*4+3] = 27;

	for (i = 0; i < 8*4; i++)
	{
		colortable[i+256] = colortable[i] + 256;
	}
}


void dday_colorram_w(int offset, int data)
{
    colorram[offset & 0x3e0] = data;
}

int dday_colorram_r(int offset)
{
    return colorram[offset & 0x3e0];
}

void dday_searchlight_w(int offset, int data)
{
	searchlight_image = &Machine->memory_region[3][0x200*(data & 0x07)];
	searchlight_flipx = (data >> 3) & 0x01;
}

void dday_control_w(int offset, int data)
{
	//fprintf(errorlog,"Control = %02X\n", data);

	/* Bit 0 is coin counter 1 */
	coin_counter_w(0, data & 0x01);

	/* Bit 1 is coin counter 2 */
	coin_counter_w(1, data & 0x02);

	/* Bit 4 is sound enable */
	dday_sound_enable(data & 0x10);

	/* Bit 6 is search light enable */
	searchlight_enable = data & 0x40;

	control = data;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dday_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int code, code2, sx, sy, flipx, flipx2;
		unsigned char* searchlight_bitmap;
		unsigned char* layer_bitmap;


		sy = (offs / 32);
		sx = (offs % 32);

		flipx2 = 0;
		code = 0;

		/* draw the search light, if enabled */
		if (searchlight_enable)
		{
			flipx2 = (sx >> 4) & 0x01;
			code = searchlight_image[(sy << 4) | (flipx2 ? sx ^ 0x1f: sx)];

			if (searchlight_flipx != flipx2)
			{
				if (code & 0x80)
				{
					/* No mirroring, draw dark spot */
					code = 1;
				}
			}

			code &= 0x3f;
		}

		searchlight_bitmap = &Machine->memory_region[3][0x1000 | (code << 3)];

		sx *= 8;
		sy *= 8;

		/* draw part of background appearing behind the vehicles */
		code2 = videoram[offs];

		layer_bitmap = &Machine->memory_region[4][code2 << 3];

		drawgfx_shadow(bitmap,Machine->gfx[0],
					   code2,
					   code2 >> 5,
					   0,
					   sx,sy,
					   &Machine->drv->visible_area,TRANSPARENCY_NONE,0,
					   searchlight_bitmap,flipx2,
					   layer_bitmap, 1);


		/* draw vehicles */
		flipx  = colorram[sy << 2] & 0x01;
		code = dday_videoram3[flipx ? offs ^ 0x1f : offs];

		if (code)
		{
			drawgfx_shadow(bitmap,Machine->gfx[2],
						   code,
						   code >> 5,
						   flipx,
						   sx,sy,
						   &Machine->drv->visible_area,TRANSPARENCY_PEN,0,
						   searchlight_bitmap,flipx2,
						   0, 0);
		}


		/* draw part of background appearing in front of the vehicles
		   skipping characters totally in the background */
		if ((layer_bitmap[0] != 0xff) ||
			(layer_bitmap[1] != 0xff) ||
			(layer_bitmap[2] != 0xff) ||
			(layer_bitmap[3] != 0xff) ||
			(layer_bitmap[4] != 0xff) ||
			(layer_bitmap[5] != 0xff) ||
			(layer_bitmap[6] != 0xff) ||
			(layer_bitmap[7] != 0xff))
		{
			drawgfx_shadow(bitmap,Machine->gfx[0],
						   code2,
						   code2 >> 5,
						   0,
						   sx,sy,
						   &Machine->drv->visible_area,TRANSPARENCY_NONE,0,
						   searchlight_bitmap,flipx2,
						   layer_bitmap, 0);
		}


		/* draw text layer */
		code = dday_videoram2[offs];

		if (code)
		{
			drawgfx_shadow(bitmap,Machine->gfx[1],
						   code,
						   code >> 5,
						   0,
						   sx,sy,
						   &Machine->drv->visible_area,TRANSPARENCY_PEN,0,
						   searchlight_bitmap,flipx2,
						   0, 0);
		}
	}
}
