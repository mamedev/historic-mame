/* Jaleco MegaSystem 32 (Preliminary Driver)


Used by Jaleco in the Mid-90's this system, based on the V70 processor consisted
of a two board set up, the first a standard mainboard and the second a 'cartridge'

-- Hardware Information (from Guru) --

MS32 Motherboard
----------------

PCB ID  : MB-93140A EB91022-20079-1
CPU     : NEC D70632GD-20 (V70)
SOUND   : Z80, YMF271, YAC513
OSC     : 48.000MHz, 16.9344MHz, 40.000MHz
RAM     : Cypress CY7C199-25 (x10)
          Sharp LH528256-70 (x5)
          Sharp LH5168D-10 (x1)
          OKI M511664-80 (x8)
DIPs    : 8 position (x3)
OTHER   : 5.5v battery
          Some PALs
          2 pin connector for right speaker (sound out is STEREO)

          Custom chips:
                       JALECO SS91022-01 (208 PIN PQFP)
                       JALECO SS91022-02 (100 PIN PQFP)
                       JALECO SS91022-03 (176 PIN PQFP) *
                       JALECO SS91022-05 (120 PIN PQFP) *
                       JALECO SS91022-07 (208 PIN PQFP)
                       JALECO GS91022-01 (120 PIN PQFP)
                       JALECO GS91022-02 (160 PIN PQFP)
                       JALECO GS91022-03 (100 PIN PQFP)
                       JALECO GS91022-04 (100 PIN PQFP) *

ROMs:     None

Chips marked * also appear on a non-megasystem 32 tetris 2 plus board

MS32 Cartridge
--------------

Game Roms + Custom Chip

The Custom chip is probably related to the encryption?

Desert War     - Custom chip: JALECO SS91022-10 (144 pin PQFP) (located on small plug-in board with) (ID: SE93139 EB91022-30056)
Game Paradise  - Custom chip: JALECO SS91022-10 9515EV 420201 06441
Tetris Plus 2  - Custom chip: JALECO SS91022-10 9513EV 370121 06441
Tetris Plus    - Custom chip: JALECO SS92046-01 9412EV 450891 06441
kirarast       - Custom chip: JALECO SS92047-01 9425EV 367821 06441
Gratia         - Custom chip: JALECO SS92047-01 9423EV 450891 06441
P47-Aces       - Custom chip: JALECO SS92048-01 9410EV 436091 06441
others are unknown

Notes
-----

Some of the roms for each game are encrypted.

16x16x8 'Scroll' Tiles (Non-Roz BG Layer)
8x8x8 'Ascii' Tiles (FG Layer)


ToDo / Notes
------------

Z80 + Sound Bits

Re-Add Priorities

Dip switches/inputs in t2m32 and f1superb
some games (hayaosi1) don't seeem to have service mode even if it's listed among the dips
service mode is still accessible through F1 though

Fix Anything Else (Palette etc.)

Replace CPU divider with Idle skip since we don't know how fast the v70 really is (cpu timing is ignored)...

Mirror Ram addresses?

Not sure about the main "global brightness" control register, I don't think it can make the palette
completely black because of kirarast attract mode, so I'm making it cut by 50% at most.

gametngk seems to need some kind of shadow sprites but the only difference in the sprite attributes is one of the
    priority bits, forcing sprites of that priority to be shadows doesn't work
tetrisp needs shadows as well, see the game selection screen.

The above might be related to the second "global brightness" control register, which is 000000 in all games
except gametngk, tetrisp, tp2m32 and gratia.

horizontal position of tx and bg tilemaps is off by 1 pixel in some games

There should be NVRAM somewhere, maybe fc000000-fc007fff

bbbxing: some sprite/roz/bg alignment issues

gratia: at the beginning of a level it shows the level name in the bottom right corner, scrolling it up
	and making the score display scroll out of the screen. Is this correct ar should there be a raster
	effect keeping the score on screen? And why didn't they just use sprites to do that?

gratia: the 3d sky shown at the beginning of the game has a black gap near the end. It would not be visible
	if I made the "global brightness" register cut to 100% instead of 50%. Mmmm...

gratia: the 3d sky seems to be the only place needed the "wrap" parameter to draw_roz to be set. All other
	games work fine with it not set, and there are many places where it definitely must not be set.

gratia: at the beginning of the game, before the sky appears, the city background appears for
	an instant. Missing layer enable register?

background color: pen 0 is correct for gametngk, but wrong for f1superb. Maybe it dpeends on the layer
	priority order?

roz layer wrapping: currently it's always ON, breaking places where it gets very small so it gets
	repeated on the screen (p47aces, kirarast, bbbxing, gametngk need it OFF).
	gratia and desertwr need it ON.

there are sprite lag issues, but they might be caused by cpu timing so it's better to leave
	them alone until the CPU clock is correct.


Not Working Games
-----------------

tp2m32 - writes invalid SBR, enables interrupts, crashes (fixed patching the bogus SBR).
f1superb - the road is straight despite the road signs saying otherwise? :-p
         - there are 4 unknown ROMS which might be related to the above.
         - the handler for IRQ 11 also seems to be valid, the game might need it.


Jaleco Megasystem 32 Game List - thanks to Yasuhiro
---------------------------------------------------

P-47 Aces (p47aces)
Game Tengoku / Game Paradise (gametngk)
Tetris Plus (tetrisp)
*Tetris Plus 2 (tp2m32)
Best Bout Boxing (bbbxing)
Wangan Sensou / Desert War (desertwr)
*Second Earth Gratia (gratia)
*Super Strong Warriors
F-1 Super Battle (f1superb)

*Idol Janshi Su-Chi-Pi 2
Ryuusei Janshi Kirara Star (kirarast)
*Mahjong Angel Kiss
*Vs. Janshi Brand New Stars

Hayaoshi Quiz Ouza Ketteisen (hayaosi1)
*Hayaoshi Quiz Nettou Namahousou
*Hayaoshi Quiz Grand Champion Taikai

Maybe some more...

Games marked * need dumping / redumping

*/

/********** BITS & PIECES **********/

#include "driver.h"

extern data32_t *ms32_fce00000;
extern data32_t *ms32_roz_ctrl;
extern data32_t *ms32_tx_scroll;
extern data32_t *ms32_bg_scroll;
extern data32_t *ms32_priram;
extern data32_t *ms32_palram;
extern data32_t *ms32_bgram;
extern data32_t *ms32_rozram;
extern data32_t *ms32_lineram;
extern data32_t *ms32_spram;
extern data32_t *ms32_txram;
extern data32_t *ms32_mainram;

