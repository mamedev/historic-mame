/*************************************************************************

  The  System8  machine hardware.   By Jarek Parchanski & Mirko Buffoni.

  This files describes the hardware behaviour of a System8 machine.
  It also includes common methods for handling of video hardware.

  Many thanks to Roberto Ventura, for precious information about
  System 8 hardware.

*************************************************************************/

#include "system8.h"

extern int system8_bank;

unsigned char 	*system8_scroll_y;
unsigned char 	*system8_scroll_x;
unsigned char 	*system8_videoram;
unsigned char 	*system8_spriteram;
unsigned char 	*system8_backgroundram;
unsigned char 	*system8_sprites_collisionram;
unsigned char 	*system8_background_collisionram;
unsigned char 	*system8_scrollx_ram;
int 	system8_videoram_size;
int 	system8_backgroundram_size;

static unsigned char	*bg_ram;
static unsigned char 	*bg_dirtybuffer;
static unsigned char 	*tx_dirtybuffer;
static unsigned char 	*SpritesData = NULL;
static unsigned char 	*SpritesCollisionTable;
static unsigned char 	scrollx=0,scrolly=0,system8_supports_banks=0,bg_bank=0,bg_bank_latch=0;

static int		scrollx_row[32];
static struct osd_bitmap *bitmap1;
static struct osd_bitmap *bitmap2;

static int  system8_pixel_mode = 0;
static void (*Check_SpriteRAM_for_Clear)(void) = NULL;


static unsigned char palette_lookup[256*3];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  There are two kind of color handling in System 8 games: in some, values in
  the palette RAM are directly mapped to colors with the usual BBGGGRRR format;
  in others, the value in the palette RAM is a lookup offset for three palette
  PROMs in RRRRGGGGBBBB format.
  It's hard to tell for sure because they use resistor packs, but here's
  what I think the values are from measurment with a volt meter:

  Blue: .250K ohms
  Blue: .495K ohms
  Green:.250K ohms
  Green:.495K ohms
  Green:.995K ohms
  Red:  .495K ohms
  Red:  .250K ohms
  Red:  .995K ohms

  accurate to +/- .003K ohms.

***************************************************************************/
void system8_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	palette = palette_lookup;

	if (color_prom)
	{
		for (i = 0;i < 256;i++)
		{
			int bit0,bit1,bit2,bit3;

			bit0 = (color_prom[0*256] >> 0) & 0x01;
			bit1 = (color_prom[0*256] >> 1) & 0x01;
			bit2 = (color_prom[0*256] >> 2) & 0x01;
			bit3 = (color_prom[0*256] >> 3) & 0x01;
			*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[1*256] >> 0) & 0x01;
			bit1 = (color_prom[1*256] >> 1) & 0x01;
			bit2 = (color_prom[1*256] >> 2) & 0x01;
			bit3 = (color_prom[1*256] >> 3) & 0x01;
			*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[2*256] >> 0) & 0x01;
			bit1 = (color_prom[2*256] >> 1) & 0x01;
			bit2 = (color_prom[2*256] >> 2) & 0x01;
			bit3 = (color_prom[2*256] >> 3) & 0x01;
			*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			color_prom++;
		}
	}
	else
	{
		for (i = 0;i < 256;i++)
		{
			int val;

			/* red component */
			val = (i >> 0) & 0x07;
			*(palette++) = (val << 5) | (val << 2) | (val >> 1);
			/* green component */
			val = (i >> 3) & 0x07;
			*(palette++) = (val << 5) | (val << 2) | (val >> 1);
			/* blue component */
			val = (i >> 5) & 0x06;
			if (val) val++;
			*(palette++) = (val << 5) | (val << 2) | (val >> 1);
		}
	}
}

void system8_paletteram_w(int offset,int data)
{
	unsigned char *palette = palette_lookup + data * 3;
	int r,g,b;


	paletteram[offset] = data;

	r = *palette++;
	g = *palette++;
	b = *palette++;

	palette_change_color(offset,r,g,b);
}



