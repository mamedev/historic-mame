/***************************************************************************

Namco System 21 Video Hardware

Sprite Hardware is identical to Namco System NB1

Polygons are rendered by DSP Processors.
The behavior in this driver is based on an ongoing study of Air Combat and Starblade.
It is possible that other games on this hardware use different DSP programs.

0x200000..0x2000ff	populated with ASCII Text during self-tests:
	ROM:
	RAM:
	PTR:
	SMU:
	IDC:
	CPU:ABORT
	DSP:
	CRC:OK  from cpu
	CRC:    from dsp
	ID:
	B-M:	P-M:	S-M:	SET UP

0x200202	current page
0x200206	page select

Page#0
0x208000..0x2080ff	camera attributes
0x208200..0x208fff	3d object attribute list

Page#1
0x20c000..0x2080ff	camera attributes
0x20c200..0x208fff	3d object attribute list

To do:
- remaining camera attributes
- clipping
- filled polygons & zbuffer

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"
#include "namcos21.h"
#include <math.h>

#define MAX_SURFACE 512
#define MAX_VERTEX	64
struct vertex
{
	double x,y,z;
	int xpos, ypos;
	int bValid;
};

static void draw_pixel( struct mame_bitmap *bitmap, int sx, int sy, int color )
{
	((UINT16 *)bitmap->line[sy])[sx] = color;
}

#define kScreenWidth	62*8
#define kScreenHeight	60*8
static int onscreen( int x,int y )
{
	return x>=0 && y>=0 && x<kScreenWidth && y<kScreenHeight;
}

static void draw_line( struct mame_bitmap *bitmap, int xpos1, int ypos1, int xpos2, int ypos2, int color )
{
	int dx,dy,sx,sy,cx,cy;

	if( onscreen(xpos1,ypos1) && onscreen(xpos2,ypos2) ) /* all-or-nothing clip */
	{
		dx = xpos2 - xpos1;
		dy = ypos2 - ypos1;

		dx = abs(dx);
		dy = abs(dy);
		sx = (xpos1 <= xpos2) ? 1: -1;
		sy = (ypos1 <= ypos2) ? 1: -1;
		cx = dx/2;
		cy = dy/2;

		if (dx>=dy)
		{
			for (;;)
			{
				draw_pixel( bitmap, xpos1, ypos1, color );
				if (xpos1 == xpos2) break;
				xpos1 += sx;
				cx -= dy;
				if (cx < 0)
				{
					ypos1 += sy;
					cx += dx;
				}
			}
		}
		else
		{
			for (;;)
			{
				draw_pixel( bitmap, xpos1, ypos1, color );
				if (ypos1 == ypos2) break;
				ypos1 += sy;
				cy -= dx;
				if (cy < 0)
				{
					xpos1 += sx;
					cy += dy;
				}
			}
		}
	}
}

static void draw_quad( struct mame_bitmap *bitmap, struct vertex *vertex, int vi[4], int color )
{
	int i;
	struct vertex *pv1, *pv2;

	/* should be flat shaded; for now just draw wireframe */
	pv1 = &vertex[vi[3]];
	for( i=0; i<4; i++ )
	{
		pv2 = &vertex[vi[i]];
		if( pv1->bValid && pv2->bValid )
		{
			draw_line( bitmap, pv1->xpos,pv1->ypos,pv2->xpos,pv2->ypos,color );
		}
		pv1 = pv2;
	}
}

#define MATRIX_SIZE 4

/* A := A*B */
static void
multiply_matrix(
	double A[MATRIX_SIZE][MATRIX_SIZE],
	double B[MATRIX_SIZE][MATRIX_SIZE] )
{
	double temp[MATRIX_SIZE][MATRIX_SIZE];
	double sum;
	int row,col;
	int i;

	for( row=0;row<MATRIX_SIZE;row++ )
	{
		for(col=0;col<MATRIX_SIZE;col++)
		{
			sum = 0.0;
			for( i=0; i<MATRIX_SIZE; i++ )
			{
				sum += A[row][i]*B[i][col];
			}
			temp[row][col] = sum;
		}
	}
	memcpy( A, temp, sizeof(temp) );
}

