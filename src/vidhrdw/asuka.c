#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

void asuka_vh_stop(void);

static UINT16 sprite_ctrl = 0;
static UINT16 sprites_flipscreen = 0;

/**********************************************************/

int asuka_core_vh_start (int x_offs)
{
	if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,0))
	{
		asuka_vh_stop();
		return 1;
	}

	if (TC0110PCR_vh_start())
	{
		asuka_vh_stop();
		return 1;
	}

	state_save_register_UINT16("sprite_ctrl", 0, "sprites", &sprite_ctrl, 1);
	state_save_register_UINT16("sprite_flip", 0, "sprites", &sprites_flipscreen, 1);

	return 0;
}

int asuka_vh_start (void)
{
	return (asuka_core_vh_start(0));
}

int galmedes_vh_start (void)
{
	return (asuka_core_vh_start(1));
}

void asuka_vh_stop (void)
{
	TC0100SCN_vh_stop();

	TC0110PCR_vh_stop();
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/

WRITE16_HANDLER( asuka_spritectrl_w )
{
	sprite_ctrl = data;
}

WRITE16_HANDLER( asuka_spriteflip_w )
{
	sprites_flipscreen = data;
}


/************************************************************
			SPRITE DRAW ROUTINE

		Asuka (information from Raine)

		OBJECT RAM
		----------

		- 8 bytes/sprite
		- 256 sprites (0x800 bytes)
		- First sprite has *highest* priority

		-----+--------+-------------------------
		Byte | Bit(s) | Use
		-----+76543210+-------------------------
		  0  |.x......| Flip Y Axis
		  0  |x.......| Flip X Axis
		  1  |....xxxx| Colour Bank
		  2  |.......x| Sprite Y
		  3  |xxxxxxxx| Sprite Y
		  4  |...xxxxx| Sprite Tile
		  5  |xxxxxxxx| Sprite Tile
		  6  |.......x| Sprite X
		  7  |xxxxxxxx| Sprite X
		-----+--------+-------------------------

		SPRITE CONTROL
		--------------

		- Maze of Flott [603D MASK] 201C 200B 200F
		- Earth Joker 001C
		- Cadash 0011 0013 0010 0000

		-----+--------+-------------------------
		Byte | Bit(s) | Use
		-----+76543210+-------------------------
		  0  |.......x| ?
		  0  |......x.| Write Acknowledge?
		  0  |..xxxx..| Colour Bank Offset
		  0  |xx......| Unused
		  1  |...xxxxx| Unused
		  1  |..x.....| BG1:Sprite Priority
		  1  |.x......| Priority?
		  1  |x.......| Unused
		-----+--------+-------------------------

		OLD SPRITE CONTROL (RASTAN TYPE)
		--------------------------------

		-----+--------+-------------------------
		Byte | Bit(s) | Use
		-----+76543210+-------------------------
		  1  |.......x| BG1:Sprite Priority?
		  1  |......x.| Write Acknowledge?
		  1  |xxx.....| Colour Bank Offset
		-----+--------+-------------------------


********************************************************/


static void asuka_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;
	int sprite_colbank = (sprite_ctrl & 0x3c) << 2;

	/* Mofflot sets this, I haven't seen the other games do so */
	int priority = (sprite_ctrl & 0x2000) >> 13;	/* 1 = sprites under top bg layer */

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int flipx, flipy;
		int x, y;
		int data,code,color;

		data = spriteram16[offs+0];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		color = (data & 0x000f) | sprite_colbank;

		code = spriteram16[offs+2] & 0x1fff;
		x = spriteram16[offs+3] & 0x1ff;   // correct mask?
		y = spriteram16[offs+1] & 0x1ff;   // correct mask?
		y += 8;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		if ((sprites_flipscreen &1) == 0)
		{
			x = 320 - x - 16;
			y = 256 - y;
			flipx = !flipx;
			flipy = !flipy;
		}

		/* Sprites can be under/over the layer below text layer */
		pdrawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				priority ? 0xfc : 0xf0);
	}
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void asuka_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	asuka_draw_sprites(bitmap);

#if 0
	{
		char buf[80];

		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

