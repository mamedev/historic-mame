/***************************************************************************

Argus (Early NMK driver 1986-1987)
-------------------------------------
driver by Yochizo


Special thanks to :
=====================
 - Gerardo Oporto Jorrin for dipswitch informations.
 - Suzuki2gou for screenshots of Argus and Valtric.
 - Jarek Parchanski for your psychic5 driver.


Desctription about this hardware :
====================================================================
 This driver is for early NMK hardware such as Argus, Valtric, Psychic5
 and Butasan.  This hardware can be distinguished in terms of existence
 or no existence of paged RAM.  Argus and Valtric (both of them are
 released in 1986) does not exist paged RAM.  So these hardwares are
 called "TYPE A".  On the other hand, Psychic5 and maybe Butasan (both
 of them are released in 1987) does exist paged RAM. So these hardwares
 are called "TYPE B".


Supported games :
==================
TYPE A:
 Argus      (C) 1986 NMK / Jaleco
 Valtric    (C) 1986 NMK / Jaleco
TYPE B:


System specs :
===============
 CPU    : Z80 (6 MHz) x 2
 Sound  : YM2203 x 2 (Only Argus is single YM2203)
 Layers : BG, sprite, text       (Valtric, Psychic5, maybe Butasan)
          BG0, BG1, sprite, text (Argus)
 Color  : 768 colors (Valtric, Psychic5, maybe Butasan)
           Sprite : 256 colors
           BG     : 256 colors
           Text   : 256 colors
        : 832 colors (Argus)
           Sprite : 128 colors
           BG0    : 256 colors
           BG1    : 256 colors
           Text   : 256 colors
 Others : This hardware has several special effects.
            - Half transparent color (Not emulated yet)
            - Brightness controller  (Emulated)
            - Gray scale effect      (Does it use in Argus or Valtric ?)
            - Mosaic effect          (Not emulated yet. Maybe it is used in only Valtric)
            - Argus has VROMs which is used for the map-pattern in background.


Note :
=======
 - To enter test mode, press coin 2 key at start.


Known issues :
===============
 - Mosaic effect in Valtric is not implemented.
 - Half transparent color (50% alpha blending) is not emulated.
 - I am not sure that Argus or Valtric use gray scale effect or not.
 - In test mode, most roms have check sum error. Why ?
 - Dip switches are not completed.


****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"



/***************************************************************************

  Variable

***************************************************************************/

extern data8_t *argus_paletteram;
extern data8_t *argus_txram;
extern data8_t *argus_bg0ram;
extern data8_t *argus_bg1ram;
extern data8_t *argus_bg0_scrollx;
extern data8_t *argus_bg0_scrolly;
extern data8_t *argus_bg1_scrollx;
extern data8_t *argus_bg1_scrolly;

extern data8_t *spriteram_new;
extern size_t spriteram_size;

int  argus_vh_start(void);
void argus_vh_stop(void);
int  valtric_vh_start(void);
void argus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void valtric_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int argus_bank_latch = 0x00;

READ_HANDLER( argus_txram_r );
READ_HANDLER( argus_paletteram_r );
READ_HANDLER( argus_bg0ram_r );
READ_HANDLER( argus_bg1ram_r );
WRITE_HANDLER( argus_txram_w );
WRITE_HANDLER( argus_bg0ram_w );
WRITE_HANDLER( argus_bg1ram_w );
WRITE_HANDLER( argus_paletteram_w );
WRITE_HANDLER( valtric_paletteram_w );
WRITE_HANDLER( argus_bg0_scrollx_w );
WRITE_HANDLER( argus_bg0_scrolly_w );
WRITE_HANDLER( argus_bg1_scrollx_w );
WRITE_HANDLER( argus_bg1_scrolly_w );
WRITE_HANDLER( argus_bg_veffect_w );
WRITE_HANDLER( argus_flipscreen_w );


/***************************************************************************

  Interrupt(s)

***************************************************************************/

