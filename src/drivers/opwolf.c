/****************************************************************************

Operation Wolf  (c) Taito 1987
==============

David Graves, Jarek Burczynski

Sources:		MAME Rastan driver
			MAME Taito F2 driver
			Raine source - many thanks to Richard Bush
			  and the Raine Team.

Main CPU: MC68000 uses irq 5.
Sound   : Z80 & YM2151 & MSM5205


Operation Wolf uses similar hardware to Rainbow Islands and Rastan.
The screen layout and registers and sprites appear to be identical.

The original game has a c-chip like Rainbow. To emulate this we
follow Raine and emulate the extra Z80 from the *bootleg* board,
which acts as a substitute c-chip. $800 bytes of the Z80's ram
space are shared, mapped to $800 words in the 68000 address space
used to access the c-chip.


Gun Travel
----------

Horizontal gun travel maybe could be widened to include more
of the status bar (you can shoot enemies underneath it).

To keep the input span 0-255 a multiplier (300/256 ?)
would be used.


TODO
====

Need to verify Opwolf against original board: various reports
claim there are discrepancies (perhaps limitations of the fake
Z80 c-chip substitute to blame?).

There are a few unmapped writes for the sound Z80 in the log.

What number should be returned for the c-chip Z80 interrupt?

Raine source has standard Asuka/Mofflot sprite/tile priority:
0x2000 in sprite_ctrl puts all sprites under top bg layer. But
Raine simply kludges in this value, failing to read it from a
register. So what is controlling priority.


***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

static data8_t *cchip_ram;

WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rastan_spriteflip_w );

VIDEO_START( opwolf );
VIDEO_EOF( opwolf );
VIDEO_UPDATE( opwolf );

static int opwolf_gun_xoffs,opwolf_gun_yoffs;

static READ16_HANDLER( cchip_r )
{
	return cchip_ram[offset];
}

static WRITE16_HANDLER( cchip_w )
{
	cchip_ram[offset] = data &0xff;
}

/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( opwolf_lightgun_r )
{
	switch (offset)
	{
		case 0x00:	/* P1X */
			return (input_port_5_word_r(0,mem_mask) + 0x15 + opwolf_gun_xoffs);
		case 0x01:	/* P1Y */
			return (input_port_6_word_r(0,mem_mask) - 0x24 + opwolf_gun_yoffs);
	}

	return 0xff;
}

static READ_HANDLER( z80_input1_r )
{
	return input_port_0_word_r(0,0);	/* irrelevant mirror ? */
}

static READ_HANDLER( z80_input2_r )
{
	return input_port_0_word_r(0,0);	/* needed for coins */
}


/******************************************************
				SOUND
******************************************************/

static WRITE_HANDLER( sound_bankswitch_w )
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + ((data-1) & 0x03) * 0x4000 + 0x10000 );
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( opwolf_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0f0008, 0x0f0009, input_port_0_word_r },	/* IN0 */
	{ 0x0f000a, 0x0f000b, input_port_1_word_r },	/* IN1 */
	{ 0x0ff000, 0x0fffff, cchip_r },
	{ 0x100000, 0x107fff, MRA16_RAM },	/* RAM */
	{ 0x200000, 0x200fff, paletteram16_word_r },
	{ 0x380000, 0x380001, input_port_2_word_r },	/* DSW A */
	{ 0x380002, 0x380003, input_port_3_word_r },	/* DSW B */
	{ 0x3a0000, 0x3a0003, opwolf_lightgun_r },	/* lightgun, read at $11e0/6 */
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_r },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( opwolf_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0ff000, 0x0fffff, cchip_w },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x200000, 0x200fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x380000, 0x380003, rainbow_spritectrl_w },	// usually 0x4, changes when you fire
	{ 0x3c0000, 0x3c0001, MWA16_NOP },	/* watchdog ?? */
	{ 0x3e0000, 0x3e0001, taitosound_port16_msb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_w },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc10000, 0xc1ffff, MWA16_RAM },	/* error in init code (?) */
	{ 0xc20000, 0xc20003, PC080SN_yscroll_word_0_w },
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xc50000, 0xc50003, PC080SN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END

/***************************************************************************
	This extra Z80 substitutes for the c-chip in the bootleg,
	but we also use it as a fake c-chip to get the original
	working. */

static MEMORY_READ_START( sub_z80_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8800, 0x8800, z80_input1_r },	/* read at PC=$637: poked to $c004 */
	{ 0x9800, 0x9800, z80_input2_r },	/* read at PC=$631: poked to $c005 */
	{ 0xc000, 0xcfff, MRA_RAM },	// does upper half exist ?
