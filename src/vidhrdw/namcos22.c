/**
 * video hardware for Namco System22
 */
#include <assert.h>
#include "namcos22.h"
#include "namcos3d.h"
#include "matrix3d.h"
#include <math.h>

static int mbDSPisActive;

static int mbSuperSystem22; /* used to conditionally support Super System22-specific features */

/* mWindowPri and mMasterBias reflect modal polygon rendering parameters */
static INT32 mWindowPri;
static INT32 mMasterBias;

#define SPRITERAM_SIZE (0x9b0000-0x980000)

#define CGRAM_SIZE 0x1e000
#define NUM_CG_CHARS ((CGRAM_SIZE*8)/(64*16)) /* 0x3c0 */

static int mPtRomSize;
static const data8_t *mpPolyH;
static const data8_t *mpPolyM;
static const data8_t *mpPolyL;

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

data32_t namcos22_point_rom_r( offs_t offs )
{
	return GetPolyData(offs);
}

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
data32_t *namcos22_gamma;
data32_t *namcos22_vics_data;
data32_t *namcos22_vics_control;

static int cgsomethingisdirty;
static unsigned char *cgdirty;
static unsigned char *dirtypal;
static struct tilemap *tilemap;

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

static void TextTilemapGetInfo( int tile_index )
{
	data16_t data = nthword( namcos22_textram,tile_index );
	/**
	 * xxxx------------ palette select
	 * ----xx---------- flip
	 * ------xxxxxxxxxx code
	 */
	SET_TILE_INFO( NAMCOS22_ALPHA_GFX,data&0x3ff,data>>12,TILE_FLIPYX((data>>10)&3) );
}

/**
 * mydrawgfxzoom is used to insert a zoomed 2d sprite into an already-rendered 3d scene.
 */
static void
mydrawgfxzoom(
	struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
	unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
	const struct rectangle *clip,int transparency,int transparent_color,
	int scalex, int scaley, INT32 zcoord )
{
	struct rectangle myclip;
	if (!scalex || !scaley) return;
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}
	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];
		UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
		int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
		int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;
		if (sprite_screen_width && sprite_screen_height)
		{
			int dx = (gfx->width<<16)/sprite_screen_width;
			int dy = (gfx->height<<16)/sprite_screen_height;
			int ex = sx+sprite_screen_width;
			int ey = sy+sprite_screen_height;
			int x_index_base;
			int y_index;
			if( flipx )
			{
				x_index_base = (sprite_screen_width-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}
			if( flipy )
			{
				y_index = (sprite_screen_height-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}
			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}
			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
				int y;
				for( y=sy; y<ey; y++ )
				{
					INT32 *pZBuf = namco_zbuffer + NAMCOS22_SCREEN_WIDTH*y;
					UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
					UINT32 *dest = (UINT32 *)dest_bmp->line[y];
					int x, x_index = x_index_base;
					for( x=sx; x<ex; x++ )
					{
						if( zcoord<pZBuf[x] )
						{
							int c = source[x_index>>16];
							if( c != transparent_color )
							{
								dest[x] = pal[c];
								pZBuf[x] = zcoord;
							}
						}
						x_index += dx;
					}
					y_index += dy;
				}
			}
		}
	}
}

static void
DrawSpritesHelper(
	struct mame_bitmap *bitmap,
	const struct rectangle *cliprect,
	const data32_t *pSource,
	const data32_t *pPal,
	int num_sprites,
	int deltax,
	int deltay,
	int bVICS)
{
	int i;
	int color,flipx,flipy;
	data32_t xypos, size, attrs, code;
	int numcols, numrows, row, col;
	int xpos, ypos, tile;
	int zoomx, zoomy;
	int sizex, sizey;

	color = 0;
	flipx = 0;
	flipy = 0;
	for( i=0; i<num_sprites; i++ )
	{
		/*
		pSource:
		    xxxx---- -------- -------- -------- xpos
			----xxxx -------- -------- -------- ypos
			-------- xxxx---- -------- -------- zoomx
			-------- ----xxxx -------- -------- zoomy
			-------- -------- -x------ -------- hide this sprite
			-------- -------- --xxxx-- -------- ?
			-------- -------- ------x- -------- flipx,numcols
			-------- -------- -------x -------- flipy,numrows
			-------- -------- -------- xxxx---- tile number
			-------- -------- -------- ----xxxx ?
		*/
		attrs = pSource[2];
		if( (attrs&0x04000000)==0 )
		{ /* sprite is not hidden */
			INT32 zcoord = pPal[0];
			if( bVICS ) zcoord = 0;
			color = pPal[1]>>16;
			/* pPal[1]&0xffff  CZ (color z - for depth cueing) */

			xypos = pSource[0];
			xpos = (xypos>>16)-deltax;
			ypos = (xypos&0xffff)-deltay;
			size = pSource[1];

			sizex = size>>16;
			sizey = size&0xffff;
			zoomx = (1<<16)*sizex/0x20;
			zoomy = (1<<16)*sizey/0x20;

			flipy = attrs&0x8;
			numrows = attrs&0x7; /* 0000 0001 1111 1111 0000 0000 fccc frrr */
			if( numrows==0 ) numrows = 8;
			if( flipy )
			{
				ypos += sizey*(numrows-1);
				sizey = -sizey;
			}

			flipx = (attrs>>4)&0x8;
			numcols = (attrs>>4)&0x7;
			if( numcols==0 ) numcols = 8;
			if( flipx )
			{
				xpos += sizex*(numcols-1);
				sizex = -sizex;
			}

			code = pSource[3];
			tile = code>>16;

			if( attrs & 0x0200 )
			{ /* right justify */
				xpos -= ((zoomx*numcols*0x20)>>16)-1;
			}

			for( row=0; row<numrows; row++ )
			{
				for( col=0; col<numcols; col++ )
				{
					if( bVICS )
					{
						drawgfxzoom( bitmap, Machine->gfx[0],
							tile,
							color,
							flipx, flipy,
							xpos+col*sizex/*-5*/, ypos+row*sizey/*+4*/,
							cliprect,
							TRANSPARENCY_PEN, 255,
							zoomx, zoomy );
					}
					else
					{
						mydrawgfxzoom( bitmap, Machine->gfx[0],
							tile,
							color,
							flipx, flipy,
							xpos+col*sizex/*-5*/, ypos+row*sizey/*+4*/,
							cliprect,
							TRANSPARENCY_PEN, 255,
							zoomx, zoomy,
							zcoord );
					}
					tile++;
				}
			}
		} /* visible sprite */
		pSource -= 4;
		pPal -= 2;
	}
}


