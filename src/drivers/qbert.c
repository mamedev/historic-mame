/***************************************************************************

Q*bert's driver : dedicated to Jeff Lee & Warren Davis

****************************************************************************

Q*bert machine's memory map (from my understanding of the schematics... FF )

Main processor (8088 minimum mode)  memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff RAM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram ?
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-7fff (empty rom slot)
8000-9fff (empty rom slot)
a000-ffff ROM (qbert's prog)

memory mapped ports:

read:
5800    Dip switch
5801    Inputs 10-17
5802    trackball input (optional)
5803    trackball input (optional)
5804    Inputs 40-47

write:
5800    watchdog timer clear
5801    trackball output (optional)
5802    Outputs 20-27
5803    Flipflop outputs:
		b7: F/B priority
		b6: horiz. flipflop
		b5: vert. flipflop
		b4: Output 33
		b3: coin meter
		b2: knocker
		b1: coin 1
		b0: coin lockout
5804    Outputs 40-47

interrupts:
INTR not connected
NMI connected to vertical blank

Sound processor (6502) memory map:
0000-0fff RIOT (6532)
1000-1fff amplitude DAC
2000-2fff SC01 voice chip
3000-3fff voice clock DAC
4000-4fff socket expansion
5000-5fff socket expansion
6000-6fff socket expansion
7000-7fff PROM
(repeated in 8000-ffff, A15 only used in socket expansion)

Nota: I've used my own 6502 emulator in order to compute the digital effects
because the clock emulation is much more precise and allows to put timestamps
on amplitude DAC writes. MAME doesn't allow to compute the digital effects in
real time like Euphoric (oops, <ad. mode end>) so the effects are provided
as precomputed samples)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int qbert_vh_start(void);
void gottlieb_vh_init_optimized_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_vh_init_basic_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void qbert_sh_w(int offset, int data);
void gottlieb_sh_update(void);
void gottlieb_output(int offset, int data);
int qbert_IN1_r(int offset);
extern unsigned char *gottlieb_paletteram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, qbert_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0xA000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x37ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3fff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x57ff, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, qbert_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0xa000, 0xffff, MWA_ROM },
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
	{ 0, 0x08, "AUTO ROUND ADVANCE", { "OFF","ON" } },
	{ 0, 0x01, "ATTRACT MODE SOUND", { "ON", "OFF" } },
	{ 0, 0x10, "FREE PLAY", { "OFF" , "ON" } },
	{ 0, 0x04, "", { "UPRIGHT", "COCKTAIL" } },
	{ 0, 0x02, "KICKER", { "OFF", "ON" } },
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
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	{ 28, 24, 20, 16, 12, 8, 4, 0},
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x2000*8, 0x4000*8, 0x6000*8 },
	{ 0 * 16, 1 * 16, 2 * 16, 3 * 16, 4 * 16, 5 * 16, 6 * 16, 7 * 16,
		8 * 16, 9 * 16, 10 * 16, 11 * 16, 12 * 16, 13 * 16, 14 * 16, 15 * 16 },
	{ 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
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
		}
	},
	60,     /* frames / second */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 16,
	gottlieb_vh_init_basic_color_palette,

	0,      /* init vh */
	qbert_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	0,
	0,
	gottlieb_sh_update
};



ROM_START( qbert_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qb-rom2.bin", 0xa000, 0x2000, 0xd1c1dad7 )
	ROM_LOAD( "qb-rom1.bin", 0xc000, 0x2000, 0xdc2bbad9 )
	ROM_LOAD( "qb-rom0.bin", 0xe000, 0x2000, 0xc23a8cfe )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "qb-bg0.bin", 0x0000, 0x1000, 0x035735a1 )
	ROM_LOAD( "qb-bg1.bin", 0x1000, 0x1000, 0xaac748c5 )
	ROM_LOAD( "qb-fg3.bin", 0x2000, 0x2000, 0x54bd5daf )       /* sprites */
	ROM_LOAD( "qb-fg2.bin", 0x4000, 0x2000, 0x200a62ae )       /* sprites */
	ROM_LOAD( "qb-fg1.bin", 0x6000, 0x2000, 0x7a17df07 )       /* sprites */
	ROM_LOAD( "qb-fg0.bin", 0x8000, 0x2000, 0x0ca72f4f )       /* sprites */
