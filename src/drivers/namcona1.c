/***************************************************************************
Namco NA-1 / NA-2 System


NA-1 Games:
-	Bakuretsu Quiz Ma-Q Dai Bouken
-	F/A
-	Super World Court (C354, C357)
-	Nettou! Gekitou! Quiztou!! (C354, C365 - both are 32pin)
-	Exvania (C350, C354)
-	Cosmo Gang the Puzzle (C356)
-	Tinkle Pit (C354, C367)
-	Emeraldia (C354, C358)

NA-2 Games:
-	Knuckle Heads
-	Numan Athletics
-	X-Day 2

To Do:
- Emeralda:
	After selecting the game type, background scrolling is briefly incorrect

- Emeralda:
	Shadow sprites, if enabled, make the score display invisible

- Hook up ROZ registers
	What signals that a layer makes use of them?
	What does each ROZ register do?

- Is view area controlled by registers?

- MCU
	Find sample table
	Analyse MCU "program" and "data" for each game

The board has a 28c16 EEPROM

No other CPUs on the board, but there are many custom chips.
Mother Board:
210 (28pin SOP)
C70 (80pin PQFP)
215 (176pin PQFP)
C218 (208pin PQFP)
219 (160pin PQFP)
Plus 1 or 2 custom chips on ROM board.

Notes:

-	NA-2 is backwards compatible with NA-1.

-	NA-2 games use a different MCU BIOS

-	Test mode for NA2 games includes an additional item: UART Test.
	No games are known to actually link up and use the UART feature.
	It's been confirmed that a Numan Athletics fails the UART test,
	behaving as it does in MAME.

-	Quiz games use 1p button 1 to pick test, 2p button 1 to begin test,
	and 2p button 2 to exit. Because quiz games doesn't have joystick.

-	Almost all quiz games using JAMMA edge connector assign
	button1 to up, button 2 to down, button 3 to left, button 4 to right.
	But Taito F2 quiz games assign button 3 to right and button 4 to left.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcona1.h"
#include "sound/namcona.h"

static data16_t *mpBank0, *mpBank1;
static data8_t mCoinCount[4];
static data8_t mCoinState;
static data16_t *mcu_ram;
static int mEnableInterrupts;
int namcona1_gametype;

/*************************************************************************/

static const UINT8 ExbaniaInitData[] =
{
/* This data oughtn't be necessary; when Exbania's EPROM area is uninitialized,
 * the game software automatically writes these values there, but then jumps
 * to an unmapped (bogus) address, causing MAME to crash.
 */
 	0x30,0x32,0x4f,0x63,0x74,0x39,0x32,0x52,0x45,0x56,0x49,0x53,0x49,0x4f,0x4e,0x35,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,
	0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x51,0x01,0x38,0x38,0xa7,0xbf,0xf1,0x04,0x0d,0x15,0x9b,0x80,0x1f,0x83,0xd5,0xa4,
	0x69,0x88,0x7c,0x9f,0xb6,0x01,0xda,0x93,0x17,0x45,0x8b,0x12,0xb2,0x02,0x33,0x5c,
	0x50,0xd6,0xe1,0x56,0xa4,0xad,0x42,0x4a,0x5c,0xdd,0x86,0x61,0xe9,0x03,0x12,0xe1,
	0x0f,0x9b,0xea,0x26,0x2c,0x61,0xdc,0x62,0x48,0x6b,0x6d,0x14,0xe0,0x03,0x85,0x4a,
	0x72,0x46,0xda,0x96,0xc8,0x7d,0x1c,0xd1,0x05,0x3e,0xe5,0x92,0x70,0x43,0x5f,0x6c,
	0x03,0x05,0xb3,0xeb,0xb3,0x20,0x35,0x4d,0x7e,0x66,0x50,0x01,0x36,0xc0,0x33,0xe1,
	0x0f,0xc9,0x38,0x2e,0xe9,0x29,0x19,0x4f,0x5e,0xb1,0xd1,0x49,0x8b,0x3b,0x53,0xfd,
	0x9f,0x3f,0xee,0x25,0x25,0x35,0x7b,0x0d,0x11,0xaf,0x4c,0x11,0x8c,0x32,0xd4,0xda,
	0x7f,0xd8,0x16,0x57,0xe1,0xa6,0xce,0x7d,0xc1,0xae,0x62,0xbf,0x13,0xe4,0x87,0x4c,
	0x3a,0xc1,0xb3,0x0c,0x59,0x99,0x47,0x58,0x5a,0xbd,0x78,0x7c,0xba,0x50,0x01,0xed,
	0x1b,0xea,0x8a,0x49,0x88,0xee,0xd6,0x14,0x85,0xab,0xb0,0x2c,0xde,0x35,0x93,0x11,
	0x2d,0x01,0x1c,0xd7,0x28,0x43,0x30,0xe7,0xb0,0x08,0xed,0x79,0x00,0x00,0x00,0x00
}; /* ExbaniaInitData */

static data8_t namcona1_nvmem[NA1_NVRAM_SIZE];

static NVRAM_HANDLER( namcosna1 )
{
	if( read_or_write )
	{
		mame_fwrite( file, namcona1_nvmem, NA1_NVRAM_SIZE );
	}
	else
	{
		if (file)
		{
			mame_fread( file, namcona1_nvmem, NA1_NVRAM_SIZE );
		}
		else
		{
			memset( namcona1_nvmem, 0x00, NA1_NVRAM_SIZE );

			if (namcona1_gametype == NAMCO_EXBANIA)
			{
				memcpy( namcona1_nvmem, ExbaniaInitData, sizeof(ExbaniaInitData) );
			}
		}
	}
} /* namcosna1_nvram_handler */

