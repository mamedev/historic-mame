/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"

#define ROTATE_TILE_WIDTH   256
#define ROTATE_TILE_HEIGHT  256
#define ROTATE_PIXEL_WIDTH  (ROTATE_TILE_WIDTH*8)
#define ROTATE_PIXEL_HEIGHT (ROTATE_TILE_HEIGHT*8)
#define ROTATE_MASK_WIDTH   (ROTATE_PIXEL_WIDTH-1)
#define ROTATE_MASK_HEIGHT  (ROTATE_PIXEL_HEIGHT-1)
#define get_gfx_pointer(gfxelement,c,line) (gfxelement->gfxdata + (c*gfxelement->height+line) * gfxelement->line_modulo)

#ifdef NAMCOS2_USE_TILEMGR

struct tilemap *namcos2_tilemap0=NULL;
struct tilemap *namcos2_tilemap1=NULL;
struct tilemap *namcos2_tilemap2=NULL;
struct tilemap *namcos2_tilemap3=NULL;
struct tilemap *namcos2_tilemap4=NULL;
struct tilemap *namcos2_tilemap5=NULL;

int namcos2_tilemap0_flip=0;
int namcos2_tilemap1_flip=0;
int namcos2_tilemap2_flip=0;
int namcos2_tilemap3_flip=0;
int namcos2_tilemap4_flip=0;
int namcos2_tilemap5_flip=0;

