/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *docastle_sharedram;
extern unsigned char *docastle_sharedram2;
extern int docastle_sharedram2_r(int offset);
extern void docastle_sharedram2_w(int offset,int data);
extern int docastle_sharedram_r(int offset);
extern void docastle_sharedram_w(int offset,int data);

extern int docastle_init_machine(const char *gamename);
extern void docastle_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct MemoryReadAddress readmem[] =
{
        { 0xa000, 0xa008, docastle_sharedram_r, &docastle_sharedram },
	{ 0x8000, 0x99ff, MRA_RAM },
	{ 0xb000, 0xb7ff, MRA_RAM },	/* video and color RAM */
        { 0xb800, 0xbbff, videoram_r }, /* mirror of video ram */
        { 0xbc00, 0xbfff, colorram_r }, /* mirror of color ram */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0xa000, 0xa008, docastle_sharedram_w },
	{ 0x8000, 0x97ff, MWA_RAM },
	{ 0xb000, 0xb3ff, videoram_w, &videoram },
	{ 0xb400, 0xb7ff, colorram_w, &colorram },
	{ 0xa800, 0xa800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress readmem2[] =
{
        { 0xa000, 0xa008, docastle_sharedram_r },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
        { 0xa000, 0xa008, docastle_sharedram_w },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { -1 }
};


static struct DSW dsw[] =
{
	{ -1 }
};



static struct GfxLayout charlayout =
{
        8,8,    /* 8*8 characters */
        512,    /* 512 characters */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 }, /* the two bitplanes are separated */
        { 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
        { 28, 24, 20, 16, 12, 8, 4, 0 },
        8*4*8   /* every char takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
        16,16,  /* 16*16 sprites */
        128,    /* 128 sprites */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 }, /* the two bitplanes for 4 pixels are packed into one byte */
        { 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
                        8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
        { 60, 56, 52, 48, 44, 40, 36, 32,
                        28, 24, 20, 16, 12, 8, 4, 0 },
        64*4*8  /* every sprite takes 256 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 8 },
	{ 1, 0x4000, &spritelayout,  0, 8 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0xdb,0x00,	/* UNUSED */
	0x00,0xdb,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x88,0x88,0x88,	/* UNUSED */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xff,0x00,0xdb,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,
		UNUSED2,YELLOW,UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,GREY};

static unsigned char colortable[] =
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9
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
			interrupt,1
		}
	},
	60,
	docastle_init_machine,

	/* video hardware */
	32*8, 32*8, { 4*8, 28*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	docastle_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
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
	ROM_LOAD( "A9",  0x4000, 0x2000 )
	ROM_LOAD( "A8",  0x6000, 0x2000 )
	ROM_LOAD( "A7",  0x8000, 0x2000 )
	ROM_LOAD( "A6",  0xa000, 0x2000 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "A10", 0x0000, 0x4000 )
ROM_END



struct GameDriver docastle_driver =
{
	"docastle",
	&machine_driver,

	docastle_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x00, 0x01,
	8*13, 8*16, 0x03,

	0, 0
};
