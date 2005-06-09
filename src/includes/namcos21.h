/**
 * @file namcos21.h
 */

#define NAMCOS21_POLY_FRAME_WIDTH 496
#define NAMCOS21_POLY_FRAME_HEIGHT 480

extern void namcos21_ClearPolyFrameBuffer( void );
extern void namcos21_DrawQuad( int sx[4], int sy[4], int zcode[4], int color );