static void
DrawSprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/*
		0x980000:	00060000 00010000 02ff0000 000007ff
                             ^^^^                       num sprites

		0x980010:	00200020 000002ff 000007ff 00000000
                             ^^^^                       delta xpos
                                      ^^^^              delta ypos

		0x980200:	000007ff 000007ff		delta xpos, delta ypos
		0x980208:	000007ff 000007ff
		0x980210:	000007ff 000007ff
		0x980218:	000007ff 000007ff
		0x980220:	000007ff 000007ff
		0x980228:	000007ff 000007ff
		0x980230:	000007ff 000007ff
		0x980238:	000007ff 000007ff

		0x980400:	?
		0x980600:	?
			0000	0000.0000.0000.0000
			8000	1000.0000.0000.0000
			8080	1000.0000.1000.0000
			8880	1000.1000.1000.0000
			8888	1000.1000.1000.1000
			a888	1010.1000.1000.1000
			a8a8	1010.1000.1010.1000
			aaa8	1010.1010.1010.1000
			aaaa	1010.1010.1010.1010
			eaaa	1110.1010.1010.1010
			eaea	1110.1010.1110.1010
			eeea	1110.1110.1110.1010
			eeee	1110.1110.1110.1110
			feee	1111.1110.1110.1110
			fefe	1111.1110.1111.1110
			fffe	1111.1111.1111.1110
			ffff	1111.1111.1111.1111

		0x980800:	0000 0001 0002 0003 ... 03ff (probably indirection for sprite list)

		eight words per sprite:
		0x984000:	010f 007b	xpos, ypos
		0x984004:	0020 0020	size x, size y
		0x984008:	00ff 0311	00ff, chr x;chr y;flip x;flip y
		0x98400c:	0001 0000	sprite code, ????
		...

		additional sorting/color data for sprite:
		0x9a0000:	C381 Z (sort)
		0x9a0004:	palette, C381 ZC (depth cueing?)
		...
	*/
	int num_sprites = (spriteram32[0x04/4]>>16)&0x3ff; /* max 1024 sprites? */
	const data32_t *pSource = &spriteram32[0x4000/4]+num_sprites*4;
	const data32_t *pPal = &spriteram32[0x20000/4]+num_sprites*2;
	int deltax = spriteram32[0x14/4]>>16;
	int deltay = spriteram32[0x18/4]>>16;
	DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay, 0 );

	/* VICS RAM provides two additional banks */
	/*
		0x940000 -x------		sprite chip busy
		0x940018 xxxx----		clr.w   $940018.l

		0x940030 ----xxxx
		0x940034 xxxxxxxx		0x3070b0f
		0x940038 xxxxxxxx

		0x940040 xxxxxxxx		sprite attribute size
		0x940044 xxxxxxxx		?
		0x940048 xxxxxxxx		sprite attribute list baseaddr
		0x94004c xxxxxxxx		?
		0x940050 xxxxxxxx		sprite color size
		0x940054 xxxxxxxx		?
		0x940058 xxxxxxxx		sprite color list baseaddr
		0x94005c xxxxxxxx		?

		0x940060 xxxxxxxx		sprite attribute size
		0x940064 xxxxxxxx		?
		0x940068 xxxxxxxx		sprite attribute list baseaddr
		0x94006c xxxxxxxx		?
		0x940070 xxxxxxxx		sprite color size
		0x940074 xxxxxxxx		?
		0x940078 xxxxxxxx		sprite color list baseaddr
		0x94007c xxxxxxxx		?
	*/

	num_sprites = (namcos22_vics_control[0x40/4]&0xffff)/0x10;
	if( num_sprites>=1 )
	{
		pSource = &namcos22_vics_data[(namcos22_vics_control[0x48/4] - 0x900000)/4];
		pPal    = &namcos22_vics_data[(namcos22_vics_control[0x58/4] - 0x900000)/4];
		if ((pSource >= &namcos22_vics_data[0]) && (pSource <= &namcos22_vics_data[0xffff]))
		{
			num_sprites--;
			pSource += num_sprites*4;
			pPal += num_sprites*2;
			num_sprites++;
			DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay, 1 );
		}
	}

	num_sprites = (namcos22_vics_control[0x60/4]&0xffff)/0x10;
	if( num_sprites>=1 )
	{
		pSource = &namcos22_vics_data[(namcos22_vics_control[0x68/4] - 0x900000)/4];
		pPal    = &namcos22_vics_data[(namcos22_vics_control[0x78/4] - 0x900000)/4];

		if ((pSource >= &namcos22_vics_data[0]) && (pSource <= &namcos22_vics_data[0xffff]))
		{
			num_sprites--;
			pSource += num_sprites*4;
			pPal += num_sprites*2;
			num_sprites++;
			DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay, 1 );
		}

	}
} /* DrawSprites */

