/*************************************************************************

  The  System16  machine hardware.   By Mirko Buffoni.

  This files describes the hardware behaviour of a System16 machine.
  It also includes common methods for handling of video hardware.

  Many thanks to Thierry Lescot (Thierry.Lescot@skynet.be),
  Phil Stroffolino (phil@maya.com), and Li Jih Hwa <nao@ms6.hinet.net>
  for precious information about S16 hardware.

*************************************************************************/

#include "vidhrdw/generic.h"
#include "machine/system16.h"

int s16_videoram_size;
int s16_soundram_size;
int s16_spriteram_size;
int s16_backgroundram_size;
int system16_sprxoffset;
unsigned char *system16_videoram;
unsigned char *system16_soundram;
unsigned char *system16_spriteram;
unsigned char *system16_scrollram;
unsigned char *system16_pagesram;
unsigned char *system16_refreshregister;
unsigned char *system16_backgroundram;
unsigned char system16_background_bank;

unsigned char *goldnaxe_mirror1;
unsigned char *goldnaxe_mirror2;

int system16_refreshenable;
int  scrollY[2], scrollX[2];
unsigned char fg_pages[4], bg_pages[4];
struct sys16_sprite_info *sys16_sprite;
int *s16_obj_bank = NULL;

#define LoggaR(s,o)   if (errorlog) fprintf(errorlog, "%s_R:  %08xh\n", s, o);
#define LoggaW(s,o,d) if (errorlog) fprintf(errorlog, "%s_W:  %08xh = %02xh\n", s, o, d);

int  system16_init_machine(void){
	int size = sizeof(struct sys16_sprite_info)*256;
	sys16_sprite = (struct sys16_sprite_info *)malloc(size);
	if( sys16_sprite ){
		memset((char *)sys16_sprite,0,size);

		scrollX[0] = scrollX[1] = scrollY[0] = scrollY[1] = 0;

		fg_pages[0] = fg_pages[1] = fg_pages[2] = fg_pages[3] = 16;
		bg_pages[0] = bg_pages[1] = bg_pages[2] = bg_pages[3] = 16;

		system16_sprxoffset = 0xb8;
		system16_refreshenable = 1;

		return 0;
	}
	return 1;
}

void system16_done(void){
	free( (char *)sys16_sprite );
}

/* Common Handlers */

int  system16_videoram_r(int offset){  return READ_WORD (&system16_videoram[offset]); }
void system16_videoram_w(int offset, int data){ COMBINE_WORD_MEM (&system16_videoram[offset], data); }

int  system16_soundram_r(int offset){  return READ_WORD (&system16_soundram[offset]); }
void system16_soundram_w(int offset, int data){ COMBINE_WORD_MEM (&system16_soundram[offset], data); }

int  system16_spriteram_r(int offset){ return READ_WORD (&system16_spriteram[offset]); }
/*
 *   Sprite control bytes:
 *          0 = Sprite End Line    \  [(0)-(1)] = Sprite Height
 *          1 = Sprite Begin Line  /
 *          2 = Sprite Horizontal position MSB
 *          3 = Sprite Horizontal position LSB
 *          4 = 0000000H -> Horizontal Flip:  1 = flipped
 *          5 = VWWWWWWW -> Sprite Width:     multiply this value * 4 (Odd/Even and 2 pixels/byte)
 *              \---------> Vertical Flip:    1 = flipped
 *          6 = Sprite position in table MSB
 *          7 = Sprite position in table LSB
 *          8 = xxxxBBBB -> Sprite Bank Selector (bits 3-0)
 *          9 = PPSSSSSS -> Palette Selector
 *              \\--------> Priority:  01 = Sprite under foreground
 *                                     10 = Sprite over foreground
 *                                     11 = Sprites over Tiles (???)
 *          A = Zoom factor MSB
 *          B = Zoom factor LSB
 */
