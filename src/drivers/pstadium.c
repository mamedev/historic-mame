/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Mahjong Triple Wars
	(c)1989 Nihon Bussan Co.,Ltd.

	Mahjong Panic Stadium
	(c)1990 Nihon Bussan Co.,Ltd.

	Mahjong Triple Wars 2
	(c)1990 Nihon Bussan Co.,Ltd.

	Mahjong Nerae! Top Star
	(c)1990 Nihon Bussan Co.,Ltd.

	Mahjong Jikken Love Story
	(c)1991 Nihon Bussan Co.,Ltd.

	Mahjong Vanilla Syndrome
	(c)1991 Nihon Bussan Co.,Ltd.

	Mahjong Final Bunny (Medal type)
	(c)1991 Nihon Bussan Co.,Ltd.

	Quiz-Mahjong Hayaku Yatteyo!
	(c)1991 Nihon Bussan Co.,Ltd.

	Mahjong Gal no Kokuhaku
	(c)1989 Nihon Bussan Co.,Ltd. / (c)1989 T.R.TEC

	Mahjong Hyouban Musume (Medal type)
	(c)1989 Nihon Bussan Co.,Ltd. / (c)1989 T.R.TEC

	Mahjong Gal no Kaika
	(c)1989 Nihon Bussan Co.,Ltd. / (c)1989 T.R.TEC

	Tokyo Gal Zukan
	(c)1989 Nihon Bussan Co.,Ltd.

	Tokimeki Bishoujo (Medal type)
	(c)1989 Nihon Bussan Co.,Ltd.

	Miss Mahjong Contest
	(c)1989 Nihon Bussan Co.,Ltd.

	Mahjong Uchuu yori Ai wo komete
	(c)1989 Nihon Bussan Co.,Ltd.

	AV2 Mahjong No.1 Bay Bridge no Seijo
	(c)1991 MIKI SYOUJI Co.,Ltd. / AV JAPAN Co.,Ltd.

	AV2 Mahjong No.2 Rouge no Kaori
	(c)1991 MIKI SYOUJI Co.,Ltd. / AV JAPAN Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/12/02 -

