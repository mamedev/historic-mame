/***************************************************************************

  vidhrdw.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *nemesis_videoram1;
unsigned char *nemesis_videoram2;

unsigned char *nemesis_characterram;
unsigned char *nemesis_xscroll1,*nemesis_xscroll2,*nemesis_yscroll;

struct osd_bitmap *tmpbitmap2;

static unsigned char *video1_dirty;	/* 0x800 chars - foreground */
static unsigned char *video2_dirty;	/* 0x800 chars - background */
static unsigned char *char_dirty;	/* 2048 chars */
static unsigned char *sprite_dirty;	/* 512 sprites */



int nemesis_videoram1_r(int offset)
{
	return READ_WORD(&nemesis_videoram1[offset]);
}

void nemesis_videoram1_w(int offset,int data)
{
	COMBINE_WORD_MEM(&nemesis_videoram1[offset],data);
	if (offset < 0x1000)
		video1_dirty[offset / 2] = 1;
	else
		video2_dirty[(offset - 0x1000) / 2] = 1;
}

int nemesis_videoram2_r(int offset)
{
	return READ_WORD(&nemesis_videoram2[offset]);
}

void nemesis_videoram2_w(int offset,int data)
{
	COMBINE_WORD_MEM(&nemesis_videoram2[offset],data);
	if (offset < 0x1000)
		video1_dirty[offset / 2] = 1;
	else
		video2_dirty[(offset - 0x1000) / 2] = 1;
}



/* we have to straighten out the 16-bit word into bytes for gfxdecode() to work */
int nemesis_characterram_r(int offset)
{
	int res;

	res = READ_WORD(&nemesis_characterram[offset]);

	#ifdef LSB_FIRST
	res = ((res & 0x00ff) << 8) | ((res & 0xff00) >> 8);
	#endif

	return res;
}

void nemesis_characterram_w(int offset,int data)
{
	int oldword = READ_WORD(&nemesis_characterram[offset]);
	int newword;


	#ifdef LSB_FIRST
	data = ((data & 0x00ff00ff) << 8) | ((data & 0xff00ff00) >> 8);
	#endif

	newword = COMBINE_WORD(oldword,data);
	if (oldword != newword)
	{
		WRITE_WORD(&nemesis_characterram[offset],newword);

		char_dirty[offset / 32] = 1;
		sprite_dirty[offset / 128] = 1;
	}
}



/* free the palette dirty array */
void nemesis_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap2);
	tmpbitmap=0;
	free (char_dirty);
	char_dirty = 0;
	free (video1_dirty);
	free (video2_dirty);
}

