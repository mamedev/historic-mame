#include "namcos3d.h"
#include "vidhrdw/poly.h"

static UINT32 *zbuffer;

static data16_t *mpTextureTileMap16;
static data8_t *mpTextureTileMapAttr;
static data8_t *mpTextureTileData;

data8_t mXYAttrToPixel[16][16][16];

static void InitXYAttrToPixel( void )
{
	unsigned attr,x,y,ix,iy,temp;
	for( attr=0; attr<16; attr++ )
	{
		for( y=0; y<16; y++ )
		{
			for( x=0; x<16; x++ )
			{
				ix = x; iy = y;
				if( attr&4 ) ix = 15-ix;
				if( attr&2 ) iy = 15-iy;
				if( attr&8 ){ temp = ix; ix = iy; iy = temp; }
				mXYAttrToPixel[attr][x][y] = (iy<<4)|ix;
			}
		}
	}
}

int
namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM )
{
	zbuffer = auto_malloc( width*height*sizeof(UINT32) );
	if( zbuffer )
	{
		if( pTilemapROM && pTextureROM )
		{
			InitXYAttrToPixel();
			mpTextureTileMapAttr = 0x200000 + (data8_t *)pTilemapROM;
			mpTextureTileMap16 = pTilemapROM;
			#ifndef LSB_FIRST
			/* if not little endian, swap each word */
			{
				unsigned i;
				for( i=0; i<0x200000/2; i++ )
				{
					data16_t data = mpTextureTileMap16[i];
					mpTextureTileMap16[i] = (data>>8)|(data<<8);
				}
			}
			#endif
			mpTextureTileData = pTextureROM;
		}
		/*DumpTexelBMP();*/
		return 0;
	}
	return -1;
}

static unsigned texel( unsigned x, unsigned y )
{
	unsigned attr,offs;

	x &= 0xfff;  /* 256 columns, 16 pixels per tile */
	y &= 0xffff; /* (0x200000/0x200)*0x10 = 0x10000 */
	offs = (x>>4)|((y>>4)<<8);
	attr = mpTextureTileMapAttr[offs>>1];
	if( offs&1 ) attr &= 0xf; else attr >>= 4;
	return mpTextureTileData[(mpTextureTileMap16[offs]<<8)|mXYAttrToPixel[attr][x&0xf][y&0xf]];
} /* texel */

#if 0
#define BMP_H (0x8000)
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
			fputc( (i*1)&0xff,f ); // blue
			fputc( (i*3)&0xff,f ); // green
			fputc( (i*9)&0xff,f ); // red
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

void
matrix_NamcoRot( double M[4][4], const struct RotParam *pParam )
{
	switch( pParam->rolt )
	{
	case 0: /* X before Y; (player torso) */
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 1: /* X before Y; (bike) */
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 2: /* Y before X; (environment) */
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 3: /* possible Y,X,Z (Starblade) */
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	case 4: /* X before Y */
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 5:
		matrix_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	default:
		logerror( "unknown rolt:%08x\n",pParam->rolt );
		break;
	}
} /* matrix_NamcoRot */

void
namcos3d_Start( struct mame_bitmap *pBitmap )
{
	memset( zbuffer, 0xff, pBitmap->width*pBitmap->height*sizeof(UINT32) );
}

void
matrix_Multiply( double A[4][4], double B[4][4] )
{
	double temp[4][4];
	double sum;
	int row,col;
	int i;

	for( row=0;row<4;row++ )
	{
		for(col=0;col<4;col++)
		{
			sum = 0.0;
			for( i=0; i<4; i++ )
			{
				sum += A[row][i]*B[i][col];
			}
			temp[row][col] = sum;
		}
	}
	memcpy( A, temp, sizeof(temp) );
} /* matrix_Multiply */

void
matrix_Identity( double M[4][4] )
{
	int r,c;

	for( r=0; r<4; r++ )
	{
		for( c=0; c<4; c++ )
		{
			M[r][c] = (r==c)?1.0:0.0;
		}
	}
} /* matrix_Identity */

void
matrix_Translate( double M[4][4], double x, double y, double z )
{
	double m[4][4];
	m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
	m[3][0] = x;			m[3][1] = y;			m[3][2] = z;			m[3][3] = 1.0;
	matrix_Multiply( M, m );
} /* matrix_Translate*/

void
matrix_RotX( double M[4][4], double thx_sin, double thx_cos )
{
	double m[4][4];
	m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] =  thx_cos;		m[1][2] = thx_sin;		m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = -thx_sin;		m[2][2] = thx_cos;		m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	matrix_Multiply( M, m );
} /* matrix_RotX */

void
matrix_RotY( double M[4][4], double thy_sin, double thy_cos )
{
	double m[4][4];
	m[0][0] = thy_cos;		m[0][1] = 0.0;			m[0][2] = -thy_sin;		m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = thy_sin;		m[2][1] = 0.0;			m[2][2] = thy_cos;		m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	matrix_Multiply( M, m );
} /* matrix_RotY */

