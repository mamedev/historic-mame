/***************************************************************************

Slapshot (c) Taito 1994
--------

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria.)

				*****

Slapshot uses one or two newer Taito custom ics, but the hardware is
very similar to the Taito F2 system, especially F2 games using the same
TC0480SCP tilemap generator (e.g. Metal Black).

This game has 6 separate layers of graphics - four 32x32 tiled scrolling
zoomable background planes of 16x16 tiles, a text plane with 64x64 8x8
character tiles with character definitions held in ram, and a sprite
plane with zoomable 16x16 sprites. This sprite system appears to be
identical to the one used in F2 and F3 games.

Slapshot switches in and out of the double-width tilemap mode of the
TC0480SCP. This is unusual, as most games stick to one width.

The palette generator is 8 bits per color gun like the Taito F3 system.
Like Metal Black the palette space is doubled, and the first half used
for sprites only so the second half can be devoted to tilemaps.

The main cpu is a 68000.

There is a slave Z80 which interfaces with a YM2610B for sound.
Commands are written to it by the 68000 (as in the Taito F2 games).


Slapshot custom ics
-------------------

TC0480SCP (IC61)	- known tilemap chip
TC0640FIO (IC83)	- new version of TC0510NIO io chip?
TC0650FDA (IC84)	- (palette?)
TC0360PRI (IC56)	- (common in pri/color combo on F2 boards)
TC0530SYC (IC58)	- known sound comm chip
TC0520TBC (IC36)	- known object chip
TC0540OBN (IC54)	- known object chip


TODO
====

TC0480SCP problems: in flipscreen the bottom few pixels
of tilemaps are often missing or messed up.

Some hanging notes (try F2 while music is playing).

Sprite colors issue: when you do a super-shot, on the cut
screen the man (it's always the American) should be black.

Col $f8 is used for the man, col $fc for the red/pink
"explosion" under the puck. (Use this to track where they are
in spriteram quickly.) Both of these colors are only set
when the first super-shot happens, so it's clear those
colors are for the super-shot... but screenshot evidence
proves the man should be entirely black.

Extract common sprite stuff from this and taito_f2 ?


Code
----
$854 marks start of service mode

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

void taito_no_buffer_eof_callback(void);
int slapshot_vh_start (void);
void slapshot_vh_stop (void);
void slapshot_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

static data16_t *color_ram;

//static data16_t *slapshot_ram;

extern data16_t *taito_sprite_ext;
extern size_t taito_spriteext_size;


/******************************************************
				COLOR
******************************************************/

static READ16_HANDLER( color_ram_word_r )
{
	return color_ram[offset];
}

static WRITE16_HANDLER( color_ram_word_w )
{
	int r,g,b;
	COMBINE_DATA(&color_ram[offset]);

	if ((offset % 2) == 1)	/* assume words written sequentially */
	{
		r = (color_ram[offset-1] &0xff);
		g = (color_ram[offset] &0xff00) >> 8;
		b = (color_ram[offset] &0xff);

		palette_change_color(offset/2,r,g,b);
	}
}


/**********************************************************
				NVRAM

 (Only alternate bytes are used, we save it all anyway)
**********************************************************/

static data16_t *slapshot_nvram;
static size_t slapshot_nvram_size;

static void slapshot_nvram_handler(void *file,int read_or_write)
{
	if( read_or_write )
	{
		osd_fwrite (file, slapshot_nvram, slapshot_nvram_size);
	}
	else
	{
		if (file)
			osd_fread (file, slapshot_nvram, slapshot_nvram_size);
		else
			memset (slapshot_nvram, 0xff, slapshot_nvram_size);
	}
}


/***********************************************************
				INTERRUPTS
***********************************************************/

void slapshot_interrupt6(int x)
{
	cpu_cause_interrupt(0,6);
}


static int slapshot_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, slapshot_interrupt6);

	return 5;
}


/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( slapshot_input_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_0_word_r(0) << 8;	/* IN0, unknown/unused */

		case 0x01:
			return input_port_1_word_r(0) << 8;	/* IN1 */

		case 0x02:
			return input_port_2_word_r(0) << 8;	/* IN2 */

		case 0x03:
			return input_port_3_word_r(0) << 8;	/* IN3 */

		case 0x07:
			return input_port_4_word_r(0) << 8;	/* IN4 */

	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %02x\n",cpu_get_pc(),offset);
	return 0xff;
}

static READ16_HANDLER( slapshot_service_input_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_0_word_r(0) << 8;	/* IN0, unknown/unused */

		case 0x01:
			return input_port_1_word_r(0) << 8;	/* IN1 */

		case 0x02:
			return input_port_2_word_r(0) << 8;	/* IN2 */

		case 0x03:
			return ((input_port_3_word_r(0) & 0xef) |
				  (input_port_5_word_r(0) & 0x10))  << 8;	/* IN3 + service switch */

		case 0x07:
			return input_port_4_word_r(0) << 8;	/* IN4 */

	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %02x\n",cpu_get_pc(),offset);
	return 0xff;
}

/*****************************************************
				SOUND
*****************************************************/

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}


