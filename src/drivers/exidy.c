/***************************************************************************

Exidy memory map

0000-00FF R/W Zero Page RAM
0100-01FF R/W Stack RAM
0200-03FF R/W Scratchpad RAM
0800-3FFF  R  Program ROM (Targ, Spectar only)
4000-43FF R/W Screen RAM
4800-4FFF R/W Character Generator RAM (except Pepper II)
5000       W  Motion Object 1 Horizontal Position Latch (sprite 1 X)
5040       W  Motion Object 1 Vertical Position Latch   (sprite 1 Y)
5080       W  Motion Object 2 Horizontal Position Latch (sprite 2 X)
50C0       W  Motion Object 2 Vertical Position Latch   (sprite 2 Y)
5100       R  Option Dipswitch Port
              bit 0  coin 2 (NOT inverted) (must activate together with IN1 bit 5)
              bit 1-2  bonus
              bit 3-4  coins per play
              bit 5-6  lives
              bit 7  US/UK coins
5100       W  Motion Objects Image Latch
              Sprite number  bits 0-3 Sprite #1  4-7 Sprite #2
5101       R  Control Inputs Port
              bit 0  start 1
              bit 1  start 2
              bit 2  right
              bit 3  left
              bit 5  up
              bit 6  down
              bit 7  coin 1 (must activate together with IN1 bit 6)
5101       W  Output Control Latch (not used in PEPPER II upright)
              bit 7  Bank for sprite #1  (Not sure!)
              bit 6  Bank for sprite #2  (Sure, see the bird in Mouse Trap)
5103       R  Interrupt Condition Latch
              bit 0  LNG0 - supposedly a language DIP switch
              bit 1  LNG1 - supposedly a language DIP switch
              bit 2  M1CHAR - collision between Motion Object 1 and background
              bit 3  TABLE - supposedly a cocktail table DIP switch
              bit 4  M1M2 - collision between Motion Objects 1 and 2
              bit 5  coin 2 (must activate together with DSW bit 0)
              bit 6  coin 1 (must activate together with IN0 bit 7)
              bit 7  SL256 - VBlank?
5213       R  IN2 (Mouse Trap)
              bit 3  blue button
              bit 2  free play
              bit 1  red button
              bit 0  yellow button
52XX      R/W Audio/Color Board Communications
6000-6FFF R/W Character Generator RAM (Pepper II only)
8000-FFF9  R  Program memory space
FFFA-FFFF  R  Interrupt and Reset Vectors

Targ:
5200      Sound board control
        bit 0 Music
        bit 1 Shoot
        bit 2 unused
        bit 3 Swarn
        bit 4 Sspec
        bit 5 crash
        bit 6 long
        bit 7 game

5201      Sound board control
        bit 0 note
        bit 1 upper

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* These are defined in vidhrdw/exidy.c */

int exidy_vh_start(void);
void exidy_vh_stop(void);
void exidy_characterram_w(int offset,int data);
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap);
void exidy_color_w(int offset,int data);

extern unsigned char *exidy_characterram;
extern unsigned char *exidy_sprite_no;
extern unsigned char *exidy_sprite_enable;
extern unsigned char *exidy_sprite1_xpos;
extern unsigned char *exidy_sprite1_ypos;
extern unsigned char *exidy_sprite2_xpos;
extern unsigned char *exidy_sprite2_ypos;
extern unsigned char *exidy_color_latch;

extern unsigned char *exidy_collision;
extern int exidy_collision_counter;

/* These are defined in drivers/exidy.c */

static int exidy_input_port_2_r(int offset);
static void exidy_init_machine(void);
static int exidy_interrupt(void);

static unsigned char exidy_collision_mask = 0x00;

