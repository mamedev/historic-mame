/***************************************************************************

	Atari Dominos hardware

	driver by Mike Balfour

	Games supported:
		* Dominos

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
			0000-03FF		RAM
			0400-07FF		DISPLAY RAM
			0800-0BFF	R	SWITCH
			0C00-0FFF	R	SYNC
			0C00-0C0F	W	ATTRACT
			0C10-0C1F	W	TUMBLE
			0C30-0C3F	W	LAMP2
			0C40-0C4F	W	LAMP1
			3000-37FF		Program ROM1
			3800-3FFF		Program ROM2
		   (F800-FFFF)		Program ROM2 - only needed for the 6502 vectors

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern READ_HANDLER( dominos_port_r );
extern READ_HANDLER( dominos_sync_r );
extern WRITE_HANDLER( dominos_attract_w );
extern WRITE_HANDLER( dominos_tumble_w );
extern WRITE_HANDLER( dominos_lamp2_w );
extern WRITE_HANDLER( dominos_lamp1_w );

extern void dominos_ac_signal_flip(int dummy);

extern UINT8 *dominos_sound_ram;

extern WRITE_HANDLER( dominos_videoram_w );

extern VIDEO_START( dominos );
extern VIDEO_UPDATE( dominos );

/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x00, 0x01,
	0x00, 0x02
};

static PALETTE_INIT( dominos )
{
	palette_set_color(0,0x80,0x80,0x80); /* LT GREY */
	palette_set_color(1,0x00,0x00,0x00); /* BLACK */
	palette_set_color(2,0xff,0xff,0xff); /* WHITE */
	palette_set_color(3,0x55,0x55,0x55); /* DK GREY */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}


static MACHINE_INIT( dominos )
{
	timer_pulse(TIME_IN_HZ(60.0 * 2), 0, dominos_ac_signal_flip);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM }, /* RAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* RAM */
	{ 0x0800, 0x083f, dominos_port_r }, /* SWITCH */
	{ 0x0840, 0x087f, input_port_3_r }, /* SWITCH */
	{ 0x0900, 0x093f, dominos_port_r }, /* SWITCH */
	{ 0x0940, 0x097f, input_port_3_r }, /* SWITCH */
	{ 0x0a00, 0x0a3f, dominos_port_r }, /* SWITCH */
	{ 0x0a40, 0x0a7f, input_port_3_r }, /* SWITCH */
	{ 0x0b00, 0x0b3f, dominos_port_r }, /* SWITCH */
	{ 0x0b40, 0x0b7f, input_port_3_r }, /* SWITCH */
	{ 0x0c00, 0x0fff, dominos_sync_r }, /* SYNC */
	{ 0x3000, 0x3fff, MRA_ROM }, /* ROM1-ROM2 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* ROM2 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
// Not quite sure where the sound ram variables are.
// The schematics seem the same as sprint2, but those locations do not sound right.
	{ 0x0014, 0x0016, MWA_RAM, &dominos_sound_ram }, /* WRAM */
	{ 0x0400, 0x07ff, dominos_videoram_w, &videoram }, /* DISPLAY */
	{ 0x0c00, 0x0c0f, dominos_attract_w }, /* ATTRACT */
	{ 0x0c10, 0x0c1f, dominos_tumble_w }, /* TUMBLE */
	{ 0x0c30, 0x0c3f, dominos_lamp2_w }, /* LAMP2 */
	{ 0x0c40, 0x0c4f, dominos_lamp1_w }, /* LAMP1 */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* TIMER RESET */
	{ 0x3000, 0x3fff, MWA_ROM }, /* ROM1-ROM2 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( dominos )
	PORT_START		/* DSW - fake port, gets mapped to Dominos ports */
	PORT_DIPNAME( 0x03, 0x01, "Points To Win" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x0C, 0x08, "Cost" )
	PORT_DIPSETTING(	0x0C, "2 players/coin" )
	PORT_DIPSETTING(	0x08, "1 coin/player" )
	PORT_DIPSETTING(	0x04, "2 coins/player" )
	PORT_DIPSETTING(	0x00, "2 coins/player" ) /* not a typo */

	PORT_START		/* IN1 - fake port, gets mapped to Dominos ports */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN2 - fake port, gets mapped to Dominos ports */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE | IPF_TOGGLE, "Self Test", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* IN4 */
	PORT_BIT ( 0x0F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* ATTRACT */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VRESET */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK* */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Alternating signal? */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x00, 0x02 }, /* offset into colors, # of colors */
	{ -1 }
};




