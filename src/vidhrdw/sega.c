/***************************************************************************

  vidhrdw.c

  Generic functions used by the Sega Vector games

***************************************************************************/

#include "driver.h"
#include "avgdvg.h"
#include "math.h"

/* ASG 080697 -- begin */
#define VEC_SHIFT 14		/* NOTE: must be even */
#define RES_SHIFT 16		/* NOTE: must be even */

#define VEC_X(x,xs)	( ( ((x) >> VEC_SHIFT/2) * ((xs) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + VEC_SHIFT/2) )
#define VEC_Y(y,ys)	( ( ((y) >> VEC_SHIFT/2) * ((ys) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + VEC_SHIFT/2) )
/* ASG 080697 -- end */

/* #define SEGA_DEBUG */

/* #define BASE	1024 */ /* Maximum resolution of the vector hardware */

#ifdef SEGA_DEBUG
FILE	*segaFile;
#endif

/* This structure holds the used screen dimension by the vector game	*/
static struct { int width; int height;
	 int x_cent; int y_cent;
	 int x_min; int y_min;
	 int x_max; int y_max; } vg_video;

extern int vectorram_size;
/* static int frameCount = 1; */
int portrait;

static long *sinTable, *cosTable;		/* ASG 080797 */


/***************************************************************************

  The Sega vector games don't have a color PROM, it uses RGB values for the
  vector guns.
  This routine sets up the color tables to simulate it.

***************************************************************************/

