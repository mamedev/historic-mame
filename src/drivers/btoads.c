/*************************************************************************

	BattleToads

	driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "btoads.h"
#include "cpu/tms34010/tms34010.h"



/*************************************
 *
 *	Global variables
 *
 *************************************/

static data16_t *code_rom;
static data16_t *cmos_base;
static data16_t *main_speedup;
static size_t cmos_size;

static UINT8 main_to_sound_data;
static UINT8 main_to_sound_ready;

static UINT8 sound_to_main_data;
static UINT8 sound_to_main_ready;
static UINT8 sound_int_state;



/*************************************
 *
 *	Main -> sound CPU communication
 *
 *************************************/

static void delayed_sound_w(int param)
{
	main_to_sound_data = param;
	main_to_sound_ready = 1;
	cpu_triggerint(1);

	/* use a timer to make long transfers faster */
	timer_set(TIME_IN_USEC(50), 0, 0);
}


static WRITE16_HANDLER( main_sound_w )
{
	if (ACCESSING_LSB)
		timer_set(TIME_NOW, data & 0xff, delayed_sound_w);
}


static READ16_HANDLER( special_port_4_r )
{
	int result = readinputport(4) & ~0x81;

	if (sound_to_main_ready)
		result |= 0x01;
	if (main_to_sound_ready)
		result |= 0x80;
	return result;
}


static READ16_HANDLER( main_sound_r )
{
	sound_to_main_ready = 0;
	return sound_to_main_data;
}



/*************************************
 *
 *	Sound -> main CPU communication
 *
 *************************************/

static WRITE_HANDLER( sound_data_w )
{
	sound_to_main_data = data;
	sound_to_main_ready = 1;
}


static READ_HANDLER( sound_data_r )
{
	main_to_sound_ready = 0;
	return main_to_sound_data;
}


static READ_HANDLER( sound_ready_to_send_r )
{
	return sound_to_main_ready ? 0x00 : 0x80;
}


static READ_HANDLER( sound_data_ready_r )
{
	if (cpu_get_pc() == 0xd50 && !main_to_sound_ready)
		cpu_spinuntil_int();
	return main_to_sound_ready ? 0x00 : 0x80;
}



/*************************************
 *
 *	Sound CPU interrupt generation
 *
 *************************************/

static int sound_interrupt(void)
{
	if (sound_int_state & 0x80)
	{
		cpu_set_irq_line(1, 0, ASSERT_LINE);
		sound_int_state = 0x00;
	}
	return ignore_interrupt();
}


static WRITE_HANDLER( sound_int_state_w )
{
	if (!(sound_int_state & 0x80) && (data & 0x80))
		cpu_set_irq_line(1, 0, CLEAR_LINE);

	sound_int_state = data;
}



/*************************************
 *
 *	Sound CPU BSMT2000 communication
 *
 *************************************/

static READ_HANDLER( bsmt_ready_r )
{
	return 0x80;
}


static WRITE_HANDLER( bsmt2000_port_w )
{
	UINT16 reg = offset >> 8;
	UINT16 val = ((offset & 0xff) << 8) | data;
	BSMT2000_data_0_w(reg, val, 0);
}



/*************************************
 *
 *	Speedup hack
 *
 *************************************/

static READ16_HANDLER( main_speedup_r )
{
	int result = *main_speedup;
	if (cpu_get_pc() == 0xffd51a50 && result == 0)
		cpu_spinuntil_int();
	return result;
}



/*************************************
 *
 *	NVRAM read/write
 *
 *************************************/

static void nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, cmos_base, cmos_size);
	else if (file)
		osd_fread(file, cmos_base, cmos_size);
	else
		memset(cmos_base, 0xff, cmos_size);
}



