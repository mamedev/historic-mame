/***************************************************************************

Tehkan World Cup - (c) Tehkan 1985


Ernesto Corvi
ernesto@imagina.com

Roberto Juan Fresca
robbiex@rocketmail.com

TODO:
- ADPCM samples for Gridiron and Tee'd Off
- dip switches and input ports for Gridiron and Tee'd Off

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *tehkanwc_videoram1;
extern int tehkanwc_videoram1_size;

/* from vidhrdw */
int tehkanwc_vh_start(void);
void tehkanwc_vh_stop(void);
void tehkanwc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int tehkanwc_videoram1_r(int offset);
void tehkanwc_videoram1_w(int offset,int data);
int tehkanwc_scroll_x_r(int offset);
int tehkanwc_scroll_y_r(int offset);
void tehkanwc_scroll_x_w(int offset,int data);
void tehkanwc_scroll_y_w(int offset,int data);
void gridiron_led0_w(int offset,int data);
void gridiron_led1_w(int offset,int data);


static unsigned char *shared_ram;

static int shared_r(int offset)
{
	return shared_ram[offset];
}

static void shared_w(int offset,int data)
{
	shared_ram[offset] = data;
}


static void sub_cpu_halt_w(int offset,int data)
{
	if (data)
	{
		cpu_reset(1);
		cpu_halt(1,1);
	}
	else
		cpu_halt(1,0);
}



static int track0[2],track1[2];

static int tehkanwc_track_0_r(int offset)
{
	return readinputport(3 + offset) - track0[offset];
}

static int tehkanwc_track_1_r(int offset)
{
	return readinputport(6 + offset) - track1[offset];
}

static void tehkanwc_track_0_reset_w(int offset,int data)
{
	/* reset the trackball counters */
	track0[offset] = readinputport(3 + offset) + data;
}

static void tehkanwc_track_1_reset_w(int offset,int data)
{
	/* reset the trackball counters */
	track1[offset] = readinputport(6 + offset) + data;
}



static void sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,Z80_NMI_INT);
}

static void reset_callback(int param)
{
	cpu_reset(2);
}

static void sound_answer_w(int offset,int data)
{
	soundlatch2_w(0,data);

	/* in Gridiron, the sound CPU goes in a tight loop after the self test, */
	/* probably waiting to be reset by a watchdog */
	if (cpu_getpc() == 0x08bc) timer_set(TIME_IN_SEC(1),0,reset_callback);
}


static int sound_num = 0;

static int tehkanwc_portA_r(int offset)
{
	if (!ADPCM_playing(0) && sound_num == 0x2000)
		ADPCM_trigger(0,sound_num);

	return ADPCM_playing(0);
}

static int tehkanwc_portB_r( int offset )
{
	return ADPCM_playing( 0 );
}

static void tehkanwc_portA_w(int offset,int data)
{
	sound_num = data & 0xff;
}