/* These are defined in sndhrdw/targ.c */
extern unsigned char targ_spec_flag;
extern void targ_sh_w(int offset,int data);
extern int targ_sh_start(void);
extern void targ_sh_stop(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
    { 0x0800, 0x3fff, MRA_ROM }, /* Targ, Spectar only */
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
	{ 0x5103, 0x5103, exidy_input_port_2_r, &exidy_collision }, /* IN1 */
    { 0x5105, 0x5105, input_port_4_r }, /* IN3 - Targ, Spectar only */
	{ 0x5213, 0x5213, input_port_3_r },     /* IN2 */
    { 0x6000, 0x6fff, MRA_RAM }, /* Pepper II only */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0800, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5210, 0x5212, exidy_color_w, &exidy_color_latch },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress targ_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0800, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x5201, targ_sh_w },
 	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pepper2_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x5201, targ_sh_w },
	{ 0x5210, 0x5212, exidy_color_w, &exidy_color_latch },
	{ 0x5213, 0x5217, MWA_NOP }, /* empty control lines on color/sound board */
	{ 0x6000, 0x6fff, exidy_characterram_w, &exidy_characterram }, /* two 6116 character RAMs */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x7fff, MRA_ROM },
	{ 0x8000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x57ff, MWA_RAM },
	{ 0x5800, 0x7fff, MWA_ROM },
	{ 0x8000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/***************************************************************************
Input Ports
***************************************************************************/

INPUT_PORTS_START( mtrap_input_ports )
	PORT_START	/* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x00, "60000" )
	PORT_DIPNAME( 0x98, 0x98, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x98, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x88, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "Coin A 2/1 Coin B 1/3" )
	PORT_DIPSETTING(    0x08, "Coin A 1/3 Coin B 2/7" )
	PORT_DIPSETTING(    0x10, "Coin A 1/1 Coin B 1/4" )
	PORT_DIPSETTING(    0x18, "Coin A 1/1 Coin B 1/5" )
	PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Dog Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "German" )
	PORT_DIPSETTING(    0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Table" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON2, "Yellow Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3, "Red Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME, "Free Play", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON4, "Blue Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( venture_input_ports )
	PORT_START	/* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "20000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x02, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x98, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x88, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x98, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Pence: LC 2C/1C - RC 1C/3C" )
	PORT_DIPSETTING(    0x18, "Pence: LC 1C/1C - RC 1C/6C" )
	/*0x10 same as 0x00 */
	/*0x90 same as 0x80 */
	PORT_DIPNAME( 0x60, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x60, "5" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "German" )
	PORT_DIPSETTING(    0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Table" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END




INPUT_PORTS_START( pepper2_input_ports )
	PORT_START      /* DSW */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Turns", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "70000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x06, "40000" )
	PORT_DIPNAME( 0x60, 0x20, "Number of Turns", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "5 Turns" )
	PORT_DIPSETTING(    0x40, "4 Turns" )
	PORT_DIPSETTING(    0x20, "3 Turns" )
	PORT_DIPSETTING(    0x00, "2 Turns" )
	PORT_DIPNAME( 0x98, 0x98, "Coins/Credit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "L-2/1 R-1/3" )
	PORT_DIPSETTING(    0x08, "L-1/1 R-1/4" )
	PORT_DIPSETTING(    0x10, "L-1/1 R-1/5" )
	PORT_DIPSETTING(    0x18, "1/3 or 2/7" )
	PORT_DIPSETTING(    0x90, "1/4" )
	PORT_DIPSETTING(    0x80, "1/2" )
	PORT_DIPSETTING(    0x88, "2/1" )
	PORT_DIPSETTING(    0x98, "1/1" )

	PORT_START      /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "German" )
	PORT_DIPSETTING(    0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Table" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END


INPUT_PORTS_START( targ_input_ports )
    PORT_START      /* DSW0 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )	/* upright/cocktail switch? */
    PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "10P/1 C 50P Coin/6 Cs" )
    PORT_DIPSETTING(    0x02, "2x10P/1 C 50P Coin/3 Cs" )
    PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Credit" )
    PORT_DIPSETTING(    0x04, "Extended Play" )
    PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x00, "1C/1C (no display)" )
    PORT_DIPSETTING(    0x18, "1 Coin/2 Credits" )
    PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x60, "2" )
    PORT_DIPSETTING(    0x40, "3" )
    PORT_DIPSETTING(    0x20, "4" )
    PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "Quarters" )
    PORT_DIPSETTING(    0x00, "Pence" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START  /* IN1 */
    PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
    PORT_BIT( 0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN3 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* identical to Targ, the only difference is the additional Language dip switch */
INPUT_PORTS_START( spectar_input_ports )
    PORT_START      /* DSW0 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )	/* upright/cocktail switch? */
    PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "10P/1 C 50P Coin/6 Cs" )
    PORT_DIPSETTING(    0x02, "2x10P/1 C 50P Coin/3 Cs" )
    PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Credit" )
    PORT_DIPSETTING(    0x04, "Extended Play" )
    PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x00, "1C/1C (no display)" )
    PORT_DIPSETTING(    0x18, "1 Coin/2 Credits" )
    PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x60, "2" )
    PORT_DIPSETTING(    0x40, "3" )
    PORT_DIPSETTING(    0x20, "4" )
    PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "Quarters" )
    PORT_DIPSETTING(    0x00, "Pence" )

	PORT_START	/* IN0 */
    PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START  /* IN1 */
    PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "English" )
    PORT_DIPSETTING(    0x01, "French" )
    PORT_DIPSETTING(    0x02, "German" )
    PORT_DIPSETTING(    0x03, "Spanish" )
    PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
    PORT_BIT( 0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN3 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************
Graphics Layout
***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

/* Pepper II used a special Plane Generation Board for 2-bit graphics */

static struct GfxLayout pepper2_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 }, /* 2 bits separated by 0x0800 bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*4,   /* 64 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout targ_spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*2,   /* 32 characters for Targ/Spectar */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32    /* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,   0, 4 },     /* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, 8, 2 },  /* Sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo pepper2_gfxdecodeinfo[] =
{
	{ 0, 0x6000, &pepper2_charlayout,   0, 4 },    /* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, 16, 2 },  /* Angel/Devil/Zipper Ripper */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo targ_gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,   0, 4 },     /* the game dynamically modifies this */
	{ 1, 0x0000, &targ_spritelayout, 8, 2 },  /* Sprites */
	{ -1 } /* end of array */
};

