/***************************************************************************

	Sega X-board hardware

***************************************************************************/

#include "driver.h"
#include "segaic16.h"



#define PRINT_UNUSUAL_MODES		(0)
#define DEBUG_ROAD				(0)



/*************************************
 *
 *	Statics
 *
 *************************************/

static struct tilemap *textmap;

static UINT8 draw_enable;
static UINT8 screen_flip;

static UINT16 latched_xscroll[4], latched_yscroll[4], latched_pageselect[4];

static UINT16 *buffered_spriteram;
static UINT16 *buffered_roadram;
static UINT8 *road_gfx;
static UINT8 road_control;
static UINT8 road_priority;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void latch_tilemap_values(int param);
static void get_tile_info(int tile_index);
static void get_text_info(int tile_index);
static int xboard_road_unpack(void);



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( xboard )
{
	/* allocate memory for the spriteram buffer */
	buffered_spriteram = auto_malloc(0x1000);
	buffered_roadram = auto_malloc(0x1000);
	if (!buffered_spriteram || !buffered_roadram)
		return 1;

	/* create the tilemap for the text layer */
	textmap = tilemap_create(get_text_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,28);
	if (!textmap)
		return 1;

	/* configure it */
	tilemap_set_transparent_pen(textmap, 0);
	tilemap_set_scrolldx(textmap, -24*8, -24*8);
	tilemap_set_scrollx(textmap, 0, 0);
	tilemap_set_palette_offset(textmap, 0x1c00);

	/* create the tilemaps for the bg/fg layers */
	if (!segaic16_init_virtual_tilemaps(16, 0x1c00, get_tile_info))
		return 1;

	/* unpack the road graphics */
	if (xboard_road_unpack())
		return 1;

	/* initialize globals */
	draw_enable = 1;
	screen_flip = 0;

	/* compute palette info */
	segaic16_init_palette(8192);
	return 0;
}


void xboard_reset_video(void)
{
	/* set a timer to latch values on scanline 261 */
	timer_set(cpu_getscanlinetime(261), 0, latch_tilemap_values);
}



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_tile_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-??------------- Unknown
	---b------------ Tile bank select (0-1)
	---ccccccc------ Palette (0-127)
	----nnnnnnnnnnnn Tile index (0-4095)
*/
	UINT16 data = segaic16_tileram[segaic16_tilemap_page * (64*32) + tile_index];
	int bank = (data >> 12) & 1;
	int color = (data >> 6) & 0x7f;
	int code = data & 0x0fff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Textmap callbacks
 *
 *************************************/

static void get_text_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-???------------ Unknown
	----ccc--------- Palette (0-7)
	-------nnnnnnnnn Tile index (0-511)
*/
	UINT16 data = segaic16_textram[tile_index];
	int bank = 0;
	int color = (data >> 9) & 0x07;
	int code = data & 0x1ff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Miscellaneous setters
 *
 *************************************/

void xboard_set_draw_enable(int enable)
{
	enable = (enable != 0);
	if (draw_enable != enable)
	{
		force_partial_update(cpu_getscanline());
		draw_enable = enable;
	}
}


void xboard_set_screen_flip(int flip)
{
	flip = (flip != 0);
	if (screen_flip != flip)
	{
		force_partial_update(cpu_getscanline());
		screen_flip = flip;
	}
}


void xboard_set_road_priority(int priority)
{
	/* this is only set at init time */
	road_priority = priority;
}



/*************************************
 *
 *	Tilemap accessors
 *
 *************************************/

WRITE16_HANDLER( xboard_textram_w )
{
	/* column/rowscroll need immediate updates */
	if (offset >= 0xf00/2)
		force_partial_update(cpu_getscanline());

	COMBINE_DATA(&segaic16_textram[offset]);
	tilemap_mark_tile_dirty(textmap, offset);
}



/*************************************
 *
 *	Misc control registers
 *
 *************************************/

WRITE16_HANDLER( xboard_render_start_w )
{
	int i;

	/* swap the halves of the sprite RAM */
	for (i = 0; i < 0x1000/2; i++)
	{
		UINT16 temp = segaic16_spriteram[i];
		segaic16_spriteram[i] = buffered_spriteram[i];
		buffered_spriteram[i] = temp;
	}

	/* hack for thunderblade */
	segaic16_spriteram[0] = 0xffff;

	/* we will render the sprites when the video update happens */
}


