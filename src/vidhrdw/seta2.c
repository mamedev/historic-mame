/***************************************************************************

						  -= Newer Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


	This hardware only generates sprites. But they're of various types,
	including some large "floating tilemap" ones.

	Sprites RAM is 0x40000 bytes long. All games write the sprites list
	at offset 0x3000. Each entry in the list holds data for a multi-sprite
	of up to 256 single-sprites. The list looks like this:

	Offset: 	Bits:					Value:

		0.w		f--- ---- ---- ----		Last sprite
				-edc ba-- ---- ----		?
				---- --9- ---- ----		Color depth: 4 bits (0) or 8 bits (1)
				---- ---8 ---- ----		Select the   4 bits: low order (0) or high order (1)
				---- ---- 7654 3210		Number of sprites - 1

		2.w		fedc b--- ---- ----		?
				---- -a-- ---- ----		Size: 8 pixels (0) or 16 pixels (1)
				---- --98 7654 3210		X displacement

		4.w		fedc b--- ---- ----		?
				---- -a-- ---- ----		Size: 8 pixels (0) or 16 pixels (1)
				---- --98 7654 3210		Y displacement

		6.w		f--- ---- ---- ----		Single-sprite(s) type: tile (0) or row of tiles (1)
				-edc ba98 7654 3210		Offset of the single-sprite(s) data


	A single-sprite can be a tile or some horizontal rows of tiles.

	Tile case:

		0.w		fedc ba-- ---- ----
				---- --98 7654 3210		X

		2.w		fedc ba-- ---- ----
				---- --98 7654 3210		Y

		4.w		fedc ba98 765- ----		Color code (16 color steps)
				---- ---- ---4 ----		Flip X
				---- ---- ---- 3---		Flip Y
				---- ---- ---- -210		Code (high bits)

		6.w								Code (low bits)


	Row case:

		0.w		fedc ba-- ---- ----
				---- --98 7654 3210		X

		2.w		fedc ba-- ---- ----		Number of rows - 1
				---- --98 7654 3210		Y

		4.w		f--- ---- ---- ----		? Tile size (pzlbowl's "ram check" is invisible!)
				-edc ba-- ---- ----		"Tilemap" page
				---- --98 7654 3210		"Tilemap" Scroll X

		6.w		fedc ba-- ---- ----
				---- --98 7654 3210		"Tilemap" Scroll Y


Note: press Z to show some info on each sprite (debug builds only)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "seta.h"

data16_t *seta2_vregs;


/***************************************************************************


								Palette Init


***************************************************************************/

/* 256 color sprites, but the color code granularity is of 16 colors */
PALETTE_INIT( seta2 )
{
	int color, pen;
	for( color = 0; color < (0x8000/16); color++ )
		for( pen = 0; pen < 256; pen++ )
				colortable[color * 256 + pen + 0x8000] = (color * 16 + pen) % 0x8000;
}


/***************************************************************************


								Video Registers


***************************************************************************/

WRITE16_HANDLER( seta2_vregs_w )
{
	COMBINE_DATA(&seta2_vregs[offset]);
	switch( offset*2 )
	{
	case 0x1c:	// FLIP SCREEN (myangel)
		flip_screen_set( data & 1 );
		if (data & ~1)	logerror("CPU #0 PC %06X: flip screen unknown bits %04X\n",activecpu_get_pc(),data);
		break;
	case 0x2a:	// FLIP X (pzlbowl)
		flip_screen_x_set( data & 1 );
		if (data & ~1)	logerror("CPU #0 PC %06X: flipx unknown bits %04X\n",activecpu_get_pc(),data);
		break;
	case 0x2c:	// FLIP Y (pzlbowl)
		flip_screen_y_set( data & 1 );
		if (data & ~1)	logerror("CPU #0 PC %06X: flipy unknown bits %04X\n",activecpu_get_pc(),data);
		break;

	case 0x30:	// BLANK SCREEN (pzlbowl, myangel)
		if (data & ~1)	logerror("CPU #0 PC %06X: blank unknown bits %04X\n",activecpu_get_pc(),data);
		break;

	default:
		logerror("CPU #0 PC %06X: Video Reg %02X <- %04X\n",activecpu_get_pc(),offset*2,data);
	}
}


