/***************************************************************************

Vendetta (GX081) (c) 1991 Konami

Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

Notes:
There is something corrupting memory (page fault on exit, during a free()),
but I haven't found what it is.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */

/* prototypes */
static void vendetta_init_machine( void );
static void vendetta_banking( int lines );
extern int konami_eeprom_read( void );
extern int konami_eeprom_ack( void );
extern void konami_eeprom_w( int clk, int dat, int active );
extern int simpsons_eeprom_load(void);
extern void simpsons_eeprom_save(void);
extern int simpsons_sound_interrupt_r( int offset );
extern int simpsons_sound_r(int offset);

/* to vidhrdw */
static int layer_colorbase[3],bg_colorbase,sprite_colorbase;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*code |= ((*color & 0x03) << 8) | ((*color & 0x30) << 6) |
			((*color & 0x0c) << 10) | (bank << 14);
	*color = ((*color & 0xc0) >> 6);
	*color += layer_colorbase[layer];
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

void vendetta_vh_stop( void )
{
	K052109_vh_stop();
}

#define TILEROM_MEM_REGION 1

int vendetta_vh_start( void )
{
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback) != 0)
		return 1;

	return 0;
}

#define SPRITEWORD(offset) (256*spriteram[(offset)]+spriteram[(offset)+1])
static void simpsons_drawsprites(struct osd_bitmap *bitmap,int pri)
{
#define NUM_SPRITES 256
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		sortedlist[SPRITEWORD(offs) & 0x00ff] = offs;
	}

	pri <<= 8;

	for (pri_code = NUM_SPRITES-1;pri_code >= 0;pri_code--)
	{
		int ox,oy,col,code,size,w,h,x,y,xa,ya,flipx,flipy,zoomx,zoomy;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		if ((SPRITEWORD(offs) & 0x8000) == 0) continue;

//		if ((SPRITEWORD(offs+0x0c) & 0x0300) != pri) continue;	/* wrong, there */
											/* are 5 priority bits in the schematics */

		size = (SPRITEWORD(offs) & 0x0f00) >> 8;

		w = 1 << (size & 0x03);
		h = 1 << ((size >> 2) & 0x03);

		code = SPRITEWORD(offs+0x02);

		/* the sprite can be start at any point in the 8x8 grid. We have to */
		/* adjust the offsets to draw it correctly. Simpsons does this all the time. */
		xa = 0;
		ya = 0;
		if (code & 0x01) xa += 1;
		if (code & 0x02) ya += 1;
		if (code & 0x04) xa += 2;
		if (code & 0x08) ya += 2;
		if (code & 0x10) xa += 4;
		if (code & 0x20) ya += 4;
		code &= ~0x3f;

		col = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
/* this bit is used in a few places but cannot be just shadow, it's used */
/* for normal sprites too. */
//			shadow = SPRITEWORD(offs+0x0c) & 0x0080;
#if 0
if (keyboard_pressed(KEYCODE_Q) && (SPRITEWORD(offs+0x0c) & 0x0800)) col = rand();
if (keyboard_pressed(KEYCODE_W) && (SPRITEWORD(offs+0x0c) & 0x0400)) col = rand();
if (keyboard_pressed(KEYCODE_E) && (SPRITEWORD(offs+0x0c) & 0x0200)) col = rand();
if (keyboard_pressed(KEYCODE_R) && (SPRITEWORD(offs+0x0c) & 0x0100)) col = rand();
if (keyboard_pressed(KEYCODE_A) && (SPRITEWORD(offs+0x0a) & 0x0080)) col = rand();
if (keyboard_pressed(KEYCODE_S) && (SPRITEWORD(offs+0x0a) & 0x0040)) col = rand();
if (keyboard_pressed(KEYCODE_D) && (SPRITEWORD(offs+0x0a) & 0x0020)) col = rand();
if (keyboard_pressed(KEYCODE_F) && (SPRITEWORD(offs+0x0a) & 0x0010)) col = rand();
#endif
		ox = (SPRITEWORD(offs+0x06) & 0x3ff) + 16+16;
		oy = 512-(SPRITEWORD(offs+0x04) & 0x3ff) - 128+8;
ox -= 128+128+32;
oy -= 128+8;

		/* zoom control:
		   0x40 = normal scale
		  <0x40 enlarge (0x20 = double size)
		  >0x40 reduce (0x80 = half size)
		*/
		zoomy = SPRITEWORD(offs+0x08);
		if (zoomy > 0x2000) continue;
		if (zoomy) zoomy = (0x400000+zoomy/2) / zoomy;
		else zoomy = 2 * 0x400000;
		if ((SPRITEWORD(offs) & 0x4000) == 0)
		{
			zoomx = SPRITEWORD(offs+0x0a);
			if (zoomx > 0x2000) continue;
			if (zoomx) zoomx = (0x400000+zoomx/2) / zoomx;
			else zoomx = 2 * 0x400000;
		}
		else zoomx = zoomy;

		/* the coordinates given are for the *center* of the sprite */
		ox -= (zoomx * w) >> 13;
		oy -= (zoomy * h) >> 13;

		flipx = SPRITEWORD(offs) & 0x1000;
		flipy = SPRITEWORD(offs) & 0x2000;

		if (zoomx == 0x10000 && zoomy == 0x10000)
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy;

				if (flipy)
					sy = oy + 16 * ((h-1) - y);
				else
					sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					if (flipx)
						sx = ox + 16 * ((w-1) - x);
					else
						sx = ox + 16 * x;

					drawgfx(bitmap,Machine->gfx[0],
							code + xoffset[(x+xa)&7] + yoffset[(y+ya)&7],
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy,zw,zh;

				if (flipy)
				{
					sy = oy + ((zoomy * ((h-1) - y) + (1<<11)) >> 12);
					zh = sy - (oy + ((zoomy * ((h-1) - (y+1)) + (1<<11)) >> 12));
				}
				else
				{
					sy = oy + ((zoomy * y + (1<<11)) >> 12);
					zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;
				}

				for (x = 0;x < w;x++)
				{
					if (flipx)
					{
						sx = ox + ((zoomx * ((w-1) - x) + (1<<11)) >> 12);
						zw = sx - (ox + ((zoomx * ((w-1) - (x+1)) + (1<<11)) >> 12));
					}
					else
					{
						sx = ox + ((zoomx * x + (1<<11)) >> 12);
						zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					}
					drawgfxzoom(bitmap,Machine->gfx[0],
							code + xoffset[(x+xa)&7] + yoffset[(y+ya)&7],
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
}

void vendetta_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;


	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI4);

	K052109_tilemap_update();

	palette_init_used_colors();

	/* palette remapping first */
	{
		unsigned short palette_map[128];
		int color;

		memset (palette_map, 0, sizeof (palette_map));

		/* sprites */
		for (offs = 0;offs < spriteram_size;offs += 16)
		{
			color = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
			}
		}
	}

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,1,TILEMAP_IGNORE_TRANSPARENCY);
	K052109_tilemap_draw(bitmap,2,0);
	simpsons_drawsprites(bitmap,0);
	K052109_tilemap_draw(bitmap,0,0);

