/***************************************************************************

  gryzor: vidhrdw.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *contra_fg_vertical_scroll;
unsigned char *contra_fg_horizontal_scroll;
unsigned char *contra_bg_vertical_scroll;
unsigned char *contra_bg_horizontal_scroll;
unsigned char *contra_fg_vram,*contra_fg_cram;
int contra_fg_vram_size;
unsigned char *contra_text_vram,*contra_text_cram;
int contra_text_vram_size;
unsigned char *contra_bg_vram,*contra_bg_cram;
int contra_bg_vram_size;

static struct osd_bitmap *fgbitmap,*bgbitmap;
static unsigned char *fgdirtybuffer,*bgdirtybuffer;
static int fg_palette_bank,bg_palette_bank;
static int flipscreen;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Contra has palette RAM, but it also has two lookup table PROMs.
  Exact associations are unknown.

***************************************************************************/
void contra_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,bank;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* fg and bg tiles use colors 16-31, 48-63, 80-95 and 112-127, in 4 banks */
	for (i = 0;i < TOTAL_COLORS(0) / 4;i++)
	{
		for (bank = 0;bank < 4;bank++)
		{
			if (i % 16 == 0)
				COLOR(0,i + bank * (TOTAL_COLORS(0) / 4)) = 0;	/* transparent */
			else
				COLOR(0,i + bank * (TOTAL_COLORS(0) / 4)) =
						(*color_prom & 0x0f) + (2 * bank + 1) * 16;
		}

		color_prom++;
	}

	/* the bottom half of the first PROM seems to be unused, or if it's */
	/* used it has no effect because it contains the same color combination */
	color_prom += 128;

	for (i = 0;i < TOTAL_COLORS(1) / 4;i++)
	{
		for (bank = 0;bank < 4;bank++)
		{
			COLOR(1,i + bank * (TOTAL_COLORS(1) / 4)) =
					(*color_prom & 0x0f) + (2 * bank + 1) * 16;
		}

		color_prom++;
	}

	/* the bottom half of the second PROM is probably unused as well */
	color_prom += 128;
}



int contra_vh_start(void)
{
	if ((bgbitmap = osd_create_bitmap(256,256)) == 0)
		return 1;

	if ((fgbitmap = osd_create_bitmap(256,256)) == 0)
	{
		osd_free_bitmap(bgbitmap);
		return 1;
	}

	if ((fgdirtybuffer = malloc(contra_fg_vram_size)) == 0)
	{
		osd_free_bitmap(bgbitmap);
		osd_free_bitmap(fgbitmap);
		return 1;
	}
	memset(fgdirtybuffer,1,contra_fg_vram_size);

	if ((bgdirtybuffer = malloc(contra_bg_vram_size)) == 0)
	{
		free(fgdirtybuffer);
		osd_free_bitmap(bgbitmap);
		osd_free_bitmap(fgbitmap);
		return 1;
	}
	memset(bgdirtybuffer,1,contra_bg_vram_size);

	return 0;
}


void contra_vh_stop(void)
{
	osd_free_bitmap(bgbitmap);
	osd_free_bitmap(fgbitmap);
	free(fgdirtybuffer);
	free(bgdirtybuffer);
}



void contra_fg_vram_w(int offset,int data)
{
	if (contra_fg_vram[offset] != data)
	{
		fgdirtybuffer[offset] = 1;
		contra_fg_vram[offset] = data;
	}
}

void contra_fg_cram_w(int offset,int data)
{
	if (contra_fg_cram[offset] != data)
	{
		fgdirtybuffer[offset] = 1;
		contra_fg_cram[offset] = data;
	}
}

void contra_bg_vram_w(int offset,int data)
{
	if (contra_bg_vram[offset] != data)
	{
		bgdirtybuffer[offset] = 1;
		contra_bg_vram[offset] = data;
	}
}

void contra_bg_cram_w(int offset,int data)
{
	if (contra_bg_cram[offset] != data)
	{
		bgdirtybuffer[offset] = 1;
		contra_bg_cram[offset] = data;
	}
}



void contra_0007_w(int offset,int data)
{
	if (flipscreen != (data & 0x08))
	{
		flipscreen = data & 0x08;
		memset(fgdirtybuffer,1,contra_fg_vram_size);
		memset(bgdirtybuffer,1,contra_bg_vram_size);
	}
}



void contra_fg_palette_bank_w(int offset,int data)
{
	if (fg_palette_bank != ((data & 0x30) >> 4))
	{
		fg_palette_bank = ((data & 0x30) >> 4);
		memset(fgdirtybuffer,1,contra_fg_vram_size);
	}
}

