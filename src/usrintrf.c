/*********************************************************************

  usrintrf.h

  Functions used to handle MAME's crude user interface.

*********************************************************************/

#include "driver.h"

extern int nocheat;


static int findbestcolor(unsigned char r,unsigned char g,unsigned char b)
{
	int i;
	int best,mindist;


	mindist = 200000;
	best = 0;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		unsigned char r1,g1,b1;
		int d1,d2,d3,dist;


		osd_get_pen(Machine->pens[i],&r1,&g1,&b1);
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
	static unsigned char colortable[4*3];
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
	int trueorientation;


	/* hack: force the display into standard orientation to avoid */
	/* rotating the user interface */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;


	/* look for appropriate colors and update the colortable. This is necessary */
	/* for dynamic palette games */
	Machine->uifont->colortable[0] = findbestcolor(0x00,0x00,0x00);	/* black */
	Machine->uifont->colortable[1] = findbestcolor(0x00,0x00,0xff);	/* blue */
	Machine->uifont->colortable[2] = findbestcolor(0xff,0xff,0xff);	/* white */

	Machine->uifont->colortable[4] = findbestcolor(0x00,0x00,0x00);	/* black */
	Machine->uifont->colortable[5] = findbestcolor(0xff,0x00,0x00);	/* red */
	Machine->uifont->colortable[6] = findbestcolor(0xff,0xff,0x00);	/* yellow */

	Machine->uifont->colortable[8] = findbestcolor(0x00,0x00,0x00);	/* black */
	Machine->uifont->colortable[9] = findbestcolor(0x00,0x00,0x00);	/* black */
	Machine->uifont->colortable[10] = findbestcolor(0xff,0x00,0x00);	/* red */


	if (erase) osd_clearbitmap(Machine->scrbitmap);

	osd_mark_dirty (0,0,Machine->scrbitmap->width-1,Machine->scrbitmap->height-1,1);	/* ASG 971011 */

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
				if (x == dt->x)
					x += Machine->uifont->width;
				else
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

					if (x + nextlen > Machine->scrbitmap->width)
					{
						x = dt->x;
						y += Machine->uifont->height + 1;
					}
				}
			}
			else
			{
				drawgfx(Machine->scrbitmap,Machine->uifont,*c,dt->color,0,0,x,y,0,TRANSPARENCY_NONE,0);
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

	if (((3*Machine->uifont->height * (total+1))/2) > (Machine->scrbitmap->height-Machine->uifont->height))
	{  /* MENU SCROLL */
		ofs = (Machine->scrbitmap->height)/2-dt[2*s].y;
		if (dt[0].y + ofs > 0) ofs = -dt[0].y;
		if (dt[2*total-2].y + ofs < Machine->scrbitmap->height-Machine->uifont->height)
			ofs = Machine->scrbitmap->height-Machine->uifont->height - dt[2*total-2].y;

		for (i = 0;i < 2*total-1;i++)
		{
			ds[i].color = dt[i].color;
			ds[i].text = dt[i].text;
			ds[i].x = dt[i].x;
			ds[i].y = dt[i].y + ofs;
			if ((ds[i].y<0) || (ds[i].y>(Machine->scrbitmap->height-Machine->uifont->height)))
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

		cpx = Machine->scrbitmap->width / Machine->gfx[bank]->width;
		cpy = Machine->scrbitmap->height / Machine->gfx[bank]->height;

		maxline = (Machine->drv->gfxdecodeinfo[bank].gfxlayout->total + cpx - 1) / cpx;

		for (i = 0; i+(line*cpx) < Machine->drv->gfxdecodeinfo[bank].gfxlayout->total ; i++)
		{
			drawgfx(Machine->scrbitmap,Machine->gfx[bank],
				i+(line*cpx),color,  /*sprite num, color*/
				0,0,
				(i % cpx) * Machine->gfx[bank]->width,
				Machine->uifont->height+1 + (i / cpx) * Machine->gfx[bank]->height,
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
				if (Machine->gfx[bank + 1])
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




static int old_setdipswitches(void)
{
	struct DisplayText dt[40];
	int settings[20];
	int i,s,key,done;
	int total;
	const struct DSW *dswsettings;


	dswsettings = Machine->gamedrv->dswsettings;

	total = 0;
	while (dswsettings[total].num != -1)
	{
		int msk,val;


		msk = dswsettings[total].mask;
		if (msk == 0) return 0;	/* error in DSW definition, quit */
		val = Machine->gamedrv->input_ports[dswsettings[total].num].default_value;
		while ((msk & 1) == 0)
		{
			val >>= 1;
			msk >>= 1;
		}
		settings[total] = val & msk;

		total++;
	}

	if (total == 0) return 0;

	for (i = 0;i < total;i++)
	{
		dt[2 * i].text = dswsettings[i].name;
		dt[2 * i].x = 2*Machine->uifont->width;
		dt[2 * i].y = 2*Machine->uifont->height * i + (Machine->scrbitmap->height - 2*Machine->uifont->height * (total + 1)) / 2;
	}

	dt[2 * total].text = "RETURN TO MAIN MENU";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = 2*Machine->uifont->height * (total+1) + (Machine->scrbitmap->height - 2*Machine->uifont->height * (total + 1)) / 2;
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
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = dswsettings[i].values[settings[i]];
				dt[2 * i + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
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
					if (dswsettings[s].reverse == 0)
					{
						if (dswsettings[s].values[settings[s] + 1] != 0) settings[s]++;
					}
					else
					{
						if (settings[s] > 0) settings[s]--;
					}
				}
				break;

			case OSD_KEY_LEFT:
				if (s < total - 1)
				{
					if (dswsettings[s].reverse == 0)
					{
						if (settings[s] > 0) settings[s]--;
					}
					else
					{
						if (dswsettings[s].values[settings[s] + 1] != 0) settings[s]++;
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

	total--;

	while (--total >= 0)
	{
		int msk;


		msk = dswsettings[total].mask;
		while ((msk & 1) == 0)
		{
			settings[total] <<= 1;
			msk >>= 1;
		}

		Machine->gamedrv->input_ports[dswsettings[total].num].default_value =
				(Machine->gamedrv->input_ports[dswsettings[total].num].default_value
				& ~dswsettings[total].mask) | settings[total];
	}

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}

static int old_setkeysettings(void)
{
	struct DisplayText dt[80];
	int i,s,key,done;
	int total;
	const struct KEYSet *keysettings;


	keysettings = Machine->gamedrv->keysettings;

	total = 0;
	while (keysettings[total].num != -1) total++;

	if (total == 0) return 0;

	for (i = 0;i < total;i++)
	{
		dt[2 * i].text = keysettings[i].name;
		dt[2 * i].x = 2*Machine->uifont->width;
		dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
	}

	dt[2 * total].text = "RETURN TO MAIN MENU";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
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
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_key_name( Machine->gamedrv->input_ports[ keysettings[i].num ].keyboard[ keysettings[i].mask ] );
				dt[2 * i + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;
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
					dt[2 * s + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);
					newkey = osd_read_key();
					if (newkey != OSD_KEY_ESC)
						Machine->gamedrv->input_ports[ keysettings[s].num ].keyboard[ keysettings[s].mask ] = newkey;

					switch (Machine->gamedrv->input_ports[ keysettings[s].num ].keyboard[ keysettings[s].mask ])

					case OSD_KEY_P:case OSD_KEY_F3:case  OSD_KEY_F4:case OSD_KEY_TAB:
					case OSD_KEY_F8:case OSD_KEY_F9:case  OSD_KEY_F10:case OSD_KEY_F11:case OSD_KEY_F12:
					    Machine->gamedrv->input_ports[ keysettings[s].num ].keyboard[ keysettings[s].mask ] = 0;
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

static int old_setjoysettings(void)
{
	struct DisplayText dt[80];
	int i,s,key,done;
	int total;
	const struct KEYSet *keysettings;


	keysettings = Machine->gamedrv->keysettings;

	total = 0;
	while (keysettings[total].num != -1) total++;

	if (total == 0) return 0;

	for (i = 0;i < total;i++)
	{
		dt[2 * i].text = keysettings[i].name;
		dt[2 * i].x = 2*Machine->uifont->width;
		dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
	}

	dt[2 * total].text = "RETURN TO MAIN MENU";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
	dt[2 * total + 1].text = " ";
	dt[2 * total + 1].text = 0;     /* terminate array */
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
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_joy_name( Machine->gamedrv->input_ports[ keysettings[i].num ].joystick[ keysettings[i].mask ] );
				dt[2 * i + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
				dt[2 * i + 1].y = dt[2 * i].y;
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
					int newjoy;
                              		int joyindex, joypressed;
                              		extern volatile int joy_left,joy_right,joy_up,joy_down,
                                                  joy_b1,joy_b2,joy_b3,joy_b4;
					dt[2 * s + 1].text = "            ";
					dt[2 * s + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);

					/* Check all possible joystick values for switch or button press */
                              		joypressed = 0;
                              		while (!joypressed) {
                                          if (osd_key_pressed(OSD_KEY_ESC))
                                              joypressed = 1;
					  osd_poll_joystick();

                                        /* Allows for "All buttons" */
                                        if (osd_key_pressed(OSD_KEY_A)) {
                                          Machine->gamedrv->input_ports[
keysettings[s].num ].joystick[ keysettings[s].mask ] = OSD_JOY_FIRE;
                                          joypressed = 1;
                                        }
                                        if (osd_key_pressed(OSD_KEY_B)) {
                                          Machine->gamedrv->input_ports[
keysettings[s].num ].joystick[ keysettings[s].mask ] = OSD_JOY2_FIRE;
                                          joypressed = 1;
                                        }
                                        /* Clears entry "None" */
                                        if (osd_key_pressed(OSD_KEY_N)) {
                                          Machine->gamedrv->input_ports[ keysettings[s].num ].joystick[ keysettings[s].mask ] = 0;
                                          joypressed = 1;
                                        }

				  	  for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++) {
					    newjoy = osd_joy_pressed(joyindex);
					    if (newjoy) {
					      Machine->gamedrv->input_ports[ keysettings[s].num ].joystick[ keysettings[s].mask ] = joyindex;
                                    	      joypressed = 1;
					      joy_left=joy_right=joy_up=joy_down=joy_b1=joy_b2=joy_b3=joy_b4=0;
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

	while (osd_key_pressed(key));   /* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
}



static int setdipswitches(void)
{
	struct DisplayText dt[80];
	struct NewInputPort *entry[40];
	int i,s,key,done;
	struct NewInputPort *in;
	int total;
const char *default_name(const struct NewInputPort *in);


if (Machine->input_ports == 0)
	return old_setdipswitches();


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME && default_name(in) != 0 &&
				(in->type & IPF_UNUSED) == 0)
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
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
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;

				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

				in = entry[i] + 1;
				while ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING &&
						in->default_value != entry[i]->default_value)
					in++;

				if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)
					dt[2 * i + 1].text = "INVALID";
				else dt[2 * i + 1].text = default_name(in);

				dt[2 * i + 1].x = Machine->scrbitmap->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
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
						if (((in+1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING)
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
						if (((in-1)->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING)
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
	struct NewInputPort *entry[40];
	int i,s,key,done;
	struct NewInputPort *in;
	int total;
const char *default_name(const struct NewInputPort *in);
int default_key(const struct NewInputPort *in);


if (Machine->input_ports == 0)
	return old_setkeysettings();


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_key(in) != IP_KEY_NONE &&
				(in->type & IPF_UNUSED) == 0)
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
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
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_key_name(default_key(entry[i]));
				dt[2 * i + 1].x = Machine->scrbitmap->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
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
					dt[2 * s + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);
					newkey = osd_read_key();
					switch (newkey)
					{
						case OSD_KEY_ESC:
							entry[s]->keyboard = IP_KEY_DEFAULT;
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
							entry[s]->keyboard = IP_KEY_NONE;
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



static int setjoysettings(void)
{
	struct DisplayText dt[80];
	struct NewInputPort *entry[40];
	int i,s,key,done;
	struct NewInputPort *in;
	int total;
const char *default_name(const struct NewInputPort *in);
int default_joy(const struct NewInputPort *in);


if (Machine->input_ports == 0)
	return old_setjoysettings();


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (default_name(in) != 0 && default_joy(in) != IP_JOY_NONE &&
				(in->type & IPF_UNUSED) == 0)
		{
			entry[total] = in;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	dt[2 * total].text = "Return to Main Menu";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = (3*Machine->uifont->height * (total+1))/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
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
				dt[2 * i].y = (3*Machine->uifont->height * i)/2 + (Machine->scrbitmap->height - (3*Machine->uifont->height * (total + 1))/2) / 2;
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				dt[2 * i + 1].text = osd_joy_name(default_joy(entry[i]));
				dt[2 * i + 1].x = Machine->scrbitmap->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
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
					int newjoy;
                              		int joyindex, joypressed;
					dt[2 * s + 1].text = "            ";
					dt[2 * s + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * s + 1].text);
					displayset(dt,total,s);

					/* Check all possible joystick values for switch or button press */
                              		joypressed = 0;
                              		while (!joypressed) {
                                          if (osd_key_pressed(OSD_KEY_ESC))
                                              joypressed = 1;
					  osd_poll_joystick();

                                        /* Allows for "All buttons" */
                                        if (osd_key_pressed(OSD_KEY_A)) {
                                          entry[s]->joystick = OSD_JOY_FIRE;
                                          joypressed = 1;
                                        }
                                        if (osd_key_pressed(OSD_KEY_B)) {
                                          entry[s]->joystick = OSD_JOY2_FIRE;
                                          joypressed = 1;
                                        }
                                        /* Clears entry "None" */
                                        if (osd_key_pressed(OSD_KEY_N)) {
                                          entry[s]->joystick = IP_JOY_NONE;
                                          joypressed = 1;
                                        }

				  	  for (joyindex = 1; joyindex < OSD_MAX_JOY; joyindex++) {
					    newjoy = osd_joy_pressed(joyindex);
					    if (newjoy) {
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



static int settraksettings(void)
{
	struct DisplayText dt[40];
	int i,s,key,done;
	int total;
	struct TrakPort *traksettings;
	char number[10][6];

	traksettings = Machine->gamedrv->trak_ports;
	if (traksettings == 0) return 0;

	total = 0;
	while (traksettings[total].axis != -1) total++;

	if (total == 0) return 0;

	for (i = 0;i < total;i++)
	{
	        switch(traksettings[i].axis) {
		case X_AXIS:
		  dt[2*i].text = "X AXIS";
		  break;
		case Y_AXIS:
		  dt[2*i].text = "Y AXIS";
		  break;
		}
		dt[2 * i].x = 2*Machine->uifont->width;
		dt[2 * i].y = 2*Machine->uifont->height * i + (Machine->scrbitmap->height - 2*Machine->uifont->height * (total + 1)) / 2;
	}

	dt[2 * total].text = "RETURN TO MAIN MENU";
	dt[2 * total].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[2 * total].text)) / 2;
	dt[2 * total].y = 2*Machine->uifont->height * (total+1) + (Machine->scrbitmap->height - 2*Machine->uifont->height * (total + 1)) / 2;
	dt[2 * total + 1].text = 0;     /* terminate array */
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
				dt[2 * i + 1].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
				sprintf(number[i],"%3.1f",traksettings[i].scale);
				dt[2 * i + 1].text = number[i];
				dt[2 * i + 1].x = Machine->scrbitmap->width - 2*Machine->uifont->width - Machine->uifont->width*strlen(dt[2 * i + 1].text);
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
				  traksettings[s].scale += 0.1;

				  if (traksettings[s].scale > 10.0 ) {
				    traksettings[s].scale = 10.0;
				  }
				}
				break;

			case OSD_KEY_LEFT:
				if (s < total - 1)
				{
				  traksettings[s].scale -= 0.1;

				  if (traksettings[s].scale < -10.0 ) {
				    traksettings[s].scale = -10.0;
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

	while (osd_key_pressed(key));   /* wait for key release */

	/* clear the screen before returning */
	osd_clearbitmap(Machine->scrbitmap);

	if (done == 2) return 1;
	else return 0;
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



int setup_menu(void)
{
	struct DisplayText dt[10];
	int i,s,key,done;
	int total;


	total = nocheat ? 6 : 7;
	dt[0].text = "DIP SWITCH SETUP";
	dt[1].text = "KEYBOARD SETUP";
	dt[2].text = "JOYSTICK SETUP";
        dt[3].text = "TRACKBALL SETUP";
	dt[4].text = "CREDITS";
	if (nocheat == 0) dt[5].text = "CHEAT";
	dt[total-1].text = "RETURN TO GAME";
	for (i = 0;i < total;i++)
	{
		dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
		dt[i].y = i * 2*Machine->uifont->height + (Machine->scrbitmap->height - 2*Machine->uifont->height * (total - 1)) / 2;
		if (i == total-1) dt[i].y += 2*Machine->uifont->height;
	}
	dt[total].text = 0; /* terminate array */

	s = 0;
	done = 0;
	do
	{
		for (i = 0;i < total;i++)
		{
			dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
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

			case OSD_KEY_ENTER:
				switch (s)
				{
					case 0:
						if (setdipswitches())
							done = 2;
						break;

					case 1:
						if (setkeysettings())
							done = 2;
						break;

					case 2:
						if (setjoysettings())
							done = 2;
						break;

				        case 3:
					        if(settraksettings())
						        done = 2;
						break;

					case 4:
						if (showcredits())
							done = 2;
						break;

					case 5:
						if (nocheat) done = 1;
						else cheat_menu();
						break;

					case 6:
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
