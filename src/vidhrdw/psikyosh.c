/*
TODO:

Psikyo PS6406B (PS3v1/PS5):
I *think* capabilities include brightness control (theres an area which increases / decreases in value on scene changes, -dh
These values turned out to be alpha for sprites. -pjp

is this table of 256 16-bit values (of the series 1/x) being used correctly as a slope for sprite zoom? -pjp

rowscroll / column scroll (s1945ii test and level 7 uses a whole block of ram for scrolling,  -dh
(224 values for both x and y scroll) Also used for daraku text layers, again only yscroll differ
Also, xscroll values are always the same, maybe the hw can't do simultaneous line/columnscroll. -pjp

figure out how the text layers work correctly, dimensions are different (even more tilemaps needed)
daraku seems to use tilemaps only for text layer (hi-scores, insert coin, warning message, test mode, psikyo (c)) how this is used is uncertain,

sprite on bg priorites

is the new alpha effect configurable? there is still a couple of unused vid regs. -pjp

flip screen, located but not implemented. wait until tilemaps.

the stuff might be converted to use the tilemaps once all the features is worked out ...
complicated by the fact that the whole tilemap will have to be marked dirty each time the bank changes (this can happen once per frame, unless a tilemap is allocated for each bank.
18 + 9 = 27 tilemaps (including both sizes, possibly another 8 if the large tilmaps can start on odd banks).
Would also need to support TRANSPARENCY_ALPHARANGE

there is possibly a bg colour selector

sol divide doesn't seem to make much use of tilemaps at all, it uses them to fade between scenes in the intro


Psikyo PS6807 (PS4):
should brightness be logarithmic? Loderndf draws level whilst increasing brightness and you can still see it. -pjp

flip screen, seperate for each of the screens.

sprite colours not 100%?
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "psikyosh.h"

static UINT32 screen; /* for PS4 games when DUAL_SCREEN=0 */

/* Shamelessly stolen from vidhrdw/xexex.c */
/* useful function to sort the three tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayers(int *layer, int *pri)
{
#define SWAP(a,b) \
	if (pri[a] > pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(1, 2)
}


/* Psikyo PS6406B */
/* --- BACKGROUNDS --- */

/* 'Normal' layers, no line/columnscroll. Can have alpha */
static void psikyosh_drawbglayer( int layer, struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	struct GfxElement *gfx;
	int offs=0, sx, sy;
	int scrollx, scrolly, bank, alpha, alphamap, trans, size, width;

	if ( BG_TYPE(layer) == BG_NORMAL_ALT )
	{
		bank    = (psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x000000ff) >> 0;
		alpha   = (psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x00003f00) >> 8;
		alphamap =(psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x00008000) >> 15;
		scrollx = (psikyosh_bgram[0x1bf0/4 + (layer*0x04)/4] & 0x000001ff) >> 0;
		scrolly = (psikyosh_bgram[0x1bf0/4 + (layer*0x04)/4] & 0x01ff0000) >> 16;
	}
	else /* BG_NORMAL */
	{
		bank    = (psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x000000ff) >> 0;
		alpha   = (psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x00003f00) >> 8;
		alphamap =(psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x00008000) >> 15;
		scrollx = (psikyosh_bgram[0x13f0/4 + (layer*0x04)/4] & 0x000001ff) >> 0;
		scrolly = (psikyosh_bgram[0x13f0/4 + (layer*0x04)/4] & 0x01ff0000) >> 16;
	}

	if ( BG_TYPE(layer) == BG_SCROLL_0D ) scrollx += 0x08; /* quick kludge until using rowscroll */

	gfx = BG_DEPTH_8BPP(layer) ? Machine->gfx[1] : Machine->gfx[0];
	size = BG_LARGE(layer) ? 32 : 16;
	width = BG_LARGE(layer) ? 0x200 : 0x100;

	if(alphamap) { /* alpha values are per-pen */
		trans = TRANSPARENCY_ALPHARANGE;
	} else if(alpha) {
		trans = TRANSPARENCY_ALPHA;
		alpha = ((0x3f-alpha)*0xff)/0x3f; /* 0x3f-0x00 maps to 0x00-0xff */
		alpha_set_level(alpha);
	} else {
		trans = TRANSPARENCY_PEN;
	}

	if((bank>=0x0c) && (bank<=0x1f)) /* shouldn't happen, 19 banks of 0x800 bytes */
	{
		for (sy=0; sy<size; sy++)
		{
			for (sx=0; sx<32; sx++)
			{
				int tileno, colour;

				tileno = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0x0007ffff); /* seems to take into account spriteram, hence -0x4000 */
				colour = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0xff000000) >> 24;

				drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&(width-1)),cliprect,trans,0); /* normal */
				if(scrollx)
					drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&(width-1)),cliprect,trans,0); /* wrap x */
				if(scrolly)
					drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&(width-1))-width,cliprect,trans,0); /* wrap y */
				if(scrollx && scrolly)
					drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&(width-1))-width,cliprect,trans,0); /* wrap xy */

				offs++;
			}
		}
	}
}

