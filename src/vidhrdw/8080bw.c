/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"

int invaders_interrupt(void);

static int use_tmpbitmap;
static int sight_xs;
static int sight_xc;
static int sight_xe;
static int sight_ys;
static int sight_yc;
static int sight_ye;
static int screen_red;
static int screen_red_enabled;		/* 1 for games that can turn the screen red */
static int color_map_select;
static int background_color;
static UINT8 polaris_cloud_pos;

static int artwork_type;
static const void *init_artwork;

static mem_write_handler videoram_w_p;
static void (*vh_screenrefresh_p)(struct osd_bitmap *bitmap,int full_refresh);
static void (*plot_pixel_p)(int x, int y, int col);

static WRITE_HANDLER( bw_videoram_w );
static WRITE_HANDLER( schaser_videoram_w );
static WRITE_HANDLER( lupin3_videoram_w );
static WRITE_HANDLER( polaris_videoram_w );
static WRITE_HANDLER( invadpt2_videoram_w );
static WRITE_HANDLER( astinvad_videoram_w );
static WRITE_HANDLER( spaceint_videoram_w );
static WRITE_HANDLER( helifire_videoram_w );

static void vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static void seawolf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static void blueshrk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static void desertgu_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static void phantom2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static void plot_pixel_8080 (int x, int y, int col);
static void plot_pixel_8080_tmpbitmap (int x, int y, int col);

/* smoothed pure colors, overlays are not so contrasted */

#define RED				0xff,0x20,0x20,OVERLAY_DEFAULT_OPACITY
#define GREEN 			0x20,0xff,0x20,OVERLAY_DEFAULT_OPACITY
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

//static const struct artwork_element invdpt2m_overlay[]=
//{
//	{{  16,  71,   0, 255}, GREEN  },
//	{{   0,  15,  16, 133}, GREEN  },
//	{{  72, 191,   0, 255}, YELLOW },
//	{{ 192, 223,   0, 255}, RED    },
//	END
//};

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

enum { NO_ARTWORK = 0, SIMPLE_OVERLAY, FILE_OVERLAY, SIMPLE_BACKDROP, FILE_BACKDROP };

void init_8080bw(void)
{
	videoram_w_p = bw_videoram_w;
	vh_screenrefresh_p = vh_screenrefresh;
	use_tmpbitmap = 0;
	screen_red = 0;
	screen_red_enabled = 0;
	artwork_type = NO_ARTWORK;
	color_map_select = 0;
	flip_screen_set(0);
}

void init_invaders(void)
{
	init_8080bw();
	init_artwork = invaders_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

void init_invaddlx(void)
{
	init_8080bw();
	//init_overlay = invdpt2m_overlay;
	//overlay_type = 1;
}

void init_invrvnge(void)
{
	init_8080bw();
	init_artwork = invrvnge_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

void init_invad2ct(void)
{
	init_8080bw();
	init_artwork = invad2ct_overlay;
	artwork_type = SIMPLE_OVERLAY;
}

void init_schaser(void)
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 2;	/* blue */
}

void init_rollingc(void)
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 0;	/* black */
}

void init_helifire(void)
{
	init_8080bw();
	videoram_w_p = helifire_videoram_w;
}

void init_polaris(void)
{
	init_8080bw();
	videoram_w_p = polaris_videoram_w;
}

void init_lupin3(void)
{
	init_8080bw();
	videoram_w_p = lupin3_videoram_w;
	background_color = 0;	/* black */
}

void init_invadpt2(void)
{
	init_8080bw();
	videoram_w_p = invadpt2_videoram_w;
	screen_red_enabled = 1;
}

void init_seawolf(void)
{
	init_8080bw();
	vh_screenrefresh_p = seawolf_vh_screenrefresh;
	use_tmpbitmap = 1;
}

void init_blueshrk(void)
{
	init_8080bw();
	vh_screenrefresh_p = blueshrk_vh_screenrefresh;
	use_tmpbitmap = 1;
}

