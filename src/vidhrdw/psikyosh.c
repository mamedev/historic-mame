#include "driver.h"
#include "vidhrdw/generic.h"

extern data32_t *psikyosh_bgram, *psikyosh_unknownram2, *psikyosh_vidregs;
extern int psikyosh_drawbg;

static data32_t zoom_table[0xff];

/* History (post MAME0.59)*/
/*
06-04-2002 - Added Sprite zooming - Paul Priest (tourniquet@mameworld.net)
07-04-2002 - Corrected tileno/palette, fixes sbomberb y scroll - pjp
09-04-2002 - Added Sprite buffering - pjp
25-04-2002 - Figured out how the background banks work + fixed priority to boot -pjp
26-04-2002 - Sorted bg layers, Enabled Alpha Blending, Added different bg layer sizes -pjp
27-04-2002 - Minor alpha fix ;), kludge for daruka text -pjp
*/

/* vid ram

TODO:
I *think* capabilities include brightness control (theres an area which increases / decreases in value on scene changes, -dh
I'm not so sure. Seems complete as is.-pjp

rowscroll / column scroll (s1945ii test and level 7 uses a whole block of ram for scrolling, 225 values for both h and v scroll) -dh
the only place i've seen line/column scroll used is s1945ii test mode and Level 7. -pjp

there is another blending effect (addition?), Guru is getting a snap of it being used in Strikers on the clouds.

flip screen?, located but not implemented.

the stuff might be converted to use the tilemaps once all the features is worked out ...

there is probably a gradient background / bg colour selector, psikyosh_unknownram2


FIXED:

there is some alpha blending *done* -pjp

daraku seems to use tilemaps only for text layer (hi-scores, insert coin, warning message, test mode, psikyo (c)) how this is
used is uncertain, sprite on bg priorites *done*

sol divide doesn't seem to make much use of tilemaps at all, it uses them to fade between scenes in the intro *done* -pjp

y scroll is probably wrong .. not really checked but it doesnt' seem right on anything that uses it ;) *fixed* -pjp

various sizes of tilemaps *done*, well two sizes anyway ;) -pjp

priority also isn't understood .. and since the depth select seems based on priority some things end up quite broken ;) *fixed* -pjp

strikers 1945 ii seems to do weird things still .. for example it doesn't seem to enable one of the layers *fixed* -pjp

at the moment I think the data in tileram isn't always correct, for example in s1945ii there are 32 unique tile values
(from what I can tell) to draw what is probably 2 full screen tilemaps, I suspect SH-2 Core bugs *core bug fixed*

why so many copies of the 4bpp tilemaps in ram? (see scrolling ingame parts of gb2 & s1945ii) just a buffer? *fixed* Nope, different banks -pjp

sprites look like they should probably be buffered by one frame, this would make the palette correct as well *done* -pjp
*/

#define FLIPSCREEN (((psikyosh_vidregs[3] & 0x0000c000) == 0x0000c000) ? 1:0)

/* --- SCREEN CLEAR --- */

/* ignore for now ;) */
static void test_psikyosh_drawgradient( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	if(1)
	{
		struct rectangle clip;
		int i, colour;
		clip.min_x = cliprect->min_x;
		clip.max_x = cliprect->max_x;

		for(i = cliprect->min_y; i < cliprect->max_y; i++)
		{
			clip.min_y = i;
			clip.max_y = i+1;

			colour = (i * 0xff)/(cliprect->max_y);

			fillbitmap(bitmap, colour, &clip);
		}
	}
}

/* --- BACKGROUNDS --- */

#define BG_LARGE(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00001000 ) ? 1:0)
#define BG_DEPTH_8BPP(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00004000 ) ? 1:0)
#define BG_LAYER_ENABLE(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00008000 ) ? 1:0)

#define ALT_BUFFER(n) ((((psikyosh_vidregs[6] << (8*n)) & 0xff000000 ) == 0x0b000000) ? 1:0)

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

/* S1945ii Level 7 and Test Mode */
#define LINE_COL_SCROLL(n) ((((psikyosh_vidregs[6] << (8*n)) & 0xff000000 ) == 0x8e000000) ? 1:0)