/* Line Scroll and/or Column Scroll, don't think it can have alpha. This isn't correct, just testing */
static void psikyosh_drawbglayerscroll( int layer, struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	struct GfxElement *gfx;
	int offs, sx, sy;
	int bank, width, scrollbank, size;

	scrollbank = BG_TYPE(layer); /* Scroll bank appears to be same as layer type */

	/* Daraku sets bank in same reg as normal layers, s1945ii doesn't */
	if( (BG_TYPE(layer) == BG_SCROLL_0C) || (BG_TYPE(layer) == BG_SCROLL_0D) )
		bank = (psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x000000ff) >> 0;
	else
		bank = BG_TYPE(layer) - 0x02;

	gfx = BG_DEPTH_8BPP(layer) ? Machine->gfx[1] : Machine->gfx[0];
	size = BG_LARGE(layer) ? 32 : 16;
	width = BG_LARGE(layer) ? 0x200 : 0x100;

	if((bank>=0x0c) && (bank<=0x1f)) /* shouldn't happen, 19 banks of 0x800 bytes */
	{
		int bg_scrollx[512], bg_scrolly[512];

		fillbitmap(tmpbitmap, get_black_pen(), NULL);

		for (offs=0; offs<(0x400/4); offs++) /* 224 values for each, or is it 256? */
		{
			bg_scrollx[2*offs] = (psikyosh_bgram[(scrollbank*0x800)/4 + offs - 0x4000/4] & 0x000001ff) >> 0; /* seems to take into account spriteram, hence -0x4000 */
			bg_scrolly[2*offs] = (psikyosh_bgram[(scrollbank*0x800)/4 + offs - 0x4000/4] & 0x01ff0000) >> 16; /* seems to take into account spriteram, hence -0x4000 */
			bg_scrollx[2*offs+1] = (psikyosh_bgram[(scrollbank*0x800)/4 + offs - 0x4000/4] & 0x000001ff) >> 0; /* seems to take into account spriteram, hence -0x4000 */
			bg_scrolly[2*offs+1] = (psikyosh_bgram[(scrollbank*0x800)/4 + offs - 0x4000/4] & 0x01ff0000) >> 16; /* seems to take into account spriteram, hence -0x4000 */
		}

		offs=0;
		for ( sy=0; sy<size; sy++)
		{
			for (sx=0; sx<32; sx++)
			{
				int tileno, colour;

				tileno = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0x0007ffff); /* seems to take into account spriteram, hence -0x4000 */
				colour = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0xff000000) >> 24;

				drawgfx(tmpbitmap,gfx,tileno,colour,0,0,(16*sx)&0x1ff,((16*sy)&(width-1)),NULL,TRANSPARENCY_PEN,0); /* normal */

				offs++;
			}
		}
		/* Only ever seems to use one linescroll value, ok for now */
		copyscrollbitmap(bitmap,tmpbitmap,1,bg_scrollx,512,bg_scrolly,cliprect,TRANSPARENCY_PEN,0);
	}
}

