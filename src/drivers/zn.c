/***************************************************************************

  Sony ZN1/ZN2 - Arcade PSX Hardware
  ==================================
  Driver by smf

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/mips/mips.h"
#include "cpu/z80/z80.h"

/*
  Capcom qsound games

  Based on information from:
   The cps1/cps2 qsound driver,
   Miguel Angel Horna
   Amuse.

  The main board is made by sony, capcom included the qsound on the game
  board. None of these have a bios dumped yet so only the music can be
  played for now.

  The qsound hardware is different to cps2 as it uses an i/o port and
  nmi's instead of shared memory. The driver uses 8bit i/o addresses
  but the real ZN1 hardware may use 16bit i/o addresses as the code
  always accesses port 0xf100. The ZN2 code seems to vary the top
  eight bits of the address ( which may or may not be important ).
*/

static int qcode;
static int qcode_last;
static int queue_data;
static int queue_len;

WRITE_HANDLER( qsound_queue_w )
{
	if( cpu_getstatus( 1 ) != 0 )
	{
		queue_data = data;
		queue_len = 2;
	}
}

static VIDEO_START( znqs )
{
	return 0;
}

static VIDEO_UPDATE( znqs )
{
	int refresh = 0;

	if( queue_len == 0 )
	{
		if( keyboard_pressed_memory( KEYCODE_UP ) )
		{
			qcode=( qcode & 0xff00 ) | ( ( qcode + 0x0001 ) & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_DOWN ) )
		{
			qcode=( qcode & 0xff00 ) | ( ( qcode - 0x0001 ) & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_RIGHT ) )
		{
			qcode=( ( qcode + 0x0100 ) & 0xff00 ) | ( qcode & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_LEFT ) )
		{
			qcode=( ( qcode - 0x0100 ) & 0xff00 ) | ( qcode & 0xff );
		}
		if( qcode != qcode_last )
		{
			qsound_queue_w( 0, qcode );
			qcode_last = qcode;
			refresh = 1;
		}
	}

	if( refresh )
	{
		struct DisplayText dt[ 4 ];
		char text1[ 256 ];
		char text2[ 256 ];
		char text3[ 256 ];

		strcpy( text1, Machine->gamedrv->description );
		if( strlen( text1 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text1[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text2, "QSOUND CODE=%02x/%02x", qcode >> 8, qcode & 0xff );
		if( strlen( text2 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text2[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		strcpy( text3, "SELECT WITH RIGHT&LEFT/UP&DN" );
		if( strlen( text3 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text3[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		dt[ 0 ].text = text1;
		dt[ 0 ].color = UI_COLOR_NORMAL;
		dt[ 0 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 0 ].text ) ) / 2;
		dt[ 0 ].y = Machine->uiheight - Machine->uifontheight * 5;
		dt[ 1 ].text = text2;
		dt[ 1 ].color = UI_COLOR_NORMAL;
		dt[ 1 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 1 ].text ) ) / 2;
		dt[ 1 ].y = Machine->uiheight - Machine->uifontheight * 3;
		dt[ 2 ].text = text3;
		dt[ 2 ].color = UI_COLOR_NORMAL;
		dt[ 2 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 2 ].text ) ) / 2;
		dt[ 2 ].y = Machine->uiheight - Machine->uifontheight * 1;
		dt[ 3 ].text = 0; /* terminate array */
		displaytext( Machine->scrbitmap, dt );
	}
}

static struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

static WRITE_HANDLER( qsound_banksw_w )
{
	unsigned char *RAM = memory_region( REGION_CPU2 );
	if( ( data & 0xf0 ) != 0 )
	{
		logerror( "%08x: qsound_banksw_w( %02x )\n", activecpu_get_pc(), data & 0xff );
	}
	cpu_setbank( 1, &RAM[ 0x10000 + ( ( data & 0x0f ) * 0x4000 ) ] );
}

static MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },	/* banked (contains music data) */
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_banksw_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( qsound_readport )
	{ 0x00, 0x00, soundlatch_r },
PORT_END

static struct GfxDecodeInfo znqs_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static MEMORY_READ16_START( znqs_readmem )
	{ 0xbfc00000, 0xbfc7ffff, MRA16_BANK3 },	/* bios */
MEMORY_END

static MEMORY_WRITE16_START( znqs_writemem )
MEMORY_END


static INTERRUPT_GEN( qsound_interrupt )
{
	if( queue_len == 2 )
	{
		soundlatch_w( 0, queue_data >> 8 );
		queue_len--;
		cpu_set_irq_line(cpu_getactivecpu(), IRQ_LINE_NMI, PULSE_LINE);
	}
	else if( queue_len == 1 )
	{
		soundlatch_w( 0, queue_data & 0xff );
		queue_len--;
		cpu_set_irq_line(cpu_getactivecpu(), IRQ_LINE_NMI, PULSE_LINE);
	}
	else
		cpu_set_irq_line(cpu_getactivecpu(), 0, HOLD_LINE);
}

static MACHINE_INIT( znqs )
{
	/* stop CPU1 as it doesn't do anything useful yet. */
	timer_suspendcpu( 0, 1, SUSPEND_REASON_DISABLE );
	/* but give it some memory so it can reset. */
	cpu_setbank( 3, memory_region( REGION_USER1 ) );

	if( strcmp( Machine->gamedrv->name, "sfex2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2p" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tgmj" ) == 0 )
	{
		qcode = 0x0400;
	}
	else if( strcmp( Machine->gamedrv->name, "kikaioh" ) == 0 )
	{
		qcode = 0x8000;
	}
	else
	{
		qcode = 0x0000;
	}
	qcode_last = -1;
	queue_len = 0;
	qsound_banksw_w( 0, 0 );
}

static MACHINE_DRIVER_START( znqs )

	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33000000) /* 33MHz ?? */
	MDRV_CPU_MEMORY(znqs_readmem,znqs_writemem)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)  /* 8MHz ?? */
	MDRV_CPU_MEMORY(qsound_readmem,qsound_writemem)
	MDRV_CPU_PORTS(qsound_readport,0)
	MDRV_CPU_VBLANK_INT(qsound_interrupt,4) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(znqs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x30*8+32*2, 0x1c*8+32*3)
	MDRV_VISIBLE_AREA(32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1)
	MDRV_GFXDECODE(znqs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(znqs)
	MDRV_VIDEO_UPDATE(znqs)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(QSOUND, qsound_interface)
MACHINE_DRIVER_END


INPUT_PORTS_START( zn )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

ROM_START( rvschool )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "jst-04a", 0x0000000, 0x080000, CRC(034b1011) SHA1(6773246be242ee336503d21d7d44a3884832eb1e) )
	ROM_LOAD16_WORD( "jst-05m", 0x0080000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD16_WORD( "jst-06m", 0x0480000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD16_WORD( "jst-07m", 0x0880000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD16_WORD( "jst-08m", 0x0c80000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD16_WORD( "jst-09m", 0x1080000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD16_WORD( "jst-10m", 0x1480000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD16_WORD( "jst-11m", 0x1880000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD16_WORD( "jst-12m", 0x1c80000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD16_WORD( "jst-13m", 0x2080000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( jgakuen )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "jst-04j", 0x0000000, 0x080000, CRC(28b8000a) SHA1(9ebf74b453d775cadca9c2d7d8e2c7eb57bb9a38) )
	ROM_LOAD16_WORD( "jst-05m", 0x0080000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD16_WORD( "jst-06m", 0x0480000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD16_WORD( "jst-07m", 0x0880000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD16_WORD( "jst-08m", 0x0c80000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD16_WORD( "jst-09m", 0x1080000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD16_WORD( "jst-10m", 0x1480000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD16_WORD( "jst-11m", 0x1880000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD16_WORD( "jst-12m", 0x1c80000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD16_WORD( "jst-13m", 0x2080000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( kikaioh )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x3080000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "kioj-04.bin", 0x0000000, 0x080000, CRC(3a2a3bc8) SHA1(3c4ae3cfe00a7f60ab2196ae042dab4a8eb6f597) )
	ROM_LOAD16_WORD( "kio-05m.bin", 0x0080000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD16_WORD( "kio-06m.bin", 0x0880000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD16_WORD( "kio-07m.bin", 0x1080000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD16_WORD( "kio-08m.bin", 0x1880000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD16_WORD( "kio-09m.bin", 0x2080000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD16_WORD( "kio-10m.bin", 0x2880000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )
ROM_END

ROM_START( sfex )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfe-04a", 0x0000000, 0x080000, CRC(08247bd4) SHA1(07f356ef2827b3fbd0bfaf2010915315d9d60ef1) )
	ROM_LOAD16_WORD( "sfe-05m", 0x0080000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD16_WORD( "sfe-06m", 0x0480000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD16_WORD( "sfe-07m", 0x0880000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD16_WORD( "sfe-08m", 0x0c80000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD16_WORD( "sfe-09m", 0x1080000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD16_WORD( "sfe-10m", 0x1480000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfe-04j", 0x0000000, 0x080000, CRC(ea100607) SHA1(27ef8c619804999d32d14fcc5ec783c057b4dc73) )
	ROM_LOAD16_WORD( "sfe-05m", 0x0080000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD16_WORD( "sfe-06m", 0x0480000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD16_WORD( "sfe-07m", 0x0880000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD16_WORD( "sfe-08m", 0x0c80000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD16_WORD( "sfe-09m", 0x1080000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD16_WORD( "sfe-10m", 0x1480000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexp )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfp-04e", 0x0000000, 0x080000, CRC(305e4ec0) SHA1(0df9572d7fc1bbc7131483960771d016fa5487a5) )
	ROM_LOAD16_WORD( "sfp-05",  0x0080000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD16_WORD( "sfp-06",  0x0480000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD16_WORD( "sfp-07",  0x0880000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD16_WORD( "sfp-08",  0x0c80000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD16_WORD( "sfp-09",  0x1080000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD16_WORD( "sfp-10",  0x1480000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexpj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfp-04j", 0x0000000, 0x080000, CRC(18d043f5) SHA1(9e6e24a722d13888fbfd391ddb1a5045b162488c) )
	ROM_LOAD16_WORD( "sfp-05",  0x0080000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD16_WORD( "sfp-06",  0x0480000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD16_WORD( "sfp-07",  0x0880000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD16_WORD( "sfp-08",  0x0c80000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD16_WORD( "sfp-09",  0x1080000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD16_WORD( "sfp-10",  0x1480000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfex2 )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ex2j-04", 0x0000000, 0x080000, CRC(5d603586) SHA1(ff546d3bd011d6441e9672b88bab763d3cd89be2) )
	ROM_LOAD16_WORD( "ex2-05m", 0x0080000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD16_WORD( "ex2-06m", 0x0880000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD16_WORD( "ex2-07m", 0x1080000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD16_WORD( "ex2-08m", 0x1880000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD16_WORD( "ex2-09m", 0x2080000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2-02",  0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( sfex2p )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x3080000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sf2p-04", 0x0000000, 0x080000, CRC(c6d0aea3) SHA1(f48ee889dd743109f830063da3eb0f687db2d86c) )
	ROM_LOAD16_WORD( "sf2p-05", 0x0080000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD16_WORD( "sf2p-06", 0x0880000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD16_WORD( "sf2p-07", 0x1080000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD16_WORD( "sf2p-08", 0x1880000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD16_WORD( "sf2p-09", 0x2080000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD16_WORD( "sf2p-10", 0x2880000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2p-02", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sf2p-03", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( shiryu2 )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2c80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "hr2j-04.bin", 0x0000000, 0x080000, CRC(0824ee5f) SHA1(a296ffe03f0d947deb9803d05de3c240a26b52bb) )
	ROM_LOAD16_WORD( "hr2-05m.bin", 0x0080000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD16_WORD( "hr2-06m.bin", 0x0880000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD16_WORD( "hr2-07m.bin", 0x1080000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD16_WORD( "hr2-08m.bin", 0x1880000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD16_WORD( "hr2-09m.bin", 0x2080000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD16_WORD( "hr2-10m.bin", 0x2880000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END

ROM_START( tgmj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x0880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ate-04j", 0x0000000, 0x080000, CRC(bb4bbb96) SHA1(808f4b29493e74efd661d561d11cbec2f4afd1c8) )
	ROM_LOAD16_WORD( "ate-05",  0x0080000, 0x400000, CRC(50977f5a) SHA1(78c2b1965957ff1756c25b76e549f11fc0001153) )
	ROM_LOAD16_WORD( "ate-06",  0x0480000, 0x400000, CRC(05973f16) SHA1(c9262e8de14c4a9489f7050316012913c1caf0ff) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ate-02",  0x00000, 0x08000, CRC(f4f6e82f) SHA1(ad6c49197a60f456367c9f78353741fb847819a1) )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ate-01",  0x0000000, 0x400000, CRC(a21c6521) SHA1(560e4855f6e00def5277bdd12064b49e55c3b46b) )
ROM_END

ROM_START( ts2j )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x0e80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ts2j-04", 0x0000000, 0x080000, CRC(4aba8c5e) SHA1(a56001bf50bfc1b03036e88ae1febd1aac8c63c0) )
	ROM_LOAD16_WORD( "ts2-05",  0x0080000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD16_WORD( "ts2-06m", 0x0480000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD16_WORD( "ts2-08m", 0x0880000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD16_WORD( "ts2-10",  0x0c80000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )
ROM_END


/*
  Other ZN1 games

  These have had their bios dumped, but they all fail the self test on
  what appears to be protection hardware & the screen turns red
  ( just enough of the psx chipset is emulated to display this ).

  There is an another romset for Gallop Racer 2 with link hw (z80-a, osc 32.0000MHz,
  upd72103agc & lh540202u x2) which has different versions of rom 119 & 120.
*/

#define VRAM_SIZE ( 1024L * 512L * 2L )

static unsigned short *m_p_vram;
static UINT32 m_p_gpu_buffer[ 16 ];
static unsigned char m_n_gpu_buffer_offset;

static VIDEO_START( zn )
{
	m_p_vram = auto_malloc( VRAM_SIZE );
	if( m_p_vram == NULL )
		return 1;
	return 0;
}

PALETTE_INIT( zn )
{
	UINT16 n_r;
	UINT16 n_g;
	UINT16 n_b;
	UINT32 n_colour;

	for( n_colour = 0; n_colour < 0x10000; n_colour++ )
	{
		n_r = n_colour & 0x1f;
		n_g = ( n_colour >> 5 ) & 0x1f;
		n_b = ( n_colour >> 10 ) & 0x1f;

		n_r = ( n_r * 0xff ) / 0x1f;
		n_g = ( n_g * 0xff ) / 0x1f;
		n_b = ( n_b * 0xff ) / 0x1f;

		palette_set_color(n_colour, n_r, n_g, n_b);
	}
}

static VIDEO_UPDATE( zn )
{
	UINT16 n_x;
	UINT16 n_y;
	pen_t *pens = Machine->pens;

	if( bitmap->depth == 16 )
	{
		UINT16 *p_line;
		for( n_y = 0; n_y < bitmap->height; n_y++ )
		{
			p_line = (UINT16 *)bitmap->line[ n_y ];
			for( n_x = 0; n_x < bitmap->width; n_x++ )
			{
				*( p_line++ ) = pens[ m_p_vram[ ( n_y * 1024 ) + n_x ] ];
			}
		}
	}
	else
	{
		UINT8 *p_line;
		for( n_y = 0; n_y < bitmap->height; n_y++ )
		{
			p_line = (UINT8 *)bitmap->line[ n_y ];
			for( n_x = 0; n_x < bitmap->width; n_x++ )
			{
				*( p_line++ ) = pens[ m_p_vram[ ( n_y * 1024 ) + n_x ] ];
			}
		}
	}
}

static void triangle( UINT32 n_x1, UINT32 n_y1, UINT32 n_x2, UINT32 n_y2, UINT32 n_x3, UINT32 n_y3, UINT32 n_colour )
{
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_mx;
	UINT32 n_my;
	UINT32 n_cx1;
	UINT32 n_cx2;
	INT32 n_dx1;
	INT32 n_dx2;

	n_colour = ( ( n_colour >> 3 ) & 0x1f ) |
		( ( ( n_colour >> 11 ) & 0x1f ) << 5 ) |
		( ( ( n_colour >> 19 ) & 0x1f ) << 10 );

	n_cx1 = n_x1 << 16;
	n_cx2 = n_x1 << 16;
	if( n_y1 < n_y3 )
	{
		n_dx1 = (INT32)( ( n_x3 << 16 ) - n_cx1 ) / (INT32)( n_y3 - n_y1 );
	}
	else
	{
		n_dx1 = 0;
	}
	if( n_y1 < n_y2 )
	{
		n_dx2 = (INT32)( ( n_x2 << 16 ) - n_cx2 ) / (INT32)( n_y2 - n_y1 );
	}
	else
	{
		n_dx2 = 0;
	}

	n_my = n_y2;
	if( n_my < n_y3 )
	{
		n_my = n_y3;
	}
	if( n_my > 512 )
	{
		n_my = 512;
	}
	n_y = n_y1;
	while( n_y < n_my )
	{
		if( n_y == n_y2 )
		{
			n_cx2 = n_x2 << 16;
			n_dx2 = (INT32)( n_cx2 - ( n_x3 << 16 ) ) / (INT32)( n_y2 - n_y3 );
		}
		if( n_y == n_y3 )
		{
			n_cx1 = n_x3 << 16;
			n_dx1 = (INT32)( n_cx1 - ( n_x2 << 16 ) ) / (INT32)( n_y3 - n_y2 );
		}
		n_mx = ( n_cx2 >> 16 );
		if( n_mx > 1024 )
		{
			n_mx = 1024;
		}
		n_x = ( n_cx1 >> 16 );
		while( n_x < n_mx )
		{
			m_p_vram[ n_y * 1024 + n_x ] = n_colour;
			n_x++;
		}
		n_cx1 += n_dx1;
		n_cx2 += n_dx2;
		n_y++;
	}
	set_vh_global_attribute(NULL,0);
}

void gpu32_w( UINT32 offset, UINT32 data )
{
	switch( offset )
	{
	case 0x00:
		m_p_gpu_buffer[ m_n_gpu_buffer_offset ] = data;
		switch( m_p_gpu_buffer[ 0 ] >> 24 )
		{
		case 0x28:
			if( m_n_gpu_buffer_offset < 4 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				triangle( m_p_gpu_buffer[ 1 ] & 0xffff, m_p_gpu_buffer[ 1 ] >> 16, m_p_gpu_buffer[ 2 ] & 0xffff, m_p_gpu_buffer[ 2 ] >> 16, m_p_gpu_buffer[ 3 ] & 0xffff, m_p_gpu_buffer[ 3 ] >> 16, m_p_gpu_buffer[ 0 ] & 0xffffff );
				triangle( m_p_gpu_buffer[ 2 ] & 0xffff, m_p_gpu_buffer[ 2 ] >> 16, m_p_gpu_buffer[ 4 ] & 0xffff, m_p_gpu_buffer[ 4 ] >> 16, m_p_gpu_buffer[ 3 ] & 0xffff, m_p_gpu_buffer[ 3 ] >> 16, m_p_gpu_buffer[ 0 ] & 0xffffff );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		}
		break;
	}
}

UINT32 gpu32_r( UINT32 offset )
{
	switch( offset )
	{
	case 0x04:
		return 0x14802000;
	}
	return 0;
}

/*

Simple 16 to 32bit bridge.

Only one register can be written to / read per instruction.
Writes occur after the program counter has been incremented.
Transfers are always 32bits no matter what the cpu reads / writes.

*/

static UINT32 m_n_bridge_data;
void *m_p_bridge_timer_w, *m_p_bridge_timer_r;
void ( *m_p_bridge32_w )( UINT32 offset, UINT32 data );

static void bridge_w_flush( int offset )
{
	m_p_bridge32_w( offset, m_n_bridge_data );
}

void bridge_w( void ( *p_bridge32_w )( UINT32 offset, UINT32 data ), UINT32 offset, UINT32 data )
{
	if( ( offset % 4 ) != 0 )
	{
		m_n_bridge_data = ( m_n_bridge_data & 0x0000ffff ) | ( data << 16 );
	}
	else
	{
		m_n_bridge_data = ( m_n_bridge_data & 0xffff0000 ) | ( data & 0xffff );
	}

	timer_adjust(m_p_bridge_timer_w, TIME_NOW, offset & ~3, 0 );
}

static void bridge_r_flush( int offset )
{
}

UINT16 bridge_r( UINT32 ( *p_bridge32_r )( UINT32 offset ), UINT32 offset )
{
	timer_adjust(m_p_bridge_timer_r, TIME_NOW, 0, 0 );
	m_n_bridge_data = p_bridge32_r( offset & ~3 );

	if( ( offset % 4 ) != 0 )
	{
		return m_n_bridge_data >> 16;
	}
	return m_n_bridge_data & 0xffff;
}

WRITE16_HANDLER( gpu_w )
{
	bridge_w( gpu32_w, offset<<1, data );
}

READ16_HANDLER( gpu_r )
{
	return bridge_r( gpu32_r, offset<<1 );
}

static MEMORY_READ16_START( zn_readmem )
	{ 0x00000000, 0x001fffff, MRA16_RAM },	/* ram */
	{ 0x1f801810, 0x1f801817, gpu_r },
	{ 0xa0000000, 0xa01fffff, MRA16_BANK1 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA16_BANK2 },	/* bios */
MEMORY_END

static MEMORY_WRITE16_START( zn_writemem )
	{ 0x00000000, 0x001fffff, MWA16_RAM },	/* ram */
	{ 0x1f801810, 0x1f801817, gpu_w },
	{ 0xa0000000, 0xa01fffff, MWA16_BANK1 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA16_ROM },	/* bios */
MEMORY_END

static MACHINE_INIT( zn )
{
	cpu_setbank( 1, memory_region( REGION_CPU1 ) );
	cpu_setbank( 2, memory_region( REGION_USER1 ) );

	m_n_gpu_buffer_offset = 0;
	memset( m_p_vram, 0x00, VRAM_SIZE );

	m_p_bridge_timer_w = timer_alloc(bridge_w_flush);
	m_p_bridge_timer_r = timer_alloc(bridge_r_flush);
}

static MACHINE_DRIVER_START( zn )

	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33868800) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(zn)
	MDRV_VIDEO_START(zn)
	MDRV_VIDEO_UPDATE(zn)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

ROM_START( doapp )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "mg-bios.bin",  0x0000000, 0x080000, CRC(69ffbcb4) SHA1(03eb2febfab3fcde716defff291babd9392de965) )

	ROM_REGION16_LE( 0x01a00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "doapp119.bin", 0x0000001, 0x100000, CRC(bbe04cef) SHA1(f2dae4810ca78075fc3007a6001531a455235a2e) )
	ROM_LOAD16_BYTE( "doapp120.bin", 0x0000000, 0x100000, CRC(b614d7e6) SHA1(373756d9b88b45c677e987ee1e5cb2d5446ecfe8) )
	ROM_LOAD16_WORD( "doapp-0.216",  0x0200000, 0x400000, CRC(acc6c539) SHA1(a744567a3d75634098b1749103307981be9acbdd) )
	ROM_LOAD16_WORD( "doapp-1.217",  0x0600000, 0x400000, CRC(14b961c4) SHA1(3fae1fcb4665ba8bad391881b26c2d087718d42f) )
	ROM_LOAD16_WORD( "doapp-2.218",  0x0a00000, 0x400000, CRC(134f698f) SHA1(6422972cf5d30a0f09f0c20f042691d5969207b4) )
	ROM_LOAD16_WORD( "doapp-3.219",  0x0e00000, 0x400000, CRC(1c6540f3) SHA1(8631fde93a1da6325d7b31c7edf12c964f0ac4fc) )
	ROM_LOAD16_WORD( "doapp-4.220",  0x1200000, 0x400000, CRC(f83bacf7) SHA1(5bd66da993f0db966581dde80dd7e5b377754412) )
	ROM_LOAD16_WORD( "doapp-5.221",  0x1600000, 0x400000, CRC(e11e8b71) SHA1(b1d1b9532b5f074ce216a603436d5674d136865d) )
ROM_END

ROM_START( glpracr2 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "mg-bios.bin",  0x0000000, 0x080000, CRC(69ffbcb4) SHA1(03eb2febfab3fcde716defff291babd9392de965) )

	ROM_REGION16_LE( 0x02200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1.119",        0x0000001, 0x100000, CRC(0fe2d2df) SHA1(031369f4e1138e2ee293c321e5ee418e560b3f06) )
	ROM_LOAD16_BYTE( "1.120",        0x0000000, 0x100000, CRC(8e3fb1c0) SHA1(2126c1e43bee7cd938e0f2a3ea841da8811223cd) )
	ROM_LOAD16_WORD( "gra2-0.217",   0x0200000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD16_WORD( "gra2-1.218",   0x0600000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD16_WORD( "gra2-2.219",   0x0a00000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD16_WORD( "gra2-3.220",   0x0e00000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD16_WORD( "gra2-4.221",   0x1200000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD16_WORD( "gra2-5.222",   0x1600000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD16_WORD( "gra2-6.223",   0x1a00000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD16_WORD( "gra2-7.323",   0x1e00000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )
ROM_END

ROM_START( sncwgltd )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "kn-bios.bin",  0x0000000, 0x080000, CRC(5ff165f3) SHA1(8f59314c1093446b9bcb06d232244da6df78e206) )

	ROM_REGION16_LE( 0x01a80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ic5.bin",      0x0000000, 0x080000, CRC(458f14aa) SHA1(b4e50be60ffb9b7911561dd35b6a7e0df3432a3a) )
	ROM_LOAD16_WORD( "ic6.bin",      0x0080000, 0x080000, CRC(8233dd1e) SHA1(1422b4530d671e3b8b471ec16c20ef7c819ab762) )
	ROM_LOAD16_WORD( "ic7.bin",      0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD16_WORD( "ic8.bin",      0x0180000, 0x080000, CRC(e8145f2b) SHA1(3a1cb189426998856dfeda47267fde64be34c6ec) )
	ROM_LOAD16_WORD( "ic9.bin",      0x0200000, 0x080000, CRC(605c9370) SHA1(9734549cae3028c089f4c9f2336ee374b3f950f8) )
	ROM_LOAD16_WORD( "ic11.bin",     0x0280000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD16_WORD( "ic12.bin",     0x0680000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD16_WORD( "ic13.bin",     0x0a80000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD16_WORD( "ic14.bin",     0x0e80000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD16_WORD( "ic15.bin",     0x1280000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD16_WORD( "ic16.bin",     0x1680000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )
ROM_END




GAME( 1995, ts2j,	  0,		znqs, zn, 0, ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (JAPAN 951124)" )
GAME( 1996, sfex,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX (ASIA 961219)" )
GAME( 1996, sfexj,	  sfex, 	znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX (JAPAN 961130)" )
GAME( 1997, sfexp,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX Plus (USA 970311)" )
GAME( 1997, sfexpj,   sfexp,	znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX Plus (JAPAN 970311)" )
GAME( 1997, rvschool, 0,		znqs, zn, 0, ROT0, "Capcom", "Rival Schools (ASIA 971117)" )
GAME( 1997, jgakuen,  rvschool, znqs, zn, 0, ROT0, "Capcom", "Justice Gakuen (JAPAN 971117)" )
GAME( 1998, sfex2,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX 2 (JAPAN 980312)" )
GAME( 1998, tgmj,	  0,		znqs, zn, 0, ROT0, "Capcom/Akira", "Tetris The Grand Master (JAPAN 980710)" )
GAME( 1998, kikaioh,  0,		znqs, zn, 0, ROT0, "Capcom", "Kikaioh (JAPAN 980914)" )
GAME( 1999, sfex2p,   0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX 2 Plus (JAPAN 990611)" )
GAME( 1999, shiryu2,  0,		znqs, zn, 0, ROT0, "Capcom", "Strider Hiryu 2 (JAPAN 991213)" )

GAMEX( 1996, sncwgltd,	0,	  zn, zn, 0, ROT0, "Video System Co.", "Sonic Wings Limited (JAPAN)", GAME_NO_SOUND )
GAMEX( 1997, glpracr2,	0,	  zn, zn, 0, ROT0, "Tecmo", "Gallop Racer 2 (JAPAN)", GAME_NO_SOUND )
GAMEX( 1998, doapp,		0,	  zn, zn, 0, ROT0, "Tecmo", "Dead Or Alive ++ (JAPAN)", GAME_NO_SOUND )
