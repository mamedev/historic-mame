/***************************************************************************

Atari Poolshark Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( poolshrk );
extern VIDEO_UPDATE( poolshrk );

extern UINT8* poolshrk_playfield_ram;
extern UINT8* poolshrk_hpos_ram;
extern UINT8* poolshrk_vpos_ram;

static int poolshrk_da_latch;


static DRIVER_INIT( poolshrk )
{
	UINT8* pSprite = memory_region(REGION_GFX1);
	UINT8* pOffset = memory_region(REGION_PROMS);

	int i;
	int j;

	/* re-arrange sprite data using the PROM */

	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 16; j++)
		{
			UINT16 v =
				(pSprite[0] << 0xC) |
				(pSprite[1] << 0x8) |
				(pSprite[2] << 0x4) |
				(pSprite[3] << 0x0);

			v >>= pOffset[j];

			pSprite[0] = (v >> 0xC) & 15;
			pSprite[1] = (v >> 0x8) & 15;
			pSprite[2] = (v >> 0x4) & 15;
			pSprite[3] = (v >> 0x0) & 15;

			pSprite += 4;
		}
	}
}


static WRITE_HANDLER( poolshrk_scratch_sound_w )
{
	discrete_sound_w(1, offset & 1);
}

static WRITE_HANDLER( poolshrk_score_sound_w )
{
	discrete_sound_w(3, 1); /* this will trigger the sound code for 1 sample */
}

static WRITE_HANDLER( poolshrk_click_sound_w )
{
	discrete_sound_w(2, 1); /* this will trigger the sound code for 1 sample */
}

static WRITE_HANDLER( poolshrk_bump_sound_w )
{
	discrete_sound_w(0, offset & 1);
}


static WRITE_HANDLER( poolshrk_da_latch_w )
{
	poolshrk_da_latch = data & 15;
}


static WRITE_HANDLER( poolshrk_led_w )
{
	if (offset & 2)
		set_led_status(0, offset & 1);
	if (offset & 4)
		set_led_status(1, offset & 1);
}


static WRITE_HANDLER( poolshrk_watchdog_w )
{
	if ((offset & 3) == 3)
	{
		watchdog_reset_w(0, 0);
	}
}


static READ_HANDLER( poolshrk_input_r )
{
	UINT8 val = readinputport(offset & 3);

	int x = readinputport(4 + (offset & 1));
	int y = readinputport(6 + (offset & 1));

	if (x >= poolshrk_da_latch) val |= 8;
	if (y >= poolshrk_da_latch) val |= 4;

	if ((offset & 3) == 3)
	{
		watchdog_reset_r(0);
	}

	return val;
}


static READ_HANDLER( poolshrk_irq_reset_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	return 0;
}


static ADDRESS_MAP_START( poolshrk_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x00ff) AM_MIRROR(0x2300) AM_RAM
	AM_RANGE(0x0400, 0x07ff) AM_MIRROR(0x2000) AM_WRITE(MWA8_RAM) AM_BASE(&poolshrk_playfield_ram)
	AM_RANGE(0x0800, 0x080f) AM_MIRROR(0x23f0) AM_WRITE(MWA8_RAM) AM_BASE(&poolshrk_hpos_ram)
	AM_RANGE(0x0c00, 0x0c0f) AM_MIRROR(0x23f0) AM_WRITE(MWA8_RAM) AM_BASE(&poolshrk_vpos_ram)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x2000) AM_READWRITE(poolshrk_input_r, poolshrk_watchdog_w)
	AM_RANGE(0x1400, 0x17ff) AM_MIRROR(0x2000) AM_WRITE(poolshrk_scratch_sound_w)
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x2000) AM_WRITE(poolshrk_score_sound_w)
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0x2000) AM_WRITE(poolshrk_click_sound_w)
	AM_RANGE(0x4000, 0x4000) AM_NOP /* diagnostic ROM location */
	AM_RANGE(0x6000, 0x63ff) AM_WRITE(poolshrk_da_latch_w)
	AM_RANGE(0x6400, 0x67ff) AM_WRITE(poolshrk_bump_sound_w)
	AM_RANGE(0x6800, 0x6bff) AM_READ(poolshrk_irq_reset_r)
	AM_RANGE(0x6c00, 0x6fff) AM_WRITE(poolshrk_led_w)
	AM_RANGE(0x7000, 0x7fff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( poolshrk )
	PORT_START
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Extended Play" )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, "Racks Per Game" )
	PORT_DIPSETTING( 0x03, "2" )
	PORT_DIPSETTING( 0x02, "3" )
	PORT_DIPSETTING( 0x01, "4" )
	PORT_DIPSETTING( 0x00, "5" )
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ))
	PORT_DIPSETTING( 0x00, DEF_STR( 2C_1C ))
	PORT_DIPSETTING( 0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING( 0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING( 0x01, DEF_STR( 1C_4C ))
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER2, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE | IPF_PLAYER1, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE | IPF_PLAYER2, 25, 1, 0, 15)

INPUT_PORTS_END


static struct GfxLayout poolshrk_sprite_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F,
		0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F
	},
	{
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0C0, 0x0E0,
		0x100, 0x120, 0x140, 0x160, 0x180, 0x1A0, 0x1C0, 0x1E0
	},
	0x200     /* increment */
};


