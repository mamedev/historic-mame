/***************************************************************************

	Taito F3 Package System (aka F3 Cybercore System)

	Emulation by Bryan McPhail, mish@tendril.co.uk/mish@mame.net
	Thanks to Ian Schmidt and Stileto for sound information!
	Major thanks to Aaron Giles for sound info, figuring out the 68K/ES5505
	rom interface and ES5505 emulator!

	Current suspected 68020 core problems:
		DariusG/Gekirido/Cleopatr:  Garbage written into sprite block control bits
		PBobble3:  Tries to execute code in spriteram and crashes (patched)

	Main Issues:
		Alpha blending not supported (sprite & playfield).
		Some games could fit in 8 bit colour - but marking pens is difficult
			because graphics are 6bpp with 4bpp palette indexes.
		Sound eats lots of memory as 8 bit PCM data is decoded as 16 bit for
			use by the current ES5505 core (which rightly should be 16 bit).
		Only 270 degree rotation is supported in the custom renderer (so you must use
			flipscreen on Gunlock & Gseeker which are really 90 degree rotation games).
		Zoomed layers are not always positioned quite correctly in flipscreen mode

	Other Issues:
		Bats in Arkretrn/Puchicar can fly off one side of the screen and appear
			on the other - analogue input problem.
		Dsp isn't hooked up.
		Crowd/boards not shown in the football games
		Sound doesn't work in RidingF/RingRage/QTheater?
		Input bit to switch between analogue/digital control panels for Arkanoid/
			Puchi Carat is not found.

	Feel free to report any other issues to me.

	Taito custom chips on motherboard:

		TC0630 FDP - Playfield generator?  (Nearest tile roms)
		TC0640 FI0 - I/O & watchdog?
		TC0650 FDA - Priority mixer?  (Near paletteram & video output)
		TC0660 FCM - Sprites? (Nearest sprite roms)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "drivers/taito_f3.h"
#include "state.h"

int  f3_vh_start(void);
void f3_vh_stop(void);
void f3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void f3_eof_callback(void);

extern data32_t *f3_vram,*f3_line_ram;
extern data32_t *f3_pf_data,*f3_pivot_ram;
static data32_t coin_word[2], *f3_ram;
data32_t *f3_shared_ram;
int f3_game;

WRITE32_HANDLER( f3_control_0_w );
WRITE32_HANDLER( f3_control_1_w );
WRITE32_HANDLER( f3_palette_24bit_w );
WRITE32_HANDLER( f3_pf_data_w );
WRITE32_HANDLER( f3_vram_w );
WRITE32_HANDLER( f3_pivot_w );
WRITE32_HANDLER( f3_videoram_w );

/* from Machine.c */
READ16_HANDLER(f3_68000_share_r);
WRITE16_HANDLER(f3_68000_share_w);
READ16_HANDLER(f3_68681_r);
WRITE16_HANDLER(f3_68681_w);
READ16_HANDLER(es5510_dsp_r);
WRITE16_HANDLER(es5510_dsp_w);
WRITE16_HANDLER(f3_volume_w);
WRITE16_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);

/******************************************************************************/

static READ32_HANDLER( f3_control_r )
{
	int e;

	switch (offset)
	{
		case 0x0: /* MSW: Test switch, coins, eeprom access, LSW: Player Buttons, Start, Tilt, Service */
			e=EEPROM_read_bit();
			e=e|(e<<8);
			return ((e | readinputport(2) | (readinputport(2)<<8))<<16) /* top byte may be mirror of bottom byte??  see bubblem */
					| readinputport(1);

		case 0x1: /* MSW: Coin counters/lockouts are readable, LSW: Joysticks (Player 1 & 2) */
			return (coin_word[0]<<16) | readinputport(0) | 0xff00;

		case 0x2: /* Analog control 1 */
			return ((readinputport(3)&0xf)<<12) | ((readinputport(3)&0xf0)>>4);

		case 0x3: /* Analog control 2 */
			return ((readinputport(4)&0xf)<<12) | ((readinputport(4)&0xf0)>>4);

		case 0x4: /* Player 3 & 4 fire buttons (Player 2 top fire buttons in Kaiser Knuckle) */
			return readinputport(5)<<8;

		case 0x5: /* Player 3 & 4 joysticks (Player 1 top fire buttons in Kaiser Knuckle) */
			return (coin_word[1]<<16) | readinputport(6);
	}

	logerror("CPU #0 PC %06x: warning - read unmapped control address %06x\n",cpu_get_pc(),offset);
	return 0xffffffff;
}

static WRITE32_HANDLER( f3_control_w )
{
	switch (offset)
	{
		case 0x00: /* Watchdog */
			watchdog_reset_w(0,0);
			return;
		case 0x01: /* Coin counters & lockouts */
			if (ACCESSING_MSB32) {
				coin_lockout_w(0,~data & 0x01000000);
				coin_lockout_w(1,~data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x08000000);
				coin_word[0]=(data>>16)&0xffff;
			}
			return;
		case 0x04: /* Eeprom */
			if (ACCESSING_LSB32) {
				EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_write_bit(data & 0x04);
				EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
			}
			return;
		case 0x05:	/* Player 3 & 4 coin counters */
			if (ACCESSING_MSB32) {
				coin_lockout_w(2,~data & 0x01000000);
				coin_lockout_w(3,~data & 0x02000000);
				coin_counter_w(2, data & 0x04000000);
				coin_counter_w(3, data & 0x08000000);
				coin_word[1]=(data>>16)&0xffff;
			}
			return;
	}
	logerror("CPU #0 PC %06x: warning - write unmapped control address %06x %08x\n",cpu_get_pc(),offset,data);
}

static WRITE32_HANDLER( f3_sound_reset_0_w )
{
	cpu_set_reset_line(1, CLEAR_LINE);
}

static WRITE32_HANDLER( f3_sound_reset_1_w )
{
	cpu_set_reset_line(1, ASSERT_LINE);
}

/******************************************************************************/

static MEMORY_READ32_START( f3_readmem )
	{ 0x000000, 0x1fffff, MRA32_ROM },
  	{ 0x400000, 0x43ffff, MRA32_RAM },
	{ 0x440000, 0x447fff, MRA32_RAM }, /* Palette ram */
	{ 0x4a0000, 0x4a0017, f3_control_r },
	{ 0x600000, 0x60ffff, MRA32_RAM }, /* Object data */
	{ 0x610000, 0x61bfff, MRA32_RAM }, /* Playfield data */
	{ 0x61c000, 0x61dfff, MRA32_RAM }, /* Text layer */
	{ 0x61e000, 0x61ffff, MRA32_RAM }, /* Vram */
	{ 0x620000, 0x62ffff, MRA32_RAM }, /* Line ram */
	{ 0x630000, 0x63ffff, MRA32_RAM }, /* Pivot ram */
	{ 0xc00000, 0xc007ff, MRA32_RAM }, /* Sound CPU shared ram */
MEMORY_END

static MEMORY_WRITE32_START( f3_writemem )
	{ 0x000000, 0x1fffff, MWA32_RAM },
	{ 0x400000, 0x43ffff, MWA32_RAM, &f3_ram },
	{ 0x440000, 0x447fff, f3_palette_24bit_w, &paletteram32 },
	{ 0x4a0000, 0x4a001f, f3_control_w },
	{ 0x600000, 0x60ffff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x610000, 0x61bfff, f3_pf_data_w, &f3_pf_data },
	{ 0x61c000, 0x61dfff, f3_videoram_w, &videoram32 },
	{ 0x61e000, 0x61ffff, f3_vram_w, &f3_vram },
	{ 0x620000, 0x62ffff, MWA32_RAM, &f3_line_ram },
	{ 0x630000, 0x63ffff, f3_pivot_w, &f3_pivot_ram },
	{ 0x660000, 0x66000f, f3_control_0_w },
	{ 0x660010, 0x66001f, f3_control_1_w },
	{ 0xc00000, 0xc007ff, MWA32_RAM, &f3_shared_ram },
	{ 0xc80000, 0xc80003, f3_sound_reset_0_w },
	{ 0xc80100, 0xc80103, f3_sound_reset_1_w },
MEMORY_END

/******************************************************************************/

static MEMORY_READ16_START( sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_r },
	{ 0x200000, 0x20001f, ES5505_data_0_r },
	{ 0x260000, 0x2601ff, es5510_dsp_r },
	{ 0x280000, 0x28001f, f3_68681_r },
	{ 0xc00000, 0xcfffff, MRA16_BANK1 },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_w },
	{ 0x200000, 0x20001f, ES5505_data_0_w },
	{ 0x260000, 0x2601ff, es5510_dsp_w },
	{ 0x280000, 0x28001f, f3_68681_w },
	{ 0x300000, 0x30003f, f3_es5505_bank_w },
	{ 0x340000, 0x340003, f3_volume_w }, /* 8 channel volume control */
	{ 0xc00000, 0xcfffff, MWA16_ROM },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( f3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 ) /* Only on some games */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* Eprom data bit */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Another service mode */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 0, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 50, 0, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
INPUT_PORTS_END

INPUT_PORTS_START( kn )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* Eprom data bit */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Another service mode */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START
	PORT_ANALOGX( 0xffff, 0x00, IPT_DIAL | IPF_PLAYER1, 30, 15, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START
	PORT_ANALOGX( 0xffff, 0x00, IPT_DIAL | IPF_PLAYER2, 30, 15, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	256,
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
    { 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4, 0*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout pivotlayout =
{
	8,8,
	2048,
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
    { 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4, 0*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spriteram_layout =
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

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{ RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+3, 0, 1, 2, 3 },
	{
	4, 0, 16+4, 16+0,
    8+4, 8+0, 24+4, 24+0,
	32+4, 32+0, 48+4, 48+0,
    40+4, 40+0, 56+4, 56+0,
   	},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0,           0x000000, &charlayout,          0,  64 }, /* Dynamically modified */
	{ REGION_GFX2, 0x000000, &tile_layout, 	       0, 512 }, /* Tiles area */
	{ REGION_GFX1, 0x000000, &spriteram_layout, 4096, 256 }, /* Sprites area */
	{ 0,           0x000000, &pivotlayout,         0, 256 }, /* Dynamically modified */
	{ -1 } /* end of array */
};

/******************************************************************************/

static int f3_interrupt(void)
{
	if (cpu_getiloops()) return 3;
	return 2;
}

static void f3_machine_reset(void)
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1,&RAM[0x80000]);

	RAM[0]=RAM[0x80000]; /* Stack and Reset vectors */
	RAM[1]=RAM[0x80001];
	RAM[2]=RAM[0x80002];
	RAM[3]=RAM[0x80003];

	cpu_set_reset_line(1, ASSERT_LINE);
	f3_68681_reset();
}

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ 16000000 },		/* freq */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
};

static struct EEPROM_interface f3_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&f3_eeprom_interface);
		if (file)
			EEPROM_load(file);
	}
}

static struct MachineDriver machine_driver_f3 =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68EC020,
			16000000,
			f3_readmem,f3_writemem,0,0,
			f3_interrupt,2
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			16000000,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, 624, /* 58.97 Hz, 624us vblank time */
	1,
	f3_machine_reset,

 	/* video hardware */
	40*8+48*2, 32*8, { 46, 40*8-1+46, 3*8, 32*8-1 },
//	40*8+48*2, 64*8, { 46, 64*8-1+46, 3*8, 64*8-1 },

	gfxdecodeinfo,
	8192, 8192,
	0,

#if TRY_ALPHA
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT,
#else
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN,
#endif

	f3_eof_callback,
	f3_vh_start,
	f3_vh_stop,
	f3_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_ES5505,
			&es5505_interface
		}
	},

	nvram_handler
};

/******************************************************************************/