int argus_interrupt(void)
{
	if (cpu_getiloops() == 0)
	   return 0xd7;		/* RST 10h */
	else
	   return 0xcf;		/* RST 08h */
}

/* Handler called by the YM2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203x1_interface =
{
	1,				/* 1 chip  */
	6000000 / 4, 	/* 1.5 MHz */
	{ YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface ym2203x2_interface =
{
	2,				/* 2 chips */
	6000000 / 4, 	/* 1.5 MHz */
	{ YM2203_VOL(50,15), YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static READ_HANDLER( argus_bankselect_r )
{
	return argus_bank_latch;
}

WRITE_HANDLER( argus_bankselect_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	if (data != argus_bank_latch)
	{
		argus_bank_latch = data;
		bankaddress = 0x10000 + ((data & 7) * 0x4000);
		cpu_setbank(1, &RAM[bankaddress]);	 /* Select 8 banks of 16k */
	}
}


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static MEMORY_READ_START( argus_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },			// Coin
	{ 0xc001, 0xc001, input_port_1_r },			// Player 1
	{ 0xc002, 0xc002, input_port_2_r },			// Player 2
	{ 0xc003, 0xc003, input_port_3_r },			// DSW 1
	{ 0xc004, 0xc004, input_port_4_r },			// DSW 2
	{ 0xc400, 0xcfff, argus_paletteram_r, },
	{ 0xd000, 0xd7ff, argus_txram_r },
	{ 0xd800, 0xdfff, argus_bg1ram_r },
	{ 0xe000, 0xf1ff, MRA_RAM },
	{ 0xf200, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( argus_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, argus_flipscreen_w },		// flip screen
	{ 0xc202, 0xc202, argus_bankselect_w },
	{ 0xc300, 0xc301, argus_bg0_scrollx_w, &argus_bg0_scrollx },
	{ 0xc302, 0xc303, argus_bg0_scrolly_w, &argus_bg0_scrolly },
	{ 0xc308, 0xc309, argus_bg1_scrollx_w, &argus_bg1_scrollx },
	{ 0xc30a, 0xc30b, argus_bg1_scrolly_w, &argus_bg1_scrolly },
	{ 0xc30c, 0xc30c, argus_bg_veffect_w },		// video effect bits
	{ 0xc400, 0xcfff, argus_paletteram_w, &argus_paletteram },
	{ 0xd000, 0xd7ff, argus_txram_w, &argus_txram },
	{ 0xd800, 0xdfff, argus_bg1ram_w, &argus_bg1ram },
	{ 0xe000, 0xf1ff, MWA_RAM },
	{ 0xf200, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( valtric_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },			// Coin
	{ 0xc001, 0xc001, input_port_1_r },			// Player 1
	{ 0xc002, 0xc002, input_port_2_r },			// Player 2
	{ 0xc003, 0xc003, input_port_3_r },			// DSW 1
	{ 0xc004, 0xc004, input_port_4_r },			// DSW 2
	{ 0xc400, 0xcfff, argus_paletteram_r, },
	{ 0xd000, 0xd7ff, argus_txram_r },
	{ 0xd800, 0xdfff, argus_bg1ram_r },
	{ 0xe000, 0xf1ff, MRA_RAM },
	{ 0xf200, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( valtric_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, argus_flipscreen_w },		// flip screen
	{ 0xc202, 0xc202, argus_bankselect_w },
	{ 0xc308, 0xc309, argus_bg1_scrollx_w, &argus_bg1_scrollx },
	{ 0xc30a, 0xc30b, argus_bg1_scrolly_w, &argus_bg1_scrolly },
	{ 0xc30c, 0xc30c, argus_bg_veffect_w },		// video effect bits
	{ 0xc400, 0xcfff, valtric_paletteram_w, &argus_paletteram },
	{ 0xd000, 0xd7ff, argus_txram_w, &argus_txram },
	{ 0xd800, 0xdfff, argus_bg1ram_w, &argus_bg1ram },
	{ 0xe000, 0xf1ff, MWA_RAM },
	{ 0xf200, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

/* sound cpu is complete */
static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport_1 )
	{ 0x0000, 0x0000, YM2203_status_port_0_r },
	{ 0x0001, 0x0001, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport_1 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

static PORT_READ_START( sound_readport_2 )
	{ 0x0000, 0x0000, YM2203_status_port_0_r },
	{ 0x0001, 0x0001, YM2203_read_port_0_r },
	{ 0x0080, 0x0080, YM2203_status_port_1_r },
	{ 0x0081, 0x0081, YM2203_read_port_1_r },
PORT_END

static PORT_WRITE_START( sound_writeport_2 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
PORT_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( argus )
    PORT_START      /* System control (0) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START      /* Player 1 control (1) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START	/* Player 2 controls (2) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* DSW 1 (3) */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "Unknown 1-2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "Unknown 1-3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x80, "2" )
    PORT_DIPSETTING(    0xc0, "3" )
    PORT_DIPSETTING(    0x40, "4" )
    PORT_DIPSETTING(    0x00, "5" )

    PORT_START  /* DSW 2 (4) */
    PORT_DIPNAME( 0x01, 0x01, "Unknown 2-1" )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "Unknown 2-2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x0C, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x1C, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
    PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A) )
    PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( valtric )
    PORT_START      /* System control (0) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START      /* Player 1 control (1) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START	/* Player 2 controls (2) */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* DSW 1 (3) */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x06, "30k, 100k and every 100k" )
    PORT_DIPSETTING(    0x04, "30k, 120k and every 120k" )
    PORT_DIPSETTING(    0x02, "40k, 140k and every 140k" )
    PORT_DIPSETTING(    0x00, "60k, 180k and every 180k" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPSETTING(    0xc0, "3" )
    PORT_DIPSETTING(    0x80, "4" )
    PORT_DIPSETTING(    0x40, "5" )

    PORT_START  /* DSW 2 (4) */
    PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Medium" )
    PORT_DIPSETTING(    0x00, "Hard" )
    PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x0C, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x1C, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
    PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,    /* 8x8 characters */
	1024,	/* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16x16 characters */
	1024,	/* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout0 =
{
	16,16,  /* 16x16 characters */
	256,	/* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout1 =
{
	16,16,  /* 16x16 characters */
	2048,	/* 2048 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxDecodeInfo argus_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0*16,   8 },
	{ REGION_GFX2, 0, &spritelayout, 8*16,  16 },
	{ REGION_GFX3, 0, &tilelayout0,  24*16, 16 },
	{ REGION_GFX4, 0, &charlayout,   40*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo valtric_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0*16,  16 },
	{ REGION_GFX2, 0, &tilelayout1,  16*16, 16 },
	{ REGION_GFX3, 0, &charlayout,   32*16, 16 },
	{ -1 } /* end of array */
};

static const struct MachineDriver machine_driver_argus =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			12000000 / 2,		/* 6 MHz */
			argus_readmem, argus_writemem, 0, 0,
			argus_interrupt, 2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			12000000 / 2,		/* 6 MHz */
			sound_readmem,    sound_writemem,
			sound_readport_1, sound_writeport_1,
			ignore_interrupt, 0
		}
	},
	54, DEFAULT_60HZ_VBLANK_DURATION,	/* this value is refered to psychic5 driver */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0*8, 32*8-1, 2*8, 30*8-1 },
	argus_gfxdecodeinfo,
	56*16, 56*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	argus_vh_start,
	argus_vh_stop,
	argus_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203x1_interface
		}
	}
};

