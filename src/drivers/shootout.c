/*
Shoot Out - (c) 1985 Data East

Preliminary driver by:
Ernesto Corvi (ernesto@imagina.com)
Phil Stroffolino

TODO:
- Verify Controls and Dipswitches mapped correctly ( eg: No sound on attract mode, even when its on ).
- Add cocktail support.
*/

extern void btime_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

/* externals: from vidhrdw */
extern int shootout_vh_start( void );
extern void shootout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
unsigned char *shootout_textram;



static void shootout_bankswitch_w( int offset, int v ) {
	int bankaddress;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);
	bankaddress = 0x10000 + ( 0x4000 * v );

	cpu_setbank(1,&RAM[bankaddress]);
}

static void sound_cpu_command_w( int offset, int v ) {
	soundlatch_w( offset, v );
	cpu_cause_interrupt( 1, M6502_INT_NMI );
}

/* stub for reading input ports as active low (makes building ports much easier) */
static int low_input_r( int offset ) {
	return ~readinputport( offset );
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1003, low_input_r },
	{ 0x1004, 0x1fff, MRA_RAM },
	{ 0x2000, 0x27ff, MRA_RAM },	/* foreground */
	{ 0x2800, 0x2bff, videoram_r }, /* background videoram */
	{ 0x2c00, 0x2fff, colorram_r }, /* background colorram */
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ef, MWA_RAM },
	{ 0x01f0, 0x01ff, MWA_RAM },
	{ 0x0200, 0x0fff, MWA_RAM },
//	{ 0x1000, 0x1002, MWA_RAM }, /* these do get written to - see error log */
	{ 0x1003, 0x1003, sound_cpu_command_w },
	{ 0x1000, 0x1000, shootout_bankswitch_w }, /* not entirely sure about this */
	{ 0x1004, 0x17ff, MWA_RAM },
	{ 0x1800, 0x19ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x27ff, MWA_RAM, &shootout_textram },
	{ 0x2800, 0x2bff, videoram_w, &videoram, &videoram_size },
	{ 0x2c00, 0x2fff, colorram_w, &colorram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x4000, 0x4000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4000, YM2203_control_port_0_w },
	{ 0x4001, 0x4001, YM2203_write_port_0_w },
	{ 0xd000, 0xd000, interrupt_enable_w },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( shootout )
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START 	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX(0,        0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k 70k" )
	PORT_DIPSETTING(    0x04, "30k 80k" )
	PORT_DIPSETTING(    0x08, "40k 90k" )
	PORT_DIPSETTING(    0x0c, "70k" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "Hardest" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* this is set when either coin is inserted */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END


static struct GfxLayout char_layout =
{
	8,8,	/* 8*8 characters */
	0x400,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0,4 },	/* the bitplanes are packed in the same byte */
	{ (0x2000*8)+0, (0x2000*8)+1, (0x2000*8)+2, (0x2000*8)+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout sprite_layout =
{
	16,16,	/* 16*16 sprites */
	0x800,	/* 2048 sprites */
	3,	/* 3 bits per pixel */
	{ 0*0x10000*8, 1*0x10000*8, 2*0x10000*8 },	/* the bitplanes are separated */
	{ 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every char takes 32 consecutive bytes */
};
static struct GfxLayout tile_layout =
{
	8,8,	/* 8*8 characters */
	0x800,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 0,4 },	/* the bitplanes are packed in the same byte */
	{ (0x4000*8)+0, (0x4000*8)+1, (0x4000*8)+2, (0x4000*8)+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,   16*4+8*8, 16 }, /* characters */
	{ REGION_GFX2, 0, &sprite_layout, 16*4,      8 }, /* sprites */
	{ REGION_GFX3, 0, &tile_layout,   0,        16 }, /* tiles */
	{ -1 } /* end of array */
};

static struct YM2203interface ym2203_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ??? */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static int shootout_interrupt(void)
{
	static int coin = 0;

	if ( readinputport( 2 ) & 0xc0 ) {
		if ( coin == 0 ) {
			coin = 1;
			return nmi_interrupt();
		}
	} else
		coin = 0;

	return ignore_interrupt();
}

static struct MachineDriver machine_driver_shootout =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,	/* 2 Mhz? */
			readmem,writemem,0,0,
			shootout_interrupt,1 /* nmi's are triggered at coin up */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz? */
			sound_readmem,sound_writemem,0,0,
			interrupt,8 /* this is a guess, but sounds just about right */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	256,256,
	btime_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	shootout_vh_start,
	generic_vh_stop,
	shootout_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



ROM_START( shootout )
	ROM_REGION( 2*0x20000, REGION_CPU1 )	/* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "cu00.b1",        0x08000, 0x8000, 0x090edeb6 ) /* opcodes encrypted */
	/* banked at 0x4000-0x8000 */
	ROM_LOAD( "cu02.c3",        0x10000, 0x8000, 0x2a913730 ) /* opcodes encrypted */
	ROM_LOAD( "cu01.c1",        0x18000, 0x4000, 0x8843c3ae ) /* opcodes encrypted */

	ROM_REGION( 0x10000, REGION_CPU2 ) /* 64k for code */
	ROM_LOAD( "cu09.j1",        0x0c000, 0x4000, 0xc4cbd558 ) /* Sound CPU */

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cu11.h19",       0x00000, 0x4000, 0xeff00460 ) /* foreground characters */

	ROM_REGION( 0x30000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cu04.c7",        0x00000, 0x8000, 0xceea6b20 )	/* sprites */
	ROM_LOAD( "cu03.c5",        0x08000, 0x8000, 0xb786bb3e )
	ROM_LOAD( "cu06.c10",       0x10000, 0x8000, 0x2ec1d17f )
	ROM_LOAD( "cu05.c9",        0x18000, 0x8000, 0xdd038b85 )
	ROM_LOAD( "cu08.c13",       0x20000, 0x8000, 0x91290933 )
	ROM_LOAD( "cu07.c12",       0x28000, 0x8000, 0x19b6b94f )

	ROM_REGION( 0x08000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cu10.h17",       0x00000, 0x2000, 0x3854c877 ) /* background tiles */
	ROM_CONTINUE(               0x04000, 0x2000 )
	ROM_CONTINUE(               0x02000, 0x2000 )
	ROM_CONTINUE(               0x06000, 0x2000 )

	ROM_REGION( 0x0200, REGION_PROMS )
	ROM_LOAD( "gb08.k10",       0x0000, 0x0100, 0x509c65b6 )
	ROM_LOAD( "gb09.k6",        0x0100, 0x0100, 0xaa090565 )	/* unknown */
ROM_END



static void init_shootout(void)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;
	int A;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0;A < diff;A++)
		rom[A+diff] = (rom[A] & 0x9f) | ((rom[A] & 0x40) >> 1) | ((rom[A] & 0x20) << 1);
}



GAME( 1985, shootout, 0, shootout, shootout, shootout, ROT0, "Data East USA", "Shoot Out (US)" )
