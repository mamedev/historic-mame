/***************************************************************************

  atarigen.c

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/


#include "driver.h"
#include "atarigen.h"
#include "cpu/m6502/m6502.h"




/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarigen_playfieldram;
unsigned char *atarigen_playfield2ram;
unsigned char *atarigen_playfieldram_color;
unsigned char *atarigen_playfield2ram_color;
unsigned char *atarigen_spriteram;
unsigned char *atarigen_alpharam;
unsigned char *atarigen_vscroll;
unsigned char *atarigen_hscroll;

int atarigen_playfieldram_size;
int atarigen_playfield2ram_size;
int atarigen_spriteram_size;
int atarigen_alpharam_size;



/*--------------------------------------------------------------------------

	EEPROM I/O

		atarigen_eeprom_default - pointer to compressed default data
		atarigen_eeprom - pointer to base of EEPROM memory
		atarigen_eeprom_size - size of EEPROM memory

		atarigen_eeprom_reset - resets the EEPROM system

		atarigen_eeprom_enable_w - write handler to enable EEPROM access
		atarigen_eeprom_w - write handler for EEPROM data (low byte)
		atarigen_eeprom_r - read handler for EEPROM data (low byte)

		atarigen_hiload - standard hi score load routine; loads EEPROM data
		atarigen_hisave - standard hi score save routine; saves EEPROM data

--------------------------------------------------------------------------*/

/* globals */
const unsigned short *atarigen_eeprom_default;
unsigned char *atarigen_eeprom;
int atarigen_eeprom_size;

/* statics */
static int unlocked;

/* prototypes */
static void decompress_eeprom_word(const unsigned short *data);
static void decompress_eeprom_byte(const unsigned short *data);


/*
 *	EEPROM reset
 *
 *	Makes sure that the unlocked state is cleared when we reset.
 *
 */

void atarigen_eeprom_reset(void)
{
	unlocked = 0;
}


/*
 *	EEPROM enable write handler
 *
 *	Any write to this handler will allow one byte to be written to the
 *	EEPROM data area the next time.
 *
 */

void atarigen_eeprom_enable_w(int offset, int data)
{
	unlocked = 1;
}


/*
 *	EEPROM write handler (low byte of word)
 *
 *	Writes a "word" to the EEPROM, which is almost always accessed via
 *	the low byte of the word only. If the EEPROM hasn't been unlocked,
 *	the write attempt is ignored.
 *
 */

void atarigen_eeprom_w(int offset, int data)
{
	if (!unlocked)
		return;

	COMBINE_WORD_MEM(&atarigen_eeprom[offset], data);
	unlocked = 0;
}


/*
 *	EEPROM read handler (low byte of word)
 *
 *	Reads a "word" from the EEPROM, which is almost always accessed via
 *	the low byte of the word only.
 *
 */

int atarigen_eeprom_r(int offset)
{
	return READ_WORD(&atarigen_eeprom[offset]) | 0xff00;
}

int atarigen_eeprom_upper_r(int offset)
{
	return READ_WORD(&atarigen_eeprom[offset]) | 0x00ff;
}


/*
 *	Standard high score load
 *
 *	Loads the EEPROM data as a "high score".
 *
 */