static const struct MachineDriver machine_driver_valtric =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			12000000 / 2,		/* 6 MHz */
			valtric_readmem, valtric_writemem, 0, 0,
			argus_interrupt, 2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			12000000 / 2,		/* 6 MHz */
			sound_readmem,    sound_writemem,
			sound_readport_2, sound_writeport_2,
			ignore_interrupt, 0
		}
	},
	54, DEFAULT_60HZ_VBLANK_DURATION,	/* this value is refered to psychic5 driver */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0*8, 32*8-1, 2*8, 30*8-1 },
	valtric_gfxdecodeinfo,
	48*16, 48*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	valtric_vh_start,
	0,
	valtric_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203x2_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( argus )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) 					/* Main CPU */
	ROM_LOAD( "ag_02.bin", 0x00000, 0x08000, 0x278a3f3d )
	ROM_LOAD( "ag_03.bin", 0x10000, 0x08000, 0x3a7f3bfa )
	ROM_LOAD( "ag_04.bin", 0x18000, 0x08000, 0x76adc9f6 )
	ROM_LOAD( "ag_05.bin", 0x20000, 0x08000, 0xf76692d6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )					/* Sound CPU */
	ROM_LOAD( "ag_01.bin", 0x00000, 0x04000, 0x769e3f57 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "ag_09.bin", 0x00000, 0x08000, 0x6dbc1c58 )
	ROM_LOAD( "ag_08.bin", 0x08000, 0x08000, 0xce6e987e )
	ROM_LOAD( "ag_07.bin", 0x10000, 0x08000, 0xbbb9638d )
	ROM_LOAD( "ag_06.bin", 0x18000, 0x08000, 0x655b48f8 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* BG0 */
	ROM_LOAD( "ag_13.bin", 0x00000, 0x08000, 0x20274268 )
	ROM_LOAD( "ag_14.bin", 0x08000, 0x08000, 0xceb8860b )
	ROM_LOAD( "ag_11.bin", 0x10000, 0x08000, 0x99ce8556 )
	ROM_LOAD( "ag_12.bin", 0x18000, 0x08000, 0xe0e5377c )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )	/* BG1 */
	ROM_LOAD( "ag_17.bin", 0x00000, 0x08000, 0x0f12d09b )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "ag_10.bin", 0x00000, 0x04000, 0x2de696c4 )

	ROM_REGION( 0x08000, REGION_USER1, 0 )					/* Map */
	ROM_LOAD( "ag_15.bin", 0x00000, 0x08000, 0x99834c1b )

	ROM_REGION( 0x08000, REGION_USER2, 0 )					/* Pattern */
	ROM_LOAD( "ag_16.bin", 0x00000, 0x08000, 0x39a51714 )
