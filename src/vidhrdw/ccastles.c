/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"


struct osd_bitmap *sprite_bm;
struct osd_bitmap *maskbitmap;
static int dirtypalette;

static int ax;
static int ay;
static unsigned char xcoor;
static unsigned char ycoor;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Csyrstal Castles doesn't have a color PROM. It uses RAM to dynamically
  create the palette. The resolution is 9 bit (3 bits per gun). The palette
  contains 32 entries, but it is accessed through a memory windows 64 bytes
  long: writing to the first 32 bytes sets the msb of the red component to 0,
  while writing to the last 32 bytes sets it to 1.
  The first 16 entries are used for sprites; the last 16 for the background
  bitmap.

  I don't know the exact values of the resistors between the RAM and the
  RGB output, I assumed the usual ones.
  bit 8 -- inverter -- 220 ohm resistor  -- RED
  bit 7 -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
        -- inverter -- 220 ohm resistor  -- BLUE
        -- inverter -- 470 ohm resistor  -- BLUE
        -- inverter -- 1  kohm resistor  -- BLUE
        -- inverter -- 220 ohm resistor  -- GREEN
        -- inverter -- 470 ohm resistor  -- GREEN
  bit 0 -- inverter -- 1  kohm resistor  -- GREEN

***************************************************************************/
void ccastles_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}


	/* sprites */
	/* we should use colors 0-15, but we swap them with 16-31 to have a black color 0 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i + 16;
	/* background */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = i;
}



