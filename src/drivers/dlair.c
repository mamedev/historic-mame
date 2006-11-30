/*************************************************************************

    Cinematronics Dragon's Lair system

    Games supported:
        * Dragon's Lair (needs laserdisc emulation)

    Still to come:
        * Dragon's Lair (European version)
        * Space Ace

**************************************************************************

    There are two revisions of the Cinematronics board used in the
    U.S. Rev A


    ROM Revisions
    -------------
    Revision A, B, and C EPROMs use the Pioneer PR-7820 only.
    Revision D EPROMs used the Pioneer LD-V1000 only.
    Revisions E, F, and F2 EPROMs used either player.

    Revisions A, B, C, and D are a five chip set.
    Revisions E, F, and F2 are a four chip set.


    Laserdisc Players Used
    ----------------------
    Pioneer PR-7820 (USA / Cinematronics)
    Pioneer LD-V1000 (USA / Cinematronics)
    Philips 22VP932 (Europe / Atari) and (Italian / Sidam)

*************************************************************************/

#define MASTER_CLOCK	16000000


#include "driver.h"
#include "machine/laserdsc.h"
#include "sound/ay8910.h"
#include "dlair.lh"

#define LASERDISC_TYPE_MASK			0x7f
#define LASERDISC_TYPE_VARIABLE		0x80

static laserdisc_state *discstate;
static UINT8 last_misc;

static UINT8 laserdisc_type;
static UINT8 laserdisc_data;


VIDEO_START( dlair )
{
	return 0;
}

VIDEO_UPDATE( dlair )
{
	if (discstate != NULL)
		popmessage("%s", laserdisc_describe_state(discstate));

	return 0;
}



/*

    0000-1fff = Y0
    2000-3fff = Y1
    4000-5fff = Y2
    6000-7fff = Y3
    8000-9fff = Y4
    a000-bfff = /SEL RAM   -> 0x800 bytes of RAM

    c000-dfff = /RD I/O
      c000-c007 = AY-8910 read
      c008-c00f = /SEL CP A (input port 0: up/down/left/right/action/aux1/aux2/aux3)
      c010-c017 = /SEL CP B (input port 1: p1/p2/cin1/coin2/aux4/aux5/enter/ready)
      c018-c01f = n/c
      c020-c027 = /RD DSC DAT (read from disc latch)
      c028-c02f = n/c
      c030-c037 = n/c
      c038-c03f = n/c
      <repeat across whole c000-dfff space>

    e000-ffff = /WR I/O
      e000-e007 = AY-8910 write
      e008-e00f = /LOAD MISC
      e010-e017 = AY-8910 control
      e018-e01f = n/c
      e020-e027 = /LOAD DISC DATA (latch data from disc)
      e028-e02f = n/c
      e030-e037 = DEN2
      e038-e03f = DEN1
      <repeat across whole e000-ffff space>

*/


static MACHINE_START( dlair )
{
	discstate = NULL;
	return 0;
}


static MACHINE_RESET( dlair )
{
	/* determine the laserdisc player from the DIP switches */
	if (laserdisc_type & LASERDISC_TYPE_VARIABLE)
	{
		laserdisc_type &= ~LASERDISC_TYPE_MASK;
		laserdisc_type |= (readinputportbytag("DSW2") & 0x08) ? LASERDISC_TYPE_LDV1000 : LASERDISC_TYPE_PR7820;
	}
	discstate = laserdisc_init(laserdisc_type & LASERDISC_TYPE_MASK);
}



void vblank_callback(void)
{
	if (cpu_getcurrentframe() % 2 == 1)
		laserdisc_vsync(discstate);
}


