#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1
#define TC0480SCP_GFX_NUM 1

void taitoz_vh_stop(void);

static int sci_spriteframe;
extern data16_t *taitoz_sharedram;


/**********************************************************
                        TC0150ROD
**********************************************************/

static data16_t *TC0150ROD_ram;
#define TC0150ROD_RAM_SIZE 0x2000

READ16_HANDLER( TC0150ROD_word_r )
{
	return TC0150ROD_ram[offset];
}

WRITE16_HANDLER( TC0150ROD_word_w )
{
	COMBINE_DATA(&TC0150ROD_ram[offset]);
}

int TC0150ROD_vh_start(void)
{
	TC0150ROD_ram = malloc(TC0150ROD_RAM_SIZE);

	if (!TC0150ROD_ram) return 1;

	state_save_register_UINT16("TC0150ROD", 0, "memory", TC0150ROD_ram, TC0150ROD_RAM_SIZE/2);
	return 0;
}

void TC0150ROD_vh_stop(void)
{
	free(TC0150ROD_ram);
	TC0150ROD_ram = 0;
}

/* These scanline drawing routines lifted from Taito F3: optimise / merge ? */

#undef ADJUST_FOR_ORIENTATION
#define ADJUST_FOR_ORIENTATION(type, orientation, bitmapi, bitmapp, x, y)	\
	type *dsti = &((type *)bitmapi->line[y])[x];							\
	UINT8 *dstp = &((UINT8 *)bitmapp->line[y])[x];							\
	int xadv = 1;															\
	if (orientation)														\
	{																		\
		int dy = (type *)bitmap->line[1] - (type *)bitmap->line[0];			\
		int tx = x, ty = y, temp;											\
		if ((orientation) & ORIENTATION_SWAP_XY)							\
		{																	\
			temp = tx; tx = ty; ty = temp;									\
			xadv = dy / sizeof(type);										\
		}																	\
		if ((orientation) & ORIENTATION_FLIP_X)								\
		{																	\
			tx = bitmap->width - 1 - tx;									\
			if (!((orientation) & ORIENTATION_SWAP_XY)) xadv = -xadv;		\
		}																	\
		if ((orientation) & ORIENTATION_FLIP_Y)								\
		{																	\
			ty = bitmap->height - 1 - ty;									\
			if ((orientation) & ORIENTATION_SWAP_XY) xadv = -xadv;			\
		}																	\
		/* can't lookup line because it may be negative! */					\
		dsti = (type *)((type *)bitmapi->line[0] + dy * ty) + tx;			\
		dstp = (UINT8 *)((UINT8 *)bitmapp->line[0] + dy * ty / sizeof(type)) + tx;	\
	}

INLINE void bryan3_drawscanline(
		struct mame_bitmap *bitmap,int x,int y,int length,
		const UINT16 *src,int transparent,UINT32 orient,int pri)
{
	ADJUST_FOR_ORIENTATION(UINT16, Machine->orientation ^ orient, bitmap, priority_bitmap, x, y);
	if (transparent) {
		while (length--) {
			UINT32 spixel = *src++;
			if (spixel<0x7fff) {
				*dsti = spixel;
				*dstp = pri;
			}
			dsti += xadv;
			dstp += xadv;
		}
	} else { /* Not transparent case */
		while (length--) {
			*dsti = *src++;
			*dstp = pri;
			dsti += xadv;
			dstp += xadv;
		}
	}
}
#undef ADJUST_FOR_ORIENTATION



