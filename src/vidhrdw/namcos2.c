/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"


static void show_word(struct osd_bitmap *bitmap, int Xpos, int Ypos, int data)
{
	char digit[16] = "0123456789abcdef";
	int n;
	for( n=0; n<4; n++ )
	{
		drawgfx(bitmap,Machine->uifont,
			digit[(data>>(4*n))&0xf],
			2,
			0,0,Ypos,Xpos+(n*16),
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

static void show_byte(struct osd_bitmap *bitmap, int Xpos, int Ypos, int data)
{
	char digit[16] = "0123456789abcdef";
	int n;
	for( n=0; n<2; n++ )
	{
		drawgfx(bitmap,Machine->uifont,
			digit[(data>>(4*n))&0xf],
			2,
			0,0,Ypos,Xpos+(n*16),
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

static void show_reg( struct osd_bitmap *bitmap )
{
	int i;
	for( i=0; i<0x20; i+=2 )
	{
		show_word(bitmap,512-(8*8),i/2*16,READ_WORD(&namcos2_1c0000[i]));
	}

	for( i=0; i<0x40; i+=2 )
	{
		show_word(bitmap,512-(17*8),i/2*16,READ_WORD(&namcos2_420000[i]));
	}

	for( i=0; i<0x10; i+=2 )
	{
		show_word(bitmap,512-(26*8),i/2*16,READ_WORD(&namcos2_4a0000[i]));
	}

	show_word(bitmap,512-(26*8),0x10*16,READ_WORD(&namcos2_c40000[0]));
	show_word(bitmap,512-(26*8),0x12*16,READ_WORD(&namcos2_d00000[4]));
	show_word(bitmap,512-(26*8),0x14*16,READ_WORD(&namcos2_d00000[6]));

	for( i=0; i<0x10; i+=2 )
	{
		show_word(bitmap,512-(36*8),i/2*16,READ_WORD(&namcos2_roz_ctrl[i]));
	}

#if 0
	{
		static FILE *f=NULL;
		int i;
		if (!f) f = fopen ("roz.log", "w");
		for (i = 0; i < 0x10; i+=2) fprintf (f, "%04lX ", READ_WORD(&namcos2_roz_ctrl[i]));
		fprintf (f, "\n");
	}
#endif
}

static void show_sprites( struct osd_bitmap *bitmap )
{
	int i;
	for( i=0; i<0x20; i++ )
	{
		int n;
		for(n=0;n<4;n++)
		{
			int data;
			data=READ_WORD(&namcos2_sprite_ram[(i*8)+(n*2)]);
			show_word(bitmap,240+((3-n)*68),i*16,data);
		}
	}
}

int namcos2_vh_start(void)
{
	return 0;
}

void namcos2_vh_stop(void)
{
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

static void draw_layer64( struct osd_bitmap *bitmap, int offset )
{
	int loop;
	for(loop=0; loop < 0x2000; loop +=2)
	{
		int tile = READ_WORD(&videoram[offset+loop]);
		tile&=0x7fff;
		drawgfx(bitmap,Machine->gfx[GFX_CHR],
			tile,
			DT_COLOR_WHITE,
			0,0,
			((loop/2)%64)*8,((loop/2)/64)*8,
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
			((loop/2)%36)*8,((loop/2)/36)*8,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
	}
}

static void draw_sprites( struct osd_bitmap *bitmap )
{
	int offset,count,loop;

	count=0x80;
	offset=0;

	for(loop=0;loop <count;loop++)
	{
		int flipy,flipx,sprn;
		int code = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+2]);
		int ypos = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+0]);
		int xpos = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+4]);

		flipy=code&0x8000;
		flipx=code&0x4000;

		sprn=code>>2;
		sprn&=0x07ff;

		xpos&=0x01ff;
		xpos-=0x20;			// Offset correction

		ypos&=0x01ff;
		ypos=0x1ff-ypos;	// Axis flip
		ypos-=0x40;			// Offset correction

		if(sprn)
		{
			drawgfx(bitmap,Machine->gfx[GFX_OBJ],
				sprn,
				DT_COLOR_WHITE,
				flipx,flipy,
				xpos,ypos,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0xff);
		}
	}
}


static void draw_layerROZ( struct osd_bitmap *bitmap, int offset )
{
	int loop;
	for(loop=0; loop < 256*256*2; loop +=2)
	{
		int tile = READ_WORD(&namcos2_roz_ram[offset+loop]);
		tile&=0x1fff;
		if(tile) drawgfx(bitmap,Machine->gfx[GFX_ROZ],
						tile,
						DT_COLOR_WHITE,
						0,0,
						((loop/2)%256)*8,((loop/2)/256)*8,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0xff);
	}
}


void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh){
	static int show[8] = {1,1,1,0,0,1,1,1};
	static int rshow[2] = {0,0};

	if(osd_key_pressed(OSD_KEY_Z)) { while( osd_key_pressed(OSD_KEY_Z)); show[0]=(show[0])?0:1; }
	if(osd_key_pressed(OSD_KEY_X)) { while( osd_key_pressed(OSD_KEY_X)); show[1]=(show[1])?0:1; }
	if(osd_key_pressed(OSD_KEY_C)) { while( osd_key_pressed(OSD_KEY_C)); show[2]=(show[2])?0:1; }
	if(osd_key_pressed(OSD_KEY_V)) { while( osd_key_pressed(OSD_KEY_V)); show[3]=(show[3])?0:1; }
	if(osd_key_pressed(OSD_KEY_B)) { while( osd_key_pressed(OSD_KEY_B)); show[4]=(show[4])?0:1; }
	if(osd_key_pressed(OSD_KEY_N)) { while( osd_key_pressed(OSD_KEY_N)); show[5]=(show[5])?0:1; }

	if(osd_key_pressed(OSD_KEY_COMMA)) { while( osd_key_pressed(OSD_KEY_COMMA)); show[6]=(show[6])?0:1; }
	if(osd_key_pressed(OSD_KEY_STOP )) { while( osd_key_pressed(OSD_KEY_STOP )); show[7]=(show[7])?0:1; }

	erase_bitmap(bitmap);
	if(show[0]) draw_layer64( bitmap, 0x0000 );
	if(show[1]) draw_layer64( bitmap, 0x2000 );
	if(show[2]) draw_layer64( bitmap, 0x4000 );
	if(show[3]) draw_layer64( bitmap, 0x6000 );
	if(show[4]) draw_layer36( bitmap, 0x8010 );
	if(show[5]) draw_layer36( bitmap, 0x8810 );
	if(show[6]) draw_layerROZ( bitmap, 0);
	if(show[7]) draw_layerROZ( bitmap, 0x20000);
	draw_sprites( bitmap );

// Save video memory

	if( osd_key_pressed(OSD_KEY_S) ){
		FILE *f;
		while( osd_key_pressed(OSD_KEY_S) ); /* wait for key release */

		f = fopen("vidmem.dbg","wb");
		if( f ){
			fwrite( videoram,0x10000,1,f );
			fclose( f );
		}

//		f = fopen("ram1.dbg","wb");
//		if( f ){
//			fwrite( namcos2_cpu1_ram1, 0x1000, 1, f );
//			fclose( f );
//		}

		f = fopen("ram2.dbg","wb");
		if( f ){
			fwrite( namcos2_cpu1_ram2, 0x4000, 1, f );
			fclose( f );
		}

		f = fopen("ram3.dbg","wb");
		if( f ){
			fwrite( namcos2_roz_ram, 0x40000, 1, f );
			fclose( f );
		}


		f = fopen("share1.dbg","wb");
		if( f ){
			fwrite( namcos2_sharedram1, 0x10000, 1, f );
			fclose( f );
		}
	}

	if(osd_key_pressed(OSD_KEY_K)) { while( osd_key_pressed(OSD_KEY_K)); rshow[0]=(rshow[0])?0:1; }
	if(osd_key_pressed(OSD_KEY_L)) { while( osd_key_pressed(OSD_KEY_L)); rshow[1]=(rshow[1])?0:1; }
	if(rshow[0]) show_reg(bitmap);
	if(rshow[1]) show_sprites(bitmap);
}