******************************************************************************/
/******************************************************************************
Memo:

- If "Game sound" is set to "OFF" in mjlstory, attract sound is not played
  even if "Attract sound" is set to "ON".

- The program of galkaika, tokyogal, and tokimbsj runs on Interrupt mode 2
  on real machine, but they don't run correctly in MAME so I changed to
  interrupt mode 1.

- Sound CPU of qmhayaku is running on 4MHz in real machine. But if I set
  it to 4MHz in MAME, sounds are not  played so I lowered the clock a bit.

- av2mj's VCR playback is not implemented.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


#define	SIGNED_DAC	0		// 0:unsigned DAC, 1:signed DAC


VIDEO_UPDATE( pstadium );
VIDEO_UPDATE( galkoku );
VIDEO_START( pstadium );

READ_HANDLER( pstadium_palette_r );
WRITE_HANDLER( pstadium_palette_w );
WRITE_HANDLER( galkoku_palette_w );
WRITE_HANDLER( galkaika_palette_w );
void pstadium_radrx_w(int data);
void pstadium_radry_w(int data);
void pstadium_sizex_w(int data);
void pstadium_sizey_w(int data);
void pstadium_gfxflag_w(int data);
void pstadium_gfxflag2_w(int data);
void pstadium_drawx_w(int data);
void pstadium_drawy_w(int data);
void pstadium_scrollx_w(int data);
void pstadium_scrolly_w(int data);
void pstadium_romsel_w(int data);
void pstadium_paltblnum_w(int data);
READ_HANDLER( pstadium_paltbl_r );
WRITE_HANDLER( pstadium_paltbl_w );


static WRITE_HANDLER( pstadium_soundbank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (!(data & 0x80)) soundlatch_clear_w(0, 0);
	cpu_setbank(1, &RAM[0x08000 + (0x8000 * (data & 0x03))]);
}

static WRITE_HANDLER( pstadium_sound_w )
{
	soundlatch_w(0, data);
}

static READ_HANDLER( pstadium_sound_r )
{
	int data;

	data = soundlatch_r(0);
	return data;
}

static DRIVER_INIT( pstadium )
{
	nb1413m3_type = NB1413M3_PSTADIUM;
}

static DRIVER_INIT( triplew1 )
{
	nb1413m3_type = NB1413M3_TRIPLEW1;
}

static DRIVER_INIT( triplew2 )
{
	nb1413m3_type = NB1413M3_TRIPLEW2;
}

static DRIVER_INIT( ntopstar )
{
	nb1413m3_type = NB1413M3_NTOPSTAR;
}

static DRIVER_INIT( mjlstory )
{
	nb1413m3_type = NB1413M3_MJLSTORY;
}

static DRIVER_INIT( vanilla )
{
	nb1413m3_type = NB1413M3_VANILLA;
}

static DRIVER_INIT( finalbny )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	int i;

	for (i = 0xf800; i < 0x10000; i++) ROM[i] = 0x00;

	nb1413m3_type = NB1413M3_FINALBNY;
}

static DRIVER_INIT( qmhayaku )
{
	nb1413m3_type = NB1413M3_QMHAYAKU;
}

static DRIVER_INIT( galkoku )
{
	nb1413m3_type = NB1413M3_GALKOKU;
}

static DRIVER_INIT( hyouban )
{
	nb1413m3_type = NB1413M3_HYOUBAN;
}

static DRIVER_INIT( galkaika )
{
#if 1
	unsigned char *ROM = memory_region(REGION_CPU1);

	// Patch to IM2 -> IM1
	ROM[0x0002] = 0x56;
#endif
	nb1413m3_type = NB1413M3_GALKAIKA;
}

static DRIVER_INIT( tokyogal )
{
#if 1
	unsigned char *ROM = memory_region(REGION_CPU1);

	// Patch to IM2 -> IM1
	ROM[0x0002] = 0x56;
#endif
	nb1413m3_type = NB1413M3_TOKYOGAL;
}

static DRIVER_INIT( tokimbsj )
{
#if 1
	unsigned char *ROM = memory_region(REGION_CPU1);

	// Patch to IM2 -> IM1
	ROM[0x0002] = 0x56;
#endif
	nb1413m3_type = NB1413M3_TOKIMBSJ;
}

static DRIVER_INIT( mcontest )
{
	nb1413m3_type = NB1413M3_MCONTEST;
}

static DRIVER_INIT( uchuuai )
{
	nb1413m3_type = NB1413M3_UCHUUAI;
}

static DRIVER_INIT( av2mj1bb )
{
	nb1413m3_type = NB1413M3_AV2MJ1BB;
}

static DRIVER_INIT( av2mj2rg )
{
	nb1413m3_type = NB1413M3_AV2MJ2RG;
}


static MEMORY_READ_START( readmem_pstadium )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_r },
	{ 0xf200, 0xf3ff, pstadium_palette_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_pstadium )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_w },
	{ 0xf200, 0xf3ff, pstadium_palette_w },
	{ 0xf800, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },	// finalbny
MEMORY_END

static MEMORY_READ_START( readmem_triplew1 )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_r },
	{ 0xf200, 0xf20f, pstadium_paltbl_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_triplew1 )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_w },
	{ 0xf200, 0xf20f, pstadium_paltbl_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_triplew2 )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_r },
	{ 0xf400, 0xf40f, pstadium_paltbl_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_triplew2 )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_w },
	{ 0xf400, 0xf40f, pstadium_paltbl_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_mjlstory )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf200, 0xf3ff, pstadium_palette_r },
	{ 0xf700, 0xf70f, pstadium_paltbl_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mjlstory )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf200, 0xf3ff, pstadium_palette_w },
	{ 0xf700, 0xf70f, pstadium_paltbl_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_galkoku )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_r },
	{ 0xf400, 0xf5ff, pstadium_palette_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_galkoku )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_w },
	{ 0xf400, 0xf5ff, galkoku_palette_w },
	{ 0xf800, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },	// hyouban
MEMORY_END

static MEMORY_READ_START( readmem_galkaika )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_r },
	{ 0xf400, 0xf5ff, pstadium_palette_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_galkaika )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_w },
	{ 0xf400, 0xf5ff, galkaika_palette_w },
	{ 0xf800, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },	// tokimbsj
MEMORY_END

static MEMORY_READ_START( readmem_tokyogal )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_r },
	{ 0xf400, 0xf40f, pstadium_paltbl_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_tokyogal )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf1ff, galkaika_palette_w },
	{ 0xf400, 0xf40f, pstadium_paltbl_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_av2mj1bb )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_r },
	{ 0xf500, 0xf50f, pstadium_paltbl_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_av2mj1bb )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf1ff, pstadium_palette_w },
	{ 0xf500, 0xf50f, pstadium_paltbl_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_av2mj2rg )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_r },
	{ 0xf200, 0xf3ff, pstadium_palette_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_av2mj2rg )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf00f, pstadium_paltbl_w },
	{ 0xf200, 0xf3ff, pstadium_palette_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static READ_HANDLER( io_pstadium_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xc000:	return nb1413m3_inputport3_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf800:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_pstadium )
	{ 0x0000, 0xffff, io_pstadium_r },
PORT_END

static WRITE_HANDLER( io_pstadium_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x0000:	pstadium_radrx_w(data); break;
		case	0x0100:	pstadium_radry_w(data); break;
		case	0x0200:	break;
		case	0x0300:	break;
		case	0x0400:	pstadium_sizex_w(data); break;
		case	0x0500:	pstadium_sizey_w(data); break;
		case	0x0600:	pstadium_gfxflag_w(data); break;
		case	0x0700:	break;
		case	0x1000:	pstadium_drawx_w(data); break;
		case	0x2000:	pstadium_drawy_w(data); break;
		case	0x3000:	pstadium_scrollx_w(data); break;
		case	0x4000:	pstadium_scrolly_w(data); break;
		case	0x5000:	pstadium_gfxflag2_w(data); break;
		case	0x6000:	pstadium_romsel_w(data); break;
		case	0x7000:	pstadium_paltblnum_w(data); break;
		case	0x8000:	pstadium_sound_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	break;
		case	0xd000:	break;
		case	0xf000:	nb1413m3_outcoin_w(data); break;
	}
}

static PORT_WRITE_START( writeport_pstadium )
	{ 0x0000, 0xffff, io_pstadium_w },
PORT_END

static WRITE_HANDLER( io_av2mj1bb_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x0000:	pstadium_radrx_w(data); break;
		case	0x0100:	pstadium_radry_w(data); break;
		case	0x0200:	break;
		case	0x0300:	break;
		case	0x0400:	pstadium_sizex_w(data); break;
		case	0x0500:	pstadium_sizey_w(data); break;
		case	0x0600:	pstadium_gfxflag_w(data); break;
		case	0x0700:	break;
		case	0x1000:	pstadium_drawx_w(data); break;
		case	0x2000:	pstadium_drawy_w(data); break;
		case	0x3000:	pstadium_scrollx_w(data); break;
		case	0x4000:	pstadium_scrolly_w(data); break;
		case	0x5000:	pstadium_gfxflag2_w(data); break;
		case	0x6000:	pstadium_romsel_w(data); break;
		case	0x7000:	pstadium_paltblnum_w(data); break;
		case	0x8000:	pstadium_sound_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_vcrctrl_w(data); break;
		case	0xd000:	break;
		case	0xf000:	break;
	}
}

static PORT_WRITE_START( writeport_av2mj1bb )
	{ 0x0000, 0xffff, io_av2mj1bb_w },
PORT_END

static READ_HANDLER( io_galkoku_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xc000:	return nb1413m3_inputport3_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_galkoku )
	{ 0x0000, 0xffff, io_galkoku_r },
PORT_END

static WRITE_HANDLER( io_galkoku_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x0000:	pstadium_radrx_w(data); break;
		case	0x0100:	pstadium_radry_w(data); break;
		case	0x0200:	break;
		case	0x0300:	break;
		case	0x0400:	pstadium_sizex_w(data); break;
		case	0x0500:	pstadium_sizey_w(data); break;
		case	0x0600:	pstadium_gfxflag_w(data); break;
		case	0x0700:	break;
		case	0x1000:	pstadium_drawx_w(data); break;
		case	0x2000:	pstadium_drawy_w(data); break;
		case	0x3000:	pstadium_scrollx_w(data); break;
		case	0x4000:	pstadium_scrolly_w(data); break;
		case	0x5000:	pstadium_gfxflag2_w(data); break;
		case	0x6000:	pstadium_romsel_w(data); break;
		case	0x7000:	pstadium_paltblnum_w(data); break;
		case	0x8000:	YM3812_control_port_0_w(0, data); break;
		case	0x8100:	YM3812_write_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
		case	0xc000:	nb1413m3_nmi_clock_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	break;
		case	0xf000:	nb1413m3_outcoin_w(data); break;
	}
}

static PORT_WRITE_START( writeport_galkoku )
	{ 0x0000, 0xffff, io_galkoku_w },
PORT_END

static READ_HANDLER( io_hyouban_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xc000:	return nb1413m3_inputport3_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_hyouban )
	{ 0x0000, 0xffff, io_hyouban_r },
PORT_END

static WRITE_HANDLER( io_hyouban_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x0000:	pstadium_radrx_w(data); break;
		case	0x0100:	pstadium_radry_w(data); break;
		case	0x0200:	break;
		case	0x0300:	break;
		case	0x0400:	pstadium_sizex_w(data); break;
		case	0x0500:	pstadium_sizey_w(data); break;
		case	0x0600:	pstadium_gfxflag_w(data); break;
		case	0x0700:	break;
		case	0x1000:	pstadium_drawx_w(data); break;
		case	0x2000:	pstadium_drawy_w(data); break;
		case	0x3000:	pstadium_scrollx_w(data); break;
		case	0x4000:	pstadium_scrolly_w(data); break;
		case	0x5000:	pstadium_gfxflag2_w(data); break;
		case	0x6000:	pstadium_romsel_w(data); break;
		case	0x7000:	pstadium_paltblnum_w(data); break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
		case	0xc000:	nb1413m3_nmi_clock_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	break;
		case	0xf000:	nb1413m3_outcoin_w(data); break;
	}
}

static PORT_WRITE_START( writeport_hyouban )
	{ 0x0000, 0xffff, io_hyouban_w },
PORT_END


static MEMORY_READ_START( sound_readmem_pstadium )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_pstadium )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport_pstadium )
	{ 0x00, 0x00, pstadium_sound_r },
PORT_END

static PORT_WRITE_START( sound_writeport_pstadium )
#if SIGNED_DAC
	{ 0x00, 0x00, DAC_0_signed_data_w },
	{ 0x02, 0x02, DAC_1_signed_data_w },
#else
	{ 0x00, 0x00, DAC_0_data_w },
	{ 0x02, 0x02, DAC_1_data_w },
#endif
	{ 0x04, 0x04, pstadium_soundbank_w },
	{ 0x06, 0x06, IOWP_NOP },
	{ 0x80, 0x80, YM3812_control_port_0_w },
	{ 0x81, 0x81, YM3812_write_port_0_w },
PORT_END


INPUT_PORTS_START( pstadium )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( triplew1 )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( ntopstar )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( mjlstory )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( vanilla )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( finalbny )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, "Game Out" )
	PORT_DIPSETTING(    0x07, "90% (Easy)" )
	PORT_DIPSETTING(    0x06, "85%" )
	PORT_DIPSETTING(    0x05, "80%" )
	PORT_DIPSETTING(    0x04, "75%" )
	PORT_DIPSETTING(    0x03, "70%" )
	PORT_DIPSETTING(    0x02, "65%" )
	PORT_DIPSETTING(    0x01, "60%" )
	PORT_DIPSETTING(    0x00, "55% (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "Last Chance" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Last chance needs 1credit" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bet1 Only" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Bet Min" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x00, "Bet Max" )
	PORT_DIPSETTING(    0x60, "8" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x20, "12" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Score Pool" )
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

INPUT_PORTS_START( qmhayaku )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( galkoku )
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
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

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

INPUT_PORTS_START( hyouban )

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
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

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

INPUT_PORTS_START( galkaika )

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
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode" )
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

INPUT_PORTS_START( tokyogal )

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
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
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

INPUT_PORTS_START( tokimbsj )

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
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Character Display Test" )
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

INPUT_PORTS_START( mcontest )

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
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
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

INPUT_PORTS_START( uchuuai )

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
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Character Display Test" )
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

INPUT_PORTS_START( av2mj1bb )
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
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0xc0, "Video Playback Time" )
	PORT_DIPSETTING(    0xc0, "Type-A" )
	PORT_DIPSETTING(    0x80, "Type-B" )
	PORT_DIPSETTING(    0x40, "Type-C" )
	PORT_DIPSETTING(    0x00, "Type-D" )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Attract mode" )
	PORT_DIPSETTING(    0x03, "No attract mode" )
	PORT_DIPSETTING(    0x02, "Once per 10min." )
	PORT_DIPSETTING(    0x01, "Once per 5min." )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( av2mj2rg )

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
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0xc0, "Video Playback Time" )
	PORT_DIPSETTING(    0xc0, "Type-A" )
	PORT_DIPSETTING(    0x80, "Type-B" )
	PORT_DIPSETTING(    0x40, "Type-C" )
	PORT_DIPSETTING(    0x00, "Type-D" )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Attract mode" )
	PORT_DIPSETTING(    0x03, "No attract mode" )
	PORT_DIPSETTING(    0x02, "Once per 10min." )
	PORT_DIPSETTING(    0x01, "Once per 5min." )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END


static struct YM3812interface pstadium_ym3812_interface =
{
	1,				/* 1 chip */
	25000000/6.25,			/* 4.00 MHz */
	{ 70 }
};

