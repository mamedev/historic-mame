/***************************************************************************

						-= Jaleco Mega System 1 =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows scroll 0
		W		shows scroll 1
		E		shows scroll 2
		A		shows sprites with attribute 0-3
		S		shows sprites with attribute 4-7
		D		shows sprites with attribute 8-b
		F		shows sprites with attribute c-f

		Keys can be used togheter!


**********  There are 3 scrolling layers, 2 bytes per tile info:

* Note: MS1-Z has 2 layers only.

  A page is 256x256, approximately the visible screen size. Each layer is
  made up of 8 pages (8x8 tiles) or 32 pages (16x16 tiles). The number of
  horizontal  pages and the tiles size  is selectable, using the  layer's
  control register. I think that when tiles are 16x16 a layer can be made
  of 16x2, 8x4, 4x8 or 2x16 pages (see below). When tile size is 8x8 we
  have two examples to guide the choice:

  the copyright screen of p47j (0x12) should be 4x2 (unless it's been hacked :)
  the ending sequence of 64th street (0x13) should be 2x4.

  I don't see a relation.


MS1-A MS1-B MS1-C
-----------------

					Scrolling layers:

90000 50000 e0000	Scroll 0
94000 54000 e8000	Scroll 1
98000 58000 f0000	Scroll 2					* Note: missing on MS1-Z

Tile format:	fedc------------	Palette
				----ba9876543210	Tile Number



84000 44000 c2208	Layers Enable				* Note: missing on MS1-Z?

	fedc ---- ---- ---- ? (unused?)
	---- ba-- ---- ----	? (used!)
	---- --9- ---- ----	Sprites below fg (sprites over fg on MS1-C?)
	---- ---8 ---- ----	Swap txt with fg (swap bg with fg on MS1-C?)
	---- ---- 7654 ----	? (unused?)
	---- ---- ---- 3---	Enable Sprites
	---- ---- ---- -210	Enable Layer 210



84200 44200 c2000	Scroll 0 Control
84208 44208 c2008	Scroll 1 Control
84008 44008 c2100	Scroll 2 Control		* Note: missing on MS1-Z

Offset:		00					   Scroll X
			02					   Scroll Y
			04 fedc ba98 765- ---- ? (unused?)
			   ---- ---- ---4 ---- 16x16 Tiles
			   ---- ---- ---- 32-- ? (used, by p47!)
			   ---- ---- ---- --10 N: Layer H pages = 16 / (2^N)



84300 44300 c2308	Screen Control

	fed- ---- ---- ---- 	? (unused?)
	---c ---- ---- ---- 	? (on, troughout peekaboo)
	---- ba9- ---- ---- 	? (unused?)
	---- ---8 ---- ---- 	Portrait F/F (?FullFill?)
	---- ---- 765- ---- 	? (unused?)
	---- ---- ---4 ---- 	? (used, see p47j copyright screen!
							   Often set high then low, it seems sound related)
	---- ---- ---- 321- 	? (unused?)
	---- ---- ---- ---0		Flip Screen



**********  There are 256*4 colors (256*3 for MS1-Z):

Colors		MS1-A/C			MS1-Z

000-0ff		Scroll 0		Scroll 0
100-1ff		Scroll 1		Sprites
200-2ff		Scroll 2		Scroll 1
300-3ff		Sprites			-

88000 48000 f8000	Palette

	fedc--------3---	Red
	----ba98-----2--	Blue
	--------7654--1-	Green
	---------------0	? (used, not RGB! [not changed in fades])


**********  There are 256 sprites (128 for MS1-Z):

&RAM[8000]	Sprite Data	(16 bytes/entry. 128? entries)

Offset:		0-6						? (used, but as normal RAM, I think)
			08 	fed- ---- ---- ----	?
				---c ---- ---- ----	mosaic sol.	(?)
				---- ba98 ---- ----	mosaic		(?)
				---- ---- 7--- ----	y flip
				---- ---- -6-- ----	x flip
				---- ---- --45 ----	?
				---- ---- ---- 3210	color code (* bit 3 = priority *)
			0A						H position
			0C						V position
			0E	fedc ---- ---- ----	? (used by p47j, 0-8!)
				---- ba98 7654 3210	Number



Object RAM tells the hw how to use Sprite Data (missing on MS1-Z).
This makes it possible to group multiple small sprite, up to 256,
into one big virtual sprite (up to a whole screen):

8e000 4e000 d2000	Object RAM (8 bytes/entry. 256*4 entries)

Offset:		00	Index into Sprite Data RAM
			02	H	Displacement
			04	V	Displacement
			06	Number	Displacement

Only one of these four 256 entries is used to see if the sprite is to be
displayed, according to this latter's flipx/y state:

Object RAM entries:		Used by sprites with:

000-0ff					No Flip
100-1ff					Flip X
200-2ff					Flip Y
300-3ff					Flip X & Y




No? No? c2108	Sprite Bank

	fedc ba98 7654 321- ? (unused?)
	---- ---- ---- ---0 Sprite Bank



84100 44100 c2200 Sprite Control

			fedc ba9- ---- ---- ? (unused?)
			---- ---8 ---- ---- Enable sprite priority effect ? (see p47j sprite test 1!)
			---- ---- 765- ---- ? (unused?)
			---- ---- ---4 ----	Enable Effect (?)
			---- ---- ---- 3210	Effect Number (?)

I think bit 4 enables some sort of color cycling for sprites having priority
bit set. See code of p7j at 6488,  affecting the rotating sprites before the
Jaleco logo is shown: values 11-1f, then 0. I fear the Effect Number is an
offset to be applied over the pens used by those sprites. As for bit 8, it's
not used during game, but it is turned on when sprite/foreground priority is
tested, along with Effect Number being 1, so ...


**********  Priority (ouch!)

Sprite order is from first in Sprite Data RAM (frontmost) to last.
* Note: the opposite on MS1-Z

Foreground isn't splittable in a "back" part using, say, pens 0-7 and a
"front part" using pens 8-15 usually. Sprites aren't splittable either.
But level A of 64th street has fg that is splittable. I think bit 0 of
the color in paletteram has a role also.


Here's how I draw things (surely incomplete/inaccurate):

	MS1-Z/A/B		MS1-C
bg  Scroll RAM 0	Scroll RAM 1
fg  Scroll RAM 1	Scroll RAM 0
txt Scroll RAM 2	Scroll RAM 2

MS1-Z	-
MS1-A	bit 8 of 84000 is on  -> swap fg, txt
MS1-B	bit 8 of 44000 is on  -> swap fg, txt
MS1-C	bit 8 of c2208 is off -> swap bg, fg


bit x is sprite priority enable:			bit 8 of 84100 44100 c2200
bit 9 is a foreground priority control:		bit 9 of 84000 44000 !c2208

					bg
	if bit x
		if !bit 9	fg
					sprites (priority bit 1)
		if  bit 9	fg
					sprites (priority bit 0)
	else
		if !bit 9	fg
					sprites (priority doesn't affect order)
		if  bit 9	fg

					txt (missing on MS1-Z)

***************************************************************************/

