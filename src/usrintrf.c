/*********************************************************************

  usrintrf.c

  Functions used to handle MAME's crude user interface.

*********************************************************************/

#include "driver.h"

extern int need_to_clear_bitmap;	/* used to tell updatescreen() to clear the bitmap */

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
	struct GfxElement *font;
	static unsigned short colortable[2*2];	/* ASG 980209 */
	int trueorientation;


	/* hack: force the display into standard orientation to avoid */
	/* creating a rotated font */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	if (Machine->uiwidth >= 420 && Machine->uiheight >= 420)
		font = decodegfx(fontdata6x8,&fontlayout12x16);
	else
		font = decodegfx(fontdata6x8,&fontlayout6x8);

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
	Machine->orientation = ORIENTATION_DEFAULT;

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
				y += Machine->uifont->height + 1;
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
						nextlen += Machine->uifont->width;
						nc++;
					}

					/* word wrap */
					if (x + Machine->uifont->width + nextlen > Machine->uiwidth)
					{
						x = dt->x;
						y += Machine->uifont->height + 1;
						wrapped = 1;
					}
				}
			}

			if (!wrapped)
			{
				drawgfx(Machine->scrbitmap,Machine->uifont,*c,dt->color,0,0,x+Machine->uixmin,y+Machine->uiymin,0,TRANSPARENCY_NONE,0);
				x += Machine->uifont->width;
			}

			c++;
		}

		dt++;
	}

	Machine->orientation = trueorientation;

	if (update_screen) osd_update_display();
}

static inline void drawpixel(int x, int y, unsigned short color)
{
	if (Machine->scrbitmap->depth == 16)
		*(unsigned short *)&Machine->scrbitmap->line[y][x*2] = color;
	else
		Machine->scrbitmap->line[y][x] = color;
}

static inline void drawhline(int x, int w, int y, unsigned short color)
{
	if (Machine->scrbitmap->depth == 16)
	{
        int i;
		for (i = x; i < x+w; i++)
			*(unsigned short *)&Machine->scrbitmap->line[y][i*2] = color;
	}
	else
		memset(&Machine->scrbitmap->line[y][x], color, w);
}

static inline void drawvline(int x, int y, int h, unsigned short color)
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
}


