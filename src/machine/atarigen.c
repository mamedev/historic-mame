/***************************************************************************

  atarigen.c

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/


#include "atarigen.h"
#include "driver.h"
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
static int last_palette[256];
static int this_palette[256];
static int last_color[256];
static int this_color[256];
static int last_intensity;
static int current_black;

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
 *		Palette remapping
 *
 *************************************/

void atarigen_init_remap (int _colortype)
{
	colortype = _colortype;

	/* initialize the palettes */
	memset (this_palette, 0xff, sizeof (this_palette));
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
	unsigned short *pram = (unsigned short *)atarigen_paletteram + base;
	int i, j, index;

	/* see if we ran out of colors last frame */
	if (ranout && errorlog)
		fprintf (errorlog, "Fell short %d colors\n", ranout);
	ranout = 0;

	/* for each palette that is used by the playfield, assign it the associated bank from the palette */
	for (i = index = 0; i < palettes; i++)
	{
		int used = usage[i];

		/* do the same thing for each color in the given palette */
		for (j = 0; j < colors; j++, index++)
		{
			last_palette[index] = this_palette[index];
			last_color[index] = this_color[index];

			/* if this palette is in use, assign the colors */
			if (used)
			{
				this_palette[index] = base + index;
				this_color[index] = pram[index];
			}

			/* else mark them free */
			else
				this_palette[index] = -1;
		}
	}

	/* find a black color to use for anything requesting black */
	if (this_palette[current_black] >= 0)
	{
		for (i = 255; i >= 0; i--)
			if (this_palette[i] < 0)
			{
				current_black = i;
				break;
			}
	}
	this_palette[current_black] = 10001;
	this_color[current_black] = 0;

	/* find the special color */
	if (this_palette[atarigen_special_color] >= 0)
	{
		for (i = 255; i >= 0; i--)
			if (this_palette[i] < 0)
			{
				atarigen_special_color = i;
				break;
			}
	}
	this_palette[atarigen_special_color] = 10000;
}


static int atarigen_find_closest (int color)
{
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

		/* first see if there's an exact color match */
		/* WARNING: this can cause flickering; use only if necessary */
/*		for (j = 0; j < 256; j++)
			if (this_color[j] == color)
				goto assignit;*/

		/* then see if we had an entry last frame */
		for (j = 0; j < 256; j++)
			if (last_palette[j] == entry && this_palette[j] < 0)
				goto assignit;

		/* then try to use an empty entry from last frame */
		for (j = 0; j < 256; j++)
			if (last_palette[j] < 0 && this_palette[j] < 0)
				goto assignit;

		/* then try to use an empty entry from this frame */
		for (j = 0; j < 256; j++)
			if (this_palette[j] < 0)
				goto assignit;

		/* finally, find the closest */
		ranout++;
		j = atarigen_find_closest (color);

assignit:
		/* now assign things */
		if (this_palette[j] < 0)
		{
			this_palette[j] = entry;
			this_color[j] = color;
		}
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
				int red = ((tcolor >> 10) & 0x1f) << 3;
				int green = ((tcolor >> 5) & 0x1f) << 3;
				int blue = (tcolor & 0x1f) << 3;

				/* adjust for intensity */
				if (!(tcolor & 0x8000) && intensity >= 0)
				{
					red = (red * intensity) >> 5;
					green = (green * intensity) >> 5;
					blue = (blue * intensity) >> 5;
				}

				osd_modify_pen (Machine->pens[j], red, green, blue);
			}
		}
	}

	/* handle the 4444 case */
	else if (colortype == COLOR_PALETTE_4444)
	{
		static const int intensity_table[16] =
			{ 0x00, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x100, 0x110 };

		int j;

		/* loop over everyone */
		for (j = 0; j < 256; j++)
		{
			int tcolor = this_color[j];

			/* only update the color if it's different than last frame's */
			if (tcolor != last_color[j])
			{
				int inten = intensity_table[(tcolor >> 12) & 15];
				int red = (((tcolor >> 8) & 15) * inten) >> 4;
				int green = (((tcolor >> 4) & 15) * inten) >> 4;
				int blue = ((tcolor & 15) * inten) >> 4;

				osd_modify_pen (Machine->pens[j], red, green, blue);
			}
		}
	}
}
