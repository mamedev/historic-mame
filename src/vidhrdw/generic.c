/***************************************************************************

  vidhrdw/generic.c

  Some general purpose functions used by many video drivers.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *videoram00,*videoram01,*videoram02,*videoram03;
unsigned char *videoram10,*videoram11,*videoram12,*videoram13;
unsigned char *videoram20,*videoram21,*videoram22,*videoram23;
unsigned char *videoram30,*videoram31,*videoram32,*videoram33;


unsigned char *videoram;
int videoram_size;
unsigned char *colorram;
unsigned char *spriteram;	/* not used in this module... */
unsigned char *spriteram_2;	/* ... */
unsigned char *spriteram_3;	/* ... */
int spriteram_size;	/* ... here just for convenience */
int spriteram_2_size;	/* ... here just for convenience */
int spriteram_3_size;	/* ... here just for convenience */
unsigned char *flip_screen;	/* ... */
unsigned char *flip_screen_x;	/* ... */
unsigned char *flip_screen_y;	/* ... */
unsigned char *dirtybuffer;
struct osd_bitmap *tmpbitmap;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int generic_vh_start(void)
{
	int i;


	for (i = 0;i < MAX_LAYERS;i++)
		Machine->layer[i] = 0;
	dirtybuffer = 0;
	tmpbitmap = 0;

	if (Machine->drv->layer)
	{
		i = 0;
		while (i < MAX_LAYERS && Machine->drv->layer[i].type)
		{
			if ((Machine->layer[i] = create_tile_layer(&Machine->drv->layer[i])) == 0)
			{
				generic_vh_stop();
				return 1;
			}

			i++;
		}
	}
	else
	{
		if (videoram_size == 0)
		{
if (errorlog) fprintf(errorlog,"Error: generic_vh_start() called but videoram_size not initialized\n");
			return 1;
		}

		if ((dirtybuffer = malloc(videoram_size)) == 0)
			return 1;
		memset(dirtybuffer,1,videoram_size);

		if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
		{
			free(dirtybuffer);
			return 1;
		}
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void generic_vh_stop(void)
{
	int i;


	for (i = 0;i < MAX_LAYERS;i++)
	{
		free_tile_layer(Machine->layer[i]);
		Machine->layer[i] = 0;
	}

	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);

	dirtybuffer = 0;
	tmpbitmap = 0;
}



int videoram_r(int offset)
{
	return videoram[offset];
}

int colorram_r(int offset)
{
	return colorram[offset];
}

void videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;
	}
}

void colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}



int spriteram_r(int offset)
{
	return spriteram[offset];
}

void spriteram_w(int offset,int data)
{
	spriteram[offset] = data;
}




void videoram00_w(int offset,int data)
{
	if (videoram00[offset] != data)
	{
		videoram00[offset] = data;
		if (Machine->drv->layer[0].updatehook0) (*Machine->drv->layer[0].updatehook0)(offset);
	}
}

void videoram01_w(int offset,int data)
{
	if (videoram01[offset] != data)
	{
		videoram01[offset] = data;
		if (Machine->drv->layer[0].updatehook1) (*Machine->drv->layer[0].updatehook1)(offset);
	}
}

void videoram02_w(int offset,int data)
{
	if (videoram02[offset] != data)
	{
		videoram02[offset] = data;
		if (Machine->drv->layer[0].updatehook2) (*Machine->drv->layer[0].updatehook2)(offset);
	}
}

void videoram03_w(int offset,int data)
{
	if (videoram03[offset] != data)
	{
		videoram03[offset] = data;
		if (Machine->drv->layer[0].updatehook3) (*Machine->drv->layer[0].updatehook3)(offset);
	}
}

void videoram10_w(int offset,int data)
{
	if (videoram10[offset] != data)
	{
		videoram10[offset] = data;
		if (Machine->drv->layer[1].updatehook0) (*Machine->drv->layer[1].updatehook0)(offset);
	}
}

void videoram11_w(int offset,int data)
{
	if (videoram11[offset] != data)
	{
		videoram11[offset] = data;
		if (Machine->drv->layer[1].updatehook1) (*Machine->drv->layer[1].updatehook1)(offset);
	}
}

