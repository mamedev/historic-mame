/*********************************************************************

  usrintrf.c

  Functions used to handle MAME's crude user interface.

*********************************************************************/

#include "driver.h"
#include "info.h"


extern int need_to_clear_bitmap;	/* used to tell updatescreen() to clear the bitmap */
extern int bitmap_dirty;	/* set by osd_clearbitmap() */

extern int nocheat;

/* Variables for stat menu */
extern char mameversion[];
extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];

/* Prototypes for routines found in inptport.c */
const char *default_name(const struct InputPort *in);
int default_key(const struct InputPort *in);
int default_joy(const struct InputPort *in);


void set_ui_visarea (int xmin, int ymin, int xmax, int ymax)
{
	int temp,w,h;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		w = Machine->drv->screen_height;
		h = Machine->drv->screen_width;
	}
	else
	{
		w = Machine->drv->screen_width;
		h = Machine->drv->screen_height;
	}
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = xmin; xmin = ymin; ymin = temp;
		temp = xmax; xmax = ymax; ymax = temp;
		temp = w; w = h; h = temp;
	}
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - xmin - 1;
		xmin = w - xmax - 1;
		xmax = temp;
	}
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - ymin - 1;
		ymin = h - ymax - 1;
		ymax = temp;
	}

	Machine->uiwidth = xmax-xmin+1;
	Machine->uiheight = ymax-ymin+1;
	Machine->uixmin = xmin;
	Machine->uiymin = ymin;
}



struct GfxElement *builduifont(void)
{
	static unsigned char fontdata6x8[] =
	{
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x30,0x48,0x84,0xb4,0xb4,0x84,0x48,0x30,0x30,0x48,0x84,0x84,0x84,0x84,0x48,0x30,
		0x00,0xfc,0x84,0x8c,0xd4,0xa4,0xfc,0x00,0x00,0xfc,0x84,0x84,0x84,0x84,0xfc,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,
		0x80,0xc0,0xe0,0xf0,0xe0,0xc0,0x80,0x00,0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04,0x00,
		0x20,0x70,0xf8,0x20,0x20,0xf8,0x70,0x20,0x48,0x48,0x48,0x48,0x48,0x00,0x48,0x00,
		0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,
		0x70,0xd8,0xe8,0xe8,0xf8,0xf8,0x70,0x00,0x1c,0x7c,0x74,0x44,0x44,0x4c,0xcc,0xc0,
		0x20,0x70,0xf8,0x70,0x70,0x70,0x70,0x00,0x70,0x70,0x70,0x70,0xf8,0x70,0x20,0x00,
		0x00,0x10,0xf8,0xfc,0xf8,0x10,0x00,0x00,0x00,0x20,0x7c,0xfc,0x7c,0x20,0x00,0x00,
		0xb0,0x54,0xb8,0xb8,0x54,0xb0,0x00,0x00,0x00,0x28,0x6c,0xfc,0x6c,0x28,0x00,0x00,
		0x00,0x30,0x30,0x78,0x78,0xfc,0x00,0x00,0xfc,0x78,0x78,0x30,0x30,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	};
#if 0	   /* HJB 990215 unused!? */
    static unsigned char fontdata8x8[] =
	{
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x3C,0x42,0x99,0xBD,0xBD,0x99,0x42,0x3C,0x3C,0x42,0x81,0x81,0x81,0x81,0x42,0x3C,
		0xFE,0x82,0x8A,0xD2,0xA2,0x82,0xFE,0x00,0xFE,0x82,0x82,0x82,0x82,0x82,0xFE,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
		0x80,0xC0,0xF0,0xFC,0xF0,0xC0,0x80,0x00,0x01,0x03,0x0F,0x3F,0x0F,0x03,0x01,0x00,
		0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0x00,0xEE,0xEE,0xEE,0xCC,0x00,0xCC,0xCC,0x00,
		0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
		0x3C,0x66,0x7A,0x7A,0x7E,0x7E,0x3C,0x00,0x0E,0x3E,0x3A,0x22,0x26,0x6E,0xE4,0x40,
		0x18,0x3C,0x7E,0x3C,0x3C,0x3C,0x3C,0x00,0x3C,0x3C,0x3C,0x3C,0x7E,0x3C,0x18,0x00,
		0x08,0x7C,0x7E,0x7E,0x7C,0x08,0x00,0x00,0x10,0x3E,0x7E,0x7E,0x3E,0x10,0x00,0x00,
		0x58,0x2A,0xDC,0xC8,0xDC,0x2A,0x58,0x00,0x24,0x66,0xFF,0xFF,0x66,0x24,0x00,0x00,
		0x00,0x10,0x10,0x38,0x38,0x7C,0xFE,0x00,0xFE,0x7C,0x38,0x38,0x10,0x10,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x1C,0x18,0x00,0x18,0x18,0x00,
		0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x7C,0x28,0x7C,0x28,0x00,0x00,
		0x10,0x38,0x60,0x38,0x0C,0x78,0x10,0x00,0x40,0xA4,0x48,0x10,0x24,0x4A,0x04,0x00,
		0x18,0x34,0x18,0x3A,0x6C,0x66,0x3A,0x00,0x18,0x18,0x20,0x00,0x00,0x00,0x00,0x00,
		0x30,0x60,0x60,0x60,0x60,0x60,0x30,0x00,0x0C,0x06,0x06,0x06,0x06,0x06,0x0C,0x00,
		0x10,0x54,0x38,0x7C,0x38,0x54,0x10,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00,
		0x38,0x4C,0xC6,0xC6,0xC6,0x64,0x38,0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00,
		0x7C,0xC6,0x0E,0x3C,0x78,0xE0,0xFE,0x00,0x7E,0x0C,0x18,0x3C,0x06,0xC6,0x7C,0x00,
		0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00,0xFC,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00,
		0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00,
		0x78,0xC4,0xE4,0x78,0x86,0x86,0x7C,0x00,0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00,
		0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
		0x1C,0x38,0x70,0xE0,0x70,0x38,0x1C,0x00,0x00,0x7C,0x00,0x00,0x7C,0x00,0x00,0x00,
		0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00,0x7C,0xC6,0xC6,0x1C,0x18,0x00,0x18,0x00,
		0x3C,0x42,0x99,0xA1,0xA5,0x99,0x42,0x3C,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00,
		0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00,0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
		0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00,
		0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00,0x3E,0x60,0xC0,0xCE,0xC6,0x66,0x3E,0x00,
		0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,
		0x06,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00,0xC6,0xCC,0xD8,0xF0,0xF8,0xDC,0xCE,0x00,
		0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
		0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
		0xFC,0xC6,0xC6,0xC6,0xFC,0xC0,0xC0,0x00,0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x7A,0x00,
		0xFC,0xC6,0xC6,0xCE,0xF8,0xDC,0xCE,0x00,0x78,0xCC,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
		0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
		0xC6,0xC6,0xC6,0xEE,0x7C,0x38,0x10,0x00,0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00,
		0xC6,0xEE,0x3C,0x38,0x7C,0xEE,0xC6,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00,
		0xFE,0x0E,0x1C,0x38,0x70,0xE0,0xFE,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,
		0x60,0x60,0x30,0x18,0x0C,0x06,0x06,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,
		0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
		0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x3C,0x00,
		0x60,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,
		0x06,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x3C,0x00,
		0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x3C,
		0x60,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x00,
		0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00,
		0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0xEC,0xFE,0xFE,0xFE,0xD6,0xC6,0x00,
		0x00,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,
		0x00,0x7C,0x66,0x66,0x66,0x7C,0x60,0x60,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x06,
		0x00,0x7E,0x70,0x60,0x60,0x60,0x60,0x00,0x00,0x3C,0x60,0x3C,0x06,0x66,0x3C,0x00,
		0x30,0x78,0x30,0x30,0x30,0x30,0x1C,0x00,0x00,0x66,0x66,0x66,0x66,0x6E,0x3E,0x00,
		0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x7C,0x6C,0x00,
		0x00,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x06,0x3C,
		0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00,0x0E,0x18,0x0C,0x38,0x0C,0x18,0x0E,0x00,
		0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x70,0x18,0x30,0x1C,0x30,0x18,0x70,0x00,
		0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x10,0x28,0x10,0x54,0xAA,0x44,0x00,0x00,
	};
#endif
    static struct GfxLayout fontlayout6x8 =
	{
		6,8,	/* 6*8 characters */
		128,    /* 128 characters */
		1,	/* 1 bit per pixel */
		{ 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
		8*8	/* every char takes 8 consecutive bytes */
	};
	static struct GfxLayout fontlayout12x8 =
	{
		12,8,	/* 12*8 characters */
		128,    /* 128 characters */
		1,	/* 1 bit per pixel */
		{ 0 },
		{ 0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7 },	/* straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
		8*8	/* every char takes 8 consecutive bytes */
	};
	static struct GfxLayout fontlayout12x16 =
	{
		12,16,	/* 6*8 characters */
		128,    /* 128 characters */
		1,	/* 1 bit per pixel */
		{ 0 },
		{ 0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7 },	/* straightforward layout */
		{ 0*8,0*8, 1*8,1*8, 2*8,2*8, 3*8,3*8, 4*8,4*8, 5*8,5*8, 6*8,6*8, 7*8,7*8 },
		8*8	/* every char takes 8 consecutive bytes */
	};
#if 0	/* HJB 990215 unused!? */
    static struct GfxLayout fontlayout8x8 =
	{
		8,8,	/* 8*8 characters */
		128,    /* 128 characters */
		1,	/* 1 bit per pixel */
		{ 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
		8*8	/* every char takes 8 consecutive bytes */
	};
#endif
    struct GfxElement *font;
	static unsigned short colortable[2*2];	/* ASG 980209 */
	int trueorientation;