#include "vidhrdw/generic.h"

/* Variables only used here: */
static struct tilemap *scrollram_0_tilemap, *scrollram_1_tilemap, *scrollram_2_tilemap;
static int scrollflag[3], scrollx[3], scrolly[3], pages_per_tmap_x[3], pages_per_tmap_y[3];
static int active_layers, spritebank, screenflag, spriteflag;
/* For debug purposes: */
int debugsprites;


/* Variables that driver has access to: */
unsigned char *megasys1_scrollram_0, *megasys1_scrollram_1, *megasys1_scrollram_2;
unsigned char *megasys1_objectram, *megasys1_vregs, *megasys1_ram;


/* Variables defined in driver: */
extern int hardware_type;

extern struct GameDriver avspirit_driver;


/***************************************************************************
							Palette routines
***************************************************************************/


void paletteram_RRRRGGGGBBBBRGBx_word_w(int offset, int data)
{
	/*	byte 0    byte 1	*/
	/*	RRRR GGGG BBBB RGB?	*/
	/*	4321 4321 4321 000?	*/

	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	int r = ((newword >> 8) & 0xF0 ) | ((newword << 0) & 0x08);
	int g = ((newword >> 4) & 0xF0 ) | ((newword << 1) & 0x08);
	int b = ((newword >> 0) & 0xF0 ) | ((newword << 2) & 0x08);

	palette_change_color( offset/2, r,g,b );

	WRITE_WORD (&paletteram[offset], newword);
}