static void drawbox(int leftx,int topy,int width,int height)
{
	int x,y;
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
	int x,y;
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


void displaymenu(const char **items,const char **subitems,int selected,int arrowize_subitem)
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
	if (maxlen * Machine->uifont->width > Machine->uiwidth)
		maxlen = Machine->uiwidth / Machine->uifont->width;

	visible = Machine->uiheight / (3 * Machine->uifont->height / 2) - 1;
	topitem = 0;
	if (visible > count) visible = count;
	else
	{
		topitem = selected - visible / 2;
		if (topitem < 0) topitem = 0;
		if (topitem > count - visible) topitem = count - visible;
	}

	leftoffs = (Machine->uiwidth - maxlen * Machine->uifont->width) / 2;
	topoffs = (Machine->uiheight - (3 * visible + 1) * Machine->uifont->height / 2) / 2;

	/* black background */
	drawbox(leftoffs,topoffs,maxlen * Machine->uifont->width,(3 * visible + 1) * Machine->uifont->height / 2);

	curr_dt = 0;
	for (i = 0;i < visible;i++)
	{
		int item = i + topitem;

		if (i == 0 && item > 0)
		{
			dt[curr_dt].text = uparrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifont->width * strlen(uparrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
			curr_dt++;
		}
		else if (i == visible - 1 && item < count - 1)
		{
			dt[curr_dt].text = downarrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifont->width * strlen(downarrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
			curr_dt++;
		}
		else
		{
			if (subitems && subitems[item])
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = leftoffs + 3*Machine->uifont->width/2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
				curr_dt++;
				dt[curr_dt].text = subitems[item];
				dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = leftoffs + Machine->uifont->width * (maxlen-1 - strlen(subitems[item])) - Machine->uifont->width/2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
				curr_dt++;
			}
			else
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = DT_COLOR_WHITE;
				dt[curr_dt].x = (Machine->uiwidth - Machine->uifont->width * strlen(items[item])) / 2;
				dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
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
			dt[curr_dt].x = leftoffs + Machine->uifont->width * (maxlen-2 - strlen(subitems[selected])) - Machine->uifont->width/2 - 1;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
			curr_dt++;
		}
		if (arrowize_subitem & 2)
		{
			dt[curr_dt].text = rightarrow;
			dt[curr_dt].color = DT_COLOR_WHITE;
			dt[curr_dt].x = leftoffs + Machine->uifont->width * (maxlen-1) - Machine->uifont->width/2;
			dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
			curr_dt++;
		}
	}
	else
	{
		dt[curr_dt].text = righthilight;
		dt[curr_dt].color = DT_COLOR_WHITE;
		dt[curr_dt].x = leftoffs + Machine->uifont->width * (maxlen-1) - Machine->uifont->width/2;
		dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
		curr_dt++;
	}
	dt[curr_dt].text = lefthilight;
	dt[curr_dt].color = DT_COLOR_WHITE;
	dt[curr_dt].x = leftoffs + Machine->uifont->width/2;
	dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
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


	strcpy(textcopy,text);

	maxlen = 0;
	lines = 0;
	c = textcopy;
	while (*c)
	{
		len = 0;
		while (*c && *c != '\n')
		{
			len++;
			c++;
		}

		if (*c == '\n') c++;

		if (len > maxlen) maxlen = len;

		lines++;
	}

	maxlen += 1;

	leftoffs = (Machine->uiwidth - Machine->uifont->width * maxlen) / 2;
	if (leftoffs < 0) leftoffs = 0;
	topoffs = (Machine->uiheight - (3 * lines + 1) * Machine->uifont->height / 2) / 2;

	/* black background */
	drawbox(leftoffs,topoffs,maxlen * Machine->uifont->width,(3 * lines + 1) * Machine->uifont->height / 2);

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
			dt[curr_dt].x = (Machine->uiwidth - Machine->uifont->width * (c - c2)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + Machine->uifont->width/2;

		dt[curr_dt].text = c2;
		dt[curr_dt].color = DT_COLOR_WHITE;
		dt[curr_dt].y = topoffs + (3*i+1)*Machine->uifont->height/2;
		curr_dt++;

		i++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(dt,0,0);
}




static void showcharset(void)
{
	int i,cpx,cpy;
	struct DisplayText dt[2];
	char buf[80];
	int bank,color,line, maxline;
	int trueorientation;


	if ((Machine->drv->gfxdecodeinfo == 0) ||
			(Machine->drv->gfxdecodeinfo[0].memory_region == -1))
		return;	/* no gfx sets, return */


	/* hack: force the display into standard orientation to avoid */
	/* rotating the user interface */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;


	bank = 0;
	color = 0;
	line = 0;

	do
	{
		osd_clearbitmap(Machine->scrbitmap);

		cpx = Machine->uiwidth / Machine->gfx[bank]->width;
		cpy = Machine->uiheight / Machine->gfx[bank]->height;

		maxline = (Machine->drv->gfxdecodeinfo[bank].gfxlayout->total + cpx - 1) / cpx;

		for (i = 0; i+(line*cpx) < Machine->drv->gfxdecodeinfo[bank].gfxlayout->total ; i++)
		{
			drawgfx(Machine->scrbitmap,Machine->gfx[bank],
				i+(line*cpx),color,  /*sprite num, color*/
				0,0,
				(i % cpx) * Machine->gfx[bank]->width + Machine->uixmin,
				Machine->uifont->height+1 + (i / cpx) * Machine->gfx[bank]->height + Machine->uiymin,
				0,TRANSPARENCY_NONE,0);
		}

		sprintf(buf,"GFXSET %d COLOR %d LINE %d ",bank,color,line);
		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,0,1);

		if (osd_key_pressed_memory_repeat(OSD_KEY_RIGHT,8))
		{
			if (bank+1 < MAX_GFX_ELEMENTS && Machine->gfx[bank + 1])
			{
				bank++;
				line = 0;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_LEFT,8))
		{
			if (bank > 0)
			{
				bank--;
				line = 0;
			}
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_PGDN,4))
		{
			if (line < maxline-1) line++;
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_PGUP,4))
		{
			if (line > 0) line--;
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_UP,6))
		{
			if (color < Machine->drv->gfxdecodeinfo[bank].total_color_codes - 1)
				color++;
		}

		if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,6))
		{
			if (color > 0) color--;
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




static int setdipswitches(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
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

	displaymenu(menu_item,menu_subitem,sel,arrowize);

	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_RIGHT,8))
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
		}
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_LEFT,8))
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
		}
	}

	if (osd_key_pressed_memory(OSD_KEY_ENTER))
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