static struct YM3812interface galkoku_ym3812_interface =
{
	1,				/* 1 chip */
	25000000/10,			/* 2.50 MHz */
	{ 70 }
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

static struct DACinterface pstadium_dac_interface =
{
	2,				/* 2 channels */
	{ 50, 50 },
};

static struct DACinterface galkoku_dac_interface =
{
	1,				/* 1 channel */
	{ 50 },
};


static MACHINE_DRIVER_START( nbmjdrv1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 6000000/2)		/* 3.00 MHz */
	MDRV_CPU_MEMORY(readmem_pstadium, writemem_pstadium)
	MDRV_CPU_PORTS(readport_pstadium, writeport_pstadium)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,1)

	MDRV_CPU_ADD(Z80 | CPU_AUDIO_CPU, 3900000)		/* 4.00 MHz */
	MDRV_CPU_MEMORY(sound_readmem_pstadium,sound_writemem_pstadium)
	MDRV_CPU_PORTS(sound_readport_pstadium,sound_writeport_pstadium)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,128)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nb1413m3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(1024, 512)
	MDRV_VISIBLE_AREA(0, 638-1, 255, 495-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(pstadium)
	MDRV_VIDEO_UPDATE(pstadium)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, pstadium_ym3812_interface)
	MDRV_SOUND_ADD(DAC, pstadium_dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nbmjdrv2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 25000000/6.25)		/* 4.00 MHz ? */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem_galkoku, writemem_galkoku)
	MDRV_CPU_PORTS(readport_galkoku, writeport_galkoku)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nb1413m3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(1024, 512)
	MDRV_VISIBLE_AREA(0, 638-1, 255, 495-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(pstadium)
	MDRV_VIDEO_UPDATE(galkoku)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("3812", YM3812, galkoku_ym3812_interface)
	MDRV_SOUND_ADD_TAG("dac",  DAC, galkoku_dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nbmjdrv3 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)

	/* sound hardware */
	MDRV_SOUND_REPLACE("3812", AY8910, ay8910_interface)
MACHINE_DRIVER_END


// ---------------------------------------------------------------------

static MACHINE_DRIVER_START( pstadium )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( triplew1 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_triplew1,writemem_triplew1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( triplew2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_triplew2,writemem_triplew2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ntopstar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mjlstory )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_mjlstory,writemem_mjlstory)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vanilla )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( finalbny )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_NVRAM_HANDLER(nb1413m3)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( qmhayaku )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( galkoku )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hyouban )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv3)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(readport_hyouban,writeport_hyouban)

	MDRV_NVRAM_HANDLER(nb1413m3)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( galkaika )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_galkaika,writemem_galkaika)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tokyogal )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_tokyogal,writemem_tokyogal)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tokimbsj )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_galkaika,writemem_galkaika)

	MDRV_NVRAM_HANDLER(nb1413m3)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mcontest )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( uchuuai )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( av2mj1bb )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_av2mj1bb,writemem_av2mj1bb)
	MDRV_CPU_PORTS(readport_pstadium,writeport_av2mj1bb)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( av2mj2rg )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nbmjdrv1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_av2mj2rg,writemem_av2mj2rg)
	MDRV_CPU_PORTS(readport_pstadium,writeport_av2mj1bb)
MACHINE_DRIVER_END




