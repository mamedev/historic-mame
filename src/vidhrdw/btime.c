/***************************************************************************

	vidhrdw.c

	Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *lnc_charbank;
unsigned char *lnc_control;
unsigned char *bnj_backgroundram;
unsigned char *bnj_scroll2;
unsigned char *zoar_scrollram;
int bnj_backgroundram_size;

static int background_image = 0;
static int flipscreen = 0;
static int zoar_palette = 0;
static unsigned char bnj_scroll1 = 0;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *background_bitmap;

/***************************************************************************

	Convert the color PROMs into a more useable format.

	Burger Time doesn't have a color PROM. It uses RAM to dynamically
	create the palette.
	The palette RAM is connected to the RGB output this way:

	bit 7 -- 15 kohm resistor  -- BLUE (inverted)
		  -- 33 kohm resistor  -- BLUE (inverted)
		  -- 15 kohm resistor  -- GREEN (inverted)
		  -- 33 kohm resistor  -- GREEN (inverted)
		  -- 47 kohm resistor  -- GREEN (inverted)
		  -- 15 kohm resistor  -- RED (inverted)
		  -- 33 kohm resistor  -- RED (inverted)
	bit 0 -- 47 kohm resistor  -- RED (inverted)

***************************************************************************/
void btime_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* Burger Time doesn't have a color PROM, but Hamburge has. */
	/* This function is also used by Eggs. */
	if (color_prom == 0) return;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}
}

void lnc_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 5) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 2) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 4) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 0) & 0x01;
		bit2 = (*color_prom >> 1) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}
}


void eggs_init_machine(void)
{
	// Suspend the dummy sound CPU
	cpu_halt(1,0);
}

void lnc_init_machine(void)
{
	*lnc_charbank = 1;
	*lnc_control = 0;
}