static WRITE8_HANDLER( misc_w )
{
	/*
        D0-D3 = B0-B3
           D4 = coin counter
           D5 = OUT DISC DATA
           D6 = ENTER
           D7 = INT/EXT
    */
	UINT8 diff = data ^ last_misc;
	last_misc = data;

	coin_counter_w(0, (~data >> 4) & 1);

	/* on bit 5 going low, push the data out to the laserdisc player */
	if ((diff & 0x20) && !(data & 0x20))
		laserdisc_data_w(discstate, laserdisc_data);

	/* on bit 6 going low, we need to signal enter */
	laserdisc_line_w(discstate, LASERDISC_LINE_ENTER, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);

//  mame_printf_debug("%04X:misc_w=%02X\n", safe_activecpu_get_pc());
}


static const UINT8 led_map[16] =
	{ 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7c,0x07,0x7f,0x67,0x77,0x7c,0x39,0x5e,0x79,0x00 };


static WRITE8_HANDLER( led_den2_w )
{
	output_set_digit_value(8 + (offset & 7), led_map[data & 0x0f]);
}


static WRITE8_HANDLER( led_den1_w )
{
	output_set_digit_value(0 + (offset & 7), led_map[data & 0x0f]);
}


static UINT32 laserdisc_status_r(void *param)
{
	if (discstate == NULL)
		return 0;

	switch (laserdisc_type & LASERDISC_TYPE_MASK)
	{
		case LASERDISC_TYPE_PR7820:
			return 0;

		case LASERDISC_TYPE_LDV1000:
			return (laserdisc_line_r(discstate, LASERDISC_LINE_STATUS) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_22VP932:
			return 0;
	}
	return 0;
}


static UINT32 laserdisc_command_r(void *param)
{
	if (discstate == NULL)
		return 0;

	switch (laserdisc_type & LASERDISC_TYPE_MASK)
	{
		case LASERDISC_TYPE_PR7820:
			return (laserdisc_line_r(discstate, LASERDISC_LINE_READY) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_LDV1000:
			return (laserdisc_line_r(discstate, LASERDISC_LINE_COMMAND) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_22VP932:
			return 0;
	}
	return 0;
}


static READ8_HANDLER( laserdisc_r )
{
	UINT8 result = laserdisc_data_r(discstate);
	mame_printf_debug("laserdisc_r = %02X\n", result);
	return result;
}


static WRITE8_HANDLER( laserdisc_w )
{
	laserdisc_data = data;
}


/*************************************
 *
 *  Cinematronics version memory map
 *
 *************************************/

static ADDRESS_MAP_START( cinemat_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_MIRROR(0x1800) AM_RAM
	AM_RANGE(0xc000, 0xc000) AM_MIRROR(0x1fc7) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xc008, 0xc008) AM_MIRROR(0x1fc7) AM_READ(input_port_2_r)
	AM_RANGE(0xc010, 0xc010) AM_MIRROR(0x1fc7) AM_READ(input_port_3_r)
	AM_RANGE(0xc020, 0xc020) AM_MIRROR(0x1fc7) AM_READ(laserdisc_r)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xe008, 0xe008) AM_MIRROR(0x1fc7) AM_WRITE(misc_w)
	AM_RANGE(0xe010, 0xe010) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xe020, 0xe020) AM_MIRROR(0x1fc7) AM_WRITE(laserdisc_w)
	AM_RANGE(0xe030, 0xe037) AM_MIRROR(0x1fc0) AM_WRITE(led_den2_w) /* DEN2 */
	AM_RANGE(0xe038, 0xe03f) AM_MIRROR(0x1fc0) AM_WRITE(led_den1_w) /* DEN1 */
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( dlair )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("A:1,0")
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Unused ) )
	PORT_DIPNAME( 0x04, 0x00, "Difficulty Mode" ) PORT_DIPLOCATION("A:2")
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, "Engineering mode" ) PORT_DIPLOCATION("A:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "2 Credits/Free play" ) PORT_DIPLOCATION("A:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) ) PORT_DIPLOCATION("A:5")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Pay as you go" ) PORT_DIPLOCATION("A:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Diagnostics" ) PORT_DIPLOCATION("A:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Sound every 8 attracts" ) PORT_DIPLOCATION("B:0")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("B:1")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unlimited Dirks" ) PORT_DIPLOCATION("B:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Joystick Feedback Sound" ) PORT_DIPLOCATION("B:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Pay as you go options" ) PORT_DIPLOCATION("B:6,5")
	PORT_DIPSETTING(    0x00, "PAYG1" )
	PORT_DIPSETTING(    0x20, "PAYG2" )
	PORT_DIPSETTING(    0x40, "PAYG3" )
	PORT_DIPSETTING(    0x60, "PAYG4" )
	PORT_DIPNAME( 0x90, 0x10, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("B:7,4")
	PORT_DIPSETTING(	0x00, "Increase after 5" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x10, "Increase after 9" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x00, DEF_STR( Hard ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x10, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_status_r, 0) 	/* status strobe */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_command_r, 0)	/* command strobe */
INPUT_PORTS_END


INPUT_PORTS_START( dlaire )
	PORT_INCLUDE(dlair)

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x08, 0x08, "LD Player" )		/* In Rev F, F2 and so on... before it was Joystick Sound Feedback */
	PORT_DIPSETTING(    0x00, "LD-PR7820" )
	PORT_DIPSETTING(    0x08, "LDV-1000" )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,16,
	512,
	1,
	{ 0 },
	{ STEP8(7,-1) },
	{ STEP16(0,8) },
	16*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,  0, 8 },
	{ -1 }
};



