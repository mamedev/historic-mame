/***************************************************************************

Renegade (preliminary)
(c)1986 Taito

Nekketsu Kouha Kunio Kun (bootleg)
(c)1986 TECHNOS JAPAN

By Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino

To enter test mode, hold down Player#1 and Player#2 start buttons as the game starts up.

known issues:
	68705 emulation (not needed for bootleg)

	cocktail mode not yet supported

	adpcm samples need to be hooked up

	high score save

	some of the graphics ROMs may be glitched (look at the knife guys on
	the last stage)

	minor sprite glitches:

	- sprites are visible on high score screen

	- first stage boss (before he enters the playfield) isn't fully
	  drawn when the playfield is scrolled to the left

	- third stage boss's lower body is garbled when she is knocked
	  down

Misc Info:

NMI is used to refresh the sprites
IRQ is used to handle coin inputs

Memory Map (Preliminary):

Working RAM
  $24		used to mirror bankswitch state
  $26		#credits (decimal)
  $2C -  $2D	sprite refresh trigger (used by NMI)
  $31		live/demo (if live, player controls are read from input ports)
  $32		indicates 2 player (alternating) game, or 1 player game
  $33		active player
  $37		stage number
  $38		stage state (for stages with more than one part)
  $40		game status flags; 0x80 indicates time over, 0x40 indicates player dead
 $220		player health
 $222 -  $223	stage timer
 $48a -  $48b	horizontal scroll buffer
 $511 -  $690	sprite RAM buffer
$1002		#lives
$1014 - $1015	stage timer - separated digits
$1017 - $1019	stage timer: (ticks,seconds,minutes)
$1020 - $1048	high score table
$10e5 - $10ff	68705 data buffer

Video RAM
$1800 - $1bff	Text Layer, characters
$1c00 - $1fff	Text Layer, character attributes
$2000 - $217f	MIX RAM (96 sprites)
$2800 - $2bff	BACK LOW MAP RAM (background tiles)
$2C00 - $2fff	BACK HIGH MAP RAM (background attributes)
$3000 - $30ff	COLOR RG RAM
$3100 - $31ff	COLOR B RAM

Registers
$3800w	scroll(0ff)
$3801w	scroll(300)
$3802w	adpcm out?
$3803w	interrupt enable

$3804w	send data from 68705
		sets a flip flop
		clears bit 0 of port C
		generates an interrupt

	a read from (?) sets bit 1 of port C

$3804r	receive data from 68705



$3805w	bankswitch


$3806w	watchdog?
$3807w	?

$3800r	'player1'
		x	x							start buttons
				x	x					fire buttons
						x	x	x	x	joystick state

$3801r	'player2'
		x	x							coin inputs
				x	x					fire buttons
						x	x	x	x	joystick state

$3802r	'DIP2'
		x								unused?
			x							vblank
				x						0: 68705 is ready to send information
					x					1: 68705 is ready to receive information
						x	x			3rd fire buttons for player 2,1
								x	x	difficulty

$3803r 'DIP1'
		x								screen flip
			x							cabinet type
				x						bonus (extra life for high score)
					x					starting lives: 1 or 2
						x	x	x	x	coins per play

ROM
$4000 - $7fff	bankswitched ROM
$8000 - $ffff	ROM

b8	insert coin
a0	silence?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6502/m6502.h"
#include "m6809/m6809.h"

static void Write68705( int data );
static void Write68705( int data ){
}

static int Read68705( void );
static int Read68705( void ){
	return 0x00;
}

extern void renegade_vh_screenrefresh(struct osd_bitmap *bitmap, int fullrefresh);
extern void renegade_paletteram_w(int offset, int data);
extern int renegade_vh_start( void );
extern void renegade_vh_stop( void );

extern unsigned char *renegade_textram;
extern int renegade_scrollx;
extern int renegade_video_refresh;

static int interrupt_renegade_e = 0x00;
static unsigned char renegade_waitIRQsound = 0;
static int bank = 0;

void renegade_register_w( int offset, int data );
void renegade_register_w( int offset, int data ){
	switch( offset ){
		case 0x0:
		renegade_scrollx = (renegade_scrollx&0x300)|data;
		break;

		case 0x1:
		renegade_scrollx = (renegade_scrollx&0xFF)|((data&3)<<8);
		break;

		case 0x2:
		renegade_waitIRQsound = 1;
		soundlatch_w(offset,data);
		break;

		case 0x3:
		interrupt_renegade_e = data;
		break;

		case 0x4:
		Write68705( data );
		break;

		case 0x5: /* bankswitch */
		if( (data&1)!=bank ){
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
			bank = data&1;
			cpu_setbank(1,&RAM[ bank?0x10000:0x4000 ]);
		}
		break;

		case 0x6: /* unknown - watchdog? */
		case 0x7:
		break;
	}
}

