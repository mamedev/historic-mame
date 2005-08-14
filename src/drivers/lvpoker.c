/***************************************************************************
Lovely Poker/Pontoon driver, updated by El Condor from work by Uki and Zsolt Vasvari respectively.
Later cleaned up by Curt Coder.

Lovely Cards also runs on this hardware, but is not a gambling game, and is in MAME.

Any fixes for this driver should be forwarded to the AGEMAME forum at (http://www.mameworld.info),
or to MAME, if they affect Lovely Cards.

---

Lovely Poker by Uki/El Condor
Enter switch test mode by pressing the Deal key twice when the
crosshatch pattern comes up.

After you get into Check Mode (F2), press the Deal key to switch pages.

Memory Mapped:

0000-5fff   R   ROM
6000-67ff   RW  Battery Backed RAM
9000-93ff   RW  Video RAM
9400-97ff   RW  Color RAM
                Bits 0-3 - color
                Bits 4-5 - character bank
                Bit  6   - flip x
                Bit  7   - Is it used?
a000        R   Input Port 0
a001        R   Input Port 1
a002        R   Input Port 2
a001        W  Control Port 0
a002        W  Control Port 1

I/O Ports:
00         RW  YM2149 Data  Port
               Port A - DSW #1
               Port B - DSW #2
01          W  YM2149 Control Port

---

Pontoon Memory Map (preliminary)

Pontoon driver by Zsolt Vasvari
Enter switch test mode by pressing the Hit key twice when the
crosshatch pattern comes up
After you get into Check Mode (F2), press the Hit key to switch pages.

Memory Mapped:

0000-5fff   R   ROM
6000-67ff   RW  Battery Backed RAM
8000-83ff   RW  Video RAM
8400-87ff   RW  Color RAM
                Bits 0-3 - color
                Bits 4-5 - character bank
                Bit  6   - flip x
                Bit  7   - Is it used?
a000        R   Input Port 0
a001        R   Input Port 1
a002        R   Input Port 2
a001        W   Control Port 0
a002        W   Control Port 1

I/O Ports:
00         RW  YM2149 Data Port
               Port A - DSW #1
               Port B - DSW #2
01          W  YM2149 Control Port

TODO:

- What do the control ports do? Payout?
- CPU speed/ YM2149 frequencies

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ay8910.h"

extern WRITE8_HANDLER( ponttehk_videoram_w );
extern WRITE8_HANDLER( ponttehk_colorram_w );

extern PALETTE_INIT( lvpoker );
extern PALETTE_INIT( ponttehk );
extern VIDEO_START( ponttehk );
extern VIDEO_UPDATE( ponttehk );

int payout;
static int pulse;

static WRITE8_HANDLER(control_port_2_w)
{
switch (data)
{
case 0x00:
	payout = 0;
	break;
case 0x40:
	payout = 1;
	break;
case 0x60:
	payout = 1;
	break;
case 0x80:
	payout = 1;
	break;
case 0xc0:
	payout = 1;
	break;
}
}
/*************************************************************************************
Hopper Theory of Operation:

When this driver was originally written, it was unclear as to the reasoning behind the
use of the two control ports on the Tehkan gaming PCBs.

Through experimentation, it is believed that they are used to drive a moderately
sophisticated hopper system which controls payout of coins and possibly tokens.

Essentially, all we need concern ourselves with here is control port 2 (There are too many
lamps and things on port 1 to even begin to look for motor on signals).

Whenever a coin needs to be dispensed, a value is written to this port (0x40, 0x80
and 0xc0 have all been recorded). This is interpreted as a signal to activate the
solenoids and dispense coins. It is currently believed that different values are sent
to activate separate payout tubes, to distinguish between coins and tokens.

As each coin is dispensed, an optometric switch connected to 0x40 on Input Port 2 is
triggered, until all coins are dispensed and and the solenoids are deactivated.

Here we cheat a little, we don't distinguish between tubes directly yet. Instead we
check the writes to the control port for the 'magic numbers', and when one occurs, we
set a payout flag, which is then used by the port handler. While perfectly accurate,
it means we can defy the laws of gravity with our coin drop speeds!

There appears to be a bug in Pontoon which stops this from working, although the routines
are identical in both cases, manual pressing of 3 still needs to be done.

*************************************************************************************/
static READ8_HANDLER( payout_r )
{
   if (payout)
   {
       if (pulse)
       pulse = 0;
       else pulse = 0x40;
   }
   return readinputport(2) + pulse;
}



/* Memory Maps */

