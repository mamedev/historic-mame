/***************************************************************************

Golden Axe Memory Map:  Preliminary

000000-0bffff  ROM
ff0000-ffffff  RAM
fe0000-feffff  SoundRAM
100000-10ffff  Foreground and Background RAM
110000-110fff  VideoRAM
200000-200fff  SpriteRAM
140000-140fff  PaletteRAM

READ:
c41003  Player 1 Joystick
c41007  Player 2 Joystick
c41001  General control buttons
c42003  DSW1 :  Used for coin selection
c42001  DSW2 :  Used for Dip Switch settings

MIRRORS:
0xffecd0 -> Mirror of 0xc41003
0xffecd1 -> Mirror of 0xc41007
0xffec96 -> Mirror of 0xc41001

WRITE:
110e98-110e99  Foreground horizontal scroll
110e9a-110e9b  Background horizontal scroll
110e91-110e92  Foreground vertical scroll
110e93-110e94  Background vertical scroll
110e81-110e82  Foreground Page selector     \   There are 16 pages, of 1000h
110e83-110e84  Background Page selector     /   bytes long, for background
												and foreground on System16


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/system16.h"

void system16_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void system16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static struct MemoryReadAddress goldnaxe_readmem[] =
{
	{ 0x000000, 0x0bffff, MRA_ROM },
	{ 0x100000, 0x10ffff, system16_backgroundram_r },
	{ 0x110000, 0x110fff, system16_videoram_r },
	{ 0x140000, 0x140fff, paletteram_word_r },
	{ 0x200000, 0x200fff, system16_spriteram_r },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xfe0000, 0xfeffff, system16_soundram_r },
	{ 0xffecd0, 0xffecd3, goldnaxe_mirror1_r, &goldnaxe_mirror1 },
	{ 0xffec94, 0xffec97, goldnaxe_mirror2_r, &goldnaxe_mirror2 },
	{ 0xff0000, 0xffffff, MRA_BANK1 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress goldnaxe_writemem[] =
{
	{ 0x000000, 0x0bffff, MWA_ROM },
	{ 0x100000, 0x10ffff, system16_backgroundram_w, &system16_backgroundram, &s16_backgroundram_size },
	{ 0x110e90, 0x110e9f, system16_scroll_w, &system16_scrollram },
	{ 0x110e80, 0x110e87, system16_pagesel_w, &system16_pagesram },
	{ 0x110000, 0x110fff, system16_videoram_w, &system16_videoram, &s16_videoram_size },
	{ 0x140000, 0x140fff, system16_paletteram_w, &paletteram },
	{ 0x1f0000, 0x1f0003, MWA_NOP },				/* IO Ctrl: Unknown */
	{ 0x200000, 0x200fff, system16_spriteram_w, &system16_spriteram, &s16_spriteram_size },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &system16_refreshregister },
	{ 0xc40008, 0xc4000f, MWA_NOP },				/* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP },				/* IO Ctrl:  Unknown */
	{ 0xfe0000, 0xfeffff, system16_soundram_w, &system16_soundram, &s16_soundram_size },
	{ 0xff0000, 0xffffff, MWA_BANK1 },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( goldnaxe_input_ports )
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
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1 , "Left Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_START2 , "Right Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
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
	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

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
	{ 1, 0x00000, &charlayout,     0, 256 },
	{ -1 } /* end of array */
};


void goldnaxe_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/*	Initialize Objects Bank vector */

	static int bank[16] = { 0,2,8,10,16,18,0,0,4,6,12,14,20,22,0,0 };

	/*	And notify this to the System16 hardware */

	system16_define_bank_vector(bank);
	system16_define_sprxoffset(-0xb8);

	/* Patch the code */

	WRITE_WORD (&RAM[0x003cb2], 0x4e75);
	WRITE_WORD (&RAM[0x0055bc], 0x6604);
	WRITE_WORD (&RAM[0x0055c6], 0x6604);
	WRITE_WORD (&RAM[0x0055d0], 0x6602);
}


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			goldnaxe_readmem,goldnaxe_writemem,0,0,
			system16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* Monoprocessor for now */
	goldnaxe_init_machine,

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

static void goldnaxe_sprite_decode (void){
	system16_sprite_decode( 3, 0x80000 );
}

/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( goldnaxe_rom )

	/*	This game eats up 3Mb!!!
	 *  "You know I'm FAT, I'm FAT,... you know it... you know ;)"
	 */

	ROM_REGION(0xc0000)	/* 12*64k for 68000 code */
	ROM_LOAD_ODD ( "ga12522.bin", 0x00000, 0x20000, 0xbc78cf80 )
	ROM_LOAD_EVEN( "ga12523.bin", 0x00000, 0x20000, 0xc4cd57c9 )
	ROM_LOAD_ODD ( "ga12544.bin", 0x40000, 0x40000, 0x0b92254c )
	ROM_LOAD_EVEN( "ga12545.bin", 0x40000, 0x40000, 0x83bcc58e )

	ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ga12385.bin",  0x00000, 0x20000, 0xd774ab10 )        /* 8x8 0 */
	ROM_LOAD( "ga12386.bin",  0x20000, 0x20000, 0x08e5b5e3 )        /* 8x8 1 */
	ROM_LOAD( "ga12387.bin",  0x40000, 0x20000, 0x49afcab3 )        /* 8x8 2 */

	ROM_REGION(0x180000*2)
	ROM_LOAD( "ga12378.bin", 0x000000, 0x40000, 0x3bbf4177 )     /* sprites */
	ROM_LOAD( "ga12379.bin", 0x040000, 0x40000, 0x06616abf )
	ROM_LOAD( "ga12380.bin", 0x080000, 0x40000, 0x92c8cf4c )
	ROM_LOAD( "ga12381.bin", 0x0c0000, 0x40000, 0x9c0347cb )
	ROM_LOAD( "ga12382.bin", 0x100000, 0x40000, 0xb0ca8be0 )     /* sprites */
	ROM_LOAD( "ga12383.bin", 0x140000, 0x40000, 0x4db9deb3 )

ROM_END


struct GameDriver goldnaxe_driver =
{
	__FILE__,
	0,
	"goldnaxe",
	"Golden Axe",
	"1989",
	"Sega",
	"Mirko Buffoni         (Mame Driver)\nThierry Lescot & Nao  (Hardware Info)",
	0,
	&machine_driver,

	goldnaxe_rom,
	goldnaxe_sprite_decode, 0,
	0,
	0,

	goldnaxe_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
