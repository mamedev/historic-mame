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

static UINT16 piv_ctrl_reg;
static UINT16 piv_zoom[3],piv_scrollx[3],piv_scrolly[3];

UINT16 wgp_rotate_ctrl[8];

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
	UINT16 tilenum  = wgp_pivram[tile_index + num*0x1000];	// 3 blocks of $2000
	UINT16 attr = wgp_pivram[tile_index + num*0x1000 + 0x8000];	// 3 blocks of $2000
	UINT16 colbank = wgp_pivram[tile_index/4 + num*0x400 + 0x3400] >> 8;	// 3 blocks of $800

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


int wgp_core_vh_start (int tc0100scn_hide_pixels, int piv_xoffs, int piv_yoffs)
{
	/* Up to $1000/8 big sprites, requires 0x200 * sizeof(*spritelist)
	   Multiply this by 32 to give room for the number of small sprites,
	   which are what actually get put in the structure. */

// Plan to move this driver over to pdrawgfx...
//	spritelist = malloc(0x4000 * sizeof(*spritelist));
//	if (!spritelist)
//		return 1;

	if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,tc0100scn_hide_pixels))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	piv_tilemap[0] = tilemap_create(get_piv0_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,64,64);
	piv_tilemap[1] = tilemap_create(get_piv1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	piv_tilemap[2] = tilemap_create(get_piv2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);

	if (!piv_tilemap[0] || !piv_tilemap[1] || !piv_tilemap[2] )
		return 1;

	tilemap_set_transparent_pen( piv_tilemap[1],0 );
	tilemap_set_transparent_pen( piv_tilemap[2],0 );

	/* flipscreen n/a */
	tilemap_set_scrolldx( piv_tilemap[0],-piv_xoffs,0 );
	tilemap_set_scrolldx( piv_tilemap[1],-piv_xoffs,0 );
	tilemap_set_scrolldx( piv_tilemap[2],-piv_xoffs,0 );
	tilemap_set_scrolldy( piv_tilemap[0],-piv_yoffs,0 );
	tilemap_set_scrolldy( piv_tilemap[1],-piv_yoffs,0 );
	tilemap_set_scrolldy( piv_tilemap[2],-piv_yoffs,0 );

	tilemap_set_scroll_rows( piv_tilemap[0],1024 );
	tilemap_set_scroll_rows( piv_tilemap[1],1024 );
	tilemap_set_scroll_rows( piv_tilemap[2],1024 );

	TC0100SCN_set_colbanks(0x80,0xc0,0x40);

	return 0;
}

int wgp_vh_start (void)
{
	return (wgp_core_vh_start(0,32,16));
}

int wgp2_vh_start (void)
{
	return (wgp_core_vh_start(4,32,16));
}

void wgp_vh_stop (void)
{
//	free(spritelist);
//	spritelist = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();
}


/******************************************************************
			PIV TILEMAP READ AND WRITE HANDLERS

Piv Tilemaps
------------

(The unused gaps look as though Taito considered making their
custom chip capable of four rather than three tilemaps.)

500000 - 501fff : unknown/unused
502000 - 507fff : piv tilemaps 0-2 [tile numbers only]

508000 - 50ffff : this area relates to pixel rows in each piv tilemap.
	Includes rowscroll for the piv tilemaps, 2 of which act as a
	simple road. To curve, it has to have rowscroll applied to each
	row.

508000 - 5087ff unknown/unused

508800 piv0 row color bank (& row horizontal zoom?)
509000 piv1 row color bank (& row horizontal zoom?)
509800 piv2 row color bank (& row horizontal zoom?)

	Initially full of 0x007f (0x7f=default row horizontal zoom ?).
	In-game the high bytes are set to various values (0 - 0x2b).
	0xc00 words equates to 3*32x32 words [3 tilemaps x 1024 words].

	The high byte could be a color offset for each 4 tiles, but
	I think it is almost certainly color offset per pixel row. This
	fits in with it living next to the three rowscroll areas (below).
	And it would give a much better illusion of road movement,
	currently pretty poor.

	(I use it as offset for every 4 tiles, as I don't know how
	to make it operate on a pixel row.)

50a000  ? maybe piv0 rowscroll [sky]  (not seen anything here yet)
50c000  piv1 rowscroll [road] (values 0xfd00 - 0x400)
50e000  piv2 rowscroll [road] (values 0xfd00 - 0x403)

510000 - 511fff : unknown/unused
512000 - 517fff : piv tilemaps 0-2 [just tile colors?]

*******************************************************************/

