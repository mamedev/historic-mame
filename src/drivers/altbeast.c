/***************************************************************************

Altered Beast Memory Map:  Preliminary

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

void system16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

unsigned char *altbeast_backgroundbank;

int altbeast_backbank_r(int offset);
int altbeast_backbank_r(int offset)
{
	int rv = READ_WORD(&altbeast_backgroundbank[offset]);
	if (offset == 0) system16_background_bank = (rv & 0xff) - 1;
	return rv;
}

static struct MemoryReadAddress altbeast_readmem[] =
{
	{ 0x410000, 0x410fff, system16_videoram_r },
	{ 0x400000, 0x40ffff, system16_backgroundram_r },
	{ 0x440000, 0x440fff, system16_spriteram_r },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xfe0000, 0xfeffff, system16_soundram_r },
	{ 0xfff094, 0xfff097, altbeast_backbank_r, &altbeast_backgroundbank },
	{ 0xff0000, 0xffffff, MRA_BANK1 },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress altbeast_writemem[] =
{
	{ 0x410e90, 0x410e9f, system16_scroll_w, &system16_scrollram },
	{ 0x410e80, 0x410e87, system16_pagesel_w, &system16_pagesram },
	{ 0x410000, 0x410fff, system16_videoram_w, &system16_videoram, &s16_videoram_size },
	{ 0x400000, 0x40ffff, system16_backgroundram_w, &system16_backgroundram, &s16_backgroundram_size },
	{ 0x440000, 0x440fff, system16_spriteram_w, &system16_spriteram, &s16_spriteram_size },
	{ 0x840000, 0x840fff, system16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &system16_refreshregister },
	{ 0xc40004, 0xc4000f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP },                 /* IO Ctrl:  Unknown */
	{ 0xfe0000, 0xfeffff, system16_soundram_w, &system16_soundram, &s16_soundram_size },
	{ 0xff0000, 0xffffff, MWA_BANK1 },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( altbeast_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

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
	PORT_DIPNAME( 0x01, 0x01, "Credits needed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue")
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x40000*8, 0x20000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,	0, 256 },
	{ -1 } /* end of array */
};

void altbeast_init_machine(void);
void altbeast_init_machine(void)
{
	/*	Initialize Objects Bank vector */

	static int bank[16] = { 0,0,2,0,4,0,6,0,8,0,10,0,12,0,0,0 };

	/*	And notify this to the System16 hardware */

	system16_define_bank_vector(bank);
	system16_define_sprxoffset(-0xb8);

}


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			altbeast_readmem,altbeast_writemem,0,0,
			system16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* Monoprocessor for now */
	altbeast_init_machine,

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
	0,
	0,
	0,
	0
};

extern void system16_sprite_decode( int num_banks, int bank_size );

static void altbeast_sprite_decode (void){
	system16_sprite_decode( 7, 0x20000 );
}


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( altbeast_rom )

	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "ab11739.bin", 0x00000, 0x20000, 0x2a86ef80 )
	ROM_LOAD_EVEN( "ab11740.bin", 0x00000, 0x20000, 0xee618577 )

	ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ab11674.bin",  0x00000, 0x20000, 0x11a5b38d )        /* 8x8 0 */
	ROM_LOAD( "ab11675.bin",  0x20000, 0x20000, 0xb4da9b5c )        /* 8x8 1 */
	ROM_LOAD( "ab11676.bin",  0x40000, 0x20000, 0xe29679fe )        /* 8x8 2 */

	ROM_REGION(0x0e0000 * 2)
	ROM_LOAD( "ab11677.bin", 0x000000, 0x10000, 0x4790062c )     /* sprites */
	ROM_LOAD( "ab11681.bin", 0x010000, 0x10000, 0x64ba3d78 )
	ROM_LOAD( "ab11726.bin", 0x020000, 0x10000, 0x92b73325 )
	ROM_LOAD( "ab11730.bin", 0x030000, 0x10000, 0x1206c29c )
	ROM_LOAD( "ab11678.bin", 0x040000, 0x10000, 0xd7e015d2 )
	ROM_LOAD( "ab11682.bin", 0x050000, 0x10000, 0x24a06d36 )
	ROM_LOAD( "ab11728.bin", 0x060000, 0x10000, 0xce35e3ed )
	ROM_LOAD( "ab11732.bin", 0x070000, 0x10000, 0xbe1f7251 )
	ROM_LOAD( "ab11679.bin", 0x080000, 0x10000, 0x047b56d5 )
	ROM_LOAD( "ab11683.bin", 0x090000, 0x10000, 0xcd0f3973 )
	ROM_LOAD( "ab11718.bin", 0x0a0000, 0x10000, 0x084baced )
	ROM_LOAD( "ab11734.bin", 0x0b0000, 0x10000, 0x60de3d94 )
	ROM_LOAD( "ab11680.bin", 0x0c0000, 0x10000, 0x8a3b1011 )
	ROM_LOAD( "ab11684.bin", 0x0d0000, 0x10000, 0x594c9fdc )

ROM_END


struct GameDriver altbeast_driver =
{
	__FILE__,
	0,
	"altbeast",
	"Altered Beast",
	"1988",
	"Sega",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&machine_driver,

	altbeast_rom,
	altbeast_sprite_decode, 0,
	0,
	0,

	altbeast_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
