/*
MAME driver for various SNK 3xZ80 games.
Preliminary!

Video Hardware #0 - placeholder only
Marvin's Maze

Video Hardware #1 - none of these are playable
Aso (US/JP), Athena, TNK3
  512 chars			(4 bitplanes)
 1024 tiles			(4 bitplanes)
 1024 16x16 sprites	(3 bitplanes)

Video Hardware #2 - these are working
Ikari Warrior (US/JP), Victory Road/Dogou Souken
  512 chars			(4 bitplanes)
 1024 tiles			(4 bitplanes)
 1024 16x16 sprites	(3 bitplanes)
  512 32x32 sprites	(3 bitplanes)

Video Hardware #3 - Guerilla War, Touchdown Fever(J), and Psycho Soldier(J) are working
TouchDown Fever (US/JP)
 1024 chars			(4 bitplanes)
 5*512 tiles		(4 bitplanes)
 1024 32x32 sprites	(4 bitplanes)
Guerrilla War, Bermude Triangle, Psycho Soldier, Chopper I/The Legend of Air Cavalry
 1024 chars			(4 bitplanes)
 2048 tiles			(4 bitplanes)
 1024 16x16 sprites	(4 bitplanes) - Guerilla War has 2048 16x16 sprites
 1024 32x32 sprites (4 bitplanes)

other possible SNK games for this driver:
	HAL21
	S.A.R (Search and Rescue)

Notes:
Chopper I sound uses YM3812 and YM8950
Touchdown Fever sound uses YM3812 and YM8950

Interrupt Notes:
	CPUA -(IW-JP)
	INT 38 - #0490  - Update Screen Related (1 every frame)
	INT 40 - #3025 (Read, c100, c200)? (not used!?)
	INT 66 - (Write c700, start SP)? (not used!?)

	CPUB -(IW-JP)
	read 0xc000?
	INT 38 - #00B6 (1 every frame)
	INT 66 - #00E3 (Write C000) (by CPUA write/read in c700)

***************************************************************************/

#define CREDITS	"Ernesto Corvi\n" \
				"Carlos A. Lozano\n" \
				"Jarek Parchanski\n" \
				"Phil Stroffolino\n" \
				"Victor Trucco\n"

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


#define MEM_COLOR			3
#define MEM_GFX_CHARS		4
#define MEM_GFX_TILES		5
#define MEM_GFX_SPRITES		6
#define MEM_GFX_BIGSPRITES	7
#define MEM_SAMPLES			8

extern void snk_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void ikari_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

extern int snk_vh_start( void );
extern void snk_vh_stop( void );

extern void tnk3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void ikari_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void tdfever_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void gwar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void psychos_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/**************************************************************/

static int hard_flags = 0;

#define DIAL_NONE		0
#define DIAL_BOOTLEG	1
#define DIAL_NORMAL		2

static int dial_type = DIAL_NORMAL;

static int snk_soundcommand=0; /* can we just use soundlatch_r/w? */

static int snk_read_joy( int which ){
	int result = readinputport(which+1);

	if( dial_type==DIAL_BOOTLEG ){
		static int dial_8[8]   = { 0xF0,0x30,0x10,0x50,0x40,0xC0,0x80,0xA0 };
		result |= dial_8[readinputport(which+7) * 8 / 256];
	}
	else if( dial_type==DIAL_NORMAL ){
		static int dial_12[12] = { 0xB0,0xA0,0x90,0x80,0x70,0x60,0x50,0x40,0x30,0x20,0x10,0x00 };
		result |= dial_12[readinputport(which+7) * 12 / 256];
	}

	return result;
}

static int snk_soundcommand_r(int offset){
	int val = snk_soundcommand;
	snk_soundcommand = 0;
	return val;
}

static unsigned char *shared_ram, *hardware_ram;

static int shared_ram_r( int offset ){
	return shared_ram[offset];
}
static void shared_ram_w( int offset, int data ){
	shared_ram[offset] = data;
}

static int hardware_r( int offset ){
	switch( offset ){
		case 0x000: return input_port_0_r( 0 );
		case 0x100: return snk_read_joy( 0 );
		case 0x200: return snk_read_joy( 1 );
		case 0x300: return input_port_3_r( 0 );
		case 0x400: return input_port_4_r( 0 );
		case 0x500: return input_port_5_r( 0 );
		case 0x600: return input_port_6_r( 0 );

		case 0x700:
		//if( errorlog ) fprintf( errorlog, "CPU1: c700r\n" );
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		return 0xff;

		/* "Hard Flags" */
		case 0xe00:
		case 0xe20:
		case 0xe40:
		case 0xe60:
		case 0xe80:
		case 0xea0:
		case 0xee0: if( hard_flags ) return 0xff;
	}
	return hardware_ram[offset];
}

static void hardware_w( int offset, int data ){
	switch( offset ){
		case 0x000:
		if( errorlog ) fprintf( errorlog, "c000 write\n");
		break;

		case 0x400:
		hardware_ram[offset] = snk_soundcommand = data;
		break;

		case 0x700:
		if( errorlog ) fprintf( errorlog, "c700w\n" );
		cpu_cause_interrupt( 1, Z80_NMI_INT );
		break;

		default:
		hardware_ram[offset] = data;
		break;
	}
}

static int hardware2_r( int offset ){
	switch( offset ){
		case 0x000: return input_port_0_r( 0 );
		case 0x100: return snk_read_joy( 0 );
		case 0x200: return snk_read_joy( 1 );
		case 0x300: return input_port_3_r( 0 );
		case 0x400: return input_port_4_r( 0 );
		case 0x500: return input_port_5_r( 0 );
		case 0x600: return 0xff;

		case 0x700:
		//if( errorlog ) fprintf( errorlog, "CPU1: c700r\n" );
//		cpu_cause_interrupt( 1, Z80_NMI_INT );
		return 0xff;

		/* "Hard Flags" */
		case 0xe00:
		case 0xe20:
		case 0xe40:
		case 0xe60:
		case 0xe80:
		case 0xea0:
		case 0xee0: if( hard_flags ) return 0xff;
	}
	return hardware_ram[offset];
}

static void hardware2_w( int offset, int data ){
	switch( offset ){
		case 0x000:
		if( errorlog ) fprintf( errorlog, "c000 write\n");
		break;

		case 0x400:
		hardware_ram[offset] = snk_soundcommand = data;
		break;

		case 0x700:
		if( errorlog ) fprintf( errorlog, "c700w\n" );
//		cpu_cause_interrupt( 1, Z80_NMI_INT );
		break;

		default:
		hardware_ram[offset] = data;
		break;
	}
}


/*********************************************************************/

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,       /* 8000Hz playback */
	MEM_SAMPLES,
	0,			/* init function */
	{ 255 }			/* volume? */
};

static struct YM3526interface ym3526_interface = {
	1,			/* needs 2, but only 1 supported */
	4000000,	/* 4 MHz? (hand tuned) */
	{ 255 }		/* (not supported) */
};

/***********************************************************************/

static void psychos_decode( void ){
	dial_type = DIAL_NONE;
	hard_flags = 0;
}

void snk_adpcm_play( int which );
void snk_adpcm_play( int which ){
	ADPCM_play( 0, 0x10000*which, 0x10000 );
}

static struct MemoryReadAddress tnk3_readmem[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, hardware_r, &hardware_ram },
	{ 0xd000, 0xffff, MRA_RAM, &shared_ram },
	{ -1 }
};

static struct MemoryWriteAddress tnk3_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, hardware_w },
	{ 0xd000, 0xffff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress tnk3_readmem_sub[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, hardware_r },
	{ 0xf800, 0xffff, MRA_RAM }, /* don't share this region */
	{ 0xd000, 0xffff, shared_ram_r },
	{ -1 }
};

static struct MemoryWriteAddress tnk3_writemem_sub[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, hardware_w },
	{ 0xf800, 0xffff, MWA_RAM }, /* don't share this region */
	{ 0xd000, 0xffff, shared_ram_w },
	{ -1 }
};

static struct MemoryReadAddress ikari_readmem[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, hardware_r, &hardware_ram },
	{ 0xd000, 0xffff, MRA_RAM, &shared_ram },
	{ -1 }
};
static struct MemoryWriteAddress ikari_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, hardware_w },
	{ 0xd000, 0xffff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress ikari_readmem_sub[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, hardware2_r },
	{ 0xd000, 0xffff, shared_ram_r },
	{ -1 }
};
static struct MemoryWriteAddress ikari_writemem_sub[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, hardware2_w },
	{ 0xd000, 0xffff, shared_ram_w },
	{ -1 }
};

