/* One Shot One Kill
   Driver by David Haywood and Paul Priest
   Dip Switches and Inputs by Stephane Humbert

Notes :
  - The YM3812 is used only for timing. All sound is played with ADPCM samples.

  - It doesn't seem possible to make 2 consecutive shots at the same place !
    Ingame bug or is there something missing in the emulation ?
    * several gun games are like this, changed the driver so it doesn't try

  - Gun X range is 0x0000-0x01ff and gun Y range is 0x0000-0x00ff, so you
    can shoot sometimes out of the "visible area" ... NOT A BUG !
  - Player 1 and 2 guns do NOT use the same routine to determine the
    coordonates of an impact on the screen : position both guns in the
    "upper left" corner in the "gun test" to see what I mean.
  - I've assumed that the shot was right at the place the shot was made,
    but I don't have any more information about that
    (what the hell is "Gun X Shift Left" REALLY used for ?)

TO DO :

  - fix some priorities for some tiles
  - verify the parameters for the guns (analog ports)
  - figure out year and manufacturer
    (NOTHING is displayed in "demo mode", nor when you complete ALL levels !)

*/

#include "driver.h"
data16_t *oneshot_sprites;
data16_t *oneshot_bg_videoram;
data16_t *oneshot_mid_videoram;
data16_t *oneshot_fg_videoram;

int gun_x_p1,gun_y_p1,gun_x_p2,gun_y_p2;
int gun_x_shift;

WRITE16_HANDLER( oneshot_bg_videoram_w );
WRITE16_HANDLER( oneshot_mid_videoram_w );
WRITE16_HANDLER( oneshot_fg_videoram_w );
VIDEO_START( oneshot );
VIDEO_UPDATE( oneshot );


static READ16_HANDLER( oneshot_in0_word_r )
{
	int data = readinputport(0);

	switch (data & 0x0c)
	{
		case 0x00 :
			gun_x_shift = 35;
			break;
		case 0x04 :
			gun_x_shift = 30;
			break;
		case 0x08 :
			gun_x_shift = 40;
			break;
		case 0x0c :
			gun_x_shift = 50;
			break;
	}

	return data;
}

static READ16_HANDLER( oneshot_gun_x_p1_r )
{
	/* shots must be in a different location to register */
	static int wobble = 0;
	wobble ^= 1;

	return gun_x_p1 ^ wobble;
}

static READ16_HANDLER( oneshot_gun_y_p1_r )
{
	return gun_y_p1;
}

static READ16_HANDLER( oneshot_gun_x_p2_r )
{
	/* shots must be in a different location to register */
	static int wobble = 0;
	wobble ^= 1;

	return gun_x_p2 ^ wobble;
}

static READ16_HANDLER( oneshot_gun_y_p2_r )
{
	return gun_y_p2;
}

static WRITE16_HANDLER( soundbank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * ((data & 0x03) ^ 0x03));
	}
}



static MEMORY_READ16_START( oneshot_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x087fff, MRA16_RAM },
	{ 0x0c0000, 0x0c07ff, MRA16_RAM },
	{ 0x120000, 0x120fff, MRA16_RAM },
	{ 0x180000, 0x182fff, MRA16_RAM },
	{ 0x190002, 0x190003, soundlatch_word_r },
	{ 0x190026, 0x190027, oneshot_gun_x_p1_r },
	{ 0x19002e, 0x19002f, oneshot_gun_x_p2_r },
	{ 0x190036, 0x190037, oneshot_gun_y_p1_r },
	{ 0x19003e, 0x19003f, oneshot_gun_y_p2_r },
	{ 0x19c020, 0x19c021, oneshot_in0_word_r },
	{ 0x19c024, 0x19c025, input_port_1_word_r },
	{ 0x19c02c, 0x19c02d, input_port_2_word_r },
	{ 0x19c030, 0x19c031, input_port_3_word_r },
	{ 0x19c034, 0x19c035, input_port_4_word_r },
MEMORY_END

static MEMORY_WRITE16_START( oneshot_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x087fff, MWA16_RAM },
	{ 0x0c0000, 0x0c07ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x120000, 0x120fff, MWA16_RAM, &oneshot_sprites },
	{ 0x180000, 0x180fff, oneshot_mid_videoram_w, &oneshot_mid_videoram }, // some people , girl etc.
	{ 0x181000, 0x181fff, oneshot_fg_videoram_w, &oneshot_fg_videoram }, // credits etc.
	{ 0x182000, 0x182fff, oneshot_bg_videoram_w, &oneshot_bg_videoram }, // credits etc.
	{ 0x188000, 0x18800f, MWA16_NOP },	// scroll registers???
	{ 0x190010, 0x190011, soundlatch_word_w },
	{ 0x190018, 0x190019, soundbank_w },
MEMORY_END

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ 0x8001, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, YM3812_status_port_0_r },
	{ 0xe010, 0xe010, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, soundlatch_w },
	{ 0x8001, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe000, YM3812_control_port_0_w },
	{ 0xe001, 0xe001, YM3812_write_port_0_w },
	{ 0xe010, 0xe010, OKIM6295_data_0_w },
MEMORY_END