void init_desertgu(void)
{
	init_8080bw();
	vh_screenrefresh_p = desertgu_vh_screenrefresh;
	use_tmpbitmap = 1;
}

void init_astinvad(void)
{
	init_8080bw();
	videoram_w_p = astinvad_videoram_w;
	screen_red_enabled = 1;
}

void init_spaceint(void)
{
	init_8080bw();
	videoram_w_p = spaceint_videoram_w;
}

void init_spcenctr(void)
{
	extern struct GameDriver driver_spcenctr;

	init_8080bw();
	init_artwork = driver_spcenctr.name;
	artwork_type = FILE_OVERLAY;
}

void init_phantom2(void)
{
	init_8080bw();
	vh_screenrefresh_p = phantom2_vh_screenrefresh;
	use_tmpbitmap = 1;
}

void init_boothill(void)
{
//	extern struct GameDriver driver_boothill;

	init_8080bw();
//	init_artwork = driver_boothill.name;
//	artwork_type = FILE_BACKDROP;
}

int invaders_vh_start(void)
{
	/* create overlay if one of was specified in init_X */
	if (artwork_type != NO_ARTWORK)
	{
		int start_pen;
		int max_pens;


		start_pen = 2;
		max_pens = Machine->drv->total_colors-start_pen;

		switch (artwork_type)
		{
		case SIMPLE_OVERLAY:
			overlay_create((const struct artwork_element *)init_artwork, start_pen, max_pens);
			break;
		case FILE_OVERLAY:
			overlay_load((const char *)init_artwork, start_pen, max_pens);
			break;
		case SIMPLE_BACKDROP:
			break;
		case FILE_BACKDROP:
			backdrop_load((const char *)init_artwork, start_pen, max_pens);
			break;
		default:
			logerror("Unknown artwork type.\n");
			break;
		}
	}

	if (use_tmpbitmap && (generic_bitmapped_vh_start() != 0))
		return 1;

	if (use_tmpbitmap)
	{
		plot_pixel_p = plot_pixel_8080_tmpbitmap;
	}
	else
	{
		plot_pixel_p = plot_pixel_8080;
	}

	/* make sure that the screen matches the videoram, this fixes invad2ct */
	schedule_full_refresh();

	return 0;
}


void invaders_vh_stop(void)
{
	if (use_tmpbitmap)  generic_bitmapped_vh_stop();
}


void invaders_flip_screen_w(int data)
{
	set_vh_global_attribute(&color_map_select, data);

	if (input_port_3_r(0) & 0x01)
	{
		flip_screen_set(data);
	}
}


void invaders_screen_red_w(int data)
{
	if (screen_red_enabled)
	{
		set_vh_global_attribute(&screen_red, data);
	}
}


int polaris_interrupt(void)
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 8)	/* every 4 frames - this was verified against real machine */
	{
		cloud_speed = 0;

		polaris_cloud_pos--;

		if (polaris_cloud_pos >= 0xe0)
		{
			polaris_cloud_pos = 0xdf;	/* no delay for invisible region */
		}

		schedule_full_refresh();
	}

	return invaders_interrupt();
}


static void plot_pixel_8080 (int x, int y, int col)
{
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}

	plot_pixel(Machine->scrbitmap,x,y,Machine->pens[col]);
}

static void plot_pixel_8080_tmpbitmap (int x, int y, int col)
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
		plot_pixel_p (x, y, (data & 0x01) ? fore_color : back_color);

		x++;
		data >>= 1;
	}
}


WRITE_HANDLER( invaders_videoram_w )
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
	int x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE_HANDLER( lupin3_videoram_w )
{
	int x,y,col;

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

	if (y < polaris_cloud_pos)
	{
		cloud_y = y - polaris_cloud_pos - 0x20;
	}
	else
	{
		cloud_y = y - polaris_cloud_pos;
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
				int offs,bit;

				col = back_color;

				bit = 1 << (~x & 0x03);
				offs = ((x >> 2) & 0x03) | ((~cloud_y & 0x3f) << 2);

				col = (memory_region(REGION_USER1)[offs] & bit) ? 7 : back_color;
			}

			plot_pixel_p (x, y, col);

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


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	vh_screenrefresh_p(bitmap, full_refresh);
}