/***************************************************************************


								Sprites Drawing


***************************************************************************/

extern const struct GameDriver driver_pzlbowl;

static void seta2_draw_sprites(struct mame_bitmap *bitmap)
{

	/* Sprites list */

	data16_t *s1  = spriteram16 + 0x3000/2;
	data16_t *end = &spriteram16[spriteram_size/2];
	data16_t *s2, *s3;

	for ( ; s1 < end; s1+=4 )
	{
		int attr, code, color, num, sprite, gfx;

		int sx, x, xoffs, flipx, xnum, xstart, xend, xinc;
		int sy, y, yoffs, flipy, ynum, ystart, yend, yinc;

		num		=		s1[ 0 ];
		xoffs	=		s1[ 1 ];
		yoffs	=		s1[ 2 ];
		sprite	=		s1[ 3 ];

		/* Single-sprite address */
		s2		=		&spriteram16[ (sprite & 0x7fff) * 4 ];

		/* Single-sprite tile size */
		xnum	=		((xoffs & 0x0400) ? 2 : 1);	// tile size (1x8, 2x8)
		ynum	=		((yoffs & 0x0400) ? 2 : 1);

/*
	myangel2:					myangel:
14	copyright	4lo			12	writing		4lo?!!
15	japanese	4hi
16	heart		8
17	children	8
*/
		/* Color depth */
		if ((num & 0x0600) == 0x0600)
								gfx = 2;	// 8 bit tiles
		else
			if (num & 0x0100)	gfx = 1;	// 4 bit tiles (high order 4 bits)
			else				gfx = 0;	// 4 bit tiles (low  order 4 bits)

		/* Number of single-sprites */
		num		=		(num & 0x00ff) + 1;

		for( ; num > 0; num--,s2+=4 )
		{
			if (s2 >= end)	break;

			if (sprite & 0x8000)		// "tilemap" sprite
			{
				struct rectangle clip;
				int x1, page;

/*
[cross hatch myangel]				[after first error (green panel) myangel]
c03000:	350f 6bf0 0404 8800			c03030:	160f 6000 0408 9f47
c04000:	0000 0000 fbf4 0010			c0fa38:	0000 0000 fff0 0008
		0000 0010 fbf4 0010					..
		..									0000 0090 fff0 0008
		0000 00f0 fbf4 0010					63ef 00a0 fc40 0008
c3c000-c3cfff:	empty						..
c3d000-c3dfff:	<empty_line>				63ef 00f0 fc40 0008
				cross hatch			c3e000-c3efff:	empty
									c3f000-c3ffff	panel

[NS & Moss logo & crosshatch pzlbowl]
803000:	2703 0010 0100 8800
804000:	73f0 0c00 fc00 00f0
		73f0 0c40 fc00 00f0
		73f0 0c80 fc00 00f0
		73f0 0cc0 fc00 00f0
83e000-83eeff:	empty
83ef00-83efff:	logo
83f000-83ffff:	logo
				<empty_line>
*/

				sx		=		s2[ 0 ];
				sy		=		s2[ 1 ];
				x		=		s2[ 2 ];
				y		=		s2[ 3 ];

				xnum	=		0x20;
				ynum	=		((sy & 0xfc00) >> 10) + 1;

				page	=		((x & 0x7c00) >> 10);
/* Hack */
if (Machine->gamedrv == &driver_pzlbowl)
{
				x		+=		sx + xoffs;
				y		-=		sy + yoffs;
}
else
{
				x		+=		sx + 0x10 + xoffs;
				y		-=		sy + yoffs;
				sx += xoffs;
				sy += yoffs;
}

				sy = (sy & 0x0ff) - (sy & 0x100);
				clip.min_x = 0;
				clip.max_x = 0x200-1;
				clip.min_y = sy;
				clip.max_y = sy + ynum * 0x10 - 1;

				if (clip.min_x > Machine->visible_area.max_x)	continue;
				if (clip.min_y > Machine->visible_area.max_y)	continue;

				if (clip.max_x < Machine->visible_area.min_x)	continue;
				if (clip.max_y < Machine->visible_area.min_y)	continue;

				if (clip.min_x < Machine->visible_area.min_x)	clip.min_x = Machine->visible_area.min_x;
				if (clip.max_x > Machine->visible_area.max_x)	clip.max_x = Machine->visible_area.max_x;

				if (clip.min_y < Machine->visible_area.min_y)	clip.min_y = Machine->visible_area.min_y;
				if (clip.max_y > Machine->visible_area.max_y)	clip.max_y = Machine->visible_area.max_y;

				x1 = -x;

				if (y & 0xf)	y	=	0x10 - (y & 0xf) + (y & ~0xf);
				else			y	-=	0x10;
				y &= 0x1ff;

				/* Draw the rows */
				sy	-=	(y & 0xf);
				for (; sy <= clip.max_y; sy+=0x10,y-=0x10)
				{
					for (x=x1,sx = clip.min_x - (x & 0xf); sx <= clip.max_x; sx+=0x10,x+=0x10)
					{
						int tx, ty;
						s3	=	&spriteram16[	(page * (0x2000/2))	+
												((y & 0x01f0) << 3)	+
												((x & 0x03f0) >> 3)		];

						attr	=	s3[0];
						code	=	s3[1];

						code	+=		(attr & 0x0007) << 16;
						flipy	=		(attr & 0x0008);
						flipx	=		(attr & 0x0010);
						color	=		(attr & 0xffe0) >> 5;

						/* Force 16x16 tiles ? */
						if (flipx)	{ xstart = 2-1;  xend = -1; xinc = -1; }
						else		{ xstart = 0;    xend = 2;  xinc = +1; }

						if (flipy)	{ ystart = 2-1;  yend = -1; yinc = -1; }
						else		{ ystart = 0;    yend = 2;  yinc = +1; }

						/*	Not very accurate (the low bits change when
						the tile is flipped) but it works	*/
						code &= ~3;

						/* Draw a tile (16x16) */
						for (ty = ystart; ty != yend; ty += yinc)
						{
							for (tx = xstart; tx != xend; tx += xinc)
							{
								drawgfx( bitmap,	Machine->gfx[gfx],
													code++,
													color,
													flipx, flipy,
													sx + tx * 8, sy + ty * 8,
													&clip,TRANSPARENCY_PEN,0 );
							}
						}
					}
				}
			}
			else			// "normal" sprite
			{
				sx		=		s2[ 0 ];
				sy		=		s2[ 1 ];
				attr	=		s2[ 2 ];
				code	=		s2[ 3 ];

				code	+=		(attr & 0x0007) << 16;
				flipy	=		(attr & 0x0008);
				flipx	=		(attr & 0x0010);
				color	=		(attr & 0xffe0) >> 5;

/* Hack */
if (Machine->gamedrv == &driver_pzlbowl)
;
else
{
				sx += xoffs;
				sy += yoffs;
}

				sx = (sx & 0x1ff) - (sx & 0x200);
				sy = (sy & 0x0ff) - (sy & 0x100);

				if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1; }
				else		{ xstart = 0;       xend = xnum;  xinc = +1; }

				if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1; }
				else		{ ystart = 0;       yend = ynum;  yinc = +1; }

				/*	Not very accurate (the low bits change when
					the tile is flipped) but it works	*/
				code &= ~3;

				for (y = ystart; y != yend; y += yinc)
				{
					for (x = xstart; x != xend; x += xinc)
					{
						drawgfx( bitmap,	Machine->gfx[gfx],
											code++,
											color,
											flipx, flipy,
											sx + x * 8, sy + y * 8,
											&Machine->visible_area,TRANSPARENCY_PEN,0 );
					}
				}
				#ifdef MAME_DEBUG
				if (keyboard_pressed(KEYCODE_Z))	/* Display some info on each sprite */
				{	struct DisplayText dt[2];	char buf[10];
					sprintf(buf, "%02X",s1[0]>>8);
					dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
					dt[0].x = sx;		dt[0].y = sy - Machine->visible_area.min_y;
					dt[1].text = 0;	/* terminate array */
					displaytext(Machine->scrbitmap,dt);		}
				#endif

			}		/* sprite type */

		}	/* single-sprites */

		/* Last sprite */
		if (s1[0] & 0x8000) break;

	}	/* sprite list */
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( seta2 )
{
	/* Black or pens[0]? */
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	if (!(seta2_vregs[0x30/2] & 1))	// BLANK SCREEN
		seta2_draw_sprites(bitmap);
}
