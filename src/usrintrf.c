/*********************************************************************

  usrintrf.c

  Functions used to handle MAME's crude user interface.

Changes:
	042898 - LBO
	* Changed UI menu to be more dynamic, added option to display stats

*********************************************************************/

#include "driver.h"

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



static int findbestcolor(unsigned char r,unsigned char g,unsigned char b)
{
	int i;
	int best,mindist;

	mindist = 200000;
	best = 0;

	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
		return rgbpen (r, g, b);

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		unsigned char r1,g1,b1;
		int d1,d2,d3,dist;

		osd_get_pen(Machine->pens[i],&r1,&g1,&b1);

		/* don't pick black for non-black colors */
		if (r1+g1+b1 > 0 || r+g+b == 0)
		{
			d1 = (int)r1 - r;
			d2 = (int)g1 - g;
			d3 = (int)b1 - b;
			dist = d1*d1 + d2*d2 + d3*d3;

			if (dist < mindist)
			{
				best = i;
				mindist = dist;
			}
		}
	}

	return Machine->pens[best];
}



struct GfxElement *builduifont(void)
{
	static unsigned char fontdata[] =
	{
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,0x2A,0x2A,0x2A,0x2A,0x6A,
           0x00,0x22,0x20,0x20,0x20,0x23,0x22,0x66,0x00,0x20,0x40,0x80,0x00,0x00,0x20,0x60,
           0x00,0x02,0x22,0x22,0x22,0x22,0x22,0x66,0x00,0x22,0x22,0x20,0x23,0x22,0x22,0x66,
           0x00,0x20,0x20,0xE0,0x00,0x00,0x20,0x60,0x00,0x00,0x2A,0x2A,0x2A,0x2A,0x2A,0x6A,
           0x00,0x38,0x22,0x22,0x02,0x3A,0x22,0x62,0x00,0x3C,0x20,0x00,0x38,0x20,0x00,0x7E,
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
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0xFC,0xD4,0xD4,0xD4,0xD4,0xD4,0x00,
           0x4C,0xCC,0xC7,0xC3,0xC7,0xCC,0xCC,0x00,0x40,0xC0,0x80,0x00,0x80,0xC0,0xC0,0x00,
           0x44,0xEC,0xDC,0xCC,0xCC,0xCC,0xCC,0x00,0x44,0xCC,0xCD,0xCF,0xCC,0xCC,0xCC,0x00,
           0x40,0xC0,0xC0,0x00,0x80,0xC0,0xC0,0x00,0x48,0xFC,0xD4,0xD4,0xD4,0xD4,0xD4,0x00,
           0x78,0xC4,0xC4,0xC4,0xFC,0xC4,0xC4,0x00,0x78,0xC0,0xC0,0xF0,0xC0,0xC0,0xFC,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x7D,0x41,0x05,0x49,0x51,0x01,0x7F,0x00,0x7D,0x41,0x41,0x41,0x41,0x01,0x7F,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x11,0x33,0x66,0x00,0x22,0x66,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x19,0x11,0x11,0x13,0x22,
           0x00,0x00,0x00,0x03,0x02,0x02,0x02,0x1E,0x00,0x02,0x02,0x02,0x00,0x03,0x06,0x0C,
           0x00,0x00,0x00,0x01,0x03,0x36,0x04,0x00,0x00,0x00,0x01,0x01,0x01,0x0F,0x08,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x06,0x0C,0x00,0x04,0x0C,
           0x12,0x12,0x5A,0x36,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x00,0x16,0x14,0x00,
           0x00,0x00,0x1C,0x00,0x10,0x02,0x2C,0x08,0x00,0x00,0x12,0x24,0x08,0x10,0x21,0x02,
           0x00,0x08,0x02,0x04,0x11,0x10,0x01,0x1D,0x00,0x04,0x0C,0x10,0x00,0x00,0x00,0x00,
           0x00,0x18,0x10,0x10,0x10,0x10,0x00,0x18,0x00,0x00,0x01,0x01,0x01,0x01,0x03,0x06,
           0x00,0x00,0x02,0x00,0x02,0x08,0x2A,0x08,0x00,0x00,0x04,0x00,0x27,0x04,0x0C,0x00,
           0x00,0x00,0x00,0x00,0x04,0x04,0x0C,0x18,0x00,0x00,0x00,0x00,0x01,0x1F,0x00,0x00,
           0x00,0x00,0x00,0x00,0x04,0x04,0x1C,0x00,0x00,0x02,0x04,0x08,0x10,0x20,0x40,0x00,
           0x04,0x32,0x29,0x21,0x21,0x13,0x46,0x3C,0x04,0x04,0x24,0x04,0x04,0x04,0x01,0x7F,
           0x02,0x39,0xE1,0x03,0x06,0x1C,0x01,0xFF,0x01,0x73,0x06,0x00,0x39,0x21,0x83,0x7E,
           0x02,0x02,0x12,0x32,0x01,0xF3,0x02,0x0E,0x02,0x3E,0x00,0xF9,0x01,0x01,0x83,0x7E,
           0x02,0x1E,0x30,0x00,0x39,0x21,0x83,0x7E,0x01,0x39,0xE3,0x06,0x0C,0x08,0x08,0x38,
           0x04,0x3A,0x12,0x86,0x61,0x41,0x83,0x7E,0x02,0x39,0x21,0x81,0x79,0x03,0x06,0x7C,
           0x00,0x00,0x00,0x0C,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x04,0x0C,
           0x02,0x06,0x0C,0x18,0x08,0x04,0x02,0x1E,0x00,0x02,0x7E,0x00,0x02,0x7E,0x00,0x00,
           0x08,0x04,0x02,0x01,0x03,0x06,0x0C,0x78,0x02,0x39,0x21,0xE3,0x06,0x1C,0x04,0x1C,
           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x12,0x31,0x21,0x01,0x39,0x21,0xE7,
           0x02,0x39,0x21,0x02,0x39,0x21,0x03,0xFE,0x00,0x19,0x37,0x20,0x20,0x91,0x43,0x3E,
           0x04,0x32,0x29,0x21,0x21,0x23,0x06,0xFC,0x01,0x3F,0x20,0x02,0x3E,0x20,0x01,0xFF,
           0x01,0x3F,0x20,0x02,0x3E,0x20,0x20,0xE0,0x01,0x1F,0x20,0x21,0x29,0x91,0x41,0x3F,
           0x21,0x21,0x21,0x01,0x39,0x21,0x21,0xE7,0x01,0x67,0x04,0x04,0x04,0x04,0x01,0x7F,
           0x01,0x01,0x01,0x01,0x21,0x21,0x83,0x7E,0x21,0x23,0x26,0x0C,0x00,0x20,0x21,0xEF,
           0x10,0x10,0x10,0x10,0x10,0x10,0x01,0x7F,0x21,0x11,0x01,0x01,0x29,0x39,0x21,0xE7,
           0x21,0x11,0x09,0x01,0x21,0x21,0x21,0xE7,0x02,0x39,0x21,0x21,0x21,0x21,0x82,0x7C,
           0x02,0x39,0x21,0x21,0x03,0x3E,0x20,0xE0,0x02,0x39,0x21,0x21,0x21,0x33,0x85,0x7B,
           0x02,0x39,0x21,0x21,0x07,0x20,0x21,0xEF,0x04,0x32,0x2E,0x82,0x79,0x21,0x83,0x7E,
           0x01,0x67,0x04,0x04,0x04,0x04,0x04,0x1C,0x21,0x21,0x21,0x21,0x21,0x21,0x83,0x7E,
           0x21,0x21,0x21,0x11,0x03,0x06,0x0C,0x18,0x21,0x21,0x29,0x01,0x01,0x11,0x31,0xE7,
           0x21,0x11,0x03,0x06,0x00,0x11,0x31,0xE7,0x11,0x11,0x11,0x43,0x26,0x04,0x04,0x1C,
           0x01,0xF1,0x03,0x06,0x0C,0x18,0x01,0xFF,0x00,0x0E,0x08,0x08,0x08,0x08,0x00,0x1E,
           0x00,0x10,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x12,0x02,0x02,0x02,0x02,0x02,0x1E,
           0x00,0x00,0x18,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
           0x00,0x08,0x00,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x01,0x19,0x11,0x03,0x1E,
           0x00,0x00,0x18,0x11,0x11,0x11,0x03,0x3E,0x00,0x00,0x18,0x13,0x10,0x10,0x03,0x1E,
           0x00,0x01,0x19,0x11,0x11,0x11,0x01,0x1F,0x00,0x00,0x18,0x11,0x01,0x1F,0x00,0x1E,
           0x00,0x0E,0x00,0x0C,0x08,0x08,0x08,0x18,0x00,0x00,0x19,0x11,0x11,0x01,0x19,0x03,
           0x00,0x00,0x08,0x19,0x11,0x11,0x11,0x33,0x00,0x0C,0x00,0x04,0x04,0x04,0x04,0x0C,
           0x00,0x06,0x00,0x02,0x02,0x02,0x02,0x06,0x00,0x10,0x10,0x13,0x06,0x10,0x10,0x33,
           0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x0C,0x00,0x00,0x00,0x01,0x01,0x29,0x29,0x63,
           0x00,0x00,0x08,0x19,0x11,0x11,0x11,0x33,0x00,0x00,0x18,0x11,0x11,0x11,0x03,0x1E,
           0x00,0x00,0x18,0x11,0x11,0x03,0x1E,0x10,0x00,0x00,0x19,0x11,0x11,0x01,0x19,0x01,
           0x00,0x00,0x0F,0x18,0x10,0x10,0x10,0x30,0x00,0x00,0x1E,0x00,0x18,0x01,0x01,0x1E,
           0x00,0x00,0x0C,0x08,0x08,0x08,0x00,0x0E,0x00,0x00,0x11,0x11,0x11,0x11,0x01,0x1F,
           0x00,0x00,0x11,0x11,0x11,0x03,0x06,0x0C,0x00,0x00,0x21,0x01,0x01,0x03,0x12,0x26,
           0x00,0x00,0x03,0x06,0x00,0x18,0x11,0x33,0x00,0x00,0x11,0x11,0x11,0x01,0x19,0x03,
           0x00,0x00,0x33,0x06,0x0C,0x18,0x00,0x3F,0x00,0x07,0x00,0x06,0x10,0x06,0x00,0x07,
           0x00,0x04,0x04,0x0C,0x00,0x04,0x04,0x0C,0x00,0x20,0x0C,0x00,0x0E,0x00,0x0C,0x38,
           0x00,0x00,0x00,0x23,0x6E,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x00,0x11,0x22,0x00,
	};
	static struct GfxLayout fontlayout =
	{
		8,8,	/* 8*8 characters */
		128,    /* 40 characters */
		2,	/* 1 bits per pixel */
		{ 0, 128*8*8 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
		8*8	/* every char takes 8 consecutive bytes */
	};
	struct GfxElement *font;
	static unsigned short colortable[4*3];	/* ASG 980209 */
	int trueorientation;


