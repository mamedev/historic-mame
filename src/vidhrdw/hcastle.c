/***************************************************************************

	Haunted Castle video emulation

	Nb:  Still a lot of test code in this file that can be removed later,
	graphics banking is only testing up to the end of level 2.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *pf1_bitmap;
static struct osd_bitmap *pf2_bitmap;
static unsigned char *dirty_pf1,*dirty_pf2,*private_spriteram_2,*private_spriteram;
unsigned char *hcastle_pf1_control,*hcastle_pf2_control;
unsigned char *hcastle_pf1_videoram,*hcastle_pf2_videoram;
static int gfx_bank;



void hcastle_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,pal;


	for (i = 0;i < 4*16*16;i++)
	{
		for (pal = 0;pal < 8;pal++)
			colortable[pal*4*16*16 + i] = 16 * pal + (*color_prom & 0x0f);

		color_prom++;
	}
}



void hcastle_pf1_video_w(int offset, int data)
{
	hcastle_pf1_videoram[offset]=data;
	dirty_pf1[offset]=1;
}

void hcastle_pf2_video_w(int offset, int data)
{
	hcastle_pf2_videoram[offset]=data;
	dirty_pf2[offset]=1;
}

void hcastle_gfxbank_w(int offset, int data)
{
	gfx_bank=data;
}

void hcastle_pf1_control_w(int offset,int data)
{
	hcastle_pf1_control[offset]=data;
	if (offset==3) {
		if ((hcastle_pf1_control[3]&0x8)==0)
			memcpy(private_spriteram,spriteram+0x800,0x800);
		else
			memcpy(private_spriteram,spriteram,0x800);
	}
}

void hcastle_pf2_control_w(int offset,int data)
{
	hcastle_pf2_control[offset]=data;
	if (offset==3) {
		if ((hcastle_pf2_control[3]&0x8)==0)
			memcpy(private_spriteram_2,spriteram_2+0x800,0x800);
		else
			memcpy(private_spriteram_2,spriteram_2,0x800);
	}
}

/*****************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap, unsigned char *sbank, int bank )
{
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned char *source,*finish;

//	if ((hcastle_pf1_control[3]&0x8)==0x8) source = sbank + 0x800;
//	else

	source = sbank;
	finish = source + 0x800;

/*

	Sprites, 5 bytes per sprite.  See also Contra, Combat School

	0:	0xff	- Sprite number
	1:	0xf0	- Color
		0x08	- Fine control for 16x8/8x8 sprites (*may* have other use for 16x16 sprites)
		0x04	- Fine control for 16x8/8x8 sprites
		0x02	- Tile bank bit 1
		0x01	- Tile bank bit 0
	2:	0xff	- Y position
	3:	0xff	- X position
	4:	0x80	- Tile bank bit 3
		0x40	- Tile bank bit 2
		0x20	- Y flip
		0x10	- X flip
		0x08	- Size
		0x04	- Size
		0x02 	- Size
		0x01	- X position high bit

	Known sizes:
		0x08	- 32 x 32 sprite (Combination of 4 16x16's)
		0x06	- 8 x 8 sprite
		0x04 	- 8 x 16 sprite
		0x02 	- 16 x 8 sprite
		0x00 	- 16 x 16 sprite

If using 16x8 sprites then the tile number given needs to be multipled by 2!
If using 8x8 sprites then the tile number given needs to be multipled by 4!

The 'fine control' bits are then added to get the correct sprite within the group
of two/four.

*/
	while( (source+5)<finish ){
		int attributes = source[4];
		int sx = source[3];
		int sy = source[2];
		int tile_number = source[0];
		int color =  source[1];

		int yflip		= 0x20 & attributes;
		int xflip		= 0x10 & attributes;
		int width = 0, height = 0;

		if( attributes&0x01 ) sx -= 256;
		if( sy > 239 ) sy = 0 - (256 - sy);

		tile_number += ((color&0x3)<<8) + ((attributes&0xc0)<<4);
		tile_number = tile_number<<2;
		tile_number += (color>>2)&3;



	if (bank == 0)
		tile_number += 0x4000 * (gfx_bank & 1);

	color=((color&0xf0)>>4);


		switch( attributes&0xe ){
			case 0x00: width = height = 2; tile_number &= (~3); break;
			case 0x08: width = height = 4; tile_number &= (~3); break;
			case 0x06: width = height = 1; break;
			case 0x02: width = 2; height = 1; tile_number &= (~1); break;
			default: width = 1; height = 1;
					//if (errorlog) fprintf(errorlog,"Unknown sprite size %02x\n",attributes&0xe);
		}

		{
			static int x_offset[4] = { 0x0,0x1,0x4,0x5 };
			static int y_offset[4] = { 0x0,0x2,0x8,0xa };
			int x,y, ex, ey;

			for( y=0; y<height; y++ ){
				for( x=0; x<width; x++ ){
					ex = xflip?(width-1-x):x;
					ey = yflip?(height-1-y):y;

					drawgfx(bitmap,Machine->gfx[bank],
						tile_number+x_offset[ex]+y_offset[ey],
						(2*bank)*4*16+(2*bank)*16+color,
						xflip,yflip,
						sx+x*8,sy+y*8,
						clip,TRANSPARENCY_PEN,0);
				}
			}
		}
		source += 5;
	}
}

/*****************************************************************************/