static void namcos2_tilemap0_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap0_flip&TILEMAP_FLIPX) col=63-col;
	if(namcos2_tilemap0_flip&TILEMAP_FLIPY) row=63-row;
	tile=READ_WORD(&videoram[0x0000+(((row<<6)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x30)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap0_flip;
}
static void namcos2_tilemap1_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap1_flip&TILEMAP_FLIPX) col=63-col;
	if(namcos2_tilemap1_flip&TILEMAP_FLIPY) row=63-row;
	tile=READ_WORD(&videoram[0x2000+(((row<<6)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x32)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap1_flip;
}
static void namcos2_tilemap2_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap2_flip&TILEMAP_FLIPX) col=63-col;
	if(namcos2_tilemap2_flip&TILEMAP_FLIPY) row=63-row;
	tile=READ_WORD(&videoram[0x4000+(((row<<6)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x34)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap2_flip;
}
static void namcos2_tilemap3_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap3_flip&TILEMAP_FLIPX) col=63-col;
	if(namcos2_tilemap3_flip&TILEMAP_FLIPY) row=63-row;
	tile=READ_WORD(&videoram[0x6000+(((row<<6)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x36)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap3_flip;
}
static void namcos2_tilemap4_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap4_flip&TILEMAP_FLIPX) col=35-col;
	if(namcos2_tilemap4_flip&TILEMAP_FLIPY) row=27-row;
	tile=READ_WORD(&videoram[0x8010+(((row*36)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x38)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap4_flip;
}
static void namcos2_tilemap5_get_info(int col,int row)
{
	int tile,colour;
	if(namcos2_tilemap5_flip&TILEMAP_FLIPX) col=35-col;
	if(namcos2_tilemap5_flip&TILEMAP_FLIPY) row=27-row;
	tile=READ_WORD(&videoram[0x8810+(((row*36)+col)<<1)])&0xffff;
	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = (Machine->memory_region[MEM_GFX_MASK]+(0x08*tile));
	/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
	tile=(tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
	colour=namcos2_68k_vram_ctrl_r(0x3a)&0x0007;

	SET_TILE_INFO(GFX_CHR,tile,colour)
	tile_info.flags=namcos2_tilemap5_flip;
}

void namcos2_display_warnings(void)
{
	static int framecount=60*15;

	if(framecount)
	{
		if(Machine->scrbitmap->depth==16)
		{
			static char message1[]="************************************";
			static char message2[]=" This game does not work correctly  ";
			static char message3[]=" in 16bit mode, use 256 colour mode ";
			static char message4[]="************************************";
			int	ypos=28;
			int xpos=36;
			if (Machine->orientation & ORIENTATION_SWAP_XY) { int tmp=xpos; xpos=ypos; ypos=tmp; }
			ypos=((ypos/2)-2)*8;
			xpos=((xpos/2)*8)-((strlen(message1)/2)*6);
			ui_text(message1,xpos,ypos);
			ui_text(message2,xpos,ypos+8);
			ui_text(message3,xpos,ypos+16);
			ui_text(message4,xpos,ypos+24);
		}
		framecount--;
	}
}

#endif

#ifdef NAMCOS2_DEBUG

int namcos2_calc_used_pens(int gfx_zone,int tile,char *penused)
{
	unsigned char* gfxdata=NULL;
	int pix_y=0,pix_x=0;
	int height=0,width=0;
	int pencount=0;

	height=Machine->gfx[gfx_zone]->height;
	width=Machine->gfx[gfx_zone]->width;

	for(pix_y=0;pix_y<height;pix_y++)
	{
		gfxdata=get_gfx_pointer(Machine->gfx[gfx_zone],tile,pix_y);

		for(pix_x=0;pix_x<width;pix_x++)
		{
			penused[(gfxdata[pix_x])>>3]|=1<<(gfxdata[pix_x]&0x07);
		}
	}
	for(pix_y=0;pix_y<256;pix_y++) if((penused[pix_y>>3])&(1<<(pix_y&0x07))) pencount++;
	return pencount;
}

#endif


int namcos2_vh_start(void)
{

#ifdef NAMCOS2_USE_TILEMGR

	/* Initialise the tilemaps */
	if((namcos2_tilemap0=tilemap_create(namcos2_tilemap0_get_info,TILEMAP_BITMASK,8,8,64,64))==NULL) return 0;
	if((namcos2_tilemap1=tilemap_create(namcos2_tilemap1_get_info,TILEMAP_BITMASK,8,8,64,64))==NULL) return 0;
	if((namcos2_tilemap2=tilemap_create(namcos2_tilemap2_get_info,TILEMAP_BITMASK,8,8,64,64))==NULL) return 0;
	if((namcos2_tilemap3=tilemap_create(namcos2_tilemap3_get_info,TILEMAP_BITMASK,8,8,64,64))==NULL) return 0;
	if((namcos2_tilemap4=tilemap_create(namcos2_tilemap4_get_info,TILEMAP_BITMASK,8,8,36,28))==NULL) return 0;
	if((namcos2_tilemap5=tilemap_create(namcos2_tilemap5_get_info,TILEMAP_BITMASK,8,8,36,28))==NULL) return 0;

	/* Setup fixed planes */
	tilemap_set_scrollx( namcos2_tilemap4, 0, 0 );
	tilemap_set_scrolly( namcos2_tilemap4, 0, 0 );
	tilemap_set_scrollx( namcos2_tilemap5, 0, 0 );
	tilemap_set_scrolly( namcos2_tilemap5, 0, 0 );

	namcos2_tilemap0_flip=0;
	namcos2_tilemap1_flip=0;
	namcos2_tilemap2_flip=0;
	namcos2_tilemap3_flip=0;
	namcos2_tilemap4_flip=0;
	namcos2_tilemap5_flip=0;

	tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	/* Rotate/Flip the mask ROM */

#ifndef PREROTATE_GFX

//
//  TILEMAP MANAGER SEEMS TO BE ABLE TO COPE OK WITH X/Y FLIPS BUT NOT SWAPXY
//
//	if (Machine->orientation & ORIENTATION_FLIP_Y)
//	{
//		int loopY,tilenum;
//		unsigned char tilecache[8],*tiledata;
//		for(tilenum=0;tilenum<0x10000;tilenum++)
//		{
//			tiledata=Machine->memory_region[MEM_GFX_MASK]+(tilenum*0x08);
//			/* Cache tile data */
//			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
//			/* Flip in Y - write back in reverse */
//			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=tilecache[7-loopY];
//		}
//	}

//	if (Machine->orientation & ORIENTATION_FLIP_X)
//	{
//		int loopX,loopY,tilenum;
//		unsigned char tilecache[8],*tiledata;
//		for(tilenum=0;tilenum<0x10000;tilenum++)
//		{
//			tiledata=Machine->memory_region[MEM_GFX_MASK]+(tilenum*0x08);
//			/* Cache tile data */
//			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
//			/* Wipe source data */
//			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
//			/* Flip in X - do bit reversal */
//			for(loopY=0;loopY<8;loopY++)
//			{
//				for(loopX=0;loopX<8;loopX++)
//				{
//					tiledata[loopY]|=(tilecache[loopY]&(1<<loopX))?(0x80>>loopX):0x00;
//				}
//			}
//		}
//	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int loopX,loopY,tilenum;
		unsigned char tilecache[8],*tiledata;

		for(tilenum=0;tilenum<0x10000;tilenum++)
		{
			tiledata=Machine->memory_region[MEM_GFX_MASK]+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Swap X/Y data */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopX]|=(tilecache[loopY]&(0x01<<loopX))?(1<<loopY):0x00;
				}
			}
		}

		/* For some reason the tilemap mgr NEEDS these additional steps */
		/* I've no idea why but it seems to work                        */

		for(tilenum=0;tilenum<0x10000;tilenum++)
		{
			tiledata=Machine->memory_region[MEM_GFX_MASK]+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Flip in Y - write back in reverse */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=tilecache[7-loopY];
		}

		for(tilenum=0;tilenum<0x10000;tilenum++)
		{
			tiledata=Machine->memory_region[MEM_GFX_MASK]+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Flip in X - do bit reversal */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopY]|=(tilecache[loopY]&(1<<loopX))?(0x80>>loopX):0x00;
				}
			}
		}
	}

#endif
#endif

	return 0;
}