static READ16_HANDLER( namcona1_nvram_r )
{
	return namcona1_nvmem[offset];
} /* namcona1_nvram_r */

static WRITE16_HANDLER( namcona1_nvram_w )
{
	if( ACCESSING_LSB )
	{
		namcona1_nvmem[offset] = data&0xff;
	}
} /* namcona1_nvram_w */

/***************************************************************************/

INPUT_PORTS_START( namcona1_joy )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 )

	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( namcona1_quiz )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 )

	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

/***************************************************************************/

static void
simulate_mcu( void )
{
	int i;
	data16_t data;
	data8_t poll_coins;

	mcu_ram[0xf60/2] = 0x0000; /* mcu ready */

	mcu_ram[0xfc0/2] = readinputport(0x0); /* dipswitch */

	for( i=1; i<=4; i++ )
	{
		data = readinputport(i)<<8;
		switch( namcona1_gametype )
		{
		case NAMCO_KNCKHEAD:
		case NAMCO_BKRTMAQ:
		case NAMCO_QUIZTOU:
		case NAMCO_EXBANIA:
			data |= data>>8;
			break;

		case NAMCO_TINKLPIT:
			if( data&0x2000 ) data |= 0x20; /* throw */
			if( data&0x4000 ) data |= 0x10; /* jump */
			if( i==1 )
			{
				if( readinputport(1)&0x80 ) data |= 0x80; /* P1 start */
				if( readinputport(2)&0x80 ) data |= 0x40; /* P2 start */
			}
			break;

		default:
			break;
		}
		mcu_ram[0xfc0/2+i] = data;
	}

	/* "analog" and "encoder" ports are polled during test mode,
	 * but I haven't found any games that make use of them.
	 */
	mcu_ram[0xfc0/2+0x05] = 0xffff; /* analog0,1 */
	mcu_ram[0xfc0/2+0x06] = 0xffff; /* analog2,3 */
	mcu_ram[0xfc0/2+0x07] = 0xffff; /* analog4,5 */
	mcu_ram[0xfc0/2+0x08] = 0xffff; /* analog6,7 */
	mcu_ram[0xfc0/2+0x09] = 0xffff; /* encoder0,1 */

	poll_coins = readinputport(5); /* coin input */
	if( (poll_coins&0x8)&~(mCoinState&0x8) ) mCoinCount[0]++;
	if( (poll_coins&0x4)&~(mCoinState&0x4) ) mCoinCount[1]++;
	if( (poll_coins&0x2)&~(mCoinState&0x2) ) mCoinCount[2]++;
	if( (poll_coins&0x1)&~(mCoinState&0x1) ) mCoinCount[3]++;
	mCoinState = poll_coins;

	mcu_ram[0xfc0/2+0xa] = (mCoinCount[0]<<8)|mCoinCount[1];
	mcu_ram[0xfc0/2+0xb] = (mCoinCount[2]<<8)|mCoinCount[3];

	/* special handling for F/A */
	data = ~((readinputport(1)<<8)|readinputport(2));
	mcu_ram[0xffc/2] = data;
	mcu_ram[0xffe/2] = data;
} /* simulate_mcu */

static READ16_HANDLER( namcona1_mcu_r )
{
	return mcu_ram[offset];
}

static WRITE16_HANDLER( namcona1_mcu_w )
{
	COMBINE_DATA( &mcu_ram[offset] );
	if( offset>=0x400/2 && (offset<0x820/2 || (offset>=0xf30/2 && offset<0xf72/2)) )
	{
		logerror( "0x%03x: 0x%04x\n", offset*2, mcu_ram[offset] );
	}
	/*
		400..53d  code for MCU?

		820:					song select
		822:					song control (volume? tempo?)
		824,826,828,....89e:	sample select
			0x40 is written to odd addresses to signal the MCU that a sound command has been issued

		8f0: 0x07 unknown
		8f2: 0x01 unknown
		8f4: 0xa4 unknown

		f30..f71	data for MCU
		f72: MCU command:
			0x07 = identify version
			0x03 = process data
			0x87 = ?

		fc0..fc9: used by knuckleheads (NA2-specific?)

		fd8: ?

		fbf: watchdog
	*/
}

/* NA2 hardware sends a special command to the MCU, then tests to
 * see if the proper BIOS version string appears in shared memory.
 */
static void write_version_info( void )
{
	const data16_t source[0x8] =
	{ /* "NSA-BIOS ver"... */
		0x534e,0x2d41,0x4942,0x534f,0x7620,0x7265,0x2e31,0x3133
	};
	int i;
	for( i=0; i<8; i++ )
	{
		namcona1_workram[i] = source[i];
	}
} /* write_version_info */

