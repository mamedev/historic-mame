#include "driver.h"
#include "vidhrdw/generic.h"

#define get_gfx_pointer(gfxelement,c,line) (gfxelement->gfxdata + (c*gfxelement->height+line) * gfxelement->line_modulo)

#define NAMCO_S1_RAM_REGION 5

/*
  video ram map
  0000-1fff : scroll playfield (0)
  2000-3fff : scroll playfield (1)
  4000-5fff : scroll playfield (2)
  6000-7fff : scroll playfield (3)
  7000-700f : ?
  7010-77ff : fixed playfield (4)
  7800-780f : ?
  7810-7fff : fixed playfield (5)
*/
static unsigned char *namcos1_videoram;
/*
  s1ram map
  0000-17ff : pallete page0 : sprite
  2000-37ff : pallete page1 : playfield
  4000-57ff : pallete page2 : playfield
  6000-7fff : work ram?
  8000-87ff : work ram ?
  8800-8fff : sprite ram    : 0x10 * 128
  9000-9xff : controll reg.
*/
static unsigned char *namcos1_spriteram;
static unsigned char *namcos1_paletteram;

#define FG_OFFSET 0x7000

#define MAX_PLAYFIELDS 6
#define MAX_SPRITES    128

struct playfield {
	void	*base;
	int		scroll_x;
	int		scroll_y;
	struct tilemap *tilemap;
	int color;
};

struct playfield playfields[MAX_PLAYFIELDS];

#define TILE_MASK_SIZE  GET_TILE_BITMASK_SIZE(8,8)
static unsigned char **mask_ptr;
static unsigned char *mask_data;

static struct gfx_object_list *objectlist;
static struct gfx_object *objects;

/* palette dirty information */
static unsigned char sprite_palette_state[128];
static unsigned char tilemap_palette_state[MAX_PLAYFIELDS];

/* per game scroll adjustment */
static int scrolloffsX[4];
static int scrolloffsY[4];
static int scrollneg;

static int sprite_fixed_sx = 0;
static int sprite_fixed_sy = 0;

void namcos1_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom) {
	int i;

	for( i = 0; i < Machine->drv->total_colors; i++ ) {
		palette[i*3+0] = 0;
		palette[i*3+1] = 0;
		palette[i*3+2] = 0;
	}
}

/* tilemap callback */
static unsigned char *info_vram;
static int info_color;

static void background_get_info( int col, int row )
{
	int tile_index = (row*64+col)*2;
	int code = info_vram[tile_index+1]+((info_vram[tile_index]&0x3f)<<8);
	SET_TILE_INFO(1,code,info_color);
	tile_info.mask_data = mask_ptr[code];
}

static void foreground_get_info( int col, int row )
{
	int tile_index = (row*36+col)*2;
	int code = info_vram[tile_index+1]+((info_vram[tile_index]&0x3f)<<8);
	SET_TILE_INFO(1,code,info_color);
	tile_info.mask_data = mask_ptr[code];
}

static void update_playfield( int layer ) {
	struct tilemap *tilemap = playfields[layer].tilemap;

	/* for background , set scroll position */
	if(layer<4)
	{
		int scrollx = -playfields[layer].scroll_x + scrolloffsX[layer];
		int scrolly = -playfields[layer].scroll_y + scrolloffsY[layer];
		if ( scrollneg ) {
			scrollx = -scrollx;
			scrolly = -scrolly;
		}
		/* set scroll */
		tilemap_set_scrollx(tilemap, 0,scrollx);
		tilemap_set_scrolly(tilemap, 0,scrolly);
	}
	info_vram  = playfields[layer].base;
	info_color = objects[layer].color;
	tilemap_update( tilemap );
}

/* tilemap draw handler */
void ns1_draw_tilemap(struct osd_bitmap *bitmap,struct gfx_object *object)
{
	int layer = object->code;
	tilemap_draw( bitmap , playfields[layer].tilemap , 0 );
}

