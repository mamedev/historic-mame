#include "driver.h"
#include "vidhrdw/ppu2c03b.h"

/* from machine */
extern int vsnes_gun_controller;


void vsnes_vh_convert_color_prom( unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom )
{
	ppu2c03b_init_palette( palette );
}

void vsdual_vh_convert_color_prom( unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom )
{
	ppu2c03b_init_palette( palette );
	ppu2c03b_init_palette( &palette[3*64] );
}

static void ppu_irq( int num )
{
	cpu_set_nmi_line( num, PULSE_LINE );
}

/* our ppu interface											*/
static struct ppu2c03b_interface ppu_interface =
{
	1,						/* num */
	{ REGION_GFX1 },		/* vrom gfx region */
	{ 0 },					/* gfxlayout num */
	{ 0 },					/* color base */
	{ PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq }				/* irq */
};

/* our ppu interface for dual games								*/
static struct ppu2c03b_interface ppu_dual_interface =
{
	2,										/* num */
	{ REGION_GFX1, REGION_GFX2 },			/* vrom gfx region */
	{ 0, 1 },								/* gfxlayout num */
	{ 0, 64 },								/* color base */
	{ PPU_MIRROR_NONE, PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq, ppu_irq }					/* irq */
};

int vsnes_vh_start( void )
{
	return ppu2c03b_init( &ppu_interface );
}

int vsdual_vh_start( void )
{
	return ppu2c03b_init( &ppu_dual_interface );
}

void vsnes_vh_stop( void )
{
	ppu2c03b_dispose();
}

/***************************************************************************

  Display refresh

***************************************************************************/
void vsnes_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh )
{
	/* render the ppu */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );

	/* if this is a gun game, draw a simple crosshair */
	if ( vsnes_gun_controller )
	{
		int x_center = readinputport( 4 );
		int y_center = readinputport( 5 );
		UINT16 color;
		int x, y;
		int minmax_x[2], minmax_y[2];

		minmax_x[0] = Machine->visible_area.min_x;
		minmax_x[1] = Machine->visible_area.max_x;
		minmax_y[0] = Machine->visible_area.min_y;
		minmax_y[1] = Machine->visible_area.max_y;

		color = Machine->pens[0x05]; /* red */

		if ( x_center < ( minmax_x[0] + 2 ) )
		  	x_center = minmax_x[0] + 2;

		if ( x_center > ( minmax_x[1] - 2 ) )
		   	x_center = minmax_x[1] - 2;

		if ( y_center < ( minmax_y[0] + 2 ) )
	    	y_center = minmax_y[0] + 2;

	    if ( y_center > ( minmax_y[1] - 2 ) )
	    	y_center = minmax_y[1] - 2;

		for( y = y_center-5; y < y_center+6; y++ )
		{
			if ( ( y >= minmax_y[0] ) && ( y <= minmax_y[1] ) )
				plot_pixel( bitmap, x_center, y, color );
		}

		for( x = x_center-5; x < x_center+6; x++ )
		{
			if( ( x >= minmax_x[0] ) && ( x <= minmax_x[1] ) )
				plot_pixel( bitmap, x, y_center, color );
		}
	}
}

void vsdual_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh )
{
	/* render the ppu's */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );
	ppu2c03b_render( 1, bitmap, 0, 0, 32*8, 0 );
}
