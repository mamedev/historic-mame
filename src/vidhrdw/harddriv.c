/***************************************************************************

	Hard Drivin' video hardware

****************************************************************************/

#include "vidhrdw/generic.h"
#include "cpu/tms34010/tms34010.h"



/*************************************
 *
 *	Constants and macros
 *
 *************************************/

#define ENABLE_RASTERIZER_OPTS		1

#ifdef LSB_FIRST
#define MASK(n)			(0x000000ffUL << ((n) * 8))
#else
#define MASK(n)			(0xff000000UL >> (((n) ^ 1) * 8))
#endif


struct gfx_update_entry
{
	int scanline;
	UINT8 palettebank;
	UINT8 blank;
	INT8 finescroll;
	offs_t offset;
	offs_t rowbytes;
};



/*************************************
 *
 *	External definitions
 *
 *************************************/

/* externally accessible */
UINT8 hdgsp_multisync;
UINT8 *hdgsp_vram;
data16_t *hdgsp_vram_2bpp;
data16_t *hdgsp_control_lo;
data16_t *hdgsp_control_hi;
data16_t *hdgsp_paletteram_lo;
data16_t *hdgsp_paletteram_hi;
size_t hdgsp_vram_size;



/*************************************
 *
 *	Static globals
 *
 *************************************/

static offs_t vram_mask;

static UINT8 shiftreg_enable;
static UINT32 shiftreg_count;

static int cycles_to_eat;
static UINT32 *mask_table;
static UINT8 *gsp_shiftreg_source;

static struct gfx_update_entry gfx_update_list[256];
static struct gfx_update_entry curr_state;
static int gfx_update_index;


static void harddriv_fast_draw(void);
static void stunrun_fast_draw(void);



/*************************************
 *
 *	Start/stop routines
 *
 *************************************/

int harddriv_vh_start(void)
{
	UINT32 *destmask, mask;
	int i;

	shiftreg_enable = 0;
	shiftreg_count = 512*8 >> hdgsp_multisync;
	memset(&curr_state, 0, sizeof(curr_state));
	
	gfx_update_index = 0;
	
	/* allocate the mask table */
	mask_table = malloc(sizeof(UINT32) * 4 * 65536);
	if (!mask_table)
		return 1;
	
	/* fill in the mask table */
	destmask = mask_table;
	for (i = 0; i < 65536; i++)
		if (hdgsp_multisync)
		{
			mask = 0;
			if (i & 0x0003) mask |= MASK(0);
			if (i & 0x000c) mask |= MASK(1);
			if (i & 0x0030) mask |= MASK(2);
			if (i & 0x00c0) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x0300) mask |= MASK(0);
			if (i & 0x0c00) mask |= MASK(1);
			if (i & 0x3000) mask |= MASK(2);
			if (i & 0xc000) mask |= MASK(3);
			*destmask++ = mask;
		}
		else
		{
			mask = 0;
			if (i & 0x0001) mask |= MASK(0);
			if (i & 0x0002) mask |= MASK(1);
			if (i & 0x0004) mask |= MASK(2);
			if (i & 0x0008) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x0010) mask |= MASK(0);
			if (i & 0x0020) mask |= MASK(1);
			if (i & 0x0040) mask |= MASK(2);
			if (i & 0x0080) mask |= MASK(3);
			*destmask++ = mask;
		
			mask = 0;
			if (i & 0x0100) mask |= MASK(0);
			if (i & 0x0200) mask |= MASK(1);
			if (i & 0x0400) mask |= MASK(2);
			if (i & 0x0800) mask |= MASK(3);
			*destmask++ = mask;
		
			mask = 0;
			if (i & 0x1000) mask |= MASK(0);
			if (i & 0x2000) mask |= MASK(1);
			if (i & 0x4000) mask |= MASK(2);
			if (i & 0x8000) mask |= MASK(3);
			*destmask++ = mask;
		}
	
	/* init VRAM pointers */
	vram_mask = hdgsp_vram_size - 1;
	
	return 0;
}


void harddriv_vh_stop(void)
{
	if (mask_table)
		free(mask_table);
	mask_table = NULL;
}



