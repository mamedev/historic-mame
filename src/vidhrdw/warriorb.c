#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

void warriorb_vh_stop(void);

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

/**********************************************************/

static int has_two_TC0100SCN(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the second TC0100SCN is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0100SCN_word_1_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
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


static int warriorb_core_vh_start (int x_offs,int multiscrn_xoffs)
{

	spritelist = malloc(0x800 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	if (TC0100SCN_vh_start(has_two_TC0100SCN() ? 2 : 1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,
			multiscrn_xoffs))
	{
		warriorb_vh_stop();
		return 1;
	}

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
		{
			warriorb_vh_stop();
			return 1;
		}

	if (has_second_TC0110PCR())
		if (TC0110PCR_1_vh_start())
		{
			warriorb_vh_stop();
			return 1;
		}

	/* Ensure palette from correct TC0110PCR used for each screen */
	TC0100SCN_set_chip_colbanks(0,0x100,0x0);

	return 0;
}

int darius2d_vh_start (void)
{
	return (warriorb_core_vh_start(4,0));
}

int warriorb_vh_start (void)
{
	return (warriorb_core_vh_start(4,1));
}

void warriorb_vh_stop (void)
{
	free(spritelist);
	spritelist = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();

	if (has_second_TC0110PCR())
		TC0110PCR_1_vh_stop();
}


/************************************************************
			SPRITE DRAW ROUTINE
************************************************************/

static void warriorb_draw_sprites(struct mame_bitmap *bitmap,int *primasks,int y_offs)
{
	int offs, data, data2, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int code;

#ifdef MAME_DEBUG
	int unknown=0;
#endif

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+1];
		tilenum = data & 0x7fff;

		if (!tilenum) continue;

		data = spriteram16[offs+0];
		y = (-(data &0x1ff) - 24) & 0x1ff;	/* (inverted y adjusted for vis area) */
		flipy = (data & 0x200) >> 9;

		data2 = spriteram16[offs+2];
		/* 8,4 also seen in msbyte */
		priority = (data2 & 0x0100) >> 8;	// 0x300
		color    = (data2 & 0x7f);

		data = spriteram16[offs+3];
		x = (data & 0x3ff);
		flipx = (data & 0x400) >> 10;


#ifdef MAME_DEBUG
		if (data2 & 0xf280)   unknown |= (data2 &0xf280);
#endif

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

#ifdef MAME_DEBUG
	if (unknown)
	{
		char buf[80];
		sprintf(buf,"unknown sprite bits: %04x",unknown);
		usrintf_showmessage(buf);
	}
#endif
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void warriorb_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layers not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	/* chip 0 does tilemaps on the left, chip 1 does the ones on the right */
	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);	/* left */
	TC0100SCN_tilemap_draw(bitmap,1,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);	/* right */
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,1,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);
	TC0100SCN_tilemap_draw(bitmap,1,layer[2],0,4);

//	/* Sprites have 3 priorities wrt tiles (unsure if they have a 4th - on top of text) */
//	{
//		int primasks[4] = {0xf0,0xfc,0x0,0xfe};
//		warriorb_draw_sprites(bitmap,primasks,8);
//	}

	/* Sprites can be under/over the layer below text layer */
	{
		int primasks[2] = {0xf0,0xfc};
		warriorb_draw_sprites(bitmap,primasks,8);
	}
}

