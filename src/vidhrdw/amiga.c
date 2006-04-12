/***************************************************************************
Amiga Computer / Arcadia System - (c) 1988, Arcadia Systems.

Driver by:

Ernesto Corvi and Mariusz Wojcieszek

To do:
 - sprite collisions (ar_sdwr, ar_sprg)
 - manual sprites

***************************************************************************/

#include "driver.h"
#include "includes/amiga.h"

#define DEBUG		0

static UINT8 sprite_comparitor_enable_mask;
static UINT8 sprite_dma_reload_mask;
static UINT16 ham_color;

static int last_scanline;

static UINT32 copper_pc;
static UINT8 copper_waiting;
static UINT8 copper_waitblit;
static UINT16 copper_waitval;
static UINT16 copper_waitmask;
static UINT16 copper_pending_offset;
static UINT16 copper_pending_data;


UINT32 amiga_gethvpos(void)
{
	return (last_scanline << 8) | (cpu_gethorzbeampos() >> 1);
}


/***************************************************************************

    Copper emulation

***************************************************************************/

INLINE void copper_reset( void )
{
	copper_pc = CUSTOM_REG_LONG(REG_COP1LCH) / 2;
	copper_waiting = FALSE;
}

void copper_setpc( unsigned long pc )
{
#if DEBUG
	logerror("copper_setpc(%06x)\n", pc);
#endif
	copper_pc = pc / 2;
	copper_waiting = FALSE;
}

void copper_enable( void )
{
//  copper.enabled = ( CUSTOM_REG(REG_DMACON) & ( DMACON_COPEN | DMACON_DMAEN ) ) == ( DMACON_COPEN | DMACON_DMAEN );
}


static int copper_execute_one(int xpos)
{
	int inst, param;

	/* bail if not enabled */
	if ((CUSTOM_REG(REG_DMACON) & (DMACON_COPEN | DMACON_DMAEN)) != (DMACON_COPEN | DMACON_DMAEN))
		return 511;

	/* flush any pending waits */
	if (copper_pending_offset)
	{
		amiga_custom_w(copper_pending_offset, copper_pending_data, 0);
		copper_pending_offset = 0;
	}

	/* if we're waiting, check for a breakthrough */
	if (copper_waiting)
	{
		int curpos = (last_scanline << 8) | (xpos >> 1);

		/* if we're past the wait time, stop it and hold up 2 cycles */
		if ((curpos & copper_waitmask) >= (copper_waitval & copper_waitmask) &&
			(!copper_waitblit || !(CUSTOM_REG(REG_DMACON) & DMACON_BBUSY)))
		{
			copper_waiting = FALSE;
			return xpos + 4;
		}

		/* otherwise, see if this line is even a possibility; if not, punt */
		if (((curpos | 0xff) & copper_waitmask) < (copper_waitval & copper_waitmask))
			return 511;

		return xpos + 2;
	}

	/* fetch the instruction */
	inst = amiga_chip_ram[copper_pc];
	param = amiga_chip_ram[copper_pc + 1];
#if DEBUG
logerror("%02X.%02X: Copper inst @ %06x = %04x %04x\n", last_scanline, xpos / 2, copper_pc * 2, inst, param);
#endif
	copper_pc += 2;

	/* handle a move */
	if ((inst & 1) == 0)
	{
		int min = (CUSTOM_REG(REG_COPCON) & 2) ? 0x20 : 0x40;

#if DEBUG
logerror("  Write to %04x = %04x\n", (inst >> 1) & 0xff, param);
#endif

		/* do the write if we're allowed */
		inst = (inst >> 1) & 0xff;
		if (inst >= min)
		{
			/* write it at the *end* of this instruction's cycles */
			/* needed for Arcadia's Fast Break */
			copper_pending_offset = inst;
			copper_pending_data = param;
		}

		/* illegal writes suspend until next frame */
		else
		{
			copper_waitval = 0xffff;
			copper_waitmask = 0xffff;
			copper_waitblit = FALSE;
			copper_waiting = TRUE;
			return 511;
		}
	}
	else
	{
		/* extract common wait/skip values */
		copper_waitval = inst & 0xfffe;
		copper_waitmask = param | 0x8001;
		copper_waitblit = (~param >> 15) & 1;

		/* handle a wait */
		if ((param & 1) == 0)
		{
#if DEBUG
			logerror("  Waiting for %04x & %04x (currently %04x)\n", copper_waitval, copper_waitmask, (last_scanline << 8) | (xpos >> 1));
#endif
			copper_waiting = TRUE;
		}

		/* handle a skip */
		else
		{
			int curpos = (last_scanline << 8) | (xpos >> 1);

#if DEBUG
			logerror("  Skipping if %04x & %04x (currently %04x)\n", copper_waitval, copper_waitmask, (last_scanline << 8) | (xpos >> 1));
#endif
			/* if we're past the wait time, stop it and hold up 2 cycles */
			if ((curpos & copper_waitmask) >= (copper_waitval & copper_waitmask) &&
				(!copper_waitblit || !(CUSTOM_REG(REG_DMACON) & DMACON_BBUSY)))
			{
#if DEBUG
				logerror("  Skipped\n");
#endif
				copper_pc += 2;
			}
		}
	}

	/* advance and consume 8 cycles */
	return xpos + 8;
}