ROM_START( pstadium )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "psdm_01.bin",  0x00000,  0x10000, 0x4af81589 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "psdm_03.bin",  0x00000,  0x10000, 0xac17cef2 )
	ROM_LOAD( "psdm_02.bin",  0x10000,  0x10000, 0xefefe881 )

	ROM_REGION( 0x110000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "psdm_04.bin",  0x000000, 0x10000, 0x01957a76 )
	ROM_LOAD( "psdm_05.bin",  0x010000, 0x10000, 0xf5dc1d20 )
	ROM_LOAD( "psdm_06.bin",  0x020000, 0x10000, 0x6fc89b50 )
	ROM_LOAD( "psdm_07.bin",  0x030000, 0x10000, 0xaec64ff4 )
	ROM_LOAD( "psdm_08.bin",  0x040000, 0x10000, 0xebeaf64a )
	ROM_LOAD( "psdm_09.bin",  0x050000, 0x10000, 0x854b2914 )
	ROM_LOAD( "psdm_10.bin",  0x060000, 0x10000, 0xeca5cd5a )
	ROM_LOAD( "psdm_11.bin",  0x070000, 0x10000, 0xa2de166d )
	ROM_LOAD( "psdm_12.bin",  0x080000, 0x10000, 0x2c99ec4d )
	ROM_LOAD( "psdm_13.bin",  0x090000, 0x10000, 0x77b99a6e )
	ROM_LOAD( "psdm_14.bin",  0x0a0000, 0x10000, 0xa3cf907b )
	ROM_LOAD( "psdm_15.bin",  0x0b0000, 0x10000, 0xb0da8d18 )
	ROM_LOAD( "psdm_16.bin",  0x0c0000, 0x10000, 0x9a2fd9c5 )
	ROM_LOAD( "psdm_17.bin",  0x0d0000, 0x10000, 0xe462d507 )
	ROM_LOAD( "psdm_18.bin",  0x0e0000, 0x10000, 0xe9ce8e02 )
	ROM_LOAD( "psdm_19.bin",  0x0f0000, 0x10000, 0xf23496c6 )
	ROM_LOAD( "psdm_20.bin",  0x100000, 0x10000, 0xc410ce4b )
ROM_END

ROM_START( triplew1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "tpw1_01.bin",  0x00000,  0x10000, 0x2542958a )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "tpw1_03.bin",  0x00000,  0x10000, 0xd86cc7d2 )
	ROM_LOAD( "tpw1_02.bin",  0x10000,  0x10000, 0x857656a7 )

	ROM_REGION( 0x160000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "tpw1_04.bin",  0x000000, 0x20000, 0xca26ccb3 )
	ROM_LOAD( "tpw1_05.bin",  0x020000, 0x20000, 0x26501af0 )
	ROM_LOAD( "tpw1_06.bin",  0x040000, 0x10000, 0x789bbacd )
	ROM_LOAD( "tpw1_07.bin",  0x050000, 0x10000, 0x38aaad61 )
	ROM_LOAD( "tpw1_08.bin",  0x060000, 0x10000, 0x9f4042b4 )
	ROM_LOAD( "tpw1_09.bin",  0x070000, 0x10000, 0x388a78b9 )
	ROM_LOAD( "tpw1_10.bin",  0x080000, 0x10000, 0x7a19730d )
	ROM_LOAD( "tpw1_11.bin",  0x090000, 0x10000, 0x1239a0c6 )
	ROM_LOAD( "tpw1_12.bin",  0x0a0000, 0x10000, 0xca469c52 )
	ROM_LOAD( "tpw1_13.bin",  0x0b0000, 0x10000, 0x0ca520c0 )
	ROM_LOAD( "tpw1_14.bin",  0x0c0000, 0x10000, 0x3880db99 )
	ROM_LOAD( "tpw1_15.bin",  0x0d0000, 0x10000, 0x996ea3e8 )
	ROM_LOAD( "tpw1_16.bin",  0x0e0000, 0x10000, 0x415ae47c )
	ROM_LOAD( "tpw1_17.bin",  0x0f0000, 0x10000, 0xb5c88f0e )
	ROM_LOAD( "tpw1_18.bin",  0x100000, 0x10000, 0xdef06191 )
	ROM_LOAD( "tpw1_19.bin",  0x110000, 0x10000, 0xb293561b )
	ROM_LOAD( "tpw1_20.bin",  0x120000, 0x10000, 0x81bfa331 )
	ROM_LOAD( "tpw1_21.bin",  0x130000, 0x10000, 0x2dbb68e5 )
	ROM_LOAD( "tpw1_22.bin",  0x140000, 0x10000, 0x9633278c )
	ROM_LOAD( "tpw1_23.bin",  0x150000, 0x10000, 0x11580513 )
ROM_END

ROM_START( triplew2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "tpw2_01.bin",  0x00000,  0x10000, 0x2637f19d )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "tpw2_03.bin",  0x00000,  0x10000, 0x8e7922c3 )
	ROM_LOAD( "tpw2_02.bin",  0x10000,  0x10000, 0x5339692d )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "tpw2_04.bin",  0x000000, 0x20000, 0xd4af2c04 )
	ROM_LOAD( "tpw2_05.bin",  0x020000, 0x20000, 0xfff198c8 )
	ROM_LOAD( "tpw2_06.bin",  0x040000, 0x20000, 0x4966b15b )
	ROM_LOAD( "tpw2_07.bin",  0x060000, 0x20000, 0xde1b8788 )
	ROM_LOAD( "tpw2_08.bin",  0x080000, 0x20000, 0xfb1b1ebc )
	ROM_LOAD( "tpw2_09.bin",  0x0a0000, 0x10000, 0xd40cacfd )
	ROM_LOAD( "tpw2_10.bin",  0x0b0000, 0x10000, 0x8fa96a92 )
	ROM_LOAD( "tpw2_11.bin",  0x0c0000, 0x10000, 0xa6a44edd )
	ROM_LOAD( "tpw2_12.bin",  0x0d0000, 0x10000, 0xd01a3a6a )
	ROM_LOAD( "tpw2_13.bin",  0x0e0000, 0x10000, 0x6b4ebd1f )
	ROM_LOAD( "tpw2_14.bin",  0x0f0000, 0x10000, 0x383d2735 )
	ROM_LOAD( "tpw2_15.bin",  0x100000, 0x10000, 0x682110f5 )
	ROM_LOAD( "tpw2_16.bin",  0x110000, 0x10000, 0x466eea24 )
	ROM_LOAD( "tpw2_17.bin",  0x120000, 0x10000, 0xa422ece3 )
	ROM_LOAD( "tpw2_18.bin",  0x130000, 0x10000, 0xf65b699d )
	ROM_LOAD( "tpw2_19.bin",  0x140000, 0x10000, 0x8356beac )
	ROM_LOAD( "tpw2_20.bin",  0x150000, 0x10000, 0x240c408e )
	ROM_LOAD( "mj_1802.bin",  0x180000, 0x80000, 0xe6213f10 )
ROM_END

