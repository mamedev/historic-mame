/******************************************************************************

  palette.c

  Palette handling functions.

  There are several levels of abstraction in the way MAME handles the palette,
  and several display modes which can be used by the drivers.

  Palette
  -------
  Note: in the following text, "color" refers to a color in the emulated
  game's virtual palette. For example, a game might be able to display 1024
  colors at the same time. If the game uses RAM to change the available
  colors, the term "palette" refers to the colors available at any given time,
  not to the whole range of colors which can be produced by the hardware. The
  latter is referred to as "color space".
  The term "pen" refers to one of the maximum of 256 colors can use to
  generate the display. PC users might want to think of them as the colors
  available in VGA, but be warned: the mapping of MAME pens to the VGA
  registers is not 1:1, so MAME's pen 10 will not necessarily be mapped to
  VGA's color #10 (actually this is never the case). This is done to unsure
  portability, since on some systems it is not possible to do a 1:1 mapping.

  So, to summarize, the three layers of palette abstraction are:

  P1) The game virtual palette (the "colors")
  P2) MAME's 256 colors palette (the "pens")
  P3) The OS specific hardware color registers (the "OS specific pens")

  The array Machine->pens is a lookup table which maps game colors to OS
  specific pens (P1 to P3). When you are working on bitmaps at the pixel level,
  *always* use Machine->pens to map the color numbers. *Never* use constants.
  For example if you want to make pixel (x,y) of color 3, do:
  bitmap->line[y][x] = Machine->pens[3];


  Lookup table
  ------------
  Palettes are only half of the story. To map the gfx data to colors, the
  graphics routines use a lookup table. For example if we have 4bpp tiles,
  which can have 256 different color codes, the lookup table for them will have
  256 * 2^4 = 4096 elements. For games using a palette RAM, the lookup table is
  usually a 1:1 map. For games using PROMs, the lookup table is often larger
  than the palette itself so for example the game can display 256 colors out
  of a palette of 16.

  The palette and the lookup table are initialized to default values by the
  main core, but can be initialized by the driver using the function
  MachineDriver->vh_convert_color_prom(). For games using palette RAM, that
  function is usually not needed, and the lookup table can be set up by
  properly initializing the color_codes_start and total_color_codes fields in
  the GfxDecodeInfo array.
  When vh_convert_color_prom() initializes the lookup table, it maps gfx codes
  to game colors (P1 above). The lookup table will be converted by the core to
  map to OS specific pens (P3 above), and stored in Machine->colortable.


  Display modes
  -------------
  The avaialble display modes can be summarized in four categories:
  1) Static palette. Use this for games which use PROMs for color generation.
     The palette is initialized by vh_convert_color_prom(), and never changed
     again.
  2) Dynamic palette. Use this for games which use RAM for color generation and
     have no more than 256 colors in the palette. The palette can be
     dynamically modified by the driver using the function
     palette_change_color(). MachineDriver->video_attributes must contain the
     flag VIDEO_MODIFIES_PALETTE.
  3) Dynamic shrinked palette. Use this for games which use RAM for color
     generation and have more than 256 colors in the palette. The palette can
     be dynamically modified by the driver using the function
     palette_change_color(). MachineDriver->video_attributes must contain the
     flag VIDEO_MODIFIES_PALETTE.
     The difference with case 2) above is that the driver must do some
     additional work to allow for palette reduction without loss of quality.
     The function palette_recalc() must be called every frame before doing any
     rendering. The argument to the function is an array telling which of the
     game colors are used, and how. This way the core can pick only the needed
     colors, and make the palette fit into 256 colors. The return code of
     palette_recalc() tells the driver whether the lookup table has changed,
	 and therefore whether a screen refresh is needed.
  4) 16-bit color. This should only be used for games which use more than 256
     colors at a time. It is slower and more awkward to use than the other
     modes, so it should be avoided whenever possible.
	 When this mode is used, the driver must manually modify the lookup table
	 to match changes in the palette. But video can be downgraded to 8-bit, but
	 with a noticeable decrease in quality.
     MachineDriver->video_attributes must contain the flag
     VIDEO_SUPPPORTS_16BIT.

  The important difference between cases 3) and 4) is that in 3), color cycling
  (which many games use) is essentially free, while in 4) every palette change
  requires a screen refresh. The color quality in 3) is also better than in 4).
  The dynamic shrinking of the palette works this way: as colors are requested,
  they are associated to a pen: When a color is no longer needed, the pen is
  freed and can be used for another color. When the code runs out of free pens,
  it compacts the palette, putting together colors with the same RGB
  components, then starts again to allocate pens for each new color. The bottom
  line of all this is that the pen assignment will automatically adapt to the
  game needs, and colors which change often will be assigned an exclusive pen,
  which can be modified using the video cards hardware registers without need
  for a screen refresh.

******************************************************************************/

