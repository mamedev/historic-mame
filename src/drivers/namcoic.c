#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"	/* for game-specific hacks */
#include "namcoic.h"

data16_t mSpritePos[4];

WRITE16_HANDLER( namco_spritepos16_w )
{
	COMBINE_DATA( &mSpritePos[offset] );
}
READ16_HANDLER( namco_spritepos16_r )
{
	return mSpritePos[offset];
}

WRITE32_HANDLER( namco_spritepos32_w )
{
	data32_t v;
	offset *= 2;
	v = (mSpritePos[offset]<<16)|mSpritePos[offset+1];
	COMBINE_DATA( &v );
	mSpritePos[offset+0] = v>>16;
	mSpritePos[offset+1] = v&0xffff;
}
READ32_HANDLER( namco_spritepos32_r )
{
	offset *= 2;
	return (mSpritePos[offset]<<16)|mSpritePos[offset+1];
}

INLINE data8_t
nth_byte16( const data16_t *pSource, int which )
{
	data16_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xff;
	}
	else
	{
		return data>>8;
	}
} /* nth_byte16 */

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE data16_t
nth_word32( const data32_t *pSource, int which )
{
	data32_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xffff;
	}
	else
	{
		return data>>16;
	}
} /* nth_word32 */

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
INLINE data8_t
nth_byte32( const data32_t *pSource, int which )
{
		data32_t data = pSource[which/4];
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

/**************************************************************************************************************/

static int (*mpCode2tile)( int code ); /* sprite banking callback */
static int mGfxC355;	/* gfx bank for sprites */
static int mPalXOR;		/* XOR'd with palette select register; needed for System21 */

static void
draw_spriteC355( int page, struct mame_bitmap *bitmap, const data16_t *pSource, int pri )
{
	unsigned screen_height_remaining, screen_width_remaining;
	unsigned source_height_remaining, source_width_remaining;
	INT32 hpos,vpos;
	data16_t hsize,vsize;
	data16_t palette;
	data16_t linkno;
	data16_t offset;
	data16_t format;
	int tile_index;
	int num_cols,num_rows;
	int dx,dy;
	int row,col;
	int sx,sy,tile;
	int flipx,flipy;
	UINT32 zoomx, zoomy;
	int tile_screen_width;
	int tile_screen_height;
	const data16_t *spriteformat16 = &spriteram16[0x4000/2];
	const data16_t *spritetile16 = &spriteram16[0x8000/2];
	int color;
	int bOpaque;

	/**
	 * xxxx------------ unused?
	 * ----xxxx-------- affects whole sprite, including transparent parts
	 * --------xxxx---- sprite-tilemap priority for opaque parts of sprite
	 * ------------xxxx palette select
	 */
	palette = pSource[6];
	switch( namcos2_gametype )
	{
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
	case NAMCOS2_SUZUKA_8_HOURS_2:
	case NAMCOS2_SUZUKA_8_HOURS:
	case NAMCOS2_LUCKY_AND_WILD:
		if( pri != ((palette>>5)&7) ) return;
		break;

	default:
		if( pri != ((palette>>4)&0x7) ) return;
		break;
	}

	bOpaque = (palette>>8);

	linkno		= pSource[0]; /* LINKNO */
	offset		= pSource[1]; /* OFFSET */
	hpos		= pSource[2]; /* HPOS		0x000..0x7ff (signed) */
	vpos		= pSource[3]; /* VPOS		0x000..0x7ff (signed) */
	hsize		= pSource[4]; /* HSIZE		max 0x3ff pixels */
	vsize		= pSource[5]; /* VSIZE		max 0x3ff pixels */
	/* pSource[6] contains priority/palette */
	/* pSource[7] is used in Lucky & Wild, probably for sprite-road priority */

	if( linkno*4>=0x4000/2 ) return; /* avoid garbage memory read */

	switch( namcos2_gametype )
	{
	case NAMCOS21_SOLVALOU: /* hack */
		hpos -= 0x80;
		vpos -= 0x40;
		break;

	case NAMCOS21_CYBERSLED: /* hack */
		hpos -= 0x110;
		vpos -= 2+32;
		break;

	case NAMCOS21_AIRCOMBAT: /* hack */
		vpos -= 0x22;
		hpos -= 0x02;
		break;

	case NAMCOS21_STARBLADE: /* hack */
		if( page )
		{
			hpos -= 0x80;
			vpos -= 0x20;
		}
		break;

	case NAMCONB1_NEBULRAY:
	case NAMCONB1_GUNBULET:
	case NAMCONB1_GSLGR94U:
	case NAMCONB1_SWS95:
	case NAMCONB1_SWS96:
	case NAMCONB1_SWS97:
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
	case NAMCONB1_VSHOOT:
	case NAMCOS2_SUZUKA_8_HOURS_2:
	case NAMCOS2_SUZUKA_8_HOURS:
	case NAMCOS2_LUCKY_AND_WILD:
	case NAMCOS2_STEEL_GUNNER_2:
	default:
		{
			int dh = mSpritePos[1];
			int dv = mSpritePos[0];

			dh &= 0x1ff; if( dh&0x100 ) dh |= ~0x1ff;
			dv &= 0x1ff; if( dv&0x100 ) dv |= ~0x1ff;
			vpos&=0x7ff; if( vpos&0x400 ) vpos |= ~0x7ff;
			hpos&=0x7ff; if( hpos&0x400 ) hpos |= ~0x7ff;
			hpos += -0x26 - dh;
			vpos += -0x19 - dv;
		}
		break;
	}
	tile_index		= spriteformat16[linkno*4+0];
	format			= spriteformat16[linkno*4+1];
	dx				= spriteformat16[linkno*4+2];
	dy				= spriteformat16[linkno*4+3];
	num_cols		= (format>>4)&0xf;
	num_rows		= (format)&0xf;

	if( num_cols == 0 ) num_cols = 0x10;
	flipx = (hsize&0x8000)?1:0;
	hsize &= 0x1ff;
	if( hsize == 0 ) return;

	zoomx = (hsize<<16)/(num_cols*16);
	dx = (dx*zoomx+0x8000)>>16;
	if( flipx )
	{
		hpos += dx;
	}
	else
	{
		hpos -= dx;
	}

	if( num_rows == 0 ) num_rows = 0x10;
	flipy = (vsize&0x8000)?1:0;
	vsize&=0x1ff;
	if( vsize == 0 ) return;

	zoomy = (vsize<<16)/(num_rows*16);
	dy = (dy*zoomy+0x8000)>>16;
	if( flipy )
	{
		vpos += dy;
	}
	else
	{
		vpos -= dy;
	}

	color = (palette&0xf)^mPalXOR;

	if( 0 && bOpaque )
	{
		int pen = get_black_pen();
		sy = vpos;
		if( sy<0 ) sy = 0;
		while( sy<vpos+vsize && sy<bitmap->height )
		{
			UINT16 *pDest = (UINT16 *)bitmap->line[sy];
			sx = hpos;
			if( sx<0 ) sx = 0;
			while( sx<hpos+hsize && sx<bitmap->width )
			{
				pDest[sx] = pen;
				sx++;
			}
			sy++;
		}
	}

	source_height_remaining = num_rows*16;
	screen_height_remaining = vsize;
	sy = vpos;
	for( row=0; row<num_rows; row++ )
	{
		tile_screen_height = 16*screen_height_remaining/source_height_remaining;
		zoomy = (screen_height_remaining<<16)/source_height_remaining;
		if( flipy )
		{
			sy -= tile_screen_height;
		}
		source_width_remaining = num_cols*16;
		screen_width_remaining = hsize;
		sx = hpos;
		for( col=0; col<num_cols; col++ )
		{
			tile_screen_width = 16*screen_width_remaining/source_width_remaining;
			zoomx = (screen_width_remaining<<16)/source_width_remaining;
			if( flipx )
			{
				sx -= tile_screen_width;
			}
			tile = spritetile16[tile_index++];
			if( (tile&0x8000)==0 )
			{
				drawgfxzoom(bitmap,Machine->gfx[mGfxC355],
					mpCode2tile(tile) + offset,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy );
			}
			if( !flipx )
			{
				sx += tile_screen_width;
			}
			screen_width_remaining -= tile_screen_width;
			source_width_remaining -= 16;
		} /* next col */
		if( !flipy )
		{
			sy += tile_screen_height;
		}
		screen_height_remaining -= tile_screen_height;
		source_height_remaining -= 16;
	} /* next row */
} /* draw_spriteC355 */

void namco_obj_init( int gfxbank, int palXOR, int (*code2tile)( int code ) )
{
	mGfxC355 = gfxbank;
	mPalXOR = palXOR;
	mpCode2tile = code2tile;
	spriteram16 = auto_malloc(0x14200);
	memset( mSpritePos,0x00,sizeof(mSpritePos) );
} /* namcosC355_init */

static void
DrawObjectList(
		struct mame_bitmap *bitmap,
		int pri,
		const data16_t *pSpriteList16,
		const data16_t *pSpriteTable,
		int n )
{
	data16_t which;
	int i;
	int count = 0;
	/* count the sprites */
	for( i=0; i<256; i++ )
	{
		which = pSpriteList16[i];
		count++;
		if( which&0x100 ) break;
	}
	/* draw the sprites */
	for( i=0; i<count; i++ )
	{
		which = pSpriteList16[i];
		draw_spriteC355( n, bitmap, &pSpriteTable[(which&0xff)*8], pri );
	}
} /* DrawObjectList */

void
namco_obj_draw( struct mame_bitmap *bitmap, int pri )
{
	DrawObjectList( bitmap,pri,&spriteram16[0x02000/2], &spriteram16[0x00000/2],0 );
	DrawObjectList( bitmap,pri,&spriteram16[0x14000/2], &spriteram16[0x10000/2],1 );
} /* namco_obj_draw */

WRITE16_HANDLER( namco_obj16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
} /* namco_obj16_w */

READ16_HANDLER( namco_obj16_r )
{
	return spriteram16[offset];
} /* namco_obj16_r */

WRITE32_HANDLER( namco_obj32_w )
{
	data32_t v;
	offset *= 2;
	v = (spriteram16[offset]<<16)|spriteram16[offset+1];
	COMBINE_DATA( &v );
	spriteram16[offset] = v>>16;
	spriteram16[offset+1] = v&0xffff;
} /* namco_obj32_w */

READ32_HANDLER( namco_obj32_r )
{
	offset *= 2;
	return (spriteram16[offset]<<16)|spriteram16[offset+1];
} /* namco_obj32_r */

/**************************************************************************************************************/

/* ROZ abstraction (preliminary)
 *
 * Used in:
 *	Namco NB2 - The Outfoxies, Mach Breakers
 *	Namco System 2 - Metal Hawk, Lucky and Wild
 */
static data16_t *rozbank16; /* TBA: use callback */
static data16_t *rozvideoram16;
static data16_t *rozcontrol16;
static int mRozGfxBank;
static int mRozMaskRegion;
static struct tilemap *mRozTilemap[2];
static int mRozColor[2];	/* palette select */
static int mRozPage[2];		/* base addr for tilemap */

static void
roz_get_info( int tile_index,int which )
{
	data16_t tile = rozvideoram16[128*128*mRozPage[which]+tile_index];
	int bank;
	int mangle;

	switch( namcos2_gametype )
	{
	case NAMCONB2_MACH_BREAKERS:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile;
		break;

	case NAMCONB2_OUTFOXIES:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile&~(0x50);
		/* the pixmap index is mangled, the transparency bitmask index is not */
		if( tile&0x10 ) mangle |= 0x40;
		if( tile&0x40 ) mangle |= 0x10;
		break;

	case NAMCOS2_LUCKY_AND_WILD:
		mangle	= tile&0x01ff;
		tile &= 0x3fff;
		switch( tile>>9 )
		{
		case 0x00: mangle |= 0x1c00; break;
		case 0x01: mangle |= 0x0800; break;
		case 0x02: mangle |= 0x0000; break;

		case 0x08: mangle |= 0x1e00; break;
		case 0x09: mangle |= 0x0a00; break;
		case 0x0a: mangle |= 0x0200; break;

		case 0x10: mangle |= 0x2000; break;
		case 0x11: mangle |= 0x0c00; break;
		case 0x12: mangle |= 0x0400; break;

		case 0x18: mangle |= 0x2200; break;
		case 0x19: mangle |= 0x0e00; break;
		case 0x1a: mangle |= 0x0600; break;
		}
		break;

	case NAMCOS2_METAL_HAWK:
	default:
		mangle = tile&0x01ff;
		if( tile&0x1000 ) mangle |= 0x0200;
		if( tile&0x0200 ) mangle |= 0x0400;
		if( tile&0x0400 ) mangle |= 0x0800;
		if( tile&0x0800 ) mangle |= 0x1000;
		break;
	}
	SET_TILE_INFO( mRozGfxBank,mangle,mRozColor[which],0 );
	tile_info.mask_data = 32*tile + (UINT8 *)memory_region( mRozMaskRegion );
} /* roz_get_info */

static void roz_get_info0( int tile_index )
{
	roz_get_info( tile_index,0 );
}
static void roz_get_info1( int tile_index )
{
	roz_get_info( tile_index,1 );
}

int
namco_roz_init( int gfxbank, int maskregion )
{
	/* allocate both ROZ layers */
	int i;
	static void (*roz_info[2])(int tile_index) =
		{ roz_get_info0, roz_get_info1 };

	mRozGfxBank = gfxbank;
	mRozMaskRegion = maskregion;

	rozbank16 = auto_malloc(0x10);
	rozvideoram16 = auto_malloc(0x20000);
	rozcontrol16 = auto_malloc(0x20);

	if( rozbank16 && rozvideoram16 && rozcontrol16 )
	{
		for( i=0; i<2; i++ )
		{
			mRozTilemap[i] = tilemap_create( roz_info[i], tilemap_scan_rows,
				TILEMAP_BITMASK,16,16,128,128 );

			if( mRozTilemap[i] == NULL ) return 1; /* error */
		}
		return 0;
	}
	return -1;
} /* namco_roz_init */

/**
 * ROZ control attributes:
 *
 * unk  attr   xx   xy   yx   yy   x0   y0
 *
 * Lucky & Wild:
 * 0000 080e 2100 0000 0000 0100 38e0 fde0	0:badge
 * 1000 084e 00fe 0000 0000 00fe f9f6 fd96	0:zooming car
 * 0000 080e 1100 0000 0000 0000 26d0 0450	0:"LUCKY & WILD"
 * 0000 03bf 0100 0000 0000 0100 fde0 7fe0	1:talking heads
 * 1000 080a 0100 0000 0000 0100 fde0 ffe0	0:player select
 *
 * Outfoxies:
 * 0x0211 (tv)
 * 0x0271 (stage)
 * 0x0252 (stage)
 */
void namco_roz_draw(
	struct mame_bitmap *bitmap,
	const struct rectangle *cliprect,
	int pri )
{
	int wOffs;
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	const int xoffset = 38,yoffset = 0;
	data16_t temp, attrs;
	int which;
	int bDirty;
	int color,page;
	int roz_pri;

	for( which=0; which<2; which++ )
	{
		wOffs = which*8;
		attrs = rozcontrol16[wOffs+1];
		if( attrs&0x8000 ) continue; /* layer disabled flag */
		/* other bits of attrs appear to be related to priority */

		switch( namcos2_gametype )
		{
		case NAMCONB2_OUTFOXIES:
		case NAMCONB2_MACH_BREAKERS:
			roz_pri = 4-which; /* hack! correct for most Outfoxies stages */
			if( attrs == 0x0211 ) roz_pri = 1; /* hack */
			page = (rozcontrol16[wOffs+3]>>14)&1;
			break;

		case NAMCOS2_LUCKY_AND_WILD:
			roz_pri = 5-which;
			page = (attrs&0x0800)?0:1;
			break;

		case NAMCOS2_METAL_HAWK:
		default:
			page = (attrs&0x8000)?0:1;
			page = (rozcontrol16[wOffs+3]>>14)&1;
			roz_pri = /*3+*/which;
			break;
		}

		if( roz_pri!=pri ) continue;

		bDirty = 0;
		color = rozcontrol16[wOffs+1]&0xf;
		if( mRozColor[which] !=color )
		{
			mRozColor[which] = color;
			bDirty = 1;
		}
		if( mRozPage[which] != page )
		{
			mRozPage[which] = page;
			bDirty = 1;
		}
		if( bDirty )
		{
			tilemap_mark_all_tiles_dirty( mRozTilemap[which] );
		}

		temp = rozcontrol16[wOffs+2];
		//incxx =  (INT16)(temp&0x3ff);
		if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
		incxx = (INT16)temp;

		/* only bits 0x8fff are meaningful as ROZ attributes.
		 * 0x8000 is a sign bit.
		 * 0x4000 appears to select the source tilemap (from ROZ videoram)
		 * 0x3000 are unused.
		 */
		temp = rozcontrol16[wOffs+3];
		if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
		incxy =  (INT16)temp;

		temp = rozcontrol16[wOffs+4];
		if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
		incyx =  (INT16)temp;

		incyy =  (INT16)rozcontrol16[wOffs+5];
		startx = (INT16)rozcontrol16[wOffs+6];
		starty = (INT16)rozcontrol16[wOffs+7];

		startx <<= 4;
		starty <<= 4;
		startx += xoffset * incxx + yoffset * incyx;
		starty += xoffset * incxy + yoffset * incyy;

		tilemap_draw_roz(bitmap,cliprect,mRozTilemap[which],
			startx << 8,
			starty << 8,
			incxx << 8,
			incxy << 8,
			incyx << 8,
			incyy << 8,
			1,	// copy with wraparound
			0,0);
	}
} /* namco_roz_draw */

READ16_HANDLER( namco_rozcontrol16_r )
{
	return rozcontrol16[offset];
}

WRITE16_HANDLER( namco_rozcontrol16_w )
{
	COMBINE_DATA( &rozcontrol16[offset] );
}

READ16_HANDLER( namco_rozbank16_r )
{
	return rozbank16[offset];
}

WRITE16_HANDLER( namco_rozbank16_w )
{
	data16_t old_data;

	old_data = rozbank16[offset];
	COMBINE_DATA( &rozbank16[offset] );
	if( rozbank16[offset]!=old_data )
	{
		tilemap_mark_all_tiles_dirty( mRozTilemap[0] );
		tilemap_mark_all_tiles_dirty( mRozTilemap[1] );
	}
}

static void writerozvideo( int offset, data16_t data )
{
	int i;
	int layer;

	if( rozvideoram16[offset]!=data )
	{
		rozvideoram16[offset] = data;
		layer = offset/(128*128);
		offset &= (128*128-1); /* mask high bits */
		for( i=0; i<2; i++ )
		{
			if( mRozPage[i]==layer )
			{
				tilemap_mark_tile_dirty( mRozTilemap[i], offset );
			}
		}
	}
}

READ16_HANDLER( namco_rozvideoram16_r )
{
	return rozvideoram16[offset];
}

WRITE16_HANDLER( namco_rozvideoram16_w )
{
	data16_t v;

	v = rozvideoram16[offset];
	COMBINE_DATA( &v );
	writerozvideo( offset, v );
}

READ32_HANDLER( namco_rozcontrol32_r )
{
	offset *= 2;
	return (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
}

WRITE32_HANDLER( namco_rozcontrol32_w )
{
	data32_t v;
	offset *=2;
	v = (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
	COMBINE_DATA(&v);
	rozcontrol16[offset] = v>>16;
	rozcontrol16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozbank32_r )
{
	offset *= 2;
	return (rozbank16[offset]<<16)|rozbank16[offset+1];
}

WRITE32_HANDLER( namco_rozbank32_w )
{
	data32_t v;
	offset *=2;
	v = (rozbank16[offset]<<16)|rozbank16[offset+1];
	COMBINE_DATA(&v);
	rozbank16[offset] = v>>16;
	rozbank16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozvideoram32_r )
{
	offset *= 2;
	return (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
}

WRITE32_HANDLER( namco_rozvideoram32_w )
{
	data32_t v;
	offset *= 2;
	v = (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
	COMBINE_DATA( &v );
	writerozvideo(offset,v>>16);
	writerozvideo(offset+1,v&0xffff);
}

/**************************************************************************************************************/
/* Preliminary!  The road circuitry is identical for all the driving games.
 *
 * There are several chunks of RAM
 *
 *	Road Tilemap:
 *		0x00000..0x0ffff	64x512 tilemap
 *
 *	Road Tiles:
 *		0x10000..0x1cfff	gfxdata; 64 bytes per tile
 *
 *	Line Attributes:
 *		0x1fa00..0x1fbdf	priority and xscroll
 *		0x1fbfe				always 0x17?
 *
 *		0x1fc00..0x1fddf	selects line in source bitmap
 *		0x1fdfe				yscroll
 *
 *		0x1fe00..0x1ffdf	zoomx
 *		0x1fffd				always 0xffff?
 */
static data16_t *mpRoadRAM;
static unsigned char *mpRoadDirty;
static int mbRoadSomethingIsDirty;
static int mRoadGfxBank;
static struct tilemap *mpRoadTilemap;

#define ROAD_COLS			64
#define ROAD_ROWS			512
#define ROAD_TILE_WIDTH		16
#define ROAD_TILE_HEIGHT	16
#define ROAD_TILEMAP_WIDTH	(ROAD_TILE_WIDTH*ROAD_COLS)
#define ROAD_TILEMAP_HEIGHT (ROAD_TILE_HEIGHT*ROAD_ROWS)
#define ROAD_TILE_COUNT		0x200 /* actually 0x6800/0x40 = 0x1a0 */

static struct GfxLayout RoadTileLayout =
{
	ROAD_TILE_WIDTH,
	ROAD_TILE_HEIGHT,
	ROAD_TILE_COUNT,
	2,
	{
#ifdef LSB_FIRST
		0,8
#else
		8,0
#endif
	},
	{/* x offset */
		0,1,2,3,4,5,6,7,
		16+0,16+1,16+2,16+3,16+4,16+5,16+6,16+7,
	},
	{/* y offset */
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
		8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

void get_road_info( int tile_index )
{
	data16_t data = mpRoadRAM[tile_index];
	/* the lower bits are associated with the tile */
	int tile = (data>>1)&0x1ff; /* guess! */
	int color = 0;
	SET_TILE_INFO( mRoadGfxBank, tile, color, 0 )
} /* get_road_info */

READ16_HANDLER( namco_road16_r )
{
	return mpRoadRAM[offset];
}

WRITE16_HANDLER( namco_road16_w )
{
	COMBINE_DATA( &mpRoadRAM[offset] );
	if( offset<0x8000 )
	{
		tilemap_mark_tile_dirty( mpRoadTilemap, offset );
	}
	else
	{
		offset -= 0x8000;
		if( offset<ROAD_TILE_COUNT )
		{
			mpRoadDirty[offset] = 1;
			mbRoadSomethingIsDirty = 1;
		}
	}
}

void
namco_road_update( void )
{
	int i;
	if( mbRoadSomethingIsDirty )
	{
		for( i=0; i<ROAD_TILE_COUNT; i++ )
		{
			if( mpRoadDirty[i] )
			{
				decodechar(
					Machine->gfx[mRoadGfxBank],
					i,
					0x10000+(UINT8 *)mpRoadRAM,
					&RoadTileLayout );
				mpRoadDirty[i] = 0;
			}
		}
		tilemap_mark_all_tiles_dirty( mpRoadTilemap );
		mbRoadSomethingIsDirty = 0;
	}
} /* namco_road_update */

int
namco_road_init( int gfxbank )
{
	mRoadGfxBank = gfxbank;
	mpRoadDirty = auto_malloc(ROAD_TILE_COUNT);
	if( mpRoadDirty )
	{
		memset( mpRoadDirty,0x00,ROAD_TILE_COUNT );
		mbRoadSomethingIsDirty = 0;
		mpRoadRAM = auto_malloc(0x20000);
		if( mpRoadRAM )
		{
			struct GfxElement *pGfx = decodegfx(
				0x10000+(UINT8 *)mpRoadRAM,
				&RoadTileLayout );
			if( pGfx )
			{
				pGfx->colortable = Machine->remapped_colortable;
				pGfx->total_colors = 8192/4;
				Machine->gfx[gfxbank] = pGfx;
				mpRoadTilemap = tilemap_create(
					get_road_info,tilemap_scan_rows,
					TILEMAP_OPAQUE,
					ROAD_TILE_WIDTH,ROAD_TILE_HEIGHT,
					ROAD_COLS,ROAD_ROWS);

				if( mpRoadTilemap )
				{
					return 0;
				}
			}
		}
	}
	return -1;
} /* namco_road_init */

void
namco_road_draw( struct mame_bitmap *bitmap, int pri )
{
	struct mame_bitmap *pSourceBitmap = tilemap_get_pixmap(mpRoadTilemap);
	unsigned yscroll = mpRoadRAM[0x1fdfe/2];
	int i,j;

	for( i=0; i<bitmap->height; i++ )
	{
		UINT16 *pDest = bitmap->line[i];
		int screenx	= mpRoadRAM[0x1fa00/2+i+15];
		unsigned sourcey = mpRoadRAM[0x1fc00/2+i+15]+yscroll;
		const UINT16 *pSourceGfx = pSourceBitmap->line[sourcey&(ROAD_TILEMAP_HEIGHT-1)];
		unsigned zoomx	= mpRoadRAM[0x1fe00/2+i+15]&0x3ff;
		unsigned dsourcex = zoomx?((ROAD_TILEMAP_WIDTH<<16)/zoomx):0;
		unsigned sourcex = 0;

        if( pri == ((screenx&0xe000)>>13) )
		{
			/* draw this scanline */
			screenx &= 0x0fff; /* mask off priority bits */
			if( screenx&0x0800 )
			{
				/* sign extend */
				screenx -= 0x1000;
			}
			screenx -= 64; /* adjust the horizontal placement; this isn't quite right */
			for( j=0; j<zoomx; j++ )
			{
				if( screenx>=0 && screenx<bitmap->width ) /* clip */
				{
					pDest[screenx] = pSourceGfx[sourcex>>16];
				}
				sourcex += dsourcex;
				screenx++;
			}
		}
	}
} /* namco_road_draw */