ROM_START( ntopstar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "ntsr_01.bin",  0x00000,  0x10000, 0x3a4325f2 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "ntsr_03.bin",  0x00000,  0x10000, 0x747ba06a )
	ROM_LOAD( "ntsr_02.bin",  0x10000,  0x10000, 0x12334718 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "ntsr_04.bin",  0x000000, 0x20000, 0x06edf3a4 )
	ROM_LOAD( "ntsr_05.bin",  0x020000, 0x20000, 0xb3f014fa )
	ROM_LOAD( "ntsr_06.bin",  0x040000, 0x10000, 0x9333ebcb )
	ROM_LOAD( "ntsr_07.bin",  0x050000, 0x10000, 0x0948f999 )
	ROM_LOAD( "ntsr_08.bin",  0x060000, 0x10000, 0xabbd7494 )
	ROM_LOAD( "ntsr_09.bin",  0x070000, 0x10000, 0xdd84badd )
	ROM_LOAD( "ntsr_10.bin",  0x080000, 0x10000, 0x7083a505 )
	ROM_LOAD( "ntsr_11.bin",  0x090000, 0x10000, 0x45ed0f6d )
	ROM_LOAD( "ntsr_12.bin",  0x0a0000, 0x10000, 0x3d51ae82 )
	ROM_LOAD( "ntsr_13.bin",  0x0b0000, 0x10000, 0xeccde427 )
	ROM_LOAD( "ntsr_14.bin",  0x0c0000, 0x10000, 0xdd21bbfb )
	ROM_LOAD( "ntsr_15.bin",  0x0d0000, 0x10000, 0x5556024b )
	ROM_LOAD( "ntsr_16.bin",  0x0e0000, 0x10000, 0xf1273c7f )
	ROM_LOAD( "ntsr_17.bin",  0x0f0000, 0x10000, 0xd5574307 )
	ROM_LOAD( "ntsr_18.bin",  0x100000, 0x10000, 0x71566140 )
	ROM_LOAD( "ntsr_19.bin",  0x110000, 0x10000, 0x6c880b9d )
	ROM_LOAD( "ntsr_20.bin",  0x120000, 0x10000, 0x4b832d37 )
	ROM_LOAD( "ntsr_21.bin",  0x130000, 0x10000, 0x133183db )
ROM_END

ROM_START( mjlstory )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "mjls_01.bin",  0x00000,  0x10000, 0xa9febe8b )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "mjls_03.bin",  0x00000,  0x10000, 0x15e54af0 )
	ROM_LOAD( "mjls_02.bin",  0x10000,  0x10000, 0xda976e4f )

	ROM_REGION( 0x190000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mjls_04.bin",  0x000000, 0x20000, 0xd3e642ee )
	ROM_LOAD( "mjls_05.bin",  0x020000, 0x20000, 0xdc888639 )
	ROM_LOAD( "mjls_06.bin",  0x040000, 0x20000, 0x8a191142 )
	ROM_LOAD( "mjls_07.bin",  0x060000, 0x20000, 0x384b9c40 )
	ROM_LOAD( "mjls_08.bin",  0x080000, 0x20000, 0x072ac9b6 )
	ROM_LOAD( "mjls_09.bin",  0x0a0000, 0x20000, 0xf4dc5e77 )
	ROM_LOAD( "mjls_10.bin",  0x0c0000, 0x20000, 0xaa5a165a )
	ROM_LOAD( "mjls_11.bin",  0x0e0000, 0x20000, 0x25a44a56 )
	ROM_LOAD( "mjls_12.bin",  0x100000, 0x20000, 0x2e19183c )
	ROM_LOAD( "mjls_13.bin",  0x120000, 0x20000, 0xcc08652c )
	ROM_LOAD( "mjls_14.bin",  0x140000, 0x20000, 0xf469f3a5 )
	ROM_LOAD( "mjls_15.bin",  0x160000, 0x20000, 0x815b187a )
	ROM_LOAD( "mjls_16.bin",  0x180000, 0x10000, 0x53366690 )
ROM_END

ROM_START( vanilla )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "vanilla.01",   0x00000,  0x10000, 0x2a3341a8 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "vanilla.03",   0x00000,  0x10000, 0xe035842f )
	ROM_LOAD( "vanilla.02",   0x10000,  0x10000, 0x93d8398a )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "vanilla.04",   0x000000, 0x20000, 0xf21e1ff4 )
	ROM_LOAD( "vanilla.05",   0x020000, 0x20000, 0x15d6ff78 )
	ROM_LOAD( "vanilla.06",   0x040000, 0x20000, 0x90da7b35 )
	ROM_LOAD( "vanilla.07",   0x060000, 0x20000, 0x71b2896f )
	ROM_LOAD( "vanilla.08",   0x080000, 0x20000, 0xdd195233 )
	ROM_LOAD( "vanilla.09",   0x0a0000, 0x20000, 0x5521c7a1 )
	ROM_LOAD( "vanilla.10",   0x0c0000, 0x20000, 0xe7d781da )
	ROM_LOAD( "vanilla.11",   0x0e0000, 0x20000, 0xba7fbf3d )
	ROM_LOAD( "vanilla.12",   0x100000, 0x20000, 0x56fe9708 )
	ROM_LOAD( "vanilla.13",   0x120000, 0x20000, 0x91011a9e )
	ROM_LOAD( "vanilla.14",   0x140000, 0x20000, 0x460db736 )
	ROM_LOAD( "vanilla.15",   0x160000, 0x20000, 0xf977655c )
	ROM_LOAD( "vanilla.16",   0x180000, 0x10000, 0xf286a9db )
	ROM_LOAD( "vanilla.17",   0x190000, 0x10000, 0x9b0a7bb5 )
	ROM_LOAD( "vanilla.18",   0x1a0000, 0x10000, 0x54120c24 )
	ROM_LOAD( "vanilla.19",   0x1b0000, 0x10000, 0xc1bb8643 )
	ROM_LOAD( "vanilla.20",   0x1c0000, 0x10000, 0x26bb26a0 )
	ROM_LOAD( "vanilla.21",   0x1d0000, 0x10000, 0x61046b51 )
	ROM_LOAD( "vanilla.22",   0x1e0000, 0x10000, 0x66de02e6 )
	ROM_LOAD( "vanilla.23",   0x1f0000, 0x10000, 0x64186e8a )
ROM_END

ROM_START( finalbny )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "22.4e",        0x00000,  0x10000, 0xccb85d99 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "3.4t",         0x00000,  0x10000, 0xf5d60735 )
	ROM_LOAD( "vanilla.02",   0x10000,  0x10000, 0x93d8398a )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "vanilla.04",   0x000000, 0x20000, 0xf21e1ff4 )
	ROM_LOAD( "vanilla.05",   0x020000, 0x20000, 0x15d6ff78 )
	ROM_LOAD( "vanilla.06",   0x040000, 0x20000, 0x90da7b35 )
	ROM_LOAD( "vanilla.07",   0x060000, 0x20000, 0x71b2896f )
	ROM_LOAD( "vanilla.08",   0x080000, 0x20000, 0xdd195233 )
	ROM_LOAD( "vanilla.09",   0x0a0000, 0x20000, 0x5521c7a1 )
	ROM_LOAD( "vanilla.10",   0x0c0000, 0x20000, 0xe7d781da )
	ROM_LOAD( "vanilla.11",   0x0e0000, 0x20000, 0xba7fbf3d )
	ROM_LOAD( "vanilla.12",   0x100000, 0x20000, 0x56fe9708 )
	ROM_LOAD( "vanilla.13",   0x120000, 0x20000, 0x91011a9e )
	ROM_LOAD( "vanilla.14",   0x140000, 0x20000, 0x460db736 )
	ROM_LOAD( "vanilla.15",   0x160000, 0x20000, 0xf977655c )
	ROM_LOAD( "16.7d",        0x180000, 0x10000, 0x7d122177 )
	ROM_LOAD( "17.7e",        0x190000, 0x10000, 0x3cfb4265 )
	ROM_LOAD( "18.7f",        0x1a0000, 0x10000, 0x7b8ca753 )
	ROM_LOAD( "19.7j",        0x1b0000, 0x10000, 0xd7deca63 )
ROM_END

