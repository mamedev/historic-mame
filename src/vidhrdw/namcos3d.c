#include "namcos3d.h"
#include "matrix3d.h"
#include "includes/namcos22.h"
#include <math.h>

#define MIN_Z (1)

/*
Renderer:
    each pixel->(BN,TX,TY,BRI,FLAGS,CZ,PAL)
        BN:    texture bank
        TX,TY: texture coordinates
        BRI:   brightness for gouraud shading
        FLAGS: specifies whether affected by depth cueing
        CZ:    index for depth cueing

Shader:
    (TX,TY,BN)->COL
    (PAL,COL)->palette index
    (palette index,BRI)->RGB

Depth BIAS:
    notes: blend function is expressed by a table
    it may be possible to specify multiple backcolors
    RGB,CZ,BackColor->RGB'

Mixer:
    (RGB',gamma table)->RGB''

    Other techniques:
        bump mapping perturbation (BX,BY)
        normal vector on polygon surface for lighting
        (normal vector, perturbation) -> N
        (lighting,N) -> BRI


        BRI=IaKa+{II/(Z+K)}.times.(Kd cost .phi.+Ks cos.sup.n.psi.) (1)

        Ia: Intensity of ambient light;
        II: Intensity of incident light;
        Ka: Diffuse reflection coefficient of ambient light [0];
        Kd: Diffuse reflection coefficient [0];
        Ks: Specular reflection coefficient [0];
        (a: ambient)
        (d: diffuse)
        (s: specular)
        K: Constant (for correcting the brightness in a less distant object [F];
        Z: Z-axis coordinate for each dot [0 in certain cases];
        .phi.: Angle between a light source vector L and a normal vector N;
        =Angle between a reflective light vector R and a normal vector N;
        .psi.: Angle between a reflective light vector R and a visual vector E =[0, 0, 1]; and
        n: Constant (sharpness in high-light) [0]
        [F]: Constant for each scene (field).
        [O]: Constant for each object (or polygon).
*/

INT32 *namco_zbuffer; /* TBR: Namco System 21 and System22 use sorting, not a real ZBuffer */

static data16_t *mpTextureTileMap16;
static data8_t *mpTextureTileMapAttr;
static data8_t *mpTextureTileData;
static data8_t mXYAttrToPixel[16][16][16];
static unsigned texel( unsigned x, unsigned y );

static void
InitXYAttrToPixel( void )
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

static void
PatchTexture( void )
{
	int i;

	switch( namcos22_gametype )
	{
	case NAMCOS22_RIDGE_RACER:
	case NAMCOS22_RIDGE_RACER2:
	case NAMCOS22_ACE_DRIVER:
	case NAMCOS22_CYBER_COMMANDO:
		break;

	default:
		return;
	}

	for( i=0; i<0x100000; i++ )
	{
		int tile = mpTextureTileMap16[i];
		int attr = mpTextureTileMapAttr[i];
		if( (attr&0x1)==0 )
		{
			tile = (tile&0x3fff)|0x8000;
			mpTextureTileMap16[i] = tile;
		}
	}
}

int
namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM )
{
	namco_zbuffer = auto_malloc( width*height*sizeof(*namco_zbuffer) );
	if( namco_zbuffer )
	{
		if( pTilemapROM && pTextureROM )
		{ /* following setup is Namco System 22 specific */
			int i;
			const data8_t *pPackedTileAttr = 0x200000 + (data8_t *)pTilemapROM;
			data8_t *pUnpackedTileAttr = auto_malloc(0x080000*2);
			if( pUnpackedTileAttr )
			{
				InitXYAttrToPixel();
				mpTextureTileMapAttr = pUnpackedTileAttr;
				for( i=0; i<0x80000; i++ )
				{
					*pUnpackedTileAttr++ = (*pPackedTileAttr)>>4;
					*pUnpackedTileAttr++ = (*pPackedTileAttr)&0xf;
					pPackedTileAttr++;
				}

				mpTextureTileMap16 = pTilemapROM;

				#ifndef LSB_FIRST
				/* if not little endian, byteswap each word */
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
				PatchTexture(); /* HACK! */

				if( 0 )
				{
					int tpage;
					for( tpage=0; tpage<0x10; tpage++ )
					{
						int ty;
						char fname[32];
						FILE *fOut;
						sprintf( fname, "tpage-%x.bmp", tpage );
						fOut = fopen( fname, "wb" );
						if( fOut )
						{
							FILE *fIn = fopen( "template.bmp", "rb" );
							if( fIn )
							{
								int i;
								for( i=0; i<14+40; i++ )
								{
									int dat = fgetc( fIn );
									fputc( dat, fOut );
								}
								fclose( fIn );

								for( i=0; i<256; i++ )
								{
									fputc( 0x00, fOut );
									fputc( i, fOut );
									fputc( (i*3)&0xff, fOut );
									fputc( (i*7)&0xff, fOut );
								}

								for( ty=0xfff; ty>=0; ty--)
								{
									int tx;
									for( tx=0; tx<0x1000; tx++ )
									{
										int color = texel(tx,ty+tpage*0x1000);
										fputc( color, fOut );
									}
								}
							}
							fclose( fOut );
						}
					}
				}
			} /* pUnpackedTileAttr */
		}
		return 0;
	}
	return -1;
}

