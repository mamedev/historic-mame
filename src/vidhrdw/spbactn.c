/* vidhrdw/spbactn.c - see drivers/spbactn.c for more info */
/* rather similar to galspnbl.c */

#include "driver.h"

extern data16_t *spbactn_bgvideoram, *spbactn_fgvideoram, *spbactn_spvideoram;

/* from gals pinball (which was in turn from ninja gaiden) */
static void draw_sprites(struct mame_bitmap *bitmap,int priority)
{
	int offs;
	const UINT8 layout[8][8] =
	{
		{0,1,4,5,16,17,20,21},
		{2,3,6,7,18,19,22,23},
		{8,9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	for (offs = (0x1000-16)/2;offs >= 0;offs -= 8)
	{
		int sx,sy,code,color,size,attr,flipx,flipy;
		int col,row;

		attr = spbactn_spvideoram[offs];
		if ((attr & 0x0004) && ((attr & 0x0040) == 0 || (cpu_getcurrentframe() & 1))
//				&& ((attr & 0x0030) >> 4) == priority)
				&& ((attr & 0x0020) >> 5) == priority)
		{
			code = spbactn_spvideoram[offs+1];
			color = spbactn_spvideoram[offs+2];
			size = 1 << (color & 0x0003); // 1,2,4,8
			color = (color & 0x00f0) >> 4;
			sx = spbactn_spvideoram[offs+4];
			sy = spbactn_spvideoram[offs+3];
			flipx = attr & 0x0001;
			flipy = attr & 0x0002;

			for (row = 0;row < size;row++)
			{
				for (col = 0;col < size;col++)
				{
					int x = sx + 8*(flipx?(size-1-col):col);
					int y = sy + 8*(flipy?(size-1-row):row);
					drawgfx(bitmap,Machine->gfx[2],
						code + layout[row][col],
						color,
						flipx,flipy,
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}
}


VIDEO_START( spbactn )
{
	return 0;
}

VIDEO_UPDATE( spbactn )
{
	int offs, sx, sy;

	/* clear screen to black */
//	fillbitmap(bitmap,get_black_pen(),cliprect);

	/* draw table bg gfx */
	sx = sy = 0;
	for (offs = 0;offs < 0x4000/2;offs++)
	{
		int attr, code, colour;

		code = spbactn_bgvideoram[offs+0x4000/2];
		attr = spbactn_bgvideoram[offs+0x0000/2];

		colour = (attr & 0x00f0) >> 4;

		drawgfx(bitmap,Machine->gfx[1],
					code,
					colour,
					0,0,
					16*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);

		sx++; if (sx > 63) {sy++;sx=0;}
	}
	draw_sprites(bitmap,0);


	/* draw table fg gfx */
	sx = sy = 0;
	for (offs = 0;offs < 0x4000/2;offs++)
	{
		int attr, code, colour;

		code = spbactn_fgvideoram[offs+0x4000/2];
		attr = spbactn_fgvideoram[offs+0x0000/2];

		colour = (attr & 0x00f0) >> 4;

//		if (!(attr & 0x0008)) /* unknown effect, things draw with this bit set look a little wrong at the moment */
		drawgfx(bitmap,Machine->gfx[0],
					code,
					colour,
					0,0,
					16*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

		sx++; if (sx > 63) {sy++;sx=0;}
	}

	draw_sprites(bitmap,1);
}