static int setkeysettings(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
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
			menu_subitem[i] = osd_key_name(default_key(entry[i]));
		else menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > 255)	/* are we waiting for a new key? */
	{
		int newkey;


		menu_subitem[sel & 0xff] = "    ";
		displaymenu(menu_item,menu_subitem,sel & 0xff,3);
		newkey = osd_read_key_immediate();
		if (newkey != OSD_KEY_NONE)
		{
			sel &= 0xff;

			if (key_to_pseudo_code(newkey) != newkey)	/* pseudo key code ? */
				entry[sel]->keyboard = IP_KEY_DEFAULT;
			else
				entry[sel]->keyboard = newkey;
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory(OSD_KEY_ENTER))
	{
		if (sel == total - 1) sel = -1;
		else sel |= 0x100;	/* we'll ask for a key */
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



#ifdef macintosh
static int setjoysettings(int selected) { return 0; }
#else
static int setjoysettings(int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
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
			menu_subitem[i] = osd_joy_name(default_joy(entry[i]));
		else menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > 255)	/* are we waiting for a new joy direction? */
	{
		int newjoy;
		int joyindex;


		menu_subitem[sel & 0xff] = "    ";
		displaymenu(menu_item,menu_subitem,sel & 0xff,3);

		/* Check all possible joystick values for switch or button press */
		if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
		{
			sel &= 0xff;
			entry[sel]->joystick = IP_JOY_DEFAULT;
		}

		/* Allows for "All buttons" */
		if (osd_key_pressed_memory(OSD_KEY_A))
		{
			sel &= 0xff;
			entry[sel]->joystick = OSD_JOY_FIRE;
		}
		if (osd_key_pressed_memory(OSD_KEY_B))
		{
			sel &= 0xff;
			entry[sel]->joystick = OSD_JOY2_FIRE;
		}
		/* Clears entry "None" */
		if (osd_key_pressed_memory(OSD_KEY_N))
		{
			sel &= 0xff;
			entry[sel]->joystick = 0;
		}

		for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++)
		{
			newjoy = osd_joy_pressed(joyindex);
			if (newjoy)
			{
				sel &= 0xff;
				entry[sel]->joystick = joyindex;
				break;
			}
		}

		return sel + 1;
	}


	displaymenu(menu_item,menu_subitem,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}

	if (osd_key_pressed_memory(OSD_KEY_ENTER))
	{
		if (sel == total - 1) sel = -1;
		else sel |= 0x100;	/* we'll ask for a joy */
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
#endif


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
				displaymenu(menu_item,menu_subitem,sel & 0xff,3);
				newkey = osd_read_key_immediate();
				if (newkey != OSD_KEY_NONE)
				{
					sel &= 0xff;

					if (key_to_pseudo_code(newkey) != newkey)	/* pseudo key code ? */
						newkey = 0;//IP_KEY_DEFAULT;

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
				displaymenu(menu_item,menu_subitem,sel & 0xff,3);

				/* Check all possible joystick values for switch or button press */
				newjoy = -1;

				if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT) || osd_key_pressed_memory (OSD_KEY_CANCEL))
				{
					sel &= 0xff;
					newjoy = 0;//IP_JOY_DEFAULT;
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


	displaymenu(menu_item,menu_subitem,sel,arrowize);

	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
	{
		if (sel < total2 - 1) sel++;
		else sel = 0;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total2 - 1;
	}

	if (osd_key_pressed_memory_repeat(OSD_KEY_LEFT,8))
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

	if (osd_key_pressed_memory_repeat(OSD_KEY_RIGHT,8))
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

	if (osd_key_pressed_memory(OSD_KEY_ENTER))
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
	int key;
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

	displaymessagewindow(buf);

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT)
			|| osd_key_pressed_memory (OSD_KEY_CANCEL)
			|| osd_key_pressed_memory(OSD_KEY_ENTER)
			|| osd_key_pressed_memory(OSD_KEY_LEFT)
			|| osd_key_pressed_memory(OSD_KEY_RIGHT)
			|| osd_key_pressed_memory(OSD_KEY_UP)
			|| osd_key_pressed_memory(OSD_KEY_DOWN))
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

