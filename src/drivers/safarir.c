/****************************************************************************

Safari Rally by SNK/Taito

Driver by Zsolt Vasvari


This hardware is a precursor to Phoenix.

----------------------------------

CPU board

76477        18MHz

              8080

Video board


 RL07  2114
       2114
       2114
       2114
       2114           RL01 RL02
       2114           RL03 RL04
       2114           RL05 RL06
 RL08  2114

11MHz

----------------------------------

TODO:

- SN76477 sound

****************************************************************************/

#include "driver.h"


unsigned char *safarir_ram1, *safarir_ram2;
size_t safarir_ram_size;

static unsigned char *safarir_ram;
static int safarir_scroll;



WRITE_HANDLER( safarir_ram_w )
{
	safarir_ram[offset] = data;
}

READ_HANDLER( safarir_ram_r )
{
	return safarir_ram[offset];
}


WRITE_HANDLER( safarir_scroll_w )
{
	safarir_scroll = data;
}

WRITE_HANDLER( safarir_ram_bank_w )
{
	safarir_ram = data ? safarir_ram1 : safarir_ram2;
}


VIDEO_UPDATE( safarir )
{
	int offs;


	for (offs = safarir_ram_size/2 - 1;offs >= 0;offs--)
	{
		int sx,sy;
		UINT8 code;


		sx = offs % 32;
		sy = offs / 32;

		code = safarir_ram[offs + safarir_ram_size/2];


		drawgfx(bitmap,Machine->gfx[0],
				code & 0x7f,
				code >> 7,
				0,0,
				(8*sx - safarir_scroll) & 0xff,8*sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */

	for (offs = safarir_ram_size/2 - 1;offs >= 0;offs--)
	{
		int sx,sy,transparency;
		UINT8 code;


		sx = offs % 32;
		sy = offs / 32;

		code = safarir_ram[offs];

		transparency = (sx >= 3) ? TRANSPARENCY_PEN : TRANSPARENCY_NONE;


		drawgfx(bitmap,Machine->gfx[1],
				code & 0x7f,
				code >> 7,
				0,0,
				8*sx,8*sy,
				&Machine->visible_area,transparency,0);
	}
}


static unsigned short colortable_source[] =
{
	0x00, 0x01,
	0x00, 0x02,
};

static PALETTE_INIT( safarir )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0x80,0x80,0x80); /* gray */
	palette_set_color(2,0xff,0xff,0xff); /* white */

	memcpy(colortable,colortable_source,sizeof(colortable_source));
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x17ff, MRA_ROM },
	{ 0x2000, 0x27ff, safarir_ram_r },
	{ 0x3800, 0x38ff, input_port_0_r },
	{ 0x3c00, 0x3cff, input_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x2000, 0x27ff, safarir_ram_w, &safarir_ram1, &safarir_ram_size },
	{ 0x2800, 0x28ff, safarir_ram_bank_w },
	{ 0x2c00, 0x2cff, safarir_scroll_w },
	{ 0x3000, 0x30ff, MWA_NOP },	/* goes to SN76477 */

	{ 0x8000, 0x87ff, MWA_NOP, &safarir_ram2 },	/* only here to initialize pointer */
MEMORY_END


INPUT_PORTS_START( safarir )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, "Acceleration Rate" )
	PORT_DIPSETTING(    0x00, "Slowest" )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPSETTING(    0x08, "Fast" )
	PORT_DIPSETTING(    0x0c, "Fastest" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x40, "7000" )
	PORT_DIPSETTING(    0x60, "9000" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	128,	/* 128 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 2 },
	{ REGION_GFX2, 0, &charlayout, 0, 2 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( safarir )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 3072000)	/* 3 MHz ? */								\
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 30*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(3)
	MDRV_COLORTABLE_LENGTH(2*2)

	MDRV_PALETTE_INIT(safarir)
	MDRV_VIDEO_UPDATE(safarir)

	/* sound hardware */
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( safarir )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "rl01",		0x0000, 0x0400, CRC(cf7703c9) SHA1(b4182df9332b355edaa518462217a6e31e1c07b2) )
	ROM_LOAD( "rl02",		0x0400, 0x0400, CRC(1013ecd3) SHA1(2fe367db8ca367b36c5378cb7d5ff918db243c78) )
	ROM_LOAD( "rl03",		0x0800, 0x0400, CRC(84545894) SHA1(377494ceeac5ad58b70f77b2b27b609491cb7ffd) )
	ROM_LOAD( "rl04",		0x0c00, 0x0400, CRC(5dd12f96) SHA1(a80ac0705648f0807ea33e444fdbea450bf23f85) )
	ROM_LOAD( "rl05",		0x1000, 0x0400, CRC(935ed469) SHA1(052a59df831dcc2c618e9e5e5fdfa47548550596) )
	ROM_LOAD( "rl06",		0x1400, 0x0400, CRC(24c1cd42) SHA1(fe32ecea77a3777f8137ca248b8f371db37b8b85) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rl08",		0x0000, 0x0400, CRC(d6a50aac) SHA1(0a0c2cefc556e4e15085318fcac485b82bac2416) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rl07",		0x0000, 0x0400, CRC(ba525203) SHA1(1c261cc1259787a7a248766264fefe140226e465) )
ROM_END


DRIVER_INIT( safarir )
{
	safarir_ram = safarir_ram1;
}


GAMEX( 1979, safarir, 0, safarir, safarir, safarir, ROT90, "SNK", "Safari Rally (Japan)", GAME_NO_SOUND | GAME_IMPERFECT_COLORS )
