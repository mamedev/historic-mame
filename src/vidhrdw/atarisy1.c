/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 42
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)


/* the color and remap PROMs are mapped as follows */
#define PROM1_BANK_4			0x80		/* active low */
#define PROM1_BANK_3			0x40		/* active low */
#define PROM1_BANK_2			0x20		/* active low */
#define PROM1_BANK_1			0x10		/* active low */
#define PROM1_OFFSET_MASK		0x0f		/* postive logic */

#define PROM2_BANK_6_OR_7		0x80		/* active low */
#define PROM2_BANK_5			0x40		/* active low */
#define PROM2_PLANE_5_ENABLE	0x20		/* active high */
#define PROM2_PLANE_4_ENABLE	0x10		/* active high */
#define PROM2_PF_COLOR_MASK		0x0f		/* negative logic */
#define PROM2_BANK_7			0x08		/* active low, plus PROM2_BANK_6_OR_7 low as well */
#define PROM2_MO_COLOR_MASK		0x07		/* negative logic */


/* these macros make accessing the indirection table easier, plus this is how the data
   is stored for the pfmapped array */
#define PACK_LOOKUP_DATA(bank,color,offset,bpp) \
		(((((bpp) - 4) & 7) << 24) | \
		 (((color) & 255) << 16) | \
		 (((bank) & 15) << 12) | \
		 (((offset) & 15) << 8))

#define LOOKUP_DIRTY_FLAG 	0x20000000

#define LOOKUP_DIRTY(data) 	((data) & LOOKUP_DIRTY_FLAG)
#define LOOKUP_FLIP(data) 	((data) & 0x10000000)
#define LOOKUP_BPP(data)	(((data) >> 24) & 7)
#define LOOKUP_COLOR(data) 	(((data) >> 16) & 0xff)
#define LOOKUP_GFX(data) 	(((data) >> 12) & 15)
#define LOOKUP_IMAGE(data) 	((data) & 0x0fff)



struct atarisys1_mo_data
{
	int pcolor;
	int *redraw_list, *redraw;
};


/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys1_bankselect;
unsigned char *atarisys1_prioritycolor;

int roadblst_screen_refresh;



/*************************************
 *
 *		Statics
 *
 *************************************/

/* bitmaps */
static struct osd_bitmap *playfieldbitmap;
static struct osd_bitmap *tempbitmap;

/* scroll registers */
static int xscroll, yscroll;

/* indirection tables */
static int pflookup[256], molookup[256];

/* playfield data */
static int *pfmapped;
static int pfbank;

/* INT3 tracking */
static void *int3_timer[YDIM];
static void *int3off_timer;

/* Road Blasters list of scroll offsets */
static unsigned int *scrolllist;
static int scrolllist_end;

/* graphics bank tracking */
static unsigned char bank_gfx[3][8];
static unsigned int *pen_usage[MAX_GFX_ELEMENTS];

/* basic form of a graphics bank */
static struct GfxLayout atarisys1_objlayout =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	6,		/* 6 bits per pixel */
	{ 5*8*0x08000, 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every sprite takes 8 consecutive bytes */
};



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

int atarisys1_decode_gfx(void);
void atarisys1_vh_stop(void);
void atarisys1_update_display_list(int scanline);

static void redraw_playfield_chunk(struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int transparency, int transparency_color, int type);

extern int atarisys1_int3_state;

#if 0
static int atarisys1_debug(void);
#endif



