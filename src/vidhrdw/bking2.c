/***************************************************************************

  bking2.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static UINT8 xld1=0;
static UINT8 xld2=0;
static UINT8 xld3=0;
static UINT8 yld1=0;
static UINT8 yld2=0;
static UINT8 yld3=0;
static int msk=0;
static int safe=0;
static int cont1=0;
static int cont2=0;
static int cont3=0;
static int eport1=0;
static int eport2=0;
static int hitclr=0;

void bking2_xld1_w(int offset, int data)
{
    xld1 = -data;
}

void bking2_yld1_w(int offset, int data)
{
    yld1 = -data;
}

void bking2_xld2_w(int offset, int data)
{
    xld2 = -data;
}

void bking2_yld2_w(int offset, int data)
{
    yld2 = -data;
}

void bking2_xld3_w(int offset, int data)
{
    /* Feeds into Binary Up/Down counters.  Crow Inv changes counter direction */
    xld3 = data;
}

void bking2_yld3_w(int offset, int data)
{
    /* Feeds into Binary Up/Down counters.  Crow Inv changes counter direction */
    yld3 = data;
}

void bking2_msk_w(int offset, int data)
{
    msk = data;
}

void bking2_safe_w(int offset, int data)
{
    safe = data;
}

void bking2_cont1_w(int offset, int data)
{
    /* D0 = COIN LOCK */
    /* D1 =  BALL 5 */
    /* D2 = VINV */
    /* D3 = Not Connected */
    /* D4-D7 = CROW0-CROW3 (Selects crow picture) */
    cont1 = data;
}

void bking2_cont2_w(int offset, int data)
{
    /* D0-D2 = BALL10 - BALL12 (Selects player 1 ball picture) */
    /* D3-D5 = BALL20 - BALL22 (Selects player 2 ball picture) */
    /* D6 = HIT1 */
    /* D7 = HIT2 */
    cont2 = data;
}

void bking2_cont3_w(int offset, int data)
{
	/* D0 = CROW INV (inverts Crow picture and direction?) */
	/* D1-D2 = COLOR 0 - COLOR 1 (switches 4 color palettes, global across all graphics) */
	/* D3 = SOUND STOP */
	cont3 = data;
}

void bking2_eport2_w(int offset, int data)
{
    /* Some type of sound data? */
    eport2 = data;
}

void bking2_hitclr_w(int offset, int data)
{
    hitclr = data;
}


/* Hack alert.  I don't know how to upper bits work, so I'm just returning
   what the code expects, otherwise the collision detection is skipped */
int bking2_pos_r(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	UINT16 pos, x, y;


	if (hitclr & 0x04)
	{
		x = xld2;
		y = yld2;
	}
	else
	{
		x = xld1;
		y = yld1;
	}

	pos = ((y >> 3 << 5) | (x >> 3)) + 2;

	switch (offset)
	{
	case 0x00:
		return (pos & 0x0f) << 4;
	case 0x08:
		return (pos & 0xf0);
	case 0x10:
		return ((pos & 0x0300) >> 4) | (RAM[0x804c] & 0xc0);
	default:
		return 0;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bking2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs + 1])
		{
			int sx,sy;
			int flipx,flipy;

			dirtybuffer[offs] = dirtybuffer[offs + 1] = 0;

			sx = (offs/2) % 32;
			sy = (offs/2) / 32;
			flipx = videoram[offs + 1] & 0x04;
			flipy = videoram[offs + 1] & 0x08;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs + 1] & 0x03) << 8),
					0, /* color */
					flipx,flipy,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw the balls */
	drawgfx(bitmap,Machine->gfx[2],
			cont2 & 0x07,
			0, /* color */
			0,0,
			xld1,yld1,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	drawgfx(bitmap,Machine->gfx[3],
			(cont2 >> 3) & 0x07,
			0, /* color */
			0,0,
			xld2,yld2,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* draw the crow */
	drawgfx(bitmap,Machine->gfx[1],
			(cont1 >> 4) & 0x0f,
			0, /* color */
			0,0,
			xld3,yld3,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}