int showcopyright(void)
{
#ifndef macintosh /* LBO - This text is displayed in a dialog box. */
	int key;
	struct DisplayText dt[2];


	dt[0].text = "PLEASE DO NOT DISTRIBUTE THE SOURCE CODE AND/OR THE EXECUTABLE "
			"APPLICATION WITH ANY ROM IMAGES.\n"
			"DOING AS SUCH WILL HARM ANY FURTHER DEVELOPMENT OF MAME AND COULD "
			"RESULT IN LEGAL ACTION BEING TAKEN BY THE LAWFUL COPYRIGHT HOLDERS "
			"OF ANY ROM IMAGES.";

	dt[0].color = DT_COLOR_RED;
	dt[0].x = 0;
	dt[0].y = 0;
	dt[1].text = 0;
	displaytext(dt,1,1);

	key = osd_read_key(1);
	while (osd_key_pressed(key)) ;	/* wait for key release */
	if (key == OSD_KEY_FAST_EXIT ||
		key == OSD_KEY_CANCEL ||
		key == OSD_KEY_ESC)
		return 1;
#endif

	osd_clearbitmap(Machine->scrbitmap);
	osd_update_display();

	return 0;
}

static int displaycredits(int selected)
{
	int key;
	char buf[2048];
	int sel;


	sel = selected - 1;

	sprintf(buf,"The following people\ncontributed to this driver:\n\n%s",Machine->gamedrv->credits);

	displaymessagewindow(buf);

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT)
			|| osd_key_pressed_memory (OSD_KEY_CANCEL)
			|| osd_key_pressed_memory(OSD_KEY_ENTER)
			|| osd_key_pressed_memory(OSD_KEY_LEFT)
			|| osd_key_pressed_memory(OSD_KEY_RIGHT)
			|| osd_key_pressed_memory(OSD_KEY_UP)
			|| osd_key_pressed_memory(OSD_KEY_DOWN))
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

void showcredits(void)
{
	while (displaycredits(1) == 1)
		osd_update_display();
	osd_update_display();
}