READ16_HANDLER( xboard_road_latch_r )
{
#if 0
	int i;

	/* swap the halves of the road RAM */
	for (i = 0; i < 0x1000/2; i++)
	{
		UINT16 temp = segaic16_roadram[i];
		segaic16_roadram[i] = buffered_roadram[i];
		buffered_roadram[i] = temp;
	}
#endif
	/* we will render the road when the video update happens */
	return 0xffff;
}


WRITE16_HANDLER( xboard_road_control_w )
{
	if (ACCESSING_LSB)
	{
		if (DEBUG_ROAD) printf("road_w = %02X\n", data & 0xff);
		road_control = data & 7;
	}
}



/*************************************
 *
 *	Latch screen-wide tilemap values
 *
 *************************************/

static void latch_tilemap_values(int param)
{
	int i;

	/* latch the scroll and page select values */
	for (i = 0; i < 4; i++)
	{
		latched_pageselect[i] = segaic16_textram[0xe80/2 + i];
		latched_yscroll[i] = segaic16_textram[0xe90/2 + i];
		latched_xscroll[i] = segaic16_textram[0xe98/2 + i];
	}

	/* set a timer to do this again next frame */
	timer_set(cpu_getscanlinetime(261), 0, latch_tilemap_values);
}



/*************************************
 *
 *	Draw a single tilemap layer
 *
 *************************************/

static void xboard_draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int which, int flags, int priority)
{
	UINT16 xscroll, yscroll, pages;
	int x, y;

	/* get global values */
	xscroll = latched_xscroll[which];
	yscroll = latched_yscroll[which];
	pages = latched_pageselect[which];

	/* column scroll? */
	if (yscroll & 0x8000)
	{
		if (PRINT_UNUSUAL_MODES) printf("Column AND row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowcolclip;

			/* adjust to clip this row only */
			rowcolclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowcolclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* loop over column chunks */
			for (x = ((cliprect->min_x + 9) & ~15) - 9; x <= cliprect->max_x; x += 16)
			{
				UINT16 effxscroll, effyscroll, rowscroll;
				UINT16 effpages = pages;

				/* adjust to clip this column only */
				rowcolclip.min_x = (x < cliprect->min_x) ? cliprect->min_x : x;
				rowcolclip.max_x = (x + 15 > cliprect->max_x) ? cliprect->max_x : x + 15;

				/* get the effective scroll values */
				rowscroll = segaic16_textram[0xf80/2 + 0x40/2 * which + y/8];
				effxscroll = (xscroll & 0x8000) ? rowscroll : xscroll;
				effyscroll = segaic16_textram[0xf16/2 + 0x40/2 * which + (x+9)/16];

				/* are we using an alternate? */
				if (rowscroll & 0x8000)
				{
					effxscroll = latched_xscroll[which + 2];
					effyscroll = latched_yscroll[which + 2];
					effpages = latched_pageselect[which + 2];
				}

				/* draw the chunk */
				effxscroll = (0xc0 - effxscroll) & 0x3ff;
				effyscroll = effyscroll & 0x1ff;
				segaic16_draw_virtual_tilemap(bitmap, &rowcolclip, effpages, effxscroll, effyscroll, flags, priority);
			}
		}
	}
	else
	{
		if (PRINT_UNUSUAL_MODES) printf("Row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowclip = *cliprect;
			UINT16 effxscroll, effyscroll, rowscroll;
			UINT16 effpages = pages;

			/* adjust to clip this row only */
			rowclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* get the effective scroll values */
			rowscroll = segaic16_textram[0xf80/2 + 0x40/2 * which + y/8];
			effxscroll = (xscroll & 0x8000) ? rowscroll : xscroll;
			effyscroll = yscroll;

			/* are we using an alternate? */
			if (rowscroll & 0x8000)
			{
				effxscroll = latched_xscroll[which + 2];
				effyscroll = latched_yscroll[which + 2];
				effpages = latched_pageselect[which + 2];
			}

			/* draw the chunk */
			effxscroll = (0xc0 - effxscroll) & 0x3ff;
			effyscroll = effyscroll & 0x1ff;
			segaic16_draw_virtual_tilemap(bitmap, &rowclip, effpages, effxscroll, effyscroll, flags, priority);
		}
	}
}



/*************************************
 *
 *	Draw a single sprite
 *
 *************************************/

