/***************************************************************************

  Video Hardware for Irem Games:
  Lode Runner, Kid Niki, Spelunker

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static unsigned char scroll[2];
static int flipscreen;
static const unsigned char *sprite_height_prom;
static int kidniki_background_bank;
static int irem_background_hscroll;
static int irem_background_vscroll;
static int kidniki_text_vscroll;
static int spelunk2_palbank;

unsigned char *irem_textram;
int irem_textram_size;


static struct rectangle spritevisiblearea =
{
	8*8, (64-8)*8-1,
	1*8, 32*8-1
};
static struct rectangle flipspritevisiblearea =
{
	8*8, (64-8)*8-1,
	0*8, 31*8-1
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
void ldrun_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the sprite height table */

	sprite_height_prom = color_prom;	/* we'll need this at run time */
}

void spelunk2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* chars */
	for (i = 0;i < 512;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*256] >> 0) & 0x01;
		bit1 = (color_prom[2*256] >> 1) & 0x01;
		bit2 = (color_prom[2*256] >> 2) & 0x01;
		bit3 = (color_prom[2*256] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*256;

	/* sprites */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[256] >> 0) & 0x01;
		bit1 = (color_prom[256] >> 1) & 0x01;
		bit2 = (color_prom[256] >> 2) & 0x01;
		bit3 = (color_prom[256] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*256] >> 0) & 0x01;
		bit1 = (color_prom[2*256] >> 1) & 0x01;
		bit2 = (color_prom[2*256] >> 2) & 0x01;
		bit3 = (color_prom[2*256] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*256;


	/* color_prom now points to the beginning of the sprite height table */
	sprite_height_prom = color_prom;	/* we'll need this at run time */
}



static int irem_vh_start( int width, int height )
{
	irem_background_hscroll = 0;
	irem_background_vscroll = 128;

	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(width,height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}

int kidniki_vh_start(void)
{
	return irem_vh_start(512,256);
}

int spelunk2_vh_start(void)
{
	return irem_vh_start(512,512);
}



void ldrun2p_scroll_w(int offset,int data)
{
	scroll[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void irem_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	/* sprites must be drawn in this order to get correct priority */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int i,incr,code,col,flipx,flipy,sx,sy;


		code = spriteram[offs+4] + ((spriteram[offs+5] & 0x07) << 8);
		col = spriteram[offs+0] & 0x1f;
		sx = 256 * (spriteram[offs+7] & 1) + spriteram[offs+6],
		sy = 256+128-15 - (256 * (spriteram[offs+3] & 1) + spriteram[offs+2]),
		flipx = spriteram[offs+5] & 0x40;
		flipy = spriteram[offs+5] & 0x80;

		i = sprite_height_prom[(code >> 5) & 0x1f];
		if (i == 1)	/* double height */
		{
			code &= ~1;
			sy -= 16;
		}
		else if (i == 2)	/* quadruple height */
		{
			i = 3;
			code &= ~3;
			sy -= 3*16;
		}

		if (flipscreen)
		{
			sx = 240 - sx;
			sy = 242 - i*16 - sy;	/* sprites are slightly misplaced by the hardware */
			flipx = !flipx;
			flipy = !flipy;
		}

		if (flipy)
		{
			incr = -1;
			code += i;
		}
		else incr = 1;

		do
		{
			drawgfx(bitmap,Machine->gfx[1],
					code + i * incr,col,
					flipx,flipy,
					sx,sy + 16 * i,
					flipscreen ? &flipspritevisiblearea : &spritevisiblearea,TRANSPARENCY_PEN,0);

			i--;
		} while (i >= 0);
	}
}



void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			flipx = videoram[offs+1] & 0x20;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2),
					videoram[offs+1] & 0x1f,
					flipx,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	irem_draw_sprites(bitmap);
}

