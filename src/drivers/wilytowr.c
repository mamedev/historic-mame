/***************************************************************************

Wily Towr   (c) 1984 Irem

driver by Nicola Salmoria


Notes:
- Unless there is some special logic related to NMI enable, the game doesn't
  rely on vblank for timing. It all seems to be controlled by the CPU clock.
  The NMI handler just handles the "Stop Mode" dip switch.

TODO:
- Flip screen (if it's like the other Irem games, the dip switch has to
  be polled directly)
- Sound: it's difficult to guess how the I8039 is connected... there's also a
  OKI MSM80C39RS chip.
- One unknown ROM. Samples?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8039/i8039.h"


data8_t *wilytowr_bgvideoram,*wilytowr_fgvideoram,*wilytowr_scrollram;

static int pal_bank;


PALETTE_INIT( wilytowr )
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + 256] >> 0) & 0x01;
		bit1 = (color_prom[i + 256] >> 1) & 0x01;
		bit2 = (color_prom[i + 256] >> 2) & 0x01;
		bit3 = (color_prom[i + 256] >> 3) & 0x01;
		g =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*256] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*256] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*256] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*256] >> 3) & 0x01;
		b =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	color_prom += 3*256;

	for (i = 0;i < 4;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i+256,r,g,b);
	}
}



static WRITE_HANDLER( wilytwr_palbank_w )
{
	pal_bank = data & 1;
}



VIDEO_UPDATE( wilytowr )
{
	int offs;

	for (offs = 0;offs < 0x400;offs++)
	{
		int sx,sy,code,color;


		sx = offs % 32;
		sy = offs / 32;
		code = wilytowr_bgvideoram[offs] | ((wilytowr_bgvideoram[0x400 + offs] & 0x30) << 4);
		color = (wilytowr_bgvideoram[0x400 + offs] & 0x0f) + (pal_bank << 4);

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				0,0,
				8*sx,(8*sy - wilytowr_scrollram[8*sx]) & 0xff,
				cliprect,TRANSPARENCY_NONE,0);
	}

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sx,sy,code,color;

		sx = spriteram[offs+3];
		sy = 238 - spriteram[offs+0];
		code = spriteram[offs+1];
		color = (spriteram[offs+2] & 0x0f) + (pal_bank << 4);

		drawgfx(bitmap,Machine->gfx[2],
				code,
				color,
				0,0,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
	}

	for (offs = 0;offs < 0x400;offs++)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				wilytowr_fgvideoram[offs],
				0,
				0,0,
				8*sx,8*sy,
				cliprect,TRANSPARENCY_PEN,0);
	}
}


static WRITE_HANDLER( coin_w )
{
	coin_counter_w(offset,data & 1);
}


static WRITE_HANDLER( snd_irq_w )
{
	cpu_set_irq_line(1, 0, PULSE_LINE);
}


static int p1,p2;

static WRITE_HANDLER( snddata_w )
{
	if ((p2 & 0xf0) == 0xe0)
		AY8910_control_port_0_w(0,offset);
	else if ((p2 & 0xf0) == 0xa0)
		AY8910_write_port_0_w(0,offset);
	else if ((p1 & 0xe0) == 0x60)
		AY8910_control_port_1_w(0,offset);
	else if ((p1 & 0xe0) == 0x40)
		AY8910_write_port_1_w(0,offset);
	else // if ((p2 & 0xf0) != 0x70)
		/* the port address is the data, while the data seems to be control bits */
		logerror("%04x: snddata_w ctrl = %02x, p1 = %02x, p2 = %02x, data = %02x\n",activecpu_get_pc(),data,p1,p2,offset);
}

static WRITE_HANDLER( p1_w )
{
	p1 = data;
}

