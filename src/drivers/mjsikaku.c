/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Mahjong Sikaku
	(c)1988 Nihon Bussan Co.,Ltd.

	Otona no Mahjong
	(c)1988 Apple

	Mahjong Camera Kozou
	(c)1988 MIKI SYOUJI Co.,Ltd.

	Mahjong Kaguyahime (Medal Type)
	(c)1988 MIKI SYOUJI Co.,Ltd.

	Second Love
	(c)1986 Nihon Bussan Co.,Ltd.

	City Love
	(c)1986 Nihon Bussan Co.,Ltd.

	Seiha
	(c)1987 Nihon Bussan Co.,Ltd.

	Seiha (Medal Type)
	(c)1987 Nihon Bussan Co.,Ltd.

	Iemoto
	(c)1987 Nihon Bussan Co.,Ltd.

	Ojousan
	(c)1987 Nihon Bussan Co.,Ltd.

	Bijokko Yume Monogatari
	(c)1987 Nihon Bussan Co.,Ltd.

	Bijokko Gakuen
	(c)1988 Nihon Bussan Co.,Ltd.

	House Mannequin
	(c)1987 Nihon Bussan Co.,Ltd.

	House Mannequin Roppongi Live hen
	(c)1987 Nihon Bussan Co.,Ltd.

	Crystal Gal 2
	(c)1986 Nihon Bussan Co.,Ltd.

	Apparel Night
	(c)1986 Central Denshi.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/01/28 -

