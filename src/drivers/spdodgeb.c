/***************************************************************************

Super Dodgeball / Nekketsu Koukou Dodgeball Bu

briver by Paul Hampson and Nicola Salmoria

TODO:
- sprite lag
- rowscroll

Notes:
- there's probably a 63701 on the board, used for protection. It is checked
  on startup and then just used to read the input ports. It doesn't return
  the ports verbatim, it adds further processing, setting flags when the
  player double-taps in one direction to run.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"


extern unsigned char *spdodgeb_videoram;

void spdodgeb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int spdodgeb_vh_start(void);
void spdodgeb_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
int spdodgeb_interrupt(void);
WRITE_HANDLER( spdodgeb_scrollx_lo_w );
WRITE_HANDLER( spdodgeb_ctrl_w );
WRITE_HANDLER( spdodgeb_videoram_w );


/* private globals */
static int toggle=0;//, soundcode = 0;
static int adpcm_pos[2],adpcm_end[2],adpcm_idle[2];
/* end of private globals */


static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,M6809_INT_IRQ);
}

static WRITE_HANDLER( spd_adpcm_w )
{
	int chip = offset & 1;

	switch (offset/2)
	{
		case 3:
			adpcm_idle[chip] = 1;
			MSM5205_reset_w(chip,1);
			break;

		case 2:
			adpcm_pos[chip] = (data & 0x7f) * 0x200;
			break;

		case 1:
			adpcm_end[chip] = (data & 0x7f) * 0x200;
			break;

		case 0:
			adpcm_idle[chip] = 0;
			MSM5205_reset_w(chip,0);
			break;
	}
}

static void spd_adpcm_int(int chip)
{
	static int adpcm_data[2] = { -1, -1 };

	if (adpcm_pos[chip] >= adpcm_end[chip] || adpcm_pos[chip] >= 0x10000)
	{
		adpcm_idle[chip] = 1;
		MSM5205_reset_w(chip,1);
	}
	else if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip,adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1) + 0x10000 * chip;

		adpcm_data[chip] = ROM[adpcm_pos[chip]++];
		MSM5205_data_w(chip,adpcm_data[chip] >> 4);
	}
}


static int mcu63701_command;
static int inputs[4];

static void mcu63705_update_inputs(void)
{
	static int running[2],jumped[2];
	int buttons[2];
	int p,j;

	/* update running state */
	for (p = 0;p <= 1;p++)
	{
		static int prev[2][2],countup[2][2],countdown[2][2];
		int curr[2][2];

		curr[p][0] = readinputport(2+p) & 0x01;
		curr[p][1] = readinputport(2+p) & 0x02;

		for (j = 0;j <= 1;j++)
		{
			if (curr[p][j] == 0)
			{
				if (prev[p][j] != 0)
					countup[p][j] = 0;
				if (curr[p][j^1])
					countup[p][j] = 100;
				countup[p][j]++;
				running[p] &= ~(1 << j);
			}
			else
			{
				if (prev[p][j] == 0)
				{
					if (countup[p][j] < 10 && countdown[p][j] < 5)
						running[p] |= 1 << j;
					countdown[p][j] = 0;
				}
				countdown[p][j]++;
			}
		}

		prev[p][0] = curr[p][0];
		prev[p][1] = curr[p][1];
	}

	/* update jumping and buttons state */
	for (p = 0;p <= 1;p++)
	{
		static int prev[2];
		int curr[2];

		curr[p] = readinputport(2+p) & 0x30;

		if (jumped[p]) buttons[p] = 0;	/* jump only momentarily flips the buttons */
		else buttons[p] = curr[p];

		if (buttons[p] == 0x30) jumped[p] = 1;
		if (curr[p] == 0x00) jumped[p] = 0;

		prev[p] = curr[p];
	}

	inputs[0] = readinputport(2) & 0xcf;
	inputs[1] = readinputport(3) & 0x0f;
	inputs[2] = running[0] | buttons[0];
	inputs[3] = running[1] | buttons[1];
}

static READ_HANDLER( mcu63701_r )
{
//	logerror("CPU #0 PC %04x: read from port %02x of 63701 data address 3801\n",cpu_get_pc(),offset);

	if (mcu63701_command == 0) return 0x6a;
	else switch (offset)
	{
		default:
		case 0: return inputs[0];
		case 1: return inputs[1];
		case 2: return inputs[2];
		case 3: return inputs[3];
		case 4: return readinputport(4);
	}
}

static WRITE_HANDLER( mcu63701_w )
{
//	logerror("CPU #0 PC %04x: write %02x to 63701 control address 3800\n",cpu_get_pc(),data);
	mcu63701_command = data;
	mcu63705_update_inputs();
}


static READ_HANDLER( port_0_r )
{
	int port = readinputport(0);

	toggle^=0x02;	/* mcu63701_busy flag */

	return (port | toggle);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3000, port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },	/* DIPs */
	{ 0x3801, 0x3805, mcu63701_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x10ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x2fff, spdodgeb_videoram_w, &spdodgeb_videoram },
//	{ 0x3000, 0x3000, MWA_RAM },
//	{ 0x3001, 0x3001, MWA_RAM },
	{ 0x3002, 0x3002, sound_command_w },
//	{ 0x3003, 0x3003, MWA_RAM },
	{ 0x3004, 0x3004, spdodgeb_scrollx_lo_w },
