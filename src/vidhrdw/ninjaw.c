#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;

int taito_hide_pixels;
size_t ninjaw_spriteram_size;
data16_t *ninjaw_spriteram16;

/**********************************************************/

static int number_of_TC0100SCN(void)
{
	int has_chip[3] = {0,0,0};
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see how many TC0100SCN are used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0100SCN_word_0_w)
					has_chip[0] = 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0100SCN_word_1_w)
					has_chip[1] = 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0100SCN_word_2_w)
					has_chip[2] = 1;
			}
			mwa++;
		}
	}

	/* Catch illegal configurations */

	if (!has_chip[0] && (has_chip[1] || has_chip[2]))
		return -1;

	if (!has_chip[1] && has_chip[2])
		return -1;

	return (has_chip[0] + has_chip[1] + has_chip[2]);
}

static int has_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0110PCR_step1_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_second_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if a second TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0110PCR_step1_word_1_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_third_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if a third TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0110PCR_step1_word_2_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}


int ninjaw_core_vh_start (void)
{
	int chips;

	spritelist = malloc(0x1000 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	chips = number_of_TC0100SCN();

	if (chips <= 0)	/* we have an erroneous TC0100SCN configuration */
		return 1;

	if (TC0100SCN_vh_start(chips,TC0100SCN_GFX_NUM,taito_hide_pixels))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	if (has_second_TC0110PCR())
		if (TC0110PCR_1_vh_start())
			return 1;

	if (has_third_TC0110PCR())
		if (TC0110PCR_2_vh_start())
			return 1;

	return 0;
}

int ninjaw_vh_start (void)
{
	taito_hide_pixels = 0;
	return (ninjaw_core_vh_start());
}

void ninjaw_vh_stop (void)
{
	free(spritelist);
	spritelist = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();

	if (has_second_TC0110PCR())
		TC0110PCR_1_vh_stop();

	if (has_third_TC0110PCR())
		TC0110PCR_2_vh_stop();
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/

READ16_HANDLER( ninjaw_spriteram_r )
{
	return ninjaw_spriteram16[offset];
}

WRITE16_HANDLER( ninjaw_spriteram_w )
{
	COMBINE_DATA(&ninjaw_spriteram16[offset]);
}


/*********************************************************
				PALETTE
*********************************************************/

void ninjaw_update_palette (void)
{
	int i,j;
	int offs,data,tilenum,color;
	unsigned short palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = (ninjaw_spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = ninjaw_spriteram16[offs+2];
		tilenum = data & 0x7fff;

		data = ninjaw_spriteram16[offs+3];
		color = (data & 0x7f00) >> 8;

		if (tilenum)
		{
			palette_map[color] |= Machine->gfx[0]->pen_usage[tilenum];
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}




/************************************************************
			SPRITE DRAW ROUTINE
************************************************************/

static void ninjaw_draw_sprites(struct osd_bitmap *bitmap,int *primasks,int y_offs)
{
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int code;

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	priority = 0;

	for (offs = (ninjaw_spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = ninjaw_spriteram16[offs+0];
		x = (data + 6) & 0x3ff;

		data = ninjaw_spriteram16[offs+1];
		y = (data - 0) & 0x1ff;	// this offset will change if screen vis area changes

		data = ninjaw_spriteram16[offs+2];
		tilenum = data & 0x7fff;	// Darius 2 mask *may* be 0x1fff

		data = ninjaw_spriteram16[offs+3];
		flipy = (data & 0x2); //>> 9;
		flipx = (data & 0x1); //>> 8;
		color = (data & 0x7f00) >> 8;

		if (!tilenum) continue;

		y += y_offs;

		/* sprite wrap: coords become negative at high values */
		if (x>0x3c0) x -= 0x400;
		if (y>0x180) y -= 0x200;

		curx = x;
		cury = y;
		code = tilenum;

		sprite_ptr->code = code;
		sprite_ptr->color = color;
		sprite_ptr->flipx = flipx;
		sprite_ptr->flipy = flipy;
		sprite_ptr->x = curx;
		sprite_ptr->y = cury;

		if (primasks)
		{
			sprite_ptr->primask = primasks[priority];
			sprite_ptr++;
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfx(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->primask);
	}
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void ninjaw_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	ninjaw_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */
	TC0100SCN_tilemap_draw(bitmap,0,layer[0],0,1);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	/* Sprites can be under/over the layer below text layer */
	{
		int primasks[2] = {0xf0,0xfc};
		ninjaw_draw_sprites(bitmap,primasks,8);
	}
}

