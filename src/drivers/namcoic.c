#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"	/* for game-specific hacks */
#include "namcoic.h"

data32_t *namconb1_spritepos32;

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

static int (*mCode2tile)( int code ); /* sprite banking callback */
static int mGfxC355;	/* gfx bank for sprites */
static int mPalXOR;		/* XOR'd with palette select register; needed for System21 */

static void
draw_spriteC355( int page, struct mame_bitmap *bitmap, const data16_t *pSource, int pri )
{
	int hpos,vpos;
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
	const data16_t *spriteformat16;
	const data16_t *spritetile16;
	int color;

	palette = pSource[6];
	if( namcos2_gametype == NAMCONB2_OUTFOXIES ||
		namcos2_gametype == NAMCONB2_MACH_BREAKERS )
	{
		/* is this correct? it looks like priority may have 4 bits, not 3 */
		if( pri != ((palette>>5)&7) ) return;
	}
	else
	{
		if( pri != ((palette>>4)&0x7) ) return;
	}
	spriteformat16	= &spriteram16[0x4000/2];
	spritetile16	= &spriteram16[0x8000/2];

	linkno		= pSource[0]; /* LINKNO */
	offset		= pSource[1]; /* OFFSET */
	hpos		= pSource[2]; /* HPOS */
	vpos		= pSource[3]; /* VPOS */
	hsize		= pSource[4]; /* HSIZE		max 0x3ff pixels */
	vsize		= pSource[5]; /* VSIZE		max 0x3ff pixels */
	/*
	 * Note that pSource[7] is used in Lucky & Wild.
	 * I suspect it encodes sprite priority with respect to the road layer.
	 */

	if( linkno*4>=0x4000/2 ) return; /* avoid garbage memory read */

	switch( namcos2_gametype )
	{
	case NAMCOS21_CYBERSLED: /* hack */
		hpos -= 256+8+8;
		vpos -= 2+32;
		break;

	case NAMCOS21_AIRCOMBAT: /* hack */
		vpos -= 34;
		hpos -= 2;
		break;

	case NAMCOS21_STARBLADE: /* hack */
		if( page )
		{
			hpos -= 128;
			vpos -= 64;
		}
		break;

	case NAMCONB1_NEBULRAY:
	case NAMCONB1_GUNBULET:
	case NAMCONB1_GSLGR94U:
	case NAMCONB1_SWS96:
	case NAMCONB1_SWS97:
	case NAMCONB1_VSHOOT:
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
		hpos -= nth_word32( namconb1_spritepos32,1 ) + 0x26;
		vpos -= nth_word32( namconb1_spritepos32,0 ) + 0x19;
		/* fallthrough */

	default:
		hpos &= 0x1ff; if( hpos>=288 ) hpos -= 512;
		vpos &= 0x1ff; if( vpos>=224 ) vpos -= 512;
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
	tile_screen_width = (zoomx*16+0x8000)>>16;
	dx = (dx*zoomx+0x8000)>>16;
	if( flipx )
	{
		hpos += dx;
		hpos -= tile_screen_width;
		tile_screen_width = -tile_screen_width;
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
	tile_screen_height = (zoomy*16+0x8000)>>16;
	dy = (dy*zoomy+0x8000)>>16;
	if( flipy )
	{
		vpos += dy;
		vpos -= tile_screen_height;
		tile_screen_height = -tile_screen_height;
	}
	else
	{
		vpos -= dy;
	}

	color = (palette&0xf)^mPalXOR;

	for( row=0; row<num_rows; row++ )
	{
		sy = row*tile_screen_height + vpos;
		for( col=0; col<num_cols; col++ )
		{
			tile = spritetile16[tile_index++];
			if( (tile&0x8000)==0 )
			{
				sx = col*tile_screen_width + hpos;
				drawgfxzoom(bitmap,Machine->gfx[mGfxC355],
					mCode2tile(tile) + offset,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy );
			}
		}
	}
} /* draw_spriteC355 */

void namco_obj_init( int gfxbank, int palXOR, int (*code2tile)( int code ) )
{
	mGfxC355 = gfxbank;
	mPalXOR = palXOR;
	mCode2tile = code2tile;
	spriteram16 = auto_malloc(0x14200);
} /* namcosC355_init */

void namco_obj_draw( struct mame_bitmap *bitmap, int pri )
{
	data16_t *spritelist16;
	data16_t which;
	int i;
	int count;
	int n;
	int offs;

	/* sprite list #1 */
	spritelist16 = &spriteram16[0x2000/2];
	offs = 0;
	for( n=0; n<2; n++ )
	{
		/* count the sprites */
		count = 0;
		for( i=0; i<256; i++ )
		{
			which = spritelist16[i];
			count++;
			if( which&0x100 ) break;
		}

		/* draw the sprites */
		for( i=0; i<count; i++ )
		{
			which = spritelist16[i];
			draw_spriteC355( n, bitmap, &spriteram16[(which&0xff)*8 + offs], pri );
		}

		/* sprite list#2 */
		spritelist16 = &spriteram16[0x14000/2];
		offs = 0x10000/2;
	} /* next n */
} /* namcosC355 */

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

	default: /* Lucky and Wild */
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

/*      attr xx   xy   yx   yy   x0   y0
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

	const int pri_hack[2] = {4,3}; /* HACK! correct for most stages */

	for( which=0; which<2; which++ )
	{
		wOffs = which*8;
		attrs = rozcontrol16[wOffs+1];
		if( attrs&0x8000 ) continue; /* layer disabled flag */
		/* other bits of attrs appear to be related to gfx priority */

		/* HACK! */
		if( namcos2_gametype == NAMCONB2_OUTFOXIES ||
			namcos2_gametype == NAMCONB2_MACH_BREAKERS )
		{ /* Outfoxies */
			roz_pri = pri_hack[which];
			if( attrs == 0x0211 ) roz_pri = 1;
			page = (rozcontrol16[wOffs+3]>>14)&1;
		}
		else
		{
			/* lucky & wild */
			roz_pri = 7-which; /* always draw as frontmost layer */
			page = (attrs&0x0800)?0:1;
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
		incxx =  (INT16)(temp&0x3ff);

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
				tilemap_mark_tile_dirty( mRozTilemap[layer], offset );
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
 * Colors are wrong because the road tilemap isn't hooked up.
 * Final Lap's road RAM attributes aren't populated because of CPU emulation problems.
 *
 * There are several chunks of RAM
 *
 *	Road Tilemap:
 *		0x00000..0x0ffff	64x512 tilemap
 *
 *	Road Tiles:
 *		0x10000..0x1cfff	gfxdata; 64 bytes per tile; 16 pixels wide and 4 pixels tall?
 *
 *	Line Attributes:
 *		0x1fa00..0x1fbdf	priority and xscroll
 *		0x1fbfe				always 0x17 (xadjust?)
 *
 *		0x1fc00..0x1fddf	selects line in source bitmap
 *		0x1fdfe				yscroll
 *
 *		0x1fe00..0x1ffdf	zoomx
 *		0x1fffd				unknown; always 0xffff
 */
static data16_t *mpRoadRAM;
static unsigned char *mpRoadDirty;
static int mbRoadSomethingIsDirty;
static int mRoadGfxBank;
static struct tilemap *mpRoadTilemap;

#define ROAD_COLS			64
#define ROAD_ROWS			512
#define ROAD_TILE_WIDTH		16
#define ROAD_TILE_HEIGHT	4
#define ROAD_TILE_COUNT		(0x6800/0x40)

static struct GfxLayout RoadTileLayout =
{
	ROAD_TILE_WIDTH,
	ROAD_TILE_HEIGHT,
	ROAD_TILE_COUNT,
	8, /* bpp */
	{/* bitplanes */
#ifdef LSB_FIRST
		4*2,4*3,
		4*0,4*1,
		4*6,4*7,
		4*3,4*5
#else
		4*0,4*1,
		4*2,4*3,
		4*3,4*5,
		4*6,4*7
#endif
	},
	{/* x offset */
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
		0x100+0*32,0x100+1*32,0x100+2*32,0x100+3*32,
		0x100+4*32,0x100+5*32,0x100+6*32,0x100+7*32
	},
	{/* y offset */
		0x000,0x001,0x002,0x003,
	},
	64*8 /* 64 bytes per tile */
};

void get_road_info( int tile_index )
{
	data16_t data;
	data = mpRoadRAM[tile_index];
	SET_TILE_INFO( mRoadGfxBank, data>>8, 0/*color*/, 0 )
}

READ16_HANDLER( namco_road16_r )
{
	return mpRoadRAM[offset];
}

WRITE16_HANDLER( namco_road16_w )
{
	COMBINE_DATA( &mpRoadRAM[offset] );
	if( offset<0x8000 )
	{
//		tilemap_mark_tile_dirty( mpRoadTilemap, offset );
	}
	else if( offset<0x8000+ROAD_TILE_COUNT )
	{
		mpRoadDirty[offset-0x8000] = 1;
		mbRoadSomethingIsDirty = 1;
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
//	struct GfxElement *pGfx;

	mRoadGfxBank = gfxbank;
	mpRoadDirty = auto_malloc(ROAD_TILE_COUNT);
	if( mpRoadDirty )
	{
		memset( mpRoadDirty, 0x00, ROAD_TILE_COUNT );
		mbRoadSomethingIsDirty = 0;
		mpRoadRAM = auto_malloc(0x20000);
		if( mpRoadRAM )
		{
//			pGfx = decodegfx( 0x10000+(UINT8 *)mpRoadRAM,&RoadTileLayout );
//			if( pGfx )
//			{
//				pGfx->colortable = Machine->remapped_colortable;
//				pGfx->total_colors = Machine->drv->color_table_len/256;
//				Machine->gfx[gfxbank] = pGfx;
//
//				mpRoadTilemap = tilemap_create(
//					get_road_info,tilemap_scan_rows,
//					TILEMAP_OPAQUE,
//					ROAD_TILE_WIDTH,ROAD_TILE_HEIGHT,
//					ROAD_COLS,ROAD_ROWS);
//				if( mpRoadTilemap )
//				{
					return 0;
//				}
//			}
		}
	}
	return -1;
} /* namco_road_init */

void
namco_road_draw( struct mame_bitmap *bitmap, int pri )
{
	UINT16 *pDest;
	int screenx,sourcey,zoomx,yscroll;
	int i,j;

	yscroll = mpRoadRAM[0x1fdfe/2];
	for( i=0; i<bitmap->height; i++ )
	{
		pDest = bitmap->line[i];

		screenx	= mpRoadRAM[0x1fa00/2+i+15];
		sourcey	= mpRoadRAM[0x1fc00/2+i+15]+yscroll;
		zoomx	= mpRoadRAM[0x1fe00/2+i+15];

		//road layer: 64x512 tiles
		//1024 pixels wide; each tile must by 16 pixels wide
		//if 4 pixels tall, 4*512 = 2048 pixels vertical

		zoomx &= 0x3ff; /* sanity check; I don't think the upper bits are ever used */

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
					if( sourcey&0x80 )
					/* just fake two alternating colors for now, so we can see road movement */
					{
						pDest[screenx] = Machine->pens[0x3];
					}
					else
					{
						pDest[screenx] = Machine->pens[0xf];
					}
				}
				screenx++;
			}
		}
	}
} /* namco_road_draw */
/**************************************************************************************************************/
