/**************************************************************************************

    Sente Super System

    Preliminary driver by Mariusz Wojcieszek

**************************************************************************************/


#include "driver.h"
#include "includes/amiga.h"
#include "vidhrdw/generic.h"

UINT16 *qchip_ram;

static READ16_HANDLER( io_handler2 )
{
	return 0xffff; /* to turn service switch off */
}

static READ16_HANDLER(unknown_r)
{
	return 0;
}

static ADDRESS_MAP_START(readmem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x07ffff) AM_READ( MRA16_BANK1 ) AM_REGION(REGION_CPU1, 0) /* Chip Ram - 512k or System ROM mirror*/
	AM_RANGE( 0x200000, 0x203fff) AM_READ(MRA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE( &generic_nvram_size )
	AM_RANGE( 0x204000, 0x2041ff) AM_READ(MRA16_RAM) AM_BASE(&qchip_ram)
	AM_RANGE( 0x2100c0, 0x2100c1) AM_READ( unknown_r )
	AM_RANGE( 0x282000, 0x282003) AM_READ( io_handler2 )
	AM_RANGE( 0x286002, 0x286003) AM_READ( input_port_2_word_r )
	AM_RANGE( 0x300000, 0x3bffff) AM_ROM AM_SHARE(1)
	AM_RANGE( 0x742030, 0x74203f) AM_READ( unknown_r )
	AM_RANGE( 0xbfd000, 0xbfefff) AM_READ( amiga_cia_r )        /* 8510's CIA A and CIA B */
	AM_RANGE( 0xdbf000, 0xdfffff) AM_READ( amiga_custom_r )     /* Custom Chips */
	AM_RANGE( 0xf00000, 0xfbffff) AM_ROM AM_REGION( REGION_USER2, 0 ) AM_SHARE(1)
	AM_RANGE( 0xfc0000, 0xffffff) AM_ROM AM_REGION( REGION_USER1, 0 ) /* System ROM - mirror */
ADDRESS_MAP_END

static ADDRESS_MAP_START(writemem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x07ffff) AM_WRITE( MWA16_ROM ) /* Chip Ram - 512k or System ROM mirror*/
	AM_RANGE( 0x200000, 0x203fff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE( &generic_nvram_size )
	AM_RANGE( 0x204000, 0x2041ff) AM_WRITE(MWA16_RAM) AM_BASE(&qchip_ram)
	AM_RANGE( 0xbfd000, 0xbfefff) AM_WRITE( amiga_cia_w )        /* 8510's CIA A and CIA B */
	AM_RANGE( 0xdbf000, 0xdfffff) AM_WRITE( amiga_custom_w )     /* Custom Chips */
	AM_RANGE( 0xf00000, 0xfbffff) AM_WRITE( MWA16_ROM )
	AM_RANGE( 0xfc0000, 0xffffff) AM_WRITE( MWA16_ROM )            /* System ROM */
ADDRESS_MAP_END

INPUT_PORTS_START( mquake )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) /* Joystick - Port 1 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_COCKTAIL /* Joystick - Port 2 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static gfx_decode gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( mquake )
	/* basic machine hardware */
	MDRV_CPU_ADD( M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(amiga_irq, 262)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( amiga )
	MDRV_NVRAM_HANDLER(generic_0fill)


    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(456, 262)
	MDRV_VISIBLE_AREA(120, 456-1, 32, 262-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(4096)
	MDRV_PALETTE_INIT( amiga )

	MDRV_VIDEO_START( amiga )
	MDRV_VIDEO_UPDATE( generic_bitmapped )
MACHINE_DRIVER_END

ROM_START( mquake )
	ROM_REGION(0x80000, REGION_CPU1, 0 ) /* for ram, etc */

	ROM_REGION(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP( "kick12.rom", 0x000000, 0x40000, CRC(a6ce1636) SHA1(11f9e62cf299f72184835b7b2a70a16333fc0d88) )
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x040000 )

	ROM_REGION(0xc0000, REGION_USER2, 0)
	ROM_LOAD16_BYTE( "rom0l.bin",    0x00000, 0x10000, CRC(60c35ec3) SHA1(84fe88af54903cbd46044ef52bb50e8f94a94dcd) )
	ROM_LOAD16_BYTE( "rom0h.bin",    0x00001, 0x10000, CRC(11551a68) SHA1(bc17e748cc7a4a547de230431ea08f0355c0eec8) )
	ROM_LOAD16_BYTE( "rom1l.bin",    0x20000, 0x10000, CRC(0128c423) SHA1(b0465069452bd11b67c9a2f2b9021c91788bedbb) )
	ROM_LOAD16_BYTE( "rom1h.bin",    0x20001, 0x10000, CRC(95119e65) SHA1(29f3c32ca110c9687f38fd03ccb979c1e7c7a87e) )
	ROM_LOAD16_BYTE( "rom2l.bin",    0x40000, 0x10000, CRC(f8b8624a) SHA1(cb769581f78882a950be418dd4b35bbb6fd78a34) )
	ROM_LOAD16_BYTE( "rom2h.bin",    0x40001, 0x10000, CRC(46e36e0d) SHA1(0813430137a31d5af2cadbd712a418e9ff339a21) )
	ROM_LOAD16_BYTE( "rom3l.bin",    0x60000, 0x10000, CRC(c00411a2) SHA1(960d3539914f587c2186ec6eefb81b3cdd9325a0) )
	ROM_LOAD16_BYTE( "rom3h.bin",    0x60001, 0x10000, CRC(4540c681) SHA1(cb0bc6dc506ed0c9561687964e57299a472c5cd8) )
	ROM_LOAD16_BYTE( "rom4l.bin",    0x80000, 0x10000, CRC(f48d0730) SHA1(703a8ed47f64b3824bc6e5a4c5bdb2895f8c3d37) )
	ROM_LOAD16_BYTE( "rom4h.bin",    0x80001, 0x10000, CRC(eee39fec) SHA1(713e24fa5f4ba0a8bc7bf67ed2d9e079fd3aa5d6) )
	ROM_LOAD16_BYTE( "rom5l.bin",    0xa0000, 0x10000, CRC(7b6ec532) SHA1(e19005269673134431eb55053d650f747f614b89) )
	ROM_LOAD16_BYTE( "rom5h.bin",    0xa0001, 0x10000, CRC(ed8ec9b7) SHA1(510416bc88382e7a548635dcba53a2b615272e0f) )

	ROM_REGION(0x040000, REGION_SOUND1, 0)
	ROM_LOAD( "qrom0.bin",    0x000000, 0x010000, CRC(753e29b4) SHA1(4c7ccff02d310c7c669aa170e8efb6f2cb996432) )
	ROM_LOAD( "qrom1.bin",    0x010000, 0x010000, CRC(e9e15629) SHA1(a0aa60357a13703f69a2a13e83f2187c9a1f63c1) )
	ROM_LOAD( "qrom2.bin",    0x020000, 0x010000, CRC(837294f7) SHA1(99e383998105a63896096629a51b3a0e9eb16b17) )
	ROM_LOAD( "qrom3.bin",    0x030000, 0x010000, CRC(530fd1a9) SHA1(e3e5969f0880de0a6cdb443a82b85d34ab8ff4f8) )
ROM_END

/***************************************************************************

                    Moonquake machine interface

***************************************************************************/

static int mquake_cia_0_portA_r( void )
{
	int ret = readinputport( 0 ) & 0xc0;
	ret |= 0x3f;
	return ret; /* Gameport 1 and 0 buttons */
}

static int mquake_cia_0_portB_r( void )
{
	/* parallel port */
	return 0;
}

static void mquake_cia_0_portA_w( int data )
{
	if ( (data & 1) == 1)
	{
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MRA16_BANK1 );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_ROM );
	}
	else if ( ((data & 1) == 0))
	{
		/* overlay disabled, map RAM on 0x000000 */
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MRA16_RAM );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_RAM );
	}

}

