/***************************************************************************

Bobble Bobble memory map (preliminary)

CPU #1
0000-bfff ROM (8000-bfff is banked)
CPU #2
0000-7fff ROM

CPU #1 AND #2
c000-dcff Graphic RAM. This contains pointers to the video RAM columns and
          to the sprites are contained in Object RAM.
dd00-dfff Object RAM (groups of four bytes: X position, code [offset in the
          Graphic RAM], Y position, gfx bank)
e000-f7fe RAM
f800-f9ff Palette RAM
fc01-fdff RAM

read:
ff00      DSWA
ff01      DSWB
ff02      IN0
ff03      IN1

write:
fa80      watchdog reset
fc00      interrupt vector? (not needed by Bobble Bobble)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/2203intf.h"

/* prototypes for functions in ../machine/bublbobl.c */
extern unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;
int bublbobl_interrupt(void);
int bublbobl_sharedram1_r(int offset);
int bublbobl_sharedram2_r(int offset);
void boblbobl_patch(void);
void bublbobl_patch(void);
void bublbobl_play_sound(int offset, int data);
void bublbobl_sharedram1_w(int offset, int data);
void bublbobl_sharedram2_w(int offset, int data);
void bublbobl_bankswitch_w(int offset, int data);

/* prototypes for functions in ../vidhrdw/bublbobl.c */
extern unsigned char *bublbobl_objectram;
extern unsigned char *bublbobl_paletteram;
extern int bublbobl_objectram_size;
void bublbobl_videoram_w(int offset,int data);
void bublbobl_objectram_w(int offset,int data);
void bublbobl_paletteram_w(int offset,int data);
void bublbobl_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int bublbobl_vh_start(void);
void bublbobl_vh_stop(void);
void bublbobl_vh_screenrefresh(struct osd_bitmap *bitmap);

void bublbobl_sound_command_w(int offset,int data);
int bublbobl_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0xbfff, MRA_BANK1 },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_r },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_r },
    { 0xff00, 0xff00, input_port_0_r },
    { 0xff01, 0xff01, input_port_1_r },
    { 0xff02, 0xff02, input_port_2_r },
    { 0xff03, 0xff03, input_port_3_r },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, bublbobl_videoram_w, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, bublbobl_objectram_w, &bublbobl_objectram, &bublbobl_objectram_size },	/* handled by bublbobl_sharedram_w() */
	{ 0xf800, 0xf9ff, bublbobl_paletteram_w, &bublbobl_paletteram },
	{ 0xc000, 0xf9ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xfa00, 0xfa00, bublbobl_sound_command_w },
	{ 0xfa80, 0xfa80, MWA_NOP },
	{ 0xfb40, 0xfb40, bublbobl_bankswitch_w },
	{ 0xfc01, 0xfdff, bublbobl_sharedram2_w, &bublbobl_sharedram2 },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_lvl[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_r },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_lvl[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xcd00, 0xd4ff, bublbobl_videoram_w },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_w },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_w },
	{ -1 }  /* end of table */
};