WRITE32_HANDLER( ms32_brightness_w );
WRITE32_HANDLER( ms32_palram_w );
READ32_HANDLER( ms32_txram_r );
WRITE32_HANDLER( ms32_txram_w );
READ32_HANDLER( ms32_rozram_r );
WRITE32_HANDLER( ms32_rozram_w );
READ32_HANDLER( ms32_lineram_r );
WRITE32_HANDLER( ms32_lineram_w );
READ32_HANDLER( ms32_bgram_r );
WRITE32_HANDLER( ms32_bgram_w );
READ32_HANDLER( ms32_spram_r );
WRITE32_HANDLER( ms32_spram_w );
READ32_HANDLER( ms32_priram_r );
WRITE32_HANDLER( ms32_priram_w );
WRITE32_HANDLER( ms32_gfxctrl_w );
VIDEO_START( ms32 );
VIDEO_UPDATE( ms32 );

static data32_t *ms32_fc000000;

static data32_t *ms32_mahjong_input_select;

/********** READ INPUTS **********/

static READ32_HANDLER ( ms32_read_inputs1 )
{
	int a,b,c,d;
	a = readinputport(0);	// unknown
	b = readinputport(1);	// System inputs
	c = readinputport(2);	// Player 1 inputs
	d = readinputport(3);	// Player 2 inputs
	return a << 24 | b << 16 | c << 0 | d << 8;
}


static READ32_HANDLER ( ms32_mahjong_read_inputs1 )
{
	int a,b,c,d;
	a = readinputport(0);	// unknown
	b = readinputport(1);	// System inputs

	switch (ms32_mahjong_input_select[0])
	{
		case 0x01:
			c = readinputport(8);	// Player 1 inputs
			break;
		case 0x02:
			c = readinputport(9);	// Player 1 inputs
			break;
		case 0x04:
			c = readinputport(10);	// Player 1 inputs
			break;
		case 0x08:
			c = readinputport(11);	// Player 1 inputs
			break;
		case 0x10:
			c = readinputport(12);	// Player 1 inputs
			break;
		default:
			c = 0;

	}
	d = readinputport(3);	// Player 2 inputs
	return a << 24 | b << 16 | c << 0 | d << 8;
}


static READ32_HANDLER ( ms32_read_inputs2 )
{
	int a,b,c,d;
	a = readinputport(4);	// Dip 1
	b = readinputport(5);	// Dip 2
	c = readinputport(6);	// Dip 3
	d = readinputport(7);	// unused ?
	return a << 8 | b << 0 | c << 16 | d << 24;
}

static READ32_HANDLER ( ms32_read_inputs3 )
{
	int a,b,c,d;
	a = readinputport(10); // unused?
	b = readinputport(10); // unused?
	c = readinputport(9);
	d = (readinputport(8) - 0xb0) & 0xff;
	return a << 24 | b << 16 | c << 8 | d << 0;
}

/********** MEMORY MAP **********/

/* some games games test more ram than others .. ram areas with closed comments before
the lines get tested by various games but I'm not sure are really used, maybe they're
mirror addresses? */

/*
p47 aces:
there are bugs in the test routine, so it checks twice the amount of RAM
actually present, relying on mirror addresses.
See how ASCII and SCROLL overlap.
SCRATCH RAM   fee00000-fee1ffff
ASCII RAM     fec00000-fec0ffff (actually fec00000-fec07fff ?)
SCROLL RAM    fec08000-fec17fff (actually fec08000-fec0ffff ?)
ROTATE RAM    fe000000-fe03ffff (actually fe000000-fe01ffff ?)
OBJECT RAM    fe800000-fe87ffff (actually fe800000-fe83ffff ?)
COLOR RAM     fd400000-fd40ffff (this one is actually larger than tested)
PRIORITY RAM  fd180000-fd1bffff (actually fd180000-fd19ffff ?)
SOUND RAM

This applies to most of the other games.
Also, gametngk uses mirror addresses for the background during gameplay, without
support for them bad tiles appear in the bg.
*/


static MEMORY_READ32_START( ms32_readmem )
	{ 0x00000000, 0x001fffff, MRA32_ROM },
	{ 0xfc000000, 0xfc007fff, MRA32_RAM },
	{ 0xfc800000, 0xfc800003, MRA32_NOP },	/* sound? */
	{ 0xfcc00004, 0xfcc00007, ms32_read_inputs1 },
	{ 0xfcc00010, 0xfcc00013, ms32_read_inputs2 },
/**/{ 0xfce00600, 0xfce0065f, MRA32_RAM },	/* roz control registers */
/**/{ 0xfce00a00, 0xfce00a17, MRA32_RAM },	/* tx scroll registers */
/**/{ 0xfce00a20, 0xfce00a37, MRA32_RAM },	/* bg scroll registers */

//	{ 0xfd000000, 0xfd000003, MRA32_NOP }, /* f1superb? */
	{ 0xfd0e0000, 0xfd0e0003, ms32_read_inputs3 }, /* analog controls in f1superb? */

///**/{ 0xfd104000, 0xfd105fff, MRA32_RAM }, /* f1superb */
///**/{ 0xfd144000, 0xfd145fff, MRA32_RAM }, /* f1superb */

	{ 0xfd180000, 0xfd19ffff, ms32_priram_r },	/* priority ram */
	{ 0xfd1a0000, 0xfd1bffff, ms32_priram_r },	/* mirror only used by memory test in service mode */

	{ 0xfd400000, 0xfd43ffff, MRA32_RAM }, /* Palette */
///**/{ 0xfd440000, 0xfd47ffff, MRA32_RAM }, /* f1superb color */

///**/{ 0xfdc00000, 0xfdc006ff, MRA32_RAM }, /* f1superb */
///**/{ 0xfde00000, 0xfde01fff, MRA32_RAM }, /* f1superb lineram */
	{ 0xfe000000, 0xfe01ffff, ms32_rozram_r },	/* roz layer */
	{ 0xfe020000, 0xfe03ffff, ms32_rozram_r },	/* mirror only used by memory test in service mode */
	{ 0xfe200000, 0xfe201fff, ms32_lineram_r }, /* line ram for roz layer */
///**/{ 0xfe202000, 0xfe2fffff, MRA32_RAM }, /* f1superb vram */

	{ 0xfe800000, 0xfe83ffff, ms32_spram_r },	/* sprites */
	{ 0xfe840000, 0xfe87ffff, ms32_spram_r },	/* mirror only used by memory test in service mode */
	{ 0xfec00000, 0xfec07fff, ms32_txram_r },	/* tx layer */
	{ 0xfec08000, 0xfec0ffff, ms32_bgram_r },	/* bg layer */
	{ 0xfec10000, 0xfec17fff, ms32_txram_r },	/* mirror only used by memory test in service mode */
	{ 0xfec18000, 0xfec1ffff, ms32_bgram_r },
	{ 0xfee00000, 0xfee1ffff, MRA32_RAM },
	{ 0xffe00000, 0xffffffff, MRA32_BANK1 },
MEMORY_END

static WRITE32_HANDLER( pip_w )
{
	if (data)
		usrintf_showmessage("fce00a7c = %02x",data);
}

