/***************************************************************************
  Functions to emulate similar video hardware on these Taito games:

  - rastan
  - operation wolf
  - rainbow islands
  - jumping (bootleg)

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

/* NB: sprite routines can be moved over to pdrawgfx.
   It seems fairly certain however that there is no
   individual sprite/tile priority. */

static UINT16 sprite_ctrl = 0;
static UINT16 sprites_flipscreen = 0;

static int beamx,beamy;

/***************************************************************************/

static void rastan_coin_ctrl(void)
{
		/* bits 0 and 1 are coin lockout */
		coin_lockout_w(1,~sprite_ctrl & 0x01);
		coin_lockout_w(0,~sprite_ctrl & 0x02);

		/* bits 2 and 3 are the coin counters */
		coin_counter_w(1,sprite_ctrl & 0x04);
		coin_counter_w(0,sprite_ctrl & 0x08);
}


int rastan_vh_start (void)
{
	/* (chips, gfxnum, x_offs, y_offs, y_invert, opaque, dblwidth) */
	if (PC080SN_vh_start(1,1,0,0,0,0,0))
		return 1;

	state_save_register_UINT16("sprite_ctrl", 0, "sprites", &sprite_ctrl, 1);
	state_save_register_UINT16("sprite_flip", 0, "sprites", &sprites_flipscreen, 1);
	state_save_register_func_postload(rastan_coin_ctrl);
	return 0;
}

int opwolf_vh_start (void)
{
	if (PC080SN_vh_start(1,1,0,0,0,0,0))
		return 1;

	state_save_register_UINT16("sprite_ctrl", 0, "sprites", &sprite_ctrl, 1);
	state_save_register_UINT16("sprite_flip", 0, "sprites", &sprites_flipscreen, 1);
	return 0;
}

int rainbow_vh_start (void)
{
	/* (chips, gfxnum, x_offs, y_offs, y_invert, opaque, dblwidth) */
	if (PC080SN_vh_start(1,1,0,0,0,0,0))
		return 1;

	state_save_register_UINT16("sprite_ctrl", 0, "sprites", &sprite_ctrl, 1);
	state_save_register_UINT16("sprite_flip", 0, "sprites", &sprites_flipscreen, 1);
	return 0;
}

int jumping_vh_start(void)
{
	if (PC080SN_vh_start(1,1,0,0,1,0,0))
		return 1;

	PC080SN_set_trans_pen(0,1,15);

	/* not 100% sure Jumping needs to save both... */
	state_save_register_UINT16("sprite_ctrl", 0, "sprites", &sprite_ctrl, 1);
	state_save_register_UINT16("sprite_flip", 0, "sprites", &sprites_flipscreen, 1);
	return 0;
}

void rastan_vh_stop(void)
{
	PC080SN_vh_stop();
}


WRITE16_HANDLER( rastan_spritectrl_w )
{
	if (offset == 0)
	{
		sprite_ctrl = data;
		rastan_coin_ctrl();

		/* bits 5-7 are the sprite palette bank */
		/* bit 4 + hi byte unknown */
	}
}

WRITE16_HANDLER( rainbow_spritectrl_w )
{
	if (offset == 0)
	{
		sprite_ctrl = data;

		/* bits 0 and 1 always set [Jumping waits 15 seconds before doing this] */
		/* bits 5-7 are the sprite palette bank */
		/* other bits unknown */
	}
}

WRITE16_HANDLER( rastan_spriteflip_w )
{
	sprites_flipscreen = data;
}


/***************************************************************************/

static void rastan_draw_sprites(struct mame_bitmap *bitmap,int y_offs)
{
	int offs,tile;
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;

	/* Draw the sprites. 256 sprites in total */
	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		tile = spriteram16[offs+2];
		if (tile)
		{
			int sx,sy,color,data1;
			int flipx,flipy;

			sx = spriteram16[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
 			sy = spriteram16[offs+1] & 0x1ff;
			sy += y_offs;
			if (sy > 400) sy = sy - 512;

			data1 = spriteram16[offs];
			color = (data1 & 0x0f) | sprite_colbank;
			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if ((sprites_flipscreen &1) == 0)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}


void rastan_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int layer[2];

	PC080SN_tilemap_update();

	layer[0] = 0;
	layer[1] = 1;

	fillbitmap(priority_bitmap,0,NULL);

 	PC080SN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	PC080SN_tilemap_draw(bitmap,0,layer[1],0,0);

	rastan_draw_sprites(bitmap,0);

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

/***************************************************************************/

void opwolf_eof_callback(void)
{
	/* Ensure the analog x,y values are read _exactly once_ */
	/* per frame irrespective of frameskip and fake DSW */
	beamx = ((input_port_5_r(0) * 256) >> 8);	//+3
	beamy = ((input_port_6_r(0) * 256) >> 8);	//+19
}

void opwolf_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int layer[2];

	PC080SN_tilemap_update();

	layer[0] = 0;
	layer[1] = 1;

	fillbitmap(priority_bitmap,0,NULL);

 	PC080SN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	rastan_draw_sprites(bitmap,0);
	PC080SN_tilemap_draw(bitmap,0,layer[1],0,0);

	/* See if we should draw artificial gun targets */
	if (input_port_4_word_r(0,0) &0x1)	/* Fake DSW */
	{
		/* Draw an aiming crosshair */
		draw_crosshair(bitmap,beamx,beamy,&Machine->visible_area);
	}

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

/***************************************************************************/

void rainbow_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;
	int layer[2];

	PC080SN_tilemap_update();

	layer[0] = 0;
	layer[1] = 1;

	fillbitmap(priority_bitmap,0,NULL);

 	PC080SN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);

	/* Draw the sprites. 256 sprites in total */
	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		int tile = spriteram16[offs+2];
		if (tile)
		{
			int sx,sy,color,data1;
			int flipx,flipy;

			sx = spriteram16[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
 			sy = spriteram16[offs+1] & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = spriteram16[offs];
			color = (data1 & 0x0f) | sprite_colbank;
			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if ((sprites_flipscreen &1) == 0)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}

			if (tile < 4096)
				drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			else
				drawgfx(bitmap,Machine->gfx[2],
					tile-4096,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	PC080SN_tilemap_draw(bitmap,0,layer[1],0,0);

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

/***************************************************************************

Jumping uses different sprite controller
than rainbow island. - values are remapped
at address 0x2EA in the code. Apart from
physical layout, the main change is that
the Y settings are active low.

*/

void jumping_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs,layer[2];
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;

	PC080SN_tilemap_update();

	/* Override values, or foreground layer is in wrong position */
	PC080SN_set_scroll(0,1,16,0);

	layer[0] = 0;
	layer[1] = 1;

	fillbitmap(priority_bitmap,0,NULL);

 	PC080SN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);

	/* Draw the sprites. 128 sprites in total */
	for (offs = spriteram_size/2-8; offs >= 0; offs -= 8)
	{
		int tile = spriteram16[offs];
		if (tile < Machine->gfx[1]->total_elements)
		{
			int sx,sy,color,data1;

			sy = ((spriteram16[offs+1] - 0xfff1) ^ 0xffff) & 0x1ff;
  			if (sy > 400) sy = sy - 512;
			sx = (spriteram16[offs+2] - 0x38) & 0x1ff;
			if (sx > 400) sx = sx - 512;

			data1 = spriteram16[offs+3];
			color = (spriteram16[offs+4] & 0x0f) | sprite_colbank;

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					data1 & 0x40, data1 & 0x80,
					sx,sy+1,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}

 	PC080SN_tilemap_draw(bitmap,0,layer[1],0,0);

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