/*************************************
 *
 *	Update list management
 *
 *************************************/

INLINE struct gfx_update_entry *init_gfx_update(int scanline)
{
	struct gfx_update_entry *entry;

	/* if we get an entry on the same scanline as the last one, just install over top of it */
	if (gfx_update_index != 0 && gfx_update_list[gfx_update_index - 1].scanline == scanline)
		entry = &gfx_update_list[gfx_update_index - 1];
	else
		entry = &gfx_update_list[gfx_update_index++];
	
	/* set the scanline while we're here */
	*entry = curr_state;
	entry->scanline = scanline;
	entry->blank = tms34010_io_display_blanked(1);
	return entry;
}



/*************************************
 *
 *	Shift register access
 *
 *************************************/

void hdgsp_write_to_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	/* access to the 1bpp/2bpp area */
	if (address >= 0x02000000 && address <= 0x020fffff)
	{
		address -= 0x02000000;
		address >>= hdgsp_multisync;
		address &= vram_mask;
		address &= ~((512*8 >> hdgsp_multisync) - 1);
		gsp_shiftreg_source = &hdgsp_vram[address];
	}

	/* access to normal VRAM area */
	else if (address >= 0xff800000 && address <= 0xffffffff)
	{
		address -= 0xff800000;
		address /= 8;
		address &= hdgsp_vram_size - 1;
		address &= ~511;
		gsp_shiftreg_source = &hdgsp_vram[address];
	}
}


void hdgsp_read_from_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	if (!shiftreg_enable)
		return;

	/* access to the 1bpp/2bpp area */
	if (address >= 0x02000000 && address <= 0x020fffff)
	{
		address -= 0x02000000;
		address >>= hdgsp_multisync;
		address &= vram_mask;
		address &= ~((512*8 >> hdgsp_multisync) - 1);
		memcpy(&hdgsp_vram[address], gsp_shiftreg_source, 512*8 >> hdgsp_multisync);
	}

	/* access to normal VRAM area */
	else if (address >= 0xff800000 && address <= 0xffffffff)
	{
		address -= 0xff800000;
		address /= 8;
		address &= hdgsp_vram_size - 1;
		address &= ~511;
		memcpy(&hdgsp_vram[address], gsp_shiftreg_source, 512);
	}
}



/*************************************
 *
 *	TMS34010 update callback
 *
 *************************************/

void hdgsp_display_update(UINT32 offs, int rowbytes, int scanline)
{
	struct gfx_update_entry *entry;
	
	/* update the screen to the current scanline */
	entry = init_gfx_update(scanline);
	entry->offset = curr_state.offset = offs >> hdgsp_multisync;
	entry->rowbytes = curr_state.rowbytes = rowbytes >> hdgsp_multisync;
}



/*************************************
 *
 *	Palette bank updating
 *
 *************************************/

static void update_palette_bank(int newbank)
{
	struct gfx_update_entry *entry;
	
	/* bail if nothing new */
	if (newbank == curr_state.palettebank)
		return;
	
	/* update with the current palette */
	entry = init_gfx_update(cpu_getscanline());
	entry->palettebank = curr_state.palettebank = newbank;
}



/*************************************
 *
 *	Video control registers (lo)
 *
 *************************************/

READ16_HANDLER( hdgsp_control_lo_r )
{
	return hdgsp_control_lo[offset];
}


WRITE16_HANDLER( hdgsp_control_lo_w )
{
	int oldword = hdgsp_control_lo[offset];
	int newword;
	
	COMBINE_DATA(&hdgsp_control_lo[offset]);
	newword = hdgsp_control_lo[offset];

#if ENABLE_RASTERIZER_OPTS
	if (offset == 0)
#else
	if (0)
#endif
	{
		switch (cpu_get_pc())
		{
			case 0xffc05700:
				harddriv_fast_draw();
				break;
				
			case 0xfff45360:
				stunrun_fast_draw();
				break;
		}
/*		logerror("Color @ %08X\n", cpu_get_pc());*/
	}
	
	if (oldword != newword && offset != 0)
		logerror("GSP:hdgsp_control_lo(%X)=%04X\n", offset, newword);
}