static void draw_polygons( struct mame_bitmap *bitmap )
{
	int i;
	const INT16 *pDSPRAM;
	const data32_t *pPointData;
	data16_t enable,code,depth;
	INT16 xpos0,ypos0,zpos0;
	double thx_sin,thx_cos;
	double thy_sin,thy_cos;
	double thz_sin,thz_cos;
	double m[MATRIX_SIZE][MATRIX_SIZE];
	double M[MATRIX_SIZE][MATRIX_SIZE];
	double camera_matrix[MATRIX_SIZE][MATRIX_SIZE];
	UINT32 MasterAddr,SubAddr;
	UINT32 NumWords, VertexCount, SurfaceCount;
	struct vertex vertex[MAX_VERTEX], *pVertex;
	int vi[4];
	INT32 x,y,z;
	int color;
	UINT32 count;
	data16_t unk,unk2;
	int bDebug;

	bDebug = keyboard_pressed( KEYCODE_D );

//	namcos21_dspram16[0x202/2] |= 1;

	if( namcos21_dspram16[0x206/2]&1 )
	{
		pDSPRAM = (INT16 *)&namcos21_dspram16[0xc000/2];
	}
	else
	{
		pDSPRAM = (INT16 *)&namcos21_dspram16[0x8000/2];
	}

	if( bDebug ) logerror( "\nDSPRAM:\n" );

/*
Camera Attributes
	The first chunk might be a pre-multiplier.
	The second is definitely paramaters for a final multiplier.

0x00	0001 0001 1c71
	    [  THX  ] [  THY  ] [  THZ  ]
		0000 7fff 0000 7fff 0000 7fff <- haven't seen these change
		*000 *000
		0000 0000
		00f8 00f2
		0003 0004
		0108 0064

		[  THX  ] [  THY  ] [  THZ  ]
0x40	EACF 7E3B 8F1E C3AA 0000 7FFF
		0002 0004
		000F FFA0
		02F0 02F0
		0000 0017?

*/
	thx_sin =  pDSPRAM[0x20]/(double)0x7fff;
	thx_cos =  pDSPRAM[0x21]/(double)0x7fff;

	thy_sin =  pDSPRAM[0x22]/(double)0x7fff;
	thy_cos =  pDSPRAM[0x23]/(double)0x7fff;

	thz_sin =  pDSPRAM[0x24]/(double)0x7fff;
	thz_cos =  pDSPRAM[0x25]/(double)0x7fff;

	if( bDebug )
	{
		logerror( "thx=%f,%f; thy=%f,%f; thz=%f,%f\n",thx_sin,thx_cos,thy_sin,thy_cos,thz_sin,thz_cos );
	}

/*
	0xc046	0002 0000
	0xc048	0000 ffa0
	0xc04a	02f0 02f0
	0xc04c	0000 fffc
*/
	xpos0 = 0;
	ypos0 = 0;
	zpos0 = 0;

	/* identity */
	m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	memcpy( camera_matrix, m, sizeof(m) );

	/* rotate x-axis */
	m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] =  thx_cos;		m[1][2] = thx_sin;		m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = -thx_sin;		m[2][2] = thx_cos;		m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	multiply_matrix(camera_matrix,m);

	/* rotate y-axis */
	m[0][0] = thy_cos;		m[0][1] = 0.0;			m[0][2] = -thy_sin;		m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = thy_sin;		m[2][1] = 0.0;			m[2][2] = thy_cos;		m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	multiply_matrix(camera_matrix,m);

	/* rotate z-axis */
	m[0][0] = thz_cos;		m[0][1] = thz_sin;		m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = -thz_sin;		m[1][1] = thz_cos;		m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
	m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
	multiply_matrix(camera_matrix,m);

	/* translate */
	m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
	m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
	m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
	m[3][0] = xpos0;		m[3][1] = ypos0;		m[3][2] = zpos0;		m[3][3] = 1.0;
	multiply_matrix(camera_matrix,m);

	pPointData = (data32_t *)memory_region( REGION_USER2 );

	for( i=0x200/2; i<0x500/2; i+=13 )
	{
		enable	= (UINT16)pDSPRAM[i+0x0];

		if( enable == 0xffff )
		{
			/* end of object list */
			if( bDebug ) logerror( "END-OF-LIST\n" );
			return;
		}

		code	= 1 + (UINT16)pDSPRAM[i+0x1];
		depth	= pDSPRAM[i+0x2]; /* always zero? */

		xpos0	= pDSPRAM[i+0x3];
		ypos0	= pDSPRAM[i+0x4];
		zpos0	= pDSPRAM[i+0x5];

		thx_sin =  pDSPRAM[i+0x6]/(double)0x7fff;
		thx_cos =  pDSPRAM[i+0x7]/(double)0x7fff;

		thy_sin =  pDSPRAM[i+0x8]/(double)0x7fff;
		thy_cos =  pDSPRAM[i+0x9]/(double)0x7fff;

		thz_sin =  pDSPRAM[i+0xa]/(double)0x7fff;
		thz_cos =  pDSPRAM[i+0xb]/(double)0x7fff;

		unk	= pDSPRAM[i+0xc]; /* always 4? */

		if( bDebug ) logerror( "code=%04x depth=%04x unk=%04x (%d,%d,%d)\n",
			code,depth,unk,zpos0,ypos0,zpos0 );

		/* identity */
		M[0][0] = 1.0;			M[0][1] = 0.0;			M[0][2] = 0.0;			M[0][3] = 0.0;
		M[1][0] = 0.0;			M[1][1] = 1.0;			M[1][2] = 0.0;			M[1][3] = 0.0;
		M[2][0] = 0.0;			M[2][1] = 0.0;			M[2][2] = 1.0;			M[2][3] = 0.0;
		M[3][0] = 0.0;			M[3][1] = 0.0;			M[3][2] = 0.0;			M[3][3] = 1.0;

		/* rotate x-axis */
		m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
		m[1][0] = 0.0;			m[1][1] =  thx_cos;		m[1][2] = thx_sin;		m[1][3] = 0.0;
		m[2][0] = 0.0;			m[2][1] = -thx_sin;		m[2][2] = thx_cos;		m[2][3] = 0.0;
		m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
		multiply_matrix(M,m);

		/* rotate y-axis */
		m[0][0] = thy_cos;		m[0][1] = 0.0;			m[0][2] = -thy_sin;		m[0][3] = 0.0;
		m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
		m[2][0] = thy_sin;		m[2][1] = 0.0;			m[2][2] = thy_cos;		m[2][3] = 0.0;
		m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
		multiply_matrix(M,m);

		/* rotate z-axis */
		m[0][0] = thz_cos;		m[0][1] = thz_sin;		m[0][2] = 0.0;			m[0][3] = 0.0;
		m[1][0] = -thz_sin;		m[1][1] = thz_cos;		m[1][2] = 0.0;			m[1][3] = 0.0;
		m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
		m[3][0] = 0.0;			m[3][1] = 0.0;			m[3][2] = 0.0;			m[3][3] = 1.0;
		multiply_matrix(M,m);

		/* translate */
		m[0][0] = 1.0;			m[0][1] = 0.0;			m[0][2] = 0.0;			m[0][3] = 0.0;
		m[1][0] = 0.0;			m[1][1] = 1.0;			m[1][2] = 0.0;			m[1][3] = 0.0;
		m[2][0] = 0.0;			m[2][1] = 0.0;			m[2][2] = 1.0;			m[2][3] = 0.0;
		m[3][0] = xpos0;		m[3][1] = ypos0;		m[3][2] = zpos0;		m[3][3] = 1.0;
		multiply_matrix(M,m);

		multiply_matrix(M,camera_matrix);

		MasterAddr = pPointData[code/*&0x7ff*/];
		if( MasterAddr>=0x400000/4 || MasterAddr == 0 )
		{
			logerror( "bad code\n" );
			return;
		}

		for(;;)
		{
			SubAddr = pPointData[MasterAddr++];
			if( bDebug ) logerror( "Master=%08x; SubAddr=%08x\n", MasterAddr,SubAddr );

			if( ~SubAddr == 0 ) break;
			if( SubAddr>=0x400000/4 ) return;
			if( MasterAddr>=0x400000/4 ) return;

			NumWords	= pPointData[SubAddr++];
			unk 		= pPointData[SubAddr++];
			VertexCount	= pPointData[SubAddr++];
			unk2		= pPointData[SubAddr++];

			if( bDebug ) logerror( "NumWords=%08x; unk1=%08x; unk2=%08x; VertexCount=%d\n",
				NumWords,unk,unk2,VertexCount );

			if( VertexCount>MAX_VERTEX )
			{
				logerror( "Max Vertex exceeded(%d)\n", VertexCount );
				return;
			}

			for( count=0; count<VertexCount; count++ )
			{
				x = pPointData[SubAddr++];
				y = pPointData[SubAddr++];
				z = pPointData[SubAddr++];
				if( bDebug ) logerror( "Vertex#%d: (%d,%d,%d)\n", count,x,y,z );

				pVertex = &vertex[count];
				pVertex->x = M[0][0]*x + M[1][0]*y + M[2][0]*z + M[3][0];
				pVertex->y = M[0][1]*x + M[1][1]*y + M[2][1]*z + M[3][1];
				pVertex->z = M[0][2]*x + M[1][2]*y + M[2][2]*z + M[3][2];

				/* Project to screen coordinates. */
				if( pVertex->z>1 )
				{
					pVertex->xpos = 0x2f0*pVertex->x/pVertex->z + bitmap->width/2;
					pVertex->ypos = -0x2f0*pVertex->y/pVertex->z + bitmap->height/2;
					pVertex->bValid = 1;
				}
				else
				{
					pVertex->bValid = 0;
				}

			} /* next vertex */

			SurfaceCount = pPointData[SubAddr++];
			if( SurfaceCount>MAX_SURFACE )
			{
				logerror( "Max Surface exceeded(%d)\n", SurfaceCount );
				return;
			}
			for( count=0; count<SurfaceCount; count++ )
			{
				if( bDebug ) logerror( "Surface#%d: ",count );
				if( SubAddr>=0x400000/4 ) return;

				vi[0] = pPointData[SubAddr++];
				vi[1] = pPointData[SubAddr++];
				vi[2] = pPointData[SubAddr++];
				vi[3] = pPointData[SubAddr++];
				color = pPointData[SubAddr++]; /* ignore for now */
				if( bDebug ) logerror( "%d,%d,%d,%d; color=%d\n", vi[0],vi[1],vi[2],vi[3],color );
				draw_quad( bitmap,vertex,vi,Machine->pens[0x1000]/*color*/ );
			} /* next surface */
		} /* next component */
	} /* next object */
} /* draw_polygons */