void ccastles_paletteram_w(int offset,int data)
{
	int r,g,b;
	int bit0,bit1,bit2;


	/* we swap colors 0-15 with colors 16-31 because color 0 is red and color */
	/* 16 is black. The MS-DOS version looks better with a black color 0. */
	offset ^= 0x10;

	r = (data & 0xC0) >> 6;
	b = (data & 0x38) >> 3;
	g = (data & 0x07);
	/* a write to offset 32-63 means to set the msb of the red component */
	if (offset & 0x20) r += 4;

	/* bits are inverted */
	r = 7-r;
	g = 7-g;
	b = 7-b;

	bit0 = (r >> 0) & 0x01;
	bit1 = (r >> 1) & 0x01;
	bit2 = (r >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (g >> 0) & 0x01;
	bit1 = (g >> 1) & 0x01;
	bit2 = (g >> 2) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (b >> 0) & 0x01;
	bit1 = (b >> 1) & 0x01;
	bit2 = (b >> 2) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	osd_modify_pen(Machine->pens[offset & 0x1f],r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int ccastles_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((maskbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((sprite_bm = osd_create_bitmap(8,16)) == 0)
	{
		osd_free_bitmap(maskbitmap);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ccastles_vh_stop(void)
{
	osd_free_bitmap(sprite_bm);
	osd_free_bitmap(maskbitmap);
	osd_free_bitmap(tmpbitmap);
}



int ccastles_bitmode_r(int offset) {
  int addr;

  addr = (ycoor<<7) | (xcoor>>1);

  if(!ax) {
    if(!RAM[0x9F02]) {
      xcoor++;
    } else {
      xcoor--;
    }
  }

  if(!ay) {
    if(!RAM[0x9F03]) {
      ycoor++;
    } else {
      ycoor--;
    }
  }

  if(xcoor & 0x01) {
    return((RAM[addr] & 0x0F) << 4);
  } else {
    return(RAM[addr] & 0xF0);
  }
}

void ccastles_xy_w(int offset, int data) {
  RAM[offset] = data;

  if(offset == 0x0000) {
    xcoor = data;
  } else {
    ycoor = data;
  }
}

void ccastles_axy_w(int offset, int data) {
  RAM[0x9F00+offset] = data;

  if(offset == 0x0000) {
    if(data) {
      ax = data;
    } else {
      ax = 0x00;
    }
  } else {
    if(data) {
      ay = data;
    } else {
      ay = 0x00;
    }
  }
}

void ccastles_bitmode_w(int offset, int data) {
  int addr;
  int mode;

  RAM[0x0002] = data;
  if(xcoor & 0x01) {
    mode = (data >> 4) & 0x0F;
  } else {
    mode = (data & 0xF0);
  }

  addr = (ycoor<<7) | (xcoor>>1);

  if((addr>0x0BFF) && (addr<0x8000))
  {
    if(xcoor & 0x01) {
      RAM[addr] = (RAM[addr] & 0xF0) | mode;
    } else {
      RAM[addr] = (RAM[addr] & 0x0F) | mode;
    }


	{
		int x,y,j;


		j = 2*(addr - 0xc00);
		x = j%256;
		y = j/256;
		tmpbitmap->line[y][x] = Machine->gfx[1]->colortable[(RAM[addr] & 0xF0) >> 4];
		tmpbitmap->line[y][x+1] = Machine->gfx[1]->colortable[RAM[addr] & 0x0F];

		/* if bit 3 of the pixel is set, background has priority over sprites when */
		/* the sprite has the priority bit set. We use a second bitmap to remember */
		/* which pixels have priority. */
		maskbitmap->line[y][x] = RAM[addr] & 0x80;
		maskbitmap->line[y][x+1] = RAM[addr] & 0x08;
	}
  }


  if(!ax) {
    if(!RAM[0x9F02]) {
      xcoor++;
    } else {
      xcoor--;
    }
  }

  if(!ay) {
    if(!RAM[0x9F03]) {
      ycoor++;
    } else {
      ycoor--;
    }
  }
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ccastles_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;
	int i,j;
	int x,y;
	int spriteaddr;
	int scrollx,scrolly;


	/* if colors have changed, redraw the whole screen. This doesn't happen often, */
	/* so we can do it without affecting performance */
	if (dirtypalette)
	{
		j = 0;

		for(i = 0x0C00;i < 0x8000;i += 2)
		{
			tmpbitmap->line[j/256][j%256] = Machine->gfx[1]->colortable[(RAM[i] & 0xF0) >> 4];
			tmpbitmap->line[j/256][j%256+1] = Machine->gfx[1]->colortable[RAM[i] & 0x0F];
			tmpbitmap->line[j/256][j%256+2] = Machine->gfx[1]->colortable[(RAM[i+1] & 0xF0) >> 4];
			tmpbitmap->line[j/256][j%256+3] = Machine->gfx[1]->colortable[RAM[i+1] & 0x0F];

			j+=4;
		}

		dirtypalette = 0;
	}

	scrollx = 256 - RAM[0x9c80];
	scrolly = 256 - RAM[0x9d00];

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,
		   &Machine->drv->visible_area,
		   TRANSPARENCY_NONE,0);


	if(RAM[0x9F07]) {
	spriteaddr = 0x8F00;
	} else {
	spriteaddr = 0x8E00;
	}


	/* Draw the sprites */
	for (offs = 0x00;offs < 0x100;offs += 0x04)
	{
		/* Get the X and Y coordinates from the MOB RAM */
		x = RAM[spriteaddr+offs+3];
		y = 216 - RAM[spriteaddr+offs+1];

		if (RAM[spriteaddr+offs+2] & 0x80)	/* background can have priority over the sprite */
		{
			clearbitmap(sprite_bm);
			drawgfx(sprite_bm,Machine->gfx[0],
					RAM[spriteaddr+offs],1,
					0,0,
					0,0,
					0,TRANSPARENCY_PEN,7);

			for (j = 0;j < 16;j++)
			{
				if (y + j >= 0)	/* avoid accesses out of the bitmap boundaries */
				{
					for (i = 0;i < 8;i++)
					{
						unsigned char pixa,pixb;


						pixa = sprite_bm->line[j][i];
						pixb = maskbitmap->line[(y+scrolly+j)%232][(x+scrollx+i)%256];

						/* if background has priority over sprite, make the */
						/* temporary bitmap transparent */
						if (pixb != 0 && (pixa != Machine->gfx[0]->colortable[0]))
							sprite_bm->line[j][i] = Machine->pens[0];
					}
				}
			}

			copybitmap(bitmap,sprite_bm,0,0,x,y,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[0],
					RAM[spriteaddr+offs],1,
					0,0,
					x,y,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,7);
		}
	}
}
