/*******************************************************************************

	Actfancer						(c) 1989 Data East Corporation

	Doesn't work due to bad program rom dump.
	Dip switches are wrong...

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/h6280/h6280.h"

void actfancr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void actfancr_pf1_data_w(int offset, int data);
int actfancr_pf1_data_r(int offset);
void actfancr_pf1_control_w(int offset, int data);
void actfancr_pf2_data_w(int offset, int data);
int actfancr_pf2_data_r(int offset);
void actfancr_pf2_control_w(int offset, int data);
int actfancr_vh_start (void);
void actfancr_vh_stop (void);

void actfancr_palette_w(int offset, int data);

extern unsigned char *actfancr_pf1_data,*actfancr_pf2_data;

/******************************************************************************/

static int actfan_control_0_r(int offset)
{
	switch (offset) {
		case 0: /* VBL */
			return readinputport(2);
	}

	return 0xff;
}

static int actfan_control_1_r(int offset)
{

if (errorlog) fprintf(errorlog,"%04x: Control 1 : %02x\n",cpu_get_pc(),offset);

	switch (offset) {
		case 0: /* Dip 1 */
			return readinputport(0);

		case 1: /* Dip 2 */
			return readinputport(1);

	}

	return 0xff;

}

static void actfancr_sound_w(int offset, int data)
{
	soundlatch_w(0,data & 0xff);
	cpu_cause_interrupt(1,M6502_INT_NMI);
}

/******************************************************************************/

static struct MemoryReadAddress actfan_readmem[] =
{
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x062000, 0x063fff, actfancr_pf1_data_r },
	{ 0x072000, 0x0727ff, actfancr_pf2_data_r },
	{ 0x100000, 0x1007ff, MRA_BANK1 },
	{ 0x130000, 0x130003, actfan_control_1_r },
	{ 0x140000, 0x140001, actfan_control_0_r },
	{ 0x120000, 0x1205ff, paletteram_r },
	{ 0x1f0000, 0x1f3fff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress actfan_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x060000, 0x06001f, actfancr_pf1_control_w },
	{ 0x062000, 0x063fff, actfancr_pf1_data_w, &actfancr_pf1_data },
	{ 0x070000, 0x07001f, actfancr_pf2_control_w },
	{ 0x072000, 0x0727ff, actfancr_pf2_data_w, &actfancr_pf2_data },
	{ 0x100000, 0x1007ff, MWA_BANK1, &spriteram },
	{ 0x110000, 0x110001, MWA_NOP },
	{ 0x120000, 0x1205ff, actfancr_palette_w, &paletteram },
	{ 0x150000, 0x150001, actfancr_sound_w },
	{ 0x1f0000, 0x1f3fff, MWA_RAM }, /* Main ram */
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress dec0_s_readmem[] =
{
	{ 0x0000, 0x05ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x3800, 0x3800, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress dec0_s_writemem[] =
{
	{ 0x0000, 0x05ff, MWA_RAM },
	{ 0x0800, 0x0800, YM2203_control_port_0_w },
	{ 0x0801, 0x0801, YM2203_write_port_0_w },
	{ 0x1000, 0x1000, YM3812_control_port_0_w },
	{ 0x1001, 0x1001, YM3812_write_port_0_w },
	{ 0x3800, 0x3800, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( actfancr_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x0c, 0x0c, "Number of K for bonus" )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x04, "90" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty" )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Atract Mode Sound" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPNAME( 0x80, 0x80, "Timer Speed" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Right Coin" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Left Coin" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "Screen Rotation" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x80, 0x80, "No Die Mode" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout chars =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x08000*8, 0x18000*8, 0x00000*8, 0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tiles =
{
	16,16,	/* 16*16 sprites */
	2048,
	4,
	{ 0x30000*8, 0x10000*8,0x20000*8, 0,    },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout sprites =
{
	16,16,	/* 16*16 sprites */
	2048+1024,
	4,
	{ 0x18000*8,  0x48000*8, 0, 0x30000*8 },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo actfan_gfxdecodeinfo[] =
{
	{ 1, 0x60000, &chars,       0, 16 },
	{ 1, 0x00000, &sprites,   256, 16 },
	{ 1, 0x80000, &tiles,     512, 16 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(void)
{
	cpu_cause_interrupt(1,M6502_INT_IRQ);
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,
	{ YM2203_VOL(50,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3.000000 MHz */
	{ 45 },
	sound_irq,
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz frequency */
	{ 3 },          /* memory region 3 */
	{ 85 }
};

/******************************************************************************/

static int actfan_interrupt(void)
{
	return H6280_INT_IRQ1;
}

static struct MachineDriver actfan_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_H6280,
			4000000,	/* 4 Mhz????? */
			0,
			actfan_readmem,actfan_writemem,0,0,
			actfan_interrupt,1 /* VBL */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0	/* Interrupts from OPL chip */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written*/
	0,

	/* video hardware */
32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
//	64*8, 64*8, { 0*8, 64*8-1, 1*8, 63*8-1 },

	actfan_gfxdecodeinfo,
	768, 768,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	actfancr_vh_start,
	actfancr_vh_stop,
	actfancr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/******************************************************************************/

ROM_START( actfancr_rom )
	ROM_REGION(0x200000) /* Need to allow full RAM allocation for now */
	ROM_LOAD( "08-1", 0x00000, 0x10000, 0x3bf214a4 )
	ROM_LOAD( "09-1", 0x10000, 0x10000, 0x13ae78d5 )
	ROM_LOAD( "10",   0x20000, 0x10000, 0x00000000 )

	ROM_REGION_DISPOSE(0xc0000)
	ROM_LOAD( "00", 0x08000, 0x10000, 0xd50a9550 ) /* Sprites */
	ROM_LOAD( "01", 0x00000, 0x08000, 0x34935e93 )
	ROM_LOAD( "02", 0x20000, 0x10000, 0xb1db0efc )
	ROM_LOAD( "03", 0x18000, 0x08000, 0xf313e04f )
	ROM_LOAD( "04", 0x38000, 0x10000, 0xbcf41795 )
	ROM_LOAD( "05", 0x30000, 0x08000, 0xd38b94aa )
	ROM_LOAD( "06", 0x50000, 0x10000, 0x8cb6dd87 )
	ROM_LOAD( "07", 0x48000, 0x08000, 0xdd345def )

	ROM_LOAD( "15", 0x60000, 0x10000, 0xa1baf21e ) /* Chars */
	ROM_LOAD( "16", 0x70000, 0x10000, 0x22e64730 )

	ROM_LOAD( "11", 0x80000, 0x10000, 0x1f006d9f ) /* Tiles */
	ROM_LOAD( "12", 0x90000, 0x10000, 0x08787b7a )
	ROM_LOAD( "13", 0xa0000, 0x10000, 0xc30c37dc )
	ROM_LOAD( "14", 0xb0000, 0x10000, 0xd6457420 )

	ROM_REGION(0x10000) /* 6502 Sound CPU */
	ROM_LOAD( "17-1", 0x08000, 0x8000, 0x289ad106 )

	ROM_REGION(0x10000) /* ADPCM sounds */
	ROM_LOAD( "18",   0x00000, 0x10000, 0x5c55b242 )
ROM_END

/******************************************************************************/

struct GameDriver actfancr_driver =
{
	__FILE__,
	0,
	"actfancr",
	"Act Fancer",
	"1989",
	"Data East Corporation",
	"Bryan McPhail",
	GAME_NOT_WORKING,
	&actfan_machine_driver,
	0,

	actfancr_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	actfancr_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};