/***************************************************************************
					  Callbacks for the TileMap code
***************************************************************************/

#define TILES_PER_PAGE_X (0x20)
#define TILES_PER_PAGE_Y (0x20)
#define TILES_PER_PAGE (TILES_PER_PAGE_X * TILES_PER_PAGE_Y)

#define MEGASYS1_SCROLLRAM_GET_TILE_INFO(_n_) \
static void get_scrollram_##_n_##_tile_info( int col, int row ) \
{ \
	if (scrollflag[_n_] & 0x10)	/* tiles are 8x8 */ \
	{ \
		int tile_index = \
			(col * TILES_PER_PAGE_Y) + \
\
			(row / TILES_PER_PAGE_Y) * TILES_PER_PAGE * pages_per_tmap_x[_n_] + \
			(row % TILES_PER_PAGE_Y); \
\
		int code = READ_WORD(&megasys1_scrollram_##_n_[tile_index * 2]); \
		SET_TILE_INFO( _n_ , code & 0xfff, code >> 12); \
/*					tile_info.priority = code>>12; */ \
	} \
	else /* tiles are 16x16 */ \
	{ \
		int tile_index = \
			((col / 2) * (TILES_PER_PAGE_Y / 2)) + \
\
			((row / 2) / (TILES_PER_PAGE_Y / 2)) * (TILES_PER_PAGE / 4) * pages_per_tmap_x[_n_] + \
			((row / 2) % (TILES_PER_PAGE_Y / 2)); \
\
		int code = READ_WORD(&megasys1_scrollram_##_n_[tile_index * 2]); \
		SET_TILE_INFO( _n_ , (code & 0xfff) * 4 + (row & 1) + (col & 1) * 2 , code >> 12); \
/*					tile_info.priority = code>>12; */ \
	} \
}


MEGASYS1_SCROLLRAM_GET_TILE_INFO(0)
MEGASYS1_SCROLLRAM_GET_TILE_INFO(1)
MEGASYS1_SCROLLRAM_GET_TILE_INFO(2)

#define MEGASYS1_SCROLLRAM_R(_n_) \
int megasys1_scrollram_##_n_##_r(int offset) {return READ_WORD(&megasys1_scrollram_##_n_[offset]);}


#define MEGASYS1_SCROLLRAM_W(_n_) \
void megasys1_scrollram_##_n_##_w(int offset,int data) \
{ \
int old_data, new_data; \
\
	old_data = READ_WORD(&megasys1_scrollram_##_n_[offset]); \
	new_data = COMBINE_WORD(old_data,data); \
	if (old_data != new_data) \
	{ \
		WRITE_WORD(&megasys1_scrollram_##_n_[offset], new_data); \
		if (scrollram_##_n_##_tilemap) \
		{ \
			int page, tile_index, row, col; \
			if (scrollflag[_n_] & 0x10)	/* tiles are 8x8 */ \
			{ \
				page		=	(offset/2) / TILES_PER_PAGE; \
				tile_index	=	(offset/2) % TILES_PER_PAGE; \
 \
				col	=	tile_index / TILES_PER_PAGE_Y + \
						( page % pages_per_tmap_x[_n_] ) * TILES_PER_PAGE_X; \
 \
				row	=	tile_index % TILES_PER_PAGE_Y + \
						( page / pages_per_tmap_x[_n_] ) * TILES_PER_PAGE_Y; \
 \
				tilemap_mark_tile_dirty(scrollram_##_n_##_tilemap, col, row); \
			} \
			else \
			{ \
				page		=	(offset/2) / (TILES_PER_PAGE / 4); \
				tile_index	=	(offset/2) % (TILES_PER_PAGE / 4); \
\
				/* col and row when tiles are 16x16 .. */ \
				col	=	tile_index / (TILES_PER_PAGE_Y / 2) + \
						( page % pages_per_tmap_x[_n_] ) * (TILES_PER_PAGE_X / 2); \
 \
				row	=	tile_index % (TILES_PER_PAGE_Y / 2) + \
						( page / pages_per_tmap_x[_n_] ) * (TILES_PER_PAGE_Y / 2); \
 \
				/* .. but we draw four 8x8 tiles, so col and row must be scaled */ \
				col *= 2;	row *= 2; \
				tilemap_mark_tile_dirty(scrollram_##_n_##_tilemap, col + 0, row + 0); \
				tilemap_mark_tile_dirty(scrollram_##_n_##_tilemap, col + 1, row + 0); \
				tilemap_mark_tile_dirty(scrollram_##_n_##_tilemap, col + 0, row + 1); \
				tilemap_mark_tile_dirty(scrollram_##_n_##_tilemap, col + 1, row + 1); \
			} \
		}\
	}\
}

MEGASYS1_SCROLLRAM_R(0)
MEGASYS1_SCROLLRAM_R(1)
MEGASYS1_SCROLLRAM_R(2)

MEGASYS1_SCROLLRAM_W(0)
MEGASYS1_SCROLLRAM_W(1)
MEGASYS1_SCROLLRAM_W(2)



int megasys1_vh_start(void)
{
	int i;

	spriteram = &megasys1_ram[0x8000];
	scrollram_0_tilemap = scrollram_1_tilemap = scrollram_2_tilemap = 0;

	active_layers = spritebank = screenflag = spriteflag = 0;

	for (i = 0; i < 3; i ++)
	{
	  scrollflag[i] = scrollx[i] = scrolly[i] = 0;
	  pages_per_tmap_x[i] = pages_per_tmap_y[i] = 0;
	}

	return 0;
}



/***************************************************************************
						Video registers access
***************************************************************************/


#ifdef MAME_DEBUG
#define SHOW_READ_ERROR(_format_,_offset_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_);\
	usrintf_showmessage(buf);\
	/*	if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf);*/ \
}

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{\
	char buf[80];\
	sprintf(buf,_format_,_offset_,_data_);\
	usrintf_showmessage(buf);\
	/*	if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf);*/ \
}

#else

#define SHOW_READ_ERROR(_format_,_offset_,_data_)\
{\
	char buf[80];\
	if (errorlog)\
	{ \
		sprintf(buf,_format_,_offset_);\
		if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf);\
	} \
}

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{\
	char buf[80];\
	if (errorlog)\
	{ \
		sprintf(buf,_format_,_offset_,_data_); \
		fprintf(errorlog, "CPU #0 PC %06X : Warning, %s\n",cpu_get_pc(), buf); \
	} \
}

#endif





#define VREG_SCROLL(_n_, _dir_) scroll##_dir_[_n_] = new_data; break;

#define VREG_FLAG(_n_) \
	if ((scrollflag[_n_] != new_data) || (scrollram_##_n_##_tilemap == 0) ) \
	{ \
		scrollflag[_n_] = new_data; \
\
		if (scrollram_##_n_##_tilemap) \
			tilemap_dispose(scrollram_##_n_##_tilemap); \
\
		/* number of pages when tiles are 16x16 */ \
		pages_per_tmap_x[_n_] = 16 / ( 1 << (scrollflag[_n_] & 0x3) ); \
		pages_per_tmap_y[_n_] = 32 / pages_per_tmap_x[_n_]; \
\
		/* when tiles are 8x8, divide the number of total pages by 4 */ \
		if (scrollflag[_n_] & 0x10) \
		{ \
			if (pages_per_tmap_y[_n_] > 4)	{ pages_per_tmap_x[_n_] /= 1;	pages_per_tmap_y[_n_] /= 4;} \
			else							{ pages_per_tmap_x[_n_] /= 2;	pages_per_tmap_y[_n_] /= 2;} \
		} \
\
		scrollram_##_n_##_tilemap = \
			tilemap_create \
				(	get_scrollram_##_n_##_tile_info, \
					TILEMAP_TRANSPARENT, \
					8,8, \
					TILES_PER_PAGE_X * pages_per_tmap_x[_n_], \
					TILES_PER_PAGE_Y * pages_per_tmap_y[_n_]  \
				); \
		scrollram_##_n_##_tilemap->transparent_pen = 15; \
\
		if (scrollram_##_n_##_tilemap == 0) SHOW_WRITE_ERROR("vreg %04X <- %04X NO MEMORY FOR SCREEN",offset,data); \
	} \
	break;




/* See Cybattler: sound latch read on irq 2 service routine */
#define SOUNDCOMMAND_W \
	soundlatch_w(0,new_data); \
	cpu_cause_interrupt(1,2); \
	break;





void megasys1_vregs_A_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&megasys1_vregs[offset]);
	COMBINE_WORD_MEM(&megasys1_vregs[offset],data);
	new_data  = READ_WORD(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x000   : {active_layers = new_data;} break;
		case 0x008+0 : VREG_SCROLL(2,x)
		case 0x008+2 : VREG_SCROLL(2,y)
		case 0x008+4 : VREG_FLAG(2)
		case 0x200+0 : VREG_SCROLL(0,x)
		case 0x200+2 : VREG_SCROLL(0,y)
		case 0x200+4 : VREG_FLAG(0)
		case 0x208+0 : VREG_SCROLL(1,x)
		case 0x208+2 : VREG_SCROLL(1,y)
		case 0x208+4 : VREG_FLAG(1)
		case 0x100   : {spriteflag = new_data;} break;
		case 0x300   : {screenflag = new_data;} break;
		case 0x308   : SOUNDCOMMAND_W

		default: SHOW_WRITE_ERROR("vreg %04X <- %04X",offset,data);
	}

}




int megasys1_vregs_C_r(int offset)
{
	switch (offset)
	{
		case 0x8000:	{return soundlatch2_r(0);}	break;
		default:		{return READ_WORD(&megasys1_vregs[offset]);}
	}
}
void megasys1_vregs_C_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&megasys1_vregs[offset]);
	COMBINE_WORD_MEM(&megasys1_vregs[offset],data);
	new_data  = READ_WORD(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x2208   : {active_layers = new_data;} break;
		case 0x2100+0 : VREG_SCROLL(2,x)
		case 0x2100+2 : VREG_SCROLL(2,y)
		case 0x2100+4 : VREG_FLAG(2)
		case 0x2000+0 : VREG_SCROLL(0,x)
		case 0x2000+2 : VREG_SCROLL(0,y)
		case 0x2000+4 : VREG_FLAG(0)
		case 0x2008+0 : VREG_SCROLL(1,x)
		case 0x2008+2 : VREG_SCROLL(1,y)
		case 0x2008+4 : VREG_FLAG(1)
		case 0x2108   : {spritebank = new_data;} break;
		case 0x2200   : {spriteflag = new_data;} break;
		case 0x2308   : {screenflag = new_data;} break;
		case 0x8000   : SOUNDCOMMAND_W

		default: SHOW_WRITE_ERROR("vreg %04X <- %04X",offset,data);
	}
}



/***************************************************************************
						Sprites Drawing
***************************************************************************/


/*	 Draw sprites in the given bitmap. Priority may be:

 0	Draw sprites whose priority bit is 0
 1	Draw sprites whose priority bit is 1
-1	Draw sprites regardless of their priority

 Sprite Data:

	Offset		Data

 	00-07						?
	08 		fed- ---- ---- ----	?
			---c ---- ---- ----	mosaic sol.	(?)
			---- ba98 ---- ----	mosaic		(?)
			---- ---- 7--- ----	y flip
			---- ---- -6-- ----	x flip
			---- ---- --45 ----	?
			---- ---- ---- 3210	color code (bit 3 = priority)
	0A		X position
	0C		Y position
	0E		Code											*/

static void draw_sprites(struct osd_bitmap *bitmap, int priority)
{
int color,code,sx,sy,attr,sprite,offs;

/* objram: 0x100*4 entries		spritedata: 0x80? entries */

	/* move the bit to the relevant position (and invert it) */
	if (priority != -1)	priority = (priority ^ 1) << 3;

	/* sprite order is from first in Sprite Data RAM (frontmost) to last */

	if (hardware_type != 'Z')
	{
		for (offs = 0x000; offs < 0x800 ; offs += 8)
		{
			for (sprite = 0; sprite < 4 ; sprite ++)
			{
				unsigned char *objectdata = &megasys1_objectram[offs + 0x800 * sprite];
				unsigned char *spritedata = &spriteram[(READ_WORD(&objectdata[0x00])&0x7f)*0x10];

				attr = READ_WORD(&spritedata[0x08]);
				if ( (attr & 0x08) == priority ) continue;

				if (((attr & 0xc0)>>6) != sprite)	continue;
#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) != (debugsprites-1)) ) continue;
#endif
				/* apply the position displacements */
				sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&objectdata[0x02]) ) % 512;
				sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&objectdata[0x04]) ) % 512;

				if (sx > 256-1) sx -= 512;
				if (sy > 256-1) sy -= 512;

				/* sprite code is displaced as well */
				code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&objectdata[0x06]);
				color = (attr & 0x0F);

				drawgfx(bitmap,Machine->gfx[3],
						(code & 0xfff ) + (spritebank << 12),
						color,
						attr & 0x40,attr & 0x80,
						sx, sy,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,15);

			}	/* sprite */
		}	/* offs */
	}	/* non Z hw */
	else
	{

		/* MS1-Z just draws Sprite Data, and in reverse order */

		for (sprite = 0; sprite < 0x80 ; sprite ++)
		{
			unsigned char *spritedata = &spriteram[sprite*0x10];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0x08) == priority ) continue;
#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) == (debugsprites-1)) ) continue;
#endif

			sx = READ_WORD(&spritedata[0x0A]) % 512;
			sy = READ_WORD(&spritedata[0x0C]) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			code  = READ_WORD(&spritedata[0x0E]);
			color = (attr & 0x0F);

			drawgfx(bitmap,Machine->gfx[2],
				code,
				color,
				attr & 0x40,attr & 0x80,
				sx, sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,15);
		}	/* sprite */
	}	/* Z hw */

}







/* Mark colors used by visible sprites */

static void mark_sprite_colors(void)
{
int color_codes_start, color, colmask[16];
int offs, sx, sy, code, attr, i;

	int xmin = Machine->drv->visible_area.min_x - (16 - 1);
	int xmax = Machine->drv->visible_area.max_x;
	int ymin = Machine->drv->visible_area.min_y - (16 - 1);
	int ymax = Machine->drv->visible_area.max_y;

	for (color = 0 ; color < 16 ; color++) colmask[color] = 0;

	if (hardware_type == 'Z')		/* different sprites hw */
	{
		int sprite;
		unsigned int *pen_usage	=	Machine->gfx[2]->pen_usage;
		int total_elements		=	Machine->gfx[2]->total_elements;
		color_codes_start		=	Machine->drv->gfxdecodeinfo[2].color_codes_start;

		for (sprite = 0; sprite < 0x80 ; sprite++)
		{
			unsigned char* spritedata = &spriteram[sprite*16];

			sx		=	READ_WORD(&spritedata[0x0A]) % 512;
			sy		=	READ_WORD(&spritedata[0x0C]) % 512;
			code	=	READ_WORD(&spritedata[0x0E]);
			color	=	READ_WORD(&spritedata[0x08]) & 0x0F;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > xmax) ||(sy > ymax) ||(sx < xmin) ||(sy < ymin)) continue;

			colmask[color] |= pen_usage[code % total_elements];
		}
	}
	else
	{
		unsigned int *pen_usage	=	Machine->gfx[3]->pen_usage;
		int total_elements		=	Machine->gfx[3]->total_elements;
		color_codes_start		=	Machine->drv->gfxdecodeinfo[3].color_codes_start;

		for (offs = 0; offs < 0x2000 ; offs += 8)
		{
			int sprite = READ_WORD(&megasys1_objectram[offs+0x00]);
			unsigned char *spritedata = &spriteram[(sprite&0x7F)*16];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0xc0) != ((offs/0x800)<<6) ) continue;

			sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&megasys1_objectram[offs+0x02]) ) % 512;
			sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&megasys1_objectram[offs+0x04]) ) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > xmax) ||(sy > ymax) ||(sx < xmin) ||(sy < ymin)) continue;

			code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&megasys1_objectram[offs+0x06]);
			code  =	(code & 0xfff ) + (spritebank << 12);
			color = (attr & 0xf);

			colmask[color] |= pen_usage[code % total_elements];
		}
	}


	for (color = 0; color < 16; color++)
	 for (i = 0; i < 16; i++)
	  if (colmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
}




