/***************************************************************************

Mad Planets' memory map

Thanks to Richard Davies who provided a keyboard/joystick substitute for the
dialer (anyone interested in adding a joystick's knob support in Mame ?-)

Main processor (8088 minimum mode) memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff RAM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram ?
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-ffff ROM (mad planets' prog)

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

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int mplanets_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh_w(int offset, int data);
void gottlieb_sh_update(void);
void gottlieb_output(int offset, int data);
int mplanets_IN1_r(int offset);
int mplanets_dial_r(int offset);
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
	{ 0x5801, 0x5801, mplanets_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball H: not used */
	{ 0x5803, 0x5803, mplanets_dial_r },     /* trackball V: dialer */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0x6000, 0xffff, MRA_ROM },
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
		0x80,
		{ OSD_KEY_3, OSD_KEY_4, /* coin 1 and 2 */
		  0,0,                  /* not connected ? */
		  0,0,                  /* not connected ? */
		  OSD_KEY_F2,           /* select */
		  0 },                  /* test mode */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball H: not used */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball V: dialer */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* joystick */
		0x00,
		{ OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_LEFT,
		OSD_KEY_LCONTROL,OSD_KEY_1,OSD_KEY_2,OSD_KEY_ALT},
		{ OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT,
					OSD_JOY_FIRE1, 0, 0, OSD_JOY_FIRE2 }
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
	{ -1 }
};


static struct KEYSet keys[] =
{
	{ 4, 0, "MOVE UP" },
	{ 4, 3, "MOVE LEFT"  },
	{ 4, 1, "MOVE RIGHT" },
	{ 4, 2, "MOVE DOWN" },
	{ 4, 4, "FIRE1"     },
	{ 4, 7, "FIRE2"     },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x08, "ROUND SELECT", { "OFF","ON" } },
	{ 0, 0x01, "ATTRACT MODE SOUND", { "ON", "OFF" } },
	{ 0, 0x1C, "", {
		"1 PLAY FOR 1 COIN" , "1 PLAY FOR 2 COINS",
		"1 PLAY FOR 1 COIN" , "1 PLAY FOR 2 COINS",
		"2 PLAYS FOR 1 COIN", "FREE PLAY",
		"2 PLAYS FOR 1 COIN", "FREE PLAY"
		} },
	{ 0, 0x20, "SHIPS PER GAME", { "3", "5" } },
	{ 0, 0x02, "EXTRA SHIP EVERY", { "10000", "12000" } },
	{ 0, 0xC0, "DIFFICULTY", { "STANDARD", "EASY", "HARD", "VERY HARD" } },
	/*{ 1, 0x80, "TEST MODE", {"ON", "OFF"} },*/
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
			3579545/4,        /* could it be /2 ? */
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
	mplanets_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	gottlieb_sh_start,
	gottlieb_sh_stop,
	gottlieb_sh_update
};

ROM_START( mplanets_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROM4", 0x6000, 0x2000, 0xf09b30bb )
	ROM_LOAD( "ROM3", 0x8000, 0x2000, 0x52223738 )
	ROM_LOAD( "ROM2", 0xa000, 0x2000, 0xe406bbb6 )
	ROM_LOAD( "ROM1", 0xc000, 0x2000, 0x385a7fa6 )
	ROM_LOAD( "ROM0", 0xe000, 0x2000, 0x29df430b )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "BG0", 0x0000, 0x1000, 0xb85b00c3 )
	ROM_LOAD( "BG1", 0x1000, 0x1000, 0x175bc547 )
	ROM_LOAD( "FG3", 0x2000, 0x2000, 0x7c6a72bc )       /* sprites */
	ROM_LOAD( "FG2", 0x4000, 0x2000, 0x6ab56cc7 )       /* sprites */
	ROM_LOAD( "FG1", 0x6000, 0x2000, 0x16c596b7 )       /* sprites */
	ROM_LOAD( "FG0", 0x8000, 0x2000, 0x96727f86 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "SND1", 0xf000, 0x800, 0xca36c072 )
		ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "SND2", 0xf800, 0x800, 0x66461044 )
		ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */

ROM_END



static int hiload(void)
{
	void *f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
	unsigned char *RAM=Machine->memory_region[0];

	/* no need to wait for anything: Mad Planets doesn't touch the tables
		if the checksum is correct */
	if (f) {
		osd_fread(f,RAM+0x536,2); /* hiscore table checksum */
		osd_fread(f,RAM+0x538,41*7); /* 20+20+1 hiscore entries */
		osd_fclose(f);
	}
	return 1;
}

static void hisave(void)
{
	void *f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
	/* not saving distributions tables : does anyone really want them ? */
		osd_fwrite(f,RAM+0x536,2); /* hiscore table checksum */
		osd_fwrite(f,RAM+0x538,41*7); /* 20+20+1 hiscore entries */
		osd_fclose(f);
	}
}


struct GameDriver mplanets_driver =
{
	"Mad Planets",
	"mplanets",
	"FABRICE FRANCES",
	&machine_driver,

	mplanets_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload,hisave     /* hi-score load and save */
};