void namcos2_vh_stop(void)
{
	tmpbitmap = 0;
}


void namcos2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	/* Zap the palette to Zero */

	for( i = 0; i < Machine->drv->total_colors; i++ )
	{
		palette[i*3+0] = 0;
		palette[i*3+1] = 0;
		palette[i*3+2] = 0;
	}
}


#ifdef NAMCOS2_DEBUG_MODE

#define TITLE_X         (6*8)
#define RGDISP_X        (5*6)
#define RGDISP_Y        (3*8)
#define RGDISP_Y_SEP    8

static void show_reg(int regbank )
{
	int i;
	char buffer[256];
	int ypos=RGDISP_Y;
	int xpos=RGDISP_X;

	switch (regbank)
	{
		case 1:
			sprintf(buffer,"VRAM Control: %08lx",0x00420000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x10; i+=2 )
			{
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[i]));
				ui_text(buffer,1*RGDISP_X,ypos);

				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x10+i]));
				ui_text(buffer,2*RGDISP_X,ypos);

				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x20+i]));
				ui_text(buffer,3*RGDISP_X,ypos);

				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x30+i]));
				ui_text(buffer,4*RGDISP_X,ypos);

				ypos+=RGDISP_Y_SEP;
			}

			ypos+=RGDISP_Y_SEP;
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"Sprite Bank : %08lx",0x00c40000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"%04x",namcos2_68k_sprite_bank_r(0));
			ui_text(buffer,RGDISP_X,ypos);

			ypos+=RGDISP_Y_SEP;
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"Palette Control: %08lx",0x00443000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x08; i+=2 )
			{
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_palette_ram[0x3000 + i]));
				ui_text(buffer,1*RGDISP_X,ypos);
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_palette_ram[0x3008 + i]));
				ui_text(buffer,2*RGDISP_X,ypos);
				ypos+=RGDISP_Y_SEP;
			}

			break;
		case 2:
			ypos+=RGDISP_Y_SEP;
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"ROZ Control: %08lx",0x00cc0000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x08; i+=2 )
			{
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_roz_ctrl[i]));
				ui_text(buffer,1*RGDISP_X,ypos);
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_roz_ctrl[0x08+i]));
				ui_text(buffer,2*RGDISP_X,ypos);
				ypos+=RGDISP_Y_SEP;
			}
			ypos+=RGDISP_Y_SEP;
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"Sprite Bank : %08lx",0x00c40000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"%04x",namcos2_68k_sprite_bank_r(0));
			ui_text(buffer,RGDISP_X,ypos);
			break;
		case 3:
			sprintf(buffer,"Protection Key: %08lx",0x00d00000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x10; i+=2 )
			{
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_key[i]));
				ui_text(buffer,RGDISP_X,ypos);
				ypos+=RGDISP_Y_SEP;
			}
			break;
		case 4:
			sprintf(buffer,"IRQ Control: %08lx",0x001c0000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x20; i++ )
			{
				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_master_C148[i]));
				ui_text(buffer,RGDISP_X,ypos);

				sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_slave_C148[i]));
				ui_text(buffer,2*RGDISP_X,ypos);
				ypos+=RGDISP_Y_SEP;
			}
			break;
		case 5:
			sprintf(buffer,"Sprite RAM: %08lx",0x00c00000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"SZ S -Y-  F F B NUM Q  -X-  SZ C P");
			ui_text(buffer,12,ypos);
			ypos+=RGDISP_Y_SEP;
			for( i=0; i<(0x10*0x08); i+=0x08 )
			{
				xpos=12;

				sprintf(buffer,"%02x %01x %03x  %01x %01x %01x %03x %01x  %03x  %02x %01x %01x",
						 ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x0a)&0x003f,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x09)&0x0001,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x00)&0x01ff,

						 ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0f)&0x0001,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0e)&0x0001,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0d)&0x0001,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x02)&0x07ff,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x00)&0x0003,

						 ((READ_WORD(&namcos2_sprite_ram[i+0x04]))>>0x00)&0x03ff,

						 ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x0a)&0x003f,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x04)&0x000f,
						 ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x00)&0x0007);
				ui_text(buffer,xpos,ypos);
				ypos+=RGDISP_Y_SEP;
			}
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"Sprite Bank : %08lx",0x00c40000L);
			ui_text(buffer,TITLE_X,ypos);
			ypos+=RGDISP_Y_SEP;
			sprintf(buffer,"%04x",namcos2_68k_sprite_bank_r(0));
			ui_text(buffer,RGDISP_X,ypos);
			break;
		default:
			break;
	}
}
#endif

