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
  VGA's color #10 (actually this is never the case). This is done to ensure
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
  they are associated to a pen. When a color is no longer needed, the pen is
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
unsigned char *palette_used_colors;
static unsigned char *old_used_colors;

unsigned short palette_transparent_pen;

#define BLACK_PEN 0
#define TRANSPARENT_PEN 1
#define FIRST_AVAILABLE_PEN 2

#define PALETTE_COLOR_NEEDS_REMAP 0x80


int palette_start(void)
{
	game_palette = malloc(3 * Machine->drv->total_colors * sizeof(unsigned char));
	game_colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));
	palette_map = malloc(Machine->drv->total_colors * sizeof(unsigned short));
	Machine->colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));

	/* ASG 980209 - allocate space for the pens */
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
	{
		if (Machine->drv->total_colors != 256)
		{
if (errorlog) fprintf(errorlog,"Error: VIDEO_SUPPORTS_16BIT requires Machine->drv->total_colors to be 256\n");
			return 1;
		}

		Machine->pens = malloc(32768 * sizeof (short));
	}
	else
		Machine->pens = malloc(Machine->drv->total_colors * sizeof (short));

	if ((Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE) &&
			Machine->drv->total_colors > MAX_PENS)
	{
		/* if the palette changes dynamically and has more than 256 colors, */
		/* we'll need the usage arrays to help in shrinking. */
		palette_used_colors = malloc(2 * Machine->drv->total_colors * sizeof(unsigned char));

		if (palette_used_colors == 0)
		{
			palette_stop();
			return 1;
		}

		old_used_colors = palette_used_colors + Machine->drv->total_colors * sizeof(unsigned char);
		memset(palette_used_colors,PALETTE_COLOR_USED,Machine->drv->total_colors * sizeof(unsigned char));
		memset(old_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));
	}
	else palette_used_colors = old_used_colors = 0;

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
	free(palette_used_colors);
	palette_used_colors = old_used_colors = 0;
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


	/* Preload the colortable with a default setting, following the same */
	/* order of the palette. The driver can overwrite this either in */
	/* convert_color_prom(), or passing palette and colortable. */
	for (i = 0;i < Machine->drv->color_table_len;i++)
		game_colortable[i] = i % Machine->drv->total_colors;

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
	}
	else if (Machine->gamedrv->palette)
	{
		memcpy(game_palette,Machine->gamedrv->palette,3 * Machine->drv->total_colors * sizeof(unsigned char));
		if (Machine->gamedrv->colortable)
			memcpy(game_colortable,Machine->gamedrv->colortable,Machine->drv->color_table_len * sizeof(unsigned short));
	}
	else
	{
		unsigned char *p = game_palette;


		/* We initialize the palette and colortable to some default values so that */
		/* drivers which dynamically change the palette don't need a convert_color_prom() */
		/* function (provided the default color table fits their needs). */

		for (i = 0;i < Machine->drv->total_colors;i++)
		{
			*(p++) = ((i & 1) >> 0) * 0xff;
			*(p++) = ((i & 2) >> 1) * 0xff;
			*(p++) = ((i & 4) >> 2) * 0xff;
		}

		/* now the driver can modify the default values if it wants to. */
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

			/* create some defaults associations of game colors to shrinked pens. */
			/* They will be dynamically modified at run time. */
			for (i = 0;i < Machine->drv->total_colors;i++)
			{
				palette_map[i] = (i & 7) + 8;
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



void palette_change_color(int color,unsigned char red,unsigned char green,unsigned char blue)
{
	if ((Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE) == 0)
	{
if (errorlog) fprintf(errorlog,"Error: palette_change_color() called, but VIDEO_MODIFIES_PALETTE not set.\n");
		return;
	}

	if (color < Machine->drv->total_colors)
	{
		int pen,change_now;


		if (game_palette[3*color + 0] == red &&
				game_palette[3*color + 1] == green &&
				game_palette[3*color + 2] == blue)
			return;


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
			if (old_used_colors[color] == PALETTE_COLOR_USED)
			{
				/* if the color maps to an exclusive pen, just change it */
				if (pen_usage_count[pen] == 1)
					change_now = 1;
				else
				/* otherwise, we'll reassign the color in palette_recalc() */
					old_used_colors[color] = PALETTE_COLOR_NEEDS_REMAP;
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



int palette_recalc(void)
{
	int color;
	int i;
	int first_free_pen;
	int did_remap = 0;
	int need_refresh = 0;
	int ran_out = 0;
	int reuse_pens = 0;
	int need,avail;


	if (old_used_colors == 0)
	{
if (errorlog) fprintf(errorlog,"Error: called palette_recalc() but you were not supposed to.\n");
		return 0;
	}


	need = 0;
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (palette_used_colors[i] != PALETTE_COLOR_UNUSED && palette_used_colors[i] != old_used_colors[i])
		{
			/* to make sure that color cycling works, never reuse pens if there */
			/* are some which have just been modified. Better to run out of pens */
			/* and do a fresh compression of the palette, freeing space for cycling */
			/* colors, than ending up with a completely full palette and having */
			/* to continuously move the cycling colors around. */
			if (old_used_colors[i] == PALETTE_COLOR_NEEDS_REMAP)
			{
				need = 0;
				break;
			}

			need++;
		}
	}
	if (need > 0)
	{
		avail = 0;
		for (i = 0;i < MAX_PENS;i++)
		{
			if (pen_usage_count[i] == 0)
				avail++;
		}

		if (need > avail)
		{
if (errorlog) fprintf(errorlog,"Need %d new pens; %d available. I'll reuse some pens.\n",need,avail);
			reuse_pens = 1;
		}
	}

	first_free_pen = FIRST_AVAILABLE_PEN;
	for (color = 0;color < Machine->drv->total_colors;color++)
	{
		/* the comparison between palette_used_colors and old_used_colors also includes */
		/* PALETTE_COLOR_NEEDS_REMAP which might have been set by palette_change_color() */
		if (palette_used_colors[color] != PALETTE_COLOR_UNUSED &&
				palette_used_colors[color] != old_used_colors[color])
		{
			int r,g,b;


			if (old_used_colors[color] != PALETTE_COLOR_UNUSED)
			{
				did_remap = 1;
				need_refresh = 1;	/* we'll have to redraw everything */
				pen_usage_count[palette_map[color]]--;
				old_used_colors[color] = PALETTE_COLOR_UNUSED;
			}

			r = game_palette[3*color + 0];
			g = game_palette[3*color + 1];
			b = game_palette[3*color + 2];

			if (palette_used_colors[color] == PALETTE_COLOR_TRANSPARENT)
			{
				/* use the fixed transparent black for this */
				did_remap = 1;

				palette_map[color] = TRANSPARENT_PEN;
				pen_usage_count[palette_map[color]]++;
				Machine->pens[color] = shrinked_pens[palette_map[color]];
				old_used_colors[color] = palette_used_colors[color];
			}
			else
			{
				if (reuse_pens)
				{
					for (i = 0;i < MAX_PENS;i++)
					{
						if (pen_usage_count[i] > 0 &&
								shrinked_palette[3*i + 0] == r &&
								shrinked_palette[3*i + 1] == g &&
								shrinked_palette[3*i + 2] == b)
						{
							did_remap = 1;

							palette_map[color] = i;
							pen_usage_count[palette_map[color]]++;
							Machine->pens[color] = shrinked_pens[palette_map[color]];
							old_used_colors[color] = palette_used_colors[color];
							break;
						}
					}
				}

				/* if we still haven't found a pen, choose a new one */
				if (old_used_colors[color] != palette_used_colors[color])
				{
					/* if possible, reuse the last associated pen */
					if (pen_usage_count[palette_map[color]] == 0)
					{
						pen_usage_count[palette_map[color]]++;
					}
					else	/* allocate a new pen */
					{
retry:
						while (first_free_pen < MAX_PENS && pen_usage_count[first_free_pen] > 0)
							first_free_pen++;

						if (first_free_pen < MAX_PENS)
						{
							did_remap = 1;

							palette_map[color] = first_free_pen;
							pen_usage_count[palette_map[color]]++;
							Machine->pens[color] = shrinked_pens[palette_map[color]];
						}
						else
						{
							int saved;
							int i,j;


							/* Ran out of pens! Let's see what we can do. */

							if (ran_out == 0)
							{
								ran_out++;

								/* from now on, try to reuse already allocated pens */
								reuse_pens = 1;

if (errorlog)
{
	int subcount[3];


	for (i = 0;i < 3;i++)
		subcount[i] = 0;

	for (i = 0;i < Machine->drv->total_colors;i++)
		subcount[palette_used_colors[i]]++;

	fprintf(errorlog,"Ran out of pens! %d colors used (%d unused, %d used, %d transparent)\n",subcount[PALETTE_COLOR_USED]+subcount[PALETTE_COLOR_TRANSPARENT],subcount[PALETTE_COLOR_UNUSED],subcount[PALETTE_COLOR_USED],subcount[PALETTE_COLOR_TRANSPARENT]);
}

								saved = 0;

								/* first step: reclaim unused pens */
								/* This usually is fruitless */
								for (i = 0;i < Machine->drv->total_colors;i++)
								{
									if (old_used_colors[i] != PALETTE_COLOR_UNUSED &&
											palette_used_colors[i] == PALETTE_COLOR_UNUSED)
									{
										pen_usage_count[palette_map[i]]--;
										old_used_colors[i] = PALETTE_COLOR_UNUSED;
										if (pen_usage_count[palette_map[i]] == 0)
											saved++;
									}
								}


								/* second step: release pens used by colors which */
								/* need a remap. Usually no immediate effect, but */
								/* will improve the following steps. */
								for (i = 0;i < Machine->drv->total_colors;i++)
								{
									if (old_used_colors[i] == PALETTE_COLOR_NEEDS_REMAP)
									{
										did_remap = 1;
										need_refresh = 1;	/* we'll have to redraw everything */

										pen_usage_count[palette_map[i]]--;
										old_used_colors[i] = PALETTE_COLOR_UNUSED;
										if (pen_usage_count[palette_map[i]] == 0)
											saved++;
									}
								}


								/* third step: remap all blacks to BLACK_PEN */
								for (i = 0;i < Machine->drv->total_colors;i++)
								{
									if (old_used_colors[i] == PALETTE_COLOR_USED &&
											palette_map[i] != BLACK_PEN &&
											game_palette[3*i + 0] == 0 &&
											game_palette[3*i + 1] == 0 &&
											game_palette[3*i + 2] == 0)
									{
										did_remap = 1;
										need_refresh = 1;	/* we'll have to redraw everything */

										pen_usage_count[palette_map[i]]--;
										if (pen_usage_count[palette_map[i]] == 0)
											saved++;
										palette_map[i] = BLACK_PEN;
										pen_usage_count[palette_map[i]]++;
										Machine->pens[i] = shrinked_pens[palette_map[i]];
									}
								}


								/* final step: merge pens of the same color */
								for (i = 0;i < Machine->drv->total_colors;i++)
								{
									if (old_used_colors[i] == PALETTE_COLOR_USED)
									{
										for (j = i+1;j < Machine->drv->total_colors;j++)
										{
											if (old_used_colors[j] == PALETTE_COLOR_USED &&
													palette_map[i] != palette_map[j] &&
													game_palette[3*i + 0] == game_palette[3*j + 0] &&
													game_palette[3*i + 1] == game_palette[3*j + 1] &&
													game_palette[3*i + 2] == game_palette[3*j + 2])
											{
												did_remap = 1;
												need_refresh = 1;	/* we'll have to redraw everything */

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

if (errorlog) fprintf(errorlog,"Compressed the palette, saving %d pens\n",saved);
								if (saved)
								{
									first_free_pen = FIRST_AVAILABLE_PEN;
									goto retry;
								}
							}

							ran_out++;

							continue;	/* go on with the loop, we might have some */
										/* transparent pens to remap */
						}
					}

					shrinked_palette[3*palette_map[color] + 0] = r;
					shrinked_palette[3*palette_map[color] + 1] = g;
					shrinked_palette[3*palette_map[color] + 2] = b;
					osd_modify_pen(Machine->pens[color],r,g,b);

					old_used_colors[color] = palette_used_colors[color];
				}
			}
		}
	}

if (errorlog && ran_out > 1)
	fprintf(errorlog,"Error: no way to shrink the palette to 256 colors, left out %d colors.\n",ran_out-1);


	/* Reclaim unused pens; we do this AFTER allocating the new ones, to avoid */
	/* using the same pen for two different colors in two consecutive frames, */
	/* which might cause flicker. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (old_used_colors[i] != PALETTE_COLOR_UNUSED && palette_used_colors[i] == PALETTE_COLOR_UNUSED)
		{
			pen_usage_count[palette_map[i]]--;
			old_used_colors[i] = PALETTE_COLOR_UNUSED;
		}
	}

#ifdef MAME_DEBUG
	/* invalidate unused pens to make bugs in color allocation evident. */
	for (i = 0;i < MAX_PENS;i++)
	{
		if (pen_usage_count[i] == 0)
		{
			int r,g,b;
			r = rand() & 0xff;
			g = rand() & 0xff;
			b = rand() & 0xff;
			shrinked_palette[i + 0] = r;
			shrinked_palette[i + 1] = g;
			shrinked_palette[i + 2] = b;
			osd_modify_pen(shrinked_pens[i],r,g,b);
		}
	}
#endif

	if (did_remap)
	{
		/* rebuild the color lookup table */
		for (i = 0;i < Machine->drv->color_table_len;i++)
			Machine->colortable[i] = Machine->pens[game_colortable[i]];

if (errorlog && need_refresh) fprintf(errorlog,"Did a palette remap, need a full screen redraw.\n");
	}

	return need_refresh;
}



void palette_change_transparent_color(unsigned char red,unsigned char green,unsigned char blue)
{
	osd_modify_pen(palette_transparent_pen,red,green,blue);
}
