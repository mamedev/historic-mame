/***************************************************************************

Arcadia System - (c) 1988 Arcadia Systems

Driver by Ernesto Corvi and Mariusz Wojcieszek

Games supported:
SportTime Bowling
Leader Board
Ninja Mission
Road Wars
Sidewinder
Space Ranger
SportTime Table Hockey
Spot
Magic Johnson's Fast Break
World Darts
Xenon

Other Arcadia games (not dumped):
Aaargh!
Blasta Ball
N.Y. Warriors
Pool
Rockford
World Trophy Soccer

Hardware description (from targets.mame.net):
In the late 80s, Arcadia collaborated with Mastertronic to create their own
ten-interchangeable-game arcade platform called the Arcadia Multi Select system,
using the same hardware as the beloved Commodore Amiga computer.
(In fact, the Multi Select's main PCB is an A500 motherboard, to which the ROM
cage is attached through the external expansion port).
Reportedly the system was also (or was originally) supposed to have been released
in two five-game Super Select versions--"Arcade Action" and "Sports Simulation"
-- but no specimens of these have ever been seen.

NOTES and TODO:
- To get into service mode, hold down F2 before pressing a button after
the 'INITIALIZATION OK' message. Pressing F2 during game brings service mode also.
- The coin input mechanism isnt well understood yet. There seems the be 2
pins per coin slot attached to the system. Right now im using a kludge
that doesnt work all the time.
- No audio yet.

Issues:
- ar_fast is missing score and time display, this needs non-dma sprite emulation
- ar_sdwr tries to write above chip mem, blitter accesses above chipmem are
discarded, this looks like game bug
- ar_dart has some sprite glitches on game screen
- ar_sdwr has some gfx glitches on background

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amiga.h"

/**************************************************************************

  Autoconfig support

***************************************************************************/


#define AUTOCONF_SIZE_2MEG		0x06
#define AUTOCONF_ROM_VALID		0x10
#define AUTOCONF_ZORRO_II		0xc0
#define AUTOCONF_NO_SHUTUP		0x40

#ifdef LSB_FIRST
#define WRITE_WORD(offs, data) \
	*(offs) = ((data16_t)data & 0x00ff); \
	*(offs + 1) = ((data16_t)data & 0xff00) >> 8;
#else
#define WRITE_WORD(offs, data) \
	*(offs + 1) = ((data16_t)data & 0x00ff); \
	*(offs) = ((data16_t)data & 0xff00) >> 8;
#endif

#define NORMAL( offs, data ) WRITE_WORD( offs, ( data & 0xf0 ) << 8 ); WRITE_WORD( offs+2, ( data & 0x0f ) << 12 );
#define INV( offs, data ) WRITE_WORD( offs, ~( ( data & 0xf0 ) << 8 ) ); WRITE_WORD( offs+2, ~( ( data & 0x0f ) << 12 ) );

static void autoconfig_init( UINT32 rom_boot_vector )
{

	data8_t *autoconf, *expansion;

	autoconf = (data8_t*)amiga_autoconfig_mem;
	expansion = (data8_t*)amiga_expansion_ram;

	/* setup a dummy autoconfig device */
	memset( autoconf, 0xff, 0x2000 );

	/* type */
	NORMAL( autoconf+0x00, ( AUTOCONF_ZORRO_II | AUTOCONF_SIZE_2MEG | AUTOCONF_ROM_VALID ) );
	/* product number */
	INV( autoconf+0x04, 0x01 )
	/* flags */
	INV( autoconf+0x08, AUTOCONF_NO_SHUTUP );
	/* manufacturer id */
	INV( autoconf+0x10, 0x07 )
	INV( autoconf+0x14, 0x70 )
	/* serial number */
	INV( autoconf+0x18, 0x00 );
	INV( autoconf+0x1c, 0x00 );
	INV( autoconf+0x20, 0x00 );
	INV( autoconf+0x24, 0x00 );
	/* rom offset */
	INV( autoconf+0x28, 0x10 );
	INV( autoconf+0x2c, 0x00 );
	/* status reg */
	NORMAL( autoconf+0x40, 0x00 );

	/* setup the diagnostics area */
	WRITE_WORD( autoconf+0x1000, 0x9000 );
	WRITE_WORD( autoconf+0x1002, 0x0106 );
	WRITE_WORD( autoconf+0x1004, 0x0100 );

	/* setup the rom entry points */
	WRITE_WORD( autoconf+0x1100, 0x4ef9 ); /* JMP */
	WRITE_WORD( autoconf+0x1102, (rom_boot_vector >> 16) & 0xffff);
	WRITE_WORD( autoconf+0x1104, rom_boot_vector & 0xffff);

	/* move it to what will be our remapped area */
	memcpy( expansion, autoconf, 0x1110 );
}


static ADDRESS_MAP_START(readmem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x07ffff) AM_READ( MRA16_BANK1 ) /* Chip Ram - 512k or System ROM mirror*/
	AM_RANGE( 0x200000, 0x201fff) AM_READ( MRA16_RAM ) AM_BASE(&amiga_expansion_ram)
	AM_RANGE( 0x800000, 0x97ffff) AM_READ( MRA16_BANK2 )
	AM_RANGE( 0x980000, 0x9fbfff) AM_READ( MRA16_ROM ) AM_REGION(REGION_USER2, 0)
	AM_RANGE( 0x9fc000, 0x9fffff) AM_READ( MRA16_RAM ) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE( 0xbfd000, 0xbfefff) AM_READ( amiga_cia_r )        /* 8510's CIA A and CIA B */
	AM_RANGE( 0xe80000, 0xe8ffff) AM_READ( MRA16_RAM ) AM_BASE(&amiga_autoconfig_mem)
	AM_RANGE( 0xdbf000, 0xdfffff) AM_READ( amiga_custom_r )     /* Custom Chips */
	AM_RANGE( 0xf00000, 0xf7ffff) AM_NOP
	AM_RANGE( 0xf80000, 0xffffff) AM_READ( MRA16_ROM ) AM_REGION(REGION_USER1, 0)  /* System ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START(writemem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x07ffff) AM_WRITE( MWA16_ROM ) /* Chip Ram - 512k or System ROM mirror*/
	AM_RANGE( 0x200000, 0x201fff) AM_WRITE( MWA16_RAM ) AM_BASE(&amiga_expansion_ram)
	AM_RANGE( 0x800000, 0x9fbfff) AM_WRITE( MWA16_ROM )	/* Game cartridge and bios*/
	AM_RANGE( 0x9fc000, 0x9fffff) AM_WRITE( MWA16_RAM ) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE( 0xbfd000, 0xbfefff) AM_WRITE( amiga_cia_w )        /* 8510's CIA A and CIA B */
	AM_RANGE( 0xe80000, 0xe8ffff) AM_WRITE( MWA16_RAM ) AM_BASE(&amiga_autoconfig_mem)
	AM_RANGE( 0xdbf000, 0xdfffff) AM_WRITE( amiga_custom_w )     /* Custom Chips */
	AM_RANGE( 0xf80000, 0xffffff) AM_WRITE( MWA16_ROM )            /* System ROM */
