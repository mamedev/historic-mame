/***************************************************************************

The New Zealand Story driver, used for tnzs & tnzs2.

TODO: - Find out how the hardware credit-counter works (MPU)
      - Verify dip switches
	Arkanoid 2:
      - What do writes at f400 do ?
      - Why does the game zero the fd00 area ?
	Extrmatn:
      - What do reads from f600 do ? (discarded)

****************************************************************************

extrmatn and arkanoi2 have a special test mode. The correct procedure to make
it succeed is as follows:
- enter service mode
- on the color test screen, press 2
- set dip switch 1 so that it reads 00000001
- press 3. Text at the bottom will change to "CHECKING NOW".
- use all the inputs, including tilt, until all inputs are OK
- set dip switch 1 to 00000000
- set dip switch 1 to 10101010
- set dip switch 1 to 11111111
- set dip switch 2 to 00000000
- set dip switch 2 to 10101010
- set dip switch 2 to 11111111
- press 1 (to confirm that coin lockout 1 works)
- press 2 (to confirm that coin lockout 2 works)
- press 1 (to confirm that OPN works)
- press 1 (to confirm that SSGCH1 works)
- press 1 (to confirm that SSGCH2 works)
- press 1 (to confirm that SSGCH3 works)
- finished ("CHECK ALL OK!")

****************************************************************************

The New Zealand Story memory map (preliminary)

CPU #1
0000-7fff ROM
8000-bfff banked - banks 0-1 RAM; banks 2-7 ROM
c000-dfff object RAM, including:
  c000-c1ff sprites (code, low byte)
  c200-c3ff sprites (x-coord, low byte)
  c400-c5ff tiles (code, low byte)

  d000-d1ff sprites (code, high byte)
  d200-d3ff sprites (x-coord and colour, high byte)
  d400-d5ff tiles (code, high byte)
  d600-d7ff tiles (colour)
e000-efff RAM shared with CPU #2
f000-ffff VDC RAM, including:
  f000-f1ff sprites (y-coord)
  f200-f2ff scrolling info
  f300-f301 vdc controller
  f302-f303 scroll x-coords (high bits)
  f600      bankswitch
  f800-fbff palette

CPU #2
0000-7fff ROM
8000-9fff banked ROM
a000      bankswitch
b000-b001 YM2203 interface (with DIPs on YM2203 ports)
c000-c001 I8742 MCU
e000-efff RAM shared with CPU #1
f000-f003 inputs (used only by Arkanoid 2)

****************************************************************************/
/***************************************************************************

				Arkanoid 2 - Revenge of Doh!
				    (C) 1987 Taito

					    driver by

				Luca Elia (eliavit@unina.it)
				Mirko Buffoni

- The game doesn't write to f800-fbff (static palette)



			Interesting routines (main cpu)
			-------------------------------

1ed	prints the test screen (first string at 206)

47a	prints dipsw1&2 e 1p&2p paddleL values:
	e821		IN DIPSW1		e823-4	1P PaddleL (lo-hi)
	e822		IN DIPSW2		e825-6	2P PaddleL (lo-hi)

584	prints OK or NG on each entry:
	if (*addr)!=0 { if (*addr)!=2 OK else NG }
	e880	1P PADDLEL		e88a	IN SERVICE
	e881	1P PADDLER		e88b	IN TILT
	e882	1P BUTTON		e88c	OUT LOCKOUT1
	e883	1P START		e88d	OUT LOCKOUT2
	e884	2P PADDLEL		e88e	IN DIP-SW1
	e885	2P PADDLER		e88f	IN DIP-SW2
	e886	2P BUTTON		e890	SND OPN
	e887	2P START		e891	SND SSGCH1
	e888	IN COIN1		e892	SND SSGCH2
	e889	IN COIN2		e893	SND SSGCH3

672	prints a char
715	prints a string (0 terminated)

		Shared Memory (values written mainly by the sound cpu)
		------------------------------------------------------

e001=dip-sw A 	e399=coin counter value		e72c-d=1P paddle (lo-hi)
e002=dip-sw B 	e3a0-2=1P score/10 (BCD)	e72e-f=2P paddle (lo-hi)
e008=level=2*(shown_level-1)+x <- remember it's a binary tree (42 last)
e7f0=country code(from 9fde in sound rom)
e807=counter, reset by sound cpu, increased by main cpu each vblank
e80b=test progress=0(start) 1(first 8) 2(all ok) 3(error)
ec09-a~=ed05-6=xy pos of cursor in hi-scores
ec81-eca8=hi-scores(8bytes*5entries)

addr	bit	name		active	addr	bit	name		active
e72d	6	coin[1]		low		e729	1	2p select	low
		5	service		high			0	1p select	low
		4	coin[2]		low

addr	bit	name		active	addr	bit	name		active
e730	7	tilt		low		e7e7	4	1p fire		low
										0	2p fire		low

			Interesting routines (sound cpu)
			--------------------------------

4ae	check starts	B73,B7a,B81,B99	coin related
8c1	check coins		62e lockout check		664	dsw check

			Interesting locations (sound cpu)
			---------------------------------

d006=each bit is on if a corresponding location (e880-e887) has changed
d00b=(c001)>>4=tilt if 0E (security sequence must be reset?)
addr	bit	name		active
d00c	7	tilt
		6	?service?
		5	coin2		low
		4	coin1		low

d00d=each bit is on if the corresponding location (e880-e887) is 1 (OK)
d00e=each of the 4 MSBs is on if ..
d00f=FF if tilt, 00 otherwise
d011=if 00 checks counter, if FF doesn't
d23f=input port 1 value

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/* prototypes for functions in ../machine/tnzs.c */
unsigned char *tnzs_objram, *tnzs_workram;
unsigned char *tnzs_vdcram, *tnzs_scrollram;
void extrmatn_init(void);
void arkanoi2_init(void);
void tnzs_init(void);
void insectx_init(void);
int arkanoi2_sh_f000_r(int offs);
void tnzs_init_machine(void);
int tnzs_interrupt (void);
int tnzs_mcu_r(int offset);
int tnzs_workram_r(int offset);
void tnzs_mcu_w(int offset, int data);
void tnzs_workram_w(int offset, int data);
void tnzs_bankswitch_w(int offset, int data);
void tnzs_bankswitch1_w(int offset, int data);


