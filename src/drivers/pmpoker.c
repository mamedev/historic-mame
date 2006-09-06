/******************************************************************************

    PLAYMAN POKER
    -------------

    Driver by Roberto Fresca.


    Games running on this hardware:

    * PlayMan Poker (Germany).


    I think "Golden Poker Double Up" from Bonanza Enterprises, should run on this hardware too.
    http://www.arcadeflyers.com/?page=thumbs&id=4616


    General Notes:

    - This set was found as "unknown playman-poker".
    - The ROMs didn't match any currently supported set (0.108u2 romident switch).
    - There are not technical notes, references, or documents about hardware inside the zip file.
    - The dump is quite incomplete. At least 1 graphics bank is missing.
    - All this driver was made using reverse engineering in the program roms.


    List of components:

    - CPU:      1x M6502.
    - Video:    1x MC6845.
    - I/O       2x 6520 or 6821 PIAs.
    - prg ROMs: 3x 2732 (32Kb) or similar.
    - gfx ROMs: 1x 2716 (16Kb) or similar. (should be at least 3 more)
    - sound:    ???


    Some odds:

    - There are pieces of code like the following sub:

    78DE: 18         clc
    78DF: 69 07      adc  #$07
    78E1: 9D 20 10   sta  $1020,x
    78E4: A9 00      lda  #$00
    78E6: 9D 20 18   sta  $1820,x
    78E9: E8         inx

    78EA: 82         DOP        ; use of DOP (double NOP)
    78EB: A2 0A      dummy (ldx #$0A)

    78ED: AD 82 08   lda  $0882

    78F0: 82         DOP        ; use of DOP (double NOP)
    78F1: 48 08      dummy
    78F3: D0 F6      bne  $78EB ; branch to the 1st DOP dummy arguments (now ldx #$0A).
    78F5: CA         dex
    78F6: D0 F8      bne  $78F0
    78F8: 29 10      and  #$10
    78FA: 60         rts

    Offset $78EA and $78F0 contains an undocumented 6502 opcode (0x82).

    At beginning, I thought that the main processor was a 65sc816, since 0x82 is a documented opcode (BRL) for this CPU.
    Even the vector $FFF8 contain 0x09 (used to indicate "Emulation Mode" for the 65sc816).
    I dropped the idea following the code. Impossible to use BRL (branch relative long) in this structure.

    Some 6502 sources list the 0x82 mnemonic as DOP (double NOP), with 2 dummy bytes as argument.
    The above routine dynamically change the X register value using the DOP undocumented opcode.
    Now all have sense.


    --------------------
    ***  Memory Map  ***
    --------------------

    $0000 - $00FF   RAM     ; zero page (pointers and registers)
    $0100 - $01FF   RAM     ; 6502 Stack Pointer.

    $0201 - $0206   ???     ; R/W.
    $0207 - $0213   ???     ; R/W.
    $0381 - $0387   ???     ; R/W (mainly to $0383). still not understood.

    $0800 - $0801   MC6845  ; mc6845 use $0800 for register addressing and $0801 for register values.

                            *** MC6845 init at $65B9 ***

                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x22  0x02  0x1F  0x04  0x1D  0x1E  0x00  0x07  0x00  0x00  0x00  0x00  #$00  #$00  #$00  #$00.

    $0844 - $0847   PIA1    ; writes: 0xFF 0x04 0xFF 0x04.  initialized at $5000.
    $0848 - $084b   PIA2    ; writes: 0xFF 0x04 0xFF 0x04.  initialized at $5000.

    $1000 - $13FF   Video RAM   ; initialized in subroutine starting at $5042.
    $1800 - $1BFF   Color RAM   ; initialized in subroutine starting at $5042.

    $5000 - $7FFF   ROM
    $F000 - $FFFF   ROM     ; mirrored from $7000 - $7FFF for vectors/pointers purposes.


    -------------------------------------------------------------------------


    Driver updates:


    [2006-09-06]

    - Understood the GFX banks:
        - 1 bank (1bpp) for text layer and minor graphics.
        - 1 bank (3bpp) for the undumped cards deck graphics.

    - Partially added inputs through 6821 PIAs.
        ("Bitte techniker rufen" error messages. Press 'W' to reset the machine)

    - Confirmed the CPU as 6502. (was in doubt due to use of illegal opcodes)


    [2006-09-02]

    - Initial release.



    TODO:

    - Check the GFX banks (a new complete dump is needed).
    - Fix Inputs.
    - Check and complete connections in both PIAs.
    - Colors: Find the palette. Maybe exist a missing color PROM.
    - Figure out the sound.


*******************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"
#include "machine/6821pia.h"


/*************************
*     Video Hardware     *
*************************/

static tilemap *bg_tilemap;

