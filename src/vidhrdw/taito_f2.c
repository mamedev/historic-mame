#include "driver.h"
#include "vidhrdw/generic.h"

#define USE_TILEMAP

size_t f2_backgroundram_size;
size_t f2_foregroundram_size;
#ifdef USE_TILEMAP
static struct tilemap *background_tilemap,*foreground_tilemap,*pivot_tilemap;
#else
unsigned char *text_dirty;
unsigned char *bg_dirty;
unsigned char *fg_dirty;
static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;
static struct osd_bitmap *tmpbitmap4;
#endif

unsigned char *taitof2_scrollx;
unsigned char *taitof2_scrolly;
unsigned char *taitof2_rotate;
unsigned char *f2_backgroundram;
unsigned char *f2_foregroundram;
unsigned char *f2_textram;
size_t f2_textram_size;
unsigned char *f2_pivotram;
size_t f2_pivotram_size;
unsigned char *taitof2_characterram;
unsigned char *char_dirty;	/* 256 chars */
size_t f2_characterram_size;
size_t f2_paletteram_size;

/* Determines what capabilities we handle in the video driver */
/* 0 = sprites, 1 = sprites + layers, 2 = sprites + layers + rotation */
int f2_video_version = 1;

static int spritebank[8];


#ifdef USE_TILEMAP
static void get_backtile_info(int tile_index)
{
	int tile,color;

	color=READ_WORD(&f2_backgroundram[4*tile_index]);
	tile=READ_WORD(&f2_backgroundram[(4*tile_index)+2]);

	SET_TILE_INFO(1,tile,color&0xff)
	tile_info.flags=TILE_FLIPYX( color >>14 );
}

static void get_foretile_info(int tile_index)
{
	int tile,color;

	color=READ_WORD(&f2_foregroundram[4*tile_index]);
	tile=READ_WORD(&f2_foregroundram[(4*tile_index)+2]);

	SET_TILE_INFO(1,tile,color&0xff)
	tile_info.flags=TILE_FLIPYX( color >>14 );
}

static void get_pivottile_info(int tile_index)
{
	int tile,color=0;

if (f2_video_version >= 2) {
	tile=READ_WORD(&f2_pivotram[2*tile_index]);

	SET_TILE_INFO(3,tile,color&0xff)
} else SET_TILE_INFO(0,0,0)
}
#endif

