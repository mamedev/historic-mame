/***************************************************************************
Amiga Computer / Arcadia System - (c) 1988, Arcadia Systems.

Driver by:

Ernesto Corvi and Mariusz Wojcieszek

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amiga.h"

extern custom_regs_def custom_regs;
#define DT_COLOR_WHITE 0;

struct update_regs_def {
/* display window */
	int v_start;
	int v_stop;
	int h_start;
	int h_stop;
	int old_DIWSTRT;
	int old_DIWSTOP;
	int h_scroll[2];
/* display data fetch */
	int ddf_start_pixel;
	int ddf_word_count;
	int old_DDFSTRT;
/* sprites */
	int sprite_v_start[8];
	int sprite_v_stop[8];
	int sprite_h_start[8];
	int sprite_attached[8];
	int sprite_dma_enabled;
	UINT16 sprite_data[8][2];
/* update time variables */
	int	current_bit;
	int fetch_pending;
	int fetch_count;
	unsigned short back_color;
	int old_COLOR0;
	unsigned char *RAM;
	unsigned int *sprite_in_scanline;
	int once_per_frame; /* for unimplemented modes */
/* HAM mode */
	UINT16 last_ham_pixel_color;
};

static struct update_regs_def update_regs;


/***************************************************************************

    Copper emulation

***************************************************************************/

#define COPPER_COLOR_CLOCKS_PER_INST 4

typedef struct {
	unsigned long pc;
	int waiting;
	int wait_v_pos;
	int wait_h_pos;
	int waitforblit;
	int enabled;
} copper_def;

static copper_def copper;

void copper_setpc( unsigned long pc ) {
	copper.pc = pc;
	copper.waiting = 0;
}

void copper_enable( void ) {
	copper.enabled = ( custom_regs.DMACON & ( DMACON_COPEN | DMACON_DMAEN ) ) == ( DMACON_COPEN | DMACON_DMAEN );
}

INLINE void copper_reset( void ) {
	copper.pc = ( custom_regs.COPLCH[0] << 16 ) | custom_regs.COPLCL[0];
	copper.waiting = 0;
}