INLINE unsigned char fetch_rotated_pixel(int xind, int yind)
{
	unsigned char* gfxdata;
	int ram_offset,pix_x,pix_y,tile;

	/* First reduce the x/y to tile & x/y subpixels */
	ram_offset=(((yind>>3)<<8)+(xind>>3))<<1;
	pix_x=xind&0x07;
	pix_y=yind&0x07;

#ifndef PREROTATE_GFX
	if(Machine->orientation & ORIENTATION_SWAP_XY) { int tmp=pix_x; pix_x=pix_y; pix_y=tmp; }
#endif

	/* Now fetch the tile number from ROZ RAM */
	tile=READ_WORD(&namcos2_68k_roz_ram[ram_offset]);

	/* Now extract the pixel from the tile gfx */
	gfxdata=get_gfx_pointer(Machine->gfx[GFX_ROZ],tile,pix_y);
	return gfxdata[pix_x];
}


static void draw_layerROZ( struct osd_bitmap *dest_bitmap)
{
	int dest_x,dest_x_delta,dest_x_start,dest_x_end,tmp_x;
	int dest_y,dest_y_delta,dest_y_start,dest_y_end,tmp_y;
	int right_dx,right_dy,down_dx,down_dy,start_x,start_y;

	/* These need to be sign extended for arithmetic useage */
	right_dx = namcos2_68k_roz_ctrl_r(0x06);
	right_dy = namcos2_68k_roz_ctrl_r(0x02);
	down_dx  = namcos2_68k_roz_ctrl_r(0x04);
	down_dy  = namcos2_68k_roz_ctrl_r(0x00);
	start_y  = namcos2_68k_roz_ctrl_r(0x0a);
	start_x  = namcos2_68k_roz_ctrl_r(0x08);

	/* Sign extend the deltas */
	if(right_dx&0x8000) right_dx|=0xffff0000;
	if(right_dy&0x8000) right_dy|=0xffff0000;
	if(down_dx &0x8000) down_dx |=0xffff0000;
	if(down_dy &0x8000) down_dy |=0xffff0000;

	/* Correct to 16 bit fixed point from 8/12 bit */
	right_dx <<=8;
	right_dy <<=8;
	down_dx  <<=8;
	down_dy  <<=8;
	start_x  <<=12;
	start_y  <<=12;

	/* Correction factor is needed for the screen offset */
	start_x+=38*right_dx;
	start_y+=38*right_dy;

	/* Select correct drawing code based on destination bitmap pixel depth */

	if(dest_bitmap->depth == 16)
	{
		unsigned short *dest_line,pixel;

		if(Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int tmp;
			tmp=right_dx; right_dx=down_dx; down_dx=tmp;
			tmp=right_dy; right_dy=down_dy; down_dy=tmp;
		}

		dest_y_delta=(Machine->orientation & ORIENTATION_FLIP_Y)?-1:1;
		dest_y_start=(Machine->orientation & ORIENTATION_FLIP_Y)?dest_bitmap->height-1:0;
		dest_y_end  =(Machine->orientation & ORIENTATION_FLIP_Y)?-1:dest_bitmap->height;

		dest_x_delta=(Machine->orientation & ORIENTATION_FLIP_X)?-1:1;
		dest_x_start=(Machine->orientation & ORIENTATION_FLIP_X)?dest_bitmap->width-1:0;
		dest_x_end  =(Machine->orientation & ORIENTATION_FLIP_X)?-1:dest_bitmap->width;

		for( dest_y=dest_y_start; dest_y!=dest_y_end; dest_y+=dest_y_delta )
		{
			dest_line = (unsigned short*)dest_bitmap->line[dest_y];
			tmp_x = start_x;
			tmp_y = start_y;

			for( dest_x=dest_x_start; dest_x!=dest_x_end; dest_x+=dest_x_delta )
			{
				int xind=(tmp_x>>16)&ROTATE_MASK_HEIGHT;
				int yind=(tmp_y>>16)&ROTATE_MASK_WIDTH;

				pixel= fetch_rotated_pixel(xind,yind);

				/* Only process non-transparent pixels */
				if(pixel!=0xff)
				{
					const struct GfxElement *rozgfx=Machine->gfx[GFX_ROZ];
					/* Now remap the colour space of the pixel */
					int colour=(namcos2_68k_sprite_bank_r(0)>>8)&0x000f;
					unsigned short *paldata = &rozgfx->colortable[rozgfx->color_granularity * colour];
					/* Now finally store the remapped pixel */
					dest_line[dest_x]=paldata[pixel];
				}
				tmp_x += right_dx;
				tmp_y += right_dy;
			}
			start_x += down_dx;
			start_y += down_dy;
		}
	}
	else
	{
		unsigned char *dest_line,pixel;

		if(Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int tmp;
			tmp=right_dx; right_dx=down_dx; down_dx=tmp;
			tmp=right_dy; right_dy=down_dy; down_dy=tmp;
		}

		dest_y_delta=(Machine->orientation & ORIENTATION_FLIP_Y)?-1:1;
		dest_y_start=(Machine->orientation & ORIENTATION_FLIP_Y)?dest_bitmap->height-1:0;
		dest_y_end  =(Machine->orientation & ORIENTATION_FLIP_Y)?-1:dest_bitmap->height;

		dest_x_delta=(Machine->orientation & ORIENTATION_FLIP_X)?-1:1;
		dest_x_start=(Machine->orientation & ORIENTATION_FLIP_X)?dest_bitmap->width-1:0;
		dest_x_end  =(Machine->orientation & ORIENTATION_FLIP_X)?-1:dest_bitmap->width;

		for( dest_y=dest_y_start; dest_y!=dest_y_end; dest_y+=dest_y_delta )
		{
			dest_line = dest_bitmap->line[dest_y];
			tmp_x = start_x;
			tmp_y = start_y;

			for( dest_x=dest_x_start; dest_x!=dest_x_end; dest_x+=dest_x_delta )
			{
				int xind=(tmp_x>>16)&ROTATE_MASK_HEIGHT;
				int yind=(tmp_y>>16)&ROTATE_MASK_WIDTH;

				pixel= fetch_rotated_pixel(xind,yind);

				/* Only process non-transparent pixels */
				if(pixel!=0xff)
				{
					const struct GfxElement *rozgfx=Machine->gfx[GFX_ROZ];
					/* Now remap the colour space of the pixel */
					int colour=(namcos2_68k_sprite_bank_r(0)>>8)&0x000f;
					unsigned short *paldata = &rozgfx->colortable[rozgfx->color_granularity * colour];
					/* Now finally store the remapped pixel */
					dest_line[dest_x]=paldata[pixel];
				}
				tmp_x += right_dx;
				tmp_y += right_dy;
			}
			start_x += down_dx;
			start_y += down_dy;
		}
	}
}