ADDRESS_MAP_END

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( arcadia )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )  /* Fake coin 1 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )  /* Fake coin 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START				/* parallel port controls */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 1" )
	PORT_DIPSETTING(    0x01, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_SERVICE_NO_TOGGLE(0x02, IP_ACTIVE_LOW)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED ) /* coin counter read?, emulated in machine/arcadia.c */

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) /* Joystick - Port 1 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_COCKTAIL /* Joystick - Port 2 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( arcadia )
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


/***************************************************************************

ROMs definitions

***************************************************************************/
#define ROM_LOAD16_BYTE_BIOS(bios,name,offset,length,hash)     ROMX_LOAD(name, offset, length, hash, ROM_SKIP(1) | ROM_BIOS(bios+1))

#define ARCADIA_BIOS \
	ROM_REGION(0x80000, REGION_CPU1, 0) /* RAM */ \
	ROM_REGION16_BE(0x80000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD( "kick13.rom", 0x000000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785) ) \
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x040000 ) \
	ROM_REGION16_BE(0x07c000, REGION_USER2, 0) \
	ROM_LOAD16_BYTE_BIOS( 0, "scpa211h", 0x000000, 0x10000, CRC(be9dbdc5) SHA1(1554da09f051ec53937d65d4e451de51bc0c69e5) ) \
	ROM_LOAD16_BYTE_BIOS( 0, "scpa211l", 0x000001, 0x10000, CRC(95b84504) SHA1(99999fc40909001b37aa1b543918118becc81800) ) \
	ROM_LOAD16_BYTE_BIOS( 1, "scpav3_0.1h", 0x000000, 0x10000, CRC(2d8e1a06) SHA1(be187f34624aeda110017c4a09242f7c00ef56a4) ) \
	ROM_LOAD16_BYTE_BIOS( 1, "scpav3_0.1l", 0x000001, 0x10000, CRC(e4f38fab) SHA1(01c2eb5965070893be6734eb1372576727716476) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-1-hi", 0x000000, 0x10000, CRC(67d44523) SHA1(f3e3699132cdf741518accb890c04d17374c4049) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-1-lo", 0x000001, 0x10000, CRC(65d9b9cf) SHA1(5c60a0dd4a0a7d9b938ce6b0446a6ad2ecaf07ec) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-2-hi", 0x020000, 0x10000, CRC(1d7594ae) SHA1(6173bbfecf18d7d9ee6bc2b6753ca9d42fabd781) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-2-lo", 0x020001, 0x10000, CRC(e776198d) SHA1(694ca4cc99ed84a95d18201c94a3332f8599654f) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-3-hi", 0x040000, 0x10000, CRC(3e7364be) SHA1(26e10d0ddc031a891138db36ce4f1732722e6847) ) \
	ROM_LOAD16_BYTE_BIOS( 2, "gcp-3-lo", 0x040001, 0x10000, CRC(87229e0d) SHA1(0b18544801e529f954b9e03226bd2e5475f36351) )



ROM_START( ar_bios )
	ARCADIA_BIOS
ROM_END

SYSTEM_BIOS_START( ar_bios )
	SYSTEM_BIOS_ADD(0, "onep211", "OnePlay 2.11" )
	SYSTEM_BIOS_ADD(1, "onep300", "OnePlay 3.00" )
	SYSTEM_BIOS_ADD(2, "tenp211", "TenPlay 2.11" )
SYSTEM_BIOS_END
/*
AIRH from Arcadia Multi Select
------------------------------

2 Roms

1L and 1H

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_airh )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "airh_1h.bin", 0x00000, 0x10000, CRC(290e8e9e) SHA1(9215e36f02adf4064934aab99accefcb17ea6d3f) )
	ROM_LOAD16_BYTE( "airh_1l.bin", 0x00001, 0x10000, CRC(155452b6) SHA1(aeaa67ea9cc543c9a43094545450159e4784fb5c) )

ROM_END

/*
BOWL V 2.1 from Arcadia Multi Select
------------------------------------

6 Roms

1,2 and 3 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_bowl )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "bowl_1h.bin", 0x00000, 0x10000, CRC(c0c20422) SHA1(3576df08e7a4cdadaf9dea5da0770efe5f461b07) )
	ROM_LOAD16_BYTE( "bowl_1l.bin", 0x00001, 0x10000, CRC(1c7fe75c) SHA1(b1830b91b53ec24d4b072898acac02552e2eae97) )
	ROM_LOAD16_BYTE( "bowl_2h.bin", 0x20000, 0x10000, CRC(a1e497d8) SHA1(4b4885c6937b7cfb24921e84a80d6d4f56844a73) )
	ROM_LOAD16_BYTE( "bowl_2l.bin", 0x20001, 0x10000, CRC(ce23aa34) SHA1(4b17a8447286aeb775c4edb1968978e281422421) )
	ROM_LOAD16_BYTE( "bowl_3h.bin", 0x40000, 0x10000, CRC(0c55da71) SHA1(db8a1494fca3aa044da27ea1d3acf68be415be23) )
	ROM_LOAD16_BYTE( "bowl_3l.bin", 0x40001, 0x10000, CRC(5ce00809) SHA1(d7f336df28a033b38b5296537826d164aaf5e8c9) )
ROM_END

/*
DART V 2.1 from Arcadia Multi Select
------------------------------------

12 Roms

1,2,3,4,5 and 6 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_dart )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "dart_1h.bin", 0x00000, 0x10000, CRC(4d6a33e2) SHA1(1a227b5b0b4aca40d46af62e44deebca60582363) )
	ROM_LOAD16_BYTE( "dart_1l.bin", 0x00001, 0x10000, CRC(0f7261df) SHA1(d4cf35aee0b3d385f1e94865b016166b1b60760a) )
	ROM_LOAD16_BYTE( "dart_2h.bin", 0x20000, 0x10000, CRC(3a30426a) SHA1(bf9226d2bfd1fb2d70e55e30aa3dde953baf5792) )
	ROM_LOAD16_BYTE( "dart_2l.bin", 0x20001, 0x10000, CRC(479c0b73) SHA1(2ad958f4f2d902635d030cf3f466097da3cc421c) )
	ROM_LOAD16_BYTE( "dart_3h.bin", 0x40000, 0x10000, CRC(dd217562) SHA1(80e21112a87259785e5d172249dfe8058970fd4d) )
	ROM_LOAD16_BYTE( "dart_3l.bin", 0x40001, 0x10000, CRC(12cff829) SHA1(3826e5442bb125dff4f10ef8b0b65a2d5b8d9985) )
	ROM_LOAD16_BYTE( "dart_4h.bin", 0x60000, 0x10000, CRC(98b27f13) SHA1(eb4fe813be4f202badfb947291e75ec0df915c25) )
	ROM_LOAD16_BYTE( "dart_4l.bin", 0x60001, 0x10000, CRC(a059204c) SHA1(01fb21175957fa8e92f918ea560ceecc809ed0b7) )
	ROM_LOAD16_BYTE( "dart_5h.bin", 0x80000, 0x10000, CRC(38f4c236) SHA1(1a5501ed8e94cff584f40c3b984aff7aea9ec956) )
	ROM_LOAD16_BYTE( "dart_5l.bin", 0x80001, 0x10000, CRC(df4103cc) SHA1(c792cc52148afa7bde6458704d9de2550b6eb636) )
	ROM_LOAD16_BYTE( "dart_6h.bin", 0xa0000, 0x10000, CRC(e21cc8be) SHA1(04280eef26f4a97c2280bdec19b1bc586fceffb0) )
	ROM_LOAD16_BYTE( "dart_6l.bin", 0xa0001, 0x10000, CRC(21112d4e) SHA1(95e49aa2f23c6d005a0de3cf96a1c06adeacf2a9) )

ROM_END

/*
Magic Johnson's Fast Break
Arcadia Systems 1988



Piggyback 1.5mb rom board

         3h       7h       x
         2h       6h       x
         1h       5h       x
         scpa1h   4h       8h
         3l       7l       x
         2l       6l       x
 DS1220Y   1l       5l       x
 sec-scpa  scpa1l   4l       8l
*/

ROM_START( ar_fast )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "fastv28.1h", 0x000000, 0x10000, CRC(091e4533) SHA1(61a16deecd32b386d62aab95e8d4a61bddcd8af4) )
	ROM_LOAD16_BYTE( "fastv28.1l", 0x000001, 0x10000, CRC(8f7685c1) SHA1(b379c1a47618401cfbfcc7bd2d13ae51f5e73e46) )
	ROM_LOAD16_BYTE( "fastv28.2h", 0x020000, 0x10000, CRC(3a3dd931) SHA1(7be3316e2acf6b14b29ef2e36d8f76999d5d4e94) )
	ROM_LOAD16_BYTE( "fastv28.2l", 0x020001, 0x10000, CRC(4838d7e5) SHA1(d2ae5b8f25df51936937ddf62001347fccdf830a) )
	ROM_LOAD16_BYTE( "fastv28.3h", 0x040000, 0x10000, CRC(db94fa62) SHA1(4fe79a4226161b15ecdda9d85c1ad84cf31b6a30) )
	ROM_LOAD16_BYTE( "fastv28.3l", 0x040001, 0x10000, CRC(a400367d) SHA1(a4362beeb35fa0c9020883eab0a71194f3a90b9a) )
	ROM_LOAD16_BYTE( "fastv28.4h", 0x060000, 0x10000, CRC(c0a021dd) SHA1(c4c40c05050a2831b55683d85ee39b8870e0bf88) )
	ROM_LOAD16_BYTE( "fastv28.4l", 0x060001, 0x10000, CRC(870e60f1) SHA1(0f0566da96dfc898dbbc35dfaba489d1fc9ab435) )
	ROM_LOAD16_BYTE( "fastv28.5h", 0x080000, 0x10000, CRC(6daf4817) SHA1(ca0bf79e77a3e878da1f97ff9a64107e8c112aee) )
	ROM_LOAD16_BYTE( "fastv28.5l", 0x080001, 0x10000, CRC(f489da29) SHA1(5e70183acfd0d849ae9691b312ca98698b1a2252) )
	ROM_LOAD16_BYTE( "fastv28.6h", 0x0a0000, 0x10000, CRC(b23dbcfd) SHA1(67495235016e4bcbf6251e4073d6938a3c5b0eea) )
	ROM_LOAD16_BYTE( "fastv28.6l", 0x0a0001, 0x10000, CRC(4e23e807) SHA1(69c910d70fb85d037257b19a1be9e99c617bf1c4) )
	ROM_LOAD16_BYTE( "fastv28.7h", 0x0c0000, 0x10000, CRC(74d598eb) SHA1(9434169d316fc2802e7790e5b09be086fccab351) )
	ROM_LOAD16_BYTE( "fastv28.7l", 0x0c0001, 0x10000, CRC(b0649050) SHA1(a8efdfc82a63fc16ee2103b4c96b92d6f9e7afc6) )
	ROM_LOAD16_BYTE( "fastv28.8h", 0x0e0000, 0x10000, CRC(3650aaf0) SHA1(cc37aa94360159f45076eafaae8140a661bd52f6) )
	ROM_LOAD16_BYTE( "fastv28.8l", 0x0e0001, 0x10000, CRC(82603f68) SHA1(8affe73e97b966b8e63bff2c7914fb5ead7b60ff) )
