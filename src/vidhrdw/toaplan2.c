/***************************************************************************

  Functions to emulate the video hardware of some Toaplan games


 To Do / Unknowns
	-  Hack is needed to reset sound CPU and sound chip when machine
		is 'tilted'in Pipi & Bibis. Otherwise sound CPU interferes
		with the main CPU test of shared RAM. You get a 'Sub CPU RAM Error'
	-  What do Scroll registers 0Eh and 0Fh really do ????
	-  Snow Bros 2 sets bit 6 of the sprite X info word during weather
		world map, and bits 4, 5 and 6 of the sprite X info word during
		the Rabbit boss screen - reasons are unknown.
	-  Fourth set of scroll registers have been used for Sprite scroll
		though it may not be correct. For most parts this looks right
		except for Snow Bros 2 when in the rabbit boss screen (all sprites
		jump when big green nasty (which is the foreground layer) comes
		in from the left)
	-  Teki Paki tests video RAM from address 0 past SpriteRAM to $37ff.
		It doesnt seem to use that memory above SpriteRAM for anything though.
	-  Teki Paki sprites seem to be three frames ahead of the tile layers.
		Since this driver double buffers the sprites, Teki Paki is still
		suffering sprite lag (Does it need an extra buffer - quite strange)


 Video RAM address layout:

	Bank		  data size of video layer
	-----------------------------------------
	$0000-07FF	  800h words for background layer
	$0800-0FFF	  800h words for foreground layer
	$1000-17FF	  800h words for top (text) layer
	$1800-1BFF	  400h words for sprites (100 possible sprites)



 Tile RAM format (each tile takes up 32 bits)

  0         1         2         3
  ---- ---- ---- ---- xxxx xxxx xxxx xxxx = Tile number (0 - FFFFh)
  ---- ---- -xxx xxxx ---- ---- ---- ---- = Color (0 - 7Fh)
  ---- ---- ?--- ---- ---- ---- ---- ---- = unknown / unused
  ---- xxxx ---- ---- ---- ---- ---- ---- = Priority (0 - Fh)
  ???? ---- ---- ---- ---- ---- ---- ---- = unknown / unused / possible flips



Sprites are of varying sizes between 8x8 and 128x128 and any size
inbetween, in multiples of 8 either way.

Here we draw the first 8x8 part of the sprite, then by using the sprite
dimensions, we draw the rest of the 8x8 parts to produce the complete
sprite.

There seems to be sprite buffering - double buffering actually.

 Sprite RAM format (data for each sprite takes up 4 words)

  0
  ---- ----  ---- --xx = top 2 bits of Sprite number
  ---- ----  xxxx xx-- = Color (0 - 3Fh)
  ---- xxxx  ---- ---- = Priority (0 - Fh)
  ---x ----  ---- ---- = Flip X
  --x- ----  ---- ---- = Flip Y
  -?-- ----  ---- ---- = unknown / unused
  x--- ----  ---- ---- = Show sprite ?

  1
  xxxx xxxx  xxxx xxxx = Sprite number (top two bits in word 0)

  2
  ---- ----  ---- xxxx = Sprite X size (add 1, then multiply by 8)
  ---- ----  -??? ---- = unknown - used in Snow Bros. 2
  xxxx xxxx  x--- ---- = X position

  3
  ---- ----  ---- xxxx = Sprite Y size (add 1, then multiply by 8)
  ---- ----  -??? ---- = unknown / unused
  xxxx xxxx  x--- ---- = Y position


Scroll Registers (hex) :

	00		Background scroll X (X flip off)
	01		Background scroll Y (Y flip off)
	02		Foreground scroll X (X flip off)
	03		Foreground scroll Y (Y flip off)
	04		Top (text) scroll X (X flip off)
	05		Top (text) scroll Y (Y flip off)
	06		Sprites    scroll X (X flip off) ???
	07		Sprites    scroll Y (Y flip off) ???
	0E		???? Initialise Video controller at startup ????
	0F		Scroll update complete ??? using this to control sprite buffering

	80		Background scroll X (X flip on)
	81		Background scroll Y (Y flip on)
	82		Foreground scroll X (X flip on)
	83		Foreground scroll Y (Y flip on)
	84		Top (text) scroll X (X flip on)
	85		Top (text) scroll Y (Y flip on)
	86		Sprites    scroll X (X flip on) ???
	87		Sprites    scroll Y (Y flip on) ???
	8F		Same as 0Fh except flip bit is active


Scroll Register 0E writes (Video controller inits ?) from different games:

Teki-Paki		 | Ghox				| Knuckle Bash	   | Pipi & Bibis	  |
0003, 0002, 4000 | ????, ????, ???? | 0202, 0203, 4200 | 0003, 0002, 4000 |

Dogyuun			 |
0202, 0203, 4200 |
1202, 1203, 5200 | <--- Second video controller ???

Fix Eight		 | Tatsujin 2		| Snow Bros. 2	   |
0202, 0203, 4200 | 0003, 0002, 4000 | 0202, 0203, 4200 |

***************************************************************************/

