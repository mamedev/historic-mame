/***************************************************************************

  atarigen.c

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/


#include "driver.h"
#include "atarigen.h"
#include "m6502/m6502.h"
#include "m68000/m68000.h"


void slapstic_init (int chip);
int slapstic_bank (void);
int slapstic_tweak (int offset);



int atarigen_cpu_to_sound, atarigen_cpu_to_sound_ready;
int atarigen_sound_to_cpu, atarigen_sound_to_cpu_ready;
int atarigen_special_color;

unsigned char *atarigen_paletteram;
unsigned char *atarigen_playfieldram;
unsigned char *atarigen_spriteram;
unsigned char *atarigen_alpharam;
unsigned char *atarigen_vscroll;
unsigned char *atarigen_hscroll;
unsigned char *atarigen_eeprom;
unsigned char *atarigen_slapstic;

int atarigen_playfieldram_size;
int atarigen_spriteram_size;
int atarigen_alpharam_size;
int atarigen_paletteram_size;
int atarigen_eeprom_size;

static int unlocked;

static void *comm_timer;
static void *stop_comm_timer;

static int colortype;
static int reuse_colors;
static unsigned char last_used[256];
static unsigned char this_used[256];
static int last_color[256];
static int this_color[256];
static int last_intensity;
static int current_black;
static unsigned short *this_colortable;
static unsigned short *last_colortable;

static struct atarigen_modesc *modesc;

static unsigned short *displaylist;
static unsigned short *displaylist_end;
static unsigned short *displaylist_last;

static void (*sound_int)(void);

static int ranout;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

void atarigen_init_machine (void (*_sound_int)(void), int slapstic)
{
	unlocked = 0;
	atarigen_cpu_to_sound = atarigen_cpu_to_sound_ready = 0;
	atarigen_sound_to_cpu = atarigen_sound_to_cpu_ready = 0;

	comm_timer = stop_comm_timer = NULL;

	sound_int = _sound_int;

	if (slapstic) slapstic_init (slapstic);
}



/*************************************
 *
 *		EEPROM read/write/enable.
 *
 *************************************/

void atarigen_eeprom_enable_w (int offset, int data)
{
	unlocked = 1;
}


int atarigen_eeprom_r (int offset)
{
	return READ_WORD (&atarigen_eeprom[offset]) | 0xff00;
}


void atarigen_eeprom_w (int offset, int data)
{
	if (!unlocked)
		return;

	COMBINE_WORD_MEM (&atarigen_eeprom[offset], data);
	unlocked = 0;
}



/*************************************
 *
 *		High score save/load
 *
 *************************************/

int atarigen_hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
	else
		memset (atarigen_eeprom, 0xff, atarigen_eeprom_size);

	return 1;
}


void atarigen_hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
}



/*************************************
 *
 *		Slapstic ROM read/write.
 *
 *************************************/

int atarigen_slapstic_r (int offset)
{
	int bank = slapstic_tweak (offset / 2) * 0x2000;
	return READ_WORD (&atarigen_slapstic[bank + (offset & 0x1fff)]);
}


void atarigen_slapstic_w (int offset, int data)
{
	slapstic_tweak (offset / 2);
}



/*************************************
 *
 *		Main CPU to sound CPU communications
 *
 *************************************/

static void atarigen_delayed_sound_reset (int param)
{
	cpu_reset (1);
	cpu_halt (1, 1);

	atarigen_cpu_to_sound_ready = atarigen_sound_to_cpu_ready = 0;
	atarigen_cpu_to_sound = atarigen_sound_to_cpu = 0;
}


void atarigen_sound_reset (void)
{
	timer_set (TIME_NOW, 0, atarigen_delayed_sound_reset);
}


static void atarigen_stop_comm_timer (int param)
{
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = stop_comm_timer = NULL;
}


static void atarigen_delayed_sound_w (int param)
{
	if (atarigen_cpu_to_sound_ready)
		if (errorlog) fprintf (errorlog, "Missed command from 68010\n");

	atarigen_cpu_to_sound = param;
	atarigen_cpu_to_sound_ready = 1;
	cpu_cause_interrupt (1, INT_NMI);

	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	if (!comm_timer)
		comm_timer = timer_pulse (TIME_IN_USEC (50), 0, 0);
	if (stop_comm_timer)
		timer_remove (stop_comm_timer);
	stop_comm_timer = timer_set (TIME_IN_USEC (1000), 0, atarigen_stop_comm_timer);
}