static WRITE16_HANDLER( mcu_command_w )
{
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	data16_t cmd = pMem[0xf72/2]>>8;

	switch( cmd ){
	case 0x03:
		/* Process data at 0xf30..0xf71
		 *
		 * f30: 0101 0020 0400 013e 8a00 0000 0000 011e
		 * f40: 0301 0000 0000 0000 8a16 0000 0000 012c
		 * f50: 0301 0000 0000 0000 8a61 0000 0000 019e
		 * f60: 0301 0000 0000 0000 8a61 0000 0000 01ae
		 * f70: 0000
		 *
		 * f30: 0301 0000 0000 0000 8a88 0000 0000 0120
		 * f40: 0301 0000 0000 0000 8ad1 0000 0000 015a
		 * f50: 0301 0000 0000 0000 8af6 0000 0000 011c
		 * f60: 0301 0000 0000 0000 8b08 0000 0000 0114
		 * f70: 0000
		 *
		 * f30: 0300 0000 0000 0000 8b1f 2004 0000 0000
		 * f40: 0301 0000 0000 0000 8b33 0000 0000 8902
		 * f50: 0000
		 */
		break;

	case 0x07:
		/* This command is used to detect Namco NA-2 hardware; without it,
		 * NA-2 games (Knuckleheads, Numan Athletics) refuse to run.
		 */
		write_version_info();
		break;
	}
} /* mcu_command_w */

/***************************************************************************/
/* sound
 *
 * 8 bit signed PCM data
 * copied to workram
 *
 *	0x01fffc: pointer
 *	0x020000: samples
 *	0x040000: samples
 *	0x060000: samples
 *
 *	0x070000: metadata; 10 byte frames
 */

/***************************************************************************/

/**
 * "Custom Key" Emulation
 *
 * The primary purpose of the custom key chip is protection.  It prevents
 * ROM swaps from working with games that otherwise run on the same hardware.
 *
 * The secondary purpose of the custom key chip is to act as a random number
 * generator in some games.
 */
static READ16_HANDLER( custom_key_r )
{
	static unsigned char keyseq;
	static data16_t count;
	int old_count;

	old_count = count;
	do
	{
		count = rand();
	} while( old_count == count );

	switch( namcona1_gametype ){
	case NAMCO_BKRTMAQ:
		if( offset==2 ) return 0x015c;
		break;
	case NAMCO_FA:
		if( offset==2 ) return 0x015d;
		if( offset==4 ) return count;
		break;
	case NAMCO_EXBANIA:
		if( offset==2 ) return 0x015e;
		break;
	case NAMCO_CGANGPZL:
		if( offset==1 ) return 0x0164;
		if( offset==2 ) return count;
		break;
	case NAMCO_SWCOURT:
		if( offset==1 ) return 0x0165;
		if( offset==2 ) return count;
		break;
	case NAMCO_EMERALDA:
		if( offset==1 ) return 0x0166;
		if( offset==2 ) return count;
		break;
	case NAMCO_NUMANATH:
		if( offset==1 ) return 0x0167;
		if( offset==2 ) return count;
		break;
	case NAMCO_KNCKHEAD:
		if( offset==1 ) return 0x0168;
		if( offset==2 ) return count;
		break;
	case NAMCO_QUIZTOU:
		if( offset==2 ) return 0x016d;
		break;
	case NAMCO_TINKLPIT:
		if( offset==7 ) return 0x016f;
		if( offset==4 ) keyseq = 0;
		if( offset==3 )
		{
			const data16_t data[] =
			{
				0x0000,0x2000,0x2100,0x2104,0x0106,0x0007,0x4003,0x6021,
				0x61a0,0x31a4,0x9186,0x9047,0xc443,0x6471,0x6db0,0x39bc,
				0x9b8e,0x924f,0xc643,0x6471,0x6db0,0x19bc,0xba8e,0xb34b,
				0xe745,0x4576,0x0cb7,0x789b,0xdb29,0xc2ec,0x16e2,0xb491
			};
			return data[(keyseq++)&0x1f];
		}
		break;
	default:
		return 0;
	}
	return rand()&0xffff;
} /* custom_key_r */

static WRITE16_HANDLER( custom_key_w )
{
} /* custom_key_w */

/***************************************************************/

static READ16_HANDLER( namcona1_vreg_r )
{
	return namcona1_vreg[offset];
} /* namcona1_vreg_r */

static int
transfer_dword( UINT32 dest, UINT32 source )
{
	data16_t data;

	if( source>=0x400000 && source<0xc00000 )
	{
		data = mpBank1[(source-0x400000)/2];
	}
	else if( source>=0xc00000 && source<0xe00000 )
	{
		data = mpBank0[(source-0xc00000)/2];
	}
	else if( source<0x80000 && source>=0x1000 )
	{
		data = namcona1_workram[(source-0x001000)/2];
	}
	else
	{
		logerror( "bad blt src %08x\n", source );
		return -1;
	}
	if( dest>=0xf00000 && dest<=0xf02000 )
	{
		namcona1_paletteram_w( (dest-0xf00000)/2, data, 0x0000 );
	}
	else if( dest>=0xf40000 && dest<=0xf80000 )
	{
		namcona1_gfxram_w( (dest-0xf40000)/2, data, 0x0000 );
	}
	else if( dest>=0xff0000 && dest<0xff8000 )
	{
		namcona1_videoram_w( (dest-0xff0000)/2, data, 0x0000 );
	}
	else if( dest>=0xff8000 && dest<=0xffdfff )
	{
		namcona1_sparevram[(dest-0xff8000)/2] = data;
	}
	else if( dest>=0xfff000 && dest<=0xffffff )
	{
		spriteram16[(dest-0xfff000)/2] = data;
	}
	else
	{
		logerror( "bad blt dst %08x\n", dest );
		return -1;
	}
	return 0;
} /* transfer_dword */

