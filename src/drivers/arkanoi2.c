/***************************************************************************

				Arkanoid 2 - Revenge of Doh!
				    (C) 1987 Taito

					    driver by

				Luca Elia (eliavit@unina.it)
				Mirko Buffoni

 This game runs on almost the same hardware as "The New Zealand Story"'s
 or maybe it's all the way round since this is an earlier  game. See the
 tnzs driver for a description of how the hardware works. I'll just hint
 at some differences here:

- The values making up the fixed security sequence (read at startup from
  c000 by the sound cpu) are different.

- Reads from c000 will yeld the coin counter and the input port 1 values
  in an alternate fashion.

- c000 will be reset to yeld the coin counter when c001 is written. The
  real mechanism of this has to be more complex (for example some specific
  values are written there in sequence: 54h 41h), and yet to be understood.
  This goes for the handling of the coin counter as well.

- Writes to c000 will modify the coin counter (the data written gets
  subracted from it, as it seems). I don't know if tnzs acts the same.

- Bits 3-0 of f301 seem to have a different meaning in this game:
  the only values written are AFAIK 2c,2a,20 and 01 (when some hardware
  error message is shown on screen). The background must be drawn in either
  case. What's more, the tiles must be drawn from f400 to f5ff (the opposite
  order of tnzs). Bits 1&2, seem to have a function also.

- The game doesn't write to f800-fbff (static palette?)

					To be done
					----------

- Colors are completely wrong, probably we just need a missing color prom.
- Test mode 2 (press start2 when test dsw L) doesn't work\display well.
- What do writes at f400 and f381 do ?
- Why the game zeros the fd00 area ?
- Does it need bankswitching ?

			Interesting routines (main cpu)
			-------------------------------

1ed	prints the test screen (first string at 206)

47a	prints dipsw1&2 e 1p&2p paddleL values:
	e821		IN DIPSW1		e823-4	1P PaddleL (lo-hi)
	e822		IN DIPSW2		e825-6	2P PaddleL (lo-hi)

584	prints OK or NG on each entry:
	if (*addr)!=0 { if (*addr)!=2 OK else NG }
	e880	1P PADDLEL		e88a	IN SERVICE
	e881	1P PADDLER		e88b	IN TILT
	e882	1P BUTTON		e88c	OUT LOCKOUT1
	e883	1P START		e88d	OUT LOCKOUT2
	e884	2P PADDLEL		e88e	IN DIP-SW1
	e885	2P PADDLER		e88f	IN DIP-SW2
	e886	2P BUTTON		e890	SND OPN
	e887	2P START		e891	SND SSGCH1
	e888	IN COIN1		e892	SND SSGCH2
	e889	IN COIN2		e893	SND SSGCH3

672	prints a char
715	prints a string (0 terminated)

		Shared Memory (values written mainly by the sound cpu)
		------------------------------------------------------

e001=dip-sw A 	e399=coin counter value		e72c-d=1P paddle (lo-hi)
e002=dip-sw B 	e3a0-2=1P score/10 (BCD)	e72e-f=2P paddle (lo-hi)
e008=level=2*(shown_level-1)+x <- remember it's a binary tree (42 last)
e7f0=country code(from 9fde in sound rom)
e807=counter, reset by sound cpu, increased by main cpu each vblank
e80b=test progress=0(start) 1(first 8) 2(all ok) 3(error)
ec09-a~=ed05-6=xy pos of cursor in hi-scores
ec81-eca8=hi-scores(8bytes*5entries)

addr	bit	name		active	addr	bit	name		active
e72d	6	coin[1]	low		e729	1	2p select	low
	5	service	high			0	1p select	low
	4	coin[2]	low

addr	bit	name		active	addr	bit	name		active
e730	7	tilt		low		e7e7	4	1p fire	low
							0	2p fire	low

			Interesting routines (sound cpu)
			--------------------------------

4ae	check starts	B73,B7a,B81,B99	coin related
8c1	check coins		62e lockout check		664	dsw check

			Interesting locations (sound cpu)
			---------------------------------

d006=each bit is on if a corresponding location (e880-e887) has changed
d00b=(c001)>>4=tilt if 0E (security sequence must be reset?)
addr	bit	name		active
d00c	7	tilt
	6	?service?
	5	coin2		low
	4	coin1		low

d00d=each bit is on if the corresponding location (e880-e887) is 1 (OK)
d00e=each of the 4 MSBs is on if ..
d00f=FF if tilt, 00 otherwise
d011=if 00 checks counter, if FF doesn't
d23f=input port 1 value

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"

/* prototypes for functions in ../machine/tnzs.c */
extern unsigned char *tnzs_objram, *tnzs_workram;
extern unsigned char *tnzs_vdcram, *tnzs_scrollram;
extern unsigned char *tnzs_cpu2ram;
extern int tnzs_objram_size;
void init_tnzs(void);
int tnzs_objram_r(int offset);

