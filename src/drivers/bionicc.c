/********************************************************************

			  Bionic Commando



ToDo:
- optimize the video driver (it currently doesn't use tmpbitmaps)

- get rid of input port hack

	Controls appear to be mapped at 0xFE4000, alongside dip switches, but there
	is something strange going on that I can't (yet) figure out.
	Player controls and coin inputs are supposed to magically appear at
	0xFFFFFB (coin/start)
	0xFFFFFD (player 2)
	0xFFFFFF (player 1)
	This is probably done by an MPU on the board (whose ROM is not
	available).

	The MPU also takes care of the commands for the sound CPU, which are stored
	at FFFFF9.

	IRQ4 seems to be control related.
	On each interrupt, it reads 0xFE4000 (coin/start), shift the bits around
	and move the resulting byte into a dword RAM location. The dword RAM location
	is rotated by 8 bits each time this happens.
	This is probably done to be pedantic about coin insertions (might be protection
	related). In fact, currently coin insertions are not consistently recognized.

********************************************************************/


#include "driver.h"

extern int bionicc_videoreg_r( int offset );
extern void bionicc_videoreg_w( int offset, int value );

extern unsigned char *bionicc_scroll2;
extern unsigned char *bionicc_scroll1;
extern unsigned char *bionicc_palette;
extern unsigned char *spriteram;
extern unsigned char *videoram;

extern int bionicc_vh_start(void);
extern void bionicc_vh_stop(void);
extern void bionicc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static void bionicc_readinputs(void);
static void bionicc_sound_cmd(int data);



int bionicc_inputs_r(int offset)
{
if (errorlog) fprintf(errorlog,"%06x: inputs_r %04x\n",cpu_getpc(),offset);
	return (readinputport(offset)<<8) + readinputport(offset+1);
}

static unsigned char bionicc_inp[6];

void hacked_controls_w( int offset, int data )
{
if (errorlog) fprintf(errorlog,"%06x: hacked_controls_w %04x %02x\n",cpu_getpc(),offset,data);
	COMBINE_WORD_MEM( &bionicc_inp[offset], data);
}

static int hacked_controls_r(int offset)
{
if (errorlog) fprintf(errorlog,"%06x: hacked_controls_r %04x %04x\n",cpu_getpc(),offset,READ_WORD( &bionicc_inp[offset] ));
	return READ_WORD( &bionicc_inp[offset] );
}

static void bionicc_mpu_trigger_w(int offset,int data)
{
	data = readinputport(0) >> 4;
	WRITE_WORD(&bionicc_inp[0x00],data ^ 0x0f);

	data = readinputport(5); /* player 2 controls */
	WRITE_WORD(&bionicc_inp[0x02],data ^ 0xff);

	data = readinputport(4); /* player 1 controls */
	WRITE_WORD(&bionicc_inp[0x04],data ^ 0xff);
}



static unsigned char soundcommand[2];

void hacked_soundcommand_w( int offset, int data )
{
	COMBINE_WORD_MEM( &soundcommand[offset], data);
	soundlatch_w(0,data & 0xff);
}

static int hacked_soundcommand_r(int offset)
{
	return READ_WORD( &soundcommand[offset] );
}


/********************************************************************

  INTERRUPT

  The game runs on 2 interrupts.

  IRQ 2 drives the game
  IRQ 4 processes the input ports

  The game is very picky about timing. The following is the only
  way I have found it to work.

********************************************************************/

int bionicc_interrupt(void)
{
	if (cpu_getiloops() == 0) return 2;
	else return 4;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },                /* 68000 ROM */
	{ 0xfe0000, 0xfe07ff, MRA_BANK1 },              /* RAM? */
	{ 0xfe0800, 0xfe0cff, MRA_BANK2 },              /* sprites */
	{ 0xfe0d00, 0xfe3fff, MRA_BANK3 },              /* RAM? */
	{ 0xfe4000, 0xfe4003, bionicc_inputs_r },       /* dipswitches */
	{ 0xfe8010, 0xfe8017, bionicc_videoreg_r },     /* scroll registers */
	{ 0xfec000, 0xfecfff, MRA_BANK4 },              /* fixed text layer */
	{ 0xff0000, 0xff3fff, MRA_BANK5 },              /* SCROLL1 layer */
	{ 0xff4000, 0xff7fff, MRA_BANK6 },              /* SCROLL2 layer */
	{ 0xff8000, 0xff86ff, MRA_BANK7 },              /* palette RAM */
	{ 0xffc000, 0xfffff7, MRA_BANK8 },               /* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_r },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_r },      /* hack */
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0000, 0xfe07ff, MWA_BANK1 },	/* RAM? */
	{ 0xfe0800, 0xfe3fff, MWA_BANK2, &spriteram },
	{ 0xfe8010, 0xfe8017, bionicc_videoreg_w },
	{ 0xfe801a, 0xfe801b, bionicc_mpu_trigger_w },	/* ??? not sure, but looks like it */
	{ 0xfec000, 0xfecfff, MWA_BANK4, &videoram },
	{ 0xff0000, 0xff3fff, MWA_BANK5, &bionicc_scroll1 },
	{ 0xff4000, 0xff7fff, MWA_BANK6, &bionicc_scroll2 },
	{ 0xff8000, 0xff86ff, MWA_BANK7, &bionicc_palette },
	{ 0xffc000, 0xfffff7, MWA_BANK8 },	/* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_w },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_w },	/* hack */
	{ -1 }
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8001, 0x8001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2151_register_port_0_w },
	{ 0x8001, 0x8001, YM2151_data_port_0_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( bionicc_input_ports )
	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20K, 40K, every 60K")
	PORT_DIPSETTING(    0x10, "30K, 50K, every 70K" )
	PORT_DIPSETTING(    0x08, "20K and 60K only")
	PORT_DIPSETTING(    0x00, "30K and 70K only" )
	PORT_DIPNAME( 0x60, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Medium")
	PORT_DIPSETTING(    0x20, "Hard")
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/********************************************************************

  GRAPHICS

********************************************************************/


static struct GfxLayout spritelayout_bionicc=
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 0x30000*8,0x20000*8,0x10000*8,0 },
	{
		0,1,2,3,4,5,6,7,
		(16*8)+0,(16*8)+1,(16*8)+2,(16*8)+3,
		(16*8)+4,(16*8)+5,(16*8)+6,(16*8)+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	},
	256   /* every sprite takes 256 consecutive bytes */
};