/* prototypes for functions in ../vidhrdw/tnzs.c */
int tnzs_vh_start(void);
void tnzs_vh_stop(void);
void arkanoi2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void arkanoi2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 }, /* ROM + RAM */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_r },	/* WORK RAM (shared by the 2 z80's */
	{ 0xf000, 0xf1ff, MRA_RAM },	/* VDC RAM */
	{ 0xf800, 0xfbff, MRA_RAM },	/* not in extrmatn and arkanoi2 (PROMs instead) */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },	/* ROM + RAM */
	{ 0xc000, 0xdfff, MWA_RAM, &tnzs_objram },
	{ 0xe000, 0xefff, tnzs_workram_w, &tnzs_workram },
	{ 0xf000, 0xf1ff, MWA_RAM, &tnzs_vdcram },
	{ 0xf200, 0xf3ff, MWA_RAM, &tnzs_scrollram }, /* scrolling info */
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
	{ 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },	/* not in extrmatn and arkanoi2 (PROMs instead) */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sub_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xc000, 0xc001, tnzs_mcu_r },	/* plain input ports in insectx (memory handler */
									/* changed in insectx_init() ) */
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf003, arkanoi2_sh_f000_r },	/* paddles in arkanoid2; the ports are */
						/* read but not used by the other games, and are not read at */
						/* all by insectx. */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sub_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xc000, 0xc001, tnzs_mcu_w },	/* not present in insectx */
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_w },
	{ -1 }  /* end of table */
};


