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

static UINT16 mCode; /* 3d primitive to render */
static namcos22_camera mCamera;

static float mViewMatrix[4][4];

#define DSP_FIXED_TO_FLOAT( X ) (((INT16)(X))*(1.0f/(float)0x7fff))

#define MAX_LIT_SURFACES 16
static struct LitSurfaceInfo
{
	struct
	{
		float x;
		float y;
		float z;
	} normal[4];
} mLitSurfaceInfo[MAX_LIT_SURFACES];
static unsigned mLitSurfaceCount;
static unsigned mLitSurfaceIndex;

#define SPRITERAM_SIZE (0x9b0000-0x980000)

#define CGRAM_SIZE 0x1e000
#define NUM_CG_CHARS ((CGRAM_SIZE*8)/(64*16)) /* 0x3c0 */

static int mPtRomSize;
static const UINT8 *mpPolyH;
static const UINT8 *mpPolyM;
static const UINT8 *mpPolyL;

#define MAX_DIRECT_POLY 16
#define DIRECT_POLY_SIZE 0x1c
static int mDirectPolyCount;
static UINT16 mDirectPolyBuf[MAX_DIRECT_POLY][DIRECT_POLY_SIZE];

static float
DspFloatToNativeFloat( UINT32 iVal )
{
	float mantissa = (iVal&0xffff);
	int exponent = (iVal>>16)&0xff;
	while( exponent<0x2e )
	{
		mantissa *= 0.5f;
		exponent++;
	}
	return mantissa;
}

void
namcos22_draw_direct_poly( const UINT16 *pSource )
{
	if( mDirectPolyCount < MAX_DIRECT_POLY )
	{
		memcpy( mDirectPolyBuf[mDirectPolyCount++], pSource, DIRECT_POLY_SIZE*sizeof(UINT16) );
	}
}

static void
DrawDirectPolys( mame_bitmap *bitmap )
{
	while( mDirectPolyCount )
	{
		const UINT16 *pSource = mDirectPolyBuf[--mDirectPolyCount];
		/**
         * 0x03a2 // 0x0fff zcode?
         * 0x0001 // 0x000f tpage?
         * 0xbd00 // color
         * 0x13a2 // flags
         *
         * 0x0100 0x009c // u,v
         * 0x0072 0xf307 // sx,sy
         * 0x602b 0x9f28 // i,zpos
         *
         * 0x00bf 0x0060 // u,v
         * 0x0040 0xf3ec // sx,sy
         * 0x602b 0xad48 // i,zpos
         *
         * 0x00fb 0x00ca // u,v
         * 0x0075 0xf205 // sx,sy
         * 0x602b 0x93e8 // i,zpos
         *
         * 0x00fb 0x00ca // u,v
         * 0x0075 0xf205 // sx,sy
         * 0x602b 0x93e8 // i,zpos
         */
		INT32 zcode = pSource[0];
//      int unk = pSource[1];
		unsigned color = pSource[2]&0x7f00;
		INT32 flags = pSource[3];
		struct VerTex v[5];
		int i;
		pSource += 4;
		for( i=0; i<4; i++ )
		{
			struct VerTex *pVerTex = &v[i];
			pVerTex->u = pSource[0];
			pVerTex->v = pSource[1];//+(tpage<<12);
			pVerTex->x = (INT16)pSource[2];
			pVerTex->y = -(INT16)pSource[3];
			pVerTex->z = DspFloatToNativeFloat( (pSource[4]<<16) | pSource[5] );
			pVerTex->i = pSource[4]>>8;
			/**
             * (x,y) are already in screen window coordinates.
             * for now, as a quick workaround, we assign an arbitrary z coordinate to all
             * four vertexes, big enough to avoid near-plane clipping.
             */
			pVerTex->z = 1000;
			pSource += 6;
		}
		//zrep = (zmin+zmax)/2.0;
		//zcode = pSource[0]+(INT32)zrep;

		mCamera.zoom = 1000; /* compensate for fake z coordinate */
		mCamera.cx = 640/2;
		mCamera.cy = 480/2;
		mCamera.clip.min_x = 0;
		mCamera.clip.min_y = 0;
		mCamera.clip.max_x = 640;
		mCamera.clip.max_y = 320;

		namcos22_BlitTri( bitmap, &v[0], color, zcode, flags, &mCamera ); /* 0,1,2 */
		v[4] = v[0]; /* wrap */
		namcos22_BlitTri( bitmap, &v[2], color, zcode, flags, &mCamera ); /* 2,3,0 */
	}
}

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

UINT32
namcos22_point_rom_r( offs_t offs )
{
	return GetPolyData(offs);
}

/* text layer uses a set of 16x16x8bpp tiles defined in RAM */
static gfx_layout cg_layout =
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

