/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"

#define ROTATE_TILE_WIDTH	256
#define ROTATE_TILE_HEIGHT	256
#define ROTATE_PIXEL_WIDTH	(ROTATE_TILE_WIDTH*8)
#define ROTATE_PIXEL_HEIGHT	(ROTATE_TILE_HEIGHT*8)

struct osd_bitmap *rotate_bitmap;

int namcos2_vh_start(void)
{
	if ((rotate_bitmap = osd_new_bitmap(ROTATE_PIXEL_WIDTH,ROTATE_PIXEL_HEIGHT,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}
	return 0;
}

void namcos2_vh_stop(void)
{
	osd_free_bitmap(rotate_bitmap);
	tmpbitmap = 0;
}

#define RGDISP_CHARSEP	6

#define TITLE_X		    (6*8)
#define TITLE_Y		    (1*8)

#define RGDISP_X		(5*8)
#define RGDISP_Y		(3*8)
#define RGDISP_Y_SEP	8
#define RGDISP_X_SEP	38


static void show_reg( struct osd_bitmap *bitmap, int regbank )
{
	int i,n;
	char buffer[256];

	switch (regbank)
	{
		case 1:
            sprintf(buffer,"VRAM Control: %08x",0x00420000);
            ui_text(buffer,TITLE_X,TITLE_Y);
			for( i=0; i<0x40; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[i]));
                ui_text(buffer,RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*(i/2));
			}
			break;
		case 2:
            sprintf(buffer,"ROZ Control: %08x",0x00cc0000);
            ui_text(buffer,TITLE_X,TITLE_Y);
			for( i=0; i<0x10; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_roz_ctrl[i]));
                ui_text(buffer,RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*(i/2));
			}
			break;
		case 3:
            sprintf(buffer,"Protection Key: %08x",0x00d00000);
            ui_text(buffer,TITLE_X,TITLE_Y);
			for( i=0; i<0x10; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_key[i]));
                ui_text(buffer,RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*(i/2));
			}
			break;
		case 4:
            sprintf(buffer,"IRQ Control: %08x",0x001c0000);
            ui_text(buffer,TITLE_X,TITLE_Y);
			for( i=0; i<0x20; i++ )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_master_C148[i]));
                ui_text(buffer,RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*i);

                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_slave_C148[i]));
                ui_text(buffer,2*RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*i);
			}
			break;
		case 5:
            sprintf(buffer,"Sprite RAM: %08x",0x00c00000);
            ui_text(buffer,TITLE_X,TITLE_Y);
			for( i=0; i<0x20; i++ )
			{
				for(n=0;n<4;n++)
				{
                    sprintf(buffer,"%04x",READ_WORD(&namcos2_sprite_ram[(i*8)+(n*2)]));
                    ui_text(buffer,(n+1)*RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*i);
				}
			}
			break;
		case 6:
            sprintf(buffer,"Unused reg slot");
            ui_text(buffer,TITLE_X,TITLE_Y);
//          sprintf(buffer,"%08lx",0x0050ccd0);
//          ui_text(buffer,TITLE_X,TITLE_Y);
//			for( i=0; i<0x20; i+=2 )
//			{
//              sprintf(buffer,"Unused");
//              ui_text(buffer,RGDISP_X,RGDISP_Y+RGDISP_Y_SEP*(i/2));
//			}
			break;
		default:
			break;
	}
#if 0
	{
		static FILE *f=NULL;
		int i;
		if (!f) f = fopen ("roz.log", "w");
		for (i = 0; i < 0x10; i+=2) fprintf (f, "%08lX ", READ_WORD(&namcos2_roz_ctrl[i]));
		fprintf (f, "\n");
	}
#endif
}


static void erase_bitmap(struct osd_bitmap *bitmap)
{
	int loop;
	for(loop=0; loop < 0x2000; loop +=2)
	{
		int tile=0x0020;
		drawgfx(bitmap,Machine->gfx[GFX_CHR],
			tile,
			DT_COLOR_WHITE,
			0,0,
			((loop/2)%64)*8,((loop/2)/64)*8,
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0x00);
	}
}

