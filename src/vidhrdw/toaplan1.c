/***************************************************************************

  vidhrdw/toaplan1.c

  Functions to emulate the video hardware of the machine.

  There are 4 scrolling layers of graphics, stored in planes of 64x64 tiles.
  Each tile in each plane is assigned a priority between 1 and 15, higher
  numbers have greater priority.

  Each tile takes up 32 bits, the format is:

  0         1         2         3
  ---- ---- ---- ---- -xxx xxxx xxxx xxxx = tile number (0 - $7fff)
  ---- ---- ---- ---- ?--- ---- ---- ---- = unknown / unused
  ---- ---- --xx xxxx ---- ---- ---- ---- = color (0 - $3f)
  ---- ???? ??-- ---- ---- ---- ---- ---- = unknown / unused
  xxxx ---- ---- ---- ---- ---- ---- ---- = priority (0-$f)

  BG Scroll Reg

  0         1         2         3
  xxxx xxxx x--- ---- ---- ---- ---- ---- = x
  ---- ---- ---- ---- yyyy yyyy ---- ---- = y


 Sprite RAM format  (except Rally Bike)

  0         1         2         3
  ---- ---- ---- ---- -xxx xxxx xxxx xxxx = tile number (0 - $7fff)
  ---- ---- ---- ---- x--- ---- ---- ---- = hidden
  ---- ---- --xx xxxx ---- ---- ---- ---- = color (0 - $3f)
  ---- xxxx xx-- ---- ---- ---- ---- ---- = sprite number
  xxxx ---- ---- ---- ---- ---- ---- ---- = priority (0-$f)

  4         5         6         7
  ---- ---- ---- ---- xxxx xxxx x--- ---- = x
  yyyy yyyy y--- ---- ---- ---- ---- ---- = y

  The tiles use a palette of 1024 colors, the sprites use a different palette
  of 1024 colors.

***************************************************************************/

#include "driver.h"
#include "palette.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

#define TOAPLAN1_SPRITERAM16_SIZE      0x800	/* sprite ram (word size) */
#define TOAPLAN1_SPRITESIZERAM16_SIZE  0x80		/* sprite size ram (word size) */
#define TOAPLAN1_TILERAM16_SIZE        0x2000	/* each tile layer ram (word size) */

data16_t *toaplan1_spriteram16;
data16_t *toaplan1_spritesizeram16;
data16_t *toaplan1_tileram16;
data16_t *toaplan1_buffered_spriteram16;
data16_t *toaplan1_buffered_spritesizeram16;

size_t toaplan1_colorram1_size;
size_t toaplan1_colorram2_size;
data16_t *toaplan1_colorram1;
data16_t *toaplan1_colorram2;

static unsigned int scrollregs[8];
static unsigned int vblank;
static unsigned int num_tiles;

static unsigned int spriteram_offs;
static unsigned int tileram_offs;

static int layer_scrollx[4];
static int layer_scrolly[4];

static int flipscreen;
static int tiles_offsetx;
static int tiles_offsety;
static int layers_offset[4];


typedef struct
	{
	UINT16 tile_num;
	UINT16 color;
	char priority;
	int xpos;
	int ypos;
	} tile_struct;

tile_struct *bg_list[4];

tile_struct *tile_list[32];
tile_struct *temp_list;
static int max_list_size[32];
static int tile_count[32];

struct mame_bitmap *tmpbitmap1;
static struct mame_bitmap *tmpbitmap2;
static struct mame_bitmap *tmpbitmap3;

#undef BGDBG

#ifdef BGDBG
int toaplan_dbg_sprite_only = 0;
int	toaplan_dbg_priority = 0;
int	toaplan_dbg_layer[4] = {1,1,1,1};
#endif

