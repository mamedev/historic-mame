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


 Sprite RAM format

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

#define VIDEORAM1_SIZE	0x1000		/* size in bytes - sprite ram */
#define VIDEORAM2_SIZE	0x100		/* size in bytes - sprite size ram */
#define VIDEORAM3_SIZE	0x4000		/* size in bytes - tile ram */

unsigned char *toaplan1_videoram1;
unsigned char *toaplan1_videoram2;
unsigned char *toaplan1_videoram3;

unsigned char *toaplan1_colorram1;
unsigned char *toaplan1_colorram2;

int colorram1_size;
int colorram2_size;

unsigned int scrollregs[8] ;
unsigned int vblank ;
unsigned int demonwld_c ;
unsigned int framedone  ;
unsigned int num_tiles ;

unsigned int video_ofs;
unsigned int video_ofs3;

int toaplan1_flipscreen;
int tiles_offsetx ;
int tiles_offsety ;
int layers_offset[4] ;

unsigned int fdflag;

extern int toaplan1_int_enable ;

typedef struct
	{
	UINT16 tile_num ;
	UINT16 color ;
	int xpos ;
	int ypos ;
	} tile_struct ;

tile_struct *tile_list[32] ;
tile_struct *temp_list ;
static int max_list_size[32];
static int tile_count[32];

tile_struct *sp_list;
static int sp_max_list_size;
static int sp_count;

struct osd_bitmap *tmpbitmap1;
static struct osd_bitmap *tmpbitmap2;


int toaplan1_vh_start(void)
{
	int i;

	tmpbitmap1 = osd_new_bitmap(
					Machine->drv->screen_width,
					Machine->drv->screen_height,
					Machine->scrbitmap->depth);

	tmpbitmap2 = osd_new_bitmap(
					Machine->drv->screen_width,
					Machine->drv->screen_height,
					Machine->scrbitmap->depth);

//	tmpbitmap = osd_new_bitmap(
//			Machine->drv->visible_area.max_x - Machine->drv->visible_area.min_x + 1,
//			Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1,
//			Machine->scrbitmap->depth);

	toaplan1_videoram1 = calloc (VIDEORAM1_SIZE, 1);
	toaplan1_videoram2 = calloc (VIDEORAM2_SIZE, 1);
	toaplan1_videoram3 = calloc (VIDEORAM3_SIZE * 4, 1); /* 4 layers */

	if (errorlog) fprintf (errorlog, "colorram_size: %08x\n", colorram1_size + colorram2_size);
	paletteram = calloc (colorram1_size + colorram2_size,1);

	for ( i=0; i<32; i++ )
	{
		max_list_size[i] = 32768;
		tile_list[i]=(tile_struct *)malloc(max_list_size[i]*sizeof(tile_struct)) ;
		memset(tile_list[i],0,max_list_size[i]*sizeof(tile_struct));
	}

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8) ;

	video_ofs = video_ofs3 = fdflag = 0;

	sp_max_list_size = 32768;
	sp_list = (tile_struct *)malloc(sp_max_list_size * sizeof(tile_struct));
	memset(sp_list,0,sp_max_list_size * sizeof(tile_struct));

	return 0;
}

void toaplan1_vh_stop(void)
{
	int i ;

	osd_free_bitmap(tmpbitmap1);
	osd_free_bitmap(tmpbitmap2);

	free(toaplan1_videoram1);
	free(toaplan1_videoram2);
	free(toaplan1_videoram3);

	for (i=0;i<32;i++)
	{
		free(tile_list[i]);
	}

	free (sp_list);

	free (paletteram);

	if (errorlog) fprintf (errorlog, "sp_max_list_size:%08x\n",sp_max_list_size);
;

}

int toaplan1_vblank_r(int offset)
{
	return vblank ^= 1;
}

int demonwld_r(int offset)
{
	demonwld_c++ ;
	if (demonwld_c & 4 )
		return 0x76;
	else
		return 0 ;
}

int toaplan1_framedone_r(int offset)
{
	framedone += 1 ;
	if ( (!toaplan1_int_enable) && (fdflag==0) )
		return 1;
	return fdflag ;
}

void toaplan1_framedone_w(int offset, int data)
{
	fdflag = data ;
}

void toaplan1_flipscreen_w(int offset, int data)
{
	toaplan1_flipscreen = data; /* 8000 flip, 0000 dont */
}

