/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"
#include "8080bw.h"


static int screen_red;
static int screen_red_enabled;		/* 1 for games that can turn the screen red */
static int color_map_select;
static int background_color;
static UINT8 cloud_pos;

static int artwork_type;
static const void *init_artwork;

static mem_write_handler videoram_w_p;
static void (*video_update_p)(struct mame_bitmap *bitmap,const struct rectangle *cliprect);

static WRITE_HANDLER( bw_videoram_w );
static WRITE_HANDLER( schaser_videoram_w );
static WRITE_HANDLER( lupin3_videoram_w );
static WRITE_HANDLER( polaris_videoram_w );
static WRITE_HANDLER( invadpt2_videoram_w );
static WRITE_HANDLER( astinvad_videoram_w );
static WRITE_HANDLER( sstrngr2_videoram_w );
static WRITE_HANDLER( spaceint_videoram_w );
static WRITE_HANDLER( helifire_videoram_w );
static WRITE_HANDLER( phantom2_videoram_w );

static VIDEO_UPDATE( 8080bw_common );
static VIDEO_UPDATE( seawolf );
static VIDEO_UPDATE( blueshrk );
static VIDEO_UPDATE( desertgu );

static void plot_pixel_8080(int x, int y, int col);

/* smoothed colors, overlays are not so contrasted */

#define RED				0xff,0x20,0x20,OVERLAY_DEFAULT_OPACITY
#define GREEN 			0x20,0xff,0x20,OVERLAY_DEFAULT_OPACITY
#define BLUE			0x20,0x20,0xff,OVERLAY_DEFAULT_OPACITY
#define LT_BLUE 		0xa0,0xa0,0xff,OVERLAY_DEFAULT_OPACITY
#define YELLOW			0xff,0xff,0x20,OVERLAY_DEFAULT_OPACITY
#define CYAN			0x20,0xff,0xff,OVERLAY_DEFAULT_OPACITY

#define	END  {{ -1, -1, -1, -1}, 0,0,0,0}


static const struct artwork_element invaders_overlay[]=
{
	{{  16,  71,   0, 255}, GREEN },
	{{   0,  15,  16, 133}, GREEN },
	{{ 192, 223,   0, 255}, RED   },
	END
};

/*static const struct artwork_element invdpt2m_overlay[]= */
/*{ */
/*	{{  16,  71,   0, 255}, GREEN  }, */
/*	{{   0,  15,  16, 133}, GREEN  }, */
/*	{{  72, 191,   0, 255}, YELLOW }, */
/*	{{ 192, 223,   0, 255}, RED    }, */
/*	END */
/*}; */

static const struct artwork_element invrvnge_overlay[]=
{
	{{   0,  71,   0, 255}, GREEN },
	{{ 192, 223,   0, 255}, RED   },
	END
};

static const struct artwork_element invad2ct_overlay[]=
{
	{{	 0,  47,   0, 255}, YELLOW },
	{{	25,  70,   0, 255}, GREEN  },
	{{	48, 139,   0, 255}, CYAN   },
	{{ 117, 185,   0, 255}, GREEN  },
	{{ 163, 231,   0, 255}, YELLOW },
	{{ 209, 255,   0, 255}, RED    },
	END
};

static const struct artwork_element phantom2_overlay[]=
{
	{{   0,  255, 0, 255}, LT_BLUE },
	END
};

static const struct artwork_element gunfight_overlay[]=
{
	{{   0,  255, 0, 255}, YELLOW },
	END
};

static const struct artwork_element bandido_overlay[]=
{
	{{   0,  23,   0, 255}, BLUE },
	{{  24,  39,   0,  99}, BLUE },
	{{  24,  39, 124, 255}, BLUE },
	{{  24,  39, 100, 123}, GREEN },
	{{  40, 183,   0,  23}, BLUE },
	{{  40,  99,  24,  31}, BLUE },
	{{ 124, 183,  24,  31}, BLUE },
	{{ 100, 123,  24,  39}, RED },
	{{ 184, 199, 100, 123}, GREEN },
	{{ 184, 199,   0,  99}, BLUE },
	{{ 184, 199, 124, 255}, BLUE },
	{{ 200, 231,   0, 255}, BLUE },
	{{ 232, 255,   0, 255}, RED },
	{{  40,  99,  32,  39}, YELLOW },
	{{ 124, 183,  32,  39}, YELLOW },
	{{  40, 183,  40, 183}, YELLOW },
	{{  40,  99, 184, 191}, YELLOW },
	{{ 124, 183, 184, 191}, YELLOW },
	{{  40,  99, 192, 199}, BLUE },
	{{ 124, 183, 192, 199}, BLUE },
	{{  40, 183, 200, 255}, BLUE },
	{{ 100, 123, 184, 199}, RED },
	END
};


