/*
old SNK games (1983-1985):

- Marvin's Maze
- Vanguard II
- Mad Crasher
- HAL21

3xZ80
Sound: PSGX2 + "Wave Generater"

Remaining issues:
- unknown (constant) video registers (probably scrolling)
- "wave generater" isn't hooked up (it supplies additional sounds)
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

#define CREDITS "Phil Stroffolino\nCarlos A. Lozano"

extern void aso_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

extern unsigned char *marvins_videoram1;
extern unsigned char *marvins_videoram2;
extern unsigned char *marvins_videoram3;
extern void marvins_videoram1_w( int, int );
extern void marvins_videoram2_w( int, int );
extern void marvins_videoram3_w( int, int );

extern void marvins_sprite_layer_w( int, int data );
extern void marvins_bg_color_w( int, int data );

/* unmapped video registers */
extern void marvins_videoreg0_w( int, int data );
extern void marvins_videoreg1_w( int, int data );
extern void marvins_videoreg2_w( int, int data );
extern void marvins_videoreg3_w( int, int data );
extern void marvins_videoreg4_w( int, int data );
extern void marvins_videoreg5_w( int, int data );
extern void marvins_videoreg6_w( int, int data );

extern int marvins_vh_start( void );
extern void marvins_vh_stop( void );
extern void marvins_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );

#define SNK_NMI_ENABLE	1
#define SNK_NMI_PENDING	2
static int cpuA_latch = 0;
static int cpuB_latch = 0;

#define SOUND_READY		0
#define SOUND_BUSY		1
static int sound_cpu_status = SOUND_READY;
static int sound_nmi_enable = 0;
static int sound_command = 0x00;

static void cpuA_int_enable( int offset, int data){
	if( cpuA_latch & SNK_NMI_PENDING ){
		cpu_cause_interrupt( 0, Z80_NMI_INT );
		cpuA_latch = 0;
	}
	else {
		cpuA_latch |= SNK_NMI_ENABLE;
	}
}

static int cpuA_int_trigger( int offset ){
	if( cpuA_latch&SNK_NMI_ENABLE ){
		cpu_cause_interrupt( 0, Z80_NMI_INT );
		cpuA_latch = 0;
	}
	else {
		cpuA_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static void cpuB_int_enable( int offset, int data ){
	if( cpuB_latch & SNK_NMI_PENDING ){
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		cpuB_latch = 0;
	}
	else {
		cpuB_latch |= SNK_NMI_ENABLE;
	}
}

static int cpuB_int_trigger( int offset ){
	if( cpuB_latch&SNK_NMI_ENABLE ){
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		cpuB_latch = 0;
	}
	else {
		cpuB_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static void sound_command_w( int offset, int data ){
	sound_command = data;
	sound_cpu_status = SOUND_BUSY;
}

static int sound_command_r( int offset ){
	int data = sound_command;
	sound_cpu_status = SOUND_READY; /* ? */
	sound_command = 0;
	return data;
}

static int sound_ack_r( int offset ){
	sound_nmi_enable = 1; /* ? */
	return 0xff;
}

int sound_interrupt( void ){
	static int count = 0;
	count = 1-count;
	if( count ) return nmi_interrupt();
	if( sound_command ) return interrupt();

//	if( sound_nmi_enable ){
//		sound_nmi_enable = 0;
//		return nmi_interrupt();
//	}
	return ignore_interrupt();
}

static struct IOReadPort readport_sound[] = {
//	{ 0x00, 0x00, MRA_NOP }, /* unknown */
	{ -1 }
};

static struct MemoryReadAddress readmem_sound[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4000, sound_command_r },
	{ 0xa000, 0xa000, sound_ack_r },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },

	// "wave" table generater(s)?
	{ 0x8002, 0x8002, MWA_NOP },
	{ 0x8003, 0x8003, MWA_NOP },
	{ 0x8004, 0x8004, MWA_NOP },
	{ 0x8005, 0x8005, MWA_NOP },
	{ 0x8006, 0x8006, MWA_NOP },
	{ 0x8007, 0x8007, MWA_NOP },

	{ 0x8008, 0x8008, AY8910_control_port_1_w },
	{ 0x8009, 0x8009, AY8910_write_port_1_w },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ -1 }
};

static int marvins_port_0_r( int offset ){
	int result = input_port_0_r( 0 );
	if( sound_cpu_status != SOUND_READY ) result |= 0x40;
	return result;
}

static struct MemoryReadAddress readmem_cpuA[] = {
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x8000, marvins_port_0_r }, /* coin input, start, sound CPU status */
	{ 0x8100, 0x8100, input_port_1_r }, /* player #1 controls */
	{ 0x8200, 0x8200, input_port_2_r }, /* player #2 controls */
	{ 0x8400, 0x8400, input_port_3_r }, /* dipswitch#1 */
	{ 0x8500, 0x8500, input_port_4_r }, /* dipswitch#2 */
	{ 0x8700, 0x8700, cpuB_int_trigger },

	{ 0xc000, 0xf7ff, MRA_RAM, &spriteram },
	{ -1 }
};

