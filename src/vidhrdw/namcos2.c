/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "namcoic.h"

#define get_gfx_pointer(gfxelement,c,line) (gfxelement->gfxdata + (c*gfxelement->height+line) * gfxelement->line_modulo)

data16_t *namcos2_sprite_ram;

static struct tilemap *tilemap[6];
static struct tilemap *tilemap_roz;

static data16_t namcos2_68k_vram_ctrl[0x20];
static data16_t namcos2_gfx_ctrl;
static int palette_bank_dirty[32];


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void get_tile_info(int tile_index,data16_t *vram,int color)
{
	int tile;
	tile = vram[tile_index];
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = memory_region(REGION_GFX4)+(0x08*tile);
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	SET_TILE_INFO(
			GFX_CHR,
			tile,
			color,
			0)
}

static void get_tile_info0(int tile_index) { get_tile_info(tile_index,videoram16+0x0000,0); }
static void get_tile_info1(int tile_index) { get_tile_info(tile_index,videoram16+0x1000,2); }
static void get_tile_info2(int tile_index) { get_tile_info(tile_index,videoram16+0x2000,4); }
static void get_tile_info3(int tile_index) { get_tile_info(tile_index,videoram16+0x3000,6); }
static void get_tile_info4(int tile_index) { get_tile_info(tile_index,videoram16+0x4008,8); }
static void get_tile_info5(int tile_index) { get_tile_info(tile_index,videoram16+0x4408,10); }

