/***************************************************************************

Atari Wolf Pack (prototype) video emulation

***************************************************************************/

#include "driver.h"

extern int wolfpack_flip_sprite;
extern int wolfpack_collision;

extern UINT8* wolfpack_alpha_num_ram;
extern UINT8* wolfpack_regs1;
extern UINT8* wolfpack_regs2;

static struct mame_bitmap* helper;


static void draw_target(struct mame_bitmap* bitmap, const struct rectangle* cliprect)
{
	int hpos = wolfpack_regs2[0];
	int chop = wolfpack_regs2[3];
	int code = wolfpack_regs2[4];

	/* wolfpack_regs2[2] might be zoom */

	drawgfx(bitmap, Machine->gfx[1],
		code,
		0,
		wolfpack_flip_sprite, 0,
		hpos - chop,
		0x64, /* ? */
		cliprect,
		TRANSPARENCY_PEN, 0);
}


static void draw_torpedo(struct mame_bitmap* bitmap, const struct rectangle* cliprect, int transparent)
{
	int code = wolfpack_regs2[1] ^ 0xFF;
	int hpos = wolfpack_regs2[5] ^ 0xFF;
	int vpos = wolfpack_regs2[6];

	drawgfx(bitmap, Machine->gfx[3],
		code,
		0,
		0, 0,
		hpos,
		vpos,
		cliprect,
		TRANSPARENCY_PEN, transparent);
}


static void draw_object(struct mame_bitmap* bitmap, const struct rectangle* cliprect)
{
	int code = wolfpack_regs1[3];
	int hpos = wolfpack_regs1[1];

	if (!(code & 0x10))
	{
		hpos -= 0x100;
	}

	drawgfx(bitmap, Machine->gfx[2],
		code,
		0,
		0, 0,
		hpos,
		0x6C, /* ? */
		cliprect,
		TRANSPARENCY_PEN, 0);
}


VIDEO_START( wolfpack )
{
	helper = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	return (helper == NULL) ? 1 : 0;
}


VIDEO_UPDATE( wolfpack )
{
	int i;
	int j;

	fillbitmap(bitmap, 0, cliprect);

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 32; j++)
		{
			int code = wolfpack_alpha_num_ram[32 * i + j];

			drawgfx(bitmap, Machine->gfx[0], code, 0, 0, 0,
				8 * j, 8 * i, cliprect, TRANSPARENCY_NONE, 0);
		}
	}

	draw_target(bitmap, cliprect);

	draw_torpedo(bitmap, cliprect, 0);

	draw_object(bitmap, cliprect);
}


VIDEO_EOF( wolfpack )
{
	struct rectangle rect;

	int hpos = wolfpack_regs2[5] ^ 0xFF;
	int vpos = wolfpack_regs2[6];

	int x;
	int y;

	rect.min_x = 0;
	rect.min_y = 0;
	rect.max_x = helper->width;
	rect.max_y = helper->height;

	fillbitmap(helper, 0, &rect);

	draw_target(helper, &rect);

	draw_torpedo(helper, &rect, 1);

	wolfpack_collision = 0;

	for (y = 0; y < Machine->gfx[3]->height; y++)
	{
		for (x = 0; x < Machine->gfx[3]->width; x++)
		{
			if (hpos + x >= helper->width)
				continue;
			if (vpos + y >= helper->height)
				continue;

			if (read_pixel(helper, hpos + x, vpos + y))
			{
				wolfpack_collision = 1;
			}
		}
	}
}

