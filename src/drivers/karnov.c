/*******************************************************************************

	Karnov - Bryan McPhail, mish@tendril.force9.net

  This file contains drivers for:

  	* Karnov (Data East USA, 1987)
    * Karnov (Japan rom set - Data East Corp, 1987)
		* Chelnov (Data East USA, 1988) (partially working)
    * Chelnov (Japan rom set - Data East Corp, 1987) (partially working)

NOTE!  Karnov USA & Karnov Japan sets have different gameplay!
  and Chelnov USA & Chelnov Japan sets have different gameplay!

These games use a 68000 main processor with a 6502, YM2203C and YM3526 for
sound.  Karnov was a major pain to get going because of the
'protection' on the main player sprite, probably connected to the Intel
microcontroller on the board.  The game is very sensitive to the wrong values
at the input ports...  There is also some sort of timer connected to the input
bits and this is set 'by hand' - I don't have any schematics or know what
this chip is.

There is another Karnov rom set - a bootleg version of the Japanese roms with
the Data East copyright removed - not supported because the original Japanese
roms work fine and two of the bootleg program roms appear to be short reads.

Chelnov is still partially cracked - seems to work until first live is lost.

Thanks to Oliver Stabel <stabel@rhein-neckar.netsurf.de> for confirming some
of the sprite & control information :)

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"


extern unsigned char *karnov_foreground,*karnov_sprites;
extern int karnov_scroll[4];

extern void karnov_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void karnov_foreground_w(int offset, int data);

extern void karnov_palette(void);

int karnov_vh_start (void);
void karnov_vh_stop (void);
void karnov_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

static int prot=0; /* For 'protection' on main player sprite */

/******************************************************************************/

void karnov_c_write(int offset, int data)
{
  static int f_pal=0;

	switch (offset) {
    case 0: /* Dunno.. but a good place to map palette for now */
			if (!f_pal) {karnov_palette(); f_pal=1;}
			return;

		case 2: /* Sound CPU in byte 3 */
			soundlatch_w(0,data&0xff);
      cpu_cause_interrupt (1, INT_NMI);
    	break;

    case 4: /* ?? */
    	break;

    case 6:  /* Controls main player animation */
   		prot=data&0xff;
    	break;

  	case 8:
			WRITE_WORD (&karnov_scroll[0], data);
			break;

    case 10:
			WRITE_WORD (&karnov_scroll[2], data);
			break;

		case 14: /* ?? */
			break;

		default:
		 if (errorlog) fprintf(errorlog,"CPU %04x - Write %02x at c0000 %d\n",cpu_getpc(),data,offset);
	}
}

/******************************************************************************/

int karnov_c_read(int offset)
{
	int msb,lsb;

	switch (offset) {
		case 0: /* Player controls */
			lsb=input_port_0_r (offset);
			return ( lsb + (lsb<<8));

		case 2: /* Start buttons & VBL */
			return input_port_1_r (offset);

		/* Dipswitch A & B */
		case 4:
			lsb=input_port_3_r (offset);
			msb=input_port_4_r (offset);
			return ( lsb + (msb<<8));

		case 6: /* Byte 7 - coins, byte 6 controls main player sprite animation */
			return (input_port_2_r (offset)<<8)+(prot*0x12);
	}
	if (errorlog) fprintf(errorlog,"Read at c0000 %d\n",offset);

	return 0xffff;
}

int chelnov_c_read(int offset)
{
	int msb,lsb;
	static int vbl_toggle=0,shot_timer=0;

	switch (offset) {
		case 0: /* Player controls */
			lsb=input_port_0_r (offset);
			return ( lsb + (lsb<<8));

		case 2: /* Start buttons & VBL */
		 //	return input_port_1_r (offset);

			lsb=input_port_1_r (offset);

			shot_timer++;
			if (shot_timer>5) { /* Value set by hand... */
				shot_timer=0;

				if (!vbl_toggle) {vbl_toggle=1; lsb=lsb&0xff;}
				else {vbl_toggle=0; lsb=lsb&0x7f;}
			}

			return lsb;

    /* Dipswitch A & B */
    case 4:
    	lsb=input_port_3_r (offset);
      msb=input_port_4_r (offset);
			return ( lsb + (msb<<8));

		case 6: /* Byte 7 - coins, byte 6 is protection */
			return (input_port_2_r (offset)<<8)+(prot);

	/*
		bpx f326 - uses a0 as base offset to sprites
		bpx f2f8 -
				f2de - IMPORTANT - sprite tables mapped via protection here


	*/


	}
	if (errorlog) fprintf(errorlog,"Read at c0000 %d\n",offset);

	return 0xffff;
}