VIDEO_START(amiga)
{
	return video_start_generic_bitmapped();
}

void amiga_prepare_frame(void)
{
	/* reset the copper */
	copper_reset();

}

INLINE UINT8 assemble_odd_bitplanes(int planes, int obitoffs)
{
	UINT8 pix = (CUSTOM_REG(REG_BPL1DAT) >> obitoffs) & 1;
	if (planes >= 3)
	{
		pix |= ((CUSTOM_REG(REG_BPL3DAT) >> obitoffs) & 1) << 2;
		if (planes >= 5)
			pix |= ((CUSTOM_REG(REG_BPL5DAT) >> obitoffs) & 1) << 4;
	}
	return pix;
}


INLINE UINT8 assemble_even_bitplanes(int planes, int ebitoffs)
{
	UINT8 pix = 0;
	if (planes >= 2)
	{
		pix |= ((CUSTOM_REG(REG_BPL2DAT) >> ebitoffs) & 1) << 1;
		if (planes >= 4)
		{
			pix |= ((CUSTOM_REG(REG_BPL4DAT) >> ebitoffs) & 1) << 3;
			if (planes >= 6)
				pix |= ((CUSTOM_REG(REG_BPL6DAT) >> ebitoffs) & 1) << 5;
		}
	}
	return pix;
}


void amiga_sprite_dma_reset(int which)
{
	sprite_dma_reload_mask |= 1 << which;
}


void amiga_sprite_enable_comparitor(int which, int enable)
{
	if (enable)
		sprite_comparitor_enable_mask |= 1 << which;
	else
		sprite_comparitor_enable_mask &= ~(1 << which);
}


static void update_sprite_dma(int scanline)
{
	int dmaenable = (CUSTOM_REG(REG_DMACON) & (DMACON_SPREN | DMACON_DMAEN)) == (DMACON_SPREN | DMACON_DMAEN);
	int num;

	/* loop over sprite channels */
	for (num = 0; num < 8; num++)
	{
		int vstart, vstop;

		/* if we are == VSTOP, fetch new words */
		if (dmaenable && (sprite_dma_reload_mask & (1 << num)))
		{
			CUSTOM_REG(REG_SPR0POS + 4 * num) = amiga_chip_ram[CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) / 2];
			CUSTOM_REG(REG_SPR0CTL + 4 * num) = amiga_chip_ram[CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) / 2 + 1];
			CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) += 4;

			/* disable the sprite */
			sprite_comparitor_enable_mask &= ~(1 << num);
			sprite_dma_reload_mask &= ~(1 << num);
		}

		/* compute vstart/vstop */
		vstart = (CUSTOM_REG(REG_SPR0POS + 4 * num) >> 8) | ((CUSTOM_REG(REG_SPR0CTL + 4 * num) << 6) & 0x100);
		vstop = (CUSTOM_REG(REG_SPR0CTL + 4 * num) >> 8) | ((CUSTOM_REG(REG_SPR0CTL + 4 * num) << 7) & 0x100);

		/* if we hit vstart, enable the comparitor */
		if (scanline == vstart)
			sprite_comparitor_enable_mask |= 1 << num;

		/* if we hit vstop, disable the comparitor and trigger a reload */
		if (scanline == vstop)
		{
			sprite_comparitor_enable_mask &= ~(1 << num);
			sprite_dma_reload_mask |= 1 << num;
		}

		/* otherwise, display */
		/* if this sprite is enabled, turn it on and fetch the data */
		if (dmaenable && (sprite_comparitor_enable_mask & (1 << num)))
		{
			CUSTOM_REG(REG_SPR0DATA + 4 * num) = amiga_chip_ram[CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) / 2];
			CUSTOM_REG(REG_SPR0DATB + 4 * num) = amiga_chip_ram[CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) / 2 + 1];
			CUSTOM_REG_LONG(REG_SPR0PTH + 2 * num) += 4;
		}
	}
}