/******************************************************************************

	Memory map for TC0150ROD
	------------------------

	0000-07ff  Road A, bank 0   [all are 256 lines]
	0800-0fff  Road A, bank 1
	1000-17ff  Road B, bank 0
	1800-1fff  Road B, bank 1

	1ffe-1fff  Control word

	           Contcirc: 08 0d   [bifurcating]
	           ChaseHQ:  00 05   [09 0c when road bifurcates]
	                             [08 0d when road rejoins]
	           Nightstr: 08 0d   [both bifurcating and not...]
	           Dblaxle:  08 0d

	           1000 1101
	           1001 1100
	           0000 0101


	Road ram line layout (thanks to Raine for original table)
	--------------------

	-----+-----------------+----------------------------------------
	Word | Bit(s)          |  Info
	-----+-----------------+----------------------------------------
	  0  |x....... ........|  Draw background outside road edge on LHS
	  0  |.x...... ........|  Left road edge from road A has priority over road B ?? (+)
	  0  |..x..... ........|  Left road edge from road A has priority over road B ?? (*)
	  0  |...x.... ........|  Left edge/background palette entry offset  (set = +2)
	  0  |......xx xxxxxxxx|  Left edge   [pixels from road center] (@)
	     |                 |
	  1  |x....... ........|  Draw background outside road edge on RHS
	  1  |.x...... ........|  Right road edge from road A has priority over road B ??
	  1  |..x..... ........|  Right road edge from road A has priority over road B ?? (*)
	  1  |...x.... ........|  Right edge/background palette entry offset  (set = +2)
	  1  |......xx xxxxxxxx|  Right edge   [pixels from road center] (@)
	     |                 |
	  2  |x....... ........|  ? unknown
	  2  |..x..... ........|  Road line body from Road A has higher priority than Road B ??
	  2  |...x.... ........|  Palette entry offset   (set = +2)
	  2  |....?... ........|  ? unknown
	  2  |.....xxx xxxxxxxx|  X Offset   [offset is inverted] (^)
	     |                 |
	  3  |xxxx.... ........|  Color Bank  (selects group of 4 palette entries used for line)
	  3  |......xx xxxxxxxx|  Road Gfx Tile number
	-----+-----------------+-----------------------------------------

	@ size of bitmask suggested by Nightstr stage C when boss appears
	^ bitmask confirmed in ChaseHQ code

	* see Nightstr "stage choice tunnel"
	+ see Contcirc track at race start

	These priority bits have a different meaning in road B ram. They appear to mean
	that the relevant part of road B slips under road A. I.e. in road A they raise
	priority, in road B they lower it.

	We need a screenshot of Nightstr "stage choice tunnel" showing exactly what effect
	happens at top and bottom of screen while tunnel bifurcates.


Priority Levels - used by this code to represent the way the TC0150ROD appears to work
---------------

To speed up the code, two bits in the existing pixel-color map are used to store
priority information:

x....... ........ = transparency
.xx..... ........ = pixel priority
....xxxx xxxxxxxx = pixel pen

The reserved bit will be needed if any TaitoZ games using twice the palette space turn
up that also use TC0150ROD. This seems unlikely...


Road A:  1 [standard - low], 3 [high]
Road B:  0 [low], 2 [standard - high]

These may need revising once Nightstr screenshots are obtained. (They may not fit in the
spare bits of the pixel map as we manage to at the moment...)

One possible theory is that the 'background' outside road always has lowest priority and
will go under any bits of road and road edge. At the moment we kludge this a little for
Nightstr for when it draws background for the 'top' road but _doesn't_ want to erase the
road underneath.

What if road bodies ALWAYS have priority over road edges? That would probably fix
Nightstr but might hurt Contcirc ?

Possible alternative priority system.....
Background: 0
Road edge: lo=1 (Road A,B standard) hi=2 [road A on top]
Road body: lo=3 (Road A,B standard) hi=4 [road A on top]
We might need 4 levels for both edge and body however.



Road info
---------

Road gfx is 2bpp, each word holds 8 pixels in this format:
xxxxxxxx ........  lo 8 bits
........ xxxxxxxx  hi 8 bits

The line gfx is back to front: this is why we call 'left' 'right' and vice versa in
this code: when the pixels are poked in they are done in reverse order, restoring
the orientation.

Each road gfx tile is 0x200 long in the rom. This comprises TWO road lines each
of 1024x1 pixels.

The first is the "edge" graphic. The second is the road body graphic. This means
separate sets of colors can be used for road edge and road body, giving greater
color variety.

The edge graphic is stored with the edges touching each other. So we must pull LHS
and RHS out separately to draw them.

Gfx lines: generally 0-0x1ff are the standard group (higher tile number indexes
wider lines). However this is just the way the games are: NOT a function of the
TC0150ROD.

Proof of background palette entry offset is in Contcirc in the tunnel on
Monaco level, the flyer screenshot shows different background colors on left
and right.

To investigate the weird road A/B priority system look at Nightstr and also
Contcirc. Contcirc: the "pit entry lane" in road B looks completely wrong if it is
allowed on top of road A body.

Aquajack road requires correct bank selection, or it goes crazy.

Should pen0 in Road A body be transparent? It seems necessary for Bshark round 6
and it makes Aquajack roads look much better. However, in Nightstr stage C
this results in a black band in the middle of the water. Also, it leaves
transparent areas in Dblaxle road which look obviously wrong. For time being a
parameter has been added to select which games use the transparency.

******************************************************************************/


