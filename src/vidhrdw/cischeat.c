/***************************************************************************

				Cisco Heat & F1 GrandPrix Star, Scud Hammer
						(c) 1990 & 1991, 1994 Jaleco


				    driver by Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q,W,E		shows scroll 0,1,2
		R,T			shows road 0,1
		A,S,D,F		shows sprites with priority 0,1,2,3
		M			disables sprites zooming
		U			toggles the display of some hardware registers'
					values ($80000/2/4/6)

		Keys can be used togheter!
		Additionally, layers can be disabled, writing to $82400
		(fake video register):

			---- ---- --54 ----		Enable Road 1,0
			---- ---- ---- 3---		Enable Sprites
			---- ---- ---- -210		Enable Scroll 2,1,0

		0 is the same as 0x3f


----------------------------------------------------------------------
Video Section Summary		[ Cisco Heat ]			[ F1 GP Star ]
----------------------------------------------------------------------

[ 3 Scrolling Layers ]
	see Megasys1.c

		Tile Format:

				Colour		fedc b--- ---- ----		fedc ---- ---- ----
				Code		---- -a98 7654 3210		---- ba98 7654 3210

		Layer Size:			May be different from Megasys1?

[ 2 Road Layers ]
	Each of the 256 (not all visible) lines of the screen
	can display any of the lines of gfx in ROM, which are
	larger than the sceen and can therefore be scrolled

				Line Width			1024					1024
				Zoom				No						Yes

[ 256 Sprites ]
	Each Sprite is up to 256x256 pixels (!) and can be zoomed in and out.
	See below.


***************************************************************************/

#include "driver.h"
#include "drivers/megasys1.h"
#include "vidhrdw/generic.h"

/* Variables only used here: */
static struct tilemap *cischeat_tmap[3];

static int cischeat_ip_select;
#ifdef MAME_DEBUG
static int debugsprites;	// For debug purposes
#endif

/* Variables that driver has access to: */
data16_t *cischeat_roadram[2];

/* Variables defined in driver: */


