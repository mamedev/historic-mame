/*************************************************************************

  The  System8  machine hardware.   By Jarek Parchanski & Mirko Buffoni.

  This files describes the hardware behaviour of a System8 machine.
  It also includes common methods for handling of video hardware.

  Many thanks to Roberto Ventura, for precious information about
  System 8 hardware.

*************************************************************************/

#include "machine/system8.h"

unsigned char	*system8_bg_pagesel;
unsigned char 	*system8_scroll_y;
unsigned char 	*system8_scroll_x;
unsigned char 	*system8_paletteram;
unsigned char 	*system8_spritepaletteram;
unsigned char 	*system8_backgroundpaletteram;
unsigned char 	*system8_videoram;
unsigned char 	*system8_spriteram;
unsigned char 	*system8_backgroundram;
unsigned char 	*system8_sprites_collisionram;
unsigned char 	*system8_background_collisionram;
unsigned char 	*system8_scrollx_ram;
int 	system8_videoram_size;
int 	system8_paletteram_size;
int 	system8_spritepaletteram_size;
int 	system8_backgroundram_size;
int 	system8_backgroundpaletteram_size;

static unsigned char	*bg_ram;
static unsigned char 	*bg_dirtybuffer;
static unsigned char 	*bg_palette_dirtybuffer;
static unsigned char 	*tx_dirtybuffer;
static unsigned char 	*tx_palette_dirtybuffer;
static unsigned char 	*SpritesData = NULL;
static unsigned char 	*SpritesCollisionTable;
static unsigned char 	scrollx=0,scrolly=0,system8_supports_banks=0,bg_bank=0,bg_bank_latch=0,sprite_offset_y=16;

static int		scrollx_row[32];
static struct osd_bitmap *bitmap1;
static struct osd_bitmap *bitmap2;
static struct rectangle system8_clip;

static int  system8_bank = 0x0f;
static void (*Check_SpriteRAM_for_Clear)(void) = NULL;



