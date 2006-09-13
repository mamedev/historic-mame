/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "8080bw.h"
#include <math.h>

static int screen_red;
static int screen_red_enabled;		/* 1 for games that can turn the screen red */
static int color_map_select;
static int background_color;
static int schaser_sx10_done;
static UINT8 cloud_pos;
static UINT8 bowler_bonus_display;

static write8_handler videoram_w_p;
static UINT32 (*video_update_p)(running_machine *machine,int screen,mame_bitmap *bitmap,const rectangle *cliprect);

static WRITE8_HANDLER( bw_videoram_w );
static WRITE8_HANDLER( rollingc_videoram_w );
static WRITE8_HANDLER( schaser_videoram_w );
static WRITE8_HANDLER( lupin3_videoram_w );
static WRITE8_HANDLER( polaris_videoram_w );
static WRITE8_HANDLER( sstrngr2_videoram_w );
static WRITE8_HANDLER( phantom2_videoram_w );
static WRITE8_HANDLER( invadpt2_videoram_w );
static WRITE8_HANDLER( cosmo_videoram_w );
static WRITE8_HANDLER( shuttlei_videoram_w );

static VIDEO_UPDATE( 8080bw_common );
static VIDEO_UPDATE( seawolf );
static VIDEO_UPDATE( blueshrk );
static VIDEO_UPDATE( desertgu );
static VIDEO_UPDATE( bowler );

static void plot_pixel_8080(int x, int y, int col);

/* smoothed colors, overlays are not so contrasted */

/*
#define OVERLAY_RED         MAKE_ARGB(0x08,0xff,0x20,0x20)
#define OVERLAY_GREEN       MAKE_ARGB(0x08,0x20,0xff,0x20)
#define OVERLAY_YELLOW      MAKE_ARGB(0x08,0xff,0xff,0x20)

OVERLAY_START( invdpt2m_overlay )
    OVERLAY_RECT(  16,   0,  72, 224, OVERLAY_GREEN )
    OVERLAY_RECT(   0,  16,  16, 134, OVERLAY_GREEN )
    OVERLAY_RECT(  72,   0, 192, 224, OVERLAY_YELLOW )
    OVERLAY_RECT( 192,   0, 224, 224, OVERLAY_RED )
OVERLAY_END
*/


DRIVER_INIT( 8080bw )
{
	videoram_w_p = bw_videoram_w;
	video_update_p = video_update_8080bw_common;
	screen_red = 0;
	screen_red_enabled = 0;
	color_map_select = 0;
	flip_screen_set(0);
}

DRIVER_INIT( invaddlx )
{
	init_8080bw(machine);
/*  artwork_set_overlay(invdpt2m_overlay);*/
}

