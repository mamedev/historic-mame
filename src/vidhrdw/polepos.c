#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *back_bitmap;
static struct osd_bitmap *road_bitmap;

/* modified vertical position built from three nibbles (12 bit)
 * of ROMs 136014-142, 136014-143, 136014-144
 * The value RVP (road vertical position, lower 12 bits) is added
 * to this value and the upper 10 bits of the result are used to
 * address the playfield video memory (AB0 - AB9).
 */
int polepos_vertical_position_modifier[256];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Pole Position has three 256x4 palette PROMs (one per gun)
  and a lot ;-) of 256x4 lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void polepos_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i, j;

    #define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* Sheet 15B: 136014-0137 red component */
		bit0 = (color_prom[0*Machine->drv->total_colors] >> 0) & 1;
		bit1 = (color_prom[0*Machine->drv->total_colors] >> 1) & 1;
		bit2 = (color_prom[0*Machine->drv->total_colors] >> 2) & 1;
		bit3 = (color_prom[0*Machine->drv->total_colors] >> 3) & 1;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* Sheet 15B: 136014-0138 green component */
		bit0 = (color_prom[1*Machine->drv->total_colors] >> 0) & 1;
		bit1 = (color_prom[1*Machine->drv->total_colors] >> 1) & 1;
		bit2 = (color_prom[1*Machine->drv->total_colors] >> 2) & 1;
		bit3 = (color_prom[1*Machine->drv->total_colors] >> 3) & 1;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* Sheet 15B: 136014-0139 blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 1;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 1;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 1;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 1;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/*
		 * Sheet 15B: top left, 136014-140
		 * inputs: SHFT0, SHFT1 and CHA8* ... CHA13*
		 */
		COLOR(0,i) = 0x020 + color_prom[0x300+i];

        /*
		 * Sheet 13A: left, 136014-141
		 * inputs: SHFT2, SHFT3 and CHA8 ... CHA13
         */
		COLOR(1,i) = 0x010 + color_prom[0x400+i];

        COLOR(2,i) = i & 0x3f;
		COLOR(3,i) = i & 0x3f;

		/* 136014-142, 136014-143, 136014-144 Vertical position modifiers */
		j = color_prom[0x700+i] + (color_prom[0x800+i] << 4) + (color_prom[0x900+i] << 8);
		if (errorlog) fprintf(errorlog, "mvp %3d $%04x (%d)\n", i, j, j);
		polepos_vertical_position_modifier[i] = j;

        color_prom++;
    }
}

int polepos_vh_start(void) {
	back_bitmap = osd_new_bitmap(Machine->drv->screen_width, Machine->drv->screen_height, 8);
	road_bitmap = osd_new_bitmap(Machine->drv->screen_width, Machine->drv->screen_height, 8);
	fillbitmap( back_bitmap, Machine->pens[0], &Machine->drv->visible_area);
	return generic_vh_start();
}

void polepos_vh_stop(void) {
	osd_free_bitmap(back_bitmap);
	osd_free_bitmap(road_bitmap);
	generic_vh_stop();
}

extern unsigned char *polepos_back_memory;
void polepos_back_draw( int offs ) {
	int sx, sy, code, color;
    if (offs & 1) return;
	sx = (offs / 2) % 32;
	sy = (offs / 2) / 32;
	code = polepos_back_memory[offs&~1];
	color = polepos_back_memory[offs|1] & 0x3f;
	drawgfx( back_bitmap,Machine->gfx[1],
		code, color,
		0, 0, 8*sx,8*sy,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

extern unsigned char *polepos_road_memory;
void polepos_road_draw( int offs ) {
	int sx, sy, code, color;
	sx = (offs / 2) % 32;
	sy = (offs / 2) / 32;
	code = polepos_road_memory[offs&~1];
	color = polepos_road_memory[offs|1] & 15;
	/* changed a character */
	drawgfx( road_bitmap, Machine->gfx[1],
		code, color,
		0, 0, 8*sx,8*sy,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

static void draw_foreground( struct osd_bitmap *bitmap ) {
	int i;

	for ( i = 0; i < videoram_size; i += 2 ) {
		int offs = i / 2;
		int sx, sy;
		int code = videoram[i];
		int color = videoram[i+1] & 0x3f;	/* 6 bits color */

		sx = offs % 32;
		sy = offs / 32;
        drawgfx( bitmap,Machine->gfx[0],
				 code, color, 0, 0, 8*sx,8*sy,
				 &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }
}

void polepos_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) {
//	copybitmap( bitmap, back_bitmap, 0,0,0,0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
	copybitmap( bitmap, road_bitmap, 0,0,0,0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
    draw_foreground( bitmap );
}