void TC0150ROD_draw(struct mame_bitmap *bitmap,int y_offs,int palette_offs,int type,int road_trans,UINT32 priority)
{
#ifdef MAME_DEBUG
	static int dislayer[6];	/* Road Layer toggles to help get road correct */
	char buf[80];
#endif

	UINT16 scanline[512];
	UINT16 roada_line[512],roadb_line[512];
	data16_t *dst16;
	data16_t *roada,*roadb;
	data16_t *roadgfx = (data16_t *)memory_region(REGION_GFX3);

	UINT16 pixel,color,gfx_word,clipl,clipr,bodyctrl;
	UINT16 pri,edgepri,pixpri;
	int x_index,roadram_index,roadram2_index,i;
	int xoffset,paloffs,palloffs,palroffs;
	int road_gfx_tilenum,colbank,road_center;
	int road_ctrl = TC0150ROD_ram[0xfff];
	int left_edge,right_edge,begin,end,right_over,left_over;
	int line_needs_drawing,draw_top_road_line,background_only;

	int rot=Machine->orientation;
	int min_x = Machine->visible_area.min_x;
	int max_x = Machine->visible_area.max_x;
	int min_y = Machine->visible_area.min_y;
	int max_y = Machine->visible_area.max_y;
	int screen_width = max_x - min_x + 1;

	int y = min_y;

	int road_A_address = y_offs * 4;		/* Index into roadram for road A */
	int road_B_address = y_offs * 4 + 0x800;		/* Same for road B */

#ifdef MAME_DEBUG
	if (keyboard_pressed_memory (KEYCODE_X))
	{
		dislayer[0] ^= 1;
		sprintf(buf,"RoadA body: %01x",dislayer[0]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_C))
	{
		dislayer[1] ^= 1;
		sprintf(buf,"RoadA l-edge: %01x",dislayer[1]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_V))
	{
		dislayer[2] ^= 1;
		sprintf(buf,"RoadA r-edge: %01x",dislayer[2]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_B))
	{
		dislayer[3] ^= 1;
		sprintf(buf,"RoadB body: %01x",dislayer[3]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_N))
	{
		dislayer[4] ^= 1;
		sprintf(buf,"RoadB l-edge: %01x",dislayer[4]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed_memory (KEYCODE_M))
	{
		dislayer[5] ^= 1;
		sprintf(buf,"RoadB r-edge: %01x",dislayer[5]);
		usrintf_showmessage(buf);
	}
#endif

	/* Check which bank Road A should be drawn from */
	if ((road_ctrl &0x100))	road_A_address += 0x400;

	/* Check which bank Road B should be drawn from */
	if ((road_ctrl &0x400))	road_B_address += 0x400;

	do
	{
		line_needs_drawing = 0;

		roadram_index  = road_A_address + y * 4;	/* in case there is some switching mechanism (unlikely) */
		roadram2_index = road_B_address + y * 4;

		roada = roada_line;
		roadb = roadb_line;

		for (i=0;i<screen_width;i++)	/* Default transparency fill */
		{
			*roada++ = 0x8000;
			*roadb++ = 0x8000;
		}

		/********************************************************/
		/*                        ROAD A                        */
            /********************************************************/

		clipr    = TC0150ROD_ram[roadram_index];
		palroffs =(clipr &0x1000) >> 11;
		clipl    = TC0150ROD_ram[roadram_index+1];
		palloffs =(clipl &0x1000) >> 11;
		bodyctrl = TC0150ROD_ram[roadram_index+2];
		xoffset  = bodyctrl &0x7ff;
		paloffs  =(bodyctrl &0x1800) >> 11;
		colbank  =(TC0150ROD_ram[roadram_index+3] &0xf000) >> 10;
		road_gfx_tilenum = TC0150ROD_ram[roadram_index+3] &0x3ff;
		right_over = 0;
		left_over = 0;

		road_center = 0x5ff - ((-xoffset + 0xa6) &0x7ff);
		left_edge = road_center - (clipl &0x3ff);		/* start pixel for left edge */
		right_edge = road_center + 1 + (clipr &0x3ff);	/* start pixel for right edge */

		if ((clipl) || (clipr))	line_needs_drawing = 1;

		/* Main road line is drawn from 'begin' to 'end'-1 */

		begin = left_edge + 1;
		if (begin < 0)
		{
			begin = 0;	/* can't begin off edge of screen */
		}

		end = right_edge;
		if (end > screen_width)
		{
			end = screen_width;	/* can't end off edge of screen */
		}

		/* We need to offset start pixel we draw for road edge when edge of
		   road is partially or wholly offscreen on the opposite side
		   e.g. Contcirc attract */

		if (right_edge < 0)
		{
			right_over = -right_edge;
			right_edge = 0;
		}
		if (left_edge >= screen_width)
		{
			left_over = left_edge - screen_width + 1;
			left_edge = screen_width - 1;
		}

		/* If road is way off to right we only need to plot background */
		background_only = (road_center > (screen_width + 1024/2)) ? 1 : 0;


		/********* Draw main part of road *********/

		color = ((palette_offs + colbank + paloffs) << 4) + ((type) ? (1) : (4));
		pri = (bodyctrl &0x2000) ? (0x6000) : (0x2000);

#ifdef MAME_DEBUG
	if (!dislayer[0])
#endif
		{
		/* Is this calculation imperfect ?  (0xa0 = screen width/2) */
		x_index = (-xoffset + 0xa6 + begin) &0x7ff;

		roada = roada_line + screen_width - 1 - begin;

		if ((line_needs_drawing) && (begin < end))
		{
			for (i=begin; i<end; i++)
			{
				if (road_gfx_tilenum)	/* fixes Nightstr round C */
				{
					gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
					pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);

					if ((pixel) || !(road_trans))
					{
						if (type==1)	pixel = (pixel-1)&3;
						*roada-- = (color + pixel) | pri;
					}
					else	*roada-- = 0x8000;	/* transparency, fixes Bshark round 6 + Aquajack */
				}
				else roada--;

				x_index++;
				x_index &= 0x7ff;
			}
		}
		}


		/********* Draw 'left' road edge *********/

		color = ((palette_offs + colbank + palloffs) << 4) + ((type) ? (1) : (4));
		pri = (clipl &0x2000) ? (0x6000) : (0x2000);

#ifdef MAME_DEBUG
	if (!dislayer[2])
#endif
		{
		if (background_only)	/* The "road edge" line is entirely off screen so can't be drawn */
		{
			if (clipl &0x8000)	/* but we may need to fill in the background color */
			{
				roada = roada_line;
				for (i=0;i<screen_width;i++)
				{
					*roada++ = (color + (type ? (3) : (0))) | pri;
				}
			}
		}
		else
		{
			if ((left_edge >= 0) && (left_edge < screen_width))
			{
				x_index = (1024/2 - 1 - left_over) &0x7ff;

				roada = roada_line + screen_width - 1 - left_edge;

				if (line_needs_drawing)
				{
					for (i=left_edge; i>=0; i--)
					{
						gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
						pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);
						if ((pixel==0) && !(clipl &0x8000))
						{
							roada++;
						}
						else
						{
							if (type==1)	pixel = (pixel-1)&3;
							*roada++ = (color + pixel) | pri;
						}

						x_index--;
						x_index &= 0x7ff;
					}
				}
			}
		}
		}


		/********* Draw 'right' road edge *********/

		color = ((palette_offs + colbank + palroffs) << 4) + ((type) ? (1) : (4));
		pri = (clipr &0x2000) ? (0x6000) : (0x2000);

#ifdef MAME_DEBUG
	if (!dislayer[1])
#endif
		{
		if ((right_edge < screen_width) && (right_edge >= 0))
		{
			x_index = (1024/2 + right_over) &0x7ff;

			roada = roada_line + screen_width - 1 - right_edge;

			if (line_needs_drawing)
			{
				for (i=right_edge; i<screen_width; i++)
				{
					gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
					pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);
					if ((pixel==0) && !(clipr &0x8000))
					{
						roada--;
					}
					else
					{
						if (type==1)	pixel = (pixel-1)&3;
						*roada-- = (color + pixel) | pri;
					}

					x_index++;
					x_index &= 0x7ff;
				}
			}
		}
		}


		/********************************************************/
		/*                        ROAD B                        */
            /********************************************************/

		clipr    = TC0150ROD_ram[roadram2_index];
		palroffs = (clipr &0x1000) >> 11;
		clipl    = TC0150ROD_ram[roadram2_index+1];
		palloffs = (clipl &0x1000) >> 11;
		bodyctrl = TC0150ROD_ram[roadram2_index+2];
		xoffset  = bodyctrl &0x7ff;
		paloffs  =(bodyctrl &0x1800) >> 11;
		colbank  =(TC0150ROD_ram[roadram2_index+3] &0xf000) >> 10;
		road_gfx_tilenum = TC0150ROD_ram[roadram2_index+3] &0x3ff;
		right_over = 0;
		left_over = 0;

		road_center = 0x5ff - ((-xoffset + 0xa6) &0x7ff);