/* the bootleg board is different, it has a third CPU (and of course no mcu) */

static void tnzsb_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,0xff);
}

static struct MemoryReadAddress tnzsb_readmem1[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb002, 0xb002, input_port_0_r },
	{ 0xb003, 0xb003, input_port_1_r },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_3_r },
	{ 0xc002, 0xc002, input_port_4_r },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf003, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress tnzsb_writemem1[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb004, 0xb004, tnzsb_sound_command_w },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_w },
	{ 0xf000, 0xf3ff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress tnzsb_readmem2[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress tnzsb_writemem2[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort tnzsb_readport[] =
{
	{ 0x00, 0x00, YM2203_status_port_0_r  },
	{ 0x02, 0x02, soundlatch_r  },
	{ -1 }	/* end of table */
};

static struct IOWritePort tnzsb_writeport[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w  },
	{ 0x01, 0x01, YM2203_write_port_0_w  },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( extrmatn_input_ports )
	PORT_START      /* DSW A */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW B */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( arkanoi2_input_ports )
	PORT_START	/* DSW1 - IN2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DSW2 - IN3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k 150k" )
	PORT_DIPSETTING(    0x0c, "100k 200k" )
	PORT_DIPSETTING(    0x04, "50k Only" )
	PORT_DIPSETTING(    0x08, "100k Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START      /* IN1 - read at c000 (sound cpu) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* spinner 1 - read at f000/1 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL, 70, 15, 0, 0, 0 )
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT   ( 0x2000, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT   ( 0x4000, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW,  IPT_TILT )	/* arbitrarily assigned, handled by the mcu */

	PORT_START      /* spinner 2 - read at f002/3 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER2, 70, 15, 0, 0, 0 )
	PORT_BIT   ( 0xf000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ark2us_input_ports )
	PORT_START	/* DSW1 - IN2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW2 - IN3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k 150k" )
	PORT_DIPSETTING(    0x0c, "100k 200k" )
	PORT_DIPSETTING(    0x04, "50k Only" )
	PORT_DIPSETTING(    0x08, "100k Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START      /* IN1 - read at c000 (sound cpu) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* spinner 1 - read at f000/1 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL, 70, 15, 0, 0, 0 )
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT   ( 0x2000, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT   ( 0x4000, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW,  IPT_TILT )	/* arbitrarily assigned, handled by the mcu */

	PORT_START      /* spinner 2 - read at f002/3 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER2, 70, 15, 0, 0, 0 )
	PORT_BIT   ( 0xf000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tnzs_input_ports )
	PORT_START      /* DSW A */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW B */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "50000 150000" )
    PORT_DIPSETTING(    0x0c, "70000 200000" )
    PORT_DIPSETTING(    0x04, "100000 250000" )
    PORT_DIPSETTING(    0x08, "200000 300000" )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x20, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tnzsb_input_ports )
	PORT_START      /* DSW A */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START      /* DSW B */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "50000 150000" )
    PORT_DIPSETTING(    0x0c, "70000 200000" )
    PORT_DIPSETTING(    0x04, "100000 250000" )
    PORT_DIPSETTING(    0x08, "200000 300000" )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x20, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tnzs2_input_ports )
	PORT_START      /* DSW A */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START      /* DSW B */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "10000 100000" )
    PORT_DIPSETTING(    0x0c, "10000 150000" )
    PORT_DIPSETTING(    0x08, "10000 200000" )
    PORT_DIPSETTING(    0x04, "10000 300000" )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x20, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( insectx_input_ports )
	PORT_START      /* DSW A */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x08, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START      /* DSW B */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x00, "1" )
    PORT_DIPSETTING(    0x10, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x20, "4" )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout arkanoi2_charlayout =
{
	16,16,
	4096,
	4,
	{ 3*4096*32*8, 2*4096*32*8, 1*4096*32*8, 0*4096*32*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxLayout tnzs_charlayout =
{
	16,16,
	8192,
	4,
	{ 3*8192*32*8, 2*8192*32*8, 1*8192*32*8, 0*8192*32*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxLayout insectx_charlayout =
{
	16,16,
	8192,
	4,
	{ 8, 0, 8192*64*8+8, 8192*64*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo arkanoi2_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &arkanoi2_charlayout, 0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo tnzs_gfxdecodeinfo[] =
{
    { 1, 0x0000, &tnzs_charlayout, 0, 32 },
	{ -1 }	/* end of array */
};

static struct GfxDecodeInfo insectx_gfxdecodeinfo[] =
{
    { 1, 0x0000, &insectx_charlayout, 0, 32 },
	{ -1 }	/* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(30,30) },
	AY8910_DEFAULT_GAIN,
	{ input_port_0_r },		/* DSW1 connected to port A */
	{ input_port_1_r },		/* DSW2 connected to port B */
	{ 0 },
	{ 0 }
};


/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_nmi_line(2,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203b_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(100,100) },
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static struct MachineDriver arkanoi2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ?? Hz (only crystal is 12MHz) */
						/* 8MHz is wrong, but extrmatn doesn't work properly at 6MHz */
			0,			/* memory region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
			6000000,	/* ?? Hz */
			2,			/* memory region */
			sub_readmem,sub_writemem,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,		/* video frequency (Hz), duration */
	100,							/* cpu slices */
	tnzs_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	arkanoi2_gfxdecodeinfo,
	512, 512,
	arkanoi2_vh_convert_color_prom,		/* convert color p-roms */

	VIDEO_TYPE_RASTER,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	arkanoi2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver tnzs_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,		/* 6 Mhz(?) */
			0,			/* memory_region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
			6000000,        /* 6 Mhz(?) */
			2,			/* memory_region */
			sub_readmem,sub_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	tnzs_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	tnzs_gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver tnzsb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,		/* 6 Mhz(?) */
			0,			/* memory_region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
			6000000,        /* 6 Mhz(?) */
			2,			/* memory_region */
			tnzsb_readmem1,tnzsb_writemem1,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,        /* 4 Mhz??? */
			3,			/* memory_region */
			tnzsb_readmem2,tnzsb_writemem2,tnzsb_readport,tnzsb_writeport,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	tnzs_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	tnzs_gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203b_interface
		}
	}
};

static struct MachineDriver insectx_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,		/* 6 Mhz(?) */
			0,			/* memory_region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
			6000000,        /* 6 Mhz(?) */
			2,			/* memory_region */
			sub_readmem,sub_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	tnzs_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	insectx_gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( extrmatn_rom )
	ROM_REGION(0x30000)				/* Region 0 - main cpu */
	ROM_LOAD( "b06-20.bin", 0x00000, 0x08000, 0x04e3fc1f )
    ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b06-21.bin", 0x20000, 0x10000, 0x1614d6a2 )	/* banked at 8000-bfff */

	ROM_REGION_DISPOSE(0x80000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "b06-01.bin", 0x00000, 0x20000, 0xd2afbf7e )
	ROM_LOAD( "b06-02.bin", 0x20000, 0x20000, 0xe0c2757a )
	ROM_LOAD( "b06-03.bin", 0x40000, 0x20000, 0xee80ab9d )
	ROM_LOAD( "b06-04.bin", 0x60000, 0x20000, 0x3697ace4 )

	ROM_REGION(0x18000)				/* Region 2 - sound cpu */
	ROM_LOAD( "b06-06.bin", 0x00000, 0x08000, 0x744f2c84 )
	ROM_CONTINUE(           0x10000, 0x08000 )	/* banked at 8000-9fff */

	ROM_REGION(0x400)				/* Region 3 - color proms */
	ROM_LOAD( "b06-09.bin", 0x00000, 0x200, 0xf388b361 )	/* hi bytes */
	ROM_LOAD( "b06-08.bin", 0x00200, 0x200, 0x10c9aac3 )	/* lo bytes */
ROM_END

ROM_START( arkanoi2_rom )
	ROM_REGION(0x30000)				/* Region 0 - main cpu */
	ROM_LOAD( "a2-05.rom",  0x00000, 0x08000, 0x136edf9d )
    ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	/* 20000-2ffff empty */

	ROM_REGION_DISPOSE(0x80000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "a2-m01.bin", 0x00000, 0x20000, 0x2ccc86b4 )
	ROM_LOAD( "a2-m02.bin", 0x20000, 0x20000, 0x056a985f )
	ROM_LOAD( "a2-m03.bin", 0x40000, 0x20000, 0x274a795f )
	ROM_LOAD( "a2-m04.bin", 0x60000, 0x20000, 0x9754f703 )

	ROM_REGION(0x18000)				/* Region 2 - sound cpu */
	ROM_LOAD( "a2-13.rom",  0x00000, 0x08000, 0xe8035ef1 )
	ROM_CONTINUE(           0x10000, 0x08000 )	/* banked at 8000-9fff */

	ROM_REGION(0x400)				/* Region 3 - color proms */
	ROM_LOAD( "b08-08.bin", 0x00000, 0x200, 0xa4f7ebd9 )	/* hi bytes */
	ROM_LOAD( "b08-07.bin", 0x00200, 0x200, 0xea34d9f7 )	/* lo bytes */
ROM_END

ROM_START( ark2us_rom )
	ROM_REGION(0x30000)				/* Region 0 - main cpu */
	ROM_LOAD( "b08-11.bin", 0x00000, 0x08000, 0x99555231 )
    ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	/* 20000-2ffff empty */

	ROM_REGION_DISPOSE(0x80000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "a2-m01.bin", 0x00000, 0x20000, 0x2ccc86b4 )
	ROM_LOAD( "a2-m02.bin", 0x20000, 0x20000, 0x056a985f )
	ROM_LOAD( "a2-m03.bin", 0x40000, 0x20000, 0x274a795f )
	ROM_LOAD( "a2-m04.bin", 0x60000, 0x20000, 0x9754f703 )

	ROM_REGION(0x18000)				/* Region 2 - sound cpu */
	ROM_LOAD( "b08-12.bin", 0x00000, 0x08000, 0xdc84e27d )
	ROM_CONTINUE(           0x10000, 0x08000 )	/* banked at 8000-9fff */

	ROM_REGION(0x400)				/* Region 3 - color proms */
	ROM_LOAD( "b08-08.bin", 0x00000, 0x200, 0xa4f7ebd9 )	/* hi bytes */
	ROM_LOAD( "b08-07.bin", 0x00200, 0x200, 0xea34d9f7 )	/* lo bytes */
ROM_END

ROM_START( tnzs_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "nzsb5310.bin", 0x00000, 0x08000, 0xa73745c6 )
    ROM_CONTINUE(             0x18000, 0x18000 )	/* banked at 8000-bfff */

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	/* ROMs taken from another set (the ones from this set were read incorrectly) */
	ROM_LOAD( "nzsb5316.bin", 0x00000, 0x20000, 0xc3519c2a )
	ROM_LOAD( "nzsb5317.bin", 0x20000, 0x20000, 0x2bf199e8 )
	ROM_LOAD( "nzsb5318.bin", 0x40000, 0x20000, 0x92f35ed9 )
	ROM_LOAD( "nzsb5319.bin", 0x60000, 0x20000, 0xedbb9581 )
	ROM_LOAD( "nzsb5322.bin", 0x80000, 0x20000, 0x59d2aef6 )
	ROM_LOAD( "nzsb5323.bin", 0xa0000, 0x20000, 0x74acfb9b )
	ROM_LOAD( "nzsb5320.bin", 0xc0000, 0x20000, 0x095d0dc0 )
	ROM_LOAD( "nzsb5321.bin", 0xe0000, 0x20000, 0x9800c54d )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "nzsb5311.bin", 0x00000, 0x08000, 0x9784d443 )
	ROM_CONTINUE(             0x10000, 0x08000 )	/* banked at 8000-9fff */
ROM_END

ROM_START( tnzsb_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "nzsb5324.bin", 0x00000, 0x08000, 0xd66824c6 )
    ROM_CONTINUE(             0x18000, 0x18000 )	/* banked at 8000-bfff */

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	/* ROMs taken from another set (the ones from this set were read incorrectly) */
	ROM_LOAD( "nzsb5316.bin", 0x00000, 0x20000, 0xc3519c2a )
	ROM_LOAD( "nzsb5317.bin", 0x20000, 0x20000, 0x2bf199e8 )
	ROM_LOAD( "nzsb5318.bin", 0x40000, 0x20000, 0x92f35ed9 )
	ROM_LOAD( "nzsb5319.bin", 0x60000, 0x20000, 0xedbb9581 )
	ROM_LOAD( "nzsb5322.bin", 0x80000, 0x20000, 0x59d2aef6 )
	ROM_LOAD( "nzsb5323.bin", 0xa0000, 0x20000, 0x74acfb9b )
	ROM_LOAD( "nzsb5320.bin", 0xc0000, 0x20000, 0x095d0dc0 )
	ROM_LOAD( "nzsb5321.bin", 0xe0000, 0x20000, 0x9800c54d )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "nzsb5325.bin", 0x00000, 0x08000, 0xd6ac4e71 )
	ROM_CONTINUE(             0x10000, 0x08000 )	/* banked at 8000-9fff */

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "nzsb5326.bin", 0x00000, 0x10000, 0xcfd5649c )
ROM_END

