#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int bg_colorbase,sprite_colorbase,layer_colorbase[3];

#define TILEROM_MEM_REGION 1

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*code |= ((*color & 0x3f) << 8) | (bank << 14);
	*color = ((*color & 0xc0) >> 6);
	*color += layer_colorbase[layer];
}


/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int simpsons_vh_start( void )
{
	spriteram_size = 0x1000;

	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback) != 0)
		return 1;

	return 0;
}

void simpsons_vh_stop( void )
{
	K052109_vh_stop();
}



/*
 * Sprite Format
 * ------------------
 *
 * Byte | Bit(s)   | Use
 * -----+-76543210-+----------------
 *   0  | x------- | active (show this sprite)
 *   0  | -x------ | maintain aspect ratio. When 1, zoom y applies to both axis
 *   0  | --x----- | flip y
 *   0  | ---x---- | flip x
 *   0  | ----xxxx | sprite size (see below)
 *   1  | xxxxxxxx | priority order
 *   2  | xxxxxxxx | sprite code (high 8 bits)
 *   3  | xxxxxxxx | sprite code (low 8 bits)
 *   4  | ------xx | y position (high bits)
 *   5  | xxxxxxxx | y position (low 8 bits)
 *   6  | ------xx | x position (high bits)
 *   7  | xxxxxxxx | x position (low 8 bits)
 *   8  | xxxxxxxx | y position (high bits, are they all used?)
 *   9  | xxxxxxxx | y position (low 8 bits)
 *  10  | xxxxxxxx | x position (high bits, are they all used?)
 *  11  | xxxxxxxx | x position (low 8 bits)
 *  12  | x------- | mirror sprite vertically?
 *  12  | ----xxxx | priority
 *  13  | ----xxxx | color
 *  14  | xxxxxxxx | unused, it seems
 *  15  | xxxxxxxx | unused, it seems
 *
 * I haven't found the "shadow" bit!
 */

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

		pri = (SPRITEWORD(offs+0x0c) & 0x0f80) >> 6;	/* ??????? */
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

		col = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x000f);

//	specialflipy = SPRITEWORD(offs+0x0c) & 0x8000;
#if 0
if (keyboard_pressed(KEYCODE_Q) && (SPRITEWORD(offs+0x02) & 0x8000)) col = rand();
if (keyboard_pressed(KEYCODE_W) && (SPRITEWORD(offs+0x04) & 0xfc00)) col = rand();
if (keyboard_pressed(KEYCODE_E) && (SPRITEWORD(offs+0x06) & 0xfc00)) col = rand();
if (keyboard_pressed(KEYCODE_R) && (SPRITEWORD(offs+0x0e) & 0xffff)) col = rand();
#endif
		ox = (SPRITEWORD(offs+0x06) & 0x3ff) + 16+16;
		oy = 512-(SPRITEWORD(offs+0x04) & 0x3ff) - 128+8;

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

				sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 16 * x;
					if (flipx) c += xoffset[(w-1-x+xa)&7];
					else c += xoffset[(x+xa)&7];
					if (flipy) c += yoffset[(h-1-y+ya)&7];
					else c += yoffset[(y+ya)&7];

					drawgfx(bitmap,Machine->gfx[0],
							c,
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

				sy = oy + ((zoomy * y + (1<<11)) >> 12);
				zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoomx * x + (1<<11)) >> 12);
					zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					if (flipx) c += xoffset[(w-1-x+xa)&7];
					else c += xoffset[(x+xa)&7];
					if (flipy) c += yoffset[(h-1-y+ya)&7];
					else c += yoffset[(y+ya)&7];

					drawgfxzoom(bitmap,Machine->gfx[0],
							c,
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
}

void simpsons_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;
	int pri0,pri1,pri2,prib;


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
			color = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x000f);
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

	palette_used_colors[16 * bg_colorbase] |= PALETTE_COLOR_VISIBLE;

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	prib = K053251_get_priority(K053251_CI0);
	pri1 = K053251_get_priority(K053251_CI3);
	pri2 = K053251_get_priority(K053251_CI4);
	pri0 = K053251_get_priority(K053251_CI2);

	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],&Machine->drv->visible_area);
	simpsons_drawsprites(bitmap,pri1+1,prib);
	K052109_tilemap_draw(bitmap,1,0);
	simpsons_drawsprites(bitmap,pri2+1,pri1);
	K052109_tilemap_draw(bitmap,2,0);
	simpsons_drawsprites(bitmap,pri0+1,pri2);
	K052109_tilemap_draw(bitmap,0,0);
	simpsons_drawsprites(bitmap,0,pri0);

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

/***************************************************************************

  Extra video banking

***************************************************************************/

static int simpsons_K052109_r( int offset ) { return K052109_r( offset + 0x2000 ); }
static void simpsons_K052109_w( int offset, int data ) { K052109_w( offset + 0x2000, data ); }

void simpsons_video_banking( int select )
{

	switch( select )
	{
		case 0x00: /* 052109 */
			cpu_setbankhandler_r( 3, K052109_r );
			cpu_setbankhandler_w( 3, K052109_w );
			cpu_setbankhandler_r( 4, simpsons_K052109_r );
			cpu_setbankhandler_w( 4, simpsons_K052109_w );
		break;

		case 0x01: /* palette ram */
			cpu_setbankhandler_r( 3, paletteram_r );
			cpu_setbankhandler_w( 3, paletteram_xBBBBBGGGGGRRRRR_swap_w );
			cpu_setbankhandler_r( 4, simpsons_K052109_r );
			cpu_setbankhandler_w( 4, simpsons_K052109_w );
		break;

		case 0x02: /* sprite ram */
			cpu_setbankhandler_r( 3, K052109_r );
			cpu_setbankhandler_w( 3, K052109_w );
			cpu_setbankhandler_r( 4, spriteram_r );
			cpu_setbankhandler_w( 4, spriteram_w );
		break;

		case 0x03: /* both */
			cpu_setbankhandler_r( 3, paletteram_r );
			cpu_setbankhandler_w( 3, paletteram_xBBBBBGGGGGRRRRR_swap_w );
			cpu_setbankhandler_r( 4, spriteram_r );
			cpu_setbankhandler_w( 4, spriteram_w );
		break;
	}
}
