/***************************************************************************

	Atari Avalanche hardware

	driver by Mike Balfour

	Games supported:
		* Avalanche

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
					0000-1FFF				RAM
					2000-2FFF		R		INPUTS
					3000-3FFF		W		WATCHDOG
					4000-4FFF		W		OUTPUTS
					5000-5FFF		W		SOUND LEVEL
					6000-7FFF		R		PROGRAM ROM
					8000-DFFF				UNUSED
					E000-FFFF				PROGRAM ROM (Remapped)

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"
#include "avalnche.h"



/*************************************
 *
 *	Overlay
 *
 *************************************/

#define OVERLAY_CYAN		MAKE_ARGB(0x04,0x80,0xff,0xff)
#define OVERLAY_BLUE		MAKE_ARGB(0x04,0x20,0x20,0xff)
#define OVERLAY_YELLOW		MAKE_ARGB(0x04,0xff,0xff,0x20)
#define OVERLAY_ORANGE		MAKE_ARGB(0x04,0xff,0x80,0x10)

OVERLAY_START( avalnche_overlay )
	OVERLAY_RECT(   0,   0, 256,  10, OVERLAY_CYAN )
	OVERLAY_RECT(   0,  10, 256,  20, OVERLAY_BLUE )
	OVERLAY_RECT(   0,  20, 256,  29, OVERLAY_YELLOW )
	OVERLAY_RECT(   0,  29, 256,  40, OVERLAY_ORANGE )
	OVERLAY_RECT(   0,  40, 256, 240, OVERLAY_CYAN )
OVERLAY_END



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static PALETTE_INIT( avalnche )
{
	/* 2 colors in the palette: black & white */
	palette_set_color(0,0x00,0x00,0x00);
	palette_set_color(1,0xff,0xff,0xff);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_RAM }, /* RAM SEL */
	{ 0x2000, 0x2fff, avalnche_input_r }, /* INSEL */
	{ 0x6000, 0x7fff, MRA_ROM }, /* ROM1-ROM2 */
	{ 0xe000, 0xffff, MRA_ROM }, /* ROM2 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, avalnche_videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x3000, 0x3fff, MWA_NOP }, /* WATCHDOG */
	{ 0x4000, 0x4fff, avalnche_output_w }, /* OUTSEL */
	{ 0x5000, 0x5fff, avalnche_noise_amplitude_w }, /* SOUNDLVL */
	{ 0x6000, 0x7fff, MWA_ROM }, /* ROM1-ROM2 */
	{ 0xe000, 0xffff, MWA_ROM }, /* ROM1-ROM2 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( avalnche )
	PORT_START /* IN0 */
	PORT_BIT (0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Spare */
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x30, "German" )
	PORT_DIPSETTING(    0x20, "French" )
	PORT_DIPSETTING(    0x10, "Spanish" )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x04, 0x04, "Allow Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Lives/Extended Play" )
	PORT_DIPSETTING(    0x00, "3/450 points" )
	PORT_DIPSETTING(    0x08, "5/750 points" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* SLAM */
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE | IPF_TOGGLE, DEF_STR ( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* Serve */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* VBLANK */

	PORT_START /* IN2 */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 50, 10, 0x40, 0xb7 )
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct DACinterface dac_interface =
{
	2,
	{ 100, 100 },
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( avalnche )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,12096000/16)	   /* clock input is the "2H" signal divided by two */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(avalnche_interrupt,32)	/* interrupt at a 4V frequency for sound */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	
	MDRV_PALETTE_INIT(avalnche)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(avalnche)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( avalnche )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	/* Note: These are being loaded into a bogus location, */
	/*		 They are nibble wide rom images which will be */
	/*		 merged and loaded into the proper place by    */
	/*		 orbit_rom_init()							   */
	ROM_LOAD( "30612.d2",     	0x8800, 0x0800, CRC(3f975171) SHA1(afe680865da97824f1ebade4c7a2ba5d7ee2cbab) )
	ROM_LOAD( "30613.e2",     	0x9000, 0x0800, CRC(47a224d3) SHA1(9feb7444a2e5a3d90a4fe78ae5d23c3a5039bfaa) )
	ROM_LOAD( "30611.c2",     	0x9800, 0x0800, CRC(0ad07f85) SHA1(5a1a873b14e63dbb69ee3686ba53f7ca831fe9d0) )

	ROM_LOAD( "30615.d3",     	0xa800, 0x0800, CRC(3e1a86b4) SHA1(3ff4cffea5b7a32231c0996473158f24c3bbe107) )
	ROM_LOAD( "30616.e3",     	0xb000, 0x0800, CRC(f620f0f8) SHA1(7802b399b3469fc840796c3145b5f63781090956) )
	ROM_LOAD( "30614.c3",     	0xb800, 0x0800, CRC(a12d5d64) SHA1(1647d7416bf9266d07f066d3797bda943e004d24) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( avalnche )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x6000-0x7fff and e000-ffff */

	for(i=0;i<0x2000;i++)
	{
		rom[0x6000+i] = (rom[0x8000+i]<<4)+rom[0xA000+i];
		rom[0xE000+i] = (rom[0x8000+i]<<4)+rom[0xA000+i];
	}
	
	artwork_set_overlay(avalnche_overlay);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1978, avalnche, 0, avalnche, avalnche, avalnche, ROT0, "Atari", "Avalanche" )
