/*
      Seibu SPI Hardware
	  Seibu SYS386

	  Driver by Ville Linde


	  Games on this hardware:

	      Raiden Fighters
		  Raiden Fighters 2
		  Raiden Fighters Jet
		  Senkyu / Battle Balls
		  E-Jan High School
		  Viper Phase 1

	  Hardware:

	      Intel 386 DX 25MHz / 33MHz
		  Z80 8MHz (sound)
		  YMF271F Sound chip
		  Seibu Custom GFX chip

	  SYS386 seems like a lower-cost version of single-board SPI.
	  It has a 40MHz AMD 386 and a considerably weaker sound system (dual MSM6295).

	  Bugs:

	  Raiden Fighters : Game pauses for a brief second when bosses are encountered
	  Senkyu / BBalls : ---
	  Viper Phase 1   : OLD version (viperp1o) Game locks up / crashes after a few attract loops
	                (386 writes to z80 fifo with data read from unmapped addresses in steps of 0x20000
	                 past the end of z80 ram)
	  E-Jan HS        : ---

*/

#include "driver.h"
#include "cpuintrf.h"
#include "vidhrdw/generic.h"
#include "machine/ds2404.h"
#include "machine/eeprom.h"

void seibuspi_text_decrypt(unsigned char *rom);
void seibuspi_bg_decrypt(unsigned char *rom);
void seibuspi_sprite_decrypt(data16_t* src, int romsize);

VIDEO_START( spi );
VIDEO_UPDATE( spi );
WRITE32_HANDLER( spi_textlayer_w );
WRITE32_HANDLER( spi_back_layer_w );
WRITE32_HANDLER( spi_mid_layer_w );
WRITE32_HANDLER( spi_fore_layer_w );
WRITE32_HANDLER( spi_paletteram32_xBBBBBGGGGGRRRRR_w );
READ32_HANDLER( spi_layer_bank_r );
WRITE32_HANDLER( spi_layer_bank_w );
WRITE32_HANDLER( spi_layer_enable_w );

extern UINT32 *back_ram, *mid_ram, *fore_ram, *scroll_ram;
extern UINT32 *back_rowscroll_ram, *mid_rowscroll_ram, *fore_rowscroll_ram;
extern int old_vidhw;
extern int bg_size;
static data32_t *spimainram;

/********************************************************************/
static int z80_fifo_pos = 0;

static UINT8 z80_semaphore = 0;
static UINT8 z80_com = 0;
static UINT8 i386_semaphore = 1;

static int z80_lastbank;

READ32_HANDLER( sound_com_r )
{
	i386_semaphore = 0x1;
	z80_semaphore = 0x1;
	return z80_com;
}

WRITE32_HANDLER( sound_com_w )
{
	if( (mem_mask & 0xff) == 0 ) {
		z80_com = data & 0xff;
		i386_semaphore = 0;
		z80_semaphore = 0x3;	/* data available */
	}
}

READ32_HANDLER( sound_semaphore_r )
{
	return i386_semaphore;
}

READ32_HANDLER( spi_unknown_r )
{
	return 0xffffffff;
}

WRITE32_HANDLER( ds2404_reset_w )
{
	if( ACCESSING_LSB32 ) {
		DS2404_1W_reset_w(offset, data);
	}
}

READ32_HANDLER( ds2404_data_r )
{
	if( ACCESSING_LSB32 ) {
		return DS2404_data_r(offset);
	}
	return 0;
}

WRITE32_HANDLER( ds2404_data_w )
{
	if( ACCESSING_LSB32 ) {
		DS2404_data_w(offset, data);
	}
}

WRITE32_HANDLER( ds2404_clk_w )
{
	if( ACCESSING_LSB32 ) {
		DS2404_clk_w(offset, data);
	}
}

READ32_HANDLER( eeprom_r )
{
	/* TODO */
	return 0;
}

WRITE32_HANDLER( eeprom_w )
{
	/* TODO */
}

WRITE32_HANDLER( z80_fifo_w )
{
	UINT8* ram = memory_region(REGION_CPU2);
	if( ACCESSING_LSB32 ) {
		if (z80_fifo_pos<0x40000) ram[z80_fifo_pos] = data & 0xff;
		z80_fifo_pos++;
	}
}

WRITE32_HANDLER( z80_enable_w )
{
	z80_fifo_pos = 0;
	if( data & 0x1 ) {
		cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE );
	} else {
		cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE );
	}
}

READ32_HANDLER( spi_controls1_r )
{
	if( ACCESSING_LSB32 ) {
		return (readinputport(1) << 8) | readinputport(0) | 0xffff0000;
	}
	return 0xffffffff;
}

static int control_bit40 = 0;

READ32_HANDLER( spi_controls2_r )
{
	if( ACCESSING_LSB32 ) {
		control_bit40 ^= 0x40;
		return ((readinputport(2) | 0xffffff00) & ~0x40 ) | control_bit40;
	}
	return 0xffffffff;
}

READ32_HANDLER( spi_6295_0_r )
{
	return OKIM6295_status_0_r(0);
}

READ32_HANDLER( spi_6295_1_r )
{
	return OKIM6295_status_1_r(0);
}

WRITE32_HANDLER( spi_6295_0_w )
{
	if( ACCESSING_LSB32 ) {
		OKIM6295_data_0_w(0, data & 0xff);
	}
}

WRITE32_HANDLER( spi_6295_1_w )
{
	if( ACCESSING_LSB32 ) {
		OKIM6295_data_1_w(0, data & 0xff);
	}
}


/********************************************************************/

READ8_HANDLER( z80_semaphore_r )
{
	return z80_semaphore;
}

READ8_HANDLER( z80_com_r )
{
	z80_semaphore = 0x1;
	i386_semaphore = 0x1;
	return z80_com;
}

WRITE8_HANDLER( z80_com_w )
{
	z80_com = data;
	z80_semaphore = 0;
	i386_semaphore = 0x3;	/* data available */
}

static WRITE8_HANDLER( z80_bank_w )
{
	if ((data & 7) != z80_lastbank)
	{
		z80_lastbank = (data & 7);
		cpu_setbank(4, memory_region(REGION_CPU2) + (0x8000 * z80_lastbank));
	}
}

static READ8_HANDLER( z80_unk_r )
{
	return 0xff;
}

/********************************************************************/

static ADDRESS_MAP_START( spi_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000417) AM_READ(MRA32_RAM)			/* Unknown */
	AM_RANGE(0x00000418, 0x0000041b) AM_READ(spi_layer_bank_r)
	AM_RANGE(0x0000041c, 0x0000041f) AM_READ(MRA32_NOP)
	AM_RANGE(0x00000420, 0x0000042b) AM_READ(MRA32_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x0000042c, 0x000005ff) AM_READ(MRA32_RAM)
	AM_RANGE(0x00000600, 0x00000603) AM_READ(spi_unknown_r)		/* Unknown */
	AM_RANGE(0x00000604, 0x00000607) AM_READ(spi_controls1_r)	/* Player controls */
	AM_RANGE(0x00000608, 0x0000060b) AM_READ(spi_unknown_r)		/* Unknown */
	AM_RANGE(0x0000060c, 0x0000060f) AM_READ(spi_controls2_r)	/* Player controls (start) */
	AM_RANGE(0x00000680, 0x00000683) AM_READ(sound_com_r)
	AM_RANGE(0x00000684, 0x00000687) AM_READ(sound_semaphore_r)
	AM_RANGE(0x000006dc, 0x000006df) AM_READ(ds2404_data_r)
	AM_RANGE(0x00000800, 0x0003efff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0003f000, 0x0003ffff) AM_READ(MRA32_RAM)	 	/* Stack */
	AM_RANGE(0x00200000, 0x003fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)		/* ROM location in real-mode */
