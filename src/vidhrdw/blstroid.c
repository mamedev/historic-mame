/***************************************************************************

	Atari Blasteroids hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *blstroid_priorityram;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int blstroid_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,32,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x100,		/* index of palette base */
		0x080,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x1fff,		/* tile data index mask */
		0xe000,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		256,				/* number of scanlines between MO updates */

		0x000,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0x0ff8,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x3fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x000f }},	/* mask for the color */
		{{ 0,0,0,0xffc0 }},	/* mask for the X position */
		{{ 0xff80,0,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0x000f,0,0,0 }},	/* mask for the height, in tiles */
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
		{{ 0,0x4000,0,0 }},	/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;
	return 0;

	/* error cases */
cant_create_mo:
	ataripf_free();
cant_create_pf:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void blstroid_vh_stop(void)
{
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

static void irq_off(int param)
{
	atarigen_scanline_int_ack_w(0, 0, 0);
}


void blstroid_scanline_update(int scanline)
{
	int offset = (scanline / 8) * 64 + 40;

	/* check for interrupts */
	if (offset < 0x1000)
		if (ataripf_0_base[offset] & 0x8000)
		{
			/* generate the interrupt */
			atarigen_scanline_int_gen();
			atarigen_update_interrupts();

			/* also set a timer to turn ourself off */
			timer_set(cpu_getscanlineperiod(), 0, irq_off);
		}
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* by default, draw anywhere the MO is non-zero */
		data->drawmode = TRANSPARENCY_PENS;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		int idx = (data->mocolor & 0x0f) | ((data->pfcolor & 0x07) << 4);

		/* determine the priority bits */
		data->drawpens = 0xffff;
		if (!blstroid_priorityram[idx])
			data->drawpens ^= 0x00ff;
		if (!blstroid_priorityram[idx + 0x80])
			data->drawpens ^= 0xff00;

		/* only overdraw if the mask is not fully transparent */
		return (data->drawpens != 0xffff) ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void blstroid_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
}