static void vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (palette_recalc() || full_refresh)
	{
		int offs;

		for (offs = 0;offs < videoram_size;offs++)
			videoram_w_p(offs, videoram[offs]);
	}


	if (use_tmpbitmap)
	{
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
		osd_mark_dirty( sight_xc, sight_ys, sight_xc, sight_ye );
		osd_mark_dirty( sight_xs, sight_yc, sight_xe, sight_yc );
	}
}


static void draw_sight(struct osd_bitmap *bitmap,int x_center, int y_center)
{
	int x,y;

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

	draw_crosshair(bitmap,x,y,&Machine->visible_area);
}


static void seawolf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* update the bitmap (and erase old cross) */
	vh_screenrefresh(bitmap, full_refresh);

    draw_sight(bitmap,((input_port_0_r(0) & 0x1f) * 8) + 4, 31);
}

static void blueshrk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* update the bitmap (and erase old cross) */
	vh_screenrefresh(bitmap, full_refresh);

    draw_sight(bitmap,((input_port_0_r(0) & 0x7f) * 2) - 12, 31);
}

static void desertgu_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* update the bitmap (and erase old cross) */
	vh_screenrefresh(bitmap, full_refresh);

	draw_sight(bitmap,((input_port_0_r(0) & 0x7f) * 2) - 30,
			   ((input_port_2_r(0) & 0x7f) * 2) - 30);
}

static void phantom2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char *clouds;
	int x, y;


	/* update the bitmap */
	vh_screenrefresh(bitmap, full_refresh);


	/* draw the clouds */
	clouds = memory_region(REGION_PROMS);

	for (y = 0; y < 128; y++)
	{
		unsigned char *offs = &memory_region(REGION_PROMS)[y * 0x10];

		for (x = 0; x < 128; x++)
		{
			if (offs[x >> 3] & (1 << (x & 0x07)))
			{
				plot_pixel_8080(x*2,   y*2,   1);
				plot_pixel_8080(x*2+1, y*2,   1);
				plot_pixel_8080(x*2,   y*2+1, 1);
				plot_pixel_8080(x*2+1, y*2+1, 1);
			}
		}
	}
}


void invadpt2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */

		*(palette++) = 0xff * ((i >> 0) & 1);
		*(palette++) = 0xff * ((i >> 2) & 1);
		*(palette++) = 0xff * ((i >> 1) & 1);
	}
}

void helifire_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = 0xff * ((i >> 0) & 1);
		*(palette++) = 0xff * ((i >> 1) & 1);
		*(palette++) = 0xff * ((i >> 2) & 1);
	}
}


static WRITE_HANDLER( invadpt2_videoram_w )
{
	int x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
		col = memory_region(REGION_PROMS)[(color_map_select ? 0x400 : 0 ) + (((y+32)/8)*32) + (x/8)] & 7;
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( astinvad_videoram_w )
{
	int x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	if (!screen_red)
	{
		if (flip_screen)
			col = memory_region(REGION_PROMS)[((y+32)/8)*32+(x/8)] >> 4;
		else
			col = memory_region(REGION_PROMS)[(31-y/8)*32+(31-x/8)] & 0x0f;
	}
	else
		col = 1; /* red */

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( spaceint_videoram_w )
{
	int i,x,y,col;

	videoram[offset] = data;

	y = 8 * (offset / 256);
	x = offset % 256;

	/* this is wrong */
	col = memory_region(REGION_PROMS)[(y/16)+16*((x+16)/32)];

	for (i = 0; i < 8; i++)
	{
		plot_pixel_p(x, y, (data & 0x01) ? col : 0);

		y++;
		data >>= 1;
	}
}