UINT32 *namcos22_cgram;
UINT32 *namcos22_textram;
UINT32 *namcos22_polygonram;
UINT32 *namcos22_gamma;
UINT32 *namcos22_vics_data;
UINT32 *namcos22_vics_control;

static int cgsomethingisdirty;
static unsigned char *cgdirty;
static unsigned char *dirtypal;
static tilemap *bg_tilemap;

static UINT8
nthbyte( const UINT32 *pSource, int offs )
{
	pSource += offs/4;
	return (pSource[0]<<((offs&3)*8))>>24;
}

static UINT16
nthword( const UINT32 *pSource, int offs )
{
	pSource += offs/2;
	return (pSource[0]<<((offs&1)*16))>>16;
}

static void TextTilemapGetInfo( int tile_index )
{
	UINT16 data = nthword( namcos22_textram,tile_index );
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
	mame_bitmap *dest_bmp,const gfx_element *gfx,
	unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
	const rectangle *clip,int transparency,int transparent_color,
	int scalex, int scaley, INT32 zcoord )
{
	rectangle myclip;
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
} /* mydrawgfxzoom */

static void
DrawSpritesHelper(
	mame_bitmap *bitmap,
	const rectangle *cliprect,
	const UINT32 *pSource,
	const UINT32 *pPal,
	int num_sprites,
	int deltax,
	int deltay,
	int bVICS)
{
	int i;
	int color,flipx,flipy;
	UINT32 xypos, size, attrs, code;
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
} /* DrawSpritesHelper */


static void
DrawSprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	/*
        0x980000:   00060000 00010000 02ff0000 000007ff
                             ^^^^                       num sprites

        0x980010:   00200020 000002ff 000007ff 00000000
                             ^^^^                       delta xpos
                                      ^^^^              delta ypos

        0x980200:   000007ff 000007ff       delta xpos, delta ypos
        0x980208:   000007ff 000007ff
        0x980210:   000007ff 000007ff
        0x980218:   000007ff 000007ff
        0x980220:   000007ff 000007ff
        0x980228:   000007ff 000007ff
        0x980230:   000007ff 000007ff
        0x980238:   000007ff 000007ff

        0x980400:   ?
        0x980600:   ?
            0000    0000.0000.0000.0000
            8000    1000.0000.0000.0000
            8080    1000.0000.1000.0000
            8880    1000.1000.1000.0000
            8888    1000.1000.1000.1000
            a888    1010.1000.1000.1000
            a8a8    1010.1000.1010.1000
            aaa8    1010.1010.1010.1000
            aaaa    1010.1010.1010.1010
            eaaa    1110.1010.1010.1010
            eaea    1110.1010.1110.1010
            eeea    1110.1110.1110.1010
            eeee    1110.1110.1110.1110
            feee    1111.1110.1110.1110
            fefe    1111.1110.1111.1110
            fffe    1111.1111.1111.1110
            ffff    1111.1111.1111.1111

        0x980800:   0000 0001 0002 0003 ... 03ff (probably indirection for sprite list)

        eight words per sprite:
        0x984000:   010f 007b   xpos, ypos
        0x984004:   0020 0020   size x, size y
        0x984008:   00ff 0311   00ff, chr x;chr y;flip x;flip y
        0x98400c:   0001 0000   sprite code, ????
        ...

        additional sorting/color data for sprite:
        0x9a0000:   C381 Z (sort)
        0x9a0004:   palette, C381 ZC (depth cueing?)
        ...
    */
	int num_sprites = (spriteram32[0x04/4]>>16)&0x3ff; /* max 1024 sprites? */
	const UINT32 *pSource = &spriteram32[0x4000/4]+num_sprites*4;
	const UINT32 *pPal = &spriteram32[0x20000/4]+num_sprites*2;
	int deltax = spriteram32[0x14/4]>>16;
	int deltay = spriteram32[0x18/4]>>16;