ROM_START( tnzs2_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "ns_c-11.rom",  0x00000, 0x08000, 0x3c1dae7b )
    ROM_CONTINUE(             0x18000, 0x18000 )	/* banked at 8000-bfff */

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ns_a13.rom",   0x00000, 0x20000, 0x7e0bd5bb )
    ROM_LOAD( "ns_a12.rom",   0x20000, 0x20000, 0x95880726 )
    ROM_LOAD( "ns_a10.rom",   0x40000, 0x20000, 0x2bc4c053 )
    ROM_LOAD( "ns_a08.rom",   0x60000, 0x20000, 0x8ff8d88c )
    ROM_LOAD( "ns_a07.rom",   0x80000, 0x20000, 0x291bcaca )
    ROM_LOAD( "ns_a05.rom",   0xa0000, 0x20000, 0x6e762e20 )
    ROM_LOAD( "ns_a04.rom",   0xc0000, 0x20000, 0xe1fd1b9d )
    ROM_LOAD( "ns_a02.rom",   0xe0000, 0x20000, 0x2ab06bda )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "ns_e-3.rom",   0x00000, 0x08000, 0xc7662e96 )
    ROM_CONTINUE(             0x10000, 0x08000 )
ROM_END

ROM_START( insectx_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "insector.u32", 0x00000, 0x08000, 0x18eef387 )
    ROM_CONTINUE(             0x18000, 0x18000 )	/* banked at 8000-bfff */

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "insector.r15", 0x00000, 0x80000, 0xd00294b1 )
	ROM_LOAD( "insector.r16", 0x80000, 0x80000, 0xdb5a7434 )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "insector.u38", 0x00000, 0x08000, 0x324b28c9 )
	ROM_CONTINUE(             0x10000, 0x08000 )	/* banked at 8000-9fff */