void hcastle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,tile,color,mx,my,bank,scrollx,scrolly;
	int pf2_bankbase,pf1_bankbase,tx;
	static int old_pf1,old_pf2;


	palette_init_used_colors();
	memset(palette_used_colors,PALETTE_COLOR_USED,128);
	palette_used_colors[0*16] = PALETTE_COLOR_TRANSPARENT;
	palette_used_colors[1*16] = PALETTE_COLOR_TRANSPARENT;
	palette_used_colors[2*16] = PALETTE_COLOR_TRANSPARENT;
	palette_used_colors[3*16] = PALETTE_COLOR_TRANSPARENT;

	pf1_bankbase = 0x0000;
	pf2_bankbase = 0x4000 * ((gfx_bank & 2) >> 1);

	if (hcastle_pf1_control[3]&0x1) pf1_bankbase += 0x2000;
	if (hcastle_pf2_control[3]&0x1) pf2_bankbase += 0x2000;

	if (palette_recalc() || pf1_bankbase!=old_pf1 || pf2_bankbase!=old_pf2)
	{
		memset(dirty_pf1,1,0x1000);
		memset(dirty_pf2,1,0x1000);
	}
	old_pf1=pf1_bankbase;
	old_pf2=pf2_bankbase;

	/* Draw foreground */
	for (my = 0;my < 32;my++)
	{
		for (mx = 0;mx < 64;mx++)
		{
			if (mx >= 32)
				offs = 0x800 + my*32 + (mx-32);
			else
				offs = my*32 + mx;

			if (!dirty_pf1[offs] && !dirty_pf1[offs+0x400]) continue;
			dirty_pf1[offs]=dirty_pf1[offs+0x400]=0;

			tile=hcastle_pf1_videoram[offs+0x400];
			tx = hcastle_pf1_videoram[offs];
			color = tx&0x7;
			bank = ((tx & 0x80) >> 7) | ((tx & 0x40) >> 2) | ((tx & 0x30) >> 3) | ((tx & 0x08) << 0);

			drawgfx(pf1_bitmap,Machine->gfx[0],
					tile+bank*0x100+pf1_bankbase,
					1*4*16+1*16+color,
					0,0,//fx,0,
					8*mx,8*my,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw background */
	for (my = 0;my < 32;my++)
	{
		for (mx = 0;mx < 64;mx++)
		{
			if (mx >= 32)
				offs = 0x800 + my*32 + (mx-32);
			else
				offs = my*32 + mx;

			if (!dirty_pf2[offs] && !dirty_pf2[offs+0x400]) continue;
			dirty_pf2[offs]=dirty_pf2[offs+0x400]=0;

			tile=hcastle_pf2_videoram[offs+0x400];
			tx = hcastle_pf2_videoram[offs];
			color = tx&0x7;
			bank = ((tx & 0x80) >> 7) | ((tx & 0x40) >> 2) | ((tx & 0x30) >> 3) | ((tx & 0x08) << 0);

/*

MAP = 7 B

0x40 is set..  (0x8 is set)


Level 2:

6 for most.. 7 for others..

*/

			drawgfx(pf2_bitmap,Machine->gfx[1],
					tile+bank*0x100+pf2_bankbase,
					3*4*16+3*16+color,
					0,0, //fx,0,
					8*mx,8*my,
					0,TRANSPARENCY_NONE,0);
		}
	}


//	/* Sprite priority */
//	if (hcastle_pf1_control[3]&0x20)
	if ((gfx_bank & 0x04) == 0)
	{
		scrolly = -hcastle_pf2_control[2];
		scrollx = -((hcastle_pf2_control[1]<<8)+hcastle_pf2_control[0]);
		copyscrollbitmap(bitmap,pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		draw_sprites( bitmap, private_spriteram, 0 );
		draw_sprites( bitmap, private_spriteram_2, 1 );

		scrolly = -hcastle_pf1_control[2];
		scrollx = -((hcastle_pf1_control[1]<<8)+hcastle_pf1_control[0]);
		copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else
	{
		scrolly = -hcastle_pf2_control[2];
		scrollx = -((hcastle_pf2_control[1]<<8)+hcastle_pf2_control[0]);
		copyscrollbitmap(bitmap,pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		scrolly = -hcastle_pf1_control[2];
		scrollx = -((hcastle_pf1_control[1]<<8)+hcastle_pf1_control[0]);
		copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

		draw_sprites( bitmap, private_spriteram, 0 );
		draw_sprites( bitmap, private_spriteram_2, 1 );
	}
}

/*****************************************************************************/

int hcastle_vh_start(void)
{
 	if ((pf1_bitmap = osd_create_bitmap(64*8,32*8)) == 0)
		return 1;
	if ((pf2_bitmap = osd_create_bitmap(64*8,32*8)) == 0)
		return 1;

	dirty_pf1=malloc (0x1000);
	dirty_pf2=malloc (0x1000);
	private_spriteram=malloc (0x800);
	private_spriteram_2=malloc (0x800);
	memset(dirty_pf1,1,0x1000);
	memset(dirty_pf2,1,0x1000);
	memset(private_spriteram,0,0x800);
	memset(private_spriteram_2,0,0x800);

	return 0;
}

void hcastle_vh_stop(void)
{
	osd_free_bitmap(pf1_bitmap);
	osd_free_bitmap(pf2_bitmap);
	free(dirty_pf1);
	free(dirty_pf2);
	free(private_spriteram);
	free(private_spriteram_2);
}
