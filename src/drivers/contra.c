/***************************************************************************

Contra/Gryzor (c) 1987 Konami

Notes:
    Press 1P and 2P together to advance through tests.

Issues:
    There appears to be at least one incorrectly mapped sprite late on the
    final stage.

	The weapon-carrying enemies in the "into the screen" stages are supposed
	to be colored differently (orange).
	A control bit in sprite RAM exists which seems to serve this purpose,
	indicating an alternate palette.

    sprite horizontal position is sometimes off by 8.

Credits:
    Carlos A. Lozano: CPU emulation
    Phil Stroffolino: video driver
    Jose Tejada Gomez (of Grytra fame) for precious information on sprites
    Eric Hustvedt: palette optimizations and cocktail support (in-progress)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6809/m6809.h"

extern unsigned char *contra_fg_horizontal_scroll;
extern unsigned char *contra_fg_vertical_scroll;
extern unsigned char *contra_bg_horizontal_scroll;
extern unsigned char *contra_bg_vertical_scroll;
extern unsigned char *contra_fg_vram,*contra_fg_cram;
extern int contra_fg_vram_size;
extern unsigned char *contra_text_vram,*contra_text_cram;
extern int contra_text_vram_size;
extern unsigned char *contra_bg_vram,*contra_bg_cram;
extern int contra_bg_vram_size;

void contra_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void contra_paletteram_w(int offset,int data);
void contra_fg_vram_w(int offset,int data);
void contra_fg_cram_w(int offset,int data);
void contra_bg_vram_w(int offset,int data);
void contra_bg_cram_w(int offset,int data);
void contra_0007_w(int offset,int data);
void contra_fg_palette_bank_w(int offset,int data);
void contra_bg_palette_bank_w(int offset,int data);
void contra_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int contra_vh_start(void);
void contra_vh_stop(void);



void contra_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}


void contra_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x0f) * 0x2000;
	if (bankaddress < 0x28000)	/* for safety */
		cpu_setbank(1,&RAM[bankaddress]);
}

void contra_sh_irqtrigger_w(int offset, int data)
{
	cpu_cause_interrupt(1,M6809_INT_IRQ);
}

void contra_coin_counter_w(int offset, int data)
{
	if (data & 0x01) coin_counter_w(0,data & 0x01);
	if (data & 0x02) coin_counter_w(1,(data & 0x02) >> 1);
}

