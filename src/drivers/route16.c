/***************************************************************************

 Route 16/Stratovox memory map (preliminary)

 Notes: Route 16 and Stratovox use identical hardware with the following
        exceptions: Stratovox has a DAC for voice.
        Route 16 has the added ability to turn off each bitplane indiviaually.
        This looks like an afterthought, as one of the same bits that control
        the palette selection is doubly utilized as the bitmap enable bit.

 CPU1

 0000-2fff ROM
 4000-43ff Shared RAM
 8000-bfff Video RAM

 I/O Read

 48xx IN0 - DIP Switches
 50xx IN1 - Input Port 1
 58xx IN2 - Input Port 2

 I/O Write

 48xx OUT0 - D0-D4 color select for VRAM 0
             D5    coin counter
 50xx OUT1 - D0-D4 color select for VRAM 1
             D5    VIDEO I/II (Flip Screen)

 I/O Port Write

 6800 AY-8910 Write Port
 6900 AY-8910 Control Port


 CPU2

 0000-1fff ROM
 4000-43ff Shared RAM
 8000-bfff Video RAM

 I/O Write

 2800      DAC output (Stratovox only)

 ***************************************************************************/

#include "driver.h"

extern unsigned char *route16_sharedram;
extern unsigned char *route16_videoram1;
extern unsigned char *route16_videoram2;

void route16_set_machine_type(void);
void stratvox_set_machine_type(void);
int  route16_vh_start(void);
void route16_vh_stop(void);
void route16_out0_w(int offset,int data);
void route16_out1_w(int offset,int data);
void route16_videoram1_w(int offset,int data);
void route16_videoram2_w(int offset,int data);
int  route16_videoram1_r(int offset);
int  route16_videoram2_r(int offset);
void route16_sharedram_w(int offset,int data);
int  route16_sharedram_r(int offset);
void route16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void stratvox_samples_w(int offset,int data);

static struct MemoryReadAddress cpu1_readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x4800, 0x4800, input_port_0_r },
	{ 0x5000, 0x5000, input_port_1_r },
	{ 0x5800, 0x5800, input_port_2_r },
	{ 0x8000, 0xbfff, route16_videoram1_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cpu1_writemem[] =
{
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_w, &route16_sharedram },
	{ 0x4800, 0x4800, route16_out0_w },
	{ 0x5000, 0x5000, route16_out1_w },
	{ 0x8000, 0xbfff, route16_videoram1_w, &route16_videoram1 },
	{ 0xc000, 0xc000, MWA_RAM }, // Stratvox has an off by one error
                                 // when clearing the screen
	{ -1 }  /* end of table */
};


static struct IOWritePort cpu1_writeport[] =
{
	{ 0x6800, 0x6800, AY8910_write_port_0_w },
	{ 0x6900, 0x6900, AY8910_control_port_0_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress cpu2_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x8000, 0xbfff, route16_videoram2_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cpu2_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2800, 0x2800, DAC_data_w }, // Not used by Route 16
	{ 0x4000, 0x43ff, route16_sharedram_w },
	{ 0x8000, 0xbfff, route16_videoram2_w, &route16_videoram2 },
	{ 0xc000, 0xc1ff, MWA_NOP }, // Route 16 sometimes writes outside of
	{ -1 }  /* end of table */   // the video ram. (Probably a bug)
};