/* Arbitrary starting colors, modified by the game */

static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* BACKGND */
	0x00,0x00,0xff,   /* CSPACE0 */
	0x00,0xff,0x00,   /* CSPACE1 */
	0x00,0xff,0xff,   /* CSPACE2 */
	0xff,0x00,0x00,   /* CSPACE3 */
	0xff,0x00,0xff,   /* 5LINES (unused?) */
	0xff,0xff,0x00,   /* 5MO2VID */
	0xff,0xff,0xff    /* 5MO1VID */
};

/* Targ doesn't have a color PROM; colors are changed by the means of 8x3 */
/* dip switches on the board. Here are the colors they map to. */
static unsigned char targ_palette[] =
{
	               	/* color   use                */
	0x00,0x00,0xFF,	/* blue    background         */
	0x00,0xFF,0xFF,	/* cyan    characters 192-255 */
	0xFF,0xFF,0x00,	/* yellow  characters 128-191 */
	0xFF,0xFF,0xFF,	/* white   characters  64-127 */
	0xFF,0x00,0x00,	/* red     characters   0- 63 */
	0x00,0xFF,0xFF,	/* cyan    not used           */
	0xFF,0xFF,0xFF,	/* white   bullet sprite      */
	0x00,0xFF,0x00,	/* green   wummel sprite      */
};

