/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define LOG_IT 0
#define DEBUG_IT 0

#define VIDEO_RAM_SIZE 0x800

unsigned char mappy_scroll;

#if LOG_IT
#include <stdio.h>
static FILE *pCharFile;
static FILE *pSpriteFile;
#endif


int mappy_vh_start(void)
{
	if ((dirtybuffer = malloc(VIDEO_RAM_SIZE)) == 0)
		return 1;
	memset(dirtybuffer,0,VIDEO_RAM_SIZE);

	if ((tmpbitmap = osd_create_bitmap(60*8,36*8)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

#if LOG_IT
	pCharFile = fopen ("mappy-ch.log", "w");
	fprintf (pCharFile, "SXx SYy CHh A CLc\n");
	fprintf (pCharFile, "=== === === = ===\n");
	pSpriteFile = fopen ("mappy-sp.log", "w");
	fprintf (pSpriteFile, " Xx   Yy  SPs CLc FX FY DX DY IXi\n");
	fprintf (pSpriteFile, "==== ==== === === == == == == ===\n");
#endif

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mappy_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
	
#if LOG_IT
	fclose (pCharFile);
	fclose (pSpriteFile);
#endif
}



void mappy_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		videoram[offset] = data;
	}
}


void mappy_colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		colorram[offset] = data;
	}
}


void mappy_scroll_w(int offset,int data)
{
	mappy_scroll = offset >> 3;
}



void mappy_draw_sprite(struct osd_bitmap *dest,unsigned int code,unsigned int color,
	int flipx,int flipy,int sx,int sy)
{
	int transparency, background;

	/* this looks cheesy, but as far as I can tell, it's right */
	if (sy < Machine->drv->visible_area.min_y + 8*8 || sy > Machine->drv->visible_area.max_y - 4*8)
	{
		transparency = TRANSPARENCY_THROUGH;
		background = Machine->background_pen;
	}
	else
	{
		transparency = TRANSPARENCY_PEN;
		background = 15;
	}
	
	drawgfx(dest,Machine->gfx[1],code,color,flipx,flipy,sx,sy,&Machine->drv->visible_area,
		transparency,background);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mappy_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;

#if DEBUG_IT
	static unsigned char realv[VIDEO_RAM_SIZE], realc[VIDEO_RAM_SIZE];
	static int touched = false;
	if (osd_key_pressed (OSD_KEY_PGUP))
	{
		memcpy (realv, videoram, VIDEO_RAM_SIZE);
		memcpy (realc, colorram, VIDEO_RAM_SIZE);
		touched = true;
		for (i = 0; i < VIDEO_RAM_SIZE; i++)
			videoram[i] = colorram[i] >> 4;
		memset (colorram, 2, VIDEO_RAM_SIZE);
		memset (dirtybuffer, 1, 60 * 36);
	}
	else if (osd_key_pressed (OSD_KEY_PGDN))
	{
		memcpy (realv, videoram, VIDEO_RAM_SIZE);
		memcpy (realc, colorram, VIDEO_RAM_SIZE);
		touched = true;
		for (i = 0; i < VIDEO_RAM_SIZE; i++)
			videoram[i] = colorram[i] & 0xf;
		memset (colorram, 2, VIDEO_RAM_SIZE);
		memset (dirtybuffer, 1, 60 * 36);
	}
	else if (touched)
	{
		memset (dirtybuffer, 1, 60 * 36);
		touched = false;
	}
#endif

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE - 128;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;

			dirtybuffer[offs] = 0;

			mx = offs / 32;
			my = offs % 32;

			sx = 59 - mx;
			sy = my + 2;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
			
			#if LOG_IT
				if (videoram[offs] != 32)
				fprintf (pCharFile, "%2dx %2dy %02Xh %c %02Xc\n", sx, sy, videoram[offs], 
							videoram[offs] > 32 && videoram[offs] < 127 ? videoram[offs] : ' ', colorram[offs]);
			#endif
		}
	}

	/* Draw the bottom 2 lines. */
	for ( ;offs < VIDEO_RAM_SIZE - 64;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;

			dirtybuffer[offs] = 0;

			mx = offs % 32;
			my = (offs - (VIDEO_RAM_SIZE - 128)) / 32;

			sx = 29 - mx;
			sy = my + 34;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

			#if LOG_IT
				if (videoram[offs] != 32)
				fprintf (pCharFile, "%2dx %2dy %02Xh %c %02Xc\n", sx, sy, videoram[offs], 
							videoram[offs] > 32 && videoram[offs] < 127 ? videoram[offs] : ' ', colorram[offs]);
			#endif
		}
	}

	/* Draw the top 2 lines. */
	for ( ;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;

			dirtybuffer[offs] = 0;

			mx = offs % 32;
			my = (offs - (VIDEO_RAM_SIZE - 64)) / 32;

			sx = 29 - mx;
			sy = my;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

			#if LOG_IT
				if (videoram[offs] != 32)
				fprintf (pCharFile, "%2dx %2dy %02Xh %c %02Xc\n", sx, sy, videoram[offs], 
							videoram[offs] > 32 && videoram[offs] < 127 ? videoram[offs] : ' ', colorram[offs]);
			#endif
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll[36];

		for (i = 0;i < 2;i++)
			scroll[i] = 0;
		for (i = 2;i < 34;i++)
			scroll[i] = mappy_scroll - 255;
		for (i = 34;i < 36;i++)
			scroll[i] = 0;

		copyscrollbitmap(bitmap,tmpbitmap,36,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* Draw the sprites. */
	for (i = 0;i < 64;i++)
	{
		/* is it on? */
		if ((spriteram_3[2*i+1] & 2) == 0)
		{
			int sprite = spriteram[2*i];
			int color = spriteram[2*i+1];
			int x = spriteram_2[2*i]-16;
			int y = (spriteram_2[2*i+1]-40) + 0x100*(spriteram_3[2*i+1] & 1);
			int flipx = spriteram_3[2*i] & 2;
			int flipy = spriteram_3[2*i] & 1;
			
			/* is it on-screen? */
			if (x > -16)
			{
				switch (spriteram_3[2*i] & 0x0c)
				{
					case 0:		/* normal size */
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
						break;

					case 4:		/* 2x vertical */
						if (!flipy)
						{
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
						}
						else
						{
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
						}
						break;
					
					case 8:		/* 2x horizontal */
						if (!flipx)
						{
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
						}
						else
						{
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
						}
						break;
					
					case 12:		/* 2x both ways */
						if (!flipy && !flipx)
						{
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,16+y);
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,16+y);
						}
						else if (flipy && flipx)
						{
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
							mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,y);
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,16+y);
						}
						else if (flipx)
						{
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
							mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,16+y);
						}
						else /* flipy */
						{
							mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,y);
							mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,16+y);
							mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,y);
							mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,16+y);
						}
						break;
				}
				
				#if LOG_IT
					fprintf (pSpriteFile, "%3dx %3dy %02Xs %02Xc %s %s %s %s %02Xi\n",
						x,y,sprite,color,flipx?"FX":"  ",flipy?"FY":"  ",
						spriteram_3[2*i]&8?"DX":"  ",
						spriteram_3[2*i]&4?"DY":"  ",i);
				#endif
			}
		}
	}

#if DEBUG_IT
	if (touched)
	{
		memcpy (videoram, realv, VIDEO_RAM_SIZE);
		memcpy (colorram, realc, VIDEO_RAM_SIZE);
	}
#endif
}