int namcos1_vh_start( void )
{
	int i;
	struct gfx_object default_object;

	/* set static memory points */
	namcos1_spriteram  = Machine->memory_region[NAMCO_S1_RAM_REGION] + 0x8800;
	namcos1_paletteram = Machine->memory_region[NAMCO_S1_RAM_REGION];

	/* allocate videoram */
	namcos1_videoram   = malloc(0x8000);
	if(!namcos1_videoram)
	{
		return 1;
	}
	memset(namcos1_videoram,0,0x8000);

	/* initialize object manager */
	memset(&default_object,0,sizeof(struct gfx_object) );
	default_object.transparency     = TRANSPARENCY_PEN;
	default_object.transparet_color = 15;
	default_object.gfx = Machine->gfx[2];
	objectlist = gfxobj_create(MAX_SPRITES+MAX_PLAYFIELDS,8,&default_object);
	if(objectlist == 0 )
	{
		free(namcos1_videoram);
		return 1;
	}
	objects = objectlist->objects;

	/* setup tilemap parameter to objects */
	for(i=0;i<MAX_PLAYFIELDS;i++)
	{
		/* set user draw handler */
		objects[i].special_handler = ns1_draw_tilemap;
		objects[i].gfx = 0; 	/* position calcration don't care */
		objects[i].code = i;	/* object->code == playfield layer number */
		objects[i].visible = 0;
		objects[i].color = i;
	}

	/* initialize playfields */
	for ( i = 0; i < MAX_PLAYFIELDS; i++ ) {
		if ( i < 4 ) {
			playfields[i].base = &namcos1_videoram[i<<13];
			playfields[i].tilemap =
				tilemap_create( background_get_info,TILEMAP_BITMASK
								,8,8
								,64,i==3 ? 32 : 64);
		} else {
			playfields[i].base = &namcos1_videoram[FG_OFFSET+0x10+( ( i - 4 ) * 0x800 )];
			playfields[i].tilemap =
				tilemap_create( foreground_get_info,TILEMAP_BITMASK
								,8,8
								,36,28);
		}
		playfields[i].scroll_x = 0;
		playfields[i].scroll_y = 0;
	}

#if 1
	if (!strcmp(Machine->gamedrv->name,"blazer"))
	{
		/* build missing gfxmaskdata patch */
		const struct GfxElement *mask = Machine->gfx[0];
		const struct GfxElement *pens = Machine->gfx[1];
		int total  = mask->total_elements;
		int width  = mask->width;
		int height = mask->height;
		int line,x,c;

		for(c=0;c<total;c++)
		{
			/* build missing gfxmaskdata patch */
			if (!strcmp(Machine->gamedrv->name,"blazer"))
			{
				for(line=0;line<height;line++)
				{
					unsigned char  *maskbm = get_gfx_pointer(mask,c,line);
					unsigned char  *pensbm = get_gfx_pointer(pens,c,line);
					for (x=0;x<width;x++)
					{
						/* transparency color is code0 */
						if(pensbm[x]!=0) maskbm[x] = 1;
					}
				}
			}
		}
	}
#endif

	/* build tilemap mask data from gfx data of mask */
	/* because this driver use ORIENTATION_ROTATE_90 */
	/* mask data can't made by ROM image             */
	{
		const struct GfxElement *mask = Machine->gfx[0];
		int total  = mask->total_elements;
		int width  = mask->width;
		int height = mask->height;
		int line,x,c;

		mask_ptr = malloc( total * sizeof(unsigned char *) );
		if(mask_ptr == 0 )
		{
			free(namcos1_videoram);
			return 1;
		}
		mask_data = malloc(total * 8 );
		if(mask_data == 0 )
		{
			free(namcos1_videoram);
			free(mask_ptr);
			return 1;
		}

		for(c=0;c<total;c++)
		{
			unsigned char *src_mask = &mask_data[c*8];
			for(line=0;line<height;line++)
			{
				unsigned char  *maskbm = get_gfx_pointer(mask,c,line);
				src_mask[line] = 0;
				for (x=0;x<width;x++)
				{
					src_mask[line] |= maskbm[x]<<(7-x);
				}
			}
			mask_ptr[c] = src_mask;
			if( mask->pen_usage)
			{
				switch(mask->pen_usage[c])
				{
				case 0x01: mask_ptr[c] = TILEMAP_BITMASK_TRANSPARENT;  break; /* blank */
				case 0x02: mask_ptr[c] = TILEMAP_BITMAK_OPAQUE; break; /* full  */
				}
			}
		}
	}

	return 0;
}