/*************************************
 *
 *  Sound chip definitions
 *
 *************************************/

struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	input_port_1_r
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( dlair )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MASTER_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(cinemat_map,0)
	MDRV_CPU_VBLANK_INT(vblank_callback, 1)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(MASTER_CLOCK/8/16/16/16/16))

	MDRV_FRAMES_PER_SECOND(24 * 2)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(dlair)
	MDRV_MACHINE_RESET(dlair)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_VIDEO_START(dlair)
	MDRV_VIDEO_UPDATE(dlair)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK/8)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( dlair )		/* revision F2 */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_f2_u1.bin", 0x0000, 0x2000,  CRC(f5ea3b9d) SHA1(c0cafff8b2982125fd3314ffc66681e47f027fc9) )
	ROM_LOAD( "dl_f2_u2.bin", 0x2000, 0x2000,  CRC(dcc1dff2) SHA1(614ca8f6c5b6fa1d590f6b80d731377faa3a65a9) )
	ROM_LOAD( "dl_f2_u3.bin", 0x4000, 0x2000,  CRC(ab514e5b) SHA1(29d1015b951f0f2d4e5257497f3bf007c5e2262c) )
	ROM_LOAD( "dl_f2_u4.bin", 0x6000, 0x2000,  CRC(f5ec23d2) SHA1(71149e2d359cc5944fbbb53dd7d0c2b42fbc9bb4) )
ROM_END

