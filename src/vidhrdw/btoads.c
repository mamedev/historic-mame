/*************************************************************************

	BattleToads

	Video hardware emulation

**************************************************************************/

#include "driver.h"
#include "btoads.h"
#include "cpu/tms34010/tms34010.h"


#define DEBUG 0


/*************************************
 *
 *	Data types
 *
 *************************************/

struct display_state
{
	int scanline;
	int xscroll0, yscroll0;
	int xscroll1, yscroll1;
	int screen_control;
};



/*************************************
 *
 *	Global variables
 *
 *************************************/

data16_t *btoads_vram_fg0, *btoads_vram_fg1, *btoads_vram_fg_data;
data16_t *btoads_vram_bg0, *btoads_vram_bg1;
data16_t *btoads_sprite_scale;
data16_t *btoads_sprite_control;

static UINT8 *vram_fg_draw, *vram_fg_display;

static struct display_state current_state;
static struct display_state *state_cache;
static UINT8 state_index;

static UINT8 palette_entry;
static UINT8 palette_color;
static UINT8 palette_data[3];

static UINT16 sprite_source_offs;
static UINT8 *sprite_dest_base;
static UINT16 sprite_dest_offs;

static UINT16 misc_control;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int btoads_vh_start(void)
{
	/* allocate memory */
	state_cache = malloc(256 * sizeof(struct display_state));
	if (!state_cache)
		return 1;

	/* initialize the swapped pointers */
	vram_fg_draw = (UINT8 *)btoads_vram_fg0;
	vram_fg_display = (UINT8 *)btoads_vram_fg1;
	return 0;
}



/*************************************
 *
 *	Video system stop
 *
 *************************************/

void btoads_vh_stop(void)
{
	if (state_cache)
		free(state_cache);
	state_cache = NULL;
}



/*************************************
 *
 *	Control registers
 *
 *************************************/

INLINE void update_display_state(void)
{
	int scanline = cpu_getscanline();

	/* if we're in mid-screen, cache the current state */
	if (scanline < 224)
	{
		current_state.scanline = scanline;
		if (state_index == 0 || state_cache[state_index - 1].scanline != scanline)
			state_cache[state_index++] = current_state;
		else
			state_cache[state_index] = current_state;
	}
}


WRITE16_HANDLER( btoads_misc_control_w )
{
	COMBINE_DATA(&misc_control);

	/* bit 3 controls sound reset line */
	cpu_set_reset_line(1, (misc_control & 8) ? CLEAR_LINE : ASSERT_LINE);
}


WRITE16_HANDLER( btoads_display_control_w )
{
	if (ACCESSING_MSB)
	{
		/* allow multiple changes during display */
		update_display_state();

		/* bit 15 controls which page is rendered and which page is displayed */
		if (data & 0x8000)
		{
			vram_fg_draw = (UINT8 *)btoads_vram_fg1;
			vram_fg_display = (UINT8 *)btoads_vram_fg0;
		}
		else
		{
			vram_fg_draw = (UINT8 *)btoads_vram_fg0;
			vram_fg_display = (UINT8 *)btoads_vram_fg1;
		}

		/* stash the remaining data for later */
		current_state.screen_control = data >> 8;
	}
}



/*************************************
 *
 *	Scroll registers
 *
 *************************************/

WRITE16_HANDLER( btoads_scroll0_w )
{
	/* allow multiple changes during display */
	update_display_state();

	/* upper bits are Y scroll, lower bits are X scroll */
	if (ACCESSING_MSB)
		current_state.yscroll0 = data >> 8;
	if (ACCESSING_LSB)
		current_state.xscroll0 = data & 0xff;
}


WRITE16_HANDLER( btoads_scroll1_w )
{
	/* allow multiple changes during display */
	update_display_state();

	/* upper bits are Y scroll, lower bits are X scroll */
	if (ACCESSING_MSB)
		current_state.yscroll1 = data >> 8;
	if (ACCESSING_LSB)
		current_state.xscroll1 = data & 0xff;
}