#include "driver.h"
#include "tilemap.h"

#define TOAPLAN2_DEBUG 0


#define TOAPLAN2_BG_VRAM_SIZE	0x1000	/* Background RAM size (in bytes) */
#define TOAPLAN2_FG_VRAM_SIZE	0x1000	/* Foreground RAM size (in bytes) */
#define TOAPLAN2_TOP_VRAM_SIZE	0x1000	/* Top Layer  RAM size (in bytes) */
#define TOAPLAN2_SPRITERAM_SIZE	0x0800	/* Sprite	  RAM size (in bytes) */

#define TOAPLAN2_SPRITE_FLIPX 0x1000	/* Sprite flip flags (for screen flip) */
#define TOAPLAN2_SPRITE_FLIPY 0x2000

static unsigned char *bgvideoram;
static unsigned char *fgvideoram;
static unsigned char *topvideoram;
static unsigned char *spriteram_now;	/* Sprites to draw this frame */
static unsigned char *spriteram_next;	/* Sprites to draw next frame */
static unsigned char *spriteram_new;	/* Sprites to add to next frame */
static int toaplan2_unk_vram;			/* Video RAM tested but not used (for Teki Paki)*/

static int toaplan2_scroll_reg;
static int toaplan2_voffs;
static int bg_offs;
static int fg_offs;
static int top_offs;
static int sprite_offs;
static int bg_scrollx;
static int bg_scrolly;
static int fg_scrollx;
static int fg_scrolly;
static int top_scrollx;
static int top_scrolly;
static int sprite_scrollx;
static int sprite_scrolly;

#if TOAPLAN2_DEBUG
static int display_bg =1;
static int display_fg =1;
static int display_top=1;
static int displog    =0;
#endif
static int display_sp =1;

static int sprite_priority[16];
static int bg_flip = 0;
static int fg_flip = 0;
static int top_flip = 0;
static int sprite_flip = 0;

extern int toaplan2_sub_cpu;

#define CPU_2_NONE      0x00
#define CPU_2_Z80		0x5a
#define CPU_2_HD647180  0xa5
#define CPU_2_UNKNOWN   0xff

static struct tilemap *top_tilemap, *fg_tilemap, *bg_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_top_tile_info( int col, int row )
{
	int color, tile_number, attrib, offset;
	UINT16 *source = (UINT16 *)(topvideoram);

	offset = ((row*64) + (col*2)) & 0x7ff;

	attrib = source[offset];
	tile_number = source[offset+1];
	color = attrib & 0x7f;
	SET_TILE_INFO(0,tile_number,color)
	tile_info.priority = (attrib & 0x0f00) >> 8;
}

static void get_fg_tile_info( int col, int row )
{
	int color, tile_number, attrib, offset;
	UINT16 *source = (UINT16 *)(fgvideoram);

	offset = ((row*64) + (col*2)) & 0x7ff;

	attrib = source[offset];
	tile_number = source[offset+1];
	color = attrib & 0x7f;
	SET_TILE_INFO(0,tile_number,color)
	tile_info.priority = (attrib & 0x0f00) >> 8;
}

