/***************************************************************************


This driver is dedicated to my loving wife Natalia Wiebelt
                                      and my daughter Lara Anna Maria
Summer 1997 Bernd Wiebelt

Many thanks to Al Kossow for the original sources and the solid documentation.
Without him, I could never had completed this driver.


--------

Most of the info here comes from the wiretap archive at:
http://www.spies.com/arcade/simulation/gameHardware/


Omega Race Memory Map
Version 1.1 (Jul 24,1997)
---------------------

0000 - 3fff	PROM
4000 - 4bff	RAM (3k)
5c00 - 5cff	NVRAM (256 x 4bits)
8000 - 8fff	Vec RAM (4k)
9000 - 9fff	Vec ROM (4k)

15 14 13 12 11 10
--+--+--+--+--+--
0  0  0  0                       M8 - 2732  (4k)
0  0  0  1                       L8 - 2732
0  0  1  0                       K8 - 2732
0  0  1  1                       J8 - 2732

0  1  -  0  0  0                 RAM (3k)
0  1  -  0  0  1
0  1  -  0  1  0

0  1  -  1  1  1                 4 Bit BB RAM (d0-d3)

1  -  -  0  0                    Vec RAM (4k)
1  -  -  0  1
1  -  -  1  0			 Vec ROM (2k) E1
1  -  -  1  1                    Vec ROM (2k) F1

I/O Ports

8	Start/ (VG start)
9	WDOG/  (Reset watchdog)
A	SEQRES/ (VG stop/reset?)
B	RDSTOP/ d7 = stop (VG running if 0)

10 I	DIP SW C4 (game ship settings)

	6 5  4 3  2 1
                      1st bonus ship at
        | |  | |  0 0  40,000
        | |  | |  0 1  50,000
        | |  | |  1 0  70,000
        | |  | |  1 1 100,000
        | |  | |      2nd and  3rd bonus ships
        | |  0 0      150,000   250,000
        | |  0 1      250,000   500,000
        | |  1 0      500,000   750,000
        | |  1 1      750,000 1,500,000
        | |           ships per credit
        0 0           1 credit = 2 ships / 2 credits = 4 ships
        0 1           1 credit = 2 ships / 2 credits = 5 ships
        1 0           1 credit = 3 ships / 2 credits = 6 ships
        1 1           1 credit = 3 ships / 2 credits = 7 ships

11 I	7 = Test
	6 = P1 Fire
	5 = P1 Thrust
	4 = Tilt

	1 = Coin 2
	0 = Coin 1

12 I	7 = 1P1CR
	6 = 1P2CR

	3 = 2P2CR -+
	2 = 2P1CR  |
	1 = P2Fire |
	0 = P2Thr -+ cocktail only

13 O   7 =
        6 = screen reverse
        5 = 2 player 2 credit start LED
        4 = 2 player 1 credit start LED
        3 = 1 player 1 credit start LED
        2 = 1 player 1 credit start LED
        1 = coin meter 2
        0 = coin meter 1

14 O	sound command (interrupts sound Z80)

15 I	encoder 1 (d7-d2)

	The encoder is a 64 position Grey Code encoder, or a
	pot and A to D converter.

	Unlike the quadrature inputs on Atari and Sega games,
        Omega Race's controller is an absolute angle.

	0x00, 0x04, 0x14, 0x10, 0x18, 0x1c, 0x5c, 0x58,
	0x50, 0x54, 0x44, 0x40, 0x48, 0x4c, 0x6c, 0x68,
	0x60, 0x64, 0x74, 0x70, 0x78, 0x7c, 0xfc, 0xf8,
	0xf0, 0xf4, 0xe4, 0xe0, 0xe8, 0xec, 0xcc, 0xc8,
	0xc0, 0xc4, 0xd4, 0xd0, 0xd8, 0xdc, 0x9c, 0x98,
	0x90, 0x94, 0x84, 0x80, 0x88, 0x8c, 0xac, 0xa8,
	0xa0, 0xa4, 0xb4, 0xb0, 0xb8, 0xbc, 0x3c, 0x38,
	0x30, 0x34, 0x24, 0x20, 0x28, 0x2c, 0x0c, 0x08

16 I	encoder 2 (d5-d0)

	The inputs aren't scrambled as they are on the 1 player
        encoder

17 I	DIP SW C6 (coin/cocktail settings)

        8  7  6 5 4  3 2 1
                             coin switch 1
        |  |  | | |  0 0 0   1 coin  2 credits
        |  |  | | |  0 0 1   1 coin  3 credits
        |  |  | | |  0 1 0   1 coin  5 credits
        |  |  | | |  0 1 1   4 coins 5 credits
        |  |  | | |  1 0 0   3 coins 4 credits
        |  |  | | |  1 0 1   2 coins 3 credits
        |  |  | | |  1 1 0   2 coins 1 credit
        |  |  | | |  1 1 1   1 coin  1 credit
        |  |  | | |
        |  |  | | |          coin switch 2
        |  |  0 0 0          1 coin  2 credits
        |  |  0 0 1          1 coin  3 credits
        |  |  0 1 0          1 coin  5 credits
        |  |  0 1 1          4 coins 5 credits
        |  |  1 0 0          3 coins 4 credits
        |  |  1 0 1          2 coins 3 credits
        |  |  1 1 0          2 coins 1 credit
        |  |  1 1 1          1 coin  1 credit
        |  |
        |  0                 coin play
        |  1                 free play
        |
        0                    normal
        1                    cocktail

display list format: (4 byte opcodes)

+------+------+------+------+------+------+------+------+
|DY07   DY06   DY05   DY04   DY03   DY02   DY01   DY00  | 0
+------+------+------+------+------+------+------+------+
|OPCD3  OPCD2  OPCD1  OPCD0  DY11   DY10   DY09   DY08  | 1 OPCD 1111 = ABBREV/
+------+------+------+------+------+------+------+------+
|DX07   DX06   DX05   DX04   DX03   DX02   DX01   DX00  | 2
+------+------+------+------+------+------+------+------+
|INTEN3 INTEN2 INTEN1 INTEN0 DX11   DX10   DX09   DX08  | 3
+------+------+------+------+------+------+------+------+

    Draw relative vector       0x80      1000YYYY YYYYYYYY IIIIXXXX XXXXXXXX

    Draw relative vector
    and load scale             0x90      1001YYYY YYYYYYYY SSSSXXXX XXXXXXXX

    Beam to absolute
    screen position            0xA0      1010YYYY YYYYYYYY ----XXXX XXXXXXXX

    Halt                       0xB0      1011---- --------

    Jump to subroutine         0xC0      1100AAAA AAAAAAAA

    Return from subroutine     0xD0      1101---- --------

    Jump to new address        0xE0      1110AAAA AAAAAAAA

    Short vector draw          0xF0      1111YYYY IIIIXXXX


Sound Z80 Memory Map

0000 ROM
1000 RAM

15 14 13 12 11 10
            0           2k prom (K5)
            1           2k prom (J5)
         1              1k RAM  (K4,J4)

I/O (write-only)

0,1 			8912 (K3)
2,3			8912 (J3)


I/O (read-only)

0                       input port from main CPU.
                        main CPU writing port generated INT
Sound Commands:

0 - reset sound CPU
***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


int omegrace_vg_status_r(int offset);
int omegrace_vg_go(int offset);
int omegrace_vg_start(void);
int omegrace_watchdog_r(int offset);
int omegrace_spinner1_r(int offset);
int omegrace_spinner2_r(int offset);

int omegrace_sh_interrupt(void);
int omegrace_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x5c00, 0x5cff, MRA_RAM }, /* NVRAM */
	{ 0x8000, 0x9fff, MRA_RAM, &vectorram, &vectorram_size },
	{ -1 }	/* end of table */

	/* 9000-9fff is ROM, hopefully there are no writes to it */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x4bff, MWA_RAM },
	{ 0x5c00, 0x5cff, MWA_RAM }, /* NVRAM */
	{ 0x8000, 0x9fff, MWA_RAM, &vectorram },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x1000, 0x13ff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x1000, 0x13ff, MWA_RAM },
	{ -1 } /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x08, 0x08, omegrace_vg_go },
 	{ 0x09, 0x09, omegrace_watchdog_r },
	{ 0x0b, 0x0b, omegrace_vg_status_r }, /* vg_halt */
	{ 0x10, 0x10, input_port_0_r }, /* DIP SW C4 */
	{ 0x17, 0x17, input_port_1_r }, /* DIP SW C6 */
	{ 0x11, 0x11, input_port_2_r }, /* Player 1 input */
	{ 0x12, 0x12, input_port_3_r }, /* Player 2 input */
	{ 0x15, 0x15, omegrace_spinner1_r }, /* 1st controller */
	{ 0x16, 0x16, omegrace_spinner2_r }, /* 2nd, not supported */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
  	{ 0x0a, 0x0a, vg_reset },
 	{ 0x13, 0x13, IOWP_NOP }, /* diverse outputs */
	{ 0x14, 0x14, sound_command_w }, /* Sound command */
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, sound_command_r },
	{ -1 }
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
        { -1 }  /* end of table */
};


