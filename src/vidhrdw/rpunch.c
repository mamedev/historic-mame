/***************************************************************************

  vidhrdw/rpunch.c

  Functions to emulate the video hardware of the machine.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define BITMAP_WIDTH	304
#define BITMAP_HEIGHT	224
#define BITMAP_XOFFSET	4


/*************************************
 *
 *	Statics
 *
 *************************************/

data16_t *rpunch_bitmapram;
size_t rpunch_bitmapram_size;
static UINT32 *rpunch_bitmapsum;

int rpunch_sprite_palette;

static struct tilemap *background[2];

static data16_t videoflags;
static UINT8 crtc_register;
static void *crtc_timer;
static UINT8 bins, gins;

static const data16_t *callback_videoram;
static UINT8 callback_gfxbank;
static UINT8 callback_colorbase;
static UINT16 callback_imagebase;
static UINT16 callback_imagemask;


/*************************************
 *
 *	Prototypes
 *
 *************************************/

void rpunch_vh_stop(void);



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_bg_tile_info(int tile_index)
{
	int data = callback_videoram[tile_index];
	SET_TILE_INFO(callback_gfxbank,
			callback_imagebase | (data & callback_imagemask),
			callback_colorbase | ((data >> 13) & 7));
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

static void crtc_interrupt_gen(int param)
{
	cpu_cause_interrupt(0, 1);
	if (param != 0)
		crtc_timer = timer_pulse(TIME_IN_HZ(Machine->drv->frames_per_second * param), 0, crtc_interrupt_gen);
}


int rpunch_vh_start(void)
{
	int i;

	/* allocate tilemaps for the backgrounds and a bitmap for the direct-mapped bitmap */
	background[0] = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     8,8,64,64);
	background[1] = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,64);

	/* allocate a bitmap sum */
	rpunch_bitmapsum = malloc(BITMAP_HEIGHT * sizeof(UINT32));

	/* if anything failed, clean up and return an error */
	if (!background[0] || !background[1] || !rpunch_bitmapsum)
	{
		rpunch_vh_stop();
		return 1;
	}

	/* configure the tilemaps */
	tilemap_set_transparent_pen(background[1],15);

	/* reset the sums and bitmap */
	for (i = 0; i < BITMAP_HEIGHT; i++)
		rpunch_bitmapsum[i] = (BITMAP_WIDTH/4) * 0xffff;
	if (rpunch_bitmapram)
		memset(rpunch_bitmapram, 0xff, rpunch_bitmapram_size);

	/* reset the timer */
	crtc_timer = NULL;
	return 0;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void rpunch_vh_stop(void)
{
	if (rpunch_bitmapsum)
		free(rpunch_bitmapsum);
	rpunch_bitmapsum = NULL;
}



/*************************************
 *
 *	Write handlers
 *
 *************************************/

WRITE16_HANDLER( rpunch_bitmap_w )
{
	if (rpunch_bitmapram)
	{
		int oldword = rpunch_bitmapram[offset];
		int newword = oldword;
		COMBINE_DATA(&newword);

		if (oldword != newword)
		{
			int row = offset / 128;
			int col = 4 * (offset % 128) - BITMAP_XOFFSET;

			rpunch_bitmapram[offset] = data;
			if (row < BITMAP_HEIGHT && col >= 0 && col < BITMAP_WIDTH)
				rpunch_bitmapsum[row] += newword - oldword;
		}
	}
}


WRITE16_HANDLER( rpunch_videoram_w )
{
	int oldword = videoram16[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		int tilemap = offset >> 12;
		int tile_index = offset & 0xfff;

		videoram16[offset] = newword;
		tilemap_mark_tile_dirty(background[tilemap],tile_index);
	}
}


WRITE16_HANDLER( rpunch_videoreg_w )
{
	int oldword = videoflags;
	COMBINE_DATA(&videoflags);

	if (videoflags != oldword)
	{
		/* invalidate tilemaps */
		if ((oldword ^ videoflags) & 0x0410)
			tilemap_mark_all_tiles_dirty(background[0]);
		if ((oldword ^ videoflags) & 0x0820)
			tilemap_mark_all_tiles_dirty(background[1]);
	}
}


WRITE16_HANDLER( rpunch_scrollreg_w )
{
	if (ACCESSING_LSB && ACCESSING_MSB)
		switch (offset)
		{
			case 0:
				tilemap_set_scrolly(background[0], 0, data & 0x1ff);
				break;

			case 1:
				tilemap_set_scrollx(background[0], 0, data & 0x1ff);
				break;

			case 2:
				tilemap_set_scrolly(background[1], 0, data & 0x1ff);
				break;

			case 3:
				tilemap_set_scrollx(background[1], 0, data & 0x1ff);
				break;
		}
}


WRITE16_HANDLER( rpunch_crtc_data_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		switch (crtc_register)
		{
			/* only register we know about.... */
			case 0x0b:
				if (crtc_timer)
					timer_remove(crtc_timer);
				crtc_timer = timer_set(cpu_getscanlinetime(Machine->visible_area.max_y + 1), (data == 0xc0) ? 2 : 1, crtc_interrupt_gen);
				break;

			default:
				logerror("CRTC register %02X = %02X\n", crtc_register, data & 0xff);
				break;
		}
	}
}


WRITE16_HANDLER( rpunch_crtc_register_w )
{
	if (ACCESSING_LSB)
		crtc_register = data & 0xff;
}