ROM_START( dlaira )		/* revision A */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_a_u1.bin", 0x0000, 0x2000,  CRC(d76e83ec) SHA1(fc7ff5d883de9b38a9e0532c35990f4b319ba1d3) )
	ROM_LOAD( "dl_a_u2.bin", 0x2000, 0x2000,  CRC(a6a723d8) SHA1(5c71cb0b6be7331083adaf6fac6bdfc8445cb485) )
	ROM_LOAD( "dl_a_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_a_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_a_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )
ROM_END

ROM_START( dlairb )		/* revision B */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_b_u1.bin", 0x0000, 0x2000,  CRC(d76e83ec) SHA1(fc7ff5d883de9b38a9e0532c35990f4b319ba1d3) )
	ROM_LOAD( "dl_b_u2.bin", 0x2000, 0x2000,  CRC(6751103d) SHA1(e94e19f738e0eb69700e56c6069c7f3c0911303f) )
	ROM_LOAD( "dl_b_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_b_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_b_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )
ROM_END

ROM_START( dlairc )		/* revision C */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_c_u1.bin", 0x0000, 0x2000,  CRC(cebfe26a) SHA1(1c808de5c92fef67d8088621fbd743c1a0a3bb5e) )
	ROM_LOAD( "dl_c_u2.bin", 0x2000, 0x2000,  CRC(6751103d) SHA1(e94e19f738e0eb69700e56c6069c7f3c0911303f) )
	ROM_LOAD( "dl_c_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_c_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_c_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )
ROM_END

ROM_START( dlaird )		/* revision D */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_d_u1.bin", 0x0000, 0x2000,  CRC(0b5ab120) SHA1(6ec59d6aaa27994d8de4f5635935fd6c1d42d2f6) )
	ROM_LOAD( "dl_d_u2.bin", 0x2000, 0x2000,  CRC(93ebfffb) SHA1(2a8f6d7ab18845e22a2ba238b44d7c636908a125) )
	ROM_LOAD( "dl_d_u3.bin", 0x4000, 0x2000,  CRC(22e6591f) SHA1(3176c07af6d942496c9ae338e3b93e28e2ce7982) )
	ROM_LOAD( "dl_d_u4.bin", 0x6000, 0x2000,  CRC(5f7212cb) SHA1(69c34de1bb44b6cd2adc2947d00d8823d3e87130) )
	ROM_LOAD( "dl_d_u5.bin", 0x8000, 0x2000,  CRC(2b469c89) SHA1(646394b51325ca9163221a43b5af64a8067eb80b) )
ROM_END

ROM_START( dlaire )		/* revision E */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_e_u1.bin", 0x0000, 0x2000,  CRC(02980426) SHA1(409de05045adbd054bc1fda24d4a9672832e2fae) )
	ROM_LOAD( "dl_e_u2.bin", 0x2000, 0x2000,  CRC(979d4c97) SHA1(5da6ceab5029ac5f5846bf52841675c5c70b17af) )
	ROM_LOAD( "dl_e_u3.bin", 0x4000, 0x2000,  CRC(897bf075) SHA1(d2ff9c2fec37544cfe8fb60273524c6610488502) )
	ROM_LOAD( "dl_e_u4.bin", 0x6000, 0x2000,  CRC(4ebffba5) SHA1(d04711247ffa88e371ec461465dd75a8158d90bc) )
ROM_END

ROM_START( dlairf )		/* revision F */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_f_u1.bin", 0x0000, 0x2000,  CRC(06fc6941) SHA1(ea8cf6d370f89d60721ab00ec58ff24027b5252f) )
	ROM_LOAD( "dl_f_u2.bin", 0x2000, 0x2000,  CRC(dcc1dff2) SHA1(614ca8f6c5b6fa1d590f6b80d731377faa3a65a9) )
	ROM_LOAD( "dl_f_u3.bin", 0x4000, 0x2000,  CRC(ab514e5b) SHA1(29d1015b951f0f2d4e5257497f3bf007c5e2262c) )
	ROM_LOAD( "dl_f_u4.bin", 0x6000, 0x2000,  CRC(a817324e) SHA1(1299c83342fc70932f67bda8ae60bace91d66429) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( pr7820 )
{
	laserdisc_type = LASERDISC_TYPE_PR7820;
}


static DRIVER_INIT( ldv1000 )
{
	laserdisc_type = LASERDISC_TYPE_LDV1000;
}


static DRIVER_INIT( variable )
{
	laserdisc_type = LASERDISC_TYPE_VARIABLE;
}


static DRIVER_INIT( 22vp932 )
{
	laserdisc_type = LASERDISC_TYPE_22VP932;
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAMEL( 1983, dlair,  0,     dlair, dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. F2)", GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlairf, dlair, dlair, dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. F)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlaire, dlair, dlair, dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. E)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlaird, dlair, dlair, dlair,  ldv1000,  ROT0, "Cinematronics", "Dragon's Lair (US Rev. D, Pioneer LD-V1000)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlairc, dlair, dlair, dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. C, Pioneer PR-7820)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlairb, dlair, dlair, dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. B, Pioneer PR-7820)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )
GAMEL( 1983, dlaira, dlair, dlair, dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. A, Pioneer PR-7820)",  GAME_NOT_WORKING | GAME_NO_SOUND, layout_dlair )