void atarigen_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	if (!(data & 0x00ff0000))
		timer_set (TIME_NOW, data & 0xff, atarigen_delayed_sound_w);
}


int atarigen_6502_sound_r (int offset)
{
	atarigen_cpu_to_sound_ready = 0;
	return atarigen_cpu_to_sound;
}



/*************************************
 *
 *		Sound CPU to main CPU communications
 *
 *************************************/

static void atarigen_delayed_6502_sound_w (int param)
{
	if (atarigen_sound_to_cpu_ready)
		if (errorlog) fprintf (errorlog, "Missed result from 6502\n");

	atarigen_sound_to_cpu = param;
	atarigen_sound_to_cpu_ready = 1;
	if (sound_int)
		(*sound_int) ();

	/* remove the high frequency timer if there is one */
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = NULL;
}


void atarigen_6502_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data, atarigen_delayed_6502_sound_w);
}


int atarigen_sound_r (int offset)
{
	atarigen_sound_to_cpu_ready = 0;
	return atarigen_sound_to_cpu | 0xff00;
}



/*************************************
 *
 *		MO display list management
 *
 *************************************/

int atarigen_init_display_list (struct atarigen_modesc *_modesc)
{
	modesc = _modesc;
	
	displaylist = malloc (modesc->maxmo * 10 * (Machine->drv->screen_height / 8));
	if (!displaylist)
		return 1;
	
	displaylist_end = displaylist;
	displaylist_last = NULL;
	
	return 0;
}


void atarigen_update_display_list (unsigned char *base, int start, int scanline)
{
	int link = start, match = 0, moskip = modesc->moskip, wordskip = modesc->mowordskip;
	int ignoreword = modesc->ignoreword;
	unsigned short *d, *startd, *lastd;
	unsigned char spritevisit[1024];

	/* scanline 0 means first update */
	if (scanline <= 0)
	{
		displaylist_end = displaylist;
		displaylist_last = NULL;
	}

	/* set up local pointers */
	startd = d = displaylist_end;
	lastd = displaylist_last;

	/* if the last list entries were on the same scanline, overwrite them */
	if (lastd)
	{
		if (*lastd == scanline)
			d = startd = lastd;
		else
			match = 1;
	}

	/* visit all the sprites and copy their data into the display list */
	memset (spritevisit, 0, sizeof (spritevisit));
	while (!spritevisit[link])
	{
		unsigned char *modata = &base[link * moskip];
		unsigned short data[4];

		/* bounds checking */
		if (d - displaylist >= modesc->maxmo * 5 * (Machine->drv->screen_height / 8))
		{
			if (errorlog) fprintf (errorlog, "Motion object list exceeded maximum\n");
			break;
		}

		/* start with the scanline */		
		*d++ = scanline;

		/* add the data words */
		data[0] = *d++ = READ_WORD (&modata[0]);
		data[1] = *d++ = READ_WORD (&modata[wordskip]);
		data[2] = *d++ = READ_WORD (&modata[2 * wordskip]);
		data[3] = *d++ = READ_WORD (&modata[3 * wordskip]);
		
		/* is this one to ignore? */
		if (data[ignoreword] == 0xffff)
			d -= 5;

		/* update our match status */
		else if (match)
		{
			lastd++;
			if (*lastd++ != data[0] || *lastd++ != data[1] || *lastd++ != data[2] || *lastd++ != data[3])
				match = 0;
		}

		/* link to the next object */
		spritevisit[link] = 1;
		if (modesc->linkword >= 0)
			link = (data[modesc->linkword] >> modesc->linkshift) & modesc->linkmask;
		else
			link = (link + 1) & modesc->linkmask;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		displaylist_end = d;
		displaylist_last = startd;
	}
}


