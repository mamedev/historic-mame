/***************************************************************************

Kageki
(c) 1988 Taito Corporation

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/11/06

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* prototypes for functions in ../machine/tnzs.c */
unsigned char *tnzs_objram, *tnzs_workram;
unsigned char *tnzs_vdcram, *tnzs_scrollram;
int tnzs_workram_r(int offset);
void tnzs_workram_w(int offset, int data);
void tnzs_bankswitch_w(int offset, int data);
void tnzs_bankswitch1_w(int offset, int data);


/* prototypes for functions in ../vidhrdw/tnzs.c */
int tnzs_vh_start(void);
void tnzs_vh_stop(void);
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


#ifdef	MAME_DEBUG
#define	NOGI_DEBUG	0
#endif


/* max samples */
#define	MAX_SAMPLES	0x2f

int kageki_init_samples(const struct MachineSound *msound)
{
	struct GameSamples *samples;
	unsigned char *scan, *src, *dest;
	int start, size;
	int i, n;

	size = sizeof(struct GameSamples) + MAX_SAMPLES * sizeof(struct GameSamples *);

	if ((Machine->samples = malloc(size)) == NULL) return 1;

	samples = Machine->samples;
	samples->total = MAX_SAMPLES;

	for (i = 0; i < samples->total; i++)
	{
		src = Machine->memory_region[3] + 0x0090;
		start = (src[(i * 2) + 1] * 256) + src[(i * 2)];
		scan = &src[start];
		size = 0;

		// check sample length
		while (1)
		{
			if (*scan++ == 0x00)
			{
				break;
			} else {
				size++;
			}
		}
		if ((samples->sample[i] = malloc(sizeof(struct GameSample) + size * sizeof(unsigned char))) == NULL) return 1;

		if (start < 0x100) start = size = 0;

		samples->sample[i]->smpfreq = 7000;	/* 7 KHz??? */
		samples->sample[i]->resolution = 8;	/* 8 bit */
		samples->sample[i]->length = size;

		// signed 8-bit sample to unsigned 8-bit sample convert
		dest = (unsigned char *)samples->sample[i]->data;
		scan = &src[start];
		for (n = 0; n < size; n++)
		{
			*dest++ = ((*scan++) ^ 0x80);
		}
#if NOGI_DEBUG
		if (errorlog) fprintf(errorlog, "samples num:%02X ofs:%04X lng:%04X\n", i, start, size);
#endif
	}

	return 0;
}


static int kageki_csport_sel = 0;
static int kageki_csport_r(int offset)
{
	int	dsw, dsw1, dsw2;

	dsw1 = readinputport(0); 		// DSW1
	dsw2 = readinputport(1); 		// DSW2

	switch (kageki_csport_sel)
	{
		case	0x00:			// DSW2 5,1 / DSW1 5,1
			dsw = (((dsw2 & 0x10) >> 1) | ((dsw2 & 0x01) << 2) | ((dsw1 & 0x10) >> 3) | ((dsw1 & 0x01) >> 0));
			break;
		case	0x01:			// DSW2 7,3 / DSW1 7,3
			dsw = (((dsw2 & 0x40) >> 3) | ((dsw2 & 0x04) >> 0) | ((dsw1 & 0x40) >> 5) | ((dsw1 & 0x04) >> 2));
			break;
		case	0x02:			// DSW2 6,2 / DSW1 6,2
			dsw = (((dsw2 & 0x20) >> 2) | ((dsw2 & 0x02) << 1) | ((dsw1 & 0x20) >> 4) | ((dsw1 & 0x02) >> 1));
			break;
		case	0x03:			// DSW2 8,4 / DSW1 8,4
			dsw = (((dsw2 & 0x80) >> 4) | ((dsw2 & 0x08) >> 1) | ((dsw1 & 0x80) >> 6) | ((dsw1 & 0x08) >> 3));
			break;
		default:
			dsw = 0x00;
#if NOGI_DEBUG
			if (errorlog) fprintf(errorlog, "kageki_csport_sel error !! (0x%08X)\n", kageki_csport_sel);
#endif
	}

	return (dsw & 0xff);
}

static void kageki_csport_w(int offset, int data)
{
	char mess[80];

	if (data > 0x3f)
	{
		// read dipsw port
		kageki_csport_sel = (data & 0x03);
	} else {
		if (data > MAX_SAMPLES)
		{
			// stop samples
			sample_stop(0);
			sprintf(mess, "VOICE:%02X STOP", data);
		} else {
			// play samples
			sample_start(0, data, 0);
			sprintf(mess, "VOICE:%02X PLAY", data);
		}
#if NOGI_DEBUG
		usrintf_showmessage(mess);
#endif
	}
}


