/***************************************************************************

Galaxian memory map

Compiled from information provided by friends and Uncles on RGVAC.

            AAAAAA
            111111AAAAAAAAAA     DDDDDDDD   Schem   function
HEX         5432109876543210 R/W 76543210   name

0000-27FF                                           Game ROM
5000-57FF   01010AAAAAAAAAAA R/W DDDDDDDD   !Vram   Character ram
5800-583F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Screen attributes
5840-585F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Sprites
5860-5FFF   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Bullets

6000        0110000000000000 R   -------D   !SW0    coin1
6000        0110000000000000 R   ------D-   !SW0    coin2
6000        0110000000000000 R   -----D--   !SW0    p1 left
6000        0110000000000000 R   ----D---   !SW0    p1 right
6000        0110000000000000 R   ---D----   !SW0    p1shoot
6000        0110000000000000 R   --D-----   !SW0    table ??
6000        0110000000000000 R   -D------   !SW0    test
6000        0110000000000000 R   D-------   !SW0    service

6000        0110000000000001 W   -------D   !DRIVER lamp 1 ??
6001        0110000000000001 W   -------D   !DRIVER lamp 2 ??
6002        0110000000000010 W   -------D   !DRIVER lamp 3 ??
6003        0110000000000011 W   -------D   !DRIVER coin control
6004        0110000000000100 W   -------D   !DRIVER Background lfo freq bit0
6005        0110000000000101 W   -------D   !DRIVER Background lfo freq bit1
6006        0110000000000110 W   -------D   !DRIVER Background lfo freq bit2
6007        0110000000000111 W   -------D   !DRIVER Background lfo freq bit3

6800        0110100000000000 R   -------D   !SW1    1p start
6800        0110100000000000 R   ------D-   !SW1    2p start
6800        0110100000000000 R   -----D--   !SW1    p2 left
6800        0110100000000000 R   ----D---   !SW1    p2 right
6800        0110100000000000 R   ---D----   !SW1    p2 shoot
6800        0110100000000000 R   --D-----   !SW1    no used
6800        0110100000000000 R   -D------   !SW1    dip sw1
6800        0110100000000000 R   D-------   !SW1    dip sw2

6800        0110100000000000 W   -------D   !SOUND  reset background F1
                                                    (1=reset ?)
6801        0110100000000001 W   -------D   !SOUND  reset background F2
6802        0110100000000010 W   -------D   !SOUND  reset background F3
6803        0110100000000011 W   -------D   !SOUND  Noise on/off
6804        0110100000000100 W   -------D   !SOUND  not used
6805        0110100000000101 W   -------D   !SOUND  shoot on/off
6806        0110100000000110 W   -------D   !SOUND  Vol of f1
6807        0110100000000111 W   -------D   !SOUND  Vol of f2

7000        0111000000000000 R   -------D   !DIPSW  dip sw 3
7000        0111000000000000 R   ------D-   !DIPSW  dip sw 4
7000        0111000000000000 R   -----D--   !DIPSW  dip sw 5
7000        0111000000000000 R   ----D---   !DIPSW  dip s2 6

7001        0111000000000001 W   -------D   9Nregen NMIon
7004        0111000000000100 W   -------D   9Nregen stars on
7006        0111000000000110 W   -------D   9Nregen hflip
7007        0111000000000111 W   -------D   9Nregen vflip

Note: 9n reg,other bits  used on moon cresta for extra graphics rom control.

7800        0111100000000000 R   --------   !wdr    watchdog reset
7800        0111100000000000 W   DDDDDDDD   !pitch  Sound Fx base frequency

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galaxian_flipx_w(int offset,int data);
void galaxian_flipy_w(int offset,int data);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
void pisces_gfxbank_w(int offset,int data);
int galaxian_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int galaxian_vh_interrupt(void);

void mooncrst_pitch_w(int offset,int data);
void mooncrst_vol_w(int offset,int data);
void mooncrst_noise_w(int offset,int data);
void mooncrst_background_w(int offset,int data);
void mooncrst_shoot_w(int offset,int data);
void mooncrst_lfo_freq_w(int offset,int data);
int mooncrst_sh_start(void);
void mooncrst_sh_stop(void);
void mooncrst_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress galaxian_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x587f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_pitch_w },
	{ 0x6800, 0x6800, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
	{ 0x6806, 0x6807, mooncrst_vol_w },
	{ 0x6000, 0x6001, osd_led_w },
	{ 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pisces_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x587f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_pitch_w },
	{ 0x6800, 0x6800, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
	{ 0x6806, 0x6807, mooncrst_vol_w },
	{ 0x6000, 0x6001, osd_led_w },
	{ 0x6002, 0x6002, pisces_gfxbank_w },
	{ 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( galaxian_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "Free Play" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "7000" )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "12000" )
	PORT_DIPSETTING(    0x03, "20000" )
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( galnamco_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "Free Play" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( pisces_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
/* 	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) */
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
/* 	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	 */
	PORT_DIPNAME( 0x40, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "LC 2C/1C RC 1C/2C 2C/5C" )
	PORT_DIPSETTING(    0x00, "LC 1C/1C RC 1C/5C" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( warofbug_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "Free Play" )
/* 0x80 gives 2 Coins/1 Credit */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( redufo_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "LC 2C/1C RC 1C/3C" )
	PORT_DIPSETTING(    0x00, "LC 1C/1C RC 1C/6C" )
	PORT_DIPSETTING(    0x80, "LC 1C/2C RC 1C/12C" )
	PORT_DIPSETTING(    0xc0, "Free Play" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( pacmanbl_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x80, 0x80, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/5 Credits" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPNAME( 0x02, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( mooncrgx_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )	/* works only in the Gremlin version */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Japanese" )
	PORT_DIPSETTING(    0x80, "English" )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
 	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout galaxian_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout galaxian_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout pisces_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout pisces_spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	3,1,	/* 3*1 line */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2 },	/* I "know" that this bit of the */
	{ 0 },			/* graphics ROMs is 1 */
	0	/* no use */
};