#ifdef MAME_DEBUG
#define SHOW_READ_ERROR(_format_,_offset_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_);\
	usrintf_showmessage(buf);\
	logerror("CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf); \
}

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_,_data_);\
	usrintf_showmessage(buf);\
	logerror("CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf); \
}

#else

#define SHOW_READ_ERROR(_format_,_offset_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_);\
	logerror("CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf);\
}

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_,_data_); \
	logerror("CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf); \
}

#endif

#define MEGASYS1_VREG_FLAG(_n_) \
		megasys1_scroll_##_n_##_flag_w(new_data); \
		if (cischeat_tmap[_n_] == 0) SHOW_WRITE_ERROR("vreg %04X <- %04X NO MEMORY FOR SCREEN",offset*2,data);

#define MEGASYS1_VREG_SCROLL(_n_, _dir_)	megasys1_scroll##_dir_[_n_] = new_data;


#define cischeat_tmap_SET_SCROLL(_n_) \
	if (cischeat_tmap[_n_]) \
	{ \
		tilemap_set_scrollx(cischeat_tmap[_n_], 0, megasys1_scrollx[_n_]); \
		tilemap_set_scrolly(cischeat_tmap[_n_], 0, megasys1_scrolly[_n_]); \
	}

#define cischeat_tmap_UPDATE(_n_) \
	if ( (cischeat_tmap[_n_]) && (megasys1_active_layers & (1 << _n_) ) ) \
		tilemap_update(cischeat_tmap[_n_]);


#define cischeat_tmap_DRAW(_n_) \
	if ( (cischeat_tmap[_n_]) && (megasys1_active_layers & (1 << _n_) ) ) \
	{ \
		tilemap_draw(bitmap, cischeat_tmap[_n_], flag, 0 ); \
		flag = 0; \
	}

/***************************************************************************

								vh_start

***************************************************************************/

/**************************************************************************
								[ Cisco Heat ]
**************************************************************************/

/* 32 colour codes for the tiles */
int cischeat_vh_start(void)
{
	if (megasys1_vh_start() == 0)
	{
	 	megasys1_bits_per_color_code = 5;
	 	return 0;
	}
	else	return 1;
}



/**************************************************************************
							[ F1 GrandPrix Star ]
**************************************************************************/

/* 16 colour codes for the tiles */
int f1gpstar_vh_start(void)
{
	if (megasys1_vh_start() == 0)
	{
	 	megasys1_bits_per_color_code = 4;
	 	return 0;
	}
	else	return 1;
}



/***************************************************************************

						Hardware registers access

***************************************************************************/

/*	This function returns the status of the shift (ACTIVE_LOW):

		1 - low  shift
		0 - high shift

	and allows the shift to be handled using two buttons */

static int read_shift(void)
{
	static int ret = 1; /* start with low shift */
	switch ( (readinputport(0) >> 2) & 3 )
	{
		case 1 : ret = 1;	break;	// low  shift: button 3
		case 2 : ret = 0;	break;	// high shift: button 4
	}
	return ret;
}


/*
	F1 GP Star has a real pedal, while Cisco Heat's is connected to
	a switch. The Former game stores, during boot, the value that
	corresponds to the pedal not pressed, and compares against it:

	The value returned must decrease when the pedal is pressed.
	We support just 2 values for now..
*/

static int read_accelerator(void)
{
	if (readinputport(0) & 1)	return 0x00;	// pedal pressed
	else						return 0xff;
}

/**************************************************************************
								[ Cisco Heat ]
**************************************************************************/

READ16_HANDLER( cischeat_vregs_r )
{
	switch (offset)
	{
		case 0x0000/2 : return readinputport(1);	// Coins
		case 0x0002/2 : return readinputport(2) +
						(read_shift()<<1);			// Buttons
		case 0x0004/2 : return readinputport(3);	// Motor Limit Switches
		case 0x0006/2 : return readinputport(4);	// DSW 1 & 2

		case 0x0010/2 :
			switch (cischeat_ip_select & 0x3)
			{
				case 0 : return readinputport(6);	// Driving Wheel
				case 1 : return ~0;					// Cockpit: Up / Down Position?
				case 2 : return ~0;					// Cockpit: Left / Right Position?
				default: return ~0;
			}

		case 0x2200/2 : return readinputport(5);	// DSW 3 (4 bits)
		case 0x2300/2 : return soundlatch2_r(0);	// From sound cpu

		default:	SHOW_READ_ERROR("vreg %04X read!",offset*2);
					return megasys1_vregs[offset];
	}
}

/**************************************************************************
							[ F1 GrandPrix Star ]
**************************************************************************/


READ16_HANDLER( f1gpstar_vregs_r )
{

	switch (offset)
	{
		case 0x0000/2 :	// DSW 1&2: coinage changes with Country
		{
			int val = readinputport(1);
			if (val & 0x0200)	return readinputport(6) | val; 	// JP, US
			else				return readinputport(7) | val; 	// UK, FR
		}

//		case 0x0002/2 :	return 0xFFFF;
		case 0x0004/2 :	return readinputport(2) +
						       (read_shift()<<5);	// Buttons

		case 0x0006/2 :	return readinputport(3);	// ? Read at boot only
		case 0x0008/2 :	return soundlatch2_r(0);	// From sound cpu

		case 0x000c/2 :	return readinputport(4);	// DSW 3

		case 0x0010/2 :	// Accel + Driving Wheel
			return (read_accelerator()&0xff) + ((readinputport(5)&0xff)<<8);

		default:		SHOW_READ_ERROR("vreg %04X read!",offset*2);
						return megasys1_vregs[offset];
	}
}

/**************************************************************************
								[ Cisco Heat ]
**************************************************************************/

WRITE16_HANDLER( cischeat_vregs_w )
{
	data16_t old_data = megasys1_vregs[offset];
	data16_t new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
 		case 0x0000/2   :	// leds
			if (ACCESSING_LSB)
			{
	 			set_led_status(0,new_data & 0x10);
				set_led_status(1,new_data & 0x20);
			}
			break;

		case 0x0002/2   :	// ?? 91/1/91/1 ...
			break;

 		case 0x0004/2   :	// motor (seat?)
			if (ACCESSING_LSB)
				set_led_status(2, (new_data != old_data) ? 1 : 0);
 			break;

 		case 0x0006/2   :	// motor (wheel?)
			break;

		case 0x0010/2   : cischeat_ip_select = new_data;	break;
		case 0x0012/2   : break; // value above + 1

		case 0x2000/2+0 : MEGASYS1_VREG_SCROLL(0,x)		break;
		case 0x2000/2+1 : MEGASYS1_VREG_SCROLL(0,y)		break;
		case 0x2000/2+2 : MEGASYS1_VREG_FLAG(0)			break;

		case 0x2008/2+0 : MEGASYS1_VREG_SCROLL(1,x)		break;
		case 0x2008/2+1 : MEGASYS1_VREG_SCROLL(1,y)		break;
		case 0x2008/2+2 : MEGASYS1_VREG_FLAG(1)			break;

		case 0x2100/2+0 : MEGASYS1_VREG_SCROLL(2,x)		break;
		case 0x2100/2+1 : MEGASYS1_VREG_SCROLL(2,y)		break;
		case 0x2100/2+2 : MEGASYS1_VREG_FLAG(2)			break;

		case 0x2108/2   : break;	// ? written with 0 only
		case 0x2208/2   : break;	// watchdog reset

		case 0x2300/2   :	/* Sound CPU: reads latch during int 4, and stores command */
							soundlatch_word_w(0,new_data,0);
							cpu_cause_interrupt(3,4);
							break;

		/* Not sure about this one.. */
		case 0x2308/2   :	cpu_set_reset_line(1, (new_data & 2) ? ASSERT_LINE : CLEAR_LINE );
							cpu_set_reset_line(2, (new_data & 2) ? ASSERT_LINE : CLEAR_LINE );
							cpu_set_reset_line(3, (new_data & 1) ? ASSERT_LINE : CLEAR_LINE );
							break;

		default: SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}
}


/**************************************************************************
							[ F1 GrandPrix Star ]
**************************************************************************/

WRITE16_HANDLER( f1gpstar_vregs_w )
{
//	data16_t old_data = megasys1_vregs[offset];
	data16_t new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
/*
CPU #0 PC 00234A : Warning, vreg 0000 <- 0000
CPU #0 PC 002350 : Warning, vreg 0002 <- 0000
CPU #0 PC 00235C : Warning, vreg 0006 <- 0000
*/
		// "shudder" motors, leds
		case 0x0004/2   :
		case 0x0014/2   :
			if (ACCESSING_LSB)
			{
				set_led_status(0,new_data & 0x04);	// 1p start lamp
				set_led_status(1,new_data & 0x20);	// 2p start lamp?
				// wheel | seat motor
				set_led_status(2, ((new_data >> 3) | (new_data >> 4)) & 1 );
			}
			break;

		/* Usually written in sequence, but not always */
		case 0x0008/2   :	soundlatch_word_w(0,new_data,0);	break;
		case 0x0018/2   :	cpu_cause_interrupt(3,4);		break;

		case 0x0010/2   :	break;

		case 0x2000/2+0 : MEGASYS1_VREG_SCROLL(0,x)		break;
		case 0x2000/2+1 : MEGASYS1_VREG_SCROLL(0,y)		break;
		case 0x2000/2+2 : MEGASYS1_VREG_FLAG(0)			break;

		case 0x2008/2+0 : MEGASYS1_VREG_SCROLL(1,x)		break;
		case 0x2008/2+1 : MEGASYS1_VREG_SCROLL(1,y)		break;
		case 0x2008/2+2 : MEGASYS1_VREG_FLAG(1)			break;

		case 0x2100/2+0 : MEGASYS1_VREG_SCROLL(2,x)		break;
		case 0x2100/2+1 : MEGASYS1_VREG_SCROLL(2,y)		break;
		case 0x2100/2+2 : MEGASYS1_VREG_FLAG(2)			break;

		case 0x2108/2   : break;	// ? written with 0 only
		case 0x2208/2   : break;	// watchdog reset

		/* Not sure about this one. Values: $10 then 0, $7 then 0 */
		case 0x2308/2   :	cpu_set_reset_line(1, (new_data & 1) ? ASSERT_LINE : CLEAR_LINE );
							cpu_set_reset_line(2, (new_data & 2) ? ASSERT_LINE : CLEAR_LINE );
							cpu_set_reset_line(3, (new_data & 4) ? ASSERT_LINE : CLEAR_LINE );
							break;

		default:		SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}
}


