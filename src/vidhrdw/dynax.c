/***************************************************************************

						-= Dynax / Nakanihon Games =-

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

				Q		Shows Layer 0
				W		Shows Layer 1
				E		Shows Layer 2

		Keys can be used together!


	There are three scrolling layers. Each layer consists of 2 frame
	buffers. The 2 images are blended together to form the final picture
	sent to the screen.

	The gfx roms do not contain tiles: the CPU controls a video blitter
	that can read data from them (instructions to draw pixel by pixel,
	in a compressed form) and write to the 6 frame buffers.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/dynax.h"

// Log Blitter
#define VERBOSE 0


WRITE_HANDLER( dynax_flipscreen_w )
{
	flip_screen_set( data & 1 );
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, flip screen <- %02X\n", activecpu_get_pc(), data);
}

/* 0 B01234 G01234 R01234 */
PALETTE_INIT( sprtmtch )
{
	int i;

	/* The bits are in reverse order */
	#define BITSWAP5( _x_ )	( (((_x_) & 0x01) ? 0x10 : 0) | \
							  (((_x_) & 0x02) ? 0x08 : 0) | \
							  (((_x_) & 0x04) ? 0x04 : 0) | \
							  (((_x_) & 0x08) ? 0x02 : 0) | \
							  (((_x_) & 0x10) ? 0x01 : 0) )

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int x =	(color_prom[i]<<8) + color_prom[0x200+i];
		int r = BITSWAP5((x >>  0) & 0x1f);
		int g = BITSWAP5((x >>  5) & 0x1f);
		int b = BITSWAP5((x >> 10) & 0x1f);
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(i,r,g,b);
	}
}

/***************************************************************************


								Video Blitter(s)


***************************************************************************/

UINT32 dynax_blit_reg;

UINT32 dynax_blit_x;
UINT32 dynax_blit_y;
UINT32 dynax_blit_scroll_x;
UINT32 dynax_blit_scroll_y;

UINT32 dynax_blit_address;
UINT32 dynax_blit_dest;

UINT32 dynax_blit_pen;
UINT32 dynax_blit_backpen;
UINT32 dynax_blit_palettes;
UINT32 dynax_blit_palbank;

UINT32 dynax_blit_enable;

// 3 layers, 2 images per layer (blended on screen)
UINT8 *dynax_pixmap[3][2];

/* Destination X */
WRITE_HANDLER( dynax_blit_x_w )
{
	dynax_blit_x = data;
#if VERBOSE
	logerror("X=%02X ",data);
#endif
}

/* Destination Y */
WRITE_HANDLER( dynax_blit_y_w )
{
	dynax_blit_y = data;
#if VERBOSE
	logerror("Y=%02X ",data);
#endif
}

WRITE_HANDLER( dynax_blit_scroll_w )
{
	// 0x800000 also used!
	if (dynax_blit_address & 0x400000)
	{
		dynax_blit_scroll_y = data;
#if VERBOSE
	logerror("SY=%02X ",data);
#endif
	}
	else
	{
		dynax_blit_scroll_x = data;
#if VERBOSE
	logerror("SX=%02X ",data);
#endif
	}
}

/* Source Address */
WRITE_HANDLER( dynax_blit_addr0_w )
{
	dynax_blit_address = (dynax_blit_address & ~0x0000ff) | (data<<0);
#if VERBOSE
	logerror("A0=%02X ",data);
#endif
}
WRITE_HANDLER( dynax_blit_addr1_w )
{
	dynax_blit_address = (dynax_blit_address & ~0x00ff00) | (data<<8);
#if VERBOSE
	logerror("A1=%02X ",data);
#endif
}
WRITE_HANDLER( dynax_blit_addr2_w )
{
	dynax_blit_address = (dynax_blit_address & ~0xff0000) | (data<<16);
#if VERBOSE
	logerror("A2=%02X ",data);
#endif
}

/* Destination Layers */
WRITE_HANDLER( dynax_blit_dest_w )
{
	dynax_blit_dest = data;
#if VERBOSE
	logerror("D=%02X ",data);
#endif
}

/* Destination Pen */
WRITE_HANDLER( dynax_blit_pen_w )
{
	dynax_blit_pen = data;
#if VERBOSE
	logerror("P=%02X ",data);
#endif
}

/* Background Color */
WRITE_HANDLER( dynax_blit_backpen_w )
{
	dynax_blit_backpen = data;
#if VERBOSE
	logerror("B=%02X ",data);
#endif
}

/* Layers 0&1 Palettes (Low Bits) */
WRITE_HANDLER( dynax_blit_palette01_w )
{
	dynax_blit_palettes = (dynax_blit_palettes & ~0xff) | data;
#if VERBOSE
	logerror("P1=%02X ",data);
#endif
}

