#include "driver.h"
#include "vidhrdw/generic.h"

#define NAMCO_S1_VIDEORAM_REGION 6
#define NAMCO_S1_RAM_REGION 5

/* direct drawing background and foreground */
#define NAMCOS1_DIRECT_DRAW

#define VIDEORAM_SIZE 0x800 /* this is for a single screen, no scroll */

#define MAX_PLAYFIELDS 6

struct playfield {
	void					*base;
	int						priority;
	int						palette;
	int						scroll_x;
	int						scroll_y;
#ifdef NAMCOS1_DIRECT_DRAW
	int						width;
	int						height;
#else
	struct osd_bitmap		*bitmap;
	void					*dirty;
#endif
};

struct playfield playfields[MAX_PLAYFIELDS];

static unsigned char *char_trans;

void namcos1_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom) {
	int i;

	for( i = 0; i < Machine->drv->total_colors; i++ ) {
		palette[i*3+0] = 0;
		palette[i*3+1] = 0;
		palette[i*3+2] = 0;
		colortable[i] = i;
	}
}

static void dirty_playfields( void ) {
#ifdef NAMCOS1_DIRECT_DRAW
#else
	memset( dirtybuffer, 1, 0x8000 );
#endif
}

int namcos1_vh_start( void ) {
	int i;

	videoram = Machine->memory_region[NAMCO_S1_VIDEORAM_REGION];

#ifdef NAMCOS1_DIRECT_DRAW
#else
	dirtybuffer = malloc( 0x8000 );
	if ( dirtybuffer == 0 )
		return 1;

	dirty_playfields();
#endif

	for ( i = 0; i < MAX_PLAYFIELDS; i++ ) {
#ifdef NAMCOS1_DIRECT_DRAW
		if ( i < 4 ) {
			playfields[i].base = &videoram[i<<13];
			if ( i == 3 ) {
				playfields[i].width  = 64*8;
				playfields[i].height = 32*8;
			} else {
				playfields[i].width  = 64*8;
				playfields[i].height = 64*8;
			}
		} else {
			playfields[i].base = &videoram[0x7000+( ( i - 4 ) * VIDEORAM_SIZE )];
			playfields[i].width  = Machine->drv->screen_width;
			playfields[i].height = Machine->drv->screen_height;
		}
#else
		if ( i < 4 ) {
			playfields[i].base = &videoram[i<<13];
			playfields[i].dirty = &dirtybuffer[i<<13];
			if ( i == 3 ) {
				playfields[i].bitmap = osd_new_bitmap( 64*8, 32*8, Machine->scrbitmap->depth );
			} else {
				playfields[i].bitmap = osd_new_bitmap( 64*8, 64*8, Machine->scrbitmap->depth );
			}
		} else {
			playfields[i].base = &videoram[0x7000+( ( i - 4 ) * VIDEORAM_SIZE )];
			playfields[i].dirty = &dirtybuffer[0x7000+( ( i - 4 ) * VIDEORAM_SIZE )];
			playfields[i].bitmap = osd_new_bitmap( Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth );
		}

		if ( playfields[i].bitmap == 0 ) {
			free( dirtybuffer );
			return 1;
		}
#endif
		playfields[i].palette = 0;
		playfields[i].priority = 0x0f;
		playfields[i].scroll_x = 0;
		playfields[i].scroll_y = 0;
	}

#ifdef NAMCOS1_DIRECT_DRAW
	/* build char mask status table */
	{
		const struct GfxElement *mask = Machine->gfx[0];
		const struct GfxElement *pens = Machine->gfx[1];
		int total  = mask->total_elements;
		int width  = mask->width;
		int height = mask->height;
		int line,x,c;

		char_trans = malloc( total );
		if(char_trans == 0 )
			return 1;

		for(c=0;c<total;c++)
		{
			unsigned char ordata = 0;
			unsigned char anddata = 0xff;

			for(line=0;line<height;line++)
			{
				unsigned char  *maskbm = mask->gfxdata->line[c*height+line];
				for (x=0;x<width;x++)
				{
					ordata  |= maskbm[x];
					anddata &= maskbm[x];
				}
			}
			if( !ordata )     char_trans[c]=0;
			else if(anddata ) char_trans[c]=1;
			else              char_trans[c]=2;

			if( !ordata )     char_trans[c]=0; /* blank */
			else if(anddata ) char_trans[c]=1; /* fill  */
			else
			{
				/* search non used pen */
				unsigned char penmap[256];
				unsigned char trans_pen;
				memset(penmap,0,256);
				for(line=0;line<height;line++)
				{
					unsigned char  *pensbm = pens->gfxdata->line[c*height+line];
					for (x=0;x<width;x++)
						penmap[pensbm[x]]=1;
				}
				for(trans_pen=2;trans_pen<256;trans_pen++)
				{
					if(!penmap[trans_pen]) break;
				}
				char_trans[c]=trans_pen; /* transparency color */
				/* fill transparency color */
				for(line=0;line<height;line++)
				{
					unsigned char  *maskbm = pens->gfxdata->line[c*height+line];
					unsigned char  *pensbm = pens->gfxdata->line[c*height+line];
					for (x=0;x<width;x++)
					{
						if(!maskbm[x]) pensbm[x] = trans_pen;
					}
				}
			}
		}
	}
#endif
	return 0;
}