int atarigen_hiload(void)
{
	void *f;

	/* first try to read from the file */
	f = osd_fopen(Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread(f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose(f);
	}

	/* if that doesn't work, initialize it */
	else
	{
		/* all 0xff's work for most games */
		memset(atarigen_eeprom, 0xff, atarigen_eeprom_size);

		/* anything else must be decompressed */
		if (atarigen_eeprom_default)
		{
			if (atarigen_eeprom_default[0] == 0)
				decompress_eeprom_byte(atarigen_eeprom_default + 1);
			else
				decompress_eeprom_word(atarigen_eeprom_default + 1);
		}
	}

	return 1;
}


/*
 *	Standard high score save
 *
 *	Saves the EEPROM data as a "high score".
 *
 */

void atarigen_hisave(void)
{
	void *f;

	f = osd_fopen(Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite(f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose(f);
	}
}


/*
 *	Decompress word-based EEPROM data
 *
 *	Used for decompressing EEPROM data that has every other byte invalid.
 *
 */

void decompress_eeprom_word(const unsigned short *data)
{
	unsigned short *dest = (unsigned short *)atarigen_eeprom;
	unsigned short value;

	while ((value = *data++) != 0)
	{
		int count = (value >> 8);
		value = (value << 8) | (value & 0xff);

		while (count--)
		{
			WRITE_WORD(dest, value);
			dest++;
		}
	}
}


/*
 *	Decompress byte-based EEPROM data
 *
 *	Used for decompressing EEPROM data that is byte-packed.
 *
 */

void decompress_eeprom_byte(const unsigned short *data)
{
	unsigned char *dest = (unsigned char *)atarigen_eeprom;
	unsigned short value;

	while ((value = *data++) != 0)
	{
		int count = (value >> 8);
		value = (value << 8) | (value & 0xff);

		while (count--)
			*dest++ = value;
	}
}



/*--------------------------------------------------------------------------

	Slapstic I/O

		atarigen_slapstic - pointer to base of slapstic memory

		atarigen_slapstic_init - selects which slapstic to emulate

		atarigen_slapstic_w - write handler for slapstic data
		atarigen_slapstic_r - read handler for slapstic data

--------------------------------------------------------------------------*/

/* externals */
void slapstic_init(int chip);
int slapstic_bank(void);
int slapstic_tweak(int offset);

/* globals */
int atarigen_slapstic_num;
unsigned char *atarigen_slapstic;


/*
 *	Slapstic initialization
 *
 *	Makes the selected slapstic number active and resets its state.
 *
 */

void atarigen_slapstic_reset(void)
{
	slapstic_init(atarigen_slapstic_num);
}


/*
 *	Slapstic write handler
 *
 *	Assuming that the slapstic sits in ROM memory space, we just simply
 *	tweak the slapstic at this address and do nothing more.
 *
 */

void atarigen_slapstic_w(int offset, int data)
{
	slapstic_tweak(offset / 2);
}


/*
 *	Slapstic read handler
 *
 *	Tweaks the slapstic at the appropriate address and then reads a
 *	word from the underlying memory.
 *
 */

int atarigen_slapstic_r(int offset)
{
	int bank = slapstic_tweak(offset / 2) * 0x2000;
	return READ_WORD(&atarigen_slapstic[bank + (offset & 0x1fff)]);
}




/*--------------------------------------------------------------------------

	Atari generic interrupt model

		atarigen_int_callback - called when the interrupt state changes
		atarigen_scanline_callback - called every 8 scanlines (optional)

		atarigen_interrupt_init - initializes the interrupt state
		atarigen_update_interrupts - forces the interrupts to be reevaluted

		atarigen_vblank_gen - standard VBLANK interrupt generator
		atarigen_vblank_ack_w - standard VBLANK interrupt acknowledgement

		atarigen_6502_irq_gen - standard 6502 IRQ interrupt generator
		atarigen_6502_irq_ack_r - standard 6502 IRQ interrupt acknowledgement
		atarigen_6502_irq_ack_w - standard 6502 IRQ interrupt acknowledgement

		atarigen_sound_w - Main CPU -> sound CPU data write (low byte)
		atarigen_sound_r - Sound CPU -> main CPU data read (low byte)
		atarigen_sound_upper_w - Main CPU -> sound CPU data write (high byte)
		atarigen_sound_upper_r - Sound CPU -> main CPU data read (high byte)

		atarigen_sound_reset_w - 6502 CPU reset
		atarigen_6502_sound_w - Sound CPU -> main CPU data write
		atarigen_6502_sound_r - Main CPU -> sound CPU data read

--------------------------------------------------------------------------*/

/* constants */
#define SOUND_INTERLEAVE_RATE		TIME_IN_USEC(50)
#define SOUND_INTERLEAVE_REPEAT		20

/* globals */
int atarigen_cpu_to_sound_ready;
int atarigen_sound_to_cpu_ready;

/* statics */
static atarigen_int_callback int_callback;
static atarigen_scanline_callback scanline_callback;

static int main_vblank_state;
static int atarigen_cpu_to_sound;
static int atarigen_sound_to_cpu;

static int last_scanline;

/* prototypes */
static void scanline_timer(int scanline);
static void sound_comm_timer(int reps_left);
static void delayed_sound_reset(int param);
static void delayed_sound_w(int param);
static void delayed_6502_sound_w(int param);

/* macros */
#define update_main_irq_states() (*int_callback)(main_vblank_state, atarigen_sound_to_cpu_ready)


/*
 *	Interrupt initialization
 *
 *	Resets the sound I/O system and the various interrupt states.
 *
 */

void atarigen_interrupt_init(atarigen_int_callback update_int, atarigen_scanline_callback update_graphics)
{
	/* reset the sound I/O states */
	atarigen_cpu_to_sound = atarigen_sound_to_cpu = 0;
	atarigen_cpu_to_sound_ready = atarigen_sound_to_cpu_ready = 0;

	/* reset the interrupts */
	main_vblank_state = 0;
	int_callback = update_int;
	scanline_callback = update_graphics;

	/* compute the last scanline */
	last_scanline = (int)(TIME_IN_HZ(Machine->drv->frames_per_second) / cpu_getscanlineperiod());
}


/*
 *	Update interrupts
 *
 *	Forces the interrupt callback to be called with the current VBLANK and sound interrupt states.
 *
 */

void atarigen_update_interrupts(void)
{
	update_main_irq_states();
}


/*
 *	VBLANK generator
 *
 *	Standard interrupt routine which sets the VBLANK state. Also sets the timer for
 *	the scanline interrupt, if requested.
 *
 */

int atarigen_vblank_gen(void)
{
	/* first update the VBLANK state */
	main_vblank_state = 1;
	update_main_irq_states();

	/* then prepare for the scanline interrupts */
	timer_set(TIME_IN_USEC(Machine->drv->vblank_duration), 0, scanline_timer);

	return 0;
}


/*
 *	VBLANK acknowledge write handler
 *
 *	Resets the state of the VBLANK interrupt.
 *
 */

void atarigen_vblank_ack_w(int offset, int data)
{
	main_vblank_state = 0;
	update_main_irq_states();
}


/*
 *	6502 IRQ generator
 *
 *	Generates an IRQ signal to the 6502 sound processor.
 *
 */

int atarigen_6502_irq_gen(void)
{
	cpu_set_irq_line(1, M6502_INT_IRQ, ASSERT_LINE);
	return 0;
}


/*
 *	6502 IRQ acknowledgement
 *
 *	Resets the IRQ signal to the 6502 sound processor. Both reads and writes can be used.
 *
 */

int atarigen_6502_irq_ack_r(int offset)
{
	cpu_set_irq_line(1, M6502_INT_IRQ, CLEAR_LINE);
	return 0;
}

void atarigen_6502_irq_ack_w(int offset, int data)
{
	cpu_set_irq_line(1, M6502_INT_IRQ, CLEAR_LINE);
}


/*
 *	Sound CPU write handler
 *
 *	Write handler which resets the sound CPU in response.
 *
 */

void atarigen_sound_reset_w(int offset, int data)
{
	timer_set(TIME_NOW, 0, delayed_sound_reset);
}


/*
 *	Main -> sound CPU data write handlers
 *
 *	Handles communication from the main CPU to the sound CPU. Two versions are provided,
 *	one with the data byte in the low 8 bits, and one with the data byte in the upper 8
 *	bits.
 *
 */

void atarigen_sound_w(int offset, int data)
{
	if (!(data & 0x00ff0000))
		timer_set(TIME_NOW, data & 0xff, delayed_sound_w);
}

void atarigen_sound_upper_w(int offset, int data)
{
	if (!(data & 0xff000000))
		timer_set(TIME_NOW, (data >> 8) & 0xff, delayed_sound_w);
}


/*
 *	Sound -> main CPU data read handlers
 *
 *	Handles reading data communicated from the sound CPU to the main CPU. Two versions
 *	are provided, one with the data byte in the low 8 bits, and one with the data byte
 *	in the upper 8 bits.
 *
 */

int atarigen_sound_r(int offset)
{
	atarigen_sound_to_cpu_ready = 0;
	update_main_irq_states();
	return atarigen_sound_to_cpu | 0xff00;
}

int atarigen_sound_upper_r(int offset)
{
	atarigen_sound_to_cpu_ready = 0;
	update_main_irq_states();
	return (atarigen_sound_to_cpu << 8) | 0x00ff;
}


/*
 *	Sound -> main CPU data write handler
 *
 *	Handles communication from the sound CPU to the main CPU.
 *
 */

void atarigen_6502_sound_w(int offset, int data)
{
	timer_set(TIME_NOW, data, delayed_6502_sound_w);
}


/*
 *	Main -> sound CPU data read handler
 *
 *	Handles reading data communicated from the main CPU to the sound CPU.
 *
 */

int atarigen_6502_sound_r(int offset)
{
	atarigen_cpu_to_sound_ready = 0;
	cpu_set_nmi_line(1, CLEAR_LINE);
	return atarigen_cpu_to_sound;
}


/*
 *	Scanline timer callback
 *
 *	Called once every 8 scanlines to generate the periodic callback to the main system.
 *
 */

static void scanline_timer(int scanline)
{
	/* if this is scanline 0, we reset the MO and playfield system */
	if (scanline == 0)
	{
		atarigen_mo_reset();
		atarigen_pf_reset();
	}

	/* callback */
	if (scanline_callback)
	{
		(*scanline_callback)(scanline);

		/* generate another? */
		scanline += 8;
		if (scanline < last_scanline)
			timer_set(cpu_getscanlineperiod() * 8.0, scanline, scanline_timer);
	}
}


/*
 *	Sound communications timer
 *
 *	Set whenever a command is written from the main CPU to the sound CPU, in order to
 *	temporarily bump up the interleave rate. This helps ensure that communications
 *	between the two CPUs works properly.
 *
 */

static void sound_comm_timer(int reps_left)
{
	if (--reps_left)
		timer_set(SOUND_INTERLEAVE_RATE, reps_left, sound_comm_timer);
}


/*
 *	Sound CPU reset timer
 *
 *	Synchronizes the sound reset command between the two CPUs.
 *
 */

static void delayed_sound_reset(int param)
{
	/* unhalt and reset the sound CPU */
	cpu_halt(1, 1);
	cpu_reset(1);

	/* reset the sound write state */
	atarigen_sound_to_cpu_ready = 0;
	update_main_irq_states();
}


/*
 *	Main -> sound data write timer
 *
 *	Synchronizes a data write from the main CPU to the sound CPU.
 *
 */

static void delayed_sound_w(int param)
{
	/* warn if we missed something */
	if (atarigen_cpu_to_sound_ready)
		if (errorlog) fprintf(errorlog, "Missed command from 68010\n");

	/* set up the states and signal an NMI to the sound CPU */
	atarigen_cpu_to_sound = param;
	atarigen_cpu_to_sound_ready = 1;
	cpu_set_nmi_line(1, ASSERT_LINE);

	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, sound_comm_timer);
}


/*
 *	Sound -> main data write timer
 *
 *	Synchronizes a data write from the sound CPU to the main CPU.
 *
 */

static void delayed_6502_sound_w(int param)
{
	/* warn if we missed something */
	if (atarigen_sound_to_cpu_ready)
		if (errorlog) fprintf(errorlog, "Missed result from 6502\n");

	/* set up the states and signal the sound interrupt to the main CPU */
	atarigen_sound_to_cpu = param;
	atarigen_sound_to_cpu_ready = 1;
	update_main_irq_states();
}





/*--------------------------------------------------------------------------

	Motion object rendering

		atarigen_mo_desc - description of the M.O. layout

		atarigen_mo_callback - called back for each M.O. during processing

		atarigen_mo_init - initializes and configures the M.O. list walker
		atarigen_mo_free - frees all memory allocated by atarigen_mo_init
		atarigen_mo_reset - reset for a new frame (use only if not using interrupt system)
		atarigen_mo_update - updates the M.O. list for the given scanline
		atarigen_mo_process - processes the current list

--------------------------------------------------------------------------*/

/* statics */
static struct atarigen_mo_desc modesc;

static unsigned short *molist;
static unsigned short *molist_end;
static unsigned short *molist_last;
static unsigned short *molist_upper_bound;


/*
 *	Motion object render initialization
 *
 *	Allocates memory for the motion object display cache.
 *
 */

int atarigen_mo_init(const struct atarigen_mo_desc *source_desc)
{
	modesc = *source_desc;
	if (modesc.entrywords == 0) modesc.entrywords = 4;
	modesc.entrywords++;

	/* make sure everything is free */
	atarigen_mo_free();

	/* allocate memory for the cached list */
	molist = malloc(modesc.maxcount * 2 * modesc.entrywords * (Machine->drv->screen_height / 8));
	if (!molist)
		return 1;
	molist_upper_bound = molist + (modesc.maxcount * modesc.entrywords * (Machine->drv->screen_height / 8));

	/* initialize the end/last pointers */
	atarigen_mo_reset();

	return 0;
}


/*
 *	Motion object render free
 *
 *	Frees all data allocated for the motion objects.
 *
 */

void atarigen_mo_free(void)
{
	if (molist)
		free(molist);
	molist = NULL;
}


/*
 *	Motion object render reset
 *
 *	Resets the motion object system for a new frame. Note that this is automatically called
 *	if you're using the interrupt system.
 *
 */

void atarigen_mo_reset(void)
{
	molist_end = molist;
	molist_last = NULL;
}


/*
 *	Motion object updater
 *
 *	Parses the current motion object list, caching all entries.
 *
 */

void atarigen_mo_update(const unsigned char *base, int link, int scanline)
{
	int entryskip = modesc.entryskip, wordskip = modesc.wordskip, wordcount = modesc.entrywords - 1;
	unsigned char spritevisit[ATARIGEN_MAX_MAXCOUNT];
	unsigned short *data, *data_start, *prev_data;
	int match = 0;

	/* set up local pointers */
	data_start = data = molist_end;
	prev_data = molist_last;

	/* if the last list entries were on the same scanline, overwrite them */
	if (prev_data)
	{
		if (*prev_data == scanline)
			data_start = data = prev_data;
		else
			match = 1;
	}

	/* visit all the sprites and copy their data into the display list */
	memset(spritevisit, 0, modesc.linkmask + 1);
	while (!spritevisit[link])
	{
		const unsigned char *modata = &base[link * entryskip];
		unsigned short tempdata[16];
		int temp, i;

		/* bounds checking */
		if (data >= molist_upper_bound)
		{
			if (errorlog) fprintf(errorlog, "Motion object list exceeded maximum\n");
			break;
		}

		/* start with the scanline */
		*data++ = scanline;

		/* add the data words */
		for (i = temp = 0; i < wordcount; i++, temp += wordskip)
			tempdata[i] = *data++ = READ_WORD(&modata[temp]);

		/* is this one to ignore? (note that ignore is predecremented by 4) */
		if (tempdata[modesc.ignoreword] == 0xffff)
			data -= wordcount + 1;

		/* update our match status */
		else if (match)
		{
			prev_data++;
			for (i = 0; i < wordcount; i++)
				if (*prev_data++ != tempdata[i])
				{
					match = 0;
					break;
				}
		}

		/* link to the next object */
		spritevisit[link] = 1;
		if (modesc.linkword >= 0)
			link = (tempdata[modesc.linkword] >> modesc.linkshift) & modesc.linkmask;
		else
			link = (link + 1) & modesc.linkmask;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		molist_end = data;
		molist_last = data_start;
	}
}


/*
 *	Motion object processor
 *
 *	Processes the cached motion object entries.
 *
 */

void atarigen_mo_process(atarigen_mo_callback callback, void *param)
{
	unsigned short *base = molist;
	int last_start_scan = -1;
	struct rectangle clip;

	/* create a clipping rectangle so that only partial sections are updated at a time */
	clip.min_x = 0;
	clip.max_x = Machine->drv->screen_width - 1;

	/* loop over the list until the end */
	while (base < molist_end)
	{
		unsigned short *data, *first, *last;
		int start_scan = base[0], step;

		last_start_scan = start_scan;
		clip.min_y = start_scan;

		/* look for an entry whose scanline start is different from ours; that's our bottom */
		for (data = base; data < molist_end; data += modesc.entrywords)
			if (*data != start_scan)
			{
				clip.max_y = *data;
				break;
			}

		/* if we didn't find any additional regions, go until the bottom of the screen */
		if (data == molist_end)
			clip.max_y = Machine->drv->screen_height - 1;

		/* set the start and end points */
		if (modesc.reverse)
		{
			first = data - modesc.entrywords;
			last = base - modesc.entrywords;
			step = -modesc.entrywords;
		}
		else
		{
			first = base;
			last = data;
			step = modesc.entrywords;
		}

		/* update the base */
		base = data;

		/* render the mos */
		for (data = first; data != last; data += step)
			(*callback)(&data[1], &clip, param);
	}
}



/*--------------------------------------------------------------------------

	Playfield rendering

		atarigen_pf_state - data block describing the playfield

		atarigen_pf_callback - called back for each chunk during processing

		atarigen_pf_init - initializes and configures the playfield state
		atarigen_pf_free - frees all memory allocated by atarigen_pf_init
		atarigen_pf_reset - reset for a new frame (use only if not using interrupt system)
		atarigen_pf_update - updates the playfield state for the given scanline
		atarigen_pf_process - processes the current list of parameters

		atarigen_pf2_init - same as above but for a second playfield
		atarigen_pf2_free - same as above but for a second playfield
		atarigen_pf2_reset - same as above but for a second playfield
		atarigen_pf2_update - same as above but for a second playfield
		atarigen_pf2_process - same as above but for a second playfield

--------------------------------------------------------------------------*/

/* types */
struct playfield_data
{
	struct osd_bitmap *bitmap;
	unsigned char *dirty;
	unsigned char *visit;

	int tilewidth;
	int tileheight;
	int tilewidth_shift;
	int tileheight_shift;
	int xtiles_mask;
	int ytiles_mask;

	int entries;
	int *scanline;
	struct atarigen_pf_state *state;
	struct atarigen_pf_state *last_state;
};

/* globals */
struct osd_bitmap *atarigen_pf_bitmap;
unsigned char *atarigen_pf_dirty;
unsigned char *atarigen_pf_visit;

struct osd_bitmap *atarigen_pf2_bitmap;
unsigned char *atarigen_pf2_dirty;
unsigned char *atarigen_pf2_visit;

struct osd_bitmap *atarigen_pf_overrender_bitmap;
unsigned short atarigen_overrender_colortable[32];

/* statics */
static struct playfield_data playfield;
static struct playfield_data playfield2;

/* prototypes */
static int internal_pf_init(struct playfield_data *pf, const struct atarigen_pf_desc *source_desc);
static void internal_pf_free(struct playfield_data *pf);
INLINE void internal_pf_reset(struct playfield_data *pf);
INLINE void internal_pf_update(struct playfield_data *pf, const struct atarigen_pf_state *state, int scanline);
static void internal_pf_process(struct playfield_data *pf, atarigen_pf_callback callback, void *param, const struct rectangle *clip);
static int compute_shift(int size);
static int compute_mask(int count);


/*
 *	Playfield render initialization
 *
 *	Allocates memory for the playfield and initializes all structures.
 *
 */

static int internal_pf_init(struct playfield_data *pf, const struct atarigen_pf_desc *source_desc)
{
	/* allocate the bitmap */
	pf->bitmap = osd_new_bitmap(source_desc->tilewidth * source_desc->xtiles,
								source_desc->tileheight * source_desc->ytiles,
								Machine->scrbitmap->depth);
	if (!pf->bitmap)
		return 1;

	/* allocate the dirty tile map */
	pf->dirty = malloc(source_desc->xtiles * source_desc->ytiles);
	if (!pf->dirty)
	{
		internal_pf_free(pf);
		return 1;
	}
	memset(pf->dirty, 0xff, source_desc->xtiles * source_desc->ytiles);

	/* allocate the visitation map */
	pf->visit = malloc(source_desc->xtiles * source_desc->ytiles);
	if (!pf->visit)
	{
		internal_pf_free(pf);
		return 1;
	}

	/* allocate the list of scanlines */
	pf->scanline = malloc(source_desc->ytiles * source_desc->tileheight * sizeof(int));
	if (!pf->scanline)
	{
		internal_pf_free(pf);
		return 1;
	}

	/* allocate the list of parameters */
	pf->state = malloc(source_desc->ytiles * source_desc->tileheight * sizeof(struct atarigen_pf_state));
	if (!pf->state)
	{
		internal_pf_free(pf);
		return 1;
	}

	/* copy the basic data */
	pf->tilewidth = source_desc->tilewidth;
	pf->tileheight = source_desc->tileheight;
	pf->tilewidth_shift = compute_shift(source_desc->tilewidth);
	pf->tileheight_shift = compute_shift(source_desc->tileheight);
	pf->xtiles_mask = compute_mask(source_desc->xtiles);
	pf->ytiles_mask = compute_mask(source_desc->ytiles);

	/* initialize the last state to all zero */
	pf->last_state = pf->state;
	memset(pf->last_state, 0, sizeof(*pf->last_state));

	/* reset */
	internal_pf_reset(pf);

	return 0;
}

int atarigen_pf_init(const struct atarigen_pf_desc *source_desc)
{
	int result = internal_pf_init(&playfield, source_desc);
	if (!result)
	{
		/* allocate the overrender bitmap */
		atarigen_pf_overrender_bitmap = osd_new_bitmap(Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth);
		if (!atarigen_pf_overrender_bitmap)
		{
			internal_pf_free(&playfield);
			return 1;
		}

		atarigen_pf_bitmap = playfield.bitmap;
		atarigen_pf_dirty = playfield.dirty;
		atarigen_pf_visit = playfield.visit;
	}
	return result;
}

int atarigen_pf2_init(const struct atarigen_pf_desc *source_desc)
{
	int result = internal_pf_init(&playfield2, source_desc);
	if (!result)
	{
		atarigen_pf2_bitmap = playfield2.bitmap;
		atarigen_pf2_dirty = playfield2.dirty;
		atarigen_pf2_visit = playfield2.visit;
	}
	return result;
}


/*
 *	Playfield render free
 *
 *	Frees all memory allocated by the playfield system.
 *
 */

static void internal_pf_free(struct playfield_data *pf)
{
	if (pf->bitmap)
		osd_free_bitmap(pf->bitmap);
	pf->bitmap = NULL;

	if (pf->dirty)
		free(pf->dirty);
	pf->dirty = NULL;

	if (pf->visit)
		free(pf->visit);
	pf->visit = NULL;

	if (pf->scanline)
		free(pf->scanline);
	pf->scanline = NULL;

	if (pf->state)
		free(pf->state);
	pf->state = NULL;
}

void atarigen_pf_free(void)
{
	internal_pf_free(&playfield);

	/* free the overrender bitmap */
	if (atarigen_pf_overrender_bitmap)
		osd_free_bitmap(atarigen_pf_overrender_bitmap);
	atarigen_pf_overrender_bitmap = NULL;
}

void atarigen_pf2_free(void)
{
	internal_pf_free(&playfield2);
}


/*
 *	Playfield render reset
 *
 *	Resets the playfield system for a new frame. Note that this is automatically called
 *	if you're using the interrupt system.
 *
 */

INLINE void internal_pf_reset(struct playfield_data *pf)
{
	/* verify memory has been allocated -- we're called even if we're not used */
	if (pf->scanline && pf->state)
	{
		pf->entries = 0;
		internal_pf_update(pf, pf->last_state, 0);
	}
}

void atarigen_pf_reset(void)
{
	internal_pf_reset(&playfield);
}

void atarigen_pf2_reset(void)
{
	internal_pf_reset(&playfield2);
}


/*
 *	Playfield render update
 *
 *	Sets the parameters for a given scanline.
 *
 */

INLINE void internal_pf_update(struct playfield_data *pf, const struct atarigen_pf_state *state, int scanline)
{
	if (pf->entries > 0)
	{
		/* if the current scanline matches the previous one, just overwrite */
		if (pf->scanline[pf->entries - 1] == scanline)
			pf->entries--;

		/* if the current data matches the previous data, ignore it */
		else if (pf->last_state->hscroll == state->hscroll &&
				 pf->last_state->vscroll == state->vscroll &&
				 pf->last_state->param[0] == state->param[0] &&
				 pf->last_state->param[1] == state->param[1])
			return;
	}

	/* remember this entry as the last set of parameters */
	pf->last_state = &pf->state[pf->entries];

	/* copy in the data */
	pf->scanline[pf->entries] = scanline;
	pf->state[pf->entries++] = *state;

	/* set the final scanline to be huge -- it will be clipped during processing */
	pf->scanline[pf->entries] = 100000;
}

void atarigen_pf_update(const struct atarigen_pf_state *state, int scanline)
{
	internal_pf_update(&playfield, state, scanline);
}

void atarigen_pf2_update(const struct atarigen_pf_state *state, int scanline)
{
	internal_pf_update(&playfield2, state, scanline);
}


/*
 *	Playfield render process
 *
 *	Processes the playfield in chunks.
 *
 */

INLINE void internal_pf_process(struct playfield_data *pf, atarigen_pf_callback callback, void *param, const struct rectangle *clip)
{
	struct rectangle curclip;
	struct rectangle tiles;
	int y;

	/* preinitialization */
	curclip.min_x = clip->min_x;
	curclip.max_x = clip->max_x;

	/* loop over all entries */
	for (y = 0; y < pf->entries; y++)
	{
		struct atarigen_pf_state *current = &pf->state[y];

		/* determine the clip rect */
		curclip.min_y = pf->scanline[y];
		curclip.max_y = pf->scanline[y + 1] - 1;

		/* skip if we're clipped out */
		if (curclip.min_y > clip->max_y || curclip.max_y < clip->min_y)
			continue;

		/* clip the clipper */
		if (curclip.min_y < clip->min_y)
			curclip.min_y = clip->min_y;
		if (curclip.max_y > clip->max_y)
			curclip.max_y = clip->max_y;

		/* determine the tile rect */
		tiles.min_x = ((current->hscroll + curclip.min_x) >> pf->tilewidth_shift) & pf->xtiles_mask;
		tiles.max_x = ((current->hscroll + curclip.max_x + pf->tilewidth) >> pf->tilewidth_shift) & pf->xtiles_mask;
		tiles.min_y = ((current->vscroll + curclip.min_y) >> pf->tileheight_shift) & pf->ytiles_mask;
		tiles.max_y = ((current->vscroll + curclip.max_y + pf->tileheight) >> pf->tileheight_shift) & pf->ytiles_mask;

		/* call the callback */
		(*callback)(&curclip, &tiles, current, param);
	}
}

void atarigen_pf_process(atarigen_pf_callback callback, void *param, const struct rectangle *clip)
{
	internal_pf_process(&playfield, callback, param, clip);
}

void atarigen_pf2_process(atarigen_pf_callback callback, void *param, const struct rectangle *clip)
{
	internal_pf_process(&playfield2, callback, param, clip);
}


/*
 *	Shift value computer
 *
 *	Determines the log2(value).
 *
 */

static int compute_shift(int size)
{
	int i;

	/* loop until we shift to zero */
	for (i = 0; i < 32; i++)
		if (!(size >>= 1))
			break;
	return i;
}


/*
 *	Mask computer
 *
 *	Determines the best mask to use for the given value.
 *
 */

static int compute_mask(int count)
{
	int shift = compute_shift(count);

	/* simple case - count is an even power of 2 */
	if (count == (1 << shift))
		return count - 1;

	/* slightly less simple case - round up to the next power of 2 */
	else
		return (1 << (shift + 1)) - 1;
}





/*--------------------------------------------------------------------------

	Sound stuff

		atarigen_init_6502_speedup - installs 6502 speedup cheat handler
		atarigen_set_ym2151_vol - set the volume of the 2151 chip
		atarigen_set_ym2413_vol - set the volume of the 2151 chip
		atarigen_set_pokey_vol - set the volume of the POKEY chip(s)
		atarigen_set_tms5220_vol - set the volume of the 5220 chip
		atarigen_set_oki6295_vol - set the volume of the OKI6295

--------------------------------------------------------------------------*/

/* statics */
static unsigned char *speed_a, *speed_b;
static unsigned int speed_pc;

/* prototypes */
static int m6502_speedup_r(int offset);


/*
 *	6502 CPU speedup cheat installer
 *
 *	Installs a special read handler to catch the main spin loop in the
 *	6502 sound code. The addresses accessed seem to be the same across
 *	a large number of games, though the PC shifts.
 *
 */

void atarigen_init_6502_speedup(int cpunum, int compare_pc1, int compare_pc2)
{
	unsigned char *memory = Machine->memory_region[Machine->drv->cpu[cpunum].memory_region];
	int address_low, address_high;

	/* determine the pointer to the first speed check location */
	address_low = memory[compare_pc1 + 1] | (memory[compare_pc1 + 2] << 8);
	address_high = memory[compare_pc1 + 4] | (memory[compare_pc1 + 5] << 8);
	if (address_low != address_high - 1)
		if (errorlog) fprintf(errorlog, "Error: address %04X does not point to a speedup location!", compare_pc1);
	speed_a = &memory[address_low];

	/* determine the pointer to the second speed check location */
	address_low = memory[compare_pc2 + 1] | (memory[compare_pc2 + 2] << 8);
	address_high = memory[compare_pc2 + 4] | (memory[compare_pc2 + 5] << 8);
	if (address_low != address_high - 1)
		if (errorlog) fprintf(errorlog, "Error: address %04X does not point to a speedup location!", compare_pc2);
	speed_b = &memory[address_low];

	/* install a handler on the second address */
	speed_pc = compare_pc2;
	install_mem_read_handler(cpunum, address_low, address_low, m6502_speedup_r);
}


/*
 *	Set the YM2151 volume
 *
 *	What it says.
 *
 */

void atarigen_set_ym2151_vol(int volume)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, "2151"))
			mixer_set_volume(ch, volume);
	}
}