void system16_spriteram_w( int offset, int data ){
	struct sys16_sprite_info *sprite = &sys16_sprite[offset>>4];

	COMBINE_WORD_MEM(&system16_spriteram[offset], data);
	data = READ_WORD(&system16_spriteram[offset]);

	switch( offset&0xF ){
		case 0x0:
		sprite->end_line = data>>8;
		sprite->begin_line = data&0xff;
		break;

		case 0x2:
		sprite->horizontal_position = data;
		break;

		case 0x4:
		sprite->horizontal_flip = data&0x100;
		sprite->vertical_flip = data&0x80;
		sprite->width = data&0x7f;
		break;

		case 0x6:
		sprite->number = data;
		break;

		case 0x8:
		sprite->bank = (data>>8)&0xf;
		sprite->priority = (data>>6)&0x3;
		sprite->color = (data&0x3f)<<4;
		break;

		case 0xA:
		sprite->zoom = data&0x3ff;
		break;
	}
}


void system16_paletteram_w(int offset, int data)
{
	int r,g,b;
	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&paletteram[offset], newword);

	r = (newword >> 0) & 0x0f;
	g = (newword >> 4) & 0x0f;
	b = (newword >> 8) & 0x0f;

	/* I'm not sure about the following bits */
	r = (r << 1) | ((newword >> 12) & 1);
	g = (g << 1) | ((newword >> 13) & 1);
	b = (b << 1) | ((newword >> 14) & 1);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset / 2,r,g,b);
}

int  system16_backgroundram_r(int offset){ return READ_WORD (&system16_backgroundram[offset]); }
void system16_backgroundram_w(int offset, int data){ COMBINE_WORD_MEM (&system16_backgroundram[offset], data); }

int  system16_refreshenable_r(int offset) { return system16_refreshenable; }
void system16_refreshenable_w(int offset, int data)
{
	system16_refreshenable = data & 0x20;
}

/*   Sprites are normal bitmapped pictures, with a bit-depth of 4 bits.
 *   A byte stream like "01 23 45" will be a 6 pixel pic, with color0,
 *   color1, color2, color3, color4, color5 pixels respectively.
 *
 *   There are sprbank-select, pattern-offset, display-position,
 *   palette-select and a general attribute(reversed,...) in a sprite
 *   entry.  Sprite area is 4096 bytes long.  It can hold max 64 sprites
 *   and each sprite is described by 16 control bytes.
 *
 */

/*
 *   The palette area is 4096 bytes long.  2 bytes are used for each color
 *   entry (65536 possible colors), and there are 1024 colors for text and
 *   backgrounds, and 1024 colors for sprites.
 *
 *   Color weight of a 2 bytes color entry is the following:
 *
 *                   byte 0    byte 1
 *                GBGR BBBB GGGG RRRR
 *                5444 3210 3210 3210
 *
 *   The screen resolution is usually 320x224.
 */


/*   The VID buffer is an area of 4096 bytes * 16 pages.  Each page is
 *   composed of 64x32 2-bytes tiles.
 *
 *   REGPS (Page Select register) is a word that select a set of 4 screens
 *   out of 16 possible.  For instance, if the value (BIG Endian) is 0123h
 *   a scrolling background like the following will be selected:
 *
 *                          Page0  Page1
 *                          Page2  Page3
 *
 *   REGHS and REGVS (Horizontal and Vertical scrolling registers) are used
 *   to select the offsets of the background to be displayed.  The offsets
 *   are zero referenced at top-left point in scrolling background and
 *   represents the dots to be skipped.
 */

void system16_scroll_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_scrollram[offset], data);
	data = READ_WORD(&system16_scrollram[offset]);

	switch (offset){
		case 0x0: scrollY[FG_OFFS] = data; break;
		case 0x2: scrollY[BG_OFFS] = data; break;
		case 0x8: scrollX[FG_OFFS] = data; break;
		case 0xa: scrollX[BG_OFFS] = data; break;
	}
}


void system16_pagesel_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_pagesram[offset], data);
	data = READ_WORD(&system16_pagesram[offset]);

	switch (offset){
		case 0x0:
		case 0x1:
			fg_pages[0] = data>>12;
			fg_pages[1] = (data>>8) & 0xf;
			fg_pages[2] = (data>>4) & 0xf;
			fg_pages[3] = data & 0xf;
			break;

		case 0x2:
		case 0x3:
			bg_pages[0] = data>>12;
			bg_pages[1] = (data>>8) & 0xf;
			bg_pages[2] = (data>>4) & 0xf;
			bg_pages[3] = data & 0xf;
			break;
	}
}