/**************************************************************************
								[ Scud Hammer ]
**************************************************************************/

WRITE16_HANDLER( scudhamm_vregs_w )
{
//	int old_data = megasys1_vregs[offset];
	int new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x000/2+0 : MEGASYS1_VREG_SCROLL(0,x)		break;
		case 0x000/2+1 : MEGASYS1_VREG_SCROLL(0,y)		break;
		case 0x000/2+2 : MEGASYS1_VREG_FLAG(0)			break;

// 		UNUSED LAYER
//		case 0x008/2+0 : MEGASYS1_VREG_SCROLL(1,x)		break;
//		case 0x008/2+1 : MEGASYS1_VREG_SCROLL(1,y)		break;
//		case 0x008/2+2 : MEGASYS1_VREG_FLAG(1)			break;

		case 0x100/2+0 : MEGASYS1_VREG_SCROLL(2,x)		break;
		case 0x100/2+1 : MEGASYS1_VREG_SCROLL(2,y)		break;
		case 0x100/2+2 : MEGASYS1_VREG_FLAG(2)			break;

		case 0x208/2   : watchdog_reset_w(0,0);	break;

		default: SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}
}


/***************************************************************************
								Road Drawing
***************************************************************************/

#define ROAD_COLOR_CODES	64
#define ROAD_COLOR(_x_)	((_x_) & (ROAD_COLOR_CODES-1))

/* horizontal size of 1 line and 1 tile of the road */
#define X_SIZE (1024)
#define TILE_SIZE (64)


/**************************************************************************
								[ Cisco Heat ]
**************************************************************************/

/*
	Road format:

	Offset		Data

	00.w		Code

	02.w		X Scroll

	04.w		fedc ---- ---- ----		unused?
				---- ba98 ---- ----		Priority
				---- ---- 76-- ----		unused?
				---- ---- --54 3210		Color

	06.w		Unused

*/

/*	Draw the road in the given bitmap. The priority1 and priority2 parameters
	specify the range of lines to draw	*/