int system8_vh_start(void)
{
	if ((SpritesCollisionTable = malloc(0x10000)) == 0)
		return 1;
	memset(SpritesCollisionTable,255,0x10000);

	if ((bg_dirtybuffer = malloc(1024)) == 0)
	{
		free(SpritesCollisionTable);
		return 1;
	}
	memset(bg_dirtybuffer,1,1024);
	if ((tx_dirtybuffer = malloc(1024)) == 0)
	{
		free(bg_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	memset(tx_dirtybuffer,1,1024);
	if ((bg_ram = malloc(0x4000)) == 0)			/* Allocate 16k for background banked ram */
	{
		free(bg_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	memset(bg_ram,0,0x4000);
	if ((SpritesData = malloc(0x40000)) == 0)	/* Allocate a maximum for 512k of 16 colors sprites */
	{
		free(bg_ram);
		free(bg_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(bg_ram);
		free(bg_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesData);
		free(SpritesCollisionTable);
		return 1;
	}
	if ((bitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(bitmap1);
		free(bg_ram);
		free(bg_dirtybuffer);
		free(tx_dirtybuffer);
		free(SpritesData);
		free(SpritesCollisionTable);
		return 1;
	}

	return 0;
}

void system8_vh_stop(void)
{
	osd_free_bitmap(bitmap2);
	osd_free_bitmap(bitmap1);
	free(bg_ram);
	free(bg_dirtybuffer);
	free(tx_dirtybuffer);
	free(SpritesData);
	free(SpritesCollisionTable);
}


void system8_define_checkspriteram(void	(*check)(void))
{
	Check_SpriteRAM_for_Clear = check;
}

void system8_define_sprite_pixelmode(int Mode)
{
	system8_pixel_mode = Mode;
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

static int GetSpriteBottomY(int spr_number)
{
	return  system8_spriteram[(spr_number<<4) + SPR_Y_BOTTOM];
}


extern struct GameDriver wbml_driver;
static void Pixel(struct osd_bitmap *bitmap,int x,int y,int spr_number,int color)
{
	int xr,yr,spr_y1,spr_y2;
	int SprOnScreen;
	int adjx = x, adjy = y;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp = adjx;
		adjx = adjy;
		adjy = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		adjx = bitmap->width - adjx - 1;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		adjy = bitmap->height - adjy - 1;

	if (SpritesCollisionTable[256*y+x] == 255)
	{
		SpritesCollisionTable[256*y+x] = spr_number;
		bitmap->line[adjy][adjx] = color;
	}
	else
	{
		SprOnScreen=SpritesCollisionTable[256*y+x];
if (Machine->gamedrv != &wbml_driver)
		system8_sprites_collisionram[SprOnScreen + (spr_number<<5)] = 0xff;
		if (system8_pixel_mode==SYSTEM8_SPRITE_PIXEL_MODE1)
		{
			spr_y1 = GetSpriteBottomY(spr_number);
			spr_y2 = GetSpriteBottomY(SprOnScreen);
			if (spr_y1 >= spr_y2)
			{
				bitmap->line[adjy][adjx]=color;
				SpritesCollisionTable[256*y+x]=spr_number;
			}
		} else
		{
			bitmap->line[adjy][adjx]=color;
			SpritesCollisionTable[256*y+x]=spr_number;
		}
	}
	xr = (x>>3);
	yr = (y>>3);
if (Machine->gamedrv != &wbml_driver)
	if (system8_backgroundram[((yr%32)<<6) + ((xr%32)<<1) + 1] & 0x10)
		system8_background_collisionram[spr_number] = 0xff;
}


static void MarkBackgroundDirtyBufferBySprite(int x,int y,int width,int height)
{
	int xr,yr,wr,hr,row,col;

	x  = (unsigned char)((x - scrollx) % 256);
	y  = (unsigned char)((scrolly + y) % 256);
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


static void RenderSprite(struct osd_bitmap *bitmap,int spr_number,const struct rectangle *clip)
{
	int SprX,SprY,Col,Row,Height,DataOffset,FlipX;
	int Color,scrx,scry,Bank;
	unsigned char *SprReg;
	unsigned short *SprPalette;
	unsigned short NextLine,Width,Offset16;
	struct rectangle myclip = { 0, 255, 0, 255 };

	if (clip == 0) clip = &myclip;

	SprReg		= system8_spriteram + (spr_number<<4);
	Bank		= ((((SprReg[SPR_BANK] & 0x80)>>7)+((SprReg[SPR_BANK] & 0x40)>>5))<<16) * system8_supports_banks;
	Width 		= SprReg[SPR_WIDTH_LO] + (SprReg[SPR_WIDTH_HI]<<8);
	Height		= SprReg[SPR_Y_BOTTOM] - SprReg[SPR_Y_TOP];
	FlipX 	    = SprReg[SPR_FLIP_X] & 0x80;
	DataOffset 	= SprReg[SPR_GFXOFS_LO]+((SprReg[SPR_GFXOFS_HI] & 0x7F)<<8)+Width;
	SprPalette	= Machine->colortable + (spr_number<<4);
	SprX 		= (SprReg[SPR_X_LO] >> 1) + ((SprReg[SPR_X_HI] & 1) << 7) + 1;
if (Machine->gamedrv == &wbml_driver) SprX += 6;
	SprY 		= SprReg[SPR_Y_TOP] + 1;
	NextLine	= Width;

	if (DataOffset & 0x8000) FlipX^=0x80;
	if (Width & 0x8000) Width = (~Width)+1;		// width isn't positive
							// and this means that sprite will has fliped Y
	Width<<=1;

	MarkBackgroundDirtyBufferBySprite(SprX,SprY,Width,Height);

	for(Row=0; Row<Height; Row++)
	{
		Offset16 = (DataOffset+(Row*NextLine)) << 1;
		scry = (unsigned char)(((SprY+Row) + scrolly) % 256);
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


	if (Check_SpriteRAM_for_Clear)
		Check_SpriteRAM_for_Clear();

if (Machine->gamedrv != &wbml_driver)
	memset(system8_sprites_collisionram,0,0x400);

	for(spr_number=0; spr_number<32; spr_number++)
	{
		SprReg 		= system8_spriteram + (spr_number<<4);
		SprTop		= SprReg[SPR_Y_TOP];
		SprBottom	= SprReg[SPR_Y_BOTTOM];
		if (SprBottom && (SprBottom-SprTop > 0))
			RenderSprite(bitmap,spr_number,clip);
	}

	ClearSpritesCollisionTable();
}



void system8_compute_palette (void)
{
	unsigned char bg_usage[64], tx_usage[64], sp_usage[32];
	int i;

	memset (bg_usage, 0, sizeof (bg_usage));
	memset (tx_usage, 0, sizeof (tx_usage));
	memset (sp_usage, 0, sizeof (sp_usage));

	for (i = 0; i<system8_backgroundram_size; i+=2)
	{
		int code = (system8_backgroundram[i] + (system8_backgroundram[i+1] << 8)) & 0x7FF;
		int palette = code >> 5;
		bg_usage[palette & 0x3f] = 1;
	}

	for (i = 0; i<system8_videoram_size; i+=2)
	{
		int code = (system8_videoram[i] + (system8_videoram[i+1] << 8)) & 0x7FF;

		if (code)
		{
			int palette = code>>5;
			tx_usage[palette & 0x3f] = 1;
		}
	}

	for (i=0; i<32; i++)
	{
		unsigned char *reg;
		int top, bottom;

		reg 	= system8_spriteram + (i<<4);
		top		= reg[SPR_Y_TOP];
		bottom	= reg[SPR_Y_BOTTOM];
		if (bottom && (bottom - top > 0))
			sp_usage[i] = 1;
	}

	for (i = 0; i < 64; i++)
	{
		if (bg_usage[i])
			memset (palette_used_colors + 1024 + i * 8, PALETTE_COLOR_USED, 8);
		else
			memset (palette_used_colors + 1024 + i * 8, PALETTE_COLOR_UNUSED, 8);

		palette_used_colors[512 + i * 8] = PALETTE_COLOR_TRANSPARENT;
		if (tx_usage[i])
			memset (palette_used_colors + 512 + i * 8 + 1, PALETTE_COLOR_USED, 7);
		else
			memset (palette_used_colors + 512 + i * 8 + 1, PALETTE_COLOR_UNUSED, 7);
	}

	for (i = 0; i < 32; i++)
	{
		palette_used_colors[0 + i * 16] = PALETTE_COLOR_TRANSPARENT;
		if (sp_usage[i])
			memset (palette_used_colors + 0 + i * 16 + 1, PALETTE_COLOR_USED, 15);
		else
			memset (palette_used_colors + 0 + i * 16 + 1, PALETTE_COLOR_UNUSED, 15);
	}

	if (palette_recalc ())
	{
		memset(bg_dirtybuffer,1,1024);
		memset(tx_dirtybuffer,1,1024);
	}
}


void system8_videoram_w(int offset,int data)
{
	system8_videoram[offset] = data;
	tx_dirtybuffer[offset>>1] = 1;
}

void system8_backgroundram_w(int offset,int data)
{
	system8_backgroundram[offset] = data;
	bg_dirtybuffer[offset>>1] = 1;
}

void system8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int scroll_x,scroll_y, sx,sy, i;


	system8_compute_palette ();


	scrollx = (system8_scroll_x[0] >> 1) + ((system8_scroll_x[1] & 1) << 7) + 15;
	scrolly = *system8_scroll_y;

	/* for every character in the background video RAM, check if it has
	 * been modified since last time and update it accordingly.
	 */

	for (i = 0; i<system8_backgroundram_size; i+=2)
	{
		int code = (system8_backgroundram[i] + (system8_backgroundram[i+1] << 8)) & 0x7FF;
		int palette = code >> 5;

		if (bg_dirtybuffer[i>>1])
		{
			bg_dirtybuffer[i>>1] = 0;
			sx = (i % 64) << 2;
			sy = (i >> 6) << 3;
			drawgfx(bitmap1,Machine->gfx[0],
					code,palette + 64,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	DrawSprites(bitmap1,0);


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

			drawgfx(bitmap1,Machine->gfx[0],
					code,palette + 64,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
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
			sy = ((i >> 6)<<3);
			drawgfx(bitmap,Machine->gfx[0],
					code,palette,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
		}
	}

}


void choplifter_scroll_x_w(int offset,int data)
{
	system8_scrollx_ram[offset] = data;

	scrollx_row[offset/2] = (system8_scrollx_ram[offset & ~1] >> 1) + ((system8_scrollx_ram[offset | 1] & 1) << 7);
}


void choplifter_backgroundrefresh(struct osd_bitmap *bitmap, int layer)
{
	int code,palette,priority,sx,sy,i;
	int choplifter_scroll_x_on = (system8_scrollx_ram[0] == 0xE5 && system8_scrollx_ram[1] == 0xFF) ? 0:1;

	/*	for every character in the background & text video RAM,
	 *	check if it has been modified since last time and update it accordingly.
	 */

	for (i = 0; i < system8_backgroundram_size>>1; i++)
	{
		code = system8_backgroundram[i*2] + (system8_backgroundram[i*2+1] << 8);
		priority = (code >> 11) & 0x0f;
		palette = (code>>5) & 0x3f;

		if (bg_dirtybuffer[i])
		{
			if (!priority)
				bg_dirtybuffer[i]=0;
			sx = (i % 32)<<3;
			sy = ((i >> 5)<<3);
			code = ((code >> 4) & 0x800) | (code & 0x7ff);

			if (!layer)
				drawgfx(bitmap1,Machine->gfx[0],
					code,palette + 64,0,0,sx,sy,0,TRANSPARENCY_NONE,0);
			else
			if (priority & layer)
			{
				if (choplifter_scroll_x_on)
				{
					int row = (i >> 5);
					sx = (((i % 32)<<3)+scrollx_row[row]) % 256;
					sy = row<<3;
					drawgfx(bitmap,Machine->gfx[0],
							code,palette + 64,0,0,sx,sy,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
				else
				{
					sx = ((i % 32)<<3);
					sy = ((i >> 5)<<3);
					drawgfx(bitmap,Machine->gfx[0],
							code,palette + 64,0,0,sx,sy,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}

	if (!layer)
	{
		if (choplifter_scroll_x_on)
			copyscrollbitmap(bitmap,bitmap1,32,scrollx_row,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		else
			copybitmap(bitmap,bitmap1,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

void choplifter_textrefresh(struct osd_bitmap *bitmap, int layer)
{
	int offs;


	for (offs = 0;offs < system8_videoram_size;offs += 2)
	{
		int sx,sy,code,priority;


		sx = (offs/2) % 32;
		sy = (offs/2) / 32;
		code = system8_videoram[offs] | (system8_videoram[offs+1] << 8);
		priority = code & 0x800;
		code = ((code >> 4) & 0x800) | (code & 0x7ff);

		if (layer)
		{
			if (priority)
			{
				drawgfx(bitmap,Machine->gfx[0],
						code,
						(code >> 5) & 0x3f,
						0,0,
						8*sx,8*sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
		else
		{
			if (tx_dirtybuffer[offs/2])
			{
				tx_dirtybuffer[offs/2] = 0;

				drawgfx(bitmap2,Machine->gfx[0],
						code,
						(code >> 5) & 0x3f,
						0,0,
						8*sx,8*sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
		}
	}

	if (!layer)
		copybitmap(bitmap,bitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}

void choplifter_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	system8_compute_palette ();
	choplifter_backgroundrefresh(bitmap,0);
	choplifter_textrefresh(bitmap,0);
	choplifter_backgroundrefresh(bitmap,2);
	DrawSprites(bitmap,&Machine->drv->visible_area);
	choplifter_backgroundrefresh(bitmap,1);
	choplifter_textrefresh(bitmap,1);

#ifdef MAME_DEBUG
	if (osd_key_pressed(OSD_KEY_SPACE))		// goto next level
	{
		Machine->memory_region[0][0xC085]=33;
	}
#endif
}







int wbml_bg_bankselect_r(int offset)
{
	return bg_bank_latch;
}

void wbml_bg_bankselect_w(int offset, int data)
{
	bg_bank_latch = data;
	bg_bank = (data >> 1) & 0x03;	/* Select 4 banks of 4k, bit 2,1 */
}
void wbml_paged_videoram_w(int offset,int data)
{
	bg_ram[0x1000*bg_bank + offset] = data;
}

void wbml_backgroundrefresh(struct osd_bitmap *bitmap, int trasp)
{
	int page;


	int xscroll = (bg_ram[0x7c0] >> 1) + ((bg_ram[0x7c1] & 1) << 7) - 256 + 5;
	int yscroll = -bg_ram[0x7ba];

	for (page=0; page < 4; page++)
	{
		const unsigned char *source = bg_ram + (bg_ram[0x0740 + page*2] & 0x07)*0x800;
		int startx = (page&1)*256+xscroll;
		int starty = (page>>1)*256+yscroll;
		int row,col;


		for( row=0; row<32*8; row+=8 )
		{
			for( col=0; col<32*8; col+=8 )
			{
				int code,priority;
				int x = (startx+col) & 0x1ff;
				int y = (starty+row) & 0x1ff;
				if (x > 256) x -= 512;
				if (y > 256) y -= 512;


				code = source[0] + (source[1] << 8);
				priority = code & 0x800;
				code = ((code >> 4) & 0x800) | (code & 0x7ff);

				if (!trasp)
					drawgfx(bitmap,Machine->gfx[0],
							code,
							((code >> 5) & 0x3f) + 64,
							0,0,
							x,y,
							&Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
				else if (priority)
					drawgfx(bitmap,Machine->gfx[0],
							code,
							((code >> 5) & 0x3f) + 64,
							0,0,
							x,y,
							&Machine->drv->visible_area, TRANSPARENCY_PEN, 0);
				source+=2;
			}
		}
	} /* next page */
}

void wbml_textrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = 0;offs < 0x700;offs += 2)
	{
		int sx,sy,code;


		sx = (offs/2) % 32;
		sy = (offs/2) / 32;
		code = bg_ram[offs] | (bg_ram[offs+1] << 8);
		code = ((code >> 4) & 0x800) | (code & 0x7ff);

		drawgfx(bitmap,Machine->gfx[0],
				code,
				(code >> 5) & 0x3f,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}


void wbml_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	palette_recalc();
	wbml_backgroundrefresh(bitmap,0);
	DrawSprites(bitmap,&Machine->drv->visible_area);
	wbml_backgroundrefresh(bitmap,1);
	wbml_textrefresh(bitmap);
}
