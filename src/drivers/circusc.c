/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/sn76496.h"
#include "M6809.h"



extern unsigned char *circusc_spritebank;
extern unsigned char *circusc_scroll;

void circusc_flipscreen_w(int offset,int data);
void circusc_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void circusc_sprite_bank_select_w(int offset, int data);
void circusc_vh_screenrefresh(struct osd_bitmap *bitmap);

void circusc_dac_w(int offset,int data);
void circusc_sh_irqtrigger_w(int offset,int data);
int circusc_sh_start(void);
void circusc_sh_stop(void);
void circusc_sh_update(void);
int circusc_sh_timer_r(int offset);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );

void circusc_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x1000, 0x1000, input_port_0_r }, /* IO Coin */
	{ 0x1001, 0x1001, input_port_1_r }, /* P1 IO */
	{ 0x1002, 0x1002, input_port_2_r }, /* P2 IO */
	{ 0x1400, 0x1400, input_port_3_r }, /* DIP 1 */
	{ 0x1800, 0x1800, input_port_4_r }, /* DIP 2 */
	{ 0x2000, 0x39ff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0000, circusc_flipscreen_w },
	{ 0x0001, 0x0001, interrupt_enable_w },
	{ 0x0003, 0x0003, MWA_NOP },  /* Coin counter 1 */
	{ 0x0004, 0x0004, MWA_NOP },  /* Coin counter 2 */
	{ 0x0005, 0x0005, MWA_RAM, &circusc_spritebank },
	{ 0x0400, 0x0400, watchdog_reset_w },
	{ 0x0800, 0x0800, soundlatch_w },
	{ 0x0c00, 0x0c00, circusc_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1c00, 0x1c00, MWA_RAM, &circusc_scroll },
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x33ff, colorram_w, &colorram },
	{ 0x3400, 0x37ff, videoram_w, &videoram, &videoram_size },
	{ 0x3800, 0x38ff, MWA_RAM, &spriteram_2 },
	{ 0x3900, 0x39ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3a00, 0x3fff, MWA_RAM },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, circusc_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP },	/* latch command for the 76496. We should buffer this */
									/* command and send it to the chip, but we just use */
									/* the triggers below because the program always writes */
									/* the same number here and there. */
	{ 0xa001, 0xa001, SN76496_0_w },	/* trigger the 76496 to read the latch */
	{ 0xa002, 0xa002, SN76496_1_w },	/* trigger the 76496 to read the latch */
	{ 0xa003, 0xa003, circusc_dac_w },
	{ 0xa004, 0xa004, MWA_NOP },		/* ??? */
	{ 0xa07c, 0xa07c, MWA_NOP },		/* ??? */
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
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
	PORT_DIPSETTING(    0x00, "Free Play" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "20000 70000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0x10, 0x10, "Dip Sw2 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Dip Sw2 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Dip Sw2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
/* This order needs to be changed to 0,1,2,3 when the correct colour proms
   are available. The order is reversed because this was the order used when
	I built the colour table by hand */
//	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 3, 2, 1, 0 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	384,	/* 384 sprites */
	4,      /* 4 bits per pixel */
//	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 3, 2, 1, 0 },        /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*4*16, 1*4*16, 2*4*16, 3*4*16, 4*4*16, 5*4*16, 6*4*16, 7*4*16,
			8*4*16, 9*4*16, 10*4*16, 11*4*16, 12*4*16, 13*4*16, 14*4*16, 15*4*16 },
	32*4*8    /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x4000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