ADDRESS_MAP_END

static ADDRESS_MAP_START( spi_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000417) AM_WRITE(MWA32_RAM)		/* Unknown */
	AM_RANGE(0x00000418, 0x0000041b) AM_WRITE(spi_layer_bank_w)
	AM_RANGE(0x0000041c, 0x0000041f) AM_WRITE(spi_layer_enable_w)
	AM_RANGE(0x00000420, 0x0000042b) AM_WRITE(MWA32_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x0000042c, 0x000005ff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x00000600, 0x00000603) AM_WRITE(MWA32_NOP)		/* Unknown */
	AM_RANGE(0x00000680, 0x00000683) AM_WRITE(sound_com_w)
	AM_RANGE(0x00000684, 0x00000687) AM_WRITE(MWA32_NOP)		/* Unknown */
	AM_RANGE(0x000006d0, 0x000006d3) AM_WRITE(ds2404_reset_w)
	AM_RANGE(0x000006d4, 0x000006d7) AM_WRITE(ds2404_data_w)
	AM_RANGE(0x000006d8, 0x000006db) AM_WRITE(ds2404_clk_w)
	AM_RANGE(0x00000800, 0x00036fff) AM_WRITE(MWA32_RAM) AM_BASE(&spimainram)
	AM_RANGE(0x00037000, 0x00037fff) AM_WRITE(MWA32_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)	/* Sprites */
	AM_RANGE(0x00038000, 0x0003bfff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x0003c000, 0x0003efff) AM_WRITE(spi_paletteram32_xBBBBBGGGGGRRRRR_w) AM_BASE(&paletteram32)
	AM_RANGE(0x0003f000, 0x0003ffff) AM_WRITE(MWA32_RAM)			/* Stack */
ADDRESS_MAP_END

static ADDRESS_MAP_START( spisound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4008, 0x4008) AM_READ(z80_com_r)			/* Checksum checking */
	AM_RANGE(0x4009, 0x4009) AM_READ(z80_semaphore_r)	/* Z80 read semaphore */
	AM_RANGE(0x400a, 0x400a) AM_READ(z80_unk_r)
	AM_RANGE(0x4013, 0x4013) AM_READNOP
	AM_RANGE(0x6000, 0x600f) AM_READ(YMF271_0_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK4)		/* Banked ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( spisound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4002, 0x4002) AM_WRITE(MWA8_NOP)		/* ack RST 10 */
	AM_RANGE(0x4003, 0x4003) AM_WRITE(MWA8_NOP)		/* Unknown */
	AM_RANGE(0x4008, 0x4008) AM_WRITE(z80_com_w)		/* Checksum checking */
	AM_RANGE(0x400b, 0x400b) AM_WRITE(MWA8_NOP)		/* Unknown */
	AM_RANGE(0x401b, 0x401b) AM_WRITE(z80_bank_w)		/* control register: bits 0-2 = bank @ 8000, bit 3 = watchdog? */
	AM_RANGE(0x6000, 0x600f) AM_WRITE(YMF271_0_w)
ADDRESS_MAP_END

static void irqhandler(int state)
{
	if (state)
	{
		cpunum_set_input_line_and_vector(1, 0, ASSERT_LINE, 0xd7);	// IRQ is RST10
	}
	else
	{
		cpunum_set_input_line(1, 0, CLEAR_LINE);
	}
}

static struct YMF271interface ymf271_interface =
{
	1,
	{ REGION_SOUND1, },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT), },
	{ irqhandler },
};

/********************************************************************/

static ADDRESS_MAP_START( seibu386_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000417) AM_READ(MRA32_RAM)
	AM_RANGE(0x00000418, 0x0000041b) AM_READ(spi_layer_bank_r)
	AM_RANGE(0x0000041c, 0x0000041f) AM_READ(MRA32_NOP)
	AM_RANGE(0x00000420, 0x0000042b) AM_READ(MRA32_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x0000042c, 0x00000603) AM_READ(MRA32_NOP)
	AM_RANGE(0x00000604, 0x00000607) AM_READ(spi_controls1_r)	/* Player controls */
	AM_RANGE(0x00000608, 0x0000060b) AM_READ(eeprom_r)
	AM_RANGE(0x0000060c, 0x0000060f) AM_READ(spi_controls2_r)	/* Player controls (start) */
	AM_RANGE(0x00000800, 0x00036fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x00037000, 0x00037fff) AM_READ(MRA32_RAM)		/* Sprites */
	AM_RANGE(0x00038000, 0x000387ff) AM_READ(MRA32_RAM)		/* Background layer */
	AM_RANGE(0x00038800, 0x00038fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x00039000, 0x000397ff) AM_READ(MRA32_RAM)		/* Foreground layer */
	AM_RANGE(0x00039800, 0x00039fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0003a000, 0x0003a7ff) AM_READ(MRA32_RAM)		/* Middle layer */
	AM_RANGE(0x0003a800, 0x0003afff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0003b000, 0x0003bfff) AM_READ(MRA32_RAM)	 	/* Text layer */
	AM_RANGE(0x0003c000, 0x0003efff) AM_READ(MRA32_RAM)	 	/* Palette */
	AM_RANGE(0x0003f000, 0x0003ffff) AM_READ(MRA32_RAM)	 	/* Stack */
	AM_RANGE(0x00200000, 0x003fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x01200000, 0x01200003) AM_READ(spi_6295_0_r)
	AM_RANGE(0x01200004, 0x01200007) AM_READ(spi_6295_1_r)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)		/* ROM location in real-mode */
ADDRESS_MAP_END

static ADDRESS_MAP_START( seibu386_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000417) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x00000418, 0x0000041b) AM_WRITE(spi_layer_bank_w)
	AM_RANGE(0x0000041c, 0x0000041f) AM_WRITE(spi_layer_enable_w)
	AM_RANGE(0x00000420, 0x0000042b) AM_WRITE(MWA32_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x0000042c, 0x00000603) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x0000068c, 0x0000068f) AM_WRITE(eeprom_w)
	AM_RANGE(0x00000800, 0x00036fff) AM_WRITE(MWA32_RAM) AM_BASE(&spimainram)
	AM_RANGE(0x00037000, 0x00037fff) AM_WRITE(MWA32_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)	/* Sprites */
	AM_RANGE(0x00038000, 0x000387ff) AM_WRITE(spi_back_layer_w) AM_BASE(&back_ram)	/* Background layer */
	AM_RANGE(0x00038800, 0x00038fff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x00039000, 0x000397ff) AM_WRITE(spi_fore_layer_w) AM_BASE(&fore_ram)	/* Foreground layer */
	AM_RANGE(0x00039800, 0x00039fff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x0003a000, 0x0003a7ff) AM_WRITE(spi_mid_layer_w) AM_BASE(&mid_ram)	/* Middle layer */
	AM_RANGE(0x0003a800, 0x0003afff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x0003b000, 0x0003bfff) AM_WRITE(spi_textlayer_w) AM_BASE(&videoram32)	/* Text layer */
	AM_RANGE(0x0003c000, 0x0003efff) AM_WRITE(spi_paletteram32_xBBBBBGGGGGRRRRR_w) AM_BASE(&paletteram32)
	AM_RANGE(0x0003f000, 0x0003ffff) AM_WRITE(MWA32_RAM)			/* Stack */
	AM_RANGE(0x01200000, 0x01200003) AM_WRITE(spi_6295_0_w)
	AM_RANGE(0x01200004, 0x01200007) AM_WRITE(spi_6295_1_w)