int namcos21_vh_start( void )
{
/*
	int i;
	UINT32 *pMem;

	pMem = (UINT32 *)memory_region(REGION_USER2);
	for( i=0; i<0x400000/4; i++ )
	{
		if( (i&0xf)==0 ) logerror( "\n%08x: ", i );
		logerror( "%06x ", pMem[i]&0xffffff );
	}
*/
	return 0;
}

void namcos21_vh_stop( void )
{
}

static void draw_sprite( int page, struct mame_bitmap *bitmap, const data16_t *pSource )
{
	INT16 hpos,vpos;
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

	linkno		= pSource[0]; /* LINKNO */
	offset		= pSource[1]; /* OFFSET */
	hpos		= pSource[2]; /* HPOS */
	vpos		= pSource[3]; /* VPOS */
	hsize		= pSource[4]; /* HSIZE		max 0x3ff pixels */
	vsize		= pSource[5]; /* VSIZE		max 0x3ff pixels */
	palette		= pSource[6]; /* PALETTE	max 0xf */


	/* hack; adjust global sprite positions */
	if( namcos21_gametype == NAMCOS21_CYBERSLED )
	{
		hpos -= 256+8+8;
		vpos -= 2+32;
	}
	else if( namcos21_gametype == NAMCOS21_AIRCOMBAT )
	{
		vpos -= 34;
		hpos -= 2;
	}
	else
	{
		/* Starblade */
		if( page )
		{
			hpos -= 128;
			vpos -= 64;
		}
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

	for( row=0; row<num_rows; row++ )
	{
		sy = row*tile_screen_height + vpos;
		for( col=0; col<num_cols; col++ )
		{
			tile = spritetile16[tile_index++];
			if( (tile&0x8000)==0 )
			{
				sx = col*tile_screen_width + hpos;

				drawgfxzoom(bitmap,Machine->gfx[0],
					tile + offset,
					0x10 + 0xf - (palette&0xf),
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy );
			}
		}
	}
}

static void draw_sprites( struct mame_bitmap *bitmap )
{
	data16_t *spritelist16;
	data16_t which;
	int i;
	int count;

	/* sprite list#1 */
	spritelist16 = &spriteram16[0x2000/2];
	/* count the sprites */
	count = 0;
	for( i=0; i<256; i++ )
	{
		which = spritelist16[i];
		count++;
		if( which&0x100 ) break;
	}
	/* draw the sprites */
	for( i=count-1; i>=0; i-- )
	{
		which = spritelist16[count-i-1];
		draw_sprite( 0, bitmap, &spriteram16[(which&0xff)*8] );
	}

	/* sprite list#2 */
	spritelist16 = &spriteram16[0x14000/2];
	/* count the sprites */
	count = 0;
	for( i=0; i<256; i++ )
	{
		which = spritelist16[i];
		count++;
		if( which&0x100 ) break;
	}
	/* draw the sprites */
	for( i=count-1; i>=0; i-- )
	{
		which = spritelist16[count-i-1];
		draw_sprite( 1, bitmap, &spriteram16[(which&0xff)*8 + 0x10000/2] );
	}
}

void namcos21_vh_update_default( struct mame_bitmap *bitmap, int fullrefresh )
{
	int i;
	data16_t data1,data2;
	int r,g,b;

	/* stuff the palette */
	for( i=0; i<NAMCOS21_NUM_COLORS; i++ )
	{
		data1 = spriteram16[0x40000/2+i];
		data2 = spriteram16[0x50000/2+i];

		r = data1>>8;
		g = data1&0xff;
		b = data2&0xff;

		palette_set_color( i, r,g,b );
	}

	/* paint background */
	fillbitmap( bitmap,Machine->pens[0xff],NULL );

	draw_sprites( bitmap );

	draw_polygons( bitmap );
}
