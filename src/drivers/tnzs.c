#define MALLOC_CPU1_ROM_AREAS

/***************************************************************************

The New Zealand Story driver, used for tnzs & tnzs2.

TO-DO: - Find out how the hardware credit-counter works
       - Fix problem with sprites and tiles being a few pixels out when the
         screen is flipped.

13/6/1998: - Hi-score saving/loading for tnzs & tnzs2, by Santeri Saarimaa

****************************************************************************

The New Zealand Story memory map (preliminary)

CPU #1
0000-7fff ROM
8000-bfff banked - banks 0-1 RAM; banks 2-7 ROM
c000-dfff object RAM, including:
  c000-c1ff sprites (code, low byte)
  c200-c3ff sprites (x-coord, low byte)
  c400-c5ff tiles (code, low byte)

  d000-d1ff sprites (code, high byte)
  d200-d3ff sprites (x-coord and colour, high byte)
  d400-d5ff tiles (code, high byte)
  d600-d7ff tiles (colour)
e000-efff RAM shared with CPU #2
f000-ffff VDC RAM, including:
  f000-f1ff sprites (y-coord)
  f200-f2ff scrolling info
  f300-f301 vdc controller
  f302-f303 scroll x-coords (high bits)
  f600      bankswitch
  f800-fbff palette

CPU #2
0000-7fff ROM
8000-9fff banked ROM
a000      bankswitch
b000-b001 YM2203 interface (with DIPs on YM2203 ports)
c000-c001 input ports (and coin counter)
e000-efff RAM shared with CPU #1
f000-f003 ???

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



/* prototypes for functions in ../machine/tnzs.c */
extern unsigned char *tnzs_objram, *tnzs_workram;
extern unsigned char *tnzs_vdcram, *tnzs_scrollram;
extern unsigned char *tnzs_cpu2ram;
extern int tnzs_objram_size;
void init_tnzs(void);
int tnzs_interrupt(void){return 0;}
int tnzs_objram_r(int offset);

int tnzs_inputport_r(int offset);
int tnzs_cpu2ram_r(int offset);
int tnzs_workram_r(int offset);
int tnzs_vdcram_r(int offset);

void tnzs_objram_w(int offset, int data);
void tnzs_bankswitch1_w(int offset, int data);
void tnzs_inputport_w(int offset, int data);
void tnzs_cpu2ram_w(int offset, int data);
void tnzs_workram_w(int offset, int data);
void tnzs_vdcram_w(int offset, int data);
void tnzs_scrollram_w(int offset, int data);

unsigned char *banked_ram_0, *banked_ram_1;

#ifdef MALLOC_CPU1_ROM_AREAS
unsigned char *banked_rom_0, *banked_rom_1, *banked_rom_2, *banked_rom_3;
#endif