static MEMORY_WRITE32_START( ms32_writemem )
	{ 0x00000000, 0x001fffff, MWA32_ROM },
	{ 0xfc000000, 0xfc007fff, MWA32_RAM, &ms32_fc000000 },	// NVRAM?
	{ 0xfc800000, 0xfc800003, MWA32_NOP }, /* sound? */
	{ 0xfce00000, 0xfce00003, ms32_gfxctrl_w },	/* flip screen + other unknown bits */
	{ 0xfce00050, 0xfce00053, MWA32_NOP },	// watchdog? I haven't investigated
//	{ 0xfce00000, 0xfce0007f, MWA32_RAM, &ms32_fce00000 }, /* registers not ram? */
	{ 0xfce00280, 0xfce0028f, ms32_brightness_w },	// global brightness control
	{ 0xfce00600, 0xfce0065f, MWA32_RAM, &ms32_roz_ctrl },	/* roz control registers */
//	{ 0xfce00800, 0xfce0085f, // f1superb, roz #2 control?
	{ 0xfce00a00, 0xfce00a17, MWA32_RAM, &ms32_tx_scroll },	/* tx layer scroll */
	{ 0xfce00a20, 0xfce00a37, MWA32_RAM, &ms32_bg_scroll },	/* bg layer scroll */
	{ 0xfce00a7c, 0xfce00a7f, pip_w },	// ??? layer related? seems to be always 0
//	{ 0xfce00e00, 0xfce00e03,  },	coin counters + something else

//	{ 0xfd104000, 0xfd105fff, MWA32_RAM }, /* f1superb */
//	{ 0xfd144000, 0xfd145fff, MWA32_RAM }, /* f1superb */

	{ 0xfd180000, 0xfd19ffff, ms32_priram_w, &ms32_priram },	/* priority ram */
	{ 0xfd1a0000, 0xfd1bffff, ms32_priram_w },			/* mirror only used by memory test in service mode */

	{ 0xfd1c0000, 0xfd1c0003, MWA32_RAM, &ms32_mahjong_input_select }, // ?

	{ 0xfd400000, 0xfd43ffff, ms32_palram_w, &ms32_palram }, /* Palette */
///**/{ 0xfd440000, 0xfd47ffff, MWA32_RAM }, /* f1superb color */
//	{ 0xfdc00000, 0xfdc006ff, MWA32_RAM }, /* f1superb */
//	{ 0xfde00000, 0xfde01fff, MWA32_RAM }, /* f1superb, lineram #2? */

	{ 0xfe000000, 0xfe01ffff, ms32_rozram_w, &ms32_rozram },	/* roz layer */
	{ 0xfe020000, 0xfe03ffff, ms32_rozram_w },		/* mirror only used by memory test in service mode */
	{ 0xfe1ffc88, 0xfe1fffff, MWA32_NOP },	/* gratia writes here before falling into lineram, could be a mirror */
	{ 0xfe200000, 0xfe201fff, ms32_lineram_w, &ms32_lineram }, /* line ram for roz layer */
///**/{ 0xfe202000, 0xfe2fffff, MWA32_RAM }, /* f1superb vram */
///**/{ 0xfe100000, 0xfe1fffff, MWA32_RAM }, /* gratia writes here ?! */
	{ 0xfe800000, 0xfe83ffff, ms32_spram_w, &ms32_spram },	/* sprites */
	{ 0xfe840000, 0xfe87ffff, ms32_spram_w },		/* mirror only used by memory test in service mode */
	{ 0xfec00000, 0xfec07fff, ms32_txram_w, &ms32_txram },	/* tx layer */
	{ 0xfec08000, 0xfec0ffff, ms32_bgram_w, &ms32_bgram },	/* bg layer */
	{ 0xfec10000, 0xfec17fff, ms32_txram_w },		/* mirror only used by memory test in service mode */
	{ 0xfec18000, 0xfec1ffff, ms32_bgram_w },		/* mirror used by gametngk at the beginning of the game */
	{ 0xfee00000, 0xfee1ffff, MWA32_RAM, &ms32_mainram },
	{ 0xffe00000, 0xffffffff, MWA32_ROM },
MEMORY_END

/*
static MEMORY_READ_START( ms32_sound_readmem )
MEMORY_END

static MEMORY_WRITE_START( ms32_sound_writemem )
MEMORY_END
*/

/********** INPUT PORTS **********/

#define MS32_UNUSED_PORT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

#define MS32_SYSTEM_INPUTS \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	/* bits 6 and 7 might be different from game to game */

#define MS32_UNKNOWN_INPUTS \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define MS32_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ )

#define MS32_DIP1 \
	PORT_START \
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) ) \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Free_Play ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )


INPUT_PORTS_START( ms32 )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( bbbxing )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )			// BUTTON5 in "test mode"
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )			// BUTTON5 in "test mode"

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)	// BUTTON4 in "test mode"
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)	// BUTTON4 in "test mode"

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x00, "Timer Speed" )
	PORT_DIPSETTING(    0x00, "60/60" )
	PORT_DIPSETTING(    0x20, "50/60" )
	PORT_DIPSETTING(    0x10, "40/60" )
	PORT_DIPSETTING(    0x30, "35/60" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
#if 0	/* this is what you have in the "test mode", but I don't see what this means 8( */
	PORT_DIPNAME( 0x80, 0x80, "Jyogi" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Kim" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Thamalatt" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Jose" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Carolde" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Biff" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Grute" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#else
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#endif
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( desertwr )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Armors" )
//	PORT_DIPSETTING(    0x00, "2" )		// duplicate setting ?
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Title screen" )
	PORT_DIPSETTING(    0x10, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( gametngk )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Voice" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( tetrisp )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* There are inputs for players 3 and 4 in the "test mode",
	   but NO addresses are read to check them ! */

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(    0x00, "1/1" )
	PORT_DIPSETTING(    0x30, "2/3" )
	PORT_DIPSETTING(    0x10, "3/5" )
	PORT_DIPSETTING(    0x20, "4/7" )
	PORT_DIPNAME( 0x0c, 0x0c, "Join In" )
	PORT_DIPSETTING(    0x0c, "All Modes" )
	PORT_DIPSETTING(    0x04, "Normal and Puzzle Modes" )
	PORT_DIPSETTING(    0x08, "VS Mode" )
//	PORT_DIPSETTING(    0x00, "Normal and Puzzle Modes" )		// "can't play normal mode" in "test mode"
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Voice" )
	PORT_DIPSETTING(    0x00, "English Only" )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "After VS Mode" )
	PORT_DIPSETTING(    0x08, "Game Ends" )
	PORT_DIPSETTING(    0x00, "Winner Continues" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

/* The Dip Switches for this game are completely wrong in the "test mode" ! */
INPUT_PORTS_START( p47aces )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)	// BUTTON3 and BUTTON4 in "test mode"

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, "FG/BG X offset" )
	PORT_DIPSETTING(    0x03, "0/0" )
	PORT_DIPSETTING(    0x02, "5/5" )
//	PORT_DIPSETTING(    0x01, "5/5" )
	PORT_DIPSETTING(    0x00, "2/4" )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( gratia )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MS32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)	// BUTTON4 in "test mode"
	MS32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)	// BUTTON4 in "test mode"

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x80, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "200k and every 1000k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( hayaosi1 )
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	// "Buzzer" (input 0 in "test mode")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER3 )	// "Buzzer" (input 0 in "test mode")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )	// "Buzzer" (input 0 in "test mode")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	MS32_DIP1	// "Service Mode" Dip Switch doesn't work ! Use the "Test" button instead !

	PORT_START	// DIP2
	PORT_DIPNAME( 0xc0, 0xc0, "Computer's AI (VS Mode)" )
	PORT_DIPSETTING(    0x40, "Low" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0x00, "Highest" )
	/*  Lap    Time    Questions */
      /*   1     1:00        4     */
      /*   2     1:00        6     */
      /*   3     1:30        8     */
      /*   4     1:30       10     */
      /*   4     2:00       14     */
      /* final   2:00       18     */
	PORT_DIPNAME( 0x30, 0x30, "Time (Race Mode)" )
	PORT_DIPSETTING(    0x00, "Default - 0:15" )
	PORT_DIPSETTING(    0x20, "Default - 0:10" )
	PORT_DIPSETTING(    0x30, "Default" )
	PORT_DIPSETTING(    0x10, "Default + 0:15" )
	/* Round   Default    More */
      /*   1       10        15  */
	PORT_DIPNAME( 0x08, 0x08, "Questions (VS Mode)" )	// TO DO : check all rounds
	PORT_DIPSETTING(    0x08, "Default" )
	PORT_DIPSETTING(    0x00, "More" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )		// "Unused" ?
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )		// "Demo Sounds" ?
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )		// "Flip Screen" ?
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