static struct InputPort input_ports[] =
{
	{       /* DSW0 - port 0x10 - bonus settings */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW1 - port 0x17 - cocktail + various credits settings */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2 - port 0x11 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, 0, 0, OSD_KEY_F1, OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_F2 },
		{ 0, 0, 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0 }
	},
	{       /* IN3 - port 0x12 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN4 - port 0x15 - spinner emulation */
		0x00,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN5 - port 0x16 - second spinner not emulated */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
	{
		X_AXIS,
		1,
		0.5,
		0
	},
        { -1 }
};

static struct KEYSet keys[] =
{
        { 4, 0, "ROTATE LEFT" },
        { 4, 1, "ROTATE RIGHT"  },
        { 2, 5, "THRUST" },
        { 2, 6, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x03, "FIRST BONUS", { "40000", "50000", "70000", "100000" } },
	{ 0, 0x0c, "BONUS 2/3", { "150000 250000", "250000 500000", "500000 750000", "750000 1500000" } },
	{ 0, 0x30, "CREDITS/SHIPS" , { "1-2/2-4", "1-2/2-5", "1-3/2-6", "1-3/2-7" } },
	{ -1 }
};

static struct GfxLayout fakelayout =
{
        1,1,
        0,
        1,
        { 0 },
        { 0 },
        { 0 },
        0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0,      &fakelayout,     0, 256 },
        { -1 } /* end of array */
};

