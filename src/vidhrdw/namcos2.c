/* video hardware for Namco System II */

//todo: screen blank, posirq

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "namcoic.h"

data16_t *namcos2_sprite_ram;
data16_t *namcos2_68k_palette_ram;
size_t namcos2_68k_palette_size;
size_t namcos2_68k_roz_ram_size;
data16_t *namcos2_68k_roz_ram;

static data16_t namcos2_68k_roz_ctrl[0x8];
static struct tilemap *tilemap_roz;

static void
TilemapCB( data16_t code, int *tile, int *mask )
{
	*mask = code;

	switch( namcos2_gametype )
	{
	case NAMCOS2_FINAL_LAP_2:
	case NAMCOS2_FINAL_LAP_3:
		*tile = (code&0x07ff)|((code&0x4000)>>3)|((code&0x3800)<<1);
		break;

	default:
		/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
		*tile = (code&0x07ff)|((code&0xc000)>>3)|((code&0x3800)<<2);
		break;
	}
}

/**
 * namcos2_gfx_ctrl selects a bank of 128 sprites within spriteram
 *
 * namcos2_gfx_ctrl also supplies palette and priority information that is applied to the output of the
 *                  Namco System 2 ROZ chip
 *
 * -xxx ---- ---- ---- roz priority
 * ---- xxxx ---- ---- roz palette
 * ---- ---- ---- xxxx sprite bank
 */
static data16_t namcos2_gfx_ctrl;

READ16_HANDLER( namcos2_gfx_ctrl_r )
{
	return namcos2_gfx_ctrl;
}

WRITE16_HANDLER( namcos2_gfx_ctrl_w )
{
	COMBINE_DATA(&namcos2_gfx_ctrl);
}

static void get_tile_info_roz(int tile_index)
{
	int tile = namcos2_68k_roz_ram[tile_index];
	SET_TILE_INFO(3,tile,0/*color*/,0)
}

struct RozParam
{
	UINT32 /*left, top,*/ size;
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	int color;//,priority;
	int wrap;
};

static void
DrawRozHelper(
	struct mame_bitmap *bitmap,
	struct tilemap *tilemap,
	const struct rectangle *clip,
	const struct RozParam *rozInfo )
{
	tilemap_set_palette_offset( tilemap, rozInfo->color );

	if( bitmap->depth == 15 || bitmap->depth == 16 )
	{
		UINT32 size_mask = rozInfo->size-1;
		struct mame_bitmap *srcbitmap = tilemap_get_pixmap( tilemap );
		struct mame_bitmap *transparency_bitmap = tilemap_get_transparency_bitmap( tilemap );
		UINT32 startx = rozInfo->startx + clip->min_x * rozInfo->incxx + clip->min_y * rozInfo->incyx;
		UINT32 starty = rozInfo->starty + clip->min_x * rozInfo->incxy + clip->min_y * rozInfo->incyy;
		int sx = clip->min_x;
		int sy = clip->min_y;
		while( sy <= clip->max_y )
		{
			int x = sx;
			UINT32 cx = startx;
			UINT32 cy = starty;
			UINT16 *dest = ((UINT16 *)bitmap->line[sy]) + sx;
			while( x <= clip->max_x )
			{
				UINT32 xpos = (cx>>16);
				UINT32 ypos = (cy>>16);
				if( rozInfo->wrap )
				{
					xpos &= size_mask;
					ypos &= size_mask;
				}
				else if( xpos>rozInfo->size || ypos>=rozInfo->size )
				{
					goto L_SkipPixel;
				}

				if( ((UINT8 *)transparency_bitmap->line[ypos])[xpos]&TILE_FLAG_FG_OPAQUE )
				{
					*dest = ((UINT16 *)srcbitmap->line[ypos])[xpos]+rozInfo->color;
				}
L_SkipPixel:
				cx += rozInfo->incxx;
				cy += rozInfo->incxy;
				x++;
				dest++;
			} /* next x */
			startx += rozInfo->incyx;
			starty += rozInfo->incyy;
			sy++;
		} /* next y */
	}
	else
	{
		tilemap_draw_roz(
			bitmap,
			clip,
			tilemap,
			rozInfo->startx, rozInfo->starty,
			rozInfo->incxx, rozInfo->incxy,
			rozInfo->incyx, rozInfo->incyy,
			rozInfo->wrap,0,0); // wrap, flags, pri
	}
} /* DrawRozHelper */

