/*
	Do not put this file in the makefile.

	The following procedure body is #included several times by
	tilemap.c to implement a set of tilemap_draw_opaque routines.

	The constants TILE_WIDTH and TILE_HEIGHT are different in
	each instance of this code, allowing fast arithmetic shifts to
	be used by the compiler instead of multiplies/divides.

	This routine should be fairly optimal, for C code.

	It renders pixels one row at a time, skipping over runs of totally
	transparent tiles, and calling custom blitters to handle runs of
	masked/totally opaque tiles.
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
		unsigned char priority = blit.priority;

		unsigned char *dest_baseaddr, *dest_next;
		const unsigned char *source_baseaddr, *source_next;

		int c1,c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		dest_baseaddr = blit.screen->line[y1] + xpos;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = blit.pixmap->line[y1];

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */

		y = y1;
		y_next = TILE_HEIGHT*(y1/TILE_HEIGHT) + TILE_HEIGHT;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*blit.dest_line_offset;
			source_next = source_baseaddr + dy*blit.source_line_offset;
		}

		for(;;){
			int row = y/TILE_HEIGHT;
			char *priority_data = blit.priority_data_row[row];

			unsigned char tile_type, prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1, x_end;

			int column;
			for( column=c1; column<=c2; column++ ){
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = TILE_OPAQUE;

				if( tile_type!=prev_tile_type ){
					x_end = column*TILE_WIDTH;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT ){
						/* TILE_OPAQUE */
						int num_pixels = x_end - x_start;
						unsigned char *dest0 = dest_baseaddr+x_start;
						const unsigned char *source0 = source_baseaddr+x_start;
						int i = y;
						for(;;){
							memcpy( dest0, source0, num_pixels );
							if( ++i == y_next ) break;

							dest0 += blit.dest_line_offset;
							source0 += blit.source_line_offset;
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			dest_baseaddr = dest_next;
			source_baseaddr = source_next;

			y = y_next;
			y_next += TILE_HEIGHT;

			if( y_next>=y2 ){
				y_next = y2;
			}
			else {
				dest_next += blit.dest_row_offset;
				source_next += blit.source_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
}