	/* hack: force the display into standard orientation to avoid */
	/* creating a rotated font */
	trueorientation = Machine->orientation;
	Machine->orientation = Machine->ui_orientation;

	if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
			== VIDEO_PIXEL_ASPECT_RATIO_1_2)
	{
		font = decodegfx(fontdata6x8,&fontlayout12x8);
		Machine->uifontwidth = 12;
		Machine->uifontheight = 8;
	}
	else if (Machine->uiwidth >= 420 && Machine->uiheight >= 420)
	{
		font = decodegfx(fontdata6x8,&fontlayout12x16);
		Machine->uifontwidth = 12;
		Machine->uifontheight = 16;
	}
	else
	{
		font = decodegfx(fontdata6x8,&fontlayout6x8);
		Machine->uifontwidth = 6;
		Machine->uifontheight = 8;
	}

	if (font)
	{
		/* colortable will be set at run time */
		memset(colortable,0,sizeof(colortable));
		font->colortable = colortable;
		font->total_colors = 2;
	}

	Machine->orientation = trueorientation;

	return font;
}



/***************************************************************************

  Display text on the screen. If erase is 0, it superimposes the text on
  the last frame displayed.

***************************************************************************/

void displaytext(const struct DisplayText *dt,int erase,int update_screen)
{
	int trueorientation;


	if (erase)
		osd_clearbitmap(Machine->scrbitmap);


	/* hack: force the display into standard orientation to avoid */
	/* rotating the user interface */
	trueorientation = Machine->orientation;
	Machine->orientation = Machine->ui_orientation;

	osd_mark_dirty (0,0,Machine->uiwidth-1,Machine->uiheight-1,1);	/* ASG 971011 */

	while (dt->text)
	{
		int x,y;
		const char *c;


		x = dt->x;
		y = dt->y;
		c = dt->text;

		while (*c)
		{
			int wrapped;


			wrapped = 0;

			if (*c == '\n')
			{
				x = dt->x;
				y += Machine->uifontheight + 1;
				wrapped = 1;
			}
			else if (*c == ' ')
			{
				/* don't try to word wrap at the beginning of a line (this would cause */
				/* an endless loop if a word is longer than a line) */
				if (x != dt->x)
				{
					int nextlen=0;
					const char *nc;


					nc = c+1;
					while (*nc && *nc != ' ' && *nc != '\n')
					{
						nextlen += Machine->uifontwidth;
						nc++;
					}

					/* word wrap */
					if (x + Machine->uifontwidth + nextlen > Machine->uiwidth)
					{
						x = dt->x;
						y += Machine->uifontheight + 1;
						wrapped = 1;
					}
				}
			}

			if (!wrapped)
			{
				drawgfx(Machine->scrbitmap,Machine->uifont,*c,dt->color,0,0,x+Machine->uixmin,y+Machine->uiymin,0,TRANSPARENCY_NONE,0);
				x += Machine->uifontwidth;
			}

			c++;
		}

		dt++;
	}

	Machine->orientation = trueorientation;

	if (update_screen) osd_update_video_and_audio();
}

/* Writes messages on the screen. */
void ui_text(char *buf,int x,int y)
{
	int trueorientation,l,i;


	/* hack: force the display into standard orientation to avoid */
	/* rotating the text */
	trueorientation = Machine->orientation;
	Machine->orientation = Machine->ui_orientation;

	l = strlen(buf);
	for (i = 0;i < l;i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				x + i*Machine->uifontwidth + Machine->uixmin,
				y + Machine->uiymin, 0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}


INLINE void drawpixel(int x, int y, unsigned short color)
{
	int temp;

	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = x; x = y; y = temp;
	}
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
		x = Machine->scrbitmap->width - x - 1;
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
		y = Machine->scrbitmap->height - y - 1;

	if (Machine->scrbitmap->depth == 16)
		*(unsigned short *)&Machine->scrbitmap->line[y][x*2] = color;
	else
		Machine->scrbitmap->line[y][x] = color;

	osd_mark_dirty(x,y,x,y,1);
}

INLINE void drawhline_norotate(int x, int w, int y, unsigned short color)
{
	if (Machine->scrbitmap->depth == 16)
	{
        int i;
		for (i = x; i < x+w; i++)
			*(unsigned short *)&Machine->scrbitmap->line[y][i*2] = color;
	}
	else
		memset(&Machine->scrbitmap->line[y][x], color, w);

	osd_mark_dirty(x,y,x+w-1,y,1);
}

INLINE void drawvline_norotate(int x, int y, int h, unsigned short color)
{
	int i;

	if (Machine->scrbitmap->depth == 16)
	{
		for (i = y; i < y+h; i++)
			*(unsigned short *)&Machine->scrbitmap->line[i][x*2] = color;
	}
	else
	{
		for (i = y; i < y+h; i++)
			Machine->scrbitmap->line[i][x] = color;
	}

	osd_mark_dirty(x,y,x,y+h-1,1);
}

INLINE void drawhline(int x, int w, int y, unsigned short color)
{
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		if (Machine->ui_orientation & ORIENTATION_FLIP_X)
			y = Machine->scrbitmap->width - y - 1;
		if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
			x = Machine->scrbitmap->height - x - w;

		drawvline_norotate(y,x,w,color);
	}
	else
	{
		if (Machine->ui_orientation & ORIENTATION_FLIP_X)
			x = Machine->scrbitmap->width - x - w;
		if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
			y = Machine->scrbitmap->height - y - 1;

		drawhline_norotate(x,w,y,color);
	}
}

INLINE void drawvline(int x, int y, int h, unsigned short color)
{
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		if (Machine->ui_orientation & ORIENTATION_FLIP_X)
			y = Machine->scrbitmap->width - y - h;
		if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
			x = Machine->scrbitmap->height - x - 1;

		drawhline_norotate(y,h,x,color);
	}
	else
	{
		if (Machine->ui_orientation & ORIENTATION_FLIP_X)
			x = Machine->scrbitmap->width - x - 1;
		if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
			y = Machine->scrbitmap->height - y - h;

		drawvline_norotate(x,y,h,color);
	}
}


static void drawbox(int leftx,int topy,int width,int height)
{
	int y;
	unsigned short black,white;


	if (leftx < 0) leftx = 0;
	if (topy < 0) topy = 0;
	if (width > Machine->uiwidth) width = Machine->uiwidth;
	if (height > Machine->uiheight) height = Machine->uiheight;

	leftx += Machine->uixmin;
	topy += Machine->uiymin;

	black = Machine->uifont->colortable[0];
	white = Machine->uifont->colortable[1];

	drawhline(leftx,width,topy, 		white);
	drawhline(leftx,width,topy+height-1,white);
	drawvline(leftx,		topy,height,white);
	drawvline(leftx+width-1,topy,height,white);
    for (y = topy+1;y < topy+height-1;y++)
		drawhline(leftx+1,width-2,y,black);
}


static void drawbar(int leftx,int topy,int width,int height,int percentage)
{
	int y;
	unsigned short black,white;


	if (leftx < 0) leftx = 0;
	if (topy < 0) topy = 0;
	if (width > Machine->uiwidth) width = Machine->uiwidth;
	if (height > Machine->uiheight) height = Machine->uiheight;

	leftx += Machine->uixmin;
	topy += Machine->uiymin;

	black = Machine->uifont->colortable[0];
	white = Machine->uifont->colortable[1];

    for (y = topy;y < topy + height/8;y++)
	{
		drawpixel(leftx,			y, white);
		drawpixel(leftx+1,			y, white);
		drawpixel(leftx+width/2-1,	y, white);
		drawpixel(leftx+width/2,	y, white);
		drawpixel(leftx+width-2,	y, white);
		drawpixel(leftx+width-1,	y, white);
	}
	for (y = topy+height/8;y < topy+height-height/8;y++)
		drawhline(leftx,width*percentage/100,y,white);
	for (y = topy+height-height/8;y < topy + height;y++)
	{
		drawpixel(leftx,			y, white);
		drawpixel(leftx+1,			y, white);
		drawpixel(leftx+width/2-1,	y, white);
		drawpixel(leftx+width/2,	y, white);
		drawpixel(leftx+width-2,	y, white);
		drawpixel(leftx+width-1,	y, white);
	}
}


