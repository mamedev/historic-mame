/****************************************************************
 Preliminary Ping Pong King driver
      (based on gladiator)

 Tomasz Slanina   analog [at] op.pl

PING PONG KING
TAITO 1985
M6100094A

-------------
P0-003


X Q0_10 Q0_9 Q0_8 Q0_7 Q0_6 6116 Q0_5 Q0_4 Q0_3 Q1_2 Q1_1
                                                 Q1
                                   Z80B
                                       8251
 2009 2009 2009
               AQ-001
          Q2
              2116
              2116
                                 AQ-003
-------------
P1-004


 QO_15 TMM2009 Q0_14 Q0_13 Q0_12     2128 2128


                                                           SW1
                                                    AQ-004

                                              SW2          SW3
   6809 X Q0_19 Q0_18
                           Z80  Q0_17 Q0_16 2009 AQ-003 YM2203


*******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

struct tilemap *ppking_tilemap,*ppking_textmap;

data8_t *textram, *mainram;

static WRITE8_HANDLER(ppking_videoram_w)
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(ppking_tilemap,offset&0x7ff);
}

static WRITE8_HANDLER(ppking_textram_w)
{
	textram[offset]=data;
	tilemap_mark_tile_dirty(ppking_textmap,offset);
}

static WRITE8_HANDLER (ppking_palette_w)
{
	int r,g,b;
	paletteram[offset]=data;
	offset&=0x3ff;
	r = (paletteram[offset] >> 0) & 0x0f;
	g = (paletteram[offset] >> 4) & 0x0f;
	b = (paletteram[offset+0x400] >> 0) & 0x0f;
	r = (r << 1) + ((paletteram[offset+0x400] >> 4) & 0x01);
	g = (g << 1) + ((paletteram[offset+0x400] >> 5) & 0x01);
	b = (b << 1) + ((paletteram[offset+0x400] >> 6) & 0x01);
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette_set_color(offset,r,g,b);
}

static void get_ppking_tile_info(int tile_index)
{
	int tileno,palno;
	tileno = videoram[tile_index]+256*(videoram[tile_index+0x800]&0x7);
	palno=0x1f-(videoram[tile_index+0x800]>>3);
	SET_TILE_INFO(1+((tileno/512)&1),tileno&511,palno,0)
}

static void get_ppking_text_info(int tile_index)
{
	int tileno;
	tileno = textram[tile_index];
	SET_TILE_INFO(0,tileno,0,0)
}

VIDEO_START(ppking)
{
	ppking_tilemap = tilemap_create(get_ppking_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,32,64);
	ppking_textmap = tilemap_create(get_ppking_text_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,64);
	tilemap_set_transparent_pen(ppking_textmap,0);
	tilemap_set_scroll_cols(ppking_tilemap,16);
	alpha_set_level(0x80);
	palette_set_color(1024,0x00,0x00,0x00);
	palette_set_color(1025,0x00,0x00,0x00);
	return 0;
}

int sprite_offset;

static void render_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect )
	{
	unsigned char *source = spriteram;
	unsigned char *finish = source+0x400;

	do{
		int attributes = source[0x800];
		int bank = attributes&3;
		int tile_number = source[0];
		int sx = source[0x400+1] + 256*(source[0x801]&1);
		int sy = (240-source[0x400]+sprite_offset)&0xff;
		int xflip = attributes&0x04;
		int yflip = attributes&0x08;
		int color = 0x20 + (source[1]&0x1F);

		if(attributes&0x10)
		{
		 bank+=4;
		 if(tile_number&128)bank++;
		 tile_number=(tile_number>>2)&31;
		 sy-=16;
		}

		drawgfx(bitmap,Machine->gfx[3+bank],
				tile_number,
				color,
				xflip,yflip,
				sx-56,sy,
				cliprect,TRANSPARENCY_PEN,0);


		source+=2;
	}while( source<finish );
}

VIDEO_UPDATE(ppking)
{
	if(code_pressed_memory(KEYCODE_Q))
		cpunum_set_input_line( 0, INPUT_LINE_NMI, PULSE_LINE );

	tilemap_draw(bitmap,cliprect,ppking_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,ppking_textmap,TILEMAP_ALPHA,0);
	render_sprites(bitmap,cliprect);
}

WRITE8_HANDLER( scroll_w ){	tilemap_set_scrolly(ppking_tilemap, offset,data);}

WRITE8_HANDLER( sprite_scroll_w ){ sprite_offset=data;}

WRITE8_HANDLER( ppking_irq_patch_w ){ }

static int data1,data2,flag1=1,flag2=1;

static WRITE8_HANDLER(qx0_w)
{
	if(!offset)
	{
		data2=data;
		flag2=1;
	}
}

static WRITE8_HANDLER(qx1_w)
{
	if(!offset)
	{
		data1=data;
		flag1=1;
	}
}

static WRITE8_HANDLER(qx2_w){ }

static WRITE8_HANDLER(qx3_w){ }

static READ8_HANDLER(qx2_r){ return rand(); }

static READ8_HANDLER(qx3_r){ return rand()&0xf; }

static READ8_HANDLER(qx0_r)
{
	if(!offset)
		 return data1;
	else
		return flag2;
}

static READ8_HANDLER(qx1_r)
{
	if(!offset)
		return data2;
	else
		return flag1;
}

static ADDRESS_MAP_START( cpu1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcbff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xcc00, 0xcc0f) AM_WRITE(scroll_w)
	AM_RANGE(0xce00, 0xce00) AM_WRITE(sprite_scroll_w)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(MRA8_RAM) AM_WRITE(ppking_palette_w)  AM_BASE(&paletteram)
	AM_RANGE(0xd800, 0xe7ff) AM_READ(MRA8_RAM) AM_WRITE(ppking_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xefff) AM_READ(MRA8_RAM) AM_WRITE(ppking_textram_w)  AM_BASE(&textram)
	AM_RANGE(0xf000, 0xf3ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf400, 0xffff) AM_RAM AM_BASE(&mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( cpu3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x2000, 0x2fff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_port, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x04, 0x04) AM_WRITE(ppking_irq_patch_w)
	AM_RANGE(0x9e, 0x9f) AM_READ(qx0_r) AM_WRITE(qx0_w)
	AM_RANGE(0xbf, 0xbf) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_port, ADDRESS_SPACE_IO, 8 )

	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READ(YM2203_read_port_0_r) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x20, 0x21) AM_READ(qx1_r) AM_WRITE(qx1_w)
	AM_RANGE(0x40, 0x40) AM_READ(MRA8_NOP)
	AM_RANGE(0x60, 0x61) AM_READ(qx2_r) AM_WRITE(qx2_w)
	AM_RANGE(0x80, 0x81) AM_READ(qx3_r) AM_WRITE(qx3_w)
ADDRESS_MAP_END

INPUT_PORTS_START( ppking )

INPUT_PORTS_END


#define DEFINE_LAYOUT( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	8,8, \
	512, \
	3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3 }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, \
	128 \
};


#define DEFINE_LAYOUTS( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	16,16, \
	128, \
	3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3,  128+0,128+1,128+2,128+3,  128+64+0, 128+64+1, 128+64+2,128+64+3  }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 256+ 0*8,256+1*8,256+2*8,256+3*8,256+4*8,256+5*8,256+6*8,256+7*8 }, \
	128*4 \
};


#define DEFINE_LAYOUTSB( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	32,32, \
	32, \
	3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3,  128+0,128+1,128+2,128+3,  128+64+0, 128+64+1, 128+64+2,128+64+3,     128*4+ 0,128*4+1,128*4+2,128*4+3,128*4+64+0,128*4+64+1,128*4+64+2,128*4+64+3,  128*4+128+0,128*4+128+1,128*4+128+2,128*4+128+3, 128*4+ 128+64+0,128*4+ 128+64+1,128*4+ 128+64+2,128*4+128+64+3 }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 256+ 0*8,256+1*8,256+2*8,256+3*8,256+4*8,256+5*8,256+6*8,256+7*8,   128*8+  0*8,128*8+ 1*8,128*8+ 2*8,128*8+ 3*8,128*8+ 4*8,128*8+ 5*8,128*8+ 6*8,128*8+ 7*8, 128*8+ 256+ 0*8,128*8+ 256+1*8,128*8+ 256+2*8,128*8+ 256+3*8,128*8+ 256+4*8,128*8+ 256+5*8,128*8+ 256+6*8,128*8+ 256+7*8 }, \
	128*16 \
};

DEFINE_LAYOUT( ppking_tile0, 4,	0x02000*8, 0x02000*8+4 )
DEFINE_LAYOUT( ppking_tile1, 0, 0x04000*8, 0x04000*8+4 )

DEFINE_LAYOUTS( ppking_tiles0, 4,0x2000*8,0x2000*8+4 )
DEFINE_LAYOUTS( ppking_tiles1, 0,0x4000*8,0x4000*8+4 )
DEFINE_LAYOUTS( ppking_tiles2, 0x6000*8+4,0x8000*8,0x8000*8+4 )
DEFINE_LAYOUTS( ppking_tiles3, 0x6000*8,0x6000*8,0x6000*8 )

DEFINE_LAYOUTSB( ppking_tilesb0, 4,0x2000*8,0x2000*8+4 )
DEFINE_LAYOUTSB( ppking_tilesb1, 0,0x4000*8,0x4000*8+4 )
DEFINE_LAYOUTSB( ppking_tilesb2, 0x6000*8+4,0x8000*8,0x8000*8+4 )
DEFINE_LAYOUTSB( ppking_tilesb3, 0x6000*8,0x6000*8,0x6000*8 )

static struct GfxLayout ppking_text_layout  =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(3,4),RGN_FRAC(3,4),RGN_FRAC(3,4)},
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	64
};
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &ppking_text_layout, 0, 64 },
	{ REGION_GFX2, 0x00000, &ppking_tile0, 0, 64 },
	{ REGION_GFX2, 0x00000, &ppking_tile1, 0, 64 },

	{ REGION_GFX3, 0x00000, &ppking_tiles0, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tiles1, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tiles2, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tiles3, 0, 64 },

	{ REGION_GFX3, 0x00000, &ppking_tilesb0, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tilesb1, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tilesb2, 0, 64 },
	{ REGION_GFX3, 0x00000, &ppking_tilesb3, 0, 64 },
	{ -1 }
};

#undef DEFINE_LAYOUT


static READ8_HANDLER(f1_r)
{
	return rand();
}

static struct YM2203interface ym2203_interface =
{
	1,		/* 1 chip */
	1500000,	/* 1.5 MHz? */
	{ YM2203_VOL(25,25) },
	{ f1_r },
	{ f1_r },         /* port B read */
	{ 0 }, 						/* port A write */
	{ 0 },
	{ 0 }          		/* NMI request for 2nd cpu */
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	455000,				/* 455KHz ??          */
	{ 0 },				/* interrupt function */
	{ MSM5205_SEX_4B},	/* vclk input mode    */
	{ 60 }
};