ROM_END

ROM_START( ar_ldrb )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "lbg2401h", 0x00000, 0x10000, CRC(fe1287e9) SHA1(34088416d970614b31b25e982ef40fd950080b3e) )
	ROM_LOAD16_BYTE( "lbg2401l", 0x00001, 0x10000, CRC(7c7bb9ee) SHA1(5c76445732ab139db82fe21c16f49e2609bb03aa) )
	ROM_LOAD16_BYTE( "lbg2402h", 0x20000, 0x10000, CRC(64e5fbae) SHA1(0dde0d05b05f232aac9ad44398cedd8c7627f146) )
	ROM_LOAD16_BYTE( "lbg2402l", 0x20001, 0x10000, CRC(bb115e1c) SHA1(768cf51661f630b1c0a4b83b9f6124c78a517d0a) )
	ROM_LOAD16_BYTE( "lbg2403h", 0x40000, 0x10000, CRC(1d290e28) SHA1(0d589628fe59de9d7e2a57ddeabca991d1c79fdf) )
	ROM_LOAD16_BYTE( "lbg2403l", 0x40001, 0x10000, CRC(b1352a77) SHA1(ac7337a3778442d444002f730e2880f61f32cf2a) )
	ROM_LOAD16_BYTE( "lbg2404h", 0x60000, 0x10000, CRC(b621c688) SHA1(f2a50ebfc50725cdef77bb8a4864405dbb203784) )
	ROM_LOAD16_BYTE( "lbg2404l", 0x60001, 0x10000, CRC(13f9c4b0) SHA1(08a1fab271307191c5caa108c4ae284f92c270e4) )
	ROM_LOAD16_BYTE( "lbg2405h", 0x80000, 0x10000, CRC(71273172) SHA1(2b6204fdf03268e920b5948c999aa725fc66cac6) )
	ROM_LOAD16_BYTE( "lbg2405l", 0x80001, 0x10000, CRC(d9028183) SHA1(009b496da31f67b11de54e50254a9897ea68cd92) )
	ROM_LOAD16_BYTE( "lbg2406h", 0xa0000, 0x10000, CRC(a6ce61a4) SHA1(6cd64b7d589c91aeee06293f473fd1b3c56b19e0) )
	ROM_LOAD16_BYTE( "lbg2406l", 0xa0001, 0x10000, CRC(13c71422) SHA1(93e6dca2b28e1b5235b922f064be96eed0bedd8c) )
	ROM_LOAD16_BYTE( "lbg2407h", 0xc0000, 0x10000, CRC(61807fa9) SHA1(9d7097b921cf4026bb2828780e3fb87e0a3a24a0) )
	ROM_LOAD16_BYTE( "lbg2407l", 0xc0001, 0x10000, CRC(c62dae9f) SHA1(59b8e1c2469edd57024a4f3ca4222811442fa077) )
	ROM_LOAD16_BYTE( "lbg2408h", 0xe0000, 0x10000, CRC(b5911807) SHA1(b2995b308b2618f312005f130048e73c151311ae) )
	ROM_LOAD16_BYTE( "lbg2408l", 0xe0001, 0x10000, CRC(1f1ea828) SHA1(4af463bc6d58d64d4f082971c71654a6bb0c26bc) )
