/* video hardware for Namco System22 */

#include <math.h>
#include <assert.h>
#include "namcos22.h"
#include "namcos3d.h"

#define NAMCOS22_SCREEN_HEIGHT	(NAMCOS22_NUM_ROWS*16)
#define NAMCOS22_SCREEN_WIDTH	(NAMCOS22_NUM_COLS*16)

#define MAX_CAMERA 128
static struct Matrix
{
	double M[4][4];
} *mpMatrix;

static data8_t *mpTextureTileMap;
static data8_t *mpTextureTileData;

static int mPtRomSize;
static const data8_t *mpPolyH;
static const data8_t *mpPolyM;
static const data8_t *mpPolyL;
static INT32 GetPolyData( INT32 addr );

static data8_t nthbyte( const data32_t *pSource, int offs );
static data16_t nthword( const data32_t *pSource, int offs );
static void tilemap_get_info( int tile_index );
static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect );
static void draw_polygons( struct mame_bitmap *bitmap, int theWindow );
static const INT32 *LoadMatrix( const INT32 *pSource, double M[4][4] );

#define CGRAM_SIZE 0x1e000
#define NUM_CG_CHARS ((CGRAM_SIZE*8)/(64*16))

/* text layer uses a set of 16x16x8bpp tiles defined in RAM */
static struct GfxLayout cg_layout =
{
	16,16,
	NUM_CG_CHARS,
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
	{ 4*6,4*7, 4*4,4*5, 4*2,4*3, 4*0,4*1,
	  4*14,4*15, 4*12,4*13, 4*10,4*11, 4*8,4*9 },
#else
	{ 4*0,4*1,4*2,4*3,4*4,4*5,4*6,4*7,
	  4*8,4*9,4*10,4*11,4*12,4*13,4*14,4*15 },
#endif
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7,64*8,64*9,64*10,64*11,64*12,64*13,64*14,64*15 },
	64*16
}; /* cg_layout */

data32_t *namcos22_cgram;
data32_t *namcos22_textram;
data32_t *namcos22_polygonram;

static int cgsomethingisdirty;
static unsigned char *cgdirty;
static unsigned char *dirtypal;
static struct tilemap *tilemap;

#if 0
#define BMP_H (0x4000)
#define BMP_W (0x1000)
static void
DumpTexelBMP( void )
{
	const unsigned char header[] =
	{
		0x42,0x4d,
		0x36,0x04,0x02,0x00, // file size in bytes
		0x00,0x00,0x00,0x00, // reserved
		0x36,0x04,0x00,0x00, // offset frolm file start to bmp data

		0x28,0x00,0x00,0x00, // sizeof(BITMAPINFOHEADER)
		(BMP_W&0xff),((BMP_W>>8)&0xff),((BMP_W>>16)&0xff),((BMP_W>>24)&0xff), // image width in pixels
		(BMP_H&0xff),((BMP_H>>8)&0xff),((BMP_H>>16)&0xff),((BMP_H>>24)&0xff), // image height in pixels
		0x01,0x00, // biPlanes
		0x08,0x00, // bits per pixel
		0x00,0x00,0x00,0x00, // compression type
		0x00,0x00,0x00,0x00, // sizeof(image data); zero is valid if no compression
		0xc4,0x0e,0x00,0x00, // horiz pixels per meter
		0xc4,0x0e,0x00,0x00, // vert pixels per meter
		0x00,0x00,0x00,0x00, // number of colors (0 means to calculate using bpp)
		0x00,0x00,0x00,0x00  // number of "important" colors (zero means "all")
	};
	FILE *f;
	f = fopen( "texel.bmp", "wb" );
	if( f )
	{
		int x,y,i;
		for( i=0; i<sizeof(header); i++ )
		{
			fputc( header[i], f );
		}

		for( i=0; i<256; i+=4 )
		{
			fputc( i,f ); // blue
			fputc( i,f ); // green
			fputc( i,f ); // red
			fputc( 0x00, f );
		}
		for( i=0; i<256; i+=4 )
		{
			fputc( i,f ); // blue
			fputc( 0,f ); // green
			fputc( 0,f ); // red
			fputc( 0x00, f );
		}
		for( i=0; i<256; i+=4 )
		{
			fputc( 0,f ); // blue
			fputc( i,f ); // green
			fputc( 0,f ); // red
			fputc( 0x00, f );
		}
		for( i=0; i<256; i+=4 )
		{
			fputc( 0,f ); // blue
			fputc( 0,f ); // green
			fputc( i,f ); // red
			fputc( 0x00, f );
		}

		for( y=0; y<BMP_H; y++ )
		{
			for( x=0; x<BMP_W; x++ )
			{
				fputc( texel(x,BMP_H-1-y), f );
			}
		}
	}
	fclose(f);
} /* DumpTexelBMP */
#endif