ADDRESS_MAP_END

/* OKIM6295 structure(s) */
static struct OKIM6295interface adpcm_6295_interface =
{
	2,          	/* 2 chips */
	{ 1431815/132, 1431815/132 },      /* 10847 Hz frequency (1.431815MHz / 132) */
	{ REGION_SOUND1, REGION_SOUND2 },  /* memory */
	{ 70, 70 }
};

/********************************************************************/

INPUT_PORTS_START( spi_2button )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test Button */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( spi_3button )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test Button */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* E-Jan Highschool has a keyboard with the following keys
   The keys are encoded with 6 bits
   A - 000010 port 0
   B - 010000 port 0
   C - 000010 port 1
   D - 010000 port 1
   E - 000011 port 0
   F - 011000 port 0
   G - 000011 port 1
   H - 011000 port 1
   I - 000100 port 0
   J - 100000 port 0
   K - 000100 port 1
   L - 100000 port 1
   M - 000101 port 0
   N - 101000 port 0
   CHI - 000101 port 1
   PON - 101000 port 1
   KAN - 000110 port 0
   REACH - 110000 port 0
   RON - 000110 port 1
   Start - 000111 port 0
*/

INPUT_PORTS_START( spi_ejanhs )
	PORT_START
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX( 0x03, IP_ACTIVE_LOW, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x18, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX( 0x05, IP_ACTIVE_LOW, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX( 0x28, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX( 0x06, IP_ACTIVE_LOW, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x30, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )

	PORT_START
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 C",     KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX( 0x03, IP_ACTIVE_LOW, 0, "P1 G",     KEYCODE_G,        IP_JOY_NONE )
	PORT_BITX( 0x18, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 K",     KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 L",     KEYCODE_L,        IP_JOY_NONE )
	PORT_BITX( 0x05, IP_ACTIVE_LOW, 0, "P1 Chi",   KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x28, IP_ACTIVE_LOW, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX( 0x06, IP_ACTIVE_LOW, 0, "P1 Ron",   KEYCODE_Z,        IP_JOY_NONE )

	PORT_START
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* Test Button */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/********************************************************************/

static struct GfxLayout spi_charlayout =
{
	8,8,		/* 8*8 characters */
	4096,		/* 4096 characters */
	6,			/* 6 bits per pixel */
	{ 0, 4, 8, 12, 16, 20 },
	{ 0, 1, 2, 3, 24, 25, 26, 27 },
	{ 0*48, 1*48, 2*48, 3*48, 4*48, 5*48, 6*48, 7*48 },
	6*8*8
};

static struct GfxLayout spi_tilelayout =
{
	16,16,
	32768,
	6,
	{ 0, 4, 8, 12, 16, 20 },
	{
		 0, 1, 2, 3,
	    24,25,26,27,
		48,49,50,51,
		72,73,74,75
	},
	{ 0*96, 1*96, 2*96, 3*96, 4*96, 5*96, 6*96, 7*96,
	  8*96, 9*96, 10*96, 11*96, 12*96, 13*96, 14*96, 15*96
	},
	6*16*16
};

static struct GfxLayout spi_spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{ 0,8, RGN_FRAC(1,3)+0,RGN_FRAC(1,3)+8,RGN_FRAC(2,3)+0,RGN_FRAC(2,3)+8  },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

// 1bpp decodes, used for viewing each plane (decryption aid)
#if 0
static struct GfxLayout spi_spritelayout0 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ 0 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

static struct GfxLayout spi_spritelayout1 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ 8 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

static struct GfxLayout spi_spritelayout2 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ RGN_FRAC(1,3)+0 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

static struct GfxLayout spi_spritelayout3 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ RGN_FRAC(1,3)+8 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

static struct GfxLayout spi_spritelayout4 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ RGN_FRAC(2,3)+0 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};

static struct GfxLayout spi_spritelayout5 =
{
	16,16,
	RGN_FRAC(1,3),
	1,
	{ RGN_FRAC(2,3)+8 },
	{
		0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32
	},
	16*32
};
#endif

static struct GfxDecodeInfo spi_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spi_charlayout,   5632, 512 },
	{ REGION_GFX2, 0, &spi_tilelayout,   4096, 1536 },
	{ REGION_GFX3, 0, &spi_spritelayout,   0, 4096 },
#if 0
	{ REGION_GFX3, 0, &spi_spritelayout0,   0, 4096 },
	{ REGION_GFX3, 0, &spi_spritelayout1,   0, 4096 },
	{ REGION_GFX3, 0, &spi_spritelayout2,   0, 4096 },
	{ REGION_GFX3, 0, &spi_spritelayout3,   0, 4096 },
	{ REGION_GFX3, 0, &spi_spritelayout4,   0, 4096 },
	{ REGION_GFX3, 0, &spi_spritelayout5,   0, 4096 },
#endif
	{ -1 } /* end of array */
};

/********************************************************************************/

static NVRAM_HANDLER( spi )
{
	if( read_or_write ) {
		DS2404_save(file);
	} else {
		DS2404_init();

		if(file)
			DS2404_load(file);
	}
}

static NVRAM_HANDLER( sxx2f )
{
	if( read_or_write ) {
		EEPROM_save(file);
	} else {
		EEPROM_init(&eeprom_interface_93C46);

		if(file)
			EEPROM_load(file);
	}
}

static INTERRUPT_GEN( spi_interrupt )
{
	cpunum_set_input_line( 0, 0x20, PULSE_LINE );
}

/* SPI */

static MACHINE_INIT( spi )
{
//	UINT8* rom = memory_region(REGION_USER1);
	UINT8* ram = memory_region(REGION_CPU1);

	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE );

	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00000688, 0x0000068b, 0, 0, z80_fifo_w);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000068c, 0x0000068f, 0, 0, z80_enable_w);

	if( !old_vidhw ) {
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00038000, 0x000387ff, 0, 0, spi_back_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00039000, 0x000397ff, 0, 0, spi_fore_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0003a000, 0x0003a7ff, 0, 0, spi_mid_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0003b000, 0x0003bfff, 0, 0, spi_textlayer_w);
		back_ram = (UINT32*)&ram[0x38000];
		back_rowscroll_ram = (UINT32*)&ram[0x38800];
		fore_ram = (UINT32*)&ram[0x39000];
		fore_rowscroll_ram = (UINT32*)&ram[0x39800];
		mid_ram = (UINT32*)&ram[0x3a000];
		mid_rowscroll_ram = (UINT32*)&ram[0x3a800];
		videoram32 = (UINT32*)&ram[0x3b000];
	} else {
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00038000, 0x000387ff, 0, 0, spi_back_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00038800, 0x00038fff, 0, 0, spi_fore_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00039000, 0x000397ff, 0, 0, spi_mid_layer_w);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00039800, 0x0003a7ff, 0, 0, spi_textlayer_w);
		back_ram = (UINT32*)&ram[0x38000];
		fore_ram = (UINT32*)&ram[0x38800];
		mid_ram = (UINT32*)&ram[0x39000];
		videoram32 = (UINT32*)&ram[0x39800];
	}

	cpu_setbank(4, memory_region(REGION_CPU2));

	// Set region code to 0x10 (USA)