/*************************************
 *
 *	Palette RAM
 *
 *************************************/

WRITE16_HANDLER( btoads_paletteram_w )
{
	switch (offset)
	{
		/* select which palette entry */
		case 0:
			palette_entry = data;
			palette_color = 0;
			break;

		/* 3 successive writes here completes a palette write */
		case 2:
			palette_data[palette_color++] = data;
			if (palette_color == 3)
			{
				int r = (palette_data[0] << 2) | (palette_data[0] >> 4);
				int g = (palette_data[1] << 2) | (palette_data[1] >> 4);
				int b = (palette_data[2] << 2) | (palette_data[2] >> 4);
				palette_set_color(palette_entry, r, g, b);
				palette_entry++;
				palette_color = 0;
			}
			break;

		/* unknown behavior */
		case 4:
			logerror("btoads_paletteram_w(4) = %04X\n", data);
			break;
	}
}



/*************************************
 *
 *	Background video RAM
 *
 *************************************/

WRITE16_HANDLER( btoads_vram_bg0_w )
{
	COMBINE_DATA(&btoads_vram_bg0[offset & 0x3fcff]);
}


WRITE16_HANDLER( btoads_vram_bg1_w )
{
	COMBINE_DATA(&btoads_vram_bg1[offset & 0x3fcff]);
}


READ16_HANDLER( btoads_vram_bg0_r )
{
	return btoads_vram_bg0[offset & 0x3fcff];
}


READ16_HANDLER( btoads_vram_bg1_r )
{
	return btoads_vram_bg1[offset & 0x3fcff];
}



/*************************************
 *
 *	Foreground video RAM
 *
 *************************************/

WRITE16_HANDLER( btoads_vram_fg_display_w )
{
	if (ACCESSING_LSB)
		vram_fg_display[offset] = data;
}


WRITE16_HANDLER( btoads_vram_fg_draw_w )
{
	if (ACCESSING_LSB)
		vram_fg_draw[offset] = data;
}


READ16_HANDLER( btoads_vram_fg_display_r )
{
	return vram_fg_display[offset];
}


READ16_HANDLER( btoads_vram_fg_draw_r )
{
	return vram_fg_draw[offset];
}



/*************************************
 *
 *	Sprite rendering
 *
 *************************************/

static void render_sprite_row(UINT16 *sprite_source, UINT32 address)
{
	int flipxor = ((*btoads_sprite_control >> 10) & 1) ? 0xffff : 0x0000;
	int width = (~*btoads_sprite_control & 0x1ff) + 2;
	int color = (~*btoads_sprite_control >> 8) & 0xf0;
	int srcoffs = sprite_source_offs << 8;
	int srcend = srcoffs + (width << 8);
	int srcstep = 0x100 - btoads_sprite_scale[0];
	int dststep = 0x100 - btoads_sprite_scale[8];
	int dstoffs = sprite_dest_offs << 8;

	/* non-shadow case */
	if (!(misc_control & 0x10))
	{
		for ( ; srcoffs < srcend; srcoffs += srcstep, dstoffs += dststep)
		{
			UINT16 src = sprite_source[(srcoffs >> 10) & 0x1ff];
			if (src)
			{
				src = (src >> (((srcoffs ^ flipxor) >> 6) & 0x0c)) & 0x0f;
				if (src)
					sprite_dest_base[(dstoffs >> 8) & 0x1ff] = src | color;
			}
		}
	}

	/* shadow case */
	else
	{
		for ( ; srcoffs < srcend; srcoffs += srcstep, dstoffs += dststep)
		{
			UINT16 src = sprite_source[(srcoffs >> 10) & 0x1ff];
			if (src)
			{
				src = (src >> (((srcoffs ^ flipxor) >> 6) & 0x0c)) & 0x0f;
				if (src)
					sprite_dest_base[(dstoffs >> 8) & 0x1ff] = color;
			}
		}
	}

	sprite_source_offs += width;
	sprite_dest_offs = dstoffs >> 8;
}



