/***************************************************************************

  vasteroid.c

  Vector video hardware used by Asteroids/Asteroids Deluxe/Lunar Lander

***************************************************************************/

#include "driver.h"
#include "vector.h"


#define VECTOR_LIST_SIZE	0x800	/* Size in bytes of the vector ram area */

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int asteroid_vh_start(void)
{
	if (vg_init (VECTOR_LIST_SIZE, USE_DVG))
		return 1;
	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void asteroid_vh_stop(void)
{
	vg_stop ();
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void asteroid_vh_screenrefresh(struct osd_bitmap *bitmap)
{
/* This routine does nothing - writes to vg_go start the drawing process */
}
