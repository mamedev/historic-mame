/***************************************************************************

Q*bert Qubes: same as Q*bert with two banks of sprites
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int qbert_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh_w(int offset, int data);
void gottlieb_output(int offset, int data);
int qbert_IN1_r(int offset);
extern unsigned char *gottlieb_paletteram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

extern struct MemoryReadAddress gottlieb_sound_readmem[];
extern struct MemoryWriteAddress gottlieb_sound_writemem[];
int gottlieb_sh_start(void);
void gottlieb_sh_stop(void);
void gottlieb_sh_update(void);
int gottlieb_sh_interrupt(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, qbert_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};




static struct InputPort input_ports[] =
{
	{       /* DSW */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x40,   /* test mode off */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, 0 /* coin 2 */,
				0, 0, 0, OSD_KEY_F2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* 2 joysticks (cocktail mode) mapped to one */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
			OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
			OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN },
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
	{ 4, 0, "MOVE DOWN RIGHT" },
	{ 4, 1, "MOVE UP LEFT"  },
	{ 4, 2, "MOVE UP RIGHT" },
	{ 4, 3, "MOVE DOWN LEFT" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x08, "ATTRACT MODE SOUND", { "ON", "OFF" } },
/*
   Too lazy to enter such a big table of coins/credit until non-consecutive
   dip-switches are handled... 8-)
   Dip-switches 2-3-4-5 are at locations 0x01, 0x04, 0x10, 0x20
   Two remarkable values:
	0x01 0x04 0x10 0x20
	-------------------
	 0    0    0    0     1-1 1-1
	 0    1    1    1     Free play
*/
	{ 0, 0x02, "1ST EXTRA LIFE AT", { "10000", "15000" } },
	{ 0, 0x40, "ADDITIONAL LIFE EVERY", { "20K", "25K" } },
	{ 0, 0x80, "DIFFICULTY LEVEL", { "NORMAL", "HARD" } },
/* the following switch must be connected to the IP16 line */
/*      { 1, 0x40, "TEST MODE", {"ON", "OFF"} },*/
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8, 0xC000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x2000, &spritelayout, 0, 1 },
	{ -1 } /* end of array */
};



static const struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			5000000,        /* 5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU ,
			3579545/4,
			2,             /* memory region #2 */
			gottlieb_sound_readmem,gottlieb_sound_writemem,0,0,
			gottlieb_sh_interrupt,1
		}
	},
	60,     /* frames / second */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	16, 16,
	gottlieb_vh_init_color_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	qbert_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	gottlieb_sh_start,
	gottlieb_sh_stop,
	gottlieb_sh_update
};



ROM_START( qbertqub_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qq-rom3.bin", 0x8000, 0x2000, 0xac3cb8e2 )
	ROM_LOAD( "qq-rom2.bin", 0xa000, 0x2000, 0x64167070 )
	ROM_LOAD( "qq-rom1.bin", 0xc000, 0x2000, 0xdc7d6dc1 )
	ROM_LOAD( "qq-rom0.bin", 0xe000, 0x2000, 0xf2bad75a )

	ROM_REGION(0x12000)      /* temporary space for graphics */
	ROM_LOAD( "qq-bg0.bin", 0x0000, 0x1000, 0x13c600e6 )
	ROM_LOAD( "qq-bg1.bin", 0x1000, 0x1000, 0x542c9488 )
	ROM_LOAD( "qq-fg3.bin", 0x2000, 0x4000, 0xacd201f8 )       /* sprites */
	ROM_LOAD( "qq-fg2.bin", 0x6000, 0x4000, 0xa6a4660c )       /* sprites */
	ROM_LOAD( "qq-fg1.bin", 0xA000, 0x4000, 0x038fc633 )       /* sprites */
	ROM_LOAD( "qq-fg0.bin", 0xE000, 0x4000, 0x65b1f0f1 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "qb-snd1.bin", 0xf000, 0x800, 0x469952eb )
	ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "qb-snd2.bin", 0xf800, 0x800, 0x200e1d22 )
	ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */
ROM_END



static int hiload(void)
{
	void *f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
	unsigned char *RAM=Machine->memory_region[0];

	/* no need to wait for anything: Q*bert Qub doesn't touch the tables
		if the checksum is correct */
	if (f) {
		osd_fread(f,RAM+0x200,2*20*15); /* hi-score entries */
		osd_fread(f,RAM+0x458,8); /* checksum */
		osd_fclose(f);
	}
	return 1;
}

static void hisave(void)
{
	void *f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		osd_fwrite(f,RAM+0x200,2*20*15); /* hi-score entries */
		osd_fwrite(f,RAM+0x458,8); /* checksum */
		osd_fclose(f);
	}
}


struct GameDriver qbertqub_driver =
{
	"Q*Bert Qubes",
	"qbertqub",
	"FABRICE FRANCES",
	&machine_driver,

	qbertqub_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload,hisave     /* hi-score load and save */
};

