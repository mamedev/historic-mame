/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *exidy_characterram;
unsigned char *exidy_color_lookup;

static unsigned char exidy_dirtycharacter[256];

unsigned char *exidy_sprite_no;
unsigned char *exidy_sprite_enable;
unsigned char *exidy_sprite1_xpos;
unsigned char *exidy_sprite1_ypos;
unsigned char *exidy_sprite2_xpos;
unsigned char *exidy_sprite2_ypos;

static unsigned char exidy_last_state=0;

void exidy_characterram_w(int offset,int data)
{
	if (exidy_characterram[offset] != data)
	{
		exidy_dirtycharacter[offset / 8 % 256] = 1;

		exidy_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;
	unsigned char enable_set=0;

	if ((*exidy_sprite_enable&0x20)==0x20)
		enable_set=1;

	if (exidy_last_state!=enable_set)
	{
		memset(exidy_dirtycharacter,1,sizeof(exidy_dirtycharacter));
		exidy_last_state=enable_set;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || exidy_dirtycharacter[charcode])
		{
			int sx,sy;


		/* decode modified characters */
			if (exidy_dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,exidy_characterram,
						Machine->drv->gfxdecodeinfo[0].gfxlayout);
				exidy_dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);
			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,exidy_color_lookup[charcode]*2+enable_set,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	for (i = 0;i < 256;i++)
	{
		if (exidy_dirtycharacter[i] == 2)
			exidy_dirtycharacter[i] = 0;
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	{
		int sx,sy;

		if (!(*exidy_sprite_enable&0x80) || *exidy_sprite_enable&0x10)
		{
			sx = 236-*exidy_sprite1_xpos-4;
			sy = 244-*exidy_sprite1_ypos-4;

			drawgfx(bitmap,Machine->gfx[1],
				(*exidy_sprite_no & 0x0F)+16*enable_set,0+enable_set,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}

		if (!(*exidy_sprite_enable&0x40))
		{
			sx = 236-*exidy_sprite2_xpos-4;
			sy = 244-*exidy_sprite2_ypos-4;

			drawgfx(bitmap,Machine->gfx[1],
				((*exidy_sprite_no>>4) & 0x0F)+32,2+enable_set,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
