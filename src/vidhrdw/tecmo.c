/***************************************************************************

  video hardware for Tecmo games

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *tecmo_videoram,*tecmo_colorram;
unsigned char *tecmo_videoram2,*tecmo_colorram2;
unsigned char *tecmo_scroll;
unsigned char *tecmo_paletteram;
int tecmo_videoram2_size;

static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static unsigned char dirtycolor[64];



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static int video_type = 0;
/*
   video_type is used to distinguish Rygar and Silkworm
   This is needed because there is a difference in the sprite indexing.
   Also, some additional optimizing is possible with Silkworm.
*/

int tecmo_vh_start(void);

int rygar_vh_start(void){
  video_type = 0;
  return tecmo_vh_start(); /*common initialization */
}

int silkworm_vh_start(void){
  video_type = 1;
  return tecmo_vh_start(); /*common initialization */
}

int tecmo_vh_start(void)
{
  if (generic_vh_start() ) return 1;

  if ((dirtybuffer2 = malloc(videoram_size)) == 0){
    generic_vh_stop();
    return 1;
  }
  memset(dirtybuffer2,1,videoram_size);

  /* the background area is twice as wide as the screen */
  if ((tmpbitmap2 = osd_new_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0){
    free(dirtybuffer2);
    generic_vh_stop();
    return 1;
  }
  if ((tmpbitmap3 = osd_new_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0){
    osd_free_bitmap(tmpbitmap2);
    free(dirtybuffer2);
    generic_vh_stop();
    return 1;
  }

  return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void tecmo_vh_stop(void){
  osd_free_bitmap(tmpbitmap3);
  osd_free_bitmap(tmpbitmap2);
  free(dirtybuffer2);
  generic_vh_stop();
}

void tecmo_videoram_w(int offset,int data){
  if (tecmo_videoram[offset] != data){
    dirtybuffer2[offset] = 1;

    tecmo_videoram[offset] = data;
  }
}

void tecmo_colorram_w(int offset,int data){
  if (tecmo_colorram[offset] != data){
    dirtybuffer2[offset] = 1;

    tecmo_colorram[offset] = data;
  }
}

void tecmo_paletteram_w(int offset,int data){
  if (tecmo_paletteram[offset] != data){
    dirtycolor[offset/2/16] = 1;
    tecmo_paletteram[offset] = data;

    if( video_type && (offset & ~1) == 0x200 ) /* silkworm-specific optimization */
      {
	int i;

	/* special case: color 0 of the character set is used for the */
	/* background. To speed up rendering, we set it in color 0 of bg #2 */

	/* we can't do this for Rygar, because the sprites priority bits must be */
	/* handled to allow the sun (made up of sprites) to be drawn between the */
	/* background and bg#2 */

	for (i = 0;i < 16;i++)
	  dirtycolor[48 + i] = 1;
      }
  }
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void tecmo_draw_sprites( struct osd_bitmap *bitmap, int priority );
void tecmo_draw_sprites( struct osd_bitmap *bitmap, int priority ){
  int offs;

  /* draw all visible sprites of specified priority */
  for (offs = 0;offs < spriteram_size;offs += 8){
    int flags = spriteram[offs+3];
    if( (flags>>6) == priority ){
      int bank = spriteram[offs+0];
      if( bank & 4 ){ /* visible */
	int size = (spriteram[offs + 2] & 3);
	/* 0 = 8x8 1 = 16x16 2 = 32x32 */

	int which = spriteram[offs+1];

	int code;

	if( video_type )
	  code = (which) + ((bank&0xf8)<<5); /* silkworm */
	else
	  code = (which)+((bank&0xf0)<<4); /* rygar */

	if (size == 1) code >>= 2;
	else if (size == 2) code >>= 4;

	drawgfx(bitmap,Machine->gfx[size+1],
		code,
		flags&0xf, /* color */
		bank&1, /* flipx */
		bank&2, /* flipy */

		spriteram[offs + 5] - ((flags & 0x10) << 4), /* sx */
		spriteram[offs + 4] - ((flags & 0x20) << 3), /* sy */

		&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	  }
	}
  }
}

void tecmo_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	int bgblue = tecmo_paletteram[0x200] & 0x0f;
	int bgred = (tecmo_paletteram[0x201] & 0xf0) >> 4;
	int bggreen = tecmo_paletteram[0x201] & 0x0f;

	bgred = (bgred << 4) + bgred;
	bggreen = (bggreen << 4) + bggreen;
	bgblue = (bgblue << 4) + bgblue;

	/* rebuild the color lookup table */
	{
		int i,j;
		int gf[4] = { 1, 0, 4, 5 };
		int off[4] = { 0x000, 0x200, 0x400, 0x600 };

		for (j = 0;j < 4;j++)
		{
			for (i = 0;i < 16*16;i++)
			{
				int blue = tecmo_paletteram[2*i+off[j]] & 0x0f;
				int red = (tecmo_paletteram[2*i+1+off[j]] & 0xf0) >> 4;
				int green = tecmo_paletteram[2*i+1+off[j]] & 0x0f;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				/* avoid undesired transparency using dark red instead of black */
				if (j >= 2 && !red && !green && !blue && i % 16 != 0) red = 0x20;

				/* silkworm specific optimization:
				   color 0 of the character set is used for the background.
				   To speed up rendering, we set it in color 0 of bg #2 */
				if ( video_type && j == 3 && i % 16 == 0) red = bgred, green = bggreen, blue = bgblue;

				setgfxcolorentry (Machine->gfx[gf[j]], i, red, green, blue);
			}
		}
	}

	/* draw the background. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int color = colorram[offs] >> 4;

		if (dirtybuffer[offs] || dirtycolor[color + 48])
		{
			int sx = offs % 32;
			int sy = offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[5],
					videoram[offs] + 256 * (colorram[offs] & 0x7),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}

		color = tecmo_colorram[offs] >> 4;

		if (dirtybuffer2[offs] || dirtycolor[color + 32])
		{
			int sx = offs % 32;
			int sy = offs / 32;

			drawgfx(tmpbitmap3,Machine->gfx[4],
					tecmo_videoram[offs] + 256 * (tecmo_colorram[offs] & 0x7),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
		dirtybuffer2[offs] = 0;
	}

	for (offs = 0;offs < 64;offs++)
		dirtycolor[offs] = 0;


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;

		/* draw background tiles */
		scrollx = -tecmo_scroll[3] - 256 * (tecmo_scroll[4] & 1) - 48;
		scrolly = -tecmo_scroll[5];
		if( video_type )	/* speedup for Silkworm */
		{
			copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			tecmo_draw_sprites( bitmap,3 ); /* this should never draw anything, but just in case... */
		}
		else
		{
			fillbitmap(bitmap,rgbpen (bgred, bggreen, bgblue),&Machine->drv->visible_area);
			tecmo_draw_sprites( bitmap,3 );
			copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
		}

		tecmo_draw_sprites( bitmap,2 );

		/* draw foreground tiles */
		scrollx = -tecmo_scroll[0] - 256 * (tecmo_scroll[1] & 1) - 48;
		scrolly = -tecmo_scroll[2];
		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}

	tecmo_draw_sprites( bitmap,1 );

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = tecmo_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx = offs % 32;
		int sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				tecmo_videoram2[offs] + ((tecmo_colorram2[offs] & 0x03) << 8),
				tecmo_colorram2[offs] >> 4,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
	tecmo_draw_sprites( bitmap,0 ); /* frontmost sprites - used? */
}


void gemini_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	int bgblue = tecmo_paletteram[0x200] & 0x0f;
	int bgred = (tecmo_paletteram[0x201] & 0xf0) >> 4;
	int bggreen = tecmo_paletteram[0x201] & 0x0f;

	bgred = (bgred << 4) + bgred;
	bggreen = (bggreen << 4) + bggreen;
	bgblue = (bgblue << 4) + bgblue;

	/* rebuild the color lookup table */
	{
		int i,j;
		int gf[4] = { 1, 0, 4, 5 };
		int off[4] = { 0x000, 0x200, 0x400, 0x600 };

		for (j = 0;j < 4;j++)
		{
			for (i = 0;i < 16*16;i++)
			{
				int blue = tecmo_paletteram[2*i+off[j]] & 0x0f;
				int red = (tecmo_paletteram[2*i+1+off[j]] & 0xf0) >> 4;
				int green = tecmo_paletteram[2*i+1+off[j]] & 0x0f;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				/* avoid undesired transparency using dark red instead of black */
				if (j >= 2 && !red && !green && !blue && i % 16 != 0) red = 0x20;

				/* silkworm specific optimization:
				   color 0 of the character set is used for the background.
				   To speed up rendering, we set it in color 0 of bg #2 */
				if ( video_type && j == 3 && i % 16 == 0) red = bgred, green = bggreen, blue = bgblue;

				setgfxcolorentry (Machine->gfx[gf[j]], i, red, green, blue);
			}
		}
	}

	/* draw the background. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int color = colorram[offs] & 0x0f;

		if (dirtybuffer[offs] || dirtycolor[color + 48])
		{
			int sx = offs % 32;
			int sy = offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[5],
					videoram[offs] + 16 * (colorram[offs] & 0x70),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}

		color = tecmo_colorram[offs] & 0xf;

		if (dirtybuffer2[offs] || dirtycolor[color + 32])
		{
			int sx = offs % 32;
			int sy = offs / 32;

			drawgfx(tmpbitmap3,Machine->gfx[4],
					tecmo_videoram[offs] + 16 * (tecmo_colorram[offs] & 0x70),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
		dirtybuffer2[offs] = 0;
	}

	for (offs = 0;offs < 64;offs++)
		dirtycolor[offs] = 0;


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;

		/* draw background tiles */
		scrollx = -tecmo_scroll[3] - 256 * (tecmo_scroll[4] & 1) - 48;
		scrolly = -tecmo_scroll[5];
		if( video_type )	/* speedup for Silkworm */
		{
			copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			tecmo_draw_sprites( bitmap,3 ); /* this should never draw anything, but just in case... */
		}
		else
		{
			fillbitmap(bitmap,rgbpen (bgred, bggreen, bgblue),&Machine->drv->visible_area);
			tecmo_draw_sprites( bitmap,3 );
			copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
		}

		tecmo_draw_sprites( bitmap,2 );

		/* draw foreground tiles */
		scrollx = -tecmo_scroll[0] - 256 * (tecmo_scroll[1] & 1) - 48;
		scrolly = -tecmo_scroll[2];
		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}

	tecmo_draw_sprites( bitmap,1 );

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = tecmo_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx = offs % 32;
		int sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				tecmo_videoram2[offs] + ((tecmo_colorram2[offs] & 0x03) << 8),
				tecmo_colorram2[offs] >> 4,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
	tecmo_draw_sprites( bitmap,0 ); /* frontmost sprites - used? */
}
