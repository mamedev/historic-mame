/*******************************************************************************

	Irem M107 games:

	Fire Barrel 							(c) 1993 Irem Corporation
	Dream Soccer '94						(c) 1994 Data East Corporation


	Graphics glitches in both games.

	Emulation by Bryan McPhail, mish@tendril.co.uk

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m92.h"
#include "machine/irem_cpu.h"

extern unsigned char *m107_vram_data;
extern int m107_spritesystem;
static unsigned char *m107_ram;
static int m107_irq_vectorbase,m107_vblank,raster_enable;
extern int m107_raster_irq_position,m107_sprite_list;

#define m107_IRQ_0 ((m107_irq_vectorbase+0)/4) /* VBL interrupt*/
#define m107_IRQ_1 ((m107_irq_vectorbase+4)/4) /* ??? */
#define m107_IRQ_2 ((m107_irq_vectorbase+8)/4) /* Raster interrupt */
#define m107_IRQ_3 ((m107_irq_vectorbase+12)/4) /* ??? */

WRITE_HANDLER( m107_spritebuffer_w );
void m107_vh_raster_partial_refresh(struct osd_bitmap *bitmap,int start_line,int end_line);
void m107_screenrefresh(struct osd_bitmap *bitmap,const struct rectangle *clip);
void m107_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void dsoccr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void m107_vh_stop(void);
int m107_vh_start(void);
WRITE_HANDLER( m107_control_w );
WRITE_HANDLER( m107_vram_w );
READ_HANDLER( m107_vram_r );

/*****************************************************************************/

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (offset==1) return; /* Unused top byte */
	cpu_setbank(1,&RAM[0x100000 + ((data&0x7)*0x10000)]);
}

static READ_HANDLER( m107_port_4_r )
{
	if (m107_vblank) return readinputport(4) | 0;
	return readinputport(4) | 0x80;
}

static WRITE_HANDLER( m107_coincounter_w )
{
	if (offset==0) {
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);
	}
}



enum { VECTOR_INIT, YM2151_ASSERT, YM2151_CLEAR, V30_ASSERT, V30_CLEAR };

