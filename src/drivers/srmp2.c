/***************************************************************************

Super Real Mahjong P2
-------------------------------------
driver by Yochizo and Takahiro Nogi

  Yochizo took charge of video and I/O part.
  Takahiro Nogi took charge of sound, I/O and NVRAM part.

  ... and this is based on "seta.c" driver written by Luca Elia.

  Thanks for your reference, Takahiro Nigi and Luca Elia.


Supported games :
==================
 Super Real Mahjong P2        (C) 1987 Seta


Not supported game :
=====================
 Super Real Mahjong P1 (not dumped)
 Super Real Mahjong P3 (Z80 based system but used same chips as "P2")


System specs :
===============
   CPU       : 68000 (8MHz)
   Sound     : AY8910 + MSM5205
   Chips     : X1-001, X1-002A, X1-003, X1-004x2, X0-005 x2
           X1-001, X1-002A  : Sprites
           X1-003           : Video output
           X1-004           : ???
           X1-005           : ???


Known issues :
===============
 - Sound frequency is hand-tuned value, so it may not be correct.
 - I/O port isn't fully analized. Currently avoid I/O error message with hack.


****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"


/***************************************************************************

  Variables

***************************************************************************/

void srmp2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void srmp2_vh_screenrefresh     (struct osd_bitmap *bitmap,int full_refresh);



extern int srmp2_color_bank;

static int srmp2_adpcm_code;					// debug
static int srmp2_adpcm_bank;
static int srmp2_adpcm_data;
static unsigned int srmp2_adpcm_sptr;
static unsigned int srmp2_adpcm_eptr;

#ifdef MAME_DEBUG
static int srmp2_adpcm_ctr = 0;					// debug
#endif

//static int srmp2_portdata1, srmp2_portdata2;	// debug
static int srmp2_port_select;

static unsigned short	*srmp2_nvram;
static size_t			srmp2_nvram_size;


/***************************************************************************

  Interrupt(s)

***************************************************************************/

static int srmp2_interrupt(void)
{
	switch (cpu_getiloops())
	{
		case 0:		return 4;	/* vblank */
		default:	return 2;	/* sound */
	}
}


static void init_srmp2(void)
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* Fix "ERROR BACK UP" and "ERROR IOX" */
	RAM[0x20c80 / 2] = 0x4e75;
}


static void srmp2_init_machine(void)
{
	srmp2_port_select = 0;
}


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static void srmp2_nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
	{
		osd_fwrite(file, srmp2_nvram, srmp2_nvram_size);
	}
	else
	{
		if (file)
		{
			osd_fread(file, srmp2_nvram, srmp2_nvram_size);
		}
		else
		{
			memset(srmp2_nvram, 0, srmp2_nvram_size);
		}
	}
}


static READ_HANDLER( srmp2_dsw1_r )
{
	return readinputport(2);
}


static READ_HANDLER( srmp2_dsw2_r )
{
	return readinputport(1);
}


static WRITE16_HANDLER( srmp2_flags_w )
{
/*
	---- ---x : Coin Counter
	---x ---- : Coin Lock Out
	--x- ---- : ADPCM Bank
	x--- ---- : Palette Bank
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
	srmp2_adpcm_bank = ( (data & 0x20) >> 5 );
	srmp2_color_bank = ( (data & 0x80) >> 0 );
}


static WRITE16_HANDLER( srmp2_adpcm_code_w )
{
	/*
		- Received data may be playing ADPCM number.
		- 0x000000 - 0x0000ff and 0x010000 - 0x0100ff are offset table.
		- When the hardware receives the ADPCM number, it refers the offset
		  table and plays the ADPCM for itself.
	*/

	unsigned char *ROM = memory_region(REGION_SOUND1);

	srmp2_adpcm_code = data;			// debug

	srmp2_adpcm_sptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 0)] << 8);
	srmp2_adpcm_eptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 1)] << 8);

	if (srmp2_adpcm_eptr)
	{
		/* Found the data */
		MSM5205_reset_w(0, 0);
		srmp2_adpcm_data = -1;

		srmp2_adpcm_sptr += (srmp2_adpcm_bank * 0x10000);
		srmp2_adpcm_eptr += (srmp2_adpcm_bank * 0x10000);
		srmp2_adpcm_eptr -= 1;
	}
	else
	{
		/* Not found the data */
		srmp2_adpcm_sptr = srmp2_adpcm_eptr = 0;
		MSM5205_reset_w(0, 1);
	}

}


static void srmp2_adpcm_int(int num)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

