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

static struct rectangle spritevisiblearea =
{
	0, (24*8)-1,          //8 protected chars on right of screen
	0, (31*8)-1
};


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Kung Fu Master has a six 256x4 palette PROMs (one per gun; three for
  characters, three for sprites).
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void yard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i,j,used;
	unsigned char allocated[3*256];


	/* The game has 512 colors, but we are limited to a maximum of 256. */
	/* Luckily, many of the colors are duplicated, so the total number of */
	/* different colors is less than 256. We select the unique colors and */
	/* put them in our palette. */

	memset(palette,0,3 * Machine->drv->total_colors);

	used = 0;
	for (i = 0;i < 512;i++)
	{
		for (j = 0;j < used;j++)
		{
			if (allocated[j] == color_prom[i] &&
					allocated[j+256] == color_prom[i+512] &&
					allocated[j+2*256] == color_prom[i+2*512])
				break;
		}
		if (j == used)
		{
			int bit0,bit1,bit2,bit3;


			used++;

			allocated[j] = color_prom[i];
			allocated[j+256] = color_prom[i+512];
			allocated[j+2*256] = color_prom[i+2*512];

			bit0 = (color_prom[i] >> 0) & 0x01;
			bit1 = (color_prom[i] >> 1) & 0x01;
			bit2 = (color_prom[i] >> 2) & 0x01;
			bit3 = (color_prom[i] >> 3) & 0x01;
			palette[3*j] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[i+512] >> 0) & 0x01;
			bit1 = (color_prom[i+512] >> 1) & 0x01;
			bit2 = (color_prom[i+512] >> 2) & 0x01;
			bit3 = (color_prom[i+512] >> 3) & 0x01;
			palette[3*j + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[i+512*2] >> 0) & 0x01;
			bit1 = (color_prom[i+512*2] >> 1) & 0x01;
			bit2 = (color_prom[i+512*2] >> 2) & 0x01;
			bit3 = (color_prom[i+512*2] >> 3) & 0x01;
			palette[3*j + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		}

		colortable[i] = j;
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
void yard_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = 0x1000-2;offs >= 0;offs-=2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1] )
		{
			int sx,sy;
			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = 8 * ( ( offs % 64 ) / 2 ) ;
			sy = 8 * ( offs / 64 ) ;

                        if ( offs >= 0x800 )
                        {
                          sy -= 32 * 8 ;
                          sx += 32 * 8 ;
                        }

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 4 * (videoram[offs+1] & 0xc0),
					videoram[offs+1] & 0x1f,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll_x , scroll_y ;

		scroll_x = - ( ( *yard_scroll_x_high * 0x100 ) + *yard_scroll_x_low ) ;
		scroll_y = - *yard_scroll_y_low ;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll_x,1,&scroll_y,&spritevisiblearea,TRANSPARENCY_NONE,0);
	}

	/* draw the static bitmapped area to screen */
	for (offs = 0x1000;offs <= 0x1FFF;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,n;
			dirtybuffer[offs] = 0;
			sx = ( ( offs - 0x1000 ) % 16 ) * 4 + ( 24 * 8 ) ;
			sy = ( ( offs - 0x1000 ) / 16 ) ;
			n = videoram [ offs ] ;
			bitmap->line[sy][sx + 3] = Machine->pens[( ( n & 0x80 ) ? 0x10 : 0x00 ) | ( ( n & 0x08 ) ? 0x01 : 0x00 )] ;
			bitmap->line[sy][sx + 2] = Machine->pens[( ( n & 0x40 ) ? 0x10 : 0x00 ) | ( ( n & 0x04 ) ? 0x01 : 0x00 )] ;
			bitmap->line[sy][sx + 1] = Machine->pens[( ( n & 0x20 ) ? 0x10 : 0x00 ) | ( ( n & 0x02 ) ? 0x01 : 0x00 )] ;
			bitmap->line[sy][sx + 0] = Machine->pens[( ( n & 0x10 ) ? 0x10 : 0x00 ) | ( ( n & 0x01 ) ? 0x01 : 0x00 )] ;
		}
	}

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sprt,bank;
		bank = ((spriteram[offs + 1] & 0x020) >> 5) + 1;
		sprt = spriteram[offs + 2];
		if (sprt > 0x7f && bank != 1)
		{
			sprt = sprt - 0x40;
		}
		drawgfx(bitmap,Machine->gfx[bank],
				sprt,//spriteram[offs + 2] ,
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 1] & 0x40,spriteram[offs + 1] & 0x80,
				spriteram[offs + 3],241 - spriteram[offs],
				&spritevisiblearea,TRANSPARENCY_PEN,0);
		sprt = sprt + 0x40;
		if (((spriteram[offs + 1] & 0xfb) != 0  && bank == 1) || (bank == 2) || (bank == 1 && sprt < 0x80))
		{
		drawgfx(bitmap,Machine->gfx[bank],
				sprt,//spriteram[offs + 2] ,
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 1] & 0x40,spriteram[offs + 1] & 0x80,
				spriteram[offs + 3],241 - spriteram[offs] + 16,
				&spritevisiblearea,TRANSPARENCY_PEN,0);
		}

	}

}
