/***************************************************************************

Splash! (c) 1992 Gaelco

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"


extern data16_t *splash_vregs;
extern data16_t *splash_videoram;
extern data16_t *splash_spriteram;
extern data16_t *splash_pixelram;

/* from vidhrdw/gaelco.c */
READ16_HANDLER( splash_vram_r );
READ16_HANDLER( splash_pixelram_r );
WRITE16_HANDLER( splash_vram_w );
WRITE16_HANDLER( splash_pixelram_w );
VIDEO_START( splash );
VIDEO_UPDATE( splash );


static WRITE16_HANDLER( splash_sh_irqtrigger_w )
{
	if (ACCESSING_LSB){
		soundlatch_w(0,data & 0xff);
		cpu_set_irq_line(1,0,HOLD_LINE);
	}
}

static ADDRESS_MAP_START( splash_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_READ(MRA16_ROM)			/* ROM */
	AM_RANGE(0x800000, 0x83ffff) AM_READ(splash_pixelram_r)	/* Pixel Layer */
	AM_RANGE(0x840000, 0x840001) AM_READ(input_port_0_word_r)/* DIPSW #1 */
	AM_RANGE(0x840002, 0x840003) AM_READ(input_port_1_word_r)/* DIPSW #2 */
	AM_RANGE(0x840004, 0x840005) AM_READ(input_port_2_word_r)/* INPUT #1 */
	AM_RANGE(0x840006, 0x840007) AM_READ(input_port_3_word_r)/* INPUT #2 */
	AM_RANGE(0x880000, 0x8817ff) AM_READ(splash_vram_r)		/* Video RAM */
	AM_RANGE(0x881800, 0x881803) AM_READ(MRA16_RAM)			/* Scroll registers */
	AM_RANGE(0x881804, 0x881fff) AM_READ(MRA16_RAM)			/* Work RAM */
	AM_RANGE(0x8c0000, 0x8c0fff) AM_READ(MRA16_RAM)			/* Palette */
	AM_RANGE(0x900000, 0x900fff) AM_READ(MRA16_RAM)			/* Sprite RAM */
	AM_RANGE(0xffc000, 0xffffff) AM_READ(MRA16_RAM)			/* Work RAM */
ADDRESS_MAP_END

WRITE16_HANDLER( splash_coin_w )
{
	if (ACCESSING_MSB){
		switch ((offset >> 3)){
			case 0x00:	/* Coin Lockouts */
			case 0x01:
				coin_lockout_w( (offset >> 3) & 0x01, (data & 0x0400) >> 8);
				break;
			case 0x02:	/* Coin Counters */
			case 0x03:
				coin_counter_w( (offset >> 3) & 0x01, (data & 0x0100) >> 8);
				break;
		}
	}
}

static ADDRESS_MAP_START( splash_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_WRITE(MWA16_ROM)										/* ROM */
	AM_RANGE(0x800000, 0x83ffff) AM_WRITE(splash_pixelram_w) AM_BASE(&splash_pixelram)			/* Pixel Layer */
	AM_RANGE(0x84000e, 0x84000f) AM_WRITE(splash_sh_irqtrigger_w)							/* Sound command */
	AM_RANGE(0x84000a, 0x84003b) AM_WRITE(splash_coin_w)									/* Coin Counters + Coin Lockout */
	AM_RANGE(0x880000, 0x8817ff) AM_WRITE(splash_vram_w) AM_BASE(&splash_videoram)				/* Video RAM */
	AM_RANGE(0x881800, 0x881803) AM_WRITE(MWA16_RAM) AM_BASE(&splash_vregs)						/* Scroll registers */
	AM_RANGE(0x881804, 0x881fff) AM_WRITE(MWA16_RAM)										/* Work RAM */
	AM_RANGE(0x8c0000, 0x8c0fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)/* Palette is xRRRRxGGGGxBBBBx */
	AM_RANGE(0x900000, 0x900fff) AM_WRITE(MWA16_RAM) AM_BASE(&splash_spriteram)					/* Sprite RAM */
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(MWA16_RAM)										/* Work RAM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( splash_readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xd7ff) AM_READ(MRA8_ROM)					/* ROM */
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_r)				/* Sound latch */
	AM_RANGE(0xf000, 0xf000) AM_READ(YM3812_status_port_0_r)		/* YM3812 */
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)					/* RAM */
ADDRESS_MAP_END