static WRITE16_HANDLER( slapshot_msb_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0,(data >> 8) & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0,(data >> 8) & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff)
	{
		char buf[80];

		sprintf(buf,"taito_msb_sound_w to low byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

static READ16_HANDLER( slapshot_msb_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff) << 8);
	else return 0;
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( slapshot_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x500000, 0x50ffff, MRA16_RAM },	/* main RAM */
	{ 0x600000, 0x60ffff, MRA16_RAM },	/* sprite ram */
	{ 0x700000, 0x701fff, MRA16_RAM },	/* debugging */
	{ 0x800000, 0x80ffff, TC0480SCP_word_r },	/* tilemaps */
	{ 0x830000, 0x83002f, TC0480SCP_ctrl_word_r },
	{ 0x900000, 0x907fff, color_ram_word_r },	/* 8bpg palette */
	{ 0xa00000, 0xa03fff, MRA16_RAM },	/* nvram (only low bytes used) */
	{ 0xc00000, 0xc0000f, slapshot_input_r },
	{ 0xc00020, 0xc0002f, slapshot_service_input_r },	/* service mirror */
	{ 0xd00000, 0xd00003, slapshot_msb_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( slapshot_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x500000, 0x50ffff, MWA16_RAM },
	{ 0x600000, 0x60ffff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x700000, 0x701fff, MWA16_RAM, &taito_sprite_ext, &taito_spriteext_size },
	{ 0x800000, 0x80ffff, TC0480SCP_word_w },	  /* tilemaps */
	{ 0x830000, 0x83002f, TC0480SCP_ctrl_word_w },
	{ 0x900000, 0x907fff, color_ram_word_w, &color_ram },
	{ 0xa00000, 0xa03fff, MWA16_RAM, &slapshot_nvram, &slapshot_nvram_size },
	{ 0xb00000, 0xb0001f, TC0360PRI_halfword_swap_w },	/* priority chip */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* watchdog ?? */
	{ 0xd00000, 0xd00003, slapshot_msb_sound_w },
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( z80_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS (DIPs in nvram)
***********************************************************/

INPUT_PORTS_START( slapshot )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )	/* bit is service switch at c0002x */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN5, so we can OR in service switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END


/***********************************************************
				GFX DECODING

***********************************************************/

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1, 0, 1, 2, 3 },
	{
	4, 0, 12, 8,
	16+4, 16+0, 16+12, 16+8,
	32+4, 32+0, 32+12, 32+8,
	48+4, 48+0, 48+12, 48+8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout slapshot_charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo slapshot_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tilelayout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &slapshot_charlayout,  0, 256 },	/* sprites & playfield */
	{ -1 } /* end of array */
};



/**************************************************************
			     YM2610B (SOUND)
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static struct MachineDriver machine_driver_slapshot =
{
	{
		{
			CPU_M68000,
			14346000,	/* 28.6860 MHz / 2 ??? */
			slapshot_readmem,slapshot_writemem,0,0,
			slapshot_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_sound_readmem, z80_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	slapshot_gfxdecodeinfo,
	8192, 8192,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	taito_no_buffer_eof_callback,
	slapshot_vh_start,
	slapshot_vh_stop,
	slapshot_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610B,
			&ym2610_interface
		}
	},

	slapshot_nvram_handler
};


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( slapshot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024K for 68000 code */
	ROM_LOAD16_BYTE( "d71-15.3",  0x00000, 0x80000, 0x1470153f )
	ROM_LOAD16_BYTE( "d71-16.1",  0x00001, 0x80000, 0xf13666e0 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD    ( "d71-07.77",    0x00000, 0x4000, 0xdd5f670c )
	ROM_CONTINUE(                 0x10000, 0xc000 )	/* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d71-04.79", 0x00000, 0x80000, 0xb727b81c )	/* SCR */
	ROM_LOAD16_BYTE( "d71-05.80", 0x00001, 0x80000, 0x7b0f5d6d )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d71-01.23", 0x000000, 0x100000, 0x0b1e8c27 )	/* OBJ 6bpp */
	ROM_LOAD16_BYTE( "d71-02.24", 0x000001, 0x100000, 0xccaaea2d )
	ROM_LOAD       ( "d71-03.25", 0x300000, 0x100000, 0xdccef9ec )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "d71-06.37", 0x00000, 0x80000, 0xf3324188 )

	/* no Delta-T samples */

//	Pals (not dumped)
//	ROM_LOAD( "d71-08.40",  0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "d71-09.57",  0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "d71-10.60",  0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "d71-11.42",  0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "d71-12.59",  0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "d71-13.8",   0x00000, 0x00???, 0x00000000 )
ROM_END

static void init_slapshot(void)
{
	unsigned int offset,i;
	UINT8 *gfx = memory_region(REGION_GFX2);
	int size=memory_region_length(REGION_GFX2);
	int data;

	offset = size/2;
	for (i = size/2+size/4; i<size; i++)
	{
		int d1,d2,d3,d4;

		/* Expand 2bits into 4bits format */
		data = gfx[i];
		d1 = (data>>0) & 3;
		d2 = (data>>2) & 3;
		d3 = (data>>4) & 3;
		d4 = (data>>6) & 3;

		gfx[offset] = (d1<<2) | (d2<<6);
		offset++;

		gfx[offset] = (d3<<2) | (d4<<6);
		offset++;
	}
}

GAME( 1994, slapshot, 0, slapshot, slapshot, slapshot, ROT0_16BIT,  "Taito Corporation", "Slap Shot (Japan)" )
