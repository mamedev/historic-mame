
/*************************************************************/
/*                                                           */
/* Lazer Command video handler                               */
/*                                                           */
/*************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VRAM_BASE       0x1c00

#define HORZ_RES        32
#define VERT_RES        24
#define HORZ_CHR        8
#define VERT_CHR        10

#define HORZ_BELOW      0x40
#define VERT_RIGHT      0x80

int     lazercmd_vh_start(void)
{
        videoram_size = HORZ_RES * VERT_RES;
        if (generic_vh_start())
                return 1;
        return 0;
}

void    lazercmd_vh_stop(void)
{
        generic_vh_stop();
}

void    lazercmd_vh_screenrefresh(struct osd_bitmap * bitmap)
{
static  int overlay = 0;
int     i;

        if (overlay != (input_port_2_r(0) & 0x80))
        {
                overlay = input_port_2_r(0) & 0x80;
                memset(dirtybuffer, 1, videoram_size);
        }

        /* The first row of characters is invsisible */
        for (i = 1 * HORZ_RES; i < VERT_RES * HORZ_RES; i++)
        {
                if (dirtybuffer[i])
                {
                int     x, y, x0, y0, chr, color = 2;

                        dirtybuffer[i] = 0;

                        x = i % HORZ_RES;
                        y = i / HORZ_RES;

                        if (overlay)
                        {
                                /* left mustard yellow, right jade green */
                                color  = (x < 16) ? 0 : 1;
                                /* but swapped in first and last lines */
                                if ((y < 2) || (y > 22))
                                        color ^= 1;
                        }
                        x *= HORZ_CHR;
                        y *= VERT_CHR;

                        drawgfx(tmpbitmap,
                                Machine->gfx[0],
                                videoram[i] & 0x3f, color,
                                0,0, x,y,
                                &Machine->drv->visible_area,
                                TRANSPARENCY_NONE,0);

                        if (videoram[i] & VERT_RIGHT)
                                for (y0 = 0; y0 < VERT_CHR; y0++)
                                        tmpbitmap->line[y+y0][x+7] = color;
                        if (videoram[i] & HORZ_BELOW)
                                for (x0 = 0; x0 < HORZ_CHR; x0++)
                                        tmpbitmap->line[y+9][x+x0] = color;
                        osd_mark_dirty(x,y,x+HORZ_CHR-1,y+VERT_CHR-1,1);
                }
        }
        copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

/*************************************************************/
/*                                                           */
/* video ram write.                                          */
/*                                                           */
/*************************************************************/
void    lazercmd_videoram_w(int offset, int data)
{
        if (offset >= videoram_size)
                return;
        Machine->memory_region[0][VRAM_BASE + offset] = data;
        if (videoram[offset] != data)
        {
                videoram[offset] = data;
                dirtybuffer[offset] = 1;
        }
}

