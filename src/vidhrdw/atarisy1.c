/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define SYSTEM1_DEBUG 0


#define XCHARS 42
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)

#define ROADBLST_CLIP 200


/* these macros make accessing the indirection table easier, plus this is how the data
   is stored for the pfmapped array */
#define LDATA(bank,color,offset) ((((color) & 255) << 16) | (((bank) & 15) << 12) | (((offset) & 15) << 8))
#define LCOLOR(data) (((data) >> 16) & 0xff)
#define LBANK(data) (((data) >> 12) & 15)
#define LPICT(data) ((data) & 0x0fff)
#define LFLIP(data) ((data) & 0x01000000)
#define LDIRTYFLAG 0x02000000
#define LDIRTY(data) ((data) & LDIRTYFLAG)

/* these macros are for more conveniently defining groups of motion and playfield objects */
#define LDATA2(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1)
#define LDATA3(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1), LDATA(b,c,(o)+2)
#define LDATA4(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1), LDATA(b,c,(o)+2), LDATA(b,c,(o)+3)
#define LDATA5(b,c,o) LDATA4(b,c,(o)+0), LDATA(b,c,(o)+4)
#define LDATA6(b,c,o) LDATA4(b,c,(o)+0), LDATA2(b,c,(o)+4)
#define LDATA7(b,c,o) LDATA4(b,c,(o)+0), LDATA3(b,c,(o)+4)
#define LDATA8(b,c,o) LDATA4(b,c,(o)+0), LDATA4(b,c,(o)+4)
#define LDATA9(b,c,o) LDATA8(b,c,(o)+0), LDATA(b,c,(o)+8)
#define LDATA10(b,c,o) LDATA8(b,c,(o)+0), LDATA2(b,c,(o)+8)
#define LDATA11(b,c,o) LDATA8(b,c,(o)+0), LDATA3(b,c,(o)+8)
#define LDATA12(b,c,o) LDATA8(b,c,(o)+0), LDATA4(b,c,(o)+8)
#define LDATA13(b,c,o) LDATA8(b,c,(o)+0), LDATA5(b,c,(o)+8)
#define LDATA14(b,c,o) LDATA8(b,c,(o)+0), LDATA6(b,c,(o)+8)
#define LDATA15(b,c,o) LDATA8(b,c,(o)+0), LDATA7(b,c,(o)+8)
#define LDATA16(b,c,o) LDATA8(b,c,(o)+0), LDATA8(b,c,(o)+8)
#define EMPTY2 0,0
#define EMPTY5 0,0,0,0,0
#define EMPTY6 0,0,0,0,0,0
#define EMPTY8 0,0,0,0,0,0,0,0
#define EMPTY15 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#define EMPTY16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys1_playfieldram;
unsigned char *atarisys1_spriteram;
unsigned char *atarisys1_alpharam;
unsigned char *atarisys1_paletteram;
unsigned char *atarisys1_vscroll;
unsigned char *atarisys1_hscroll;
unsigned char *atarisys1_bankselect;
unsigned char *atarisys1_prioritycolor;

int atarisys1_playfieldram_size;
int atarisys1_spriteram_size;
int atarisys1_alpharam_size;
int atarisys1_paletteram_size;



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *colordirty;
static unsigned char *spritevisit;
static unsigned short *displaylist;
static unsigned int *scrolllist;

static unsigned short *displaylist_end;
static unsigned short *displaylist_last;
static int scrolllist_end;

static int *pflookup, *molookup;

static int *pfmapped;
static int pfbank;

static int playfield_count[32];
static int last_map[16], this_map[16];
static int sprite_map_shift, pf_map_shift;
static int hipri_color0;

static struct osd_bitmap *playfieldbitmap;
static struct osd_bitmap *tempbitmap;

static void *int3_timer[YDIM];
static void *int3off_timer;
static int int3_state;

static const int intensity_table[16] =
	{ 0x00, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x100, 0x110 };



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void atarisys1_vh_stop (void);
void atarisys1_sound_reset (void);
void atarisys1_update_display_list (int scanline);

static int atarisys1_debug (void);


/***************************************************************************

  System 1 doesn't have a color PROM. It uses 1024 words of RAM to
  dynamically create the palette. Each word defines one color (5 bits
  per pixel). However, more than 256 colors is rarely used, so the
  video system has a dynamic palette mapping function

***************************************************************************/


/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int atarisys1_vh_start(void)
{
	int i;

	/* allocate dirty buffers */
	if (!pfmapped)
		pfmapped = calloc (atarisys1_playfieldram_size / 2 * sizeof (int) +
		                   atarisys1_paletteram_size / 2 +
		                   atarisys1_spriteram_size / 8 / 8 +
		                   atarisys1_spriteram_size * sizeof (short) +
		                   YDIM * sizeof (int), 1);
	if (!pfmapped)
	{
		atarisys1_vh_stop ();
		return 1;
	}
	colordirty = (unsigned char *)(pfmapped + atarisys1_playfieldram_size / 2);
	spritevisit = (unsigned char *)(colordirty + atarisys1_paletteram_size / 2);
	displaylist = (unsigned short *)(spritevisit + atarisys1_spriteram_size / 8 / 8);
	scrolllist = (unsigned int *)(displaylist + atarisys1_spriteram_size);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		atarisys1_vh_stop ();
		return 1;
	}

	if (!tempbitmap)
		tempbitmap = osd_new_bitmap (Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth);
	if (!tempbitmap)
	{
		atarisys1_vh_stop ();
		return 1;
	}

	/* no motion object lists */
	displaylist_end = displaylist;
	displaylist_last = NULL;
	scrolllist_end = 0;

	/* reset the timers */
	memset (int3_timer, 0, sizeof (int3_timer));
	int3_state = 0;

	/* reset the color counter */
	memset (playfield_count, 0, sizeof (playfield_count));
	playfield_count[LCOLOR (pflookup[0])] = 64 * 64;

	/* initialize the pre-mapped playfield */
	for (i = atarisys1_playfieldram_size / 2; i >= 0; i--)
		pfmapped[i] = pflookup[0] | LDIRTYFLAG;

	return 0;
}



/*************************************
 *
 *		Game-specific video system start
 *
 *************************************/

