/***************************************************************************

Paperboy (System 2) Memory Map
------------------------------

PAPERBOY T11 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program RAM                        0000-0FFF      R/W  D0-D15

Motion Object Color RAM            1000-107F       W   D0-D15 (RGBZ)
Alphanumeric Color RAM             1080-10BF       W   D0-D15 (RGBZ)
Playfield Color RAM                1100-11FF       W   D0-D15 (RGBZ)

Program Page 0 Address             1400            W   D10-D15
A/D Converter Output                               R   D0-D7

Program Page 1 Address             1402            W   D10-D15
A/D Converter Output                               R   D0-D7

A/D Converter Start Strobe         1480            W   xx

Video Memory Page Select (VMMU)    1500            W   D12-D13

IRQ0 Clear                         1580            W   xx
6502 Reset                         15A0            W   xx
IRQ2 Clear                         15C0            W   xx
IRQ3 Clear                         15E0            W   xx

IRQ0 Enable 6502 RD (Active High)  1600            W   D0
IRQ1 Enable 6502 WT (Active High)                  W   D1
IRQ2 Enable 32V (Active High)                      W   D2
IRQ3 Enable VBLANK (Active High)                   W   D3

Communications Port Write          1680            W   D0-D7

Horizontal Scroll                  1700            W   D6-D15
Playfield Bank 0                                   W   D0-D3

Vertical Scroll                    1780            W   D6-D14
Playfield Bank 1                                   W   D0-D3

Watchdog                           1800            W   xx

SW 6 (Active Low)                  1800            R   D0
SW 5 (Active Low)                                  R   D1
SW 4 (Active Low)                                  R   D2
SW 3 (Active Low)                                  R   D3
6502 Comm Flag (Active High)                       R   D4
T-11 Comm Flag (Active High)                       R   D5
SW 2 (Active Low)                                  R   D6
SW 1 (Active Low)                                  R   D7
Self-Test (Active Low)                             R   D15

Communications Port Read           1C00            R   D0-D7

Alphanumerics RAM (VMMU=0)         2000-37FE      R/W  D0-D15
Motion Object RAM (VMMU=0)         3800-3FFF      R/W  D0-D15
Playfield RAM Top (VMMU=2)         2000-3FFF      R/W  D0-D15
Playfield RAM Bottom (VMMU=3)      2000-3FFF      R/W  D0-D15

Paged Program ROM (Page 0)         4000-5FFF       R   D0-D15
Paged Program ROM (Page 1)         6000-7FFF       R   D0-D15
Program ROM                        8000-FFFF       R   D0-D15



PAPERBOY 6502 MEMORY MAP

Function                                  Address     R/W  Data
---------------------------------------------------------------
Program RAM                               0000-0FFF   R/W  D0-D7

EEROM                                     1000-17FF   R/W  D0-D7

POKEY 1                                   1800-180F   R/W  D0-D7
LETA                                      1810-1813    R   D0-D7
POKEY 2                                   1830-183F   R/W  D0-D7

T-11 Talk (Active High)                   1840         R   D0
6502 Talk (Active High)                                R   D1
TMS5220 Ready (Active High)                            R   D2
Self-Test (Active Low)                                 R   D4
Auxiliary Coin Switch (Active Low)                     R   D5
Left Coin Switch (Active Low)                          R   D6
Right Coin Switch (Active Low)                         R   D7

Music (YM-2151)                           1850-1851   R/W  D0-D7

Communications Port Read                  1860         R   D0-D7

TMS5220 Data                              1870         W   D0-D7
TMS5220 Write Strobe                      1872-1873    W

Communications Port Write                 1874         W   D0-D7

Right Coin Counter (Active High)          1876         W   D0
Left Coin Counter (Active High)                        W   D1

IRQ Clear                                 1878         W

Yamaha Mixer                              187A         W   D0-D2
POKEY Mixer                                            W   D3-D4
TMS5220 Mixer                                          W   D5-D7

LED1                                      187C         W   D2
LED2                                                   W   D3
LETA Resolution                                        W   D4
TMS5220 Squeak                                         W   D5

Sound Enable (Active High)                187E         W   D0

Program ROM (48K bytes)                   4000-FFFF   R    D0-D7

****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokyintf.h"
#include "sndhrdw/5220intf.h"
#include "sndhrdw/2151intf.h"


extern unsigned char *atarisys2_interrupt_enable;
extern unsigned char *atarisys2_bankselect;
extern unsigned char *atarisys2_slapstic_base;

int atarisys2_slapstic_r (int offset);
int atarisys2_adc_r (int offset);
int atarisys2_switch_r (int offset);
int atarisys2_videoram_r (int offset);
int atarisys2_leta_r (int offset);
int atarisys2_6502_switch_r (int offset);

void atarisys2_slapstic_w (int offset, int data);
void atarisys2_watchdog_w (int offset, int data);
void atarisys2_bankselect_w (int offset, int data);
void atarisys2_adc_strobe_w (int offset, int data);
void atarisys2_vmmu_w (int offset, int data);
void atarisys2_interrupt_ack_w (int offset, int data);
void atarisys2_vscroll_w (int offset, int data);
void atarisys2_hscroll_w (int offset, int data);
void atarisys2_videoram_w (int offset, int data);
void atarisys2_paletteram_w (int offset, int data);
void atarisys2_tms5220_w (int offset, int data);
void atarisys2_tms5220_strobe_w (int offset, int data);
void atarisys2_mixer_w (int offset, int data);
void atarisys2_sound_enable_w (int offset, int data);
void atarisys2_6502_switch_w (int offset, int data);

int atarisys2_interrupt (void);
int atarisys2_sound_interrupt (void);

void paperboy_init_machine (void);
void apb_init_machine (void);
void a720_init_machine (void);
void ssprint_init_machine (void);
void csprint_init_machine (void);

int paperboy_vh_start (void);
int apb_vh_start (void);
int a720_vh_start (void);
int ssprint_vh_start (void);
int csprint_vh_start (void);

void atarisys2_vh_stop (void);
void atarisys2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress atarisys2_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x11ff, paletteram_word_r },
	{ 0x1400, 0x1403, atarisys2_adc_r },
	{ 0x1800, 0x1801, atarisys2_switch_r },
	{ 0x1c00, 0x1c01, atarigen_sound_r },
	{ 0x2000, 0x3fff, atarisys2_videoram_r },
	{ 0x4000, 0x5fff, MRA_BANK1 },
	{ 0x6000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0x81ff, atarisys2_slapstic_r },
	{ 0x8200, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys2_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, atarisys2_paletteram_w, &paletteram },
	{ 0x1400, 0x1403, atarisys2_bankselect_w, &atarisys2_bankselect },
	{ 0x1480, 0x148f, atarisys2_adc_strobe_w },
	{ 0x1500, 0x1501, atarisys2_vmmu_w },
	{ 0x1580, 0x15ff, atarisys2_interrupt_ack_w },
	{ 0x1600, 0x1601, MWA_RAM, &atarisys2_interrupt_enable },
	{ 0x1680, 0x1681, atarigen_sound_w },
	{ 0x1700, 0x1701, atarisys2_hscroll_w, &atarigen_hscroll },
	{ 0x1780, 0x1781, atarisys2_vscroll_w, &atarigen_vscroll },
	{ 0x1800, 0x1801, atarisys2_watchdog_w },
	{ 0x2000, 0x3fff, atarisys2_videoram_w },
	{ 0x4000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x81ff, atarisys2_slapstic_w, &atarisys2_slapstic_base },
	{ 0x8200, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static void sound_eeprom_w (int offset, int data)
{
	Machine->memory_region[2][offset + 0x1000] = data;
/*	{
	static FILE *f;
	if (!f) f = fopen ("eeprom.log", "w");
	fprintf (f, "Write %02X @ %04X (PC = %04X)\n", data, offset + 0x1000, cpu_getpc ());
	}*/
}

