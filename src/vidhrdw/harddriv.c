/***************************************************************************

	Hard Drivin' video hardware

****************************************************************************/

#include "vidhrdw/generic.h"
#include "cpu/tms34010/tms34010.h"
#include "harddriv.h"



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

static offs_t gfx_offset;
static offs_t gfx_rowbytes;
static int gfx_offsetscan;
static INT8 gfx_finescroll;
static UINT8 gfx_palettebank;



static void harddriv_fast_draw(void);
static void stunrun_fast_draw(void);



/*************************************
 *
 *	Start/stop routines
 *
 *************************************/

VIDEO_START( harddriv )
{
	UINT32 *destmask, mask;
	int i;

	shiftreg_enable = 0;
	shiftreg_count = 512*8 >> hdgsp_multisync;

	gfx_offset = 0;
	gfx_rowbytes = 0;
	gfx_offsetscan = 0;
	gfx_finescroll = 0;
	gfx_palettebank = 0;

	/* allocate the mask table */
	mask_table = auto_malloc(sizeof(UINT32) * 4 * 65536);
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
	force_partial_update(scanline - 1);
	gfx_offset = offs >> hdgsp_multisync;
	gfx_offsetscan = scanline;
	gfx_rowbytes = rowbytes >> hdgsp_multisync;
}



/*************************************
 *
 *	Palette bank updating
 *
 *************************************/

static void update_palette_bank(int newbank)
{
	if (gfx_palettebank != newbank)
	{
		force_partial_update(cpu_getscanline());
		gfx_palettebank = newbank;
	}
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
		switch (activecpu_get_pc())
		{
			case 0xffc05700:
				harddriv_fast_draw();
				break;

			case 0xfff45360:
				stunrun_fast_draw();
				break;
		}
/*		logerror("Color @ %08X\n", activecpu_get_pc());*/
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
			if (gfx_finescroll != data)
			{
				force_partial_update(cpu_getscanline() - 1);
				gfx_finescroll = data;
			}
			break;

		case 0x02:
			update_palette_bank((gfx_palettebank & ~1) | val);
			break;

		case 0x03:
			update_palette_bank((gfx_palettebank & ~2) | (val << 1));
			break;

		case 0x04:
			if (Machine->drv->total_colors >= 256 * 8)
				update_palette_bank((gfx_palettebank & ~4) | (val << 2));
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
	palette_set_color(offset, red, green, blue);
}


