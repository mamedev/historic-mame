/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *rastan_spriteram;
extern unsigned char *rastan_scrollx;
extern unsigned char *rastan_scrolly;

void rastan_updatehook0(int offset);
void rastan_updatehook1(int offset);

void rastan_spriteram_w(int offset,int data);
int rastan_spriteram_r(int offset);

void rastan_scrollY_w(int offset,int data);
void rastan_scrollX_w(int offset,int data);

void rastan_videocontrol_w(int offset,int data);

int rastan_interrupt(void);
int rastan_s_interrupt(void);

void rastan_background_w(int offset,int data);
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int rastan_input_r (int offset);

void rastan_sound_w(int offset,int data);
int rastan_sound_r(int offset);

void rastan_speedup_w(int offset,int data);
int rastan_speedup_r(int offset);


int r_rd_a000(int offset);
int r_rd_a001(int offset);
void r_wr_a000(int offset,int data);
void r_wr_a001(int offset,int data);

void r_wr_b000(int offset,int data);
void r_wr_c000(int offset,int data);
void r_wr_d000(int offset,int data);



static struct MemoryReadAddress rastan_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
//	{ 0x10dc10, 0x10dc13, rastan_speedup_r },
	{ 0x10c000, 0x10ffff, MRA_BANK1 },	/* RAM */
	{ 0x200000, 0x20ffff, paletteram_word_r },
	{ 0x3e0000, 0x3e0003, rastan_sound_r },
	{ 0x390000, 0x39000f, rastan_input_r },
	{ 0xc00000, 0xc03fff, videoram10_word_r },
	{ 0xc04000, 0xc07fff, MRA_BANK2 },
	{ 0xc08000, 0xc0bfff, videoram00_word_r },
	{ 0xc0c000, 0xc0ffff, MRA_BANK3 },
	{ 0xd00000, 0xd0ffff, MRA_BANK4, &rastan_spriteram },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rastan_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
//	{ 0x10dc10, 0x10dc13, rastan_speedup_w },
	{ 0x10c000, 0x10ffff, MWA_BANK1 },
	{ 0x200000, 0x20ffff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x350008, 0x35000b, MWA_NOP },     /* 0 only (often) ? */
	{ 0x380000, 0x380003, rastan_videocontrol_w },	/* sprite palette bank, coin counters, other unknowns */
	{ 0x3c0000, 0x3c0003, MWA_NOP },     /*0000,0020,0063,0992,1753 (very often) watchdog? */
	{ 0x3e0000, 0x3e0003, rastan_sound_w },
	{ 0xc00000, 0xc03fff, videoram10_word_w, &videoram10 },
	{ 0xc04000, 0xc07fff, MWA_BANK2 },
	{ 0xc08000, 0xc0bfff, videoram00_word_w, &videoram00 },
	{ 0xc0c000, 0xc0ffff, MWA_BANK3 },
	{ 0xc20000, 0xc20003, rastan_scrollY_w, &rastan_scrolly },  /* scroll Y  1st.w plane1  2nd.w plane2 */
	{ 0xc40000, 0xc40003, rastan_scrollX_w, &rastan_scrollx },  /* scroll X  1st.w plane1  2nd.w plane2 */