ROM_END

/*
LDRB V 2.5 from Arcadia Multi Select
------------------------------------

16 Roms

1,2,3,4,5,6,7 and 8 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_ldrba )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "ldrb_1h.bin", 0x00000, 0x10000, CRC(0236511c) SHA1(22b2ee076ed57ba38413c16a52510383d8488e25) )
	ROM_LOAD16_BYTE( "ldrb_1l.bin", 0x00001, 0x10000, CRC(786d34b9) SHA1(5fd6ef94f65c6fd503d3682154b576d6509a3aa9) )
	ROM_LOAD16_BYTE( "ldrb_2h.bin", 0x20000, 0x10000, CRC(64e5fbae) SHA1(0dde0d05b05f232aac9ad44398cedd8c7627f146) )
	ROM_LOAD16_BYTE( "ldrb_2l.bin", 0x20001, 0x10000, CRC(bb115e1c) SHA1(768cf51661f630b1c0a4b83b9f6124c78a517d0a) )
	ROM_LOAD16_BYTE( "ldrb_3h.bin", 0x40000, 0x10000, CRC(1d290e28) SHA1(0d589628fe59de9d7e2a57ddeabca991d1c79fdf) )
	ROM_LOAD16_BYTE( "ldrb_3l.bin", 0x40001, 0x10000, CRC(b1352a77) SHA1(ac7337a3778442d444002f730e2880f61f32cf2a) )
	ROM_LOAD16_BYTE( "ldrb_4h.bin", 0x60000, 0x10000, CRC(b621c688) SHA1(f2a50ebfc50725cdef77bb8a4864405dbb203784) )
	ROM_LOAD16_BYTE( "ldrb_4l.bin", 0x60001, 0x10000, CRC(13f9c4b0) SHA1(08a1fab271307191c5caa108c4ae284f92c270e4) )
	ROM_LOAD16_BYTE( "ldrb_5h.bin", 0x80000, 0x10000, CRC(71273172) SHA1(2b6204fdf03268e920b5948c999aa725fc66cac6) )
	ROM_LOAD16_BYTE( "ldrb_5l.bin", 0x80001, 0x10000, CRC(d9028183) SHA1(009b496da31f67b11de54e50254a9897ea68cd92) )
	ROM_LOAD16_BYTE( "ldrb_6h.bin", 0xa0000, 0x10000, CRC(a6ce61a4) SHA1(6cd64b7d589c91aeee06293f473fd1b3c56b19e0) )
	ROM_LOAD16_BYTE( "ldrb_6l.bin", 0xa0001, 0x10000, CRC(13c71422) SHA1(93e6dca2b28e1b5235b922f064be96eed0bedd8c) )
	ROM_LOAD16_BYTE( "ldrb_7h.bin", 0xc0000, 0x10000, CRC(4ebb8d12) SHA1(c328a26139ba0792cab1020b32eb4b8e39d51a22) )
	ROM_LOAD16_BYTE( "ldrb_7l.bin", 0xc0001, 0x10000, CRC(1afa9a4f) SHA1(3e5ca56e03d693a72424b9ad0717494ea8eb561e) )
	ROM_LOAD16_BYTE( "ldrb_8h.bin", 0xe0000, 0x10000, CRC(fbdca9af) SHA1(9612eb777a00ba4153f40eaefd162ca5b5efdb54) )
	ROM_LOAD16_BYTE( "ldrb_8l.bin", 0xe0001, 0x10000, CRC(322f52eb) SHA1(3033eb753fb8b3bf56b152377bf567b06a0c8144) )
ROM_END

/*
NINJ V 2.5 from Arcadia Multi Select
------------------------------------

12 Roms

1,2,3,4,5 and 6 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_ninj )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x200000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "ninj_1h.bin", 0x00000, 0x10000, CRC(53b07b4d) SHA1(4852005adf60fe63f2da880dd32740d18fd31169) )
	ROM_LOAD16_BYTE( "ninj_1l.bin", 0x00001, 0x10000, CRC(3337a6c1) SHA1(be9719f0cd5872b51f4c6d32fcac2638c0dedaf4) )
	ROM_LOAD16_BYTE( "ninj_2h.bin", 0x20000, 0x10000, CRC(e28a5fa8) SHA1(150e26aea24706b72d2e6612280d5dddc527061b) )
	ROM_LOAD16_BYTE( "ninj_2l.bin", 0x20001, 0x10000, CRC(4f52c008) SHA1(c26bf9a7a21a5b78697a684bada90ff70160f868) )
	ROM_LOAD16_BYTE( "ninj_3h.bin", 0x40000, 0x10000, CRC(c6e4dd36) SHA1(a8dcea97e0eb1da462ad55fd543c637544bfd059) )
	ROM_LOAD16_BYTE( "ninj_3l.bin", 0x40001, 0x10000, CRC(1dca7ea5) SHA1(2950ea2e9267d27e0ebe785a08e2d6627ae5eb17) )
	ROM_LOAD16_BYTE( "ninj_4h.bin", 0x60000, 0x10000, CRC(dc1a21d4) SHA1(76463837e0da8fd61de334e00adb807c7ef92523) )
	ROM_LOAD16_BYTE( "ninj_4l.bin", 0x60001, 0x10000, CRC(64660b15) SHA1(9e9c5f61add1439613400fee0c2376dc4000e6c6) )
	ROM_LOAD16_BYTE( "ninj_5h.bin", 0x80000, 0x10000, CRC(49cda31b) SHA1(e9579b9d47f7e638f933b8ce659bc63c8bdeb0a4) )
	ROM_LOAD16_BYTE( "ninj_5l.bin", 0x80001, 0x10000, CRC(1c5ef815) SHA1(7e88c1545ee15efd928220989f8b29207a8fec7e) )
	ROM_LOAD16_BYTE( "ninj_6h.bin", 0xa0000, 0x10000, CRC(b647f31e) SHA1(18367b96418ab950ba97d656e1466234af3bca80) )
	ROM_LOAD16_BYTE( "ninj_6l.bin", 0xa0001, 0x10000, CRC(9e5407e3) SHA1(85a8383573f3cd120f323e867c7fa2b6badd5aad) )
ROM_END

/*
RDWR V 2.3 from Arcadia Multi Select
------------------------------------

8 Roms

1,2,3 and 4 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_rdwr )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x200000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "rdwr_1h.bin", 0x00000, 0x10000, CRC(f52cb704) SHA1(cce8c7484ae8c3a3d14b2e79a981780a277c9b1c) )
	ROM_LOAD16_BYTE( "rdwr_1l.bin", 0x00001, 0x10000, CRC(fde0de6d) SHA1(7f62ce854a040775548c5ba3b05e6a4dcb0d7cfb) )
	ROM_LOAD16_BYTE( "rdwr_2h.bin", 0x20000, 0x10000, CRC(8f3c1a2c) SHA1(e473e55457c04ebd597375e9936aeb0473507ed7) )
	ROM_LOAD16_BYTE( "rdwr_2l.bin", 0x20001, 0x10000, CRC(21865e15) SHA1(be4b0e77a17edeb77f6a9d4bec6d49d4a46242ea) )
	ROM_LOAD16_BYTE( "rdwr_3h.bin", 0x40000, 0x10000, CRC(0cb3bc66) SHA1(5e22abcd38fc74f472cc5090b7c2893aaabc37bd) )
	ROM_LOAD16_BYTE( "rdwr_3l.bin", 0x40001, 0x10000, CRC(d863a958) SHA1(d27b8ff2daa51319d5c44700c6dd74e4bc8d99a4) )
	ROM_LOAD16_BYTE( "rdwr_4h.bin", 0x60000, 0x10000, CRC(466fe771) SHA1(1cc65887e097302bd504b8c4da5f7d2b760d7f74) )
	ROM_LOAD16_BYTE( "rdwr_4l.bin", 0x60001, 0x10000, CRC(fff39238) SHA1(05b4a70e1f808254e1fb20a15c460655d14d4216) )
ROM_END

/*
SDWR V 2.1 from Arcadia Multi Select
------------------------------------

12 Roms

1,2,3,4,5 and 6 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_sdwr )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "sdwr_1h.bin", 0x00000, 0x10000, CRC(aef3eea8) SHA1(4bf7619f52395fcbde3c8f7af3fd7da4af03c673) )
	ROM_LOAD16_BYTE( "sdwr_1l.bin", 0x00001, 0x10000, CRC(daed4add) SHA1(a9404a87f1958d7ab829fbb48855d2deb64c5aec) )
	ROM_LOAD16_BYTE( "sdwr_2h.bin", 0x20000, 0x10000, CRC(d67ba564) SHA1(2afba72a77806e3925c9ca1e13c16c442a6cfc3a) )
	ROM_LOAD16_BYTE( "sdwr_2l.bin", 0x20001, 0x10000, CRC(97f58a6d) SHA1(161bc8b3e14e5efca7b988f80cc16345280ca4bd) )
	ROM_LOAD16_BYTE( "sdwr_3h.bin", 0x40000, 0x10000, CRC(b31ad2b2) SHA1(66003bd331f61d1bd2e8f4d595b61503dad4e4b8) )
	ROM_LOAD16_BYTE( "sdwr_3l.bin", 0x40001, 0x10000, CRC(af929620) SHA1(5fde0f199016abf8fd9db821ee492feeba21b604) )
	ROM_LOAD16_BYTE( "sdwr_4h.bin", 0x60000, 0x10000, CRC(7502a271) SHA1(aa318619c0b98873b435b5bbf7feb2d5d51198f9) )
	ROM_LOAD16_BYTE( "sdwr_4l.bin", 0x60001, 0x10000, CRC(942d50b4) SHA1(eb0c9057ffd0d03dc2cde1158ce9f07de8ea6905) )
	ROM_LOAD16_BYTE( "sdwr_5h.bin", 0x80000, 0x10000, CRC(c25ac91d) SHA1(da4d46a2c987e2be2e31c081557b2de1744fa237) )
	ROM_LOAD16_BYTE( "sdwr_5l.bin", 0x80001, 0x10000, CRC(ecd1fbd3) SHA1(0b859d608859ccbff03db655219dfea4e609454d) )
	ROM_LOAD16_BYTE( "sdwr_6h.bin", 0xa0000, 0x10000, CRC(ea3c8ab3) SHA1(95cb5b9dd29c19862a2659867474cbf49192f830) )
	ROM_LOAD16_BYTE( "sdwr_6l.bin", 0xa0001, 0x10000, CRC(2544ccd7) SHA1(953aa00f2610ecd31db6e36964cbe7c2866050b9) )
ROM_END

ROM_START( ar_spot )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "spotv2.1h", 0x00000, 0x10000, CRC(a8440838) SHA1(8d64ddb603754c85aad47bd079d0a7d80d57b36c) )
	ROM_LOAD16_BYTE( "spotv2.1l", 0x00001, 0x10000, CRC(2abd2835) SHA1(b419da47c6390334ed8af56bc21430e5b43d6d58) )
	ROM_LOAD16_BYTE( "spotv2.2h", 0x20000, 0x10000, CRC(f4c95f77) SHA1(46c70755e3c6d06bec4b1bd164a586292a59249d) )
	ROM_LOAD16_BYTE( "spotv2.2l", 0x20001, 0x10000, CRC(58d7bf54) SHA1(0da63d32d738f8ed3675c6d14b2d12039af5ff21) )
	ROM_LOAD16_BYTE( "spotv2.3h", 0x40000, 0x10000, CRC(c9d2f3b7) SHA1(1b4693bcde14dc5eefe7456d4d613e6cb674c972) )
	ROM_LOAD16_BYTE( "spotv2.3l", 0x40001, 0x10000, CRC(adf94e81) SHA1(5ad56044008236edea0a44393daee06e572b1cc2) )
	ROM_LOAD16_BYTE( "spotv2.4h", 0x60000, 0x10000, CRC(cdea2feb) SHA1(4bb24b8cb5dd1e88d3f468979e2f350568414668) )
	ROM_LOAD16_BYTE( "spotv2.4l", 0x60001, 0x10000, CRC(214c353b) SHA1(819283248eac2a516f9fcdda060284ffe9c39bc8) )
	ROM_LOAD16_BYTE( "spotv2.5h", 0x80000, 0x10000, CRC(809d0f5c) SHA1(d1bae86090db8e5cc066afb76203704e7d217fde) )
	ROM_LOAD16_BYTE( "spotv2.5l", 0x80001, 0x10000, CRC(b86d8153) SHA1(42a564fa608e806d04052e67263afc4a5a417d40) )
	ROM_LOAD16_BYTE( "spotv2.6h", 0xa0000, 0x10000, CRC(8c221a34) SHA1(8f246bbcb79f5e508932d776fbfa648392f7f78d) )
	ROM_LOAD16_BYTE( "spotv2.6l", 0xa0001, 0x10000, CRC(821fa69a) SHA1(f037853be96158b8a6dd5f34e15ddfc16b6410c3) )
	ROM_LOAD16_BYTE( "spotv2.7h", 0xc0000, 0x10000, CRC(054355db) SHA1(6f4a46230b6dfd4727816737c31bce9483d3a3f7) )
	ROM_LOAD16_BYTE( "spotv2.7l", 0xc0001, 0x10000, CRC(30d396d8) SHA1(2a56727554a823f56b37b9e8d324e9f53524eb02) )
	ROM_LOAD16_BYTE( "spotv2.8h", 0xe0000, 0x10000, CRC(94dbb239) SHA1(0c475c8e102cc835d01e3de4604c1323219048f1) )
	ROM_LOAD16_BYTE( "spotv2.8l", 0xe0001, 0x10000, CRC(4d7f8f05) SHA1(04690717cec5912cd12ccb7135614842f5597898) )
ROM_END

/*
SPRG from Arcadia Multi Select
------------------------------

8 Roms

1,2,3 and 4 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_sprg )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "sprg_1h.bin", 0x00000, 0x10000, CRC(90b45dc5) SHA1(7cf1fc27e95bf207ed94cc5c20cf0c0ae7799d83) )
	ROM_LOAD16_BYTE( "sprg_1l.bin", 0x00001, 0x10000, CRC(e5ce68e9) SHA1(dfda2e0bffc499a497865bc214450653880eccf2) )
	ROM_LOAD16_BYTE( "sprg_2h.bin", 0x20000, 0x10000, CRC(02ef780f) SHA1(d21d6e8a379a2b38de7f3ec0540f67dd6425cbc9) )
	ROM_LOAD16_BYTE( "sprg_2l.bin", 0x20001, 0x10000, CRC(fa1f5b23) SHA1(4f808d8ee9cd672061995d0bfab65851bf1c01d3) )
	ROM_LOAD16_BYTE( "sprg_3h.bin", 0x40000, 0x10000, CRC(48130e6e) SHA1(479555c4a5c041c04135f01fbdd5c5f78f4422cf) )
	ROM_LOAD16_BYTE( "sprg_3l.bin", 0x40001, 0x10000, CRC(4b968cc6) SHA1(fbf3bcb5803dbe75e5a9bcde39c98c9c30bd1699) )
	ROM_LOAD16_BYTE( "sprg_4h.bin", 0x60000, 0x10000, CRC(23c8f667) SHA1(da75def3a34f6e7d48f2c6cefff608348c09cf70) )
	ROM_LOAD16_BYTE( "sprg_4l.bin", 0x60001, 0x10000, CRC(13ba011f) SHA1(75da2fbbfe2e957992b2a73609737d777fe9a151) )
ROM_END

/*
XEON V 2.3 from Arcadia Multi Select
------------------------------------

8 Roms

1,2,3 and 4 Lo and Hi

There is also a PAL16L8 but I cannot read that.
*/