void cischeat_draw_road(struct osd_bitmap *bitmap, int road_num, int priority1, int priority2, int transparency)
{
	int curr_code,sx,sy;
	int min_priority, max_priority;

	struct rectangle rect		=	Machine->visible_area;
	struct GfxElement *gfx		=	Machine->gfx[(road_num & 1)?5:4];

	data16_t *roadram			=	cischeat_roadram[road_num & 1];

	int min_y = rect.min_y;
	int max_y = rect.max_y;

	int max_x = rect.max_x;

	if (priority1 < priority2)	{	min_priority = priority1;	max_priority = priority2; }
	else						{	min_priority = priority2;	max_priority = priority1; }

	/* Move the priority values in place */
	min_priority = (min_priority & 7) * 0x100;
	max_priority = (max_priority & 7) * 0x100;

	/* Let's draw from the top to the bottom of the visible screen */
	for (sy = min_y ; sy <= max_y ; sy ++)
	{
		int code	= roadram[ sy * 4 + 0 ];
		int xscroll = roadram[ sy * 4 + 1 ];
		int attr	= roadram[ sy * 4 + 2 ];

		/* high byte is a priority information */
		if ( ((attr & 0x700) < min_priority) || ((attr & 0x700) > max_priority) )
			continue;

		/* line number converted to tile number (each tile is TILE_SIZE x 1) */
		code = code * (X_SIZE/TILE_SIZE);

		xscroll %= X_SIZE;
		curr_code = code + xscroll/TILE_SIZE;

		for (sx = -(xscroll%TILE_SIZE) ; sx <= max_x ; sx +=TILE_SIZE)
		{
			drawgfx(bitmap,gfx,
					curr_code++,
					ROAD_COLOR(attr),
					0,0,
					sx,sy,
					&rect,
					transparency,15);

			/* wrap around */
			if (curr_code%(X_SIZE/TILE_SIZE)==0)	curr_code = code;
		}
	}

}