static struct MemoryWriteAddress writemem_cpuA[] = {
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6000, marvins_bg_color_w },
	{ 0x8300, 0x8300, sound_command_w },
	{ 0x8600, 0x8600, MWA_NOP }, /* video reg; text layer color select? */
	{ 0x8700, 0x8700, cpuA_int_enable },

	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, marvins_videoram1_w, &marvins_videoram1 }, /* BACK1 VRAM */
	{ 0xd400, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe3ff, marvins_videoram2_w, &marvins_videoram2 }, /* BACK2 VRAM */
	{ 0xe400, 0xefff, MWA_RAM },
	{ 0xf000, 0xf3ff, marvins_videoram3_w, &marvins_videoram3 }, /* SIDE  VRAM */
	{ 0xf400, 0xf7ff, MWA_RAM },

	{ 0xf800, 0xf800, marvins_videoreg0_w }, /* sprite scroll x? */
	{ 0xf900, 0xf900, marvins_videoreg1_w }, /* sprite scroll y? */
	{ 0xfa00, 0xfa00, marvins_videoreg2_w }, /* bg scroll x? */
	{ 0xfb00, 0xfb00, marvins_videoreg3_w }, /* bg scroll y? */
	{ 0xfc00, 0xfc00, marvins_videoreg4_w }, /* fg scroll x? */
	{ 0xfd00, 0xfd00, marvins_videoreg5_w }, /* fg scroll y? */
	{ 0xfe00, 0xfe00, marvins_sprite_layer_w },
	{ 0xff00, 0xff00, marvins_videoreg6_w }, /* ? */
	{ -1 }
};

static int sharedram_r( int offset ){
	return spriteram[offset];
}

static void sharedram_w( int offset, int data ){
	spriteram[offset] = data;
}