ROM_START( ar_xeon )
	ARCADIA_BIOS

	ROM_REGION16_BE(0x180000, REGION_USER3, 0)
	ROM_LOAD16_BYTE( "xeon_1h.bin", 0x00000, 0x10000, CRC(ca422811) SHA1(fa6f82e1d91b48d58b61f916d5b04dc1a13774fb) )
	ROM_LOAD16_BYTE( "xeon_1l.bin", 0x00001, 0x10000, CRC(97edf967) SHA1(57fca524e01ba21f7420472f14aaf3fa63a326fa) )
	ROM_LOAD16_BYTE( "xeon_2h.bin", 0x20000, 0x10000, CRC(8078c10e) SHA1(599995374b23da7187556e2f4f285b60d818f885) )
	ROM_LOAD16_BYTE( "xeon_2l.bin", 0x20001, 0x10000, CRC(a8845d8f) SHA1(2d54dc25af68c46bbbdf8f9ed8014ae7d8564e09) )
	ROM_LOAD16_BYTE( "xeon_3h.bin", 0x40000, 0x10000, CRC(9d013152) SHA1(7a3bec56d564efbca9721d308b3eddc76763ec41) )
	ROM_LOAD16_BYTE( "xeon_3l.bin", 0x40001, 0x10000, CRC(331b1449) SHA1(0e282d04b2c7e68051e5ea1671737b11dfb71521) )
	ROM_LOAD16_BYTE( "xeon_4h.bin", 0x60000, 0x10000, CRC(fbf43d5c) SHA1(6d335b7b1d3b75887526cb8ea3518661b5554774) )
	ROM_LOAD16_BYTE( "xeon_4l.bin", 0x60001, 0x10000, CRC(47b60bf5) SHA1(10d8addc090ad3fa2663c40e22f736ac3522b177) )