static void
UpdatePaletteS( void ) /* for Super System22 - apply gamma correction and preliminary fader support */
{
	int i,j;

	int red   = nthbyte( namcos22_gamma, 0x16 );
	int green = nthbyte( namcos22_gamma, 0x17 );
	int blue  = nthbyte( namcos22_gamma, 0x18 );
	int fade  = nthbyte( namcos22_gamma, 0x19 );
	/* int flags = nthbyte( namcos22_gamma, 0x1a ); */

	tilemap_set_palette_offset( tilemap, nthbyte(namcos22_gamma,0x1b)*256 );

	for( i=0; i<NAMCOS22_PALETTE_SIZE/4; i++ )
	{
		if( dirtypal[i] )
		{
			for( j=0; j<4; j++ )
			{
				int which = i*4+j;
				int r = nthbyte(paletteram32,which+0x00000);
				int g = nthbyte(paletteram32,which+0x08000);
				int b = nthbyte(paletteram32,which+0x10000);

				if( fade )
				{ /**
				   * if flags&0x01 is set, fader affects polygon layer
				   * flags&0x02 and flags&0x04 are used to fade text/sprite layer
				   *
				   * for now, ignore flags and fade all palette entries
				   */
					r = (r*(0x100-fade)+red*fade)/256;
					g = (g*(0x100-fade)+green*fade)/256;
					b = (b*(0x100-fade)+blue*fade)/256;
				}

				/* map through gamma table (before or after fader?) */
				r = nthbyte( &namcos22_gamma[0x100/4], r );
				g = nthbyte( &namcos22_gamma[0x200/4], g );
				b = nthbyte( &namcos22_gamma[0x300/4], b );

				palette_set_color( which,r,g,b );
			}
			dirtypal[i] = 0;
		}
	}
} /* UpdatePaletteS */

static void
UpdatePalette( void ) /* for System22 - ignore gamma/fader effects for now */
{
	int i,j;

	tilemap_set_palette_offset( tilemap, 0x7f00 );

	for( i=0; i<NAMCOS22_PALETTE_SIZE/4; i++ )
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
} /* UpdatePalette */

static void
DrawTextLayer( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
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
} /* DrawTextLayer */

/*********************************************************************************************/

/**
 * 00000
 * 00004 set by main CPU; triggers DSP to start drawing scene.  DSP stores zero when finished.
 * 00008
 * 0000c
 * 0002c busy status(?)
 * 00010 display list bank select
 *
 * 00030
 * 00040	Point RAM Write Enable (ff=enable)
 * 00044	Point RAM IDC (index count register)
 * 00058
 * 0005c
 *
 * 00c00..0ffff  Point RAM Port (Point RAM has 128K words, 512KB space (128K 24bit)
 *
 * 0e000: used for collision detection? (Time Crisis)
 *
 * 10000..1007f
 *	00)10000:	0000000f (always?)
 *
 *	01)10004	000019b1 00007d64 // rolx (master camera)
 *	03)1000c	00005ece 000055fe // roly (master camera)
 *	05)10014	000001dd 00007ffc // rolz (master camera)
 *	07)1001c	00000002 (rolt)
 *
 *	08)10020	000000c8 // light power   [C008]
 *	09)10024	00000014 // light ambient [C009]
 *	0a)10028	00000000 00005a82 ffffa57e // light vector [C00A]
 *
 *	0d)10034	00000002 (always?)        [C00D]
 *	0e)10038	00000780 // field of view [C00E]
 *	0f)1003c	00000140                  [C00F]
 *  10)10040	000000f0                  [C010]
 *
 *	11)10048	00000000 00000000 00000000[C011..C013]
 *
 *	14)10050	00000007 // priority          [C014]
 *	15)10054	00000000 // viewport center x [C015]
 *	16)10058	00000000 // viewport center y [C016]
 *
 *	17)1005c	00000000 00007fff // rolx? [C017 C018]
 *				00000000 00007fff // roly? [C019 C01A]
 *				00000000 00007fff // rolz? [C01B C01C]
 *
 *	1d)10074	00010000 // flags?         [C01D]
 *	1e)			00000000 00000000 // unknown
 *
 * 10080..100ff
 * 10100..1017f
 * 10180..101ff
 * 10200..1027f
 * 10280..102ff
 * 10300..1037f
 * 10380..103ff
 *
 * 10400..17fff: scene data
 *
 * 18000..183ff: bank#2 window attr
 * 18400..1ffff: bank#2, scene data
 */
static namcos22_camera mCamera;