static void
DrawROZ(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	const int xoffset = 38,yoffset = 0;
	struct RozParam rozParam;

	rozParam.color = namcos2_gfx_ctrl & 0x0f00;
	rozParam.incxx  = (INT16)namcos2_68k_roz_ctrl[0];
	rozParam.incxy  = (INT16)namcos2_68k_roz_ctrl[1];
	rozParam.incyx  = (INT16)namcos2_68k_roz_ctrl[2];
	rozParam.incyy  = (INT16)namcos2_68k_roz_ctrl[3];
	rozParam.startx = (INT16)namcos2_68k_roz_ctrl[4];
	rozParam.starty = (INT16)namcos2_68k_roz_ctrl[5];
	rozParam.size = 2048;
	rozParam.wrap = 1;

	switch( namcos2_68k_roz_ctrl[7] )
	{
	case 0x4400: /* (2048x2048) */
		break;

	case 0x4488: /* attract mode */
		rozParam.wrap = 0;
		break;

	case 0x44cc: /* stage1 demo */
		rozParam.wrap = 0;
		break;

	case 0x44ee: /* (256x256) used in Dragon Saber */
		rozParam.wrap = 0;
		rozParam.size = 256;
		break;
	}

	rozParam.startx <<= 4;
	rozParam.starty <<= 4;
	rozParam.startx += xoffset * rozParam.incxx + yoffset * rozParam.incyx;
	rozParam.starty += xoffset * rozParam.incxy + yoffset * rozParam.incyy;

	rozParam.startx<<=8;
	rozParam.starty<<=8;
	rozParam.incxx<<=8;
	rozParam.incxy<<=8;
	rozParam.incyx<<=8;
	rozParam.incyy<<=8;

	DrawRozHelper( bitmap, tilemap_roz, cliprect, &rozParam );
}

/**
 *	ROZ - Rotate & Zoom memory function handlers
 *
 *	0 - inc xx
 *	2 - inc xy
 *	4 - inc yx
 *	6 - inc yy
 *	8 - start x
 *	A	start y
 *	C - ??
 *	E - ??
 */
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
	COMBINE_DATA(&namcos2_68k_palette_ram[offset]);
}

static void
UpdatePalette( void )
{
	int bank;
	for( bank=0; bank<0x20; bank++ )
	{
		int pen = bank*256;
		int offset = ((pen & 0x1800) << 2) | (pen & 0x07ff);
		int i;
		for( i=0; i<256; i++ )
		{
			int r = namcos2_68k_palette_ram[offset | 0x0000] & 0x00ff;
			int g = namcos2_68k_palette_ram[offset | 0x0800] & 0x00ff;
			int b = namcos2_68k_palette_ram[offset | 0x1000] & 0x00ff;
			palette_set_color(pen++,r,g,b);
			offset++;
		}
	}
} /* UpdatePalette */

/**************************************************************************/

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
	for( i = 0; i<16*256; i++ )
	{
		palette_shadow_table[i] = i+0x2000;
	}
}

WRITE16_HANDLER( namcos2_sprite_ram_w )
{
	COMBINE_DATA(&namcos2_sprite_ram[offset]);
}

READ16_HANDLER( namcos2_sprite_ram_r )
{
	return namcos2_sprite_ram[offset];
}

