#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 3

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

void undrfire_vh_stop (void)
{
	free(spritelist);
	spritelist = 0;

	TC0100SCN_vh_stop();

	TC0480SCP_vh_stop();
}

int undrfire_vh_start (void)
{
	spritelist = malloc(0x4000 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	/* aligns piv layers with window but vertical align may be bad */
	if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,50))
		return 1;

	if (TC0480SCP_vh_start(1,0,0x24,0,-1,0,0,0,0))
		return 1;

	return 0;
}

/************************************************************
			SPRITE DRAW ROUTINES

We draw a series of small tiles ("chunks") together to
create each big sprite. The spritemap rom provides the lookup
table for this. The game hardware looks up 16x16 sprite chunks
from the spritemap rom, creating a 64x64 sprite like this:

	 0  1  2  3
	 4  5  6  7
	 8  9 10 11
	12 13 14 15

(where the number is the word offset into the spritemap rom).
It can also create 32x32 sprites.

NB: unused portions of the spritemap rom contain hex FF's.
It is a useful coding check to warn in the log if these
are being accessed. [They can be inadvertently while
spriteram is being tested, take no notice of that.]

Heavy use is made of sprite zooming.

		***

	Sprite table layout (4 long words per entry)

	------------------------------------------
	 0 | ........ x....... ........ ........ | Flip X
	 0 | ........ .xxxxxxx ........ ........ | ZoomX
	 0 | ........ ........ .xxxxxxx xxxxxxxx | Sprite Tile
	   |                                     |
	 2 | ........ ....xx.. ........ ........ | Sprite/tile priority [*]
	 2 | ........ ......xx xxxxxx.. ........ | Palette bank
	 2 | ........ ........ ......xx xxxxxxxx | X position
	   |                                     |
	 3 | ........ .....x.. ........ ........ | Sprite size (0=32x32, 1=64x64)
	 3 | ........ ......x. ........ ........ | Flip Y
	 3 | ........ .......x xxxxxx.. ........ | ZoomY
	 3 | ........ ........ ......xx xxxxxxxx | Y position
	------------------------------------------

	[* 00=over BG0, 01=BG1, 10=BG2, 11=BG3 ]

********************************************************/

static void undrfire_draw_sprites_16x16(struct osd_bitmap *bitmap,int *primasks,int x_offs,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, dblsize, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int dimension,total_chunks,bad_chunks;

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/4-4);offs >= 0;offs -= 4)
	{
		data = spriteram32[offs+0];
		flipx =    (data & 0x00800000) >> 23;
		zoomx =    (data & 0x007f0000) >> 16;
		tilenum =  (data & 0x00007fff);

		data = spriteram32[offs+2];
		priority = (data & 0x000c0000) >> 18;
		color =    (data & 0x0003fc00) >> 10;
		x =        (data & 0x000003ff);

		data = spriteram32[offs+3];
		dblsize =  (data & 0x00040000) >> 18;
		flipy =    (data & 0x00020000) >> 17;
		zoomy =    (data & 0x0001fc00) >> 10;
		y =        (data & 0x000003ff);

		color |= (0x100 + (priority << 6));		/* priority bits select color bank */
		color /= 2;		/* as sprites are 5bpp */
		flipy = !flipy;
		y = (-y &0x3ff);

		if (!tilenum) continue;

		flipy = !flipy;
		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x340) x -= 0x400;
		if (y>0x340) y -= 0x400;

		x -= x_offs;

		bad_chunks = 0;
		dimension = ((dblsize*2) + 2);	// 2 or 4
		total_chunks = ((dblsize*3) + 1) << 2;	// 4 or 16
		map_offset = tilenum << 2;

		{
			for (sprite_chunk=0;sprite_chunk<total_chunks;sprite_chunk++)
			{
				j = sprite_chunk / dimension;   /* rows */
				k = sprite_chunk % dimension;   /* chunks per row */

				px = k;
				py = j;
				/* pick tiles back to front for x and y flips */
				if (flipx)  px = dimension-1-k;
				if (flipy)  py = dimension-1-j;

				code = spritemap[map_offset + px + (py<<(dblsize+1))];

				if (code==0xffff)
				{
					bad_chunks += 1;
					continue;
				}

				curx = x + ((k*zoomx)/dimension);
				cury = y + ((j*zoomy)/dimension);

				zx= x + (((k+1)*zoomx)/dimension) - curx;
				zy= y + (((j+1)*zoomy)/dimension) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				sprite_ptr->gfx = 0;
				sprite_ptr->code = code;
				sprite_ptr->color = color;
				sprite_ptr->flipx = !flipx;
				sprite_ptr->flipy = flipy;
				sprite_ptr->x = curx;
				sprite_ptr->y = cury;
				sprite_ptr->zoomx = zx << 12;
				sprite_ptr->zoomy = zy << 12;

				if (primasks)
				{
					sprite_ptr->primask = primasks[priority];

					sprite_ptr++;
				}
				else
				{
					drawgfxzoom(bitmap,Machine->gfx[sprite_ptr->gfx],
							sprite_ptr->code,
							sprite_ptr->color,
							sprite_ptr->flipx,sprite_ptr->flipy,
							sprite_ptr->x,sprite_ptr->y,
							&Machine->visible_area,TRANSPARENCY_PEN,0,
							sprite_ptr->zoomx,sprite_ptr->zoomy);
				}
			}
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}

	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfxzoom(bitmap,Machine->gfx[sprite_ptr->gfx],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->zoomx,sprite_ptr->zoomy,
				sprite_ptr->primask);
	}
}