ROM_END

/***************************************************************************

  Decryption routines

***************************************************************************/

/* decryption of bios rom: scpa211l, spotv3.0l, scpav3_0.1l */
static void arcadia_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER2);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 6, 1, 0, 2, 3, 4, 5, 7 );
		rom ++;
	}

}

/* decryption of game roms */

/* ar_airh: airh_1l.bin */
static void airh_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 5, 0, 2, 4, 7, 6, 1, 3 );
		rom ++;
	}
}

/* ar_bowl: bowl_1l.bin */
static void bowl_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 7, 6, 0, 1, 2, 3, 4, 5 );
		rom ++;
	}
}

/* ar_dart: dart_1l.bin */
static void dart_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 4, 0, 7, 6, 3, 1, 2, 5 );
		rom ++;
	}

	/* patch needed for Arcadia bios to recognize game properly */
	/* maybe dart_1l.bin is a bad dump? */
	rom = (UINT16 *)memory_region(REGION_USER3);
	*rom = 0x4afc;
}

/* ar_ldrba: ldrb_1l.bin */
static void ldrba_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 2, 3, 4, 1, 0, 7, 5, 6 );
		rom ++;
	}
}

  /* ar_ninj: ninj_1l.bin */
static void ninj_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 1, 6, 5, 7, 4, 2, 0, 3 );
		rom ++;
	}
}