void namcos1_vh_stop( void )
{
	free(namcos1_videoram);
	free(mask_ptr);
	free(mask_data);
}

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
	/* 0-15 : scrolling */
	if ( offs < 16 )
	{
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
	}
	/* 16-21 : priority */
	else if ( offs < 22 )
	{
		/* bit.0-3 priority */
		/* bit 4   disable  */
		int wichone = offs - 16;
		objects[wichone].priority = data & 7;
		objects[wichone].visible = (data&0xf8)? 0 : 1;
	}
	/* 22,23 unknown */
	/* 24-29 palette */
	else if (offs < 24)
	{
	}
	else if ( offs < 30 )
	{ /* palette */
		int wichone = offs - 24;
		if (playfields[wichone].color != (data & 7))
		{
			playfields[wichone].color = data & 7;
			tilemap_palette_state[wichone] = 1;
		}
	}
}

int namcos1_videoram_r( int offset ) {
	return namcos1_videoram[offset];
}

void namcos1_videoram_w( int offset, int data ) {
	if ( namcos1_videoram[offset] != data ) {
		namcos1_videoram[offset] = data;
		if(offset < FG_OFFSET)
		{	/* background 0-3 */
			int layer = offset/0x2000;
			int num = (offset &= 0x1fff)/2;
			tilemap_mark_tile_dirty(playfields[layer].tilemap,num%64,num/64);
		}
		else if((offset&0x7ff)>=0x10)
		{	/* foreground 4-5 */
			int layer = (offset&0x800) ? 5 : 4;
			int num = ((offset&0x7ff)-0x10)/2;
			tilemap_mark_tile_dirty(playfields[layer].tilemap,num%36,num/36);
		}
	}
}

int namcos1_paletteram_r( int offset ) {
	return namcos1_paletteram[offset];
}

void namcos1_paletteram_w( int offset, int data )
{
	if(namcos1_paletteram[ offset ] != data)
	{
		namcos1_paletteram[ offset ] = data;
		if ( (offset&0x1fff) < 0x1800 )
		{
			if (offset < 0x2000)
			{
				sprite_palette_state[(offset&0x7f0)/16] = 1;
			}
			else
			{
				int i,color;

				color = (offset&0x700)/256;
				for(i=0;i<MAX_PLAYFIELDS;i++)
				{
					if (playfields[i].color == color)
						tilemap_palette_state[i] = 1;
				}
			}
		}
	}
}

static void namcos1_palette_refresh(int start,int offset,int num)
{
	int color;

	for (color = start;color < start + num;color++)
	{
		int r = namcos1_paletteram[offset];
		int g = namcos1_paletteram[offset + 0x0800];
		int b = namcos1_paletteram[offset + 0x1000];
		palette_change_color(color,r,g,b);
		offset++;
	}
}

static void namcos1_sprite_palette_refresh(int color)
{
	namcos1_palette_refresh(16*color,16*color,16);
}

static void namcos1_tile_palette_refresh(int color)
{
	namcos1_palette_refresh(256*color + 128*16,0x2000 + 256 * playfields[color].color,256);
}