/* Spectar has different colors */
static unsigned char spectar_palette[] =
{
	               	/* color   use                */
	0x00,0x00,0xFF,	/* blue    background         */
	0x00,0xFF,0x00,	/* green   characters 192-255 */
	0x00,0xFF,0x00,	/* green   characters 128-191 */
	0xFF,0xFF,0xFF,	/* white   characters  64-127 */
	0xFF,0x00,0x00,	/* red     characters   0- 63 */
	0x00,0xFF,0x00,	/* green   not used           */
	0xFF,0xFF,0x00,	/* yellow  bullet sprite      */
	0x00,0xFF,0x00,	/* green   wummel sprite      */
};


static unsigned short colortable[] =
{
	/* one-bit characters */
	0, 4,  /* chars 0x00-0x3F */
	0, 3,  /* chars 0x40-0x7F */
	0, 2,  /* chars 0x80-0xBF */
	0, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,

};

static unsigned short pepper2_colortable[] =
{
	/* two-bit characters */
	/* (Because this is 2-bit color, the colorspace is only divided
	    in half instead of in quarters.  That's why 00-3F = 40-7F and
	    80-BF = C0-FF) */
	0, 0, 4, 3,  /* chars 0x00-0x3F */
	0, 0, 4, 3,  /* chars 0x40-0x7F */
	0, 0, 2, 1,  /* chars 0x80-0xBF */
	0, 0, 2, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,

};

/***************************************************************************
Special Functions
***************************************************************************/

static int exidy_input_port_2_r(int offset)
{
	int value;


	/* Get 2 coin inputs and VBLANK */
	value = readinputport(2);

        /* Combine with collision bits */
        value = value | ((*exidy_collision) & (exidy_collision_mask));

        /* Reset collision bits */
        *exidy_collision &= 0xEB;

	return value;
}


static void exidy_init_machine(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

    if (strcmp(Machine->gamedrv->name,"mtrap")==0)
    {
        exidy_collision_mask = 0x14;

	    /* Disable ROM Check for quicker startup */
	    RAM[0xF439]=0xEA;
	    RAM[0xF43A]=0xEA;
	    RAM[0xF43B]=0xEA;
    }
    else if (strcmp(Machine->gamedrv->name,"pepper2")==0)
    {
        exidy_collision_mask = 0x00;

    	/* Disable ROM Check for quicker startup */
    	RAM[0xF52D]=0xEA;
    	RAM[0xF52E]=0xEA;
    	RAM[0xF52F]=0xEA;
    }
    else if (strcmp(Machine->gamedrv->name,"venture")==0)
    {
        exidy_collision_mask = 0x80;

    	/* Disable ROM Check for quicker startup */
    	RAM[0x8AF4]=0xEA;
    	RAM[0x8AF5]=0xEA;
    	RAM[0x8AF6]=0xEA;
    }
    else if (strcmp(Machine->gamedrv->name,"venture2")==0)
    {
        exidy_collision_mask = 0x80;

    	/* Disable ROM Check for quicker startup */
        RAM[0x8B04]=0xEA;
        RAM[0x8B05]=0xEA;
        RAM[0x8B06]=0xEA;
    }
    else if (strcmp(Machine->gamedrv->name,"targ")==0)
    {
        /* Targ does not have a sprite enable register so we have to fake it out */
    	*exidy_sprite_enable = 0x10;
        exidy_collision_mask = 0x00;
        targ_spec_flag = 1;
    }
    else if (strcmp(Machine->gamedrv->name,"spectar")==0)
    {
        /* Spectar does not have a sprite enable register so we have to fake it out */
    	*exidy_sprite_enable = 0x10;
        exidy_collision_mask = 0x00;
        targ_spec_flag = 0;
    }

}


static int venture_interrupt(void)
{
       static int first_time = 1;
       static int interrupt_counter = 0;

       *exidy_collision = (*exidy_collision | 0x80) & exidy_collision_mask;

       if (first_time)
       {
	    first_time=0;
	    return nmi_interrupt();
       }

        interrupt_counter = (interrupt_counter + 1) % 32;

        if (interrupt_counter == 0)
        {
            *exidy_collision &= 0x7F;
            return interrupt();
        }

        if (exidy_collision_counter>0)
        {
            exidy_collision_counter--;
            return interrupt();
        }

       return 0;
}