static struct MemoryReadAddress readmem_sound[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundcommand_r },
	{ 0xe800, 0xe800, YM3526_status_port_0_r },
	{ 0xf000, 0xf000, MRA_NOP }, /* YM3526 #2 status port */
	{ 0xf800, 0xf800, MRA_RAM }, /* IRQ CTRL FOR YM8950  NOT EMULATED */
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP }, /* YM3526 #2 control port */
	{ 0xf400, 0xf400, MWA_NOP }, /* YM3526 #2 write port   */
	{ 0xe800, 0xe800, YM3526_control_port_0_w },
	{ 0xec00, 0xec00, YM3526_write_port_0_w },
	{ 0xf800, 0xf800, MWA_RAM }, /* wrong */
	{ -1 }
};

/***********************************************************************/

INPUT_PORTS_START( ikarius_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No")
	PORT_DIPSETTING(    0x00, "Yes")

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* player 1 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END

INPUT_PORTS_START( ikarijp_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,IPT_UNKNOWN ) 	/* sound related ? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  	/* tilt related ? (it resets 1 cpu) */

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_DIPSETTING(    0x00, "Infinite Lives" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* player 1 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END

INPUT_PORTS_START( victroad_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_DIPSETTING(	0x00, "Infinite Lives" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "On")
	PORT_DIPSETTING(    0x00, "Off")

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* player 1 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END

INPUT_PORTS_START( gwar_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* causes reset */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* dial CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	PORT_DIPNAME( 0x01, 0x01, "Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Coin Up" )
	PORT_DIPSETTING(    0x00, "Standard" )
	PORT_DIPNAME( 0x02, 0x02, "Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Upside Down" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Only" )
	PORT_DIPSETTING(    0x00, "Every" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Monitor", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Freeze Game" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 50000" )
	PORT_DIPSETTING(    0x20, "40000 80000" )
	PORT_DIPSETTING(    0x10, "50000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* player 1 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 rotary control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END

INPUT_PORTS_START( athena_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* following dips are unverified - copied from Psycho Soldier */
	PORT_START  /* DIPSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Test Mode", IP_KEY_NONE )   /* Test Mode */
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPNAME( 0x02, 0x02, "Video Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Extra man at every bonus" )
	PORT_DIPSETTING(    0x04, "1st & 2nd bonus only" )
	PORT_DIPNAME( 0x08, 0x08, "Number of lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x30, 0x30, "Coin/Credit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xC0, 0xC0, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0xc0, "Off" )

	PORT_START  /* DIPSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPNAME( 0x04, 0x04, "Sound Demo", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Freeze Video", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus points", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x10, "100K 200K" )
	PORT_DIPSETTING(    0x20, "60K 120K" )
	PORT_DIPSETTING(    0x30, "50K 100K" )
	PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )
INPUT_PORTS_END

INPUT_PORTS_START( tdfever_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )   /* Single Coin     */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )  /* Start 1 player  */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )  /* Start 2 players */

    PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )

    PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )
INPUT_PORTS_END

INPUT_PORTS_START( tnk3_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )   /* Multiple Coin   */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )   /* Single Coin     */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )  /* Start 1 player  */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )  /* Start 2 players */		   /*Player 2*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )   /* Single Coin     */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	/*DIAL P1*/
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	/*DIAL P2*/
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown DSW1 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown DSW1 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sound" )
	PORT_DIPSETTING(    0x04, "Stop Tv" )
	PORT_DIPSETTING(    0x00, "Never Finish" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50K / 100K" )
	PORT_DIPSETTING(    0x20, "60K / 120K" )
	PORT_DIPSETTING(    0x10, "100K / 200K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown DSW2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown DSW2 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( psychos_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Reset */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START  /* DIPSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Test Mode", IP_KEY_NONE )   /* Test Mode */
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPNAME( 0x02, 0x02, "Video Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Extra man at every bonus" )
	PORT_DIPSETTING(    0x04, "1st & 2nd bonus only" )
	PORT_DIPNAME( 0x08, 0x08, "Number of lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x30, 0x30, "Coin/Credit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xC0, 0xC0, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0xc0, "Off" )

	PORT_START  /* DIPSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPNAME( 0x04, 0x04, "Sound Demo", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Freeze Video", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus points", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x10, "100K 200K" )
	PORT_DIPSETTING(    0x20, "60K 120K" )
	PORT_DIPSETTING(    0x30, "50K 100K" )
	PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

/*********************************************************************/

static struct GfxLayout char512 = {
	8,8,
	512,
	4,
	{ 0, 1, 2, 3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	256
};

static struct GfxLayout char1024 = {
	8,8,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	256
};

static struct GfxLayout tile1024 = {
	16,16,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	128*8
};

static struct GfxLayout tile2048 = {
	16,16,
	2048,
	4,
	{ 0, 1, 2, 3 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	128*8
};

static struct GfxLayout tdfever_tiles = {
	16,16,
	512*5,
	4,
	{ 0, 1, 2, 3 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	128*8
};

static struct GfxLayout sprite1024 = {
	16,16,
	1024,
	3,
	{ 2*1024*256, 1*1024*256, 0*1024*256 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 },
	256
};

static struct GfxLayout gwar_sprite2048 = {
	16,16,
	2048,
	4,
	{ 3*2048*256, 2*2048*256, 1*2048*256, 0*2048*256  },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 },
	256
};


static struct GfxLayout big_sprite512 = {
	32,32,
	512,
	3,
	{ 2*2048*256, 1*2048*256, 0*2048*256 },
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8,
		23,22,21,20,19,18,17,16,
		31,30,29,28,27,26,25,24
	},
	16*32*2
};

static struct GfxLayout gwar_big_sprite1024 = {
	32,32,
	1024,
	4,
	{ 0*1024*1024, 1*1024*1024, 2*1024*1024, 3*1024*1024 },
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8,
		23,22,21,20,19,18,17,16,
		31,30,29,28,27,26,25,24
	},
	1024
};

static struct GfxLayout gwar_big_sprite1024_a = {
	32,32,
	1024,
	4,
	{ 3*1024*1024, 2*1024*1024, 1*1024*1024, 0*1024*1024 },
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8,
		23,22,21,20,19,18,17,16,
		31,30,29,28,27,26,25,24
	},
	1024
};


/*********************************************************************/

static struct GfxDecodeInfo tnk3_gfxdecodeinfo[] = {
	{ MEM_GFX_CHARS,      0x0, &char512,             256, 16 },
	{ MEM_GFX_TILES,      0x0, &char1024,            256, 16 },
	{ MEM_GFX_SPRITES,    0x0, &sprite1024,            0, 16 },
	{ -1 }
};

static struct GfxDecodeInfo ikari_gfxdecodeinfo[] = {
	{ MEM_GFX_CHARS,      0x0, &char512,             256, 16 },
	{ MEM_GFX_TILES,      0x0, &tile1024,            256, 16 },
	{ MEM_GFX_SPRITES,    0x0, &sprite1024,            0, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &big_sprite512,       128, 16 },
	{ -1 }
};

static struct GfxDecodeInfo gwar_gfxdecodeinfo[] = {
	{ MEM_GFX_CHARS,      0x0, &char1024,             256*0, 16 },
	{ MEM_GFX_TILES,      0x0, &tile2048,             256*3, 16 },
	{ MEM_GFX_SPRITES,    0x0, &gwar_sprite2048,      256*1, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &gwar_big_sprite1024,  256*2, 16 },
	{ -1 }
};

static struct GfxDecodeInfo tdfever_gfxdecodeinfo[] = {
	{ MEM_GFX_CHARS,      0x0, &char1024,            	0, 16 },
	{ MEM_GFX_TILES,      0x0, &tdfever_tiles,        512, 16 },
	{ MEM_GFX_SPRITES,    0x0, &gwar_big_sprite1024,  256, 16 },
	{ -1 }
};

/**********************************************************************/

static struct MachineDriver tnk3_machine_driver = {
	{
		{
			CPU_Z80,
			5500000,
			0,
			tnk3_readmem,tnk3_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			5500000,
			1,
			tnk3_readmem_sub,tnk3_writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	256, 36*8, { 16, 256-1-16, 0*8, 36*8-1 },

	tnk3_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tnk3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    },
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver ikari_machine_driver = {
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			ikari_readmem,ikari_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			1,
			ikari_readmem_sub,ikari_writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	256, 36*8, { 16, 256-1-16, 0*8, 36*8-1 },
	ikari_gfxdecodeinfo,
	1024,1024,
	ikari_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	ikari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    },
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};


static struct MachineDriver gwar_machine_driver = {
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			tnk3_readmem,tnk3_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			1,
			tnk3_readmem_sub,tnk3_writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	240, 384, { 0, 239-16, 0, 383 },
	gwar_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	gwar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    },
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver psychos_machine_driver = {
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			ikari_readmem,ikari_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			1,
			ikari_readmem_sub,ikari_writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	240, 384, { 0, 239-16, 0, 383 },
	gwar_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,// | VIDEO_SUPPORTS_16BIT,
	0,
	snk_vh_start,
	snk_vh_stop,
	psychos_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    },
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver tdfever_machine_driver = {
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			ikari_readmem,ikari_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			1,
			ikari_readmem_sub,ikari_writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init_machine */

	/* video hardware */
	240, 384, { 0, 239-16, 0, 383 },
	tdfever_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tdfever_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	       SOUND_YM3526,
	       &ym3526_interface
	    },
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

#define OFFS 8

static struct GfxLayout charx = {
	8,8,
	0x2000/(8*OFFS),
	1,
	{ 0 },
	{ 0*OFFS,1*OFFS,2*OFFS,3*OFFS,4*OFFS,5*OFFS,6*OFFS,7*OFFS },
	{ 0*8*OFFS,1*8*OFFS,2*8*OFFS,3*8*OFFS,4*8*OFFS,5*8*OFFS,6*8*OFFS,7*8*OFFS },
	64*OFFS
};

static struct GfxDecodeInfo marvins_gfxdecodeinfo[] = {
	{ MEM_GFX_CHARS,      0x02000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x04000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x06000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x08000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x0a000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x0c000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x0e000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x10000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x12000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x14000, &charx,            	0, 16 },
	{ MEM_GFX_CHARS,      0x16000, &charx,            	0, 16 },
	{ -1 }
};

static struct MachineDriver marvins_machine_driver = {
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			ikari_readmem,ikari_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			1,
			ikari_readmem_sub,ikari_writemem_sub,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init_machine */

	/* video hardware */
	240, 384, { 0, 239-16, 0, 383 },
	marvins_gfxdecodeinfo,
	1024,1024,
	0,//snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tnk3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};

/***********************************************************************/
/***********************************************************************/

ROM_START( marvins_rom )
/* three z80s with two AY-3-8910 */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_REGION(0x10000)	/* 64k for sound code */

	ROM_REGION(0x18000)	/* gfx */
	ROM_LOAD( "b1",    0x00000, 0x2000, 0xe528bc60 )
	ROM_LOAD( "b2",    0x02000, 0x2000, 0xe528bc60 )
	ROM_LOAD( "f1",    0x04000, 0x2000, 0x0bd6b4e5 )
	ROM_LOAD( "f2",    0x06000, 0x2000, 0x8fc2b081 )
	ROM_LOAD( "f3",    0x08000, 0x2000, 0xe55c9b83 )
	ROM_LOAD( "m1",    0x0a000, 0x2000, 0x2314c696 )//s
	ROM_LOAD( "m2",    0x0c000, 0x2000, 0x74ba5799 )//s
	ROM_LOAD( "pa1",   0x0e000, 0x2000, 0x0008d791 )//s
	ROM_LOAD( "pa2",   0x10000, 0x2000, 0x9457003c )//s
	ROM_LOAD( "pa3",   0x12000, 0x2000, 0x54c33ecb )//s
	ROM_LOAD( "pb1",   0x14000, 0x2000, 0x3b6941a5 )//s
	ROM_LOAD( "s1",    0x16000, 0x2000, 0x327f70f3 )
ROM_END

/***********************************************************************/

ROM_START( aso_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "aso.1",    0x0000, 0x8000, 0x3fc9d5e4 )
	ROM_LOAD( "aso.3",    0x8000, 0x4000, 0x39a666d2 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "aso.4",    0x0000, 0x8000, 0x2429792b )
	ROM_LOAD( "aso.6",    0x8000, 0x4000, 0xc0bfdf1f )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "aso.7",    0x0000, 0x8000, 0x49258162 )  /* YM3526 */
	ROM_LOAD( "aso.9",    0x8000, 0x4000, 0xaef5a4f4 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up02_f12.rom",  0x00000, 0x00400, 0x5b0a0059 )
	ROM_LOAD( "up02_f13.rom",  0x00400, 0x00400, 0x37e28dd8 )
	ROM_LOAD( "up02_f14.rom",  0x00800, 0x00400, 0xc3fd1dd3 )

	ROM_REGION_DISPOSE(0x4000) /* characters */
	ROM_LOAD( "aso.14",   0x0000, 0x2000, 0x8baa2253 )
	ROM_LOAD( "aso.14",   0x2000, 0x2000, 0x8baa2253 )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "aso.10",  0x0000, 0x8000, 0x00dff996 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "aso.11",  0x00000, 0x8000, 0x7feac86c )
	ROM_LOAD( "aso.12",  0x08000, 0x8000, 0x6895990b )
	ROM_LOAD( "aso.13",  0x10000, 0x8000, 0x87a81ce1 )
ROM_END

ROM_START( asoj_rom ) /* unconfirmed! */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "up02_d8.rom",    0x0000, 0x4000, 0x6df46d74 )
	ROM_LOAD( "up02_d7.rom",    0x4000, 0x4000, 0x9cfebad6 )
	ROM_LOAD( "up02_d5.rom",    0x8000, 0x4000, 0xc79702e1 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "up02_d3.rom",    0x0000, 0x4000, 0xedb2157a )
	ROM_LOAD( "up02_d2.rom",    0x4000, 0x4000, 0x41ea628a )
	ROM_LOAD( "up02_d1.rom",    0x8000, 0x4000, 0xfa747816 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "up02_f4.rom",    0x0000, 0x4000, 0x4b4505ab ) /* YM3526 */
	ROM_LOAD( "up02_f3.rom",    0x4000, 0x4000, 0xed065d56 )
	ROM_LOAD( "up02_f2.rom",    0x8000, 0x4000, 0xc0d74405 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up02_f12.rom",  0x00000, 0x00400, 0xf73e0002 )
	ROM_LOAD( "up02_f13.rom",  0x00400, 0x00400, 0x983c0000 )
	ROM_LOAD( "up02_f14.rom",  0x00800, 0x00400, 0x647f0101 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "up01_h1.rom",   0x0000, 0x4000, 0x6c340000 )

	ROM_REGION_DISPOSE( 0x4000 ) /* background tiles */
	ROM_LOAD( "up01_h14.rom",  0x0000, 0x4000, 0xc6576a2d )

	ROM_REGION_DISPOSE( 0xc000 ) /* 16x16 sprites */
	ROM_LOAD( "up01_h8.rom",  0x0000, 0x4000, 0xe98604a6 )
	ROM_LOAD( "up01_h9.rom",  0x4000, 0x4000, 0xf315aee5 )
	ROM_LOAD( "up01_h10.rom", 0x8000, 0x4000, 0x4ea302df )
ROM_END

ROM_START( athena_rom )
	ROM_REGION( 0x10000 ) /* 64k for cpuA code */
	ROM_LOAD( "up02_p4.rom",  0x0000, 0x4000,  0x900a113c )
	ROM_LOAD( "up02_m4.rom",  0x4000, 0x8000,  0x61c69474 )

	ROM_REGION( 0x10000 ) /* 64k for cpuB code */
	ROM_LOAD( "up02_p8.rom",  0x0000, 0x4000, 0xdf50af7e )
	ROM_LOAD( "up02_m8.rom",  0x4000, 0x8000, 0xf3c933df )

	ROM_REGION( 0x10000 ) /* 64k for sound code */
	ROM_LOAD( "up02_g6.rom",  0x0000, 0x4000, 0xe890ce09 )
	ROM_CONTINUE( 0x00000, 0x4000 )  /* skip 0x4000 bytes */
	ROM_LOAD( "up02_k6.rom",  0x4000, 0x8000, 0x596f1c8a )

	ROM_REGION( 0xC00 )	/* color proms */
	ROM_LOAD( "up02_b1.rom",  0x000, 0x400, 0xd25c9099 )
	ROM_LOAD( "up02_c2.rom",  0x400, 0x400, 0x294279ae )
	ROM_LOAD( "up02_c1.rom",  0x800, 0x400, 0xa4a4e7dc )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "up01_d2.rom",  0x0000, 0x4000,  0x18b4bcca )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "up01_b2.rom",  0x0000, 0x8000,  0xf269c0eb )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "up01_p2.rom",  0x00000, 0x8000, 0xc63a871f )
	ROM_LOAD( "up01_s2.rom",  0x08000, 0x8000, 0x760568d8 )
	ROM_LOAD( "up01_t2.rom",  0x10000, 0x8000, 0x57b35c73 )
