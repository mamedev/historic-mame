#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

static struct tilemap *piv_tilemap[3];

/*
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
*/

data16_t *wgp_spritemap;
size_t wgp_spritemap_size;
data16_t *wgp_pivram;
data16_t *wgp_piv_ctrlram;
int wgp_hide_pixels;

static UINT16 piv_ctrl_reg;
static UINT16 piv_zoom[3],piv_scrollx[3],piv_scrolly[3];


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
				if ((mwa->handler == TC0110PCR_step1_word_w) || (mwa->handler == TC0110PCR_step1_rbswap_word_w))
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}


static void common_get_piv_tile_info(int num,int tile_index)
{
	int tilenum  = wgp_pivram[tile_index + num*0x1000];	// 3 blocks of $2000
	int attr = wgp_pivram[tile_index + num*0x1000 + 0x8000];	// 3 blocks of $2000
	int colbank = wgp_pivram[tile_index/4 + num*0x400 + 0x3400] >> 8;	// 3 blocks of $800

	int a = (colbank &0xe0);	// keep 3 msbs
	colbank = ((colbank &0xf) << 1) | a;	// shift 4 lsbs up

	SET_TILE_INFO(2, tilenum & 0x3fff, colbank + (attr & 0x3f));	// hmm, attr &0x1 ?
	tile_info.flags = TILE_FLIPYX( (attr & 0xc0) >> 6);
}

static void get_piv0_tile_info(int tile_index)
{
	common_get_piv_tile_info(0,tile_index);
}

static void get_piv1_tile_info(int tile_index)
{
	common_get_piv_tile_info(1,tile_index);
}

static void get_piv2_tile_info(int tile_index)
{
	common_get_piv_tile_info(2,tile_index);
}


int wgp_core_vh_start (void)
{
	/* Up to $1000/8 big sprites, requires 0x200 * sizeof(*spritelist)
	   Multiply this by 32 to give room for the number of small sprites,
	   which are what actually get put in the structure. */

// Plan to move this driver over to pdrawgfx...
//	spritelist = malloc(0x4000 * sizeof(*spritelist));
//	if (!spritelist)
//		return 1;

	if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,wgp_hide_pixels))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	piv_tilemap[0] = tilemap_create(get_piv0_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,64,64);
	piv_tilemap[1] = tilemap_create(get_piv1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	piv_tilemap[2] = tilemap_create(get_piv2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);

	if (!piv_tilemap[0] || !piv_tilemap[1] || !piv_tilemap[2] )
		return 1;

	tilemap_set_transparent_pen(piv_tilemap[1],0);
	tilemap_set_transparent_pen(piv_tilemap[2],0);

	/* values seem about right, flipscreen n/a */
	tilemap_set_scrolldx( piv_tilemap[0],-32,0 );
	tilemap_set_scrolldx( piv_tilemap[1],-32,0 );
	tilemap_set_scrolldx( piv_tilemap[2],-32,0 );
	tilemap_set_scrolldy( piv_tilemap[0],-16,0 );
	tilemap_set_scrolldy( piv_tilemap[1],-16,0 );
	tilemap_set_scrolldy( piv_tilemap[2],-16,0 );

	tilemap_set_scroll_rows( piv_tilemap[0],1024 );
	tilemap_set_scroll_rows( piv_tilemap[1],1024 );
	tilemap_set_scroll_rows( piv_tilemap[2],1024 );

	TC0100SCN_set_colbanks(0x80,0xc0,0x40);

	return 0;
}

int wgp_vh_start (void)
{
	wgp_hide_pixels = 0;

	return (wgp_core_vh_start());
}

void wgp_vh_stop (void)
{
//	free(spritelist);
//	spritelist = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();
}


/********************************************************
          TILEMAP READ AND WRITE HANDLERS
********************************************************/

READ16_HANDLER( wgp_spritemap_word_r )
{
	return wgp_spritemap[offset];
}

WRITE16_HANDLER( wgp_spritemap_word_w )
{
	COMBINE_DATA(&wgp_spritemap[offset]);
}

READ16_HANDLER( wgp_pivram_word_r )
{
	return wgp_pivram[offset];
}