/*
	A note about zooming:

	The current implementation is a guess at how the hardware works. Hopefully
	we will eventually get some good tests run on the hardware to understand
	which rows/columns are skipped during a zoom operation.
*/
#define draw_pixel() 														\
	/* only draw if onscreen, not 0 or 15 */								\
	if (x >= cliprect->min_x && x <= cliprect->max_x && pix != 0 && pix != 15) \
	{																		\
		/* are we high enough priority to be visible? */					\
		if (sprpri > pri[x])												\
		{																	\
			/* shadow/hilight mode? */										\
			if (shadow && pix == 0xa)										\
				dest[x] += (paletteram16[dest[x]] & 0x8000) ? 16384 : 8192;	\
																			\
			/* regular draw */												\
			else															\
				dest[x] = pix | color;										\
		}																	\
																			\
		/* always mark priority so no one else draws here */				\
		pri[x] = 0xff;														\
	}																		\

static void draw_one_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT16 *data)
{
	int hide    = (data[0] & 0x5000);
	int bank    = (data[0] >> 9) & 7;
	int top     = (data[0] & 0x1ff) - 0x100;
	UINT16 addr = data[1];
	int pitch   = (INT16)data[2] >> 9;
	int xpos    = (data[2] & 0x1ff) - 0xbe;
	int shadow  = (data[3] >> 14) & 1;
	int sprpri  = 1 << ((data[3] >> 12) & 3);
	int vzoom   = data[3] & 0x3ff;
	int ydelta  = (data[4] & 0x8000) ? 1 : -1;
	int flip    = (~data[4] >> 14) & 1;
	int xdelta  = (data[4] & 0x2000) ? 1 : -1;
	int hzoom   = data[4] & 0x3ff;
	int height  = (data[5] & 0xfff) + 1;
	int color   = (data[6] & 0xff) << 4;
	int x, y, ytarget, yacc = 0, pix, numbanks;
	UINT32 *spritedata;

	/* initialize the end address to the start address */
	data[7] = addr;

	/* if hidden, or top greater than/equal to bottom, or invalid bank, punt */
	if (hide || height == 0)
		return;

	/* clamp to within the memory region size */
	numbanks = memory_region_length(REGION_GFX2) / 0x40000;
	if (numbanks)
		bank %= numbanks;
	spritedata = (UINT32 *)memory_region(REGION_GFX2) + 0x10000 * bank;

	/* clamp to a maximum of 8x (not 100% confirmed) */
	if (vzoom < 0x40) vzoom = 0x40;
	if (hzoom < 0x40) hzoom = 0x40;

	/* loop from top to bottom */
	ytarget = top + ydelta * height;
	for (y = top; y != ytarget; y += ydelta)
	{
		/* skip drawing if not within the cliprect */
		if (y >= cliprect->min_y && y <= cliprect->max_y)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT8 *pri = (UINT8 *)priority_bitmap->line[y];
			int xacc = 0;

			/* non-flipped case */
			if (!flip)
			{
				/* start at the word before because we preincrement below */
				data[7] = addr - 1;
				for (x = xpos; (xdelta > 0 && x <= cliprect->max_x) || (xdelta < 0 && x >= cliprect->min_x); )
				{
					UINT32 pixels = spritedata[++data[7]];

					/* draw four pixels */
					pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;

					/* stop if the second-to-last pixel in the group was 0xf */
					if ((pixels & 0x000000f0) == 0x000000f0)
						break;
				}
			}

			/* flipped case */
			else
			{
				/* start at the word after because we predecrement below */
				data[7] = addr + 1;
				for (x = xpos; (xdelta > 0 && x <= cliprect->max_x) || (xdelta < 0 && x >= cliprect->min_x); )
				{
					UINT32 pixels = spritedata[--data[7]];

					/* draw four pixels */
					pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
					pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;

					/* stop if the second-to-last pixel in the group was 0xf */
					if ((pixels & 0x0f000000) == 0x0f000000)
						break;
				}
			}
		}

		/* accumulate zoom factors; if we carry into the high bit, skip an extra row */
		yacc += vzoom;
		addr += pitch * (yacc >> 9);
		yacc &= 0x1ff;
	}
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 *cursprite;

	/* first scan forward to find the end of the list */
	for (cursprite = buffered_spriteram; cursprite < buffered_spriteram + 0xfff/2; cursprite += 8)
		if (cursprite[0] & 0x8000)
			break;

	/* now scan backwards and render the sprites in order */
	for (cursprite -= 8; cursprite >= buffered_spriteram; cursprite -= 8)
		draw_one_sprite(bitmap, cliprect, cursprite);
}



/*************************************
 *
 *	Road drawing
 *
 *************************************/