/*
 *	Set the YM2413 volume
 *
 *	What it says.
 *
 */

void atarigen_set_ym2413_vol(int volume)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, "2413"))
			mixer_set_volume(ch, volume);
	}
}


/*
 *	Set the POKEY volume
 *
 *	What it says.
 *
 */

void atarigen_set_pokey_vol(int volume)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, "POKEY"))
			mixer_set_volume(ch, volume);
	}
}


/*
 *	Set the TMS5220 volume
 *
 *	What it says.
 *
 */

void atarigen_set_tms5220_vol(int volume)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, "5220"))
			mixer_set_volume(ch, volume);
	}
}


/*
 *	Set the OKI6295 volume
 *
 *	What it says.
 *
 */

void atarigen_set_oki6295_vol(int volume)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, "6295"))
			mixer_set_volume(ch, volume);
	}
}


/*
 *	Generic 6502 CPU speedup handler
 *
 *	Special shading renderer that runs any pixels under pen 1 through a lookup table.
 *
 */

static int m6502_speedup_r(int offset)
{
	int result = speed_b[0];

	if (cpu_getpreviouspc() == speed_pc && speed_a[0] == speed_a[1] && result == speed_b[1])
		cpu_spinuntil_int();

	return result;
}



/*--------------------------------------------------------------------------

	Misc Video stuff
	
		atarigen_get_hblank - returns the current HBLANK state
		atarigen_halt_until_hblank_0_w - write handler for a HBLANK halt
		atarigen_666_paletteram_w - 6-6-6 special RGB paletteram handler
		atarigen_expanded_666_paletteram_w - byte version of above
		atarigen_shade_render - Vindicators shading renderer

--------------------------------------------------------------------------*/