ROM_END

ROM_START( valtric )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) 					/* Main CPU */
	ROM_LOAD( "vt_04.bin",    0x00000, 0x08000, 0x709c705f )
	ROM_LOAD( "vt_06.bin",    0x10000, 0x10000, 0xc9cbb4e4 )
	ROM_LOAD( "vt_05.bin",    0x20000, 0x10000, 0x7ab2684b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )					/* Sound CPU */
	ROM_LOAD( "vt_01.bin",    0x00000, 0x08000, 0x4616484f )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "vt_02.bin",    0x00000, 0x10000, 0x66401977 )
	ROM_LOAD( "vt_03.bin",    0x10000, 0x10000, 0x9203bbce )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* BG */
	ROM_LOAD( "vt_08.bin",    0x00000, 0x10000, 0x661dd338 )
	ROM_LOAD( "vt_09.bin",    0x10000, 0x10000, 0x085a35b1 )
	ROM_LOAD( "vt_10.bin",    0x20000, 0x10000, 0x09c47323 )
	ROM_LOAD( "vt_11.bin",    0x30000, 0x10000, 0x4cf800b5 )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "vt_07.bin",    0x00000, 0x08000, 0xd5f9bfb9 )
ROM_END


/*  ( YEAR   NAME    PARENT MACHINE  INPUT    INIT MONITOR COMPANY                 FULLNAME ) */
GAME( 1986, argus,   0,     argus,   argus,   0,   ROT270, "[NMK] (Jaleco license)", "Argus"   )
GAME( 1986, valtric, 0,     valtric, valtric, 0,   ROT270, "[NMK] (Jaleco license)", "Valtric" )