void namcos1_vh_stop( void ) {
	videoram = 0;

#ifdef NAMCOS1_DIRECT_DRAW
	free(char_trans);
#else
	free( dirtybuffer );

	for ( i = 0; i < MAX_PLAYFIELDS; i++ )
		osd_free_bitmap( playfields[i].bitmap );
#endif
}

/* per game scroll adjustment */
static int scrolloffsX[4];
static int scrolloffsY[4];
static int scrollneg;

void namcos1_set_scroll_offsets( int *bgx, int*bgy, int negative ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		scrolloffsX[i] = bgx[i];
		scrolloffsY[i] = bgy[i];
	}

	scrollneg = negative;
}

void namcos1_playfield_control_w( int offs, int data ) {

	offs &= 0xff; 	/* splatterhouse needs this */

	if ( offs < 16 ) { /* scrolling */
		int wichone = offs / 4;
		int xy = offs & 2;
		if ( xy == 0 ) { /* scroll x */
			if ( offs & 1 )
				playfields[wichone].scroll_x = ( playfields[wichone].scroll_x & 0xff00 ) | data;
			else
				playfields[wichone].scroll_x = ( playfields[wichone].scroll_x & 0xff ) | ( data << 8 );
		} else { /* scroll y */
			if ( offs & 1 )
				playfields[wichone].scroll_y = ( playfields[wichone].scroll_y & 0xff00 ) | data;
			else
				playfields[wichone].scroll_y = ( playfields[wichone].scroll_y & 0xff ) | ( data << 8 );
		}
		return;
	}

	if ( offs < 22 ) { /* priority */
		int wichone = offs - 16;
		playfields[wichone].priority = data;
		return;
	}

	if ( offs > 23 && offs < 30 ) { /* palette */
		int wichone = offs - 24;
		if ( playfields[wichone].palette != data ) {
			playfields[wichone].palette = data;
			dirty_playfields();
		}
		return;
	}
}

int namcos1_videoram_r( int offs ) {
	return videoram[offs];
}

void namcos1_videoram_w( int offs, int data ) {
	if ( videoram[offs] != data ) {
		videoram[offs] = data;

#ifdef NAMCOS1_DIRECT_DRAW
#else
		dirtybuffer[offs] = 1;
#endif
	}
}

static int sprite_fixed_sx = 0;
static int sprite_fixed_sy = 0;

#define MAX_SPRITES		128

static void draw_sprites( struct osd_bitmap *bitmap, int pri ) {
	unsigned char *sprite_ram = ( Machine->memory_region[NAMCO_S1_RAM_REGION] + 0x8800 );
	int	i, code, color, flipx, row, col, priority, bank;
	int sy, sx;

	for ( i = 0; i < MAX_SPRITES*16; i += 16, sprite_ram += 16 ) {
		priority = ( sprite_ram[8] >> 5 ) & 0x7;

		if ( pri == priority ) {

			color = ( sprite_ram[6] >> 1 ) & 0x7f;
			sx = sprite_ram[7] + ( ( sprite_ram[6] & 1 ) << 8 );
			sy = sprite_fixed_sy - sprite_ram[9];

			sx += sprite_fixed_sx;

			flipx = ( sprite_ram[4] & 0x20 );

			switch( ( sprite_ram[8] >> 1 ) & 3 ) {
				case 0: /* 16x16 */
					code = ( ( sprite_ram[4] & 7 ) * 0x400 ) + ( sprite_ram[5] * 4 );
					code += ( sprite_ram[8] >> 3 ) & 2;
					code += ( sprite_ram[4] >> 4 ) & 1;

					drawgfx( bitmap, Machine->gfx[2],
							 code,
							 color,
							 flipx, 0,
							 sx, sy,
							 &Machine->drv->visible_area,
							 TRANSPARENCY_PEN, 15 );
				break;

				case 1: /* 8x8 */
					code = ( ( sprite_ram[4] & 7 ) * 0x1000 ) + ( sprite_ram[5] * 16 );
					code += ( sprite_ram[8] >> 1 ) & 8;
					code += ( sprite_ram[4] >> 2 ) & 4;
					code += ( sprite_ram[4] >> 2 ) & 2;
					code += ( sprite_ram[8] >> 3 ) & 1;

					if ( code & 1 )
						bank = 4;
					else
						bank = 3;

					code >>= 1;

					sx += 4;
					sy += 4;

					drawgfx( bitmap, Machine->gfx[bank],
							 code,
							 color,
							 flipx, 0,
							 sx, sy,
							 &Machine->drv->visible_area,
							 TRANSPARENCY_PEN, 15 );
				break;

				case 2: /* 32x32 */
					code = ( ( sprite_ram[4] & 7 ) * 0x400 ) + ( sprite_ram[5] * 4 );

					sy -= 16;

					for( row = 0; row <= 1; row++ ) {
						for( col=0; col <= 1; col++ ) {
							drawgfx( bitmap, Machine->gfx[2],
								code + ( 2 * row ) + col,
								color,
								flipx, 0,
								sx + 16 * ( flipx ? ( 1 - col ) : col ),
								sy + 16 * row,
								&Machine->drv->visible_area,
								TRANSPARENCY_PEN, 15 );
						}
					}
				break;
			}
		}
	}
}

