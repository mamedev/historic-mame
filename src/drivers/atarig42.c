/***************************************************************************

	Atari G42 hardware

	driver by Aaron Giles

	Games supported:
		* Road Riot 4WD (1991)
		* Guardians of the 'Hood (1992)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

int atarig42_vh_start(void);
void atarig42_vh_stop(void);
void atarig42_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void atarig42_scanline_update(int param);

extern UINT8 atarig42_guardian;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 which_input;
static UINT8 pedal_value;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 4;
	if (atarigen_sound_int_state)
		newstate = 5;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarig42_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static int vblank_gen(void)
{
	/* update the pedals once per frame */
	if (readinputport(5) & 1)
	{
		if (pedal_value >= 4)
			pedal_value -= 4;
	}
	else
	{
		if (pedal_value < 0x40 - 4)
			pedal_value += 4;
	}
	return atarigen_video_int_gen();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	temp ^= 0x0008;		/* A2D.EOC always high for now */
	return temp;
}


static WRITE16_HANDLER( a2d_select_w )
{
	which_input = offset;
}


static READ16_HANDLER( a2d_data_r )
{
	/* otherwise, assume it's hydra */
	switch (which_input)
	{
		case 0:
			return readinputport(4) << 8;

		case 1:
			return ~pedal_value << 8;
	}
	return 0;
}


static WRITE16_HANDLER( io_latch_w )
{
	/* lower byte */
	if (ACCESSING_LSB)
	{
		/* bit 4 resets the sound CPU */
		cpu_set_reset_line(1, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		if (!(data & 0x10)) atarijsa_reset();

		/* bit 5 is /XRESET, probably related to the ASIC */

		/* bits 3 and 0 are coin counters */
	}
	logerror("sound control = %04X\n", data);
}



/*************************************
 *
 *	!$#@$ asic
 *
 *************************************/

static UINT16 asic65_command;
static UINT16 asic65_param[32];
static UINT8  asic65_param_index;
static UINT16 asic65_result[32];
static UINT8  asic65_result_index;

static FILE * asic65_log;

static WRITE16_HANDLER( asic65_w )
{
//	if (!asic65_log) asic65_log = fopen("asic65.log", "w");

	/* parameters go to offset 0 */
	if (offset == 0)
	{
		if (asic65_log) fprintf(asic65_log, " W=%04X", data);

		asic65_param[asic65_param_index++] = data;
		if (asic65_param_index >= 32)
			asic65_param_index = 32;
	}

	/* commands go to offset 2 */
	else
	{
		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", cpu_getpreviouspc(), data);

		asic65_command = data;
		asic65_result_index = asic65_param_index = 0;
	}

	/* update results */
	switch (asic65_command)
	{
		case 0x01:	/* reflect data */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x02:	/* compute checksum (should be XX27) */
			asic65_result[0] = 0x0027;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x03:	/* get version (returns 1.3) */
			asic65_result[0] = 0x0013;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x04:	/* internal RAM test (result should be 0) */
			asic65_result[0] = 0;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x10:	/* matrix multiply */
			if (asic65_param_index >= 9+6)
			{
				INT64 element, result;

				element = (INT32)((asic65_param[9] << 16) | asic65_param[10]);
				result = element * (INT16)asic65_param[0] +
						 element * (INT16)asic65_param[1] +
						 element * (INT16)asic65_param[2];
				result >>= 14;
				asic65_result[0] = result >> 16;
				asic65_result[1] = result & 0xffff;

				element = (INT32)((asic65_param[11] << 16) | asic65_param[12]);
				result = element * (INT16)asic65_param[3] +
						 element * (INT16)asic65_param[4] +
						 element * (INT16)asic65_param[5];
				result >>= 14;
				asic65_result[2] = result >> 16;
				asic65_result[3] = result & 0xffff;

				element = (INT32)((asic65_param[13] << 16) | asic65_param[14]);
				result = element * (INT16)asic65_param[6] +
						 element * (INT16)asic65_param[7] +
						 element * (INT16)asic65_param[8];
				result >>= 14;
				asic65_result[4] = result >> 16;
				asic65_result[5] = result & 0xffff;
			}
			break;

		case 0x14:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]) >> 16;
				else
					asic65_result[0] = 0x7fff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x15:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]);
				else
					asic65_result[0] = 0xffff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x17:	/* vector scale */
			if (asic65_param_index >= 2)
			{
				asic65_result[0] = ((INT16)asic65_param[0] * (INT16)asic65_param[1]) >> 12;
				asic65_result_index = 0;
				asic65_param_index = 1;
			}
			break;
	}
}