static data8_t
nthbyte( const data32_t *pSource, int offs )
{
	pSource += offs/4;
	return (pSource[0]<<((offs&3)*8))>>24;
}

static data16_t
nthword( const data32_t *pSource, int offs )
{
	pSource += offs/2;
	return (pSource[0]<<((offs&1)*16))>>16;
}

static void tilemap_get_info( int tile_index )
{
	data16_t data = nthword( namcos22_textram,tile_index );
	SET_TILE_INFO( NAMCOS22_ALPHA_GFX,data&0x3ff,(data>>12),0 );
}

READ32_HANDLER( namcos22_cgram_r )
{
	return namcos22_cgram[offset];
}

WRITE32_HANDLER( namcos22_cgram_w )
{
	COMBINE_DATA( &namcos22_cgram[offset] );
	cgdirty[offset/32] = 1;
	cgsomethingisdirty = 1;
}

READ32_HANDLER( namcos22_paletteram_r )
{
	return paletteram32[offset];
}

WRITE32_HANDLER( namcos22_paletteram_w )
{
	COMBINE_DATA( &paletteram32[offset] );
	dirtypal[offset&(0x7fff/4)] = 1;
}

READ32_HANDLER( namcos22_textram_r )
{
	return namcos22_textram[offset];
}

WRITE32_HANDLER( namcos22_textram_w )
{
	COMBINE_DATA( &namcos22_textram[offset] );
	tilemap_mark_tile_dirty( tilemap, offset*2 );
	tilemap_mark_tile_dirty( tilemap, offset*2+1 );
//	logerror( "%08x: text_w(%08x)\n", activecpu_get_pc(), offset );
}

VIDEO_START( namcos22s )
{
	struct GfxElement *pGfx;

	mpMatrix = auto_malloc(sizeof(struct Matrix)*MAX_CAMERA);
	if( !mpMatrix ) return -1; /* error */

	mpTextureTileMap = memory_region(REGION_GFX3);
	mpTextureTileData = memory_region(REGION_GFX2);

	if( namcos3d_Init(NAMCOS22_SCREEN_WIDTH,NAMCOS22_SCREEN_HEIGHT) == 0 )
	{
		mPtRomSize = memory_region_length(REGION_GFX4)/3;
		mpPolyL = memory_region(REGION_GFX4);
		mpPolyM = mpPolyL + mPtRomSize;
		mpPolyH = mpPolyM + mPtRomSize;
		//DumpTexelBMP();
		if(0)
		{
			int i;
			for( i=0; i<mPtRomSize; i++ )
			{
				if( (i&0xf)==0 ) logerror( "\n %08x:",i );
				logerror( " %06x", (unsigned)(GetPolyData(i)&0xffffff) );
			}
		}
		pGfx = decodegfx( (UINT8 *)namcos22_cgram,&cg_layout );
		if( pGfx )
		{
			Machine->gfx[NAMCOS22_ALPHA_GFX] = pGfx;
			pGfx->colortable = Machine->remapped_colortable+0x7f00;
			pGfx->total_colors = 16;
			tilemap = tilemap_create( tilemap_get_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64 );
			if( tilemap )
			{
				tilemap_set_transparent_pen( tilemap, 0xf );
				dirtypal = auto_malloc(0x8000/4);
				if( dirtypal )
				{
					cgdirty = auto_malloc( 0x400 );
					if( cgdirty )
					{
						return 0; /* no error */
					}
				}
			}
		}
	}
	return -1; /* error */
}

