/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* local */
int atarifb_alphap1_vram_size;
int atarifb_alphap2_vram_size;
unsigned char *atarifb_alphap1_vram;
unsigned char *atarifb_alphap2_vram;
unsigned char *atarifb_scroll_register;
unsigned char *alphap1_dirtybuffer;
unsigned char *alphap2_dirtybuffer;

void atarifb_alphap1_vram_w(int offset,int data);
void atarifb_alphap2_vram_w(int offset,int data);
void atarifb_scroll_w(int offset,int data);
void atarifb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

struct rectangle bigfield_area = {  4*8, 34*8-1, 0*8, 32*8-1 };
struct rectangle left_area =     {  0*8,  3*8-1, 0*8, 32*8-1 };
struct rectangle right_area =    { 34*8, 38*8-1, 0*8, 32*8-1 };

/***************************************************************************
***************************************************************************/
void atarifb_alphap1_vram_w(int offset,int data)
{
	if (atarifb_alphap1_vram[offset] != data)
	{
		atarifb_alphap1_vram[offset] = data;

		alphap1_dirtybuffer[offset] = 1;
	}
}

void atarifb_alphap2_vram_w(int offset,int data)
{
	if (atarifb_alphap2_vram[offset] != data)
	{
		atarifb_alphap2_vram[offset] = data;

		alphap2_dirtybuffer[offset] = 1;
	}
}

/***************************************************************************
***************************************************************************/
void atarifb_scroll_w(int offset,int data)
{
	if (data - 8 != *atarifb_scroll_register)
	{
		*atarifb_scroll_register = data - 8;
		memset(dirtybuffer,1,videoram_size);
	}
}

/***************************************************************************
***************************************************************************/

int atarifb_vh_start(void)
{
	if (generic_vh_start()!=0)
		return 1;

	alphap1_dirtybuffer = malloc (atarifb_alphap1_vram_size);
	alphap2_dirtybuffer = malloc (atarifb_alphap2_vram_size);
	if ((!alphap1_dirtybuffer) || (!alphap2_dirtybuffer))
	{
		generic_vh_stop();
		return 1;
	}

	memset(alphap1_dirtybuffer, 1, atarifb_alphap1_vram_size);
	memset(alphap2_dirtybuffer, 1, atarifb_alphap2_vram_size);
	memset(dirtybuffer, 1, videoram_size);

	return 0;
}

/***************************************************************************
***************************************************************************/