static void psikyosh_drawbglayer( int layer, struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	struct GfxElement *gfx;
	int offs, sx=0, sy=0;
	int scrollx, scrolly, bank, alpha, size;

	gfx = BG_DEPTH_8BPP(layer)?Machine->gfx[1]:Machine->gfx[0];

	if ( ALT_BUFFER(layer) )
	{
		bank    = (psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x000000ff) >> 0;
		/* 0x00-0x3f = as+(a-1)d, 0x80 = s+d? */
		alpha   = (psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x00003f00) >> 8;
		scrollx = (psikyosh_bgram[0x1bf0/4 + (layer*0x04)/4] & 0x000001ff) >> 0;
		scrolly = (psikyosh_bgram[0x1bf0/4 + (layer*0x04)/4] & 0x01ff0000) >> 16;

//		if(psikyosh_bgram[0x1ff0/4 + (layer*0x04)/4] & 0x00008000) alpha = 0x20; /* Additive Blending? */
	}
	else
	{
		bank    = (psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x000000ff) >> 0;
		alpha   = (psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x00003f00) >> 8;
		scrollx = (psikyosh_bgram[0x13f0/4 + (layer*0x04)/4] & 0x000001ff) >> 0;
		scrolly = (psikyosh_bgram[0x13f0/4 + (layer*0x04)/4] & 0x01ff0000) >> 16;

//		if(psikyosh_bgram[0x17f0/4 + (layer*0x04)/4] & 0x00008000) alpha = 0x20; /* Additive Blending? */
	}

	alpha = ((0x3f-alpha)*0xff)/0x3f; /* 0x3f-0x00 maps to 0x00-0xff */
	size = BG_LARGE(layer) ? (0x1000/4) : (0x800/4);

	if((bank>=0x0c) && (bank<=0x1f)) /* shouldn't happen, 18 banks of 0x800 bytes */
	{
		for (offs=0; offs<size; offs++) /* does the size depend on something? */
		{
			int tileno, colour;

			tileno = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0x0007ffff); /* seems to take into account spriteram, hence -0x4000 */
			colour = (psikyosh_bgram[(bank*0x800)/4 + offs - 0x4000/4] & 0xff000000) >> 24;

			if(alpha != 0xff) /* alpha */
			{
				alpha_set_level(alpha);

				if( BG_LARGE(layer) )
				{
					drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_ALPHA,0); /* normal */
					if(scrollx)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_ALPHA,0); /* wrap x */
					if(scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_ALPHA,0); /* wrap y */
					if(scrollx && scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_ALPHA,0); /* wrap xy */
				}
				else
				{
					drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x0ff),cliprect,TRANSPARENCY_ALPHA,0); /* normal */
					if(scrollx)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x0ff),cliprect,TRANSPARENCY_ALPHA,0); /* wrap x */
					if(scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x0ff)-0x100,cliprect,TRANSPARENCY_ALPHA,0); /* wrap y */
					if(scrollx && scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x0ff)-0x100,cliprect,TRANSPARENCY_ALPHA,0); /* wrap xy */
				}
			}
			else
			{
				if( BG_LARGE(layer) )
				{
					drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* normal */
					if(scrollx)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* wrap x */
					if(scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrap y */
					if(scrollx && scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrap xy */
				}
				else
				{
					drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x0ff),cliprect,TRANSPARENCY_PEN,0); /* normal */
					if(scrollx)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x0ff),cliprect,TRANSPARENCY_PEN,0); /* wrap x */
					if(scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x0ff)-0x100,cliprect,TRANSPARENCY_PEN,0); /* wrap y */
					if(scrollx && scrolly)
						drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x0ff)-0x100,cliprect,TRANSPARENCY_PEN,0); /* wrap xy */
				}
			}
			sx++; if (sx > 31) {sy++; sx=0;}
}}}

static void psikyosh_drawbackground( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i, layer[3] = {0, 1, 2}, bg_pri[3];

	/* Priority seems to be in range 0-6 */
	for (i=0; i<=2; i++) {
		if ( ALT_BUFFER(i) )
			bg_pri[i]  = ((psikyosh_bgram[0x1ff0/4 + (i*0x04)/4] & 0xff000000) >> 24);
		else
			bg_pri[i]  = ((psikyosh_bgram[0x17f0/4 + (i*0x04)/4] & 0xff000000) >> 24);
	}

	sortlayers(layer, bg_pri);

	/* 1st layer */
	if ( BG_LAYER_ENABLE(layer[0]) )
		psikyosh_drawbglayer(layer[0], bitmap, cliprect);

	/* 2nd layer */
	if ( BG_LAYER_ENABLE(layer[1]) )
		psikyosh_drawbglayer(layer[1], bitmap, cliprect);

	/* 3rd layer */
	if ( BG_LAYER_ENABLE(layer[2]) )
		psikyosh_drawbglayer(layer[2], bitmap, cliprect);

	/* 4th layer */
	if ( BG_LAYER_ENABLE(3) )
		usrintf_showmessage	("Fourth Background Layer Unimplemented"); /* I don't know of anywhere it is used and therefore if it exists */

/*
#ifdef MAME_DEBUG
	usrintf_showmessage	("Pri %d=%x-%s %d=%x-%s %d=%x-%s",
		layer[0], bg_pri[0], BG_LAYER_ENABLE(layer[0])?"y":"n",
		layer[1], bg_pri[1], BG_LAYER_ENABLE(layer[1])?"y":"n",
		layer[2], bg_pri[2], BG_LAYER_ENABLE(layer[2])?"y":"n");
#endif
*/
}

