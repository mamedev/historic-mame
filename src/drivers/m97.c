/*

IREM M97 board

CPU is V30? V60?(Irem/Nanao custom), Z80+YM2151 for sound

The main CPU is encrypted.
It uses a simple opcode lookup encryption, the painful part is that it's
preprogrammed into the IREM D800 cpu and isn't a algorithmic based one.

So unless someone wants to go though all the permutations I don't see it
being hacked very soon


games running on this:

Risky Challenge/Gussun Oyoyo D8000019A1
Shisensho II                 D8000020A1 023 9320NK700
Quiz F-1 1,2 Finish          NANAO 08J27291A4 014 9147KK700
Bomber Man World

*/


#include "driver.h"


int m97_vh_start(void)
{
	return 0;
}

void m97_vh_stop(void)
{
}

void m97_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x80000, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x80000, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	65536,	/* 65536 characters */
	4,	/* 4 bits per pixel */
	{ 3*65536*8*8, 2*65536*8*8, 1*65536*8*8, 0*65536*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,     0, 8 },
	{ -1 } /* end of array */
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
{0},//	{ irq_handler },
	{ 0 }
};



/* V30 1 MB address mask */
static int shanghai_addrmask = 0x0fffff;

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/4,	/* ??? */
			0,
			readmem,writemem,0,0,
			ignore_interrupt,1,
			0,0,&shanghai_addrmask
		},
#if 0
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			26666666/8,	/* ??? */
			2,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	384, 280, { 0, 384-1, 0, 280-1 },
	gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER,
	0,
	m97_vh_start,
	m97_vh_stop,
	m97_vh_screenrefresh,

	/* sound hardware */
#if 0
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
#endif
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( riskchal_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_EVEN( "rc_h0.rom",    0x80000, 0x40000, 0x4c9b5344 )
	ROM_LOAD_ODD ( "rc_l0.rom",    0x80000, 0x40000, 0x0455895a )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rc_c0.rom",         0x000000, 0x80000, 0x84d0b907 )
	ROM_LOAD( "rc_c1.rom",         0x080000, 0x80000, 0xcb3784ef )
	ROM_LOAD( "rc_c2.rom",         0x100000, 0x80000, 0x687164d7 )
	ROM_LOAD( "rc_c3.rom",         0x180000, 0x80000, 0xc86be6af )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rc_sp.rom",         0x0000, 0x10000, 0xbb80094e )

	ROM_REGION(0x40000)	/* ADPCM samples */
	ROM_LOAD( "rc_v0.rom",         0x0000, 0x40000, 0xcddac360 )
ROM_END

ROM_START( shisen2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_EVEN( "sis2-ho-.rom", 0x80000, 0x40000, 0x6fae0aea )
	ROM_LOAD_ODD ( "sis2-lo-.rom", 0x80000, 0x40000, 0x2af25182 )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic81.rom",          0x000000, 0x80000, 0x5a7cb88f )
	ROM_LOAD( "ic82.rom",          0x080000, 0x80000, 0x54a7852c )
	ROM_LOAD( "ic83.rom",          0x100000, 0x80000, 0x2bd65dc6 )
	ROM_LOAD( "ic84.rom",          0x180000, 0x80000, 0x876d5fdb )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sis2-sp-.rom",      0x0000, 0x10000, 0x6fc0ff3a )
ROM_END

ROM_START( quizf1_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_EVEN( "qf1-h0-.77",   0x080000, 0x40000, 0x280e3049 )
	ROM_LOAD_ODD ( "qf1-l0-.79",   0x080000, 0x40000, 0x94588a6f )
	ROM_LOAD_EVEN( "qf1-h1-.78",   0x100000, 0x80000, 0xc6c2eb2b )	/* banked? */
	ROM_LOAD_ODD ( "qf1-l1-.80",   0x100000, 0x80000, 0x3132c144 )	/* banked? */

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "qf1-c0-.81",        0x000000, 0x80000, 0xc26b521e )
	ROM_LOAD( "qf1-c1-.82",        0x080000, 0x80000, 0xdb9d7394 )
	ROM_LOAD( "qf1-c2-.83",        0x100000, 0x80000, 0x0b1460ae )
	ROM_LOAD( "qf1-c3-.84",        0x180000, 0x80000, 0x2d32ff37 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "qf1-sp-.33",        0x0000, 0x10000, 0x0664fa9f )

	ROM_REGION(0x40000)	/* ADPCM samples */
	ROM_LOAD( "qf1-v0-.30",        0x0000, 0x40000, 0xb8d16e7c )
ROM_END

ROM_START( bbmanw_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_EVEN( "bbm2_h0.bin",  0x080000, 0x20000, 0xf694b461 )
	ROM_LOAD_ODD ( "bbm2_l0.bin",  0x080000, 0x20000, 0x755126cc )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bbm2_c0.bin",       0x000000, 0x40000, 0xe7ce058a )
	ROM_LOAD( "bbm2_c1.bin",       0x080000, 0x40000, 0x636a78a9 )
	ROM_LOAD( "bbm2_c2.bin",       0x100000, 0x40000, 0x9ac2142f )
	ROM_LOAD( "bbm2_c3.bin",       0x180000, 0x40000, 0x47af1750 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2sp-b.bin",      0x0000, 0x10000, 0xb8d8108c )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "bbm2_vo.bin",       0x0000, 0x20000, 0x0ae655ff )
ROM_END



struct GameDriver riskchal_driver =
{
	__FILE__,
	0,
	"riskchal",
	"Risky Challenge",
	"1993",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	riskchal_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver shisen2_driver =
{
	__FILE__,
	0,
	"shisen2",
	"Shisensho II",
	"1993",
	"Tamtex",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	shisen2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver quizf1_driver =
{
	__FILE__,
	0,
	"quizf1",
	"Quiz F-1 1,2finish",
	"1992",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	quizf1_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver bbmanw_driver =
{
	__FILE__,
	0,
	"bbmanw",
	"Bomber Man World",
	"1992",
	"Irem (licensed from Hudson Soft)",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	bbmanw_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