#ifndef NAMCOS2_USE_TILEMGR

static void draw_layer64( struct osd_bitmap *bitmap, int mem_offset, int xoff, int yoff, int flipx, int flipy, int colour )
{
	int xloop,yloop,addr,tile,tmp;

	/* BEWARE - FUDGE FACTORS IN OPERATION */
	xoff+=44;
	yoff+=24;

	for(yloop=0;yloop<64;yloop++)
	{
		for(xloop=0;xloop<64;xloop++)
		{
			addr=(((yloop<<6) + xloop))<<1;
			tile=(READ_WORD(&videoram[mem_offset+addr]))&0xffff;
			/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
			tmp=((tile&0xc000)>>3) | ((tile&0x3800)<<2);
			tile=(tile&0x07ff)|tmp;

			drawgfx(bitmap,Machine->gfx[GFX_CHR],
				tile,
				colour,
				flipx,flipy,
				((((flipx)?(63-xloop):xloop)<<3)-xoff)&0x1ff,
				((((flipy)?(63-yloop):yloop)<<3)-yoff)&0x1ff,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
		}
	}
}

static void draw_layer36( struct osd_bitmap *bitmap, int mem_offset, int flipx, int flipy, int colour )
{
	int xloop,yloop,addr,tile,tmp,linetot;

	for(yloop=0;yloop<28;yloop++)
	{
		linetot=yloop*36;
		for(xloop=0;xloop<36;xloop++)
		{
			addr=(linetot+xloop)<<1;
			tile=(READ_WORD(&videoram[mem_offset+addr]))&0xffff;
			/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
			tmp=((tile&0xc000)>>3) | ((tile&0x3800)<<2);
			tile=(tile&0x07ff)|tmp;

			drawgfx(bitmap,Machine->gfx[GFX_CHR],
				tile,
				colour,
				flipx,flipy,
				(flipx)?((35-xloop)<<3):(xloop<<3),(flipy)?((27-yloop)<<3):(yloop<<3),
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
		}
	}
}

#endif

static void draw_sprites( struct osd_bitmap *bitmap, int priority )
{
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;
	struct rectangle rect;

	offset=(namcos2_68k_sprite_bank_r(0)&0x000f)*(128*8);

	for(loop=0;loop < 128;loop++)
	{
		/****************************************
		* Sprite data is 8 byte packed format   *
		*                                       *
		* Offset 0,1                            *
		*   Sprite Y position           D00-D08 *
		*   Sprite Size 16/32           D09     *
		*   Sprite Size Y               D10-D15 *
		*                                       *
		* Offset 2,3                            *
		*   Sprite Quadrant             D00-D01 *
		*   Sprite Number               D02-D12 *
		*   Sprite ROM Bank select      D13     *
		*   Sprite flip X               D14     *
		*   Sprite flip Y               D15     *
		*                                       *
		* Offset 4,5                            *
		*   Sprite X position           D00-D10 *
		*                                       *
		* Offset 6,7                            *
		*   Sprite priority             D00-D02 *
		*   Sprite colour index         D04-D07 *
		*   Sprite Size X               D10-D15 *
		*                                       *
		****************************************/

		offset0 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+0]);
		offset2 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+2]);
		offset4 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+4]);
		offset6 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+6]);

		/* Fetch sprite size registers */

		sizey=((offset0>>10)&0x3f)+1;
		sizex=(offset6>>10)&0x3f;

		if((offset0&0x0200)==0) sizex>>=1;

		if((sizey-1) && sizex && (offset6&0x0007)==priority)
		{
			rect=Machine->drv->visible_area;

			sprn=(offset2>>2)&0x7ff;
			spr_region=(offset2&0x2000)?GFX_OBJ2:GFX_OBJ1;

			ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
			xpos=(offset4&0x07ff)-0x50+0x07;

			flipy=offset2&0x8000;
			flipx=offset2&0x4000;

			scalex=((sizex<<16)/((offset0&0x0200)?0x20:0x10));
			scaley=((sizey<<16)/((offset0&0x0200)?0x20:0x10));

			/* Set the clipping rect to mask off the other portion of the sprite */
			rect.min_x=xpos;
			rect.max_x=xpos+(sizex-1);
			rect.min_y=ypos;
			rect.max_y=ypos+(sizey-1);

			if((offset0&0x0200)==0)
			{
				if(((offset2&0x0001) && !flipx) || (!(offset2&0x0001) && flipx)) xpos-=sizex;
				if(((offset2&0x0002) && !flipy) || (!(offset2&0x0002) && flipy)) ypos-=sizey;
			}

			if((scalex==(1<<16)) && (scaley==(1<<16)))
			{
				drawgfx(bitmap,Machine->gfx[spr_region],
					sprn,
					(offset6>>4)&0x000f,     /* Selected colour bank */
					flipx,flipy,
					xpos,ypos,
					&rect,TRANSPARENCY_PEN,0xff);
			}
			else if(scalex && scaley)
			{
				drawgfxzoom(bitmap,Machine->gfx[spr_region],
					sprn,
					(offset6>>4)&0x000f,     /* Selected colour bank */
					flipx,flipy,
					xpos,ypos,
					&rect,TRANSPARENCY_PEN,0xff,
					scalex,scaley);
			}
		}
	}
}