ROM_END



static int arkanoi2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];

		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f, &RAM[0xe3a8], 3);
			osd_fclose(f);
		}


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xeca5], "\x54\x4b\x4e\xff", 4) == 0 && memcmp(&RAM[0xec81], "\x01\x00\x00\x05", 4) == 0 )
	{

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f, &RAM[0xec81], 8*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void arkanoi2_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f, &RAM[0xec81], 8*5);
		osd_fclose(f);
		RAM[0xeca5] = 0;
	}
}

static int tnzs_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xe6ad], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xe68d], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xe68d], 35);
		osd_fclose(f);
	}
}

static int tnzs2_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xec2a], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xec0a], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs2_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xec0a], 35);
		osd_fclose(f);
	}
}

/****  Insector X high score save routine - RJF (April 17, 1999)  ****/

static int insectx_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xc604],"\x4b\x52\x59",3) == 0) &&
            (memcmp(&RAM[0xc64c],"\x48\x54\x42",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xc600], 10*8);        /* HS table */

                        RAM[0xc6ea] = RAM[0xc600];      /* update high score */
                        RAM[0xc6eb] = RAM[0xc601];      /* on top of screen */
                        RAM[0xc6ec] = RAM[0xc602];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void insectx_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xc600], 10*8);       /* HS table */
		osd_fclose(f);
	}
}



