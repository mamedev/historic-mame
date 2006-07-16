/***************************************************************************

    Atari Crystal Castles hardware

    driver by Pat Lawrence

    Games supported:
        * Crystal Castles (1983) [3 sets]

    Known issues:
        * none at this time

****************************************************************************

    Sync chain:
      _   _   _   _   _   _   _   _   _   _   _   _   _   _
    _| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |  10MHz
      ___     ___     ___     ___     ___     ___     ___
    _|   |___|   |___|   |___|   |___|   |___|   |___|   |__  5MHz, J@8L
    _     ___     ___     ___     ___     ___     ___     __
     |___|   |___|   |___|   |___|   |___|   |___|   |___|    /5MHz, /K@8L
          _______         _______         _______         __
    _____|       |_______|       |_______|       |_______|    Q@8L
    _____         _______         _______         _______
         |_______|       |_______|       |_______|       |__  /Q@8L, /1H = 2.5MHz

    Pixel clock = 5MHz
    1H = 2.5MHz
    HSYNC = 32H clocked by 16H fter 256H goes high, or at H=256+32+16=304
    HBLANK = 256H
    /HBLANK1 = HBLANK clocked by 4H, or at H=256+4=260
    HBLANK2 = HBLANK1 clocked by /4H, or at H=260+4=264

    According to the schematics, there is no way to reset the horizontal
    sync chain, which would count it up to 512 before resetting. But this
    doesn't work out. On the assumption, then, that the falling edge of
    HSYNC should reset the horizontal chain, that would happen at
    H=304+16=320. This makes sense as 5MHz/320 = 15.625kHz, which is the
    right frequency for a standard res. monitor.

    The vertical sync chain appears to just count 0..255 and wrap. The
    PROM at 7k (.108) controls the IRQCK, VSYNC, and VBLANK signals. With
    the standard PROM, VBLANK is held from V=0 through V=31. VSYNC is
    held from V=4 through V=6. And the IRQ clock has a rising edge at
    V=32, V=96, V=160, and V=224.

****************************************************************************

    Crystal Castles memory map.

     Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
              1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
              5 4 3 2 1 0                      W
    -------------------------------------------------------------------------------
    0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
    0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
    0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
    0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
    0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
    8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
    8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
    -------------------------------------------------------------------------------
    8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
                                          0 0 R/W D D D D D D D D  MOB Picture
                                          0 1 R/W D D D D D D D D  MOB Vertical
                                          1 0 R/W D D D D D D D D  MOB Priority
                                          1 1 R/W D D D D D D D D  MOB Horizontal
    -------------------------------------------------------------------------------
    9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
    9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
    9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
    9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
    9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                               R                D  COIN R
                                               R              D    COIN L
                                               R            D      COIN AUX
                                               R          D        SLAM
                                               R        D          SELF TEST
                                               R      D            VBLANK
                                               R    D              JMP1
                                               R  D                JMP2
    -------------------------------------------------------------------------------
    9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
    9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
    9A08                                                    D D D  Option SW
                                                          D        SPARE
                                                        D          SPARE
                                                      D            SPARE
    9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
    -------------------------------------------------------------------------------
    9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
    9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
    9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
    9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
              1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
    9E80                                0 0 0  W                D  Trak Ball Light P1
    9E81                                0 0 1  W                D  Trak Ball Light P2
    9E82                                0 1 0  W                D  Store Low
    9E83                                0 1 1  W                D  Store High
    9E84                                1 0 0  W                D  Spare
    9E85                                1 0 1  W                D  Coin Counter R
    9E86                                1 1 0  W                D  Coin Counter L
    9E87                                1 1 1  W                D  BANK0-BANK1
              1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
    9F00                                0 0 0  W          D        ^AX
    9F01                                0 0 1  W          D        ^AY
    9F02                                0 1 0  W          D        ^XINC
    9F03                                0 1 1  W          D        ^YINC
    9F04                                1 0 0  W          D        PLAYER2 (flip screen)
    9F05                                1 0 1  W          D        ^SIRE
    9F06                                1 1 0  W          D        BOTHRAM
    9F07                                1 1 1  W          D        BUF1/^BUF2 (sprite bank)
    9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
    A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

***************************************************************************/

#include "driver.h"
#include "sound/pokey.h"
#include "ccastles.h"


#define MASTER_CLOCK	(10000000)



/*************************************
 *
 *  Globals
 *
 *************************************/