static void
draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/*
		0x980000:	00060000 00020053
							 ^^^^			num sprites

		0x980010:	03000200 028004ff
							 ^^^^			delta xpos

		0x980018:	032a0509
					^^^^					delta ypos

		0x980200:	028004ff 032a0509		delta xpos, delta ypos

		0x980400:	?
		0x980600:	?

		0x980800:	0000 0001 0002 0003 ...

		0x984000:	010f 007b	xpos, ypos
		0x984004:	0020 0020	size x, size y
		0x984008:	00ff 0311	00ff, chr x;chr y;flip x;flip y
		0x98400c:	0001 0000	sprite code, ????

		0x9a0000:	00c0 0000	C381 Z
		0x9a0004:	007d 0000	palette, C381 ZC
	*/
	int i;
	int deltax, deltay;
	int color,flipx,flipy;
	data32_t xypos, size, attrs, code;
	const data32_t *pSource, *pPal;
	int numcols, numrows, row, col;
	int xpos, ypos, tile;
	int num_sprites;
	int zoomx, zoomy;
	int sizex, sizey;

	deltax = spriteram32[5]>>16;
	deltay = spriteram32[6]>>16;
	num_sprites = (spriteram32[1]>>16)&0xf;

	color = 0;
	flipx = 0;
	flipy = 0;
	pSource = &spriteram32[0x4000/4]+num_sprites*4;
	pPal = &spriteram32[0x20000/4]+num_sprites*2;
	for( i=0; i<=num_sprites; i++ )
	{
		color = pPal[1]>>16;

		xypos = pSource[0];
		xpos = (xypos>>16)-deltax;
		ypos = (xypos&0xffff)-deltay;

		size = pSource[1];

		sizex = size>>16;
		sizey = size&0xffff;
		zoomx = (1<<16)*sizex/0x20;
		zoomy = (1<<16)*sizey/0x20;

		attrs = pSource[2];
		flipy = attrs&0x8;
		numrows = attrs&0x7;
		if( numrows==0 ) numrows = 8;
		if( flipy )
		{
			ypos += sizey*(numrows-1);
			sizey = -sizey;
		}

		attrs>>=4;

		flipx = attrs&0x8;
		numcols = attrs&0x7;
		if( numcols==0 ) numcols = 8;
		if( flipx )
		{
			xpos += sizex*(numcols-1);
			sizex = -sizex;
		}

		code = pSource[3];
		tile = code>>16;

		for( row=0; row<numrows; row++ )
		{
			for( col=0; col<numcols; col++ )
			{
				drawgfxzoom( bitmap, Machine->gfx[0],
					tile,
					color,
					flipx, flipy,
					xpos+col*sizex, ypos+row*sizey,
					cliprect,
					TRANSPARENCY_PEN, 255,
					zoomx, zoomy );
				tile++;
			}
		}
		pSource -= 4;
		pPal -= 2;
	}
} /* draw_sprites */

unsigned texel( unsigned x, unsigned y )
{
	unsigned attr,offs,tile,temp;

	x &= 0xfff;  /* 256 columns, 16 pixels per tile */
	y &= 0xffff; /* (0x200000/0x200)*0x10 = 0x10000 */
	offs = (x/0x10)+(y/0x10)*0x100;
	tile = 256*mpTextureTileMap[offs*2+1]+mpTextureTileMap[offs*2];
	attr = mpTextureTileMap[0x200000+offs/2];
	x &= 0xf;
	y &= 0xf;
	if( offs&1 ) attr &= 0xf; else attr >>= 4;
	if( attr&4 ) x = 15-x;
	if( attr&2 ) y = 15-y;
	if( attr&8 ){ temp = x; x = y; y = temp; }
	return mpTextureTileData[tile*16*16+y*16+x];
} /* texel */