#ifdef NAMCOS1_DIRECT_DRAW
static void namcos1_drawgfx_bg(struct osd_bitmap *dest,unsigned int code,
		unsigned int color,int flipx,int flipy,int sx,int sy,int width,int height)
{
	int trans_parency = (char_trans[code]==1) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN;
	/* if( char_trans[code] == 0) return; */

	drawgfx( dest,Machine->gfx[1],code,color,0, 0,sx,sy,&Machine->drv->visible_area, trans_parency , char_trans[code]);
	if( sy+8 > height)
		drawgfx( dest,Machine->gfx[1],code,color,0, 0,sx,sy-height,&Machine->drv->visible_area,trans_parency , char_trans[code]);
	if( sx+8 > width)
	{
		sx-=width;
		drawgfx( dest,Machine->gfx[1],code,color,0, 0,sx,sy,&Machine->drv->visible_area,trans_parency , char_trans[code]);
		if( sy+8 > height)
			drawgfx( dest,Machine->gfx[1],code,color,0, 0,sx,sy-height,&Machine->drv->visible_area,trans_parency , char_trans[code]);
	}
}
#else /* NAMCOS1_DIRECT_DRAW */
static void namcos1_drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int mask_region)
{
	int ox,oy,ex,ey,y,start,dy;
	unsigned short *bm,*bme;
	int col;
	struct rectangle myclip;
	int *sd4;
	int trans4,col4;
	const unsigned char *sd;
	const unsigned short *paldata;

	if (!gfx) return;

	if (!gfx->colortable) return;

	if (dest->depth != 16) {
		if ( errorlog )
			fprintf( errorlog, "Namco System 1 requires 16 bit!\n" );
		return;
	}

	if( char_trans[code] == 0 ) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - gfx->width - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - gfx->height - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	if (flipy)	/* Y flip */
	{
		start = code * gfx->height + gfx->height-1 - (sy-oy);
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height + (sy-oy);
		dy = 1;
	}

	paldata = &gfx->colortable[gfx->color_granularity * color];

	if (flipx)	/* X flip */
	{
		const struct GfxElement *mask = Machine->gfx[mask_region];
		const unsigned char *sdmask;
		const struct osd_bitmap *sbm = gfx->gfxdata;
		const struct osd_bitmap *mbm = mask->gfxdata;
		const int sdiff = gfx->width-1 - (sx-ox);
		const int mdiff = mask->width-1 - (sx-ox);
		for (y = sy;y <= ey;y++)
		{
			bm  = (unsigned short *)dest->line[y];
			bme = bm + ex;
			sd = sbm->line[start] + sdiff;
			sdmask = mbm->line[start] + mdiff;
			for( bm += sx ; bm <= bme ; bm++ )
			{
				if (*sdmask)
					*bm = paldata[*sd];
				else
					*bm = 0xfffe;
				sd--;
				sdmask--;
			}
			start+=dy;
		}
	}
	else		/* normal */
	{
		const struct GfxElement *mask = Machine->gfx[mask_region];
		const unsigned char *sdmask;
		const struct osd_bitmap *sbm = gfx->gfxdata;
		const struct osd_bitmap *mbm = mask->gfxdata;
		const int diff = (sx-ox);
		for (y = sy;y <= ey;y++)
		{
			bm  = (unsigned short *)dest->line[y];
			bme = bm + ex;
			sd = sbm->line[start] + diff;
			sdmask = mbm->line[start] + diff;
			for( bm += sx ; bm <= bme ; bm++ )
			{
				if (*sdmask)
					*bm = paldata[*sd];
				else
					*bm = 0xfffe;
				sd++;
				sdmask++;
			}
			start+=dy;
		}
	}
}