#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(spriteram, spriteram_size, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
}

/********************************************/

static int vendetta_K052109_r( int offset ) { return K052109_r( offset + 0x2000 ); }
static void vendetta_K052109_w( int offset, int data ) { K052109_w( offset + 0x2000, data ); }

static void vendetta_video_banking( int select ) {

	if ( select & 1 ) {
		cpu_setbankhandler_r( 2, paletteram_r );
		cpu_setbankhandler_w( 2, paletteram_xBBBBBGGGGGRRRRR_swap_w );
		cpu_setbankhandler_r( 3, spriteram_r );
		cpu_setbankhandler_w( 3, spriteram_w );
	} else {
		cpu_setbankhandler_r( 2, vendetta_K052109_r );
		cpu_setbankhandler_w( 2, vendetta_K052109_w );
		cpu_setbankhandler_r( 3, K052109_r );
		cpu_setbankhandler_w( 3, K052109_w );
	}
}

static int vendetta_eeprom_r( int offset ) {
	int data = konami_eeprom_read();

	data |= konami_eeprom_ack() << 1;

	data |= readinputport( 3 ) & 0x0c;

	return data;
}

static void vendetta_eeprom_w( int offset, int data ) {
	int clk = ( data >> 4 ) & 1;
	int dat = ( data >> 5 ) & 1;
	int mode = ( data >> 3 ) & 1;

	if ( data == 0xff )
		return;

	konami_eeprom_w( clk, dat, mode );

	vendetta_video_banking( data & 1 );
}