int marble_vh_start(void)
{
	static int marble_pflookup[256] =
	{
		LDATA16 (1,0+8,0),
		LDATA16 (1,1+8,0),
		LDATA16 (1,2+8,0),
		LDATA16 (1,3+8,0),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	static int marble_molookup[256] =
	{
		LDATA8 (2,0,8),
		LDATA8 (2,2,8),
		LDATA8 (2,4,8),
		LDATA8 (2,6,8),
		LDATA8 (2,8,8),
		LDATA8 (2,10,8),
		LDATA8 (2,12,8),
		LDATA8 (2,14,8),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	pflookup = marble_pflookup;
	molookup = marble_molookup;
	sprite_map_shift = 1;
	pf_map_shift = 1;
	hipri_color0 = 1;
	return atarisys1_vh_start ();
}


int peterpak_vh_start(void)
{
	static int peterpak_pflookup[256] =
	{
		LDATA16 (1,0+16,0),
		LDATA16 (2,0+16,0),
		LDATA8 (3,0+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,1+16,0),
		LDATA16 (2,1+16,0),
		LDATA8 (3,1+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,2+16,0),
		LDATA16 (2,2+16,0),
		LDATA8 (3,2+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,3+16,0),
		LDATA16 (2,3+16,0),
		LDATA8 (3,3+16,8), EMPTY8,
		EMPTY16,
	};

	static int peterpak_molookup[256] =
	{
		LDATA16 (1,0,0),
		LDATA16 (2,0,0),
		LDATA8 (3,0,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,1,0),
		LDATA16 (2,1,0),
		LDATA8 (3,1,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,2,0),
		LDATA16 (2,2,0),
		LDATA8 (3,2,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,3,0),
		LDATA16 (2,3,0),
		LDATA8 (3,3,8), EMPTY8,
		EMPTY16,
	};

	pflookup = peterpak_pflookup;
	molookup = peterpak_molookup;
	sprite_map_shift = 0;
	pf_map_shift = 0;
	hipri_color0 = 1;
	return atarisys1_vh_start ();
}


int indytemp_vh_start(void)
{
	#define SEVEN 1
	#define SIX 2
	#define FIVE 3
	#define THREE 4

	static int indytemp_pflookup[256] =
	{
		LDATA (SEVEN,0+16,0), LDATA7 (SIX,3+16,8), LDATA8 (THREE,2+16,0),
		LDATA8 (THREE,2+16,8), LDATA8 (SIX,1+16,8),
		LDATA16 (FIVE,1+16,0),
		LDATA16 (THREE,1+16,0),
		LDATA8 (SIX,0+16,8), LDATA8 (FIVE,0+16,0),
		LDATA8 (FIVE,0+16,8), LDATA8 (THREE,0+16,0),
		LDATA8 (THREE,0+16,8), LDATA8 (SIX,2+16,8),
		LDATA16 (FIVE,2+16,0),
		LDATA (SEVEN,0+16,0), LDATA7 (SIX,3+16,8), LDATA8 (THREE,2+16,0),
		LDATA8 (THREE,2+16,8), LDATA8 (SIX,1+16,8),
		LDATA16 (FIVE,1+16,0),
		LDATA16 (THREE,1+16,0),
		LDATA8 (SIX,0+16,8), LDATA8 (FIVE,0+16,0),
		LDATA8 (FIVE,0+16,8), LDATA8 (THREE,0+16,0),
		LDATA8 (THREE,0+16,8), LDATA8 (SIX,2+16,8),
		LDATA16 (FIVE,2+16,0),
	};

	static int indytemp_molookup[256] =
	{
		LDATA16 (SEVEN,1,0),
		LDATA16 (SIX,1,0),
		LDATA16 (SEVEN,0,0),
		LDATA16 (SIX,0,0),
		LDATA16 (SEVEN,2,0),
		LDATA16 (SIX,2,0),
		LDATA16 (SEVEN,3,0),
		LDATA16 (SIX,3,0),
		LDATA16 (SEVEN,4,0),
		LDATA16 (SIX,4,0),
		LDATA16 (SEVEN,5,0),
		LDATA16 (SIX,5,0),
		LDATA16 (SEVEN,6,0),
		LDATA16 (SIX,6,0),
		LDATA16 (SEVEN,7,0),
		LDATA16 (SIX,7,0),
	};

	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN

	pflookup = indytemp_pflookup;
	molookup = indytemp_molookup;
	sprite_map_shift = 0;
	pf_map_shift = 0;
	hipri_color0 = 0;
	return atarisys1_vh_start ();
}


int roadrunn_vh_start(void)
{
	#define SEVEN 1	/* enable off */
	#define SIX 2
	#define FIVE 3
	#define THREE 4
	#define SEVENY 5	/* enable on */
	#define SEVENX 6  /* looks like 3bpp */

	static int roadrunn_pflookup[256] =
	{
		LDATA (SEVEN,0+16,0), LDATA (SEVEN,15+16,0), LDATA (SEVEN,14+16,0), LDATA (SEVEN,13+16,0),
			LDATA (SEVEN,12+16,0), LDATA (SEVEN,11+16,0), LDATA (SEVEN,10+16,0), LDATA (SEVEN,9+16,0),
			LDATA (SEVEN,8+16,0), LDATA (SEVEN,7+16,0), LDATA (SEVEN,6+16,0), LDATA (SEVEN,5+16,0),
			LDATA (SEVEN,4+16,0), LDATA (SEVEN,3+16,0), LDATA (SEVEN,2+16,0), LDATA (SEVEN,1+16,0),
		LDATA (SEVEN,0+16,0), LDATA9 (SIX,12+16,1), LDATA6 (SIX,1+16,10),
		LDATA16 (FIVE,1+16,0),
		LDATA13 (THREE,5+16,0), LDATA3 (THREE,2+16,13),
		LDATA16 (SEVENY,2+16,0),
		LDATA3 (SEVENX,2+16,0), LDATA7 (SEVENX,4+16,3), LDATA2 (SEVENX,3+16,10), LDATA (SEVENX,6+16,12),
			LDATA (SEVENX,7+16,13), LDATA2 (SEVENX,0+16,14),
		LDATA (FIVE,5+16,15), LDATA (FIVE,2+16,15), LDATA6 (THREE,2+16,7), LDATA (FIVE,4+16,15),
			LDATA2 (THREE,4+16,11), LDATA (SEVENX,4+16,2), LDATA (THREE,3+16,12), LDATA (SEVENX,3+16,9),
			LDATA (FIVE,6+16,15), LDATA (THREE,6+16,11),
		LDATA (THREE,6+16,12), LDATA (SEVENX,6+16,2), LDATA (SEVENX,6+16,9), LDATA (SEVENX,6+16,11),
			LDATA (FIVE,7+16,15), LDATA (THREE,7+16,12), LDATA (SEVENX,7+16,9), LDATA2 (SEVENX,7+16,11),
			LDATA (FIVE,0+16,15), LDATA (SEVENX,0+16,13), EMPTY5,
		LDATA16 (SEVEN,0+16,0),
		LDATA16 (SIX,0+16,0),
		LDATA16 (FIVE,0+16,0),
		LDATA16 (THREE,0+16,0),
		LDATA16 (SEVENY,0+16,0),
		LDATA16 (SEVENX,0+16,0),
		LDATA (SEVEN,1+16,15), LDATA14 (SIX,1+16,0),
		LDATA (SIX,1+16,15), LDATA14 (FIVE,1+16,0)
	};

	static int roadrunn_molookup[256] =
	{
		LDATA16 (SEVEN,0,0),
		LDATA16 (SIX,0,0),
		LDATA16 (SEVEN,1,0),
		LDATA16 (SIX,1,0),
		LDATA16 (SEVEN,2,0),
		LDATA16 (SIX,2,0),
		LDATA16 (SEVEN,3,0),
		LDATA16 (SIX,3,0),
		LDATA16 (SEVEN,4,0),
		LDATA16 (SIX,4,0),
		LDATA16 (SEVEN,5,0),
		LDATA16 (SIX,5,0),
		LDATA16 (SEVEN,6,0),
		LDATA16 (SIX,6,0),
		LDATA16 (SEVEN,7,0),
		LDATA16 (SIX,7,0)
	};

	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN
	#undef SEVENX
	#undef SEVENY

	pflookup = roadrunn_pflookup;
	molookup = roadrunn_molookup;
	sprite_map_shift = 0;
	pf_map_shift = 0;
	hipri_color0 = 0;
	return atarisys1_vh_start ();
}


int roadblst_vh_start(void)
{
	#define SEVEN6 1
	#define SEVEN 2
	#define SIX 3
	#define FIVE 4
	#define THREE 5
	#define SEVENMO 6
	#define SEVENMO0 7  /* looks like 3bpp, with a 0 in the high bit of the 2nd byte */
	#define SEVENMO1 8  /* looks like 3bpp, with a 1 in the high bit of the 2nd byte */

	static int roadblst_pflookup[256] =
	{
		LDATA2 (SEVEN6,2+4,0), LDATA14 (SEVEN,1+16,2),
		LDATA2 (SEVEN6,3+4,0), LDATA14 (SEVEN,2+16,2),
		LDATA2 (SEVEN6,1+4,0), LDATA14 (SEVEN,0+16,2),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	static int roadblst_molookup[256] =
	{
		LDATA16 (SIX,0,0),
		LDATA16 (FIVE,0,0),
		LDATA16 (THREE,0,0),
		LDATA16 (SEVENMO,0,0),
		LDATA16 (SEVENMO0,0,0),
		LDATA16 (SEVENMO1,0,0),
		LDATA14 (FIVE,1,2), LDATA2 (THREE,1,0),
		LDATA4 (FIVE,2,12), LDATA12 (THREE,2,0),
		LDATA9 (THREE,3,7), LDATA7 (SEVENMO,3,0),
		LDATA (SEVENMO,3,7), EMPTY15,
		LDATA11 (SEVENMO,4,5), LDATA5 (SEVENMO0,4,0),
		LDATA (SEVENMO,6,15), LDATA15 (SEVENMO0,6,0),
		LDATA5 (SEVENMO0,7,11), LDATA11 (SEVENMO1,7,0),
		LDATA10 (SEVENMO1,5,6), EMPTY6,
		EMPTY16,
		LDATA14 (SEVENMO,0,2), EMPTY2
	};
	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN
	#undef SEVEN6

	pflookup = roadblst_pflookup;
	molookup = roadblst_molookup;
	return atarisys1_vh_start ();
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void atarisys1_vh_stop (void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;
	if (tempbitmap)
		osd_free_bitmap (tempbitmap);
	tempbitmap = 0;

	/* free dirty buffers */
	if (pfmapped)
		free (pfmapped);
	colordirty = spritevisit = 0;
	pfmapped = 0;
	displaylist = 0;
	scrolllist = 0;
}



/*************************************
 *
 *		Graphics bank selection
 *
 *************************************/

void atarisys1_bankselect_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_bankselect[offset]);
	int newword = COMBINE_WORD (oldword, data);
	int diff = oldword ^ newword;

	/* update memory */
	WRITE_WORD (&atarisys1_bankselect[offset], newword);

	/* sound CPU reset */
	if (diff & 0x80)
	{
		if (data & 0x80)
			atarisys1_sound_reset ();
		else
			cpu_halt (1, 0);
	}

	/* motion object bank select */
	atarisys1_update_display_list (cpu_getscanline ());

	/* playfield bank select */
	if (diff & 0x04)
	{
		int i, *pf;

		/* set the new bank globally */
		pfbank = (newword & 0x04) ? 0x80 : 0x00;

		/* reset the color counters */
		memset (playfield_count, 0, sizeof (playfield_count));

		/* and remap the entire playfield */
		for (i = atarisys1_playfieldram_size / 2, pf = pfmapped; i >= 0; i--)
		{
			int val = READ_WORD (&atarisys1_playfieldram[i * 2]);
			int map = pflookup[pfbank | ((val >> 8) & 0x7f)] | (val & 0xff) | ((val & 0x8000) << 9) | LDIRTYFLAG;
			playfield_count[LCOLOR (map)] += 1;
			*pf++ = map;
		}
	}
}



/*************************************
 *
 *		Playfield special priority color
 *
 *************************************/

int atarisys1_prioritycolor_r (int offset)
{
	return READ_WORD (&atarisys1_prioritycolor[offset]);
}


void atarisys1_prioritycolor_w (int offset, int data)
{
	COMBINE_WORD_MEM (&atarisys1_prioritycolor[offset], data);
}



/*************************************
 *
 *		Playfield horizontal scroll
 *
 *************************************/

void atarisys1_hscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_hscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&atarisys1_hscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((oldword & 0x1ff) << 12) + (READ_WORD (&atarisys1_vscroll[0]) & 0x1ff);
		int i, end = cpu_getscanline ();

		if (end > YDIM)
			end = YDIM;

		for (i = scrolllist_end; i < end; i++)
			scrolllist[i] = scrollval;
		scrolllist_end = end;
	}
}