/*************************************
 *
 *	Video control registers (hi)
 *
 *************************************/

READ16_HANDLER( hdgsp_control_hi_r )
{
	return hdgsp_control_hi[offset];
}


WRITE16_HANDLER( hdgsp_control_hi_w )
{
	struct gfx_update_entry *entry;
	int val = (offset >> 3) & 1;

	int oldword = hdgsp_control_hi[offset];
	int newword;
	
	COMBINE_DATA(&hdgsp_control_hi[offset]);
	newword = hdgsp_control_hi[offset];

	switch (offset & 7)
	{
		case 0x00:
			shiftreg_enable = val;
			break;
		
		case 0x01:
			data &= 15;
			if (curr_state.finescroll != data)
			{
				entry = init_gfx_update(cpu_getscanline());
				entry->finescroll = curr_state.finescroll = data;
			}
			break;
			
		case 0x02:
			update_palette_bank((curr_state.palettebank & ~1) | val);
			break;
		
		case 0x03:
			update_palette_bank((curr_state.palettebank & ~2) | (val << 1));
			break;
		
		case 0x04:
			if (Machine->drv->total_colors >= 256 * 8)
				update_palette_bank((curr_state.palettebank & ~4) | (val << 2));
			break;
		
		default:
			if (oldword != newword)
				logerror("GSP:video_control_hi(%X)=%04X\n", offset / 2, newword);
			break;
	}
}



/*************************************
 *
 *	Video RAM expanders
 *
 *************************************/

READ16_HANDLER( hdgsp_vram_2bpp_r )
{
	return hdgsp_vram_2bpp[offset];
}


WRITE16_HANDLER( hdgsp_vram_1bpp_w )
{
	int newword = hdgsp_vram_2bpp[offset];
	COMBINE_DATA(&newword);

	if (newword)
	{
		UINT32 *dest = (UINT32 *)&hdgsp_vram[offset * 16];
		UINT32 *mask = &mask_table[newword * 4];
		UINT32 color = hdgsp_control_lo[0] & 0xff;
		UINT32 curmask;
		
		color |= color << 8;
		color |= color << 16;
		
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	}
}


WRITE16_HANDLER( hdgsp_vram_2bpp_w )
{
	int newword = hdgsp_vram_2bpp[offset];
	COMBINE_DATA(&newword);

	if (newword)
	{
		UINT32 *dest = (UINT32 *)&hdgsp_vram[offset * 8];
		UINT32 *mask = &mask_table[newword * 2];
		UINT32 color = hdgsp_control_lo[0] & 0xff;
		UINT32 curmask;
		
		color |= color << 8;
		color |= color << 16;
		
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	
		curmask = *mask++;		
		*dest = (*dest & ~curmask) | (color & curmask);
		dest++;
	}
}



/*************************************
 *
 *	Palette registers (lo)
 *
 *************************************/

INLINE void gsp_palette_change(int offset)
{
	int red = (hdgsp_paletteram_lo[offset] >> 8) & 0xff;
	int green = hdgsp_paletteram_lo[offset] & 0xff;
	int blue = hdgsp_paletteram_hi[offset] & 0xff;
	palette_change_color(offset, red, green, blue);
}


READ16_HANDLER( hdgsp_paletteram_lo_r )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = curr_state.palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_lo[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_lo_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = curr_state.palettebank * 0x100 + (offset & 0xff);

	COMBINE_DATA(&hdgsp_paletteram_lo[offset]);
	gsp_palette_change(offset);
}



/*************************************
 *
 *	Palette registers (hi)
 *
 *************************************/

READ16_HANDLER( hdgsp_paletteram_hi_r )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = curr_state.palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_hi[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_hi_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = curr_state.palettebank * 0x100 + (offset & 0xff);

	COMBINE_DATA(&hdgsp_paletteram_hi[offset]);
	gsp_palette_change(offset);
}



/*************************************
 *
 *	End of frame routine
 *
 *************************************/

static void init_draw_state(int param)
{
	gfx_update_index = 0;
	init_gfx_update(0);
}