void cischeat_mark_road_colors(int road_num)
{
	int i, color, colmask[ROAD_COLOR_CODES], sy;

	int gfx_num					=	(road_num & 1) ? 5 : 4;
	struct GfxDecodeInfo gfx	=	Machine->drv->gfxdecodeinfo[gfx_num];
	int total_elements			=	Machine->gfx[gfx_num]->total_elements;
	unsigned int *pen_usage		=	Machine->gfx[gfx_num]->pen_usage;
//	int total_color_codes		=	gfx.total_color_codes;
	int color_codes_start		=	gfx.color_codes_start;

	data16_t *roadram			=	cischeat_roadram[road_num & 1];

	int min_y = Machine->visible_area. min_y;
	int max_y = Machine->visible_area. max_y;

	for (color = 0 ; color < ROAD_COLOR_CODES ; color++) colmask[color] = 0;

	/* Let's walk from the top to the bottom of the visible screen */
	for (sy = min_y ; sy <= max_y ; sy ++)
	{
		int code	= roadram[sy * 4 + 0];
		int attr	= roadram[sy * 4 + 2];

		color = ROAD_COLOR(attr);

		/* line number converted to tile number (each tile is TILE_SIZE x 1) */
		code = code * (X_SIZE/TILE_SIZE);

		for (i = 0; i < (X_SIZE/TILE_SIZE); i++)
			colmask[color] |= pen_usage[(code + i) % total_elements];
	}

	for (color = 0; color < ROAD_COLOR_CODES; color++)
	 for (i = 0; i < 16; i++)
	  if (colmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
}



/**************************************************************************
							[ F1 GrandPrix Star ]
**************************************************************************/


/*
	Road format:

	Offset		Data

	00.w		fedc ---- ---- ----		Priority
				---- ba98 7654 3210		X Scroll (After Zoom)

	02.w		fedc ba-- ---- ----		unused?
				---- --98 7654 3210		X Zoom

	04.w		fe-- ---- ---- ----		unused?
				--dc ba98 ---- ----		Color
				---- ---- 7654 3210		?

	06.w		Code


	Imagine an "empty" line, 2 * X_SIZE wide, with the gfx from
	the ROM - whose original size is X_SIZE - in the middle.

	Zooming acts on this latter and can shrink it to 1 pixel or
	widen it to 2 * X_SIZE, while *keeping it centered* in the
	empty line.	Scrolling acts on the resulting line.

*/


/*	Draw the road in the given bitmap. The priority1 and priority2 parameters
	specify the range of lines to draw	*/

void f1gpstar_draw_road(struct osd_bitmap *bitmap, int road_num, int priority1, int priority2, int transparency)
{
	int sx,sy;
	int xstart;
	int min_priority, max_priority;

	struct rectangle rect		=	Machine->visible_area;
	struct GfxElement *gfx		=	Machine->gfx[(road_num & 1)?5:4];

	data16_t *roadram			=	cischeat_roadram[road_num & 1];

	int min_y = rect.min_y;
	int max_y = rect.max_y;

	int max_x = rect.max_x << 16;	// use fixed point values (16.16), for accuracy

	if (priority1 < priority2)	{	min_priority = priority1;	max_priority = priority2; }
	else						{	min_priority = priority2;	max_priority = priority1; }

	/* Move the priority values in place */
	min_priority = (min_priority & 7) * 0x1000;
	max_priority = (max_priority & 7) * 0x1000;

	/* Let's draw from the top to the bottom of the visible screen */
	for (sy = min_y ; sy <= max_y ; sy ++)
	{
		int xscale, xdim;

		int xscroll	= roadram[ sy * 4 + 0 ];
		int xzoom	= roadram[ sy * 4 + 1 ];
		int attr	= roadram[ sy * 4 + 2 ];
		int code	= roadram[ sy * 4 + 3 ];

		/* highest nibble is a priority information */
		if ( ((xscroll & 0x7000) < min_priority) || ((xscroll & 0x7000) > max_priority) )
			continue;

		/* zoom code range: 000-3ff		scale range: 0.0-2.0 */
		xscale = ( ((xzoom & 0x3ff)+1) << (16+1) ) / 0x400;

		/* line number converted to tile number (each tile is TILE_SIZE x 1) */
		code	= code * (X_SIZE/TILE_SIZE);

		/* dimension of a tile after zoom */
		xdim = TILE_SIZE * xscale;

		xscroll %= 2 * X_SIZE;

		xstart  = (X_SIZE - xscroll) * 0x10000;
		xstart -= (X_SIZE * xscale) / 2;

		/* let's approximate to the nearest greater integer value
		   to avoid holes in between tiles */
		xscale += (1<<16)/TILE_SIZE;

		/* Draw the line */
		for (sx = xstart ; sx <= max_x ; sx += xdim)
		{
			drawgfxzoom(bitmap,gfx,
						code++,
						ROAD_COLOR(attr>>8),
						0,0,
						sx / 0x10000, sy,
						&rect,
						transparency,15,
						xscale, 1 << 16);

			/* stop when the end of the line of gfx is reached */
			if ((code % (X_SIZE/TILE_SIZE)) == 0)	break;
		}

	}

}


void f1gpstar_mark_road_colors(int road_num)
{
	int i, color, colmask[ROAD_COLOR_CODES], sy;

	int gfx_num					=	(road_num & 1) ? 5 : 4;
	struct GfxDecodeInfo gfx	=	Machine->drv->gfxdecodeinfo[gfx_num];
	int total_elements			=	Machine->gfx[gfx_num]->total_elements;
	unsigned int *pen_usage		=	Machine->gfx[gfx_num]->pen_usage;
//	int total_color_codes		=	gfx.total_color_codes;
	int color_codes_start		=	gfx.color_codes_start;

	data16_t *roadram			=	cischeat_roadram[road_num & 1];

	int min_y = Machine->visible_area.min_y;
	int max_y = Machine->visible_area.max_y;

	for (color = 0 ; color < ROAD_COLOR_CODES ; color++) colmask[color] = 0;

	/* Let's walk from the top to the bottom of the visible screen */
	for (sy = min_y ; sy <= max_y ; sy ++)
	{
		int attr	=	roadram[ sy * 4 + 2 ];
		int code	=	roadram[ sy * 4 + 3 ];

		color	= ROAD_COLOR(attr>>8);

		/* line number converted to tile number (each tile is TILE_SIZE x 1) */
		code	= code * (X_SIZE/TILE_SIZE);

		for (i = 0; i < (X_SIZE/TILE_SIZE); i++)
			colmask[color] |= pen_usage[(code + i) % total_elements];
	}

	for (color = 0; color < ROAD_COLOR_CODES; color++)
	 for (i = 0; i < 16; i++)
	  if (colmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
}





/***************************************************************************

	Sprite Data:

	Offset		Data

	00 		fed- ---- ---- ----		unused?
	 		---c ---- ---- ----		Don't display this sprite
			---- ba98 ---- ----		unused?
			---- ---- 7654 ----		Number of tiles along Y, minus 1 (1-16)
			---- ---- ---- 3210		Number of tiles along X, minus 1 (1-16)

	02/04 	fed- ---- ---- ----		unused?
	 		---c ---- ---- ----		Flip X/Y
			---- ba9- ---- ----		? X/Y zoom ?
			---- ---8 7654 3210		X/Y zoom

	06/08	fedc ba-- ---- ----		? X/Y position ?
			---- --98 7654 3210		X/Y position

	0A		0 ?

	0C		Code

	0E
			fed- ---- ---- ----		unused?
			---c ---- ---- ----		used!
			---- ba98 ---- ----		Priority
			---- ---- 7--- ----		unused?
			---- ---- -654 3210		Color

***************************************************************************/

#define SIGN_EXTEND_POS(_var_)	{_var_ &= 0x3ff; if (_var_ > 0x1ff) _var_ -= 0x400;}
#define SPRITE_COLOR_CODES	(0x80)
#define SPRITE_COLOR(_x_)	((_x_) & (SPRITE_COLOR_CODES - 1))
#define SHRINK(_org_,_fact_) ( ( ( (_org_) << 16 ) * (_fact_ & 0x01ff) ) / 0x80 )

/*	Draw sprites, in the given priority range, to a bitmap.

	Priorities between 0 and 15 cover sprites whose priority nibble
	is between 0 and 15. Priorities between	0+16 and 15+16 cover
	sprites whose priority nibble is between 0 and 15 and whose
	colour code's high bit is set.	*/

static void cischeat_draw_sprites(struct osd_bitmap *bitmap , int priority1, int priority2)
{
	int x, y, sx, sy;
	int xzoom, yzoom, xscale, yscale, flipx, flipy;
	int xdim, ydim, xnum, ynum;
	int xstart, ystart, xend, yend, xinc, yinc;
	int code, attr, color, size;

	int min_priority, max_priority, high_sprites;

	data16_t		*source	=	spriteram16;
	const data16_t	*finish	=	source + 0x1000/2;


	/* Move the priority values in place */
	high_sprites = (priority1 >= 16) | (priority2 >= 16);
	priority1 = (priority1 & 0x0f) * 0x100;
	priority2 = (priority2 & 0x0f) * 0x100;

	if (priority1 < priority2)	{	min_priority = priority1;	max_priority = priority2; }
	else						{	min_priority = priority2;	max_priority = priority1; }

	for (; source < finish; source += 0x10/2 )
	{
		size	=	source[ 0 ];
		if (size & 0x1000)	continue;

		/* number of tiles */
		xnum	=	( (size & 0x0f) >> 0 ) + 1;
		ynum	=	( (size & 0xf0) >> 4 ) + 1;

		xzoom	=	source[ 1 ];
		yzoom	=	source[ 2 ];
		flipx	=	xzoom & 0x1000;
		flipy	=	yzoom & 0x1000;

		sx		=	source[ 3 ];
		sy		=	source[ 4 ];
		SIGN_EXTEND_POS(sx)
		SIGN_EXTEND_POS(sy)

		/* use fixed point values (16.16), for accuracy */
		sx <<= 16;
		sy <<= 16;

		/* dimension of a tile after zoom */
#ifdef MAME_DEBUG
		if ( keyboard_pressed(KEYCODE_Z) && keyboard_pressed(KEYCODE_M) )
		{
			xdim	=	16 << 16;
			ydim	=	16 << 16;
		}
		else
#endif
		{
			xdim	=	SHRINK(16,xzoom);
			ydim	=	SHRINK(16,yzoom);
		}

		if ( ( (xdim / 0x10000) == 0 ) || ( (ydim / 0x10000) == 0) )	continue;

		/* the y pos passed to the hardware is the that of the last line,
		   we need the y pos of the first line  */
		sy -= (ydim * ynum);

		code	=	source[ 6 ];
		attr	=	source[ 7 ];
		color	=	SPRITE_COLOR(attr);

		/* high byte is a priority information */
		if ( ((attr & 0x700) < min_priority) || ((attr & 0x700) > max_priority) )
			continue;

		if ( high_sprites && (!(color & (SPRITE_COLOR_CODES/2))) )
			continue;

#ifdef MAME_DEBUG
if ( (debugsprites) && ( ((attr & 0x0300)>>8) != (debugsprites-1) ) ) 	{ continue; };
#endif

		xscale = xdim / 16;
		yscale = ydim / 16;


		/* let's approximate to the nearest greater integer value
		   to avoid holes in between tiles */
		if (xscale & 0xffff)	xscale += (1<<16)/16;
		if (yscale & 0xffff)	yscale += (1<<16)/16;


		if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1; }
		else		{ xstart = 0;       xend = xnum;  xinc = +1; }

		if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1; }
		else		{ ystart = 0;       yend = ynum;  yinc = +1; }

		for (y = ystart; y != yend; y += yinc)
		{
			for (x = xstart; x != xend; x += xinc)
			{
				drawgfxzoom(bitmap,Machine->gfx[3],
							code++,
							color,
							flipx,flipy,
							(sx + x * xdim) / 0x10000, (sy + y * ydim) / 0x10000,
							&Machine->visible_area,
							TRANSPARENCY_PEN,15,
							xscale, yscale );
			}
		}
	}	/* end sprite loop */
}