/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int atarisys1_vh_start(void)
{
	static struct atarigen_modesc atarisys1_modesc =
	{
		64,                  /* maximum number of MO's */
		2,                   /* number of bytes per MO entry */
		0x80,                /* number of bytes between MO words */
		1,                   /* ignore an entry if this word == 0xffff */
		3, 0, 0x3f,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	int i, e;

	/* first decode the graphics */
	if (atarisys1_decode_gfx())
		return 1;

	/* allocate dirty buffers */
	if (!pfmapped)
		pfmapped = calloc(atarigen_playfieldram_size / 2 * sizeof(int) + YDIM * sizeof(int), 1);
	if (!pfmapped)
	{
		atarisys1_vh_stop();
		return 1;
	}
	scrolllist = (unsigned int *)(pfmapped + atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap(64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		atarisys1_vh_stop();
		return 1;
	}

	if (!tempbitmap)
		tempbitmap = osd_new_bitmap(Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth);
	if (!tempbitmap)
	{
		atarisys1_vh_stop();
		return 1;
	}

	/* reset the scroll list */
	scrolllist_end = 0;

	/* reset the timers */
	memset(int3_timer, 0, sizeof(int3_timer));
	atarisys1_int3_state = 0;

	/* initialize the pre-mapped playfield */
	for (i = atarigen_playfieldram_size / 2; i >= 0; i--)
		pfmapped[i] = pflookup[0] | LOOKUP_DIRTY_FLAG;

	/* initialize the pen usage array */
	for (e = 0; e < MAX_GFX_ELEMENTS; e++)
		if (Machine->gfx[e])
		{
			pen_usage[e] = Machine->gfx[e]->pen_usage;

			/* if this element has 6bpp data, create a special new usage array for it */
			if (Machine->gfx[e]->color_granularity == 64)
			{
				struct GfxElement *gfx = Machine->gfx[e];

				/* allocate storage */
				pen_usage[e] = malloc(gfx->total_elements * 2 * sizeof(int));
				if (pen_usage[e])
				{
					int x, y;
					unsigned int *entry;

					/* scan each entry, marking which pens are used */
					memset(pen_usage[e], 0, gfx->total_elements * 2 * sizeof(int));
					for (i = 0, entry = pen_usage[e]; i < gfx->total_elements; i++, entry += 2)
						for (y = 0; y < gfx->height; y++)
						{
							unsigned char *dp = gfx->gfxdata->line[i * gfx->height + y];
							for (x = 0; x < gfx->width; x++)
							{
								int color = dp[x];
								entry[(color >> 5) & 1] |= 1 << (color & 31);
							}
						}
				}
			}
		}

	/* initialize the displaylist system */
	return atarigen_init_display_list(&atarisys1_modesc);
}


/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void atarisys1_vh_stop(void)
{
	int i;

	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap(playfieldbitmap);
	playfieldbitmap = 0;
	if (tempbitmap)
		osd_free_bitmap(tempbitmap);
	tempbitmap = 0;

	/* free dirty buffers */
	if (pfmapped)
		free(pfmapped);
	pfmapped = 0;
	scrolllist = 0;

	/* free any extra pen usage */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		if (pen_usage[i] && Machine->gfx[i] && pen_usage[i] != Machine->gfx[i]->pen_usage)
			free(pen_usage[i]);
		pen_usage[i] = 0;
	}
}



/*************************************
 *
 *		Graphics decoding routines
 *
 *************************************/

int atarisys1_get_bank(unsigned char prom1, unsigned char prom2, int bpp)
{
	int bank_offset[8] = { 0, 0x00000, 0x30000, 0x60000, 0x90000, 0xc0000, 0xe0000, 0x100000 };
	int bank_index, i, gfx_index;

	/* determine the bank index */
	if ((prom1 & PROM1_BANK_1) == 0)
		bank_index = 1;
	else if ((prom1 & PROM1_BANK_2) == 0)
		bank_index = 2;
	else if ((prom1 & PROM1_BANK_3) == 0)
		bank_index = 3;
	else if ((prom1 & PROM1_BANK_4) == 0)
		bank_index = 4;
	else if ((prom2 & PROM2_BANK_5) == 0)
		bank_index = 5;
	else if ((prom2 & PROM2_BANK_6_OR_7) == 0)
	{
		if ((prom2 & PROM2_BANK_7) == 0)
			bank_index = 7;
		else
			bank_index = 6;
	}
	else
		return 0;

	/* find the bank */
	if (bank_gfx[bpp - 4][bank_index])
		return bank_gfx[bpp - 4][bank_index];

	/* don't have one? let's make it ... first find any empty slot */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == NULL)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return -1;

	/* tweak the structure for the number of bitplanes we have */
	atarisys1_objlayout.planes = bpp;
	for (i = 0; i < bpp; i++)
		atarisys1_objlayout.planeoffset[i] = (bpp - i - 1) * 0x8000 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(&Machine->memory_region[4][bank_offset[bank_index]], &atarisys1_objlayout);
	if (!Machine->gfx[gfx_index])
		return -1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = &Machine->colortable[256];
	Machine->gfx[gfx_index]->total_colors = (48 >> (bpp - 4)) + 1;

	/* set the entry and return it */
	return bank_gfx[bpp - 4][bank_index] = gfx_index;
}


