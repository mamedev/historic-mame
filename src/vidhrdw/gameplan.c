//#define VERBOSE
int logging = 0;

/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"


static int clear_to_colour = 0;
static int fix_clear_to_colour = -1;
static unsigned char *gameplan_videoram;


void gameplan_clear_screen(void);


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/


int gameplan_vh_start(void)
{
	if ((gameplan_videoram = malloc(256*256)) == 0)
		return 1;

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width, Machine->drv->screen_height)) == 0)
	{
		free(gameplan_videoram);
		return 1;
	}

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/


void gameplan_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(gameplan_videoram);
}


void gameplan_video_w(int offset,int data)
{
	int cpu_getpc(void);
	static int r0 = -1;
	static unsigned char xpos, ypos, colour = 7;

#ifdef VERY_VERBOSE
	if (errorlog && logging)
	{
		fprintf(errorlog, "%04x: offset %d, data %d\n",
				cpu_getpc(), offset, data);
	}
#endif /* VERBOSE */

	if (offset == 0)
	{
		r0 = data;
#ifdef VERBOSE
		if (errorlog && logging) fprintf(errorlog, "mode = %d\n", data);
#endif
	}
	else if (offset == 1)
	{
		if (r0 == 0 && 1)
		{
#ifdef VERBOSE
			if (errorlog && logging)
			{
				fprintf(errorlog, "line command %02x at (%d, %d) col %d\n",
						data, xpos, ypos, colour);
				fflush(errorlog);
			}
#endif

			if (data & 0x20)
			{
				if (data & 0x80) xpos--;
				else xpos++;
			}
			if (data & 0x10)
			{
				if (data & 0x40) ypos--;
				else ypos++;
			}

			if (data & 0x0f)
				if (errorlog)
					fprintf(errorlog,
							"!movement command %02x unknown\n", data);

			if (gameplan_videoram[xpos+256*ypos] != colour)
			{
				gameplan_videoram[xpos+256*ypos] = colour;

				tmpbitmap->line[xpos][ypos] = Machine->pens[colour];
				osd_mark_dirty(ypos, xpos, ypos, xpos, 0);
			}
		}
		else if (r0 == 1)
		{
			ypos = data;
#ifdef VERBOSE
			if (errorlog && logging)
				fprintf(errorlog, "Y = %d\n", ypos);
#endif
		}
		else if (r0 == 2)
		{
			xpos = data;
#ifdef VERBOSE
			if (errorlog && logging)
				fprintf(errorlog, "X = %d\n", xpos);
#endif
		}
		else if (r0 == 3)
		{
			if (offset == 1 && data == 0)
			{
#ifdef VERBOSE
				if (errorlog)
					fprintf(errorlog, "clear screen\n");
#endif
				gameplan_clear_screen();
			}
			else if (errorlog)
				fprintf(errorlog,
						"!not clear screen: offset = %d, data = %d\n",
						offset, data);
		}
		else
		{
			if (errorlog)
				fprintf(errorlog, "!offset = %d, data = %02x\n",
						offset, data);
		}
	}
	else if (offset == 2)
	{
		if (data == 7)
		{
			if (fix_clear_to_colour == -1)
				clear_to_colour = colour;
#ifdef VERBOSE
			if (errorlog)
			{
				if (fix_clear_to_colour == -1)
					fprintf(errorlog, "clear screen colour = %d\n",
							colour);
				else
					fprintf(errorlog,
							"clear req colour %d hidden by fixed colour %d\n",
							colour, fix_clear_to_colour);
			}
#endif
		}
		else
		{
			if (errorlog)
				fprintf(errorlog, "!offset = %d, data = %02x\n",
						offset, data);
		}
	}
	else if (offset == 3)
	{
		if (r0 == 0)
		{
			if (errorlog && (data & 0xf8) != 0xf8)
			{
				fprintf(errorlog,
						"!unknown data (%02x) written for pixel (%3d, %3d)\n",
						data, xpos, ypos);
			}

			colour = data & 7;
#ifdef VERBOSE
			if (errorlog && logging)
				fprintf(errorlog, "colour %d, move to (%d, %d)\n",
						colour, xpos, ypos);
#endif
		}
		else if ((data & 0xf8) == 0xf8 && data != 0xff)
		{
			clear_to_colour = fix_clear_to_colour = data & 0x07;
			if (errorlog) fprintf(errorlog, "unusual colour request %d\n",
								  data & 7);
		}
		else
		{
			if (errorlog)
				fprintf(errorlog, "!offset = %d, data = %02x\n",
						offset, data);
		}
	}
	else
	{
		if (errorlog)
			fprintf(errorlog, "!offset = %d, data = %02x\n",
					offset, data);
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


void gameplan_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* enable run-time switching of debugging log information */
	if (osd_key_pressed(OSD_KEY_9)) logging = 1;
	if (osd_key_pressed(OSD_KEY_8)) logging = 0;

	/* copy the character mapped graphics */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0,
			   &Machine->drv->visible_area,
			   TRANSPARENCY_NONE, 0);
}


void gameplan_clear_screen(void)
{
	int x, y;

	if (errorlog)
		fprintf(errorlog, "clearing the screen to colour %d (%d)\n",
				clear_to_colour, Machine->pens[clear_to_colour]);

	fillbitmap(tmpbitmap, Machine->pens[clear_to_colour], 0);

	for (x = 0; x < 256; x++)
		for (y = 0; y < 256; y++)
			gameplan_videoram[x+256*y] = clear_to_colour;

	fix_clear_to_colour = -1;
}
