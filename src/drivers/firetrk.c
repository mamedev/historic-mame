/***************************************************************************

	Atari Fire Truck hardware

	Games supported:
		* Fire Truck

	Known issues:
		* none at this time

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "firetrk.h"

UINT8 firetrk_invert_display;
UINT8 firetrk_bit7_flags;
UINT8 firetrk_bit0_flags;
static UINT8 firetrk_steer1, firetrk_steer2;



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x00, 0x00,
	0x01, 0x00,
	0x02, 0x00,
	0x03, 0x00,
	0x03, 0x03,
	0x02, 0x03,
	0x01, 0x03,
	0x00, 0x03
};

static PALETTE_INIT( firetrk )
{
	palette_set_color(0,0x00,0x00,0x00);
	palette_set_color(1,0x55,0x55,0x55);
	palette_set_color(2,0xaa,0xaa,0xaa);
	palette_set_color(3,0xff,0xff,0xff);
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

INTERRUPT_GEN( firetrk_interrupt )
{
	if (cpu_getiloops()==0)
	{
		/* NMI is triggered by VBLANK */
		if( readinputport(1)&0x80 )
			/* disable NMI while in test mode */
			;

		else
			cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
		cpu_set_irq_line(0, 0, HOLD_LINE); /* IRQ */
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ_HANDLER( firetrk_dsw_r )
{
	int dip = readinputport(0);
	int data = (dip&0x3)<<2; /* coinage */
	switch (offset)
	{
	case 0x00:  data |= (dip>>6)&0x03; break; /* Language */
	case 0x01:  data |= (dip>>4)&0x03; break; /* Game Time */
	case 0x02:  data |= (dip>>2)&0x03; break; /* Extended Play */
	}
	return data;
}


static READ_HANDLER( firetrk_io_r )
{
	data8_t data, bit0, bit6, bit7;
	int bit,steer_poll;

	bit0 = readinputport(1);
	bit6 = readinputport(2);
	bit7 = readinputport(3);
	bit = 1<<offset;

	steer_poll = ((INT8)(readinputport(4) - firetrk_steer1))/4;
	if( steer_poll )
	{
		if( steer_poll<0 ) bit0 |= 0x04;
		firetrk_bit7_flags &= ~0x04;
	}

	steer_poll = ((INT8)(readinputport(5) - firetrk_steer2))/4;
	if( steer_poll )
	{
		if( steer_poll<0 ) bit0 |= 0x08;
		firetrk_bit7_flags &= ~0x08;
	}

	bit0 |= firetrk_bit0_flags;
	bit7 |= firetrk_bit7_flags;

	data = 0x00;
	if( bit0 & bit ) data |= 0x01;
	if( bit6 & bit ) data |= 0x40;
	if( bit7 & bit ) data |= 0x80;
	return data;
}


static WRITE_HANDLER( steer_reset_w )
{
	firetrk_bit7_flags |= 0x0c;
	firetrk_steer1 = readinputport(4);
	firetrk_steer2 = readinputport(5);
}


static WRITE_HANDLER( crash_reset_w )
{
	firetrk_bit7_flags &= ~0x40;
}



/*************************************
 *
 *	Sound control
 *
 *************************************/

static WRITE_HANDLER( skid_reset_w )
{
	firetrk_bit0_flags &= ~0x40;
	// Clear skip sound output, Preset to D-Type, fed into NAND with crash noise
	discrete_sound_w(0x04,0x00);
}


static WRITE_HANDLER( firetrk_out_w )
{
	firetrk_invert_display = data&0x04;
	//		x-------	0x80 BELLOUT
	//		-x------	0x40 unused?
	//		--x-----	0x20 LED for START button
	//		---X----	0x10 ATTRACT (disables horn circuit)
	//		----x---	0x08 (LED for track select button?)
	//		-----x--	0x04 FLASH - inverts screen
	//		------x-	0x02 unused?
	//		-------x	0x01 LED for START button
	discrete_sound_w(0x06,!(data&0x10));	// Attract
	discrete_sound_w(0x05,(data&0x80)?0x00:0x01);	// Bell
}


static WRITE_HANDLER( firetrk_motorsnd_w )
{
	//		xxxx----	0xf0 Siren Frequency
	//		----xxxx	0x0f Motor Frequency
	discrete_sound_w(0x00,data&0x0f);
	discrete_sound_w(0x02,(data&0xf0)>>4);
}