void atarifb_vh_stop(void)
{
	generic_vh_stop();
	free (alphap1_dirtybuffer);
	free (alphap2_dirtybuffer);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void atarifb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,obj;

	if (full_refresh)
	{
		memset(alphap1_dirtybuffer, 1, atarifb_alphap1_vram_size);
		memset(alphap2_dirtybuffer, 1, atarifb_alphap2_vram_size);
		memset(dirtybuffer,1,videoram_size);
	}

	/* for every character in the Player 1 Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = atarifb_alphap1_vram_size - 1;offs >= 0;offs--)
	{
		if (alphap1_dirtybuffer[offs])
		{
			int charcode;
			int flipbit;
			int disable;
			int sx,sy;

			alphap1_dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32) + 35*8;
			sy = 8 * (offs % 32) + 8;

			charcode = atarifb_alphap1_vram[offs] & 0x3F;
			flipbit = (atarifb_alphap1_vram[offs] & 0x40) >> 6;
			disable = (atarifb_alphap1_vram[offs] & 0x80) >> 7;

			if (!disable)
			{
				drawgfx(bitmap,Machine->gfx[0],
					charcode, 0,
					flipbit,flipbit,sx,sy,
					&right_area,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* for every character in the Player 2 Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = atarifb_alphap2_vram_size - 1;offs >= 0;offs--)
	{
		if (alphap2_dirtybuffer[offs])
		{
			int charcode;
			int flipbit;
			int disable;
			int sx,sy;

			alphap2_dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32);
			sy = 8 * (offs % 32) + 8;

			charcode = atarifb_alphap2_vram[offs] & 0x3F;
			flipbit = (atarifb_alphap2_vram[offs] & 0x40) >> 6;
			disable = (atarifb_alphap2_vram[offs] & 0x80) >> 7;

			if (!disable)
			{
				drawgfx(bitmap,Machine->gfx[0],
					charcode, 0,
					flipbit,flipbit,sx,sy,
					&left_area,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int charcode;
			int flipx,flipy;
			int sx,sy;

			dirtybuffer[offs]=0;

			charcode = videoram[offs] & 0x3F;
			flipx = (videoram[offs] & 0x40) >> 6;
			flipy = (videoram[offs] & 0x80) >> 7;

			sx = (8 * (offs % 32) - *atarifb_scroll_register);
			sy = 8 * (offs / 32) + 8;
			if (sx < 0) sx += 256;
			if (sx > 255) sx -= 256;

			drawgfx(tmpbitmap,Machine->gfx[1],
					charcode, 0,
					flipx,flipy,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,8*3,0,&bigfield_area,TRANSPARENCY_NONE,0);

	/* Draw our motion objects */
	for (obj=0;obj<16;obj++)
	{
		int charcode;
		int flipx,flipy;
		int sx,sy;

		sy = spriteram[obj*2 + 1];
		if (sy == 0) continue;

		charcode = spriteram[obj*2] & 0x3F;
		flipx = (spriteram[obj*2] & 0x40);
		flipy = (spriteram[obj*2] & 0x80);
		sx = spriteram[obj*2 + 0x20] + 8*3;

		drawgfx(bitmap,Machine->gfx[1],
				charcode, 0,
				flipx,flipy,sx,sy,
				&bigfield_area,TRANSPARENCY_PEN,0);

		/* The down markers are multiplexed by altering the y location during */
		/* mid-screen. We'll fake it by essentially doing the same thing here. */
		if ((charcode == 0x11) && (sy == 0xf8))
		{
			sy = 0; /* When multiplexed, it's 0x10...why? */
			drawgfx(bitmap,Machine->gfx[1],
				charcode, 0,
				flipx,flipy,sx,sy,
				&bigfield_area,TRANSPARENCY_PEN,0);
		}
	}

{
	int x;
	char buf[20];
	struct osd_bitmap *mybitmap = Machine->scrbitmap;
extern int atarifb_lamp1, atarifb_lamp2;

	switch (atarifb_lamp1)
	{
		case 0x00:
			sprintf (buf, "          ");
			break;
		case 0x01:
			sprintf (buf, "SWEEP     ");
			break;
		case 0x02:
			sprintf (buf, "KEEPER    ");
			break;
		case 0x04:
			sprintf (buf, "BOMB      ");
			break;
		case 0x08:
			sprintf (buf, "DOWN & OUT");
			break;
	}
	for (x = 0;x < 10;x++)
		drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x + 30*8,0,0,TRANSPARENCY_NONE,0);

	switch (atarifb_lamp2)
	{
		case 0x00:
			sprintf (buf, "          ");
			break;
		case 0x01:
			sprintf (buf, "SWEEP     ");
			break;
		case 0x02:
			sprintf (buf, "KEEPER    ");
			break;
		case 0x04:
			sprintf (buf, "BOMB      ");
			break;
		case 0x08:
			sprintf (buf, "DOWN & OUT");
			break;
	}
	for (x = 0;x < 10;x++)
		drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x,0,0,TRANSPARENCY_NONE,0);
}

#if 0
{
	int x,x2;
	char buf[20];
	int trueorientation;
	struct osd_bitmap *mybitmap = Machine->scrbitmap;
extern int atarifb_lamp1, atarifb_lamp2;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	x2 = 0;
	for (x2 = 0;x2 < 1;x2 ++)
	{
		sprintf(buf,"%02X %02X", atarifb_lamp1, atarifb_lamp2);
		for (x = 0;x < 5;x++)
			drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x + 16*x2,0,0,TRANSPARENCY_NONE,0);
	}
}
#endif
}

