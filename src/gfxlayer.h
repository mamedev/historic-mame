#ifndef GFXLAYER_H
#define GFXLAYER_H

struct GfxTileLayout
{
	short total;			/* total numer of tiles in this set */
	short planes;			/* number of bitplanes */
	int planeoffset[8];		/* start of every bitplane */
	int xoffset[8];			/* coordinates of the bit corresponding to the pixel */
	int yoffset[8];			/* of the given coordinates */
	short charincrement;	/* distance between two consecutive tiles */
};


#define MAX_COMPOSED_TILE_SIZE 4

struct GfxTileCompose
{
	char width,height;	/* width and height in 8x8 tiles */
	short multiplier;	/* how much to multiply with the virtual tile code to get */
						/* the 8x8 tile code */
	short xoffset[MAX_COMPOSED_TILE_SIZE];		/* how much to add to the base 8x8 tile code to get the */
	short yoffset[MAX_COMPOSED_TILE_SIZE];		/* codes of all the components. */
	char bankxoffset[MAX_COMPOSED_TILE_SIZE];	/* in some cases the components might */
	char bankyoffset[MAX_COMPOSED_TILE_SIZE];	/* in different GfxTileBanks */
};

struct GfxTile
{
	unsigned char pen[64];		/* pen pixmap for 8x8 tile */
	unsigned char mask[8];		/* bitmask for each horizontal slice */
	unsigned long pens_used;	/* bitmask of pen usage (max 32 pens) */
};

struct GfxTileBank
{
	struct GfxTile *tiles;
	short total_elements;	/* total number of characters/sprites */

	short color_granularity;	/* number of colors for each color code */
							/* (for example, 4 for 2 bitplanes gfx) */
	unsigned short *colortable;	/* map color codes to screen pens */
								/* if this is 0, the function does a verbatim copy */
	short total_colors;
	short transparency_mask;
};

struct GfxTileDecodeInfo
{
	int memory_region;	/* memory region where the data resides (usually 1) */
						/* -1 marks the end of the array */
	int start;	/* beginning of data to decode */
	struct GfxTileLayout *gfxtilelayout;
	short color_codes_start;	/* offset in the color lookup table where color codes start */
	short total_color_codes;	/* total number of color codes */
	long transparency_mask;	/* mask of transparent pens */
};

struct MachineLayer
{
	short type;
	short width,height;
	struct GfxTileDecodeInfo *gfxtiledecodeinfo;
	struct GfxTileCompose *gfxtilecompose;
	void (*updatehook0)(int offset);	/* hooks called when the video RAM is written to */
	void (*updatehook1)(int offset);
	void (*updatehook2)(int offset);
	void (*updatehook3)(int offset);
};

#define LAYER_TILE		1	/* a tile layer, width and height are in tiles */
#define LAYER_BITMAP	2	/* a bitmap layer, width and height are in pixels */



#define MAX_TILE_BANKS 16

struct TileMap
{
	short width,height;		/* width and height in pixels */
	struct GfxTileBank *gfxtilebank[MAX_TILE_BANKS];
	struct GfxTileCompose *gfxtilecompose;
	short scrollx,scrolly;
	long flip;				/* orientation to draw the map in */
	unsigned long globalattr;
	unsigned long *tiles;	/* bit 0-7: color */
							/* bit 8-11: gfx bank */
							/* bit 12: flip x */
							/* bit 13: flip y */
							/* bit 14-15: transparency */
							/* bit 16-31: code */
	short virtualwidth,virtualheight;		/* width and height in virtual tiles */
	unsigned long *virtualtiles;
	unsigned char *virtualdirty;
};


/* values for the tiles array */
#define TILE_FLIP_MASK 0x3000
#define TILE_FLIPX 0x1000
#define TILE_FLIPY 0x2000
#define TILE_TRANSPARENCY_MASK 0xc000
#define TILE_TRANSPARENCY_TRANSPARENT	0x0000	/* totally transparent, don't draw */
#define TILE_TRANSPARENCY_OPAQUE 		0x4000	/* totally opaque, don't care about transparency */
#define TILE_TRANSPARENCY_PEN 			0x8000	/* pen based trasparency, use masks */
#define TILE_TRANSPARENCY_COLOR			0xc000	/* color based transparency, check the palette */

#define TILE_COLOR(tile) ((tile)&0xff)
#define TILE_BANK(tile) (((tile)>>8)&0x0f)
#define TILE_FLIP(tile) ((tile) & TILE_FLIP_MASK)
#define TILE_TRANSPARENCY(tile) ((tile) & TILE_TRANSPARENCY_MASK)
#define TILE_CODE(tile) ((tile)>>16)
#define TILE_ATTR(tile) ((tile)&0xffff)	/* all but code */

#define MAKE_TILE_COLOR(color) ((color)&0xff)
#define MAKE_TILE_BANK(bank) (((bank) & 0x0f) << 8)
#define MAKE_TILE_CODE(code) ((code) << 16)

#define MAKE_TILE(bank,code,color,flipx,flipy,transparency) \
		(MAKE_TILE_COLOR(color) | MAKE_TILE_BANK(bank) | MAKE_TILE_CODE(code) \
		| ((flipx) ? TILE_FLIPX : 0) | ((flipy) ? TILE_FLIPY : 0) \
		| (transparency))
#define MAKE_TILE_CODEATTR(code,attr) (MAKE_TILE_CODE(code) | (attr))


struct GfxLayer
{
	unsigned char *dirty;
	struct TileMap tilemap;
};


/* values for the dirty array */
#define TILE_CLEAN 0
#define TILE_DIRTY 1		/* tile has changed and must be redrawn */
#define TILE_NOT_CLEAN 2	/* tile has been overwritten and will need to be redrawn */
							/* when this portion of the screen is refreshed */



void decodetile(struct GfxTileBank *gfxtilebank,int num,const unsigned char *src,const struct GfxTileLayout *gl,const struct MachineLayer *ml);
struct GfxTileBank *decodetiles(const struct GfxTileDecodeInfo *di,const struct MachineLayer *ml);
void freetiles(struct GfxTileBank *gfxtilebank);

struct GfxLayer *create_tile_layer(struct MachineLayer *ml);
void free_tile_layer(struct GfxLayer *layer);
void layer_mark_block_dirty(struct GfxLayer *layer,int minx,int miny,int action);
void layer_mark_all_dirty(struct GfxLayer *layer,int action);
#define MARK_ALL_DIRTY 1
#define MARK_ALL_NOT_CLEAN 2
#define MARK_DIRTY_IF_NOT_CLEAN 3
#define MARK_CLEAN_IF_DIRTY 4	/* supported only by layer_mark_all_dirty() */
int layer_is_block_dirty(struct GfxLayer *layer,int minx,int miny);
void update_tile_layer(int layer_num,struct osd_bitmap *bitmap);
void set_tile_layer_attributes(int layer_num,struct osd_bitmap *bitmap,int scrollx,int scrolly,int flipx,int flipy,unsigned long global_attr_mask,unsigned long global_attr);
void set_tile_attributes_fast(int layer_num,int tile,int attr);
#define set_tile_attributes(layer,tile,bank,code,color,flipx,flipy,transparency) \
	set_tile_attributes_fast(layer,tile,MAKE_TILE(bank,code,color,flipx,flipy,transparency))
int are_tiles_opaque_norotate(int layer_num,int minx,int miny);

void layer_mark_rectangle_dirty(struct GfxLayer *layer,int minx,int maxx,int miny,int maxy);
void layer_mark_full_screen_dirty(void);

#endif