int rallybik_vh_start(void)
{
	int i;

	if ((toaplan1_tileram16 = calloc(TOAPLAN1_TILERAM16_SIZE * 4, 2)) == 0) /* 4 layers */
	{
		return 1;
	}

	logerror("colorram_size: %08x\n", toaplan1_colorram1_size + toaplan1_colorram2_size);
	if ((paletteram16 = calloc((toaplan1_colorram1_size/2) + (toaplan1_colorram2_size/2), 2)) == 0)
	{
		free(toaplan1_tileram16);
		return 1;
	}

	for (i=0; i<4; i++)
	{
		if ((bg_list[i]=(tile_struct *)malloc( 33 * 44 * sizeof(tile_struct))) == 0)
		{
			free(paletteram16);
			free(toaplan1_tileram16);
			return 1;
		}
		memset(bg_list[i], 0, 33 * 44 * sizeof(tile_struct));
	}

	for (i=0; i<16; i++)
	{
		max_list_size[i] = 8192;
		if ((tile_list[i]=(tile_struct *)malloc(max_list_size[i]*sizeof(tile_struct))) == 0)
		{
			for (i=3; i>=0; i--)
				free(bg_list[i]);
			free(paletteram16);
			free(toaplan1_tileram16);
			return 1;
		}
		memset(tile_list[i],0,max_list_size[i]*sizeof(tile_struct));
	}

	max_list_size[16] = 65536;
	if ((tile_list[16]=(tile_struct *)malloc(max_list_size[16]*sizeof(tile_struct))) == 0)
	{
		for (i=15; i>=0; i--)
			free(tile_list[i]);
		for (i=3; i>=0; i--)
			free(bg_list[i]);
		free(paletteram16);
		free(toaplan1_tileram16);
		return 1;
	}
	memset(tile_list[16],0,max_list_size[16]*sizeof(tile_struct));

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8);

	spriteram_offs = tileram_offs = 0;

	return 0;
}

void rallybik_vh_stop(void)
{
	int i;

	for (i=16; i>=0; i--)
	{
		free(tile_list[i]);
		logerror("max_list_size[%d]=%08x\n",i,max_list_size[i]);
	}

	for (i=3; i>=0; i--)
		free(bg_list[i]);

	free(paletteram16);
	free(toaplan1_tileram16);
}

int toaplan1_vh_start(void)
{
	tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap2 = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap3 = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if ((toaplan1_spriteram16 = calloc(TOAPLAN1_SPRITERAM16_SIZE, 2)) == 0)
	{
		return 1;
	}
	if ((toaplan1_buffered_spriteram16 = calloc(TOAPLAN1_SPRITERAM16_SIZE, 2)) == 0)
	{
		free(toaplan1_spriteram16);
		return 1;
	}
	if ((toaplan1_spritesizeram16 = calloc(TOAPLAN1_SPRITESIZERAM16_SIZE, 2)) == 0)
	{
		free(toaplan1_buffered_spriteram16);
		free(toaplan1_spriteram16);
		return 1;
	}
	if ((toaplan1_buffered_spritesizeram16 = calloc(TOAPLAN1_SPRITESIZERAM16_SIZE, 2)) == 0)
	{
		free(toaplan1_spritesizeram16);
		free(toaplan1_buffered_spriteram16);
		free(toaplan1_spriteram16);
		return 1;
	}

	/* Also include all allocated stuff in Rally Bike startup */
	return rallybik_vh_start();
}

void toaplan1_vh_stop(void)
{
	rallybik_vh_stop();

	free(toaplan1_buffered_spritesizeram16);
	free(toaplan1_spritesizeram16);
	free(toaplan1_buffered_spriteram16);
	free(toaplan1_spriteram16);
	bitmap_free(tmpbitmap3);
	bitmap_free(tmpbitmap2);
	bitmap_free(tmpbitmap1);
}




READ16_HANDLER( toaplan1_vblank_r )
{
	vblank ^= 1;
	return vblank;
}

WRITE16_HANDLER( toaplan1_flipscreen_w )
{
	if (ACCESSING_MSB)
		flipscreen = data & 0xff00;		/* 0x8000 = flip, 0x0000 = no flip */
}

READ16_HANDLER( toaplan1_spriteram_offs_r )
{
	return spriteram_offs;
}

WRITE16_HANDLER( toaplan1_spriteram_offs_w )
{
	COMBINE_DATA(&spriteram_offs);
}

/* tile palette */
READ16_HANDLER( toaplan1_colorram1_r )
{
	return toaplan1_colorram1[offset];
}

WRITE16_HANDLER( toaplan1_colorram1_w )
{
	COMBINE_DATA(&toaplan1_colorram1[offset]);
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset, data, 0);
}

/* sprite palette */
READ16_HANDLER( toaplan1_colorram2_r )
{
	return toaplan1_colorram2[offset];
}

WRITE16_HANDLER( toaplan1_colorram2_w )
{
	COMBINE_DATA(&toaplan1_colorram2[offset]);
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset+(toaplan1_colorram1_size/2), data, 0);
}

READ16_HANDLER( toaplan1_spriteram16_r )
{
	return toaplan1_spriteram16[spriteram_offs & (TOAPLAN1_SPRITERAM16_SIZE-1)];
}