/* 3 BG layers, with priority */
static void psikyosh_drawbackground( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i, layer[3] = {0, 1, 2}, bg_pri[3];

	/* Priority seems to be in range 0-6 */
	for (i=0; i<=2; i++)
	{
		if ( BG_TYPE(i) == BG_NORMAL_ALT )
			bg_pri[i]  = ((psikyosh_bgram[0x1ff0/4 + (i*0x04)/4] & 0xff000000) >> 24);
		else // Everything else
			bg_pri[i]  = ((psikyosh_bgram[0x17f0/4 + (i*0x04)/4] & 0xff000000) >> 24);
	}

#if 0
#ifdef MAME_DEBUG
	usrintf_showmessage	("Pri %d=%x-%s %d=%x-%s %d=%x-%s",
		layer[0], bg_pri[0], BG_LAYER_ENABLE(layer[0])?"y":"n",
		layer[1], bg_pri[1], BG_LAYER_ENABLE(layer[1])?"y":"n",
		layer[2], bg_pri[2], BG_LAYER_ENABLE(layer[2])?"y":"n");
#endif
#endif

	sortlayers(layer, bg_pri);

	/* 1st-3rd layers */
	for(i=0; i<=2; i++)
	{
		if ( BG_LAYER_ENABLE(layer[i]) )
		{
			switch( BG_TYPE(layer[i]) )
			{
			case BG_NORMAL:
			case BG_NORMAL_ALT:
			case BG_SCROLL_0C: // Using normal for now
			case BG_SCROLL_0D: // Using normal for now
				psikyosh_drawbglayer(layer[i], bitmap, cliprect);
				break;
			case BG_SCROLL_0E:
			case BG_SCROLL_0F:
			case BG_SCROLL_10:
			case BG_SCROLL_11:
			case BG_SCROLL_12:
				psikyosh_drawbglayerscroll(layer[i], bitmap, cliprect);
				break;
			default:
				usrintf_showmessage	("Unknown layer type %02x", BG_TYPE(layer[i]));
				break;
			}
		}
	}

	/* 4th layer */
	if ( BG_LAYER_ENABLE(3) )
		usrintf_showmessage	("Fourth Background Layer Unimplemented"); /* I don't know of anywhere it is used and therefore if it exists */

}

/* --- SPRITES --- */
#define SPRITE_PRI(n) (((psikyosh_vidregs[2] << (4*n)) & 0xf0000000 ) >> 28)

static void psikyosh_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/*- Sprite Format 0x0000 - 0x37ff -**

	0 ---- --yy yyyy yyyy | ---- --xx xxxx xxxx  1  F--- hhhh ZZZZ ZZZZ | fPPP wwww zzzz zzzz
	2 pppp pppp -aaa -nnn | nnnn nnnn nnnn nnnn  3  ---- ---- ---- ---- | ---- ---- ---- ----

	y = ypos
	x = xpos

	h = height
	w = width

	F = flip (y)
	f = flip (x)

	Z = zoom (y)
	z = zoom (x)

	n = tile number

	p = palette

	a = alpha blending, selects which of the 8 alpha values in vid_regs[0-1] to use

	P = priority related.
	Points to a 4-bit entry in vid_regs[2] which provides a priority comparable with the bg layer's priorities.
	However, sprite-sprite priority seems to need to be preserved.
	daraku and soldivid only use the LSB

	* missing priorities.

	**- End Sprite Format -*/

	const struct GfxElement *gfx;
	data32_t *src = buffered_spriteram32; /* Use buffered spriteram */
	data16_t *list = (data16_t *)buffered_spriteram32 + 0x3800/2;
	data16_t listlen=0x800/2, listcntr=0;
	data16_t *zoom_table = (data16_t *)psikyosh_zoomram;
	data8_t  *alpha_table = (data8_t *)psikyosh_vidregs;

	int spr_val=0x00;

#if 0
#ifdef MAME_DEBUG
	{
		if (keyboard_pressed(KEYCODE_Q))	spr_val |= 0x01;	// priority against layer 0 only
		if (keyboard_pressed(KEYCODE_W))	spr_val |= 0x02;	//          ""            1
		if (keyboard_pressed(KEYCODE_E))	spr_val |= 0x04;	//          ""            2
		if (keyboard_pressed(KEYCODE_R))	spr_val |= 0x08;	//          ""            3
		if (keyboard_pressed(KEYCODE_T))	spr_val |= 0x10;	//          ""            4
		if (keyboard_pressed(KEYCODE_Y))	spr_val |= 0x20;	//          ""            5
		if (keyboard_pressed(KEYCODE_U))	spr_val |= 0x40;	//          ""            6
		if (keyboard_pressed(KEYCODE_I))	spr_val |= 0x80;	//          ""            7
		if (keyboard_pressed(KEYCODE_O))	spr_val |= 0xff;	//          ""            all
	}