/* ar_rdwr: rdwr_1l.bin */
static void rdwr_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 3, 1, 6, 4, 0, 5, 2, 7 );
		rom ++;
	}
}

/* ar_sdwr: sdwr_1l.bin */
static void sdwr_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 6, 3, 4, 5, 2, 1, 0, 7 );
		rom ++;
	}
}

/* ar_sprg: sprg_1l.bin */
static void sprg_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 4, 7, 3, 0, 6, 5, 2, 1 );
		rom ++;
	}
}

/* ar_xeon: xeon_1l.bin */
static void xeon_decode(void)
{
	UINT16* rom = (UINT16 *)memory_region(REGION_USER3);
	int i;

	for ( i = 0; i < 0x20000; i += 2 )
	{
		*rom = BITSWAP16(*rom, 15,14,13,12,11,10,9,8, 3, 1, 2, 4, 0, 5, 6, 7 );
		rom ++;
	}
}

/***************************************************************************

                  Arcadia machine interface

***************************************************************************/

static unsigned char coin_counter[2];

static int arcadia_cia_0_portA_r( void )
{
	int ret = readinputport( 0 ) & 0xc0;
	ret |= 0x3f;
	return ret; /* Gameport 1 and 0 buttons */
}

static int arcadia_cia_0_portB_r( void )
{
	/* parallel port */
	int ret = 0;
	ret = readinputport( 1 ) & 0x0f;

	ret |= ( coin_counter[0] & 3 ) << 4;
	ret |= ( coin_counter[1] & 3 ) << 6;

	logerror( "Coin counter read at PC=%06x\n", activecpu_get_pc() );
	return ret;
}

static void arcadia_cia_0_portA_w( int data )
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

	set_led_status( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led on Amiga*/
}

static void arcadia_cia_0_portB_w( int data )
{
	/* parallel port */
	/* bit 0 = coin counter reset? */
	if ( (data & 1) == 1 ) {
		if (coin_counter[0] > 0 )
			coin_counter[0]--;
		if (coin_counter[1] > 0 )
			coin_counter[1]--;
	}
}