//	{ 0xc50000, 0xc50003, MWA_NOP },     /* 0 only (rarely)*/
	{ 0xd00000, 0xd0ffff, MWA_BANK4 },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress rastan_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa000, 0xa000, r_rd_a000 },
	{ 0xa001, 0xa001, r_rd_a001 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rastan_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, r_wr_a000 },
	{ 0xa001, 0xa001, r_wr_a001 },
	{ 0xb000, 0xb000, ADPCM_trigger },
	{ 0xc000, 0xc000, r_wr_c000 },
	{ 0xd000, 0xd000, r_wr_d000 },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( rastan_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPSETTING(    0x00, "250000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* same as rastan, coinage is different */
INPUT_PORTS_START( rastsaga_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
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

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPSETTING(    0x00, "250000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
	0, 4, 0x40000*8+0 ,0x40000*8+4,
	8+0, 8+4, 0x40000*8+8+0, 0x40000*8+8+4,
	16+0, 16+4, 0x40000*8+16+0, 0x40000*8+16+4,
	24+0, 24+4, 0x40000*8+24+0, 0x40000*8+24+4
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x80000, &spritelayout,  0, 0x80 },	/* sprites 16x16*/
	{ -1 } /* end of array */
};



static struct GfxTileLayout tilelayout =
{
	16384,	/* 16384 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { 0, 4, 0x40000*8+0 ,0x40000*8+4, 8+0, 8+4, 0x40000*8+8+0, 0x40000*8+8+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxTileDecodeInfo gfxtiledecodeinfo0[] =
{
	{ 1, 0x00000, &tilelayout, 0, 128, 0x00000001 },	/* foreground tiles */
	{ -1 } /* end of array */
};

static struct GfxTileDecodeInfo gfxtiledecodeinfo1[] =
{
	{ 1, 0x00000, &tilelayout, 0, 128, 0 },	/* background tiles */
	{ -1 } /* end of array */
};


static struct MachineLayer machine_layers[MAX_LAYERS] =
{
	{
		LAYER_TILE,
		64*8,64*8,
		gfxtiledecodeinfo0,
		0,
		rastan_updatehook0,0,0,0
	},
	{
		LAYER_TILE,
		64*8,64*8,
		gfxtiledecodeinfo1,
		0,
		rastan_updatehook1,0,0,0
	}
};



static void rastan_irq_handler (void)
{
	cpu_cause_interrupt (1, 0);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ 255 },
	{ rastan_irq_handler }
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,       /* 8000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0,
			rastan_readmem,rastan_writemem,0,0,
			rastan_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz */
			2,
			rastan_s_readmem,rastan_s_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	machine_layers,
	generic_vh_start,
	generic_vh_stop,
	rastan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
//			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( rastan_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "IC19_38.bin", 0x00000, 0x10000, 0x7407497b )
	ROM_LOAD_ODD ( "IC07_37.bin", 0x00000, 0x10000, 0x7938d6ce )
	ROM_LOAD_EVEN( "IC20_40.bin", 0x20000, 0x10000, 0xb7e92d83 )
	ROM_LOAD_ODD ( "IC08_39.bin", 0x20000, 0x10000, 0xd5ca9e04 )
	ROM_LOAD_EVEN( "IC21_42.bin", 0x40000, 0x10000, 0x975f77ad )
	ROM_LOAD_ODD ( "IC09_43.bin", 0x40000, 0x10000, 0x5ad5c7ab )

	ROM_REGION(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC40_01.bin",   0x00000, 0x10000, 0x67dad0cc )        /* 8x8 0 */
	ROM_LOAD( "IC40_01H.bin",  0x10000, 0x10000, 0x114df5eb )        /* 8x8 0 */
	ROM_LOAD( "IC39_03.bin",   0x20000, 0x10000, 0xe8b5dfe5 )        /* 8x8 0 */
	ROM_LOAD( "IC39_03H.bin",  0x30000, 0x10000, 0xc3dd6377 )        /* 8x8 0 */

	ROM_LOAD( "IC67_02.bin",   0x40000, 0x10000, 0xe6a11bdb )        /* 8x8 1 */
	ROM_LOAD( "IC67_02H.bin",  0x50000, 0x10000, 0x8eefac47 )        /* 8x8 1 */
	ROM_LOAD( "IC66_04.bin",   0x60000, 0x10000, 0xfde7e2cf )        /* 8x8 1 */
	ROM_LOAD( "IC66_04H.bin",  0x70000, 0x10000, 0x80000000 )        /* 8x8 1 */

	ROM_LOAD( "IC15_05.bin",   0x80000, 0x10000, 0xecd6fbcc )        /* sprites 1a */
	ROM_LOAD( "IC15_05H.bin",  0x90000, 0x10000, 0x1fe25954 )        /* sprites 2a */
	ROM_LOAD( "IC14_07.bin",   0xa0000, 0x10000, 0x022daf1d )        /* sprites 3a */
	ROM_LOAD( "IC14_07H.bin",  0xb0000, 0x10000, 0x22a736c3 )        /* sprites 4a */
	ROM_LOAD( "IC28_06.bin",   0xc0000, 0x10000, 0x81e8d0be )        /* sprites 1b */
	ROM_LOAD( "IC28_06H.bin",  0xd0000, 0x10000, 0xab0be5c7 )        /* sprites 2b */
	ROM_LOAD( "IC27_08.bin",   0xe0000, 0x10000, 0xbf187cba )        /* sprites 3b */
	ROM_LOAD( "IC27_08H.bin",  0xf0000, 0x10000, 0xb7eaa116 )        /* sprites 4b */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "IC49_19.bin", 0x0000, 0x10000, 0x73fbbecf )   /* Audio CPU is a Z80  */
                                                     /* sound chip is YM2151*/
	ROM_REGION(0x10000)	/* 64k for the samples */
	ROM_LOAD( "IC76_20.bin", 0x0000, 0x10000, 0x121edf84 ) /* samples are 4bit ADPCM */
ROM_END

ROM_START( rastsaga_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "RS19_38.bin", 0x00000, 0x10000, 0x7428495a )
	ROM_LOAD_ODD ( "RS07_37.bin", 0x00000, 0x10000, 0x7632da3c )
	ROM_LOAD_EVEN( "RS20_40.bin", 0x20000, 0x10000, 0x092456b0 )
	ROM_LOAD_ODD ( "RS08_39.bin", 0x20000, 0x10000, 0x5c4a02b4 )
	ROM_LOAD_EVEN( "RS21_42.bin", 0x40000, 0x10000, 0x1193f1a5 )
	ROM_LOAD_ODD ( "RS09_43.bin", 0x40000, 0x10000, 0x149b90fd )

	ROM_REGION(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC40_01.bin",   0x00000, 0x10000, 0x67dad0cc )        /* 8x8 0 */
	ROM_LOAD( "IC40_01H.bin",  0x10000, 0x10000, 0x114df5eb )        /* 8x8 0 */
	ROM_LOAD( "IC39_03.bin",   0x20000, 0x10000, 0xe8b5dfe5 )        /* 8x8 0 */
	ROM_LOAD( "IC39_03H.bin",  0x30000, 0x10000, 0xc3dd6377 )        /* 8x8 0 */
	ROM_LOAD( "IC67_02.bin",   0x40000, 0x10000, 0xe6a11bdb )        /* 8x8 1 */
	ROM_LOAD( "IC67_02H.bin",  0x50000, 0x10000, 0x8eefac47 )        /* 8x8 1 */
	ROM_LOAD( "IC66_04.bin",   0x60000, 0x10000, 0xfde7e2cf )        /* 8x8 1 */
	ROM_LOAD( "IC66_04H.bin",  0x70000, 0x10000, 0x80000000 )        /* 8x8 1 */
	ROM_LOAD( "IC15_05.bin",   0x80000, 0x10000, 0xecd6fbcc )        /* sprites 1a */
	ROM_LOAD( "IC15_05H.bin",  0x90000, 0x10000, 0x1fe25954 )        /* sprites 2a */
	ROM_LOAD( "IC14_07.bin",   0xa0000, 0x10000, 0x022daf1d )        /* sprites 3a */
	ROM_LOAD( "IC14_07H.bin",  0xb0000, 0x10000, 0x22a736c3 )        /* sprites 4a */
	ROM_LOAD( "IC28_06.bin",   0xc0000, 0x10000, 0x81e8d0be )        /* sprites 1b */
	ROM_LOAD( "IC28_06H.bin",  0xd0000, 0x10000, 0xab0be5c7 )        /* sprites 2b */
	ROM_LOAD( "IC27_08.bin",   0xe0000, 0x10000, 0xbf187cba )        /* sprites 3b */
	ROM_LOAD( "IC27_08H.bin",  0xf0000, 0x10000, 0xb7eaa116 )        /* sprites 4b */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "IC49_19.bin", 0x0000, 0x10000, 0x73fbbecf )   /* Audio CPU is a Z80  */
                                                     /* sound chip is YM2151*/
	ROM_REGION(0x10000)	/* 64k for the samples */
	ROM_LOAD( "IC76_20.bin", 0x0000, 0x10000, 0x121edf84 ) /* samples are 4bit ADPCM */
ROM_END


ADPCM_SAMPLES_START(rastan_samples)
	ADPCM_SAMPLE(0x00, 0x0000, 0x0200*2)
	ADPCM_SAMPLE(0x02, 0x0200, 0x0500*2)
	ADPCM_SAMPLE(0x07, 0x0700, 0x2100*2)
	ADPCM_SAMPLE(0x28, 0x2800, 0x3b00*2)
	ADPCM_SAMPLE(0x63, 0x6300, 0x4e00*2)
	ADPCM_SAMPLE(0xb1, 0xb100, 0x1600*2)
ADPCM_SAMPLES_END



struct GameDriver rastan_driver =
{
	__FILE__,
	0,
	"rastan",
	"Rastan",
	"1987",
	"Taito Japan",
	"Jarek Burczynski\nMarco Cassili",
	0,
	&machine_driver,

	rastan_rom,
	0, 0,
	0,
	(void *)rastan_samples,	/* sound_prom */

	rastan_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver rastsaga_driver =
{
	__FILE__,
	&rastan_driver,
	"rastsaga",
	"Rastan Saga",
	"1987",
	"Taito",
	"Jarek Burczynski\nMarco Cassili",
	0,
	&machine_driver,

	rastsaga_rom,
	0, 0,
	0,
	(void *)rastan_samples,	/* sound_prom */

	rastsaga_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};








#if 0
/* ---------------------------  CUT HERE  ----------------------------- */
/**************** This can be deleted somewhere in the future ***********/
/*                This is here just to test YM2151 emulator             */


int rastan_smus_interrupt(void);
void rastan_vhmus_screenrefresh(struct osd_bitmap *bitmap);
int r_rd_a001mus(int offset);

static struct MemoryReadAddress rastan_smus_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
        { 0x9002, 0x9100, MRA_RAM },
        { 0xa000, 0xa000, r_rd_a000 },
        { 0xa001, 0xa001, r_rd_a001mus },
	{ -1 }  /* end of table */
};


static struct MachineDriver rastmu_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			2,
			rastan_smus_readmem,rastan_s_writemem,0,0,
			rastan_smus_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	rastan_vhmus_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
//			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

struct GameDriver rastmus_driver =
{
	__FILE__,
	0,
	"rastmus",
	"RASTAN MUSIC PLAY (YM2151)",
	"????",
	"?????",
	"JAREK BURCZYNSKI",
	0,
	&rastmu_driver,

	rastan_rom,
	0, 0,
	0,
	(void *)rastan_samples,	/* sound_prom */

	rastan_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
#endif