enum { NO_ARTWORK = 0, SIMPLE_OVERLAY, FILE_OVERLAY, SIMPLE_BACKDROP, FILE_BACKDROP };

DRIVER_INIT( 8080bw )
{
	videoram_w_p = bw_videoram_w;
	video_update_p = video_update_8080bw_common;
	screen_red = 0;
	screen_red_enabled = 0;
	artwork_type = NO_ARTWORK;
	color_map_select = 0;
	flip_screen_set(0);
}

DRIVER_INIT( invaders )
{
	init_8080bw();
	init_artwork = invaders_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

DRIVER_INIT( invaddlx )
{
	init_8080bw();
	/*init_artwork = invdpt2m_overlay; */
	/*artwork_type = SIMPLE_OVERLAY; */
}

DRIVER_INIT( invrvnge )
{
	init_8080bw();
	init_artwork = invrvnge_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

DRIVER_INIT( invad2ct )
{
	init_8080bw();
	init_artwork = invad2ct_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

DRIVER_INIT( sstrngr2 )
{
	init_8080bw();
	videoram_w_p = sstrngr2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( schaser )
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 2;	/* blue */
}

DRIVER_INIT( rollingc )
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 0;	/* black */
}

DRIVER_INIT( helifire )
{
	init_8080bw();
	videoram_w_p = helifire_videoram_w;
}

DRIVER_INIT( polaris )
{
	init_8080bw();
	videoram_w_p = polaris_videoram_w;
}

DRIVER_INIT( lupin3 )
{
	init_8080bw();
	videoram_w_p = lupin3_videoram_w;
	background_color = 0;	/* black */
}

DRIVER_INIT( invadpt2 )
{
	init_8080bw();
	videoram_w_p = invadpt2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( seawolf )
{
	init_8080bw();
	video_update_p = video_update_seawolf;
}

DRIVER_INIT( blueshrk )
{
	init_8080bw();
	video_update_p = video_update_blueshrk;
}

DRIVER_INIT( desertgu )
{
	init_8080bw();
	video_update_p = video_update_desertgu;
}

DRIVER_INIT( astinvad )
{
	init_8080bw();
	videoram_w_p = astinvad_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( spaceint )
{
	init_8080bw();
	videoram_w_p = spaceint_videoram_w;
}

DRIVER_INIT( spcenctr )
{
	extern struct GameDriver driver_spcenctr;

	init_8080bw();
	init_artwork = driver_spcenctr.name;
	artwork_type = FILE_OVERLAY;
}

DRIVER_INIT( phantom2 )
{
	init_8080bw();
	videoram_w_p = phantom2_videoram_w;
	init_artwork = phantom2_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

DRIVER_INIT( boothill )
{
	extern struct GameDriver driver_boothill;

	init_8080bw();
	init_artwork = driver_boothill.name;
	artwork_type = FILE_BACKDROP;
}

DRIVER_INIT( gunfight )
{
	init_8080bw();
	init_artwork = gunfight_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

DRIVER_INIT( bandido )
{
	init_8080bw();
	init_artwork = bandido_overlay;
	artwork_type = SIMPLE_OVERLAY;
}


VIDEO_START( 8080bw )
{
	int start_pen = Machine->drv->total_colors - 32768;


	/* create overlay if one of was specified in init_X */
	switch (artwork_type)
	{
	case SIMPLE_OVERLAY:
		overlay_create((const struct artwork_element *)init_artwork, start_pen);
		break;
	case FILE_OVERLAY:
		overlay_load((const char *)init_artwork, start_pen);
		break;
	case SIMPLE_BACKDROP:
		break;
	case FILE_BACKDROP:
		backdrop_load((const char *)init_artwork, start_pen);
		break;
	case NO_ARTWORK:
		break;
	default:
		logerror("Unknown artwork type.\n");
		break;
	}

	/* make sure that the screen matches the videoram, this fixes invad2ct */
	set_vh_global_attribute(NULL,0);

	return video_start_generic_bitmapped();
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

		cloud_pos--;

		if (cloud_pos >= 0xe0)
		{
			cloud_pos = 0xdf;	/* no delay for invisible region */
		}

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


WRITE_HANDLER( c8080bw_videoram_w )
{
	videoram_w_p(offset, data);
}


static WRITE_HANDLER( bw_videoram_w )
{
	int x,y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	plot_byte(x, y, data, 1, 0);
}

static WRITE_HANDLER( schaser_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE_HANDLER( lupin3_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = ~colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE_HANDLER( polaris_videoram_w )
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

	color_map = memory_region(REGION_PROMS)[(((y+32)/8)*32) + (x/8)];
	back_color = (color_map & 1) ? 6 : 2;
	fore_color = ~colorram[offset & 0x1f1f] & 0x07;

	/* bit 3 is connected to the cloud enable. bits 1 and 2 are marked 'not use' (sic)
	   on the schematics */

	if (y < cloud_pos)
	{
		cloud_y = y - cloud_pos - 0x20;
	}
	else
	{
		cloud_y = y - cloud_pos;
	}

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

static WRITE_HANDLER( helifire_videoram_w )
{
	int x,y,back_color,foreground_color;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	back_color = 0;
	foreground_color = colorram[offset] & 0x07;

	if (x < 0x78)
	{
		back_color = 4;	/* blue */
	}

	plot_byte(x, y, data, foreground_color, back_color);
}


WRITE_HANDLER( schaser_colorram_w )
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

READ_HANDLER( schaser_colorram_r )
{
	return colorram[offset & 0x1f1f];
}


WRITE_HANDLER( helifire_colorram_w )
{
	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	videoram_w_p(offset, videoram[offset]);
}


static WRITE_HANDLER( phantom2_videoram_w )
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


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( 8080bw )
{
	video_update_p(bitmap, cliprect);
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
}


static void draw_sight(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int x_center, int y_center)
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


	draw_crosshair(bitmap,x,y,cliprect);
}


static VIDEO_UPDATE( seawolf )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x1f) * 8) + 4, 31);
}

static VIDEO_UPDATE( blueshrk )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x7f) * 2) - 12, 31);
}