/* Mark colors used by visible sprites */

static void cischeat_mark_sprite_colors(void)
{
	int i, color, colmask[SPRITE_COLOR_CODES];
	unsigned int *pen_usage	=	Machine->gfx[3]->pen_usage;
	int total_elements		=	Machine->gfx[3]->total_elements;
	int color_codes_start	=	Machine->drv->gfxdecodeinfo[3].color_codes_start;

	int xmin = Machine->visible_area.min_x;
	int xmax = Machine->visible_area.max_x;
	int ymin = Machine->visible_area.min_y;
	int ymax = Machine->visible_area.max_y;

	data16_t		*source	=	spriteram16;
	const data16_t	*finish	=	source + 0x1000/2;

	for (color = 0 ; color < SPRITE_COLOR_CODES ; color++) colmask[color] = 0;

	for (; source < finish; source += 0x10/2 )
	{
		int sx, sy, xzoom, yzoom;
		int xdim, ydim, xnum, ynum;
		int code, attr, size;

 		size	=	source[ 0 ];
		if (size & 0x1000)	continue;

		/* number of tiles */
		xnum	=	( (size & 0x0f) >> 0 ) + 1;
		ynum	=	( (size & 0xf0) >> 4 ) + 1;

		xzoom	=	source[ 1 ];
		yzoom	=	source[ 2 ];

		sx		=	source[ 3 ];
		sy		=	source[ 4 ];
		SIGN_EXTEND_POS(sx)
		SIGN_EXTEND_POS(sy)

		/* dimension of the sprite after zoom */
		xdim	=	( xnum * SHRINK(16,xzoom) ) >> 16;
		ydim	=	( ynum * SHRINK(16,yzoom) ) >> 16;

		/* the y pos passed to the hardware is the that of the last line,
		   we need the y pos of the first line  */
		sy -= ydim;

		if (	((sx+xdim-1) < xmin) || (sx > xmax) ||
				((sy+ydim-1) < ymin) || (sy > ymax)		)	continue;

		code	=	source[ 6 ];
		attr	=	source[ 7 ];
		color	=	SPRITE_COLOR(attr);

		for (i = 0; i < xnum * ynum; i++)
			colmask[color] |= pen_usage[(code + i) % total_elements];
	}

	for (color = 0; color < SPRITE_COLOR_CODES; color++)
	 for (i = 0; i < (16-1); i++)	// pen 15 is transparent
	  if (colmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
}




/***************************************************************************

								Screen Drawing

***************************************************************************/

#define CISCHEAT_LAYERSCTRL \
debugsprites = 0; \
if (keyboard_pressed(KEYCODE_Z)) \
{ \
	int msk = 0; \
	if (keyboard_pressed(KEYCODE_Q))	{ msk |= 0xffc1;} \
	if (keyboard_pressed(KEYCODE_W))	{ msk |= 0xffc2;} \
	if (keyboard_pressed(KEYCODE_E))	{ msk |= 0xffc4;} \
	if (keyboard_pressed(KEYCODE_A))	{ msk |= 0xffc8; debugsprites = 1;} \
	if (keyboard_pressed(KEYCODE_S))	{ msk |= 0xffc8; debugsprites = 2;} \
	if (keyboard_pressed(KEYCODE_D))	{ msk |= 0xffc8; debugsprites = 3;} \
	if (keyboard_pressed(KEYCODE_F))	{ msk |= 0xffc8; debugsprites = 4;} \
	if (keyboard_pressed(KEYCODE_R))	{ msk |= 0xffd0;} \
	if (keyboard_pressed(KEYCODE_T))	{ msk |= 0xffe0;} \
 \
	if (msk != 0) megasys1_active_layers &= msk; \
} \
\
{ \
	static int show_unknown; \
	if ( keyboard_pressed(KEYCODE_Z) && keyboard_pressed_memory(KEYCODE_U) ) \
		show_unknown ^= 1; \
 \
	if (show_unknown) \
	{ \
		char buf[80]; \
		sprintf(buf, "0:%04X 2:%04X 4:%04X 6:%04X", \
					megasys1_vregs[0],megasys1_vregs[1], \
					megasys1_vregs[2],megasys1_vregs[3] ); \
		usrintf_showmessage(buf); \
	} \
}


/**************************************************************************
								[ Cisco Heat ]
**************************************************************************/

void cischeat_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int megasys1_active_layers1, flag;

#ifdef MAME_DEBUG
	/* FAKE Videoreg */
	megasys1_active_layers = megasys1_vregs[0x2400/2];
	if (megasys1_active_layers == 0)	megasys1_active_layers = 0x3f;
#else
	megasys1_active_layers = 0x3f;
#endif

	megasys1_active_layers1 = megasys1_active_layers;

#ifdef MAME_DEBUG
	CISCHEAT_LAYERSCTRL
#endif

	cischeat_tmap_SET_SCROLL(0)
	cischeat_tmap_SET_SCROLL(1)
	cischeat_tmap_SET_SCROLL(2)

	cischeat_tmap_UPDATE(0)
	cischeat_tmap_UPDATE(1)
	cischeat_tmap_UPDATE(2)

	palette_init_used_colors();

	if (megasys1_active_layers & 0x08)	cischeat_mark_sprite_colors();
	if (megasys1_active_layers & 0x10)	cischeat_mark_road_colors(0);	// road 0
	if (megasys1_active_layers & 0x20)	cischeat_mark_road_colors(1);	// road 1

	palette_recalc();

	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

										/* bitmap, road, priority, transparency */
	if (megasys1_active_layers & 0x10)	cischeat_draw_road(bitmap,0,7,5,TRANSPARENCY_NONE);
	if (megasys1_active_layers & 0x20)	cischeat_draw_road(bitmap,1,7,5,TRANSPARENCY_PEN);

	flag = 0;
	cischeat_tmap_DRAW(0)
//	else fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
	cischeat_tmap_DRAW(1)

	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,15,3);
	if (megasys1_active_layers & 0x10)	cischeat_draw_road(bitmap,0,4,1,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x20)	cischeat_draw_road(bitmap,1,4,1,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,2,2);
	if (megasys1_active_layers & 0x10)	cischeat_draw_road(bitmap,0,0,0,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x20)	cischeat_draw_road(bitmap,1,0,0,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,1,0);
	cischeat_tmap_DRAW(2)

	/* for the map screen */
	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,0+16,0+16);


	megasys1_active_layers = megasys1_active_layers1;
}