static struct GfxLayout poolshrk_tile_layout =
{
	8, 8,     /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		7, 6, 5, 4, 3, 2, 1, 0
	},
	{
		0x000, 0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xE00
	},
	0x8       /* increment */
};


static struct GfxDecodeInfo poolshrk_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &poolshrk_sprite_layout, 0, 2 },
	{ REGION_GFX2, 0, &poolshrk_tile_layout, 0, 1 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( poolshrk )
{
	palette_set_color(0,0x7F, 0x7F, 0x7F);
	palette_set_color(1,0xFF, 0xFF, 0xFF);
	palette_set_color(2,0x7F, 0x7F, 0x7F);
	palette_set_color(3,0x00, 0x00, 0x00);
}


/************************************************************************/
/* poolshrk Sound System Analog emulation                               */
/* Jan 2004, Derrick Renaud                                             */
/************************************************************************/

/* Nodes - Inputs */
#define POOLSHRK_BUMP_EN	NODE_01
#define POOLSHRK_CLICK_EN	NODE_02
#define POOLSHRK_SCORE_EN	NODE_03
/* Nodes - Sounds */
#define POOLSHRK_BUMP_SND	NODE_10
#define POOLSHRK_SCRATCH_SND	NODE_11
#define POOLSHRK_CLICK_SND	NODE_12
#define POOLSHRK_SCORE_SND	NODE_13

static DISCRETE_SOUND_START(poolshrk_sound_interface)
	/************************************************/
	/* poolshrk  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect  V-ampIn  Gain ratio        Relative  */
	/* Bump     3.8     1/(82+1)           1000.0   */
	/* Scratch  3.8     1/(330+1)           250.8   */
	/* Click    3.8     1/(330+1)           250.8   */
	/* Score    3.8     1/(330+1)           250.8   */
	/************************************************/

	/************************************************/
	/* Input register mapping for poolshrk          */
	/************************************************/
	/*                   NODE                 ADDR  MASK     GAIN      OFFSET  INIT */
	DISCRETE_INPUT      (POOLSHRK_BUMP_EN,     0x00, 0x000f,                    0.0)
	DISCRETE_INPUTX     (POOLSHRK_SCRATCH_SND, 0x01, 0x000f,  250.8,    0,      0.0)
	DISCRETE_INPUT_PULSE(POOLSHRK_CLICK_EN,    0x02, 0x000f,                    0.0)
	DISCRETE_INPUT_PULSE(POOLSHRK_SCORE_EN,    0x03, 0x000f,                    0.0)
	/************************************************/

	/************************************************/
	/* Scratch is just the trigger sent directly    */
	/* to the output.  We take care of it's         */
	/* amplitude right in it's DISCRETE_INPUTX.     */
	/************************************************/

	/************************************************/
	/* Bump is just a triggered 128V signal         */
	/************************************************/
	DISCRETE_SQUAREWFIX(NODE_20, POOLSHRK_BUMP_EN, 15750.0/2.0/128.0, 1000.0, 50.0, 1000.0/2, 0.0)	// 128V signal
	DISCRETE_RCFILTER(POOLSHRK_BUMP_SND, 1, NODE_20, 470, 4.7e-6)	// Filtered by R53/C14

	/************************************************/
	/* Score is a triggered 0-15 count of the       */
	/* 64V signal.  This then sets the frequency of */
	/* the 555 timer (C9).  The final signal is /2  */
	/* to set a 50% duty cycle.  We can just start  */
	/* the frequency at half.                       */
	/* NOTE: when first powered up the counter is   */
	/* not at TC, so the score is counted once.     */
	/* This should also happen on the original PCB. */
	/************************************************/
	DISCRETE_COUNTER_FIX(NODE_30,		// Counter E8 (9316 is a 74161)
	                     NODE_32,		// Clock enabled by F8, pin 13
	                     POOLSHRK_SCORE_EN,	// Reset/triggered by score
	                     15750.0/2.0/64.0,	// 64V signal
	                     15, 1,		// 4 bit binary up counter
	                     0)			// Cleared to 0
	DISCRETE_TRANSFORM2(NODE_31, 1, NODE_30, 15, "01=")	// TC output of E8, pin 15.
	DISCRETE_LOGIC_INVERT(NODE_32, 1, NODE_31)	// TC output modified to function as F8 clock enable
	DISCRETE_MULTADD(NODE_33, 1, NODE_31, 200, 100)	// Freqs not tested.
	DISCRETE_SQUAREWAVE(POOLSHRK_SCORE_SND,		// D9, pin 3 is 1/2 of 555 timer C9
				NODE_32,		// buffer B8, pin 2
				NODE_33,		// frequency
				250.8, 50.0, 250.8/2, 0.0)

	/*
	 * The TC ouput of E8 is sent to a one shot made up by
	 * C12/R62.  Clamped by CR16. Shaped to square by L9.
	 * This causes click to be triggered at the end of score.
	 */
	DISCRETE_ONESHOT(NODE_39,	// buffer L9 pin 12
			 NODE_31,	// from TC pin 15 of E8
			 1, 0,		// output 0/1 for the minimum sample period
			 DISC_ONESHOT_REDGE | DISC_ONESHOT_NORETRIG | DISC_OUT_ACTIVE_HIGH)

	/************************************************/
	/* Click is a triggered 0-15 count of the       */
	/* 2V signal.  It is also triggered at the end  */
	/* of the score sound.                          */
	/************************************************/
	DISCRETE_LOGIC_OR(NODE_40 ,1 ,POOLSHRK_CLICK_EN , NODE_39)	// gate K9, pin 11
	DISCRETE_COUNTER_FIX(NODE_41,		// Counter J9 (9316 is a 74161)
	                     NODE_42,		// Clock enabled by F8, pin 1
	                     NODE_40,		// Reset/triggered by K9, pin 11
	                     15750.0/2.0/2.0,	// 2V signal
	                     15, 1,		// 4 bit binary up counter
	                     0)			// Cleared to 0
	DISCRETE_TRANSFORM2(NODE_42, 1, NODE_41, 15, "01=!")	// TC output of J9, pin 15. Modified to function as F8 clock enable
	DISCRETE_TRANSFORM3(POOLSHRK_CLICK_SND, 1, NODE_41, 1, 250.8, "01&2*")	// Q0 output of J9, pin 14.  Set to proper amplitude

	/************************************************/
	/* Final gain and ouput.                        */
	/************************************************/
	DISCRETE_ADDER4(NODE_90, 1, POOLSHRK_BUMP_SND, POOLSHRK_SCRATCH_SND, POOLSHRK_CLICK_SND, POOLSHRK_SCORE_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0 + 250.8 + 250.8 + 250.8))
	DISCRETE_CRFILTER(NODE_92, 1, NODE_91, 100000, 1e-7)	// C21 feeds amp, 100K is a guess at amp impedance.
	DISCRETE_OUTPUT(NODE_92, 100)