int renegade_register_r( int offset );
int renegade_register_r( int offset ){
	switch( offset ){
		case 0: return input_port_0_r(0);	/* Player#1 controls */
		case 1: return input_port_1_r(0);	/* Player#2 controls */
		case 2: return input_port_2_r(0);	/* DIP2 ; various IO ports */
		case 3: return input_port_3_r(0);	/* DIP1 */
		case 4: return Read68705();
	}
	return 0;
}

static struct MemoryReadAddress main_readmem[] = {
	{ 0x0000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x380f, renegade_register_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress main_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_RAM },						/* Working RAM */
	{ 0x1800, 0x1fff, MWA_RAM, &renegade_textram },		/* Text Layer */
	{ 0x2000, 0x27ff, MWA_RAM, &spriteram },			/* MIX RAM (sprites) */
	{ 0x2800, 0x2bff, MWA_RAM, &videoram },				/* BACK LOW MAP RAM */
	{ 0x2C00, 0x2fff, MWA_RAM }, 						/* BACK HIGH MAP RAM */
	{ 0x3000, 0x30ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3100, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x3800, 0x380f, renegade_register_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }
};

static int renegade_interrupt(void){
	static int count = 0;

	/* IRQ is used to handle coin insertion */
	static int last = 0xC0;
	int current = readinputport(1) & 0xC0;
	if( current!=last ){
		last = current;
		if( current!=0xC0 ) return interrupt();
	}

	/* hack! */
	count++;
	if( count>=2 ){
		count = 0;
		renegade_video_refresh = 1;
		if( interrupt_renegade_e ) return nmi_interrupt();
	}

	return ignore_interrupt();
}

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x2801, 0x2801, YM3526_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
/*
	1800 \
	2000 - adpcm related?
	3000 /
*/

	{ 0x2800, 0x2800, YM3526_control_port_0_w },
	{ 0x2801, 0x2801, YM3526_write_port_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};


static int renegade_sound_interrupt(void){
	if (renegade_waitIRQsound){
		renegade_waitIRQsound = 0;
		return (M6809_INT_IRQ);
	}
	return (M6809_INT_FIRQ);
}

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (not supported) */
	{ 255 }		/* (not supported) */
};

INPUT_PORTS_START( input_ports )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* attack left */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* jump */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START /* DIP2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING( 0x00, "Hard" )
	PORT_DIPSETTING( 0x01, "Very Hard" )
	PORT_DIPSETTING( 0x02, "Easy" )
	PORT_DIPSETTING( 0x03, "Normal" )

	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* attack right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 	/* 68705 status */
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) 	/* 68705 status */
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DIP1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x01, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x03, "1 Coin/1 Credits" )

	PORT_DIPNAME( 0x0C, 0x0C, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x0C, "1 Coin/1 Credits" )

 	PORT_DIPNAME (0x10, 0x00, "Lives", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x10, "1" )
 	PORT_DIPSETTING (   0x00, "2" )

 	PORT_DIPNAME (0x20, 0x20, "Bonus", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x20, "30k" )
 	PORT_DIPSETTING (   0x00, "None" )

 	PORT_DIPNAME (0x40, 0x40, "Cabinet Type", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x40, "Table" )
 	PORT_DIPSETTING (   0x00, "Upright" )

 	PORT_DIPNAME (0x80, 0x80, "Video Flip", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x80, "Off" )
 	PORT_DIPSETTING (   0x00, "On" )