static int displaygameinfo(int selected)
{
	int i;
	char buf[2048];
	static char *cpunames[] =
	{
		"",
		"Z80",
		"I8085",
		"6502",
		"8086",
		"8035",
		"6803",
		"6805",
		"6809",
		"68000",
		"T-11",
		"S2650",
		"TMS34010",
		"TMS9900"
	};
	static char *soundnames[] =
	{
		"",
		"<custom>",
		"<samples>",
		"DAC",
		"AY-3-8910",
		"YM2203",
		"YM2151",
		"YM2151",
		"YM2413",
		"YM2610",
		"YM3812",
		"SN76496",
		"Pokey",
		"Namco",
		"NES",
		"TMS5220",
		"VLM5030",
		"ADPCM samples",
		"OKIM6295 ADPCM",
		"MSM5205 ADPCM",
		"HC-55516 CVSD",
		"Astrocade 'IO' chip"
	};
	int sel;


	sel = selected - 1;


	sprintf(buf,"%s\n%s %s\n\nCPU:\n",Machine->gamedrv->description,Machine->gamedrv->year,Machine->gamedrv->manufacturer);
	i = 0;
	while (i < MAX_CPU && Machine->drv->cpu[i].cpu_type)
	{
		sprintf(&buf[strlen(buf)],"%s %d.%06d MHz",
				cpunames[Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK],
				Machine->drv->cpu[i].cpu_clock / 1000000,
				Machine->drv->cpu[i].cpu_clock % 1000000);

		if (Machine->drv->cpu[i].cpu_type & CPU_AUDIO_CPU)
			strcat(buf," (sound)");

		strcat(buf,"\n");

		i++;
	}

	sprintf(&buf[strlen(buf)],"\nSound:\n");
	i = 0;
	while (i < MAX_SOUND && Machine->drv->sound[i].sound_type)
	{
		if (Machine->drv->sound[i].sound_type >= SOUND_AY8910 &&
				Machine->drv->sound[i].sound_type <= SOUND_POKEY)
			sprintf(&buf[strlen(buf)],"%d x ",((struct AY8910interface *)Machine->drv->sound[i].sound_interface)->num);

		sprintf(&buf[strlen(buf)],"%s",
				soundnames[Machine->drv->sound[i].sound_type]);

		if (Machine->drv->sound[i].sound_type >= SOUND_AY8910 &&
				Machine->drv->sound[i].sound_type <= SOUND_POKEY)
			sprintf(&buf[strlen(buf)]," %d.%06d MHz",
					((struct AY8910interface *)Machine->drv->sound[i].sound_interface)->clock / 1000000,
					((struct AY8910interface *)Machine->drv->sound[i].sound_interface)->clock % 1000000);

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

	strcat(buf,"\n\n\tMAME ");	/* \t means that the line will be centered */
	strcat(buf,mameversion);


	displaymessagewindow(buf);

	if (osd_key_pressed_memory(OSD_KEY_FAST_EXIT)
			|| osd_key_pressed_memory (OSD_KEY_CANCEL)
			|| osd_key_pressed_memory(OSD_KEY_ENTER)
			|| osd_key_pressed_memory(OSD_KEY_LEFT)
			|| osd_key_pressed_memory(OSD_KEY_RIGHT)
			|| osd_key_pressed_memory(OSD_KEY_UP)
			|| osd_key_pressed_memory(OSD_KEY_DOWN))
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

int showgamewarnings(void)
{
	int i;
	int key;
	int counter;
	char buf[2048];
	struct DisplayText dt[3];

	if (Machine->gamedrv->flags)
	{
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
							strcat(buf,"\n\n\nThere are clones of this game which work. They are:\n\n");
						foundworking = 1;

						sprintf(&buf[strlen(buf)],"%s\n",drivers[i]->name);
					}
				}
				i++;
			}
		}

		strcat(buf,"\n\n\nType OK to continue");

		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,1,1);

		do
		{
			key = osd_read_key(1);
			if (key == OSD_KEY_ESC ||
				key == OSD_KEY_CANCEL ||
				key == OSD_KEY_FAST_EXIT)
			{
				while (osd_key_pressed(key)) ;	/* wait for key release */
				return 1;
			}
		} while (key != OSD_KEY_O && key != OSD_KEY_LEFT);

		while (osd_key_pressed(OSD_KEY_K) == 0 && osd_key_pressed(OSD_KEY_RIGHT) == 0) ;
		while (osd_key_pressed(OSD_KEY_K) || osd_key_pressed(OSD_KEY_RIGHT)) ;
	}


	osd_clearbitmap(Machine->scrbitmap);

	/* don't stay on screen for more than 10 seconds */
	counter = 10 * Machine->drv->frames_per_second;

	while (displaygameinfo(1) == 1 && --counter > 0)
		osd_update_display();
	osd_update_display();

	osd_clearbitmap(Machine->scrbitmap);
	osd_update_display();

	return 0;
}



enum { UI_SWITCH = 0,UI_KEY, UI_JOY,UI_ANALOG,
		UI_STATS,UI_CREDITS,UI_GAMEINFO,
		UI_CHEAT,UI_RESET,UI_EXIT };

#define MAX_SETUPMENU_ITEMS 20
static const char *menu_item[MAX_SETUPMENU_ITEMS];
static int menu_action[MAX_SETUPMENU_ITEMS];
static int menu_total;


static void setup_menu_init(void)
{
	menu_total = 0;

	menu_item[menu_total] = "Dip Switch Setup"; menu_action[menu_total++] = UI_SWITCH;
	menu_item[menu_total] = "Keyboard Setup"; menu_action[menu_total++] = UI_KEY;
	menu_item[menu_total] = "Joystick Setup"; menu_action[menu_total++] = UI_JOY;

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
			menu_item[menu_total] = "Analog Setup"; menu_action[menu_total++] = UI_ANALOG;
		}
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
while (osd_key_pressed(OSD_KEY_ENTER))
	osd_update_audio();     /* give time to the sound hardware to apply the volume change */
				cheat_menu();
