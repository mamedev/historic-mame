/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/sn76496.h"
#include "M6809.h"



void hyperspt_flipscreen_w(int offset,int data);
void hyperspt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int hyperspt_vh_start(void);
void hyperspt_vh_stop(void);
void hyperspt_vh_screenrefresh(struct osd_bitmap *bitmap);
int konami_IN1_r(int offset);

extern unsigned char *konami_speech;
extern unsigned char *konami_dac;
void konami_sh_irqtrigger_w(int offset,int data);
int trackfld_speech_r(int offset);
int hyperspt_sh_timer_r(int offset);
void hyperspt_sound_w(int offset , int data);
extern const char *hyperspt_sample_names[];

void konami_dac_w(int offset,int data);
int konami_sh_start(void);
void konami_sh_stop(void);
void konami_sh_update(void);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );

void hyperspt_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

extern unsigned char *hyperspt_scroll;


static struct MemoryReadAddress readmem[] =
{
	{ 0x1000, 0x13ff, MRA_RAM },
	{ 0x1600, 0x1600, input_port_4_r }, /* DIP 2 */
	{ 0x1680, 0x1680, input_port_0_r }, /* IO Coin */
//	{ 0x1681, 0x1681, input_port_1_r }, /* P1 IO */
	{ 0x1681, 0x1681, konami_IN1_r }, /* P1 IO and handle fake button for cheating */
	{ 0x1682, 0x1682, input_port_2_r }, /* P2 IO */
	{ 0x1683, 0x1683, input_port_3_r }, /* DIP 1 */
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x1000, 0x13ff, MWA_RAM },
	{ 0x1400, 0x1400, watchdog_reset_w },
	{ 0x1480, 0x1480, hyperspt_flipscreen_w },
	{ 0x1481, 0x1481, konami_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1483, 0x1483, MWA_NOP },  /* Coin counter 1 */
	{ 0x1484, 0x1484, MWA_NOP },  /* Coin counter 2 */
	{ 0x1487, 0x1487, interrupt_enable_w },  /* Interrupt enable */
	{ 0x1500, 0x1500, soundlatch_w },
	{ 0x2000, 0x27ff, videoram_w, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, colorram_w, &colorram },
	{ 0x3000, 0x35ff, MWA_RAM },
	{ 0x3600, 0x36bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x36C0, 0x36ff, MWA_RAM, &hyperspt_scroll },  /* Scroll amount */
	{ 0x3700, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, hyperspt_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_RAM, &konami_speech }, /* speech data */
	{ 0xc000, 0xdfff, hyperspt_sound_w },     /* speech and output controll */

	{ 0xe001, 0xe001, SN76496_0_w }, // Loads the snd command into the snd latch
	{ 0xe002, 0xe002, MWA_NOP },		// This address triggers the SN chip to read the data port.
	{ 0xe000, 0xe000, konami_dac_w, &konami_dac },