void
namcos3d_Rotate( double M[4][4], const struct RotParam *pParam )
{
	switch( pParam->rolt )
	{
	case 0:
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 1:
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 2:
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 3:
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	case 4:
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 5:
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	default:
		logerror( "unknown rolt:%08x\n",pParam->rolt );
		break;
	}
} /* namcos3d_Rotate */

void
namcos3d_Start( struct mame_bitmap *pBitmap )
{
	memset( namco_zbuffer, 0x7f, pBitmap->width*pBitmap->height*sizeof(*namco_zbuffer) );
}

/*********************************************************************************************/

typedef struct
{
	double x,y;
	double u,v,i,z;
} vertex;

typedef struct
{
	double x;
	double u,v,i,z;
} edge;

#define SWAP(A,B) { const void *temp = A; A = B; B = temp; }

static unsigned mColor;
static INT32 mZSort;

static unsigned texel( unsigned x, unsigned y )
{
	unsigned offs = ((y&0xfff0)<<4)|((x&0xff0)>>4);
	unsigned tile = mpTextureTileMap16[offs];
	return mpTextureTileData[(tile<<8)|mXYAttrToPixel[mpTextureTileMapAttr[offs]][x&0xf][y&0xf]];
} /* texel */

typedef void drawscanline_t( const edge *e1, const edge *e2, int sy, const struct rectangle *clip );

static void
renderscanline_flat( const edge *e1, const edge *e2, int sy, const struct rectangle *clip )
{
	struct mame_bitmap *pBitmap = Machine->scrbitmap;

	if ((sy < 0) || (sy >= pBitmap->height))
	{
		logerror ("sy (%d) is bogus, bitmap height: %d", sy, pBitmap->height);
		return;
	}

	if( e1->x > e2->x )
	{
		SWAP(e1,e2);
	}

	{
		UINT16 *pDest = (UINT16 *)pBitmap->line[sy];
		INT32 *pZBuf = namco_zbuffer + pBitmap->width*sy;

		int x0 = (int)e1->x;
		int x1 = (int)e2->x;
		int w = x1-x0;
		if( w )
		{
			double z = e1->z; /* 1/z */
			double dz = (e2->z - e1->z)/w;
			int x, crop;
			crop = clip->min_x - x0;
			if( crop>0 )
			{
				z += crop*dz;
				x0 = clip->min_x;
			}
			if (x0<0) x0 = 0;
			if( x1>clip->max_x )
			{
				x1 = clip->max_x;
			}

			for( x=x0; x<x1; x++ )
			{
				if( mZSort<pZBuf[x] )
				{
					pDest[x] = mColor;
					pZBuf[x] = mZSort;
				}
			}
		}
	}
}