static WRITE_HANDLER( firetrk_crashsnd_w )
{
	//		xxxx----	0xf0 Crash Volume
	discrete_sound_w(0x03,(data&0xf0)>>4);
}


static WRITE_HANDLER( firetrk_skidsnd_w )
{
	//		Write starts the skid sound, Clear input to D-Type, fed into NAND with crash noise
	discrete_sound_w(0x04,0x01);
}


static WRITE_HANDLER( firetrk_xtndply_w )
{
	//		-------x	0x01 Extend play sound
	discrete_sound_w(0x07,!(data&0x01));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( firetrk_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x0800, 0x08ff, MRA_RAM },
	{ 0x1800, 0x1807, firetrk_io_r },
	{ 0x1c00, 0x1c02, firetrk_dsw_r },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( firetrk_writemem )
	{ 0x0000, 0x01ff, MWA_RAM, &videoram },
	{ 0x0800, 0x08ff, MWA_RAM },				/* PRAM */
	{ 0x1000, 0x1000, MWA_RAM },				/* PVPLOAD */
	{ 0x1020, 0x1020, MWA_RAM },				/* PHPLOAD */
	{ 0x1048, 0x1048, crash_reset_w },			/* CRASHRESET */
	{ 0x1060, 0x1060, skid_reset_w },			/* SKIDRESET */
	{ 0x1080, 0x1080, MWA_RAM },				/* CABROT */
	{ 0x10a0, 0x10a0, steer_reset_w },			/* STEERRESET */
	{ 0x10c0, 0x10c0, MWA_RAM },				/* WATCHDOGRESET */
	{ 0x10e0, 0x10e0, MWA_RAM },				/* ARROWOFF */
	{ 0x1400, 0x1400, firetrk_motorsnd_w },	/* MOTORSND */
	{ 0x1420, 0x1420, firetrk_crashsnd_w }, 	/* CRASHSND */
	{ 0x1440, 0x1440, firetrk_skidsnd_w },	/* SKIDSND */
	{ 0x1460, 0x1460, MWA_RAM },				/* HPOS */
	{ 0x1480, 0x1480, MWA_RAM },				/* VPOS */
	{ 0x14a0, 0x14a0, MWA_RAM },				/* TAILROT */
	{ 0x14c0, 0x14c0, firetrk_out_w },		/* OUT */
	{ 0x14e0, 0x14e0, firetrk_xtndply_w },	/* XTNDPLY */
	{ 0x1800, 0x1807, MWA_RAM },				/* ? */
	{ 0x2000, 0x3fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( firetrk )
	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* Extended Play DIP affects required points to earn extended play.
	 * This interacts with the Game Time setting. */
	PORT_DIPNAME( 0x0c, 0x04, "Extended Play" )
	PORT_DIPSETTING(    0x00, "No Extended Play" )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x0c, "Hard" )
	PORT_DIPNAME( 0x30, 0x10, "Game Time")
	PORT_DIPSETTING(    0x00, "60 Seconds" )
	PORT_DIPSETTING(    0x10, "90 Seconds" )
	PORT_DIPSETTING(    0x20, "120 Seconds" )
	PORT_DIPSETTING(    0x30, "150 Seconds" )
	PORT_DIPNAME( 0xc0, 0x00, "Game Language")
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "French" )
	PORT_DIPSETTING(    0x80, "Spanish" )
	PORT_DIPSETTING(    0xc0, "German" )

	PORT_START /* bit 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_UNUSED )	/* SPARE */
	PORT_BITX(0x02, IP_ACTIVE_HIGH,	IPT_BUTTON1,"Gas",IP_KEY_DEFAULT,IP_JOY_DEFAULT )	/* GAS */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* STEER DIR1 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* STEER DIR2 */
	PORT_BITX(0x10, IP_ACTIVE_LOW,	IPT_BUTTON2,"Bell",IP_KEY_DEFAULT,IP_JOY_DEFAULT )	/* BELL */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,	IPT_TILT )		/* SLAM */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* SKIDIN */
	PORT_BITX(0x80, IP_ACTIVE_HIGH,	IPT_SERVICE|IPF_TOGGLE, DEF_STR(Service_Mode), KEYCODE_F2, IP_JOY_NONE )

	PORT_START /* bit 6 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,	IPT_START1 )	/* START1 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH,	IPT_START2 )	/* START2 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,	IPT_START3 )	/* START3 */
	PORT_BITX(0x08, IP_ACTIVE_HIGH,	IPT_BUTTON3, "Track Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT)
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_UNUSED )	/* SPARE */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_VBLANK )	/* VBLANK */
	PORT_DIPNAME( 0x40, 0x40, "Cabinet Type" )		/* CABINET */
	PORT_DIPSETTING(	0x00, "Single Player" )
	PORT_DIPSETTING(	0x40, "2 Players" )
	PORT_BITX(0x80, IP_ACTIVE_HIGH,	IPT_SERVICE, "Diag Hold", KEYCODE_F6, IP_JOY_NONE)

	PORT_START /* bit 7 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_UNUSED )	/* SPARE */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_UNUSED )	/* SPARE */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* STEER FLAG1 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* STEER FLAG2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_COIN1 )		/* COIN1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_COIN2 )		/* COIN1 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,	IPT_UNUSED )	/* CRASHIN */
	PORT_BITX(0x80, IP_ACTIVE_HIGH,	IPT_SERVICE, "Diag Step", KEYCODE_F1, IP_JOY_NONE)

	PORT_START /* p1 steering */
	PORT_ANALOG ( 0xff, 0x80, IPT_DIAL, 100, 0, 0 ,255 )

	PORT_START /* p2 steering */
	PORT_ANALOG ( 0xff, 0x80, IPT_DIAL | IPF_PLAYER2, 100, 0, 0, 255 )

	PORT_START /* Mechanical Horn switch */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON4,"Horn Button",IP_KEY_DEFAULT,IP_JOY_DEFAULT)
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout char_layout =
{
	16,16,	/* character size */
	0x20,	/* number of chars */
	1,		/* bits per pixel */
	{ 4 },
	{
		24,25,26,27,
		0,1,2,3,
		8,9,10,11,
		16,17,18,19
	},
	{
		0*64,0*64+32,1*64,1*64+32,
		2*64,2*64+32,3*64,3*64+32,
		4*64,4*64+32,5*64,5*64+32,
		6*64,6*64+32,7*64,7*64+32
	},
	8*64
};


