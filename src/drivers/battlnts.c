/***************************************************************************

Battlantis(GX777) (c) 1987 Konami

Preliminary driver by:
	Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/konamiic.h"

/* from vidhrdw */
extern void battlnts_spritebank_w(int offset,int data);
int battlnts_vh_start(void);
void battlnts_vh_stop(void);
void battlnts_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int battlnts_interrupt( void )
{
	if (K007342_is_INT_enabled)
        return M6309_INT_IRQ;
    else
        return ignore_interrupt();
}

void battlnts_sh_irqtrigger_w(int offset, int data)
{
	cpu_cause_interrupt(1,0xff);
}

static void battlnts_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	/* bits 6 & 7 = bank number */
	bankaddress = 0x10000 + ((data & 0xc0) >> 6) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bits 4 & 5 = coin counters */
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);

	/* other bits unknown */
}

static struct MemoryReadAddress battlnts_readmem[] =
{
	{ 0x0000, 0x1fff, K007342_r },			/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_r },			/* Sprite RAM */
	{ 0x2200, 0x23ff, MRA_RAM },			/* ??? */
	{ 0x2400, 0x24ff, MRA_RAM },			/* Palette */
	{ 0x2e00, 0x2e00, input_port_0_r },		/* DIPSW #1 */
	{ 0x2e01, 0x2e01, input_port_4_r },		/* 2P controls */
	{ 0x2e02, 0x2e02, input_port_3_r },		/* 1P controls */
	{ 0x2e03, 0x2e03, input_port_2_r },		/* coinsw, testsw, startsw */
	{ 0x2e04, 0x2e04, input_port_1_r },		/* DISPW #2 */
	{ 0x4000, 0x7fff, MRA_BANK1 },			/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },			/* ROM 777e02.bin */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress battlnts_writemem[] =
{
	{ 0x0000, 0x1fff, K007342_w },				/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_w },				/* Sprite RAM */
	{ 0x2200, 0x23ff, MWA_RAM },				/* ??? */
	{ 0x2400, 0x24ff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },/* palette */
	{ 0x2600, 0x2607, K007342_vreg_w },			/* Video Registers */
	{ 0x2e08, 0x2e08, battlnts_bankswitch_w },	/* bankswitch control */
	{ 0x2e0c, 0x2e0c, battlnts_spritebank_w },	/* sprite bank select */
	{ 0x2e10, 0x2e10, watchdog_reset_w },		/* watchdog reset */
	{ 0x2e14, 0x2e14, soundlatch_w },			/* sound code # */
	{ 0x2e18, 0x2e18, battlnts_sh_irqtrigger_w },/* cause interrupt on audio CPU */
	{ 0x4000, 0x7fff, MWA_ROM },				/* banked ROM */
	{ 0x8000, 0xffff, MWA_ROM },				/* ROM 777e02.bin */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress battlnts_readmem_sound[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa000, 0xa000, YM3812_status_port_0_r },	/* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_status_port_1_r },	/* YM3812 (chip 2) */
	{ 0xe000, 0xe000, soundlatch_r },			/* soundlatch_r */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress battlnts_writemem_sound[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0xa000, 0xa000, YM3812_control_port_0_w },	/* YM3812 (chip 1) */
	{ 0xa001, 0xa001, YM3812_write_port_0_w },		/* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_control_port_1_w },	/* YM3812 (chip 2) */
	{ 0xc001, 0xc001, YM3812_write_port_1_w },		/* YM3812 (chip 2) */
	{ -1 }	/* end of table */
};

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30k and every 70k" )
	PORT_DIPSETTING(    0x10, "40k and every 80k" )
	PORT_DIPSETTING(    0x08, "40k" )
	PORT_DIPSETTING(    0x00, "50k" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Upright Controls" )
	PORT_DIPSETTING(    0x40, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, "Continue limit" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,			/* 8 x 8 characters */
	0x40000/32,		/* 8192 characters */
	4,				/* 4bpp */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8			/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,			/* 16*16 sprites */
	0x40000/128,	/* 2048 sprites */
	4,				/* 4 bpp */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
	128*8			/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,     0,	1 },	/* colors  0-15 */
	{ 1, 0x040000, &spritelayout,4*16,	1 },	/* colors 64-79 */
	{ -1 } /* end of array */
};

/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM3812interface ym3812_interface =
{
	2,				/* 2 chips */
	3579545,		/* 3.57945 MHz */
	{ 255, 255 },
	{ 0, 0 },
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6309,
			3000000,		/* ? */
			0,
			battlnts_readmem,battlnts_writemem,0,0,
            battlnts_interrupt,1
        },
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,		/* ? */
			2,
			battlnts_readmem_sound, battlnts_writemem_sound,0,0,
			ignore_interrupt,0	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	128, 128,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	battlnts_vh_start,
	battlnts_vh_stop,
	battlnts_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( battlnts_rom )
	ROM_REGION( 0x20000 ) /* code + banked roms */
	ROM_LOAD( "g02.7e",     0x08000, 0x08000, 0xdbd8e17e )	/* fixed ROM */
	ROM_LOAD( "g03.8e",     0x10000, 0x10000, 0x7bd44fef )	/* banked ROM */

	ROM_REGION_DISPOSE( 0x080000 ) /* graphics (disposed after conversion) */
	ROM_LOAD( "777c04.bin",	0x000000, 0x40000, 0x45d92347 )	/* tiles */
	ROM_LOAD( "777c05.bin",	0x040000, 0x40000, 0xaeee778c )	/* sprites */

	ROM_REGION( 0x10000 ) /* 64k for the sound CPU */
	ROM_LOAD( "777c01.bin", 0x00000, 0x08000, 0xc21206e9 )
ROM_END

ROM_START( battlntj_rom )
	ROM_REGION( 0x20000 ) /* code + banked roms */
	ROM_LOAD( "777e02.bin", 0x08000, 0x08000, 0xd631cfcb )	/* fixed ROM */
	ROM_LOAD( "777e03.bin", 0x10000, 0x10000, 0x5ef1f4ef )	/* banked ROM */

	ROM_REGION_DISPOSE( 0x080000 ) /* graphics (disposed after conversion) */
	ROM_LOAD( "777c04.bin",	0x000000, 0x40000, 0x45d92347 )	/* tiles */
	ROM_LOAD( "777c05.bin",	0x040000, 0x40000, 0xaeee778c )	/* sprites */

	ROM_REGION( 0x10000 ) /* 64k for the sound CPU */
	ROM_LOAD( "777c01.bin", 0x00000, 0x08000, 0xc21206e9 )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver battlnts_driver =
{
	__FILE__,
	0,
	"battlnts",
	"Battlantis",
	"1987",
	"Konami",
	"Manuel Abadia",
	0,
	&machine_driver,
	0,

	battlnts_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver battlntj_driver =
{
	__FILE__,
	&battlnts_driver,
	"battlntj",
	"Battlantis (Japan)",
	"1987",
	"Konami",
	"Manuel Abadia",
	0,
	&machine_driver,
	0,

	battlntj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_ROTATE_90,
	0, 0
};