static void setvector_callback(int param)
{
	static int irqvector;

	switch(param)
	{
		case VECTOR_INIT:	irqvector = 0;		break;
		case YM2151_ASSERT:	irqvector |= 0x2;	break;
		case YM2151_CLEAR:	irqvector &= ~0x2;	break;
		case V30_ASSERT:	irqvector |= 0x1;	break;
		case V30_CLEAR:		irqvector &= ~0x1;	break;
	}

	if (irqvector & 0x2)		/* YM2151 has precedence */
		cpu_irq_line_vector_w(1,0,0x18);
	else if (irqvector & 0x1)	/* V30 */
		cpu_irq_line_vector_w(1,0,0x19);

	if (irqvector == 0)	/* no IRQs pending */
		cpu_set_irq_line(1,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpu_set_irq_line(1,0,ASSERT_LINE);
}

static WRITE_HANDLER( m92_soundlatch_w )
{
	if (offset==0)
	{
		timer_set(TIME_NOW,V30_ASSERT,setvector_callback);
		soundlatch_w(0,data);
//		logerror("soundlatch_w %02x\n",data);
	}
}

static int sound_status;

static READ_HANDLER( m92_sound_status_r )
{
	if (offset == 0)
	{
//logerror("%06x: read sound status\n",cpu_get_pc());

/*

	Gunforce waits on bit 1 (word) going high on bootup, just after
	setting up interrupt controller.

	Gunforce reads bit 2 (word) on every coin insert, doesn't seem to
	do much though..

	Ninja Batman polls this.

	R-Type Leo reads it now & again..

*/
		if (Machine->sample_rate)
			return sound_status;
		else
			return 0xff;
	}
	else return 0xff;
}

static READ_HANDLER( m92_soundlatch_r )
{
	if (offset == 0)
	{
		int res = soundlatch_r(offset);
//		logerror("soundlatch_r %02x\n",res);
		return res;
	}
	else return 0xff;
}

static WRITE_HANDLER( m92_sound_irq_ack_w )
{
	if (offset == 0)
	{
		timer_set(TIME_NOW,V30_CLEAR,setvector_callback);
	}
}

static WRITE_HANDLER( m92_sound_status_w )
{
	if (offset == 0)
	{
//		usrintf_showmessage("sound answer %02x",data);
		sound_status = data;
	}
}

/*****************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x00000, 0x9ffff, MRA_ROM },
	{ 0xa0000, 0xbffff, MRA_BANK1 },
	{ 0xd0000, 0xdffff, m107_vram_r },
	{ 0xe0000, 0xeffff, MRA_RAM },
	{ 0xf8000, 0xf8fff, MRA_RAM },
	{ 0xf9000, 0xf9fff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0xbffff, MWA_ROM },
	{ 0xd0000, 0xdffff, m107_vram_w, &m107_vram_data },
	{ 0xe0000, 0xeffff, MWA_RAM, &m107_ram }, /* System ram */
	{ 0xf8000, 0xf8fff, MWA_RAM, &spriteram },
	{ 0xf9000, 0xf9fff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xffff0, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r }, /* Player 1 */
	{ 0x01, 0x01, input_port_1_r }, /* Player 2 */
	{ 0x02, 0x02, m107_port_4_r }, /* Coins */
	{ 0x03, 0x03, input_port_7_r }, /* Dip 3 */
	{ 0x04, 0x04, input_port_6_r }, /* Dip 2 */
	{ 0x05, 0x05, input_port_5_r }, /* Dip 1 */
	{ 0x06, 0x06, input_port_2_r }, /* Player 3 */
	{ 0x07, 0x07, input_port_3_r }, /* Player 4 */
//	{ 0x08, 0x09, m92_sound_status_r },	/* answer from sound CPU */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x01, m92_soundlatch_w },
	{ 0x02, 0x03, m107_coincounter_w },
	{ 0x04, 0x05, MWA_NOP }, /* ??? 0008 */
	{ 0x06, 0x07, bankswitch_w },
	{ 0x80, 0x9f, m107_control_w },
	{ 0xa0, 0xaf, MWA_NOP }, /* Written with 0's in interrupt */
	{ 0xb0, 0xb1, m107_spritebuffer_w },
PORT_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x00000, 0x1ffff, MRA_ROM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xa8042, 0xa8043, YM2151_status_port_0_r },
	{ 0xa8044, 0xa8045, m92_soundlatch_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x00000, 0x1ffff, MWA_ROM },
	{ 0x9ff00, 0x9ffff, MWA_NOP }, /* Irq controller? */
	{ 0xa0000, 0xa3fff, MWA_RAM },
	{ 0xa8000, 0xa803f, IremGA20_w },
	{ 0xa8040, 0xa8041, YM2151_register_port_0_w },
	{ 0xa8042, 0xa8043, YM2151_data_port_0_w },
	{ 0xa8044, 0xa8045, m92_sound_irq_ack_w },
	{ 0xa8046, 0xa8047, m92_sound_status_w },
	{ 0xffff0, 0xfffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( firebarr )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( dsoccr94 )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_PLAYER3_2BUTTON_JOYSTICK
	PORT_PLAYER4_2BUTTON_JOYSTICK
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	0x20000,
	4,
	{ 8, 0, 24, 16 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	0x8000,
	4,
	{ 0x300000*8, 0x200000*8, 0x100000*8, 0x000000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout spritelayout2 =
{
	16,16,
	0x8000,
	4,
	{ 0x300000*8, 0x200000*8, 0x100000*8, 0x000000*8 },
	{ 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo firebarr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 128 },
	{ REGION_GFX2, 0, &spritelayout2,0, 128 },
	{ -1 } /* end of array */
};

/***************************************************************************/

static void sound_irq(int state)
{
	if (state)
		timer_set(TIME_NOW,YM2151_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,YM2151_CLEAR,setvector_callback);
}

static struct YM2151interface ym2151_interface =
{
	1,
	14318180/4,
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct IremGA20_interface iremGA20_interface =
{
	14318180/4,
	REGION_SOUND1,
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) },
};

/***************************************************************************/

static int m107_interrupt(void)
{
	m107_vblank=0;
	m107_vh_raster_partial_refresh(Machine->scrbitmap,0,248);

	return m107_IRQ_0; /* VBL */
}

static int m107_raster_interrupt(void)
{
	static int last_line=0;
	int line = 256 - cpu_getiloops();

	if (keyboard_pressed_memory(KEYCODE_F1)) {
		raster_enable ^= 1;
		if (raster_enable)
			usrintf_showmessage("Raster IRQ enabled");
		else
			usrintf_showmessage("Raster IRQ disabled");
	}

	/* Raster interrupt */
	if (raster_enable && line==m107_raster_irq_position) {
		if (osd_skip_this_frame()==0)
			m107_vh_raster_partial_refresh(Machine->scrbitmap,last_line,line);
		last_line=line+1;

		return m107_IRQ_2;
	}

	/* Kludge to get Fire Barrel running */
	if (line==118)
		return m107_IRQ_3;

	/* Redraw screen, then set vblank and trigger the VBL interrupt */
	if (line==248) {
		if (osd_skip_this_frame()==0)
			m107_vh_raster_partial_refresh(Machine->scrbitmap,last_line,248);
		last_line=0;
		m107_vblank=1;
		return m107_IRQ_0;
	}

	/* End of vblank */
	if (line==255)
		m107_vblank=0;

	return 0;
}

static const struct MachineDriver machine_driver_firebarr =
{
	/* basic machine hardware */
	{
		{
			CPU_V33,	/* NEC V33 */
			28000000,	/* 28MHz clock */
			readmem,writemem,readport,writeport,
			m107_raster_interrupt,256 /* 8 prelines, 240 visible lines, 8 for vblank? */
		},
		{
			CPU_V30 | CPU_AUDIO_CPU,
			14318000,	/* 14.318 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 512, { 80, 511-112, 128+8, 511-128-8 }, /* 320 x 240 */

	firebarr_gfxdecodeinfo,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	m107_vh_start,
	m107_vh_stop,
	m107_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_IREMGA20,
			&iremGA20_interface
		}
	}
};

static const struct MachineDriver machine_driver_dsoccr94 =
{
	/* basic machine hardware */
	{
		{
			CPU_V33,	/* NEC V33 */
			20000000,	/* Could be 28MHz clock? */
			readmem,writemem,readport,writeport,
			m107_interrupt,1
		},
		{
			CPU_V30 | CPU_AUDIO_CPU,
			14318000,	/* 14.318 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 512, { 80, 511-112, 128+8, 511-128-8 }, /* 320 x 240 */

	gfxdecodeinfo,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	m107_vh_start,
	m107_vh_stop,
	m107_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_IREMGA20,
			&iremGA20_interface
		}
	}
};

/***************************************************************************/

ROM_START( firebarr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "f4-h0",  0x000001, 0x40000, 0x2aa5676e )
	ROM_LOAD16_BYTE( "f4-l0",  0x000000, 0x40000, 0x42f75d59 )
	ROM_LOAD16_BYTE( "f4-h1",  0x080001, 0x20000, 0xbb7f6968 )
	ROM_LOAD16_BYTE( "f4-l1",  0x080000, 0x20000, 0x9d57edd6 )

	ROM_REGION( 0x100000 * 2, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "f4-sh0", 0x000001, 0x10000, 0x30a8e232 )
	ROM_LOAD16_BYTE( "f4-sl0", 0x000000, 0x10000, 0x204b5f1f )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD16_BYTE( "f4-c00", 0x000000, 0x80000, 0x50cab384 )
	ROM_LOAD16_BYTE( "f4-c10", 0x000001, 0x80000, 0x330c6df2 )
	ROM_LOAD16_BYTE( "f4-c01", 0x100000, 0x80000, 0x12a698c8 )
	ROM_LOAD16_BYTE( "f4-c11", 0x100001, 0x80000, 0x3f9add18 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "f4-000", 0x000000, 0x80000, 0x920deee9 )
	ROM_LOAD16_BYTE( "f4-001", 0x000001, 0x80000, 0xe5725eaf )
	ROM_LOAD16_BYTE( "f4-010", 0x100000, 0x80000, 0x3505d185 )
	ROM_LOAD16_BYTE( "f4-011", 0x100001, 0x80000, 0x1912682f )
	ROM_LOAD16_BYTE( "f4-020", 0x200000, 0x80000, 0xec130b8e )
	ROM_LOAD16_BYTE( "f4-021", 0x200001, 0x80000, 0x8dd384dc )
	ROM_LOAD16_BYTE( "f4-030", 0x300000, 0x80000, 0x7e7b30cd )
	ROM_LOAD16_BYTE( "f4-031", 0x300001, 0x80000, 0x83ac56c5 )

	ROM_REGION( 0x40000, REGION_USER1, 0 )	/* sprite tables */
	ROM_LOAD16_BYTE( "f4-drh", 0x000001, 0x20000, 0x12001372 )
	ROM_LOAD16_BYTE( "f4-drl", 0x000000, 0x20000, 0x08cb7533 )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* ADPCM samples */
	ROM_LOAD( "f4-da0",          0x000000, 0x80000, 0x7a493e2e )
ROM_END

ROM_START( dsoccr94 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE("ds_h0-c.rom",  0x000001, 0x040000, 0xd01d3fd7 )
	ROM_LOAD16_BYTE("ds_l0-c.rom",  0x000000, 0x040000, 0x8af0afe2 )
	ROM_LOAD16_BYTE("ds_h1-c.rom",  0x100001, 0x040000, 0x6109041b )
	ROM_LOAD16_BYTE("ds_l1-c.rom",  0x100000, 0x040000, 0x97a01f6b )

	ROM_REGION( 0x100000 * 2, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE("ds_sh0.rom",   0x000001, 0x010000, 0x23fe6ffc )
	ROM_LOAD16_BYTE("ds_sl0.rom",   0x000000, 0x010000, 0x768132e5 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD16_BYTE("ds_c00.rom",   0x000000, 0x100000, 0x2d31d418 )
	ROM_LOAD16_BYTE("ds_c10.rom",   0x000001, 0x100000, 0x57f7bcd3 )
	ROM_LOAD16_BYTE("ds_c01.rom",   0x200000, 0x100000, 0x9d31a464 )
	ROM_LOAD16_BYTE("ds_c11.rom",   0x200001, 0x100000, 0xa372e79f )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ds_000.rom",   0x000000, 0x100000, 0x366b3e29 )
	ROM_LOAD( "ds_010.rom",   0x100000, 0x100000, 0x28a4cc40 )
	ROM_LOAD( "ds_020.rom",   0x200000, 0x100000, 0x5a310f7f )
	ROM_LOAD( "ds_030.rom",   0x300000, 0x100000, 0x328b1f45 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	 /* ADPCM samples */
	ROM_LOAD( "ds_da0.rom" ,  0x000000, 0x100000, 0x67fc52fd )
ROM_END

/***************************************************************************/

static void init_m107(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x7fff0,0x10); /* Start vector */
	cpu_setbank(1,&RAM[0xa0000]); /* Initial bank */

	RAM = memory_region(REGION_CPU2);
	memcpy(RAM+0xffff0,RAM+0x1fff0,0x10); /* Sound cpu Start vector */

	irem_cpu_decrypt(1,rtypeleo_decryption_table);

	m107_irq_vectorbase=0x20;
	m107_spritesystem = 1;

	raster_enable=1;
}

static void init_dsoccr94(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x7fff0,0x10); /* Start vector */
	cpu_setbank(1,&RAM[0xa0000]); /* Initial bank */

	RAM = memory_region(REGION_CPU2);
	memcpy(RAM+0xffff0,RAM+0x1fff0,0x10); /* Sound cpu Start vector */

	irem_cpu_decrypt(1,dsoccr94_decryption_table);

	m107_irq_vectorbase=0x80;
	m107_spritesystem = 0;

	/* This game doesn't use raster IRQ's */
	raster_enable=0;
}

/***************************************************************************/

GAMEX(1993, firebarr, 0, firebarr, firebarr, m107,     ROT270, "Irem", "Fire Barrel (Japan)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1994, dsoccr94, 0, dsoccr94, dsoccr94, dsoccr94, ROT0,   "Irem (Data East Corporation license)", "Dream Soccer '94" )