INLINE int copper_update( int x_pos, int y_pos, int *end_x ) {
	int i, inst, param;

	if ( copper.enabled == 0 )
		return 1;

	/*********************************************************************************

        The Copper can only read the lower 8 bits of the vertical counter. To access
        the next 6 lines, you need to wait for line 255, and then you can check from
        lines 0 to 6 (corresponding to 256 to 261) to do things before it wraps again.

     ********************************************************************************/

	y_pos &= 0xff;

	/* convert the x coordinate to color clocks coordinate */
	x_pos = ( x_pos >> 1 );

	/* 56.794 instructions per line max (280 ns per inst) */
	for ( i = 0; i < 57; i++ ) {

		/* see if we are in a waiting state */
		if ( copper.waiting ) {
			if ( y_pos < copper.wait_v_pos ) { /* are we in the right scanline yet? */
				return 1; /* we arent, render the whole line as is */
			}

			if ( y_pos == copper.wait_v_pos ) {

				if ( x_pos < copper.wait_h_pos ) {
					/* see if we can ever reach it, otherwise, finish the line */
					if ( copper.wait_h_pos > ( ( Machine->drv->screen_width - 1 ) >> 1 ) )
						return 1;

					*end_x = ( copper.wait_h_pos << 1 );
					return 0;
				}
			}

			/* if we're told to wait for the blitter too, check for it now */
			if ( copper.waitforblit ) {
				if ( custom_regs.DMACON & DMACON_BBUSY )
					return 1;
				else
					copper.waitforblit = 0;
			}

			copper.waiting = 0;
			copper.pc += 4;
		}

		inst = *((UINT16 *) ( &(update_regs.RAM[copper.pc]) ));
		param = *((UINT16 *) ( &(update_regs.RAM[copper.pc + 2]) ));

		if ( !( inst & 1 ) ) { /* MOVE instruction */
			int min = 0x80 - ( custom_regs.COPCON << 5 );

			inst &= 0x1fe;

			copper.pc += 4;

			if ( inst >= min )	/* If not invalid, go at it */
				/* KT - I've no idea what the memory mask should be so
                added -1 for now! */
			    amiga_custom_w( inst>>1, param , 0 /*???*/);
			else {
				/* stop the copper until the next frame */
				copper.waiting = 1;
				copper.wait_v_pos = 0xff;
				copper.wait_h_pos = 0xfe;
				return 1;
			}

			x_pos += COPPER_COLOR_CLOCKS_PER_INST;
			continue;
		}

		if ( !( param & 1 ) ) { /* WAIT instruction */
			copper.waiting = 1;
			copper.wait_v_pos = ( inst >> 8 ) & 0xff;
			copper.wait_v_pos &= ( ( param >> 8 ) & 0x7f ) | 0x80;
			copper.wait_h_pos = ( inst & 0xfe );
			copper.wait_h_pos &= ( param & 0xfe );
			copper.waitforblit = !( ( param >> 15 ) & 1 );
			x_pos += COPPER_COLOR_CLOCKS_PER_INST;
			continue;
		}

		if ( ( inst & 1 ) && ( param & 1 ) ) { /* SKIP instruction */
			int skip_v_pos, skip_h_pos, skip_waitforblit;

			skip_v_pos = ( inst >> 8 ) & 0xff;
			skip_v_pos &= ( param >> 8 ) & 0x7f;
			skip_h_pos = ( inst & 0xfe );
			skip_h_pos &= ( param & 0xfe );
			skip_waitforblit = !( ( param >> 15 ) & 1 );

			if ( y_pos < skip_v_pos ) {
				copper.pc += 4;
				x_pos += COPPER_COLOR_CLOCKS_PER_INST;
				continue;
			}

			if ( y_pos == skip_v_pos ) {
				if ( x_pos < skip_h_pos ) {
					copper.pc += 4;
					x_pos += COPPER_COLOR_CLOCKS_PER_INST;
					continue;
				}
			}

			if ( skip_waitforblit ) {
				if ( custom_regs.DMACON & DMACON_BBUSY ) {
					copper.pc += 4;
					x_pos += COPPER_COLOR_CLOCKS_PER_INST;
					continue;
				}
			}

			copper.pc += 8;
			x_pos += COPPER_COLOR_CLOCKS_PER_INST;
			continue;
		}

		/* if we got here, we're stuck in a illegal copper instruction */
		logerror("This program attempted to execute an illegal copper instruction!\n" );

		/* stop the copper until the next frame */
		copper.waiting = 1;
		copper.wait_v_pos = 0xff;
		copper.wait_h_pos = 0xfe;

		return 1;
	}

	/* if we got here, we ran enough instructions for this line, thus we complete it */
	return 1;
}

/***************************************************************************

    Sprites emulation

***************************************************************************/

void amiga_sprite_set_pos( int spritenum, unsigned short data ) {

	update_regs.sprite_v_start[spritenum] &= 0x100;
	update_regs.sprite_v_start[spritenum] |= data >> 8;

	update_regs.sprite_h_start[spritenum] &= 0x01;
	update_regs.sprite_h_start[spritenum] |= ( data << 1 ) & 0x1fe;

}

void amiga_sprite_set_ctrl( int spritenum, unsigned short data ) {

	update_regs.sprite_h_start[spritenum] &= 0x1fe;
	update_regs.sprite_h_start[spritenum] |= data & 1;

	update_regs.sprite_v_start[spritenum] &= 0xff;
	update_regs.sprite_v_start[spritenum] |= ( data << 6 ) & 0x100;

	update_regs.sprite_v_stop[spritenum] = ( data << 7 ) & 0x100;
	update_regs.sprite_v_stop[spritenum] |= data >> 8;

	if ( spritenum & 1 )
		update_regs.sprite_attached[spritenum] = data & 0x80;

	/* safety */
	if ( update_regs.sprite_v_start[spritenum] > update_regs.sprite_v_stop[spritenum] )
		update_regs.sprite_v_stop[spritenum] = update_regs.sprite_v_start[spritenum];

}

void amiga_reload_sprite_info( int spritenum ) {

	unsigned char *RAM = memory_region(REGION_CPU1);

	amiga_sprite_set_pos( spritenum, *((UINT16 *) &RAM[custom_regs.SPRxPT[spritenum]] ) );

	amiga_sprite_set_ctrl( spritenum, *((UINT16 *) &RAM[custom_regs.SPRxPT[spritenum] + 2] ) );

}

