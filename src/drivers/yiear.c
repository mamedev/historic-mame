/***************************************************************************
YIE AR KUNG-FU hardware description
enrique.sanchez@cs.us.es

Main CPU:    Motorola 6809
Sound chip:  ???

Normal 6809 IRQs must be generated each video frame (60 fps).
The 6809 NMI vector is present, though I don't know what's its use.

ROM files

D12_8.BIN (16K)  -- ROM $8000-$BFFF   \___ 6809 code
D14_7.BIN (16K)  -- ROM $C000-$FFFF   /

G16_1.BIN (8K)   -- Background tiles; bitplanes 0,1
G15_2.BIN (8K)   -- Background tiles; bitplanes 2,3

G06_3.BIN (16K)  -- Sprites; bitplanes 0,1 (part 1)
G05_4.BIN (16K)  -- Sprites; bitplanes 0,1 (part 2)
G04_5.BIN (16K)  -- Sprites; bitplanes 2,3 (part 1)
G03_6.BIN (16K)  -- Sprites; bitplanes 2,3 (part 2)

A12_9.BIN (8K)   -- Sound related??

Memory map

0000 - 3FFF   = RAM
4000          = watchdog?
4800          = ?
4900          = ?
4A00          = ?
4B00          = ?
4C00          = DIP SWITCH 1
4D00          = DIP SWITCH 2
4E00		  = COIN,START
4E01          = JOY1
4E02		  = JOY2
4E03		  = DIP SWITCH 3
4F00          = watchdog?
5000 - 57FF   = RAM (includes sprite attribute zone mapped in it)
5030 - 51AF   = SPRITE ATTRIBUTES
5800 - 5FFF   = BACKGROUND ATTRIBUTES
6000 - 7FFF   = Not used.
8000 - FFFF   = Code ROM.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void yiear_vh_screenrefresh(struct osd_bitmap *bitmap);
void yiear_init_machine(void);
void yiear_videoram_w(int offset,int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x5000, 0x57FF, MRA_RAM },
	{ 0x5800, 0x5FFF, videoram_r, &videoram, &videoram_size },
	{ 0x8000, 0xFFFF, MRA_ROM },

	{ 0x4E00, 0x4E00, input_port_0_r },	/* coin,start */
	{ 0x4E01, 0x4E01, input_port_1_r },	/* joy1 */
	{ 0x4E02, 0x4E02, input_port_2_r },	/* joy2 */
	{ 0x4C00, 0x4C00, input_port_3_r },	/* misc */
	{ 0x4D00, 0x4D00, input_port_4_r },	/* test mode */
	{ 0x4E03, 0x4E03, input_port_5_r },	/* coins per play */
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x5030, 0x51AF, MWA_RAM, &spriteram },
	{ 0x5800, 0x5FFF, yiear_videoram_w, &videoram },
	{ 0x8000, 0xFFFF, MWA_ROM },
	{ -1 } /* end of table */
};

static struct InputPort input_ports[] =
{
	{	/* coin,start */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* joy1 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_SPACE, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_FIRE3, 0 }
	},
	{	/* joy2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* misc */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* test mode */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* coins per play */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 1, 2, "MOVE UP" },
        { 1, 0, "MOVE LEFT"  },
        { 1, 1, "MOVE RIGHT" },
        { 1, 3, "MOVE DOWN" },
        { 1, 4, "PUNCH" },
        { 1, 5, "KICK" },
        { 1, 6, "JUMP" },
        { -1 }
};

