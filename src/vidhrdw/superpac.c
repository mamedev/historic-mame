/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400


void superpac_draw_sprite(struct osd_bitmap *dest,unsigned int code,unsigned int color,
	int flipx,int flipy,int sx,int sy)
{
	drawgfx(dest,Machine->gfx[1],code,color,flipx,flipy,sx,sy,&Machine->drv->visible_area,
		TRANSPARENCY_PEN,0);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void superpac_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;

			dirtybuffer[offs] = 0;

	/* Even if Pengo's screen is 28x36, the memory layout is 32x32. We therefore */
	/* have to convert the memory coordinates into screen coordinates. */
	/* Note that 32*32 = 1024, while 28*36 = 1008: therefore 16 bytes of Video RAM */
	/* don't map to a screen position. We don't check that here, however: range */
	/* checking is performed by drawgfx(). */

			mx = offs / 32;
			my = offs % 32;

			if (mx <= 1)
			{
				sx = 29 - my;
				sy = mx + 34;
			}
			else if (mx >= 30)
			{
				sx = 29 - my;
				sy = mx - 30;
			}
			else
			{
				sx = 29 - mx;
				sy = my + 2;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. */
	for (i = 0;i < 64;i++)
	{
		/* is it on? */
		if ((spriteram_3[2*i+1] & 2) == 0)
		{
			int sprite = spriteram[2*i];
			int color = spriteram[2*i+1];
			int x = spriteram_2[2*i]-17;
			int y = (spriteram_2[2*i+1]-40) + 0x100*(spriteram_3[2*i+1] & 1);
			int flipx = spriteram_3[2*i] & 2;
			int flipy = spriteram_3[2*i] & 1;
			
			if (color > 37) color -= 32;
			
			/* is it on-screen? */
			if (x > -16)
			{
				switch (spriteram_3[2*i] & 0x0c)
				{
					case 0:		/* normal size */
						superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
						break;

					case 4:		/* 2x vertical */
						if (!flipy)
						{
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
						}
						else
						{
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
						}
						break;
					
					case 8:		/* 2x horizontal */
						if (!flipx)
						{
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
						}
						else
						{
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
						}
						break;
					
					case 12:		/* 2x both ways */
						if (!flipy && !flipx)
						{
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,16+y);
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,16+y);
						}
						else if (flipy && flipx)
						{
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
							superpac_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,y);
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,16+y);
						}
						else if (flipx)
						{
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
							superpac_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,16+y);
						}
						else /* flipy */
						{
							superpac_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,y);
							superpac_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,16+y);
							superpac_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,y);
							superpac_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,16+y);
						}
						break;
				}
			}
		}
	}