struct GameDriver extrmatn_driver =
{
	__FILE__,
	0,
	"extrmatn",
	"Extermination (US)",
	"1987",
	"[Taito] World Games",
	"Luca Elia\nMirko Buffoni",
	0,
	&arkanoi2_machine_driver,
	extrmatn_init,

	extrmatn_rom,
	0, 0,
	0,
	0,

	extrmatn_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver arkanoi2_driver =
{
	__FILE__,
	0,
	"arkanoi2",
	"Arkanoid - Revenge of DOH (World)",
	"1987",
	"Taito Corporation Japan",
	"Luca Elia\nMirko Buffoni",
	0,
	&arkanoi2_machine_driver,
	arkanoi2_init,

	arkanoi2_rom,
	0, 0,
	0,
	0,

	arkanoi2_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_270,

	arkanoi2_hiload, arkanoi2_hisave
};

struct GameDriver ark2us_driver =
{
	__FILE__,
	&arkanoi2_driver,
	"ark2us",
	"Arkanoid - Revenge of DOH (US)",
	"1987",
	"Taito America Corporation (Romstar license)",
	"Luca Elia\nMirko Buffoni",
	0,
	&arkanoi2_machine_driver,
	arkanoi2_init,

	ark2us_rom,
	0, 0,
	0,
	0,