void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh)
{
	int priority,loop,offset,count;
	int colour_codes_used[NAMCOS2_COLOUR_CODES];
	static int show[10] = {1,1,1,1,1,1,1,1,1,1};

#ifdef NAMCOS2_USE_TILEMGR
	tilemap_update(namcos2_tilemap0);
	tilemap_update(namcos2_tilemap1);
	tilemap_update(namcos2_tilemap2);
	tilemap_update(namcos2_tilemap3);
	tilemap_update(namcos2_tilemap4);
	tilemap_update(namcos2_tilemap5);
#endif
	palette_init_used_colors();

	for(loop=0;loop<NAMCOS2_COLOUR_CODES;loop++) colour_codes_used[loop]=0;

	/* Mark off all of the colour codes used by the sprites */
	offset=(namcos2_68k_sprite_bank_r(0)&0x000f)*(128*8);
	for(loop=0;loop<128;loop++)
	{
		int sizey=(READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+0])>>10)&0x3f;
		if(sizey) colour_codes_used[(READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+6])>>4)&0x000f]=1;
	}
	/* Mark off the colour codes used by scroll & ROZ planes */
#ifndef NAMCOS2_USE_TILEMGR
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x30)&0x0007)]=1;
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x32)&0x0007)]=1;
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x34)&0x0007)]=1;
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x36)&0x0007)]=1;
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x38)&0x0007)]=1;
	colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x3a)&0x0007)]=1;