static struct MemoryReadAddress kageki_readmem1[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf1ff, MRA_RAM },
	{ 0xf200, 0xf3ff, MRA_RAM },
	{ 0xf600, 0xf600, MRA_NOP },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress kageki_writemem1[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc000, 0xdfff, MWA_RAM, &tnzs_objram },
	{ 0xe000, 0xefff, tnzs_workram_w, &tnzs_workram },
	{ 0xf000, 0xf1ff, MWA_RAM, &tnzs_vdcram },
	{ 0xf200, 0xf3ff, MWA_RAM, &tnzs_scrollram },
	{ 0xf400, 0xf400, MWA_NOP },
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
	{ 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress kageki_readmem2[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_3_r },
	{ 0xc002, 0xc002, input_port_4_r },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress kageki_writemem2[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, tnzs_workram_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( kageki_input_ports )
	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout tnzs_charlayout =
{
	16,16,
	8192,
	4,
	{ 3*8192*32*8, 2*8192*32*8, 1*8192*32*8, 0*8192*32*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};


static struct GfxDecodeInfo tnzs_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &tnzs_charlayout, 0, 32 },
	{ -1 }	/* end of array */
};


static struct Samplesinterface samples_interface =
{
	1,					/* 1 channel */
	100					/* volume */
};

static struct CustomSound_interface custom_interface =
{
	kageki_init_samples,
	0,
	0
};

static struct YM2203interface ym2203_interface =
{
	1,					/* 1 chip */
	3000000,				/* 12000000/4 ??? */
	{ YM2203_VOL(35, 15) },
	AY8910_DEFAULT_GAIN,
	{ kageki_csport_r },
	{ 0 },
	{ 0 },
	{ kageki_csport_w },
};


static struct MachineDriver kageki_machine_driver =
{
	{
		{
			CPU_Z80,
			6000000,		/* 12000000/2 ??? */
			0,			/* memory_region */
			kageki_readmem1, kageki_writemem1, 0, 0,
			interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,		/* 12000000/3 ??? */
			2,			/* memory_region */
			kageki_readmem2, kageki_writemem2, 0, 0,
			interrupt, 1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* 200 CPU slices per frame - an high value to ensure proper */
		/* synchronization of the CPUs */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	tnzs_gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		},
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kageki )
	ROM_REGION(0x30000)		// main code
	ROM_LOAD( "b35-16.11c",  0x00000, 0x08000, 0xa4e6fd58 )	/* US ver */
	ROM_CONTINUE(            0x18000, 0x08000 )
	ROM_LOAD( "b35-10.9c",   0x20000, 0x10000, 0xb150457d )

	ROM_REGION_DISPOSE(0x100000)	// gfx
	ROM_LOAD( "b35-01.13a",  0x00000, 0x20000, 0x01d83a69 )
	ROM_LOAD( "b35-02.12a",  0x20000, 0x20000, 0xd8af47ac )
	ROM_LOAD( "b35-03.10a",  0x40000, 0x20000, 0x3cb68797 )
	ROM_LOAD( "b35-04.8a",   0x60000, 0x20000, 0x71c03f91 )
	ROM_LOAD( "b35-05.7a",   0x80000, 0x20000, 0xa4e20c08 )
	ROM_LOAD( "b35-06.5a",   0xa0000, 0x20000, 0x3f8ab658 )
	ROM_LOAD( "b35-07.4a",   0xc0000, 0x20000, 0x1b4af049 )
	ROM_LOAD( "b35-08.2a",   0xe0000, 0x20000, 0xdeb2268c )

	ROM_REGION(0x18000)		// sound code
	ROM_LOAD( "b35-17.43e",  0x00000, 0x08000, 0xfdd9c246 )	/* US ver */
	ROM_CONTINUE(            0x10000, 0x08000 )

	ROM_REGION(0x10000)		// samples
	ROM_LOAD( "b35-15.98g",  0x00000, 0x10000, 0xe6212a0f )	/* US ver */
ROM_END

ROM_START( kagekij )
	ROM_REGION(0x30000)		// main code
	ROM_LOAD( "b35-09j.11c", 0x00000, 0x08000, 0x829637d5 )	/* JP ver */
	ROM_CONTINUE(            0x18000, 0x08000 )
	ROM_LOAD( "b35-10.9c",   0x20000, 0x10000, 0xb150457d )

	ROM_REGION_DISPOSE(0x100000)	// gfx
	ROM_LOAD( "b35-01.13a",  0x00000, 0x20000, 0x01d83a69 )
	ROM_LOAD( "b35-02.12a",  0x20000, 0x20000, 0xd8af47ac )
	ROM_LOAD( "b35-03.10a",  0x40000, 0x20000, 0x3cb68797 )
	ROM_LOAD( "b35-04.8a",   0x60000, 0x20000, 0x71c03f91 )
	ROM_LOAD( "b35-05.7a",   0x80000, 0x20000, 0xa4e20c08 )
	ROM_LOAD( "b35-06.5a",   0xa0000, 0x20000, 0x3f8ab658 )
	ROM_LOAD( "b35-07.4a",   0xc0000, 0x20000, 0x1b4af049 )
	ROM_LOAD( "b35-08.2a",   0xe0000, 0x20000, 0xdeb2268c )

	ROM_REGION(0x18000)		// sound code
	ROM_LOAD( "b35-11j.43e", 0x00000, 0x08000, 0x64d093fc )	/* JP ver */
	ROM_CONTINUE(            0x10000, 0x08000 )

	ROM_REGION(0x10000)		// samples
	ROM_LOAD( "b35-12j.98g", 0x00000, 0x10000, 0x184409f1 )	/* JP ver */
ROM_END



struct GameDriver kageki_driver =
{
	__FILE__,
	0,
	"kageki",
	"Kageki (US)",
	"1988",
	"Taito America Corporation (Romstar license)",
	"Takahiro Nogi",
	0,
	&kageki_machine_driver,
	0,

	kageki_rom,
	0, 0,
	0,
	0,

	kageki_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver kagekij_driver =
{
	__FILE__,
	&kageki_driver,
	"kagekij",
	"Kageki (Japan)",
	"1988",
	"Taito Corporation",
	"Takahiro Nogi",
	0,
	&kageki_machine_driver,
	0,

	kagekij_rom,
	0, 0,
	0,
	0,

	kageki_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