ROM_END

ROM_START( tnk3_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "tnk3-p1.bin",  0x0000, 0x4000, 0x0d2a8ca9 )
	ROM_LOAD( "tnk3-p2.bin",  0x4000, 0x4000, 0x0ae0a483 )
	ROM_LOAD( "tnk3-p3.bin",  0x8000, 0x4000, 0xd16dd4db )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "tnk3-p4.bin",  0x0000, 0x4000, 0x01b45a90 )
	ROM_LOAD( "tnk3-p5.bin",  0x4000, 0x4000, 0x60db6667 )
	ROM_LOAD( "tnk3-p6.bin",  0x8000, 0x4000, 0x4761fde7 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "tnk3-p10.bin",  0x0000, 0x4000, 0x7bf0a517 )
	ROM_LOAD( "tnk3-p11.bin",  0x4000, 0x4000, 0x0569ce27 )

	ROM_REGION( 0xC00 ) /* color proms */
	ROM_LOAD( "7122.2",  0x800, 0x400, 0x34c06bc6 )
	ROM_LOAD( "7122.1",  0x400, 0x400, 0x6d0ac66a )
	ROM_LOAD( "7122.0",  0x000, 0x400, 0x4662b4c8 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "tnk3-p14.bin", 0x0000, 0x2000, 0x1fd18c43 )
	ROM_LOAD( "tnk3-p14.bin", 0x2000, 0x2000, 0x1fd18c43 )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "tnk3-p12.bin", 0x0000, 0x4000, 0xff495a16 )
	ROM_LOAD( "tnk3-p13.bin", 0x4000, 0x4000, 0xf8344843 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "tnk3-p7.bin", 0x00000, 0x4000, 0x06b92c88 )
	ROM_LOAD( "tnk3-p8.bin", 0x08000, 0x4000, 0x63d0e2eb )
	ROM_LOAD( "tnk3-p9.bin", 0x10000, 0x4000, 0x872e3fac )
