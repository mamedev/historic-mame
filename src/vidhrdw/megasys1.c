/***************************************************************************

						-= Jaleco Mega System 1 =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows scroll 1
		W		shows scroll 2
		E		shows scroll 3
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

90000 50000 e0000	Scroll 1
94000 54000 e8000	Scroll 2
98000 58000 f0000	Scroll 3					* Note: missing on MS1-Z

Tile format:	fedc------------	Palette
				----ba9876543210	Tile Number



84000 44000 c2208	Layers Enable				* Note: missing on MS1-Z?

	fedc ---- ---- ---- ? (unused?)
	---- ba-- ---- ----	? (used!)
	---- --9- ---- ----	Sprites below fg (sprites over fg on MS1-C?)
	---- ---8 ---- ----	Swap txt with fg (swap bg with fg on MS1-C?)
	---- ---- 7654 ----	? (unused?)
	---- ---- ---- 3---	Enable Sprites
	---- ---- ---- -210	Enable Layer 321



84200 44200 c2000	Scroll 1 Control
84208 44208 c2008	Scroll 2 Control
84008 44008 c2100	Scroll 3 Control		* Note: missing on MS1-Z

Offset:		00					   Scroll X
			02					   Scroll Y
			04 fedc ba98 765- ---- ? (unused?)
			   ---- ---- ---4 ---- 16x16 Tiles
			   ---- ---- ---- 32-- ? (used, by p47!)
			   ---- ---- ---- --10 N: Layer H pages = 16 / (2^N)



84300 44300 c2308	Screen Control

	fedc ba9- ---- ---- ? (unused?)
	---- ---8 ---- ---- Portrait F/F (?FullFill?)
	---- ---- 765- ---- ? (unused?)
	---- ---- ---4 ---- ? (used, see p47j copyright screen!)
	---- ---- ---- 321- ? (unused?)
	---- ---- ---- ---0	Screen V Flip



**********  There are 256*4 colors (256*3 for MS1-Z):

Colors		MS1-A/C			MS1-Z

000-0ff		Scroll 1		Scroll 1
100-1ff		Scroll 2		Sprites
200-2ff		Scroll 3		Scroll 2
300-3ff		Sprites			-

88000 48000 f8000	Palette

	fedc--------3---	Red
	----ba98-----2--	Blue
	--------7654--1-	Green
	---------------0	? (used, not RGB! [not changed in fades])


**********  There are 256 sprites (128 for MS1-Z):

RAM[8000]	Sprite Data	(16 bytes/entry. 128? entries)

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

Object RAM entries:		Useb by sprites with:

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
bg  Scroll RAM 1	Scroll RAM 2
fg  Scroll RAM 2	Scroll RAM 1
txt Scroll RAM 3	Scroll RAM 3

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

extern unsigned char *scrollram[3],*scrollram_dirty[3];		// memory pointers
extern unsigned char *objectram, *videoregs, *ram;
extern int scrollx[3],scrolly[3],scrollflag[3],nx[3],ny[3];	// video registers
extern int active_layers, spritebank, screenflag, spriteflag,hardware_type;
extern int bg, fg, txt;
extern struct GameDriver avspirit_driver;

struct osd_bitmap *scroll_bitmap[3];


/* these are for debug purposes */
int debugsprites;


void mark_dirty(int n) { memset(scrollram_dirty[n],1,256*256/64*8); }
void mark_dirty_all(void)
{
	mark_dirty(0);
	mark_dirty(1);
	mark_dirty(2);
}

int vh_start(void)
{
int i;

/* allocate a dirty page for each scrolling layer */

	for (i = 0; i < 3; i++)
	 if ((scrollram_dirty[i] = malloc(256*4*256*2/64))==0) return 1;

/*  temporary bitmaps are not created here. They are instead
   created on the fly when refreshing the screen. */

	mark_dirty_all();

	return 0;
}

void vh_stop(void)
{
int i;

	for (i = 0; i < 3; i++)
	{
	 if (scroll_bitmap[i])		osd_free_bitmap(scroll_bitmap[i]);
	 if (scrollram_dirty[i])	free(scrollram_dirty[i]);
	}
}