INPUT_PORTS_START( oneshot )
	PORT_START	/* DSW 1	(0x19c020.l -> 0x08006c.l) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )		// 0x080084.l : credits (00-09)
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, "Gun X Shift Left" )		// 0x0824ec.l (not in "test mode")
	PORT_DIPSETTING(    0x04, "30" )
	PORT_DIPSETTING(    0x00, "35" )
	PORT_DIPSETTING(    0x08, "40" )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )	// 0x082706.l - to be confirmed
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )			// 0x0824fe.l
	PORT_DIPNAME( 0x40, 0x00, "Start Round" )			// 0x08224e.l
	PORT_DIPSETTING(    0x00, "Gun Trigger" )
	PORT_DIPSETTING(    0x40, "Start Button" )
	PORT_DIPNAME( 0x80, 0x00, "Gun Test" )			// 0x082286.l
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW 2	(0x19c024.l -> 0x08006e.l) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )		// 0x082500.l
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )	// 0x082506.l
	PORT_DIPSETTING(    0x10, "Easy" )				// 0
	PORT_DIPSETTING(    0x00, "Normal" )			// 1
	PORT_DIPSETTING(    0x20, "Hard" )				// 2
	PORT_DIPSETTING(    0x30, "Hardest" )			// 3
	PORT_DIPNAME( 0x40, 0x00, "Round Select" )		// 0x082f16.l - only after 1st stage
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )		// "On"  in the "test mode"
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )			// "Off" in the "test mode"
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Free_Play ) )	// 0x0800ca.l
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* Credits	(0x19c02c.l -> 0x08007a.l) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* Player 1 Gun Trigger	(0x19c030.l) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* Player 2 Gun Trigger	(0x19c034.l) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* Player 1 Gun X		($190026.l) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START	/* Player 1 Gun Y		($190036.l) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START	/* Player 2 Gun X		($19002e.l) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 35, 15, 0, 0xff )

	PORT_START	/* Player 2 Gun Y		($19003e.l) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 35, 15, 0, 0xff )

INPUT_PORTS_END



static struct GfxLayout oneshot16x16_layout =
{
	16,16,
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(0,8),RGN_FRAC(1,8),RGN_FRAC(2,8),RGN_FRAC(3,8),RGN_FRAC(4,8),RGN_FRAC(5,8),RGN_FRAC(6,8),RGN_FRAC(7,8) },
	{ 0,1,2,3,4,5,6,7,
	 64+0,64+1,64+2,64+3,64+4,64+5,64+6,64+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	 128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	16*16
};

static struct GfxLayout oneshot8x8_layout =
{
	8,8,
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(0,8),RGN_FRAC(1,8),RGN_FRAC(2,8),RGN_FRAC(3,8),RGN_FRAC(4,8),RGN_FRAC(5,8),RGN_FRAC(6,8),RGN_FRAC(7,8) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &oneshot16x16_layout,   0x00, 4  }, /* sprites */
	{ REGION_GFX1, 0, &oneshot8x8_layout,     0x00, 4  }, /* sprites */
	{ -1 } /* end of array */
};

static void irq_handler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,
	5000000,	/* ? */
	{ 100 },
	{ irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static MACHINE_DRIVER_START( oneshot )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(oneshot_readmem,oneshot_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(snd_readmem, snd_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0*16, 20*16-1, 0*16, 15*16-1)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(oneshot)
	MDRV_VIDEO_UPDATE(oneshot)

	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( oneshot )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "1shot-u.a24", 0x00000, 0x20000, 0x0ecd33da  )
	ROM_LOAD16_BYTE( "1shot-u.a22", 0x00001, 0x20000, 0x26c3ae2d  )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "1shot.ua2", 0x00000, 0x010000, 0xf655b80e )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "1shot-ui.16a",0x000000, 0x080000, 0xf765f9a2 )
	ROM_LOAD( "1shot-ui.13a",0x080000, 0x080000, 0x3361b5d8 )
	ROM_LOAD( "1shot-ui.11a",0x100000, 0x080000, 0x8f8bd027 )
	ROM_LOAD( "1shot-ui.08a",0x180000, 0x080000, 0x254b1701 )
	ROM_LOAD( "1shot-ui.16", 0x200000, 0x080000, 0xff246b27 )
	ROM_LOAD( "1shot-ui.13", 0x280000, 0x080000, 0x80342e83 )
	ROM_LOAD( "1shot-ui.11", 0x300000, 0x080000, 0xb8938345 )
	ROM_LOAD( "1shot-ui.08", 0x380000, 0x080000, 0xc9953bef )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "1shot.u15", 0x000000, 0x080000, 0xe3759a47 )
	ROM_LOAD( "1shot.u14", 0x080000, 0x080000, 0x222e33f8 )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	// BAD ADDRESS LINES (mask=00f000)
	ROM_LOAD( "1shot.mb", 0x00000, 0x10000, 0x6b213183 ) // motherboard rom, zooming?
ROM_END



GAMEX(199?, oneshot, 0, oneshot, oneshot, 0, ROT0, "unknown", "One Shot One Kill", GAME_IMPERFECT_GRAPHICS )
