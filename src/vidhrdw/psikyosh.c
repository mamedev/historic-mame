#include "driver.h"

extern data32_t *psikyosh_spriteram, *psikyosh_unknownram1, *psikyosh_unknownram2, *psikyosh_unknownram3;
extern int psikyosh_drawbg;


/* this is wrong but for testing */


/* vid ram

I *think* capabilities include brightness control (theres an area which increases / decreases in value on scene changes
rowscroll / column scroll (s1945ii test uses a whole block of ram for scrolling, not seen that elsewhere tho)
various sizes of tilemaps

at the moment I think the data in tileram isn't always correct, for example in s1945ii there are 32 unique tile values
(from what I can tell) to draw what is probably 2 full screen tilemaps, I suspect SH-2 Core bugs *core bug fixed*

the stuff might be converted to use the tilemaps once all the features is worked out ... strikers 1945 ii seems to do
weird things still .. for example it doesn't seem to enable one of the layers

priority also isn't understood .. and since the depth select seems based on priority some things end up quite broken ;)

there is probably some alpha blending ..

size changes?

flip screen?

its not that efficient i guess ;)

why so many copies of the 4bpp tilemaps in ram? (see scrolling ingame parts of gb2 & s1945ii) just a buffer?

there is probably a gradient background / bg colour selector

daraku seems to use tilemaps only for text layer (hi-scores, insert coin, warning message, test mode, psikyo (c)) how this is
used is uncertain

sol divide doesn't seem to make much use of tilemaps at all

y scroll is probably wrong .. not really checked but it doesnt' seem right on anything that uses it ;)

the only place i've seen line/column scroll used is s1945ii test mode ..


*/

static void test_psikyosh_drawbackground( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	struct GfxElement *gfx;

	int offs,sx,sy;

	/* 1st layer */

	sx = sy = 0;
	for (offs = 0;offs < 0x1000/4;offs++)
	{
		int tileno, colour, scrollx, scrolly;


		if (( psikyosh_unknownram3[6] & 0xff000000) == 0x0b000000)
		{
			scrollx = (psikyosh_unknownram1[0x1bf0/4] & 0x000001ff);
			scrolly = ((psikyosh_unknownram1[0x1bf0/4] & 0x01ff0000)) >>16;
			tileno =  psikyosh_unknownram1[((0x2800/4)+offs)] & 0x0007ffff;
			colour =  (psikyosh_unknownram1[((0x2800/4)+offs)] & 0xff000000) >> 24;
		}
		else
		{
			scrollx = (psikyosh_unknownram1[0x13f0/4] & 0x000001ff);
			scrolly = ((psikyosh_unknownram1[0x13f0/4] & 0x01ff0000)) >>16;
			tileno =  psikyosh_unknownram1[((0x2000/4)+offs)] & 0x0007ffff;
			colour =  (psikyosh_unknownram1[((0x2000/4)+offs)] & 0xff000000) >> 24;
		}

		if ((psikyosh_unknownram3[7] & 0x0000f000) == 0x00008000) gfx = Machine->gfx[0];
			else  gfx = Machine->gfx[1];

		if (psikyosh_unknownram3[7] & 0x0000f000) /* might not be enable .. strikers doesn't seem to use the same way */
		{
			drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* normal */
			drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* wrap x */
			drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrapy */
			drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrap xy */
		}

		sx++; if (sx > 31) {sy++;sx=0;}
	}

	/* 2nd layer */

	sx = sy = 0;

	for (offs = 0;offs < 0x1000/4;offs++)
	{
		int tileno, colour, scrollx, scrolly;

		if (( psikyosh_unknownram3[6] & 0x00ff0000) == 0x000b0000) /* some kind of double buffer ? */
		{
			tileno =  psikyosh_unknownram1[(0x3800/4)+offs] & 0x0007ffff;
			colour =  (psikyosh_unknownram1[(0x3800/4)+offs] & 0xff000000) >> 24;
			scrollx = (psikyosh_unknownram1[0x1bf4/4] & 0x000001ff);
			scrolly = ((psikyosh_unknownram1[0x1bf4/4] & 0x01ff0000)) >>16;
		}
		else
		{
			tileno =  psikyosh_unknownram1[(0x3000/4)+offs] & 0x0007ffff;
			colour =  (psikyosh_unknownram1[(0x3000/4)+offs] & 0xff000000) >> 24;
			scrollx = (psikyosh_unknownram1[0x13f4/4] & 0x000001ff);
			scrolly = ((psikyosh_unknownram1[0x13f4/4] & 0x01ff0000)) >>16;
		}

		if ((psikyosh_unknownram3[7] & 0x00000f00) == 0x00000800) gfx = Machine->gfx[0];
			else  gfx = Machine->gfx[1];

		if (psikyosh_unknownram3[7] & 0x00000f00) /* might not be enable .. strikers doesn't seem to use the same way */
		{
			drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* normal */
			drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff),cliprect,TRANSPARENCY_PEN,0); /* wrap x */
			drawgfx(bitmap,gfx,tileno,colour,0,0,(16*sx+scrollx)&0x1ff,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrap y */
			drawgfx(bitmap,gfx,tileno,colour,0,0,((16*sx+scrollx)&0x1ff)-0x200,((16*sy+scrolly)&0x1ff)-0x200,cliprect,TRANSPARENCY_PEN,0); /* wrap xy */
		}

		sx++; if (sx > 31) {sy++;sx=0;}

	}

}



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

