#define GFX_REGIONS 1

/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "ctype.h"



extern unsigned char *tnzs_objram;
extern unsigned char *tnzs_vdcram;
extern unsigned char *tnzs_scrollram;
int tnzs_objram_size;
extern unsigned char *banked_ram_0, *banked_ram_1;


unsigned char *tnzs_objectram;
unsigned char *tnzs_paletteram;
unsigned char *tnzs_workram;
int tnzs_objectram_size;
static struct osd_bitmap *sc1bitmap;



/***************************************************************************

  Bubble Bobble doesn't have a color PROM. It uses 512 bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (4 bits per pixel; the low 4 bits of the second byte are unused).
  Since the graphics use 4 bitplanes, hence 16 colors, this makes for 16
  different color codes.

  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)
  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
        -- 2.2kohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
  bit 0 -- 2.2kohm resistor  -- GREEN

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
        -- 2.2kohm resistor  -- BLUE
         -- unused
        -- unused
        -- unused
  bit 0 -- unused

***************************************************************************/
void tnzs_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b,val;


	tnzs_paletteram[offset] = data;

	/* red component */
	val = tnzs_paletteram[offset & ~1];
	bit0 = (val >> 4) & 0x01;
	bit1 = (val >> 5) & 0x01;
	bit2 = (val >> 6) & 0x01;
	bit3 = (val >> 7) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	val = tnzs_paletteram[offset & ~1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	val = tnzs_paletteram[offset | 1];
	bit0 = (val >> 4) & 0x01;
	bit1 = (val >> 5) & 0x01;
	bit2 = (val >> 6) & 0x01;
	bit3 = (val >> 7) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	palette_change_color((offset / 2) ^ 0x0f,r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int tnzs_vh_start(void)
{
	if ((sc1bitmap = osd_create_bitmap(18 * 16, 16 * 16)) == 0)
		return 1;

	/* In Bubble Bobble the video RAM and the color RAM and interleaved, */
	/* forming a contiguous memory region 0x800 bytes long. We only need half */
	/* that size for the dirtybuffer */
	if ((dirtybuffer = malloc(videoram_size / 2)) == 0)
	{
		osd_free_bitmap(sc1bitmap);
		return 1;
	}
	memset(dirtybuffer,1,videoram_size / 2);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(sc1bitmap);
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void tnzs_vh_stop(void)
{
	osd_free_bitmap(sc1bitmap);
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer);
}



void tnzs_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset / 2] = 1;
		videoram[offset] = data;
	}
}



void tnzs_objectram_w(int offset,int data)
{
	if (tnzs_objectram[offset] != data)
	{
		/* gfx bank selector for the background playfield */
		if (offset < 0x40 && offset % 4 == 3) memset(dirtybuffer,1,videoram_size / 2);
		tnzs_objectram[offset] = data;
	}
}



void tnzs_vh_draw_background(struct osd_bitmap *bitmap,
					  unsigned char *m)
{
	static int sc1map[18][16];
	int i, b, c, x, y, sx, sy;

	/* this is probably the right place */
	sx = tnzs_scrollram[0x14]; sy = tnzs_scrollram[0x10];

	/* the byte at 0x24 is 0x20 ahead of sx's value, so if sx is
	   reading a true e0-ff, 0x24 will read a false but identical
	   value.  so if 0x24's value is different, sx's value is
	   false...  geddit? */
	if (sx >= 0xe0 && tnzs_scrollram[0x24] != sx) sx += 0x20;
	sx = -sx; sy = -sy;

	for (i = 0; i < 0x20 * 9; i++)
	{
		if (i > 0xff) { b = m[i - 0x200 + 0x1000]; c = m[i - 0x200]; }
		else { b = m[i + 0x1000]; c = m[i]; }

		x = ((i / 32) * 2 + (i % 2)); y = ((i % 32) / 2);

		if (sc1map[x][y] != 0x100*b+c)
		{
			sc1map[x][y] = 0x100*b+c;

			drawgfx(sc1bitmap,
					Machine->gfx[b / (32/GFX_REGIONS)],	/* bank */
					(b%(32/GFX_REGIONS)) * 0x100 + c, /* code */
					0, 0, 0,	/* color, flipx, flipy */
					(2 * 288 - 16*x - 16) % 288, /* x */
					(2 * 256 + 16*y - 16) % 256, /* y */
					0, TRANSPARENCY_NONE, 0); /* other stuff */
		}
	}

	copyscrollbitmap(bitmap,sc1bitmap,
					 1,&sx,
					 1,&sy,
					 &Machine->drv->visible_area,
					 TRANSPARENCY_COLOR,0);
}