int atarisys1_decode_gfx(void)
{
	unsigned char *prom1 = &Machine->memory_region[3][0x000];
	unsigned char *prom2 = &Machine->memory_region[3][0x200];
	int obj, i;

	/* reset the globals */
	memset(&bank_gfx, 0, sizeof(bank_gfx));

	/* loop for two sets of objects */
	for (obj = 0; obj < 2; obj++)
	{
		int *table = (obj == 0) ? pflookup : molookup;

		/* loop for 256 objects in the set */
		for (i = 0; i < 256; i++, prom1++, prom2++)
		{
			int bank, bpp, color, offset;

			/* determine the bpp */
			bpp = 4;
			if (*prom2 & PROM2_PLANE_4_ENABLE)
			{
				bpp = 5;
				if (*prom2 & PROM2_PLANE_5_ENABLE)
					bpp = 6;
			}

			/* determine the color */
			if (obj == 0)
				color = (16 + (~*prom2 & PROM2_PF_COLOR_MASK)) >> (bpp - 4); /* playfield */
			else
				color = (~*prom2 & PROM2_MO_COLOR_MASK) >> (bpp - 4);	/* motion objects (high bit ignored) */

			/* determine the offset */
			offset = *prom1 & PROM1_OFFSET_MASK;

			/* determine the bank */
			bank = atarisys1_get_bank(*prom1, *prom2, bpp);
			if (bank < 0)
				return 1;

			/* set the value */
			if (bank == 0)
				*table++ = 0;
			else
				*table++ = PACK_LOOKUP_DATA(bank, color, offset, bpp);
		}
	}
	return 0;
}



/*************************************
 *
 *		Graphics bank selection
 *
 *************************************/

void atarisys1_bankselect_w(int offset, int data)
{
	int oldword = READ_WORD(&atarisys1_bankselect[offset]);
	int newword = COMBINE_WORD(oldword, data);
	int diff = oldword ^ newword;

	/* update memory */
	WRITE_WORD(&atarisys1_bankselect[offset], newword);

	/* sound CPU reset */
	if (diff & 0x80)
	{
		if (data & 0x80)
			atarigen_sound_reset_w(0, 0);
		else
			cpu_halt(1, 0);
	}

	/* motion object bank select */
	atarisys1_update_display_list(cpu_getscanline());

	/* playfield bank select */
	if (diff & 0x04)
	{
		int i, *pf;

		/* set the new bank globally */
		pfbank = (newword & 0x04) ? 0x80 : 0x00;

		/* and remap the entire playfield */
		for (i = atarigen_playfieldram_size / 2, pf = pfmapped; i >= 0; i--)
		{
			int val = READ_WORD(&atarigen_playfieldram[i * 2]);
			int map = pflookup[pfbank | ((val >> 8) & 0x7f)] | (val & 0xff) | ((val & 0x8000) << 13) | LOOKUP_DIRTY_FLAG;
			*pf++ = map;
		}
	}
}



/*************************************
 *
 *		Playfield horizontal scroll
 *
 *************************************/

void atarisys1_hscroll_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_hscroll[offset]);
	int newword = COMBINE_WORD(oldword, data);

	WRITE_WORD(&atarigen_hscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((oldword & 0x1ff) << 12) + (READ_WORD(&atarigen_vscroll[0]) & 0x1ff);
		int i, end = cpu_getscanline();

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

void atarisys1_vscroll_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_vscroll[offset]);
	int newword = COMBINE_WORD(oldword, data);

	WRITE_WORD(&atarigen_vscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((READ_WORD(&atarigen_hscroll[0]) & 0x1ff) << 12) + (oldword & 0x1ff);
		int i, end = cpu_getscanline();

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

int atarisys1_playfieldram_r(int offset)
{
	return READ_WORD(&atarigen_playfieldram[offset]);
}


void atarisys1_playfieldram_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD(oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_playfieldram[offset], newword);

		/* remap it now and mark it dirty in the process */
		pfmapped[offset / 2] = pflookup[pfbank | ((newword >> 8) & 0x7f)] | (newword & 0xff) | ((newword & 0x8000) << 13) | LOOKUP_DIRTY_FLAG;
	}
}



/*************************************
 *
 *		Sprite RAM read/write handlers
 *
 *************************************/

int atarisys1_spriteram_r(int offset)
{
	return READ_WORD(&atarigen_spriteram[offset]);
}


