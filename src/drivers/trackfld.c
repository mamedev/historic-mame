/***************************************************************************

Konami games memory map (preliminary)

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

Track'n'Field

MAIN BOARD:
0000-17ff RAM
1800-183f Sprite RAM Pt 1
1C00-1C3f Sprite RAM Pt 2
3800-3bff Color RAM
3000-33ff Video RAM
6000-ffff ROM
1200-12ff IO

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"



void trackfld_flipscreen_w(int offset,int data);
void trackfld_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void trackfld_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int trackfld_vh_start(void);
void trackfld_vh_stop(void);
int konami_IN1_r(int offset);

void hyperspt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char *konami_dac;
void konami_sh_irqtrigger_w(int offset,int data);
int trackfld_sh_timer_r(int offset);
int trackfld_speech_r(int offset);
void trackfld_sound_w(int offset , int data);

extern struct VLM5030interface konami_vlm5030_interface;
extern struct SN76496interface konami_sn76496_interface;
extern struct DACinterface konami_dac_interface;
void konami_dac_w(int offset,int data);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );



void trackfld_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

extern unsigned char *trackfld_scroll;
extern unsigned char *trackfld_scroll2;



static struct MemoryReadAddress readmem[] =
{
	{ 0x1800, 0x185f, MRA_RAM },
	{ 0x1c00, 0x1c5f, MRA_RAM },
	{ 0x1200, 0x1200, input_port_4_r }, /* DIP 2 */
	{ 0x1280, 0x1280, input_port_0_r }, /* IO Coin */
//	{ 0x1281, 0x1281, input_port_1_r }, /* P1 IO */
	{ 0x1281, 0x1281, konami_IN1_r },	/* P1 IO and handle fake button for cheating */
	{ 0x1282, 0x1282, input_port_2_r }, /* P2 IO */
	{ 0x1283, 0x1283, input_port_3_r }, /* DIP 1 */
	{ 0x2800, 0x3fff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x1000, 0x1000, watchdog_reset_w },
	{ 0x1080, 0x1080, trackfld_flipscreen_w },
	{ 0x1081, 0x1081, konami_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1083, 0x1083, MWA_NOP },  /* Coin counter 1 */
	{ 0x1084, 0x1084, MWA_NOP },  /* Coin counter 2 */
	{ 0x1087, 0x1087, interrupt_enable_w },
	{ 0x1100, 0x1100, soundlatch_w },
	{ 0x2800, 0x2fff, MWA_RAM },
	{ 0x1800, 0x183f, MWA_RAM, &spriteram_2 },
	{ 0x1840, 0x185f, MWA_RAM, &trackfld_scroll },  /* Scroll amount */
	{ 0x1C00, 0x1c3f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1C40, 0x1C5f, MWA_RAM, &trackfld_scroll2 },  /* Scroll amount */
	{ 0x2800, 0x2fff, MWA_RAM },
	{ 0x3000, 0x37ff, videoram_w, &videoram, &videoram_size },
	{ 0x3800, 0x3fff, colorram_w, &colorram },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, trackfld_sh_timer_r },
	{ 0xe002, 0xe002, trackfld_speech_r},
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, SN76496_0_w }, // Loads the snd command into the snd latch
	{ 0xc000, 0xc000, MWA_NOP },		// This address triggers the SN chip to read the data port.
	{ 0xe000, 0xe000, konami_dac_w, &konami_dac },