INPUT_PORTS_END

INPUT_PORTS_START( kirarast )	// player 1 inputs done? others?
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MS32_UNUSED_PORT // p1
	MS32_UNUSED_PORT // p2

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )	// to be confirmed !
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

	PORT_START	// Mahjong Inputs 0x01
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// Mahjong Inputs 0x02
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// Mahjong Inputs 0x04
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 C",     KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 G",     KEYCODE_G,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Chi",   KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 K",     KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// Mahjong Inputs 0x08
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 L",     KEYCODE_L,        IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// Mahjong Inputs 0x10
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( f1superb )	// Mostly wrong !
	MS32_UNKNOWN_INPUTS

	MS32_SYSTEM_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

	MS32_DIP1

	PORT_START	// DIP2
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	MS32_UNUSED_PORT

	PORT_START	// Acceleration (wrong?)
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1, 50, 15, 0, 0xff)


	PORT_START	// Steering
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 50, 15, 0x00, 0xff)

	PORT_START	// Shift + Brake
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/********** GFX DECODE **********/

/* sprites are contained in 256x256 "tiles" */
static struct GfxLayout spritelayout =
{
	256,256,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 256*8 },	/* line modulo */
	256*256*8	/* char modulo */
};

static struct GfxLayout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 16*8 },	/* line modulo */
	16*16*8		/* char modulo */
};


static struct GfxLayout txlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	8*8*8		/* char modulo */
};

static struct GfxDecodeInfo ms32_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0x0000, 0x10 },
	{ REGION_GFX2, 0, &bglayout,     0x2000, 0x10 },
	{ REGION_GFX3, 0, &bglayout,     0x1000, 0x10 },
	{ REGION_GFX4, 0, &txlayout,     0x6000, 0x10 },
	{ -1 }
};



/********** INTERRUPTS **********/

/* Irqs used in desertwr:
   1  - 6a0 - minimal
   9  - 6c8 - minimal
   10 - 6d4 - big, vbl?
*/

static UINT16 irqreq;

static int irq_callback(int irqline)
{
	int i;
	for(i=15; i>=0 && !(irqreq & (1<<i)); i--);
	irqreq &= ~(1<<i);
	if(!irqreq)
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	return i;
}

static void irq_init(void)
{
	irqreq = 0;
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}

static void irq_raise(int level)
{
	irqreq |= (1<<level);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static INTERRUPT_GEN(ms32_interrupt)
{
	if( cpu_getiloops() == 0 ) irq_raise(10);
	if( cpu_getiloops() == 1 ) irq_raise(9);
	if( cpu_getiloops() == 2 ) irq_raise(1);
	/* hayaosi1 needs at least 12 IRQ 0 per frame to work (see code at FFE02289)
	   kirarast needs it too, at least 8 per frame, but waits for a variable amount
	   in different points. Could this be a raster interrupt?
	   Other games using it but not needing it to work:
	   desertwr
	   p47aces
	   */
	if( cpu_getiloops() >= 3 && cpu_getiloops() <= 15 ) irq_raise(0);
}

/********** MACHINE INIT **********/

static MACHINE_INIT( ms32 )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();
}

/********** MACHINE DRIVER **********/

static MACHINE_DRIVER_START( ms32 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V70, 20000000/9) // 20MHz
	MDRV_CPU_MEMORY(ms32_readmem,ms32_writemem)
	MDRV_CPU_VBLANK_INT(ms32_interrupt,15)
/*
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(ms32_sound_readmem,ms32_sound_writemem)
	MDRV_CPU_PORTS(ms32_sound_readport,ms32_sound_writeport)
*/
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ms32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(ms32_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(ms32)
	MDRV_VIDEO_UPDATE(ms32)

	/* Sound is unemulated YMF271 */
MACHINE_DRIVER_END


/********** ROM LOADING **********/

ROM_START( bbbxing )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "bbbx25.bin", 0x000003, 0x80000, 0xb526b41e )
	ROM_LOAD32_BYTE( "bbbx27.bin", 0x000002, 0x80000, 0x45b27ad8 )
	ROM_LOAD32_BYTE( "bbbx29.bin", 0x000001, 0x80000, 0x85bbbe79 )
	ROM_LOAD32_BYTE( "bbbx31.bin", 0x000000, 0x80000, 0xe0c865ed )

	ROM_REGION( 0x1100000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "bbbx1.bin",   0x0000002, 0x200000, 0xc1c10c3b )
	ROM_LOAD32_WORD( "bbbx13.bin",  0x0000000, 0x200000, 0x4b8c1574 )
	ROM_LOAD32_WORD( "bbbx2.bin",   0x0400002, 0x200000, 0x03b77c1e )
	ROM_LOAD32_WORD( "bbbx14.bin",  0x0400000, 0x200000, 0xe9cfd83b )
	ROM_LOAD32_WORD( "bbbx3.bin",   0x0800002, 0x200000, 0xbba0d1a4 )
	ROM_LOAD32_WORD( "bbbx15.bin",  0x0800000, 0x200000, 0x6ab64a10 )
	ROM_LOAD32_WORD( "bbbx4.bin",   0x0c00002, 0x200000, 0x97f97e3a )
	ROM_LOAD32_WORD( "bbbx16.bin",  0x0c00000, 0x200000, 0xe001d6cb )
	ROM_LOAD32_WORD( "bbbx5.bin",   0x1000002, 0x080000, 0x64989edf )
	ROM_LOAD32_WORD( "bbbx17.bin",  0x1000000, 0x080000, 0x1d7ebaf0 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "bbbx9.bin",   0x000000, 0x200000, 0xa41cb650 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "bbbx11.bin",  0x000000, 0x200000, 0x85238ca9 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "bbbx32-2.bin",0x000000, 0x080000, 0x3ffdae75 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "bbbx21.bin",  0x000000, 0x040000, 0x5f3ea01f )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "bbbx22.bin",  0x000000, 0x200000, 0x0fa26f65 ) // common samples
	ROM_LOAD( "bbbx23.bin",  0x200000, 0x200000, 0xb7875a23 )