static READ8_HANDLER( ppking_dsw1_r )
{
	int orig = readinputport(0); /* DSW1 */
/*Reverse all bits for Input Port 0*/
/*ie..Bit order is: 0,1,2,3,4,5,6,7*/
return   ((orig&0x01)<<7) | ((orig&0x02)<<5)
       | ((orig&0x04)<<3) | ((orig&0x08)<<1)
       | ((orig&0x10)>>1) | ((orig&0x20)>>3)
       | ((orig&0x40)>>5) | ((orig&0x80)>>7);;
}

static READ8_HANDLER( ppking_dsw2_r )
{
	int orig = readinputport(1); /* DSW2 */
/*Bits 2-7 are reversed for Input Port 1*/
/*ie..Bit order is: 2,3,4,5,6,7,1,0*/
return	  (orig&0x01) | (orig&0x02)
	| ((orig&0x04)<<5) | ((orig&0x08)<<3)
	| ((orig&0x10)<<1) | ((orig&0x20)>>1)
	| ((orig&0x40)>>3) | ((orig&0x80)>>5);
}

static READ8_HANDLER( ppking_controll_r ) {	return 0; }

static READ8_HANDLER( ppking_button3_r ) {	return 0; }

static MACHINE_DRIVER_START( ppking )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_PROGRAM_MAP(cpu1_map,0)
	MDRV_CPU_IO_MAP(cpu1_port,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(cpu2_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	MDRV_CPU_IO_MAP(cpu2_port,0)

	MDRV_CPU_ADD(M6809, 750000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(cpu3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER| VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024+2)

	MDRV_VIDEO_START(ppking)
	MDRV_VIDEO_UPDATE(ppking)

	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

ROM_START( ppking )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "q1_1.a1",          0x00000, 0x2000,  CRC(b74b2718) SHA1(29833439211076873324ccfa5897eb1e6aa9d134) )
	ROM_LOAD( "q1_2.b1",          0x02000, 0x2000,  CRC(1b1e4cd4) SHA1(34c6cf5e0775c0c834dda34a3a2a4685465daa8e) )
	ROM_LOAD( "q0_3.c1",          0x04000, 0x2000,  CRC(6a7acf8e) SHA1(06d37e813605f507ea1c720764fc554e58defdf8) )
	ROM_LOAD( "q0_4.d1",          0x06000, 0x2000,  CRC(b83eb6d5) SHA1(f112d3c0d701977dcc5c312ad74d78b44882201b) )
	ROM_LOAD( "q0_5.e1",          0x08000, 0x4000,  CRC(4d2007e2) SHA1(973ef0e6ff6065b753402489a3d10a9b68164969) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "q0_17.6f",       	0x0000, 0x2000,  CRC(f7fe0d24) SHA1(6dcb23aa7fc08fc892a8b3843ccb982997c20571) )
	ROM_LOAD( "q0_16.6e",       	0x4000, 0x2000,  CRC(b1e32588) SHA1(13c74479238a34a08e249f9120b42a52d80f8274) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )
	ROM_LOAD( "q0_19.5n",         0x0c000, 0x2000,  CRC(4bcf896d) SHA1(f587a66fcc63e989742ce2d5f4cf2bb464987038) )
	ROM_LOAD( "q0_18.5m",         0x0e000, 0x2000,  CRC(89ba64f8) SHA1(fa01316ea744b4277ee64d5f14cb6d7e3a949f2b) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_15.1r",       	0x00000, 0x2000,  CRC(fbd33219) SHA1(78b9bb327ededaa818d26c41c5e8fd1c041ef142) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_12.1j",       	0x00000, 0x2000,  CRC(b1a44482) SHA1(84cc40976aa9b015a9f970a878bbde753651b3ba) )
	ROM_LOAD( "q0_13.1k",       	0x02000, 0x2000,  CRC(468f35e6) SHA1(8e28481910663fe525cefd4ad406468b7736900e) )
	ROM_LOAD( "q0_14.1m",       	0x04000, 0x2000,  CRC(eed04a7f) SHA1(d139920889653c33ded38a85510789380dd0aa9e) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_6.k1",        	0x00000, 0x2000,  CRC(bb3d666c) SHA1(a689c7a1e75b916d69f396db7c4688ac355c2aff) )
	ROM_LOAD( "q0_8.m1",        	0x02000, 0x2000,  CRC(41235b22) SHA1(4d9702efe0ea320dab7c0d889f4d03f196b32661) )
	ROM_LOAD( "q0_9.p1",        	0x04000, 0x2000,  CRC(74cc94b2) SHA1(2cb981ecb2487dfa5c0974e036106fc06c2c1880) )
	ROM_LOAD( "q0_7.l1",        	0x06000, 0x2000,  CRC(16a2550e) SHA1(adb54b70a6db660b5f29ad66da02afd8e99884bb) )
	ROM_LOAD( "q0_10.r1",       	0x08000, 0x2000,  CRC(af438cc6) SHA1(cf79c8d3f2a0c489a756b341f8df747f6654764b) )

	ROM_REGION( 0x040, REGION_PROMS, 0 )
	ROM_LOAD( "q1",   0x0000, 0x020, CRC(cca9ae7b) SHA1(e18416fbe2a5b09db749cc9a14fa89186ed8f4ba) )
	ROM_LOAD( "q2",   0x0020, 0x020, CRC(da952b5e) SHA1(0863ad8fdcf69435a7455cd345d3d0af0b0c0a0a) )

ROM_END


static READ8_HANDLER(f6a3_r)
{
	if(activecpu_get_previouspc()==0x8e)
		mainram[0x2a3]=1;

	return mainram[0x2a3];
}

static DRIVER_INIT(ppking)
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf6a3,0xf6a3,0,0, f6a3_r );
}

GAMEX( 1985, ppking,   0,        ppking, ppking, ppking, ROT90, "Taito America Corporation", "Ping Pong King",GAME_NOT_WORKING)