/* prototypes */
static void unhalt_cpu(int param);


/*
 *	Compute HBLANK state
 *
 *	Returns a guesstimate about the current HBLANK state, based on the assumption that
 *	HBLANK represents 10% of the scanline period.
 *
 */

int atarigen_get_hblank(void)
{
	return (cpu_gethorzbeampos() > (Machine->drv->screen_width * 9 / 10));
}


/*
 *	Halt CPU 0 until HBLANK
 *
 *	What it says.
 *
 */

void atarigen_halt_until_hblank_0_w(int offset, int data)
{
	/* halt the CPU until the next HBLANK */
	int hpos = cpu_gethorzbeampos();
	int hblank = Machine->drv->screen_width * 9 / 10;
	double fraction;

	/* if we're in hblank, set up for the next one */
	if (hpos >= hblank)
		hblank += Machine->drv->screen_width;

	/* halt and set a timer to wake up */
	fraction = (double)(hblank - hpos) / (double)Machine->drv->screen_width;
	timer_set(cpu_getscanlineperiod() * fraction, 0, unhalt_cpu);
	cpu_halt(0, 0);
}


/*
 *	6-6-6 RGB palette RAM handler
 *
 *	What it says.
 *
 */

void atarigen_666_paletteram_w(int offset, int data)
{
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	WRITE_WORD(&paletteram[offset],newword);

	{
		int r, g, b;

		r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
		g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
		b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		palette_change_color(offset / 2, r, g, b);
	}
}