static struct MemoryReadAddress readmem_cpuB[] = {
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8700, 0x8700, cpuA_int_trigger },
	{ 0xc000, 0xf7ff, sharedram_r  },
	{ -1 }
};
static struct MemoryWriteAddress writemem_cpuB[] = {
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8700, 0x8700, cpuB_int_enable },
	{ 0xd000, 0xd3ff, marvins_videoram1_w }, /* BACK1 VRAM */
	{ 0xe000, 0xe3ff, marvins_videoram2_w }, /* BACK2 VRAM */
	{ 0xf000, 0xf3ff, marvins_videoram3_w }, /* SIDE  VRAM */
	{ 0xc000, 0xf7ff, sharedram_w  },
	{ 0xfe00, 0xfe00, marvins_sprite_layer_w },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	2000000,	/* 2 MHz */
	{ 50, 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct GfxLayout sprite_layout = {
	16,16,
	0x200,
	3,
	{ 0,0x4000*8,0x8000*8 },
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	256
};

static struct GfxLayout tile_layout = {
	8,8,
	0x100,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxDecodeInfo marvins_gfxdecodeinfo[] = {
	{ 3,      0x0000, &tile_layout,	    	24*16, 8  }, /* 384..511 */
	{ 4,      0x0000, &tile_layout,	    	16*16, 8  }, /* 256..383 */
	{ 5,      0x0000, &tile_layout,	    	 8*16, 8  }, /* 128..255 */
	{ 6,      0x0000, &sprite_layout,		 0*16, 16 }, /*   0..127 */
	{ -1 }
};

static struct MachineDriver marvins_machine_driver = {
	{
		{
			CPU_Z80,
			3360000,	/* 3.336 Mhz */
			0,
			readmem_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			3360000,	/* 3.336 Mhz */
			1,
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4.0 Mhz */
			2,
			readmem_sound,writemem_sound,readport_sound,0,
			sound_interrupt,4
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init_machine */

	/* video hardware */
	256+32, 224, { 0, 255+32,0, 223 },
	marvins_gfxdecodeinfo,
	1024,1024,
	aso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	marvins_vh_start,
	marvins_vh_stop,
	marvins_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/***********************************************************************/

ROM_START( marvins_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "pa1",   0x0000, 0x2000, 0x0008d791 )
	ROM_LOAD( "pa2",   0x2000, 0x2000, 0x9457003c )
	ROM_LOAD( "pa3",   0x4000, 0x2000, 0x54c33ecb )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "pb1",	0x0000, 0x2000, 0x3b6941a5 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "m1",    0x0000, 0x2000, 0x2314c696 )
	ROM_LOAD( "m2",    0x2000, 0x2000, 0x74ba5799 )

	ROM_REGION_DISPOSE( 0x2000 ) /* text characters */
	ROM_LOAD( "s1",		0x0000, 0x2000, 0x327f70f3 )
	ROM_REGION_DISPOSE( 0x2000 ) /* background tiles */
	ROM_LOAD( "b1",		0x0000, 0x2000, 0xe528bc60 )
	ROM_REGION_DISPOSE( 0x2000 ) /* foreground tiles */
	ROM_LOAD( "b2",		0x0000, 0x2000, 0xe528bc60 )
	ROM_REGION_DISPOSE( 0xc000 ) /* sprites */
	ROM_LOAD( "f1",		0x8000, 0x2000, 0x0bd6b4e5 )
	ROM_LOAD( "f2",		0x4000, 0x2000, 0x8fc2b081 )
	ROM_LOAD( "f3",		0x0000, 0x2000, 0xe55c9b83 )

	ROM_REGION(0xC00)	/* color PROMs */
	ROM_LOAD( "marvmaze.j1",  0x000, 0x400, 0x92f5b06d )
	ROM_LOAD( "marvmaze.j2",  0x400, 0x400, 0xd2b25665 )
	ROM_LOAD( "marvmaze.j3",  0x800, 0x400, 0xdf9e6005 )
ROM_END

ROM_START( madcrash_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "p8",    0x0000, 0x2000, 0xecb2fdc9 )
	ROM_LOAD( "p9",    0x2000, 0x2000, 0x0a87df26 )
	ROM_LOAD( "p10",   0x4000, 0x2000, 0x6eb8a87c )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "p4",   0x0000, 0x2000, 0x5664d699 )
	ROM_LOAD( "p5",   0x2000, 0x2000, 0xdea2865a )
	ROM_LOAD( "p6",   0x4000, 0x2000, 0xe25a9b9c )
	ROM_LOAD( "p7",   0x6000, 0x2000, 0x55b14a36 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "p1",   0x0000, 0x2000, 0x2dcd036d )
	ROM_LOAD( "p2",   0x2000, 0x2000, 0xcc30ae8b )
	ROM_LOAD( "p3",   0x4000, 0x2000, 0xe3c8c2cb )

	ROM_REGION_DISPOSE( 0x2000 ) /* characters */
	ROM_LOAD( "p13",    0x0000, 0x2000, 0x48c4ade0 )
	ROM_REGION_DISPOSE( 0x2000 ) /* tiles */
	ROM_LOAD( "p11",	0x0000, 0x2000, 0x67174956 )
	ROM_REGION_DISPOSE( 0x2000 ) /* tiles */
	ROM_LOAD( "p12",	0x0000, 0x2000, 0x085094c1 )
	ROM_REGION_DISPOSE( 0xc000 ) /* 16x16 sprites */
	ROM_LOAD( "p14",	0x0000, 0x2000, 0x07e807bc )
	ROM_LOAD( "p15",	0x4000, 0x2000, 0xa74149d4 )
	ROM_LOAD( "p16",	0x8000, 0x2000, 0x6153611a )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "m1-prom.j5",  0x000, 0x400, 0x07678443 )
	ROM_LOAD( "m2-prom.j4",  0x400, 0x400, 0x9fc325af )
	ROM_LOAD( "m3-prom.j3",  0x800, 0x400, 0xd19e8a91 )
ROM_END

ROM_START( vangrd2_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "p1.9a",  0x0000, 0x2000, 0xbc9eeca5 )
	ROM_LOAD( "p2.12a", 0x2000, 0x2000, 0x58b08b58 )
	ROM_LOAD( "p3.11a", 0x4000, 0x2000, 0x3970f69d )
	ROM_LOAD( "p4.14a", 0x6000, 0x2000, 0xa95f11ea )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "p5.4a", 0x0000, 0x2000, 0xe4dfd0ba )
	ROM_LOAD( "p6.6a", 0x2000, 0x2000, 0x894ff00d )
	ROM_LOAD( "p7.7a", 0x4000, 0x2000, 0x40b4d069 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "p8.6a", 0x0000, 0x2000, 0xa3daa438 )
	ROM_LOAD( "p9.8a", 0x2000, 0x2000, 0x9345101a )

	ROM_REGION_DISPOSE( 0x2000 )	/* characters */
	ROM_LOAD( "p15.1e", 0x0000, 0x2000, 0x85718a41 )
	ROM_REGION_DISPOSE( 0x2000 ) /* tiles */
	ROM_LOAD( "p13.1a", 0x0000, 0x2000, 0x912f22c6 )
	ROM_REGION_DISPOSE( 0x2000 ) /* tiles */

	ROM_REGION_DISPOSE( 0xc000 ) /* 16x16 sprites */
	ROM_LOAD( "p10.4kl", 0x0000, 0x2000, 0x5bfc04c0 )
	ROM_LOAD( "p11.3kl", 0x4000, 0x2000, 0x620cd4ec )
	ROM_LOAD( "p12.1kl", 0x8000, 0x2000, 0x8658ea6c )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "mb7054.3j",	0x000, 0x00400, 0x506f659a )
	ROM_LOAD( "mb7054.4j",	0x400, 0x00400, 0x222133ce )
	ROM_LOAD( "mb7054.5j",	0x800, 0x00400, 0x2e21a79b )