void namcos1_spriteram_w( int offset, int data )
{
	if(namcos1_spriteram[offset] != data)
	{
		static const int sprite_sizemap[4] = {16,8,32,4};
		int num = offset / 0x10;
		struct gfx_object *object = &objectlist->objects[num+MAX_PLAYFIELDS];
		unsigned char *base = &namcos1_spriteram[num*0x10];
		int sy, sx;
		int recalc_size=0;

		namcos1_spriteram[offset] = data;
		switch(offset&0x0f)
		{
		case 4:
			/* bit.6-7 : x size (16/8/32/4) */
			/* bit.5   : flipx              */
			/* bit.3-4 : x offset           */
			/* bit.0-2 : code.8-10          */
			object->width = sprite_sizemap[(data>>6)&3];
			object->flipx = (data>>5)&1;
			object->left  = (data&0x18) & (~(object->width-1));
		case 5:
			/* bit.0-7 : code.0-7          */
			object->code = (base[4]&7)*256 + base[5];
			break;
		case 6:
			/* bit.1-7 : color */
			/* bit.0   : x draw position.8   */
			object->color = data>>1;
		case 0x07:
			/* bit.0-7 : x draw position.0-7 */
			sx = (base[6]&1)*256 + base[7];
			sx += sprite_fixed_sx;
			if( sx > 0x1e0 ) sx -= 0x200;
			if( sx < -32   ) sx += 0x200;
			object->sx = sx;
if (object->sx > -32 && object->sx < 288 && object->sy > - 32 && object->sy < 224)
	object->visible = 1;
else object->visible = 0;
			break;
		case 0x08:
			/* bit.5-7 : priority            */
			/* bit.3-4 : y offset            */
			/* bit.1-2 : y size  (16/8/32/4) */
			/* bit.0   : flipy               */
			object->priority = (data>>5)&7;
			object->height = sprite_sizemap[(data>>1)&3];
			object->top = (data&0x18) & (~(object->height-1));
			object->flipy  = data&1;
			recalc_size=1;
			break;
		case 0x09:
			/* bit.0-7 : y draw position */
			recalc_size = 1;
			break;
		default:
			return;
		}
		if(recalc_size)
		{
			sy = sprite_fixed_sy - base[9];
			sy = sy - object->height;
			if( sy < -32 ) sy += 0x100;
			object->sy = sy;
if (object->sx > -32 && object->sx < 288 && object->sy > - 32 && object->sy < 224)
	object->visible = 1;
else object->visible = 0;
		}
		object->dirty_flag = GFXOBJ_DIRTY_ALL;
	}
#if 0
	if(rows==32 && cols==32)  usrintf_showmessage("32x32 used");
	if( (cols|rows)&7)
	{	/* under x8 step */
		/* !!!!! used by dangseed !!!! */
		/* do not support 4dot sprite  */
	} else if( (cols|rows)&8)
	{	/* x8 step */
		code = (code*16) | (block);
		cols = (cols/8)-1;
		rows = (rows/8)-1;
		for( row=0; row <= rows; row++ ) {
			for( col=0; col <= cols; col++ ) {
				drawgfx( bitmap, Machine->gfx[3+(code&1)],
					code/2,
					color,
					flipx, flipy,
					sx + 8 * (flipx ? ( cols - col ) : col),
					sy + 8 * (flipy ? ( rows - row ) : row ),
					&Machine->drv->visible_area,
					TRANSPARENCY_PEN, 15 );
				code++;
			}
		}
	}
	else
	{	/* x16 step */
		code = (code*4) | (block/4);
		cols = (cols/16)-1;
		rows = (rows/16)-1;
		code &= 0xfffd; /* dangseed */
		for( row = 0; row <= rows; row++ ) {
			for( col=0; col <= cols; col++ ) {
				drawgfx( bitmap, Machine->gfx[2],
					code,
					color,
					flipx, flipy,
					sx + 16 * (flipx ? ( cols - col ) : col),
					sy + 16 * (flipy ? ( rows - row ) : row ),
					&Machine->drv->visible_area,
					TRANSPARENCY_PEN, 15 );
				code++;
			}
		}
	}
#endif
}

void namcos1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;
	struct gfx_object *object;
	unsigned short palette_map[128];


	/* update all tilemaps */
	for(i=0;i<MAX_PLAYFIELDS;i++)
		update_playfield(i);
	/* object list (sprite) update */
	gfxobj_update();
	/* palette resource marking */
	palette_init_used_colors();
	memset (palette_map, 0, sizeof (palette_map));
	for(object=objectlist->first_object ; object!=0 ; object=object->next)
	{
		if (object->visible)
		{
			int color = object->color;
			if(object->gfx)
			{	/* sprite object */
				if (sprite_palette_state[color])
				{
					namcos1_sprite_palette_refresh(color);
					sprite_palette_state[color] = 0;
				}

				palette_map[color] |= Machine->gfx[2]->pen_usage[object->code];
			}
			else
			{	/* playfield object */
				if (tilemap_palette_state[color])
				{
					namcos1_tile_palette_refresh(color);
					tilemap_palette_state[color] = 0;
				}
			}
		}
	}

	for (i = 0; i < 128; i++)
	{
		int usage = palette_map[i], j;
		if (usage)
		{
			for (j = 0; j < 15; j++)
				if (usage & (1 << j))
					palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
		}
	}
	palette_used_colors[0x800] |= PALETTE_COLOR_VISIBLE;

	if ( palette_recalc() )
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	/* */
	tilemap_render(ALL_TILEMAPS);
	/* back color */
	fillbitmap(bitmap,Machine->pens[0x800],&Machine->drv->visible_area);
	/* draw objects , tilemap and sprite */
	gfxobj_draw(objectlist);
}

void namcos1_set_sprite_offsets( int x, int y ) {
	sprite_fixed_sx = x;
	sprite_fixed_sy = y;
}