static int adpcm_data;

static WRITE_HANDLER( splash_adpcm_data_w ){
	adpcm_data = data;
}

static void splash_msm5205_int(int data)
{
	MSM5205_data_w(0,adpcm_data >> 4);
	adpcm_data = (adpcm_data << 4) & 0xf0;
}


static ADDRESS_MAP_START( splash_writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xd7ff) AM_WRITE(MWA8_ROM)					/* ROM */
	AM_RANGE(0xd800, 0xd800) AM_WRITE(splash_adpcm_data_w)		/* ADPCM data for the MSM5205 chip */
//	AM_RANGE(0xe000, 0xe000) AM_WRITE(MWA8_NOP)					/* ??? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(YM3812_control_port_0_w)	/* YM3812 */
	AM_RANGE(0xf001, 0xf001) AM_WRITE(YM3812_write_port_0_w)		/* YM3812 */
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)					/* RAM */
ADDRESS_MAP_END


INPUT_PORTS_START( splash )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin B too)" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin A too)" )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	/* 	according to the manual, Lives = 0x00 is NOT used */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Girls" )
	PORT_DIPSETTING(    0x00, "Light" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Paint Mode" )
	PORT_DIPSETTING(    0x00, "Paint again" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


static struct GfxLayout tilelayout8 =
{
	8,8,									/* 8x8 tiles */
	0x20000/8,								/* number of tiles */
	4,										/* bitplanes */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	0x20000/32,								/* number of tiles */
	4,										/* bitplanes */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout8 ,0,128 },
	{ REGION_GFX1, 0x000000, &tilelayout16,0,128 },
	{ -1 }
};

static struct YM3812interface splash_ym3812_interface =
{
	1,						/* 1 chip */
	3000000,				/* 3 MHz? */
	{ 80 },					/* volume */
	{ 0 }					/* IRQ handler */
};

static struct MSM5205interface splash_msm5205_interface =
{
	1,						/* 1 chip */
	384000,					/* 384KHz */
	{ splash_msm5205_int },	/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8KHz */
	{ 80 }					/* volume */
};


static MACHINE_DRIVER_START( splash )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(splash_readmem,splash_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80,30000000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)			/* 3.75 MHz? */
	MDRV_CPU_PROGRAM_MAP(splash_readmem_sound,splash_writemem_sound)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,64)	/* needed for the msm5205 to play the samples */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(2*8, 49*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(splash)
	MDRV_VIDEO_UPDATE(splash)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, splash_ym3812_interface)
	MDRV_SOUND_ADD(MSM5205, splash_msm5205_interface)
MACHINE_DRIVER_END



/***************************************************************************

The Return of Lady Frog
Microhard, 1993

PCB Layout
----------


YM2203                            68000
YM3014    6116           **       2   6
          6116          6116      3   7
6264                              4   8
1  Z80           MACH130          5   9
                 681000        6264  6264


DSW2              2148                10
DSW1              2148  6264  30MHz   11
                  2148  6264  24MHz   12
                  2148                13

Notes:
      68000 Clock = >10MHz (my meter can only read up to 10.000MHz)
        Z80 Clock = 3MHz
               ** = possibly PLD (surface is scratched, type PLCC44)
    Vertical Sync = 60Hz
      Horiz. Sync = 15.56kHz





Lady Frog is a protected bootleg of splash, and the program jumps straight away
to an unmapped memory address (crashing at 0x00401E72)...

roldfrog.001 contains
VIDEO COMPUTER SYSTEM  (C)1989 DYNAX INC  NAGOYA JAPAN  DRAGON PUNCH  VER. 1.30

***************************************************************************/