static struct GfxDecodeInfo galaxian_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &galaxian_charlayout,    0,  8 },
	{ 1, 0x0000, &galaxian_spritelayout,  0,  8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo pisces_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pisces_charlayout,    0,  8 },
	{ 1, 0x0000, &pisces_spritelayout,  0,  8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo pacmanbl_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &galaxian_charlayout,    0,  8 },
	{ 1, 0x1000, &galaxian_spritelayout,  0,  8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};



static unsigned char galaxian_color_prom[] =
{
	/* palette */
	0x00,0x00,0x00,0xF6,0x00,0x16,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC0,0xA0,0x07,0x00,0x00,0x00,0x07,0x00,0xF6,0x07,0xF0,0x00,0x76,0x07,0xC6
};

static unsigned char japirem_color_prom[] =
{
	/* palette */
	0x00,0x7A,0x36,0x07,0x00,0xF0,0x38,0x1F,0x00,0xC7,0xF0,0x3F,0x00,0xDB,0xC6,0x38,
	0x00,0x36,0x07,0xF0,0x00,0x33,0x3F,0xDB,0x00,0x3F,0x57,0xC6,0x00,0xC6,0x3F,0xFF
};

static unsigned char uniwars_color_prom[] =
{
	/* palette */
	0x00,0xe8,0x17,0x3f,0x00,0x2f,0x87,0x20,0x00,0xff,0x3f,0x38,0x00,0x83,0x3f,0x06,
	0x00,0xdc,0x1f,0xd0,0x00,0xef,0x20,0x96,0x00,0x3f,0x17,0xf0,0x00,0x3f,0x17,0x14
};

static unsigned char warofbug_color_prom[] =
{
	/* palette */
	0x00,0xFF,0x07,0xC0,0x00,0x07,0xC0,0x3F,0x00,0x38,0x07,0xC0,0x00,0x07,0xC0,0x38,
	0x00,0x3F,0x38,0x07,0x00,0xC0,0x3F,0x07,0x00,0xF8,0x07,0x38,0x00,0xC0,0x38,0xC7,
};

static unsigned char pacmanbl_color_prom[] =
{
	/* palette */
	0x00,0x00,0x00,0xF6,0x00,0x16,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC0,0xA0,0x0C,0x00,0x00,0x00,0x07,0x00,0xF6,0x07,0xF0,0x00,0x76,0x07,0xC6
};