INPUT_PORTS_START( route16_input_ports )
	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 1", IP_KEY_NONE ) // Doesn't seem to
	PORT_DIPSETTING(    0x00, "Off" )                    // be referenced
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 2", IP_KEY_NONE ) // Doesn't seem to
	PORT_DIPSETTING(    0x00, "Off" )                    // be referenced
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x18, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
//	PORT_DIPSETTING(    0x18, "2 Coins/1 Credit" ) // Same as 0x08
	PORT_DIPNAME( 0x20, 0x20, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* Input Port 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(0x00, "Off" )
	PORT_DIPSETTING(0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Input Port 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END



INPUT_PORTS_START( stratvox_input_ports )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Replenish Astronouts", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x02, "Yes" )
	PORT_DIPNAME( 0x0c, 0x00, "2 Attackers At Wave", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Astronauts Kidnapped", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Less Often" )
	PORT_DIPSETTING(    0x00, "More Often" )
	PORT_DIPNAME( 0x20, 0x20, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Voices", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END


// Route 16 and Stratovox use identical color PROMs
static unsigned char color_prom[] =
{
	/* top bitmap colors */
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	0x00,0x01,0x04,0x02,0x00,0x04,0x04,0x02,0x00,0x02,0x04,0x02,0x00,0x03,0x04,0x02,
	0x00,0x07,0x04,0x02,0x00,0x01,0x01,0x01,0x00,0x01,0x04,0x03,0x00,0x01,0x04,0x05,
	0x00,0x01,0x04,0x06,0x00,0x01,0x04,0x07,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x05,
	0x00,0x01,0x02,0x06,0x00,0x01,0x02,0x07,0x00,0x01,0x03,0x05,0x00,0x01,0x03,0x06,
	0x00,0x01,0x03,0x07,0x00,0x01,0x05,0x06,0x00,0x01,0x05,0x07,0x00,0x01,0x06,0x07,
	0x00,0x04,0x02,0x05,0x00,0x04,0x02,0x06,0x00,0x04,0x03,0x06,0x00,0x04,0x06,0x07,
	0x00,0x02,0x03,0x06,0x00,0x02,0x03,0x07,0x00,0x02,0x06,0x07,0x00,0x03,0x06,0x07,
	0x02,0x01,0x03,0x04,0x02,0x01,0x05,0x04,0x02,0x01,0x06,0x04,0x02,0x01,0x07,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/* bottom bitmap colors - same as above */
	0x00,0x01,0x04,0x02,0x00,0x04,0x04,0x02,0x00,0x02,0x04,0x02,0x00,0x03,0x04,0x02,
	0x00,0x07,0x04,0x02,0x00,0x01,0x01,0x01,0x00,0x01,0x04,0x03,0x00,0x01,0x04,0x05,
	0x00,0x01,0x04,0x06,0x00,0x01,0x04,0x07,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x05,
	0x00,0x01,0x02,0x06,0x00,0x01,0x02,0x07,0x00,0x01,0x03,0x05,0x00,0x01,0x03,0x06,
	0x00,0x01,0x03,0x07,0x00,0x01,0x05,0x06,0x00,0x01,0x05,0x07,0x00,0x01,0x06,0x07,
	0x00,0x04,0x02,0x05,0x00,0x04,0x02,0x06,0x00,0x04,0x03,0x06,0x00,0x04,0x06,0x07,
	0x00,0x02,0x03,0x06,0x00,0x02,0x03,0x07,0x00,0x02,0x06,0x07,0x00,0x03,0x06,0x07,
	0x02,0x01,0x03,0x04,0x02,0x01,0x05,0x04,0x02,0x01,0x06,0x04,0x02,0x01,0x07,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	10000000/8,     /* 10Mhz / 8 = 1.25Mhz */
	{ 255 },
	{ 0 },
	{ 0 },
	{ stratvox_samples_w },  /* SN76477 commands (not used in Route 16?) */
	{ 0 }
};


static struct DACinterface dac_interface =
{
	1,
	441000,
	{ 255 },
	{ 1 }
};


static struct Samplesinterface samples_interface =
{
	1	/* 1 channel */
};


#define MACHINE_DRIVER(GAMENAME, AUDIO_INTERFACES)   		\
															\
static struct MachineDriver GAMENAME##_machine_driver =		\
{															\
	/* basic machine hardware */							\
	{														\
		{													\
			CPU_Z80 | CPU_16BIT_PORT,						\
			2500000,	/* 10Mhz / 4 = 2.5Mhz */			\
			0,												\
			cpu1_readmem,cpu1_writemem,0,cpu1_writeport,	\
			interrupt,1										\
		},													\
		{													\
			CPU_Z80,										\
			2500000,	/* 10Mhz / 4 = 2.5Mhz */			\
			2,												\
			cpu2_readmem,cpu2_writemem,0,0,					\
			ignore_interrupt,0								\
		}													\
	},														\
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */ \
	1,														\
	0,														\
															\
	/* video hardware */									\
	256, 256, { 8, 256-8-1, 0, 256-1 },						\
	0,														\
	8, 0,													\
	0,														\
															\
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE, \
	0,														\
	route16_vh_start,										\
	route16_vh_stop,										\
	route16_vh_screenrefresh,								\
															\
	/* sound hardware */									\
	0,0,0,0,												\
	{														\
		AUDIO_INTERFACES									\
	}														\
};

#define ROUTE16_AUDIO_INTERFACE  \
		{						 \
			SOUND_AY8910,		 \
			&ay8910_interface	 \
		}

#define STRATVOX_AUDIO_INTERFACE \
		{						 \
			SOUND_AY8910,		 \
			&ay8910_interface	 \
		},						 \
		{						 \
			SOUND_DAC,			 \
			&dac_interface		 \
		},						 \
		{						 \
			SOUND_SAMPLES,		 \
			&samples_interface	 \
		}

MACHINE_DRIVER(route16,  ROUTE16_AUDIO_INTERFACE )
MACHINE_DRIVER(stratvox, STRATVOX_AUDIO_INTERFACE)

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( route16_rom )
	ROM_REGION(0x10000)  // 64k for the first CPU
	ROM_LOAD( "rt16.0", 0x0000, 0x0800, 0xe997d36b )
	ROM_LOAD( "rt16.1", 0x0800, 0x0800, 0x46fdd75f )
	ROM_LOAD( "rt16.2", 0x1000, 0x0800, 0xfa13051d )
	ROM_LOAD( "rt16.3", 0x1800, 0x0800, 0x459e3926 )
	ROM_LOAD( "rt16.4", 0x2000, 0x0800, 0x8b700e7a )
	ROM_LOAD( "rt16.5", 0x2800, 0x0800, 0x629f0ab7 )

	ROM_REGION(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)  // 64k for the second CPU
	ROM_LOAD( "rt16.6", 0x0000, 0x0800, 0x534957bd )
	ROM_LOAD( "rt16.7", 0x0800, 0x0800, 0xa48435cc )
	ROM_LOAD( "rt16.8", 0x1000, 0x0800, 0x3ad22b58 )
	ROM_LOAD( "rt16.9", 0x1800, 0x0800, 0x13f7b5ab )
ROM_END


static const char *stratvox_sample_names[] =
{
	"*stratvox",
	"explode.sam", // Sample played when player's ship is exploding
	"bonus.sam",   // Sample played when reached 5000 pts and bonus ship
                   // is awarded
    0   /* end of array */
};

ROM_START( stratvox_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ls01.bin", 0x0000, 0x0800, 0x2128ac32 )
	ROM_LOAD( "ls02.bin", 0x0800, 0x0800, 0x7ff83536 )
	ROM_LOAD( "ls03.bin", 0x1000, 0x0800, 0x39532473 )
	ROM_LOAD( "ls04.bin", 0x1800, 0x0800, 0x126f1c33 )
	ROM_LOAD( "ls05.bin", 0x2000, 0x0800, 0x771da163 )
	ROM_LOAD( "ls06.bin", 0x2800, 0x0800, 0xd007d1fd )

	ROM_REGION(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the second CPU */
	ROM_LOAD( "ls07.bin", 0x0000, 0x0800, 0x759bff3d )
	ROM_LOAD( "ls08.bin", 0x0800, 0x0800, 0x74dd1ce5 )
ROM_END


static int route16_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x4028],"\x01",1) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4032],9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void route16_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4032],9);
		osd_fclose(f);
	}
}


static int stratvox_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x410f],"\x0f",1) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4010],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void stratvox_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4010],3);
		osd_fclose(f);
	}
}


struct GameDriver route16_driver =
{
	__FILE__,
	0,
	"route16",
	"Route 16",
	"1981",
	"bootleg",
	"Zsolt Vasvari\nMike Balfour",
	0,
	&route16_machine_driver,

	route16_rom,
	route16_set_machine_type, 0,
	0,
	0,	/* sound_prom */

	route16_input_ports,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	route16_hiload, route16_hisave
};

struct GameDriver stratvox_driver =
{
	__FILE__,
	0,
	"stratvox",
	"Stratovox",
	"1980",
	"Taito",
	"Darren Olafson\nZsolt Vasvari\nMike Balfour",
	0,
	&stratvox_machine_driver,

	stratvox_rom,
	stratvox_set_machine_type, 0,
	stratvox_sample_names,
	0,	/* sound_prom */

	stratvox_input_ports,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	stratvox_hiload, stratvox_hisave
};