void harddriv_vh_eof(void)
{
	/* reset the partial drawing */
	timer_set(cpu_getscanlinetime(0), 0, init_draw_state);
}



/*************************************
 *
 *	Core refresh routine
 *
 *************************************/

void harddriv_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	struct gfx_update_entry *draw_state;
	UINT8 palettes_used[8];
	UINT32 curr_offset = 0;
	int i;
	
	/* mark any pens used and recalc the palette */
	palette_init_used_colors();
	palettes_used[0] = palettes_used[1] = palettes_used[2] = palettes_used[3] = 0;
	palettes_used[4] = palettes_used[5] = palettes_used[6] = palettes_used[7] = 0;
	for (i = 0; i < gfx_update_index; i++)
		palettes_used[gfx_update_list[i].palettebank] = 1;
	for (i = 0; i < 8; i++)
		if (palettes_used[i])
			memset(&palette_used_colors[i * 256], PALETTE_COLOR_USED, 256);
	palette_recalc();
	
	/* make a final entry in the list */
	init_gfx_update(Machine->visible_area.max_y);

	/* redraw the screen */
	for (i = 0, draw_state = &gfx_update_list[0]; i < gfx_update_index - 1; i++, draw_state++)
	{
		UINT16 *pens = &Machine->pens[draw_state->palettebank * 256];
		int x, y, width = Machine->drv->screen_width;
		int sy = draw_state[0].scanline;
		int ey = draw_state[1].scanline - 1;
		
		/* make sure things stay in bounds */
		if (sy < Machine->visible_area.min_y)
			sy = Machine->visible_area.min_y;
		if (ey > Machine->visible_area.max_y)
			ey = Machine->visible_area.max_y;
		
		/* check for disabled video */
		if (draw_state->blank)
		{
			struct rectangle clip = Machine->visible_area;
			clip.min_y = sy;
			clip.max_y = ey;
			fillbitmap(bitmap, pens[0], &clip);
		}
		
		/* copy video data */
		else
		{
			/* loop over scanlines */
			for (y = sy; y <= ey; y++)
			{
				UINT32 offset = draw_state->offset + curr_offset + ((draw_state->finescroll + 1) & 15);
				UINT16 *source = (UINT16 *)&hdgsp_vram[offset & vram_mask & ~1];
				
				/* 16-bit case */
				if (bitmap->depth == 16)
				{
					UINT16 *dest = (UINT16 *)bitmap->line[y];

					/* if we're on an even pixel boundary, it's easy */
					if ((offset & 1) == 0)
					{
						for (x = 0; x < width; x += 2)
						{
							int temp = *source++;
							*dest++ = pens[temp & 0xff];
							*dest++ = pens[temp >> 8];
						}
					}
					
					/* if we're on an odd pixel boundary, handle the edge cases */
					else
					{
						int temp = *source++;
						*dest++ = pens[temp >> 8];
						for (x = 2; x < width; x += 2)
						{
							temp = *source++;
							*dest++ = pens[temp & 0xff];
							*dest++ = pens[temp >> 8];
						}
						temp = *source++;
						*dest++ = pens[temp & 0xff];
					}
				}
				
				/* 8-bit case */
				else
				{
					UINT8 *dest = bitmap->line[y];

					/* if we're on an even pixel boundary, it's easy */
					if ((offset & 1) == 0)
					{
						for (x = 0; x < width; x += 2)
						{
							int temp = *source++;
							*dest++ = pens[temp & 0xff];
							*dest++ = pens[temp >> 8];
						}
					}
					
					/* if we're on an odd pixel boundary, handle the edge cases */
					else
					{
						int temp = *source++;
						*dest++ = pens[temp >> 8];
						for (x = 2; x < width; x += 2)
						{
							temp = *source++;
							*dest++ = pens[temp & 0xff];
							*dest++ = pens[temp >> 8];
						}
						temp = *source++;
						*dest++ = pens[temp & 0xff];
					}
				}
				
				/* advance to the next row */
				curr_offset = (curr_offset + draw_state->rowbytes) & vram_mask;
			}
		}
				
		/* if the base changed, reset the offset */
		if (draw_state[1].offset != draw_state[0].offset)
			curr_offset = 0;
	}
}



