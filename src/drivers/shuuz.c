/***************************************************************************

	Shuuz

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"


void shuuz_playfieldram_w(int offset, int data);

int shuuz_vh_start(void);
void shuuz_vh_stop(void);
void shuuz_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void shuuz_scanline_update(int scanline);


static int v32_state;


/*************************************
 *
 *		Interrupt handling
 *
 *************************************/

static void update_interrupts(int vblank, int sound)
{
	int newstate = 0;

	if (v32_state)
		newstate = 4;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void v32_off(int param)
{
	v32_state = 0;
	atarigen_update_interrupts();
}


static void scanline_update(int scanline)
{
	/* update video */
	shuuz_scanline_update(scanline);

	/* generate 32V signals */
	if (scanline % 64 == 0)
	{
		v32_state = 1;
		atarigen_update_interrupts();

		timer_set(cpu_getscanlineperiod() * (7. / 8.), 0, v32_off);
	}
}



/*************************************
 *
 *		Initialization
 *
 *************************************/

static void init_machine(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_eeprom_reset();

	atarigen_interrupt_init(update_interrupts, scanline_update);
}


static void latch_w(int offset, int data)
{
}



/*************************************
 *
 *		LETA I/O
 *
 *************************************/

static int leta_r(int offset)
{
	/* trackball -- rotated 45 degrees? */
	static int cur[2];
	int which = (offset >> 1) & 1;

	/* when reading the even ports, do a real analog port update */
	if (which == 0)
	{
		int dx = (signed char)input_port_2_r(offset);
		int dy = (signed char)input_port_3_r(offset);

		cur[0] = dx + dy;
		cur[1] = dx - dy;
	}

	/* clip the result to -0x3f to +0x3f to remove directional ambiguities */
	return cur[which];
}



/*************************************
 *
 *		MSM5295 I/O
 *
 *************************************/

static int adpcm_r(int offset)
{
	return OKIM6295_status_0_r(offset) | 0xff00;
}


static void adpcm_w(int offset, int data)
{
	if (!(data & 0x00ff0000))
		OKIM6295_data_0_w(offset, data & 0xff);
}



/*************************************
 *
 *		YM2149 I/O
 *
 *************************************/

static int ym2149_r(int offset)
{
	return (AY8910_read_port_0_r(offset / 2) << 8) | 0x00ff;
}


static void ym2149_w(int offset, int data)
{
	if (!(data & 0xff000000))
	{
		if (offset & 2)
			AY8910_control_port_0_w(offset, (data >> 8) & 0xff);
		else
			AY8910_write_port_0_w(offset, (data >> 8) & 0xff);
	}
}



/*************************************
 *
 *		Additional I/O
 *
 *************************************/

static int special_port0_r(int offset)
{
	int result = input_port_0_r(offset);

	if ((result & 0x0800) && atarigen_get_hblank())
		result &= ~0x0800;

	return result;
}



/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x100fff, atarigen_eeprom_r, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x103000, 0x103003, leta_r },
	{ 0x105000, 0x105001, special_port0_r },
	{ 0x105002, 0x105003, input_port_1_r },
	{ 0x106000, 0x106001, adpcm_r },
	{ 0x107004, 0x107007, ym2149_r },
	{ 0x3e0000, 0x3e087f, MRA_BANK1, &paletteram },
	{ 0x3effc0, 0x3effff, MRA_BANK2 },
	{ 0x3f4000, 0x3f7fff, MRA_BANK3, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0x3f8000, 0x3fcfff, MRA_BANK4 },
	{ 0x3fd000, 0x3fd3ff, MRA_BANK5, &atarigen_spriteram },
	{ 0x3fd400, 0x3fffff, MRA_BANK6 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x100fff, atarigen_eeprom_w },
	{ 0x101000, 0x101fff, atarigen_eeprom_enable_w },
	{ 0x102000, 0x102001, watchdog_reset_w },
	{ 0x105000, 0x105001, latch_w },
	{ 0x106000, 0x106001, adpcm_w },
	{ 0x107004, 0x107007, ym2149_w },
	{ 0x3e0000, 0x3e087f, atarigen_666_paletteram_w },
	{ 0x3effc0, 0x3effff, MWA_BANK2 },
	{ 0x3f4000, 0x3f7fff, shuuz_playfieldram_w },
	{ 0x3f8000, 0x3fcfff, MWA_BANK4 },
	{ 0x3fd000, 0x3fd3ff, MWA_BANK5 },
	{ 0x3fd400, 0x3fffff, MWA_BANK6 },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( shuuz_ports )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x07fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x07fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(    0x0000, DEF_STR( On ))
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER1, 50, 30, 0x7f, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 50, 30, 0x7f, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( shuuz2_ports )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0100, IP_ACTIVE_LOW, 0, "Step Debug SW", OSD_KEY_S, IP_JOY_NONE )
	PORT_BIT( 0x0600, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(  0x1000, IP_ACTIVE_LOW, 0, "Playfield Debug SW", OSD_KEY_Y, IP_JOY_NONE )
	PORT_BITX(  0x2000, IP_ACTIVE_LOW, 0, "Reset Debug SW", OSD_KEY_E, IP_JOY_NONE )
	PORT_BITX(  0x4000, IP_ACTIVE_LOW, 0, "Crosshair Debug SW", OSD_KEY_C, IP_JOY_NONE )
	PORT_BITX(  0x8000, IP_ACTIVE_LOW, 0, "Freeze Debug SW", OSD_KEY_F, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0100, IP_ACTIVE_LOW, 0, "Replay Debug SW", OSD_KEY_R, IP_JOY_NONE )
	PORT_BIT( 0x0600, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(    0x0000, DEF_STR( On ))
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER1, 50, 30, 0x7f, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 50, 30, 0x7f, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,	/* 8*8 sprites */
	16384,	/* 16384 of them */
	4,		/* 4 bits per pixel */
	{ 0, 4, 0+0x40000*8, 4+0x40000*8 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxLayout molayout =
{
	8,8,	/* 8*8 sprites */
	32768,	/* 32768 of them */
	4,		/* 4 bits per pixel */
	{ 0, 4, 0+0x80000*8, 4+0x80000*8 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &pflayout,  256, 16 },		/* sprites & playfield */
	{ 1, 0x080000, &molayout,    0, 16 },		/* sprites & playfield */
	{ -1 } /* end of array */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	7159160 / 1024,		/* ~7000 Hz */
	{ 2 },       		/* memory region 2 */
	{ 100 }
};


static struct AY8910interface ay8910_interface =
{
	1,					/* 1 chip */
	7159160/4,			/* 1.7 MHz */
	{ 100 },
	AY8910_DEFAULT_GAIN,
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			readmem,writemem,0,0,
			atarigen_vblank_gen,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_SUPPORTS_DIRTY,
	0,
	shuuz_vh_start,
	shuuz_vh_stop,
	shuuz_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		},
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/*************************************
 *
 *		ROM decoding
 *
 *************************************/

static void rom_decode(void)
{
	int i;

	for (i = 0; i < Machine->memory_region_length[1]; i++)
		Machine->memory_region[1][i] ^= 0xff;
}



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( shuuz_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "4010.23P",     0x00000, 0x20000, 0x1c2459f8 )
	ROM_LOAD_ODD ( "4011.13P",     0x00000, 0x20000, 0x6db53a85 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2030.43X", 0x000000, 0x20000, 0x8ecf1ed8 )
	ROM_LOAD( "2032.20X", 0x020000, 0x20000, 0x5af184e6 )
	ROM_LOAD( "2031.87X", 0x040000, 0x20000, 0x72e9db63 )
	ROM_LOAD( "2033.65X", 0x060000, 0x20000, 0x8f552498 )

	ROM_LOAD( "1020.43U", 0x080000, 0x20000, 0xd21ad039 )
	ROM_LOAD( "1022.20U", 0x0a0000, 0x20000, 0x0c10bc90 )
	ROM_LOAD( "1024.43M", 0x0c0000, 0x20000, 0xadb09347 )
	ROM_LOAD( "1026.20M", 0x0e0000, 0x20000, 0x9b20e13d )
	ROM_LOAD( "1021.87U", 0x100000, 0x20000, 0x8388910c )
	ROM_LOAD( "1023.65U", 0x120000, 0x20000, 0x71353112 )
	ROM_LOAD( "1025.87M", 0x140000, 0x20000, 0xf7b20a64 )
	ROM_LOAD( "1027.65M", 0x160000, 0x20000, 0x55d54952 )

	ROM_REGION(0x40000)	/* ADPCM data */
	ROM_LOAD( "1040.75B", 0x00000, 0x20000, 0x0896702b )
	ROM_LOAD( "1041.65B", 0x20000, 0x20000, 0xb3b07ce9 )
ROM_END


ROM_START( shuuz2_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "23P.ROM",     0x00000, 0x20000, 0x98aec4e7 )
	ROM_LOAD_ODD ( "13P.ROM",     0x00000, 0x20000, 0xdd9d5d5c )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2030.43X", 0x000000, 0x20000, 0x8ecf1ed8 )
	ROM_LOAD( "2032.20X", 0x020000, 0x20000, 0x5af184e6 )
	ROM_LOAD( "2031.87X", 0x040000, 0x20000, 0x72e9db63 )
	ROM_LOAD( "2033.65X", 0x060000, 0x20000, 0x8f552498 )

	ROM_LOAD( "1020.43U", 0x080000, 0x20000, 0xd21ad039 )
	ROM_LOAD( "1022.20U", 0x0a0000, 0x20000, 0x0c10bc90 )
	ROM_LOAD( "1024.43M", 0x0c0000, 0x20000, 0xadb09347 )
	ROM_LOAD( "1026.20M", 0x0e0000, 0x20000, 0x9b20e13d )
	ROM_LOAD( "1021.87U", 0x100000, 0x20000, 0x8388910c )
	ROM_LOAD( "1023.65U", 0x120000, 0x20000, 0x71353112 )
	ROM_LOAD( "1025.87M", 0x140000, 0x20000, 0xf7b20a64 )
	ROM_LOAD( "1027.65M", 0x160000, 0x20000, 0x55d54952 )

	ROM_REGION(0x40000)	/* ADPCM data */
	ROM_LOAD( "1040.75B", 0x00000, 0x20000, 0x0896702b )
	ROM_LOAD( "1041.65B", 0x20000, 0x20000, 0xb3b07ce9 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver shuuz_driver =
{
	__FILE__,
	0,
	"shuuz",
	"Shuuz (version 8.0)",
	"1990",
	"Atari Games",
	"Aaron Giles (MAME driver)",
	0,
	&machine_driver,
	0,

	shuuz_rom,
	rom_decode,
	0,
	0,
	0,	/* sound_prom */

	shuuz_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver shuuz2_driver =
{
	__FILE__,
	&shuuz_driver,
	"shuuz2",
	"Shuuz (version 7.1)",
	"1990",
	"Atari Games",
	"Aaron Giles (MAME driver)",
	0,
	&machine_driver,
	0,

	shuuz2_rom,
	rom_decode,
	0,
	0,
	0,	/* sound_prom */

	shuuz2_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