WRITE16_HANDLER( toaplan1_spriteram16_w )
{
	COMBINE_DATA(&toaplan1_spriteram16[spriteram_offs & (TOAPLAN1_SPRITERAM16_SIZE-1)]);

#ifdef MAME_DEBUG
	if (spriteram_offs >= TOAPLAN1_SPRITERAM16_SIZE)
	{
		logerror("Sprite_RAM_word_w, %08x out of range !\n", spriteram_offs);
		return;
	}
#endif

	spriteram_offs++;
}

READ16_HANDLER( toaplan1_spritesizeram16_r )
{
	return toaplan1_spritesizeram16[spriteram_offs & (TOAPLAN1_SPRITESIZERAM16_SIZE-1)];
}

WRITE16_HANDLER( toaplan1_spritesizeram16_w )
{
	COMBINE_DATA(&toaplan1_spritesizeram16[spriteram_offs & (TOAPLAN1_SPRITESIZERAM16_SIZE-1)]);

#ifdef MAME_DEBUG
	if (spriteram_offs >= TOAPLAN1_SPRITESIZERAM16_SIZE)
	{
		logerror("Sprite_Size_RAM_word_w, %08x out of range !\n", spriteram_offs);
		return;
	}
#endif

	spriteram_offs++;
}

READ16_HANDLER( toaplan1_tileram_offs_r )
{
	return tileram_offs;
}

WRITE16_HANDLER( toaplan1_tileram_offs_w )
{
	COMBINE_DATA(&tileram_offs);
}

READ16_HANDLER( rallybik_tileram16_r )
{
	data16_t data = toaplan1_tileram16[((tileram_offs * 2) + offset) & ((TOAPLAN1_TILERAM16_SIZE*4)-1)];

	if (offset == 0)	/* some bit lines may be stuck to others */
	{
		data |= ((data & 0xf000) >> 4);
		data |= ((data & 0x0030) << 2);
	}
	return data;
}

READ16_HANDLER( toaplan1_tileram16_r )
{
	data16_t data = toaplan1_tileram16[((tileram_offs * 2) + offset) & ((TOAPLAN1_TILERAM16_SIZE*4)-1)];

	return data;
}

WRITE16_HANDLER( toaplan1_tileram16_w )
{
	COMBINE_DATA(&toaplan1_tileram16[((tileram_offs * 2) + offset) & ((TOAPLAN1_TILERAM16_SIZE*4)-1)]);

#ifdef MAME_DEBUG
	if ( ((tileram_offs * 2) + offset) >= (TOAPLAN1_TILERAM16_SIZE*4) )
	{
		logerror("Tile_RAM_w, %08x out of range !\n", tileram_offs);
		return;
	}
#endif

	if ( offset == 1 ) tileram_offs++;
}

READ16_HANDLER( toaplan1_scroll_regs_r )
{
	return scrollregs[offset];
}

WRITE16_HANDLER( toaplan1_scroll_regs_w )
{
	COMBINE_DATA(&scrollregs[offset]);
}

WRITE16_HANDLER( toaplan1_tile_offsets_w )
{
	if ( offset == 0 )
		COMBINE_DATA(&tiles_offsetx);
	else
		COMBINE_DATA(&tiles_offsety);
}

WRITE16_HANDLER( toaplan1_layers_offset_w )
{
	switch (offset & 0x3)
	{
		case 0:
			COMBINE_DATA(&layers_offset[0]);
			break;
		case 1:
			COMBINE_DATA(&layers_offset[1]);
			break;
		case 2:
			COMBINE_DATA(&layers_offset[2]);
			break;
		case 3:
			COMBINE_DATA(&layers_offset[3]);
			break;
		default:
			logerror("Unknown layers_offset[%08x] select\n",offset); break;
	}

	logerror("layers_offset[0]:%08x\n",layers_offset[0]);
	logerror("layers_offset[1]:%08x\n",layers_offset[1]);
	logerror("layers_offset[2]:%08x\n",layers_offset[2]);
	logerror("layers_offset[3]:%08x\n",layers_offset[3]);

}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.

***************************************************************************/