static void
update_palette( void )
{
	int i,j;

	for( i=0; i<0x8000/4; i++ )
	{
		if( dirtypal[i] )
		{
			for( j=0; j<4; j++ )
			{
				int which = i*4+j;
				int r = nthbyte(paletteram32,which+0x00000);
				int g = nthbyte(paletteram32,which+0x08000);
				int b = nthbyte(paletteram32,which+0x10000);
				palette_set_color( which,r,g,b );
			}
			dirtypal[i] = 0;
		}
	}
} /* update_palette */

static void
draw_text_layer( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	unsigned i;
	data32_t data;

	if( cgsomethingisdirty )
	{
		for( i=0; i<64*64; i+=2 )
		{
			data = namcos22_textram[i/2];
			if( cgdirty[(data>>16)&0x3ff] )
			{
				tilemap_mark_tile_dirty( tilemap,i );
			}
			if( cgdirty[data&0x3ff] )
			{
				tilemap_mark_tile_dirty( tilemap,i+1 );
			}
		}
		for( i=0; i<NUM_CG_CHARS; i++ )
		{
			if( cgdirty[i] )
			{
				decodechar( Machine->gfx[NAMCOS22_ALPHA_GFX],i,(UINT8 *)namcos22_cgram,&cg_layout );
				cgdirty[i] = 0;
			}
		}
		cgsomethingisdirty = 0;
	}
	tilemap_draw( bitmap, cliprect, tilemap, 0, 0 );
} /* draw_text_layer */

VIDEO_UPDATE( namcos22s )
{
	int i;

	update_palette();
	fillbitmap( bitmap, get_black_pen(), cliprect );
	draw_polygons( bitmap,0 );
	draw_sprites( bitmap, cliprect );
	for( i=1; i<8; i++ )
	{
		draw_polygons( bitmap,i );
	}
	draw_text_layer( bitmap, cliprect );
}

VIDEO_UPDATE( namcos22 )
{
	int i;

	update_palette();
	fillbitmap( bitmap, get_black_pen(), cliprect );
	for( i=0; i<8; i++ )
	{
		draw_polygons( bitmap,i );
	}
	draw_text_layer( bitmap, cliprect );
}

/*********************************************************************************************/

static void
ApplyRotation( const INT32 *pSource, double M[4][4] )
{
	struct RotParam param;
	param.thx_sin = (INT16)(pSource[0])/(double)0x7fff;
	param.thx_cos = (INT16)(pSource[1])/(double)0x7fff;
	param.thy_sin = (INT16)(pSource[2])/(double)0x7fff;
	param.thy_cos = (INT16)(pSource[3])/(double)0x7fff;
	param.thz_sin = (INT16)(pSource[4])/(double)0x7fff;
	param.thz_cos = (INT16)(pSource[5])/(double)0x7fff;
	param.rolt = pSource[6];
	matrix_NamcoRot( M, &param );
} /* ApplyRotation */

/* +0x00 0	always 0xf?
 * +0x04 1	sin(x) world-view matrix
 * +0x08 2	cos(x)
 * +0x0c 3	sin(y)
 * +0x10 4	cos(y)
 * +0x14 5	sin(z)
 * +0x18 6	cos(z)
 * +0x1c 7	ROLT? always 0x0002?
 * +0x20 8	light power
 * +0x24 9	light ambient
 * +0x28 10	light vector(x)
 * +0x2c 11	light vector(y)
 * +0x30 12	light vector(z)
 * +0x34 13	always 0x0002?
 * +0x38 14	field of view angle (fovx) in degrees
 * +0x3c 15	viewport width
 * +0x40 16	viewport height
 * +0x44 17
 * +0x48 18
 * +0x4c 19
 * +0x50 20	priority (top 7 > .. > 0 behind)
 * +0x54 21	viewport center x
 * +0x58 22	viewport center y
 * +0x5c 23	sin(x) ?
 * +0x60 24	cos(x)
 * +0x64 25	sin(y)
 * +0x68 26	cos(y)
 * +0x6c 27	sin(z)
 * +0x70 28	cos(z)
 * +0x74 29	flags (axis flipping?) 0004, 0003
 * +0x78 30	6630, 0001
 * +0x7c 31	7f02
 */