/* There are lots more address's which are used for setting a two bit volume
	controls for speech and music

	Currently these are un-supported by Mame
*/
	{ 0xe001, 0xe001, MWA_NOP }, /* watch dog ? */
	{ 0xe004, 0xe004, VLM5030_data_w },
	{ 0xe000, 0xefff, trackfld_sound_w, }, /* e003 speech controll */

	{ 0xe079, 0xe079, MWA_NOP },  /* ???? */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Fake button to press buttons 1 and 3 impossibly fast. Handle via konami_IN1_r */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_CHEAT | IPF_PLAYER1, "Run Like Hell Cheat", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 disables Coin 2. It still accepts coins and makes the sound, but
   it doesn't give you any credit */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x00, "After Last Event", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Game Over" )
	PORT_DIPSETTING(    0x00, "Game Continues" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail")
	PORT_DIPNAME( 0x08, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "None" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x10, 0x10, "World Records", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Don't Erase" )
	PORT_DIPSETTING(    0x00, "Erase on Reset" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 256*64*8+4, 256*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x6000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* tfprom.1 - palette */
	0x00,0xFF,0x52,0xAD,0xF8,0xC0,0x3F,0x30,0x07,0x02,0x14,0x66,0xB7,0xA6,0xAF,0x2D,
	0x00,0xFF,0xA4,0xAD,0xF8,0xC0,0x64,0x31,0x07,0xC7,0x55,0x5E,0x68,0x3F,0xAF,0xA8,
	/* tfprom.3 - sprite lookup table */
	0x00,0x01,0x02,0x03,0x04,0x05,0x01,0x03,0x08,0x09,0x0A,0x05,0x0D,0x0E,0x0E,0x05,
	0x00,0x01,0x04,0x04,0x00,0x08,0x06,0x0F,0x05,0x09,0x09,0x09,0x0A,0x0A,0x0A,0x08,
	0x00,0x01,0x00,0x04,0x04,0x05,0x05,0x05,0x08,0x09,0x09,0x09,0x0A,0x0B,0x0B,0x01,
	0x00,0x01,0x02,0x03,0x04,0x02,0x01,0x03,0x05,0x09,0x08,0x09,0x0E,0x0C,0x0C,0x08,
	0x00,0x01,0x05,0x03,0x04,0x05,0x06,0x07,0x01,0x01,0x01,0x06,0x06,0x06,0x06,0x06,
	0x00,0x01,0x05,0x03,0x04,0x05,0x06,0x07,0x0E,0x0E,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x01,0x00,0x03,0x04,0x05,0x06,0x07,0x06,0x06,0x06,0x06,0x06,0x00,0x00,0x00,
	0x00,0x01,0x04,0x04,0x00,0x08,0x06,0x0F,0x05,0x09,0x09,0x09,0x0B,0x0B,0x0B,0x08,
	0x00,0x01,0x00,0x00,0x04,0x05,0x06,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01,0x02,0x09,0x04,0x08,0x06,0x07,0x08,0x05,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x05,0x06,0x05,0x06,0x06,0x06,0x05,0x06,0x05,0x06,0x05,0x06,0x05,0x06,0x05,
	0x00,0x05,0x05,0x05,0x06,0x06,0x05,0x05,0x06,0x06,0x05,0x05,0x06,0x06,0x05,0x05,
	0x00,0x05,0x06,0x06,0x05,0x06,0x05,0x05,0x06,0x06,0x06,0x06,0x05,0x05,0x05,0x05,
	0x00,0x05,0x06,0x06,0x06,0x06,0x06,0x06,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x0E,0x0E,0x09,0x0A,0x0B,0x0D,0x0E,0x0E,0x0F,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	/* tfprom.2 - character lookup table */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x04,0x02,0x03,0x0E,0x08,0x06,0x07,0x01,0x09,0x0A,0x0B,0x0C,0x0D,0x05,0x0F,
	0x00,0x05,0x02,0x03,0x0E,0x04,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x01,0x0F,
	0x00,0x08,0x04,0x0D,0x0D,0x08,0x07,0x05,0x04,0x01,0x05,0x0B,0x0E,0x07,0x0E,0x0F,
	0x00,0x0F,0x0F,0x0F,0x04,0x05,0x0F,0x07,0x08,0x01,0x0F,0x0B,0x0F,0x0D,0x0E,0x0F,
	0x0C,0x08,0x0C,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x02,0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x01,0x0D,0x0A,0x0F,
	0x00,0x09,0x02,0x03,0x04,0x05,0x06,0x07,0x02,0x09,0x02,0x02,0x0C,0x0D,0x02,0x0F,
	0x02,0x08,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x07,0x02,0x03,0x04,0x05,0x06,0x07,0x07,0x07,0x07,0x07,0x0C,0x0D,0x07,0x0F,
	0x00,0x0D,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x0B,0x05,0x02,0x03,0x09,0x05,0x05,0x00,0x08,0x08,0x0A,0x0B,0x01,0x01,0x08,0x0F,
	0x0B,0x0D,0x02,0x0D,0x09,0x05,0x0D,0x00,0x08,0x0D,0x0D,0x0B,0x01,0x0D,0x0D,0x0F,
	0x03,0x00,0x03,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x01,0x0D,0x0A,0x0F,
	0x03,0x08,0x03,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x0C,0x00,0x0C,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x01,0x0D,0x0A,0x0F
};

/* filename for trackn field sample files */
static const char *trackfld_sample_names[] =
{
	"*trackfld",
	"00.sam","01.sam","02.sam","03.sam","04.sam","05.sam","06.sam","07.sam",
	"08.sam","09.sam","0a.sam","0b.sam","0c.sam","0d.sam","0e.sam","0f.sam",
	"10.sam","11.sam","12.sam","13.sam","14.sam","15.sam","16.sam","17.sam",
	"18.sam","19.sam","1a.sam","1b.sam","1c.sam","1d.sam","1e.sam","1f.sam",
	"20.sam","21.sam","22.sam","23.sam","24.sam","25.sam","26.sam","27.sam",
	"28.sam","29.sam","2a.sam","2b.sam","2c.sam","2d.sam","2e.sam","2f.sam",
	"30.sam","31.sam","32.sam","33.sam","34.sam","35.sam","36.sam","37.sam",
	"38.sam","39.sam","3a.sam","3b.sam","3c.sam","3d.sam",
	0
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 1.400 Mhz ??? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* Z80 Clock is derived from a 14.31818 Mhz crystal */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	trackfld_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,16*16+16*16,
	trackfld_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	trackfld_vh_start,
	trackfld_vh_stop,
	trackfld_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&konami_dac_interface
		},
		{
			SOUND_SN76496,
			&konami_sn76496_interface
		},
		{
			SOUND_VLM5030,
			&konami_vlm5030_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( trackfld_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "a01_e01.bin", 0x6000, 0x2000, 0xca01e553 )
	ROM_LOAD( "a02_e02.bin", 0x8000, 0x2000, 0x21213565 )
	ROM_LOAD( "a03_k03.bin", 0xA000, 0x2000, 0xfb67e07f )
	ROM_LOAD( "a04_e04.bin", 0xC000, 0x2000, 0x33b1674b )
	ROM_LOAD( "a05_e05.bin", 0xE000, 0x2000, 0xf6600ba4 )

	ROM_REGION(0xe000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "h16_e12.bin", 0x0000, 0x2000, 0xeee82d14 )
	ROM_LOAD( "h15_e11.bin", 0x2000, 0x2000, 0x9fd214e8 )
	ROM_LOAD( "h14_e10.bin", 0x4000, 0x2000, 0x36c0fa04 )

	ROM_LOAD( "c11_d06.bin", 0x6000, 0x2000, 0xca2da5bd )
	ROM_LOAD( "c12_d07.bin", 0x8000, 0x2000, 0x9a7b57d9 )
	ROM_LOAD( "c13_d08.bin", 0xa000, 0x2000, 0xed0cac48 )
	ROM_LOAD( "c14_d09.bin", 0xc000, 0x2000, 0x0e569456 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin", 0x0000, 0x2000, 0x6244bd30 )

	ROM_REGION(0x10000)	/*  64k for speech rom    */
	ROM_LOAD( "c9_d15.bin", 0x0000, 0x2000, 0xbaaab302 )
ROM_END

ROM_START( hyprolym_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "hyprolym.a01", 0x6000, 0x2000, 0x16be0b86 )
	ROM_LOAD( "hyprolym.a02", 0x8000, 0x2000, 0x05052d7f )
	ROM_LOAD( "hyprolym.a03", 0xA000, 0x2000, 0x32591b7b )
	ROM_LOAD( "hyprolym.a04", 0xC000, 0x2000, 0x1af45818 )
	ROM_LOAD( "hyprolym.a05", 0xE000, 0x2000, 0x29a7d3c1 )

	ROM_REGION(0xe000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hyprolym.h16", 0x0000, 0x2000, 0xddca2c0c )
	ROM_LOAD( "hyprolym.h15", 0x2000, 0x2000, 0x9b4721b5 )
	ROM_LOAD( "h14_e10.bin",  0x4000, 0x2000, 0x36c0fa04 )

	ROM_LOAD( "c11_d06.bin",  0x6000, 0x2000, 0xca2da5bd )
	ROM_LOAD( "c12_d07.bin",  0x8000, 0x2000, 0x9a7b57d9 )
	ROM_LOAD( "c13_d08.bin",  0xa000, 0x2000, 0xed0cac48 )
	ROM_LOAD( "c14_d09.bin",  0xc000, 0x2000, 0x0e569456 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin", 0x0000, 0x2000, 0x6244bd30 )

	ROM_REGION(0x10000)	/*  64k for speech rom    */
	ROM_LOAD( "c9_d15.bin", 0x0000, 0x2000, 0xbaaab302 )
ROM_END



static void trackfld_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0x6000;A < 0x10000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}



/*
 Track'n'Field has 1k of battery backed RAM which can be erased by setting a dipswitch
 All we need to do is load it in. If the Dipswitch is set it will be erased
*/
static int we_flipped_the_switch;

static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x2C00],0x3000-0x2c00);
		osd_fclose(f);

		we_flipped_the_switch = 0;
	}
	else
	{
		struct InputPort *in;


		/* find the dip switch which resets the high score table, and set it on */
		in = Machine->input_ports;

		while (in->type != IPT_END)
		{
			if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
					strcmp(in->name,"World Records") == 0)
			{
				if (in->default_value == in->mask)
				{
					in->default_value = 0;
					we_flipped_the_switch = 1;
				}
				break;
			}

			in++;
		}
	}

	return 1;
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2C00],0x400);
		osd_fclose(f);
	}

	if (we_flipped_the_switch)
	{
		struct InputPort *in;


		/* find the dip switch which resets the high score table, and set it */
		/* back to off. */
		in = Machine->input_ports;

		while (in->type != IPT_END)
		{
			if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
					strcmp(in->name,"World Records") == 0)
			{
				if (in->default_value == 0)
					in->default_value = in->mask;
				break;
			}

			in++;
		}

		we_flipped_the_switch = 0;
	}
}



struct GameDriver trackfld_driver =
{
	__FILE__,
	0,
	"trackfld",
	"Track & Field",
	"1983",
	"Konami",
	"Chris Hardy (MAME driver)\nTim Lindquist (color info)\nTatsuyuki Satoh(speech sound)",
	0,
	&machine_driver,

	trackfld_rom,
	0, trackfld_decode,
	trackfld_sample_names,

	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver hyprolym_driver =
{
	__FILE__,
	&trackfld_driver,
	"hyprolym",
	"Hyper Olympic",
	"1983",
	"Konami",
	"Chris Hardy (MAME driver)\nTim Lindquist (color info)\nTatsuyuki Satoh(speech sound)",
	0,
	&machine_driver,

	hyprolym_rom,
	0, trackfld_decode,
	trackfld_sample_names,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
