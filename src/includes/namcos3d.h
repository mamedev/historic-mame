#include "driver.h" /* for mame_bitmap */

#define NAMCOS22_SCREEN_WIDTH  640
#define NAMCOS22_SCREEN_HEIGHT 480

extern INT32 *namco_zbuffer;

struct VerTex
{
	float x,y,z;
	float u,v,i;
};

typedef struct
{
	rectangle clip;
	float zoom, cx, cy;
	float x,y,z; /* unit vector for light direction */
	float ambient; /* 0.0..1.0 */
	float power;	/* 0.0..1.0 */
} namcos22_camera;

typedef struct
{
	float zoomx, zoomy, cx, cy;
} namcos21_camera;

struct RotParam
{
	float thx_sin, thx_cos;
	float thy_sin, thy_cos;
	float thz_sin, thz_cos;
	int rolt; /* rotation type: 0,1,2,3,4,5 */
};

void namcos3d_Rotate( float M[4][4], const struct RotParam *pParam );

int namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM );

void namcos3d_Start( mame_bitmap *pBitmap );

void namcos22_BlitTri(
	mame_bitmap *pBitmap,
	/* const rectangle *cliprect, */
	const struct VerTex v[3],
	unsigned color,
	INT32 zsort,
	INT32 flags,
	const namcos22_camera * );

