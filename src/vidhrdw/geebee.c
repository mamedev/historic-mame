/****************************************************************************
 *
 * geebee.c
 *
 * video driver
 * juergen buchmueller <pullmoll@t-online.de>, jan 2000
 *
 * TODO:
 *
 * backdrop support for lamps? (player1, player2 and serve)
 * what is the counter output anyway?
 *
 ****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from machine/geebee.c */
extern int geebee_ball_h;
extern int geebee_ball_v;
extern int geebee_lamp1;
extern int geebee_lamp2;
extern int geebee_lamp3;
extern int geebee_counter;
extern int geebee_lock_out_coil;
extern int geebee_bgw;
extern int geebee_ball_on;
extern int geebee_inv;

int geebee_vh_start(void)
{
    if (generic_vh_start())
        return 1;
    return 0;
}

void geebee_vh_stop(void)
{
    generic_vh_stop();
}

INLINE void geebee_plot(struct osd_bitmap *bitmap, int x, int y)
{
    struct rectangle r = Machine->drv->visible_area;
    if (x >= r.min_x && x <= r.max_x && y >= r.min_y && y <= r.max_y)
        plot_pixel(bitmap,x,y,Machine->pens[1]);
}

INLINE void mark_dirty(int x, int y)
{
	int cx, cy, offs;
	cy = y / 8;
	cx = x / 8;
    if (geebee_inv)
	{
		offs = (31 - cx) + (31 - cy) * 32;
		dirtybuffer[offs % videoram_size] = 1;
		dirtybuffer[(offs - 1) % videoram_size] = 1;
		dirtybuffer[(offs - 32) % videoram_size] = 1;
		dirtybuffer[(offs - 32 - 1) % videoram_size] = 1;
	}
    else
	{
		offs = (cx - 2) + cy * 32;
		dirtybuffer[offs % videoram_size] = 1;
		dirtybuffer[(offs + 1) % videoram_size] = 1;
		dirtybuffer[(offs + 32) % videoram_size] = 1;
		dirtybuffer[(offs + 32 + 1) % videoram_size] = 1;
	}
}

void geebee_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

	if( full_refresh )
		memset(dirtybuffer, 1, videoram_size);

	if( geebee_inv )
	{
		for( offs = 0; offs < videoram_size; offs++ )
		{
			if( dirtybuffer[offs] )
			{
				int sx, sy, code, color;

                dirtybuffer[offs] = 0;
				if (offs < 2 * 32)
				{
					sx = (33 - (offs / 32)) * 8;
					sy = (31 - (offs % 32)) * 8;
				}
				else
				{
					sy = (31 - (offs / 32)) * 8;
					sx = (33 - (2 + (offs % 32))) * 8;
				}
				code = videoram[offs] & 0x7f;
				color = ((geebee_bgw & 1) << 1) | ((videoram[offs] & 0x80) >> 7);
				drawgfx(
					bitmap,Machine->gfx[0],code,color,1,1,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				osd_mark_dirty(sx,sy,sx+7,sy+7,0);
			}
        }
	}
	else
	{
		for( offs = 0; offs < videoram_size; offs++ )
		{
			if( dirtybuffer[offs] )
            {
				int sx, sy, code, color;

                dirtybuffer[offs] = 0;
                if (offs < 2 * 32)
				{
					sx = (offs / 32) * 8;
					sy = (offs % 32) * 8;
				}
				else
				{
					sy = (offs / 32) * 8;
					sx = (2 + (offs % 32)) * 8;
				}
				code = videoram[offs] & 0x7f;
				color = ((geebee_bgw & 1) << 1) | ((videoram[offs] & 0x80) >> 7);
				drawgfx(
					bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				osd_mark_dirty(sx,sy,sx+7,sy+7,0);
			}
		}
    }

	if( geebee_ball_on )
    {
		int x, y;
		if( geebee_inv )
		{
			mark_dirty(geebee_ball_h-4,geebee_ball_v-4);
			for( y = 0; y < 4; y++ )
				for( x = 0; x < 4; x++ )
					geebee_plot(bitmap,geebee_ball_h+x-4,geebee_ball_v+y-4);
		}
		else
		{
			mark_dirty(geebee_ball_h+12,geebee_ball_v-4);
			for( y = 0; y < 4; y++ )
				for( x = 0; x < 4; x++ )
					geebee_plot(bitmap,geebee_ball_h+x+12,geebee_ball_v+y-4);
        }
    }
}

