/***************************************************************************
  vidhrdw.c
  Functions to emulate the video hardware of the machine.
***************************************************************************/

#include "driver.h"
#include "palette.h"
#include "vidhrdw/generic.h"

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

unsigned int video_ofs = 0 ;
unsigned int video_ofs3 = 0 ;
unsigned int fdflag = 0 ;

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
	int i,c;

	zerowing_videoram1 = calloc( 0x400,2 ) ;
	zerowing_videoram2 = calloc( 0x40,2 ) ;
	zerowing_videoram3 = calloc( 0x8000,2 ) ;

	if (errorlog) fprintf (errorlog, "colorram_size: %08x\n", colorram1_size + colorram2_size);
	paletteram = calloc (colorram1_size + colorram2_size,1);

	for ( i=0; i<16; i++ )
		{
		max_list_size[i]=512;
		tile_list[i]=(tile_struct *)malloc(max_list_size[i]*sizeof(tile_struct)) ;
		memset(tile_list[i],0,max_list_size[i]*sizeof(tile_struct));
		}

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8) ;
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

void zerowing_colorram1_w(int offset, int data)
{
	WRITE_WORD (&zerowing_colorram1[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset,data);
}

int zerowing_colorram1_r(int offset)
{
	return READ_WORD (&zerowing_colorram1[offset]);
}

void zerowing_colorram2_w(int offset, int data)
{
	WRITE_WORD (&zerowing_colorram2[offset], data);
	paletteram_xBBBBBGGGGGRRRRR_word_w(offset+0x800,data);
}

int zerowing_colorram2_r(int offset)
{
	return READ_WORD (&zerowing_colorram2[offset]);
}

void zerowing_videoram1_w(int offset, int data)
{
	int oldword = READ_WORD (&zerowing_videoram1[2*video_ofs]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&zerowing_videoram1[2*video_ofs],newword);
	video_ofs++;
}

int zerowing_videoram1_r(int offset)
{
	return READ_WORD (&zerowing_videoram1[2*video_ofs]);
}

void zerowing_videoram2_w(int offset, int data)
{
	int oldword = READ_WORD (&zerowing_videoram2[2*video_ofs]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&zerowing_videoram2[2*video_ofs],newword);
	video_ofs++;
}

int zerowing_videoram2_r(int offset)
{
	return READ_WORD (&zerowing_videoram2[2*video_ofs]);
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
	int oldword = READ_WORD (&zerowing_videoram3[(video_ofs3&0x3fff)*4+offset]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&zerowing_videoram3[(video_ofs3&0x3fff)*4+offset],newword);
	if ( offset == 2 ) video_ofs3++;
}

int zerowing_videoram3_r(int offset)
{
	return READ_WORD (&zerowing_videoram3[(video_ofs3&0x3fff)*4+offset]);
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
void zerowing_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,sprite,sprite_size_ptr,s_sizex,s_sizey;
	int sx,sy,dx,dy,tattr,layer;
	int scrolly,scrollx,offsetx,offsety;
	int priority,tchar ;
	int width,i,tile_count[16];
	tile_struct *tinfo ;

	unsigned char *t_info,*s_info,*s_size ;

	width = Machine->drv->screen_width/8 + 1;

	palette_recalc();

	for ( priority = 0 ; priority < 16 ; priority++ )
		{
		tile_count[priority]=0;
		}

	for ( layer = 0 ; layer <4 ; layer++ )
		{
		t_info = (zerowing_videoram3+layer*0x4000);
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
	s_size = (zerowing_videoram2);	/* sprite block size */
	s_info = (zerowing_videoram1) ;	/* start of sprite ram */

	for ( sprite = 0 ; sprite < 256 ; sprite++ )
		{
		tattr = READ_WORD (&s_info[2]);
		if ( tattr & 0xc000 )	/* no need to render hidden sprites */
			{
			sx=READ_WORD(&s_info[4]);
			sx >>= 7 ;
			if ( sx > 416 ) sx -= 512 ;

			sy=READ_WORD(&s_info[6])-0x800;
			sy >>= 7 ;
			if ( sy > 416 ) sy -= 512 ;

			priority = tattr >> 12 ;
			tchar = READ_WORD(&s_info[0])&0x3fff;

			sprite_size_ptr = (tattr>>6)&0x3f ;
			s_sizey = (READ_WORD(&s_size[2*sprite_size_ptr])>>4)&0xf ;
			s_sizex = (READ_WORD(&s_size[2*sprite_size_ptr]))&0xf ;
			for ( dy = s_sizey ; dy > 0 ; dy-- )
				for ( dx = s_sizex; dx > 0 ; dx-- )
					{
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


	fillbitmap(bitmap,0,&Machine->drv->visible_area);

	for ( priority = 0 ; priority < 16 ; priority++ )	/* draw priority layers in order */
		{
		tinfo = (tile_struct *)&(tile_list[priority][0]) ;
		for ( i = 0 ; i < tile_count[priority] ; i++ )	/* draw only tiles in list */
			{
			drawgfx(bitmap,Machine->gfx[tinfo->color>>7], 	/* bit 7 set for sprites */
				tinfo->tile_num,
				tinfo->color&0x7f, 			/* bit 7 not for colour */
				0,0,
				tinfo->xpos,tinfo->ypos,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			tinfo++ ;
			}
		}
}