static void
renderscanline_uvi( const edge *e1, const edge *e2, int sy, const struct rectangle *clip )
{
	struct mame_bitmap *pBitmap = Machine->scrbitmap;

	if ((sy < 0) || (sy >= pBitmap->height))
	{
		logerror ("sy (%d) is bogus, bitmap height: %d", sy, pBitmap->height);
		return;
	}

	if( e1->x > e2->x )
	{
		SWAP(e1,e2);
	}

	{
		UINT32 *pDest = (UINT32 *)pBitmap->line[sy];
		INT32 *pZBuf = namco_zbuffer + pBitmap->width*sy;

		int x0 = (int)e1->x;
		int x1 = (int)e2->x;
		int w = x1-x0;
		if( w )
		{
			double u = e1->u; /* u/z */
			double v = e1->v; /* v/z */
			double i = e1->i; /* i/z */
			double z = e1->z; /* 1/z */
			double du = (e2->u - e1->u)/w;
			double dv = (e2->v - e1->v)/w;
			double dz = (e2->z - e1->z)/w;
			double di = (e2->i - e1->i)/w;
			int x, crop;
			int baseColor = mColor&0x7f00;

			crop = clip->min_x - x0;
			if( crop>0 )
			{
				u += crop*du;
				v += crop*dv;
				i += crop*di;
				z += crop*dz;
				x0 = clip->min_x;
			}
			if (x0<0) x0 = 0;
			if( x1>clip->max_x )
			{
				x1 = clip->max_x;
			}

			for( x=x0; x<x1; x++ )
			{
				if( mZSort<pZBuf[x] )
				{
					UINT32 color = Machine->pens[texel(u/z,v/z)|baseColor];
					int r = color>>16;
					int g = (color>>8)&0xff;
					int b = color&0xff;
					int shade = i/z;
					r+=shade; if( r<0 ) r = 0; else if( r>0xff ) r = 0xff;
					g+=shade; if( g<0 ) g = 0; else if( g>0xff ) g = 0xff;
					b+=shade; if( b<0 ) b = 0; else if( b>0xff ) b = 0xff;
					pDest[x] = (r<<16)|(g<<8)|b;
					pZBuf[x] = mZSort;
				}
				u += du;
				v += dv;
				i += di;
				z += dz;
			}
		}
	}
}

/**
 * rendertri is a (temporary?) replacement for the scanline conversion that used to be done in poly.c
 * rendertri uses floating point arithmetic
 */
static void
rendertri( const vertex *v0, const vertex *v1, const vertex *v2, const struct rectangle *clip, drawscanline_t pdraw )
{
	int dy,ystart,yend,crop;

	/* first, sort so that v0->y <= v1->y <= v2->y */
	for(;;)
	{
		if( v0->y > v1->y )
		{
			SWAP(v0,v1);
		}
		else if( v1->y > v2->y )
		{
			SWAP(v1,v2);
		}
		else
		{
			break;
		}
	}

	ystart = v0->y;
	yend   = v2->y;
	dy = yend-ystart;
	if( dy )
	{
		int y;
		edge e1; /* short edge (top and bottom) */
		edge e2; /* long (common) edge */

		double dx2dy = (v2->x - v0->x)/dy;
		double du2dy = (v2->u - v0->u)/dy;
		double dv2dy = (v2->v - v0->v)/dy;
		double di2dy = (v2->i - v0->i)/dy;
		double dz2dy = (v2->z - v0->z)/dy;

		double dx1dy;
		double du1dy;
		double dv1dy;
		double di1dy;
		double dz1dy;

		e2.x = v0->x;
		e2.u = v0->u;
		e2.v = v0->v;
		e2.i = v0->i;
		e2.z = v0->z;
		crop = clip->min_y - ystart;
		if( crop>0 )
		{
			e2.x += dx2dy*crop;
			e2.u += du2dy*crop;
			e2.v += dv2dy*crop;
			e2.i += di2dy*crop;
			e2.z += dz2dy*crop;
		}

		ystart = v0->y;
		yend = v1->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v0->x;
			e1.u = v0->u;
			e1.v = v0->v;
			e1.i = v0->i;
			e1.z = v0->z;

			dx1dy = (v1->x - v0->x)/dy;
			du1dy = (v1->u - v0->u)/dy;
			dv1dy = (v1->v - v0->v)/dy;
			di1dy = (v1->i - v0->i)/dy;
			dz1dy = (v1->z - v0->z)/dy;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if (ystart< 0) ystart = 0;
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				pdraw( &e1,&e2,y, clip );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}

		ystart = v1->y;
		yend = v2->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v1->x;
			e1.u = v1->u;
			e1.v = v1->v;
			e1.i = v1->i;
			e1.z = v1->z;

			dx1dy = (v2->x - v1->x)/dy;
			du1dy = (v2->u - v1->u)/dy;
			dv1dy = (v2->v - v1->v)/dy;
			di1dy = (v2->i - v1->i)/dy;
			dz1dy = (v2->z - v1->z)/dy;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if (ystart< 0) ystart = 0;
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				pdraw( &e1,&e2,y, clip );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}
	}
} /* rendertri */

