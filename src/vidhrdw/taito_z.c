#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1
#define TC0480SCP_GFX_NUM 1

struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;

static int taito_hide_pixels;
static int sci_spriteframe;
extern data16_t *taitoz_sharedram;


/**********************************************************
				TC0150ROD
**********************************************************/

static data16_t *TC0150ROD_ram;
#define TC0150ROD_RAM_SIZE 0x2000

READ16_HANDLER( TC0150ROD_word_r )
{
	return TC0150ROD_ram[offset];
}

WRITE16_HANDLER( TC0150ROD_word_w )
{
	COMBINE_DATA(&TC0150ROD_ram[offset]);
}

int TC0150ROD_vh_start(void)
{
	TC0150ROD_ram = malloc(TC0150ROD_RAM_SIZE);

	if (!TC0150ROD_ram) return 1;

	state_save_register_UINT16("TC0150ROD", 0, "memory", TC0150ROD_ram, TC0150ROD_RAM_SIZE/2);
	return 0;
}

void TC0150ROD_vh_stop(void)
{
	free(TC0150ROD_ram);
	TC0150ROD_ram = 0;
}

void TC0150ROD_draw(struct osd_bitmap *bitmap,UINT32 priority)
{
	// Empty function, not implemented //

	/*
		Observations
		------------

		Color bytes 0xc0-df in all TaitoZ games look "road" like.
		In each case only four colors appear to be used. 0xe0-ff
		sometimes have road-like colors in too. Suggest 2 bbp
		gfx data for lines.

		Layout of tc0150rod [table from Raine]
		-------------------

		- Road Scroll Chip

		0000-07FF : BANK#0 [A] - 256 Lines
		0800-0FFF : BANK#0 [B] - 256 Lines
		1000-17FF : BANK#1 [A] - 256 Lines
		1800-1FFF : BANK#1 [B] - 256 Lines

		ctrl data at 1FFE:

		0000/0500 = Single
		0900/0C00 = Double

		-----+--------+--------------------------------
		Byte | Bit(s) | Info
		-----+76543210+--------------------------------
		  0  |....x...| Enable Bank#1
		  0  |.....x..| Select A/B Buffer Bank#1
		  0  |......x.| Enable Bank#0 (error chase hq?)
		  0  |.......x| Select A/B Buffer Bank#0
		-----+--------+--------------------------------

		8 bytes per line:

		-----+--------+--------------------------------
		Byte | Bit(s) | Info
		-----+76543210+--------------------------------
		  0  |x.......| Solid from Screen Start to Line
		  0  |.......x| Line Start (high) [See Road Split]
		  1  |xxxxxxxx| Line Start (low)
		  2  |x.......| Solid from Line to Screen End
		  2  |.......x| Line End (high) [See Road Split]
		  3  |xxxxxxxx| Line End (low)
		  4  |x.......| ?
		  4  |....x...| ?
		  4  |.....xxx| X Offset (high)
		  5  |xxxxxxxx| X Offset (low)
		  6  |xxxx....| Colour Bank
		  6  |.....xxx| Gfx Line (high)
		  7  |xxxxxxxx| Gfx Line (low)
		-----+--------+--------------------------------

		0/1 = related to road steepness
		4/5 = related to road straightness
		6/7 = related to road steepness
	*/
}

/**********************************************************/

