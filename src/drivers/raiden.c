/***************************************************************************

	Raiden							(c) 1990 Seibu Kaihatsu
	Raiden (Alternate)				(c) 1990 Seibu Kaihatsu

	There is an additional romset with a decrypted sound cpu but encrypted
	program roms (same as the alternate set above).

	To access test mode, reset with both start buttons held.

	Coins are controlled by the sound cpu, there is a small kludge to allow
	the game to coin up if sound is off.  (Gives 9 coins though!)

	Raiden (Alternate) has a different memory map, and different fix char
	layer memory layout!

	Todo: add support for Coin Mode B

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

int raiden_background_r(int offset);
int raiden_foreground_r(int offset);
void raiden_background_w(int offset,int data);
void raiden_foreground_w(int offset,int data);
void raiden_text_w(int offset,int data);
void raidena_text_w(int offset,int data);
int raiden_vh_start(void);
void raiden_control_w(int offset, int data);
void raiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static unsigned char *raiden_shared_ram,*sound_shared_ram;
extern unsigned char *raiden_back_data,*raiden_fore_data,*raiden_scroll_ram;

/***************************************************************************/

static int raiden_shared_r(int offset) { return raiden_shared_ram[offset]; }
static void raiden_shared_w(int offset,int data) { raiden_shared_ram[offset]=data; }

static int raiden_soundcpu_r(int offset)
{
	int erg,orig;
	orig=sound_shared_ram[offset];

	/* Small kludge to allows coins with sound off */
	if (offset==4 && (!Machine->sample_rate) && (readinputport(4)&1)!=1) return 1;

	switch (offset)
	{/* misusing $d006 as a latch...but it works !*/
		case 0x04:{erg=sound_shared_ram[6];sound_shared_ram[6]=0;break;} /* just 1 time */
		case 0x06:{erg=0xa0;break;}
		case 0x0a:{erg=0;break;}
		default: erg=sound_shared_ram[offset];
	}
	if (errorlog) fprintf(errorlog,"Main <- Sound from $d0%02x %04x (%04x)\n",offset,erg,orig);
	return erg;
}

static void raiden_soundcpu_w(int offset, int data)
{
	sound_shared_ram[offset]=data;
	// if write to $d00c cause irq
	if (offset==0xc && data!=0xff) {
		if (errorlog) fprintf(errorlog,"Sound latch %02x %02x\n",sound_shared_ram[0],sound_shared_ram[2]);
		cpu_cause_interrupt(2,0xdf); /* RST 18 */
	}
}

static int maincpu_data_r(int offset)
{
	if (errorlog) fprintf(errorlog,"Sound <- Main from $d0%02x %4x\n",offset<<1,sound_shared_ram[offset<<1]);
	return sound_shared_ram[offset<<1];
	// main: $d000 + offset*2
	// 4010 -> d000
	// 4011 -> d002
}

static void maincpu_data_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog,"Sound -> Main at $d0%02x %4x\n",offset<<1,data);
	sound_shared_ram[offset<<1]=data;
}

static void sound_bank_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[2];

	if (data&1) { cpu_setbank(1,&RAM[0x0000]); }
	else { cpu_setbank(1,&RAM[0x8000]); }
}

static void sound_clear_w(int offset,int data)
{
	sound_shared_ram[0]=data;
}