ROM_START( qmhayaku )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD( "1.4e",    0x00000,  0x10000, 0x5a73cdf8 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "3.4t",    0x00000,  0x10000, 0xd420dac8 )
	ROM_LOAD( "2.4s",    0x10000,  0x10000, 0xf88cb623 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "4.9b",    0x000000, 0x20000, 0x2fba26fe )
	ROM_LOAD( "5.9d",    0x020000, 0x20000, 0x105f9930 )
	ROM_LOAD( "6.9e",    0x040000, 0x20000, 0x5e8f0177 )
	ROM_LOAD( "7.9f",    0x060000, 0x20000, 0x612803ba )
	ROM_LOAD( "8.9j",    0x080000, 0x20000, 0x874fe074 )
	ROM_LOAD( "9.9k",    0x0a0000, 0x20000, 0xafa873d2 )
	ROM_LOAD( "10.9l",   0x0c0000, 0x20000, 0x17a4a609 )
	ROM_LOAD( "11.9n",   0x0e0000, 0x20000, 0xd2357c72 )
	ROM_LOAD( "12.9p",   0x100000, 0x20000, 0x4b63c040 )
	ROM_LOAD( "13.7a",   0x120000, 0x20000, 0xa182d9cd )
	ROM_LOAD( "14.7b",   0x140000, 0x20000, 0x22b1f1fd )
	ROM_LOAD( "15.7d",   0x160000, 0x20000, 0x3db4df6c )
	ROM_LOAD( "16.7e",   0x180000, 0x20000, 0xc1283063 )
	ROM_LOAD( "17.7f",   0x1a0000, 0x10000, 0x4ca71ef1 )
	ROM_LOAD( "18.7j",   0x1b0000, 0x10000, 0x81190d74 )
	ROM_LOAD( "19.7k",   0x1c0000, 0x10000, 0xcad37c2f )
	ROM_LOAD( "20.7l",   0x1d0000, 0x10000, 0x18e18174 )
ROM_END

ROM_START( galkoku )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "gkok_01.bin",  0x00000,  0x10000, 0x254c526c )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "gkok_02.bin",  0x00000,  0x10000, 0x3dec7469 )
	ROM_LOAD( "gkok_03.bin",  0x10000,  0x10000, 0x66f51b21 )

	ROM_REGION( 0x110000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "gkok_04.bin",  0x000000, 0x10000, 0x741815a5 )
	ROM_LOAD( "gkok_05.bin",  0x010000, 0x10000, 0x28a17cd8 )
	ROM_LOAD( "gkok_06.bin",  0x020000, 0x10000, 0x8eac2143 )
	ROM_LOAD( "gkok_07.bin",  0x030000, 0x10000, 0xde5f3f20 )
	ROM_LOAD( "gkok_08.bin",  0x040000, 0x10000, 0xf3348126 )
	ROM_LOAD( "gkok_09.bin",  0x050000, 0x10000, 0x691f2521 )
	ROM_LOAD( "gkok_10.bin",  0x060000, 0x10000, 0xf1b0b411 )
	ROM_LOAD( "gkok_11.bin",  0x070000, 0x10000, 0xef42af9e )
	ROM_LOAD( "gkok_12.bin",  0x080000, 0x10000, 0xe2b32195 )
	ROM_LOAD( "gkok_13.bin",  0x090000, 0x10000, 0x83d913a1 )
	ROM_LOAD( "gkok_14.bin",  0x0a0000, 0x10000, 0x04c97de9 )
	ROM_LOAD( "gkok_15.bin",  0x0b0000, 0x10000, 0x3845280d )
	ROM_LOAD( "gkok_16.bin",  0x0c0000, 0x10000, 0x7472a7ce )
	ROM_LOAD( "gkok_17.bin",  0x0d0000, 0x10000, 0x92b605a2 )
	ROM_LOAD( "gkok_18.bin",  0x0e0000, 0x10000, 0x8bb7bdcc )
	ROM_LOAD( "gkok_19.bin",  0x0f0000, 0x10000, 0xb1b4643a )
	ROM_LOAD( "gkok_20.bin",  0x100000, 0x10000, 0x36107e6f )
ROM_END

ROM_START( hyouban )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.3d",         0x00000,  0x10000, 0x307b4f7e )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "gkok_02.bin",  0x00000,  0x10000, 0x3dec7469 )
	ROM_LOAD( "gkok_03.bin",  0x10000,  0x10000, 0x66f51b21 )

	ROM_REGION( 0x110000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "gkok_04.bin",  0x000000, 0x10000, 0x741815a5 )
	ROM_LOAD( "gkok_05.bin",  0x010000, 0x10000, 0x28a17cd8 )
	ROM_LOAD( "6.10d",        0x020000, 0x10000, 0x2a941698 )
	ROM_LOAD( "gkok_07.bin",  0x030000, 0x10000, 0xde5f3f20 )
	ROM_LOAD( "gkok_08.bin",  0x040000, 0x10000, 0xf3348126 )
	ROM_LOAD( "gkok_09.bin",  0x050000, 0x10000, 0x691f2521 )
	ROM_LOAD( "gkok_10.bin",  0x060000, 0x10000, 0xf1b0b411 )
	ROM_LOAD( "gkok_11.bin",  0x070000, 0x10000, 0xef42af9e )
	ROM_LOAD( "gkok_12.bin",  0x080000, 0x10000, 0xe2b32195 )
	ROM_LOAD( "gkok_13.bin",  0x090000, 0x10000, 0x83d913a1 )
	ROM_LOAD( "gkok_14.bin",  0x0a0000, 0x10000, 0x04c97de9 )
	ROM_LOAD( "gkok_15.bin",  0x0b0000, 0x10000, 0x3845280d )
	ROM_LOAD( "gkok_16.bin",  0x0c0000, 0x10000, 0x7472a7ce )
	ROM_LOAD( "gkok_17.bin",  0x0d0000, 0x10000, 0x92b605a2 )
	ROM_LOAD( "gkok_18.bin",  0x0e0000, 0x10000, 0x8bb7bdcc )
	ROM_LOAD( "gkok_19.bin",  0x0f0000, 0x10000, 0xb1b4643a )
	ROM_LOAD( "gkok_20.bin",  0x100000, 0x10000, 0x36107e6f )
ROM_END

ROM_START( galkaika )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "gkai_01.bin",  0x00000,  0x10000, 0x81b89559 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "gkai_02.bin",  0x00000,  0x10000, 0xdb899dd5 )
	ROM_LOAD( "gkai_03.bin",  0x10000,  0x10000, 0xa66a1c52 )

	ROM_REGION( 0x120000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "gkai_04.bin",  0x000000, 0x20000, 0xb1071e49 )
	ROM_LOAD( "gkai_05.bin",  0x020000, 0x20000, 0xe5162326 )
	ROM_LOAD( "gkai_06.bin",  0x040000, 0x20000, 0xe0cebb15 )
	ROM_LOAD( "gkai_07.bin",  0x060000, 0x20000, 0x26915aa7 )
	ROM_LOAD( "gkai_08.bin",  0x080000, 0x20000, 0xdf009be3 )
	ROM_LOAD( "gkai_09.bin",  0x0a0000, 0x20000, 0xcebfb4f3 )
	ROM_LOAD( "gkai_10.bin",  0x0c0000, 0x20000, 0x43ecb3c5 )
	ROM_LOAD( "gkai_11.bin",  0x0e0000, 0x20000, 0x66f4dbfa )
	ROM_LOAD( "gkai_12.bin",  0x100000, 0x10000, 0xdc35168a )
	ROM_LOAD( "gkai_13.bin",  0x110000, 0x10000, 0xd9f495f3 )