void atarisys1_spriteram_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_spriteram[offset]);
	int newword = COMBINE_WORD(oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_spriteram[offset], newword);

		/* if modifying a timer, beware */
		if (((offset & 0x180) == 0x000 && READ_WORD(&atarigen_spriteram[offset | 0x080]) == 0xffff) ||
		    ((offset & 0x180) == 0x080 && newword == 0xffff))
		{
			/* if the timer is in the active bank, update the display list */
			if ((offset >> 9) == ((READ_WORD(&atarisys1_bankselect[0]) >> 3) & 7))
			{
				if (errorlog) fprintf(errorlog, "Caught timer mod!\n");
				atarisys1_update_display_list(cpu_getscanline());
			}
		}
	}
}



/*************************************
 *
 *		Motion object interrupt handlers
 *
 *************************************/

void atarisys1_int3off_callback(int param)
{
	/* clear the state */
	atarisys1_int3_state = 0;
	atarigen_update_interrupts();

	/* make this timer go away */
	int3off_timer = 0;
}


void atarisys1_int3_callback(int param)
{
	/* update the state */
	atarisys1_int3_state = 1;
	atarigen_update_interrupts();

	/* set a timer to turn it off */
	if (int3off_timer)
		timer_remove(int3off_timer);
	int3off_timer = timer_set(cpu_getscanlineperiod(), 0, atarisys1_int3off_callback);

	/* set ourselves up to go off next frame */
	int3_timer[param] = timer_set(TIME_IN_HZ(Machine->drv->frames_per_second), param, atarisys1_int3_callback);
}