	/* hack: force the display into standard orientation to avoid */
	/* creating a rotated font */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	if ((font = decodegfx(fontdata,&fontlayout)) != 0)
	{
		/* colortable will be set at run time */
		font->colortable = colortable;
		font->total_colors = 3;
	}

	Machine->orientation = trueorientation;

	return font;
}



/***************************************************************************

  Display text on the screen. If erase is 0, it superimposes the text on
  the last frame displayed.

***************************************************************************/

void displaytext(const struct DisplayText *dt,int erase)
{
	int i,j,trueorientation;

	/*             blac blue whit    blac red  yelw    blac blac red  */
	int dt_r[] = { 0x00,0x00,0xff,-1,0x00,0xff,0xff,-1,0x00,0x00,0xff };
	int dt_g[] = { 0x00,0x00,0xff,-1,0x00,0x00,0xff,-1,0x00,0x00,0x00 };
	int dt_b[] = { 0x00,0xff,0xff,-1,0x00,0x00,0x00,-1,0x00,0x00,0x00 };

	/* hack: force the display into standard orientation to avoid */
	/* rotating the user interface */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	/* look for appropriate colors and update the colortable. This is necessary */
	/* for dynamic palette games */

	/* modified to get better results with limited palettes .ac JAN2498 */
	for( i=0; i<=10; i++ ) {
		if( dt_r[i] >= 0 ) {
			j = findbestcolor(dt_r[i],dt_g[i],dt_b[i]);
			Machine->uifont->colortable[i] = j;
		}
	}

	if (erase) osd_clearbitmap(Machine->scrbitmap);

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
			if (*c == '\n')
			{
				x = dt->x;
				y += Machine->uifont->height + 1;
			}
			else if (*c == ' ')
			{
				/* don't try to word wrap at the beginning of a line (this would cause */
				/* an endless loop if a word is longer than a line) */
				if (x != dt->x)
				{
					int nextlen=0;
					const char *nc;


					x += Machine->uifont->width;
					nc = c+1;
					while (*nc && *nc != ' ' && *nc != '\n')
					{
						nextlen += Machine->uifont->width;
						nc++;
					}

					/* word wrap */

					if (x + nextlen > Machine->uiwidth)
					{
						x = dt->x;
						y += Machine->uifont->height + 1;
					}
				}
			}
			else
			{
				drawgfx(Machine->scrbitmap,Machine->uifont,*c,dt->color,0,0,x+Machine->uixmin,y+Machine->uiymin,0,TRANSPARENCY_NONE,0);
				x += Machine->uifont->width;
			}

			c++;
		}