ROM_END

ROM_START( tokyogal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "tgal_21.bin",  0x00000,  0x10000, 0xad4eecec )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "tgal_22.bin",  0x00000,  0x10000, 0x36be0868 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "tgal_01.bin",  0x000000, 0x10000, 0x6a7a5c13 )
	ROM_LOAD( "tgal_02.bin",  0x010000, 0x10000, 0x31e052e6 )
	ROM_LOAD( "tgal_03.bin",  0x020000, 0x10000, 0xd4bbf1e6 )
	ROM_LOAD( "tgal_04.bin",  0x030000, 0x10000, 0xf2b30256 )
	ROM_LOAD( "tgal_05.bin",  0x040000, 0x10000, 0xaf820677 )
	ROM_LOAD( "tgal_06.bin",  0x050000, 0x10000, 0xd9ff9b76 )
	ROM_LOAD( "tgal_07.bin",  0x060000, 0x10000, 0xd5288e37 )
	ROM_LOAD( "tgal_08.bin",  0x070000, 0x10000, 0x824fa5cc )
	ROM_LOAD( "tgal_09.bin",  0x080000, 0x10000, 0x795b8f8c )
	ROM_LOAD( "tgal_10.bin",  0x090000, 0x10000, 0xf2c13f7a )
	ROM_LOAD( "tgal_11.bin",  0x0a0000, 0x10000, 0x551f6fb4 )
	ROM_LOAD( "tgal_12.bin",  0x0b0000, 0x10000, 0x78db30a7 )
	ROM_LOAD( "tgal_13.bin",  0x0c0000, 0x10000, 0x04a81e7a )
	ROM_LOAD( "tgal_14.bin",  0x0d0000, 0x10000, 0x12b43b21 )
	ROM_LOAD( "tgal_15.bin",  0x0e0000, 0x10000, 0xaf06f649 )
	ROM_LOAD( "tgal_16.bin",  0x0f0000, 0x10000, 0x2996431a )
	ROM_LOAD( "tgal_17.bin",  0x100000, 0x10000, 0x470dde3c )
	ROM_LOAD( "tgal_18.bin",  0x110000, 0x10000, 0x0d04d3bc )
	ROM_LOAD( "tgal_19.bin",  0x120000, 0x10000, 0x1c8fe0e8 )
	ROM_LOAD( "tgal_20.bin",  0x130000, 0x10000, 0xb8542eeb )
ROM_END

ROM_START( tokimbsj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "tmbj_01.bin",  0x00000,  0x10000, 0xb335c300 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "tgal_22.bin",  0x00000,  0x10000, 0x36be0868 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "tgal_01.bin",  0x000000, 0x10000, 0x6a7a5c13 )
	ROM_LOAD( "tmbj_04.bin",  0x010000, 0x10000, 0x09e3f23d )
	ROM_LOAD( "tgal_03.bin",  0x020000, 0x10000, 0xd4bbf1e6 )
	ROM_LOAD( "tgal_04.bin",  0x030000, 0x10000, 0xf2b30256 )
	ROM_LOAD( "tgal_05.bin",  0x040000, 0x10000, 0xaf820677 )
	ROM_LOAD( "tgal_06.bin",  0x050000, 0x10000, 0xd9ff9b76 )
	ROM_LOAD( "tgal_07.bin",  0x060000, 0x10000, 0xd5288e37 )
	ROM_LOAD( "tgal_08.bin",  0x070000, 0x10000, 0x824fa5cc )
	ROM_LOAD( "tgal_09.bin",  0x080000, 0x10000, 0x795b8f8c )
	ROM_LOAD( "tgal_10.bin",  0x090000, 0x10000, 0xf2c13f7a )
	ROM_LOAD( "tgal_11.bin",  0x0a0000, 0x10000, 0x551f6fb4 )
	ROM_LOAD( "tgal_12.bin",  0x0b0000, 0x10000, 0x78db30a7 )
	ROM_LOAD( "tgal_13.bin",  0x0c0000, 0x10000, 0x04a81e7a )
	ROM_LOAD( "tgal_14.bin",  0x0d0000, 0x10000, 0x12b43b21 )
	ROM_LOAD( "tgal_15.bin",  0x0e0000, 0x10000, 0xaf06f649 )
	ROM_LOAD( "tgal_16.bin",  0x0f0000, 0x10000, 0x2996431a )
	ROM_LOAD( "tgal_17.bin",  0x100000, 0x10000, 0x470dde3c )
	ROM_LOAD( "tgal_18.bin",  0x110000, 0x10000, 0x0d04d3bc )
	ROM_LOAD( "tmbj_21.bin",  0x120000, 0x10000, 0xb608d6b1 )
	ROM_LOAD( "tmbj_22.bin",  0x130000, 0x10000, 0xe706fc87 )
ROM_END