******************************************************************************/
/******************************************************************************
Memo:

- Animation in bijokkoy and bijokkog (while DAC playback) is not correct.
  Interrupt problem?

- Sampling rate of some DAC playback in bijokkoy and bijokkog is too high.
  Interrupt problem?

- LCD driver (HD61830B) is not emulated.

- You cannot set to LCD mode (it's not implemented yet).
  Housemnq is LCD mode only, so you cannot play it.

- Input handling is wrong in crystal2.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "machine/nb1413m3.h"


#define	SIGNED_DAC	0		// 0:unsigned DAC, 1:signed DAC


void mjsikaku_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
void seiha_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
void crystal2_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
void mjsikaku_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int mjsikaku_vh_start(void);
int secolove_vh_start(void);
int bijokkoy_vh_start(void);
int seiha_vh_start(void);
int crystal2_vh_start(void);
void mjsikaku_vh_stop(void);

WRITE_HANDLER( mjsikaku_palette_w );
WRITE_HANDLER( secolove_palette_w );
void mjsikaku_radrx_w(int data);
void mjsikaku_radry_w(int data);
void mjsikaku_sizex_w(int data);
void mjsikaku_sizey_w(int data);
void mjsikaku_gfxflag1_w(int data);
void mjsikaku_gfxflag2_w(int data);
void mjsikaku_gfxflag3_w(int data);
void mjsikaku_drawx_w(int data);
void mjsikaku_drawy_w(int data);
void mjsikaku_scrolly_w(int data);
void mjsikaku_romsel_w(int data);
void secolove_romsel_w(int data);
void iemoto_romsel_w(int data);
void seiha_romsel_w(int data);
void crystal2_romsel_w(int data);


static void init_mjsikaku(void)
{
	nb1413m3_type = NB1413M3_MJSIKAKU;
	nb1413m3_int_count = 144;
}

static void init_otonano(void)
{
	nb1413m3_type = NB1413M3_OTONANO;
	nb1413m3_int_count = 144;
}

static void init_mjcamera(void)
{
#if 1
	unsigned char *ROM = memory_region(REGION_CPU1);

	// Protection ROM check skip
	ROM[0x013d] = 0x00;
	ROM[0x013e] = 0x00;
	ROM[0x013f] = 0x00;
#endif
	nb1413m3_type = NB1413M3_MJCAMERA;
	nb1413m3_int_count = 144;
}

static void init_kaguya(void)
{
	nb1413m3_type = NB1413M3_KAGUYA;
	nb1413m3_int_count = 128;
}

static void init_secolove(void)
{
	nb1413m3_type = NB1413M3_SECOLOVE;
	nb1413m3_int_count = 128;
}

static void init_citylove(void)
{
	nb1413m3_type = NB1413M3_CITYLOVE;
	nb1413m3_int_count = 128;
}

static void init_seiha(void)
{
	nb1413m3_type = NB1413M3_SEIHA;
	nb1413m3_int_count = 128;
}

static void init_seiham(void)
{
	nb1413m3_type = NB1413M3_SEIHAM;
	nb1413m3_int_count = 128;
}

static void init_iemoto(void)
{
	nb1413m3_type = NB1413M3_IEMOTO;
	nb1413m3_int_count = 128;
}

static void init_ojousan(void)
{
	nb1413m3_type = NB1413M3_OJOUSAN;
	nb1413m3_int_count = 128;
}

static void init_bijokkoy(void)
{
	nb1413m3_type = NB1413M3_BIJOKKOY;
	nb1413m3_int_count = 128;
}

static void init_bijokkog(void)
{
	nb1413m3_type = NB1413M3_BIJOKKOG;
	nb1413m3_int_count = 128;
}

static void init_housemnq(void)
{
	nb1413m3_type = NB1413M3_HOUSEMNQ;
	nb1413m3_int_count = 128;
}

static void init_housemn2(void)
{
	nb1413m3_type = NB1413M3_HOUSEMN2;
	nb1413m3_int_count = 128;
}

static void init_crystal2(void)
{
	nb1413m3_type = NB1413M3_CRYSTAL2;
	nb1413m3_int_count = 96;
}

static void init_apparel(void)
{
	nb1413m3_type = NB1413M3_APPAREL;
	nb1413m3_int_count = 128;
}


static MEMORY_READ_START( readmem_mjsikaku )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mjsikaku )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END

static MEMORY_READ_START( readmem_secolove )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_secolove )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END

static MEMORY_READ_START( readmem_ojousan )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_ojousan )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static READ_HANDLER( io_mjsikaku_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_mjsikaku )
	{ 0x0000, 0xffff, io_mjsikaku_r },
PORT_END

static WRITE_HANDLER( io_mjsikaku_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x2000 <= offset) && (0x4000 > offset))
	{
		mjsikaku_palette_w(((offset & 0x1f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x1000:	nb1413m3_sndrombank2_w(data); break;
		case	0x5000:	mjsikaku_romsel_w(data); break;
		case	0x6000:	mjsikaku_radrx_w(data); break;
		case	0x6100:	mjsikaku_radry_w(data); break;
		case	0x6200:	mjsikaku_drawx_w(data); break;
		case	0x6300:	mjsikaku_drawy_w(data); break;
		case	0x6400:	mjsikaku_sizex_w(data); break;
		case	0x6500:	mjsikaku_sizey_w(data); break;
		case	0x6600:	mjsikaku_gfxflag1_w(data); break;
		case	0x6700:	break;
		case	0x8000:	YM3812_control_port_0_w(0, data); break;
		case	0x8100:	YM3812_write_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
		case	0xc000:	break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	mjsikaku_gfxflag2_w(data); break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_mjsikaku )
	{ 0x0000, 0xffff, io_mjsikaku_w },
PORT_END

static READ_HANDLER( io_otonano_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8000:	return YM3812_status_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_otonano )
	{ 0x0000, 0xffff, io_otonano_r },
PORT_END

static WRITE_HANDLER( io_otonano_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x2000 <= offset) && (0x4000 > offset))
	{
		mjsikaku_palette_w(((offset & 0x1f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x5000:	mjsikaku_romsel_w(data); break;
		case	0x7000:	mjsikaku_radrx_w(data); break;
		case	0x7100:	mjsikaku_radry_w(data); break;
		case	0x7200:	mjsikaku_drawx_w(data); break;
		case	0x7300:	mjsikaku_drawy_w(data); break;
		case	0x7400:	mjsikaku_sizex_w(data); break;
		case	0x7500:	mjsikaku_sizey_w(data); break;
		case	0x7600:	mjsikaku_gfxflag1_w(data); break;
		case	0x7700:	break;
		case	0x8000:	YM3812_control_port_0_w(0, data); break;
		case	0x8100:	YM3812_write_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
		case	0xc000:	break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	mjsikaku_gfxflag2_w(data); break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_otonano )
	{ 0x0000, 0xffff, io_otonano_w },
PORT_END

static READ_HANDLER( io_kaguya_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_kaguya )
	{ 0x0000, 0xffff, io_kaguya_r },
PORT_END

static WRITE_HANDLER( io_kaguya_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x2000 <= offset) && (0x4000 > offset))
	{
		mjsikaku_palette_w(((offset & 0x1f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x5000:	mjsikaku_romsel_w(data); break;
		case	0x7000:	mjsikaku_radrx_w(data); break;
		case	0x7100:	mjsikaku_radry_w(data); break;
		case	0x7200:	mjsikaku_drawx_w(data); break;
		case	0x7300:	mjsikaku_drawy_w(data); break;
		case	0x7400:	mjsikaku_sizex_w(data); break;
		case	0x7500:	mjsikaku_sizey_w(data); break;
		case	0x7600:	mjsikaku_gfxflag1_w(data); break;
		case	0x7700:	break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
		case	0xc000:	break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	mjsikaku_gfxflag2_w(data); break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_kaguya )
	{ 0x0000, 0xffff, io_kaguya_w },
PORT_END

static READ_HANDLER( io_secolove_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_secolove )
	{ 0x0000, 0xffff, io_secolove_r },
PORT_END

static WRITE_HANDLER( io_secolove_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		secolove_palette_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	mjsikaku_radrx_w(data); break;
		case	0x9100:	mjsikaku_radry_w(data); break;
		case	0x9200:	mjsikaku_drawx_w(data); break;
		case	0x9300:	mjsikaku_drawy_w(data); break;
		case	0x9400:	mjsikaku_sizex_w(data); break;
		case	0x9500:	mjsikaku_sizey_w(data); break;
		case	0x9600:	mjsikaku_gfxflag1_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	secolove_romsel_w(data);
				mjsikaku_gfxflag2_w(data);
				break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_secolove )
	{ 0x0000, 0xffff, io_secolove_w },
PORT_END

static WRITE_HANDLER( io_iemoto_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x2000 <= offset) && (0x4000 > offset))
	{
		mjsikaku_palette_w(((offset & 0x1f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x1000:	nb1413m3_sndrombank2_w(data); break;
		case	0x4000:	mjsikaku_radrx_w(data); break;
		case	0x4100:	mjsikaku_radry_w(data); break;
		case	0x4200:	mjsikaku_drawx_w(data); break;
		case	0x4300:	mjsikaku_drawy_w(data); break;
		case	0x4400:	mjsikaku_sizex_w(data); break;
		case	0x4500:	mjsikaku_sizey_w(data); break;
		case	0x4600:	mjsikaku_gfxflag1_w(data); break;
		case	0x4700:	break;
		case	0x5000:	iemoto_romsel_w(data);
				mjsikaku_gfxflag3_w(data);
				break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	mjsikaku_gfxflag2_w(data); break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_iemoto )
	{ 0x0000, 0xffff, io_iemoto_w },
PORT_END

static WRITE_HANDLER( io_seiha_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x2000 <= offset) && (0x4000 > offset))
	{
		mjsikaku_palette_w(((offset & 0x1f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x1000:	nb1413m3_sndrombank2_w(data); break;
		case	0x5000:	seiha_romsel_w(data);
				mjsikaku_gfxflag3_w(data);
				break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	mjsikaku_radrx_w(data); break;
		case	0x9100:	mjsikaku_radry_w(data); break;
		case	0x9200:	mjsikaku_drawx_w(data); break;
		case	0x9300:	mjsikaku_drawy_w(data); break;
		case	0x9400:	mjsikaku_sizex_w(data); break;
		case	0x9500:	mjsikaku_sizey_w(data); break;
		case	0x9600:	mjsikaku_gfxflag1_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_outcoin_w(data);
				nb1413m3_sndrombank1_w(data);
				break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	mjsikaku_gfxflag2_w(data); break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_seiha )
	{ 0x0000, 0xffff, io_seiha_w },
PORT_END

static WRITE_HANDLER( io_crystal2_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		secolove_palette_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	mjsikaku_radrx_w(data); break;
		case	0x9100:	mjsikaku_radry_w(data); break;
		case	0x9200:	mjsikaku_drawx_w(data); break;
		case	0x9300:	mjsikaku_drawy_w(data); break;
		case	0x9400:	mjsikaku_sizex_w(data); break;
		case	0x9500:	mjsikaku_sizey_w(data); break;
		case	0x9600:	mjsikaku_gfxflag1_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	crystal2_romsel_w(data);
				mjsikaku_gfxflag2_w(data);
				break;
		case	0xf000:	break;
	}
}

static PORT_WRITE_START( writeport_crystal2 )
	{ 0x0000, 0xffff, io_crystal2_w },
PORT_END

static WRITE_HANDLER( io_bijokkoy_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		secolove_palette_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	nb1413m3_nmi_clock_w(data); break;
		case	0x4200:	break;		// HD61830B LCD control ?
		case	0x4300:	break;		// 		"
		case	0x4400:	break;		// 		"
		case	0x4500:	break;		// 		"
		case	0x4600:	break;		// 		"
		case	0x4700:	break;		// 		"
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	mjsikaku_radrx_w(data); break;
		case	0x9100:	mjsikaku_radry_w(data); break;
		case	0x9200:	mjsikaku_drawx_w(data); break;
		case	0x9300:	mjsikaku_drawy_w(data); break;
		case	0x9400:	mjsikaku_sizex_w(data); break;
		case	0x9500:	mjsikaku_sizey_w(data); break;
		case	0x9600:	mjsikaku_gfxflag1_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	secolove_romsel_w(data);
				mjsikaku_gfxflag2_w(data);
				break;
		case	0xf000:	mjsikaku_scrolly_w(data); break;
	}
}

static PORT_WRITE_START( writeport_bijokkoy )
	{ 0x0000, 0xffff, io_bijokkoy_w },
PORT_END


INPUT_PORTS_START( mjsikaku )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( otonano )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "TSUMIPAI ENCHOU" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Last chance needs 1,000points" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x40, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Play fee display" )
	PORT_DIPSETTING(    0x80, "100 Yen" )
	PORT_DIPSETTING(    0x00, "50 Yen" )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "Character Display Test" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( mjcamera )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Character Display Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( kaguya )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	// NOTE:Coins counted by pressing service switch
	PORT_DIPNAME( 0x04, 0x04, "NOTE" )
	PORT_DIPSETTING(    0x04, "Coin x5" )
	PORT_DIPSETTING(    0x00, "Coin x10" )
	PORT_DIPNAME( 0x18, 0x18, "Game Out" )
	PORT_DIPSETTING(    0x18, "90% (Easy)" )
	PORT_DIPSETTING(    0x10, "80%" )
	PORT_DIPSETTING(    0x08, "70%" )
	PORT_DIPSETTING(    0x00, "60% (Hard)" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus awarded on" )
	PORT_DIPSETTING(    0x20, "[over BAIMAN]" )
	PORT_DIPSETTING(    0x00, "[BAIMAN]" )
	PORT_DIPNAME( 0x40, 0x40, "Variability of payout rate" )
	PORT_DIPSETTING(    0x40, "[big]" )
	PORT_DIPSETTING(    0x00, "[small]" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, "Nudity graphic on bet" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, "Bet Min" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x18, 0x00, "Number of extend TSUMO" )
	PORT_DIPSETTING(    0x18, "0" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x20, "Extend TSUMO needs credit" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( secolove )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x00, "Hanahai" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Cancel Hand" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Wareme" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( citylove )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0f, "1 (Easy)" )
	PORT_DIPSETTING(    0x0e, "2" )
	PORT_DIPSETTING(    0x0d, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0b, "5" )
	PORT_DIPSETTING(    0x0a, "6" )
	PORT_DIPSETTING(    0x09, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPSETTING(    0x06, "10" )
	PORT_DIPSETTING(    0x05, "11" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x03, "13" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPSETTING(    0x01, "15" )
	PORT_DIPSETTING(    0x00, "16 (Hard)" )
	PORT_DIPNAME( 0x30, 0x30, "YAKUMAN cut" )
	PORT_DIPSETTING(    0x30, "10%" )
	PORT_DIPSETTING(    0x20, "30%" )
	PORT_DIPSETTING(    0x10, "50%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x40, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "Hanahai" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Chonbo" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Open Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, "Cansel Type" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, "TSUMO 3" )
	PORT_DIPSETTING(    0x40, "TSUMO 7" )
	PORT_DIPSETTING(    0x00, "HAIPAI" )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( seiha )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Hard)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Easy)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Change Pai and Mat Color" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( seiham )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, "Game Out" )
	PORT_DIPSETTING(    0x07, "60% (Hard)" )
	PORT_DIPSETTING(    0x06, "65%" )
	PORT_DIPSETTING(    0x05, "70%" )
	PORT_DIPSETTING(    0x04, "75%" )
	PORT_DIPSETTING(    0x03, "80%" )
	PORT_DIPSETTING(    0x02, "85%" )
	PORT_DIPSETTING(    0x01, "90%" )
	PORT_DIPSETTING(    0x00, "95% (Easy)" )
	PORT_DIPNAME( 0x18, 0x18, "Rate Min" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x00, "Rate Max" )
	PORT_DIPSETTING(    0x60, "8" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x20, "12" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Score Pool" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, "Rate Up" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Last Chance" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Character Display Test" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( iemoto )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Hard)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Easy)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( bijokkoy )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "2P Simultaneous Play (LCD req'd)" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "See non-Reacher's hand" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Character Display Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Double Tsumo" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Chonbo" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( bijokkog )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "2P Simultaneous Play (LCD req'd)" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "See non-Reacher's hand" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bonus Score for Extra Credit" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Double Tsumo" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Chonbo" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Cancel Hand" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( housemnq )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Chonbo" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "See CPU's hand" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Time" )
	PORT_DIPSETTING(    0x03, "120" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x01, "80" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Speed" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Coin Selector" )
	PORT_DIPSETTING(    0x20, "common" )
	PORT_DIPSETTING(    0x00, "separate" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( housemn2 )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Chonbo" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "See CPU's hand" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Time" )
	PORT_DIPSETTING(    0x03, "120" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x01, "80" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Speed" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Coin Selector" )
	PORT_DIPSETTING(    0x20, "common" )
	PORT_DIPSETTING(    0x00, "separate" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( ojousan )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( crystal2 )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x0f, 0x0d, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0d, "1 (Easy)" )
	PORT_DIPSETTING(    0x0a, "2" )
	PORT_DIPSETTING(    0x09, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x07, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x05, "7" )
	PORT_DIPSETTING(    0x04, "8" )
	PORT_DIPSETTING(    0x00, "9 (Hard)" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x0c, 0x00, "SANGEN Rush" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "infinite" )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )		// OPTION (?)

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( apparel )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END


static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip */
	20000000/8,			/* 2.50 MHz */
	{ 35 }
};