/*
 *	6-6-6 RGB exanded palette RAM handler
 *
 *	What it says.
 *
 */

void atarigen_expanded_666_paletteram_w(int offset, int data)
{
	COMBINE_WORD_MEM(&paletteram[offset], data);

	if (!(data & 0xff000000))
	{
		int palentry = offset / 4;
		int newword = (READ_WORD(&paletteram[palentry * 4]) & 0xff00) | (READ_WORD(&paletteram[palentry * 4 + 2]) >> 8);

		int r, g, b;

		r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
		g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
		b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		palette_change_color(palentry & 0x1ff, r, g, b);
	}
}


/*
 *	Vindicators shading renderer
 *
 *	Special shading renderer that runs any pixels under pen 1 through a lookup table.
 *
 */

void atarigen_shade_render(struct osd_bitmap *bitmap, const struct GfxElement *gfx, int code, int hflip, int x, int y, const struct rectangle *clip, const unsigned char *shade_table)
{
	unsigned char **base = (unsigned char **)&gfx->gfxdata->line[code * gfx->height];
	int width = gfx->width;
	int height = gfx->height;
	int i, j, diff, xoff = 0;

	/* apply X clipping */
	diff = clip->min_x - x;
	if (diff > 0)
		xoff += diff, x += diff, width -= diff;
	diff = x + width - clip->max_x;
	if (diff > 0)
		width -= diff;

	/* apply Y clipping */
	diff = clip->min_y - y;
	if (diff > 0)
		base += diff, y += diff, height -= diff;
	diff = y + height - clip->max_y;
	if (diff > 0)
		height -= diff;

	/* loop over the data */
	for (i = 0; i < height; i++, y++, base++)
	{
		const unsigned char *src = &(*base)[xoff];
		unsigned char *dst = &bitmap->line[y][x + xoff];

		if (hflip)
		{
			src += width;
			for (j = 0; j < width; j++, dst++)
				if (*--src == 1)
					*dst = shade_table[*dst];
		}
		else
		{
			for (j = 0; j < width; j++, dst++)
				if (*src++ == 1)
					*dst = shade_table[*dst];
		}
	}
}