void video_ofs_w(int offset, int data)
{
	video_ofs = data ;
}

int video_ofs_r(int offset)
{
	return video_ofs ;
}

/* tile palette */
void toaplan1_colorram1_w(int offset, int data)
{
	WRITE_WORD (&toaplan1_colorram1[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset,data);
}

int toaplan1_colorram1_r(int offset)
{
	return READ_WORD (&toaplan1_colorram1[offset]);
}

/* sprite palette */
void toaplan1_colorram2_w(int offset, int data)
{
	WRITE_WORD (&toaplan1_colorram2[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset+colorram1_size,data);
}

int toaplan1_colorram2_r(int offset)
{
	return READ_WORD (&toaplan1_colorram2[offset]);
}

void toaplan1_videoram1_w(int offset, int data)
{
	int oldword = READ_WORD (&toaplan1_videoram1[2*video_ofs & (VIDEORAM1_SIZE-1)]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
	if (2*video_ofs >= VIDEORAM1_SIZE)
	{
		if (errorlog) fprintf (errorlog, "videoram1_w, %08x\n", 2*video_ofs);
		return;
	}
#endif

	WRITE_WORD (&toaplan1_videoram1[2*video_ofs & (VIDEORAM1_SIZE-1)],newword);
	video_ofs++;
}

int toaplan1_videoram1_r(int offset)
{
	return READ_WORD (&toaplan1_videoram1[2*(video_ofs & (VIDEORAM1_SIZE-1))]);
}

void toaplan1_videoram2_w(int offset, int data)
{
	int oldword = READ_WORD (&toaplan1_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
	if (2*video_ofs >= VIDEORAM2_SIZE)
	{
		if (errorlog) fprintf (errorlog, "videoram2_w, %08x\n", 2*video_ofs);
		return;
	}
#endif

	WRITE_WORD (&toaplan1_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)],newword);
	video_ofs++;
}

int toaplan1_videoram2_r(int offset)
{
	return READ_WORD (&toaplan1_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)]);
}

void video_ofs3_w(int offset, int data)
{
	video_ofs3 = data ;
}

int video_ofs3_r(int offset)
{
	return video_ofs3;
}

void toaplan1_videoram3_w(int offset, int data)
{
	int oldword = READ_WORD (&toaplan1_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
	if ((video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset >= VIDEORAM3_SIZE*4)
	{
		if (errorlog) fprintf (errorlog, "videoram3_w, %08x\n", 2*video_ofs3);
		return;
	}
#endif

	WRITE_WORD (&toaplan1_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset],newword);
	if ( offset == 2 ) video_ofs3++;
}

int toaplan1_videoram3_r(int offset)
{
	return READ_WORD (&toaplan1_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset]);
}

int rallybik_videoram1_r(int offset)
{
	return READ_WORD(&toaplan1_videoram1[offset]);
}

void rallybik_videoram1_w(int offset, int data)
{
	WRITE_WORD(&toaplan1_videoram1[offset],data);
}

int scrollregs_r(int offset)
{
	return scrollregs[offset>>1];
}

void scrollregs_w(int offset, int data)
{
	scrollregs[offset>>1] = data ;
}

void offsetregs_w(int offset, int data)
{
	if ( offset == 0 )
		tiles_offsetx = data ;
	else
		tiles_offsety = data ;
}

void layers_offset_w(int offset, int data)
{
	switch (offset)
	{
/*		case 0:
			layers_offset[0] = (data&0xff) - 0xdb ;
			break ;
		case 2:
			layers_offset[1] = (data&0xff) - 0x14 ;
			break ;
		case 4:
			layers_offset[2] = (data&0xff) - 0x85 ;
			break ;
		case 6:
			layers_offset[3] = (data&0xff) - 0x07 ;
			break ;
*/
		case 0:
			layers_offset[0] = data;
			break ;
		case 2:
			layers_offset[1] = data;
			break ;
		case 4:
			layers_offset[2] = data;
			break ;
		case 6:
			layers_offset[3] = data;
			break ;
	}

	if (errorlog) fprintf (errorlog, "layers_offset[0]:%08x\n",layers_offset[0]);
	if (errorlog) fprintf (errorlog, "layers_offset[1]:%08x\n",layers_offset[1]);
	if (errorlog) fprintf (errorlog, "layers_offset[2]:%08x\n",layers_offset[2]);
	if (errorlog) fprintf (errorlog, "layers_offset[3]:%08x\n",layers_offset[3]);

}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.