static void
blit_setup( int format, int *bytes_per_row, int *pitch, int mode )
{
	if( mode == 3 )
	{
		switch( format )
		{
		case 0x0001:
			*bytes_per_row = 0x1000;
			*pitch = 0x1000;
			break;

		case 0x0081:
			*bytes_per_row = 4*8;
			*pitch = 36*8;
			break;

		default:
	//	case 0x00f1:
	//	case 0x00f9:
	//	case 0x00fd:
			*bytes_per_row = (64 - (format>>2))*0x08;
			*pitch = 0x200;
			break;
		}
	}
	else
	{
		switch( format )
		{
		case 0x00bd: /* Numan Athletics */
			*bytes_per_row = 4;
			*pitch = 0x120;
			break;
		case 0x008d: /* Numan Athletics */
			*bytes_per_row = 8;
			*pitch = 0x120;
			break;

		case 0x0001:
			*bytes_per_row = 0x1000;
			*pitch = 0x1000;
			break;

		case 0x0401: /* F/A */
			*bytes_per_row = 4*0x40;
			*pitch = 36*0x40;
			break;

		default:
	//	case 0x00f1:
	//	case 0x0781:
	//	case 0x07c1:
	//	case 0x07e1:
			*bytes_per_row = (64 - (format>>5))*0x40;
			*pitch = 0x1000;
			break;
		}
	}
} /* blit_setup */

/*
$efff20: sprite control: 0x3a,0x3e,0x3f
			bit 0x01 selects spriteram bank

$efff00:	src0 src1 src2 dst0 dst1 dst2 BANK [src
$efff10:	src] [dst dst] #BYT BLIT eINT 001f 0001
$efff20:	003f 003f IACK ---- ---- ---- ---- ----
$efff30:	---- ---- ---- ---- ---- ---- ---- ----
$efff40:	---- ---- ---- ---- ---- ---- ---- ----
$efff50:	---- ---- ---- ---- ---- ---- ---- ----
$efff60:	---- ---- ---- ---- ---- ---- ---- ----
$efff70:	---- ---- ---- ---- ---- ---- ---- ----
$efff80:	0050 0170 0020 0100 0000 0000 0000 GFXE
$efff90:	0000 0001 0002 0003 FLIP ---- ---- ----
$efffa0:	PRI  PRI  PRI  PRI  ---- ---- 00c0 ----		priority (0..7)
$efffb0:	COLR COLR COLR COLR 0001 0004 0000 ----		color (0..f)
$efffc0:	???? ???? ???? ????	???? ???? ???? ----		ROZ

Emeralda:
$efff80:	0048 0177 0020 0100 0000 00fd 0000 GFXE

*/
static void namcona1_blit( void )
{
	int src0 = namcona1_vreg[0x0];
	int src1 = namcona1_vreg[0x1];
	int src2 = namcona1_vreg[0x2];

	int dst0 = namcona1_vreg[0x3];
	int dst1 = namcona1_vreg[0x4];
	int dst2 = namcona1_vreg[0x5];

	int gfxbank = namcona1_vreg[0x6];

	/* dest and source are provided as dword offsets */
	UINT32 src_baseaddr	= 2*((namcona1_vreg[0x7]<<16)|namcona1_vreg[0x8]);
	UINT32 dst_baseaddr	= 2*((namcona1_vreg[0x9]<<16)|namcona1_vreg[0xa]);

	int num_bytes = namcona1_vreg[0xb];

	int dest_offset, source_offset;
	int dest_bytes_per_row, dst_pitch;
	int source_bytes_per_row, src_pitch;

	(void)dst2;
	(void)dst0;
	(void)src2;
	(void)src0;
/*
	logerror( "0x%08x: blt(%08x,%08x,%08x);%04x %04x %04x; %04x %04x %04x; gfx=%04x\n",
		activecpu_get_pc(),
		dst_baseaddr,src_baseaddr,num_bytes,
		src0,src1,src2,
		dst0,dst1,dst2,
		gfxbank );
*/
	blit_setup( dst1, &dest_bytes_per_row, &dst_pitch, gfxbank);
	blit_setup( src1, &source_bytes_per_row, &src_pitch, gfxbank );

	if( num_bytes&1 )
	{
		num_bytes++;
	}

	if( dst_baseaddr < 0xf00000 )
	{
		dst_baseaddr += 0xf40000;
	}

	dest_offset		= 0;
	source_offset	= 0;

	while( num_bytes>0 )
	{
		if( transfer_dword(
			dst_baseaddr + dest_offset,
			src_baseaddr + source_offset ) )
		{
			return;
		}

		num_bytes -= 2;

		dest_offset+=2;
		if( dest_offset >= dest_bytes_per_row )
		{
			dst_baseaddr += dst_pitch;
			dest_offset = 0;
		}

		source_offset+=2;
		if( source_offset >= source_bytes_per_row )
		{
			src_baseaddr += src_pitch;
			source_offset = 0;
		}
	}
} /* namcona1_blit */

static WRITE16_HANDLER( namcona1_vreg_w )
{
	COMBINE_DATA( &namcona1_vreg[offset] );

	switch( offset )
	{
	case 0x18/2:
		namcona1_blit();
		/* see also 0x1e */
		break;

	case 0x1a/2:
		mEnableInterrupts = 1;
		/* interrupt enable mask; 0 enables INT level */
		break;
	}
} /* namcona1_vreg_w */

/***************************************************************/