static void toaplan1_find_tiles(int xoffs,int yoffs)
{
	int priority;
	int layer;
	tile_struct *tinfo;
	data16_t *t_info;

	if (flipscreen){
		layer_scrollx[0] = ((scrollregs[0]) >> 7) + (523 - xoffs);
		layer_scrollx[1] = ((scrollregs[2]) >> 7) + (525 - xoffs);
		layer_scrollx[2] = ((scrollregs[4]) >> 7) + (527 - xoffs);
		layer_scrollx[3] = ((scrollregs[6]) >> 7) + (529 - xoffs);

		layer_scrolly[0] = ((scrollregs[1]) >> 7) + (256 - yoffs);
		layer_scrolly[1] = ((scrollregs[3]) >> 7) + (256 - yoffs);
		layer_scrolly[2] = ((scrollregs[5]) >> 7) + (256 - yoffs);
		layer_scrolly[3] = ((scrollregs[7]) >> 7) + (256 - yoffs);
	}else{
		layer_scrollx[0] = ((scrollregs[0]) >> 7) + (495 - xoffs + 6);
		layer_scrollx[1] = ((scrollregs[2]) >> 7) + (495 - xoffs + 4);
		layer_scrollx[2] = ((scrollregs[4]) >> 7) + (495 - xoffs + 2);
		layer_scrollx[3] = ((scrollregs[6]) >> 7) + (495 - xoffs);

		layer_scrolly[0] = ((scrollregs[1]) >> 7) + (0x101 - yoffs);
		layer_scrolly[1] = ((scrollregs[3]) >> 7) + (0x101 - yoffs);
		layer_scrolly[2] = ((scrollregs[5]) >> 7) + (0x101 - yoffs);
		layer_scrolly[3] = ((scrollregs[7]) >> 7) + (0x101 - yoffs);
	}

	for ( layer = 3; layer >= 0; layer-- )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

#ifdef BGDBG
		if( toaplan_dbg_layer[layer] == 1 ){
#endif

		t_info = toaplan1_tileram16 + (layer * TOAPLAN1_TILERAM16_SIZE);
		scrollx = layer_scrollx[layer];
		offsetx = scrollx / 8;
		scrolly = layer_scrolly[layer];
		offsety = scrolly / 8;

		for ( sy = 0; sy < 32; sy++ )
		{
			for ( sx = 0; sx <= 40; sx++ )
			{
				i = ((sy+offsety)&0x3f)*128 + ((sx+offsetx)&0x3f)*2;
				tattr = t_info[i];
				priority = (tattr >> 12);

				tinfo = (tile_struct *)&(bg_list[layer][sy*41+sx]);
				tinfo->tile_num = t_info[i+1];
				tinfo->priority = priority;
				tinfo->color = tattr & 0x3f;
				tinfo->xpos = (sx*8)-(scrollx&0x7);
				tinfo->ypos = (sy*8)-(scrolly&0x7);

				if ( (priority) || (layer == 0) )	/* if priority 0 draw layer 0 only */
				{

					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]);
					tinfo->tile_num = t_info[i+1];
					if ( (tinfo->tile_num & 0x8000) == 0 )
					{
						tinfo->priority = priority;
						tinfo->color = tattr & 0x3f;
						tinfo->color |= layer<<8;
						tinfo->xpos = (sx*8)-(scrollx&0x7);
						tinfo->ypos = (sy*8)-(scrolly&0x7);
						tile_count[priority]++;
						if(tile_count[priority]==max_list_size[priority])
						{
							/*reallocate tile_list[priority] to larger size */
							temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512));
							memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
							max_list_size[priority]+=512;
							free(tile_list[priority]);
							tile_list[priority] = temp_list;
						}
					}
				}
			}
		}
#ifdef BGDBG
		}
#endif
	}
	for ( layer = 3; layer >= 0; layer-- )
	{
		layer_scrollx[layer] &= 0x7;
		layer_scrolly[layer] &= 0x7;
	}

}

static void rallybik_find_tiles(void)
{
	int priority;
	int layer;
	tile_struct *tinfo;
	data16_t *t_info;

	for ( priority = 0; priority < 16; priority++ )
	{
		tile_count[priority]=0;
	}

	for ( layer = 3; layer >= 0; layer-- )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

		t_info = toaplan1_tileram16 + (layer * TOAPLAN1_TILERAM16_SIZE);
		scrollx = scrollregs[layer*2];
		scrolly = scrollregs[(layer*2)+1];

		scrollx >>= 7;
		scrollx += 43;
		if ( layer == 0 ) scrollx += 2;
		if ( layer == 2 ) scrollx -= 2;
		if ( layer == 3 ) scrollx -= 4;
		offsetx = scrollx / 8;

		scrolly >>= 7;
		scrolly += 21;
		offsety = scrolly / 8;

		for ( sy = 0; sy < 32; sy++ )
		{
			for ( sx = 0; sx <= 40; sx++ )
			{
				i = ((sy+offsety)&0x3f)*128 + ((sx+offsetx)&0x3f)*2;
				tattr = t_info[i];
				priority = tattr >> 12;
				if ( (priority) || (layer == 0) )	/* if priority 0 draw layer 0 only */
				{
					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]);
					tinfo->tile_num = t_info[i+1];

					if ( !((priority) && (tinfo->tile_num & 0x8000)) )
					{
						tinfo->tile_num &= 0x3fff;
						tinfo->color = tattr & 0x3f;
						tinfo->xpos = (sx*8)-(scrollx&0x7);
						tinfo->ypos = (sy*8)-(scrolly&0x7);
						tile_count[priority]++;
						if(tile_count[priority]==max_list_size[priority])
						{
							/*reallocate tile_list[priority] to larger size */
							temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512));
							memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
							max_list_size[priority]+=512;
							free(tile_list[priority]);
							tile_list[priority] = temp_list;
						}
					}
				}
			}
		}
	}
}