/*************************************
 *
 *	Rasterizer optimizations
 *
 *************************************/

#define FILL_SCANLINE(BPP)													\
	if (offset >= 0 && offset < 0x80000 && cury >= tclip && cury <= bclip) 	\
	{																		\
		UINT8 *dst = &hdgsp_vram[offset];									\
		sx = lx;															\
		ex = rx;															\
		if (sx < lclip) sx = lclip;											\
		if (ex > rclip) ex = rclip + 1;										\
		if (ex > sx)														\
		{																	\
			if (pattern == 0xffffffff)										\
			{																\
				for (x = sx; x < ex; x++)									\
					dst[BYTE_XOR_LE(x)] = color;							\
			}																\
			else															\
			{																\
				for (x = sx; x < ex; x++)									\
					if (pattern & (((1 << BPP) - 1) << ((x * BPP) & 31)))	\
						dst[BYTE_XOR_LE(x)] = color;						\
			}																\
			cycles_to_eat += 11 + 2 * (ex - sx) / (8 / BPP);				\
		}																	\
	}


#define EAT_CYCLES()														\
	if (cycles_to_eat < tms34010_ICount)									\
		tms34010_ICount -= cycles_to_eat, cycles_to_eat = 0;				\
	else																	\
		cycles_to_eat -= tms34010_ICount, tms34010_ICount = 0;


