
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Todo:
  Scroll 2 to scroll 3 priority (not quite right in Strider).
  Layer-sprite priority (almost there... masking not correct)
  Loads of unknown attribute bits on scroll 2.
  Entire unknown graphic RAM region
  Final fight 32x32 tiles are not linear.
  Speed, speed, speed...

  OUTPUT PORTS (preliminary)
  0x00-0x01     OBJ RAM base (/256)
  0x02-0x03     Scroll1 RAM base (/256)
  0x04-0x05     Scroll2 RAM base (/256)
  0x06-0x07     Scroll3 RAM base (/256)
  0x08-0x09     ??? Base address of something (this could be important) ???
  0x0a-0x0b     Palette base (/256)
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

  0x4e-0x4f     ????
  0x5e-0x5f     ????

  0x66-0x67     Video control register (location varies according to game)
  0x6a-0x6b     ????
  0x6c-0x6d     ????
  0x6e-0x6f     ????
  0x70-0x72     ????


  0x80-0x81     ????            Sound output.
  0x88-0x89     ????            Port thingy (sound fade???)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/cps1.h"

#include "osdepend.h"

/* Public variables */
unsigned char *cps1_gfxram;
unsigned char *cps1_output;
int cps1_gfxram_size;
int cps1_output_size;

/* Private */
const int cps1_scroll1_size=0x4000;
const int cps1_scroll2_size=0x4000;
const int cps1_scroll3_size=0x4000;
const int cps1_obj_size    =0x4000;
const int cps1_unknown_size=0x4000;
const int cps1_palette_size=0x1000;

static unsigned char *cps1_scroll1;
static unsigned char *cps1_scroll2;
static unsigned char *cps1_scroll3;
static unsigned char *cps1_obj;
static unsigned char *cps1_palette;
static unsigned char *cps1_unknown;
static unsigned char *cps1_old_palette;

int scroll1x, scroll1y, scroll2x, scroll2y, scroll3x, scroll3y;
struct CPS1config *cps1_game_config;

/* Output ports */
#define CPS1_OBJ_BASE           0x00    /* Base address of objects */
#define CPS1_SCROLL1_BASE       0x02    /* Base address of scroll 1 */
#define CPS1_SCROLL2_BASE       0x04    /* Base address of scroll 2 */
#define CPS1_SCROLL3_BASE       0x06    /* Base address of scroll 3 */
#define CPS1_UNKNOWN_BASE       0x08    /* Base address of unknown video */
#define CPS1_PALETTE_BASE       0x0a    /* Base address of palette */
#define CPS1_SCROLL1_SCROLLX    0x0c    /* Scroll 1 X */
#define CPS1_SCROLL1_SCROLLY    0x0e    /* Scroll 1 Y */
#define CPS1_SCROLL2_SCROLLX    0x10    /* Scroll 2 X */
#define CPS1_SCROLL2_SCROLLY    0x12    /* Scroll 2 Y */
#define CPS1_SCROLL3_SCROLLX    0x14    /* Scroll 3 X */
#define CPS1_SCROLL3_SCROLLY    0x16    /* Scroll 3 Y */

#define CPS1_LAYER_CONTROL      0x66    /* (or maybe not) */

inline int cps1_port(int offset)
{
        return READ_WORD(&cps1_output[offset]);
}

inline unsigned char * cps1_base(int offset)
{
        int base=cps1_port(offset)*256;
        return &cps1_gfxram[base&0x3ffff];
}

