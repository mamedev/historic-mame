/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data8_t *sidearms_bg_scrollx, *sidearms_bg_scrolly;
int sidearms_vidhrdw;

static int bgon, objon, staron, charon;
static int flipscreen;
static unsigned int hflop_74a_n, hcount_191, vcount_191, latch_374, hadd_283, vadd_283;
static data8_t *bg_tilemap, *sf_rom;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( sidearms )
{
	if (video_start_generic() != 0) return 1;
	if (!(Machine->scrbitmap->depth == 15 || Machine->scrbitmap->depth == 16)) return 1;

	hflop_74a_n = 1;
	hcount_191 = vcount_191 = latch_374 = hadd_283 = vadd_283 = 0;

	bg_tilemap = memory_region(REGION_GFX4);
	sf_rom = memory_region(REGION_USER1);

	return 0;
}


WRITE_HANDLER( sidearms_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 and 3 lock the coin chutes */

	/* bit 4 resets the sound CPU */
	if (data & 0x10) cpuint_reset_cpu(1);

	/* bit 5 enables starfield */
	if (!(staron = data & 0x20)) { hflop_74a_n = 1; hcount_191 = vcount_191 = 0; }

	/* bit 6 enables char layer */
	charon = data & 0x40;

	/* bit 7 flips screen (TODO) */
	flipscreen = data & 0x80;
}


WRITE_HANDLER( sidearms_gfxctrl_w )
{
	objon = data & 0x01;
	bgon = data & 0x02;
}


WRITE_HANDLER( sidearms_star_scrollx_w )
{
	unsigned int last_state;

	last_state = hcount_191;
	hcount_191++; hcount_191 &= 0x1ff;

	// invert 74LS74A(flipflop) output on 74LS191(hscan counter) carry's rising edge
	if (hcount_191 & ~last_state & 0x100) hflop_74a_n ^= 1;
}