READ16_HANDLER( hdgsp_paletteram_lo_r )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_lo[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_lo_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

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
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_hi[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_hi_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	COMBINE_DATA(&hdgsp_paletteram_hi[offset]);
	gsp_palette_change(offset);
}



/*************************************
 *
 *	End of frame routine
 *
 *************************************/

VIDEO_EOF( harddriv )
{
	/* reset the display offset */
	gfx_offsetscan = 0;
}



/*************************************
 *
 *	Core refresh routine
 *
 *************************************/

VIDEO_UPDATE( harddriv )
{
	pen_t *pens = &Machine->pens[gfx_palettebank * 256];
	int x, y, width = Machine->drv->screen_width;
	UINT8 scanline[512];

	/* check for disabled video */
	if (tms34010_io_display_blanked(1))
	{
		fillbitmap(bitmap, pens[0], cliprect);
		return;
	}

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 offset = gfx_offset + gfx_rowbytes * (y - gfx_offsetscan) + ((gfx_finescroll + 1) & 15);
		UINT8 *dest = scanline;

		/* if we're on an even pixel boundary, it's easy */
		for (x = 0; x < width; x++)
			*dest++ = hdgsp_vram[BYTE_XOR_LE(offset++) & vram_mask];

		/* draw the scanline */
		draw_scanline8(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x + 1, &scanline[cliprect->min_x], pens, -1);
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
	UINT32 a5 = activecpu_get_reg(TMS34010_A5);
	UINT16 *data = (UINT16 *)&hdgsp_vram[TOBYTE(a5) & vram_mask];
	UINT32 rowbytes = activecpu_get_reg(TMS34010_B3);
	UINT32 offset = activecpu_get_reg(TMS34010_B4);
	UINT32 b5 = activecpu_get_reg(TMS34010_B5);
	UINT32 b6 = activecpu_get_reg(TMS34010_B6);
	UINT32 pattern = activecpu_get_reg(TMS34010_B9);
	UINT32 lx,rx,lfrac,rfrac,rdelta=0,ldelta=0,count,cury,b8;
	int lclip = (INT16)b5, rclip = (INT16)b6;
	int tclip = (INT32)b5 >> 16, bclip = (INT32)b6 >> 16;
	int color = hdgsp_control_lo[0];
	int sx,ex,x;

	if (offset < 0x01f80000 || offset >= 0x02080000)
		return;
	offset -= 0x02000000;

	lx = (INT16)*data++;
	rx = (INT16)*data++ + 1;

	cury = (INT16)*data++;
	b8 = (cury << 1) & 31;
	pattern = (pattern << b8) | (pattern >> (32 - b8));
	lfrac = rfrac = 0x8000;
	offset += rowbytes * cury;

	FILL_SCANLINE(1)

	/* handle wireframe case */
	if (*(UINT16 *)&hdgsp_vram[TOBYTE(0xffc36e40) & vram_mask] ||
		(*(UINT16 *)&hdgsp_vram[TOBYTE(0xffc3d580) & vram_mask] & 8))
	{
		while (1)
		{
			rdelta = *data++; rdelta |= *data++ << 16;
			if (rdelta == 0xffffffff)
			{
				FILL_SCANLINE(1)

				activecpu_set_reg(TMS34010_A5, (((UINT8 *)data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
				activecpu_set_reg(TMS34010_PC, activecpu_get_pc() + 0xffc05c80 - 0xffc05700);
				EAT_CYCLES();
				return;
			}
			ldelta = *data++; ldelta |= *data++ << 16;
			count = *data++;

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
			rdelta = *data++; rdelta |= *data++ << 16;
			if (rdelta == 0xffffffff)
			{
				activecpu_set_reg(TMS34010_A5, (((UINT8 *)data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
				activecpu_set_reg(TMS34010_PC, activecpu_get_pc() + 0xffc05920 - 0xffc05700);
				EAT_CYCLES();
				return;
			}
			ldelta = *data++; ldelta |= *data++ << 16;
			count = *data++;

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
	UINT32 a5 = activecpu_get_reg(TMS34010_A5);
	UINT16 *data = (UINT16 *)&hdgsp_vram[TOBYTE(a5) & vram_mask];
	UINT32 rx = (INT16)activecpu_get_reg(TMS34010_A6) + 1;
	UINT32 lx = (INT16)activecpu_get_reg(TMS34010_B0);
	UINT32 rowbytes = activecpu_get_reg(TMS34010_B3) >> 1;
	UINT32 offset = activecpu_get_reg(TMS34010_B4);
	UINT32 b5 = activecpu_get_reg(TMS34010_B5);
	UINT32 b6 = activecpu_get_reg(TMS34010_B6);
	UINT32 pattern = activecpu_get_reg(TMS34010_B9);
	UINT32 lfrac,rfrac,rdelta=0,ldelta=0,count,cury,b8;
	int lclip = (INT16)b5, rclip = (INT16)b6;
	int tclip = (INT32)b5 >> 16, bclip = (INT32)b6 >> 16;
	int color = hdgsp_control_lo[0];
	int sx,ex,x;

	if (offset < 0x01f80000 || offset >= 0x02080000)
		return;
	offset = (INT32)(offset - 0x02000000) >> 1;

	cury = (INT16)*data++ + 1;
	b8 = (cury << 2) & 31;
	pattern = (pattern << b8) | (pattern >> (32 - b8));
	lfrac = rfrac = 0x8000;
	offset += rowbytes * cury;

	FILL_SCANLINE(2)

	while (1)
	{
		rdelta = *data++; rdelta |= *data++ << 16;
		if (rdelta == 0xffffffff)
		{
			activecpu_set_reg(TMS34010_A5, (((UINT8 *)data - hdgsp_vram) * 8) | (a5 & ~vram_mask));
			activecpu_set_reg(TMS34010_PC, activecpu_get_pc() + 0xfff45630 - 0xfff45360);
			EAT_CYCLES();
			return;
		}
		ldelta = *data++; ldelta |= *data++ << 16;
		count = *data++;

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