static READ16_HANDLER( asic65_r )
{
	int result;

	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	if (asic65_log) fprintf(asic65_log, " (R=%04X)", asic65_result[asic65_result_index]);

	/* return the next result */
	result = asic65_result[asic65_result_index++];
	if (asic65_result_index >= 32)
		asic65_result_index = 32;
	return result;
}


static READ16_HANDLER( asic65_io_r )
{
	/* indicate that we always are ready to accept data and always ready to send */
	return 0x4000;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x080001, MRA16_ROM },
	{ 0xe00000, 0xe00001, input_port_0_word_r },
	{ 0xe00002, 0xe00003, input_port_1_word_r },
	{ 0xe00010, 0xe00011, special_port2_r },
	{ 0xe00012, 0xe00013, input_port_3_word_r },
	{ 0xe00020, 0xe00027, a2d_data_r },
	{ 0xe00030, 0xe00031, atarigen_sound_r },
	{ 0xe80000, 0xe80fff, MRA16_RAM },
	{ 0xf40000, 0xf40001, asic65_io_r },
	{ 0xf60000, 0xf60001, asic65_r },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_r },
	{ 0xfc0000, 0xfc0fff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x080001, MWA16_ROM },
	{ 0xe00020, 0xe00027, a2d_select_w },
	{ 0xe00040, 0xe00041, atarigen_sound_w },
	{ 0xe00050, 0xe00051, io_latch_w },
	{ 0xe00060, 0xe00061, atarigen_eeprom_enable_w },
	{ 0xe03000, 0xe03001, atarigen_video_int_ack_w },
	{ 0xe03800, 0xe03801, watchdog_reset16_w },
	{ 0xe80000, 0xe80fff, MWA16_RAM },
	{ 0xf80000, 0xf80003, asic65_w },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfc0000, 0xfc0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xff0000, 0xff0fff, atarirle_0_spriteram_w, &atarirle_0_spriteram },
	{ 0xff1000, 0xff1fff, MWA16_RAM },
	{ 0xff2000, 0xff5fff, ataripf_0_split_w, &ataripf_0_base },
	{ 0xff6000, 0xff6fff, atarian_0_vram_w, &atarian_0_base },
	{ 0xff7000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( roadriot )
	PORT_START		/* e00000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* e00002 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START		/* e00012 */
	PORT_ANALOG ( 0x00ff, 0x0080, IPT_AD_STICK_X, 50, 10, 0, 255 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fake for pedals */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( guardian )
	PORT_START		/* e00000 */
	PORT_BIT( 0x01ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START      /* e00002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x01f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START      /* e00012 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* not used */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,3),
	5,
	{ 0, 0, 1, 2, 3 },
	{ RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+4, 0, 4, RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout pftoplayout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+4, 0, 0, 0, 0 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout, 0x000, 64 },
	{ REGION_GFX2, 0, &anlayout, 0x000, 16 },
	{ REGION_GFX1, 0, &pftoplayout, 0x000, 64 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_atarig42 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			ATARI_CLOCK_14MHz,
			main_readmem,main_writemem,0,0,
			vblank_gen,1
		},
		JSA_III_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	atarig42_vh_start,
	atarig42_vh_stop,
	atarig42_vh_screenrefresh,

	/* sound hardware */
	JSA_III_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( roadriot )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "rriot.8d", 0x00000, 0x20000, 0xbf8aaafc )
	ROM_LOAD16_BYTE( "rriot.8c", 0x00001, 0x20000, 0x5dd2dd70 )
	ROM_LOAD16_BYTE( "rriot.9d", 0x40000, 0x20000, 0x6191653c )
	ROM_LOAD16_BYTE( "rriot.9c", 0x40001, 0x20000, 0x0d34419a )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "rriots.12c", 0x10000, 0x4000, 0x849dd26c )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22d",    0x000000, 0x20000, 0xb7451f92 ) /* playfield, planes 0-1 */
	ROM_LOAD( "rriot.22c",    0x020000, 0x20000, 0x90f3c6ee )
	ROM_LOAD( "rriot20.21d",  0x040000, 0x20000, 0xd40de62b ) /* playfield, planes 2-3 */
	ROM_LOAD( "rriot20.21c",  0x060000, 0x20000, 0xa7e836b1 )
	ROM_LOAD( "rriot.20d",    0x080000, 0x20000, 0xa81ae93f ) /* playfield, planes 4-5 */
	ROM_LOAD( "rriot.20c",    0x0a0000, 0x20000, 0xb8a6d15a )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22j",    0x000000, 0x20000, 0x0005bab0 ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "rriot.2s", 0x000000, 0x20000, 0x19590a94 )
	ROM_LOAD16_BYTE( "rriot.2p", 0x000001, 0x20000, 0xc2bf3f69 )
	ROM_LOAD16_BYTE( "rriot.3s", 0x040000, 0x20000, 0xbab110e4 )
	ROM_LOAD16_BYTE( "rriot.3p", 0x040001, 0x20000, 0x791ad2c5 )
	ROM_LOAD16_BYTE( "rriot.4s", 0x080000, 0x20000, 0x79cba484 )
	ROM_LOAD16_BYTE( "rriot.4p", 0x080001, 0x20000, 0x86a2e257 )
	ROM_LOAD16_BYTE( "rriot.5s", 0x0c0000, 0x20000, 0x67d28478 )
	ROM_LOAD16_BYTE( "rriot.5p", 0x0c0001, 0x20000, 0x74638838 )
	ROM_LOAD16_BYTE( "rriot.6s", 0x100000, 0x20000, 0x24933c37 )
	ROM_LOAD16_BYTE( "rriot.6p", 0x100001, 0x20000, 0x054a163b )
	ROM_LOAD16_BYTE( "rriot.7s", 0x140000, 0x20000, 0x31ff090a )
	ROM_LOAD16_BYTE( "rriot.7p", 0x140001, 0x20000, 0xbbe5b69b )
	ROM_LOAD16_BYTE( "rriot.8s", 0x180000, 0x20000, 0x6c89d2c5 )
	ROM_LOAD16_BYTE( "rriot.8p", 0x180001, 0x20000, 0x40d9bde5 )
	ROM_LOAD16_BYTE( "rriot.9s", 0x1c0000, 0x20000, 0xeca3c595 )
	ROM_LOAD16_BYTE( "rriot.9p", 0x1c0001, 0x20000, 0x88acdb53 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "rriots.19e",  0x80000, 0x20000, 0x2db638a7 )
	ROM_LOAD( "rriots.17e",  0xa0000, 0x20000, 0xe1dd7f9e )
	ROM_LOAD( "rriots.15e",  0xc0000, 0x20000, 0x64d410bb )
	ROM_LOAD( "rriots.12e",  0xe0000, 0x20000, 0xbffd01c8 )