/******************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x00000, 0x07fff, MRA_RAM },
	{ 0x0a000, 0x0afff, raiden_shared_r },
	{ 0x0b000, 0x0b000, input_port_0_r },
	{ 0x0b001, 0x0b001, input_port_1_r },
	{ 0x0b002, 0x0b002, input_port_2_r },
	{ 0x0b003, 0x0b003, input_port_3_r },
	{ 0x0d000, 0x0d00f, raiden_soundcpu_r },
	{ 0xa0000, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x00000, 0x06fff, MWA_RAM },
	{ 0x07000, 0x07fff, MWA_RAM, &spriteram },
	{ 0x0a000, 0x0afff, raiden_shared_w, &raiden_shared_ram },
	{ 0x0b000, 0x0b007, raiden_control_w },
	{ 0x0c000, 0x0c7ff, raiden_text_w, &videoram },
	{ 0x0d000, 0x0d00f, raiden_soundcpu_w, &sound_shared_ram },
	{ 0x0d060, 0x0d067, MWA_RAM, &raiden_scroll_ram },
	{ 0xa0000, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sub_readmem[] =
{
	{ 0x00000, 0x01fff, MRA_RAM },
	{ 0x02000, 0x027ff, raiden_background_r },
	{ 0x02800, 0x02fff, raiden_foreground_r },
	{ 0x03000, 0x03fff, paletteram_r },
	{ 0x04000, 0x04fff, raiden_shared_r },
	{ 0xc0000, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sub_writemem[] =
{
	{ 0x00000, 0x01fff, MWA_RAM },
	{ 0x02000, 0x027ff, raiden_background_w, &raiden_back_data },
	{ 0x02800, 0x02fff, raiden_foreground_w, &raiden_fore_data },
	{ 0x03000, 0x03fff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0x04000, 0x04fff, raiden_shared_w },
	{ 0x07ffe, 0x0afff, MWA_NOP },
	{ 0xc0000, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/************************* Alternate board set ************************/

static struct MemoryReadAddress alt_readmem[] =
{
	{ 0x00000, 0x07fff, MRA_RAM },
	{ 0x08000, 0x08fff, raiden_shared_r },
	{ 0x0e000, 0x0e000, input_port_0_r },
	{ 0x0e001, 0x0e001, input_port_1_r },
	{ 0x0e002, 0x0e002, input_port_2_r },
	{ 0x0e003, 0x0e003, input_port_3_r },
	{ 0x0d000, 0x0d00f, MWA_NOP },
	{ 0xa0000, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress alt_writemem[] =
{
	{ 0x00000, 0x06fff, MWA_RAM },
	{ 0x07000, 0x07fff, MWA_RAM, &spriteram },
	{ 0x08000, 0x08fff, raiden_shared_w, &raiden_shared_ram },
	{ 0x0b000, 0x0b007, raiden_control_w },
	{ 0x0c000, 0x0c7ff, raidena_text_w, &videoram },
	{ 0x0d000, 0x0d00f, MWA_NOP },
	{ 0x0f000, 0x0f035, MWA_RAM, &raiden_scroll_ram },
	{ 0xa0000, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4011, maincpu_data_r }, /* Soundlatch (16 bits) */
	{ 0x4013, 0x4013, input_port_4_r }, /* Coins */
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, sound_clear_w }, /* Or command ack?? */
	{ 0x4002, 0x4002, MWA_NOP }, /* RST 10h interrupt ack */
	{ 0x4003, 0x4003, MWA_NOP }, /* RST 18h interrupt ack */
	{ 0x4007, 0x4007, sound_bank_w },
	{ 0x4008, 0x4008, YM3812_control_port_0_w },
	{ 0x4009, 0x4009, YM3812_write_port_0_w },
	{ 0x4018, 0x401f, maincpu_data_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ -1 }	/* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( raiden_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Dip switch A */
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "A" )
	PORT_DIPSETTING(    0x00, "B" )
/* Coin Mode A */
	PORT_DIPNAME( 0x1e, 0x1e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, "8 Coins/3 Credit" )
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, "5 Coins/3 Credirs" )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

/* Coin Mode B */
/*	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, "5C/1C or Free if Coin B too" )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1C/6C or Free if Coin A too" ) */

	PORT_DIPNAME( 0x20, 0x20, "Credits to Start" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "80000 300000" )
	PORT_DIPSETTING(    0x0c, "150000 400000" )
	PORT_DIPSETTING(    0x04, "300000 1000000" )
	PORT_DIPSETTING(    0x00, "1000000 5000000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* Coins */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout raiden_charlayout =
{
	8,8,		/* 8*8 characters */
	2048,		/* 512 characters */
	4,			/* 4 bits per pixel */
	{ 4,0,(0x08000*8)+4,0x08000*8  },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128
};

static struct GfxLayout raiden_spritelayout =
{
  16,16,	/* 16*16 tiles */
  4096,		/* 2048*4 tiles */
  4,		/* 4 bits per pixel */
  { 12, 8, 4, 0 },
  {
    0,1,2,3, 16,17,18,19,
	512+0,512+1,512+2,512+3,
	512+8+8,512+9+8,512+10+8,512+11+8,
  },
  {
	0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
  },
  1024
};

static struct GfxDecodeInfo raiden_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &raiden_charlayout,      768, 16 },
	{ 1, 0x020000, &raiden_spritelayout,      0, 16 },
	{ 1, 0x0a0000, &raiden_spritelayout,    256, 16 },
	{ 1, 0x120000, &raiden_spritelayout,    512, 16 },
	{ -1 } /* end of array */
};

