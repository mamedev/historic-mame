#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *taitob_fscroll;
unsigned char *taitob_bscroll;
unsigned char *b_backgroundram;
size_t b_backgroundram_size;
unsigned char *b_foregroundram;
size_t b_foregroundram_size;
unsigned char *b_textram;
size_t b_textram_size;


unsigned char video_control;


static unsigned char *text_dirty;
static unsigned char *bg_dirty;
static unsigned char *fg_dirty;
size_t b_paletteram_size;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;


unsigned char *taitob_pixelram;
static struct osd_bitmap *pixel_layer;



WRITE_HANDLER( taitob_video_control_w )
{
	int val = (data>>8)&0xff;

/*
* tetrist:
* 0x8f - ???
* 0xcf - ???
* 0xaf - display lower part of pixel layer
* 0xef - display upper part of pixel_layer
*
* crimec:
* 0xef - display upper part of pixel layer
* 0x28 - don't display  pixel layer (or maybe diplay the other part of it ???)
*        (but continue displaying the background, foreground, text, sprites)
* 0x29 - set just before enabling pixel layer
*        (note that the game doesn't clear the pixel layer between enabling and
*         disabling it, it just writes 0x29 here instead)
* 0x20 - set at the third level (in the garage)
*        (disable what ?)
*/


	if ( ((video_control & 1)==0) && ((val & 1)==1) ) /*kludge*/
	{
		fillbitmap(pixel_layer, Machine->pens[0x800], NULL);
	}

#if 0
	if (val != video_control)
	{
		usrintf_showmessage("video control = %02x",val);
	}
#endif
	video_control = val;

}

READ_HANDLER( tetrist_pixelram_r )
{
	return READ_WORD(&taitob_pixelram[offset]);
}

WRITE_HANDLER( tetrist_pixelram_w )
{
	int sx,sy,color;

	if ( (offset&1) == 0)
	{
		COMBINE_WORD_MEM(&taitob_pixelram[offset],data);

		sx = (offset) & 0x1ff;
		sy = (offset >> 9);

		color = READ_WORD(&taitob_pixelram[offset]);

		plot_pixel(pixel_layer, sx  , sy, Machine->pens[0x400 + ((color>>8) & 0xff)]);
		plot_pixel(pixel_layer, sx+1, sy, Machine->pens[0x400 + (color & 0xff)]);
	}
	else
	{
		logerror("pixelram write to odd address\n");
		/*usrintf_showmessage("write to odd address");*/
	}
}


WRITE_HANDLER( crimec_pixelram_w )
{
	int sx,sy,color;

	if ( (offset&1) == 0)
	{
		COMBINE_WORD_MEM(&taitob_pixelram[offset],data);

		sx = (offset) & 0x1ff;
		sy = (offset >> 9);

		color = READ_WORD(&taitob_pixelram[offset]);

		plot_pixel(pixel_layer, sx  , sy, Machine->pens[0x800 + ((color>>8) & 0xff)]);
		plot_pixel(pixel_layer, sx+1, sy, Machine->pens[0x800 + (color & 0xff)]);
	}
	else
	{
		logerror("pixelram write to odd address\n");
		/*usrintf_showmessage("write to odd address");*/
	}
}