/************************************************************************/
/* dominos Sound System Analog emulation                               */
/************************************************************************/

/* Nodes - Inputs */
#define DOMINOS_FREQ_DATA		NODE_01
#define DOMINOS_AMP_DATA		NODE_02
#define DOMINOS_TOPPLE_EN		NODE_03
#define DOMINOS_ATTRACT_EN		NODE_04
/* Nodes - Sounds */
#define DOMINOS_TONE_SND		NODE_10
#define DOMINOS_TOPPLE_SND		NODE_11

static DISCRETE_SOUND_START(dominos_sound_interface)
	/************************************************/
	/* Dominos Effects Relataive Gain Table         */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Tone          3.8      5/(5+10)     1000.0   */
	/* Topple        3.8      5/(33+5)      394.7   */
	/************************************************/

	/************************************************/
	/* Input register mapping for dominos           */
	/************************************************/
	/*              NODE                  ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT (DOMINOS_FREQ_DATA,    0x00, 0x000f,                   0.0)
	DISCRETE_INPUTX(DOMINOS_AMP_DATA,     0x01, 0x000f, 1000.0/15, 0,     0.0)
	DISCRETE_INPUT (DOMINOS_TOPPLE_EN,    0x02, 0x000f,                   0.0)
	DISCRETE_INPUT (DOMINOS_ATTRACT_EN,   0x03, 0x000f,                   0.0)

	/************************************************/
	/* Tone Sound                                   */
	/* Note: True freq range has not been tested    */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, DOMINOS_FREQ_DATA, 123000, 1e-7)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (289.0-95.0)/3/15, (4500.0-95.0)/3/15, (458.0-95.0)/3/15, DISC_LOGADJ, "Tone Freq")
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_ADDER2(NODE_23, 1, NODE_22, 95.0/3)
	DISCRETE_SQUAREWAVE(NODE_24, DOMINOS_ATTRACT_EN, NODE_23, DOMINOS_AMP_DATA, 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(DOMINOS_TONE_SND, 1, NODE_24, 546, 1e-7)

	/************************************************/
	/* Topple sound is just the 4V source           */
	/* 4V = HSYNC/8                                 */
	/*    = 15750/8                                 */
	/************************************************/
	DISCRETE_SQUAREWFIX(DOMINOS_TOPPLE_SND, DOMINOS_TOPPLE_EN, 15750.0/8, 394.7, 50.0, 0, 0)

	DISCRETE_ADDER2(NODE_90, 1, 0, DOMINOS_TOPPLE_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+394.7))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( dominos )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 750000)	   /* 750 kHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(dominos)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))
	
	MDRV_PALETTE_INIT(dominos)
	MDRV_VIDEO_START(dominos)
	MDRV_VIDEO_UPDATE(dominos)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, dominos_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( dominos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "7352-02.d1",   0x3000, 0x0800, CRC(738b4413) SHA1(3a90ab25bb5f65504692f97da43f03e21392dcd8) )
	ROM_LOAD( "7438-02.e1",   0x3800, 0x0800, CRC(c84e54e2) SHA1(383b388a1448a195f28352fc5e4ff1a2af80cc95) )
	ROM_RELOAD( 			  0xf800, 0x0800 )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7439-01.p4",   0x0000, 0x0200, CRC(4f42fdd6) SHA1(f8ea4b582e26cad37b746174cdc9f1c7ae0819c3) )
	ROM_LOAD( "7440-01.r4",   0x0200, 0x0200, CRC(957dd8df) SHA1(280457392f40cd66eae34d2fcdbd4d2142793402) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1977, dominos, 0, dominos, dominos, 0, ROT0, "Atari", "Dominos", GAME_IMPERFECT_SOUND )