static struct AY8910interface ay8910_interface =
{
	1,				/* 1 chip */
	1250000,			/* 1.25 MHz ?? */
	{ 35 },
	{ input_port_0_r },		// DIPSW-A read
	{ input_port_1_r },		// DIPSW-B read
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,				/* 1 channels */
	{ 50 }
};


#define NBMJDRV1( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			20000000/4,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	4096, 4096, \
	mjsikaku_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	mjsikaku_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
\
	/* sound hardware */ \
	0, 0, 0, 0, \
	{ \
		{ \
			SOUND_YM3812, \
			&ym3812_interface \
		}, \
		{ \
			SOUND_DAC, \
			&dac_interface \
		} \
	}, \
	##_nvram_ \
};

#define NBMJDRV2( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			10000000/2,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	4096, 4096, \
	mjsikaku_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	mjsikaku_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
\
	/* sound hardware */ \
	0, 0, 0, 0, \
	{ \
		{ \
			SOUND_YM3812, \
			&ym3812_interface \
		}, \
		{ \
			SOUND_DAC, \
			&dac_interface \
		} \
	}, \
	##_nvram_ \
};

#define NBMJDRV3( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			5000000/1,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	65536, 65536, \
	seiha_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	bijokkoy_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
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
	##_nvram_ \
};