INPUT_PORTS_END

static struct GfxLayout charlayout1 =
{
	8,8, /* 8x8 characters */
	1024, /* 1024 characters */
	3, /* bits per pixel */
	{ 2, 4, 6 },	/* plane offsets; bit 0 is always clear */
	{ 1, 0, 65, 64, 129, 128, 193, 192 }, /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 }, /* y offsets */
	32*8 /* offset to next character */
};
static struct GfxLayout charlayout =
{
	8,8, /* 8x8 characters */
	1024, /* 1024 characters */
	3, /* bits per pixel */
	{ 2, 4, 6 },	/* plane offsets; bit 0 is always clear */
	{ 1, 0, 65, 64, 129, 128, 193, 192 }, /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 }, /* y offsets */
	32*8 /* offset to next character */
};

static struct GfxLayout tileslayout1 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 4, 0x8000*8+0, 0x8000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout2 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0, 0xC000*8+0, 0xC000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout3 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0x4000*8+4, 0x10000*8+0, 0x10000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout4 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0x4000*8+0, 0x14000*8+0, 0x14000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* 8x8 text, 8 colors */
	{ 1, 0x00000, &charlayout,     0, 4 },	/* colors   0- 32 */

	/* 16x16 background tiles, 8 colors */
	{ 1, 0x08000, &tileslayout1, 192, 8 },	/* colors 192-255 */
	{ 1, 0x08000, &tileslayout2, 192, 8 },
	{ 1, 0x08000, &tileslayout3, 192, 8 },
	{ 1, 0x08000, &tileslayout4, 192, 8 },

	{ 1, 0x20000, &tileslayout1, 192, 8 },
	{ 1, 0x20000, &tileslayout2, 192, 8 },
	{ 1, 0x20000, &tileslayout3, 192, 8 },
	{ 1, 0x20000, &tileslayout4, 192, 8 },

	/* 16x16 sprites, 8 colors */
	{ 1, 0x38000, &tileslayout1, 128, 4 },	/* colors 128-159 */
	{ 1, 0x38000, &tileslayout2, 128, 4 },
	{ 1, 0x38000, &tileslayout3, 128, 4 },
	{ 1, 0x38000, &tileslayout4, 128, 4 },

	{ 1, 0x68000, &tileslayout1, 128, 4 },
	{ 1, 0x68000, &tileslayout2, 128, 4 },
	{ 1, 0x68000, &tileslayout3, 128, 4 },
	{ 1, 0x68000, &tileslayout4, 128, 4 },

	{ 1, 0x50000, &tileslayout1, 128, 4 },
	{ 1, 0x50000, &tileslayout2, 128, 4 },
	{ 1, 0x50000, &tileslayout3, 128, 4 },
	{ 1, 0x50000, &tileslayout4, 128, 4 },

	{ 1, 0x80000, &tileslayout1, 128, 4 },
	{ 1, 0x80000, &tileslayout2, 128, 4 },
	{ 1, 0x80000, &tileslayout3, 128, 4 },
	{ 1, 0x80000, &tileslayout4, 128, 4 },
	{ -1 }
};

static struct MachineDriver renegade_machine_driver =
{
	{
		{
 			CPU_M6502,
			1500000,	/* 1.5 MHz */
			0,
			main_readmem,main_writemem,0,0,
			renegade_interrupt,1
		}
		,{
 			CPU_M6809 | CPU_AUDIO_CPU,
			1200000,	/* ? */
			2,
			sound_readmem,sound_writemem,0,0,
			renegade_sound_interrupt,16
		}
	},
	60,
	DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1, /* cpu slices */
	0, /* init machine */