#endif /* NAMCOS1_DIRECT_DRAW */

static void draw_background( struct osd_bitmap *bitmap, int layer ) {
	int offs;
	unsigned char *vid = playfields[layer].base;
#ifdef NAMCOS1_DIRECT_DRAW
	int width   = playfields[layer].width;
	int height  = playfields[layer].height;
	int color   = playfields[layer].palette;
	int scrollx = playfields[layer].scroll_x;
	int scrolly = playfields[layer].scroll_y;

	scrollx -= scrolloffsX[layer];
	scrolly -= scrolloffsY[layer];

	if ( scrollneg ) {
		scrollx = -scrollx;
		scrolly = -scrolly;
	}

	if (scrollx < 0) scrollx = width - (-scrollx) % width;
	else scrollx %= width;
	if (scrolly < 0) scrolly = height - (-scrolly) % height;
	else scrolly %= height;

	for ( offs = 0; offs < VIDEORAM_SIZE; offs += 2 ) {
		int sx,sy,code;

		code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

		if(char_trans[code])
		{
			sx = ( ( offs / 2 ) % 64 ) * 8;
			sy = ( ( offs / 2 ) / 64 ) * 8;
			if( (sx += scrollx) >= width  ) sx-=width;
			if( (sy += scrolly) >= height ) sy-=height;
			namcos1_drawgfx_bg( bitmap,code,color,0, 0,sx,sy,width,height);
		}
	}

	for ( offs = VIDEORAM_SIZE; offs < VIDEORAM_SIZE*2; offs += 2 ) {
		int sx,sy,code;

		code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );
		if(char_trans[code])
		{
			sx = ( ( offs / 2 ) % 64 ) * 8;
			sy = ( ( offs / 2 ) / 64 ) * 8;
			if( (sx += scrollx) >= width  ) sx-=width;
			if( (sy += scrolly) >= height ) sy-=height;
			namcos1_drawgfx_bg( bitmap,code,color,0, 0,sx,sy,width,height);
		}
	}

	if ( layer < 3 ) {
		for ( offs = VIDEORAM_SIZE*2; offs < VIDEORAM_SIZE*3; offs += 2 ) {
			int sx,sy,code;

			code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );
			if(char_trans[code])
			{
				sx = ( ( offs / 2 ) % 64 ) * 8;
				sy = ( ( offs / 2 ) / 64 ) * 8;
				if( (sx += scrollx) >= width  ) sx-=width;
				if( (sy += scrolly) >= height ) sy-=height;
				namcos1_drawgfx_bg( bitmap,code,color,0, 0,sx,sy,width,height);
			}
		}

		for ( offs = VIDEORAM_SIZE*3; offs < VIDEORAM_SIZE*4; offs += 2 ) {
			int sx,sy,code;

			code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );
			if(char_trans[code])
			{
				sx = ( ( offs / 2 ) % 64 ) * 8;
				sy = ( ( offs / 2 ) / 64 ) * 8;
				if( (sx += scrollx) >= width  ) sx-=width;
				if( (sy += scrolly) >= height ) sy-=height;
				namcos1_drawgfx_bg( bitmap,code,color,0, 0,sx,sy,width,height);
			}
		}
	}