unsigned long toaplan_sp_ram_dump = 0;

static void toaplan1_find_sprites (void)
{
	int priority;
	int sprite;
	data16_t *s_info;
	data16_t *s_size;


	for ( priority = 0; priority < 17; priority++ )
	{
		tile_count[priority]=0;
	}


#if 0		// sp ram dump start
	s_size = toaplan1_buffered_spritesizeram16;	/* sprite block size */
	s_info = toaplan1_buffered_spriteram16;		/* start of sprite ram */
	if( (toaplan_sp_ram_dump == 0)
	 && (keyboard_pressed(KEYCODE_N)) )
	{
		toaplan_sp_ram_dump = 1;
		for ( sprite = 0; sprite < 256; sprite++ )
		{
			int tattr,tchar;
			tchar = s_info[0];
			tattr = s_info[1];
			logerror("%08x: %04x %04x\n", sprite, tattr, tchar);
			s_info += 4;
		}
	}
#endif		// end

	s_size = toaplan1_buffered_spritesizeram16;	/* sprite block size */
	s_info = toaplan1_buffered_spriteram16;		/* start of sprite ram */

	for ( sprite = 0; sprite < 256; sprite++ )
	{
		int tattr,tchar;

		tchar = s_info[0];
		tattr = s_info[1];

		if ( (tattr & 0xf000) && ((tchar & 0x8000) == 0) )
		{
			int sx,sy,dx,dy,s_sizex,s_sizey;
			int sprite_size_ptr;

			sx=s_info[2];
			sx >>= 7;
			if ( sx > 416 ) sx -= 512;

			sy=s_info[3];
			sy >>= 7;
			if ( sy > 416 ) sy -= 512;

			priority = (tattr >> 12);

			sprite_size_ptr = (tattr>>6)&0x3f;
			s_sizey = (s_size[sprite_size_ptr]>>4)&0xf;
			s_sizex =  s_size[sprite_size_ptr]    &0xf;

			for ( dy = s_sizey; dy > 0; dy-- )
			for ( dx = s_sizex; dx > 0; dx-- )
			{
				tile_struct *tinfo;

				tinfo = (tile_struct *)&(tile_list[16][tile_count[16]]);
				tinfo->priority = priority;
				tinfo->tile_num = tchar;
				tinfo->color = 0x80 | (tattr & 0x3f);
				tinfo->xpos = sx-dx*8+s_sizex*8;
				tinfo->ypos = sy-dy*8+s_sizey*8;
				tile_count[16]++;
				if(tile_count[16]==max_list_size[16])
				{
					/*reallocate tile_list[priority] to larger size */
					temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[16]+512));
					memcpy(temp_list,tile_list[16],sizeof(tile_struct)*max_list_size[16]);
					max_list_size[16]+=512;
					free(tile_list[16]);
					tile_list[16] = temp_list;
				}
				tchar++;
			}
		}
		s_info += 4;
	}
}

static void rallybik_find_sprites (void)
{
	int offs;
	int tattr;
	int sx,sy,tchar;
	int priority;
	tile_struct *tinfo;

	for (offs = 0;offs < (spriteram_size/2);offs += 4)
	{
		tattr = buffered_spriteram16[offs+1];
		if ( tattr )	/* no need to render hidden sprites */
		{
			sx=buffered_spriteram16[offs+2];
			sx >>= 7;
			sx &= 0x1ff;
			if ( sx > 416 ) sx -= 512;

			sy=buffered_spriteram16[offs+3];
			sy >>= 7;
			sy &= 0x1ff;
			if ( sy > 416 ) sy -= 512;

			priority = (tattr>>8) & 0xc;
			tchar = buffered_spriteram16[offs];
			tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]);
			tinfo->tile_num = tchar & 0x7ff;
			tinfo->color = 0x80 | (tattr&0x3f);
			tinfo->color |= (tattr & 0x0100);
			tinfo->color |= (tattr & 0x0200);
			if (tinfo->color & 0x0100) sx -= 15;

			tinfo->xpos = sx-31;
			tinfo->ypos = sy-16;
			tile_count[priority]++;
			if(tile_count[priority]==max_list_size[priority])
			{
				/*reallocate tile_list[priority] to larger size */
				temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512));
				memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
				max_list_size[priority]+=512;
				free(tile_list[priority]);
				tile_list[priority] = temp_list;
			}
		}  // if tattr
	} // for sprite
}



