/***************************************************************************

  vidhrdw/zerozone.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *zerozone_videoram;

static unsigned char *video_dirty;



WRITE16_HANDLER( zerozone_videoram_w )
{
	int oldword = zerozone_videoram[offset];
	COMBINE_DATA(&zerozone_videoram[offset]);

	if (oldword != zerozone_videoram[offset])
		video_dirty[offset] = 1;
}



void zerozone_vh_stop(void)
{
	free(video_dirty);
	video_dirty = NULL;
}

int zerozone_vh_start(void)
{
	video_dirty = malloc(videoram_size/2);

	if (!video_dirty)
	{
		zerozone_vh_stop();
		return 1;
	}

	memset(video_dirty,1,videoram_size/2);

	return 0;
}

void zerozone_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs;

	if (full_refresh)
		memset(video_dirty,1,videoram_size/2);

	for (offs = 0;offs < videoram_size/2;offs++)
	{
		if (video_dirty[offs])
		{
			int sx,sy;
			int tile, color;

			tile = zerozone_videoram[offs] & 0xfff;
			color = (zerozone_videoram[offs] & 0xf000) >> 12;

			video_dirty[offs] = 0;

			sx = offs / 32;
			sy = offs % 32;

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
}
