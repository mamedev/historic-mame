/***************************************************************************

tecmo.c

M68000 based Tecmo games (Final Starforce) may fit in here as well


Silkworm memory map (preliminary)

0000-bfff ROM
c000-c1ff Background video RAM #2
c200-c3ff Background color RAM #2
c400-c5ff Background video RAM #1
c600-c7ff Background color RAM #1
c800-cbff Video RAM
cc00-cfff Color RAM
d000-dfff RAM
e000-e7ff Sprites
e800-efff Palette RAM, groups of 2 bytes, 4 bits per gun: xB RG
          e800-e9ff sprites
          ea00-ebff characters
          ec00-edff bg #1
          ee00-efff bg #2
f000-f7ff window for banked ROM

read:
f800      IN0 (heli) bit 0-3
f801      IN0 bit 4-7
f802      IN1 (jeep) bit 0-3
f803      IN1 bit 4-7
f806      DSWA bit 0-3
f807      DSWA bit 4-7
f808      DSWB bit 0-3
f809      DSWB bit 4-7
f80f      COIN

write:
f800-f801 bg #1 x scroll
f802      bg #1 y scroll
f803-f804 bg #2 x scroll
f805      bg #2 y scroll
f806      ????
f808      ROM bank selector
f809      ????
f80b      ????

***************************************************************************

Rygar memory map (preliminary)

read:
f800	player #1 joystick
f801	player #1 buttons; service
f802	player #2 joystick (mirror player#1 - since players take turns)
f803	player #2 buttons (cocktail mode reads these)
f804	start, coins
f806	DSWA
f807	DSWA cocktail
f808	DSWB
f809	DSWB

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



void tecmo_bankswitch_w(int offset,int data);
int tecmo_bankedrom_r(int offset);



void tecmo_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	cpu_setbank(1,&RAM[bankaddress]);
}

static void tecmo_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

static int tecmo_adpcm_start;

static void tecmo_adpcm_lo_w(int offset,int data)
{
	tecmo_adpcm_start = (tecmo_adpcm_start & 0xff00) | data;
}
static void tecmo_adpcm_hi_w(int offset,int data)
{
	tecmo_adpcm_start = (tecmo_adpcm_start & 0x00ff) | (data << 8);
}
static void tecmo_adpcm_trigger_w(int offset,int data)
{
	if (data & 0x0f)	/* maybe this selects the volume? */
		ADPCM_trigger(0,tecmo_adpcm_start);
}



extern unsigned char *tecmo_videoram,*tecmo_colorram;
extern unsigned char *tecmo_videoram2,*tecmo_colorram2;
extern unsigned char *tecmo_scroll;
extern int tecmo_videoram2_size;

void tecmo_videoram_w(int offset,int data);
void tecmo_colorram_w(int offset,int data);

int rygar_vh_start(void);
int silkworm_vh_start(void);
int gemini_vh_start(void);

