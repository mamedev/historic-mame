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
at the input ports...

There is another Karnov rom set - a bootleg version of the Japanese roms with
the Data East copyright removed - not supported because the original Japanese
roms work fine and two of the bootleg program roms appear to be short reads.

Chelnov is still partially cracked - seems to work until first live is lost.

One of the two color PROMs for chelnov and chelnoj is different; one is most
likely a bad read, but I don't know which one.

Thanks to Oliver Stabel <stabel@rhein-neckar.netsurf.de> for confirming some
of the sprite & control information :)

Changes, May 1998 :
* Fixed front plane graphics in both games.
* Improved 'slowdowns' by adjusting vbl time. (still not perfect)
* Chelnov attract mode now works properly.
* Palette bug that affected Windows port fixed.
* Karnov sprite banks merged.
July:
* Fixed visible area
* Fixed Chelnov interrupts
* Adjusted vbl time (again)
* There was a serious glitch, big slowdowns occured with more than about 7 sprites
on screen, increasing cpu speed (from 10 to 14) seems to have helped.

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"

extern unsigned char *karnov_foreground,*karnov_sprites;
extern int karnov_scroll[4];

void karnov_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void karnov_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void karnov_foreground_w(int offset, int data);

int karnov_vh_start (void);
void karnov_vh_stop (void);

static int prot=0; /* For 'protection' on main player sprite */
int karnov_a,karnov_b,karnov_c;  /* For testing of unknown ports */

/******************************************************************************/

void karnov_c_write(int offset, int data)
{
	switch (offset) {
		case 0: /* Dunno..  */
			return;

		case 2: /* Sound CPU in byte 3 */
			soundlatch_w(0,data&0xff);
			cpu_cause_interrupt (1, INT_NMI);
			break;

		case 4: /* ?? */
			karnov_a=data;
			break;

		case 6:  /* Controls main player animation in Karnov */
			if (errorlog) fprintf(errorlog,"CPU %04x - Write %02x at c0000 %d\n",cpu_getpc(),data,offset);
			karnov_b=data;  /****/
			prot=data;
			break;

		case 8:
			WRITE_WORD (&karnov_scroll[0], data);
			break;

		case 10:
			WRITE_WORD (&karnov_scroll[2], data);
			break;

		case 14: /* ?? */
			karnov_c=data;
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

		case 4: /* Dipswitch A & B */
			lsb=input_port_3_r (offset);
			msb=input_port_4_r (offset);
			return ( lsb + (msb<<8));

		case 6: /* Byte 7 - coins, byte 6 controls main player sprite animation */
			return (input_port_2_r (offset)<<8)+((prot&0xff)*0x12);
	}
	if (errorlog) fprintf(errorlog,"Read at c0000 %d\n",offset);

	return 0xffff;
}

