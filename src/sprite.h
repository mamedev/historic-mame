#define SPRITE_FLIPX					0x01
#define SPRITE_FLIPY					0x02

extern struct sprite_info {
	int flags;
	const unsigned short *pal_data;

	int source_w,source_h;
	const unsigned char *source_baseaddr;
	int source_row_offset;

	int dest_x,dest_y,dest_w,dest_h;
} sprite_info;

void draw_sprite( struct osd_bitmap *bitmap );
