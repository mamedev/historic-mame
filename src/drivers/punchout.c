/***************************************************************************

Punch Out memory map (preliminary)

0000-3fff ROM


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



void punchout_vh_screenrefresh(struct osd_bitmap *bitmap);


static int pip(int offset)
{
	return 0;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xd7d0, 0xd7d0, pip },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
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
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 0, 2048*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 2 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout3 =
{
	8,8,	/* 8*8 characters */
	12*1024,	/* a lot of characters ;-) */
	1,	/* 1 bit per pixel */
	{ 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,  0, 8 },
	{ 1, 0x04000, &charlayout,  0, 8 },
	{ 1, 0x08000, &charlayout2,  0, 8 },
	{ 1, 0x0e000, &charlayout,  0, 8 },
	{ 1, 0x12000, &charlayout1,  0, 8 },
	{ 1, 0x1a000, &charlayout3,  0, 8 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0x00,0x00, /* RED */
	0x00,0xff,0x00, /* GREEN */
	0x00,0x00,0xff, /* BLUE */
	0xff,0xff,0x00, /* YELLOW */
	0xff,0x00,0xff, /* MAGENTA */
	0x00,0xff,0xff, /* CYAN */
	0xff,0xff,0xff, /* WHITE */
	0xE0,0xE0,0xE0, /* LTGRAY */
	0xC0,0xC0,0xC0, /* DKGRAY */
	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0xc0,0x90,0x50,	/* BROWN1 */
	0xa3,0x78,0x3a,	/* BROWN2 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */
	0x54,0xa8,0xff, /* LTBLUE */
	0x00,0xa0,0x00, /* DKGREEN */
	0x00,0xe0,0x00, /* GRASSGREEN */
	0xff,0xb6,0xdb,	/* PINK */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,96,0x49,	/* DKORANGE */
	0xff,128,0x00,	/* ORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};


static unsigned char colortable[] =
{
	0,1,2,3,
	0,4,5,6,
	0,7,8,9,
	0,10,11,12,
	0,13,14,15,
	0,1,3,5,
	0,7,9,11,
	0,13,15,17,

	0,1,2,3,7,4,5,6
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			ignore_interrupt,1
		},
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	punchout_vh_screenrefresh,

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

ROM_START( punchout_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "chp1-c.8l",  0x0000, 0x2000, 0xc13743e5 )
	ROM_LOAD( "chp1-c.8k",  0x2000, 0x2000, 0xc7e699da )
#if 0
	ROM_LOAD( "chp1-c.4k",  0x0000, 0x2000, 0x89829a5c )
	ROM_LOAD( "chp1-c.6p",  0x0000, 0x4000, 0x49ccbe4e )
	ROM_LOAD( "chp1-c.8f",  0x0000, 0x4000, 0x2b8a61f6 )
	ROM_LOAD( "chp1-c.8h",  0x0000, 0x2000, 0x6e4b5c2d )
	ROM_LOAD( "chp1-c.8j",  0x0000, 0x2000, 0xd8348e68 )
#endif
	ROM_REGION(0x32000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chp1-b.4a",  0x00000, 0x2000, 0xe4db3e1f )
	ROM_LOAD( "chp1-b.4b",  0x02000, 0x2000, 0x7d8ace36 )
	ROM_LOAD( "chp1-b.4c",  0x04000, 0x2000, 0x87da31ee )
	ROM_LOAD( "chp1-b.4d",  0x06000, 0x2000, 0x595ba5a5 )
	ROM_LOAD( "chp1-v.2u",  0x08000, 0x2000, 0x6b5c1b2e )
	ROM_LOAD( "chp1-v.3u",  0x0a000, 0x2000, 0x87040284 )
	ROM_LOAD( "chp1-v.4u",  0x0c000, 0x2000, 0x8ffd2fef )
	ROM_LOAD( "chp1-v.2v",  0x0e000, 0x2000, 0x96c570a9 )
	ROM_LOAD( "chp1-v.3v",  0x10000, 0x2000, 0xce3207da )
	ROM_LOAD( "chp1-v.6n",  0x12000, 0x2000, 0x09e19979 )
	ROM_LOAD( "chp1-v.6p",  0x14000, 0x2000, 0x25774659 )
	ROM_LOAD( "chp1-v.8n",  0x16000, 0x2000, 0xfd94b194 )
	ROM_LOAD( "chp1-v.8p",  0x18000, 0x2000, 0xe3a2249a )

	ROM_LOAD( "chp1-v.2r",  0x1a000, 0x4000, 0x2098df66 )
	ROM_LOAD( "chp1-v.3r",  0x1e000, 0x4000, 0xf8d949ef )
	ROM_LOAD( "chp1-v.4r",  0x22000, 0x4000, 0xb1fb2ff3 )
	ROM_LOAD( "chp1-v.2t",  0x26000, 0x4000, 0x64066ce4 )
	ROM_LOAD( "chp1-v.3t",  0x2a000, 0x4000, 0xb77806f8 )
	ROM_LOAD( "chp1-v.4t",  0x2e000, 0x4000, 0x6ff58bf3 )
ROM_END



struct GameDriver punchout_driver =
{
	"Punch Out",
	"punchout",
	"Nicola Salmoria",
	&machine_driver,

	punchout_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