int tnzs_cpu2ram_r(int offset);
int tnzs_workram_r(int offset);
int tnzs_vdcram_r(int offset);

void tnzs_objram_w(int offset, int data);
void tnzs_inputport_w(int offset, int data);
void tnzs_cpu2ram_w(int offset, int data);
void tnzs_workram_w(int offset, int data);
void tnzs_vdcram_w(int offset, int data);
void tnzs_scrollram_w(int offset, int data);

unsigned char *banked_ram_0, *banked_ram_1;
unsigned char *banked_rom_0, *banked_rom_1, *banked_rom_2, *banked_rom_3;


int current_inputport;	/* reads of c000 (sound cpu) expect a sequence of values */
int current_dipswitch1;	/* reads of 2203 portA&B (sound cpu) expect a sequence of values */
int current_dipswitch2;
int number_of_credits;


/* prototypes for functions in ../vidhrdw/tnzs.c */
void tnzs_videoram_w(int offset,int data);
void tnzs_objectram_w(int offset,int data);
int tnzs_vh_start(void);
void tnzs_vh_stop(void);
void arkanoi2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);




/*
**
**	Main cpu data
**
**
*/


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x9fff, MRA_ROM },		/* Code ROM */
	{ 0xc000, 0xdfff, tnzs_objram_r },
	{ 0xe000, 0xefff, tnzs_workram_r },	/* WORK RAM (shared by the 2 z80's */
	{ 0xf000, 0xf1ff, tnzs_vdcram_r },	/* VDC RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },	/* Code ROM */
	{ 0xc000, 0xdfff, tnzs_objram_w, &tnzs_objram, &tnzs_objram_size },
	{ 0xe000, 0xefff, tnzs_workram_w, &tnzs_workram },
	{ 0xf000, 0xf1ff, tnzs_vdcram_w, &tnzs_vdcram },
	{ 0xf200, 0xf3ff, tnzs_scrollram_w, &tnzs_scrollram }, /* scrolling info */
	{ -1 }  /* end of table */
};



/*
**
** 				Sound cpu data
**
*/



/* number of input ports to be cycled through (coins, buttons etc.) */
#define ip_num 2

int arkanoi2_inputport_r(int offset)
{
	int ret;

	if (offset == 0)
	{
        switch(current_inputport)
		{
			case -3: ret = 0x55; break;
			case -2: ret = 0xaa; break;
			case -1: ret = 0x5a; break;
		case ip_num: current_inputport = 0; /* fall through */
			case 0:  ret = number_of_credits; break;
			default: ret = readinputport(current_inputport); break;
		}
	  current_inputport++;
	  return ret;
	}
	else
		return (0x01);	/* 0xE1 for tilt, 31 service ok */
}


void arkanoi2_inputport_w(int offset, int data)
{
	if (offset == 0)
		number_of_credits -= ((~data)&0xff)+1;	/* sub data from credits */
	else
/* TBD: value written (or its low nibble) subtracted from sequence index? */
		if (current_inputport>=0)	/* if the initial sequence is done */
			current_inputport=0;	/* reset input port number */
}



/* Here we simulate a 12 bit dial position with mame's built-in 8 bit one */
int arkanoi2_sh_f000_r(int offs)
{
static int pos=0;
static int lastval=0;
int val;
unsigned char diff;

	val = readinputport(0)&0xff;
	diff = val-lastval;
	lastval = val;

	pos += (signed char) diff;

	val = (readinputport(4)<<8)+(pos&0x0fff);
	if (offs==0)
		return val&0xff;
	else
		return val>>8;
}



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x9fff, MRA_ROM },			/* code ROM */

	{ 0xb000, 0xb000, YM2203_status_port_0_r  },
	{ 0xb001, 0xb001, YM2203_read_port_0_r  },
	{ 0xc000, 0xc001, arkanoi2_inputport_r},	/* returns coins,input port, etc. */

	{ 0xd000, 0xdfff, tnzs_cpu2ram_r },
	{ 0xe000, 0xefff, tnzs_workram_r },

	{ 0xf000, 0xf001, arkanoi2_sh_f000_r },	/* IN0 paddle */
	{ 0xf002, 0xf003, arkanoi2_sh_f000_r },	/* IN0 paddle (the same?)*/

	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },	/* code ROM */

	{ 0xb000, 0xb000, YM2203_control_port_0_w },	/* YM control */
	{ 0xb001, 0xb001, YM2203_write_port_0_w },	/* YM data write */
	{ 0xc000, 0xc001, arkanoi2_inputport_w },	/* sub coins, reset input port */

	{ 0xd000, 0xdfff, tnzs_cpu2ram_w, &tnzs_cpu2ram },
	{ 0xe000, 0xefff, tnzs_workram_w },

	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 - spinner (1 paddle?) */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 0, 0, 0)

	PORT_START      /* IN1 - read at c000 (sound cpu) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* DSW1 - IN2 */
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
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 - IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX ( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x04, "Off" )
	PORT_DIPSETTING ( 0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN4 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE)