WRITE16_HANDLER( wgp_pivram_word_w )
{
	data16_t oldword = wgp_pivram[offset];
	COMBINE_DATA(&wgp_pivram[offset]);

	if (offset<0x3000)
	{
		if (oldword != wgp_pivram[offset])
			tilemap_mark_tile_dirty(piv_tilemap[(offset / 0x1000)], (offset % 0x1000) );
	}
	else if ((offset >=0x3400) && (offset<0x4000))
	{
		if (oldword != wgp_pivram[offset])
			tilemap_mark_tile_dirty(piv_tilemap[((offset - 0x3400) / 0x400)], (offset % 0x400)*4 );
			tilemap_mark_tile_dirty(piv_tilemap[((offset - 0x3400) / 0x400)], (offset % 0x400)*4 + 1 );
			tilemap_mark_tile_dirty(piv_tilemap[((offset - 0x3400) / 0x400)], (offset % 0x400)*4 + 2 );
			tilemap_mark_tile_dirty(piv_tilemap[((offset - 0x3400) / 0x400)], (offset % 0x400)*4 + 3 );
	}
	else if ((offset >=0x8000) && (offset<0xb000))
	{
		if (oldword != wgp_pivram[offset])
			tilemap_mark_tile_dirty(piv_tilemap[((offset - 0x8000)/ 0x1000)], (offset % 0x1000) );
	}
}

READ16_HANDLER( wgp_piv_ctrl_word_r )
{
	return wgp_piv_ctrlram[offset];
}