/******************************************************************************/

/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void YM3812_irqhandler(int linestate)
{
	cpu_irq_line_vector_w(2,0,0xd7); /* RST 10h */
	cpu_set_irq_line(2,0,linestate);
	//cpu_cause_interrupt(2,0xd7); /* RST 10h */
}

static struct YM3812interface ym3812_interface =
{
	1,
	3600000,
	{ 50 },
	{ YM3812_irqhandler },
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 8000 },
	{ 4 },
	{ 40 }
};

static int raiden_interrupt(void)
{
	return 0xc8/4;	/* VBL */
}

static struct MachineDriver raiden_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30, /* NEC V30 CPU */
			14000000, /* 10MHz, but cycle counts are wrong in the core */
			0,
			readmem,writemem,0,0,
			raiden_interrupt,1
		},
		{
			CPU_V30, /* NEC V30 CPU */
			14000000, /* 10MHz, but cycle counts are wrong in the core */
			3,
			sub_readmem,sub_writemem,0,0,
			raiden_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU, /* Don't remove the CPU_AUDIO_CPU :P */
			4000000, /* Although it controls coins, there is a free play dipswitch */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	120,	/* CPU interleave  */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	raiden_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	raiden_vh_start,
	0,
	raiden_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver raidena_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30, /* NEC V30 CPU */
			14000000, /* 10MHz, but cycle counts are wrong in the core */
			0,
			alt_readmem,alt_writemem,0,0,
			raiden_interrupt,1
		},
		{
			CPU_V30, /* NEC V30 CPU */
			14000000, /* 10MHz, but cycle counts are wrong in the core */
			3,
			sub_readmem,sub_writemem,0,0,
			raiden_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU, /* Don't remove the CPU_AUDIO_CPU :P */
			4000000, /* Although it controls coins, there is a free play dipswitch */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	120,	/* CPU interleave  */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	raiden_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	raiden_vh_start,
	0,
	raiden_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/***************************************************************************/

ROM_START( raiden_rom )
	ROM_REGION(0x100000) /* Region 0 - v30 main cpu */
	ROM_LOAD_GFX_EVEN( "rai1.bin",   0x0a0000, 0x10000, 0xa4b12785 )
	ROM_LOAD_GFX_ODD ( "rai2.bin",   0x0a0000, 0x10000, 0x17640bd5 )
	ROM_LOAD_GFX_EVEN( "rai3.bin",   0x0c0000, 0x20000, 0x9d735bf5 )
	ROM_LOAD_GFX_ODD ( "rai4.bin",   0x0c0000, 0x20000, 0x8d184b99 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - Graphics */
	ROM_LOAD( "rai9.bin",     0x000000, 0x08000, 0x1922b25e ) /* chars */
	ROM_LOAD( "rai10.bin",    0x008000, 0x08000, 0x5f90786a )

	ROM_LOAD( "raiu0919.bin", 0x020000, 0x80000, 0xda151f0b ) /* tiles */
	ROM_LOAD( "raiu0920.bin", 0x0a0000, 0x80000, 0xac1f57ac ) /* tiles */
	ROM_LOAD( "raiu165.bin",  0x120000, 0x80000, 0x946d7bde ) /* sprites */

	ROM_REGION(0x10000)	 /* Region 2 - 64k code for sound Z80 */
	ROM_LOAD( "rai6.bin", 0x000000, 0x10000, 0x723a483b )

	ROM_REGION(0x100000) /* Region 3 - v30 sub cpu */
	ROM_LOAD_GFX_EVEN( "rai5.bin",   0x0c0000, 0x20000, 0x7aca6d61 )
	ROM_LOAD_GFX_ODD ( "rai6a.bin",  0x0c0000, 0x20000, 0xe3d35cc2 )

	ROM_REGION(0x10000)	 /* Region 4 - ADPCM samples */
	ROM_LOAD( "rai7.bin", 0x000000, 0x10000, 0x8f927822 )
ROM_END

ROM_START(raidena_rom)
	ROM_REGION(0x100000) /* Region 0 - v20 main cpu */
	ROM_LOAD_GFX_EVEN( "rai1.bin",     0x0a0000, 0x10000, 0xa4b12785 )
	ROM_LOAD_GFX_ODD ( "rai2.bin",     0x0a0000, 0x10000, 0x17640bd5 )
//	ROM_LOAD_GFX_EVEN( "raiden03.rom", 0x0c0000, 0x20000, 0xffffffff )
//	ROM_LOAD_GFX_ODD ( "raiden04.rom", 0x0c0000, 0x20000, 0xffffffff )

	ROM_LOAD("dec.out",               0x0c0000, 0x40000, 0xffffffff )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - Graphics */
	ROM_LOAD( "rai9.bin",     0x000000, 0x08000, 0x1922b25e ) /* chars */
	ROM_LOAD( "rai10.bin",    0x008000, 0x08000, 0x5f90786a )

	ROM_LOAD( "raiu0919.bin", 0x020000, 0x80000, 0xda151f0b )
	ROM_LOAD( "raiu0920.bin", 0x0a0000, 0x80000, 0xac1f57ac )
	ROM_LOAD( "raiu165.bin",  0x120000, 0x80000, 0x946d7bde )

	ROM_REGION(0x10000)	 /* Region 2 - 64k code for sound Z80 */
	ROM_LOAD( "raiden08.rom", 0x000000, 0x10000, 0xffffffff )

	ROM_REGION(0x100000) /* Region 3 - v20 sub cpu */
//	ROM_LOAD_GFX_EVEN( "raiden05.rom",   0x0c0000, 0x20000, 0xffffffff )
//	ROM_LOAD_GFX_ODD ( "raiden06.rom",   0x0c0000, 0x20000, 0xffffffff )
	ROM_LOAD_GFX_EVEN( "rai5.bin",   0x0c0000, 0x20000, 0x7aca6d61 )
	ROM_LOAD_GFX_ODD ( "rai6a.bin",  0x0c0000, 0x20000, 0xe3d35cc2 )

	ROM_REGION(0x10000)	 /* Region 4 - ADPCM samples */
	ROM_LOAD( "rai7.bin", 0x000000, 0x10000, 0x8f927822 )
ROM_END

/***************************************************************************/

static void raidena_decrypt(void)
{
	/* Not finished yet! Gimme time */
}

/* Spin the 2nd cpu if it is waiting on the master cpu */
static int sub_cpu_spin(int offset)
{
	int pc=cpu_get_pc();
	int ret=raiden_shared_ram[0x8];

	if (offset==1) return raiden_shared_ram[0x9];

	if (pc==0xfcde6 && ret!=0x40)
		cpu_spin();

	return ret;
}

static void memory_patch(void)
{
	install_mem_read_handler(1, 0x4008, 0x4009, sub_cpu_spin);
}

/***************************************************************************/

struct GameDriver raiden_driver =
{
	__FILE__,
	0,
	"raiden",
	"Raiden",
	"1990",
	"Seibu Kaihatsu",
	"Oliver Bergmann\nBryan McPhail\nRandy Mongenel",
	0,
	&raiden_machine_driver,
	memory_patch,

	raiden_rom,
	0, 0,
	0, 0,
	raiden_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/* Don't add this yet */
struct GameDriver raidena_driver =
{
	__FILE__,
	&raiden_driver,
	"raidena",
	"Raiden (Alternate)",
	"1990",
	"Seibu Kaihatsu",
	"Oliver Bergmann\nBryan McPhail\nRandy Mongenel",
	0,
	&raidena_machine_driver,
	0,

	raidena_rom,
	raidena_decrypt, 0,
	0, 0,
	raiden_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