static MEMORY_READ16_START( namcona1_readmem )
	{ 0x000000, 0x000fff, namcona1_mcu_r },
	{ 0x001000, 0x07ffff, MRA16_RAM },		/* work RAM */
	{ 0x400000, 0xbfffff, MRA16_BANK2 },	/* data */
	{ 0xc00000, 0xdfffff, MRA16_BANK1 },	/* code */
	{ 0xe00000, 0xe00fff, namcona1_nvram_r },
	{ 0xe40000, 0xe4000f, custom_key_r },
	{ 0xefff00, 0xefffff, namcona1_vreg_r },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_r },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_r },
	{ 0xff0000, 0xff7fff, namcona1_videoram_r },
	{ 0xff8000, 0xffdfff, MRA16_RAM },		/* spare videoram */
	{ 0xffe000, 0xffefff, MRA16_RAM },		/* scroll registers */
	{ 0xfff000, 0xffffff, MRA16_RAM },		/* spriteram */
MEMORY_END

static MEMORY_WRITE16_START( namcona1_writemem )
	{ 0x000000, 0x000fff, namcona1_mcu_w, &mcu_ram },
	{ 0x001000, 0x07ffff, MWA16_RAM, &namcona1_workram },
	{ 0x3f8008, 0x3f8009, mcu_command_w },
	{ 0x400000, 0xdfffff, MWA16_ROM }, /* data + code */
	{ 0xe00000, 0xe00fff, namcona1_nvram_w },
	{ 0xe40000, 0xe4000f, custom_key_w },
	{ 0xefff00, 0xefffff, namcona1_vreg_w, &namcona1_vreg },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_w, &paletteram16 },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_w },
	{ 0xff0000, 0xff7fff, namcona1_videoram_w, &videoram16 },
	{ 0xff8000, 0xffdfff, MWA16_RAM, &namcona1_sparevram },
	{ 0xffe000, 0xffefff, MWA16_RAM, &namcona1_scroll },
	{ 0xfff000, 0xffffff, MWA16_RAM, &spriteram16 },
MEMORY_END

INTERRUPT_GEN( namcona1_interrupt )
{
	int level = cpu_getiloops(); /* 0,1,2,3,4 */
	if( level==0 )
	{
		simulate_mcu();
	}
	if( mEnableInterrupts )
	{
		if( (namcona1_vreg[0x1a/2]&(1<<level))==0 )
		{
			cpu_set_irq_line(0, level+1, HOLD_LINE);
		}
	}
}

static struct NAMCONAinterface NAMCONA_interface =
{
	4*8000,
	REGION_CPU1,
	100
};

/* cropped at sides */
static MACHINE_DRIVER_START( namcona1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000) /* 8MHz? */
	MDRV_CPU_MEMORY(namcona1_readmem,namcona1_writemem)
	MDRV_CPU_VBLANK_INT(namcona1_interrupt,5)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(namcosna1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(38*8, 32*8)
	MDRV_VISIBLE_AREA(8, 38*8-1-8, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(namcona1)
	MDRV_VIDEO_UPDATE(namcona1)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCONA, NAMCONA_interface)
MACHINE_DRIVER_END


/* full-width */
static MACHINE_DRIVER_START( namcona1w )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(namcona1)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 38*8-1-0, 4*8, 32*8-1)
MACHINE_DRIVER_END


static void init_namcona1( size_t program_size )
{
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	pMem[0] = 0x0007; pMem[1] = 0xfffc; /* (?) stack */
	pMem[2] = 0x00c0; pMem[3] = 0x0000; /* reset vector */

	mpBank0 = &pMem[0x80000/2];
	mpBank1 = mpBank0 + program_size/2;

	cpu_setbank( 1, mpBank0 ); /* code */
	cpu_setbank( 2, mpBank1 ); /* data */

	mCoinCount[0] = mCoinCount[1] = mCoinCount[2] = mCoinCount[3] = 0;
	mCoinState = 0;
	mEnableInterrupts = 0;
}

DRIVER_INIT( bkrtmaq ){		init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_BKRTMAQ; }
DRIVER_INIT( cgangpzl ){	init_namcona1( 0x100000 ); namcona1_gametype = NAMCO_CGANGPZL; }
DRIVER_INIT( emeralda ){	init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_EMERALDA; }
DRIVER_INIT( exbania ){		init_namcona1( 0x100000 ); namcona1_gametype = NAMCO_EXBANIA; }
DRIVER_INIT( fa ){			init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_FA; }
DRIVER_INIT( knckhead ){	init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_KNCKHEAD; }
DRIVER_INIT( numanath ){	init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_NUMANATH; }
DRIVER_INIT( quiztou ){		init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_QUIZTOU; }
DRIVER_INIT( swcourt ){		init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_SWCOURT; }
DRIVER_INIT( tinklpit ){	init_namcona1( 0x200000 ); namcona1_gametype = NAMCO_TINKLPIT; }