static unsigned char digger_color_prom[] =
{
	0x00,0xC7,0xF0,0x3F,0x00,0xDB,0xC6,0x38,0x00,0xF0,0x15,0x1F,0x00,0xF6,0x06,0x07,
	0x00,0x91,0x07,0xF6,0x00,0xF0,0xFE,0x07,0x00,0x38,0x07,0xFE,0x00,0x07,0x3F,0xFE,
};

static unsigned char mooncrgx_color_prom[] =
{
	/* palette */
	0x00,0x7a,0x36,0x07,0x00,0xf0,0x38,0x1f,0x00,0xc7,0xf0,0x3f,0x00,0xdb,0xc6,0x38,
	0x00,0x36,0x07,0xf0,0x00,0x33,0x3f,0xdb,0x00,0x3f,0x57,0xc6,0x00,0xc6,0x3f,0xff
};



static struct CustomSound_interface custom_interface =
{
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};



static struct MachineDriver galaxian_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,galaxian_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

static struct MachineDriver pisces_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,pisces_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	pisces_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

static struct MachineDriver pacmanbl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,pisces_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	pacmanbl_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

static const char *mooncrst_sample_names[] =
{
	"*galaxian",
	"shot.sam",
	"death.sam",
	0	/* end of array */
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galaxian_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7f", 0x0000, 0x1000, 0xa9d897cc )
	ROM_LOAD( "7j", 0x1000, 0x1000, 0x1b7269ca )
	ROM_LOAD( "7l", 0x2000, 0x1000, 0x3ec2aec6 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h", 0x0000, 0x0800, 0x4852a7c2 )
	ROM_LOAD( "1k", 0x0800, 0x0800, 0x17902ece )
ROM_END

ROM_START( galmidw_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galmidw.u",  0x0000, 0x0800, 0x168a654c )
	ROM_LOAD( "galmidw.v",  0x0800, 0x0800, 0x934ef280 )
	ROM_LOAD( "galmidw.w",  0x1000, 0x0800, 0x587af4d8 )
	ROM_LOAD( "galmidw.y",  0x1800, 0x0800, 0xc2f89d12 )
	ROM_LOAD( "galmidw.z",  0x2000, 0x0800, 0x9471bfe9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galmidw.1j", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "galmidw.1k", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galnamco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galnamco.u",  0x0000, 0x0800, 0xa9f9434b )
	ROM_LOAD( "galnamco.v",  0x0800, 0x0800, 0x1fd66534 )
	ROM_LOAD( "galnamco.w",  0x1000, 0x0800, 0xde73ca2f )
	ROM_LOAD( "galnamco.y",  0x1800, 0x0800, 0x3bddfc4b )
	ROM_LOAD( "galnamco.z",  0x2000, 0x0800, 0x98f4d194 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galnamco.1h", 0x0000, 0x0800, 0x4852a7c2 )
	ROM_LOAD( "galnamco.1k", 0x0800, 0x0800, 0x17902ece )
ROM_END

ROM_START( superg_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "superg.u",  0x0000, 0x0800, 0xaa7b3253 )
	ROM_LOAD( "superg.v",  0x0800, 0x0800, 0xb2c47640 )
	ROM_LOAD( "superg.w",  0x1000, 0x0800, 0x2afb5745 )
	ROM_LOAD( "superg.y",  0x1800, 0x0800, 0xb6749510 )
	ROM_LOAD( "superg.z",  0x2000, 0x0800, 0xd16558c9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "superg.1h", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "superg.1k", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galapx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx.u",  0x0000, 0x0800, 0x57b8a2b6 )
	ROM_LOAD( "galx.v",  0x0800, 0x0800, 0xa08d337b )
	ROM_LOAD( "galx.w",  0x1000, 0x0800, 0x2865868b )
	ROM_LOAD( "galx.y",  0x1800, 0x0800, 0xac089510 )
	ROM_LOAD( "galx.z",  0x2000, 0x0800, 0x6de3d409 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx.1h", 0x0000, 0x0800, 0xea88446e )
	ROM_LOAD( "galx.1k", 0x0800, 0x0800, 0x4aeef848 )
ROM_END

ROM_START( galap1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx_1.rom",   0x0000, 0x2800, 0x9e96085a )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx_1c1.rom", 0x0000, 0x0800, 0xc05187c1 )
	ROM_LOAD( "galx_1c2.rom", 0x0800, 0x0800, 0x8f8f0ecd )
ROM_END

ROM_START( galap4_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galx_4.rom",   0x0000, 0x2800, 0x7d13c18f )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galx_4c1.rom", 0x0000, 0x0800, 0xe3934181 )
	ROM_LOAD( "galx_4c2.rom", 0x0800, 0x0800, 0x8cf8cc7c )
ROM_END

ROM_START( galturbo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "galturbo.u",  0x0000, 0x0800, 0xaa7b3253 )
	ROM_LOAD( "galturbo.v",  0x0800, 0x0800, 0xa08d337b )
	ROM_LOAD( "galturbo.w",  0x1000, 0x0800, 0x2afb5745 )
	ROM_LOAD( "galturbo.y",  0x1800, 0x0800, 0x9574b410 )
	ROM_LOAD( "galturbo.z",  0x2000, 0x0800, 0xd525c4cb )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "galturbo.1h", 0x0000, 0x0800, 0xb545ede3 )
	ROM_LOAD( "galturbo.1k", 0x0800, 0x0800, 0xcfbf64ef )
ROM_END

ROM_START( pisces_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pisces.a1", 0x0000, 0x0800, 0xe94d1451 )
	ROM_LOAD( "pisces.a2", 0x0800, 0x0800, 0x79b828ae )
	ROM_LOAD( "pisces.b2", 0x1000, 0x0800, 0x94a55e7d )
	ROM_LOAD( "pisces.c1", 0x1800, 0x0800, 0xc859bcc9 )
	ROM_LOAD( "pisces.d1", 0x2000, 0x0800, 0x0c767804 )
	ROM_LOAD( "pisces.e2", 0x2800, 0x0800, 0x6d0ac2d8 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pisces.1j", 0x0000, 0x1000, 0x1a5c1d66 )
	ROM_LOAD( "pisces.1k", 0x1000, 0x1000, 0x3a8d10fb )
ROM_END

ROM_START( japirem_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",  0x0000, 0x0800, 0x9cc3c45f )
	ROM_LOAD( "h07_2a.bin",  0x0800, 0x0800, 0x76d8a0c6 )
	ROM_LOAD( "k07_3a.bin",  0x1000, 0x0800, 0x6511100f )
	ROM_LOAD( "m07_4a.bin",  0x1800, 0x0800, 0x0ac76feb )
	ROM_LOAD( "d08p_5a.bin", 0x2000, 0x0800, 0x1ce1f21b )
	ROM_LOAD( "e08p_6a.bin", 0x2800, 0x0800, 0xb2bdb8c9 )
	ROM_LOAD( "m08p_7a.bin", 0x3000, 0x0800, 0x7f2cc704 )
	ROM_LOAD( "n08p_8a.bin", 0x3800, 0x0800, 0x79b90327 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "h01_1.bin",   0x0000, 0x0800, 0x11311ceb )
	ROM_LOAD( "h01_2.bin",   0x0800, 0x0800, 0xc2870825 )
	ROM_LOAD( "k01_1.bin",   0x1000, 0x0800, 0x79b1be9f )
	ROM_LOAD( "k01_2.bin",   0x1800, 0x0800, 0xa42e795c )
ROM_END

ROM_START( uniwars_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",  0x0000, 0x0800, 0x9cc3c45f )
	ROM_LOAD( "h07_2a.bin",  0x0800, 0x0800, 0x76d8a0c6 )
	ROM_LOAD( "k07_3a.bin",  0x1000, 0x0800, 0x6511100f )
	ROM_LOAD( "m07_4a.bin",  0x1800, 0x0800, 0x0ac76feb )
	ROM_LOAD( "u5",          0x2000, 0x0800, 0x37e1e91b )
	ROM_LOAD( "u6",          0x2800, 0x0800, 0x528d7839 )
	ROM_LOAD( "m08p_7a.bin", 0x3000, 0x0800, 0x7f2cc704 )
	ROM_LOAD( "u8",          0x3800, 0x0800, 0xe370a4d6 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "u10",         0x0000, 0x0800, 0x8b6d9ed9 )
	ROM_LOAD( "h01_2.bin",   0x0800, 0x0800, 0xc2870825 )
	ROM_LOAD( "u9",          0x1000, 0x0800, 0x2acb176d )
	ROM_LOAD( "k01_2.bin",   0x1800, 0x0800, 0xa42e795c )
ROM_END

ROM_START( warofbug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "warofbug.u",  0x0000, 0x0800, 0x07c19765 )
	ROM_LOAD( "warofbug.v",  0x0800, 0x0800, 0x577383df )
	ROM_LOAD( "warofbug.w",  0x1000, 0x0800, 0xeda29210 )
	ROM_LOAD( "warofbug.y",  0x1800, 0x0800, 0x30b3e93f )
	ROM_LOAD( "warofbug.z",  0x2000, 0x0800, 0x3dc8509c )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "warofbug.1k", 0x0000, 0x0800, 0x9dd46522 )
	ROM_LOAD( "warofbug.1j", 0x0800, 0x0800, 0x50dd974f )
ROM_END

ROM_START( redufo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ru1a", 0x0000, 0x0800, 0xae67005f )
	ROM_LOAD( "ru2a", 0x0800, 0x0800, 0xcbbacf42 )
	ROM_LOAD( "ru3a", 0x1000, 0x0800, 0xb210562a )
	ROM_LOAD( "ru4a", 0x1800, 0x0800, 0xeeac4674 )
	ROM_LOAD( "ru5a", 0x2000, 0x0800, 0xe75718fd )
	ROM_LOAD( "ru6a", 0x2800, 0x0800, 0xaf51b219 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ruhja", 0x0000, 0x0800, 0xbdc668f6 )
	ROM_LOAD( "rukla", 0x0800, 0x0800, 0xa902210e )
ROM_END

ROM_START( pacmanbl_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "blpac1b",  0x0000, 0x0800, 0x4bd21c86 )
	ROM_LOAD( "blpac2b",  0x0800, 0x0800, 0xe07062da )
	ROM_LOAD( "blpac3b",  0x1000, 0x0800, 0xafc1dd17 )
	ROM_LOAD( "blpac4b",  0x1800, 0x0800, 0x8ed7013f )
	ROM_LOAD( "blpac5b",  0x2000, 0x0800, 0x64a3d81d )
	ROM_LOAD( "blpac6b",  0x2800, 0x0800, 0x3c79c681 )
	ROM_LOAD( "blpac7b",  0x3000, 0x0800, 0x9b4800c6 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "blpac12b", 0x0000, 0x0800, 0xb3aff349 )
	ROM_LOAD( "blpac11b", 0x0800, 0x0800, 0x068fcb5b )
	ROM_LOAD( "blpac10b", 0x1000, 0x0800, 0x93c21554 )
	ROM_LOAD( "blpac9b",  0x1800, 0x0800, 0x9df6dba6 )
ROM_END

ROM_START( digger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "z1", 0x0000, 0x1000, 0xcaa36573 )
	ROM_LOAD( "z2", 0x1000, 0x1000, 0x3f51b4a7 )
	ROM_LOAD( "z3", 0x2000, 0x1000, 0x1f56e5bc )
	ROM_LOAD( "z4", 0x3000, 0x1000, 0x14641a92 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "z5", 0x0000, 0x1000, 0xb9732537 )
	ROM_LOAD( "z6", 0x1000, 0x1000, 0x24b20f8e )
ROM_END

ROM_START( zigzag_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "zz1", 0x0000, 0x1000, 0xb3a75d89 )
	ROM_LOAD( "zz2", 0x1000, 0x1000, 0x3f51b4a7 )
	ROM_LOAD( "zz3", 0x2000, 0x1000, 0x1f56e5bc )
	ROM_LOAD( "zz4", 0x3000, 0x1000, 0x14641a92 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zz5", 0x0000, 0x1000, 0xb9732537 )
	ROM_LOAD( "zz6", 0x1000, 0x1000, 0x24b20f8e )
ROM_END

ROM_START( mooncrgx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1", 0x0000, 0x0800, 0xbd53f38d )
	ROM_LOAD( "2", 0x0800, 0x0800, 0x5907a613 )
	ROM_LOAD( "3", 0x1000, 0x0800, 0x1d78f6cc )
	ROM_LOAD( "4", 0x1800, 0x0800, 0x0be2a472 )
	ROM_LOAD( "5", 0x2000, 0x0800, 0xef14f1f4 )
	ROM_LOAD( "6", 0x2800, 0x0800, 0xdc48b506 )
	ROM_LOAD( "7", 0x3000, 0x0800, 0x90ca9316 )
	ROM_LOAD( "8", 0x3800, 0x0800, 0xd29f6aa7 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "10.chr", 0x0000, 0x0800, 0x22bd9067 )
	ROM_LOAD( "12.chr", 0x0800, 0x0800, 0xdfbc68ba )
	ROM_LOAD( "9.chr",  0x1000, 0x0800, 0x377a137c )
	ROM_LOAD( "11.chr", 0x1800, 0x0800, 0xc1dc1cde )
ROM_END



static int galaxian_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for the checkerboard pattern to be on screen */
	if (memcmp(&RAM[0x5000],"\x30\x32",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x40a8],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void galaxian_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x40a8],3);
		osd_fclose(f);
	}
}



static int pisces_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for the screen to initialize */
	if (memcmp(&RAM[0x5000],"\x10\x10\x10",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4021],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void pisces_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4021],3);
		osd_fclose(f);
	}
}

static int warofbug_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for memory to be set */
	if (memcmp(&RAM[0x4045],"\x1F\x1F",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4034],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void warofbug_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4034],3);
		osd_fclose(f);
	}
}



static int pacmanbl_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for "HIGH" to be on screen */
	if (memcmp(&RAM[0x5240],"\x48\x40",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			osd_fread(f,&RAM[0x4288],3);
			osd_fclose(f);

			hi = (RAM[0x4288] & 0x0f) +
					(RAM[0x4288] >> 4) * 10 +
					(RAM[0x4289] & 0x0f) * 100 +
					(RAM[0x4289] >> 4) * 1000 +
					(RAM[0x428a] & 0x0f) * 10000 +
					(RAM[0x428a] >> 4) * 100000;

			if (hi > 0)
				RAM[0x5180] = RAM[0x4288] & 0x0F;
			if (hi >= 10)
				RAM[0x51A0] = RAM[0x4288] >> 4;
			if (hi >= 100)
				RAM[0x51C0] = RAM[0x4289] & 0x0F;
			if (hi >= 1000)
				RAM[0x51E0] = RAM[0x4289] >> 4;
			if (hi >= 10000)
				RAM[0x5200] = RAM[0x428a] & 0x0F;
			if (hi >= 100000)
				RAM[0x5220] = RAM[0x428a] >> 4;
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void pacmanbl_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4288],3);
		osd_fclose(f);
	}
}



struct GameDriver galaxian_driver =
{
	__FILE__,
	0,
	"galaxian",
	"Galaxian (Namco)",
	"1979",
	"Namco",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galaxian_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galaxian_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galmidw_driver =
{
	__FILE__,
	&galaxian_driver,
	"galmidw",
	"Galaxian (Midway)",
	"1979",
	"[Namco] (Midway license)",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galmidw_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galaxian_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galnamco_driver =
{
	__FILE__,
	&galaxian_driver,
	"galnamco",
	"Galaxian (Namco, modified)",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galnamco_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver superg_driver =
{
	__FILE__,
	&galaxian_driver,
	"superg",
	"Super Galaxians",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	superg_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galapx_driver =
{
	__FILE__,
	&galaxian_driver,
	"galapx",
	"Galaxian Part X",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galapx_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galap1_driver =
{
	__FILE__,
	&galaxian_driver,
	"galap1",
	"Space Invaders Galactica",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galap1_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galap4_driver =
{
	__FILE__,
	&galaxian_driver,
	"galap4",
	"Galaxian Part 4",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galap4_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver galturbo_driver =
{
	__FILE__,
	&galaxian_driver,
	"galturbo",
	"Galaxian Turbo",
	"1979",
	"hack",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	galturbo_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver pisces_driver =
{
	__FILE__,
	0,
	"pisces",
	"Pisces",
	"????",
	"<unknown>",
	"Robert Anschuetz\nNicola Salmoria\nAndrew Scott\nMike Balfour\nMarco Cassili",
	0,
	&pisces_machine_driver,

	pisces_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	pisces_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	pisces_hiload, pisces_hisave
};

struct GameDriver japirem_driver =
{
	__FILE__,
	0,
	"japirem",
	"Gingateikoku No Gyakushu",
	"1980",
	"Irem",
	"Nicola Salmoria\nLionel Theunissen\nRobert Anschuetz\nAndrew Scott\nMarco Cassili",
	0,
	&pisces_machine_driver,

	japirem_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	japirem_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver uniwars_driver =
{
	__FILE__,
	&japirem_driver,
	"uniwars",
	"Uniwars",
	"1980",
	"Karateco",
	"Nicola Salmoria\nGary Walton\nRobert Anschuetz\nAndrew Scott\nMarco Cassili",
	0,
	&pisces_machine_driver,

	uniwars_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	galnamco_input_ports,

	uniwars_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

struct GameDriver warofbug_driver =
{
	__FILE__,
	0,
	"warofbug",
	"War of the Bugs",
	"1981",
	"Armenia",
	"Robert Aanchuetz\nNicola Salmoria\nAndrew Scott\nMike Balfour\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	warofbug_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	warofbug_input_ports,

	warofbug_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	warofbug_hiload, warofbug_hisave
};

struct GameDriver redufo_driver =
{
	__FILE__,
	0,
	"redufo",
	"Defend the Terra Attack on the Red UFO",
	"????",
	"hack",
	"Robert Aanchuetz\nNicola Salmoria\nAndrew Scott\nValerio Verrando (high score save)\nMarco Cassili",
	0,
	&galaxian_machine_driver,

	redufo_rom,
	0, 0,
	mooncrst_sample_names,
	0,      /* sound_prom */

	redufo_input_ports,

	galaxian_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	galaxian_hiload, galaxian_hisave
};

extern struct GameDriver pacman_driver;
struct GameDriver pacmanbl_driver =
{
	__FILE__,
	&pacman_driver,
	"pacmanbl",
	"Pac Man (bootleg on Galaxian hardware)",
	"1981",
	"bootleg",
	"Robert Aanchuetz\nNicola Salmoria\nAndrew Scott\nValerio Verrando (high score save)\nMarco Cassili",
	0,
	&pacmanbl_machine_driver,

	pacmanbl_rom,
	0, 0,
	mooncrst_sample_names,
	0,      /* sound_prom */

	pacmanbl_input_ports,

	pacmanbl_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	pacmanbl_hiload, pacmanbl_hisave
};

struct GameDriver digger_driver =
{
	__FILE__,
	0,
	"digger",
	"Digger",
	"????",
	"?????",
	"Robert Aanchuetz\nNicola Salmoria\nAndrew Scott",
	0,
	&pisces_machine_driver,

	digger_rom,
	0, 0,
	mooncrst_sample_names,
	0,      /* sound_prom */

	pisces_input_ports,

	digger_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver zigzag_driver =
{
	__FILE__,
	0,
	"zigzag",
	"Zig Zag",
	"????",
	"?????",
	"Robert Aanchuetz\nNicola Salmoria\nAndrew Scott",
	0,
	&pisces_machine_driver,

	zigzag_rom,
	0, 0,
	mooncrst_sample_names,
	0,      /* sound_prom */

	pisces_input_ports,

	digger_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

extern struct GameDriver mooncrst_driver;
struct GameDriver mooncrgx_driver =
{
	__FILE__,
	&mooncrst_driver,
	"mooncrgx",
	"Moon Cresta (bootleg on Galaxian hardware)",
	"1980",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&pisces_machine_driver,

	mooncrgx_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrgx_input_ports,

	mooncrgx_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