// 0a6 and lower => 0x5ff 5fe etc.

// 35c => 575 right road edge wraps back onto other side of screen
// 5ff-54a     thru    5ff-331
// b6          thru    2ce

// 2a6 thru 0 thru 5a7 ??

		left_edge = road_center - (clipl &0x3ff);		/* start pixel for left edge */
		right_edge = road_center + 1 + (clipr &0x3ff);	/* start pixel for right edge */

		if (((clipl) || (clipr)) && (road_ctrl &0x800))
		{
			draw_top_road_line = 1;
			line_needs_drawing = 1;
		}
		else	draw_top_road_line = 0;

		/* Main road line is drawn from 'begin' to 'end'-1 */

		begin = left_edge + 1;
		if (begin < 0)
		{
			begin = 0;	/* can't begin off edge of screen */
		}

		end = right_edge;
		if (end > screen_width)
		{
			end = screen_width;	/* can't end off edge of screen */
		}

		/* We need to offset start pixel we draw for road edge when edge of
		   road is partially or wholly offscreen on the opposite side
		   e.g. Contcirc attract */

		if (right_edge < 0)
		{
			right_over = -right_edge;
			right_edge = 0;
		}
		if (left_edge >= screen_width)
		{
			left_over = left_edge - screen_width + 1;
			left_edge = screen_width - 1;
		}

		/* If road is way off to right we only need to plot background */
		background_only = (road_center > (screen_width + 1024/2)) ? 1 : 0;


		/********* Draw main part of road *********/

		color = ((palette_offs + colbank + paloffs) << 4) + ((type) ? (1) : (4));
		pri = (bodyctrl &0x2000) ? (0) : (0x4000);