void displaymenu(const char **items,const char **subitems,char *flag,int selected,int arrowize_subitem)
{
	struct DisplayText dt[256];
	int curr_dt;
	char lefthilight[2] = "\x1a";
	char righthilight[2] = "\x1b";
	char uparrow[2] = "\x18";
	char downarrow[2] = "\x19";
	char leftarrow[2] = "\x11";
	char rightarrow[2] = "\x10";
	int i,count,len,maxlen;
	int leftoffs,topoffs,visible,topitem;


	i = 0;
	maxlen = 0;
	while (items[i])
	{
		len = strlen(items[i]);
		if (subitems && subitems[i])
		{
			len += 1 + strlen(subitems[i]);
		}
		if (len > maxlen) maxlen = len;
		i++;
	}
	count = i;

	maxlen += 3;
	if (maxlen * Machine->uifontwidth > Machine->uiwidth)
		maxlen = Machine->uiwidth / Machine->uifontwidth;

	visible = Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;
	topitem = 0;
	if (visible > count) visible = count;
	else
	{
		topitem = selected - visible / 2;
		if (topitem < 0) topitem = 0;
		if (topitem > count - visible) topitem = count - visible;
	}

	leftoffs = (Machine->uiwidth - maxlen * Machine->uifontwidth) / 2;
	topoffs = (Machine->uiheight - (3 * visible + 1) * Machine->uifontheight / 2) / 2;

	/* black background */
	drawbox(leftoffs,topoffs,maxlen * Machine->uifontwidth,(3 * visible + 1) * Machine->uifontheight / 2);

	curr_dt = 0;
	for (i = 0;i < visible;i++)
	{
		int item = i + topitem;

		if (i == 0 && item > 0)
		{
			dt[curr_dt].text = uparrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifontwidth * strlen(uparrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
			curr_dt++;
		}
		else if (i == visible - 1 && item < count - 1)
		{
			dt[curr_dt].text = downarrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifontwidth * strlen(downarrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
			curr_dt++;
		}
		else
		{
			if (subitems && subitems[item])
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = leftoffs + 3*Machine->uifontwidth/2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
				curr_dt++;
				dt[curr_dt].text = subitems[item];
				/* If this item is flagged, draw it in inverse print */
				if (flag && flag[item])
					dt[curr_dt].color = DT_COLOR_YELLOW;
				else
					dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = leftoffs + Machine->uifontwidth * (maxlen-1 - strlen(dt[curr_dt].text)) - Machine->uifontwidth/2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
				curr_dt++;
			}
			else
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = (Machine->uiwidth - Machine->uifontwidth * strlen(items[item])) / 2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
				curr_dt++;
			}
		}
	}

	i = selected - topitem;
	if (subitems && subitems[selected] && arrowize_subitem)
	{
		if (arrowize_subitem & 1)
		{
			dt[curr_dt].text = leftarrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = leftoffs + Machine->uifontwidth * (maxlen-2 - strlen(subitems[selected])) - Machine->uifontwidth/2 - 1;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
			curr_dt++;
		}
		if (arrowize_subitem & 2)
		{
			dt[curr_dt].text = rightarrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = leftoffs + Machine->uifontwidth * (maxlen-1) - Machine->uifontwidth/2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
			curr_dt++;
		}
	}
	else
	{
		dt[curr_dt].text = righthilight;
		dt[curr_dt].color = DT_COLOR_WHITE;
		dt[curr_dt].x = leftoffs + Machine->uifontwidth * (maxlen-1) - Machine->uifontwidth/2;
		dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
		curr_dt++;
	}
	dt[curr_dt].text = lefthilight;
	dt[curr_dt].color = DT_COLOR_WHITE;
	dt[curr_dt].x = leftoffs + Machine->uifontwidth/2;
	dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
	curr_dt++;

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(dt,0,0);
}


static void displaymessagewindow(const char *text)
{
	struct DisplayText dt[256];
	int curr_dt;
	char *c,*c2;
	int i,len,maxlen,lines;
	char textcopy[2048];
	int leftoffs,topoffs;
	int	maxcols,maxrows;

	maxcols = (Machine->uiwidth / Machine->uifontwidth) - 1;
	maxrows = (2 * Machine->uiheight - Machine->uifontheight) / (3 * Machine->uifontheight);

	/* copy text, calculate max len, count lines, wrap long lines and crop height to fit */
	maxlen = 0;
	lines = 0;
	c = (char *)text;
	c2 = textcopy;
	while (*c)
	{
		len = 0;
		while (*c && *c != '\n')
		{
			*c2++ = *c++;
			len++;
			if (len == maxcols && *c != '\n')
			{
				/* attempt word wrap */
				char *csave = c, *c2save = c2;
				int lensave = len;

				/* back up to last space or beginning of line */
				while (*c != ' ' && *c != '\n' && c > text)
					--c, --c2, --len;

				/* if no space was found, hard wrap instead */
				if (*c != ' ')
					c = csave, c2 = c2save, len = lensave;
				else
					c++;

				*c2++ = '\n'; /* insert wrap */
				break;
			}
		}

		if (*c == '\n')
			*c2++ = *c++;

		if (len > maxlen) maxlen = len;

		lines++;
		if (lines == maxrows)
			break;
	}
	*c2 = '\0';

	maxlen += 1;

	leftoffs = (Machine->uiwidth - Machine->uifontwidth * maxlen) / 2;
	if (leftoffs < 0) leftoffs = 0;
	topoffs = (Machine->uiheight - (3 * lines + 1) * Machine->uifontheight / 2) / 2;

	/* black background */
	drawbox(leftoffs,topoffs,maxlen * Machine->uifontwidth,(3 * lines + 1) * Machine->uifontheight / 2);

	curr_dt = 0;
	c = textcopy;
	i = 0;
	while (*c)
	{
		c2 = c;
		while (*c && *c != '\n')
			c++;

		if (*c == '\n')
		{
			*c = '\0';
			c++;
		}

		if (*c2 == '\t')	/* center text */
		{
			c2++;
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifontwidth * (c - c2)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + Machine->uifontwidth/2;

		dt[curr_dt].text = c2;
		dt[curr_dt].color = DT_COLOR_WHITE;
		dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifontheight/2;
		curr_dt++;

		i++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(dt,0,0);
}



#ifndef NEOFREE
#ifndef TINY_COMPILE
extern int no_of_tiles;
void NeoMVSDrawGfx(unsigned char **line,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
        int zx,int zy,const struct rectangle *clip);
extern struct GameDriver neogeo_bios;
#endif
#endif

static void showcharset(void)
{
	int i;
	struct DisplayText dt[2];
	char buf[80];
	int bank,color,firstdrawn;
	int trueorientation;
	int changed;
	int game_is_neogeo=0;


	if ((Machine->drv->gfxdecodeinfo == 0) ||
			(Machine->drv->gfxdecodeinfo[0].memory_region == -1))
		return;	/* no gfx sets, return */

#ifndef NEOFREE
#ifndef TINY_COMPILE
	if(Machine->gamedrv->clone_of == &neogeo_bios) game_is_neogeo=1;
#endif
#endif

	/* hack: force the display into standard orientation to avoid */
	/* rotating the user interface */
	trueorientation = Machine->orientation;
	Machine->orientation = Machine->ui_orientation;


	bank = 0;
	color = 0;
	firstdrawn = 0;

	changed = 1;

	do
	{
		int cpx,cpy,skip_chars;

		cpx = Machine->uiwidth / Machine->gfx[bank]->width;
		cpy = (Machine->uiheight - Machine->uifontheight) / Machine->gfx[bank]->height;
		skip_chars = cpx * cpy;

		if (changed)
		{
			int lastdrawn=0;

			osd_clearbitmap(Machine->scrbitmap);

			/* validity chack after char bank change */
			if (firstdrawn >= Machine->drv->gfxdecodeinfo[bank].gfxlayout->total)
			{
				firstdrawn = Machine->drv->gfxdecodeinfo[bank].gfxlayout->total - skip_chars;
				if (firstdrawn < 0) firstdrawn = 0;
			}

			if(bank!=2 || !game_is_neogeo)
			{
				for (i = 0; i+firstdrawn < Machine->drv->gfxdecodeinfo[bank].gfxlayout->total && i<cpx*cpy; i++)
				{
					drawgfx(Machine->scrbitmap,Machine->gfx[bank],
						i+firstdrawn,color,  /*sprite num, color*/
						0,0,
						(i % cpx) * Machine->gfx[bank]->width + Machine->uixmin,
						Machine->uifontheight + (i / cpx) * Machine->gfx[bank]->height + Machine->uiymin,
						0,TRANSPARENCY_NONE,0);

					lastdrawn = i+firstdrawn;
				}
			}
#ifndef NEOFREE
#ifndef TINY_COMPILE
			else	/* neogeo sprite tiles */
			{
				struct rectangle clip;

				clip.min_x = Machine->uixmin;
				clip.max_x = Machine->uixmin + Machine->uiwidth - 1;
				clip.min_y = Machine->uiymin;
				clip.max_y = Machine->uiymin + Machine->uiheight - 1;

				for (i = 0; i+firstdrawn < no_of_tiles && i<cpx*cpy; i++)
				{
					NeoMVSDrawGfx(Machine->scrbitmap->line,Machine->gfx[bank],
						i+firstdrawn,color,  /*sprite num, color*/
						0,0,
						(i % cpx) * Machine->gfx[bank]->width + Machine->uixmin,
						Machine->uifontheight+1 + (i / cpx) * Machine->gfx[bank]->height + Machine->uiymin,
						16,16,&clip);

					lastdrawn = i+firstdrawn;
				}
			}
#endif
#endif

			sprintf(buf,"GFXSET %d COLOR %d CODE %x-%x",bank,color,firstdrawn,lastdrawn);
			dt[0].text = buf;
			dt[0].color = DT_COLOR_RED;
			dt[0].x = 0;
			dt[0].y = 0;
			dt[1].text = 0;
			displaytext(dt,0,0);

			changed = 0;
		}

		/* Necessary to keep the video from getting stuck if a frame happens to be skipped in here */
/* I beg to differ - the OS dependant code must not assume that */
/* osd_skip_this_frame() is called before osd_update_video_and_audio() - NS */
//		osd_skip_this_frame();
		osd_update_video_and_audio();

		if (osd_key_pressed(OSD_KEY_LCONTROL) || osd_key_pressed(OSD_KEY_RCONTROL))
		{
			skip_chars = cpx;
		}
		if (osd_key_pressed(OSD_KEY_LSHIFT) || osd_key_pressed(OSD_KEY_RSHIFT))
		{
			skip_chars = 1;
		}


		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_RIGHT,8))
		{
			if (bank+1 < MAX_GFX_ELEMENTS && Machine->gfx[bank + 1])
			{
				bank++;
//				firstdrawn = 0;
				changed = 1;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_LEFT,8))
		{
			if (bank > 0)
			{
				bank--;
//				firstdrawn = 0;
				changed = 1;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_PGDN,4))
		{
			if (firstdrawn + skip_chars < Machine->drv->gfxdecodeinfo[bank].gfxlayout->total)
			{
				firstdrawn += skip_chars;
				changed = 1;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_PGUP,4))
		{
			firstdrawn -= skip_chars;
			if (firstdrawn < 0) firstdrawn = 0;
			changed = 1;
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,6))
		{
			if (color < Machine->drv->gfxdecodeinfo[bank].total_color_codes - 1)
			{
				color++;
				changed = 1;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,6))
		{
			if (color > 0)
			{
				color--;
				changed = 1;
			}
		}

		if (osd_key_pressed_memory(OSD_KEY_SNAPSHOT))
		{
			osd_save_snapshot();
		}
	} while (!osd_key_pressed_memory(OSD_KEY_SHOW_GFX) &&
			!osd_key_pressed_memory(OSD_KEY_CANCEL) &&
			!osd_key_pressed_memory(OSD_KEY_FAST_EXIT));

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	Machine->orientation = trueorientation;

	return;
}