ROM_END

/***********************************************************************/

ROM_START( ikarius_rom )
	ROM_REGION(0x10000)	/* CPU A */
	ROM_LOAD( "1.rom",  0x0000, 0x10000, 0x52a8b2dd )

	ROM_REGION(0x10000)	/* CPU B */
	ROM_LOAD( "2.rom",  0x0000, 0x10000, 0x45364d55 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "3.rom",  0x0000, 0x10000, 0x56a26699 )

	ROM_REGION(0xC00)	/* color proms */
	ROM_LOAD( "7122er.prm",  0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm",  0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm",  0x800, 0x400, 0x0a11cdde )

	ROM_REGION_DISPOSE(0x4000) /* characters */
	ROM_LOAD( "ik7",    0x00000, 0x4000, 0x9e88f536 )	/* characters */

	ROM_REGION( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 )
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f )
	ROM_LOAD( "19.rom", 0x10000, 0x8000, 0x9ee59e91 )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "8.rom",  0x00000, 0x8000, 0x9827c14a )
	ROM_LOAD( "9.rom",  0x08000, 0x8000, 0x545c790c )
	ROM_LOAD( "10.rom", 0x10000, 0x8000, 0xec9ba07e )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

ROM_START( ikarijp_rom ) /* unconfirmed */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "up03_14.rom",  0x0000, 0x4000, 0xcde006be )
	ROM_LOAD( "up03_k4.rom",  0x4000, 0x8000, 0x26948850 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "ik3",  0x0000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",  0x4000, 0x8000, 0x3a144bca )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "ik5",  0x0000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",  0x4000, 0x8000, 0x9b16aa57 )

	ROM_REGION(0xC00)	/* color proms */
	ROM_LOAD( "7122er.prm",  0x000, 0x400, 0xb9bf2c2c ) /* red */
	ROM_LOAD( "7122eg.prm",  0x400, 0x400, 0x0703a770 ) /* green */
	ROM_LOAD( "7122eb.prm",  0x800, 0x400, 0x0a11cdde ) /* blue */

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "ik7",    0x00000, 0x4000, 0x9e88f536 )	/* characters */

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 ) /* same as 17.rom */
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f ) /* same as 18.rom */
	ROM_LOAD( "ik19", 0x10000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a ) /* same as 20.rom */

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "ik8",  0x00000, 0x8000, 0x75d796d0 )
	ROM_LOAD( "ik9",  0x08000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10", 0x10000, 0x8000, 0xda9ccc94 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

ROM_START( ikarijpb_rom )
	ROM_REGION(0x10000) /* CPU A */
	ROM_LOAD( "ik1",		  0x00000, 0x4000, 0x2ef87dce )
	ROM_LOAD( "up03_k4.rom",  0x04000, 0x8000, 0x26948850 )

	ROM_REGION(0x10000) /* CPU B code */
	ROM_LOAD( "ik3",    0x0000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",    0x4000, 0x8000, 0x3a144bca )

	ROM_REGION(0x10000)				/* 64k for sound code */
	ROM_LOAD( "ik5",    0x0000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",    0x4000, 0x8000, 0x9b16aa57 )

	ROM_REGION(0xc00) /* color proms */
	ROM_LOAD( "7122er.prm", 0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm", 0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm", 0x800, 0x400, 0x0a11cdde )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "ik7", 0x0000, 0x4000, 0x9e88f536 )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 ) /* tiles */
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f )
	ROM_LOAD( "ik19",   0x10000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "ik8",    0x00000, 0x8000, 0x75d796d0 )
	ROM_LOAD( "ik9",    0x08000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10",   0x10000, 0x8000, 0xda9ccc94 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

/***********************************************************************/

ROM_START( victroad_rom )
	ROM_REGION( 0x10000 )	/* CPU A code */
	ROM_LOAD( "p1",  0x0000, 0x10000,  0xe334acef )

	ROM_REGION( 0x10000 )	/* CPU B code */
	ROM_LOAD( "p2",  0x00000, 0x10000, 0x907fac83 )

	ROM_REGION( 0x10000 )	/* sound code */
	ROM_LOAD( "p3",  0x00000, 0x10000, 0xbac745f6 )

	ROM_REGION( 0xC00 )	/* color proms */
	ROM_LOAD( "mb7122e.1k", 0x000, 0x400, 0x491ab831 )
	ROM_LOAD( "mb7122e.2l", 0x400, 0x400, 0x8feca424 )
	ROM_LOAD( "mb7122e.1l", 0x800, 0x400, 0x220076ca )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "p7",  0x0000, 0x4000,  0x2b6ed95b )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "p17",  0x00000, 0x8000, 0x19d4518c )
	ROM_LOAD( "p18",  0x08000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19",  0x10000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20",  0x18000, 0x8000, 0xedba0f31 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "p8",  0x00000, 0x8000, 0xdf7f252a )
	ROM_LOAD( "p9",  0x08000, 0x8000, 0x9897bc05 )
	ROM_LOAD( "p10", 0x10000, 0x8000, 0xecd3c0ea )

	ROM_REGION( 0x40000 ) /* 32x32 sprites */
	ROM_LOAD( "p11", 0x00000, 0x8000, 0x668b25a4 )
	ROM_LOAD( "p14", 0x08000, 0x8000, 0xa7031d4a )
	ROM_LOAD( "p12", 0x10000, 0x8000, 0xf44e95fa )
	ROM_LOAD( "p15", 0x18000, 0x8000, 0x120d2450 )
	ROM_LOAD( "p13", 0x20000, 0x8000, 0x980ca3d8 )
	ROM_LOAD( "p16", 0x28000, 0x8000, 0x9f820e8a )

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "p4",  0x00000, 0x10000, 0xe10fb8cc )
	ROM_LOAD( "p5",  0x10000, 0x10000, 0x93e5f110 )