/* The only difference is the play naming */
void atarifb4_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,obj;

	if (full_refresh)
	{
		memset(alphap1_dirtybuffer, 1, atarifb_alphap1_vram_size);
		memset(alphap2_dirtybuffer, 1, atarifb_alphap2_vram_size);
		memset(dirtybuffer,1,videoram_size);
	}

	/* for every character in the Player 1 Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = atarifb_alphap1_vram_size - 1;offs >= 0;offs--)
	{
		if (alphap1_dirtybuffer[offs])
		{
			int charcode;
			int flipbit;
			int disable;
			int sx,sy;

			alphap1_dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32) + 35*8;
			sy = 8 * (offs % 32) + 8;

			charcode = atarifb_alphap1_vram[offs] & 0x3F;
			flipbit = (atarifb_alphap1_vram[offs] & 0x40) >> 6;
			disable = (atarifb_alphap1_vram[offs] & 0x80) >> 7;

			if (!disable)
			{
				drawgfx(bitmap,Machine->gfx[0],
					charcode, 0,
					flipbit,flipbit,sx,sy,
					&right_area,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* for every character in the Player 2 Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = atarifb_alphap2_vram_size - 1;offs >= 0;offs--)
	{
		if (alphap2_dirtybuffer[offs])
		{
			int charcode;
			int flipbit;
			int disable;
			int sx,sy;

			alphap2_dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32);
			sy = 8 * (offs % 32) + 8;

			charcode = atarifb_alphap2_vram[offs] & 0x3F;
			flipbit = (atarifb_alphap2_vram[offs] & 0x40) >> 6;
			disable = (atarifb_alphap2_vram[offs] & 0x80) >> 7;

			if (!disable)
			{
				drawgfx(bitmap,Machine->gfx[0],
					charcode, 0,
					flipbit,flipbit,sx,sy,
					&left_area,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int charcode;
			int flipx,flipy;
			int sx,sy;

			dirtybuffer[offs]=0;

			charcode = videoram[offs] & 0x3F;
			flipx = (videoram[offs] & 0x40) >> 6;
			flipy = (videoram[offs] & 0x80) >> 7;

			sx = (8 * (offs % 32) - *atarifb_scroll_register);
			sy = 8 * (offs / 32) + 8;
			if (sx < 0) sx += 256;
			if (sx > 255) sx -= 256;

			drawgfx(tmpbitmap,Machine->gfx[1],
					charcode, 0,
					flipx,flipy,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,8*3,0,&bigfield_area,TRANSPARENCY_NONE,0);

	/* Draw our motion objects */
	for (obj=0;obj<16;obj++)
	{
		int charcode;
		int flipx,flipy;
		int sx,sy;

		sy = spriteram[obj*2 + 1];
		if (sy == 0) continue;

		charcode = spriteram[obj*2] & 0x3F;
		flipx = (spriteram[obj*2] & 0x40);
		flipy = (spriteram[obj*2] & 0x80);
		sx = spriteram[obj*2 + 0x20] + 8*3;

		drawgfx(bitmap,Machine->gfx[1],
				charcode, 0,
				flipx,flipy,sx,sy,
				&bigfield_area,TRANSPARENCY_PEN,0);

		/* The down markers are multiplexed by altering the y location during */
		/* mid-screen. We'll fake it by essentially doing the same thing here. */
		if ((charcode == 0x11) && (sy == 0xf8))
		{
			sy = 0; /* When multiplexed, it's 0x10...why? */
			drawgfx(bitmap,Machine->gfx[1],
				charcode, 0,
				flipx,flipy,sx,sy,
				&bigfield_area,TRANSPARENCY_PEN,0);
		}
	}

{
	int x;
	char buf[20];
	struct osd_bitmap *mybitmap = Machine->scrbitmap;
extern int atarifb_lamp1, atarifb_lamp2;

	switch (atarifb_lamp1 & 0x1f)
	{
		case 0x01:
			sprintf (buf, "SLANT OUT ");
			break;
		case 0x02:
			sprintf (buf, "SLANT IN  ");
			break;
		case 0x04:
			sprintf (buf, "BOMB      ");
			break;
		case 0x08:
			sprintf (buf, "DOWN & OUT");
			break;
		case 0x10:
			sprintf (buf, "KICK      ");
			break;
		default:
			sprintf (buf, "          ");
			break;
	}
	for (x = 0;x < 10;x++)
		drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x + 30*8,0,0,TRANSPARENCY_NONE,0);

	switch (atarifb_lamp2 & 0x1f)
	{
		case 0x01:
			sprintf (buf, "SLANT OUT ");
			break;
		case 0x02:
			sprintf (buf, "SLANT IN  ");
			break;
		case 0x04:
			sprintf (buf, "BOMB      ");
			break;
		case 0x08:
			sprintf (buf, "DOWN & OUT");
			break;
		case 0x10:
			sprintf (buf, "KICK      ");
			break;
		default:
			sprintf (buf, "          ");
			break;
	}
	for (x = 0;x < 10;x++)
		drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x,0,0,TRANSPARENCY_NONE,0);
}

#if 0
{
	int x,x2;
	char buf[20];
	int trueorientation;
	struct osd_bitmap *mybitmap = Machine->scrbitmap;
extern int atarifb_lamp1, atarifb_lamp2;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	x2 = 0;
	for (x2 = 0;x2 < 1;x2 ++)
	{
		sprintf(buf,"%02X %02X", atarifb_lamp1, atarifb_lamp2);
		for (x = 0;x < 5;x++)
			drawgfx(bitmap,Machine->uifont,buf[x],DT_COLOR_WHITE,0,0,6*x + 16*x2,0,0,TRANSPARENCY_NONE,0);
	}
}
#endif
}