***************************************************************************/


static void toaplan1_update_palette (void)
{
	int i;
	int priority;
	int color;
	unsigned short palette_map[64*2];

	memset (palette_map, 0, sizeof (palette_map));

	/* extract color info from priority layers in order */
	for (priority = 0; priority < 32; priority++ )
	{
		tile_struct *tinfo;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		/* draw only tiles in list */
		for ( i = 0; i < tile_count[priority]; i++ )
		{
			int bank;

			bank  = (tinfo->color >> 7) & 1;
			color = (tinfo->color & 0x3f);
			palette_map[color + bank*64] |= Machine->gfx[bank]->pen_usage[tinfo->tile_num & (Machine->gfx[bank]->total_elements-1)];

			tinfo++;
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0; i < 64*2; i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 0; j < 16; j++)
			{
				if (usage & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
				else
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
			}
		}
		else
			memset(&palette_used_colors[i * 16],PALETTE_COLOR_UNUSED,16);
	}

	palette_recalc ();
}



static void toaplan1_find_tiles(int xoffs,int yoffs)
{
	int priority;
	int layer;
	tile_struct *tinfo;
	unsigned char *t_info;

	for ( layer = 3 ; layer >= 0 ; layer-- )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

		t_info = (toaplan1_videoram3+layer * VIDEORAM3_SIZE);
		scrollx = scrollregs[layer*2];
		scrolly = scrollregs[(layer*2)+1];

		scrollx >>= 7 ;

		scrollx += (495 - xoffs) ;		// 495 - xoffs
		if ( layer == 0 ) scrollx += 6 ;
		if ( layer == 1 ) scrollx += 4 ;
		if ( layer == 2 ) scrollx += 2 ;
//		scrollx += layers_offset[layer] ;


		offsetx = scrollx / 8 ;

		scrolly >>= 7 ;
		scrolly += (0x101 - yoffs) ;		// 257 - yoffs
		offsety = scrolly / 8 ;

		for ( sy = 0 ; sy < 32 ; sy++ )
		{
			for ( sx = 0 ; sx <= 40 ; sx++ )
			{
				i = ((sy+offsety)&0x3f)*256 + ((sx+offsetx)&0x3f)*4 ;
				tattr = READ_WORD(&t_info[i]);
				priority = (tattr >> 12);
				if ( (priority) || (layer == 0) )	/* if priority 0 draw layer 0 only */
				{
					priority = priority * 2 + 1;
//					priority = (layer+1) * 2 + 1;	/* for debug */
					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
					tinfo->tile_num = READ_WORD(&t_info[i+2]) ;
					if ( !(tinfo->tile_num & 0x8000) )
					{
						tinfo->color = tattr & 0x3f ;
						tinfo->color |= layer<<8;
						tinfo->xpos = (sx*8)-(scrollx&0x7) ;
						tinfo->ypos = (sy*8)-(scrolly&0x7) ;
						tile_count[priority]++ ;
						if(tile_count[priority]==max_list_size[priority])
						{
							/*reallocate tile_list[priority] to larger size */
							temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512)) ;
							memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
							max_list_size[priority]+=512;
							free(tile_list[priority]);
							tile_list[priority] = temp_list ;
						}
					}
				}
			}
		}
	}
}