static ADDRESS_MAP_START( ponttehk_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_WRITE(ponttehk_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8400, 0x87ff) AM_RAM AM_WRITE(ponttehk_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r) AM_WRITENOP // ???
	AM_RANGE(0xa002, 0xa002) AM_READ(payout_r) AM_WRITE(control_port_2_w)//AM_WRITENOP // ???
ADDRESS_MAP_END

static ADDRESS_MAP_START( ponttehk_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lvpoker_map, ADDRESS_SPACE_PROGRAM, 8  )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9000, 0x93ff) AM_RAM AM_WRITE(ponttehk_videoram_w) AM_BASE (&videoram)
	AM_RANGE(0x9400, 0x97ff) AM_RAM AM_WRITE(ponttehk_colorram_w) AM_BASE (&colorram)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r) AM_WRITENOP // ???
	AM_RANGE(0xa002, 0xa002) AM_READ(payout_r) AM_WRITE(control_port_2_w) //AM_READ(input_port_2_r)
	AM_RANGE(0xc000, 0xdfff) AM_ROM
ADDRESS_MAP_END


/* Input Ports */

INPUT_PORTS_START( ponttehk )
	PORT_START_TAG("IN0")
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Reset All")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Clear Stats")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_U)  PORT_NAME("Call Attendant")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_EQUALS) PORT_NAME("Hopper Reset")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Bonus Game")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Stand")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hit")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Use Credit") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Remove Credit as coins") PORT_CODE(KEYCODE_A)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )// Token Drop
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	// Overflow optometric switch - reversed logic
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_3) PORT_NAME("Coinout Sensor") //Token Out
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )//Motor On?

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, "Winning Percentage" )
	PORT_DIPSETTING(    0x06, "70%" )
	PORT_DIPSETTING(    0x05, "74%" )
	PORT_DIPSETTING(    0x04, "78%" )
	PORT_DIPSETTING(    0x03, "82%" )
	PORT_DIPSETTING(    0x02, "86%" )
	PORT_DIPSETTING(    0x07, "90%" )
	PORT_DIPSETTING(    0x01, "94%" )
	PORT_DIPSETTING(    0x00, "98%" )
	PORT_BIT( 0x38, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x60, 0x20, "Payment Method" )
	PORT_DIPSETTING(    0x00, "Credit In/Coin Out" )
	PORT_DIPSETTING(    0x20, "Coin In/Coin Out" )
	PORT_DIPSETTING(    0x40, "Credit In/Credit Out" )
  	//PORT_DIPSETTING(    0x60, "Credit In/Coin Out" ) Again, clearly no Coin in, Credit out
	PORT_DIPNAME( 0x80, 0x80, "Reset All Switch" )
	PORT_DIPSETTING(    0x80, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Coin C (Service Switch)" )
	PORT_DIPSETTING(    0x40, "1 Push/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Push/10 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( lvpoker )
  	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Analyzer") PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_MINUS) PORT_NAME("Memory Reset")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Clear Stats")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_EQUALS) PORT_NAME("Hopper Reset")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Red")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Black")

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CODE(KEYCODE_Z) PORT_NAME("Hold 1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_CODE(KEYCODE_X) PORT_NAME("Hold 2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_CODE(KEYCODE_C) PORT_NAME("Hold 3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_CODE(KEYCODE_V) PORT_NAME("Hold 4")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_CODE(KEYCODE_B) PORT_NAME("Hold 5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Deal/Double")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_2) PORT_NAME("Bet")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Remove Credit as coins") PORT_CODE(KEYCODE_A)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) // Token Drop
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_SPACE) PORT_NAME("Cancel / Take Score")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) // Overflow
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) // Token Out
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x07, 0x07, "Winning Percentage" )
	PORT_DIPSETTING(    0x06, "70%" )
	PORT_DIPSETTING(    0x05, "74%" )
	PORT_DIPSETTING(    0x04, "78%" )
	PORT_DIPSETTING(    0x03, "82%" )
	PORT_DIPSETTING(    0x02, "86%" )
	PORT_DIPSETTING(    0x07, "90%" )
	PORT_DIPSETTING(    0x01, "94%" )
	PORT_DIPSETTING(    0x00, "98%" )
	PORT_DIPNAME( 0x08, 0x08, "Max. Payout Adjustment")
	PORT_DIPSETTING(    0x08, "Free")
	PORT_DIPSETTING(    0x00, "2000")
	PORT_DIPNAME( 0x10, 0x10, "Bonus Game Difficulty")
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x60, 0x20, "Payment Method" )
	PORT_DIPSETTING(    0x00, "Credit In/Coin Out")
	PORT_DIPSETTING(    0x20, "Credit In/Credit Out")
	PORT_DIPSETTING(    0x40, "Coin In/Coin Out")
  	//PORT_DIPSETTING(    0x60, "Credit In/Coin Out") Again, clearly no Coin in, Credit out
	PORT_DIPNAME( 0x80, 0x80, "Memory Reset Switch" )
	PORT_DIPSETTING(    0x80, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Coin C (Service Switch)" )
	PORT_DIPSETTING(    0x40, "1 Push/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Push/10 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

/* Graphics Layouts */

static struct GfxLayout charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

/* Graphics Decode Information */

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 }
};

/* Sound Interfaces */

