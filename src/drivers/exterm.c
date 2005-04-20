/****************************************************************************

	Exterminator memory map

driver by Zsolt Vasvari and Alex Pasadyn


 Master CPU (TMS34010, all addresses are in bits)

 00000000-000fffff RW Video RAM (256x256x15)
 00c00000-00ffffff RW RAM
 01000000-010fffff  W Host Control Interface (HSTADRL)
 01100000-011fffff  W Host Control Interface (HSTADRH)
 01200000-012fffff RW Host Control Interface (HSTDATA)
 01300000-013fffff  W Host Control Interface (HSTCTLH)
 01400000-01400007 R  Input Port 0
 01400008-0140000f R  Input Port 1
 01440000-01440007 R  Input Port 2
 01440008-0144000f R  Input Port 3
 01480000-01480007 R  Input Port 4
 01500000-0150000f  W Output Port 0 (See machine/exterm.c)
 01580000-0158000f  W Sound Command
 015c0000-015c000f  W Watchdog
 01800000-01807fff RW Palette RAM
 02800000-02807fff RW EEPROM
 03000000-03ffffff R  ROM
 3f000000-3fffffff R  ROM Mirror
 c0000000-c00001ff RW TMS34010 I/O Registers
 ff000000-ffffffff R  ROM Mirror


 Slave CPU (TMS34010, all addresses are in bits)

 00000000-000fffff RW Video RAM (2 banks of 256x256x8)
 c0000000-c00001ff RW TMS34010 I/O Registers
 ff800000-ffffffff RW RAM


 DAC Controller CPU (6502)

 0000-07ff RW RAM
 4000      R  Sound Command
 8000-8001  W 2 Channels of DAC output
 8000-ffff R  ROM


 YM2151 Controller CPU (6502)

 0000-07ff RW RAM
 4000       W YM2151 Command/Data Register (Controlled by a bit A000)
 6000  		W NMI occurence rate (fed into a binary counter)
 6800      R  Sound Command
 7000      R  Causes NMI on DAC CPU
 8000-ffff R  ROM
 a000       W Control register (see sndhrdw/gottlieb.c)

****************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "sound/dac.h"
#include "sound/2151intf.h"

static size_t code_rom_size;
static data16_t *exterm_code_rom;
static data16_t *exterm_master_speedup, *exterm_slave_speedup;

extern data16_t *exterm_master_videoram, *exterm_slave_videoram;

static data8_t aimpos[2];
static data8_t trackball_old[2];


/* Functions in vidhrdw/exterm.c */
PALETTE_INIT( exterm );
VIDEO_START( exterm );

VIDEO_UPDATE( exterm );
void exterm_to_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_to_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_slave(unsigned int address, unsigned short* shiftreg);

/* Functions in sndhrdw/gottlieb.c */
void gottlieb_sound_init(void);
WRITE16_HANDLER( gottlieb_sh_word_w );
READ8_HANDLER( gottlieb_cause_dac_nmi_r );
WRITE8_HANDLER( gottlieb_nmi_rate_w );
WRITE8_HANDLER( exterm_sound_control_w );
WRITE8_HANDLER( exterm_ym2151_w );
WRITE8_HANDLER( exterm_dac_vol_w );
WRITE8_HANDLER( exterm_dac_data_w );


static MACHINE_INIT( exterm )
{
	gottlieb_sound_init();
}


/*************************************
 *
 *	Master/slave communications
 *
 *************************************/

WRITE16_HANDLER( exterm_host_data_w )
{
	tms34010_host_w(1, offset / TOWORD(0x00100000), data);
}


READ16_HANDLER( exterm_host_data_r )
{
	return tms34010_host_r(1, TMS34010_HOST_DATA);
}



/*************************************
 *
 *	Input port handlers
 *
 *************************************/

static data16_t exterm_trackball_port_r(int which, data16_t mem_mask)
{
	data16_t port;

	/* Read the fake input port */
	data8_t trackball_pos = readinputport(3 + which);

	/* Calculate the change from the last position. */
	data8_t trackball_diff = trackball_old[which] - trackball_pos;

	/* Store the new position for the next comparision. */
	trackball_old[which] = trackball_pos;

	/* Move the sign bit to the high bit of the 6-bit trackball count. */
	if (trackball_diff & 0x80)
		trackball_diff |= 0x20;

	/* Keep adding the changes.  The counters will be reset later by a hardware write. */
	aimpos[which] = (aimpos[which] + trackball_diff) & 0x3f;

	/* Combine it with the standard input bits */
	port = which ? input_port_1_word_r(0, mem_mask) :
				   input_port_0_word_r(0, mem_mask);

	return (port & 0xc0ff) | (aimpos[which] << 8);
}