static void vendetta_5fe0_w(int offset,int data)
{
char baf[40];
sprintf(baf,"5fe0 = %02x",data);
//usrintf_showmessage(baf);

	/* bit 0,1 coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
	/* bit 3 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line( ( data & 0x08 ) ? ASSERT_LINE : CLEAR_LINE );
	/* other bits unknown */
}

static int speedup_r( int offs )
{
	unsigned char *RAM = Machine->memory_region[0];

	int data = ( RAM[0x28d2] << 8 ) | RAM[0x28d3];

	if ( data < Machine->memory_region_length[0] )
	{
		data = ( RAM[data] << 8 ) | RAM[data + 1];

		if ( data == 0xffff )
			cpu_spinuntil_int();
	}

	return RAM[0x28d2];
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_BANK1	},
	{ 0x28d2, 0x28d2, speedup_r },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x5fc0, 0x5fc0, input_port_0_r },
	{ 0x5fc1, 0x5fc1, input_port_1_r },
	{ 0x5fd0, 0x5fd0, vendetta_eeprom_r }, /* vblank, service */
	{ 0x5fd1, 0x5fd1, input_port_2_r },
	{ 0x5fe4, 0x5fe4, simpsons_sound_interrupt_r },
	{ 0x5fe6, 0x5fe7, simpsons_sound_r },
	{ 0x5fea, 0x5fea, MRA_NOP }, /* watchdog */
	{ 0x4000, 0x5fff, MRA_BANK3 },
	{ 0x6000, 0x6fff, MRA_BANK2 },
	{ 0x4000, 0x7fff, K052109_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static void z80_irq_w( int offset, int data ) {
	cpu_cause_interrupt( 1, 0xff );
}

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x5fa0, 0x5faf, K053251_w },
	{ 0x5fe0, 0x5fe0, vendetta_5fe0_w },
	{ 0x5fe2, 0x5fe2, vendetta_eeprom_w },
	{ 0x5fe4, 0x5fe4, z80_irq_w },
	{ 0x5fe6, 0x5fe7, K053260_WriteReg },
	{ 0x4000, 0x5fff, MWA_BANK3 },
	{ 0x6000, 0x6fff, MWA_BANK2 },
	{ 0x4000, 0x7fff, K052109_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static void z80_arm_nmi(int offset,int data)
{
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, z80_arm_nmi },
	{ 0xfc00, 0xfc2f, K053260_WriteReg },
	{ -1 }	/* end of table */
};


/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************

	Graphics Decoding

***************************************************************************/

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	32768,	/* 32768 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
	  2*4, 3*4, 0*4, 1*4,
	  0x200000*8+2*4, 0x200000*8+3*4, 0x200000*8+0*4, 0x200000*8+1*4,
	  0x100000*8+2*4, 0x100000*8+3*4, 0x100000*8+0*4, 0x100000*8+1*4,
	  0x300000*8+2*4, 0x300000*8+3*4, 0x300000*8+0*4, 0x300000*8+1*4,
	},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 2, 0x000000, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};

/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K053260_interface k053260_interface =
{
	3579545,
	4, /* memory region */
	{ MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) },
	0
};

static int vendetta_irq( void )
{
	if (K052109_is_IRQ_enabled())
		return KONAMI_INT_IRQ;

	return ignore_interrupt();
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_KONAMI,
			6000000,		/* ? */
			0,
			readmem,writemem,0,0,
            vendetta_irq,1
        },
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,
			3,
			readmem_sound, writemem_sound,0,0,
			ignore_interrupt,0	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	vendetta_init_machine,

	/* video hardware */
	64*8, 32*8, { 13*8, (64-13)*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	vendetta_vh_start,
	vendetta_vh_stop,
	vendetta_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface
		}
	}
};