static void cpu_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0010, 0x0010, input_port_0_r },		/* IN0 */
	{ 0x0011, 0x0011, input_port_1_r },		/* IN1 */
	{ 0x0012, 0x0012, input_port_2_r },		/* IN2 */

	{ 0x0014, 0x0014, input_port_3_r },		/* DIPSW1 */
	{ 0x0015, 0x0015, input_port_4_r },		/* DIPSW2 */
	{ 0x0016, 0x0016, input_port_5_r },		/* DIPSW3 */

	{ 0x0c00, 0x0cff, MRA_RAM },
	{ 0x1000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0000, MWA_RAM, &contra_fg_vertical_scroll },
	{ 0x0002, 0x0002, MWA_RAM, &contra_fg_horizontal_scroll },
	{ 0x0006, 0x0006, contra_fg_palette_bank_w },
	{ 0x0007, 0x0007, contra_0007_w },
	{ 0x0018, 0x0018, contra_coin_counter_w },
	{ 0x001a, 0x001a, contra_sh_irqtrigger_w },
	{ 0x001c, 0x001c, cpu_sound_command_w },
	{ 0x001e, 0x001e, MWA_NOP },	/* ??? */
	{ 0x0060, 0x0060, MWA_RAM, &contra_bg_vertical_scroll },
	{ 0x0062, 0x0062, MWA_RAM, &contra_bg_horizontal_scroll },
	{ 0x0066, 0x0066, contra_bg_palette_bank_w },
	{ 0x0c00, 0x0cff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0x1000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x23ff, contra_fg_cram_w, &contra_fg_cram },
	{ 0x2400, 0x27ff, contra_fg_vram_w, &contra_fg_vram, &contra_fg_vram_size },
	{ 0x2800, 0x2bff, MWA_RAM, &contra_text_cram },
	{ 0x2c00, 0x2fff, MWA_RAM, &contra_text_vram, &contra_text_vram_size },
	{ 0x3000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3fff, MWA_RAM, &spriteram }, /* more sprites at 0x5800 */
	{ 0x4000, 0x43ff, contra_bg_cram_w, &contra_bg_cram },
	{ 0x4400, 0x47ff, contra_bg_vram_w, &contra_bg_vram, &contra_bg_vram_size },
	{ 0x4800, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_ROM },
 	{ 0x7000, 0x7000, contra_bankswitch_w },
	{ 0x7001, 0xffff, MWA_ROM },
	{ -1 }
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x0000, soundlatch_r },
	{ 0x2001, 0x2001, YM2151_status_port_0_r },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x4000, 0x4000, MWA_NOP }, /* read triggers irq reset and latch read (in the hardware only). */
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	/* 0x00 is invalid */

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 80000" )
	PORT_DIPSETTING(    0x08, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Mono" )
	PORT_DIPSETTING(    0x08, "Stereo" )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	8,8,
	8192,
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	2048,
	4,
	{ 0,1,2,3 },
	{
		0, 4, 8, 12, 16, 20, 24, 28,
		8*32+0, 8*32+4, 8*32+8, 8*32+12, 8*32+16, 8*32+20, 8*32+24, 8*32+28,
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		32*16+0*32, 32*16+1*32, 32*16+2*32, 32*16+3*32,
		32*16+4*32, 32*16+5*32, 32*16+6*32, 32*16+7*32
	},
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout,          128, 8*4 },
	{ 1, 0x40000, &tilelayout,   128+16*8*4, 8*4 },
	{ 1, 0x80000, &spritelayout,          0, 8 },
	{ 1, 0xC0000, &spritelayout,          0, 8 },
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3582071,	/* seems to be the standard */
	{ 255 },
	{ 0 }
};



static struct MachineDriver contra_machine_driver =
{
	{
		{
 			CPU_M6809,
			1500000,
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			2000000,
			3,
			readmem_sound,writemem_sound,0,0,
			ignore_interrupt,0	/* IRQs are caused by the main CPU */
		},
	},
	60,DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	contra_init_machine, /* init machine */

	/* video hardware */

	37*8, 32*8, { 0*8, 35*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	128, 128 + 16*8*4 + 16*8*4,
	contra_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	contra_vh_start,
	contra_vh_stop,
	contra_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};



ROM_START( contra_rom )
	ROM_REGION(0x28000)	/* 64k for code + 96k for banked ROMs */
	ROM_LOAD( "633e03.18a",   0x20000, 0x08000, 0x7fc0d8cf )
	ROM_CONTINUE(           0x08000, 0x08000 )
	ROM_LOAD( "633e02.17a",   0x10000, 0x10000, 0xb2f7bd9a )

	ROM_REGION_DISPOSE(0x10000*16) /* temporary space for graphics */
	ROM_LOAD( "g-7.rom",      0x00000, 0x10000, 0x57f467d2 )	/* foreground tiles */
	ROM_LOAD( "g-10.rom",     0x10000, 0x10000, 0xe6db9685 )
	ROM_LOAD( "g-9.rom",      0x20000, 0x10000, 0x875c61de )
	ROM_LOAD( "g-8.rom",      0x30000, 0x10000, 0x642765d6 )
	ROM_LOAD( "g-4.rom",      0x40000, 0x10000, 0x2cc7e52c )	/* background tiles */
	ROM_LOAD( "g-5.rom",      0x50000, 0x10000, 0xe01a5b9c )
	ROM_LOAD( "g-6.rom",      0x60000, 0x10000, 0xaeea6744 )
	/* 0x70000 is unpopulated */
	ROM_LOAD( "g-11.rom",     0x80000, 0x10000, 0xbd9ba92c )	/* enemy sprites */
	ROM_LOAD( "g-12.rom",     0x90000, 0x10000, 0xd0be7ec2 )
	ROM_LOAD( "g-13.rom",     0xA0000, 0x10000, 0x2b513d12 )
	ROM_LOAD( "g-14.rom",     0xB0000, 0x10000, 0xfca77c5a )
	ROM_LOAD( "g-15.rom",     0xC0000, 0x10000, 0xdaa2324b )	/* player sprites */
	ROM_LOAD( "g-16.rom",     0xD0000, 0x10000, 0xe27cc835 )
	ROM_LOAD( "g-17.rom",     0xE0000, 0x10000, 0xce4330b9 )
	ROM_LOAD( "g-18.rom",     0xF0000, 0x10000, 0x1571ce42 )