int atarisys1_int3state_r(int offset)
{
	return atarisys1_int3_state ? 0x0080 : 0x0000;
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void atarisys1_update_display_list(int scanline)
{
	int bank = ((READ_WORD(&atarisys1_bankselect[0]) >> 3) & 7) * 0x200;
	unsigned char *base = &atarigen_spriteram[bank];
	unsigned char spritevisit[64], timer[YDIM];
	int link = 0, i;

	/* generic update first */
	if (!scanline)
	{
		scrolllist_end = 0;
		atarigen_update_display_list(base, 0, 0);
	}
	else
		atarigen_update_display_list(base, 0, scanline + 1);

	/* visit all the sprites and look for timers */
	memset(spritevisit, 0, sizeof(spritevisit));
	memset(timer, 0, sizeof(timer));
	while (!spritevisit[link])
	{
		int data2 = READ_WORD(&base[link * 2 + 0x080]);

		/* a picture of 0xffff is really an interrupt - gross! */
		if (data2 == 0xffff)
		{
			int data1 = READ_WORD(&base[link * 2 + 0x000]);
			int vsize = (data1 & 15) + 1;
			int ypos = (256 - (data1 >> 5) - vsize * 8) & 0x1ff;

			/* only generate timers on visible scanlines */
			if (ypos < YDIM)
				timer[ypos] = 1;
		}

		/* link to the next object */
		spritevisit[link] = 1;
		link = READ_WORD(&atarigen_spriteram[bank + link * 2 + 0x180]) & 0x3f;
	}

	/* update our interrupt timers */
	for (i = 0; i < YDIM; i++)
	{
		if (timer[i] && !int3_timer[i])
			int3_timer[i] = timer_set(cpu_getscanlinetime(i), i, atarisys1_int3_callback);
		else if (!timer[i] && int3_timer[i])
		{
			timer_remove(int3_timer[i]);
			int3_timer[i] = 0;
		}
	}
}


/*
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

void atarisys1_calc_mo_colors(struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int lookup = molookup[data[1] >> 8];
	int color = LOOKUP_COLOR(lookup);
	int bpp = LOOKUP_BPP(lookup);
	colors[bpp * 32 + color] = 1;
}


void atarisys1_render_mo(struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct atarisys1_mo_data *modata = param;
	int y, sy, redraw_val;

	/* extract data from the various words */
	int pict = data[1];
	int lookup = molookup[pict >> 8];
	int vsize = (data[0] & 15) + 1;
	int xpos = data[2] >> 5;
	int ypos = 256 - (data[0] >> 5) - vsize * 8;
	int color = LOOKUP_COLOR(lookup);
	int bank = LOOKUP_GFX(lookup);
	int hflip = data[0] & 0x8000;
	int hipri = data[2] & 0x8000;

	/* adjust the final coordinates */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	redraw_val = (xpos << 23) + (ypos << 14) + vsize;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* bail if X coordinate is out of range */
	if (xpos <= -8 || xpos >= XDIM)
		return;

	/* do we have a priority color active? */
	if (modata->pcolor)
	{
		int *redraw_list = modata->redraw_list, *redraw = modata->redraw;
		int *r;

		/* if so, add an entry to the redraw list for later */
		for (r = redraw_list; r < redraw; )
			if (*r++ == redraw_val)
				break;

		/* but only add it if we don't have a matching entry already */
		if (r == redraw)
		{
			*redraw++ = redraw_val;
			modata->redraw = redraw;
		}
	}

	/*
	 *
	 *  case 1: normal
	 *
	 */

	if (!hipri)
	{
		/* loop over the height */
		for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
		{
			/* clip the Y coordinate */
			if (sy <= clip->min_y - 8)
				continue;
			else if (sy > clip->max_y)
				break;

			/* draw the sprite */
			drawgfx(bitmap, Machine->gfx[bank],
					LOOKUP_IMAGE(lookup) | (pict & 0xff), color,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}

	/*
	 *
	 *  case 2: translucency
	 *
	 */

	else
	{
		struct rectangle tclip;

		/* loop over the height */
		for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
		{
			/* clip the Y coordinate */
			if (sy <= clip->min_y - 8)
				continue;
			else if (sy > clip->max_y)
				break;

			/* draw the sprite in bright pink on the real bitmap */
			drawgfx(bitmap, Machine->gfx[bank],
					LOOKUP_IMAGE(lookup) | (pict & 0xff), 0x30,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_PEN, 0);

			/* also draw the sprite normally on the temp bitmap */
			drawgfx(tempbitmap, Machine->gfx[bank],
					LOOKUP_IMAGE(lookup) | (pict & 0xff), 0x20,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_NONE, 0);
		}

		/* now redraw the playfield tiles over top of the sprite */
		redraw_playfield_chunk(tempbitmap, xpos, ypos, 1, vsize, TRANSPARENCY_PEN, 0, -1);

		/* finally, copy this chunk to the real bitmap */
		tclip.min_x = xpos;
		tclip.max_x = xpos + 7;
		tclip.min_y = ypos;
		tclip.max_y = ypos + vsize * 8 - 1;
		if (tclip.min_y < clip->min_y) tclip.min_y = clip->min_y;
		if (tclip.max_y > clip->max_y) tclip.max_y = clip->max_y;
		copybitmap(bitmap, tempbitmap, 0, 0, 0, 0, &tclip, TRANSPARENCY_THROUGH, palette_transparent_pen);
	}
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


/*
 *   playfield redraw function
 */

static void redraw_playfield_chunk(struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int transparency, int transparency_color, int type)
{
	struct rectangle clip;
	int y, x;

	/* make a clip */
	clip.min_x = xpos;
	clip.max_x = xpos + w * 8 - 1;
	clip.min_y = ypos;
	clip.max_y = ypos + h * 8 - 1;

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
				int color = LOOKUP_COLOR(data);

				/* draw */
				if (type == -1)
					drawgfx(bitmap, Machine->gfx[LOOKUP_GFX(data)], LOOKUP_IMAGE(data), color, LOOKUP_FLIP(data), 0,
							sx, sy, &clip, transparency, transparency_color);
				else if (type == -2)
				{
					int bpp = LOOKUP_BPP(data);
					if (color == (16 >> bpp))
						drawgfx(bitmap, Machine->gfx[LOOKUP_GFX(data)], LOOKUP_IMAGE(data), color, LOOKUP_FLIP(data), 0,
								sx, sy, &clip, transparency, transparency_color);
				}
				else
					drawgfx(bitmap, Machine->gfx[LOOKUP_GFX(data)], LOOKUP_IMAGE(data), type, LOOKUP_FLIP(data), 0,
							sx, sy, &clip, transparency, transparency_color);
			}
		}
	}
}



/*************************************
 *
 *		RoadBlasters-specific bitmap copy
 *
 *************************************/

