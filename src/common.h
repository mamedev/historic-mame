/*********************************************************************

  common.h

  Generic functions used in different emulators.
  There's not much for now, but it could grow in the future... ;-)

*********************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include "osdepend.h"


struct RomModule
{
	const char *name;	/* name of the file to load. If 0, continue reading the previous one. */
	int offset;			/* offset to load it to */
	int size;			/* length of the file. 0 marks the end of the array */
};


struct GfxLayout
{
	int width,height;	/* width and height of chars/sprites */
	int total;	/* total numer of chars/sprites in the rom */
	int planes;	/* number of bitplanes */
	int planeoffset[8];	/* start of every bitplane */
	int xoffset[32];	/* coordinates of the bit corresponding to the pixel */
	int yoffset[32];	/* of the given coordinates */
	int charincrement;	/* distance between two consecutive characters/sprites */
};



struct GfxElement
{
	int width,height;

	struct osd_bitmap *gfxdata;	/* graphic data */
	int total_elements;	/* total number of characters/sprites */

	int color_granularity;	/* number of colors for each color code */
							/* (for example, 4 for 2 bitplanes gfx) */
	const unsigned char *colortable;	/* map color codes to screen pens */
										/* if this is 0, the function does a verbatim copy */
	int total_colors;
};



struct rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};



struct DisplayText
{
	const char *text;	/* 0 marks the end of the array */
	int color;
	int x;
	int y;
};

#define TRANSPARENCY_NONE 0
#define TRANSPARENCY_PEN 1
#define TRANSPARENCY_COLOR 2

int readroms(unsigned char *dest,const struct RomModule *romp,const char *basename);
struct GfxElement *decodegfx(const unsigned char *src,const struct GfxLayout *gl);
void freegfx(struct GfxElement *gfx);
void drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color);
void copybitmap(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color);
void clearbitmap(struct osd_bitmap *bitmap);
void setdipswitches(void);
void displaytext(const struct DisplayText *dt,int erase);
void showcharset(void);


#endif