static WRITE_HANDLER( p2_w )
{
	p2 = data;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf800, 0xf800, input_port_0_r },
	{ 0xf801, 0xf801, input_port_1_r },
	{ 0xf802, 0xf802, input_port_2_r },
	{ 0xf806, 0xf806, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe1ff, MWA_RAM },
	{ 0xe200, 0xe2ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe300, 0xe3ff, MWA_RAM, &wilytowr_scrollram },
	{ 0xe400, 0xe7ff, MWA_RAM, &wilytowr_fgvideoram },
	{ 0xe800, 0xefff, MWA_RAM, &wilytowr_bgvideoram },
	{ 0xf000, 0xf000, interrupt_enable_w },	/* NMI enable */
	{ 0xf003, 0xf003, wilytwr_palbank_w },
	{ 0xf006, 0xf007, coin_w },
	{ 0xf800, 0xf800, soundlatch_w },
	{ 0xf801, 0xf801, MWA_NOP },	/* unknown (cleared by NMI handler) */
	{ 0xf803, 0xf803, snd_irq_w },
MEMORY_END

static MEMORY_READ_START( i8039_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( i8039_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
MEMORY_END

static PORT_READ_START( i8039_readport )
//	{ 0x00, 0xff, },
//	{ I8039_t1, I8039_t1,  },
PORT_END

static PORT_WRITE_START( i8039_writeport )
	{ 0x00, 0xff, snddata_w },
	{ I8039_p1, I8039_p1, p1_w },
	{ I8039_p2, I8039_p2, p2_w },
PORT_END



INPUT_PORTS_START( wilytowr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coinage ) ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x60, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x04, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/* In stop mode, press 1 to stop and 2 to restart */
	PORT_BITX   ( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), RGN_FRAC(0,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			RGN_FRAC(1,6)+0, RGN_FRAC(1,6)+1, RGN_FRAC(1,6)+2, RGN_FRAC(1,6)+3,
			RGN_FRAC(1,6)+4, RGN_FRAC(1,6)+5, RGN_FRAC(1,6)+6, RGN_FRAC(1,6)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   256, 1 },
	{ REGION_GFX2, 0, &tilelayout,     0, 32 },
	{ REGION_GFX3, 0, &spritelayout,   0, 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	3579545/4,	/* ??? using the same as other Irem games */
	{ 20, 20 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};



static MACHINE_DRIVER_START( wilytowr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(I8039,8000000/15)	/* ????? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(i8039_readmem,i8039_writemem)
	MDRV_CPU_PORTS(i8039_readport,i8039_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256+4)

	MDRV_PALETTE_INIT(wilytowr)
	MDRV_VIDEO_UPDATE(wilytowr)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wilytowr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "wt4e.bin",     0x0000, 0x2000, 0xa38e4b8a )
	ROM_LOAD( "wt4h.bin",     0x2000, 0x2000, 0xc1405ceb )
	ROM_LOAD( "wt4j.bin",     0x4000, 0x2000, 0x379fb1c3 )
	ROM_LOAD( "wt4k.bin",     0x6000, 0x2000, 0x2dd6f9c7 )
	ROM_LOAD( "wt_a-4m.bin",  0x8000, 0x2000, 0xc1f8a7d5 )
	ROM_LOAD( "wt_a-4n.bin",  0xa000, 0x2000, 0xb212f7d2 )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* 8039 */
	ROM_LOAD( "wt4d.bin",     0x0000, 0x1000, 0x25a171bf )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	/* '3' character is bad, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_b-5e.bin",  0x0000, 0x1000, 0xfe45df43 )
	ROM_LOAD( "wt_b-5f.bin",  0x1000, 0x1000, 0x87a17eff )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wtb5a.bin",    0x0000, 0x2000, 0xefc1cbfa )
	ROM_LOAD( "wtb5b.bin",    0x2000, 0x2000, 0xab4bfd07 )
	ROM_LOAD( "wtb5d.bin",    0x4000, 0x2000, 0x40f23e1d )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	/* there are horizontal lines in some tiles, but ROMs have been verified on four boards */
	ROM_LOAD( "wt2j.bin",     0x0000, 0x1000, 0xd1bf0670 )
	ROM_LOAD( "wt3k.bin",     0x1000, 0x1000, 0x83c39a0e )
	ROM_LOAD( "wt_a-3m.bin",  0x2000, 0x1000, 0xe7e468ae )
	ROM_LOAD( "wt_a-3n.bin",  0x3000, 0x1000, 0x0741d1a9 )
	ROM_LOAD( "wt_a-3p.bin",  0x4000, 0x1000, 0x7299f362 )
	ROM_LOAD( "wt_a-3s.bin",  0x5000, 0x1000, 0x9b37d50d )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown; sound? */
	ROM_LOAD( "wt_a-6d.bin",  0x0000, 0x1000, 0xa5dde29b )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "wt_a-5s-.bpr", 0x0000, 0x0100, 0x041950e7 )	/* red */
	ROM_LOAD( "wt_a-5r-.bpr", 0x0100, 0x0100, 0xbc04bf25 )	/* green */
	ROM_LOAD( "wt_a-5p-.bpr", 0x0200, 0x0100, 0xed819a19 )	/* blue */
	ROM_LOAD( "wt_b-9l-.bpr", 0x0300, 0x0020, 0xd2728744 )	/* char palette */
ROM_END

ROM_START( atomboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "wt_a-4e.bin",  0x0000, 0x2000, 0xf7978185 )
	ROM_LOAD( "wt_a-4h.bin",  0x2000, 0x2000, 0x0ca9950b )
	ROM_LOAD( "wt_a-4j.bin",  0x4000, 0x2000, 0x1badbc65 )
	ROM_LOAD( "wt_a-4k.bin",  0x6000, 0x2000, 0x5a341f75 )
	ROM_LOAD( "wt_a-4m.bin",  0x8000, 0x2000, 0xc1f8a7d5 )
	ROM_LOAD( "wt_a-4n.bin",  0xa000, 0x2000, 0xb212f7d2 )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* 8039 */
	ROM_LOAD( "wt_a-4d.bin",  0x0000, 0x1000, 0x3d43361e )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	/* '3' character is bad, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_b-5e.bin",  0x0000, 0x1000, 0xfe45df43 )
	ROM_LOAD( "wt_b-5f.bin",  0x1000, 0x1000, 0x87a17eff )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wt_b-5a.bin",  0x0000, 0x2000, 0xda22c452 )
	ROM_LOAD( "wt_b-5b.bin",  0x2000, 0x2000, 0x4fb25a1f )
	ROM_LOAD( "wt_b-5d.bin",  0x4000, 0x2000, 0x75be2604 )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	/* there are horizontal lines in some tiles, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_a-3j.bin",  0x0000, 0x1000, 0xb30ca38f )
	ROM_LOAD( "wt_a-3k.bin",  0x1000, 0x1000, 0x9a77eb73 )
	ROM_LOAD( "wt_a-3m.bin",  0x2000, 0x1000, 0xe7e468ae )
	ROM_LOAD( "wt_a-3n.bin",  0x3000, 0x1000, 0x0741d1a9 )
	ROM_LOAD( "wt_a-3p.bin",  0x4000, 0x1000, 0x7299f362 )
	ROM_LOAD( "wt_a-3s.bin",  0x5000, 0x1000, 0x9b37d50d )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown; sound? */
	ROM_LOAD( "wt_a-6d.bin",  0x0000, 0x1000, 0xa5dde29b )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "wt_a-5s-.bpr", 0x0000, 0x0100, 0x041950e7 )	/* red */
	ROM_LOAD( "wt_a-5r-.bpr", 0x0100, 0x0100, 0xbc04bf25 )	/* green */
	ROM_LOAD( "wt_a-5p-.bpr", 0x0200, 0x0100, 0xed819a19 )	/* blue */
	ROM_LOAD( "wt_b-9l-.bpr", 0x0300, 0x0020, 0xd2728744 )	/* char palette */
ROM_END


GAMEX( 1984, wilytowr, 0,        wilytowr, wilytowr, 0, ROT180, "Irem",                    "Wily Tower", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1985, atomboy,  wilytowr, wilytowr, wilytowr, 0, ROT180, "Irem (Memetron license)", "Atomic Boy", GAME_NO_SOUND | GAME_NO_COCKTAIL )