void copy_roadblst_bitmap(struct osd_bitmap *bitmap)
{
	int lasty, y, offs, rows;
	struct rectangle clip;
	int *scroll;

	lasty = -1;
	scroll = (int *)scrolllist;

	/* finish the scrolling list from the previous frame */
	xscroll = READ_WORD(&atarigen_hscroll[0]) & 0x1ff;
	yscroll = READ_WORD(&atarigen_vscroll[0]) & 0x1ff;
	offs = (xscroll << 12) + yscroll;
	for (y = scrolllist_end; y < YDIM; y++)
		scrolllist[y] = offs;

	/* initialize the cliprect */
	clip.min_x = 0;
	clip.max_x = XDIM;

	/* loop over and copy the data row by row */
	for (y = rows = 0; y < YDIM; y += rows)
	{
		int scrollval = *scroll++;
		int scrollx, scrolly;
		int tempy;

		/* see how many rows share this value */
		for (tempy = y + 1, rows = 1; tempy < YDIM; tempy++, rows++, scroll++)
			if (*scroll != scrollval)
				break;

		/* extract X/Y scroll values */
		scrollx = (scrollval >> 12) & 0x1ff;
		scrolly = scrollval & 0x1ff;

		/* when a write to the scroll register occurs, the counter is reset */
		if (scrolly != lasty)
		{
			offs = y;
			lasty = scrolly;
		}

		/* compute final scroll values */
		scrollx = -(scrollx & 0x1ff);
		scrolly = -((scrolly - offs) & 0x1ff);

		/* copy the bitmap piece */
		clip.min_y = y;
		clip.max_y = y + rows;
		copyscrollbitmap(bitmap, playfieldbitmap, 1, &scrollx, 1, &scrolly, &clip, TRANSPARENCY_NONE, 0);
	}
}


/*************************************
 *
 *		Generic System 1 palette
 *
 *************************************/

static void atarisys1_update_palette(int xstart, int xend, int ystart, int yend)
{
	unsigned char mo_map[3 * 32], al_map[8];
	unsigned short pf_map[32];
	int x, y, sx, sy, offs, i;

	/* reset color tracking */
	memset(mo_map, 0, sizeof(mo_map));
	memset(pf_map, 0, sizeof(pf_map));
	memset(al_map, 0, sizeof(al_map));
	palette_init_used_colors();

	/* assign our special pen here */
	memset(&palette_used_colors[1024], PALETTE_COLOR_TRANSPARENT, 16);

	/* loop over the visible Y region */
	for (y = yend; y >= ystart; y--)
	{
		sy = y & 63;

		/* loop over the visible X region */
		for (x = xend; x >= xstart; x--)
		{
			int data, bank, color, bpp, image;
			int usage;

			/* read the data word */
			sx = x & 63;
			offs = sy * 64 + sx;
			data = pfmapped[offs];

			/* set the appropriate palette entries */
			bpp = LOOKUP_BPP(data);
			bank = LOOKUP_GFX(data);
			image = LOOKUP_IMAGE(data);
			color = LOOKUP_COLOR(data);

			/* based on the depth, we need to tweak our pen mapping */
			if (bpp == 0)
				pf_map[color] |= pen_usage[bank][image];
			else if (bpp == 1)
			{
				usage = pen_usage[bank][image];
				pf_map[color * 2] |= usage;
				pf_map[color * 2 + 1] |= usage >> 16;
			}
			else
			{
				usage = pen_usage[bank][image * 2];
				pf_map[color * 4] |= usage;
				pf_map[color * 4 + 1] |= usage >> 16;
				usage = pen_usage[bank][image * 2 + 1];
				pf_map[color * 4 + 2] |= usage;
				pf_map[color * 4 + 3] |= usage >> 16;
			}
		}
	}

	/* update color usage for the mo's */
	atarigen_render_display_list(NULL, atarisys1_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD(&atarigen_alpharam[offs * 2]);
			int color = (data >> 10) & 7;
			al_map[color] = 1;
		}

	/* determine the final playfield palette */
	for (i = 0; i < 32; i++)
	{
		unsigned short usage = pf_map[i];
		if (usage)
		{
			int j;
			for (j = 0; j < 16; j++)
				if (usage & (1 << j))
					palette_used_colors[256 + i * 16 + j] = PALETTE_COLOR_USED;
		}
	}

	/* determine the final motion object palette */
	for (i = 0; i < 8; i++)
	{
		if (mo_map[2 * 32 + i])
		{
			palette_used_colors[256 + i * 64] = PALETTE_COLOR_TRANSPARENT;
			memset(&palette_used_colors[257 + i * 64], PALETTE_COLOR_USED, 63);
		}
		else
		{
			if (mo_map[1 * 32 + 2 * i])
			{
				palette_used_colors[256 + i * 64] = PALETTE_COLOR_TRANSPARENT;
				memset(&palette_used_colors[257 + i * 64], PALETTE_COLOR_USED, 31);
			}
			else
			{
				if (mo_map[0 * 32 + 4 * i])
				{
					palette_used_colors[256 + i * 64] = PALETTE_COLOR_TRANSPARENT;
					memset(&palette_used_colors[257 + i * 64], PALETTE_COLOR_USED, 15);
				}
				if (mo_map[0 * 32 + 4 * i + 1])
				{
					palette_used_colors[256 + i * 64 + 16] = PALETTE_COLOR_TRANSPARENT;
					memset(&palette_used_colors[257 + i * 64 + 16], PALETTE_COLOR_USED, 15);
				}
			}
			if (mo_map[1 * 32 + 2 * i + 1])
			{
				palette_used_colors[256 + i * 64 + 32] = PALETTE_COLOR_TRANSPARENT;
				memset(&palette_used_colors[257 + i * 64 + 32], PALETTE_COLOR_USED, 31);
			}
			else
			{
				if (mo_map[0 * 32 + 4 * i + 2])
				{
					palette_used_colors[256 + i * 64 + 32] = PALETTE_COLOR_TRANSPARENT;
					memset(&palette_used_colors[257 + i * 64 + 32], PALETTE_COLOR_USED, 15);
				}
				if (mo_map[0 * 32 + 4 * i + 3])
				{
					palette_used_colors[256 + i * 64 + 48] = PALETTE_COLOR_TRANSPARENT;
					memset(&palette_used_colors[257 + i * 64 + 48], PALETTE_COLOR_USED, 15);
				}
			}
		}
	}

	/* determine the final alpha palette */
	for (i = 0; i < 8; i++)
		if (al_map[i])
			memset(&palette_used_colors[0 + i * 4], PALETTE_COLOR_USED, 4);

	/* always remap the transluscent colors (except for RoadBlasters, which we ignore) */
	if (!roadblst_screen_refresh)
		memset(&palette_used_colors[768], PALETTE_COLOR_USED, 16);

	/* recalculate the palette */
	if (palette_recalc())
		for (i = atarigen_playfieldram_size / 2 - 1; i >= 0; i--)
			pfmapped[i] |= LOOKUP_DIRTY_FLAG;
}


