/***************************************************************************
  Rainbow Islands

  A lot of this is based on Rastan driver, it may be re-joined with
  that at a later date.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *rastan_spriteram;
extern unsigned char *rastan_scrollx;
extern unsigned char *rastan_scrolly;

void rastan_updatehook0(int offset);
void rastan_updatehook1(int offset);

void rastan_spriteram_w(int offset,int data);
int  rastan_spriteram_r(int offset);

void rastan_scrollY_w(int offset,int data);
void rastan_scrollX_w(int offset,int data);

void rastan_videocontrol_w(int offset,int data);

int rastan_interrupt(void);
int rastan_s_interrupt(void);

int rastan_input_r (int offset);

void rastan_sound_w(int offset,int data);
int rastan_sound_r(int offset);

void rastan_speedup_w(int offset,int data);
int rastan_speedup_r(int offset);


int r_rd_a000(int offset);
int r_rd_a001(int offset);
void r_wr_a000(int offset,int data);
void r_wr_a001(int offset,int data);

int  rastan_sh_init(const char *gamename);
void r_wr_b000(int offset,int data);
void r_wr_c000(int offset,int data);
void r_wr_d000(int offset,int data);

/* Rainbow Extras */

int  rainbow_interrupt(void);
void rainbow_c_chip_w(int offset, int data);
int  rainbow_c_chip_r(int offset);
void rainbow_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static struct MemoryReadAddress rainbow_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x10c000, 0x10ffff, MRA_BANK1 },	/* RAM */
	{ 0x200000, 0x20ffff, paletteram_word_r },
    { 0x390000, 0x390003, input_port_0_r },
    { 0x3B0000, 0x3B0003, input_port_1_r },
	{ 0x3e0000, 0x3e0003, rastan_sound_r },
    { 0x800000, 0x80ffff, rainbow_c_chip_r },
	{ 0xc00000, 0xc03fff, videoram10_word_r },
	{ 0xc04000, 0xc07fff, MRA_BANK2 },
	{ 0xc08000, 0xc0bfff, videoram00_word_r },
	{ 0xc0c000, 0xc0ffff, MRA_BANK3 },
	{ 0xd00000, 0xd0ffff, MRA_BANK4, &rastan_spriteram },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rainbow_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x10c000, 0x10ffff, MWA_BANK1 },
	{ 0x200000, 0x20ffff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
    { 0x800000, 0x80ffff, rainbow_c_chip_w },
	{ 0xc00000, 0xc03fff, videoram10_word_w, &videoram10 },
	{ 0xc04000, 0xc07fff, MWA_BANK2 },
	{ 0xc08000, 0xc0bfff, videoram00_word_w, &videoram00 },
	{ 0xc0c000, 0xc0ffff, MWA_BANK3 },
	{ 0xc20000, 0xc20003, rastan_scrollY_w, &rastan_scrolly },  /* scroll Y  1st.w plane1  2nd.w plane2 */
	{ 0xc40000, 0xc40003, rastan_scrollX_w, &rastan_scrollx },  /* scroll X  1st.w plane1  2nd.w plane2 */
	{ 0xd00000, 0xd0ffff, MWA_BANK4 },
	{ 0x380000, 0x380003, rastan_videocontrol_w },	/* sprite palette bank, coin counters, other unknowns */
	{ 0x3e0000, 0x3e0003, rastan_sound_w },
	{ 0x3c0000, 0x3c0003, MWA_NOP },
	{ -1 }  /* end of table */
};

#if 0
static struct MemoryReadAddress rastan_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_read_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa000, 0xa000, r_rd_a000 },
	{ 0xa001, 0xa001, r_rd_a001 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rastan_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_control_port_0_w },
	{ 0x9001, 0x9001, YM2151_write_port_0_w },
	{ 0xa000, 0xa000, r_wr_a000 },
	{ 0xa001, 0xa001, r_wr_a001 },
	{ 0xb000, 0xb000, r_wr_b000 },
	{ 0xc000, 0xc000, r_wr_c000 },
	{ 0xd000, 0xd000, r_wr_d000 },
	{ -1 }  /* end of table */
};
#endif



