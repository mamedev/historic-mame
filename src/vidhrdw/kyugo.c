#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *kyugo_videoram;
size_t kyugo_videoram_size;
unsigned char *kyugo_back_scrollY_lo;
unsigned char *kyugo_back_scrollX;

static unsigned char kyugo_back_scrollY_hi;
static int palbank,frontcolor;
static int flipscreen;



WRITE_HANDLER( kyugo_gfxctrl_w )
{
	/* bit 0 is scroll MSB */
	kyugo_back_scrollY_hi = data & 0x01;

	/* bit 5 is front layer color (Son of Phoenix only) */
	frontcolor = (data & 0x20) >> 5;

	/* bit 6 is background palette bank */
	if (palbank != ((data & 0x40) >> 6))
	{
		palbank = (data & 0x40) >> 6;
		memset(dirtybuffer,1,videoram_size);
	}

if (data & 0x9e)
{
	char baf[40];
	sprintf(baf,"%02x",data);
	usrintf_showmessage(baf);
}
}


WRITE_HANDLER( kyugo_flipscreen_w )
{
	if (flipscreen != (data & 0x01))
	{
		flipscreen = (data & 0x01);
		memset(dirtybuffer,1,videoram_size);
	}
}



static void draw_sprites(struct mame_bitmap *bitmap)
{
	/* sprite information is scattered through memory */
	/* and uses a portion of the text layer memory (outside the visible area) */
	unsigned char *spriteram_area1 = &spriteram[0x28];
	unsigned char *spriteram_area2 = &spriteram_2[0x28];
	unsigned char *spriteram_area3 = &kyugo_videoram[0x28];

	int n;

	for( n = 0; n < 12*2; n++ )
	{
		int offs,y,sy,sx,color;

		offs = 2*(n % 12) + 64*(n / 12);

		sx = spriteram_area3[offs+1] + 256 * (spriteram_area2[offs+1] & 1);
		if (sx > 320) sx -= 512;

		sy = 255 - spriteram_area1[offs];
		if (flipscreen) sy = 240 - sy;

		color = spriteram_area1[offs+1] & 0x1f;

		for (y = 0;y < 16;y++)
		{
			int attr2,code,flipx,flipy;

			attr2 = spriteram_area2[offs + 128*y];
			code = spriteram_area3[offs + 128*y];
			if (attr2 & 0x01) code += 512;
			if (attr2 & 0x02) code += 256;
			flipx =  attr2 & 0x08;
			flipy =  attr2 & 0x04;
			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx( bitmap, Machine->gfx[1],
					 code,
					 color,
					 flipx,flipy,
					 sx,flipscreen ? sy - 16*y : sy + 16*y,
					 &Machine->visible_area,TRANSPARENCY_PEN, 0 );
		 }
	}
}



void kyugo_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs;
	const UINT8 *color_codes = memory_region(REGION_PROMS) + 0x300;


	/* back layer */
	for( offs = videoram_size - 1;offs >= 0;offs-- )
	{
		if ( dirtybuffer[offs] )
		{
			int sx,sy,tile,flipx,flipy;

			dirtybuffer[offs] = 0;

			sx = offs % 64;
			sy = offs / 64;
			flipx = colorram[offs] & 0x04;
			flipy = colorram[offs] & 0x08;
			if (flipscreen)
			{
				sx = 63 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			tile = videoram[offs] + ( 256 * ( colorram[offs] & 3 ) );

			drawgfx( tmpbitmap, Machine->gfx[2],
					tile,
					((colorram[offs] & 0xf0) >> 4) + (palbank << 4),
					flipx,flipy,
					8*sx, 8*sy,
					0,TRANSPARENCY_NONE, 0 );
		}
	}

	{
		int scrollx,scrolly;

		if (flipscreen)
		{
			scrollx = -32 - ( ( kyugo_back_scrollY_lo[0] ) + ( kyugo_back_scrollY_hi * 256 ) );
			scrolly = kyugo_back_scrollX[0];
		}
		else
		{
			scrollx = -32 - ( ( kyugo_back_scrollY_lo[0] ) + ( kyugo_back_scrollY_hi * 256 ) );
			scrolly = -kyugo_back_scrollX[0];
		}

		/* copy the temporary bitmap to the screen */
		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	/* sprites */
	draw_sprites(bitmap);

	/* front layer */
	for( offs = kyugo_videoram_size - 1;offs >= 0;offs-- )
	{
		int sx,sy,code;

		sx = offs % 64;
		sy = offs / 64;
		if (flipscreen)
		{
			sx = 35 - sx;
			sy = 31 - sy;
		}

		code = kyugo_videoram[offs];

		drawgfx( bitmap, Machine->gfx[0],
				code,
				2*color_codes[code/8] + frontcolor,
				flipscreen, flipscreen,
				8*sx, 8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