static struct MemoryReadAddress atarisys2_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM, &atarigen_eeprom, &atarigen_eeprom_size },	/* EEPROM */
	{ 0x1800, 0x180f, pokey1_r },
	{ 0x1810, 0x1813, atarisys2_leta_r },
	{ 0x1830, 0x183f, pokey2_r },
	{ 0x1840, 0x1840, atarisys2_6502_switch_r },
	{ 0x1850, 0x1851, YM2151_status_port_0_r },
	{ 0x1860, 0x1860, atarigen_6502_sound_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys2_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x17ff, sound_eeprom_w },	/* EEPROM */
	{ 0x1800, 0x180f, pokey1_w },
	{ 0x1830, 0x183f, pokey2_w },
	{ 0x1850, 0x1850, YM2151_register_port_0_w },
	{ 0x1851, 0x1851, YM2151_data_port_0_w },
	{ 0x1870, 0x1870, atarisys2_tms5220_w },
	{ 0x1872, 0x1873, atarisys2_tms5220_strobe_w },
	{ 0x1874, 0x1874, atarigen_6502_sound_w },
	{ 0x1876, 0x1876, MWA_NOP },	/* coin counters */
	{ 0x1878, 0x1878, MWA_NOP },	/* IRQ clear */
	{ 0x187a, 0x187a, atarisys2_mixer_w },
	{ 0x187c, 0x187c, atarisys2_6502_switch_w },
	{ 0x187e, 0x187e, atarisys2_sound_enable_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( paperboy_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* ADC1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* ADC2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( apb_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_COIN1  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN2  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN3  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA0 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1,  4, 8, 0x40, 0x3f )

	PORT_START	/* LETA1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( a720_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON6 )

	PORT_START	/* IN2 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA0 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* LETA1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* LETA2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( ssprint_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA0 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_PLAYER3, 25, 0, 0x00, 0x7f )

	PORT_START	/* LETA1 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_PLAYER2, 25, 0, 0x00, 0x7f )

	PORT_START	/* LETA2 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_PLAYER1, 25, 0, 0x00, 0x7f )

	PORT_START	/* LETA3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( csprint_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START	/* IN2 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* ADC2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA0 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_PLAYER2, 25, 0, 0x00, 0x7f )

	PORT_START	/* LETA1 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_PLAYER1, 25, 0, 0x00, 0x7f )

	PORT_START	/* LETA2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* LETA3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout atarisys2_alpha_512 =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout atarisys2_alpha_1024 =
{
	8,8,	/* 8*8 chars */
	1024,	/* 1024 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};



static struct GfxLayout atarisys2_8x8x4_4096 =
{
	8,8,	/* 8*8 sprites */
	4096,   /* 4096 of them */
	4,		/* 4 bits per pixel */
	{ 0, 4, 4096*16*8, 4096*16*8+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout atarisys2_8x8x4_8192 =
{
	8,8,	/* 8*8 sprites */
	8192,   /* 8192 of them */
	4,		/* 4 bits per pixel */
	{ 0, 4, 8192*16*8, 8192*16*8+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout atarisys2_8x8x4_16384 =
{
	8,8,	/* 8*8 sprites */
	16384,  /* 16384 of them */
	4,		/* 4 bits per pixel */
	{ 0, 4, 16384*16*8, 16384*16*8+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every sprite takes 16 consecutive bytes */
};



static struct GfxLayout atarisys2_16x16x4_2048 =
{
	16,16, /* 16*16 sprites */
	2048,  /* 2048 of them */
	4,	   /* 4 bits per pixel */
	{ 0, 4, 2048*64*8, 2048*64*8+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	8*64	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout atarisys2_16x16x4_8192 =
{
	16,16, /* 16*16 sprites */
	8192,  /* 8192 of them */
	4,	   /* 4 bits per pixel */
	{ 0, 4, 8192*64*8, 8192*64*8+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	8*64	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo paperboy_gfxdecodeinfo[] =
{
	{ 1, 0x060000, &atarisys2_alpha_512,     64, 8 },
	{ 1, 0x000000, &atarisys2_8x8x4_4096,   128, 8 },
	{ 1, 0x020000, &atarisys2_16x16x4_2048,   0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo apb_gfxdecodeinfo[] =
{
	{ 1, 0x180000, &atarisys2_alpha_1024,    64, 8 },
	{ 1, 0x000000, &atarisys2_8x8x4_16384,  128, 8 },
	{ 1, 0x080000, &atarisys2_16x16x4_8192,   0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo a720_gfxdecodeinfo[] =
{
	{ 1, 0x140000, &atarisys2_alpha_1024,    64, 8 },
	{ 1, 0x000000, &atarisys2_8x8x4_8192,   128, 8 },
	{ 1, 0x040000, &atarisys2_16x16x4_8192,   0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ssprint_gfxdecodeinfo[] =
{
	{ 1, 0x0c0000, &atarisys2_alpha_1024,    64, 8 },
	{ 1, 0x000000, &atarisys2_8x8x4_16384,  128, 8 },
	{ 1, 0x080000, &atarisys2_16x16x4_2048,   0, 4 },
	{ -1 } /* end of array */
};

/* warning: not real; same as Super Sprint for now */
static struct GfxDecodeInfo csprint_gfxdecodeinfo[] =
{
	{ 1, 0x0c0000, &atarisys2_alpha_1024,    64, 8 },
	{ 1, 0x000000, &atarisys2_8x8x4_16384,  128, 8 },
	{ 1, 0x080000, &atarisys2_16x16x4_2048,   0, 4 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

static struct TMS5220interface tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    255,        /* volume */
    0 /* irq handler */
};


static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1789790,	/* ? */
	128,
	POKEY_DEFAULT_GAIN,
	USE_CLIP,
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


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 255 },
	{ 0 }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver paperboy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_T11,
			10000000,	/* 10 MHz */
			0,
			atarisys2_readmem,atarisys2_writemem,0,0,
			atarisys2_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			atarisys2_sound_readmem,atarisys2_sound_writemem,0,0,
			0,0,
			atarisys2_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	paperboy_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	paperboy_gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	paperboy_vh_start,
	atarisys2_vh_stop,
	atarisys2_vh_screenrefresh,

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


static struct MachineDriver apb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_T11,
			10000000,	/* 10 MHz */
			0,
			atarisys2_readmem,atarisys2_writemem,0,0,
			atarisys2_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			atarisys2_sound_readmem,atarisys2_sound_writemem,0,0,
			0,0,
			atarisys2_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	apb_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	apb_gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	apb_vh_start,
	atarisys2_vh_stop,
	atarisys2_vh_screenrefresh,

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


static struct MachineDriver a720_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_T11,
			10000000,	/* 10 MHz */
			0,
			atarisys2_readmem,atarisys2_writemem,0,0,
			atarisys2_interrupt,1
		},
		{
			CPU_M6502,
			2500000, /* NOTE: this value causes a timeout on startup - 1789790,*/
			2,
			atarisys2_sound_readmem,atarisys2_sound_writemem,0,0,
			0,0,
			atarisys2_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	a720_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	a720_gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	a720_vh_start,
	atarisys2_vh_stop,
	atarisys2_vh_screenrefresh,

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


static struct MachineDriver ssprint_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_T11,
			10000000,	/* 10 MHz */
			0,
			atarisys2_readmem,atarisys2_writemem,0,0,
			atarisys2_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			atarisys2_sound_readmem,atarisys2_sound_writemem,0,0,
			0,0,
			atarisys2_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	ssprint_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	ssprint_gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_SUPPORTS_DIRTY,
	0,
	ssprint_vh_start,
	atarisys2_vh_stop,
	atarisys2_vh_screenrefresh,

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


static struct MachineDriver csprint_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_T11,
			10000000,	/* 10 MHz */
			0,
			atarisys2_readmem,atarisys2_writemem,0,0,
			atarisys2_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			atarisys2_sound_readmem,atarisys2_sound_writemem,0,0,
			0,0,
			atarisys2_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	csprint_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	csprint_gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_SUPPORTS_DIRTY,
	0,
	csprint_vh_start,
	atarisys2_vh_stop,
	atarisys2_vh_screenrefresh,

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



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( paperboy_rom )
	ROM_REGION(0x90000)	/* 9*64k for T11 code */
	ROM_LOAD_ODD ( "CPU_L07.bin", 0x08000, 0x04000, 0x79cc9d68 )   /* even */
	ROM_LOAD_EVEN( "CPU_N07.bin", 0x08000, 0x04000, 0x206ffd2f )   /* odd  */
	ROM_LOAD_ODD ( "CPU_F06.bin", 0x10000, 0x04000, 0xacfb244b )   /* even */
	ROM_LOAD_EVEN( "CPU_N06.bin", 0x10000, 0x04000, 0x5e0db27f )   /* odd  */
	ROM_LOAD_ODD ( "CPU_J06.bin", 0x30000, 0x04000, 0xcfd016a8 )   /* even */
	ROM_LOAD_EVEN( "CPU_P06.bin", 0x30000, 0x04000, 0x55dca02a )   /* odd  */
	ROM_LOAD_ODD ( "CPU_K06.bin", 0x50000, 0x04000, 0x3fccb3ca )   /* even */
	ROM_LOAD_EVEN( "CPU_R06.bin", 0x50000, 0x04000, 0x193a576e )   /* odd  */
	ROM_LOAD_ODD ( "CPU_L06.bin", 0x70000, 0x04000, 0xe1b012a6 )   /* even */
	ROM_LOAD_EVEN( "CPU_S06.bin", 0x70000, 0x04000, 0xda597105 )   /* odd  */

	ROM_REGION(0x64000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "VID_A06.bin", 0x000000, 0x08000, 0xaf1ed4f8 )  /* bank 0 (4 bpp) */
	ROM_LOAD( "VID_B06.bin", 0x008000, 0x04000, 0x86a56783 )
	ROM_RELOAD(              0x00c000, 0x04000 )
	ROM_LOAD( "VID_C06.bin", 0x010000, 0x08000, 0xa18a8540 )
	ROM_LOAD( "VID_D06.bin", 0x018000, 0x04000, 0x05d1a065 )
	ROM_RELOAD(              0x01c000, 0x04000 )
	ROM_LOAD( "VID_L06.bin", 0x020000, 0x08000, 0x30aa98e8 ) /* bank 1 (4 bpp) */
	ROM_LOAD( "VID_K06.bin", 0x028000, 0x08000, 0xef61eedf )
	ROM_LOAD( "VID_J06.bin", 0x030000, 0x08000, 0xfd0aaff4 )
	ROM_LOAD( "VID_H06.bin", 0x038000, 0x08000, 0xb17ee882 )
	ROM_LOAD( "VID_S06.bin", 0x040000, 0x08000, 0xa7b6b114 )
	ROM_LOAD( "VID_P06.bin", 0x048000, 0x08000, 0xe79358bd )
	ROM_LOAD( "VID_N06.bin", 0x050000, 0x08000, 0xdffee874 )
	ROM_LOAD( "VID_M06.bin", 0x058000, 0x08000, 0x3bed9971 )
	ROM_LOAD( "VID_T06.bin", 0x060000, 0x02000, 0x30a21020 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "CPU_A02.bin", 0x04000, 0x04000, 0x93796679 )
	ROM_LOAD( "CPU_B02.bin", 0x08000, 0x04000, 0xbdfa5fa6 )
	ROM_LOAD( "CPU_C02.bin", 0x0c000, 0x04000, 0x9720fb28 )
ROM_END


ROM_START( apb_rom )
	ROM_REGION(0x90000)     /* 9 * 64k T11 code */
	ROM_LOAD_ODD ( "2126", 0x08000, 0x04000, 0xb7fa1c14 ) // CPU L/M7   27128     2126      4100
	ROM_LOAD_EVEN( "2127", 0x08000, 0x04000, 0x4b4b3b9f ) // CPU M/N7   27128     2127      2D00
	ROM_LOAD_ODD ( "5128", 0x10000, 0x10000, 0xa3609230 ) // CPU F/H6   27512     5128      D700
	ROM_LOAD_EVEN( "5129", 0x10000, 0x10000, 0x792bc975 ) // CPU M/N6   27512     5129      1000
	ROM_LOAD_ODD ( "1130", 0x30000, 0x10000, 0x3e4168e3 ) // CPU H/J6   27512     1130      5E87
	ROM_LOAD_EVEN( "1131", 0x30000, 0x10000, 0x525bc559 ) // CPU P6     27512     1131      0E98
	ROM_LOAD_ODD ( "1132", 0x70000, 0x10000, 0xb35ea8a2 ) // CPU L/M6   27512     1132      7DC4
	ROM_LOAD_EVEN( "1133", 0x70000, 0x10000, 0x53bad6fa ) // CPU S6     27512     1133      65CC

	ROM_REGION(0x184000)
	ROM_LOAD( "1118", 0x000000, 0x08000, 0x0df6a9ec ) // VID A5     27256     1118      2121
	ROM_RELOAD(       0x008000, 0x08000 )
	ROM_LOAD( "1120", 0x010000, 0x10000, 0x7ff500eb ) // VID B6     27512     1120      1D97
	ROM_LOAD( "1122", 0x020000, 0x10000, 0x2b9a003a ) // VID A7     27512     1122      2F6A
	ROM_LOAD( "1124", 0x030000, 0x10000, 0xc15920c7 ) // VID B7     27512     1124      7E4D
	ROM_LOAD( "1117", 0x040000, 0x08000, 0xc998097e ) // VID C5     27256     1117      B247
	ROM_RELOAD(       0x048000, 0x08000 )
	ROM_LOAD( "1119", 0x050000, 0x10000, 0xb2e7006b ) // VID D/E6   27512     1119      D585
	ROM_LOAD( "1121", 0x060000, 0x10000, 0x037dde9b ) // VID D/E7   27512     1121      4A17
	ROM_LOAD( "1123", 0x070000, 0x10000, 0x4869317b ) // VID C/D7   27512     1123      C2D0

	ROM_LOAD( "1101", 0x080000, 0x10000, 0xd17523df ) // VID T5     27512     1101      98E0
	ROM_LOAD( "1102", 0x090000, 0x10000, 0x8cd6d942 ) // VID S5     27512     1102      9153
	ROM_LOAD( "1103", 0x0a0000, 0x10000, 0x0afde6df ) // VID P/R5   27512     1103      DA71
	ROM_LOAD( "1104", 0x0b0000, 0x10000, 0xdc198f47 ) // VID N5     27512     1104      2DEC
	ROM_LOAD( "1109", 0x0c0000, 0x10000, 0x4c1f7eb1 ) // VID M5     27512     1109      22F5
	ROM_LOAD( "1110", 0x0d0000, 0x10000, 0xd3477a8f ) // VID L/K5   27512     1110      2B09
	ROM_LOAD( "1111", 0x0e0000, 0x10000, 0xe4935b6b ) // VID J/K5   27512     1111      78C2
	ROM_LOAD( "1112", 0x0f0000, 0x10000, 0x820f0435 ) // VID H/J5   27512     1112      593B

	ROM_LOAD( "1105", 0x100000, 0x10000, 0x2a03b7d9 ) // VID T6     27512     1105      E44E
	ROM_LOAD( "1106", 0x110000, 0x10000, 0x79a64a30 ) // VID S6     27512     1106      7634
	ROM_LOAD( "1107", 0x120000, 0x10000, 0xdd57c973 ) // VID P/R6   27512     1107      ECD0
	ROM_LOAD( "1108", 0x130000, 0x10000, 0x3f258b31 ) // VID N6     27512     1108      C522
	ROM_LOAD( "1113", 0x140000, 0x10000, 0x589812d0 ) // VID M6     27512     1113      D6D6
	ROM_LOAD( "1114", 0x150000, 0x10000, 0x9aecb5ac ) // VID K/L6   27512     1114      A023
	ROM_LOAD( "1115", 0x160000, 0x10000, 0x0e3f5ea5 ) // VID J/K6   27512     1115      4AA3
	ROM_LOAD( "1116", 0x170000, 0x10000, 0xe88d9225 ) // VID H/J6   27512     1116      6BEB

	ROM_LOAD( "1125", 0x180000, 0x04000, 0x5ac1b7f5 ) // VID T4     27128     1125      B722

	ROM_REGION(0x10000)     /* 64k for 6502 code */
	ROM_LOAD( "4134", 0x04000, 0x04000, 0x4e3ee1de ) // CPU A2     27128     4134      B7FF
	ROM_LOAD( "4135", 0x08000, 0x04000, 0x29be014e ) // CPU B/C2   27128     4135      66FF
	ROM_LOAD( "4136", 0x0c000, 0x04000, 0xe8bf76d3 ) // CPU C/D2   27128     4136      6DFF
ROM_END


ROM_START( a720_rom )
	ROM_REGION(0x90000)     /* 9 * 64k T11 code */
	ROM_LOAD_ODD ( "3126.rom", 0x08000, 0x04000, 0xf214fc6a )
	ROM_LOAD_EVEN( "3127.rom", 0x08000, 0x04000, 0x612ed02a )
	ROM_LOAD_ODD ( "3128.rom", 0x10000, 0x10000, 0x83b14377 )
	ROM_LOAD_EVEN( "4131.rom", 0x10000, 0x10000, 0x55fc3c36 )
	ROM_LOAD_ODD ( "1129.rom", 0x30000, 0x10000, 0x711c91ba )
	ROM_LOAD_EVEN( "1132.rom", 0x30000, 0x10000, 0x71c9aaf7 )
	ROM_LOAD_ODD ( "1130.rom", 0x50000, 0x10000, 0x93c32805 )
	ROM_LOAD_EVEN( "1133.rom", 0x50000, 0x10000, 0xc66e2344 )

	ROM_REGION(0x144000)
	ROM_LOAD( "1121.rom", 0x000000, 0x08000, 0xd1e685da )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "1122.rom", 0x008000, 0x08000, 0xf9c39abb )
	ROM_LOAD( "1123.rom", 0x010000, 0x08000, 0x980d4979 )
	ROM_LOAD( "1124.rom", 0x018000, 0x08000, 0x49dd4c69 )
	ROM_LOAD( "1117.rom", 0x020000, 0x08000, 0x836d94e5 )
	ROM_LOAD( "1118.rom", 0x028000, 0x08000, 0x4aae2d54 )
	ROM_LOAD( "1119.rom", 0x030000, 0x08000, 0x9d08f10c )
	ROM_LOAD( "1120.rom", 0x038000, 0x08000, 0x3efb420d )
	ROM_LOAD( "1109.rom", 0x040000, 0x10000, 0xd10987cb ) /* bank 1 (4 bpp) */
	ROM_LOAD( "1110.rom", 0x050000, 0x10000, 0xae50f8bc )
	ROM_LOAD( "1111.rom", 0x060000, 0x10000, 0xf34359c9 )
	ROM_LOAD( "1112.rom", 0x070000, 0x10000, 0x220d548f )
	ROM_LOAD( "1113.rom", 0x080000, 0x10000, 0x5c9d0377 )
	ROM_LOAD( "1114.rom", 0x090000, 0x10000, 0x354ac6a8 )
	ROM_LOAD( "1115.rom", 0x0a0000, 0x10000, 0xa624dba2 )
	ROM_LOAD( "1116.rom", 0x0b0000, 0x10000, 0x68b85c62 )
	ROM_LOAD( "1101.rom", 0x0c0000, 0x10000, 0xb1a2e72a )
	ROM_LOAD( "1102.rom", 0x0d0000, 0x10000, 0x42b335db )
	ROM_LOAD( "1103.rom", 0x0e0000, 0x10000, 0x9d2caf5e )
	ROM_LOAD( "1104.rom", 0x0f0000, 0x10000, 0x9f570e33 )
	ROM_LOAD( "1105.rom", 0x100000, 0x10000, 0xa8fca004 )
	ROM_LOAD( "1106.rom", 0x110000, 0x10000, 0x3ff0e4e4 )
	ROM_LOAD( "1107.rom", 0x120000, 0x10000, 0x2620cc40 )
	ROM_LOAD( "1108.rom", 0x130000, 0x10000, 0xdf792e47 )
	ROM_LOAD( "1125.rom", 0x140000, 0x04000, 0xcc8387ed )  /* alpha font */

	ROM_REGION(0x10000)     /* 64k for 6502 code */
	ROM_LOAD( "1134.rom", 0x04000, 0x04000, 0xf77f18f3 )
	ROM_LOAD( "1135.rom", 0x08000, 0x04000, 0xf39c9590 )
	ROM_LOAD( "1136.rom", 0x0c000, 0x04000, 0x5962452a )
ROM_END


ROM_START( ssprint_rom )
	ROM_REGION(0x90000)	/* 9*64k for T11 code */
	ROM_LOAD_ODD ( "136042.330", 0x08000, 0x04000, 0xd9b099da )   /* even */
	ROM_LOAD_EVEN( "136042.331", 0x08000, 0x04000, 0x7748dab4 )   /* odd  */
	ROM_LOAD_ODD ( "136042.329", 0x10000, 0x08000, 0x2f9e3d6a )   /* even */
	ROM_LOAD_EVEN( "136042.325", 0x10000, 0x08000, 0x4456b606 )   /* odd  */
	ROM_LOAD_ODD ( "136042.127", 0x50000, 0x08000, 0x5d918bed )   /* even */
	ROM_LOAD_EVEN( "136042.123", 0x50000, 0x08000, 0x3634cdf6 )   /* odd  */
	ROM_LOAD_ODD ( "136042.126", 0x70000, 0x08000, 0x42a289c8 )   /* even */
	ROM_LOAD_EVEN( "136042.122", 0x70000, 0x08000, 0x1f2c5602 )   /* odd  */

	ROM_REGION(0xc4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136042.105", 0x000000, 0x10000, 0xfb757389 )  /* bank 0 (4 bpp) */
	ROM_LOAD( "136042.106", 0x010000, 0x08000, 0x5eecd1b2 )
	ROM_RELOAD(             0x018000, 0x08000 )
	ROM_LOAD( "136042.101", 0x020000, 0x10000, 0x8ebf7a67 )
	ROM_LOAD( "136042.102", 0x030000, 0x08000, 0x9d93c76d )
	ROM_RELOAD(             0x038000, 0x08000 )
	ROM_LOAD( "136042.107", 0x040000, 0x10000, 0xb8a770db )
	ROM_LOAD( "136042.108", 0x050000, 0x08000, 0xe75e5036 )
	ROM_RELOAD(             0x058000, 0x08000 )
	ROM_LOAD( "136042.104", 0x060000, 0x10000, 0xac4250e2 )
	ROM_LOAD( "136042.103", 0x070000, 0x08000, 0x1a2939ad )
	ROM_RELOAD(             0x078000, 0x08000 )
	ROM_LOAD( "136042.113", 0x080000, 0x08000, 0x6cee3e08 ) /* bank 1 (4 bpp) */
	ROM_LOAD( "136042.112", 0x088000, 0x08000, 0xe689879b )
	ROM_LOAD( "136042.110", 0x090000, 0x08000, 0x0a182ce0 )
	ROM_LOAD( "136042.109", 0x098000, 0x08000, 0x19ed9abf )
	ROM_LOAD( "136042.117", 0x0a0000, 0x08000, 0x503f6da5 )
	ROM_LOAD( "136042.116", 0x0a8000, 0x08000, 0x6c32fb4e )
	ROM_LOAD( "136042.115", 0x0b0000, 0x08000, 0xcc1d6a23 )
	ROM_LOAD( "136042.114", 0x0b8000, 0x08000, 0xd6679241 )
	ROM_LOAD( "136042.218", 0x0c0000, 0x04000, 0x2b0fd785 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136042.419", 0x08000, 0x4000, 0xcd67ea49 )
	ROM_LOAD( "136042.420", 0x0c000, 0x4000, 0x7c85a629 )
ROM_END


ROM_START( csprint_rom )
	ROM_REGION(0x90000)	/* 9*64k for T11 code */
	ROM_LOAD_ODD ( "045-2126.7l",  0x08000, 0x04000, 0xaaed8a4b )   /* even */
	ROM_LOAD_EVEN( "045-1127.7mn", 0x08000, 0x04000, 0xb92ace3c )   /* odd  */
	ROM_LOAD_ODD ( "045-1125.6f",  0x10000, 0x08000, 0x167c6f6c )   /* even */
	ROM_LOAD_EVEN( "045-1122.6mn", 0x10000, 0x08000, 0xdbcc2098 )   /* odd  */
	ROM_LOAD_ODD ( "045-1124.6k",  0x50000, 0x08000, 0xb8a15bbf )   /* even */
	ROM_LOAD_EVEN( "045-1121.6r",  0x50000, 0x08000, 0x2dc554b5 )   /* odd  */
	ROM_LOAD_ODD ( "045-1123.6l",  0x70000, 0x08000, 0x01a948cf )   /* even */
	ROM_LOAD_EVEN( "045-1120.6s",  0x70000, 0x08000, 0x37060000 )   /* odd  */

	ROM_REGION(0xc4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "045-1105.6a", 0x000000, 0x08000, 0xc7182678 )  /* bank 0 (4 bpp) */
	ROM_RELOAD(              0x008000, 0x08000 )
	ROM_LOAD( "045-1106.6b", 0x010000, 0x08000, 0x9b40c1d6 )
	ROM_RELOAD(              0x018000, 0x08000 )
	ROM_LOAD( "045-1101.7a", 0x020000, 0x10000, 0x5b9d941b )
	ROM_LOAD( "045-1102.7b", 0x030000, 0x08000, 0x3108b87e )
	ROM_RELOAD(              0x038000, 0x08000 )
	ROM_LOAD( "045-1107.6c", 0x040000, 0x08000, 0x540b73e1 )
	ROM_RELOAD(              0x048000, 0x08000 )
	ROM_LOAD( "045-1108.6d", 0x050000, 0x08000, 0x37f61c92 )
	ROM_RELOAD(              0x058000, 0x08000 )
	ROM_LOAD( "045-1104.7d", 0x060000, 0x10000, 0x51f74b8f )
	ROM_LOAD( "045-1103.7c", 0x070000, 0x08000, 0x3a4e4ccc )
	ROM_RELOAD(              0x078000, 0x08000 )
	ROM_LOAD( "045-1112.6t", 0x080000, 0x08000, 0x6cee3e08 ) /* bank 1 (4 bpp) */
	ROM_LOAD( "045-1111.6s", 0x088000, 0x08000, 0xe689879b )
	ROM_LOAD( "045-1110.6p", 0x090000, 0x08000, 0x0a182ce0 )
	ROM_LOAD( "045-1109.6n", 0x098000, 0x08000, 0x19ed9abf )
	ROM_LOAD( "045-1116.5t", 0x0a0000, 0x08000, 0x503f6da5 )
	ROM_LOAD( "045-1115.5s", 0x0a8000, 0x08000, 0x6c32fb4e )
	ROM_LOAD( "045-1114.5p", 0x0b0000, 0x08000, 0xcc1d6a23 )
	ROM_LOAD( "045-1113.5n", 0x0b8000, 0x08000, 0xd6679241 )
	ROM_LOAD( "045-1117.4t", 0x0c0000, 0x04000, 0x5c77dd8b )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "045-1118.2bc", 0x08000, 0x4000, 0x5ed47dee )
	ROM_LOAD( "045-1119.2d",  0x0c000, 0x4000, 0xa98f68c5 )
ROM_END



/*************************************
 *
 *		ROM decoding
 *
 *************************************/

void paperboy_rom_decode (void)
{
	int i;

	/* expand the 16k program ROMs into full 64k chunks */
	for (i = 0x10000; i < 0x90000; i += 0x20000)
	{
		memcpy (&Machine->memory_region[0][i + 0x08000], &Machine->memory_region[0][i], 0x8000);
		memcpy (&Machine->memory_region[0][i + 0x10000], &Machine->memory_region[0][i], 0x8000);
		memcpy (&Machine->memory_region[0][i + 0x18000], &Machine->memory_region[0][i], 0x8000);
	}

	/* invert the bits of the sprites */
	for (i = 0x20000; i < 0x60000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


void apb_rom_decode (void)
{
	int i;

	/* invert the bits of the sprites */
	for (i = 0x80000; i < 0x180000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


void a720_rom_decode (void)
{
	int i;

	/* invert the bits of the sprites */
	for (i = 0x40000; i < 0x140000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


void ssprint_rom_decode (void)
{
	int i;

	/* expand the 32k program ROMs into full 64k chunks */
	for (i = 0x10000; i < 0x90000; i += 0x20000)
		memcpy (&Machine->memory_region[0][i + 0x10000], &Machine->memory_region[0][i], 0x10000);

	/* invert the bits of the sprites */
	for (i = 0x80000; i < 0xc0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


void csprint_rom_decode (void)
{
	int i;

	/* expand the 32k program ROMs into full 64k chunks */
	for (i = 0x10000; i < 0x90000; i += 0x20000)
		memcpy (&Machine->memory_region[0][i + 0x10000], &Machine->memory_region[0][i], 0x10000);

	/* invert the bits of the sprites */
	for (i = 0x80000; i < 0xc0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}




/*************************************
 *
 *		High score save/load
 *
 *************************************/

int paperboy_hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
	else
	{
		static const unsigned char data0042[] =
		{
			0x00,0x13,0x24,0x50,0x53,0x54,0x00,0x12,0xc0,0x55,0x43,0x48,0x00,0x12,0x5c,0x54,
			0x4f,0x49,0x00,0x11,0xf8,0x20,0x52,0x53,0x00,0x11,0x49,0x59,0x45,0x20,0x00,0x11,
			0x30,0x4f,0x20,0x54,0x00,0x10,0xcc,0x55,0x4f,0x41,0x00,0x10,0x68,0x52,0x4e,0x42,
			0x00,0x10,0x04,0x20,0x54,0x4c,0x00,0x0f,0xa0,0x20,0x4f,0x45,0x00,0x26,0xac,0x50,
			0x49,0x47,0x00,0x26,0x48,0x41,0x53,0x52,0x00,0x25,0xe4,0x50,0x20,0x45,0x00,0x25,
			0x80,0x45,0x54,0x41,0x00,0x25,0x1c,0x52,0x48,0x54,0x00,0x24,0xb8,0x20,0x45,0x45,
			0x00,0x24,0x54,0x42,0x20,0x53,0x00,0x23,0xf0,0x4f,0x20,0x54,0x00,0x23,0x8c,0x59,
			0x20,0x20,0x00,0x23,0x28,0x20,0x20,0x20,0x00,0x3a,0x34,0x44,0x41,0x54,0x00,0x39,
			0xd0,0x54,0x48,0x45,0x00,0x39,0x6c,0x42,0x4f,0x59,0x00,0x39,0x08,0x42,0x46,0x20,
			0x00,0x38,0xa4,0x4d,0x45,0x43,0x00,0x38,0x40,0x43,0x4a,0x20,0x00,0x37,0xdc,0x4a,
			0x45,0x53,0x00,0x37,0x78,0x50,0x43,0x54,0x00,0x37,0x14,0x4d,0x41,0x41,0x00,0x36,
			0xb0,0x42,0x41,0x46,0x01
		};
		static const unsigned char data00fb[] =
		{
			0x41,0x7f,0x41,0x7f,0x86,0x41,0x7f,0x41,0x7f,0x85,0x41,0x7f,0x41,0x7f,0x84
		};
		int i;

		for (i = 0x0000; i < 0x0042; i++)
			atarigen_eeprom[i + 0x0000] = 0x00;

		for (i = 0; i < sizeof (data0042); i++)
			atarigen_eeprom[i + 0x0042] = data0042[i];

		for (i = 0; i < sizeof (data00fb); i++)
			atarigen_eeprom[i + 0x00fb] = data00fb[i];
	}

	return 1;
}


int ssprint_hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
	else
	{
		static const unsigned char data0000[] =
		{
			0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
			0x00,0x20,0x00,0x20,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x20,0xff,0x00,
			0x40,0x00,0x40,0x10,0x00,0x10,0x50,0x00,0x10,0x00,0x00,0x00,0x40,0xff,0x00,0x60,
			0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x60,0xff,0x00,0x80,0x00,
			0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0x00,0xa0,0x00,0xa0,
			0x00,0x00,0x00,0xa0,0x00,0x00,0x00,0x00,0x00,0xa0,0xff,0x00,0xc0,0x00,0xc0,0x00,
			0x00,0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0xc0
		};
		static const unsigned char data0186[] =
		{
			0x03,0xe8,0x46,0xd6,0x03,0xde,0x28,0xb3,0x03,0xd4,0x44,0x23,0x03,0xca,0x1c,0x0b,
			0x03,0xc0,0x59,0xbf,0x03,0xb6,0x29,0x9f,0x03,0xac,0x4a,0xc2,0x03,0xa2,0x0e,0xdf,
			0x03,0x98,0x31,0xbf,0x03,0x8e,0x0d,0x06,0x03,0x84,0x0e,0x86,0x03,0x7a,0x24,0x0c,
			0x03,0x70,0x4a,0x48,0x03,0x66,0x51,0xf2,0x03,0x5c,0x3e,0x3f,0x03,0x52,0x11,0x06,
			0x03,0x48,0x45,0xb1,0x03,0x3e,0x7e,0x64,0x03,0x34,0x7f,0xe0,0x03,0x2a,0x7f,0xf3,
			0x03,0x20,0x7f,0xff,0x03,0x16,0x2a,0xd6,0x03,0x0c,0x25,0x76,0x03,0x02,0x4c,0x61,
			0x02,0xf8,0x28,0x01,0x02,0xee,0x01,0x53,0x02,0xe4,0x09,0x32,0x02,0xda,0x2c,0x32,
			0x02,0xd0,0x25,0x86,0x02,0xc6,0x1d,0x1f
		};
		int i;

		for (i = 0; i < sizeof (data0000); i++)
			atarigen_eeprom[i + 0x0000] = data0000[i];

		for (i = 0x0069; i < 0x0186; i++)
			atarigen_eeprom[i] = 0xff;

		for (i = 0; i < sizeof (data0186); i++)
			atarigen_eeprom[i + 0x0186] = data0186[i];
	}

	return 1;
}



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver paperboy_driver =
{
	__FILE__,
	0,
	"paperboy",
	"Paperboy",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nJuergen Buchmueller (MAME driver)\nMike Balfour (hardware info)",
	0,
	&paperboy_machine_driver,

	paperboy_rom,
	paperboy_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	paperboy_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	paperboy_hiload, atarigen_hisave
};


struct GameDriver apb_driver =
{
	__FILE__,
	0,
	"apb",
	"APB",
	"1987",
	"Atari Games",
	"Juergen Buchmueller (MAME driver)\nAaron Giles (MAME driver)\nMike Balfour (hardware info)",
	GAME_NOT_WORKING,
	&apb_machine_driver,

	apb_rom,
	apb_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	apb_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver a720_driver =
{
	__FILE__,
	0,
	"720",
	"720 Degrees",
	"1986",
	"Atari Games",
	"Aaron Giles (MAME driver)\nJuergen Buchmueller (MAME driver)\nMike Balfour (hardware info)",
	GAME_NOT_WORKING,
	&a720_machine_driver,

	a720_rom,
	a720_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	a720_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver ssprint_driver =
{
	__FILE__,
	0,
	"ssprint",
	"Super Sprint",
	"1986",
	"Atari Games",
	"Aaron Giles (MAME driver)\nJuergen Buchmueller (MAME driver)\nMike Balfour (hardware info)",
	0,
	&ssprint_machine_driver,

	ssprint_rom,
	ssprint_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	ssprint_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	ssprint_hiload, atarigen_hisave
};


struct GameDriver csprint_driver =
{
	__FILE__,
	0,
	"csprint",
	"Championship Sprint",
	"1986",
	"Atari Games",
	"Aaron Giles (MAME driver)\nJuergen Buchmueller (MAME driver)\nMike Balfour (hardware info)",
	0,
	&csprint_machine_driver,

	csprint_rom,
	csprint_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	csprint_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