INPUT_PORTS_START( rainbow_input_ports )
	PORT_START	/* DIP SWITCH A */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )

	PORT_START	/* DIP SWITCH B */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "100k,1000k" )
	PORT_DIPSETTING(    0x04, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Complete Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Up" )
	PORT_DIPSETTING(    0x08, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, "Coin Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Type 1" )
	PORT_DIPSETTING(    0x80, "Type 2" )

	PORT_START	/* 800007 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )

    PORT_START /* 800009 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* 80000B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* 80000d */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout spritelayout2 =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8, 12, 0, 4, 24, 28, 16, 20, 40, 44, 32, 36, 56, 60, 48, 52 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout spritelayout3 =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
	0, 4, 0x10000*8+0 ,0x10000*8+4,
	8+0, 8+4, 0x10000*8+8+0, 0x10000*8+8+4,
	16+0, 16+4, 0x10000*8+16+0, 0x10000*8+16+4,
	24+0, 24+4, 0x10000*8+24+0, 0x10000*8+24+4
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo rainbowe_gfxdecodeinfo[] =
{
	{ 1, 0x080000, &spritelayout2, 0, 0x80 },	/* sprites 16x16 */
	{ 1, 0x100000, &spritelayout3, 0, 0x80 },/* sprites 16x16 (What For ?) */
	{ -1 } 										/* end of array */
};


static struct GfxTileLayout tilelayout =
{
	16384,	/* 16384 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { 8, 12, 0, 4, 24, 28, 16, 20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxTileDecodeInfo gfxtiledecodeinfo0[] =
{
	{ 1, 0x00000, &tilelayout, 0, 128, 0x00000001 },	/* foreground tiles */
	{ -1 } /* end of array */
};

static struct GfxTileDecodeInfo gfxtiledecodeinfo1[] =
{
	{ 1, 0x00000, &tilelayout, 0, 128, 0 },	/* background tiles */
	{ -1 } /* end of array */
};


static struct MachineLayer machine_layers[MAX_LAYERS] =
{
	{
		LAYER_TILE,
		64*8,64*8,
		gfxtiledecodeinfo0,
		0,
		rastan_updatehook0,0,0,0
	},
	{
		LAYER_TILE,
		64*8,64*8,
		gfxtiledecodeinfo1,
		0,
		rastan_updatehook1,0,0,0
	}
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0,
			rainbow_readmem,rainbow_writemem,0,0,
			rainbow_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slices per frame - no sound CPU yet! */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },
	rainbowe_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	machine_layers,
	generic_vh_start,
	generic_vh_stop,
	rainbow_vh_screenrefresh,

	/* sound hardware */
    0,
    0,
    0,
    0
};



ROM_START( rainbow_rom )
	ROM_REGION(0x80000)		/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "b22-10",     0x00000, 0x10000, 0x4dbf4585 )
	ROM_LOAD_ODD ( "b22-11",     0x00000, 0x10000, 0xcd9b9d19 )
	ROM_LOAD_EVEN( "b22-08",     0x20000, 0x10000, 0x9969d64d )
	ROM_LOAD_ODD ( "b22-09",     0x20000, 0x10000, 0xe7687f18 )
	ROM_LOAD_EVEN( "ri_m03.rom", 0x40000, 0x20000, 0x5c4aded4 )
	ROM_LOAD_ODD ( "ri_m04.rom", 0x40000, 0x20000, 0x8b4e3d12 )

	ROM_REGION(0x120000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ri_m01.rom", 0x000000, 0x80000, 0x1f58a3d8 )        /* 8x8 gfx */
  	ROM_LOAD( "ri_m02.rom", 0x080000, 0x80000, 0x38c9c265 )        /* sprites */
	ROM_LOAD( "b22-13",     0x100000, 0x10000, 0x852d58a3 )
	ROM_LOAD( "b22-12",     0x110000, 0x10000, 0x086dcc57 )

#if 0
	ROM_REGION(0x10000)		/* 64k for the audio CPU */
	ROM_LOAD( "b22-14", 0x0000, 0x10000, 0x0 )
#endif
ROM_END

ROM_START( rainbowe_rom )
	ROM_REGION(0x80000)		/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "ri_01.rom",  0x00000, 0x10000, 0xbd67d825 )
	ROM_LOAD_ODD ( "ri_02.rom",  0x00000, 0x10000, 0xbe32da3e )
	ROM_LOAD_EVEN( "ri_03.rom",  0x20000, 0x10000, 0x724f374b )
	ROM_LOAD_ODD ( "ri_04.rom",  0x20000, 0x10000, 0x33f3af7b )
	ROM_LOAD_EVEN( "ri_m03.rom", 0x40000, 0x20000, 0x5c4aded4 )
	ROM_LOAD_ODD ( "ri_m04.rom", 0x40000, 0x20000, 0x8b4e3d12 )

	ROM_REGION(0x120000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ri_m01.rom", 0x000000, 0x80000, 0x1f58a3d8 )        /* 8x8 gfx */
  	ROM_LOAD( "ri_m02.rom", 0x080000, 0x80000, 0x38c9c265 )        /* sprites */
	ROM_LOAD( "b22-13",     0x100000, 0x10000, 0x852d58a3 )
	ROM_LOAD( "b22-12",     0x110000, 0x10000, 0x086dcc57 )

#if 0
	ROM_REGION(0x10000)		/* 64k for the audio CPU */
	ROM_LOAD( "b22-14", 0x0000, 0x10000, 0x0 )
#endif
ROM_END



struct GameDriver rainbow_driver =
{
	__FILE__,
	0,
	"rainbow",
	"Rainbow Islands",
	"1987",
	"Taito",
	"Richard Bush (Raine & Info)\nMike Coates (MAME driver)",
	GAME_NOT_WORKING,
	&machine_driver,

	rainbow_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rainbow_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver rainbowe_driver =
{
	__FILE__,
	&rainbow_driver,
	"rainbowe",
	"Rainbow Islands (Extra)",
	"1988",
	"Taito",
	"Richard Bush (Raine & Info)\nMike Coates (MAME driver)",
	GAME_NOT_WORKING,
	&machine_driver,

	rainbowe_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rainbow_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