void contra_bg_palette_bank_w(int offset,int data)
{
	if (bg_palette_bank != ((data & 0x30) >> 4))
	{
		bg_palette_bank = ((data & 0x30) >> 4);
		memset(bgdirtybuffer,1,contra_bg_vram_size);
	}
}



static void draw_sprite( struct osd_bitmap *bitmap, struct GfxElement *gfx, int tile_number, int color, int sx, int sy, int xflip, int yflip, int big)
{
	int x,y;

	int size = big?2:1;

	for( y=0; y<size; y++ ){
		for( x=0; x<size; x++ ){
			int ex = xflip?(size-1-x):x;
			int ey = yflip?(size-1-y):y;

			drawgfx(bitmap,gfx,
				ey*2+ex+tile_number,
				color,
				xflip,yflip,
				5*8+sx+x*16,sy+y*16,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

static void draw_sprites( struct osd_bitmap *bitmap, int bank )
{
	struct GfxElement *gfx = Machine->gfx[3-bank];
	const unsigned char *source = spriteram + bank*0x2000;
	const unsigned char *finish = source+40*5;
	int base_color = bank?2:0;


	while( source<finish ){
		int attributes = source[4];
		if( 1 || attributes&0x80 ){ /* sprite enable? */
			int sx = source[3]; /* vertical position */
			int sy = source[2]; /* horizontal position */
			int tile_number = source[0];
			int tile_bank =  source[1];
			int color = (tile_bank&0x10)? base_color:base_color;
			int big = attributes&0x08;
			int yflip = attributes&0x20; /* horizontal flip */
			int xflip = attributes&0x10; /* vertical flip */
			if( attributes&0x01 ) sx -= 256;

			if( tile_bank & 0x08 ) sy -= 8; /* adjust sprite horizontal position */

			tile_bank &= 0x3;
			if( attributes&0x40 ) tile_bank += 4;

			tile_number += tile_bank*256;
			draw_sprite( bitmap, gfx, tile_number, color, sx, sy, xflip, yflip, big );
		}
		source += 5;
	}
}



void contra_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	for (offs = contra_bg_vram_size - 1;offs >= 0;offs--)
	{
		if (bgdirtybuffer[offs])
		{
			int sy,sx,attr,bank,num;


			bgdirtybuffer[offs] = 0;
			sx = offs % 32;
			sy = offs / 32;

			attr = contra_bg_cram[offs];
			bank = (attr & 0xf8) >> 3;
			num = contra_bg_vram[offs];

			/* rotate bank bits - tiles don't map linearly */
			bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);

			drawgfx(bgbitmap,Machine->gfx[1],
					num + bank * 256,
					(attr & 0x07) + 8 * bg_palette_bank,
					0,0, /* no flip */
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;

		/* draw background tiles */
		scrollx = 5*8 - *contra_bg_vertical_scroll;
		scrolly = -*contra_bg_horizontal_scroll;

		copyscrollbitmap(bitmap,bgbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	for (offs = contra_fg_vram_size - 1;offs >= 0;offs--)
	{
		if (fgdirtybuffer[offs])
		{
			int sy,sx,attr,bank,num;


			fgdirtybuffer[offs] = 0;
			sx = offs % 32;
			sy = offs / 32;

			attr = contra_fg_cram[offs];
			bank = (attr & 0xf8) >> 3;
			num = contra_fg_vram[offs];

			/* rotate bank bits - tiles don't map linearly */
			bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);

			drawgfx(fgbitmap,Machine->gfx[0],
					num + bank * 256,
					(attr & 0x07) + 8 * fg_palette_bank,
					0,0, /* no flip */
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;

		/* draw background tiles */
		scrollx = 5*8 - *contra_fg_vertical_scroll;
		scrolly = -*contra_fg_horizontal_scroll;

		copyscrollbitmap(bitmap,fgbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	draw_sprites( bitmap, 0 ); /* enemies */
	draw_sprites( bitmap, 1 ); /* players */


	/* draw the score */
	for (offs = contra_text_vram_size - 1;offs >= 0;offs--)
	{
		int sy,sx,attr,bank,num;


		sx = offs % 32;
		sy = offs / 32;
		if (sx < 5)	/* only draw top 5 rows */
		{
			attr = contra_text_cram[offs];
			bank = (attr & 0xf8) >> 3;
			num = contra_text_vram[offs];

			/* rotate bank bits - tiles don't map linearly */
			bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);

			drawgfx(bitmap,Machine->gfx[0],
					num + bank * 256,
					attr & 0x07,
					0,0, /* no flip */
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
}
