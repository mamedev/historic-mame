/***************************************************************************

	Coors Light Bowling/Bowl-O-Rama hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/tms34061.h"
#include "cpu/m6809/m6809.h"
#include "capbowl.h"

UINT8 *capbowl_rowaddress;

static offs_t blitter_addr;



/*************************************
 *
 *	TMS34061 interfacing
 *
 *************************************/

static void generate_interrupt(int state)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, state);
}

static struct tms34061_interface tms34061intf =
{
	8,						/* VRAM address is (row << rowshift) | col */
	0x10000,				/* size of video RAM */
	0x100,					/* size of dirty chunks (must be power of 2) */
	generate_interrupt		/* interrupt gen callback */
};



/*************************************
 *
 *	Video start
 *
 *************************************/

VIDEO_START( capbowl )
{
	/* initialize TMS34061 emulation */
    return tms34061_start(&tms34061intf);
}



/*************************************
 *
 *	TMS34061 I/O
 *
 *************************************/

WRITE_HANDLER( capbowl_tms34061_w )
{
	int func = (offset >> 8) & 3;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	tms34061_w(col, *capbowl_rowaddress, func, data);
}


READ_HANDLER( capbowl_tms34061_r )
{
	int func = (offset >> 8) & 3;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	return tms34061_r(col, *capbowl_rowaddress, func);
}



/*************************************
 *
 *	Bowl-o-rama blitter
 *
 *************************************/

WRITE_HANDLER( bowlrama_blitter_w )
{
	switch (offset)
	{
		case 0x08:	  /* Write address high byte (only 2 bits used) */
			blitter_addr = (blitter_addr & ~0xff0000) | (data << 16);
			break;

		case 0x17:    /* Write address mid byte (8 bits)   */
			blitter_addr = (blitter_addr & ~0x00ff00) | (data << 8);
			break;

		case 0x18:	  /* Write Address low byte (8 bits)   */
			blitter_addr = (blitter_addr & ~0x0000ff) | (data << 0);
			break;

		default:
			logerror("PC=%04X Write to unsupported blitter address %02X Data=%02X\n", activecpu_get_pc(), offset, data);
			break;
	}
}


READ_HANDLER( bowlrama_blitter_r )
{
	data8_t data = memory_region(REGION_GFX1)[blitter_addr];
	data8_t result = 0;

	switch (offset)
	{
		/* Read Mask: Graphics data are 4bpp (2 pixels per byte).
			This function returns 0's for new pixel data.
			This allows data to be read as a mask, AND the mask with
			the screen data, then OR new data read by read data command. */
		case 0:
			if (!(data & 0xf0))
				result |= 0xf0;		/* High nibble is transparent */
			if (!(data & 0x0f))
				result |= 0x0f;		/* Low nibble is transparent */
			break;

		/* Read data and increment address */
		case 4:
			result = data;
			blitter_addr = (blitter_addr + 1) & 0x3ffff;
			break;

		default:
			logerror("PC=%04X Read from unsupported blitter address %02X\n", activecpu_get_pc(), offset);
			break;
	}

	return result;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( capbowl )
{
	int halfwidth = (cliprect->max_x - cliprect->min_x + 1) / 2;
	struct tms34061_display state;
	int x, y;

	/* first get the current display state */
	tms34061_get_display_state(&state);

	/* if we're blanked, just fill with black */
	if (state.blanked)
	{
		fillbitmap(bitmap, Machine->pens[0], cliprect);
		return;
	}

	/* update the palette and color usage */
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
		if (state.dirty[y])
		{
			UINT8 *src = &state.vram[256 * y];

			/* update the palette */
			for (x = 0; x < 16; x++)
			{
				int r = *src++ & 0x0f;
				int g = *src >> 4;
				int b = *src++ & 0x0f;

				palette_set_color(y * 16 + x, (r << 4) | r, (g << 4) | g, (b << 4) | b);
			}
		}

	/* now regenerate the bitmap */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT8 *src = &state.vram[256 * y + 32 + cliprect->min_x / 2];
		UINT8 scanline[400];
		UINT8 *dst = scanline;

		/* expand row to 8bpp */
		for (x = 0; x < halfwidth; x++)
		{
			int pix = *src++;
			*dst++ = pix >> 4;
			*dst++ = pix & 0x0f;
		}

		/* redraw the scanline and mark it no longer dirty */
		draw_scanline8(bitmap, cliprect->min_x, y, halfwidth * 2, scanline, &Machine->pens[16 * y], -1);
		state.dirty[y] = 0;
	}
}