int  system16_vh_start(void) { return system16_init_machine(); }
void system16_vh_stop(void)  { system16_done(); }

void system16_define_bank_vector(int * bank) { s16_obj_bank = bank; }
void system16_define_sprxoffset(int sprxoffset) { system16_sprxoffset = sprxoffset; }

int system16_interrupt(void)
{
	return 4;       /* Interrupt vector 4, used by VBlank */
}


int shinobi_control_r (int offset){
	switch (offset){
		case 0: return input_port_2_r (offset);    /* GEN */
		case 2: return input_port_0_r (offset);    /* PL1 */
		case 6: return input_port_1_r (offset);    /* PL2 */
	}
	return 0x00;
}

int passshot_control_r (int offset){
	switch (offset){
		case 0: return input_port_2_r (offset);    /* GEN */
		case 2: return input_port_0_r (offset);    /* PL1 */
		case 4: return input_port_1_r (offset);    /* PL2 */
	}
	return 0x00;
}

int shinobi_dsw_r (int offset){
	switch (offset){
		case 0: return input_port_4_r (offset);    /* DS2 */
		case 2: return input_port_3_r (offset);    /* DS1 */
	}
	return 0x00;
}


void shinobi_scroll_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_videoram[0x0e90+offset], data);
	data = READ_WORD(&system16_videoram[0x0e90+offset]);

	switch (offset){
		case 0x0: scrollY[FG_OFFS] = data; break;
		case 0x2: scrollY[BG_OFFS] = data; break;
		case 0x8: scrollX[FG_OFFS] = data; break;
		case 0xa: scrollX[BG_OFFS] = data; break;
	}
}


void shinobi_pagesel_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_videoram[0x0e80+offset], data);
	data = READ_WORD(&system16_videoram[0x0e80+offset]);

	switch (offset){
		case 0x0:
		case 0x1:
			fg_pages[0] = data>>12;
			fg_pages[1] = (data>>8) & 0xf;
			fg_pages[2] = (data>>4) & 0xf;
			fg_pages[3] = data & 0xf;
			break;

		case 0x2:
		case 0x3:
			bg_pages[0] = data>>12;
			bg_pages[1] = (data>>8) & 0xf;
			bg_pages[2] = (data>>4) & 0xf;
			bg_pages[3] = data & 0xf;
			break;
	}
}

void shinobi_refreshenable_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_refreshregister[offset], data);
	data = READ_WORD(&system16_refreshregister[offset]);
	system16_refreshenable = (data >> 8) & 0x20;
}


int goldnaxe_mirror1_r (int offset)
{
	return (input_port_0_r(offset) << 8) + input_port_1_r(offset);
}



int goldnaxe_mirror2_r (int offset)
{
	int rv = READ_WORD(&goldnaxe_mirror2[offset]);;
	switch (offset)
	{
		case 0:
		case 1:
			system16_background_bank = (rv-1) & 0x0f;
			return rv;
			break;

		case 2:
		case 3:
			return (input_port_2_r(offset) << 8) + (rv & 0xff);
			break;
	}
	return 0;
}

void goldnaxe_refreshenable_w(int offset, int data)
{
	if (offset == 0)
		system16_refreshenable = (data & 0x20);
}


void tetris_fgpagesel_w(int offset, int data)
{
	switch (offset){
		case 0x0:
		case 0x1:
			fg_pages[0] = data>>12;
			fg_pages[1] = (data>>8) & 0xf;
			fg_pages[2] = (data>>4) & 0xf;
			fg_pages[3] = data & 0xf;
			break;
	}
}
void tetris_bgpagesel_w(int offset, int data)
{
	switch (offset){
		case 0x0:
		case 0x1:
			bg_pages[0] = data>>12;
			bg_pages[1] = (data>>8) & 0xf;
			bg_pages[2] = (data>>4) & 0xf;
			bg_pages[3] = data & 0xf;
			break;
	}
}

void passshot_scroll_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_scrollram[offset], data);
	data = READ_WORD(&system16_scrollram[offset]);