void toaplan1_sprite_mask
	(
	struct mame_bitmap *dest_bmp,
	struct mame_bitmap *src_bmp,
	const struct rectangle *clip
	)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;
	int transparent_color;

	transparent_color = Machine->pens[0];

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = clip->min_y;
		myclip.min_y = temp;
		temp = clip->max_x;
		myclip.max_x = clip->max_y;
		myclip.max_y = temp;
		clip = &myclip;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		int temp;

		sx = -sx;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = dest_bmp->width-1 - clip->max_x;
		myclip.max_x = dest_bmp->width-1 - temp;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;
		clip = &myclip;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		int temp;

		sy = -sy;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_y;
		myclip.min_y = dest_bmp->height-1 - clip->max_y;
		myclip.max_y = dest_bmp->height-1 - temp;
		clip = &myclip;
	}

	{
		int ex = sx+src_bmp->width;
		int ey = sy+src_bmp->height;

		if( sx < clip->min_x)
		{ /* clip left */
			sx = clip->min_x;
		}
		if( sy < clip->min_y )
		{ /* clip top */
			sy = clip->min_y;
		}
		if( ex > clip->max_x+1 )
		{ /* clip right */
			ex = clip->max_x + 1;
		}
		if( ey > clip->max_y+1 )
		{ /* clip bottom */
			ey = clip->max_y + 1;
		}

		if( ex>sx )
		{ /* skip if inner loop doesn't draw anything */
			int y;

			for( y=sy; y<ey; y++ )
			{
				unsigned short *dest = (unsigned short *)dest_bmp->line[y];
				unsigned short *source = (unsigned short *)src_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
				{
					int c = source[x];
					if( c != transparent_color )
						dest[x] = transparent_color;
				}
			}
		}
	}
}



void toaplan1_sprite_copy
	(
	struct mame_bitmap *dest_bmp,
	struct mame_bitmap *src_bmp,
	struct mame_bitmap *look_bmp,
	const struct rectangle *clip
	)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;
	int transparent_color;

	transparent_color = Machine->pens[0];

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = clip->min_y;
		myclip.min_y = temp;
		temp = clip->max_x;
		myclip.max_x = clip->max_y;
		myclip.max_y = temp;
		clip = &myclip;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		int temp;

		sx = -sx;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = dest_bmp->width-1 - clip->max_x;
		myclip.max_x = dest_bmp->width-1 - temp;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;
		clip = &myclip;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		int temp;

		sy = -sy;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_y;
		myclip.min_y = dest_bmp->height-1 - clip->max_y;
		myclip.max_y = dest_bmp->height-1 - temp;
		clip = &myclip;
	}

	{
		int ex = sx+src_bmp->width;
		int ey = sy+src_bmp->height;

		if( sx < clip->min_x)
		{ /* clip left */
			sx = clip->min_x;
		}
		if( sy < clip->min_y )
		{ /* clip top */
			sy = clip->min_y;
		}
		if( ex > clip->max_x+1 )
		{ /* clip right */
			ex = clip->max_x + 1;
		}
		if( ey > clip->max_y+1 )
		{ /* clip bottom */
			ey = clip->max_y + 1;
		}

		if( ex>sx )
		{ /* skip if inner loop doesn't draw anything */
			int y;

			for( y=sy; y<ey; y++ )
			{
				unsigned short *dest = (unsigned short *)dest_bmp->line[y];
				unsigned short *source = (unsigned short *)src_bmp->line[y];
				unsigned short *look = (unsigned short *)look_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
				{
					if( look[x] != transparent_color )
						dest[x] = source[x];
				}
			}
		}
	}
}



static void toaplan1_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

#ifdef BGDBG

