/***************************************************************************

Atari System 1 Memory Map
-------------------------

SYSTEM 1 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-087FFF  R    D0-D15

Program RAM                        400000-401FFF  R/W  D0-D15

Playfield Horizontal Scroll        800000,800001  W    D0-D8
Playfield Vertical Scroll          820000,820001  W    D0-D8
Playfield Special Priority Color   840000,840001  R    D0-D7

Sound Processor Reset (Low Reset)  860000         W    D7
Trak-Ball Test                                    W    D6
Motion Object Parameter Buffer Select             W    D5-D3
Playfield ROM Bank Select                         W    D2
Trak-Ball Resolution and Test LED                 W    D1
Alphanumerics ROM Bank Select                     W    D0

Watchdog (128 msec. timeout)       880000         W    xx
VBlank Acknowledge                 8A0000         W    xx
Unlock EEPROM                      8C0000         W    xx

Cartridge External                 900000-9FFFFF  R/W  D0-D15

Playfield RAM                      A00000-A01FFF  R/W  D0-D15
Motion Object Vertical Position    A02000-A0207F  R/W  D0-D15
                                   A02200-A0227F  R/W  D0-D15
                                   A02E00-A02E7F  R/W  D0-D15
Motion Object Picture              A02080-A020FF  R/W  D0-D15
                                   A02280-A022FF  R/W  D0-D15
                                   A02E80-A02EFF  R/W  D0-D15
Motion Object Horizontal Position  A02100-A02170  R/W  D0-D15
                                   A02300-A02370  R/W  D0-D15
                                   A02F00-A02F70  R/W  D0-D15
Motion Object Link                 A02180-A021FF  R/W  D0-D15
                                   A02380-A023FF  R/W  D0-D15
                                   A02F80-A02FFF  R/W  D0-D15
Alphanumerics RAM                  A03000-A03FFF  R/W  D0-D15

Color RAM Alpha                    B00000-B001FF  R/W  D0-D15
Color RAM Motion Object            B00200-B003FF  R/W  D0-D15
Color RAM Playfield                B00400-B005FF  R/W  D0-D15
Color RAM Translucent              B00600-B0061F  R/W  D0-D15

EEPROM                             F00001-F00FFF  R/W  D7-D0
Trak-Ball                          F20000-F20006  R    D7-D0
Analog Joystick                    F40000-F4000E  R    D7-D0
Analog Joystick IRQ Disable        F40010         R    D7

Output Buffer Full (@FE0000)       F60000         R    D7
Self-Test                                         R    D6
Switch Input                                      R    D5
Vertical Blank                                    R    D4
Switch Input                                      R    D3
Switch Input                                      R    D2
Switch Input                                      R    D1
Switch Input                                      R    D0

Read Sound Processor (6502)        FC0000         R    D0-D7
Write Sound Processor (6502)       FE0000         W    D0-D7


NOTE: All addresses can be accessed in byte or word mode.


SYSTEM 1 6502 MEMORY MAP

Function                                  Address     R/W  Data
---------------------------------------------------------------
Program RAM                               0000-0FFF   R/W  D0-D7
Cartridge External                        1000-1FFF   R/W  D0-D7

Music                                     1800-1801   R/W  D0-D7
Read 68010 Port (Input Buffer)            1810        R    D0-D7
Write 68010 Port (Outbut Buffer)          1810        W    D0-D7

Self-Test (Active Low)                    1820        R    D7
Output Buffer Full (@1000) (Active High)              R    D4
Data Available (@ 1010) (Active High)                 R    D3
Auxilliary Coin Switch                                R    D2
Left Coin Switch                                      R    D1
Right Coin Switch                                     R    D0

Music Reset (Low Reset)                   1820        W    D0
LED 1                                     1824        W    D0
LED 2                                     1825        W    D0
Coin Counter Right (Active High)          1826        W    D0
Coin Counter Left (Active High)           1827        W    D0

Effects                                   1870-187F   R/W  D0-D7

Program ROM (48K bytes)                   4000-FFFF   R    D0-D7
****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokyintf.h"
#include "sndhrdw/5220intf.h"
#include "sndhrdw/2151intf.h"


extern unsigned char *atarisys1_bankselect;
extern unsigned char *atarisys1_prioritycolor;

extern unsigned char *marble_speedcheck;

int atarisys1_io_r (int offset);
int atarisys1_6502_switch_r (int offset);
int atarisys1_6522_r (int offset);
int atarisys1_int3state_r (int offset);
int atarisys1_trakball_r (int offset);
int atarisys1_joystick_r (int offset);
int atarisys1_playfieldram_r (int offset);
int atarisys1_spriteram_r (int offset);

int marble_speedcheck_r (int offset);

void atarisys1_led_w (int offset, int data);
void atarisys1_6522_w (int offset, int data);
void atarisys1_joystick_w (int offset, int data);
void atarisys1_playfieldram_w (int offset, int data);
void atarisys1_spriteram_w (int offset, int data);
void atarisys1_bankselect_w (int offset, int data);
void atarisys1_hscroll_w (int offset, int data);
void atarisys1_vscroll_w (int offset, int data);

void marble_speedcheck_w (int offset, int data);

int atarisys1_interrupt (void);
void atarisys1_sound_interrupt (void);

void marble_init_machine (void);
void peterpak_init_machine (void);
void indytemp_init_machine (void);
void roadrunn_init_machine (void);
void roadblst_init_machine (void);

int marble_vh_start (void);
int peterpak_vh_start (void);
int indytemp_vh_start (void);
int roadrunn_vh_start (void);
int roadblst_vh_start (void);
void atarisys1_vh_stop (void);

void atarisys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void roadblst_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress atarisys1_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_r },
	{ 0x2e0000, 0x2e0003, atarisys1_int3state_r },
	{ 0x400000, 0x401fff, MRA_BANK1 },
	{ 0x840000, 0x840003, MRA_BANK4, &atarisys1_prioritycolor },
	{ 0x900000, 0x9fffff, MRA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_r },
	{ 0xa02000, 0xa02fff, atarisys1_spriteram_r },
	{ 0xa03000, 0xa03fff, MRA_BANK6 },
	{ 0xb00000, 0xb007ff, paletteram_word_r },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_r },
	{ 0xf20000, 0xf20007, atarisys1_trakball_r },
	{ 0xf40000, 0xf40013, atarisys1_joystick_r },
	{ 0xf60000, 0xf60003, atarisys1_io_r },
	{ 0xfc0000, 0xfc0003, atarigen_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys1_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_w, &atarigen_slapstic },
	{ 0x400000, 0x401fff, MWA_BANK1 },
	{ 0x800000, 0x800003, atarisys1_hscroll_w, &atarigen_hscroll },
	{ 0x820000, 0x820003, atarisys1_vscroll_w, &atarigen_vscroll },
	{ 0x840000, 0x840003, MWA_BANK4 },
	{ 0x860000, 0x860003, atarisys1_bankselect_w, &atarisys1_bankselect },
	{ 0x880000, 0x880003, watchdog_reset_w },
	{ 0x8a0000, 0x8a0003, MWA_NOP },		/* VBLANK ack */
	{ 0x8c0000, 0x8c0003, atarigen_eeprom_enable_w },
	{ 0x900000, 0x9fffff, MWA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xa02000, 0xa02fff, atarisys1_spriteram_w, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xa03000, 0xa03fff, MWA_BANK6, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xb00000, 0xb007ff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xf40010, 0xf40013, atarisys1_joystick_w },
	{ 0xfe0000, 0xfe0003, atarigen_sound_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress marble_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_r },
	{ 0x2e0000, 0x2e0003, atarisys1_int3state_r },
	{ 0x400014, 0x400017, marble_speedcheck_r },
	{ 0x400000, 0x401fff, MRA_BANK1 },
	{ 0x840000, 0x840003, MRA_BANK4, &atarisys1_prioritycolor },
	{ 0x900000, 0x9fffff, MRA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_r },
	{ 0xa02000, 0xa02fff, MRA_BANK3 },
	{ 0xa03000, 0xa03fff, MRA_BANK6 },
	{ 0xb00000, 0xb007ff, paletteram_word_r },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_r },
	{ 0xf20000, 0xf20007, atarisys1_trakball_r },
	{ 0xf40000, 0xf40013, atarisys1_joystick_r },
	{ 0xf60000, 0xf60003, atarisys1_io_r },
	{ 0xfc0000, 0xfc0003, atarigen_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress marble_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_w, &atarigen_slapstic },
	{ 0x400014, 0x400017, marble_speedcheck_w, &marble_speedcheck },
	{ 0x400000, 0x401fff, MWA_BANK1 },
	{ 0x800000, 0x800003, atarisys1_hscroll_w, &atarigen_hscroll },
	{ 0x820000, 0x820003, atarisys1_vscroll_w, &atarigen_vscroll },
	{ 0x840000, 0x840003, MWA_BANK4 },
	{ 0x860000, 0x860003, atarisys1_bankselect_w, &atarisys1_bankselect },
	{ 0x880000, 0x880003, watchdog_reset_w },
	{ 0x8a0000, 0x8a0003, MWA_NOP },		/* VBLANK ack */
	{ 0x8c0000, 0x8c0003, atarigen_eeprom_enable_w },
	{ 0x900000, 0x9fffff, MWA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xa02000, 0xa02fff, MWA_BANK3, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xa03000, 0xa03fff, MWA_BANK6, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xb00000, 0xb007ff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xf40010, 0xf40013, atarisys1_joystick_w },
	{ 0xfe0000, 0xfe0003, atarigen_sound_w },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress atarisys1_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x100f, atarisys1_6522_r },
	{ 0x1800, 0x1801, YM2151_status_port_0_r },
	{ 0x1810, 0x1810, atarigen_6502_sound_r },
	{ 0x1820, 0x1820, atarisys1_6502_switch_r },
	{ 0x1870, 0x187f, pokey1_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys1_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x100f, atarisys1_6522_w },
	{ 0x1800, 0x1800, YM2151_register_port_0_w },
	{ 0x1801, 0x1801, YM2151_data_port_0_w },
	{ 0x1810, 0x1810, atarigen_6502_sound_w },
	{ 0x1824, 0x1825, atarisys1_led_w },
	{ 0x1820, 0x1827, MWA_NOP },
	{ 0x1870, 0x187f, pokey1_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( marble_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_CENTER | IPF_PLAYER1, 100, 0x7f, 0, 0 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER1, 100, 0x7f, 0,0)

	PORT_START      /* IN2 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_X | IPF_CENTER | IPF_REVERSE | IPF_PLAYER2, 100, 0x7f, 0, 0 )

	PORT_START      /* IN3 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER2, 100, 0x7f, 0,0)

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
INPUT_PORTS_END


INPUT_PORTS_START( peterpak_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( indytemp_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( roadrunn_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( roadblst_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_REVERSE, 25, 0, 0x00, 0x7f )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout atarisys1_charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_3bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	3,		/* 3 bits per pixel */
	{ 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_4bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	4,		/* 4 bits per pixel */
	{ 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_5bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	5,		/* 5 bits per pixel */
	{ 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_6bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	6,		/* 6 bits per pixel */
	{ 5*8*0x08000, 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxDecodeInfo marble_gfxdecodeinfo[] =
{
	{ 1, 0x40000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_5bpp, 256, 24+1 },
	{ 1, 0x28000, &atarisys1_objlayout_3bpp, 256, 96+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo peterpak_gfxdecodeinfo[] =
{
	{ 1, 0x60000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo indytemp_gfxdecodeinfo[] =
{
	{ 1, 0x80000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x60000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo roadrunn_gfxdecodeinfo[] =
{
	{ 1, 0xc0000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x60000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x80000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xa0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo roadblst_gfxdecodeinfo[] =
{
	{ 1, 0xf0000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_6bpp, 256, 12+1 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x30000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x50000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x70000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x90000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xb0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xd0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	1,	/* 1 chip */
	1789790,	/* 1.78 MHz */
	128,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ 0 }
};


static struct TMS5220interface tms5220_interface =
{
    650826, /*640000,     * clock speed (80*samplerate) */
    255,        /* volume */
    0 /* irq handler */
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 255 },
	{ atarisys1_sound_interrupt }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver marble_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			marble_readmem,marble_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	marble_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	marble_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	marble_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};


static struct MachineDriver peterpak_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	peterpak_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	peterpak_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	peterpak_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};


static struct MachineDriver indytemp_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	indytemp_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	indytemp_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	indytemp_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};


static struct MachineDriver roadrunn_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	roadrunn_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	roadrunn_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	roadrunn_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};


static struct MachineDriver roadblst_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	roadblst_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	roadblst_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	roadblst_vh_start,
	atarisys1_vh_stop,
	roadblst_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/*************************************
 *
 *		ROM decoders
 *
 *************************************/

static void marble_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x40000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void peterpak_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x60000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void indytemp_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x80000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void roadrunn_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0xc0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void roadblst_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0xf0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( marble_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136033.401", 0x10000, 0x08000, 0xe40b98a5 )
	ROM_LOAD_ODD ( "136033.402", 0x10000, 0x08000, 0x668d8781 )
	ROM_LOAD_EVEN( "136033.403", 0x20000, 0x08000, 0x1e13cce9 )
	ROM_LOAD_ODD ( "136033.404", 0x20000, 0x08000, 0x071a7c6a )
	ROM_LOAD_EVEN( "136033.107", 0x80000, 0x04000, 0xa38dc551 )
	ROM_LOAD_ODD ( "136033.108", 0x80000, 0x04000, 0x4d90714a )

	ROM_REGION(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137", 0x00000, 0x04000, 0xa8cd8791 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138", 0x04000, 0x04000, 0x38e292e6 )
	ROM_LOAD( "136033.139", 0x08000, 0x04000, 0x6eb184fb )
	ROM_LOAD( "136033.140", 0x0c000, 0x04000, 0xfd1b9dd7 )
	ROM_LOAD( "136033.141", 0x10000, 0x04000, 0xd6e7eaff )
	ROM_LOAD( "136033.142", 0x14000, 0x04000, 0x2dd6d8ce )
	ROM_LOAD( "136033.143", 0x18000, 0x04000, 0xea9fb0b9 )
	ROM_LOAD( "136033.144", 0x1c000, 0x04000, 0x3b088c8c )
	ROM_LOAD( "136033.145", 0x20000, 0x04000, 0x3062bb18 )
	ROM_LOAD( "136033.146", 0x24000, 0x04000, 0x8834e240 )
	ROM_LOAD( "136033.149", 0x2c000, 0x04000, 0x57f77323 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151", 0x34000, 0x04000, 0x85b4d29e )
	ROM_LOAD( "136033.153", 0x3c000, 0x04000, 0x35547ca8 )
	ROM_LOAD( "136032.107", 0x40000, 0x02000, 0xad209966 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.421", 0x8000, 0x4000, 0x9ea59035 )
	ROM_LOAD( "136033.422", 0xc000, 0x4000, 0xb6b7e6cf )
ROM_END


ROM_START( marble2_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136033.201", 0x10000, 0x08000, 0xad4275e8 )
	ROM_LOAD_ODD ( "136033.202", 0x10000, 0x08000, 0xf103a7af )
	ROM_LOAD_EVEN( "136033.203", 0x20000, 0x08000, 0x1e13cce9 )
	ROM_LOAD_ODD ( "136033.204", 0x20000, 0x08000, 0xf32e9852 )
	ROM_LOAD_EVEN( "136033.107", 0x80000, 0x04000, 0xa38dc551 )
	ROM_LOAD_ODD ( "136033.108", 0x80000, 0x04000, 0x4d90714a )

	ROM_REGION(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137", 0x00000, 0x04000, 0xa8cd8791 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138", 0x04000, 0x04000, 0x38e292e6 )
	ROM_LOAD( "136033.139", 0x08000, 0x04000, 0x6eb184fb )
	ROM_LOAD( "136033.140", 0x0c000, 0x04000, 0xfd1b9dd7 )
	ROM_LOAD( "136033.141", 0x10000, 0x04000, 0xd6e7eaff )
	ROM_LOAD( "136033.142", 0x14000, 0x04000, 0x2dd6d8ce )
	ROM_LOAD( "136033.143", 0x18000, 0x04000, 0xea9fb0b9 )
	ROM_LOAD( "136033.144", 0x1c000, 0x04000, 0x3b088c8c )
	ROM_LOAD( "136033.145", 0x20000, 0x04000, 0x3062bb18 )
	ROM_LOAD( "136033.146", 0x24000, 0x04000, 0x8834e240 )
	ROM_LOAD( "136033.149", 0x2c000, 0x04000, 0x57f77323 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151", 0x34000, 0x04000, 0x85b4d29e )
	ROM_LOAD( "136033.153", 0x3c000, 0x04000, 0x35547ca8 )
	ROM_LOAD( "136032.107", 0x40000, 0x02000, 0xad209966 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.121", 0x8000, 0x4000, 0xd973483f )
	ROM_LOAD( "136033.122", 0xc000, 0x4000, 0x769b5e75 )
ROM_END


ROM_START( marblea_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136033.323", 0x10000, 0x04000, 0x81bd0c8b )
	ROM_LOAD_ODD ( "136033.324", 0x10000, 0x04000, 0xeffc948c )
	ROM_LOAD_EVEN( "136033.225", 0x18000, 0x04000, 0x2d76bb2a )
	ROM_LOAD_ODD ( "136033.226", 0x18000, 0x04000, 0x699b3b7b )
	ROM_LOAD_EVEN( "136033.227", 0x20000, 0x04000, 0x1e739091 )
	ROM_LOAD_ODD ( "136033.228", 0x20000, 0x04000, 0x1b735fe7 )
	ROM_LOAD_EVEN( "136033.229", 0x28000, 0x04000, 0x6611c209 )
	ROM_LOAD_ODD ( "136033.230", 0x28000, 0x04000, 0xea7fd571 )
	ROM_LOAD_EVEN( "136033.107", 0x80000, 0x04000, 0xa38dc551 )
	ROM_LOAD_ODD ( "136033.108", 0x80000, 0x04000, 0x4d90714a )

	ROM_REGION(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137", 0x00000, 0x04000, 0xa8cd8791 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138", 0x04000, 0x04000, 0x38e292e6 )
	ROM_LOAD( "136033.139", 0x08000, 0x04000, 0x6eb184fb )
	ROM_LOAD( "136033.140", 0x0c000, 0x04000, 0xfd1b9dd7 )
	ROM_LOAD( "136033.141", 0x10000, 0x04000, 0xd6e7eaff )
	ROM_LOAD( "136033.142", 0x14000, 0x04000, 0x2dd6d8ce )
	ROM_LOAD( "136033.143", 0x18000, 0x04000, 0xea9fb0b9 )
	ROM_LOAD( "136033.144", 0x1c000, 0x04000, 0x3b088c8c )
	ROM_LOAD( "136033.145", 0x20000, 0x04000, 0x3062bb18 )
	ROM_LOAD( "136033.146", 0x24000, 0x04000, 0x8834e240 )
	ROM_LOAD( "136033.149", 0x2c000, 0x04000, 0x57f77323 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151", 0x34000, 0x04000, 0x85b4d29e )
	ROM_LOAD( "136033.153", 0x3c000, 0x04000, 0x35547ca8 )
	ROM_LOAD( "136032.107", 0x40000, 0x02000, 0xad209966 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.257", 0x8000, 0x4000, 0xbc88cbf8 )
	ROM_LOAD( "136033.258", 0xc000, 0x4000, 0xc1512241 )
ROM_END


ROM_START( peterpak_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136028.142", 0x10000, 0x04000, 0x2308bb70 )
	ROM_LOAD_ODD ( "136028.143", 0x10000, 0x04000, 0x84577b53 )
	ROM_LOAD_EVEN( "136028.144", 0x18000, 0x04000, 0xfe5bd2e7 )
	ROM_LOAD_ODD ( "136028.145", 0x18000, 0x04000, 0x64417df1 )
	ROM_LOAD_EVEN( "136028.146", 0x20000, 0x04000, 0x52409f7e )
	ROM_LOAD_ODD ( "136028.147", 0x20000, 0x04000, 0xa40e10ac )
	ROM_LOAD_EVEN( "136028.148", 0x80000, 0x04000, 0x077a9584 )
	ROM_LOAD_ODD ( "136028.149", 0x80000, 0x04000, 0x942b6f19 )

	ROM_REGION(0x62000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136028.138", 0x00000, 0x08000, 0x465b4407 )  /* bank 0 (4 bpp) */
	ROM_LOAD( "136028.139", 0x08000, 0x08000, 0x237d8a43 )
	ROM_LOAD( "136028.140", 0x10000, 0x08000, 0x20e81c4a )
	ROM_LOAD( "136028.141", 0x18000, 0x08000, 0xa847b74b )
	ROM_LOAD( "136028.150", 0x20000, 0x08000, 0x3e8a4cdc )  /* bank 1 (4 bpp) */
	ROM_LOAD( "136028.151", 0x28000, 0x08000, 0x24ea4208 )
	ROM_LOAD( "136028.152", 0x30000, 0x08000, 0x1a627174 )
	ROM_LOAD( "136028.153", 0x38000, 0x08000, 0x132fa469 )
	ROM_LOAD( "136028.105", 0x44000, 0x04000, 0x3e17283d )  /* bank 2 (4 bpp) */
	ROM_LOAD( "136028.108", 0x4c000, 0x04000, 0x69facace )
	ROM_LOAD( "136028.111", 0x54000, 0x04000, 0xee6fc27b )
	ROM_LOAD( "136028.114", 0x5c000, 0x04000, 0x423c0678 )
	ROM_LOAD( "136032.107", 0x60000, 0x02000, 0xad209966 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136028.101", 0x8000, 0x4000, 0xacc8052c )
	ROM_LOAD( "136028.102", 0xc000, 0x4000, 0x8da227e4 )
ROM_END


ROM_START( indytemp_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136036.432", 0x10000, 0x08000, 0x200f332d )
	ROM_LOAD_ODD ( "136036.431", 0x10000, 0x08000, 0xd1b9f59d )
	ROM_LOAD_EVEN( "136036.434", 0x20000, 0x08000, 0xb67f63cd )
	ROM_LOAD_ODD ( "136036.433", 0x20000, 0x08000, 0xe45c22ae )
	ROM_LOAD_EVEN( "136036.456", 0x30000, 0x04000, 0xa815a6f7 )
	ROM_LOAD_ODD ( "136036.457", 0x30000, 0x04000, 0x32d99f7b )
	ROM_LOAD_EVEN( "136036.358", 0x80000, 0x04000, 0xd9efc251 )
	ROM_LOAD_ODD ( "136036.359", 0x80000, 0x04000, 0x00c54b2f )

	ROM_REGION(0x82000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136036.135", 0x00000, 0x08000, 0x8174cad6 )
	ROM_LOAD( "136036.139", 0x08000, 0x08000, 0xa0b7ca25 )
	ROM_LOAD( "136036.143", 0x10000, 0x08000, 0x99c12461 )
	ROM_LOAD( "136036.147", 0x18000, 0x08000, 0x83b30385 )
	ROM_LOAD( "136036.136", 0x20000, 0x08000, 0xbbe0c676 )
	ROM_LOAD( "136036.140", 0x28000, 0x08000, 0x783ef31c )
	ROM_LOAD( "136036.144", 0x30000, 0x08000, 0x5b1bd0dd )
	ROM_LOAD( "136036.148", 0x38000, 0x08000, 0x3d8f80d1 )
	ROM_LOAD( "136036.137", 0x40000, 0x08000, 0x8c15beb7 )
	ROM_LOAD( "136036.141", 0x48000, 0x08000, 0xa9d69018 )
	ROM_LOAD( "136036.145", 0x50000, 0x08000, 0x8726336a )
	ROM_LOAD( "136036.149", 0x58000, 0x08000, 0xaead95a9 )
	ROM_LOAD( "136036.138", 0x60000, 0x08000, 0x20bb3ac5 )
	ROM_LOAD( "136036.142", 0x68000, 0x08000, 0x29b0c61c )
	ROM_LOAD( "136036.146", 0x70000, 0x08000, 0xd84e1e48 )
	ROM_LOAD( "136036.150", 0x78000, 0x08000, 0x1db9177f )
	ROM_LOAD( "136032.107", 0x80000, 0x02000, 0xad209966 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136036.153", 0x4000, 0x4000, 0x2dbcbba6 )
	ROM_LOAD( "136036.154", 0x8000, 0x4000, 0xd7270c51 )
	ROM_LOAD( "136036.155", 0xc000, 0x4000, 0x5924f9fe )
ROM_END


ROM_START( roadrunn_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205", 0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206", 0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "136040.228", 0x10000, 0x08000, 0xddbb81c7 )
	ROM_LOAD_ODD ( "136040.229", 0x10000, 0x08000, 0x4106ba4c )
	ROM_LOAD_EVEN( "136040.230", 0x20000, 0x08000, 0xefcca892 )

	ROM_LOAD_ODD ( "136040.231", 0x20000, 0x08000, 0xc42846ea )
	ROM_LOAD_EVEN( "136040.134", 0x50000, 0x08000, 0xf34733b3 )
	ROM_LOAD_ODD ( "136040.135", 0x50000, 0x08000, 0x48c1ed47 )
	ROM_LOAD_EVEN( "136040.136", 0x60000, 0x08000, 0xa03ce20a )
	ROM_LOAD_ODD ( "136040.137", 0x60000, 0x08000, 0x776b9b15 )
	ROM_LOAD_EVEN( "136040.138", 0x70000, 0x08000, 0xda27f929 )
	ROM_LOAD_ODD ( "136040.139", 0x70000, 0x08000, 0x1151fd17 )
	ROM_LOAD_EVEN( "136040.140", 0x80000, 0x04000, 0x605b18ab )
	ROM_LOAD_ODD ( "136040.141", 0x80000, 0x04000, 0x8253a79d )

	ROM_REGION(0xc2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136040.101", 0x00000, 0x08000, 0x22dd0455 )
	ROM_LOAD( "136040.107", 0x08000, 0x08000, 0x6f9b1267 )
	ROM_LOAD( "136040.113", 0x10000, 0x08000, 0x2a3b5f79 )
	ROM_LOAD( "136040.119", 0x18000, 0x08000, 0x11e0ffe2 )
	ROM_LOAD( "136040.102", 0x20000, 0x08000, 0x925028b6 )
	ROM_LOAD( "136040.108", 0x28000, 0x08000, 0x718d20dd )
	ROM_LOAD( "136040.114", 0x30000, 0x08000, 0x01efdb55 )
	ROM_LOAD( "136040.120", 0x38000, 0x08000, 0x2ec91507 )
	ROM_LOAD( "136040.103", 0x40000, 0x08000, 0xac8c456c )
	ROM_LOAD( "136040.109", 0x48000, 0x08000, 0x89bf9967 )
	ROM_LOAD( "136040.115", 0x50000, 0x08000, 0xa054b3ba )
	ROM_LOAD( "136040.121", 0x58000, 0x08000, 0x378775d1 )
	ROM_LOAD( "136040.104", 0x60000, 0x08000, 0x5dd0a652 )
	ROM_LOAD( "136040.110", 0x68000, 0x08000, 0xf314c106 )
	ROM_LOAD( "136040.116", 0x70000, 0x08000, 0xf645f975 )
	ROM_LOAD( "136040.122", 0x78000, 0x08000, 0x970b0a61 )
	ROM_LOAD( "136040.105", 0x80000, 0x08000, 0x09ae5d16 )
	ROM_LOAD( "136040.111", 0x88000, 0x08000, 0x1cdd8149 )
	ROM_LOAD( "136040.117", 0x90000, 0x08000, 0x7f7c7e38 )
	ROM_LOAD( "136040.123", 0x98000, 0x08000, 0xa082cc96 )
	ROM_LOAD( "136040.106", 0xa0000, 0x08000, 0x25a049a2 )
	ROM_LOAD( "136040.112", 0xa8000, 0x08000, 0xa2c71103 )
	ROM_LOAD( "136040.118", 0xb0000, 0x08000, 0xc0530833 )
	ROM_LOAD( "136040.124", 0xb8000, 0x08000, 0x33835a31 )
	ROM_LOAD( "136032.107", 0xc0000, 0x02000, 0xad209966 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136040.143", 0x8000, 0x4000, 0x99ab0257 )
	ROM_LOAD( "136040.144", 0xc000, 0x4000, 0xe97a31d0 )
ROM_END


ROM_START( roadblst_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0xc635d4d5 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0xb7a3e369 )
	ROM_LOAD_EVEN( "048-1139.ROM", 0x10000, 0x08000, 0x8b85435b )
	ROM_LOAD_ODD ( "048-1140.ROM", 0x10000, 0x08000, 0xd879e3cd )
	ROM_LOAD_EVEN( "048-1155.ROM", 0x20000, 0x08000, 0xa784569a )
	ROM_LOAD_ODD ( "048-1156.ROM", 0x20000, 0x08000, 0x93586684 )
	/* this is strange because we need to load the top halves of these ROMs higher */
	ROM_LOAD_EVEN( "048-1155.ROM", 0x50000, 0x10000, 0xa10737b7 )
	ROM_LOAD_ODD ( "048-1156.ROM", 0x50000, 0x10000, 0xb5cf198b )
	ROM_LOAD_EVEN( "048-1139.ROM", 0x40000, 0x10000, 0x97344c1a )
	ROM_LOAD_ODD ( "048-1140.ROM", 0x40000, 0x10000, 0xf91ec820 )
	ROM_LOAD_EVEN( "048-1167.ROM", 0x70000, 0x08000, 0x3fe74dad )
	ROM_LOAD_ODD ( "048-1168.ROM", 0x70000, 0x08000, 0xc0067e08 )
	ROM_LOAD_EVEN( "048-2147.ROM", 0x80000, 0x04000, 0xe23aa718 )
	ROM_LOAD_ODD ( "048-2148.ROM", 0x80000, 0x04000, 0xb5cb0cb5 )

	ROM_REGION(0xf2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "048-1101.ROM", 0x00000, 0x08000, 0xe32d6e65 )
	ROM_LOAD( "048-1102.ROM", 0x08000, 0x08000, 0x10183166 )
	ROM_LOAD( "048-1103.ROM", 0x10000, 0x08000, 0x9aad8c2d )
	ROM_LOAD( "048-1104.ROM", 0x18000, 0x08000, 0x3b85c33b )
	ROM_LOAD( "048-1105.ROM", 0x20000, 0x08000, 0xae646c52 )
	ROM_LOAD( "048-1106.ROM", 0x28000, 0x08000, 0x8005b205 )
	ROM_LOAD( "048-1107.ROM", 0x30000, 0x08000, 0xc79fc811 )
	ROM_CONTINUE(             0x50000, 0x08000 )
	ROM_LOAD( "048-1108.ROM", 0x38000, 0x08000, 0x60384c4c )
	ROM_CONTINUE(             0x58000, 0x08000 )
	ROM_LOAD( "048-1109.ROM", 0x40000, 0x08000, 0xf89b7613 )
	ROM_CONTINUE(             0x60000, 0x08000 )
	ROM_LOAD( "048-1110.ROM", 0x48000, 0x08000, 0x9ca6aecc )
	ROM_CONTINUE(             0x68000, 0x08000 )
	ROM_LOAD( "048-1111.ROM", 0x70000, 0x08000, 0xc66643fc )
	ROM_CONTINUE(             0x90000, 0x08000 )
	ROM_LOAD( "048-1112.ROM", 0x78000, 0x08000, 0xad0bcea3 )
	ROM_CONTINUE(             0x98000, 0x08000 )
	ROM_LOAD( "048-1113.ROM", 0x80000, 0x08000, 0xfbf06250 )
	ROM_CONTINUE(             0xa0000, 0x08000 )
	ROM_LOAD( "048-1114.ROM", 0x88000, 0x08000, 0xf8145464 )
	ROM_CONTINUE(             0xa8000, 0x08000 )
	ROM_LOAD( "048-1115.ROM", 0xb0000, 0x08000, 0x597bc32d )
	ROM_CONTINUE(             0xd0000, 0x08000 )
	ROM_LOAD( "048-1116.ROM", 0xb8000, 0x08000, 0x4f15fe81 )
	ROM_CONTINUE(             0xd8000, 0x08000 )
	ROM_LOAD( "048-1117.ROM", 0xc0000, 0x08000, 0xe2de028a )
	ROM_CONTINUE(             0xe0000, 0x08000 )
	ROM_LOAD( "048-1118.ROM", 0xc8000, 0x08000, 0x4e2e0cb2 )
	ROM_CONTINUE(             0xe8000, 0x08000 )
	ROM_LOAD( "136032.107", 0xf0000, 0x02000, 0xad209966 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "048-1149.ROM", 0x4000, 0x4000, 0x009ec170 )
	ROM_LOAD( "048-1169.ROM", 0x8000, 0x4000, 0x79d01f94 )
	ROM_LOAD( "048-1170.ROM", 0xc000, 0x4000, 0xb5879699 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver marble_driver =
{
	__FILE__,
	0,
	"marble",
	"Marble Madness (set 1)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,

	marble_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver marble2_driver =
{
	__FILE__,
	&marble_driver,
	"marble2",
	"Marble Madness (set 2)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,

	marble2_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver marblea_driver =
{
	__FILE__,
	&marble_driver,
	"marblea",
	"Marble Madness (set 3)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,

	marblea_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver peterpak_driver =
{
	__FILE__,
	0,
	"peterpak",
	"Peter Pack-Rat",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&peterpak_machine_driver,

	peterpak_rom,
	peterpak_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	peterpak_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver indytemp_driver =
{
	__FILE__,
	0,
	"indytemp",
	"Indiana Jones and the Temple of Doom",
	"1985",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&indytemp_machine_driver,

	indytemp_rom,
	indytemp_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	indytemp_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver roadrunn_driver =
{
	__FILE__,
	0,
	"roadrunn",
	"Road Runner",
	"1985",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&roadrunn_machine_driver,

	roadrunn_rom,
	roadrunn_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	roadrunn_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver roadblst_driver =
{
	__FILE__,
	0,
	"roadblst",
	"Road Blasters",
	"1987",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&roadblst_machine_driver,

	roadblst_rom,
	roadblst_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	roadblst_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