ROM_END

ROM_START( desertwr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "93166-26.37", 0x000003, 0x80000, 0x582b9584 )
	ROM_LOAD32_BYTE( "93166-27.38", 0x000002, 0x80000, 0xcb60dda3 )
	ROM_LOAD32_BYTE( "93166-28.39", 0x000001, 0x80000, 0x0de40efb )
	ROM_LOAD32_BYTE( "93166-29.40", 0x000000, 0x80000, 0xfc25eae2 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "94038-01.20", 0x000000, 0x200000, 0xf11f83e2 )
	ROM_LOAD32_WORD( "94038-02.3",  0x000002, 0x200000, 0x3d1fa710 )
	ROM_LOAD32_WORD( "94038-03.21", 0x400000, 0x200000, 0x84fd5790 )
	ROM_LOAD32_WORD( "94038-04.4",  0x400002, 0x200000, 0xb9ef5b78 )
	ROM_LOAD32_WORD( "94038-05.22", 0x800000, 0x200000, 0xfeee1b8d )
	ROM_LOAD32_WORD( "94038-06.5",  0x800002, 0x200000, 0xd417f289 )
	ROM_LOAD32_WORD( "94038-07.23", 0xc00000, 0x200000, 0x426f4193 )
	ROM_LOAD32_WORD( "94038-08.6",  0xc00002, 0x200000, 0xf4088399 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "94038-11.13", 0x000000, 0x200000, 0xbf2ec3a3 )
	ROM_LOAD( "94038-12.14", 0x200000, 0x200000, 0xd0e113da )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "94038-09.12", 0x000000, 0x200000, 0x72ec1ce7 )
	ROM_LOAD( "94038-10.11", 0x200000, 0x200000, 0x1e17f2a9 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "93166-30.41", 0x000000, 0x080000, 0x980ab89c )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "93166-21.30", 0x000000, 0x040000, 0x9300be4c )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "94038-13.34", 0x000000, 0x200000, 0xb0cac8f2 )
	ROM_LOAD( "92042-01.33", 0x200000, 0x200000, 0x0fa26f65 ) // common samples
ROM_END

ROM_START( f1superb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "f1sb26.bin", 0x000003, 0x80000, 0x042fccd5 )
	ROM_LOAD32_BYTE( "f1sb27.bin", 0x000002, 0x80000, 0x5f96cf32 )
	ROM_LOAD32_BYTE( "f1sb28.bin", 0x000001, 0x80000, 0xcfda8003 )
	ROM_LOAD32_BYTE( "f1sb29.bin", 0x000000, 0x80000, 0xf21f1481 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* 8x8 not all? */
	ROM_LOAD32_WORD( "f1sb1.bin",  0x0000002, 0x200000, 0x53a3a97b )
	ROM_LOAD32_WORD( "f1sb13.bin", 0x0000000, 0x200000, 0x36565a99 )
	ROM_LOAD32_WORD( "f1sb2.bin",  0x0400002, 0x200000, 0xa452f50a )
	ROM_LOAD32_WORD( "f1sb14.bin", 0x0400000, 0x200000, 0xc0c20490 )
	ROM_LOAD32_WORD( "f1sb3.bin",  0x0800002, 0x200000, 0x265d068c )
	ROM_LOAD32_WORD( "f1sb15.bin", 0x0800000, 0x200000, 0x575a146e )
	ROM_LOAD32_WORD( "f1sb4.bin",  0x0c00002, 0x200000, 0x0ccc66fd )
	ROM_LOAD32_WORD( "f1sb16.bin", 0x0c00000, 0x200000, 0xa2d017a1 )
	ROM_LOAD32_WORD( "f1sb5.bin",  0x1000002, 0x200000, 0xbff4271b )
	ROM_LOAD32_WORD( "f1sb17.bin", 0x1000000, 0x200000, 0x2b9739d5 )
	ROM_LOAD32_WORD( "f1sb6.bin",  0x1400002, 0x200000, 0x6caf48ec )
	ROM_LOAD32_WORD( "f1sb18.bin", 0x1400000, 0x200000, 0xc49055ff )
	ROM_LOAD32_WORD( "f1sb7.bin",  0x1800002, 0x200000, 0xa5458947 )
	ROM_LOAD32_WORD( "f1sb19.bin", 0x1800000, 0x200000, 0xb7cacf0d )
	ROM_LOAD32_WORD( "f1sb8.bin",  0x1c00002, 0x200000, 0xba3f1533 )
	ROM_LOAD32_WORD( "f1sb20.bin", 0x1c00000, 0x200000, 0xfa349897 )

	ROM_REGION( 0x800000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "f1sb9.bin",  0x000000, 0x200000, 0x66b12e1f )
	ROM_LOAD( "f1sb10.bin", 0x200000, 0x200000, 0x893d7f4b )
	ROM_LOAD( "f1sb11.bin", 0x400000, 0x200000, 0x0b848bb5 )
	ROM_LOAD( "f1sb12.bin", 0x600000, 0x200000, 0xedecd5f4 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "f1sb31.bin", 0x000000, 0x200000, 0x1d0d2efd )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "f1sb32.bin", 0x000000, 0x080000, 0x1b31fcce )

	ROM_REGION( 0x800000, REGION_USER1, 0 ) /* extra ROMs, unknown */
	ROM_LOAD( "f1sb2b.bin", 0x000000, 0x200000, 0x18d73b16 )
	ROM_LOAD( "f1sb3b.bin", 0x200000, 0x200000, 0xce728fe0 )
	ROM_LOAD( "f1sb4b.bin", 0x400000, 0x200000, 0x077180c5 )
	ROM_LOAD( "f1sb5b.bin", 0x600000, 0x200000, 0xefabc47d )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "f1sb21.bin", 0x000000, 0x040000, 0xe131e1c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "f1sb23.bin", 0x000000, 0x200000, 0xbfefa3ab )
	ROM_LOAD( "f1sb24.bin", 0x200000, 0x200000, 0x0fa26f65 ) // common samples
ROM_END

