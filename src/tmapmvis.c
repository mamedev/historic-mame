/*
	Do not put this file in the makefile.

	The following procedure body is #included several times by
	tilemap.c to implement a set of mark_visible routines.

	The constants TILE_WIDTH and TILE_HEIGHT are different in
	each instance of this code, allowing fast arithmetic shifts to
	be used by the compiler instead of multiplies/divides.
*/

{
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ){ /* do nothing if totally clipped */
		int c1,c2; /* leftmost and rightmost visible columns in source tilemap */
		int r1,r2;
		char **visible_row;
		int span;
		int row;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		r1 = y1/TILE_HEIGHT;
		r2 = (y2+TILE_HEIGHT-1)/TILE_HEIGHT;

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */
		visible_row = blit.visible_row;
		span = c2-c1;

		for( row=r1; row<r2; row++ ){
			memset( visible_row[row]+c1, 1, span );
		}
	}
}

