/*
Championship VBall
Driver by Paul "TBBle" Hampson

TODO:
Needs to be tilemapped. The background layer and sprite layer are identical to spdodgeb, except for the
 back-switched graphics roms and the size of the pallete banks.
Someone needs to look at Naz's board, and see what PCM sound chips are present.
And get whatever's in the dip package on Naz's board. (BG/FG Roms, I hope)
I'd also love to know whether Naz's is a bootleg or is missing the story for a different reason (US release?)

*/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/z80/z80.h"
#include "sound/adpcm.h"
#include "sound/msm5205.h"

/* from vidhrdw */
extern unsigned char *vb_attribram;
extern unsigned char *vb_spriteram;
extern unsigned char *vb_videoram;
extern unsigned char *vb_fgattribram;
extern unsigned char *vb_scrollx_lo;
extern int vb_scrollx_hi;
extern int vball_gfxset;

int vb_vh_start(void);
void vb_vh_stop(void);
void vb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void vb_bgprombank_w(int bank);
extern void vb_spprombank_w(int bank);
extern WRITE_HANDLER( vb_foreground_w );
extern WRITE_HANDLER( vb_attrib_w );
extern WRITE_HANDLER( vb_fgattrib_w );
/* end of extern code & data */

/* private globals */
static int sound_irq, ym_irq;
static int adpcm_pos[2],adpcm_end[2],adpcm_idle[2];
/* end of private globals */

static void vb_init_machine( void ) {
	sound_irq = Z80_NMI_INT;
	ym_irq = 0;//-1000;

}

static WRITE_HANDLER( vb_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( data & 1 ) ) ] );

	if (vball_gfxset != ((data  & 0x20) ^ 0x20)) {
		vball_gfxset = (data  & 0x20) ^ 0x20;
		memset(dirtybuffer,1, 0x800);
	}

//	logerror("CPU #0 PC %04x: warning - write %02x to bankswitch memory address 1009\n",cpu_get_pc(),data);
}

/* The sound system comes all but verbatim from Double Dragon */


WRITE_HANDLER( cpu_sound_command_w ) {
	soundlatch_w( offset, data );
	cpu_cause_interrupt( 1, sound_irq );
	logerror("Sound_command_w\n");
}

static WRITE_HANDLER( dd_adpcm_w )
{
	int chip = 0;
	logerror("dd_adpcm_w: %d %d\n",offset, data);
	switch (offset)
	{
		case 0:
			adpcm_idle[chip] = 1;
			MSM5205_reset_w(chip,1);
			break;

		case 3:
			adpcm_pos[chip] = (data) * 0x200;
			adpcm_end[chip] = (data + 1) * 0x200;
			break;

		case 1:
			adpcm_idle[chip] = 0;
			MSM5205_reset_w(chip,0);
			break;
	}
}

static void dd_adpcm_int(int chip)
{
	static int adpcm_data[2] = { -1, -1 };
	if (adpcm_pos[chip] >= adpcm_end[chip] || adpcm_pos[chip] >= 0x10000)
	{
		adpcm_idle[chip] = 1;
		MSM5205_reset_w(chip,1);
	}
	else if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip,adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1) + 0x10000 * chip;

		adpcm_data[chip] = ROM[adpcm_pos[chip]++];
		MSM5205_data_w(chip,adpcm_data[chip] >> 4);
	}
}

static READ_HANDLER( dd_adpcm_status_r )
{
//	logerror("dd_adpcm_status_r\n");
	return adpcm_idle[0] + (adpcm_idle[1] << 1);
}


