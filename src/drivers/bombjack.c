/***************************************************************************

Bomb Jack memory map (preliminary)
MAIN BOARD:

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-83ff RAM 0
8400-87ff RAM 1
8800-8bff RAM 2
8c00-8fff RAM 3
9000-93ff Video RAM (RAM 4)
9400-97ff Color RAM (RAM 4)
c000-dfff ROM 4

memory mapped ports:
read:
b000      IN0
b001      IN1
b002      IN2
b003      watchdog reset?
b004      DSW1
b005      DSW2

write:
9820-987f sprites
9a00      ? number of small sprites for video controller
9c00-9cff palette
9e00      background image selector
b000      interrupt enable
b800      command to soundboard & trigger NMI on sound board



SOUND BOARD:
0x0000 0x1fff ROM
0x2000 0x4400 RAM

memory mapped ports:
read:
0x6000 command from soundboard
write :
none

IO ports:
write:
0x00 AY#1 control
0x01 AY#1 write
0x10 AY#2 control
0x11 AY#2 write
0x80 AY#3 control
0x81 AY#3 write

interrupts:
NMI triggered by the commands sent by MAIN BOARD (?)
NMI interrupts for music timing

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *bombjack_paletteram;
extern void bombjack_paletteram_w(int offset,int data);
extern void bombjack_background_w(int offset,int data);
extern void bombjack_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int bombjack_sh_intflag_r(int offset);
extern int bombjack_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xb003, 0xb003, MRA_NOP },
	{ 0xb000, 0xb000, input_port_0_r },	/* player 1 input */
	{ 0xb001, 0xb001, input_port_1_r },	/* player 2 input */
	{ 0xb002, 0xb002, input_port_2_r },	/* coin */
	{ 0xb004, 0xb004, input_port_3_r },	/* DSW1 */
	{ 0xb005, 0xb005, input_port_4_r },	/* DSW2 */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },	/* including video and color RAM */
	{ 0xc000, 0xdfff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x9820, 0x987f, MWA_RAM, &spriteram },
	{ 0x9c00, 0x9cff, bombjack_paletteram_w, &bombjack_paletteram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0x9e00, 0x9e00, bombjack_background_w },
	{ 0xb800, 0xb800, sound_command_w },
	{ 0x9a00, 0x9a00, MWA_NOP },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bombjack_sound_readmem[] =
{
	{ 0x4390, 0x4390, bombjack_sh_intflag_r },	/* kludge to speed up the emulation */
	{ 0x2000, 0x5fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x6000, 0x6000, sound_command_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bombjack_sound_writemem[] =
{
	{ 0x2000, 0x5fff, MWA_RAM },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOWritePort bombjack_sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x11, 0x11, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_2_w },
	{ 0x81, 0x81, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xc0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "3", "4", "5", "2" } },
	{ 4, 0x18, "DIFFICULTY 1", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ 4, 0x60, "DIFFICULTY 2", { "MEDIUM", "EASY", "HARD", "HARDEST" } },
		/* not a mistake, MEDIUM and EASY are swapped */
	{ 4, 0x80, "SPECIAL", { "EASY", "HARD" } },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" } },
	{ 4, 0x07, "INITIAL HIGH SCORE", { "10000", "100000", "30000", "50000", "100000", "50000", "100000", "50000" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 2*512*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	32,	/* 32 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 87*8, 86*8, 85*8, 84*8, 83*8, 82*8, 81*8, 80*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the remapped color table and dynamically build the real one. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,      0, 16 },	/* characters */
	{ 1, 0x3000, &charlayout2,      0, 16 },	/* background tiles */
	{ 1, 0x9000, &spritelayout1,    0, 16 },	/* normal sprites */
	{ 1, 0xa000, &spritelayout2,    0, 16 },	/* large sprites */
	{ 0, 0,      &fakelayout,    16*8, 182 },
	{ -1 } /* end of array */
};



/* Bomb Jack doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. Here is the list of all the colors is uses. */
static unsigned char color_prom[] =
{
	/* total: 182 colors (2 bytes per color) */
	0x00,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x08,0x00,0x09,0x00,
	0x0b,0x00,0x0c,0x00,0x0d,0x00,0x0f,0x00,0x10,0x00,0x17,0x00,0x20,0x00,0x27,0x00,
	0x28,0x00,0x2a,0x00,0x2f,0x00,0x30,0x00,0x37,0x00,0x3f,0x00,0x40,0x00,0x45,0x00,
	0x47,0x00,0x4a,0x00,0x4f,0x00,0x50,0x00,0x55,0x00,0x57,0x00,0x5f,0x00,0x60,0x00,
	0x66,0x00,0x67,0x00,0x6f,0x00,0x70,0x00,0x77,0x00,0x7f,0x00,0x80,0x00,0x88,0x00,
	0x8f,0x00,0x90,0x00,0x99,0x00,0x9f,0x00,0xa0,0x00,0xaa,0x00,0xaf,0x00,0xb0,0x00,
	0xbb,0x00,0xbf,0x00,0xc0,0x00,0xcc,0x00,0xcf,0x00,0xd0,0x00,0xdd,0x00,0xdf,0x00,
	0xe0,0x00,0xee,0x00,0xf0,0x00,0xf2,0x00,0xf4,0x00,0xf6,0x00,0xf8,0x00,0xfa,0x00,
	0xfb,0x00,0xfc,0x00,0xff,0x00,0x77,0x01,0x0f,0x02,0x22,0x02,0x2f,0x02,0x67,0x02,
	0x77,0x02,0xf0,0x02,0xf2,0x02,0xff,0x02,0x00,0x03,0x33,0x03,0x77,0x03,0x0f,0x04,
	0x44,0x04,0x4f,0x04,0x77,0x04,0x89,0x04,0xf0,0x04,0xf4,0x04,0xff,0x04,0x00,0x05,
	0x55,0x05,0x5f,0x05,0x77,0x05,0x00,0x06,0x0f,0x06,0x66,0x06,0x6f,0x06,0x77,0x06,
	0xab,0x06,0xf0,0x06,0xf6,0x06,0xff,0x06,0x00,0x07,0x77,0x07,0x00,0x08,0x0f,0x08,
	0x88,0x08,0x8f,0x08,0xaa,0x08,0xcd,0x08,0xce,0x08,0xf0,0x08,0xf8,0x08,0xff,0x08,
	0x00,0x09,0x99,0x09,0x00,0x0a,0x0f,0x0a,0xaa,0x0a,0xaf,0x0a,0xef,0x0a,0xf0,0x0a,
	0xfa,0x0a,0xff,0x0a,0x00,0x0b,0x58,0x0b,0xbb,0x0b,0x00,0x0c,0x08,0x0c,0x0f,0x0c,
	0x60,0x0c,0x88,0x0c,0xaf,0x0c,0xcc,0x0c,0xcf,0x0c,0xf0,0x0c,0xfc,0x0c,0xff,0x0c,
	0x00,0x0d,0xdd,0x0d,0x47,0x0e,0xee,0x0e,0xef,0x0e,0xfe,0x0e,0xff,0x0e,0x00,0x0f,
	0x02,0x0f,0x04,0x0f,0x06,0x0f,0x08,0x0f,0x0a,0x0f,0x0c,0x0f,0x0f,0x0f,0x20,0x0f,
	0x22,0x0f,0x2f,0x0f,0x40,0x0f,0x44,0x0f,0x4f,0x0f,0x50,0x0f,0x60,0x0f,0x66,0x0f,
	0x6f,0x0f,0x80,0x0f,0x88,0x0f,0x8f,0x0f,0xa0,0x0f,0xaa,0x0f,0xaf,0x0f,0xb0,0x0f,
	0xc0,0x0f,0xcc,0x0f,0xcf,0x0f,0xee,0x0f,0xef,0x0f,0xf0,0x0f,0xf2,0x0f,0xf4,0x0f,
	0xf6,0x0f,0xf8,0x0f,0xfa,0x0f,0xfc,0x0f,0xfe,0x0f,0xff,0x0f
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz????? */
			3,	/* memory region #3 */
			bombjack_sound_readmem,bombjack_sound_writemem,0,bombjack_sound_writeport,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0, 32*8-1 },
	gfxdecodeinfo,
	182,16*8+182,
	bombjack_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	bombjack_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	bombjack_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bombjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "09_j01b.bin",  0x0000, 0x2000 )
	ROM_LOAD( "10_l01b.bin",  0x2000, 0x2000 )
	ROM_LOAD( "11_m01b.bin",  0x4000, 0x2000 )
	ROM_LOAD( "12_n01b.bin",  0x6000, 0x2000 )
	ROM_LOAD( "13_r01b.bin",  0xc000, 0x2000 )

	ROM_REGION(0xf000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "03_e08t.bin",  0x0000, 0x1000 )	/* chars */
	ROM_LOAD( "04_h08t.bin",  0x1000, 0x1000 )
	ROM_LOAD( "05_k08t.bin",  0x2000, 0x1000 )
	ROM_LOAD( "06_l08t.bin",  0x3000, 0x2000 )	/* background tiles */
	ROM_LOAD( "07_n08t.bin",  0x5000, 0x2000 )
	ROM_LOAD( "08_r08t.bin",  0x7000, 0x2000 )
	ROM_LOAD( "16_m07b.bin",  0x9000, 0x2000 )	/* sprites */
	ROM_LOAD( "15_l07b.bin",  0xb000, 0x2000 )
	ROM_LOAD( "14_j07b.bin",  0xd000, 0x2000 )

	ROM_REGION(0x1000)	/* background graphics */
	ROM_LOAD( "02_p04t.bin",  0x0000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for sound board */
	ROM_LOAD( "01_h03t.bin",  0x0000, 0x2000 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8100],"\x00\x00\x01\x00",4) == 0 &&
			memcmp(&RAM[0x8124],"\x00\x00\x01\x00",4) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			char buf[10];
			int hi;


			fread(&RAM[0x8100],1,15*10,f);
			RAM[0x80e2] = RAM[0x8100];
			RAM[0x80e3] = RAM[0x8101];
			RAM[0x80e4] = RAM[0x8102];
			RAM[0x80e5] = RAM[0x8103];
			/* also copy the high score to the screen, otherwise it won't be */
			/* updated until a new game is started */
			hi = (RAM[0x8100] & 0x0f) +
					(RAM[0x8100] >> 4) * 10 +
					(RAM[0x8101] & 0x0f) * 100 +
					(RAM[0x8101] >> 4) * 1000 +
					(RAM[0x8102] & 0x0f) * 10000 +
					(RAM[0x8102] >> 4) * 100000 +
					(RAM[0x8103] & 0x0f) * 1000000 +
					(RAM[0x8103] >> 4) * 10000000;
			sprintf(buf,"%8d",hi);
			videoram_w(0x013f,buf[0]);
			videoram_w(0x011f,buf[1]);
			videoram_w(0x00ff,buf[2]);
			videoram_w(0x00df,buf[3]);
			videoram_w(0x00bf,buf[4]);
			videoram_w(0x009f,buf[5]);
			videoram_w(0x007f,buf[6]);
			videoram_w(0x005f,buf[7]);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x8100],1,15*10,f);
		fclose(f);
	}
}



struct GameDriver bombjack_driver =
{
	"Bomb Jack",
	"bombjack",
	"BRAD THOMAS\nJAKOB FRENDSEN\nCONNY MELIN\nMIRKO BUFFONI\nNICOLA SALMORIA\nJAREK BURCZYNSKI",
	&machine_driver,

	bombjack_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom,0,0,
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	5, 0,
	8*13, 8*16,2,

	hiload, hisave
};