WRITE_HANDLER( sidearms_star_scrolly_w )
{
	vcount_191++; vcount_191 &= 0xff;
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void draw_sprites(
	struct mame_bitmap *bitmap,
	const struct GfxElement *gfx,
	const struct rectangle *cliprect,
	int start_offset, int end_offset)
{
	int offset, code, color, x, y;

	for (offset=end_offset-32; offset>=start_offset; offset-=32)
	{
		y = buffered_spriteram[offset + 2];
		if (!y || buffered_spriteram[offset + 5] == 0xc3) continue;

		code = buffered_spriteram[offset];
		color = buffered_spriteram[offset + 1];
		x = buffered_spriteram[offset + 3];
		code += color<<3 & 0x700;
		x += color<<4 & 0x100;
		color &= 0xf;

		drawgfx(bitmap, gfx, code, color, 0, 0, x, y, cliprect, TRANSPARENCY_PEN, 15);
	}
}


VIDEO_UPDATE( sidearms )
{

#define BMPADX 64
#define TMPITCH_L2 8
#define TMPITCH (1<<TMPITCH_L2)

	static int tilebank_base[4] = {0, 0x100, 0x200, 0x100};

	struct GfxElement *gfx;
	int drawstars, dispx, dispy, code, color, bmpitch, tmoffs, offset, x, y, i;

	unsigned int last_state, _hflop_74a_n, _hcount_191, _vcount_191, _vadd_283;
	data16_t *lineptr;
	data8_t *_sf_rom;


	drawstars = (sidearms_vidhrdw == 0) ? 1 : 0;

	if (drawstars)
	{
		lineptr = (data16_t *)bitmap->line[15] + 64;
		bmpitch = bitmap->rowpixels;

		// clear display
		for (x=224; x; x--) memset(lineptr+=bmpitch, 0, 768);

		// draw starfield if enabled
		if (staron)
		{
			lineptr = (data16_t *)bitmap->line[0];

			// cache read-only globals to stack frame
			_hcount_191 = hcount_191;
			_vcount_191 = vcount_191;
			_vadd_283 = vadd_283;
			_hflop_74a_n = hflop_74a_n;
			_sf_rom = sf_rom;

			//.p2align 4(how in GCC???)
			for (y=0; y<256; y++) // 8-bit V-clock input
			{
				for (x=0; x<512; x++) // 9-bit H-clock input
				{
					last_state = hadd_283;
					hadd_283 = (_hcount_191 & 0xff) + (x & 0xff);  // add lower 8 bits and preserve carry

					if (x<64 || x>447 || y<16 || y>239) continue;  // clip rejection

					_vadd_283 = _vcount_191 + y;                   // add lower 8 bits and discard carry(later)

					if (!((_vadd_283 ^ (x>>3)) & 4)) continue;     // logic rejection 1
					if ((_vadd_283 | (hadd_283>>1)) & 2) continue; // logic rejection 2

					// latch data from starfield EPROM on rising edge of 74LS374's clock input
					if ((last_state & 0x1f) == 0x1f)
					{
						_vadd_283 = _vadd_283<<4 & 0xff0;               // to starfield EPROM A04-A11 (8 bits)
						_vadd_283 |= (_hflop_74a_n^(hadd_283>>8)) << 3; // to starfield EPROM A03     (1 bit)
						_vadd_283 |= hadd_283>>5 & 7;                   // to starfield EPROM A00-A02 (3 bits)
						latch_374 = _sf_rom[_vadd_283 + 0x3000];        // lines A12-A13 are always high
					}

					if ((((latch_374 ^ hadd_283) ^ 1) & 0x1f) != 0x1f) continue; // logic rejection 3

					lineptr[x] = (data16_t)(latch_374>>5 | 0x378); // to color mixer
				}
				lineptr += bmpitch;
			}
		}
	}


	/* Draw the entire background scroll */
	if (bgon)
	{
		gfx = Machine->gfx[1];

		x = sidearms_bg_scrollx[0] + (sidearms_bg_scrollx[1]<<8 & 0xf00) + BMPADX;
		y = sidearms_bg_scrolly[0] + (sidearms_bg_scrolly[1]<<8 & 0xf00);
		tmoffs = ((y>>5) << TMPITCH_L2) + ((x>>5) << 1);
		dispx = -(x & 0x1f);
		dispy = -(y & 0x1f);

		if (sidearms_vidhrdw == 0)
		{
			i = (drawstars) ? TRANSPARENCY_PEN : TRANSPARENCY_NONE;

			for (y=0; y<9; tmoffs+=TMPITCH, y++)
			for (x=0; x<13; x++)
			{
				offset = tmoffs + (x << 1);
				/* swap bits 1-7 and 8-10 of the address to compensate for the funny layout of the ROM data */
				offset = ((offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3)) & 0x7fff;
				color = bg_tilemap[offset + 1];

				drawgfx(
					bitmap, gfx,
					bg_tilemap[offset] | (color<<8 & 0x100),
					color>>3 & 0x1f,
					color & 0x02,
					color & 0x04,
					(x<<5) + dispx + BMPADX,
					(y<<5) + dispy,
					cliprect, i, 15);
			}
		}
		else
		{
			for (y=0; y<9; tmoffs+=TMPITCH, y++)
			for (x=0; x<13; x++)
			{
				offset = tmoffs + (x << 1);
				offset = ((offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3)) & 0x7fff;
				color = bg_tilemap[offset + 1];

				drawgfx(
					bitmap, gfx,
					bg_tilemap[offset] | tilebank_base[(color>>6 & 2)|(color&1)],
					color>>3 & 0x0f,
					color & 0x02,
					color & 0x04,
					(x<<5) + dispx + BMPADX,
					(y<<5) + dispy,
					cliprect, TRANSPARENCY_NONE, 15);
			}
		}
	}


	/* Draw the sprites. */
	if (objon)
	{
		gfx = Machine->gfx[2];

		if (sidearms_vidhrdw == 2) // Dyger has simple front-to-back sprite priority
			draw_sprites(bitmap, gfx, cliprect, 0x0000, 0x1000);
		else
		{
			draw_sprites(bitmap, gfx, cliprect, 0x0700, 0x0800);
			draw_sprites(bitmap, gfx, cliprect, 0x0e00, 0x1000);
			draw_sprites(bitmap, gfx, cliprect, 0x0800, 0x0f00);
			draw_sprites(bitmap, gfx, cliprect, 0x0000, 0x0700);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	if (charon)
	{
		gfx = Machine->gfx[0];
		i = videoram_size - 256;

		for (x=0; x<i; x++)
		{
			offset = x + 128;
			code = videoram[offset];
			color = colorram[offset];

			if (code==0x27 && !color) continue;

			code += color<<2 & 0x300;
			color &= 0x3f;

			drawgfx(
				bitmap, gfx, code, color, 0, 0,
				offset<<3 & 0x1f8,
				offset>>3 & ~7,
				cliprect, TRANSPARENCY_PEN, 3);
		}
	}
}

VIDEO_EOF( sidearms )
{
	buffer_spriteram_w(0, 0);
}