/* --- SPRITES --- */

static void psikyosh_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/*- Sprite Format 0x0000 - 0x37ff? -**

	0 ---- --yy yyyy yyyy | ---- --xx xxxx xxxx  1  F--- hhhh ZZZZ ZZZZ | fPPP wwww zzzz zzzz
	2 pppp pppp ---- -nnn | nnnn nnnn nnnn nnnn  3  ---- ---- ---- ---- | ---- ---- ---- ----

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

	P = priority related.
	Still not sure about this, if 0 then nearly always has to be drawn between bg's with priority 3 and 4.
	sbomberb seems to be the exception on the inter-world screen.3
	However, sprite-sprite priority seems to need to be preserved.
	daraku and soldivid only use the LSB

	* missing alpha blending? and priorities.

	**- End Sprite Format -*/

	const struct GfxElement *gfx;
	data32_t *src = buffered_spriteram32; /* Use buffered spriteram */
	data16_t *list = (data16_t *)buffered_spriteram32 + 0x3800/2;
	data16_t listlen=0x800/2, listcntr=0;

	while( listcntr < listlen )
	{
		data32_t listdat, sprnum, xpos, ypos, high, wide, flpx, flpy, zoomx, zoomy, tnum, colr, dpth;

		listdat = list[BYTE_XOR_BE(listcntr)];
		sprnum = (listdat & 0x03ff) * 4;

		{
//			logerror("Sprites, what's left: %08x %08x %08x %08x\n", src[sprnum+0] & 0xfc00fc00, src[sprnum+1] & 0x70007000, src[sprnum+2] & 0x00f80000, src[sprnum+3]);

			ypos = (src[sprnum+0] & 0x03ff0000) >> 16;
			xpos = (src[sprnum+0] & 0x000003ff) >> 00;

			if(ypos & 0x200) ypos -= 0x400;
			if(xpos & 0x200) xpos -= 0x400;

			high = (src[sprnum+1] & 0x0f000000) >> 24;
			wide = (src[sprnum+1] & 0x00000f00) >> 8;

			flpy = (src[sprnum+1] & 0x80000000) >> 31;
			flpx = (src[sprnum+1] & 0x00008000) >> 15;

			zoomy = (src[sprnum+1] & 0x00ff0000) >> 16;
			zoomx = (src[sprnum+1] & 0x000000ff) >> 00;

			tnum = (src[sprnum+2] & 0x0007ffff) >> 00;

			dpth = (src[sprnum+2] & 0x00800000) >> 23;

			colr = (src[sprnum+2] & 0xff000000) >> 24;

			gfx = dpth?Machine->gfx[1]:Machine->gfx[0];

			/* start drawing */
			{
				int loopnum = 0;
				int cnt, cnt2;
				data32_t xoffset, yoffset, xzoom, yzoom;

				/* increase zoom slightly to remove seams in large blocks of sprites */
				yzoom = ((zoomy==0x3f)?0x10000:zoom_table[zoomx]+0x400);
				xzoom = ((zoomx==0x3f)?0x10000:zoom_table[zoomx]+0x400);

				/* no flip */
				if ((!flpx) && (!flpy))
					for (cnt2 = 0; cnt2<=high; cnt2++)
						for (cnt = 0; cnt<=wide ; cnt++)
						{
							/* offset, not taking into account slightly increased zoom */
							yoffset = ((cnt2*zoom_table[zoomy]) >> 12);
							xoffset = ((cnt*zoom_table[zoomx]) >> 12);

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,TRANSPARENCY_PEN,0,xzoom,yzoom);
							loopnum++;
						}

				/* x flip */
				if ((flpx) && (!flpy))
					for (cnt2 = 0; cnt2<=high; cnt2++)
						for (cnt = wide; cnt>=0 ; cnt--)
						{
							/* offset, not taking into account slightly increased zoom */
							yoffset = ((cnt2*zoom_table[zoomy]) >> 12);
							xoffset = ((cnt*zoom_table[zoomx]) >> 12);

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,TRANSPARENCY_PEN,0,xzoom,yzoom);
							loopnum++;
						}

				/* y flip */
				if ((!flpx) && (flpy))
					for (cnt2 = high; cnt2>=0; cnt2--)
						for (cnt = 0; cnt<=wide ; cnt++)
						{
							/* offset, not taking into account slightly increased zoom */
							yoffset = ((cnt2*zoom_table[zoomy]) >> 12);
							xoffset = ((cnt*zoom_table[zoomx]) >> 12);

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,TRANSPARENCY_PEN,0,xzoom,yzoom);
							loopnum++;
						}

				/* x & y flip */
				if ((flpx) && (flpy))
					for (cnt2 = high; cnt2>=0; cnt2--)
						for (cnt = wide; cnt>=0 ; cnt--)
						{
							/* offset, not taking into account slightly increased zoom */
							yoffset = ((cnt2*zoom_table[zoomy]) >> 12);
							xoffset = ((cnt*zoom_table[zoomx]) >> 12);

							drawgfxzoom(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+xoffset,ypos+yoffset,cliprect,TRANSPARENCY_PEN,0,xzoom,yzoom);
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
	int i;

	Machine->gfx[1]->color_granularity=16; /* 256 colour sprites with palette selectable on 16 colour boundaries */

	for (i=0x0;i<0xff;i++) /* Linear zoom, 0x3f is a scale factor of 1, 0x00 is the smallest level of zoom, not non-existent */
		zoom_table[i] = ((i+1) << 24)/0x4000;

	return 0;
}