int taitob_vh_start (void)
{

	text_dirty = malloc (b_textram_size/2);
	if (!text_dirty) return 1;
	memset(text_dirty,1,b_textram_size/2);

	bg_dirty = malloc (b_backgroundram_size/4);
	if (!bg_dirty)
	{
		free (text_dirty);
		return 1;
	}
	memset (bg_dirty,1,b_backgroundram_size/4);

	fg_dirty = malloc (b_foregroundram_size/4);
	if (!fg_dirty)
	{
		free (text_dirty);
		free (bg_dirty);
		return 1;
	}
	memset (fg_dirty,1,b_foregroundram_size/4);

	/* create a temporary bitmap slightly larger than the screen for the background */
	if ((tmpbitmap = osd_create_bitmap(64*16, 64*16)) == 0)
	{
		free (text_dirty);
		free (bg_dirty);
		free (fg_dirty);
		return 1;
	}

	/* create a temporary bitmap slightly larger than the screen for the foreground */
	if ((tmpbitmap2 = osd_create_bitmap(64*16, 64*16)) == 0)
	{
		free (text_dirty);
		free (bg_dirty);
		free (fg_dirty);
		osd_free_bitmap (tmpbitmap);
		return 1;
	}

	/* create a temporary bitmap slightly larger than the screen for the text layer */
	if ((tmpbitmap3 = osd_create_bitmap(64*8, 64*8)) == 0)
	{
		free (text_dirty);
		free (bg_dirty);
		free (fg_dirty);
		osd_free_bitmap (tmpbitmap);
		osd_free_bitmap (tmpbitmap2);
		return 1;
	}

	pixel_layer = osd_create_bitmap (512, 512);

	if (!pixel_layer)
	{
		free (text_dirty);
		free (bg_dirty);
		free (fg_dirty);
		osd_free_bitmap (tmpbitmap);
		osd_free_bitmap (tmpbitmap2);
		osd_free_bitmap (tmpbitmap3);
		return 1;
	}

	{
		int i;

		memset (paletteram, 0, b_paletteram_size); /* probably not needed */
		for (i = 0; i < b_paletteram_size/2; i++)
			palette_change_color (i,i*3,i*5,i*7); /*random colors*/
	}

	return 0;
}

/*unsigned char *b_rom;*/
void taitob_vh_stop (void)
{
#if 0
FILE *f1;
FILE *f2;
int i,j;
	f1 = fopen("rom0o.bin","wb");
	f2 = fopen("rom0e.bin","wb");
	if (f1)
	{
		for (i=0; i<0x40000; i+=2)
		{
			fputc(b_rom[i + 0], f1 );
			fputc(b_rom[i + 1], f2 );
		}
	}
	fclose(f1);
	fclose(f2);
#endif
	free (text_dirty);
	free (bg_dirty);
	free (fg_dirty);
	osd_free_bitmap (tmpbitmap);
	osd_free_bitmap (tmpbitmap2);
	osd_free_bitmap (tmpbitmap3);
	osd_free_bitmap (pixel_layer);
}


READ_HANDLER ( taitob_text_r )
{
	return READ_WORD(&b_textram[offset]);
}

WRITE_HANDLER( taitob_text_w )
{
	int oldword = READ_WORD (&b_textram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&b_textram[offset],newword);
		text_dirty[offset / 2] = 1;
	}
}

READ_HANDLER ( taitob_background_r )
{
	return READ_WORD(&b_backgroundram[offset]);
}
WRITE_HANDLER( taitob_background_w )
{
	int oldword = READ_WORD (&b_backgroundram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&b_backgroundram[offset],newword);
		bg_dirty[(offset&0x1fff) / 2] = 1;
	}
}

READ_HANDLER ( taitob_foreground_r )
{
	return READ_WORD(&b_foregroundram[offset]);
}
WRITE_HANDLER( taitob_foreground_w )
{
	int oldword = READ_WORD (&b_foregroundram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&b_foregroundram[offset],newword);
		fg_dirty[(offset&0x1fff) / 2] = 1;
	}
}