INLINE void amiga_render_sprite( int num, int x, int y, unsigned short *dst ) {
	int bit;

	if ( update_regs.sprite_dma_enabled ) {

		if ( y < update_regs.sprite_v_start[num] )
			return;

		if ( x < update_regs.sprite_h_start[num] || x > update_regs.sprite_h_start[num] + 15 )
			return;

		bit = 15 - ( x - update_regs.sprite_h_start[num] );

		/* check for attached sprites */
		if ( num < 7 && update_regs.sprite_attached[num+1] ) {
			unsigned short word[4];
			int color, i;

			word[0] = update_regs.sprite_data[num][0];
			word[1] = update_regs.sprite_data[num][1];
			word[2] = update_regs.sprite_data[num+1][0];
			word[3] = update_regs.sprite_data[num+1][1];

			color = 0;

			for( i = 0; i < 4; i++ )
				color |= ( ( word[i] >> bit ) & 1 ) << i;

			if ( color )
				dst[x] = Machine->pens[custom_regs.COLOR[color+16]];
		} else {
			if ( !update_regs.sprite_attached[num] ) {
				unsigned short word[2];
				int color, i;

				word[0] = update_regs.sprite_data[num][0];
				word[1] = update_regs.sprite_data[num][1];

				color = 0;

				for( i = 0; i < 2; i++ )
					color |= ( ( word[i] >> bit ) & 1 ) << i;

				if ( color ) {
					color += 16 + ( ( num >> 1 ) << 2 );
					dst[x] = Machine->pens[custom_regs.COLOR[color]];
				}
			}
		}

	}
}

void amiga_sprite_dma( int scanline )
{
	int num;
	UINT16 dataA, dataB;

	if ( ( custom_regs.DMACON & ( DMACON_SPREN | DMACON_DMAEN ) ) == ( DMACON_SPREN | DMACON_DMAEN ) )
	{
		for ( num = 0; num < 8; num++ )
		{
			if ( scanline >= update_regs.sprite_v_start[num] && scanline <= update_regs.sprite_v_stop[num] )
			{
				custom_regs.SPRxPT[num] += 4;
				dataA = *((UINT16 *) &update_regs.RAM[custom_regs.SPRxPT[num]] );
				dataB = *((UINT16 *) &update_regs.RAM[custom_regs.SPRxPT[num]+2] );

				if ( scanline >= update_regs.sprite_v_stop[num] )
				{
					amiga_sprite_set_pos( num, dataA );
					amiga_sprite_set_ctrl( num, dataB );
					update_regs.sprite_in_scanline[scanline] = 0;
				}
				else
				{
					update_regs.sprite_data[num][0] = dataA;
					update_regs.sprite_data[num][1] = dataB;
					update_regs.sprite_in_scanline[scanline] = update_regs.sprite_h_start[num] + 16;
				}
			}
		}
	}
}

/***************************************************************************

    Raster emulation

***************************************************************************/

INLINE void amiga_display_msg (mame_bitmap *bitmap, const char *str ) {
	if ( update_regs.once_per_frame == 0 )
		ui_draw_text(str, 10, 10);

	update_regs.once_per_frame = 1;
}


INLINE void update_modulos ( int planes ) {
	int i;

	for ( i = 0; i < planes; i++ )
		if ( i & 1 )
			custom_regs.BPLPTR[i] += custom_regs.BPL2MOD;
		else
			custom_regs.BPLPTR[i] += custom_regs.BPL1MOD;
}