ROM_END

ROM_START( dogosoke_rom ) /* Victory Road Japan */
	ROM_REGION(0x10000)	/* CPU A code */
	ROM_LOAD( "up03_p4.rom",  0x0000, 0x10000,  0x37867ad2 )

	ROM_REGION(0x10000)	/* CPU B code */
	ROM_LOAD( "up03_p8.rom",  0x00000, 0x10000, 0x907fac83 )

	ROM_REGION(0x10000)	/* sound code */
	ROM_LOAD( "up03_k7.rom",  0x00000, 0x10000, 0x173fa571 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_k1.rom",  0x000, 0x400, 0x10a2ce2b )
	ROM_LOAD( "up03_l2.rom",  0x400, 0x400, 0x99dc9792 )
	ROM_LOAD( "up03_l1.rom",  0x800, 0x400, 0xe7213160 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "up02_b3.rom",  0x0000, 0x4000,  0x51a4ec83 )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "p17",  0x00000, 0x8000, 0x19d4518c )
	ROM_LOAD( "p18",  0x08000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19",  0x10000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20",  0x18000, 0x8000, 0xedba0f31 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "up02_d3.rom",  0x00000, 0x8000, 0xd43044f8 )
	ROM_LOAD( "up02_e3.rom",  0x08000, 0x8000, 0x365ed2d8 )
	ROM_LOAD( "up02_g3.rom",  0x10000, 0x8000, 0x92579bf3 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "p11", 0x00000, 0x8000, 0x668b25a4 ) /* same as p11 */
	ROM_LOAD( "p14", 0x08000, 0x8000, 0xa7031d4a ) /* same as p14 */
	ROM_LOAD( "p12", 0x10000, 0x8000, 0xf44e95fa ) /* same as p12 */
	ROM_LOAD( "p15", 0x18000, 0x8000, 0x120d2450 ) /* same as p15 */
	ROM_LOAD( "p13", 0x20000, 0x8000, 0x980ca3d8 ) /* same as p13 */
	ROM_LOAD( "p16", 0x28000, 0x8000, 0x9f820e8a ) /* same as p16 */

	ROM_REGION(0x20000) /* ADPCM Samples */
	ROM_LOAD( "up03_f5.rom", 0x00000, 0x10000, 0x5b43fe9f )
	ROM_LOAD( "up03_g5.rom", 0x10000, 0x10000, 0xaae30cd6 )
ROM_END

/***********************************************************************/

ROM_START( bermudet_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "bt_p1.rom",  0x0000, 0x10000,  0x43dec5e9 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "bt_p2.rom",  0x00000, 0x10000, 0x0e193265 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "bt_p3.rom",  0x00000, 0x10000, 0x53a82e50 )    /* YM3526 */

	ROM_REGION( 0xc00 ) /* color proms - missing! */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "bt_p10.rom",  0x0000, 0x8000,  0xd3650211 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "bt_p19.rom",  0x00000, 0x10000, 0x8ed759a0 )
	ROM_LOAD( "bt_p20.rom",  0x10000, 0x10000, 0xab6217b7 )
	ROM_LOAD( "bt_p21.rom",  0x20000, 0x10000, 0xb7689599 )
	ROM_LOAD( "bt_p22.rom",  0x30000, 0x10000, 0x8daf7df4 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "bt_p6.rom",  0x00000, 0x8000, 0x8ffdf969 )
	ROM_LOAD( "bt_p7.rom",  0x08000, 0x8000, 0x268d10df )
	ROM_LOAD( "bt_p8.rom",  0x10000, 0x8000, 0x3e39e9dd )
	ROM_LOAD( "bt_p9.rom",  0x18000, 0x8000, 0xbf56da61 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "bt_p11.rom",  0x00000, 0x10000, 0xaae7410e )
	ROM_LOAD( "bt_p12.rom",  0x10000, 0x10000, 0x18914f70 )
	ROM_LOAD( "bt_p13.rom",  0x20000, 0x10000, 0xcd79ce81 )
	ROM_LOAD( "bt_p14.rom",  0x30000, 0x10000, 0xedc57117 )
	ROM_LOAD( "bt_p15.rom",  0x40000, 0x10000, 0x448bf9f4 )
	ROM_LOAD( "bt_p16.rom",  0x50000, 0x10000, 0x119999eb )
	ROM_LOAD( "bt_p17.rom",  0x60000, 0x10000, 0xb5462139 )
	ROM_LOAD( "bt_p18.rom",  0x70000, 0x10000, 0xcb416227 )

	ROM_REGION(0x20000)	/* Samples?? */
	ROM_LOAD( "bt_p4.rom",  0x00000, 0x10000, 0x4bc83229 )
	ROM_LOAD( "bt_p5.rom",  0x10000, 0x10000, 0x817bd62c )
ROM_END

/***********************************************************************/

ROM_START( chopper_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "kk_01.rom",  0x0000, 0x10000,  0x8fa2f839 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "kk_04.rom",  0x00000, 0x10000, 0x004f7d9a )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "kk_03.rom",  0x00000, 0x10000, 0xdbaafb87 )   /* YM3526 */

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_k1.rom",  0x000, 0x400, 0x794f0c09 )
	ROM_LOAD( "up03_k2.rom",  0x400, 0x400, 0xc3910507 )
	ROM_LOAD( "up03_l1.rom",  0x800, 0x400, 0xca7d0807 )

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "kk_05.rom",  0x0000, 0x8000, 0xdefc0987 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "kk_10.rom",  0x00000, 0x10000, 0x5cf4d22b )
	ROM_LOAD( "kk_11.rom",  0x10000, 0x10000, 0x9af4cad0 )
	ROM_LOAD( "kk_12.rom",  0x20000, 0x10000, 0x02fec778 )
	ROM_LOAD( "kk_13.rom",  0x30000, 0x10000, 0x2756817d )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "kk_06.rom",  0x00000, 0x08000, 0x284fad9e )
	ROM_LOAD( "kk_07.rom",  0x08000, 0x08000, 0xa0ebebdf )
	ROM_LOAD( "kk_08.rom",  0x10000, 0x08000, 0x2da45894 )
	ROM_LOAD( "kk_09.rom",  0x18000, 0x08000, 0x653c4342 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "kk_14.rom",  0x00000, 0x10000, 0x9bad9e25 )
	ROM_LOAD( "kk_15.rom",  0x10000, 0x10000, 0x89faf590 )
	ROM_LOAD( "kk_16.rom",  0x20000, 0x10000, 0xefb1fb6c )
	ROM_LOAD( "kk_17.rom",  0x30000, 0x10000, 0x6b7fb0a5 )
	ROM_LOAD( "kk_18.rom",  0x40000, 0x10000, 0x6abbff36 )
	ROM_LOAD( "kk_19.rom",  0x50000, 0x10000, 0x5283b4d3 )
	ROM_LOAD( "kk_20.rom",  0x60000, 0x10000, 0x6403ddf2 )
	ROM_LOAD( "kk_21.rom",  0x70000, 0x10000, 0x9f411940 )

	ROM_REGION(0x10000)	/* Samples ?? */
	ROM_LOAD( "kk_02.rom",  0x00000, 0x10000, 0x06169ae0 )
ROM_END

