/***************************************************************************
  Great Swordsman

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

//void adpcm_soundcommand_w(int offset,int data);

unsigned int gs_videoram_size;
unsigned int gs_spritexy_size;

unsigned char *gs_videoram;
unsigned char *gs_scrolly_ram;
unsigned char *gs_spritexy_ram;
unsigned char *gs_spritetile_ram;
unsigned char *gs_spriteattrib_ram;

static struct osd_bitmap 	*bitmap_bg;
static unsigned char 	 	*dirtybuffer;
static int 			video_attributes=0;
static int 			flipscreen=0;


void gsword_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,val;

		/* red component */
		val = color_prom[256+144+i*3+2];
                bit0 = (val >> 0) & 0x01;
                bit1 = (val >> 1) & 0x01;
                bit2 = (val >> 2) & 0x01;
                bit3 = (val >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		val = color_prom[256+144+i*3+1];
                bit0 = (val >> 0) & 0x01;
                bit1 = (val >> 1) & 0x01;
                bit2 = (val >> 2) & 0x01;
                bit3 = (val >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		val = color_prom[256+144+i*3+0];
                bit0 = (val >> 0) & 0x01;
                bit1 = (val >> 1) & 0x01;
                bit2 = (val >> 2) & 0x01;
                bit3 = (val >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	for (i = 0;i < TOTAL_COLORS(2);i++)
		COLOR(2,i) = color_prom[i];

	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = color_prom[256+i];

	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = color_prom[512+i];

}


int gsword_vh_start(void)
{
        if ((dirtybuffer = malloc(gs_videoram_size)) == 0) return 1;
        if ((bitmap_bg = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}
        memset(dirtybuffer,1,gs_videoram_size);
        return 0;
}

void gsword_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(bitmap_bg);
}

void gs_video_attributes_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (data != video_attributes)
	{
	        memset(dirtybuffer,1,gs_videoram_size);
		video_attributes = data;
	}
	RAM[0xa980] = data;
}

void gs_flipscreen_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (data != flipscreen)
	{
	        memset(dirtybuffer,1,gs_videoram_size);
		flipscreen = data;
	}
	RAM[0x9835] = data;
}

void gs_videoram_w(int offset,int data)
{
	if (gs_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		gs_videoram[offset] = data;
	}
}

void render_background(struct osd_bitmap *bitmap)
{
	int offs;
        int tileoffs=0,tilebank=0;

        /*Swap Tile Bank & Offset*/

        tileoffs = (video_attributes & 1) ? 256 : 0;
        tilebank = (video_attributes > 1) ?   1 : 0;

        /* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

        for (offs = 0; offs < gs_videoram_size ;offs++)
	{
                if (dirtybuffer[offs])
		{
			int sx,sy,tile,flipx,flipy,color;

                        dirtybuffer[offs] = 0;

                        sx = offs % 32;
                        sy = offs / 32;
			flipx = 0;
			flipy = 0;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}
			tile = gs_videoram[offs]+tileoffs;
			color = (tile>>6);

			drawgfx(bitmap_bg, Machine->gfx[0+tilebank],
					   tile,
                                           color,
					   flipx,flipy,
                                           8*sx,
                                           8*sy,
					   0,TRANSPARENCY_NONE,0);
		}
	}
}


void render_sprites(struct osd_bitmap *bitmap)
{
        int offs;

        for (offs = 0; offs < gs_spritexy_size - 1; offs+=2)
	{
                int sx,sy,flipx,flipy,spritebank,tile,color;

                // Dump sprite tile, x,y, and attribute

                if (osd_key_pressed(OSD_KEY_S))
                {
                        fprintf(errorlog,"tile=%x  sx=%x  sy=%x  attr1=%x  attr2=%x  color=%x\n",
			gs_spritetile_ram[offs],
			gs_spritexy_ram[offs+1],
			gs_spritexy_ram[offs],
			gs_spriteattrib_ram[offs],
			gs_spriteattrib_ram[offs+1],
			gs_spritetile_ram[offs+1]);

		}
		if (gs_spritexy_ram[offs]!=0xf1)
		{
	                spritebank = 0;
        	        tile = gs_spritetile_ram[offs];
                	sy = 241-gs_spritexy_ram[offs];
	                sx = gs_spritexy_ram[offs+1]-56;
	                flipx = gs_spriteattrib_ram[offs] & 0x02;
        	        flipy = gs_spriteattrib_ram[offs] & 0x01;

        	        // Adjust sprites that should be far far right!
	                if (sx<0) sx+=256;

	                // Color select?
        	        color = (gs_spritetile_ram[offs+1] & 0x3f);

	                // Adjuste for 32x32 tiles(#128-256)
        	        if (tile > 127)
			{
                        	spritebank = 1;
	                        tile -= 128;
	                        sy-=16;
			}
			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}

	                drawgfx(bitmap, Machine->gfx[2+spritebank],
					tile,
					color,
					flipx,flipy,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
		}
	}
}


void gsword_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int scrollx=0,scrolly;

	render_background(bitmap_bg);
	scrolly = -(*gs_scrolly_ram);
	copyscrollbitmap(bitmap,bitmap_bg,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	render_sprites(bitmap);

	/* ADPCM SOUND TEST */

//	if (osd_key_pressed(OSD_KEY_Q))
//	{
//		adpcm_soundcommand_w(0,9);
//	        while (osd_key_pressed(OSD_KEY_Q));
//	}
//	if (osd_key_pressed(OSD_KEY_W))
//	{
//		adpcm_soundcommand_w(0,4);
//	        while (osd_key_pressed(OSD_KEY_W));
//	}
//	if (osd_key_pressed(OSD_KEY_E))
//	{
//		adpcm_soundcommand_w(0,6);
//	        while (osd_key_pressed(OSD_KEY_E));
//	}
//	if (osd_key_pressed(OSD_KEY_R))
//	{
//		adpcm_soundcommand_w(0,31);
//	        while (osd_key_pressed(OSD_KEY_R));
//	}
}