/*************************************
 *
 *		Playfield vertical scroll
 *
 *************************************/

void atarisys1_vscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_vscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&atarisys1_vscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((READ_WORD (&atarisys1_hscroll[0]) & 0x1ff) << 12) + (oldword & 0x1ff);
		int i, end = cpu_getscanline ();

		if (end > YDIM)
			end = YDIM;

		for (i = scrolllist_end; i < end; i++)
			scrolllist[i] = scrollval;
		scrolllist_end = end;
	}
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int atarisys1_playfieldram_r (int offset)
{
	return READ_WORD (&atarisys1_playfieldram[offset]);
}


void atarisys1_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		int map, oldmap;

		WRITE_WORD (&atarisys1_playfieldram[offset], newword);

		/* remap it now and mark it dirty in the process */
		map = pflookup[pfbank | ((newword >> 8) & 0x7f)] | (newword & 0xff) | ((newword & 0x8000) << 9) | LDIRTYFLAG;
		oldmap = pfmapped[offset / 2];
		pfmapped[offset / 2] = map;

		/* update the color counts */
		playfield_count[LCOLOR (oldmap)] -= 1;
		playfield_count[LCOLOR (map)] += 1;
	}
}



/*************************************
 *
 *		Sprite RAM read/write handlers
 *
 *************************************/