static void psikyosh_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/*- Spite Format 0x0000 - 0x37ff? -**

	 0 ---- --yy yyyy yyyy | ---- --xx xxxx xxxx  1  F--- hhhh ---- ---- | f--- wwww ---- ----
	 2 pppp pppp ---- --nn | nnnn nnnn nnnn nnnn  3  ---- ---- ---- ---- | ---- ---- ---- ----

	y = ypos
	x = xpos

	h = height
	w = width

	F = flip (y)
	f = flip (x)

	n = tile number

	p = palette

	* missing zoom, and maybe priorities, colour isn't 100% certain (some sprites seem odd)

	**- End Sprite Format -*/

	const struct GfxElement *gfx;
	data32_t *src = psikyosh_spriteram;
	data16_t *list = (data16_t *)psikyosh_spriteram + 0x3800/2;
	data16_t listlen=0x800/2, listcntr=0;

	while( listcntr<listlen )
	{
		data32_t listdat, sprnum, xpos, ypos, high, wide, flpx, flpy, tnum, colr,dpth;

		listdat = list[BYTE_XOR_BE(listcntr)];
		sprnum = (listdat & 0x1fff) *4;

		ypos = (src[sprnum+0] & 0x03ff0000) >> 16;
		xpos = (src[sprnum+0] & 0x000003ff) >> 00;

		high = (src[sprnum+1] & 0x0f000000) >> 24;
		wide = (src[sprnum+1] & 0x00000f00) >> 8;

		flpy = (src[sprnum+1] & 0x80000000) >> 31;
		flpx = (src[sprnum+1] & 0x00008000) >> 15;

		tnum = (src[sprnum+2] & 0x0007ffff) >> 00;


		dpth = (src[sprnum+2] & 0x00800000) >> 23;

		colr = (src[sprnum+2] & 0xff000000) >> 24;

		gfx = dpth?Machine->gfx[1]:Machine->gfx[0];

		/* start drawing */
		{

			int loopnum = 0;
			int cnt, cnt2;

			/* no flip */
			if ((!flpx) && (!flpy))
			{
			for (cnt2 = 0; cnt2<=high; cnt2++)
			{
				for (cnt = 0; cnt<=wide ; cnt++)
				{
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					loopnum++;
				}
			}
			}

			/* xflip */
			if ((flpx) && (!flpy))
			{
			for (cnt2 = 0; cnt2<=high; cnt2++)
			{
				for (cnt = wide; cnt>=0 ; cnt--)
				{
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					loopnum++;
				}
			}
			}

			/* y flip */
			if ((!flpx) && (flpy))
			{
			for (cnt2 = high; cnt2>=0; cnt2--)
			{
				for (cnt = 0; cnt<=wide ; cnt++)
				{
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					loopnum++;
				}
			}
			}

			/* x & y flip */
			if ((flpx) && (flpy))
			{
			for (cnt2 = high; cnt2>=0; cnt2--)
			{
				for (cnt = wide; cnt>=0 ; cnt--)
				{
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2,cliprect,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,gfx,tnum+loopnum,colr,flpx,flpy,xpos+cnt*16-0x400,ypos+16*cnt2-0x400,cliprect,TRANSPARENCY_PEN,0);
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

VIDEO_START(psikyosh)
{
	Machine->gfx[1]->color_granularity=16; /* 256 colour sprites with palette selectable on 16 colour boundaries */
	return 0;
}

VIDEO_UPDATE(psikyosh)
{
	fillbitmap(bitmap,get_black_pen(),&Machine->visible_area); /* should probably be a gradient / colour from somewhere */

	if (psikyosh_drawbg) test_psikyosh_drawbackground(bitmap,cliprect);

	psikyosh_drawsprites(bitmap, cliprect);

/*
	usrintf_showmessage	("Regs %08x %08x %08x %08x %08x %08x %08x %08x",
	psikyosh_unknownram1[0x13f0/4],
	psikyosh_unknownram1[0x13f4/4],
	psikyosh_unknownram1[0x17f0/4],
	psikyosh_unknownram1[0x17f4/4],
	psikyosh_unknownram1[0x1bf0/4],
	psikyosh_unknownram1[0x1bf4/4],
	psikyosh_unknownram1[0x1ff0/4],
	psikyosh_unknownram1[0x1ff4/4]);
*/

 // 0x13f0 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = a)
 // 0x13f4 | ?vvv?xxx - v = vertical scroll - x = x scroll *OTHER LAYER* (used when register below = a)
 // 0x17f0 | priority related?
 // 0x17f4 | priority related?
 // 0x1bf0 | ?vvv?xxx - v = vertical scroll - x = x scroll (used when register below = b)
 // 0x1bf4 | ?vvv?xxx - v = vertical scroll - x = x scroll *OTHER LAYER* (used when register below = a)
 // 0x1ff0 | priority related?
 // 0x1ff4 | priority related?

/*
usrintf_showmessage	("Regs %08x %08x %08x %08x %08x %08x %08x %08x",
psikyosh_unknownram3[0], // e0 - e3
psikyosh_unknownram3[1], // e4 - e7
psikyosh_unknownram3[2], // e8 - eb
psikyosh_unknownram3[3], // ec - ef
psikyosh_unknownram3[4], // ec - ef
psikyosh_unknownram3[5], // ec - ef
psikyosh_unknownram3[6], // f8 - fb
psikyosh_unknownram3[7]);// fc - ff;
*/

//  0x00
//  0x04
//  0x08
//  0x0c
//  0x10 -- 00aa2000? always? -gb2 tested -
//  0x14 -- 83ff000e? always? -gb2 tested-
//  0x18 -- 0a0a0a0a quite often .. seems to be some kind of double buffer for tilemaps ... xx------ being first layer 0a = piece 1 0b = piece 2 ...
//  0x1c -- ????xx?? enable bits?  8 seems to represent 4bpp tiles .. c seems to represent 8bpp tiles .. except in strikers . seems in priority order. ://


}