ROM_START( gratia )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "94019.026", 0x000003, 0x80000, 0xf398cba5 )
	ROM_LOAD32_BYTE( "94019.027", 0x000002, 0x80000, 0xba3318c5 )
	ROM_LOAD32_BYTE( "94019.028", 0x000001, 0x80000, 0xe0762e89 )
	ROM_LOAD32_BYTE( "94019.029", 0x000000, 0x80000, 0x8059800b )

	ROM_REGION( 0x0c00000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "94019.01", 0x000000, 0x200000, 0x92d8ae9b )
	ROM_LOAD32_WORD( "94019.02", 0x000002, 0x200000, 0xf7bd9cc4 )
	ROM_LOAD32_WORD( "94019.03", 0x400000, 0x200000, 0x62a69590 )
	ROM_LOAD32_WORD( "94019.04", 0x400002, 0x200000, 0x5a76a39b )
	ROM_LOAD32_WORD( "94019.05", 0x800000, 0x200000, 0xa16994df )
	ROM_LOAD32_WORD( "94019.06", 0x800002, 0x200000, 0x01d52ef1 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "94019.08", 0x000000, 0x200000, 0xabd124e0 )
	ROM_LOAD( "94019.09", 0x200000, 0x200000, 0x711ab08b )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "94019.07", 0x000000, 0x200000, BADCRC( 0xacb75824 ) )	// FIXED BITS (xxxxxxxx11111111)

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "94019.030",0x000000, 0x080000, 0x026b5379 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "94019.021",0x000000, 0x040000, 0x6e8dd039 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "92042.01", 0x000000, 0x200000, 0x0fa26f65 ) // common rom?
	ROM_LOAD( "94019.10", 0x200000, 0x200000, 0xa751e316 )
ROM_END