static int pip(int offset)
{
//if (errorlog) fprintf(errorlog,"%04x read pip\n",cpu_getpc());
	return 0x80;
}

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, pip },
	{ 0xb000, 0xb000, soundlatch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* sound chip? */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa001, MWA_NOP },	/* sound chip? */
	{ 0xb000, 0xb001, MWA_NOP },	/* sound chip? */
	{ 0xb002, 0xb002, MWA_NOP },	/* interrupt enable/acknowledge? */
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( bublbobl_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Spare", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x40, "B" )
	PORT_DIPSETTING(    0x80, "C" )
	PORT_DIPSETTING(    0xc0, "D" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( boblbobl_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPNAME( 0x02, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0xc0, "Very High" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sboblbob_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Super Bobble Bobble" )
	PORT_DIPSETTING(    0x01, "Bobble Bobble" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0xc0, "Very High" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* the characters are 8x8 pixels */
	256*8*6,			/* 256 characters per bank,
						* 8 banks per ROM pair,
						* 6 ROM pairs */
	4,	/* 4 bits per pixel */
	{ 0, 4, 6*0x8000*8, 6*0x8000*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 bytes in two ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* read all graphics into one big graphics region */
	{ 1, 0x00000, &charlayout, 0, 16 },
	{ -1 }	/* end of array */
};



static struct MachineDriver bublbobl_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			bublbobl_interrupt,1
		},
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			2,			/* memory_region */
			readmem_lvl,writemem_lvl,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1	/* NMIs are triggered by the main CPU; I don't know */
						/* whether the IRQs should be triggered by the 2203 */
		}
    },
    60,				/* frames_per_second */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    0,		/* init_machine() */

    /* video hardware */
    32*8, 32*8,			/* screen_width, height */
    { 0, 32*8-1, 2*8, 30*8-1 }, /* visible_area */
    gfxdecodeinfo,
    256, /* total_colours */
    16*16,		/* color_table_len */
    bublbobl_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	bublbobl_vh_start,
	bublbobl_vh_stop,
	bublbobl_vh_screenrefresh,

	0,
	bublbobl_sh_start,
	YM2203_sh_stop,
	YM2203_sh_update
};

static struct MachineDriver boblbobl_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			interrupt,1	/* interrupt mode 1, unlike Bubble Bobble */
		},
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			2,			/* memory_region */
			readmem_lvl,writemem_lvl,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1	/* NMIs are triggered by the main CPU; I don't know */
						/* whether the IRQs should be triggered by the 2203 */
		}
    },
    60,				/* frames_per_second */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    0,		/* init_machine() */

    /* video hardware */
    32*8, 32*8,			/* screen_width, height */
    { 0, 32*8-1, 2*8, 30*8-1 }, /* visible_area */
    gfxdecodeinfo,
    256, /* total_colours */
    16*16,		/* color_table_len */
    bublbobl_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	bublbobl_vh_start,
	bublbobl_vh_stop,
	bublbobl_vh_screenrefresh,

	0,
	bublbobl_sh_start,
	YM2203_sh_stop,
	YM2203_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bublbobl_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "a78_06.bin", 0x00000, 0x8000, 0x54bbd2ad )
    ROM_LOAD( "a78_05.bin", 0x08000, 0x4000, 0x366354e5 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(           0x10000, 0xc000 )				/* bank 0 at 8000 because the code falls into */
															/* it from 7fff, so bank switching wouldn't work */
    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "a78_09.bin", 0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "a78_10.bin", 0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "a78_11.bin", 0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "a78_12.bin", 0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "a78_13.bin", 0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "a78_14.bin", 0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "a78_15.bin", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "a78_16.bin", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "a78_17.bin", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "a78_18.bin", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "a78_19.bin", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "a78_20.bin", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "a78_08.bin", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "a78_07.bin", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END

ROM_START( boblbobl_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "bb3", 0x00000, 0x8000, 0x9202336e )
    ROM_LOAD( "bb5", 0x08000, 0x4000, 0x0d5aa1e0 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(    0x10000, 0x4000 )				/* bank 0 at 8000 because the code falls into */
													/* it from 7fff, so bank switching wouldn't work */
    ROM_LOAD( "bb4", 0x14000, 0x8000, 0xc6fc6192 )	/* banked at 8000-bfff */

    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bb6",  0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "bb7",  0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "bb8",  0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "bb9",  0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "bb10", 0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "bb11", 0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "bb12", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "bb13", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "bb14", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "bb15", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "bb16", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "bb17", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "bb1", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "bb2", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END

ROM_START( sboblbob_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "bbb-3.rom", 0x00000, 0x8000, 0xcbbab03c )
    ROM_LOAD( "bbb-5.rom", 0x08000, 0x4000, 0x0d5aa1e0 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(    0x10000, 0x4000 )				/* bank 0 at 8000 because the code falls into */
													/* it from 7fff, so bank switching wouldn't work */
    ROM_LOAD( "bbb-4.rom", 0x14000, 0x8000, 0x8e95270d )	/* banked at 8000-bfff */

    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bbb-11.rom", 0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "bbb-10.rom", 0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "bbb-9.rom",  0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "bbb-8.rom",  0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "bbb-7.rom",  0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "bbb-6.rom",  0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "bbb-17.rom", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "bbb-16.rom", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "bbb-15.rom", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "bbb-14.rom", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "bbb-13.rom", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "bbb-12.rom", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "bbb-1.rom", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "bbb-2.rom", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END



/* High Score memory, etc
 *
 *  E64C-E64E - the high score - initialised with points for first extra life
 *
 *  [ E64F-E653 - memory for copy protection routine ]
 *
 *  E654/5/6 score 1     E657 level     E658/9/A initials
 *  E65B/C/D score 2     E65E level     E65f/0/1 initials
 *  E662/3/4 score 3     E665 level     E666/7/8 initials
 *  E669/A/B score 4     E66C level     E66d/E/F initials
 *  E670/1/2 score 5     E673 level     E674/5/6 initials
 *
 *  E677 - position we got to in the high scores
 *  E67B - initialised to 1F - record level number reached
 *  E67C - initialised to 1F - p1's best level reached
 *  E67D - initialised to 13 - p2's best level reached
 */
static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe654],&RAM[0xe670],3) == 0 &&
			memcmp(&RAM[0xe654],&RAM[0xe64c],3) == 0 &&	/* high score */
			memcmp(&RAM[0xe67b],"\x1f\x1f\x13",3) == 0 &&	/* level number */
			(memcmp(&RAM[0xe654],"\x00\x20\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x30\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x40\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x50\x00",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe654],7*5);
			osd_fread(f,&RAM[0xe67b],3);
			RAM[0xe64c] = RAM[0xe654];
			RAM[0xe64d] = RAM[0xe655];
			RAM[0xe64e] = RAM[0xe656];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xe654],7*5);
		osd_fwrite(f,&RAM[0xe67b],3);
		osd_fclose(f);
	}
}



struct GameDriver bublbobl_driver =
{
	"Bubble Bobble",
	"bublbobl",
	"Chris Moore\nOliver White\nNicola Salmoria\nMarco Cassili",
	&bublbobl_machine_driver,

	bublbobl_rom,
	bublbobl_patch, 0,	/* remove protection */
	0,
	0,	/* sound_prom */

	0/*TBR*/, bublbobl_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver boblbobl_driver =
{
	"Bobble Bobble (bootleg Bubble Bobble)",
	"boblbobl",
	"Chris Moore\nOliver White\nNicola Salmoria\nMarco Cassili",
	&boblbobl_machine_driver,

	boblbobl_rom,
	boblbobl_patch, 0,	/* remove protection */
	0,
	0,	/* sound_prom */

	0/*TBR*/, boblbobl_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver sboblbob_driver =
{
	"Bobble Bobble (bootleg Bubble Bobble, alternate version)",
	"sboblbob",
	"Chris Moore\nOliver White\nNicola Salmoria\nMarco Cassili",
	&boblbobl_machine_driver,

	sboblbob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, sboblbob_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