void atarigen_render_display_list (struct osd_bitmap *bitmap, atarigen_morender morender, void *param)
{
	unsigned short *base = displaylist;
	int last_start_scan = -1;
	struct rectangle clip;

	/* create a clipping rectangle so that only partial sections are updated at a time */
	clip.min_x = 0;
	clip.max_x = Machine->drv->screen_width - 1;

	/* loop over the list until the end */
	while (base < displaylist_end)
	{
		unsigned short *d, *first, *last;
		int start_scan = base[0], step;
		
		last_start_scan = start_scan;
		clip.min_y = start_scan;

		/* look for an entry whose scanline start is different from ours; that's our bottom */
		for (d = base; d < displaylist_end; d += 5)
			if (*d != start_scan)
			{
				clip.max_y = *d;
				break;
			}

		/* if we didn't find any additional regions, go until the bottom of the screen */
		if (d == displaylist_end)
			clip.max_y = Machine->drv->screen_height - 1;
		
		/* set the start and end points */
		if (modesc->reverse)
		{
			first = d - 5;
			last = base - 5;
			step = -5;
		}
		else
		{
			first = base;
			last = d;
			step = 5;
		}

		/* update the base */
		base = d;

		/* render the mos */
		for (d = first; d != last; d += step)
			(*morender)(bitmap, &clip, &d[1], param);
	}
}



/*************************************
 *
 *		Palette remapping
 *
 *************************************/

void atarigen_init_remap (int _colortype, int reuse)
{
	colortype = _colortype;
	reuse_colors = reuse;
	
	/* allocate arrays */
	last_colortable = malloc (Machine->drv->color_table_len * sizeof (unsigned short));
	memset (last_colortable, 0xff, Machine->drv->color_table_len * sizeof (unsigned short));
	this_colortable = malloc (Machine->drv->color_table_len * sizeof (unsigned short));
	memset (this_colortable, 0xff, Machine->drv->color_table_len * sizeof (unsigned short));

	/* initialize the palettes */
	memset (this_used, 0, sizeof (this_used));
	memset (this_color, 0xff, sizeof (this_color));

	/* reset the ranout count */
	ranout = 0;
	last_intensity = 0;
	current_black = 0;
}


inline int is_black (int color)
{
	if (colortype == COLOR_PALETTE_4444)
		return ((color & 0xf000) == 0 || (color & 0x0fff) == 0);
	else
		return ((color & 0x7fff) == 0);
}


void atarigen_alloc_fixed_colors (int *usage, int base, int colors, int palettes)
{
	unsigned short *pram = (unsigned short *)atarigen_paletteram + base, *temp;
	int i, j, index;

	/* see if we ran out of colors last frame */
	if (ranout && errorlog)
		fprintf (errorlog, "Fell short %d colors\n", ranout);
	ranout = 0;
	
	/* initialize the colortables */
	temp = last_colortable;
	last_colortable = this_colortable;
	this_colortable = temp;
	memset (this_colortable, 0xff, Machine->drv->color_table_len * sizeof (unsigned short));

	/* for each palette that is used by the playfield, assign it the associated bank from the palette */
	for (i = index = 0; i < palettes; i++)
	{
		int used = usage[i];

		/* do the same thing for each color in the given palette */
		for (j = 0; j < colors; j++, index++)
		{
			last_used[index] = this_used[index];
			last_color[index] = this_color[index];

			/* if this palette is in use, assign the colors */
			if (used)
			{
				this_used[index] = 1;
				this_color[index] = pram[index];
				this_colortable[base + index] = index;
			}

			/* else mark them free */
			else
				this_used[index] = 0;
		}
	}

	/* find a black color to use for anything requesting black */
	if (this_used[current_black])
	{
		for (i = 255; i >= 0; i--)
			if (!this_used[i])
			{
				current_black = i;
				break;
			}
	}
	this_used[current_black] = 1;
	this_color[current_black] = 0;

	/* find the special color */
	if (this_used[atarigen_special_color])
	{
		for (i = 255; i >= 0; i--)
			if (!this_used[i])
			{
				atarigen_special_color = i;
				break;
			}
	}
	this_used[atarigen_special_color] = 1;
}


