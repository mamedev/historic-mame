/* vidhrdw/shangkid */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

static struct tilemap *background;
UINT8 *shangkid_videoreg;
int shangkid_gfx_type;

void shangkid_vh_convert_color_prom( unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom )
{
	int i;
	int bit0,bit1,bit2,bit3;
	int intensity;

	for( i = 0; i<256; i++ )
	{
		intensity = color_prom[0x300+i]; /* ? */

		colortable[i] = i;

		bit0 = (color_prom[0x000+i] >> 0) & 0x01;
		bit1 = (color_prom[0x000+i] >> 1) & 0x01;
		bit2 = (color_prom[0x000+i] >> 2) & 0x01;
		bit3 = (color_prom[0x000+i] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x100+i] >> 0) & 0x01;
		bit1 = (color_prom[0x100+i] >> 1) & 0x01;
		bit2 = (color_prom[0x100+i] >> 2) & 0x01;
		bit3 = (color_prom[0x100+i] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x200+i] >> 0) & 0x01;
		bit1 = (color_prom[0x200+i] >> 1) & 0x01;
		bit2 = (color_prom[0x200+i] >> 2) & 0x01;
		bit3 = (color_prom[0x200+i] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}
}

static void get_bg_tile_info(int tile_index){
	int attributes = videoram[tile_index+0x800];
	int tile_number = videoram[tile_index]+0x100*(attributes&0x3);
	int color;

	if( shangkid_gfx_type==1 )
	{
		/* Shanghai Kid:
			------xx	bank
			-----x--	flipx
			xxxxx---	color
		*/
		color = attributes>>3;
		color = (color&0x03)|((color&0x1c)<<1);
		SET_TILE_INFO(0,tile_number,color )
		tile_info.flags = (attributes&0x04)?TILE_FLIPX:0;
	}
	else
	{
		/* Chinese Hero:
			------xx	bank
			-xxxxx--	color
			x-------	flipx?
		*/
		color = (attributes>>2)&0x1f;
		SET_TILE_INFO(0,tile_number,color )
		tile_info.flags = (attributes&0x80)?TILE_FLIPX:0;
	}

	tile_info.priority =
		(memory_region( REGION_PROMS )[0x800+color*4]==2)?1:0;
}

int shangkid_vh_start( void )
{
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	return background?0:1;
}

void shangkid_vh_stop( void )
{
/* (debug proms)
	int i;
	for( i=0x300; i<0xa80; i++ )
	{
		if( (i&0xf)==0 ) logerror( "\n %04x: ", i );
		logerror( "%02x ", memory_region( REGION_PROMS )[i] );
	}
	logerror( "\n" );
*/
}

WRITE_HANDLER( shangkid_videoram_w )
{
	if( videoram[offset]!=data ){
		videoram[offset] = data;
		tilemap_mark_tile_dirty( background, offset&0x7ff );
	}
}

static void draw_sprite( const UINT8 *source, struct osd_bitmap *bitmap ){
	struct rectangle *clip = &Machine->visible_area;
	const struct GfxElement *gfx;
	int transparent_pen;
	int bank_index;
	int c,r;
	int width,height;
	int sx,sy;

	int ypos		= 209 - source[0];
	int tile		= source[1]&0x3f;
	int xflip		= (source[1]&0x40)?1:0;
	int yflip		= (source[1]&0x80)?1:0;
	int bank		= source[2]&0x3f;
	int xsize		= (source[2]&0x40)?1:0;
	int ysize		= (source[2]&0x80)?1:0;
	int yscale		= source[3]&0x07;	/* 0x0 = smallest; 0x7 = biggest */
	int xpos		= ((source[4]+source[5]*255)&0x1ff)-23;
	int color		= source[6]&0x3f;
	int xscale		= source[7]&0x07;	/* 0x0 = smallest; 0x7 = biggest */

	if( shangkid_gfx_type == 1 )
	{
		/* Shanghai Kid */
		switch( bank&0x30 ){
		case 0x00:
		case 0x10:
			tile += 0x40*(bank&0xf);
			break;

		case 0x20:
			tile += 0x40*((bank&0x3)|0x10);
			break;

		case 0x30:
			tile += 0x40*((bank&0x3)|0x14);
			break;
		}
		bank_index = 0;
		transparent_pen = 3;
	}
	else
	{
		/* Chinese Hero */
		color >>= 1;
		switch( bank>>2 )
		{
		case 0x0: bank_index = 0; break;
		case 0x9: bank_index = 1; break;
		case 0x6: bank_index = 2; break;
		case 0xf: bank_index = 3; break;
		default:
			bank_index = 0;
			break;
		}

		if( bank&0x01 ) tile += 0x40;
		transparent_pen = 7;
	}

	gfx = Machine->gfx[1+bank_index];

	width = (xscale+1)*2;
	height = (yscale+1)*2;

	for( r=0; r<=ysize; r++ )
	{
		for( c=0; c<=xsize; c++ )
		{
			sx = xpos+(c^xflip)*width;
			sy = ypos+(r^yflip)*height;

			drawgfxzoom(
				bitmap,
				gfx,
				tile+c*8+r,
				color,
				xflip,yflip,
				sx,sy,
				clip,
				TRANSPARENCY_PEN,transparent_pen,
				(width<<16)/16, (height<<16)/16 );
		}
	}
}

static void draw_sprites( struct osd_bitmap *bitmap )
{
	const UINT8 *source, *finish;

	finish = spriteram;
	source = spriteram+0x200;
	while( source>finish ){
		source -= 8;
		draw_sprite( source, bitmap );
	}
}

void shangkid_screenrefresh( struct osd_bitmap *bitmap, int fullfresh )
{
	int flipscreen = shangkid_videoreg[1]&0x80;
	tilemap_set_flip( background, flipscreen?(TILEMAP_FLIPX|TILEMAP_FLIPY):0 );
	tilemap_set_scrollx( background,0,shangkid_videoreg[0]-40 );
	tilemap_set_scrolly( background,0,shangkid_videoreg[2]+0x10 );
	tilemap_update( ALL_TILEMAPS );
	palette_recalc();
	tilemap_draw( bitmap,background,0,0 );
	draw_sprites( bitmap );
	tilemap_draw( bitmap,background,1,0 ); /* high priority tiles */
}