static VIDEO_UPDATE( desertgu )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

	draw_sight(bitmap,cliprect,
			   ((input_port_0_r(0) & 0x7f) * 2) - 30,
			   ((input_port_2_r(0) & 0x7f) * 2) - 30);
}


PALETTE_INIT( invadpt2 )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 2) & 1);
		int b = 0xff * ((i >> 1) & 1);
		palette_set_color(i,r,g,b);
	}
}

PALETTE_INIT( helifire )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 1) & 1);
		int b = 0xff * ((i >> 2) & 1);
		palette_set_color(i,r,g,b);
	}
}


static WRITE_HANDLER( invadpt2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0x400 : 0;
		col = memory_region(REGION_PROMS)[colbase + (((y+32)/8)*32) + (x/8)] & 7;
	}
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( sstrngr2_videoram_w )
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
		col = memory_region(REGION_PROMS)[colbase + ((y/16+2) & 0x0f)*32 + (x/8)] & 0x0f;
	}
	else
		col = 1;	/* red */

	if (color_map_select)
	{
		x = 240 - x;
		y = 223 - y;
	}

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( astinvad_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	if (!screen_red)
	{
		if (flip_screen)
			col = memory_region(REGION_PROMS)[((y+32)/8)*32 + (x/8)] >> 4;
		else
			col = memory_region(REGION_PROMS)[(31-y/8)*32 + (31-x/8)] & 0x0f;
	}
	else
		col = 1; /* red */

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( spaceint_videoram_w )
{
	UINT8 x,y,col;
	int i;

	videoram[offset] = data;

	y = 8 * (offset / 256);
	x = offset % 256;

	/* this is wrong */
	col = memory_region(REGION_PROMS)[(y/16)+16*((x+16)/32)];

	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x, y, (data & 0x01) ? col : 0);

		y++;
		data >>= 1;
	}
}