ROM_END

ROM_START( roadriop )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "pgm0h.d8", 0x00000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "pgm0l.c8", 0x00001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "pgm1h.c9", 0x40000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "pgm1h.d9", 0x40001, 0x20000, 0x00000000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "rriots.12c", 0x10000, 0x4000, 0x849dd26c )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spf.0l",    0x000000, 0x10000, 0x00000000 ) /* playfield, planes 0-1 */
	ROM_LOAD( "spf.1l",    0x010000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.2l",    0x020000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.3l",    0x030000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.0m",    0x040000, 0x10000, 0x00000000 ) /* playfield, planes 2-3 */
	ROM_LOAD( "spf.1m",    0x050000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.2m",    0x060000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.3m",    0x070000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.0h",    0x080000, 0x10000, 0x00000000 ) /* playfield, planes 4-5 */
	ROM_LOAD( "spf.1h",    0x090000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.2h",    0x0a0000, 0x10000, 0x00000000 )
	ROM_LOAD( "spf.3h",    0x0b0000, 0x10000, 0x00000000 )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "alpha.22j", 0x000000, 0x20000, 0x00000000 ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "rriot.2s", 0x000000, 0x20000, 0x19590a94 )
	ROM_LOAD16_BYTE( "rriot.2p", 0x000001, 0x20000, 0xc2bf3f69 )
	ROM_LOAD16_BYTE( "rriot.3s", 0x040000, 0x20000, 0xbab110e4 )
	ROM_LOAD16_BYTE( "rriot.3p", 0x040001, 0x20000, 0x791ad2c5 )
	ROM_LOAD16_BYTE( "rriot.4s", 0x080000, 0x20000, 0x79cba484 )
	ROM_LOAD16_BYTE( "rriot.4p", 0x080001, 0x20000, 0x86a2e257 )
	ROM_LOAD16_BYTE( "rriot.5s", 0x0c0000, 0x20000, 0x67d28478 )
	ROM_LOAD16_BYTE( "rriot.5p", 0x0c0001, 0x20000, 0x74638838 )
	ROM_LOAD16_BYTE( "rriot.6s", 0x100000, 0x20000, 0x24933c37 )
	ROM_LOAD16_BYTE( "rriot.6p", 0x100001, 0x20000, 0x054a163b )
	ROM_LOAD16_BYTE( "rriot.7s", 0x140000, 0x20000, 0x31ff090a )
	ROM_LOAD16_BYTE( "rriot.7p", 0x140001, 0x20000, 0xbbe5b69b )
	ROM_LOAD16_BYTE( "rriot.8s", 0x180000, 0x20000, 0x6c89d2c5 )
	ROM_LOAD16_BYTE( "rriot.8p", 0x180001, 0x20000, 0x40d9bde5 )
	ROM_LOAD16_BYTE( "rriot.9s", 0x1c0000, 0x20000, 0xeca3c595 )
	ROM_LOAD16_BYTE( "rriot.9p", 0x1c0001, 0x20000, 0x88acdb53 )
/*	ROM_LOAD16_BYTE( "mo0h.2s", 0x000000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo0l.2p", 0x000001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo1h.3s", 0x040000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo1l.3p", 0x040001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo2h.4s", 0x080000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo2l.4p", 0x080001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo3h.5s", 0x0c0000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo3l.5p", 0x0c0001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo4h.6s", 0x100000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo4l.6p", 0x100001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo5h.7s", 0x140000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo5l.7p", 0x140001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo6h.8s", 0x180000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo6l.8p", 0x180001, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo7h.9s", 0x1c0000, 0x20000, 0x00000000 )
	ROM_LOAD16_BYTE( "mo7l.9p", 0x1c0001, 0x20000, 0x00000000 )*/

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "rriots.19e",  0x80000, 0x20000, 0x2db638a7 )
	ROM_LOAD( "rriots.17e",  0xa0000, 0x20000, 0xe1dd7f9e )
	ROM_LOAD( "rriots.15e",  0xc0000, 0x20000, 0x64d410bb )
	ROM_LOAD( "rriots.12e",  0xe0000, 0x20000, 0xbffd01c8 )
ROM_END

ROM_START( guardian )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "2021.8e",  0x00000, 0x20000, 0xefea1e02 )
	ROM_LOAD16_BYTE( "2020.8cd", 0x00001, 0x20000, 0xa8f655ba )
	ROM_LOAD16_BYTE( "2023.9e",  0x40000, 0x20000, 0xcfa29316 )
	ROM_LOAD16_BYTE( "2022.9cd", 0x40001, 0x20000, 0xed2abc91 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "0080-snd.12c", 0x10000, 0x4000, 0x0388f805 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0037a.23e",  0x000000, 0x80000, 0xca10b63e ) /* playfield, planes 0-1 */
	ROM_LOAD( "0038a.22e",  0x080000, 0x80000, 0xcb1431a1 ) /* playfield, planes 2-3 */
	ROM_LOAD( "0039a.20e",  0x100000, 0x80000, 0x2eee7188 ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0030.23k",   0x000000, 0x20000, 0x0fd7baa1 ) /* alphanumerics */

	ROM_REGION16_BE( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0041.2s",  0x000000, 0x80000, 0xa2a5ae08 )
	ROM_LOAD16_BYTE( "0040.2p",  0x000001, 0x80000, 0xef95132e )
	ROM_LOAD16_BYTE( "0043.3s",  0x100000, 0x80000, 0x6438b8e4 )
	ROM_LOAD16_BYTE( "0042.3p",  0x100001, 0x80000, 0x46bf7c0d )
	ROM_LOAD16_BYTE( "0045.4s",  0x200000, 0x80000, 0x4f4f2bee )
	ROM_LOAD16_BYTE( "0044.4p",  0x200001, 0x80000, 0x20a4250b )
	ROM_LOAD16_BYTE( "0063a.6s", 0x300000, 0x80000, 0x93933bcf )
	ROM_LOAD16_BYTE( "0062a.6p", 0x300001, 0x80000, 0x613e6f1d )
	ROM_LOAD16_BYTE( "0065a.7s", 0x400000, 0x80000, 0x6bcd1205 )
	ROM_LOAD16_BYTE( "0064a.7p", 0x400001, 0x80000, 0x7b4dce05 )
	ROM_LOAD16_BYTE( "0067a.9s", 0x500000, 0x80000, 0x15845fba )
	ROM_LOAD16_BYTE( "0066a.9p", 0x500001, 0x80000, 0x7130c575 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "0010-snd",  0x80000, 0x80000, 0xbca27f40 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_roadriot(void)
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,0x01CC,
		0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,0x01C8,
		0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,0x0162,
		0x013A,0x010A,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,
		0x01CC,0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,
		0x01C8,0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,
		0x0162,0x013A,0x010A,0xE700,0x0164,0x0106,0x0100,0x0104,
		0x01B0,0x0146,0x012E,0x1A00,0x01C8,0x01D0,0x0118,0x0D00,
		0x0118,0x0100,0x01C8,0x01D0,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x4168, 0x4180);

	atarig42_guardian = 0;
}


static void init_guardian(void)
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,0x0104,
		0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,0x0120,
		0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,0x0179,
		0x012D,0x01BD,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,
		0x0104,0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,
		0x0120,0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,
		0x0179,0x012D,0x01BD,0x8C00,0x0118,0x01AB,0x015A,0x0100,
		0x01D0,0x010B,0x01B8,0x01C7,0x01E2,0x0134,0x0100,0x010A,
		0x01BE,0x016D,0x0142,0x0100,0x0120,0x0109,0x0110,0x0141,
		0x0109,0x0100,0x0108,0x0134,0x0105,0x0148,0x1400,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x3168, 0x3180);

	atarig42_guardian = 1;

	/* it looks like they jsr to $80000 as some kind of protection */
	/* put an RTS there so we don't die */
	*(data16_t *)&memory_region(REGION_CPU1)[0x80000] = 0x4E75;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1991, roadriot, 0,        atarig42, roadriot, roadriot, ROT0, "Atari Games", "Road Riot 4WD", GAME_UNEMULATED_PROTECTION )
GAMEX( 1991, roadriop, roadriot, atarig42, roadriot, roadriot, ROT0, "Atari Games", "Road Riot 4WD (prototype)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, guardian, 0,        atarig42, guardian, guardian, ROT0, "Atari Games", "Guardians of the Hood", GAME_UNEMULATED_PROTECTION )
