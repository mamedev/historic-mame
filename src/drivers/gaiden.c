/***************************************************************************

Ninja Gaiden memory map (preliminary)

000000-03ffff ROM
060000-063fff RAM
070000-070fff Video RAM (text layer)
072000-075fff VRAM (backgrounds)
076000-077fff Sprite RAM
078000-079fff Palette RAM

07a100-07a1ff Unknown

memory mapped ports:

read:
07a001    IN0
07a002    IN2
07a003    IN1
07a004    DWSB
07a005    DSWA
see the input_ports definition below for details on the input bits

write:
07a104-07a105 text layer Y scroll
07a10c-07a10d text layer X scroll
07a204-07a205 front layer Y scroll
07a20c-07a20d front layer X scroll
07a304-07a305 back layer Y scroll
07a30c-07a30d back layer Xscroll

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"
#include "Z80/Z80.h"

extern unsigned char *gaiden_videoram;
extern unsigned char *gaiden_spriteram;
extern unsigned char *gaiden_videoram2;
extern unsigned char *gaiden_videoram3;
extern unsigned char *gaiden_txscrollx,*gaiden_txscrolly;
extern unsigned char *gaiden_fgscrollx,*gaiden_fgscrolly;
extern unsigned char *gaiden_bgscrollx,*gaiden_bgscrolly;

extern int gaiden_videoram_size;
extern int gaiden_videoram2_size;
extern int gaiden_videoram3_size;
extern int gaiden_spriteram_size;

void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void gaiden_unknownram_w(int offset,int data);
int gaiden_unknownram_r(int offset);
void gaiden_videoram_w(int offset,int data);
int gaiden_videoram_r(int offset);
void gaiden_videoram2_w(int offset,int data);
int gaiden_videoram2_r(int offset);
void gaiden_videoram3_w(int offset,int data);
int gaiden_videoram3_r(int offset);
void gaiden_spriteram_w(int offset,int data);
int gaiden_spriteram_r(int offset);

void gaiden_txscrollx_w(int offset,int data);
void gaiden_txscrolly_w(int offset,int data);
void gaiden_fgscrollx_w(int offset,int data);
void gaiden_fgscrolly_w(int offset,int data);
void gaiden_bgscrollx_w(int offset,int data);
void gaiden_bgscrolly_w(int offset,int data);


void gaiden_background_w(int offset,int data);
void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int  gaiden_vh_start(void);
void gaiden_vh_stop(void);


int gaiden_interrupt(void)
{
	return 5;  /*Interrupt vector 5*/
}

int gaiden_input_r (int offset)
{
	int ans = 0xff;
	switch (offset)
	{
		case 0:
			ans = input_port_4_r (offset);
			break;
		case 2:
			ans = (input_port_1_r (offset) << 8) + (input_port_0_r (offset));
			break;
		case 4:
			ans = (input_port_3_r (offset) << 8) + (input_port_2_r (offset));
			break;
	}
	return ans;
}