VIDEO_UPDATE( psikyosh )
{
	fillbitmap(bitmap,get_black_pen(),cliprect); /* should probably be a gradient / colour from somewhere */

	if(1)
	{
		/* Minor kludge for scroll, makes the text layer readable until layer type is supported */
		extern struct GameDriver driver_daraku;
		if (Machine->gamedrv == &driver_daraku) psikyosh_bgram[0x13f4/4] |= 0x08;
	}

	if(psikyosh_drawbg) psikyosh_drawbackground(bitmap, cliprect);
	psikyosh_drawsprites(bitmap, cliprect);
}

VIDEO_EOF( psikyosh )
{
	buffer_spriteram32_w(0,0,0);
}

/*
#if 0
#ifdef MAME_DEBUG
	int spr_val=0, bg_val=0;

	{
		if (keyboard_pressed(KEYCODE_Q))	spr_val |= 0x01;	// priority 0 only
		if (keyboard_pressed(KEYCODE_W))	spr_val |= 0x02;	// ""       1
		if (keyboard_pressed(KEYCODE_E))	spr_val |= 0x04;	// ""       2
		if (keyboard_pressed(KEYCODE_R))	spr_val |= 0x08;	// ""       3
		if (keyboard_pressed(KEYCODE_T))	spr_val |= 0x10;	// ""       4
		if (keyboard_pressed(KEYCODE_Y))	spr_val |= 0x20;	// ""       5
		if (keyboard_pressed(KEYCODE_U))	spr_val |= 0x40;	// ""       6
//		if (keyboard_pressed(KEYCODE_U))	spr_val |= 0xff;	// ""       all

		if (keyboard_pressed(KEYCODE_Z))	bg_val |= 0x01;		// ""       0
		if (keyboard_pressed(KEYCODE_X))	bg_val |= 0x02;		// ""       1
		if (keyboard_pressed(KEYCODE_C))	bg_val |= 0x04;		// ""       2
	}
#endif
#endif
*/