#endif
	colour_codes_used[(namcos2_68k_sprite_bank_r(0)>>8)&0x000f]=1;

	/* Mark colour indexes used by text planes/roz/sprites */
	for(loop=0;loop<NAMCOS2_COLOUR_CODES;loop++)
	{
		if(colour_codes_used[loop])
		{
			for(count=0;count<256;count++) palette_used_colors[(loop*256)+count] |= PALETTE_COLOR_VISIBLE;
		}
	}


#ifdef NAMCOS2_USE_TILEMGR
	if (palette_recalc())
	{
		/* Mark all planes as dirty */
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	}
	tilemap_render(ALL_TILEMAPS);
#else
	palette_recalc();
#endif

	/* Scrub the bitmap clean */
	fillbitmap(bitmap,0,0);

	/* Render the screen */
	for(priority=0;priority<=7;priority++)
	{
#ifdef NAMCOS2_USE_TILEMGR

		if((namcos2_68k_vram_ctrl_r(0x20)&0x07)==priority && show[0]) tilemap_draw(bitmap,namcos2_tilemap0,0);
		if((namcos2_68k_vram_ctrl_r(0x22)&0x07)==priority && show[1]) tilemap_draw(bitmap,namcos2_tilemap1,0);
		if((namcos2_68k_vram_ctrl_r(0x24)&0x07)==priority && show[2]) tilemap_draw(bitmap,namcos2_tilemap2,0);
		if((namcos2_68k_vram_ctrl_r(0x26)&0x07)==priority && show[3]) tilemap_draw(bitmap,namcos2_tilemap3,0);
		if((namcos2_68k_vram_ctrl_r(0x28)&0x07)==priority && show[4]) tilemap_draw(bitmap,namcos2_tilemap4,0);
		if((namcos2_68k_vram_ctrl_r(0x2a)&0x07)==priority && show[5]) tilemap_draw(bitmap,namcos2_tilemap5,0);

#else

/*1*/	if((namcos2_68k_vram_ctrl_r(0x20)&0x07)==priority && show[0])
			draw_layer64( bitmap, 0x0000,
						  (namcos2_68k_vram_ctrl_r(0x02)&0x0fff)+4,namcos2_68k_vram_ctrl_r(0x06)&0x0fff,
						  namcos2_68k_vram_ctrl_r(0x02)&0x8000,namcos2_68k_vram_ctrl_r(0x06)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x30)&0x0007 );