/*
Cyber Cycles:
The "reference" - Table is _NOT_ a reference Table ! Checked this with
Cyber Cycles & all I get is totally garbage !
To see sprites in Cyber Cycles (*HACK*):

if (game_load == CYBER_CYCLES)
{
    deltax = 0x0280;
    deltay = 0x0400;
}

And: The y-size is right, but wrong (!!!). Don't know _why_, but the
sprites have same x / y size, but they are drawn distorted. fixed this
by if (game_load == CYBER_CYCLES) sizey *= 2;

Now the Y-Position is wrong for OSD- Sprites.
You will see:
- The Game-Screen has a Skyline. Checked some real screenshots & found
out
that it starts at the bottom of the screen
- For The Position - Meter the Y-coords have to be multiplied by 2
- Same for "3-2-1-Go!".
But: The "dirt" when driving through grass or the skid seems to be
positioned well this way.
*/
	DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay, 0 );

	/* VICS RAM provides two additional banks */
	/*
        0x940000 -x------       sprite chip busy
        0x940018 xxxx----       clr.w   $940018.l

        0x940030 ----xxxx
        0x940034 xxxxxxxx       0x3070b0f
        0x940038 xxxxxxxx

        0x940040 xxxxxxxx       sprite attribute size
        0x940044 xxxxxxxx       ?
        0x940048 xxxxxxxx       sprite attribute list baseaddr
        0x94004c xxxxxxxx       ?
        0x940050 xxxxxxxx       sprite color size
        0x940054 xxxxxxxx       ?
        0x940058 xxxxxxxx       sprite color list baseaddr
        0x94005c xxxxxxxx       ?

        0x940060 xxxxxxxx       sprite attribute size
        0x940064 xxxxxxxx       ?
        0x940068 xxxxxxxx       sprite attribute list baseaddr
        0x94006c xxxxxxxx       ?
        0x940070 xxxxxxxx       sprite color size
        0x940074 xxxxxxxx       ?
        0x940078 xxxxxxxx       sprite color list baseaddr
        0x94007c xxxxxxxx       ?
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

	tilemap_set_palette_offset( bg_tilemap, nthbyte(namcos22_gamma,0x1b)*256 );

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

	tilemap_set_palette_offset( bg_tilemap, 0x7f00 );

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
DrawTextLayer( mame_bitmap *bitmap, const rectangle *cliprect )
{
	unsigned i;
	UINT32 data;

	if( cgsomethingisdirty )
	{
		for( i=0; i<64*64; i+=2 )
		{
			data = namcos22_textram[i/2];
			if( cgdirty[(data>>16)&0x3ff] )
			{
				tilemap_mark_tile_dirty( bg_tilemap,i );
			}
			if( cgdirty[data&0x3ff] )
			{
				tilemap_mark_tile_dirty( bg_tilemap,i+1 );
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
	tilemap_draw( bitmap, cliprect, bg_tilemap, 0, 0 );
} /* DrawTextLayer */

/*********************************************************************************************/

static void
TransformPoint( float *vx, float *vy, float *vz, float m[4][4] )
{
	float x = *vx;
	float y = *vy;
	float z = *vz;
	*vx = m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0];
	*vy = m[0][1]*x + m[1][1]*y + m[2][1]*z + m[3][1];
	*vz = m[0][2]*x + m[1][2]*y + m[2][2]*z + m[3][2];
}

static void
TransformNormal( float *nx, float *ny, float *nz, float m[4][4] )
{
	float x = *nx;
	float y = *ny;
	float z = *nz;
	*nx = m[0][0]*x + m[1][0]*y + m[2][0]*z;
	*ny = m[0][1]*x + m[1][1]*y + m[2][1]*z;
	*nz = m[0][2]*x + m[1][2]*y + m[2][2]*z;
}

/**
 * @brief render a single quad
 *
 * @param flags
 *      x.----.-x--.--x- (1042) (always set?)
 *      -.-xxx.----.---- (0700) representative z algorithm to use?
 *      -.----.--x-.---- (0020) one-sided poly
 *      -.----.----.---x (0001) poly/tilemap priority?
 *
 *      1163 // sky
 *      1262 // score (front)
 *      1242 // score (hinge)
 *      1243 // ?
 *      1063 // n/a
 *      1243 // various (2-sided?)
 *      1263 // everything else (1-sided?)
 *      1663 // ?
 *
 * @param color
 *      -------- xxxxxxxx unused?
 *      -xxxxxxx -------- palette select
 *      x------- -------- unknown
 */