	32*8, 32*8,
	{ 8, 31*8-1, 0, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	renegade_vh_start,
	renegade_vh_stop,
	renegade_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    }
	}
};

ROM_START( renegade_rom )
	ROM_REGION(0x14000)	/* 64k for code + bank switched ROM */
	ROM_LOAD( "nb-5.BIN", 0x8000, 0x8000, 0x6d1f3113 )
	ROM_LOAD( "na-5.BIN", 0x4000, 0x4000, 0x6b5ecea0 )
	ROM_CONTINUE( 0x10000, 0x4000 )

	ROM_REGION(0x98000) /* temporary space for graphics (disposed after conversion) */

	/* characters */
	ROM_LOAD( "nc-5.BIN",  0x00000, 0x8000, 0xd69c081e )	/* 4 bitplanes */

	/* tiles */
	ROM_LOAD( "n1-5.bin",  0x08000, 0x8000, 0xf4b647ce )	/* bitplane 3 */
	ROM_LOAD( "n6-5.bin",  0x10000, 0x8000, 0x1b1d27db )	/* bitplanes 1,2 */
	ROM_LOAD( "n7-5.bin",  0x18000, 0x8000, 0xcf04d9a0 )	/* bitplanes 1,2 */

	ROM_LOAD( "n2-5.bin",  0x20000, 0x8000, 0x458feab7 )	/* bitplane 3 */
	ROM_LOAD( "n8-5.bin",  0x28000, 0x8000, 0xef22bb06 )	/* bitplanes 1,2 */
	ROM_LOAD( "n9-5.bin",  0x30000, 0x8000, 0x77f07e56 )	/* bitplanes 1,2 */

	/* sprites */
	ROM_LOAD( "nh-5.bin",  0x38000, 0x8000, 0x7f9d5463 )	/* bitplane 3 */
	ROM_LOAD( "nd-5.bin",  0x40000, 0x8000, 0xdbc54395 )	/* bitplanes 1,2 */
	ROM_LOAD( "nj-5.bin",  0x48000, 0x8000, 0xeee3d7ef )	/* bitplanes 1,2 */

	ROM_LOAD( "ni-5.bin",  0x50000, 0x8000, 0x2792480c )	/* bitplane 3 */
	ROM_LOAD( "nf-5.bin",  0x58000, 0x8000, 0x5ca5d811 )	/* bitplanes 1,2 */
	ROM_LOAD( "nl-5.bin",  0x60000, 0x8000, 0x7d932607 )	/* bitplanes 1,2 */

	ROM_LOAD( "nn-5.bin",  0x68000, 0x8000, 0xf918fb7a )	/* bitplane 3 */
	ROM_LOAD( "ne-5.bin",  0x70000, 0x8000, 0x5b800b36 )	/* bitplanes 1,2 */
	ROM_LOAD( "nk-5.bin",  0x78000, 0x8000, 0xbd328ea8 )	/* bitplanes 1,2 */

	ROM_LOAD( "no-5.bin",  0x80000, 0x8000, 0x43b494d0 )	/* bitplane 3 */
	ROM_LOAD( "ng-5.bin",  0x88000, 0x8000, 0x6943ff3b )	/* bitplanes 1,2 */
	ROM_LOAD( "nm-5.bin",  0x90000, 0x8000, 0xffd47a0a )	/* bitplanes 1,2 */

	ROM_REGION(0x10000) /* audio CPU (M6809) */
	ROM_LOAD( "n0-5.bin", 0x08000,0x08000,	0xb6094bf9 )

/*
	4-bit adpcm sampled sound:
		ROM_LOAD( "n3-5.bin" )
		ROM_LOAD( "n4-5.bin" )
		ROM_LOAD( "n5-5.bin" )
*/
ROM_END

