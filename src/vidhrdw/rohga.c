/***************************************************************************

	Rohga Video emulation - Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "deco16ic.h"

/******************************************************************************/

static int wizdfire_bank_callback(const int bank)
{
	return ((bank>>4)&0x3)<<12;
}

VIDEO_START( rohga )
{
	if (deco16_2_video_init(0))
		return 1;

	deco16_set_tilemap_bank_callback(0,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(1,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(2,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(3,wizdfire_bank_callback);

	return 0;
}

VIDEO_START( wizdfire )
{
	if (deco16_2_video_init(0))
		return 1;

	deco16_set_tilemap_bank_callback(0,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(1,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(2,wizdfire_bank_callback);
	deco16_set_tilemap_bank_callback(3,wizdfire_bank_callback);

	deco16_pf1_rowscroll=deco16_pf2_rowscroll=0;

	alpha_set_level(0x80);

	return 0;
}

/******************************************************************************/

static void rohga_drawsprites(struct mame_bitmap *bitmap, const data16_t *spriteptr)
{
	int offs;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult,pri=0;
		sprite = spriteptr[offs+1];
		if (!sprite) continue;

		x = spriteptr[offs+2];

		/* Sprite/playfield priority */
		switch (x&0xc000) {
		case 0x0000: pri=0; break;
		case 0x4000: pri=0xf0; break;
		case 0x8000: pri=0xf0|0xcc; break;
		case 0xc000: pri=0xf0|0xcc; break; /* Perhaps 0xf0|0xcc|0xaa (Sprite under bottom layer) */
		}
//todo - test above..
		y = spriteptr[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;
		colour = (x >> 9) &0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen) {
			x=304-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=-16;
		}
		else mult=+16;

		while (multi >= 0)
		{
			pdrawgfx(bitmap,Machine->gfx[3],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0,pri);

			multi--;
		}
	}
}

static void wizdfire_drawsprites(struct mame_bitmap *bitmap, data16_t *spriteptr, int mode, int bank)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;
		int trans=TRANSPARENCY_PEN;

		sprite = spriteptr[offs+1];
		if (!sprite) continue;

		x = spriteptr[offs+2];

		/*
		Sprite/playfield priority - we can't use pdrawgfx because we need alpha'd sprites overlaid
		over non-alpha'd sprites, plus sprites underneath and above an alpha'd background layer.

		Hence, we rely on the hardware sorting everything correctly and not relying on any orthoganality
		effects (it doesn't seem to), and instead draw seperate passes for each sprite priority.  :(
		*/
		switch (mode) {
		case 4:
			if ((x&0xc000)!=0xc000)
				continue;
			break;
		case 3:
			if ((x&0xc000)!=0x8000)
				continue;
			break;
		case 2:
			if ((x&0x8000)!=0x8000)
				continue;
			break;
		case 1:
		case 0:
		default:
			if ((x&0x8000)!=0)
				continue;
			break;
		}

		y = spriteptr[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;
		colour = (x >> 9) &0x1f;

		if (bank==4 && colour&0x10) {
			trans=TRANSPARENCY_ALPHA;
			colour&=0xf;
		}

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen) {
			x=304-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=-16;
		}
		else
			mult=+16;

		if (fx) fx=0; else fx=1;
		if (fy) fy=0; else fy=1;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[bank],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,trans,0);

			multi--;
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( rohga )
{
	/* Update playfields */
//	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);
	deco16_pf34_update(deco16_pf3_rowscroll,deco16_pf4_rowscroll);

	/* Draw playfields */
	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[512],cliprect);

	if (!keyboard_pressed(KEYCODE_Z))
	deco16_tilemap_4_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,1);
	if (!keyboard_pressed(KEYCODE_X))
	deco16_tilemap_3_draw(bitmap,cliprect,0,2);
	if (!keyboard_pressed(KEYCODE_C))
	deco16_tilemap_2_draw(bitmap,cliprect,0,4);

	rohga_drawsprites(bitmap,spriteram16);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);

//	deco16_print_debug_info();
}

VIDEO_UPDATE( wizdfire )
{
	/* Update playfields */
	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);
	deco16_pf34_update(deco16_pf3_rowscroll,deco16_pf4_rowscroll);

	/* Draw playfields - Palette of 2nd playfield chip visible if playfields turned off */
	fillbitmap(bitmap,Machine->pens[512],&Machine->visible_area);

	deco16_tilemap_4_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
	wizdfire_drawsprites(bitmap,buffered_spriteram16,4,3);
	deco16_tilemap_2_draw(bitmap,cliprect,0,0);
	wizdfire_drawsprites(bitmap,buffered_spriteram16,3,3);

	if ((deco16_priority&0x1f)==0x1f) /* Wizdfire has bit 0x40 always set, Dark Seal 2 doesn't?! */
		deco16_tilemap_3_draw(bitmap,cliprect,TILEMAP_ALPHA,0);
	else
		deco16_tilemap_3_draw(bitmap,cliprect,0,0);

	/* See notes in wizdfire_drawsprites about this */
	wizdfire_drawsprites(bitmap,buffered_spriteram16,0,3);
	wizdfire_drawsprites(bitmap,buffered_spriteram16_2,2,4);
	wizdfire_drawsprites(bitmap,buffered_spriteram16_2,1,4);

	deco16_tilemap_1_draw(bitmap,cliprect,0,0);
}