static void
SetupWindow( const INT32 *pWindow, int which )
{
	double fov = pWindow[0x38/4];
	double vw,vh;
	pWindow += which*0x80/4;
	vw = pWindow[0x3c/4];
	vh = pWindow[0x40/4];
	mCamera.cx  = 320+pWindow[0x54/4];
	mCamera.cy  = 240-pWindow[0x58/4];
	mCamera.clip.min_x = mCamera.cx - vw;
	mCamera.clip.min_y = mCamera.cy - vh;
	mCamera.clip.max_x = mCamera.cx + vw;
	mCamera.clip.max_y = mCamera.cy + vh;
	if( mCamera.clip.min_x <   0 ) mCamera.clip.min_x = 0;
	if( mCamera.clip.min_y <   0 ) mCamera.clip.min_y = 0;
	if( mCamera.clip.max_x > NAMCOS22_NUM_COLS*16 ) mCamera.clip.max_x = NAMCOS22_NUM_COLS*16;
	if( mCamera.clip.max_y > NAMCOS22_NUM_ROWS*16 ) mCamera.clip.max_y = NAMCOS22_NUM_ROWS*16;
	if( namcos22_gametype == NAMCOS22_PROP_CYCLE )
	{
		fov /= 32.0;
	}
	else if( namcos22_gametype == NAMCOS22_TIME_CRISIS )
	{
		fov /= 32.0;
	}
	fov = fov*3.141592654/180.0; /* degrees to radians */
	mCamera.zoom = vw/tan(fov/2.0);
//	mWindowPri = pWindow[0x50/4]&0x7;
//	mCamera.power   = (INT16)(pWindow[0x20/4])/(double)0xff;
//	mCamera.ambient = (INT16)(pWindow[0x24/4])/(double)0xff;
	mCamera.x       = (INT16)(pWindow[0x28/4])/(double)0x7fff;
	mCamera.y       = (INT16)(pWindow[0x2c/4])/(double)0x7fff;
	mCamera.z       = (INT16)(pWindow[0x30/4])/(double)0x7fff;
} /* SetupWindow */

static void
BlitQuadHelper(
		struct mame_bitmap *pBitmap,
		unsigned color,
		unsigned addr,
		double m[4][4],
		INT32 zcode,
		INT32 flags )
{
	double zmin, zmax, zrep;
	struct VerTex v[5];
	int i;

	zmin = zmax = 0;

	color &= 0x7f00;

	for( i=0; i<4; i++ )
	{
		struct VerTex *pVerTex = &v[i];
		double lx = GetPolyData(  8+i*3+addr );
		double ly = GetPolyData(  9+i*3+addr );
		double lz = GetPolyData( 10+i*3+addr );
		pVerTex->x = m[0][0]*lx + m[1][0]*ly + m[2][0]*lz + m[3][0];
		pVerTex->y = m[0][1]*lx + m[1][1]*ly + m[2][1]*lz + m[3][1];
		pVerTex->z = m[0][2]*lx + m[1][2]*ly + m[2][2]*lz + m[3][2];
		pVerTex->u = GetPolyData( 0+2*i+addr )&0x0fff;
		pVerTex->v = GetPolyData( 1+2*i+addr )&0xffff;
		pVerTex->i = (GetPolyData(i+addr)>>16)&0xff;
		if( i==0 || pVerTex->z > zmax ) zmax = pVerTex->z;
		if( i==0 || pVerTex->z < zmin ) zmin = pVerTex->z;
	}

	/**
	 * The method for computing represenative z value may vary per polygon.
	 * Possible methods include:
	 * - minimum value: zmin
	 * - maximum value: zmax
	 * - average value: (zmin+zmax)/2
	 * - average of all four z coordinates
	 */
	zrep = (zmin+zmax)/2.0; /* for now just always use the simpler average */

	/**
	 * hardware supports two types priority modes:
	 *
	 * direct: use explicit zcode, ignoring the polygon's z coordinate
	 * ---xxxxx xxxxxxxx xxxxxxxx
	 *
	 * relative: representative z + shift values
	 * ---xxx-- -------- -------- shift absolute priority
	 * ------xx xxxxxxxx xxxxxxxx shift z-representative value
	 */

	if( namcos22_gametype == NAMCOS22_PROP_CYCLE )
	{ /* for now, assume all polygons use relative priority */
		INT32 dw = (zcode&0x1c0000)>>18; /* window (master)priority bias */
		INT32 dz = (zcode&0x03ffff); /* bias for representative z coordinate */

		if( dw&4 )
		{
			dw |= ~0x7; /* sign extend */
		}
		dw += mWindowPri;
		if( dw<0 ) dw = 0; else if( dw>7 ) dw = 7; /* cap it at min/max */
		dw <<= 21;

		if( dz&0x020000 )
		{
			dz |= ~0x03ffff; /* sign extend */
		}
		dz += (INT32)zrep;
		dz += mMasterBias;
		if( dz<0 ) dz = 0; else if( dz>0x1fffff ) dz = 0x1fffff; /* cap it at min/max */

		/**
		 * xxx----- -------- -------- master-priority
		 * ---xxxxx xxxxxxxx xxxxxxxx sub-priority
		 */
		zcode = dw|dz;
	}
	else if( namcos22_gametype == NAMCOS22_RAVE_RACER ||
			 namcos22_gametype == NAMCOS22_VICTORY_LAP )
	{
		/** Rave Racer:
		 * 3b7740
		 * --11.1011.0111.0111.0100.0000
		 */
		INT32 dw = mWindowPri<<24;
		INT32 dz = zcode;
		dz += (INT32)zrep;
		zcode = dw|dz;
	}
	else
	{
		zcode = 0x10000 + (INT32)zrep;
	}

	namcos22_BlitTri( pBitmap, &v[0], color, zcode, flags, &mCamera ); /* 0,1,2 */
	v[4] = v[0]; /* wrap */
	namcos22_BlitTri( pBitmap, &v[2], color, zcode, flags, &mCamera ); /* 2,3,0 */
} /* BlitQuadHelper */


