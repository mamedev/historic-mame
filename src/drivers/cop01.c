/***************************************************************************

Cops 01      (c) 1985 Nichibutsu
Mighty Guy   (c) 1986 Nichibutsu

driver by Carlos A. Lozano <calb@gsyc.inf.uc3m.es>

TODO:
----
mightguy:
- crashes during the confrontation with the final boss (only tested with Invincibility on)
- missing emulation of the 1412M2 protection chip, used by the sound CPU.


Mighty Guy board layout:
-----------------------

  cpu

12MHz     SW1
          SW2                 clr.13D clr.14D clr.15D      clr.19D

      Nichibutsu
      NBB 60-06                        4     5

  1 2 3  6116 6116                    6116   6116

-------

 video

 6116   11  Nichibutsu
            NBA 60-07    13B                               20MHz
                                2148                2148
                                2148                2148

              6116             9                        8  2E
 20G          10               7                        6
 -------

 audio sub-board MT-S3
 plugs into 40 pin socket at 20G

                     10.IC2

                     Nichibutsu
                     1412M2 (Yamaha 3810?)
                               8MHz
                     YM3526

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

#define MIGHTGUY_HACK	0


extern data8_t *cop01_bgvideoram,*cop01_fgvideoram;

PALETTE_INIT( cop01 );
VIDEO_START( cop01 );
VIDEO_UPDATE( cop01 );
WRITE_HANDLER( cop01_background_w );
WRITE_HANDLER( cop01_foreground_w );
WRITE_HANDLER( cop01_vreg_w );


static WRITE_HANDLER( cop01_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

static READ_HANDLER( cop01_sound_command_r )
{
	int res;
	static int pulse;
#define TIMER_RATE 12000	/* total guess */


	res = (soundlatch_r(offset) & 0x7f) << 1;

	/* bit 0 seems to be a timer */
	if ((cpu_gettotalcycles() / TIMER_RATE) & 1)
	{
		if (pulse == 0) res |= 1;
		pulse = 1;
	}
	else pulse = 0;

	return res;
}


static READ_HANDLER( mightguy_dsw_r )
{
	int data = 0xff;

	switch (offset)
	{
		case 0 :				// DSW1
			data = (readinputport(3) & 0x7f) | ((readinputport(5) & 0x04) << 5);
			break;
		case 1 :				// DSW2
			data = (readinputport(4) & 0x3f) | ((readinputport(5) & 0x03) << 6);
			break;
		}

	return (data);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },	/* c000-c7ff in cop01 */
	{ 0xd000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },	/* c000-c7ff in cop01 */
	{ 0xd000, 0xdfff, cop01_background_w, &cop01_bgvideoram },
	{ 0xe000, 0xe0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf3ff, cop01_foreground_w, &cop01_fgvideoram },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
PORT_END

static PORT_READ_START( mightguy_readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x04, mightguy_dsw_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x40, 0x43, cop01_vreg_w },
	{ 0x44, 0x44, cop01_sound_command_w },
	{ 0x45, 0x45, watchdog_reset_w }, /* ? */
PORT_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8000, MRA_NOP },	/* irq ack? */
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x06, 0x06, cop01_sound_command_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
	{ 0x04, 0x04, AY8910_control_port_2_w },
	{ 0x05, 0x05, AY8910_write_port_2_w },
PORT_END

/* this just gets some garbage out of the YM3526 */
static READ_HANDLER( kludge ) { static int timer; return timer++; }

static PORT_READ_START( mightguy_sound_readport )
	{ 0x03, 0x03, kludge },		/* 1412M2? */
	{ 0x06, 0x06, cop01_sound_command_r },
PORT_END

static PORT_WRITE_START( mightguy_sound_writeport )
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x02, MWA_NOP },	/* 1412M2? */
	{ 0x03, 0x03, MWA_NOP },	/* 1412M2? */
PORT_END