DRIVER_INIT( sstrngr2 )
{
	init_8080bw(machine);
	videoram_w_p = sstrngr2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( schaser )
{
	int i;
	UINT8* promm = memory_region( REGION_PROMS );

	schaser_effect_555_timer = timer_alloc(schaser_effect_555_cb);

	init_8080bw(machine);
	videoram_w_p = schaser_videoram_w;
	// make background palette same as foreground one
	for (i = 0; i < 0x80; i++) promm[i] = 0;

	for (i = 0x80; i < 0x400; i++)
	{
		if (promm[i] == 0x0c) promm[i] = 4;
		if (promm[i] == 0x03) promm[i] = 2;
	}

	memcpy(promm+0x400,promm,0x400);

	for (i = 0x500; i < 0x800; i++)
		if (promm[i] == 4) promm[i] = 2;
}

DRIVER_INIT( schasrcv )
{
	init_8080bw(machine);
	videoram_w_p = rollingc_videoram_w;
	background_color = 2;	/* blue */
}

DRIVER_INIT( rollingc )
{
	init_8080bw(machine);
	videoram_w_p = rollingc_videoram_w;
	background_color = 0;	/* black */
}

DRIVER_INIT( polaris )
{
	init_8080bw(machine);
	videoram_w_p = polaris_videoram_w;
}

DRIVER_INIT( lupin3 )
{
	init_8080bw(machine);
	videoram_w_p = lupin3_videoram_w;
}

DRIVER_INIT( invadpt2 )
{
	init_8080bw(machine);
	videoram_w_p = invadpt2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( cosmo )
{
	init_8080bw(machine);
	videoram_w_p = cosmo_videoram_w;
}

DRIVER_INIT( seawolf )
{
	init_8080bw(machine);
	video_update_p = video_update_seawolf;
}

DRIVER_INIT( blueshrk )
{
	init_8080bw(machine);
	video_update_p = video_update_blueshrk;
}

DRIVER_INIT( desertgu )
{
	init_8080bw(machine);
	video_update_p = video_update_desertgu;
}

DRIVER_INIT( bowler )
{
	init_8080bw(machine);
	video_update_p = video_update_bowler;
}

DRIVER_INIT( phantom2 )
{
	init_8080bw(machine);
	videoram_w_p = phantom2_videoram_w;
}

DRIVER_INIT( indianbt )
{
	init_8080bw(machine);
	videoram_w_p = invadpt2_videoram_w;
}

DRIVER_INIT( shuttlei )
{
	init_8080bw(machine);
	videoram_w_p = shuttlei_videoram_w;
}

void c8080bw_flip_screen_w(int data)
{
	set_vh_global_attribute(&color_map_select, data);

	if (input_port_3_r(0) & 0x01)
	{
		flip_screen_set(data);
	}
}


void c8080bw_screen_red_w(int data)
{
	if (screen_red_enabled)
	{
		set_vh_global_attribute(&screen_red, data);
	}
}


INTERRUPT_GEN( polaris_interrupt )
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 8)	/* every 4 frames - this was verified against real machine */
	{
		cloud_speed = 0;

		cloud_pos++;

		set_vh_global_attribute(NULL,0);
	}

	c8080bw_interrupt();
}


INTERRUPT_GEN( phantom2_interrupt )
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 2)	/* every 2 frames - no idea of correct */
	{
		cloud_speed = 0;

		cloud_pos++;
		set_vh_global_attribute(NULL,0);
	}

	c8080bw_interrupt();
}


static void plot_pixel_8080(int x, int y, int col)
{
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}

	plot_pixel(tmpbitmap,x,y,Machine->pens[col]);
}

INLINE void plot_byte(int x, int y, int data, int fore_color, int back_color)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x, y, (data & 0x01) ? fore_color : back_color);

		x++;
		data >>= 1;
	}
}


WRITE8_HANDLER( c8080bw_videoram_w )
{
	videoram_w_p(offset, data);
}


static WRITE8_HANDLER( bw_videoram_w )
{
	int x,y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	plot_byte(x, y, data, 1, 0);
}

static WRITE8_HANDLER( rollingc_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE8_HANDLER( schaser_videoram_w )
{
	int x,y,z,proma,promb=0x400;	// promb = 0 for green band, promb = 400 for all blue
	UINT8 col,chg=0;
	UINT8* promm = memory_region( REGION_PROMS );
	if (schaser_sx10 != schaser_sx10_done) chg++;
	if (schaser_sx10) promb = 0;

	if (chg)
	{
		for (y = 64; y < 224; y++)
		{
			for (x = 40; x < 200; x+=8)
			{
				z = y*32+x/8;
				col = colorram[z & 0x1f1f] & 0x07;
				proma = promb+((z%32)*32+y/8+93);
				plot_byte(x, y, videoram[z], col, promm[proma&0x7ff]);
			}
		}
		schaser_sx10_done = schaser_sx10;
	}

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;
	proma = promb+((offset%32)*32+y/8+93);
	plot_byte(x, y, data, col, promm[proma&0x7ff]);
}

static WRITE8_HANDLER( lupin3_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = ~colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, 0);
}

