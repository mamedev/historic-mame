/*
 *	System 16 Sprite Drawing (this is #included by the video driver)
 *	zooming implemented
 *
 *	needs:
 *		clipping may not work in all cases
 *		when zoom=0, a faster draw method should be provided
 *		vflipped sprites may not work properly
 */


#define IS_OPAQUE(X) ((X+1) & 0xEE)

static void DrawNormal( struct osd_bitmap *bitmap,
	const unsigned char *data,
	const unsigned short *pal,
	int rowbytes,
	int numrows,
	int x, int y, int zoom )
{

	int delta = 0x400+zoom; // 100 0000 0000

	int width = rowbytes*2*0x3ff/delta;

	int sy, ycount = 0;
	int x0 = 0;

	if( x<0 )
	{
		x0 = -delta*x;
		width += x;
		x = 0;
	}

	if( y<0 )
	{
		ycount = -delta*y;
		numrows += y;
		y = 0;
	}

	if( x+width>320 )
	{
		width = 320-x;
	}

	if( y+numrows>224 )
	{
		numrows = 224-y;
	}

	if( width<=0 || numrows<=0 ) return; /* totally clipped */

	for( sy=y; sy<y+numrows; sy++ )
	{
		int sx, xcount = x0;
		char *dest = &bitmap->line[sy][0];
		const char *source = data+rowbytes*(ycount>>10);
		ycount+=delta;

		for( sx=x; sx<x+width; sx++ )
		{
			int offset = xcount>>10;
			unsigned char color = source[offset/2];
			if( (offset&1) ) color = color&0xf; else color = color>>4;
			if( IS_OPAQUE(color) ) dest[sx] = pal[color];
			xcount += delta;
		}
	}
}

static void DrawHflip( struct osd_bitmap *bitmap,
	const unsigned char *data,
	const unsigned short *pal,
	int rowbytes,
	int numrows,
	int x, int y, int zoom )
{

	int delta = 0x400+zoom; // 100 0000 0000

	int width = rowbytes*2*0x3ff/delta;

	int sy, ycount = 0;
	int x0 = 0;

	if( x<0 )
	{
		width += x;
		x = 0;
	}

	if( y<0 )
	{
		ycount = -delta*y;
		numrows += y;
		y = 0;
	}

	if( x+width>320 )
	{
		x0 = (x+width-320)*delta;
		width = 320-x;
	}

	if( y+numrows>224 )
	{
		numrows = 224-y;
	}

	if( width<=0 || numrows<=0 ) return; /* totally clipped */

	for( sy=y; sy<y+numrows; sy++ )
	{
		int sx, xcount = x0;
		char *dest = &bitmap->line[sy][0];
		const char *source = data+rowbytes*(ycount>>10);
		ycount+=delta;

		for( sx=x+width-1; sx>=x; sx-- )
		{
			int offset = xcount>>10;
			unsigned char color = source[offset/2];
			if( (offset&1) ) color = color&0xf; else color = color>>4;
			if( IS_OPAQUE(color) ) dest[sx] = pal[color];
			xcount += delta;
		}
	}
}

static void DrawVflip( struct osd_bitmap *bitmap,
	const unsigned char *data,
	const unsigned short *pal,
	int rowbytes,
	int numrows,
	int x, int y, int zoom )
{

	int delta = 0x400+zoom; // 100 0000 0000

	int width = rowbytes*2*0x3ff/delta;

	int sy, ycount = 0;
	int x0 = 0;

	if( x<0 )
	{
		x0 = -delta*x;
		width += x;
		x = 0;
	}

	if( y<0 )
	{
		ycount = -delta*y;
		numrows += y;
		y = 0;
	}

	if( x+width>320 )
	{
		width = 320-x;
	}

	if( y+numrows>224 )
	{
		numrows = 224-y;
	}

	if( width<=0 || numrows<=0 ) return; /* totally clipped */

	for( sy=y; sy<y+numrows; sy++ )
	{
		int sx, xcount = x0;
		char *dest = &bitmap->line[sy][0];
		const char *source = data-rowbytes*(ycount>>10);
		ycount+=delta;

		for( sx=x; sx<x+width; sx++ )
		{
			int offset = xcount>>10;
			unsigned char color = source[-(offset/2)];
			if( (offset&1) ) color = color>>4; else color = color&0xf;
			if( IS_OPAQUE(color) ) dest[sx] = pal[color];
			xcount += delta;
		}
	}
}

static void DrawHVflip( struct osd_bitmap *bitmap,
	const unsigned char *data,
	const unsigned short *pal,
	int rowbytes,
	int numrows,
	int x, int y, int zoom )
{

	int delta = 0x400+zoom; // 100 0000 0000

	int width = rowbytes*2*0x3ff/delta;

	int sy, ycount = 0;
	int x0 = 0;

	if( x<0 )
	{
		width += x;
		x = 0;
	}

	if( y<0 )
	{
		ycount = -delta*y;
		numrows += y;
		y = 0;
	}

	if( x+width>320 )
	{
		x0 = delta*(320-x);
		width = 320-x;
	}

	if( y+numrows>224 )
	{
		numrows = 224-y;
	}

	if( width<=0 || numrows<=0 ) return; /* totally clipped */

	for( sy=y; sy<y+numrows; sy++ )
	{
		int sx, xcount = x0;
		char *dest = &bitmap->line[sy][0];
		const char *source = data-rowbytes*(ycount>>10);
		ycount+=delta;

		for( sx=x+width-1; sx>=x; sx-- )
		{
			int offset = xcount>>10;
			unsigned char color = source[-(offset/2)];
			if( (offset&1) ) color = color>>4; else color = color&0xf;
			if( IS_OPAQUE(color) ) dest[sx] = pal[color];
			xcount += delta;
		}
	}
}

static void DrawSprite(struct osd_bitmap *bitmap, struct sys16_sprite_info *sprite )
{
	const unsigned char *data = Machine->memory_region[2] +
		sprite->number * 2 + (s16_obj_bank[sprite->bank] << 16);

	const unsigned short *pal = Machine->gfx[1]->colortable + sprite->color;

	int x			= sprite->horizontal_position + system16_sprxoffset;
	int y			= sprite->begin_line;
	int numrows		= sprite->end_line - y;
	int rowbytes	= sprite->width*2;

	if( sprite->vertical_flip )
	{
		if( sprite->horizontal_flip )
			DrawHVflip( bitmap,data,pal,256-rowbytes,numrows,x,y,sprite->zoom );
		else
			DrawVflip( bitmap,data,pal,256-rowbytes,numrows,x,y,sprite->zoom );
	}
	else
	{
		if( sprite->horizontal_flip )
			DrawHflip( bitmap,data+2,pal,rowbytes,numrows,x,y,sprite->zoom );
		else
			DrawNormal( bitmap,data+rowbytes,pal,rowbytes,numrows,x,y,sprite->zoom );
	}
}


