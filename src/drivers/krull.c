/***************************************************************************

Krull's memory map

Main processor (8088 minimum mode) memory map.
0000-0fff RAM
1000-1fff ROM
2000-2fff ROM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram ?
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-ffff ROM

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

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int krull_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh_w(int offset, int data);
void gottlieb_sh_update(void);
extern const char *gottlieb_sample_names[];
void gottlieb_output(int offset, int data);
int krull_IN1_r(int offset);
extern unsigned char *gottlieb_paletteram;
extern unsigned char *gottlieb_characterram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_characterram_w(int offset, int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

int gottlieb_sh_start(void);
void gottlieb_sh_stop(void);
void gottlieb_sh_update(void);
int gottlieb_sh_interrupt(void);
int gottlieb_sh_interrupt(void);
int riot_ram_r(int offset);
int gottlieb_riot_r(int offset);
int gottlieb_sound_expansion_socket_r(int offset);
void riot_ram_w(int offset, int data);
void gottlieb_riot_w(int offset, int data);
void gottlieb_amplitude_DAC_w(int offset, int data);
void gottlieb_speech_w(int offset, int data);
void gottlieb_speech_clock_DAC_w(int offset, int data);
void gottlieb_sound_expansion_socket_w(int offset, int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, krull_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, gottlieb_characterram_w, &gottlieb_characterram },
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress krull_sound_readmem[] =
{
	{ 0x0000, 0x01ff, riot_ram_r },
	{ 0x0200, 0x03ff, gottlieb_riot_r },
	{ 0x4000, 0x5fff, gottlieb_sound_expansion_socket_r },
	{ 0x6000, 0x7fff, MRA_ROM },
			 /* A15 not decoded except in socket expansion */
	{ 0x8000, 0x81ff, riot_ram_r },
	{ 0x8200, 0x83ff, gottlieb_riot_r },
	{ 0xc000, 0xdfff, gottlieb_sound_expansion_socket_r },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

struct MemoryWriteAddress krull_sound_writemem[] =
{
	{ 0x0000, 0x01ff, riot_ram_w },
	{ 0x0200, 0x03ff, gottlieb_riot_w },
	{ 0x1000, 0x1000, gottlieb_amplitude_DAC_w },
	{ 0x2000, 0x2000, gottlieb_speech_w },
	{ 0x3000, 0x3000, gottlieb_speech_clock_DAC_w },
	{ 0x4000, 0x5fff, gottlieb_sound_expansion_socket_w },
	{ 0x6000, 0x7fff, MWA_ROM },
			 /* A15 not decoded except in socket expansion */
	{ 0x8000, 0x81ff, riot_ram_w },
	{ 0x8200, 0x83ff, gottlieb_riot_w },
	{ 0x9000, 0x9000, gottlieb_amplitude_DAC_w },
	{ 0xa000, 0xa000, gottlieb_speech_w },
	{ 0xb000, 0xb000, gottlieb_speech_clock_DAC_w },
	{ 0xc000, 0xdfff, gottlieb_sound_expansion_socket_w },
	{ 0xe000, 0xffff, MWA_ROM },
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
		0x01,
		{ 0,                    /* diag mode */
		  OSD_KEY_F2,            /* select */
		  OSD_KEY_3, OSD_KEY_4, /* coin 1 & 2 */
		  0,0,                  /* not connected ? */
		  OSD_KEY_1,
		  OSD_KEY_2 },
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
	{       /* joysticks */
		0x00,
		{
		OSD_KEY_E,OSD_KEY_F,OSD_KEY_D,OSD_KEY_S,
		OSD_KEY_UP,OSD_KEY_RIGHT,OSD_KEY_DOWN,OSD_KEY_LEFT},
		{ OSD_JOY_FIRE2, OSD_JOY_FIRE4, OSD_JOY_FIRE3, OSD_JOY_FIRE1, /* V.V */
		OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT }       /* V.V */
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
	{ -1 }
};

static struct KEYSet keys[] =
{
	{ 4, 4, "MOVE UP" },
	{ 4, 7, "MOVE LEFT"  },
	{ 4, 5, "MOVE RIGHT" },
	{ 4, 6, "MOVE DOWN" },
	{ 4, 0, "FIRE UP" },
	{ 4, 3, "FIRE LEFT"  },
	{ 4, 1, "FIRE RIGHT" },
	{ 4, 2, "FIRE DOWN" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x08, "LIVES PER GAME", { "3","5" } },
	{ 0, 0x01, "ATTRACT MODE SOUND", { "ON", "OFF" } },
	{ 0, 0x1C, "", {
		"1 PLAY FOR 1 COIN" , "1 PLAY FOR 2 COINS",
		"1 PLAY FOR 1 COIN" , "1 PLAY FOR 2 COINS",
		"2 PLAYS FOR 1 COIN", "FREE PLAY",
		"2 PLAYS FOR 1 COIN", "FREE PLAY"
		} },
	{ 0, 0x20, "HEXAGON", { "ROVING", "STATIONARY" } },
	{ 0, 0x02, "DIFFICULTY", { "NORMAL", "HARD" } },
	{ 0, 0xC0, "", {
		"LIFE AT 30000 THEN EVERY 50000",
		"LIFE AT 30000 THEN EVERY 30000",
		"LIFE AT 40000 THEN EVERY 50000",
		"LIFE AT 50000 THEN EVERY 75000"
		} },
	/*{ 1, 0x01, "TEST MODE", {"ON", "OFF"} },*/
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
	{ 0, 0x4000, &charlayout, 0, 1 },       /* the game dynamically modifies this */
	{ 1, 0, &spritelayout, 0, 1 },
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
			krull_sound_readmem,krull_sound_writemem,0,0,
			gottlieb_sh_interrupt,1
		}

	},
	60,     /* frames / second */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1+16, 16,
	gottlieb_vh_init_color_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	krull_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	gottlieb_sh_start,
	gottlieb_sh_stop,
	gottlieb_sh_update
};