#ifdef MAME_DEBUG
	if (!dislayer[3])
#endif
		{
		/* Is this calculation imperfect ?  (0xa0 = screen width/2) */
		x_index = (-xoffset + 0xa6 + begin) &0x7ff;

		if (x_index > 0x3ff)	/* Second half of gfx contains the road body line */
		{
			roadb = roadb_line + screen_width - 1 - begin;

			if (draw_top_road_line && road_gfx_tilenum && (begin < end))
			{
				for (i=begin; i<end; i++)
				{
					gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
					pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);

					if (type==1)	pixel = (pixel-1)&3;
					*roadb-- = (color + pixel) | pri;

					x_index++;
					x_index &= 0x7ff;
				}
			}
		}
		}


		/********* Draw 'left' road edge *********/

		color = ((palette_offs + colbank + palloffs) << 4) + ((type) ? (1) : (4));
		edgepri = (clipl &0x2000) ? (0) : (0x4000);

#ifdef MAME_DEBUG
	if (!dislayer[5])
#endif
		{
		if (background_only)	/* The "road edge" line is entirely off screen so can't be drawn */
		{
			if ((clipl &0x8000) && draw_top_road_line)	/* but we may need to fill in the background color */
			{
				roadb = roadb_line;
				for (i=0;i<screen_width;i++)
				{
					*roadb++ = (color + (type ? (3) : (0))) | pri;
				}
			}
		}
		else
		{
			if ((left_edge >= 0) && (left_edge < screen_width))
			{
				x_index = (1024/2 - 1 - left_over) &0x7ff;

				roadb = roadb_line + screen_width - 1 - left_edge;

				if (draw_top_road_line)
				{
					for (i=left_edge; i>=0; i--)
					{
						gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
						pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);

						pixpri = (pixel==0) ? (pri) : (edgepri);	/* background follows body priority */
						if ((pixel==0) && !(clipl &0x8000))	/* test for background disabled */
						{
							roadb++;
						}
						else
						{
							if (type==1)	pixel = (pixel-1)&3;
							*roadb++ = (color + pixel) | pixpri;
						}

						x_index--;
						if (x_index < 0)	break;
					}
				}
			}
		}
		}


		/********* Draw 'right' road edge *********/

		color = ((palette_offs + colbank + palroffs) << 4) + ((type) ? (1) : (4));
		edgepri = (clipr &0x2000) ? (0) : (0x4000);

#ifdef MAME_DEBUG
	if (!dislayer[4])
#endif
		{
		if ((right_edge < screen_width) && (right_edge >= 0))
		{
			x_index = (1024/2 + right_over) &0x7ff;

			roadb = roadb_line + screen_width - 1 - right_edge;

			if (draw_top_road_line)
			{
				for (i=right_edge; i<screen_width; i++)
				{
					gfx_word = roadgfx[(road_gfx_tilenum << 8) + (x_index >> 3)];
					pixel = ((gfx_word >> (7-(x_index % 8) + 8)) &0x1) * 2 + ((gfx_word >> (7-(x_index % 8))) &0x1);

					pixpri = (pixel==0) ? (pri) : (edgepri);	/* background follows body priority */
					if ((pixel==0) && !(clipr &0x8000))	/* test for background disabled */
					{
						roadb--;
					}
					else
					{
						if (type==1)	pixel = (pixel-1)&3;
						*roadb-- =  (color + pixel) | pixpri;
					}

					x_index++;
					if (x_index > 0x3ff)	break;
				}
			}
		}
		}


		/******** Combine the two lines according to pixel priorities ********/

		if (line_needs_drawing)
		{

			if (rot & ORIENTATION_FLIP_X)
			{
				dst16 = scanline + screen_width - 1;

				for (i=0;i<screen_width;i++)
				{
					if (roada_line[i] == 0x8000)	/* road A pixel transparent */
					{
						*dst16-- = roadb_line[i] & 0x9fff;
					}
					else if (roadb_line[i] == 0x8000)	/* road B pixel transparent */
					{
						*dst16-- = roada_line[i] & 0x9fff;
					}
					else	/* two competing pixels, which has highest priority... */
					{
						if ((roadb_line[i] & 0x6000) >= (roada_line[i] & 0x6000))
						{
							*dst16-- = roadb_line[i] & 0x9fff;
						}
						else
 						{
							*dst16-- = roada_line[i] & 0x9fff;
						}
					}
				}
			}
			else	/* standard non-flipped case */
			{
				dst16 = scanline;

				for (i=0;i<screen_width;i++)
				{
					if (roada_line[i] == 0x8000)	/* road A pixel transparent */
					{
						*dst16++ = roadb_line[i] & 0x9fff;
					}
					else if (roadb_line[i] == 0x8000)	/* road B pixel transparent */
					{
						*dst16++ = roada_line[i] & 0x9fff;
					}
					else	/* two competing pixels, which has highest priority... */
					{
						if ((roadb_line[i] & 0x6000) >= (roada_line[i] & 0x6000))
						{
							*dst16++ = roadb_line[i] & 0x9fff;
						}
						else
 						{
							*dst16++ = roada_line[i] & 0x9fff;
						}
					}
				}
			}

			bryan3_drawscanline(bitmap,0,y,screen_width,scanline,1,rot,priority);
		}

		y++;
	}
	while (y <= max_y);
}