	ROM_REGION(0x0200)	/* lookup table PROMs */
	ROM_LOAD( "633e09.12g",   0x0000, 0x0100, 0x14ca5e19 )	/* fg lookup table */
	ROM_LOAD( "633e08.10g",   0x0100, 0x0100, 0x9f0949fa )	/* ? bg lookup table ? */

	ROM_REGION(0x10000)	/* 64k for SOUND code */
	ROM_LOAD( "633e01.12a",   0x08000, 0x08000, 0xd1549255 )
ROM_END

ROM_START( contrab_rom )
	ROM_REGION(0x28000)	/* 64k for code + 96k for banked ROMs */
	ROM_LOAD( "contra.20",    0x20000, 0x08000, 0xd045e1da )
	ROM_CONTINUE(           0x08000, 0x08000 )
	ROM_LOAD( "633e02.17a",   0x10000, 0x10000, 0xb2f7bd9a )

	ROM_REGION_DISPOSE(0x10000*16) /* temporary space for graphics */
	ROM_LOAD( "g-7.rom",      0x00000, 0x10000, 0x57f467d2 )	/* foreground tiles */
	ROM_LOAD( "g-10.rom",     0x10000, 0x10000, 0xe6db9685 )
	ROM_LOAD( "g-9.rom",      0x20000, 0x10000, 0x875c61de )
	ROM_LOAD( "g-8.rom",      0x30000, 0x10000, 0x642765d6 )
	ROM_LOAD( "g-4.rom",      0x40000, 0x10000, 0x2cc7e52c )	/* background tiles */
	ROM_LOAD( "g-5.rom",      0x50000, 0x10000, 0xe01a5b9c )
	ROM_LOAD( "g-6.rom",      0x60000, 0x10000, 0xaeea6744 )
	/* 0x70000 is unpopulated */
	ROM_LOAD( "g-11.rom",     0x80000, 0x10000, 0xbd9ba92c )	/* enemy sprites */
	ROM_LOAD( "g-12.rom",     0x90000, 0x10000, 0xd0be7ec2 )
	ROM_LOAD( "g-13.rom",     0xA0000, 0x10000, 0x2b513d12 )
	ROM_LOAD( "g-14.rom",     0xB0000, 0x10000, 0xfca77c5a )
	ROM_LOAD( "g-15.rom",     0xC0000, 0x10000, 0xdaa2324b )	/* player sprites */
	ROM_LOAD( "g-16.rom",     0xD0000, 0x10000, 0xe27cc835 )
	ROM_LOAD( "g-17.rom",     0xE0000, 0x10000, 0xce4330b9 )
	ROM_LOAD( "g-18.rom",     0xF0000, 0x10000, 0x1571ce42 )

	ROM_REGION(0x0200)	/* lookup table PROMs */
	ROM_LOAD( "633e09.12g",   0x0000, 0x0100, 0x14ca5e19 )	/* fg lookup table */
	ROM_LOAD( "633e08.10g",   0x0100, 0x0100, 0x9f0949fa )	/* ? bg lookup table ? */

	ROM_REGION(0x10000)	/* 64k for SOUND code */
	ROM_LOAD( "633e01.12a",   0x08000, 0x08000, 0xd1549255 )
ROM_END

ROM_START( gryzorb_rom )
	ROM_REGION(0x28000)	/* 64k for code + 96k for banked ROMs */
	ROM_LOAD( "g-2.rom",      0x20000, 0x08000, 0xbdb9196d )
	ROM_CONTINUE(        0x08000, 0x08000 )
	ROM_LOAD( "g-3.rom",      0x10000, 0x10000, 0x5d5f7438 )

