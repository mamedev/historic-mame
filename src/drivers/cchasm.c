/***************************************************************************

	Cinematronics Cosmic Chasm hardware

	driver by Mathis Rosenhauer

	Games supported:
		* Cosmic Chasm

	Known bugs:
		* none at this time

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "machine/z80fmly.h"
#include "cchasm.h"



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x040000, 0x04000f, cchasm_6840_r },
	{ 0x060000, 0x060001, input_port_0_word_r },
	{ 0xf80000, 0xf800ff, cchasm_io_r },
	{ 0xffb000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x040000, 0x04000f, cchasm_6840_w },
	{ 0x050000, 0x050001, cchasm_refresh_control_w },
	{ 0x060000, 0x060001, cchasm_led_w },
	{ 0x070000, 0x070001, watchdog_reset16_w },
	{ 0xf80000, 0xf800ff, cchasm_io_w },
	{ 0xffb000, 0xffffff, MWA16_RAM, &cchasm_ram },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5000, 0x53ff, MRA_RAM },
	{ 0x6000, 0x6fff, cchasm_snd_io_r },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x5000, 0x53ff, MWA_RAM },
	{ 0x6000, 0x6fff, cchasm_snd_io_w },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x00, 0x03, z80ctc_0_r },
PORT_END


static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x03, z80ctc_0_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cchasm )
	PORT_START /* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "40000" )
	PORT_DIPSETTING(    0x04, "60000" )
	PORT_DIPSETTING(    0x02, "80000" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Once" )
	PORT_DIPSETTING(    0x10, "Every" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START /* IN1 */
	PORT_ANALOG( 0xff, 0, IPT_DIAL, 100, 10, 0, 0)

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Test 1", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test 2, not used in cchasm */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test 3, not used in cchasm */
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1818182,	/* 1.82 MHz */
	{ 20, 20 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};


static struct CustomSound_interface custom_interface =
{
	cchasm_sh_start,
    0,
	cchasm_sh_update
};



/*************************************
 *
 *	CPU config
 *
 *************************************/

static Z80_DaisyChain daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0 }, /* CTC number 0 */
	{ 0,0,0,-1 } 		/* end mark */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( cchasm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,8000000)	/* 8 MHz (from schematics) */
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_CPU_ADD(Z80,3584229)		/* 3.58  MHz (from schematics) */
	MDRV_CPU_CONFIG(daisy_chain)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(40)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 1024-1, 0, 768-1)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(cchasm)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( cchasm )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "chasm.u4",  0x000000, 0x001000, 0x19244f25 )
    ROM_LOAD16_BYTE( "chasm.u12", 0x000001, 0x001000, 0x5d702c7d )
    ROM_LOAD16_BYTE( "chasm.u8",  0x002000, 0x001000, 0x56a7ce8a )
    ROM_LOAD16_BYTE( "chasm.u16", 0x002001, 0x001000, 0x2e192db0 )
    ROM_LOAD16_BYTE( "chasm.u3",  0x004000, 0x001000, 0x9c71c600 )
    ROM_LOAD16_BYTE( "chasm.u11", 0x004001, 0x001000, 0xa4eb59a5 )
    ROM_LOAD16_BYTE( "chasm.u7",  0x006000, 0x001000, 0x8308dd6e )
    ROM_LOAD16_BYTE( "chasm.u15", 0x006001, 0x001000, 0x9d3abf97 )
    ROM_LOAD16_BYTE( "u2",        0x008000, 0x001000, 0x4e076ae7 )
    ROM_LOAD16_BYTE( "u10",       0x008001, 0x001000, 0xcc9e19ca )
    ROM_LOAD16_BYTE( "chasm.u6",  0x00a000, 0x001000, 0xa96525d2 )
    ROM_LOAD16_BYTE( "chasm.u14", 0x00a001, 0x001000, 0x8e426628 )
    ROM_LOAD16_BYTE( "u1",        0x00c000, 0x001000, 0x88b71027 )
    ROM_LOAD16_BYTE( "chasm.u9",  0x00c001, 0x001000, 0xd90c9773 )
    ROM_LOAD16_BYTE( "chasm.u5",  0x00e000, 0x001000, 0xe4a58b7d )
    ROM_LOAD16_BYTE( "chasm.u13", 0x00e001, 0x001000, 0x877e849c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "2732.bin", 0x0000, 0x1000, 0x715adc4a )
ROM_END

ROM_START( cchasm1 )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "chasm.u4",  0x000000, 0x001000, 0x19244f25 )
    ROM_LOAD16_BYTE( "chasm.u12", 0x000001, 0x001000, 0x5d702c7d )
    ROM_LOAD16_BYTE( "chasm.u8",  0x002000, 0x001000, 0x56a7ce8a )
    ROM_LOAD16_BYTE( "chasm.u16", 0x002001, 0x001000, 0x2e192db0 )
    ROM_LOAD16_BYTE( "chasm.u3",  0x004000, 0x001000, 0x9c71c600 )
    ROM_LOAD16_BYTE( "chasm.u11", 0x004001, 0x001000, 0xa4eb59a5 )
    ROM_LOAD16_BYTE( "chasm.u7",  0x006000, 0x001000, 0x8308dd6e )
    ROM_LOAD16_BYTE( "chasm.u15", 0x006001, 0x001000, 0x9d3abf97 )
    ROM_LOAD16_BYTE( "chasm.u2",  0x008000, 0x001000, 0x008b26ef )
    ROM_LOAD16_BYTE( "chasm.u10", 0x008001, 0x001000, 0xc2c532a3 )
    ROM_LOAD16_BYTE( "chasm.u6",  0x00a000, 0x001000, 0xa96525d2 )
    ROM_LOAD16_BYTE( "chasm.u14", 0x00a001, 0x001000, 0x8e426628 )
    ROM_LOAD16_BYTE( "chasm.u1",  0x00c000, 0x001000, 0xe02293f8 )
    ROM_LOAD16_BYTE( "chasm.u9",  0x00c001, 0x001000, 0xd90c9773 )
    ROM_LOAD16_BYTE( "chasm.u5",  0x00e000, 0x001000, 0xe4a58b7d )
    ROM_LOAD16_BYTE( "chasm.u13", 0x00e001, 0x001000, 0x877e849c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "2732.bin", 0x0000, 0x1000, 0x715adc4a )
ROM_END




/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, cchasm,  0,      cchasm, cchasm, 0, ROT270, "Cinematronics / GCE", "Cosmic Chasm (set 1)" )
GAME( 1983, cchasm1, cchasm, cchasm, cchasm, 0, ROT270, "Cinematronics / GCE", "Cosmic Chasm (set 2)" )