static int has_TC0480SCP(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the TC0480SCP is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0480SCP_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_TC0150ROD(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers for CPUA/B and see if the TC0150ROD is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[1].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	mwa = Machine->drv->cpu[2].memory_write;	// needed in case Z80 sandwiched between the 68Ks
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0150ROD_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if ((mwa->handler == TC0110PCR_step1_word_w) || (mwa->handler == TC0110PCR_step1_rbswap_word_w))
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int taitoz_core_vh_start (void)
{
	/* Up to $2000/8 big sprites, requires 0x400 * sizeof(*spritelist)
	   Multiply this by 32 to give room for the number of small sprites,
	   which are what actually get put in the structure. */

	spritelist = malloc(0x8000 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	if (has_TC0480SCP())	/* it's Dblaxle, a tc0480scp game */
	{
		if (TC0480SCP_vh_start(TC0480SCP_GFX_NUM,taito_hide_pixels,0x21,0x08,4,0,0,0,0))
			return 1;
	}
	else	/* it's a tc0100scn game */
	{
		if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,taito_hide_pixels))
			return 1;
	}

	if (has_TC0150ROD())
		if (TC0150ROD_vh_start())
			return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	return 0;
}

int taitoz_vh_start (void)
{
	taito_hide_pixels = 0;
	return (taitoz_core_vh_start());
}

int spacegun_vh_start (void)
{
	taito_hide_pixels = 4;
	return (taitoz_core_vh_start());
}

void taitoz_vh_stop (void)
{
	free(spritelist);
	spritelist = 0;

	if (has_TC0480SCP())
	{
		TC0480SCP_vh_stop();
	}
	else
	{
		TC0100SCN_vh_stop();
	}

	if (has_TC0150ROD())
		TC0150ROD_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/


READ16_HANDLER( sci_spriteframe_r )
{
	return (sci_spriteframe << 8);
}

WRITE16_HANDLER( sci_spriteframe_w )
{
	sci_spriteframe = (data >> 8) &0xff;
}


/*********************************************************
				PALETTE
*********************************************************/

static void contcirc_update_palette (void)
{
	int i,j;
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;
	int map_offset,sprite_chunk,code;
	int offs,data,tilenum,color;
	UINT16 palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+1];
		tilenum = data &0x7ff;

		data = spriteram16[offs+3];
		color = (data &0xff00) >> 8;

		if (tilenum)
		{
			map_offset = tilenum << 7;

			for (sprite_chunk=0;sprite_chunk<128;sprite_chunk++)
			{
				i = sprite_chunk % 8;   // 8 sprite chunks across
				j = sprite_chunk / 8;   // 16 sprite chunks down

				code = spritemap[map_offset + i + (j<<3)] &tile_mask;
				palette_map[color] |= Machine->gfx[0]->pen_usage[code];
			}
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}

static void chasehq_update_palette (void)
{
	int i,j;
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	UINT16 tile_mask   = (Machine->gfx[0]->total_elements) - 1;
	UINT16 tile_mask_2 = (Machine->gfx[2]->total_elements) - 1;
	int map_offset,sprite_chunk,code;
	int offs,data,tilenum,color,zoomx;
	UINT16 palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+1];
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// not sure this is right, Raine has 0x7ff

		if (tilenum)
		{
			if (zoomx & 0x40)	// 128x128 sprites, $0-$3ffff in spritemap rom, OBJA
			{
				map_offset = tilenum << 6;

				for (sprite_chunk=0;sprite_chunk<64;sprite_chunk++)
				{
					i = sprite_chunk % 8;   // 8 sprite chunks per row
					j = sprite_chunk / 8;   // 8 rows

					code = spritemap[map_offset + i + (j<<3)] &tile_mask;
					palette_map[color] |= Machine->gfx[0]->pen_usage[code];
				}
			}
			else if (zoomx & 0x20)	// 64x128 sprites, $40000-$5ffff in spritemap rom, OBJB
			{
				map_offset = (tilenum << 5) + 0x20000;

				for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
				{
					i = sprite_chunk % 4;   // 4 sprite chunks per row
					j = sprite_chunk / 4;   // 8 rows

					code = spritemap[map_offset + i + (j<<2)] &tile_mask_2;
					palette_map[color] |= Machine->gfx[2]->pen_usage[code];
				}
			}
			else if ((zoomx & 0x60)==0)	// 32x128 sprites, $60000-$7ffff in spritemap rom, OBJB
			{
				map_offset = (tilenum << 4) + 0x30000;

				for (sprite_chunk=0;sprite_chunk<16;sprite_chunk++)
				{
					i = sprite_chunk % 2;   // 2 sprite chunks per row
					j = sprite_chunk / 2;   // 8 rows

					code = spritemap[map_offset + i + (j<<1)] &tile_mask_2;
					palette_map[color] |= Machine->gfx[2]->pen_usage[code];
				}
			}
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}

static void bshark_update_palette (void)
{
	int i,j;
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;
	int map_offset,sprite_chunk,code;
	int offs,data,tilenum,color;
	UINT16 palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+1];
		color = (data &0x7f80) >> 7;

		data = spriteram16[offs+3];
		tilenum = data &0x1fff;

		if (tilenum)
		{
			map_offset = tilenum << 5;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				i = sprite_chunk % 4;   // 4 sprite chunks across
				j = sprite_chunk / 4;   // 8 sprite chunks down

				code = spritemap[map_offset + i + (j<<2)] &tile_mask;
				palette_map[color] |= Machine->gfx[0]->pen_usage[code];
			}
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}

static void aquajack_update_palette (void)
{
	int i,j;
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;
	int map_offset,sprite_chunk,code;
	int offs,data,tilenum,color;
	UINT16 palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+2];
		color = (data &0xff00) >> 8;

		data = spriteram16[offs+3];
		tilenum = data &0x1fff;

		if (tilenum)
		{
			map_offset = tilenum << 5;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				i = sprite_chunk % 4;   // 4 sprite chunks across
				j = sprite_chunk / 4;   // 8 sprite chunks down

				code = spritemap[map_offset + i + (j<<2)] &tile_mask;
				palette_map[color] |= Machine->gfx[0]->pen_usage[code];
			}
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}