void tecmo_vh_stop(void);
void tecmo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_BANK1 },
	{ 0xf800, 0xf800, input_port_0_r },
	{ 0xf801, 0xf801, input_port_1_r },
	{ 0xf802, 0xf802, input_port_2_r },
	{ 0xf803, 0xf803, input_port_3_r },
	{ 0xf804, 0xf804, input_port_4_r },
	{ 0xf805, 0xf805, input_port_5_r },
	{ 0xf806, 0xf806, input_port_6_r },
	{ 0xf807, 0xf807, input_port_7_r },
	{ 0xf808, 0xf808, input_port_8_r },
	{ 0xf809, 0xf809, input_port_9_r },
	{ 0xf80f, 0xf80f, input_port_10_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress silkworm_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc1ff, videoram_w, &videoram, &videoram_size },
	{ 0xc200, 0xc3ff, colorram_w, &colorram },
	{ 0xc400, 0xc5ff, tecmo_videoram_w, &tecmo_videoram },
	{ 0xc600, 0xc7ff, tecmo_colorram_w, &tecmo_colorram },
	{ 0xc800, 0xcbff, MWA_RAM, &tecmo_videoram2, &tecmo_videoram2_size },
	{ 0xcc00, 0xcfff, MWA_RAM, &tecmo_colorram2 },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe800, 0xefff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf805, MWA_RAM, &tecmo_scroll },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, MWA_NOP },	/* ???? */
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf809, 0xf809, MWA_NOP },	/* ???? */
	{ 0xf80b, 0xf80b, MWA_NOP },	/* ???? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress rygar_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, MWA_RAM, &tecmo_videoram2, &tecmo_videoram2_size },
	{ 0xd400, 0xd7ff, MWA_RAM, &tecmo_colorram2 },
	{ 0xd800, 0xd9ff, tecmo_videoram_w, &tecmo_videoram },
	{ 0xda00, 0xdbff, tecmo_colorram_w, &tecmo_colorram },
	{ 0xdc00, 0xddff, videoram_w, &videoram, &videoram_size },
	{ 0xde00, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe800, 0xefff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf805, MWA_RAM, &tecmo_scroll },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, MWA_NOP },	/* ???? */
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf809, 0xf809, MWA_NOP },	/* ???? */
	{ 0xf80b, 0xf80b, MWA_NOP },	/* ???? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress gemini_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, MWA_RAM, &tecmo_videoram2, &tecmo_videoram2_size },
	{ 0xd400, 0xd7ff, MWA_RAM, &tecmo_colorram2 },
	{ 0xd800, 0xd9ff, tecmo_videoram_w, &tecmo_videoram },
	{ 0xda00, 0xdbff, tecmo_colorram_w, &tecmo_colorram },
	{ 0xdc00, 0xddff, videoram_w, &videoram, &videoram_size },
	{ 0xde00, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xe7ff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xe800, 0xefff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf805, MWA_RAM, &tecmo_scroll },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, MWA_NOP },	/* ???? */
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf809, 0xf809, MWA_NOP },	/* ???? */
	{ 0xf80b, 0xf80b, MWA_NOP },	/* ???? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_lo_w },
	{ 0xc400, 0xc400, tecmo_adpcm_hi_w },
	{ 0xc800, 0xc800, tecmo_adpcm_trigger_w },
	{ 0xcc00, 0xcc00, MWA_NOP },	/* NMI acknowledge? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress rygar_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress rygar_sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM3812_control_port_0_w },
	{ 0x8001, 0x8001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_hi_w },
	{ 0xd000, 0xd000, tecmo_adpcm_lo_w },
	{ 0xe000, 0xe000, tecmo_adpcm_trigger_w },
	{ 0xf000, 0xf000, MWA_NOP },	/* NMI acknowledge? */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( silkworm_input_ports )
	PORT_START	/* IN0 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START	/* IN0 bit 4-7 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* unused? */

	PORT_START	/* IN1 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN1 bit 4-7 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* unused? */

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0C, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0C, "1 Coin/3 Credits" )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x00, "A 7", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 800000" )
	PORT_DIPSETTING(    0x02, "50000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x05, "100000" )
	PORT_DIPSETTING(    0x06, "200000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x08, 0x00, "B 4", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	/* 0x06 and 0x07 are the same as 0x00 */
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

INPUT_PORTS_START( rygar_input_ports )
	PORT_START	/* IN0 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START	/* IN1 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN4 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0C, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0C, "1 Coin/3 Credits" )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 600000" )
	PORT_DIPSETTING(    0x02, "200000 500000" )
	PORT_DIPSETTING(    0x03, "100000" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, "2P Can Start Anytime", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x04, "Yes" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x08, "Yes" )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gemini_input_ports )
	PORT_START	/* IN0 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START	/* IN1 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN4 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x07, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Final Round Continuation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Round 6" )
	PORT_DIPSETTING(    0x08, "Round 7" )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Buy in During Final Round", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x08, "Yes" )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Hardest" )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 200000" )
	PORT_DIPSETTING(    0x01, "50000 300000" )
	PORT_DIPSETTING(    0x02, "100000 500000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x04, "100000" )
	PORT_DIPSETTING(    0x05, "200000" )
	PORT_DIPSETTING(    0x06, "300000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout tecmo_charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout silkworm_spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout silkworm_spritelayout2x =
{
	32,32,	/* 32*32 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4,
			128*8+0*4, 128*8+1*4, 128*8+2*4, 128*8+3*4, 128*8+4*4, 128*8+5*4, 128*8+6*4, 128*8+7*4,
			160*8+0*4, 160*8+1*4, 160*8+2*4, 160*8+3*4, 160*8+4*4, 160*8+5*4, 160*8+6*4, 160*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
			64*32, 65*32, 66*32, 67*32, 68*32, 69*32, 70*32, 71*32,
			80*32, 81*32, 82*32, 83*32, 84*32, 85*32, 86*32, 87*32 },
	512*8	/* every sprite takes 512 consecutive bytes */
};

static struct GfxLayout silkworm_spritelayout8x8 =
{
	8,8,	/* 8*8 xprites */
	8192,	/* 8192 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

/* the only difference in rygar_spritelayout is that half as many sprites are present */

static struct GfxLayout rygar_spritelayout = /* only difference is half as many sprites as silkworm */
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout rygar_spritelayout2x =
{
	32,32,	/* 32*32 sprites */
	256,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4,
			128*8+0*4, 128*8+1*4, 128*8+2*4, 128*8+3*4, 128*8+4*4, 128*8+5*4, 128*8+6*4, 128*8+7*4,
			160*8+0*4, 160*8+1*4, 160*8+2*4, 160*8+3*4, 160*8+4*4, 160*8+5*4, 160*8+6*4, 160*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
			64*32, 65*32, 66*32, 67*32, 68*32, 69*32, 70*32, 71*32,
			80*32, 81*32, 82*32, 83*32, 84*32, 85*32, 86*32, 87*32 },
	512*8	/* every sprite takes 512 consecutive bytes */
};

static struct GfxLayout rygar_spritelayout8x8 =
{
	8,8,	/* 8*8 xprites */
	4096,	/* 8192 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tecmo_charlayout,        256, 16 },	/* colors 256 - 511 */
	{ 1, 0x08000, &silkworm_spritelayout8x8,  0, 16 },	/* colors   0 - 255 */
	{ 1, 0x08000, &silkworm_spritelayout,     0, 16 },	/* 16x16 sprites */
	{ 1, 0x08000, &silkworm_spritelayout2x,   0, 16 },	/* double size hack */
	{ 1, 0x48000, &silkworm_spritelayout,   512, 16 },	/* bg#1 colors 512 - 767 */
	{ 1, 0x88000, &silkworm_spritelayout,   768, 16 },	/* bg#2 colors 768 - 1023 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rygar_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tecmo_charlayout,     256, 16 },	/* colors 256 - 511 */
	{ 1, 0x08000, &rygar_spritelayout8x8,  0, 16 },	/* colors   0 - 255 */
	{ 1, 0x08000, &rygar_spritelayout,     0, 16 },	/* 16x16 sprites */
	{ 1, 0x08000, &rygar_spritelayout2x,   0, 16 },	/* double size hack */
	{ 1, 0x28000, &rygar_spritelayout,   512, 16 },	/* bg#1 colors 512 - 767 */
	{ 1, 0x48000, &rygar_spritelayout,   768, 16 },	/* bg#2 colors 768 - 1023 */
	{ -1 } /* end of array */
};



static struct YM3526interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,       /* 8000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};

static struct MachineDriver silkworm_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			7600000,	/* 7.6 Mhz (?????) */
			0,
			readmem,silkworm_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,2	/* ?? */
						/* NMIs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	silkworm_vh_start,
	tecmo_vh_stop,
	tecmo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver rygar_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			7600000,
			0,
			readmem,rygar_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ???? */
			2,	/* memory region #2 */
			rygar_sound_readmem,rygar_sound_writemem,0,0,
			interrupt,2	/* ?? */
						/* NMIs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	rygar_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	rygar_vh_start,
	tecmo_vh_stop,
	tecmo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		},
	}
};

static struct MachineDriver gemini_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			7600000,	/* 7.6 Mhz (?????) */
			0,
			readmem,gemini_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,2	/* ?? */
						/* NMIs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gemini_vh_start,
	tecmo_vh_stop,
	tecmo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rygar_rom )
	ROM_REGION(0x18000)	/* 64k for code */
	ROM_LOAD( "CPU_5P.BIN", 0x00000, 0x08000, 0xff6100e3 ) /* code */
	ROM_LOAD( "CPU_5M.BIN", 0x08000, 0x04000, 0x94172f9f ) /* code */
	ROM_LOAD( "CPU_5J.BIN", 0x10000, 0x08000, 0x48d6187c ) /* banked at f000-f7ff */

	ROM_REGION(0x68000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CPU_8K.BIN",  0x00000, 0x08000, 0x4f88d512 )	/* characters */

	ROM_LOAD( "VID_6K.BIN",  0x08000, 0x08000, 0x97c92065 )	/* sprites */
	ROM_LOAD( "VID_6J.BIN",  0x10000, 0x08000, 0x7f08b292 )	/* sprites */
	ROM_LOAD( "VID_6H.BIN",  0x18000, 0x08000, 0x8fa1b533 )	/* sprites */
	ROM_LOAD( "VID_6G.BIN",  0x20000, 0x08000, 0x3a9c929c )	/* sprites */

	ROM_LOAD( "VID_6P.BIN",  0x28000, 0x08000, 0x13e6b7cc )
	ROM_LOAD( "VID_6O.BIN",  0x30000, 0x08000, 0xe47401ce )
	ROM_LOAD( "VID_6N.BIN",  0x38000, 0x08000, 0x0a5e9210 )
	ROM_LOAD( "VID_6L.BIN",  0x40000, 0x08000, 0x970036da )

	ROM_LOAD( "VID_6F.BIN",  0x48000, 0x08000, 0x64eb6eb9 )
	ROM_LOAD( "VID_6E.BIN",  0x50000, 0x08000, 0xd096db24 )
	ROM_LOAD( "VID_6C.BIN",  0x58000, 0x08000, 0x9ff6bcd4 )
	ROM_LOAD( "VID_6B.BIN",  0x60000, 0x08000, 0x9289536b )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "CPU_4H.BIN",  0x0000, 0x4000, 0x3d840000 )

	ROM_REGION(0x4000)	/* ADPCM samples */
	ROM_LOAD( "CPU_1F.BIN",  0x0000, 0x4000, 0xf592c358 )
ROM_END

ROM_START( rygarj_rom )
	ROM_REGION(0x18000)	/* 64k for code */

	ROM_LOAD( "CPUJ_5P.BIN", 0x00000, 0x08000, 0x4ff67dda ) /* code */
	ROM_LOAD( "CPUJ_5M.BIN", 0x08000, 0x04000, 0x55f3a025 ) /* code */
	ROM_LOAD( "CPUJ_5J.BIN", 0x10000, 0x08000, 0x777a78aa ) /* banked at f000-f7ff */

	ROM_REGION(0x68000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CPUJ_8K.BIN", 0x00000, 0x08000, 0xb94a3248 )	/* characters */

	ROM_LOAD( "VID_6K.BIN",  0x08000, 0x08000, 0x97c92065 )	/* sprites */
	ROM_LOAD( "VID_6J.BIN",  0x10000, 0x08000, 0x7f08b292 )	/* sprites */
	ROM_LOAD( "VID_6H.BIN",  0x18000, 0x08000, 0x8fa1b533 )	/* sprites */
	ROM_LOAD( "VID_6G.BIN",  0x20000, 0x08000, 0x3a9c929c )	/* sprites */

	ROM_LOAD( "VID_6P.BIN",  0x28000, 0x08000, 0x13e6b7cc )
	ROM_LOAD( "VID_6O.BIN",  0x30000, 0x08000, 0xe47401ce )
	ROM_LOAD( "VID_6N.BIN",  0x38000, 0x08000, 0x0a5e9210 )
	ROM_LOAD( "VID_6L.BIN",  0x40000, 0x08000, 0x970036da )

	ROM_LOAD( "VID_6F.BIN",  0x48000, 0x08000, 0x64eb6eb9 )
	ROM_LOAD( "VID_6E.BIN",  0x50000, 0x08000, 0xd096db24 )
	ROM_LOAD( "VID_6C.BIN",  0x58000, 0x08000, 0x9ff6bcd4 )
	ROM_LOAD( "VID_6B.BIN",  0x60000, 0x08000, 0x9289536b )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "CPU_4H.BIN",  0x0000, 0x4000, 0x3d840000 )

	ROM_REGION(0x4000)	/* ADPCM samples */
	ROM_LOAD( "CPU_1F.BIN",  0x0000, 0x4000, 0xf592c358 )
ROM_END

ROM_START( silkworm_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "silkworm.4",  0x00000, 0x10000, 0x8242f71e )	/* c000-ffff is not used */
	ROM_LOAD( "silkworm.5",  0x10000, 0x10000, 0xdc2e3a8c )	/* banked at f000-f7ff */

	ROM_REGION(0xc8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "silkworm.2",  0x00000, 0x08000, 0xcc03f6a3 )	/* characters */
	ROM_LOAD( "silkworm.6",  0x08000, 0x10000, 0x9fa0b862 )	/* sprites */
	ROM_LOAD( "silkworm.7",  0x18000, 0x10000, 0x61dfce63 )	/* sprites */
	ROM_LOAD( "silkworm.8",  0x28000, 0x10000, 0xc7a80a5c )	/* sprites */
	ROM_LOAD( "silkworm.9",  0x38000, 0x10000, 0x4b6e4340 )	/* sprites */
	ROM_LOAD( "silkworm.10", 0x48000, 0x10000, 0xfad1bcad )	/* tiles #1 */
	ROM_LOAD( "silkworm.11", 0x58000, 0x10000, 0x35f18a5b )	/* tiles #1 */
	ROM_LOAD( "silkworm.12", 0x68000, 0x10000, 0xc4faff70 )	/* tiles #1 */
	ROM_LOAD( "silkworm.13", 0x78000, 0x10000, 0x98692fdd )	/* tiles #1 */
	ROM_LOAD( "silkworm.14", 0x88000, 0x10000, 0x23e5846f )	/* tiles #2 */
	ROM_LOAD( "silkworm.15", 0x98000, 0x10000, 0xb389f5f5 )	/* tiles #2 */
	ROM_LOAD( "silkworm.16", 0xa8000, 0x10000, 0x783c76d8 )	/* tiles #2 */
	ROM_LOAD( "silkworm.17", 0xb8000, 0x10000, 0xf292cf5e )	/* tiles #2 */

	ROM_REGION(0x20000)	/* 64k for the audio CPU */
	ROM_LOAD( "silkworm.3",  0x0000, 0x8000, 0x0867f097 )

	ROM_REGION(0x8000)	/* ADPCM samples */
	ROM_LOAD( "silkworm.1",  0x0000, 0x8000, 0x83601ea4 )
ROM_END

ROM_START( gemini_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "gw04-5s.rom",  0x00000, 0x10000, 0x6ae20f36 )	/* c000-ffff is not used */
	ROM_LOAD( "gw05-6s.rom",  0x10000, 0x10000, 0xe6dc716c )	/* banked at f000-f7ff */

	ROM_REGION(0xc8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gw02-3h.rom",  0x00000, 0x08000, 0x1b7f9715 )	/* characters */
	ROM_LOAD( "gw06-1c.rom",  0x08000, 0x10000, 0xa435719b )	/* sprites */
	ROM_LOAD( "gw07-1d.rom",  0x18000, 0x10000, 0xb93f65df )	/* sprites */
	ROM_LOAD( "gw08-1f.rom",  0x28000, 0x10000, 0xcdc53fb1 )	/* sprites */
	ROM_LOAD( "gw09-1h.rom",  0x38000, 0x10000, 0xf7686c68 )	/* sprites */
	ROM_LOAD( "gw10-1n.rom",  0x48000, 0x10000, 0x7ab6d402 )	/* tiles #1 */
	ROM_LOAD( "gw11-2na.rom", 0x58000, 0x10000, 0x22d12005 )	/* tiles #1 */
	ROM_LOAD( "gw12-2nb.rom", 0x68000, 0x10000, 0xd4873e8d )	/* tiles #1 */
	ROM_LOAD( "gw13-3n.rom",  0x78000, 0x10000, 0xe80df095 )	/* tiles #1 */
	ROM_LOAD( "gw14-1r.rom",  0x88000, 0x10000, 0x6e2bb603 )	/* tiles #2 */
	ROM_LOAD( "gw15-2ra.rom", 0x98000, 0x10000, 0x5117008f )	/* tiles #2 */
	ROM_LOAD( "gw16-2rb.rom", 0xa8000, 0x10000, 0x20036f93 )	/* tiles #2 */
	ROM_LOAD( "gw17-3r.rom",  0xb8000, 0x10000, 0x9fe690b0 )	/* tiles #2 */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gw03-5h.rom", 0x0000, 0x8000, 0x81722c2e )

	ROM_REGION(0x8000)	/* ADPCM samples */
	ROM_LOAD( "gw01-6a.rom", 0x0000, 0x8000, 0xa0da9812 )
ROM_END



ADPCM_SAMPLES_START(rygar_samples)
	ADPCM_SAMPLE(0x0005, 0x0005, (0x060b-0x0005)*2)
	ADPCM_SAMPLE(0x060b, 0x060b, (0x0c1a-0x060b)*2)
	ADPCM_SAMPLE(0x0c1a, 0x0c1a, (0x1a3f-0x0c1a)*2)
	ADPCM_SAMPLE(0x1a3f, 0x1a3f, (0x4000-0x1a3f)*2)
ADPCM_SAMPLES_END

/* We are probably missing some samples here. The first half of the ADPCM ROMs */
/* of Silkworm and Gemini Wing is identical, and it is the only part used here. */
ADPCM_SAMPLES_START(silkworm_samples)
	ADPCM_SAMPLE(0x0000, 0x0000, 0x0100*2)
	ADPCM_SAMPLE(0x0100, 0x0100, 0x0200*2)
	ADPCM_SAMPLE(0x0300, 0x0300, 0x0100*2)
	ADPCM_SAMPLE(0x0404, 0x0404, 0x0100*2)
	ADPCM_SAMPLE(0x0504, 0x0504, 0x0200*2)
	ADPCM_SAMPLE(0x0704, 0x0704, 0x0100*2)
	ADPCM_SAMPLE(0x0808, 0x0808, 0x0100*2)
	ADPCM_SAMPLE(0x0908, 0x0908, 0x0200*2)
	ADPCM_SAMPLE(0x0b08, 0x0b08, 0x0100*2)
	ADPCM_SAMPLE(0x0c0c, 0x0c0c, 0x0100*2)
	ADPCM_SAMPLE(0x0d0c, 0x0d0c, 0x0200*2)
	ADPCM_SAMPLE(0x0f0c, 0x0f0c, 0x0100*2)
	ADPCM_SAMPLE(0x1010, 0x1010, 0x0100*2)
	ADPCM_SAMPLE(0x1110, 0x1110, 0x0200*2)
	ADPCM_SAMPLE(0x1310, 0x1310, 0x0100*2)
	ADPCM_SAMPLE(0x1414, 0x1414, 0x0100*2)
	ADPCM_SAMPLE(0x1514, 0x1514, 0x0200*2)
	ADPCM_SAMPLE(0x1714, 0x1714, 0x0100*2)
	ADPCM_SAMPLE(0x1818, 0x1818, 0x0100*2)
	ADPCM_SAMPLE(0x1918, 0x1918, 0x0200*2)
	ADPCM_SAMPLE(0x1b18, 0x1b18, 0x0100*2)
	ADPCM_SAMPLE(0x1c1c, 0x1c1c, 0x0100*2)
	ADPCM_SAMPLE(0x1d1c, 0x1d1c, 0x0200*2)
	ADPCM_SAMPLE(0x1f1c, 0x1f1c, 0x0100*2)
	ADPCM_SAMPLE(0x2020, 0x2020, 0x0100*2)
	ADPCM_SAMPLE(0x2120, 0x2120, 0x0200*2)
	ADPCM_SAMPLE(0x2320, 0x2320, 0x0100*2)
	ADPCM_SAMPLE(0x2424, 0x2424, 0x0100*2)
	ADPCM_SAMPLE(0x2524, 0x2524, 0x0200*2)
	ADPCM_SAMPLE(0x2724, 0x2724, 0x0100*2)
	ADPCM_SAMPLE(0x2828, 0x2828, 0x0100*2)
	ADPCM_SAMPLE(0x2928, 0x2928, 0x0200*2)
	ADPCM_SAMPLE(0x2b28, 0x2b28, 0x0100*2)
	ADPCM_SAMPLE(0x2c2c, 0x2c2c, 0x0100*2)
	ADPCM_SAMPLE(0x2d2c, 0x2d2c, 0x0200*2)
	ADPCM_SAMPLE(0x2f2c, 0x2f2c, 0x0100*2)
	ADPCM_SAMPLE(0x3030, 0x3030, 0x0100*2)
	ADPCM_SAMPLE(0x3130, 0x3130, 0x0200*2)
	ADPCM_SAMPLE(0x3330, 0x3330, 0x0100*2)
	ADPCM_SAMPLE(0x3434, 0x3434, 0x0100*2)
	ADPCM_SAMPLE(0x3534, 0x3534, 0x0200*2)
	ADPCM_SAMPLE(0x3734, 0x3734, 0x0100*2)
	ADPCM_SAMPLE(0x3838, 0x3838, 0x0100*2)
	ADPCM_SAMPLE(0x3938, 0x3938, 0x0200*2)
	ADPCM_SAMPLE(0x3b38, 0x3b38, 0x0100*2)
	ADPCM_SAMPLE(0x3c3c, 0x3c3c, 0x0100*2)
	ADPCM_SAMPLE(0x3d3c, 0x3d3c, 0x0200*2)
	ADPCM_SAMPLE(0x3f3c, 0x3f3c, 0x0100*2)
ADPCM_SAMPLES_END



static int rygar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc984],"\x00\x03\x00\x00",4) == 0 &&
			memcmp(&RAM[0xcb46],"\x00\x03\x00\x00",4) == 0 &&
			memcmp(&RAM[0xc023],"\x00\x00\x03\x00",4) == 0)	/* high score */
	{
		void *f;
		int p;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc983],9*50);
			RAM[0xc023]=RAM[0xc987];
			RAM[0xc024]=RAM[0xc986];
			RAM[0xc025]=RAM[0xc985];
			RAM[0xc026]=RAM[0xc984];
			osd_fclose(f);

			/* update the screen hi score (now or never) */

			for (p=0;p<4;p++)
			{
				RAM[0xd06c+(p*2)]=0x60+(RAM[0xc984+p]/16);
				RAM[0xd06d+(p*2)]=0x60+(RAM[0xc984+p]%16);
			}
			for (p=0;p<6;p++ )
			{
				if (RAM[0xd06c+p]==0x60)
						RAM[0xd06c+p]=0x01;
				else break;
			}
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void rygar_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc983],9*50);
		osd_fclose(f);
	}
}