	ark2us_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_270,

	arkanoi2_hiload, arkanoi2_hisave
};

struct GameDriver tnzs_driver =
{
	__FILE__,
	0,
	"tnzs",
	"The Newzealand Story (Japan)",
	"1988",
	"Taito Corporation",
    "Chris Moore\nMartin Scragg\nRichard Mitton",
	0,
	&tnzs_machine_driver,
	tnzs_init,

	tnzs_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tnzs_input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

	tnzs_hiload, tnzs_hisave
};

struct GameDriver tnzsb_driver =
{
	__FILE__,
	&tnzs_driver,
	"tnzsb",
	"The Newzealand Story (World, bootleg)",
	"1988",
	"bootleg",
    "Chris Moore\nMartin Scragg\nRichard Mitton",
	0,
	&tnzsb_machine_driver,
	tnzs_init,

	tnzsb_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tnzsb_input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

	tnzs_hiload, tnzs_hisave
};

struct GameDriver tnzs2_driver =
{
	__FILE__,
	&tnzs_driver,
	"tnzs2",
	"The Newzealand Story 2 (World)",
	"1988",
	"Taito Corporation Japan",
    "Chris Moore\nMartin Scragg\nRichard Mitton",
	0,
	&tnzs_machine_driver,
	tnzs_init,

	tnzs2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tnzs2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

    tnzs2_hiload, tnzs2_hisave
};

struct GameDriver insectx_driver =
{
	__FILE__,
	0,
	"insectx",
	"Insector X (World)",
	"1989",
	"Taito Corporation Japan",
    "Chris Moore\nMartin Scragg\nRichard Mitton",
	0,
	&insectx_machine_driver,
	insectx_init,

	insectx_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	insectx_input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

	insectx_hiload, insectx_hisave
};
