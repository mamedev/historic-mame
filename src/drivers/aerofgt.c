/***************************************************************************

Notes:
- The plane in the attract mode animation is missing one wing. Actually it's
  there, but it's misplaced.
- The ship explosion in the attract mode animation is jagged at the four corners.
  This doesn't happen in the original.
- Both of the above might be caused by some hardware zoom support.
- There are graphics for different tiles (Sonic Wings, The Final War) but I
  haven't found a way to display them - different program, maybe.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern unsigned char *aerofgt_rasterram;
extern unsigned char *aerofgt_bg1videoram,*aerofgt_bg2videoram;
extern int aerofgt_bg1videoram_size,aerofgt_bg2videoram_size;

int aerofgt_bg1videoram_r(int offset);
int aerofgt_bg2videoram_r(int offset);
void aerofgt_bg1videoram_w(int offset,int data);
void aerofgt_bg2videoram_w(int offset,int data);
void aerofgt_gfxbank_w(int offset,int data);
void aerofgt_bg1scrolly_w(int offset,int data);
void aerofgt_bg2scrolly_w(int offset,int data);
int aerofgt_vh_start(void);
void aerofgt_vh_stop(void);
void aerofgt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int pending_command;

static void sound_command_w(int offset,int data)
{
	pending_command = 1;
	soundlatch_w(offset,data & 0xff);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

static int pending_command_r(int offset)
{
	return pending_command;
}

static void pending_command_clear_w(int offset,int data)
{
	pending_command = 0;
}

static void aerofgt_sh_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;


	bankaddress = 0x10000 + (data & 0x03) * 0x8000;
	cpu_setbank(1,&RAM[bankaddress]);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x1a0000, 0x1a07ff, paletteram_word_r },
	{ 0x1b0000, 0x1b07ff, MRA_BANK2 },
	{ 0x1b0800, 0x1b0801, MRA_NOP },	/* ??? */
	{ 0x1b0ff0, 0x1b0fff, MRA_BANK7 },	/* stack area during boot */
	{ 0x1b2000, 0x1b3fff, aerofgt_bg1videoram_r },
	{ 0x1b4000, 0x1b5fff, aerofgt_bg2videoram_r },
	{ 0x1c0000, 0x1c7fff, MRA_BANK4 },
	{ 0x1d0000, 0x1d1fff, MRA_BANK5 },
	{ 0xfef000, 0xffefff, MRA_BANK6 },
	{ 0xffffa0, 0xffffa1, input_port_0_r },
	{ 0xffffa2, 0xffffa3, input_port_1_r },
	{ 0xffffa4, 0xffffa5, input_port_2_r },
	{ 0xffffa6, 0xffffa7, input_port_3_r },
	{ 0xffffa8, 0xffffa9, input_port_4_r },
	{ 0xffffac, 0xffffad, pending_command_r },
	{ 0xffffae, 0xffffaf, input_port_5_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x1a0000, 0x1a07ff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0x1b0000, 0x1b07ff, MWA_BANK2, &aerofgt_rasterram },	/* used only for the scroll registers */
	{ 0x1b0800, 0x1b0801, MWA_NOP },	/* ??? */
	{ 0x1b0ff0, 0x1b0fff, MWA_BANK7 },	/* stack area during boot */
	{ 0x1b2000, 0x1b3fff, aerofgt_bg1videoram_w, &aerofgt_bg1videoram, &aerofgt_bg1videoram_size },
	{ 0x1b4000, 0x1b5fff, aerofgt_bg2videoram_w, &aerofgt_bg2videoram, &aerofgt_bg2videoram_size },
	{ 0x1c0000, 0x1c7fff, MWA_BANK4, &spriteram },
	{ 0x1d0000, 0x1d1fff, MWA_BANK5, &spriteram_2 },
	{ 0xfef000, 0xffefff, MWA_BANK6 },	/* work RAM */
	{ 0xffff80, 0xffff87, aerofgt_gfxbank_w },
	{ 0xffff88, 0xffff89, aerofgt_bg1scrolly_w },	/* + something else in the top byte */
	{ 0xffff90, 0xffff91, aerofgt_bg2scrolly_w },	/* + something else in the top byte */
	{ 0xffffac, 0xffffad, MWA_NOP },	/* ??? */
	{ 0xffffc0, 0xffffc1, sound_command_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM2610_status_port_0_A_r },
	{ 0x02, 0x02, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x04, 0x04, aerofgt_sh_bankswitch_w },
	{ 0x08, 0x08, pending_command_clear_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Slot", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0001, "Same" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x000e, 0x000e, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(      0x000a, "3 Coins/1 Credit" )
	PORT_DIPSETTING(      0x000c, "2 Coins/1 Credit" )
	PORT_DIPSETTING(      0x000e, "1 Coin/1 Credit" )
	PORT_DIPSETTING(      0x0008, "1 Coin/2 Credits" )
	PORT_DIPSETTING(      0x0006, "1 Coin/3 Credits" )
	PORT_DIPSETTING(      0x0004, "1 Coin/4 Credits" )
	PORT_DIPSETTING(      0x0002, "1 Coin/5 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x0070, 0x0070, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0050, "3 Coins/1 Credit" )
	PORT_DIPSETTING(      0x0060, "2 Coins/1 Credit" )
	PORT_DIPSETTING(      0x0070, "1 Coin/1 Credit" )
	PORT_DIPSETTING(      0x0040, "1 Coin/2 Credits" )
	PORT_DIPSETTING(      0x0030, "1 Coin/3 Credits" )
	PORT_DIPSETTING(      0x0020, "1 Coin/4 Credits" )
	PORT_DIPSETTING(      0x0010, "1 Coin/5 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Coins to Start, 1 to Continue", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0080, "Off" )
	PORT_DIPSETTING(      0x0000, "On" )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0001, "Off" )
	PORT_DIPSETTING(      0x0000, "On" )
	PORT_DIPNAME( 0x0002, 0x0000, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0002, "Off" )
	PORT_DIPSETTING(      0x0000, "On" )
	PORT_DIPNAME( 0x000c, 0x000c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0040, "200000" )
	PORT_DIPSETTING(      0x0000, "300000" )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(      0x0080, "Off" )
	PORT_DIPSETTING(      0x0000, "On" )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x0000, "Country", IP_KEY_NONE )
	PORT_DIPSETTING(      0x0000, "Any" )
	PORT_DIPSETTING(      0x000f, "USA" )
	PORT_DIPSETTING(      0x000e, "Korea" )
	PORT_DIPSETTING(      0x000d, "Hong Kong" )
	PORT_DIPSETTING(      0x000b, "Taiwan" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	32768,	/* 32768 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
			10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
			10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,      0, 32 },
	{ 1, 0x100000, &spritelayout1, 512, 16 },
	{ 1, 0x200000, &spritelayout2, 768, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ 4 },
	{ 3 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz ??? (slows down a lot at 8MHz) */
			0,
			readmem,writemem,0,0,
			m68_level1_irq,1	/* all inrq vectors are the same */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2610 */
		}
	},
	60, 500,	/* frames per second, vblank duration */
				/* wrong but improves sprite-background synchronization */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	aerofgt_vh_start,
	aerofgt_vh_stop,
	aerofgt_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface,
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( aerofgt_rom )
	ROM_REGION(0x80000)	/* 68000 code */
	ROM_LOAD_WIDE_SWAP( "1.u4",         0x00000, 0x80000, 0x6fdff0a2 )

	ROM_REGION_DISPOSE(0x280000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "538a54.124",   0x000000, 0x080000, 0x4d2c4df2 )
	ROM_LOAD( "1538a54.124",  0x080000, 0x080000, 0x286d109e )
	ROM_LOAD( "538a53.u9",    0x100000, 0x100000, 0x630d8e0b )
	ROM_LOAD( "534g8f.u18",   0x200000, 0x080000, 0x76ce0926 )

	ROM_REGION(0x30000)	/* 64k for the audio CPU + banks */
	ROM_LOAD( "2.153",        0x00000, 0x20000, 0xa1ef64ec )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION(0x100000) /* sound samples */
	ROM_LOAD( "it1906.137",   0x000000, 0x80000, 0x748b7e9e )
	ROM_LOAD( "1ti1906.137",  0x080000, 0x80000, 0xd39ced76 )

	ROM_REGION(0x40000) /* sound samples */
	ROM_LOAD( "it1901.104",   0x00000, 0x40000, 0x6d42723d )
ROM_END



struct GameDriver aerofgt_driver =
{
	__FILE__,
	0,
	"aerofgt",
	"Aero Fighters",
	"1992",
	"Video System Co.",
	"Nicola Salmoria",
	0,
	&machine_driver,
	0,

	aerofgt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
