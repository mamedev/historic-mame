#include "driver.h" /* for mame_bitmap */

struct VerTex
{
	double x,y,z;
	UINT32 tx,ty;
};

struct RotParam
{
	double thx_sin, thx_cos;
	double thy_sin, thy_cos;
	double thz_sin, thz_cos;
	int rolt; /* 0..5 */
};

void matrix_NamcoRot( double M[4][4], const struct RotParam *pParam );

void matrix_Multiply( double A[4][4], double B[4][4] );
void matrix_Identity( double M[4][4] );
void matrix_Translate( double M[4][4], double x, double y, double z );
void matrix_RotX( double M[4][4], double thx_sin, double thx_cos );
void matrix_RotY( double M[4][4], double thy_sin, double thy_cos );
void matrix_RotZ( double M[4][4], double thz_sin, double thz_cos );

int namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM );

void namcos3d_Start( struct mame_bitmap *pBitmap );


void BlitTri(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color,
	double zoom );

void BlitTriFlat(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color );