int system8_vh_start(void)
{
	int i;

	if ((SpritesCollisionTable = malloc(0x10000)) == 0)
		return 1;

	if ((bg_dirtybuffer = malloc(1024)) == 0)
	{
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bg_palette_dirtybuffer = malloc(64)) == 0)
	{
		free(bg_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((tx_dirtybuffer = malloc(1024)) == 0)
	{
		free(bg_dirtybuffer);
		free(bg_palette_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((tx_palette_dirtybuffer = malloc(64)) == 0)
	{
		free(tx_dirtybuffer);
		free(bg_dirtybuffer);
		free(bg_palette_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bg_ram = malloc(0x4000)) == 0)			/* Allocate 16k for background banked ram */
	{
		free(bg_palette_dirtybuffer);
		free(bg_dirtybuffer);
		free(tx_palette_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((SpritesData = malloc(0x40000)) == 0)	/* Allocate a maximum for 512k of 16 colors sprites */
	{
		free(bg_ram);
		free(bg_palette_dirtybuffer);
		free(bg_dirtybuffer);
		free(tx_palette_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(bg_ram);
		free(bg_palette_dirtybuffer);
		free(bg_dirtybuffer);
		free(tx_palette_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesData);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(bitmap1);
		free(bg_ram);
		free(bg_palette_dirtybuffer);
		free(bg_dirtybuffer);
		free(tx_palette_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesData);
		free(SpritesCollisionTable);
		return 1;
	}

	memset(bg_dirtybuffer,0,1024);
	memset(bg_palette_dirtybuffer,0,64);
	memset(tx_dirtybuffer,0,1024);
	memset(tx_palette_dirtybuffer,0,64);
	memset(system8_backgroundram,0,system8_backgroundram_size);
	memset(system8_videoram,0,system8_videoram_size);
	memset(SpritesCollisionTable,255,0x10000);

	for (i=0; i < 512; i++)
	{
		Machine->gfx[0]->colortable[i] = Machine->pens[0];
		Machine->gfx[1]->colortable[i] = Machine->pens[0];
	}
	return 0;
}

void system8_vh_stop(void)
{
	osd_free_bitmap(bitmap2);
	osd_free_bitmap(bitmap1);
	free(bg_ram);
	free(bg_palette_dirtybuffer);
	free(bg_dirtybuffer);
	free(tx_palette_dirtybuffer);
	free(tx_dirtybuffer);
	free(SpritesData);
	free(SpritesCollisionTable);
}


void system8_define_checkspriteram(void	(*check)(void))
{
	Check_SpriteRAM_for_Clear = check;
}

void system8_define_spritememsize(int region, int size)
{
	unsigned char *SpritesPackedData = Machine->memory_region[region];

	if (!SpritesData)
		SpritesData = malloc(size*2);

	if (SpritesData)
	{
		int a;
		for (a=0; a < size; a++)
		{
			SpritesData[a*2  ] = SpritesPackedData[a] >> 4;
			SpritesData[a*2+1] = SpritesPackedData[a] & 0xF;
		}
	}
}

void system8_define_banksupport(int support)
{
	system8_supports_banks = support;
}

void system8_define_sprite_offset_y(int offset)
{
	sprite_offset_y = offset;
}

void system8_define_cliparea(int x1, int x2, int y1, int y2)
{
	system8_clip.min_x = x1;
	system8_clip.max_x = x2;
	system8_clip.min_y = y1;
	system8_clip.max_y = y2;
}

static int GetSpriteBottomY(int spr_number)
{
	return  system8_spriteram[(spr_number<<4) + SPR_Y_BOTTOM];
}


static void Pixel(struct osd_bitmap *bitmap,int x,int y,int spr_number,int color)
{
	int xr,yr,spr_y1,spr_y2;
	int SprOnScreen;

	if (SpritesCollisionTable[256*y+x] == 255)
	{
		SpritesCollisionTable[256*y+x] = spr_number;
		bitmap->line[y][x] = Machine->pens[color];
	}
	else
	{
		SprOnScreen=SpritesCollisionTable[256*y+x];
		system8_sprites_collisionram[SprOnScreen + (spr_number<<5)] = 0xff;
		spr_y1 = GetSpriteBottomY(spr_number);
		spr_y2 = GetSpriteBottomY(SprOnScreen);
		if (spr_y1 >= spr_y2)
		{
			bitmap->line[y][x]=Machine->pens[color];
			SpritesCollisionTable[256*y+x]=spr_number;
		}
	}
	xr = (x>>3);
	yr = (y>>3);
	if (system8_backgroundram[((yr%32)<<6) + ((xr%32)<<1) + 1] & 0x10)
	  system8_background_collisionram[spr_number] = 0xff;
}


static void MarkBackgroundDirtyBufferBySprite(int x,int y,int width,int height)
{
	int xr,yr,wr,hr,row,col;

	x  = (unsigned char)((x - scrollx) % 256);
	y  = (unsigned char)((scrolly + y + sprite_offset_y) % 256);
	xr = (x>>3);
	yr = (y>>3);
	wr = (width>>3)  + ((x & 7) ? 1:0) +  ((width & 7) ? 1:0);
	hr = (height>>3) + ((y & 7) ? 1:0) + ((height & 7) ? 1:0);

	for (row=0;row<hr;row++)
	{
		int sumrow = (((row+yr)%32)<<5);
		for (col=0;col<wr;col++)
		{
//			tx_dirtybuffer[sumrow+((col+xr)%32)] = 1;
			bg_dirtybuffer[sumrow+((col+xr)%32)] = 1;
		}
	}
}


static void RenderSprite(struct osd_bitmap *bitmap, const struct rectangle *clip, int spr_number)
{
	int SprX,SprY,Col,Row,Height,DataOffset,FlipX;
	int Color,scrx,scry,Bank;
	unsigned char *SprPalette,*SprReg;
	unsigned short NextLine,Width,Offset16;

	SprReg		= system8_spriteram + (spr_number<<4);
	Bank		= ((((SprReg[SPR_BANK] & 0x80)>>7)+((SprReg[SPR_BANK] & 0x40)>>5))<<16) * system8_supports_banks;
	Width 		= SprReg[SPR_WIDTH_LO] + (SprReg[SPR_WIDTH_HI]<<8);
	Height		= SprReg[SPR_Y_BOTTOM] - SprReg[SPR_Y_TOP];
	FlipX 		= SprReg[SPR_FLIP_X] & 0x80;
	DataOffset 	= SprReg[SPR_GFXOFS_LO]+((SprReg[SPR_GFXOFS_HI] & 0x7F)<<8)+Width;
	SprPalette	= system8_spritepaletteram + (spr_number<<4);
	SprX 		= (SprReg[SPR_X_LO] >> 1) + ((SprReg[SPR_X_HI] & 1) << 7);
	SprY 		= SprReg[SPR_Y_TOP];
	NextLine	= Width;

	if (DataOffset & 0x8000) FlipX^=0x80;
	if (Width & 0x8000) Width = (~Width)+1;		// width isn't positive
							// and this means that sprite will has fliped Y
	Width<<=1;

	MarkBackgroundDirtyBufferBySprite(SprX,SprY,Width,Height);

	for(Row=0; Row<Height; Row++)
	{
		Offset16 = (DataOffset+(Row*NextLine)) << 1;
		scry = (unsigned char)(((SprY+Row) + scrolly + sprite_offset_y) % 256);
		if ((clip->min_y <= SprY+Row) && (clip->max_y >= SprY+Row))
		{
			for(Col=0; Col<Width; Col++)
			{
				Color = (FlipX) ? SpritesData[Bank+Offset16-Col+1]
						: SpritesData[Bank+Offset16+Col];
				if (Color == 15) break;
				if (Color && (clip->min_x <= SprX+Col) && (clip->max_x >= SprX+Col))
				{
					scrx = (unsigned char)(((SprX+Col) - scrollx) % 256);
					Pixel(bitmap,scrx,scry,spr_number,SprPalette[Color]);
				}
			}
		}
	}
}


static void ClearSpritesCollisionTable(void)
{
	int col,row,i,sx,sy;

	for (i = 0; i<1024; i++)
	{
		if (bg_dirtybuffer[i])
		{
			sx = (i % 32)<<3;
			sy = (i >> 5)<<3;
			for(row=sy; row<sy+8; row++)
				for(col=sx; col<sx+8; col++)
					SpritesCollisionTable[256*row+col] = 255;
		}
	}
}


void pitfall2_clear_spriteram(void)
{
	if (!*system8_backgroundram && system8_videoram[212] != 'M' &&
			system8_spriteram[SPR_GFXOFS_LO] != 0xDD &&
				system8_spriteram[SPR_GFXOFS_HI] != 0x2C)
		memset(system8_spriteram,0,0x200);		/* Workaround for Pitfall!  TODO BETTER! */
}

static void DrawSprites(struct osd_bitmap *bitmap,const struct rectangle *clip)
{
	int spr_number,SprBottom,SprTop;
	unsigned char *SprReg;

	if (!clip) clip = &Machine->drv->visible_area;

	if (Check_SpriteRAM_for_Clear)
		Check_SpriteRAM_for_Clear();

	memset(system8_sprites_collisionram,0,0x400);

	for(spr_number=0; spr_number<32; spr_number++)
	{
		SprReg 		= system8_spriteram + (spr_number<<4);
		SprTop		= SprReg[SPR_Y_TOP];
		SprBottom	= SprReg[SPR_Y_BOTTOM];
		if (SprBottom && (SprBottom-SprTop > 0))
			RenderSprite(bitmap,clip,spr_number);
	}

	ClearSpritesCollisionTable();
}



void system8_soundport_w(int offset, int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1, Z80_NMI_INT);
}

int  system8_bg_bankselect_r(int offset)
{
	return bg_bank_latch;
}

void system8_bg_bankselect_w(int offset, int data)
{
	if (data != bg_bank_latch)
	{
		bg_bank_latch = data;
		bg_bank = (data >> 1) & 0x03;	/* Select 4 banks of 4k, bit 2,1 */
	}
}

void system8_videoram_w(int offset,int data)
{
	system8_videoram[offset] = data;
	bg_ram[0x1000*bg_bank + offset] = data;
	tx_dirtybuffer[offset>>1] = 1;
}

void system8_paletteram_w(int offset,int data)
{
	system8_paletteram[offset] = data;
	tx_palette_dirtybuffer[offset>>3] = 1;
	Machine->gfx[1]->colortable[offset] = Machine->pens[data];
}

void system8_backgroundram_w(int offset,int data)
{
	system8_backgroundram[offset] = data;
	bg_ram[0x1000*bg_bank + offset + 0x800] = data;
	bg_dirtybuffer[offset>>1] = 1;
}

void system8_backgroundpaletteram_w(int offset,int data)
{
	system8_backgroundpaletteram[offset] = data;
	bg_palette_dirtybuffer[offset>>3] = 1;
	Machine->gfx[0]->colortable[offset] = Machine->pens[data];
}

void system8_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int scroll_x,scroll_y, sx,sy, i;
	int pitfall2_trans_pen;

	scrollx = (system8_scroll_x[0] >> 1) + ((system8_scroll_x[1] & 1) << 7) + 14;
	scrolly = *system8_scroll_y - 16;

	/* for every character in the background video RAM, check if it has
	 * been modified since last time and update it accordingly.
	 */

	for (i = 0; i<system8_backgroundram_size; i+=2)
	{
		int code = (system8_backgroundram[i] + (system8_backgroundram[i+1] << 8)) & 0x7FF;
		int palette = code >> 5;

		if (bg_dirtybuffer[i>>1] || bg_palette_dirtybuffer[palette])
		{
			bg_dirtybuffer[i>>1] = 0;
			sx = (i % 64) << 2;
			sy = (i >> 6) << 3;
			drawgfx(bitmap1,Machine->gfx[0],
					code,palette,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
	memset(bg_palette_dirtybuffer,0,64);

	DrawSprites(bitmap1, &system8_clip);


	/* for every character in the background video RAM,
	 * check if it has clip attribute and update it accordingly.
	 */

	for (i = 0; i<system8_backgroundram_size; i+=2)
	{
		if (system8_backgroundram[i+1] & 8)
		{
			int code = (system8_backgroundram[i] + (system8_backgroundram[i+1] << 8)) & 0x7FF;
			int palette = code >> 5;
			sx = (i % 64) << 2;
			sy = (i >> 6) << 3;
			pitfall2_trans_pen=0;

			if (strcmp(Machine->gamedrv->name,"pitfall")==0)
			{
			   if (code==1658) pitfall2_trans_pen=5;
			   if (code==1660) pitfall2_trans_pen=6;
			   if (code==227  || code==976 || (code>977 && code<983)) pitfall2_trans_pen=3;
			   if (code==1645 || code==1046 || code==1039 || code==1805
						  || code==1820 || code==1641) pitfall2_trans_pen=1;
			}
			drawgfx(bitmap1,Machine->gfx[0],
					code,palette,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,pitfall2_trans_pen);
		}
	}


	/* copy the temporary bitmap to the screen */

	scroll_x = scrollx;
	scroll_y = -scrolly;
	copyscrollbitmap(bitmap,bitmap1,1,&scroll_x,1,&scroll_y,
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* for every character in the text video RAM,
	 * check if it different than 0 and update it accordingly.
	 */

	for (i = 0; i<system8_videoram_size; i+=2)
	{
		int code = (system8_videoram[i] + (system8_videoram[i+1] << 8)) & 0x7FF;

		if (code)
		{
			int palette = code>>5;
			sx = (i % 64)<<2;
			sy = ((i >> 6)<<3)+16;
			drawgfx(bitmap,Machine->gfx[1],
					code,palette,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
		}
	}

}


void system8_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[0*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[0*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[0*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[1*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[1*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[1*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[1*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		color_prom++;
	}
}



void system8_bankswitch_w(int offset,int data)
{
	int bankaddress;

	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);

	system8_bank=data;
}

int system8_bankswitch_r(int offset)
{
	return(system8_bank);
}



void choplifter_scroll_x_w(int offset,int data)
{
	system8_scrollx_ram[offset] = data;

	scrollx_row[offset/2] = (system8_scrollx_ram[offset & ~1] >> 1) + ((system8_scrollx_ram[offset | 1] & 1) << 7);
}


void choplifter_backgroundrefresh(struct osd_bitmap *bitmap, int layer)
{
	int code,palette,priority,sx,sy,i;
	int choplifter_scroll_x_on = (system8_scrollx_ram[4] == 0xE5 && system8_scrollx_ram[5] == 0xFF) ? 0:1;

	/*	for every character in the background & text video RAM,
	 *	check if it has been modified since last time and update it accordingly.
	 */

	for (i = 0; i < system8_backgroundram_size>>1; i++)
	{
		code = system8_backgroundram[i*2] + (system8_backgroundram[i*2+1] << 8);
		priority = (code >> 11) & 0x0f;
		palette = (code>>5) & 0x3f;

		if (bg_dirtybuffer[i] || bg_palette_dirtybuffer[palette])
		{
			if (!priority)
				bg_dirtybuffer[i]=0;
			sx = (i % 32)<<3;
			sy = ((i >> 5)<<3)+16;
			code = ((code >> 4) & 0x800) | (code & 0x7ff);

//			code = 48+priority;
			if (!layer)
				drawgfx(bitmap1,Machine->gfx[0],
					code,palette,0,0,sx,sy,0,TRANSPARENCY_NONE,0);
			else
			if (priority & layer)
			{
				if (choplifter_scroll_x_on)
				{
					int row = (i >> 5)+2;
					sx = (((i % 32)<<3)+scrollx_row[row]) % 256;
					sy = row<<3;
					drawgfx(bitmap,Machine->gfx[0],
							code,palette,0,0,sx,sy,&system8_clip,TRANSPARENCY_PEN,0);
				}
				else
				{
					sx = ((i % 32)<<3);
					sy = ((i >> 5)<<3);
					drawgfx(bitmap,Machine->gfx[0],
							code,palette,0,0,sx,sy+16,&system8_clip,TRANSPARENCY_PEN,0);
				}
			}
		}
	}
	memset(bg_palette_dirtybuffer,0,64);

	if (!layer)
	{
		if (choplifter_scroll_x_on)
			copyscrollbitmap(bitmap,bitmap1,32,scrollx_row,0,0,&system8_clip,TRANSPARENCY_NONE,0);
		else
			copybitmap(bitmap,bitmap1,0,0,0,0,&system8_clip,TRANSPARENCY_NONE,0);
	}
}

void choplifter_textrefresh(struct osd_bitmap *bitmap, int layer)
{
	int code,palette,priority,sx,sy,i;

	/*	for every character in the background & text video RAM,
	 *	check if it has been modified since last time and update it accordingly.
	 */

	for (i = 0; i < system8_backgroundram_size>>1; i++)
	{
		code = system8_videoram[i*2] + (system8_videoram[i*2+1] << 8);
		priority = (code >> 11) & 0x01;
		palette = (code>>5) & 0x3f;

		if (tx_dirtybuffer[i] || tx_palette_dirtybuffer[palette])
		{
			if (!priority)
				tx_dirtybuffer[i]=0;
			sx = (i % 32)<<3;
			sy = ((i >> 5)<<3);
			code = ((code >> 4) & 0x800) | (code & 0x7ff);

//			code = 48+priority;
			if (layer)
				drawgfx(bitmap,Machine->gfx[1],
					code,palette,0,0,sx,sy+16,&system8_clip,TRANSPARENCY_PEN,0);
			else
				drawgfx(bitmap2,Machine->gfx[1],
					code,palette,0,0,sx,sy,0,TRANSPARENCY_NONE,0);
		}
	}
	memset(tx_palette_dirtybuffer,0,64);

	if (!layer)
	/*  Copy background & text bitmap to the screen */
	copybitmap(bitmap,bitmap2,0,0,0,16,&system8_clip,TRANSPARENCY_COLOR,131);
//	else
//	copybitmap(bitmap,bitmap2,0,0,0,16,&system8_clip,TRANSPARENCY_NONE,0);

}


void choplifter_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	choplifter_backgroundrefresh(bitmap,0);
	choplifter_textrefresh(bitmap,0);
	choplifter_backgroundrefresh(bitmap,2);
	DrawSprites(bitmap,&system8_clip);
	choplifter_backgroundrefresh(bitmap,1);
	choplifter_textrefresh(bitmap,1);

#ifdef MAME_DEBUG
	if (osd_key_pressed(OSD_KEY_SPACE))		// goto next level
	{
		Machine->memory_region[0][0xC085]=33;
	}
#endif
}



void system8_backgroundrefresh(struct osd_bitmap *bitmap, int trasp)
{
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;

	unsigned char * scrx = ((system8_bank&0x80)?(system8_videoram + 0x7F6):(system8_videoram + 0x7c0));
	unsigned char * scry = ((system8_bank&0x80)?(system8_videoram + 0x784):(system8_videoram + 0x7ba));

	int scrollx = (scrx[0] >> 1) + ((scrx[1] & 1) << 7)-256-2;
	int scrolly = -*scry;

	if (scrollx < 0) scrollx = 512 - (-scrollx) % 512; else scrollx = scrollx % 512;
	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512; else scrolly = scrolly % 512;

	for (page=0; page < 4; page++)
	{
		const unsigned char *source = bg_ram + (system8_bg_pagesel[ page*2 ] & 0x07)*0x800;

		int startx = (page&1)*256+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<28*8; row+=8 )
		{
			for( col=0; col<32*8; col+=8 )
			{
				int x = startx+col;
				int y = starty+row;
				if( x>256 ) x-=512;
				if( y>224 ) y-=512;
				if( x>-8 && x<256 && y>-8 && y<224 )
				{
					int tile = source[0] + source[1]*256;
					int priority = tile & 0x800;
					tile = ((tile >> 4) & 0x800)|(tile & 0x7ff);
					if (!trasp)
						drawgfx(bitmap, gfx,
								tile,
								(tile >> 5),
								0, 0, x, y,
								&system8_clip, TRANSPARENCY_NONE, 0);
					else if (priority)
						drawgfx(bitmap, gfx,
								tile,
								(tile >> 5),
								0, 0, x, y,
								&system8_clip, TRANSPARENCY_COLOR, 0);
				}
				source+=2;
			}
		}
	} /* next page */
}

void system8_textrefresh(struct osd_bitmap *bitmap)
{
	int i;

	for (i = 0; i < system8_videoram_size>>1; i++)
	{
		int code = bg_ram[i*2] | (bg_ram[i*2+1] << 8);
		int palette = (code>>5) & 0x3f;

		if (tx_dirtybuffer[i] || tx_palette_dirtybuffer[palette])
		{
			int sx,sy;

			tx_dirtybuffer[i]=0;
			sx = (i % 32)<<3;
			sy = ((i >> 5)<<3);
			code = ((code >> 4) & 0x800) + (code & 0x7ff);

			if (sx < 7*8 || sy < 3*8 || sy >= 216)
				drawgfx(bitmap,Machine->gfx[1],
					code,palette,0,0,sx,sy,0,TRANSPARENCY_NONE,0);
			else
				drawgfx(bitmap,Machine->gfx[1],
					code,palette,0,0,sx,sy,0,TRANSPARENCY_COLOR,0);
		}
	}
}

void wbml_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	system8_backgroundrefresh(bitmap,0);
	DrawSprites(bitmap,&system8_clip);
	system8_backgroundrefresh(bitmap,1);
	system8_textrefresh(bitmap);
}