static void
BlitQuads( struct mame_bitmap *pBitmap, INT32 addr, double m[4][4], INT32 base )
{
	INT32 size = GetPolyData(addr++);
	INT32 finish = addr + (size&0xff);
	INT32 flags;
	INT32 color;
	INT32 bias;

	while( addr<finish )
	{
		size = GetPolyData(addr++);
		size &= 0xff;
		switch( size )
		{
		case 0x17:
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias = 0;
			BlitQuadHelper( pBitmap,color,addr+3,m,0,flags );
			break;

		case 0x18:
			/**
			 * word 0: ?
			 *		0b3480 ?
			 *		800000 ?
			 *		000040 ?
			 *
			 * word 1: (flags)
			 *		1042 (always set?)
			 *		0200 ?
			 *		0100 ?
			 *		0020 one-sided
			 *		0001 ?
			 *
			 *		1163 // sky
			 *		1262 // score (front)
			 *		1242 // score (hinge)
			 *		1063 // n/a
			 *		1243 // various (2-sided?)
			 *		1263 // everything else (1-sided?)
			 *
			 * word 2: color
			 *			-------- xxxxxxxx unused?
			 *			-xxxxxxx -------- palette select
			 *          x------- -------- palette bank? (normally 0x4000; see Cyber Commando)
			 *
			 * word 3: depth bias
			 */
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias  = GetPolyData(addr+3);
			BlitQuadHelper( pBitmap,color,addr+4,m,bias,flags );
			break;

		case 0x10: /* apply lighting effects */
		/*
			333401 000000 000000 004000 parameters
			  	000000 000000 007fff normal vector
			  	000000 000000 007fff normal vector
			  	000000 000000 007fff normal vector
  				000000 000000 007fff normal vector
		*/
			break;

		case 0x0d:
			/* unknown! */
			break;

		default:
			//printf( "unexpected point data %08x at %08x.%08x\n", size, base, start );
			break;
		}
		addr += size;
	}
} /* BlitQuads */

static void
BlitPolyObject( struct mame_bitmap *pBitmap, int code, double M[4][4] )
{
	unsigned addr1 = GetPolyData(code);
	if( namcos22_gametype == NAMCOS22_CYBER_CYCLES )
	{ /* HACK! */
		if( code == 0x69c || code == 0x69b ||
		    code == 0x6b1 || code == 0x6b2 ) return;
	}
	for(;;)
	{
		INT32 addr2 = GetPolyData(addr1++);
		if( addr2<0 )
		{
			break;
		}
		BlitQuads( pBitmap, addr2, M, code );
	}
} /* BlitPolyObject */

/****************************************************************/

READ32_HANDLER( namcos22_dspram_r )
{
	return namcos22_polygonram[offset];
}

WRITE32_HANDLER( namcos22_dspram_w )
{
	COMBINE_DATA( &namcos22_polygonram[offset] );
//	namcos22_UploadCodeToDSP();
}

static float
FloatDspToNative( data32_t iVal )
{
	int expVal = (iVal>>16) - 0x24;
	float fVal = (iVal&0xffff);
	while( expVal>0 )
	{
		fVal /= 2.0;
		expVal--;
	}
	while( expVal<0 )
	{
		fVal *= 2.0;
		expVal++;
	}
	return fVal;
}

/*******************************************************************************/
//#define SLAVE_BUFSIZE 0x4000 /* TBA: dynamically allocate */
//static data32_t mSlaveBuf[SLAVE_BUFSIZE];
//static int mSlaveBufWriteIdx;
//static int mSlaveBufReadIdx;
//
//void
//namcos22_WriteDataToRenderDevice( data32_t data )
//{
//	mSlaveBuf[mSlaveBufWriteIdx++] = data;
//	logerror( "render: 0x%08x\n", data );
//	if( mSlaveBufWriteIdx>=SLAVE_BUFSIZE )
//	{
//		mSlaveBufWriteIdx = 0;
//	}
//}
//
//static int
//SlaveBufSize( void )
//{
//	return (mSlaveBufWriteIdx - mSlaveBufReadIdx)&(SLAVE_BUFSIZE-1);
//}
//
//static data32_t
//ReadDataFromSlaveBuf( void )
//{
//	data32_t result = mSlaveBuf[mSlaveBufReadIdx++];
//	if( mSlaveBufReadIdx>=SLAVE_BUFSIZE )
//	{
//		mSlaveBufReadIdx = 0;
//	}
//	return result;
//}