DISCRETE_SOUND_END


static MACHINE_DRIVER_START( poolshrk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 11055000 / 8) /* ? */
	MDRV_CPU_PROGRAM_MAP(poolshrk_cpu_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 1)

	MDRV_FRAMES_PER_SECOND(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(1, 255, 24, 255)
	MDRV_GFXDECODE(poolshrk_gfx_decode_info)
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(poolshrk)
	MDRV_VIDEO_START(poolshrk)
	MDRV_VIDEO_UPDATE(poolshrk)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, poolshrk_sound_interface)
MACHINE_DRIVER_END


ROM_START( poolshrk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "7329.k1", 0x7000, 0x800, CRC(88152245) SHA1(c7c5e43ea488a197e92a1dc2231578f8ed86c98d) )
	ROM_LOAD( "7330.l1", 0x7800, 0x800, CRC(fb41d3e9) SHA1(c17994179362da13acfcd36a28f45e328428c031) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )   /* sprites */
	ROM_LOAD( "7325.j5", 0x0000, 0x200, CRC(fae87eed) SHA1(8891d0ea60f72f826d71dc6b064a2ba81b298914) )
	ROM_LOAD( "7326.h5", 0x0200, 0x200, CRC(05ec9762) SHA1(6119c4529334c98a0a42ca13a98a8661fc594d80) )

	ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "7328.n6", 0x0000, 0x200, CRC(64bcbf3a) SHA1(a4e3ce6b4734234359e3ef784a771e40580c2a2a) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )                   /* line offsets */
	ROM_LOAD( "7327.k6", 0x0000, 0x020, CRC(f74cef5b) SHA1(f470bf5b193dae4b44e89bc4c4476cf8d98e7cfd) )
ROM_END


GAME( 1977, poolshrk, 0, poolshrk, poolshrk, poolshrk, 0, "Atari", "Poolshark" )
