/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int x,y,screen_width;
static int last_command;

// We reason we need this pending business, is that otherwise, when the guy
// walks on the rainbow, he'd leave a trail behind him
static int pending, pending_x, pending_y, pending_color;

WRITE_HANDLER( leprechn_graphics_command_w )
{
    last_command = data;
}

WRITE_HANDLER( leprechn_graphics_data_w )
{
    int direction;

    if (pending)
    {
		plot_pixel(tmpbitmap, pending_x, pending_y, Machine->pens[pending_color]);
        videoram[pending_y * screen_width + pending_x] = pending_color;

        pending = 0;
    }

    switch (last_command)
    {
    // Write Command
    case 0x00:
        direction = (data & 0xf0) >> 4;
        switch (direction)
        {
        case 0x00:
        case 0x04:
        case 0x08:
        case 0x0c:
            break;

        case 0x01:
        case 0x09:
            x++;
            break;

        case 0x02:
        case 0x06:
            y++;
            break;

        case 0x03:
            x++;
            y++;
            break;

        case 0x05:
        case 0x0d:
            x--;
            break;

        case 0x07:
            x--;
            y++;
            break;

        case 0x0a:
        case 0x0e:
            y--;
            break;

        case 0x0b:
            x++;
            y--;
            break;

        case 0x0f:
            x--;
            y--;
            break;
        }

        x = x & 0xff;
        y = y & 0xff;

        pending = 1;
        pending_x = x;
        pending_y = y;
        pending_color = data & 0x0f;

        return;

    // X Position Write
    case 0x08:
        x = data;
        return;

    // Y Position Write
    case 0x10:
        y = data;
        return;

    // Clear Bitmap
    case 0x18:
        fillbitmap(tmpbitmap,Machine->pens[data],0);
        memset(videoram, data, screen_width * Machine->drv->screen_height);
        return;
    }

    // Just a precaution. Doesn't seem to happen.
    logerror("Unknown Graphics Command #%2X at %04X\n", last_command, activecpu_get_pc());
}


READ_HANDLER( leprechn_graphics_data_r )
{
    return videoram[y * screen_width + x];
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( leprechn )
{
    screen_width = Machine->drv->screen_width;

    if ((videoram = auto_malloc(screen_width*Machine->drv->screen_height)) == 0)
        return 1;

    if ((tmpbitmap = auto_bitmap_alloc(screen_width,Machine->drv->screen_height)) == 0)
        return 1;

    pending = 0;

    return 0;
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( leprechn )
{
	if (get_vh_global_attribute_changed())
	{
		int sx, sy;

		/* redraw bitmap */

		for (sx = 0; sx < screen_width; sx++)
		{
			for (sy = 0; sy < Machine->drv->screen_height; sy++)
			{
				plot_pixel(tmpbitmap, sx, sy, Machine->pens[videoram[sy * screen_width + sx]]);
			}
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