/**********************************************************/

static int has_TC0480SCP(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the TC0480SCP is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0480SCP_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_TC0150ROD(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers for CPUA/B and see if the TC0150ROD is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[1].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[2].memory_write;	// needed in case Z80 sandwiched between the 68Ks
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

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

static int taitoz_core_vh_start (int x_offs)
{
	if (has_TC0480SCP())	/* it's Dblaxle, a tc0480scp game */
	{
		if (TC0480SCP_vh_start(TC0480SCP_GFX_NUM,x_offs,0x21,0x08,4,0,0,0,0))
		{
			taitoz_vh_stop();
			return 1;
		}
	}
	else	/* it's a tc0100scn game */
	{
		if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,0))
		{
			taitoz_vh_stop();
			return 1;
		}
	}

	if (has_TC0150ROD())
		if (TC0150ROD_vh_start())
		{
			taitoz_vh_stop();
			return 1;
		}

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
		{
			taitoz_vh_stop();
			return 1;
		}

	return 0;
}

int taitoz_vh_start (void)
{
	return (taitoz_core_vh_start(0));
}

int spacegun_vh_start (void)
{
	return (taitoz_core_vh_start(4));
}

void taitoz_vh_stop (void)
{
	if (has_TC0480SCP())
	{
		TC0480SCP_vh_stop();
	}
	else
	{
		TC0100SCN_vh_stop();
	}

	if (has_TC0150ROD())
		TC0150ROD_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();
}


/********************************************************
            SPRITE READ AND WRITE HANDLERS
********************************************************/


READ16_HANDLER( sci_spriteframe_r )
{
	return (sci_spriteframe << 8);
}

WRITE16_HANDLER( sci_spriteframe_w )
{
	sci_spriteframe = (data >> 8) &0xff;
}


/************************************************************
                   SPRITE DRAW ROUTINES

These draw a series of small tiles ("chunks") together to
create each big sprite. The spritemap rom provides the lookup
table for this. E.g. Spacegun looks up 16x8 sprite chunks
from the spritemap rom, creating this 64x64 sprite (numbers
are the word offset into the spritemap rom):

	 0  1  2  3
	 4  5  6  7
	 8  9 10 11
	12 13 14 15
	16 17 18 19
	20 21 22 23
	24 25 26 27
	28 29 30 31

Chasehq/Nightstr are the only games to build from 16x16
tiles. They are also more complicated to draw, as they have
3 different aggregation formats [32/64/128 x 128]
whereas the other games stick to one, typically 64x64.

All the games make heavy use of sprite zooming.

I'm 99% sure there are probably just two levels of sprite
priority - under and over the road - but this isn't certain.

		***

[The routines for the 16x8 tile games seem to have large
common elements that could be extracted as subroutines.]

NB: unused portions of the spritemap rom contain hex FF's.
It is a useful coding check to warn in the log if these
are being accessed. [They can be inadvertently while
spriteram is being tested, take no notice of that.]

BUT: Nightstr uses code 0x3ff as a mask sprite. This
references 0x1ff00-ff in spritemap rom, which is an 0xff fill.
This must be accessing sprite chunk 0x3fff: in other words the
top bits are thrown away and so accessing chunk 0xfe45 would
display chunk 0x3e45 etc.


		Aquajack/Spacegun (modified table from Raine; size of bit
		masks verified in Spacegun code)

		Byte | Bit(s) | Description
		-----+76543210+-------------------------------------
		  0  |xxxxxxx.| ZoomY (0 min, 63 max - msb unused as sprites are 64x64)
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Priority (0=sprites high)
		  2  |.x......| Flip X
		  2  |..?????.| unknown/unused ?
		  2  |.......x| X position (High)
		  3  |xxxxxxxx| X position (Low)
		  4  |xxxxxxxx| Palette bank
		  5  |?.......| unknown/unused ?
		  5  |.xxxxxxx| ZoomX (0 min, 63 max - msb unused as sprites are 64x64)
		  6  |x.......| Flip Y ?
		  6  |.??.....| unknown/unused ?
		  6  |...xxxxx| Sprite Tile high (msb unused - half of spritemap rom empty)
		  7  |xxxxxxxx| Sprite Tile low

		Continental circus (modified Raine table): note similar format.
		The zoom msb is actually used, as sprites are 128x128.

		---+-------------------+--------------
		 0 | xxxxxxx. ........ | ZoomY
		 0 | .......x xxxxxxxx | Y position
						// unsure about Flip Y
		 2 | .....xxx xxxxxxxx | Sprite Tile
		 4 | x....... ........ | Priority (0=sprites high)
		 4 | .x...... ........ | Flip X
		 4 | .......x xxxxxxxx | X position
		 6 | xxxxxxxx ........ | Palette bank
		 6 | ........ .xxxxxxx | ZoomX
		---+-------------------+--------------

		 Bshark/Chasehq/Nightstr/SCI (modified Raine table): similar format.
		 The zoom msb is only used for 128x128 sprites.

		-----+--------+------------------------
		Byte | Bit(s) | Description
		-----+76543210+------------------------
		  0  |xxxxxxx.| ZoomY
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Priority (0=sprites high)
		  2  |.xxxxxxx| Palette bank (High)
		  3  |x.......| Palette bank (Low)
		  3  |.xxxxxxx| ZoomX
		  4  |x.......| Flip Y
		  4  |.x......| Flip X
		  4  |.......x| X position (High)
		  5  |xxxxxxxx| X position (Low)
		  6  |...xxxxx| Sprite Tile (High)
		  7  |xxxxxxxx| Sprite Tile (Low)
		-----+--------+------------------------

 [Raine Chasehq sprite plotting is peculiar. It determines
 the type of big sprite by reference to the zoomx and y.
 Therefore it says that the big sprite number is 0-0x7ff
 across ALL three sizes and you can't distinguish them by
 tilenum. FWIW I seem to be ok just using zoomx.]

TODO: Contcirc, Aquajack, Spacegun need flip y bit to be
confirmed

********************************************************/