/*

 Draw sprites in the given bitmap. Priority may be:

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
	0A		H position
	0C		V position
	0E		Number

*/
static void draw_sprites(struct osd_bitmap *bitmap, int priority)
{
int color,code,sx,sy,attr,xdisp,ydisp,ndisp,sprite,offs;
unsigned char *spritedata, *objectdata;

/* objram: 0x100*4 entries		spritedata: 0x80? entries */

	/* move the bit to the relevant position (and invert it) */
	if (priority != -1)	priority = (priority ^ 1) << 3;

	/* sprite order is from first in Sprite Data RAM (frontmost) to last */

	if (hardware_type != 'Z')
	{
		for (sprite = 0x80-1; sprite >= 0 ; sprite --)
		{
			spritedata = &spriteram[sprite*0x10];

			attr = READ_WORD(&spritedata[0x08]);

			if ( (attr & 0x08) == priority ) continue;
#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) != (debugsprites-1)) ) continue;
#endif

			objectdata = &objectram[((attr & 0xc0) >> 6) * 0x800];

			for (offs = 0x000; offs < 0x800 ; offs += 8)
			{
				/* seek a reference to current sprite */
				if ( sprite != READ_WORD(&objectdata[offs+0x00]) ) continue;

				/* apply the position displacements */
				sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&objectdata[offs+0x02]) ) % 512;
				sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&objectdata[offs+0x04]) ) % 512;

				if (sx > 256-1) sx -= 512;
				if (sy > 256-1) sy -= 512;

				/* out of screen */
				if ((sx > 256-1) ||(sy > 256-1) ||(sx < 0-15) ||(sy < 0-15)) continue;

				code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&objectdata[offs+0x06]);
				color = (attr & 0x0F);

				drawgfx(bitmap,Machine->gfx[3],
					(code & 0xfff ) + (spritebank << 12),
					color,
					attr & 0x40,attr & 0x80,
					sx, sy,
					&Machine->drv->visible_area,
					TRANSPARENCY_PEN,15);
			}	/* offs */
		}	/* sprite */
	}	/* non Z hw */
	else
	{

		/* MS1-Z just draws Sprite Data, and in reverse order */

		for (sprite = 0; sprite < 0x80 ; sprite ++)
		{
			spritedata = &spriteram[sprite*0x10];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0x08) == priority ) continue;
#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) == (debugsprites-1)) ) continue;
#endif

			sx = READ_WORD(&spritedata[0x0A]) % 512;
			sy = READ_WORD(&spritedata[0x0C]) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			/* out of screen */
			if ((sx > 256-1) ||(sy > 256-1) ||(sx < 0-15) ||(sy < 0-15)) continue;

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



/***************************************************************************

  Draw the game screen in the given osd_bitmap.

***************************************************************************/
void paletteram_RRRRGGGGBBBBRGBx_word_w(int offset, int data);
void vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
int offs,code,sx,sy,attr,i,j;
unsigned char *current_ram, *current_dirty;
int current_mask;
int mask[3] = {0xfff,0xfff,0xfff};

unsigned int *pen_usage;
int color_codes_start, color, colmask[16], layers_n;

int active_layers1;

	active_layers1 = active_layers;

	layers_n = 3;

	switch (hardware_type)
	{
		/* MS1-Z has scroll 1,2 & Sprites only (always active, no priority ?) */
		case 'Z':
		{
			layers_n = 2;
			bg = 0;		fg = 1;		txt = 2;
			active_layers = 0x020B;
		}	break;

		case 'A':
		{
			bg = 0;
			if (active_layers & 0x0100)	{fg = 2;	txt = 1;}
			else						{fg = 1;	txt = 2;}
		}

		case 'B':
		{
			if (Machine->gamedrv != &avspirit_driver)
			{
				bg = 0;
				if (active_layers & 0x0100)	{fg = 2;	txt = 1;}
				else						{fg = 1;	txt = 2;}
			}
			else
			{
				txt = 2;
				if (active_layers & 0x0100)	{bg = 0;	fg = 1;}
				else						{bg = 1;	fg = 0;}
			}
		}	break;

		case 'C':
		{
			active_layers ^= 0x0200;	/* sprites/fg priority swapped */
			txt = 2;
			if (active_layers & 0x0100)	{bg = 0;	fg = 1;}
			else						{bg = 1;	fg = 0;}
		}	break;

	}


/* Palette stuff */

	palette_init_used_colors();