static void spacegun_update_palette (void)
{
	int i,j;
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	UINT16 tile_mask = (Machine->gfx[0]->total_elements) - 1;
	int map_offset,sprite_chunk,code;
	int offs,data,tilenum,color;
	UINT16 palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+2];
		color = (data &0xff00) >> 8;

		data = spriteram16[offs+3];
		tilenum = data &0x1fff;

		if (tilenum)
		{
			map_offset = tilenum << 5;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				i = sprite_chunk % 4;   // 4 sprite chunks across
				j = sprite_chunk / 4;   // 8 sprite chunks down

				code = spritemap[map_offset + i + (j<<2)] &tile_mask;
				palette_map[color] |= Machine->gfx[0]->pen_usage[code];
			}
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}



/************************************************************
			SPRITE DRAW ROUTINES

These draw a series of small tiles ("chunks") together to
create each big sprite. The spritemap rom provides the lookup
table for this. E.g. Spacegun looks up 16x8 sprite chunks
from the spritemap rom, creating this 64x64 sprite (numbers
are the word offset into the spritemap rom):

	 0  1  2  3
	 4  5  6  7
	 8  9 10 11
	12 13 14 15
	16 17 18 19
	20 21 22 23
	24 25 26 27
	28 29 30 31

Chasehq/Nightstr are the only games to build from 16x16
tiles. They are also more complicated to draw, as they have
3 different aggregation formats [32/64/128 x 128]
whereas the other games stick to one, typically 64x64.

All the games make heavy use of sprite zooming.

It is thought that there are just two levels of sprite
priority - under and over the road - but this isn't
certain.

The road games will need to be moved to pdrawgfx() and use
pdrawscanline() for the road.

		***

[The routines for the 16x8 tile games seem to have large
common elements that could be extracted as subroutines.]

NB: unused portions of the spritemap rom contain hex FF's.
It is a useful coding check to warn in the log if these
are being accessed. [They can be inadvertently while
spriteram is being tested, take no notice of that.]


		Aquajack/Spacegun (modified table from Raine; size of bit
		masks verified in Spacegun code)

		Byte | Bit(s) | Description
		-----+76543210+-------------------------------------
		  0  |xxxxxxx.| ZoomY (0 min, 63 max - msb unused as sprites are 64x64)
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Sprite/BG Priority (0=sprites high)
		  2  |.x......| Flip X
		  2  |..?????.| unknown/unused ?
		  2  |.......x| X position (High)
		  3  |xxxxxxxx| X position (Low)
		  4  |xxxxxxxx| Palette bank
		  5  |?.......| unknown/unused ?
		  5  |.xxxxxxx| ZoomX (0 min, 63 max - msb unused as sprites are 64x64)
		  6  |x.......| Flip Y ?
		  6  |.??.....| unknown/unused ?
		  6  |...xxxxx| Sprite Tile high (msb unused - half of spritemap rom empty)
		  7  |xxxxxxxx| Sprite Tile low

		Continental circus (modified Raine table): note similar format.
		The zoom msb is actually used, as sprites are 128x128.

		---+-------------------+--------------
		 0 | xxxxxxx. ........ | ZoomY
		 0 | .......x xxxxxxxx | Y position
						// unsure about Flip Y
		 2 | .....xxx xxxxxxxx | Sprite Tile
		 4 | x....... ........ | Pri ???
		 4 | .x...... ........ | Flip X
		 4 | .......x xxxxxxxx | X position
		 6 | xxxxxxxx ........ | Palette bank
		 6 | ........ .xxxxxxx | ZoomX
		---+-------------------+--------------

		 Bshark/Chasehq/Nightstr/SCI (modified Raine table): similar format.
		 The zoom msb is only used for 128x128 sprites.

		-----+--------+------------------------
		Byte | Bit(s) | Description
		-----+76543210+------------------------
		  0  |xxxxxxx.| ZoomY
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Pri ???
		  2  |.xxxxxxx| Palette bank (High)
		  3  |x.......| Palette bank (Low)
		  3  |.xxxxxxx| ZoomX
		  4  |x.......| Flip Y ???
		  4  |.x......| Flip X
		  4  |.......x| X position (High)
		  5  |xxxxxxxx| X position (Low)
		  6  |...xxxxx| Sprite Tile (High)
		  7  |xxxxxxxx| Sprite Tile (Low)
		-----+--------+------------------------

 [Raine Chasehq sprite plotting is peculiar. It determines
 the type of big sprite by reference to the zoomx and y.
 Therefore it says that the big sprite number is 0-0x7ff
 across ALL three sizes and you can't distinguish them by
 tilenum. FWIW I seem to be ok just using zoomx.

 Also it treats three tile values as being *mask* sprites.
 I don't follow the code, but this may relate to the junk
 sprite which currently appears over the criminal car you
 are chasing.]

********************************************************/