static struct GfxLayout vramlayout_bionicc=
{
	8,8,    /* 8*8 characters */
	2048,   /* 2048 character */
	2,      /* 2 bitplanes */
	{ 4,0 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every character takes 128 consecutive bytes */
};

static struct GfxLayout scroll2layout_bionicc=
{
	8,8,    /* 8*8 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x08000*8)+4,0x08000*8,4,0 },
	{ 0,1,2,3, 8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every tile takes 128 consecutive bytes */
};

static struct GfxLayout scroll1layout_bionicc=
{
	16,16,  /* 16*16 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x020000*8)+4,0x020000*8,4,0 },
	{
		0,1,2,3, 8,9,10,11,
		(8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
		(8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	512   /* each tile takes 512 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_bionicc[] =
{
	{ 1, 0x48000, &scroll2layout_bionicc, 0,    4 },
	{ 1, 0x58000, &scroll1layout_bionicc, 64,   4 },
	{ 1, 0x00000, &spritelayout_bionicc,  128, 16 },
	{ 1, 0x40000, &vramlayout_bionicc,    384, 16 },
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579580,                /* 3.579580 MHz ? */
	{ 255 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* ?? MHz ? */
			0,
			readmem,writemem,0,0,
			bionicc_interrupt,8
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,4	/* ??? */
		}
	},
	60, 5000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_bionicc,
	64*3+256, /* colours */
	64*3+256, /* Colour table length */
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bionicc_vh_start,
	bionicc_vh_stop,
	bionicc_vh_screenrefresh,

	0,0,0,0,
	{
		 { SOUND_YM2151,  &ym2151_interface },
	}
};



ROM_START( bionicc_rom )
	ROM_REGION(0x40000)      /* 68000 code */
	ROM_LOAD_EVEN("TSU_02B.ROM", 0x00000, 0x10000, 0x2cfa1b7c ) /* 68000 code */
	ROM_LOAD_ODD ("TSU_04B.ROM", 0x00000, 0x10000, 0xb8683c02 ) /* 68000 code */
	ROM_LOAD_EVEN("TSU_03B.ROM", 0x20000, 0x10000, 0x00fc7968 ) /* 68000 code */
	ROM_LOAD_ODD ("TSU_05B.ROM", 0x20000, 0x10000, 0x9752acdc ) /* 68000 code */

	ROM_REGION(0x098000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "TSU_10.ROM", 0x000000, 0x08000, 0x11537a11 )	/* Sprites */
	ROM_LOAD( "TSU_09.ROM", 0x008000, 0x08000, 0xc102ed0e )
	ROM_LOAD( "TSU_15.ROM", 0x010000, 0x08000, 0x9df09b68 )
	ROM_LOAD( "TSU_14.ROM", 0x018000, 0x08000, 0x4bebdef5 )
	ROM_LOAD( "TSU_20.ROM", 0x020000, 0x08000, 0xe03c8ca6 )
	ROM_LOAD( "TSU_19.ROM", 0x028000, 0x08000, 0xcc55b54f )
	ROM_LOAD( "TSU_22.ROM", 0x030000, 0x08000, 0x21628a8e )
	ROM_LOAD( "TSU_21.ROM", 0x038000, 0x08000, 0x2b4ea408 )
	ROM_LOAD( "TSU_08.ROM", 0x040000, 0x08000, 0x8eda0000 )	/* VIDEORAM (text layer) tiles */
	ROM_LOAD( "TSU_07.ROM", 0x048000, 0x08000, 0x137a62ce )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "TSU_06.ROM", 0x050000, 0x08000, 0xecc9ec5d )
	ROM_LOAD( "TS_12.ROM",  0x058000, 0x08000, 0x49c23374 )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "TS_11.ROM",  0x060000, 0x08000, 0x452733d9 )
	ROM_LOAD( "TS_17.ROM",  0x068000, 0x08000, 0xfecf91af )
	ROM_LOAD( "TS_16.ROM",  0x070000, 0x08000, 0x130bc553 )
	ROM_LOAD( "TS_13.ROM",  0x078000, 0x08000, 0xf8498663 )
	ROM_LOAD( "TS_18.ROM",  0x080000, 0x08000, 0xb356a558 )
	ROM_LOAD( "TS_23.ROM",  0x088000, 0x08000, 0x5d62bc70 )
	ROM_LOAD( "TS_24.ROM",  0x090000, 0x08000, 0x00296253 )

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "TSU_01B.ROM",0x00000, 0x8000, 0x0ae40f4a )
ROM_END



struct GameDriver bionicc_driver =
{
	__FILE__,
	0,
	"bionicc",
	"Bionic Commando",
	"1987",
	"Capcom",
	"Steven Frew\nPhil Stroffolino\nPaul Leaman",
	0,
	&machine_driver,

	bionicc_rom,
	0,
	0,0,
	0,

	bionicc_input_ports,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};

