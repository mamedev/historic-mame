/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Pastel Gal
	(c)1985 Nihon Bussan Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/06/07 -

******************************************************************************/
/******************************************************************************
Memo:

- Custom chip used by pastelgl PCB is 1411M1.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "machine/nb1413m3.h"


#define	SIGNED_DAC	0		// 0:unsigned DAC, 1:signed DAC


void pastelgl_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
void pastelgl_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
int pastelgl_vh_start(void);
void pastelgl_vh_stop(void);

void pastelgl_paltbl_w(int offset, int data);
void pastelgl_radrx_w(int data);
void pastelgl_radry_w(int data);
void pastelgl_sizex_w(int data);
void pastelgl_sizey_w(int data);
void pastelgl_drawx_w(int data);
void pastelgl_drawy_w(int data);
void pastelgl_dispflag_w(int data);
void pastelgl_romsel_w(int data);


static int voiradr_l, voiradr_h;


void pastelgl_voiradr_l_w(int data)
{
	voiradr_l = data;
}

void pastelgl_voiradr_h_w(int data)
{
	voiradr_h = data;
}

int pastelgl_sndrom_r(int offset)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	return ROM[(((0x0100 * voiradr_h) + voiradr_l) & 0x7fff)];
}

static void init_pastelgl(void)
{
	nb1413m3_type = NB1413M3_PASTELGL;
	nb1413m3_int_count = 96;
}


static MEMORY_READ_START( readmem_pastelgl )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_pastelgl )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END


static READ_HANDLER( io_pastelgl_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xc000:	return pastelgl_sndrom_r(0);
		case	0xe000:	return input_port_2_r(0);
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_pastelgl )
	{ 0x0000, 0xffff, io_pastelgl_r },
PORT_END

static WRITE_HANDLER( io_pastelgl_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		pastelgl_paltbl_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	pastelgl_radrx_w(data);
				pastelgl_voiradr_l_w(data); break;
		case	0x9100:	pastelgl_radry_w(data);
				pastelgl_voiradr_h_w(data); break;
		case	0x9200:	pastelgl_drawx_w(data); break;
		case	0x9300:	pastelgl_drawy_w(data); break;
		case	0x9400:	pastelgl_sizex_w(data); break;
		case	0x9500:	pastelgl_sizey_w(data); break;
		case	0x9600:	pastelgl_dispflag_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	pastelgl_romsel_w(data);
				nb1413m3_sndrombank1_w(data);
				break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
	}
}

static PORT_WRITE_START( writeport_pastelgl )
	{ 0x0000, 0xffff, io_pastelgl_w },
PORT_END


INPUT_PORTS_START( pastelgl )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "No. of tiles on final match" )
	PORT_DIPSETTING(    0x04, "20" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x60, 0x00, "SANGEN Rush" )
	PORT_DIPSETTING(    0x06, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "infinite" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) DIPSW-C */
	PORT_DIPNAME( 0x03, 0x03, "Change Rate" )
	PORT_DIPSETTING(    0x03, "Type-A" )
	PORT_DIPSETTING(    0x02, "Type-B" )
	PORT_DIPSETTING(    0x01, "Type-C" )
	PORT_DIPSETTING(    0x00, "Type-D" )
	PORT_DIPNAME( 0x04, 0x00, "Open CPU's hand on Player's Reach" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x60, 0x60, "YAKUMAN cut" )
	PORT_DIPSETTING(    0x60, "10%" )
	PORT_DIPSETTING(    0x40, "30%" )
	PORT_DIPSETTING(    0x20, "50%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x80, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (3) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE4 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (4) PORT 1-1 */
	NBMJCTRL_PORT2	/* (5) PORT 1-2 */
	NBMJCTRL_PORT3	/* (6) PORT 1-3 */
	NBMJCTRL_PORT4	/* (7) PORT 1-4 */
	NBMJCTRL_PORT5	/* (8) PORT 1-5 */
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	1,				/* 1 chip */
	1250000,			/* 1.25 MHz ?? */
	{ 35 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct DACinterface dac_interface =
{
	1,				/* 1 channels */
	{ 50 }
};


#define NBMJDRV(_name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			19968000/8,		/* 2.496 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, _intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	0, \
\
	/* video hardware */ \
	256, 256, { 0, 256-1, 16, 240-1 }, \
	0, \
	32, 0, \
	pastelgl_init_palette, \
\
	VIDEO_TYPE_RASTER, \
	0, \
	pastelgl_vh_start, \
	pastelgl_vh_stop, \
	pastelgl_vh_screenrefresh, \
\
	/* sound hardware */ \
	0, 0, 0, 0, \
	{ \
		{ \
			SOUND_AY8910, \
			&ay8910_interface \
		}, \
		{ \
			SOUND_DAC, \
			&dac_interface \
		} \
	}, \
	_nvram_ \
};


//	     NAME, INT,  MAIN_RM,  MAIN_WM,  MAIN_RP,  MAIN_WP, NV_RAM
NBMJDRV( pastelgl,  96, pastelgl, pastelgl, pastelgl, pastelgl, nb1413m3_nvram_handler )


ROM_START( pastelgl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "pgal_09.bin",  0x00000, 0x04000, 0x1e494af3 )
	ROM_LOAD( "pgal_10.bin",  0x04000, 0x04000, 0x677cccea )
	ROM_LOAD( "pgal_11.bin",  0x08000, 0x04000, 0xc2ccea38 )

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "pgal_08.bin",  0x00000, 0x08000, 0x895961a1 )

	ROM_REGION( 0x38000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "pgal_01.bin",  0x00000, 0x08000, 0x1bb14d52 )
	ROM_LOAD( "pgal_02.bin",  0x08000, 0x08000, 0xea85673a )
	ROM_LOAD( "pgal_03.bin",  0x10000, 0x08000, 0x40011248 )
	ROM_LOAD( "pgal_04.bin",  0x18000, 0x08000, 0x10613a66 )
	ROM_LOAD( "pgal_05.bin",  0x20000, 0x08000, 0x6a152703 )
	ROM_LOAD( "pgal_06.bin",  0x28000, 0x08000, 0xf56acfe8 )
	ROM_LOAD( "pgal_07.bin",  0x30000, 0x08000, 0xfa4226dc )

	ROM_REGION( 0x0040, REGION_PROMS, 0 ) /* color */
	ROM_LOAD( "pgal_bp1.bin", 0x0000, 0x0020, 0x2b7fc61a )
	ROM_LOAD( "pgal_bp2.bin", 0x0020, 0x0020, 0x4433021e )
ROM_END


GAME( 1985, pastelgl, 0, pastelgl, pastelgl, pastelgl, ROT0, "Nichibutsu", "Pastel Gal (Japan)" )