/***************************************************************************
			  Draw the game screen in the given osd_bitmap.
***************************************************************************/

struct priority
{
	struct GameDriver *driver;
	int priorities[16];
};

extern struct GameDriver p47_driver;
extern struct GameDriver street64_driver;
extern struct GameDriver edf_driver;
extern struct GameDriver avspirit_driver;
extern struct GameDriver hachoo_driver;
extern struct GameDriver cybattlr_driver;

static struct priority priorities[] =
{
	{	&p47_driver,	/* same as edf, it seems */
		{ 0x0132f,0x0231f,0x0312f,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&street64_driver,
		{ 0xfffff,0x0312f,0xfffff,0x0132f,0xfffff,0x04152,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&edf_driver,
		{ 0x0132f,0x0231f,0x0312f,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&avspirit_driver,
		{ 0x1032f,0x0132f,0x1302f,0x0312f,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&hachoo_driver,
		{ 0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x24105,0xfffff,
		  0x24105,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&cybattlr_driver,
		{ 0x0132f,0xfffff,0xfffff,0xfffff,0x1032f,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x0132f }
	},
	{	0,	/* default */
		{ 0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	}
};

void megasys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int active_layers1 = active_layers;
//	int layers_n = 3;
	int i,flag,pri;


#ifdef MAME_DEBUG
debugsprites = 0;
if (keyboard_pressed(KEYCODE_Z))
{
int msk = 0;

	if (keyboard_pressed(KEYCODE_Q)) { msk |= 0xfff1;}
	if (keyboard_pressed(KEYCODE_W)) { msk |= 0xfff2;}
	if (keyboard_pressed(KEYCODE_E)) { msk |= 0xfff4;}
	if (keyboard_pressed(KEYCODE_A))	{ msk |= 0xfff8; debugsprites = 1;}
	if (keyboard_pressed(KEYCODE_S))	{ msk |= 0xfff8; debugsprites = 2;}
	if (keyboard_pressed(KEYCODE_D))	{ msk |= 0xfff8; debugsprites = 3;}
	if (keyboard_pressed(KEYCODE_F))	{ msk |= 0xfff8; debugsprites = 4;}

	if (msk != 0) active_layers &= msk;
}
#endif

#define MEGASYS1_TMAP_SET_SCROLL(_n_) \
	if (scrollram_##_n_##_tilemap) \
	{ \
		tilemap_set_scrollx(scrollram_##_n_##_tilemap, 0, scrollx[_n_]); \
		tilemap_set_scrolly(scrollram_##_n_##_tilemap, 0, scrolly[_n_]); \
	}

	MEGASYS1_TMAP_SET_SCROLL(0)
	MEGASYS1_TMAP_SET_SCROLL(1)
	MEGASYS1_TMAP_SET_SCROLL(2)



#define MEGASYS1_TMAP_UPDATE(_n_) \
	if ( (scrollram_##_n_##_tilemap) && (active_layers & (1 << _n_) ) ) \
		tilemap_update(scrollram_##_n_##_tilemap);

	MEGASYS1_TMAP_UPDATE(0)
	MEGASYS1_TMAP_UPDATE(1)
	MEGASYS1_TMAP_UPDATE(2)

	palette_init_used_colors();

	mark_sprite_colors();

	if (palette_recalc())	tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);


#define MEGASYS1_TMAP_RENDER(_n_) \
	if ( (scrollram_##_n_##_tilemap) && (active_layers & (1 << _n_) ) )\
		tilemap_render(scrollram_##_n_##_tilemap);

	MEGASYS1_TMAP_RENDER(0)
	MEGASYS1_TMAP_RENDER(1)
	MEGASYS1_TMAP_RENDER(2)

#define MEGASYS1_TMAP_DRAW(_n_) \
	if ( (scrollram_##_n_##_tilemap) && (active_layers & (1 << _n_) ) ) \
	{ \
		tilemap_draw(bitmap, scrollram_##_n_##_tilemap, flag); \
		flag = 0; \
	}


	i = 0;
	while (priorities[i].driver && priorities[i].driver != Machine->gamedrv &&
			priorities[i].driver != Machine->gamedrv->clone_of)
		i++;
	pri = priorities[i].priorities[(active_layers & 0x0f00) >> 8];
#ifdef MAME_DEBUG
	if (pri == 0xfffff || keyboard_pressed(KEYCODE_Z))
	{
		char baf[40];
		sprintf(baf,"pri = %x",(active_layers & 0x0f00) >> 8);
		usrintf_showmessage(baf);
	}
#endif
	if (pri == 0xfffff) pri = 0x0132f;

	flag = TILEMAP_IGNORE_TRANSPARENCY;

	for (i = 0;i < 5;i++)
	{
		int layer = (pri & 0xf0000) >> 16;
		pri <<= 4;

		switch (layer)
		{
			case 0: MEGASYS1_TMAP_DRAW(0); break;
			case 1: MEGASYS1_TMAP_DRAW(1); break;
			case 2: MEGASYS1_TMAP_DRAW(2); break;
			case 3:
			case 4:
			case 5:
				if (active_layers & 0x08)
				{
					if (flag == TILEMAP_IGNORE_TRANSPARENCY)
					{
						flag = 0;
						osd_clearbitmap(bitmap);	/* should use fillbitmap */
					}
					draw_sprites(bitmap,-1+(layer-4));
				}
				break;
		}
	}

#if 0
{
	int i;
	for (i = 1 ; i < 16 ; i++)
	{
		if ( (scrollram_0_tilemap) && (active_layers & (1 << 0) ) )
			tilemap_draw(bitmap, scrollram_0_tilemap, i);
		if ( (scrollram_1_tilemap) && (active_layers & (1 << 1) ) )
			tilemap_draw(bitmap, scrollram_1_tilemap, i);
	}
}
#endif

	active_layers = active_layers1;
}