static const UINT8 *syncprom;
static UINT8 irq_state;
static mame_timer *irq_timer;

int ccastles_vblank_start;
int ccastles_vblank_end;



/*************************************
 *
 *  VBLANK and IRQ generation
 *
 *************************************/

INLINE void schedule_next_irq(int curscanline)
{
	/* scan for a rising edge on the IRQCK signal */
	for (curscanline++; ; curscanline = (curscanline + 1) & 0xff)
		if ((syncprom[(curscanline - 1) & 0xff] & 8) == 0 && (syncprom[curscanline] & 8) != 0)
			break;

	/* next one at the start of this scanline */
	timer_adjust(irq_timer, cpu_getscanlinetime(curscanline), curscanline, 0);
}


static void clock_irq(int param)
{
	/* assert the IRQ if not already asserted */
	if (!irq_state)
	{
		cpunum_set_input_line(0, 0, ASSERT_LINE);
		irq_state = 1;
	}

	/* find the next edge */
	schedule_next_irq(param);
}


static UINT32 get_vblank(void *param)
{
	int scanline = cpu_getscanline();
	return syncprom[scanline & 0xff] & 1;
}



/*************************************
 *
 *  Machine setup
 *
 *************************************/

static MACHINE_START( ccastles )
{
	/* initialize globals */
	syncprom = memory_region(REGION_PROMS) + 0x000;

	/* find the start of VBLANK in the SYNC PROM */
	for (ccastles_vblank_start = 0; ccastles_vblank_start < 256; ccastles_vblank_start++)
		if ((syncprom[(ccastles_vblank_start - 1) & 0xff] & 1) == 0 && (syncprom[ccastles_vblank_start] & 1) != 0)
			break;
	if (ccastles_vblank_start == 0)
		ccastles_vblank_start = 256;

	/* find the end of VBLANK in the SYNC PROM */
	for (ccastles_vblank_end = 0; ccastles_vblank_end < 256; ccastles_vblank_end++)
		if ((syncprom[(ccastles_vblank_end - 1) & 0xff] & 1) != 0 && (syncprom[ccastles_vblank_end] & 1) == 0)
			break;

	/* can't handle the wrapping case */
	assert(ccastles_vblank_end < ccastles_vblank_start);

	/* reconfigure the visible area to match */
	set_visible_area(0, Machine->visible_area[0].min_x, Machine->visible_area[0].max_x, ccastles_vblank_end, ccastles_vblank_start - 1);

	/* configure the ROM banking */
	memory_configure_bank(1, 0, 2, memory_region(REGION_CPU1) + 0xa000, 0x6000);

	/* create a timer for IRQs and set up the first callback */
	irq_timer = timer_alloc(clock_irq);
	irq_state = 0;
	schedule_next_irq(0);

	/* setup for save states */
	state_save_register_global(irq_state);

	return 0;
}


static MACHINE_RESET( ccastles )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	irq_state = 0;
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE8_HANDLER( irq_ack_w )
{
	if (irq_state)
	{
		cpunum_set_input_line(0, 0, CLEAR_LINE);
		irq_state = 0;
	}
}


static WRITE8_HANDLER( ccastles_led_w )
{
	set_led_status(offset, ~data & 1);
}


static WRITE8_HANDLER( ccastles_coin_counter_w )
{
	coin_counter_w(offset, data & 1);
}


static WRITE8_HANDLER( ccastles_bankswitch_w )
{
	memory_set_bank(1, data & 1);
}


