/***************************************************************************

Shinobi Memory Map:  Preliminary

000000-03ffff  ROM
ff0000-ffffff  RAM
fe0000-feffff  SoundRAM
400000-40ffff  Foreground and Background RAM
410000-410fff  VideoRAM
440000-440fff  SpriteRAM
840000-840fff  PaletteRAM

READ:
c41003  Player 1 Joystick
c41007  Player 2 Joystick
c41001  General control buttons
c42003  DSW1 :  Used for coin selection
c42001  DSW2 :  Used for Dip Switch settings

WRITE:
410e98-410e99  Foreground horizontal scroll
410e9a-410e9b  Background horizontal scroll
410e91-410e92  Foreground vertical scroll
410e93-410e94  Background vertical scroll
410e81-410e82  Foreground Page selector     \   There are 16 pages, of 1000h
410e83-410e84  Background Page selector     /   bytes long, for background
												and foreground on System16


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/system16.h"
#include "sndhrdw/2151intf.h"

void system16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void system16_soundcommand_w(int offset, int data);
void system16_soundcommand_w(int offset, int data){
	switch (offset){
		case 0x2:
			soundlatch_w(0, data);
			break;
	}
}

static struct MemoryReadAddress shinobi_readmem[] =
{
	{ 0x410000, 0x410fff, system16_videoram_r },
	{ 0x400000, 0x40ffff, system16_backgroundram_r },
	{ 0x440000, 0x440fff, system16_spriteram_r },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xff0000, 0xffffff, MRA_BANK1 },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress shinobi_writemem[] =
{
	{ 0x410e90, 0x410e9f, shinobi_scroll_w },
	{ 0x410e80, 0x410e83, shinobi_pagesel_w },
	{ 0x410000, 0x410fff, system16_videoram_w, &system16_videoram, &s16_videoram_size },
	{ 0x418028, 0x41802b, tetris_bgpagesel_w },
	{ 0x418038, 0x41803b, tetris_fgpagesel_w },
	{ 0x400000, 0x40ffff, system16_backgroundram_w, &system16_backgroundram, &s16_backgroundram_size },
	{ 0x440000, 0x440fff, system16_spriteram_w, &system16_spriteram, &s16_spriteram_size },
	{ 0x840000, 0x840fff, system16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &system16_refreshregister }, /* this is valid only for aliensyndrome, but other games should not use it */
	{ 0xc40004, 0xc4000f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xfe0004, 0xfe0007, system16_soundcommand_w },
	{ 0xfff018, 0xfff018, shinobi_refreshenable_w, &system16_refreshregister },  /* this is valid for Shinobi and Tetris */
	{ 0xff0000, 0xffffff, MWA_BANK1 },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress passshot_readmem[] =
{
	{ 0x410000, 0x410fff, system16_videoram_r },
	{ 0x400000, 0x40ffff, system16_backgroundram_r },
	{ 0x440000, 0x440fff, system16_spriteram_r },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, passshot_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xff0000, 0xffffff, MRA_BANK1 },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress passshot_writemem[] =
{
	{ 0x410ff4, 0x410ff7, passshot_pagesel_w, &system16_pagesram },
	{ 0x410000, 0x410fff, system16_videoram_w, &system16_videoram, &s16_videoram_size },
	{ 0x400000, 0x40ffff, system16_backgroundram_w, &system16_backgroundram, &s16_backgroundram_size },
	{ 0x440000, 0x440fff, passshot_spriteram_w, &system16_spriteram, &s16_spriteram_size },
	{ 0x840000, 0x840fff, system16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &system16_refreshregister }, /* this is valid only for aliensyndrome, but other games should not use it */
	{ 0xc40004, 0xc4000f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xc42004, 0xc42007, system16_soundcommand_w },
	{ 0xfe0004, 0xfe0007, system16_soundcommand_w },
	{ 0xfff4bc, 0xfff4c3, passshot_scroll_w, &system16_scrollram },
	{ 0xff0000, 0xffffff, MWA_BANK1 },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};