ROM_START( bkrtmaq )
	ROM_REGION( 0x680000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mq1_ep0l.bin", 0x080001, 0x080000, 0xf029bc57 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "mq1_ep0u.bin", 0x080000, 0x080000, 0x4cff62b8 )
	ROM_LOAD16_BYTE( "mq1_ep1l.bin", 0x180001, 0x080000, 0xe3be6f4b )
	ROM_LOAD16_BYTE( "mq1_ep1u.bin", 0x180000, 0x080000, 0xb44e31b2 )

	ROM_LOAD16_BYTE( "mq1_ma0l.bin", 0x280001, 0x100000, 0x11fed35f ) /* 0x400000 */
	ROM_LOAD16_BYTE( "mq1_ma0u.bin", 0x280000, 0x100000, 0x23442ac0 )
	ROM_LOAD16_BYTE( "mq1_ma1l.bin", 0x480001, 0x100000, 0xfe82205f )
	ROM_LOAD16_BYTE( "mq1_ma1u.bin", 0x480000, 0x100000, 0x0cdb6bd0 )
ROM_END

ROM_START( cgangpzl )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cp2-ep0l.bin", 0x080001, 0x80000, 0x8f5cdcc5 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "cp2-ep0u.bin", 0x080000, 0x80000, 0x3a816140 )
ROM_END

ROM_START( cgangpzj )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cp1-ep0l.bin", 0x080001, 0x80000, 0x2825f7ba ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "cp1-ep0u.bin", 0x080000, 0x80000, 0x94d7d6fc )
ROM_END

ROM_START( emeralda )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep0lb.bin",    0x080001, 0x080000, 0xfcd55293 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "ep0ub.bin",    0x080000, 0x080000, 0xa52f00d5 )
	ROM_LOAD16_BYTE( "em1-ep1l.bin", 0x180001, 0x080000, 0x373c1c59 )
	ROM_LOAD16_BYTE( "em1-ep1u.bin", 0x180000, 0x080000, 0x4e969152 )
ROM_END

ROM_START( emerldaa )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "em1-ep0l.bin", 0x080001, 0x080000, 0x443f3fce ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "em1-ep0u.bin", 0x080000, 0x080000, 0x484a2a81 )
	ROM_LOAD16_BYTE( "em1-ep1l.bin", 0x180001, 0x080000, 0x373c1c59 )
	ROM_LOAD16_BYTE( "em1-ep1u.bin", 0x180000, 0x080000, 0x4e969152 )
ROM_END

ROM_START( exvania )
	ROM_REGION( 0x580000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ex1-ep0l.bin", 0x080001, 0x080000, 0x18c12015 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "ex1-ep0u.bin", 0x080000, 0x080000, 0x07d054d1 )

	ROM_LOAD16_BYTE( "ex1-ma0l.bin", 0x180001, 0x100000, 0x17922cd4 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "ex1-ma0u.bin", 0x180000, 0x100000, 0x93d66106 )
	ROM_LOAD16_BYTE( "ex1-ma1l.bin", 0x380001, 0x100000, 0xe4bba6ed )
	ROM_LOAD16_BYTE( "ex1-ma1u.bin", 0x380000, 0x100000, 0x04e7c4b0 )
ROM_END

ROM_START( knckhead )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "kh2-ep0l.bin", 0x080001, 0x080000, 0xb4b88077 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "kh2-ep0u.bin", 0x080000, 0x080000, 0xa578d97e )
	ROM_LOAD16_BYTE( "kh1-ep1l.bin", 0x180001, 0x080000, 0x27e6ab4e )
	ROM_LOAD16_BYTE( "kh1-ep1u.bin", 0x180000, 0x080000, 0x487b2434 )

	ROM_LOAD16_BYTE( "kh1-ma0l.bin", 0x280001, 0x100000, 0x7b2db5df ) /* 0x400000 */
	ROM_LOAD16_BYTE( "kh1-ma0u.bin", 0x280000, 0x100000, 0x6983228b )
	ROM_LOAD16_BYTE( "kh1-ma1l.bin", 0x480001, 0x100000, 0xb24f93e6 )
	ROM_LOAD16_BYTE( "kh1-ma1u.bin", 0x480000, 0x100000, 0x18a60348 )
	ROM_LOAD16_BYTE( "kh1-ma2l.bin", 0x680001, 0x100000, 0x82064ee9 )
	ROM_LOAD16_BYTE( "kh1-ma2u.bin", 0x680000, 0x100000, 0x17fe8c3d )
	ROM_LOAD16_BYTE( "kh1-ma3l.bin", 0x880001, 0x100000, 0xad9a7807 )
	ROM_LOAD16_BYTE( "kh1-ma3u.bin", 0x880000, 0x100000, 0xefeb768d )
ROM_END

ROM_START( knckhedj )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "kh1-ep0l.bin", 0x080001, 0x080000, 0x94660bec ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "kh1-ep0u.bin", 0x080000, 0x080000, 0xad640d69 )
	ROM_LOAD16_BYTE( "kh1-ep1l.bin", 0x180001, 0x080000, 0x27e6ab4e )
	ROM_LOAD16_BYTE( "kh1-ep1u.bin", 0x180000, 0x080000, 0x487b2434 )

	ROM_LOAD16_BYTE( "kh1-ma0l.bin", 0x280001, 0x100000, 0x7b2db5df ) /* 0x400000 */
	ROM_LOAD16_BYTE( "kh1-ma0u.bin", 0x280000, 0x100000, 0x6983228b )
	ROM_LOAD16_BYTE( "kh1-ma1l.bin", 0x480001, 0x100000, 0xb24f93e6 )
	ROM_LOAD16_BYTE( "kh1-ma1u.bin", 0x480000, 0x100000, 0x18a60348 )
	ROM_LOAD16_BYTE( "kh1-ma2l.bin", 0x680001, 0x100000, 0x82064ee9 )
	ROM_LOAD16_BYTE( "kh1-ma2u.bin", 0x680000, 0x100000, 0x17fe8c3d )
	ROM_LOAD16_BYTE( "kh1-ma3l.bin", 0x880001, 0x100000, 0xad9a7807 )
	ROM_LOAD16_BYTE( "kh1-ma3u.bin", 0x880000, 0x100000, 0xefeb768d )
ROM_END