//	{ 0x3005, 0x3005, MWA_RAM }, /* mcu63701_output_w */
	{ 0x3006, 0x3006, spdodgeb_ctrl_w },	/* scroll hi, flip screen, bank switch, palette select */
	{ 0x3800, 0x3800, mcu63701_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM3812_control_port_0_w },
	{ 0x2801, 0x2801, YM3812_write_port_0_w },
	{ 0x3800, 0x3807, spd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( spdodgeb )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* mcu63701_busy flag */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy")
	PORT_DIPSETTING(    0xc0, "Normal")
	PORT_DIPSETTING(    0x40, "Hard")
	PORT_DIPSETTING(    0x00, "Very Hard")

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 64+1, 64+0, 128+1, 128+0, 192+1, 192+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0,4 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		  32*8+3, 32*8+2, 32*8+1, 32*8+0, 48*8+3, 48*8+2, 48*8+1, 48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000, 32 },	/* colors 0x000-0x1ff */
	{ REGION_GFX2, 0, &spritelayout, 0x200, 32 },	/* colors 0x200-0x3ff */
	{ -1 } /* end of array */
};


static void irq_handler(int irq)
{
	cpu_set_irq_line(1,M6809_FIRQ_LINE,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000, 	/* 3MHz ? */
	{ 60 },		/* volume */
	{ irq_handler }
};

static struct MSM5205interface msm5205_interface =
{
	2,			/* 2 chips */
	384000,		/* 384KHz */
	{ spd_adpcm_int, spd_adpcm_int },	/* interrupt function */
	{ MSM5205_S48_4B, MSM5205_S48_4B },	/* 8kHz? */
	{ 50, 50 }	/* volume */
};



static struct MachineDriver machine_driver_spdodgeb =
{
	{
		{
 			CPU_M6502,
			12000000/6,	/* 2MHz ? */
			readmem,writemem,0,0,
			spdodgeb_interrupt,34	/* 1 IRQ every 8 visible scanlines, plus NMI for vblank */
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			12000000/6,	/* 2MHz ? */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0 /* irq on command */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */				\
	0,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	spdodgeb_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,	/* palette is static, but doesn't fit in 256 colors */
	0,
	spdodgeb_vh_start,
	0,
	spdodgeb_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



ROM_START( spdodgeb )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "22a-04.139",	  0x10000, 0x08000, 0x66071fda )  /* Two banks */
	ROM_CONTINUE(             0x08000, 0x08000 )		 /* Static code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* audio cpu */
	ROM_LOAD( "22j5-0.33",    0x08000, 0x08000, 0xc31e264e )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* I/O mcu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, 0x00000000 )	/* missing */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE ) /* text */
	ROM_LOAD( "22a-4.121",    0x00000, 0x20000, 0xacc26051 )
	ROM_LOAD( "22a-3.107",    0x20000, 0x20000, 0x10bb800d )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "22a-1.2",      0x00000, 0x20000, 0x3bd1c3ec )
	ROM_LOAD( "22a-2.35",     0x20000, 0x20000, 0x409e1be1 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "22j6-0.83",    0x00000, 0x10000, 0x744a26e3 )
	ROM_LOAD( "22j7-0.82",    0x10000, 0x10000, 0x2fa1de21 )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD( "mb7132e.158",  0x0000, 0x0400, 0x7e623722 )
	ROM_LOAD( "mb7122e.159",  0x0400, 0x0400, 0x69706e8d )
ROM_END

ROM_START( nkdodgeb )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "12.bin",	      0x10000, 0x08000, 0xaa674fd8 )  /* Two banks */
	ROM_CONTINUE(             0x08000, 0x08000 )		 /* Static code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* audio cpu */
	ROM_LOAD( "22j5-0.33",    0x08000, 0x08000, 0xc31e264e )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* I/O mcu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, 0x00000000 )	/* missing */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE ) /* text */
	ROM_LOAD( "10.bin",       0x00000, 0x10000, 0x442326fd )
	ROM_LOAD( "11.bin",       0x10000, 0x10000, 0x2140b070 )
	ROM_LOAD( "9.bin",        0x20000, 0x10000, 0x18660ac1 )
	ROM_LOAD( "8.bin",        0x30000, 0x10000, 0x5caae3c9 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin",        0x00000, 0x10000, 0x1271583e )
	ROM_LOAD( "1.bin",        0x10000, 0x10000, 0x5ae6cccf )
	ROM_LOAD( "4.bin",        0x20000, 0x10000, 0xf5022822 )
	ROM_LOAD( "3.bin",        0x30000, 0x10000, 0x05a71179 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "22j6-0.83",    0x00000, 0x10000, 0x744a26e3 )
	ROM_LOAD( "22j7-0.82",    0x10000, 0x10000, 0x2fa1de21 )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD( "mb7132e.158",  0x0000, 0x0400, 0x7e623722 )
	ROM_LOAD( "mb7122e.159",  0x0400, 0x0400, 0x69706e8d )
ROM_END



GAME( 1987, spdodgeb, 0,        spdodgeb, spdodgeb, 0, ROT0, "Technos", "Super Dodge Ball (US)" )
GAME( 1987, nkdodgeb, spdodgeb, spdodgeb, spdodgeb, 0, ROT0, "Technos", "Nekketsu Koukou Dodgeball Bu (Japan bootleg)" )