#ifdef MAME_DEBUG
static void showtotalcolors(void)
{
	char used[0x10000];
	int i,l,x,y,total;
	char buf[40];
	int trueorientation;


	for (i = 0;i < 0x10000;i++)
		used[i] = 0;

	if (Machine->scrbitmap->depth == 16)
	{
		for (y = 0;y < Machine->scrbitmap->height;y++)
		{
			for (x = 0;x < Machine->scrbitmap->width;x++)
			{
				used[((unsigned short *)Machine->scrbitmap->line[y])[x]] = 1;
			}
		}
	}
	else
	{
		for (y = 0;y < Machine->scrbitmap->height;y++)
		{
			for (x = 0;x < Machine->scrbitmap->width;x++)
			{
				used[Machine->scrbitmap->line[y][x]] = 1;
			}
		}
	}

	total = 0;
	for (i = 0;i < 0x10000;i++)
		if (used[i]) total++;

	/* hack: force the display into standard orientation to avoid */
	/* rotating the text */
	trueorientation = Machine->orientation;
	Machine->orientation = Machine->ui_orientation;

	sprintf(buf,"%5d colors",total);
	l = strlen(buf);
	for (i = 0;i < l;i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],total>256?DT_COLOR_YELLOW:DT_COLOR_WHITE,0,0,Machine->uixmin+i*Machine->uifontwidth,Machine->uiymin,0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}
#endif