#ifdef MAME_DEBUG
	if (keyboard_pressed(KEYCODE_INSERT))
	{
		usrintf_showmessage("CODE:%04X SPTR:%08X EPTR:%08X DATA:%08X CTR:%08X",
			srmp2_adpcm_code, srmp2_adpcm_sptr, srmp2_adpcm_eptr, srmp2_adpcm_data, srmp2_adpcm_ctr++);
	}
#endif

	if (srmp2_adpcm_sptr)
	{
		if (srmp2_adpcm_data == -1)
		{
			srmp2_adpcm_data = ROM[srmp2_adpcm_sptr];

			if (srmp2_adpcm_sptr >= srmp2_adpcm_eptr)
			{
				MSM5205_reset_w(0, 1);
				srmp2_adpcm_data = 0;
				srmp2_adpcm_sptr = 0;
			}
			else
			{
				MSM5205_data_w(0, ((srmp2_adpcm_data >> 4) & 0x0f));
			}
		}
		else
		{
			MSM5205_data_w(0, ((srmp2_adpcm_data >> 0) & 0x0f));
			srmp2_adpcm_sptr++;
			srmp2_adpcm_data = -1;
		}
	}
	else
	{
		MSM5205_reset_w(0, 1);
	}
}


static READ16_HANDLER( srmp2_adpcm_status_0_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_adpcm_status_1_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_input_1_r )
{

/*
	---x xxxx : Key code
	--x- ---- : Player 1 and 2 side flag
*/

	if (!ACCESSING_LSB)
	{
		return 0x00;
	}

	if (!srmp2_port_select)		/* Panel keys */
	{
		int i, j, t;

		for (i = 0x00 ; i < 0x20 ; i += 8)
		{
			j = (i / 0x08) + 3;

			for (t = 0 ; t < 8 ; t ++)
			{
				if (readinputport(j) & ( 1 << t ))
				{
					return i + t;
				}
			}
		}
	}
	else						/* Analizer and memory reset keys */
	{
		return readinputport(7);
	}

	return 0x00;
}


static READ16_HANDLER( srmp2_input_2_r )
{
	if (!ACCESSING_LSB)
	{
		return 0x01;
	}

	srmp2_port_select ^= 1;

	/* Always return 1, otherwise freeze. Maybe read I/O status */
	return 0x01;
}



/**************************************************************************

  Memory Map(s)

**************************************************************************/


static MEMORY_READ16_START( srmp2_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c3fff, MRA16_RAM },
	{ 0x140000, 0x143fff, MRA16_RAM },				/* Sprites Code + X + Attr */
	{ 0x180000, 0x180607, MRA16_RAM },				/* Sprites Y */
	{ 0x900000, 0x900001, input_port_0_word_r },	/* Coinage */

	{ 0xa00000, 0xa00001, srmp2_input_1_r }, 		/* I/O port 1 */
	{ 0xa00002, 0xa00003, srmp2_input_2_r },		/* I/O port 2 */

	{ 0xb00000, 0xb00001, srmp2_adpcm_status_0_r },	/* ADPCM status ??? */
	{ 0xb00002, 0xb00003, srmp2_adpcm_status_1_r },	/* ADPCM status ??? */
	{ 0xf00000, 0xf00001, AY8910_read_port_0_lsb_r	},
MEMORY_END

static MEMORY_WRITE16_START( srmp2_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0c0000, 0x0c3fff, MWA16_RAM, &srmp2_nvram, &srmp2_nvram_size },
	{ 0x140000, 0x143fff, MWA16_RAM, &spriteram16_2 },	/* Sprites Code + X + Attr */
	{ 0x180000, 0x180609, MWA16_RAM, &spriteram16 },	/* Sprites Y */
	{ 0x1c0000, 0x1c0001, MWA16_NOP },					/* ??? */
	{ 0x800000, 0x800001, srmp2_flags_w },				/* Sprite color bank, etc. */
	{ 0x900000, 0x900001, MWA16_NOP },					/* ??? */

	{ 0xa00000, 0xa00001, MWA16_NOP },					/* I/O ??? */
	{ 0xa00002, 0xa00003, MWA16_NOP },					/* I/O ??? */

	{ 0xb00000, 0xb00001, srmp2_adpcm_code_w },			/* ADPCM number */
	{ 0xc00000, 0xc00001, MWA16_NOP },					/* ??? */
	{ 0xd00000, 0xd00001, MWA16_NOP },					/* ??? */
	{ 0xe00000, 0xe00001, MWA16_NOP },					/* ??? */
	{ 0xf00000, 0xf00001, AY8910_control_port_0_lsb_w },
	{ 0xf00002, 0xf00003, AY8910_write_port_0_lsb_w },