#define NBMJDRV4( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			5000000/1,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	65536, 65536, \
	seiha_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	seiha_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
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
	##_nvram_ \
};

#define NBMJDRV5( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			5000000/1,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	256, 256, \
	crystal2_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	crystal2_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
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
	##_nvram_ \
};

#define NBMJDRV6( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			10000000/2,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, ##_intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	4096, 4096, \
	mjsikaku_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	mjsikaku_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
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
	##_nvram_ \
};

#define NBMJDRV7( _name_, _intcnt_, _mrmem_, _mwmem_, _mrport_, _mwport_, _nvram_ ) \
static struct MachineDriver machine_driver_##_name_ = \
{ \
	{ \
		{ \
			CPU_Z80 | CPU_16BIT_PORT, \
			5000000/1,		/* 5.00 MHz ? */ \
			readmem_##_mrmem_, writemem_##_mwmem_, readport_##_mrport_, writeport_##_mwport_, \
			nb1413m3_interrupt, _intcnt_ \
		} \
	}, \
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, \
	1, \
	nb1413m3_init_machine, \
\
	/* video hardware */ \
	512, 256, { 0, 512-1, 15, 239-1 }, \
	0, \
	4096, 4096, \
	mjsikaku_init_palette, \
\
	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2, \
	0, \
	secolove_vh_start, \
	mjsikaku_vh_stop, \
	mjsikaku_vh_screenrefresh, \
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


//	      NAME, INT,  MAIN_RM,  MAIN_WM,  MAIN_RP,  MAIN_WP, NV_RAM
NBMJDRV1( mjsikaku, 144, mjsikaku, mjsikaku, mjsikaku, mjsikaku, nb1413m3_nvram_handler )
NBMJDRV2(  otonano, 144, mjsikaku, mjsikaku,  otonano,  otonano, nb1413m3_nvram_handler )
NBMJDRV2( mjcamera, 144, mjsikaku, mjsikaku,  otonano,  otonano, nb1413m3_nvram_handler )
NBMJDRV7( secolove, 128, secolove, secolove, secolove, secolove, nb1413m3_nvram_handler )
NBMJDRV7( citylove, 128, secolove, secolove, secolove, secolove, nb1413m3_nvram_handler )
NBMJDRV3( bijokkoy, 128, secolove, secolove, secolove, bijokkoy, nb1413m3_nvram_handler )
NBMJDRV3( bijokkog, 128, secolove, secolove, secolove, bijokkoy, nb1413m3_nvram_handler )
NBMJDRV3( housemnq, 128, secolove, secolove, secolove, bijokkoy, nb1413m3_nvram_handler )
NBMJDRV3( housemn2, 128, secolove, secolove, secolove, bijokkoy, nb1413m3_nvram_handler )
NBMJDRV4(    seiha, 128, secolove, secolove, secolove,    seiha, nb1413m3_nvram_handler )
NBMJDRV4(   seiham, 128, secolove, secolove, secolove,    seiha, nb1413m3_nvram_handler )
NBMJDRV4(   iemoto, 128, secolove, secolove, secolove,   iemoto, nb1413m3_nvram_handler )
NBMJDRV4(  ojousan, 128,  ojousan,  ojousan, secolove,   iemoto, nb1413m3_nvram_handler )
NBMJDRV6(   kaguya, 128, mjsikaku, mjsikaku,   kaguya,   kaguya, nb1413m3_nvram_handler )
NBMJDRV5( crystal2,  96, secolove, secolove, secolove, crystal2, nb1413m3_nvram_handler )
NBMJDRV5(  apparel, 128, secolove, secolove, secolove, secolove, nb1413m3_nvram_handler )


ROM_START( mjsikaku )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mjsk_01.bin", 0x00000, 0x10000, 0x6b64c96a )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mjsk_02.bin", 0x00000, 0x10000, 0xcc0262bb )
	ROM_LOAD( "mjsk_03.bin", 0x10000, 0x10000, 0x7dedcd75 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mjsk_04.bin", 0x000000, 0x20000, 0x34d13d1e )
	ROM_LOAD( "mjsk_05.bin", 0x020000, 0x20000, 0x8c70aed5 )
	ROM_LOAD( "mjsk_06.bin", 0x040000, 0x20000, 0x1dad8355 )
	ROM_LOAD( "mjsk_07.bin", 0x060000, 0x20000, 0x8174a28a )
	ROM_LOAD( "mjsk_08.bin", 0x080000, 0x20000, 0x3e182aaa )
	ROM_LOAD( "mjsk_09.bin", 0x0a0000, 0x20000, 0xa17a3328 )
	ROM_LOAD( "mjsk_10.bin", 0x0c0000, 0x10000, 0xcab4909f )
	ROM_LOAD( "mjsk_11.bin", 0x0d0000, 0x10000, 0xdd7a95c8 )
	ROM_LOAD( "mjsk_12.bin", 0x0e0000, 0x10000, 0x20c25377 )
	ROM_LOAD( "mjsk_13.bin", 0x0f0000, 0x10000, 0x967e9a91 )
ROM_END

ROM_START( mjsikakb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "sikaku.1",    0x00000, 0x10000, 0x66349663 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mjsk_02.bin", 0x00000, 0x10000, 0xcc0262bb )
	ROM_LOAD( "mjsk_03.bin", 0x10000, 0x10000, 0x7dedcd75 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mjsk_04.bin", 0x000000, 0x20000, 0x34d13d1e )
	ROM_LOAD( "mjsk_05.bin", 0x020000, 0x20000, 0x8c70aed5 )
	ROM_LOAD( "mjsk_06.bin", 0x040000, 0x20000, 0x1dad8355 )
	ROM_LOAD( "mjsk_07.bin", 0x060000, 0x20000, 0x8174a28a )
	ROM_LOAD( "mjsk_08.bin", 0x080000, 0x20000, 0x3e182aaa )
	ROM_LOAD( "mjsk_09.bin", 0x0a0000, 0x20000, 0xa17a3328 )
	ROM_LOAD( "sikaku.10",   0x0c0000, 0x20000, 0xf91757bc )
	ROM_LOAD( "sikaku.11",   0x0e0000, 0x20000, 0xabd280b6 )
ROM_END

ROM_START( otonano )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "otona_01.bin", 0x00000, 0x10000, 0xee629b72 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "otona_02.bin", 0x00000, 0x10000, 0x2864b8ef )
	ROM_LOAD( "otona_03.bin", 0x10000, 0x10000, 0xece880e0 )
	ROM_LOAD( "otona_04.bin", 0x20000, 0x10000, 0x5a25b251 )
	ROM_LOAD( "otona_05.bin", 0x30000, 0x10000, 0x469d580d )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "otona_06.bin", 0x000000, 0x20000, 0x2d41f854 )
	ROM_LOAD( "otona_07.bin", 0x020000, 0x20000, 0x58d6717d )
	ROM_LOAD( "otona_08.bin", 0x040000, 0x20000, 0x40f8d432 )
	ROM_LOAD( "otona_09.bin", 0x060000, 0x20000, 0xfd80fdc2 )
	ROM_LOAD( "otona_10.bin", 0x080000, 0x20000, 0x50ff867a )
	ROM_LOAD( "otona_11.bin", 0x0a0000, 0x20000, 0xc467e822 )
	ROM_LOAD( "otona_12.bin", 0x0c0000, 0x20000, 0x1a0f9250 )
	ROM_LOAD( "otona_13.bin", 0x0e0000, 0x20000, 0x208dee43 )
ROM_END

ROM_START( mjcamera )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mcam_01.bin", 0x00000, 0x10000, 0x73d4b9ff )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mcam_02.bin", 0x00000, 0x10000, 0xfe8e975e )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mcam_03.bin", 0x000000, 0x10000, 0x273fb8bc )
	ROM_LOAD( "mcam_04.bin", 0x010000, 0x10000, 0x82995399 )
	ROM_LOAD( "mcam_05.bin", 0x020000, 0x10000, 0xa7c51d54 )
	ROM_LOAD( "mcam_06.bin", 0x030000, 0x10000, 0xf221700c )
	ROM_LOAD( "mcam_07.bin", 0x040000, 0x10000, 0x6baa4d45 )
	ROM_LOAD( "mcam_08.bin", 0x050000, 0x10000, 0x91d9c868 )
	ROM_LOAD( "mcam_09.bin", 0x060000, 0x10000, 0x56a35d4b )
	ROM_LOAD( "mcam_10.bin", 0x070000, 0x10000, 0x480e23c4 )
	ROM_LOAD( "mcam_11.bin", 0x080000, 0x10000, 0x2c29accc )
	ROM_LOAD( "mcam_12.bin", 0x090000, 0x10000, 0x902d73f8 )
	ROM_LOAD( "mcam_13.bin", 0x0a0000, 0x10000, 0xfcba0179 )
	ROM_LOAD( "mcam_14.bin", 0x0b0000, 0x10000, 0xee2c37a9 )
	ROM_LOAD( "mcam_15.bin", 0x0c0000, 0x10000, 0x90fd36f8 )
	ROM_LOAD( "mcam_16.bin", 0x0d0000, 0x10000, 0x41265f7f )
	ROM_LOAD( "mcam_17.bin", 0x0e0000, 0x10000, 0x78cef468 )
	ROM_LOAD( "mcam_18.bin", 0x0f0000, 0x10000, 0x3a3da341 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* protection data */
	ROM_LOAD( "mcam_m1.bin", 0x00000, 0x40000, 0xf85c5b07 )
ROM_END

ROM_START( secolove )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "slov_08.bin",  0x00000, 0x08000, 0x5aad556e )
	ROM_LOAD( "slov_07.bin",  0x08000, 0x08000, 0x94175129 )

	ROM_REGION( 0x18000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "slov_05.bin",  0x00000, 0x08000, 0xfa1debd9 )
	ROM_LOAD( "slov_06.bin",  0x08000, 0x08000, 0xa83be399 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "slov_01.bin",  0x000000, 0x10000, 0x9d792c34 )
	ROM_LOAD( "slov_02.bin",  0x010000, 0x10000, 0xb9671c88 )
	ROM_LOAD( "slov_03.bin",  0x020000, 0x10000, 0x5f57e4f2 )
	ROM_LOAD( "slov_04.bin",  0x030000, 0x10000, 0x4b0c700c )
	ROM_LOAD( "slov_c1.bin",  0x100000, 0x80000, 0x200170ba )
	ROM_LOAD( "slov_c2.bin",  0x180000, 0x80000, 0xdd5c23a1 )
