#include "driver.h"


static unsigned char *game_palette;	/* RGB palette as set by the driver. */
static unsigned short *game_colortable;	/* lookup table as set up by the driver */
static unsigned char shrinked_palette[3 * MAX_PENS];
static unsigned short shrinked_pens[MAX_PENS];
static unsigned char *palette_map;	/* map indexes from game_palette to shrinked_palette */


int palette_start(void)
{
	game_palette = malloc(3 * Machine->drv->total_colors * sizeof(unsigned char));
	game_colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));
	palette_map = malloc(Machine->drv->total_colors * sizeof(unsigned char));
	Machine->colortable = malloc(Machine->drv->color_table_len * sizeof(unsigned short));

	/* ASG 980209 - allocate space for the pens */
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)
		Machine->pens = malloc(32768 * sizeof (short));
	else
		Machine->pens = malloc(Machine->drv->total_colors * sizeof (short));

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
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT)       /* ASG 980209 - RGB/332 for 16-bit games */
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
 			(*Machine->drv->vh_convert_color_prom)(game_palette,game_colortable,Machine->gamedrv->color_prom);
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
	}

	for (i = 0;i < Machine->drv->color_table_len;i++)
		Machine->colortable[i] = Machine->pens[game_colortable[i]];
}



void palette_change_color(int color,unsigned char red, unsigned char green, unsigned char blue)
{
	if (Machine->drv->total_colors <= MAX_PENS)
	{
		if (color <= Machine->drv->total_colors)
		{
			shrinked_palette[3*color + 0] = game_palette[3*color + 0] = red;
			shrinked_palette[3*color + 1] = game_palette[3*color + 1] = green;
			shrinked_palette[3*color + 2] = game_palette[3*color + 2] = blue;
			osd_modify_pen(Machine->pens[color],red,green,blue);
		}
		else
if (errorlog) fprintf(errorlog,"error: palette_change_color() called with color %d, but only %d allocated.\n",color,Machine->drv->total_colors);
	}
	else
	{
		/* not implemented yet */
	}
}
