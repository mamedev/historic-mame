/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *galaxian_attributesram;

static struct rectangle spritevisiblearea =
{
      2*8, 32*8-1,
      2*8, 30*8-1
};

static struct rectangle spritevisibleareaflipx =
{
        0*8, 30*8-1,
        2*8, 30*8-1
};

static unsigned char character_bank[2];
static int flipscreen[2];

/***************************************************************************

  Convert the color PROMs into a more useable format.

  I'm assuming 1942 resistor values

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void fastfred_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
        int i;
        #define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
        #define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


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


        /* characters and sprites use the same palette */
        for (i = 0;i < TOTAL_COLORS(0);i++)
        {
                COLOR(0,i) = i;
        }
}


void fastfred_character_bank_select_w (int offset, int data)
{
    if (character_bank[offset] != data)
    {
        character_bank[offset] = data;

        memset(dirtybuffer, 1, videoram_size);
    }
}


void fastfred_flipx_w(int offset,int data)
{
        if (flipscreen[0] != (data & 1))
        {
                flipscreen[0] = data & 1;
                memset(dirtybuffer,1,videoram_size);
        }
}

void fastfred_flipy_w(int offset,int data)
{
        if (flipscreen[1] != (data & 1))
        {
                flipscreen[1] = data & 1;
                memset(dirtybuffer,1,videoram_size);
        }
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void fastfred_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int offs, bank;

        bank = 1 + (((character_bank[1] & 0x01) << 1) | (character_bank[0] & 0x01));

        for (offs = videoram_size - 1;offs >= 0;offs--)
        {
                if (dirtybuffer[offs])
                {
                        int sx,sy;

                        dirtybuffer[offs] = 0;

                        sx = offs % 32;
                        sy = offs / 32;

                        if (flipscreen[0]) sx = 31 - sx;
                        if (flipscreen[1]) sy = 31 - sy;

                        // Background seem to be a single color
                        drawgfx(tmpbitmap,Machine->gfx[bank],
                                videoram[offs],
                                galaxian_attributesram[2 * (offs % 32) + 1] & 0x07,
                                flipscreen[0],flipscreen[1],
                                8*sx,8*sy,
                                0,TRANSPARENCY_NONE,0);
                }
        }

        /* copy the temporary bitmap to the screen */
        {
                int i, scroll[32];


                if (flipscreen[0])
                {
                        for (i = 0;i < 32;i++)
                        {
                                scroll[31-i] = -galaxian_attributesram[2 * i];
                                if (flipscreen[1]) scroll[31-i] = -scroll[31-i];
                        }
                }
                else
                {
                        for (i = 0;i < 32;i++)
                        {
                                scroll[i] = -galaxian_attributesram[2 * i];
                                if (flipscreen[1]) scroll[i] = -scroll[i];
                        }
                }

                copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }


        /* Draw the sprites */
        for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
        {
                int flipy,sx,sy;

                sx = (spriteram[offs + 3] + 1) & 0xff;  /* ??? */
                sy = 240 - spriteram[offs];
                flipy = ~spriteram[offs + 1] & 0x80;

                if (flipscreen[0])
                {
                        sx = 241 - sx;  /* note: 241, not 240 (this is correct in Amidar, at least) */
                }
                if (flipscreen[1])
                {
                        sy = 240 - sy;
                        flipy = !flipy;
                }

                drawgfx(bitmap,Machine->gfx[0],
                                spriteram[offs + 1] & 0x7f,
                                spriteram[offs + 2] & 0x07,
                                flipscreen[0],flipy,
                                sx,sy,
                                flipscreen[0] ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
        }
}