ROM_START( gametngk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "mr94041.26", 0x000003, 0x80000, 0xe622e774 )
	ROM_LOAD32_BYTE( "mr94041.27", 0x000002, 0x80000, 0xda862b9c )
	ROM_LOAD32_BYTE( "mr94041.28", 0x000001, 0x80000, 0xb3738934 )
	ROM_LOAD32_BYTE( "mr94041.29", 0x000000, 0x80000, 0x45154a45 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "mr94041.01", 0x0000000, 0x200000, 0x3f99adf7 )
	ROM_LOAD32_WORD( "mr94041.02", 0x0000002, 0x200000, 0xc3c5ae69 )
	ROM_LOAD32_WORD( "mr94041.03", 0x0400000, 0x200000, 0xd858b6de )
	ROM_LOAD32_WORD( "mr94041.04", 0x0400002, 0x200000, 0x8c96ca20 )
	ROM_LOAD32_WORD( "mr94041.05", 0x0800000, 0x200000, 0xac664a0b )
	ROM_LOAD32_WORD( "mr94041.06", 0x0800002, 0x200000, 0x70dd0dd4 )
	ROM_LOAD32_WORD( "mr94041.07", 0x0c00000, 0x200000, 0xa6966af5 )
	ROM_LOAD32_WORD( "mr94041.08", 0x0c00002, 0x200000, 0xd7d2f73a )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr94041.11", 0x000000, 0x200000, 0x00dcdbc3 )
	ROM_LOAD( "mr94041.12", 0x200000, 0x200000, 0x0ce48329 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr94041.09", 0x000000, 0x200000, 0xa33e6051 )
	ROM_LOAD( "mr94041.10", 0x200000, 0x200000, 0xb3497147 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr94041.30", 0x000000, 0x080000, 0xc0f27b7f )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "mr94041.21", 0x000000, 0x040000, 0x38dcb837 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "mr94041.13", 0x000000, 0x200000, 0xfba84caf )
	ROM_LOAD( "mr94041.14", 0x200000, 0x200000, 0x2d6308bd )
ROM_END

ROM_START( hayaosi1 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "mb93138a.25", 0x000003, 0x80000, 0x563c6f2f )
	ROM_LOAD32_BYTE( "mb93138a.27", 0x000002, 0x80000, 0xfe8e283a )
	ROM_LOAD32_BYTE( "mb93138a.29", 0x000001, 0x80000, 0xe6fe3d0d )
	ROM_LOAD32_BYTE( "mb93138a.31", 0x000000, 0x80000, 0xd944bf8c )

	ROM_REGION( 0x900000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "mr93038.04",  0x000000, 0x200000, 0xab5edb11 )
	ROM_LOAD32_WORD( "mr93038.05",  0x000002, 0x200000, 0x274522f1 )
	ROM_LOAD32_WORD( "mr93038.06",  0x400000, 0x200000, 0xf9961ebf )
	ROM_LOAD32_WORD( "mr93038.07",  0x400002, 0x200000, 0x1abef1c5 )
	ROM_LOAD32_WORD( "mb93138a.15", 0x800000, 0x080000, 0xa5f64d87 )
	ROM_LOAD32_WORD( "mb93138a.3",  0x800002, 0x080000, 0xa2ae2b21 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr93038.03",  0x000000, 0x200000, 0x6999dec9 )

	ROM_REGION( 0x100000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr93038.08",  0x000000, 0x100000, 0x21282cb0 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mb93138a.32", 0x000000, 0x080000, 0xf563a144 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "mb93138a.21", 0x000000, 0x040000, 0x8e8048b0 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples - 8-bit signed PCM */
	ROM_LOAD( "mr93038.01",  0x000000, 0x200000, 0xb8a38bfc )
	ROM_LOAD( "mr92042.01",  0x000000, 0x200000, 0x0fa26f65 ) // common samples
ROM_END

ROM_START( kirarast )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "mr95025.26", 0x000003, 0x80000, 0xeb7faf5f )
	ROM_LOAD32_BYTE( "mr95025.27", 0x000002, 0x80000, 0x80644d05 )
	ROM_LOAD32_BYTE( "mr95025.28", 0x000001, 0x80000, 0x6df8c384 )
	ROM_LOAD32_BYTE( "mr95025.29", 0x000000, 0x80000, 0x3b6e681b )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "mr95025.01",  0x000000, 0x200000, 0x02279069 )
	ROM_LOAD32_WORD( "mr95025.02",  0x000002, 0x200000, 0x885161d4 )
	ROM_LOAD32_WORD( "mr95025.03",  0x400000, 0x200000, 0x1ae06df9 )
	ROM_LOAD32_WORD( "mr95025.04",  0x400002, 0x200000, 0x91ab7006 )
	ROM_LOAD32_WORD( "mr95025.05",  0x800000, 0x200000, 0xe61af029 )
	ROM_LOAD32_WORD( "mr95025.06",  0x800002, 0x200000, 0x63f64ffc )
	ROM_LOAD32_WORD( "mr95025.07",  0xc00000, 0x200000, 0x0263a010 )
	ROM_LOAD32_WORD( "mr95025.08",  0xc00002, 0x200000, 0x8efc00d6 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95025.10",  0x000000, 0x200000, 0xba7ad413 )
	ROM_LOAD( "mr95025.11",  0x200000, 0x200000, 0x11557299 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95025.09",  0x000000, 0x200000, 0xca6cbd17 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95025.30",  0x000000, 0x080000, 0xaee6e0c2 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "mr95025.21",  0x000000, 0x040000, 0xa6c70c7f )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples - 8-bit signed PCM */
	ROM_LOAD( "mr95025.12",  0x000000, 0x200000, 0x1dd4f766 )
	ROM_LOAD( "mr95025.13",  0x000000, 0x200000, 0x0adfe5b8 )
ROM_END

ROM_START( p47aces )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "p47-26.bin", 0x000003, 0x80000, 0xe017b819 )
	ROM_LOAD32_BYTE( "p47-27.bin", 0x000002, 0x80000, 0xbd1b81e0 )
 	ROM_LOAD32_BYTE( "p47-28.bin", 0x000001, 0x80000, 0x4742a5f7 )
	ROM_LOAD32_BYTE( "p47-29.bin", 0x000000, 0x80000, 0x86e17d8b )

	ROM_REGION( 0xe00000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "p47-01.bin",  0x000002, 0x200000, 0x28732d3c )
	ROM_LOAD32_WORD( "p47-13.bin",  0x000000, 0x200000, 0xa6ccf999 )
	ROM_LOAD32_WORD( "p47-02.bin",  0x400002, 0x200000, 0x128db576 )
	ROM_LOAD32_WORD( "p47-14.bin",  0x400000, 0x200000, 0xefc52b38 )
	ROM_LOAD32_WORD( "p47-03.bin",  0x800002, 0x200000, 0x324cd504 )
	ROM_LOAD32_WORD( "p47-15.bin",  0x800000, 0x200000, 0xca164b17 )
	ROM_LOAD32_WORD( "p47-04.bin",  0xc00002, 0x100000, 0x4b3372be )
	ROM_LOAD32_WORD( "p47-16.bin",  0xc00000, 0x100000, 0xc23c5467 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "p47-11.bin",  0x000000, 0x200000, 0xc1fe16b3 )
	ROM_LOAD( "p47-12.bin",  0x200000, 0x200000, 0x75871325 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "p47-10.bin",  0x000000, 0x200000, 0xa44e9e06 )
	ROM_LOAD( "p47-09.bin",  0x200000, 0x200000, 0x226014a6 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "p47-30.bin",  0x000000, 0x080000, 0x7ba90fad )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "p47-21.bin",  0x000000, 0x040000, 0xf2d43927 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples - 8-bit signed PCM */
	ROM_LOAD( "p47-22.bin",  0x000000, 0x200000, 0x0fa26f65 )
	ROM_LOAD( "p47-23.bin",  0x000000, 0x200000, 0x547fa4d4 )
ROM_END

ROM_START( tetrisp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "mr95024.26", 0x000003, 0x80000, 0xd318a9ba )
	ROM_LOAD32_BYTE( "mr95024.27", 0x000002, 0x80000, 0x2d69b6d3 )
	ROM_LOAD32_BYTE( "mr95024.28", 0x000001, 0x80000, 0x87522e16 )
	ROM_LOAD32_BYTE( "mr95024.29", 0x000000, 0x80000, 0x43a61941 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "mr95024.01", 0x000002, 0x200000, 0xcb0e92b9 )
	ROM_LOAD32_WORD( "mr95024.13", 0x000000, 0x200000, 0x4a825990 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95024.11", 0x000000, 0x200000, 0xc0d5246f )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95024.10", 0x000000, 0x200000, 0xa03e4a8d )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr95024.30", 0x000000, 0x080000, 0xcea7002d )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "mr95024.21", 0x000000, 0x040000, 0x5c565e3b )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "mr95024.22", 0x000000, 0x200000, 0x0fa26f65 ) // common samples
	ROM_LOAD( "mr95024.23", 0x200000, 0x200000, 0x57502a17 )
ROM_END

ROM_START( tp2m32 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
 	ROM_LOAD32_BYTE( "tp2m3226.26", 0x000003, 0x80000, 0x152f0ccf )
	ROM_LOAD32_BYTE( "tp2m3227.27", 0x000002, 0x80000, 0xd89468d0 )
	ROM_LOAD32_BYTE( "tp2m3228.28", 0x000001, 0x80000, 0x041aac23 )
	ROM_LOAD32_BYTE( "tp2m3229.29", 0x000000, 0x80000, 0x4e83b2ca )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	/* either there are two ROMs missing, or the other two are bad dumps */
	ROM_LOAD32_WORD( "sprite.1",    0x000002, 0x200000, 0x00000000 )
	ROM_LOAD32_WORD( "sprite.2",    0x000000, 0x200000, 0x00000000 )
	ROM_LOAD32_WORD( "tp2m3202.1",  0x400002, 0x200000, 0x0771a979 )
	ROM_LOAD32_WORD( "tp2m3201.13", 0x400000, 0x200000, 0xf128137b )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "tp2m3204.11", 0x000000, 0x200000, 0xb5a03129 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "tp2m3203.10", 0x000000, 0x200000, BADCRC( 0xf95aacf9 ) )	// odd bytes don't contain data

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "tp2m3230.30", 0x000000, 0x080000, 0x6845e476 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "tp2m3221.21", 0x000000, 0x040000, 0x2bcc4176 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* samples */
	ROM_LOAD( "tp2m3205.22", 0x000000, 0x200000, 0x74aa5c31 )
ROM_END


/********** DECRYPT **********/

/* 4 known types */

/* SS91022-10: desertwr, gratia, tp2m32, gametngk */

/* SS92046_01: bbbxing, f1superb, tetrisp, hayaosi1 */

/* SS92047-01: kirarast */

/* SS92048-01: p47aces */

static void rearrange_sprites(void)
{
	/* sprites are not encrypted, but we need to move the data around to handle them as 256x256 tiles */
	int i;
	unsigned char *source_data;
	int source_size;

	unsigned char *result_data;

	source_data = memory_region       ( REGION_GFX1 );
	source_size = memory_region_length( REGION_GFX1 );

	result_data = malloc(source_size);
	if (!result_data) return;

	for(i=0; i<source_size; i++)
	{
		int j = (i & ~0x07f8) | ((i & 0x00f8) << 3) | ((i & 0x0700) >> 5);

		result_data[i] = source_data[j];
	}

	memcpy (source_data, result_data, source_size);
	free (result_data);
}


static void decrypt_ms32_tx(int addr_xor,int data_xor)
{
	int i;
	unsigned char *source_data;
	int source_size;

	unsigned char *result_data;

	source_data = memory_region       ( REGION_GFX4 );
	source_size = memory_region_length( REGION_GFX4 );

	result_data = malloc(source_size);
	if (!result_data) return;

	addr_xor ^= 0x1005d;

	for(i=0; i<source_size; i++)
	{
		int j;

		/* two groups of cascading XORs for the address */
		j = 0;
		i ^= addr_xor;

		if (BIT(i,18)) j ^= 0x40000;	// 18
		if (BIT(i,17)) j ^= 0x60000;	// 17
		if (BIT(i, 7)) j ^= 0x70000;	// 16
		if (BIT(i, 3)) j ^= 0x78000;	// 15
		if (BIT(i,14)) j ^= 0x7c000;	// 14
		if (BIT(i,13)) j ^= 0x7e000;	// 13
		if (BIT(i, 0)) j ^= 0x7f000;	// 12
		if (BIT(i,11)) j ^= 0x7f800;	// 11
		if (BIT(i,10)) j ^= 0x7fc00;	// 10

		if (BIT(i, 9)) j ^= 0x00200;	//  9
		if (BIT(i, 8)) j ^= 0x00300;	//  8
		if (BIT(i,16)) j ^= 0x00380;	//  7
		if (BIT(i, 6)) j ^= 0x003c0;	//  6
		if (BIT(i,12)) j ^= 0x003e0;	//  5
		if (BIT(i, 4)) j ^= 0x003f0;	//  4
		if (BIT(i,15)) j ^= 0x003f8;	//  3
		if (BIT(i, 2)) j ^= 0x003fc;	//  2
		if (BIT(i, 1)) j ^= 0x003fe;	//  1
		if (BIT(i, 5)) j ^= 0x003ff;	//  0

		i ^= addr_xor;

		/* simple XOR for the data */
		result_data[i] = source_data[j] ^ (i & 0xff) ^ data_xor;
	}

	memcpy (source_data, result_data, source_size);
	free (result_data);
}

static void decrypt_ms32_bg(int addr_xor,int data_xor)
{
	int i;
	unsigned char *source_data;
	int source_size;

	unsigned char *result_data;

	source_data = memory_region       ( REGION_GFX3 );
	source_size = memory_region_length( REGION_GFX3 );

	result_data = malloc(source_size);
	if (!result_data) return;

	addr_xor ^= 0xc1c5b;

	for(i=0; i<source_size; i++)
	{
		int j;

		/* two groups of cascading XORs for the address */
		j = (i & ~0xfffff);	/* top bits are not affected */
		i ^= addr_xor;

		if (BIT(i,19)) j ^= 0x80000;	// 19
		if (BIT(i, 8)) j ^= 0xc0000;	// 18
		if (BIT(i,17)) j ^= 0xe0000;	// 17
		if (BIT(i, 2)) j ^= 0xf0000;	// 16
		if (BIT(i,15)) j ^= 0xf8000;	// 15
		if (BIT(i,14)) j ^= 0xfc000;	// 14
		if (BIT(i,13)) j ^= 0xfe000;	// 13
		if (BIT(i,12)) j ^= 0xff000;	// 12
		if (BIT(i, 1)) j ^= 0xff800;	// 11
		if (BIT(i,10)) j ^= 0xffc00;	// 10

		if (BIT(i, 9)) j ^= 0x00200;	//  9
		if (BIT(i, 3)) j ^= 0x00300;	//  8
		if (BIT(i, 7)) j ^= 0x00380;	//  7
		if (BIT(i, 6)) j ^= 0x003c0;	//  6
		if (BIT(i, 5)) j ^= 0x003e0;	//  5
		if (BIT(i, 4)) j ^= 0x003f0;	//  4
		if (BIT(i,18)) j ^= 0x003f8;	//  3
		if (BIT(i,16)) j ^= 0x003fc;	//  2
		if (BIT(i,11)) j ^= 0x003fe;	//  1
		if (BIT(i, 0)) j ^= 0x003ff;	//  0

		i ^= addr_xor;

		/* simple XOR for the data */
		result_data[i] = source_data[j] ^ (i & 0xff) ^ data_xor;
	}

	memcpy (source_data, result_data, source_size);
	free (result_data);
}



/* SS91022-10: desertwr, gratia, tp2m32, gametngk */
static DRIVER_INIT (ss91022_10)
{
	rearrange_sprites();
	decrypt_ms32_tx(0x00000,0x35);
	decrypt_ms32_bg(0x00000,0xa3);
}

/* SS92046_01: bbbxing, f1superb, tetrisp, hayaosi1 */
static DRIVER_INIT (ss92046_01)
{
	rearrange_sprites();
	decrypt_ms32_tx(0x00020,0x7e);
	decrypt_ms32_bg(0x00001,0x9b);
}

/* SS92047-01: kirarast */
static DRIVER_INIT (ss92047_01)
{
	rearrange_sprites();
	decrypt_ms32_tx(0x24000,0x18);
	decrypt_ms32_bg(0x24000,0x55);
}

/* SS92048-01: p47aces */
static DRIVER_INIT (ss92048_01)
{
	rearrange_sprites();
	decrypt_ms32_tx(0x20400,0xd6);
	decrypt_ms32_bg(0x20400,0xd4);
}

static DRIVER_INIT (kirarast)
{
//	{ 0xfcc00004, 0xfcc00007, ms32_mahjong_read_inputs1 }
	install_mem_read32_handler(0, 0xfcc00004, 0xfcc00007, ms32_mahjong_read_inputs1 );

	init_ss92047_01();
}

static DRIVER_INIT (tp2m32)
{
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);
	/* fix SBR register */
	pROM[0x1b848/4] &= 0x0000ffff;

	init_ss91022_10();
}

/********** GAME DRIVERS **********/

GAMEX( 1994, hayaosi1, 0,        ms32, hayaosi1, ss92046_01, ROT0,   "Jaleco", "Hayaoshi Quiz Ouza Ketteisen", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1994, bbbxing,  0,        ms32, bbbxing,  ss92046_01, ROT0,   "Jaleco", "Best Bout Boxing", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, desertwr, 0,        ms32, desertwr, ss91022_10, ROT270, "Jaleco", "Desert War", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1995, gametngk, 0,        ms32, gametngk, ss91022_10, ROT270, "Jaleco", "The Game Paradise - Master of Shooting! / Game Tengoku - The Game Paradise", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, tetrisp,  0,        ms32, tetrisp,  ss92046_01, ROT0,   "Jaleco / BPS", "Tetris Plus", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, p47aces,  0,        ms32, p47aces,  ss92048_01, ROT0,   "Jaleco", "P-47 Aces", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, gratia,   0,        ms32, gratia,   ss91022_10, ROT0,   "Jaleco", "Gratia - Second Earth", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, kirarast, 0,        ms32, kirarast, kirarast,   ROT0,   "Jaleco", "Ryuusei Janshi Kirara Star", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, tp2m32,   tetrisp2, ms32, ms32,     tp2m32,     ROT0,   "Jaleco", "Tetris Plus 2 (MegaSystem 32 Version)", GAME_NOT_WORKING | GAME_NO_SOUND )

/* these boot and show something */
GAMEX( 1994, f1superb, 0,        ms32, f1superb, ss92046_01, ROT0,   "Jaleco", "F1 Super Battle", GAME_NOT_WORKING | GAME_NO_SOUND )