//	system16_ram[0xf4bc+offset] = data;
	switch (offset)
	{
		case 0x0:  scrollY[FG_OFFS] = data; break;
		case 0x2:  scrollX[FG_OFFS] = data; break;
		case 0x4:  scrollY[BG_OFFS] = data; break;
		case 0x6:  scrollX[BG_OFFS] = data; break;
	}
}

void passshot_pagesel_w(int offset, int data)
{
	COMBINE_WORD_MEM(&system16_pagesram[offset], data);
	data = READ_WORD(&system16_pagesram[offset]);

	switch (offset){
		case 0x0:
		case 0x1:
			bg_pages[0] = data>>12;
			bg_pages[1] = (data>>8) & 0xf;
			bg_pages[2] = (data>>4) & 0xf;
			bg_pages[3] = data & 0xf;
			break;

		case 0x2:
		case 0x3:
			fg_pages[0] = data>>12;
			fg_pages[1] = (data>>8) & 0xf;
			fg_pages[2] = (data>>4) & 0xf;
			fg_pages[3] = data & 0xf;
			break;
	}
}
#if 0
void passshot_spriteram_w( int offset, int data )
{
	struct sys16_sprite_info *sprite = &sys16_sprite[offset>>4];

	COMBINE_WORD_MEM(&system16_spriteram[offset], data);
	data = READ_WORD(&system16_spriteram[offset]);

	switch( offset&0xF )
	{
		case 0x2:
		{
			/* Do not draw 0xffff sprite */
			if (data == 0xffff) data = 0x00ff;
			sprite->end_line = data>>8;
			sprite->begin_line = data&0xff;

			/* Bug workaround */
			if ((sprite->begin_line) != 0)
			{
				sprite->end_line += 2;
				sprite->begin_line += 2;
			}
		}
		break;

		case 0x0:
		sprite->horizontal_position = data;
		break;

		case 0x6:
		sprite->horizontal_flip = 0;
		sprite->vertical_flip = data&0x80;
		sprite->width = data&0x7f;
		if (data & 0x80)
		{
			sprite->number += 256-(data&0xff);
		}
		else
		{
			sprite->number -= sprite->width;
		}
		break;

		case 0x4:
		sprite->number = data;
		break;

		case 0xa:
		sprite->bank = (data>>4)&0xf;        /* [0x0b] >> 4 */
		sprite->priority = (data>>14)&0x3;
		sprite->color = (data>>4)&0x3f0;
		if (sprite->number & 0x8000)
		{
			sprite->bank = (sprite->bank + 15) & 0xf; /* Dec bank modularly */
			sprite->horizontal_flip = 1;
		}
		break;

		case 0x8:
		sprite->zoom = data&0x3ff;
		break;
	}
}
#endif

void passshot_spriteram_w( int offset, int data )
{
	struct sys16_sprite_info *sprite = &sys16_sprite[offset>>4];

	COMBINE_WORD_MEM(&system16_spriteram[offset], data);
	data = READ_WORD(&system16_spriteram[offset]);

	switch( offset&0xF )
	{
		case 0x2:
		{
			/* Do not draw 0xffff sprite */
			if (data == 0xffff) data = 0x00ff;
			sprite->end_line = data>>8;
			sprite->begin_line = data&0xff;

			/* Bug workaround */
			if ((sprite->begin_line) != 0)
			{
				sprite->end_line += 2;
				sprite->begin_line += 2;
			}
		}
		break;

		case 0x0:
		sprite->horizontal_position = data;
		break;

		case 0x6:
		sprite->vertical_flip = data&0x80;
		sprite->width = data&0x7f;
		if (data & 0x80)
		{
			sprite->number += 256-(data&0xff);
		}
		else
		{
			sprite->number -= sprite->width;
		}
		break;

		case 0x4:
		{
			sprite->horizontal_flip = data & 0x8000;
			sprite->number = data;
		}
		break;

		case 0xa:
		sprite->bank = (data>>4)&0xf;        /* [0x0b] >> 4 */
		sprite->priority = (data>>14)&0x3;
		sprite->color = (data>>4)&0x3f0; /* your color fix */
		if( sprite->horizontal_flip )/* dec bank modularly */
		{
			sprite->bank = (sprite->bank + 15) & 0xf;
		}
		break;

		case 0x8:
		sprite->zoom = data&0x3ff;
		break;
	}
}