/*
 *	CPU unhalter
 *
 *	Timer callback to release the CPU from a halted state.
 *
 */

static void unhalt_cpu(int param)
{
	cpu_halt(param, 1);
}



/*--------------------------------------------------------------------------

	General stuff
	
		atarigen_show_slapstic_message - display warning about slapstic
		atarigen_show_sound_message - display warning about coins
		atarigen_update_messages - update messages

--------------------------------------------------------------------------*/

/* statics */
static char *message_text[10];
static int message_countdown;

/*
 *	Display a warning message about slapstic protection
 *
 *	What it says.
 *
 */
 
void atarigen_show_slapstic_message(void)
{
	message_text[0] = "There are known problems with";
	message_text[1] = "later levels of this game due";
	message_text[2] = "to incomplete slapstic emulation.";
	message_text[3] = "You have been warned.";
	message_text[4] = NULL;
	message_countdown = 15 * Machine->drv->frames_per_second;
}


/*
 *	Display a warning message about sound being disabled
 *
 *	What it says.
 *
 */

void atarigen_show_sound_message(void)
{
	if (Machine->sample_rate == 0)
	{
		message_text[0] = "This game may have trouble accepting";
		message_text[1] = "coins, or may even behave strangely,";
		message_text[2] = "because you have disabled sound.";
		message_text[3] = NULL;
		message_countdown = 15 * Machine->drv->frames_per_second;
	}
}