int atarisys1_spriteram_r (int offset)
{
	return READ_WORD (&atarisys1_spriteram[offset]);
}


void atarisys1_spriteram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_spriteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarisys1_spriteram[offset], newword);

		/* if modifying a timer, beware */
		if (((offset & 0x180) == 0x000 && READ_WORD (&atarisys1_spriteram[offset | 0x080]) == 0xffff) ||
		    ((offset & 0x180) == 0x080 && newword == 0xffff))
		{
			/* if the timer is in the active bank, update the display list */
			if ((offset >> 9) == ((READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7))
			{
				if (errorlog) fprintf (errorlog, "Caught timer mod!\n");
				atarisys1_update_display_list (cpu_getscanline ());
			}
		}
	}
}



/*************************************
 *
 *		Palette RAM read/write handlers
 *
 *************************************/

int atarisys1_paletteram_r (int offset)
{
	return READ_WORD (&atarisys1_paletteram[offset]);
}


void atarisys1_paletteram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarisys1_paletteram[offset], newword);
		colordirty[offset / 2] = 1;
	}
}



/*************************************
 *
 *		Motion object interrupt handlers
 *
 *************************************/

void atarisys1_int3off_callback (int param)
{
	/* clear the state */
	int3_state = 0;

	/* clear the interrupt generated as well */
	cpu_clear_pending_interrupts (0);

	/* make this timer go away */
	int3off_timer = 0;
}


void atarisys1_int3_callback (int param)
{
	/* generate the interrupt */
	cpu_cause_interrupt (0, 3);

	/* update the state */
	int3_state = 1;

	/* set a timer to turn it off */
	if (int3off_timer)
		timer_remove (int3off_timer);
	int3off_timer = timer_set (cpu_getscanlineperiod (), 0, atarisys1_int3off_callback);

	/* set ourselves up to go off next frame */
	int3_timer[param] = timer_set (TIME_IN_HZ (Machine->drv->frames_per_second), param, atarisys1_int3_callback);
}