ROM_END

ROM_START( qbertjp_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qb-rom2.bin", 0xa000, 0x2000, 0x22b59259 )
	ROM_LOAD( "qb-rom1.bin", 0xc000, 0x2000, 0xa9ffed43 )
	ROM_LOAD( "qb-rom0.bin", 0xe000, 0x2000, 0xf20e301e )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "qb-bg0.bin", 0x0000, 0x1000, 0x035735a1 )
	ROM_LOAD( "qb-bg1.bin", 0x1000, 0x1000, 0xaac748c5 )
	ROM_LOAD( "qb-fg3.bin", 0x2000, 0x2000, 0x54bd5daf )       /* sprites */
	ROM_LOAD( "qb-fg2.bin", 0x4000, 0x2000, 0x200a62ae )       /* sprites */
	ROM_LOAD( "qb-fg1.bin", 0x6000, 0x2000, 0x7a17df07 )       /* sprites */
	ROM_LOAD( "qb-fg0.bin", 0x8000, 0x2000, 0x0ca72f4f )       /* sprites */
ROM_END



const char *gottlieb_sample_names[] =
{
	"FX_00.SAM",
	"FX_01.SAM",
	"FX_02.SAM",
	"FX_03.SAM",
	"FX_04.SAM",
	"FX_05.SAM",
	"FX_06.SAM",
	"FX_07.SAM",
	"FX_08.SAM",
	"FX_09.SAM",
	"FX_10.SAM",
	"FX_11.SAM",
	"FX_12.SAM",
	"FX_13.SAM",
	"FX_14.SAM",
	"FX_15.SAM",
	"FX_16.SAM",
	"FX_17.SAM",
	"FX_18.SAM",
	"FX_19.SAM",
	"FX_20.SAM",
	"FX_21.SAM",
	"FX_22.SAM",
	"FX_23.SAM",
	"FX_24.SAM",
	"FX_25.SAM",
	"FX_26.SAM",
	"FX_27.SAM",
	"FX_28.SAM",
	"FX_29.SAM",
	"FX_30.SAM",
	"FX_31.SAM",
	"FX_32.SAM",
	"FX_33.SAM",
	"FX_34.SAM",
	"FX_35.SAM",
	"FX_36.SAM",
	"FX_37.SAM",
	"FX_38.SAM",
	"FX_39.SAM",
	"FX_40.SAM",
	"FX_41.SAM",
	0       /* end of array */
};



/* here is a table of colors used by Q*bert. The table might not be
   complete... but that's surely better than mapping a 4*4*4 color cube
   on a 3*3*2 one ...*/

