/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *invaders_videoram;

/* palette colors (see drivers/invaders.c) */
enum { BLACK, RED, GREEN, YELLOW, WHITE, CYAN, PURPLE };



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int invaders_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void invaders_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void invaders_videoram_w(int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;


		invaders_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[WHITE];
			if (y >= 184 && y < 240) col = Machine->pens[GREEN];
			if (y >= 240 && x > 32 && x < 150) col = Machine->pens[GREEN];
			if (y >= 32 && y < 64) col = Machine->pens[RED];

			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->pens[BLACK];

			osd_mark_dirty(x,y,x,y,0);      /* ASG 971015 */

			y++;
			data <<= 1;
		}
	}
}


void invrvnge_videoram_w(int offset,int data)   /* V.V */ /* Whole function */
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;


		invaders_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[WHITE];
			if (y >= 184) col = Machine->pens[GREEN];
			if (y > 32 && y < 64) col = Machine->pens[RED];

			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->pens[BLACK];

			osd_mark_dirty(x,y,x,y,0);      /* ASG 971015 */

			y++;
			data <<= 1;
		}
	}
}



void lrescue_videoram_w(int offset,int data)    /* V.V */ /* Whole function */
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;


		invaders_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[WHITE];

			if (y >= 8 && y < 16) {
				if (x < 88) col = Machine->pens[CYAN];
				if (x >= 88 && x < 168) col = Machine->pens[RED];
				if (x >= 168) col = Machine->pens[YELLOW];
				}


			if (y >= 16 && y < 24) {
				if (x < 88) col = Machine->pens[CYAN];
				if (x >= 88 && x < 176) col = Machine->pens[GREEN];
				if (x >= 176) col = Machine->pens[YELLOW];
				}


			if (y >= 24 && y < 32) {
				if (x >= 88 && x < 176) col = Machine->pens[GREEN];     /* or 168? */
				if (x >= 176) col = Machine->pens[YELLOW];
				}

			if (y >= 32 && y < 40) col = Machine->pens[RED];
			if (y >= 40 && y < 64) col = Machine->pens[PURPLE];
			if (y >= 64 && y < 96) col = Machine->pens[GREEN];
			if (y >= 96 && y < 128) col = Machine->pens[CYAN];
			if (y >= 128 && y < 160) col = Machine->pens[PURPLE];
			if (y >= 160 && y < 192) col = Machine->pens[YELLOW];
			if (y >= 192 && y < 216) col = Machine->pens[RED];
			if (y >= 216 && y < 232) col = Machine->pens[CYAN];
			if (y >= 232 && y < 240) col = Machine->pens[RED];
			if (y >= 240) {
				if (x < 152) col = Machine->pens[CYAN];
				if (x >= 152 && x < 208) col = Machine->pens[PURPLE];
				if (x >= 208) col = Machine->pens[CYAN];
				}
			if (x == 239) col = Machine->pens[BLACK];

			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->pens[BLACK];

			osd_mark_dirty(x,y,x,y,0);      /* ASG 971015 */

			y++;
			data <<= 1;
		}
	}
}



void rollingc_videoram_w(int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;


		invaders_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[RAM[0xa400 + offset] & 0x0f];
			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->pens[17];

			osd_mark_dirty(x,y,x,y,0);      /* ASG 971015 */

			y++;
			data <<= 1;
		}
	}
}





/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