/******************************************************************************/

static struct MemoryReadAddress karnov_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
  { 0x060000, 0x063fff, MRA_BANK1 },
	{ 0x080000, 0x080fff, MRA_BANK2 },
  { 0x0a0000, 0x0a07ff, MRA_BANK3 },
	{ 0x0c0000, 0x0c0007, karnov_c_read },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress chelnov_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x063fff, MRA_BANK1 },
	{ 0x080000, 0x080fff, MRA_BANK2 },
	{ 0x0a0000, 0x0a07ff, MRA_BANK3 },
	{ 0x0c0000, 0x0c0007, chelnov_c_read },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress karnov_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x063fff, MWA_BANK1 },
	{ 0x080000, 0x080fff, MWA_BANK2 , &karnov_sprites },
	{ 0x0a0000, 0x0a07ff, MWA_BANK3 , &videoram, &videoram_size },
	{ 0x0a1000, 0x0a1fff, karnov_foreground_w },
	{ 0x0c0000, 0x0c000f, karnov_c_write },
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress karnov_s_readmem[] =
{
	{ 0x0000, 0x05ff, MRA_RAM},
	{ 0x0800, 0x0800, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress karnov_s_writemem[] =
{
 	{ 0x0000, 0x05ff, MWA_RAM},
	{ 0x1000, 0x1000, YM2203_control_port_0_w },
  { 0x1001, 0x1001, YM2203_write_port_0_w },
  { 0x1800, 0x1800, YM3526_control_port_0_w },
	{ 0x1801, 0x1801, YM3526_write_port_0_w },
 	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( karnov_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Keep low for karnov.. */

	PORT_START	/* start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* Service button?  Other bits are return from microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )  /* Unchecked.. */

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Number of K for bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x04, "90" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x30, "Easy" )
  PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Atract Mode Sound", IP_KEY_NONE )
  PORT_DIPSETTING(    0x00, "On" )
  PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Normal" )
  PORT_DIPSETTING(    0x00, "Fast" )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Cabinet */
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Screen flip */
	PORT_DIPNAME( 0x80, 0x80, "No Die Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( chelnov_input_ports )
	PORT_START	/* Player controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Keep low for karnov.. */

	PORT_START	/* start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
//	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Credits, protection */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "Infinite" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x0c, 0x0c, "Number of K for bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Continues", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_BIT( 0xE0, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Top 3 bits unused */

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
  PORT_DIPNAME( 0x10, 0x10, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
 	PORT_DIPNAME( 0x20, 0x20, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout karnov_characters =
{
	8,8,
	1024,
	4,
	{ 0x0000*8,0x6000*8,0x4000*8,0x2000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

/* 16x16 tiles, 4 Planes, each plane is 0x10000 bytes */
static struct GfxLayout tiles2 =
{
	16,16,
	2048,
	4,
 	{ 0x30000*8,0x00000*8,0x10000*8,0x20000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
  	0,1,2,3,4,5,6,7 },
  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxLayout tiles3 =
{
	16,16,
	1024,
	4,
	{ 0x18000*8,0x0000*8,0x08000*8,0x10000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxDecodeInfo karnov_gfxdecodeinfo[] =
{
  { 1, 0x00000, &karnov_characters,0,0x80 },
  { 1, 0x08000, &tiles2,  0, 0x80 },  /* Backgrounds */
	{ 1, 0x48000, &tiles2,  0, 0x80 },	/* Sprites */
	{ 1, 0x88000, &tiles3,  0, 0x80 },	/* Sprites continued */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo chelnov_gfxdecodeinfo[] =
{
  { 1, 0x00000, &karnov_characters,0,0x80 },
	{ 1, 0x08000, &tiles2,  0, 0x80 },  /* Backgrounds */
	{ 1, 0x48000, &tiles2,  0, 0x80 },	/* Sprites */
	{ 1, 0x48000, &tiles3,  0, 0x80 },	/* Dummy area to be same as Karnov.. */
	{ -1 } /* end of array */
};

/******************************************************************************/

int karnov_interrupt(void)
{
	static int a=0;

	if (a) {a=0; return 6;}
	a=1; return 7;
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Unknown */
	{ YM2203_VOL(100,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (not supported) */
	{ 255 }		/* (not supported) */
};

/******************************************************************************/

static struct MachineDriver karnov_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz ?? */
			0,
			karnov_readmem,karnov_writemem,0,0,
			karnov_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			2,
			karnov_s_readmem,karnov_s_writemem,0,0,
			interrupt,12
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },

	karnov_gfxdecodeinfo,
	256,
	64*16,
	karnov_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	karnov_vh_start,
	karnov_vh_stop,
	karnov_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
    	SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

static struct MachineDriver chelnov_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz ?? */
			0,
			chelnov_readmem,karnov_writemem,0,0,
			karnov_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			2,
			karnov_s_readmem,karnov_s_writemem,0,0,
			interrupt,12
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },

	chelnov_gfxdecodeinfo,
	256,
	64*16,
	karnov_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	karnov_vh_start,
	karnov_vh_stop,
	karnov_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
    	SOUND_YM2203,
			&ym2203_interface
		},
    {
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

/******************************************************************************/

ROM_START( karnov_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "dn08-5", 0x00000, 0x10000, 0x87776617 )
	ROM_LOAD_ODD ( "dn11-5", 0x00000, 0x10000, 0xd6ecaea4 )
	ROM_LOAD_EVEN( "dn07-",  0x20000, 0x10000, 0x730e1ed8 )
	ROM_LOAD_ODD ( "dn10-",  0x20000, 0x10000, 0x9813e18f )
  ROM_LOAD_EVEN( "dn06-5", 0x40000, 0x10000, 0xc54bcfa1 )
  ROM_LOAD_ODD ( "dn09-5", 0x40000, 0x10000, 0xb678cfd8 )

  ROM_REGION(0xa8000)
  /* Characters */
  ROM_LOAD( "dn00-", 0x00000, 0x08000, 0x8cf6e300 )
  /* Backgrounds */
  ROM_LOAD( "dn04-", 0x08000, 0x10000, 0x85d7a661 )
	ROM_LOAD( "dn01-", 0x18000, 0x10000, 0xbe2ab384 )
  ROM_LOAD( "dn03-", 0x28000, 0x10000, 0xb2032daf )
  ROM_LOAD( "dn02-", 0x38000, 0x10000, 0xe60970ed )
  /* Sprites */
  ROM_LOAD( "dn12-", 0x48000, 0x10000, 0x0300f4c8 )
  ROM_LOAD( "dn13-", 0x58000, 0x10000, 0x7d211a85 )
	ROM_LOAD( "dn16-", 0x68000, 0x10000, 0xc945ee31 )
  ROM_LOAD( "dn18-", 0x78000, 0x10000, 0xbb24da3a )
  /* Sprites */
	ROM_LOAD( "dn14-5", 0x88000, 0x8000, 0xb6b9f841 )
	ROM_LOAD( "dn15-5", 0x90000, 0x8000, 0xcf18d74a )
  ROM_LOAD( "dn17-5", 0x98000, 0x8000, 0x06e9df53 )
  ROM_LOAD( "dn19-5", 0xa0000, 0x8000, 0x83fcbe26 )
  /* 6502 Sound CPU */
  ROM_REGION(0x10000)
  ROM_LOAD( "dn05-5", 0x8000, 0x8000, 0x4fc9a353 )
  /* Colour proms */
  ROM_REGION(2048)
  ROM_LOAD( "karnprom.21", 0x000, 0x400, 0x8485ab35)
  ROM_LOAD( "karnprom.20", 0x400, 0x400, 0x13c00e08)
ROM_END

ROM_START( karnovj_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
  ROM_LOAD_EVEN( "kar8",  0x00000, 0x10000, 0xc945f329 )
	ROM_LOAD_ODD ( "kar11", 0x00000, 0x10000, 0x347e4e36 )
	ROM_LOAD_EVEN( "kar7",  0x20000, 0x10000, 0x730e1ed8 )
	ROM_LOAD_ODD ( "kar10", 0x20000, 0x10000, 0x9813e18f )
  ROM_LOAD_EVEN( "kar6",  0x40000, 0x10000, 0x062d7e77 )
	ROM_LOAD_ODD ( "kar9",  0x40000, 0x10000, 0x37c0a23a )

  ROM_REGION(0xa8000)
  /* Characters */
  ROM_LOAD( "kar0", 0x00000, 0x08000, 0x8cf6e300 )
  /* Backgrounds */
  ROM_LOAD( "kar4", 0x08000, 0x10000, 0x85d7a661 )
  ROM_LOAD( "kar1", 0x18000, 0x10000, 0xbe2ab384 )
  ROM_LOAD( "kar3", 0x28000, 0x10000, 0xb2032daf )
  ROM_LOAD( "kar2", 0x38000, 0x10000, 0xe60970ed )
  /* Sprites */
	ROM_LOAD( "kar12", 0x48000, 0x10000, 0x0300f4c8 )
  ROM_LOAD( "kar13", 0x58000, 0x10000, 0x7d211a85 )
  ROM_LOAD( "kar16", 0x68000, 0x10000, 0xc945ee31 )
  ROM_LOAD( "kar18", 0x78000, 0x10000, 0xbb24da3a )
  /* Sprites */
  ROM_LOAD( "kar14", 0x88000, 0x8000, 0x3d95baef )
	ROM_LOAD( "kar15", 0x90000, 0x8000, 0xbb2d9d09 )
  ROM_LOAD( "kar17", 0x98000, 0x8000, 0xc1a153c7 )
  ROM_LOAD( "kar19", 0xa0000, 0x8000, 0x77773ad5 )
	/* 6502 Sound CPU */
	ROM_REGION(0x10000)
  ROM_LOAD( "kar5", 0x8000, 0x8000, 0x50cf61cf)
  /* Colour proms */
  ROM_REGION(2048)
  ROM_LOAD( "karnprom.21", 0x000, 0x400, 0x8485ab35)
  ROM_LOAD( "karnprom.20", 0x400, 0x400, 0x13c00e08)
ROM_END

ROM_START( chelnov_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "ee08-a.j15", 0x00000, 0x10000, 0x6727fe5d )
	ROM_LOAD_ODD ( "ee11-a.j20", 0x00000, 0x10000, 0x9ea7dc0d )
	ROM_LOAD_EVEN( "ee07-a.j14", 0x20000, 0x10000, 0x01080e4a )
	ROM_LOAD_ODD ( "ee10-a.j18", 0x20000, 0x10000, 0xe68e006a )
	ROM_LOAD_EVEN( "ee06-e.j13", 0x40000, 0x10000, 0x463270f6 )
	ROM_LOAD_ODD ( "ee09-e.j17", 0x40000, 0x10000, 0x18020e86 )

  ROM_REGION(0x88000)
  /* Characters */
	ROM_LOAD( "ee00-e.c5", 0x00000, 0x08000, 0x25ad554f )
	/* Backgrounds */
	ROM_LOAD( "ee04-.d18", 0x08000, 0x10000, 0xd447b383 )
	ROM_LOAD( "ee01-.c15", 0x18000, 0x10000, 0x476a4d90 )
	ROM_LOAD( "ee03-.d15", 0x28000, 0x10000, 0xe23fd4ff )
	ROM_LOAD( "ee02-.c18", 0x38000, 0x10000, 0x7897fcb7 )
	/* Sprites */
	ROM_LOAD( "ee12-.f8",  0x48000, 0x10000, 0x62adc797 )
	ROM_LOAD( "ee13-.f9",  0x58000, 0x10000, 0x4524d74c )
	ROM_LOAD( "ee14-.f13", 0x68000, 0x10000, 0x47efa38b )
	ROM_LOAD( "ee15-.f15", 0x78000, 0x10000, 0x0aaca864 )

	/* 6502 Sound CPU */
	ROM_REGION(0x10000)
	ROM_LOAD( "ee05-.f3", 0x8000, 0x8000, 0xc9f33353 )

	ROM_REGION(2048)
	ROM_LOAD( "ee21.k8", 0x000, 0x400, 0xe6be044a )
	ROM_LOAD( "ee20.l6", 0x400, 0x400, 0xfeba090e )
ROM_END

ROM_START( chelnovj_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "a-j15.bin", 0x00000, 0x10000, 0x4af03e04 )
	ROM_LOAD_ODD ( "a-j20.bin", 0x00000, 0x10000, 0x7ff4255c )
  ROM_LOAD_EVEN( "a-j14.bin", 0x20000, 0x10000, 0x639164ef )
	ROM_LOAD_ODD ( "a-j18.bin", 0x20000, 0x10000, 0x53df7af1 )
  ROM_LOAD_EVEN( "a-j13.bin", 0x40000, 0x10000, 0xb60270c6 )
	ROM_LOAD_ODD ( "a-j17.bin", 0x40000, 0x10000, 0x1a1f248f )

  ROM_REGION(0x88000)
	/* Characters */
  ROM_LOAD( "a-c5.bin", 0x00000, 0x08000, 0x25c8aab0 )
  /* Backgrounds */
  ROM_LOAD( "a-d18.bin", 0x08000, 0x10000, 0xd447b383 )
  ROM_LOAD( "a-c15.bin", 0x18000, 0x10000, 0x476a4d90 )
  ROM_LOAD( "a-d15.bin", 0x28000, 0x10000, 0xe23fd4ff )
	ROM_LOAD( "a-c18.bin", 0x38000, 0x10000, 0x7897fcb7 )
  /* Sprites */
  ROM_LOAD( "b-f8.BIN",  0x48000, 0x10000, 0x62adc797 )
	ROM_LOAD( "b-f9.BIN",  0x58000, 0x10000, 0x4524d74c )
	ROM_LOAD( "b-f13.BIN", 0x68000, 0x10000, 0x47efa38b )
	ROM_LOAD( "b-f16.BIN", 0x78000, 0x10000, 0x0aaca864 )

  /* 6502 Sound CPU */
  ROM_REGION(0x10000)
  ROM_LOAD( "a-f3.BIN", 0x8000, 0x8000, 0xc9f33353)

  ROM_REGION(2048)
  ROM_LOAD( "a-k7.bin", 0x000, 0x400, 0xd97d37c5)
  ROM_LOAD( "a-l6.bin", 0x400, 0x400, 0xfeba090e)
ROM_END

/******************************************************************************/

static void karnov_patch(void)
{
	WRITE_WORD (&RAM[0x05E4],0x600A);
}

static void karnovj_patch(void)
{
	WRITE_WORD (&RAM[0x05E4],0x600A);
}

static void chelnov_patch(void)
{
//  WRITE_WORD (&RAM[0x0E54],0x4E71); /* Speed up */
	//  WRITE_WORD (&RAM[0x0E46],0x4E71);
//  WRITE_WORD (&RAM[0x0E6C],0x4E71);   /* tst*/
	//  WRITE_WORD (&RAM[0x0E14],0x4E71); /* Speed up in game.. */


	WRITE_WORD (&RAM[0x0A26],0x4E71);  /* removes a protection lookup table */

  /* Pulls out corrupt writes to protection location */
	WRITE_WORD (&RAM[0x0A7A],0x4E71);
	WRITE_WORD (&RAM[0x0A7C],0x4E71);
	WRITE_WORD (&RAM[0x0A7E],0x4E71);

	/* Hmmm, strange value gets written to protection here.. */
	WRITE_WORD (&RAM[0x0C52],0x4E71);
	WRITE_WORD (&RAM[0x0C54],0x4E71);
	WRITE_WORD (&RAM[0x0C56],0x4E71);
	WRITE_WORD (&RAM[0x0C58],0x4E71);
}

static void chelnovj_patch(void)
{
	WRITE_WORD (&RAM[0x0E54],0x4E71); /* Speed up */
	//  WRITE_WORD (&RAM[0x0E46],0x4E71);
	WRITE_WORD (&RAM[0x0E6C],0x4E71);   /* tst*/
	//  WRITE_WORD (&RAM[0x0E14],0x4E71); /* Speed up in game.. */


	WRITE_WORD (&RAM[0x0A2E],0x4E71);  /* removes a protection lookup table */
	WRITE_WORD (&RAM[0x0AAC],0x4E75);
}

struct GameDriver karnov_driver =
{
	"Karnov",
	"karnov",
	"Bryan McPhail",
	&karnov_machine_driver,

	karnov_rom,
	karnov_patch,
	0,
	0,
	0,	/* sound_prom */

	karnov_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver karnovj_driver =
{
	"Karnov (Japan)",
	"karnovj",
	"Bryan McPhail",
	&karnov_machine_driver,

	karnovj_rom,
	karnovj_patch,
	0,
	0,
	0,	/* sound_prom */

	karnov_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver chelnov_driver =
{
	"Chelnov - Atomic Runner",
	"chelnov",
	"Bryan McPhail",
	&chelnov_machine_driver,

	chelnov_rom,
	chelnov_patch,
	0,
	0,
	0,	/* sound_prom */

	chelnov_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver chelnovj_driver =
{
	"Chelnov - Atomic Runner (Japan)",
	"chelnovj",
	"Bryan McPhail",
	&chelnov_machine_driver,

	chelnovj_rom,
	chelnovj_patch,
	0,
	0,
	0,	/* sound_prom */

	chelnov_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