WRITE_HANDLER( vb_scrollx_hi_w )
{
	vb_scrollx_hi = (data & 0x02) << 7;
	vb_bgprombank_w((data >> 2)&0x07);
	vb_spprombank_w((data >> 5)&0x07);

//	logerror("vb_scrollx_hi %02x\n",data);

}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_0_r },
	{ 0x1001, 0x1001, input_port_1_r },
	{ 0x1002, 0x1002, input_port_2_r },
	{ 0x1003, 0x1003, input_port_3_r },
	{ 0x1004, 0x1004, input_port_4_r },
	{ 0x1005, 0x1005, input_port_5_r },
	{ 0x1006, 0x1006, input_port_6_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START( vball2pj_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_0_r },
	{ 0x1001, 0x1001, input_port_1_r },
	{ 0x1002, 0x1002, input_port_2_r },
	{ 0x1003, 0x1003, input_port_3_r },
	{ 0x1004, 0x1004, input_port_4_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x08ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1008, 0x1008, vb_scrollx_hi_w },
	{ 0x1009, 0x1009, vb_bankswitch_w },
	{ 0x100a, 0x100a, MWA_RAM },
	{ 0x100b, 0x100b, MWA_RAM },
	{ 0x100c, 0x100c, MWA_RAM, &vb_scrollx_lo },
	{ 0x100d, 0x100d, cpu_sound_command_w },
	{ 0x100e, 0x100e, MWA_RAM },
	{ 0x2000, 0x27ff, vb_foreground_w, &vb_videoram },
	{ 0x2800, 0x2fff, videoram_w, &videoram },
	{ 0x3000, 0x37ff, vb_fgattrib_w, &vb_fgattribram },
	{ 0x3800, 0x3fff, vb_attrib_w, &vb_attribram },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

static MEMORY_READ_START( vball2pj_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
//	{ 0x9800, 0x9800, dd_adpcm_status_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( vball2pj_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
//	{ 0x9800, 0x9807, dd_adpcm_w },
MEMORY_END

#define COMMON_PORTS_BEFORE  PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) \
  PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
  PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \

#define COMMON_PORTS_COINS  PORT_START \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A )) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C )) \
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C )) \
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C )) \
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C )) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C )) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C )) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C )) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C )) \
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B )) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C )) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C )) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C )) \
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C )) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C )) \
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C )) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C )) \
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C )) \
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen )) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x40, DEF_STR( On )) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds )) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x80, DEF_STR( On )) \

INPUT_PORTS_START (vball)

COMMON_PORTS_BEFORE
/* The dipswitch instructions in naz's dump (vball) don't quite sync here) */
/* Looks like the pins from the dips to the board were mixed up a little. */

  PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ))
// This ordering is assumed. Someone has to play it a lot and find out.
	PORT_DIPSETTING(    0x01, "Easy")
	PORT_DIPSETTING(    0x00, "Medium")
	PORT_DIPSETTING(    0x02, "Hard")
	PORT_DIPSETTING(    0x03, "Very Hard")
	PORT_DIPNAME( 0x0c, 0x00, "Single Player Game Time")
	PORT_DIPSETTING(    0x00, "1:15")
	PORT_DIPSETTING(    0x04, "1:30")
	PORT_DIPSETTING(    0x0c, "1:45")
	PORT_DIPSETTING(    0x08, "2:00")
	PORT_DIPNAME( 0x30, 0x00, "Start Buttons (4-player)")
	PORT_DIPSETTING(    0x00, "Normal")
	PORT_DIPSETTING(    0x20, "Button A")
	PORT_DIPSETTING(    0x10, "Button B")
	PORT_DIPSETTING(    0x30, "Normal")
	PORT_DIPNAME( 0x40, 0x40, "PL 1&4 (4-player)")
	PORT_DIPSETTING(    0x40, "Normal")
	PORT_DIPSETTING(    0x00, "Rot 90")
	PORT_DIPNAME( 0x80, 0x00, "Player Mode")
	PORT_DIPSETTING(    0x80, "2")
	PORT_DIPSETTING(    0x00, "4")

COMMON_PORTS_COINS

  PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
  PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START (vball2pj)

COMMON_PORTS_BEFORE

/* The 2-player roms have the game-time in the difficulty spot, and
   I've assumed vice-versa. (VS the instructions scanned in Naz's dump)
*/
  PORT_START
	PORT_DIPNAME( 0x03, 0x00, "Single Player Game Time")
	PORT_DIPSETTING(    0x00, "1:30")
	PORT_DIPSETTING(    0x01, "1:45")
	PORT_DIPSETTING(    0x03, "2:00")
	PORT_DIPSETTING(    0x02, "2:15")
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Difficulty ))
// This ordering is assumed. Someone has to play it a lot and find out.
	PORT_DIPSETTING(    0x04, "Easy")
	PORT_DIPSETTING(    0x00, "Medium")
	PORT_DIPSETTING(    0x08, "Hard")
	PORT_DIPSETTING(    0x0c, "Very Hard")

COMMON_PORTS_COINS
INPUT_PORTS_END


#define CHAR_LAYOUT( name, num ) \
	static struct GfxLayout name = \
	{ \
		8,8, /* 8*8 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ 0, 2, 4, 6 }, /* plane offset */ \
		{ 1, 0, 65, 64, 129, 128, 193, 192 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },	\
		32*8 /* every char takes 32 consecutive bytes */ \
	};