void rotatebitmap( struct osd_bitmap *dest_bitmap, struct osd_bitmap *source_bitmap,
	int tx0, int ty0, int dx_right, int dy_right, int dx_down, int dy_down )
{

	int sx, sy, tx, ty;
	unsigned char *dest;
//	int myfuck_x,myfuck_y;

//	myfuck_x=tx0>>16;
//	myfuck_y=ty0>>16;
//	if(myfuck_x<0) myfuck_x+=dest_bitmap->height;
//	if(myfuck_y<0) myfuck_y+=dest_bitmap->width;
	tx0=0;
	ty0=0;

	for( sy=0; sy<dest_bitmap->height; sy++ )
	{
		dest = dest_bitmap->line[sy];
//		dest = dest_bitmap->line[(sy+myfuck_y)%dest_bitmap->height];
		tx = tx0;
		ty = ty0;

		for( sx=0; sx<dest_bitmap->width; sx++ )
		{
//			dest[sx] = source_bitmap->line[2047-((ty>>16)&0x7ff)][2047-((tx>>16)&0x7ff)];
			dest[sx] = source_bitmap->line[(ty>>16)&0x7ff][(tx>>16)&0x7ff];

//			dest[(sx+myfuck_x)%dest_bitmap->width] = source_bitmap->line[2047-(ty>>16)&0x7ff][2047-(tx>>16)&0x7ff];

			tx += dx_right;
			ty += dy_right;
		}

		tx0 += dx_down;
		ty0 += dy_down;
	}
}


static void draw_layerROZ( struct osd_bitmap *bitmap, int offset )
{
	int loop;
	for(loop=0; loop < ROTATE_TILE_WIDTH*ROTATE_TILE_HEIGHT*2; loop +=2)
	{
		int tile = READ_WORD(&namcos2_roz_ram[offset+loop]);
		tile&=0x7fff;
		drawgfx(rotate_bitmap,Machine->gfx[GFX_ROZ],
				tile,
				DT_COLOR_WHITE,
				0,0,
				((loop/2)%ROTATE_TILE_WIDTH)*8,((loop/2)/ROTATE_TILE_HEIGHT)*8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0xff);
	}

//	rotatebitmap( bitmap,rotate_bitmap,	int tx0, int ty0, int dx_right, int dy_right, int dx_down, int dy_down );
//	rotatebitmap( bitmap,rotate_bitmap,	   0   ,    0   , 0.7*(1<<16) ,  0.7*(1<<16),-0.7*(1<<16), 0.7*(1<<16) );
//	{
//		static int x=0,y=0;
//		rotatebitmap( bitmap,rotate_bitmap,	   y<<16   ,    y<<16   , (1<<16) ,  0, 0, (1<<16) );
//		if(++y>2048) y=0;
//	}

	/* Rotate register mapping guesstimate  */
	/*                                      */
	/* 00 - DX_RIGHT                        */
	/* 02 - DX_DOWN                         */
	/* 04 - DY_RIGHT                        */
	/* 06 - DY_DOWN                         */
	/* 08 - X_START                         */
	/* 0A - Y_START                         */
	/*                                      */
	/* This fits with zoom on 00 & 06 only  */
	/* its a possibility that x & y are     */
	/* swapped.                             */

	{
		/* These need to be sign extended for arithmetic useage */
		int dx_right = namcos2_68k_roz_ctrl_r( 0x00 );
		int dx_down  = namcos2_68k_roz_ctrl_r( 0x02 );
		int dy_right = namcos2_68k_roz_ctrl_r( 0x04 );
		int dy_down  = namcos2_68k_roz_ctrl_r( 0x06 );
		int x_start  = namcos2_68k_roz_ctrl_r( 0x08 );
		int y_start  = namcos2_68k_roz_ctrl_r( 0x0a );

		if(dx_right&0x8000) dx_right|=0xffff0000;
		if(dx_down &0x8000) dx_down |=0xffff0000;
		if(dy_right&0x8000) dy_right|=0xffff0000;
		if(dy_down &0x8000) dy_down |=0xffff0000;
		if(x_start &0x8000) x_start |=0xffff0000;
		if(y_start &0x8000) y_start |=0xffff0000;

		/* Correct to 16 bit fixed point from 8 bit */
		dx_right <<=8;
		dx_down  <<=8;
		dy_right <<=8;
		dy_down  <<=8;
		x_start  <<=8;
		y_start  <<=8;
//		x_start=-x_start;
//		y_start=-y_start;

		rotatebitmap( bitmap, rotate_bitmap, x_start, y_start, dx_right, dy_right, dx_down, dy_down );
#if 0
		{
			static FILE *f=NULL;
			if (!f) f = fopen ("rot.log", "w");
			fprintf (f, "%8ld ", dx_right);
			fprintf (f, "%8ld ", dx_down );
			fprintf (f, "%8ld ", dy_right);
			fprintf (f, "%8ld ", dy_down );
			fprintf (f, "%8ld ", x_start );
			fprintf (f, "%8ld ", y_start );
			fprintf (f, "\n");
		}
#endif
	}
}

