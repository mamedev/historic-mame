/***************************************************************************

Cyberball Memory Map
--------------------

CYBERBALL 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-07FFFF  R    D0-D15

Switch 1 (Player 1)                FC0000         R    D0-D7
Action                                            R    D5
Step (Development Only)                           R    D4
Joystick Left                                     R    D3
Joystick Right                                    R    D2
Joystick Down                                     R    D1
Joystick Up                                       R    D0

Switch 2 (Player 2)                FC2000         R    D0-D7
Action                                            R    D5
Step (Development Only)                           R    D4
Joystick Left                                     R    D3
Joystick Right                                    R    D2
Joystick Down                                     R    D1
Joystick Up                                       R    D0

Self-Test (Active Low)             FC4000         R    D7
Vertical Blank                                    R    D6
Audio Busy Flag (Active Low)                      R    D5

Audio Receive Port                 FC6000         R    D8-D15

EEPROM                             FC8000-FC8FFE  R/W  D0-D7

Color RAM                          FCA000-FCAFFE  R/W  D0-D15

Unlock EEPROM                      FD0000         W    xx
Sound Processor Reset              FD2000         W    xx
Watchdog reset                     FD4000         W    xx
IRQ Acknowledge                    FD6000         W    xx
Audio Send Port                    FD8000         W    D8-D15

Playfield RAM                      FF0000-FF1FFF  R/W  D0-D15
Alpha RAM                          FF2000-FF2FFF  R/W  D0-D15
Motion Object RAM                  FF3000-FF3FFF  R/W  D0-D15
RAM                                FF4000-FFFFFF  R/W

****************************************************************************/


#include "driver.h"
#include "sound/adpcm.h"
#include "machine/atarigen.h"
#include "sndhrdw/ataraud2.h"
#include "vidhrdw/generic.h"


void cyberbal_playfieldram_w(int offset, int data);

int cyberbal_vh_start(void);
void cyberbal_vh_stop(void);
void cyberbal_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void cyberbal_scanline_update(int param);



/*************************************
 *
 *		Initialization
 *
 *************************************/