#endif
#endif

	while( listcntr < listlen )
	{
		data32_t listdat, sprnum, xpos, ypos, high, wide, flpx, flpy, zoomx, zoomy, tnum, colr, dpth;
		data32_t pri, alpha, alphamap, trans;

		listdat = list[BYTE_XOR_BE(listcntr)];
		sprnum = (listdat & 0x03ff) * 4;

		pri   = (src[sprnum+1] & 0x00007000) >> 12; // & 0x00007000/0x00003000 ?
		pri = SPRITE_PRI(pri);

		if ( !(((pri==0) && (spr_val&0x01)) || ((pri==1) && (spr_val&0x02)) || ((pri==2) && (spr_val&0x04)) || ((pri==3) && (spr_val&0x08)) || ((pri==4) && (spr_val&0x10)) || ((pri==5) && (spr_val&0x20)) || ((pri==6) && (spr_val&0x40)) || ((pri==7) && (spr_val&0x80))) )
		{
			ypos = (src[sprnum+0] & 0x03ff0000) >> 16;
			xpos = (src[sprnum+0] & 0x000003ff) >> 00;

			if(ypos & 0x200) ypos -= 0x400;
			if(xpos & 0x200) xpos -= 0x400;

			high  = (src[sprnum+1] & 0x0f000000) >> 24;
			wide  = (src[sprnum+1] & 0x00000f00) >> 8;

			flpy  = (src[sprnum+1] & 0x80000000) >> 31;
			flpx  = (src[sprnum+1] & 0x00008000) >> 15;

			zoomy = (src[sprnum+1] & 0x00ff0000) >> 16;
			zoomx = (src[sprnum+1] & 0x000000ff) >> 00;

			tnum  = (src[sprnum+2] & 0x0007ffff) >> 00;
			dpth  = (src[sprnum+2] & 0x00800000) >> 23;
			colr  = (src[sprnum+2] & 0xff000000) >> 24;

			alpha = (src[sprnum+2] & 0x00700000) >> 20;

			alphamap = (alpha_table[BYTE4_XOR_BE(alpha)] & 0x80)? 1:0;
			alpha = alpha_table[BYTE4_XOR_BE(alpha)] & 0x3f;

			gfx = dpth ? Machine->gfx[1] : Machine->gfx[0];

			if(alphamap) { /* alpha values are per-pen */
				trans = TRANSPARENCY_ALPHARANGE;
			} else if(alpha) {
				alpha = ((0x3f-alpha)*0xff)/0x3f; /* 0x3f-0x00 maps to 0x00-0xff */
				trans = TRANSPARENCY_ALPHA;
				alpha_set_level(alpha);
			} else {
				trans = TRANSPARENCY_PEN;
			}

			/* start drawing */
			{
				int loopnum = 0;
				int cnt, cnt2;
				data32_t xoffset, yoffset, xzoom, yzoom;

				/* zoom_table contains a table that the hardware uses to read pixel slopes (fixed point) */
				/* Need to convert this into scale factor for drawgfxzoom. Would benefit from custom renderer to be pixel-perfect */
				/* For all games so far table is = 2^16/(offs+1) i.e. the 16-bit mantissa of 1/x == linear zoom. */

				if( zoom_table[BYTE_XOR_BE(zoomy)] ) { /* Avoid division-by-zero when table contains 0 (Uninitialised/Bug) */
					/* increase zoom slightly to remove seams between multiple sprites. Wouldn't be necessary if done correctly */
					if (zoomy==0x3f)
						yzoom = 0x10000; /* Theoretically this isn't always true, have to check that other games don't change the table in future :) */
					else
						yzoom = ((0x400 * 0x400 * 0x40) / (UINT32)zoom_table[BYTE_XOR_BE(zoomy)]) + 0x800;
				} else
					yzoom = 0x00;

				if( zoom_table[BYTE_XOR_BE(zoomx)] ) {
					if (zoomx==0x3f)
						xzoom = 0x10000;
					else
						xzoom = ((0x400 * 0x400 * 0x40) / (UINT32)zoom_table[BYTE_XOR_BE(zoomx)]) + 0x800;
				} else
					xzoom = 0x00;


				/* no flip */
				if ((!flpx) && (!flpy))
					for (cnt2 = 0; cnt2<=high; cnt2++)
						for (cnt = 0; cnt<=wide ; cnt++)
						{
							/* offset, not taking into account slightly increased zoom */
							if( zoom_table[BYTE_XOR_BE(zoomy)] ) {
								if (zoomy==0x3f) yoffset = cnt2 * 16; /* Theoretically this isn't always true :) */
								else yoffset = ((cnt2 * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomy)] + (1<<11)) >> 12;
							} else
								yoffset = 0x00;

							if( zoom_table[BYTE_XOR_BE(zoomx)] ) {
								if (zoomx==0x3f) xoffset = cnt * 16;
								else xoffset = ((cnt * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomx)] + (1<<11)) >> 12;
							} else
								xoffset = 0x00;

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,trans,0,xzoom,yzoom);
							loopnum++;
						}

				/* x flip */
				if ((flpx) && (!flpy))
					for (cnt2 = 0; cnt2<=high; cnt2++)
						for (cnt = wide; cnt>=0 ; cnt--)
						{
							/* offset, not taking into account slightly increased zoom */
							if( zoom_table[BYTE_XOR_BE(zoomy)] ) {
								if (zoomy==0x3f) yoffset = cnt2 * 16;
								else yoffset = ((cnt2 * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomy)] + (1<<11)) >> 12;
							} else
								yoffset = 0x00;

							if( zoom_table[BYTE_XOR_BE(zoomx)] ) {
								if (zoomx==0x3f) xoffset = cnt * 16;
								else xoffset = ((cnt * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomx)] + (1<<11)) >> 12;
							} else
								xoffset = 0x00;

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,trans,0,xzoom,yzoom);
							loopnum++;
						}

				/* y flip */
				if ((!flpx) && (flpy))
					for (cnt2 = high; cnt2>=0; cnt2--)
						for (cnt = 0; cnt<=wide ; cnt++)
						{
							/* offset, not taking into account slightly increased zoom */
							if( zoom_table[BYTE_XOR_BE(zoomy)] ) {
								if (zoomy==0x3f) yoffset = cnt2 * 16;
								else yoffset = ((cnt2 * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomy)] + (1<<11)) >> 12;
							} else
								yoffset = 0x00;

							if( zoom_table[BYTE_XOR_BE(zoomx)] ) {
								if (zoomx==0x3f) xoffset = cnt * 16;
								else xoffset = ((cnt * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomx)] + (1<<11)) >> 12;
							} else
								xoffset = 0x00;

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,trans,0,xzoom,yzoom);
							loopnum++;
						}

				/* x & y flip */
				if ((flpx) && (flpy))
					for (cnt2 = high; cnt2>=0; cnt2--)
						for (cnt = wide; cnt>=0 ; cnt--)
						{
							/* offset, not taking into account slightly increased zoom */
							if( zoom_table[BYTE_XOR_BE(zoomy)] ) {
								if (zoomy==0x3f) yoffset = cnt2 * 16;
								else yoffset = ((cnt2 * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomy)] + (1<<11)) >> 12;
							} else
								yoffset = 0x00;

							if( zoom_table[BYTE_XOR_BE(zoomx)] ) {
								if (zoomx==0x3f) xoffset = cnt * 16;
								else xoffset = ((cnt * 0x400 * 0x400 * 0x40)/zoom_table[BYTE_XOR_BE(zoomx)] + (1<<11)) >> 12;
							} else
								xoffset = 0x00;

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,trans,0,xzoom,yzoom);
							loopnum++;
						}
			}
			/* end drawing */
		}
		listcntr++;
		if (listdat & 0x4000) break;
	}
}