static void rallybik_find_tiles(void)
{
	int priority;
	int layer;
	tile_struct *tinfo;
	unsigned char *t_info;

	for ( priority = 0 ; priority < 16 ; priority++ )
	{
		tile_count[priority]=0;
	}

	for ( layer = 3 ; layer >= 0 ; layer-- )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

		t_info = (toaplan1_videoram3+layer * VIDEORAM3_SIZE);
		scrollx = scrollregs[layer*2];
		scrolly = scrollregs[(layer*2)+1];

		scrollx >>= 7 ;
		scrollx += 43 ;
		if ( layer == 0 ) scrollx += 2 ;
		if ( layer == 2 ) scrollx -= 2 ;
		if ( layer == 3 ) scrollx -= 4 ;
		offsetx = scrollx / 8 ;

		scrolly >>= 7 ;
		scrolly += 21 ;
		offsety = scrolly / 8 ;

		for ( sy = 0 ; sy < 32 ; sy++ )
		{
			for ( sx = 0 ; sx <= 40 ; sx++ )
			{
				i = ((sy+offsety)&0x3f)*256 + ((sx+offsetx)&0x3f)*4 ;
				tattr = READ_WORD(&t_info[i]);
				priority = tattr >> 12 ;
				if ( (priority) || (layer == 0) )	/* if priority 0 draw layer 0 only */
				{
					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
					tinfo->tile_num = READ_WORD(&t_info[i+2]) ;

					if ( !((priority) && (tinfo->tile_num & 0x8000)) )
					{
						tinfo->tile_num &= 0x3fff ;
						tinfo->color = tattr & 0x3f ;
						tinfo->xpos = (sx*8)-(scrollx&0x7) ;
						tinfo->ypos = (sy*8)-(scrolly&0x7) ;
						tile_count[priority]++ ;
						if(tile_count[priority]==max_list_size[priority])
						{
							/*reallocate tile_list[priority] to larger size */
							temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512)) ;
							memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
							max_list_size[priority]+=512;
							free(tile_list[priority]);
							tile_list[priority] = temp_list ;
						}
					}
				}
			}
		}
	}
}

static void toaplan1_find_sprites (void)
{
	int priority;
	int sprite;
	unsigned char *s_info,*s_size;


	sp_count = 0;

	for ( priority = 0 ; priority < 32 ; priority++ )
	{
		tile_count[priority]=0;
	}

	s_size = (toaplan1_videoram2);	/* sprite block size */
	s_info = (toaplan1_videoram1) ;	/* start of sprite ram */

	for ( sprite = 0 ; sprite < 256 ; sprite++ )
	{
		int tattr,tchar;

		tchar = READ_WORD (&s_info[0]) & 0xffff;
		tattr = READ_WORD (&s_info[2]);

		if ( (tattr & 0xf000) && ((tchar & 0x8000) == 0) )
		{
			int sx,sy,dx,dy,s_sizex,s_sizey;
			int sprite_size_ptr;

			sx=READ_WORD(&s_info[4]);
			sx >>= 7 ;
			if ( sx > 416 ) sx -= 512 ;

			sy=READ_WORD(&s_info[6]);
			sy >>= 7 ;
			if ( sy > 416 ) sy -= 512 ;

			priority = (tattr >> 12) * 2;

			sprite_size_ptr = (tattr>>6)&0x3f ;
			s_sizey = (READ_WORD(&s_size[2*sprite_size_ptr])>>4)&0xf ;
			s_sizex = (READ_WORD(&s_size[2*sprite_size_ptr]))&0xf ;

			for ( dy = s_sizey ; dy > 0 ; dy-- )
			for ( dx = s_sizex; dx > 0 ; dx-- )
			{
				tile_struct *tinfo;

				tinfo = &sp_list[sp_count];
				tinfo->tile_num = tchar;
				tinfo->color = 0x80 | (tattr & 0x3f) ;
				tinfo->xpos = sx-dx*8+s_sizex*8 ;
				tinfo->ypos = sy-dy*8+s_sizey*8 ;
				sp_count++;
				if(sp_count == sp_max_list_size)
				{
					/*reallocate sp_list[] to larger size */
					temp_list=(tile_struct *)malloc((sp_max_list_size+512) * sizeof(tile_struct));
					memcpy(temp_list,sp_list,sizeof(tile_struct)*sp_max_list_size);
					sp_max_list_size+=512;
					free(sp_list);
					sp_list = temp_list;
				}

				tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
				tinfo->tile_num = tchar;
				tinfo->color = 0x80 | (tattr & 0x3f) ;
				tinfo->xpos = sx-dx*8+s_sizex*8 ;
				tinfo->ypos = sy-dy*8+s_sizey*8 ;
				tile_count[priority]++ ;
				if(tile_count[priority]==max_list_size[priority])
				{
					/*reallocate tile_list[priority] to larger size */
					temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512)) ;
					memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
					max_list_size[priority]+=512;
					free(tile_list[priority]);
					tile_list[priority] = temp_list ;
				}
				tchar++;
			}
		}
		s_info += 8 ;
	}
}

