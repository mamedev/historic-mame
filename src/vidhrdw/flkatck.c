/***************************************************************************

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *k007121_tilemap[2];

unsigned char* k007121_ram;
unsigned char* k007121_regs;

int flkatck_irq_enabled;

static int k007121_flip_screen = 0;

/***************************************************************************

  Callbacks for the K007121

***************************************************************************/

static void get_tile_info_A( int col, int row )
{
	int offs = row*32 + col;
	int attr = k007121_ram[offs];
	int code = k007121_ram[offs+0x400];
	int bank =	((attr & 0x80) << 1) | ((attr & 0x10) << 5) |
				((attr & 0x40) << 4) | ((k007121_regs[4] & 0x0c) << 9);

	if ((attr == 0x0d) && (!(k007121_regs[0])) && (!(k007121_regs[2])))
		bank = 0;	/*	this allows the game to print text
					in all banks selected by the k007121 */
	tile_info.flags = (attr & 0x20) ? TILE_FLIPY : 0;

	SET_TILE_INFO(0, code | bank, (attr & 0x0f) + 16)
}

static void get_tile_info_B( int col, int row )
{
	int offs = (row*32 + col) + 0x800;
	int attr = k007121_ram[offs];
	int code = k007121_ram[offs+0x400];

	SET_TILE_INFO(0, code, (attr & 0x0f) + 16)
}

/***************************************************************************

  Memory handlers

***************************************************************************/

void flkatck_k007121_w(int offset,int data){
	if (offset < 0x1000){	/* tiles */
		if (k007121_ram[offset] != data){
			k007121_ram[offset] = data;
			tilemap_mark_tile_dirty(k007121_tilemap[offset/0x800], (offset & 0x3ff)%32, (offset & 0x3ff)/32 );
		}
	}
	else{	/* sprites */
		k007121_ram[offset] = data;
	}
}

void flkatck_k007121_regs_w(int offset,int data){
	switch (offset){
		case 0x04:	/* ROM bank select */
			if (data != k007121_regs[0x04])
				tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
			break;

		case 0x07:	/* flip screen + IRQ control */
			k007121_flip_screen = data & 0x08;
			tilemap_set_flip(ALL_TILEMAPS, k007121_flip_screen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			flkatck_irq_enabled = data & 0x02;
			break;
	}

	k007121_regs[offset] = data;
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int flkatck_vh_start(void){
	k007121_tilemap[0] = tilemap_create(get_tile_info_A, TILEMAP_OPAQUE, 8,8, 32, 32 );
	k007121_tilemap[1] = tilemap_create(get_tile_info_B, TILEMAP_TRANSPARENT, 8,8, 32, 32 );

	if (k007121_tilemap[0] && k007121_tilemap[1]){
		k007121_tilemap[1]->transparent_pen = 0;

		return 0;
	}

	return 1;
}

void flkatck_vh_stop(void)
{
}

/***************************************************************************

	Display Refresh

***************************************************************************/

/***************************************************************************

	Flack Attack sprites. Each sprite has 16 bytes!:

	Byte | Bit(s)   | Use
	-----+-76543210-+----------------
	0	 | xxxxxxxx | ???
	1	 | xxxxxxxx | ???
	2	 | xxxxxxxx | ???
	3	 | xxxxxxxx | ???
	4	 | xxxxxxxx | x position (bits 0-7)
	5	 | xxxxxxxx | ???
	6	 | xxxxxxxx | y position
	7	 | xxxxxxxx | ???
	8	 | xx------ | code (bits 12-13)
	8	 | --xx---- | flip
	8	 | ----xxx- | sprite size
	8	 | -------x | x position (bit 8)
	9	 | xxxxxxxx | ???
	a	 | xxxxxxxx | ???
	b	 | xxxxxxxx | ???
	c	 | xxxxxxxx | ???
	d	 | xxxxxxxx | ???
	e	 | xxxxxxxx | code (bits 2-9)
	f	 | xxxx---- | color
	f	 | ----xx-- | code (bits 0-1)
	f	 | ------xx | code (bits 10-11)

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	const struct GfxElement *gfx = Machine->gfx[0];
	unsigned char *source, *finish;

	source = &RAM[0x3000 + 0x800 - 0x20];
	finish = &RAM[0x3000];

	while( source >= finish ){
		int number = (source[0x0e] << 2) | ((source[0x0f] & 0x0c) >> 2) /* sprite number */
					| ((source[0x08] & 0xc0) << 6) | ((source[0x0f] & 0x03) << 10);
		int sx = source[0x04];				/* x position */
		int sy = source[0x06];				/* y position */
		int color = source[0x0f] >> 4;		/* color */
		int xflip = source[0x08] & 0x10;	/* flip x */
		int yflip = source[0x08] & 0x20;	/* flip y */
		int width, height;

		if (source[0x08] & 0x01 )	sx -= 256;

		switch( source[0x08] & 0x0e ){
			case 0x06: width = height = 1; break;
			case 0x04: width = 1; height = 2; number &= (~2); break;
			case 0x02: width = 2; height = 1; number &= (~1); break;
			case 0x00: width = height = 2; number &= (~3); break;
			case 0x08: width = height = 4; number &= (~3); break;
			default: width = 1; height = 1;
		}

		if (source[0x00]){
			static int x_offset[4] = { 0x00,0x01,0x04,0x05 };
			static int y_offset[4] = { 0x00,0x02,0x08,0x0a };
			int x,y, ex, ey;

			for( y=0; y < height; y++ ){
				for( x=0; x < width; x++ ){
					ex = xflip ? (width-1-x) : x;
					ey = yflip ? (height-1-y) : y;

					drawgfx(bitmap,gfx,
						number+x_offset[ex]+y_offset[ey],
						color,
						xflip, yflip,
						sx+x*8,sy+y*8,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,0);
				}
			}
		}
	source -= 0x20;
	}
}

static void mark_sprites_colors(void)
{
	int i;
	unsigned char *RAM = memory_region(REGION_CPU1);
	unsigned char *source, *finish;

	unsigned short palette_map[16];

	source = &RAM[0x3000 + 0x800 - 0x20];
	finish = &RAM[0x3000];

	memset (palette_map, 0, sizeof (palette_map));

	while( source >= finish ){
		int color = source[0x0f] >> 4;
		palette_map[color] |= 0xffff;
		source -= 0x20;
	}

	/* now build the final table */
	for (i = 0; i < 16; i++){
		int usage = palette_map[i], j;
		if (usage){
			for (j = 1; j < 16; j++)
				if (usage & (1 << j))
					palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
		}
	}
}

void flkatck_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update( ALL_TILEMAPS );

	palette_init_used_colors();
	mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render( ALL_TILEMAPS );

	/* set scroll registers */
	tilemap_set_scrollx( k007121_tilemap[0], 0, k007121_regs[0] );
	tilemap_set_scrolly( k007121_tilemap[0], 0, k007121_regs[2] );

	/* draw the graphics */
	tilemap_draw( bitmap,k007121_tilemap[0], 0 );
	draw_sprites( bitmap );
	tilemap_draw( bitmap,k007121_tilemap[1], 0 );
}