static int atarigen_find_closest (int color)
{
	/* find the closest color match -- hopefully this won't be necessary 99% of the time */
	
	/* 555 case */
	if (colortype == COLOR_PALETTE_555)
	{
		int mask = 0x7fff, i, j;

		/* this is cheesy, but it should do the trick */
		for (i = 0; i < 5; i++)
		{
			mask ^= 0x421 << i;
			color &= mask;
			for (j = 0; j < 256; j++)
				if ((this_color[j] & mask) == color)
					return j;
		}
	}

	/* 4444 case */
	else if (colortype == COLOR_PALETTE_4444)
	{
		int mask = 0xffff, i, j;

		/* this is cheesy, but it should do the trick */
		for (i = 0; i < 4; i++)
		{
			mask ^= 0x1111 << i;
			color &= mask;
			for (j = 0; j < 256; j++)
				if ((this_color[j] & mask) == color)
					return j;
		}
	}
	
	return (rand () & 255);
}


void atarigen_alloc_dynamic_colors (int entry, int number)
{
	unsigned short *pram = (unsigned short *)atarigen_paletteram + entry;
	unsigned short *ctable = Machine->colortable + entry;
	int i;

	/* loop over each requested color, finding the best match */
	for (i = 0; i < number; i++, entry++)
	{
		int j, color = pram[i];

		/* assign black to the current winner */
		if (is_black (color))
		{
			j = current_black;
			goto assignit;
		}

		/* first see if we had an entry last frame and try to reuse that */
		j = last_colortable[entry];
		if (j != 0xffff)
		{
			if (!this_used[j])
				goto assignit;
			if (this_color[j] == color)
				goto assignit;
		}

		/* next see if there's an exact color match */
		/* WARNING: this can cause flickering; use only if necessary */
		if (reuse_colors)
		{
			for (j = 0; j < 256; j++)
				if (this_color[j] == color)
					goto assignit;
		}

		/* then try to use an empty entry from last frame */
		for (j = 0; j < 256; j++)
			if (!last_used[j] && !this_used[j])
				goto assignit;

		/* then try to use an empty entry from this frame */
		for (j = 0; j < 256; j++)
			if (!this_used[j])
				goto assignit;

		/* finally, find the closest */
		ranout++;
		j = atarigen_find_closest (color);
		ctable[i] = Machine->pens[j];
		continue;

assignit:
		/* now assign things */
		if (!this_used[j])
		{
			this_used[j] = 1;
			this_color[j] = color;
		}
		this_colortable[entry] = j;
		ctable[i] = Machine->pens[j];
	}
}


void atarigen_update_colors (int intensity)
{
	int j, idirty = 0;

	/* see if the intensity has changed */
	if (last_intensity != intensity)
		idirty = 1;
	last_intensity = intensity;

	/* handle the 555 case */
	if (colortype == COLOR_PALETTE_555)
	{
		/* loop over everyone */
		for (j = 0; j < 256; j++)
		{
			int tcolor = this_color[j];

			/* only update the color if it's different than last frame's */
			if (tcolor != last_color[j] || idirty)
			{
				int red =   (((tcolor >> 10) & 31) * 224) >> 5;
				int green = (((tcolor >>  5) & 31) * 224) >> 5;
				int blue =  (((tcolor      ) & 31) * 224) >> 5;

				if (red) red += 38;
				if (green) green += 38;
				if (blue) blue += 38;
				
				/* adjust for intensity */
				if (!(tcolor & 0x8000) && intensity >= 0)
				{
					red = (red * intensity) >> 5;
					green = (green * intensity) >> 5;
					blue = (blue * intensity) >> 5;
				}

				palette_change_color (j, red, green, blue);
			}
		}
	}

	/* handle the 4444 case */
	else if (colortype == COLOR_PALETTE_4444)
	{
		int j;

		/* loop over everyone */
		for (j = 0; j < 256; j++)
		{
			int tcolor = this_color[j];

			/* only update the color if it's different than last frame's */
			if (tcolor != last_color[j])
			{
				static const int ztable[16] =
					{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };
				int inten = ztable[(tcolor >> 12) & 15];
				int red =   ((tcolor >> 8) & 15) * inten;
				int green = ((tcolor >> 4) & 15) * inten;
				int blue =  ((tcolor     ) & 15) * inten;

				palette_change_color (j, red, green, blue);
			}
		}
	}
}