static void
ProjectPoint( const struct VerTex *v, vertex *pv, const namcos22_camera *camera )
{
	pv->x = camera->cx + v->x*camera->zoom/v->z;
	pv->y = camera->cy - v->y*camera->zoom/v->z;
	pv->u = (v->u+0.5)/v->z;
	pv->v = (v->v+0.5)/v->z;
	pv->i = (v->i+0.5 - 0x40)/v->z;
//  pv->i = (v->i+0.5)/v->z;
	pv->z = 1/v->z;
}

static void
BlitTriHelper(
		struct mame_bitmap *pBitmap,
		const struct VerTex *v0,
		const struct VerTex *v1,
		const struct VerTex *v2,
		unsigned color,
		const namcos22_camera *camera )
{
	vertex a,b,c;
	ProjectPoint( v0,&a,camera );
	ProjectPoint( v1,&b,camera );
	ProjectPoint( v2,&c,camera );
	mColor = color;
	rendertri( &a, &b, &c, &camera->clip, renderscanline_uvi );
}

static double
interp( double x0, double ns3d_y0, double x1, double ns3d_y1 )
{
	double m = (ns3d_y1-ns3d_y0)/(x1-x0);
	double b = ns3d_y0 - m*x0;
	return m*MIN_Z+b;
}

static int
VertexEqual( const struct VerTex *a, const struct VerTex *b )
{
	return
		a->x == b->x &&
		a->y == b->y &&
		a->z == b->z;
}

/**
 * BlitTri is used by Namco System22 to draw texture-mapped, perspective-correct triangles.
 */
void
namcos22_BlitTri(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color, INT32 zsort, INT32 flags,
	const namcos22_camera *camera )
{
	struct VerTex vc[3];
	int i,j;
	int iBad = 0, iGood = 0;
	int bad_count = 0;

	/* don't bother rendering a degenerate triangle */
	if( VertexEqual(&v[0],&v[1]) ) return;
	if( VertexEqual(&v[0],&v[2]) ) return;
	if( VertexEqual(&v[1],&v[2]) ) return;

	if( flags&0x0020 ) /* one-sided */
	{
		if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
			(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
			(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
		{
			return; /* backface cull */
		}
	}

	for( i=0; i<3; i++ )
	{
		if( v[i].z<MIN_Z )
		{
			bad_count++;
			iBad = i;
		}
		else
		{
			iGood = i;
		}
	}

	mZSort = zsort;

	switch( bad_count )
	{
	case 0:
		BlitTriHelper( pBitmap, &v[0],&v[1],&v[2], color, camera );
		break;

	case 1:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iBad+1)%3;
		vc[iBad].x = interp( v[i].z,v[i].x, v[iBad].z,v[iBad].x  );
		vc[iBad].y = interp( v[i].z,v[i].y, v[iBad].z,v[iBad].y  );
		vc[iBad].u = interp( v[i].z,v[i].u, v[iBad].z,v[iBad].u );
		vc[iBad].v = interp( v[i].z,v[i].v, v[iBad].z,v[iBad].v );
		vc[iBad].i = interp( v[i].z,v[i].i, v[iBad].z,v[iBad].i );
		vc[iBad].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );

		j = (iBad+2)%3;
		vc[i].x = interp(v[j].z,v[j].x, v[iBad].z,v[iBad].x  );
		vc[i].y = interp(v[j].z,v[j].y, v[iBad].z,v[iBad].y  );
		vc[i].u = interp(v[j].z,v[j].u, v[iBad].z,v[iBad].u );
		vc[i].v = interp(v[j].z,v[j].v, v[iBad].z,v[iBad].v );
		vc[i].i = interp(v[j].z,v[j].i, v[iBad].z,v[iBad].i );
		vc[i].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );
		break;

	case 2:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iGood+1)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].u = interp(v[iGood].z,v[iGood].u, v[i].z,v[i].u );
		vc[i].v = interp(v[iGood].z,v[iGood].v, v[i].z,v[i].v );
		vc[i].i = interp(v[iGood].z,v[iGood].i, v[i].z,v[i].i );
		vc[i].z = MIN_Z;

		i = (iGood+2)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].u = interp(v[iGood].z,v[iGood].u, v[i].z,v[i].u );
		vc[i].v = interp(v[iGood].z,v[iGood].v, v[i].z,v[i].v );
		vc[i].i = interp(v[iGood].z,v[iGood].i, v[i].z,v[i].i );
		vc[i].z = MIN_Z;

		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );
		break;

	case 3:
		/* wholly clipped */
		break;
	}
} /* BlitTri */