WRITE16_HANDLER( wgp_piv_ctrl_word_w )
{
	int a,b;

	COMBINE_DATA(&wgp_piv_ctrlram[offset]);
	data = wgp_piv_ctrlram[offset];

	switch (offset)
	{
		case 0x00:
			a = -data;
			b = (a &0xffe0) >> 1;
			piv_scrollx[0] = (a &0xf) | b;
			break;

		case 0x01:
			a = -data;
			b = (a &0xffe0) >> 1;
			piv_scrollx[1] = (a &0xf) | b;
			break;

		case 0x02:
			a = -data;
			b = (a &0xffe0) >> 1;
			piv_scrollx[2] = (a &0xf) | b;
			break;

		case 0x03:
			piv_scrolly[0] = data;
			break;

		case 0x04:
			piv_scrolly[1] = data;
			break;

		case 0x05:
			piv_scrolly[2] = data;
			break;

		case 0x06:
			/* looks like overall control reg, apparently always 0x39 */
			piv_ctrl_reg = data;
			break;

		case 0x08:
			// piv 0 zoom? (0x7f = normal, not seen any other values)
			piv_zoom[0] = data;
			break;

		case 0x09:
			// piv 1 zoom?
			piv_zoom[1] = data;
			break;

		case 0x0a:
			// piv 2 zoom?
			piv_zoom[2] = data;
			break;
	}
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/

// none //


/*********************************************************
				PALETTE
*********************************************************/

void wgp_update_palette (void)
{
	int i,j;
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;
	int offs,code,tile,color;
	int bigsprite,map_index;
	unsigned short palette_map[256];

	memset (palette_map, 0, sizeof (palette_map));

	/* Do sprite palette */

	for (offs = 0;offs <0x1c0;offs += 1)	// Up to 0x1c0 active sprites
	{
		code = (spriteram16[0xe00+offs]);	// $1c00/2

		if (code)
		{
			i = code << 3;

			i &= 0xfff;	// Needed as next line may page-fault entering service mode

			bigsprite = spriteram16[i + 2] &0x3fff;

			map_index = bigsprite << 1;

			for (i=0;i<16;i++)
			{
				tile  = wgp_spritemap[(map_index + (i << 1))] &tile_mask;
				color = wgp_spritemap[(map_index + (i << 1) + 1)] &0xff;

				palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
			}
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


/****************************************************************
			SPRITE DRAW ROUTINES

TODO
====

Some junky looking brown sprites in-game; perhaps meant to be mask
sprites? Or a fault in the spritemap decoding?

Map
===

400000 - 40bfff : big sprite tile mapping area. Tile numbers
	alternate with word containing color byte.

	Tile map for each big sprite is 64 bytes.
	(= 32 words = 16 tiles: every big sprite comprises 4x4
	16x16 tiles i.e. is 64x64 pixels in size)

40c000 - 40dbff : sprite table. Every 16 bytes is one sprite entry.
	0x1c0 [no.of entries] * 0x40 [bytes for big sprite] = 0x6fff
	of big sprite mapping area can be addressed at any one time.
	First entry is ignored [as 0 in active sprites list means
	no sprite].

	Sprite entry (preliminary)
	------------
      +0x00  x pos (signed)
      +0x02  y pos (signed)
      +0x04  index to tile mapping area [2 msbs *always* set]
      +0x06  ? usually around 1 thru 0x5f, maybe zoom.
		  0x3f (63) = standard ?
	+0x08  rotation, inc xy? (usually stuck at 0xf800)
	+0x0a  rotation, inc yx? (usually stuck at 0xf800)
      +0x0c  zoom: around 0x1400 means sprite is small,
	        0x400 = standard size, <0x400 and it's magnified
	+0x0e  non-zero only during rotation (?)

	For rotation example look at 40c000 when Taito logo
	displayed.

	(Unused entries often have 0xfff6 in +0x06 and +0x08.)

	(400000 + (index &0x3fff) * 4) points to relevant stuff
	in big sprite tile mapping area. Any index > 0x2fff would
	obviously be invalid.

40dc00 - 40df7f	(dbff-c000/0x10 * 2 = max possible extent)
	active sprites list, each word is a sprite number. If !=0
	a word makes active the 0x10 bytes of sprite data at
	(40c000 + sprite_num * 0x10).

****************************************************************/

/* Sprite tilemapping area doesn't have a straightforward
   structure for each big sprite, so we use a lookup table. */

static UINT8 xlookup[16] =
	{ 0, 1, 0, 1,
	  2, 3, 2, 3,
	  0, 1, 0, 1,
	  2, 3, 2, 3 };

static UINT8 ylookup[16] =
	{ 0, 0, 1, 1,
	  0, 0, 1, 1,
	  2, 2, 3, 3,
	  2, 2, 3, 3 };

static void wgp_draw_sprites(struct osd_bitmap *bitmap,int *primasks,int y_offs)
{
	int offs,code,i,j,k;
	int x,y,bigsprite,map_index;
	int col,flipx,flipy,zoomx,zoomy;
	int curx,cury,zx,zy;

	for (offs = 0;offs <0x1c0;offs += 1)	// Up to 0x1c0 active sprites
	{
		code = (spriteram16[0xe00+offs]);	// $1c00/2

		if (code)	/* do we have an active sprite ? */
		{
			i = code << 3;	/* yes, so we look up its sprite entry */

			i &= 0xfff;	// Needed as next line may page-fault entering service mode

			x = spriteram16[i];
			y = spriteram16[i + 1];
			bigsprite = spriteram16[i + 2] &0x3fff;

			/* The last five words [i + 3 thru 7] seem to be
			   zoom/rotation control, not properly implemented */

			zoomx = (spriteram16[i + 3] &0x7f) + 1;
			zoomy = (spriteram16[i + 3] &0x7f) + 1;

			y -=4;
			y -=((0x40-zoomy)/4);	// distant sprites were some 16 pixels too far down

			/* Treat coords as signed */
			if (x & 0x8000)  x -= 0x10000;
			if (y & 0x8000)  y -= 0x10000;

			map_index = bigsprite << 1;	/* now we can access the sprite tile map */

			for (i=0;i<16;i++)
			{
				code = wgp_spritemap[(map_index + (i << 1))];
				col  = wgp_spritemap[(map_index + (i << 1) + 1)];

				flipx=0;	// ? where are flip xy
				flipy=0;

				k = xlookup[i];	// assumes no xflip
				j = ylookup[i];	// assumes no yflip

				curx = x + ((k*zoomx)/4);
				cury = y + ((j*zoomy)/4);

				zx= x + (((k+1)*zoomx)/4) - curx;
				zy= y + (((j+1)*zoomy)/4) - cury;

				drawgfxzoom(bitmap, Machine->gfx[0],
						code & 0x3fff,
						col & 0xff,
						flipx, flipy,
						curx,cury,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						zx << 12, zy << 12);
			}
		}

	}
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void wgp_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int a,b,i,j,layer[3];

#ifdef MAME_DEBUG
	static int dislayer[4];
	char buf[80];
#endif

#ifdef MAME_DEBUG
	if (keyboard_pressed (KEYCODE_V))
	{
		while (keyboard_pressed (KEYCODE_V) != 0) {};
		dislayer[0] ^= 1;
		sprintf(buf,"piv0: %01x",dislayer[0]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed (KEYCODE_B))
	{
		while (keyboard_pressed (KEYCODE_B) != 0) {};
		dislayer[1] ^= 1;
		sprintf(buf,"piv1: %01x",dislayer[1]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed (KEYCODE_N))
	{
		while (keyboard_pressed (KEYCODE_N) != 0) {};
		dislayer[2] ^= 1;
		sprintf(buf,"piv2: %01x",dislayer[2]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed (KEYCODE_M))
	{
		while (keyboard_pressed (KEYCODE_M) != 0) {};
		dislayer[3] ^= 1;
		sprintf(buf,"TC0100SCN top bg layer: %01x",dislayer[3]);
		usrintf_showmessage(buf);
	}
#endif

	for (i=0;i<3;i++)
	{
		tilemap_set_scrolly(piv_tilemap[i], 0, piv_scrolly[i]);
	}

	/* Results from rowscroll seem ok. Rowscroll values from 0xfd00
	   thru 0x403 seen. As per other x axis stuff in the piv layers
	   we have to knock out bit 4. So the range is *really* only
	   +-0x200, 512 pixels either way: what you'd expect for a
	   1024x1024 pixel tilemap. */

	for (j = 0;j < 512;j++)		/* this layer is sky */
	{
		a = wgp_pivram[0x4000 + ((j + 16 + piv_scrolly[0]) &0x3ff)];
		b = (a &0xffe0) >> 1;
		a = (a &0xf) | b;

		tilemap_set_scrollx(piv_tilemap[0],
				(j + 16 + piv_scrolly[0]) & 0x3ff,
				piv_scrollx[0] - a);
	}

	for (j = 0;j < 512;j++)		/* road layer */
	{
		a = wgp_pivram[0x5000 + ((j + 16 + piv_scrolly[1]) &0x3ff)];
		b = (a &0xffe0) >> 1;
		a = (a &0xf) | b;

		tilemap_set_scrollx(piv_tilemap[1],
				(j + 16 + piv_scrolly[1]) & 0x3ff,
				piv_scrollx[1] - a);
	}

	for (j = 0;j < 512;j++)		/* road layer */
	{
		a = wgp_pivram[0x6000 + ((j + 16 + piv_scrolly[2]) &0x3ff)];
		b = (a &0xffe0) >> 1;
		a = (a &0xf) | b;

		tilemap_set_scrollx(piv_tilemap[2],
				(j + 16 + piv_scrolly[2]) & 0x3ff,
				piv_scrollx[2] - a);
	}

	tilemap_update(piv_tilemap[0]);
	tilemap_update(piv_tilemap[1]);
	tilemap_update(piv_tilemap[2]);

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	wgp_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

#ifdef MAME_DEBUG
	if (dislayer[0]==0)
#endif
	tilemap_draw(bitmap,piv_tilemap[0],0,0);

#ifdef MAME_DEBUG
	if (dislayer[1]==0)
#endif
	tilemap_draw(bitmap,piv_tilemap[1],0,0);

#ifdef MAME_DEBUG
	if (dislayer[2]==0)
#endif
	tilemap_draw(bitmap,piv_tilemap[2],0,0);

	wgp_draw_sprites(bitmap,0,16);

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],0,0);

#ifdef MAME_DEBUG
	if (dislayer[3]==0)
#endif
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);

	{
// May be possible to use pdrawgfx for sprites when sprite/tile priority understood...
//		int primasks[2] = {0xf0,0xfc};
	}

#if 0
	{
		char buf[80];

		sprintf(buf,"piv_ctrl_reg: %04x zoom: %04x %04x %04x",piv_ctrl_reg,piv_zoom[0],piv_zoom[1],piv_zoom[2]);
		usrintf_showmessage(buf);
	}
#endif
}