ROM_START( numanatj )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "nm1_ep0l.bin", 0x080001, 0x080000, 0x4398b898 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "nm1_ep0u.bin", 0x080000, 0x080000, 0xbe90aa79 )
	ROM_LOAD16_BYTE( "nm1_ep1l.bin", 0x180001, 0x080000, 0x4581dcb4 )
	ROM_LOAD16_BYTE( "nm1_ep1u.bin", 0x180000, 0x080000, 0x30cd589a )

	ROM_LOAD16_BYTE( "nm1_ma0l.bin", 0x280001, 0x100000, 0x20faaa57 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "nm1_ma0u.bin", 0x280000, 0x100000, 0xed7c37f2 )
	ROM_LOAD16_BYTE( "nm1_ma1l.bin", 0x480001, 0x100000, 0x2232e3b4 )
	ROM_LOAD16_BYTE( "nm1_ma1u.bin", 0x480000, 0x100000, 0x6cc9675c )
	ROM_LOAD16_BYTE( "nm1_ma2l.bin", 0x680001, 0x100000, 0x208abb39 )
	ROM_LOAD16_BYTE( "nm1_ma2u.bin", 0x680000, 0x100000, 0x03a3f204 )
	ROM_LOAD16_BYTE( "nm1_ma3l.bin", 0x880001, 0x100000, 0x42a539e9 )
	ROM_LOAD16_BYTE( "nm1_ma3u.bin", 0x880000, 0x100000, 0xf79e2112 )
ROM_END

ROM_START( numanath )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "nm2_ep0l.bin", 0x080001, 0x080000, 0xf24414bb ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "nm2_ep0u.bin", 0x080000, 0x080000, 0x25c41616 )
	ROM_LOAD16_BYTE( "nm1_ep1l.bin", 0x180001, 0x080000, 0x4581dcb4 )
	ROM_LOAD16_BYTE( "nm1_ep1u.bin", 0x180000, 0x080000, 0x30cd589a )

	ROM_LOAD16_BYTE( "nm1_ma0l.bin", 0x280001, 0x100000, 0x20faaa57 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "nm1_ma0u.bin", 0x280000, 0x100000, 0xed7c37f2 )
	ROM_LOAD16_BYTE( "nm1_ma1l.bin", 0x480001, 0x100000, 0x2232e3b4 )
	ROM_LOAD16_BYTE( "nm1_ma1u.bin", 0x480000, 0x100000, 0x6cc9675c )
	ROM_LOAD16_BYTE( "nm1_ma2l.bin", 0x680001, 0x100000, 0x208abb39 )
	ROM_LOAD16_BYTE( "nm1_ma2u.bin", 0x680000, 0x100000, 0x03a3f204 )
	ROM_LOAD16_BYTE( "nm1_ma3l.bin", 0x880001, 0x100000, 0x42a539e9 )
	ROM_LOAD16_BYTE( "nm1_ma3u.bin", 0x880000, 0x100000, 0xf79e2112 )
ROM_END

ROM_START( quiztou )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "qt1_ep0l.bin", 0x080001, 0x080000, 0xb680e543 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "qt1_ep0u.bin", 0x080000, 0x080000, 0x143c5e4d )
	ROM_LOAD16_BYTE( "qt1_ep1l.bin", 0x180001, 0x080000, 0x33a72242 )
	ROM_LOAD16_BYTE( "qt1_ep1u.bin", 0x180000, 0x080000, 0x69f876cb )

	ROM_LOAD16_BYTE( "qt1_ma0l.bin", 0x280001, 0x100000, 0x5597f2b9 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "qt1_ma0u.bin", 0x280000, 0x100000, 0xf0a4cb7d )
	ROM_LOAD16_BYTE( "qt1_ma1l.bin", 0x480001, 0x100000, 0x1b9ce7a6 )
	ROM_LOAD16_BYTE( "qt1_ma1u.bin", 0x480000, 0x100000, 0x58910872 )
	ROM_LOAD16_BYTE( "qt1_ma2l.bin", 0x680001, 0x100000, 0x94739917 )
	ROM_LOAD16_BYTE( "qt1_ma2u.bin", 0x680000, 0x100000, 0x6ba5b893 )
	ROM_LOAD16_BYTE( "qt1_ma3l.bin", 0x880001, 0x100000, 0xaa9dc6ff )
	ROM_LOAD16_BYTE( "qt1_ma3u.bin", 0x880000, 0x100000, 0x14a5a163 )
ROM_END