static void harddriv_fast_draw(void)
{
	UINT32 a5 = cpu_get_reg(TMS34010_A5);
	UINT8 *data = &hdgsp_vram[TOBYTE(a5) & vram_mask];
	UINT32 rowbytes = cpu_get_reg(TMS34010_B3);
	UINT32 offset = cpu_get_reg(TMS34010_B4);
	UINT32 b5 = cpu_get_reg(TMS34010_B5);
	UINT32 b6 = cpu_get_reg(TMS34010_B6);
	UINT32 pattern = cpu_get_reg(TMS34010_B9);
	UINT32 lx,rx,lfrac,rfrac,rdelta=0,ldelta=0,count,cury,b8;
	int lclip = (INT16)b5, rclip = (INT16)b6;
	int tclip = (INT32)b5 >> 16, bclip = (INT32)b6 >> 16;
	int color = hdgsp_control_lo[0];
	int sx,ex,x;
	
	if (offset < 0x01f80000 || offset >= 0x02080000)
		return;
	offset -= 0x02000000;
	
	lx = (INT16)READ_WORD(data); data += 2;
	rx = (INT16)READ_WORD(data) + 1; data += 2;

	cury = (INT16)READ_WORD(data); data += 2;
	b8 = (cury << 1) & 31;
	pattern = (pattern << b8) | (pattern >> (32 - b8));
	lfrac = rfrac = 0x8000;
	offset += rowbytes * cury;

	FILL_SCANLINE(1)
	
	/* handle wireframe case */
	if (READ_WORD(&hdgsp_vram[TOBYTE(0xffc36e40) & vram_mask]) ||
		(READ_WORD(&hdgsp_vram[TOBYTE(0xffc3d580) & vram_mask]) & 8))
	{
		while (1)
		{
			rdelta = READ_WORD(data); rdelta |= READ_WORD(data + 2) << 16; data += 4;
			if (rdelta == 0xffffffff)
			{
				FILL_SCANLINE(1)

				cpu_set_reg(TMS34010_A5, ((data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
				cpu_set_reg(TMS34010_PC, cpu_get_pc() + 0xffc05c80 - 0xffc05700);
				EAT_CYCLES();
				return;
			}
			ldelta = READ_WORD(data); ldelta |= READ_WORD(data + 2) << 16; data += 4;
			count = READ_WORD(data); data += 2;
	
			while (--count)
			{
				cury++;
				
				lfrac += ldelta;
				lx += (INT32)lfrac >> 16;
				lfrac &= 0xffff;
				
				rfrac += rdelta;
				rx += (INT32)rfrac >> 16;
				rfrac &= 0xffff;
				
				if (offset >= 0 && offset < 0x80000 && cury >= tclip && cury <= bclip)
				{
					UINT8 *dst = &hdgsp_vram[offset];
					
					if (lx >= lclip && lx <= rclip)
						if (pattern & (1 << (lx & 31)))
							dst[lx] = color;
					if (rx >= lclip && rx <= rclip)
						if (pattern & (1 << (rx & 31)))
							dst[rx] = color;
				}
				offset += rowbytes;
				pattern = (pattern << 2) | (pattern >> 30);
			}
		}
	}
	
	/* handle non-wireframe case */
	else
	{
		while (1)
		{
			rdelta = READ_WORD(data); rdelta |= READ_WORD(data + 2) << 16; data += 4;
			if (rdelta == 0xffffffff)
			{
				cpu_set_reg(TMS34010_A5, ((data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
				cpu_set_reg(TMS34010_PC, cpu_get_pc() + 0xffc05920 - 0xffc05700);
				EAT_CYCLES();
				return;
			}
			ldelta = READ_WORD(data); ldelta |= READ_WORD(data + 2) << 16; data += 4;
			count = READ_WORD(data); data += 2;
	
			while (--count)
			{
				cury++;
				
				lfrac += ldelta;
				lx += (INT32)lfrac >> 16;
				lfrac &= 0xffff;
				
				rfrac += rdelta;
				rx += (INT32)rfrac >> 16;
				rfrac &= 0xffff;
				
				FILL_SCANLINE(1)

				offset += rowbytes;
				pattern = (pattern << 2) | (pattern >> 30);
			}
		}
	}
}


static void stunrun_fast_draw(void)
{
	UINT32 a5 = cpu_get_reg(TMS34010_A5);
	UINT8 *data = &hdgsp_vram[TOBYTE(a5) & vram_mask];
	UINT32 rx = (INT16)cpu_get_reg(TMS34010_A6) + 1;
	UINT32 lx = (INT16)cpu_get_reg(TMS34010_B0);
	UINT32 rowbytes = cpu_get_reg(TMS34010_B3) >> 1;
	UINT32 offset = cpu_get_reg(TMS34010_B4);
	UINT32 b5 = cpu_get_reg(TMS34010_B5);
	UINT32 b6 = cpu_get_reg(TMS34010_B6);
	UINT32 pattern = cpu_get_reg(TMS34010_B9);
	UINT32 lfrac,rfrac,rdelta=0,ldelta=0,count,cury,b8;
	int lclip = (INT16)b5, rclip = (INT16)b6;
	int tclip = (INT32)b5 >> 16, bclip = (INT32)b6 >> 16;
	int color = hdgsp_control_lo[0];
	int sx,ex,x;
	
	if (offset < 0x01f80000 || offset >= 0x02080000)
		return;
	offset = (INT32)(offset - 0x02000000) >> 1;

	cury = (INT16)READ_WORD(data) + 1; data += 2;
	b8 = (cury << 2) & 31;
	pattern = (pattern << b8) | (pattern >> (32 - b8));
	lfrac = rfrac = 0x8000;
	offset += rowbytes * cury;

	FILL_SCANLINE(2)

	while (1)
	{
		rdelta = READ_WORD(data); rdelta |= READ_WORD(data + 2) << 16; data += 4;
		if (rdelta == 0xffffffff)
		{
			cpu_set_reg(TMS34010_A5, ((data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
			cpu_set_reg(TMS34010_PC, cpu_get_pc() + 0xfff45630 - 0xfff45360);
			EAT_CYCLES();
			return;
		}
		ldelta = READ_WORD(data); ldelta |= READ_WORD(data + 2) << 16; data += 4;
		count = READ_WORD(data); data += 2;

		while (--count)
		{
			cury++;
			
			lfrac += ldelta;
			lx += (INT32)lfrac >> 16;
			lfrac &= 0xffff;
			
			rfrac += rdelta;
			rx += (INT32)rfrac >> 16;
			rfrac &= 0xffff;
			
			FILL_SCANLINE(2)

			offset += rowbytes;
			pattern = (pattern << 4) | (pattern >> 28);
		}
	}
}