static READ8_HANDLER( leta_r )
{
	return readinputport(2 + offset);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

/* complete memory map derived from schematics */
static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(ccastles_bitmode_r, ccastles_bitmode_w)
	AM_RANGE(0x0000, 0x7fff) AM_READWRITE(MRA8_RAM, ccastles_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x8e00, 0x8fff) AM_BASE(&spriteram)
	AM_RANGE(0x9000, 0x90ff) AM_MIRROR(0x0300) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9400, 0x9403) AM_MIRROR(0x01fc) AM_READ(leta_r)
	AM_RANGE(0x9600, 0x97ff) AM_READ(input_port_0_r)
	AM_RANGE(0x9800, 0x980f) AM_MIRROR(0x01f0) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x9a00, 0x9a0f) AM_MIRROR(0x01f0) AM_READWRITE(pokey2_r, pokey2_w)
	AM_RANGE(0x9c00, 0x9c7f) /* /RECALL */
	AM_RANGE(0x9c80, 0x9cff) AM_WRITE(ccastles_hscroll_w)
	AM_RANGE(0x9d00, 0x9d7f) AM_WRITE(ccastles_vscroll_w)
	AM_RANGE(0x9d80, 0x9dff) AM_WRITE(irq_ack_w)
	AM_RANGE(0x9e00, 0x9e7f) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x9e80, 0x9e81) AM_MIRROR(0x0078) AM_WRITE(ccastles_led_w)
	AM_RANGE(0x9e82, 0x9e83) AM_MIRROR(0x0078) /* STORE */
	AM_RANGE(0x9e85, 0x9e86) AM_MIRROR(0x0078) AM_WRITE(ccastles_coin_counter_w)
	AM_RANGE(0x9e87, 0x9e87) AM_MIRROR(0x0078) AM_WRITE(ccastles_bankswitch_w)
	AM_RANGE(0x9f00, 0x9f07) AM_MIRROR(0x0078) AM_WRITE(ccastles_video_control_w)
	AM_RANGE(0x9f80, 0x9fbf) AM_MIRROR(0x0040) AM_WRITE(ccastles_paletteram_w)
	AM_RANGE(0xa000, 0xdfff) AM_ROMBANK(1)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( ccastles )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_vblank, 0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )					/* 1p Jump, non-cocktail start1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)		/* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("LETA0")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START_TAG("LETA1")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30)

	PORT_START_TAG("LETA2")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_Y ) PORT_COCKTAIL PORT_SENSITIVITY(10) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START_TAG("LETA3")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_X ) PORT_COCKTAIL PORT_SENSITIVITY(10) PORT_KEYDELTA(30)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

/* technically, this is 4bpp graphics, but the top bit is thrown away during
    processing to make room for the priority bit in the sprite buffers */
static const gfx_layout ccastles_spritelayout =
{
	8,16,
	RGN_FRAC(1,2),
	3,
	{ RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &ccastles_spritelayout,  0, 2 },
	{ -1 }
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	{ 0 },
	input_port_1_r
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ccastles )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, MASTER_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_FRAMES_PER_SECOND((float)(MASTER_CLOCK/2) / 320.0f / 256.0f)
	MDRV_VBLANK_DURATION(0)		/* VBLANK is handled manually */

	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_WATCHDOG_VBLANK_INIT(8)
	MDRV_MACHINE_START(ccastles)
	MDRV_MACHINE_RESET(ccastles)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 231)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(ccastles)
	MDRV_VIDEO_UPDATE(ccastles)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, MASTER_CLOCK/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(POKEY, MASTER_CLOCK/8)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( ccastles )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-403.bin", 0x0a000, 0x2000, CRC(81471ae5) SHA1(8ec13b48119ecf8fe85207403c0a0de5240cded4) )
     ROM_LOAD( "136022-404.bin", 0x0c000, 0x2000, CRC(820daf29) SHA1(a2cff00e9ddce201344692b75038431e4241fedd) )
     ROM_LOAD( "136022-405.bin", 0x0e000, 0x2000, CRC(4befc296) SHA1(2e789a32903808014e9d5f3021d7eff57c3e2212) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Sync PROM */
     ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM */
     ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* VRAM Write Protect PROM */
     ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color/Priority PROM */
ROM_END


ROM_START( ccastle3 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-303.bin", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
     ROM_LOAD( "136022-304.bin", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
     ROM_LOAD( "136022-305.bin", 0x0e000, 0x2000, CRC(9418cf8a) SHA1(1f835db94270e4a16e721b2ac355fb7e7c052285) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Sync PROM */
     ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM */
     ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* VRAM Write Protect PROM */
     ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color/Priority PROM */
ROM_END


ROM_START( ccastle2 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-203.bin", 0x0a000, 0x2000, CRC(348a96f0) SHA1(76de7bf6a01ccb15a4fe7333c1209f623a2e0d1b) )
     ROM_LOAD( "136022-204.bin", 0x0c000, 0x2000, CRC(d48d8c1f) SHA1(8744182a3e2096419de63e341feb77dd8a8bcb34) )
     ROM_LOAD( "136022-205.bin", 0x0e000, 0x2000, CRC(0e4883cc) SHA1(a96abbf654e087409a90c1686d9dd553bd08c14e) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Sync PROM */
     ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM */
     ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* VRAM Write Protect PROM */
     ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color/Priority PROM */
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, ccastles, 0,        ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 4)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastle3, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastle2, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 2)", GAME_SUPPORTS_SAVE )
