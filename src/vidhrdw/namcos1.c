#include "driver.h"
#include "vidhrdw/generic.h"

#define NAMCO_S1_VIDEORAM_REGION 6
#define NAMCO_S1_RAM_REGION 5

#define VIDEORAM_SIZE 0x800 /* this is for a single screen, no scroll */

int namcos1_vh_start( void ) {

	videoram = Machine->memory_region[NAMCO_S1_VIDEORAM_REGION];

	{
		/* horrid kludge to get namco sound to use the proper wave data */
		namco_wavedata = ( Machine->memory_region[NAMCO_S1_RAM_REGION] ) + 0xa000; /* Ram 1, bank 1, offset 0x0000 */
	}

	return 0;
}

void namcos1_vh_stop( void ) {
	videoram = 0;
}

int namcos1_videoram_r( int offs ) {
	return videoram[offs];
}

void namcos1_videoram_w( int offs, int data ) {
	videoram[offs] = data;
}

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
				sd++;
				sdmask++;
			}
			start+=dy;
		}
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

static void draw_background( struct osd_bitmap *bitmap, int base, int size, int color, int scrollx, int scrolly ) {
	int offs;
	unsigned char *vid = &videoram[base];

	int sxmask = ( ( 64 * 8 ) - 1 );
	int symask = ( size == VIDEORAM_SIZE * 4 ) ? ( ( 64 * 8 ) - 1 ) : ( ( 32 * 8 ) - 1 );

	for ( offs = 0; offs < size; offs += 2 ) {
		int sx,sy,code;

		sx = ( ( offs / 2 ) % 64 ) * 8;
		sy = ( ( offs / 2 ) / 64 ) * 8;

		sx += scrollx;
		sy += scrolly;

		sx &= sxmask;
		sy &= symask;

		if ( sx < 0 || sx > Machine->drv->screen_width )
			continue;
		if ( sy < 0 || sy > Machine->drv->screen_height )
			continue;

		code = vid[offs+1] + ( ( vid[offs+0] & 0x3f ) << 8 );

		namcos1_drawgfx( bitmap,Machine->gfx[1],
				 code,
				 color,
				 0, 0,
				 sx,sy,
				 &Machine->drv->visible_area,0);
	}
}

static void draw_foreground( struct osd_bitmap *bitmap, int base, int color ) {
	int offs;
	unsigned char *vid = &videoram[base];

	for ( offs = 0; offs < VIDEORAM_SIZE; offs += 2 ) {
		int offs2;
		int sx,sy,code;

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

		namcos1_drawgfx( bitmap,Machine->gfx[1],
				 code,
				 color,
				 0, 0,
				 8*sx,8*sy,
				 &Machine->drv->visible_area,0);
	}
}

/* per game scroll adjustment */
static int scrolloffsX[4];
static int scrolloffsY[4];
static int scrollneg;

static void namcos1_calc_scroll( int layer, unsigned char *layer_control, int *scrollx, int *scrolly ) {

	*scrollx = ( ( layer_control[layer] << 8 ) + ( layer_control[layer+1] ) ) - scrolloffsX[layer/4];
	*scrolly = ( ( layer_control[layer+2] << 8 ) + layer_control[layer+3] ) - scrolloffsY[layer/4];

	if ( scrollneg ) {
		*scrollx = -(*scrollx);
		*scrolly = -(*scrolly);
	}
}

#define SCROLL_SET( a ) namcos1_calc_scroll( a, layer_control, &scrollx, &scrolly );

void namcos1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) {
	unsigned char *layer_control = ( Machine->memory_region[NAMCO_S1_RAM_REGION] + 0x9000 );
	int i, color, scrollx, scrolly, pri, base, size, src_pri;

	palette_recalc();

	fillbitmap(bitmap,Machine->pens[0x800],&Machine->drv->visible_area);

	for ( pri = 0x00; pri < 0x0f; pri++ ) { /* priority */
		for( i = 0; i < 6; i++ ) {
			src_pri = layer_control[i+0x10];

			if ( ( src_pri & 0x0f ) == pri ) {
				color = layer_control[i+0x18];
				if ( i < 4 ) { /* background */
					SCROLL_SET( ( i << 2 ) )
					base = i << 13;
					size = ( i == 3 ) ? ( VIDEORAM_SIZE * 2 ) : ( VIDEORAM_SIZE * 4 );
					draw_background( bitmap, base, size, color, scrollx, scrolly );
				} else { /* foreground */
					base = 0x7000 + ( ( i - 4 ) * 0x800 );
					draw_foreground( bitmap, base, color );
				}
			}
		}

		if ( pri < 8 )
			draw_sprites( bitmap, pri );
	}
}

void namcos1_set_scroll_offsets( int *bgx, int*bgy, int negative ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		scrolloffsX[i] = bgx[i];
		scrolloffsY[i] = bgy[i];
	}

	scrollneg = negative;
}

void namcos1_set_sprite_offsets( int x, int y ) {
	sprite_fixed_sx = x;
	sprite_fixed_sy = y;
}