void gaiden_sound_command_w(int offset,int data)
{
	if (offset == 2)
	{
		soundlatch_w(0,data & 0xff);
		cpu_cause_interrupt(1,Z80_NMI_INT);
	}
//	else
//if (errorlog) fprintf(errorlog,"PC %06x write %02x to 0x07a800\n",cpu_getpc(),data);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x060000, 0x063fff, MRA_BANK1 },   /* RAM */
	{ 0x070000, 0x070fff, gaiden_videoram_r },
	{ 0x072000, 0x073fff, gaiden_videoram2_r },
	{ 0x074000, 0x075fff, gaiden_videoram3_r },
	{ 0x076000, 0x077fff, gaiden_spriteram_r },
	{ 0x078000, 0x0787ff, paletteram_word_r },
	{ 0x078800, 0x079fff, MRA_NOP },   /* extra portion of palette RAM, not really used */
	{ 0x07a000, 0x07a007, gaiden_input_r },
	{ 0x07a100, 0x07a1ff, MRA_BANK2 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x060000, 0x063fff, MWA_BANK1 },
	{ 0x070000, 0x070fff, gaiden_videoram_w, &gaiden_videoram, &gaiden_videoram_size },
//	{ 0x071000, 0x071fff, MWA_NOP },   /* I'm not sure */
	{ 0x072000, 0x073fff, gaiden_videoram2_w,  &gaiden_videoram2, &gaiden_videoram2_size },
	{ 0x074000, 0x075fff, gaiden_videoram3_w,  &gaiden_videoram3, &gaiden_videoram3_size },
	{ 0x076000, 0x077fff, gaiden_spriteram_w, &gaiden_spriteram, &gaiden_spriteram_size },
	{ 0x078000, 0x0787ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x078800, 0x079fff, MWA_NOP },   /* extra portion of palette RAM, not really used */
//	{ 0x07a000, 0x07a00f, MWA_NOP },   /* I'm not sure */
//	{ 0x07a100, 0x07a111, MWA_BANK2 },  /* video? */
	{ 0x07a104, 0x07a105, gaiden_txscrolly_w, &gaiden_txscrolly },
	{ 0x07a10c, 0x07a10d, gaiden_txscrollx_w, &gaiden_txscrollx },
	{ 0x07a204, 0x07a205, gaiden_fgscrolly_w, &gaiden_fgscrolly },
	{ 0x07a20c, 0x07a20d, gaiden_fgscrollx_w, &gaiden_fgscrollx },
	{ 0x07a304, 0x07a305, gaiden_bgscrolly_w, &gaiden_bgscrolly },
	{ 0x07a30c, 0x07a30d, gaiden_bgscrollx_w, &gaiden_bgscrollx },
//	{ 0x07a400, 0x07a809, MWA_NOP },   /* I'm not sure */
	{ 0x07a800, 0x07a803, gaiden_sound_command_w },
	{ 0x07a804, 0x07a807, MWA_NOP },	/* ??? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, OKIM6295_status_r },
	{ 0xfc00, 0xfc00, MRA_NOP },	/* ?? */
	{ 0xfc20, 0xfc20, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, OKIM6295_data_w },
	{ 0xf810, 0xf810, YM2203_control_port_0_w },
	{ 0xf811, 0xf811, YM2203_write_port_0_w },
	{ 0xf820, 0xf820, YM2203_control_port_1_w },
	{ 0xf821, 0xf821, YM2203_write_port_1_w },
	{ 0xfc00, 0xfc00, MWA_NOP },	/* ?? */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( gaiden_input_ports )
        PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSWA */
	PORT_DIPNAME( 0xe0, 0xe0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x60, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x1c, 0x1c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x1c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x14, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x01, 0x01, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )

	PORT_START	/* DSWB */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 tiles */
	2048,	/* 2048 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
        { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
        { 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every tile takes 32 consecutive bytes */


};

static struct GfxLayout tile2layout =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 2048 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	  32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4,
	  32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4,},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32},
	128*8	/* every sprite takes 512 bytes */

};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 0*4+131072*8, 1*4+131072*8, 2*4, 3*4, 2*4+131072*8, 3*4+131072*8,
	  32*4+0*4, 32*4+1*4, 32*4+0*4+131072*8, 32*4+1*4+131072*8, 32*4+2*4, 32*4+3*4, 32*4+2*4+131072*8, 32*4+3*4+131072*8,
	  128*4+0*4, 128*4+1*4, 128*4+0*4+131072*8, 128*4+1*4+131072*8, 128*4+2*4, 128*4+3*4, 128*4+2*4+131072*8, 128*4+3*4+131072*8,
	  160*4+0*4, 160*4+1*4, 160*4+0*4+131072*8, 160*4+1*4+131072*8, 160*4+2*4, 160*4+3*4, 160*4+2*4+131072*8, 160*4+3*4+131072*8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
	  64*16, 65*16, 66*16, 67*16, 68*16, 69*16, 70*16, 71*16,
	  80*16, 81*16, 82*16, 83*16, 84*16, 85*16, 86*16, 87*16 },
	256*8	/* 256 bytes to next sprite */
};