void tnzs_patch(void)
{
	if ((banked_ram_0 = malloc(0x4000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x4000 for ram 0\n");
		exit(-1);
	}
	if ((banked_ram_1 = malloc(0x4000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x4000 for ram 1\n");
		exit(-1);
	}

#ifdef MALLOC_CPU1_ROM_AREAS
	if ((banked_rom_0 = malloc(0x2000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x2000 for rom 0\n");
		exit(-1);
	}
	memcpy(banked_rom_0, Machine->memory_region[2] + 0x8000 + 0x2000 * 0, 0x2000);

	if ((banked_rom_1 = malloc(0x2000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x2000 for rom 1\n");
		exit(-1);
	}
	memcpy(banked_rom_1, Machine->memory_region[2] + 0x8000 + 0x2000 * 1, 0x2000);

	if ((banked_rom_2 = malloc(0x2000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x2000 for rom 2\n");
		exit(-1);
	}
	memcpy(banked_rom_2, Machine->memory_region[2] + 0x8000 + 0x2000 * 2, 0x2000);

	if ((banked_rom_3 = malloc(0x2000)) == NULL)
	{
		if (errorlog)
			fprintf(errorlog, "can't malloc 0x2000 for rom 3\n");
		exit(-1);
	}
	memcpy(banked_rom_3, Machine->memory_region[2] + 0x8000 + 0x2000 * 3, 0x2000);
#endif
}
void tnzs_sound_command_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "play sound %02x\n", data);
}

void tnzs_bankswitch_w(int offset, int data);

/* prototypes for functions in ../vidhrdw/tnzs.c */
void tnzs_videoram_w(int offset,int data);
void tnzs_objectram_w(int offset,int data);
int tnzs_vh_start(void);
void tnzs_vh_stop(void);
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x7fff, MRA_ROM },

    { 0x8000, 0xbfff, MRA_BANK1 }, /* BANK RAM */
    { 0xc000, 0xdfff, tnzs_objram_r },
    { 0xe000, 0xefff, tnzs_workram_r }, /* WORK RAM */
    { 0xf000, 0xf1ff, tnzs_vdcram_r }, /*  VDC RAM */
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },

	/* { 0xef10, 0xef10, tnzs_sound_command_w }, */

    { 0x8000, 0xbfff, MWA_BANK1 },
    { 0xc000, 0xdfff, tnzs_objram_w, &tnzs_objram, &tnzs_objram_size },

    { 0xe000, 0xefff, tnzs_workram_w, &tnzs_workram },

    { 0xf000, 0xf1ff, tnzs_vdcram_w, &tnzs_vdcram },
    { 0xf200, 0xf3ff, tnzs_scrollram_w, &tnzs_scrollram }, /* scrolling info */
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
    { 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem1[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0x9fff, MRA_BANK2 },
    { 0xb000, 0xb000, YM2203_status_port_0_r  },
    { 0xb001, 0xb001, YM2203_read_port_0_r  },
    { 0xc000, 0xc001, tnzs_inputport_r},
    { 0xd000, 0xdfff, tnzs_cpu2ram_r },
    { 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf003, MRA_RAM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem1[] =
{
    { 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
    { 0xb000, 0xb000, YM2203_control_port_0_w  },
    { 0xb001, 0xb001, YM2203_write_port_0_w  },
    { 0xc000, 0xc001, tnzs_inputport_w},
    { 0xd000, 0xdfff, tnzs_cpu2ram_w, &tnzs_cpu2ram },
    { 0xe000, 0xefff, tnzs_workram_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( tnzs_input_ports )
	PORT_START      /* IN0 - number of credits??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

 /* DIP switch settings supplied by Greg Best <gregbest98@hotmail.com> */
	PORT_START      /* DSW A - ef0e */
    PORT_DIPNAME( 0x01, 0x01, "Game style", IP_KEY_NONE )
    PORT_DIPSETTING(    0x01, "Table" )
    PORT_DIPSETTING(    0x00, "Upright" )
    PORT_DIPNAME( 0x02, 0x02, "Flip screen", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x04, 0x04, "Test mode", IP_KEY_NONE )
    PORT_DIPSETTING(    0x04, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Always off */
    PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
    PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
    PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
    PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
    PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
    PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )

	PORT_START      /* DSW B - ef0f */
    PORT_DIPNAME( 0x03, 0x02, "Difficulty", IP_KEY_NONE)
    PORT_DIPSETTING(    0x03, "Easy" )
    PORT_DIPSETTING(    0x02, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0C, 0x00, "Bonus Life", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "at 10k and 100k" )
    PORT_DIPSETTING(    0x0C, "at 10k and 150k" )
    PORT_DIPSETTING(    0x08, "at 10k and 200k" )
    PORT_DIPSETTING(    0x04, "at 10k and 300k" )
    PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x20, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Continues", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x40, "On" )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* Always off */

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	16,16,	/* the characters are 16x16 pixels */
    0x100 * 32,
	4,	/* 4 bits per pixel */
    { 0xc0000*8, 0x80000*8, 0x40000*8, 0x00000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 64, 65, 66, 67, 68, 69, 70, 71 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 128, 136, 144, 152, 160, 168, 176, 184},
	32*8	/* every char takes 32 bytes in four ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x00000, &charlayout, 0, 32 },
	{ -1 }	/* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	1500000,	/* 1.5 MHz ??? */
	{ YM2203_VOL(255,255) },
	{ input_port_6_r },
	{ input_port_7_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver tnzs_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			4000000,		/* 4 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
            6000000,        /* 6 Mhz??? */
			2,			/* memory_region */
			readmem1,writemem1,0,0,
			interrupt,1
		}
    },
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    init_tnzs,		/* init_machine() */

    /* video hardware */
    16*16, 14*16,         /* screen_width, height */
    { 0, 16*16-1, 0, 14*16-1 }, /* visible_area */
    gfxdecodeinfo,
    512, 512,
    0,

    VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

/* 00000 ROM (static)
   08000 RAM page 0
   0c000 RAM page 1
   10000 ROM page 2
   14000 ROM page 3
   18000 ROM page 4
   1c000 ROM page 5
   20000 ROM page 6
   24000 ROM page 7
   28000 (end)
  */
ROM_START( tnzs_rom )
    ROM_REGION(0x28000)	/* 64k+64k for the first CPU */
#if 1
    ROM_LOAD( "NZSB5310.BIN", 0x00000, 0x08000, 0x935a36b4 )
    ROM_CONTINUE(            0x10000, 0x18000 )
#else
    ROM_LOAD( "NZSB5310.BIN", 0x00000, 0x20000, 0x935a36b4 )
#endif

    ROM_REGION(0x100000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "NZSB5308.BIN", 0x00000, 0x10000, 0x2d871843 )
    ROM_LOAD( "NZSB5307.BIN", 0x20000, 0x10000, 0xe6f8be50 )
    ROM_LOAD( "NZSB5306.BIN", 0x40000, 0x10000, 0x4465123b )
    ROM_LOAD( "NZSB5305.BIN", 0x60000, 0x10000, 0x8b0eff5e )
    ROM_LOAD( "NZSB5304.BIN", 0x80000, 0x10000, 0x1b5e0a3c )
    ROM_LOAD( "NZSB5303.BIN", 0xa0000, 0x10000, 0x59b5cbf1 )
    ROM_LOAD( "NZSB5302.BIN", 0xc0000, 0x10000, 0xb3fab8ac )
    ROM_LOAD( "NZSB5301.BIN", 0xe0000, 0x10000, 0x030d388d )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "NZSB5311.BIN", 0x00000, 0x10000, 0xcc5f8183 )
ROM_END




ROM_START( tnzs2_rom )
    ROM_REGION(0x28000)	/* 64k+64k+32k for the first CPU */
#if 1
    ROM_LOAD( "NS_C-11.ROM", 0x00000, 0x08000, 0x6134e9ee )
    ROM_CONTINUE(            0x10000, 0x18000 )
#else
    ROM_LOAD( "NS_C-11.ROM", 0x00000, 0x20000, 0x6134e9ee )
#endif

    ROM_REGION(0x100000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "NS_A13.ROM", 0x00000, 0x20000, 0x635a5412 )
    ROM_LOAD( "NS_A12.ROM", 0x20000, 0x20000, 0x68bed40a )
    ROM_LOAD( "NS_A10.ROM", 0x40000, 0x20000, 0x274c4100 )
    ROM_LOAD( "NS_A08.ROM", 0x60000, 0x20000, 0x6d0de83d )
    ROM_LOAD( "NS_A07.ROM", 0x80000, 0x20000, 0x453ad1f8 )
    ROM_LOAD( "NS_A05.ROM", 0xa0000, 0x20000, 0x186f4b07 )
    ROM_LOAD( "NS_A04.ROM", 0xc0000, 0x20000, 0x7c65f6df )
    ROM_LOAD( "NS_A02.ROM", 0xe0000, 0x20000, 0x2377128b )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "NS_E-3.ROM", 0x00000, 0x10000, 0x244e8a60 )
ROM_END

static int tnzs_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xe6ad], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xe68d], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xe68d], 35);
		osd_fclose(f);
	}
}

static int tnzs2_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xec2a], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xec0a], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs2_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xec0a], 35);
		osd_fclose(f);
	}
}


struct GameDriver tnzs_driver =
{
	__FILE__,
	0,
	"tnzs",
	"The New Zealand Story",
	"1988",
	"Taito",
    "Chris Moore\nMartin Scragg\nRichard Mitton\nSanteri Saarimaa (hi-scores)",
	GAME_NOT_WORKING,
	&tnzs_machine_driver,

	tnzs_rom,
	tnzs_patch, 0,	/* remove protection */
	0,
	0,	/* sound_prom */

	tnzs_input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    tnzs_hiload, tnzs_hisave
};

struct GameDriver tnzs2_driver =
{
	__FILE__,
	0,
	"tnzs2",
	"The New Zealand Story 2",
	"1988",
	"Taito",
    "Chris Moore\nMartin Scragg\nRichard Mitton\nSanteri Saarimaa (hi-scores)",
	0,
	&tnzs_machine_driver,

	tnzs2_rom,
	tnzs_patch, 0,	/* remove protection */
	0,
	0,	/* sound_prom */

	tnzs_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

    tnzs2_hiload, tnzs2_hisave
};