static struct GfxLayout tile_layout =
{
	16,16,		/* character size */
	0x40,		/* number of characters */
	1,			/* bits per pixel */
	{ 0 },
	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	8*32
};


static struct GfxLayout tail_layout =
{
	64,64,	/* character size */
	0x8,	/* number of characters */
	1,		/* bits per pixel */
	{ 0 },
	{
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
		32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
		48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
	},
	{
		0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64,
		0x400+0*64, 0x400+1*64, 0x400+2*64, 0x400+3*64,
		0x400+4*64, 0x400+5*64, 0x400+6*64, 0x400+7*64,
		0x400+8*64, 0x400+9*64, 0x400+10*64, 0x400+11*64,
		0x400+12*64, 0x400+13*64, 0x400+14*64, 0x400+15*64,
		0x800+0*64, 0x800+1*64, 0x800+2*64, 0x800+3*64,
		0x800+4*64, 0x800+5*64, 0x800+6*64, 0x800+7*64,
		0x800+8*64, 0x800+9*64, 0x800+10*64, 0x800+11*64,
		0x800+12*64, 0x800+13*64, 0x800+14*64, 0x800+15*64,
		0xc00+0*64, 0xc00+1*64, 0xc00+2*64, 0xc00+3*64,
		0xc00+4*64, 0xc00+5*64, 0xc00+6*64, 0xc00+7*64,
		0xc00+8*64, 0xc00+9*64, 0xc00+10*64, 0xc00+11*64,
		0xc00+12*64, 0xc00+13*64, 0xc00+14*64, 0xc00+15*64
	},
	16*64*4
};


static struct GfxLayout cab_layout1 =
{
	32,32,	/* character size */
	0x4,	/* number of characters */
	1,		/* bits per pixel */
	{ 4 },
	{
		0,1,2,3,8,9,10,11,
		16,17,18,19,24,25,26,27,
		32,33,34,35,40,41,42,43,
		48,49,50,51,56,57,58,59
	},
	{
		0*64, 1*64, 2*64, 3*64,
		4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64,
		12*64, 13*64, 14*64, 15*64,
		0x400+0*64, 0x400+1*64, 0x400+2*64, 0x400+3*64,
		0x400+4*64, 0x400+5*64, 0x400+6*64, 0x400+7*64,
		0x400+8*64, 0x400+9*64, 0x400+10*64, 0x400+11*64,
		0x400+12*64, 0x400+13*64, 0x400+14*64, 0x400+15*64
	},
	16*64*2
};