void
matrix_RotZ( double M[4][4], double thz_sin, double thz_cos )
{
	double m[4][4];
	m[0][0] = thz_cos;		m[0][1] = thz_sin;		m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = -thz_sin;		m[1][1] = thz_cos;		m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	matrix_Multiply( M, m );
} /* matrix_RotZ */

/*********************************************************************************************/

static void
BlitTextureSpan(
	UINT16 *pDest, UINT32 *pZBuf, const struct poly_scanline *scan, const INT64 *deltas, unsigned color)
{
	INT64 z = scan->p[0];
	INT32 tx = scan->p[1] >> 14;
	INT32 ty = scan->p[2] >> 14;
	INT64 dz = deltas[0];
	INT32 dtx = deltas[1] >> 14;
	INT32 dty = deltas[2] >> 14;
	INT32 x;

	for (x = scan->sx; x <= scan->ex; x++)
	{
		INT32 sz = z >> 16;
		if( sz < pZBuf[x] )
		{
			pZBuf[x] = sz;
			pDest[x] = texel(tx>>16,ty>>16)|color;
		}
		z += dz;
		tx += dtx;
		ty += dty;
	}
} /* BlitTextureSpan */

void
BlitTri( struct mame_bitmap *pBitmap, const struct VerTex v[3], unsigned color, double zoom )
{
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	struct poly_vertex pv[3];
	struct rectangle cliprect;
	INT32 y;//, min_z, max_z, zshift = 16;
	int i;

	cliprect.min_x = cliprect.min_y = 0;
	cliprect.max_x = pBitmap->width - 1;
	cliprect.max_y = pBitmap->height - 1;

#define USE_BACKFACE_CULLING
#ifdef USE_BACKFACE_CULLING
	if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
		(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
		(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
	{
		return; /* backface cull */
	}
#endif

	for( i=0; i<3; i++ )
	{
		if( v[i].z <=0 ) return; /* TBA: near plane clipping */
		pv[i].x = pBitmap->width/2 + v[i].x*zoom/v[i].z;
		pv[i].y = pBitmap->height/2 - v[i].y*zoom/v[i].z;
		pv[i].p[0] = v[i].z;
		pv[i].p[1] = v[i].tx << 14;
		pv[i].p[2] = v[i].ty << 14;
	}
	scans = setup_triangle_3(&pv[0], &pv[1], &pv[2], &cliprect);
	if (!scans)
		return;
	curscan = scans->scanline;
	for (y = scans->sy; y <= scans->ey; y++, curscan++)
	{
		UINT16 *pDest = (UINT16 *)pBitmap->line[y];
		UINT32 *pZBuf = zbuffer + pBitmap->width*y;
		BlitTextureSpan(pDest, pZBuf, curscan, scans->dp, color);
	}

} /* BlitTri */

static void
BlitFlatSpan(
	UINT16 *pDest, UINT32 *pZBuf, const struct poly_scanline *scan, const INT64 *deltas, unsigned color)
{
	INT64 z = scan->p[0];
	INT64 dz = deltas[0];
	INT32 x;

	for (x = scan->sx; x <= scan->ex; x++)
	{
		INT32 sz = z >> 16;
		if( sz < pZBuf[x] )
		{
			pZBuf[x] = sz;
			pDest[x] = color;
		}
		z += dz;
	}
} /* BlitFlatSpan */

void
BlitTriFlat( struct mame_bitmap *pBitmap, const struct VerTex v[3], unsigned color )
{
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	struct poly_vertex pv[3];
	struct rectangle cliprect;
	INT32 y;
	int i;

	cliprect.min_x = cliprect.min_y = 0;
	cliprect.max_x = pBitmap->width - 1;
	cliprect.max_y = pBitmap->height - 1;

#define USE_BACKFACE_CULLING
#ifdef USE_BACKFACE_CULLING
	if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
		(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
		(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
	{
		return; /* backface cull */
	}
#endif

	for( i=0; i<3; i++ )
	{
		if( v[i].z <=0 ) return; /* TBA: near plane clipping */
		/* HACK! */
		pv[i].x = pBitmap->width/2 + v[i].x*0x248/v[i].z;
		pv[i].y = pBitmap->height/2 - v[i].y*0x2a0/v[i].z;// - 32;
		pv[i].p[0] = v[i].z;
	}
	scans = setup_triangle_1(&pv[0], &pv[1], &pv[2], &cliprect);
	if (!scans)
		return;
	curscan = scans->scanline;
	for (y = scans->sy; y <= scans->ey; y++, curscan++)
	{
		UINT16 *pDest = (UINT16 *)pBitmap->line[y];
		UINT32 *pZBuf = zbuffer + pBitmap->width*y;
		BlitFlatSpan(pDest, pZBuf, curscan, scans->dp, color);
	}
} /* BlitTri */