static void update_interrupts(int vblank, int sound)
{
	int newstate = 0;

	if (vblank)
		newstate |= 1;
	if (sound)
		newstate |= 3;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_eeprom_reset();

	atarigen_interrupt_init(update_interrupts, cyberbal_scanline_update);
	ataraud2_init(1, 3, 2, 0x8000);

	/* speed up the 6502 */
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

static int special_port2_r(int offset)
{
	int temp = input_port_2_r(offset);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x2000;
	return temp;
}


static int sound_state_r(int offset)
{
	int temp = 0xffff;
	if (atarigen_cpu_to_sound_ready) temp ^= 0xffff;
	return temp;
}



/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress main_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0xfc0000, 0xfc0003, input_port_0_r },
	{ 0xfc2000, 0xfc2003, input_port_1_r },
	{ 0xfc4000, 0xfc4003, special_port2_r },
	{ 0xfc6000, 0xfc6003, atarigen_sound_upper_r },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_r, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfca000, 0xfcafff, MRA_BANK1, &paletteram },
	{ 0xfe0000, 0xfe0003, sound_state_r },
	{ 0xff0000, 0xff1fff, MRA_BANK2, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xff2000, 0xff2fff, MRA_BANK3, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xff3000, 0xff3fff, MRA_BANK4, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xff4000, 0xffffff, MRA_BANK5 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress main_writemem[] =
{
	{ 0x000000, 0x08ffff, MWA_ROM },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_w },
	{ 0xfca000, 0xfcafff, atarigen_666_paletteram_w },
	{ 0xfd0000, 0xfd0003, atarigen_eeprom_enable_w },
	{ 0xfd2000, 0xfd2003, atarigen_sound_reset_w },
	{ 0xfd4000, 0xfd4003, watchdog_reset_w },
	{ 0xfd6000, 0xfd6003, atarigen_vblank_ack_w },
	{ 0xfd8000, 0xfd8003, atarigen_sound_upper_w },
	{ 0xff0000, 0xff1fff, cyberbal_playfieldram_w },
	{ 0xff2000, 0xff2fff, MWA_BANK3 },
	{ 0xff3000, 0xff3fff, MWA_BANK4 },
	{ 0xff4000, 0xffffff, MWA_BANK5 },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( cyberb2p_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* DSW */
	PORT_BIT( 0x1fff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BITX(  0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x8000, "Off")
	PORT_DIPSETTING(    0x0000, "On")

	ATARI_AUDIO_2_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	16,8,	/* 8*8 chars */
	8192,	/* 8192 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxLayout anlayout =
{
	16,8,	/* 8*8 chars */
	4096,	/* 4096 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxLayout molayout =
{
	16,8,	/* 8*8 chars */
	20480,	/* 20480 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0xf0000*8+0, 0xf0000*8+4, 0xa0000*8+0, 0xa0000*8+4, 0x50000*8+0, 0x50000*8+4, 0, 4,
	  0xf0000*8+8, 0xf0000*8+12, 0xa0000*8+8, 0xa0000*8+12, 0x50000*8+8, 0x50000*8+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8	/* every char takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 3, 0x140000, &pflayout,     0, 128 },
	{ 3, 0x000000, &molayout, 0x600, 16 },
	{ 3, 0x180000, &anlayout, 0x780, 8 },
	{ -1 } /* end of array */
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
			CPU_M68010,
			7159160,		/* 7.159 Mhz */
			0,
			main_readmem,main_writemem,0,0,
			atarigen_vblank_gen,1
		},
		{
			ATARI_AUDIO_2_CPU(1)
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	init_machine,

	/* video hardware */
	42*16, 30*8, { 0*16, 42*16-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_PIXEL_ASPECT_RATIO_1_2 | VIDEO_SUPPORTS_16BIT,
	0,
	cyberbal_vh_start,
	cyberbal_vh_stop,
	cyberbal_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		ATARI_AUDIO_2_YM2151_MONO,
		ATARI_AUDIO_2_OKIM6295(2)
	}
};



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( cyberb2p_rom )
	ROM_REGION(0x80000)	/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "3019.bin", 0x00000, 0x10000, 0x029f8cb6 )
	ROM_LOAD_ODD ( "3020.bin", 0x00000, 0x10000, 0x1871b344 )
	ROM_LOAD_EVEN( "3021.bin", 0x20000, 0x10000, 0xfd7ebead )
	ROM_LOAD_ODD ( "3022.bin", 0x20000, 0x10000, 0x173ccad4 )
	ROM_LOAD_EVEN( "2023.bin", 0x40000, 0x10000, 0xe541b08f )
	ROM_LOAD_ODD ( "2024.bin", 0x40000, 0x10000, 0x5a77ee95 )
	ROM_LOAD_EVEN( "1025.bin", 0x60000, 0x10000, 0x95ff68c6 )
	ROM_LOAD_ODD ( "1026.bin", 0x60000, 0x10000, 0xf61c4898 )

	ROM_REGION(0x14000)	/* 64k for 6502 code */
	ROM_LOAD( "1042.bin",  0x10000, 0x4000, 0xe63cf125 )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION(0x40000)	/* 256k for ADPCM samples */
	ROM_LOAD( "1049.bin",  0x00000, 0x10000, 0x94f24575 )
	ROM_LOAD( "1050.bin",  0x10000, 0x10000, 0x87208e1e )
	ROM_LOAD( "1051.bin",  0x20000, 0x10000, 0xf82558b9 )
	ROM_LOAD( "1052.bin",  0x30000, 0x10000, 0xd96437ad )

	ROM_REGION_DISPOSE(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1001.bin",  0x000000, 0x20000, 0x586ba107 ) /* MO */
	ROM_LOAD( "1005.bin",  0x020000, 0x20000, 0xa53e6248 ) /* MO */
	ROM_LOAD( "1032.bin",  0x040000, 0x10000, 0x131f52a0 ) /* MO */

	ROM_LOAD( "1002.bin",  0x050000, 0x20000, 0x0f71f86c ) /* MO */
	ROM_LOAD( "1006.bin",  0x070000, 0x20000, 0xdf0ab373 ) /* MO */
	ROM_LOAD( "1033.bin",  0x090000, 0x10000, 0xb6270943 ) /* MO */

	ROM_LOAD( "1003.bin",  0x0a0000, 0x20000, 0x1cf373a2 ) /* MO */
	ROM_LOAD( "1007.bin",  0x0c0000, 0x20000, 0xf2ffab24 ) /* MO */
	ROM_LOAD( "1034.bin",  0x0e0000, 0x10000, 0x6514f0bd ) /* MO */

	ROM_LOAD( "1004.bin",  0x0f0000, 0x20000, 0x537f6de3 ) /* MO */
	ROM_LOAD( "1008.bin",  0x110000, 0x20000, 0x78525bbb ) /* MO */
	ROM_LOAD( "1035.bin",  0x130000, 0x10000, 0x1be3e5c8 ) /* MO */

	ROM_LOAD( "1036.bin",  0x140000, 0x10000, 0xcdf6e3d6 ) /* playfield */
	ROM_LOAD( "1037.bin",  0x150000, 0x10000, 0xec2fef3e ) /* playfield */
	ROM_LOAD( "1038.bin",  0x160000, 0x10000, 0xe866848f ) /* playfield */
	ROM_LOAD( "1039.bin",  0x170000, 0x10000, 0x9b9a393c ) /* playfield */

	ROM_LOAD( "1040.bin",  0x180000, 0x10000, 0xa4c116f9 ) /* alphanumerics */
	ROM_LOAD( "1041.bin",  0x190000, 0x10000, 0xe25d7847 ) /* alphanumerics */
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver cyberb2p_driver =
{
	__FILE__,
	0,
	"cyberb2p",
	"Cyberball 2072 (2 player)",
	"1989",
	"Atari Games",
	"Aaron Giles (MAME driver)\nPatrick Lawrence (Hardware Info)",
	0,
	&machine_driver,
	0,

	cyberb2p_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	cyberb2p_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