static double mWindowZoom;
static struct Matrix mWindowTransform;

static void
SetupWindow( const INT32 *pWindow )
{
	INT32 fovx = pWindow[0x38/4];
	if( fovx == 0x980 )
	{
		mWindowZoom = /* 0x980 -> */0x198;
	}
	else
	{
		mWindowZoom = /* 0x780 -> */0x21e;
	}
	matrix_Identity( mWindowTransform.M );
	ApplyRotation( &pWindow[1], mWindowTransform.M );
} /* SetupWindow */

static INT32
GetPolyData( INT32 addr )
{
	INT32 result;
	if( addr<0 || addr>=mPtRomSize )
	{
		return -1; /* HACK */
	}
	result = (mpPolyH[addr]<<16)|(mpPolyM[addr]<<8)|mpPolyL[addr];
	if( result&0x00800000 )
	{
		result |= 0xff000000; /* sign extend */
	}
	return result;
} /* GetPolyData */

static void
BlitQuadHelper(
		struct mame_bitmap *pBitmap,
		unsigned color,
		unsigned addr,
		double m[4][4] )
{
	double kScale = (namcos22_gametype == NAMCOS22_ALPINE_RACER)?0.005:0.5;
	struct VerTex v[5];
	int i;
	for( i=0; i<4; i++ )
	{
		double lx = kScale * GetPolyData(  8+i*3+addr );
		double ly = kScale * GetPolyData(  9+i*3+addr );
		double lz = kScale * GetPolyData( 10+i*3+addr );
		struct VerTex *pVerTex = &v[i];
		pVerTex->x = m[0][0]*lx + m[1][0]*ly + m[2][0]*lz + m[3][0];
		pVerTex->y = m[0][1]*lx + m[1][1]*ly + m[2][1]*lz + m[3][1];
		pVerTex->z = m[0][2]*lx + m[1][2]*ly + m[2][2]*lz + m[3][2];
		pVerTex->tx = GetPolyData( 0+2*i+addr )&0xffff;
		pVerTex->ty = GetPolyData( 1+2*i+addr )&0xffff;
	}
	BlitTri( pBitmap, &v[0], color, mWindowZoom ); /* 0,1,2 */
	v[4] = v[0]; /* wrap */
	BlitTri( pBitmap, &v[2], color, mWindowZoom ); /* 2,3,0 */
} /* BlitQuadHelper */

static void
BlitQuads( struct mame_bitmap *pBitmap, INT32 addr, double m[4][4] )
{
	unsigned finish,size, color;

	if( namcos22_gametype == NAMCOS22_ALPINE_RACER )
	{
		if( addr<0x180000 )
		{
			size = GetPolyData(addr+0)&0xff;
			if( size && (size%25)==0 )
			{
				if( (GetPolyData(addr+1)&0xff)==0x18 )
				{
					goto L_Normal;
				}
			}
		}
		return;
		logerror( "unmapped code=%08x\n", addr );
	}
	else
	{
L_Normal:
		size = GetPolyData(addr++)&0xff;
		finish = addr + size;
	}

	while( addr<finish )
	{
		size = GetPolyData(addr++)&0xff;
		switch( size )
		{
		case 0x17:
			color = GetPolyData(addr+2)&0x7f00;
			//BlitQuadHelper( pBitmap,color,addr+3,m );
			break;

		case 0x18:
			color = GetPolyData(addr+2)&0x7f00;
			BlitQuadHelper( pBitmap,color,addr+4,m );
			break;
		}
		addr += size;
	}
} /* BlitQuads */

static void
BlitPolyObject( struct mame_bitmap *pBitmap, int code, double m[4][4] )
{
	unsigned addr1 = GetPolyData(code+0x45);
	for(;;)
	{
		INT32 addr2 = GetPolyData(addr1++);
		if( addr2<0 )
		{
			/* TBA: if addr2 != -1, it may be a jump command to a new object chain */
			break;
		}
		BlitQuads( pBitmap, addr2, m );
	}
} /* BlitPolyObject */