/*
GunBird 2

warning (sprites only) : 00000000 00000000 13672040 0f102038 00aa2000 83ff000e 0a0a0a0a 08000000
test mode (sprites?  ) : 001e1000 31000000 13672006 0f100038 00aa2000 83ff000e 0a0a0a0a 08000000
test mode -flip)       : 001e1000 31000000 13672006 0f10c038 00aa2000 83ff000e 0a0a0a0a 08000000
psikyologo (4bpp)      : 00000000 00000000 06672007 0f102038 00aa2000 83ff000e 0b0a0a0a 08008000
attract 1 (8bpp, 1 l)  : 00000000 00000000 06672007 0f102038 00aa2000 83ff000e 0a0a0a0a 0800c000
attract 2 (8bpp, othr) : 00803f00 00000000 06672007 0f102038 00aa2000 83ff000e 0a0b0a0a 08000c00
boss (1 lay 4bpp)      : 00801b00 00000000 06672006 0f102038 00aa2000 83ff000e 0b0a0a0a 08008000
attract (1 ly 8bpp)    : 00200000 00000000 13672007 0f102038 00aa2000 83ff000e 0b0a0a0a 0800c000
level (2ly 1-8 2-4)    : 00800000 00000000 06672006 0f102038 00aa2000 83ff000e 0b0b0a0a 08008c00
*/

/*
GunBird 2

game : 1layer 4bpp       : 00000020 00800000 0200000c 0100000e 00000017 00600000 0200000d 0100000f
attr:  1layer 8bpp       : 00000000 00800000 0100000c 0100000e 00000000 00600000 0100000d 0100000f
game : 2layers 8bpp4bpp  : 000001dc 000001e0 0200001a 0400800c 000001d8 000001d8 0200001b 0400800d
attr : 1 layer 8bpp      : 00000000 00000070 0100000c 0400800c 00000000 00000061 0100000d 0400800d
game : 1 layer 8bpp      : 000001e0 00000070 0200000c 0400800c 000001d9 00000061 0200000d 0400800d
attr : 1 layer 8bpp      : 00000000 00000070 0100000c 0400800c 00000000 00000061 0100000d 0400800d
psiko : 1 layer 4bpp     : 00000000 00000070 0400000c 0400800c 00000000 00000061 0400000d 0400800d
attr : 1 layer 8bpp      : 004001c8 00000070 0200000c 0400800c 004001c7 00000061 0200000d 0400800d
attr : 1 layer(2) 8bpp   : 00c00000 00c00000 0100000c 0200000e 00ce0000 00c00000 0100000d 0100000f
*/

/*	usrintf_showmessage	("Regs %08x %08x %08x\n     %08x %08x %08x",
	psikyosh_bgram[0x17f0/4],
	psikyosh_bgram[0x17f4/4],
	psikyosh_bgram[0x17f8/4],
	psikyosh_bgram[0x1ff0/4],
	psikyosh_bgram[0x1ff4/4],
	psikyosh_bgram[0x1ff8/4]);*/

 // 0x13f0/4/8 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = 0x0a)
 // 0x1bf0/4/8 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = 0x0b)

/*	usrintf_showmessage	("Regs %08x %08x %08x\n     %08x %08x %08x",
	psikyosh_bgram[0x13f0/4],
	psikyosh_bgram[0x13f4/4],
	psikyosh_bgram[0x13f8/4],
	psikyosh_bgram[0x1bf0/4],
	psikyosh_bgram[0x1bf4/4],
	psikyosh_bgram[0x1bf8/4]);*/

 // 0x17f0/4/8 | pp??aabb p = priority - a = alpha value/effect - b = tilebank (used when register below = 0x0a)
 // 0x1ff0/4/8 | pp??aabb p = priority - a = alpha value/effect - b = tilebank (used when register below = 0x0b)

/*
usrintf_showmessage	("Regs %08x %08x %08x %08x %08x %08x %08x %08x",
psikyosh_vidregs[0], // e0 - e3
psikyosh_vidregs[1], // e4 - e7
psikyosh_vidregs[2], // e8 - eb
psikyosh_vidregs[3], // ec - ef
psikyosh_vidregs[4], // ec - ef
psikyosh_vidregs[5], // ec - ef
psikyosh_vidregs[6], // f8 - fb
psikyosh_vidregs[7]);// fc - ff;
*/

//  0x00
//  0x04
//  0x08
//  0x0c -- ????c0?? -c0 is flip screen
//  0x10 -- 00aa2000? always? -gb2 tested -
//  0x14 -- 83ff000e? always? -gb2 tested-
//  0x18 -- 0a0a0a0a quite often .. seems to be some kind of double buffer for tilemaps ... xx------ being first layer 0a = piece 1 0b = piece 2 ...
//          8e indicates first layer uses row and/or line scroll. values come from associated bank instead of tiles, guessing applied to layer below.
//			8c / 8d are used by daraku for text layers (Where 8d is automatically offset by 0x08 on the x scroll so that alternate characters can be drawn to each layer)
//  0x1c -- ????xxxx enable bits  8 is enable. 4 seems to represent 8bpp tiles. 1 is size select for tilemap