		dt++;
	}

	osd_update_display();

	Machine->orientation = trueorientation;
}




void displayset (const struct DisplayText *dt,int total,int s)
{
	struct DisplayText ds[80];
	int i,ofs;

	if (((3*Machine->uifont->height * (total+1))/2) > (Machine->uiheight-Machine->uifont->height))
	{  /* MENU SCROLL */
		ofs = (Machine->uiheight)/2-dt[2*s].y;
		if (dt[0].y + ofs > 0) ofs = -dt[0].y;
		if (dt[2*total-2].y + ofs < Machine->uiheight-Machine->uifont->height)
			ofs = Machine->uiheight-Machine->uifont->height - dt[2*total-2].y;

		for (i = 0;i < 2*total-1;i++)
		{
			ds[i].color = dt[i].color;
			ds[i].text = dt[i].text;
			ds[i].x = dt[i].x;
			ds[i].y = dt[i].y + ofs;
			if ((ds[i].y<0) || (ds[i].y>(Machine->uiheight-Machine->uifont->height)))
			{
				ds[i].x=0;
				ds[i].y=0;
				ds[i].text="  ";
			}
		}
		ds[total*2-1].text=0;
		displaytext(ds,1);
	}
	else displaytext(dt,1);
}


int showcharset(void)
{
	int i,key,cpx,cpy;
	struct DisplayText dt[2];
	char buf[80];
	int bank,color,line, maxline;
	int trueorientation;


	if ((Machine->drv->gfxdecodeinfo == 0) ||
			(Machine->drv->gfxdecodeinfo[0].memory_region == -1))
		return 0;	/* no gfx sets, return */


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

		sprintf(buf,"GFXSET %d  COLOR %d  LINE %d ",bank,color,line);
		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,0);

		key = osd_read_keyrepeat();

		switch (key)
		{
			case OSD_KEY_RIGHT:
				if (bank+1 < MAX_GFX_ELEMENTS && Machine->gfx[bank + 1])
				{
					bank++;
					line = 0;
				}
				break;

			case OSD_KEY_LEFT:
				if (bank > 0)
				{
					bank--;
					line = 0;
				}
				break;

			case OSD_KEY_PGDN:
				if (line < maxline-1) line++;
				break;

			case OSD_KEY_PGUP:
				if (line > 0) line--;
				break;

			case OSD_KEY_UP:
				if (color < Machine->drv->gfxdecodeinfo[bank].total_color_codes - 1)
					color++;
				break;

			case OSD_KEY_DOWN:
				if (color > 0) color--;
				break;
		}
	} while (key != OSD_KEY_F4 && key != OSD_KEY_ESC);

	while (osd_key_pressed(key));	/* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	Machine->orientation = trueorientation;

	return 0;
}




static int setdipswitches(void)
{
	struct DisplayText dt[80];
	struct InputPort *entry[40];
	int i,s,key,done;
	struct InputPort *in;
	int total;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME && default_name(in) != 0 &&
				(in->type & IPF_UNUSED) == 0 &&
				!(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;
	dt[2 * total + 1].text = 0;	/* terminate array */
	total++;

	s = 0;
	done = 0;
	do
	{
		for (i = 0;i < total;i++)
		{
			dt[2 * i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
			if (i < total - 1)
			{
				dt[2 * i].text = default_name(entry[i]);
				dt[2 * i].x = 0;
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;

				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

				in = entry[i] + 1;
				while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
						in->default_value != entry[i]->default_value)
					in++;

				if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
					dt[2 * i + 1].text = "INVALID";
				else dt[2 * i + 1].text = default_name(in);

				dt[2 * i + 1].x = Machine->uiwidth - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;
			}
		}

		displaytext(dt,1);

		key = osd_read_keyrepeat();

		switch (key)
		{
			case OSD_KEY_DOWN:
				if (s < total - 1) s++;
				else s = 0;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				else s = total - 1;
				break;

			case OSD_KEY_RIGHT:
				if (s < total - 1)
				{
					in = entry[s] + 1;
					while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
							in->default_value != entry[s]->default_value)
						in++;

					if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
						/* invalid setting: revert to a valid one */
						entry[s]->default_value = (entry[s]+1)->default_value & entry[s]->mask;
					else
					{
						if (((in+1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
								!(nocheat && ((in+1)->type & IPF_CHEAT)))
							entry[s]->default_value = (in+1)->default_value & entry[s]->mask;
					}
				}
				break;

			case OSD_KEY_LEFT:
				if (s < total - 1)
				{
					in = entry[s] + 1;
					while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
							in->default_value != entry[s]->default_value)
						in++;

					if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
						/* invalid setting: revert to a valid one */
						entry[s]->default_value = (entry[s]+1)->default_value & entry[s]->mask;
					else
					{
						if (((in-1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
								!(nocheat && ((in-1)->type & IPF_CHEAT)))
							entry[s]->default_value = (in-1)->default_value & entry[s]->mask;
					}
				}
				break;

			case OSD_KEY_ENTER:
				if (s == total - 1) done = 1;
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				done = 1;
				break;
		}
	} while (done == 0);

	while (osd_key_pressed(key));	/* wait for key release */


	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}



static int setkeysettings(void)
{
	struct DisplayText dt[80];
	struct InputPort *entry[40];
	int i,s,key,done;
	struct InputPort *in;
	int total;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_key(in) != IP_KEY_NONE && (in->type & IPF_UNUSED) == 0
			&& (((in->type & 0xff) < IPT_ANALOG_START) || ((in->type & 0xff) > IPT_ANALOG_END))
			&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;
	dt[2 * total + 1].text = 0;	/* terminate array */
	total++;

	s = 0;
	done = 0;
	do
	{
		for (i = 0;i < total;i++)
		{
			dt[2 * i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
			if (i < total - 1)
			{
				dt[2 * i].text = default_name(entry[i]);
				dt[2 * i].x = 0;
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_key_name(default_key(entry[i]));
				dt[2 * i + 1].x = Machine->uiwidth - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;

				in++;
			}
		}

		displayset(dt,total,s);

		key = osd_read_keyrepeat();

		switch (key)
		{
			case OSD_KEY_DOWN:
				if (s < total - 1) s++;
				else s = 0;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				else s = total - 1;
				break;

			case OSD_KEY_ENTER:
				if (s == total - 1) done = 1;
				else
				{
					int newkey;


					dt[2 * s + 1].text = "            ";
					dt[2 * s + 1].x = Machine->uiwidth - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);
					newkey = osd_read_key();
					switch (newkey)
					{
						case OSD_KEY_ESC:
						case OSD_KEY_P:
						case OSD_KEY_F3:
						case OSD_KEY_F4:
						case OSD_KEY_TAB:
						case OSD_KEY_F8:
						case OSD_KEY_F9:
						case OSD_KEY_F10:
						case OSD_KEY_F11:
						case OSD_KEY_F12:
							entry[s]->keyboard = IP_KEY_DEFAULT;
							break;

						default:
							entry[s]->keyboard = newkey;
							break;
					}
				}
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				done = 1;
				break;
		}
	} while (done == 0);

	while (osd_key_pressed(key));	/* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}



#ifdef macintosh
static int setjoysettings(void) { return 0; }
#else
static int setjoysettings(void)
{
	struct DisplayText dt[80];
	struct InputPort *entry[40];
	int i,s,key,done;
	struct InputPort *in;
	int total;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_joy(in) != IP_JOY_NONE && (in->type & IPF_UNUSED) == 0
			&& (((in->type & 0xff) < IPT_ANALOG_START) || ((in->type & 0xff) > IPT_ANALOG_END))
			&& !(nocheat && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;
	dt[2 * total + 1].text = 0;	/* terminate array */
	total++;

	s = 0;
	done = 0;
	do
	{
		for (i = 0;i < total;i++)
		{
			dt[2 * i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
			if (i < total - 1)
			{
				dt[2 * i].text = default_name(entry[i]);
				dt[2 * i].x = 0;
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->uiheight - (3*Machine->uifont->height * (total + 1))/2) / 2;
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_joy_name(default_joy(entry[i]));
				dt[2 * i + 1].x = Machine->uiwidth - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;

				in++;
			}
		}

		displayset(dt,total,s);

		key = osd_read_keyrepeat();
		osd_poll_joystick();

		switch (key)
		{
			case OSD_KEY_DOWN:
				if (s < total - 1) s++;
				else s = 0;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				else s = total - 1;
				break;

			case OSD_KEY_ENTER:
				if (s == total - 1) done = 1;
				else
				{
					int newjoy;
					int joyindex, joypressed;
					dt[2 * s + 1].text = "            ";
					dt[2 * s + 1].x = Machine->uiwidth - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);

					/* Check all possible joystick values for switch or button press */
					joypressed = 0;
					while (!joypressed)
					{
						if (osd_key_pressed(OSD_KEY_ESC))
							joypressed = 1;
						osd_poll_joystick();

						/* Allows for "All buttons" */
						if (osd_key_pressed(OSD_KEY_A))
						{
							entry[s]->joystick = OSD_JOY_FIRE;
							joypressed = 1;
						}
						if (osd_key_pressed(OSD_KEY_B))
						{
							entry[s]->joystick = OSD_JOY2_FIRE;
							joypressed = 1;
						}
						/* Clears entry "None" */
						if (osd_key_pressed(OSD_KEY_N))
						{
							entry[s]->joystick = 0;
							joypressed = 1;
						}

						for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++)
						{
							newjoy = osd_joy_pressed(joyindex);
							if (newjoy)
							{
								entry[s]->joystick = joyindex;
								joypressed = 1;
								break;
							}
						}
					}
				}
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				done = 1;
				break;
		}
	} while (done == 0);

	while (osd_key_pressed(key));	/* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}
#endif


static int settraksettings(void)
{
	struct DisplayText dt[80];
	struct InputPort *entry[40];
	int i,s,pkey,done;
	struct InputPort *in;
	int total,total2;


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

	dt[2 * total2].text = "Return to Main Menu";
	dt[2 * total2].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[2 * total2].text)) / 2;
	dt[2 * total2].y = (3*Machine->uifont->height * (total2+1))/2 + (Machine->uiheight - (3*Machine->uifont->height * (total2 + 1))/2) / 2;
	dt[2 * total2 + 1].text = 0;	/* terminate array */
	total2++;

	s = 0;
	done = 0;
	do
	{
		for (i = 0;i < total2;i++)
		{
			dt[2 * i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
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
						break;
					case 5:
						strcat (label[i], " Reverse ");
						if (reverse)
							sprintf(setting[i],"On");
						else
							sprintf(setting[i],"Off");
						break;
					case 6:
						strcat (label[i], " Sensitivity (%)");
						sprintf(setting[i],"%3d",sensitivity);
						break;
				}
				dt[2 * i].text = label[i];
				dt[2 * i].x = 0;
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->uiheight - (3*Machine->uifont->height * (total2 + 1))/2) / 2;
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = setting[i];
				dt[2 * i + 1].x = Machine->uiwidth - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;

				in++;
			}
		}

		displayset(dt,total2,s);

		pkey = osd_read_keyrepeat();

		switch (pkey)
		{
			case OSD_KEY_DOWN:
				if (s < total2 - 1) s++;
				else s = 0;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				else s = total2 - 1;
				break;

			case OSD_KEY_LEFT:
				if ((s % ENTRIES) == 4)
				/* keyboard/joystick delta */
				{
					int oldval = default_key (entry[s/ENTRIES]);
					int val = (oldval & 0xff0000) >> 16;

					val --;
					if (val < 1) val = entry[s/ENTRIES]->mask - 1;
					oldval &= ~0xff0000;
					oldval |= (val & 0xff) << 16;
					entry[s/ENTRIES]->keyboard = oldval;
				}
				else if ((s % ENTRIES) == 5)
				/* reverse */
				{
					int reverse = entry[s/ENTRIES]->type & IPF_REVERSE;
					if (reverse)
						reverse=0;
					else
						reverse=IPF_REVERSE;
					entry[s/ENTRIES]->type &= ~IPF_REVERSE;
					entry[s/ENTRIES]->type |= reverse;
				}
				else if ((s % ENTRIES) == 6)
				/* sensitivity */
				{
					int oldval = (entry[s/ENTRIES]->arg);
					int val = (oldval & 0xff);

					val --;
					if (val < 1) val = 1;
					oldval &= ~0xff;
					oldval |= (val & 0xff);
					entry[s/ENTRIES]->arg = oldval;
				}
				break;

			case OSD_KEY_RIGHT:
				if ((s % ENTRIES) == 4)
				/* keyboard/joystick delta */
				{
					int oldval = default_key (entry[s/ENTRIES]);
					int val = (oldval & 0xff0000) >> 16;

					val ++;
					if (val > entry[s/ENTRIES]->mask - 1) val = 1;
					oldval &= ~0xff0000;
					oldval |= (val & 0xff) << 16;
					entry[s/ENTRIES]->keyboard = oldval;
				}
				else if ((s % ENTRIES) == 5)
				/* reverse */
				{
					int reverse = entry[s/ENTRIES]->type & IPF_REVERSE;
					if (reverse)
						reverse=0;
					else
						reverse=IPF_REVERSE;
					entry[s/ENTRIES]->type &= ~IPF_REVERSE;
					entry[s/ENTRIES]->type |= reverse;
				}
				else if ((s % ENTRIES) == 6)
				/* sensitivity */
				{
					int oldval = (entry[s/ENTRIES]->arg);
					int val = (oldval & 0xff);

					val ++;
					if (val > 255) val = 255;
					oldval &= ~0xff;
					oldval |= (val & 0xff);
					entry[s/ENTRIES]->arg = oldval;
				}
				break;

			case OSD_KEY_ENTER:
				if (s == total2 - 1) done = 1;
				else
				{
					switch (s % ENTRIES)
					{
						case 0:
						case 1:
						/* We're changing either the dec key or the inc key */
						{
							int newkey;
							int oldkey = default_key (entry[s/ENTRIES]);

							dt[2 * s + 1].text = "            ";
							dt[2 * s + 1].x = Machine->uiwidth - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
							displayset(dt,total2,s);
							newkey = osd_read_key();
							switch (newkey)
							{
								case OSD_KEY_ESC:
									newkey = IP_KEY_DEFAULT;
									break;
								case OSD_KEY_P:
								case OSD_KEY_F3:
								case OSD_KEY_F4:
								case OSD_KEY_TAB:
								case OSD_KEY_F8:
								case OSD_KEY_F9:
								case OSD_KEY_F10:
								case OSD_KEY_F11:
								case OSD_KEY_F12:
									newkey = IP_KEY_NONE;
									break;
							}
							if (s % ENTRIES)
							{
								oldkey &= ~0xff00;
								oldkey |= (newkey << 8);
							}
							else
							{
								oldkey &= ~0xff;
								oldkey |= (newkey);
							}
							entry[s/ENTRIES]->keyboard = oldkey;
							break;
						}
						case 2:
						case 3:
						/* We're changing either the dec joy or the inc joy */
						{
							int newjoy;
							int oldjoy = default_joy (entry[s/ENTRIES]);
							int joyindex, joypressed;
							dt[2 * s + 1].text = "            ";
							dt[2 * s + 1].x = Machine->uiwidth - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
							displayset(dt,total,s);

							/* Check all possible joystick values for switch or button press */
							joypressed = 0;
							newjoy = -1;
							while (!joypressed)
							{
								if (osd_key_pressed(OSD_KEY_ESC))
									joypressed = 1;
								osd_poll_joystick();

								/* Allows for "All buttons" */
								if (osd_key_pressed(OSD_KEY_A))
								{
									newjoy = OSD_JOY_FIRE;
									joypressed = 1;
								}
								if (osd_key_pressed(OSD_KEY_B))
								{
									newjoy = OSD_JOY2_FIRE;
									joypressed = 1;
								}
								/* Clears entry "None" */
								if (osd_key_pressed(OSD_KEY_N))
								{
									newjoy = IP_JOY_NONE;
									joypressed = 1;
								}

								for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++)
								{
									newjoy = osd_joy_pressed(joyindex);
									if (newjoy)
									{
										newjoy = joyindex;
										joypressed = 1;
										break;
									}
								}
							}
							if (newjoy != -1)
							{
								if ((s % ENTRIES) == 3)
								{
									oldjoy &= ~0xff00;
									oldjoy |= (newjoy << 8);
								}
								else
								{
									oldjoy &= ~0xff;
									oldjoy |= (newjoy);
								}
								entry[s/ENTRIES]->joystick = oldjoy;
							}
							break;
						}
					}
				}
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				done = 1;
				break;
		}
	} while (done == 0);

	while (osd_key_pressed(pkey));	/* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}

void mame_stats (void)
{
	int key;
	struct DisplayText dt[2];
	char temp[10];
	char buf[1024];

	strcpy (buf, "MAME ver: ");
	strcat (buf, mameversion);
	strcat (buf, "\n\n");

	strcat (buf, "Tickets dispensed: ");
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
		strcat (buf, "NA\n\n");
	else
	{
		sprintf (temp, "%d\n\n", coins[2]);
		strcat (buf, temp);
	}
	dt[0].text = buf;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = 0;
	dt[0].y = 0;
	dt[1].text = 0;
	displaytext(dt,1);

	key = osd_read_key();
	while (osd_key_pressed(key));	/* wait for key release */
}