static int exidy_interrupt(void)
{
       static int first_time = 1;

       *exidy_collision = (*exidy_collision | 0x80) & exidy_collision_mask;

       if (first_time)
       {
	    first_time=0;
	    return nmi_interrupt();
       }

       return interrupt();
}


/***************************************************************************
  Game ROMs
***************************************************************************/

ROM_START( mtrap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mtl11a.bin", 0xA000, 0x1000, 0xb4e109f7 )
	ROM_LOAD( "mtl10a.bin", 0xB000, 0x1000, 0xe890bac6 )
	ROM_LOAD( "mtl9a.bin",  0xC000, 0x1000, 0x06628e86 )
	ROM_LOAD( "mtl8a.bin",  0xD000, 0x1000, 0xa12b0c55 )
	ROM_LOAD( "mtl7a.bin",  0xE000, 0x1000, 0xb5c75a2f )
	ROM_LOAD( "mtl6a.bin",  0xF000, 0x1000, 0x2e7f499b )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mtl11D.bin", 0x0000, 0x0800, 0x389ef2ec )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "mta5a.bin", 0x6800, 0x0800, 0xdc40685a )
	ROM_LOAD( "mta6a.bin", 0x7000, 0x0800, 0x250b2f5f )
	ROM_LOAD( "mta7a.bin", 0x7800, 0x0800, 0x3ba2700a )
	ROM_RELOAD(	    0xF800, 0x0800 )
ROM_END


ROM_START( venture_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "13A-CPU", 0x8000, 0x1000, 0x891abe62 )
	ROM_LOAD( "12A-CPU", 0x9000, 0x1000, 0xac004ea6 )
	ROM_LOAD( "11A-CPU", 0xA000, 0x1000, 0x225ec9ee )
	ROM_LOAD( "10A-CPU", 0xB000, 0x1000, 0x4c8a0c70 )
	ROM_LOAD( "9A-CPU",  0xC000, 0x1000, 0x4ec5e145 )
	ROM_LOAD( "8A-CPU",  0xD000, 0x1000, 0x25eac9e2 )
	ROM_LOAD( "7A-CPU",  0xE000, 0x1000, 0x2173eca5 )
	ROM_LOAD( "6A-CPU",  0xF000, 0x1000, 0x1714e61c )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11D-CPU", 0x0000, 0x0800, 0xceb42d02 )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "3a-ac", 0x5800, 0x0800, 0x6098790a )
	ROM_LOAD( "4a-ac", 0x6000, 0x0800, 0x9bd6ad80 )
	ROM_LOAD( "5a-ac", 0x6800, 0x0800, 0xee5c9752 )
	ROM_LOAD( "6a-ac", 0x7000, 0x0800, 0x9559adbb )
	ROM_LOAD( "7a-ac", 0x7800, 0x0800, 0x9c5601b0 )
	ROM_RELOAD(	0xF800, 0x0800 )
ROM_END

ROM_START( venture2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vent_a13.cpu", 0x8000, 0x1000, 0x9ec428cc )
	ROM_LOAD( "vent_a12.cpu", 0x9000, 0x1000, 0x08ad19e9 )
	ROM_LOAD( "vent_a11.cpu", 0xA000, 0x1000, 0x1ec8e4e0 )
	ROM_LOAD( "vent_a10.cpu", 0xB000, 0x1000, 0xfc6c4faa )
	ROM_LOAD( "vent_a9.cpu",  0xC000, 0x1000, 0x3441c84f )
	ROM_LOAD( "vent_a8.cpu",  0xD000, 0x1000, 0x82f7f317 )
	ROM_LOAD( "vent_a7.cpu",  0xE000, 0x1000, 0xa44a760c )
	ROM_LOAD( "vent_a6.cpu",  0xF000, 0x1000, 0xdb8a7148 )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vent_d11.cpu", 0x0000, 0x0800, 0xceb42d02 )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "vent_3a.vid", 0x5800, 0x0800, 0x6098790a )
	ROM_LOAD( "vent_4a.vid", 0x6000, 0x0800, 0x9bd6ad80 )
	ROM_LOAD( "vent_5a.vid", 0x6800, 0x0800, 0xee5c9752 )
	ROM_LOAD( "vent_6a.vid", 0x7000, 0x0800, 0x9559adbb )
	ROM_LOAD( "vent_7a.vid", 0x7800, 0x0800, 0xdc5641b0 )
	ROM_RELOAD(	      0xF800, 0x0800 )
ROM_END


ROM_START( pepper2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "main_12a", 0x9000, 0x1000, 0x0a47e1e3 )
	ROM_LOAD( "main_11a", 0xA000, 0x1000, 0x120e0da0 )
	ROM_LOAD( "main_10a", 0xB000, 0x1000, 0x4ab11dc7 )
	ROM_LOAD( "main_9a",  0xC000, 0x1000, 0xa6f0e57e )
	ROM_LOAD( "main_8a",  0xD000, 0x1000, 0x04b6fd2a )
	ROM_LOAD( "main_7a",  0xE000, 0x1000, 0x9e350147 )
	ROM_LOAD( "main_6a",  0xF000, 0x1000, 0xda172fe9 )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "main_11d", 0x0000, 0x0800, 0x8ae4f8ba )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "audio_5a", 0x6800, 0x0800, 0x96de524a )
	ROM_LOAD( "audio_6a", 0x7000, 0x0800, 0xf2685a2c )
	ROM_LOAD( "audio_7a", 0x7800, 0x0800, 0xe5a6f8ec )
	ROM_RELOAD(	   0xF800, 0x0800 )
