/***************************************************************************

  vidhrdw.c

  10 Yard Fight

L Taylor
J Clegg

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *yard_scroll_x_low;
unsigned char *yard_scroll_x_high;
unsigned char *yard_scroll_y_low;
unsigned char *yard_sprite_priority; /* JB 970912 */

/* JB 970912 */
static struct rectangle spritevisiblearea =
{
	0, (26*8)-1,          //6 protected chars on right of screen
	8, (31*8)-1
};
static struct rectangle spritevisiblearea2 =
{
	0, (31*8)-1,
	8, (31*8)-1
};


/***************************************************************************

  Convert the color PROMs into a more useable format.

  10 Yard Fight has two 256x4 character palette PROMs, one 32x8 sprite
  palette PROM, one 256x4 sprite color lookup table PROM, and two 256x4
  radar palette PROMs.

  I don't know for sure how the palette PROMs are connected to the RGB
  output, but it's probably something like this; note that RED and BLUE
  are swapped wrt the usual configuration.

  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
  bit 0 -- 1  kohm resistor  -- BLUE

***************************************************************************/
void yard_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* character palette and lookup table */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = 0;
		bit1 = (color_prom[256] >> 2) & 0x01;
		bit2 = (color_prom[256] >> 3) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[256] >> 0) & 0x01;
		bit2 = (color_prom[256] >> 1) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		COLOR(0,i) = i;
		color_prom++;
	}

	color_prom += TOTAL_COLORS(0);
	/* color_prom now points to the beginning of the sprite palette */


	/* make the transparent pen unique (none of the game colors can have */
	/* these RGB components) */
	*(palette++) = 1;
	*(palette++) = 1;
	*(palette++) = 1;
	color_prom++;

	/* sprite palette */
	for (i = 1;i < 16;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	color_prom += 16;
	/* color_prom now points to the beginning of the sprite lookup table */


	/* sprite lookup table */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = 256 + (*color_prom & 0x0f);
		color_prom++;
	}

	/* color_prom now points to the beginning of the radar palette */


	/* radar palette and lookup table */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = 0;
		bit1 = (color_prom[256] >> 2) & 0x01;
		bit2 = (color_prom[256] >> 3) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[256] >> 0) & 0x01;
		bit2 = (color_prom[256] >> 1) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		COLOR(2,i) = 256 + 16 + i;
		color_prom++;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int yard_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width*2,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void yard_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void yard_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	struct rectangle *visible_rect;/* JB 970912 */

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0x1000-2;offs >= 0;offs-=2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy;
			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = 8 * ( ( offs % 64 ) / 2 ) ;
			sy = 8 * ( offs / 64 ) ;

			if ( offs >= 0x800 )
			{
				sy -= 32 * 8;
				sx += 32 * 8;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 4 * (videoram[offs+1] & 0xc0),
					videoram[offs+1] & 0x1f,
					videoram[offs+1] & 0x20,0,/* JB 970912 */
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll_x , scroll_y ;

		scroll_x = - ( ( *yard_scroll_x_high * 0x100 ) + *yard_scroll_x_low ) ;
		scroll_y = - *yard_scroll_y_low ;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll_x,1,&scroll_y,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	if (! *yard_sprite_priority)	/* JB 971008 */
	{
		/* draw the static bitmapped area to screen */
		for (offs = 0x1000;offs <= 0x1FFF;offs++)
		{
			int sx,sy,n,i;

			dirtybuffer[offs] = 0;
			sx = ( ( offs - 0x1000 ) % 16 ) * 4 + ( 25 * 8 ) - 4;/* JB 970912 */
			sy = ( ( offs - 0x1000 ) / 16 ) ;
			if (sy >= Machine->drv->visible_area.min_y &&
					sy <= Machine->drv->visible_area.max_y)
			{
				n = videoram [ offs ];
				for (i = 0;i < 4;i++)
				{
					int col;


					col = (n >> i) & 0x11;
					col = ((col >> 3) | col) & 3;
					if (sx+i >= Machine->drv->visible_area.max_x-(6*8-1) &&
							sx+i <= Machine->drv->visible_area.max_x)
						bitmap->line[sy][sx + i] = Machine->gfx[2]->colortable[(sy & 0xfc) + col];
				}
			}
		}
	}

	visible_rect = (*yard_sprite_priority ? &spritevisiblearea2 : &spritevisiblearea);/* JB 970912 */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sprt,bank,flipx,flipy;
		bank = (spriteram[offs + 1] & 0x020) >> 5;
		sprt = spriteram[offs + 2];
		sprt &= 0xbf;
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;

		if (flipy) sprt = sprt + 0x40;
		drawgfx(bitmap,Machine->gfx[1],
				sprt + 256 * bank,
				spriteram[offs + 1] & 0x1f,
				flipx,flipy,
				spriteram[offs + 3],241 - spriteram[offs],
				visible_rect,TRANSPARENCY_COLOR,256);
		if (flipy) sprt = sprt - 0x40;
		else sprt = sprt + 0x40;
		drawgfx(bitmap,Machine->gfx[1],
				sprt + 256 * bank,
				spriteram[offs + 1] & 0x1f,
				flipx,flipy,
				spriteram[offs + 3],241 - spriteram[offs] + 16,
				visible_rect,TRANSPARENCY_COLOR,256);
	}
}