VIDEO_START( psikyosh )
{
	tmpbitmap = 0;
	if ((tmpbitmap = auto_bitmap_alloc(32*16,32*16)) == 0)
		return 1;

	Machine->gfx[1]->color_granularity=16; /* 256 colour sprites with palette selectable on 16 colour boundaries */

	{ /* Pens 0xc0-0xff have a gradient of alpha values associated with them */
		int i;
		for (i=0; i<0xc0; i++)
			gfx_alpharange_table[i] = 0xff;
		for (i=0; i<0x40; i++) {
			int alpha = ((0x3f-i)*0xff)/0x3f;
			gfx_alpharange_table[i+0xc0] = alpha;
		}
	}

	return 0;
}

VIDEO_UPDATE( psikyosh )
{
	fillbitmap(bitmap,get_black_pen(),cliprect); /* should probably be a colour from somewhere */

	psikyosh_drawbackground(bitmap, cliprect);
	psikyosh_drawsprites(bitmap, cliprect);
}

VIDEO_EOF( psikyosh )
{
	buffer_spriteram32_w(0,0,0);
}

/* Psikyo PS6406B BG Scroll/Priority */

// 0x13f0/4/8 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = 0x0a)
// 0x1bf0/4/8 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = 0x0b)
// 0x17f0/4/8 | pp??aabb p = priority - a = alpha value/effect - b = tilebank (used when register below = 0x0a)
// 0x1ff0/4/8 | pp??aabb p = priority - a = alpha value/effect - b = tilebank (used when register below = 0x0b)