/*2*/	if((namcos2_68k_vram_ctrl_r(0x22)&0x07)==priority && show[1])
			draw_layer64( bitmap, 0x2000,
						  (namcos2_68k_vram_ctrl_r(0x0a)&0x0fff)+2, namcos2_68k_vram_ctrl_r(0x0e)&0x0fff,
						  namcos2_68k_vram_ctrl_r(0x0a)&0x8000,namcos2_68k_vram_ctrl_r(0x0e)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x32)&0x0007 );

/*3*/	if((namcos2_68k_vram_ctrl_r(0x24)&0x07)==priority && show[2])
			draw_layer64( bitmap, 0x4000,
						  (namcos2_68k_vram_ctrl_r(0x12)&0x0fff)+1,namcos2_68k_vram_ctrl_r(0x16)&0x0fff,
						  namcos2_68k_vram_ctrl_r(0x12)&0x8000,namcos2_68k_vram_ctrl_r(0x16)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x34)&0x0007 );

/*4*/	if((namcos2_68k_vram_ctrl_r(0x26)&0x07)==priority && show[3])
			draw_layer64( bitmap, 0x6000,
						  (namcos2_68k_vram_ctrl_r(0x1a)&0x0fff)+0, namcos2_68k_vram_ctrl_r(0x1e)&0x0fff,
						  namcos2_68k_vram_ctrl_r(0x1a)&0x8000,namcos2_68k_vram_ctrl_r(0x1e)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x36)&0x0007 );

/*5*/	if((namcos2_68k_vram_ctrl_r(0x28)&0x07)==priority && show[4])
			draw_layer36( bitmap, 0x8010,
						  namcos2_68k_vram_ctrl_r(0x02)&0x8000,namcos2_68k_vram_ctrl_r(0x06)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x38)&0x0007 );

/*6*/	if((namcos2_68k_vram_ctrl_r(0x2a)&0x07)==priority && show[5])
			draw_layer36( bitmap, 0x8810,
						  namcos2_68k_vram_ctrl_r(0x02)&0x8000, namcos2_68k_vram_ctrl_r(0x06)&0x8000,
						  namcos2_68k_vram_ctrl_r(0x3a)&0x0007 );

#endif

		/* Draw ROZ if enabled */
		if(priority>=1 && ((namcos2_68k_sprite_bank_r(0)>>12)&0x07)==priority && show[6]) draw_layerROZ( bitmap);

		/* Sprites */
		draw_sprites( bitmap,priority );
	}

#ifdef NAMCOS2_USE_TILEMGR
	namcos2_display_warnings();
#endif

#ifdef NAMCOS2_DEBUG_MODE
	/* NAMCOS2 Video debugging */
	if(keyboard_pressed(KEYCODE_Z))     { while( keyboard_pressed(KEYCODE_Z));     show[0]=(show[0])?0:1;  }
	if(keyboard_pressed(KEYCODE_X))     { while( keyboard_pressed(KEYCODE_X));     show[1]=(show[1])?0:1;  }
	if(keyboard_pressed(KEYCODE_C))     { while( keyboard_pressed(KEYCODE_C));     show[2]=(show[2])?0:1;  }
	if(keyboard_pressed(KEYCODE_V))     { while( keyboard_pressed(KEYCODE_V));     show[3]=(show[3])?0:1;  }
	if(keyboard_pressed(KEYCODE_B))     { while( keyboard_pressed(KEYCODE_B));     show[4]=(show[4])?0:1;  }
	if(keyboard_pressed(KEYCODE_N))     { while( keyboard_pressed(KEYCODE_N));     show[5]=(show[5])?0:1;  }
	if(keyboard_pressed(KEYCODE_COMMA)) { while( keyboard_pressed(KEYCODE_COMMA)); show[6]=(show[6])?0:1;  }

	if(!show[0] || !show[1] || !show[2] || !show[3] || !show[4] || !show[5] || !show[6])
	{
		char buffer[256];
		sprintf(buffer,"Planes %d%d%d%d %d%d %d",show[0],show[1],show[2],show[3],show[4],show[5],show[6]);
		ui_text(buffer,4,4);
	}

	{
		static int regshow = 0;
		if(keyboard_pressed(KEYCODE_L)) { while( keyboard_pressed(KEYCODE_L)); regshow++;if(regshow>5) regshow=0; }
		if(regshow) show_reg(regshow);
	}
#endif
}