static int get_sprite_pixel(int x)
{
	int mask = sprite_comparitor_enable_mask;
	int pixels = 0;
	int pair;

	/* loop over sprite channel pairs */
	for (pair = 0; mask; pair++, mask >>= 2)
	{
		/* lower of the pair */
		if (mask & 1)
		{
			int hstart = ((CUSTOM_REG(REG_SPR0POS + 8 * pair) & 0xff) << 1) | (CUSTOM_REG(REG_SPR0CTL + 8 * pair) & 1);
			if (x >= hstart && x < hstart + 16)
			{
				UINT16 lobits = CUSTOM_REG(REG_SPR0DATA + 8 * pair);
				UINT16 hibits = CUSTOM_REG(REG_SPR0DATB + 8 * pair);
				int bit = 15 - (x - hstart);

				pixels |= ((lobits >> bit) & 1) << 0;
				pixels |= ((hibits >> bit) & 1) << 1;
			}
		}

		/* upper of the pair */
		if (mask & 2)
		{
			int hstart = ((CUSTOM_REG(REG_SPR1POS + 8 * pair) & 0xff) << 1) | (CUSTOM_REG(REG_SPR1CTL + 8 * pair) & 1);
			if (x >= hstart && x < hstart + 16)
			{
				UINT16 lobits = CUSTOM_REG(REG_SPR1DATA + 8 * pair);
				UINT16 hibits = CUSTOM_REG(REG_SPR1DATB + 8 * pair);
				int bit = 15 - (x - hstart);

				pixels |= ((lobits >> bit) & 1) << 2;
				pixels |= ((hibits >> bit) & 1) << 3;
			}
		}

		/* if we have pixels, determine the actual color and get out */
		if (pixels)
		{
			/* attached case */
			if (CUSTOM_REG(REG_SPR1CTL + 8 * pair) & 0x0080)
				return pixels | 0x10 | (pair << 6);
			else if (pixels & 3)
				return (pixels & 3) | 0x10 | (pair << 2) | (pair << 6);
			else
				return ((pixels >> 2) & 3) | 0x10 | (pair << 2) | (pair << 6);
		}
	}

	return 0;
}


INLINE int update_ham(int newpix)
{
	switch (newpix >> 4)
	{
		case 0:
			ham_color = CUSTOM_REG(REG_COLOR00 + (newpix & 0xf));
			break;

		case 1:
			ham_color = (ham_color & 0xff0) | ((newpix & 0xf) << 0);
			break;

		case 2:
			ham_color = (ham_color & 0x0ff) | ((newpix & 0xf) << 8);
			break;

		case 3:
			ham_color = (ham_color & 0xf0f) | ((newpix & 0xf) << 4);
			break;
	}
	return ham_color;
}