ROM_START( swcourt )
	ROM_REGION( 0x680000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sc1-ep0l.bin", 0x080001, 0x080000, 0x145111dd ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "sc1-ep0u.bin", 0x080000, 0x080000, 0xc721c138 )
	ROM_LOAD16_BYTE( "sc1-ep1l.bin", 0x180001, 0x080000, 0xfb45cf5f )
	ROM_LOAD16_BYTE( "sc1-ep1u.bin", 0x180000, 0x080000, 0x1ce07b15 )

	ROM_LOAD16_BYTE( "sc1-ma0l.bin", 0x280001, 0x100000, 0x3e531f5e ) /* 0x400000 */
	ROM_LOAD16_BYTE( "sc1-ma0u.bin", 0x280000, 0x100000, 0x31e76a45 )
	ROM_LOAD16_BYTE( "sc1-ma1l.bin", 0x480001, 0x100000, 0x8ba3a4ec )
	ROM_LOAD16_BYTE( "sc1-ma1u.bin", 0x480000, 0x100000, 0x252dc4b7 )
ROM_END

ROM_START( tinklpit )
	ROM_REGION( 0x880000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tk1-ep0l.bin", 0x080001, 0x080000, 0xfdccae42 ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "tk1-ep0u.bin", 0x080000, 0x080000, 0x62cdb48c )
	ROM_LOAD16_BYTE( "tk1-ep1l.bin", 0x180001, 0x080000, 0x7e90f104 )
	ROM_LOAD16_BYTE( "tk1-ep1u.bin", 0x180000, 0x080000, 0x9c0b70d6 )

	ROM_LOAD16_BYTE( "tk1-ma0l.bin", 0x280001, 0x100000, 0xc6b4e15d ) /* 0x400000 */
	ROM_LOAD16_BYTE( "tk1-ma0u.bin", 0x280000, 0x100000, 0xa3ad6f67 )
	ROM_LOAD16_BYTE( "tk1-ma1l.bin", 0x480001, 0x100000, 0x61cfb92a )
	ROM_LOAD16_BYTE( "tk1-ma1u.bin", 0x480000, 0x100000, 0x54b77816 )
	ROM_LOAD16_BYTE( "tk1-ma2l.bin", 0x680001, 0x100000, 0x087311d2 )
	ROM_LOAD16_BYTE( "tk1-ma2u.bin", 0x680000, 0x100000, 0x5ce20c2c )
ROM_END

ROM_START( fghtatck )
	ROM_REGION( 0x680000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "fa2_ep0l.bin", 0x080001, 0x080000, 0x8996db9c ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "fa2_ep0u.bin", 0x080000, 0x080000, 0x58d5e090 )
	ROM_LOAD16_BYTE( "fa1_ep1l.bin", 0x180001, 0x080000, 0xb23a5b01 )
	ROM_LOAD16_BYTE( "fa1_ep1u.bin", 0x180000, 0x080000, 0xde2eb129 )

	ROM_LOAD16_BYTE( "fa1_ma0l.bin", 0x280001, 0x100000, 0xa0a95e54 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "fa1_ma0u.bin", 0x280000, 0x100000, 0x1d0135bd )
	ROM_LOAD16_BYTE( "fa1_ma1l.bin", 0x480001, 0x100000, 0xc4adf0a2 )
	ROM_LOAD16_BYTE( "fa1_ma1u.bin", 0x480000, 0x100000, 0x900297be )
ROM_END

ROM_START( fa )
	ROM_REGION( 0x680000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "fa1_ep0l.bin", 0x080001, 0x080000, 0x182eee5c ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "fa1_ep0u.bin", 0x080000, 0x080000, 0x7ea7830e )
	ROM_LOAD16_BYTE( "fa1_ep1l.bin", 0x180001, 0x080000, 0xb23a5b01 )
	ROM_LOAD16_BYTE( "fa1_ep1u.bin", 0x180000, 0x080000, 0xde2eb129 )

	ROM_LOAD16_BYTE( "fa1_ma0l.bin", 0x280001, 0x100000, 0xa0a95e54 ) /* 0x400000 */
	ROM_LOAD16_BYTE( "fa1_ma0u.bin", 0x280000, 0x100000, 0x1d0135bd )
	ROM_LOAD16_BYTE( "fa1_ma1l.bin", 0x480001, 0x100000, 0xc4adf0a2 )
	ROM_LOAD16_BYTE( "fa1_ma1u.bin", 0x480000, 0x100000, 0x900297be )
ROM_END

GAMEX( 1992,bkrtmaq,  0,        namcona1w, namcona1_quiz,	bkrtmaq,  ROT0, "Namco", "Bakuretsu Quiz Ma-Q Dai Bouken (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,cgangpzl, 0,        namcona1w, namcona1_joy,	cgangpzl, ROT0, "Namco", "Cosmo Gang the Puzzle (US)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,cgangpzj, cgangpzl, namcona1w, namcona1_joy,	cgangpzl, ROT0, "Namco", "Cosmo Gang the Puzzle (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,exvania,  0,        namcona1,  namcona1_joy,	exbania,  ROT0, "Namco", "Exvania (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,fghtatck, 0,        namcona1,  namcona1_joy,	fa,       ROT90,"Namco", "Fighter & Attacker (US)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,fa,       fghtatck, namcona1,  namcona1_joy,	fa,       ROT90,"Namco", "F/A (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,knckhead, 0,        namcona1,  namcona1_joy,	knckhead, ROT0, "Namco", "Knuckle Heads (World)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,knckhedj, knckhead, namcona1,  namcona1_joy,	knckhead, ROT0, "Namco", "Knuckle Heads (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,swcourt,  0,        namcona1w, namcona1_joy,	swcourt,  ROT0, "Namco", "Super World Court (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,emeralda, 0,        namcona1w, namcona1_joy,	emeralda, ROT0, "Namco", "Emeraldia (Japan Version B)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,emerldaa, emeralda, namcona1w, namcona1_joy,	emeralda, ROT0, "Namco", "Emeraldia (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,numanath, 0,        namcona1,  namcona1_joy,	numanath, ROT0, "Namco", "Numan Athletics (World)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,numanatj, numanath, namcona1,  namcona1_joy,	numanath, ROT0, "Namco", "Numan Athletics (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,quiztou,  0,        namcona1w, namcona1_quiz,	quiztou,  ROT0, "Namco", "Nettou! Gekitou! Quiztou!! (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,tinklpit, 0,        namcona1w, namcona1_joy,	tinklpit, ROT0, "Namco", "Tinkle Pit (Japan)", GAME_IMPERFECT_SOUND )