ROM_END

ROM_START( citylove )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "14.12c", 0x00000, 0x08000, 0x2db5186c )
	ROM_LOAD( "13.11c", 0x08000, 0x08000, 0x52c7632b )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "11.8c",  0x00000, 0x08000, 0xeabb3f32 )
	ROM_LOAD( "12.10c", 0x08000, 0x08000, 0xc280f573 )

	ROM_REGION( 0xa0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1.1h",   0x00000, 0x10000, 0x55b911a3 )
	ROM_LOAD( "2.2h",   0x10000, 0x10000, 0x35298484 )
	ROM_LOAD( "3.4h",   0x20000, 0x10000, 0x6860c6d3 )
	ROM_LOAD( "4.5h",   0x30000, 0x10000, 0x21085a9a )
	ROM_LOAD( "5.7h",   0x40000, 0x10000, 0xfcf53e1a )
	ROM_LOAD( "6.1f",   0x50000, 0x10000, 0xdb11300c )
	ROM_LOAD( "7.2f",   0x60000, 0x10000, 0x57a90aac )
	ROM_LOAD( "8.4f",   0x70000, 0x10000, 0x58e1ad6f )
	ROM_LOAD( "9.5f",   0x80000, 0x10000, 0x242f07e9 )
	ROM_LOAD( "10.7f",  0x90000, 0x10000, 0xc032d8c3 )
ROM_END

ROM_START( apparel )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "11.bin", 0x00000, 0x04000, 0x31bd49d5 )
	ROM_LOAD( "12.bin", 0x04000, 0x04000, 0x56acd87d )
	ROM_LOAD( "13.bin", 0x08000, 0x04000, 0x3e2a9c66 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	// not used

	ROM_REGION( 0x0a0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1.bin",  0x000000, 0x10000, 0x6c7713ea )
	ROM_LOAD( "2.bin",  0x010000, 0x10000, 0x206f4d2c )
	ROM_LOAD( "3.bin",  0x020000, 0x10000, 0x5d8a732b )
	ROM_LOAD( "4.bin",  0x030000, 0x10000, 0xc40e4435 )
	ROM_LOAD( "5.bin",  0x040000, 0x10000, 0xe5bde704 )
	ROM_LOAD( "6.bin",  0x050000, 0x10000, 0x263673bc )
	ROM_LOAD( "7.bin",  0x060000, 0x10000, 0xc502dc5a )
	ROM_LOAD( "8.bin",  0x070000, 0x10000, 0xc0af5f0f )
	ROM_LOAD( "9.bin",  0x080000, 0x10000, 0x477b6cdd )
	ROM_LOAD( "10.bin", 0x090000, 0x10000, 0xd06d8972 )
ROM_END

ROM_START( seiha )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "seiha1.4g",  0x00000, 0x08000, 0xad5ba5b5 )
	ROM_LOAD( "seiha2.3g",  0x08000, 0x08000, 0x0fe7a4b8 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "seiha03.3i",  0x00000, 0x10000, 0x2bcf3d87 )
	ROM_LOAD( "seiha04.2i",  0x10000, 0x10000, 0x2fc905d0 )
	ROM_LOAD( "seiha05.1i",  0x20000, 0x10000, 0x8eace19c )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "seiha19.1a",  0x000000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "seiha20.2a",  0x040000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "seiha21.3a",  0x080000, 0x40000, 0xda46163e )
	ROM_LOAD( "seiha22.4a",  0x0c0000, 0x40000, 0xea2b78b3 )
	ROM_LOAD( "seiha23.5a",  0x100000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "seiha06.8a",  0x180000, 0x10000, 0x9fefe2ca )
	ROM_LOAD( "seiha07.9a",  0x190000, 0x10000, 0xa7d438ec )
	ROM_LOAD( "se1507.6a",   0x200000, 0x80000, 0xf1e9555e )