/***************************************************************************

Start the video hardware emulation.

***************************************************************************/
int bnj_vh_start (void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(bnj_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,bnj_backgroundram_size);

	/* the background area is twice as tall as the screen */
	if ((background_bitmap = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}

/***************************************************************************

Stop the video hardware emulation.

***************************************************************************/
void bnj_vh_stop (void)
{
	osd_free_bitmap(background_bitmap);
	free(dirtybuffer2);
	generic_vh_stop();
}


void btime_paletteram_w(int offset,int data)
{
	int r,g,b;
	int bit0,bit1,bit2;

	r = (~data & 0x07);
	g = (~data & 0x38) >> 3;
	b = (~data & 0xC0) >> 6;

	bit0 = (r >> 0) & 0x01;
	bit1 = (r >> 1) & 0x01;
	bit2 = (r >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (g >> 0) & 0x01;
	bit1 = (g >> 1) & 0x01;
	bit2 = (g >> 2) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = 0;
	bit1 = (b >> 0) & 0x01;
	bit2 = (b >> 1) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_change_color(offset,r,g,b);
}

void lnc_videoram_w(int offset, int data)
{
	if (videoram[offset] != data || colorram[offset] != *lnc_charbank)
	{
		videoram[offset] = data;
		colorram[offset] = *lnc_charbank;

		dirtybuffer[offset] = 1;
	}
}

int btime_mirrorvideoram_r(int offset)
{
	int x,y;

	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	return videoram_r(offset);
}

int btime_mirrorcolorram_r(int offset)
{
	int x,y;

	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	return colorram_r(offset);
}

void btime_mirrorvideoram_w(int offset,int data)
{
	int x,y;

	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	videoram_w(offset,data);
}

void lnc_mirrorvideoram_w(int offset,int data)
{
	int x,y;

	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	lnc_videoram_w(offset,data);
}

void btime_mirrorcolorram_w(int offset,int data)
{
	int x,y;

	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	colorram_w(offset,data);
}

void btime_background_w(int offset,int data)
{
	if (background_image != data)
	{
		int offs, base;

		/* kludge to get the correct background */
		static int mapconvert[8] = { 1,2,3,0,5,6,7,4 };

		memset(dirtybuffer,1,videoram_size);

		background_image = data;

		// Create background image
		base = 0x100 * mapconvert[(background_image & 0x07)];

		for (offs = 0; offs < 0x100; offs++)
		{
			int bx,by;

			bx = 16 * (offs % 16);
			by = 16 * (offs / 16);

			drawgfx(tmpbitmap,Machine->gfx[2],
					Machine->memory_region[3][base+offs],
					0,
					0,0,
					bx,by,
					0,TRANSPARENCY_NONE,0);
		}
	}
}

void bnj_background_w(int offset, int data)
{
	if (bnj_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		bnj_backgroundram[offset] = data;
	}
}

void bnj_scroll1_w(int offset, int data)
{
	// Dirty screen if background is being turned off
	if (bnj_scroll1 && !data)
	{
		memset(dirtybuffer,1,videoram_size);
	}

	bnj_scroll1 = data;
}

void zoar_video_control_w(int offset,int data)
{
	// Zoar video control
	//
	// Bit 0-2 = Unknown (always 0). Marked as MCOL on schematics
	// Bit 3-4 = Palette
	// Bit 7   = Flip Screen

	static int zoar_video_control = -1;

	if (zoar_video_control != data)
	{
		memset(dirtybuffer,1,videoram_size);

		zoar_video_control = data;

		flipscreen   =  zoar_video_control & 0x80;
		zoar_palette = (zoar_video_control & 0x30) >> 3;
	}
}

/***************************************************************************

Draw the game screen in the given osd_bitmap.
Do NOT call osd_update_display() from this function, it will be called by
the main emulation engine.

***************************************************************************/
static void drawchars(struct osd_bitmap *bitmap, int color, int background_on)
{
	int offs, transparency;
	struct osd_bitmap *copytobitmap;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. If the background is on, */
	/* draw characters as sprites */

	transparency = background_on ? TRANSPARENCY_PEN : TRANSPARENCY_NONE;
	copytobitmap = background_on ? bitmap : tmpbitmap;

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;

		if (!dirtybuffer[offs] && !background_on) continue;

		dirtybuffer[offs] = 0;

		sx = offs % 32;
		sy = offs / 32;

		if (flipscreen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
		}

		drawgfx(copytobitmap,Machine->gfx[0],
				videoram[offs] + 256 * (colorram[offs] & 3),
				color,
				flipscreen,flipscreen,
				8*sx,8*sy,
				&Machine->drv->visible_area,transparency,0);
	}
}

static void drawsprites(struct osd_bitmap *bitmap, int color,
						int start, int sprite_x_adjust)
{
	int offs;

	/* Draw the sprites */
	for (offs = start;offs < videoram_size;offs += 4*0x20)
	{
		int sx,sy,flipx,flipy;

		if (!(videoram[offs + 0] & 0x01)) continue;

		sx = 240 - videoram[offs + 2*0x20] - sprite_x_adjust;
		sy = videoram[offs + 3*0x20];

		flipx = videoram[offs + 0] & 0x02;
		flipy = videoram[offs + 0] & 0x04;

		if (flipscreen)
		{
			sx = 240 - sx;
			sy = 240 - sy;

			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				videoram[offs + 0x20],
				color,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		sx += (flipscreen ? -256 : 256);

		// Wrap around
		drawgfx(bitmap,Machine->gfx[1],
				videoram[offs + 0x20],
				color,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}


void btime_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	if (background_image & 0x10)
	{
		// The background layer was drawn into tmpbitmap
		copybitmap(bitmap,tmpbitmap,flipscreen,flipscreen,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		drawchars(bitmap, 0, 1);
	}
	else
	{
		drawchars(bitmap, 0, 0);

		/* copy the temporary bitmap to the screen */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	drawsprites(bitmap, 0, 0, 1);
}


void eggs_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	drawchars(bitmap, 0, 0);

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	drawsprites(bitmap, 0, 0, 0);
}


void lnc_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	drawchars(bitmap, 0, 0);

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	drawsprites(bitmap, 0, 0, 1);
}


void zoar_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int scroll = -(*bnj_scroll2 | ((bnj_scroll1 & 0x03) << 8));

	if (bnj_scroll1 & 0x04)
	{
		int i, offs;

		// One extra iteration for wrap around
		for (i = 0; i < 5; i++, scroll += 256)
		{
			int tileoffset = zoar_scrollram[i & 3] * 0x100;

			// Skip if this title is completely off the screen
			if (scroll > 256)  break;
			if (scroll < -256) continue;

			for (offs = 0; offs < 0x100; offs++)
			{
				int sx,sy;

				sx = 16 * (offs % 16);
				sy = 16 * (offs / 16) + scroll;

				if (flipscreen)
				{
					sx = 240 - sx;
					sy = 240 - sy;
				}

				drawgfx(bitmap, Machine->gfx[2],
						Machine->memory_region[3][tileoffset + offs],
						zoar_palette,
						flipscreen,flipscreen,
						sx,sy,
						0,TRANSPARENCY_NONE,0);
			}
		}

		drawchars(bitmap, zoar_palette+1, 1);
	}
	else
	{
		drawchars(bitmap, zoar_palette+1, 0);

		/* copy the temporary bitmap to the screen */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* The order is important for correct priorities */
	drawsprites(bitmap, zoar_palette+1, 0x1f, 1);
	drawsprites(bitmap, zoar_palette+1, 0, 1);
}


void bnj_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/*
	 *  For each character in the background RAM, check if it has been
	 *  modified since last time and update it accordingly.
	 */
	if (bnj_scroll1)
	{
		int scroll, offs;

		for (offs = bnj_backgroundram_size-1; offs >=0; offs--)
		{
			int sx,sy;

			if (!dirtybuffer2[offs]) continue;

			dirtybuffer2[offs] = 0;

			sx = 16 * (((offs % 0x100) < 0x80) ? offs % 8 : (offs % 8) + 8);
			sy = 16 * ((offs < 0x100) ? ((offs % 0x80) / 8) : ((offs % 0x80) / 8) + 16);

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}

			drawgfx(background_bitmap, Machine->gfx[2],
					(bnj_backgroundram[offs] >> 4) + ((offs & 0x80) >> 3) + 32,
					0,
					flipscreen, flipscreen,
					sx, sy,
					0, TRANSPARENCY_NONE, 0);
		}

		/* copy the background bitmap to the screen */
		scroll = (bnj_scroll1 & 0x02) * 128 + 511 - *bnj_scroll2;
		copyscrollbitmap (bitmap, background_bitmap, 0, 0, 1, &scroll, &Machine->drv->visible_area,TRANSPARENCY_NONE, 0);

		// The sprites appear below the foremost layer
		drawsprites(bitmap, 0, 0, 0);

		drawchars(bitmap, 0, 1);
	}
	else
	{
		drawchars(bitmap, 0, 0);

		/* copy the temporary bitmap to the screen */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		drawsprites(bitmap, 0, 0, 0);
	}
}

