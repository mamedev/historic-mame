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
		dirtybuffer[offset] = 1;
}



VIDEO_UPDATE( zerozone )
{
	int offs;

	if (get_vh_global_attribute_changed())
		memset(video_dirty,1,videoram_size/2);

	for (offs = 0;offs < videoram_size/2;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int tile, color;

			tile = zerozone_videoram[offs] & 0xfff;
			color = (zerozone_videoram[offs] & 0xf000) >> 12;

			dirtybuffer[offs] = 0;

			sx = offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					tile,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