ROM_END

ROM_START( seiham )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "seih_01m.bin", 0x00000, 0x08000, 0x0c9a081b )
	ROM_LOAD( "seih_02m.bin", 0x08000, 0x08000, 0xa32cdb9a )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "seiha03.3i",   0x00000, 0x10000, 0x2bcf3d87 )
	ROM_LOAD( "seiha04.2i",   0x10000, 0x10000, 0x2fc905d0 )
	ROM_LOAD( "seiha05.1i",   0x20000, 0x10000, 0x8eace19c )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "seiha19.1a",   0x000000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "seiha20.2a",   0x040000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "seiha21.3a",   0x080000, 0x40000, 0xda46163e )
	ROM_LOAD( "seiha22.4a",   0x0c0000, 0x40000, 0xea2b78b3 )
	ROM_LOAD( "seiha23.5a",   0x100000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "seiha06.8a",   0x180000, 0x10000, 0x9fefe2ca )
	ROM_LOAD( "seiha07.9a",   0x190000, 0x10000, 0xa7d438ec )
	ROM_LOAD( "seih_08m.bin", 0x1a0000, 0x10000, 0xe8e61e48 )
	ROM_LOAD( "se1507.6a",    0x200000, 0x80000, 0xf1e9555e )
ROM_END

ROM_START( iemoto )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "iemoto1.4g",  0x00000, 0x08000, 0xab51f5c3 )
	ROM_LOAD( "iemoto2.3g",  0x08000, 0x08000, 0x873cd265 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "iemoto3.3i",  0x00000, 0x10000, 0x32d71ff9 )
	ROM_LOAD( "iemoto4.2i",  0x10000, 0x10000, 0x06f8e505 )
	ROM_LOAD( "iemoto5.1i",  0x20000, 0x10000, 0x261eb61a )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "iemoto31.1a", 0x000000, 0x40000, 0xba005a3a )
	ROM_LOAD( "iemoto32.2a", 0x040000, 0x40000, 0xfa9a74ae )
	ROM_LOAD( "iemoto33.3a", 0x080000, 0x40000, 0xefb13b61 )
	ROM_LOAD( "iemoto44.4a", 0x0c0000, 0x40000, 0x9acc54fa )