/* Psikyo PS6406B Vid Regs */

//  0x00 -- alpha values for sprites. sbomberb = 0000 3830
//  0x04 --   "     "     "     "     sbomberb = 2820 1810.
//  0x08 -- priority values for sprites, 4-bits per value
//  0x0c -- ????c0?? -c0 is flip screen
//  0x10 -- 00aa2000? always? -gb2 tested -
//  0x14 -- 83ff000e? always? -gb2 tested-
//  0x18 -- 0a0a0a0a quite often .. seems to be some kind of double buffer for tilemaps ... xx------ being first layer 0a = piece 1 0b = piece 2 ...
//          8e-92 indicates first layer uses row and/or line scroll. values come from associated bank instead of tiles, guessing applied to layer below.
//			8c / 8d are used by daraku for text layers. same as above except bank is still controlled by registers and seems to contain two 16x16 timemaps with alternate columns from each.
//  0x1c -- ????xxxx enable bits  8 is enable. 4 seems to represent 8bpp tiles. 1 is size select for tilemap?

/*usrintf_showmessage	("Regs %08x %08x %08x\n     %08x %08x %08x",
	psikyosh_bgram[0x17f0/4], psikyosh_bgram[0x17f4/4], psikyosh_bgram[0x17f8/4],
	psikyosh_bgram[0x1ff0/4], psikyosh_bgram[0x1ff4/4], psikyosh_bgram[0x1ff8/4]);*/
/*usrintf_showmessage	("Regs %08x %08x %08x\n     %08x %08x %08x",
	psikyosh_bgram[0x13f0/4], psikyosh_bgram[0x13f4/4], psikyosh_bgram[0x13f8/4],
	psikyosh_bgram[0x1bf0/4], psikyosh_bgram[0x1bf4/4], psikyosh_bgram[0x1bf8/4]);*/
/*usrintf_showmessage	("Regs %08x %08x %08x %08x %08x %08x %08x %08x",
	psikyosh_vidregs[0], psikyosh_vidregs[1],
	psikyosh_vidregs[2], psikyosh_vidregs[3],
	psikyosh_vidregs[4], psikyosh_vidregs[5],
	psikyosh_vidregs[6], psikyosh_vidregs[7]);*/


/* Psikyo PS6807 */