static int setdipswitches(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	char flag[40];
	int i,sel;
	struct InputPort *in;
	int total;
	int arrowize;


	sel = selected - 1;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME && default_name(in) != 0 &&
				(in->type & IPF_UNUSED) == 0 &&
				!(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = default_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = "Return to Main Menu";
	menu_item[total + 1] = 0;	/* terminate array */
	total++;


	for (i = 0;i < total;i++)
	{
		flag[i] = 0; /* TODO: flag the dip if it's not the real default */
		if (i < total - 1)
		{
			in = entry[i] + 1;
			while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
					in->default_value != entry[i]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
				menu_subitem[i] = "INVALID";
			else menu_subitem[i] = default_name(in);
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	arrowize = 0;
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
				in->default_value != entry[sel]->default_value)
			in++;

		if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
			/* invalid setting: revert to a valid one */
			arrowize |= 1;
		else
		{
			if (((in-1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
					!(nocheat && ((in-1)->type & IPF_CHEAT)))
				arrowize |= 1;
		}
	}
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
				in->default_value != entry[sel]->default_value)
			in++;

		if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
			/* invalid setting: revert to a valid one */
			arrowize |= 2;
		else
		{
			if (((in+1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
					!(nocheat && ((in+1)->type & IPF_CHEAT)))
				arrowize |= 2;
		}
	}

	displaymenu(menu_item,menu_subitem,flag,sel,arrowize);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_RIGHT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
					in->default_value != entry[sel]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if (((in+1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
						!(nocheat && ((in+1)->type & IPF_CHEAT)))
					entry[sel]->default_value = (in+1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_LEFT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
					in->default_value != entry[sel]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if (((in-1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
						!(nocheat && ((in-1)->type & IPF_CHEAT)))
					entry[sel]->default_value = (in-1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		sel = -1;

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}



static int setdefkeysettings(int selected)
{
	const char *menu_item[400];
	const char *menu_subitem[400];
	struct ipd *entry[400];
	char flag[400];
	int i,sel;
	struct ipd *in;
	int total;
	extern struct ipd inputport_defaults[];


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = inputport_defaults;

	total = 0;
	while (in->type != IPT_END)
	{
		if (in->name != 0 && in->keyboard != IP_KEY_NONE && (in->type & IPF_UNUSED) == 0
			&& (((in->type & 0xff) < IPT_ANALOG_START) || ((in->type & 0xff) > IPT_ANALOG_END))
			&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = in->name;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = "Return to Main Menu";
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
			menu_subitem[i] = osd_key_name(entry[i]->keyboard);
		else menu_subitem[i] = 0;	/* no subitem */
		flag[i] = 0;
	}

	if (sel > 255)	/* are we waiting for a new key? */
	{
		int newkey;


		menu_subitem[sel & 0xff] = "    ";
		displaymenu(menu_item,menu_subitem,flag,sel & 0xff,3);
		newkey = osd_read_key_immediate();
		if (newkey != OSD_KEY_NONE)
		{
			sel &= 0xff;

			if (!osd_key_invalid(newkey))	/* don't use pseudo key code */
				entry[sel]->keyboard = newkey;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,flag,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			sel |= 0x100;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		sel = -1;

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}



static int setkeysettings(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	char flag[40];
	int i,sel;
	struct InputPort *in;
	int total;


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_key(in) != IP_KEY_NONE && (in->type & IPF_UNUSED) == 0
			&& (((in->type & 0xff) < IPT_ANALOG_START) || ((in->type & 0xff) > IPT_ANALOG_END))
			&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = default_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = "Return to Main Menu";
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
		{
			menu_subitem[i] = osd_key_name(default_key(entry[i]));
			/* If the key isn't the default, flag it */
			if (entry[i]->keyboard != IP_KEY_DEFAULT)
				flag[i] = 1;
			else
				flag[i] = 0;

		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > 255)	/* are we waiting for a new key? */
	{
		int newkey;


		menu_subitem[sel & 0xff] = "    ";
		displaymenu(menu_item,menu_subitem,flag,sel & 0xff,3);
		newkey = osd_read_key_immediate();
		if (newkey != OSD_KEY_NONE)
		{
			sel &= 0xff;

			if (osd_key_invalid(newkey))	/* pseudo key code? */
				entry[sel]->keyboard = IP_KEY_DEFAULT;
				/* BUG: hitting a pseudo-key to restore the default for a custom key removes it */
			else
				entry[sel]->keyboard = newkey;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,flag,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			sel |= 0x100;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		sel = -1;

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}



static int setjoysettings(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	char flag[40];
	int i,sel;
	struct InputPort *in;
	int total;


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_joy(in) != IP_JOY_NONE && (in->type & IPF_UNUSED) == 0
			&& (((in->type & 0xff) < IPT_ANALOG_START) || ((in->type & 0xff) > IPT_ANALOG_END))
			&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = default_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = "Return to Main Menu";
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
		{
			menu_subitem[i] = osd_joy_name(default_joy(entry[i]));
			if (entry[i]->joystick != IP_JOY_DEFAULT)
				flag[i] = 1;
			else
				flag[i] = 0;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > 255)	/* are we waiting for a new joy direction? */
	{
		int newjoy;
		int joyindex;


		menu_subitem[sel & 0xff] = "    ";
		displaymenu(menu_item,menu_subitem,flag,sel & 0xff,3);

		/* Check all possible joystick values for switch or button press */
		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		{
			sel &= 0xff;
			entry[sel]->joystick = IP_JOY_DEFAULT;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}

		/* Allows for "All buttons" */
		if (osd_key_pressed_memory(OSD_KEY_A))
		{
			sel &= 0xff;
			entry[sel]->joystick = OSD_JOY_FIRE;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
		if (osd_key_pressed_memory(OSD_KEY_B))
		{
			sel &= 0xff;
			entry[sel]->joystick = OSD_JOY2_FIRE;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
		/* Clears entry "None" */
		if (osd_key_pressed_memory(OSD_KEY_N))
		{
			sel &= 0xff;
			entry[sel]->joystick = 0;

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}

		for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++)
		{
			newjoy = osd_joy_pressed(joyindex);
			if (newjoy)
			{
				sel &= 0xff;
				entry[sel]->joystick = joyindex;

				/* tell updatescreen() to clean after us (in case the window changes size) */
				need_to_clear_bitmap = 1;
				break;
			}
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,flag,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			sel |= 0x100;	/* we'll ask for a joy */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			need_to_clear_bitmap = 1;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		sel = -1;

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}


static int calibratejoysticks(int selected)
{
	char *msg;
	char buf[2048];
	int sel;
	static int calibration_started = 0;

	sel = selected - 1;

	if (calibration_started == 0)
	{
		osd_joystick_start_calibration();
		calibration_started = 1;
		strcpy (buf, "");
	}

	if (sel > 255) /* Waiting for the user to acknowledge joystick movement */
	{
		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT)
				|| osd_key_pressed_memory (OSD_KEY_CANCEL))
		{
			calibration_started = 0;
			sel = -1;
		}
		else if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
		{
			osd_joystick_calibrate();
			sel &= 0xff;
		}

		displaymessagewindow(buf);
	}
	else
	{
		msg = osd_joystick_calibrate_next();
		need_to_clear_bitmap = 1;
		if (msg == 0)
		{
			calibration_started = 0;
			osd_joystick_end_calibration();
			sel = -1;
		}
		else
		{
			strcpy (buf, msg);
			displaymessagewindow(buf);
			sel |= 0x100;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}


static int settraksettings(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	int i,sel;
	struct InputPort *in;
	int total,total2;
	int arrowize;


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	/* Count the total number of analog controls */
	total = 0;
	while (in->type != IPT_END)
	{
		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
				&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			total++;
		}
		in++;
	}

	if (total == 0) return 0;
	/* Each analog control has 7 entries - deckey, inckey, key & joy delta, decjoy, incjoy, reverse, sensitivity */

#define ENTRIES 7

	total2 = total * ENTRIES;

	menu_item[total2] = "Return to Main Menu";
	menu_item[total2 + 1] = 0;	/* terminate array */
	total2++;

	arrowize = 0;
	for (i = 0;i < total2;i++)
	{
		if (i < total2 - 1)
		{
			char label[30][40];
			char setting[30][40];
			int key, joy;
			int sensitivity;
			int reverse;

			strcpy (label[i], default_name(entry[i/ENTRIES]));
			key = default_key (entry[i/ENTRIES]);
			joy = default_joy (entry[i/ENTRIES]);
			sensitivity = (entry[i/ENTRIES]->arg & 0x000000ff);
			reverse = (entry[i/ENTRIES]->type & IPF_REVERSE);

			switch (i%ENTRIES)
			{
				case 0:
					strcat (label[i], " Key Dec");
					strcpy (setting[i], osd_key_name (key & 0xff));
					break;
				case 1:
					strcat (label[i], " Key Inc");
					strcpy (setting[i], osd_key_name ((key & 0xff00) >> 8));
					break;
				case 2:
					strcat (label[i], " Joy Dec");
					strcpy (setting[i], osd_joy_name (joy & 0xff));
					break;
				case 3:
					strcat (label[i], " Joy Inc");
					strcpy (setting[i], osd_joy_name ((joy & 0xff00) >> 8));
					break;
				case 4:
					strcat (label[i], " Key/Joy Delta");
					sprintf(setting[i],"%d",(key & 0xff0000) >> 16);
					if (i == sel) arrowize = 3;
					break;
				case 5:
					strcat (label[i], " Reverse ");
					if (reverse)
						sprintf(setting[i],"On");
					else
						sprintf(setting[i],"Off");
					if (i == sel) arrowize = 3;
					break;
				case 6:
					strcat (label[i], " Sensitivity (%)");
					sprintf(setting[i],"%3d",sensitivity);
					if (i == sel) arrowize = 3;
					break;
			}

			menu_item[i] = label[i];
			menu_subitem[i] = setting[i];

			in++;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > 255)	/* are we waiting for a new key? */
	{
		switch ((sel & 0xff) % ENTRIES)
		{
			case 0:
			case 1:
			/* We're changing either the dec key or the inc key */
			{
				int newkey;
				int oldkey = default_key(entry[(sel & 0xff)/ENTRIES]);

				menu_subitem[sel & 0xff] = "    ";
				displaymenu(menu_item,menu_subitem,0,sel & 0xff,3);
				newkey = osd_read_key_immediate();
				if (newkey != OSD_KEY_NONE)
				{
					sel &= 0xff;

					if (osd_key_invalid(newkey))	/* pseudo key code ? */
						newkey = 0;/*IP_KEY_DEFAULT;*/

					if (sel % ENTRIES)
					{
						oldkey &= ~0xff00;
						oldkey |= (newkey << 8);
					}
					else
					{
						oldkey &= ~0xff;
						oldkey |= (newkey);
					}
					entry[sel/ENTRIES]->keyboard = oldkey;
				}
				break;
			}
			case 2:
			case 3:
			/* We're changing either the dec joy or the inc joy */
			{
				int newjoy;
				int oldjoy = default_joy(entry[(sel & 0xff)/ENTRIES]);
				int joyindex;


				menu_subitem[sel & 0xff] = "    ";
				displaymenu(menu_item,menu_subitem,0,sel & 0xff,3);

				/* Check all possible joystick values for switch or button press */
				newjoy = -1;

				if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
				{
					sel &= 0xff;
					newjoy = 0;/*IP_JOY_DEFAULT;*/
				}

				/* Allows for "All buttons" */
				if (osd_key_pressed_memory(OSD_KEY_A))
				{
					sel &= 0xff;
					newjoy = OSD_JOY_FIRE;
				}
				if (osd_key_pressed_memory(OSD_KEY_B))
				{
					sel &= 0xff;
					newjoy = OSD_JOY2_FIRE;
				}
				/* Clears entry "None" */
				if (osd_key_pressed_memory(OSD_KEY_N))
				{
					sel &= 0xff;
					newjoy = 0;
				}

				if (newjoy == -1)
				{
					for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++)
					{
						newjoy = osd_joy_pressed(joyindex);
						if (newjoy)
						{
							sel &= 0xff;
							newjoy = joyindex;
							break;
						}
					}
				}

				if (newjoy != -1)
				{
					if ((sel % ENTRIES) == 3)
					{
						oldjoy &= ~0xff00;
						oldjoy |= (newjoy << 8);
					}
					else
					{
						oldjoy &= ~0xff;
						oldjoy |= (newjoy);
					}
					entry[sel/ENTRIES]->joystick = oldjoy;
				}
				break;
			}
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,0,sel,arrowize);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
	{
		if (sel < total2 - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total2 - 1;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_LEFT,8))
	{
		if ((sel % ENTRIES) == 4)
		/* keyboard/joystick delta */
		{
			int oldval = default_key (entry[sel/ENTRIES]);
			int val = (oldval & 0xff0000) >> 16;

			val --;
			if (val < 1) val = entry[sel/ENTRIES]->mask - 1;
			oldval &= ~0xff0000;
			oldval |= (val & 0xff) << 16;
			entry[sel/ENTRIES]->keyboard = oldval;
		}
		else if ((sel % ENTRIES) == 5)
		/* reverse */
		{
			int reverse = entry[sel/ENTRIES]->type & IPF_REVERSE;
			if (reverse)
				reverse=0;
			else
				reverse=IPF_REVERSE;
			entry[sel/ENTRIES]->type &= ~IPF_REVERSE;
			entry[sel/ENTRIES]->type |= reverse;
		}
		else if ((sel % ENTRIES) == 6)
		/* sensitivity */
		{
			int oldval = (entry[sel/ENTRIES]->arg);
			int val = (oldval & 0xff);

			val --;
			if (val < 1) val = 1;
			oldval &= ~0xff;
			oldval |= (val & 0xff);
			entry[sel/ENTRIES]->arg = oldval;
		}
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_RIGHT,8))
	{
		if ((sel % ENTRIES) == 4)
		/* keyboard/joystick delta */
		{
			int oldval = default_key (entry[sel/ENTRIES]);
			int val = (oldval & 0xff0000) >> 16;

			val ++;
			if (val > entry[sel/ENTRIES]->mask - 1) val = 1;
			oldval &= ~0xff0000;
			oldval |= (val & 0xff) << 16;
			entry[sel/ENTRIES]->keyboard = oldval;
		}
		else if ((sel % ENTRIES) == 5)
		/* reverse */
		{
			int reverse = entry[sel/ENTRIES]->type & IPF_REVERSE;
			if (reverse)
				reverse=0;
			else
				reverse=IPF_REVERSE;
			entry[sel/ENTRIES]->type &= ~IPF_REVERSE;
			entry[sel/ENTRIES]->type |= reverse;
		}
		else if ((sel % ENTRIES) == 6)
		/* sensitivity */
		{
			int oldval = (entry[sel/ENTRIES]->arg);
			int val = (oldval & 0xff);

			val ++;
			if (val > 255) val = 255;
			oldval &= ~0xff;
			oldval |= (val & 0xff);
			entry[sel/ENTRIES]->arg = oldval;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		if (sel == total2 - 1) sel = -1;
		else if ((sel % ENTRIES) <= 3) sel |= 0x100;	/* we'll ask for a key/joy */
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		sel = -1;

	if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

static int mame_stats(int selected)
{
	char temp[10];
	char buf[2048];
	int sel;


	sel = selected - 1;

	strcpy(buf, "Tickets dispensed: ");
	if (!dispensed_tickets)
		strcat (buf, "NA\n\n");
	else
	{
		sprintf (temp, "%d\n\n", dispensed_tickets);
		strcat (buf, temp);
	}

	strcat (buf, "Coin A: ");
	if (!coins[0])
		strcat (buf, "NA\n");
	else
	{
		sprintf (temp, "%d\n", coins[0]);
		strcat (buf, temp);
	}

	strcat (buf, "Coin B: ");
	if (!coins[1])
		strcat (buf, "NA\n");
	else
	{
		sprintf (temp, "%d\n", coins[1]);
		strcat (buf, temp);
	}

	strcat (buf, "Coin C: ");
	if (!coins[2])
		strcat (buf, "NA\n");
	else
	{
		sprintf (temp, "%d\n", coins[2]);
		strcat (buf, temp);
	}

	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t\x1a Return to Main Menu \x1b");

		displaymessagewindow(buf);

		if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

int showcopyright(void)
{
#ifndef macintosh /* LBO - This text is displayed in a dialog box. */
	int done;
	char buf[] =
			"Do not distribute the source code and/or the executable "
			"application with any rom images.\n"
			"Doing as such will harm any further development of mame and could "
			"result in legal action being taken by the lawful copyright holders "
			"of any rom images.\n"
			"\n"
			"IF YOU DON'T AGREE WITH THE ABOVE, PRESS ESC.\n"
			"If you agree, type OK.";

	displaymessagewindow(buf);

	done = 0;
	do
	{
		osd_update_video_and_audio();
		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) ||
				osd_key_pressed_memory(OSD_KEY_CANCEL))
			return 1;
		if (osd_key_pressed_memory(OSD_KEY_O) ||
				osd_key_pressed_memory(OSD_KEY_UI_LEFT))
			done = 1;
		if (done == 1 && (osd_key_pressed_memory(OSD_KEY_K) ||
				osd_key_pressed_memory(OSD_KEY_UI_RIGHT)))
			done = 2;
	} while (done < 2);

	osd_clearbitmap(Machine->scrbitmap);
	osd_update_video_and_audio();

#endif

	return 0;
}

static int displaycredits(int selected)
{
	char buf[2048];
	int sel;


	sel = selected - 1;

	sprintf(buf,"The following people\ncontributed to this driver:\n\n%s",Machine->gamedrv->credits);

	if (sel == -1)
	{
		/* startup info, ask for any key */
		strcat(buf,"\n\tPress any key");

		displaymessagewindow(buf);

		sel = 0;
		if (osd_key_pressed_memory (OSD_KEY_ANY))
			sel = -1;
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t\x1a Return to Main Menu \x1b");

		displaymessagewindow(buf);

		if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

void showcredits(void)
{
	while (displaycredits(0) == 1)
		osd_update_video_and_audio();

	osd_clearbitmap(Machine->scrbitmap);
	osd_update_video_and_audio();
}

static int displaygameinfo(int selected)
{
	int i;
	char buf[2048];
	int sel;


	sel = selected - 1;


	sprintf(buf,"%s\n%s %s\n\nCPU:\n",Machine->gamedrv->description,Machine->gamedrv->year,Machine->gamedrv->manufacturer);
	i = 0;
	while (i < MAX_CPU && Machine->drv->cpu[i].cpu_type)
	{
		sprintf(&buf[strlen(buf)],"%s %d.%06d MHz",
				cpu_name(Machine->drv->cpu[i].cpu_type),
				Machine->drv->cpu[i].cpu_clock / 1000000,
				Machine->drv->cpu[i].cpu_clock % 1000000);

		if (Machine->drv->cpu[i].cpu_type & CPU_AUDIO_CPU)
			strcat(buf," (sound)");

		strcat(buf,"\n");

		i++;
	}

	strcat(buf,"\nSound");
	if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
		sprintf(&buf[strlen(buf)]," (stereo)");
	strcat(buf,":\n");

	i = 0;
	while (i < MAX_SOUND && Machine->drv->sound[i].sound_type)
	{
		if (info_sound_num(&Machine->drv->sound[i]))
			sprintf(&buf[strlen(buf)],"%dx",info_sound_num(&Machine->drv->sound[i]));

		sprintf(&buf[strlen(buf)],"%s",info_sound_name(&Machine->drv->sound[i]));

		if (info_sound_clock(&Machine->drv->sound[i]))
			sprintf(&buf[strlen(buf)]," %d.%06d MHz",
					info_sound_clock(&Machine->drv->sound[i]) / 1000000,
					info_sound_clock(&Machine->drv->sound[i]) % 1000000);

		strcat(buf,"\n");

		i++;
	}

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		sprintf(&buf[strlen(buf)],"\nVector Game\n");
	else
	{
		sprintf(&buf[strlen(buf)],"\nScreen resolution:\n");
		if (Machine->gamedrv->orientation & ORIENTATION_SWAP_XY)
			sprintf(&buf[strlen(buf)],"%d x %d (vert) %d Hz\n",
					Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1,
					Machine->drv->visible_area.max_x - Machine->drv->visible_area.min_x + 1,
					Machine->drv->frames_per_second);
		else
			sprintf(&buf[strlen(buf)],"%d x %d (horz) %d Hz\n",
					Machine->drv->visible_area.max_x - Machine->drv->visible_area.min_x + 1,
					Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1,
					Machine->drv->frames_per_second);
		sprintf(&buf[strlen(buf)],"%d colors ",Machine->drv->total_colors);
		if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
			strcat(buf,"(16-bit required)\n");
		else if (Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
			strcat(buf,"(dynamic)\n");
		else strcat(buf,"(static)\n");
	}


	if (sel == -1)
	{
		/* startup info, print MAME version and ask for any key */
		strcat(buf,"\n\tMAME ");	/* \t means that the line will be centered */
		strcat(buf,mameversion);
		strcat(buf,"\n\tPress any key");

		displaymessagewindow(buf);

		sel = 0;
		if (osd_key_pressed_memory (OSD_KEY_ANY))
			sel = -1;
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t\x1a Return to Main Menu \x1b");

		displaymessagewindow(buf);

		if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
			sel = -1;

		if (osd_key_pressed_memory(OSD_KEY_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

int showgamewarnings(void)
{
	int i;
	int counter;
	char buf[2048];

	if (Machine->gamedrv->flags)
	{
		int done;


		strcpy(buf, "There are known problems with this game:\n\n");

		if (Machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
		{
			strcat(buf, "The colors aren't 100% accurate.\n");
		}

		if (Machine->gamedrv->flags & GAME_WRONG_COLORS)
		{
			strcat(buf, "The colors are completely wrong.\n");
		}

		if (Machine->gamedrv->flags & GAME_NOT_WORKING)
		{
			const struct GameDriver *main;
			int foundworking;

			strcpy(buf,"THIS GAME DOESN'T WORK PROPERLY");
			if (Machine->gamedrv->clone_of) main = Machine->gamedrv->clone_of;
			else main = Machine->gamedrv;

			foundworking = 0;
			i = 0;
			while (drivers[i])
			{
				if (drivers[i] == main || drivers[i]->clone_of == main)
				{
					if ((drivers[i]->flags & GAME_NOT_WORKING) == 0)
					{
						if (foundworking == 0)
							strcat(buf,"\n\nThere are clones of this game which work. They are:\n\n");
						foundworking = 1;

						sprintf(&buf[strlen(buf)],"%s\n",drivers[i]->name);
					}
				}
				i++;
			}
		}

		strcat(buf,"\n\nType OK to continue");

		displaymessagewindow(buf);

		done = 0;
		do
		{
			osd_update_video_and_audio();
			if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) ||
					osd_key_pressed_memory(OSD_KEY_CANCEL))
				return 1;
			if (osd_key_pressed_memory(OSD_KEY_O) ||
					osd_key_pressed_memory(OSD_KEY_UI_LEFT))
				done = 1;
			if (done == 1 && (osd_key_pressed_memory(OSD_KEY_K) ||
					osd_key_pressed_memory(OSD_KEY_UI_RIGHT)))
				done = 2;
		} while (done < 2);
	}


	osd_clearbitmap(Machine->scrbitmap);

	/* don't stay on screen for more than 10 seconds */
	counter = 10 * Machine->drv->frames_per_second;

	while (displaygameinfo(0) == 1 && --counter > 0)
		osd_update_video_and_audio();
	osd_update_video_and_audio();

	osd_clearbitmap(Machine->scrbitmap);
	osd_update_video_and_audio();

	return 0;
}



enum { UI_SWITCH = 0,UI_DEFKEY, UI_KEY, UI_JOY,UI_ANALOG, UI_CALIBRATE,
		UI_STATS,UI_CREDITS,UI_GAMEINFO,
		UI_CHEAT,UI_RESET,UI_EXIT };

#define MAX_SETUPMENU_ITEMS 20
static const char *menu_item[MAX_SETUPMENU_ITEMS];
static int menu_action[MAX_SETUPMENU_ITEMS];
static int menu_total;


static void setup_menu_init(void)
{
	menu_total = 0;

	menu_item[menu_total] = "Dip Switches"; menu_action[menu_total++] = UI_SWITCH;
	menu_item[menu_total] = "Default Keys"; menu_action[menu_total++] = UI_DEFKEY;
	menu_item[menu_total] = "Keys for This Game"; menu_action[menu_total++] = UI_KEY;
	menu_item[menu_total] = "Joystick"; menu_action[menu_total++] = UI_JOY;

	/* Determine if there are any analog controls */
	{
		struct InputPort *in;
		int num;

		in = Machine->input_ports;

		num = 0;
		while (in->type != IPT_END)
		{
			if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
					&& !(nocheat && (in->type & IPF_CHEAT)))
				num++;
			in++;
		}

		if (num != 0)
		{
			menu_item[menu_total] = "Analog Controls"; menu_action[menu_total++] = UI_ANALOG;
		}
	}

	/* Joystick calibration possible? */
	if ((osd_joystick_needs_calibration()) != 0)
	{
		menu_item[menu_total] = "Calibrate Joysticks"; menu_action[menu_total++] = UI_CALIBRATE;
	}

	menu_item[menu_total] = "Stats"; menu_action[menu_total++] = UI_STATS;
	menu_item[menu_total] = "Credits"; menu_action[menu_total++] = UI_CREDITS;
	menu_item[menu_total] = "Game Information"; menu_action[menu_total++] = UI_GAMEINFO;

	if (nocheat == 0)
	{
		menu_item[menu_total] = "Cheat"; menu_action[menu_total++] = UI_CHEAT;
	}

	menu_item[menu_total] = "Reset Game"; menu_action[menu_total++] = UI_RESET;
	menu_item[menu_total] = "Return to Game"; menu_action[menu_total++] = UI_EXIT;
	menu_item[menu_total] = 0; /* terminate array */
}


static int setup_menu(int selected)
{
	int sel,res;
	static int menu_lastselected = 0;


	if (selected == -1)
		sel = menu_lastselected;
	else sel = selected - 1;

	if (sel > 0xff)
	{
		switch (menu_action[sel & 0xff])
		{
			case UI_SWITCH:
				res = setdipswitches(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_DEFKEY:
				res = setdefkeysettings(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_KEY:
				res = setkeysettings(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_JOY:
				res = setjoysettings(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_ANALOG:
				res = settraksettings(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_CALIBRATE:
				res = calibratejoysticks(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;


			case UI_STATS:
				res = mame_stats(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_CREDITS:
				res = displaycredits(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_GAMEINFO:
				res = displaygameinfo(sel >> 8);
				if (res == -1)
				{
					menu_lastselected = sel;
					sel = -1;
				}
				else
					sel = (sel & 0xff) | (res << 8);
				break;

			case UI_CHEAT:
osd_sound_enable(0);
while (osd_key_pressed(OSD_KEY_UI_SELECT))
	osd_update_video_and_audio();     /* give time to the sound hardware to apply the volume change */
				cheat_menu();
osd_sound_enable(1);
sel = sel & 0xff;
				break;
		}

		return sel + 1;
	}


	displaymenu(menu_item,0,0,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
		sel = (sel + 1) % menu_total;

	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
		sel = (sel + menu_total - 1) % menu_total;

	if (osd_key_pressed_memory(OSD_KEY_UI_SELECT))
	{
		switch (menu_action[sel])
		{
			case UI_SWITCH:
			case UI_DEFKEY:
			case UI_KEY:
			case UI_JOY:
			case UI_ANALOG:
			case UI_CALIBRATE:
			case UI_STATS:
			case UI_CREDITS:
			case UI_GAMEINFO:
			case UI_CHEAT:
				sel |= 0x100;
				/* tell updatescreen() to clean after us */
				need_to_clear_bitmap = 1;
				break;

			case UI_RESET:
				machine_reset();
				break;

			case UI_EXIT:
				menu_lastselected = 0;
				sel = -1;
				break;
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) ||
			osd_key_pressed_memory(OSD_KEY_CANCEL) ||
			osd_key_pressed_memory(OSD_KEY_CONFIGURE))
	{
		menu_lastselected = sel;
		sel = -1;
	}

	if (sel == -1)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}



/*********************************************************************

  start of On Screen Display handling

*********************************************************************/

static void displayosd(const char *text,int percentage)
{
	struct DisplayText dt[2];
	int avail;


	avail = (Machine->uiwidth / Machine->uifontwidth) * 19 / 20;

	drawbox((Machine->uiwidth - Machine->uifontwidth * avail) / 2,
			(Machine->uiheight - 7*Machine->uifontheight/2),
			avail * Machine->uifontwidth,
			3*Machine->uifontheight);

	avail--;

	drawbar((Machine->uiwidth - Machine->uifontwidth * avail) / 2,
			(Machine->uiheight - 3*Machine->uifontheight),
			avail * Machine->uifontwidth,
			Machine->uifontheight,
			percentage);

	dt[0].text = text;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = (Machine->uiwidth - Machine->uifontwidth * strlen(text)) / 2;
	dt[0].y = (Machine->uiheight - 2*Machine->uifontheight) + 2;
	dt[1].text = 0;	/* terminate array */
	displaytext(dt,0,0);
}



static void onscrd_volume(int increment,int arg)
{
	char buf[20];
	int attenuation;

	if (increment)
	{
		attenuation = osd_get_mastervolume();
		attenuation += increment;
		if (attenuation > 0) attenuation = 0;
		if (attenuation < -32) attenuation = -32;
		osd_set_mastervolume(attenuation);
	}
	attenuation = osd_get_mastervolume();

	sprintf(buf,"Volume %3ddB",attenuation);
	displayosd(buf,100 * (attenuation + 32) / 32);
}

static void onscrd_streamvol(int increment,int arg)
{
	static void *driver = 0;
	char buf[40];
	int volume,ch;
	int doallstreams = 0;
	int proportional = 0;


	if (osd_key_pressed(OSD_KEY_LSHIFT) || osd_key_pressed(OSD_KEY_RSHIFT))
		doallstreams = 1;
	if (!osd_key_pressed(OSD_KEY_LCONTROL) && !osd_key_pressed(OSD_KEY_RCONTROL))
		increment *= 5;
	if (osd_key_pressed(OSD_KEY_ALT) || osd_key_pressed(OSD_KEY_ALTGR))
		proportional = 1;

	if (increment)
	{
		if (proportional)
		{
			static int old_vol[MAX_STREAM_CHANNELS];
			float ratio = 1.0;
			int overflow = 0;

			if (driver != Machine->drv)
			{
				driver = (void *)Machine->drv;
				for (ch = 0; ch < MAX_STREAM_CHANNELS; ch++)
					old_vol[ch] = stream_get_volume(ch);
			}

			volume = stream_get_volume(arg);
			if (old_vol[arg])
				ratio = (float)(volume + increment) / (float)old_vol[arg];

			for (ch = 0; ch < MAX_STREAM_CHANNELS; ch++)
			{
				if (stream_get_name(ch) != 0)
				{
					volume = ratio * old_vol[ch];
					if (volume < 0 || volume > 100)
						overflow = 1;
				}
			}

			if (!overflow)
			{
				for (ch = 0; ch < MAX_STREAM_CHANNELS; ch++)
				{
					volume = ratio * old_vol[ch];
					stream_set_volume(ch, volume);
				}
			}
		}
		else
		{
			driver = 0; /* force reset of saved volumes */

			volume = stream_get_volume(arg);
			volume += increment;
			if (volume > 100) volume = 100;
			if (volume < 0) volume = 0;

			if (doallstreams)
			{
				for (ch = 0;ch < MAX_STREAM_CHANNELS;ch++)
					stream_set_volume(ch,volume);
			}
			else
				stream_set_volume(arg,volume);
		}
	}
	volume = stream_get_volume(arg);

	if (proportional)
		sprintf(buf,"ALL CHANNELS Relative %3d%%", volume);
	else if (doallstreams)
		sprintf(buf,"ALL CHANNELS Volume %3d%%",volume);
	else
		sprintf(buf,"%s Volume %3d%%",stream_get_name(arg),volume);
	displayosd(buf,volume);
}

static void onscrd_brightness(int increment,int arg)
{
	char buf[20];
	int brightness;


	if (increment)
	{
		brightness = osd_get_brightness();
		brightness += 5 * increment;
		if (brightness < 0) brightness = 0;
		if (brightness > 100) brightness = 100;
		osd_set_brightness(brightness);
	}
	brightness = osd_get_brightness();

	sprintf(buf,"Brightness %3d%%",brightness);
	displayosd(buf,brightness);
}

static void onscrd_gamma(int increment,int arg)
{
	char buf[20];
	float gamma_correction;


	if (increment)
	{
		gamma_correction = osd_get_gamma();
		gamma_correction += 0.05 * increment;
		if (gamma_correction < 0.5) gamma_correction = 0.5;
		if (gamma_correction > 2.0) gamma_correction = 2.0;
		osd_set_gamma(gamma_correction);
	}
	gamma_correction = osd_get_gamma();

	sprintf(buf,"Gamma %1.2f",gamma_correction);
	displayosd(buf,100*(gamma_correction-0.5)/(2.0-0.5));
}


#define MAX_OSD_ITEMS 30
static void (*onscrd_fnc[MAX_OSD_ITEMS])(int increment,int arg);
static int onscrd_arg[MAX_OSD_ITEMS];
static int onscrd_total_items;

static void onscrd_init(void)
{
	int item,ch;


	item = 0;

	onscrd_fnc[item] = onscrd_volume;
	onscrd_arg[item] = 0;
	item++;

	for (ch = 0;ch < MAX_STREAM_CHANNELS;ch++)
	{
		if (stream_get_name(ch) != 0)
		{
			onscrd_fnc[item] = onscrd_streamvol;
			onscrd_arg[item] = ch;
			item++;
		}
	}

	onscrd_fnc[item] = onscrd_brightness;
	onscrd_arg[item] = 0;
	item++;

	onscrd_fnc[item] = onscrd_gamma;
	onscrd_arg[item] = 0;
	item++;

	onscrd_total_items = item;
}

static int on_screen_display(int selected)
{
	int increment,sel;
	static int lastselected = 0;


	if (selected == -1)
		sel = lastselected;
	else sel = selected - 1;

	increment = 0;
	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_LEFT,8))
        increment = -1;
	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_RIGHT,8))
        increment = 1;
	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
		sel = (sel + 1) % onscrd_total_items;
	if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
		sel = (sel + onscrd_total_items - 1) % onscrd_total_items;

	(*onscrd_fnc[sel])(increment,onscrd_arg[sel]);

	lastselected = sel;

	if (osd_key_pressed_memory(OSD_KEY_ON_SCREEN_DISPLAY))
	{
		sel = -1;

		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

/*********************************************************************

  end of On Screen Display handling

*********************************************************************/


static void displaymessage(const char *text)
{
	struct DisplayText dt[2];
	int avail;


	avail = strlen(text)+2;

	drawbox((Machine->uiwidth - Machine->uifontwidth * avail) / 2,
			Machine->uiheight - 3*Machine->uifontheight,
			avail * Machine->uifontwidth,
			2*Machine->uifontheight);

	dt[0].text = text;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = (Machine->uiwidth - Machine->uifontwidth * strlen(text)) / 2;
	dt[0].y = Machine->uiheight - 5*Machine->uifontheight/2;
	dt[1].text = 0;	/* terminate array */
	displaytext(dt,0,0);
}


static char messagetext[80];
static int messagecounter;

void usrintf_showmessage(const char *text)
{
	strcpy(messagetext,text);
	messagecounter = 2 * Machine->drv->frames_per_second;
}




static int setup_selected;
static int osd_selected;
static int jukebox_selected;

int handle_user_interface(void)
{
	static int show_total_colors;


	/* if the user pressed F12, save the screen to a file */
	if (osd_key_pressed_memory(OSD_KEY_SNAPSHOT))
		osd_save_snapshot();

	/* This call is for the cheat, it must be called at least each frames */
	if (nocheat == 0) DoCheat();

	if (osd_key_pressed_memory (OSD_KEY_FAST_EXIT)) return 1;

	/* if the user pressed ESC, stop the emulation */
	/* but don't quit if the setup menu is on screen */
	if (setup_selected == 0 && osd_key_pressed_memory(OSD_KEY_CANCEL))
		return 1;

	if (setup_selected == 0 && osd_key_pressed_memory(OSD_KEY_CONFIGURE))
	{
		setup_selected = -1;
		if (osd_selected != 0)
		{
			osd_selected = 0;	/* disable on screen display */
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}
	if (setup_selected != 0) setup_selected = setup_menu(setup_selected);

	if (osd_selected == 0 && osd_key_pressed_memory(OSD_KEY_ON_SCREEN_DISPLAY))
	{
		osd_selected = -1;
		if (setup_selected != 0)
		{
			setup_selected = 0;	/* disable setup menu */
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}
	if (osd_selected != 0) osd_selected = on_screen_display(osd_selected);


#if 0
	if (osd_key_pressed_memory(OSD_KEY_BACKSPACE))
	{
		if (jukebox_selected != -1)
		{
			jukebox_selected = -1;
			cpu_halt(0,1);
		}
		else
		{
			jukebox_selected = 0;
			cpu_halt(0,0);
		}
	}

	if (jukebox_selected != -1)
	{
		char buf[40];
		watchdog_reset_w(0,0);
		if (osd_key_pressed_memory(OSD_KEY_LCONTROL))
		{
#include "cpu/z80/z80.h"
			soundlatch_w(0,jukebox_selected);
			cpu_cause_interrupt(1,Z80_NMI_INT);
		}
		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_RIGHT,8))
		{
			jukebox_selected = (jukebox_selected + 1) & 0xff;
		}
		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_LEFT,8))
		{
			jukebox_selected = (jukebox_selected - 1) & 0xff;
		}
		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_UP,8))
		{
			jukebox_selected = (jukebox_selected + 16) & 0xff;
		}
		if (osd_key_pressed_memory_repeat(OSD_KEY_UI_DOWN,8))
		{
			jukebox_selected = (jukebox_selected - 16) & 0xff;
		}
		sprintf(buf,"sound cmd %02x",jukebox_selected);
		displaymessage(buf);
	}
#endif


	/* if the user pressed F3, reset the emulation */
	if (osd_key_pressed_memory(OSD_KEY_RESET_MACHINE))
		machine_reset();


	if (osd_key_pressed_memory(OSD_KEY_PAUSE)) /* pause the game */
	{
		float orig_brt;
		int pressed;


/*		osd_selected = 0;	   disable on screen display, since we are going   */
							/* to change parameters affected by it */

		osd_sound_enable(0);
		orig_brt = osd_get_brightness();
		osd_set_brightness(orig_brt * 0.65);

		pressed = 1;

		while (pressed || osd_key_pressed_memory(OSD_KEY_UNPAUSE) == 0)
		{
#ifdef MAME_NET
			osd_net_sync();
#endif /* MAME_NET */
			osd_profiler(OSD_PROFILE_VIDEO);
			if (need_to_clear_bitmap || bitmap_dirty)
			{
				osd_clearbitmap(Machine->scrbitmap);
				need_to_clear_bitmap = 0;
				(*Machine->drv->vh_update)(Machine->scrbitmap,bitmap_dirty);
				bitmap_dirty = 0;
			}
#ifdef MAME_DEBUG
/* keep calling vh_screenrefresh() while paused so we can stuff */
/* debug code in there */
(*Machine->drv->vh_update)(Machine->scrbitmap,bitmap_dirty);
bitmap_dirty = 0;
#endif
			osd_profiler(OSD_PROFILE_END);

			if (osd_key_pressed_memory (OSD_KEY_FAST_EXIT)) return 1;

			if (setup_selected == 0 && osd_key_pressed_memory(OSD_KEY_CANCEL))
				return 1;

			if (setup_selected == 0 && osd_key_pressed_memory(OSD_KEY_CONFIGURE))
			{
				setup_selected = -1;
				if (osd_selected != 0)
				{
					osd_selected = 0;	/* disable on screen display */
					/* tell updatescreen() to clean after us */
					need_to_clear_bitmap = 1;
				}
			}
			if (setup_selected != 0) setup_selected = setup_menu(setup_selected);

			if (osd_selected == 0 && osd_key_pressed_memory(OSD_KEY_ON_SCREEN_DISPLAY))
			{
				osd_selected = -1;
				if (setup_selected != 0)
				{
					setup_selected = 0;	/* disable setup menu */
					/* tell updatescreen() to clean after us */
					need_to_clear_bitmap = 1;
				}
			}
			if (osd_selected != 0) osd_selected = on_screen_display(osd_selected);

			/* show popup message if any */
			if (messagecounter > 0) displaymessage(messagetext);

			osd_update_video_and_audio();

			if (!osd_key_pressed(OSD_KEY_PAUSE)) pressed = 0;
		}

		osd_sound_enable(1);
		osd_set_brightness(orig_brt);
	}


	/* show popup message if any */
	if (messagecounter > 0)
	{
		displaymessage(messagetext);

		if (--messagecounter == 0)
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
	}


#ifdef MAME_DEBUG
	if (osd_key_pressed_memory(OSD_KEY_SHOW_TOTAL_COLORS))
	{
		show_total_colors ^= 1;
		if (show_total_colors == 0)
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
	}

	if (show_total_colors)
	{
		showtotalcolors();
	}
#endif


	/* if the user pressed F4, show the character set */
	if (osd_key_pressed_memory(OSD_KEY_SHOW_GFX))
	{
		osd_sound_enable(0);

		showcharset();

		osd_sound_enable(1);
	}

	return 0;
}


void init_user_interface(void)
{
	setup_menu_init();
	setup_selected = 0;

	onscrd_init();
	osd_selected = 0;

	jukebox_selected = -1;
}

int onscrd_active(void)
{
    return osd_selected;
}

int setup_active(void)
{
    return setup_selected;
}
