/***************************************************************************

  vidhrdw.c

  Generic functions used by the Cinematronics Vector games

***************************************************************************/

#include "driver.h"
#include "vector.h"
#include "cpu/ccpu/ccpu.h"

#define VEC_SHIFT 16

#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE

static int cinemat_monitor_type;
static int cinemat_overlay_req;
static int cinemat_backdrop_req;
static int cinemat_screenh;
static struct artwork_element *cinemat_simple_overlay;

static int color_display;
static struct artwork_info *spacewar_panel;
static struct artwork_info *spacewar_pressed_panel;

struct artwork_element starcas_overlay[]=
{
	{{ 0, 400-1, 0, 300-1},0x00,  61,  0xff, OVERLAY_DEFAULT_OPACITY},
	{{ 200, 49, 150, -2},  0xff, 0x20, 0x20, OVERLAY_DEFAULT_OPACITY},
	{{ 200, 29, 150, -2},  0xff, 0xff, 0xff, OVERLAY_DEFAULT_OPACITY}, /* punch hole in outer circle */
	{{ 200, 38, 150, -1},  0xe0, 0xff, 0x00, OVERLAY_DEFAULT_OPACITY},
	{{-1,-1,-1,-1},0,0,0,0}
};

struct artwork_element tailg_overlay[]=
{
	{{0, 400-1, 0, 300-1}, 0, 64, 64, OVERLAY_DEFAULT_OPACITY},
	{{-1,-1,-1,-1},0,0,0,0}
};

struct artwork_element sundance_overlay[]=
{
	{{0, 400-1, 0, 300-1}, 0xff, 0xff, 0x20, OVERLAY_DEFAULT_OPACITY},
	{{-1,-1,-1,-1},0,0,0,0}
};

struct artwork_element solarq_overlay[]=
{
	{{0, 400-1, 30, 300-1},0x20, 0x20, 0xff, OVERLAY_DEFAULT_OPACITY},
	{{ 0,  399, 0,    29}, 0xff, 0x20, 0x20, OVERLAY_DEFAULT_OPACITY},
	{{ 200, 12, 150,  -2}, 0xff, 0xff, 0x20, OVERLAY_DEFAULT_OPACITY},
	{{-1,-1,-1,-1},0,0,0,0}
};


void CinemaVectorData (int fromx, int fromy, int tox, int toy, int color)
{
    static int lastx, lasty;

	fromy = cinemat_screenh - fromy;
	toy = cinemat_screenh - toy;

	if (fromx != lastx || fromx != lasty)
		vector_add_point (fromx << VEC_SHIFT, fromy << VEC_SHIFT, 0, 0);

    if (color_display)
        vector_add_point (tox << VEC_SHIFT, toy << VEC_SHIFT, VECTOR_COLOR111(color & 0x07), color & 0x08 ? 0x80: 0x40);
    else
        vector_add_point (tox << VEC_SHIFT, toy << VEC_SHIFT, VECTOR_COLOR111(WHITE), color * 12);

	lastx = tox;
	lasty = toy;
}

/* This is called by the game specific init function and sets the local
 * parameters for the generic function cinemat_init_colors() */
void cinemat_select_artwork (int monitor_type, int overlay_req, int backdrop_req, struct artwork_element *simple_overlay)
{
	cinemat_monitor_type = monitor_type;
	cinemat_overlay_req = overlay_req;
	cinemat_backdrop_req = backdrop_req;
	cinemat_simple_overlay = simple_overlay;
}

void cinemat_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	char filename[1024];

	/* fill the rest of the 256 color entries depending on the game */
	switch (cinemat_monitor_type)
	{
		case  CCPU_MONITOR_BILEV:
		case  CCPU_MONITOR_16LEV:
            color_display = FALSE;
			/* Attempt to load backdrop if requested */
			if (cinemat_backdrop_req)
			{
                sprintf (filename, "%sb.png", Machine->gamedrv->name );
				backdrop_load(filename, 0);
			}
			/* Attempt to load overlay if requested */
			if (cinemat_overlay_req)
			{
				if (cinemat_simple_overlay != NULL)
				{
					/* use simple overlay */
					artwork_elements_scale(cinemat_simple_overlay,
										   Machine->scrbitmap->width,
										   Machine->scrbitmap->height);
					overlay_create(cinemat_simple_overlay, 0);
				}
				else
				{
					/* load overlay from file */
	                sprintf (filename, "%so.png", Machine->gamedrv->name );
					overlay_load(filename, 0);
				}
			}
			break;

		case  CCPU_MONITOR_WOWCOL:
            color_display = TRUE;
			break;
	}
}