ROM_START( ringrage )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d21-23.40", 0x000000, 0x40000, 0x14e9ed65 )
	ROM_LOAD32_BYTE("d21-22.38", 0x000001, 0x40000, 0x6f8b65b0 )
	ROM_LOAD32_BYTE("d21-21.36", 0x000002, 0x40000, 0xbf7234bc )
	ROM_LOAD32_BYTE("ringr-25.rom",0x000003, 0x40000, 0xaeff6e19 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("d21-02.66", 0x000000, 0x200000, 0xfacd3a02 )
	ROM_LOAD16_BYTE("d21-03.67", 0x000001, 0x200000, 0x6f653e68 )
	ROM_LOAD       ("d21-04.68", 0x600000, 0x200000, 0x9dcdceca )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("d21-06.49", 0x000000, 0x080000, 0x92d4a720 )
  	ROM_LOAD16_BYTE("d21-07.50", 0x000001, 0x080000, 0x6da696e9 )
	ROM_LOAD       ("d21-08.51", 0x180000, 0x080000, 0xa0d95be9 )
	ROM_FILL       (             0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d21-18.5", 0x100000, 0x20000, 0x133b55d0 )
	ROM_LOAD16_BYTE("d21-19.6", 0x100001, 0x20000, 0x1f98908f )

	ROM_REGION16_BE(0x800000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d21-01.17", 0x000000, 0x200000, 0x1fb6f07d )
	ROM_LOAD16_BYTE("d21-05.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( ringragu )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d21-23.40", 0x000000, 0x40000, 0x14e9ed65 )
	ROM_LOAD32_BYTE("d21-22.38", 0x000001, 0x40000, 0x6f8b65b0 )
	ROM_LOAD32_BYTE("d21-21.36", 0x000002, 0x40000, 0xbf7234bc )
	ROM_LOAD32_BYTE("d21-24.bin",0x000003, 0x40000, 0x404dee67 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("d21-02.66", 0x000000, 0x200000, 0xfacd3a02 )
	ROM_LOAD16_BYTE("d21-03.67", 0x000001, 0x200000, 0x6f653e68 )
	ROM_LOAD       ("d21-04.68", 0x600000, 0x200000, 0x9dcdceca )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("d21-06.49", 0x000000, 0x080000, 0x92d4a720 )
  	ROM_LOAD16_BYTE("d21-07.50", 0x000001, 0x080000, 0x6da696e9 )
	ROM_LOAD       ("d21-08.51", 0x180000, 0x080000, 0xa0d95be9 )
	ROM_FILL       (             0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d21-18.5", 0x100000, 0x20000, 0x133b55d0 )
	ROM_LOAD16_BYTE("d21-19.6", 0x100001, 0x20000, 0x1f98908f )

	ROM_REGION16_BE(0x800000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d21-01.17", 0x000000, 0x200000, 0x1fb6f07d )
	ROM_LOAD16_BYTE("d21-05.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( ringragj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d21-23.40", 0x000000, 0x40000, 0x14e9ed65 )
	ROM_LOAD32_BYTE("d21-22.38", 0x000001, 0x40000, 0x6f8b65b0 )
	ROM_LOAD32_BYTE("d21-21.36", 0x000002, 0x40000, 0xbf7234bc )
	ROM_LOAD32_BYTE("d21-20.34", 0x000003, 0x40000, 0xa8eb68a4 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("d21-02.66", 0x000000, 0x200000, 0xfacd3a02 )
	ROM_LOAD16_BYTE("d21-03.67", 0x000001, 0x200000, 0x6f653e68 )
	ROM_LOAD       ("d21-04.68", 0x600000, 0x200000, 0x9dcdceca )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("d21-06.49", 0x000000, 0x080000, 0x92d4a720 )
  	ROM_LOAD16_BYTE("d21-07.50", 0x000001, 0x080000, 0x6da696e9 )
	ROM_LOAD       ("d21-08.51", 0x180000, 0x080000, 0xa0d95be9 )
	ROM_FILL       (             0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d21-18.5", 0x100000, 0x20000, 0x133b55d0 )
	ROM_LOAD16_BYTE("d21-19.6", 0x100001, 0x20000, 0x1f98908f )

	ROM_REGION16_BE(0x800000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d21-01.17", 0x000000, 0x200000, 0x1fb6f07d )
	ROM_LOAD16_BYTE("d21-05.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( arabianm )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d29-23.rom", 0x000000, 0x40000, 0x89a0c706 )
	ROM_LOAD32_BYTE("d29-22.rom", 0x000001, 0x40000, 0x4afc22a4 )
	ROM_LOAD32_BYTE("d29-21.rom", 0x000002, 0x40000, 0xac32eb38 )
	ROM_LOAD32_BYTE("d29-25.rom", 0x000003, 0x40000, 0xb9b652ed )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d29-03.rom", 0x000000, 0x100000, 0xaeaff456 )
	ROM_LOAD16_BYTE("d29-04.rom", 0x000001, 0x100000, 0x01711cfe )
	ROM_LOAD       ("d29-05.rom", 0x300000, 0x100000, 0x9b5f7a17 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d29-06.rom", 0x000000, 0x080000, 0xeea07bf3 )
  	ROM_LOAD16_BYTE("d29-07.rom", 0x000001, 0x080000, 0xdb3c094d )
	ROM_LOAD       ("d29-08.rom", 0x180000, 0x080000, 0xd7562851 )
	ROM_FILL       (              0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d29-18.rom", 0x100000, 0x20000, 0xd97780df )
	ROM_LOAD16_BYTE("d29-19.rom", 0x100001, 0x20000, 0xb1ad365c )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d29-01.rom", 0x000000, 0x200000, 0x545ac4b3 )
	ROM_LOAD16_BYTE("d29-02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( arabiamj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d29-23.rom", 0x000000, 0x40000, 0x89a0c706 )
	ROM_LOAD32_BYTE("d29-22.rom", 0x000001, 0x40000, 0x4afc22a4 )
	ROM_LOAD32_BYTE("d29-21.rom", 0x000002, 0x40000, 0xac32eb38 )
	ROM_LOAD32_BYTE("d29-20.34",  0x000003, 0x40000, 0x57b833c1 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d29-03.rom", 0x000000, 0x100000, 0xaeaff456 )
	ROM_LOAD16_BYTE("d29-04.rom", 0x000001, 0x100000, 0x01711cfe )
	ROM_LOAD       ("d29-05.rom", 0x300000, 0x100000, 0x9b5f7a17 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d29-06.rom", 0x000000, 0x080000, 0xeea07bf3 )
  	ROM_LOAD16_BYTE("d29-07.rom", 0x000001, 0x080000, 0xdb3c094d )
	ROM_LOAD       ("d29-08.rom", 0x180000, 0x080000, 0xd7562851 )
	ROM_FILL       (              0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d29-18.rom", 0x100000, 0x20000, 0xd97780df )
	ROM_LOAD16_BYTE("d29-19.rom", 0x100001, 0x20000, 0xb1ad365c )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d29-01.rom", 0x000000, 0x200000, 0x545ac4b3 )
	ROM_LOAD16_BYTE("d29-02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( arabiamu )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d29-23.rom", 0x000000, 0x40000, 0x89a0c706 )
	ROM_LOAD32_BYTE("d29-22.rom", 0x000001, 0x40000, 0x4afc22a4 )
	ROM_LOAD32_BYTE("d29-21.rom", 0x000002, 0x40000, 0xac32eb38 )
	ROM_LOAD32_BYTE("d29-24.bin", 0x000003, 0x40000, 0xceb1627b )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d29-03.rom", 0x000000, 0x100000, 0xaeaff456 )
	ROM_LOAD16_BYTE("d29-04.rom", 0x000001, 0x100000, 0x01711cfe )
	ROM_LOAD       ("d29-05.rom", 0x300000, 0x100000, 0x9b5f7a17 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d29-06.rom", 0x000000, 0x080000, 0xeea07bf3 )
  	ROM_LOAD16_BYTE("d29-07.rom", 0x000001, 0x080000, 0xdb3c094d )
	ROM_LOAD       ("d29-08.rom", 0x180000, 0x080000, 0xd7562851 )
	ROM_FILL       (              0x100000, 0x080000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d29-18.rom", 0x100000, 0x20000, 0xd97780df )
	ROM_LOAD16_BYTE("d29-19.rom", 0x100001, 0x20000, 0xb1ad365c )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d29-01.rom", 0x000000, 0x200000, 0x545ac4b3 )
	ROM_LOAD16_BYTE("d29-02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( ridingf )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d34-12.40", 0x000000, 0x40000, 0xe67e69d4 )
	ROM_LOAD32_BYTE("d34-11.38", 0x000001, 0x40000, 0x7d240a88 )
	ROM_LOAD32_BYTE("d34-10.36", 0x000002, 0x40000, 0x8aa3f4ac )
	ROM_LOAD32_BYTE("d34_14.34", 0x000003, 0x40000, 0xe000198e )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d34-01.66", 0x000000, 0x200000, 0x7974e6aa )
	ROM_LOAD16_BYTE("d34-02.67", 0x000001, 0x200000, 0xf4422370 )
	ROM_FILL       (             0x400000, 0x400000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d34-05.49", 0x000000, 0x080000, 0x72e3ee4b )
  	ROM_LOAD16_BYTE("d34-06.50", 0x000001, 0x080000, 0xedc9b9f3 )
	ROM_FILL       (             0x100000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d34-07.5", 0x100000, 0x20000, 0x67239e2b )
	ROM_LOAD16_BYTE("d34-08.6", 0x100001, 0x20000, 0x2cf20323 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d34-03.17", 0x000000, 0x200000, 0xe534ef74 )
	ROM_LOAD16_BYTE("d34-04.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( ridefgtj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d34-12.40", 0x000000, 0x40000, 0xe67e69d4 )
	ROM_LOAD32_BYTE("d34-11.38", 0x000001, 0x40000, 0x7d240a88 )
	ROM_LOAD32_BYTE("d34-10.36", 0x000002, 0x40000, 0x8aa3f4ac )
	ROM_LOAD32_BYTE("d34-09.34", 0x000003, 0x40000, 0x0e0e78a2 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d34-01.66", 0x000000, 0x200000, 0x7974e6aa )
	ROM_LOAD16_BYTE("d34-02.67", 0x000001, 0x200000, 0xf4422370 )
	ROM_FILL       (             0x400000, 0x400000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d34-05.49", 0x000000, 0x080000, 0x72e3ee4b )
  	ROM_LOAD16_BYTE("d34-06.50", 0x000001, 0x080000, 0xedc9b9f3 )
	ROM_FILL       (             0x100000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d34-07.5", 0x100000, 0x20000, 0x67239e2b )
	ROM_LOAD16_BYTE("d34-08.6", 0x100001, 0x20000, 0x2cf20323 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d34-03.17", 0x000000, 0x200000, 0xe534ef74 )
	ROM_LOAD16_BYTE("d34-04.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( ridefgtu )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d34-12.40", 0x000000, 0x40000, 0xe67e69d4 )
	ROM_LOAD32_BYTE("d34-11.38", 0x000001, 0x40000, 0x7d240a88 )
	ROM_LOAD32_BYTE("d34-10.36", 0x000002, 0x40000, 0x8aa3f4ac )
	ROM_LOAD32_BYTE("d34_13.34", 0x000003, 0x40000, 0x97072918 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d34-01.66", 0x000000, 0x200000, 0x7974e6aa )
	ROM_LOAD16_BYTE("d34-02.67", 0x000001, 0x200000, 0xf4422370 )
	ROM_FILL       (             0x400000, 0x400000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d34-05.49", 0x000000, 0x080000, 0x72e3ee4b )
  	ROM_LOAD16_BYTE("d34-06.50", 0x000001, 0x080000, 0xedc9b9f3 )
	ROM_FILL       (             0x100000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d34-07.5", 0x100000, 0x20000, 0x67239e2b )
	ROM_LOAD16_BYTE("d34-08.6", 0x100001, 0x20000, 0x2cf20323 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d34-03.17", 0x000000, 0x200000, 0xe534ef74 )
	ROM_LOAD16_BYTE("d34-04.18", 0x400000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( gseeker )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d40_12.rom", 0x000000, 0x40000, 0x884055fb )
	ROM_LOAD32_BYTE("d40_11.rom", 0x000001, 0x40000, 0x85e701d2 )
	ROM_LOAD32_BYTE("d40_10.rom", 0x000002, 0x40000, 0x1e659ac5 )
	ROM_LOAD32_BYTE("d40_14.rom", 0x000003, 0x40000, 0xd9a76bd9 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d40_03.rom", 0x000000, 0x100000, 0xbcd70efc )
	ROM_LOAD16_BYTE("d40_04.rom", 0x100001, 0x080000, 0xcd2ac666 )
	ROM_CONTINUE(0,0x80000)
	ROM_LOAD16_BYTE("d40_15.rom", 0x000000, 0x080000, 0x50555125 )
	ROM_LOAD16_BYTE("d40_16.rom", 0x000001, 0x080000, 0x3f9bbe1e )
	/* Taito manufactured mask roms 3 + 4 wrong, and later added 15 + 16 as a patch */
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d40_05.rom", 0x000000, 0x100000, 0xbe6eec8f )
	ROM_LOAD16_BYTE("d40_06.rom", 0x000001, 0x100000, 0xa822abe4 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d40_07.rom", 0x100000, 0x20000, 0x7e9b26c2 )
	ROM_LOAD16_BYTE("d40_08.rom", 0x100001, 0x20000, 0x9c926a28 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d40_01.rom", 0x000000, 0x200000, 0xee312e95 )
	ROM_LOAD16_BYTE("d40_02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( gseekerj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d40_12.rom", 0x000000, 0x40000, 0x884055fb )
	ROM_LOAD32_BYTE("d40_11.rom", 0x000001, 0x40000, 0x85e701d2 )
	ROM_LOAD32_BYTE("d40_10.rom", 0x000002, 0x40000, 0x1e659ac5 )
	ROM_LOAD32_BYTE("d40-09.34",  0x000003, 0x40000, 0x37a90af5 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d40_03.rom", 0x000000, 0x100000, 0xbcd70efc )
	ROM_LOAD16_BYTE("d40_04.rom", 0x100001, 0x080000, 0xcd2ac666 )
	ROM_CONTINUE(0,0x80000)
	ROM_LOAD16_BYTE("d40_15.rom", 0x000000, 0x080000, 0x50555125 )
	ROM_LOAD16_BYTE("d40_16.rom", 0x000001, 0x080000, 0x3f9bbe1e )
	/* Taito manufactured mask roms 3 + 4 wrong, and later added 15 + 16 as a patch */
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d40_05.rom", 0x000000, 0x100000, 0xbe6eec8f )
	ROM_LOAD16_BYTE("d40_06.rom", 0x000001, 0x100000, 0xa822abe4 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d40_07.rom", 0x100000, 0x20000, 0x7e9b26c2 )
	ROM_LOAD16_BYTE("d40_08.rom", 0x100001, 0x20000, 0x9c926a28 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d40_01.rom", 0x000000, 0x200000, 0xee312e95 )
	ROM_LOAD16_BYTE("d40_02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( gseekeru )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d40_12.rom", 0x000000, 0x40000, 0x884055fb )
	ROM_LOAD32_BYTE("d40_11.rom", 0x000001, 0x40000, 0x85e701d2 )
	ROM_LOAD32_BYTE("d40_10.rom", 0x000002, 0x40000, 0x1e659ac5 )
	ROM_LOAD32_BYTE("d40-13.bin", 0x000003, 0x40000, 0xaea05b4f )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d40_03.rom", 0x000000, 0x100000, 0xbcd70efc )
	ROM_LOAD16_BYTE("d40_04.rom", 0x100001, 0x080000, 0xcd2ac666 )
	ROM_CONTINUE(0,0x80000)
	ROM_LOAD16_BYTE("d40_15.rom", 0x000000, 0x080000, 0x50555125 )
	ROM_LOAD16_BYTE("d40_16.rom", 0x000001, 0x080000, 0x3f9bbe1e )
	/* Taito manufactured mask roms 3 + 4 wrong, and later added 15 + 16 as a patch */
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d40_05.rom", 0x000000, 0x100000, 0xbe6eec8f )
	ROM_LOAD16_BYTE("d40_06.rom", 0x000001, 0x100000, 0xa822abe4 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d40_07.rom", 0x100000, 0x20000, 0x7e9b26c2 )
	ROM_LOAD16_BYTE("d40_08.rom", 0x100001, 0x20000, 0x9c926a28 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d40_01.rom", 0x000000, 0x200000, 0xee312e95 )
	ROM_LOAD16_BYTE("d40_02.rom", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( cupfinal )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d49-13.bin", 0x000000, 0x20000, 0xccee5e73 )
	ROM_LOAD32_BYTE("d49-14.bin", 0x000001, 0x20000, 0x2323bf2e )
	ROM_LOAD32_BYTE("d49-16.bin", 0x000002, 0x20000, 0x8e73f739 )
	ROM_LOAD32_BYTE("d49-20.bin", 0x000003, 0x20000, 0x1e9c392c )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d49-01", 0x000000, 0x200000, 0x1dc89f1c )
  	ROM_LOAD16_BYTE("d49-02", 0x000001, 0x200000, 0x1e4c374f )
	ROM_LOAD16_BYTE("d49-06", 0x400000, 0x100000, 0x71ef4ee1 )
  	ROM_LOAD16_BYTE("d49-07", 0x400001, 0x100000, 0xe5655b8f )
	ROM_LOAD       ("d49-03", 0x900000, 0x200000, 0xcf9a8727 )
	ROM_LOAD       ("d49-08", 0xb00000, 0x100000, 0x7d3c6536 )
	ROM_FILL       (          0x600000, 0x300000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d49-09", 0x000000, 0x080000, 0x257ede01 )
	ROM_LOAD16_BYTE("d49-10", 0x000001, 0x080000, 0xf587b787 )
	ROM_LOAD       ("d49-11", 0x180000, 0x080000, 0x11318b26 )
	ROM_FILL       (          0x100000, 0x080000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d49-17", 0x100000, 0x40000, 0x49942466 )
	ROM_LOAD16_BYTE("d49-18", 0x100001, 0x40000, 0x9d75b7d4 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d49-04", 0x000000, 0x200000, 0x44b365a9 )
	ROM_LOAD16_BYTE("d49-05", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( hthero93 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d49-13.bin", 0x000000, 0x20000, 0xccee5e73 )
	ROM_LOAD32_BYTE("d49-14.bin", 0x000001, 0x20000, 0x2323bf2e )
	ROM_LOAD32_BYTE("d49-16.bin", 0x000002, 0x20000, 0x8e73f739 )
	ROM_LOAD32_BYTE("d49-19.35",  0x000003, 0x20000, 0xf0925800 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d49-01", 0x000000, 0x200000, 0x1dc89f1c )
  	ROM_LOAD16_BYTE("d49-02", 0x000001, 0x200000, 0x1e4c374f )
	ROM_LOAD16_BYTE("d49-06", 0x400000, 0x100000, 0x71ef4ee1 )
  	ROM_LOAD16_BYTE("d49-07", 0x400001, 0x100000, 0xe5655b8f )
	ROM_LOAD       ("d49-03", 0x900000, 0x200000, 0xcf9a8727 )
	ROM_LOAD       ("d49-08", 0xb00000, 0x100000, 0x7d3c6536 )
	ROM_FILL       (          0x600000, 0x300000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d49-09", 0x000000, 0x080000, 0x257ede01 )
	ROM_LOAD16_BYTE("d49-10", 0x000001, 0x080000, 0xf587b787 )
	ROM_LOAD       ("d49-11", 0x180000, 0x080000, 0x11318b26 )
	ROM_FILL       (          0x100000, 0x080000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d49-17", 0x100000, 0x40000, 0x49942466 )
	ROM_LOAD16_BYTE("d49-18", 0x100001, 0x40000, 0x9d75b7d4 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d49-04", 0x000000, 0x200000, 0x44b365a9 )
	ROM_LOAD16_BYTE("d49-05", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( trstar )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15-1.24", 0x000000, 0x40000, 0x098bba94 )
	ROM_LOAD32_BYTE("d53-16-1.26", 0x000001, 0x40000, 0x4fa8b15c )
	ROM_LOAD32_BYTE("d53-18-1.37", 0x000002, 0x40000, 0xaa71cfcc )
	ROM_LOAD32_BYTE("d53-20-1.rom", 0x000003, 0x40000, 0x4de1e287 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( trstarj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15-1.24", 0x000000, 0x40000, 0x098bba94 )
	ROM_LOAD32_BYTE("d53-16-1.26", 0x000001, 0x40000, 0x4fa8b15c )
	ROM_LOAD32_BYTE("d53-18-1.37", 0x000002, 0x40000, 0xaa71cfcc )
	ROM_LOAD32_BYTE("d53-17-1.35", 0x000003, 0x40000, 0xa3ef83ab )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( prmtmfgt )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15-1.24", 0x000000, 0x40000, 0x098bba94 )
	ROM_LOAD32_BYTE("d53-16-1.26", 0x000001, 0x40000, 0x4fa8b15c )
	ROM_LOAD32_BYTE("d53-18-1.37", 0x000002, 0x40000, 0xaa71cfcc )
	ROM_LOAD32_BYTE("d53-19-1.bin", 0x000003, 0x40000, 0x3ae6d211 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( trstaro )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15.24", 0x000000, 0x40000, 0xf24de51b )
	ROM_LOAD32_BYTE("d53-16.26", 0x000001, 0x40000, 0xffc84429 )
	ROM_LOAD32_BYTE("d53-18.37", 0x000002, 0x40000, 0xea2d6e13 )
	ROM_LOAD32_BYTE("d53-20.rom",0x000003, 0x40000, 0x77e1f267 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( trstaroj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15.24", 0x000000, 0x40000, 0xf24de51b )
	ROM_LOAD32_BYTE("d53-16.26", 0x000001, 0x40000, 0xffc84429 )
	ROM_LOAD32_BYTE("d53-18.37", 0x000002, 0x40000, 0xea2d6e13 )
	ROM_LOAD32_BYTE("d53-17.35", 0x000003, 0x40000, 0x99ef934b )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( prmtmfgo )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d53-15.24", 0x000000, 0x40000, 0xf24de51b )
	ROM_LOAD32_BYTE("d53-16.26", 0x000001, 0x40000, 0xffc84429 )
	ROM_LOAD32_BYTE("d53-18.37", 0x000002, 0x40000, 0xea2d6e13 )
	ROM_LOAD32_BYTE("d53-19.35", 0x000003, 0x40000, 0x00e6c2f1 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d53-03.45", 0x000000, 0x200000, 0x91b66145 )
  	ROM_LOAD16_BYTE("d53-04.46", 0x000001, 0x200000, 0xac3a5e80 )
	ROM_LOAD16_BYTE("d53-06.64", 0x400000, 0x100000, 0xf4bac410 )
  	ROM_LOAD16_BYTE("d53-07.65", 0x400001, 0x100000, 0x2f4773c3 )
	ROM_LOAD       ("d53-05.47", 0x900000, 0x200000, 0xb9b68b15 )
	ROM_LOAD       ("d53-08.66", 0xb00000, 0x100000, 0xad13a1ee )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d53-09.48", 0x000000, 0x100000, 0x690554d3 )
	ROM_LOAD16_BYTE("d53-10.49", 0x000001, 0x100000, 0x0ec05dc5 )
	ROM_LOAD       ("d53-11.50", 0x300000, 0x100000, 0x39c0a546 )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d53-13.10", 0x100000, 0x20000, 0x877f0361 )
	ROM_LOAD16_BYTE("d53-14.23", 0x100001, 0x20000, 0xa8664867 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d53-01.2", 0x000000, 0x200000, 0x28fd2d9b )
	ROM_LOAD16_BYTE("d53-02.3", 0xc00000, 0x200000, 0x8bd4367a )
ROM_END

ROM_START( gunlock )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d66-18.rom", 0x000000, 0x40000, 0x8418513e )
	ROM_LOAD32_BYTE("d66-19.rom", 0x000001, 0x40000, 0x95731473 )
	ROM_LOAD32_BYTE("d66-21.rom", 0x000002, 0x40000, 0xbd0d60f2 )
	ROM_LOAD32_BYTE("d66-24.rom", 0x000003, 0x40000, 0x97816378 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d66-03.rom", 0x000000, 0x100000, 0xe7a4a491 )
	ROM_LOAD16_BYTE("d66-04.rom", 0x000001, 0x100000, 0xc1c7aaa7 )
	ROM_LOAD       ("d66-05.rom", 0x300000, 0x100000, 0xa3cefe04 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d66-06.rom", 0x000000, 0x100000, 0xb3d8126d )
  	ROM_LOAD16_BYTE("d66-07.rom", 0x000001, 0x100000, 0xa6da9be7 )
	ROM_LOAD       ("d66-08.rom", 0x300000, 0x100000, 0x9959f30b )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("d66-23.rom", 0x100000, 0x40000, 0x57fb7c49 )
	ROM_LOAD16_BYTE("d66-22.rom", 0x100001, 0x40000, 0x83dd7f9b )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d66-01.rom", 0x000000, 0x200000, 0x58c92efa )
	ROM_LOAD16_BYTE("d66-02.rom", 0xc00000, 0x200000, 0xdcdafaab )
ROM_END

ROM_START( rayforce )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d66-18.rom", 0x000000, 0x40000, 0x8418513e )
	ROM_LOAD32_BYTE("d66-19.rom", 0x000001, 0x40000, 0x95731473 )
	ROM_LOAD32_BYTE("d66-21.rom", 0x000002, 0x40000, 0xbd0d60f2 )
	ROM_LOAD32_BYTE("gunlocku.35",0x000003, 0x40000, 0xe08653ee )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d66-03.rom", 0x000000, 0x100000, 0xe7a4a491 )
	ROM_LOAD16_BYTE("d66-04.rom", 0x000001, 0x100000, 0xc1c7aaa7 )
	ROM_LOAD       ("d66-05.rom", 0x300000, 0x100000, 0xa3cefe04 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d66-06.rom", 0x000000, 0x100000, 0xb3d8126d )
  	ROM_LOAD16_BYTE("d66-07.rom", 0x000001, 0x100000, 0xa6da9be7 )
	ROM_LOAD       ("d66-08.rom", 0x300000, 0x100000, 0x9959f30b )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("d66-23.rom", 0x100000, 0x40000, 0x57fb7c49 )
	ROM_LOAD16_BYTE("d66-22.rom", 0x100001, 0x40000, 0x83dd7f9b )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d66-01.rom", 0x000000, 0x200000, 0x58c92efa )
	ROM_LOAD16_BYTE("d66-02.rom", 0xc00000, 0x200000, 0xdcdafaab )
ROM_END

ROM_START( rayforcj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d66-18.rom", 0x000000, 0x40000, 0x8418513e )
	ROM_LOAD32_BYTE("d66-19.rom", 0x000001, 0x40000, 0x95731473 )
	ROM_LOAD32_BYTE("d66-21.rom", 0x000002, 0x40000, 0xbd0d60f2 )
	ROM_LOAD32_BYTE("d66-20.35",  0x000003, 0x40000, 0x798f0254 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d66-03.rom", 0x000000, 0x100000, 0xe7a4a491 )
	ROM_LOAD16_BYTE("d66-04.rom", 0x000001, 0x100000, 0xc1c7aaa7 )
	ROM_LOAD       ("d66-05.rom", 0x300000, 0x100000, 0xa3cefe04 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d66-06.rom", 0x000000, 0x100000, 0xb3d8126d )
  	ROM_LOAD16_BYTE("d66-07.rom", 0x000001, 0x100000, 0xa6da9be7 )
	ROM_LOAD       ("d66-08.rom", 0x300000, 0x100000, 0x9959f30b )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("d66-23.rom", 0x100000, 0x40000, 0x57fb7c49 )
	ROM_LOAD16_BYTE("d66-22.rom", 0x100001, 0x40000, 0x83dd7f9b )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d66-01.rom", 0x000000, 0x200000, 0x58c92efa )
	ROM_LOAD16_BYTE("d66-02.rom", 0xc00000, 0x200000, 0xdcdafaab )
ROM_END

ROM_START( scfinals )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d68-01", 0x000000, 0x40000, 0xcb951856 )
	ROM_LOAD32_BYTE("d68-02", 0x000001, 0x40000, 0x4f94413a )
	ROM_LOAD32_BYTE("d68-04", 0x000002, 0x40000, 0x4a4e4972 )
	ROM_LOAD32_BYTE("d68-03", 0x000003, 0x40000, 0xa40be699 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d49-01", 0x000000, 0x200000, 0x1dc89f1c )
  	ROM_LOAD16_BYTE("d49-02", 0x000001, 0x200000, 0x1e4c374f )
	ROM_LOAD16_BYTE("d49-06", 0x400000, 0x100000, 0x71ef4ee1 )
  	ROM_LOAD16_BYTE("d49-07", 0x400001, 0x100000, 0xe5655b8f )
	ROM_LOAD       ("d49-03", 0x900000, 0x200000, 0xcf9a8727 )
	ROM_LOAD       ("d49-08", 0xb00000, 0x100000, 0x7d3c6536 )
	ROM_FILL       (          0x600000, 0x300000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d49-09", 0x000000, 0x080000, 0x257ede01 )
	ROM_LOAD16_BYTE("d49-10", 0x000001, 0x080000, 0xf587b787 )
	ROM_LOAD       ("d49-11", 0x180000, 0x080000, 0x11318b26 )
	ROM_FILL       (          0x100000, 0x080000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d49-17", 0x100000, 0x40000, 0x49942466 )
	ROM_LOAD16_BYTE("d49-18", 0x100001, 0x40000, 0x9d75b7d4 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d49-04", 0x000000, 0x200000, 0x44b365a9 )
	ROM_LOAD16_BYTE("d49-05", 0x600000, 0x100000, 0xed894fe1 )
ROM_END

ROM_START( lightbr )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d69-20.bin", 0x000000, 0x80000, 0x33650fe4 )
	ROM_LOAD32_BYTE("d69-13.bin", 0x000001, 0x80000, 0xdec2ec17 )
	ROM_LOAD32_BYTE("d69-15.bin", 0x000002, 0x80000, 0x323e1955 )
	ROM_LOAD32_BYTE("d69-14.bin", 0x000003, 0x80000, 0x990bf945 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d69-06.bin", 0x000000, 0x200000, 0xcb4aac81 )
  	ROM_LOAD16_BYTE("d69-07.bin", 0x000001, 0x200000, 0xb749f984 )
	ROM_LOAD16_BYTE("d69-09.bin", 0x400000, 0x100000, 0xa96c19b8 )
	ROM_LOAD16_BYTE("d69-10.bin", 0x400001, 0x100000, 0x36aa80c6 )
	ROM_LOAD       ("d69-08.bin", 0x900000, 0x200000, 0x5b68d7d8 )
	ROM_LOAD       ("d69-11.bin", 0xb00000, 0x100000, 0xc11adf92 )
	ROM_FILL       (              0x600000, 0x300000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d69-03.bin", 0x000000, 0x200000, 0x6999c86f )
  	ROM_LOAD16_BYTE("d69-04.bin", 0x000001, 0x200000, 0xcc91dcb7 )
	ROM_LOAD       ("d69-05.bin", 0x600000, 0x200000, 0xf9f5433c )
	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d69-18.bin", 0x100000, 0x20000, 0x04600d7b )
	ROM_LOAD16_BYTE("d69-19.bin", 0x100001, 0x20000, 0x1484e853 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d69-01.bin", 0x000000, 0x200000, 0x9ac93ac2 )
	ROM_LOAD16_BYTE("d69-02.bin", 0xc00000, 0x200000, 0xdce28dd7 )
ROM_END

ROM_START( kaiserkn )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d84-25.rom", 0x000000, 0x80000, 0x2840893f )
	ROM_LOAD32_BYTE("d84-24.rom", 0x000001, 0x80000, 0xbf20c755 )
	ROM_LOAD32_BYTE("d84-23.rom", 0x000002, 0x80000, 0x39f12a9b )
	ROM_LOAD32_BYTE("d84-29.rom", 0x000003, 0x80000, 0x9821f17a )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d84-03.rom", 0x000000, 0x200000, 0xd786f552 )
  	ROM_LOAD16_BYTE("d84-04.rom", 0x000001, 0x200000, 0xd1f32b5d )
	ROM_LOAD16_BYTE("d84-06.rom", 0x400000, 0x200000, 0xfa924dab )
  	ROM_LOAD16_BYTE("d84-07.rom", 0x400001, 0x200000, 0x54517a6b )
	ROM_LOAD16_BYTE("d84-09.rom", 0x800000, 0x200000, 0xfaa78d98 )
  	ROM_LOAD16_BYTE("d84-10.rom", 0x800001, 0x200000, 0xb84b7320 )
	ROM_LOAD       ("d84-05.rom", 0x1200000, 0x200000, 0x31a3c75d )
	ROM_LOAD       ("d84-08.rom", 0x1400000, 0x200000, 0x07347bf1 )
	ROM_LOAD       ("d84-11.rom", 0x1600000, 0x200000, 0xa062c1d4 )
	ROM_FILL       (              0xc00000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d84-12.rom", 0x000000, 0x200000, 0x66a7a9aa )
	ROM_LOAD16_BYTE("d84-13.rom", 0x000001, 0x200000, 0xae125516 )
	ROM_LOAD16_BYTE("d84-16.rom", 0x400000, 0x100000, 0xbcff9b2d )
	ROM_LOAD16_BYTE("d84-17.rom", 0x400001, 0x100000, 0x0be37cc3 )
	ROM_LOAD       ("d84-14.rom", 0x900000, 0x200000, 0x2b2e693e )
	ROM_LOAD       ("d84-18.rom", 0xb00000, 0x100000, 0xe812bcc5 )
	ROM_FILL       (              0x600000, 0x300000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d84-26.rom", 0x100000, 0x40000, 0x4f5b8563 )
	ROM_LOAD16_BYTE("d84-27.rom", 0x100001, 0x40000, 0xfb0cb1ba )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d84-01.rom", 0x000000, 0x200000, 0x9ad22149 )
	ROM_LOAD16_BYTE("d84-02.rom", 0x400000, 0x200000, 0x9e1827e4 )
ROM_END

ROM_START( kaiserkj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d84-25.rom", 0x000000, 0x80000, 0x2840893f )
	ROM_LOAD32_BYTE("d84-24.rom", 0x000001, 0x80000, 0xbf20c755 )
	ROM_LOAD32_BYTE("d84-23.rom", 0x000002, 0x80000, 0x39f12a9b )
	ROM_LOAD32_BYTE("d84-22.17",  0x000003, 0x80000, 0x762f9056 )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d84-03.rom", 0x000000, 0x200000, 0xd786f552 )
  	ROM_LOAD16_BYTE("d84-04.rom", 0x000001, 0x200000, 0xd1f32b5d )
	ROM_LOAD16_BYTE("d84-06.rom", 0x400000, 0x200000, 0xfa924dab )
  	ROM_LOAD16_BYTE("d84-07.rom", 0x400001, 0x200000, 0x54517a6b )
	ROM_LOAD16_BYTE("d84-09.rom", 0x800000, 0x200000, 0xfaa78d98 )
  	ROM_LOAD16_BYTE("d84-10.rom", 0x800001, 0x200000, 0xb84b7320 )
	ROM_LOAD       ("d84-05.rom", 0x1200000, 0x200000, 0x31a3c75d )
	ROM_LOAD       ("d84-08.rom", 0x1400000, 0x200000, 0x07347bf1 )
	ROM_LOAD       ("d84-11.rom", 0x1600000, 0x200000, 0xa062c1d4 )
	ROM_FILL       (              0xc00000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d84-12.rom", 0x000000, 0x200000, 0x66a7a9aa )
	ROM_LOAD16_BYTE("d84-13.rom", 0x000001, 0x200000, 0xae125516 )
	ROM_LOAD16_BYTE("d84-16.rom", 0x400000, 0x100000, 0xbcff9b2d )
	ROM_LOAD16_BYTE("d84-17.rom", 0x400001, 0x100000, 0x0be37cc3 )
	ROM_LOAD       ("d84-14.rom", 0x900000, 0x200000, 0x2b2e693e )
	ROM_LOAD       ("d84-18.rom", 0xb00000, 0x100000, 0xe812bcc5 )
	ROM_FILL       (              0x600000, 0x300000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d84-26.rom", 0x100000, 0x40000, 0x4f5b8563 )
	ROM_LOAD16_BYTE("d84-27.rom", 0x100001, 0x40000, 0xfb0cb1ba )

	ROM_REGION16_BE( 0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d84-01.rom", 0x000000, 0x200000, 0x9ad22149 )
	ROM_LOAD16_BYTE("d84-02.rom", 0x400000, 0x200000, 0x9e1827e4 )
ROM_END

ROM_START( gblchmp )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d84-25.rom", 0x000000, 0x80000, 0x2840893f )
	ROM_LOAD32_BYTE("d84-24.rom", 0x000001, 0x80000, 0xbf20c755 )
	ROM_LOAD32_BYTE("d84-23.rom", 0x000002, 0x80000, 0x39f12a9b )
	ROM_LOAD32_BYTE("d84-28.bin", 0x000003, 0x80000, 0xef26c1ec )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d84-03.rom", 0x000000, 0x200000, 0xd786f552 )
  	ROM_LOAD16_BYTE("d84-04.rom", 0x000001, 0x200000, 0xd1f32b5d )
	ROM_LOAD16_BYTE("d84-06.rom", 0x400000, 0x200000, 0xfa924dab )
  	ROM_LOAD16_BYTE("d84-07.rom", 0x400001, 0x200000, 0x54517a6b )
	ROM_LOAD16_BYTE("d84-09.rom", 0x800000, 0x200000, 0xfaa78d98 )
  	ROM_LOAD16_BYTE("d84-10.rom", 0x800001, 0x200000, 0xb84b7320 )
	ROM_LOAD       ("d84-05.rom", 0x1200000, 0x200000, 0x31a3c75d )
	ROM_LOAD       ("d84-08.rom", 0x1400000, 0x200000, 0x07347bf1 )
	ROM_LOAD       ("d84-11.rom", 0x1600000, 0x200000, 0xa062c1d4 )
	ROM_FILL       (              0xc00000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d84-12.rom", 0x000000, 0x200000, 0x66a7a9aa )
	ROM_LOAD16_BYTE("d84-13.rom", 0x000001, 0x200000, 0xae125516 )
	ROM_LOAD16_BYTE("d84-16.rom", 0x400000, 0x100000, 0xbcff9b2d )
	ROM_LOAD16_BYTE("d84-17.rom", 0x400001, 0x100000, 0x0be37cc3 )
	ROM_LOAD       ("d84-14.rom", 0x900000, 0x200000, 0x2b2e693e )
	ROM_LOAD       ("d84-18.rom", 0xb00000, 0x100000, 0xe812bcc5 )
	ROM_FILL       (              0x600000, 0x300000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d84-26.rom", 0x100000, 0x40000, 0x4f5b8563 )
	ROM_LOAD16_BYTE("d84-27.rom", 0x100001, 0x40000, 0xfb0cb1ba )

	ROM_REGION16_BE( 0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d84-01.rom", 0x000000, 0x200000, 0x9ad22149 )
	ROM_LOAD16_BYTE("d84-02.rom", 0x400000, 0x200000, 0x9e1827e4 )
ROM_END

ROM_START( dankuga )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("dkg_mpr3.bin", 0x000000, 0x80000, 0xee1531ca )
	ROM_LOAD32_BYTE("dkg_mpr2.bin", 0x000001, 0x80000, 0x18a4748b )
	ROM_LOAD32_BYTE("dkg_mpr1.bin", 0x000002, 0x80000, 0x97566f69 )
	ROM_LOAD32_BYTE("dkg_mpr0.bin", 0x000003, 0x80000, 0xad6ada07 )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d84-03.rom", 0x000000, 0x200000, 0xd786f552 )
  	ROM_LOAD16_BYTE("d84-04.rom", 0x000001, 0x200000, 0xd1f32b5d )
	ROM_LOAD16_BYTE("d84-06.rom", 0x400000, 0x200000, 0xfa924dab )
  	ROM_LOAD16_BYTE("d84-07.rom", 0x400001, 0x200000, 0x54517a6b )
	ROM_LOAD16_BYTE("d84-09.rom", 0x800000, 0x200000, 0xfaa78d98 )
  	ROM_LOAD16_BYTE("d84-10.rom", 0x800001, 0x200000, 0xb84b7320 )
	ROM_LOAD       ("d84-05.rom", 0x1200000, 0x200000, 0x31a3c75d )
	ROM_LOAD       ("d84-08.rom", 0x1400000, 0x200000, 0x07347bf1 )
	ROM_LOAD       ("d84-11.rom", 0x1600000, 0x200000, 0xa062c1d4 )
	ROM_FILL       (              0xc00000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d84-12.rom", 0x000000, 0x200000, 0x66a7a9aa )
	ROM_LOAD16_BYTE("d84-13.rom", 0x000001, 0x200000, 0xae125516 )
	ROM_LOAD16_BYTE("d84-16.rom", 0x400000, 0x100000, 0xbcff9b2d )
	ROM_LOAD16_BYTE("d84-17.rom", 0x400001, 0x100000, 0x0be37cc3 )
	ROM_LOAD       ("d84-14.rom", 0x900000, 0x200000, 0x2b2e693e )
	ROM_LOAD       ("d84-18.rom", 0xb00000, 0x100000, 0xe812bcc5 )
	ROM_FILL       (              0x600000, 0x300000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d84-26.rom", 0x100000, 0x40000, 0x4f5b8563 )
	ROM_LOAD16_BYTE("d84-27.rom", 0x100001, 0x40000, 0xfb0cb1ba )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d84-01.rom", 0x000000, 0x200000, 0x9ad22149 )
	ROM_LOAD16_BYTE("d84-02.rom", 0x400000, 0x200000, 0x9e1827e4 )
ROM_END

ROM_START( dariusg )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d87-12.bin", 0x000000, 0x80000, 0xde78f328 )
	ROM_LOAD32_BYTE("d87-11.bin", 0x000001, 0x80000, 0xf7bed18e )
	ROM_LOAD32_BYTE("d87-10.bin", 0x000002, 0x80000, 0x4149f66f )
	ROM_LOAD32_BYTE("d87-09.bin", 0x000003, 0x80000, 0x6170382d )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d87-03.bin", 0x000000, 0x200000, 0x4be1666e )
	ROM_LOAD16_BYTE("d87-04.bin", 0x000001, 0x200000, 0x2616002c )
	ROM_LOAD       ("d87-05.bin", 0x600000, 0x200000, 0x4e5891a9 )
	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d87-06.bin", 0x000000, 0x200000, 0x3b97a07c )
	ROM_LOAD16_BYTE("d87-17.bin", 0x000001, 0x200000, 0xe601d63e )
	ROM_LOAD       ("d87-08.bin", 0x600000, 0x200000, 0x76d23602 )
	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d87-13.bin", 0x100000, 0x40000, 0x15b1fff4 )
	ROM_LOAD16_BYTE("d87-14.bin", 0x100001, 0x40000, 0xeecda29a )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d87-01.bin", 0x000000, 0x200000, 0x3848a110 )
	ROM_LOAD16_BYTE("d87-02.bin", 0xc00000, 0x200000, 0x9250abae )
ROM_END

ROM_START( dariusgx )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("dge_mpr3.bin", 0x000000, 0x80000, 0x1c1e24a7 )
	ROM_LOAD32_BYTE("dge_mpr2.bin", 0x000001, 0x80000, 0x7be23e23 )
	ROM_LOAD32_BYTE("dge_mpr1.bin", 0x000002, 0x80000, 0xbc030f6f )
	ROM_LOAD32_BYTE("dge_mpr0.bin", 0x000003, 0x80000, 0xc5bd135c )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d87-03.bin", 0x000000, 0x200000, 0x4be1666e )
	ROM_LOAD16_BYTE("d87-04.bin", 0x000001, 0x200000, 0x2616002c )
	ROM_LOAD       ("d87-05.bin", 0x600000, 0x200000, 0x4e5891a9 )
	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d87-06.bin", 0x000000, 0x200000, 0x3b97a07c )
	ROM_LOAD16_BYTE("d87-17.bin", 0x000001, 0x200000, 0xe601d63e )
	ROM_LOAD       ("d87-08.bin", 0x600000, 0x200000, 0x76d23602 )
	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d87-13.bin", 0x100000, 0x40000, 0x15b1fff4 )
	ROM_LOAD16_BYTE("d87-14.bin", 0x100001, 0x40000, 0xeecda29a )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d87-01.bin", 0x000000, 0x200000, 0x3848a110 )
	ROM_LOAD16_BYTE("d87-02.bin", 0xc00000, 0x200000, 0x9250abae )
ROM_END

ROM_START( bublbob2 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d90.12", 0x000000, 0x40000, 0x9e523996 )
	ROM_LOAD32_BYTE("d90.11", 0x000001, 0x40000, 0xedfdbb7f )
	ROM_LOAD32_BYTE("d90.10", 0x000002, 0x40000, 0x8e957d3d )
	ROM_LOAD32_BYTE("d90.17", 0x000003, 0x40000, 0x711f1894 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d90.03", 0x000000, 0x100000, 0x6fa894a1 )
	ROM_LOAD16_BYTE("d90.02", 0x000001, 0x100000, 0x5ab04ca2 )
	ROM_LOAD       ("d90.01", 0x300000, 0x100000, 0x8aedb9e5 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d90.08", 0x000000, 0x100000, 0x25a4fb2c )
	ROM_LOAD16_BYTE("d90.07", 0x000001, 0x100000, 0xb436b42d )
	ROM_LOAD       ("d90.06", 0x300000, 0x100000, 0x166a72b8 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d90.13", 0x100000, 0x40000, 0x6762bd90 )
	ROM_LOAD16_BYTE("d90.14", 0x100001, 0x40000, 0x8e33357e )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE("d90.04", 0x000000, 0x200000, 0xfeee5fda )
 	ROM_LOAD16_BYTE("d90.05", 0xc00000, 0x200000, 0xc192331f )
ROM_END

ROM_START( bubsymph )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d90.12", 0x000000, 0x40000, 0x9e523996 )
	ROM_LOAD32_BYTE("d90.11", 0x000001, 0x40000, 0xedfdbb7f )
	ROM_LOAD32_BYTE("d90.10", 0x000002, 0x40000, 0x8e957d3d )
	ROM_LOAD32_BYTE("d90.09", 0x000003, 0x40000, 0x3f2090b7 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d90.03", 0x000000, 0x100000, 0x6fa894a1 )
	ROM_LOAD16_BYTE("d90.02", 0x000001, 0x100000, 0x5ab04ca2 )
	ROM_LOAD       ("d90.01", 0x300000, 0x100000, 0x8aedb9e5 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d90.08", 0x000000, 0x100000, 0x25a4fb2c )
	ROM_LOAD16_BYTE("d90.07", 0x000001, 0x100000, 0xb436b42d )
	ROM_LOAD       ("d90.06", 0x300000, 0x100000, 0x166a72b8 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d90.13", 0x100000, 0x40000, 0x6762bd90 )
	ROM_LOAD16_BYTE("d90.14", 0x100001, 0x40000, 0x8e33357e )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE("d90.04", 0x000000, 0x200000, 0xfeee5fda )
 	ROM_LOAD16_BYTE("d90.05", 0xc00000, 0x200000, 0xc192331f )
ROM_END

ROM_START( bubsympu )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d90.12", 0x000000, 0x40000, 0x9e523996 )
	ROM_LOAD32_BYTE("d90.11", 0x000001, 0x40000, 0xedfdbb7f )
	ROM_LOAD32_BYTE("d90.10", 0x000002, 0x40000, 0x8e957d3d )
	ROM_LOAD32_BYTE("d90.usa",0x000003, 0x40000, 0x06182802 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d90.03", 0x000000, 0x100000, 0x6fa894a1 )
	ROM_LOAD16_BYTE("d90.02", 0x000001, 0x100000, 0x5ab04ca2 )
	ROM_LOAD       ("d90.01", 0x300000, 0x100000, 0x8aedb9e5 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d90.08", 0x000000, 0x100000, 0x25a4fb2c )
	ROM_LOAD16_BYTE("d90.07", 0x000001, 0x100000, 0xb436b42d )
	ROM_LOAD       ("d90.06", 0x300000, 0x100000, 0x166a72b8 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("d90.13", 0x100000, 0x40000, 0x6762bd90 )
	ROM_LOAD16_BYTE("d90.14", 0x100001, 0x40000, 0x8e33357e )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE("d90.04", 0x000000, 0x200000, 0xfeee5fda )
 	ROM_LOAD16_BYTE("d90.05", 0xc00000, 0x200000, 0xc192331f )
ROM_END

ROM_START( spcinvdj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d93_04.bin", 0x000000, 0x20000, 0xcd9a4e5c )
	ROM_LOAD32_BYTE("d93_03.bin", 0x000001, 0x20000, 0x0174bfc1 )
	ROM_LOAD32_BYTE("d93_02.bin", 0x000002, 0x20000, 0x01922b31 )
	ROM_LOAD32_BYTE("d93_01.bin", 0x000003, 0x20000, 0x4a74ab1c )

	ROM_REGION(0x200000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d93-07.12", 0x000000, 0x80000, 0x8cf5b972 )
	ROM_LOAD16_BYTE("d93-08.08", 0x000001, 0x80000, 0x4c11af2b )
	ROM_FILL       (             0x100000, 0x100000,0 )

	ROM_REGION(0x80000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d93-09.47", 0x000000, 0x20000, 0x9076f663 )
	ROM_LOAD16_BYTE("d93-10.45", 0x000001, 0x20000, 0x8a3f531b )
	ROM_FILL       (             0x040000, 0x40000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d93_05.bin", 0x100000, 0x20000, 0xff365596 )
	ROM_LOAD16_BYTE("d93_06.bin", 0x100001, 0x20000, 0xef7ad400 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d93-11.38", 0x000000, 0x80000, 0xdf5853de )
	ROM_LOAD16_BYTE("d93-12.39", 0xe00000, 0x80000, 0xb0f71d60 )
ROM_END

ROM_START( pwrgoal )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d94-18.bin", 0x000000, 0x40000, 0xb92681c3 )
	ROM_LOAD32_BYTE("d94-17.bin", 0x000001, 0x40000, 0x6009333e )
	ROM_LOAD32_BYTE("d94-16.bin", 0x000002, 0x40000, 0xc6dbc9c8 )
	ROM_LOAD32_BYTE("d94-22.rom", 0x000003, 0x40000, 0xf672e487 )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d94-09.bin", 0x000000, 0x200000, 0x425e6bec )
	ROM_LOAD16_BYTE("d94-08.bin", 0x400000, 0x200000, 0xbd909caf )
	ROM_LOAD16_BYTE("d94-07.bin", 0x800000, 0x200000, 0xc8c95e49 )
	ROM_LOAD16_BYTE("d94-06.bin", 0x000001, 0x200000, 0x0ed1df55 )
	ROM_LOAD16_BYTE("d94-05.bin", 0x400001, 0x200000, 0x121c8542 )
	ROM_LOAD16_BYTE("d94-04.bin", 0x800001, 0x200000, 0x24958b50 )
	ROM_LOAD       ("d94-03.bin", 0x1200000, 0x200000, 0x95e32072 )
	ROM_LOAD       ("d94-02.bin", 0x1400000, 0x200000, 0xf460b9ac )
	ROM_LOAD       ("d94-01.bin", 0x1600000, 0x200000, 0x410ffccd )
	ROM_FILL       (              0xc00000, 0x600000,0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d94-14.bin", 0x000000, 0x100000, 0xb8ba5761 )
	ROM_LOAD16_BYTE("d94-13.bin", 0x000001, 0x100000, 0xcafc68ce )
	ROM_LOAD       ("d94-12.bin", 0x300000, 0x100000, 0x47064189 )
	ROM_FILL       (              0x200000, 0x100000,0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d94-19.bin", 0x100000, 0x40000, 0xc93dbcf4 )
	ROM_LOAD16_BYTE("d94-20.bin", 0x100001, 0x40000, 0xf232bf64 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d94-10.bin", 0x000000, 0x200000, 0xa22563ae )
	ROM_LOAD16_BYTE("d94-11.bin", 0xc00000, 0x200000, 0x61ed83fa )
ROM_END

ROM_START( hthero95 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d94-18.bin", 0x000000, 0x40000, 0xb92681c3 )
	ROM_LOAD32_BYTE("d94-17.bin", 0x000001, 0x40000, 0x6009333e )
	ROM_LOAD32_BYTE("d94-16.bin", 0x000002, 0x40000, 0xc6dbc9c8 )
	ROM_LOAD32_BYTE("d94-15.bin", 0x000003, 0x40000, 0x187c85ab )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d94-09.bin", 0x000000, 0x200000, 0x425e6bec )
	ROM_LOAD16_BYTE("d94-08.bin", 0x400000, 0x200000, 0xbd909caf )
	ROM_LOAD16_BYTE("d94-07.bin", 0x800000, 0x200000, 0xc8c95e49 )
	ROM_LOAD16_BYTE("d94-06.bin", 0x000001, 0x200000, 0x0ed1df55 )
	ROM_LOAD16_BYTE("d94-05.bin", 0x400001, 0x200000, 0x121c8542 )
	ROM_LOAD16_BYTE("d94-04.bin", 0x800001, 0x200000, 0x24958b50 )
	ROM_LOAD       ("d94-03.bin", 0x1200000, 0x200000, 0x95e32072 )
	ROM_LOAD       ("d94-02.bin", 0x1400000, 0x200000, 0xf460b9ac )
	ROM_LOAD       ("d94-01.bin", 0x1600000, 0x200000, 0x410ffccd )
	ROM_FILL       (              0xc00000, 0x600000,0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d94-14.bin", 0x000000, 0x100000, 0xb8ba5761 )
	ROM_LOAD16_BYTE("d94-13.bin", 0x000001, 0x100000, 0xcafc68ce )
	ROM_LOAD       ("d94-12.bin", 0x300000, 0x100000, 0x47064189 )
	ROM_FILL       (              0x200000, 0x100000,0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d94-19.bin", 0x100000, 0x40000, 0xc93dbcf4 )
	ROM_LOAD16_BYTE("d94-20.bin", 0x100001, 0x40000, 0xf232bf64 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d94-10.bin", 0x000000, 0x200000, 0xa22563ae )
	ROM_LOAD16_BYTE("d94-11.bin", 0xc00000, 0x200000, 0x61ed83fa )
ROM_END

ROM_START( hthro95u )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("d94-18.bin", 0x000000, 0x40000, 0xb92681c3 )
	ROM_LOAD32_BYTE("d94-17.bin", 0x000001, 0x40000, 0x6009333e )
	ROM_LOAD32_BYTE("d94-16.bin", 0x000002, 0x40000, 0xc6dbc9c8 )
	ROM_LOAD32_BYTE("d94-21.bin", 0x000003, 0x40000, 0x8175d411 )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d94-09.bin", 0x000000, 0x200000, 0x425e6bec )
	ROM_LOAD16_BYTE("d94-08.bin", 0x400000, 0x200000, 0xbd909caf )
	ROM_LOAD16_BYTE("d94-07.bin", 0x800000, 0x200000, 0xc8c95e49 )
	ROM_LOAD16_BYTE("d94-06.bin", 0x000001, 0x200000, 0x0ed1df55 )
	ROM_LOAD16_BYTE("d94-05.bin", 0x400001, 0x200000, 0x121c8542 )
	ROM_LOAD16_BYTE("d94-04.bin", 0x800001, 0x200000, 0x24958b50 )
	ROM_LOAD       ("d94-03.bin", 0x1200000, 0x200000, 0x95e32072 )
	ROM_LOAD       ("d94-02.bin", 0x1400000, 0x200000, 0xf460b9ac )
	ROM_LOAD       ("d94-01.bin", 0x1600000, 0x200000, 0x410ffccd )
	ROM_FILL       (              0xc00000, 0x600000,0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d94-14.bin", 0x000000, 0x100000, 0xb8ba5761 )
	ROM_LOAD16_BYTE("d94-13.bin", 0x000001, 0x100000, 0xcafc68ce )
	ROM_LOAD       ("d94-12.bin", 0x300000, 0x100000, 0x47064189 )
	ROM_FILL       (              0x200000, 0x100000,0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d94-19.bin", 0x100000, 0x40000, 0xc93dbcf4 )
	ROM_LOAD16_BYTE("d94-20.bin", 0x100001, 0x40000, 0xf232bf64 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d94-10.bin", 0x000000, 0x200000, 0xa22563ae )
	ROM_LOAD16_BYTE("d94-11.bin", 0xc00000, 0x200000, 0x61ed83fa )
ROM_END

ROM_START( qtheater )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
	ROM_LOAD32_BYTE("d95-12.20", 0x000000, 0x80000, 0xfcee76ee )
	ROM_LOAD32_BYTE("d95-11.19", 0x000001, 0x80000, 0xb3c2b8d5 )
	ROM_LOAD32_BYTE("d95-10.18", 0x000002, 0x80000, 0x85236e40 )
	ROM_LOAD32_BYTE("d95-09.17", 0x000003, 0x80000, 0xf456519c )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("d95-02.12", 0x000000, 0x200000, 0x74ce6f3e )
  	ROM_LOAD16_BYTE("d95-01.8",  0x000001, 0x200000, 0x141beb7d )
	ROM_FILL       (             0x400000, 0x400000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("d95-06.47", 0x000000, 0x200000, 0x70a0dcbb )
	ROM_LOAD16_BYTE("d95-05.45", 0x000001, 0x200000, 0x1a1a852b )
	ROM_FILL       (             0x400000, 0x400000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("d95-07.32", 0x100000, 0x40000, 0xd82cdfe2 )
	ROM_LOAD16_BYTE("d95-08.33", 0x100001, 0x40000, 0x01c23354 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("d95-03.38", 0x000000, 0x200000, 0x4149ea67 )
	ROM_LOAD16_BYTE("d95-04.41", 0x400000, 0x200000, 0xe9049d16 )
ROM_END

ROM_START( akkanvdr )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e06.14", 0x000000, 0x20000, 0x71ba7f00 )
	ROM_LOAD32_BYTE("e06.13", 0x000001, 0x20000, 0xf506ba4b )
	ROM_LOAD32_BYTE("e06.12", 0x000002, 0x20000, 0x06cbd72b )
	ROM_LOAD32_BYTE("e06.11", 0x000003, 0x20000, 0x3fe550b9 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e06.03", 0x000000, 0x100000, 0xa24070ef )
	ROM_LOAD16_BYTE("e06.02", 0x000001, 0x100000, 0x8f646dea )
	ROM_LOAD       ("e06.01", 0x300000, 0x100000, 0x51721b15 )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e06.08", 0x000000, 0x100000, 0x72ae2fbf )
	ROM_LOAD16_BYTE("e06.07", 0x000001, 0x100000, 0x4b02e8f5 )
	ROM_LOAD       ("e06.06", 0x300000, 0x100000, 0x9380db3c )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e06.09", 0x100000, 0x40000, 0x9bcafc87 )
	ROM_LOAD16_BYTE("e06.10", 0x100001, 0x40000, 0xb752b61f )

	ROM_REGION16_BE( 0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e06.04", 0x000000, 0x200000, 0x1dac29df )
 	ROM_LOAD16_BYTE("e06.05", 0x400000, 0x200000, 0xf370ff15 )
ROM_END

ROM_START( elvactr )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("ea2w.b63",  0x000003, 0x80000, 0xcd97182b )

	ROM_REGION(0x800000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

ROM_START( elvactrj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("e02-09.17", 0x000003, 0x80000, 0x23997907 )

	ROM_REGION(0x800000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

ROM_START( elvact2u )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("ea2.b63",   0x000003, 0x80000, 0xba9028bd )

	ROM_REGION(0x800000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

ROM_START( twinqix )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("mpr0-3.b60", 0x000000, 0x40000, 0x1a63d0de )
	ROM_LOAD32_BYTE("mpr0-2.b61", 0x000001, 0x40000, 0x45a70987 )
	ROM_LOAD32_BYTE("mpr0-1.b62", 0x000002, 0x40000, 0x531f9447 )
	ROM_LOAD32_BYTE("mpr0-0.b63", 0x000003, 0x40000, 0xa4c44c11 )

	ROM_REGION(0x200000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("obj0-0.a08", 0x000000, 0x080000, 0xc6ea845c )
	ROM_LOAD16_BYTE("obj0-1.a20", 0x000001, 0x080000, 0x8c12b7fb )
	ROM_FILL       (              0x100000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
  	ROM_LOAD32_BYTE("scr0-0.b07",  0x000000, 0x080000, 0x9a1b9b34 )
	ROM_LOAD32_BYTE("scr0-1.b06",  0x000002, 0x080000, 0xe9bef879 )
	ROM_LOAD32_BYTE("scr0-2.b05",  0x000001, 0x080000, 0xcac6854b )
	ROM_LOAD32_BYTE("scr0-3.b04",  0x000003, 0x080000, 0xce063034 )
	ROM_LOAD16_BYTE("scr0-4.b03",  0x300000, 0x080000, 0xd32280fe )
	ROM_LOAD16_BYTE("scr0-5.b02",  0x300001, 0x080000, 0xfdd1a85b )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* sound CPU */
	ROM_LOAD16_BYTE("spr0-1.b66", 0x100000, 0x40000, 0x4b20e99d )
	ROM_LOAD16_BYTE("spr0-0.b65", 0x100001, 0x40000, 0x2569eb30 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("snd-0.b43", 0xe00000, 0x80000, 0xad5405a9 )
	ROM_LOAD16_BYTE("snd-14.b10",0x000000, 0x80000, 0x26312451 )
	ROM_LOAD16_BYTE("snd-1.b44", 0xf00000, 0x80000, 0x274864af )
	ROM_LOAD16_BYTE("snd-15.b11",0x100000, 0x80000, 0x2edaa9dc )
ROM_END

ROM_START( quizhuhu )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
	ROM_LOAD32_BYTE("e08-16.20", 0x000000, 0x80000, 0xfaa8f373 )
	ROM_LOAD32_BYTE("e08-15.19", 0x000001, 0x80000, 0x23acf231 )
	ROM_LOAD32_BYTE("e08-14.18", 0x000002, 0x80000, 0x33a4951d )
	ROM_LOAD32_BYTE("e08-13.17", 0x000003, 0x80000, 0x0936fd2a )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e08-06.12", 0x000000, 0x200000, 0x8dadc9ac )
  	ROM_LOAD16_BYTE("e08-04.8",  0x000001, 0x200000, 0x5423721d )
	ROM_LOAD16_BYTE("e08-05.11", 0x400000, 0x100000, 0x79d2e516 )
  	ROM_LOAD16_BYTE("e08-03.7",  0x400001, 0x100000, 0x07b9ab6a )
	ROM_LOAD       ("e08-02.4",  0x900000, 0x200000, 0xd89eb067 )
	ROM_LOAD       ("e08-01.3",  0xb00000, 0x100000, 0x90223c06 )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e08-12.47", 0x000000, 0x100000, 0x6c711d36 )
	ROM_LOAD16_BYTE("e08-11.45", 0x000001, 0x100000, 0x56775a60 )
	ROM_LOAD       ("e08-10.43", 0x300000, 0x100000, 0x60abc71b )
	ROM_FILL       (             0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e08-18.32", 0x100000, 0x20000, 0xe695497e )
	ROM_LOAD16_BYTE("e08-17.33", 0x100001, 0x20000, 0xfafc7e4e )

	ROM_REGION16_BE(0xe00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e08-07.38", 0x000000, 0x200000, 0xc05dc85b )
	ROM_LOAD16_BYTE("e08-08.39", 0x400000, 0x200000, 0x3eb94a99 )
	ROM_LOAD16_BYTE("e08-09.41", 0x800000, 0x200000, 0x200b26ee )
ROM_END

ROM_START( pbobble2 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
	ROM_LOAD32_BYTE("e10-11.rom", 0x000000, 0x40000, 0xb82f81da )
	ROM_LOAD32_BYTE("e10-10.rom", 0x000001, 0x40000, 0xf432267a )
	ROM_LOAD32_BYTE("e10-09.rom", 0x000002, 0x40000, 0xe0b1b599 )
	ROM_LOAD32_BYTE("e10-15.rom", 0x000003, 0x40000, 0xa2c0a268 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e10-02.rom", 0x000000, 0x100000, 0xc0564490 )
  	ROM_LOAD16_BYTE("e10-01.rom", 0x000001, 0x100000, 0x8c26ff49 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e10-07.rom", 0x000000, 0x100000, 0xdcb3c29b )
	ROM_LOAD16_BYTE("e10-06.rom", 0x000001, 0x100000, 0x1b0f20e2 )
	ROM_LOAD       ("e10-05.rom", 0x300000, 0x100000, 0x81266151 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e10-12.rom", 0x100000, 0x40000, 0xb92dc8ad )
	ROM_LOAD16_BYTE("e10-13.rom", 0x100001, 0x40000, 0x87842c13 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e10-03.rom", 0x000000, 0x200000, 0x46d68ac8 )
	ROM_LOAD16_BYTE("e10-04.rom", 0x400000, 0x200000, 0x5c0862a6 )
ROM_END

ROM_START( pbobbl2j )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
	ROM_LOAD32_BYTE("e10-11.rom", 0x000000, 0x40000, 0xb82f81da )
	ROM_LOAD32_BYTE("e10-10.rom", 0x000001, 0x40000, 0xf432267a )
	ROM_LOAD32_BYTE("e10-09.rom", 0x000002, 0x40000, 0xe0b1b599 )
	ROM_LOAD32_BYTE("e10-08.17",  0x000003, 0x40000, 0x4ccec344 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e10-02.rom", 0x000000, 0x100000, 0xc0564490 )
  	ROM_LOAD16_BYTE("e10-01.rom", 0x000001, 0x100000, 0x8c26ff49 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e10-07.rom", 0x000000, 0x100000, 0xdcb3c29b )
	ROM_LOAD16_BYTE("e10-06.rom", 0x000001, 0x100000, 0x1b0f20e2 )
	ROM_LOAD       ("e10-05.rom", 0x300000, 0x100000, 0x81266151 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e10-12.rom", 0x100000, 0x40000, 0xb92dc8ad )
	ROM_LOAD16_BYTE("e10-13.rom", 0x100001, 0x40000, 0x87842c13 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e10-03.rom", 0x000000, 0x200000, 0x46d68ac8 )
	ROM_LOAD16_BYTE("e10-04.rom", 0x400000, 0x200000, 0x5c0862a6 )
ROM_END

ROM_START( pbobbl2u )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
	ROM_LOAD32_BYTE("e10-11.rom", 0x000000, 0x40000, 0xb82f81da )
	ROM_LOAD32_BYTE("e10-10.rom", 0x000001, 0x40000, 0xf432267a )
	ROM_LOAD32_BYTE("e10-09.rom", 0x000002, 0x40000, 0xe0b1b599 )
	ROM_LOAD32_BYTE("e10-14.bin", 0x000003, 0x40000, 0xd5c792fe )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e10-02.rom", 0x000000, 0x100000, 0xc0564490 )
  	ROM_LOAD16_BYTE("e10-01.rom", 0x000001, 0x100000, 0x8c26ff49 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e10-07.rom", 0x000000, 0x100000, 0xdcb3c29b )
	ROM_LOAD16_BYTE("e10-06.rom", 0x000001, 0x100000, 0x1b0f20e2 )
	ROM_LOAD       ("e10-05.rom", 0x300000, 0x100000, 0x81266151 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e10-12.rom", 0x100000, 0x40000, 0xb92dc8ad )
	ROM_LOAD16_BYTE("e10-13.rom", 0x100001, 0x40000, 0x87842c13 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e10-03.rom", 0x000000, 0x200000, 0x46d68ac8 )
	ROM_LOAD16_BYTE("e10-04.rom", 0x400000, 0x200000, 0x5c0862a6 )
ROM_END

ROM_START( pbobbl2x )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e10.29", 0x000000, 0x40000, 0xf1e9ad3f )
	ROM_LOAD32_BYTE("e10.28", 0x000001, 0x40000, 0x412a3602 )
	ROM_LOAD32_BYTE("e10.27", 0x000002, 0x40000, 0x88cc0b5c )
	ROM_LOAD32_BYTE("e10.26", 0x000003, 0x40000, 0xa5c24047 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e10-02.rom", 0x000000, 0x100000, 0xc0564490 )
  	ROM_LOAD16_BYTE("e10-01.rom", 0x000001, 0x100000, 0x8c26ff49 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e10-07.rom", 0x000000, 0x100000, 0xdcb3c29b )
	ROM_LOAD16_BYTE("e10-06.rom", 0x000001, 0x100000, 0x1b0f20e2 )
	ROM_LOAD       ("e10-05.rom", 0x300000, 0x100000, 0x81266151 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e10.30", 0x100000, 0x40000, 0xbb090c1e )
	ROM_LOAD16_BYTE("e10.31", 0x100001, 0x40000, 0xf4b88d65 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e10-03.rom", 0x000000, 0x200000, 0x46d68ac8 )
	ROM_LOAD16_BYTE("e10-04.rom", 0x400000, 0x200000, 0x5c0862a6 )
ROM_END

ROM_START( gekirido )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e11-12.bin", 0x000000, 0x40000, 0x6a7aaacf )
	ROM_LOAD32_BYTE("e11-11.bin", 0x000001, 0x40000, 0x2284a08e )
	ROM_LOAD32_BYTE("e11-10.bin", 0x000002, 0x40000, 0x8795e6ba )
	ROM_LOAD32_BYTE("e11-09.bin", 0x000003, 0x40000, 0xb4e17ef4 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e11-03.bin", 0x000000, 0x200000, 0xf73877c5 )
  	ROM_LOAD16_BYTE("e11-02.bin", 0x000001, 0x200000, 0x5722a83b )
	ROM_LOAD       ("e11-01.bin", 0x600000, 0x200000, 0xc2cd1069 )
 	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e11-08.bin", 0x000000, 0x200000, 0x907f69d3 )
	ROM_LOAD16_BYTE("e11-07.bin", 0x000001, 0x200000, 0xef018607 )
	ROM_LOAD       ("e11-06.bin", 0x600000, 0x200000, 0x200ce305 )
 	ROM_FILL       (              0x400000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e11-13.bin", 0x100000, 0x20000, 0x51a11ff7 )
	ROM_LOAD16_BYTE("e11-14.bin", 0x100001, 0x20000, 0xdce2ba91 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e11-04.bin", 0x000000, 0x200000, 0xe0ff4fb1 )
	ROM_LOAD16_BYTE("e11-05.bin", 0xc00000, 0x200000, 0xa4d08cf1 )
ROM_END

ROM_START( ktiger2 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e15-14.bin", 0x000000, 0x40000, 0xb527b733 )
	ROM_LOAD32_BYTE("e15-13.bin", 0x000001, 0x40000, 0x0f03daf7 )
	ROM_LOAD32_BYTE("e15-12.bin", 0x000002, 0x40000, 0x59d832f2 )
	ROM_LOAD32_BYTE("e15-11.bin", 0x000003, 0x40000, 0xa706a286 )

	ROM_REGION(0xc00000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e15-04.bin", 0x000000, 0x200000, 0x6ea8d9bd )
  	ROM_LOAD16_BYTE("e15-02.bin", 0x000001, 0x200000, 0x2ea7f2bd )
	ROM_LOAD16_BYTE("e15-03.bin", 0x400000, 0x100000, 0xbe45a52f )
  	ROM_LOAD16_BYTE("e15-01.bin", 0x400001, 0x100000, 0x85421aac )
 	ROM_FILL       (              0x600000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e15-10.bin", 0x000000, 0x200000, 0xd8c96b00 )
	ROM_LOAD16_BYTE("e15-08.bin", 0x000001, 0x200000, 0x4bdb2bf3 )
	ROM_LOAD16_BYTE("e15-09.bin", 0x400000, 0x100000, 0x07c29f60 )
	ROM_LOAD16_BYTE("e15-07.bin", 0x400001, 0x100000, 0x8164f7ee )
	ROM_FILL       (              0x600000, 0x600000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e15-15.bin", 0x100000, 0x20000, 0x22126dfb )
	ROM_LOAD16_BYTE("e15-16.bin", 0x100001, 0x20000, 0xf8b58ea0 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e15-05.bin", 0x000000, 0x200000, 0x3e5da5f6 )
	ROM_LOAD16_BYTE("e15-06.bin", 0x400000, 0x200000, 0xb182a3e1 )
ROM_END

ROM_START( bubblem )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e21-21.rom", 0x000000, 0x080000, 0xcac4169c )
	ROM_LOAD32_BYTE("e21-20.rom", 0x000001, 0x080000, 0x7727c673 )
	ROM_LOAD32_BYTE("e21-19.rom", 0x000002, 0x080000, 0xbe0b907d )
	ROM_LOAD32_BYTE("e21-18.rom", 0x000003, 0x080000, 0xd14e313a )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e21-02.rom", 0x000000, 0x200000, 0xb7cb9232 )
	ROM_LOAD16_BYTE("e21-01.rom", 0x000001, 0x200000, 0xa11f2f99 )
	ROM_FILL       (              0x400000, 0x400000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e21-07.rom", 0x000000, 0x100000, 0x7789bf7c )
	ROM_LOAD16_BYTE("e21-06.rom", 0x000001, 0x100000, 0x997fc0d7 )
	ROM_LOAD       ("e21-05.rom", 0x300000, 0x100000, 0x07eab58f )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* Sound CPU */
	ROM_LOAD16_BYTE("e21-12.rom", 0x100000, 0x40000, 0x34093de1 )
	ROM_LOAD16_BYTE("e21-13.rom", 0x100001, 0x40000, 0x9e9ec437 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e21-03.rom", 0x000000, 0x200000, 0x54c5f83d )
	ROM_LOAD16_BYTE("e21-04.rom", 0x400000, 0x200000, 0xe5af2a2d )
ROM_END

ROM_START( bubblemj )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e21-11.20", 0x000000, 0x080000, 0xdf0eeae4 )
	ROM_LOAD32_BYTE("e21-10.19", 0x000001, 0x080000, 0xcdfb58f6 )
	ROM_LOAD32_BYTE("e21-09.18", 0x000002, 0x080000, 0x6c305f17 )
	ROM_LOAD32_BYTE("e21-08.17", 0x000003, 0x080000, 0x27381ae2 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e21-02.rom", 0x000000, 0x200000, 0xb7cb9232 )
	ROM_LOAD16_BYTE("e21-01.rom", 0x000001, 0x200000, 0xa11f2f99 )
	ROM_FILL       (              0x400000, 0x400000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e21-07.rom", 0x000000, 0x100000, 0x7789bf7c )
	ROM_LOAD16_BYTE("e21-06.rom", 0x000001, 0x100000, 0x997fc0d7 )
	ROM_LOAD       ("e21-05.rom", 0x300000, 0x100000, 0x07eab58f )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* Sound CPU */
	ROM_LOAD16_BYTE("e21-12.rom", 0x100000, 0x40000, 0x34093de1 )
	ROM_LOAD16_BYTE("e21-13.rom", 0x100001, 0x40000, 0x9e9ec437 )

	ROM_REGION16_BE(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e21-03.rom", 0x000000, 0x200000, 0x54c5f83d )
	ROM_LOAD16_BYTE("e21-04.rom", 0x400000, 0x200000, 0xe5af2a2d )
ROM_END

ROM_START( cleopatr )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e28-10.bin", 0x000000, 0x80000, 0x013fbc39 )
	ROM_LOAD32_BYTE("e28-09.bin", 0x000001, 0x80000, 0x1c48a1f9 )
	ROM_LOAD32_BYTE("e28-08.bin", 0x000002, 0x80000, 0x7564f199 )
	ROM_LOAD32_BYTE("e28-07.bin", 0x000003, 0x80000, 0xa507797b )

	ROM_REGION(0x200000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e28-02.bin", 0x000000, 0x080000, 0xb20d47cb )
	ROM_LOAD16_BYTE("e28-01.bin", 0x000001, 0x080000, 0x4440e659 )
	ROM_FILL       (              0x100000, 0x100000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e28-06.bin", 0x000000, 0x100000, 0x21d0c454 )
	ROM_LOAD16_BYTE("e28-05.bin", 0x000001, 0x100000, 0x2870dbbc )
	ROM_LOAD       ("e28-04.bin", 0x300000, 0x100000, 0x57aef029 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* Sound CPU */
	ROM_LOAD16_BYTE("e28-11.bin", 0x100000, 0x20000, 0x01a06950 )
	ROM_LOAD16_BYTE("e28-12.bin", 0x100001, 0x20000, 0xdc19260f )

	ROM_REGION16_BE(0x600000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e28-03.bin", 0x000000, 0x200000, 0x15c7989d )
ROM_END

ROM_START( pbobble3 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("pb3_12.rom", 0x000000, 0x80000, 0x9eb19a00 )
	ROM_LOAD32_BYTE("pb3_11.rom", 0x000001, 0x80000, 0xe54ada97 )
	ROM_LOAD32_BYTE("pb3_10.rom", 0x000002, 0x80000, 0x1502a122 )
	ROM_LOAD32_BYTE("pb3_16.rom", 0x000003, 0x80000, 0xaac293da )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("pb3_02.rom", 0x000000, 0x100000, 0x437391d3 )
	ROM_LOAD16_BYTE("pb3_01.rom", 0x000001, 0x100000, 0x52547c77 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("pb3_08.rom", 0x000000, 0x100000, 0x7040a3d5 )
	ROM_LOAD16_BYTE("pb3_07.rom", 0x000001, 0x100000, 0xfca2ea9b )
	ROM_LOAD       ("pb3_06.rom", 0x300000, 0x100000, 0xc16184f8 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("pb3_13.rom", 0x100000, 0x40000, 0x1ef551ef )
	ROM_LOAD16_BYTE("pb3_14.rom", 0x100001, 0x40000, 0x7ee7e688 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("pb3_05.rom", 0x000000, 0x200000, 0xe33c1234 )
	ROM_LOAD16_BYTE("pb3_04.rom", 0x400000, 0x200000, 0xd1f42457 )
	ROM_LOAD16_BYTE("pb3_03.rom", 0xc00000, 0x200000, 0xa4371658 )
ROM_END

ROM_START( pbobbl3u )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("pb3_12.rom", 0x000000, 0x80000, 0x9eb19a00 )
	ROM_LOAD32_BYTE("pb3_11.rom", 0x000001, 0x80000, 0xe54ada97 )
	ROM_LOAD32_BYTE("pb3_10.rom", 0x000002, 0x80000, 0x1502a122 )
	ROM_LOAD32_BYTE("e29_09u.bin", 0x000003, 0x80000, 0xddc5a34c )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("pb3_02.rom", 0x000000, 0x100000, 0x437391d3 )
	ROM_LOAD16_BYTE("pb3_01.rom", 0x000001, 0x100000, 0x52547c77 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("pb3_08.rom", 0x000000, 0x100000, 0x7040a3d5 )
	ROM_LOAD16_BYTE("pb3_07.rom", 0x000001, 0x100000, 0xfca2ea9b )
	ROM_LOAD       ("pb3_06.rom", 0x300000, 0x100000, 0xc16184f8 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("pb3_13.rom", 0x100000, 0x40000, 0x1ef551ef )
	ROM_LOAD16_BYTE("pb3_14.rom", 0x100001, 0x40000, 0x7ee7e688 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("pb3_05.rom", 0x000000, 0x200000, 0xe33c1234 )
	ROM_LOAD16_BYTE("pb3_04.rom", 0x400000, 0x200000, 0xd1f42457 )
	ROM_LOAD16_BYTE("pb3_03.rom", 0xc00000, 0x200000, 0xa4371658 )
ROM_END

ROM_START( pbobbl3j )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("pb3_12.rom", 0x000000, 0x80000, 0x9eb19a00 )
	ROM_LOAD32_BYTE("pb3_11.rom", 0x000001, 0x80000, 0xe54ada97 )
	ROM_LOAD32_BYTE("pb3_10.rom", 0x000002, 0x80000, 0x1502a122 )
	ROM_LOAD32_BYTE("e29_09.bin", 0x000003, 0x80000, 0x44ccf2f6 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("pb3_02.rom", 0x000000, 0x100000, 0x437391d3 )
	ROM_LOAD16_BYTE("pb3_01.rom", 0x000001, 0x100000, 0x52547c77 )
	ROM_FILL       (              0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("pb3_08.rom", 0x000000, 0x100000, 0x7040a3d5 )
	ROM_LOAD16_BYTE("pb3_07.rom", 0x000001, 0x100000, 0xfca2ea9b )
	ROM_LOAD       ("pb3_06.rom", 0x300000, 0x100000, 0xc16184f8 )
	ROM_FILL       (              0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("pb3_13.rom", 0x100000, 0x40000, 0x1ef551ef )
	ROM_LOAD16_BYTE("pb3_14.rom", 0x100001, 0x40000, 0x7ee7e688 )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("pb3_05.rom", 0x000000, 0x200000, 0xe33c1234 )
	ROM_LOAD16_BYTE("pb3_04.rom", 0x400000, 0x200000, 0xd1f42457 )
	ROM_LOAD16_BYTE("pb3_03.rom", 0xc00000, 0x200000, 0xa4371658 )
ROM_END

ROM_START( arkretrn )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e36.11", 0x000000, 0x040000, 0xb50cfb92 )
	ROM_LOAD32_BYTE("e36.10", 0x000001, 0x040000, 0xc940dba1 )
	ROM_LOAD32_BYTE("e36.09", 0x000002, 0x040000, 0xf16985e0 )
	ROM_LOAD32_BYTE("e36.08", 0x000003, 0x040000, 0xaa699e1b )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* Sound CPU */
	ROM_LOAD16_BYTE("e36.12", 0x100000, 0x40000, 0x3bae39be )
	ROM_LOAD16_BYTE("e36.13", 0x100001, 0x40000, 0x94448e82 )

	ROM_REGION(0x100000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e36.03", 0x000000, 0x040000, 0x1ea8558b )
	ROM_LOAD16_BYTE("e36.02", 0x000001, 0x040000, 0x694eda31 )
	ROM_LOAD       ("e36.01", 0x0c0000, 0x040000, 0x54b9b2cd )
	ROM_FILL       (          0x080000, 0x040000, 0 )

	ROM_REGION(0x200000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e36.07", 0x000000, 0x080000, 0x266bf1c1 )
	ROM_LOAD16_BYTE("e36.06", 0x000001, 0x080000, 0x110ab729 )
	ROM_LOAD       ("e36.05", 0x180000, 0x080000, 0xdb18bce2 )
	ROM_FILL       (          0x100000, 0x080000, 0 )

	ROM_REGION16_BE(0x600000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e36.04", 0x000000, 0x200000, 0x2250959b )
ROM_END

ROM_START( kirameki )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e44-19.20", 0x000000, 0x80000, 0x2f3b239a )
	ROM_LOAD32_BYTE("e44-18.19", 0x000001, 0x80000, 0x3137c8bc )
	ROM_LOAD32_BYTE("e44-17.18", 0x000002, 0x80000, 0x5905cd20 )
	ROM_LOAD32_BYTE("e44-16.17", 0x000003, 0x80000, 0x5e9ac3fd )

	ROM_REGION(0x1800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e44-06.12", 0x0000000, 0x400000, 0x80526d58 )
	ROM_LOAD16_BYTE("e44-04.8",  0x0000001, 0x400000, 0x28c7c295 )
	ROM_LOAD16_BYTE("e44-05.11", 0x0800000, 0x200000, 0x0fbc2b26 )
	ROM_LOAD16_BYTE("e44-03.7",  0x0800001, 0x200000, 0xd9e63fb0 )
	ROM_LOAD       ("e44-02.4",  0x1200000, 0x400000, 0x5481efde )
	ROM_LOAD       ("e44-01.3",  0x1600000, 0x200000, 0xc4bdf727 )
	ROM_FILL       (             0x0c00000, 0x600000, 0 )

	ROM_REGION(0xc00000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e44-14.47", 0x000000, 0x200000, 0x4597c034 )
	ROM_LOAD16_BYTE("e44-12.45", 0x000001, 0x200000, 0x7160a181 )
	ROM_LOAD16_BYTE("e44-13.46", 0x400000, 0x100000, 0x0b016c4e )
	ROM_LOAD16_BYTE("e44-11.44", 0x400001, 0x100000, 0x34d84143 )
	ROM_LOAD       ("e44-10.43", 0x900000, 0x200000, 0x326f738e )
	ROM_LOAD       ("e44-09.42", 0xb00000, 0x100000, 0xa8e68eb7 )
	ROM_FILL       (             0x600000, 0x300000, 0 )

	ROM_REGION(0x200000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e44-21.52", 0x100001, 0x80000, 0xd31b94b8 )
	ROM_LOAD16_BYTE("e44-20.51", 0x100000, 0x80000, 0x4df7e051 )

	ROM_REGION(0x200000, REGION_USER1, 0)	/* 68000 unknown code!? */
	ROM_LOAD("e44-15.53", 0x000000, 0x200000, 0x5043b608 )

	ROM_REGION(0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD("e44-07.38", 0x000000, 0x400000, 0xa9e28544 )
	ROM_LOAD("e44-08.39", 0x400000, 0x400000, 0x33ba3037 )
ROM_END

ROM_START( puchicar )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e46.16", 0x000000, 0x80000, 0xcf2accdf )
	ROM_LOAD32_BYTE("e46.15", 0x000001, 0x80000, 0xc32c6ed8 )
	ROM_LOAD32_BYTE("e46.14", 0x000002, 0x80000, 0xa154c300 )
	ROM_LOAD32_BYTE("e46.13", 0x000003, 0x80000, 0x59fbdf3a )

	ROM_REGION(0x1000000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e46.06", 0x000000, 0x200000, 0xb74336f2 )
	ROM_LOAD16_BYTE("e46.04", 0x000001, 0x200000, 0x463ecc4c )
	ROM_LOAD16_BYTE("e46.05", 0x400000, 0x200000, 0x83a32eee )
	ROM_LOAD16_BYTE("e46.03", 0x400001, 0x200000, 0xeb768193 )
	ROM_LOAD       ("e46.02", 0xc00000, 0x200000, 0xfb046018 )
	ROM_LOAD       ("e46.01", 0xe00000, 0x200000, 0x34fc2103 )
	ROM_FILL       (          0x800000, 0x400000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e46.12", 0x000000, 0x100000, 0x1f3a9851 )
	ROM_LOAD16_BYTE("e46.11", 0x000001, 0x100000, 0xe9f10bf1 )
	ROM_LOAD       ("e46.10", 0x300000, 0x100000, 0x1999b76a )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e46.19", 0x100000, 0x40000, 0x2624eba0 )
	ROM_LOAD16_BYTE("e46.20", 0x100001, 0x40000, 0x065e934f )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e46.07", 0xc00000, 0x200000, 0xf20af91e )
	ROM_LOAD16_BYTE("e46.08", 0x800000, 0x200000, 0xf7f96e1d )
	ROM_LOAD16_BYTE("e46.09", 0x000000, 0x200000, 0x824135f8 )
ROM_END

ROM_START( pbobble4 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e49.12", 0x000000, 0x80000, 0xfffea203 )
	ROM_LOAD32_BYTE("e49.11", 0x000001, 0x80000, 0xbf69a087 )
	ROM_LOAD32_BYTE("e49.10", 0x000002, 0x80000, 0x0307460b )
	ROM_LOAD32_BYTE("e49.16", 0x000003, 0x80000, 0x0a021624 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e49.02", 0x000000, 0x100000, 0xc7d2f40b )
	ROM_LOAD16_BYTE("e49.01", 0x000001, 0x100000, 0xa3dd5f85 )
	ROM_FILL       (          0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e49.08", 0x000000, 0x100000, 0x87408106 )
	ROM_LOAD16_BYTE("e49.07", 0x000001, 0x100000, 0x1e1e8e1c )
	ROM_LOAD       ("e49.06", 0x300000, 0x100000, 0xec85f7ce )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("e49.13", 0x100000, 0x40000, 0x83536f7f )
	ROM_LOAD16_BYTE("e49.14", 0x100001, 0x40000, 0x19815bdb )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e49.03", 0xc00000, 0x200000, 0xf64303e0 )
	ROM_LOAD16_BYTE("e49.04", 0x800000, 0x200000, 0x09be229c )
	ROM_LOAD16_BYTE("e49.05", 0x000000, 0x200000, 0x5ce90ee2 )
ROM_END

ROM_START( pbobbl4j )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e49.12", 0x000000, 0x80000, 0xfffea203 )
	ROM_LOAD32_BYTE("e49.11", 0x000001, 0x80000, 0xbf69a087 )
	ROM_LOAD32_BYTE("e49.10", 0x000002, 0x80000, 0x0307460b )
	ROM_LOAD32_BYTE("e49-09.17", 0x000003, 0x80000, 0xe40c7708 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e49.02", 0x000000, 0x100000, 0xc7d2f40b )
	ROM_LOAD16_BYTE("e49.01", 0x000001, 0x100000, 0xa3dd5f85 )
	ROM_FILL       (          0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e49.08", 0x000000, 0x100000, 0x87408106 )
	ROM_LOAD16_BYTE("e49.07", 0x000001, 0x100000, 0x1e1e8e1c )
	ROM_LOAD       ("e49.06", 0x300000, 0x100000, 0xec85f7ce )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("e49.13", 0x100000, 0x40000, 0x83536f7f )
	ROM_LOAD16_BYTE("e49.14", 0x100001, 0x40000, 0x19815bdb )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e49.03", 0xc00000, 0x200000, 0xf64303e0 )
	ROM_LOAD16_BYTE("e49.04", 0x800000, 0x200000, 0x09be229c )
	ROM_LOAD16_BYTE("e49.05", 0x000000, 0x200000, 0x5ce90ee2 )
ROM_END

ROM_START( pbobbl4u )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e49.12", 0x000000, 0x80000, 0xfffea203 )
	ROM_LOAD32_BYTE("e49.11", 0x000001, 0x80000, 0xbf69a087 )
	ROM_LOAD32_BYTE("e49.10", 0x000002, 0x80000, 0x0307460b )
	ROM_LOAD32_BYTE("e49-15.17", 0x000003, 0x80000, 0x7d0526b2 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e49.02", 0x000000, 0x100000, 0xc7d2f40b )
	ROM_LOAD16_BYTE("e49.01", 0x000001, 0x100000, 0xa3dd5f85 )
	ROM_FILL       (          0x200000, 0x200000, 0 )

	ROM_REGION(0x400000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e49.08", 0x000000, 0x100000, 0x87408106 )
	ROM_LOAD16_BYTE("e49.07", 0x000001, 0x100000, 0x1e1e8e1c )
	ROM_LOAD       ("e49.06", 0x300000, 0x100000, 0xec85f7ce )
	ROM_FILL       (          0x200000, 0x100000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 code */
	ROM_LOAD16_BYTE("e49.13", 0x100000, 0x40000, 0x83536f7f )
	ROM_LOAD16_BYTE("e49.14", 0x100001, 0x40000, 0x19815bdb )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e49.03", 0xc00000, 0x200000, 0xf64303e0 )
	ROM_LOAD16_BYTE("e49.04", 0x800000, 0x200000, 0x09be229c )
	ROM_LOAD16_BYTE("e49.05", 0x000000, 0x200000, 0x5ce90ee2 )
ROM_END

ROM_START( popnpop )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e51-12.20", 0x000000, 0x80000, 0x86a237d5 )
	ROM_LOAD32_BYTE("e51-11.19", 0x000001, 0x80000, 0x8a49f34f )
	ROM_LOAD32_BYTE("e51-10.18", 0x000002, 0x80000, 0x4bce68f8 )
	ROM_LOAD32_BYTE("e51-09.17", 0x000003, 0x80000, 0x4a086017 )

	ROM_REGION(0x400000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e51-03.12",0x000000, 0x100000, 0xa24c4607 )
	ROM_LOAD16_BYTE("e51-02.8", 0x000001, 0x100000, 0x6aa8b96c )
	ROM_LOAD       ("e51-01.4", 0x300000, 0x100000, 0x70347e24 )
	ROM_FILL       (            0x200000, 0x100000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e51-08.47", 0x000000, 0x200000, 0x3ad41f02 )
	ROM_LOAD16_BYTE("e51-07.45", 0x000001, 0x200000, 0x95873e46 )
	ROM_LOAD       ("e51-06.43", 0x600000, 0x200000, 0xc240d6c8 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e51-13.32", 0x100000, 0x40000, 0x3b9e3986 )
	ROM_LOAD16_BYTE("e51-14.33", 0x100001, 0x40000, 0x1f9a5015 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e51-04.38", 0x000000, 0x200000, 0x66790f55 )
	ROM_LOAD16_BYTE("e51-05.41", 0xc00000, 0x200000, 0x4d08b26d )
ROM_END

ROM_START( landmakr )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68020 code */
 	ROM_LOAD32_BYTE("e61-13.20", 0x000000, 0x80000, 0x0af756a2 )
	ROM_LOAD32_BYTE("e61-12.19", 0x000001, 0x80000, 0x636b3df9 )
	ROM_LOAD32_BYTE("e61-11.18", 0x000002, 0x80000, 0x279a0ee4 )
	ROM_LOAD32_BYTE("e61-10.17", 0x000003, 0x80000, 0xdaabf2b2 )

	ROM_REGION(0x800000, REGION_GFX1 , ROMREGION_DISPOSE) /* Sprites */
	ROM_LOAD16_BYTE("e61-03.12",0x000000, 0x200000, 0xe8abfc46 )
	ROM_LOAD16_BYTE("e61-02.08",0x000001, 0x200000, 0x1dc4a164 )
	ROM_LOAD       ("e61-01.04",0x600000, 0x200000, 0x6cdd8311 )
	ROM_FILL       (            0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 , ROMREGION_DISPOSE) /* Tiles */
	ROM_LOAD16_BYTE("e61-09.47", 0x000000, 0x200000, 0x6ba29987 )
	ROM_LOAD16_BYTE("e61-08.45", 0x000001, 0x200000, 0x76c98e14 )
	ROM_LOAD       ("e61-07.43", 0x600000, 0x200000, 0x4a57965d )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x140000, REGION_CPU2, 0)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e61-14.32", 0x100000, 0x20000, 0xb905f4a7 )
	ROM_LOAD16_BYTE("e61-15.33", 0x100001, 0x20000, 0x87909869 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e61-04.38", 0x000000, 0x200000, 0xc27aec0c )
	ROM_LOAD16_BYTE("e61-06.40", 0x800000, 0x200000, 0x2e717bfe )
	ROM_LOAD16_BYTE("e61-05.39", 0xc00000, 0x200000, 0x83920d9d )
ROM_END

/******************************************************************************/

static void tile_decode(int uses_5bpp_tiles)
{
	unsigned char lsb,msb;
	unsigned int offset,i;
	UINT8 *gfx = memory_region(REGION_GFX2);
	int size=memory_region_length(REGION_GFX2);
	int half=size/2,data;

	/* Setup ROM formats:

		Some games will only use 4 or 5 bpp sprites, and some only use 4 bpp tiles,
		I don't believe this is software or prom controlled but simply the unused data lines
		are tied low on the game board if unused.  This is backed up by the fact the palette
		indexes are always related to 4 bpp data, even in 6 bpp games.

	*/
	if (uses_5bpp_tiles)
		for (i=half; i<size; i+=2)
			gfx[i+1]=0;

	offset = size/2;
	for (i = size/2+size/4; i<size; i+=2)
	{
		/* Expand 2bits into 4bits format */
		lsb = gfx[i+1];
		msb = gfx[i];

		gfx[offset+0]=((msb&0x02)<<3) | ((msb&0x01)>>0) | ((lsb&0x02)<<4) | ((lsb&0x01)<<1);
		gfx[offset+2]=((msb&0x08)<<1) | ((msb&0x04)>>2) | ((lsb&0x08)<<2) | ((lsb&0x04)>>1);
		gfx[offset+1]=((msb&0x20)>>1) | ((msb&0x10)>>4) | ((lsb&0x20)<<0) | ((lsb&0x10)>>3);
		gfx[offset+3]=((msb&0x80)>>3) | ((msb&0x40)>>6) | ((lsb&0x80)>>2) | ((lsb&0x40)>>5);

		offset+=4;
	}

	gfx = memory_region(REGION_GFX1);
	size=memory_region_length(REGION_GFX1);

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
	state_save_register_UINT32("f3", 0, "coinword", coin_word, 2);
}

#define F3_IRQ_SPEEDUP_1_R(GAME, counter, mem_addr, mask) 		\
static READ32_HANDLER( irq_speedup_r_##GAME )					\
{																\
	if (cpu_get_pc()==counter && (f3_ram[mem_addr]&mask)!=0)	\
		cpu_spinuntil_int();									\
	return f3_ram[mem_addr];									\
}

#define F3_IRQ_SPEEDUP_2_R(GAME, counter, mem_addr, mask) 		\
static READ32_HANDLER( irq_speedup_r_##GAME )					\
{																\
	if (cpu_get_pc()==counter && (f3_ram[mem_addr]&mask)==0)	\
		cpu_spinuntil_int();									\
	return f3_ram[mem_addr];									\
}

#define F3_IRQ_SPEEDUP_3_R(GAME, counter, mem_addr, stack) 		\
static READ32_HANDLER( irq_speedup_r_##GAME )					\
{																\
	int ptr;													\
	if ((cpu_get_sp()&2)==0) ptr=f3_ram[(cpu_get_sp()&0xffff)/4];	\
	else ptr=(((f3_ram[(cpu_get_sp()&0xffff)/4])&0xffff)<<16) | \
	(f3_ram[((cpu_get_sp()&0xffff)/4)+1]>>16); 					\
	if (cpu_get_pc()==counter && ptr==stack)					\
		cpu_spinuntil_int();									\
	return f3_ram[mem_addr];									\
}
//logerror("ptr is %08x, pc is %08x\n",ptr,cpu_get_pc());
F3_IRQ_SPEEDUP_2_R(arabianm, 0x238,    0x8124/4, 0xff000000 )
F3_IRQ_SPEEDUP_1_R(gseeker,  0x43ac,   0xad94/4, 0xffff0000 )
F3_IRQ_SPEEDUP_1_R(gunlock,  0x646,    0x0004/4, 0xffffffff )
F3_IRQ_SPEEDUP_2_R(cupfinal, 0x254,    0x8114/4, 0x0000ff00 )
F3_IRQ_SPEEDUP_2_R(scfinals, 0x25a,    0x8114/4, 0x0000ff00 )
F3_IRQ_SPEEDUP_1_R(lightbr,  0xe0b02,  0x0130/4, 0x000000ff )
F3_IRQ_SPEEDUP_2_R(kaiserkn, 0x256,    0x8110/4, 0xff000000 )
F3_IRQ_SPEEDUP_1_R(spcinvdj, 0x60b4e,  0x0230/4, 0x000000ff )
F3_IRQ_SPEEDUP_2_R(pwrgoal,  0x234,    0x8114/4, 0x0000ff00 )
F3_IRQ_SPEEDUP_2_R(spcinv95, 0x25a,    0x8114/4, 0x0000ff00 )
F3_IRQ_SPEEDUP_2_R(ktiger2,  0x5ba,    0x0570/4, 0x0000ffff )
F3_IRQ_SPEEDUP_1_R(bubsymph, 0xe9a3e,  0x0134/4, 0x000000ff )
F3_IRQ_SPEEDUP_1_R(bubblem,  0x100a62, 0x0134/4, 0x000000ff )
F3_IRQ_SPEEDUP_2_R(cleopatr, 0x254,    0x8114/4, 0x0000ff00 )
F3_IRQ_SPEEDUP_3_R(pbobble2, 0x2c2c,   0x4a50/4, 0x00002900 )
F3_IRQ_SPEEDUP_3_R(pbobbl2x, 0x2c4c,   0x5c58/4, 0x00002920 )
F3_IRQ_SPEEDUP_3_R(pbobble3, 0xf22,    0x5af4/4, 0x0000159e )
F3_IRQ_SPEEDUP_3_R(pbobble4, 0xf8a,    0x58f4/4, 0x000015ee )
F3_IRQ_SPEEDUP_3_R(gekirido, 0x1da8,   0x6bb0/4, 0x00001a90 )
F3_IRQ_SPEEDUP_3_R(dariusg,  0x1d8e,   0x6ba8/4, 0x00001a76 )
F3_IRQ_SPEEDUP_2_R(puchicar, 0x9dc,    0x24d8/4, 0x80000000 )
F3_IRQ_SPEEDUP_2_R(popnpop,  0x9bc,    0x1cf8/4, 0x00008000 )
F3_IRQ_SPEEDUP_2_R(arkretrn, 0x960,    0x2154/4, 0x0000ffff )
F3_IRQ_SPEEDUP_3_R(landmakr, 0x146c,   0x0824/4, 0x00000000 )

static void init_ringrage(void)
{
	f3_game=RINGRAGE;
	tile_decode(0);
}

static void init_arabianm(void)
{
	install_mem_read32_handler(0, 0x408124, 0x408127, irq_speedup_r_arabianm );
	f3_game=ARABIANM;
	tile_decode(1);
}

static void init_ridingf(void)
{
	f3_game=RIDINGF;
	tile_decode(1);
}

static void init_gseeker(void)
{
	install_mem_read32_handler(0, 0x40ad94, 0x40ad97, irq_speedup_r_gseeker );
	f3_game=GSEEKER;
	tile_decode(0);
}

static void init_gunlock(void)
{
	install_mem_read32_handler(0, 0x400004, 0x400007, irq_speedup_r_gunlock );
	f3_game=GUNLOCK;
	tile_decode(1);
}

static void init_elvactr(void)
{
	f3_game=EACTION2;
	tile_decode(1);
}

static void init_cupfinal(void)
{
	install_mem_read32_handler(0, 0x408114, 0x408117, irq_speedup_r_cupfinal );
	f3_game=SCFINALS;
	tile_decode(1);
}

static void init_trstaroj(void)
{
	f3_game=TRSTAR;
	tile_decode(1);
}

static void init_scfinals(void)
{
	data32_t *RAM = (UINT32 *)memory_region(REGION_CPU1);

	/* Doesn't boot without this - eprom related? */
    RAM[0x5af0/4]=0x4e710000|(RAM[0x5af0/4]&0xffff);

	/* Rom checksum error */
	RAM[0xdd0/4]=0x4e750000;

	install_mem_read32_handler(0, 0x408114, 0x408117, irq_speedup_r_scfinals );
	f3_game=SCFINALS;
	tile_decode(1);
}

static void init_lightbr(void)
{
	install_mem_read32_handler(0, 0x400130, 0x400133, irq_speedup_r_lightbr );
	f3_game=LIGHTBR;
	tile_decode(1);
}

static void init_kaiserkn(void)
{
	install_mem_read32_handler(0, 0x408110, 0x408113, irq_speedup_r_kaiserkn );
	f3_game=KAISERKN;
	tile_decode(1);
}

static void init_dariusg(void)
{
	install_mem_read32_handler(0, 0x406ba8, 0x406bab, irq_speedup_r_dariusg );
	f3_game=DARIUSG;
	tile_decode(0);
}

static void init_spcinvdj(void)
{
	install_mem_read32_handler(0, 0x400230, 0x400233, irq_speedup_r_spcinvdj );
	f3_game=SPCINVDX;
	tile_decode(0);
}

static void init_qtheater(void)
{
	f3_game=QTHEATER;
	tile_decode(0);
}

static void init_spcinv95(void)
{
	data32_t *RAM = (UINT32 *)memory_region(REGION_CPU1);

	/* Rogue eprom 0 bit at the end of read commands - would be ignored on real eprom
		(Leading zeros are ignored until first start bit) */
    RAM[0x4ae0/4]=(RAM[0x4ae0/4]&0xffff0000) | 0x00004e71;
    RAM[0x4ae4/4]=(RAM[0x4ae4/4]&0x0000ffff) | 0x4e710000;

	/* Fix ROM checksum error */
	RAM[0xfd8/4]=0x4e714e71;

	install_mem_read32_handler(0, 0x408114, 0x408117, irq_speedup_r_spcinv95 );
	f3_game=SPCINV95;
	tile_decode(1);
}

static void init_gekirido(void)
{
	install_mem_read32_handler(0, 0x406bb0, 0x406bb3, irq_speedup_r_gekirido );
	f3_game=GEKIRIDO;
	tile_decode(1);
}

static void init_ktiger2(void)
{
	install_mem_read32_handler(0, 0x400570, 0x400573, irq_speedup_r_ktiger2 );
	f3_game=KTIGER2;
	tile_decode(0);
}

static void init_bubsymph(void)
{
	install_mem_read32_handler(0, 0x400134, 0x400137, irq_speedup_r_bubsymph );
	f3_game=BUBSYMPH;
	tile_decode(1);
}

static void init_bubblem(void)
{
	install_mem_read32_handler(0, 0x400134, 0x400137, irq_speedup_r_bubblem );
	f3_game=BUBBLEM;
	tile_decode(1);
}

static void init_cleopatr(void)
{
	data32_t *RAM = (UINT32 *)memory_region(REGION_CPU1);

	/* Rogue eprom 0 bit at the end of read commands - would be ignored on real eprom
		(Leading zeros are ignored until first start bit) */
    RAM[0x42d0/4]=0x4e714e71;

	install_mem_read32_handler(0, 0x408114, 0x408117, irq_speedup_r_cleopatr );
	f3_game=CLEOPATR;
	tile_decode(0);
}

static void init_popnpop(void)
{
	install_mem_read32_handler(0, 0x401cf8, 0x401cfb, irq_speedup_r_popnpop );
	f3_game=POPNPOP;
	tile_decode(0);
}

static void init_landmakr(void)
{
	install_mem_read32_handler(0, 0x400824, 0x400827, irq_speedup_r_landmakr );
	f3_game=LANDMAKR;
	tile_decode(0);
}

static void init_pbobble3(void)
{
	data32_t *RAM = (UINT32 *)memory_region(REGION_CPU1);

	/* JSR crash - It tries to execute code in spriteram */
    RAM[0x80a94/4]=0x4E714E71;
    RAM[0x80a98/4]=0x4E710000 | (RAM[0x80a98/4]&0xffff);

	install_mem_read32_handler(0, 0x405af4, 0x405af7, irq_speedup_r_pbobble3 );
	f3_game=PBOBBLE3;
	tile_decode(0);
}

static void init_pbobble4(void)
{
	install_mem_read32_handler(0, 0x4058f4, 0x4058f7, irq_speedup_r_pbobble4 );
	f3_game=PBOBBLE4;
	tile_decode(0);
}

static void init_quizhuhu(void)
{
	f3_game=QUIZHUHU;
	tile_decode(0);
}

static void init_pbobble2(void)
{
	install_mem_read32_handler(0, 0x404a50, 0x404a53, irq_speedup_r_pbobble2 );
	f3_game=PBOBBLE2;
	tile_decode(0);
}

static void init_pbobbl2x(void)
{
	install_mem_read32_handler(0, 0x405c58, 0x405c5b, irq_speedup_r_pbobbl2x );
	f3_game=PBOBBLE2;
	tile_decode(0);
}

static void init_hthero95(void)
{
	install_mem_read32_handler(0, 0x408114, 0x408117, irq_speedup_r_pwrgoal );
	f3_game=HTHERO95;
	tile_decode(0);
}

static void init_kirameki(void)
{
	data32_t *RAM = (UINT32 *)memory_region(REGION_CPU1);

	/* Rogue eprom 0 bit at the end of read commands - would be ignored on real eprom
		(Leading zeros are ignored until first start bit) */
    RAM[0x4aec/4]=(RAM[0x4aec/4]&0xffff0000) | 0x00004e71;
    RAM[0x4af0/4]=(RAM[0x4af0/4]&0x0000ffff) | 0x4e710000;

	f3_game=KIRAMEKI;
	tile_decode(0);
}

static void init_puchicar(void)
{
	install_mem_read32_handler(0, 0x4024d8, 0x4024db, irq_speedup_r_puchicar );
	f3_game=PUCHICAR;
	tile_decode(0);
}

static void init_twinqix(void)
{
	f3_game=TWINQIX;
	tile_decode(0);
}

static void init_arkretrn(void)
{
	install_mem_read32_handler(0, 0x402154, 0x402157, irq_speedup_r_arkretrn );
	f3_game=ARKRETRN;
	tile_decode(0);
}

/******************************************************************************/

GAME( 1992, ringrage, 0       , f3, f3, ringrage, ROT0_16BIT,   "Taito Corporation Japan",   "Ring Rage (World)" )
GAME( 1992, ringragj, ringrage, f3, f3, ringrage, ROT0_16BIT,   "Taito Corporation",         "Ring Rage (Japan)" )
GAME( 1992, ringragu, ringrage, f3, f3, ringrage, ROT0_16BIT,   "Taito America Corporation", "Ring Rage (US)" )
GAME( 1992, arabianm, 0       , f3, f3, arabianm, ROT0_16BIT,   "Taito Corporation Japan",   "Arabian Magic (World)" )
GAME( 1992, arabiamj, arabianm, f3, f3, arabianm, ROT0_16BIT,   "Taito Corporation",         "Arabian Magic (Japan)" )
GAME( 1992, arabiamu, arabianm, f3, f3, arabianm, ROT0_16BIT,   "Taito America Corporation", "Arabian Magic (US)" )
GAME( 1992, ridingf,  0       , f3, f3, ridingf,  ROT0_16BIT,   "Taito Corporation Japan",   "Riding Fight (World)" )
GAME( 1992, ridefgtj, ridingf , f3, f3, ridingf,  ROT0_16BIT,   "Taito Corporation",         "Riding Fight (Japan)" )
GAME( 1992, ridefgtu, ridingf , f3, f3, ridingf,  ROT0_16BIT,   "Taito America Corporation", "Riding Fight (US)" )
GAME( 1992, gseeker,  0       , f3, f3, gseeker,  ROT270_16BIT, "Taito Corporation Japan",   "Grid Seeker: Project Stormhammer (World)" )
GAME( 1992, gseekerj, gseeker,  f3, f3, gseeker,  ROT270_16BIT, "Taito Corporation",         "Grid Seeker: Project Stormhammer (Japan)" )
GAME( 1992, gseekeru, gseeker,  f3, f3, gseeker,  ROT270_16BIT, "Taito America Corporation", "Grid Seeker: Project Stormhammer (US)" )
GAME( 1993, gunlock,  0       , f3, f3, gunlock,  ROT270_16BIT, "Taito Corporation Japan",   "Gunlock (World)" )
GAME( 1993, rayforcj, gunlock , f3, f3, gunlock,  ROT270_16BIT, "Taito Corporation",         "Rayforce (Japan)" )
GAME( 1993, rayforce, gunlock , f3, f3, gunlock,  ROT270_16BIT, "Taito America Corporation", "Rayforce (US)" )
GAME( 1993, scfinals, 0       , f3, f3, scfinals, ROT0_16BIT,   "Taito Corporation Japan",   "Super Cup Finals (World)" )
/* I don't think these really are clones of SCFinals - SCFinals may be a sequel that just shares graphics roms (Different Taito ROM code) */
GAME( 1992, hthero93, scfinals, f3, f3, cupfinal, ROT0_16BIT,   "Taito Corporation",         "Hat Trick Hero '93 (Japan)" )
GAME( 1993, cupfinal, scfinals, f3, f3, cupfinal, ROT0_16BIT,   "Taito Corporation Japan",   "Taito Cup Finals (World)" )
GAME( 1993, trstar,   0       , f3, f3, trstaroj, ROT0_16BIT,   "Taito Corporation Japan",   "Top Ranking Stars (World new version)" )
GAME( 1993, trstarj,  trstar  , f3, f3, trstaroj, ROT0_16BIT,   "Taito Corporation",         "Top Ranking Stars (Japan new version)" )
GAME( 1993, prmtmfgt, trstar  , f3, f3, trstaroj, ROT0_16BIT,   "Taito America Corporation", "Prime Time Fighter (US new version)" )
GAME( 1993, trstaro,  trstar  , f3, f3, trstaroj, ROT0_16BIT,   "Taito Corporation Japan",   "Top Ranking Stars (World old version)" )
GAME( 1993, trstaroj, trstar  , f3, f3, trstaroj, ROT0_16BIT,   "Taito Corporation",         "Top Ranking Stars (Japan old version)" )
GAME( 1993, prmtmfgo, trstar  , f3, f3, trstaroj, ROT0_16BIT,   "Taito America Corporation", "Prime Time Fighter (US old version)" )
GAME( 1993, lightbr,  0       , f3, f3, lightbr,  ROT0_16BIT,   "Taito Corporation",         "Lightbringer (Japan)" )
/* Hacking the Lightbringer rom enables American & World versions called 'Dungeon Magic' - were they ever officially released? */
GAME( 1994, kaiserkn, 0       , f3, kn, kaiserkn, ROT0_16BIT,   "Taito Corporation Japan",   "Kaiser Knuckle (World)" )
GAME( 1994, kaiserkj, kaiserkn, f3, kn, kaiserkn, ROT0_16BIT,   "Taito Corporation",         "Kaiser Knuckle (Japan)" )
GAME( 1994, gblchmp,  kaiserkn, f3, kn, kaiserkn, ROT0_16BIT,   "Taito America Corporation", "Global Champion (US)" )
GAME( 1994, dankuga,  kaiserkn, f3, kn, kaiserkn, ROT0_16BIT,   "Taito Corporation",         "Dan-Ku-Ga (Japan)" )
GAME( 1994, dariusg,  0       , f3, f3, dariusg,  ROT0_16BIT,   "Taito Corporation",         "Darius Gaiden - Silver Hawk" )
GAME( 1994, dariusgx, dariusg , f3, f3, dariusg,  ROT0_16BIT,   "Taito Corporation",         "Darius Gaiden - Silver Hawk (Extra Version)" )
GAME( 1994, bublbob2, 0       , f3, f3, bubsymph, ROT0_16BIT,   "Taito Corporation Japan",   "Bubble Bobble 2 (World)" )
GAME( 1994, bubsymph, bublbob2, f3, f3, bubsymph, ROT0_16BIT,   "Taito Corporation",         "Bubble Symphony (Japan)" )
GAME( 1994, bubsympu, bublbob2, f3, f3, bubsymph, ROT0_16BIT,   "Taito America Corporation", "Bubble Symphony (US)" )
GAME( 1994, spcinvdj, spacedx , f3, f3, spcinvdj, ROT0_16BIT,   "Taito Corporation",         "Space Invaders DX (Japan F3 version)" )
GAME( 1995, pwrgoal,  0       , f3, f3, hthero95, ROT0_16BIT,   "Taito Corporation Japan",   "Power Goal (World)" )
GAME( 1995, hthero95, pwrgoal , f3, f3, hthero95, ROT0_16BIT,   "Taito Corporation",         "Hat Trick Hero '95 (Japan)" )
GAME( 1995, hthro95u, pwrgoal , f3, f3, hthero95, ROT0_16BIT,   "Taito America Corporation", "Hat Trick Hero '95 (US)" )
GAME( 1994, qtheater, 0       , f3, f3, qtheater, ROT0_16BIT,   "Taito Corporation",         "Quiz Theater - 3tsu no Monogatari (Japan)" )
GAME( 1994, elvactr,  0       , f3, f3, elvactr,  ROT0_16BIT,   "Taito Corporation Japan",   "Elevator Action Returns (World)" )
GAME( 1994, elvactrj, elvactr , f3, f3, elvactr,  ROT0_16BIT,   "Taito Corporation",         "Elevator Action Returns (Japan)" )
GAME( 1994, elvact2u, elvactr , f3, f3, elvactr,  ROT0_16BIT,   "Taito America Corporation", "Elevator Action 2 (US)" )
/* There is also a prototype Elevator Action 2 (US) pcb with the graphics in a different rom format (same program code) */
GAME( 1995, akkanvdr, 0       , f3, f3, spcinv95, ROT270_16BIT, "Taito Corporation",         "Akkanvader (Japan)" )
/* Space Invaders '95 - Attack Of The Lunar Loonies */
GAME( 1995, twinqix,  0       , f3, f3, twinqix,  ROT0_16BIT,   "Taito America Corporation", "Twin Qix (US Prototype)" )
GAME( 1995, gekirido, 0       , f3, f3, gekirido, ROT270_16BIT, "Taito Corporation",         "Gekirindan (Japan)" )
GAME( 1995, quizhuhu, 0       , f3, f3, quizhuhu, ROT0_16BIT,   "Taito Corporation",         "Moriguchi Hiroko no Quiz de Hyuuhyuu (Japan)" )
GAME( 1995, pbobble2, 0       , f3, f3, pbobble2, ROT0_16BIT,   "Taito Corporation Japan",	 "Puzzle Bobble 2 (World)" )
GAME( 1995, pbobbl2j, pbobble2, f3, f3, pbobble2, ROT0_16BIT,   "Taito Corporation", 		 "Puzzle Bobble 2 (Japan)" )
GAME( 1995, pbobbl2u, pbobble2, f3, f3, pbobble2, ROT0_16BIT,   "Taito America Corporation", "Bust-A-Move Again (US)" )
GAME( 1995, pbobbl2x, pbobble2, f3, f3, pbobbl2x, ROT0_16BIT,   "Taito Corporation", 		 "Puzzle Bobble 2X (Japan)" )
GAME( 1995, ktiger2,  0       , f3, f3, ktiger2,  ROT270_16BIT, "Taito Corporation",         "Kyukyoku Tiger 2 (Japan)" )
/* Twin Cobra 2 (US & World) is known to exist */
GAME( 1995, bubblem,  0       , f3, f3, bubblem,  ROT0_16BIT,   "Taito Corporation Japan",   "Bubble Memories - The Story Of Bubble Bobble 3 (World)" )
GAME( 1995, bubblemj, bubblem,  f3, f3, bubblem,  ROT0_16BIT,   "Taito Corporation", "Bubble Memories - The Story Of Bubble Bobble 3 (Japan)" )
GAME( 1996, cleopatr, 0       , f3, f3, cleopatr, ROT0_16BIT,   "Taito Corporation", "Cleopatra Fortune (Japan)" )
GAME( 1996, pbobble3, 0       , f3, f3, pbobble3, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 3 (World)" )
GAME( 1996, pbobbl3u, pbobble3, f3, f3, pbobble3, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 3 (US)" )
GAME( 1996, pbobbl3j, pbobble3, f3, f3, pbobble3, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 3 (Japan)" )
GAME( 1997, arkretrn, 0       , f3, f3, arkretrn, ROT0_16BIT,   "Taito Corporation", "Arkanoid Returns (Japan)" )
GAME( 1997, kirameki, 0       , f3, f3, kirameki, ROT0_16BIT,   "Taito Corporation", "Kirameki Star Road (Japan)" )
GAME( 1997, puchicar, 0       , f3, f3, puchicar, ROT0_16BIT,   "Taito Corporation", "Puchi Carat (Japan)" )
GAME( 1997, pbobble4, 0       , f3, f3, pbobble4, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 4 (World)" )
GAME( 1997, pbobbl4j, pbobble4, f3, f3, pbobble4, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 4 (Japan)" )
GAME( 1997, pbobbl4u, pbobble4, f3, f3, pbobble4, ROT0_16BIT,   "Taito Corporation", "Puzzle Bobble 4 (US)" )
GAME( 1997, popnpop,  0       , f3, f3, popnpop,  ROT0_16BIT,   "Taito Corporation", "Pop 'N Pop (Japan)" )
GAME( 1998, landmakr, 0       , f3, f3, landmakr, ROT0_16BIT,   "Taito Corporation", "Landmaker (Japan)" )