void amiga_render_scanline(int scanline)
{
	int ddf_start_pixel = 0, ddf_stop_pixel = 0;
	int hires = 0, dualpf = 0, lace = 0, ham = 0;
	int hstart = 0, hstop = 0;
	int vstart = 0, vstop = 0;
	int pf1pri = 0, pf2pri = 0;
	int planes = 0;

	int x;
	UINT16 *dst;
	int ebitoffs, obitoffs;
	int edelay, odelay;
	int next_copper_x;
	int p;

	/* update sprite data fetching */
	update_sprite_dma(scanline);

	last_scanline = scanline;

	if (scanline == 0)
		ham_color = CUSTOM_REG(REG_COLOR00);

	/* normalize some register values */
	if (CUSTOM_REG(REG_DDFSTOP) < CUSTOM_REG(REG_DDFSTRT))
		CUSTOM_REG(REG_DDFSTOP) = CUSTOM_REG(REG_DDFSTRT);

	/* start of a new line, signal we're not done with it and fill up vars */
	dst = (UINT16 *)tmpbitmap->line[scanline];

	odelay = CUSTOM_REG(REG_BPLCON1) & 0xf;
	edelay = CUSTOM_REG(REG_BPLCON1) >> 4;
	obitoffs = 15 + odelay;
	ebitoffs = 15 + edelay;
	for (p = 0; p < 6; p++)
		CUSTOM_REG(REG_BPL1DAT + p) = 0;

#if DEBUG
if (CUSTOM_REG(REG_BPLCON1) != 0)
	ui_popup("BPLCON1=%02X", CUSTOM_REG(REG_BPLCON1));
#endif

	/* loop over the line */
	next_copper_x = 0;
	for (x = 0; x < 0xe4*2; x++)
	{
		/* time to execute the copper? */
		if (x == next_copper_x)
		{
			next_copper_x = copper_execute_one(x);

			/* compute update-related register values */
			planes = (CUSTOM_REG(REG_BPLCON0) & (BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2)) >> 12;
			hires = CUSTOM_REG(REG_BPLCON0) & BPLCON0_HIRES;
			ham = CUSTOM_REG(REG_BPLCON0) & BPLCON0_HOMOD;
			dualpf = CUSTOM_REG(REG_BPLCON0) & BPLCON0_DBLPF;
			lace = CUSTOM_REG(REG_BPLCON0) & BPLCON0_LACE;

			/* compute the pixel fetch parameters */
			ddf_start_pixel = CUSTOM_REG(REG_DDFSTRT) * 2 + (hires ? 9 : 17);
			ddf_stop_pixel = CUSTOM_REG(REG_DDFSTOP) * 2 + (hires ? (9 + 23) : (17 + 15));

#if DEBUG
if (planes != 0)
{
	static int lastcon0;
	if ((CUSTOM_REG(REG_BPLCON0) ^ lastcon0) & (BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 | BPLCON0_HIRES | BPLCON0_DBLPF | BPLCON0_LACE))
	{
		printf("Mode: %dbpp ham=%d hi=%d dual=%d lace=%d (%02X-%02X)\n", planes, ham != 0, hires != 0, dualpf != 0, lace != 0, CUSTOM_REG(REG_DDFSTRT), CUSTOM_REG(REG_DDFSTOP));
		lastcon0 = CUSTOM_REG(REG_BPLCON0);
	}
}
#endif
			/* compute the horizontal start/stop */
			hstart = CUSTOM_REG(REG_DIWSTRT) & 0xff;
			hstop = 0x100 + (CUSTOM_REG(REG_DIWSTOP) & 0xff);

			/* compute the vertical start/stop */
			vstart = CUSTOM_REG(REG_DIWSTRT) >> 8;
			vstop = (CUSTOM_REG(REG_DIWSTOP) >> 8) | ((~CUSTOM_REG(REG_DIWSTOP) >> 7) & 0x100);

			/* extract playfield priorities */
			pf1pri = CUSTOM_REG(REG_BPLCON2) & 7;
			pf2pri = (CUSTOM_REG(REG_BPLCON2) >> 3) & 7;
		}

		/* clear the target pixels to the background color as a starting point */
		dst[x*2+0] =
		dst[x*2+1] = CUSTOM_REG(REG_COLOR00);

		/* to render, we must have bitplane DMA enabled, at least 1 plane, and be within the */
		/* vertical display window */
		if ((CUSTOM_REG(REG_DMACON) & (DMACON_BPLEN | DMACON_DMAEN)) == (DMACON_BPLEN | DMACON_DMAEN) &&
			planes > 0 && scanline >= vstart && scanline < vstop)
		{
			int pfpix0 = 0, pfpix1 = 0;

			/* fetch the odd bits if we are within the fetching region */
			if (x >= ddf_start_pixel && x <= ddf_stop_pixel + odelay)
			{
				/* if we need to fetch more data, do it now */
				if (obitoffs == 15)
					for (p = 0; p < planes; p += 2)
					{
						CUSTOM_REG(REG_BPL1DAT + p) = amiga_chip_ram[CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) / 2];
						CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) += 2;
					}

				/* now assemble the bits */
				pfpix0 |= assemble_odd_bitplanes(planes, obitoffs);
				obitoffs--;

				/* for high res, assemble a second set of bits */
				if (hires)
				{
					pfpix1 |= assemble_odd_bitplanes(planes, obitoffs);
					obitoffs--;
				}
				else
					pfpix1 |= pfpix0 & 0x15;

				/* reset bit offsets if needed */
				if (obitoffs < 0)
					obitoffs = 15;
			}

			/* fetch the even bits if we are within the fetching region */
			if (x >= ddf_start_pixel && x <= ddf_stop_pixel + edelay)
			{
				/* if we need to fetch more data, do it now */
				if (ebitoffs == 15)
					for (p = 1; p < planes; p += 2)
					{
						CUSTOM_REG(REG_BPL1DAT + p) = amiga_chip_ram[CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) / 2];
						CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) += 2;
					}

				/* now assemble the bits */
				pfpix0 |= assemble_even_bitplanes(planes, ebitoffs);
				ebitoffs--;

				/* for high res, assemble a second set of bits */
				if (hires)
				{
					pfpix1 |= assemble_even_bitplanes(planes, ebitoffs);
					ebitoffs--;
				}
				else
					pfpix1 |= pfpix0 & 0x2a;

				/* reset bit offsets if needed */
				if (ebitoffs < 0)
					ebitoffs = 15;
			}

			/* if we are within the display region, render */
			if (x >= hstart && x <= hstop)
			{
				int sprpix = get_sprite_pixel(x);

				/* hold-and-modify mode -- assume low-res (hi-res not supported by the hardware) */
				if (ham)
				{
					/* update the HAM color */
					pfpix0 = update_ham(pfpix0);

					/* sprite has priority */
					if (sprpix && pf1pri > (sprpix >> 6))
					{
						dst[x*2+0] =
						dst[x*2+1] = CUSTOM_REG(REG_COLOR00 + (sprpix & 0x1f));
					}

					/* playfield has priority */
					else
					{
						dst[x*2+0] =
						dst[x*2+1] = pfpix0;
					}
				}

				/* dual playfield mode */
				else if (dualpf)
				{
				}

				/* single playfield mode */
				else
				{
					/* sprite has priority */
					if (sprpix && pf1pri > (sprpix >> 6))
					{
						dst[x*2+0] =
						dst[x*2+1] = CUSTOM_REG(REG_COLOR00 + (sprpix & 0x1f));
					}

					/* playfield has priority */
					else
					{
						dst[x*2+0] = CUSTOM_REG(REG_COLOR00 + pfpix0);
						dst[x*2+1] = CUSTOM_REG(REG_COLOR00 + pfpix1);
					}
				}
			}
		}
	}

	if (scanline >= vstart && scanline < vstop)
	{
		int p;

		/* update odd planes */
		for (p = 0; p < planes; p += 2)
			CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) += CUSTOM_REG_SIGNED(REG_BPL1MOD);

		/* update even planes */
		for (p = 1; p < planes; p += 2)
			CUSTOM_REG_LONG(REG_BPL1PTH + p * 2) += CUSTOM_REG_SIGNED(REG_BPL2MOD);
	}
}


PALETTE_INIT( amiga )
{
	int i;

	for (i = 0; i < 0x1000; i++)
		palette_set_color(i, pal4bit(i >> 8), pal4bit(i >> 4), pal4bit(i));
}