/*
 *	Update on-screen messages
 *
 *	What it says.
 *
 */

void atarigen_update_messages(void)
{
	if (message_countdown && message_text[0])
	{
		int maxwidth = 0;
		int lines, x, y, i, j;
		
		/* first count lines and determine the maximum width */
		for (lines = 0; lines < 10; lines++)
		{
			if (!message_text[lines]) break;
			x = strlen(message_text[lines]);
			if (x > maxwidth) maxwidth = x;
		}
		maxwidth += 2;

		/* determine y offset */
		x = (Machine->uiwidth - Machine->uifontwidth * maxwidth) / 2;
		y = (Machine->uiheight - Machine->uifontheight * (lines + 2)) / 2;

		/* draw a row of spaces at the top and bottom */
		for (i = 0; i < maxwidth; i++)
		{
			ui_text(" ", x + i * Machine->uifontwidth, y);
			ui_text(" ", x + i * Machine->uifontwidth, y + (lines + 1) * Machine->uifontheight);
		}
		y += Machine->uifontheight;

		/* draw the message */
		for (i = 0; i < lines; i++)		
		{
			int width = strlen(message_text[i]) * Machine->uifontwidth;
			int dx = (Machine->uifontwidth * maxwidth - width) / 2;
			
			for (j = 0; j < dx; j += Machine->uifontwidth)
			{
				ui_text(" ", x + j, y);
				ui_text(" ", x + (maxwidth - 1) * Machine->uifontwidth - j, y);
			}
			
			ui_text(message_text[i], x + dx, y);
			y += Machine->uifontheight;
		}

		/* decrement the counter */
		message_countdown--;
		
		/* if a coin is inserted, make the message go away */
		if (osd_key_pressed(OSD_KEY_3) || osd_key_pressed(OSD_KEY_4))
			message_countdown = 0;
	}
	else
		message_text[0] = NULL;
}