#include "driver.h"


static unsigned char *game_palette;	/* RGB palette as set by the driver. */
static unsigned short *game_colortable;	/* lookup table as set up by the driver */
static unsigned char shrinked_palette[3 * MAX_PENS];
static unsigned short shrinked_pens[MAX_PENS];
static unsigned short *palette_map;	/* map indexes from game_palette to shrinked_palette */
static unsigned short pen_usage_count[MAX_PENS];
/* arrays which keep track of colors actually used, to help in the palette shrinking. */
static unsigned char *old_used_colors;

unsigned short palette_transparent_pen;

#define TRANSPARENT_PEN 0
#define BLACK_PEN 1
#define FIRST_AVAILABLE_PEN 2


int palette_start(void)
{
	game_palette = malloc(3 * Machine->drv->total_colors * sizeof(unsigned char));
	game_colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));
	palette_map = malloc(Machine->drv->total_colors * sizeof(unsigned short));
	Machine->colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));

	/* ASG 980209 - allocate space for the pens */
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
		Machine->pens = malloc(32768 * sizeof (short));
	else
		Machine->pens = malloc(Machine->drv->total_colors * sizeof (short));

	if ((Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE) &&
			Machine->drv->total_colors > MAX_PENS)
	{
		/* if the palette changes dynamically and has more than 256 colors, */
		/* we'll need the usage arrays to help in shrinking. */
		old_used_colors = malloc(Machine->drv->total_colors * sizeof(unsigned char));

		if (old_used_colors == 0)
		{
			palette_stop();
			return 1;
		}
	}
	else old_used_colors = 0;

	if (game_colortable == 0 || game_palette == 0 || palette_map == 0 ||
			Machine->colortable == 0 || Machine->pens == 0)
	{
		palette_stop();
		return 1;
	}

	return 0;
}

void palette_stop(void)
{
	free(old_used_colors);
	old_used_colors = 0;
	free(game_palette);
	game_palette = 0;
	free(game_colortable);
	game_colortable = 0;
	free(palette_map);
	palette_map = 0;
	free(Machine->colortable);
	Machine->colortable = 0;
	free(Machine->pens);
	Machine->pens = 0;
}