#ifdef MAME_DEBUG
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

        fp=fopen("UNKNOWN.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_unknown, cps1_unknown_size, 1, fp);
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
#endif


inline void cps1_get_video_base(void)
{
        /* Re-calculate the VIDEO RAM base */
        cps1_scroll1=cps1_base(CPS1_SCROLL1_BASE);
        cps1_scroll2=cps1_base(CPS1_SCROLL2_BASE);
        cps1_scroll3=cps1_base(CPS1_SCROLL3_BASE);
        cps1_obj    =cps1_base(CPS1_OBJ_BASE);
        cps1_palette=cps1_base(CPS1_PALETTE_BASE);
        cps1_unknown=cps1_base(CPS1_UNKNOWN_BASE);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cps1_vh_start(void)
{
        cps1_old_palette=(unsigned char *)malloc(cps1_palette_size);
        if (!cps1_old_palette)
        {
                return -1;
        }
        memset(cps1_old_palette, 0xff, cps1_palette_size);

        memset(cps1_gfxram, 0, cps1_gfxram_size);   /* Clear GFX RAM */
        memset(cps1_output, 0, cps1_output_size);   /* Clear output ports */

        cps1_get_video_base();   /* Calculate base pointers */

        /*
           Some games interrogate a couple of registers on bootup.
           These are CPS1 board B self test checks.
        */
        WRITE_WORD(&cps1_output[0x40], 0x406);  /* Carrier Air Wing */
        WRITE_WORD(&cps1_output[0x60], 0x004);  /* Final fight */
        WRITE_WORD(&cps1_output[0x72], 0x800);  /* 3 Wonders */
        WRITE_WORD(&cps1_output[0x4e], 0x405);  /* Nemo */
        WRITE_WORD(&cps1_output[0x5e], 0x404);  /* Mega twins */

        /*
            Note: Carrier air wing does not work because it writes
                  over this value before it reads it.
        */

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cps1_vh_stop(void)
{
        if (cps1_old_palette)
                free(cps1_old_palette);
}

/***************************************************************************

  Build palette from palette RAM

  12 bit RGB with a 4 bit brightness value.

***************************************************************************/

inline void cps1_build_palette(void)
{
                          /* Chars  Sprites   16*16   32*32 */
        static int start[4]={0x0400, 0x0000,  0x0800, 0x0c00};
        static int count[4]={0x0200, 0x0200,  0x0200, 0x0200};

        int j, i;
        /* rebuild the colour lookup table from RAM palette */
        for (j=0; j<4; j++)
	{
              int max=count[j];
              int offset=start[j];

              for (i=0; i<max; i++)
              {
                    int palette=READ_WORD(&cps1_palette[offset]);
                    if (palette != READ_WORD(&cps1_old_palette[offset]) )
                    {
                        int red, green, blue, bright;
                        bright= (palette>>12);
                        red   = ((palette>>8)&0x0f);
                        green = ((palette>>4)&0x0f);
                        blue  = (palette&0x0f);
                        red   = (red * bright ) + red ;
                        green = (green * bright) + green ;
                        blue  = (blue * bright) + blue ;

                        setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
                        WRITE_WORD(&cps1_old_palette[offset], palette);
                    }
                    offset+=2;
              }
        }
}

/***************************************************************************

  Scroll 1 (8x8)

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

inline void cps1_render_scroll1(struct osd_bitmap *bitmap)
{
        int x,y, offs, offsx, sx, sy;
        int base_scroll1=cps1_game_config->base_scroll1;
        int space_char=cps1_game_config->space_scroll1;

        int scrlxrough=(scroll1x>>3)+8;
        int scrlyrough=(scroll1y>>3);

        sx=-(scroll1x&0x07);

        for (x=0; x<0x34; x++)
        {
             sy=-(scroll1y&0x07);
             offsx=(scrlxrough+x)*0x80;
             offsx&=0x1fff;

             for (y=0; y<0x20; y++)
             {
                 int code, offsy;
                 int n=scrlyrough+y;
                 offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
                 offs=offsy+offsx;
                 offs &= 0x3fff;

                 code=READ_WORD(&cps1_scroll1[offs])&0x0fff;

                 if (code >= base_scroll1 && code != space_char)
                 {
                       /* Optimization: only draw non-spaces */
                       int colour=READ_WORD(&cps1_scroll1[offs+2]);
                       code -= base_scroll1;
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

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080        unknown
  0x0100        X block size (in sprites)
  0x0200        X block size
  0x0400        X block size
  0x0800        X block size
  0x1000        Y block size (in sprites)
  0x2000        Y block size
  0x4000        Y block size
  0x8000        Y block size


***************************************************************************/

inline void cps1_render_sprites(struct osd_bitmap *bitmap)
{
        int i;
        int base_obj=cps1_game_config->base_obj;
        int obj_size=cps1_obj_size;

        int gng_obj_kludge=cps1_game_config->gng_sprite_kludge;
        int bank;
        if (cps1_game_config->size_obj)
        {
                obj_size=cps1_game_config->size_obj;
        }

        if (gng_obj_kludge)
        {
                /*
                Ghouls and Ghosts splits the sprites over two separate
                graphics layouts */
                bank=4;
        }

        /* Draw the sprites */
        for (i=obj_size-8; i>=0; i-=8)
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
                   if (code >= base_obj)
                   {
                     code -= base_obj;
                     bank=1;
                     if (gng_obj_kludge && code >= 0x01000)
                     {
                         code &= 0xfff;
                         bank=4;
                     }

                     if (colour & 0xff00)
                     {
                           /* handle blocked sprites */
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
                                         drawgfx(bitmap,Machine->gfx[bank],
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
                                         drawgfx(bitmap,Machine->gfx[bank],
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
                                         drawgfx(bitmap,Machine->gfx[bank],
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
                                         drawgfx(bitmap,Machine->gfx[bank],
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
                           drawgfx(bitmap,Machine->gfx[bank],
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
}



/***************************************************************************

  Scroll 2 (16x16 layer)

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

inline void cps1_render_scroll2(struct osd_bitmap *bitmap, int priority)
{
         int base_scroll2=cps1_game_config->base_scroll2;
         int space_char=cps1_game_config->space_scroll2;
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
                        if (code >= base_scroll2 && code != space_char )
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

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000

***************************************************************************/

inline void cps1_render_scroll3(struct osd_bitmap *bitmap, int priority)
{
        int base_scroll3=cps1_game_config->base_scroll3;
        int space_char=cps1_game_config->space_scroll3;
        int sx,sy;
         /* CPS1_SCROLL 3 */
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
                        if (code>=base_scroll3 && code != space_char)
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
                                                case 0x080:
                                                       transp=0xf001;
                                                       break;
                                                case 0x0100:
                                                       transp=0x8000;
                                                       break;
                                                case 0x0180:
                                                       transp=0x8000;
                                                       break;
                                                case 0x0200:
                                                       transp=0x8000;
                                                       break;
                                        }

                                        if (transp)
                                        {
                                            drawgfx(bitmap,Machine->gfx[3],
                                                 code,
                                                colour&0x1f,
                                                colour&0x20,colour&0x40,
                                                32*sx-nxoffset,32*sy-nyoffset,
                                                &Machine->drv->visible_area,
                                                TRANSPARENCY_PENS,transp);
                                        }
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

        Refresh screen

***************************************************************************/

void cps1_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int layercontrolport;
        int layercontrol;
        int scrl1on, scrl2on, scrl3on;
        int scroll1priority;
        int scroll2priority;

        /* Get video memory base registers */
        cps1_get_video_base();

        /* Get scroll values */
        scroll1x=cps1_port(CPS1_SCROLL1_SCROLLX);
        scroll1y=cps1_port(CPS1_SCROLL1_SCROLLY);
        scroll2x=cps1_port(CPS1_SCROLL2_SCROLLX);
        scroll2y=cps1_port(CPS1_SCROLL2_SCROLLY);
        scroll3x=cps1_port(CPS1_SCROLL3_SCROLLX);
        scroll3y=cps1_port(CPS1_SCROLL3_SCROLLY);

        /*
         Welcome to kludgesville... I have no idea how this is supposed
         to work.
        */
        switch (cps1_game_config->alternative)
        {
                /* Wandering video control ports */
                case 0:
                        /* default */
                        layercontrolport=0x66;
                        layercontrol=READ_WORD(&cps1_output[layercontrolport]);
                        /* Layers on / off */
                        scrl1on=1;
                        scrl2on=layercontrol&0x08;      /* Not quite should probably */
                        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

                        scroll2priority=(layercontrol&0x040);
                        break;
                case 1:
                        /* Willow */
                        layercontrolport=0x70;
                        layercontrol=READ_WORD(&cps1_output[layercontrolport]);
                        /* Layers on / off */
                        scrl1on=1;
                        scrl2on=layercontrol&0x08;      /* Not quite should probably */
                        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

                        /* Scroll 2 / 3 priority */
                        scroll2priority=(layercontrol&0x040);
                        break;
                case 2:
                        layercontrolport=0x6e;
                        layercontrol=READ_WORD(&cps1_output[layercontrolport]);
                        /* Layers on / off */
                        scrl1on=1;
                        scrl2on=layercontrol&0x08;      /* Not quite should probably */
                        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

                        /* Scroll 2 / 3 priority */
                        scroll2priority=(layercontrol&0x040);
                        break;

                case 4:
                        /* Strider (completely kludged) */
                        layercontrolport=0x66;
                        layercontrol=READ_WORD(&cps1_output[layercontrolport]);
                        /* Layers on / off */
                        scrl1on=1;
                        scrl2on=layercontrol&0x08;      /* Not quite should probably */
                        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

                        /*
                         Scroll 2 / 3 priority (best kluge, so far)
                         works on level 1 but not the in-between intermissions
                        */
                        scroll2priority=(layercontrol&0x060)==0x40;
                        if (layercontrol == 0x0e4e)
                        {
                            scroll2priority=0;
                        }

                        break;

                case 5: /* Ghouls and Ghosts */
                        layercontrolport=0x66;
                        layercontrol=READ_WORD(&cps1_output[layercontrolport]);

                        /* Layers on / off */
                        scrl1on=layercontrol&0x02;
                        scrl2on=1;
                        scrl3on=1;
                        scroll2priority=(layercontrol&0x060)==0x40;
                        break;

                default:
                        layercontrolport=0;
                        layercontrol=0xffff;
                        scrl1on=1;
                        scrl2on=layercontrol&0x08;      /* Not quite should probably */
                        scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

                        /* Scroll 2 / 3 priority */
                        scroll2priority=1;
                        break;
        }

        /* Build palette */
        cps1_build_palette();

        /* Blank screen */
        fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


        /* Scroll 1 priority */
        scroll1priority=(layercontrol&0x0080);

        /* Scroll 1 priority */
        if (!scroll1priority && scrl1on)
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

        if (scroll1priority && scrl1on)
        {
                cps1_render_scroll1(bitmap);
        }

#if 0
        {
                int i;
                int nReg=layercontrolport;
                int nVal=cps1_port(nReg);
		struct DisplayText dt[2];
                char szBuffer[40];
		int count = 0;

                char *pszPriority="SCROLL 2";
                if (!scroll2priority)
                {
                        pszPriority="SCROLL 3";
                }
                sprintf(szBuffer, "%04x=%04x %s", nReg, nVal, pszPriority);
                dt[0].text = szBuffer;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[0].text)) / 2;
                dt[0].y = (Machine->uiheight - Machine->uifont->height) / 2 + 2;
		dt[1].text = 0;
                displaytext(dt,0);
         }
#endif
}