static void tehkanwc_portB_w(int offset,int data)
{
	sound_num |= data << 8;

	if (sound_num == 0x2000)
		ADPCM_setvol(0,255);
	else
		ADPCM_setvol(0,128);

	ADPCM_trigger(0,sound_num);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r, &shared_ram },
	{ 0xd000, 0xd3ff, videoram_r },
	{ 0xd400, 0xd7ff, colorram_r },
	{ 0xd800, 0xddff, paletteram_r },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram1_r },
	{ 0xe800, 0xebff, spriteram_r }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_r },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_r },
	{ 0xf800, 0xf801, tehkanwc_track_0_r }, /* track 0 x/y */
	{ 0xf802, 0xf802, input_port_9_r }, /* Coin & Start */
	{ 0xf803, 0xf803, input_port_5_r }, /* joy0 - button */
	{ 0xf810, 0xf811, tehkanwc_track_1_r }, /* track 1 x/y */
	{ 0xf813, 0xf813, input_port_8_r }, /* joy1 - button */
	{ 0xf820, 0xf820, soundlatch2_r },	/* answer from the sound CPU */
	{ 0xf840, 0xf840, input_port_0_r }, /* DSW1 */
	{ 0xf850, 0xf850, input_port_1_r },	/* DSW2 */
	{ 0xf860, 0xf860, watchdog_reset_r },
	{ 0xf870, 0xf870, input_port_2_r }, /* DSW3 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram1_w, &tehkanwc_videoram1, &tehkanwc_videoram1_size },
	{ 0xe800, 0xebff, spriteram_w, &spriteram, &spriteram_size }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
	{ 0xf800, 0xf801, tehkanwc_track_0_reset_w },
	{ 0xf802, 0xf802, gridiron_led0_w },
	{ 0xf810, 0xf811, tehkanwc_track_1_reset_w },
	{ 0xf812, 0xf812, gridiron_led1_w },
	{ 0xf820, 0xf820, sound_command_w },
	{ 0xf840, 0xf840, sub_cpu_halt_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sub[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r },
	{ 0xd000, 0xd3ff, videoram_r },
	{ 0xd400, 0xd7ff, colorram_r },
	{ 0xd800, 0xddff, paletteram_r },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram1_r },
	{ 0xe800, 0xebff, spriteram_r }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_r },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_r },
	{ 0xf860, 0xf860, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_sub[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w },
	{ 0xd000, 0xd3ff, videoram_w },
	{ 0xd400, 0xd7ff, colorram_w },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram1_w },
	{ 0xe800, 0xebff, spriteram_w }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8001, 0x8001, MWA_NOP },	/* ???? */
	{ 0x8002, 0x8002, MWA_NOP },	/* ?? written in the IRQ handler */
	{ 0x8003, 0x8003, MWA_NOP },	/* ?? written in the NMI handler */
	{ 0xc000, 0xc000, sound_answer_w },	/* answer for main CPU */
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, AY8910_read_port_0_r },
	{ 0x02, 0x02, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
	{ 0x02, 0x02, AY8910_write_port_1_w },
	{ 0x03, 0x03, AY8910_control_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START /* DSW1 - Active LOW */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING ( 0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING ( 0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING ( 0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING ( 0x03, "1 Coin/5 Credits" )
	PORT_DIPSETTING ( 0x02, "1 Coin/6 Credits" )
	PORT_DIPSETTING ( 0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING ( 0x00, "2 Coins/3 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING ( 0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING ( 0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING ( 0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING ( 0x18, "1 Coin/5 Credits" )
	PORT_DIPSETTING ( 0x10, "1 Coin/6 Credits" )
	PORT_DIPSETTING ( 0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING ( 0x00, "2 Coins/3 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Extra Time/Extra Coin", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x40, "Normal" )
	PORT_DIPSETTING ( 0x00, "Double" )
	PORT_DIPNAME( 0x80, 0x80, "1P Start", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x80, "1 Credit" )
	PORT_DIPSETTING ( 0x00, "2 Credits" )

	PORT_START /* DSW2 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, "1P Game Time", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x03, "1:30" )
	PORT_DIPSETTING ( 0x02, "1:00" )
	PORT_DIPSETTING ( 0x01, "2:00" )
	PORT_DIPSETTING ( 0x00, "2:30" )
	PORT_DIPNAME( 0x7c, 0x7c, "2P Game Time", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x7c, "3:30/2:00 Extra" )
	PORT_DIPSETTING ( 0x78, "1:00/0:45 Extra" )
	PORT_DIPSETTING ( 0x74, "1:30/1:00 Extra" )
	PORT_DIPSETTING ( 0x70, "2:00/1:15 Extra" )
	PORT_DIPSETTING ( 0x6c, "2:30/1:30 Extra" )
	PORT_DIPSETTING ( 0x68, "3:00/1:45 Extra" )
	PORT_DIPSETTING ( 0x64, "4:00/2:15 Extra" )
	PORT_DIPSETTING ( 0x60, "5:00/2:45 Extra" )
	PORT_DIPSETTING ( 0x5c, "3:30/1:45 Extra" )
	PORT_DIPSETTING ( 0x58, "1:00/0:30 Extra" )
	PORT_DIPSETTING ( 0x54, "1:30/0:45 Extra" )
	PORT_DIPSETTING ( 0x50, "2:00/1:00 Extra" )
	PORT_DIPSETTING ( 0x4c, "2:30/1:15 Extra" )
	PORT_DIPSETTING ( 0x48, "3:00/1:30 Extra" )
	PORT_DIPSETTING ( 0x44, "4:00/2:00 Extra" )
	PORT_DIPSETTING ( 0x40, "5:00/2:30 Extra" )
	PORT_DIPSETTING ( 0x3c, "3:30/1:50 Extra" )
	PORT_DIPSETTING ( 0x38, "1:00/0:35 Extra" )
	PORT_DIPSETTING ( 0x34, "1:30/0:50 Extra" )
	PORT_DIPSETTING ( 0x30, "2:00/1:05 Extra" )
	PORT_DIPSETTING ( 0x2c, "2:30/1:20 Extra" )
	PORT_DIPSETTING ( 0x28, "3:00/1:35 Extra" )
	PORT_DIPSETTING ( 0x24, "4:00/2:05 Extra" )
	PORT_DIPSETTING ( 0x20, "5:00/2:35 Extra" )
	PORT_DIPSETTING ( 0x1c, "3:30/2:15 Extra" )
	PORT_DIPSETTING ( 0x18, "1:00/1:00 Extra" )
	PORT_DIPSETTING ( 0x14, "1:30/1:15 Extra" )
	PORT_DIPSETTING ( 0x10, "2:00/1:30 Extra" )
	PORT_DIPSETTING ( 0x0c, "2:30/1:45 Extra" )
	PORT_DIPSETTING ( 0x08, "3:00/2:00 Extra" )
	PORT_DIPSETTING ( 0x04, "4:00/2:30 Extra" )
	PORT_DIPSETTING ( 0x00, "5:00/3:00 Extra" )
	PORT_DIPNAME( 0x80, 0x80, "Game Type", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x80, "Timer In" )
	PORT_DIPSETTING ( 0x00, "Credit In" )

	PORT_START /* DSW3 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x03, "Moderate" )
	PORT_DIPSETTING ( 0x02, "Easy" )
	PORT_DIPSETTING ( 0x01, "Hard" )
	PORT_DIPSETTING ( 0x00, "Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x04, "60/60" )
	PORT_DIPSETTING ( 0x00, "55/60" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x08, "On" )
	PORT_DIPSETTING ( 0x00, "Off" )

	PORT_START /* IN0 - X AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 63, 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_JOY_LEFT, OSD_JOY_RIGHT, 63 )

	PORT_START /* IN0 - Y AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 63, 0, 0, OSD_KEY_UP, OSD_KEY_DOWN, OSD_JOY_UP, OSD_JOY_DOWN, 63 )

	PORT_START /* IN0 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* IN1 - X AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 63, 0, 0, OSD_KEY_D, OSD_KEY_G, 0, 0, 63 )

	PORT_START /* IN1 - Y AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 63, 0, 0, OSD_KEY_R, OSD_KEY_F, 0, 0, 63 )

	PORT_START /* IN1 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 - Active LOW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
			8*32+1*4, 8*32+0*4, 8*32+3*4, 8*32+2*4, 8*32+5*4, 8*32+4*4, 8*32+7*4, 8*32+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,8,	/* 16*8 characters */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
		32*8+1*4, 32*8+0*4, 32*8+3*4, 32*8+2*4, 32*8+5*4, 32*8+4*4, 32*8+7*4, 32*8+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,     0, 16 }, /* Colors 0 - 255 */
	{ 1, 0x04000, &spritelayout, 256,  8 }, /* Colors 256 - 383 */
	{ 1, 0x14000, &tilelayout,   512, 16 }, /* Colors 512 - 767 */
	{ -1 } /* end of array */
};



static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,		/* 8000Hz playback */
	4,			/* memory region 4 */
	0,			/* init function */
	{ 255 }
};

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	3579545 / 2,	/* 3.579545 / 2 MHz */
	{ 255, 255 },
	{ 0, tehkanwc_portA_r },
	{ 0, tehkanwc_portB_r },
	{ tehkanwc_portA_w, 0 },
	{ tehkanwc_portB_w, 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4608000,	/* 18.432000 / 4 */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4608000,	/* 18.432000 / 4 */
			2,
			readmem_sub,writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80,	/* communication is bidirectional, can't mark it as AUDIO_CPU */
			4608000,	/* 18.432000 / 4 */
			3,
			readmem_sound,writemem_sound,sound_readport,sound_writeport,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - seems enough to keep the CPUs in sync */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	768, 768,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tehkanwc_vh_start,
	tehkanwc_vh_stop,
	tehkanwc_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tehkanwc_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "twc-1.bin", 0x0000, 0x4000, 0xbf8ac078 , 0x34d6d5ff )
	ROM_LOAD( "twc-2.bin", 0x4000, 0x4000, 0x75a41fb0 , 0x7017a221 )
	ROM_LOAD( "twc-3.bin", 0x8000, 0x4000, 0xf0e98f11 , 0x8b662902 )

	ROM_REGION_DISPOSE(0x24000)	/* 64k for graphics (disposed after conversion) */
	ROM_LOAD( "twc-12.bin", 0x00000, 0x4000, 0xaa4b72a3 , 0xa9e274f8 )	/* fg tiles */
	ROM_LOAD( "twc-8.bin", 0x04000, 0x8000, 0x75636cf3 , 0x055a5264 )	/* sprites */
	ROM_LOAD( "twc-7.bin", 0x0c000, 0x8000, 0x769572b5 , 0x59faebe7 )
	ROM_LOAD( "twc-11.bin", 0x14000, 0x8000, 0xeea545a1 , 0x669389fc )	/* bg tiles */
	ROM_LOAD( "twc-9.bin", 0x1c000, 0x8000, 0x6d4f0a05 , 0x347ef108 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "twc-4.bin", 0x0000, 0x8000, 0xff8f3651 , 0x70a9f883 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "twc-6.bin", 0x0000, 0x4000, 0xccbc61e8 , 0xe3112be2 )

	ROM_REGION(0x4000)	/* 64k for adpcm sounds */
	ROM_LOAD( "twc-5.bin", 0x0000, 0x4000, 0x425783fb , 0x444b5544 )
ROM_END

ROM_START( gridiron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gfight1.bin", 0x0000, 0x4000, 0xd620e7e0 , 0x51612741 )
	ROM_LOAD( "gfight2.bin", 0x4000, 0x4000, 0x1a9d4459 , 0xa678db48 )
	ROM_LOAD( "gfight3.bin", 0x8000, 0x4000, 0x4f3af468 , 0x8c227c33 )

	ROM_REGION_DISPOSE(0x24000)	/* 64k for graphics (disposed after conversion) */
	ROM_LOAD( "gfight7.bin", 0x00000, 0x4000, 0x3937e885 , 0x04390cca )	/* fg tiles */
	ROM_LOAD( "gfight8.bin", 0x04000, 0x4000, 0xd38a3450 , 0x5de6a70f )	/* sprites */
	ROM_LOAD( "gfight9.bin", 0x08000, 0x4000, 0xa01e61fc , 0xeac9dc16 )
	ROM_LOAD( "gfight10.bin", 0x0c000, 0x4000, 0xac85ae41 , 0x61d0690f )
	/* 10000-13fff empty */
	ROM_LOAD( "gfight11.bin", 0x14000, 0x4000, 0x6fc495e4 , 0x80b09c03 )	/* bg tiles */
	ROM_LOAD( "gfight12.bin", 0x18000, 0x4000, 0xe071fec3 , 0x1b615eae )
	/* 1c000-23fff empty */

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gfight4.bin", 0x0000, 0x4000, 0xce164204 , 0x8821415f )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gfight5.bin", 0x0000, 0x4000, 0x4986b4ca , 0x92ca3c07 )

	ROM_REGION(0x8000)	/* 64k for adpcm sounds */
	ROM_LOAD( "gfight6.bin", 0x0000, 0x4000, 0x1a15a82b , 0xd05d463d )
ROM_END

ROM_START( teedoff_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "to-1.bin", 0x0000, 0x4000, 0xaaa9ebd7 , 0xcc2aebc5 )
	ROM_LOAD( "to-2.bin", 0x4000, 0x4000, 0x0132ca22 , 0xf7c9f138 )
	ROM_LOAD( "to-3.bin", 0x8000, 0x4000, 0x05a337a1 , 0xa0f0a6da )

	ROM_REGION_DISPOSE(0x24000)	/* 64k for graphics (disposed after conversion) */
	ROM_LOAD( "to-12.bin", 0x00000, 0x4000, 0x6ba92983 , 0x4f44622c )	/* fg tiles */
	ROM_LOAD( "to-8.bin", 0x04000, 0x8000, 0x7f159985 , 0x363bd1ba )	/* sprites */
	ROM_LOAD( "to-7.bin", 0x0c000, 0x8000, 0x125ba6ef , 0x6583fa5b )
	ROM_LOAD( "to-11.bin", 0x14000, 0x8000, 0x3e826868 , 0x1ec00cb5 )	/* bg tiles */
	ROM_LOAD( "to-9.bin", 0x1c000, 0x8000, 0x30a3b693 , 0xa14347f0 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "to-4.bin", 0x0000, 0x8000, 0x9cafeca9 , 0xe922cbd2 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "to-6.bin", 0x0000, 0x4000, 0xa6e01ccc , 0xd8dfe1c8 )

	ROM_REGION(0x8000)	/* 64k for adpcm sounds */
	ROM_LOAD( "to-5.bin", 0x0000, 0x8000, 0x379b170b , 0xe5e4246b )
ROM_END


/* This table was recreated from the sound rom */
/* Originally it uses a OKI M51xx with counters */
ADPCM_SAMPLES_START( twc_samples )
	ADPCM_SAMPLE( 0x0000, 0x0000, (0x01f8-0x0000)*2 )
	ADPCM_SAMPLE( 0x0200, 0x0200, (0x04f8-0x0200)*2 )
	ADPCM_SAMPLE( 0x0500, 0x0500, (0x06f8-0x0500)*2 )
	ADPCM_SAMPLE( 0x0700, 0x0700, (0x08f8-0x0700)*2 )
	ADPCM_SAMPLE( 0x0900, 0x0900, (0x0af8-0x0900)*2 )
	ADPCM_SAMPLE( 0x0b00, 0x0b00, (0x0cf8-0x0b00)*2 )
	ADPCM_SAMPLE( 0x0d00, 0x0d00, (0x0ef8-0x0d00)*2 )
	ADPCM_SAMPLE( 0x0f00, 0x0f00, (0x10f8-0x0f00)*2 )
	ADPCM_SAMPLE( 0x1100, 0x1100, (0x12f8-0x1100)*2 )
	ADPCM_SAMPLE( 0x1300, 0x1300, (0x1ff8-0x1300)*2 )
	ADPCM_SAMPLE( 0x2000, 0x2000, (0x2ff8-0x2000)*2 )
ADPCM_SAMPLES_END



static int tehkanwc_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (RAM[0xc600] == 0x03)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc600],8*12);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void tehkanwc_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc600],8*12);
		osd_fclose(f);
	}

}


struct GameDriver tehkanwc_driver =
{
	__FILE__,
	0,
	"tehkanwc",
	"Tehkan World Cup",
	"1985",
	"Tehkan",
	"Ernesto Corvi\nRoberto Fresca",
	0,
	&machine_driver,

	tehkanwc_rom,
	0, 0,
	0,
	(void*)twc_samples,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tehkanwc_hiload, tehkanwc_hisave
};

struct GameDriver gridiron_driver =
{
	__FILE__,
	0,
	"gridiron",
	"Gridiron",
	"1985",
	"Tehkan",
	"Ernesto Corvi\nRoberto Fresca",
	0,
	&machine_driver,

	gridiron_rom,
	0, 0,
	0,
	(void*)twc_samples,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver teedoff_driver =
{
	__FILE__,
	0,
	"teedoff",
	"Tee'd Off",
	"1986",
	"Tecmo",
	"Ernesto Corvi\nRoberto Fresca",
	GAME_NOT_WORKING,
	&machine_driver,

	teedoff_rom,
	0, 0,
	0,
	(void*)twc_samples,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