static WRITE8_HANDLER( polaris_videoram_w )
{
	int x,i,col,back_color,fore_color,color_map;
	UINT8 y, cloud_y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* for the background color, bit 0 of the map PROM is connected to green gun.
       red is 0 and blue is 1, giving cyan and blue for the background.  This
       is different from what the schematics shows, but it's supported
       by screenshots. */

	color_map = memory_region(REGION_PROMS)[(y >> 3 << 5) | (x >> 3)];
	back_color = (color_map & 1) ? 6 : 2;
	fore_color = ~colorram[offset & 0x1f1f] & 0x07;

	/* bit 3 is connected to the cloud enable. bits 1 and 2 are marked 'not use' (sic)
       on the schematics */

	cloud_y = y - cloud_pos;

	if ((color_map & 0x08) || (cloud_y > 64))
	{
		plot_byte(x, y, data, fore_color, back_color);
	}
	else
	{
		/* cloud appears in this part of the screen */
		for (i = 0; i < 8; i++)
		{
			if (data & 0x01)
			{
				col = fore_color;
			}
			else
			{
				int bit;
				offs_t offs;

				col = back_color;

				bit = 1 << (~x & 0x03);
				offs = ((x >> 2) & 0x03) | ((~cloud_y & 0x3f) << 2);

				col = (memory_region(REGION_USER1)[offs] & bit) ? 7 : back_color;
			}

			plot_pixel_8080(x, y, col);

			x++;
			data >>= 1;
		}
	}
}


WRITE8_HANDLER( schaser_colorram_w )
{
	int i;


	offset &= 0x1f1f;

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i = 0; i < 8; i++, offset += 0x20)
	{
		videoram_w_p(offset, videoram[offset]);
	}
}

READ8_HANDLER( schaser_colorram_r )
{
	return colorram[offset & 0x1f1f];
}


static WRITE8_HANDLER( phantom2_videoram_w )
{
	static int CLOUD_SHIFT[] = { 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08,
	                             0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80 };

	int i,col;
	UINT8 x,y,cloud_x;
	UINT8 *cloud_region;
	offs_t cloud_offs;


	videoram[offset] = data;

	y = offset / 32;
	x = (offset % 32) * 8;


	cloud_region = memory_region(REGION_PROMS);
	cloud_offs = ((y - cloud_pos) & 0xff) >> 1 << 4;
	cloud_x = x - 12;  /* based on screen shots */


	for (i = 0; i < 8; i++)
	{
		if (data & 0x01)
		{
			col = 1;	/* white foreground */
		}
		else
		{
			UINT8 cloud_data;


			cloud_offs = (cloud_offs & 0xfff0) | (cloud_x >> 4);
			cloud_data = cloud_region[cloud_offs];

			if (cloud_data & (CLOUD_SHIFT[cloud_x & 0x0f]))
			{
				col = 2;	/* grey cloud */
			}
			else
			{
				col = 0;	/* black background */
			}
		}

		plot_pixel_8080(x, y, col);

		x++;
		cloud_x++;
		data >>= 1;
	}
}


static WRITE8_HANDLER( shuttlei_videoram_w )
{
	int x,y,i;
	videoram[offset] = data;
	y = offset >>5;
	x = 8 * (offset &31);
	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x+(7-i), y, (data & 0x01) ? 1 : 0);
		data >>= 1;
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( 8080bw )
{
	video_update_p(machine, screen, bitmap, cliprect);
	return 0;
}


static VIDEO_UPDATE( 8080bw_common )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		for (offs = 0;offs < videoram_size;offs++)
			videoram_w_p(offs, videoram[offs]);
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
	return 0;
}


static void draw_sight(mame_bitmap *bitmap,const rectangle *cliprect,int x_center, int y_center)
{
	int x,y;
	int sight_xs;
	int sight_xc;
	int sight_xe;
	int sight_ys;
	int sight_yc;
	int sight_ye;


	sight_xc = x_center;
	if( sight_xc < 2 )
	{
		sight_xc = 2;
	}
	else if( sight_xc > 253 )
	{
		sight_xc = 253;
	}

	sight_yc = y_center;
	if( sight_yc < 2 )
	{
		sight_yc = 2;
	}
	else if( sight_yc > 221 )
	{
		sight_yc = 221;
	}

	sight_xs = sight_xc - 20;
	if( sight_xs < 0 )
	{
		sight_xs = 0;
	}
	sight_xe = sight_xc + 20;
	if( sight_xe > 255 )
	{
		sight_xe = 255;
	}

	sight_ys = sight_yc - 20;
	if( sight_ys < 0 )
	{
		sight_ys = 0;
	}
	sight_ye = sight_yc + 20;
	if( sight_ye > 223 )
	{
		sight_ye = 223;
	}

	x = sight_xc;
	y = sight_yc;
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}


	draw_crosshair(bitmap,x,y,cliprect,0);
}