int showcredits(void)
{
	int key;
	struct DisplayText dt[2];
	char buf[1024];


	strcpy(buf,"The following people contributed to this driver:\n\n");
	strcat(buf,Machine->gamedrv->credits);
	dt[0].text = buf;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = 0;
	dt[0].y = 0;
	dt[1].text = 0;
	displaytext(dt,1);

	key = osd_read_key();
	while (osd_key_pressed(key));	/* wait for key release */

	return 0;
}

int showgameinfo(void)
{
	int key;
	int i;
	struct DisplayText dt[2];
	char buf[1024];
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
		"T-11"
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
		"YM3812",
		"SN76496",
		"Pokey",
		"Namco",
		"NES",
		"TMS5220",
		"VLM5030",
		"ADPCM samples",
		"OKIM6295 ADPCM",
		"MSM5205 ADPCM"
	};


	sprintf(buf,"%s\n\nCPU:\n",Machine->gamedrv->description);
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
			sprintf(&buf[strlen(buf)],"%d x ",((struct PSGinterface *)Machine->drv->sound[i].sound_interface)->num);

		sprintf(&buf[strlen(buf)],"%s",
				soundnames[Machine->drv->sound[i].sound_type]);

		if (Machine->drv->sound[i].sound_type >= SOUND_AY8910 &&
				Machine->drv->sound[i].sound_type <= SOUND_POKEY)
			sprintf(&buf[strlen(buf)]," %d.%06d MHz",
					((struct PSGinterface *)Machine->drv->sound[i].sound_interface)->clock / 1000000,
					((struct PSGinterface *)Machine->drv->sound[i].sound_interface)->clock % 1000000);

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
		if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
			sprintf(&buf[strlen(buf)],">256 colors (16-bit required)\n");
		else
		{
			sprintf(&buf[strlen(buf)],"%d colors",Machine->drv->total_colors);
			if (Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
				strcat(buf," (dynamic palette)");
			strcat(buf,"\n");
		}
	}

	dt[0].text = buf;
	dt[0].color = DT_COLOR_WHITE;
	dt[0].x = 0;
	dt[0].y = 0;
	dt[1].text = 0;
	displaytext(dt,1);

	key = osd_read_key();
	while (osd_key_pressed(key));	/* wait for key release */

	return 0;
}