//	rom[0x1ffffc] = 0x10;
}

static MACHINE_DRIVER_START( spi )

	/* basic machine hardware */
	MDRV_CPU_ADD(I386, 50000000/2)	/* Intel 386DX, 25MHz */
	MDRV_CPU_PROGRAM_MAP(spi_readmem,spi_writemem)
	MDRV_CPU_VBLANK_INT(spi_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(spisound_readmem, spisound_writemem)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(200)

	MDRV_MACHINE_INIT(spi)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(spi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(6144)

	MDRV_VIDEO_START(spi)
	MDRV_VIDEO_UPDATE(spi)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF271, ymf271_interface)

MACHINE_DRIVER_END

static MACHINE_INIT( sxx2f )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00000688, 0x0000068b, 0, 0, eeprom_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000068c, 0x0000068f, 0, 0, eeprom_w);
}

static MACHINE_DRIVER_START( sxx2f )

	MDRV_IMPORT_FROM(spi)
	MDRV_MACHINE_INIT(sxx2f)
	MDRV_NVRAM_HANDLER(sxx2f)

MACHINE_DRIVER_END

READ32_HANDLER ( senkyu_speedup_r )
{
 if (activecpu_get_pc()==0x00305bb2) cpu_spinuntil_int(); // idle
 return spimainram[(0x0018cb4-0x800)/4];
}

READ32_HANDLER ( batlball_speedup_r )
{
 if (activecpu_get_pc()==0x003058aa) cpu_spinuntil_int(); // idle
 return spimainram[(0x0018db4-0x800)/4];
}

READ32_HANDLER ( raidnfgt_speedup_r )
{
 if (activecpu_get_pc()==0x0203f0a) cpu_spinuntil_int(); // idle
 return spimainram[(0x00298d0-0x800)/4];
}

READ32_HANDLER ( viperp1_speedup_r )
{
 if (activecpu_get_pc()==0x0202769) cpu_spinuntil_int(); // idle
 return spimainram[(0x001e2e0-0x800)/4];
}

READ32_HANDLER ( viperp1o_speedup_r )
{
 if (activecpu_get_pc()==0x0201f99) cpu_spinuntil_int(); // idle
 return spimainram[(0x001d49c-0x800)/4];
}

/*
// causes input problems?
READ32_HANDLER ( ejanhs_speedup_r )
{
// printf("%08x\n",activecpu_get_pc());
 if (activecpu_get_pc()==0x03032c7) cpu_spinuntil_int(); // idle
 return spimainram[(0x002d224-0x800)/4];
}
*/


static DRIVER_INIT( spi )
{
	seibuspi_text_decrypt(memory_region(REGION_GFX1));
	seibuspi_bg_decrypt(memory_region(REGION_GFX2));
	seibuspi_sprite_decrypt((data16_t*)memory_region(REGION_GFX3), 0x400000);

}

static DRIVER_INIT( raidnfgt )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x6d847] = 0x00;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00298d0, 0x00298d3, 0, 0, raidnfgt_speedup_r );

	init_spi();
	old_vidhw = 0;
	bg_size = 1;
}

static DRIVER_INIT( senkyu )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x13c813] = 0x00;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0018cb4, 0x0018cb7, 0, 0, senkyu_speedup_r );

	init_spi();
	old_vidhw = 1;
	bg_size = 0;
}

static DRIVER_INIT( batlball )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x13c8bf] = 0x00;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0018db4, 0x0018db7, 0, 0, batlball_speedup_r );

	init_spi();
	old_vidhw = 1;
	bg_size = 0;
}

static DRIVER_INIT( ejanhs )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x12f167] = 0x00;
//	idle skip doesn't work properly?
//	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x002d224, 0x002d227, 0, 0, ejanhs_speedup_r );

	init_spi();
	old_vidhw = 1;
	bg_size = 1;
}

static DRIVER_INIT( viperp1 )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x6442f] = 0x00;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x001e2e0, 0x001e2e3, 0, 0, viperp1_speedup_r );

	init_spi();
	old_vidhw = 1;
	bg_size = 1;
}

static DRIVER_INIT( viperp1o )
{
	UINT8* rom = memory_region(REGION_USER1);
	/* sound hack */
	rom[0x60a73] = 0x00;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x001d49c, 0x001d49f, 0, 0, viperp1o_speedup_r );

	init_spi();
	old_vidhw = 1;
	bg_size = 1;
}



/* SYS386 */

READ32_HANDLER ( rf2_2k_speedup_r )
{
 if (activecpu_get_pc()==0x0203926) cpu_spinuntil_int(); // idle
 return spimainram[(0x0282AC-0x800)/4];
}

static DRIVER_INIT( rf2_2k )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0282AC, 0x0282AF, 0, 0, rf2_2k_speedup_r );
}

static MACHINE_INIT( seibu386 )
{

}

