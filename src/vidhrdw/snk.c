/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define SPRITES_16X16		0x010
#define SPRITES_32X32		0x020
#define BG_VIDEORAM_SIZE	0x800
#define BG_SCROLL_X		0x880
#define BG_SCROLL_Y		0x800
#define BG_SCROLLHI_XY		0x900
#define SPR16_SCROLL_X		0xA80
#define SPR16_SCROLL_Y		0xA00
#define SPR32_SCROLL_X		0xB80
#define SPR32_SCROLL_Y		0xB00
#define SPR1632_SCROLLHI_XY	0xD00

static int bg_scrollx,   bg_scrolly;
static int spr16_scrollx, spr16_scrolly;
static int spr32_scrollx, spr32_scrolly;

unsigned char *bg_dirtybuffer;
unsigned char *bg_videoram;
unsigned char *fg_videoram;
unsigned char *sprite16x16_ram;
unsigned char *sprite32x32_ram;

static struct osd_bitmap *bitmap_bg;		/* background bitmap */
static struct osd_bitmap *bitmap_sp1;		/* sprites bitmap 32x32 */
static struct osd_bitmap *bitmap_sp2;		/* sprites bitmap 16x16 A*/
static struct osd_bitmap *bitmap_sp3;		/* sprites bitmap 16x16 B*/

extern unsigned char *snk_hrdwmem;
extern unsigned char *snk_sharedram;
extern int bg_video_offs;






/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/

