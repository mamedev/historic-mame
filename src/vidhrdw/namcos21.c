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
#include "namcos2.h"
#include "namcoic.h"
#include <math.h>

#define MAX_SURFACE 64
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


static void
draw_tri( struct mame_bitmap *pBitmap, struct vertex pVertex[3], UINT16 pen )
{
	INT32 xshort,xlong,dxdyshort,dxdylong;
	INT32 sy,bottom,clip,sx,width,height;
	int iTop, iMid, iBot, i, iTemp;
	UINT16 *pDest;

	iTop = 0;
	iMid = 1;
	iBot = 2;
	for( i=0; i<2; i++ )
	{
		if( pVertex[iMid].ypos < pVertex[iTop].ypos )
		{
			iTemp = iMid;
			iMid = iTop;
			iTop = iTemp;
		}
		if( pVertex[iBot].ypos < pVertex[iMid].ypos )
		{
			iTemp = iMid;
			iMid = iBot;
			iBot = iTemp;
		}
	}

	xshort = xlong = pVertex[iTop].xpos<<16;
	sy = pVertex[iTop].ypos;
	height = pVertex[iBot].ypos-sy;
	if( height == 0 )
	{
		dxdylong = 0;
	}
	else
	{
		dxdylong = ((pVertex[iBot].xpos<<16) - xlong)/height;
	}
	i = iMid;
	for(;;)
	{
		bottom = pVertex[i].ypos;
		height = bottom - sy;
		if( height == 0 )
		{
			dxdyshort = 0;
		}
		else
		{
			dxdyshort = ((pVertex[i].xpos<<16) - xshort)/height;
		}
		if( sy<0 )
		{
			clip = -sy;
			if( clip>bottom-sy )
			{
				clip = bottom-sy;
			}
			xshort += dxdyshort*clip;
			xlong += dxdylong*clip;
			sy+=clip;
		}
		if( bottom>pBitmap->height )
		{
			bottom = pBitmap->height;
		}
		while( sy<bottom )
		{
			if( xshort<xlong )
			{
				sx = xshort>>16;
				width = (xlong-xshort)>>16;
			}
			else
			{
				sx = xlong>>16;
				width = (xshort-xlong)>>16;
			}
			if( sx<0 )
			{
				width += sx;
				sx = 0;
			}
			if( sx+width>pBitmap->width )
			{
				width = pBitmap->width-sx;
			}
			pDest = sx+(UINT16*)pBitmap->line[sy];
			while( width>0 )
			{
				*pDest++ = pen;
				width--;
			}
			xshort += dxdyshort;
			xlong += dxdylong;
			sy++;
		} /* while( sy<bottom ) */
		if( i==iMid )
		{
			i = iBot;
		}
		else
		{
			break;
		}
	}
}

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
		/* The first table entry doesn't reference a polyobject.
		 * Rather, it contains the first address beyond the polyobject lookup table.
		 * This address in turn contains a linked list (header information?)
		 *	In Starblade:
		 *		000000:	0018e3
		 *		0018e3:	0048cd 0048cf 0048d1 0048d3 ------
		 *		0048cd:	000001 000000
		 *		0048cf:	000001 000001
		 *		0048d1:	000001 000002
		 *		0048d3:	000001 000003
		 */
		if( code >= pPointData[0] )
		{
			/* out of range */
			continue;
		}

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

		MasterAddr = pPointData[code];
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
			VertexCount	= pPointData[SubAddr++]&0xff;
			unk2		= pPointData[SubAddr++];

			if( bDebug ) logerror( "NumWords=%08x; unk1=%08x; unk2=%08x; VertexCount=%d\n",
				NumWords,unk,unk2,VertexCount );

			if( VertexCount>MAX_VERTEX )
			{
				logerror( "Max Vertex exceeded(%d) %08x;%08x\n", code,MasterAddr,SubAddr);
				return;
			}

			for( count=0; count<VertexCount; count++ )
			{
				x = (INT16)(pPointData[SubAddr++]&0xffff);
				y = (INT16)(pPointData[SubAddr++]&0xffff);
				z = (INT16)(pPointData[SubAddr++]&0xffff);
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

			SurfaceCount = pPointData[SubAddr++]&0xff;
			if( SurfaceCount>MAX_SURFACE )
			{
				logerror( "Max Surface exceeded(%d)\n", SurfaceCount );
				return;
			}
			for( count=0; count<SurfaceCount; count++ )
			{
				if( bDebug ) logerror( "Surface#%d: ",count );
				if( SubAddr>=0x400000/4 ) return;

				vi[0] = pPointData[SubAddr++]&0xff;
				vi[1] = pPointData[SubAddr++]&0xff;
				vi[2] = pPointData[SubAddr++]&0xff;
				vi[3] = pPointData[SubAddr++]&0xff;
				color = pPointData[SubAddr++]&0xff; /* ignore for now */
				if( bDebug ) logerror( "%d,%d,%d,%d; color=%d\n", vi[0],vi[1],vi[2],vi[3],color );
				draw_quad( bitmap,vertex,vi,Machine->pens[0x1000]/*color*/ );
			} /* next surface */
		} /* next component */
	} /* next object */
} /* draw_polygons */

static int objcode2tile( int code )
{
	return code;
}

VIDEO_START( namcos21 )
{
	namco_obj_init(
		0,		/* gfx bank */
		0xf,	/* reverse palette mapping */
		objcode2tile );

	/*	int i;
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

VIDEO_UPDATE( namcos21_default )
{
	int i,pri;
	data16_t data1,data2;
	int r,g,b;

	/* stuff the palette */
	for( i=0; i<NAMCOS21_NUM_COLORS; i++ )
	{
		data1 = paletteram16[0x00000/2+i];
		data2 = paletteram16[0x10000/2+i];

		r = data1>>8;
		g = data1&0xff;
		b = data2&0xff;

		palette_set_color( i, r,g,b );
	}

	/* paint background */
	fillbitmap( bitmap,Machine->pens[0xff],NULL );

	for( pri=0; pri<8; pri++ )
	{
		namco_obj_draw( bitmap,pri );
	}

	draw_polygons( bitmap );

	if(0){
		struct vertex Pt[3];
		Pt[0].xpos = 64;
		Pt[0].ypos = 0;
		Pt[1].xpos = 128;
		Pt[1].ypos = 64;
		Pt[2].xpos = 0;
		Pt[2].ypos = 128;
		draw_tri( bitmap, Pt, Machine->pens[0x1004] );
	}
}
