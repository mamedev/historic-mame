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

static struct MachineDriver machine_driver_pong =
{
	/* basic machine hardware */
	{
		{
			CPU_GENSYNC,
			PONG_CLOCK,
			0, 0, 0, 0,
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
	NULL,
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

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver driver_pong =
{
	__FILE__,
	0,
	"pong",
	"Pong",
	"1972",
	"Atari",
	NULL,
	0,
	&machine_driver_pong,
	0,

	NULL,
	0,
	0,
	0,
	0,

	input_ports_pong,

	0, 0, 0,
	ROT0,

	0,0
};