/*************************************
 *
 *	Shift register read/write
 *
 *************************************/

void btoads_to_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	address &= ~0x40000000;

	/* reads from this first region are usual shift register reads */
	if (address >= 0xa0000000 && address <= 0xa3ffffff)
		memcpy(shiftreg, &vram_fg_display[TOWORD(address & 0x3fffff)], TOBYTE(0x1000));

	/* reads from this region set the sprite destination address */
	else if (address >= 0xa4000000 && address <= 0xa7ffffff)
	{
		sprite_dest_base = &vram_fg_draw[TOWORD(address & 0x3fc000)];
		sprite_dest_offs = (INT16)((address & 0x3ff) << 2) >> 2;
	}

	/* reads from this region set the sprite source address */
	else if (address >= 0xa8000000 && address <= 0xabffffff)
	{
		memcpy(shiftreg, &btoads_vram_fg_data[TOWORD(address & 0x7fc000)], TOBYTE(0x2000));
		sprite_source_offs = (address & 0x003fff) >> 3;
	}

	else
		logerror("%08X:btoads_to_shiftreg(%08X)\n", cpu_get_pc(), address);
}


void btoads_from_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	address &= ~0x40000000;

	/* writes to this first region are usual shift register writes */
	if (address >= 0xa0000000 && address <= 0xa3ffffff)
		memcpy(&vram_fg_display[TOWORD(address & 0x3fc000)], shiftreg, TOBYTE(0x1000));

	/* writes to this region are ignored for our purposes */
	else if (address >= 0xa4000000 && address <= 0xa7ffffff)
		;

	/* writes to this region copy standard data */
	else if (address >= 0xa8000000 && address <= 0xabffffff)
		memcpy(&btoads_vram_fg_data[TOWORD(address & 0x7fc000)], shiftreg, TOBYTE(0x2000));

	/* writes to this region render the current sprite data */
	else if (address >= 0xac000000 && address <= 0xafffffff)
		render_sprite_row(shiftreg, address);

	else
		logerror("%08X:btoads_from_shiftreg(%08X)\n", cpu_get_pc(), address);
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void btoads_vh_eof(void)
{
	/* reset the display state counter */
	state_index = 0;
}