static VIDEO_UPDATE( seawolf )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(machine, screen, bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x1f) * 8) + 4, 63);
    return 0;
}

static VIDEO_UPDATE( blueshrk )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(machine, screen, bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x7f) * 2) - 12, 63);
    return 0;
}

static VIDEO_UPDATE( desertgu )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(machine, screen, bitmap, cliprect);

	draw_sight(bitmap,cliprect,
			   ((input_port_0_r(0) & 0x7f) * 2) - 30,
			   ((input_port_2_r(0) & 0x7f) * 2) + 2);
    return 0;
}


WRITE8_HANDLER( bowler_bonus_display_w )
{
	/* Bits 0-6 control which score is lit.
       Bit 7 appears to be a global enable, but the exact
       effect is not known. */

	bowler_bonus_display = data;
}


static VIDEO_UPDATE( bowler )
{

	/* update the bitmap */
	video_update_8080bw_common(machine, screen, bitmap, cliprect);


	/* draw the current bonus value - on the original game this
       was done using lamps that lit score displays on the bezel. */

/*
    int x,y,i;

    static const char score_line_1[] = "Bonus 200 400 500 700 500 400 200";
    static const char score_line_2[] = "      110 220 330 550 330 220 110";


    fix me -- this should be done with artwork
    x = 33 * 8;
    y = 31 * 8;

    for (i = 0; i < 33; i++)
    {
        int col;


        col = UI_COLOR_NORMAL;

        if ((i >= 6) && ((i % 4) != 1))
        {
            int bit = (i - 6) / 4;

            if (bowler_bonus_display & (1 << bit))
            {
                col = UI_COLOR_INVERSE;
            }
        }


        drawgfx(bitmap,Machine->uifont,
                score_line_1[i],col,
                0,1,
                x,y,
                cliprect,TRANSPARENCY_NONE,0);

        drawgfx(bitmap,Machine->uifont,
                score_line_2[i],col,
                0,1,
                x+8,y,
                cliprect,TRANSPARENCY_NONE,0);

        y -= Machine->uifontwidth;
    }
*/
    return 0;
}


PALETTE_INIT( invadpt2 )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 2),pal1bit(i >> 1));
	}
}


PALETTE_INIT( sflush )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 2),pal1bit(i >> 1));
	}
	palette_set_color(machine,0,0x80,0x80,0xff);
}

PALETTE_INIT( indianbt )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 1),pal1bit(i >> 2));
	}

}


static WRITE8_HANDLER( invadpt2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0x0400 : 0;
		col = memory_region(REGION_PROMS)[colbase | (y >> 3 << 5) | (x >> 3)] & 0x07;
	}
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

PALETTE_INIT( cosmo )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 1),pal1bit(i >> 2));
	}
}

WRITE8_HANDLER( cosmo_colorram_w )
{
	int i;
	int offs = ((offset>>5)<<8) | (offset&0x1f);

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i=0; i<8; i++)
	{
		videoram_w_p(offs, videoram[offs]);
		offs+= 0x20;
	}
}

static WRITE8_HANDLER( cosmo_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = offset % 32;

	/* 32 x 32 colormap */
	col = colorram[(y >> 3 << 5) | x ] & 0x07;

	plot_byte(8*x, y, data, col, 0);
}

static WRITE8_HANDLER( sstrngr2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 16 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0 : 0x0200;
		col = memory_region(REGION_PROMS)[colbase | (y >> 4 << 5) | (x >> 3)] & 0x0f;
	}
	else
		col = 1;	/* red */

	if (color_map_select)
	{
		x = 240 - x;
		y = 31 - y;
	}

	plot_byte(x, y, data, col, 0);
}
