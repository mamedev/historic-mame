/***************************************************************************

  Black Tiger

  Driver provided by Paul Leaman (paull@vortexcomputing.demon.co.uk)

  Please do not send anything large to this address without asking me
  first.

  Thanks to Ishmair for providing information about the screen
  layout on level 3.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"

extern unsigned char *blktiger_paletteram;
extern int blktiger_paletteram_size;
extern unsigned char *blktiger_backgroundram;
extern unsigned char *blktiger_backgroundattribram;
extern int blktiger_backgroundram_size;
extern unsigned char *blktiger_palette_bank;
extern unsigned char *blktiger_screen_layout;

void blktiger_bankswitch_w(int offset,int data);
void blktiger_paletteram_w(int offset,int data);
void blktiger_palette_bank_w(int offset,int data);
void blktiger_screen_layout_w(int offset,int data);

int blktiger_background_r(int offset);
void blktiger_background_w(int offset,int data);
void blktiger_video_control_w(int offset,int data);
void blktiger_scrollbank_w(int offset,int data);
void blktiger_scrollx_w(int offset,int data);
void blktiger_scrolly_w(int offset,int data);

int blktiger_interrupt(void);

int blktiger_vh_start(void);
void blktiger_vh_stop(void);
void blktiger_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void blktiger_vh_screenrefresh(struct osd_bitmap *bitmap);

int capcom_sh_start(void);
int capcom_sh_interrupt(void);

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0xbfff, MRA_ROM },   /* CODE */
        { 0xc000, 0xcfff, blktiger_background_r},
        { 0xc000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
        { 0xd400, 0xd7ff, colorram_w, &colorram },
        { 0xc000, 0xcfff, blktiger_background_w, &blktiger_backgroundram, &blktiger_backgroundram_size },
        { 0xfe00, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
        { 0xd800, 0xdfff, blktiger_paletteram_w, &blktiger_paletteram, &blktiger_paletteram_size},
        { 0xe000, 0xffff, MWA_RAM },
        { 0xc000, 0xcfff, MWA_RAM },
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
        { 0x00, 0x00, input_port_0_r },
        { 0x01, 0x01, input_port_1_r },
        { 0x02, 0x02, input_port_2_r },
        { 0x03, 0x03, input_port_3_r },
        { 0x04, 0x04, input_port_4_r },
        { 0x05, 0x05, input_port_5_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
        { 0x00, 0x00, sound_command_w },        /* Sound board output */
        { 0x01, 0x01, blktiger_bankswitch_w },  /* Code bank switch */
        { 0x06, 0x07, IOWP_NOP}, /* Watchdog (6) Software protection (7) */
        { 0x08, 0x09, blktiger_scrollx_w},      /* X scroll */
        { 0x0a, 0x0b, blktiger_scrolly_w},      /* Y scroll */
        { 0x0d, 0x0d, blktiger_scrollbank_w},   /* Scroll ram bank register */
        { 0x0e, 0x0e, blktiger_screen_layout_w},/* Video scrolling layout */
        { 0x04, 0x04, blktiger_video_control_w},/* Video control */

#if 0
        { 0x03, 0x03, IOWP_NOP}, /* Unknown port 3 */
        { 0x0c, 0x0c, IOWP_NOP}, /* Unknown port c */
#endif

	{ -1 }	/* end of table */
};

/*
  The sound board reads both sound chip control registers (or a value supplied
  by the main CPU. It will not output any sound unless I return 3. Must be
  some "is device ready?" code.
*/
static int ControlPort_r(int addr)
{
        return 3;
}


static struct MemoryReadAddress sound_readmem[] =
{
        { 0xc000, 0xc7ff, MRA_RAM },
        { 0xc800, 0xc800, sound_command_latch_r },
        { 0xe000, 0xe000, ControlPort_r },
        { 0xe001, 0xe001, MRA_RAM },
        { 0xe002, 0xe002, ControlPort_r },
        { 0xe003, 0xe003, MRA_RAM },
        { 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
        { 0xc000, 0xc7ff, MWA_RAM },
        { 0xe000, 0xe000, AY8910_control_port_0_w },
        { 0xe001, 0xe001, AY8910_write_port_0_w },
        { 0xe002, 0xe002, AY8910_control_port_1_w },
        { 0xe003, 0xe003, AY8910_write_port_1_w },
        { 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
        PORT_DIPNAME( 0x80, 0x80, "Service Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_DIPNAME( 0x07, 0x07, "Coin 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1")
        PORT_DIPSETTING(    0x01, "2")
        PORT_DIPSETTING(    0x02, "3")
        PORT_DIPSETTING(    0x03, "4")
        PORT_DIPSETTING(    0x04, "5")
        PORT_DIPSETTING(    0x05, "6")
        PORT_DIPSETTING(    0x06, "7")
        PORT_DIPSETTING(    0x07, "1 coin 1 credit")

        PORT_DIPNAME( 0x38, 0x38, "Coin 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1")
        PORT_DIPSETTING(    0x08, "2")
        PORT_DIPSETTING(    0x10, "3")
        PORT_DIPSETTING(    0x18, "4")
        PORT_DIPSETTING(    0x20, "5")
        PORT_DIPSETTING(    0x28, "6")
        PORT_DIPSETTING(    0x30, "7")
        PORT_DIPSETTING(    0x38, "1 coin 1 credit")

        PORT_START      /* DSW1 */
        PORT_DIPNAME( 0x80, 0x00, "Type", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Upright" )
        PORT_DIPSETTING(    0x80, "Table")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPSETTING(    0x20, "On" )
        PORT_DIPNAME( 0x1c, 0x0c, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Very difficult")
        PORT_DIPSETTING(    0x04, "Diff 2" )
        PORT_DIPSETTING(    0x08, "Diff 1" )
        PORT_DIPSETTING(    0x0c, "Normal" )
        PORT_DIPSETTING(    0x10, "Easy 1" )
        PORT_DIPSETTING(    0x14, "Easy 2" )
        PORT_DIPSETTING(    0x18, "Easy 3" )
        PORT_DIPSETTING(    0x1c, "Very easy")
        PORT_DIPNAME( 0x03, 0x03, "Players", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "7")
        PORT_DIPSETTING(    0x01, "5" )
        PORT_DIPSETTING(    0x02, "2" )
        PORT_DIPSETTING(    0x03, "3" )

        PORT_START      /* DSW2 */
        PORT_DIPNAME( 0x01, 0x01, "Screen stop", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "Off")
        PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
        2048,   /* 1024 characters */
	2,	/* 2 bits per pixel */
        { 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
        2048,   /* 2048 sprites */
	4,	/* 4 bits per pixel */
        { 0x20000*8+4, 0x20000*8+0, 4, 0 },
        { 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
	    32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /*   start    pointer       colour start   number of colours */
        { 1, 0x00000, &charlayout,      0,  32 },
        { 1, 0x10000, &spritelayout,  32*4, 16 },
        { 1, 0x50000, &spritelayout,  32*4+16*16,  16 },
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
                        readmem,writemem,readport,writeport,
                        blktiger_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
                        capcom_sh_interrupt,12
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
        32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
        256,               /* 256 colours */
        32*4+16*16+8*16,   /* Colour table length */
        blktiger_vh_convert_color_prom,

        VIDEO_TYPE_RASTER,
        0,
        blktiger_vh_start,
        blktiger_vh_stop,
        blktiger_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
        capcom_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blktiger_rom )
        ROM_REGION(0x50000)     /* 64k for code + banked ROMs images */
        ROM_LOAD( "blktiger.5e",  0x00000, 0x08000, 0xfd01f39b )  /* CODE */
        ROM_LOAD( "blktiger.6e",  0x10000, 0x10000, 0xabf76cc7 )  /* 0+1 */
        ROM_LOAD( "blktiger.8e",  0x20000, 0x10000, 0x3f25d1f7 )  /* 2+3 */
        ROM_LOAD( "blktiger.9e",  0x30000, 0x10000, 0xdfb8f0f8 )  /* 4+5 */
        ROM_LOAD( "blktiger.10e", 0x40000, 0x10000, 0x4a2a8eaa )  /* 6+7 */

        ROM_REGION(0x90000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "blktiger.2n", 0x00000, 0x8000, 0x94039dc5 )  /* characters */

        ROM_LOAD( "blktiger.5b", 0x10000, 0x10000, 0x18e0c452 ) /* tiles */
        ROM_LOAD( "blktiger.4b", 0x20000, 0x10000, 0xfa2939e9 ) /* tiles */
        ROM_LOAD( "blktiger.9b", 0x30000, 0x10000, 0x7ae80858 ) /* tiles */
        ROM_LOAD( "blktiger.8b", 0x40000, 0x10000, 0x1d500c92 ) /* tiles */

        ROM_LOAD( "blktiger.5a", 0x50000, 0x10000, 0xdc33c175 ) /* sprites */
        ROM_LOAD( "blktiger.4a", 0x60000, 0x10000, 0x51f829e4 ) /* sprites */
        ROM_LOAD( "blktiger.9a", 0x70000, 0x10000, 0x057f831b ) /* sprites */
        ROM_LOAD( "blktiger.8a", 0x80000, 0x10000, 0x03585086 ) /* sprites */

        ROM_REGION(0x10000) /* 64k for the audio CPU */
        ROM_LOAD( "blktiger.1l", 0x0000, 0x8000, 0xdc92e1f4 )
ROM_END

static void blktiger_decode(void)
{
        /*
          Patch the software protection.

          The game checks input port 7 and ends up in a tight loop if
          values do not match. The arcade machine will lock up solid.

          Here I patch the ROMS directly, nopping out the tight loops. Someone
          less lazy than myself would figure out how the device worked and
          emulate it.

          This approach obviously won't work for a different ROM set. If you
          have another ROM set then disassemble blktiger.5e and search for and
          make a note of all tight loops (JR NZ,-02). All such instructions
          should be patched.

        */

        /* Startup check */
        RAM[0x0ac4]=0; RAM[0x0ac5]=0;

        /* Sneaky little check at the end of level */
        RAM[0x5bbb]=0; RAM[0x5bbc]=0;

        /*
          They also check the CRC of the protection routines. Zap those
          tight loops as well.
        */
        RAM[0x0b5c]=0; RAM[0x0b5d]=0;
        RAM[0x5c19]=0; RAM[0x5c1a]=0;
        RAM[0x5d05]=0; RAM[0x5d06]=0;
}

struct GameDriver blktiger_driver =
{
        "Black Tiger",
        "blktiger",
        "Paul Leaman\nIshmair",
        &machine_driver,

        blktiger_rom,
        blktiger_decode,
        0,0,

        0,input_ports,0,0,0,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

