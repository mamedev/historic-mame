/***************************************************************************

	Art & Magic hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "artmagic.h"


data16_t *artmagic_vram0;
data16_t *artmagic_vram1;

/* decryption parameters */
int artmagic_xor[16], artmagic_no_onelinedecrypt;

static UINT8 palette_data[3];
static UINT8 palette_index_w;
static UINT8 palette_index_r;
static UINT8 palette_color;

static UINT16 *blitter_base;
static UINT32 blitter_mask;
static UINT16 blitter_data[8];
static UINT8 blitter_page;



INLINE UINT16 *address_to_vram(offs_t *address)
{
	offs_t original = *address;
	*address = TOWORD(original & 0x001fffff);
	if (original >= 0x00000000 && original < 0x001fffff)
		return artmagic_vram0;
	else if (original >= 0x00400000 && original < 0x005fffff)
		return artmagic_vram1;
	logerror("address_to_vram(%08X) failed!\n", original);
	return NULL;
}



/*************************************
 *
 *	Video start
 *
 *************************************/

VIDEO_START( artmagic )
{
	blitter_base = (UINT16 *)memory_region(REGION_GFX1);
	blitter_mask = memory_region_length(REGION_GFX1)/2 - 1;
	return 0;
}



/*************************************
 *
 *	Shift register transfers
 *
 *************************************/

void artmagic_to_shiftreg(offs_t address, data16_t *data)
{
	UINT16 *vram = address_to_vram(&address);
	if (vram)
		memcpy(data, &vram[address], TOBYTE(0x2000));
}


void artmagic_from_shiftreg(offs_t address, data16_t *data)
{
	UINT16 *vram = address_to_vram(&address);
	if (vram)
		memcpy(&vram[address], data, TOBYTE(0x2000));
}



/*************************************
 *
 *	Palette RAM
 *
 *************************************/

READ16_HANDLER( artmagic_paletteram_r )
{
	int result = 0xff;

	switch (offset)
	{
		/* stonebal reads the palette back via this address */
		case 1:
			if (palette_color == 3)
			{
				palette_color = 0;
				palette_get_color(palette_index_r++, &palette_data[0], &palette_data[1], &palette_data[2]);
			}
			result = palette_data[palette_color++] >> 2;
			break;
	}
	return result;
}


WRITE16_HANDLER( artmagic_paletteram_w )
{
	if (ACCESSING_LSB)
		switch (offset)
		{
			case 0:
				palette_index_w = data;
				palette_color = 0;
				break;

			case 1:
				palette_data[palette_color++] = data;
				if (palette_color == 3)
				{
					palette_color = 0;
					palette_set_color(palette_index_w++, palette_data[0] << 2, palette_data[1] << 2, palette_data[2] << 2);
				}
				break;

			/* stonebal writes a 0 here before reading back the palette */
			case 3:
				palette_index_r = data;
				palette_color = 3;
				break;
		}
}



/*************************************
 *
 *	Custom blitter
 *
 *************************************/

READ16_HANDLER( artmagic_blitter_r )
{
	// bit 1 is a busy flag; loops tightly if clear
	// bit 4 determines the page
//	logerror("%08X:tms_blitter_r(%d)\n", activecpu_get_pc(), offset);
	return 0xffef | (blitter_page << 4);
}