ROM_START( krull_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "RAM2.BIN", 0x1000, 0x1000, 0x03fa87a8 )
	ROM_LOAD( "RAM4.BIN", 0x2000, 0x1000, 0x8d50227a )
	ROM_LOAD( "ROM4.BIN", 0x6000, 0x2000, 0x5e10647c )
	ROM_LOAD( "ROM3.BIN", 0x8000, 0x2000, 0xdda2011c )
	ROM_LOAD( "ROM2.BIN", 0xa000, 0x2000, 0x2ab22372 )
	ROM_LOAD( "ROM1.BIN", 0xc000, 0x2000, 0x5341023f )
	ROM_LOAD( "ROM0.BIN", 0xe000, 0x2000, 0x16e7bc1d )

	ROM_REGION(0x8000)      /* temporary space for graphics */
	ROM_LOAD( "FG3.BIN", 0x0000, 0x2000, 0xf7bee74c )       /* sprites */
	ROM_LOAD( "FG2.BIN", 0x2000, 0x2000, 0xcf79bc05 )       /* sprites */
	ROM_LOAD( "FG1.BIN", 0x4000, 0x2000, 0xf2f27094 )       /* sprites */
	ROM_LOAD( "FG0.BIN", 0x6000, 0x2000, 0xdae82e5a )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "snd1.bin", 0xe000, 0x1000, 0x7390800c )
		ROM_RELOAD(0x6000, 0x1000) /* A15 is not decoded */
	ROM_LOAD( "snd2.bin", 0xf000, 0x1000, 0xe65ea116 )
		ROM_RELOAD(0x7000, 0x1000) /* A15 is not decoded */

ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0b3d],"\x7F\x7F\x7F\x00\x00\x00\x00\x00\x00\x00",10) == 0 &&
			memcmp(&RAM[0x0c2d],"\x7F\x7F\x7F\x00\x00\x00\x00\x00\x00\x00",10) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0ace],10*10,1,f);
			fread(&RAM[0x0b3d],10*25,1,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x0ace],10*10,1,f);
		fwrite(&RAM[0x0b3d],10*25,1,f);
		fclose(f);
	}
}



struct GameDriver krull_driver =
{
	"Krull",
	"krull",
	"FABRICE FRANCES",
	&machine_driver,

	krull_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