/* hold "M" for polygon test; a single 3d object will be displayed onscreen.
 * J,K,M,N modify the polygon select code, which is displayed at the topleft of the screen
 *
 * press "U" to dump the current 3d object list data to errorlog
 */
static void
PolyTest( struct mame_bitmap *pBitmap )
{
	double M[4][4];
	static int mCode = 0x3c0;
	static double angle;

	if( keyboard_pressed( KEYCODE_M ) )
	{
		while( keyboard_pressed( KEYCODE_M ) ){}
		mCode++;
	}
	if( keyboard_pressed( KEYCODE_N ) )
	{
		while( keyboard_pressed( KEYCODE_N ) ){}
		mCode--;
	}
	if( keyboard_pressed( KEYCODE_K ) )
	{
		while( keyboard_pressed( KEYCODE_K ) ){}
		mCode+=0x10;
	}
	if( keyboard_pressed( KEYCODE_J ) )
	{
		while( keyboard_pressed( KEYCODE_J ) ){}
		mCode-=0x10;
	}
	matrix_Identity( M );
	matrix_RotY( M, sin(angle), cos(angle) );
	matrix_Translate( M, 0, 0, 0x1800 );
	BlitPolyObject( pBitmap, mCode, M );
	angle += 5*3.14159/180.0;
	drawgfx( pBitmap, Machine->uifont, "0123456789abcdef"[(mCode>>8)&0xf],
		0,0,0,12*0,0,NULL,TRANSPARENCY_NONE,0 );
	drawgfx( pBitmap, Machine->uifont, "0123456789abcdef"[(mCode>>4)&0xf],
		0,0,0,12*1,0,NULL,TRANSPARENCY_NONE,0 );
	drawgfx( pBitmap, Machine->uifont, "0123456789abcdef"[(mCode>>0)&0xf],
		0,0,0,12*2,0,NULL,TRANSPARENCY_NONE,0 );
} /* PolyTest */