/**************************************************************************
							[ F1 GrandPrix Star ]
**************************************************************************/


void f1gpstar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int megasys1_active_layers1, flag;

#ifdef MAME_DEBUG
	/* FAKE Videoreg */
	megasys1_active_layers = megasys1_vregs[0x2400/2];
	if (megasys1_active_layers == 0)	megasys1_active_layers = 0x3f;
#else
	megasys1_active_layers = 0x3f;
#endif

	megasys1_active_layers1 = megasys1_active_layers;

#ifdef MAME_DEBUG
	CISCHEAT_LAYERSCTRL
#endif

	cischeat_tmap_SET_SCROLL(0)
	cischeat_tmap_SET_SCROLL(1)
	cischeat_tmap_SET_SCROLL(2)

	cischeat_tmap_UPDATE(0)
	cischeat_tmap_UPDATE(1)
	cischeat_tmap_UPDATE(2)

	palette_init_used_colors();

	if (megasys1_active_layers & 0x08)	cischeat_mark_sprite_colors();
	if (megasys1_active_layers & 0x10)	f1gpstar_mark_road_colors(0);	// road 0
	if (megasys1_active_layers & 0x20)	f1gpstar_mark_road_colors(1);	// road 1

	palette_recalc();

	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

/*	1: clouds 5, grad 7, road 0		2: clouds 5, grad 7, road 0, tunnel roof 0 */

	/* NOTE: TRANSPARENCY_NONE isn't supported by drawgfxzoom    */
	/* (the function used in f1gpstar_drawroad to draw the road) */

	/* road 1!! 0!! */					/* bitmap, road, min_priority, max_priority, transparency */
	if (megasys1_active_layers & 0x20)	f1gpstar_draw_road(bitmap,1,6,7,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x10)	f1gpstar_draw_road(bitmap,0,6,7,TRANSPARENCY_PEN);

	flag = 0;
	cischeat_tmap_DRAW(0)