ROM_START( mcontest )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mcon_01.bin",  0x00000, 0x10000, 0x79a30028 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mcon_02.bin",  0x00000, 0x10000, 0x236b8fdc )
	ROM_LOAD( "mcon_03.bin",  0x10000, 0x10000, 0x6d6bdefb )

	ROM_REGION( 0x160000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mcon_04.bin",  0x000000, 0x20000, 0xadb6e002 )
	ROM_LOAD( "mcon_05.bin",  0x020000, 0x20000, 0xea8ceb49 )
	ROM_LOAD( "mcon_06.bin",  0x040000, 0x10000, 0xd3fee691 )
	ROM_LOAD( "mcon_07.bin",  0x050000, 0x10000, 0x7685a1b1 )
	ROM_LOAD( "mcon_08.bin",  0x060000, 0x10000, 0xeee52454 )
	ROM_LOAD( "mcon_09.bin",  0x070000, 0x10000, 0x2ad2d00f )
	ROM_LOAD( "mcon_10.bin",  0x080000, 0x10000, 0x6ff32ed9 )
	ROM_LOAD( "mcon_11.bin",  0x090000, 0x10000, 0x4f9c340f )
	ROM_LOAD( "mcon_12.bin",  0x0a0000, 0x10000, 0x41cffdf0 )
	ROM_LOAD( "mcon_13.bin",  0x0b0000, 0x10000, 0xd494fdb7 )
	ROM_LOAD( "mcon_14.bin",  0x0c0000, 0x10000, 0x9fe3f75d )
	ROM_LOAD( "mcon_15.bin",  0x0d0000, 0x10000, 0x79fa427a )
	ROM_LOAD( "mcon_16.bin",  0x0e0000, 0x10000, 0xf5ae3668 )
	ROM_LOAD( "mcon_17.bin",  0x0f0000, 0x10000, 0xcb02f51d )
	ROM_LOAD( "mcon_18.bin",  0x100000, 0x10000, 0x8e5fe1bc )
	ROM_LOAD( "mcon_19.bin",  0x110000, 0x10000, 0x5b382cf3 )
	ROM_LOAD( "mcon_20.bin",  0x120000, 0x10000, 0x8ffbd8fe )
	ROM_LOAD( "mcon_21.bin",  0x130000, 0x10000, 0x9476d11d )
	ROM_LOAD( "mcon_22.bin",  0x140000, 0x10000, 0x07d21863 )
	ROM_LOAD( "mcon_23.bin",  0x150000, 0x10000, 0x979e0f93 )
ROM_END

ROM_START( uchuuai )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.3h",   0x00000, 0x10000, 0x6a6fd569 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "2.3h",   0x00000, 0x10000, 0x8673ba16 )

	ROM_REGION( 0x160000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "3.10a",  0x000000, 0x20000, 0x67b8dcd9 )
	ROM_LOAD( "4.10c",  0x020000, 0x20000, 0x6a3b50ce )
	ROM_LOAD( "5.10d",  0x040000, 0x10000, 0x5334ed3c )
	ROM_LOAD( "6.10e",  0x050000, 0x10000, 0x2871addf )
	ROM_LOAD( "7.10f",  0x060000, 0x10000, 0x0a75383d )
	ROM_LOAD( "8.10j",  0x070000, 0x10000, 0x4a45a098 )
	ROM_LOAD( "9.10k",  0x080000, 0x10000, 0x36ec60f8 )
	ROM_LOAD( "10.10m", 0x090000, 0x10000, 0x4f17dce6 )
	ROM_LOAD( "11.10n", 0x0a0000, 0x10000, 0x84c31068 )
	ROM_LOAD( "12.10p", 0x0b0000, 0x10000, 0x8a263dfb )
	ROM_LOAD( "13.11a", 0x0c0000, 0x10000, 0x3f47bf0b )
	ROM_LOAD( "14.11c", 0x0d0000, 0x10000, 0x89f0143f )
	ROM_LOAD( "15.11d", 0x0e0000, 0x10000, 0xdc3d52ad )
	ROM_LOAD( "16.11e", 0x0f0000, 0x10000, 0xaba3e0c5 )
	ROM_LOAD( "17.11f", 0x100000, 0x10000, 0x23a75436 )
	ROM_LOAD( "18.11j", 0x110000, 0x10000, 0x3602af29 )
	ROM_LOAD( "19.11k", 0x120000, 0x10000, 0x1c4a3b49 )
	ROM_LOAD( "20.11m", 0x130000, 0x10000, 0xcc491fa9 )
	ROM_LOAD( "21.11n", 0x140000, 0x10000, 0xba4e42a1 )
	ROM_LOAD( "22.11p", 0x150000, 0x10000, 0xbe5ebd80 )
ROM_END

ROM_START( av2mj1bb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.bin",      0x00000, 0x10000, 0xdf0f03fb )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "3.bin",      0x00000, 0x10000, 0x0cdc9489 )
	ROM_LOAD( "2.bin",      0x10000, 0x10000, 0x6283a444 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "4.bin",      0x000000, 0x20000, 0x18fe29c3 )
	ROM_LOAD( "5.bin",      0x020000, 0x20000, 0x0eff4bbf )
	ROM_LOAD( "6.bin",      0x040000, 0x20000, 0xac351796 )
	ROM_LOAD( "mj-1802.9a", 0x180000, 0x80000, 0xe6213f10 )
ROM_END

ROM_START( av2mj2rg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4e",       0x00000, 0x10000, 0x2295b8df )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sub program */
	ROM_LOAD( "3.4t",       0x00000, 0x10000, 0x52be7b5e )
	ROM_LOAD( "2.4s",       0x10000, 0x10000, 0x6283a444 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "4.9b",       0x000000, 0x20000, 0x4d965b5c )
	ROM_LOAD( "5.9d",       0x020000, 0x20000, 0x4f5bd948 )
	ROM_LOAD( "6.9e",       0x040000, 0x20000, 0x1921dae4 )
	ROM_LOAD( "7.9f",       0x060000, 0x20000, 0xfbd9d0b0 )
	ROM_LOAD( "8.9j",       0x080000, 0x20000, 0x637098a9 )
	ROM_LOAD( "9.9k",       0x0a0000, 0x20000, 0x6c06ca0d )
	ROM_LOAD( "mj-1802.9a", 0x180000, 0x80000, 0xe6213f10 )
ROM_END


GAME( 1990, pstadium,        0, pstadium, pstadium, pstadium, ROT0, "Nichibutsu", "Mahjong Panic Stadium (Japan)" )
GAME( 1989, triplew1,        0, triplew1, triplew1, triplew1, ROT0, "Nichibutsu", "Mahjong Triple Wars (Japan)" )
GAME( 1990, triplew2,        0, triplew2, triplew1, triplew2, ROT0, "Nichibutsu", "Mahjong Triple Wars 2 (Japan)" )
GAME( 1990, ntopstar,        0, ntopstar, ntopstar, ntopstar, ROT0, "Nichibutsu", "Mahjong Nerae! Top Star (Japan)" )
GAME( 1991, mjlstory,        0, mjlstory, mjlstory, mjlstory, ROT0, "Nichibutsu", "Mahjong Jikken Love Story (Japan)" )
GAME( 1991,  vanilla,        0,  vanilla,  vanilla,  vanilla, ROT0, "Nichibutsu", "Mahjong Vanilla Syndrome (Japan)" )
GAME( 1991, finalbny,  vanilla, finalbny, finalbny, finalbny, ROT0, "Nichibutsu", "Mahjong Final Bunny [BET] (Japan)" )
GAME( 1991, qmhayaku,        0, qmhayaku, qmhayaku, qmhayaku, ROT0, "Nichibutsu", "Quiz-Mahjong Hayaku Yatteyo! (Japan)" )
GAME( 1989,  galkoku,        0,  galkoku,  galkoku,  galkoku, ROT0, "Nichibutsu/T.R.TEC", "Mahjong Gal no Kokuhaku (Japan)" )
GAME( 1989,  hyouban,  galkoku,  hyouban,  hyouban,  hyouban, ROT0, "Nichibutsu/T.R.TEC", "Mahjong Hyouban Musume [BET] (Japan)" )
GAME( 1989, galkaika,        0, galkaika, galkaika, galkaika, ROT0, "Nichibutsu/T.R.TEC", "Mahjong Gal no Kaika (Japan)" )
GAME( 1989, tokyogal,        0, tokyogal, tokyogal, tokyogal, ROT0, "Nichibutsu", "Tokyo Gal Zukan (Japan)" )
GAME( 1989, tokimbsj, tokyogal, tokimbsj, tokimbsj, tokimbsj, ROT0, "Nichibutsu", "Tokimeki Bishoujo [BET] (Japan)" )
GAME( 1989, mcontest,        0, mcontest, mcontest, mcontest, ROT0, "Nichibutsu", "Miss Mahjong Contest (Japan)" )
GAME( 1989,  uchuuai,        0,  uchuuai,  uchuuai,  uchuuai, ROT0, "Nichibutsu", "Mahjong Uchuu yori Ai wo komete (Japan)" )
GAMEX(1991, av2mj1bb,        0, av2mj1bb, av2mj1bb, av2mj1bb, ROT0, "MIKI SYOUJI/AV JAPAN", "AV2Mahjong No.1 Bay Bridge no Seijo (Japan)", GAME_NOT_WORKING )
GAMEX(1991, av2mj2rg,        0, av2mj2rg, av2mj2rg, av2mj2rg, ROT0, "MIKI SYOUJI/AV JAPAN", "AV2Mahjong No.2 Rouge no Kaori (Japan)", GAME_NOT_WORKING )