WRITE8_HANDLER( pmpoker_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( pmpoker_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
//  - bits -
//  7654 3210
//  --xx xx--   tiles color.
//  ---- --x-   tiles bank.
//  xx-- ---x   seems unused.

	int attr = colorram[tile_index];
	int code = videoram[tile_index];
	int bank = (attr & 0x02) >> 1;	// bit 1 switch the gfx banks.
	int color = (attr & 0x3c) >> 2;	// bits 2-3-4-5 for color.

	SET_TILE_INFO(bank, code, color, 0)
}

VIDEO_START(pmpoker)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 29);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE(pmpoker)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( pmpoker_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM    // zero page (pointers and registers).
	AM_RANGE(0x0100, 0x01ff) AM_RAM    // stack pointer.
	AM_RANGE(0x0200, 0x07ff) AM_RAM

//  AM_RANGE(0x0201, 0x0206) AM_WRITENOP    // writes at beginning.
//  AM_RANGE(0x0381, 0x0387) AM_WRITENOP    // R/W in main code.

	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)    // crtc6845 register addressing.
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)    // crtc6845 register values.

	// $844 - $845 / FF 04 -
	// $846 - $847 / FF 04   \  writes to a couple of PIAs.
	// $848 - $849 / FF 04   /
	// $84a - $84b / FF 04 -

	AM_RANGE(0x0844, 0x0847) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0848, 0x084b) AM_READWRITE(pia_1_r, pia_1_w)

	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(pmpoker_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(pmpoker_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( pmpoker )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_I)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_K)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_L)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tilelayout =
{
	8, 8,
	RGN_FRAC(1,3),
	3,
	{ 0, RGN_FRAC(1,3), RGN_FRAC(2,3) },    // bitplanes are separated.
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 6, 16 },
	{ REGION_GFX2, 0, &tilelayout, 0, 16 },
	{ -1 }
};


/***********************
*    PIA Interfaces    *
**********************/

static const pia6821_interface pia0_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_0_r, input_port_1_r, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};

static const pia6821_interface pia1_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_2_r, input_port_3_r, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( pmpoker )

	MDRV_CPU_ADD(M6502, 10000000/16)	// guessing...
	MDRV_CPU_PROGRAM_MAP(pmpoker_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)           // Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1).
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 29*8-1)    // Taken from MC6845 init, registers 01 & 06.

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_VIDEO_START(pmpoker)
	MDRV_VIDEO_UPDATE(pmpoker)

MACHINE_DRIVER_END


/*************************
*        Rom Load        *
*************************/

ROM_START( pmpoker )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "2-5.bin",	0x5000, 0x1000, CRC(3446a643) SHA1(e67854e3322e238c17fed4e05282922028b5b5ea) )
	ROM_LOAD( "2-6.bin",	0x6000, 0x1000, CRC(50d2d026) SHA1(7f58ab176de0f0f7666d87271af69a845faec090) )
	ROM_LOAD( "2-7.bin",	0x7000, 0x1000, CRC(a9ab972e) SHA1(477441b7ff3acae3a5d5a3e4c2a428e0b3121534) )
	ROM_RELOAD(				0xf000, 0x1000 )	// for vectors/pointers

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-4.bin",	0x0000, 0x0800, CRC(62b9f90d) SHA1(39c61a01225027572fdb75543bb6a78ed74bb2fb) )	// text layer

	/* missing gfx roms: cards deck bitplanes */
	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1-1.bin",	0x0000, 0x0800, NO_DUMP )	// logical guessed rom name.
	ROM_LOAD( "1-2.bin",	0x0800, 0x0800, NO_DUMP )	// logical guessed rom name.
	ROM_LOAD( "1-3.bin",	0x1000, 0x0800, NO_DUMP )	// logical guessed rom name.
ROM_END

/*

U43_2A.bin                                      BADADDR           --xxxxxxxxxxx
U38_5A.bin                                      1ST AND 2ND HALF IDENTICAL
UPS39_12A.bin                                            0xxxxxxxxxxxxxx = 0xFF

*/

ROM_START( bigboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ups39_12a.bin",	0x0000, 0x8000, CRC(216b45fb) SHA1(fbfcd98cc39b2e791cceb845b166ff697f584add) )
	ROM_RELOAD(				0x8000, 0x8000 )	// for vectors/pointers

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u38_5a.bin",	0x0000, 0x2000, CRC(32705e1d) SHA1(84f9305af38179985e0224ae2ea54c01dfef6e12) )	// text layer

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "u43_2a.bin",	0x0000, 0x2000, CRC(10b34856) SHA1(52e4cc81b36b4c807b1d4471c0f7bea66108d3fd) )	// text layer
	ROM_LOAD( "u40_4a.bin",	0x2000, 0x2000, CRC(6f524795) SHA1(eea0e95007ae85c4b06cebd788c7a0ee6254ae74) )	// text layer
	/* only 2bpp? */
ROM_END


/*************************
*      Driver Init       *
*************************/

static DRIVER_INIT( pmpoker )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}


/*************************
*      Game Drivers      *
*************************/

//    YEAR  NAME        PARENT  MACHINE     INPUT       INIT        ROT     COMPANY     FULLNAME                    FLAGS
GAME( 198?,	pmpoker,	0,		pmpoker,	pmpoker,	pmpoker,	ROT0,	"PlayMan?",	"PlayMan Poker (Germany)",	GAME_IMPERFECT_GRAPHICS |GAME_WRONG_COLORS | GAME_NO_SOUND | GAME_NOT_WORKING )
GAME( 198?,	bigboy,	    0,		pmpoker,	pmpoker,	pmpoker,	ROT0,	"Bonanza",	"Big Boy",	GAME_IMPERFECT_GRAPHICS |GAME_WRONG_COLORS | GAME_NO_SOUND | GAME_NOT_WORKING ) // aka golden poker double up??