static void contcirc_draw_sprites_16x8(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		tilenum = data & 0x7ff;		/* $80000 spritemap rom maps up to $7ff 128x128 sprites */

		data = spriteram16[offs+2];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		flipy = (data & 0x2000) >> 13;	// ???
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+3];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		if (!tilenum) continue;

		map_offset = tilenum << 7;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<128;sprite_chunk++)
		{
			k = sprite_chunk % 8;   /* 8 sprite chunks per row */
			j = sprite_chunk / 8;   /* 16 rows */

			px = flipx ? (7-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (15-j) : j;

			code = spritemap[map_offset + px + (py<<3)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/8);
			cury = y + ((j*zoomy)/16);

			zx = x + (((k+1)*zoomx)/8) - curx;
			zy = y + (((j+1)*zoomy)/16) - cury;

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

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,primasks[priority]);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void chasehq_draw_sprites_16x16(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = spriteram_size/2-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		/* higher bits are sometimes used... e.g. sign over flashing enemy car...! */
		tilenum = data & 0x7ff;

		if (!tilenum) continue;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		if ((zoomx-1) & 0x40)	/* 128x128 sprites, $0-$3ffff in spritemap rom, OBJA */
		{
			map_offset = tilenum << 6;

			for (sprite_chunk=0;sprite_chunk<64;sprite_chunk++)
			{
				j = sprite_chunk / 8;   /* 8 rows */
				k = sprite_chunk % 8;   /* 8 sprite chunks per row */

				px = flipx ? (7-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<3)];

				if (code == 0xffff)	bad_chunks++;

				curx = x + ((k*zoomx)/8);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/8) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

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

				pdrawgfxzoom(bitmap,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						curx,cury,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}
		else if ((zoomx-1) & 0x20)	/* 64x128 sprites, $40000-$5ffff in spritemap rom, OBJB */
		{
			map_offset = (tilenum << 5) + 0x20000;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				j = sprite_chunk / 4;   /* 8 rows */
				k = sprite_chunk % 4;   /* 4 sprite chunks per row */

				px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<2)];

				if (code == 0xffff)	bad_chunks++;

				curx = x + ((k*zoomx)/4);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/4) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

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

				pdrawgfxzoom(bitmap,Machine->gfx[2],
						code,
						color,
						flipx,flipy,
						curx,cury,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}
		else if (!((zoomx-1) & 0x60))	/* 32x128 sprites, $60000-$7ffff in spritemap rom, OBJB */
		{
			map_offset = (tilenum << 4) + 0x30000;

			for (sprite_chunk=0;sprite_chunk<16;sprite_chunk++)
			{
				j = sprite_chunk / 2;   /* 8 rows */
				k = sprite_chunk % 2;   /* 2 sprite chunks per row */

				px = flipx ? (1-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<1)];

				if (code == 0xffff)	bad_chunks ++;

				curx = x + ((k*zoomx)/2);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/2) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

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

				pdrawgfxzoom(bitmap,Machine->gfx[2],
						code,
						color,
						flipx,flipy,
						curx,cury,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void bshark_draw_sprites_16x8(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = spriteram_size/2-4;offs >= 0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

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

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void sci_draw_sprites_16x8(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, start_offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	/* SCI alternates between two areas of its spriteram */

	// This gave back to front frames causing bad flicker... but
	// reversing it now only gives us sprite updates on alternate
	// frames. So we probably have to partly buffer spriteram?

	start_offs = (sci_spriteframe &1) * 0x800;
	start_offs = 0x800 - start_offs;

	for (offs = (start_offs + 0x800 - 4);offs >= start_offs;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			j = sprite_chunk / 4;   /* 8 rows */
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

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

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void aquajack_draw_sprites_16x8(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+3];
		flipy = (data & 0x8000) >> 15;	// ???
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

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

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void spacegun_draw_sprites_16x8(struct mame_bitmap *bitmap,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0; offs < spriteram_size/2-4;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+3];
		flipy = (data & 0x8000) >> 15;	// ???
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

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

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}


/**************************************************************
                        SCREEN REFRESH
**************************************************************/

void contcirc_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,-6,0xc0,1,0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	contcirc_draw_sprites_16x8(bitmap,7);
}


/* Nightstr and ChaseHQ */

void chasehq_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,-1,0xc0,0,0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	chasehq_draw_sprites_16x16(bitmap,7);
}


void bshark_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,-1,0xc0,0,1,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	bshark_draw_sprites_16x8(bitmap,8);
}