static struct GfxLayout cab_layout2 =
{
	32,32,	/* character size */
	0x4,	/* number of characters */
	1,		/* bits per pixel */
	{ 4 },
	{
		0*64, 1*64, 2*64, 3*64,
		4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64,
		12*64, 13*64, 14*64, 15*64,
		0x400+0*64, 0x400+1*64, 0x400+2*64, 0x400+3*64,
		0x400+4*64, 0x400+5*64, 0x400+6*64, 0x400+7*64,
		0x400+8*64, 0x400+9*64, 0x400+10*64, 0x400+11*64,
		0x400+12*64, 0x400+13*64, 0x400+14*64, 0x400+15*64
	},
	{
		0,1,2,3,8,9,10,11,
		16,17,18,19,24,25,26,27,
		32,33,34,35,40,41,42,43,
		48,49,50,51,56,57,58,59
	},
	16*64*2
};


static struct GfxDecodeInfo firetrk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,	0, 0x8 },
	{ REGION_GFX2, 0, &tile_layout,	0, 0x8 },
	{ REGION_GFX3, 0, &tail_layout,	0, 0x8 },
	{ REGION_GFX4, 0, &cab_layout1,	0, 0x8 },
	{ REGION_GFX4, 0, &cab_layout2,	0, 0x8 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

/************************************************************************/
/* firetrk Sound System Analog emulation by K.Wilkins Feb 2001        */
/* Questions/Suggestions to mame@dysfunction.demon.co.uk                */
/************************************************************************/

#define	FIRETRUCK_MOTORSND		NODE_01
#define	FIRETRUCK_HORNSND		NODE_02
#define	FIRETRUCK_SIRENSND		NODE_03
#define	FIRETRUCK_CRASHSND		NODE_04
#define	FIRETRUCK_SKIDSND		NODE_05
#define	FIRETRUCK_BELLSND		NODE_06
#define	FIRETRUCK_ATTRACT		NODE_07
#define	FIRETRUCK_XTNDPLY		NODE_08

static DISCRETE_SOUND_START(firetrk_sound_interface)
	/************************************************/
	/* Firetruck sound system: 7 Sound Sources      */
	/*                     Relative Volume          */
	/*    1) Horn (Button)     10.02%               */
	/*    2) Motor              8.17%               */
	/*    3) Siren              1.47%               */
	/*    4) Crash             22.05%               */
	/*    5) Screech/Skid       6.68%               */
	/*    6) Bell               4.69%               */
	/*    7) Xtnd              46.91%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  FireTruck Discrete sound mapping via:       */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Motorsound frequency                  */
	/*  $01 - Hornsound enable                      */
	/*  $02 - Siren frequency                       */
	/*  $03 - Crash volume                          */
	/*  $04 - Skid enable                           */
	/*  $05 - Bell enable                           */
	/*  $06 - Attract mode                          */
	/*  $07 - Extend sound                          */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for firetruck         */
	/************************************************/
	/*                   NODE             ADDR   MASK   GAIN    OFFSET  INIT */
	DISCRETE_INPUTX(FIRETRUCK_MOTORSND    ,0x00,0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUT (FIRETRUCK_HORNSND     ,0x01,0x000f,0)
	DISCRETE_INPUTX(FIRETRUCK_SIRENSND    ,0x02,0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUTX(FIRETRUCK_CRASHSND    ,0x03,0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUT (FIRETRUCK_SKIDSND     ,0x04,0x000f,0)
	DISCRETE_INPUT (FIRETRUCK_BELLSND     ,0x05,0x000f,0)
	DISCRETE_INPUT (FIRETRUCK_ATTRACT     ,0x06,0x000f,0)
	DISCRETE_INPUT (FIRETRUCK_XTNDPLY     ,0x07,0x000f,0)


	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div2 & div12*/
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 370-454Hz            */
	/*  F1 freq = 185Hz - 227Hz (Div2)              */
	/*  F2 freq = 30Hz - 38Hz   (Div12)             */
	/* Motorsnd => 0000=454Hz  1111=270Hz           */
	/* (Input register does the inversion of sign)  */
	/************************************************/
	DISCRETE_GAIN(NODE_18,FIRETRUCK_MOTORSND,(227.0-185.0)/16.0)	// F1
	DISCRETE_ADDER2(NODE_17,1,NODE_18,185.0)
	DISCRETE_SQUAREWAVE(NODE_16,1,NODE_17,(817.0/2.0),50.0,0)

	DISCRETE_GAIN(NODE_14,FIRETRUCK_MOTORSND,(38.0-30.0)/16.0)		// F2
	DISCRETE_ADDER2(NODE_13,1,NODE_14,30.0)
	DISCRETE_SQUAREWAVE(NODE_12,1,NODE_13,(817.0/2.0),50.0,0)

	DISCRETE_ADDER2(NODE_10,FIRETRUCK_ATTRACT,NODE_12,NODE_16)
//	DISCRETE_RCFILTER(NODE_10,1,NODE_11,5000,0.1e-6)

	/************************************************/
	/* Horn, this is taken from the 64V signal that */
	/* is a 750Hz sqruare wave. 1H = 3MHz with 32H  */
	/* used to clock the V counter with 64V=3M/32/64*/
	/************************************************/
	DISCRETE_SQUAREWAVE(NODE_29,FIRETRUCK_ATTRACT,750.0,1002.0,50.0,0)
	DISCRETE_ONOFF(NODE_20,FIRETRUCK_HORNSND,NODE_29)

	/************************************************/
	/* Siren is built around a 556 based VCO, the   */
	/* 4 bit input value is smoothed between trans- */
	/* itions by a 10u capacitor with around a 0.5s */
	/* time constant, modelled with an RC filter.   */
	/* 0000 = 666Hz with 35% duty cycle             */
	/* 1111 = 526Hz with 63% duty cycle             */
	/* Input register does the inversion of sense   */
	/* to map this to:                              */
	/* 0000 = 526Hz with 37% duty cycle             */
	/* 1111 = 666Hz with 65% duty cycle             */
	/* Duty cycle is inverted 100-x to make things  */
	/* a little simpler and it doesnt affect sound. */
	/************************************************/
	DISCRETE_RCFILTER(NODE_38,1,FIRETRUCK_SIRENSND,250000,1e-6)		/* Input smoothing */

	DISCRETE_GAIN(NODE_36,NODE_38,(666.0-526.0)/16.0)				/* Frequency modelling */
	DISCRETE_ADDER2(NODE_35,1,NODE_36,526.0)

	DISCRETE_GAIN(NODE_34,NODE_38,(65.0-37.0)/16.0)					/* Duty Cycle modelling */
	DISCRETE_ADDER2(NODE_33,1,NODE_34,37.0)

	DISCRETE_SQUAREWAVE(NODE_30,FIRETRUCK_ATTRACT,NODE_35,147.0,NODE_33,0)	/* VCO */


	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 1H = 3MHz and 1V = 32H /2 = 3MHz/32/2/2      */
	/*               2V = 3MHz/32/4 = 23483Hz       */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/* Volume is inverted by input register mapping */
	/************************************************/
	DISCRETE_NOISE(NODE_49,FIRETRUCK_ATTRACT,23483.0,FIRETRUCK_CRASHSND)
	DISCRETE_GAIN(NODE_40,NODE_49,2205.0/15.0)

	/************************************************/
	/* Skid circuit takes the noise output from     */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 2.2K & 2.2uF                           */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO, estimated   */
	/* freq range as 1Khz to 6Khz (sounds OK)       */
	/************************************************/
	DISCRETE_NOISE(NODE_59,1,23483.0,5000.0)
	DISCRETE_RCFILTER(NODE_58,1,NODE_59,2200,2.2e-6)
	DISCRETE_ADDER2(NODE_57,1,NODE_58,1500.0)
	DISCRETE_SQUAREWAVE(NODE_50,FIRETRUCK_SKIDSND,NODE_57,668.0,50.0,0.0)


	/************************************************/
	/* Bell circuit - The bellsound signal is a one */
	/* shot signal, a low pulse enables the bell    */
	/* sound for 600ms with a 6Khz tone, the 600ms  */
	/* decays with an RC TC of around 100ms.        */
	/* The Hsync signal is put into a div 4 counter */
	/* to get the 6KHz.                             */
	/************************************************/
	DISCRETE_ONESHOTR(NODE_69,1,FIRETRUCK_BELLSND,1,0.6)

	DISCRETE_ADDER2(NODE_68,1,FIRETRUCK_BELLSND,-1.0)		// Invert sense of bellsnd for RC decay
	DISCRETE_GAIN(NODE_67,NODE_68,-469.0)
	DISCRETE_RCFILTER(NODE_66,1,NODE_67,10000,1.0e-6)

	DISCRETE_SQUAREWAVE(NODE_60,NODE_69,5870.0,NODE_66,50.0,0.0)


	/************************************************/
	/* Extended play circuit is just the 8V signal  */
	/* gated with regestered output from a D-Type   */
	/* mand mapped into memory. 5870Hz              */
	/************************************************/
	DISCRETE_SQUAREWAVE(NODE_70,FIRETRUCK_XTNDPLY,5870.0,4691.0,50.0,0.0)

	/************************************************/
	/* Combine all 7 sound sources with a double    */
	/* adder circuit                                */
	/* (Note each cct contributes 1000 to the total */
	/* and this needs to be scaled to give the      */
	/* overall level to 65535 i.e *(65535/7000)     */
	/************************************************/
	DISCRETE_ADDER4(NODE_91,1,NODE_10,NODE_20,NODE_30,NODE_40)
	DISCRETE_ADDER4(NODE_92,1,NODE_50,NODE_60,NODE_70,NODE_91)
	DISCRETE_GAIN(NODE_90,NODE_92,(65535.0/10000.0))

	DISCRETE_OUTPUT(NODE_90)														// Take the output from the mixer
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( firetrk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6808,1000000)		/* 1MHz during normal operation */
									/* 750Khz during self-test sequence */
	MDRV_CPU_MEMORY(firetrk_readmem,firetrk_writemem)
	MDRV_CPU_VBLANK_INT(firetrk_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(firetrk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source)/2)

	MDRV_PALETTE_INIT(firetrk)
	MDRV_VIDEO_START(firetrk)
	MDRV_VIDEO_UPDATE(firetrk)

	/* sound hardware */
	MDRV_SOUND_ADD(DISCRETE, firetrk_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( firetrk )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )
	ROM_LOAD( "32823-02.c1", 0x2000, 0x800, 0x9570bdd3 )	/* ROM0 */
	ROM_LOAD( "32824-01.d1", 0x2800, 0x800, 0xa5fc5629 )	/* ROM1 */
	ROM_LOAD( "32816-01.k1", 0x10000, 0x800, 0xc0535598 )	/* ROM2a */
	ROM_LOAD( "32815-01.j1", 0x10800, 0x800, 0x506ee759 )	/* ROM3a */
	ROM_LOAD( "32820-01.k2", 0x11000, 0x800, 0x5733f9ed )	/* ROM2b */
	ROM_LOAD( "32819-01.j2", 0x11800, 0x800, 0xf1c3fa87 )	/* ROM3b */

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "32827-01.r3", 0x000, 0x800, 0xcca31d2b ) /* characters */

	ROM_REGION( 0x800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "32828-02.f5", 0x000, 0x800, 0x68ef5f19 ) /* tiles */

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "32829-01.j5", 0x000, 0x800, 0xe7267d71 ) /* Trailer */
	ROM_LOAD( "32830-01.l5", 0x800, 0x800, 0xe4d8b685 )

	ROM_REGION( 0x400, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "32831-01.p7", 0x000, 0x400, 0xbb8d144f ) /* Cab */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "9114-01.bin", 0x0000, 0x0100, 0xb8094b4c )	/* sync (not used) */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( firetrk )
{
	/* combine the 4 bit program ROMs */
	UINT8 *pMem = memory_region( REGION_CPU1 );
	int i;
	for( i=0; i<0x1000; i++ )
	{
		int msn = pMem[0x10000+i];
		int lsn = pMem[0x10000+i+0x1000];
		int data = (msn<<4)|lsn;
		pMem[i+0x3000] = data;
		pMem[i+0xf000] = data;
	}
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1978, firetrk, 0,        firetrk,	firetrk,	firetrk,	ROT270, "Atari", "Fire Truck" )