#else
	UINT16 *dir = (UINT16 *)playfields[layer].dirty;
	struct osd_bitmap *bm = playfields[layer].bitmap;
	int color = playfields[layer].palette;
	for ( offs = 0; offs < VIDEORAM_SIZE; offs += 2 ) {
		if ( dir[offs/2] ) {
			int sx,sy,code;

			dir[offs/2] = 0;

			sx = ( ( offs / 2 ) % 64 ) * 8;
			sy = ( ( offs / 2 ) / 64 ) * 8;

			code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

			namcos1_drawgfx( bm,Machine->gfx[1],
					 code,
					 color,
					 0, 0,
					 sx,sy,
					 0,0);
		}
	}

	for ( offs = VIDEORAM_SIZE; offs < VIDEORAM_SIZE*2; offs += 2 ) {
		if ( dir[offs/2] ) {
			int sx,sy,code;

			dir[offs/2] = 0;

			sx = ( ( offs / 2 ) % 64 ) * 8;
			sy = ( ( offs / 2 ) / 64 ) * 8;

			code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

			namcos1_drawgfx( bm,Machine->gfx[1],
					 code,
					 color,
					 0, 0,
					 sx,sy,
					 0,0);
		}
	}

	if ( layer < 3 ) {
		for ( offs = VIDEORAM_SIZE*2; offs < VIDEORAM_SIZE*3; offs += 2 ) {
			if ( dir[offs/2] ) {
				int sx,sy,code;

				dir[offs/2] = 0;

				sx = ( ( offs / 2 ) % 64 ) * 8;
				sy = ( ( offs / 2 ) / 64 ) * 8;

				code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

				namcos1_drawgfx( bm,Machine->gfx[1],
						 code,
						 color,
						 0, 0,
						 sx,sy,
					 0,0);
			}
		}

		for ( offs = VIDEORAM_SIZE*3; offs < VIDEORAM_SIZE*4; offs += 2 ) {
			if ( dir[offs/2] ) {
				int sx,sy,code;

				dir[offs/2] = 0;

				sx = ( ( offs / 2 ) % 64 ) * 8;
				sy = ( ( offs / 2 ) / 64 ) * 8;

				code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

				namcos1_drawgfx( bm,Machine->gfx[1],
						 code,
						 color,
						 0, 0,
						 sx,sy,
					 0,0);
			}
		}
	}

	{
		int scrollx = playfields[layer].scroll_x;
		int scrolly = playfields[layer].scroll_y;

		scrollx -= scrolloffsX[layer];
		scrolly -= scrolloffsY[layer];

		if ( scrollneg ) {
			scrollx = -scrollx;
			scrolly = -scrolly;
		}

		copyscrollbitmap( bitmap, bm, 1, &scrollx, 1, &scrolly, &Machine->drv->visible_area, TRANSPARENCY_PEN, 0xfffe );
	}
#endif
}

static void draw_foreground( struct osd_bitmap *bitmap, int layer ) {
	int offs;
	unsigned char *vid = playfields[layer].base;
#ifdef NAMCOS1_DIRECT_DRAW
	int color = playfields[layer].palette;

	for ( offs = 0; offs < VIDEORAM_SIZE; offs += 2 ) {
		int offs2;
		int sx,sy,code;

		code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );
		if(char_trans[code])
		{
			offs2 = offs / 2;
			sx = offs2 % 36;
			sy = offs2 / 36;

			/* This is offsetted for some unknown reason */

			if ( sx < 8 ) {
				sx += 28;
				sy--;
			} else {
				sx -= 8;
			}

			drawgfx( bitmap,Machine->gfx[1],
					 code,
					 color,
					 0, 0,
					 8*sx,8*sy,
					 &Machine->drv->visible_area,
					 (char_trans[code]==1) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN,
					 char_trans[code]);
		}
	}
#else
	UINT16 *dir = (UINT16 *)playfields[layer].dirty;
	struct osd_bitmap *bm = playfields[layer].bitmap;
	int color = playfields[layer].palette;

	for ( offs = 0; offs < VIDEORAM_SIZE; offs += 2 ) {
		if ( dir[offs/2] ) {
			int offs2;
			int sx,sy,code;

			dir[offs/2] = 0;

			offs2 = offs / 2;
			sx = offs2 % 36;
			sy = offs2 / 36;

			/* This is offsetted for some unknown reason */

			if ( sx < 8 ) {
				sx += 28;
				sy--;
			} else {
				sx -= 8;
			}

			code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

			namcos1_drawgfx( bm,Machine->gfx[1],
					 code,
					 color,
					 0, 0,
					 8*sx,8*sy,
					 0,0);
		}
	}

	copybitmap( bitmap, bm, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_PEN, 0xfffe );
#endif
}

void namcos1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) {
	int pri, i;

	if ( palette_recalc() )
		dirty_playfields();

	fillbitmap(bitmap,Machine->pens[0x800],&Machine->drv->visible_area);

	for ( pri = 0x00; pri < 0x08; pri++ ) { /* priority */
		for( i = 0; i < MAX_PLAYFIELDS; i++ ) {
			if ( playfields[i].priority == pri ) {
				if ( i < 4 )
					draw_background( bitmap, i );
				else
					draw_foreground( bitmap, i );
			}
		}
		draw_sprites( bitmap, pri );
	}
}

void namcos1_set_sprite_offsets( int x, int y ) {
	sprite_fixed_sx = x;
	sprite_fixed_sy = y;
}