void tnzs_vh_draw_foreground(struct osd_bitmap *bitmap,
							 unsigned char *char_pointer,
							 unsigned char *x_pointer,
							 unsigned char *y_pointer,
							 unsigned char *ctrl_pointer,
							 unsigned char *vis_pointer)
{
	int i, c;

	for (i = 0xff; i >= 0; i--)
	{
		c = char_pointer[i];
#if 0
		if (y_pointer[i] != 0xf8) /* this is either a special code
									 meaning 'don't draw' or f8-ff
									 just happens to be off the screen
									 */
#endif
		{
#if 0
			if (errorlog)
				fprintf(errorlog,
						"show a '%c' (%02x) at %d, %d\n",
						c, c,
						x_pointer[i],
						y_pointer[i]);
#endif

			if ((vis_pointer[i] & 1) == 0)
				drawgfx(bitmap,
						Machine->gfx[((ctrl_pointer[i] & 15) /
									  (32/GFX_REGIONS))],
						((ctrl_pointer[i] & 15)%
						 (32/GFX_REGIONS)) * 0x100 + c, /* code */
						0,			/* color */
						ctrl_pointer[i] & 0x80, 0, /* flipx, flipy */
						256 - 16 - (((x_pointer[i] - 16) % 256) + 16), /* x */
						256 - y_pointer[i] - 32, /* y */
						0,			/* clip */
						TRANSPARENCY_PEN, /* transparency */
						0);			/* transparent_color */
			/* } */
		}
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
extern int number_of_credits;
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	if (osd_key_pressed(OSD_KEY_3))
	{
		while (osd_key_pressed(OSD_KEY_3));
		number_of_credits++;
	}

#if 1
	fillbitmap(bitmap, Machine->pens[0],
			   &Machine->drv->visible_area);
#endif

	tnzs_vh_draw_background(bitmap, tnzs_objram + 0x500);

#if 0							/* this is the one i've been using */
	tnzs_vh_draw_foreground(bitmap,
							tnzs_workram + 0x400, /* chars : e400 */
							tnzs_workram + 0x200, /*     x : e200 */
							tnzs_workram + 0x100, /*     y : e100 */
							tnzs_workram + 0x500, /*  ctrl : e500 */
							tnzs_workram + 0x300); /*  vis : e300 */
#endif
	tnzs_vh_draw_foreground(bitmap,
							tnzs_objram + 0x0000, /* chars : c000 */
							tnzs_objram + 0x0200, /*     x : c200 */
							tnzs_vdcram + 0x0000, /*     y : f000 */
							tnzs_objram + 0x1000, /*  ctrl : d000 */
							tnzs_objram + 0x1200); /*  vis : d200 */

#if 0
	{
		int fps,i,j;
		static unsigned char *base, *basebase = NULL;
		static int show_hex = 0;

		if (basebase == NULL)
		{
			base = tnzs_objram;
			basebase = tnzs_objram - 0xc000;
		}
		if (osd_key_pressed(OSD_KEY_C))
		{
			while (osd_key_pressed(OSD_KEY_C));
			show_hex = 1 - show_hex;
		}
		if (osd_key_pressed(OSD_KEY_V))
		{
			while (osd_key_pressed(OSD_KEY_V));
			base = banked_ram_0;
			basebase = base - 0x8000;
		}
		if (osd_key_pressed(OSD_KEY_B))
		{
			while (osd_key_pressed(OSD_KEY_B));
			base = banked_ram_1;
			basebase = base - 0x8000;
		}
		if (osd_key_pressed(OSD_KEY_N))
		{
			while (osd_key_pressed(OSD_KEY_N));
			base = tnzs_objram;
			basebase = base - 0xc000;
		}
		if (osd_key_pressed(OSD_KEY_M))
		{
			while (osd_key_pressed(OSD_KEY_M));
			base = tnzs_workram;
			basebase = base - 0xe000;
		}
		if (show_hex)
		{
			if (osd_key_pressed(OSD_KEY_Z))
			{
				while (osd_key_pressed(OSD_KEY_Z));
				base -= 0x100;
			}
			if (osd_key_pressed(OSD_KEY_X))
			{
				while (osd_key_pressed(OSD_KEY_X));
				base+= 0x100;
			}

#define d(digit,x,y) \
			drawgfx(bitmap,Machine->uifont,digit+(digit > 9 ? ('A'-10) : '0'),DT_COLOR_WHITE, \
					0,0,8*(x),8*(y),0,TRANSPARENCY_NONE,0)
				for (i = 0; i < 16; i++){
					d(i, 0, i * 2);
					d(0, 1, i * 2);
				}
			for (j = 0;j < 16*2;j++){
				for (i = 0;i < 8;i++){
					fps = base[i + j * 8];

					d(fps/0x10, 3*i+4, j);
					d(fps%0x10, 3*i+5, j);
				}
			}
			fps = base - basebase;
			d((fps%0x10000)/0x1000, 28, 0);
			d((fps%0x1000 )/0x100 , 29, 0);
			d((fps%0x100  )/0x10  , 30, 0);
			d((fps%0x10   )/0x1   , 31, 0);
		}
	}
#endif
}