static int xboard_road_unpack(void)
{
	int x, y;

	/* allocate memory for the unpacked road data */
	road_gfx = auto_malloc((memory_region_length(REGION_GFX3) / 0x80) * 512);
	if (!road_gfx)
		return 1;

	/* loop over rows */
	for (y = 0; y < memory_region_length(REGION_GFX3) / 0x80; y++)
	{
		UINT8 *src = memory_region(REGION_GFX3) + (y & 0xff) * 0x40 + (y >> 8) * 0x8000;
		UINT8 *dst = road_gfx + y * 512;

		/* loop over columns */
		for (x = 0; x < 512; x++)
		{
			dst[x] = (((src[x/8] >> (~x & 7)) & 1) << 0) | (((src[x/8 + 0x4000] >> (~x & 7)) & 1) << 1);

			/* pre-mark road data in the "stripe" area with a high bit */
			if (x >= 248 && x < 256 && dst[x] == 3)
				dst[x] |= 4;
		}
	}
	return 0;
}


static void xboard_draw_road_low(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int x, y;

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		int data0 = segaic16_roadram[0x000 + y];
		int data1 = segaic16_roadram[0x100 + y];
		int color = -1;

		/* based on the road_control, we can figure out which sky to draw */
		switch (road_control & 3)
		{
			case 0:
				if (data0 & 0x800)
					color = data0 & 0x7f;
				break;

			case 1:
				if (data0 & 0x800)
					color = data0 & 0x7f;
				else if (data1 & 0x800)
					color = data1 & 0x7f;
				break;

			case 2:
				if (data1 & 0x800)
					color = data1 & 0x7f;
				else if (data0 & 0x800)
					color = data0 & 0x7f;
				break;

			case 3:
				if (data1 & 0x800)
					color = data1 & 0x7f;
				break;
		}

		/* fill the scanline with color */
		if (color != -1)
		{
			color += 0x1780;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				dest[x] = color;
		}
	}
}