READ16_HANDLER( exterm_input_port_0_r )
{
	return exterm_trackball_port_r(0, mem_mask);
}

READ16_HANDLER( exterm_input_port_1_r )
{
	return exterm_trackball_port_r(1, mem_mask);
}



/*************************************
 *
 *	Output port handlers
 *
 *************************************/

WRITE16_HANDLER( exterm_output_port_0_w )
{
	/* All the outputs are activated on the rising edge */

	static data16_t last = 0;

	if (ACCESSING_LSB)
	{
		/* Bit 0-1= Resets analog controls */
		if ((data & 0x0001) && !(last & 0x0001))
			aimpos[0] = 0;

		if ((data & 0x0002) && !(last & 0x0002))
			aimpos[1] = 0;
	}

	if (ACCESSING_MSB)
	{
		/* Bit 13 = Resets the slave CPU */
		if ((data & 0x2000) && !(last & 0x2000))
			cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);

		/* Bits 14-15 = Coin counters */
		coin_counter_w(0, data & 0x8000);
		coin_counter_w(1, data & 0x4000);
	}

	COMBINE_DATA(&last);
}



/*************************************
 *
 *	Speedup handlers
 *
 *************************************/

READ16_HANDLER( exterm_master_speedup_r )
{
	int value = exterm_master_speedup[offset];

	/* Suspend cpu if it's waiting for an interrupt */
	if (activecpu_get_pc() == 0xfff4d9b0 && !value)
		cpu_spinuntil_int();

	return value;
}

WRITE16_HANDLER( exterm_slave_speedup_w )
{
	/* Suspend cpu if it's waiting for an interrupt */
	if (activecpu_get_pc() == 0xfffff050)
		cpu_spinuntil_int();

	COMBINE_DATA(&exterm_slave_speedup[offset]);
}

READ8_HANDLER( exterm_sound_dac_speedup_r )
{
	UINT8 *RAM = memory_region(REGION_CPU3);
	int value = RAM[0x0007];

	/* Suspend cpu if it's waiting for an interrupt */
	if (activecpu_get_pc() == 0x8e79 && !value)
		cpu_spinuntil_int();

	return value;
}

READ8_HANDLER( exterm_sound_ym2151_speedup_r )
{
	/* Doing this won't flash the LED, but we're not emulating that anyhow, so
	   it doesn't matter */
	UINT8 *RAM = memory_region(REGION_CPU4);
	int value = RAM[0x02b6];

	/* Suspend cpu if it's waiting for an interrupt */
	if (activecpu_get_pc() == 0x8179 && !(value & 0x80) &&  RAM[0x00bc] == RAM[0x00bb] &&
		RAM[0x0092] == 0x00 &&  RAM[0x0093] == 0x00 && !(RAM[0x0004] & 0x80))
		cpu_spinuntil_int();

	return value;
}



/*************************************
 *
 *	Master/slave memory maps
 *
 *************************************/

