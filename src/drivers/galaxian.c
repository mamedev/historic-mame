/***************************************************************************

Galaxian memory map

Compiled from information provided by friends and Uncles on RGVAC.

            AAAAAA
            111111AAAAAAAAAA     DDDDDDDD   Schem   function
HEX         5432109876543210 R/W 76543210   name

0000-27FF                                           Game ROM
5000-57FF   01010AAAAAAAAAAA R/W DDDDDDDD   !Vram   Character ram
5800-583F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Screen attributes
5840-585F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Sprites
5860-5FFF   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Bullets

6000        0110000000000000 R   -------D   !SW0    coin1
6000        0110000000000000 R   ------D-   !SW0    coin2
6000        0110000000000000 R   -----D--   !SW0    p1 left
6000        0110000000000000 R   ----D---   !SW0    p1 right
6000        0110000000000000 R   ---D----   !SW0    p1shoot
6000        0110000000000000 R   --D-----   !SW0    table ??
6000        0110000000000000 R   -D------   !SW0    test
6000        0110000000000000 R   D-------   !SW0    service

6000        0110000000000001 W   -------D   !DRIVER lamp 1 ??
6001        0110000000000001 W   -------D   !DRIVER lamp 2 ??
6002        0110000000000010 W   -------D   !DRIVER lamp 3 ??
6003        0110000000000011 W   -------D   !DRIVER coin control
6004        0110000000000100 W   -------D   !DRIVER Background lfo freq bit0
6005        0110000000000101 W   -------D   !DRIVER Background lfo freq bit1
6006        0110000000000110 W   -------D   !DRIVER Background lfo freq bit2
6007        0110000000000111 W   -------D   !DRIVER Background lfo freq bit3

6800        0110100000000000 R   -------D   !SW1    1p start
6800        0110100000000000 R   ------D-   !SW1    2p start
6800        0110100000000000 R   -----D--   !SW1    p2 left
6800        0110100000000000 R   ----D---   !SW1    p2 right
6800        0110100000000000 R   ---D----   !SW1    p2 shoot
6800        0110100000000000 R   --D-----   !SW1    no used
6800        0110100000000000 R   -D------   !SW1    dip sw1
6800        0110100000000000 R   D-------   !SW1    dip sw2

6800        0110100000000000 W   -------D   !SOUND  reset background F1
                                                    (1=reset ?)
6801        0110100000000001 W   -------D   !SOUND  reset background F2
6802        0110100000000010 W   -------D   !SOUND  reset background F3
6803        0110100000000011 W   -------D   !SOUND  Noise on/off
6804        0110100000000100 W   -------D   !SOUND  not used
6805        0110100000000101 W   -------D   !SOUND  shoot on/off
6806        0110100000000110 W   -------D   !SOUND  Vol of f1
6807        0110100000000111 W   -------D   !SOUND  Vol of f2

7000        0111000000000000 R   -------D   !DIPSW  dip sw 3
7000        0111000000000000 R   ------D-   !DIPSW  dip sw 4
7000        0111000000000000 R   -----D--   !DIPSW  dip sw 5
7000        0111000000000000 R   ----D---   !DIPSW  dip s2 6

7001        0111000000000001 W   -------D   9Nregen NMIon
7004        0111000000000100 W   -------D   9Nregen stars on
7006        0111000000000110 W   -------D   9Nregen hflip
7007        0111000000000111 W   -------D   9Nregen vflip

Note: 9n reg,other bits  used on moon cresta for extra graphics rom control.

7800        0111100000000000 R   --------   !wdr    watchdog reset
7800        0111100000000000 W   DDDDDDDD   !pitch  Sound Fx base frequency

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
void pisces_gfxbank_w(int offset,int data);
int galaxian_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap);
int galaxian_vh_interrupt(void);

void mooncrst_sound_freq_w(int offset,int data);
void mooncrst_noise_w(int offset,int data);
void mooncrst_background_w(int offset,int data);
void mooncrst_shoot_w(int offset,int data);
void mooncrst_lfo_freq_w(int offset,int data);
void mooncrst_sound_freq_sel_w(int offset,int data);
int mooncrst_sh_init(const char *gamename);
int mooncrst_sh_start(void);
void mooncrst_sh_stop(void);
void mooncrst_sh_update(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x3fff, MRA_ROM },	/* not all games use all the space */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, MRA_NOP },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress galaxian_writemem[] =
{
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x5880, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_sound_freq_w },
	{ 0x6800, 0x6800, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
        { 0x6806, 0x6807, mooncrst_sound_freq_sel_w },
        { 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x0000, 0x27ff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pisces_writemem[] =
{
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x5880, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_sound_freq_w },
	{ 0x6800, 0x6800, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
        { 0x6806, 0x6807, mooncrst_sound_freq_sel_w },
        { 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x6002, 0x6002, pisces_gfxbank_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x0000, 0x3fff, MWA_ROM },	/* not all games use all the space */
	{ -1 }	/* end of table */
};



static struct InputPort galaxian_input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, OSD_KEY_3, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, OSD_KEY_F2, 0 },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE LEFT"  },
        { 0, 3, "MOVE RIGHT" },
        { 0, 4, "FIRE"       },
        { -1 }
};