static void draw_layer64( struct osd_bitmap *bitmap, int mem_offset, int xoff, int yoff )
{
	int loop;
	for(loop=0; loop < 0x2000; loop +=2)
	{
		int tile = READ_WORD(&videoram[mem_offset+loop]);
		tile&=0x7fff;
		drawgfx(bitmap,Machine->gfx[GFX_CHR],
			tile,
			DT_COLOR_WHITE,
			0,0,
			(((loop/2)%64)*8-xoff)&0x1ff,(((loop/2)/64)*8-yoff)&0x1ff,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
	}
}

static void draw_layer36( struct osd_bitmap *bitmap, int offset )
{
	int loop;
	for(loop=0; loop < 0x7e0; loop +=2){
		int data = READ_WORD(&videoram[offset+loop]);
		drawgfx(bitmap,Machine->gfx[GFX_CHR],
			data,
			DT_COLOR_WHITE,
			0,0,
			((loop/2)%36)*8,(((loop/2)/36)+2)*8,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
	}
}

static void draw_sprites( struct osd_bitmap *bitmap )
{
	int offset,count,loop,spr_region;

	count=128;  // 128 Sprites per bank
	offset=namcos2_sprite_bank*(128*8);

	for(loop=0;loop <count;loop++)
	{
		int flipy,flipx,sprn;
		int code = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+2]);
		int ypos = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+0]);
		int xpos = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+4]);

		flipy=code&0x8000;
		flipx=code&0x4000;

		sprn=code>>2;
		spr_region=(sprn&0x0800)?GFX_OBJ2:GFX_OBJ1;
		sprn&=0x07ff;


		xpos&=0x01ff;
		xpos-=0x20;			// Offset correction

		ypos&=0x01ff;
		ypos=0x1ff-ypos;	// Axis flip
		ypos-=0x40;			// Offset correction

		if(sprn)
		{
			drawgfx(bitmap,Machine->gfx[spr_region],
				sprn,
				DT_COLOR_WHITE,
				flipx,flipy,
				xpos-32,ypos,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0xff);
		}
	}
}


void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh){
	static int show[8] = {1,1,1,0,0,1,1,1};
	static int rshow = 0;

	if(keyboard_pressed(KEYCODE_Z)) { while( keyboard_pressed(KEYCODE_Z)); show[0]=(show[0])?0:1; }
	if(keyboard_pressed(KEYCODE_X)) { while( keyboard_pressed(KEYCODE_X)); show[1]=(show[1])?0:1; }
	if(keyboard_pressed(KEYCODE_C)) { while( keyboard_pressed(KEYCODE_C)); show[2]=(show[2])?0:1; }
	if(keyboard_pressed(KEYCODE_V)) { while( keyboard_pressed(KEYCODE_V)); show[3]=(show[3])?0:1; }
	if(keyboard_pressed(KEYCODE_B)) { while( keyboard_pressed(KEYCODE_B)); show[4]=(show[4])?0:1; }
	if(keyboard_pressed(KEYCODE_N)) { while( keyboard_pressed(KEYCODE_N)); show[5]=(show[5])?0:1; }

	if(keyboard_pressed(KEYCODE_COMMA)) { while( keyboard_pressed(KEYCODE_COMMA)); show[6]=(show[6])?0:1; }
	if(keyboard_pressed(KEYCODE_STOP )) { while( keyboard_pressed(KEYCODE_STOP )); show[7]=(show[7])?0:1; }

//	erase_bitmap(bitmap);
	if(show[6]) draw_layerROZ( bitmap, 0);
//	if(show[7]) draw_layerROZ( bitmap, 0x20000);
	if(show[0]) draw_layer64( bitmap, 0x0000, namcos2_68k_vram_ctrl_r(0x02), namcos2_68k_vram_ctrl_r(0x06) );
	if(show[1]) draw_layer64( bitmap, 0x2000, namcos2_68k_vram_ctrl_r(0x0a), namcos2_68k_vram_ctrl_r(0x0e) );
	if(show[2]) draw_layer64( bitmap, 0x4000, namcos2_68k_vram_ctrl_r(0x12), namcos2_68k_vram_ctrl_r(0x16) );
	if(show[3]) draw_layer64( bitmap, 0x6000, namcos2_68k_vram_ctrl_r(0x1a), namcos2_68k_vram_ctrl_r(0x1e) );
	if(show[4]) draw_layer36( bitmap, 0x8010 );
	if(show[5]) draw_layer36( bitmap, 0x8810 );
	draw_sprites( bitmap );

// Save video memory

	if( keyboard_pressed(KEYCODE_S) ){
		FILE *f;
		while( keyboard_pressed(KEYCODE_S) ); /* wait for key release */

		f = fopen("vidmem.dbg","wb");
		if( f ){
			fwrite( videoram,0x10000,1,f );
			fclose( f );
		}

		f = fopen("ram3.dbg","wb");
		if( f ){
			fwrite( namcos2_roz_ram, 0x40000, 1, f );
			fclose( f );
		}
	}

	if(keyboard_pressed(KEYCODE_L)) { while( keyboard_pressed(KEYCODE_L)); rshow++;if(rshow>6) rshow=0; }
	if(rshow) show_reg(bitmap,rshow);
}