static void contcirc_draw_sprites_16x8(struct osd_bitmap *bitmap,int pri,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		tilenum = data & 0x7ff;	// $80000 spritemap rom maps up to $7ff 128x128 sprites

		data = spriteram16[offs+2];
		priority = (data & 0x8000) >> 15;	// ???
		flipx = (data & 0x4000) >> 14;	// ???
		flipy = (data & 0x2000) >> 13;	// ???
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+3];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		if ((!tilenum) || (priority!=pri)) continue;

		map_offset = tilenum << 7;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<128;sprite_chunk++)
		{
			k = sprite_chunk % 8;   // 8 sprite chunks per row
			j = sprite_chunk / 8;   // 16 rows

			px = k;
			py = j;
			if (flipx)  px = 7-k;	/* pick tiles back to front for x and y flips */
			if (flipy)  py = 15-j;

			code = spritemap[map_offset + px + (py<<3)];

			if (code==0xffff)
			{
				bad_chunks += 1;
				continue;
			}

			curx = x + ((k*zoomx)/8);
			cury = y + ((j*zoomy)/16);

			zx= x + (((k+1)*zoomx)/8) - curx;
			zy= y + (((j+1)*zoomy)/16) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			sprite_ptr->code = code;
			sprite_ptr->color = color;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;
			sprite_ptr->zoomx = zx << 12;
			sprite_ptr->zoomy = zy << 13;

			drawgfxzoom(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					sprite_ptr->zoomx,sprite_ptr->zoomy);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void chasehq_draw_sprites_16x16(struct osd_bitmap *bitmap,int pri,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	struct tempsprite *sprite_ptr = spritelist;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;	// unsure of this
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;	// ???
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// not sure of this, Raine has 0x7ff

		if ((!tilenum) || (priority!=pri)) continue;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		if ((zoomx-1) & 0x40)	// 128x128 sprites, $0-$3ffff in spritemap rom, OBJA
		{
			map_offset = tilenum << 6;

			for (sprite_chunk=0;sprite_chunk<64;sprite_chunk++)
			{
				j = sprite_chunk / 8;   // 8 rows
				k = sprite_chunk % 8;   // 8 sprite chunks per row

				px = k;
				py = j;
				if (flipx)  px = 7-k;	/* pick tiles back to front for x and y flips */
				if (flipy)  py = 7-j;

				code = spritemap[map_offset + px + (py<<3)];

				if (code==0xffff)
				{
					bad_chunks += 1;
					continue;
				}

				code &= 0x3fff;	// correct mask for Chasehq?

				curx = x + ((k*zoomx)/8);
				cury = y + ((j*zoomy)/8);

				zx= x + (((k+1)*zoomx)/8) - curx;
				zy= y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				sprite_ptr->gfx = 0;
				sprite_ptr->code = code;
				sprite_ptr->color = color;
				sprite_ptr->flipx = flipx;
				sprite_ptr->flipy = flipy;
				sprite_ptr->x = curx;
				sprite_ptr->y = cury;
				sprite_ptr->zoomx = zx << 12;
				sprite_ptr->zoomy = zy << 12;

				drawgfxzoom(bitmap,Machine->gfx[sprite_ptr->gfx],
						sprite_ptr->code,
						sprite_ptr->color,
						sprite_ptr->flipx,sprite_ptr->flipy,
						sprite_ptr->x,sprite_ptr->y,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						sprite_ptr->zoomx,sprite_ptr->zoomy);
			}
		}
		else if ((zoomx-1) & 0x20)	// 64x128 sprites, $40000-$5ffff in spritemap rom, OBJB
		{
			map_offset = (tilenum << 5) + 0x20000;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				j = sprite_chunk / 4;   // 8 rows
				k = sprite_chunk % 4;   // 4 sprite chunks per row

				px = k;
				py = j;
				if (flipx)  px = 3-k;	/* pick tiles back to front for x and y flips */
				if (flipy)  py = 7-j;

				code = spritemap[map_offset + px + (py<<2)];

				if (code==0xffff)
				{
					bad_chunks += 1;
					continue;
				}

				code &= 0x3fff;	// correct mask for Chasehq?

				curx = x + ((k*zoomx)/4);
				cury = y + ((j*zoomy)/8);

				zx= x + (((k+1)*zoomx)/4) - curx;
				zy= y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				sprite_ptr->gfx = 2;
				sprite_ptr->code = code;
				sprite_ptr->color = color;
				sprite_ptr->flipx = flipx;
				sprite_ptr->flipy = flipy;
				sprite_ptr->x = curx;
				sprite_ptr->y = cury;
				sprite_ptr->zoomx = zx << 12;
				sprite_ptr->zoomy = zy << 12;

				drawgfxzoom(bitmap,Machine->gfx[sprite_ptr->gfx],
						sprite_ptr->code,
						sprite_ptr->color,
						sprite_ptr->flipx,sprite_ptr->flipy,
						sprite_ptr->x,sprite_ptr->y,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						sprite_ptr->zoomx,sprite_ptr->zoomy);
			}
		}
		else if (((zoomx-1) & 0x60)==0)	// 32x128 sprites, $60000-$7ffff in spritemap rom, OBJB
		{
			map_offset = (tilenum << 4) + 0x30000;

			for (sprite_chunk=0;sprite_chunk<16;sprite_chunk++)
			{
				j = sprite_chunk / 2;   // 8 rows
				k = sprite_chunk % 2;   // 2 sprite chunks per row

				px = k;
				py = j;
				if (flipx)  px = 1-k;	/* pick tiles back to front for x and y flips */
				if (flipy)  py = 7-j;

				code = spritemap[map_offset + px + (py<<1)];

				if (code==0xffff)
				{
					bad_chunks += 1;
					continue;
				}

				code &= 0x3fff;	// correct mask for Chasehq?

				curx = x + ((k*zoomx)/2);
				cury = y + ((j*zoomy)/8);

				zx= x + (((k+1)*zoomx)/2) - curx;
				zy= y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				sprite_ptr->gfx = 2;
				sprite_ptr->code = code;
				sprite_ptr->color = color;
				sprite_ptr->flipx = flipx;
				sprite_ptr->flipy = flipy;
				sprite_ptr->x = curx;
				sprite_ptr->y = cury;
				sprite_ptr->zoomx = zx << 12;
				sprite_ptr->zoomy = zy << 12;

				drawgfxzoom(bitmap,Machine->gfx[sprite_ptr->gfx],
						sprite_ptr->code,
						sprite_ptr->color,
						sprite_ptr->flipx,sprite_ptr->flipy,
						sprite_ptr->x,sprite_ptr->y,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						sprite_ptr->zoomx,sprite_ptr->zoomy);
			}
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void bshark_draw_sprites_16x8(struct osd_bitmap *bitmap,int pri,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	struct tempsprite *sprite_ptr = spritelist;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;	// unsure of this
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;	// ???
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// $80000 spritemap rom maps up to $2000 64x64 sprites

		if ((!tilenum) || (priority!=pri)) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;	// or sprite text spoils fg text bshark end of round 2
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   // 4 sprite chunks per row
			j = sprite_chunk / 4;   // 8 rows

			px = k;
			py = j;
			if (flipx)  px = 3-k;	/* pick tiles back to front for x and y flips */
			if (flipy)  py = 7-j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code==0xffff)
			{
				bad_chunks += 1;
				continue;
			}

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx= x + (((k+1)*zoomx)/4) - curx;
			zy= y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			sprite_ptr->code = code;
			sprite_ptr->color = color;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;
			sprite_ptr->zoomx = zx << 12;
			sprite_ptr->zoomy = zy << 13;

			drawgfxzoom(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					sprite_ptr->zoomx,sprite_ptr->zoomy);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void sci_draw_sprites_16x8(struct osd_bitmap *bitmap,int pri,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, start_offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	struct tempsprite *sprite_ptr = spritelist;

	/* SCI alternates between two areas of its spriteram */

	// This gave back to front frames causing bad flicker... but
	// reversing it now only gives us sprite updates on alternate
	// frames. So we probably have to partly buffer spriteram :(

	start_offs = (sci_spriteframe &1) * 0x800;
	start_offs = 0x800 - start_offs;

	for (offs = start_offs;offs < (start_offs+0x800);offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;	// unsure of this
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;	// ???
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// $80000 spritemap rom maps up to $2000 64x64 sprites

		if ((!tilenum) || (priority!=pri)) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			j = sprite_chunk / 4;   // 8 rows
			k = sprite_chunk % 4;   // 4 sprite chunks per row

			px = k;
			py = j;
			if (flipx)  px = 3-k;	/* pick tiles back to front for x and y flips */
			if (flipy)  py = 7-j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code==0xffff)
			{
				bad_chunks += 1;
				continue;
			}

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx= x + (((k+1)*zoomx)/4) - curx;
			zy= y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			sprite_ptr->code = code;
			sprite_ptr->color = color;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;
			sprite_ptr->zoomx = zx << 12;
			sprite_ptr->zoomy = zy << 13;

			drawgfxzoom(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					sprite_ptr->zoomx,sprite_ptr->zoomy);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void aquajack_draw_sprites_16x8(struct osd_bitmap *bitmap,int pri,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		flipx = (data & 0x4000) >> 14;
		priority = (data & 0x8000) >> 15;
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// $80000 spritemap rom maps up to $2000 64x64 sprites
		flipy = (data & 0x8000) >> 15;	// ???

		if ((!tilenum) || (priority!=pri)) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   // 4 sprite chunks per row
			j = sprite_chunk / 4;   // 8 rows

			px = k;
			py = j;
			if (flipx)  px = 3-k;	/* pick tiles back to front for x and y flips */
			if (flipy)  py = 7-j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code==0xffff)
			{
				bad_chunks += 1;
				continue;
			}

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx= x + (((k+1)*zoomx)/4) - curx;
			zy= y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			sprite_ptr->code = code;
			sprite_ptr->color = color;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;
			sprite_ptr->zoomx = zx << 12;
			sprite_ptr->zoomy = zy << 13;

			drawgfxzoom(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					sprite_ptr->zoomx,sprite_ptr->zoomy);
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void spacegun_draw_sprites_16x8(struct osd_bitmap *bitmap,int *primasks,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		flipx = (data & 0x4000) >> 14;
		priority = (data & 0x8000) >> 15;
		x = data & 0x1ff;   // correct mask?

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	// $80000 spritemap rom maps up to $2000 64x64 sprites
		flipy = (data & 0x8000) >> 15;	// ???

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;	// fix sprite/tile alignment at end of round screen

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   // 4 sprite chunks per row
			j = sprite_chunk / 4;   // 8 rows

			px = k;
			py = j;
			if (flipx)  px = 3-k;	/* pick tiles back to front for x and y flips */
			if (flipy)  py = 7-j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code==0xffff)
			{
				bad_chunks += 1;
				continue;
			}

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx= x + (((k+1)*zoomx)/4) - curx;
			zy= y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			sprite_ptr->code = code;
			sprite_ptr->color = color;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
			sprite_ptr->x = curx;
			sprite_ptr->y = cury;
			sprite_ptr->zoomx = zx << 12;
			sprite_ptr->zoomy = zy << 13;

			if (primasks)
			{
				sprite_ptr->primask = primasks[priority];
				sprite_ptr++;
			}
			else
			{
				drawgfxzoom(bitmap,Machine->gfx[0],
						sprite_ptr->code,
						sprite_ptr->color,
						sprite_ptr->flipx,sprite_ptr->flipy,
						sprite_ptr->x,sprite_ptr->y,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						sprite_ptr->zoomx,sprite_ptr->zoomy);
			}
		}

		if (bad_chunks)
logerror("Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}

	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfxzoom(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->zoomx,sprite_ptr->zoomy,
				sprite_ptr->primask);
	}
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void contcirc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	contcirc_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);

	contcirc_draw_sprites_16x8(bitmap,1,3);
	TC0150ROD_draw(bitmap,0);
	contcirc_draw_sprites_16x8(bitmap,0,3);

	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);	// text layer
}