static struct GfxLayout spritelayout16x16 =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 0*4+131072*8, 1*4+131072*8, 2*4, 3*4, 2*4+131072*8, 3*4+131072*8,
	  32*4+0*4, 32*4+1*4, 32*4+0*4+131072*8, 32*4+1*4+131072*8, 32*4+2*4, 32*4+3*4, 32*4+2*4+131072*8, 32*4+3*4+131072*8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* 64 bytes to the next sprite */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &tilelayout,        256, 16 },	/* tiles 8x8*/
	{ 1, 0x110000, &tile2layout,       768, 16 },	/* tiles 16x16*/
	{ 1, 0x190000, &tile2layout,       512, 16 },	/* tiles 16x16*/
	{ 1, 0x010000, &spritelayout,        0, 16 },	/* sprites 32x32*/
	{ 1, 0x050000, &spritelayout,        0, 16 },	/* sprites 32x32*/
	{ 1, 0x090000, &spritelayout,        0, 16 },	/* sprites 32x32*/
	{ 1, 0x0d0000, &spritelayout,        0, 16 },	/* sprites 32x32*/
	{ 1, 0x010000, &spritelayout16x16,   0, 16 },	/* sprites 16x16*/
	{ 1, 0x050000, &spritelayout16x16,   0, 16 },	/* sprites 16x16*/
	{ 1, 0x090000, &spritelayout16x16,   0, 16 },	/* sprites 16x16*/
	{ 1, 0x0d0000, &spritelayout16x16,   0, 16 },	/* sprites 16x16*/
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	2000000,	/* 2 MHz ? (hand tuned) */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz frequency */
	3,              /* memory region 3 */
	{ 128 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0,
			readmem,writemem,0,0,
			gaiden_interrupt,1,0,0
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 30*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gaiden_vh_start,
	gaiden_vh_stop,
	gaiden_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( gaiden_rom )
	ROM_REGION(0x40000)	/* 2*128k for 68000 code */
	ROM_LOAD_EVEN( "gaiden.1", 0x00000, 0x20000, 0x0b92d014 )
	ROM_LOAD_ODD ( "gaiden.2", 0x00000, 0x20000, 0xb0a2e242 )

	ROM_REGION(0x210000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gaiden.5",  0x000000, 0x10000, 0xf3653a0f )	/* 8x8 tiles */
	ROM_LOAD( "gaiden.6",  0x010000, 0x20000, 0xde50047a )	/* sprites A1 */
	ROM_LOAD( "gaiden.7",  0x030000, 0x20000, 0x9f40c78e )	/* sprites A2 */
	ROM_LOAD( "gaiden.8",  0x050000, 0x20000, 0xc0d813a6 )	/* sprites B1 */
	ROM_LOAD( "gaiden.9",  0x070000, 0x20000, 0xdeea0a22 )	/* sprites B2 */
	ROM_LOAD( "gaiden.10", 0x090000, 0x20000, 0xfc2e2818 )	/* sprites C1 */
	ROM_LOAD( "gaiden.11", 0x0b0000, 0x20000, 0xfb2d2641 )	/* sprites C2 */
	ROM_LOAD( "gaiden.12", 0x0D0000, 0x20000, 0xe7739fed )	/* sprites D1 */
	ROM_LOAD( "gaiden.13", 0x0f0000, 0x20000, 0xbd928520 )	/* sprites D2 */
	ROM_LOAD( "14.bin",    0x110000, 0x20000, 0x00d6531c )
	ROM_LOAD( "15.bin",    0x130000, 0x20000, 0xae21271b )
	ROM_LOAD( "16.bin",    0x150000, 0x20000, 0x4dee5eec )
	ROM_LOAD( "17.bin",    0x170000, 0x20000, 0x9e0c7684 )
	ROM_LOAD( "18.bin",    0x190000, 0x20000, 0xc76c8d20 )
	ROM_LOAD( "19.bin",    0x1b0000, 0x20000, 0xc3845b96 )
	ROM_LOAD( "20.bin",    0x1d0000, 0x20000, 0x40e78ca5 )
	ROM_LOAD( "21.bin",    0x1f0000, 0x20000, 0xccdcbd02 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gaiden.3", 0x0000, 0x10000, 0x1db13fc3 )   /* Audio CPU is a Z80  */

	ROM_REGION(0x20000)	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "gaiden.4", 0x0000, 0x20000, 0xb8e72ccf ) /* samples */
ROM_END

ROM_START( shadoww_rom )
	ROM_REGION(0x40000)	/* 2*128k for 68000 code */
	ROM_LOAD_EVEN( "shadoww.1", 0x00000, 0x20000, 0x5e4e90c2 )
	ROM_LOAD_ODD ( "shadoww.2", 0x00000, 0x20000, 0x33f84e7e )

	ROM_REGION(0x210000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gaiden.5",    0x000000, 0x10000, 0xf3653a0f )	/* 8x8 tiles */
	ROM_LOAD( "gaiden.6",    0x010000, 0x20000, 0xde50047a )	/* sprites A1 */
	ROM_LOAD( "gaiden.7",    0x030000, 0x20000, 0x9f40c78e )	/* sprites A2 */
	ROM_LOAD( "gaiden.8",    0x050000, 0x20000, 0xc0d813a6 )	/* sprites B1 */
	ROM_LOAD( "gaiden.9",    0x070000, 0x20000, 0xdeea0a22 )	/* sprites B2 */
	ROM_LOAD( "gaiden.10",   0x090000, 0x20000, 0xfc2e2818 )	/* sprites C1 */
	ROM_LOAD( "gaiden.11",   0x0b0000, 0x20000, 0xfb2d2641 )	/* sprites C2 */
	ROM_LOAD( "shadoww.12a", 0x0D0000, 0x10000, 0xb9da7f3c )	/* sprites D1 */
	ROM_LOAD( "shadoww.12b", 0x0E0000, 0x10000, 0xe2915fb7 )	/* sprites D1 */
	ROM_LOAD( "shadoww.13a", 0x0f0000, 0x10000, 0x5e66bed4 )	/* sprites D2 */
	ROM_LOAD( "shadoww.13b", 0x100000, 0x10000, 0x8a0508d7 )	/* sprites D2 */
	ROM_LOAD( "14.bin",      0x110000, 0x20000, 0x00d6531c )
	ROM_LOAD( "15.bin",      0x130000, 0x20000, 0xae21271b )
	ROM_LOAD( "16.bin",      0x150000, 0x20000, 0x4dee5eec )
	ROM_LOAD( "17.bin",      0x170000, 0x20000, 0x9e0c7684 )
	ROM_LOAD( "18.bin",      0x190000, 0x20000, 0xc76c8d20 )
	ROM_LOAD( "19.bin",      0x1b0000, 0x20000, 0xc3845b96 )
	ROM_LOAD( "20.bin",      0x1d0000, 0x20000, 0x40e78ca5 )
	ROM_LOAD( "21.bin",      0x1f0000, 0x20000, 0xccdcbd02 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gaiden.3", 0x0000, 0x10000, 0x1db13fc3 )   /* Audio CPU is a Z80  */

	ROM_REGION(0x20000)	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "gaiden.4", 0x0000, 0x20000, 0xb8e72ccf ) /* samples */
ROM_END


struct GameDriver gaiden_driver =
{
	__FILE__,
	0,
	"gaiden",
	"Ninja Gaiden",
	"1988",
	"Tecmo",
	"Alex Pasadyn",
	0,
	&machine_driver,

	gaiden_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gaiden_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver shadoww_driver =
{
	__FILE__,
	&gaiden_driver,
	"shadoww",
	"Shadow Warriors",
	"1988",
	"Tecmo",
	"Alex Pasadyn",
	0,
	&machine_driver,

	shadoww_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gaiden_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

