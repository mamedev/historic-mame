/***************************************************************************

Mr. Do's Castle memory map (preliminary)

FIRST CPU:
0000-7fff ROM
8000-97ff RAM
9800-99ff Sprites
b000-b3ff Video RAM
b400-b7ff Color RAM

read:
a000-a008 data from second CPU

write:
a000-a008 data for second CPU
a800      Watchdog reset?
e000      Trigger NMI on second CPU (?)

SECOND CPU:
0000-3fff ROM
8000-87ff RAM

read:
a000-a008 data from first CPU
c001      DSWB (unused?)
c002      DSWA
          bit 6-7 = lives
		  bit 5 = upright/cocktail (0 = upright)
          bit 4 = difficulty of EXTRA (1 = easy)
          bit 3 = unused?
          bit 2 = RACK TEST
		  bit 0-1 = difficulty
c003      IN0
          bit 4-7 = joystick player 2
          bit 0-3 = joystick player 1
c004      ?
c005	  IN1
          bit 7 = START 2
		  bit 6 = unused
		  bit 5 = jump player 2
		  bit 4 = fire player 2
		  bit 3 = START 1
		  bit 2 = unused
          bit 1 = jump player 1(same effect as fire)
          bit 0 = fire player 1
c007      IN2
          bit 7 = unused
          bit 6 = unused
          bit 5 = COIN 2
          bit 4 = COIN 1
          bit 3 = PAUSE
          bit 2 = SERVICE (doesn't work?)
          bit 1 = TEST (doesn't work?)
          bit 0 = TILT
c081      coins per play
c085      during the boot sequence, clearing any of bits 0, 1, 3, 4, 5, 7 enters the
          test mode, while clearing bit 2 or 6 seems to lock the machine.

write:
a000-a008 data for first CPU
e000      sound port 1
e400      sound port 2
e800      sound port 3
ec00      sound port 4

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *docastle_intkludge1,*docastle_intkludge2;
extern int docastle_intkludge1_r(int offset);
extern int docastle_intkludge2_r(int offset);
extern int docastle_shared0_r(int offset);
extern int docastle_shared1_r(int offset);
extern void docastle_shared0_w(int offset,int data);
extern void docastle_shared1_w(int offset,int data);
extern void docastle_nmitrigger(int offset,int data);
extern int docastle_interrupt2(void);

extern void docastle_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int docastle_vh_start(void);
extern void docastle_vh_stop(void);
extern void docastle_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void docastle_sound1_w(int offset,int data);
extern void docastle_sound2_w(int offset,int data);
extern void docastle_sound3_w(int offset,int data);
extern void docastle_sound4_w(int offset,int data);
extern int docastle_sh_start(void);
extern void docastle_sh_stop(void);
extern void docastle_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x95f2, 0x95f2, docastle_intkludge1_r, &docastle_intkludge1 },	/* kludge to speed up the emulation */
	{ 0x8000, 0x97ff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xb800, 0xbbff, videoram_r }, /* mirror of video ram */
	{ 0xbc00, 0xbfff, colorram_r }, /* mirror of color ram */
	{ 0xa000, 0xa008, docastle_shared0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x97ff, MWA_RAM },
	{ 0xb000, 0xb3ff, videoram_w, &videoram, &videoram_size },
	{ 0xb400, 0xb7ff, colorram_w, &colorram },
	{ 0x9800, 0x99ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa000, 0xa008, docastle_shared1_w },
	{ 0xe000, 0xe000, docastle_nmitrigger },
	{ 0xa800, 0xa800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress readmem2[] =
{
	{ 0x8050, 0x8050, docastle_intkludge2_r, &docastle_intkludge2 },	/* kludge to speed up the emulation */
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc003, 0xc003, input_port_0_r },
	{ 0xc005, 0xc005, input_port_1_r },
	{ 0xc007, 0xc007, input_port_2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc081, 0xc081, input_port_4_r },
	{ 0xc085, 0xc085, input_port_5_r },
	{ 0xa000, 0xa008, docastle_shared1_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa008, docastle_shared0_w },
	{ 0xe000, 0xe000, docastle_sound1_w },
	{ 0xe400, 0xe400, docastle_sound2_w },
	{ 0xe800, 0xe800, docastle_sound3_w },
	{ 0xec00, 0xec00, docastle_sound4_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_LEFT, OSD_KEY_DOWN,
				OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R },
		{ OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_LEFT, OSD_JOY_DOWN,
				0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_CONTROL, 0, 0, OSD_KEY_1,
				0, 0, 0, OSD_KEY_2 },
		{ OSD_JOY_FIRE, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSWA */
		0xdf,
		{ 0, 0, OSD_KEY_F1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* COIN */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* TEST */
		0xff,
		{ OSD_KEY_F2, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
	{ 0, 1, "MOVE UP" },
	{ 0, 2, "MOVE LEFT"  },
	{ 0, 0, "MOVE RIGHT" },
	{ 0, 3, "MOVE DOWN" },
	{ 1, 0, "HAMMER" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0xc0, "LIVES", { "2", "5", "4", "3" }, 1 },
	{ 3, 0x03, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 3, 0x10, "EXTRA", { "HARD", "EASY" }, 1 },
	{ 3, 0x08, "SW4", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	{ 28, 24, 20, 16, 12, 8, 4, 0 },
	32*8   /* every char takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	{ 60, 56, 52, 48, 44, 40, 36, 32,
			28, 24, 20, 16, 12, 8, 4, 0 },
	128*8  /* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 64 },
	{ 1, 0x4000, &spritelayout, 64*16, 32 },
	{ -1 } /* end of array */
};



/* this is NOT the original color PROM (I'm using Wild Ride's one) */
static unsigned char color_prom[] =
{
	0x00,0xB6,0x92,0x6F,0x4A,0x42,0x00,0x00,0x80,0xD8,0x90,0x48,0x4C,0xFF,0xE1,0x77,
	0x80,0xF9,0xF5,0xED,0x4A,0xFF,0x81,0xD3,0x80,0x1C,0x14,0x0C,0x4A,0xFF,0x81,0x77,
	0x80,0xD9,0xB5,0x91,0x4A,0xFF,0x81,0xD3,0x80,0x1B,0x13,0x0B,0x4A,0xFF,0x81,0x77,
	0xC1,0x1E,0x16,0x0E,0x4A,0xFF,0x49,0xD3,0x00,0xFC,0xE0,0x1C,0xB4,0x0F,0xF2,0x00,
	0x00,0xA4,0x8C,0x60,0xFF,0x1C,0xF0,0x00,0x00,0x1C,0xF0,0xF8,0x82,0xA2,0x65,0x00,
	0xFF,0xFC,0xF4,0xE0,0x17,0x60,0x1C,0x00,0xFF,0xFC,0xF4,0xE0,0x17,0x60,0x1C,0x77,
	0xFF,0xFC,0xF4,0xE0,0x17,0x60,0x1C,0x00,0xE0,0x14,0x00,0x92,0xFC,0x13,0x8C,0x00,
	0x00,0xFC,0xE0,0x1B,0xF4,0x0F,0xFF,0xFC,0x90,0x18,0xE0,0xFC,0x6E,0xFF,0x00,0x00,
	0x00,0x92,0xF4,0x60,0x10,0xFC,0xEC,0x00,0x00,0xE0,0x80,0xB6,0x17,0xEC,0xFF,0xB6,
	0x00,0xA0,0x6E,0x14,0xFF,0xD3,0x13,0xE0,0x00,0xA0,0x6E,0x14,0xFF,0x77,0x13,0xE0,
	0x00,0xE4,0xC0,0xF8,0x60,0x0F,0xFF,0x00,0x4A,0x92,0xB6,0xFF,0xFC,0xF0,0xE0,0x00,
	0x1F,0xFC,0xF0,0xE2,0xE0,0x6E,0xFD,0x00,0x00,0xE0,0x80,0xB6,0x17,0xEC,0x6E,0x1F,
	0x00,0xE8,0xE0,0xF8,0xFC,0x0F,0xFF,0x88,0x61,0x40,0x4A,0x6E,0xB6,0xFF,0xF4,0xF0,
	0x00,0x46,0x92,0x6E,0x49,0xFC,0xB4,0x1B,0x00,0xE0,0x00,0xFC,0x0F,0x00,0x00,0xE2,
	0x00,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0,0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,
	0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x00,0x24,0x49,0x6D,0x92,0xB6,0xDB,0xFF
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ??? */
			2,	/* memory region #2 */
			readmem2,writemem2,0,0,
			docastle_interrupt2,16
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 4*8, 28*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	256, 96*16,
	docastle_vh_convert_color_prom,

	0,
	docastle_vh_start,
	docastle_vh_stop,
	docastle_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	docastle_sh_start,
	docastle_sh_stop,
	docastle_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( docastle_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "A1",  0x0000, 0x2000 )
	ROM_LOAD( "A2",  0x2000, 0x2000 )
	ROM_LOAD( "A3",  0x4000, 0x2000 )
	ROM_LOAD( "A4",  0x6000, 0x2000 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "A5",  0x0000, 0x4000 )
	ROM_LOAD( "A6",  0x4000, 0x2000 )
	ROM_LOAD( "A7",  0x6000, 0x2000 )
	ROM_LOAD( "A8",  0x8000, 0x2000 )
	ROM_LOAD( "A9",  0xa000, 0x2000 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "A10", 0x0000, 0x4000 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8020],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0x8068],"\x01\x00\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x8020],1,10*8,f);
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
		fwrite(&RAM[0x8020],1,10*8,f);
		fclose(f);
	}
}



struct GameDriver docastle_driver =
{
	"Mr. Do's Castle",
	"docastle",
	"MIRKO BUFFONI\nNICOLA SALMORIA",
	&machine_driver,

	docastle_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};