INPUT_PORTS_END



/*
**
** 				Gfx data
**
*/

/* displacement in bits to the lower part of sprites: */
#define lo 8*8*2

/* layout of sprites */
static struct GfxLayout spritelayout =
{
	16,16,						/* dimx, dimy */
	0x20000/(16*16/8),				/* # of elements */
	4,							/* bits per pixel */
	{0, 0x20000*8, 0x40000*8, 0x60000*8},	/* bitplanes displacements IN BITS */
	{0, 1, 2, 3, 4, 5, 6, 7,			/* x displacements IN BITS */
	 8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7},
	{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,	/* y displacements IN BITS */
	 lo+0*8, lo+1*8, lo+2*8, lo+3*8, lo+4*8, lo+5*8, lo+6*8, lo+7*8},
	16*16     						/* displacement to next IN BITS */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
/* Mem region, start, layout*, start color, color sets */
	{ 1, 0, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,					/* chips */
	1500000,				/* ?? Mhz */
	{ YM2203_VOL(0xff,0xff) },	/* gain,volume */
	{ input_port_3_r 	},		/* DSW1 connected to port A */
	{ input_port_4_r	},		/* DSW2 connected to port B */
	{ 0 },
	{ 0 }
};


static struct MachineDriver arkanoi2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,			/* main cpu */
			2000000,			/* ?? Hz (only crystal is 12MHz) */
			0,				/* memory region */
			readmem,writemem,0,0,
			interrupt,1			/* interrupts rtn, # per frame */
		},

		{
			CPU_Z80,					/* sound cpu */
			4000000,					/* ?? Hz */
			2,						/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,1					/* interrupts rtn, # per frame */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,		/* video frequency (Hz), duration */
	100,							/* cpu slices */
	init_tnzs,					/* called at startup */

	/* video hardware */
	16*16, 14*16,			/* screen dimx, dimy (pixels) */
	{ 0, 16*16-1, 0, 14*16-1 },	/* visible rectangle (pixels) */
	gfxdecodeinfo,
	512, 512,
	0,			/* convert color p-roms */
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	tnzs_vh_start,
	tnzs_vh_stop,
	arkanoi2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}

};



/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( arkanoi2_rom )
    ROM_REGION(0x28000)	/* 64k+64k for the first CPU */

/*	ROM_REGION(0x10000)*/		/* Region 0 - main cpu */
	ROM_LOAD( "a2-05.rom", 0x00000, 0x10000, 0x136edf9d )

	ROM_REGION(0x20000*4)		/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "a2-m01.rom", 0x00000, 0x20000, 0x70cc559d )	/* btp 0 */
	ROM_LOAD( "a2-m02.rom", 0x20000, 0x20000, 0x056a985f )	/* btp 1 */
	ROM_LOAD( "a2-m03.rom", 0x40000, 0x20000, 0x49a21c5e )	/* btp 2 */
	ROM_LOAD( "a2-m04.rom", 0x60000, 0x20000, 0x548117c6 )	/* btp 3 */

	ROM_REGION(0x10000)		/* Region 2 - sound cpu */
	ROM_LOAD( "a2-13.rom", 0x00000, 0x10000, 0xe8035ef1 )

ROM_END


static int arkanoi2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xeca5], "\x54\x4b\x4e\xff", 4) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            	osd_fread(f, &RAM[0xec81], 8*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void arkanoi2_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f, &RAM[0xec81], 8*5);
		osd_fclose(f);
	}
}



struct GameDriver arkanoi2_driver =
{
	__FILE__,
	0,
	"arkanoi2",
	"Arkanoid 2 - Revenge of DOH!",
	"1987",
	"Taito",
	"Luca Elia\nMirko Buffoni",
	GAME_WRONG_COLORS,
	&arkanoi2_machine_driver,

	arkanoi2_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	arkanoi2_hiload, arkanoi2_hisave
};


