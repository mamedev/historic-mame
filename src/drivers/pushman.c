/***************************************************************************

	Pushman							(c) 1990 Comad

	With 'Debug Mode' on button 2 advances a level, button 3 goes back.

	The microcontroller mainly controls the animation of the enemy robots,
	the communication between the 68000 and MCU is probably not emulated
	100% correct but it works.  Later levels (using the cheat mode) seem
	to have some corrupt tilemaps, I'm not sure if this is a driver bug
	or a game bug from using the cheat mode.

	Text layer banking is wrong on the continue screen.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"

void pushman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE16_HANDLER( pushman_scroll_w );
WRITE16_HANDLER( pushman_videoram_w );
int pushman_vh_start(void);

static UINT8 shared_ram[8];
static UINT16 latch,new_latch=0;

/******************************************************************************/

static WRITE16_HANDLER( pushman_control_w )
{
	if (ACCESSING_MSB)
		soundlatch_w(0,(data>>8)&0xff);
}

static READ16_HANDLER( pushman_68705_r )
{
	if (offset==0)
		return latch;

	if (offset==3 && new_latch) { new_latch=0; return 0; }
	if (offset==3 && !new_latch) return 0xff;

	return (shared_ram[2*offset+1]<<8)+shared_ram[2*offset];
}

static WRITE16_HANDLER( pushman_68705_w )
{
	if (ACCESSING_MSB)
		shared_ram[2*offset]=data>>8;
	if (ACCESSING_LSB)
		shared_ram[2*offset+1]=data&0xff;

	if (offset==1)
	{
		cpu_cause_interrupt(1,M68705_INT_IRQ);
		cpu_spin();
		new_latch=0;
	}
}

static READ_HANDLER( pushman_68000_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( pushman_68000_w )
{
	if (offset==2 && (shared_ram[2]&2)==0 && data&2) {
		latch=(shared_ram[1]<<8)|shared_ram[0];
		new_latch=1;
	}
	shared_ram[offset]=data;
}

/******************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x060000, 0x060007, pushman_68705_r },
	{ 0xfe0800, 0xfe17ff, MRA16_RAM },
	{ 0xfe4000, 0xfe4001, input_port_0_word_r },
	{ 0xfe4002, 0xfe4003, input_port_1_word_r },
	{ 0xfe4004, 0xfe4005, input_port_2_word_r },
	{ 0xfec000, 0xfec7ff, MRA16_RAM },
	{ 0xff8000, 0xff87ff, MRA16_RAM },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x060000, 0x060007, pushman_68705_w },
	{ 0xfe0800, 0xfe17ff, MWA16_RAM, &spriteram16 },
	{ 0xfe4002, 0xfe4003, pushman_control_w },
	{ 0xfe8000, 0xfe8003, pushman_scroll_w },
	{ 0xfe800e, 0xfe800f, MWA16_NOP }, /* ? */
	{ 0xfec000, 0xfec7ff, pushman_videoram_w, &videoram16 },
	{ 0xff8000, 0xff87ff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0007, pushman_68000_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0007, pushman_68000_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x0fff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
PORT_END

/******************************************************************************/

INPUT_PORTS_START( pushman )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_VBLANK ) /* not sure, probably wrong */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BITX(    0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Level Select" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout tilelayout =
{
	32,32,
	RGN_FRAC(1,2),
	4,
	{ 4, 0, RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8
};

static struct GfxDecodeInfo pushman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &charlayout,   0x300, 16 },	/* colors 0x300-0x33f */
	{ REGION_GFX2, 0x000000, &spritelayout, 0x200, 16 },	/* colors 0x200-0x2ff */
	{ REGION_GFX3, 0x000000, &tilelayout,   0x100, 16 },	/* colors 0x100-0x1ff */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	2000000,
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static UINT32 amask_m68705 = 0xfff;

static struct MachineDriver machine_driver_pushman =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,
			readmem,writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_M68705,
			400000,	/* No idea */
			mcu_readmem,mcu_writemem,0,0,
			ignore_interrupt,1,
			0,0,
			&amask_m68705
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			sound_readmem,sound_writemem,0,sound_writeport,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	60,					/* CPU interleave  */
	0,					/* Hardware initialization-function */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	pushman_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pushman_vh_start,
	0,
	pushman_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

/***************************************************************************/


ROM_START( pushman )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pman-12.212", 0x000000, 0x10000, 0x4251109d )
	ROM_LOAD16_BYTE( "pman-11.197", 0x000001, 0x10000, 0x1167ed9f )

	ROM_REGION( 0x01000, REGION_CPU2, 0 )
	ROM_LOAD( "pushman.uc",  0x00000, 0x01000, 0xd7916657 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "pman-13.216", 0x00000, 0x08000, 0xbc03827a )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-1.130",  0x00000, 0x08000, 0x14497754 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-4.58", 0x00000, 0x10000, 0x16e5ce6b )
	ROM_LOAD( "pman-5.59", 0x10000, 0x10000, 0xb82140b8 )
	ROM_LOAD( "pman-2.56", 0x20000, 0x10000, 0x2cb2ac29 )
	ROM_LOAD( "pman-3.57", 0x30000, 0x10000, 0x8ab957c8 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pman-6.131", 0x00000, 0x10000, 0xbd0f9025 )
	ROM_LOAD( "pman-8.148", 0x10000, 0x10000, 0x591bd5c0 )
	ROM_LOAD( "pman-7.132", 0x20000, 0x10000, 0x208cb197 )
	ROM_LOAD( "pman-9.149", 0x30000, 0x10000, 0x77ee8577 )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )	/* bg tilemaps */
	ROM_LOAD( "pman-10.189", 0x00000, 0x08000, 0x5f9ae9a1 )
ROM_END


GAME( 1990, pushman, 0, pushman, pushman, 0, ROT0, "Comad (American Sammy license)", "Pushman" )