static int silkworm_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd262],"\x00\x00\x03",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xd262],10*10);
			osd_fread(f,&RAM[0xd54e],4);
			osd_fread(f,&RAM[0xd572],4);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void silkworm_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xd262],10*10);
		osd_fwrite(f,&RAM[0xd54e],4);
		osd_fwrite(f,&RAM[0xd572],4);
		osd_fclose(f);
	}

}


static int gemini_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc026],"\x00\x50\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc026],3);
			osd_fread(f,&RAM[0xcf41],3*10+4*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void gemini_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc026],3);
		osd_fwrite(f,&RAM[0xcf41],3*10+4*10);
		osd_fclose(f);
	}
}



struct GameDriver rygar_driver =
{
	__FILE__,
	0,
	"rygar",
	"Rygar (US)",
	"1986",
	"Tecmo",
	"Nicola Salmoria\nErnesto Corvi (ADPCM sound)",
	0,
	&rygar_machine_driver,

	rygar_rom,
	0, 0,
	0,
	(void *)rygar_samples,	/* sound_prom */

	rygar_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	rygar_hiload, rygar_hisave
};

struct GameDriver rygarj_driver =
{
	__FILE__,
	&rygar_driver,
	"rygarj",
	"Rygar (Japan)",
	"1986",
	"Tecmo",
	"Nicola Salmoria\nErnesto Corvi (ADPCM sound)",
	0,
	&rygar_machine_driver,

	rygarj_rom,
	0, 0,
	0,
	(void *)rygar_samples,	/* sound_prom */

	rygar_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	rygar_hiload, rygar_hisave
};

struct GameDriver gemini_driver =
{
	__FILE__,
	0,
	"gemini",
	"Gemini Wing",
	"1987",
	"Tecmo",
    "Nicola Salmoria (MAME driver)\nMirko Buffoni (additional code)\nMartin Binder (dip switches)",
	0,
	&gemini_machine_driver,

	gemini_rom,
	0, 0,
	0,
	(void *)silkworm_samples,	/* sound_prom */

	gemini_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	gemini_hiload, gemini_hisave
};

struct GameDriver silkworm_driver =
{
	__FILE__,
	0,
	"silkworm",
	"Silkworm",
	"1988",
	"Tecmo",
	"Nicola Salmoria",
	0,
	&silkworm_machine_driver,

	silkworm_rom,
	0, 0,
	0,
	(void *)silkworm_samples,	/* sound_prom */

	silkworm_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	silkworm_hiload, silkworm_hisave
};