void sega_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for( i = 0; i < 128; i++ )
	{
		int bit0, bit1;

		/* Red is bits 5 & 6 (0x60) */
		bit0 = ( i >> 5 ) & 0x01;
		bit1 = ( i >> 6 ) & 0x01;
		palette[ 3 * i ] = 0x47 * bit0 + 0x97 * bit1;
		/* Green is bits 3 & 4 (0x18) */
		bit0 = ( i >> 3 ) & 0x01;
		bit1 = ( i >> 4 ) & 0x01;
		palette[ 3 * i + 1 ] = 0x47 * bit0 + 0x97 * bit1;
		/* Blue is bits 1 & 2 (0x06) */
		bit0 = ( i >> 1 ) & 0x01;
		bit1 = ( i >> 2 ) & 0x01;
		palette[ 3 * i + 2 ] = 0x47 * bit0 + 0x97 * bit1;
		/* Set the color table */
		colortable[i] = i;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int sega_vh_start (void)
{
	int i;		/* ASG 080697 */

	if (vectorram_size == 0)
		return 1;
#ifdef SEGA_DEBUG
	segaFile = fopen ("Sega Dump","w");
#endif
	vg_video.width =Machine->drv->visible_area.max_x-Machine->drv->visible_area.min_x;
	vg_video.height=Machine->drv->visible_area.max_y-Machine->drv->visible_area.min_y;
	vg_video.x_cent=(Machine->drv->visible_area.max_x+Machine->drv->visible_area.min_x)/2;
	vg_video.y_cent=(Machine->drv->visible_area.max_y+Machine->drv->visible_area.min_y)/2;
	vg_video.x_min=Machine->drv->visible_area.min_x;
	vg_video.y_min=Machine->drv->visible_area.min_y;
	vg_video.x_max=Machine->drv->visible_area.max_x;
	vg_video.y_max=Machine->drv->visible_area.max_y;

/* ASG 080697 -- begin */

	/* allocate memory for the sine and cosine lookup tables */
	sinTable = malloc (0x400 * sizeof (long));
	if (!sinTable)
		return 1;
	cosTable = malloc (0x400 * sizeof (long));
	if (!cosTable)
	{
		free (sinTable);
		return 1;
	}

	/* generate the sine/cosine lookup tables */
	for (i = 0; i < 0x400; i++)
	{
		double angle = ((2. * PI) / (double)0x400) * (double)i;
		double temp;

		temp = sin (angle);
		if (temp < 0)
			sinTable[i] = (long)(temp * (double)(1 << VEC_SHIFT) - 0.5);
		else
			sinTable[i] = (long)(temp * (double)(1 << VEC_SHIFT) + 0.5);

		temp = cos (angle);
		if (temp < 0)
			cosTable[i] = (long)(temp * (double)(1 << VEC_SHIFT) - 0.5);
		else
			cosTable[i] = (long)(temp * (double)(1 << VEC_SHIFT) + 0.5);
	}

/* ASG 080697 -- end */

	portrait = 0;
	return 0;
}

int tacscan_vh_start (void)
{
	if (sega_vh_start ()) return 1;
	portrait = 1;
	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void sega_vh_stop (void)
{
#ifdef SEGA_DEBUG
	fclose (segaFile);
#endif

/* ASG 080797 -- begin */
	if (sinTable)
		free (sinTable);
	sinTable = NULL;
	if (cosTable)
		free (cosTable);
	cosTable = NULL;
/* ASG 080797 -- end */
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sega_vh_screenrefresh (struct osd_bitmap *bitmap) {
/* This routine does nothing. The interrupt handler refreshes the screen. */
	}


/* ASG 080797 -- begin */
void sega_vg_draw (void)
{
	int x_res, y_res;
	int xscale, yscale;

	int deltax, deltay;
	int currentX, currentY;

	int vectorIndex;
	int symbolIndex;

	int rotate, scale;
	int attrib;

	int angle, length;
	int color;

	int draw;

	if (osd_update_vectors(&x_res, &y_res, 0))
		return;

	symbolIndex = 0;					/* Reset vector PC to 0 */

	xscale = (x_res << RES_SHIFT) / vg_video.width;
	yscale = (y_res << RES_SHIFT) / vg_video.height;

	/*
	* walk the symbol list until 'last symbol' set
	*/

	do
	{
		draw = vectorram[symbolIndex++];

		if (draw & 1)								/* if symbol active */
		{
			currentX    = vectorram[symbolIndex + 0] | (vectorram[symbolIndex + 1] << 8);
			currentY    = vectorram[symbolIndex + 2] | (vectorram[symbolIndex + 3] << 8);
			vectorIndex = vectorram[symbolIndex + 4] | (vectorram[symbolIndex + 5] << 8);
			rotate      = vectorram[symbolIndex + 6] | (vectorram[symbolIndex + 7] << 8);
			scale       = vectorram[symbolIndex + 8];

			if (portrait) {
				currentX = ((currentX & 0x7ff) - vg_video.y_min) << VEC_SHIFT;
				currentY = (vg_video.x_max - (currentY & 0x7ff)) << VEC_SHIFT;
				osd_draw_to (VEC_X (currentY, xscale), VEC_Y (vg_video.y_max-currentX, yscale), -1);

			} else {
				currentX = ((currentX & 0x7ff) - vg_video.x_min) << VEC_SHIFT;
				currentY = (vg_video.y_max - (currentY & 0x7ff)) << VEC_SHIFT;
				osd_draw_to (VEC_X (currentX, xscale), VEC_Y (currentY, yscale), -1);
			}
			vectorIndex &= 0xfff;

			/*
			 * walk the vector list until 'last vector' bit is set in attributes
			 */

			do
			{
				attrib = vectorram[vectorIndex + 0];
				length = vectorram[vectorIndex + 1];
				angle  = vectorram[vectorIndex + 2] | (vectorram[vectorIndex + 3] << 8);

				vectorIndex += 4;

			  /*
			   * calculate delta x / delta y based on len, angle(s), and scale factor
			   */

				angle = (angle + rotate) & 0x3ff;
				deltax = sinTable[angle] * scale * length;
				deltay = cosTable[angle] * scale * length;

				currentX += deltax >> 7;
				currentY -= deltay >> 7;

				color = attrib & 0x7e;
				if (!(attrib & 1) || !color)
					color = -1;

					if (portrait)
						osd_draw_to (VEC_X (currentY, xscale), VEC_Y (vg_video.y_max-currentX, yscale), color);
					else
						osd_draw_to (VEC_X (currentX, xscale), VEC_Y (currentY, yscale), color);

			} while (!(attrib & 0x80));
		}

		symbolIndex += 9;
		if (symbolIndex >= vectorram_size)
			break;

	} while (!(draw & 0x80));

	 /*
	  * all done, push it out to the screen
	  */

}
/* ASG 080797 -- end */

















/*	ASG 080797 -- begin removal */
#if 0

#define USE_SCALING

struct symbolVal {
	unsigned char draw;
	unsigned short x;
	unsigned short y;
	unsigned long vectorAdr;
	unsigned short angle;
	unsigned char scale;
} sym;

struct vectorVal {
	unsigned char attrib;
	unsigned char len;
	unsigned int angle;
} vec;


void sega_vg_draw (void) {

	int x_res;
	int y_res;

	int x_init_scale;
	int y_init_scale;

	double real_angle = 0.0;
	double deltax;
	double deltay;
	int currentX;
	int currentY;
	int color;

	int vectorIndex;
	int symbolIndex;
	unsigned char lastColor = 0;

	if (osd_update_vectors(&x_res,&y_res,0))
		return;

	symbolIndex = 0;					/* Reset vector PC to 0 */

	x_init_scale = x_res;
	y_init_scale = y_res;


	/*
	* walk the symbol list until 'last symbol' set
	*/

	do {
		sym.draw = vectorram[symbolIndex++];
		if (sym.draw & 0x1) {								/* if symbol active */

			sym.x = vectorram[symbolIndex++];									/* X Start */
			sym.x |= (vectorram[symbolIndex++] << 8);
			sym.y = vectorram[symbolIndex++];
			sym.y |= (vectorram[symbolIndex++] << 8);							/* Y Start */
			sym.vectorAdr = vectorram[symbolIndex++];
			sym.vectorAdr |= (vectorram[symbolIndex++] << 8);					/* Adr of vector data */
			sym.angle = vectorram[symbolIndex++];
			sym.angle |= (vectorram[symbolIndex++] << 8);						/* Initial rotation of symbol */
			sym.scale = vectorram[symbolIndex++];								/* Fixed-point scale factor */

#ifdef USE_SCALING
			/* Shifting by 10 is the equivalent of dividing by 1024, which is the base resolution */
			currentX = (((sym.x & 0x7ff) * x_init_scale) >> 10) - x_res/2;
			currentY = (((sym.y & 0x7ff) * y_init_scale) >> 10) - y_res/2;
#else
			currentX = ((sym.x & 0x7ff) - BASE/2);
			currentY = ((sym.y & 0x7ff) - BASE/2);
#endif

/*			if (errorlog) fprintf (errorlog, "current x: %f  ",currentX); */

			/* Position the pen */
#ifdef USE_SCALING
			if (portrait)
				osd_draw_to (x_res - currentY, y_res - currentX, -1);
			else
				osd_draw_to (currentX, y_res - currentY, -1);
#else
			if (portrait)
				osd_draw_to (BASE/2 - (currentY >> 1), BASE/2 - (currentX >> 1), -1);
			else
				osd_draw_to (currentX >> 1, BASE/2 - (currentY >> 1), -1);
#endif

			vectorIndex = sym.vectorAdr & 0xfff;

			/*
			 * walk the vector list until 'last vector' bit is set in attributes
			 */
			do {
				vec.attrib = vectorram[vectorIndex++];
				vec.len = vectorram[vectorIndex++];
				vec.angle = vectorram[vectorIndex++];
				vec.angle |= (vectorram[vectorIndex++] << 8);

			  /*
			   * calculate delta x / delta y based on len, angle(s), and scale factor
			   */

				real_angle = ((2. * PI) / BASE) * (double)((vec.angle + sym.angle) & 0x3ff);

#ifdef USE_SCALING
				deltax = (sin(real_angle) * ((sym.scale * vec.len * x_init_scale) >> 17));
				deltay = (cos(real_angle) * ((sym.scale * vec.len * y_init_scale) >> 17));
#else
				deltax = (sin(real_angle) * ((sym.scale * vec.len) >> 7));
				deltay = (cos(real_angle) * ((sym.scale * vec.len) >> 7));
#endif

#ifdef SEGA_DEBUG
				fprintf (segaFile,"real_angle %f ",real_angle);
				fprintf (segaFile,"sin:%f cos: %f deltax: %d deltay: %d\n",sin(real_angle), cos(real_angle),deltax, deltay);
#endif

				currentX += (int) deltax;
				currentY += (int) deltay;

				if ((vec.attrib & 1) && (vec.attrib & 0x7e))
					color = vec.attrib & 0x7e;
				else color = -1;

#ifdef USE_SCALING
			if (portrait)
				osd_draw_to (x_res - currentY, y_res - currentX, color);
			else
				osd_draw_to (currentX, y_res - currentY, color);
#else
			if (portrait)
				osd_draw_to (BASE/2 - (currentY >> 1), BASE/2 - (currentX >> 1), color);
			else
				osd_draw_to (currentX >> 1, BASE/2 - (currentY >> 1), color);
#endif

				} while ((vec.attrib & 0x80) == 0);
			}
		else {											/* symbol inactive */
			symbolIndex += 9;							/* bump to next one */
			}

		if (symbolIndex >= vectorram_size){
/*		    printf("ran off the end of the vector ram\n"); */
			break;
			}

		} while((sym.draw & 0x80) == 0);

	 /*
	  * all done, push it out to the screen
	  */

	}
#endif
/*	ASG 080697 -- end removal */