static void
BlitQuadHelper(
		mame_bitmap *pBitmap,
		unsigned color,
		unsigned addr,
		float m[4][4],
		INT32 zcode,
		INT32 flags )
{
	struct LitSurfaceInfo *pLitSurfaceInfo = NULL;
	float zmin, zmax, zrep;
	struct VerTex v[5];
	int i;

profiler_mark(PROFILER_USER2);

//  if( (flags&0x0400) && code_pressed(KEYCODE_W) ) color = (rand()&0x7f)<<8;
//  if( (flags&0x0200) && code_pressed(KEYCODE_E) ) color = (rand()&0x7f)<<8;
//  if( (flags&0x0100) && code_pressed(KEYCODE_R) ) color = (rand()&0x7f)<<8;
//  if( (flags&0x0020) && code_pressed(KEYCODE_T) ) color = (rand()&0x7f)<<8;
//  if( (flags&0x0001) && code_pressed(KEYCODE_Y) ) color = (rand()&0x7f)<<8;

//  if( (color&0x8000) && code_pressed(KEYCODE_U) ) color = (rand()&0x7f)<<8;
//  if( (color&0x00ff) && code_pressed(KEYCODE_I) ) color = (rand()&0x7f)<<8;

	zmin = zmax = 0;

	if( code_pressed(KEYCODE_Q) )
	{
		logerror( "\tquad:zcode=%08x flags=%08x color=%08x\n", zcode, flags, color );
	}

	if( mLitSurfaceIndex < mLitSurfaceCount )
	{
		pLitSurfaceInfo = &mLitSurfaceInfo[mLitSurfaceIndex++];
	}

	for( i=0; i<4; i++ )
	{
		struct VerTex *pVerTex = &v[i];

		pVerTex->x = GetPolyData(  8+i*3+addr );
		pVerTex->y = GetPolyData(  9+i*3+addr );
		pVerTex->z = GetPolyData( 10+i*3+addr );
		pVerTex->u = GetPolyData( 0+2*i+addr )&0x0fff;
		pVerTex->v = GetPolyData( 1+2*i+addr )&0xffff;
		pVerTex->i = (GetPolyData(i+addr)>>16)&0xff;

		TransformPoint( &pVerTex->x, &pVerTex->y, &pVerTex->z, m );
		if( i==0 || pVerTex->z > zmax ) zmax = pVerTex->z;
		if( i==0 || pVerTex->z < zmin ) zmin = pVerTex->z;

		if( pLitSurfaceInfo )
		{
			/* extract normal vector */
			float nx = pLitSurfaceInfo->normal[i].x;
			float ny = pLitSurfaceInfo->normal[i].y;
			float nz = pLitSurfaceInfo->normal[i].z;

			float lx = mCamera.x;
			float ly = mCamera.y;
			float lz = mCamera.z;

			float dotproduct;

			/* transform normal vector */
			TransformNormal( &nx, &ny, &nz, m );

			/* transform light vector */
			TransformNormal( &lx, &ly, &lz, mViewMatrix );

			dotproduct = nx*ly + ny*ly + nz*lz;
			if( dotproduct>=0.0 )
			{
				pVerTex->i = mCamera.ambient + mCamera.power*dotproduct;
			}
		} /* pLitSurfaceInfo */
	} /* for( i=0; i<4; i++ ) */

	/**
     * The method for computing represenative z value may vary per polygon.
     * Possible methods include:
     * - minimum value: zmin
     * - maximum value: zmax
     * - average value: (zmin+zmax)/2
     * - average of all four z coordinates
     */
	zrep = (zmin+zmax)*0.5f; /* for now just always use the simpler average */

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

	if( namcos22_gametype == NAMCOS22_PROP_CYCLE || namcos22_gametype == NAMCOS22_TIME_CRISIS )
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
		zcode = zmax;
	}

	namcos22_BlitTri( pBitmap, &v[0], color, zcode, flags, &mCamera ); /* 0,1,2 */
	v[4] = v[0]; /* wrap */
	namcos22_BlitTri( pBitmap, &v[2], color, zcode, flags, &mCamera ); /* 2,3,0 */

profiler_mark(PROFILER_END);
} /* BlitQuadHelper */


static void
BlitQuads( mame_bitmap *pBitmap, INT32 addr, float m[4][4], INT32 base )
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
			/**
             * word 0: opcode (8a24c0)
             * word 1: flags
             * word 2: color
             */
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias = 0;
			BlitQuadHelper( pBitmap,color,addr+3,m,bias,flags );
			break;

		case 0x18:
			/**
             * word 0: opcode (0b3480 for first N-1 quads or 8b3480 for final quad in primitive)
             * word 1: flags
             * word 2: color
             * word 3: depth bias
             */
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias  = GetPolyData(addr+3);
			BlitQuadHelper( pBitmap,color,addr+4,m,bias,flags );
			break;

		case 0x10: /* vertex lighting */
			/*
            333401 (opcode)
                000000 000000 004000 (parameters; 2nd param is num 300401 following)
                000000 000000 007fff // normal vector
                000000 000000 007fff // normal vector
                000000 000000 007fff // normal vector
                000000 000000 007fff // normal vector
            */
			mLitSurfaceCount = 0;
			mLitSurfaceIndex = 0;
			if( mLitSurfaceCount < MAX_LIT_SURFACES )
			{
				struct LitSurfaceInfo *pLitSurfaceInfo = &mLitSurfaceInfo[mLitSurfaceCount++];
				int i;
				for( i=0; i<4; i++ )
				{
					pLitSurfaceInfo->normal[i].x = DSP_FIXED_TO_FLOAT(GetPolyData(addr+4+i*3+0));
					pLitSurfaceInfo->normal[i].y = DSP_FIXED_TO_FLOAT(GetPolyData(addr+4+i*3+1));
					pLitSurfaceInfo->normal[i].z = DSP_FIXED_TO_FLOAT(GetPolyData(addr+4+i*3+2));
				}
			}
			break;

		case 0x0d: /* additional normals */
			/*
            300401 (opcode)
                007b09 ffdd04 0004c2
                007a08 ffd968 0001c1
                ff8354 ffe401 000790
                ff84f7 ffdd04 0004c2
            */
			if( mLitSurfaceCount < MAX_LIT_SURFACES )
			{
				struct LitSurfaceInfo *pLitSurfaceInfo = &mLitSurfaceInfo[mLitSurfaceCount++];
				int i;
				for( i=0; i<4; i++ )
				{
					pLitSurfaceInfo->normal[i].x = DSP_FIXED_TO_FLOAT(GetPolyData(addr+1+i*3+0));
					pLitSurfaceInfo->normal[i].y = DSP_FIXED_TO_FLOAT(GetPolyData(addr+1+i*3+1));
					pLitSurfaceInfo->normal[i].z = DSP_FIXED_TO_FLOAT(GetPolyData(addr+1+i*3+2));
				}
			}
			break;

		default:
			break;
		}
		addr += size;
	}
} /* BlitQuads */