READ16_HANDLER( wgp_pivram_word_r )
{
	return wgp_pivram[offset];
}

WRITE16_HANDLER( wgp_pivram_word_w )
{
	UINT16 oldword = wgp_pivram[offset];
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
	UINT16 a,b;

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
			/* %00111001 */
			piv_ctrl_reg = data;
			break;

		case 0x08:
			// piv 0 zoom? (0x7f = normal, not seen other values)
			piv_zoom[0] = data;
			break;

		case 0x09:
			// piv 1 zoom? (0x7f = normal, values 0 & 0xff7f-ffbc in Wgp2)
			piv_zoom[1] = data;
			break;

		case 0x0a:
			// piv 2 zoom? (0x7f = normal, values 0 & 0xff7f-ffbc in Wgp2)
			piv_zoom[2] = data;
			break;
	}
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/

READ16_HANDLER( wgp_spritemap_word_r )
{
	return wgp_spritemap[offset];
}

WRITE16_HANDLER( wgp_spritemap_word_w )
{
	COMBINE_DATA(&wgp_spritemap[offset]);
}


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

	for (offs = 0;offs <0x200;offs += 1)	/* Check active sprites area */
	{
		code = (spriteram16[0xe00+offs]);

		if (code)	/* Active sprite ? */
		{
			i = (code << 3) &0xfff;

			bigsprite = spriteram16[i + 2] &0x3fff;
			map_index = bigsprite << 1;

			for (i=0;i<16;i++)
			{
				tile  = wgp_spritemap[(map_index + (i << 1))] &tile_mask;
				color = wgp_spritemap[(map_index + (i << 1) + 1)] &0xf;

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

Implement rotation/zoom properly. (Screen shots show that
close-to sprites aren't big enough.)

Sprite/piv priority: two later circuits have wide sprites which
wander up the screen once per lap, and look like they ought to
be *under* the 2 road piv layers.

Wgp1 has some junky brown sprites in-game. I think they are meant
to be mud banks on the track edge.

(They are indexed 0xe720-e790. 0x2720*4 => 0x9c80 thru 9e80 is
the spritemap area defining these junk sprites. Data there is
read in from the data rom, so perhaps that is a partial bad dump.
The odd thing in this spritemap area is the presence of tile
numbers 1,2,4,8: these are the dark brown lumps which appear
in the junk sprites. Even zeroing those, many of the remaining
tiles look wrong.)

Sprite colors ok? (dust after crash in Wgp2 is odd; some
black/grey barrels on late Wgp circuit also look strange)


Memory Map
----------

400000 - 40bfff : Sprite tile mapping area

	Tile numbers (0-0x3fff) alternate with word containing
	tile color/unknown bits.

	xxxxxxxx x.x.....  unused ??
	........ .x......  unknown (Wgp2 only)
	........ ...x....  unknown (Wgp2 only)
	........ ....cccc  color (0-15)

	Tile map for each big sprite is 64 bytes.
	(= 32 words = 16 tiles: every big sprite comprises 4x4
	16x16 tiles so is 64x64 pixels)

40c000 - 40dbff : Sprite Table

	Every 16 bytes contains one sprite entry. First entry is
	ignored [as 0 in active sprites list means no sprite].

	(0x1c0 [no.of entries] * 0x40 [bytes for big sprite] = 0x6fff
	of sprite tile mapping area can be addressed at any one time.)

	Sprite entry (preliminary)
	------------
      +0x00  x pos (signed)
      +0x02  y pos (signed)
      +0x04  index to tile mapping area [2 msbs always set?]

		(400000 + (index &0x3fff) * 4) points to relevant part
		of sprite tile mapping area. Can't have index > 0x2fff

      +0x06  ? usually around 1 thru 0x5f, maybe zoom.
		  0x3f (63) = standard ?
	+0x08  rotation? (usually stuck at 0xf800)
	+0x0a  rotation? (usually stuck at 0xf800)
      +0x0c  zoom? around 0x1400 means sprite is small,
	        0x400 = standard size, <0x400 and it's magnified
	+0x0e  non-zero only during rotation (?)

	(No longer used entries often have 0xfff6 in +0x06 and +0x08.)

	Only 2 rotation examples (i) at 40c000, when Taito
	logo displayed (Wgp only). (ii) the stage with rain.
	Other in-game sprites are simply using +0x06 and +0x0c,
	i.e. zoom.

	So the sprite rotation in Wgp screenshots must be a *blanket*
	rotate effect, identical to the one applied to piv layers.
	This explains why sprite/piv positions are mostly okay
	despite my failure to implement rotation.

40dc00 - 40dfff: Active Sprites list

	Each word is a sprite number, 0x0 thru 0x1bf. If !=0
	a word makes active the 0x10 bytes of sprite data at
	(40c000 + sprite_num * 0x10). (Wgp2 fills this in reverse).

40fff0: Unknown (sprite control word?)

	Wgp alternates 0x8000 and 0. Wgp2 only pokes 0.

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
	int x,y,bigsprite;
	int col,flipx,flipy,zoomx,zoomy;
	int curx,cury,zx,zy;
	UINT16 map_index;
	UINT16 rotate=0;
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;

	for (offs = 0;offs <0x200;offs += 1)
	{
		code = (spriteram16[0xe00+offs]);

		if (code)	/* do we have an active sprite ? */
		{
			i = (code << 3) &0xfff;	/* yes, so we look up its sprite entry */

			x = spriteram16[i];
			y = spriteram16[i + 1];
			bigsprite = spriteram16[i + 2] &0x3fff;

			/* The last five words [i + 3 thru 7] must be zoom/rotation
			   control: for time being we kludge zoom using 1 word.
			   Int timing must be bad: the fff6/0 "dead" sprites should
			   surely not be referenced by the active sprites list. */

if ((spriteram16[i + 4]=0xfff6) && (spriteram16[i + 5]=0))
	continue;

if (((spriteram16[i + 4]!=0xf800) && (spriteram16[i + 4]!=0xfff6))
	|| ((spriteram16[i + 5]!=0xf800) && (spriteram16[i + 5]!=0))
	|| spriteram16[i + 7]!=0)

	rotate = i << 1;

	/***** Begin zoom kludge ******/
			zoomx = (spriteram16[i + 3] &0x7f) + 1;
			zoomy = (spriteram16[i + 3] &0x7f) + 1;

			y -=4;
			y -=((0x40-zoomy)/4);	// distant sprites were some 16 pixels too far down
	/****** end zoom kludge *******/

			/* Treat coords as signed */
			if (x & 0x8000)  x -= 0x10000;
			if (y & 0x8000)  y -= 0x10000;

			map_index = bigsprite << 1;	/* now we can access the sprite tile map */

			for (i=0;i<16;i++)
			{
				code = wgp_spritemap[(map_index + (i << 1))] &tile_mask;
				col  = wgp_spritemap[(map_index + (i << 1) + 1)] &0xf;

				flipx=0;	// no flip xy if sprites are rotatable?
				flipy=0;

				k = xlookup[i];	// assumes no xflip
				j = ylookup[i];	// assumes no yflip

				curx = x + ((k*zoomx)/4);
				cury = y + ((j*zoomy)/4);

				zx= x + (((k+1)*zoomx)/4) - curx;
				zy= y + (((j+1)*zoomy)/4) - cury;

				drawgfxzoom(bitmap, Machine->gfx[0],
						code,
						col,
						flipx, flipy,
						curx,cury,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						zx << 12, zy << 12);
			}
		}

	}
#if 0
	if (rotate)
	{
		char buf[80];
		sprintf(buf,"sprite rotate offs %04x ?",rotate);
		usrintf_showmessage(buf);
	}
#endif
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

	for (j = 0;j < 512;j++)		/* sky layer */
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

/// A few sprites may need to be drawn here (under road) ///

#ifdef MAME_DEBUG
	if (dislayer[1]==0)
#endif
	tilemap_draw(bitmap,piv_tilemap[1],0,0);

#ifdef MAME_DEBUG
	if (dislayer[2]==0)
#endif
	tilemap_draw(bitmap,piv_tilemap[2],0,0);

	wgp_draw_sprites(bitmap,0,16);

/// Here we should apply rotation from wgp_rotate_ctrl[] to entire bitmap ///
/// before we draw TC0100SCN layers

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
// Use pdrawgfx for sprites when sprite/tile priority understood...
//		int primasks[2] = {0xf0,0xfc};
	}

#if 0
	{
		char buf[80];

		sprintf(buf,"piv_ctrl_reg: %04x zoom: %04x %04x %04x",piv_ctrl_reg,piv_zoom[0],piv_zoom[1],piv_zoom[2]);
		usrintf_showmessage(buf);
	}
#endif

/* Enable this to watch the rotation control words */
#if 0
	{
		char buf[80];
		int i;

		for (i = 0; i < 8; i += 1)
		{
			sprintf (buf, "%02x: %04x", i, wgp_rotate_ctrl[i]);
			ui_text (Machine->scrbitmap, buf, 0, i*8);
		}
	}
#endif
}

