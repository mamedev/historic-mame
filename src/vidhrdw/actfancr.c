/*******************************************************************************

	actfancr - Bryan McPhail, mish@tendril.force9.net

little endian addressed BAC-06 MXC-06 chips
Same as Oscar, Cobra Command, Bad Dudes, etc


*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned *dirty_f;
static struct osd_bitmap *pf1_bitmap;
static int actfancr_control_1[0x20],actfancr_control_2[0x20];

unsigned char *actfancr_pf1_data,*actfancr_pf2_data;

void actfancr_palette_w(int offset, int data)
{
	int r,g,b;

	data=data&0xfb; //pull out 4

	paletteram[offset]=data;
	offset=offset&0xffe;

	r = ((paletteram[offset]) >> 0) & 0xf;
	g = ((paletteram[offset]) >> 4) & 0xf;
	b = ((paletteram[offset+1]) >> 0) & 0xf;

	palette_change_color(offset / 2,r*0x11,g*0x11,b*0x11);
}

/******************************************************************************/

void actfancr_pf1_control_w(int offset, int data)
{
	actfancr_control_1[offset]=data;
}

void actfancr_pf2_control_w(int offset, int data)
{
	actfancr_control_2[offset]=data;
}

void actfancr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int my,mx,offs,color,tile,pal_base,colmask[16],i;
	int scrolly=actfancr_control_1[0x10]+(actfancr_control_1[0x11]<<8);
	int scrollx=actfancr_control_1[0x12]+(actfancr_control_1[0x13]<<8);

	palette_init_used_colors();
	memset(palette_used_colors+256,PALETTE_COLOR_USED,256);

	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < 0x800; offs += 2)
	{
		tile = actfancr_pf2_data[offs]+(actfancr_pf2_data[offs+1]<<8);
		colmask[tile>>12] |= Machine->gfx[0]->pen_usage[tile&0xfff];
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < 0x2000; offs += 2)
	{
		tile = actfancr_pf1_data[offs]+(actfancr_pf1_data[offs+1]<<8);
		colmask[tile>>12] |= Machine->gfx[2]->pen_usage[tile&0xfff];
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
    	memset(dirty_f,1,0x400);

	/* Playfield */
	mx=-1; my=0;
	for (offs = 0x000;offs < 0x400; offs += 2) {
		mx++;
		if (mx==16) {mx=0; my++;}
		if (!dirty_f[offs/2]) continue;
		dirty_f[offs/2]=0;

		tile=actfancr_pf1_data[offs]+(actfancr_pf1_data[offs+1]<<8);
		color = ((tile & 0xf000) >> 12);
        tile=tile&0xfff;

		drawgfx(pf1_bitmap,Machine->gfx[2],tile,
			color, 0,0, 16*mx,16*my,
		 	0,TRANSPARENCY_NONE,0);
	}

	/* Playfield */
	mx=-1; my=0;
	for (offs = 0x400;offs < 0x800; offs += 2) {
		mx++;
		if (mx==16) {mx=0; my++;}
		if (!dirty_f[offs/2]) continue;
		dirty_f[offs/2]=0;

		tile=actfancr_pf1_data[offs]+(actfancr_pf1_data[offs+1]<<8);
		color = ((tile & 0xf000) >> 12);
        tile=tile&0xfff;

		drawgfx(pf1_bitmap,Machine->gfx[2],tile,
			color, 0,0, (16*mx)+256,16*my,
		 	0,TRANSPARENCY_NONE,0);
	}

	scrolly=-scrolly;
	scrollx=-scrollx;
	copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,0,TRANSPARENCY_NONE,0);

	/* Sprites */
	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash;

		y=spriteram[offs]+(spriteram[offs+1]<<8);
 		if ((y&0x8000) == 0) continue;
		x = spriteram[offs+4]+(spriteram[offs+5]<<8);
		colour = ((x & 0xf000) >> 12);
		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */

											/* multi = 0   1   3   7 */
		sprite = spriteram[offs+2]+(spriteram[offs+3]<<8);
		sprite &= 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[1],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y - 16 * multi,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			multi--;
		}
	}

	/* Draw character tiles */
	for (offs = 0x800 - 2;offs >= 0;offs -= 2) {
		tile=actfancr_pf2_data[offs]+(actfancr_pf2_data[offs+1]<<8);
		if (!tile) continue;
		color=tile>>12;
		tile=tile&0xfff;
		mx = (offs/2) % 32;
		my = (offs/2) / 32;
		drawgfx(bitmap,Machine->gfx[0],
			tile,color,0,0,8*mx,8*my,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/

void actfancr_pf1_data_w(int offset, int data)
{
	actfancr_pf1_data[offset]=data;
	dirty_f[offset/2] = 1;
}

int actfancr_pf1_data_r(int offset)
{
	return actfancr_pf1_data[offset];
}

void actfancr_pf2_data_w(int offset, int data)
{
	actfancr_pf2_data[offset]=data;
}

int actfancr_pf2_data_r(int offset)
{
	return actfancr_pf2_data[offset];
}

/******************************************************************************/

void actfancr_vh_stop (void)
{
	free(dirty_f);
	osd_free_bitmap (pf1_bitmap);
}

int actfancr_vh_start (void)
{
	/* Allocate bitmaps */
	if ((pf1_bitmap = osd_create_bitmap(512,512)) == 0) {
		actfancr_vh_stop ();
		return 1;
	}

	dirty_f=malloc(0x4000); //test later

	return 0;
}

/******************************************************************************/