static void
BlitPolyObject( mame_bitmap *pBitmap, int code, float M[4][4] )
{
	unsigned addr1 = GetPolyData(code);
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

/*******************************************************************************/

READ32_HANDLER( namcos22_dspram_r )
{
	return namcos22_polygonram[offset];
}

WRITE32_HANDLER( namcos22_dspram_w )
{
	COMBINE_DATA( &namcos22_polygonram[offset] );
	namcos22_UploadCodeToDSP();
}

/*******************************************************************************/

/**
 * master DSP can write directly to render device via port 0xc.
 * This is used for "direct drawn" polygons, and "direct draw from point rom"
 * feature - both opcodes exist in Ridge Racer's display-list processing
 *
 * record format:
 *  header (3 words)
 *      zcode
 *      color
 *      flags
 *
 *  per-vertex data (4*6 words)
 *      u,v
 *      sx,sy
 *      intensity/z.exponent
 *      z.mantissa
 *
 * master DSP can specify 3d objects indirectly (along with view transforms),
 * via the "transmit" PDP opcode.  the "render device" sends quad data to the slave DSP
 * viewspace clipping and projection
 *
 * most "3d object" references are 0x45 and greater.  references less than 0x45 are "special"
 * commands, using a similar point rom format.  the point rom header may point to point ram.
 *
 * slave DSP reads records via port 4
 * its primary purpose is applying lighting calculations
 * the slave DSP forwards draw commands to a "draw device"
 */

/*******************************************************************************/

/**
 * 0xfffd
 * 0x0: transform
 * 0x1
 * 0x2
 * 0x5: transform
 * >=0x45: draw primitive
 */

static void
HandleBB0003( const INT32 *pSource )
{
	/*
        w.setViewport (float (320 + vx0 - vw), float (240 - vy0 - vh),
                 float (vw * 2), float (vh * 2),
                  z0, z1, margin);

        00bb0003
        001400c8            light.ambient     light.power
        00010000            ?                 light.dx
        00065a82            window priority   light.dy
        0000a57e            ?                 light.dz
        00c80081            cx,cy

        00296092            zoom = 772.5625
        001e95f8 001e95f8            0.5858154296875
        001eb079 001eb079            0.6893463134765625
        002958e8                   711.25 (see time crisis)

        7ffe 0000 0000
        0000 7ffe 0000
        0000 0000 7ffe
    */
	mCamera.power   = pSource[0x1]&0xffff;
	mCamera.ambient = pSource[0x1]>>16;

	mCamera.x       = DSP_FIXED_TO_FLOAT(pSource[0x2]);
	mCamera.y       = DSP_FIXED_TO_FLOAT(pSource[0x3]);
	mCamera.z       = DSP_FIXED_TO_FLOAT(pSource[0x4]);

	mWindowPri      = pSource[0x3]>>16;
	mCamera.zoom    = DspFloatToNativeFloat(pSource[6]);

	/* HACK! contains cx in upper 16 bits, cy in lower 16 bits; for now just ignore these polygons */
	if( pSource[5] ) mCamera.zoom = 0;
	/*
        DspFloatToNativeFloat(pSource[0x7]);
        DspFloatToNativeFloat(pSource[0x8]);

        DspFloatToNativeFloat(pSource[0x9]);
        DspFloatToNativeFloat(pSource[0xa]);

        DspFloatToNativeFloat(pSource[0xb]);
    */

	mViewMatrix[0][0] = DSP_FIXED_TO_FLOAT(pSource[0x0c]);
	mViewMatrix[1][0] = DSP_FIXED_TO_FLOAT(pSource[0x0d]);
	mViewMatrix[2][0] = DSP_FIXED_TO_FLOAT(pSource[0x0e]);

	mViewMatrix[0][1] = DSP_FIXED_TO_FLOAT(pSource[0x0f]);
	mViewMatrix[1][1] = DSP_FIXED_TO_FLOAT(pSource[0x10]);
	mViewMatrix[2][1] = DSP_FIXED_TO_FLOAT(pSource[0x11]);

	mViewMatrix[0][2] = DSP_FIXED_TO_FLOAT(pSource[0x12]);
	mViewMatrix[1][2] = DSP_FIXED_TO_FLOAT(pSource[0x13]);
	mViewMatrix[2][2] = DSP_FIXED_TO_FLOAT(pSource[0x14]);
} /* HandleBB0003 */

static void
Handle200002( mame_bitmap *bitmap, const INT32 *pSource )
{
	if( mCode>=0x45 )
	{
		float m[4][4]; /* row major */

		matrix3d_Identity( m );

		m[0][0] = DSP_FIXED_TO_FLOAT(pSource[0x1]);
		m[1][0] = DSP_FIXED_TO_FLOAT(pSource[0x2]);
		m[2][0] = DSP_FIXED_TO_FLOAT(pSource[0x3]);

		m[0][1] = DSP_FIXED_TO_FLOAT(pSource[0x4]);
		m[1][1] = DSP_FIXED_TO_FLOAT(pSource[0x5]);
		m[2][1] = DSP_FIXED_TO_FLOAT(pSource[0x6]);

		m[0][2] = DSP_FIXED_TO_FLOAT(pSource[0x7]);
		m[1][2] = DSP_FIXED_TO_FLOAT(pSource[0x8]);
		m[2][2] = DSP_FIXED_TO_FLOAT(pSource[0x9]);

		m[3][0] = pSource[0xa]; /* xpos */
		m[3][1] = pSource[0xb]; /* ypos */
		m[3][2] = pSource[0xc]; /* zpos */

		matrix3d_Multiply( m, mViewMatrix );
		BlitPolyObject( bitmap, mCode, m );
	}
} /* Handle200002 */

static void
Handle300000( const INT32 *pSource )
{ /* set view transform */
	mViewMatrix[0][0] = DSP_FIXED_TO_FLOAT(pSource[1]);
	mViewMatrix[1][0] = DSP_FIXED_TO_FLOAT(pSource[2]);
	mViewMatrix[2][0] = DSP_FIXED_TO_FLOAT(pSource[3]);

	mViewMatrix[0][1] = DSP_FIXED_TO_FLOAT(pSource[4]);
	mViewMatrix[1][1] = DSP_FIXED_TO_FLOAT(pSource[5]);
	mViewMatrix[2][1] = DSP_FIXED_TO_FLOAT(pSource[6]);

	mViewMatrix[0][2] = DSP_FIXED_TO_FLOAT(pSource[7]);
	mViewMatrix[1][2] = DSP_FIXED_TO_FLOAT(pSource[8]);
	mViewMatrix[2][2] = DSP_FIXED_TO_FLOAT(pSource[9]);
} /* Handle300000 */

static void
Handle233002( const INT32 *pSource )
{ /* set modal rendering options */
//  mMasterBias = pSource[2];
/*
    00233002 (always?)
    00000000
    0003dd00 // z bias adjust
    001fffff
    00007fff 00000000 00000000
    00000000 00007fff 00000000
    00000000 00000000 00007fff
    00000000 00000000 00000000
*/
} /* Handle233002 */

static void
SimulateSlaveDSP( mame_bitmap *bitmap, int bDebug )
{
	const INT32 *pSource = 0x300 + (INT32 *)namcos22_polygonram;
	INT16 len;

	mCamera.cx  = 320;
	mCamera.cy  = 240;
	mCamera.clip.min_x = 0;
	mCamera.clip.min_y = 0;
	mCamera.clip.max_x = NAMCOS22_NUM_COLS*16;
	mCamera.clip.max_y = NAMCOS22_NUM_ROWS*16;

	matrix3d_Identity( mViewMatrix );

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
			logerror( "[code=0x%04x; len=0x%02x] ", mCode, len );
			for( i=0; i<len; i++ )
			{
				logerror( " %08x", pSource[i] );
			}
			logerror( "\n" );
		}

		switch( len )
		{
		case 0x15:
			HandleBB0003( pSource ); /* define viewport */
			break;

		case 0x10:
			Handle233002( pSource ); /* set modal rendering options */
			break;

		case 0x0a:
			Handle300000( pSource ); /* modify view transform */
			break;

		case 0x0d:
			Handle200002( bitmap, pSource ); /* render primitive */
			break;

		default:
			return;
		}

		/* hackery! commands should be streamed by PDP, not parsed here */
		pSource += len;
		marker = (INT16)*pSource++; /* always 0xffff */
		next   = (INT16)*pSource++; /* link to next command */
		if( (next&0x7fff) != (pSource - (INT32 *)namcos22_polygonram) )
		{ /* end of list */
			break;
		}
	} /* for(;;) */
} /* SimulateSlaveDSP */

