/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


data8_t *tubep_textram;
data8_t *rjammer_backgroundram;

struct mame_bitmap *tmpbitmap2;
static data8_t *dirtybuff;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Roller Jammer has two 32 bytes palette PROMs, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
PALETTE_INIT( rjammer )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* sprites and text share the same palette (prom @16B)*/

	/* text */
	for (i = 0; i < TOTAL_COLORS(0); i++)
	{
		COLOR(0, 2*i + 0) = 0;		/* transparent "black" */
		COLOR(0, 2*i + 1) = i+16;	/* SOFF\ is on the 4bit line */
	}

//	/* sprites */
//	for (i = 0; i < TOTAL_COLORS(4); i++)
//	{
//		COLOR(4, i) = i;
//	}

}
PALETTE_INIT( tubep )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* text */
	for (i = 0; i < TOTAL_COLORS(0); i++)
	{
		COLOR(0, 2*i + 0) = 0;		/* transparent "black" */
		COLOR(0, 2*i + 1) = i;
	}
}


VIDEO_START( tubep )
{
	if ((dirtybuff = auto_malloc(0x800/2)) == 0)
		return 1;
	memset(dirtybuff,1,0x800/2);

	tmpbitmap  = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if ( (tmpbitmap == 0) || (tmpbitmap2 == 0) )
		return 1;

	return 0;
}

WRITE_HANDLER( tubep_textram_w )
{
	if (tubep_textram[offset] != data)
	{
		dirtybuff[offset/2] = 1;
		tubep_textram[offset] = data;
	}
}




static UINT8 ls377_data = 0;

WRITE_HANDLER( rjammer_background_LS377_w )
{
	ls377_data = data;
}


static UINT32 page = 0;

WRITE_HANDLER( rjammer_background_page_w )
{
	page = (data & 1) * 0x200;
}


VIDEO_UPDATE( rjammer )
{
	int offs;

	for (offs = 0;offs < 0x800; offs+=2)
	{
		if (dirtybuff[offs/2])
		{
			int sx,sy;

			dirtybuff[offs/2] = 0;

			sx = (offs/2) % 32;
			//if (flipscreen[0]) sx = 31 - sx;
			sy = (offs/2) / 32;
			//if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					tubep_textram[offs],
					tubep_textram[offs+1],
					0,0, /*flipscreen[0],flipscreen[1],*/
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* draw background ram */
	{
		pen_t *pens = &Machine->pens[ 0x20 ];

		UINT32 h,v;
		unsigned char * rom13D  = memory_region(REGION_USER1);
		unsigned char * rom11BD = memory_region(REGION_USER1)+0x1000;
		unsigned char * rom19C  = memory_region(REGION_USER1)+0x5000;

		for (v = 2*8; v < 30*8; v++)	/* only for visible area */
		{
			UINT8 pal14h4_pin19;
			UINT8 pal14h4_pin18;
			UINT8 pal14h4_pin13;

			UINT32 addr = (v*2) | page;
			UINT32 ram_data = rjammer_backgroundram[ addr ] + 256*(rjammer_backgroundram[ addr+1 ]&0x2f);

			addr = (v>>3) | ((ls377_data&0x1f)<<5);
			pal14h4_pin13 = (rom19C[addr] >> ((v&7)^7) ) &1;
			pal14h4_pin19 = (ram_data>>13) & 1;


            /* this can be further optimized by extracting constants out of the loop */
            /* especially read from ROM19C can be done once per 8 pixels*/
            /* and the data could be bitswapped beforehand */

			for (h = 0*8; h < 32*8; h++)
			{
				UINT8 bg_data;
				UINT8 color_bank;

				UINT32 ls283 = (ram_data & 0xfff) + h;
				UINT32 rom13D_addr = ((ls283>>4)&0x00f) | (v&0x0f0) | (ls283&0xf00);

				/* note: there is a jumper between bit 7 and bit 6 lines (bit 7 line is unused by default) */
				/* default: bit 6 is rom select signal 0=rom @11B, 1=rom @11D */

				UINT32 rom13D_data = rom13D[ rom13D_addr ] & 0x7f;
				/* rom13d_data is actually a content of LS377 @14C */


				UINT32 rom11BD_addr = (rom13D_data<<7) | ((v&0x0f)<<3) | ((ls283>>1)&0x07);
				UINT8  rom11_data = rom11BD[ rom11BD_addr];

				if ((ls283&1)==0)
					bg_data = rom11_data & 0x0f;
				else
					bg_data = (rom11_data>>4) & 0x0f;

				addr = (h>>3) | (ls377_data<<5);
				pal14h4_pin18 = (rom19C[addr] >> ((h&7)^7) ) &1;

/*
	PAL14H4 @15A functions:

		PIN6 = disable color on offscreen area

		PIN19,PIN18,PIN13 = arguments for PIN17 function

		PIN17 = background color bank (goes to A4 line on PROM @16A)
		formula for PIN17 is:

		PIN17 =	 ( PIN13 & PIN8 & PIN9 & !PIN11 &  PIN12 )
		       | ( PIN18 & PIN8 & PIN9 &  PIN11 & !PIN12 )
		       | ( PIN19 )

			where:
		PIN 8 = bit 3 of bg_data
		PIN 9 = bit 2 of bg_data
		PIN 11= bit 1 of bg_data
		PIN 12= bit 0 of bg_data


        not used by now, but for the record:

        PIN15 = select prom @16B (active low)
        PIN16 = select prom @16A (active low)
        PINs: 1,2,3,4,5 and 7,14 are used for priority system

*/

				color_bank =  (pal14h4_pin13 & ((bg_data&0x08)>>3) & ((bg_data&0x04)>>2) & (((bg_data&0x02)>>1)^1) &  (bg_data&0x01)    )
							| (pal14h4_pin18 & ((bg_data&0x08)>>3) & ((bg_data&0x04)>>2) &  ((bg_data&0x02)>>1)    & ((bg_data&0x01)^1) )
							| (pal14h4_pin19);

				plot_pixel(bitmap,h,v,pens[ color_bank*0x10 + bg_data ] );
			}
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] );

}



VIDEO_UPDATE( tubep )
{
	int offs;

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	for (offs = 0;offs < 0x800; offs+=2)
	{
		if (dirtybuff[offs/2])
		{
			int sx,sy;

			dirtybuff[offs/2] = 0;

			sx = (offs/2) % 32;
			//if (flipscreen[0]) sx = 31 - sx;
			sy = (offs/2) / 32;
			//if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					tubep_textram[offs],
					tubep_textram[offs+1],
					0,0, /*flipscreen[0],flipscreen[1],*/
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] );

}