if (keyboard_pressed(KEYCODE_Q)) {
	toaplan_dbg_priority = 0;
}
if (keyboard_pressed(KEYCODE_Q)) { toaplan_dbg_priority = 0; }
if (keyboard_pressed(KEYCODE_W)) { toaplan_dbg_priority = 1; }
if (keyboard_pressed(KEYCODE_E)) { toaplan_dbg_priority = 2; }
if (keyboard_pressed(KEYCODE_R)) { toaplan_dbg_priority = 3; }
if (keyboard_pressed(KEYCODE_T)) { toaplan_dbg_priority = 4; }
if (keyboard_pressed(KEYCODE_Y)) { toaplan_dbg_priority = 5; }
if (keyboard_pressed(KEYCODE_U)) { toaplan_dbg_priority = 6; }
if (keyboard_pressed(KEYCODE_I)) { toaplan_dbg_priority = 7; }
if (keyboard_pressed(KEYCODE_A)) { toaplan_dbg_priority = 8; }
if (keyboard_pressed(KEYCODE_S)) { toaplan_dbg_priority = 9; }
if (keyboard_pressed(KEYCODE_D)) { toaplan_dbg_priority = 10; }
if (keyboard_pressed(KEYCODE_F)) { toaplan_dbg_priority = 11; }
if (keyboard_pressed(KEYCODE_G)) { toaplan_dbg_priority = 12; }
if (keyboard_pressed(KEYCODE_H)) { toaplan_dbg_priority = 13; }
if (keyboard_pressed(KEYCODE_J)) { toaplan_dbg_priority = 14; }
if (keyboard_pressed(KEYCODE_K)) { toaplan_dbg_priority = 15; }

if (keyboard_pressed(KEYCODE_Z)) {
	toaplan_dbg_sprite_only ^= 1;
}
if (keyboard_pressed(KEYCODE_X)) {
	toaplan_dbg_layer[0] ^= 1;
}
if (keyboard_pressed(KEYCODE_C)) {
	toaplan_dbg_layer[1] ^= 1;
}
if (keyboard_pressed(KEYCODE_V)) {
	toaplan_dbg_layer[2] ^= 1;
}
if (keyboard_pressed(KEYCODE_B)) {
	toaplan_dbg_layer[3] ^= 1;
}

if( toaplan_dbg_priority != 0 ){

	priority = toaplan_dbg_priority;
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]);
		/* hack to fix black blobs in Demon's World sky */
		pen = TRANSPARENCY_NONE;
		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				0,0,						/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
	}

}else{

#endif

//	if (flipscreen)
//		flip = 1;
//	else
		flip = 0;
//

#ifdef BGDBG
if ( toaplan_dbg_sprite_only == 0 ){
#endif

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);

		if ( (layer == 0) && (priority >= 8 ) )
		{
			pen = TRANSPARENCY_NONE;
		}else{
			pen = TRANSPARENCY_PEN;
		}

		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			int xpos,ypos;

//			if ( flip ){
//				xpos = tinfo->xpos;
//				ypos = tinfo->ypos;
//			}
//			else{
				xpos = tinfo->xpos;
				ypos = tinfo->ypos;
//			}

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
		priority++;
	}
#ifdef BGDBG
}
#endif

#ifdef BGDBG
}
#endif

}



static void zerowing_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	palette_set_color(0,0x00,0x00,0x00);
	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);


	flip = 0;

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);
		pen = TRANSPARENCY_PEN;

		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			int xpos,ypos;

			xpos = tinfo->xpos;
			ypos = tinfo->ypos;

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
		priority++;
	}

}


static void demonwld_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	flip = 0;

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);

		if ( priority == 1 )
			pen = TRANSPARENCY_NONE;
		else
			pen = TRANSPARENCY_PEN;

		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			int xpos,ypos;

			xpos = tinfo->xpos;
			ypos = tinfo->ypos;

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
		priority++;
	}

}


static void rallybik_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	for ( priority = 0; priority < 16; priority++ )	/* draw priority layers in order */
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]);
		/* hack to fix black blobs in Demon's World sky */
		if ( priority == 1 )
			pen = TRANSPARENCY_NONE;
		else
			pen = TRANSPARENCY_PEN;
		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			/* hack to fix blue blobs in Zero Wing attract mode */
			if ((pen == TRANSPARENCY_NONE) && ((tinfo->color&0x3f)==0))
				pen = TRANSPARENCY_PEN;

			drawgfx(bitmap,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
				tinfo->tile_num,
				(tinfo->color&0x3f), 			/* bit 7 not for colour */
				(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
	}
}