ROM_END

ROM_START( hal21_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "hal21p1.bin",    0x0000, 0x2000, 0x9d193830 )
	ROM_LOAD( "hal21p2.bin",    0x2000, 0x2000, 0xc1f00350 )
	ROM_LOAD( "hal21p3.bin",    0x4000, 0x2000, 0x881d22a6 )
	ROM_LOAD( "hal21p4.bin",    0x6000, 0x2000, 0xce692534 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "hal21p5.bin",    0x0000, 0x2000, 0x3ce0684a )
	ROM_LOAD( "hal21p6.bin",    0x2000, 0x2000, 0x878ef798 )
	ROM_LOAD( "hal21p7.bin",    0x4000, 0x2000, 0x72ebbe95 )
	ROM_LOAD( "hal21p8.bin",    0x6000, 0x2000, 0x17e22ad3 )
	ROM_LOAD( "hal21p9.bin",    0x8000, 0x2000, 0xb146f891 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "hal21p10.bin",   0x0000, 0x4000, 0x916f7ba0 )

	ROM_REGION_DISPOSE( 0x2000 ) /* characters */
	ROM_LOAD( "hal21p12.bin",   0x0000, 0x2000, 0x9839a7cd )
	ROM_REGION_DISPOSE( 0x4000 ) /* tiles */
	ROM_LOAD( "hal21p11.bin",	0x0000, 0x4000, 0x24abc57e )
	ROM_REGION_DISPOSE( 0xc000 ) /* 16x16 sprites */
	ROM_LOAD( "hal21p13.bin",  0x0000, 0x4000, 0x052b4f4f )
	ROM_LOAD( "hal21p14.bin",  0x4000, 0x4000, 0xda0cb670 )
	ROM_LOAD( "hal21p15.bin",  0x8000, 0x4000, 0x5c5ea945 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "marvmaze.j1",  0x000, 0x400, 0 ) //fake
	ROM_LOAD( "marvmaze.j2",  0x400, 0x400, 0 ) //fake
	ROM_LOAD( "marvmaze.j3",  0x800, 0x400, 0 ) //fake