MEMORY_END

static MEMORY_WRITE_START( sub_z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x9000, 0x9000, MWA_NOP },	/* unknown write, 0 then 1 each interrupt */
	{ 0xa000, 0xa000, MWA_NOP },	/* unknown write, once per interrupt */
	{ 0xc000, 0xcfff, MWA_RAM, &cchip_ram },
MEMORY_END

/***************************************************************************/

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static UINT8 adpcm_b[0x08];
static UINT8 adpcm_c[0x08];

//static unsigned char adpcm_d[0x08];
//0 - start ROM offset LSB
//1 - start ROM offset MSB
//2 - end ROM offset LSB
//3 - end ROM offset MSB
//start & end need to be multiplied by 16 to get a proper _byte_ address in adpcm ROM
//4 - always zero write (start trigger ?)
//5 - different values
//6 - different values

static WRITE_HANDLER( opwolf_adpcm_b_w )
{
	int start;
	int end;

	adpcm_b[offset] = data;

	if (offset==0x04) //trigger ?
	{
		start = adpcm_b[0] + adpcm_b[1]*256;
		end   = adpcm_b[2] + adpcm_b[3]*256;
		start *=16;
		end   *=16;
		ADPCM_play(0,start,(end-start)*2);
	}

	/*logerror("CPU #1     b00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );*/
}


static WRITE_HANDLER( opwolf_adpcm_c_w )
{
	int start;
	int end;

	adpcm_c[offset] = data;

	if (offset==0x04) //trigger ?
	{
		start = adpcm_c[0] + adpcm_c[1]*256;
		end   = adpcm_c[2] + adpcm_c[3]*256;
		start *=16;
		end   *=16;
		ADPCM_play(1,start,(end-start)*2);
	}

	/*logerror("CPU #1     c00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );*/
}


