/*********************************************************************

  drawgfx.h

  Generic graphic functions.

*********************************************************************/

#ifndef DRAWGFX_H
#define DRAWGFX_H

#define MAX_GFX_PLANES 8

struct GfxLayout
{
	unsigned short width,height; /* width and height (in pixels) of chars/sprites */
	unsigned int total; /* total numer of chars/sprites in the rom */
	unsigned short planes; /* number of bitplanes */
	int planeoffset[MAX_GFX_PLANES]; /* start of every bitplane (in bits) */
	int xoffset[64]; /* position of the bit corresponding to the pixel */
	int yoffset[64]; /* of the given coordinates */
	short charincrement; /* distance between two consecutive characters/sprites (in bits) */
};

struct GfxElement
{
	int width,height;

	unsigned int total_elements;	/* total number of characters/sprites */
	int color_granularity;	/* number of colors for each color code */
							/* (for example, 4 for 2 bitplanes gfx) */
	unsigned short *colortable;	/* map color codes to screen pens */
								/* if this is 0, the function does a verbatim copy */
	int total_colors;
	unsigned int *pen_usage;	/* an array of total_elements ints. */
								/* It is a table of the pens each character uses */
								/* (bit 0 = pen 0, and so on). This is used by */
								/* drawgfgx() to do optimizations like skipping */
								/* drawing of a totally transparent characters */
	unsigned char *gfxdata;	/* pixel data */
	int line_modulo;	/* amount to add to get to the next line (usually = width) */
	int char_modulo;	/* = line_modulo * height */
};

struct GfxDecodeInfo
{
	int memory_region;	/* memory region where the data resides (usually 1) */
						/* -1 marks the end of the array */
	int start;	/* beginning of data to decode */
	struct GfxLayout *gfxlayout;
	int color_codes_start;	/* offset in the color lookup table where color codes start */
	int total_color_codes;	/* total number of color codes */
};


struct rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};



#define TRANSPARENCY_NONE 0
#define TRANSPARENCY_PEN 1
#define TRANSPARENCY_PENS 4
#define TRANSPARENCY_COLOR 2
#define TRANSPARENCY_THROUGH 3
#define TRANSPARENCY_PEN_TABLE 5

/* drawing mode case TRANSPARENCY_PEN_TABLE */
extern UINT8 gfx_drawmode_table[256];
#define DRAWMODE_NONE   0
#define DRAWMODE_SOURCE 1
#define DRAWMODE_HALF   2
#define DRAWMODE_DOUBLE 3
#define DRAWMODE_MIX    4

void decodechar(struct GfxElement *gfx,int num,const unsigned char *src,const struct GfxLayout *gl);
struct GfxElement *decodegfx(const unsigned char *src,const struct GfxLayout *gl);
void freegfx(struct GfxElement *gfx);
void drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color);
void copybitmap(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color);
void copybitmapzoom(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scalex,int scaley);
void copyscrollbitmap(struct osd_bitmap *dest,struct osd_bitmap *src,
		int rows,const int *rowscroll,int cols,const int *colscroll,
		const struct rectangle *clip,int transparency,int transparent_color);
void fillbitmap(struct osd_bitmap *dest,int pen,const struct rectangle *clip);
void drawgfxzoom( struct osd_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scalex, int scaley );

#endif