/* Layer 2 Palette (Low Bits) */
WRITE_HANDLER( dynax_blit_palette2_w )
{
	dynax_blit_palettes = (dynax_blit_palettes & ~0xff00) | (data<<8);
#if VERBOSE
	logerror("P2=%02X ",data);
#endif
}

/* Layers Palettes (High Bits) */
WRITE_HANDLER( dynax_blit_palbank_w )
{
	dynax_blit_palbank = data;
#if VERBOSE
	logerror("PB=%02X ",data);
#endif
}

/* Layers Enable */
WRITE_HANDLER( dynax_blit_enable_w )
{
	dynax_blit_enable = data;
#if VERBOSE
	logerror("E=%02X ",data);
#endif
}


/***************************************************************************

							Blitter Data Format

	The blitter reads its commands from the gfx ROMs. They are
	instructions to draw an image pixel by pixel (in a compressed
	form) in a frame buffer.

	Fetch 1 Byte from the ROM:

	7654 ----	Pen to draw with
	---- 3210	Command

	Other bytes may follow, depending on the command

	Commands:

	0		Stop.
	1-b		Draw 1-b pixels along X.
	c		Followed by 1 byte (N): draw N pixels along X.
	d		Followed by 2 bytes (X,N): skip X pixels, draw N pixels along X.
	e		? unused
	f		Increment Y

***************************************************************************/


/* Plot a pixel (in the pixmaps specified by dynax_blit_dest) */
INLINE void sprtmtch_plot_pixel(int x, int y, int pen, int flags)
{
	/* "Flip Screen" just means complement the coordinates to 256 */
	if (flip_screen)	{	x = 0x100 - x;	y = 0x100 - y;	}

	/* Swap X with Y */
	if (flags & 0x08)	{ int t = x; x = y; y = t;	}

	/* Ignore the pens specified in ROM, draw everything with the
	   supplied one instead */
	if (flags & 0x02)	{ pen = (dynax_blit_pen >> 4) & 0xf;	}

	if (	(x >= 0) && (x <= 0xff) &&
			(y >= 0) && (y <= 0xff)	)
	{
		if (dynax_blit_dest & 0x01)	dynax_pixmap[0][0][(y<<8)|x] = pen;
		if (dynax_blit_dest & 0x02)	dynax_pixmap[0][1][(y<<8)|x] = pen;
		if (dynax_blit_dest & 0x04)	dynax_pixmap[1][0][(y<<8)|x] = pen;
		if (dynax_blit_dest & 0x08)	dynax_pixmap[1][1][(y<<8)|x] = pen;
		if (dynax_blit_dest & 0x10)	dynax_pixmap[2][0][(y<<8)|x] = pen;
		if (dynax_blit_dest & 0x20)	dynax_pixmap[2][1][(y<<8)|x] = pen;
	}
}