void spacewar_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int width, height;

    color_display = FALSE;

	spacewar_pressed_panel = NULL;
	width = Machine->scrbitmap->width;
	height = 0.16 * width;

	artwork_load_size(&spacewar_panel, "spacewr1.png", 0, width, height);
	if (spacewar_panel != NULL)
	{
		artwork_load_size(&spacewar_pressed_panel, "spacewr2.png", 0, width, height);
		if (spacewar_pressed_panel == NULL)
		{
			artwork_free (&spacewar_panel);
			return ;
		}
	}
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cinemat_vh_start (void)
{
	vector_set_shift (VEC_SHIFT);
	cinemat_screenh = Machine->visible_area.max_y - Machine->visible_area.min_y;
	return vector_vh_start();
}

int spacewar_vh_start (void)
{
	vector_set_shift (VEC_SHIFT);
	cinemat_screenh = Machine->visible_area.max_y - Machine->visible_area.min_y;
	return vector_vh_start();
}


void cinemat_vh_stop (void)
{
	vector_vh_stop();
}

void spacewar_vh_stop (void)
{
	if (spacewar_panel != NULL)
		artwork_free(&spacewar_panel);

	if (spacewar_pressed_panel != NULL)
		artwork_free(&spacewar_pressed_panel);

	vector_vh_stop();
}

void cinemat_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
    vector_vh_screenrefresh(bitmap, full_refresh);
    vector_clear_list ();
}

void spacewar_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
    int tk[] = {3, 8, 4, 9, 1, 6, 2, 7, 5, 0};
	int i, pwidth, pheight, key, row, col, sw_option;
	float scale;
	struct mame_bitmap vector_bitmap;
	struct rectangle rect;

    static int sw_option_change;

	if (spacewar_panel == NULL)
	{
        vector_vh_screenrefresh(bitmap, full_refresh);
        vector_clear_list ();
		return;
	}

	pwidth = spacewar_panel->artwork->width;
	pheight = spacewar_panel->artwork->height;

	vector_bitmap = *bitmap;

	vector_vh_screenrefresh(&vector_bitmap,full_refresh);
    vector_clear_list ();

	if (full_refresh)
		copybitmap(bitmap,spacewar_panel->artwork,0,0,
				   0,bitmap->height - pheight, 0,TRANSPARENCY_NONE,0);

	scale = pwidth/1024.0;

    sw_option = input_port_1_r(0);

    /* move bits 10-11 to position 8-9, so we can just use a simple 'for' loop */
    sw_option = (sw_option & 0xff) | ((sw_option >> 2) & 0x0300);

    sw_option_change ^= sw_option;

	for (i = 0; i < 10; i++)
	{
		if (sw_option_change & (1 << i) || full_refresh)
		{
            key = tk[i];
            col = key % 5;
            row = key / 5;
			rect.min_x = scale * (465 + 20 * col);
			rect.max_x = scale * (465 + 20 * col + 18);
			rect.min_y = scale * (39  + 20 * row) + vector_bitmap.height;
			rect.max_y = scale * (39  + 20 * row + 18) + vector_bitmap.height;

			if (sw_option & (1 << i))
            {
				copybitmap(bitmap,spacewar_panel->artwork,0,0,
						   0, vector_bitmap.height, &rect,TRANSPARENCY_NONE,0);
            }
			else
            {
				copybitmap(bitmap,spacewar_pressed_panel->artwork,0,0,
						   0, vector_bitmap.height, &rect,TRANSPARENCY_NONE,0);
            }

			osd_mark_dirty(rect.min_x,rect.min_y,rect.max_x,rect.max_y);
		}
	}
    sw_option_change = sw_option;
}

int cinemat_clear_list(void)
{
    if (osd_skip_this_frame())
        vector_clear_list ();
    return ignore_interrupt();
}