/**
 * master DSP can write directly to render device via port 0xc.
 * This is used for "direct drawn" polygons, and "direct draw from point rom"
 * feature - both opcodes exist in Ridge Racer's display-list processing
 *
 * record format:
 *	header (3 words)
 *		zcode
 *		color
 *		flags
 *
 *	per-vertex data (4*6 words)
 *		u,v
 *		sx,sy
 *		intensity/z.exponent
 *		z.mantissa
 *
 * master DSP can specify 3d objects indirectly (along with view transforms),
 * via the "transmit" PDP opcode.  the "render device" emits object records in the
 * same format above, performing viewspace clipping, projection, and lighting calculations
 *
 * most "3d object" references are 0x45 and greater.  references less than 0x45 are "special"
 * commands, using a similar point rom format.  the point rom header may point to point ram.
 *
 *
 * slave DSP can read records via port 4
 * its primary purpose appears to be sorting the list of quads, and passing them on to a "draw device"
 *
 */

/*******************************************************************************/

static INT32 mCode;
static double M[4][4], V[4][4];
/**
 * 0xfffd
 * 0x0: transform
 * 0x1
 * 0x2
 * 0x5: transform
 * >=0x45: draw primitive
 */

static void
Handle00BB0003( const INT32 *pSource )
{
	/*
		00bb0003 (fov = 0x980)
		001400c8
		00010000 // ?,   light.dx
		00065a82 // pri, light.dy
		0000a57e // ?,   light.dz
		00000000
		00280000
		0020a801		0020a801
		001fbc01		001fbc01
		002e0000
		----7ffe		----0000		----0000
		----0000		----7ffe		----0000
		----0000		----0000		----7ffe

		003b0003 (fov = 0x780)
		001400c8
		00010000 // ?,   light.dx
		00075a82 // pri, light.dy
		0000a57e // ?,   light.dz
		00000000
		00294548
		001fb7f4		001fb7f4
		001e9fee		001e9fee
		002e0000
		----7ffc		----0000		----0000
		----0000		----7ffc		----0000
		----0000		----0000		----7ffc
	*/
	mWindowPri = pSource[0x3]>>16;
	mCamera.power   = (INT16)(pSource[0x1]&0xffff)/(double)0xff;
	mCamera.ambient = (INT16)(pSource[0x1]>>16)/(double)0xff;
	//mCamera.zoom = FloatDspToNative(pSource[6]);
	/**
	 *	light.dx      = (INT16)pSource[0x2];
	 *	light.dy      = (INT16)pSource[0x3];
	 *	light.dz      = (INT16)pSource[0x4];
	 *
	 * field of view related (floating point)
	 *	pSource[0x6]
	 *	pSource[0x7]==pSource[0x8]
	 *	pSource[0x9]==pSource[0xa]
	 */
	V[0][0] = ((INT16)pSource[0x0c])/(double)0x7fff;
	V[1][0] = ((INT16)pSource[0x0d])/(double)0x7fff;
	V[2][0] = ((INT16)pSource[0x0e])/(double)0x7fff;
	V[0][1] = ((INT16)pSource[0x0f])/(double)0x7fff;
	V[1][1] = ((INT16)pSource[0x10])/(double)0x7fff;
	V[2][1] = ((INT16)pSource[0x11])/(double)0x7fff;
	V[0][2] = ((INT16)pSource[0x12])/(double)0x7fff;
	V[1][2] = ((INT16)pSource[0x13])/(double)0x7fff;
	V[2][2] = ((INT16)pSource[0x14])/(double)0x7fff;

	{ /* hack for time crisis */
		int i,j;
		int bGood = 0;
		for( i=0; i<3; i++ )
		{
			for( j=0; j<3; j++ )
			{
				if( V[i][j] ) bGood = 1;
			}
		}
		if( !bGood )
		{
			struct RotParam param;
			const INT32 *pSrc = 1 + &namcos22_polygonram[0x10080/4];
			param.thx_sin = ((INT16)pSrc[0])/(double)0x7fff;
			param.thx_cos = ((INT16)pSrc[1])/(double)0x7fff;
			param.thy_sin = ((INT16)pSrc[2])/(double)0x7fff;
			param.thy_cos = ((INT16)pSrc[3])/(double)0x7fff;
			param.thz_sin = ((INT16)pSrc[4])/(double)0x7fff;
			param.thz_cos = ((INT16)pSrc[5])/(double)0x7fff;
			param.rolt = (INT16)pSrc[6];
			matrix3d_Identity( V );
			namcos3d_Rotate( V, &param );
		}
	}
}

static void
Handle00200002( const INT32 *pSource )
{
	M[0][0] = ((INT16)pSource[0x1])/(double)0x7fff;
	M[1][0] = ((INT16)pSource[0x2])/(double)0x7fff;
	M[2][0] = ((INT16)pSource[0x3])/(double)0x7fff;
	M[0][1] = ((INT16)pSource[0x4])/(double)0x7fff;
	M[1][1] = ((INT16)pSource[0x5])/(double)0x7fff;
	M[2][1] = ((INT16)pSource[0x6])/(double)0x7fff;
	M[0][2] = ((INT16)pSource[0x7])/(double)0x7fff;
	M[1][2] = ((INT16)pSource[0x8])/(double)0x7fff;
	M[2][2] = ((INT16)pSource[0x9])/(double)0x7fff;
	M[3][0] = pSource[0xa]; /* xpos */
	M[3][1] = pSource[0xb]; /* ypos */
	M[3][2] = pSource[0xc]; /* zpos */
	if( mCode>=0x45 )
	{
		matrix3d_Multiply( M, V );
		BlitPolyObject( Machine->scrbitmap, mCode, M );
	}
}