ROM_START( legofair_rom ) /* ChopperI (Japan) */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "up03_m4.rom",  0x0000, 0x10000,  0x79a485c0 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "up03_m8.rom",  0x00000, 0x10000, 0x96d3a4d9 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "kk_03.rom",  0x00000, 0x10000, 0xdbaafb87 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_k1.rom",  0x00000, 0x00400, 0x7f07a45c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x00400, 0x00400, 0x15359fc3 ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x00800, 0x00400, 0x79b50f7d ) /* blue */

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "up02_a3.rom",  0x0000, 0x8000,  0xdefc0987 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "kk_10.rom",  0x00000, 0x10000, 0x5cf4d22b )
	ROM_LOAD( "kk_11.rom",  0x10000, 0x10000, 0x9af4cad0 )
	ROM_LOAD( "kk_12.rom",  0x20000, 0x10000, 0x02fec778 )
	ROM_LOAD( "kk_13.rom",  0x30000, 0x10000, 0x2756817d )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "kk_06.rom",  0x00000, 0x08000, 0x68e49f05 )
	ROM_LOAD( "kk_07.rom",  0x10000, 0x08000, 0xe56e4804 )
	ROM_LOAD( "kk_08.rom",  0x20000, 0x08000, 0xa7ec26b6 )
	ROM_LOAD( "kk_09.rom",  0x30000, 0x08000, 0x170f2f95 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "kk_14",  0x00000, 0x10000, 0x9bad9e25 )
	ROM_LOAD( "kk_15",  0x10000, 0x10000, 0x89faf590 )
	ROM_LOAD( "kk_16",  0x20000, 0x10000, 0xefb1fb6c )
	ROM_LOAD( "kk_17",  0x30000, 0x10000, 0x6b7fb0a5 )
	ROM_LOAD( "kk_18",  0x40000, 0x10000, 0x6abbff36 )
	ROM_LOAD( "kk_19",  0x50000, 0x10000, 0x5283b4d3 )
	ROM_LOAD( "kk_20",  0x60000, 0x10000, 0x6403ddf2 )
	ROM_LOAD( "kk_21",  0x70000, 0x10000, 0x9f411940 )

	ROM_REGION(0x10000)	/* Samples?? */
	ROM_LOAD( "kk_02.rom",  0x00000, 0x10000, 0x06169ae0 )
ROM_END

/***********************************************************************/

ROM_START( gwar_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "g01",  0x00000, 0x10000, 0xce1d3c80 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "g02",  0x00000, 0x10000, 0x86d931bf )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "g03",  0x00000, 0x10000, 0xeb544ab9 )

	ROM_REGION( 0xc00 ) /* color proms */
	ROM_LOAD( "guprom.3", 0x000, 0x400, 0x090236a3 ) /* red */
	ROM_LOAD( "guprom.2", 0x400, 0x400, 0x9147de69 ) /* green */
	ROM_LOAD( "guprom.1", 0x800, 0x400, 0x7f9c839e ) /* blue */

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "g05",  0x0000, 0x08000, 0x80f73e2e )

	ROM_REGION_DISPOSE(0x40000) /* background tiles */
	ROM_LOAD( "g06",  0x00000, 0x10000, 0xf1dcdaef )
	ROM_LOAD( "g07",  0x10000, 0x10000, 0x326e4e5e )
	ROM_LOAD( "g08",  0x20000, 0x10000, 0x0aa70967 )
	ROM_LOAD( "g09",  0x30000, 0x10000, 0xb7686336 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "g10",  0x00000, 0x10000, 0x58600f7d )
	ROM_LOAD( "g11",  0x10000, 0x10000, 0xa3f9b463 )
	ROM_LOAD( "g12",  0x20000, 0x10000, 0x092501be )
	ROM_LOAD( "g13",  0x30000, 0x10000, 0x25801ea6 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "g14",  0x00000, 0x10000, 0x8dfc7b87 )
	ROM_LOAD( "g15",  0x10000, 0x10000, 0x06822aac )
	ROM_LOAD( "g16",  0x20000, 0x10000, 0xaeb3707f )
	ROM_LOAD( "g17",  0x30000, 0x10000, 0x0808f95f )
	ROM_LOAD( "g18",  0x40000, 0x10000, 0x2d653f0c )
	ROM_LOAD( "g19",  0x50000, 0x10000, 0xebbf3ba2 )
	ROM_LOAD( "g20",  0x60000, 0x10000, 0x2b46edff )
	ROM_LOAD( "g21",  0x70000, 0x10000, 0xbe19888d )

	ROM_REGION(0x10000)	/* Samples?? */
	ROM_LOAD( "g04",  0x00000, 0x10000, 0x2255f8dd )
ROM_END

/***********************************************************************/

ROM_START( psychos_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "p7",  0x00000, 0x10000, 0x562809f4 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "p6",  0x00000, 0x10000, 0x5f426ddb )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "p5",  0x0000, 0x10000,  0x64503283 )

	ROM_REGION(0x1400)	/* color PROM */
	ROM_LOAD( "mb7122e.1k", 0x0000, 0x400, 0x27b8ca8c ) /* red */
	ROM_LOAD( "mb7122e.2l", 0x0400, 0x400, 0x40e78c9e ) /* green */
	ROM_LOAD( "mb7122e.2k", 0x0800, 0x400, 0xd845d5ac ) /* blue */
	ROM_LOAD( "mb7122e.8j", 0x0c00, 0x400, 0xc20b197b ) /* ? */
	ROM_LOAD( "mb7122e.8k", 0x1000, 0x400, 0x5d0c617f ) /* ? */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "ps8",  0x0000, 0x8000,  0x11a71919 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "ps16",  0x00000, 0x10000, 0x167e5765 )
	ROM_LOAD( "ps15",  0x10000, 0x10000, 0x8b0fe8d0 )
	ROM_LOAD( "ps14",  0x20000, 0x10000, 0xf4361c50 )
	ROM_LOAD( "ps13",  0x30000, 0x10000, 0xe4b0b95e )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "ps9",   0x00000, 0x8000, 0x24559ee1 )
	ROM_LOAD( "ps10",  0x10000, 0x8000, 0xefa830e1 )
	ROM_LOAD( "ps11",  0x20000, 0x8000, 0x2b007733 )
	ROM_LOAD( "ps12",  0x30000, 0x8000, 0xf96f82db )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "ps17",  0x00000, 0x10000, 0x2bac250e )
	ROM_LOAD( "ps18",  0x10000, 0x10000, 0x5e1ba353 )
	ROM_LOAD( "ps19",  0x20000, 0x10000, 0x9ff91a97 )
	ROM_LOAD( "ps20",  0x30000, 0x10000, 0xae1965ef )
	ROM_LOAD( "ps21",  0x40000, 0x10000, 0xdf283b67 )
	ROM_LOAD( "ps22",  0x50000, 0x10000, 0x914f051f )
	ROM_LOAD( "ps23",  0x60000, 0x10000, 0xc4488472 )
	ROM_LOAD( "ps24",  0x70000, 0x10000, 0x8ec7fe18 )

	ROM_REGION(0x40000)	/* samples */
	ROM_LOAD( "p1",  0x00000, 0x10000, 0x58f1683f )
	ROM_LOAD( "p2",  0x10000, 0x10000, 0xda3abda1 )
	ROM_LOAD( "p3",  0x20000, 0x10000, 0xf3683ae8 )
	ROM_LOAD( "p4",  0x30000, 0x10000, 0x437d775a )
ROM_END

ROM_START( psychos_alt_rom ) /* USA set */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "up03_m4.rom",  0x0000, 0xC000,  0x05dfb409 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "up03_m8.rom",  0x00000, 0xC000, 0x5f426ddb )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "up03_j6.rom",  0x00000, 0xC000, 0xbbd0a8e3 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_k1.rom",  0x00000, 0x00400, 0x27b8ca8c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x00400, 0x00400, 0x40e78c9e ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x00800, 0x00400, 0xd845d5ac ) /* blue */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "up02_a3.rom",  0x0000, 0x8000,  0x11a71919 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "up01_a1.rom",  0x00000, 0x10000, 0xe4b0b95e )
	ROM_LOAD( "up01_c1.rom",  0x10000, 0x10000, 0xf4361c50 )
	ROM_LOAD( "up01_d1.rom",  0x20000, 0x10000, 0x8b0fe8d0 )
	ROM_LOAD( "up01_f1.rom",  0x30000, 0x10000, 0x167e5765 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "up02_b3.rom",  0x00000, 0x8000, 0x24559ee1 )
	ROM_LOAD( "up02_c3.rom",  0x10000, 0x8000, 0xefa830e1 )
	ROM_LOAD( "up02_e3.rom",  0x20000, 0x8000, 0x2b007733 )
	ROM_LOAD( "up02_f3.rom",  0x30000, 0x8000, 0xf96f82db )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_f10.rom",  0x00000, 0x10000, 0x2bac250e )
	ROM_LOAD( "up01_h10.rom",  0x10000, 0x10000, 0x5e1ba353 )
	ROM_LOAD( "up01_j10.rom",  0x20000, 0x10000, 0x9ff91a97 )
	ROM_LOAD( "up01_l10.rom",  0x30000, 0x10000, 0xae1965ef )
	ROM_LOAD( "up01_m10.rom",  0x40000, 0x10000, 0xdf283b67 )
	ROM_LOAD( "up01_n10.rom",  0x50000, 0x10000, 0x914f051f )
	ROM_LOAD( "up01_r10.rom",  0x60000, 0x10000, 0xc4488472 )
	ROM_LOAD( "up01_s10.rom",  0x70000, 0x10000, 0x8ec7fe18 )

	ROM_REGION(0x40000)	/* Samples */
	ROM_LOAD( "up03_b5.rom",  0x00000, 0x10000, 0x0f8e8276 )
	ROM_LOAD( "up03_c5.rom",  0x10000, 0x10000, 0x34e41dfb )
	ROM_LOAD( "up03_d5.rom",  0x20000, 0x10000, 0xaa583c5e )
	ROM_LOAD( "up03_f5.rom",  0x30000, 0x10000, 0x7e8bce7a )