static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};
static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
	{ -1 }	/* end of table */
};
static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, IOWP_NOP },   /* adpcm */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( shinobi_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright")
	PORT_DIPSETTING(    0x01, "Cocktail")
	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Enemy's Bullet Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
INPUT_PORTS_END

INPUT_PORTS_START( aliensyn_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sound?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Free (127?)", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "150" )
	PORT_DIPSETTING(    0x20, "140" )
	PORT_DIPSETTING(    0x10, "130" )
	PORT_DIPSETTING(    0x00, "120" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( tetrisbl_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )      /* Player Service in Alien Storm */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )      /* Player Service in Alien Storm */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright")
	PORT_DIPSETTING(    0x00, "Cocktail")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "FREE" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Bullets Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

INPUT_PORTS_END

INPUT_PORTS_START( passshot_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Game Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	8192,	/* 8192 chars */
	3,	/* 3 bits per pixel */
	{ 0x20000*8, 0x10000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,     0, 256 },
	{ -1 } /* end of array */
};

static void system16_irq_handler_mus(void)
{
	cpu_cause_interrupt (1, 0);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4096000,	/* 3.58 MHZ ? */
	{ 255 },
	{ system16_irq_handler_mus }
};


static void shinobi_init_machine(void)
{
	/*	Initialize Objects Bank vector */

	static int bank[16] = { 0,0,0,0,0,0,0,6,0,0,0,4,0,2,0,0 };

	/*	And notify this to the System16 hardware */

	system16_define_bank_vector(bank);
	system16_define_sprxoffset(-0xb8);
}


static void tetris_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/*	Initialize Objects Bank vector */

	static int bank[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	/*	And notify this to the System16 hardware */

	system16_define_bank_vector(bank);
	system16_define_sprxoffset(-0x40);

	/*	Patch the audio CPU */
	RAM = Machine->memory_region[3];
	RAM[0x020f] = 0xc7;
}

static void passshot_init_machine(void)
{
	/*	Initialize Objects Bank vector */

	static int bank[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,3 };

	/*	And notify this to the System16 hardware */

	system16_define_bank_vector(bank);
	system16_define_sprxoffset(-0x48);
}


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			shinobi_readmem,shinobi_writemem,0,0,
			system16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 3 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* Monoprocessor for now */
	shinobi_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	system16_vh_start,
	system16_vh_stop,
	system16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};


static struct MachineDriver tetris_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			shinobi_readmem,shinobi_writemem,0,0,
			system16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 4 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	tetris_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	system16_vh_start,
	system16_vh_stop,
	system16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};


static struct MachineDriver passshot_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			passshot_readmem,passshot_writemem,0,0,
			system16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 4 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	2,	/* Monoprocessor for now */
	passshot_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	system16_vh_start,
	system16_vh_stop,
	system16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};

void system16_sprite_decode( int num_banks, int bank_size );
void system16_sprite_decode( int num_banks, int bank_size ){
	unsigned char *base = Machine->memory_region[2];
	unsigned char *temp = malloc( bank_size );
	int i;

	if( !temp ) return;

	for( i = num_banks; i >0; i-- ){
		unsigned char *finish	= base + 2*bank_size*i;
		unsigned char *dest = finish - 2*bank_size;

		unsigned char *p1 = temp;
		unsigned char *p2 = temp+bank_size/2;

		unsigned char data;

		memcpy (temp, base+bank_size*(i-1), bank_size);

		do {
			data = *p2++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;

			data = *p1++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
		} while( dest<finish );
	}

	free( temp );
}

static void shinobi_sprite_decode (void){
	system16_sprite_decode( 4, 0x20000 );
}

static void tetrisbl_sprite_decode (void){
	system16_sprite_decode( 1, 0x20000 );
}


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( shinobi_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "shinobi.a1", 0x00000, 0x10000, 0xfe8b9d89 )
	ROM_LOAD_EVEN( "shinobi.a4", 0x00000, 0x10000, 0x8f1814ee )
	ROM_LOAD_ODD ( "shinobi.a2", 0x20000, 0x10000, 0xc27fc83f )
	ROM_LOAD_EVEN( "shinobi.a5", 0x20000, 0x10000, 0x24ed7c53 )

	ROM_REGION(0x30000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "shinobi.b9",   0x00000, 0x10000, 0x48f08ef6 )        /* 8x8 0 */
	ROM_LOAD( "shinobi.b10",  0x10000, 0x10000, 0x68400bb0 )        /* 8x8 1 */
	ROM_LOAD( "shinobi.b11",  0x20000, 0x10000, 0x28d8e20e )        /* 8x8 2 */

	ROM_REGION(0x80000 * 2) /* sprites */
	ROM_LOAD( "shinobi.b1", 0x00000, 0x10000, 0xcffe9a7c )
	ROM_LOAD( "shinobi.b5", 0x10000, 0x10000, 0xc08a5da8 )
	ROM_LOAD( "shinobi.b2", 0x20000, 0x10000, 0xd8d607c2 )
	ROM_LOAD( "shinobi.b6", 0x30000, 0x10000, 0x97cf47a1 )
	ROM_LOAD( "shinobi.b3", 0x40000, 0x10000, 0xed9c64e2 )
	ROM_LOAD( "shinobi.b7", 0x50000, 0x10000, 0x760836ba )
	ROM_LOAD( "shinobi.b4", 0x60000, 0x10000, 0xd8b2b390 )
	ROM_LOAD( "shinobi.b8", 0x70000, 0x10000, 0x8e3087a8 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "shinobi.a7", 0x0000, 0x8000, 0x74c6c582 )
ROM_END