static void mquake_cia_0_portB_w( int data )
{
	/* parallel port */
}

static UINT16 mquake_read_joy0dat(void)
{
	int input = ( readinputport( 1 ) >> 4 );
	int	top,bot,lft,rgt;

	top = ( input >> 3 ) & 1;
	bot = ( input >> 2 ) & 1;
	lft = ( input >> 1 ) & 1;
	rgt = input & 1;

	if ( lft ) top ^= 1;
	if ( rgt ) bot ^= 1;

	return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
}

static UINT16 mquake_read_joy1dat(void)
{
	int input = ( readinputport( 1 ) & 0x0f );
	int	top,bot,lft,rgt;

	top = ( input >> 3 ) & 1;
	bot = ( input >> 2 ) & 1;
	lft = ( input >> 1 ) & 1;
	rgt = input & 1;

	if ( lft ) top ^= 1;
	if ( rgt ) bot ^= 1;

	return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
}

/***************************************************************************

                    Driver init

***************************************************************************/

static const struct amiga_machine_interface mquake_intf =
{
	mquake_cia_0_portA_r,
	mquake_cia_0_portB_r,
	mquake_cia_0_portA_w,
	mquake_cia_0_portB_w,
	NULL,
	NULL,
	NULL,
	NULL,
	mquake_read_joy0dat,
	mquake_read_joy1dat,
	NULL,
	NULL,
	NULL,
	NULL
};

DRIVER_INIT(mquake)
{
	amiga_machine_config(&mquake_intf);

	/* set up memory */
	memory_set_bankptr(1, memory_region(REGION_USER1));

}

GAME( 1987, mquake, 0, mquake, mquake, mquake, 0, "Sente", "Moonquake", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