static WRITE_HANDLER( opwolf_adpcm_d_w )
{
	logerror("CPU #1         d00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}

static WRITE_HANDLER( opwolf_adpcm_e_w )
{
	logerror("CPU #1         e00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}


static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb006, opwolf_adpcm_b_w },
	{ 0xc000, 0xc006, opwolf_adpcm_c_w },
	{ 0xd000, 0xd000, opwolf_adpcm_d_w },
	{ 0xe000, 0xe000, opwolf_adpcm_e_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( opwolf )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "NY Conversion of Upright" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Ammo Magazines at Start" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )	// Manual says all 3 unused
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	PORT_START	/* Fake DSW */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Show gun target", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_START	/* P1X (span allows you to shoot enemies behind status bar) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 25, 15, 0x00, 0xff)

	PORT_START	/* P1Y (span allows you to be slightly offscreen) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 25, 15, 0x00, 0xff)
INPUT_PORTS_END


/**************************************************************
				GFX DECODING
**************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4, 10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout_b =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout_b =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo opwolf_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo opwolfb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout_b,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout_b,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2151 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irq_handler },
	{ sound_bankswitch_w }
};


static struct ADPCMinterface adpcm_interface =
{
	2,			/* 2 chips ?? */
	8000,       /* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 60, 60 }	/* volume - guessed  */
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( opwolf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(opwolf_readmem,opwolf_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_CPU_ADD(Z80, 4000000)	/* fake, not present on the original board */
	MDRV_CPU_MEMORY(sub_z80_readmem,sub_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolf_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_EOF(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( opwolfb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(opwolf_readmem,opwolf_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sub_z80_readmem,sub_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolfb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_EOF(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( opwolf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "opwlf.40",  0x00000, 0x10000, 0x3ffbfe3a )
	ROM_LOAD16_BYTE( "opwlf.30",  0x00001, 0x10000, 0xfdabd8a5 )
	ROM_LOAD16_BYTE( "opwlf.39",  0x20000, 0x10000, 0x216b4838 )
	ROM_LOAD16_BYTE( "opwlf.29",  0x20001, 0x10000, 0xb71bc44c )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "opwlf_s.10",  0x00000, 0x04000, 0x45c7ace3 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )      /* fake Z80 from the bootleg */
	ROM_LOAD( "opwlfb.09",   0x00000, 0x08000, 0xab27a3dd )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "opwlf.13",  0x00000, 0x80000, 0xf6acdab1 )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "opwlf.72",  0x00000, 0x80000, 0x89f889e5 )	/* Sprites (16 x 16) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "opwlf_s.21",   0x00000, 0x80000, 0xf3e19c64 )
ROM_END

ROM_START( opwolfb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "opwlfb.12",  0x00000, 0x10000, 0xd87e4405 )
	ROM_LOAD16_BYTE( "opwlfb.10",  0x00001, 0x10000, 0x9ab6f75c )
	ROM_LOAD16_BYTE( "opwlfb.13",  0x20000, 0x10000, 0x61230c6e )
	ROM_LOAD16_BYTE( "opwlfb.11",  0x20001, 0x10000, 0x342e318d )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "opwlfb.30",  0x00000, 0x04000, 0x0669b94c )
	ROM_CONTINUE(           0x10000, 0x04000 ) /* banked stuff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )      /* c-chip substitute Z80 */
	ROM_LOAD( "opwlfb.09",   0x00000, 0x08000, 0xab27a3dd )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.08",   0x00000, 0x10000, 0x134d294e )	/* SCR tiles (8 x 8) */
	ROM_LOAD16_BYTE( "opwlfb.06",   0x20000, 0x10000, 0x317d0e66 )
	ROM_LOAD16_BYTE( "opwlfb.07",   0x40000, 0x10000, 0xe1c4095e )
	ROM_LOAD16_BYTE( "opwlfb.05",   0x60000, 0x10000, 0xfd9e72c8 )
	ROM_LOAD16_BYTE( "opwlfb.04",   0x00001, 0x10000, 0xde0ca98d )
	ROM_LOAD16_BYTE( "opwlfb.02",   0x20001, 0x10000, 0x6231fdd0 )
	ROM_LOAD16_BYTE( "opwlfb.03",   0x40001, 0x10000, 0xccf8ba80 )
	ROM_LOAD16_BYTE( "opwlfb.01",   0x60001, 0x10000, 0x0a65f256 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.14",   0x00000, 0x10000, 0x663786eb )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "opwlfb.15",   0x20000, 0x10000, 0x315b8aa9 )
	ROM_LOAD16_BYTE( "opwlfb.16",   0x40000, 0x10000, 0xe01099e3 )
	ROM_LOAD16_BYTE( "opwlfb.17",   0x60000, 0x10000, 0x56fbe61d )
	ROM_LOAD16_BYTE( "opwlfb.18",   0x00001, 0x10000, 0xde9ab08e )
	ROM_LOAD16_BYTE( "opwlfb.19",   0x20001, 0x10000, 0x645cf85e )
	ROM_LOAD16_BYTE( "opwlfb.20",   0x40001, 0x10000, 0xd80b9cc6 )
	ROM_LOAD16_BYTE( "opwlfb.21",   0x60001, 0x10000, 0x97d25157 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples (interleaved) */
	ROM_LOAD16_BYTE( "opwlfb.29",   0x00000, 0x10000, 0x05a9eac0 )
	ROM_LOAD16_BYTE( "opwlfb.28",   0x20000, 0x10000, 0x281b2175 )
	ROM_LOAD16_BYTE( "opwlfb.27",   0x40000, 0x10000, 0x441211a6 )
	ROM_LOAD16_BYTE( "opwlfb.26",   0x60000, 0x10000, 0x86d1d42d )
	ROM_LOAD16_BYTE( "opwlfb.25",   0x00001, 0x10000, 0x85b87f58 )
	ROM_LOAD16_BYTE( "opwlfb.24",   0x20001, 0x10000, 0x8efc5d4d )
	ROM_LOAD16_BYTE( "opwlfb.23",   0x40001, 0x10000, 0xa874c703 )
	ROM_LOAD16_BYTE( "opwlfb.22",   0x60001, 0x10000, 0x9228481f )
ROM_END


static DRIVER_INIT( opwolf )
{
	opwolf_gun_xoffs = 0;
	opwolf_gun_yoffs = 0;

	/* (there are other sound vars that may need saving too) */
	state_save_register_UINT8("sound2", 0, "registers", adpcm_b, 8);
	state_save_register_UINT8("sound3", 0, "registers", adpcm_c, 8);
}

static DRIVER_INIT( opwolfb )
{
	/* bootleg needs different range of raw gun coords */
	opwolf_gun_xoffs = -2;
	opwolf_gun_yoffs = 17;

	/* (there are other sound vars that may need saving too) */
	state_save_register_UINT8("sound2", 0, "registers", adpcm_b, 8);
	state_save_register_UINT8("sound3", 0, "registers", adpcm_c, 8);
}



/*    year  rom       parent    machine   inp       init */
GAME( 1987, opwolf,   0,        opwolf,   opwolf,   opwolf,   ROT0, "Taito America Corp", "Operation Wolf (US)" )
GAME( 1987, opwolfb,  opwolf,   opwolfb,  opwolf,   opwolfb,  ROT0, "bootleg", "Operation Bear" )