static void
SimulateSlaveDSP( int bDebug )
{
	const INT32 *pSource = 0x300 + (INT32 *)namcos22_polygonram;
	INT16 len;

	matrix3d_Identity( V );
	matrix3d_Identity( M );

	if( mbSuperSystem22 )
	{
		pSource += 4; /* FFFE 0400 */
	}
	else
	{
		pSource--;
	}

	for(;;)
	{
		INT16 marker, next;
		mCode = *pSource++;
		len  = (INT16)*pSource++;
		if( bDebug )
		{
			int i;
		//	logerror( "addr=0x%04x ", 0x8000 + (pSource - namcos22_polygonram) );
			logerror( "[code=0x%x; len=0x%x]\n", mCode, len );
			for( i=0; i<len; i++ )
			{
				logerror( " %08x", pSource[i] );
			}
			logerror( "\n" );
		}

		switch( len )
		{
		case 0x15:
			Handle00BB0003( pSource ); /* define viewport */
			break;

		case 0x10:
			/* unknown (used with 0x8010) */
			break;

		case 0x0a:
			/* unknown 3x3 Matrix */
			break;

		case 0x0d:
			Handle00200002( pSource );
			break;

		default:
			return;
		}

		/* hackery! commands should be streamed by PDP, not parsed here */
		pSource += len;
		marker = (INT16)*pSource++; /* always 0xffff */
		next   = (INT16)*pSource++; /* link to next command */

		if( bDebug )
		{
			logerror( "cmd: %04x %04x\n", marker, next );
		}

		if( (next&0x7fff) != (pSource - (INT32 *)namcos22_polygonram) )
		{ /* end of list */
			break;
		}
	} /* for(;;) */
} /* SimulateSlaveDSP */

//static void DrawPolyOrig( struct mame_bitmap *bitmap );

static void
DrawPolygons( struct mame_bitmap *bitmap )
{
	const INT32 *pWindow = (INT32 *)&namcos22_polygonram[0x10000/4];
	SetupWindow( pWindow,0 ); /* HACK! */
	if( mbDSPisActive )
	{
		SimulateSlaveDSP( keyboard_pressed(KEYCODE_Q) );

		if( 0 && keyboard_pressed(KEYCODE_Q) )
		{
			int i;
			for( i=0x10000; i<0x20000; i+=4 )
			{
				if( (i&0x7f)==0 ) logerror( "\n%08x: ", i );
				logerror( " %08x", namcos22_polygonram[i/4] );
			}
		}

		while( keyboard_pressed(KEYCODE_Q) ){}
	}
} /* DrawPolygons */

void
namcos22_dsp_enable( void )
{
	mbDSPisActive = 1;
}

/*********************************************************************************************/

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

READ32_HANDLER( namcos22_gamma_r )
{
	return namcos22_gamma[offset];
}

/*
	+0x0002.w	Fader Enable(?) (0: disabled)
	+0x0011.w	Display Fader (R) (0x0100 = 1.0)
	+0x0013.w	Display Fader (G) (0x0100 = 1.0)
	+0x0015.w	Display Fader (B) (0x0100 = 1.0)
	+0x0100.b	Fog1 Color (R) (world fogging)
	+0x0101.b	Fog2 Color (R) (used for heating of brake-disc on RV1)
	+0x0180.b	Fog1 Color (G)
	+0x0181.b	Fog2 Color (G)
	+0x0200.b	Fog1 Color (B)
	+0x0201.b	Fog2 Color (B)
*/

/**
 * 0x800000: 08380000
 *
 * 0x810000: 0004 0004 0004 0004 4444
 *           ^^^^ ^^^^ ^^^^ ^^^^      fog thickness? 0 if disabled
 *
 * 0x810200..0x8103ff: 0x100 words (depth cueing table)
 *		0000 0015 002a 003f 0054 0069 007e 0093
 *		00a8 00bd 00d2 00e7 00fc 0111 0126 013b
 *
 * 0x810400..0x810403: (air combat22?)
 * 0x820000..0x8202ff: ?
 *
 * 0x824000..0x8243ff: gamma
 *	 Prop Cycle: start of stage#2
 *	 ffffff00 00ffffff 0000007f 00000000
 *	 0000ff00 0f00ffff ff00017f 00010007
 *	 00000001
 *
 *	 Prop Cycle: submerged in swamp
 *	 ffffff00 0003ffae 0000007f 00000000
 *   0000ff00 0f00ffff ff00017f 00010007
 *   00000001
 *
 * 0x860000..0x860007: ?
 */
WRITE32_HANDLER( namcos22_gamma_w )
{
	data32_t old = namcos22_gamma[offset];
	COMBINE_DATA( &namcos22_gamma[offset] );
	if( old!=namcos22_gamma[offset] )
	{
		memset( dirtypal, 1, NAMCOS22_PALETTE_SIZE/4 );
	}
	/**
	 * 824000: ffffff00 00ffffff 0000007f 00000000
	 *                    ^^^^^^                    RGB(fog)
	 *
	 * 824010: 0000ff00 0f00RRGG BBII017f 00010007
	 *                      ^^^^ ^^                 RGB(fade)
	 *                             ^^               fade (zero for none, 0xff for max)
	 *                               ^^             flags; fader targer
	 *                                                     1: affects polygon layer
	 *                                                     2: affects text(?)
	 *                                                     4: affects sprites(?)
	 *                                 ^^           tilemap palette base
	 *
	 * 824020: 00000001 00000000 00000000 00000000
	 *
	 * 824100: 00 05 0a 0f 13 17 1a 1e ... (red)
	 * 824200: 00 05 0a 0f 13 17 1a 1e ... (green)
	 * 824300: 00 05 0a 0f 13 17 1a 1e ... (blue)
	 */
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
}