#define TILE_LAYOUT( name, num, planeoffset ) \
	static struct GfxLayout name = \
	{ \
		16,16, /* 16x16 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ planeoffset*8+0, planeoffset*8+4, 0,4 }, /* plane offset */ \
		{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0, \
	          32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, \
	          8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 }, \
		64*8 /* every char takes 64 consecutive bytes */ \
	};


CHAR_LAYOUT( vb_char_layout, 16384 ) /* foreground chars */
TILE_LAYOUT( vb_sprite_layout, 2048, 0x20000 ) /* sprites */

static struct GfxDecodeInfo vb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &vb_char_layout,	  0, 8 },	/* 8x8 chars */
	{ REGION_GFX2, 0, &vb_sprite_layout,	128, 8 },	/* 16x16 sprites */
	{ -1 } // end of array
};

static void vball_irq_handler(int irq) {
		cpu_set_irq_line( 1, ym_irq , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ vball_irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 6000 },           /* frequency (Hz) */
	{ REGION_SOUND1 },  /* memory region */
	{ 20000 }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 2 chips             */
	384000,				/* 384KHz             */
	{ dd_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B },	/* 8kHz and 6kHz      */
	{ 40 }				/* volume */
};

int vball_interrupt(void)
{
	int line = 33 - cpu_getiloops();

	if (line < 30)
	{
//		scrollx[line] = lastscroll;
		return M6502_INT_IRQ;
	}
	else if (line == 30)	/* vblank */
		return M6502_INT_NMI;
	else 	/* skip 31 32 33 to allow vblank to finish */
		return ignore_interrupt();
}


static struct MachineDriver machine_driver_vball =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6502,
			3579545,	/* 3.579545 MHz */
			readmem,writemem,0,0,
			vball_interrupt,34	/* 1 IRQ every 8 visible scanlines, plus NMI for vblank */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	vb_init_machine,

	/* video hardware */
	32*8, 32*8,{ 0*8, 32*8-1, 0*8, 32*8-1 },
	vb_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	vb_vh_start,
	vb_vh_stop,
	vb_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
/* This is here purely based on what the Z80 seems to be doing. And the fact that it works */
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_vball2pj =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6502,
			3579545,	/* 3.579545 MHz */
			vball2pj_readmem,writemem,0,0,
			vball_interrupt,34	/* 1 IRQ every 8 visible scanlines, plus NMI for vblank */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			vball2pj_sound_readmem,vball2pj_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	vb_init_machine,

	/* video hardware */
	32*8, 32*8,{ 0*8, 32*8-1, 0*8, 32*8-1 },
	vb_gfxdecodeinfo,
	256, 256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	vb_vh_start,
	vb_vh_stop,
	vb_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( vball )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* Main CPU: 64k for code */
	ROM_LOAD( "vball.124",  0x10000, 0x08000, 0xbe04c2b5 )/* Bankswitched */
	ROM_CONTINUE(		0x08000, 0x08000 )		 /* Static code  */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* region#2: music CPU, 64kb */
	ROM_LOAD( "vball.47",  0x00000, 0x8000,  0x10ca79ad )

	/* These are from the bootleg */
	ROM_REGION(0x80000, REGION_GFX1, ROMREGION_DISPOSE )	 /* fg tiles */
	ROM_LOAD( "vball13.bin",  0x00000, 0x10000, 0xf26df8e1 ) /* 0,1,2,3 */
	ROM_LOAD( "vball14.bin",  0x10000, 0x10000, 0xc9798d0e ) /* 0,1,2,3 */
	ROM_LOAD( "vball15.bin",  0x20000, 0x10000, 0x68e69c4b ) /* 0,1,2,3 */
	ROM_LOAD( "vball16.bin",  0x30000, 0x10000, 0x936457ba ) /* 0,1,2,3 */
	ROM_LOAD( "vball09.bin",  0x40000, 0x10000, 0x42874924 ) /* 0,1,2,3 */
	ROM_LOAD( "vball10.bin",  0x50000, 0x10000, 0x6cc676ee ) /* 0,1,2,3 */
	ROM_LOAD( "vball11.bin",  0x60000, 0x10000, 0x4754b303 ) /* 0,1,2,3 */
	ROM_LOAD( "vball12.bin",  0x70000, 0x10000, 0x21294a84 ) /* 0,1,2,3 */
	/* This is how I'd expect they were originally */