INLINE void init_update_regs( void ) {
	int	ddf_color_clocks_offs, ddf_res_offs;

	if ( update_regs.old_DIWSTRT != custom_regs.DIWSTRT ) {
		/* display window */
		update_regs.v_start = ( custom_regs.DIWSTRT >> 8 ) & 0xff;
		update_regs.h_start	= custom_regs.DIWSTRT & 0xff;
		update_regs.old_DIWSTRT = custom_regs.DIWSTRT;
	}

	if ( update_regs.old_DIWSTOP != custom_regs.DIWSTOP ) {
		/* display window */
		update_regs.v_stop = ( custom_regs.DIWSTOP >> 8 ) & 0xff;
		update_regs.v_stop |= ( ( update_regs.v_stop << 1 ) ^ 0x0100 ) & 0x0100; /* bit 8 = !bit 7 */
		update_regs.h_stop = ( custom_regs.DIWSTOP & 0xff ) | 0x0100;
		update_regs.old_DIWSTOP = custom_regs.DIWSTOP;
	}

	if ( update_regs.old_DDFSTRT != custom_regs.DDFSTRT ) {
		update_regs.fetch_pending = 1;
		update_regs.fetch_count = 0;
		update_regs.old_DDFSTRT = custom_regs.DDFSTRT;
	}

	/* display data fetch */
	if ( custom_regs.BPLCON0 & BPLCON0_HIRES ) {
		ddf_color_clocks_offs = 9; /* 4.5 color clocks * 2 in hi res */
		ddf_res_offs = 2;
	} else {
		ddf_color_clocks_offs = 17; /* 8.5 color clocks * 2 in lo res */
		ddf_res_offs = 3;
	}

	update_regs.ddf_start_pixel = ( custom_regs.DDFSTRT << 1 ) + ddf_color_clocks_offs;
	update_regs.ddf_word_count = ( -( custom_regs.DDFSTRT - custom_regs.DDFSTOP - 12 ) ) >> ddf_res_offs;

	if ( update_regs.old_COLOR0 != custom_regs.COLOR[0] ) {
		update_regs.back_color = Machine->pens[custom_regs.COLOR[0]];
		update_regs.old_COLOR0 = custom_regs.COLOR[0];
	}

	update_regs.sprite_dma_enabled = ( custom_regs.DMACON & ( DMACON_SPREN | DMACON_DMAEN ) ) == ( DMACON_SPREN | DMACON_DMAEN );
	update_regs.h_scroll[0] = custom_regs.BPLCON1 & 0xf;
	update_regs.h_scroll[1] = (custom_regs.BPLCON1 >> 4) & 0xf;
}

/***********************************************************************************

    Common update stuff coming. (aka abusing the preprocessor)

***********************************************************************************/
#define BEGIN_UPDATE( name ) \
static void name(mame_bitmap *bitmap, unsigned short *dst, int planes, int x, int y, int min_x ) { \
	int i; \
	if ( x < update_regs.ddf_start_pixel ) { /* see if we need to start fetching */ \
		return; \
	} else { \
		/* Now we have to figure out if we need to fetch another 16 bit word */ \
		if ( update_regs.fetch_pending ) { \
			/* see if we are past DDFSTOP */ \
			if ( ++update_regs.fetch_count > update_regs.ddf_word_count ) { \
				return; \
			} \
			/* fetch the new word from the bitplane pointers, and update them */ \
			for ( i = 0; i < planes; i++ ) { \
				custom_regs.BPLxDAT[i] = *((UINT16 *) &(update_regs.RAM[custom_regs.BPLPTR[i]]) ); \
				custom_regs.BPLPTR[i] += 2; \
			} \
			update_regs.current_bit = 15; \
			update_regs.fetch_pending = 0; \
		} \
		x += update_regs.h_scroll[0]; \
		if ( x >= min_x ) { \
			/* before rendering the pixel, see if we still are within display window bounds */ \
			if ( x >= update_regs.h_start && x <= update_regs.h_stop ) { \


#if 0 /* this is a small kludge for the Mac source code editor */
} } } }
#endif

#define BEGIN_UPDATE_WITH_SPRITES( name ) \
static void name(mame_bitmap *bitmap, unsigned short *dst, int planes, int x, int y, int min_x ) { \
	int i; \
	if ( x < update_regs.ddf_start_pixel ) { /* see if we need to start fetching */ \
		if ( x < update_regs.h_stop ) { \
			for ( i = 0; i < 8; i++ ) \
				amiga_render_sprite( i, x, y, dst ); \
		} \
		return; \
	} else { \
		/* Now we have to figure out if we need to fetch another 16 bit word */ \
		if ( update_regs.fetch_pending ) { \
			/* see if we are past DDFSTOP */ \
			if ( ++update_regs.fetch_count > update_regs.ddf_word_count ) { \
				if ( x < update_regs.h_stop ) { \
					for ( i = 0; i < 8; i++ ) \
						amiga_render_sprite( i, x, y, dst ); \
				} \
				return; \
			} \
			/* fetch the new word from the bitplane pointers, and update them */ \
			for ( i = 0; i < planes; i++ ) { \
				custom_regs.BPLxDAT[i] = *((UINT16 *) &(update_regs.RAM[custom_regs.BPLPTR[i]]) ); \
				custom_regs.BPLPTR[i] += 2; \
			} \
			update_regs.current_bit = 15; \
			update_regs.fetch_pending = 0; \
		} \
		x += update_regs.h_scroll[0]; \
		if ( x >= min_x ) { \
			/* before rendering the pixel, see if we still are within display window bounds */ \
			if ( x >= update_regs.h_start && x <= update_regs.h_stop ) { \

#define END_UPDATE( curbit ) \
			} \
		} \
		update_regs.current_bit -= curbit; \
		/* see if we're done with this word */ \
		if ( update_regs.current_bit < 0 ) \
			update_regs.fetch_pending = 1; /* signal we need a new fetch */ \
	} \
}

