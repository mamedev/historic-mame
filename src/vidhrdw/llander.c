/***************************************************************************

	Atari Lunar Lander hardware

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "vidhrdw/avgdvg.h"
#include "vidhrdw/vector.h"
#include "asteroid.h"

#define NUM_LIGHTS 5

static struct artwork_info *llander_panel;
static struct artwork_info *llander_lit_panel;

static struct rectangle light_areas[NUM_LIGHTS] =
{
	{  0, 205, 0, 127 },
	{206, 343, 0, 127 },
	{344, 481, 0, 127 },
	{482, 616, 0, 127 },
	{617, 799, 0, 127 },
};

/* current status of each light */
static int lights[NUM_LIGHTS];
/* whether or not each light needs to be redrawn*/
static int lights_changed[NUM_LIGHTS];


/***************************************************************************

  Lunar Lander video routines

***************************************************************************/

PALETTE_INIT( llander )
{
	int width, height, i, nextcol;

	palette_init_avg_white(palette,colortable,color_prom);

	llander_lit_panel = NULL;
	width = Machine->scrbitmap->width;
	height = 0.16 * width;

	nextcol = 24;

	artwork_load_size(&llander_panel, "llander.png", nextcol, width, height);
	if (llander_panel != NULL)
	{
		artwork_load_size(&llander_lit_panel, "llander1.png", nextcol, width, height);
		if (llander_lit_panel == NULL)
		{
			artwork_free (&llander_panel);
			return ;
		}
	}
	else
		return;

	for (i = 0; i < 16; i++)
		palette[3*(i+8)]=palette[3*(i+8)+1]=palette[3*(i+8)+2]= (255*i)/15;
}


VIDEO_START( llander )
{
	int i;

	if (video_start_dvg())
		return 1;

	if (llander_panel == NULL)
		return 0;

	for (i=0;i<NUM_LIGHTS;i++)
	{
		lights[i] = 0;
		lights_changed[i] = 1;
	}
	return 0;
}


VIDEO_UPDATE( llander )
{
	int i, pwidth, pheight;
	float scale;
	struct mame_bitmap vector_bitmap;
	struct rectangle rect;

	if (llander_panel == NULL)
	{
		video_update_vector(bitmap,0);
		return;
	}

	pwidth = llander_panel->artwork->width;
	pheight = llander_panel->artwork->height;

	vector_bitmap = *bitmap;

	video_update_vector(&vector_bitmap,0);

	{
		rect.min_x = 0;
		rect.max_x = pwidth-1;
		rect.min_y = bitmap->height - pheight;
		rect.max_y = bitmap->height - 1;

		copybitmap(bitmap,llander_panel->artwork,0,0,
				   0,bitmap->height - pheight,&rect,TRANSPARENCY_NONE,0);
		osd_mark_dirty(rect.min_x,rect.min_y,rect.max_x,rect.max_y);
	}

	scale = pwidth/800.0;

	for (i=0;i<NUM_LIGHTS;i++)
	{
		rect.min_x = scale * light_areas[i].min_x;
		rect.max_x = scale * light_areas[i].max_x;
		rect.min_y = bitmap->height - pheight + scale * light_areas[i].min_y;
		rect.max_y = bitmap->height - pheight + scale * light_areas[i].max_y;

		if (lights[i])
			copybitmap(bitmap,llander_lit_panel->artwork,0,0,
					   0,bitmap->height - pheight,&rect,TRANSPARENCY_NONE,0);
		else
			copybitmap(bitmap,llander_panel->artwork,0,0,
					   0,bitmap->height - pheight,&rect,TRANSPARENCY_NONE,0);

		osd_mark_dirty(rect.min_x,rect.min_y,rect.max_x,rect.max_y);

		lights_changed[i] = 0;
	}
}

/* Lunar lander LED port seems to be mapped thus:

   NNxxxxxx - Apparently unused
   xxNxxxxx - Unknown gives 4 high pulses of variable duration when coin put in ?
   xxxNxxxx - Start    Lamp ON/OFF == 0/1
   xxxxNxxx - Training Lamp ON/OFF == 1/0
   xxxxxNxx - Cadet    Lamp ON/OFF
   xxxxxxNx - Prime    Lamp ON/OFF
   xxxxxxxN - Command  Lamp ON/OFF

   Selection lamps seem to all be driver 50/50 on/off during attract mode ?

*/


WRITE_HANDLER( llander_led_w )
{
	/*      logerror("LANDER LED: %02x\n",data); */

    int i;

    for (i=0;i<5;i++)
    {
		int new_light = (data & (1 << (4-i))) != 0;
		if (lights[i] != new_light)
		{
			lights[i] = new_light;
			lights_changed[i] = 1;
		}
    }



}