int atarisys1_int3state_r (int offset)
{
	return int3_state ? 0x0080 : 0x0000;
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void atarisys1_update_display_list (int scanline)
{
	unsigned short *d = displaylist_end, *startd = d, *lastd = displaylist_last;
	int bank = ((READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7) * 0x200;
	int link = 0, match = 0, i;
	char timer[YDIM];

	/* if the last list entries were on the same scanline, overwrite them */
	if (lastd)
	{
		if (*lastd == scanline)
			d = startd = lastd;
		else
			match = 1;
	}

	/* visit all the sprites and copy their data into the display list */
	memset (spritevisit, 0, atarisys1_spriteram_size / 8 / 8);
	memset (timer, 0, sizeof (timer));
	while (!spritevisit[link])
	{
		int data1 = READ_WORD (&atarisys1_spriteram[bank + link * 2 + 0x000]);
		int data2 = READ_WORD (&atarisys1_spriteram[bank + link * 2 + 0x080]);
		int data3 = READ_WORD (&atarisys1_spriteram[bank + link * 2 + 0x100]);

		/* a picture of 0xffff is really an interrupt - gross! */
		if (data2 == 0xffff)
		{
			int vsize = (data1 & 15) + 1;
			int ypos = (256 - (data1 >> 5) - vsize * 8) & 0x1ff;

			/* only generate timers on visible scanlines */
			if (ypos < YDIM)
				timer[ypos] = 1;
		}

		/* ignore updates after the end of the frame */
		else if (scanline < YDIM && data2)
		{
			*d++ = scanline;
			*d++ = data1;
			*d++ = data2;
			*d++ = data3;

			/* update our match status */
			if (match)
			{
				lastd++;
				if (*lastd++ != data1 || *lastd++ != data2 || *lastd++ != data3)
					match = 0;
			}
		}

		/* link to the next object */
		spritevisit[link] = 1;
		link = READ_WORD (&atarisys1_spriteram[bank + link * 2 + 0x180]) & 0x3f;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		displaylist_end = d;
		displaylist_last = startd;
	}

	/* update our interrupt timers */
	for (i = 0; i < YDIM; i++)
	{
		if (timer[i] && !int3_timer[i])
			int3_timer[i] = timer_set (cpu_getscanlinetime (i), i, atarisys1_int3_callback);
		else if (!timer[i] && int3_timer[i])
		{
			timer_remove (int3_timer[i]);
			int3_timer[i] = 0;
		}
	}
}



void atarisys1_begin_frame (int param)
{
	/* update the counts in preparation for the new frame */
	displaylist_end = displaylist;
	displaylist_last = NULL;
	scrolllist_end = 0;

	/* update the display list for the beginning of the frame */
	atarisys1_update_display_list (0);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static int xscroll, yscroll;


/*************************************
 *
 *		Refresh helpers
 *
 *************************************/

static inline void remap_color (int color)
{
	int i, j;

	/* first see if there is a match from last frame */
	for (i = 0; i < 16; i++)
		if (last_map[i] == color && this_map[i] < 0)
		{
			this_map[i] = color;
			return;
		}

	/* if not, pick a blank entry from last frame and use it */
	for (i = 0; i < 16; i++)
		if (last_map[i] < 0 && this_map[i] < 0)
		{
			this_map[i] = color;

			/* and update the colortable appropriately */
			for (j = 0; j < 16; j++)
				Machine->colortable[color * 16 + j] = Machine->pens[i * 16 + j];
			return;
		}

	/* if all else fails, pick a blank entry from this frame and use it */
	for (i = 0; i < 16; i++)
		if (this_map[i] < 0)
		{
			this_map[i] = color;

			/* and update the colortable appropriately */
			for (j = 0; j < 16; j++)
				Machine->colortable[color * 16 + j] = Machine->pens[i * 16 + j];
			return;
		}
}


/*
 *   playfield redraw function
 */

inline void redraw_playfield_chunk (struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int transparency, int transparency_color, int check_color)
{
	int y, x;

	/* round the positions */
	xpos = (xpos - xscroll) / 8;
	ypos = (ypos - yscroll) / 8;

	/* loop over the rows */
	for (y = ypos + h; y >= ypos; y--)
	{
		/* compute the scroll-adjusted y position */
		int sy = (y * 8 + yscroll) & 0x1ff;
		if (sy > 0x1f8) sy -= 0x200;

		/* loop over the columns */
		for (x = xpos + w; x >= xpos; x--)
		{
			/* compute the scroll-adjusted x position */
			int sx = (x * 8 + xscroll) & 0x1ff;
			if (sx > 0x1f8) sx -= 0x200;

			{
				/* process the data */
				int data = pfmapped[(y & 0x3f) * 64 + (x & 0x3f)];
				int color = LCOLOR (data);

				/* draw */
				if (check_color < 0 || color == check_color)
					drawgfx (bitmap, Machine->gfx[LBANK (data)], LPICT (data), color, LFLIP (data), 0,
							sx, sy, &Machine->drv->visible_area, transparency, transparency_color);
			}
		}
	}
}



/*************************************
 *
 *		Generic System 1 refresh
 *
 *************************************/

void atarisys1_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	int x, y, sx, sy, xoffs, yoffs, offs, hipri_pen = -1, i;
	int pcolor = READ_WORD (&atarisys1_prioritycolor[0]) & 0xff;
	int redraw_list[1024], *redraw = redraw_list;
	int sprite_map[16], alpha_map[16];


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 0: debugging
	 *
	 *---------------------------------------------------------------------------------
	 */
	#if SYSTEM1_DEBUG
		int hidebank = atarisys1_debug ();
	#endif


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 1: give the playfield first pick of colors
	 *
	 *---------------------------------------------------------------------------------
	 */
	for (i = 0; i < 16; i++)
	{
		last_map[i] = this_map[i];
		this_map[i] = playfield_count[(i + 16) >> pf_map_shift] ? i + 32 : -1;
		sprite_map[i] = alpha_map[i] = 0;
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 2: load the scroll values from the start of the previous frame
	 *
	 *---------------------------------------------------------------------------------
	 */
	if (scrolllist_end)
	{
		xscroll = scrolllist[0] >> 12;
		yscroll = scrolllist[0];
	}
	else
	{
		xscroll = READ_WORD (&atarisys1_hscroll[0]);
		yscroll = READ_WORD (&atarisys1_vscroll[0]);
	}

	xscroll = -(xscroll & 0x1ff);
	yscroll = -(yscroll & 0x1ff);

	xoffs = (-xscroll / 8);
	yoffs = (-yscroll / 8);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 3: update the offscreen version of the visible area of the playfield
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-14  = index of the image; a 16th bit is pulled from the playfield
	 *                    bank selection bit.  The upper 8 bits of this 16-bit index
	 *                    are passed through a pair of lookup PROMs to select which
	 *                    graphics bank and color to use
	 *			Bit  15    = horizontal flip
	 *
	 *---------------------------------------------------------------------------------
	 */
	for (y = yoffs + YCHARS + 1; y >= yoffs; y--)
	{
		sy = y & 63;
		for (x = xoffs + XCHARS + 1; x >= xoffs; x--)
		{
			int data;

			sx = x & 63;
			offs = sy * 64 + sx;
			data = pfmapped[offs];

			if (LDIRTY (data))
			{
				int bank = LBANK (data);
				if (bank)
					drawgfx (playfieldbitmap, Machine->gfx[bank], LPICT (data), LCOLOR (data), LFLIP (data), 0,
							8*sx, 8*sy, 0, TRANSPARENCY_NONE, 0);
				pfmapped[offs] = data & ~LDIRTYFLAG;
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 4: copy the playfield bitmap to the destination
	 *
	 *---------------------------------------------------------------------------------
	 */
	copyscrollbitmap (bitmap, playfieldbitmap,
			1, &xscroll,
			1, &yscroll,
			&Machine->drv->visible_area,
			TRANSPARENCY_NONE, 0);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 5: render the pregenerated list of motion objects
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Motion Object encoding
	 *
	 *		4 16-bit words are used total
	 *
	 *		Word 1: Vertical position
	 *
	 *			Bits 0-3   = vertical size of the object, in tiles
	 *			Bits 5-13  = vertical position
	 *			Bit  15    = horizontal flip
	 *
	 *		Word 2: Image
	 *
	 *			Bits 0-15  = index of the image; the upper 8 bits are passed through a
	 *			             pair of lookup PROMs to select which graphics bank and color
	 *			              to use (high bit of color is ignored and comes from Bit 15, below)
	 *
	 *		Word 3: Horizontal position
	 *
	 *			Bits 0-3   = horizontal size of the object, in tiles
	 *			Bits 5-13  = horizontal position
	 *			Bit  15    = special playfield priority
	 *
	 *		Word 4: Link
	 *
	 *			Bits 0-5   = link to the next motion object
	 *
	 *---------------------------------------------------------------------------------
	 */
	{
		int last_start_scan = -1;
		struct rectangle clip;
		unsigned short *d;

		/* create a clipping rectangle so that only partial sections are updated at a time */
		clip.min_x = 0;
		clip.max_x = Machine->drv->screen_width - 1;

		/* loop over the list until the end */
		for (d = displaylist; d < displaylist_end; d += 4)
		{
			int start_scan = d[0], data1 = d[1], data2 = d[2], data3 = d[3];

			/* extract data from the various words */
			int pict = data2;
			int lookup = molookup[pict >> 8];
			int vsize = (data1 & 15) + 1;
			int xpos = data3 >> 5;
			int ypos = 256 - (data1 >> 5) - vsize * 8;
			int color = LCOLOR (lookup);
			int bank = LBANK (lookup);
			int hflip = data1 & 0x8000;
			int hipri = data3 & 0x8000;

			#if SYSTEM1_DEBUG
				if ((pict >> 8) == hidebank)
					color = Machine->gfx[bank]->total_colors - 1;
			#endif

			/* do we have a priority color active? */
			if (pcolor)
			{
				int *r, val;

				/* if so, add an entry to the redraw list for later */
				val = ((xpos & 0x1ff) << 23) + ((ypos & 0x1ff) << 14) + vsize;
				for (r = redraw_list; r < redraw; )
					if (*r++ == val)
						break;

				/* but only add it if we don't have a matching entry already */
				if (r == redraw)
					*redraw++ = val;
			}

			/* if this is a new region of the screen, find the end and adjust our clipping */
			if (start_scan != last_start_scan)
			{
				unsigned short *df;

				last_start_scan = start_scan;
				clip.min_y = start_scan;

				/* look for an entry whose scanline start is different from ours; that's our bottom */
				for (df = d; df < displaylist_end; df += 4)
					if (*df != start_scan)
					{
						clip.max_y = *df - 1;
						break;
					}

				/* if we didn't find any additional regions, go until the bottom of the screen */
				if (df == displaylist_end)
					clip.max_y = Machine->drv->screen_height - 1;
			}

			/* keep them in range to start */
			xpos &= 0x1ff;
			ypos &= 0x1ff;

			/*
			 *
			 *      case 1: normal
			 *
			 */
			if (!hipri)
			{
				/* wrap and clip the X coordinate */
				if (xpos > 0x1f8) xpos -= 0x200;
				if (xpos > -8 && xpos < XDIM)
				{
					/* wrap the Y coordinate */
					if (ypos >= YDIM) ypos -= 0x200;

					/* loop over the height */
					for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
					{
						/* clip the Y coordinate */
						if (sy <= -8)
							continue;
						else if (sy >= YDIM)
							break;

						/* if this sprite's color has not been used yet this frame, grab it */
						/* note that by doing it here, we don't map sprites that aren't visible */
						{
							int temp = color >> sprite_map_shift;
							if (!sprite_map[temp])
							{
								remap_color (temp + 16);
								sprite_map[temp] = 1;
							}
						}

						/* draw the sprite */
						drawgfx (bitmap, Machine->gfx[bank],
								LPICT (lookup) | (pict & 0xff), color,
								hflip, 0, xpos, sy, &clip, TRANSPARENCY_PEN, 0);
					}
				}
			}

			/*
			 *
			 *      case 2: high priority playfield
			 *
			 */
			else
			{
				int through_color = Machine->gfx[bank]->total_colors - 1;
				struct rectangle tclip;

				/* wrap and clip the X coordinate */
				if (xpos > 0x1f8) xpos -= 0x200;
				if (xpos > -8 && xpos < XDIM)
				{
					/* wrap the Y coordinate */
					if (ypos >= YDIM) ypos -= 0x200;

					/* loop over the height */
					for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
					{
						/* clip the Y coordinate */
						if (sy <= -8)
							continue;
						else if (sy >= YDIM)
							break;

						/* if this sprite's color has not been used yet this frame, grab it */
						/* note that by doing it here, we don't map sprites that aren't visible */
						{
							int temp = color >> sprite_map_shift;
							if (!sprite_map[temp])
							{
								remap_color (temp + 16);
								sprite_map[temp] = 1;
							}
						}

						/* if the hipri_pen is invalid, find a new one */
						if (hipri_pen < 0 || this_map[hipri_pen] >= 0)
						{
							int i, pen;

							/* find an unused slice of the palette */
							for (i = 15; i > 0; i--)
								if (this_map[i] < 0)
								{
									hipri_pen = i;
									break;
								}

							/* then set the colortable up for it */
							pen = Machine->pens[hipri_pen * 16];
							for (i = 1024 + 1; i < 1024 + 64; i++)
								Machine->colortable[i] = pen;
						}

						/* draw the sprite in bright pink on the real bitmap */
						drawgfx (bitmap, Machine->gfx[bank],
								LPICT (lookup) | (pict & 0xff), through_color,
								hflip, 0, xpos, sy, &clip, TRANSPARENCY_PEN, 0);

						/* also draw the sprite normally on the temp bitmap */
						if (!hipri_color0)
							drawgfx (tempbitmap, Machine->gfx[bank],
									LPICT (lookup) | (pict & 0xff), color,
									hflip, 0, xpos, sy, &clip, TRANSPARENCY_NONE, 0);
					}
				}

				/* now redraw the playfield tiles over top of the sprite */
				if (hipri_color0)
					redraw_playfield_chunk (bitmap, xpos, ypos, 1, vsize, TRANSPARENCY_THROUGH, hipri_pen * 16, -1);
				else
				{
					redraw_playfield_chunk (tempbitmap, xpos, ypos, 1, vsize, TRANSPARENCY_PEN, 0, -1);

					/* finally, copy this chunk to the real bitmap */
					if (xpos > XDIM) xpos -= 0x200;
					if (ypos > YDIM) ypos -= 0x200;
					tclip.min_x = xpos;
					tclip.max_x = xpos + 7;
					tclip.min_y = ypos;
					tclip.max_y = ypos + vsize * 8 - 1;
					if (tclip.min_y < clip.min_y) tclip.min_y = clip.min_y;
					if (tclip.max_y > clip.max_y) tclip.max_y = clip.max_y;
					copybitmap (bitmap, tempbitmap, 0, 0, 0, 0, &tclip, TRANSPARENCY_THROUGH, hipri_pen * 16);
				}
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 6: redraw playfield tiles that need it
	 *
	 *---------------------------------------------------------------------------------
	 */
	{
		int *r;
		for (r = redraw_list; r < redraw; r++)
		{
			int val = *r;
			int xpos = (val >> 23) & 0x1ff;
			int ypos = (val >> 14) & 0x1ff;
			int h = val & 0x1f;

			redraw_playfield_chunk (bitmap, xpos, ypos, 1, h, TRANSPARENCY_PENS, ~pcolor, 16 >> pf_map_shift);
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 7: render the alpha layer
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bits 10-12 = color
	 *			Bit  13    = transparency
	 *
	 *---------------------------------------------------------------------------------
	 */

	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarisys1_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x2000))
			{
				int color = ((data >> 10) & 7);

				/* if this character's color has not been used yet this frame, grab it */
				/* note that by doing it here, we don't map characters that aren't visible */
				if (!alpha_map[color / 4])
				{
					remap_color (color / 4);
					alpha_map[color / 4] = 1;
				}

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x2000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 8: update the palette
	 *
	 *---------------------------------------------------------------------------------
	 */
	for (i = 0; i < 16; i++)
		if (this_map[i] >= 0)
		{
			int index = this_map[i] * 16, pal = i * 16, j;

			/* modify the pens if this set is dirty or different than last frame */
			for (j = 0; j < 16; j++)
				if (colordirty[index + j] || this_map[i] != last_map[i])
				{
					int palette = READ_WORD (&atarisys1_paletteram[(index + j) * 2]);

					int inten = intensity_table[(palette >> 12) & 15];
					int red = (((palette >> 8) & 15) * inten) >> 4;
					int green = (((palette >> 4) & 15) * inten) >> 4;
					int blue = ((palette & 15) * inten) >> 4;

					osd_modify_pen (Machine->pens[pal + j], red, green, blue);

					colordirty[index + j] = 0;
				}
		}
}



/*************************************
 *
 *		Road Blasters refresh
 *
 *************************************/

void roadblst_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	int pcolor = READ_WORD (&atarisys1_prioritycolor[0]) & 0xff;
	int redraw_list[1024], *redraw = redraw_list;
	int y, sx, sy, offs, hipri_pen = -1;
	unsigned char smalldirty[32];


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 0: debugging
	 *
	 *---------------------------------------------------------------------------------
	 */
	#if SYSTEM1_DEBUG
		int hidebank = atarisys1_debug ();
	#endif


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 1: update the colortable
	 *
	 *---------------------------------------------------------------------------------
	 */
	memset (smalldirty, 0, sizeof (smalldirty));
	for (offs = atarisys1_paletteram_size - 2; offs >= 0; offs -= 2)
	{
		if (colordirty[offs / 2])
		{
			int palette = READ_WORD (&atarisys1_paletteram[offs]);
			int inten = intensity_table[(palette >> 12) & 15];
			int red = (((palette >> 8) & 15) * inten) >> 4;
			int green = (((palette >> 4) & 15) * inten) >> 4;
			int blue = ((palette & 15) * inten) >> 4;

			int index = offs / 2;
			int penindex;

			if (index >= 256 && index < 768)
				smalldirty[(index - 256) >> 4] = 1;

			/* compute the pen index, but map away from the high priority color */
			penindex = rgbpenindex (red, green, blue);
			Machine->colortable[index] = Machine->pens[penindex];

			colordirty[offs / 2] = 0;
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 2: update the offscreen version of the playfield
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-14  = index of the image; a 16th bit is pulled from the playfield bank selection
	 *			             bit.  The upper 8 bits of this 16-bit index are passed through a pair of
	 *			             lookup PROMs to select which graphics bank and color to use
	 *			Bit  15    = horizontal flip
	 *
	 *---------------------------------------------------------------------------------
	 */
	for (sy = offs = 0; sy < 64; sy++)
	{
		for (sx = 0; sx < 64; sx++, offs++)
		{
			int data = pfmapped[offs];
			int color = LCOLOR (data);
			int gfxbank = LBANK (data);

			/* things only get tricky if we're not already dirty */
			if (!LDIRTY (data))
			{
				/* check for the non-6-bit graphics case */
				if (gfxbank != 1)
				{
					if (!smalldirty[color])
						continue;
				}
				else
				{
					int temp = color * 4;
					if (!smalldirty[temp] && !smalldirty[temp+1] && !smalldirty[temp+2] && !smalldirty[temp+3])
						continue;
				}
			}

			/* draw this character */
			drawgfx (playfieldbitmap, Machine->gfx[gfxbank], LPICT (data), color, LFLIP (data), 0,
					8*sx, 8*sy, 0, TRANSPARENCY_NONE, 0);
			pfmapped[offs] = data & ~LDIRTYFLAG;
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 3: copy the playfield bitmap to the destination using the scroll list
	 *
	 *---------------------------------------------------------------------------------
	 */
	{
		int shift = bitmap->depth / 8 - 1, lasty = -1;
		unsigned int *scroll = scrolllist;

		/* finish the scrolling list from the previous frame */
		xscroll = READ_WORD (&atarisys1_hscroll[0]);
		yscroll = READ_WORD (&atarisys1_vscroll[0]);
		offs = ((xscroll & 0x1ff) << 12) + (yscroll & 0x1ff);
		for (y = scrolllist_end; y < YDIM; y++)
			scrolllist[y] = offs;

		/* loop over and copy the data row by row */
		for (y = 0; y < YDIM; y++)
		{
			int xscroll = (*scroll >> 12) & 0x1ff;
			int yscroll = *scroll++ & 0x1ff;
			int dy;

			/* when a write to the scroll register occurs, the counter is reset */
			if (yscroll != lasty)
			{
				offs = y;
				lasty = yscroll;
			}
			dy = (yscroll + y - offs) & 0x1ff;

			/* handle the wrap around case */
			if (xscroll + XDIM > 0x200)
			{
				int chunk = 0x200 - xscroll;
				memcpy (&bitmap->line[y][0], &playfieldbitmap->line[dy][xscroll << shift], chunk << shift);
				memcpy (&bitmap->line[y][chunk << shift], &playfieldbitmap->line[dy][0], (XDIM - chunk) << shift);
			}
			else
				memcpy (&bitmap->line[y][0], &playfieldbitmap->line[dy][xscroll << shift], XDIM << shift);
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 4: render the pregenerated list of motion objects
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Motion Object encoding
	 *
	 *		4 16-bit words are used total
	 *
	 *		Word 1: Vertical position
	 *
	 *			Bits 0-3   = vertical size of the object, in tiles
	 *			Bits 5-13  = vertical position
	 *			Bit  15    = horizontal flip
	 *
	 *		Word 2: Image
	 *
	 *			Bits 0-15  = index of the image; the upper 8 bits are passed through a
	 *			             pair of lookup PROMs to select which graphics bank and color
	 *			              to use (high bit of color is ignored and comes from Bit 15, below)
	 *
	 *		Word 3: Horizontal position
	 *
	 *			Bits 0-3   = horizontal size of the object, in tiles
	 *			Bits 5-13  = horizontal position
	 *			Bit  15    = special playfield priority
	 *
	 *		Word 4: Link
	 *
	 *			Bits 0-5   = link to the next motion object
	 *
	 *---------------------------------------------------------------------------------
	 */
	{
		int last_start_scan = -1;
		struct rectangle clip;
		unsigned short *d;

		/* create a clipping rectangle so that only partial sections are updated at a time */
		clip.min_x = 0;
		clip.max_x = Machine->drv->screen_width - 1;

		/* loop over the list until the end */
		for (d = displaylist; d < displaylist_end; d += 4)
		{
			int start_scan = d[0], data1 = d[1], data2 = d[2], data3 = d[3];

			/* extract data from the various words */
			int pict = data2;
			int lookup = molookup[pict >> 8];
			int vsize = (data1 & 15) + 1;
			int xpos = data3 >> 5;
			int ypos = 256 - (data1 >> 5) - vsize * 8;
			int color = LCOLOR (lookup);
			int bank = LBANK (lookup);
			int hflip = data1 & 0x8000;
			int hipri = data3 & 0x8000;

			#if SYSTEM1_DEBUG
				if ((pict >> 12) == hidebank)
					color = Machine->gfx[bank]->total_colors - 1;
/*				if ((pict >> 12) == 8 && hidebank >= 1 && hidebank <= 8)
					bank = hidebank;*/
			#endif

			/* do we have a priority color active? */
			if (pcolor)
			{
				int *r, val;

				/* if so, add an entry to the redraw list for later */
				val = ((xpos & 0x1ff) << 23) + ((ypos & 0x1ff) << 14) + vsize;
				for (r = redraw_list; r < redraw; )
					if (*r++ == val)
						break;

				/* but only add it if we don't have a matching entry already */
				if (r == redraw)
					*redraw++ = val;
			}

			/* if this is a new region of the screen, find the end and adjust our clipping */
			if (start_scan != last_start_scan)
			{
				unsigned short *df;

				last_start_scan = start_scan;
				clip.min_y = start_scan;

				/* look for an entry whose scanline start is different from ours; that's our bottom */
				for (df = d; df < displaylist_end; df += 4)
					if (*df != start_scan)
					{
						clip.max_y = *df - 1;
						break;
					}

				/* if we didn't find any additional regions, go until the bottom of the screen */
				if (df == displaylist_end || clip.max_y >= ROADBLST_CLIP)
					clip.max_y = ROADBLST_CLIP - 1;
			}

			/* keep them in range to start */
			xpos &= 0x1ff;
			ypos &= 0x1ff;

			/*
			 *
			 *      case 1: normal
			 *
			 */
			if (!hipri)
			{
				/* wrap and clip the X coordinate */
				if (xpos > 0x1f8) xpos -= 0x200;
				if (xpos > -8 && xpos < XDIM)
				{
					/* wrap the Y coordinate */
					if (ypos >= YDIM) ypos -= 0x200;

					/* loop over the height */
					for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
					{
						/* clip the Y coordinate */
						if (sy <= -8)
							continue;
						else if (sy >= YDIM)
							break;

						/* draw the sprite */
						drawgfx (bitmap, Machine->gfx[bank],
								LPICT (lookup) | (pict & 0xff), color,
								hflip, 0, xpos, sy, &clip, TRANSPARENCY_PEN, 0);
					}
				}
			}

			/*
			 *
			 *      case 2: high priority playfield
			 *
			 */
			else
			{
				int through_color = Machine->gfx[bank]->total_colors - 1;

				/* wrap and clip the X coordinate */
				if (xpos > 0x1f8) xpos -= 0x200;
				if (xpos > -8 && xpos < XDIM)
				{
					/* wrap the Y coordinate */
					if (ypos >= YDIM) ypos -= 0x200;

					/* loop over the height */
					for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
					{
						/* clip the Y coordinate */
						if (sy <= -8)
							continue;
						else if (sy >= YDIM)
							break;

						/* draw the sprite in bright pink on the real bitmap */
						drawgfx (bitmap, Machine->gfx[bank],
								LPICT (lookup) | (pict & 0xff), through_color,
								hflip, 0, xpos, sy, &clip, TRANSPARENCY_PEN, 0);
					}
				}

				/* now redraw the playfield tiles over top of the sprite */
/*				redraw_playfield_chunk (bitmap, xpos, ypos, 1, vsize, TRANSPARENCY_THROUGH, hipri_pen * 16, -1);*/
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 5: render the alpha layer
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bits 10-12 = color
	 *			Bit  13    = transparency
	 *
	 *---------------------------------------------------------------------------------
	 */

	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarisys1_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x2000))
			{
				int color = ((data >> 10) & 7);

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x2000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

static int atarisys1_debug (void)
{
	int hidebank = -1;

	if (osd_key_pressed (OSD_KEY_Q)) hidebank = 0;
	if (osd_key_pressed (OSD_KEY_W)) hidebank = 1;
	if (osd_key_pressed (OSD_KEY_E)) hidebank = 2;
	if (osd_key_pressed (OSD_KEY_R)) hidebank = 3;
	if (osd_key_pressed (OSD_KEY_T)) hidebank = 4;
	if (osd_key_pressed (OSD_KEY_Y)) hidebank = 5;
	if (osd_key_pressed (OSD_KEY_U)) hidebank = 6;
	if (osd_key_pressed (OSD_KEY_I)) hidebank = 7;

	if (osd_key_pressed (OSD_KEY_A)) hidebank = 8;
	if (osd_key_pressed (OSD_KEY_S)) hidebank = 9;
	if (osd_key_pressed (OSD_KEY_D)) hidebank = 10;
	if (osd_key_pressed (OSD_KEY_F)) hidebank = 11;
	if (osd_key_pressed (OSD_KEY_G)) hidebank = 12;
	if (osd_key_pressed (OSD_KEY_H)) hidebank = 13;
	if (osd_key_pressed (OSD_KEY_J)) hidebank = 14;
	if (osd_key_pressed (OSD_KEY_K)) hidebank = 15;

	if (osd_key_pressed (OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i, bank;

		while (osd_key_pressed (OSD_KEY_9)) { }

		sprintf (name, "Dump %d", ++count);
		f = fopen (name, "wt");

		fprintf (f, "\n\nAlpha Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarisys1_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarisys1_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nPlayfield Palette:\n");
		for (i = 0x200; i < 0x300; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarisys1_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nTrans Palette:\n");
		for (i = 0x300; i < 0x320; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarisys1_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		bank = (READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7;
		fprintf (f, "\n\nMotion Object Bank = %d\n", bank);
		bank *= 0x200;
		for (i = 0; i < 0x40; i++)
		{
			fprintf (f, "   Object %02X:  P=%04X  X=%04X  Y=%04X  L=%04X\n",
					i,
					READ_WORD (&atarisys1_spriteram[bank+0x080+i*2]),
					READ_WORD (&atarisys1_spriteram[bank+0x100+i*2]),
					READ_WORD (&atarisys1_spriteram[bank+0x000+i*2]),
					READ_WORD (&atarisys1_spriteram[bank+0x180+i*2])
			);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarisys1_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarisys1_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}

	return hidebank;
}