ROM_END

ROM_START( targ_rom )
	ROM_REGION(0x10000)	/* 64k for code */
    ROM_LOAD( "targ10a1", 0x1800, 0x0800, 0x2ef6836a )
    ROM_LOAD( "targ09a1", 0x2000, 0x0800, 0x98556db9 )
    ROM_LOAD( "targ08a1", 0x2800, 0x0800, 0xb8a37e39 )
    ROM_LOAD( "targ07a4", 0x3000, 0x0800, 0x9bdfac7b )
    ROM_LOAD( "targ06a3", 0x3800, 0x0800, 0x84a5a66f )
    ROM_RELOAD(	   0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "targ11d1", 0x0000, 0x0400, 0x3f892727 )
ROM_END

ROM_START( spectar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
    ROM_LOAD( "spl12a1", 0x0800, 0x0800, 0xefa4d9f2 )
    ROM_LOAD( "spl11a1", 0x1000, 0x0800, 0xe39df787 )
    ROM_LOAD( "spl10a1", 0x1800, 0x0800, 0x266a1f3c )
    ROM_LOAD( "spl9a1",  0x2000, 0x0800, 0x720689a0 )
    ROM_LOAD( "spl8a1",  0x2800, 0x0800, 0x3c62f338 )
    ROM_LOAD( "spl7a1",  0x3000, 0x0800, 0x024dbb5f )
    ROM_LOAD( "spl6a1",  0x3800, 0x0800, 0x408e5a4e )
    ROM_RELOAD(	  0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "splaud",  0x0000, 0x0400, 0x7d2aa084 )	/* this is actually not used (all FF) */
    ROM_CONTINUE(	0x0000, 0x0400 )	/* overwrite with the real one */
ROM_END


/***************************************************************************
  Hi Score Routines
***************************************************************************/


static int mtrap_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"LWH",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0380],5+6*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void mtrap_hisave(void)
{
	void *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0380],5+6*5);
		osd_fclose(f);
	}

}