INPUT_PORTS_START( cop01 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST, COIN, START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, "1st Bonus Life" )
	PORT_DIPSETTING(    0x10, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPNAME( 0x60, 0x60, "2nd Bonus Life" )
	PORT_DIPSETTING(    0x60, "30000" )
	PORT_DIPSETTING(    0x20, "50000" )
	PORT_DIPSETTING(    0x40, "100000" )
	PORT_DIPSETTING(    0x00, "150000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* There is an ingame bug at 0x00e4 to 0x00e6 that performs the 'rrca' instead of 'rlca'
   so you DSW1-8 has no effect and you can NOT start a game at areas 5 to 8. */
INPUT_PORTS_START( mightguy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE )	/* same as the service dip switch */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "every 200000" )
	PORT_DIPSETTING(	0x00, "only 500000" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	// "Start Area" - see fake Dip Switch

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x30, "Easy" )
	PORT_DIPSETTING(	0x20, "Normal" )
	PORT_DIPSETTING(	0x10, "Hard" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invincibility", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_SPECIAL )	// "Start Area" - see fake Dip Switch

	PORT_START	/* FAKE Dip Switch */
	PORT_DIPNAME( 0x07, 0x07, "Starting Area" )
	PORT_DIPSETTING(	0x07, "1" )
	PORT_DIPSETTING(	0x06, "2" )
	PORT_DIPSETTING(	0x05, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	/* Not working due to ingame bug (see above) */
#if MIGHTGUY_HACK
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPSETTING(	0x02, "6" )
	PORT_DIPSETTING(	0x01, "7" )
	PORT_DIPSETTING(	0x00, "8" )
#endif
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4+8*0, 0+8*0, 4+8*1, 0+8*1, 4+8*2, 0+8*2, 4+8*3, 0+8*3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{
		RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0,   4, 0,
		RGN_FRAC(1,2)+12, RGN_FRAC(1,2)+8,  12, 8,
		RGN_FRAC(1,2)+20, RGN_FRAC(1,2)+16, 20, 16,
		RGN_FRAC(1,2)+28, RGN_FRAC(1,2)+24, 28, 24,
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32
	},
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,         0,  1 },
	{ REGION_GFX2, 0, &tilelayout,        16,  8 },
	{ REGION_GFX3, 0, &spritelayout, 16+8*16, 16 },
	{ -1 }
};



static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 15, 15, 15 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface YM3526_interface =
{
	1,
	4000000,	/* 4 MHz??? */
	{ 100 }
};



static MACHINE_DRIVER_START( cop01 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16+8*16+16*16)

	MDRV_PALETTE_INIT(cop01)
	MDRV_VIDEO_START(cop01)
	MDRV_VIDEO_UPDATE(cop01)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mightguy )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(mightguy_readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(mightguy_sound_readport,mightguy_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16+8*16+16*16)

	MDRV_PALETTE_INIT(cop01)
	MDRV_VIDEO_START(cop01)
	MDRV_VIDEO_UPDATE(cop01)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3526, YM3526_interface)
MACHINE_DRIVER_END



