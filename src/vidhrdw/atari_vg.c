/***************************************************************************

  atari_vg.c

  Generic functions used by the Atari Vector games

***************************************************************************/

#include "driver.h"
#include "vector.h"

/*
 * This initialises the colors for all atari games
 * We must have the basic palette (8 colors) in color_prom[],
 * and the rest is set up to the correct intensity
 * This code is derived from Brad
 */

void atari_vg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int c,i,e;   	/* (c)olor, (i)ntensity, palette (e)ntry */
	int r,g,b;
	for (c=0; c<16; c++)
	{
		for (i=0; i<16; i++)
		{
			e=c+i*16;
			colortable[e]=e;
			r=color_prom[3*c  ]*i*0x18;
			g=color_prom[3*c+1]*i*0x18;
			b=color_prom[3*c+2]*i*0x18;
			palette[3*e  ]=(r < 256) ? r : 0xff;
			palette[3*e+1]=(g < 256) ? g : 0xff;
			palette[3*e+2]=(b < 256) ? b : 0xff;
		}
	}
}

/* If you want to use this, please make sure that you have
 * a fake GfxLayout, otherwise you'll crash */
void atari_vg_colorram_w (int offset, int data)
{
	int i;

	data&=0x0f;
	for (i=0; i<16; i++)
		Machine->gfx[0]->colortable[offset+i*16]=Machine->pens[data+i*16];
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void atari_vg_stop(void)
{
	vg_stop ();
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void atari_vg_screenrefresh(struct osd_bitmap *bitmap)
{
/* This routine does nothing - writes to vg_go start the drawing process */
}


/*
 * AVG-games start like this
 */
int atari_vg_avg_start(void)
{
	if (vg_init (0x1000, USE_AVG,0))
		return 1;
	return 0;
}

/*
 * DVG-games start like this
 */
int atari_vg_dvg_start(void)
{

	if (vg_init (0x800, USE_DVG,0))
		return 1;
	return 0;
}

/*
 * Starwars starts like this
 */
int atari_vg_avg_flip_start(void)
{
	if (vg_init(0x4000,USE_AVG,1))
		return 1;
	return 0;
}