//	else fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
	cischeat_tmap_DRAW(1)

	/* road 1!! 0!! */					/* bitmap, road, min_priority, max_priority, transparency */
	if (megasys1_active_layers & 0x20)	f1gpstar_draw_road(bitmap,1,1,5,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x10)	f1gpstar_draw_road(bitmap,0,1,5,TRANSPARENCY_PEN);

	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,15,2);

	/* road 1!! 0!! */					/* bitmap, road, min_priority, max_priority, transparency */
	if (megasys1_active_layers & 0x20)	f1gpstar_draw_road(bitmap,1,0,0,TRANSPARENCY_PEN);
	if (megasys1_active_layers & 0x10)	f1gpstar_draw_road(bitmap,0,0,0,TRANSPARENCY_PEN);

	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,1,1);
	cischeat_tmap_DRAW(2)
	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,0,0);


	megasys1_active_layers = megasys1_active_layers1;
}







/**************************************************************************
								[ Scud Hammer ]
**************************************************************************/


extern data16_t scudhamm_motor_command;

	READ16_HANDLER( scudhamm_motor_pos_r );
	READ16_HANDLER( scudhamm_motor_status_r );
	READ16_HANDLER( scudhamm_analog_r );

void scudhamm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int megasys1_active_layers1, flag;
	megasys1_active_layers = 0x0d;
	megasys1_active_layers1 = megasys1_active_layers;

#ifdef MAME_DEBUG
debugsprites = 0;
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	{ msk |= 0x1;}
	if (keyboard_pressed(KEYCODE_W))	{ msk |= 0x2;}
	if (keyboard_pressed(KEYCODE_E))	{ msk |= 0x4;}
	if (keyboard_pressed(KEYCODE_A))	{ msk |= 0x8; debugsprites = 1;}
	if (keyboard_pressed(KEYCODE_S))	{ msk |= 0x8; debugsprites = 2;}
	if (keyboard_pressed(KEYCODE_D))	{ msk |= 0x8; debugsprites = 3;}
	if (keyboard_pressed(KEYCODE_F))	{ msk |= 0x8; debugsprites = 4;}

	if (msk != 0) megasys1_active_layers &= msk;
#if 1
{	char buf[80];
	sprintf(buf, "Cmd: %04X Pos:%04X Lim:%04X Inp:%04X",
				scudhamm_motor_command,
				scudhamm_motor_pos_r(0,0),
				scudhamm_motor_status_r(0,0),
				scudhamm_analog_r(0,0) );
	usrintf_showmessage(buf);	}
#endif
}
#endif

	cischeat_tmap_SET_SCROLL(0)
//	cischeat_tmap_SET_SCROLL(1)
	cischeat_tmap_SET_SCROLL(2)

	cischeat_tmap_UPDATE(0)
//	cischeat_tmap_UPDATE(1)
	cischeat_tmap_UPDATE(2)

	palette_init_used_colors();

	if (megasys1_active_layers & 0x08)	cischeat_mark_sprite_colors();

	palette_recalc();

	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

	flag = 0;
	cischeat_tmap_DRAW(0)
//	else fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
//	cischeat_tmap_DRAW(1)
	if (megasys1_active_layers & 0x08)	cischeat_draw_sprites(bitmap,0,15);
	cischeat_tmap_DRAW(2)

	megasys1_active_layers = megasys1_active_layers1;
}