ROM_START( roldfrog )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "roldfrog.002",	0x000000, 0x080000, CRC(724cf022) SHA1(f8cddfb785ae7900cb95b854811ec3fb250fa7fe) )
	ROM_LOAD16_BYTE( "roldfrog.006",	0x000001, 0x080000, CRC(e52a7ae2) SHA1(5c6ecbc2711376afdd7b8da11f84d36ffc464c8a) )
	ROM_LOAD16_BYTE( "roldfrog.003",	0x100000, 0x080000, CRC(a1d49967) SHA1(54d73c1db1090b7d5109906525ce95ee8c00ad1f) )
	ROM_LOAD16_BYTE( "roldfrog.007",	0x100001, 0x080000, CRC(e5805c4e) SHA1(5691807b711ea5137f91afd6033fcd734d2b5ad0) )
	ROM_LOAD16_BYTE( "roldfrog.004",	0x200000, 0x080000, CRC(709281f5) SHA1(01453168df4dc84069346cecd1fba9adeae6fcb8) )
	ROM_LOAD16_BYTE( "roldfrog.008",	0x200001, 0x080000, CRC(39adcba4) SHA1(6c8c945b6383fa2549e6654b427a7ce4c7ff46b5) )
	ROM_LOAD16_BYTE( "roldfrog.005",	0x300000, 0x080000, CRC(b683160c) SHA1(526a772108a6bf71207a7b6de7cbd14f8e9496bc) )
	ROM_LOAD16_BYTE( "roldfrog.009",	0x300001, 0x080000, CRC(e475fb76) SHA1(9ab56db86530647ea4a5d2109a02119710ff9b7e) )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "roldfrog.001", 0x00000, 0x20000, CRC(ba9eb1c6) SHA1(649d1103f3188554eaa3fc87a1f52c53233932b2) )
	ROM_RELOAD(               0x20000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "roldfrog.010",       0x00000, 0x20000, CRC(51fd0e1a) SHA1(940c4231b21d16c62cad47c22fe735b18662af4a) )
	ROM_LOAD( "roldfrog.011",       0x20000, 0x20000, CRC(610bf6f3) SHA1(04a7efac2e83c6747d4bd480b1f3118eb44a1f76) )
	ROM_LOAD( "roldfrog.012",       0x40000, 0x20000, CRC(466ede67) SHA1(2d44dba1e76e5ceebf33fa6fc148ed665701a7ff) )
	ROM_LOAD( "roldfrog.013",       0x60000, 0x20000, CRC(fad3e8be) SHA1(eccd7b1440d3a0d433c92ff33213326e0d57422a) )
ROM_END

ROM_START( roldfrga )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "roldfrog.002",	0x000000, 0x080000, CRC(724cf022) SHA1(f8cddfb785ae7900cb95b854811ec3fb250fa7fe) )
	ROM_LOAD16_BYTE( "roldfrog.006",	0x000001, 0x080000, CRC(e52a7ae2) SHA1(5c6ecbc2711376afdd7b8da11f84d36ffc464c8a) )
	ROM_LOAD16_BYTE( "roldfrog.003",	0x100000, 0x080000, CRC(a1d49967) SHA1(54d73c1db1090b7d5109906525ce95ee8c00ad1f) )
	ROM_LOAD16_BYTE( "roldfrog.007",	0x100001, 0x080000, CRC(e5805c4e) SHA1(5691807b711ea5137f91afd6033fcd734d2b5ad0) )
	ROM_LOAD16_BYTE( "roldfrog.004",	0x200000, 0x080000, CRC(709281f5) SHA1(01453168df4dc84069346cecd1fba9adeae6fcb8) )
	ROM_LOAD16_BYTE( "roldfrog.008",	0x200001, 0x080000, CRC(39adcba4) SHA1(6c8c945b6383fa2549e6654b427a7ce4c7ff46b5) )
	ROM_LOAD16_BYTE( "roldfrog.005",	0x300000, 0x080000, CRC(b683160c) SHA1(526a772108a6bf71207a7b6de7cbd14f8e9496bc) )
	ROM_LOAD16_BYTE( "9",	            0x300001, 0x080000, CRC(fd515b58) SHA1(7926ab9afbc260219351a02b56b82ede883f9aab) )	// differs with roldfrog.009 by 1 byte

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "roldfrog.001", 0x00000, 0x20000, CRC(ba9eb1c6) SHA1(649d1103f3188554eaa3fc87a1f52c53233932b2) )
	ROM_RELOAD(               0x20000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "roldfrog.010",       0x00000, 0x20000, CRC(51fd0e1a) SHA1(940c4231b21d16c62cad47c22fe735b18662af4a) )
	ROM_LOAD( "roldfrog.011",       0x20000, 0x20000, CRC(610bf6f3) SHA1(04a7efac2e83c6747d4bd480b1f3118eb44a1f76) )
	ROM_LOAD( "roldfrog.012",       0x40000, 0x20000, CRC(466ede67) SHA1(2d44dba1e76e5ceebf33fa6fc148ed665701a7ff) )
	ROM_LOAD( "roldfrog.013",       0x60000, 0x20000, CRC(fad3e8be) SHA1(eccd7b1440d3a0d433c92ff33213326e0d57422a) )