void sci_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,-1,0xc0,0,0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	sci_draw_sprites_16x8(bitmap,6);
}


void aquajack_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,-1,0,1,1,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	aquajack_draw_sprites_16x8(bitmap,3);
}


void spacegun_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	spacegun_draw_sprites_16x8(bitmap,4);

	/* See if we should draw artificial gun targets */

	if (input_port_9_word_r(0,0) &0x1)	/* Fake DSW */
	{
		int rawx, rawy, centrex, centrey, screenx, screeny;

		/* A lag of one frame can be seen with the scope pickup and on the test
		   screen, however there is conflicting evidence so this isn't emulated
		   During high score entry the scope is displayed slightly offset
		   from the crosshair, close inspection shows the crosshair location
		   being used to target letters. */

		/* calculate p1 screen co-ords by matching routine at $195D4 */
		rawx = taitoz_sharedram[0xd94/2];
		centrex = taitoz_sharedram[0x26/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x2e/2] +
				(((rawx * taitoz_sharedram[0x30/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x8/2]) rawx = taitoz_sharedram[0x8/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x36/2] +
				(((rawx * taitoz_sharedram[0x38/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd96/2];
		centrey = taitoz_sharedram[0x28/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x32/2] +
				(((rawy * taitoz_sharedram[0x34/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x10/2]) rawy = taitoz_sharedram[0x10/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x3a/2] +
				(((rawy * taitoz_sharedram[0x3c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317540, $317542 */
		--screenx;
		screeny += 15;

		draw_crosshair(bitmap,screenx,screeny,&Machine->visible_area);

		/* calculate p2 screen co-ords by matching routine at $196EA */
		rawx = taitoz_sharedram[0xd98/2];
		centrex = taitoz_sharedram[0x2a/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x3e/2] +
				(((rawx * taitoz_sharedram[0x40/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x18/2]) rawx = taitoz_sharedram[0x18/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x46/2] +
				(((rawx * taitoz_sharedram[0x48/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd9a/2];
		centrey = taitoz_sharedram[0x2c/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x42/2] +
				(((rawy * taitoz_sharedram[0x44/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x20/2]) rawy = taitoz_sharedram[0x20/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x4a/2] +
				(((rawy * taitoz_sharedram[0x4c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317544, $317546 */
		--screenx;
		screeny += 15;

		draw_crosshair(bitmap,screenx,screeny,&Machine->visible_area);
	}
}


void dblaxle_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[5];
	UINT16 priority;

	TC0480SCP_tilemap_update();

	priority = TC0480SCP_get_bg_priority();

	layer[0] = (priority &0xf000) >> 12;	/* tells us which bg layer is bottom */
	layer[1] = (priority &0x0f00) >>  8;
	layer[2] = (priority &0x00f0) >>  4;
	layer[3] = (priority &0x000f) >>  0;	/* tells us which is top */
	layer[4] = 4;   /* text layer always over bg layers */

	fillbitmap(priority_bitmap,0,NULL);

	/* Ensure screen blanked - this shouldn't be necessary! */
	fillbitmap(bitmap, Machine->pens[0], &Machine -> visible_area);

	TC0480SCP_tilemap_draw(bitmap,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0480SCP_tilemap_draw(bitmap,layer[1],0,0);
	TC0480SCP_tilemap_draw(bitmap,layer[2],0,1);

	TC0150ROD_draw(bitmap,-1,0xc0,0,0,2);
	bshark_draw_sprites_16x8(bitmap,7);

	/* This layer used for the big numeric displays */
	TC0480SCP_tilemap_draw(bitmap,layer[3],0,4);

	TC0480SCP_tilemap_draw(bitmap,layer[4],0,0);	/* Text layer */
}