//	{ 0xe079, 0xe079, MWA_NOP },  /* ???? */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
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
	PORT_DIPNAME( 0x01, 0x00, "After Last Event", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Game Over" )
	PORT_DIPSETTING(    0x00, "Game Continues" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "World Records", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Don't Erase" )
	PORT_DIPSETTING(    0x00, "Erase on Reset" )
	PORT_DIPNAME( 0xf0, 0xf0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "Easy 1" )
	PORT_DIPSETTING(    0xe0, "Easy 2" )
	PORT_DIPSETTING(    0xd0, "Easy 3" )
	PORT_DIPSETTING(    0xc0, "Easy 4" )
	PORT_DIPSETTING(    0xb0, "Normal 1" )
	PORT_DIPSETTING(    0xa0, "Normal 2" )
	PORT_DIPSETTING(    0x90, "Normal 3" )
	PORT_DIPSETTING(    0x80, "Normal 4" )
	PORT_DIPSETTING(    0x70, "Normal 5" )
	PORT_DIPSETTING(    0x60, "Normal 6" )
	PORT_DIPSETTING(    0x50, "Normal 7" )
	PORT_DIPSETTING(    0x40, "Normal 8" )
	PORT_DIPSETTING(    0x30, "Difficult 1" )
	PORT_DIPSETTING(    0x20, "Difficult 2" )
	PORT_DIPSETTING(    0x10, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Difficult 4" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 sprites */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
//	{ 0x4000*8+4, 0x4000*8+0, 4, 0  },
	{ 0, 4, 0x4000*8+0, 0x4000*8+4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
//	{ 0x8000*8+4, 0x8000*8+0, 4, 0 },
	{ 0x8000*8+0, 0x8000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,
		32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8, },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x8000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



/* Hand Encoded RGB values. These should be replaced with colour PROMS */
static unsigned char color_prom[] = {
	(0>>5) | ((0>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((0>>5)<<3) | ((222>>6)<<6),
	(0>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(0>>5) | ((189>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((189>>5)<<3) | ((222>>6)<<6),
	(0>>5) | ((222>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((222>>5)<<3) | ((222>>6)<<6),
	(0>>5) | ((8>>5)<<3) | ((0>>6)<<6),
	(33>>5) | ((33>>5)<<3) | ((222>>6)<<6),
	(33>>5) | ((148>>5)<<3) | ((66>>6)<<6),
	(33>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(33>>5) | ((189>>5)<<3) | ((0>>6)<<6),
	(33>>5) | ((189>>5)<<3) | ((66>>6)<<6),
	(33>>5) | ((189>>5)<<3) | ((222>>6)<<6),
	(33>>5) | ((222>>5)<<3) | ((222>>6)<<6),
	(107>>5) | ((66>>5)<<3) | ((0>>6)<<6),
	(107>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(148>>5) | ((66>>5)<<3) | ((66>>6)<<6),
	(148>>5) | ((107>>5)<<3) | ((66>>6)<<6),
	(148>>5) | ((107>>5)<<3) | ((148>>6)<<6),
	(148>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(148>>5) | ((189>>5)<<3) | ((148>>6)<<6),
	(189>>5) | ((66>>5)<<3) | ((66>>6)<<6),
	(189>>5) | ((107>>5)<<3) | ((66>>6)<<6),
	(189>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(189>>5) | ((189>>5)<<3) | ((148>>6)<<6),
	(189>>5) | ((222>>5)<<3) | ((0>>6)<<6),
	(222>>5) | ((148>>5)<<3) | ((148>>6)<<6),
	(222>>5) | ((222>>5)<<3) | ((0>>6)<<6),
	(255>>5) | ((0>>5)<<3) | ((0>>6)<<6),
	(255>>5) | ((33>>5)<<3) | ((66>>6)<<6),
	(255>>5) | ((255>>5)<<3) | ((255>>6)<<6),

	0,29,6,4,20,17,3,18,31,7,8,26,25,18,12,16,
	0,29,6,7,16,15,31,24,14,7,8,12,20,8,15,8,
	0,3,4,5,6,7,8,9,29,11,12,13,14,15,1,2,
	0,29,5,6,7,8,9,10,29,12,13,28,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,26,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,3,2,3,4,5,6,7,8,
	0,10,11,19,13,7,15,25,2,7,7,5,6,7,8,9,
	0,12,12,13,14,15,1,2,26,4,5,6,7,8,9,10,
	13,11,13,14,15,1,2,3,4,6,6,13,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	25,14,15,1,2,3,4,5,0,7,31,9,10,11,12,13,
	0,16,1,2,3,4,5,6,7,2,8,10,18,11,11,14,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
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
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	hyperspt_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,16*16+16*16,
	hyperspt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	hyperspt_vh_start,
	hyperspt_vh_stop,
	hyperspt_vh_screenrefresh,

	/* sound hardware */
	0,
	konami_sh_start,
	konami_sh_stop,
	konami_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( hyperspt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "c01", 0x4000, 0x2000, 0x2c6a45d4 )
	ROM_LOAD( "c02", 0x6000, 0x2000, 0x88d9e7a1 )
	ROM_LOAD( "c03", 0x8000, 0x2000, 0xb7af516d )
	ROM_LOAD( "c04", 0xA000, 0x2000, 0x82016223 )
	ROM_LOAD( "c05", 0xC000, 0x2000, 0x31bdc4cf )
	ROM_LOAD( "c06", 0xE000, 0x2000, 0x1fb6303a )

	ROM_REGION(0x18000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c26", 0x00000, 0x2000, 0x60c9d39d )
	ROM_LOAD( "c24", 0x02000, 0x2000, 0xd7f39563 )
	ROM_LOAD( "c22", 0x04000, 0x2000, 0x149cf900 )
	ROM_LOAD( "c20", 0x06000, 0x2000, 0x1a7952d3 )
	ROM_LOAD( "c18", 0x08000, 0x2000, 0x3ac2203a )
	ROM_LOAD( "c17", 0x0a000, 0x2000, 0x14f47b2a )
	ROM_LOAD( "c16", 0x0c000, 0x2000, 0x735a4fbc )
	ROM_LOAD( "c15", 0x0e000, 0x2000, 0x489827a0 )
	ROM_LOAD( "c14", 0x10000, 0x2000, 0x96d716c7 )
	ROM_LOAD( "c13", 0x12000, 0x2000, 0x3edd7977 )
	ROM_LOAD( "c12", 0x14000, 0x2000, 0x68074367 )
	ROM_LOAD( "c11", 0x16000, 0x2000, 0xa8ec8174 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "c10", 0x0000, 0x2000, 0xb865b22d )
	ROM_LOAD( "c09", 0x2000, 0x2000, 0x7d7134df )

	ROM_REGION(0x02000)	/*  8k for speech rom    */
	ROM_LOAD( "c08", 0x0000, 0x2000, 0xe7673ce5 )
ROM_END

static void hyperspt_decode(void)
{
	int A;


	for (A = 0x4000;A < 0x10000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}



/*
 HyperSports has 2k of battery backed RAM which can be erased by setting a dipswitch
 All we need to do is load it in. If the Dipswitch is set it will be erased
*/

static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x3800],0x4000-0x3800);
		osd_fclose(f);
	}

	return 1;
}

static void hisave(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x3800],0x800);
		osd_fclose(f);
	}
}



struct GameDriver hyperspt_driver =
{
	"HyperSports",
	"hyperspt",
	"Chris Hardy (MAME driver)\n",
	&machine_driver,

	hyperspt_rom,
	0, hyperspt_decode,
	hyperspt_sample_names,
	0,	/* sound_prom */

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