static struct InputPort warofbug_input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_3, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, OSD_KEY_DOWN, OSD_KEY_UP },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE, 0, OSD_JOY_DOWN, OSD_JOY_UP }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet wbug_keys[] =
{
        { 0, 6, "MOVE UP" },
        { 0, 2, "MOVE LEFT"  },
        { 0, 3, "MOVE RIGHT" },
        { 0, 7, "MOVE DOWN" },
        { 0, 4, "FIRE"      },
        { -1 }
};


static struct DSW galaxian_dsw[] =
{
	{ 2, 0x04, "LIVES", { "2", "3" } },
	{ 2, 0x03, "BONUS", { "7000", "10000", "12000", "20000" } },
	{ -1 }
};



static struct DSW galboot_dsw[] =
{
	{ 2, 0x04, "LIVES", { "3", "5" } },
	{ 2, 0x03, "BONUS", { "7000", "10000", "12000", "20000" } },
	{ -1 }
};



static struct DSW pisces_dsw[] =
{
	{ 1, 0x40, "LIVES", { "3", "4" } },
	{ 1, 0x80, "SW2", { "OFF", "ON" } },
	{ 2, 0x01, "SW3", { "OFF", "ON" } },
	{ 2, 0x04, "SW5", { "OFF", "ON" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct DSW japirem_dsw[] =
{
	{ 2, 0x04, "LIVES", { "3", "5" } },
	{ 2, 0x03, "BONUS", { "NONE", "4000", "5000", "7000" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct DSW warofbug_dsw[] =
{
	{ 2, 0x03, "LIVES", { "1", "2", "3", "4" } },
	{ 2, 0x04, "SW5", { "OFF", "ON" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout galaxian_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout galaxian_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout pisces_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout pisces_spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	3,1,	/* 3*1 line */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2 },	/* I "know" that this bit is 1 */
	{ 0 },	/* I "know" that this bit is 1 */
	0	/* no use */
};



static struct GfxDecodeInfo galaxian_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &galaxian_charlayout,    0,  8 },
	{ 1, 0x0000, &galaxian_spritelayout,  0,  8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo pisces_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pisces_charlayout,    0,  8 },
	{ 1, 0x0000, &pisces_spritelayout,  0,  8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};



static unsigned char galaxian_color_prom[] =
{
	/* palette */
	0x00,0x00,0x00,0xF6,0x00,0x16,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC0,0xA0,0x0C,0x00,0x00,0x00,0x07,0x00,0xF6,0x07,0xF0,0x00,0x76,0x07,0xC6
};



static unsigned char japirem_color_prom[] =
{
	/* palette */
	0x00,0x7A,0x36,0x07,0x00,0xF0,0x38,0x1F,0x00,0xC7,0xF0,0x3F,0x00,0xDB,0xC6,0x38,
	0x00,0x36,0x07,0xF0,0x00,0x33,0x3F,0xDB,0x00,0x3F,0x57,0xC6,0x00,0xC6,0x3F,0xFF
};



static unsigned char uniwars_color_prom[] =
{
	/* palette */
	0x00,0xe8,0x17,0x3f,0x00,0x2f,0x87,0x20,0x00,0xff,0x3f,0x38,0x00,0x83,0x3f,0x06,
	0x00,0xdc,0x1f,0xd0,0x00,0xef,0x20,0x96,0x00,0x3f,0x17,0xf0,0x00,0x3f,0x17,0x14
};

static unsigned char samples[32*2] =
{
   0x88, 0x88, 0x88, 0x88, 0xaa, 0xaa, 0xaa, 0xaa,
   0xcc, 0xcc, 0xcc, 0xcc, 0xee, 0xee, 0xee, 0xee,
   0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
   0x44, 0x44, 0x44, 0x44, 0x66, 0x66, 0x66, 0x66,
   0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
   0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
   0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
   0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc
};

static struct MachineDriver galaxian_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,galaxian_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	samples,
	mooncrst_sh_init,
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};



static struct MachineDriver pisces_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,pisces_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	pisces_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	samples,
	mooncrst_sh_init,
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};

static const char *mooncrst_sample_names[] =
{
	"shot.sam",
        "death.sam",
        "backgrnd.sam",
	0	/* end of array */
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galaxian_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7f", 0x0000, 0x1000, 0xa9d897cc )
	ROM_LOAD( "7j", 0x1000, 0x1000, 0x1b7269ca )
	ROM_LOAD( "7l", 0x2000, 0x1000, 0x3ec2aec6 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h", 0x0000, 0x0800, 0x4852a7c2 )
	ROM_LOAD( "1k", 0x0800, 0x0800, 0x17902ece )
ROM_END

ROM_START( galmidw_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galaxian.u",  0x0000, 0x0800, 0x168a654c )
	ROM_LOAD( "galaxian.v",  0x0800, 0x0800, 0x934ef280 )
	ROM_LOAD( "galaxian.w",  0x1000, 0x0800, 0x587af4d8 )
	ROM_LOAD( "galaxian.y",  0x1800, 0x0800, 0xc2f89d12 )
	ROM_LOAD( "galaxian.z",  0x2000, 0x0800, 0x9471bfe9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galaxian.1j", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "galaxian.1k", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galnamco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galaxian.u",  0x0000, 0x0800, 0xa9f9434b )
	ROM_LOAD( "galaxian.v",  0x0800, 0x0800, 0x1fd66534 )
	ROM_LOAD( "galaxian.w",  0x1000, 0x0800, 0xde73ca2f )
	ROM_LOAD( "galaxian.y",  0x1800, 0x0800, 0x3bddfc4b )
	ROM_LOAD( "galaxian.z",  0x2000, 0x0800, 0x98f4d194 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galaxian.1h", 0x0000, 0x0800, 0x4852a7c2 )
	ROM_LOAD( "galaxian.1k", 0x0800, 0x0800, 0x17902ece )
ROM_END

ROM_START( superg_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galaxian.u",  0x0000, 0x0800, 0xaa7b3253 )
	ROM_LOAD( "galaxian.v",  0x0800, 0x0800, 0xb2c47640 )
	ROM_LOAD( "galaxian.w",  0x1000, 0x0800, 0x2afb5745 )
	ROM_LOAD( "galaxian.y",  0x1800, 0x0800, 0xb6749510 )
	ROM_LOAD( "galaxian.z",  0x2000, 0x0800, 0xd16558c9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galaxian.1h", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "galaxian.1k", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galapx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx.u",  0x0000, 0x0800, 0x57b8a2b6 )
	ROM_LOAD( "galx.v",  0x0800, 0x0800, 0xa08d337b )
	ROM_LOAD( "galx.w",  0x1000, 0x0800, 0x2865868b )
	ROM_LOAD( "galx.y",  0x1800, 0x0800, 0xac089510 )
	ROM_LOAD( "galx.z",  0x2000, 0x0800, 0x6de3d409 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx.1h", 0x0000, 0x0800, 0xea88446e )
	ROM_LOAD( "galx.1k", 0x0800, 0x0800, 0x4aeef848 )
ROM_END

ROM_START( galap1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx_1.rom",   0x0000, 0x2800, 0x9e96085a )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx_1c1.rom", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "galx_1c2.rom", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galap4_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx_4.rom",   0x0000, 0x2800, 0x7d13c18f )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx_4c1.rom", 0x0000, 0x0800, 0xe3934181 )
	ROM_LOAD( "galx_4c2.rom", 0x0800, 0x0800, 0x8cf8cc7c )
ROM_END

ROM_START( galturbo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galaxian.u",  0x0000, 0x0800, 0xaa7b3253 )
	ROM_LOAD( "galaxian.v",  0x0800, 0x0800, 0xa08d337b )
	ROM_LOAD( "galaxian.w",  0x1000, 0x0800, 0x2afb5745 )
	ROM_LOAD( "galaxian.y",  0x1800, 0x0800, 0x9574b410 )
	ROM_LOAD( "galaxian.z",  0x2000, 0x0800, 0xd525c4cb )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galaxian.1h", 0x0000, 0x0800, 0xb545ede3 )
	ROM_LOAD( "galaxian.1k", 0x0800, 0x0800, 0xcfbf64ef )
ROM_END

ROM_START( pisces_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pisces.a1", 0x0000, 0x0800, 0xe94d1451 )
	ROM_LOAD( "pisces.a2", 0x0800, 0x0800, 0x79b828ae )
	ROM_LOAD( "pisces.b2", 0x1000, 0x0800, 0x94a55e7d )
	ROM_LOAD( "pisces.c1", 0x1800, 0x0800, 0xc859bcc9 )
	ROM_LOAD( "pisces.d1", 0x2000, 0x0800, 0x0c767804 )
	ROM_LOAD( "pisces.e2", 0x2800, 0x0800, 0x6d0ac2d8 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pisces.1j", 0x0000, 0x1000, 0x1a5c1d66 )
	ROM_LOAD( "pisces.1k", 0x1000, 0x1000, 0x3a8d10fb )
ROM_END

ROM_START( japirem_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",  0x0000, 0x0800, 0x9cc3c45f )
	ROM_LOAD( "h07_2a.bin",  0x0800, 0x0800, 0x76d8a0c6 )
	ROM_LOAD( "k07_3a.bin",  0x1000, 0x0800, 0x6511100f )
	ROM_LOAD( "m07_4a.bin",  0x1800, 0x0800, 0x0ac76feb )
	ROM_LOAD( "d08p_5a.bin", 0x2000, 0x0800, 0x1ce1f21b )
	ROM_LOAD( "e08p_6a.bin", 0x2800, 0x0800, 0xb2bdb8c9 )
	ROM_LOAD( "m08p_7a.bin", 0x3000, 0x0800, 0x7f2cc704 )
	ROM_LOAD( "n08p_8a.bin", 0x3800, 0x0800, 0x79b90327 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "h01_1.bin",   0x0000, 0x0800, 0x11311ceb )
	ROM_LOAD( "h01_2.bin",   0x0800, 0x0800, 0xc2870825 )
	ROM_LOAD( "k01_1.bin",   0x1000, 0x0800, 0x79b1be9f )
	ROM_LOAD( "k01_2.bin",   0x1800, 0x0800, 0xa42e795c )
ROM_END

ROM_START( uniwars_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u1",  0x0000, 0x0800, 0x9cc3c45f )
	ROM_LOAD( "u2",  0x0800, 0x0800, 0x76d8a0c6 )
	ROM_LOAD( "u3",  0x1000, 0x0800, 0x6511100f )
	ROM_LOAD( "u4",  0x1800, 0x0800, 0x0ac76feb )
	ROM_LOAD( "u5",  0x2000, 0x0800, 0x37e1e91b )
	ROM_LOAD( "u6",  0x2800, 0x0800, 0x528d7839 )
	ROM_LOAD( "u7",  0x3000, 0x0800, 0x7f2cc704 )
	ROM_LOAD( "u8",  0x3800, 0x0800, 0xe370a4d6 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "u10", 0x0000, 0x0800, 0x8b6d9ed9 )
	ROM_LOAD( "u12", 0x0800, 0x0800, 0xc2870825 )
	ROM_LOAD( "u9",  0x1000, 0x0800, 0x2acb176d )
	ROM_LOAD( "u11", 0x1800, 0x0800, 0xa42e795c )
ROM_END

ROM_START( warofbug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "warofbug.u",  0x0000, 0x0800, 0x07c19765 )
	ROM_LOAD( "warofbug.v",  0x0800, 0x0800, 0x577383df )
	ROM_LOAD( "warofbug.w",  0x1000, 0x0800, 0xeda29210 )
	ROM_LOAD( "warofbug.y",  0x1800, 0x0800, 0x30b3e93f )
	ROM_LOAD( "warofbug.z",  0x2000, 0x0800, 0x3dc8509c )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "warofbug.1j", 0x0000, 0x0800, 0x50dd974f )
	ROM_LOAD( "warofbug.1k", 0x0800, 0x0800, 0x9dd46522 )
ROM_END



static int galaxian_hiload(const char *name)
{
	/* wait for the checkerboard pattern to be on screen */
	if (memcmp(&RAM[0x5000],"\x30\x32",2) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x40a8],1,3,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void galaxian_hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x40a8],1,3,f);
		fclose(f);
	}
}



static int pisces_hiload(const char *name)
{
	/* wait for the screen to initialize */
	if (memcmp(&RAM[0x5000],"\x10\x10\x10",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x4021],1,3,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void pisces_hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x4021],1,3,f);
		fclose(f);
	}
}

static int warofbug_hiload(const char *name)
{
	/* wait for memory to be set */
	if (memcmp(&RAM[0x4045],"\x1F\x1F",2) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x4034],1,3,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void warofbug_hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x4034],1,3,f);
		fclose(f);
	}
}



struct GameDriver galaxian_driver =
{
	"Galaxian (Namco)",
	"galaxian",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galaxian_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galaxian_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galmidw_driver =
{
	"Galaxian (Midway)",
	"galmidw",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galmidw_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galaxian_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galnamco_driver =
{
	"Galaxian (Namco, modified)",
	"galnamco",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galnamco_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver superg_driver =
{
	"Super Galaxian",
	"superg",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	superg_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galapx_driver =
{
	"Galaxian Part X",
	"galapx",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galapx_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galap1_driver =
{
	"Galaxian Part 1",
	"galap1",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galap1_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galap4_driver =
{
	"Galaxian Part 4",
	"galap4",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galap4_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galturbo_driver =
{
	"Galaxian Turbo",
	"galturbo",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT",
	&galaxian_machine_driver,

	galturbo_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, galboot_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver pisces_driver =
{
	"Pisces",
	"pisces",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT\nMIKE BALFOUR",
	&pisces_machine_driver,

	pisces_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, pisces_dsw, keys,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	pisces_hiload, pisces_hisave
};

struct GameDriver japirem_driver =
{
	"Gingateikoku No Gyakushu",
	"japirem",
	"NICOLA SALMORIA\nLIONEL THEUNISSEN\nROBERT ANSCHUETZ\nANDREW SCOTT",
	&pisces_machine_driver,

	japirem_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, japirem_dsw, keys,

	japirem_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver uniwars_driver =
{
	"Uniwars",
	"uniwars",
	"NICOLA SALMORIA\nGARY WALTON\nROBERT ANSCHUETZ\nANDREW SCOTT",
	&pisces_machine_driver,

	uniwars_rom,
	0, 0,
	mooncrst_sample_names,

	galaxian_input_ports, 0, trak_ports, japirem_dsw, keys,

	uniwars_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver warofbug_driver =
{
	"War of the Bugs",
	"warofbug",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA\nANDREW SCOTT\nMIKE BALFOUR",
	&galaxian_machine_driver,

	warofbug_rom,
	0, 0,
	mooncrst_sample_names,

	warofbug_input_ports, 0, trak_ports, warofbug_dsw, wbug_keys,

	japirem_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	warofbug_hiload, warofbug_hisave
};