/*************************************
 *
 *		Generic System 1 refresh
 *
 *************************************/

void atarisys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x, y, sx, sy, xoffs, yoffs, xend, yend, offs, *r;
	struct atarisys1_mo_data modata;
	int redraw_list[1024];


	/* load the scroll values from the start of the previous frame */
	if (scrolllist_end)
	{
		xscroll = scrolllist[0] >> 12;
		yscroll = scrolllist[0];
	}
	else
	{
		xscroll = READ_WORD(&atarigen_hscroll[0]);
		yscroll = READ_WORD(&atarigen_vscroll[0]);
	}
	xscroll = -(xscroll & 0x1ff);
	yscroll = -(yscroll & 0x1ff);


	/* determine the portion of the playfield that might be visible */
	if (roadblst_screen_refresh)
	{
		xoffs = yoffs = 0;
		xend = yend = 63;
	}
	else
	{
		xoffs = (-xscroll / 8);
		yoffs = (-yscroll / 8);
		xend = xoffs + XCHARS + 1;
		yend = yoffs + YCHARS + 1;
	}

	/* update the palette */
	atarisys1_update_palette(0, 63, 0, 63);
	/*atarisys1_update_palette(xoffs, xend, yoffs, yend); <--- for some reason, this breaks Road Runner */



	/*
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

	/* loop over the visible Y region */
	for (y = yend; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X region */
		for (x = xend; x >= xoffs; x--)
		{
			int data;

			/* read the data word */
			sx = x & 63;
			offs = sy * 64 + sx;
			data = pfmapped[offs];

			/* rerender if dirty */
			if (LOOKUP_DIRTY(data))
			{
				int bank = LOOKUP_GFX(data);
				if (bank)
					drawgfx(playfieldbitmap, Machine->gfx[bank], LOOKUP_IMAGE(data), LOOKUP_COLOR(data), LOOKUP_FLIP(data), 0,
							8*sx, 8*sy, 0, TRANSPARENCY_NONE, 0);
				pfmapped[offs] = data & ~LOOKUP_DIRTY_FLAG;
			}
		}
	}

	/* copy the playfield to the destination */
	if (roadblst_screen_refresh)
	{
		copy_roadblst_bitmap(bitmap);
		modata.pcolor = 0;
	}
	else
	{
		copyscrollbitmap(bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
		modata.pcolor = READ_WORD(&atarisys1_prioritycolor[0]) & 0xff;
	}

	/* prepare the motion object data structure */
	modata.pcolor = roadblst_screen_refresh ? 0 : READ_WORD(&atarisys1_prioritycolor[0]) & 0xff;
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list(bitmap, atarisys1_render_mo, &modata);

	/* redraw playfield tiles with higher priority */
	if (!roadblst_screen_refresh)
		for (r = modata.redraw_list; r < modata.redraw; r++)
		{
			int val = *r;
			int xpos = (val >> 23) & 0x1ff;
			int ypos = (val >> 14) & 0x1ff;
			int h = val & 0x1f;

			redraw_playfield_chunk(bitmap, xpos, ypos, 1, h, TRANSPARENCY_PENS, ~modata.pcolor, -2);
		}

	/*
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

	/* redraw the alpha layer completely */
	for (sy = 0; sy < YCHARS; sy++)
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD(&atarigen_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x2000))
			{
				int color = ((data >> 10) & 7);

				drawgfx(bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x2000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#if 0
static int atarisys1_debug(void)
{
	static int lasttrans[0x200];
	int hidebank = -1;

	if (memcmp(lasttrans, &paletteram[0x600], 0x200))
	{
		static FILE *trans;
		int i;

		if (!trans) trans = fopen("trans.log", "w");
		if (trans)
		{
			fprintf(trans, "\n\nTrans Palette:\n");
			for (i = 0x300; i < 0x400; i++)
			{
				fprintf(trans, "%04X ", READ_WORD(&paletteram[i*2]));
				if ((i & 15) == 15) fprintf(trans, "\n");
			}
		}
		memcpy(lasttrans, &paletteram[0x600], 0x200);
	}

	if (osd_key_pressed(OSD_KEY_Q)) hidebank = 0;
	if (osd_key_pressed(OSD_KEY_W)) hidebank = 1;
	if (osd_key_pressed(OSD_KEY_E)) hidebank = 2;
	if (osd_key_pressed(OSD_KEY_R)) hidebank = 3;
	if (osd_key_pressed(OSD_KEY_T)) hidebank = 4;
	if (osd_key_pressed(OSD_KEY_Y)) hidebank = 5;
	if (osd_key_pressed(OSD_KEY_U)) hidebank = 6;
	if (osd_key_pressed(OSD_KEY_I)) hidebank = 7;

	if (osd_key_pressed(OSD_KEY_A)) hidebank = 8;
	if (osd_key_pressed(OSD_KEY_S)) hidebank = 9;
	if (osd_key_pressed(OSD_KEY_D)) hidebank = 10;
	if (osd_key_pressed(OSD_KEY_F)) hidebank = 11;
	if (osd_key_pressed(OSD_KEY_G)) hidebank = 12;
	if (osd_key_pressed(OSD_KEY_H)) hidebank = 13;
	if (osd_key_pressed(OSD_KEY_J)) hidebank = 14;
	if (osd_key_pressed(OSD_KEY_K)) hidebank = 15;

	if (osd_key_pressed(OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i, bank;

		while (osd_key_pressed(OSD_KEY_9)) { }

		sprintf(name, "Dump %d", ++count);
		f = fopen(name, "wt");

		fprintf(f, "\n\nAlpha Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nMotion Object Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nPlayfield Palette:\n");
		for (i = 0x200; i < 0x300; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nTrans Palette:\n");
		for (i = 0x300; i < 0x400; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		bank = (READ_WORD(&atarisys1_bankselect[0]) >> 3) & 7;
		fprintf(f, "\n\nMotion Object Bank = %d\n", bank);
		bank *= 0x200;
		for (i = 0; i < 0x40; i++)
		{
			fprintf(f, "   Object %02X:  P=%04X  X=%04X  Y=%04X  L=%04X\n",
					i,
					READ_WORD(&atarigen_spriteram[bank+0x080+i*2]),
					READ_WORD(&atarigen_spriteram[bank+0x100+i*2]),
					READ_WORD(&atarigen_spriteram[bank+0x000+i*2]),
					READ_WORD(&atarigen_spriteram[bank+0x180+i*2])
			);
		}

		fprintf(f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf(f, "\n");
		}

		fclose(f);
	}

	return hidebank;
}
#endif