	ROM_REGION_DISPOSE(0x10000*16) /* temporary space for graphics */
	ROM_LOAD( "g-7.rom",      0x00000, 0x10000, 0x57f467d2 )	/* foreground tiles */
	ROM_LOAD( "g-10.rom",     0x10000, 0x10000, 0xe6db9685 )
	ROM_LOAD( "g-9.rom",      0x20000, 0x10000, 0x875c61de )
	ROM_LOAD( "g-8.rom",      0x30000, 0x10000, 0x642765d6 )
	ROM_LOAD( "g-4.rom",      0x40000, 0x10000, 0x2cc7e52c )	/* background tiles */
	ROM_LOAD( "g-5.rom",      0x50000, 0x10000, 0xe01a5b9c )
	ROM_LOAD( "g-6.rom",      0x60000, 0x10000, 0xaeea6744 )
	/* 0x70000 is unpopulated */
	ROM_LOAD( "g-11.rom",     0x80000, 0x10000, 0xbd9ba92c )	/* enemy sprites */
	ROM_LOAD( "g-12.rom",     0x90000, 0x10000, 0xd0be7ec2 )
	ROM_LOAD( "g-13.rom",     0xA0000, 0x10000, 0x2b513d12 )
	ROM_LOAD( "g-14.rom",     0xB0000, 0x10000, 0xfca77c5a )
	ROM_LOAD( "g-15.rom",     0xC0000, 0x10000, 0xdaa2324b )	/* player sprites */
	ROM_LOAD( "g-16.rom",     0xD0000, 0x10000, 0xe27cc835 )
	ROM_LOAD( "g-17.rom",     0xE0000, 0x10000, 0xce4330b9 )
	ROM_LOAD( "g-18.rom",     0xF0000, 0x10000, 0x1571ce42 )

	ROM_REGION(0x0200)	/* lookup table PROMs */
	ROM_LOAD( "633e09.12g",   0x0000, 0x0100, 0x14ca5e19 )	/* fg lookup table */
	ROM_LOAD( "633e08.10g",   0x0100, 0x0100, 0x9f0949fa )	/* ? bg lookup table ? */

	ROM_REGION(0x10000)	/* 64k for SOUND code */
	ROM_LOAD( "633e01.12a",   0x08000, 0x08000, 0xd1549255 )
ROM_END


static int contra_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x1125],"\x02\x58\x00",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x1120],64);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0x1118] = RAM[0x1124];
                        RAM[0x1119] = RAM[0x1125];
                        RAM[0x111a] = RAM[0x1126];
                        RAM[0x111b] = RAM[0x1127];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void contra_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x1120],64);
		osd_fclose(f);
	}
}

struct GameDriver contra_driver =
{
	__FILE__,
	0,
	"contra",
	"Contra (US)",
	"1987",
	"Konami",
	"Carlos A. Lozano\nJose Tejada Gomez\nPhil Stroffolino\nEric Hustvedt",
	0,
	&contra_machine_driver,
	0,

	contra_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

        contra_hiload, contra_hisave
};

struct GameDriver contrab_driver =
{
	__FILE__,
	&contra_driver,
	"contrab",
	"Contra (US bootleg)",
	"1987",
	"bootleg",
	"Carlos A. Lozano\nJose Tejada Gomez\nPhil Stroffolino\nEric Hustvedt",
	0,
	&contra_machine_driver,
	0,

	contrab_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

        contra_hiload, contra_hisave
};

struct GameDriver gryzorb_driver =
{
	__FILE__,
	&contra_driver,
	"gryzorb",
	"Contra (Japan bootleg)",
	"1987",
	"bootleg",
	"Carlos A. Lozano\nJose Tejada Gomez\nPhil Stroffolino\nEric Hustvedt",
	0,
	&contra_machine_driver,
	0,

	gryzorb_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

        contra_hiload, contra_hisave
};