int taitof2_core_vh_start (void)
{
	if (f2_video_version >= 1)
	{
		char_dirty = malloc (f2_characterram_size/16);
		if (!char_dirty) return 1;
		memset (char_dirty,1,f2_characterram_size/16);

#ifndef USE_TILEMAP
		text_dirty = malloc (f2_textram_size/2);
		if (!text_dirty)
		{
			free (char_dirty);
			return 1;
		}

		bg_dirty = malloc (f2_backgroundram_size/4);
		if (!bg_dirty)
		{
			free (char_dirty);
			free (text_dirty);
			return 1;
		}
		memset (bg_dirty,1,f2_backgroundram_size/4);

		fg_dirty = malloc (f2_foregroundram_size/4);
		if (!fg_dirty)
		{
			free (char_dirty);
			free (text_dirty);
			free (bg_dirty);
			return 1;
		}
		memset (fg_dirty,1,f2_foregroundram_size/4);

		/* create a temporary bitmap slightly larger than the screen for the background */
		if ((tmpbitmap = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (bg_dirty);
			free (fg_dirty);
			return 1;
		}

		/* create a temporary bitmap slightly larger than the screen for the foreground */
		if ((tmpbitmap2 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (bg_dirty);
			free (fg_dirty);
			bitmap_free (tmpbitmap);
			return 1;
		}

		/* create a temporary bitmap slightly larger than the screen for the text layer */
		if ((tmpbitmap3 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (bg_dirty);
			free (fg_dirty);
			bitmap_free (tmpbitmap);
			bitmap_free (tmpbitmap2);
			return 1;
		}
#endif
	}

#ifdef USE_TILEMAP
	background_tilemap = tilemap_create(get_backtile_info, tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,64);
	foreground_tilemap = tilemap_create(get_foretile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	pivot_tilemap      = tilemap_create(get_pivottile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!background_tilemap || !foreground_tilemap || !pivot_tilemap)
		return 1;

	foreground_tilemap->transparent_pen = 0;
	pivot_tilemap->transparent_pen = 0;
#else
	if (f2_video_version >= 2)
	{
		/* create a temporary bitmap for the pivot layer */
		if ((tmpbitmap4 = bitmap_alloc(64*8, 64*8)) == 0)
		{
			free (char_dirty);
			free (text_dirty);
			free (bg_dirty);
			free (fg_dirty);
			bitmap_free (tmpbitmap);
			bitmap_free (tmpbitmap2);
			bitmap_free (tmpbitmap3);
			return 1;
		}
	}

	{
		int i;

		memset (paletteram, 0, f2_paletteram_size); /* probably not needed */
		for (i = 0; i < f2_paletteram_size/2; i++)
			palette_change_color (i,0,0,0);
	}

#endif

	{
		int i;

		for (i = 0; i < 8; i ++)
			spritebank[i] = 0x400 * i;
	}

	return 0;
}

int taitof2_0_vh_start (void)
{
	f2_video_version = 0;
	return (taitof2_core_vh_start());
}

int taitof2_vh_start (void)
{
	f2_video_version = 1;
	return (taitof2_core_vh_start());
}

int taitof2_2_vh_start (void)
{
	f2_video_version = 2;
	return (taitof2_core_vh_start());
}

void taitof2_vh_stop (void)
{
	if (f2_video_version >= 1)
	{
		free (char_dirty);
#ifndef USE_TILEMAP
		free (text_dirty);
		free (bg_dirty);
		free (fg_dirty);
		bitmap_free (tmpbitmap);
		bitmap_free (tmpbitmap2);
		bitmap_free (tmpbitmap3);
#endif
	}
#ifndef USE_TILEMAP
	if (f2_video_version >= 2)
	{
		bitmap_free (tmpbitmap4);
	}
#endif
}

READ_HANDLER( taitof2_spriteram_r )
{
	return READ_WORD(&spriteram[offset]);
}

WRITE_HANDLER( taitof2_spriteram_w )
{
	COMBINE_WORD_MEM(&spriteram[offset],data);
}

/* we have to straighten out the 16-bit word into bytes for gfxdecode() to work */
READ_HANDLER( taitof2_characterram_r )
{
	int res;

	res = READ_WORD (&taitof2_characterram[offset]);

	#ifdef LSB_FIRST
	res = ((res & 0x00ff) << 8) | ((res & 0xff00) >> 8);
	#endif

	return res;
}

WRITE_HANDLER( taitof2_characterram_w )
{
	int oldword = READ_WORD (&taitof2_characterram[offset]);
	int newword;


	#ifdef LSB_FIRST
	data = ((data & 0x00ff00ff) << 8) | ((data & 0xff00ff00) >> 8);
	#endif

	newword = COMBINE_WORD (oldword,data);
	if (oldword != newword)
	{
		WRITE_WORD (&taitof2_characterram[offset],newword);
		char_dirty[offset / 16] = 1;
	}
}

READ_HANDLER( taitof2_text_r )
{
	return READ_WORD(&f2_textram[offset]);
}

WRITE_HANDLER( taitof2_text_w )
{
#ifdef USE_TILEMAP
	COMBINE_WORD_MEM(&f2_textram[offset],data);
#else
	int oldword = READ_WORD (&f2_textram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_textram[offset],newword);
		text_dirty[offset / 2] = 1;
	}
#endif
}

READ_HANDLER( taitof2_pivot_r )
{
	return READ_WORD(&f2_pivotram[offset]);
}

WRITE_HANDLER( taitof2_pivot_w )
{
#ifdef USE_TILEMAP
	COMBINE_WORD_MEM(&f2_pivotram[offset],data);
	tilemap_mark_tile_dirty(pivot_tilemap,offset/2);
#else
	int oldword = READ_WORD (&f2_pivotram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_pivotram[offset],newword);
//		pivot_dirty[offset / 2] = 1;
	}
#endif
}

READ_HANDLER( taitof2_background_r )
{
	return READ_WORD(&f2_backgroundram[offset]);
}

WRITE_HANDLER( taitof2_background_w )
{
#ifdef USE_TILEMAP
	COMBINE_WORD_MEM(&f2_backgroundram[offset],data);
	tilemap_mark_tile_dirty(background_tilemap,offset/4);
#else
	int oldword = READ_WORD (&f2_backgroundram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_backgroundram[offset],newword);
		bg_dirty[offset / 4] = 1;
	}
#endif
}

READ_HANDLER( taitof2_foreground_r )
{
	return READ_WORD(&f2_foregroundram[offset]);
}

WRITE_HANDLER( taitof2_foreground_w )
{
#ifdef USE_TILEMAP
	COMBINE_WORD_MEM(&f2_foregroundram[offset],data);
	tilemap_mark_tile_dirty(foreground_tilemap,offset/4);
#else
	int oldword = READ_WORD (&f2_foregroundram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&f2_foregroundram[offset],newword);
		fg_dirty[offset / 4] = 1;
	}
#endif
}