/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( vendetta_rom )
	ROM_REGION( 0x4b000 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081u01", 0x10000, 0x38000, 0xb4d9ade5 )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x100000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, 0xb4c777a9 ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, 0x272ac8d9 ) /* characters */

	ROM_REGION( 0x400000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, 0x464b9aa4 ) /* sprites */
	ROM_LOAD( "081a06", 0x100000, 0x100000, 0xe9fe6d80 ) /* sprites */
	ROM_LOAD( "081a05", 0x200000, 0x100000, 0x00000000 ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, 0x8a22b29a ) /* sprites */

	ROM_REGION( 0x10000 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, 0x4c604d9b )

	ROM_REGION( 0x100000 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, 0x14b6baea )
ROM_END

ROM_START( vendett2_rom )
	ROM_REGION( 0x4b000 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081d01", 0x10000, 0x38000, 0x335da495 )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x100000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, 0xb4c777a9 ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, 0x272ac8d9 ) /* characters */

	ROM_REGION( 0x400000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, 0x464b9aa4 ) /* sprites */
	ROM_LOAD( "081a06", 0x100000, 0x100000, 0xe9fe6d80 ) /* sprites */
	ROM_LOAD( "081a05", 0x200000, 0x100000, 0x00000000 ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, 0x8a22b29a ) /* sprites */

	ROM_REGION( 0x10000 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, 0x4c604d9b )

	ROM_REGION( 0x100000 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, 0x14b6baea )
ROM_END

ROM_START( vendettj_rom )
	ROM_REGION( 0x4b000 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081p01", 0x10000, 0x38000, 0x5fe30242 )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x100000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, 0xb4c777a9 ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, 0x272ac8d9 ) /* characters */

	ROM_REGION( 0x400000 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, 0x464b9aa4 ) /* sprites */
	ROM_LOAD( "081a06", 0x100000, 0x100000, 0xe9fe6d80 ) /* sprites */
	ROM_LOAD( "081a05", 0x200000, 0x100000, 0x00000000 ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, 0x8a22b29a ) /* sprites */

	ROM_REGION( 0x10000 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, 0x4c604d9b )

	ROM_REGION( 0x100000 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, 0x14b6baea )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void vendetta_banking( int lines )
{
	unsigned char *RAM = Machine->memory_region[0];

	if ( lines >= 0x1c ) {
		if ( errorlog )
			fprintf( errorlog, "PC = %04x : Unknown bank selected %02x\n", cpu_get_pc(), lines );
	} else
		cpu_setbank( 1, &RAM[ 0x10000 + ( lines * 0x2000 ) ] );
}

static void vendetta_init_machine( void )
{
	konami_cpu_setlines_callback = vendetta_banking;

	paletteram = &Machine->memory_region[0][0x48000];
	spriteram = &Machine->memory_region[0][0x49000];
	spriteram_size = 0x1000;

	/* init banks */
	cpu_setbank( 1, &Machine->memory_region[0][0x10000] );
	vendetta_video_banking( 0 );
}

static void gfx_untangle(void)
{
	konami_rom_deinterleave(1);
}

struct GameDriver vendetta_driver =
{
	__FILE__,
	0,
	"vendetta",
	"Vendetta (Asia set 1)",
	"1991",
	"Konami",
	"Ernesto Corvi",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	vendetta_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_DEFAULT,
	simpsons_eeprom_load, simpsons_eeprom_save
};

struct GameDriver vendett2_driver =
{
	__FILE__,
	&vendetta_driver,
	"vendett2",
	"Vendetta (Asia set 2)",
	"1991",
	"Konami",
	"Ernesto Corvi",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	vendett2_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_DEFAULT,
	simpsons_eeprom_load, simpsons_eeprom_save
};

struct GameDriver vendettj_driver =
{
	__FILE__,
	&vendetta_driver,
	"vendettj",
	"Crime Fighters 2 (Japan)",
	"1991",
	"Konami",
	"Ernesto Corvi",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	vendettj_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_DEFAULT,
	simpsons_eeprom_load, simpsons_eeprom_save
};