static void arcadia_update_coins(void)
{
	/* update coin counters */

	/* check for a 0 -> 1 transition */
	if ( readinputport( 0 ) & 0x20 ) {
		if ( ( coin_counter[0] & 0x80 ) == 0 ) {
			if ( coin_counter[0] < 3 )
				coin_counter[0]++;
			coin_counter[0] |= 0x80;
		}
	} else
		coin_counter[0] = 0;

	/* check for a 0 -> 1 transition */
	if ( readinputport( 0 ) & 0x10 ) {
		if ( ( coin_counter[1] & 0x80 ) == 0 ) {
			if ( coin_counter[1] < 3 )
				coin_counter[1]++;
			coin_counter[1] |= 0x80;
		}
	} else
		coin_counter[1] = 0;
}

static void arcadia_reset_coins(void)
{
	/* reset coin counters */
	coin_counter[0] = coin_counter[1] = 0;
}

static data16_t arcadia_read_joy0dat(void)
{
	int input = ( readinputport( 2 ) >> 4 );
	int	top,bot,lft,rgt;

	top = ( input >> 3 ) & 1;
	bot = ( input >> 2 ) & 1;
	lft = ( input >> 1 ) & 1;
	rgt = input & 1;

	if ( lft ) top ^= 1;
	if ( rgt ) bot ^= 1;

	return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
}

static data16_t arcadia_read_joy1dat(void)
{
	int input = ( readinputport( 2 ) & 0x0f );
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

static const struct amiga_machine_interface arcadia_intf =
{
	arcadia_cia_0_portA_r,
	arcadia_cia_0_portB_r,
	arcadia_cia_0_portA_w,
	arcadia_cia_0_portB_w,
	NULL,
	NULL,
	NULL,
	NULL,
	arcadia_read_joy0dat,
	arcadia_read_joy1dat,
	NULL,
	NULL,
	arcadia_update_coins,
	arcadia_reset_coins
};

static struct
{
	const char*	name;
	void (*decodefun)(void);
} arcadia_config_table[] =
{
	{ "ar_airh",	airh_decode },
	{ "ar_bowl",	bowl_decode },
	{ "ar_dart",	dart_decode },
	{ "ar_ldrba",	ldrba_decode },
	{ "ar_ninj",	ninj_decode },
	{ "ar_rdwr",	rdwr_decode },
	{ "ar_sdwr",	sdwr_decode },
	{ "ar_sprg",	sprg_decode },
	{ "ar_xeon",	xeon_decode },
	{ NULL,			NULL }
};

WRITE16_HANDLER(arcadia_multibios_change_game)
{
	if (data == 0)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x800000, 0x97ffff, 0, 0, MRA16_BANK2 );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x800000, 0x97ffff, 0, 0, MWA16_NOP );
	}
	else
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x800000, 0x97ffff, 0, 0, MRA16_NOP );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x800000, 0x97ffff, 0, 0, MWA16_NOP );
	}
}

DRIVER_INIT(arcadia)
{
	UINT32 boot_vector;
	UINT16* RAM;
	UINT8 is_multi_game_bios;
	int	  i;

	amiga_machine_config(&arcadia_intf);

	/* set up memory */
	memory_set_bankptr(1, memory_region(REGION_USER1));
	memory_set_bankptr(2, memory_region(REGION_USER3));

	RAM = (UINT16*)memory_region(REGION_USER2);

	/* OnePlay bios is encrypted, TenPlay is not */
	if ( RAM[0] != 0x4afc )
	{
		is_multi_game_bios = 0;
		arcadia_decode();
	}
	else
	{
		is_multi_game_bios = 1;
	}

	/* determine boot vector */
	boot_vector = (RAM[0x16/2] << 16) | RAM[0x18/2];

	/* set up autoconfig */
	autoconfig_init( boot_vector );

	/* install game switch handler for TenPlay */
	if ( is_multi_game_bios )
	{
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x009ffffe, 0x009fffff, 0, 0, arcadia_multibios_change_game );
	}

	/* decrypt game */
	for ( i = 0; arcadia_config_table[i].name != NULL; i++ )
	{
		if ( strcmp( Machine->gamedrv->name, arcadia_config_table[i].name ) == 0 )
		{
			arcadia_config_table[i].decodefun();
		}
	}
}


/* This kludge should be gone after sound and sound irqs are emulated */
READ16_HANDLER( dart_kludge2 )
{
	return 0;
}

READ16_HANDLER( dart_kludge1 )
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x02a804, 0x02a805, 0, 0, dart_kludge2 );
	return ((data16_t*)generic_nvram16)[0];
}

DRIVER_INIT( ar_dart )
{
	init_arcadia();
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x9fc000, 0x9fc001, 0, 0, dart_kludge1 );
}


/* BIOS */
GAMEBX( 1988, ar_bios,	0,		 ar_bios, arcadia, arcadia, 0,		 0, "Arcadia Systems", "Arcadia System BIOS", NOT_A_DRIVER )

/* working */
GAMEBX( 1988, ar_bowl,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "SportTime Bowling (Arcadia, V 2.1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1987, ar_dart,	ar_bios, ar_bios, arcadia, arcadia, ar_dart, 0, "Arcadia Systems", "World Darts (Arcadia, V 2.1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_fast,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Magic Johnson's Fast Break (Arcadia, V 2.8)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_ldrb,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Leader Board (Arcadia, V 2.4?)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_ldrba,	ar_ldrb, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Leader Board (Arcadia, V 2.5)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1987, ar_ninj,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Ninja Mission (Arcadia, V 2.5)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_rdwr,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "RoadWars (Arcadia, V 2.3)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEBX( 1990, ar_spot,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Spot (Arcadia)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_xeon,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Xenon (Arcadia, V 2.3)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1987, ar_sprg,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Space Ranger (Arcadia, V 2.0)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_airh,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "SportTime Table Hockey (Arcadia, V 2.1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEBX( 1988, ar_sdwr,	ar_bios, ar_bios, arcadia, arcadia, arcadia, 0, "Arcadia Systems", "Sidewinder (Arcadia, V 2.1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