static void rallybik_find_sprites (void)
{
	int sprite;
	unsigned char *s_info;
	int tattr;
	int sx,sy,tchar;
	int priority;
	tile_struct *tinfo;

	s_info = (toaplan1_videoram1) ;	/* start of sprite ram */

	for ( sprite = 0 ; sprite < 512 ; sprite++ )
	{
		tattr = READ_WORD(&s_info[2]);
		if ( tattr )	/* no need to render hidden sprites */
		{
			sx=READ_WORD(&s_info[4]);
			sx >>= 7 ;
			sx &= 0x1ff ;
			if ( sx > 416 ) sx -= 512 ;

			sy=READ_WORD(&s_info[6]);
			sy >>= 7 ;
			sy &= 0x1ff ;
			if ( sy > 416 ) sy -= 512 ;

			priority = (tattr>>8) & 0xc ;
			tchar = READ_WORD(&s_info[0]);
			tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
			tinfo->tile_num = tchar & 0x7fff ;
			tinfo->color = 0x80 | (tattr&0x3f) ;
			tinfo->color |= (tattr & 0x0100) ;
			tinfo->color |= (tattr & 0x0200) ;
			if (tinfo->color & 0x0100) sx -= 15;

			tinfo->xpos = sx-31 ;
			tinfo->ypos = sy-16 ;
			tile_count[priority]++ ;
			if(tile_count[priority]==max_list_size[priority])
			{
				/*reallocate tile_list[priority] to larger size */
				temp_list=(tile_struct *)malloc(sizeof(tile_struct)*(max_list_size[priority]+512)) ;
				memcpy(temp_list,tile_list[priority],sizeof(tile_struct)*max_list_size[priority]);
				max_list_size[priority]+=512;
				free(tile_list[priority]);
				tile_list[priority] = temp_list ;
			}
		}  // if tattr
		s_info += 8 ;
	} // for sprite
}


/* suz */
void copybitmapmask(struct osd_bitmap *dest_bmp,struct osd_bitmap *source_bmp,
	struct osd_bitmap *mask_bmp,const struct rectangle *clip,int transparent_color)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;

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

	if (dest_bmp->depth != 16)
	{
		int ex = sx+source_bmp->width;
		int ey = sy+source_bmp->height;

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
				unsigned char *dest = dest_bmp->line[y];
				unsigned char *source = source_bmp->line[y];
				unsigned char *mask = mask_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
				{
					int c = mask[x];
					if( c != transparent_color )
						dest[x] = source[x];
				}
			}
		}
	}

	else
	{
		int ex = sx+source_bmp->width;
		int ey = sy+source_bmp->height;

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
				unsigned char *source = source_bmp->line[y];
				unsigned char *mask = mask_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
				{
					int c = mask[x];
					if( c != transparent_color )
						dest[x] = source[x];
				}
			}
		}
	}
}

// #define BGDBG
#ifdef BGDBG
int	toaplan_dbg_priority = 0;
#endif