//	ROM_LOAD( "bgdata1",	  0x00000, 0x20000, 0x0 )
//	ROM_LOAD( "bgdata2",	  0x20000, 0x20000, 0x0 )
//	ROM_LOAD( "bgdata3",	  0x40000, 0x20000, 0x0 )
//	ROM_LOAD( "bgdata4",	  0x60000, 0x20000, 0x0 )

	ROM_REGION(0x40000, REGION_GFX2, ROMREGION_DISPOSE )	 /* sprites */
	ROM_LOAD( "vball.35",  0x00000, 0x20000, 0x877826d8 ) /* 0,1,2,3 */
	ROM_LOAD( "vball.5",   0x20000, 0x20000, 0xc6afb4fa ) /* 0,1,2,3 */

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Sound region#1: adpcm */
	ROM_LOAD( "vball.78a",  0x00000, 0x10000, 0xf3e63b76 )
	ROM_LOAD( "vball.78b",  0x10000, 0x10000, 0x7ad9d338 )

	ROM_REGION(0x1000, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD_NIB_LOW ( "vball.44",   0x0000, 0x00800, 0xa317240f )
	ROM_LOAD_NIB_HIGH( "vball.43",   0x0000, 0x00800, 0x1ff70b4f )
	ROM_LOAD( "vball.160",  0x0800, 0x00800, 0x2ffb68b3 )
ROM_END

ROM_START( vball2pj )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* Main CPU */
	ROM_LOAD( "vball01.bin",  0x10000, 0x08000,  0x432509c4 )/* Bankswitched */
	ROM_CONTINUE(		  0x08000, 0x08000 )		 /* Static code  */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* region#2: music CPU, 64kb */
	ROM_LOAD( "vball04.bin",  0x00000, 0x8000,  0x534dfbd9 )

	ROM_REGION(0x80000, REGION_GFX1, ROMREGION_DISPOSE )	 /* fg tiles */
	ROM_LOAD( "vball13.bin",  0x00000, 0x10000, 0xf26df8e1 ) /* 0,1,2,3 */
	ROM_LOAD( "vball14.bin",  0x10000, 0x10000, 0xc9798d0e ) /* 0,1,2,3 */
	ROM_LOAD( "vball15.bin",  0x20000, 0x10000, 0x68e69c4b ) /* 0,1,2,3 */
	ROM_LOAD( "vball16.bin",  0x30000, 0x10000, 0x936457ba ) /* 0,1,2,3 */
	ROM_LOAD( "vball09.bin",  0x40000, 0x10000, 0x42874924 ) /* 0,1,2,3 */
	ROM_LOAD( "vball10.bin",  0x50000, 0x10000, 0x6cc676ee ) /* 0,1,2,3 */
	ROM_LOAD( "vball11.bin",  0x60000, 0x10000, 0x4754b303 ) /* 0,1,2,3 */
	ROM_LOAD( "vball12.bin",  0x70000, 0x10000, 0x21294a84 ) /* 0,1,2,3 */

	ROM_REGION(0x40000, REGION_GFX2, ROMREGION_DISPOSE )	 /* sprites */
	ROM_LOAD( "vball08.bin",  0x00000, 0x10000, 0xb18d083c ) /* 0,1,2,3 */
	ROM_LOAD( "vball07.bin",  0x10000, 0x10000, 0x79a35321 ) /* 0,1,2,3 */
	ROM_LOAD( "vball06.bin",  0x20000, 0x10000, 0x49c6aad7 ) /* 0,1,2,3 */
	ROM_LOAD( "vball05.bin",  0x30000, 0x10000, 0x9bb95651 ) /* 0,1,2,3 */

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Sound region#1: adpcm */
	ROM_LOAD( "vball.78a",  0x00000, 0x10000, 0xf3e63b76 )
	ROM_LOAD( "vball.78b",  0x10000, 0x10000, 0x7ad9d338 )

	ROM_REGION(0x1000, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD_NIB_LOW ( "vball.44",   0x0000, 0x00800, 0xa317240f )
	ROM_LOAD_NIB_HIGH( "vball.43",   0x0000, 0x00800, 0x1ff70b4f )
	ROM_LOAD( "vball.160",  0x0800, 0x00800, 0x2ffb68b3 )
ROM_END


GAMEX( 1988, vball,    0,     vball,    vball,    0, ROT0, "Technos", "U.S. Championship V'ball (set 1)", GAME_NO_COCKTAIL )
GAMEX( 1988, vball2pj, vball, vball2pj, vball2pj, 0, ROT0, "Technos", "U.S. Championship V'ball (Japan bootleg)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