ROM_START( aliensyn_rom )
	ROM_REGION(0x40000)     /* 6*32k for 68000 code */
	ROM_LOAD_ODD ( "11080.a1", 0x00000, 0x8000, 0xb9cab5ae )
	ROM_LOAD_EVEN( "11083.a4", 0x00000, 0x8000, 0x1eff7093 )
	ROM_LOAD_ODD ( "11081.a2", 0x10000, 0x8000, 0x35b95ec1 )
	ROM_LOAD_EVEN( "11084.a5", 0x10000, 0x8000, 0xaf10166e )
	ROM_LOAD_ODD ( "11082.a3", 0x20000, 0x8000, 0xe00daaa3 )
	ROM_LOAD_EVEN( "11085.a6", 0x20000, 0x8000, 0x41bce6e2 )

	ROM_REGION(0x30000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "10702.b9",   0x00000, 0x10000, 0xe7889186 )        /* 8x8 0 */
	ROM_LOAD( "10703.b10",  0x10000, 0x10000, 0x13ac0520 )        /* 8x8 1 */
	ROM_LOAD( "10704.b11",  0x20000, 0x10000, 0x93bef886 )        /* 8x8 2 */

	ROM_REGION(0x80000*2)
	ROM_LOAD( "10709.b1", 0x00000, 0x10000, 0x24e86498 )     /* sprites */
	ROM_LOAD( "10713.b5", 0x10000, 0x10000, 0x93353427 )
	ROM_LOAD( "10710.b2", 0x20000, 0x10000, 0xb3c675d6 )
	ROM_LOAD( "10714.b6", 0x30000, 0x10000, 0x77704f2e )
	ROM_LOAD( "10711.b3", 0x40000, 0x10000, 0x7636e106 )     /* sprites */
	ROM_LOAD( "10715.b7", 0x50000, 0x10000, 0x069913ad )
	ROM_LOAD( "10712.b4", 0x60000, 0x10000, 0x2eb0fade )
	ROM_LOAD( "10716.b8", 0x70000, 0x10000, 0xfe474ccd )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	/* not supported yet! */
ROM_END


ROM_START( tetrisbl_rom )
	ROM_REGION(0x40000)     /* 2*32k for 68000 code */
	ROM_LOAD_ODD ( "rom1.bin", 0x00000, 0x8000, 0xcf652215 )
	ROM_LOAD_EVEN( "rom2.bin", 0x00000, 0x8000, 0xc170a0f6 )

	ROM_REGION(0x30000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scr01.rom",  0x00000, 0x10000, 0x65864110 )        /* 8x8 0 */
	ROM_LOAD( "scr02.rom",  0x10000, 0x10000, 0x24f4f4b4 )        /* 8x8 1 */
	ROM_LOAD( "scr03.rom",  0x20000, 0x10000, 0xa19cfd40 )        /* 8x8 2 */

	ROM_REGION(0x20000*2)
	ROM_LOAD( "obj0-o.rom", 0x00000, 0x10000, 0x54463eb2 )     /* sprites */
	ROM_LOAD( "obj0-e.rom", 0x10000, 0x10000, 0x5ef58537 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "s-prog.rom", 0x0000, 0x8000, 0x614b77e5 )
ROM_END


ROM_START( passshtb_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "pass4_2p.bin", 0x00000, 0x10000, 0x9d3fc8bd )
	ROM_LOAD_EVEN( "pass3_2p.bin", 0x00000, 0x10000, 0x82700ff0 )

	ROM_REGION(0x30000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "passshot.b9",   0x00000, 0x10000, 0xd87a3c78 )        /* 8x8 0 */
	ROM_LOAD( "passshot.b10",  0x10000, 0x10000, 0xcb2fb653 )        /* 8x8 1 */
	ROM_LOAD( "passshot.b11",  0x20000, 0x10000, 0x5981f12f )        /* 8x8 2 */

	ROM_REGION(0x80000*2)
	ROM_LOAD( "passshot.b1", 0x00000, 0x10000, 0xcae4a044 )     /* sprites */
	ROM_LOAD( "passshot.b5", 0x10000, 0x10000, 0xb8bc0832 )
	ROM_LOAD( "passshot.b2", 0x20000, 0x10000, 0x8fb69228 )
	ROM_LOAD( "passshot.b6", 0x30000, 0x10000, 0xa77128e5 )
	ROM_LOAD( "passshot.b3", 0x40000, 0x10000, 0x1375d339 )     /* sprites */
	ROM_LOAD( "passshot.b7", 0x50000, 0x10000, 0x4dfcbd52 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "passshot.a7", 0x0000, 0x8000, 0x37f388e5 )
ROM_END



struct GameDriver shinobi_driver =
{
	__FILE__,
	0,
	"shinobi",
	"Shinobi",
	"1987",
	"Sega",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&machine_driver,

	shinobi_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	shinobi_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


struct GameDriver aliensyn_driver =
{
	__FILE__,
	0,
	"aliensyn",
	"Alien Syndrome",
	"1987",
	"Sega",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&machine_driver,

	aliensyn_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	aliensyn_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver tetrisbl_driver =
{
	__FILE__,
	0,
	"tetrisbl",
	"Tetris (Sega, bootleg)",
	"1988",
	"Sega",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&tetris_machine_driver,

	tetrisbl_rom,
	tetrisbl_sprite_decode, 0,
	0,
	0,

	tetrisbl_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


struct GameDriver passshtb_driver =
{
	__FILE__,
	0,
	"passshtb",
	"Passing Shot (bootleg)",
	"????",
	"bootleg",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&passshot_machine_driver,

	passshtb_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	passshot_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
//	ORIENTATION_ROTATE_270,
	0, 0
};
