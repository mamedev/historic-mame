/* video hardware for Namco System II */

/* TBA:
 * get rid of "virtual palette" optimization (it's not needed)
 * use common vh_update with gfx hardware type flags set in game-specific vh_start
 * tile/sprite priority should be orthogonal
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "namcoic.h"

#define SPRITE_PAL	1
#define TILEMAP_PAL	2
#define ROZ_PAL		4

data16_t *namcos2_sprite_ram;
size_t namcos2_68k_vram_size;
data16_t *namcos2_68k_palette_ram;
size_t namcos2_68k_palette_size;
static data16_t namcos2_68k_roz_ctrl[0x8];
size_t namcos2_68k_roz_ram_size;
data16_t *namcos2_68k_roz_ram;

static struct tilemap *tilemap[6];
static struct tilemap *tilemap_roz;
static data16_t namcos2_68k_vram_ctrl[0x20];
static data16_t namcos2_gfx_ctrl;
static int palette_bank_dirty[32];

/**************************************************************************/

INLINE void get_tile_info(int tile_index,data16_t *vram,int color)
{
	int tile;
	tile = vram[tile_index];

	switch( namcos2_gametype )
	{
	case NAMCOS2_FINAL_LAP_2:
		tile &= 0x3fff;
		tile_info.mask_data = memory_region(REGION_GFX4)+(0x08*tile);
		break;

	case NAMCOS2_FINAL_LAP_3:
		tile &= 0x3fff;
		tile_info.mask_data = memory_region(REGION_GFX4)+(0x08*tile);
		break;

	default:
		/* The tile mask DOESNT use the mangled tile number */
		tile_info.mask_data = memory_region(REGION_GFX4)+(0x08*tile);
		/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
		tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
		break;
	}
	SET_TILE_INFO(2,tile,color,0)
}

static void get_tile_info0(int tile_index) { get_tile_info(tile_index,videoram16+0x0000,0); }
static void get_tile_info1(int tile_index) { get_tile_info(tile_index,videoram16+0x1000,2); }
static void get_tile_info2(int tile_index) { get_tile_info(tile_index,videoram16+0x2000,4); }
static void get_tile_info3(int tile_index) { get_tile_info(tile_index,videoram16+0x3000,6); }
static void get_tile_info4(int tile_index) { get_tile_info(tile_index,videoram16+0x4008,8); }
static void get_tile_info5(int tile_index) { get_tile_info(tile_index,videoram16+0x4408,10); }