/**************************************************************
				SCREEN REFRESH
**************************************************************/

void undrfire_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[5];
	UINT8 pivlayer[3];
	UINT16 priority;

#ifdef MAME_DEBUG
	static int dislayer[5];	/* Layer toggles to help get the layers correct */
	char buf[80];
#endif

#ifdef MAME_DEBUG
	if (keyboard_pressed_memory (KEYCODE_C))
	{
		dislayer[0] ^= 1;
		sprintf(buf,"bg0: %01x",dislayer[0]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_V))
	{
		dislayer[1] ^= 1;
		sprintf(buf,"bg1: %01x",dislayer[1]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_B))
	{
		dislayer[2] ^= 1;
			sprintf(buf,"bg2: %01x",dislayer[2]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_N))
	{
		dislayer[3] ^= 1;
		sprintf(buf,"bg3: %01x",dislayer[3]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_M))
	{
		dislayer[4] ^= 1;
		sprintf(buf,"sprites: %01x",dislayer[4]);
		usrintf_showmessage(buf);
	}
#endif

	TC0100SCN_tilemap_update();
	TC0480SCP_tilemap_update();

	priority = TC0480SCP_get_bg_priority();

	layer[0] = (priority &0xf000) >> 12;	/* tells us which bg layer is bottom */
	layer[1] = (priority &0x0f00) >>  8;
	layer[2] = (priority &0x00f0) >>  4;
	layer[3] = (priority &0x000f) >>  0;	/* tells us which is top */
	layer[4] = 4;   /* text layer always over bg layers */

	pivlayer[0] = TC0100SCN_bottomlayer(0);
	pivlayer[1] = pivlayer[0]^1;
	pivlayer[2] = 2;

	palette_init_used_colors();
	memset(palette_used_colors,PALETTE_COLOR_VISIBLE,Machine->drv->total_colors);
	TC0480SCP_mark_transparent_colors(layer[0]);
	palette_recalc();

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

/* Suspect the PIV chip is a TC0100SCN in disguise ? */

	TC0100SCN_tilemap_draw(bitmap,0,pivlayer[0],0,0);
	TC0100SCN_tilemap_draw(bitmap,0,pivlayer[1],0,0);

#ifdef MAME_DEBUG
	if (dislayer[layer[0]]==0)
#endif
	TC0480SCP_tilemap_draw(bitmap,layer[0],0,1);

#ifdef MAME_DEBUG
	if (dislayer[layer[1]]==0)
#endif
	TC0480SCP_tilemap_draw(bitmap,layer[1],0,2);

#ifdef MAME_DEBUG
	if (dislayer[layer[2]]==0)
#endif
	TC0480SCP_tilemap_draw(bitmap,layer[2],0,4);

#ifdef MAME_DEBUG
	if (dislayer[layer[3]]==0)
#endif
	TC0480SCP_tilemap_draw(bitmap,layer[3],0,8);

#ifdef MAME_DEBUG
	if (dislayer[4]==0)
#endif
	/* Sprites have variable priority */
	{
		int primasks[4] = {0xfffc, 0xfff0, 0xff00, 0x0};
		undrfire_draw_sprites_16x16(bitmap,primasks,44,-574);	// aligns round 1 window
	}

	TC0100SCN_tilemap_draw(bitmap,0,pivlayer[2],0,0);	/* piv text layer */
	TC0480SCP_tilemap_draw(bitmap,layer[4],0,0);	/* text layer */

}