VIDEO_START( namcos22s )
{
	mbDSPisActive = 0;
	if( namcos3d_Init(
		NAMCOS22_SCREEN_WIDTH,
		NAMCOS22_SCREEN_HEIGHT,
		memory_region(REGION_GFX3),	/* tilemap */
		memory_region(REGION_GFX2)	/* texture */
	) == 0 )
	{
		struct GfxElement *pGfx = decodegfx( (UINT8 *)namcos22_cgram,&cg_layout );
		if( pGfx )
		{
			Machine->gfx[NAMCOS22_ALPHA_GFX] = pGfx;
			pGfx->colortable = Machine->remapped_colortable;
			pGfx->total_colors = NAMCOS22_PALETTE_SIZE/16;
			tilemap = tilemap_create( TextTilemapGetInfo,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64 );
			if( tilemap )
			{
				tilemap_set_transparent_pen( tilemap, 0xf );
				dirtypal = auto_malloc(NAMCOS22_PALETTE_SIZE/4);
				if( dirtypal )
				{
					cgdirty = auto_malloc( 0x400 );
					if( cgdirty )
					{
						mPtRomSize = memory_region_length(REGION_GFX4)/3;
						mpPolyL = memory_region(REGION_GFX4);
						mpPolyM = mpPolyL + mPtRomSize;
						mpPolyH = mpPolyM + mPtRomSize;
						return 0; /* no error */
					}
				}
			}
		}
	}
	return -1; /* error */
}

VIDEO_UPDATE( namcos22s )
{
	int beamx,beamy;
	mbSuperSystem22 = 1;
	if( namcos22_gametype == NAMCOS22_TIME_CRISIS )
	{
		UpdatePalette();
	}
	else
	{
		UpdatePaletteS();
	}
	fillbitmap( bitmap, get_black_pen(), cliprect );
	namcos3d_Start( bitmap );
	DrawPolygons( bitmap );
	DrawSprites( bitmap, cliprect );
	DrawTextLayer( bitmap, cliprect );

	if( namcos22_gametype == NAMCOS22_TIME_CRISIS )
	{
		beamx = ((readinputport(1))*640)/256;
		beamy = ((readinputport(2))*480)/256;
		draw_crosshair( bitmap, beamx, beamy, cliprect );
	}
}

VIDEO_UPDATE( namcos22 )
{
	mbSuperSystem22 = 0;
	UpdatePalette();
	fillbitmap( bitmap, get_black_pen(), cliprect );
	namcos3d_Start( bitmap );
	DrawPolygons( bitmap );
	DrawTextLayer( bitmap, cliprect );
}

/* 16 bit access to DSP RAM */

static data16_t namcos22_dspram_bank;

WRITE16_HANDLER( namcos22_dspram16_bank_w )
{
	COMBINE_DATA( &namcos22_dspram_bank );
	namcos22_UploadCodeToDSP();
}

READ16_HANDLER( namcos22_dspram16_r )
{
	data32_t value = namcos22_polygonram[offset];
	switch( namcos22_dspram_bank )
	{
	case 1:
		value>>=16;
		break;

	default:
		value &= 0xffff;
//		if( offset==0xe014-0x8000 || offset==0xc014-0x8000 )
//		{
//			return 0x3;
//		}
//		if( offset==0xe034-0x8000 || offset==0xc034-0x8000 )
//		{
//			return 0x4;
//		}
/*	0e)10038	00000780 // field of view [C00E]
 *	0f)1003c	00000140                  [C00F]
 *  10)10040	000000f0                  [C010]
 *
 *	11)10048	00000000 00000000 00000000[C011..C013]
 *
 *	14)10050	00000007 // priority          [C014]
 *	15)10054	00000000 // viewport center x [C015]
 *	16)10058	00000000 // viewport center y [C016]
 *
 *	17)1005c	00000000 00007fff // rolx? [C017 C018]
 *				00000000 00007fff // roly? [C019 C01A]
 *				00000000 00007fff // rolz? [C01B C01C]
 */
		break;
	}
	return (data16_t)value;
}

WRITE16_HANDLER( namcos22_dspram16_w )
{
	data32_t value = namcos22_polygonram[offset];
	data16_t lo = value&0xffff;
	data16_t hi = value>>16;
	switch( namcos22_dspram_bank )
	{
	case 0:
		COMBINE_DATA( &lo );
		break;

	case 1:
		COMBINE_DATA( &hi );
		break;

	case 2:
		COMBINE_DATA( &lo );
		if( lo&0x8000 )
		{
			hi = 0xffff;
		}
		else
		{
			hi = 0x0000;
		}
		break;

	default:
		break;
	}
	namcos22_polygonram[offset] = (hi<<16)|lo;
	namcos22_UploadCodeToDSP();
}