static void xboard_draw_road_high(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	static const UINT8 priority_map[2][8] =
	{
		{ 0x80,0x81,0x81,0x83,0,0,0,0x00 },
		{ 0x81,0x87,0x87,0x8f,0,0,0,0x00 }
	};
	UINT8 dummy_road[512];
	int x, y;

#if DEBUG_ROAD
	int debug0 = code_pressed(KEYCODE_D);
	int debug1 = code_pressed(KEYCODE_F);
	if (debug0 || debug1)
	{
		palette_set_color(0x1800, (0x1800 & 1) * 0xff, ((0x1800 >> 1) & 1) * 0xff, ((0x1800 >> 2) & 1) * 0xff);
		palette_set_color(0x1801, (0x1801 & 1) * 0xff, ((0x1801 >> 1) & 1) * 0xff, ((0x1801 >> 2) & 1) * 0xff);
		palette_set_color(0x1802, (0x1802 & 1) * 0xff, ((0x1802 >> 1) & 1) * 0xff, ((0x1802 >> 2) & 1) * 0xff);
		palette_set_color(0x1803, (0x1803 & 1) * 0xff, ((0x1803 >> 1) & 1) * 0xff, ((0x1803 >> 2) & 1) * 0xff);
		palette_set_color(0x1807, (0x1807 & 1) * 0xff, ((0x1807 >> 1) & 1) * 0xff, ((0x1807 >> 2) & 1) * 0xff);
	}
#endif

	/* set up a dummy road */
	memset(dummy_road, 3, 512);

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		int data0 = segaic16_roadram[0x000 + y];
		int data1 = segaic16_roadram[0x100 + y];
		int hpos0, hpos1, color0, color1;
		int control = road_control & 3;
		UINT16 color_table[32];
		UINT8 *src0, *src1;
		UINT8 bgcolor;

		/* if both roads are low priority, skip */
		if ((data0 & 0x800) && (data1 & 0x800))
			continue;

		/* get road 0 data */
		src0 = (data0 & 0x800) ? dummy_road : (road_gfx + (0x000 + ((data0 >> 1) & 0xff)) * 512);
		hpos0 = (segaic16_roadram[0x200 + ((road_control & 4) ? y : (data0 & 0x1ff))]) & 0xfff;
		color0 = segaic16_roadram[0x600 + ((road_control & 4) ? y : (data0 & 0x1ff))];

		/* get road 1 data */
		src1 = (data1 & 0x800) ? dummy_road : (road_gfx + (0x100 + ((data1 >> 1) & 0xff)) * 512);
		hpos1 = (segaic16_roadram[0x400 + ((road_control & 4) ? (0x100 + y) : (data1 & 0x1ff))]) & 0xfff;
		color1 = segaic16_roadram[0x600 + ((road_control & 4) ? (0x100 + y) : (data1 & 0x1ff))];

		/* determine the 5 colors for road 0 */
		color_table[0x00] = 0x1700 + ((color0 >> 0) & 1);
		color_table[0x01] = 0x1702 + ((color0 >> 1) & 1);
		color_table[0x02] = 0x1704 + ((color0 >> 2) & 1);
		bgcolor = (color0 >> 8) & 0xf;
		color_table[0x03] = (data0 & 0x200) ? color_table[0x00] : (0x1720 + bgcolor);
		color_table[0x07] = 0x1706 + ((color0 >> 3) & 1);

		/* determine the 5 colors for road 1 */
		color_table[0x10] = 0x1708 + ((color1 >> 4) & 1);
		color_table[0x11] = 0x170a + ((color1 >> 5) & 1);
		color_table[0x12] = 0x170c + ((color1 >> 6) & 1);
		bgcolor = (color1 >> 8) & 0xf;
		color_table[0x13] = (data1 & 0x200) ? color_table[0x10] : (0x1730 + bgcolor);
		color_table[0x17] = 0x170e + ((color1 >> 7) & 1);

#if DEBUG_ROAD
		if (debug0)
		{
			color_table[0x00] = 0x1800;
			color_table[0x01] = 0x1801;
			color_table[0x02] = 0x1802;
			color_table[0x03] = 0x1803;
			color_table[0x07] = 0x1807;
			control = 0;
		}

		if (debug1)
		{
			color_table[0x10] = 0x1800;
			color_table[0x11] = 0x1801;
			color_table[0x12] = 0x1802;
			color_table[0x13] = 0x1803;
			color_table[0x17] = 0x1807;
			control = 3;
		}
#endif

		/* draw the road */
		switch (control)
		{
			case 0:
				if (data0 & 0x800)
					continue;
				for (x = cliprect->min_x; x <= cliprect->max_x; x++, hpos0++)
				{
					int pix0 = (hpos0 >= 0x552 && hpos0 < 0x752) ? src0[hpos0 - 0x552] : 3;
					dest[x] = color_table[0x00 + pix0];
				}
				break;

			case 1:
				for (x = cliprect->min_x; x <= cliprect->max_x; x++, hpos0++, hpos1++)
				{
					int pix0 = (hpos0 >= 0x552 && hpos0 < 0x752) ? src0[hpos0 - 0x552] : 3;
					int pix1 = (hpos1 >= 0x552 && hpos1 < 0x752) ? src1[hpos1 - 0x552] : 3;
					if ((priority_map[0][pix0] >> pix1) & 1)
						dest[x] = color_table[0x10 + pix1];
					else
						dest[x] = color_table[0x00 + pix0];
				}
				break;

			case 2:
				for (x = cliprect->min_x; x <= cliprect->max_x; x++, hpos0++, hpos1++)
				{
					int pix0 = (hpos0 >= 0x552 && hpos0 < 0x752) ? src0[hpos0 - 0x552] : 3;
					int pix1 = (hpos1 >= 0x552 && hpos1 < 0x752) ? src1[hpos1 - 0x552] : 3;
					if ((priority_map[1][pix0] >> pix1) & 1)
						dest[x] = color_table[0x10 + pix1];
					else
						dest[x] = color_table[0x00 + pix0];
				}
				break;

			case 3:
				if (data1 & 0x800)
					continue;
				for (x = cliprect->min_x; x <= cliprect->max_x; x++, hpos1++)
				{
					int pix1 = (hpos1 >= 0x552 && hpos1 < 0x752) ? src1[hpos1 - 0x552] : 3;
					dest[x] = color_table[0x10 + pix1];
				}
				break;
		}
	}
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( xboard )
{
	/* if no drawing is happening, fill with black and get out */
	if (!draw_enable)
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* reset priorities */
	fillbitmap(priority_bitmap, 0, cliprect);

	/* draw the low priority road layer */
	xboard_draw_road_low(bitmap, cliprect);
	if (road_priority == 0)
		xboard_draw_road_high(bitmap, cliprect);

	/* draw background */
	xboard_draw_layer(bitmap, cliprect, 1, 0, 0x01);
	xboard_draw_layer(bitmap, cliprect, 1, 1, 0x02);

	/* draw foreground */
	xboard_draw_layer(bitmap, cliprect, 0, 0, 0x02);
	xboard_draw_layer(bitmap, cliprect, 0, 1, 0x04);

	/* draw the high priority road */
	if (road_priority == 1)
		xboard_draw_road_high(bitmap, cliprect);

	/* text layer */
	tilemap_draw(bitmap, cliprect, textmap, 0, 0x04);
	tilemap_draw(bitmap, cliprect, textmap, 1, 0x08);

	/* draw the sprites */
	draw_sprites(bitmap, cliprect);
}