static void get_bg_tile_info( int col, int row )
{
	int color, tile_number, attrib, offset;
	UINT16 *source = (UINT16 *)(bgvideoram);

	offset = ((row*64) + (col*2)) & 0x7ff;

	attrib = source[offset];
	tile_number = source[offset+1];
	color = attrib & 0x7f;
	SET_TILE_INFO(0,tile_number,color)
	tile_info.priority = (attrib & 0x0f00) >> 8;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
void toaplan2_vh_stop(void) {
	free(bgvideoram);
	free(fgvideoram);
	free(topvideoram);
	free(spriteram_now);
	free(spriteram_next);
	free(spriteram_new);
}

int toaplan2_vh_start(void) {
	if ((spriteram_new = malloc(TOAPLAN2_SPRITERAM_SIZE)) == 0)
	{
		return 1;
	}
	memset(spriteram_new,0,TOAPLAN2_SPRITERAM_SIZE);

	if ((spriteram_next = malloc(TOAPLAN2_SPRITERAM_SIZE)) == 0)
	{
		free(spriteram_new);
		return 1;
	}
	memset(spriteram_next,0,TOAPLAN2_SPRITERAM_SIZE);

	if ((spriteram_now = malloc(TOAPLAN2_SPRITERAM_SIZE)) == 0)
	{
		free(spriteram_next);
		free(spriteram_new);
		return 1;
	}
	memset(spriteram_now,0,TOAPLAN2_SPRITERAM_SIZE);

	if ((topvideoram = malloc(TOAPLAN2_TOP_VRAM_SIZE)) == 0)
	{
		free(spriteram_now);
		free(spriteram_next);
		free(spriteram_new);
		return 1;
	}
	memset(topvideoram,0,TOAPLAN2_TOP_VRAM_SIZE);

	if ((fgvideoram = malloc(TOAPLAN2_FG_VRAM_SIZE)) == 0)
	{
		free(topvideoram);
		free(spriteram_now);
		free(spriteram_next);
		free(spriteram_new);
		return 1;
	}
	memset(fgvideoram,0,TOAPLAN2_FG_VRAM_SIZE);

	if ((bgvideoram = malloc(TOAPLAN2_BG_VRAM_SIZE)) == 0)
	{
		free(fgvideoram);
		free(topvideoram);
		free(spriteram_now);
		free(spriteram_next);
		free(spriteram_new);
		return 1;
	}
	memset(bgvideoram,0,TOAPLAN2_BG_VRAM_SIZE);

	top_tilemap = tilemap_create(
		get_top_tile_info,
		TILEMAP_TRANSPARENT,
		16,16,
		32,32
	);

	fg_tilemap = tilemap_create(
		get_fg_tile_info,
		TILEMAP_TRANSPARENT,
		16,16,
		32,32
	);

	bg_tilemap = tilemap_create(
		get_bg_tile_info,
		TILEMAP_TRANSPARENT,
		16,16,
		32,32
	);


	if (top_tilemap && fg_tilemap && bg_tilemap)
	{
		top_tilemap->transparent_pen = 0;
		fg_tilemap->transparent_pen = 0;
		bg_tilemap->transparent_pen = 0;

		return 0;
	}

	return 1;
}



/***************************************************************************

  Video I/O port hardware.

***************************************************************************/


int toaplan2_voffs_r(int offset)
{
	return toaplan2_voffs ;
}

void toaplan2_voffs_w(int offset, int data)
{
	toaplan2_voffs = data;

	/* Layers are seperated by ranges in the offset */
	switch (data & 0xfc00)
	{
		case 0x0400:
		case 0x0000:	bg_offs = (data & 0x7ff) * 2; break;
		case 0x0c00:
		case 0x0800:	fg_offs = (data & 0x7ff) * 2; break;
		case 0x1400:
		case 0x1000:	top_offs = (data & 0x7ff) * 2; break;
		case 0x1800:	sprite_offs = (data & 0x3ff) * 2; break;
		default:		if (errorlog)
							fprintf(errorlog,"Hmmm, unknown video layer being selected (%08x)\n",data);
						break;
	}
}

int toaplan2_videoram_r(int offset)
{
	static int video_data = 0;
	int videoram_offset;

	switch (toaplan2_voffs & 0xfc00)
	{
		case 0x0400:
		case 0x0000:
				videoram_offset = bg_offs & (TOAPLAN2_BG_VRAM_SIZE-1);
				video_data = READ_WORD (&bgvideoram[videoram_offset]);
				bg_offs+=2;
				if (bg_offs > TOAPLAN2_BG_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Reading %04x from out of range BG Layer address (%08x) !!!\n",video_data,bg_offs);
				}
				break;
		case 0x0c00:
		case 0x0800:
				videoram_offset = fg_offs & (TOAPLAN2_FG_VRAM_SIZE-1);
				video_data = READ_WORD (&fgvideoram[videoram_offset]);
				fg_offs+=2;
				if (fg_offs > TOAPLAN2_FG_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Reading %04x from out of range FG Layer address (%08x) !!!\n",video_data,fg_offs);
				}
				break;
		case 0x1400:
		case 0x1000:
				videoram_offset = top_offs & (TOAPLAN2_TOP_VRAM_SIZE-1);
				video_data = READ_WORD (&topvideoram[videoram_offset]);
				top_offs+=2;
				if (top_offs > TOAPLAN2_TOP_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Reading %04x from out of range TOP Layer address (%08x) !!!\n",video_data,top_offs);
				}
				break;
		case 0x1800:
				videoram_offset = sprite_offs & (TOAPLAN2_SPRITERAM_SIZE-1);
				video_data = READ_WORD (&spriteram_new[videoram_offset]);
				sprite_offs+=2;
				if (sprite_offs > TOAPLAN2_SPRITERAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Reading %04x from out of range Sprite address (%08x) !!!\n",video_data,sprite_offs);
				}
				break;
		default:
				video_data = toaplan2_unk_vram;
				if (errorlog)
					fprintf(errorlog,"Hmmm, reading %04x from unknown video layer (%08x)\n",video_data,toaplan2_voffs);
				break;
	}
	return video_data;
}

void toaplan2_videoram_w(int offset, int data)
{
	int oldword = 0;
	int videoram_offset;
	int dirty_cell;

	switch (toaplan2_voffs & 0xfc00)
	{
		case 0x0400:
		case 0x0000:
				videoram_offset = bg_offs & (TOAPLAN2_BG_VRAM_SIZE-1);
				oldword = READ_WORD (&bgvideoram[videoram_offset]);
				if (data != oldword)
				{
					WRITE_WORD (&bgvideoram[videoram_offset],data);
					dirty_cell = (bg_offs & (TOAPLAN2_BG_VRAM_SIZE-3))/2;
					tilemap_mark_tile_dirty(bg_tilemap, (dirty_cell%64)/2, dirty_cell/64);
				}
				bg_offs+=2;
				if (bg_offs > TOAPLAN2_BG_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Writing %04x to out of range BG Layer address (%08x) !!!\n",data,bg_offs);
				}
				break;
		case 0x0c00:
		case 0x0800:
				videoram_offset = fg_offs & (TOAPLAN2_FG_VRAM_SIZE-1);
				oldword = READ_WORD (&fgvideoram[videoram_offset]);
				if (data != oldword)
				{
					WRITE_WORD (&fgvideoram[videoram_offset],data);
					dirty_cell = (fg_offs & (TOAPLAN2_FG_VRAM_SIZE-3))/2;
					tilemap_mark_tile_dirty(fg_tilemap, (dirty_cell%64)/2, dirty_cell/64);
				}
				fg_offs+=2;
				if (fg_offs > TOAPLAN2_FG_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Writing %04x to out of range FG Layer address (%08x) !!!\n",data,fg_offs);
				}
				break;
		case 0x1400:
		case 0x1000:
				videoram_offset = top_offs & (TOAPLAN2_TOP_VRAM_SIZE-1);
				oldword = READ_WORD (&topvideoram[videoram_offset]);
				if (data != oldword)
				{
					WRITE_WORD (&topvideoram[videoram_offset],data);
					dirty_cell = (top_offs & (TOAPLAN2_TOP_VRAM_SIZE-3))/2;
					tilemap_mark_tile_dirty(top_tilemap, (dirty_cell%64)/2, dirty_cell/64);
				}
				top_offs+=2;
				if (top_offs > TOAPLAN2_TOP_VRAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Writing %04x to out of range TOP Layer address (%08x) !!!\n",data,top_offs);
				}
				break;
		case 0x1800:
				videoram_offset = sprite_offs & (TOAPLAN2_SPRITERAM_SIZE-1);
				WRITE_WORD (&spriteram_new[videoram_offset],data);
				sprite_offs+=2;
				if (sprite_offs > TOAPLAN2_SPRITERAM_SIZE)
				{
					if (errorlog)
						fprintf(errorlog,"Writing %04x to out of range Sprite address (%08x) !!!\n",data,sprite_offs);
				}
				break;
		default:
				toaplan2_unk_vram = data;
				if (errorlog)
					fprintf(errorlog,"Hmmm, writing %04x to unknown video layer (%08x)\n",toaplan2_unk_vram,toaplan2_voffs);
				break;
	}
}



void toaplan2_scroll_reg_select_w(int offset, int data)
{
	toaplan2_scroll_reg = data;
	if (toaplan2_scroll_reg & 0xffffff70)
	{
		if (errorlog) fprintf(errorlog,"Hmmm, unknown video control register selected (%08x)\n",toaplan2_scroll_reg);
	}
}

void toaplan2_scroll_reg_data_w(int offset, int data)
{
	/************************************************************************/
	/***** X and Y layer flips can be set independantly, so emulate it ******/
	/************************************************************************/

	switch(toaplan2_scroll_reg)
	{
		case 0x00:	bg_scrollx = data - 0x1d6;			/* 1D6h */
					bg_flip &= (~TILEMAP_FLIPX);
					tilemap_set_flip(bg_tilemap,bg_flip);
					tilemap_set_scrollx(bg_tilemap,0,bg_scrollx);
					break;
		case 0x01:	bg_scrolly = data - 0x1ef;			/* 1EFh */
					bg_flip &= (~TILEMAP_FLIPY);
					tilemap_set_flip(bg_tilemap,bg_flip);
					tilemap_set_scrolly(bg_tilemap,0,bg_scrolly);
					break;
		case 0x02:	fg_scrollx = data - 0x1d8;			/* 1D0h */
					fg_flip &= (~TILEMAP_FLIPX);
					tilemap_set_flip(fg_tilemap,fg_flip);
					tilemap_set_scrollx(fg_tilemap,0,fg_scrollx);
					break;
		case 0x03:  fg_scrolly = data - 0x1ef;			/* 1EFh */
					fg_flip &= (~TILEMAP_FLIPY);
					tilemap_set_flip(fg_tilemap,fg_flip);
					tilemap_set_scrolly(fg_tilemap,0,fg_scrolly);
					break;
		case 0x04:	top_scrollx = data - 0x1da;			/* 1DAh */
					top_flip &= (~TILEMAP_FLIPX);
					tilemap_set_flip(top_tilemap,top_flip);
					tilemap_set_scrollx(top_tilemap,0,top_scrollx);
					break;
		case 0x05:	top_scrolly = data - 0x1ef;			/* 1EFh */
					top_flip &= (~TILEMAP_FLIPY);
					tilemap_set_flip(top_tilemap,top_flip);
					tilemap_set_scrolly(top_tilemap,0,top_scrolly);
					break;
		case 0x06:	sprite_scrollx = data - 0x1cc;		/* 1D4h */
					if (sprite_scrollx & 0x80000000) sprite_scrollx |= 0xfffffe00;
					else sprite_scrollx &= 0x1ff;
					sprite_flip &= (~TOAPLAN2_SPRITE_FLIPX);
					break;
		case 0x07:	sprite_scrolly = data - 0x1ef;		/* 1F7h */
					if (sprite_scrolly & 0x80000000) sprite_scrolly |= 0xfffffe00;
					else sprite_scrolly &= 0x1ff;
					sprite_flip &= (~TOAPLAN2_SPRITE_FLIPY);
					break;
		case 0x80:  bg_scrollx = data - 0x229;			/* 169h */
					bg_flip |= TILEMAP_FLIPX;
					tilemap_set_flip(bg_tilemap,bg_flip);
					tilemap_set_scrollx(bg_tilemap,0,bg_scrollx);
					break;
		case 0x81:	bg_scrolly = data - 0x210;			/* 100h */
					bg_flip |= TILEMAP_FLIPY;
					tilemap_set_flip(bg_tilemap,bg_flip);
					tilemap_set_scrolly(bg_tilemap,0,bg_scrolly);
					break;
		case 0x82:	fg_scrollx = data - 0x227;			/* 15Fh */
					fg_flip |= TILEMAP_FLIPX;
					tilemap_set_flip(fg_tilemap,fg_flip);
					tilemap_set_scrollx(fg_tilemap,0,fg_scrollx);
					break;
		case 0x83:	fg_scrolly = data - 0x210;			/* 100h */
					fg_flip |= TILEMAP_FLIPY;
					tilemap_set_flip(fg_tilemap,fg_flip);
					tilemap_set_scrolly(fg_tilemap,0,fg_scrolly);
					break;
		case 0x84:	top_scrollx = data - 0x225;			/* 165h */
					top_flip |= TILEMAP_FLIPX;
					tilemap_set_flip(top_tilemap,top_flip);
					tilemap_set_scrollx(top_tilemap,0,top_scrollx);
					break;
		case 0x85:	top_scrolly = data - 0x210;			/* 100h */
					top_flip |= TILEMAP_FLIPY;
					tilemap_set_flip(top_tilemap,top_flip);
					tilemap_set_scrolly(top_tilemap,0,top_scrolly);
					break;
		case 0x86:	sprite_scrollx = data - 0x17b;		/* 17Bh */
					if (sprite_scrollx & 0x80000000) sprite_scrollx |= 0xfffffe00;
					else sprite_scrollx &= 0x1ff;
					sprite_flip |= TOAPLAN2_SPRITE_FLIPX;
					break;
		case 0x87:	sprite_scrolly = data - 0x108;		/* 108h */
					if (sprite_scrolly & 0x80000000) sprite_scrolly |= 0xfffffe00;
					else sprite_scrolly &= 0x1ff;
					sprite_flip |= TOAPLAN2_SPRITE_FLIPY;
					break;

		case 0x0f:	/* shift sprite buffers */
		case 0x8f:	memcpy(spriteram_now,spriteram_next,TOAPLAN2_SPRITERAM_SIZE);
					memcpy(spriteram_next,spriteram_new,TOAPLAN2_SPRITERAM_SIZE);
					break;

		case 0x0e:	/******* Initialise video controller register ? *******/
					if ((toaplan2_sub_cpu == CPU_2_Z80) && (data == 3))
					{
						/* HACK! When tilted, sound CPU needs to be reset. */
						cpu_set_reset_line(1,PULSE_LINE);
						YM3812_sh_reset();
					}
		default:	if (errorlog)
						fprintf(errorlog,"Hmmm, writing %08x to unknown video control register (%08x)\n",data ,toaplan2_scroll_reg);
					break;
	}

#if TOAPLAN2_DEBUG
	if ( keyboard_pressed(KEYCODE_L) )
	{
		while (keyboard_pressed(KEYCODE_L)) ;
		display_sp += 1;
		display_sp &= 1;
	}
	if ( keyboard_pressed(KEYCODE_K) )
	{
		while (keyboard_pressed(KEYCODE_K)) ;
		display_top += 1;
		display_top &= 1;
		tilemap_set_enable(top_tilemap, display_top);
	}
	if ( keyboard_pressed(KEYCODE_J) )
	{
		while (keyboard_pressed(KEYCODE_J)) ;
		display_fg += 1;
		display_fg &= 1;
		tilemap_set_enable(fg_tilemap, display_fg);
	}
	if ( keyboard_pressed(KEYCODE_H) )
	{
		while (keyboard_pressed(KEYCODE_H)) ;
		display_bg += 1;
		display_bg &= 1;
		tilemap_set_enable(bg_tilemap, display_bg);
	}
#endif
}


#if TOAPLAN2_DEBUG
void toaplan2_log_vram(void)
{
	int sprite_voffs, tile_voffs;

	if ( keyboard_pressed(KEYCODE_M) )
	{
		UINT16 *source = (UINT16 *)(spriteram_now);
		UINT16 *source1 = (UINT16 *)(spriteram_next);
		UINT16 *source2 = (UINT16 *)(spriteram_new);

		while (keyboard_pressed(KEYCODE_M)) ;
		if (errorlog) fprintf (errorlog, "Scrolls   BG-X  BG-Y   FG-X  FG-Y   TOP-X  TOP-Y   Sprite-X  Sprite-Y\n");
		if (errorlog) fprintf (errorlog, "------>   %04x  %04x   %04x  %04x    %04x  %04x       %04x    %04x\n", bg_scrollx,bg_scrolly,fg_scrollx,fg_scrolly,top_scrollx,top_scrolly,sprite_scrollx, sprite_scrolly);
		for ( sprite_voffs = 0; sprite_voffs < (TOAPLAN2_SPRITERAM_SIZE/2); sprite_voffs+=4 )
		{
			int tchar,tattr,txpos,typos;
			tattr = source[sprite_voffs];
			tchar = source[sprite_voffs + 1];
			txpos = source[sprite_voffs + 2];
			typos = source[sprite_voffs + 3];
			if (errorlog) fprintf (errorlog, "Current SPoffs:%04x Attr=%04x Sprite=%04x Xpos=%04x Ypos=%04x\n", sprite_voffs, tattr, tchar, txpos, typos);
			tattr = source1[sprite_voffs];
			tchar = source1[sprite_voffs + 1];
			txpos = source1[sprite_voffs + 2];
			typos = source1[sprite_voffs + 3];
			if (errorlog) fprintf (errorlog, "Next    SPoffs:%04x Attr=%04x Sprite=%04x Xpos=%04x Ypos=%04x\n", sprite_voffs, tattr, tchar, txpos, typos);
			tattr = source2[sprite_voffs];
			tchar = source2[sprite_voffs + 1];
			txpos = source2[sprite_voffs + 2];
			typos = source2[sprite_voffs + 3];
			if (errorlog) fprintf (errorlog, "New     SPoffs:%04x Attr=%04x Sprite=%04x Xpos=%04x Ypos=%04x\n", sprite_voffs, tattr, tchar, txpos, typos);
		}
	}
	if ( keyboard_pressed(KEYCODE_N) )
	{
		while (keyboard_pressed(KEYCODE_N)) ;
		if (errorlog) fprintf (errorlog, "Scrolls   BG-X  BG-Y   FG-X  FG-Y   TOP-X  TOP-Y   Sprite-X  Sprite-Y\n");
		if (errorlog) fprintf (errorlog, "------>   %04x  %04x   %04x  %04x    %04x  %04x       %04x    %04x\n", bg_scrollx,bg_scrolly,fg_scrollx,fg_scrolly,top_scrollx,top_scrolly,sprite_scrollx, sprite_scrolly);
		for ( tile_voffs = 0; tile_voffs < TOAPLAN2_TOP_VRAM_SIZE; tile_voffs+=4 )
		{
			int tattr,tchar;
			tchar = READ_WORD (&topvideoram[tile_voffs + 2]);
			tattr = READ_WORD (&topvideoram[tile_voffs]);
			if (errorlog) fprintf (errorlog, "TXoffs:%08x Tile:%04x Attr:%04x\n", tile_voffs/2, tchar, tattr);
		}
	}
	if ( keyboard_pressed(KEYCODE_B) )
	{
		while (keyboard_pressed(KEYCODE_B)) ;
		if (errorlog) fprintf (errorlog, "Scrolls   BG-X  BG-Y   FG-X  FG-Y   TOP-X  TOP-Y   Sprite-X  Sprite-Y\n");
		if (errorlog) fprintf (errorlog, "------>   %04x  %04x   %04x  %04x    %04x  %04x       %04x    %04x\n", bg_scrollx,bg_scrolly,fg_scrollx,fg_scrolly,top_scrollx,top_scrolly,sprite_scrollx, sprite_scrolly);
		for ( tile_voffs = 0; tile_voffs < TOAPLAN2_FG_VRAM_SIZE; tile_voffs+=4 )
		{
			int tattr,tchar;
			tchar = READ_WORD (&fgvideoram[tile_voffs + 2]);
			tattr = READ_WORD (&fgvideoram[tile_voffs]);
			if (errorlog) fprintf (errorlog, "FGoffs:%08x tile:%04x Attr:%04x\n", tile_voffs/2, tchar, tattr);
		}
	}
	if ( keyboard_pressed(KEYCODE_V) )
	{
		while (keyboard_pressed(KEYCODE_V)) ;
		if (errorlog) fprintf (errorlog, "Scrolls   BG-X  BG-Y   FG-X  FG-Y   TOP-X  TOP-Y   Sprite-X  Sprite-Y\n");
		if (errorlog) fprintf (errorlog, "------>   %04x  %04x   %04x  %04x    %04x  %04x       %04x    %04x\n", bg_scrollx,bg_scrolly,fg_scrollx,fg_scrolly,top_scrollx,top_scrolly,sprite_scrollx, sprite_scrolly);
		for ( tile_voffs = 0; tile_voffs < TOAPLAN2_BG_VRAM_SIZE; tile_voffs+=4 )
		{
			int tattr,tchar;
			tchar = READ_WORD (&bgvideoram[tile_voffs + 2]);
			tattr = READ_WORD (&bgvideoram[tile_voffs]);
			if (errorlog) fprintf (errorlog, "BGoffs:%08x Tile:%04x Attr:%04x\n", tile_voffs/2, tchar, tattr);
		}
	}

	if ( keyboard_pressed(KEYCODE_C) )
	{
		while (keyboard_pressed(KEYCODE_C)) ;
		if (errorlog) fprintf (errorlog, "Mark here\n");
	}
	if ( keyboard_pressed(KEYCODE_G) )
	{
		while (keyboard_pressed(KEYCODE_G)) ;
		displog += 1;
		displog &= 1;
	}
	if (displog)
	{
		if (errorlog) fprintf (errorlog, "Scrolls   BG-X  BG-Y   FG-X  FG-Y   TOP-X  TOP-Y   Sprite-X  Sprite-Y\n");
		if (errorlog) fprintf (errorlog, "------>   %04x  %04x   %04x  %04x    %04x  %04x       %04x    %04x\n", bg_scrollx,bg_scrolly,fg_scrollx,fg_scrolly,top_scrollx,top_scrolly,sprite_scrollx, sprite_scrolly);
	}
}
#endif

/***************************************************************************
	Sprite Handlers
***************************************************************************/

static void mark_sprite_colors(void)
{
	int offs, attrib, sprite, color, i, pal_base;
	int sprite_sizex, sprite_sizey, temp_x, temp_y;
	int colmask[64];

	UINT16 *source = (UINT16 *)(spriteram_now);

	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;

	for(i=0; i < 64; i++) colmask[i] = 0;

	for (offs = 0; offs < (TOAPLAN2_SPRITERAM_SIZE/2); offs += 4)
	{
		attrib = source[offs];
		sprite = source[offs + 1] | ((attrib & 3) << 16);
		if (attrib & 0x8000)
		{
			/* While we're here, mark all priorities used */
			sprite_priority[((attrib & 0x0f00) >> 8)] = display_sp;

			color = (attrib >> 2) & 0x3f;
			sprite_sizex = (source[offs + 2] & 0x0f) + 1;
			sprite_sizey = (source[offs + 3] & 0x0f) + 1;

			for (temp_y = 0; temp_y < sprite_sizey; temp_y++)
			{
				for (temp_x = 0; temp_x < sprite_sizex; temp_x++)
				{
					colmask[color] |= Machine->gfx[1]->pen_usage[sprite];
					sprite++ ;
				}
			}
		}
	}

	for (color = 0;color < 64;color++)
	{
		if ((color == 0) && (colmask[0] & 1))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1; i < 16; i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}
}



static void draw_sprites( struct osd_bitmap *bitmap, int priority_to_display )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	const struct rectangle *clip = &Machine->drv->visible_area;

	UINT16 *source = (UINT16 *)(spriteram_now);

	int offs;
	for (offs = 0; offs < (TOAPLAN2_SPRITERAM_SIZE/2); offs += 4)
	{
		int attrib, sprite, color, priority, flipx, flipy, sx, sy;
		int sprite_sizex, sprite_sizey, temp_x, temp_y, sx_base, sy_base;

		attrib = source[offs];
		priority = (attrib & 0x0f00) >> 8;

		if ((priority == priority_to_display) && (attrib & 0x8000))
		{
			sprite = ((attrib & 3) << 16) | source[offs + 1] ;	/* 18 bit */
			color = (attrib >> 2) & 0x3f;

			/****** find out sprite size ******/
			sprite_sizex = ((source[offs + 2] & 0x0f) + 1) * 8;
			sprite_sizey = ((source[offs + 3] & 0x0f) + 1) * 8;

			/****** find position to display sprite ******/
			sx_base = (source[offs + 2] >> 7) - sprite_scrollx;
			sy_base = (source[offs + 3] >> 7) - sprite_scrolly;

			flipx = attrib & TOAPLAN2_SPRITE_FLIPX;
			flipy = attrib & TOAPLAN2_SPRITE_FLIPY;

			if (flipx)
			{
				sx_base -= 7;
				/****** wrap around sprite position ******/
				if (sx_base >= 0x1c0) sx_base -= 0x200;
			}
			else
			{
				if (sx_base >= 0x180) sx_base -= 0x200;
			}

			if (flipy)
			{
				sy_base -= 7;
				if (sy_base >= 0x1c0) sy_base -= 0x200;
			}
			else
			{
				if (sy_base >= 0x180) sy_base -= 0x200;
			}

			/****** flip the sprite layer in any active X or Y flip ******/
			if (sprite_flip)
			{
				if (sprite_flip & TOAPLAN2_SPRITE_FLIPX)
					sx_base = 320 - sx_base;
				if (sprite_flip & TOAPLAN2_SPRITE_FLIPY)
					sy_base = 240 - sy_base;
			}

			/****** cancel flip, if it and sprite layer flip are active ******/
			flipx = (flipx ^ (sprite_flip & TOAPLAN2_SPRITE_FLIPX));
			flipy = (flipy ^ (sprite_flip & TOAPLAN2_SPRITE_FLIPY));

			for (temp_y = 0; temp_y < sprite_sizey; temp_y += 8)
			{
				if (flipy) sy = sy_base - temp_y;
				else       sy = sy_base + temp_y;
				for (temp_x = 0; temp_x < sprite_sizex; temp_x += 8)
				{
					if (flipx) sx = sx_base - temp_x;
					else       sx = sx_base + temp_x;

					drawgfx(bitmap,gfx,sprite,
						color,
						flipx,flipy,
						sx,sy,
						clip,TRANSPARENCY_PEN,0);

					sprite++ ;
				}
			}
		}
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


void toaplan2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int priority;

	for (priority = 0; priority < 16; priority++)
		sprite_priority[priority] = 0;		/* Clear priorities used list */

#if TOAPLAN2_DEBUG
	if (errorlog) toaplan2_log_vram();
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprite_colors();	/* Also mark priorities used */

	if (palette_recalc()) tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

	for (priority = 0; priority < 16; priority++)
	{
		tilemap_draw(bitmap,bg_tilemap,priority);
		tilemap_draw(bitmap,fg_tilemap,priority);
		tilemap_draw(bitmap,top_tilemap,priority);
		if (sprite_priority[priority])
			draw_sprites(bitmap,priority);
	}
}