void btoads_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	struct display_state *curstate = state_cache;
	int x, y;

	/* add a final state for the last scanline */
	if (state_index == 0 || state_cache[state_index - 1].scanline != 224)
	{
		current_state.scanline = 224;
		state_cache[state_index] = current_state;
	}

	/* loop over all scanlines */
	for (y = 0; y < 224; y++)
	{
		UINT16 *bg0_base = &btoads_vram_bg0[((y + curstate->yscroll0) & 0xff) * TOWORD(0x4000)];
		UINT16 *bg1_base = &btoads_vram_bg1[((y + curstate->yscroll1) & 0xff) * TOWORD(0x4000)];
		UINT8 *spr_base = &vram_fg_display[y * TOWORD(0x4000)];
		UINT8 scanline[512];
		UINT8 *dst = scanline;

		/* for each scanline, switch off the render mode */
		switch (curstate->screen_control & 3)
		{
			/* mode 0: used in ship level, snake boss, title screen (free play) */
			case 0:

			/* mode 2: used in EOA screen, jetpack level, first level, high score screen */
			case 2:
				for (x = 0; x < 256; x++, dst += 2)
				{
					UINT8 sprpix = spr_base[x];

					if (sprpix)
					{
						dst[0] = sprpix;
						dst[1] = sprpix;
					}
					else
					{
						UINT16 bg0pix = bg0_base[(x + curstate->xscroll0) & 0xff];
						UINT16 bg1pix = bg1_base[(x + curstate->xscroll1) & 0xff];

						if (bg1pix & 0xff)
							dst[0] = bg1pix;
						else
							dst[0] = bg0pix;

						if (bg1pix >> 8)
							dst[1] = bg1pix >> 8;
						else
							dst[1] = bg0pix >> 8;
					}
				}
				break;

			/* mode 1: used in snow level, title screen (free play), top part of rolling ball level */
			case 1:
				for (x = 0; x < 256; x++, dst += 2)
				{
					UINT8 sprpix = spr_base[x];

					if (sprpix && !(sprpix & 0x80))
					{
						dst[0] = sprpix;
						dst[1] = sprpix;
					}
					else
					{
						UINT16 bg0pix = bg0_base[(x + curstate->xscroll0) & 0xff];
						UINT16 bg1pix = bg1_base[(x + curstate->xscroll1) & 0xff];

						if (bg0pix & 0xff)
							dst[0] = bg0pix;
						else if (bg1pix & 0x80)
							dst[0] = bg1pix;
						else if (sprpix)
							dst[0] = sprpix;
						else
							dst[0] = bg1pix;

						if (bg0pix >> 8)
							dst[1] = bg0pix >> 8;
						else if (bg1pix & 0x8000)
							dst[1] = bg1pix >> 8;
						else if (sprpix)
							dst[1] = sprpix;
						else
							dst[1] = bg1pix >> 8;
					}
				}
				break;

			/* mode 3: used in toilet level, toad intros, bottom of rolling ball level */
			case 3:
				for (x = 0; x < 256; x++, dst += 2)
				{
					UINT16 bg0pix = bg0_base[(x + curstate->xscroll0) & 0xff];
					UINT16 bg1pix = bg1_base[(x + curstate->xscroll1) & 0xff];
					UINT8 sprpix = spr_base[x];

					if (bg1pix & 0x80)
						dst[0] = bg1pix;
					else if (sprpix & 0x80)
						dst[0] = sprpix;
					else if (bg1pix & 0xff)
						dst[0] = bg1pix;
					else if (sprpix)
						dst[0] = sprpix;
					else
						dst[0] = bg0pix;

					if (bg1pix & 0x8000)
						dst[1] = bg1pix >> 8;
					else if (sprpix & 0x80)
						dst[1] = sprpix;
					else if (bg1pix >> 8)
						dst[1] = bg1pix >> 8;
					else if (sprpix)
						dst[1] = sprpix;
					else
						dst[1] = bg0pix >> 8;
				}
				break;
		}

		/* render the scanline */
		draw_scanline8(bitmap, 0, y, 512, scanline, NULL, -1);

		/* switch to the next state if we need to */
		if (y >= curstate->scanline)
			curstate++;
	}

	/* debugging - dump the screen contents to a file */
#if DEBUG
	if (keyboard_pressed(KEYCODE_X))
	{
		static int count = 0;
		char name[10];
		FILE *f;
		int i;

		sprintf(name, "disp%d.log", count++);
		f = fopen(name, "w");
		fprintf(f, "screen_control = %04X\n\n", current_state.screen_control << 8);

		for (i = 0; i < 3; i++)
		{
			UINT16 *base = (i == 0) ? (UINT16 *)vram_fg_display : (i == 1) ? btoads_vram_bg0 : btoads_vram_bg1;
			int xscr = (i == 0) ? 0 : (i == 1) ? current_state.xscroll0 : current_state.xscroll1;
			int yscr = (i == 0) ? 0 : (i == 1) ? current_state.yscroll0 : current_state.yscroll1;

			for (y = 0; y < 224; y++)
			{
				UINT32 offs = ((y + yscr) & 0xff) * TOWORD(0x4000);
				for (x = 0; x < 256; x++)
				{
					UINT16 pix = base[offs + ((x + xscr) & 0xff)];
					fprintf(f, "%02X%02X", pix & 0xff, pix >> 8);
					if (x % 16 == 15) fprintf(f, " ");
				}
				fprintf(f, "\n");
			}
			fprintf(f, "\n\n");
		}
		fclose(f);
	}

	logerror("---VBLANK---\n");
#endif
}
