/***************************************************************************

  vidhrdw/toaplan1.c

  Functions to emulate the video hardware of the machine.

  There are 4 scrolling layers of graphics, stored in planes of 64x64 tiles.
  Each tile in each plane is assigned a priority between 1 and 15, higher
  numbers have greater priority.

  Each tile takes up 32 bits, the format is:

  ---- ---- ---- ---- --xx xxxx xxxx xxxx = tile number (0 - $3fff)
  ---- ---- --xx xxxx ---- ---- ---- ---- = color (0 - $3f)
  xxxx ---- ---- ---- ---- ---- ---- ---- = priority (0-$f)

  The tiles use a palette of 1024 colors, the sprites use a different palette
  of 1024 colors.

***************************************************************************/

#include "driver.h"
#include "palette.h"
#include "vidhrdw/generic.h"

#define VIDEORAM1_SIZE	0x800   /* size in bytes - sprite ram */
#define VIDEORAM2_SIZE	0x100   /* size in bytes - sprite size ram */
#define VIDEORAM3_SIZE	0x4000  /* size in bytes - tile ram */

unsigned char *zerowing_videoram1;
unsigned char *zerowing_videoram2;
unsigned char *zerowing_videoram3;

unsigned char *zerowing_colorram1;
unsigned char *zerowing_colorram2;

int colorram1_size;
int colorram2_size;

unsigned int scrollregs[8] ;
unsigned int vblank ;
unsigned int framedone  ;
unsigned int num_tiles ;

unsigned int video_ofs;
unsigned int video_ofs3;
unsigned int fdflag;

extern int int_enable ;

typedef struct
	{
	UINT16 tile_num ;
	UINT8  color ;
	UINT16 xpos ;
	UINT16 ypos ;
	} tile_struct ;

tile_struct *tile_list[16] ;
tile_struct *temp_list ;
static int max_list_size[16];

int zerowing_vh_start(void)
{
	int i;

	zerowing_videoram1 = calloc (VIDEORAM1_SIZE, 1);
	zerowing_videoram2 = calloc (VIDEORAM2_SIZE, 1);
	zerowing_videoram3 = calloc (VIDEORAM3_SIZE * 4, 1); /* 4 layers */

	if (errorlog) fprintf (errorlog, "colorram_size: %08x\n", colorram1_size + colorram2_size);
	paletteram = calloc (colorram1_size + colorram2_size,1);

	for ( i=0; i<16; i++ )
		{
		max_list_size[i]=512;
		tile_list[i]=(tile_struct *)malloc(max_list_size[i]*sizeof(tile_struct)) ;
		memset(tile_list[i],0,max_list_size[i]*sizeof(tile_struct));
		}

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8) ;

	video_ofs = video_ofs3 = fdflag = 0;

	return 0;
}

void zerowing_vh_stop(void)
{
	int i ;

	free(zerowing_videoram1);
	free(zerowing_videoram2);
	free(zerowing_videoram3);

	for (i=0;i<16;i++)
		{
		free(tile_list[i]);
		}

	free (paletteram);
}

int vblank_r(int offset)
{
	return vblank ^= 1;
}

int framedone_r(int offset)
{
	framedone += 1 ;
	if ( (!int_enable) && (fdflag==0) )
		return 1;
	return fdflag ;
}