static void toaplan1_render (struct osd_bitmap *bitmap)
{
	int i;
	int priority,pen;
	tile_struct *tinfo;
	struct rectangle sp_rect;

	fillbitmap (bitmap, palette_transparent_pen, &Machine->drv->visible_area);

#ifdef BGDBG

if (keyboard_pressed(KEYCODE_Q)) { toaplan_dbg_priority = 0; }
if (keyboard_pressed(KEYCODE_W)) { toaplan_dbg_priority = 3; }
if (keyboard_pressed(KEYCODE_E)) { toaplan_dbg_priority = 5; }
if (keyboard_pressed(KEYCODE_R)) { toaplan_dbg_priority = 7; }
if (keyboard_pressed(KEYCODE_T)) { toaplan_dbg_priority = 9; }
if (keyboard_pressed(KEYCODE_Y)) { toaplan_dbg_priority = 11; }
if (keyboard_pressed(KEYCODE_U)) { toaplan_dbg_priority = 13; }
if (keyboard_pressed(KEYCODE_I)) { toaplan_dbg_priority = 15; }
if (keyboard_pressed(KEYCODE_A)) { toaplan_dbg_priority = 17; }
if (keyboard_pressed(KEYCODE_S)) { toaplan_dbg_priority = 19; }
if (keyboard_pressed(KEYCODE_D)) { toaplan_dbg_priority = 21; }
if (keyboard_pressed(KEYCODE_F)) { toaplan_dbg_priority = 23; }
if (keyboard_pressed(KEYCODE_G)) { toaplan_dbg_priority = 25; }
if (keyboard_pressed(KEYCODE_H)) { toaplan_dbg_priority = 27; }
if (keyboard_pressed(KEYCODE_J)) { toaplan_dbg_priority = 29; }
if (keyboard_pressed(KEYCODE_K)) { toaplan_dbg_priority = 31; }

if( toaplan_dbg_priority != 0 ){

	priority = toaplan_dbg_priority;
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		/* hack to fix black blobs in Demon's World sky */
		pen = TRANSPARENCY_NONE ;
		for ( i = 0 ; i < tile_count[priority] ; i++ ) /* draw only tiles in list */
		{
			drawgfx(bitmap,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
				tinfo->tile_num,
				(tinfo->color&0x3f), 			/* bit 7 not for color */
				(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,pen,0);
			tinfo++ ;
		}
	}

}else{

#endif

	tinfo = sp_list;
	for ( i = 0 ; i < sp_count ; i++ )	/* draw sprite No. in order */
	{
		drawgfx(tmpbitmap1,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
			tinfo->tile_num,
			(tinfo->color&0x3f), 			/* bit 7 not for colour */
			(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
			tinfo->xpos,tinfo->ypos,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		tinfo++ ;
	}

	priority = 0;
	while ( priority < 32 )			/* draw priority layers in order */
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		for ( i = 0 ; i < tile_count[priority] ; i++ )
		{

			sp_rect.min_x = tinfo->xpos;
			sp_rect.min_y = tinfo->ypos;
			sp_rect.max_x = tinfo->xpos + 7;
			sp_rect.max_y = tinfo->ypos + 7;

			fillbitmap (tmpbitmap2, palette_transparent_pen, &sp_rect);

			drawgfx(tmpbitmap2,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
				tinfo->tile_num,
				(tinfo->color&0x3f), 			/* bit 7 not for colour */
				(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			copybitmapmask(
				bitmap,					// dist
				tmpbitmap1,				// src
				tmpbitmap2,				// mask
				&sp_rect,
				palette_transparent_pen
			);

			tinfo++ ;
		}
		priority++;


		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		/* hack to fix black blobs in Demon's World sky */
		/* truxtun background etc..						*/

		if ( ((tinfo->color >> 8) == 0) && (priority < 4) )
										/* Layer 0 && priority */
			pen = TRANSPARENCY_NONE ;
		else
			pen = TRANSPARENCY_PEN ;

		for ( i = 0 ; i < tile_count[priority] ; i++ ) /* draw only tiles in list */
		{
			/* hack to fix blue blobs in Zero Wing attract mode */
//			if ((pen == TRANSPARENCY_NONE) && ((tinfo->color&0x3f)==0))
//				pen = TRANSPARENCY_PEN ;

			drawgfx(bitmap,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
				tinfo->tile_num,
				(tinfo->color&0x3f), 			/* bit 7 not for colour */
				0,0,							/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,pen,0);
			tinfo++ ;
		}
		priority++;

	}

#ifdef BGDBG
}
#endif

}


static void rallybik_render (struct osd_bitmap *bitmap)
{
	int i;
	int priority,pen;
	tile_struct *tinfo;

	fillbitmap (bitmap, palette_transparent_pen, &Machine->drv->visible_area);

	for ( priority = 0 ; priority < 16 ; priority++ )	/* draw priority layers in order */
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		/* hack to fix black blobs in Demon's World sky */
		if ( priority == 1 )
			pen = TRANSPARENCY_NONE ;
		else
			pen = TRANSPARENCY_PEN ;
		for ( i = 0 ; i < tile_count[priority] ; i++ ) /* draw only tiles in list */
		{
			/* hack to fix blue blobs in Zero Wing attract mode */
			if ((pen == TRANSPARENCY_NONE) && ((tinfo->color&0x3f)==0))
				pen = TRANSPARENCY_PEN ;

			drawgfx(bitmap,Machine->gfx[(tinfo->color>>7)&1],	/* bit 7 set for sprites */
				tinfo->tile_num,
				(tinfo->color&0x3f), 			/* bit 7 not for colour */
				(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,pen,0);
			tinfo++ ;
		}
	}
}


void toaplan1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles(tiles_offsetx,tiles_offsety);

	toaplan1_update_palette();
	toaplan1_render(bitmap);
}

void rallybik_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	rallybik_find_tiles();
	rallybik_find_sprites();

	toaplan1_update_palette();
	rallybik_render(bitmap);
}