/* almost identical but scrolling background, more characters, */
/* no char x flip, and more sprites */
void ldrun2p_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2) + ((videoram[offs+1] & 0x20) << 5),
					videoram[offs+1] & 0x1f,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	{
		int scrollx;

		scrollx = -(scroll[1] + 256 * scroll[0] - 2);

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	irem_draw_sprites(bitmap);
}




void irem_background_hscroll_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_hscroll = (irem_background_hscroll&0xff00)|data;
		break;

		case 1:
		irem_background_hscroll = (irem_background_hscroll&0xff)|(data<<8);
		break;
	}
}

void irem_background_vscroll_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_vscroll = (irem_background_vscroll&0xff00)|data;
		break;

		case 1:
		irem_background_vscroll = (irem_background_vscroll&0xff)|(data<<8);
		break;
	}
}

void kidniki_text_vscroll_w( int offset, int data )
{
	switch( offset ){
		case 0:
		kidniki_text_vscroll = (kidniki_text_vscroll&0xff00)|data;
		break;

		case 1:
		kidniki_text_vscroll = (kidniki_text_vscroll&0xff)|(data<<8);
		break;
	}
}

void kidniki_background_bank_w( int offset, int data )
{
	if (kidniki_background_bank != (data & 1))
	{
		kidniki_background_bank = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



static void kidniki_draw_text(struct osd_bitmap *bitmap)
{
	int offs;
	int scrolly = kidniki_text_vscroll-0x180;


	for (offs = irem_textram_size - 2;offs >= 0;offs -= 2)
	{
		int sx,sy;


		sx = (offs/2) % 32;
		sy = (offs/2) / 32;

		drawgfx(bitmap,Machine->gfx[2],
				irem_textram[offs] + ((irem_textram[offs + 1] & 0xc0) << 2),
				(irem_textram[offs + 1] & 0x1f),
				0, 0,
				12*sx + 64,8*sy - scrolly,
				&Machine->drv->visible_area,TRANSPARENCY_PEN, 0);
	}
}

static void kidniki_draw_background(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xe0) << 3) + (kidniki_background_bank << 11),
					videoram[offs+1] & 0x1f,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	{
		int scrollx = -irem_background_hscroll;
		int scrolly = -irem_background_vscroll - 128;

		copyscrollbitmap(bitmap,tmpbitmap,
				1,&scrollx,
				1,&scrolly,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}



void kidniki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	kidniki_draw_background( bitmap );
	irem_draw_sprites( bitmap );
	kidniki_draw_text( bitmap );
}



void spelunk2_gfxport_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_vscroll_w(0,data);
		break;

		case 1:
		irem_background_hscroll_w(0,data);
		break;

		case 2:
		irem_background_hscroll_w(1,(data&2)>>1);
		irem_background_vscroll_w(1,(data&1));
		if (spelunk2_palbank != ((data & 0x0c) >> 2))
		{
			spelunk2_palbank = (data & 0x0c) >> 2;
			memset(dirtybuffer,1,videoram_size);
		}
		break;
	}
}


static void spelunk2_draw_text(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = irem_textram_size - 2;offs >= 0;offs -= 2)
	{
		int sx,sy;


		sx = (offs/2) % 32;
		sy = (offs/2) / 32;

		drawgfx(bitmap,Machine->gfx[(irem_textram[offs + 1] & 0x10) ? 3 : 2],
				irem_textram[offs],
				(irem_textram[offs + 1] & 0x0f),
				0, 0,
				12*sx + 64,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN, 0);
	}
}

static void spelunk2_draw_background(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xf0) << 4),
					(videoram[offs+1] & 0x0f) + (spelunk2_palbank << 4),
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	{
		int scrollx = -irem_background_hscroll;
		int scrolly = -irem_background_vscroll - 128;

		copyscrollbitmap(bitmap,tmpbitmap,
				1,&scrollx,
				1,&scrolly,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

void spelunk2_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	spelunk2_draw_background( bitmap );
	irem_draw_sprites( bitmap );
	spelunk2_draw_text( bitmap );
}
