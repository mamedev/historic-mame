/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int flipscreen;

extern unsigned char *slapfight_bg_ram1;
extern unsigned char *slapfight_bg_ram2;
extern int slapfight_bg_ram_size;

extern unsigned char *spriteram;
extern int spriteram_size;

extern int slapfight_scroll_pixel_x;
extern int slapfight_scroll_pixel_y;
extern int slapfight_scroll_char_x;
extern int slapfight_scroll_char_y;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Slapfight has three 256x4 palette PROMs (one per gun) all colours for all
  outputs are mapped to the palette directly.

  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void slapfight_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int slapfight_vh_start(void)
{
	if (generic_vh_start() != 0) return 1; else return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void slapfight_vh_stop(void)
{
	generic_vh_stop();
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

  BG Scroll area byte decode
  --------------------------
  Tile Word  BG_RAM1 Tile7  Tile6  Tile5  Tile4  Tile3  Tile2  Tile1  Tile0
             BG_RAM2 Colr7  Colr6  Colr5  Colr4  Tile11 Tile10 Tile9  Tile8

             Lower 4 colour bits are taken from the tile pixel


  Sprite ram byte decode
  ----------------------
  Byte 0 : NNNN NNNN   Number of tile
  Byte 1 : XXXX XXXX   X
  Byte 2 : NNCC CC?O   Hi-Number + Upper 4 colour bits + Sprite On/Off
  Byte 3 : YYYY YYYY   Y

***************************************************************************/
void slapfight_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int addr,scrx,scry,scrollx;

	/* Draw the background layer */

	for (scry = 2 ; scry < 32 ; scry++)
	{
		for (scrx = 0 ; scrx < 0x37 ; scrx++)
		{
			scrollx=(scrx+27+slapfight_scroll_char_x)%64;
//			scrollx=scrx;
			if(flipscreen) addr =((31-(scry-2))*64)+(63-scrollx) ; else addr = ((scry-2)*64)+scrollx;

			drawgfx(bitmap,Machine->gfx[1],
				slapfight_bg_ram1[addr]+((slapfight_bg_ram2[addr]&0x0f)<<8) ,
				(slapfight_bg_ram2[addr]>>4)&0x0f,
				flipscreen,flipscreen,
				(8*scrx)-slapfight_scroll_pixel_x,8*scry,
//				8*scrx,8*scry,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the sprite layer */

	for (offs = 0 ; offs<spriteram_size ;  offs+=4)
	{
		if (!(spriteram[offs+2]&1)) /* Not sure about it */
		{
			drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs] + ((spriteram[offs+2]&0xC0)<<2),
				(spriteram[offs+2]>>1)&0x0f,
				flipscreen,flipscreen,  /* flip not found*/
				spriteram[offs+1]-12,spriteram[offs+3],		/* Mysterious fudge factor sprite offset */
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	/* draw the frontmost playfield. They are characters, but draw them as sprites */

	for (scry = 2 ; scry < 0x20 ; scry++)
	{
		for (scrx = 0 ; scrx < 0x25 ; scrx++)
		{
			if(flipscreen) addr =((31-scry)*64)+(63-scrx) ; else addr = (scry*64)+scrx;

			drawgfx(bitmap,Machine->gfx[0],
				videoram[addr] + ((colorram[addr]&0x03)<<8) ,
				(colorram[addr]&0x7e)>>2,	// Was 0x7f
				flipscreen,flipscreen,
				8*scrx,8*scry,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

// Debug output for scrolling

#if 0
	{
		char debugstr[256];
		int x,i;
		sprintf(debugstr,"Char=%02d Pixel=%02d",slapfight_scroll_char_x,slapfight_scroll_pixel_x);

		x = (Machine->uiwidth - 16*Machine->uifont->width)/2;

		for (i = 0;i < 16;i++)
		{
			drawgfx(bitmap,Machine->uifont,
				(unsigned int)debugstr[i],
				DT_COLOR_WHITE,
				0,0,
				64,64+(i*8),
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
#endif
}