ROM_START( kuniokun_rom )
	ROM_REGION(0x14000)	/* 64k for code + bank switched ROM */
	ROM_LOAD( "TA18-10.BIN", 0x8000, 0x8000, 0x2e0e998e )
	ROM_LOAD( "TA18-11.BIN", 0x4000, 0x4000, 0xd6362034 )
	ROM_CONTINUE( 0x10000, 0x4000 )

	ROM_REGION(0x98000) /* temporary space for graphics (disposed after conversion) */

	/* characters */
	ROM_LOAD( "TA18-25.BIN",  0x00000, 0x8000, 0x67cd0a2b )	/* 4 bitplanes */

	ROM_LOAD( "TA18-01.BIN",  0x08000, 0x8000, 0x134b2dd1 )
	ROM_LOAD( "TA18-06.BIN",  0x10000, 0x8000, 0x6a47d299 )
	ROM_LOAD( "TA18-05.BIN",  0x18000, 0x8000, 0xcf04d9a0 )

	ROM_LOAD( "TA18-02.BIN",  0x20000, 0x8000, 0x4c3d1cd5 )
	ROM_LOAD( "TA18-04.bin",  0x28000, 0x8000, 0xc0000000 )	/* bad! */
	ROM_LOAD( "TA18-03.BIN",  0x30000, 0x8000, 0x75efad31 )

	/* sprites */
	ROM_LOAD( "TA18-20.BIN",  0x38000, 0x8000, 0x9b1d3b5b )
	ROM_LOAD( "TA18-24.BIN",  0x40000, 0x8000, 0x77c9072f )
	ROM_LOAD( "TA18-18.BIN",  0x48000, 0x8000, 0xab207ec0 )

	ROM_LOAD( "TA18-19.BIN",  0x50000, 0x8000, 0xeb9dfbef )
	ROM_LOAD( "TA18-22.BIN",  0x58000, 0x8000, 0xc6d659a8 )
	ROM_LOAD( "TA18-16.BIN",  0x60000, 0x8000, 0x9dacf54e )

	ROM_LOAD( "TA18-14.BIN",  0x68000, 0x8000, 0x4b8853b6 )
	ROM_LOAD( "TA18-23.BIN",  0x70000, 0x8000, 0x1855745b )
	ROM_LOAD( "TA18-17.BIN",  0x78000, 0x8000, 0x83d283e4 )

	ROM_LOAD( "TA18-13.BIN",  0x80000, 0x8000, 0x61a0bfaa )
	ROM_LOAD( "TA18-21.BIN",  0x88000, 0x8000, 0xa2487992 )
	ROM_LOAD( "TA18-15.BIN",  0x90000, 0x8000, 0xf42e859c )

	ROM_REGION(0x10000) /* audio CPU (M6809) */
	ROM_LOAD( "TA18-00.BIN", 0x08000,0x08000,	0xb6094bf9 )

	ROM_REGION(0x40000) /* adpcm */
	ROM_LOAD( "TA18-07.BIN", 0x00000, 0x8000, 0xdb1f09d7 )
	ROM_LOAD( "TA18-08.BIN", 0x08000, 0x8000, 0xa6e4c8aa )
	ROM_LOAD( "TA18-09.BIN", 0x10000, 0x8000, 0xc121b46f )
ROM_END



struct GameDriver renegade_driver =
{
	__FILE__,
	0,
	"renegade",
	"Renegade (US)",
	"1986",
	"Taito",
	"Phil Stroffolino\nCarlos A. Lozano\nRob Rosenbrock",
	GAME_NOT_WORKING,
	&renegade_machine_driver,

	renegade_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver kuniokub_driver =
{
	__FILE__,
	&renegade_driver,
	"kuniokub",
	"Nekketsu Kouha Kunio Kun (Jap bootleg)",
	"1986",
	"bootleg",
	"Phil Stroffolino\nCarlos A. Lozano\nRob Rosenbrock",
	0,
	&renegade_machine_driver,

	kuniokun_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