enum { UI_SWITCH = 0, UI_KEY, UI_JOY, UI_ANALOG, UI_STATS, UI_CREDITS, UI_GAMEINFO, UI_CHEAT, UI_EXIT };

int setup_menu (void)
{
	struct DisplayText dt[20];
	int ui_menu[9];
	int i,s,key,done;
	int total = 0;


	dt[total].text = "DIP SWITCH SETUP"; ui_menu[total++] = UI_SWITCH;
	dt[total].text = "KEYBOARD SETUP"; ui_menu[total++] = UI_KEY;
	dt[total].text = "JOYSTICK SETUP"; ui_menu[total++] = UI_JOY;

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
			dt[total].text = "ANALOG SETUP"; ui_menu[total++] = UI_ANALOG;
		}
	}

	dt[total].text = "STATS"; ui_menu[total++] = UI_STATS;
	dt[total].text = "CREDITS"; ui_menu[total++] = UI_CREDITS;
	dt[total].text = "GAME INFORMATION"; ui_menu[total++] = UI_GAMEINFO;

	if (nocheat == 0)
	{
		dt[total].text = "CHEAT"; ui_menu[total++] = UI_CHEAT;
	}

	dt[total].text = "RETURN TO GAME"; ui_menu[total++] = UI_EXIT;
	for (i = 0; i < total; i++)
	{
		dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
		dt[i].y = i * 2*Machine->uifont->height + (Machine->uiheight - 2*Machine->uifont->height * (total - 1)) / 2;
		if (i == total-1) dt[i].y += 2*Machine->uifont->height;
	}
	dt[total].text = 0; /* terminate array */

	s = 0;
	done = 0;
	do
	{
		for (i = 0; i < total; i++)
		{
			dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
		}

		displaytext (dt,1);

		key = osd_read_keyrepeat ();

		switch (key)
		{
			case OSD_KEY_DOWN:
				if (s < total - 1) s++;
				else s = 0;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				else s = total - 1;
				break;

			case OSD_KEY_ENTER:
				switch (ui_menu[s])
				{
					case UI_SWITCH:
						if (setdipswitches()) done = 2;
						break;

					case UI_KEY:
						if (setkeysettings()) done = 2;
						break;

					case UI_JOY:
						if (setjoysettings()) done = 2;
						break;

					case UI_ANALOG:
						if (settraksettings()) done = 2;
						break;

					case UI_STATS:
						mame_stats ();
						break;

					case UI_CREDITS:
						if (showcredits()) done = 2;
						break;

					case UI_GAMEINFO:
						if (showgameinfo()) done = 2;
						break;

					case UI_CHEAT:
						cheat_menu();
						break;

					case UI_EXIT:
						done = 1;
						break;
				}
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				done = 1;
				break;
		}
	} while (done == 0);

	while (osd_key_pressed(key));	/* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}
