#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"

static struct mame_bitmap* pixel_layer = 0;

static UINT16* video_ram = NULL;

static UINT16 video_ctrl = 0;
static UINT16 video_mask = 0;
static UINT16 sprite_ctrl = 0;
static UINT16 sprite_flip = 0;

static UINT8* line_dirty = NULL;

static void mark_all_dirty(void)
{
	memset(line_dirty, 1, 256);
}


/******************************************************
          INITIALISATION AND CLEAN-UP
******************************************************/

int volfied_vh_start (void)
{
	pixel_layer = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (pixel_layer == NULL)
		return 1;

	line_dirty = malloc(256);
	if (line_dirty == NULL)
		return 1;

	video_ram = malloc(0x40000 * sizeof (UINT16));
	if (video_ram == NULL)
		return 1;

	state_save_register_UINT16("volfied", 0, "video_ram",     video_ram,     0x40000);
	state_save_register_UINT16("volfied", 0, "video_ctrl",    &video_ctrl,   1);
	state_save_register_UINT16("volfied", 0, "video_mask",    &video_mask,   1);
	state_save_register_UINT16("volfied", 0, "sprite_ctrl",   &sprite_ctrl,  1);
	state_save_register_UINT16("volfied", 0, "sprite_flip",   &sprite_flip,  1);

	state_save_register_func_postload (mark_all_dirty);

	return 0;
}

void volfied_vh_stop (void)
{
	if (video_ram != NULL)
		free(video_ram);

	if (line_dirty != NULL)
		free(line_dirty);

	if (pixel_layer != NULL)
		bitmap_free(pixel_layer);
}


/*******************************************************
          READ AND WRITE HANDLERS
*******************************************************/

READ16_HANDLER( volfied_video_ram_r )
{
	return video_ram[offset];
}

WRITE16_HANDLER( volfied_video_ram_w )
{
	mem_mask |= ~video_mask;

	line_dirty[(offset >> 9) & 0xff] = 1;

	COMBINE_DATA(&video_ram[offset]);
}

WRITE16_HANDLER( volfied_video_ctrl_w )
{
	if (ACCESSING_LSB && (data & 1) != (video_ctrl & 1))
	{
		mark_all_dirty();    /* screen switch */
	}

	COMBINE_DATA(&video_ctrl);
}

READ16_HANDLER( volfied_video_ctrl_r )
{
	/* Could this be some kind of hardware collision detection? If bit 6 is
	   set the game will check for collisions with the large enemy, whereas
	   bit 5 does the same for small enemies. Bit 7 is also used although
	   its purpose is unclear. This register is usually read during a VBI
	   and stored in work RAM for later use. */

	return 0x60;
}

WRITE16_HANDLER( volfied_video_mask_w )
{
	COMBINE_DATA(&video_mask);
}

WRITE16_HANDLER( volfied_sprite_ctrl_w )
{
	COMBINE_DATA(&sprite_ctrl);
}

WRITE16_HANDLER( volfied_sprite_flip_w )
{
	COMBINE_DATA(&sprite_flip);
}


/*******************************************************
				SCREEN REFRESH
*******************************************************/

static void draw_sprites(struct mame_bitmap* bitmap)
{
	int offs;

	int color_bank = 4 * (sprite_ctrl & 0x3c);

	for (offs = (spriteram_size / 2) - 4; offs >= 0; offs -= 4)
	{
		int tilenum = spriteram16[offs + 2] & 0x1fff;

		if (tilenum != 0)
		{
			int color = spriteram16[offs] & 0xf;

			int x = spriteram16[offs+3] & 0x1ff;
			int y = spriteram16[offs+1] & 0x1ff;

			int flipx = (spriteram16[offs] >> 14) & 1;
			int flipy = (spriteram16[offs] >> 15) & 1;

			/* treat coords as signed */
			if (x > 0x140) x -= 0x200;
			if (y > 0x140) y -= 0x200;

			if ((sprite_flip & 1) == 0)
			{
				x = 320 - x - 15; /* ? */
				y = 256 - y - 17; /* ? */
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap, Machine->gfx[0], tilenum, color_bank | color,
				flipx, flipy, x, y, &Machine->visible_area, TRANSPARENCY_PEN, 0);
		}
	}
}

static void refresh_pixel_layer(void)
{
	int x, y;

	/*********************************************************

	VIDEO RAM has 2 screens x 256 rows x 512 columns x 16 bits

	x---------------  select image
	-x--------------  ?             (used for 3-D corners)
	--x-------------  ?             (used for 3-D walls)
	---xxxx---------  image B
	-------xxx------  palette index bits #8 to #A
	----------x-----  ?
	-----------x----  ?
	------------xxxx  image A

	*********************************************************/

	UINT16* p = video_ram;

	if (video_ctrl & 1)
	{
		p += 0x20000;
	}

	for (y = 0; y < Machine->drv->screen_height; y++)
	{
		if (line_dirty[y])
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				int color = (p[x] << 2) & 0x700;

				if (p[x] & 0x8000)
				{
					color |= 0x800 | ((p[x] >> 9) & 0xf);

					if (p[x] & 0x2000)
					{
						color &= ~0xf;	  /* hack */
					}
				}
				else
				{
					color |= p[x] & 0xf;
				}

				plot_pixel(pixel_layer, x, y, Machine->pens[color]);
			}

			line_dirty[y] = 0;
		}

		p += 512;
	}
}

void volfied_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	refresh_pixel_layer();

	copybitmap(bitmap, pixel_layer, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);

	draw_sprites(bitmap);
}