int chelnov_c_read(int offset)
{
	int msb,lsb;

	switch (offset) {
		case 0: /* Player controls */
			lsb=input_port_0_r (offset);
			return ( lsb + (lsb<<8));

		case 2: /* Start buttons & VBL */
		 	return input_port_1_r (offset);

		case 4: /* Dipswitch A & B */
			lsb=input_port_3_r (offset);
			msb=input_port_4_r (offset);
			return ( lsb + (msb<<8));

		case 6: /* Byte 7 - coins, byte 6 is protection */
      		if (errorlog) fprintf(errorlog,"Read at c0000 %d %d\n",offset,prot);

			/* Some known return values for 8571 commands */
 //     if (prot==0x100) return 0x71b;
 //     if (prot==0x200) return 0x783e;

			return (input_port_2_r (offset)<<8)+((prot&0xff));

	/*
    m/c -> d3 -> 0x60024 -> 0x6018c


    CHEAT - byte at 0x60189 is level - enter value 0 - 6 at cartoon intro..


    *** M/C check at 992
    *** Check at 9e0

    Another check around a10


		bpx f326 - uses a0 as base offset to sprites
		bpx f2f8 -
				f2de - IMPORTANT - sprite tables mapped via protection here

    bpx f2c8 to see results of m/c return


    f31e...  main check???




    115e & cdc8 & d536 & de84 & e9c0  clear 6018c - bpx this..
    59a6 alters 6018c
    f2de - main check

    something at fa6...


    0x62050 - set to FFFF when life is lost... so no baddies..

    0xf0ae  check...

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
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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

static struct GfxLayout chars =
{
	8,8,
	1024,
	3,
	{ 0x6000*8,0x4000*8,0x2000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

/* 16x16 sprites, 4 Planes, each plane is 0x10000 + 0x8000 bytes */
static struct GfxLayout sprites =
{
	16,16,
	2048+1024,
	4,
 	{ 0x48000*8,0x00000*8,0x18000*8,0x30000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
  	0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

/* 16x16 tiles, 4 Planes, each plane is 0x10000 bytes */
static struct GfxLayout tiles =
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

static struct GfxDecodeInfo karnov_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &chars,     0,  4 },	/* colors 0-31 */
	{ 1, 0x08000, &tiles,   512, 16 },	/* colors 512-767 */
	{ 1, 0x48000, &sprites, 256, 16 },	/* colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo chelnov_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &chars,     0,  4 },	/* colors 0-31 */
	{ 1, 0x08000, &tiles,   512, 16 },	/* colors 512-767 */
	{ 1, 0x48000, &tiles,   256, 16 },	/* colors 256-511 */
	{ -1 } /* end of array */
};

/******************************************************************************/

int karnov_interrupt(void)
{
	if (cpu_getiloops() == 0) return 6;
	else return 7;
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Unknown */
	{ YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

/******************************************************************************/

static struct MachineDriver karnov_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			14000000,	/* 14 Mhz ?? */
			0,
			karnov_readmem,karnov_writemem,0,0,
			karnov_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			3,
			karnov_s_readmem,karnov_s_writemem,0,0,
			interrupt,14	/* hand tuned */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	karnov_gfxdecodeinfo,
	1024, 1024,
	karnov_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
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
			14000000,	/* 14 Mhz ?? */
			0,
			chelnov_readmem,karnov_writemem,0,0,
			karnov_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			3,
			karnov_s_readmem,karnov_s_writemem,0,0,
			interrupt,12
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	chelnov_gfxdecodeinfo,
	1024, 1024,
	karnov_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
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
	ROM_LOAD( "dn00-", 0x00000, 0x08000, 0x8cf6e300 )	/* Characters */
	ROM_LOAD( "dn04-", 0x08000, 0x10000, 0x85d7a661 )	/* Backgrounds */
	ROM_LOAD( "dn01-", 0x18000, 0x10000, 0xbe2ab384 )
	ROM_LOAD( "dn03-", 0x28000, 0x10000, 0xb2032daf )
	ROM_LOAD( "dn02-", 0x38000, 0x10000, 0xe60970ed )
	ROM_LOAD( "dn12-",  0x48000, 0x10000, 0x0300f4c8 )	/* Sprites - 2 sets of 4, interleaved here */
	ROM_LOAD( "dn14-5", 0x58000, 0x08000, 0xb6b9f841 )
	ROM_LOAD( "dn13-",  0x60000, 0x10000, 0x7d211a85 )
	ROM_LOAD( "dn15-5", 0x70000, 0x08000, 0xcf18d74a )
	ROM_LOAD( "dn16-",  0x78000, 0x10000, 0xc945ee31 )
	ROM_LOAD( "dn17-5", 0x88000, 0x08000, 0x06e9df53 )
	ROM_LOAD( "dn18-",  0x90000, 0x10000, 0xbb24da3a )
	ROM_LOAD( "dn19-5", 0xa0000, 0x08000, 0x83fcbe26 )

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "karnprom.21", 0x0000, 0x0400, 0x8485ab35 )
	ROM_LOAD( "karnprom.20", 0x0400, 0x0400, 0x13c00e08 )

	/* 6502 Sound CPU */
	ROM_REGION(0x10000)
	ROM_LOAD( "dn05-5", 0x8000, 0x8000, 0x4fc9a353 )
ROM_END

ROM_START( karnovj_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "kar8",  0x00000, 0x10000, 0xc945f329 )
	ROM_LOAD_ODD ( "kar11", 0x00000, 0x10000, 0x347e4e36 )
	ROM_LOAD_EVEN( "dn07-", 0x20000, 0x10000, 0x730e1ed8 )
	ROM_LOAD_ODD ( "dn10-", 0x20000, 0x10000, 0x9813e18f )
	ROM_LOAD_EVEN( "kar6",  0x40000, 0x10000, 0x062d7e77 )
	ROM_LOAD_ODD ( "kar9",  0x40000, 0x10000, 0x37c0a23a )

	ROM_REGION(0xa8000)
	ROM_LOAD( "dn00-", 0x00000, 0x08000, 0x8cf6e300 )	/* Characters */
	ROM_LOAD( "dn04-", 0x08000, 0x10000, 0x85d7a661 )	/* Backgrounds */
	ROM_LOAD( "dn01-", 0x18000, 0x10000, 0xbe2ab384 )
	ROM_LOAD( "dn03-", 0x28000, 0x10000, 0xb2032daf )
	ROM_LOAD( "dn02-", 0x38000, 0x10000, 0xe60970ed )
	ROM_LOAD( "dn12-", 0x48000, 0x10000, 0x0300f4c8 )	/* Sprites - 2 sets of 4, interleaved here */
	ROM_LOAD( "kar14", 0x58000, 0x08000, 0x3d95baef )
	ROM_LOAD( "dn13-", 0x60000, 0x10000, 0x7d211a85 )
	ROM_LOAD( "kar15", 0x70000, 0x08000, 0xbb2d9d09 )
	ROM_LOAD( "dn16-", 0x78000, 0x10000, 0xc945ee31 )
	ROM_LOAD( "kar17", 0x88000, 0x08000, 0xc1a153c7 )
	ROM_LOAD( "dn18-", 0x90000, 0x10000, 0xbb24da3a )
	ROM_LOAD( "kar19", 0xa0000, 0x08000, 0x77773ad5 )

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "karnprom.21", 0x0000, 0x0400, 0x8485ab35 )
	ROM_LOAD( "karnprom.20", 0x0400, 0x0400, 0x13c00e08 )

	/* 6502 Sound CPU */
	ROM_REGION(0x10000)
	ROM_LOAD( "kar5", 0x8000, 0x8000, 0x50cf61cf )
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
	ROM_LOAD( "ee00-e.c5", 0x00000, 0x08000, 0x25ad554f )	/* Characters */
	ROM_LOAD( "ee04-.d18", 0x08000, 0x10000, 0xd447b383 )	/* Backgrounds */
	ROM_LOAD( "ee01-.c15", 0x18000, 0x10000, 0x476a4d90 )
	ROM_LOAD( "ee03-.d15", 0x28000, 0x10000, 0xe23fd4ff )
	ROM_LOAD( "ee02-.c18", 0x38000, 0x10000, 0x7897fcb7 )
	ROM_LOAD( "ee12-.f8",  0x48000, 0x10000, 0x62adc797 )	/* Sprites */
	ROM_LOAD( "ee13-.f9",  0x58000, 0x10000, 0x4524d74c )
	ROM_LOAD( "ee14-.f13", 0x68000, 0x10000, 0x47efa38b )
	ROM_LOAD( "ee15-.f15", 0x78000, 0x10000, 0x0aaca864 )

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "ee21.k8", 0x0000, 0x0400, 0xe6be044a )	/* different from the other set; */
														/* might be bad */
	ROM_LOAD( "ee20.l6", 0x0400, 0x0400, 0xfeba090e )

	ROM_REGION(0x10000)	/* 6502 Sound CPU */
	ROM_LOAD( "ee05-.f3", 0x8000, 0x8000, 0xc9f33353 )
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
	ROM_LOAD( "a-c5.bin",  0x00000, 0x08000, 0x25c8aab0 )	/* Characters */
	ROM_LOAD( "ee04-.d18", 0x08000, 0x10000, 0xd447b383 )	/* Backgrounds */
	ROM_LOAD( "ee01-.c15", 0x18000, 0x10000, 0x476a4d90 )
	ROM_LOAD( "ee03-.d15", 0x28000, 0x10000, 0xe23fd4ff )
	ROM_LOAD( "ee02-.c18", 0x38000, 0x10000, 0x7897fcb7 )
	ROM_LOAD( "ee12-.f8",  0x48000, 0x10000, 0x62adc797 )	/* Sprites */
	ROM_LOAD( "ee13-.f9",  0x58000, 0x10000, 0x4524d74c )
	ROM_LOAD( "ee14-.f13", 0x68000, 0x10000, 0x47efa38b )
	ROM_LOAD( "ee15-.f15", 0x78000, 0x10000, 0x0aaca864 )

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "a-k7.bin", 0x0000, 0x0400, 0xd97d37c5 )	/* different from the other set; */
														/* might be bad */
	ROM_LOAD( "a-l6.bin", 0x0400, 0x0400, 0xfeba090e )

	ROM_REGION(0x10000)	/* 6502 Sound CPU */
	ROM_LOAD( "ee05-.f3", 0x8000, 0x8000, 0xc9f33353 )