/*************************************
 *
 *	Main CPU memory map
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), MRA16_RAM },
	{ TOBYTE(0x20000000), TOBYTE(0x2000007f), input_port_0_word_r },
	{ TOBYTE(0x20000080), TOBYTE(0x200000ff), input_port_1_word_r },
	{ TOBYTE(0x20000100), TOBYTE(0x2000017f), input_port_2_word_r },
	{ TOBYTE(0x20000180), TOBYTE(0x200001ff), input_port_3_word_r },
	{ TOBYTE(0x20000200), TOBYTE(0x2000027f), special_port_4_r },
	{ TOBYTE(0x20000280), TOBYTE(0x200002ff), input_port_5_word_r },
	{ TOBYTE(0x20000300), TOBYTE(0x2000037f), input_port_6_word_r },
	{ TOBYTE(0x20000380), TOBYTE(0x200003ff), main_sound_r },
	{ TOBYTE(0x60000000), TOBYTE(0x6003ffff), MRA16_RAM },
	{ TOBYTE(0xa0000000), TOBYTE(0xa03fffff), btoads_vram_fg_display_r },
	{ TOBYTE(0xa4000000), TOBYTE(0xa43fffff), btoads_vram_fg_draw_r },
	{ TOBYTE(0xa8000000), TOBYTE(0xa87fffff), MRA16_RAM },
	{ TOBYTE(0xb0000000), TOBYTE(0xb03fffff), btoads_vram_bg0_r },
	{ TOBYTE(0xb4000000), TOBYTE(0xb43fffff), btoads_vram_bg1_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_r },
	{ TOBYTE(0xfc000000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( main_writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), MWA16_RAM },
	{ TOBYTE(0x20000000), TOBYTE(0x200000ff), MWA16_RAM, &btoads_sprite_scale },
	{ TOBYTE(0x20000100), TOBYTE(0x2000017f), MWA16_RAM, &btoads_sprite_control },
	{ TOBYTE(0x20000180), TOBYTE(0x200001ff), btoads_display_control_w },
	{ TOBYTE(0x20000200), TOBYTE(0x2000027f), btoads_scroll0_w },
	{ TOBYTE(0x20000280), TOBYTE(0x200002ff), btoads_scroll1_w },
	{ TOBYTE(0x20000300), TOBYTE(0x2000037f), btoads_paletteram_w },
	{ TOBYTE(0x20000380), TOBYTE(0x200003ff), main_sound_w },
	{ TOBYTE(0x20000400), TOBYTE(0x2000047f), btoads_misc_control_w },
	{ TOBYTE(0x40000000), TOBYTE(0x4000000f), MWA16_NOP },	/* watchdog? */
	{ TOBYTE(0x60000000), TOBYTE(0x6003ffff), MWA16_RAM, &cmos_base, &cmos_size },
	{ TOBYTE(0xa0000000), TOBYTE(0xa03fffff), btoads_vram_fg_display_w, &btoads_vram_fg0 },
	{ TOBYTE(0xa4000000), TOBYTE(0xa43fffff), btoads_vram_fg_draw_w, &btoads_vram_fg1 },
	{ TOBYTE(0xa8000000), TOBYTE(0xa87fffff), MWA16_RAM, &btoads_vram_fg_data },
	{ TOBYTE(0xa8800000), TOBYTE(0xa8ffffff), MWA16_NOP },
	{ TOBYTE(0xb0000000), TOBYTE(0xb03fffff), btoads_vram_bg0_w, &btoads_vram_bg0 },
	{ TOBYTE(0xb4000000), TOBYTE(0xb43fffff), btoads_vram_bg1_w, &btoads_vram_bg1 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_w },
	{ TOBYTE(0xfc000000), TOBYTE(0xffffffff), MWA16_ROM, &code_rom },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory map
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x8000, 0x8000, sound_data_r },
	{ 0x8004, 0x8004, sound_data_ready_r },
	{ 0x8005, 0x8005, sound_ready_to_send_r },
	{ 0x8006, 0x8006, bsmt_ready_r },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x0000, 0x7fff, bsmt2000_port_w },
	{ 0x8000, 0x8000, sound_data_w },
	{ 0x8002, 0x8002, sound_int_state_w },
MEMORY_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_SERVICE1, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

INPUT_PORTS_START( btoads )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN3, 2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_SERVICE_NO_TOGGLE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffd, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0000, "Stereo")
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0000, "Common Coin Mech")
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Three Players")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Blood Free Mode")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Credit Retention")
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/*************************************
 *
 *	34010 configuration
 *
 *************************************/

static struct tms34010_config cpu_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	btoads_to_shiftreg,				/* write to shiftreg function */
	btoads_from_shiftreg,			/* read from shiftreg function */
	0,								/* display address changed */
	0								/* display interrupt callback */
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ 24000000 },
	{ 12 },
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static struct MachineDriver machine_driver_btoads =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34020,
			40000000/TMS34020_CLOCK_DIVIDER,
			main_readmem,main_writemem,0,0,
			ignore_interrupt,0,
			0,0,&cpu_config
		},
		{
			CPU_Z80 | CPU_16BIT_PORT,
			4000000,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			0,0,
			sound_interrupt,183
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	0,

	/* video hardware */
	512, 256, { 0, 511, 0, 223 },

	0,
	256,0,
	0,

	VIDEO_TYPE_RASTER,
	btoads_vh_eof,
	btoads_vh_start,
	btoads_vh_stop,
	btoads_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_BSMT2000, &bsmt2000_interface }
	},
	nvram_handler
};



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( btoads )
	ROM_REGION( TOBYTE(0x400000), REGION_CPU1, 0 )		/* 34020 dummy region */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound program */
	ROM_LOAD( "btu102.bin", 0x0000, 0x8000, 0xa90b911a )

	ROM_REGION32_LE( 0x800000, REGION_USER1, ROMREGION_DISPOSE )	/* 34020 code */
	ROM_LOAD32_WORD( "btu120.bin",  0x000000, 0x400000, 0x0dfd1e35 )
	ROM_LOAD32_WORD( "btu121.bin",  0x000002, 0x400000, 0xdf7487e1 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* BSMT data */
	ROM_LOAD( "btu109.bin", 0x00000, 0x200000, 0xd9612ddb )
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static void init_btoads(void)
{
	/* set up code ROMs */
	memcpy(code_rom, memory_region(REGION_USER1), memory_region_length(REGION_USER1));

	/* install main CPU speedup */
	main_speedup = install_mem_read16_handler(0, TOBYTE(0x22410), TOBYTE(0x2241f), main_speedup_r);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1994, btoads,   0,         btoads, btoads, btoads, ROT0, "Rare",   "Battle Toads" )