ROM_END

/***********************************************************************/

ROM_START( tdfever_rom ) /* USA set - unconfirmed! */
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "td2-ver3.6c",  0x0000, 0x10000,  0x92138fe4 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "td1-ver3.2c",  0x00000, 0x10000, 0x798711f5 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "td3-ver3.3j",  0x00000, 0x10000, 0x5d13e0b1 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_e8.rom",  0x000, 0x00400, 0x67bdf8a0 )
	ROM_LOAD( "up03_d8.rom",  0x400, 0x00400, 0x9c4a9198 )
	ROM_LOAD( "up03_e9.rom",  0x800, 0x00400, 0xc93c18e8 )

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "td14ver3.4n",  0x0000, 0x8000,  0xe841bf1a )

	ROM_REGION_DISPOSE( 0x50000 )/* background tiles */
	ROM_LOAD( "up01_d8.rom",  0x00000, 0x10000, 0xad6e0927 )
	ROM_LOAD( "up01_e8.rom",  0x10000, 0x10000, 0x181db036 )
	ROM_LOAD( "up01_f8.rom",  0x20000, 0x10000, 0xc5decca3 )
	ROM_LOAD( "td18ver2.8gh", 0x30000, 0x10000, 0x3924da37 )
	ROM_LOAD( "up01_j8.rom",  0x40000, 0x10000, 0xbc17ea7f )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_j2.rom",  0x10000, 0x10000, 0x9b6d4053 ) /* plane 0 */
	ROM_LOAD( "up01_n2.rom",  0x00000, 0x10000, 0xa8979657 )
	ROM_LOAD( "up01_l2.rom",  0x30000, 0x10000, 0x28f49182 ) /* plane 1 */
	ROM_LOAD( "up01_k2.rom",  0x20000, 0x10000, 0x72a5590d )
	ROM_LOAD( "up01_p2.rom",  0x50000, 0x10000, 0xc8c71c7b ) /* plane 2 */
	ROM_LOAD( "up01_r2.rom",  0x40000, 0x10000, 0xa0d53fbd )
	ROM_LOAD( "up01_s2.rom",  0x70000, 0x10000, 0xf6f83d63 ) /* plane 3 */
	ROM_LOAD( "up01_t2.rom",  0x60000, 0x10000, 0x88e2e819 )
ROM_REGION( 1 ) /* dummy */

	ROM_REGION(0x20000)	/* sound samples? */
	ROM_LOAD( "up02_n6.rom",  0x00000, 0x10000, 0x155e472e )
	ROM_LOAD( "up02_p6.rom",  0x10000, 0x10000, 0x04794557 )
ROM_END

ROM_START( tdfeverj_rom )
	ROM_REGION(0x10000)	/* 64k for cpuA code */
	ROM_LOAD( "up02_c6.rom",  0x0000, 0x10000,  0x88d88ec4 )

	ROM_REGION(0x10000)	/* 64k for cpuB code */
	ROM_LOAD( "up02_c2.rom",  0x00000, 0x10000, 0x191e6442 )

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "up02_j3.rom",  0x00000, 0x10000, 0x4e4d71c7 )

	ROM_REGION(0xC00)	/* color PROM */
	ROM_LOAD( "up03_e8.rom",  0x000, 0x00400, 0x67bdf8a0 )
	ROM_LOAD( "up03_d8.rom",  0x400, 0x00400, 0x9c4a9198 )
	ROM_LOAD( "up03_e9.rom",  0x800, 0x00400, 0xc93c18e8 )

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "up01_n4.rom",  0x0000, 0x8000,  0xaf9bced5 )

	ROM_REGION_DISPOSE( 0x50000 )/* background tiles */
	ROM_LOAD( "up01_d8.rom",  0x00000, 0x10000, 0xad6e0927 )
	ROM_LOAD( "up01_e8.rom",  0x10000, 0x10000, 0x181db036 )
	ROM_LOAD( "up01_f8.rom",  0x20000, 0x10000, 0xc5decca3 )
	ROM_LOAD( "up01_g8.rom",  0x30000, 0x10000, 0x4512cdfb )
	ROM_LOAD( "up01_j8.rom",  0x40000, 0x10000, 0xbc17ea7f )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_j2.rom",  0x10000, 0x10000, 0x9b6d4053 )
	ROM_LOAD( "up01_n2.rom",  0x00000, 0x10000, 0xa8979657 )
	ROM_LOAD( "up01_l2.rom",  0x30000, 0x10000, 0x28f49182 )
	ROM_LOAD( "up01_k2.rom",  0x20000, 0x10000, 0x72a5590d )
	ROM_LOAD( "up01_p2.rom",  0x50000, 0x10000, 0xc8c71c7b )
	ROM_LOAD( "up01_r2.rom",  0x40000, 0x10000, 0xa0d53fbd )
	ROM_LOAD( "up01_s2.rom",  0x70000, 0x10000, 0xf6f83d63 )
	ROM_LOAD( "up01_t2.rom",  0x60000, 0x10000, 0x88e2e819 )
ROM_REGION( 1 ) /* dummy */

	ROM_REGION(0x20000)	/* sound samples? */
	ROM_LOAD( "up02_n6.rom",  0x00000, 0x10000, 0x155e472e )
	ROM_LOAD( "up02_p6.rom",  0x10000, 0x10000, 0x04794557 )
ROM_END

/***********************************************************************/

static int ikari_hiload(void){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xfc5f],"\x00\x30\x00",3) == 0){
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f){
			osd_fread(f,&RAM[0xff0b], 0x50);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0xfc5f] = RAM[0xff0b];
			RAM[0xfc60] = RAM[0xff0c];
			RAM[0xfc61] = RAM[0xff0d];
		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static int victroad_hiload(void){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xfc5f],"\x00\x30\x00",3) == 0){
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f){
			osd_fread(f,&RAM[0xff2c], 0x50);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0xfc5f] = RAM[0xff2c];
			RAM[0xfc60] = RAM[0xff2d];
			RAM[0xfc61] = RAM[0xff2e];
		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void ikari_hisave(void){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	/* The game clears hi score table after reset, */
	/* so we must be sure that hi score table was already loaded */

	if (memcmp(&RAM[0xfc2a],"\x48\x49",2) == 0){
		if (f){
			osd_fwrite(f,&RAM[0xff0b],0x50);
			osd_fclose(f);
		}
	}
}

static void victroad_hisave(void){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	/* The game clears hi score table after reset, */
	/* so we must be sure that hi score table was already loaded */

	if (memcmp(&RAM[0xfc2a],"\x48\x49",2) == 0){
		if (f){
			osd_fwrite(f,&RAM[0xff2c],0x50);
			osd_fclose(f);
		}
	}
}

/***********************************************/