/* --- SPRITES --- */
static void psikyo4_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT32 scr )
{
	/*- Sprite Format 0x0000 - 0x2bff -**

	0 hhhh --yy yyyy yyyy | wwww- --xx xxxx xxxx  1  -fpp pppp p--- -nnn | nnnn nnnn nnnn nnnn

	y = ypos
	x = xpos

	h = height
	w = width

	f = flip (x)

	n = tile number

	p = palette

	**- End Sprite Format -*/

	const struct GfxElement *gfx = Machine->gfx[1];
	data32_t *source = spriteram32;
	data16_t *list = (data16_t *)spriteram32 + 0x2c00/2 + 0x04/2; /* 0x2c00/0x2c02 what are these for, pointers? one for each screen */
	data16_t listlen=(0xc00/2 - 0x04/2), listcntr=0;

	while( listcntr < listlen )
	{
		data16_t listdat, sprnum, thisscreen;

		listdat = list[BYTE_XOR_BE(listcntr)];
		sprnum = (listdat & 0x03ff) * 2;

		thisscreen = 0;
		if ((listdat & 0x2000) == scr) thisscreen = 1;


		/* start drawing */
		if (!(listdat & 0x8000) && thisscreen) /* draw only selected screen */
		{
			int loopnum=0, i, j;
			data32_t xpos, ypos, tnum, wide, high, colr, flipx;

			ypos = (source[sprnum+0] & 0x03ff0000) >> 16;
			xpos = (source[sprnum+0] & 0x000003ff) >> 00;

			if(ypos & 0x200) ypos -= 0x400;
			if(xpos & 0x200) xpos -= 0x400;

#if DUAL_SCREEN /* if we are displaying both screens simultaneously */
			if (scr) xpos += 40*8;
#endif

			high = (source[sprnum+0] & 0xf0000000) >> (12+16);
			wide = (source[sprnum+0] & 0x0000f000) >> 12;

			tnum = (source[sprnum+1] & 0x0007ffff) >> 00;

			colr = (source[sprnum+1] & 0x3f800000) >> 23;
			if(scr)	colr += 0x80; /* Use second copy of palette which is dimmed appropriately */

			flipx = (source[sprnum+1] & 0x40000000);

			if(!flipx)
			{
				for (j = 0; j<=high; j++) {
					for (i = 0; i<=wide ; i++) {
						drawgfx(bitmap,gfx,tnum+loopnum,colr,0,0,xpos+16*i,ypos+16*j,cliprect,TRANSPARENCY_PEN,0);
						loopnum++;
					}
				}
			} else {
				for (j = 0; j<=high; j++) {
					for (i = wide; i>=0 ; i--) {
						drawgfx(bitmap,gfx,tnum+loopnum,colr,1,0,xpos+16*i,ypos+16*j,cliprect,TRANSPARENCY_PEN,0);
						loopnum++;
					}
				}
			}

		}
		/* end drawing */
		listcntr++;
		if (listdat & 0x4000) break;
	}
}

VIDEO_UPDATE( psikyo4 )
{
#if DUAL_SCREEN
	{
		struct rectangle clip;

		clip.min_x = 0;
		clip.max_x = 40*8-1;
		clip.min_y = Machine->visible_area.min_y;
		clip.max_y = Machine->visible_area.max_y;

		fillbitmap(bitmap, Machine->pens[0x1000], &clip);
		psikyo4_drawsprites(bitmap, &clip, 0x0000);

		clip.min_x = 40*8;
		clip.max_x = 80*8-1;
		clip.min_y = Machine->visible_area.min_y;
		clip.max_y = Machine->visible_area.max_y;

		fillbitmap(bitmap, Machine->pens[0x1001], &clip);
		psikyo4_drawsprites(bitmap, &clip, 0x2000);
	}
#else
	{
		if (readinputport(9) & 1) screen = 0x0000; /* change screens from false dip, is this ok? */
		else if (readinputport(9) & 2) screen = 0x2000;

		fillbitmap(bitmap, Machine->pens[(screen==0x0000)?0x1000:0x1001], cliprect);
		psikyo4_drawsprites(bitmap, cliprect, screen);
	}
#endif


#if 0
#ifdef MAME_DEBUG
	{
		usrintf_showmessage	("Regs %08x %08x %08x",
			psikyosh_vidregs[0], psikyosh_vidregs[1],
			psikyosh_vidregs[2]);
//		usrintf_showmessage ("Brightness %08x %08x",
//			screen1_brt[0], screen2_brt[0]);
	}
#endif
#endif
}

VIDEO_START( psikyo4 )
{
	Machine->gfx[1]->color_granularity=16; /* 256 colour sprites with palette selectable on 16 colour boundaries */
	screen = 0;
	return 0;
}

/* Psikyo PS6807 Vid Regs */

//  0x3003fe4 -- ??xx???? vblank? 86??0000 always?
//  0x3003fe8 -- c0c0???? flipscreen for screen 1 and 2 resp.
//                        ????8080 Screen size select?
//  0x3003fec -- a0000000 always? is in two working games.
//  0x3003ff0 -- ??????xx brightness for screen 1
//  0x3003ff4 -- xxxxxx?? screen 1 clear colour
//  0x3003ff8 -- ??????xx brightness for screen 2
//  0x3003ffc -- xxxxxx?? screen 2 clear colour

/*usrintf_showmessage	("Regs %08x %08x %08x %08x %08x %08x %08x %08x",
	psikyosh_vidregs[0], psikyosh_vidregs[1],
	psikyosh_vidregs[2], psikyosh_vidregs[3],
	psikyosh_vidregs[4], psikyosh_vidregs[5],
	psikyosh_vidregs[6], psikyosh_vidregs[7]);*/