WRITE16_HANDLER( rpunch_ins_w )
{
	if (ACCESSING_LSB)
	{
		if (offset == 0)
		{
			gins = data & 0x3f;
			logerror("GINS = %02X\n", data & 0x3f);
		}
		else
		{
			bins = data & 0x3f;
			logerror("BINS = %02X\n", data & 0x3f);
		}
	}
}


/*************************************
 *
 *	Sprite routines
 *
 *************************************/

static void mark_sprite_palette(int start, int stop)
{
	UINT16 used_colors[16];
	int offs, i, j;

	start *= 4;
	stop *= 4;

	memset(used_colors, 0, sizeof(used_colors));
	for (offs = start; offs < stop; offs += 4)
	{
		int data1 = spriteram16[offs + 1];
		int code = data1 & 0x7ff;

		if (code < 0x600)
		{
			int data0 = spriteram16[offs + 0];
			int data2 = spriteram16[offs + 2];
			int x = (data2 & 0x1ff) + 8;
			int y = 513 - (data0 & 0x1ff);
			int color = ((data1 >> 13) & 7) | ((videoflags & 0x0040) >> 3);

			if (x >= BITMAP_WIDTH) x -= 512;
			if (y >= BITMAP_HEIGHT) y -= 512;

			if (x > -16 && y > -32)
				used_colors[color] |= Machine->gfx[2]->pen_usage[code];
		}
	}

	for (i = 0; i < 16; i++)
	{
		UINT16 used = used_colors[i];
		if (used)
		{
			for (j = 0; j < 15; j++)
				if (used & (1 << j))
					palette_used_colors[rpunch_sprite_palette + i * 16 + j] = PALETTE_COLOR_USED;
			palette_used_colors[rpunch_sprite_palette + i * 16 + 15] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}


static void draw_sprites(struct osd_bitmap *bitmap, int start, int stop)
{
	int offs;

	start *= 4;
	stop *= 4;

	/* draw the sprites */
	for (offs = start; offs < stop; offs += 4)
	{
		int data1 = spriteram16[offs + 1];
		int code = data1 & 0x7ff;

		if (code < 0x600 && code != 0)
		{
			int data0 = spriteram16[offs + 0];
			int data2 = spriteram16[offs + 2];
			int x = (data2 & 0x1ff) + 8;
			int y = 513 - (data0 & 0x1ff);
			int xflip = data1 & 0x1000;
			int yflip = data1 & 0x0800;
			int color = ((data1 >> 13) & 7) | ((videoflags & 0x0040) >> 3);

			if (x >= BITMAP_WIDTH) x -= 512;
			if (y >= BITMAP_HEIGHT) y -= 512;

			drawgfx(bitmap, Machine->gfx[2],
					code, color + (rpunch_sprite_palette / 16), xflip, yflip, x, y, 0, TRANSPARENCY_PEN, 15);
		}
	}
}


/*************************************
 *
 *	Bitmap routines
 *
 *************************************/

static void draw_bitmap(struct osd_bitmap *bitmap)
{
	UINT16 *pens = &Machine->pens[512 + (videoflags & 15) * 16];
	int x, y;

	/* draw any non-transparent scanlines from the VRAM directly */
	for (y = 0; y < BITMAP_HEIGHT; y++)
		if (rpunch_bitmapsum[y] != (BITMAP_WIDTH/4) * 0xffff)
		{
			data16_t *src = &rpunch_bitmapram[y * 128 + BITMAP_XOFFSET/4];
			UINT8 scanline[BITMAP_WIDTH], *dst = scanline;

			/* extract the scanline */
			for (x = 0; x < BITMAP_WIDTH/4; x++)
			{
				int data = *src++;

				dst[0] = data >> 12;
				dst[1] = (data >> 8) & 15;
				dst[2] = (data >> 4) & 15;
				dst[3] = data & 15;
				dst += 4;
			}
			draw_scanline8(bitmap, 0, y, BITMAP_WIDTH, scanline, pens, 15);
		}
}


/*************************************
 *
 *	Main screen refresh
 *
 *************************************/

void rpunch_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x, penbase, effbins;

	/* update background 0 */
	callback_gfxbank = 0;
	callback_videoram = &videoram16[0];
	callback_colorbase = (videoflags & 0x0010) >> 1;
	callback_imagebase = (videoflags & 0x0400) << 3;
	callback_imagemask = callback_imagebase ? 0x0fff : 0x1fff;
	tilemap_update(background[0]);

	/* update background 1 */
	callback_gfxbank = 1;
	callback_videoram = &videoram16[videoram_size / 4];
	callback_colorbase = (videoflags & 0x0020) >> 2;
	callback_imagebase = (videoflags & 0x0800) << 2;
	callback_imagemask = callback_imagebase ? 0x0fff : 0x1fff;
	tilemap_update(background[1]);

	/* update the palette usage */
	palette_init_used_colors();
	mark_sprite_palette(0, gins);
	if (rpunch_bitmapram)
	{
		penbase = 512 + (videoflags & 15);
		for (x = 0; x < 15; x++)
			palette_used_colors[penbase + x] = PALETTE_COLOR_USED;
		palette_used_colors[penbase + 15] = PALETTE_COLOR_TRANSPARENT;
	}
	palette_recalc();

	/* draw the background layer */
	tilemap_draw(bitmap, background[0], 0,0);

	/* this seems like the most plausible explanation */
	effbins = (bins > gins) ? gins : bins;

	draw_sprites(bitmap, 0, effbins);
	tilemap_draw(bitmap, background[1], 0,0);
	draw_sprites(bitmap, effbins, gins);
	if (rpunch_bitmapram)
		draw_bitmap(bitmap);
}