/* We consider whole layers, redraw whole layers: worst case is slow! */

	for (j = 0 ; j < layers_n ; j++ )
	{
		/* let's skip disabled layers */
//		if (!(active_layers & (1 << j )))	continue;

		pen_usage = Machine->gfx[j]->pen_usage;
		color_codes_start = Machine->drv->gfxdecodeinfo[j].color_codes_start;

		for (color = 0 ; color < 16 ; color++) colmask[color] = 0;

		for (offs  = 0 ; offs < 0x4000 ; offs += 2)
		{
			color = ( READ_WORD(&scrollram[j][offs]) & 0xF000 ) >> 12;
			code  =   READ_WORD(&scrollram[j][offs]) & mask[j];

			if (scrollflag[j] & 0x10)
					colmask[color] |= pen_usage[code];
			else{	colmask[color] |= pen_usage[code*4+0];
					colmask[color] |= pen_usage[code*4+1];
					colmask[color] |= pen_usage[code*4+2];
					colmask[color] |= pen_usage[code*4+3];}
		}

		for (color = 0; color < 16; color++)
		{
			if (colmask[color])
			{
				for (i = 0; i < 16; i++)
					if (colmask[color] & (1 << i))
						palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
			}
		}

		if (j != bg)	/* background colors are all used */
		{
			for (color = 0; color < 16; color++)
				palette_used_colors[16 * color + 15 + color_codes_start] = PALETTE_COLOR_TRANSPARENT;
		}
	}



	/* Sprites */

	for (color = 0 ; color < 16 ; color++) colmask[color] = 0;

	/* different sprites hw */
	if (hardware_type == 'Z')
	{
	int sprite;

		pen_usage = Machine->gfx[2]->pen_usage;
		color_codes_start = Machine->drv->gfxdecodeinfo[2].color_codes_start;

		for (sprite = 0; sprite < 0x80 ; sprite++)
		{
		unsigned char* spritedata;

			spritedata = &spriteram[sprite*16];

			sx = READ_WORD(&spritedata[0x0A]) % 512;
			sy = READ_WORD(&spritedata[0x0C]) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > 256-1) ||(sy > 256-1) ||(sx < 0-15) ||(sy < 0-15)) continue;

			code  = READ_WORD(&spritedata[0x0E]);
			color = READ_WORD(&spritedata[0x08]) & 0x0F;

			colmask[color] |= pen_usage[code];
		}
	}
	else
	{
		pen_usage = Machine->gfx[3]->pen_usage;
		color_codes_start = Machine->drv->gfxdecodeinfo[3].color_codes_start;

		for (offs = 0; offs < 0x2000 ; offs += 8)
		{
		int sprite;
		unsigned char* spritedata;

			sprite = READ_WORD(&objectram[offs+0x00]);
			spritedata = &spriteram[(sprite&0x7F)*16];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0xc0) != ((offs/0x800)<<6) ) continue;

			sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&objectram[offs+0x02]) ) % 512;
			sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&objectram[offs+0x04]) ) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > 256-1) ||(sy > 256-1) ||(sx < 0-15) ||(sy < 0-15)) continue;

			code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&objectram[offs+0x06]);
			code  =	(code & 0xfff ) + (spritebank << 12);
			color = (attr & 0x0F);

			colmask[color] |= pen_usage[code];
		}
	}


	for (color = 0; color < 16; color++)
	 for (i = 0; i < 16; i++)
	  if (colmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;

/* hack: colors with bit 0 high will flash */
#if 0
	{
	unsigned char tmp_paletteram[256*4*2];

				for (color = 0; color < 256*4; color++)
					tmp_paletteram[color*2] = paletteram[color*2];

				for (color = 0; color < 256*4; color++)
				{
					if (paletteram_word_r(color*2)&1)
					{
						palette_used_colors[color] = PALETTE_COLOR_USED;
						paletteram_RRRRGGGGBBBBRGBx_word_w(color*2,(cpu_getcurrentframe() & 1)?0xF000:0xFFFF);
					}
				}

				if (palette_recalc()) mark_dirty_all();

				for (color = 0; color < 256*4; color++)
					paletteram[color*2] = tmp_paletteram[color*2];
	}
#else
	if (palette_recalc()) mark_dirty_all();
#endif



/* Allocate off screen bitmaps */

	for (i = 0; i < layers_n ; i++)
	{
		nx[i] = 16 / ( 1 << (scrollflag[i] & 0x3) );
		ny[i] = 32 / nx[i];

		if (scrollflag[i]&0x10)
		{
			if (ny[i] > 4)
			{
				nx[i] /= 1;
				ny[i] /= 4;
			}
			else
			{
				nx[i] /= 2;
				ny[i] /= 2;
			}
		}

		/* let's skip disabled layers */
		if ( (active_layers & (1 << i )) && (scroll_bitmap[i] == 0) )
		{
			if ((scroll_bitmap[i] = osd_new_bitmap(256*nx[i], 256*ny[i], 4))==0)
			{
			char buf[80];
				sprintf(buf, "NO MEM FOR %dx%d SCREEN",nx[i],ny[i]);
				if (errorlog) fprintf(errorlog, "%s\n", buf);
				usrintf_showmessage(buf);
				return;
			}
			else
			{
			char buf[80];
				sprintf(buf, "MALLOC'D %dx%d SCREEN",nx[i],ny[i]);
				if (errorlog) fprintf(errorlog, "%s\n", buf);
//				usrintf_showmessage(buf);
//				return;
			}

		}

	}


#ifdef MAME_DEBUG
debugsprites = 0;
if (osd_key_pressed(OSD_KEY_Z))
{
int msk = 0;

	if (osd_key_pressed(OSD_KEY_Q)) { msk |= 0xfff1;}
	if (osd_key_pressed(OSD_KEY_W)) { msk |= 0xfff2;}
	if (osd_key_pressed(OSD_KEY_E)) { msk |= 0xfff4;}
	if (osd_key_pressed(OSD_KEY_A))	{ msk |= 0xfff8; debugsprites = 1;}
	if (osd_key_pressed(OSD_KEY_S))	{ msk |= 0xfff8; debugsprites = 2;}
	if (osd_key_pressed(OSD_KEY_D))	{ msk |= 0xfff8; debugsprites = 3;}
	if (osd_key_pressed(OSD_KEY_F))	{ msk |= 0xfff8; debugsprites = 4;}

	if (msk != 0) active_layers &= msk;
}
#endif



/* Update the scrolling layers */

	for (i = 0; i < layers_n ; i++)
	{
		/* let's skip disabled layers */
		if (!(active_layers & (1 << i )))	continue;

		current_ram   = scrollram[i];
		current_dirty = scrollram_dirty[i];
		current_mask  = mask[i];

		if (scrollflag[i] & 0x10)	/* 8x8 tiles */
		{
		 for (j=0 ; j < nx[i]*ny[i] ; j++)
		  for (sx = (j%nx[i])*256; sx < (j%nx[i])*256+256; sx += 8)
		   for (sy = (j/nx[i])*256; sy < (j/nx[i])*256+256; sy += 8)
		   {
			if (current_dirty[0])
			{
				current_dirty[0] = 0;
				code = READ_WORD(&current_ram[0]);
				drawgfx(scroll_bitmap[i],Machine->gfx[i],
					code & current_mask,
					(code & 0xF000)>>12,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
			};
			current_dirty++;	current_ram+=2;
		   }
		}
		else	/* 16x16 tiles */
		{
		 for (j=0 ; j < nx[i]*ny[i] ; j++)
		  for (sx = (j%nx[i])*256; sx < (j%nx[i])*256+256; sx += 16)
		   for (sy = (j/nx[i])*256; sy < (j/nx[i])*256+256; sy += 16)
		   {
			if (current_dirty[0])
			{
				current_dirty[0] = 0;
				code = READ_WORD(&current_ram[0]);
				attr = (code >> 12) & 0x0F;
				code = (code & current_mask) * 4;
				drawgfx(scroll_bitmap[i],Machine->gfx[i],
					code+0,
					attr,
					0,0,sx+0,sy+0,0,TRANSPARENCY_NONE,0);
				drawgfx(scroll_bitmap[i],Machine->gfx[i],
					code+1,
					attr,
					0,0,sx+0,sy+8,0,TRANSPARENCY_NONE,0);
				drawgfx(scroll_bitmap[i],Machine->gfx[i],
					code+2,
					attr,
					0,0,sx+8,sy+0,0,TRANSPARENCY_NONE,0);
				drawgfx(scroll_bitmap[i],Machine->gfx[i],
					code+3,
					attr,
					0,0,sx+8,sy+8,0,TRANSPARENCY_NONE,0);
			};
			current_dirty++;	current_ram+=2;
		   }
		} /* 16x16 tiles */
	} /* i */

/* copy each (enabled) layer */


#define copylayer(_n_,_transparency_,_pen_)\
	if (active_layers & (1 << _n_ ))\
	{\
		copyscrollbitmap(bitmap, scroll_bitmap[_n_],\
				1,&scrollx[_n_],1,&scrolly[_n_],\
				&Machine->drv->visible_area,\
				_transparency_,_pen_);\
	}


	/* Copy the background */
	copylayer(bg,TRANSPARENCY_NONE,0)
	else	osd_clearbitmap(Machine->scrbitmap);

	/* If priority effect is active .. */
	if (spriteflag & 0x0100)
	{
		/* Copy the foreground below ... */
		if (!(active_layers &0x0200))	copylayer(fg,TRANSPARENCY_PEN,palette_transparent_pen);

		/* draw sprites with color >= 8 */
		if (active_layers & 0x08)	draw_sprites(bitmap,1);

		/* ... or over sprites with color >= 8 */
		if (active_layers &0x0200)	copylayer(fg,TRANSPARENCY_PEN,palette_transparent_pen);

		/* Draw sprites with color 0-7 */
		if (active_layers & 0x08)	draw_sprites(bitmap,0);
	}
	else
	{
		/* Copy the foreground below ... */
		if (!(active_layers &0x0200))	copylayer(fg,TRANSPARENCY_PEN,palette_transparent_pen);

		/* Draw sprites regardless of their color */
		if (active_layers & 0x08)	draw_sprites(bitmap,-1);

		/* ... or over sprites */
		if (active_layers &0x0200)	copylayer(fg,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	/* Copy the text layer */
	copylayer(txt,TRANSPARENCY_PEN,palette_transparent_pen);

	active_layers = active_layers1;
}
