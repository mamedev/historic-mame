/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Todo:

  Scroll 2 to scroll 3 priority (not quite right in Strider).
  Layer-sprite priority (almost there... masking not correct)
  Brightness.
  Loads of unknown attribute bits on scroll 2.
  Final fight 32x32 tiles are not linear.
  Speed, speed, speed... (not optimized yet)

  OUTPUT PORTS (preliminary)
  0x00-0x01     OBJ RAM paragraph base (Alternates between Work Ram and obj Ram)
  0x02-0x03     Scroll1 RAM paragraph base
  0x04-0x05     Scroll2 RAM paragraph base
  0x06-0x07     Scroll3 RAM paragraph base
  0x08-0x09     ????? Base of something ????
  0x0a-0x0b     ????? Palette paragraph base ????
  0x0c-0x0d     Scroll 1 X
  0x0e-0x0f     Scroll 1 Y
  0x10-0x11     Scroll 2 X
  0x12-0x13     Scroll 2 Y
  0x14-0x15     Scroll 3 X
  0x16-0x17     Scroll 3 Y

  0x18-0x19     ????
  0x20-0x21     ????
  0x22-0x23     ????
  0x24-0x25     ????


  0x66-0x67     Video control register (location varies according to game)
  0x6a-0x6b     ????
  0x6c-0x6d     ????
  0x6e-0x6f     ????
  0x70-0x72     ????


  0x80-0x81     ????            Port thingy 1
  0x88-0x89     ????            Port thingy 2

  Sprite ram appears to alternate (double buffered)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/cps1.h"

#include "osdepend.h"

int cps1_scroll1_size;
int cps1_scroll2_size;
int cps1_scroll3_size;
int cps1_obj_size;
int cps1_work_ram_size;
int cps1_palette_size;
int cps1_output_size;

unsigned char *cps1_scroll1;
unsigned char *cps1_scroll2;
unsigned char *cps1_scroll3;
unsigned char *cps1_obj;
unsigned char *cps1_work_ram;
unsigned char *cps1_palette;
unsigned char *cps1_output;

static unsigned char *cps1_palette_dirty;

#define SCROLL1_XSCROLL 0x0c
#define SCROLL1_YSCROLL 0x0e
#define SCROLL2_XSCROLL 0x10
#define SCROLL2_YSCROLL 0x12
#define SCROLL3_XSCROLL 0x14
#define SCROLL3_YSCROLL 0x16

#define LAYER_CONTROL   0x66

int cps1_paletteram_size;
unsigned char *cps1_paletteram;

                   /* Chars  Sprites   16*16   32*32 */
static int start[4]={0x0400, 0x0000,  0x0800, 0x0c00};
static int count[4]={0x0200, 0x0200,  0x0200, 0x0200};

int scrollx, scrolly, scroll2x, scroll2y, scroll3x, scroll3y;

struct CPS1config *cps1_game_config;