WRITE_HANDLER( taitof2_spritebank_w )
{
	logerror("bank %d, new value: %04x\n", offset >> 1, data << 10);
	if ((offset >> 1) < 4) return;
//	if (data == 0) data = (offset >> 1) * 0x400;
	spritebank[offset >> 1] = data << 10;
//	spritebank[offset >> 1] = data & 0x0f;
}

#define CONVERT_SPRITE_CODE												\
{																		\
	int bank;															\
																		\
	bank = (code & 0x1800) >> 11;										\
	switch (bank)														\
	{																	\
		case 0: code = spritebank[2] * 0x800 + (code & 0x7ff); break;	\
		case 1: code = spritebank[3] * 0x800 + (code & 0x7ff); break;	\
		case 2: code = spritebank[4] * 0x400 + (code & 0x7ff); break;	\
		case 3: code = spritebank[6] * 0x800 + (code & 0x7ff); break;	\
	}																	\
}																		\

void taitof2_update_palette (void)
{
	int i;
	int offs,code,color,spritecont;
	unsigned short palette_map[256];

	memset (palette_map, 0, sizeof (palette_map));

#ifndef USE_TILEMAP
	if (f2_video_version >= 1)
	{
		/* Background layer */
		for (offs = 0;offs < f2_backgroundram_size;offs += 4)
		{
			int tile;

			tile = READ_WORD (&f2_backgroundram[offs + 2]);
			color = (READ_WORD (&f2_backgroundram[offs]) & 0xff);

			palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
		}

		/* Foreground layer */
		for (offs = 0;offs < f2_foregroundram_size;offs += 4)
		{
			int tile;

			tile = READ_WORD (&f2_foregroundram[offs + 2]);
			color = (READ_WORD (&f2_foregroundram[offs]) & 0xff);

			palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
		}
	}
#endif
	color = 0;

	/* Sprites */
	for (offs = 0;offs < 0x3400;offs += 16)
	{
		spritecont = (READ_WORD(&spriteram[offs+8]) & 0xff00) >> 8;
//		if (!spritecont) continue;

		code = READ_WORD(&spriteram[offs]) & 0x1fff;

#if 1
		{
			int bank;

			bank = (code & 0x1c00) >> 10;
			code = spritebank[bank] + (code & 0x3ff);
		}
#else
		CONVERT_SPRITE_CODE;
#endif
		if ((spritecont & 0xf4) == 0)
		{
			color = READ_WORD(&spriteram[offs+8]) & 0x00ff;
		}
		if (!code) continue;

		palette_map[color] |= Machine->gfx[0]->pen_usage[code];
	}

#ifdef USE_TILEMAP
	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int j;
		for (j = 1;j < 16;j++)
			if (palette_map[i] & (1 << j))
				palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
	}

	if (f2_video_version >= 1)

	{
		memset (palette_map, 0, sizeof (palette_map));
		for (offs = 0;offs < 0x1000;offs += 2)
		{
			int tile;

			tile = READ_WORD (&f2_textram[offs]) & 0xff;
			if (!tile) continue;

			color = ((READ_WORD (&f2_textram[offs]) & 0x0f00) >> 8);
			palette_map[color] |= Machine->gfx[2]->pen_usage[tile];
		}
		for (i = 0;i < 256;i++)
		{
			int j;
			for (j = 1;j < 16;j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
#else
	if (f2_video_version >= 1)
	{
		/* Do the text layer */
		for (offs = 0;offs < 0x1000;offs += 2)
		{
			int tile;

			tile = READ_WORD (&f2_textram[offs]) & 0xff;
			color = (READ_WORD (&f2_textram[offs]) & 0x0f00) >> 8;

			palette_map[color] |= Machine->gfx[2]->pen_usage[tile];
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
				else
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
		}
		else
			memset(&palette_used_colors[i * 16],PALETTE_COLOR_UNUSED,16);
	}
#endif

	if (palette_recalc ())
	{
#ifdef USE_TILEMAP
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
#else
		memset (text_dirty, 1, f2_textram_size/2);
		memset (bg_dirty, 1, f2_backgroundram_size/4);
		memset (fg_dirty, 1, f2_foregroundram_size/4);
#endif
	}
}

void taitof2_draw_sprites (struct osd_bitmap *bitmap)
{
	/*
		Sprite format:
		0000: 000xxxxxxxxxxxxx: tile code (0x0000 - 0x1fff)
		0002: unused?
		0004: 0000xxxxxxxxxxxx: x-coordinate absolute (-0x1ff to 0x01ff)
		      0x00000000000000: don't compensate for scrolling ??
		0006: 0000xxxxxxxxxxxx: y-coordinate absolute (-0x1ff to 0x01ff)
		0008: 00000000xxxxxxxx: color (0x00 - 0xff)
		      000000xx00000000: flipx & flipy
		      00000x0000000000: if clear, latch x & y coordinates, tile & color ?
		      0000x00000000000: if set, next sprite entry is part of sequence
		      000x000000000000: if clear, use latched y coordinate, else use current y
		      00x0000000000000: if set, y += 16
		      0x00000000000000: if clear, use latched x coordinate, else use current x
		      x000000000000000: if set, x += 16
		000a - 000f : unused?

		Additionally, the first 3 sprite entries are configuration info instead of
		actual sprites:

		offset 0x24: sprite x offset
		offset 0x26: sprite y offset
	*/
	int x,y,offs,code,color,spritecont,flipx,flipy;
	int xcurrent,ycurrent;
	int scroll1x, scroll1y;
	int scrollx=0, scrolly=0;
	int curx,cury;

	scroll1x = READ_WORD(&spriteram[0x24]);
	scroll1y = READ_WORD(&spriteram[0x26]);

	x = y = 0;
	xcurrent = ycurrent = 0;
	color = 0;

	for (offs = 0x30;offs < 0x3400;offs += 16)
	{
        spritecont = (READ_WORD(&spriteram[offs+8]) & 0xff00) >> 8;

		if ((spritecont & 0xf4) == 0)
		{
			x = READ_WORD(&spriteram[offs+4]);
			if (x & 0x4000)
			{
				scrollx = 0;
				scrolly = 0;
			}
			else
			{
				scrollx = scroll1x;
				scrolly = scroll1y;
			}
			x &= 0x1ff;
			y = READ_WORD(&spriteram[offs+6]) & 0x01ff;
			color = READ_WORD(&spriteram[offs+8]) & 0x00ff;

			xcurrent = x;
			ycurrent = y;
		}
		else
		{
			if ((spritecont & 0x10) == 0)
				y = ycurrent;
			else if ((spritecont & 0x20) != 0)
				y += 16;

			if ((spritecont & 0x40) == 0)
				x = xcurrent;
			else if ((spritecont & 0x80) != 0)
				x += 16;
		}

		code = READ_WORD(&spriteram[offs]) & 0x1fff;
#if 1
		{
			int bank;

			bank = (code & 0x1c00) >> 10;
			code = spritebank[bank] + (code & 0x3ff);
		}
#else
		CONVERT_SPRITE_CODE;
#endif
		if (!code) continue;
		flipx = spritecont & 0x01;
		flipy = spritecont & 0x02;

		// AJP (fixes sprites off right side of screen)
		curx = (x + scrollx) & 0x1ff;
		if (curx>0x140) curx -= 0x200;
		cury = (y + scrolly) & 0x1ff;
		if (cury>0x100) cury -= 0x200;

		if (color!=0xff)
		{
			drawgfx (bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				curx,cury,
				0,TRANSPARENCY_PEN,0);
		}
		else
		{
			// Mask sprite
			struct rectangle myclip;
			int tscrollx, tscrolly;

			myclip.min_x = curx;
			myclip.max_x = curx+15;
			myclip.min_y = cury;
			myclip.max_y = cury+15;
			tscrollx = READ_WORD (&taitof2_scrollx[0]) - ((READ_WORD (&spriteram[0x14])&0x7f)-0x50);
			tscrolly = READ_WORD (&taitof2_scrolly[0]) - 8;
#ifndef USE_TILEMAP
			copyscrollbitmap (tmpbitmap3,tmpbitmap,1,&tscrollx,1,&tscrolly,&myclip,TRANSPARENCY_NONE,0);
#endif
			tscrollx = READ_WORD (&taitof2_scrollx[2]) - ((READ_WORD (&spriteram[0x14])&0x7f)-0x50);
			tscrolly = READ_WORD (&taitof2_scrolly[2]) - 8;
#ifndef USE_TILEMAP
// TODO! fix for tilemaps
			copyscrollbitmap (tmpbitmap3,tmpbitmap2,1,&tscrollx,1,&tscrolly,&myclip,TRANSPARENCY_PEN,palette_transparent_pen);

			drawgfx (tmpbitmap3,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				curx,cury,
				0,TRANSPARENCY_PEN,15);

			copybitmap (bitmap,tmpbitmap3,0,0,0,0,&myclip,TRANSPARENCY_PEN,palette_transparent_pen);
#endif
		}
	}
}

void taitof2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
#ifndef USE_TILEMAP
	int scrollx, scrolly;
#endif
#ifdef MAME_DEBUG
	static int layer[5];
	static int dump_info = 1;
	char buf[80];
#endif

#ifdef USE_TILEMAP
	tilemap_set_scrollx( background_tilemap,0, -( READ_WORD(&taitof2_scrollx[0]) - ((READ_WORD(&spriteram[0x14])&0x7f)-0x50) ) );
	tilemap_set_scrolly( background_tilemap,0, -(READ_WORD(&taitof2_scrolly[0]) - 8) );
	tilemap_set_scrollx( foreground_tilemap,0, -( READ_WORD(&taitof2_scrollx[2]) - ((READ_WORD(&spriteram[0x14])&0x7f)-0x50) ) );
	tilemap_set_scrolly( foreground_tilemap,0, -(READ_WORD(&taitof2_scrolly[2]) - 8) );

	if (f2_video_version >= 1)
	{
		/* Decode any characters that have changed */
		{
			int i;

			for (i = 0; i < 256; i ++)
			{
				if (char_dirty[i])
				{
					decodechar (Machine->gfx[2],i,taitof2_characterram, Machine->drv->gfxdecodeinfo[2].gfxlayout);
					char_dirty[i] = 0;
				}
			}
		}
	}
#endif

#ifdef MAME_DEBUG
	if (keyboard_pressed (KEYCODE_Z))
	{
		while (keyboard_pressed (KEYCODE_Z) != 0) {};
		layer[0] ^= 1;
		sprintf(buf,"taitof2_foreground: %04x",layer[0]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed (KEYCODE_X))
	{
		while (keyboard_pressed (KEYCODE_X) != 0) {};
		layer[1] ^= 1;
		sprintf(buf,"taitof2_background: %04x",layer[1]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed (KEYCODE_C))
	{
		while (keyboard_pressed (KEYCODE_C) != 0) {};
		layer[2] ^= 1;
		sprintf(buf,"taitof2_sprites: %04x",layer[2]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed (KEYCODE_V))
	{
		while (keyboard_pressed (KEYCODE_V) != 0) {};
		layer[3] ^= 1;
		sprintf(buf,"taitof2_text: %04x",layer[3]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed (KEYCODE_B))
	{
		while (keyboard_pressed (KEYCODE_B) != 0) {};
		layer[4] ^= 1;
		sprintf(buf,"taitof2_pivot: %04x",layer[4]);
		usrintf_showmessage(buf);
	}
	if (keyboard_pressed (KEYCODE_N))
	{
		while (keyboard_pressed (KEYCODE_N) != 0) {};
		dump_info ^= 1;
	}
#endif



#ifdef USE_TILEMAP
	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
#endif
	taitof2_update_palette ();
#ifdef USE_TILEMAP
	tilemap_render(ALL_TILEMAPS);
#endif

#ifdef MAME_DEBUG
	if (layer[0]) goto f2_fore;
#endif

	/* SSI only uses sprites, so we clear the bitmap each frame */
	if (f2_video_version == 0)
		osd_clearbitmap(bitmap);

	if (f2_video_version >= 1)
	{
#ifdef USE_TILEMAP
		tilemap_draw(bitmap,background_tilemap,0);
#else
		/* Do the background layer */
		// This first pair seems to work only when video flip is on
//		scrollx = READ_WORD(&taitof2_scrollx[0]) - READ_WORD(&taitof2_scrollx[4]);
//		scrolly = READ_WORD(&taitof2_scrolly[0]) - READ_WORD(&taitof2_scrolly[4]);
		scrollx = READ_WORD(&taitof2_scrollx[0]) - ((READ_WORD(&videoram[0x14])&0x7f)-0x50);
		scrolly = READ_WORD(&taitof2_scrolly[0]) - 8;

		for (offs = 0;offs < f2_backgroundram_size;offs += 4)
		{
			int tile, color, flipx, flipy;

			tile = READ_WORD (&f2_backgroundram[offs + 2]);
			color = (READ_WORD (&f2_backgroundram[offs]) & 0xff);
			flipy = (READ_WORD (&f2_backgroundram[offs]) & 0x8000);
			flipx = (READ_WORD (&f2_backgroundram[offs]) & 0x4000);

			if (bg_dirty[offs/4])
			{
				int sx,sy;

				bg_dirty[offs/4] = 0;

				sy = (offs/4) / 64;
				sx = (offs/4) % 64;

				drawgfx(tmpbitmap,Machine->gfx[1],
					tile,
					color,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
			}
		}
		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
#endif
	}

#ifdef MAME_DEBUG
f2_fore:

	if (layer[1]) goto f2_sprite;
#endif

	if (f2_video_version >= 1)
	{
#ifdef USE_TILEMAP
		tilemap_draw(bitmap,foreground_tilemap,0);
#else
		/* Do the foreground layer */
		// This first pair seems to work only when video flip is on
//		scrollx = READ_WORD(&taitof2_scrollx[2]) - READ_WORD(&taitof2_scrollx[4]);
//		scrolly = READ_WORD(&taitof2_scrolly[2]) - READ_WORD(&taitof2_scrolly[4]);
		scrollx = READ_WORD(&taitof2_scrollx[2]) - ((READ_WORD(&videoram[0x14])&0x7f)-0x50);
		scrolly = READ_WORD(&taitof2_scrolly[2]) - 8;

		for (offs = 0;offs < f2_foregroundram_size;offs += 4)
		{
			int tile, color, flipx, flipy;

			tile = READ_WORD (&f2_foregroundram[offs + 2]);
			color = (READ_WORD (&f2_foregroundram[offs]) & 0xff);
			flipy = (READ_WORD (&f2_foregroundram[offs]) & 0x8000);
			flipx = (READ_WORD (&f2_foregroundram[offs]) & 0x4000);

			if (fg_dirty[offs/4])
			{
				int sx,sy;

				fg_dirty[offs/4] = 0;

				sy = (offs/4) / 64;
				sx = (offs/4) % 64;

				drawgfx(tmpbitmap2,Machine->gfx[1],
					tile,
					color,
					flipy,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
			}
		}
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
#endif
	}

#ifdef MAME_DEBUG
f2_sprite:
	if (layer[2]) goto f2_text;
#endif

	taitof2_draw_sprites (bitmap);

#ifdef MAME_DEBUG
f2_text:
	if (layer[3]) goto f2_pivot;
#endif

	if (f2_video_version >= 1)
	{
#ifndef USE_TILEMAP
		/* Decode any characters that have changed */
		{
			int i;

			for (i = 0; i < 256; i ++)
			{
				if (char_dirty[i])
				{
					decodechar (Machine->gfx[2],i,taitof2_characterram, Machine->drv->gfxdecodeinfo[2].gfxlayout);
					char_dirty[i] = 0;
				}
			}
		}
#endif
		for (offs = 0;offs < 0x1000;offs += 2)
		{
			int tile, color, flipx, flipy;

			tile = READ_WORD (&f2_textram[offs]) & 0xff;
			if (!tile) continue;

			color = (READ_WORD (&f2_textram[offs]) & 0x0f00) >> 8;
			flipy = (READ_WORD (&f2_textram[offs]) & 0x8000);
			flipx = (READ_WORD (&f2_textram[offs]) & 0x4000);

//			if (text_dirty[offs/2] || char_dirty[tile])
			{
				int sx,sy;


//				text_dirty[offs/2] = 0;
//				char_dirty[tile] = 0;

				sy = (offs/2) / 64;
				sx = (offs/2) % 64;

				drawgfx(bitmap,Machine->gfx[2],
					tile,
					color*4,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_PEN,0);
			}
		}
	}

#ifdef MAME_DEBUG
f2_pivot:

	if (layer[4]) goto f2_done;
#endif

	if (f2_video_version >= 2)
	{
#ifdef USE_TILEMAP
		tilemap_draw(bitmap,pivot_tilemap,0);
#else
		for (offs = 0;offs < f2_pivotram_size;offs += 2)
		{
			int tile/*, color, flipx, flipy*/;

			tile = READ_WORD (&f2_pivotram[offs]);
			if (!tile) continue;

//			color = (READ_WORD (&f2_pivotram[offs]) & 0xff);
//			flipy = (READ_WORD (&f2_pivotram[offs]) & 0x8000);
//			flipx = (READ_WORD (&f2_pivotram[offs]) & 0x4000);

//			if (text_dirty[offs/2] || char_dirty[tile])
			{
				int sx,sy;

//				text_dirty[offs/2] = 0;
//				char_dirty[tile] = 0;

				sy = (offs/2) / 64;
				sx = (offs/2) % 64;

				drawgfx(bitmap,Machine->gfx[3],
					tile,
					3,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
			}
		}
#endif
	}
//	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
#ifdef MAME_DEBUG
f2_done:

	if (f2_video_version >= 2)
	{
		if (dump_info)
		{
			int i;

			for (i = 0; i < 16; i += 2)
			{
				sprintf (buf, "%02x: %02x%02x", i, taitof2_rotate[i+0], taitof2_rotate[i+1]);
				ui_text (Machine->scrbitmap, buf, 0, (i/2)*8);
			}
		}
	}
#endif
	return;
}