/* Hand Encoded RGB values. These should be replaced with colour PROMS */
static unsigned char color_prom[] = {
	(0>>5) | ((0>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((0>>5)<<3) | ((222>>6)<<6),
	(0>>5) | ((107>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((107>>5)<<3) | ((66>>6)<<6),
	(0>>5) | ((148>>5)<<3) | ((0>>6)<<6),
	(0>>5) | ((148>>5)<<3) | ((222>>6)<<6),
	(107>>5) | ((107>>5)<<3) | ((148>>6)<<6),
	(148>>5) | ((0>>5)<<3) | ((0>>6)<<6),
	(148>>5) | ((0>>5)<<3) | ((222>>6)<<6),
	(189>>5) | ((0>>5)<<3) | ((222>>6)<<6),
	(189>>5) | ((148>>5)<<3) | ((222>>6)<<6),
	(222>>5) | ((0>>5)<<3) | ((0>>6)<<6),
	(222>>5) | ((107>>5)<<3) | ((148>>6)<<6),
	(222>>5) | ((107>>5)<<3) | ((222>>6)<<6),
	(222>>5) | ((222>>5)<<3) | ((0>>6)<<6),
	(255>>5) | ((255>>5)<<3) | ((255>>6)<<6),

	0,6,14,12,7,5,1,4,15,10,5,13,11,6,12,2,
	0,2,3,4,5,6,15,8,9,10,11,12,13,14,15,1,
	0,3,4,14,6,7,8,9,0,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,8,11,12,13,14,15,1,2,3,4,
	0,6,7,11,7,10,11,12,14,14,15,0,2,3,4,4,
	0,7,8,9,10,11,12,13,0,15,1,2,3,4,5,6,
	0,8,9,10,11,12,14,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,3,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,12,12,13,14,15,11,2,0,4,5,6,7,8,9,10,
	0,11,13,14,15,1,2,3,4,6,6,13,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,0,7,0,9,10,11,12,13,
	0,16,1,2,3,4,5,6,7,2,8,10,18,11,11,14,
	0,1,2,3,4,5,4,7,8,9,10,11,12,13,14,15,
	0,6,14,1,14,9,1,4,15,13,5,13,11,12,10,2,
	0,2,3,4,5,6,15,8,9,10,15,12,13,14,15,1,
	0,3,4,14,6,7,8,14,0,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,14,8,9,9,11,12,13,8,5,1,11,3,4,5,
	0,7,8,9,10,11,12,13,26,15,1,2,3,4,5,6,
	0,8,9,10,11,12,14,14,15,1,14,3,4,5,6,7,
	0,9,10,11,12,13,14,15,3,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,12,12,13,14,15,11,2,0,4,11,6,7,8,9,10,
	0,11,13,14,15,1,2,3,4,6,6,13,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,0,7,0,9,10,11,12,13,
	0,16,1,2,3,4,5,6,7,2,8,10,18,11,11,14,
	0,1,2,3,4,5,4,7,8,9,4,11,12,13,14,15,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 2 Mhz */
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
	circusc_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32/2,16*16+16*16,
	circusc_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	circusc_vh_screenrefresh,

	/* sound hardware */
	0,
	circusc_sh_start,
	circusc_sh_stop,
	circusc_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( circusc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "h03_r05.bin", 0x6000, 0x2000, 0x014a2838 )
	ROM_LOAD( "h04_n04.bin", 0x8000, 0x2000, 0x1a708c52 )
	ROM_LOAD( "h05_n03.bin", 0xA000, 0x2000, 0x624a16a2 )
	ROM_LOAD( "h06_n02.bin", 0xC000, 0x2000, 0xcc30b1b8 )
	ROM_LOAD( "h07_n01.bin", 0xE000, 0x2000, 0x59c0afce )

	ROM_REGION(0x10000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a04_j12.bin", 0x0000, 0x2000, 0xb87e4276 )
	ROM_LOAD( "a05_k13.bin", 0x2000, 0x2000, 0x91e42b1a )
	ROM_LOAD( "e11_j06.bin", 0x4000, 0x2000, 0xf8dad4f2 )
	ROM_LOAD( "e12_j07.bin", 0x6000, 0x2000, 0x4a3b8c93 )
	ROM_LOAD( "e13_j08.bin", 0x8000, 0x2000, 0xeebecdd0 )
	ROM_LOAD( "e14_j09.bin", 0xa000, 0x2000, 0xb0fe94f2 )
	ROM_LOAD( "e15_j10.bin", 0xc000, 0x2000, 0xef7e6bc0 )
	ROM_LOAD( "e16_j11.bin", 0xe000, 0x2000, 0xc68c2584 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, 0x7d613615 )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, 0x12edc96d )
ROM_END

ROM_START( circusc2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "s05", 0x6000, 0x2000, 0x215f0a8f )
	ROM_LOAD( "q04", 0x8000, 0x2000, 0x7911c25b )
	ROM_LOAD( "q03", 0xA000, 0x2000, 0xf1ddab95 )
	ROM_LOAD( "q02", 0xC000, 0x2000, 0xf2760124 )
	ROM_LOAD( "q01", 0xE000, 0x2000, 0x331c99ea )

	ROM_REGION(0x10000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "j12", 0x0000, 0x2000, 0xb87e4276 )
	ROM_LOAD( "k13", 0x2000, 0x2000, 0x91e42b1a )
	ROM_LOAD( "j06", 0x4000, 0x2000, 0xf8dad4f2 )
	ROM_LOAD( "j07", 0x6000, 0x2000, 0x4a3b8c93 )
	ROM_LOAD( "j08", 0x8000, 0x2000, 0xeebecdd0 )
	ROM_LOAD( "j09", 0xa000, 0x2000, 0xb0fe94f2 )
	ROM_LOAD( "j10", 0xc000, 0x2000, 0xef7e6bc0 )
	ROM_LOAD( "j11", 0xe000, 0x2000, 0xc68c2584 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "l14", 0x0000, 0x2000, 0x7d613615 )
	ROM_LOAD( "l15", 0x2000, 0x2000, 0x12edc96d )
ROM_END

static void circusc_decode(void)
{
	int A;


	for (A = 0x6000;A < 0x10000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if (memcmp(&RAM[0x2163],"CBR",3) == 0 &&
		memcmp(&RAM[0x20A6],"\x01\x98\x30",3) == 0 &&
		memcmp(&RAM[0x3627],"\x01",1) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2160],0x32);
			RAM[0x20A6] = RAM[0x2160];
			RAM[0x20A7] = RAM[0x2161];
			RAM[0x20A8] = RAM[0x2162];

			/* Copy high score to videoram */
			RAM[0x35A7] = RAM[0x2162] & 0x0F;
			RAM[0x35C7] = (RAM[0x2162] & 0xF0) >> 4;
			RAM[0x35E7] = RAM[0x2161] & 0x0F;
			RAM[0x3607] = (RAM[0x2161] & 0xF0) >> 4;
			RAM[0x3627] = RAM[0x2160] & 0x0F;
			if ((RAM[0x2160] & 0xF0) != 0)
				RAM[0x3647] = (RAM[0x2160] & 0xF0) >> 4;

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2160],0x32);
		osd_fclose(f);
	}
}



struct GameDriver circusc_driver =
{
	"Circus Charlie",
	"circusc",
	"Chris Hardy (MAME driver)\nValerio Verrando (high score save)",
	&machine_driver,

	circusc_rom,
	0, circusc_decode,
	0,
	0,	/* sound_prom */

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver circusc2_driver =
{
	"Circus Charlie (level select)",
	"circusc2",
	"Chris Hardy (MAME driver)\nValerio Verrando (high score save)",
	&machine_driver,

	circusc2_rom,
	0, circusc_decode,
	0,
	0,	/* sound_prom */

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