#define END_UPDATE_WITH_SPRITES( curbit ) \
			} \
		} \
		x -= update_regs.h_scroll[0]; \
		if ( x >= min_x ) \
			for ( i = 0; i < 8; i++ ) \
				amiga_render_sprite(i, x, y, dst); \
		update_regs.current_bit -= curbit; \
		/* see if we're done with this word */ \
		if ( update_regs.current_bit < 0 ) \
			update_regs.fetch_pending = 1; /* signal we need a new fetch */ \
	} \
}

#define UNIMPLEMENTED( name ) \
	static void name(mame_bitmap *bitmap, unsigned short *dst, int planes, int x, int y, int min_x ) { \
		amiga_display_msg(bitmap,  "Unimplemented screen mode: "#name ); \
	}


/***********************************************************************************

    Low Resolution handlers

***********************************************************************************/

BEGIN_UPDATE( render_pixel_lores ) {
	/* now we're ready to render it */
	int color = 0;

	for ( i = 0; i < planes; i++ ) {
		color |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << i;
	}

	dst[x] = Machine->pens[custom_regs.COLOR[color]];

} END_UPDATE( 1 )

BEGIN_UPDATE_WITH_SPRITES( render_pixel_lores_sprites ) {
	/* now we're ready to render it */
	int color = 0;

	for ( i = 0; i < planes; i++ ) {
		color |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << i;
	}

	dst[x] = Machine->pens[custom_regs.COLOR[color]];

} END_UPDATE_WITH_SPRITES( 1 )

UNIMPLEMENTED( render_pixel_lores_lace )
UNIMPLEMENTED( render_pixel_lores_lace_sprites )

/***********************************************************************************

    High Resolution handlers

***********************************************************************************/

BEGIN_UPDATE( render_pixel_hires ) {
	/* now we're ready to render it */
	int color = 0;

	for ( i = 0; i < planes; i++ ) {
		color |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << i;
	}

	dst[x] = Machine->pens[custom_regs.COLOR[color]];

} END_UPDATE( 2 )

BEGIN_UPDATE_WITH_SPRITES( render_pixel_hires_sprites ) {
	/* now we're ready to render it */
	int color = 0;

	for ( i = 0; i < planes; i++ ) {
		color |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << i;
	}

	dst[x] = Machine->pens[custom_regs.COLOR[color]];

} END_UPDATE_WITH_SPRITES( 2 )

UNIMPLEMENTED( render_pixel_hires_lace )
UNIMPLEMENTED( render_pixel_hires_lace_sprites )

/***********************************************************************************

    Dual Playfield handlers

***********************************************************************************/

BEGIN_UPDATE( render_pixel_dualplayfield_lores ) {
	/* now we're ready to render it */
	int color[2];
	color[0] = color[1] = 0;

	for ( i = 0; i < planes; i++ ) {
		if ( i & 1 ) {
			color[1] |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << ( i >> 1 );
		} else {
			color[0] |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << ( i >> 1 );
		}
	}

	if ( color[0] || color[1] ) { /* if theres a pixel to draw */
		if ( custom_regs.BPLCON2 & 0x40 ) { /* check wich playfield has priority */
			if ( color[1] )
				dst[x] = Machine->pens[custom_regs.COLOR[color[1]+8]];
			else
				dst[x] = Machine->pens[custom_regs.COLOR[color[0]]];
		} else {
			if ( color[0] )
				dst[x] = Machine->pens[custom_regs.COLOR[color[0]]];
			else
				dst[x] = Machine->pens[custom_regs.COLOR[color[1]+8]];
		}
	} else
		dst[x] = update_regs.back_color;
} END_UPDATE( 1 )

UNIMPLEMENTED( render_pixel_dualplayfield_hires )
UNIMPLEMENTED( render_pixel_dualplayfield_lores_lace )
UNIMPLEMENTED( render_pixel_dualplayfield_hires_lace )
UNIMPLEMENTED( render_pixel_dualplayfield_lores_sprites )
UNIMPLEMENTED( render_pixel_dualplayfield_hires_sprites )
UNIMPLEMENTED( render_pixel_dualplayfield_lores_lace_sprites )
UNIMPLEMENTED( render_pixel_dualplayfield_hires_lace_sprites )

/***********************************************************************************

    HAM handlers

***********************************************************************************/

BEGIN_UPDATE( render_pixel_ham ) {
	/* now we're ready to render it */
	int color = 0;

	for ( i = 0; i < planes; i++ ) {
		color |= ( ( ( custom_regs.BPLxDAT[i] ) >> update_regs.current_bit ) & 1 ) << i;
	}

	switch( (color >> 4) & 0x3 )
	{
		case 0x0:
			update_regs.last_ham_pixel_color = custom_regs.COLOR[color];
			break;
		case 0x1:
			update_regs.last_ham_pixel_color = (update_regs.last_ham_pixel_color & 0x0ff0) | (color & 0xf);
			break;
		case 0x2:
			update_regs.last_ham_pixel_color = (update_regs.last_ham_pixel_color & 0xf0ff) | ((color & 0xf) << 8);
			break;
		case 0x3:
			update_regs.last_ham_pixel_color = (update_regs.last_ham_pixel_color & 0xff0f) | ((color & 0xf) << 4);
			break;
	}

	dst[x] = Machine->pens[update_regs.last_ham_pixel_color];

} END_UPDATE(1)

UNIMPLEMENTED( render_pixel_ham_lace )
UNIMPLEMENTED( render_pixel_ham_sprites )
UNIMPLEMENTED( render_pixel_ham_lace_sprites )

typedef void (*render_pixel_def)(mame_bitmap *bitmap, unsigned short *dst, int planes, int x, int y, int min_x );

static render_pixel_def render_pixel[] = {
	render_pixel_lores,
	render_pixel_hires,
	render_pixel_dualplayfield_lores,
	render_pixel_dualplayfield_hires,
	render_pixel_ham,
	render_pixel_lores_lace,
	render_pixel_hires_lace,
	render_pixel_dualplayfield_lores_lace,
	render_pixel_dualplayfield_hires_lace,
	render_pixel_ham_lace,
	render_pixel_lores_sprites,
	render_pixel_hires_sprites,
	render_pixel_dualplayfield_lores_sprites,
	render_pixel_dualplayfield_hires_sprites,
	render_pixel_ham_sprites,
	render_pixel_lores_lace_sprites,
	render_pixel_hires_lace_sprites,
	render_pixel_dualplayfield_lores_lace_sprites,
	render_pixel_dualplayfield_hires_lace_sprites,
	render_pixel_ham_lace_sprites
};

#if 0
static char mode_names[20][48] = {
	"LoRes",
	"HiRes",
	"DPF LoRes",
	"DPF HiRes",
	"HAM",
	"LoRes Interlace",
	"HiRes Interlace",
	"DPF LoRes Interlace",
	"DPF HiRes Interlace",
	"HAM Interlace",
	"LoRes + Sprites",
	"HiRes + Sprites",
	"DPF LoRes + Sprites",
	"DPF HiRes + Sprites",
	"HAM + Sprites",
	"LoRes Interlace + Sprites",
	"HiRes Interlace + Sprites",
	"DPF LoRes Interlace + Sprites",
	"DPF HiRes Interlace + Sprites",
	"HAM Interlace + Sprites"
};
#endif

INLINE int get_mode( void ) {

	int ret = 0;

	ret += ( custom_regs.BPLCON0 & BPLCON0_LACE ) ? 5 : 0;

	if ( custom_regs.BPLCON0 & BPLCON0_HOMOD )
		return ( ret + 4 );

	ret += ( custom_regs.BPLCON0 & BPLCON0_HIRES ) ? 1 : 0;
	ret += ( custom_regs.BPLCON0 & BPLCON0_DBLPF ) ? 2 : 0;

	return ret;
}

VIDEO_START(amiga)
{
	/* init cached data */
	update_regs.old_COLOR0 = -1;
	update_regs.old_DIWSTRT = -1;
	update_regs.old_DIWSTOP = -1;
	update_regs.old_DDFSTRT = -1;
	update_regs.RAM = memory_region(REGION_CPU1);

	update_regs.sprite_in_scanline = auto_malloc( Machine->drv->screen_height * sizeof( int ) );
	if ( update_regs.sprite_in_scanline == 0 )
		return 1;

	memset( update_regs.sprite_in_scanline, 0, Machine->drv->screen_height * sizeof( int ) );

	return video_start_generic_bitmapped();
}

void amiga_prepare_frame(void)
{
	/* reset the copper */
	copper_reset();

	/* preload HAM pixel color */
	update_regs.last_ham_pixel_color = update_regs.back_color;

}

void amiga_render_scanline(int scanline)
{
	int planes = 0, sw = Machine->drv->screen_width;
	int min_x = Machine->visible_area.min_x;
	int y, x, start_x, end_x, line_done;
	unsigned short *dst;
	int bitplane_dma_disabled;
	render_pixel_def local_render;

	amiga_sprite_dma(scanline);

	y = scanline;

	/* normalize some register values */
	if ( custom_regs.DDFSTOP < custom_regs.DDFSTRT )
		custom_regs.DDFSTOP = custom_regs.DDFSTRT;

	/* start of a new line, signal we're not done with it and fill up vars */
	line_done = 0;
	start_x = 0;
	dst = ( unsigned short * )tmpbitmap->line[y];

	update_regs.fetch_pending = 1;
	update_regs.fetch_count = 0;

	/* make sure we complete the line */
	do {

		/* start of a new line... check if the copper is (still) enabled */
		line_done = copper_update( start_x, y, &end_x );

		if ( line_done )
			end_x = sw;

		/* precaulculate some update registers */
		init_update_regs();

		/* get the number of planes */
		planes = ( custom_regs.BPLCON0 & ( BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 ) ) >> 12;
		if ( planes == 6 && (custom_regs.BPLCON0 & (BPLCON0_DBLPF | BPLCON0_HOMOD)) == 0 )
		{
			planes = 5;
		}

		/* precalculate if the bitplane dma is enabled */
		bitplane_dma_disabled = ( custom_regs.DMACON & ( DMACON_BPLEN | DMACON_DMAEN ) ) != ( DMACON_BPLEN | DMACON_DMAEN );


		/***************************************************************************
            First we check for a number of conditions to see if we can render image
            pixels yet. Otherwise, we fill with the background color.
         **************************************************************************/

		if ( bitplane_dma_disabled || planes == 0 || y < update_regs.v_start || y >= update_regs.v_stop ) {
			for ( x = start_x; x < end_x; x++ )
				dst[x] = update_regs.back_color;
		} else { /* render the pixels (inlined for readability ) */
			if ( update_regs.sprite_in_scanline[y] >= update_regs.h_start )
				local_render = render_pixel[get_mode()+10];
			else
				local_render = render_pixel[get_mode()];

			/* first clear buffer */
			if ( start_x == 0 ) {
				for ( x = start_x; x < end_x; x++ )
					dst[x] = update_regs.back_color;
			} else {
				for ( x = start_x + update_regs.h_scroll[0]; x < end_x; x++ )
					dst[x] = update_regs.back_color;
			}

			for ( x = start_x; x < end_x; x++ )
				(*local_render)( tmpbitmap, dst, planes, x, y, min_x );
		}

		/* now we start from where we left off */
		start_x = end_x;

	} while ( !line_done );

	if ( y >= update_regs.v_start && y < update_regs.v_stop )
		update_modulos( planes ); /* update modulos */

	update_regs.sprite_in_scanline[y] = 0;

}


PALETTE_INIT( amiga )
{
	int i;

	for ( i = 0; i < 0x1000; i++ ) {
		int r, g, b;

		r = ( i >> 8 ) & 0x0f;
		g = ( i >> 4 ) & 0x0f;
		b = i & 0x0f;

		r = ( r << 4 ) | ( r );
		g = ( g << 4 ) | ( g );
		b = ( b << 4 ) | ( b );

		palette_set_color(i, r, g, b);
		colortable[i] = i;
	}
}

