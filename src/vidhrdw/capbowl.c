/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char capbowl_scanline;
unsigned int  capbowl_scanline_times_16;
unsigned int  capbowl_scanline_times_256;
unsigned int  capbowl_scanline_offseted;

static unsigned char *raw_video_ram; /* To be able to read values back easily */
static unsigned char *line_palette;
static unsigned int  *line_pens;

int capbowl_vh_start(void)
{
  if (generic_vh_start() == 1)
  {
      return 1;
  }

  raw_video_ram = (unsigned char *)malloc(sizeof(unsigned char) * 256 * 256);
  line_palette  = (unsigned char *)malloc(sizeof(unsigned char) * 256 * 16);
  line_pens     = (unsigned int  *)malloc(sizeof(unsigned int ) * 256 * 16);

  if (!raw_video_ram || !line_palette || !line_pens)
  {
      return 1;
  }

  return 0;
}

void capbowl_vh_stop(void)
{
  generic_vh_stop();

  if (raw_video_ram) free(raw_video_ram);
  if (line_palette)  free(line_palette);
  if (line_pens)     free(line_pens);
}


void capbowl_scanline_w(int offset,int data)
{
  capbowl_scanline           = data;
  capbowl_scanline_times_16  = data << 4;
  capbowl_scanline_times_256 = data << 8;
  capbowl_scanline_offseted  = data + 128;
}

#define VID_PTR(offset) raw_video_ram[capbowl_scanline_times_256 + offset]

void capbowl_videoram_w(int offset,int data)
{
  int i;

  VID_PTR(offset) = data;

   if (offset >= 0x20)
   {
     tmpbitmap->line[512 - (offset << 1)][capbowl_scanline_offseted] = line_pens[capbowl_scanline_times_16 + (data >> 4)  ];
     tmpbitmap->line[511 - (offset << 1)][capbowl_scanline_offseted] = line_pens[capbowl_scanline_times_16 + (data & 0x0f)];
   }
   else
   {
     /* Offsets 0-1f are the palette */
     unsigned char *this_palette = &line_palette[capbowl_scanline_times_16];
     unsigned int  *these_pens   = &line_pens[capbowl_scanline_times_16];

     if (offset & 1)
     {
       this_palette[offset >> 1] = (this_palette[offset >> 1] & 0xe0) | ((data & 0xe0) >> 3) | ((data & 0x0c) >> 2);
       these_pens[offset >> 1] = Machine->pens[this_palette[offset >> 1]];
     }
     else
     {
       this_palette[offset >> 1] = (this_palette[offset >> 1] & 0x1f) | ((data & 0x0e) << 4);
       these_pens[offset >> 1] = Machine->pens[this_palette[offset >> 1]];
     }

     /* Remap colors */
     for (i = 32; i < 212; i++)
     {
       int data2 = VID_PTR(i);

       tmpbitmap->line[512 - (i << 1)][capbowl_scanline_offseted] = these_pens[data2 >> 4  ];
       tmpbitmap->line[511 - (i << 1)][capbowl_scanline_offseted] = these_pens[data2 & 0x0f];
     }
   }
}


int capbowl_videoram_r(int offset)
{
  return VID_PTR(offset);
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap)
{
  copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