static struct AY8910interface ay8910_interface =
{
input_port_3_r,	// DSW0
input_port_4_r,	// DSW1
0,
0
};

/* Machine Drivers */

static MACHINE_DRIVER_START( ponttehk )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, 3072000)	// ???
	MDRV_CPU_PROGRAM_MAP(ponttehk_map,0)
	MDRV_CPU_IO_MAP(ponttehk_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_1fill)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(ponttehk)
	MDRV_VIDEO_START(ponttehk)
	MDRV_VIDEO_UPDATE(ponttehk)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910,1536000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( lvpoker )
	MDRV_IMPORT_FROM(ponttehk)

	// basic machine hardware
 	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(lvpoker_map,0)

	// video hardware
	MDRV_PALETTE_INIT(lvpoker)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( ponttehk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ponttehk.001", 0x0000, 0x4000, CRC(1f8c1b38) SHA1(3776ddd695741223bd9ad41f74187bff31f2cd3b) )
	ROM_LOAD( "ponttehk.002", 0x4000, 0x2000, CRC(befb4f48) SHA1(8ca146c8b52afab5deb6f0ff52bdbb2b1ff3ded7) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ponttehk.003", 0x0000, 0x2000, CRC(a6a91b3d) SHA1(d180eabe67efd3fd1205570b661a74acf7ed93b3) )
	ROM_LOAD( "ponttehk.004", 0x2000, 0x2000, CRC(976ed924) SHA1(4d305694b3e157411068baf3052e3aac7d0b32d5) )
	ROM_LOAD( "ponttehk.005", 0x4000, 0x2000, CRC(2b8e8ca7) SHA1(dd86d3b4fd1627bdaa0603ffd2f1bc2953bc51f8) )
	ROM_LOAD( "ponttehk.006", 0x6000, 0x2000, CRC(6bc23965) SHA1(b73a584fc5b2dd9436bbb8bc1620f5a51d351aa8) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "pon24s10.003", 0x0000, 0x0100, CRC(4623b7f3) SHA1(55948753dec09d0a476b90ca75e7e092ce0f68ee) )  /* red component */
	ROM_LOAD( "pon24s10.002", 0x0100, 0x0100, CRC(117e1b67) SHA1(b753137878fe5cd650722cf526cd4929821240a8) )  /* green component */
	ROM_LOAD( "pon24s10.001", 0x0200, 0x0100, CRC(c64ecee8) SHA1(80c9ec21e135235f7f2d41ce7900cf3904123823) )  /* blue component */
ROM_END

ROM_START( lvpoker )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "lp1.bin",	  0x0000, 0x4000, CRC(ecfefa91) SHA1(7f6e0f30a2c4299797a8066419bf247b2900e312) )
	ROM_LOAD( "lp2.bin",	  0x4000, 0x2000, CRC(06d5484f) SHA1(326756a03eaeefc944428c7e011fcdc128aa415a) )
	ROM_LOAD( "lp3.bin",	  0xc000, 0x2000, CRC(05e17de8) SHA1(76b38e414f225789de8af9ca0556008e17285ffe) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "lp4.bin",	  0x0000, 0x2000, CRC(04fd2a6b) SHA1(33fb42f54646dc91f5aca1c55cfc932fa04f5d77) )
	ROM_CONTINUE(			  0x8000, 0x2000 )
	ROM_LOAD( "lp5.bin",	  0x2000, 0x2000, CRC(9b5b531c) SHA1(1ce700361ea39a15c9c62fc0fa61df0cda62a340) )
	ROM_CONTINUE(			  0xa000, 0x2000 )
	ROM_LOAD( "lc6.bin",	  0x4000, 0x2000, CRC(2991a6ec) SHA1(b2c32550884b7b708db48bb7f0854bbad504417d) )
	ROM_RELOAD(				  0xc000, 0x2000 )
	ROM_LOAD( "lp7.bin",	  0x6000, 0x2000, CRC(217e9cfb) SHA1(3e7d76db5195c599c2bf186ae6616b29bc0fd337) )
	ROM_RELOAD(				  0xe000, 0x2000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "3.7c",		  0x0000, 0x0100, CRC(0c2ead4a) SHA1(e8e29e21366622c9bf3ee5e807c83b5cd7da8e3e) )
	ROM_LOAD( "2.7b",		  0x0100, 0x0100, CRC(f4bc51e2) SHA1(38293c1117d6f3076081b6f2da3f42819558b04f) )
	ROM_LOAD( "1.7a",		  0x0200, 0x0100, CRC(e40f2363) SHA1(cea598b6ed037dd3b4306c2ca3b0b4d5197d42a4) )
ROM_END

/* Game Drivers */

GAME( 1985, ponttehk, 0, ponttehk, ponttehk, 0, ROT0, "Tehkan", "Pontoon (Tehkan)" )
GAME( 1985, lvpoker,  0, lvpoker,  lvpoker,  0, ROT0, "Tehkan", "Lovely Poker [BET]" )