void snk_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = (color_prom[1*1024+i] >> 0) & 0x01;
		bit1 = (color_prom[1*1024+i] >> 1) & 0x01;
		bit2 = (color_prom[1*1024+i] >> 2) & 0x01;
		bit3 = (color_prom[1*1024+i] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */
		bit0 = (color_prom[2*1024+i] >> 0) & 0x01;
		bit1 = (color_prom[2*1024+i] >> 1) & 0x01;
		bit2 = (color_prom[2*1024+i] >> 2) & 0x01;
		bit3 = (color_prom[2*1024+i] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* characters */
	for (i = 0;i < 16+16;i++)
		COLOR(0,i) = 256+128+i;

	/* tiles */
	for (i = 0;i < 256;i++)
		COLOR(1,i) = 256+i;

	/* Sprites 1 & 2 */
	for (i = 0;i < 128;i++)
	{
		COLOR(2,i) = i;
		COLOR(3,i) = i+128;
	}

	/* Since sprites in SNK games are using pen 6 as (alpha-channel transparent pen)
	   & we currently cannot support that, I've set all that pens as black color */

	for (i = 0;i < 256;i++)
	{
		if ((i % 8)==6)
		{
			*(palette-(Machine->drv->total_colors*3)+i*3+0) = 14;
			*(palette-(Machine->drv->total_colors*3)+i*3+1) = 14;
			*(palette-(Machine->drv->total_colors*3)+i*3+2) = 14;
		}
	}
}


/*************************************
 *
 *		Start/Stop
 *
 *************************************/

int snk_vh_start(void)
{
	if ((bg_dirtybuffer = malloc (BG_VIDEORAM_SIZE / 2)) == 0)
	{
		free (bg_dirtybuffer);
		return 1;
	}
	if ((bitmap_bg = osd_new_bitmap (512, 512, Machine->scrbitmap->depth)) == 0)
	{
		free (bg_dirtybuffer);
		return 1;
	}
	if ((bitmap_sp1 = osd_new_bitmap (512, 512, Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap (bitmap_bg);
		free (bg_dirtybuffer);
		return 1;
	}
	if ((bitmap_sp2 = osd_new_bitmap (512,512,Machine->scrbitmap->depth)) == 0)
	{
		free (bg_dirtybuffer);
		osd_free_bitmap (bitmap_bg);
		osd_free_bitmap (bitmap_sp1);
		return 1;
	}
	if ((bitmap_sp3 = osd_new_bitmap (512,512,Machine->scrbitmap->depth)) == 0)
	{
		free (bg_dirtybuffer);
		osd_free_bitmap (bitmap_bg);
		osd_free_bitmap (bitmap_sp1);
		osd_free_bitmap (bitmap_sp2);
		return 1;
	}
	memset (bg_dirtybuffer, 1 , BG_VIDEORAM_SIZE / 2);

	memset(palette_used_colors,PALETTE_COLOR_USED,Machine->drv->total_colors);

	fillbitmap(bitmap_sp1,palette_transparent_pen,0);
	fillbitmap(bitmap_sp2,palette_transparent_pen,0);
	fillbitmap(bitmap_sp3,palette_transparent_pen,0);

	bg_videoram = &snk_sharedram[bg_video_offs]; 	/* background ram */
	fg_videoram = &snk_sharedram[0x2800];		/* foreground ram */
	sprite16x16_ram = &snk_sharedram[0x1800]; 	/* sprites ram */
	sprite32x32_ram = &snk_sharedram[0x1000];

	return 0;

}

void snk_vh_stop(void)
{
	free (bg_dirtybuffer);
	osd_free_bitmap (bitmap_bg);
	osd_free_bitmap (bitmap_sp1);
	osd_free_bitmap (bitmap_sp2);
	osd_free_bitmap (bitmap_sp3);
}


/*************************************
 *
 *	Sprites rendering
 *
 *************************************/

void render_sprite(struct osd_bitmap *bitmap, int size ,int x, int y, int tile, int color)
{
	struct rectangle clip;
	clip.min_x = 0;
	clip.min_y = 0;
	clip.max_x = 511;
	clip.max_y = 511;

	if (size==SPRITES_16X16)
	{
		drawgfx(bitmap,Machine->gfx[2],
					tile,
					color,
					0,0,
					x,y,
					&clip,
					TRANSPARENCY_PEN, 7);

		if (x>=496)
			x = -16+(x-496);
		if (y>=496)
			y = -16+(y-496);

		drawgfx(bitmap,Machine->gfx[2],
					tile,
					color,
					0,0,
					x,y,
					&clip,
					TRANSPARENCY_PEN, 7);

	} else
	{
		drawgfx(bitmap,Machine->gfx[3],
					tile,
					color,
					0,0,
					x,y,
					&clip,
					TRANSPARENCY_PEN, 7);

		if (x>=480)
			x = -32+(x-480);
		if (y>=480)
			y = -32+(y-480);

		drawgfx(bitmap,Machine->gfx[3],
					tile,
					color,
					0,0,
					x,y,
					&clip,
					TRANSPARENCY_PEN, 7);
	}

}

void snk_render_sprites(struct osd_bitmap *bitmap,int start,int count,int size)
{
	int offs,sx,sy,tile,palette;

	if (size == SPRITES_16X16)
	{
		for (offs = start*4; offs < (start+count)*4; offs+=4)
		{
			if (!(sprite16x16_ram[offs]==0xff && sprite16x16_ram[offs+1]==0xff &&
			    sprite16x16_ram[offs+2]==0xff && sprite16x16_ram[offs+3]==0xff))
			{
				sx = sprite16x16_ram[offs+2];
				sy = sprite16x16_ram[offs+0];
				if (sprite16x16_ram[offs+3] & 0x80) sx+=256;
				if (sprite16x16_ram[offs+3] & 0x10) sy+=256;
				tile = sprite16x16_ram[offs+1]+(((sprite16x16_ram[offs+3] & 0x60)) << 3);
				palette	= sprite16x16_ram[offs+3] & 0x0f;
				render_sprite(bitmap, SPRITES_16X16, 511-sx, sy, tile, palette);
			}
		}
	} else
	{
		for (offs = start*4; offs < (start+count)*4; offs+=4)
		{
			if (!(sprite32x32_ram[offs]==0xff && sprite32x32_ram[offs+1]==0xff &&
			    sprite32x32_ram[offs+2]==0xff && sprite32x32_ram[offs+3]==0xff))
			{
				sx = sprite32x32_ram[offs+2];
				sy = sprite32x32_ram[offs+0];
				if (sprite32x32_ram[offs+3] & 0x80) sx+=256;
				if (sprite32x32_ram[offs+3] & 0x10) sy+=256;
				tile = sprite32x32_ram[offs+1]+((sprite32x32_ram[offs+3] & 0x40) << 2);
				palette	= sprite32x32_ram[offs+3] & 0x0f;
				render_sprite(bitmap, SPRITES_32X32, 511-sx, sy, tile, palette);
			}
		}
	}
}

void clear_sprite(struct osd_bitmap *bitmap, int x, int y)
{
	struct rectangle clip;

	clip.min_x = x;
	clip.min_y = y;
	clip.max_x = x+31;
	clip.max_y = y+31;

	if (x+31 > 511)
		clip.max_x = 511;
	if (y+31 > 511)
		clip.max_y = 511;

	fillbitmap(bitmap,palette_transparent_pen,&clip);

	if (x+31 > 511)
	{
		clip.max_x = (x+31)-511;
		clip.min_x = 0;
	}
	if (y+31 > 511)
	{
		clip.max_y = (y+31)-511;
		clip.min_y = 0;
	}

	fillbitmap(bitmap,palette_transparent_pen,&clip);
}

void snk_clear_sprites(struct osd_bitmap *bitmap,int start,int count,int size)
{
	int offs,sx,sy,tile,palette;

	if (size == SPRITES_16X16)
	{
		for (offs = start*4; offs < (start+count)*4; offs+=4)
		{
			if (!(sprite16x16_ram[offs]==0xff && sprite16x16_ram[offs+1]==0xff &&
			    sprite16x16_ram[offs+2]==0xff && sprite16x16_ram[offs+3]==0xff))
			{
				sx = sprite16x16_ram[offs+2];
				sy = sprite16x16_ram[offs+0];
				if (sprite16x16_ram[offs+3] & 0x80) sx+=256;
				if (sprite16x16_ram[offs+3] & 0x10) sy+=256;
				clear_sprite(bitmap, 511-sx, sy );
			}
		}
	} else
	{
		for (offs = start*4; offs < (start+count)*4; offs+=4)
		{
			if (!(sprite32x32_ram[offs]==0xff && sprite32x32_ram[offs+1]==0xff &&
			    sprite32x32_ram[offs+2]==0xff && sprite32x32_ram[offs+3]==0xff))
			{
				sx = sprite32x32_ram[offs+2];
				sy = sprite32x32_ram[offs+0];
				if (sprite32x32_ram[offs+3] & 0x80) sx+=256;
				if (sprite32x32_ram[offs+3] & 0x10) sy+=256;
				clear_sprite(bitmap, 511-sx, sy );
			}
		}
	}
}


/*************************************
 *
 *		Foreground rendering
 *
 *************************************/

void snk_render_foreground(struct osd_bitmap *bitmap)
{
	int x,y,offs;
	int sx,sy,tile;

	for (x = 31; x >=0; x--)
	{
		for (y = 0; y < 2; y++)
		{
			offs = y*32+x;
			sx   = y << 3;
			sy   = (x << 3) + 8;
			tile = fg_videoram[offs+0x7c0];
			drawgfx(bitmap,Machine->gfx[0],
						tile,
						0,
						0,0,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE, 0);
		}
	}

	for (x = 31; x >=0; x--)
	{
		for (y = 0; y < 32; y++)
		{
			offs = y*32+x;
			sx   = y << 3;
			sy   = (x << 3) + 8;
			tile = fg_videoram[offs];
			if (tile!=0x20)
			drawgfx(bitmap,Machine->gfx[0],
						tile,
						0,
						0,0,
						sx+16,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN, 15);
		}
	}

	for (x = 31; x >=0; x--)
	{
		for (y = 0; y < 2; y++)
		{
			offs = y*32+x;
			sx   = y << 3;
			sy   = (x << 3) + 8;
			tile = fg_videoram[offs+0x400];

			drawgfx(bitmap,Machine->gfx[0],
						tile,
						0,
						0,0,
						sx+272,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE, 0);
		}
	}
}

/*************************************
 *
 *		Background rendering
 *
 *************************************/

void snk_render_background(struct osd_bitmap *bitmap)
{
	int x,y,offs;
	int sx,sy,tile,palette;

	for (x = 31; x >=0; x--)
	{
		for (y = 0; y < 32; y++)
		{
			offs = y*64+x*2;
			if (bg_dirtybuffer[offs >> 1])
			{
				sx 	= y << 4;
				sy 	= x << 4;
				tile 	= bg_videoram[offs]+((bg_videoram[offs+1] & 15) * 256);
				palette = bg_videoram[offs+1] >> 4;
				bg_dirtybuffer[offs >> 1] = 0;

				drawgfx(bitmap,Machine->gfx[1],
							tile,
							palette,
							0,0,
							sx,sy,
							0,TRANSPARENCY_NONE, 0);
			}
		}
	}
}

/*************************************
 *
 *	Master update function
 *
 *************************************/

void snk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	bg_scrollx	= -snk_hrdwmem[BG_SCROLL_X]+13;
	bg_scrolly	= -snk_hrdwmem[BG_SCROLL_Y]+8;
	spr16_scrollx	= snk_hrdwmem[SPR16_SCROLL_X]-227;
	spr16_scrolly	= -snk_hrdwmem[SPR16_SCROLL_Y]-9;
	spr32_scrollx	= snk_hrdwmem[SPR32_SCROLL_X]-243;
	spr32_scrolly	= -snk_hrdwmem[SPR32_SCROLL_Y]-25;

	if (snk_hrdwmem[BG_SCROLLHI_XY] & 0x01) bg_scrolly -= 256;
	if (snk_hrdwmem[BG_SCROLLHI_XY] & 0x02) bg_scrollx -= 256;
	if (snk_hrdwmem[SPR1632_SCROLLHI_XY] & 0x04) spr16_scrolly -= 256;
	if (snk_hrdwmem[SPR1632_SCROLLHI_XY] & 0x10) spr16_scrollx += 256;
	if (snk_hrdwmem[SPR1632_SCROLLHI_XY] & 0x08) spr32_scrolly -= 256;
	if (snk_hrdwmem[SPR1632_SCROLLHI_XY] & 0x20) spr32_scrollx += 256;

	/* recompute */
	if (palette_recalc ())
		memset (bg_dirtybuffer, 1, BG_VIDEORAM_SIZE / 2);

	snk_render_background(bitmap_bg);
	snk_render_sprites(bitmap_sp1, 0, 25, SPRITES_16X16);
	snk_render_sprites(bitmap_sp2, 0, 25, SPRITES_32X32);
	snk_render_sprites(bitmap_sp3,25, 25, SPRITES_16X16);

	copyscrollbitmap(bitmap,bitmap_bg,1,&bg_scrollx,1,&bg_scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copyscrollbitmap(bitmap,bitmap_sp1,1,&spr16_scrollx,1,&spr16_scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	copyscrollbitmap(bitmap,bitmap_sp2,1,&spr32_scrollx,1,&spr32_scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	copyscrollbitmap(bitmap,bitmap_sp3,1,&spr16_scrollx,1,&spr16_scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	snk_render_foreground(bitmap);

	snk_clear_sprites(bitmap_sp1, 0, 25, SPRITES_16X16);
	snk_clear_sprites(bitmap_sp2, 0, 25, SPRITES_32X32);
	snk_clear_sprites(bitmap_sp3,25, 25, SPRITES_16X16);

}