/* Nightstr and ChaseHQ */

void chasehq_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	chasehq_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);

	chasehq_draw_sprites_16x16(bitmap,1,0);
	TC0150ROD_draw(bitmap,0);
	chasehq_draw_sprites_16x16(bitmap,0,0);

	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);	// text layer
}


void bshark_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	bshark_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);

	bshark_draw_sprites_16x8(bitmap,1,8);
	TC0150ROD_draw(bitmap,0);
	bshark_draw_sprites_16x8(bitmap,0,8);

	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);	// text layer
}


void sci_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	bshark_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);

	sci_draw_sprites_16x8(bitmap,1,6);
	TC0150ROD_draw(bitmap,0);
	sci_draw_sprites_16x8(bitmap,0,6);

	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);	// text layer
}


void aquajack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	aquajack_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,0);

	aquajack_draw_sprites_16x8(bitmap,1,3);
	TC0150ROD_draw(bitmap,0);
	aquajack_draw_sprites_16x8(bitmap,0,3);

	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,0);	// text layer
}


void spacegun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	spacegun_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

	TC0100SCN_tilemap_draw(bitmap,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	/* Sprites can be under/over the layer below text layer */
	{
		int primasks[2] = {0xf0,0xfc};
		spacegun_draw_sprites_16x8(bitmap,primasks,4);
	}

	/* See if we should draw artificial gun targets */

	if (input_port_4_word_r(0,0) &0x1)	/* Fake DSW */
	{
		int rawx, rawy, centrex, centrey, screenx, screeny;

		/* A lag of one frame can be seen with the scope pickup and on the test
		   screen, however there is conflicting evidence so this isn't emulated
		   During high score entry the scope is displayed slightly offset
		   from the crosshair, close inspection shows the crosshair location
		   being used to target letters. */

		/* calculate p1 screen co-ords by matching routine at $195D4 */
		rawx = taitoz_sharedram[0xd94/2];
		centrex = taitoz_sharedram[0x26/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x2e/2] +
				(((rawx * taitoz_sharedram[0x30/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x8/2]) rawx = taitoz_sharedram[0x8/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x36/2] +
				(((rawx * taitoz_sharedram[0x38/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd96/2];
		centrey = taitoz_sharedram[0x28/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x32/2] +
				(((rawy * taitoz_sharedram[0x34/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x10/2]) rawy = taitoz_sharedram[0x10/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x3a/2] +
				(((rawy * taitoz_sharedram[0x3c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317540, $317542 */
		--screenx;
		screeny += 15;

		draw_crosshair(bitmap,screenx,screeny,&Machine->visible_area);

		/* calculate p2 screen co-ords by matching routine at $196EA */
		rawx = taitoz_sharedram[0xd98/2];
		centrex = taitoz_sharedram[0x2a/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x3e/2] +
				(((rawx * taitoz_sharedram[0x40/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x18/2]) rawx = taitoz_sharedram[0x18/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x46/2] +
				(((rawx * taitoz_sharedram[0x48/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd9a/2];
		centrey = taitoz_sharedram[0x2c/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x42/2] +
				(((rawy * taitoz_sharedram[0x44/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x20/2]) rawy = taitoz_sharedram[0x20/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x4a/2] +
				(((rawy * taitoz_sharedram[0x4c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317544, $317546 */
		--screenx;
		screeny += 15;

		draw_crosshair(bitmap,screenx,screeny,&Machine->visible_area);
	}
}


void dblaxle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT8 layer[5];
	UINT16 priority;

	TC0480SCP_tilemap_update();

	palette_init_used_colors();
	bshark_update_palette();

	/* This needs replacing with Bryan's superior method ! */
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	{
		int i;

		/* fix TC0480SCP transparency, but this could compromise the background color */
		for (i = 0;i < Machine->drv->total_colors;i += 16)
			palette_used_colors[i] = PALETTE_COLOR_TRANSPARENT;
	}
	palette_recalc();

	priority = TC0480SCP_get_bg_priority();

	layer[0] = (priority &0xf000) >> 12;	/* tells us which bg layer is bottom */
	layer[1] = (priority &0x0f00) >>  8;
	layer[2] = (priority &0x00f0) >>  4;
	layer[3] = (priority &0x000f) >>  0;	/* tells us which is top */
	layer[4] = 4;   /* text layer always over bg layers */

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,0,NULL);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	TC0480SCP_tilemap_draw(bitmap,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0480SCP_tilemap_draw(bitmap,layer[1],0,0);
	TC0480SCP_tilemap_draw(bitmap,layer[2],0,0);

	bshark_draw_sprites_16x8(bitmap,1,8);
	TC0150ROD_draw(bitmap,0);
	bshark_draw_sprites_16x8(bitmap,0,8);

	/* This layer used for the big numeric displays */
	TC0480SCP_tilemap_draw(bitmap,layer[3],0,0);

	TC0480SCP_tilemap_draw(bitmap,layer[4],0,0);	/* Text layer */

}