void palette_init(void)
{
	int i;


	/* convert the palette */
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
	{
		unsigned char *p = game_palette;
		int r, g, b;

		for (r = 0; r < 8; r++)
		{
			for (g = 0; g < 8; g++)
			{
				for (b = 0; b < 4; b++)
				{
					*p++ = (r << 5) | (r << 2) | (r >> 1);
					*p++ = (g << 5) | (g << 2) | (g >> 1);
					*p++ = (b << 6) | (b << 4) | (b << 2) | b;
				}
			}
		}

		for (i = 0;i < Machine->drv->color_table_len;i++)
			game_colortable[i] = i % 256;
	}
	else if (Machine->gamedrv->palette)
	{
		memcpy(game_palette,Machine->gamedrv->palette,3 * Machine->drv->total_colors * sizeof(unsigned char));
		memcpy(game_colortable,Machine->gamedrv->colortable,Machine->drv->color_table_len * sizeof(unsigned short));
	}
	else
	{
		unsigned char *p = game_palette;


		/* We initialize the palette and colortable to some default values so that */
		/* drivers which dynamically change the palette don't need a convert_color_prom() */
		/* function (provided the default color table fits their needs). */
		/* The default color table follows the same order of the palette. */

		for (i = 0;i < Machine->drv->total_colors;i++)
		{
			*(p++) = ((i & 1) >> 0) * 0xff;
			*(p++) = ((i & 2) >> 1) * 0xff;
			*(p++) = ((i & 4) >> 2) * 0xff;
		}

		for (i = 0;i < Machine->drv->color_table_len;i++)
			game_colortable[i] = i % Machine->drv->total_colors;

		/* now the driver can modify the dafult values if it wants to. */
		if (Machine->drv->vh_convert_color_prom)
		{
			const unsigned char *color_prom;


			color_prom = Machine->gamedrv->color_prom;

			/* if the special PROM_MEMORY_REGION() macro has been used, make */
			/* color_prom point to the specified memory region. */
			for (i = 0;i < MAX_MEMORY_REGIONS;i++)
			{
				if (color_prom == PROM_MEMORY_REGION(i))
				{
					color_prom = Machine->memory_region[i];
					break;
				}
			}
 			(*Machine->drv->vh_convert_color_prom)(game_palette,game_colortable,color_prom);
		}
	}

	if (Machine->drv->total_colors <= MAX_PENS)
	{
		for (i = 0;i < Machine->drv->total_colors;i++)
		{
			palette_map[i] = i;
			shrinked_palette[3*i + 0] = game_palette[3*i + 0];
			shrinked_palette[3*i + 1] = game_palette[3*i + 1];
			shrinked_palette[3*i + 2] = game_palette[3*i + 2];
		}
	}
	else
	{
		if (Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
		{
			/* copy over the default palette... */
			for (i = 0;i < MAX_PENS;i++)
			{
				palette_map[i] = i;
				shrinked_palette[3*i + 0] = game_palette[3*i + 0];
				shrinked_palette[3*i + 1] = game_palette[3*i + 1];
				shrinked_palette[3*i + 2] = game_palette[3*i + 2];

				pen_usage_count[i] = 0;
			}

			/* ... but allocate two fixed pens at the beginning: */
			/*  transparent black */
			shrinked_palette[3*TRANSPARENT_PEN+0] = 0;
			shrinked_palette[3*TRANSPARENT_PEN+1] = 0;
			shrinked_palette[3*TRANSPARENT_PEN+2] = 0;
			pen_usage_count[TRANSPARENT_PEN] = 1;	/* so the pen will not be reused */

			/* non transparent black */
			shrinked_palette[3*BLACK_PEN+0] = 0;
			shrinked_palette[3*BLACK_PEN+1] = 0;
			shrinked_palette[3*BLACK_PEN+2] = 0;
			pen_usage_count[BLACK_PEN] = 1;

			/* clear out the game palette, just to be safe from optimizations */
			memset(game_palette,0,3 * Machine->drv->total_colors * sizeof(unsigned char));


			/* create some defaults associations of game colors to shrinked pens. */
			/* They will be dynamically modified at run time. */
			for (i = 0;i < Machine->drv->total_colors;i++)
			{
				palette_map[i] = i % (MAX_PENS-2) + 2;	/* leave out the two blacks */
				old_used_colors[i] = PALETTE_COLOR_UNUSED;
			}
		}
		else
		{
			int j,used;


if (errorlog) fprintf(errorlog,"shrinking %d colors palette...\n",Machine->drv->total_colors);
			/* shrink palette to fit */
			used = 0;

			for (i = 0;i < Machine->drv->total_colors;i++)
			{
				for (j = 0;j < used;j++)
				{
					if (shrinked_palette[3*j + 0] == game_palette[3*i + 0] &&
							shrinked_palette[3*j + 1] == game_palette[3*i + 1] &&
							shrinked_palette[3*j + 2] == game_palette[3*i + 2])
						break;
				}

				palette_map[i] = j;

				if (j == used)
				{
					used++;
					if (used > MAX_PENS)
					{
						used = MAX_PENS;
if (errorlog) fprintf(errorlog,"error: ran out of free pens to shrink the palette.\n");
					}
					else
					{
						shrinked_palette[3*j + 0] = game_palette[3*i + 0];
						shrinked_palette[3*j + 1] = game_palette[3*i + 1];
						shrinked_palette[3*j + 2] = game_palette[3*i + 2];
					}
				}
			}

if (errorlog) fprintf(errorlog,"shrinked palette uses %d colors\n",used);

			/* fill out the remaining palette entries with black */
			while (used < MAX_PENS)
			{
				shrinked_palette[3*used + 0] = 0;
				shrinked_palette[3*used + 1] = 0;
				shrinked_palette[3*used + 2] = 0;

				used++;
			}
		}
	}


	if (Machine->scrbitmap->depth == 16)
	{
		osd_allocate_colors(32768,0,Machine->pens);
	}
	else
	{
		osd_allocate_colors(
				(Machine->drv->total_colors > MAX_PENS) ? MAX_PENS : Machine->drv->total_colors,
				shrinked_palette,
				shrinked_pens);

		for (i = 0;i < Machine->drv->total_colors;i++)
			Machine->pens[i] = shrinked_pens[palette_map[i]];

		palette_transparent_pen = shrinked_pens[TRANSPARENT_PEN];	/* for dynamic palette games */
	}

	for (i = 0;i < Machine->drv->color_table_len;i++)
		Machine->colortable[i] = Machine->pens[game_colortable[i]];
}



void palette_change_color(int color,unsigned char red, unsigned char green, unsigned char blue)
{
	if ((Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE) == 0)
	{
if (errorlog) fprintf(errorlog,"Error: palette_change_color() called, but VIDEO_MODIFIES_PALETTE not set.\n");
		return;
	}

	if (color < Machine->drv->total_colors)
	{
		int pen,change_now;

		game_palette[3*color + 0] = red;
		game_palette[3*color + 1] = green;
		game_palette[3*color + 2] = blue;

		pen = palette_map[color];

		change_now = 0;
		if (Machine->drv->total_colors <= MAX_PENS)
			change_now = 1;
		else
		{
			/* if we are doing dynamic palette shrinking, modify the pen only if */
			/* the color is actually in use (and it's the only one mapped to that */
			/* pen). This is because if it is not in use, palette_map[] points to */
			/* the pen which was previously associated with it, but might now be */
			/* used by another one. */
			if (old_used_colors[color] != PALETTE_COLOR_UNUSED)
			{
				if (pen_usage_count[pen] == 1)
					change_now = 1;
				else
				{
/* 	TODO: this is just a quick hack to make palette_recalc() reassign this pen. */
/* this will have to be handled more cleanly. */
					pen_usage_count[pen]--;
					old_used_colors[color] = PALETTE_COLOR_UNUSED;
				}
			}
		}

		if (change_now)
		{
			shrinked_palette[3*pen + 0] = red;
			shrinked_palette[3*pen + 1] = green;
			shrinked_palette[3*pen + 2] = blue;
			osd_modify_pen(Machine->pens[color],red,green,blue);
		}
	}
	else
if (errorlog) fprintf(errorlog,"error: palette_change_color() called with color %d, but only %d allocated.\n",color,Machine->drv->total_colors);
}



int palette_recalc(const unsigned char *used_colors)
{
	int i;
	int first_free_pen;
	int did_remap = 0;
	int ran_out = 0;


	if (old_used_colors == 0)
	{
if (errorlog) fprintf(errorlog,"Error: called palette_recalc() but you were not supposed to.\n");
		return 0;
	}


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (old_used_colors[i] != PALETTE_COLOR_UNUSED && used_colors[i] == PALETTE_COLOR_UNUSED)
		{
			pen_usage_count[palette_map[i]]--;
			old_used_colors[i] = PALETTE_COLOR_UNUSED;
		}
	}

	first_free_pen = FIRST_AVAILABLE_PEN;
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (old_used_colors[i] == PALETTE_COLOR_UNUSED && used_colors[i] != PALETTE_COLOR_UNUSED)
		{
			int r,g,b;


			r = game_palette[3*i + 0];
			g = game_palette[3*i + 1];
			b = game_palette[3*i + 2];

			if (used_colors[i] == PALETTE_COLOR_TRANSPARENT)
			{
				/* use the fixed transparent black for this */
				if (did_remap == 0) did_remap = 1;

				palette_map[i] = TRANSPARENT_PEN;
				pen_usage_count[palette_map[i]]++;
				Machine->pens[i] = shrinked_pens[palette_map[i]];
				old_used_colors[i] = used_colors[i];
			}
			else if (used_colors[i] == PALETTE_COLOR_CONSTANT && r == 0 && g == 0 && b == 0)
			{
				/* we can use the fixed non transparent black for this */
				if (did_remap == 0) did_remap = 1;

				palette_map[i] = BLACK_PEN;
				pen_usage_count[palette_map[i]]++;
				Machine->pens[i] = shrinked_pens[palette_map[i]];
				old_used_colors[i] = used_colors[i];
			}
			else
			{
				if (pen_usage_count[palette_map[i]] == 0)	/* if possible, reuse the last associated pen */
				{
					pen_usage_count[palette_map[i]]++;
				}
				else	/* allocate a new pen */
				{
retry:
					while (first_free_pen < MAX_PENS && pen_usage_count[first_free_pen] > 0)
						first_free_pen++;

					if (first_free_pen < MAX_PENS)
					{
						if (used_colors[i] == PALETTE_COLOR_FIXED)
							did_remap = 2;	/* we'll have to redraw everything */
						else if (did_remap == 0) did_remap = 1;

						palette_map[i] = first_free_pen;
						pen_usage_count[palette_map[i]]++;
						Machine->pens[i] = shrinked_pens[palette_map[i]];
					}
					else
					{
						int saved;
						int i,j;


						/* Ran out of pens! We can't handle this yet. */

						ran_out++;

						if (ran_out == 1)
						{
							int subcount[5];


							for (i = 0;i < 5;i++)
								subcount[i] = 0;

							for (i = 0;i < Machine->drv->total_colors;i++)
							{
								subcount[used_colors[i]]++;
							}

//							if (subcount[1]+subcount[2]+subcount[3] > MAX_PENS)
							{
if (errorlog) fprintf(errorlog,"Ran out of pens! %d colors used (%d unused, %d constant, %d fixed, %d dynamic, %d transparent)\n",subcount[1]+subcount[2]+subcount[3],subcount[0],subcount[1],subcount[2],subcount[3],subcount[4]);
							}
						}

						saved = 0;
						for (i = 0;i < Machine->drv->total_colors;i++)
						{
							if (old_used_colors[i] != PALETTE_COLOR_UNUSED && palette_map[i] != TRANSPARENT_PEN)
							{
								for (j = i+1;j < Machine->drv->total_colors;j++)
								{
									if (old_used_colors[j] != PALETTE_COLOR_UNUSED &&
											palette_map[j] != TRANSPARENT_PEN &&
											palette_map[i] != palette_map[j] &&
											game_palette[3*i + 0] == game_palette[3*j + 0] &&
											game_palette[3*i + 1] == game_palette[3*j + 1] &&
											game_palette[3*i + 2] == game_palette[3*j + 2])
									{
										if (used_colors[j] == PALETTE_COLOR_FIXED)
											did_remap = 2;	/* we'll have to redraw everything */
										else if (did_remap == 0) did_remap = 1;

										pen_usage_count[palette_map[j]]--;
										if (pen_usage_count[palette_map[j]] == 0)
											saved++;
										palette_map[j] = palette_map[i];
										pen_usage_count[palette_map[j]]++;
										Machine->pens[j] = shrinked_pens[palette_map[j]];
									}
								}
							}
						}

if (errorlog) fprintf(errorlog,"Let's do the shrink! Saved %d pens\n",saved);

						if (saved)
						{
							first_free_pen = FIRST_AVAILABLE_PEN;
							goto retry;
						}
continue;
					}
				}


				shrinked_palette[3*palette_map[i] + 0] = r;
				shrinked_palette[3*palette_map[i] + 1] = g;
				shrinked_palette[3*palette_map[i] + 2] = b;
				osd_modify_pen(Machine->pens[i],r,g,b);

				old_used_colors[i] = used_colors[i];
			}
		}
	}


	if (did_remap)
	{
		/* rebuild the color lookup table */
		for (i = 0;i < Machine->drv->color_table_len;i++)
			Machine->colortable[i] = Machine->pens[game_colortable[i]];

if (errorlog) fprintf(errorlog,"Did a palette remap, need a full screen redraw.\n");
	}


	return (did_remap == 2);	/* only return TRUE if we changed COLOR_FIXED pens */
}



void palette_change_transparent_color(unsigned char red, unsigned char green, unsigned char blue)
{
	osd_modify_pen(palette_transparent_pen,red,green,blue);
}