void videoram12_w(int offset,int data)
{
	if (videoram12[offset] != data)
	{
		videoram12[offset] = data;
		if (Machine->drv->layer[1].updatehook2) (*Machine->drv->layer[1].updatehook2)(offset);
	}
}

void videoram13_w(int offset,int data)
{
	if (videoram13[offset] != data)
	{
		videoram13[offset] = data;
		if (Machine->drv->layer[1].updatehook3) (*Machine->drv->layer[1].updatehook3)(offset);
	}
}

void videoram20_w(int offset,int data)
{
	if (videoram20[offset] != data)
	{
		videoram20[offset] = data;
		if (Machine->drv->layer[2].updatehook0) (*Machine->drv->layer[2].updatehook0)(offset);
	}
}

void videoram21_w(int offset,int data)
{
	if (videoram21[offset] != data)
	{
		videoram21[offset] = data;
		if (Machine->drv->layer[2].updatehook1) (*Machine->drv->layer[2].updatehook1)(offset);
	}
}

void videoram22_w(int offset,int data)
{
	if (videoram22[offset] != data)
	{
		videoram22[offset] = data;
		if (Machine->drv->layer[2].updatehook2) (*Machine->drv->layer[2].updatehook2)(offset);
	}
}

void videoram23_w(int offset,int data)
{
	if (videoram23[offset] != data)
	{
		videoram23[offset] = data;
		if (Machine->drv->layer[2].updatehook3) (*Machine->drv->layer[2].updatehook3)(offset);
	}
}

void videoram30_w(int offset,int data)
{
	if (videoram30[offset] != data)
	{
		videoram30[offset] = data;
		if (Machine->drv->layer[3].updatehook0) (*Machine->drv->layer[3].updatehook0)(offset);
	}
}

void videoram31_w(int offset,int data)
{
	if (videoram31[offset] != data)
	{
		videoram31[offset] = data;
		if (Machine->drv->layer[3].updatehook1) (*Machine->drv->layer[3].updatehook1)(offset);
	}
}

void videoram32_w(int offset,int data)
{
	if (videoram32[offset] != data)
	{
		videoram32[offset] = data;
		if (Machine->drv->layer[3].updatehook2) (*Machine->drv->layer[3].updatehook2)(offset);
	}
}

void videoram33_w(int offset,int data)
{
	if (videoram33[offset] != data)
	{
		videoram33[offset] = data;
		if (Machine->drv->layer[3].updatehook3) (*Machine->drv->layer[3].updatehook3)(offset);
	}
}


int videoram00_r(int offset)
{
	return videoram00[offset];
}

int videoram01_r(int offset)
{
	return videoram01[offset];
}

int videoram02_r(int offset)
{
	return videoram02[offset];
}

int videoram03_r(int offset)
{
	return videoram03[offset];
}

int videoram10_r(int offset)
{
	return videoram10[offset];
}

int videoram11_r(int offset)
{
	return videoram11[offset];
}

int videoram12_r(int offset)
{
	return videoram12[offset];
}

int videoram13_r(int offset)
{
	return videoram13[offset];
}

int videoram20_r(int offset)
{
	return videoram20[offset];
}

int videoram21_r(int offset)
{
	return videoram21[offset];
}

int videoram22_r(int offset)
{
	return videoram22[offset];
}

int videoram23_r(int offset)
{
	return videoram23[offset];
}

int videoram30_r(int offset)
{
	return videoram30[offset];
}

int videoram31_r(int offset)
{
	return videoram31[offset];
}

int videoram32_r(int offset)
{
	return videoram32[offset];
}

int videoram33_r(int offset)
{
	return videoram33[offset];
}



void videoram00_word_w(int offset,int data)
{
	int oldword = READ_WORD(&videoram00[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&videoram00[offset],newword);
		if (Machine->drv->layer[0].updatehook0) (*Machine->drv->layer[0].updatehook0)(offset);
	}
}

void videoram10_word_w(int offset,int data)
{
	int oldword = READ_WORD(&videoram10[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&videoram10[offset],newword);
		if (Machine->drv->layer[1].updatehook0) (*Machine->drv->layer[1].updatehook0)(offset);
	}
}

int videoram00_word_r(int offset)
{
	return READ_WORD(&videoram00[offset]);
}

int videoram10_word_r(int offset)
{
	return READ_WORD(&videoram10[offset]);
}