static void get_tile_info_roz(int tile_index)
{
	int tile;
	tile = namcos2_68k_roz_ram[tile_index];
	SET_TILE_INFO(
			GFX_ROZ,
			tile,
			0,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( namcos2 )
{
	int i;


	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[4] = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(get_tile_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap_roz = tilemap_create(get_tile_info_roz,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,256,256);

	if (!tilemap[0] || !tilemap[1] || !tilemap[2] || !tilemap[3] || !tilemap[4] || !tilemap[5] || !tilemap_roz)
		return 1;

	tilemap_set_transparent_pen(tilemap_roz,0xff);


	/* set table for sprite color == 0x0f */
	for(i = 0;i <= 253;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	gfx_drawmode_table[254] = DRAWMODE_SHADOW;
	gfx_drawmode_table[255] = DRAWMODE_NONE;

	for (i = 0;i < 14*256;i++)
		palette_shadow_table[Machine->pens[i+16*256]] = Machine->pens[(i+16*256)|256];

	for (i = 0;i < 32;i++)
		palette_bank_dirty[i] = 1;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

/*************************************************************/
/* 68000 Shared memory area - Video RAM control 			 */
/*************************************************************/

size_t namcos2_68k_vram_size;

WRITE16_HANDLER( namcos2_68k_vram_w )
{
	data16_t oldword = videoram16[offset];
	COMBINE_DATA( &videoram16[offset] );
	/* Note: some games appear to use the 409000 region as SRAM to */
	/* communicate between master/slave processors ??		       */

	if( oldword != videoram16[offset] ){
		if( offset<0x4000 ){
			switch( offset&0x3000 ){
			case 0x0000:
				tilemap_mark_tile_dirty(tilemap[0],offset&0xfff);
				break;
			case 0x1000:
				tilemap_mark_tile_dirty(tilemap[1],offset&0xfff);
				break;
			case 0x2000:
				tilemap_mark_tile_dirty(tilemap[2],offset&0xfff);
				break;
			case 0x3000:
				tilemap_mark_tile_dirty(tilemap[3],offset&0xfff);
				break;
			}
		}
		else if( offset>=0x8010/2 && offset<0x87f0/2 ){
			offset-=0x8010/2;	/* Fixed plane offsets */
			tilemap_mark_tile_dirty( tilemap[4], offset );
		}
		else if( offset>=0x8810/2 && offset<0x8ff0/2 ){
			offset-=0x8810/2;	/* Fixed plane offsets */
			tilemap_mark_tile_dirty( tilemap[5], offset );
		}
	}
}

READ16_HANDLER( namcos2_68k_vram_r )
{
	return videoram16[offset];
}


WRITE16_HANDLER( namcos2_68k_vram_ctrl_w )
{
	data16_t oldword, newword;
	static int flipscreen;
	oldword = namcos2_68k_vram_ctrl[offset];
	COMBINE_DATA( &namcos2_68k_vram_ctrl[offset] );
	newword = namcos2_68k_vram_ctrl[offset];


	if (oldword != newword)
	{
		switch( offset )
		{
			case 0x02/2:
				/* All planes are flipped X+Y from D15 of this word */
				flipscreen = newword & 0x8000;
				tilemap_set_flip(tilemap[0],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);
				tilemap_set_flip(tilemap[1],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);
				tilemap_set_flip(tilemap[2],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);
				tilemap_set_flip(tilemap[3],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);
				tilemap_set_flip(tilemap[4],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);
				tilemap_set_flip(tilemap[5],flipscreen ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0);

				if (flipscreen)
					tilemap_set_scrollx( tilemap[0], 0, -288-(newword+44+4) );
				else
					tilemap_set_scrollx( tilemap[0], 0, newword+44+4 );
				break;
			case 0x06/2:
				if (flipscreen)
					tilemap_set_scrolly( tilemap[0], 0, -224-(newword+24) );
				else
					tilemap_set_scrolly( tilemap[0], 0, newword+24 );
				break;
			case 0x0a/2:
				if (flipscreen)
					tilemap_set_scrollx( tilemap[1], 0, -288-(newword+44+2) );
				else
					tilemap_set_scrollx( tilemap[1], 0, newword+44+2 );
				break;
			case 0x0e/2:
				if (flipscreen)
					tilemap_set_scrolly( tilemap[1], 0, -224-(newword+24) );
				else
					tilemap_set_scrolly( tilemap[1], 0, newword+24 );
				break;
			case 0x12/2:
				if (flipscreen)
					tilemap_set_scrollx( tilemap[2], 0, -288-(newword+44+1) );
				else
					tilemap_set_scrollx( tilemap[2], 0, newword+44+1 );
				break;
			case 0x16/2:
				if (flipscreen)
					tilemap_set_scrolly( tilemap[2], 0, -224-(newword+24) );
				else
					tilemap_set_scrolly( tilemap[2], 0, newword+24 );
				break;
			case 0x1a/2:
				if (flipscreen)
					tilemap_set_scrollx( tilemap[3], 0, -288-(newword+44) );
				else
					tilemap_set_scrollx( tilemap[3], 0, newword+44 );
				break;
			case 0x1e/2:
				if (flipscreen)
					tilemap_set_scrolly( tilemap[3], 0, -224-(newword+24) );
				else
					tilemap_set_scrolly( tilemap[3], 0, newword+24 );
				break;
		}
	}
}

READ16_HANDLER( namcos2_68k_vram_ctrl_r )
{
	return namcos2_68k_vram_ctrl[offset];
}


/*************************************************************/
/* 68000 Shared memory area - Video palette control 		 */
/*************************************************************/

data16_t *namcos2_68k_palette_ram;
size_t namcos2_68k_palette_size;

READ16_HANDLER( namcos2_68k_video_palette_r )
{
	offset*=2;
	/* 0x3000 offset is control registers */
	if( (offset & 0xf000) == 0x3000 )
	{
		/* Palette chip control registers */
		offset&=0x001f;
		switch( offset ){
			case 0x1a:
			case 0x1e:
				return 0xff;
				break;
			default:
				break;
		}
	}
	return namcos2_68k_palette_ram[(offset/2)&0x7fff];
}

WRITE16_HANDLER( namcos2_68k_video_palette_w )
{
	int pen;


	COMBINE_DATA(&namcos2_68k_palette_ram[offset]);

	pen = ((offset & 0x6000) >> 2) | (offset & 0x07ff);
	palette_bank_dirty[pen / 256] = 1;
}


WRITE16_HANDLER( namcos2_gfx_ctrl_w )
{
	COMBINE_DATA(&namcos2_gfx_ctrl);
}


WRITE16_HANDLER( namcos2_sprite_ram_w )
{
	COMBINE_DATA(&namcos2_sprite_ram[offset]);
}



/**************************************************************/
/*	ROZ - Rotate & Zoom memory function handlers			  */
/*															  */
/*	0 - inc xx												  */
/*	2 - inc xy	 											  */
/*	4 - inc yx	 											  */
/*	6 - inc yy												  */
/*	8 - start x												  */
/*	A	start y												  */
/*	C - ??													  */
/*	E - ??													  */
/*															  */
/**************************************************************/

static data16_t namcos2_68k_roz_ctrl[0x8];
size_t namcos2_68k_roz_ram_size;
data16_t *namcos2_68k_roz_ram;

READ16_HANDLER(namcos2_68k_roz_ctrl_r)
{
	return namcos2_68k_roz_ctrl[offset];
}

WRITE16_HANDLER( namcos2_68k_roz_ctrl_w )
{
	COMBINE_DATA(&namcos2_68k_roz_ctrl[offset]);
}

READ16_HANDLER( namcos2_68k_roz_ram_r )
{
	return namcos2_68k_roz_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_roz_ram_w )
{
	data16_t oldword = namcos2_68k_roz_ram[offset];
	COMBINE_DATA(&namcos2_68k_roz_ram[offset]);
	if (oldword != namcos2_68k_roz_ram[offset])
		tilemap_mark_tile_dirty(tilemap_roz,offset);
}




/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_layerROZ(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	const int xoffset = 38,yoffset = 0;

	incxx =  (INT16)namcos2_68k_roz_ctrl[0];
	incxy =  (INT16)namcos2_68k_roz_ctrl[1];
	incyx =  (INT16)namcos2_68k_roz_ctrl[2];
	incyy =  (INT16)namcos2_68k_roz_ctrl[3];
	startx = (INT16)namcos2_68k_roz_ctrl[4];
	starty = (INT16)namcos2_68k_roz_ctrl[5];

//usrintf_showmessage("%04x %04x",namcos2_68k_roz_ctrl[6],namcos2_68k_roz_ctrl[7]);

	startx <<= 4;
	starty <<= 4;
	startx += xoffset * incxx + yoffset * incyx;
	starty += xoffset * incxy + yoffset * incyy;

	tilemap_draw_roz(bitmap,cliprect,tilemap_roz,startx << 8,starty << 8,
			incxx << 8,incxy << 8,incyx << 8,incyy << 8,
			1,	/* copy with wraparound */
			0,0);
}


static void draw_sprites_default( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;
	struct rectangle rect;

	offset=(namcos2_gfx_ctrl & 0x000f) * (128*4);

	for(loop=0;loop < 128;loop++)
	{
		/****************************************
		* Sprite data is 8 byte packed format   *
		*                                       *
		* Offset 0,1                            *
		*   Sprite Y position           D00-D08 *
		*   Sprite Size 16/32           D09     *
		*   Sprite Size Y               D10-D15 *
		*                                       *
		* Offset 2,3                            *
		*   Sprite Quadrant             D00-D01 *
		*   Sprite Number               D02-D12 *
		*   Sprite ROM Bank select      D13     *
		*   Sprite flip X               D14     *
		*   Sprite flip Y               D15     *
		*                                       *
		* Offset 4,5                            *
		*   Sprite X position           D00-D10 *
		*                                       *
		* Offset 6,7                            *
		*   Sprite priority             D00-D02 *
		*   Sprite colour index         D04-D07 *
		*   Sprite Size X               D10-D15 *
		*                                       *
		****************************************/

		offset0 = namcos2_sprite_ram[offset+(loop*4)+0];
		offset2 = namcos2_sprite_ram[offset+(loop*4)+1];
		offset4 = namcos2_sprite_ram[offset+(loop*4)+2];
		offset6 = namcos2_sprite_ram[offset+(loop*4)+3];

		/* Fetch sprite size registers */

		sizey=((offset0>>10)&0x3f)+1;
		sizex=(offset6>>10)&0x3f;

		if((offset0&0x0200)==0) sizex>>=1;

		if((sizey-1) && sizex && (offset6&0x0007)==pri)
		{
			int color = (offset6>>4)&0x000f;

			rect=Machine->visible_area;

			sprn=(offset2>>2)&0x7ff;
			spr_region=(offset2&0x2000)?GFX_OBJ2:GFX_OBJ1;

			ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
			xpos=(offset4&0x03ff)-0x50+0x07;

			flipy=offset2&0x8000;
			flipx=offset2&0x4000;

			scalex=((sizex<<16)/((offset0&0x0200)?0x20:0x10));
			scaley=((sizey<<16)/((offset0&0x0200)?0x20:0x10));

			/* Set the clipping rect to mask off the other portion of the sprite */
			rect.min_x=xpos;
			rect.max_x=xpos+(sizex-1);
			rect.min_y=ypos;
			rect.max_y=ypos+(sizey-1);

			if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
			if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
			if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
			if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

			if((offset0&0x0200)==0)
			{
				if(((offset2&0x0001) && !flipx) || (!(offset2&0x0001) && flipx)) xpos-=sizex;
				if(((offset2&0x0002) && !flipy) || (!(offset2&0x0002) && flipy)) ypos-=sizey;
			}

			if((scalex==(1<<16)) && (scaley==(1<<16)))
			{
				drawgfx(bitmap,Machine->gfx[spr_region],
					sprn,
					color,
					flipx,flipy,
					xpos,ypos,
					&rect,(color==0x0f ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN),0xff);
			}
			else if(scalex && scaley)
			{
				drawgfxzoom(bitmap,Machine->gfx[spr_region],
					sprn,
					color,
					flipx,flipy,
					xpos,ypos,
					&rect,(color==0x0f ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN),0xff,
					scalex,scaley);
			}
		}
	}
}


static void draw_sprites_finallap( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;
	struct rectangle rect;

	offset=(namcos2_gfx_ctrl & 0x000f) * (128*4);

	for(loop=0;loop < 128;loop++)
	{
		/****************************************
		* Sprite data is 8 byte packed format   *
		*                                       *
		* Offset 0,1                            *
		*   Sprite Y position           D00-D08 *
		*   Sprite ROM Bank select      D09     * DIFFERENT FROM DEFAULT SPRITES
		*   Sprite Size Y               D10-D15 *
		*                                       *
		* Offset 2,3                            *
		*   Sprite Quadrant             D00-D01 *
		*   Sprite Number               D02-D12 *
		*   Sprite Size 16/32           D13     * DIFFERENT FROM DEFAULT SPRITES
		*   Sprite flip X               D14     *
		*   Sprite flip Y               D15     *
		*                                       *
		* Offset 4,5                            *
		*   Sprite X position           D00-D10 *
		*                                       *
		* Offset 6,7                            *
		*   Sprite priority             D00-D03 * DIFFERENT FROM DEFAULT SPRITES 4 BIT
		*   Sprite colour index         D04-D07 *
		*   Sprite Size X               D10-D15 *
		*                                       *
		****************************************/

		offset0 = namcos2_sprite_ram[offset+(loop*4)+0];
		offset2 = namcos2_sprite_ram[offset+(loop*4)+1];
		offset4 = namcos2_sprite_ram[offset+(loop*4)+2];
		offset6 = namcos2_sprite_ram[offset+(loop*4)+3];

		/* Fetch sprite size registers */

		sizey=((offset0>>10)&0x3f)+1;
		sizex=(offset6>>10)&0x3f;

		if((offset2&0x2000)==0) sizex>>=1;

		if((sizey-1) && sizex && (offset6&0x000f)==pri)
		{
			sprn=(offset2>>2)&0x7ff;
			spr_region=(offset0&0x0200)?GFX_OBJ2:GFX_OBJ1;
			spr_region=GFX_OBJ1;	// Always fixed on Final Lap

			if(offset0&0x0200)
			{
				ypos=((offset0&0x0100)?-0x100:0)+(offset0&0x00ff)+0xa8;	// 0x70
				xpos=((offset4&0x0200)?-0x200:0)+(offset4&0x01ff)+0x90;	// 0x90
			}
			else
			{
				ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
				xpos=(offset4&0x03ff)-0x50+0x07;
			}


			flipy=offset2&0x8000;
			flipx=offset2&0x4000;

			scalex=((sizex<<16)/((offset2&0x2000)?0x20:0x10));
			scaley=((sizey<<16)/((offset2&0x2000)?0x20:0x10));

			/* Set the clipping rect to mask off the other portion of the sprite */
			rect.min_x=xpos;
			rect.max_x=xpos+(sizex-1);
			rect.min_y=ypos;
			rect.max_y=ypos+(sizey-1);

			if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
			if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
			if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
			if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

			if((offset2&0x2000)==0)
			{
				if(((offset2&0x0001) && !flipx) || (!(offset2&0x0001) && flipx)) xpos-=sizex;
				if(((offset2&0x0002) && !flipy) || (!(offset2&0x0002) && flipy)) ypos-=sizey;
			}

			if((scalex==(1<<16)) && (scaley==(1<<16)))
			{
				drawgfx(bitmap,Machine->gfx[spr_region],
					sprn,
					(offset6>>4)&0x000f,     /* Selected colour bank */
					flipx,flipy,
					xpos,ypos,
					&rect,TRANSPARENCY_PEN,0xff);
			}
			else if(scalex && scaley)
			{
				drawgfxzoom(bitmap,Machine->gfx[spr_region],
					sprn,
					(offset6>>4)&0x000f,     /* Selected colour bank */
					flipx,flipy,
					xpos,ypos,
					&rect,TRANSPARENCY_PEN,0xff,
					scalex,scaley);
			}
		}
	}
}


static void fill_palette_bank(int virtual,int physical)
{
	int pen,offset;
	static int prev[VIRTUAL_PALETTE_BANKS];

	if (!palette_bank_dirty[physical] && prev[virtual] == physical) return;

	prev[virtual] = physical;

	pen = physical*256;
	offset = ((pen & 0x1800) << 2) | (pen & 0x07ff);

	for (pen = 0;pen < 256;pen++)
	{
		int r,g,b;

		r = namcos2_68k_palette_ram[offset | 0x0000] & 0x00ff;
		g = namcos2_68k_palette_ram[offset | 0x0800] & 0x00ff;
		b = namcos2_68k_palette_ram[offset | 0x1000] & 0x00ff;
		palette_set_color(pen + virtual*256,r,g,b);
		offset++;
	}
}


VIDEO_UPDATE( namcos2_default )
{
	int pri;
	static int show[10] = {1,1,1,1,1,1,1,1,1,1};
	int i;


profiler_mark(PROFILER_USER1);
	/* generate the virtual palette */
	/* sprites */
	for (i = 0;i < 16;i++)
		fill_palette_bank(Machine->drv->gfxdecodeinfo[GFX_OBJ1].color_codes_start/256 + i,i);
	/* tilemaps */
	for (i = 0;i <= 5;i++)
	{
		int virtual = Machine->drv->gfxdecodeinfo[GFX_CHR].color_codes_start/256 + 2*i;
		int physical = 16 + (namcos2_68k_vram_ctrl[0x30/2+i] & 0x07);

		fill_palette_bank(virtual,  physical);
		fill_palette_bank(virtual+1,physical+8);	/* shadows */
	}
	/* roz */
	{
		int virtual = Machine->drv->gfxdecodeinfo[GFX_ROZ].color_codes_start/256;
		int physical = (namcos2_gfx_ctrl & 0x0f00) >> 8;

		fill_palette_bank(virtual,  physical);

		/* it's not clear where the ROZ shadow palette should come from. I'm using */
		/* tilemap #1's shadow palette since it seems to be (close to) correct for valkyrie */
		physical = 16 + (namcos2_68k_vram_ctrl[0x32/2] & 0x07);
		fill_palette_bank(virtual+1,physical+8);
	}

	for (i = 0;i < 32;i++)
		palette_bank_dirty[i] = 0;
profiler_mark(PROFILER_END);


	/* enable ROZ layer only if it has priority > 0 */
	tilemap_set_enable(tilemap_roz,(namcos2_gfx_ctrl & 0x7000) ? 1 : 0);

	/* Scrub the bitmap clean */
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	/* Render the screen */
	for(pri=0;pri<=7;pri++)
	{
		if((namcos2_68k_vram_ctrl[0x20/2]&0x07)==pri && show[0]) tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
		if((namcos2_68k_vram_ctrl[0x22/2]&0x07)==pri && show[1]) tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
		if((namcos2_68k_vram_ctrl[0x24/2]&0x07)==pri && show[2]) tilemap_draw(bitmap,cliprect,tilemap[2],0,0);
		if((namcos2_68k_vram_ctrl[0x26/2]&0x07)==pri && show[3]) tilemap_draw(bitmap,cliprect,tilemap[3],0,0);
		if((namcos2_68k_vram_ctrl[0x28/2]&0x07)==pri && show[4]) tilemap_draw(bitmap,cliprect,tilemap[4],0,0);
		if((namcos2_68k_vram_ctrl[0x2a/2]&0x07)==pri && show[5]) tilemap_draw(bitmap,cliprect,tilemap[5],0,0);

		/* Draw ROZ if enabled */
		if(pri>=1 && ((namcos2_gfx_ctrl & 0x7000) >> 12)==pri && show[6]) draw_layerROZ(bitmap,cliprect);

		/* Sprites */
		draw_sprites_default( bitmap,cliprect,pri );
	}
}


VIDEO_UPDATE( namcos2_finallap )
{
	int pri;
	static int show[10] = {1,1,1,1,1,1,1,1,1,1};
	int i;


profiler_mark(PROFILER_USER1);
	/* generate the virtual palette */
	/* sprites */
	for (i = 0;i < 16;i++)
		fill_palette_bank(Machine->drv->gfxdecodeinfo[GFX_OBJ1].color_codes_start/256 + i,i);
	/* tilemaps */
	for (i = 0;i <= 5;i++)
	{
		int virtual = Machine->drv->gfxdecodeinfo[GFX_CHR].color_codes_start/256 + 2*i;
		int physical = 16 + (namcos2_68k_vram_ctrl[0x30/2+i] & 0x07);

		fill_palette_bank(virtual,  physical);
		fill_palette_bank(virtual+1,physical+8);	/* shadows */
	}
profiler_mark(PROFILER_END);


	/* Scrub the bitmap clean */
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	/* Render the screen */
	for(pri=0;pri<=15;pri++)
	{
		if((namcos2_68k_vram_ctrl[0x20/2]&0x0f)==pri && show[0]) tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
		if((namcos2_68k_vram_ctrl[0x22/2]&0x0f)==pri && show[1]) tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
		if((namcos2_68k_vram_ctrl[0x24/2]&0x0f)==pri && show[2]) tilemap_draw(bitmap,cliprect,tilemap[2],0,0);
		if((namcos2_68k_vram_ctrl[0x26/2]&0x0f)==pri && show[3]) tilemap_draw(bitmap,cliprect,tilemap[3],0,0);
		if((namcos2_68k_vram_ctrl[0x28/2]&0x0f)==pri && show[4]) tilemap_draw(bitmap,cliprect,tilemap[4],0,0);
		if((namcos2_68k_vram_ctrl[0x2a/2]&0x0f)==pri && show[5]) tilemap_draw(bitmap,cliprect,tilemap[5],0,0);
		/* Not sure if priority should be 0x07 or 0x0f */

		/* Sprites */
		draw_sprites_finallap( bitmap,cliprect,pri );
	}


	/********************************************************************/
	/*                                                                  */
	/* Final Lap roadway implementation notes                           */
	/*                                                                  */
	/* namcos2_68k_roadtile_ram                                         */
	/* ------------------------                                         */
	/* Overall tilemap looks to be 64 wide x 512 deep in terms of tiles */
	/* this gives and overall dimension of 512 x 4096 pixels. This is   */
	/* very large and it may be that it uses double buffering which     */
	/* would halve the depth of the buffer.                             */
	/*                                                                  */
	/* Each row of the tilemap is made of 64 words (128 bytes), the     */
	/* first byte most likely holds the colour+attr, the second byte    */
	/* contains the tilenumber, this references a RAM based tile        */
	/* definition within namcos2_68k_roadgfx_ram, tiles are 8x8 8bpp.   */
	/*                                                                  */
	/* namcos2_68k_roadgfx_ram                                          */
	/* -----------------------                                          */
	/* This ram holds the gfx data for the 8x8 tiles in 8bpp format.    */
	/* The data is arranged in 2 x 32 byte blocks with each block       */
	/* arranged in a 4 x 8 byte pattern.                                */
	/*                                                                  */
	/* Bxxby == Byte <xx> bit <y>                                       */
	/*                                                                  */
	/* So Pixel 0,0 = B00b0 B01b0 B02b0 B03b0 B32b0 B33b0 B34b0 B35b0   */
	/*    Pixel 0,1 = B00b1 B01b1 B02b1 B03b1 B32b1 B33b1 B34b1 B35b1   */
	/*                          ..................                      */
	/*    Pixel 0,7 = B00b7 B01b7 B02b7 B03b7 B32b7 B33b7 B34b7 B35b7   */
	/*                                                                  */
	/*    Pixel 1,0 = B04b0 B05b0 B06b0 B07b0 B36b0 B37b0 B38b0 B39b0   */
	/*                                                                  */
	/********************************************************************/

	/* Flat rendering of the road tilemap */
	if(0)
	{
		int loop,linel,data;
		unsigned char *dest_line;
		for(loop=0;loop<28*8;loop++)
			if (loop >= cliprect->min_y && loop <= cliprect->max_y)
			{
				dest_line = bitmap->line[loop];
				for(linel=0;linel<64;linel++)
				{
					data = namcos2_68k_roadtile_ram[(loop*64)+linel];
					*(dest_line++)=(data&0x000f)>>0;
					*(dest_line++)=(data&0x00f0)>>4;
					*(dest_line++)=(data&0x0f00)>>8;
					*(dest_line++)=(data&0xf000)>>12;
				}
			}
	}
}

static int objcode2tile( int code )
{
	return code;
}

VIDEO_START( luckywld )
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[4] = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(get_tile_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);

	namco_obj_init( 0, 0x0, objcode2tile );
	namco_roz_init( 1, REGION_GFX5 );
	namco_road_init(4);
	return 0;
}

VIDEO_UPDATE( luckywld )
{
	int pri;
	int i;

	/* generate the virtual palette */
	/* sprites */
	for (i = 0;i < 16;i++)
	{
		fill_palette_bank(Machine->drv->gfxdecodeinfo[GFX_OBJ1].color_codes_start/256 + i,i);
	}

	/* tilemaps */
	for (i = 0;i <= 5;i++)
	{
		int virtual = Machine->drv->gfxdecodeinfo[GFX_CHR].color_codes_start/256 + 2*i;
		int physical = 16 + (namcos2_68k_vram_ctrl[0x30/2+i] & 0x07);

		fill_palette_bank(virtual,  physical);
		fill_palette_bank(virtual+1,physical+8);	/* shadows */
	}

//	/* roz */
//	{
//		int virtual = Machine->drv->gfxdecodeinfo[GFX_ROZ].color_codes_start/256;
//		int physical = (namcos2_gfx_ctrl & 0x0f00) >> 8;
//
//		fill_palette_bank(virtual,  physical);
//
//		/* it's not clear where the ROZ shadow palette should come from. I'm using */
//		/* tilemap #1's shadow palette since it seems to be (close to) correct for valkyrie */
//		physical = 16 + (namcos2_68k_vram_ctrl[0x32/2] & 0x07);
//		fill_palette_bank(virtual+1,physical+8);
//	}

//	for (i = 0;i < 32;i++)
//	{
//		palette_bank_dirty[i] = 0;
//	}

	/* Scrub the bitmap clean */
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	//	void namco_road_update( void );

	/* Render the screen */
	for(pri=0;pri<8;pri++)
	{
		if( (namcos2_68k_vram_ctrl[0x20/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
		if( (namcos2_68k_vram_ctrl[0x22/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
		if( (namcos2_68k_vram_ctrl[0x24/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[2],0,0);
		if( (namcos2_68k_vram_ctrl[0x26/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[3],0,0);
		if( (namcos2_68k_vram_ctrl[0x28/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[4],0,0);
		if( (namcos2_68k_vram_ctrl[0x2a/2]&0x07)==pri ) tilemap_draw(bitmap,cliprect,tilemap[5],0,0);

		namco_road_draw( bitmap,pri );
		//namco_obj_draw( bitmap, pri );

		namco_roz_draw( bitmap, cliprect, pri );
	}
	for( pri=0; pri<8; pri++ )
	{
		namco_obj_draw( bitmap, pri );
	}
}