static unsigned char color_prom[] =
{
        0x00,0x00,0x00, /* BLACK */
        0x00,0x00,0x01, /* BLUE */
        0x00,0x01,0x00, /* GREEN */
        0x00,0x01,0x01, /* CYAN */
        0x01,0x00,0x00, /* RED */
        0x01,0x00,0x01, /* MAGENTA */
        0x01,0x01,0x00, /* YELLOW */
        0x01,0x01,0x01, /* WHITE */
        0x00,0x00,0x00, /* BLACK */
        0x00,0x00,0x01, /* BLUE */
        0x00,0x01,0x00, /* GREEN */
        0x00,0x01,0x02, /* CYAN */
        0x01,0x00,0x00, /* RED */
        0x01,0x00,0x01, /* MAGENTA */
        0x01,0x01,0x00, /* YELLOW */
        0x01,0x01,0x01  /* WHITE */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3000000,	/* 3.0 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,4 	/* approx. 240Hz */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1500000,	/* 1.5 Mhz */
			2, 		/* memory region 1*/
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			omegrace_sh_interrupt,8
		}
	},
	60,
	0,

	/* video hardware */
	288, 224, { 0, 1020, -10, 1010 },
	gfxdecodeinfo,
	256,256,
	atari_vg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	omegrace_vg_start,
	atari_vg_stop,
	atari_vg_screenrefresh,

	/* sound hardware */
	0,
	0,
	omegrace_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( omegrace_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "omega.m7", 0x0000, 0x1000, 0x51948d4e )
	ROM_LOAD( "omega.l7", 0x1000, 0x1000, 0xe1639841 )
	ROM_LOAD( "omega.k7", 0x2000, 0x1000, 0x4ec4afd2 )
	ROM_LOAD( "omega.j7", 0x3000, 0x1000, 0x273fa6b7 )
	ROM_LOAD( "omega.e1", 0x9000, 0x0800, 0x63c42592 )
	ROM_LOAD( "omega.f1", 0x9800, 0x0800, 0xe63e51e2 )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sound.k5", 0x0000, 0x0800, 0x7f858859 )	/* not needed - could be removed */

	ROM_REGION(0x10000)	/* 64k for audio cpu */
	ROM_LOAD( "sound.k5", 0x0000, 0x0800, 0x7f858859 )
ROM_END

static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* no reason to check hiscore table. It's an NV_RAM! */
	/* However, it does not work yet. Don't know why. BW */
	FILE *f;

	if ((f = fopen(name,"rb")) != 0)
	{
		fread(&RAM[0x5c00],1,0x100,f);
		fclose(f);
	}
	return 1;
}

static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x5c00],1,0x100,f);
		fclose(f);
	}
}

struct GameDriver omegrace_driver =
{
	"Omega Race",
	"omegrace",
	"AL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"BERND WIEBELT\n  dedicated to Natalia\n"
	"  and Lara Anna Maria",
	&machine_driver,

	omegrace_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};