static void
draw_polygons( struct mame_bitmap *pBitmap, int theWindow )
{
	double M[4][4];
	INT32 code,mode;
	INT32 xpos,ypos,zpos;
	INT32 window;
	INT32 i, iSource0, iSource1, iTarget;
	INT32 param;
	const INT32 *pSource, *pDebug;
	int bDebug = (theWindow==0) && keyboard_pressed( KEYCODE_U );
	const INT32 *pWindow;
	namcos22_polygonram[0x4/4] = 0; /* clear busy flag */
	if( theWindow==0 )
	{
		namcos3d_Start( pBitmap ); /* wipe zbuffer */
	}

	if( namcos22_polygonram[0x10/4] )
	{ /* bank#0 */
		pWindow = (INT32 *)&namcos22_polygonram[0x10000/4];
		pSource = (INT32 *)&namcos22_polygonram[0x10400/4];
	}
	else
	{ /* bank#1 */
		pWindow = (INT32 *)&namcos22_polygonram[0x18000/4];
		pSource = (INT32 *)&namcos22_polygonram[0x18400/4];
	}
	if( !pSource[0] ) return;

	window = -1;
	param = 0;
	mode = 0x8000;
	pWindow += 32*theWindow;
	SetupWindow( pWindow );

	if( keyboard_pressed( KEYCODE_B ) )
	{
		PolyTest( pBitmap );
		return;
	}
	if( bDebug )
	{
		for( i=0; i<8*32; i++ )
		{
			if( (i&0x1f)==0 )
			{
				logerror( "\n%d:\t",i/32 );
			}
			logerror( " %08x", pWindow[i] );
		}
		logerror( "\n" );
	}
	pDebug = pSource;
	for(;;)
	{
		if( bDebug )
		{
			logerror( "\n" );
			while( pDebug<pSource )
			{
				logerror( "%08x ", *pDebug++ );
			}
		}
		code = *pSource++;
		if( ((UINT32)code) < 0x8000 )
		{
			xpos = *pSource++;
			ypos = *pSource++;
			zpos = *pSource++;
			matrix_Identity( M );
			if( mode != 0x8000 )
			{
				if( theWindow == window )
				{
					ApplyRotation( pSource, M );
				}
				pSource += 7;
			}
			if( theWindow == window )
			{
				matrix_Translate( M, xpos,ypos,zpos );
				if( mode != 0x8001 )
				{
					matrix_Multiply( M, mWindowTransform.M );
				}
				BlitPolyObject( pBitmap, code, M );
			}
		}
		else
		{
			switch( code )
			{
			case 0x8000: /* Prop Cycle: environment */
			case 0x8001: /* Prop Cycle: score digits */
			case 0x8002: /* Prop Cycle: for large decorations, starting platform */
				mode = code;
				window = *pSource++;
				break;

			case 0x8004: /* direct polygon (not used in Prop Cycle) */
				pSource += 4*6 + 5;
				/*	depth?, palette#, ?, flags, texpage#
				 *	vertex#0: u, v, x, y, z, intensity
				 *	vertex#1: u, v, x, y, z, intensity
				 *	vertex#2: u, v, x, y, z, intensity
				 *	vertex#3: u, v, x, y, z, intensity
				 */
				break;

			case 0x8008: /* load transformation matrix */
				i = *pSource++;
				assert( i<0x80 );
				pSource = LoadMatrix( pSource,mpMatrix[i].M );
				break;

			case 0x8009: /* matrix composition */
				iSource0 = pSource[0];
				iSource1 = pSource[1];
				iTarget  = pSource[2];
				assert( iSource0<0x80 && iSource1<0x80 && iTarget<0x80 );
				memcpy( M, mpMatrix[iSource0].M, sizeof(M) );
				matrix_Multiply( M, mpMatrix[iSource1].M );
				memcpy( mpMatrix[iTarget].M, M, sizeof(M) );
				pSource += 3;
				break;

			case 0x800a: /* Prop Cycle: flying bicycle, birds */
				code = *pSource++;
				i = *pSource++;
				assert( i<0x80 );
				memcpy( M, &mpMatrix[i], sizeof(M) );
				xpos = *pSource++;
				ypos = *pSource++;
				zpos = *pSource++;
				if( theWindow == window )
				{
					matrix_Translate( M, xpos,ypos,zpos );
					matrix_Multiply( M, mWindowTransform.M );
					BlitPolyObject( pBitmap, code, M );
				}
				break;

			case 0x8010: /* Prop Cycle: preceeds balloons */
				if( pSource[0] == 3 )
				{
					param = pSource[1];
				}
				else
				{
					param = 0;
				}
				while( *pSource++ != 0xffffffff ){}
				/* skip unknown tags:
				 * 0x00000003 0x???????? 0xffffffff
				 * -or-
				 * 0xffffffff
				 */
				break;

			case 0x8017: /* Rave Racer */
				pSource += 4; /* ??? */
				break;

			case (INT32)0xffffffff:
			case 0xffff:
				if( bDebug )
				{
					while( keyboard_pressed( KEYCODE_U ) ){}
					logerror( "\n\n" );
				}
				return;
			default:
				logerror( "unknown opcode: %08x\n", code );
				return;
			}
		}
	}
} /* draw_polygons */

static const INT32 *
LoadMatrix( const INT32 *pSource, double M[4][4] )
{
	double temp[4][4];
	int r,c;

	matrix_Identity( M );
	for(;;)
	{
		switch( *pSource++ )
		{
		case 0x0000: /* translate */
			matrix_Translate( M, pSource[0], pSource[1], pSource[2] );
			pSource += 3;
			break;

		case 0x0001: /* rotate */
			ApplyRotation( pSource, M );
			pSource += 7;
			break;

		case 0x0003: /* unknown */
			pSource += 2;
			break;

		case 0x0006: /* custom */
			for( c=0; c<3; c++ )
			{
				for( r=0; r<3; r++ )
				{
					temp[r][c] = *pSource++ / (double)0x3fff;
				}
				temp[c][3] = temp[3][c] = 0;
			}
			temp[3][3] = 1;
			matrix_Multiply( M, temp );
			break;

		case 0xffffffff:
			return pSource;

		default:
			exit(1);
		}
	}
} /* LoadMatrix */
