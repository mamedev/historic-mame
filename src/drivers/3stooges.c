/***************************************************************************

3 Stooges' memory map

Main processor (8088 minimum mode) memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff ROM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-ffff ROM

memory mapped ports:

read:
5800    Dip switch
5801    Inputs 10-17
5802    trackball input
5803    trackball input
5804    Inputs 40-47

write:
5800    watchdog timer clear
5801    trackball output
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

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int stooges_vh_start(void);
void gottlieb_vh_init_basic_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh_w(int offset, int data);
void gottlieb_sh_update(void);
extern const char *gottlieb_sample_names[];
void gottlieb_output(int offset, int data);
int stooges_IN1_r(int offset);
int stooges_joysticks(int offset);
extern unsigned char *gottlieb_characterram;
extern unsigned char *gottlieb_paletteram;
void gottlieb_characterram_w(int offset,int data);
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, stooges_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball H: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball V: not used */
	{ 0x5804, 0x5804, stooges_joysticks },  /* joysticks demultiplexer */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x37ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3fff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, gottlieb_characterram_w, &gottlieb_characterram },
	{ 0x5000, 0x57ff, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball output not used */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct InputPort input_ports[] =
{
	{       /* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x11, /* tilt off, test mode off */
		{ 0, OSD_KEY_F2, /* test mode, select */
		  OSD_KEY_4,OSD_KEY_3, /* coin 1 & 2 */
		  OSD_KEY_T, /* tilt : does someone really want that ??? */
		  0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball H: not used */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball V: not used */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* joystick 2 (Moe) */
		0x00,
		{ OSD_KEY_I, OSD_KEY_L, OSD_KEY_K, OSD_KEY_J,
		OSD_KEY_ALT,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* joystick 1 (Curly) */
		0x00,
		{ OSD_KEY_E, OSD_KEY_F, OSD_KEY_D, OSD_KEY_S,
		0,OSD_KEY_CONTROL,0,0 },	/* Larry fire */
		{ 0, 0, 0, 0, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{       /* joystick 3 (Larry) */
		0x00,
		{ OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_LEFT,
		0,0,OSD_KEY_ENTER,0 },	/* Curly fire */
		{ OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
	{ 4, 0, "MOE UP" },
	{ 4, 3, "MOE LEFT"  },
	{ 4, 1, "MOE RIGHT" },
	{ 4, 2, "MOE DOWN" },
	{ 4, 4, "MOE FIRE"     },
	{ 5, 0, "CURLY UP" },
	{ 5, 3, "CURLY LEFT"  },
	{ 5, 1, "CURLY RIGHT" },
	{ 5, 2, "CURLY DOWN" },
	{ 6, 6, "CURLY FIRE"     },
	{ 6, 0, "LARRY UP" },
	{ 6, 3, "LARRY LEFT"  },
	{ 6, 1, "LARRY RIGHT" },
	{ 6, 2, "LARRY DOWN" },
	{ 5, 5, "LARRY FIRE" },
	{ -1 }
};

static struct DSW dsw[] =
{
	{ 0, 0x01, "ATTRACT MODE SOUND", { "ON", "OFF" } },
	{ 0, 0x02, "DIFFICULTY", { "NORMAL", "HARD" } },
	{ 0, 0x08, "LIVES PER GAME", { "3", "5" } },
	{ 0, 0x1C, "", {
		"1 PLAY FOR 1 COIN" , "2 PLAYS FOR 1 COIN",
		"1 PLAY FOR 1 COIN" , "2 PLAYS FOR 1 COIN",
		"1 PLAY FOR 2 COINS", "FREE PLAY",
		"1 PLAY FOR 2 COINS", "FREE PLAY"
		} },
	{ 0, 0x40, "FIRST EXTRA LIVE AT", { "20000","10000" } },
	{ 0, 0x80, "ADD. EXTRA LIVE EVERY", { "20000", "10000" } },
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
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x2000*8, 0x4000*8, 0x6000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 }, /* 1 palette for the game */
	{ 1, 0x0000, &spritelayout, 0, 1 },
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
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	256,256,        /* 256 for colormap, 1*16 for the game, 2*16 for the dsw menu. Silly, isn't it ? */
	gottlieb_vh_init_basic_color_palette,

	0,      /* init vh */
	stooges_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	0,
	0,
	gottlieb_sh_update
};

ROM_START( stooges_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "GV113RAM.4", 0x2000, 0x1000, 0x64249570 )
	ROM_LOAD( "GV113ROM.4", 0x6000, 0x2000, 0x8fdb5ff5 )
	ROM_LOAD( "GV113ROM.3", 0x8000, 0x2000, 0x8d135dd7 )
	ROM_LOAD( "GV113ROM.2", 0xa000, 0x2000, 0x093ee71e )
	ROM_LOAD( "GV113ROM.1", 0xc000, 0x2000, 0x65319da1 )
	ROM_LOAD( "GV113ROM.0", 0xe000, 0x2000, 0x20f3727b )

	ROM_REGION(0x8000)      /* temporary space for graphics */
	ROM_LOAD( "GV113FG3", 0x0000, 0x2000, 0xf3e09a2a )       /* sprites */
	ROM_LOAD( "GV113FG2", 0x2000, 0x2000, 0x5bde03f8 )       /* sprites */
	ROM_LOAD( "GV113FG1", 0x4000, 0x2000, 0x3904746a )       /* sprites */
	ROM_LOAD( "GV113FG0", 0x6000, 0x2000, 0xa2b57805 )       /* sprites */
ROM_END

static int hiload(const char *name)
{
	FILE *f=fopen(name,"rb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fread(RAM+0x485,22,7,f); /* 21 hiscore entries + 1 (?) */
		fclose(f);
	}
	return 1;
}

static void hisave(const char *name)
{
	FILE *f=fopen(name,"wb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fwrite(RAM+0x485,22,7,f); /* hiscore entries */
		fclose(f);
	}
}


struct GameDriver stooges_driver =
{
	"Three Stooges",
	"3stooges",
	"FABRICE FRANCES",
	&machine_driver,

	stooges_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, trak_ports, dsw, keys,

	(char *)1,
	0,0,    /* palette, colortable */

	8*11,8*20,

        hiload, hisave
};
