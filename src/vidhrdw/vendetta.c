#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


static int layer_colorbase[3],bg_colorbase,sprite_colorbase;

#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2


/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x03) << 8) | ((*color & 0x30) << 6) |
			((*color & 0x0c) << 10) | (bank << 14);
	*color = layer_colorbase[layer] + ((*color & 0xc0) >> 6);
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int vendetta_vh_start(void)
{
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback) != 0)
		return 1;

	return 0;
}

void vendetta_vh_stop(void)
{
	K052109_vh_stop();
}


#define SPRITEWORD(offset) (256*spriteram[(offset)]+spriteram[(offset)+1])
static void simpsons_drawsprites(struct osd_bitmap *bitmap,int min_priority,int max_priority)
{
#define NUM_SPRITES 256
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		sortedlist[SPRITEWORD(offs) & 0x00ff] = offs;
	}

	for (pri_code = NUM_SPRITES-1;pri_code >= 0;pri_code--)
	{
		int ox,oy,col,code,size,w,h,x,y,xa,ya,flipx,flipy,zoomx,zoomy,pri;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		if ((SPRITEWORD(offs) & 0x8000) == 0) continue;

		pri = (SPRITEWORD(offs+0x0c) & 0x03e0) >> 4;	/* ??????? */
		if (pri < min_priority || pri > max_priority) continue;

		size = (SPRITEWORD(offs) & 0x0f00) >> 8;

		w = 1 << (size & 0x03);
		h = 1 << ((size >> 2) & 0x03);

		code = SPRITEWORD(offs+0x02);

		/* the sprite can be start at any point in the 8x8 grid. We have to */
		/* adjust the offsets to draw it correctly. Simpsons does this all the time. */
		xa = 0;
		ya = 0;
		if (code & 0x01) xa += 1;
		if (code & 0x02) ya += 1;
		if (code & 0x04) xa += 2;
		if (code & 0x08) ya += 2;
		if (code & 0x10) xa += 4;
		if (code & 0x20) ya += 4;
		code &= ~0x3f;

		col = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
#if 1
if (keyboard_pressed(KEYCODE_Q) && (SPRITEWORD(offs+0x0c) & 0x8000)) col = rand();
if (keyboard_pressed(KEYCODE_W) && (SPRITEWORD(offs+0x0c) & 0x4000)) col = rand();
if (keyboard_pressed(KEYCODE_E) && (SPRITEWORD(offs+0x0c) & 0x2000)) col = rand();
if (keyboard_pressed(KEYCODE_R) && (SPRITEWORD(offs+0x0c) & 0x1000)) col = rand();
if (keyboard_pressed(KEYCODE_T) && (SPRITEWORD(offs+0x0c) & 0x0800)) col = rand();
if (keyboard_pressed(KEYCODE_Y) && (SPRITEWORD(offs+0x0c) & 0x0400)) col = rand();
if (keyboard_pressed(KEYCODE_U) && (SPRITEWORD(offs+0x0c) & 0x0200)) col = rand();
if (keyboard_pressed(KEYCODE_I) && (SPRITEWORD(offs+0x0c) & 0x0100)) col = rand();
if (keyboard_pressed(KEYCODE_A) && (SPRITEWORD(offs+0x0c) & 0x0080)) col = rand();
if (keyboard_pressed(KEYCODE_S) && (SPRITEWORD(offs+0x0c) & 0x0040)) col = rand();
if (keyboard_pressed(KEYCODE_D) && (SPRITEWORD(offs+0x0c) & 0x0020)) col = rand();
#endif
		ox = (SPRITEWORD(offs+0x06) & 0x3ff) + 16+16;
		oy = 512-(SPRITEWORD(offs+0x04) & 0x3ff) - 128+8;
ox -= 128+128+32;
oy -= 128+8;

		/* zoom control:
		   0x40 = normal scale
		  <0x40 enlarge (0x20 = double size)
		  >0x40 reduce (0x80 = half size)
		*/
		zoomy = SPRITEWORD(offs+0x08);
		if (zoomy > 0x2000) continue;
		if (zoomy) zoomy = (0x400000+zoomy/2) / zoomy;
		else zoomy = 2 * 0x400000;
		if ((SPRITEWORD(offs) & 0x4000) == 0)
		{
			zoomx = SPRITEWORD(offs+0x0a);
			if (zoomx > 0x2000) continue;
			if (zoomx) zoomx = (0x400000+zoomx/2) / zoomx;
			else zoomx = 2 * 0x400000;
		}
		else zoomx = zoomy;

		/* the coordinates given are for the *center* of the sprite */
		ox -= (zoomx * w) >> 13;
		oy -= (zoomy * h) >> 13;

		flipx = SPRITEWORD(offs) & 0x1000;
		flipy = SPRITEWORD(offs) & 0x2000;

		if (zoomx == 0x10000 && zoomy == 0x10000)
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy;

				if (flipy)
					sy = oy + 16 * ((h-1) - y);
				else
					sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					if (flipx)
						sx = ox + 16 * ((w-1) - x);
					else
						sx = ox + 16 * x;

					drawgfx(bitmap,Machine->gfx[0],
							code + xoffset[(x+xa)&7] + yoffset[(y+ya)&7],
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy,zw,zh;

				if (flipy)
				{
					sy = oy + ((zoomy * ((h-1) - y) + (1<<11)) >> 12);
					zh = sy - (oy + ((zoomy * ((h-1) - (y+1)) + (1<<11)) >> 12));
				}
				else
				{
					sy = oy + ((zoomy * y + (1<<11)) >> 12);
					zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;
				}

				for (x = 0;x < w;x++)
				{
					if (flipx)
					{
						sx = ox + ((zoomx * ((w-1) - x) + (1<<11)) >> 12);
						zw = sx - (ox + ((zoomx * ((w-1) - (x+1)) + (1<<11)) >> 12));
					}
					else
					{
						sx = ox + ((zoomx * x + (1<<11)) >> 12);
						zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					}
					drawgfxzoom(bitmap,Machine->gfx[0],
							code + xoffset[(x+xa)&7] + yoffset[(y+ya)&7],
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}

#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(spriteram, spriteram_size, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
}

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

void vendetta_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;
	int pri[3],layer[3];


	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI4);

	K052109_tilemap_update();

	palette_init_used_colors();

	/* palette remapping first */
	{
		unsigned short palette_map[128];
		int color;

		memset (palette_map, 0, sizeof (palette_map));

		/* sprites */
		for (offs = 0;offs < spriteram_size;offs += 16)
		{
			color = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
			}
		}
	}

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = 0;
	pri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	pri[1] = K053251_get_priority(K053251_CI3);
	layer[2] = 2;
	pri[2] = K053251_get_priority(K053251_CI4);

	sortlayers(layer,pri);

	K052109_tilemap_draw(bitmap,layer[0],TILEMAP_IGNORE_TRANSPARENCY);
	simpsons_drawsprites(bitmap,pri[1]+1,pri[0]);
	K052109_tilemap_draw(bitmap,layer[1],0);
	simpsons_drawsprites(bitmap,pri[2]+1,pri[1]);
	K052109_tilemap_draw(bitmap,layer[2],0);
	simpsons_drawsprites(bitmap,0,pri[2]);
}