static int venture_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"DJS",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0380],5+6*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void venture_hisave(void)
{
	void *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0380],5+6*5);
		osd_fclose(f);
	}

}



static int pepper2_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0360],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x0380],"\x15\x20\x11",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0360],5+6*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void pepper2_hisave(void)
{
	void *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0360],5+6*5);
		osd_fclose(f);
	}

}

static int targ_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x00AE],"\x00\x10",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x00AE],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void targ_hisave(void)
{
	void *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x00AE],2);
		osd_fclose(f);
	}

}



/***************************************************************************
  Game drivers
***************************************************************************/

static const char *targ_sample_names[] =
{
    "*targ",
    "expl.sam",
    "shot.sam",
    "sexpl.sam",
    "spslow.sam",
    "spfast.sam",
    "oneup.sam",
        0	/* end of array */
};

static struct Samplesinterface targ_samples_interface=
{
	3	/* 3 Channels */
};

static struct CustomSound_interface targ_custom_interface =
{
	targ_sh_start,
	targ_sh_stop,
	0
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			exidy_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1000000,	/* 1 Mhz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
    0,
    0,
    0,
    0,

};

static struct MachineDriver venture_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
                        venture_interrupt,32 /* Need to have multiple IRQs per frame if there's a collision */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1000000,	/* 1 Mhz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



static struct MachineDriver pepper2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,pepper2_writemem,0,0,
			exidy_interrupt,1
		},
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	pepper2_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(pepper2_colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};


static struct MachineDriver targ_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            704000,    /* .7Mhz */
			0,
            readmem,targ_writemem,0,0,
			exidy_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
    1,
    exidy_init_machine,

    /* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	targ_gfxdecodeinfo,
    sizeof(targ_palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
    0,0,0,0,
	{
		{
			SOUND_CUSTOM,
            &targ_custom_interface
		},
		{
			SOUND_SAMPLES,
            &targ_samples_interface
		}
	}
};



struct GameDriver mtrap_driver =
{
	"Mouse Trap",
	"mtrap",
	"Marc LaFontaine\nBrian Levine\nMike Balfour\nMarco Cassili",
	&machine_driver,

	mtrap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mtrap_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	mtrap_hiload,mtrap_hisave
};


struct GameDriver venture_driver =
{
	"Venture",
	"venture",
	"Marc LaFontaine\nNicola Salmoria\nBrian Levine\nMike Balfour\nBryan Smith (hardware info)",
	&venture_machine_driver,

	venture_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	venture_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	venture_hiload,venture_hisave
};

struct GameDriver venture2_driver =
{
	"Venture (alternate version)",
	"venture2",
	"Marc LaFontaine\nNicola Salmoria\nBrian Levine\nMike Balfour\nBryan Smith (hardware info)",
	&venture_machine_driver,

	venture2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	venture_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	venture_hiload,venture_hisave
};



struct GameDriver pepper2_driver =
{
	"Pepper II",
	"pepper2",
	"Marc LaFontaine\nBrian Levine\nMike Balfour",
	&pepper2_machine_driver,

	pepper2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pepper2_input_ports,

	0, palette, pepper2_colortable,
	ORIENTATION_DEFAULT,

	pepper2_hiload,pepper2_hisave
};

struct GameDriver targ_driver =
{
	"Targ",
	"targ",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	&targ_machine_driver,

	targ_rom,
	0, 0,
    targ_sample_names,
	0,

	targ_input_ports,

	0, targ_palette, colortable,

	ORIENTATION_DEFAULT,

	targ_hiload,targ_hisave
};

struct GameDriver spectar_driver =
{
	"Spectar",
	"spectar",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	&targ_machine_driver,

	spectar_rom,
	0, 0,
    targ_sample_names,
	0,

	spectar_input_ports,

	0, spectar_palette, colortable,

	ORIENTATION_DEFAULT,

	targ_hiload,targ_hisave
};