void framedone_w(int offset, int data)
{
	fdflag = data ;
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
void zerowing_colorram1_w(int offset, int data)
{
	WRITE_WORD (&zerowing_colorram1[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset,data);
}

int zerowing_colorram1_r(int offset)
{
	return READ_WORD (&zerowing_colorram1[offset]);
}

/* sprite palette */
void zerowing_colorram2_w(int offset, int data)
{
	WRITE_WORD (&zerowing_colorram2[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset+colorram1_size,data);
}

int zerowing_colorram2_r(int offset)
{
	return READ_WORD (&zerowing_colorram2[offset]);
}

void zerowing_videoram1_w(int offset, int data)
{
	int oldword = READ_WORD (&zerowing_videoram1[2*video_ofs & (VIDEORAM1_SIZE-1)]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
if (2*video_ofs >= VIDEORAM1_SIZE)
{
	if (errorlog) fprintf (errorlog, "videoram1_w, %08x\n", 2*video_ofs);
	return;
}
#endif

	WRITE_WORD (&zerowing_videoram1[2*video_ofs & (VIDEORAM1_SIZE-1)],newword);
	video_ofs++;
}

int zerowing_videoram1_r(int offset)
{
	return READ_WORD (&zerowing_videoram1[2*(video_ofs & (VIDEORAM1_SIZE-1))]);
}

void zerowing_videoram2_w(int offset, int data)
{
	int oldword = READ_WORD (&zerowing_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
if (2*video_ofs >= VIDEORAM2_SIZE)
{
	if (errorlog) fprintf (errorlog, "videoram2_w, %08x\n", 2*video_ofs);
	return;
}
#endif

	WRITE_WORD (&zerowing_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)],newword);
	video_ofs++;
}

int zerowing_videoram2_r(int offset)
{
	return READ_WORD (&zerowing_videoram2[2*video_ofs & (VIDEORAM2_SIZE-1)]);
}

void video_ofs3_w(int offset, int data)
{
	video_ofs3 = data ;
}

int video_ofs3_r(int offset)
{
	return video_ofs3;
}

void zerowing_videoram3_w(int offset, int data)
{
	int oldword = READ_WORD (&zerowing_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset]);
	int newword = COMBINE_WORD (oldword, data);

#ifdef DEBUG
if ((video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset >= VIDEORAM3_SIZE*4)
{
	if (errorlog) fprintf (errorlog, "videoram3_w, %08x\n", 2*video_ofs3);
	return;
}
#endif

	WRITE_WORD (&zerowing_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset],newword);
	if ( offset == 2 ) video_ofs3++;
}

int zerowing_videoram3_r(int offset)
{
	return READ_WORD (&zerowing_videoram3[(video_ofs3 & (VIDEORAM3_SIZE-1))*4 + offset]);
}

int scrollregs_r(int offset)
{
	return scrollregs[offset>>1];
}

void scrollregs_w(int offset, int data)
{
	scrollregs[offset>>1] = data ;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.

***************************************************************************/

static int tile_count[16];

static void toaplan1_update_palette (void)
{
	int i;
	int priority;
	int color;
	unsigned short palette_map[64*2];

	memset (palette_map, 0, sizeof (palette_map));

	/* extract color info from priority layers in order */
	for (priority = 1; priority < 16; priority++ )
	{
		tile_struct *tinfo;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		/* draw only tiles in list */
		for ( i = 0; i < tile_count[priority]; i++ )
		{
			int bank;

			bank = tinfo->color >> 7;
			color = tinfo->color & 0x3f;
			palette_map[color + bank*64] |= Machine->gfx[bank]->pen_usage[tinfo->tile_num];

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
			for (j = 1; j < 16; j++)
				if (usage & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
				else
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
		}
		else
			memset(&palette_used_colors[i * 16],PALETTE_COLOR_UNUSED,16);
	}

	palette_recalc ();
}

static void toaplan1_find_tiles (void)
{
	int priority;
	int layer;
	int width;
	tile_struct *tinfo;
	unsigned char *t_info;

	width = Machine->drv->screen_width/8 + 1;

	for ( priority = 0 ; priority < 16 ; priority++ )
	{
		tile_count[priority]=0;
	}

	for ( layer = 0 ; layer <4 ; layer++ )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

		t_info = (zerowing_videoram3+layer * VIDEORAM3_SIZE);
		scrollx = scrollregs[layer*2];
		scrolly = scrollregs[(layer*2)+1];

		scrollx >>= 7 ;
		scrollx += 56 ;
		if ( layer==0 ) scrollx += 8 ;
		scrollx &= 0x1ff ;
		offsetx = scrollx / 8 ;

		scrolly >>= 7 ;
		scrolly += 15 ;
		scrolly &= 0x1ff ;
		offsety = scrolly / 8 ;

		for ( sy = 0 ; sy < 32 ; sy++ )
		{
			i = ((sy+offsety)&0x3f)*128 ;
			for ( sx = 0 ; sx <= width ; sx++ )
		 	{
				tattr = READ_WORD(&t_info[2*(i+2*((sx+offsetx)&0x3f))]);
				if ( tattr )
				{
					priority = tattr >> 12 ;
					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
					tinfo->tile_num = READ_WORD(&t_info[2*(2*(((sy+offsety)&0x3f)*64+((sx+offsetx)&0x3f))+1)]) & 0x3fff ;
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

static void toaplan1_find_sprites (void)
{
	int sprite;
	unsigned char *s_info,*s_size;

	s_size = (zerowing_videoram2);	/* sprite block size */
	s_info = (zerowing_videoram1) ;	/* start of sprite ram */

	for ( sprite = 0 ; sprite < 256 ; sprite++ )
	{
		int tattr;

		tattr = READ_WORD (&s_info[2]);
		if ( tattr & 0xc000 )	/* no need to render hidden sprites */
		{
			int sx,sy,dx,dy,s_sizex,s_sizey,tchar;
			int sprite_size_ptr;
			int priority;

			sx=READ_WORD(&s_info[4]);
			sx >>= 7 ;
			if ( sx > 416 ) sx -= 512 ;

			sy=READ_WORD(&s_info[6])-0x800;
			sy >>= 7 ;
			if ( sy > 416 ) sy -= 512 ;

			priority = tattr >> 12 ;
			tchar = READ_WORD(&s_info[0])&0x7fff;

			sprite_size_ptr = (tattr>>6)&0x3f ;
			s_sizey = (READ_WORD(&s_size[2*sprite_size_ptr])>>4)&0xf ;
			s_sizex = (READ_WORD(&s_size[2*sprite_size_ptr]))&0xf ;
			for ( dy = s_sizey ; dy > 0 ; dy-- )
				for ( dx = s_sizex; dx > 0 ; dx-- )
				{
					tile_struct *tinfo;

					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
					tinfo->tile_num = tchar++ ;
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
				}
		}
		s_info += 8 ;
	}

}

static void vimana_find_sprites (void)
{
	int sprite;
	unsigned char *s_info,*s_size;

	s_size = (zerowing_videoram2);	/* sprite block size */
	s_info = (zerowing_videoram1) ;	/* start of sprite ram */

	for ( sprite = 0 ; sprite < 256 ; sprite++ )
	{
		int tattr;

		tattr = READ_WORD (&s_info[2]);
		if ( tattr & 0xc000 )	/* no need to render hidden sprites */
		{
			int sx,sy,dx,dy,s_sizex,s_sizey,tchar;
			int sprite_size_ptr;
			int priority;

			sx=READ_WORD(&s_info[4]);
			sx >>= 7 ;
			if ( sx > 416 ) sx -= 512 ;

			sy=READ_WORD(&s_info[6])-0x800;
			sy >>= 7 ;
			if ( sy > 416 ) sy -= 512 ;

			priority = tattr >> 12 ;
			tchar = READ_WORD(&s_info[0])&0x7fff;

			sprite_size_ptr = (tattr>>6)&0x3f ;
			s_sizey = (READ_WORD(&s_size[2*sprite_size_ptr])>>4)&0xf ;
			s_sizex = (READ_WORD(&s_size[2*sprite_size_ptr]))&0xf ;
			for ( dy = s_sizey ; dy > 0 ; dy-- )
				for ( dx = s_sizex; dx > 0 ; dx-- )
				{
					tile_struct *tinfo;

					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]) ;
					tinfo->tile_num = tchar++ ;
					tinfo->color = 0x80 | (tattr & 0x3f) ;
					tinfo->xpos = sx-dx*8+s_sizex*8 ;
					tinfo->ypos = sy-dy*8+s_sizey*8+16  ;
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
		s_info += 8 ;
	}
}

static void toaplan1_render (struct osd_bitmap *bitmap)
{
	int i;
	int priority;
	tile_struct *tinfo;

	fillbitmap (bitmap, palette_transparent_pen, &Machine->drv->visible_area);

	for ( priority = 1 ; priority < 16 ; priority++ )	/* draw priority layers in order */
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		for ( i = 0 ; i < tile_count[priority] ; i++ )	/* draw only tiles in list */
		{
			drawgfx(bitmap,Machine->gfx[tinfo->color>>7], 	/* bit 7 set for sprites */
				tinfo->tile_num,
				tinfo->color&0x3f, 			/* bit 7 not for colour */
				0,0,
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			tinfo++ ;
		}
	}
}

void zerowing_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_tiles ();
	toaplan1_find_sprites ();

	toaplan1_update_palette ();
	toaplan1_render (bitmap);
}

/* layer and sprite offsets are different from zerowing */
void vimana_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* discover what data will be drawn */
	toaplan1_find_tiles ();
	vimana_find_sprites ();

	toaplan1_update_palette ();
	toaplan1_render (bitmap);
}