static int
CreateTilemaps( void )
{
	int i;
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[4] = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(get_tile_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	for( i=0; i<=5; i++ )
	{
		if( !tilemap[i] ) return -1;
	}
	return 0;
}

static void
DrawTilemaps( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	if( (namcos2_68k_vram_ctrl[0x20/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
	if( (namcos2_68k_vram_ctrl[0x22/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
	if( (namcos2_68k_vram_ctrl[0x24/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[2],0,0);
	if( (namcos2_68k_vram_ctrl[0x26/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[3],0,0);
	if( (namcos2_68k_vram_ctrl[0x28/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[4],0,0);
	if( (namcos2_68k_vram_ctrl[0x2a/2]&0x7)==pri ) tilemap_draw(bitmap,cliprect,tilemap[5],0,0);
}

/**************************************************************************/

static void get_tile_info_roz(int tile_index)
{
	int tile;
	tile = namcos2_68k_roz_ram[tile_index];
	SET_TILE_INFO(3,tile,0,0)
}

static void
DrawROZ(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
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

	startx <<= 4;
	starty <<= 4;
	startx += xoffset * incxx + yoffset * incyx;
	starty += xoffset * incxy + yoffset * incyy;

	tilemap_draw_roz(bitmap,cliprect,tilemap_roz,startx << 8,starty << 8,
			incxx << 8,incxy << 8,incyx << 8,incyy << 8,
			1,	/* copy with wraparound */
			0,0);
}

/**************************************************************************/

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

READ16_HANDLER( namcos2_gfx_ctrl_r )
{
	return namcos2_gfx_ctrl;
}

WRITE16_HANDLER( namcos2_gfx_ctrl_w )
{
	COMBINE_DATA(&namcos2_gfx_ctrl);
}


WRITE16_HANDLER( namcos2_sprite_ram_w )
{
	COMBINE_DATA(&namcos2_sprite_ram[offset]);
}

READ16_HANDLER( namcos2_sprite_ram_r )
{
	return namcos2_sprite_ram[offset];
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
	{
		tilemap_mark_tile_dirty(tilemap_roz,offset);
	}
}

/**************************************************************************/

static void
DrawSpritesDefault( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
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
			spr_region=(offset2&0x2000)?1:0;

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

			if(scalex && scaley)
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
} /* DrawSpritesDefault */

static void
DrawSpritesFinalLap( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	int sprn,flipy,flipx;
	INT16 ypos,xpos;
	UINT32 sw,sh;
	UINT32 scalex,scaley;
	int offset = (namcos2_gfx_ctrl & 0x000f) * (128*4);
	data16_t y_position,tile_number,x_position,attrs;
    int loop, spr_region;
	struct rectangle rect;
	int bHalfSize = 0;

	for(loop=0;loop < 128*16;loop++)
	{
		/****************************************
		* Sprite data is 8 byte packed format   *
		*                                       *
		* word0                                 *
		*   Sprite Y position           0x01ff  *
		*   Sprite Size 16/32?          0x0200  *
		*   Sprite Size Y               0xfc00  *
		*                                       *
		* word1                                 *
		*   Sprite Number               0x1fff  *
		*   Sprite Bank           		0x2000  *
		*   Sprite flip X               0x4000  *
		*   Sprite flip Y               0x8000  *
		*                                       *
		* word2                                 *
		*   Sprite X position           0xffff  *
		*                                       *
		* word3                                 *
		*   Sprite priority             0x000f  *
		*   Sprite colour index         0x00f0  *
		*   Sprite Size X               0xfc00  *
		*                                       *
		****************************************/

		y_position	= namcos2_sprite_ram[offset+(loop*4)+0];
		tile_number	= namcos2_sprite_ram[offset+(loop*4)+1];
		x_position	= namcos2_sprite_ram[offset+(loop*4)+2];
		attrs		= namcos2_sprite_ram[offset+(loop*4)+3];

        if( (attrs & 0x000f)!=pri) continue;
		if( tile_number == 0 ) continue;
		sh = (y_position>>10)+1; /* sprite height in pixels */
		sw = attrs>>10; /* sprite width in pixels */
		scalex= (sw<<16)/0x20;
		scaley= (sh<<16)/0x20;
		if( bHalfSize )
		{
			sh>>=1;
			sw>>=1;
		}
		sprn = (tile_number & 0x1fff)>>2;
        spr_region=(tile_number&0x2000)?1:0;

		ypos=(0x1ff-(y_position&0x01ff))-0x50+0x02;
		xpos=(x_position&0x03ff)-0x50+0x07;

		/* convert flipx and flipy to bits that will interact with LSB of tilenumber */
		flipy=(tile_number&0x8000)>>14;
		flipx=(tile_number&0x4000)>>14;

		/* set the clipping rect to mask off the other portion of the sprite */
		rect.min_x=xpos;
		rect.max_x=xpos+(sw-1);
		rect.min_y=ypos;
		rect.max_y=ypos+(sh-1);

		if( bHalfSize )
		{
			if( tile_number^flipx ) xpos -= sw;
			if( tile_number^flipy ) ypos -= sh;
		}

		drawgfxzoom(bitmap,Machine->gfx[spr_region],
				sprn++,
				(attrs>>4)&0x000f,     /* Selected colour bank */
				flipx,flipy,
				xpos,ypos,
				&rect,TRANSPARENCY_PEN,0xff,
				scalex,scaley );
	}
} /* DrawSpritesFinalLap */

static void
DrawSpritesMetalHawk( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	/**
	 * word#0
	 *	xxxxxx---------- ysize
	 *	------x--------- sprite tile size
	 *	-------xxxxxxxxx screeny
	 *
	 * word#1
	 *	--x------------- bank
	 *	----xxxxxxxxxxxx tile
	 *
	 * word#2
	 *	xxxxxx---------- xsize
	 *	------xxxxxxxxxx screenx
	 *
	 * word#6
	 *	--------------xx unknown
	 *	-------------x-- flipy
	 *	------------x--- unknown
	 *
	 * word#7
	 *	------------xxxx unknown
	 *	--------xxxx---- color
	 *	x--------------- unknown
	 */
	const data16_t *pSource = namcos2_sprite_ram;
	struct rectangle rect;
	int loop;
	for(loop=0;loop < 128;loop++)
	{
		int ypos = pSource[0];
		int tile = pSource[1];
		int xpos = pSource[3];

		int flags = pSource[6];
		int attrs = pSource[7];
		int sizey=((ypos>>10)&0x3f)+1;
		int sizex=(xpos>>10)&0x3f;
		int sprn=(tile>>2)&0x7ff;
		if( tile&0x2000 ) sprn&=0x3ff; else sprn|=0x400;
		if((sizey-1) && sizex && /*(attrs&7)*/2==pri )
		{
			int bSmallSprite = ((ypos&0x1000)==0);
			int color = (attrs>>4)&0xf;
			int sx = (xpos&0x03ff)-0x50+0x07;
			int sy = (0x1ff-(ypos&0x01ff))-0x50+0x02;
			int flipx = flags&1;
			int flipy = flags&4;
			int scalex = (sizex<<16)/(bSmallSprite?0x10:0x20);
			int scaley = (sizey<<16)/(bSmallSprite?0x10:0x20);

			/* Set the clipping rect to mask off the other portion of the sprite */
			rect.min_x=sx;
			rect.max_x=sx+(sizex-1);
			rect.min_y=sy;
			rect.max_y=sy+(sizey-1);

			if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
			if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
			if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
			if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

			if( bSmallSprite )
			{
				if(((tile&0x0001) && !flipx) || (!(tile&0x0001) && flipx)) sx-=sizex;
				if(((tile&0x0002) && !flipy) || (!(tile&0x0002) && flipy)) sy-=sizey;
			}
			drawgfxzoom(
				bitmap,Machine->gfx[0],
				sprn, color,
				flipx,flipy,
				sx,sy,
				&rect,
				TRANSPARENCY_PEN,0xff,
				scalex, scaley );
		}
		pSource += 8;
	}
} /* DrawSpritesMetalHawk */

static void
FillPaletteBank(int virtual,int physical)
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

static void
FillPalettes( int flags )
{
	int virtual, physical, i;

	if( flags&SPRITE_PAL )
	{
		for( i=0; i<16; i++ )
		{
			FillPaletteBank( Machine->drv->gfxdecodeinfo[0].color_codes_start/256 + i,i);
		}
	}
	if( flags&TILEMAP_PAL )
	{
		for( i=0; i<6; i++ )
		{
			virtual = Machine->drv->gfxdecodeinfo[2].color_codes_start/256 + 2*i;
			physical = 16 + (namcos2_68k_vram_ctrl[0x30/2+i] & 0x07);
			FillPaletteBank(virtual,  physical);
			FillPaletteBank(virtual+1,physical+8);	/* shadows */
		}
	}
	if( flags&ROZ_PAL )
	{
		virtual = Machine->drv->gfxdecodeinfo[3].color_codes_start/256;
		physical = (namcos2_gfx_ctrl & 0x0f00) >> 8;
		FillPaletteBank(virtual,  physical);
		/* it's not clear where the ROZ shadow palette should come from. I'm using */
		/* tilemap #1's shadow palette since it seems to be (close to) correct for valkyrie */
		physical = 16 + (namcos2_68k_vram_ctrl[0x32/2] & 0x07);
		FillPaletteBank(virtual+1,physical+8);
	}
	for (i = 0;i < 32;i++)
	{
		palette_bank_dirty[i] = 0;
	}
} /* FillPalettes */

static void
DrawSpriteInit( void )
{
	int i;
	/* set table for sprite color == 0x0f */
	for(i = 0;i <= 253;i++)
	{
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	}
	gfx_drawmode_table[254] = DRAWMODE_SHADOW;
	gfx_drawmode_table[255] = DRAWMODE_NONE;
	for (i = 0;i < 14*256;i++)
	{
		palette_shadow_table[Machine->pens[i+16*256]] = Machine->pens[(i+16*256)|256];
	}
	for (i = 0;i < 32;i++)
	{
		palette_bank_dirty[i] = 1;
	}
}

/**************************************************************************/

VIDEO_START( namcos2 )
{
	if( CreateTilemaps()==0 )
	{
		tilemap_roz = tilemap_create(get_tile_info_roz,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,256,256);
		if( tilemap_roz )
		{
			tilemap_set_transparent_pen(tilemap_roz,0xff);
			DrawSpriteInit();
			return 0;
		}
	}
	return -1;
}

VIDEO_UPDATE( namcos2_default )
{
	int pri;

	FillPalettes(SPRITE_PAL|TILEMAP_PAL|ROZ_PAL);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	/* enable ROZ layer only if it has priority > 0 */
	tilemap_set_enable(tilemap_roz,(namcos2_gfx_ctrl & 0x7000) ? 1 : 0);

	for(pri=0;pri<=7;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		if(pri>=1 && ((namcos2_gfx_ctrl & 0x7000) >> 12)==pri) DrawROZ(bitmap,cliprect);
		DrawSpritesDefault( bitmap,cliprect,pri );
	}
}

VIDEO_START( finallap )
{
	if( CreateTilemaps()==0 )
	{
		DrawSpriteInit();
		namco_road_init(3);
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( finallap )
{
	int pri;

	FillPalettes(SPRITE_PAL|TILEMAP_PAL);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	namco_road_update();

	for(pri=0;pri<=15;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_road_draw( bitmap,pri );
		DrawSpritesFinalLap( bitmap,cliprect,pri );
	}
}

static int objcode2tile( int code )
{
	return code;
}

VIDEO_START( luckywld )
{
	if( CreateTilemaps()==0 )
	{
		namco_obj_init( 0, 0x0, objcode2tile );
		if( namcos2_gametype==NAMCOS2_LUCKY_AND_WILD )
		{
			namco_roz_init( 1, REGION_GFX5 );
		}
		if( namcos2_gametype!=NAMCOS2_STEEL_GUNNER_2 )
		{
			namco_road_init(3);
		}
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( luckywld )
{
	int pri;

	FillPalettes(SPRITE_PAL|TILEMAP_PAL);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	namco_road_update();

	for(pri=0;pri<8;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_road_draw( bitmap,pri );
		if( namcos2_gametype==NAMCOS2_LUCKY_AND_WILD )
		{
			namco_roz_draw( bitmap, cliprect, pri );
		}
		namco_obj_draw( bitmap, pri );
	}
}

VIDEO_START( sgunner )
{
	if( CreateTilemaps()==0 )
	{
		namco_obj_init( 0, 0x0, objcode2tile );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( sgunner )
{
	int pri;

	FillPalettes(SPRITE_PAL|TILEMAP_PAL);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<8;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_obj_draw( bitmap, pri );
	}
}

VIDEO_START( metlhawk )
{
	if( CreateTilemaps()==0 )
	{
		namco_roz_init( 1, REGION_GFX5 );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( metlhawk )
{
	int pri;

	FillPalettes(SPRITE_PAL|TILEMAP_PAL);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<8;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_roz_draw( bitmap, cliprect, pri );
		DrawSpritesMetalHawk( bitmap,cliprect,pri );
	}
}
