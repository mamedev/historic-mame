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

  1942 has three 256x4 palette PROMs (one per gun) and three 256x4 lookup
  table PROMs (one for characters, one for sprites, one for background tiles).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void slapfight_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,j,used;
	unsigned char allocated[3*256];

	memset(palette,0,3 * Machine->drv->total_colors);

	used = 0;
        for (i = 0;i < 256;i++)
	{
                int bit0,bit1,bit2,bit3;

                bit0 = (color_prom[i] >> 0) & 0x01;
                bit1 = (color_prom[i] >> 1) & 0x01;
                bit2 = (color_prom[i] >> 2) & 0x01;
                bit3 = (color_prom[i] >> 3) & 0x01;
                palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
                bit0 = (color_prom[i+512] >> 0) & 0x01;
                bit1 = (color_prom[i+512] >> 1) & 0x01;
                bit2 = (color_prom[i+512] >> 2) & 0x01;
                bit3 = (color_prom[i+512] >> 3) & 0x01;
                palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
                bit0 = (color_prom[i+512*2] >> 0) & 0x01;
                bit1 = (color_prom[i+512*2] >> 1) & 0x01;
                bit2 = (color_prom[i+512*2] >> 2) & 0x01;
                bit3 = (color_prom[i+512*2] >> 3) & 0x01;
                palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

                colortable[i] = i;
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int slapfight_vh_start(void)
{
  if (generic_vh_start() != 0)
    return 1;

  return 0;
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

***************************************************************************/
void slapfight_vh_update(struct osd_bitmap *bitmap)
{

  int offs,offs_scroll;

  /* Draw the background layer */


  for (offs = slapfight_bg_ram_size - 1;offs >= 0;offs--)
  {
      int sx,sy;

      sx = (offs % 64);
      sy = (offs / 64);

      if (flipscreen)
      {
        sx = 63 - sx;
        sy = 31 - sy;
      }

/* BG Ram decode bytes

   Tile Word  BG_RAM1 Tile7  Tile6  Tile5  Tile4  Tile3  Tile2  Tile1  Tile0
              BG_RAM2 ?????  ?????  ?????  ?????  Tile11 Tile10 Tile9  Tile8

*/
	  /* We on only draw the X axis from 0x00-0x24 the rest is wasted or so it seems */

//          if(sx < 0x25)
	  {
              offs_scroll=offs+slapfight_scroll_char_x;

              drawgfx(bitmap,Machine->gfx[1],
                      slapfight_bg_ram1[offs_scroll]+((slapfight_bg_ram2[offs_scroll]&0x0f)<<8) ,
                      0,
                      flipscreen,flipscreen,
                      (8*sx)-slapfight_scroll_pixel_x,8*sy,
                      &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	  }
  }

  /* Draw the sprite layer */

  /* Byte 0 : NNNN NNNN   Number of tile
     Byte 1 : XXXX XXXX   X
     Byte 2 : NN?? ???O   Hi-Number + Sprite On/Off
     Byte 3 : YYYY YYYY   Y */

  for (offs = 0 ; offs<spriteram_size ;  offs+=4)
  {
     if (!(spriteram[offs+2]&1)) /* Not sure about it */
     {
        drawgfx(bitmap,Machine->gfx[2],
          spriteram[offs] + ((spriteram[offs+2]&0xC0)<<2),
          0,    /* color not found*/
          0,0,  /* flip not found*/
          spriteram[offs+1],spriteram[offs+3],
          &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
     }
  }

  /* draw the frontmost playfield. They are characters, but draw them as sprites */

  for (offs = videoram_size - 1;offs >= 0;offs--)
  {
      int sx,sy;

      sx = offs % 64;
      sy = offs / 64;

      if (flipscreen)
      {
        sx = 63 - sx;
        sy = 31 - sy;
      }

      /* We on only draw the X axis from 0x00-0x24 the rest is wasted or so it seems */

      if(sx < 0x25)
      {
         drawgfx(bitmap,Machine->gfx[0],
           videoram[offs] + ((colorram[offs]&0x01)?0x100:0) ,
           colorram[offs] & 0x7f,
           flipscreen,flipscreen,
           8*sx,8*sy,
           &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
      }
   }
}