osd_sound_enable(1);
sel = sel & 0xff;
				break;
		}

		return sel + 1;
	}


	displaymenu(menu_item,0,sel,0);

	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
		sel = (sel + 1) % menu_total;

	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
		sel = (sel + menu_total - 1) % menu_total;

	if (osd_key_pressed_memory(OSD_KEY_ENTER))
	{
		switch (menu_action[sel])
		{
			case UI_SWITCH:
			case UI_KEY:
			case UI_JOY:
			case UI_ANALOG:
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
	int i,avail;


	avail = (Machine->uiwidth / Machine->uifont->width) * 19 / 20;

	drawbox((Machine->uiwidth - Machine->uifont->width * avail) / 2,
			(Machine->uiheight - 7*Machine->uifont->height/2),
			avail * Machine->uifont->width,
			3*Machine->uifont->height);

	avail--;

	drawbar((Machine->uiwidth - Machine->uifont->width * avail) / 2,
			(Machine->uiheight - 3*Machine->uifont->height),
			avail * Machine->uifont->width,
			Machine->uifont->height,
			percentage);

	dt[0].text = text;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(text)) / 2;
	dt[0].y = (Machine->uiheight - 2*Machine->uifont->height) + 2;
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
	char buf[40];
	int volume,ch;
	int doallstreams = 0;


	if (osd_key_pressed(OSD_KEY_LSHIFT) || osd_key_pressed(OSD_KEY_RSHIFT))
		doallstreams = 1;
	if (!osd_key_pressed(OSD_KEY_LCONTROL) && !osd_key_pressed(OSD_KEY_RCONTROL))
		increment *= 5;

	if (increment)
	{
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
	volume = stream_get_volume(arg);

	if (doallstreams)
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
	if (osd_key_pressed_memory_repeat(OSD_KEY_LEFT,8))
        increment = -1;
	if (osd_key_pressed_memory_repeat(OSD_KEY_RIGHT,8))
        increment = 1;
	if (osd_key_pressed_memory_repeat(OSD_KEY_DOWN,8))
		sel = (sel + 1) % onscrd_total_items;
	if (osd_key_pressed_memory_repeat(OSD_KEY_UP,8))
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


static char messagetext[80];
static int messagecounter;

static void displaymessage(void)
{
	struct DisplayText dt[2];
	int i,avail;


	if (messagecounter > 0)
	{
		avail = strlen(messagetext)+2;

		drawbox((Machine->uiwidth - Machine->uifont->width * avail) / 2,
				Machine->uiheight - 3*Machine->uifont->height,
				avail * Machine->uifont->width,
				2*Machine->uifont->height);

		dt[0].text = messagetext;
		dt[0].color = DT_COLOR_WHITE;
		dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(messagetext)) / 2;
		dt[0].y = Machine->uiheight - 5*Machine->uifont->height/2;
		dt[1].text = 0;	/* terminate array */
		displaytext(dt,0,0);

		if (--messagecounter == 0)
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
	}
}

void usrintf_showmessage(const char *text)
{
	strcpy(messagetext,text);
	messagecounter = 2 * Machine->drv->frames_per_second;
}




static int setup_selected;
static int osd_selected;

int handle_user_interface(void)
{
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


	/* if the user pressed F3, reset the emulation */
	if (osd_key_pressed_memory(OSD_KEY_RESET_MACHINE))
		machine_reset();


	if (osd_key_pressed_memory(OSD_KEY_PAUSE)) /* pause the game */
	{
		float orig_brt;
		int pressed;


//		osd_selected = 0;	/* disable on screen display, since we are going */
							/* to change parameters affected by it */

		osd_sound_enable(0);
		orig_brt = osd_get_brightness();
		osd_set_brightness(orig_brt * 0.65);

		pressed = 1;

		while (pressed || osd_key_pressed_memory(OSD_KEY_UNPAUSE) == 0)
		{
			osd_profiler(OSD_PROFILE_VIDEO);
			(*Machine->drv->vh_update)(Machine->scrbitmap,1);
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

			osd_update_display();
			osd_update_audio();

			if (!osd_key_pressed(OSD_KEY_PAUSE)) pressed = 0;
		}

		osd_sound_enable(1);
		osd_set_brightness(orig_brt);
	}


	displaymessage();	/* show popup message if any */


	/* if the user pressed F4, show the character set */
	if (osd_key_pressed_memory(OSD_KEY_SHOW_GFX))
	{
		osd_sound_enable(0);

		while (osd_key_pressed(OSD_KEY_SHOW_GFX))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

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
}