void cps1_dump_video(void)
{
        FILE *fp;
        fp=fopen("SCROLL1.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_scroll1, cps1_scroll1_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("SCROLL2.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_scroll2, cps1_scroll2_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("SCROLL3.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_scroll3, cps1_scroll3_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("OBJ.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_obj, cps1_obj_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("WORK.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_work_ram, cps1_work_ram_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("PALETTE.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_palette, cps1_palette_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("OUTPUT.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_output, cps1_output_size, 1, fp);
                fclose(fp);
        }
}


int cps1_scroll1_r(int offset)
{
        return READ_WORD(&cps1_scroll1[offset]);
}
void cps1_scroll1_w(int offset, int value)
{
        COMBINE_WORD_MEM(&cps1_scroll1[offset],value);
}

int cps1_scroll2_r(int offset)
{
        return READ_WORD(&cps1_scroll2[offset]);
}
void cps1_scroll2_w(int offset, int value)
{
        COMBINE_WORD_MEM(&cps1_scroll2[offset],value);
}

int cps1_scroll3_r(int offset)
{
        return READ_WORD(&cps1_scroll3[offset]);
}
void cps1_scroll3_w(int offset, int value)
{
        COMBINE_WORD_MEM(&cps1_scroll3[offset],value);
}

int cps1_obj_r(int offset)
{
        return READ_WORD(&cps1_obj[offset]);
}
void cps1_obj_w(int offset, int value)
{
        COMBINE_WORD_MEM(&cps1_obj[offset],value);
}

int cps1_work_ram_r(int offset)
{
        return READ_WORD(&cps1_work_ram[offset]);
}
void cps1_work_ram_w(int offset, int value)
{
        COMBINE_WORD_MEM(&cps1_work_ram[offset],value);
}

int cps1_palette_r(int offset)
{
        return READ_WORD(&cps1_palette[offset]);
}
void cps1_palette_w(int offset, int value)
{
        cps1_palette_dirty[offset]=1;
        COMBINE_WORD_MEM(&cps1_palette[offset],value);
}


void cps1_output_w(int offset, int value)
{
        if (errorlog)
        {
              fprintf(errorlog, "%04x=%04x\n", offset, cps1_output[offset]);
        }
        COMBINE_WORD_MEM(&cps1_output[offset],value);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cps1_vh_start()
{
        int i,j;
        cps1_palette_dirty=(unsigned char *)malloc(cps1_palette_size);
        if (!cps1_palette_dirty)
        {
                return -1;
        }
        memset(cps1_palette_dirty, 0, cps1_palette_size);

        /* Clear areas of memory */
        memset(cps1_obj, 0, cps1_obj_size);
        memset(cps1_output, 0, cps1_output_size);

        /* build a default colour lookup table from RAM palette */
        for (j=0; j<4; j++)
	{
                int offset=start[j];
                for (i=0; i<count[j]; i++)
                {
                      int n=offset&0x0f;
                      int w=(n<<8)+(n<<4)+n;
                      COMBINE_WORD_MEM(&cps1_palette[offset], w);
                      offset+=2;
                }
        }

        /*
                Some games interrogate a couple of registers on bootup.
                These appear to be CPS1 board B self test checks.
         */
        WRITE_WORD(&cps1_output[0x60], 4);     /* Final fight */
        WRITE_WORD(&cps1_output[0x72], 0x800); /* 3 Wonders */
        WRITE_WORD(&cps1_output[0x4e], 0x405);  /* Nemo */
        WRITE_WORD(&cps1_output[0x5e], 0x404);  /* Mega twins */

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cps1_vh_stop(void)
{
        if (cps1_palette_dirty)
                free(cps1_palette_dirty);
}

/***************************************************************************

        Build palette from palette RAM

***************************************************************************/

void cps1_build_palette(void)
{
        int j, i;
        /* rebuild the colour lookup table from RAM palette */
        for (j=0; j<4; j++)
	{
              int max=count[j];
              int offset=start[j];

              for (i=0; i<max; i++)
              {
                    if (cps1_palette_dirty[offset]||cps1_palette_dirty[offset+1])
                    {
                        int red, green, blue, bright;
                        int nPalette=READ_WORD(&cps1_palette[offset]);
                        bright= (nPalette>>12); /* TODO: use this value */
                        red  = ((nPalette>>8)&0x0f);
                        green   = ((nPalette>>4)&0x0f);
                        blue = (nPalette&0x0f);

                        red = (red << 4) + red;
                        green = (green << 4) + green;
                        blue = (blue << 4) + blue;

                        setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
                        cps1_palette_dirty[offset]=0;
                        cps1_palette_dirty[offset+1]=0;
                    }
                    offset+=2;
              }
        }
}

/***************************************************************************

        Scroll 1 (8x8)

***************************************************************************/

void cps1_render_scroll1(struct osd_bitmap *bitmap)
{
        int x,y, offs,  sx, sy;
        int base_scroll1=cps1_game_config->base_scroll1;

        int scrlxrough=(scrollx>>3)+8;
        int scrlyrough=(scrolly>>3);

        sx=-(scrollx&0x07);

        for (x=0; x<0x34; x++)
        {
             sy=-(scrolly&0x07);
             for (y=0; y<0x20; y++)
             {
                 int code;
                 int offsy, offsx;
                 int n=scrlyrough+y;
                 offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
                 offsx=(scrlxrough+x)*0x80;
                 offsx&=0x1fff;
                 offs=offsy+offsx;
                 offs &= 0x3fff;

                 code=READ_WORD(&cps1_scroll1[offs])&0x0fff;

                 if (code >= base_scroll1)
                 {
                        int colour;
                        code -= base_scroll1;

                        colour=READ_WORD(&cps1_scroll1[offs+2]);
                        drawgfx(bitmap,Machine->gfx[0],
                                     code,
                                     colour&0x1f,
                                     colour&0x20,colour&0x40,sx,sy,
                                     &Machine->drv->visible_area,
                                     TRANSPARENCY_PEN,15);
                 }
                 sy+=8;
             }
             sx+=8;
      }
}

/***************************************************************************

        Sprites

***************************************************************************/

void cps1_render_sprites(struct osd_bitmap *bitmap)
{
        int i;
        /* Draw the sprites */
        for (i=cps1_obj_size-8; i>=0; i-=8)
        {
                int x=READ_WORD(&cps1_obj[i]);
                int y=READ_WORD(&cps1_obj[i+2]);
                if (x && y)
                {
                   int code=READ_WORD(&cps1_obj[i+4]);
                   int colour=READ_WORD(&cps1_obj[i+6]);
                   int col=colour&0x1f;
                   x-=0x40;
                   code &= 0x3fff;

                   if (colour & 0xff00)
                   {
                           int nx=(colour & 0x0f00) >> 8;
                           int ny=(colour & 0xf000) >> 12;
                           int nxs, nys;
                           nx++;
                           ny++;

                           if (colour & 0x40)
                           {
                             /* Y flip */
                             if (colour &0x20)
                             {
                               for (nys=0; nys<ny; nys++)
                               {
                                    for (nxs=0; nxs<nx; nxs++)
                                    {
                                         drawgfx(bitmap,Machine->gfx[1],
                                                code+(nx-1)-nxs+0x10*(ny-1-nys),
                                                col&0x1f,
                                                1,1,
                                                x+nxs*16,y+nys*16,
                                                &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                                    }
                               }
                             }
                             else
                             {
                               for (nys=0; nys<ny; nys++)
                               {
                                    for (nxs=0; nxs<nx; nxs++)
                                    {
                                         drawgfx(bitmap,Machine->gfx[1],
                                                code+nxs+0x10*(ny-1-nys),
                                                col&0x1f,
                                                0,1,
                                                x+nxs*16,y+nys*16,
                                                &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                                    }
                               }
                            }

                           }
                           else
                           {
                             if (colour &0x20)
                             {
                               for (nys=0; nys<ny; nys++)
                               {
                                    for (nxs=0; nxs<nx; nxs++)
                                    {
                                         drawgfx(bitmap,Machine->gfx[1],
                                                code+(nx-1)-nxs+0x10*nys,
                                                col&0x1f,
                                                1,0,
                                                x+nxs*16,y+nys*16,
                                                &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                                    }
                               }
                             }
                             else
                             {
                               for (nys=0; nys<ny; nys++)
                               {
                                    for (nxs=0; nxs<nx; nxs++)
                                    {
                                         drawgfx(bitmap,Machine->gfx[1],
                                                code+nxs+0x10*nys,
                                                col&0x1f,
                                                0,0,
                                                x+nxs*16,y+nys*16,
                                                &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                                    }
                               }
                            }
                         }
                  }
                  else
                  {
                        /* Simple case... 1 sprite */
                           drawgfx(bitmap,Machine->gfx[1],
                                       code,
                                       col&0x1f,
                                       colour&0x20,colour&0x40,
                                       x,y,
                                       &Machine->drv->visible_area,
                                       TRANSPARENCY_PEN,15);
                  }

             }
        }
}



/***************************************************************************

        Scroll 2 (16x16 layer)

***************************************************************************/

void cps1_render_scroll2(struct osd_bitmap *bitmap, int priority)
{
         int base_scroll2=cps1_game_config->base_scroll2;
         int sx, sy;
         int nxoffset=scroll2x&0x0f;    /* Smooth X */
         int nyoffset=scroll2y&0x0f;    /* Smooth Y */
         int nx=(scroll2x>>4)+4;        /* Rough X */
         int ny=(scroll2y>>4);          /* Rough Y */

         for (sx=0; sx<0x32/2+1; sx++)
         {
                for (sy=0; sy<0x09*2; sy++)
                {
                        int offsy, offsx, offs, colour, code;
                        int n;
                        n=ny+sy;
                        offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
                        offsx=((nx+sx)*0x040)&0xfff;
                        offs=offsy+offsx;
                        offs &= 0x3fff;

                        code=READ_WORD(&cps1_scroll2[offs]);
                        if (code > base_scroll2)
                        {
                                code -= base_scroll2;
                                colour=READ_WORD(&cps1_scroll2[offs+2]);
                                if (priority)
                                {
                                        int mask=colour & 0x0180;
                                        if (mask)
                                        {
                                             int transp=0;
                                             switch (mask)
                                             {
                                                case 0x80:
                                                        transp=0x8620;
                                                        break;
                                                case 0x0100:
                                                        // 0x8e20
                                                        transp=~0x8620;
//                                                        transp=0x8000;
                                                        break;
                                                case 0x0180:
                                                        transp=0x8000;
                                                        break;
                                             }

                                             drawgfx(bitmap,Machine->gfx[2],
                                                code,
                                                colour&0x1f,
                                                colour&0x20,colour&0x40,
                                                16*sx-nxoffset,16*sy-nyoffset,
                                                &Machine->drv->visible_area,
                                                TRANSPARENCY_PENS,transp);
                                        }
                                }
                                else
                                {
                                       /* Draw entire tile */
                                       drawgfx(bitmap,Machine->gfx[2],
                                                code,
                                                colour&0x1f,
                                                colour&0x20,colour&0x40,
                                                16*sx-nxoffset,16*sy-nyoffset,
                                                &Machine->drv->visible_area,
                                                TRANSPARENCY_PEN,15);
                                }

                        }
              }
       }
}


/***************************************************************************

        Scroll 3 (32x32 layer)

***************************************************************************/

void cps1_render_scroll3(struct osd_bitmap *bitmap, int priority)
{
        int base_scroll3=cps1_game_config->base_scroll3;
        int sx,sy;
         /* SCROLL 3 */
        int nxoffset=scroll3x&0x1f;
        int nyoffset=scroll3y&0x1f;
        int nx=(scroll3x>>5)+2;
        int ny=(scroll3y>>5);

        for (sx=0; sx<0x32/4+1; sx++)
        {
                for (sy=0; sy<0x20/4+1; sy++)
                {
                        int offsy, offsx, offs, colour, code;
                        int n;
                        n=ny+sy;
                        offsy  = ((n&0x07)*4 | ((n&0xf8)*0x0100))&0x3fff;
                        offsx=((nx+sx)*0x020)&0x7ff;
                        offs=offsy+offsx;
                        offs &= 0x3fff;
                        code=READ_WORD(&cps1_scroll3[offs]);
                        if (code>=base_scroll3)
                        {
                            code -= base_scroll3;
                            colour=READ_WORD(&cps1_scroll3[offs+2]);
                            if (priority)
                            {
                                int mask=colour & 0x0180;
                                if (mask)
                                {
                                       int transp=0;
                                       switch (mask)
                                       {
                                                case 0x80:
                                                       transp=0xf001;
                                                       break;
                                                case 0x0100:
                                                       transp=0x8000;
                                                       break;
                                                case 0x0180:
                                                       transp=0x8000;
                                                       break;
                                        }


                                        drawgfx(bitmap,Machine->gfx[3],
                                                 code,
                                                colour&0x1f,
                                                colour&0x20,colour&0x40,
                                                32*sx-nxoffset,32*sy-nyoffset,
                                                &Machine->drv->visible_area,
                                                TRANSPARENCY_PENS,transp);
                               }
                           }
                           else
                           {
                              drawgfx(bitmap,Machine->gfx[3],
                                        code,
                                        colour&0x1f,
                                        colour&0x20,colour&0x40,
                                        32*sx-nxoffset,32*sy-nyoffset,
                                        &Machine->drv->visible_area,
                                        TRANSPARENCY_PEN,15);
                           }
                        }
                }
         }
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void cps1_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int layercontrol;
        int scrl2on, scrl3on;
        int scroll1priority;
        int scroll2priority;        /* Temporary kludge */
        scrollx=READ_WORD(&cps1_output[SCROLL1_XSCROLL]);
        scrolly=READ_WORD(&cps1_output[SCROLL1_YSCROLL]);
        scroll2x=READ_WORD(&cps1_output[SCROLL2_XSCROLL]);
        scroll2y=READ_WORD(&cps1_output[SCROLL2_YSCROLL]);
        scroll3x=READ_WORD(&cps1_output[SCROLL3_XSCROLL]);
        scroll3y=READ_WORD(&cps1_output[SCROLL3_YSCROLL]);

        switch (cps1_game_config->alternative)
        {
                /* Wandering video control ports */
                case 0: layercontrol=READ_WORD(&cps1_output[0x66]);
                        break;
                case 1:
                        layercontrol=READ_WORD(&cps1_output[0x70]);
                        break;
                case 2:
                        layercontrol=READ_WORD(&cps1_output[0x6e]);
                        break;
                default:
                        layercontrol=0xffff;
                        break;

        }
        /* Build palette */
        cps1_build_palette();

        /* Blank screen */
        fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

        /* Layers on / off */
        scrl2on=layercontrol&0x08;      /* Not quite should probably */
        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

        /* Scroll 1 priority */
        scroll1priority=(layercontrol&0x0080);

        /* Scroll 2 / 3 priority */
        scroll2priority=(layercontrol&0x0040);

        /* Scroll 1 priority */
        if (!scroll1priority)
        {
                cps1_render_scroll1(bitmap);
        }

        if (scroll2priority)
        {
                if (scrl3on)
                {
                        cps1_render_scroll3(bitmap, 0);
                }
                if (scrl2on)
                {
                        cps1_render_scroll2(bitmap, 0);
                }
        }
        else
        {
                if (scrl2on)
                {
                        cps1_render_scroll2(bitmap, 0);
                }
                if (scrl3on)
                {
                        cps1_render_scroll3(bitmap, 0);
                }
        }

        cps1_render_sprites(bitmap);

        if (scroll2priority)
        {
                if (scrl3on)
                {
                        cps1_render_scroll3(bitmap, 0x0100);
                }
                if (scrl2on)
                {
                        cps1_render_scroll2(bitmap, 0x0100);
                }
        }
        else
        {
                if (scrl2on)
                {
                        cps1_render_scroll2(bitmap, 0x0100);
                }
                if (scrl3on)
                {
                        cps1_render_scroll3(bitmap, 0x0100);
                }
        }

        if (scroll1priority)
        {
                cps1_render_scroll1(bitmap);
        }
}