static void
Namcos21ProjectPoint( const struct VerTex *v, vertex *pv, const namcos21_camera *camera )
{
	pv->x = camera->cx + v->x*camera->zoomx/v->z;
	pv->y = camera->cy - v->y*camera->zoomy/v->z;
}

static void
BlitFlatTriHelper(
		struct mame_bitmap *pBitmap,
		const struct rectangle *cliprect,
		const struct VerTex *v0,
		const struct VerTex *v1,
		const struct VerTex *v2,
		const namcos21_camera *camera )
{
	vertex a,b,c;
	Namcos21ProjectPoint( v0, &a, camera );
	Namcos21ProjectPoint( v1, &b, camera );
	Namcos21ProjectPoint( v2, &c, camera );
	rendertri( &a, &b, &c, cliprect, renderscanline_flat );
}

/**
 * BlitTriFlat is used by Namco System21 to draw flat-shaded triangles.
 *
 * TBA: merge further with the triangle-rendering code used by Namco System22
 */
void
namcos21_BlitTriFlat( struct mame_bitmap *pBitmap, const struct rectangle *cliprect, const struct VerTex v[3], unsigned color, INT32 zsort, const namcos21_camera *camera )
{
	struct VerTex vc[3];
	int i,j;
	int iBad = 0, iGood = 0;
	int bad_count = 0;

	/* don't bother rendering a degenerate triangle */
	if( VertexEqual(&v[0],&v[1]) ) return;
	if( VertexEqual(&v[0],&v[2]) ) return;
	if( VertexEqual(&v[1],&v[2]) ) return;

	if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
		(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
		(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
	{
		return; /* backface cull */
	}


	mZSort = zsort;
	mColor = color;

	for( i=0; i<3; i++ )
	{
		if( v[i].z<MIN_Z )
		{
			bad_count++;
			iBad = i;
		}
		else
		{
			iGood = i;
		}
	}

	switch( bad_count )
	{
	case 0:
		BlitFlatTriHelper( pBitmap, cliprect, &v[0],&v[1],&v[2], camera );
		break;

	case 1:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iBad+1)%3;
		vc[iBad].x = interp( v[i].z,v[i].x, v[iBad].z,v[iBad].x  );
		vc[iBad].y = interp( v[i].z,v[i].y, v[iBad].z,v[iBad].y  );
		vc[iBad].z = MIN_Z;
		BlitFlatTriHelper( pBitmap, cliprect, &vc[0],&vc[1],&vc[2], camera );

		j = (iBad+2)%3;
		vc[i].x = interp(v[j].z,v[j].x, v[iBad].z,v[iBad].x  );
		vc[i].y = interp(v[j].z,v[j].y, v[iBad].z,v[iBad].y  );
		vc[i].z = MIN_Z;
		BlitFlatTriHelper( pBitmap, cliprect, &vc[0],&vc[1],&vc[2], camera );
		break;

	case 2:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iGood+1)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].z = MIN_Z;

		i = (iGood+2)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].z = MIN_Z;

		BlitFlatTriHelper( pBitmap, cliprect, &vc[0],&vc[1],&vc[2], camera );
		break;

	case 3:
		/* wholly clipped */
		break;
	}
} /* BlitTri */