static ADDRESS_MAP_START( master_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x00c00000, 0x00ffffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x01200000, 0x012fffff) AM_READ(exterm_host_data_r)
	AM_RANGE(0x01400000, 0x0140000f) AM_READ(exterm_input_port_0_r)
	AM_RANGE(0x01440000, 0x0144000f) AM_READ(exterm_input_port_1_r)
	AM_RANGE(0x01480000, 0x0148000f) AM_READ(input_port_2_word_r)
	AM_RANGE(0x01800000, 0x01807fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x02800000, 0x02807fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x03000000, 0x03ffffff) AM_READ(MRA16_BANK1)
	AM_RANGE(0x3f000000, 0x3fffffff) AM_READ(MRA16_BANK2)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READ(tms34010_io_register_r)
	AM_RANGE(0xff000000, 0xffffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( master_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_WRITE(MWA16_RAM) AM_BASE(&exterm_master_videoram)
	AM_RANGE(0x00c00000, 0x00ffffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x01000000, 0x013fffff) AM_WRITE(exterm_host_data_w)
	AM_RANGE(0x01500000, 0x0150000f) AM_WRITE(exterm_output_port_0_w)
	AM_RANGE(0x01580000, 0x0158000f) AM_WRITE(gottlieb_sh_word_w)
	AM_RANGE(0x015c0000, 0x015c000f) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x01800000, 0x01807fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x02800000, 0x02807fff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size) /* EEPROM */
	AM_RANGE(0xc0000000, 0xc00001ff) AM_WRITE(tms34010_io_register_w)
	AM_RANGE(0xff000000, 0xffffffff) AM_WRITE(MWA16_ROM) AM_BASE(&exterm_code_rom) AM_SIZE(&code_rom_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( slave_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READ(tms34010_io_register_r)
	AM_RANGE(0xff800000, 0xffffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_WRITE(MWA16_RAM) AM_BASE(&exterm_slave_videoram)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_WRITE(tms34010_io_register_w)
	AM_RANGE(0xff800000, 0xffffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END



/*************************************
 *
 *	Audio memory maps
 *
 *************************************/

static ADDRESS_MAP_START( sound_dac_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_READ(soundlatch_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_dac_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(exterm_dac_vol_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(exterm_dac_data_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_ym2151_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_READ(soundlatch_r)
	AM_RANGE(0x7000, 0x7000) AM_READ(gottlieb_cause_dac_nmi_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_ym2151_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(exterm_ym2151_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(gottlieb_nmi_rate_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(exterm_sound_control_w)
ADDRESS_MAP_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( exterm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x3f00, IP_ACTIVE_LOW, IPT_SPECIAL) /* trackball data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x3f00, IP_ACTIVE_LOW, IPT_SPECIAL) /* trackball data */
	PORT_BIT( 0xc000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) ) /* According to the test screen */
	PORT_DIPSETTING(	  0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	/* Note that the coin settings don't match the setting shown on the test screen,
	   but instead what the game appears to used. This is either a bug in the game,
	   or I don't know what else. */
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Memory Test" )
	PORT_DIPSETTING(      0x0040, "Once" )
	PORT_DIPSETTING(      0x0000, "Continous" )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN3, fake trackball input port */
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(KEYCODE_X)

	PORT_START /* IN4, fake trackball input port. */
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(2) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_M)

INPUT_PORTS_END



/*************************************
 *
 *	34010 configurations
 *
 *************************************/

static struct tms34010_config master_config =
{
	0,							/* halt on reset */
	NULL,						/* generate interrupt */
	exterm_to_shiftreg_master,	/* write to shiftreg function */
	exterm_from_shiftreg_master	/* read from shiftreg function */
};

static struct tms34010_config slave_config =
{
	1,							/* halt on reset */
	NULL,						/* generate interrupt */
	exterm_to_shiftreg_slave,	/* write to shiftreg function */
	exterm_from_shiftreg_slave	/* read from shiftreg function */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( exterm )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010,40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(master_config)
	MDRV_CPU_PROGRAM_MAP(master_readmem,master_writemem)

	MDRV_CPU_ADD(TMS34010,40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(slave_config)
	MDRV_CPU_PROGRAM_MAP(slave_readmem,slave_writemem)

	MDRV_CPU_ADD(M6502, 2000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_dac_readmem,sound_dac_writemem)

	MDRV_CPU_ADD(M6502, 2000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_ym2151_readmem,sound_ym2151_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (263 - 240)) / (60 * 263))
	MDRV_INTERLEAVE(1675)	// anything lower will have drop outs on the drums

	MDRV_MACHINE_INIT(exterm)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_PALETTE_LENGTH(4096+32768)

	MDRV_PALETTE_INIT(exterm)
	MDRV_VIDEO_START(exterm)
	MDRV_VIDEO_UPDATE(exterm)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( exterm )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for TMS34010 #1 */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* dummy region for TMS34010 #2 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* 64k for DAC code */
	ROM_LOAD( "v101d1", 0x8000, 0x8000, CRC(83268b7d) SHA1(a9139e80e2382122e9919c0555937e120d4414cf) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )		/* 64k for YM2151 code */
	ROM_LOAD( "v101y1", 0x8000, 0x8000, CRC(cbeaa837) SHA1(87d8a258f059512dbf9bc0e7cfff728ef9e616f1) )

	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )	/* 2MB for 34010 code */
	ROM_LOAD16_BYTE( "v101bg0",  0x000000, 0x10000, CRC(8c8e72cf) SHA1(5e0fa805334f54f7e0293ea400bacb0e3e79ed56) )
	ROM_LOAD16_BYTE( "v101bg1",  0x000001, 0x10000, CRC(cc2da0d8) SHA1(4ac23048d3ca771e315388603ad3b1b25030d6ff) )
	ROM_LOAD16_BYTE( "v101bg2",  0x020000, 0x10000, CRC(2dcb3653) SHA1(2d74b58b02ae0587e3789d69feece268f582f226) )
	ROM_LOAD16_BYTE( "v101bg3",  0x020001, 0x10000, CRC(4aedbba0) SHA1(73b7e4864b1e71103229edd3cae268ab91144ef2) )
	ROM_LOAD16_BYTE( "v101bg4",  0x040000, 0x10000, CRC(576922d4) SHA1(c8cdfb0727c9f1f6e2d2008611372f386fd35fc4) )
	ROM_LOAD16_BYTE( "v101bg5",  0x040001, 0x10000, CRC(a54a4bc2) SHA1(e0f3648454cafeee1f3f58af03489d3256f66965) )
	ROM_LOAD16_BYTE( "v101bg6",  0x060000, 0x10000, CRC(7584a676) SHA1(c9bc651f90ab752f73e735cb80e5bb109e2cac5f) )
	ROM_LOAD16_BYTE( "v101bg7",  0x060001, 0x10000, CRC(a4f24ff6) SHA1(adabbe1c93beb4fcc6fa2f13e687a866fb54fbdb) )
	ROM_LOAD16_BYTE( "v101bg8",  0x080000, 0x10000, CRC(fda165d6) SHA1(901bdede00a936c0160d9fea8a2975ff893e52d0) )
	ROM_LOAD16_BYTE( "v101bg9",  0x080001, 0x10000, CRC(e112a4c4) SHA1(8938d6857b3c5cd3f5560496e087e3b3ff3dab81) )
	ROM_LOAD16_BYTE( "v101bg10", 0x0a0000, 0x10000, CRC(f1a5cf54) SHA1(749531036a1100e092b7edfba14097d5aaab26aa) )
	ROM_LOAD16_BYTE( "v101bg11", 0x0a0001, 0x10000, CRC(8677e754) SHA1(dd8135de8819096150914798ab37a17ae396af32) )
	ROM_LOAD16_BYTE( "v101fg0",  0x180000, 0x10000, CRC(38230d7d) SHA1(edd575192c0376183c415c61a3c3f19555522549) )
	ROM_LOAD16_BYTE( "v101fg1",  0x180001, 0x10000, CRC(22a2bd61) SHA1(59ed479b8ae8328014be4e2a5575d00105fd83f3) )
	ROM_LOAD16_BYTE( "v101fg2",  0x1a0000, 0x10000, CRC(9420e718) SHA1(1fd9784d40e496ebc4772baff472eb25b5106725) )
	ROM_LOAD16_BYTE( "v101fg3",  0x1a0001, 0x10000, CRC(84992aa2) SHA1(7dce2bef695c2a9b5a03d217bbff8fbece459a92) )
	ROM_LOAD16_BYTE( "v101fg4",  0x1c0000, 0x10000, CRC(38da606b) SHA1(59479ff99b1748ddc36de32b368dd38cb2965868) )
	ROM_LOAD16_BYTE( "v101fg5",  0x1c0001, 0x10000, CRC(842de63a) SHA1(0b292a8b7f4b86a2d3bd6b5b7ec0287e2bf88263) )
	ROM_LOAD16_BYTE( "v101p0",   0x1e0000, 0x10000, CRC(6c8ee79a) SHA1(aa051e33e3ed6eed475a37e5dae1be0ac6471b12) )
	ROM_LOAD16_BYTE( "v101p1",   0x1e0001, 0x10000, CRC(557bfc84) SHA1(8d0f1b40adbf851a85f626663956f3726ca8026d) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

DRIVER_INIT( exterm )
{
	memcpy(exterm_code_rom, memory_region(REGION_USER1), code_rom_size);

	/* install speedups */
	exterm_master_speedup = memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x00c800e0, 0x00c800ef, 0, 0, exterm_master_speedup_r);
	exterm_slave_speedup = memory_install_write16_handler(1, ADDRESS_SPACE_PROGRAM, 0xfffffb90, 0xfffffb9f, 0, 0, exterm_slave_speedup_w);
	memory_install_read8_handler(2, ADDRESS_SPACE_PROGRAM, 0x0007, 0x0007, 0, 0, exterm_sound_dac_speedup_r);
	memory_install_read8_handler(3, ADDRESS_SPACE_PROGRAM, 0x02b6, 0x02b6, 0, 0, exterm_sound_ym2151_speedup_r);

	/* set up mirrored ROM access */
	cpu_setbank(1, exterm_code_rom);
	cpu_setbank(2, exterm_code_rom);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1989, exterm, 0, exterm, exterm, exterm, ROT0, "Gottlieb / Premier Technology", "Exterminator" )