void taitob_update_palette (void)
{
	int i;
	int offs,color;
	unsigned int palette_map[256];


	palette_init_used_colors();

	memset(palette_map, 0, sizeof(palette_map));

	/* Background layer */
	for (offs = 0;offs < b_backgroundram_size/2;offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_backgroundram[offs]) & 0x0fff;
		color = 0xc0 + (READ_WORD (&b_backgroundram[offs+0x2000]) & 0x3f);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Foreground layer */
	for (offs = 0;offs < b_foregroundram_size/2;offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_foregroundram[offs]) & 0x0fff;
		color = 0x80 + (READ_WORD (&b_foregroundram[offs+0x2000]) & 0x3f);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Sprites */
	for (offs = 0;offs < 0x1980;offs += 16)
	{
		int tile;

		tile = READ_WORD(&videoram[offs]) & 0x1fff;
		color = 0x40 + (READ_WORD(&videoram[offs+2]) & 0x00ff);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Do the text layer */
	for (offs = 0; offs < 0x1000; offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_textram[offs]);
		color = (tile>>12) & 0x0f;
		tile &= 0x0fff;

		palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
	}

	/* Tell MAME about the color usage */
	for (color = 0; color < 256; color++)
	{
		int usage = palette_map[color];

		if (usage)
		{
			if (palette_map[color] & (1 << 0))
				palette_used_colors[ color * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1; i < 16; i++)
				if (palette_map[color] & (1 << i))
					palette_used_colors[ color * 16 + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc ())
	{
		memset (text_dirty, 1, b_textram_size/2);
		memset (bg_dirty, 1, b_backgroundram_size/4);
		memset (fg_dirty, 1, b_foregroundram_size/4);
	}
}

void crimec_update_palette (void)
{
	int i;
	int offs,color;
	unsigned int palette_map[256];


	palette_init_used_colors();

	memset(palette_map, 0, sizeof(palette_map));

	/* Background layer */
	for (offs = 0;offs < b_backgroundram_size/2;offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_backgroundram[offs]) & 0x0fff;
		color = 0x00 + (READ_WORD (&b_backgroundram[offs+0x2000]) & 0x3f);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Foreground layer */
	for (offs = 0;offs < b_foregroundram_size/2;offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_foregroundram[offs]) & 0x0fff;
		color = 0x40 + (READ_WORD (&b_foregroundram[offs+0x2000]) & 0x3f);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Sprites */
	for (offs = 0;offs < 0x1980;offs += 16)
	{
		int tile;

		tile = READ_WORD(&videoram[offs]) & 0x1fff;
		color = 0x80 + (READ_WORD(&videoram[offs+2]) & 0x00ff);

		palette_map[color] |= Machine->gfx[1]->pen_usage[tile];
	}

	/* Do the text layer */
	for (offs = 0; offs < 0x1000; offs += 2)
	{
		int tile;

		tile = READ_WORD (&b_textram[offs]);
		color = 0xc0 + ((tile>>12) & 0x0f);
		tile &= 0x0fff;

		palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
	}

	/* Tell MAME about the color usage */
	for (color = 0; color < 256; color++)
	{
		int usage = palette_map[color];

		if (usage)
		{
			if (palette_map[color] & (1 << 0))
				palette_used_colors[ color * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1; i < 16; i++)
				if (palette_map[color] & (1 << i))
					palette_used_colors[ color * 16 + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc ())
	{
		memset (text_dirty, 1, b_textram_size/2);
		memset (bg_dirty, 1, b_backgroundram_size/4);
		memset (fg_dirty, 1, b_foregroundram_size/4);
	}
}

void taitob_draw_sprites (struct osd_bitmap *bitmap)
{
	/*
		Sprite format:
		0000: 000xxxxxxxxxxxxx: tile code (0x0000 - 0x1fff)
		0002: 00000000xxxxxxxx: color (0x00 - 0xff)
		      x000000000000000: flipy
		      0x00000000000000: flipx
		      00??????00000000: unknown
		0004: xxxxxxxx xxxxxxxx: x-coordinate 16 bits signed
		0006: xxxxxxxx xxxxxxxx: y-coordinate 16 bits signed
		0008 - 000f : unused?

	*/
	int x,y,xlatch=0,ylatch=0,x_no=0,y_no=0,x_num=0,y_num=0,big_sprite=0;
	int data,offs,code,color,flipx,flipy;
	unsigned int zoomx, zoomy, zoomxlatch=0, zoomylatch=0;

	for (offs = 0x1980-16; offs >=0; offs -= 16)
	{

		code = READ_WORD(&videoram[offs]) & 0x1fff;
		if (!code) continue;

		color = READ_WORD(&videoram[offs+2]);
		flipx = color & 0x4000;
		flipy = color & 0x8000;

		color = 0x40 + (color & 0xff);

		x = READ_WORD(&videoram[offs+4]);
		y = READ_WORD(&videoram[offs+6]);

		if (x>0x8000)
			x -= 65536;
		if (y>0x8000)
			y -= 65536;

		data = READ_WORD(&videoram[offs+0x0a]);
		if (data)
		{
			if (!big_sprite)
			{
				x_num = (data>>8) & 0xff;
				y_num = (data) & 0xff;
				x_no  = 0;
				y_no  = 0;
				xlatch = x;
				ylatch = y;
				data = READ_WORD(&videoram[offs+0x08]);
				zoomxlatch = (data>>8) & 0xff;
				zoomylatch = (data) & 0xff;
				big_sprite = 1;
			}
		}

		data = READ_WORD(&videoram[offs+0x08]);
		zoomx = (data>>8) & 0xff;
		zoomy = (data) & 0xff;

		if (big_sprite)
		{
			zoomx = zoomxlatch;
			zoomy = zoomylatch;

			x = xlatch + x_no*16;
			y = ylatch + y_no*16;
			y_no++;
			if (y_no>y_num)
			{
				y_no = 0;
				x_no++;
				if (x_no>x_num)
					big_sprite = 0;
			}
		}


		if ( (zoomx!=0) || (zoomy!=0) )
		{
			//logerror("1.zoomx=%8x zoomy=%8x\n",zoomx,zoomy);
			if (zoomx==0)
			{
				zoomx = (1<<16);
			}
			else
			{
				zoomx = (0x100 - zoomx);	/*1 to 255*/
				zoomx = ((zoomx/16)+1)<<12;
			}

			if (zoomy==0)
			{
				zoomy = (1<<16);
			}
			else
			{
				zoomy = (0x100 - zoomy);
				zoomy = ((zoomy/16)+1)<<12;
			}

			//logerror("2.zoomx=%8x zoomy=%8x\n",zoomx,zoomy);
			/*temporarily disabled - caused Mame to crash :(*/
		#if 0
			drawgfxzoom (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,palette_transparent_pen,zoomx,zoomy);
		#endif
			drawgfx (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,0);
		}
		else
		{
			drawgfx (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,0);
		}
	}
}

void crimec_draw_sprites (struct osd_bitmap *bitmap)
{
	/*
		Sprite format:
		0000: 000xxxxxxxxxxxxx: tile code (0x0000 - 0x1fff)
		0002: 00000000xxxxxxxx: color (0x00 - 0xff)
		      x000000000000000: flipy
		      0x00000000000000: flipx
		      00??????00000000: unused ?
		0004: xxxxxxxxxxxxxxxx: x-coordinate 16 bits signed (all zero for big sprites)
		0006: xxxxxxxxxxxxxxxx: y-coordinate 16 bits signed (all zero for big sprites)


		0008: ????????!!!!!!!!: sprite zoom level
		      ??? and !!! are usualy the same (x scale and y scale ?)

		sprite zoom is used at the end of the third level (in the garage)
		where there are four columns on the sides of the screen


		000a: xxxxxxxx00000000: x-sprites number (big sprite)
		      00000000xxxxxxxx: y-sprites number (big sprite)

		000c - 000f: unused ???
	*/

	int x,y,xlatch=0,ylatch=0,x_no=0,y_no=0,x_num=0,y_num=0,big_sprite=0;
	int data,offs,code,color,flipx,flipy;
	unsigned int zoomx, zoomy, zoomxlatch=0, zoomylatch=0;

	for (offs = 0x1980-16; offs >= 0; offs -= 16)
	{

		code = READ_WORD(&videoram[offs]) & 0x1fff;
		/*if (!code) continue;*/


		color = READ_WORD(&videoram[offs+2]);
		flipx = color & 0x4000;
		flipy = color & 0x8000;

		color = 0x80 + (color & 0xff);

		x = READ_WORD(&videoram[offs+4]);
		y = READ_WORD(&videoram[offs+6]);
		if (x>0x8000)
			x -= 65536;
		if (y>0x8000)
			y -= 65536;

		data = READ_WORD(&videoram[offs+0x0a]);
		if (data)
		{
			if (!big_sprite)
			{
				x_num = (data>>8) & 0xff;
				y_num = (data) & 0xff;
				x_no  = 0;
				y_no  = 0;
				xlatch = x;
				ylatch = y;
				data = READ_WORD(&videoram[offs+0x08]);
				zoomxlatch = (data>>8) & 0xff;
				zoomylatch = (data) & 0xff;
				big_sprite = 1;
			}
		}

		data = READ_WORD(&videoram[offs+0x08]);
		zoomx = (data>>8) & 0xff;
		zoomy = (data) & 0xff;

		if (big_sprite)
		{
			zoomx = zoomxlatch;
			zoomy = zoomylatch;

			x = xlatch + x_no*16;
			y = ylatch + y_no*16;
			y_no++;
			if (y_no>y_num)
			{
				y_no = 0;
				x_no++;
				if (x_no>x_num)
					big_sprite = 0;
			}
		}


		if ( (zoomx!=0) || (zoomy!=0) )
		{
			//logerror("1.zoomx=%8x zoomy=%8x\n",zoomx,zoomy);
			if (zoomx==0)
			{
				zoomx = (1<<16);
			}
			else
			{
				zoomx = (0x100 - zoomx);	/*1 to 255*/
				zoomx = ((zoomx/16)+1)<<12;
			}

			if (zoomy==0)
			{
				zoomy = (1<<16);
			}
			else
			{
				zoomy = (0x100 - zoomy);
				zoomy = ((zoomy/16)+1)<<12;
			}

			//logerror("2.zoomx=%8x zoomy=%8x\n",zoomx,zoomy);
			/*temporarily disabled - caused Mame to crash :(*/
		#if 0
			drawgfxzoom (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,palette_transparent_pen,zoomx,zoomy);
		#endif
			drawgfx (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,0);
		}
		else
		{
			drawgfx (bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				x,y,
				0,TRANSPARENCY_PEN,0);
		}
	}
}

void taitob_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx, scrolly;


	taitob_update_palette();

	/* Do the background layer */
	scrollx = READ_WORD(&taitob_bscroll[0]);
	scrolly = READ_WORD(&taitob_bscroll[2]);


	for (offs = 0;offs < b_backgroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_backgroundram[offs]) & 0x0fff;
		color = 0xc0 + (READ_WORD (&b_backgroundram[offs+0x2000]) & 0x003f);

		if (bg_dirty[offs/2])
		{
			int sx,sy;

			bg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Do the foreground layer */
	scrollx = READ_WORD(&taitob_fscroll[0]);
	scrolly = READ_WORD(&taitob_fscroll[2]);

	for (offs = 0;offs < b_foregroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_foregroundram[offs]) & 0x0fff;
		color = 0x80 + (READ_WORD (&b_foregroundram[offs+0x2000]) & 0x003f);

		if (fg_dirty[offs/2])
		{
			int sx,sy;


			fg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap2,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);


	taitob_draw_sprites (bitmap);


	for (offs = 0;offs < 0x1000;offs += 2)
	{
		if (text_dirty[offs/2] )
		{
			int sx,sy;
			int tile, color;

			tile  = READ_WORD (&b_textram[offs]);
			color = (tile>>12) & 0x0f;
			tile &= 0x0fff;

			text_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap3,Machine->gfx[0],
				tile,
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap3,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

}

void crimec_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx, scrolly;


	crimec_update_palette();

	/* Do the background layer */
	scrollx = READ_WORD(&taitob_bscroll[0]);
	scrolly = READ_WORD(&taitob_bscroll[2]);


	for (offs = 0;offs < b_backgroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_backgroundram[offs]) & 0x0fff;
		color = 0x00 + (READ_WORD (&b_backgroundram[offs+0x2000]) & 0x003f);

		if (bg_dirty[offs/2])
		{
			int sx,sy;

			bg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Do the foreground layer */
	scrollx = READ_WORD(&taitob_fscroll[0]);
	scrolly = READ_WORD(&taitob_fscroll[2]);

	for (offs = 0;offs < b_foregroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_foregroundram[offs]) & 0x0fff;
		color = 0x40 + (READ_WORD (&b_foregroundram[offs+0x2000]) & 0x003f);

		if (fg_dirty[offs/2])
		{
			int sx,sy;


			fg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap2,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	if (video_control==0xef) /*a HACK !*/
		copybitmap(bitmap,pixel_layer,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,Machine->pens[0x800]);


	crimec_draw_sprites (bitmap);


	for (offs = 0;offs < 0x1000;offs += 2)
	{
		if (text_dirty[offs/2] )
		{
			int sx,sy;
			int tile, color;

			tile  = READ_WORD (&b_textram[offs]);
			color = 0xc0 + ((tile>>12) & 0x0f);
			tile &= 0x0fff;

			text_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap3,Machine->gfx[0],
				tile,
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap3,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

}

void tetrist_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/*int offs;*/
	int scrollx, scrolly;

	/*crimec_update_palette();*/

	switch ( (video_control>>6) & 3 )
	{
	/*is the highest bit - pixel layer enable ? */

	case 0: /* 00xx xxxx */
		break;

	case 1: /* 01xx xxxx */
		break;

	case 2: /* 10xx xxxx */
		scrollx = 0;
		scrolly = 256;
		copyscrollbitmap(bitmap,pixel_layer,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		break;

	case 3: /* 11xx xxxx */
		scrollx = 0;
		scrolly = 0;
		copyscrollbitmap(bitmap,pixel_layer,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		break;
	}

#if 0
	/* Do the background layer */
	scrollx = READ_WORD(&taitob_bscroll[0]);
	scrolly = READ_WORD(&taitob_bscroll[2]);

	for (offs = 0;offs < b_backgroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_backgroundram[offs]) & 0x0fff;
		color = 0x00 + (READ_WORD (&b_backgroundram[offs+0x2000]) & 0x003f);

		if (bg_dirty[offs/2])
		{
			int sx,sy;

			bg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
#endif

#if 0
	/* Do the foreground layer */
	scrollx = READ_WORD(&taitob_fscroll[0]);
	scrolly = READ_WORD(&taitob_fscroll[2]);

	for (offs = 0;offs < b_foregroundram_size/2;offs += 2)
	{
		int tile, color;

		tile = READ_WORD (&b_foregroundram[offs]) & 0x0fff;
		color = 0x40 + (READ_WORD (&b_foregroundram[offs+0x2000]) & 0x003f);

		if (fg_dirty[offs/2])
		{
			int sx,sy;


			fg_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap2,Machine->gfx[1],
				tile,
				color,
				0,0,
				16*sx,16*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
#endif

#if 0
	crimec_draw_sprites (bitmap);
#endif


#if 0
	for (offs = 0;offs < 0x1000;offs += 2)
	{
		if (text_dirty[offs/2] )
		{
			int sx,sy;
			int tile, color;

			tile  = READ_WORD (&b_textram[offs]);
			color = 0xc0 + ((tile>>12) & 0x0f);
			tile &= 0x0fff;

			text_dirty[offs/2] = 0;

			sy = (offs/2) / 64;
			sx = (offs/2) % 64;

			drawgfx(tmpbitmap3,Machine->gfx[0],
				tile,
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap3,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
#endif

}

