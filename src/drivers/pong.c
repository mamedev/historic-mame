/***************************************************************************
	pong.c
	Machine driver

	J. Buchmueller, November '99
***************************************************************************/

#include "driver.h"
#include "sndintrf.h"
#include "vidhrdw/pong.h"

extern int pong_sh_start(const struct MachineSound *msound);
extern void pong_sh_stop(void);
extern void pong_sh_update(void);

/*************************************************************
 *
 * Memory layout
 * (This is all fake to suit the needs of MAME environment)
 *
 *************************************************************/
static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff,	MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff,	MRA_RAM },
    { -1 }  /* end of table */
};

INPUT_PORTS_START( pong )
	PORT_START		/* IN0 buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1	)
	PORT_DIPNAME( 0x02, 0x00, "Ending Score" )
    PORT_DIPSETTING(    0x00, "11" )
	PORT_DIPSETTING(	0x02, "15" )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW,	IPT_UNUSED )

	PORT_START		/* IN1 control 1 */
	PORT_ANALOG( 0x1ff, (PONG_MAX_V-15)/2, IPT_AD_STICK_Y, 100, 5, 0, PONG_VBLANK - 12, 255)

	PORT_START		/* IN2 control 2 */
	PORT_ANALOG( 0x1ff, (PONG_MAX_V-15)/2, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 5, 0, PONG_VBLANK - 12, 255)

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,							/* 8*8 characters */
	128,							/* 128 characters */
	1,								/* 1 bit per pixel */
	{ 0 },							/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	/* pretty straight layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 							/* every char takes 8 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,	0, 2},		/* character generator */
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	0x00,0x00,0x00, /* black */
	0xff,0xff,0xff, /* white (1k resistor) */
	0xd4,0xd4,0xd4, /* slightly darker white (1.2k resistor) */
};

static unsigned short colortable[] =
{
	0,1,
	1,0,
	2,0,
};

static struct CustomSound_interface custom_interface =
{
	pong_sh_start,
	pong_sh_stop,
	pong_sh_update
};

#define PONG_CLOCK	PONG_FPS * PONG_MAX_V * PONG_MAX_H

static int pong_video[] = {
    PONG_MAX_H, PONG_MAX_V,
    0, PONG_HSYNC0, PONG_HSYNC1, PONG_HBLANK,
    0, PONG_VSYNC0, PONG_VSYNC1, PONG_VBLANK
};

static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,palette,sizeof(palette));
	memcpy(game_colortable,colortable,sizeof(colortable));
}

static struct MachineDriver pong_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_GENSYNC,
			PONG_CLOCK,
			0,
			readmem, writemem, 0, 0,
			pong_vh_scanline, PONG_MAX_V,
			0, 0, &pong_video
        }
    },
	PONG_FPS, 0,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	PONG_MAX_H, PONG_MAX_V,
	{ PONG_HBLANK, PONG_MAX_H-1, PONG_VBLANK, PONG_MAX_V-1 },
	gfxdecodeinfo,			/* dummy gfxdecodeinfo */
	sizeof(palette)/sizeof(palette[0])/3,
	sizeof(colortable)/sizeof(colortable[0]),
	init_palette,

	VIDEO_TYPE_RASTER,
	0,
	pong_vh_start,
	pong_vh_stop,
	pong_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
        }
    }
};

ROM_START( pong )
	ROM_REGION(0x00400) 	/* 1024 bytes memory (dummy) */
	ROM_REGION(0x00400) 	/* 1024 bytes for 128 8x8 1bpp characters (do I need a gfxdecodeinfo?) */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver driver_pong =
{
	__FILE__,
	0,
	"pong",
	"Pong!",
	"1972",
	"Atari",
	"obsolete",
	0,
	&pong_machine_driver,
	0,

	rom_pong,	/* really no ROM, but memory used to hold the bitmap */
	0,
	0,
	0,
	0,			/* sound_prom */

	input_ports_pong,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