MEMORY_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( srmp2 )
	PORT_START			/* Coinnage (0) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (1) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	// DIPSW B8
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )	// DIPSW B7
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )	// DIPSW B6
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )	// DIPSW B5
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )	// DIPSW B4
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (Easy)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )

	PORT_START			/* DSW (2) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )	// DIPSW A1
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* INPUT1 (3) */
	PORT_BIT(  0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "P1 Small",       KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "P1 Double Up",   KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, 0, "P1 Big",         KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "P1 Take Score",  KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, 0, "P1 Flip",        KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, 0, "P1 Last Chance", KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START			/* INPUT1 (4) */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, 0, "P1 K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "P1 Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "P1 G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "P1 C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, 0, "P1 L",   KEYCODE_L,     IP_JOY_NONE )
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START			/* INPUT1 (5) */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, 0, "P1 Start", KEYCODE_1,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE )

	PORT_START			/* INPUT1 (6) */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, 0, "P1 A",     KEYCODE_A,      IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "P1 Bet",   KEYCODE_2,      IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "P1 J",     KEYCODE_J,      IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "P1 F",     KEYCODE_F,      IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, 0, "P1 N",     KEYCODE_N,      IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, 0, "P1 B",     KEYCODE_B,      IP_JOY_NONE )
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START			/* INPUT1 (7) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct AY8910interface ay8910_interface =
{
	1,
	20000000/8,					/* 2.50 MHz ??? */
	{ 40 },
	{ srmp2_dsw1_r },			/* Input A: DSW 1 */
	{ srmp2_dsw2_r },			/* Input B: DSW 2 */
	{ 0 },
	{ 0 }
};


struct MSM5205interface msm5205_interface =
{
	1,
	384000,
	{ srmp2_adpcm_int },		/* IRQ handler */
	{ MSM5205_S48_4B },			/* 8 KHz, 4 Bits  */
	{ 40 }
};


static struct GfxLayout charlayout =
{
	16, 16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1, 2) + 8, RGN_FRAC(1, 2) + 0, 8, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	16*16*2
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16*32 },
	{ -1 }
};


static struct MachineDriver machine_driver_srmp2 =
{
	{
		{
			CPU_M68000,
			8000000,
			srmp2_readmem, srmp2_writemem, 0, 0,
			srmp2_interrupt, 16		/* Interrupt times is not understood */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	srmp2_init_machine,

	/* video hardware */
	464, 256 -16, { 16, 464-1, 8, 256-1-24 },
	gfxdecodeinfo,
	1024, 1024,			/* sprites only */
	srmp2_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	0,	/* no need for a vh_start: no tilemaps */
	0,
	srmp2_vh_screenrefresh, 	/* just draw the sprites */

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_AY8910, &ay8910_interface },
		{ SOUND_MSM5205, &msm5205_interface }
	},
	srmp2_nvram_handler
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( srmp2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "uco-2.17", 0x000000, 0x020000, 0x0d6c131f )
	ROM_LOAD16_BYTE( "uco-3.18", 0x000001, 0x020000, 0xe9fdf5f8 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD       ( "ubo-4.60",  0x000000, 0x040000, 0xcb6f7cce )
	ROM_LOAD       ( "ubo-5.61",  0x040000, 0x040000, 0x7b48c540 )
	ROM_LOAD16_BYTE( "uco-8.64",  0x080000, 0x040000, 0x1ca1c7c9 )
	ROM_LOAD16_BYTE( "uco-9.65",  0x080001, 0x040000, 0xef75471b )
	ROM_LOAD       ( "ubo-6.62",  0x100000, 0x040000, 0x6c891ac5 )
	ROM_LOAD       ( "ubo-7.63",  0x140000, 0x040000, 0x60a45755 )
	ROM_LOAD16_BYTE( "uco-10.66", 0x180000, 0x040000, 0xcb6bd857 )
	ROM_LOAD16_BYTE( "uco-11.67", 0x180001, 0x040000, 0x199f79c0 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "uco-1.19", 0x000000, 0x020000, 0xf284af8e )

	ROM_REGION( 0x000800, REGION_PROMS, 0 )					/* Color PROMs */
	ROM_LOAD( "uc-1o.12", 0x000000, 0x000400, 0xfa59b5cb )
	ROM_LOAD( "uc-2o.13", 0x000400, 0x000400, 0x50a33b96 )
ROM_END


/*   ( YEAR   NAME  PARENT  MACHINE    INPUT     INIT     MONITOR  COMPANY  FULLNAME ) */
GAME( 1987, srmp2,      0,   srmp2,   srmp2,   srmp2, ROT0_16BIT,  "Seta", "Super Real Mahjong P2 (Japan)" )