ROM_START( cop01 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cop01.2b",     0x0000, 0x4000, 0x5c2734ab )
	ROM_LOAD( "cop02.4b",     0x4000, 0x4000, 0x9c7336ef )
	ROM_LOAD( "cop03.5b",     0x8000, 0x4000, 0x2566c8bf )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for code */
	ROM_LOAD( "cop15.17b",    0x0000, 0x4000, 0x6a5f08fa )
	ROM_LOAD( "cop16.18b",    0x4000, 0x4000, 0x56bf6946 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cop14.15g",    0x00000, 0x2000, 0x066d1c55 )	/* chars */

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cop04.15c",    0x00000, 0x4000, 0x622d32e6 )	/* tiles */
	ROM_LOAD( "cop05.16c",    0x04000, 0x4000, 0xc6ac5a35 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cop06.3g",     0x00000, 0x2000, 0xf1c1f4a5 )	/* sprites */
	ROM_LOAD( "cop07.5g",     0x02000, 0x2000, 0x11db7b72 )
	ROM_LOAD( "cop08.6g",     0x04000, 0x2000, 0xa63ddda6 )
	ROM_LOAD( "cop09.8g",     0x06000, 0x2000, 0x855a2ec3 )
	ROM_LOAD( "cop10.3e",     0x08000, 0x2000, 0x444cb19d )
	ROM_LOAD( "cop11.5e",     0x0a000, 0x2000, 0x9078bc04 )
	ROM_LOAD( "cop12.6e",     0x0c000, 0x2000, 0x257a6706 )
	ROM_LOAD( "cop13.8e",     0x0e000, 0x2000, 0x07c4ea66 )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "copproma.13d", 0x0000, 0x0100, 0x97f68a7a )	/* red */
	ROM_LOAD( "coppromb.14d", 0x0100, 0x0100, 0x39a40b4c )	/* green */
	ROM_LOAD( "coppromc.15d", 0x0200, 0x0100, 0x8181748b )	/* blue */
	ROM_LOAD( "coppromd.19d", 0x0300, 0x0100, 0x6a63dbb8 )	/* tile lookup table */
	ROM_LOAD( "copprome.2e",  0x0400, 0x0100, 0x214392fa )	/* sprite lookup table */
ROM_END

ROM_START( cop01a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cop01alt.001", 0x0000, 0x4000, 0xa13ee0d3 )
	ROM_LOAD( "cop01alt.002", 0x4000, 0x4000, 0x20bad28e )
	ROM_LOAD( "cop01alt.003", 0x8000, 0x4000, 0xa7e10b79 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for code */
	ROM_LOAD( "cop01alt.015", 0x0000, 0x4000, 0x95be9270 )
	ROM_LOAD( "cop01alt.016", 0x4000, 0x4000, 0xc20bf649 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cop01alt.014", 0x00000, 0x2000, 0xedd8a474 )	/* chars */

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cop04.15c",    0x00000, 0x4000, 0x622d32e6 )	/* tiles */
	ROM_LOAD( "cop05.16c",    0x04000, 0x4000, 0xc6ac5a35 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cop01alt.006", 0x00000, 0x2000, 0xcac7dac8 )	/* sprites */
	ROM_LOAD( "cop07.5g",     0x02000, 0x2000, 0x11db7b72 )
	ROM_LOAD( "cop08.6g",     0x04000, 0x2000, 0xa63ddda6 )
	ROM_LOAD( "cop09.8g",     0x06000, 0x2000, 0x855a2ec3 )
	ROM_LOAD( "cop01alt.010", 0x08000, 0x2000, 0x94aee9d6 )
	ROM_LOAD( "cop11.5e",     0x0a000, 0x2000, 0x9078bc04 )
	ROM_LOAD( "cop12.6e",     0x0c000, 0x2000, 0x257a6706 )
	ROM_LOAD( "cop13.8e",     0x0e000, 0x2000, 0x07c4ea66 )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "copproma.13d", 0x0000, 0x0100, 0x97f68a7a )	/* red */
	ROM_LOAD( "coppromb.14d", 0x0100, 0x0100, 0x39a40b4c )	/* green */
	ROM_LOAD( "coppromc.15d", 0x0200, 0x0100, 0x8181748b )	/* blue */
	ROM_LOAD( "coppromd.19d", 0x0300, 0x0100, 0x6a63dbb8 )	/* tile lookup table */
	ROM_LOAD( "copprome.2e",  0x0400, 0x0100, 0x214392fa )	/* sprite lookup table */
	/* a timing PROM (13B?) is probably missing */
ROM_END

ROM_START( mightguy )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* Z80 code (main cpu) */
	ROM_LOAD( "1.2b",       0x0000, 0x4000,0xbc8e4557 )
	ROM_LOAD( "2.4b",       0x4000, 0x4000,0xfb73d684 )
	ROM_LOAD( "3.5b",       0x8000, 0x4000,0xb14b6ab8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound cpu) */
	ROM_LOAD( "11.15b",     0x0000, 0x4000, 0x576183ea)

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* 1412M2 protection data */
	ROM_LOAD( "10.ic2",     0x0000, 0x8000, 0x1a5d2bb1 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE ) /* alpha */
	ROM_LOAD( "10.15g",     0x0000, 0x2000, 0x17874300)

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "4.15c",      0x0000, 0x4000,0x84d29e76 )
	ROM_LOAD( "5.16c",      0x4000, 0x4000,0xf7bb8d82 )

	ROM_REGION( 0x14000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "6.3g",       0x00000, 0x2000, 0x6ff88615)
	ROM_LOAD( "7.8g",       0x02000, 0x8000, 0xe79ea66f)
	ROM_LOAD( "8.3e",       0x0a000, 0x2000, 0x29f6eb44)
	ROM_LOAD( "9.8e",       0x0c000, 0x8000, 0xb9f64c6f)

	ROM_REGION( 0x600, REGION_PROMS, 0 )
	ROM_LOAD( "clr.13d",    0x000, 0x100, 0xc4cf0bdd ) /* red */
	ROM_LOAD( "clr.14d",    0x100, 0x100, 0x5b3b8a9b ) /* green */
	ROM_LOAD( "clr.15d",    0x200, 0x100, 0x6c072a64 ) /* blue */
	ROM_LOAD( "clr.19d",    0x300, 0x100, 0x19b66ac6 ) /* tile lookup table */
	ROM_LOAD( "2e",         0x400, 0x100, 0xd9c45126 ) /* sprite lookup table */
	ROM_LOAD( "13b",        0x500, 0x100, 0x4a6f9a6d ) /* state machine data used for video signals generation (not used in emulation)*/
ROM_END


static DRIVER_INIT( mightguy )
{
#if MIGHTGUY_HACK
	/* This is a hack to fix the game code to get a fully working
	   "Starting Area" fake Dip Switch */
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU1);
	RAM[0x00e4] = 0x07;	// rlca
	RAM[0x00e5] = 0x07;	// rlca
	RAM[0x00e6] = 0x07;	// rlca
	/* To avoid checksum error */
	RAM[0x027f] = 0x00;
	RAM[0x0280] = 0x00;
#endif
}


GAME( 1985, cop01,    0,     cop01,    cop01,    0,        ROT0,   "Nichibutsu", "Cop 01 (set 1)" )
GAME( 1985, cop01a,   cop01, cop01,    cop01,    0,        ROT0,   "Nichibutsu", "Cop 01 (set 2)" )
GAMEX(1986, mightguy, 0,     mightguy, mightguy, mightguy, ROT270, "Nichibutsu", "Mighty Guy", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