static void
DrawSpritesDefault( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri, int pri_mask )
{
	int pri_shift = (pri_mask==0xf)?0:1;
	struct GfxElement gfx;
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;

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

		if((sizey-1) && sizex && ((offset6<<pri_shift)&0xf)==pri)
		{
			int color = (offset6>>4)&0x000f;

			sprn=(offset2>>2)&0x7ff;
			spr_region=(offset2&0x2000)?1:0;

			ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
			xpos=(offset4&0x03ff)-0x50+0x07;

			flipy=offset2&0x8000;
			flipx=offset2&0x4000;

			scalex = (sizex<<16)/((offset0&0x0200)?0x20:0x10);
			scaley = (sizey<<16)/((offset0&0x0200)?0x20:0x10);

			if(scalex && scaley)
			{
				gfx = *Machine->gfx[spr_region];

				if( (offset0&0x0200)==0 )
				{
					gfx.width = 16;
					gfx.height = 16;
					if( offset2&0x0001 ) gfx.gfxdata += 16;
					if( offset2&0x0002 ) gfx.gfxdata += 16*gfx.line_modulo;
				}

				drawgfxzoom(bitmap,&gfx,
					sprn,
					color,
					flipx,flipy,
					xpos,ypos,
					cliprect,
					(color==0x0f ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN),0xff,
					scalex,scaley);
			}
		}
	}
} /* DrawSpritesDefault */

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
	 * word#3				2->3 by N
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
		if((sizey-1) && sizex && ((attrs>>1)&7)==pri )
		{
			int bSmallSprite =
				(sprn>=0x208 && sprn<=0x20F)||
				(sprn>=0x3BC && sprn<=0x3BF)||
				(sprn>=0x688 && sprn<=0x68B)||
				(sprn>=0x6D8 && sprn<=0x6D9)||
				(sprn>=0x6EA && sprn<=0x6EB); // very stupid...

			int color = (attrs>>4)&0xf;
			int sx = (xpos&0x03ff)-0x50+0x07;
			int sy = (0x1ff-(ypos&0x01ff))-0x50+0x02;
			int flipx = flags&2;
			int flipy = flags&4;
			int scalex = (sizex<<16)/(bSmallSprite?0x10:0x20);
			int scaley = (sizey<<16)/(bSmallSprite?0x10:0x20);

			// 90 degrees use a turned character
			if( (flags&0x01) ) {
				sprn |= 0x800;
			}

			// little zoom fix...
			if( !bSmallSprite ) {
				if( sizex < 0x20 ) {
					sx -= (0x20-sizex)/0x8;
				}
				if( sizey < 0x20 ) {
					sy += (0x20-sizey)/0xC;
				}
			}

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
				sizex = 16;
				sizey = 16;
				scalex = 1<<16;
				scaley = 1<<16;

				sx -= (tile&1)?16:0;
				sy -= (tile&2)?16:0;

				rect.min_x=sx;
				rect.max_x=sx+(sizex-1);
				rect.min_y=sy;
				rect.max_y=sy+(sizey-1);
				rect.min_x += (tile&1)?16:0;
				rect.max_x += (tile&1)?16:0;
				rect.min_y += (tile&2)?16:0;
				rect.max_y += (tile&2)?16:0;
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

/**************************************************************************/

static void
DrawCrossshair( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int x1port, y1port, x2port, y2port;
	int beamx, beamy;

	switch( namcos2_gametype )
	{
	case NAMCOS2_GOLLY_GHOST:
		x1port = 0;
		y1port = 1;
		x2port = 2;
		y2port = 3;
		break;
	case NAMCOS2_LUCKY_AND_WILD:
		x1port = 4;
		y1port = 2;
		x2port = 3;
		y2port = 1;
		break;
	case NAMCOS2_STEEL_GUNNER_2:
		x1port = 4;
		x2port = 5;
		y1port = 6;
		y2port = 7;
		break;
	default:
		return;
	}

	beamx = readinputport(2+x1port)*bitmap->width/256;
	beamy = readinputport(2+y1port)*bitmap->height/256;
	draw_crosshair( bitmap, beamx, beamy, cliprect );

	beamx = readinputport(2+x2port)*bitmap->width/256;
	beamy = readinputport(2+y2port)*bitmap->height/256;
	draw_crosshair( bitmap, beamx, beamy, cliprect );
}

/**************************************************************************/

VIDEO_START( namcos2 )
{
	if( namco_tilemap_init(2,memory_region(REGION_GFX4),TilemapCB)==0 )
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

	UpdatePalette();

	fillbitmap(bitmap,get_black_pen(),cliprect);

	/* enable ROZ layer only if it has priority > 0 */
	tilemap_set_enable(tilemap_roz,(namcos2_gfx_ctrl & 0x7000) ? 1 : 0);

	for( pri=0; pri<16; pri++ )
	{
		namco_tilemap_draw( bitmap, cliprect, pri );
		if( pri>=1 && ((namcos2_gfx_ctrl & 0x7000) >> 12)*2==pri )
		{
			DrawROZ(bitmap,cliprect);
		}
		DrawSpritesDefault( bitmap, cliprect, pri, 0x7 );
	}
	DrawCrossshair( bitmap,cliprect );
} /* namcos2_default */

/**************************************************************************/

VIDEO_START( finallap )
{
	if( namco_tilemap_init(2,memory_region(REGION_GFX4),TilemapCB)==0 )
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

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for( pri=0; pri<16; pri++ )
	{
		namco_tilemap_draw( bitmap, cliprect, pri );
		namco_road_draw( bitmap,cliprect,pri );
		DrawSpritesDefault( bitmap,cliprect,pri,0x000f );
	}
}

/**************************************************************************/

VIDEO_START( luckywld )
{
	if( namco_tilemap_init(2,memory_region(REGION_GFX4),TilemapCB)==0 )
	{
		namco_obj_init( 0, 0x0, NULL );
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
} /* luckywld */

VIDEO_UPDATE( luckywld )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for( pri=0; pri<16; pri++ )
	{
		namco_tilemap_draw( bitmap, cliprect, pri );
		namco_road_draw( bitmap,cliprect,pri );
		if( namcos2_gametype==NAMCOS2_LUCKY_AND_WILD )
		{
			namco_roz_draw( bitmap, cliprect, pri );
		}
		namco_obj_draw( bitmap, cliprect, pri );
	}
	DrawCrossshair( bitmap,cliprect );
} /* luckywld */

/**************************************************************************/

VIDEO_START( sgunner )
{
	if( namco_tilemap_init(2,memory_region(REGION_GFX4),TilemapCB)==0 )
	{
		namco_obj_init( 0, 0x0, NULL );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( sgunner )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for( pri=0; pri<16; pri++ )
	{
		namco_tilemap_draw( bitmap, cliprect, pri );
		namco_obj_draw( bitmap, cliprect, pri );
	}
	DrawCrossshair( bitmap,cliprect );
}

/**************************************************************************/

VIDEO_START( metlhawk )
{
	if( namco_tilemap_init(2,memory_region(REGION_GFX4),TilemapCB)==0 )
	{
		namco_roz_init( 1, REGION_GFX5 );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( metlhawk )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for( pri=0; pri<16; pri++ )
	{
		namco_tilemap_draw( bitmap, cliprect, pri );
		namco_roz_draw( bitmap, cliprect, pri );
		DrawSpritesMetalHawk( bitmap,cliprect,pri );
	}
}