static void
DrawPolygons( mame_bitmap *bitmap )
{
	if( mbDSPisActive )
	{
		int bDebug = 0;
#ifdef MAME_DEBUG
		bDebug = code_pressed(KEYCODE_Q);
#endif
		SimulateSlaveDSP( bitmap, bDebug );

		if( bDebug )
		{
			int i;
			for( i=0x10000; i<0x20000; i+=4 )
			{
				if( (i&0x7f)==0 ) logerror( "\n%08x: ", i );
				logerror( " %08x", namcos22_polygonram[i/4] );
			}
			while( code_pressed(KEYCODE_Q) ){}
		}
		DrawDirectPolys(bitmap);
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
    +0x0002.w   Fader Enable(?) (0: disabled)
    +0x0011.w   Display Fader (R) (0x0100 = 1.0)
    +0x0013.w   Display Fader (G) (0x0100 = 1.0)
    +0x0015.w   Display Fader (B) (0x0100 = 1.0)
    +0x0100.b   Fog1 Color (R) (world fogging)
    +0x0101.b   Fog2 Color (R) (used for heating of brake-disc on RV1)
    +0x0180.b   Fog1 Color (G)
    +0x0181.b   Fog2 Color (G)
    +0x0200.b   Fog1 Color (B)
    +0x0201.b   Fog2 Color (B)
*/

/**
 * 0x800000: 08380000
 *
 * 0x810000: 0004 0004 0004 0004 4444
 *           ^^^^ ^^^^ ^^^^ ^^^^      fog thickness? 0 if disabled
 *
 * 0x810200..0x8103ff: 0x100 words (depth cueing table)
 *      0000 0015 002a 003f 0054 0069 007e 0093
 *      00a8 00bd 00d2 00e7 00fc 0111 0126 013b
 *
 * 0x810400..0x810403: (air combat22?)
 * 0x820000..0x8202ff: ?
 *
 * 0x824000..0x8243ff: gamma
 *   Prop Cycle: start of stage#2
 *   ffffff00 00ffffff 0000007f 00000000
 *   0000ff00 0f00ffff ff00017f 00010007
 *   00000001
 *
 *   Prop Cycle: submerged in swamp
 *   ffffff00 0003ffae 0000007f 00000000
 *   0000ff00 0f00ffff ff00017f 00010007
 *   00000001
 *
 * 0x860000..0x860007: ?
 */
WRITE32_HANDLER( namcos22_gamma_w )
{
	UINT32 old = namcos22_gamma[offset];
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
	tilemap_mark_tile_dirty( bg_tilemap, offset*2 );
	tilemap_mark_tile_dirty( bg_tilemap, offset*2+1 );
}


VIDEO_START( namcos22s )
{
	mbDSPisActive = 0;
	memset( namcos22_polygonram, 0xcc, 0x20000 );
/**
 * +0x00 0  always 0xf?
 * +0x04 1  sin(x) world-view matrix
 * +0x08 2  cos(x)
 * +0x0c 3  sin(y)
 * +0x10 4  cos(y)
 * +0x14 5  sin(z)
 * +0x18 6  cos(z)
 * +0x1c 7  ROLT? always 0x0002?
 * +0x20 8  light power
 * +0x24 9  light ambient
 * +0x28 10 light vector(x)
 * +0x2c 11 light vector(y)
 * +0x30 12 light vector(z)
 * +0x34 13 always 0x0002?
 * +0x38 14 field of view angle (fovx) in degrees
 * +0x3c 15 viewport width
 * +0x40 16 viewport height
 * +0x44 17
 * +0x48 18
 * +0x4c 19
 * +0x50 20 priority (top 7 > .. > 0 behind)
 * +0x54 21 viewport center x
 * +0x58 22 viewport center y
 * +0x5c 23 sin(x) ?
 * +0x60 24 cos(x)
 * +0x64 25 sin(y)
 * +0x68 26 cos(y)
 * +0x6c 27 sin(z)
 * +0x70 28 cos(z)
 * +0x74 29 flags (axis flipping?) 0004, 0003
 * +0x78 30 6630, 0001
 * +0x7c 31 7f02
 */
	/*
    namcos22_polygonram[0x100] = 0x7fff;
    namcos22_polygonram[0x101] = 0x0000;
    namcos22_polygonram[0x102] = 0x0000;
    namcos22_polygonram[0x103] = 0x0000;
    namcos22_polygonram[0x104] = 0x7fff;
    namcos22_polygonram[0x105] = 0x0000;
    namcos22_polygonram[0x106] = 0x0000;
    namcos22_polygonram[0x107] = 0x0000;
    namcos22_polygonram[0x108] = 0x7fff;

    namcos22_polygonram[0x112] = 0x3c; // fov
    namcos22_polygonram[0x113] = 0x0; // xsize exp
    namcos22_polygonram[0x114] = 0x0; // xsize mantissa

    namcos22_polygonram[0x119] = 0;// (45d3)
    namcos22_polygonram[0x11a] = 0;// (45d5)
    namcos22_polygonram[0x11b] = 0;// 0x4294
    namcos22_polygonram[0x11c] = 0;// 0x4294

    namcos22_polygonram[0x120] = 0x00c8;
    namcos22_polygonram[0x121] = 0x0014;
    namcos22_polygonram[0x122] = 0x0000;
    namcos22_polygonram[0x123] = 0x5a82;
    namcos22_polygonram[0x123] = 0xa57e;
    namcos22_polygonram[0x124] = 0x0002; // 0x4291
    namcos22_polygonram[0x125] = 0x0140;
    namcos22_polygonram[0x126] = 0x00f0;
    namcos22_polygonram[0x127] = 0x0000;
    namcos22_polygonram[0x128] = 0x0000;
    namcos22_polygonram[0x129] = 0x0000;
    namcos22_polygonram[0x12a] = 0x0007;
    namcos22_polygonram[0x12b] = 0x0000;
    namcos22_polygonram[0x12c] = 0x0000;
    namcos22_polygonram[0x12d] = 0x0000;
    namcos22_polygonram[0x12e] = 0x7fff;
    namcos22_polygonram[0x12f] = 0x0000;
    namcos22_polygonram[0x130] = 0x7fff;
    namcos22_polygonram[0x131] = 0x0000;
    namcos22_polygonram[0x132] = 0x7fff; // 0x4293
    namcos22_polygonram[0x133] = 0x0002; // 0x429b
*/
	if( namcos3d_Init(
		NAMCOS22_SCREEN_WIDTH,
		NAMCOS22_SCREEN_HEIGHT,
		memory_region(REGION_GFX3),	/* tilemap */
		memory_region(REGION_GFX2)	/* texture */
	) == 0 )
	{
		gfx_element *pGfx = decodegfx( (UINT8 *)namcos22_cgram,&cg_layout );
		if( pGfx )
		{
			Machine->gfx[NAMCOS22_ALPHA_GFX] = pGfx;
			pGfx->colortable = Machine->remapped_colortable;
			pGfx->total_colors = NAMCOS22_PALETTE_SIZE/16;
			bg_tilemap = tilemap_create( TextTilemapGetInfo,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64 );
			if( bg_tilemap )
			{
				tilemap_set_transparent_pen( bg_tilemap, 0xf );
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
profiler_mark(PROFILER_USER1);
	DrawPolygons( bitmap );
profiler_mark(PROFILER_END);
	DrawSprites( bitmap, cliprect );
	DrawTextLayer( bitmap, cliprect );

	if( namcos22_gametype == NAMCOS22_TIME_CRISIS )
	{
		beamx = ((readinputport(1))*640)/256;
		beamy = ((readinputport(2))*480)/256;
		draw_crosshair( bitmap, beamx, beamy, cliprect, 0 );
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

static UINT16 namcos22_dspram_bank;

WRITE16_HANDLER( namcos22_dspram16_bank_w )
{
	COMBINE_DATA( &namcos22_dspram_bank );
}

static UINT16 mUpperWordLatch;

READ16_HANDLER( namcos22_dspram16_r )
{
	UINT32 value = namcos22_polygonram[offset];

	switch( namcos22_dspram_bank )
	{
	case 0:
		value &= 0xffff;
		break;

	case 1:
		value>>=16;
		break;

	case 2:
		mUpperWordLatch = value>>16;
		value &= 0xffff;
		break;

	default:
		break;
	}
	return (UINT16)value;
}

WRITE16_HANDLER( namcos22_dspram16_w )
{
	UINT32 value = namcos22_polygonram[offset];
	UINT16 lo = value&0xffff;
	UINT16 hi = value>>16;
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
		hi = mUpperWordLatch;
		break;

	default:
		break;
	}
	namcos22_polygonram[offset] = (hi<<16)|lo;
}