ROM_END

ROM_START( bijokkoy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",   0x00000, 0x08000, 0x7dec7ae1 )
	ROM_LOAD( "2.3c",   0x08000, 0x08000, 0x3ae9650f )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.ic1",  0x00000, 0x10000, 0x221743b1 )
	ROM_LOAD( "4.ic2",  0x10000, 0x10000, 0x9f1f4461 )
	ROM_LOAD( "5.ic3",  0x20000, 0x10000, 0x6e7b3024 )
	ROM_LOAD( "6.ic4",  0x30000, 0x10000, 0x5e912211 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1h.bin", 0x000000, 0x40000, 0xda56ccac )
	ROM_LOAD( "2h.bin", 0x040000, 0x40000, 0x21c0227a )
	ROM_LOAD( "3h.bin", 0x080000, 0x40000, 0xaa66d9f3 )
	ROM_LOAD( "4h.bin", 0x0c0000, 0x40000, 0x5d10fb0a )
	ROM_LOAD( "5h.bin", 0x100000, 0x40000, 0xe22d6ca8 )
ROM_END

ROM_START( bijokkog )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",    0x00000, 0x08000, 0x3c28b45c )
	ROM_LOAD( "2.3c",    0x08000, 0x08000, 0x396f6a05 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.ic1",   0x00000, 0x10000, 0xa92b1445 )
	ROM_LOAD( "4.ic2",   0x10000, 0x10000, 0x5127e958 )
	ROM_LOAD( "5.ic3",   0x20000, 0x10000, 0x6c717330 )
	ROM_LOAD( "6.ic4",   0x30000, 0x10000, 0xa3cf8d12 )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1s.bin",  0x000000, 0x10000, 0x9eadc3ea )
	ROM_LOAD( "2s.bin",  0x010000, 0x10000, 0x1161484c )
	ROM_LOAD( "3s.bin",  0x020000, 0x10000, 0x41f5dc43 )
	ROM_LOAD( "4s.bin",  0x030000, 0x10000, 0x3d9b79db )
	ROM_LOAD( "5s.bin",  0x040000, 0x10000, 0xeb54c3e3 )
	ROM_LOAD( "6s.bin",  0x050000, 0x10000, 0xd8deeea2 )
	ROM_LOAD( "7s.bin",  0x060000, 0x10000, 0xe42c67f1 )
	ROM_LOAD( "8s.bin",  0x070000, 0x10000, 0xcd11c78a )
	ROM_LOAD( "9s.bin",  0x080000, 0x10000, 0x2f3453a1 )
	ROM_LOAD( "10s.bin", 0x090000, 0x10000, 0xd80dd0b4 )
	ROM_LOAD( "11s.bin", 0x0a0000, 0x10000, 0xade64867 )
	ROM_LOAD( "12s.bin", 0x0b0000, 0x10000, 0x918a8f36 )
ROM_END

ROM_START( ojousan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4g",    0x00000, 0x08000, 0xc0166351 )
	ROM_LOAD( "2.3g",    0x08000, 0x08000, 0x2c264eb2 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mask.3i", 0x00000, 0x10000, 0x59f355eb )
	ROM_LOAD( "mask.2i", 0x10000, 0x10000, 0x6f750500 )
	ROM_LOAD( "mask.1i", 0x20000, 0x10000, 0x4babcb40 )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "3.5a",    0x000000, 0x20000, 0x3bdb9d2a )
	ROM_LOAD( "4.6a",    0x020000, 0x20000, 0x72b689b9 )
	ROM_LOAD( "5.7a",    0x040000, 0x20000, 0xe32e5e8a )
	ROM_LOAD( "6.8a",    0x060000, 0x20000, 0xf313337a )
	ROM_LOAD( "7.9a",    0x080000, 0x20000, 0xc2428e95 )
	ROM_LOAD( "8.10a",   0x0a0000, 0x20000, 0xf04c6003 )
ROM_END

ROM_START( housemnq )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",   0x00000, 0x08000, 0x465f61bb )
	ROM_LOAD( "2.3c",   0x08000, 0x08000, 0xe4499d02 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.5a",   0x00000, 0x10000, 0x141ce8b9 )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1i.bin", 0x000000, 0x40000, 0x2199e3e9 )
	ROM_LOAD( "2i.bin", 0x040000, 0x40000, 0xf730ea47 )
	ROM_LOAD( "3i.bin", 0x080000, 0x40000, 0xf85c5b07 )
	ROM_LOAD( "4i.bin", 0x0c0000, 0x40000, 0x88f33049 )
	ROM_LOAD( "5i.bin", 0x100000, 0x40000, 0x77ba1eaf )
ROM_END

ROM_START( housemn2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "hmq2_01.bin",  0x00000, 0x08000, 0xa5aaf6c8 )
	ROM_LOAD( "hmq2_02.bin",  0x08000, 0x08000, 0x6bdcc867 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "hmq2_03.bin",  0x00000, 0x10000, 0xc08081d8 )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "hmq2_c5.bin",  0x000000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "hmq2_c1.bin",  0x040000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "hmq2_c2.bin",  0x080000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "hmq2_c3.bin",  0x0c0000, 0x40000, 0xda46163e )
	ROM_LOAD( "hmq2_c4.bin",  0x100000, 0x40000, 0xea2b78b3 )
ROM_END

ROM_START( crystal2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "cgl2_01.bin",  0x00000, 0x04000, 0x67673350 )
	ROM_LOAD( "cgl2_02.bin",  0x04000, 0x04000, 0x79c599d8 )
	ROM_LOAD( "cgl2_03.bin",  0x08000, 0x04000, 0xc11987ed )
	ROM_LOAD( "cgl2_04.bin",  0x0c000, 0x04000, 0xae0b7df8 )

	ROM_REGION( 0x080000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "cgl2_01s.bin",  0x000000, 0x08000, 0x99b982ea )
	ROM_LOAD( "cgl2_01m.bin",  0x008000, 0x08000, 0x7c7a0416 )
	ROM_LOAD( "cgl2_02m.bin",  0x010000, 0x08000, 0x8511ddcd )
	ROM_LOAD( "cgl2_03m.bin",  0x018000, 0x08000, 0xf594e3bc )
	ROM_LOAD( "cgl2_04m.bin",  0x020000, 0x08000, 0x01a6bf99 )
	ROM_LOAD( "cgl2_05m.bin",  0x028000, 0x08000, 0xee941bf6 )
	ROM_LOAD( "cgl2_06m.bin",  0x030000, 0x08000, 0x93a8bf3b )
	ROM_LOAD( "cgl2_07m.bin",  0x038000, 0x08000, 0xb9626199 )
	ROM_LOAD( "cgl2_08m.bin",  0x040000, 0x08000, 0x8a4d02c9 )
	ROM_LOAD( "cgl2_09m.bin",  0x048000, 0x08000, 0xe0d58e86 )
	ROM_LOAD( "cgl2_02s.bin",  0x050000, 0x08000, 0x7e0ca2a5 )
	ROM_LOAD( "cgl2_03s.bin",  0x058000, 0x08000, 0x78fc9502 )
	ROM_LOAD( "cgl2_04s.bin",  0x060000, 0x08000, 0xc2140826 )
	ROM_LOAD( "cgl2_05s.bin",  0x068000, 0x08000, 0x257df5f3 )
	ROM_LOAD( "cgl2_06s.bin",  0x070000, 0x08000, 0x27da3e4d )
	ROM_LOAD( "cgl2_07s.bin",  0x078000, 0x08000, 0xbd202788 )
ROM_END

ROM_START( kaguya )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "kaguya01.bin", 0x00000, 0x10000, 0x6ac18c32 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "kaguya02.bin", 0x00000, 0x10000, 0x561dc656 )
	ROM_LOAD( "kaguya03.bin", 0x10000, 0x10000, 0xa09e9387 )

	ROM_REGION( 0x120000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "kaguya04.bin", 0x000000, 0x20000, 0xccd08d8d )
	ROM_LOAD( "kaguya05.bin", 0x020000, 0x20000, 0xa3abc686 )
	ROM_LOAD( "kaguya06.bin", 0x040000, 0x20000, 0x6accd6d3 )
	ROM_LOAD( "kaguya07.bin", 0x060000, 0x20000, 0x93c64846 )
	ROM_LOAD( "kaguya08.bin", 0x080000, 0x20000, 0xf0ad7c6c )
	ROM_LOAD( "kaguya09.bin", 0x0a0000, 0x20000, 0xf33fefdf )
	ROM_LOAD( "kaguya10.bin", 0x0c0000, 0x20000, 0x741d13f6 )
	ROM_LOAD( "kaguya11.bin", 0x0e0000, 0x20000, 0xfcbede4f )
ROM_END


//     YEAR,     NAME,   PARENT,  MACHINE,    INPUT,     INIT,    MONITOR, COMPANY, FULLNAME, FLAGS
GAMEX( 1988, mjsikaku,        0, mjsikaku, mjsikaku, mjsikaku, ROT0_16BIT, "Nichibutsu", "Mahjong Sikaku (Japan set 1)", 0 )
GAMEX( 1988, mjsikakb, mjsikaku, mjsikaku, mjsikaku, mjsikaku, ROT0_16BIT, "Nichibutsu", "Mahjong Sikaku (Japan set 2)", 0 )
GAMEX( 1988,  otonano,        0,  otonano,  otonano,  otonano, ROT0_16BIT, "Apple", "Otona no Mahjong (Japan)", 0 )
GAMEX( 1988, mjcamera,        0, mjcamera, mjcamera, mjcamera, ROT0_16BIT, "MIKI SYOUJI", "Mahjong Camera Kozou (Japan)", 0 )
GAMEX( 1986, secolove,        0, secolove, secolove, secolove, ROT0_16BIT, "Nichibutsu", "Second Love (Japan)", 0 )
GAMEX( 1986, citylove,        0, citylove, citylove, citylove, ROT0_16BIT, "Nichibutsu", "City Love (Japan)", 0 )
GAMEX( 1987,    seiha,        0,    seiha,    seiha,    seiha, ROT0_16BIT, "Nichibutsu", "Seiha (Japan)", 0 )
GAMEX( 1987,   seiham,    seiha,   seiham,   seiham,   seiham, ROT0_16BIT, "Nichibutsu", "Seiha [BET] (Japan)", 0 )
GAMEX( 1987,   iemoto,        0,   iemoto,   iemoto,   iemoto, ROT0_16BIT, "Nichibutsu", "Iemoto (Japan)", 0 )
GAMEX( 1987,  ojousan,        0,  ojousan,  ojousan,  ojousan, ROT0_16BIT, "Nichibutsu", "Ojousan (Japan)", 0 )
GAMEX( 1987, bijokkoy,        0, bijokkoy, bijokkoy, bijokkoy, ROT0_16BIT, "Nichibutsu", "Bijokko Yume Monogatari (Japan)", 0 )
GAMEX( 1988, bijokkog,        0, bijokkog, bijokkog, bijokkog, ROT0_16BIT, "Nichibutsu", "Bijokko Gakuen (Japan)", 0 )
GAMEX( 1987, housemnq,        0, housemnq, housemnq, housemnq, ROT0_16BIT, "Nichibutsu", "House Mannequin (Japan)", GAME_NOT_WORKING )
GAMEX( 1987, housemn2,        0, housemn2, housemn2, housemn2, ROT0_16BIT, "Nichibutsu", "House Mannequin Roppongi Live hen (Japan)", GAME_NOT_WORKING )
GAMEX( 1988,   kaguya,        0,   kaguya,   kaguya,   kaguya, ROT0_16BIT, "MIKI SYOUJI", "Mahjong Kaguyahime [BET] (Japan)", 0 )
GAMEX( 1986, crystal2,        0, crystal2, crystal2, crystal2,       ROT0, "Nichibutsu", "Crystal Gal 2 (Japan)", GAME_NOT_WORKING )
GAMEX( 1986,  apparel,        0,  apparel,  apparel,  apparel,       ROT0, "Central Denshi", "Apparel Night (Japan)", 0 )