#if 0

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (i = 0;i < 56; i++)
	{
		if (spriteram[32*i+1])
			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
				/*charnum*/ spriteram[32*i + 1],
				spriteram[32*i + 2] + 32,
				/*hflip*/ spriteram[32*i] & 0x02,
				/*vflip*/ spriteram[32*i] & 0x01,
				/*xpos*/ 231 - spriteram[32*i + 3],
				/*ypos*/ 8 + spriteram[32*i + 4],
				&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}
	/* Draw the pacman sprite(s). */
	for( i=0; i<4; i++)
	{
		/* ugly hack to handle big pacman sprite */
		if( spriteram_2[32*i] & 0x04 )
		{
			unsigned char off[4] = { 0,0,0,0 };
			
			/* order of chars depends on hflip/vflip */
			switch( spriteram_2[32*i] & 0x03 )
			{
				case 0x00:
					off[0] = 2; off[1] = 3; off[2] = 0; off[3] = 1;
					break;
				case 0x01:
					off[0] = 3; off[1] = 2; off[2] = 1; off[3] = 0;
					break;
				case 0x02:
					off[0] = 0; off[1] = 1; off[2] = 2; off[3] = 3;
					break;
			}
			/*	0 norm  -8  +16,  -8 + 0	A	char + 2
				1 norm  -8  +16,  -8 + 16	B	char + 3
				2 norm  -8  +0,   -8 + 0	C	char + 0
				3 norm  -8  +0,   -8 + 16	D	char + 1

				0 hflip -8  +0,   -8 + 0	C   char + 0
				1 hflip -8  +0,   -8 + 16	D   char + 1
				2 hflip -8 +16,   -8 + 0	A	char + 2
				3 hflip -8 +16,   -8 + 16	B	char + 3

				0 vflip -8 +16,   -8 + 16	B	char + 3
				1 vflip -8 +16,   -8 + 0	A	char + 2
				2 vflip -8 +0,    -8 + 16	D	char + 1
				3 vflip -8 + 0,   -8 + 0	C	char + 0	*/
			
			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
					/*charnum*/ spriteram_2[32*i + 1]+off[0],     /* spriteram[2*i] >> 2, */
					/*color*/ spriteram_2[32*i + 2],              /* spriteram[2*i + 1],  */
					/*hflip*/ spriteram_2[32*i] & 0x02,           /* spriteram[2*i] & 2,  */
					/*vflip*/ spriteram_2[32*i] & 0x01,           /* spriteram[2*i] & 1,  */
					/*xpos*/ 231 - 8 - spriteram_2[32*i + 3],     /* 239 - spriteram_2[2*i], */
					/*ypos*/ 8 - 8 + spriteram_2[32*i + 4],       /* 272 - spriteram_2[2*i + 1], */
					&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);

			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
					/*charnum*/ spriteram_2[32*i + 1]+off[1],
					/*color*/ spriteram_2[32*i + 2],
					/*hflip*/ spriteram_2[32*i] & 0x02,
					/*vflip*/ spriteram_2[32*i] & 0x01,
					/*xpos*/ 231 - 8 - spriteram_2[32*i + 3],
					/*ypos*/ 8 + 8 + spriteram_2[32*i + 4],
					&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);

			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
					/*charnum*/ spriteram_2[32*i + 1]+off[2],
					/*color*/ spriteram_2[32*i + 2],
					/*hflip*/ spriteram_2[32*i] & 0x02,
					/*vflip*/ spriteram_2[32*i] & 0x01,
					/*xpos*/ 8 + 231 - spriteram_2[32*i + 3],
					/*ypos*/ 8 - 8 + spriteram_2[32*i + 4],
					&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
					
			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
					/*charnum*/ spriteram_2[32*i + 1]+off[3],
					/*color*/ spriteram_2[32*i + 2],
					/*hflip*/ spriteram_2[32*i] & 0x02,
					/*vflip*/ spriteram_2[32*i] & 0x01,
					/*xpos*/ 8 + 231 - spriteram_2[32*i + 3],
					/*ypos*/ 8 + 8 + spriteram_2[32*i + 4],
					&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
		}
		else
		{
			/* another ugly hack (sigh...) */
			unsigned char color;
			
			color = spriteram_2[32*i + 2];
			if( color<0x20 ) color += 0x20;

			drawgfx(bitmap,Machine->gfx[gfx_bank ? 3 : 1],
					/*charnum*/ spriteram_2[32*i + 1],              /* spriteram[2*i] >> 2,        */
					/*color*/ color /*spriteram_2[32*i + 2]*/,      /* spriteram[2*i + 1],         */
					/*hflip*/ spriteram_2[32*i] & 0x02,             /* spriteram[2*i] & 2,         */
					/*vflip*/ spriteram_2[32*i] & 0x01,             /* spriteram[2*i] & 1,         */
					/*xpos*/ 231 - spriteram_2[32*i + 3],           /* 239 - spriteram_2[2*i],     */
					/*ypos*/ 8 + spriteram_2[32*i + 4],             /* 272 - spriteram_2[2*i + 1], */
					&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
		}
	}
#endif
}