UINT32 sprtmtch_drawgfx( UINT32 i, UINT32 x, UINT32 y, UINT32 flags )
{
	data8_t cmd, pen, count;

	data8_t *SRC		=	memory_region( REGION_GFX1 );
	size_t   size_src	=	memory_region_length( REGION_GFX1 );

	int sx;

	if ( flags & 1 )
	{
		/* Clear the buffer(s) starting from the given scanline and exit */

		if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][0][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[0][1][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[1][0][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[1][1][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		if (dynax_blit_dest & 0x10)	memset(&dynax_pixmap[2][0][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		if (dynax_blit_dest & 0x20)	memset(&dynax_pixmap[2][1][y<<8],0,sizeof(UINT8)*(0x100-y)*0x100);
		return i;
	}

	sx = x;

	while ( y < Machine->drv->screen_height )
	{
		if (i >= size_src)	return i;
		cmd = SRC[i++];
		pen = (cmd & 0xf0)>>4;
		cmd = (cmd & 0x0f)>>0;

		switch (cmd)
		{
		case 0x0:	// Stop
			return i;

		case 0x1:	// Draw N pixels
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			count = cmd;
			for ( ; count>0; count-- )
				sprtmtch_plot_pixel(x++, y, pen, flags);
			break;

		case 0xd:	// Skip X pixels
			if (i >= size_src)	return i;
			x = sx + SRC[i++];
		case 0xc:	// Draw N pixels
			if (i >= size_src)	return i;
			count = SRC[i++];

			for ( ; count>0; count-- )
				sprtmtch_plot_pixel(x++, y, pen, flags);
			break;

//		case 0xe:	// ? unused

		case 0xf:	// Increment Y
			y++;
			x = sx;
			break;

		default:
			logerror("Blitter unknown command %06X: %02X\n", i-1, cmd);
		}
	}
	return i;
}

WRITE_HANDLER( sprtmtch_blit_draw_w )
{
	UINT32 i =
	sprtmtch_drawgfx(
		dynax_blit_address & 0x3fffff,
		dynax_blit_x, dynax_blit_y,
		data
	);

#if VERBOSE
	logerror("BLIT=%02X\n",data);
#endif

	dynax_blit_address	=	(dynax_blit_address & ~0x3fffff) |
							(i                  &  0x3fffff) ;

	/* Generate an IRQ */
	dynax_blitter_irq = 1;
	sprtmtch_update_irq();
}


/***************************************************************************


								Video Init


***************************************************************************/

VIDEO_START( dynax )
{
	return 0;
}

VIDEO_START( sprtmtch )
{
	if (!(dynax_pixmap[0][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[0][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][1] = auto_malloc(256*256)))	return 1;
	return 0;
}

/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( dynax )
{
//	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
}


void sprtmtch_copylayer(struct mame_bitmap *bitmap,int i)
{
	struct GfxElement gfx;
	int color;
	int sx,sy;

	switch ( i )
	{
		case 0:	color = (dynax_blit_palettes >> 0) & 0xf;	break;
		case 1:	color = (dynax_blit_palettes >> 4) & 0xf;	break;
		case 2:	color = (dynax_blit_palettes >> 8) & 0xf;	break;
		default:	return;
	}

	color += (dynax_blit_palbank & 1) * 16;

	gfx.width				=	256;
	gfx.height				=	256;
	gfx.total_elements		=	1;
	gfx.color_granularity	=	16;
	gfx.colortable			=	Machine->remapped_colortable;
	gfx.total_colors		=	32;
	gfx.pen_usage			=	NULL;
	gfx.gfxdata				=	dynax_pixmap[i][0];
	gfx.line_modulo			=	gfx.width;
	gfx.char_modulo			=	0;
	gfx.flags				=	0;

	for ( sy = dynax_blit_scroll_y+8 - 0x100; sy < 0x100; sy += 0x100 )
	{
		for ( sx = dynax_blit_scroll_x - 0x100;	sx < 0x100; sx += 0x100 )
		{
			gfx.gfxdata = dynax_pixmap[i][0];
			drawgfx(	bitmap, &gfx,
						0,
						color,
						0, 0,
						sx, sy,
						&Machine->visible_area, TRANSPARENCY_PEN, 0);
//			if (!keyboard_pressed(KEYCODE_Z))
			{
			gfx.gfxdata = dynax_pixmap[i][1];
			alpha_set_level(0x100/2);	// blend the 2 pictures (50%)
			drawgfx(	bitmap, &gfx,
						0,
						color,
						0, 0,
						sx, sy,
						&Machine->visible_area, TRANSPARENCY_ALPHA, 0);
			}
		}
	}
}

VIDEO_UPDATE( sprtmtch )
{
#ifdef MAME_DEBUG
#if 0
/*	A primitive gfx viewer:

	T          -  Toggle viewer
	I,O        -  Change palette (-,+)
	J,K & N,M  -  Change "tile"  (-,+, slow & fast)
	R          -  "tile" = 0		*/

static int toggle;
if (keyboard_pressed_memory(KEYCODE_T))	toggle = 1-toggle;
if (toggle)	{
	data8_t *RAM = memory_region( REGION_GFX1 );
	static int i = 0, c = 0;

	if (keyboard_pressed_memory(KEYCODE_I))	c = (c-1) & 0x1f;
	if (keyboard_pressed_memory(KEYCODE_O))	c = (c+1) & 0x1f;
	if (keyboard_pressed_memory(KEYCODE_R))	i = 0;
	if (keyboard_pressed(KEYCODE_M) | keyboard_pressed_memory(KEYCODE_K))	{
		while( RAM[i] )	i++;		i++;	}
	if (keyboard_pressed(KEYCODE_N) | keyboard_pressed_memory(KEYCODE_J))	{
		i-=2;	while( RAM[i] )	i--;		i++;	}

	dynax_blit_palettes = (c & 0xf) * 0x111;
	dynax_blit_palbank  = (c >>  4) & 1;
	dynax_blit_dest = 3;

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	memset(dynax_pixmap[0][0],0,sizeof(UINT8)*0x100*0x100);
	memset(dynax_pixmap[0][1],0,sizeof(UINT8)*0x100*0x100);
	sprtmtch_drawgfx(i, Machine->visible_area.min_x, Machine->visible_area.min_y, 0);
	sprtmtch_copylayer(bitmap, 0);
	usrintf_showmessage("%06X C%02X",i,c);
}
else
#endif
#endif
{
	int layers_ctrl = ~dynax_blit_enable;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (msk != 0)	layers_ctrl &= msk;		}
#endif

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		&Machine->visible_area);

	if (layers_ctrl & 1)	sprtmtch_copylayer( bitmap, 0 );
	if (layers_ctrl & 2)	sprtmtch_copylayer( bitmap, 1 );
	if (layers_ctrl & 4)	sprtmtch_copylayer( bitmap, 2 );
}

}