/* claim a palette dirty array */
int nemesis_vh_start(void)
{
	if ((tmpbitmap = osd_new_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	if ((tmpbitmap2 = osd_new_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	char_dirty = malloc(2048);
	if (!char_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(char_dirty,1,2048);

	sprite_dirty = malloc(512);
	if (!sprite_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite_dirty,1,512);

	video1_dirty = malloc (0x800);
	video2_dirty = malloc (0x800);
	if (!video1_dirty || !video2_dirty)
	{
		nemesis_vh_stop();
		return 1;
	}
	memset(video1_dirty,1,0x800);
	memset(video2_dirty,1,0x800);

	return 0;
}



void draw_sprites(struct osd_bitmap *bitmap)
{
	/*
	 *	16 bytes per sprite, in memory from 56000-56fff
	 *
	 *	byte	0 :	relative priority.
	 *	byte	2 :	size (?) value #E0 means not used., bit 0x01 is flipx
	 * 	byte	4 :	zoom = 0xff
	 *	byte	6 :	low bits sprite pos.
	 *	byte	8 :	color + hi bits sprite pos., bit 0x20 is flipy
	 *	byte	A :	X position.
	 *	byte	C :	Y position.
	 * 	byte	E :	not used.
	 */

	int adress;	/* start of sprite in spriteram */
	int sx;	/* sprite X-pos */
	int sy;	/* sprite Y-pos */
	int sizeX,sizeY;
	int code;	/* start of sprite in obj RAM */
	int color;	/* color of the sprite */
	int active;	/* sprite on */
	int flipx,flipy;
//	int zoom;


	for (adress = 0;adress < spriteram_size;adress += 16)
	{
		if (READ_WORD(&spriteram[adress+4]) != 0xFF) 	/* values 80 && FF */
		{
			switch (READ_WORD(&spriteram[adress+2]) & 0x38)
			{
				case 0x38:
					sizeX = 16;
					sizeY = 16;
					active = 1;
					break;

				case 0x30:
					sizeX = 8;
					sizeY = 16;
					active = 0;
					break;

				case 0x20:
					sizeX = 8;
					sizeY = 8;
					active = 0;
					break;

				case 0x10:
					sizeX = 32;
					sizeY = 16;
					active = 0;
					break;

				case 0x18:
					sizeX = 64;
					sizeY = 64;
					active = 0;
					break;

				default:
					sizeX = 0;
					sizeY = 0;
					active = 0;
					break;
			}

			if (active)
			{
				sx = READ_WORD(&spriteram[adress+10]);
				sy = READ_WORD(&spriteram[adress+12]);
				code = READ_WORD(&spriteram[adress+6]) + ((READ_WORD(&spriteram[adress+8]) & 0xc0) << 2);
				code /= 2;
				color = (READ_WORD(&spriteram[adress+8]) & 0x1e) >> 1;
				flipx = READ_WORD(&spriteram[adress+2]) & 0x01;
				flipy = READ_WORD(&spriteram[adress+8]) & 0x20;

				if (sprite_dirty[code] == 1)
				{
					decodechar(Machine->gfx[1],code,nemesis_characterram,
							Machine->drv->gfxdecodeinfo[1].gfxlayout);
					sprite_dirty[code] = 2;
				}

				drawgfx(bitmap,Machine->gfx[1],
						code,
						color,
						flipx,flipy,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}

		} /* if sprite */
	} /* for loop */
}



void nemesis_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

{
	int color,code,i;
	int colmask[0x80];
	int pal_base;


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 0x80;color++) colmask[color] = 0;

	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD (&nemesis_videoram1[offs + 0x1000]) & 0x7ff;
		if (char_dirty[code] == 1)
		{
			decodechar(Machine->gfx[0],code,nemesis_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);
			char_dirty[code] = 2;
		}
		color = READ_WORD (&nemesis_videoram2[offs + 0x1000]) & 0x7f;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;

	for (color = 0;color < 0x80;color++) colmask[color] = 0;

	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		if (READ_WORD(&spriteram[offs+4]) != 0xFF)
		{
			color = (READ_WORD(&spriteram[offs+8]) & 0x1e) >> 1;
			colmask[color] |= 0xffff;
		}
	}

	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 0x80;color++) colmask[color] = 0;

	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD (&nemesis_videoram1[offs]) & 0x7ff;
		if (char_dirty[code] == 1)
		{
			decodechar(Machine->gfx[0],code,nemesis_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);
			char_dirty[code] = 2;
		}
		color = READ_WORD (&nemesis_videoram2[offs]) & 0x7f;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	if (palette_recalc())
	{
		memset(video1_dirty,1,0x800);
		memset(video2_dirty,1,0x800);
	}
}


	/* Do the background first */
	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		int code,color;


		code = READ_WORD (&nemesis_videoram1[offs + 0x1000]) & 0x7ff;
		color = READ_WORD (&nemesis_videoram2[offs + 0x1000]) & 0x7f;

		if (video2_dirty[offs/2] || char_dirty[code])
		{
			int sx,sy,flipx,flipy;


			video2_dirty[offs/2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			flipx = READ_WORD (&nemesis_videoram2[offs + 0x1000]) & 0x80;
			flipy =  READ_WORD (&nemesis_videoram1[offs + 0x1000]) & 0x800;

			drawgfx(tmpbitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}


	/* Copy the background bitmap */
	{
		int xscroll[256],yscroll;


		yscroll = -(READ_WORD(&nemesis_yscroll[0x300]) & 0xff);	/* used on level 2 */
		for (offs = 0;offs < 256;offs++)
		{
			xscroll[offs] = -((READ_WORD(&nemesis_xscroll2[2 * offs]) & 0xff) +
					((READ_WORD(&nemesis_xscroll2[0x200 + 2 * offs]) & 1) << 8));
		}
		copyscrollbitmap(bitmap,tmpbitmap,256,xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Do the foreground */
	/* We got a serious priority problem here :-) */
	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		int code,color;


		code = READ_WORD (&nemesis_videoram1[offs]) & 0x7ff;
		color = READ_WORD (&nemesis_videoram2[offs]) & 0x7f;

		if (video1_dirty[offs/2] || char_dirty[code])
		{
			int sx,sy,flipx,flipy;


			video1_dirty[offs/2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			flipx = READ_WORD (&nemesis_videoram2[offs]) & 0x80;
			flipy = READ_WORD (&nemesis_videoram1[offs]) & 0x800;

			drawgfx(tmpbitmap2,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}


	/* Copy the foreground bitmap */
	{
		int xscroll[256];


		for (offs = 0;offs < 256;offs++)
		{
			xscroll[offs] = -((READ_WORD(&nemesis_xscroll1[2 * offs]) & 0xff) +
					((READ_WORD(&nemesis_xscroll1[0x200 + 2 * offs]) & 1) << 8));
		}
		copyscrollbitmap(bitmap,tmpbitmap2,256,xscroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}


	draw_sprites(bitmap);


	for (offs = 0; offs < 2048; offs++)
	{
		if (char_dirty[offs] == 2)
			char_dirty[offs] = 0;
	}
	for (offs = 0; offs < 512; offs++)
	{
		if (sprite_dirty[offs] == 2)
			sprite_dirty[offs] = 0;
	}
}


void salamand_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
}