static void toaplan1_sprite_render (struct mame_bitmap *bitmap)
{
	int i,j;
	int priority;
	int flip;
	tile_struct *tinfo;
	tile_struct *tinfo_sp;
	struct rectangle sp_rect;

	flip = 0;

	fillbitmap (tmpbitmap1, Machine->pens[0],&Machine->visible_area);
	tinfo_sp = (tile_struct *)&(tile_list[16][0]);
	for ( i = 0; i < tile_count[16]; i++ )	/* draw sprite No. in order */
	{
		int	flipx,flipy;

		sp_rect.min_x = tinfo_sp->xpos;
		sp_rect.min_y = tinfo_sp->ypos;
		sp_rect.max_x = tinfo_sp->xpos + 7;
		sp_rect.max_y = tinfo_sp->ypos + 7;


		flipx = (tinfo_sp->color & 0x0100);
		flipy = (tinfo_sp->color & 0x0200);
//		if (flipscreen){
//			flipx = !flipx;
//			flipy = !flipy;
//		}

		fillbitmap (tmpbitmap2, Machine->pens[0], &sp_rect);
		drawgfx(tmpbitmap2,Machine->gfx[1],
			tinfo_sp->tile_num,
			(tinfo_sp->color&0x3f), 			/* bit 7 not for colour */
			flipx,flipy,					/* flipx,flipy */
			tinfo_sp->xpos,tinfo_sp->ypos,
			&Machine->visible_area,TRANSPARENCY_PEN,0
		);
		fillbitmap (tmpbitmap3, Machine->pens[0], &sp_rect);

		priority = tinfo_sp->priority;
		{
		int ix0,ix1,ix2,ix3;
		int dirty;

		dirty = 0;
		for ( j = 0; j < 4; j++ )
		{
			int x,y;

			y = tinfo_sp->ypos+layer_scrolly[j];
			x = tinfo_sp->xpos+layer_scrollx[j];
			ix0 = ( y   /8) * 41 +  x   /8;
			ix1 = ( y   /8) * 41 + (x+7)/8;
			ix2 = ((y+7)/8) * 41 +  x   /8;
			ix3 = ((y+7)/8) * 41 + (x+7)/8;

			if(	(ix0 >= 0) && (ix0 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix0]);
				if( tinfo->priority >= tinfo_sp->priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if(	(ix0 != ix1) && (ix1 >= 0) && (ix1 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix1]);
				if( tinfo->priority >= tinfo_sp->priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if(	(ix1 != ix2) && (ix2 >= 0) && (ix2 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix2]);
				if( tinfo->priority >= tinfo_sp->priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if( (ix2 != ix3) && (ix3 >= 0) && (ix3 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix3]);
				if( tinfo->priority >= tinfo_sp->priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
		}
		if( dirty != 0 )
		{
			toaplan1_sprite_mask(
				tmpbitmap2,
				tmpbitmap3,
				&sp_rect
			);
			fillbitmap (tmpbitmap3, Machine->pens[0], &sp_rect);
			drawgfx(tmpbitmap3,Machine->gfx[1],
				tinfo_sp->tile_num,
				(tinfo_sp->color&0x3f),
				flipx,flipy,
				tinfo_sp->xpos,tinfo_sp->ypos,
				&Machine->visible_area,TRANSPARENCY_PEN,0
			);
			toaplan1_sprite_copy(
				tmpbitmap1,
				tmpbitmap2,
				tmpbitmap3,
				&sp_rect
			);
		}else{
			drawgfx(tmpbitmap1,Machine->gfx[1],
				tinfo_sp->tile_num,
				(tinfo_sp->color&0x3f),
				flipx,flipy,
				tinfo_sp->xpos,tinfo_sp->ypos,
				&Machine->visible_area,TRANSPARENCY_PEN,0
			);
		}
		tinfo_sp++;
		}
	}
	copybitmap(bitmap, tmpbitmap1, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_PEN, 0);

}



void toaplan1_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles(tiles_offsetx,tiles_offsety);

	toaplan1_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

void zerowing_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles(tiles_offsetx,tiles_offsety);

	zerowing_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

void demonwld_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles(tiles_offsetx,tiles_offsety);

	demonwld_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

void rallybik_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	rallybik_find_tiles();
	rallybik_find_sprites();

	rallybik_render(bitmap);
}



/****************************************************************************
	Spriteram is always 1 frame ahead, suggesting spriteram buffering.
	There are no CPU output registers that control this so we
	assume it happens automatically every frame, at the end of vblank
****************************************************************************/

void toaplan1_eof_callback(void)
{
	memcpy(toaplan1_buffered_spriteram16, toaplan1_spriteram16, TOAPLAN1_SPRITERAM16_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM16_SIZE);
}

void rallybik_eof_callback(void)
{
	buffer_spriteram16_w(0, 0, 0);
}

void samesame_eof_callback(void)
{
	memcpy(toaplan1_buffered_spriteram16, toaplan1_spriteram16, TOAPLAN1_SPRITERAM16_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM16_SIZE);
	cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);  /* Frame done */
}