WRITE16_HANDLER( artmagic_blitter_w )
{
#if 0
	static UINT32 hit_list[50000];
	static int hit_index;
#endif
	UINT16 *dest = blitter_page ? artmagic_vram0 : artmagic_vram1;
	int sx, sy, x, y, w, h, i, j, color, zoomx, zoomy, last;
//static FILE *log;

//	logerror("%08X:tms_blitter_w(%d) = %04X\n", activecpu_get_pc(), offset, data);
	COMBINE_DATA(&blitter_data[offset]);

	// check out FFE031C0
	switch (offset)
	{
		case 0:
			// 16 LSBs of source address
			break;

		case 1:
			// 8 MSBs of source address, plus something else
			break;

		case 2:
			// X location
			break;

		case 3:
			// Y location; also triggers blit
			offset = ((blitter_data[1] & 0xff) << 16) | blitter_data[0];
			x = (INT16)blitter_data[2];
			y = (INT16)blitter_data[3];
			w = ((blitter_data[7] & 0xff) + 1) * 4;
			h = (blitter_data[7] >> 8) + 1;
			zoomx = blitter_data[6] & 0xff;
			zoomy = blitter_data[6] >> 8;
			color = (blitter_data[1] >> 4) & 0xf0;

			profiler_mark(PROFILER_VIDEO);

			logerror("%08X:Blit from %06X to (%d,%d) %dx%d -- %04X %04X %04X %04X %04X %04X %04X %04X\n",
					activecpu_get_pc(), offset, x, y, w, h,
					blitter_data[0], blitter_data[1],
					blitter_data[2], blitter_data[3],
					blitter_data[4], blitter_data[5],
					blitter_data[6], blitter_data[7]);

#if 0
if (!log) log = fopen("blitter.log", "w");
if (log)
{
			for (i = 0; i < hit_index; i++)
				if (hit_list[i] == offset)
					break;
			if (i == hit_index)
			{
				hit_list[hit_index++] = offset;
				fprintf(log, "Blit @ %06X  %3dx%3d  (ends @ %06X?)\n", offset, w, h, offset + (w * h) / 4);
			}
}
#endif
#if 0
			for (i = 0; i < hit_index; i++)
				if (hit_list[i] == offset)
					break;
			if (i == hit_index)
			{
				int tempoffs = offset;
				hit_list[hit_index++] = offset;
				logerror("\t");
				for (i = 0; i < h; i++)
				{
					for (j = 0; j < w; j += 4)
						logerror("%04X ", blitter_base[tempoffs++]);
					logerror("\n\t");
				}
				logerror("\n");
			}
#endif

			sy = y;
			for (i = 0; i < h; i++)
			{
				if ((i & 1) || !((zoomy << ((i/2) & 7)) & 0x80))
				{
					if (sy >= 0 && sy < 256)
					{
						int tsy = sy * TOWORD(0x2000);
						sx = x;
						last = rand() & 0xf;
						for (j = 0; j < w; j += 4)
						{
							UINT16 val = blitter_base[(offset + j/4) & blitter_mask];
							if (sx < 508)
							{
								if (h == 1 && artmagic_no_onelinedecrypt)
									last = ((val) >>  0) & 0xf;
								else
									last = ((val ^ artmagic_xor[last]) >>  0) & 0xf;
								if (!((zoomx << ((j/2) & 7)) & 0x80))
								{
									if (last && sx >= 0 && sx < 512)
										dest[tsy + sx] = color | (last);
									sx++;
								}

								if (h == 1 && artmagic_no_onelinedecrypt)
									last = ((val) >>  4) & 0xf;
								else
									last = ((val ^ artmagic_xor[last]) >>  4) & 0xf;
								{
									if (last && sx >= 0 && sx < 512)
										dest[tsy + sx] = color | (last);
									sx++;
								}

								if (h == 1 && artmagic_no_onelinedecrypt)
									last = ((val) >>  8) & 0xf;
								else
									last = ((val ^ artmagic_xor[last]) >>  8) & 0xf;
								if (!((zoomx << ((j/2) & 7)) & 0x40))
								{
									if (last && sx >= 0 && sx < 512)
										dest[tsy + sx] = color | (last);
									sx++;
								}

								if (h == 1 && artmagic_no_onelinedecrypt)
									last = ((val) >> 12) & 0xf;
								else
									last = ((val ^ artmagic_xor[last]) >> 12) & 0xf;
								{
									if (last && sx >= 0 && sx < 512)
										dest[tsy + sx] = color | (last);
									sx++;
								}
							}
						}
					}
					sy++;
				}
				offset += w/4;
			}

			profiler_mark(PROFILER_END);

			break;

		case 4:
			blitter_page = (data >> 1) & 1;		// bit 1 = blitter page
//			logerror("blitter_page = %d\n", blitter_page);
			break;

		case 6:
			// zoomx/zoomy
			break;

		case 7:
			// height/width
			break;
	}
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( artmagic )
{
	UINT32 offset, dpytap;
	UINT16 *vram;
	int x, y;

	/* get the current parameters */
	cpuintrf_push_context(0);
	offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 8;
	dpytap = tms34010_io_register_r(REG_DPYTAP, 0);
	cpuintrf_pop_context();
//	logerror("offset = %08X  tap = %04X\n", offset, dpytap);

	/* compute the base and offset */
	vram = address_to_vram(&offset);
	if (!vram || tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	offset += cliprect->min_y * TOWORD(0x2000);
	offset += dpytap;

	/* render the bitmap */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels;
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = vram[(offset + x) & TOWORD(0x1fffff)] & 0xff;
		offset += TOWORD(0x2000);
	}
}