ROM_END


/******************************************************************************/

static void karnov_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	WRITE_WORD (&RAM[0x05E4],0x600A);
}

static void karnovj_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	WRITE_WORD (&RAM[0x05E4],0x600A);
}

static void chelnov_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

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

  /* These next two are protection checks, a value is written to the 8751 and
  	a specific value is needed back...  However, it doesn't seem to make any
    reads from the 8751 before a timeout... Perhaps interrupts are wrong?

    Let's just patch out the checks for now ;) */
	WRITE_WORD (&RAM[0x09EC],0x4E75);
	WRITE_WORD (&RAM[0x099E],0x4E75);

  /* The main protection check...  kinda.. */
//	WRITE_WORD (&RAM[0xf0ac],0x4E71);

}

static void chelnovj_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	WRITE_WORD (&RAM[0x0A2E],0x4E71);  /* removes a protection lookup table */

	/* Static checks, as for usa version */
	WRITE_WORD (&RAM[0x09FC],0x4E75);
	WRITE_WORD (&RAM[0x09A6],0x4E75);
}

struct GameDriver karnov_driver =
{
	__FILE__,
	0,
	"karnov",
	"Karnov (US)",
	"1987",
	"Data East USA",
	"Bryan McPhail",
	0,
	&karnov_machine_driver,

	karnov_rom,
	karnov_patch,
	0,
	0,
	0,	/* sound_prom */

	karnov_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver karnovj_driver =
{
	__FILE__,
	&karnov_driver,
	"karnovj",
	"Karnov (Japan)",
	"1987",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&karnov_machine_driver,

	karnovj_rom,
	karnovj_patch,
	0,
	0,
	0,	/* sound_prom */

	karnov_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver chelnov_driver =
{
	__FILE__,
	0,
	"chelnov",
	"Chelnov - Atomic Runner (US)",
	"1988",
	"Data East USA",
	"Bryan McPhail",
	GAME_NOT_WORKING,
	&chelnov_machine_driver,

	chelnov_rom,
	chelnov_patch,
	0,
	0,
	0,	/* sound_prom */

	chelnov_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver chelnovj_driver =
{
	__FILE__,
	&chelnov_driver,
	"chelnovj",
	"Chelnov - Atomic Runner (Japan)",
	"1988",
	"Data East Corporation",
	"Bryan McPhail",
	GAME_NOT_WORKING,
	&chelnov_machine_driver,

	chelnovj_rom,
	chelnovj_patch,
	0,
	0,
	0,	/* sound_prom */

	chelnov_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