static void ikarius_decode(void){
	unsigned char *RAM = Machine->memory_region[0];

	/*  Hack ROM test */
	RAM[0x11a6] = 0x00;
	RAM[0x11a7] = 0x00;
	RAM[0x11a8] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x1003] = 0xc3;
	RAM[0x1004] = 0x02;
	RAM[0x1005] = 0x10;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void ikarijp_decode(void){
	unsigned char *RAM = Machine->memory_region[0];
	RAM[0x190b] = 0xc9; /* faster test */

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void ikarijpb_decode( void ){
	dial_type = DIAL_BOOTLEG;
	hard_flags = 1;
}

static void aso_decode(void){
	unsigned char *RAM = Machine->memory_region[0];

	/* Hack Halt */
//	RAM[0x007e] = 0x00;

//	RAM = Machine->memory_region[2];

	/* Hack Halt */
//	RAM[0x007e] = 0x00;

	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void victroad_decode(void){
	unsigned char *RAM = Machine->memory_region[0];

	/* Hack ROM test */
	RAM[0x17bd] = 0x00;
	RAM[0x17be] = 0x00;
	RAM[0x17bf] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x161a] = 0xc3;
	RAM[0x161b] = 0x19;
	RAM[0x161c] = 0x16;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void dogosoke_decode(void){
	unsigned char *RAM = Machine->memory_region[0];
	/* Hack ROM test */
	RAM[0x179f] = 0x00;
	RAM[0x17a0] = 0x00;
	RAM[0x17a1] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x15fc] = 0xc3;
	RAM[0x15fd] = 0xfb;
	RAM[0x15fe] = 0x15;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void gwar_decode(void){
	unsigned char *RAM = Machine->memory_region[0];

	/* Hack ROM Test */
	RAM[0x0ce1] = 0x00;
	RAM[0x0ce2] = 0x00;

	/* Hack strange loop */
	RAM[0x008b] = 0x00;
	RAM[0x008c] = 0x00;

	/* Hack strange loop */
	RAM = Machine->memory_region[2];
	RAM[0x05] = 0x8c;

	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

static void chopper_decode(void){
	unsigned char *RAM = Machine->memory_region[0];

	/* Hack ROM Test */
//	RAM[0x0d43] = 0x00;
//	RAM[0x0d44] = 0x00;

	/* Hack strange loop */
//	RAM[0x0088] = 0x00;
//	RAM[0x0089] = 0x00;

	/* Hack strange loop */
//	RAM = Machine->memory_region[2];
//	RAM[0x05] = 0x80;

	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void bermudet_decode(void){
	/* Bermude Triangle  (NOT WORKING)*/
	unsigned char *RAM = Machine->memory_region[0];

//	RAM[0x127e] = 0xc9;

//	RAM[0x118d] = 0x00;
//	RAM[0x118e] = 0x00;

	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

static void tdfever_decode( void ){
	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void tnk3_decode( void ){
	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

/***********************************************************************/


struct GameDriver tnk3_driver =
{
	__FILE__,
	0,
	"tnk3",
	"TNK3",
	"1985",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&tnk3_machine_driver,
	0,

	tnk3_rom,
	tnk3_decode, 0,
	0,
	0, /* sound_prom */

	tnk3_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

struct GameDriver aso_driver =
{
	__FILE__,
	0,
	"aso",
	"ASO (Armored Scrum Object)",
	"1985",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&tnk3_machine_driver,
	0,

	aso_rom,
	aso_decode, 0,
	0,
	0, /* sound_prom */

	tnk3_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};


struct GameDriver asoj_driver =
{
	__FILE__,
	&asoj_driver,
	"asoj",
	"ASO (Armored Scrum Object) (Japan)",
	"1985",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&tnk3_machine_driver,
	0,

	asoj_rom,
	aso_decode, 0,
	0,
	0, /* sound_prom */

	tnk3_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

struct GameDriver athena_driver =
{
	__FILE__,
	0,
	"athena",
	"Athena",
	"1986",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&tnk3_machine_driver,
	0,

	athena_rom,
	tnk3_decode, 0,
	0,
	0, /* sound_prom */

	athena_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_90 ^ ORIENTATION_FLIP_X,
//	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,
	0,0
};

struct GameDriver ikari_driver = {
	__FILE__,
	0,
	"ikari",
	"Ikari Warriors (US)",
	"1986",
	"SNK",
	CREDITS,
	0,
	&ikari_machine_driver,
	0,

	ikarius_rom,
	ikarius_decode, 0,
	0,
	0, /* sound_prom */

	ikarius_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	ikari_hiload,ikari_hisave
};


struct GameDriver ikarijp_driver =
{
	__FILE__,
	&ikari_driver,
	"ikarijp",
	"Ikari Warriors (Japan)",
	"1986",
	"SNK",
	CREDITS,
	0,
	&ikari_machine_driver,
	0,

	ikarijp_rom,
	ikarijp_decode, 0,
	0,
	0, /* sound_prom */

	ikarijp_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	ikari_hiload,ikari_hisave
};

struct GameDriver ikarijpb_driver =
{
	__FILE__,
	&ikari_driver,
	"ikarijpb",
	"Ikari Warriors (Japan bootleg)",
	"1986",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&ikari_machine_driver,
	0,
	ikarijpb_rom,
	ikarijpb_decode,0,
	0,
	0, /* sound prom */

	ikarijp_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,
	ikari_hiload,ikari_hisave
};

struct GameDriver victroad_driver =
{
	__FILE__,
	0,
	"victroad",
	"Victory Road",
	"1986",
	"SNK",
	CREDITS,
	0,
	&ikari_machine_driver,
	0,

	victroad_rom,
	victroad_decode, 0,
	0,
	0, /* sound_prom */

	victroad_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	victroad_hiload,victroad_hisave
};


struct GameDriver dogosoke_driver =
{
	__FILE__,
	&victroad_driver,
	"dogosoke",
	"Dogosoken",
	"1986",
	"SNK",
	CREDITS,
	0,
	&ikari_machine_driver,
	0,

	dogosoke_rom,
	dogosoke_decode, 0,
	0,
	0, /* sound_prom */

	victroad_input_ports,

	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	victroad_hiload,victroad_hisave
};

struct GameDriver gwar_driver =
{
	__FILE__,
	0,
	"gwar",
	"Guerrilla War",
	"1987",
	"SNK",
	CREDITS,
	0,
	&gwar_machine_driver,
	0,

	gwar_rom,
	gwar_decode, 0,
	0,
	0, /* sound_prom */

	gwar_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

struct GameDriver bermudet_driver =
{
	__FILE__,
	0,
	"bermudet",
	"Bermude Triangle",
	"1987",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&gwar_machine_driver,
	0,

	bermudet_rom,
	bermudet_decode, 0,
	0,
	0, /* sound_prom */

	victroad_input_ports,

	PROM_MEMORY_REGION(0/*MEM_COLOR*/), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

struct GameDriver psychos_driver =
{
	__FILE__,
	0,
	"psychos",
	"Psycho Soldier",
	"1987",
	"SNK",
	CREDITS,
	0,
	&psychos_machine_driver,
	0,
	psychos_rom,
	psychos_decode, 0,
	0,
	0, /* sound_prom */
	psychos_input_ports,
	PROM_MEMORY_REGION( MEM_COLOR ), 0, 0,
	ORIENTATION_ROTATE_90 ^ ORIENTATION_FLIP_X,
	0,0
};

struct GameDriver chopper_driver =
{
	__FILE__,
	0,
	"chopper",
	"Chopper I",
	"1988",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&gwar_machine_driver,
	0,

	chopper_rom,
	chopper_decode, 0,
	0,
	0, /* sound_prom */

	psychos_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

struct GameDriver legofair_driver =
{
	__FILE__,
	&chopper_driver,
	"legofair",
	"The Legend of Air Cavalry",
	"1988",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&gwar_machine_driver,
	0,

	legofair_rom,
	chopper_decode, 0,
	0,
	0, /* sound_prom */

	psychos_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

/***********************************************************************/

struct GameDriver tdfever_driver =
{
	__FILE__,
	0,
	"tdfever",
	"TouchDown Fever (US)",
	"1987",
	"SNK",
	CREDITS,
	0,
	&tdfever_machine_driver,
	0,

	tdfever_rom,
	tdfever_decode, 0,
	0,
	0, /* sound_prom */

	tdfever_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

/***********************************************************************/

struct GameDriver tdfeverj_driver =
{
	__FILE__,
	&tdfever_driver,
	"tdfeverj",
	"TouchDown Fever (Japan)",
	"1987",
	"SNK",
	CREDITS,
	0,
	&tdfever_machine_driver,
	0,

	tdfeverj_rom,
	tdfever_decode, 0,
	0,
	0, /* sound_prom */

	tdfever_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};

/***********************************************************************/

struct GameDriver marvins_driver =
{
	__FILE__,
	0,
	"marvins",
	"Marvin's Maze",
	"????",
	"SNK",
	CREDITS,
	GAME_NOT_WORKING,
	&marvins_machine_driver,
	0,

	marvins_rom,
	0,0,//marvins_decode, 0,
	0,
	0, /* sound_prom */

	tdfever_input_ports,

	0,0,0,//PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_ROTATE_180 ^ ORIENTATION_FLIP_X,

	0,0
};