ROM_END


ROM_START( splash )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code + gfx */
	ROM_LOAD16_BYTE(	"4g",	0x000000, 0x020000, CRC(b38fda40) SHA1(37ddf4b6f9f2f6cc58efefc277bc3ae9dc71e6d0) )
	ROM_LOAD16_BYTE(	"4i",	0x000001, 0x020000, CRC(02359c47) SHA1(6817424b2b1afffa99cec5b8fae4fb8436db2bb5) )
	ROM_LOAD16_BYTE(	"5g",	0x100000, 0x080000, CRC(a4e8ed18) SHA1(64ce47193ee4bb3a8014d7c14c559b4ebb3af083) )
	ROM_LOAD16_BYTE(	"5i",	0x100001, 0x080000, CRC(73e1154d) SHA1(2c055ad29a32c6c1e712cc35b5972f1e69cdebb7) )
	ROM_LOAD16_BYTE(	"6g",	0x200000, 0x080000, CRC(ffd56771) SHA1(35ad9874b6ea5aa3ba38a31d723093b4dd2cfdb8) )
	ROM_LOAD16_BYTE(	"6i",	0x200001, 0x080000, CRC(16e9170c) SHA1(96fc237cb172039df153dc70d15ed7d9ee750363) )
	ROM_LOAD16_BYTE(	"8g",	0x300000, 0x080000, CRC(dc3a3172) SHA1(2b322b52e3e8da00f26dd276cb72bd2d48c2deaa) )
	ROM_LOAD16_BYTE(	"8i",	0x300001, 0x080000, CRC(2e23e6c3) SHA1(baf9ab4c3261c3f06f5e43c1e50aba9222acb71d) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )	/* Z80 code + sound data */
	ROM_LOAD( "5c",	0x00000, 0x10000, CRC(0ed7ebc9) SHA1(28ef16e20d754deef49be6a5c9f63311e9ec94a3) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "18i",	0x000000, 0x020000, CRC(028a4a68) SHA1(19384988e3690886ed55886ecdc4e4c566dbe4ba) )
	ROM_LOAD( "15i",	0x020000, 0x020000, CRC(2a8cb830) SHA1(bc54dfb03fade154085aa2f66784e07664a7a3d8) )
	ROM_LOAD( "16i",	0x040000, 0x020000, CRC(21aeff2c) SHA1(0c307e94f4a814c674ba0ab471a6bdd57e43c265) )
	ROM_LOAD( "13i",	0x060000, 0x020000, CRC(febb9893) SHA1(bb607a608c6c1658748a17a62431e8c30323c7ec) )
ROM_END


GAME( 1992, splash,   0,        splash, splash, 0, ROT0, "Gaelco",    "Splash! (Ver. 1.2 World)" )

GAMEX(1993, roldfrog, 0,        splash, splash, 0, ROT0, "Microhard", "The Return of Lady Frog", GAME_NOT_WORKING )
GAMEX(1993, roldfrga, roldfrog, splash, splash, 0, ROT0, "Microhard", "The Return of Lady Frog (set 2)", GAME_NOT_WORKING )