static MACHINE_DRIVER_START( seibu386 )

	/* basic machine hardware */
	MDRV_CPU_ADD(I386, 40000000)	/* AMD 386DX, 40MHz */
	MDRV_CPU_PROGRAM_MAP(seibu386_readmem, seibu386_writemem)
	MDRV_CPU_VBLANK_INT(spi_interrupt,1)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(sxx2f)
	MDRV_MACHINE_INIT(seibu386)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(spi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(6144)

	MDRV_VIDEO_START(spi)
	MDRV_VIDEO_UPDATE(spi)

	MDRV_SOUND_ADD(OKIM6295, adpcm_6295_interface)

MACHINE_DRIVER_END

/*******************************************************************/
#define ROM_LOAD24_BYTE(name,offset,length,hash)		ROMX_LOAD(name, offset, length, hash, ROM_SKIP(2))
#define ROM_LOAD24_WORD(name,offset,length,hash)		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(1) | ROM_REVERSE)

/* SPI games */

ROM_START(senkyu)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("fb_1.211", 0x100000, 0x40000, CRC(20a3e5db) SHA1(f1109aeceac7993abc9093d09429718ffc292c77) )
	ROM_LOAD32_BYTE("fb_2.212", 0x100001, 0x40000, CRC(38e90619) SHA1(451ab5f4a5935bb779f9c245c1c4358e80d93c15) )
	ROM_LOAD32_BYTE("fb_3.210", 0x100002, 0x40000, CRC(226f0429) SHA1(69d0fe6671278d7fe215e455bb50abf631cdb484) )
	ROM_LOAD32_BYTE("fb_4.29",  0x100003, 0x40000, CRC(b46d66b7) SHA1(1acd0fea9384e1488b44661e0c99b9672a3f9803) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("fb_6.413", 0x000000, 0x20000, CRC(b57115c9) SHA1(eb95f416f522032ca949bfb6348f1ff824101f2d) )
	ROM_LOAD24_BYTE("fb_5.48",	0x000002, 0x10000, CRC(440a9ae3) SHA1(3f57e6da91f0dac2d816c873759f1e1d3259caf1) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("fb_bg-1d.415", 0x000000, 0x200000, CRC(eae7a1fc) SHA1(26d8a9f4e554848977ec1f6a8aad8751b558a8d4) )
	ROM_LOAD24_BYTE("fb_bg-1p.410", 0x000002, 0x100000, CRC(b46e774e) SHA1(00b6c1d0b0ea37f4354acab543b270c0bf45896d) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("fb_obj-1.322", 0x000000, 0x400000, CRC(29f86f68) SHA1(1afe809ce00a25f8b27543e4188edc3e3e604951) )
	ROM_LOAD("fb_obj-2.324", 0x400000, 0x400000, CRC(c9e3130b) SHA1(12b5d5363142e8efb3b7fc44289c0afffa5011c6) )
	ROM_LOAD("fb_obj-3.323", 0x800000, 0x400000, CRC(f6c3bc49) SHA1(d0eb9c6aa3954d94e3a442a48e0fe6cc279f5513) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("fb_pcm-1.215",  0x000000, 0x100000, CRC(1d83891c) SHA1(09502437562275c14c0f3a0e62b19e91bedb4693) )
	ROM_LOAD("fb_7.216",      0x100000, 0x080000, CRC(874d7b59) SHA1(0236753636c9a818780b23f5f506697b9f6d93c7) )
ROM_END

ROM_START(batlball)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("1.211", 0x100000, 0x40000, CRC(d4e48f89) SHA1(10e43a9ff3f6f169de6352280a8a06e7f482271a) )
	ROM_LOAD32_BYTE("2.212", 0x100001, 0x40000, CRC(3077720b) SHA1(b65c3d02ac75eb56e0c5dc1bf6bb6a4e445a41cf) )
	ROM_LOAD32_BYTE("3.210", 0x100002, 0x40000, CRC(520d31e1) SHA1(998ae968113ab5b2891044187d93793903c13452) )
	ROM_LOAD32_BYTE("4.029", 0x100003, 0x40000, CRC(22419b78) SHA1(67475a654d4ad94e5dfda88cbe2f9c1b5ba6d2cc) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("fb_6.413", 0x000000, 0x20000, CRC(b57115c9) SHA1(eb95f416f522032ca949bfb6348f1ff824101f2d) )
	ROM_LOAD24_BYTE("fb_5.48",	0x000002, 0x10000, CRC(440a9ae3) SHA1(3f57e6da91f0dac2d816c873759f1e1d3259caf1) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("fb_bg-1d.415", 0x000000, 0x200000, CRC(eae7a1fc) SHA1(26d8a9f4e554848977ec1f6a8aad8751b558a8d4) )
	ROM_LOAD24_BYTE("fb_bg-1p.410", 0x000002, 0x100000, CRC(b46e774e) SHA1(00b6c1d0b0ea37f4354acab543b270c0bf45896d) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("fb_obj-1.322", 0x000000, 0x400000, CRC(29f86f68) SHA1(1afe809ce00a25f8b27543e4188edc3e3e604951) )
	ROM_LOAD("fb_obj-2.324", 0x400000, 0x400000, CRC(c9e3130b) SHA1(12b5d5363142e8efb3b7fc44289c0afffa5011c6) )
	ROM_LOAD("fb_obj-3.323", 0x800000, 0x400000, CRC(f6c3bc49) SHA1(d0eb9c6aa3954d94e3a442a48e0fe6cc279f5513) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("fb_pcm-1.215",  0x000000, 0x100000, CRC(1d83891c) SHA1(09502437562275c14c0f3a0e62b19e91bedb4693) )
	ROM_LOAD("fb_7.216",      0x100000, 0x080000, CRC(874d7b59) SHA1(0236753636c9a818780b23f5f506697b9f6d93c7) )
ROM_END

ROM_START(batlbala)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("senkyua1.bin", 0x100000, 0x40000, CRC(ec3c4d4d) SHA1(6c57b8fbb77ce1615850842d06c054e88e240eef) )
	ROM_LOAD32_BYTE("2.212", 0x100001, 0x40000, CRC(3077720b) SHA1(b65c3d02ac75eb56e0c5dc1bf6bb6a4e445a41cf) )
	ROM_LOAD32_BYTE("3.210", 0x100002, 0x40000, CRC(520d31e1) SHA1(998ae968113ab5b2891044187d93793903c13452) )
	ROM_LOAD32_BYTE("4.029", 0x100003, 0x40000, CRC(22419b78) SHA1(67475a654d4ad94e5dfda88cbe2f9c1b5ba6d2cc) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("fb_6.413", 0x000000, 0x20000, CRC(b57115c9) SHA1(eb95f416f522032ca949bfb6348f1ff824101f2d) )
	ROM_LOAD24_BYTE("fb_5.48",	0x000002, 0x10000, CRC(440a9ae3) SHA1(3f57e6da91f0dac2d816c873759f1e1d3259caf1) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("fb_bg-1d.415", 0x000000, 0x200000, CRC(eae7a1fc) SHA1(26d8a9f4e554848977ec1f6a8aad8751b558a8d4) )
	ROM_LOAD24_BYTE("fb_bg-1p.410", 0x000002, 0x100000, CRC(b46e774e) SHA1(00b6c1d0b0ea37f4354acab543b270c0bf45896d) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("fb_obj-1.322", 0x000000, 0x400000, CRC(29f86f68) SHA1(1afe809ce00a25f8b27543e4188edc3e3e604951) )
	ROM_LOAD("fb_obj-2.324", 0x400000, 0x400000, CRC(c9e3130b) SHA1(12b5d5363142e8efb3b7fc44289c0afffa5011c6) )
	ROM_LOAD("fb_obj-3.323", 0x800000, 0x400000, CRC(f6c3bc49) SHA1(d0eb9c6aa3954d94e3a442a48e0fe6cc279f5513) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("fb_pcm-1.215",  0x000000, 0x100000, CRC(1d83891c) SHA1(09502437562275c14c0f3a0e62b19e91bedb4693) )
	ROM_LOAD("fb_7.216",      0x100000, 0x080000, CRC(874d7b59) SHA1(0236753636c9a818780b23f5f506697b9f6d93c7) )
ROM_END

ROM_START(ejanhs)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("ejan3_1.211", 0x100000, 0x40000, CRC(e626d3d2) SHA1(d23cb5e218a85e09de98fa966afbfd43090b396e) )
	ROM_LOAD32_BYTE("ejan3_2.212", 0x100001, 0x40000, CRC(83c39da2) SHA1(9526ffb5d5becccf0aa2e338ab4a3c873d575e6f) )
	ROM_LOAD32_BYTE("ejan3_3.210", 0x100002, 0x40000, CRC(46897b7d) SHA1(a22e0467c016e72bf99df2c1e6ecc792b2151b15) )
	ROM_LOAD32_BYTE("ejan3_4.29",  0x100003, 0x40000, CRC(b3187a2b) SHA1(7fc11ed5ceb2e45f784e75307fef8b850a981a2e) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("ejan3_6.413", 0x000000, 0x20000, CRC(837e012c) SHA1(815452083b65885d6e66dfc058ceec81bb3e6678) )
	ROM_LOAD24_BYTE("ejan3_5.48",  0x000002, 0x10000, CRC(d62db7bf) SHA1(c88f1bb6106c59179b914962ed8cdd4095fd9ce8) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("ej3_bg1d.415", 0x000000, 0x200000, CRC(bcacabe0) SHA1(b73581cf923196326b5b0b99e6aedb915bab0880) )
	ROM_LOAD24_BYTE("ej3_bg1p.410", 0x000002, 0x100000, CRC(1fd0eb5e) SHA1(ca64c8020b246128232f4f6c0a0a2dd9cd3efeae) )
	ROM_LOAD24_WORD("ej3_bg2d.416", 0x300000, 0x100000, CRC(ea2acd69) SHA1(b796e9e4b7342bf452f5ffdbce32cfefc603ba0f) )
	ROM_LOAD24_BYTE("ej3_bg2p.49",  0x300002, 0x080000, CRC(a4a9cb0f) SHA1(da177d13bb95bf6b987d3ca13bcdc86570807b2c) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("ej3_obj1.322", 0x000000, 0x400000, CRC(852f180e) SHA1(d4845dace45c05a68f3b38ccb301c5bf5dce4174) )
	ROM_LOAD("ej3_obj2.324", 0x400000, 0x400000, CRC(1116ad08) SHA1(d5c81383b3f9ede7dd03e6be35487b40740b1f8f) )
	ROM_LOAD("ej3_obj3.323", 0x800000, 0x400000, CRC(ccfe02b6) SHA1(368bc8efe9d6677ba3d0cfc0f450a4bda32988be) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("ej3_pcm1.215",  0x000000, 0x100000, CRC(a92a3a82) SHA1(b86c27c5a2831ddd2a1c2b071018a99afec14018) )
	ROM_LOAD("ejan3_7.216",   0x100000, 0x080000, CRC(c6fc6bcf) SHA1(d4d8c06d295f8eacfa10c21dbab5858f936121f3) )
ROM_END


ROM_START(viperp1)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("v_1-n.211", 0x000000, 0x80000, CRC(55f10b72) SHA1(2a1ebaa969f346bf3659ed8b0f469dce9eaf3b4b) )
	ROM_LOAD32_BYTE("v_2-n.212", 0x000001, 0x80000, CRC(0f888283) SHA1(7e5ac81279b9c7a06f07cb8ae76938cdd5c9beee) )
	ROM_LOAD32_BYTE("v_3-n.210", 0x000002, 0x80000, CRC(842434ac) SHA1(982d219c1d329122789c552208db2f4aaa4af7e4) )
	ROM_LOAD32_BYTE("v_4-n.29",  0x000003, 0x80000, CRC(a3948824) SHA1(fe076951427126c8b7fe81be84ecf0699597225b) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("v_5-n.413", 0x000000, 0x20000, CRC(5ece677c) SHA1(b782cf3296f866f79fafa69ff719211c9d4026df) )
	ROM_LOAD24_BYTE("v_6-n.48",  0x000002, 0x10000, CRC(44844ef8) SHA1(bcbe24d2ffb64f9165ba4ab7de27f44b99b5ff5a) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("v_bg-11.415",  0x000000, 0x200000, CRC(6fc96736) SHA1(12df47d8af2c1febc1bce5bcf3218766447885bd) )
	ROM_LOAD24_BYTE("v_bg-12.415",  0x000002, 0x100000, CRC(d3c7281c) SHA1(340bca1f31486609b3c34dd7830362a216ff648e) )
	ROM_LOAD24_WORD("v_bg-21.410",  0x300000, 0x100000, CRC(d65b4318) SHA1(6522970d95ffa7fa2f32e0b5b4f0eb69e0286b36) )
	ROM_LOAD24_BYTE("v_bg-22.416",  0x300002, 0x080000, CRC(24a0a23a) SHA1(0b0330717620e3f3274a25845d9edaf8023b9db2) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("v_obj-1.322",  0x000000, 0x400000, CRC(3be5b631) SHA1(fd1064428d28ca166a9267b968c0ba846cfed656) )
	ROM_LOAD("v_obj-2.324",  0x400000, 0x400000, CRC(924153b4) SHA1(db5dadcfb4cd5e6efe9d995085936ce4f4eb4254) )
	ROM_LOAD("v_obj-3.323",  0x800000, 0x400000, CRC(e9fb9062) SHA1(18e97b4c5cced2b529e6e72d8041c6f78fcec76e) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("v_pcm.215",  0x000000, 0x100000, CRC(e3111b60) SHA1(f7a7747f29c392876e43efcb4e6c0741454082f2) )
ROM_END

ROM_START(viperp1o)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("v_1-o.211", 0x000000, 0x80000, CRC(4430be64) SHA1(96501a490042c289060d8510f6f79fbf64f79c1a) )
	ROM_LOAD32_BYTE("v_2-o.212", 0x000001, 0x80000, CRC(ffbd88f7) SHA1(cd7f291117dd18bd80fb1130eb87936ff7517ee3) )
	ROM_LOAD32_BYTE("v_3-o.210", 0x000002, 0x80000, CRC(6146db39) SHA1(04e68bfff320a3ffcb47686fa012a038538adc1a) )
	ROM_LOAD32_BYTE("v_4-o.29",  0x000003, 0x80000, CRC(dc8dd2b6) SHA1(20970706240c38c54084b4ae24b7ad23b31aa3de) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_WORD("v_5-o.413", 0x000000, 0x20000, CRC(6d863acc) SHA1(3e3e14f51b9394b24d7cbf562f1cfffc9ec2216d) )
	ROM_LOAD24_BYTE("v_6-o.48",  0x000002, 0x10000, CRC(fe7cb8f7) SHA1(55c7ab977c3666c8770deb62718d535673ffd4f8) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("v_bg-11.415",  0x000000, 0x200000, CRC(6fc96736) SHA1(12df47d8af2c1febc1bce5bcf3218766447885bd) )
	ROM_LOAD24_BYTE("v_bg-12.415",  0x000002, 0x100000, CRC(d3c7281c) SHA1(340bca1f31486609b3c34dd7830362a216ff648e) )
	ROM_LOAD24_WORD("v_bg-21.410",  0x300000, 0x100000, CRC(d65b4318) SHA1(6522970d95ffa7fa2f32e0b5b4f0eb69e0286b36) )
	ROM_LOAD24_BYTE("v_bg-22.416",  0x300002, 0x080000, CRC(24a0a23a) SHA1(0b0330717620e3f3274a25845d9edaf8023b9db2) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("v_obj-1.322",  0x000000, 0x400000, CRC(3be5b631) SHA1(fd1064428d28ca166a9267b968c0ba846cfed656) )
	ROM_LOAD("v_obj-2.324",  0x400000, 0x400000, CRC(924153b4) SHA1(db5dadcfb4cd5e6efe9d995085936ce4f4eb4254) )
	ROM_LOAD("v_obj-3.323",  0x800000, 0x400000, CRC(e9fb9062) SHA1(18e97b4c5cced2b529e6e72d8041c6f78fcec76e) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0) /* samples?*/
	ROM_LOAD("v_pcm.215",  0x000000, 0x100000, CRC(e3111b60) SHA1(f7a7747f29c392876e43efcb4e6c0741454082f2) )
ROM_END

ROM_START(raidnfgt)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("gd_1.211", 0x000000, 0x80000, CRC(f6b2cbdc) SHA1(040c4ff961c8be388c8279b06b777d528c2acc1b) )
	ROM_LOAD32_BYTE("gd_2.212", 0x000001, 0x80000, CRC(1982f812) SHA1(4f12fc3fd7f7a4beda4d29cc81e3a58d255e441f) )
	ROM_LOAD32_BYTE("gd_3.210", 0x000002, 0x80000, CRC(b0f59f44) SHA1(d44fe074ddab35cd0190535cd9fbd7f9e49312a4) )
	ROM_LOAD32_BYTE("gd_4.29",  0x000003, 0x80000, CRC(cd8705bd) SHA1(b19a1486d6b899a134d7b518863ddc8f07967e8b) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)	/* text layer roms */
	ROM_LOAD24_BYTE("gd_5.423", 0x000000, 0x10000, CRC(8f8d4e14) SHA1(06c803975767ae98f40ba7ac5764a5bc8baa3a30) )
	ROM_LOAD24_BYTE("gd_6.424", 0x000001, 0x10000, CRC(6ac64968) SHA1(ec395205c24c4f864a1f805bb0d4641562d4faa9) )
	ROM_LOAD24_BYTE("gd_7.48",	0x000002, 0x10000, CRC(4d87e1ea) SHA1(3230e9b643fad773e61ab8ce09c0cd7d4d0558e3) )

	ROM_REGION( 0x600000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("gd_bg1-d.415", 0x000000, 0x200000, CRC(6a68054c) SHA1(5cbfc4ac90045f1401c2dda7a51936558c9de07e) )
	ROM_LOAD24_BYTE("gd_bg1-p.410", 0x000002, 0x100000, CRC(3400794a) SHA1(719808f7442bac612cefd7b7fffcd665e6337ad0) )
	ROM_LOAD24_WORD("gd_bg2-d.416", 0x300000, 0x200000, CRC(61cd2991) SHA1(bb608e3948bf9ea35b5e1615d2ba6858d029dcbe) )
	ROM_LOAD24_BYTE("gd_bg2-p.49",  0x300002, 0x100000, CRC(502d5799) SHA1(c3a0e1a4f5a7b35572ae1ff31315da4ed08aa2fe) )

	ROM_REGION( 0xc00000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("gd_obj-1.322", 0x000000, 0x400000, CRC(59d86c99) SHA1(d3c9241e7b51fe21f8351051b063f91dc69bf905))
	ROM_LOAD("gd_obj-2.324", 0x400000, 0x400000, CRC(1ceb0b6f) SHA1(97225a9b3e7be18080aa52f6570af2cce8f25c06) )
	ROM_LOAD("gd_obj-3.323", 0x800000, 0x400000, CRC(36e93234) SHA1(51917a80b7da5c32a9434a1076fc2916d62e6a3e) )

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */

	ROM_REGION(0x280000, REGION_SOUND1, 0)	/* YMF271 sound data, music ? */
	ROM_LOAD("gd_pcm.217", 0x000000, 0x200000, CRC(31253ad7) SHA1(c81c8d50f8f287f5cbfaec77b30d969b01ce11a9) )
	ROM_LOAD("gd_8.216",  0x200000, 0x80000, CRC(f88cb6e4) SHA1(fb35b41307b490d5d08e4b8a70f8ff4ce2ca8105) )

ROM_END

/*

file   : readme.txt
author : Tormod
created: 2003-01-20

Raiden Fighter 2 (Single PCB, SXX2F V1.2)

This dump just contains the non surface mounted chips.

Name         Size    CRC32
-------------------------------
fix0.bin    65536    0x6fdf4cf6
fix1.bin    65536    0x69b7899b
fixp.bin    65536    0x99a5fece
prg0.bin   524288    0xff3eeec1
prg1.bin   524288    0xe2cf77d6
prg2.bin   524288    0xcae87e1f
prg3.bin   524288    0x83f4fb5f
sound1.bin 524288    0x20384b0e
zprg.bin   131072    0xcc543c4f

*/

ROM_START(rf2_us)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("prg0u.bin", 0x000000, 0x80000, CRC(ff3eeec1) SHA1(88c1741e4936db9a5b13e562061b0f1cc6fa6b36) )
	ROM_LOAD32_BYTE("prg1u.bin", 0x000001, 0x80000, CRC(e2cf77d6) SHA1(173cc0e304c9dadea4ed0812ebb64c6c83549912) )
	ROM_LOAD32_BYTE("prg2u.bin", 0x000002, 0x80000, CRC(cae87e1f) SHA1(e460aad693eb2702ae11f758b11d37f852d00790) )
	ROM_LOAD32_BYTE("prg3u.bin", 0x000003, 0x80000, CRC(83f4fb5f) SHA1(73f58daa1aae0c4978db409cedd736fb49b15f1c) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_BYTE("fix0.bin", 0x000000, 0x10000, CRC(6fdf4cf6) SHA1(7e9d4a49e829dfdc373c0f5acfbe8c7a91ac115b) )
	ROM_LOAD24_BYTE("fix1.bin", 0x000001, 0x10000, CRC(69b7899b) SHA1(d3cacd4ef4d2c95d803403101beb9d4be75fae61) )
	ROM_LOAD24_BYTE("fixp.bin", 0x000002, 0x10000, CRC(99a5fece) SHA1(44ae95d650ed6e00202d3438f5f91a5e52e319cb) )

	ROM_REGION( 0xc00000, REGION_GFX2, 0)	/* background layer roms */
	/* Not dumped ? */

	ROM_REGION( 0x1200000, REGION_GFX3, 0)	/* sprites */
	/* Not dumped ? */

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */
	ROM_LOAD("zprg.bin", 0x000000, 0x20000, CRC(cc543c4f) SHA1(6e5c93fd3d21c594571b071d4a830211e1f162b2) )

	ROM_REGION(0x280000, REGION_SOUND1, 0)	/* YMF271 sound data */
	ROM_LOAD("sound1.bin", 0x000000, 0x80000, CRC(20384b0e) SHA1(9c5d725418543df740f9145974ed6ffbbabee1d0) )

ROM_END

/* RF2 SPI version */

ROM_START(rf2_eur)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("prg0e.bin", 0x000000, 0x80000, CRC(046b3f0e) SHA1(033898f658d6007f891828835734422d4af36321) )
	ROM_LOAD32_BYTE("prg1e.bin", 0x000001, 0x80000, CRC(cab55d88) SHA1(246e13880d34b6c7c3f4ab5e18fa8a0547c03d9d) )
	ROM_LOAD32_BYTE("prg2e.bin", 0x000002, 0x80000, CRC(83758b0e) SHA1(63adb2d09e7bd7dba47a55b3b579d543dfb553e3) )
	ROM_LOAD32_BYTE("prg3e.bin", 0x000003, 0x80000, CRC(084fb5e4) SHA1(588bfe091662b88f02f528181a2f1d9c67c7b280) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)
	ROM_LOAD24_BYTE("fix0.bin", 0x000000, 0x10000, CRC(6fdf4cf6) SHA1(7e9d4a49e829dfdc373c0f5acfbe8c7a91ac115b) )
	ROM_LOAD24_BYTE("fix1.bin", 0x000001, 0x10000, CRC(69b7899b) SHA1(d3cacd4ef4d2c95d803403101beb9d4be75fae61) )
	ROM_LOAD24_BYTE("fixp.bin", 0x000002, 0x10000, CRC(99a5fece) SHA1(44ae95d650ed6e00202d3438f5f91a5e52e319cb) )

	ROM_REGION( 0xc00000, REGION_GFX2, 0)	/* background layer roms */
	/* Not dumped ? */

	ROM_REGION( 0x1200000, REGION_GFX3, 0)	/* sprites */
	/* Not dumped ? */

	ROM_REGION(0x40000, REGION_CPU2, 0)		/* 256k for the Z80 */
//	ROM_LOAD("zprg.bin", 0x000000, 0x20000, CRC(91238498) ) // not in this set? single board version only

	ROM_REGION(0x280000, REGION_SOUND1, 0)	/* YMF271 sound data */
	ROM_LOAD("rf2_8_sound1.bin", 0x000000, 0x80000, CRC(b7bd3703) SHA1(6427a7e6de10d6743d6e64b984a1d1c647f5643a) ) // different?

ROM_END

/*******************************************************************/
/* SYS386 games */

ROM_START(rf2_2k)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_WORD("prg0-1.267", 0x000000, 0x100000, CRC(0d7d6eb8) SHA1(3a71e1e0ba5bb500dc026debbb6189723c0c2890) )
	ROM_LOAD32_WORD("prg2-3.266", 0x000002, 0x100000, CRC(ead53e69) SHA1(b0e2e06f403317054ecb48d2747034424245f129) )

	ROM_REGION( 0x30000, REGION_GFX1, 0)	/* text layer roms */
	ROM_LOAD24_BYTE("fix0.524", 0x000000, 0x10000, CRC(ed11d043) SHA1(fd3a5a33aa4d795941d64c0d23f9d6f8222843e3) )
	ROM_LOAD24_BYTE("fix1.518", 0x000001, 0x10000, CRC(7036d70a) SHA1(3535b52c0fa1a1158cacc041f8aba2b9a1b43af5) )
	ROM_LOAD24_BYTE("fix2.514", 0x000002, 0x10000, CRC(29b465da) SHA1(644454ab5e0dc1028e9512f85adfe5d8adb757de) )

	ROM_REGION( 0xc00000, REGION_GFX2, 0)	/* background layer roms */
	ROM_LOAD24_WORD("bg-1d.535", 0x000000, 0x400000, CRC(6143f576) SHA1(c034923d0663d9ef24357a03098b8cb81dbab9f8) )
	ROM_LOAD24_BYTE("bg-1p.544", 0x000002, 0x200000, CRC(55e64ef7) SHA1(aae991268948d07342ee8ba1b3761bd180aab8ec) )
	ROM_LOAD24_WORD("bg-2d.536", 0x600000, 0x400000, CRC(c607a444) SHA1(dc1aa96a42e9394ca6036359670a4ec6f830c96d) )
	ROM_LOAD24_BYTE("bg-2p.545", 0x600002, 0x200000, CRC(f0830248) SHA1(6075df96b49e70d2243fef691e096119e7a4d044) )

	ROM_REGION( 0x1200000, REGION_GFX3, 0)	/* sprites */
	ROM_LOAD("obj1.073",  0x0000000, 0x400000, CRC(c2c50f02) SHA1(b81397b5800c6d49f58b7ac7ff6eac56da3c5257) )
	ROM_LOAD("obj4.076",  0x0400000, 0x200000, CRC(5259321f) SHA1(3c70c1147e49f81371d0f60f7108d9718d56faf4) )
	ROM_LOAD("obj2.074",  0x0600000, 0x400000, CRC(7aeadd8e) SHA1(47103c0579240c5b1add4d0b164eaf76f5fa97f0) )
	ROM_LOAD("obj5.077",  0x0a00000, 0x200000, CRC(5d790a5d) SHA1(1ed5d4ad4c9a7e505ce35dcc90d184c26ce891dc) )
	ROM_LOAD("obj3.075",  0x0c00000, 0x400000, CRC(e08f42dc) SHA1(5188d71d4355eaf43ea8893b4cfc4fe80cc24f41) )
	ROM_LOAD("obj6.078",  0x1000000, 0x200000, CRC(1b6a523c) SHA1(99a420dbc8e22e7832ccda7cec9fa661a2a2687a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0)	/* sound data for MSM6295 */
	ROM_LOAD("pcm0.1022", 0x000000, 0x80000, CRC(fd599b35) SHA1(00c0307d1b503bd5ce02d7960ce5e1ad600a7290) )
	ROM_REGION( 0x80000, REGION_SOUND2, 0)	/* sound data for MSM6295 */
	ROM_LOAD("pcm1.1023", 0x000000, 0x80000, CRC(8b716356) SHA1(42ee1896c02518cd1e9cb0dc130321834665a79e) )

ROM_END

/*******************************************************************/

/* SPI */
GAMEX( 1995, senkyu,    0,	     spi, spi_3button, senkyu,		ROT0,	"Seibu Kaihatsu",	"Senkyu (Japan)", GAME_NOT_WORKING )
GAMEX( 1995, batlball,	senkyu,	 spi, spi_3button, batlball,	ROT0,	"Seibu Kaihatsu",	"Battle Balls (Germany, Tuning License)", GAME_NOT_WORKING )
GAMEX( 1995, batlbala,	senkyu,	 spi, spi_3button, batlball,	ROT0,	"Seibu Kaihatsu",	"Battle Balls (Asia, Metrotainment License)", GAME_NOT_WORKING )
GAMEX( 1995, viperp1,	0,	     spi, spi_3button, viperp1,		ROT270,	"Seibu Kaihatsu",	"Viper Phase 1 (New Version)", GAME_NOT_WORKING )
GAMEX( 1995, viperp1o,  viperp1, spi, spi_3button, viperp1o,	ROT270,	"Seibu Kaihatsu",	"Viper Phase 1", GAME_NOT_WORKING )
GAMEX( 1996, ejanhs, 	0,	     spi, spi_ejanhs, ejanhs,		ROT0,	"Seibu Kaihatsu",	"E-Jan High School (JPN)", GAME_NOT_WORKING )
GAMEX( 1996, raidnfgt,	0,	     spi, spi_3button, raidnfgt,	ROT270,	"Seibu Kaihatsu",	"Raiden Fighters", GAME_NOT_WORKING )
GAMEX( 1997, rf2_eur,	0,       spi, spi_2button, spi,			ROT270,	"Seibu Kaihatsu",	"Raiden Fighters 2 (EUR, SPI)", GAME_NOT_WORKING )
/* there is another rf dump rf_spi_asia.zip but it seems strange, 1 program rom, cart pic seems to show others as a different type of rom */

/* SXX2F */
GAMEX( 1997, rf2_us,    rf2_eur,	  sxx2f, spi_2button, 0,	ROT270,	"Seibu Kaihatsu",	"Raiden Fighters 2 (US, Single Board)", GAME_NOT_WORKING )

/* SYS386 */
GAMEX( 2000, rf2_2k,    rf2_eur,	seibu386, spi_2button, rf2_2k,	ROT270,	"Seibu Kaihatsu",	"Raiden Fighters 2 - 2000", GAME_NOT_WORKING )