static unsigned short qbert_colors[256]={
	0x000, 0xfff, 0xff0,
	0x00f, 0x010, 0x020, 0x030, 0x039, 0x040, 0x04d,
	0x04e, 0x050, 0x060, 0x06e, 0x070, 0x07f, 0x080, 0x090,
	0x0a0, 0x0b0, 0x0c0, 0x0d0, 0x0e0, 0x0f0, 0x0ff, 0x100,
	0x111, 0x119, 0x200, 0x222, 0x26c, 0x28c, 0x2b3, 0x300,
	0x327, 0x333, 0x344, 0x400, 0x410, 0x444, 0x4b0, 0x510,
	0x520, 0x549, 0x54e, 0x555, 0x5a9, 0x620, 0x630, 0x666,
	0x706, 0x730, 0x731, 0x741, 0x777, 0x788, 0x7f0, 0x807,
	0x841, 0x842, 0x852, 0x888, 0x8fb, 0x8fc, 0x8fd, 0x8fe,
	0x8ff, 0x900, 0x901, 0x902, 0x903, 0x904, 0x905, 0x906,
	0x907, 0x908, 0x909, 0x90a, 0x90b, 0x90c, 0x90d, 0x90e,
	0x90f, 0x910, 0x911, 0x912, 0x913, 0x914, 0x915, 0x916,
	0x917, 0x918, 0x919, 0x91a, 0x91b, 0x91c, 0x91d, 0x91e,
	0x91f, 0x920, 0x921, 0x922, 0x923, 0x924, 0x925, 0x926,
	0x927, 0x928, 0x929, 0x92a, 0x92b, 0x92c, 0x92d, 0x92e,
	0x92f, 0x930, 0x931, 0x932, 0x933, 0x934, 0x935, 0x936,
	0x937, 0x938, 0x939, 0x93a, 0x93b, 0x93c, 0x93d, 0x93e,
	0x93f, 0x940, 0x941, 0x942, 0x943, 0x944, 0x945, 0x946,
	0x947, 0x948, 0x949, 0x94a, 0x94b, 0x94c, 0x94d, 0x94e,
	0x94f, 0x950, 0x951, 0x952, 0x953, 0x954, 0x955, 0x956,
	0x957, 0x958, 0x959, 0x95a, 0x95b, 0x95c, 0x95d, 0x95e,
	0x95f, 0x960, 0x961, 0x962, 0x963, 0x964, 0x965, 0x966,
	0x967, 0x968, 0x969, 0x96a, 0x96b, 0x96c, 0x96d, 0x96e,
	0x96f, 0x970, 0x971, 0x972, 0x973, 0x974, 0x999, 0xa63,
	0xa64, 0xa74, 0xaaa, 0xab1, 0xb0b, 0xb33, 0xb74, 0xbb2,
	0xbbb, 0xbcc, 0xc20, 0xcc0, 0xccc, 0xdd0, 0xddd, 0xe12,
	0xe50, 0xe69, 0xed7, 0xeee, 0xf00, 0xf07, 0xf0f, 0xf66,
	0xf70, 0xf72, 0xf86
};


static int hiload(const char *name)
{
	FILE *f=fopen(name,"rb");
	unsigned char *RAM=Machine->memory_region[0];

	/* no need to wait for anything: Q*bert doesn't touch the tables
	if the checksum is correct */
	if (f) {
		fread(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fread(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fread(RAM+0xBB0,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
	return 1;
}

static void hisave(const char *name)
{
	FILE *f=fopen(name,"wb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
	/* not saving distributions tables : does anyone really want them ? */
		fwrite(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fwrite(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fwrite(RAM+0xBB0,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
}

static int hiload_jp(const char *name)
{
	FILE *f=fopen(name,"rb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fread(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fread(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fread(RAM+0xC0C,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
	return 1;
}

static void hisave_jp(const char *name)
{
	FILE *f=fopen(name,"wb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fwrite(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fwrite(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fwrite(RAM+0xC0C,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
}

struct GameDriver qbert_driver =
{
	"Q*Bert (US version)",
	"qbert",
	"FABRICE FRANCES",
	&machine_driver,

	qbert_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, trak_ports, dsw, keys,

	(char *)qbert_colors,
	0,0,    /* palette, colortable */

	8*11,8*20,

	hiload,hisave     /* hi-score load and save */
};

struct GameDriver qbertjp_driver =
{
	"Q*Bert (Japanese version)",
	"qbertjp",
	"FABRICE FRANCES",
	&machine_driver,

	qbertjp_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, trak_ports, dsw, keys,

	(char *)qbert_colors,
	0,0,    /* palette, colortable */
	8*11,8*20,

	hiload_jp,hisave_jp     /* hi-score load and save */
};