ROM_END

/*******************************************************************************************/

INPUT_PORTS_START( marvins_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,	IPT_UNKNOWN )

	PORT_START /* player#1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* not used in Marvin's Maze */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* player#2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* not used in Marvin's Maze */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Infinite Lives" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START	/* DSW2 (unverified) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x07, "10000" )
	PORT_DIPSETTING(    0x06, "20000" )
	PORT_DIPSETTING(    0x05, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x01, "70000" )
	PORT_DIPSETTING(    0x00, "80000" )
	PORT_DIPNAME( 0x18, 0x18, "2nd Bonus" )
	PORT_DIPSETTING(    0x18, "unused" )
	PORT_DIPSETTING(    0x10, "1st bonus*2" )
	PORT_DIPSETTING(    0x08, "1st bonus*3" )
	PORT_DIPSETTING(    0x00, "1st bonus*4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

/*
INPUT_PORTS_START( hal21_input_ports )
	PORT_START	// DSW1
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )

	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, DEF_STR( "3" ) )
	PORT_DIPSETTING(    0x04, DEF_STR( "5" ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x28, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Unknown ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 60000" )
	PORT_DIPSETTING(    0x40, "40000 90000" )
	PORT_DIPNAME( 0x80, 0x80, "50000 120000" )
	PORT_DIPSETTING(    0xc0, "None" )

	PORT_START	// DSW2
	PORT_DIPNAME( 0x01, 0x01, "Bonus Type" )
	PORT_DIPSETTING(    0x00, "Every Bonus Set" )
	PORT_DIPSETTING(    0x01, "Second Bonus Set" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x06, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Freeze Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
INPUT_PORTS_END
*/

struct GameDriver marvins_driver =
{
	__FILE__,
	0,
	"marvins",
	"Marvin's Maze",
	"1983",
	"SNK",
	CREDITS,
	0,
	&marvins_machine_driver,
	0,
	marvins_rom,
	0,0,0,0,
	marvins_input_ports,
	PROM_MEMORY_REGION(7), 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};


struct GameDriver madcrash_driver =
{
	__FILE__,
	0,
	"madcrash",
	"Mad Crasher",
	"????",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&marvins_machine_driver,
	0,
	madcrash_rom,
	0,0,0,0,
	marvins_input_ports,
	PROM_MEMORY_REGION(7), 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver vangrd2_driver =
{
	__FILE__,
	0,
	"vangrd2",
	"Vanguard II",
	"1984",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&marvins_machine_driver,
	0,
	vangrd2_rom,
	0,0,0,0,
	marvins_input_ports,
	PROM_MEMORY_REGION(7), 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};

struct GameDriver hal21_driver =
{
	__FILE__,
	0,
	"hal21",
	"HAL21",
	"1985",
	"SNK",
	CREDITS,
	GAME_NO_SOUND,
	&marvins_machine_driver,
	0,
	hal21_rom,
	0,0,0,0,
	marvins_input_ports,
	PROM_MEMORY_REGION(7), 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};