static struct DSW dsw[] =
{
	/* DIP SWITCH 1 */
	/* input port, mask, name, values, reverse */
	{ 3, 0x03, "LIVES", { "5", "3", "1", "1" }, 1},
	{ 3, 0x08, "BONUS", { "40000 130000", "30000 110000" }, 1 },
	{ 3, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 3, 0x80, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{-1}
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	512,	/* 512 characters */
	4,		/* 4 bits per pixel */
	{ 0,4,65536L, 65536L+4 },		/* plane */
	{ 0, 1, 2, 3, 64, 65, 66, 67},		/* x */
	{ 0, 8, 16, 24, 32, 40, 48, 56},	/* y */
	128
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16 by 16 */
	512,	/* 512 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 4, 32768L*8L, 32768L*8L+4 },	/* plane offsets */
	{ 0, 1, 2, 3, 64, 65, 66, 67,
	  128+0, 128+1, 128+2, 128+3, 128+64, 128+65, 128+66, 128+67 },
	{ 0, 8, 16, 24, 32, 40, 48, 56,
	  256+0, 256+8, 256+16, 256+24, 256+32, 256+40, 256+48, 256+56 },
	512
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 1 },
	{ 1, 0x4000, &spritelayout, 16, 1 },
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	/* characters */
	0,0,0,			/* black */
	255,0,0,		/* red */
	180,242,255,		/* bluegreen */
	144,82,67,		/* reddish */
	88,148,48,		/* green */
	50,50,80,		/* dark blue */
	41,93,80,		/* green */
	20,20,20,		/* dark grey */
	128,0,100,		/* temple trim */
	224,255,214,		/* sky */
	130,212,88,		/* lgreen */
	165,159,64,		/* temple lawn */
	100,100,100,		/* grey */
	136,152,126,		/* brownish */
	130,138,236,		/* lblue */
	0xFF, 0xFF, 0xFF,	/* white */

	/* sprites */
	0x00,0x00,0x00, 	/* (transparent) */
	0xFF,0xFF,0x00, 	/* yellow */
	0xFF,0x0F,0x0F, 	/* bright red */
	0,255,0,		/* lime */
	0x5F,0x5F,0xFF, 	/* blue */
	255,255,153,    	/* light flesh */
	153,51,0,		/* brown */
	0xBF,0xBF,0xBF, 	/* dark grey */
	0,0,0,			/* black */
	204,153,51,		/* dark flesh */
	218,104,112,     	/* dark red */
	0x1F,0x7F,0, 	        /* green */
	185,255,253,     	/* light grey */
	255,204,153,	        /* flesh */
	153,102,51, 	        /* wood */
	0xFF,0xFF,0xFF         /* white */
};

static unsigned char colortable[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,	/* 1.25 Mhz */
			0,			/* memory region */
			readmem,	/* MemoryReadAddress */
			writemem,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			interrupt,	/* interrupt routine */
			1			/* interrupts per frame */
		}
	},
	60,					/* frames per second */
	0,//yiear_init_machine,	/* init machine routine - needed?*/

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0, 255, 0, 255 },	/* struct rectangle visible_area */
	gfxdecodeinfo,			/* GfxDecodeInfo * */

	32,						/* total colors */
	32,						/* color table length */
	0,						/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,						/* vh_init routine */
	generic_vh_start,		/* vh_start routine */
	generic_vh_stop,		/* vh_stop routine */
	yiear_vh_screenrefresh,	/* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	0					/* sh_update routine */
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( yiear_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D12_8.BIN", 0x8000, 0x4000, 0x53e66644 )
	ROM_LOAD( "D14_7.BIN", 0xC000, 0x4000, 0xd5f1fb77 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "G16_1.BIN", 0x00000, 0x2000, 0x12fd01fd )
	ROM_LOAD( "G15_2.BIN", 0x02000, 0x2000, 0xf889b63f )
	ROM_LOAD( "G06_3.BIN", 0x04000, 0x4000, 0x5c60737a )
	ROM_LOAD( "G05_4.BIN", 0x08000, 0x4000, 0xe9a3d4d9 )
	ROM_LOAD( "G04_5.BIN", 0x0c000, 0x4000, 0x35ca933a )
	ROM_LOAD( "G03_6.BIN", 0x10000, 0x4000, 0x9e25b6cb )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x5520],"\x00\x36\x70",3) == 0) &&
		(memcmp(&RAM[0x55A9],"\x10\x10\x10",3) == 0))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			/* Read the scores */
                        fread(&RAM[0x5520],1,14*10,f);
			/* reset score at top */
			memcpy(&RAM[0x521C],&RAM[0x5520],3);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		/* Save the scores */
                fwrite(&RAM[0x5520],1,14*10,f);
		fclose(f);
	}

}



struct GameDriver yiear_driver =
{
	"Yie Ar Kung Fu (Konami)",
	"yiear",
	"ENRIQUE SANCHEZ\nPHILIP STROFFOLINO\nMIKE BALFOUR",
	&machine_driver,

	yiear_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,

	hiload, hisave
};
