/******************************************************************************

    MAGIC FLY
    ----------

    Driver by Roberto Fresca.


    Games running in this hardware:

    * Magic Fly (P&A Games), 198?
    * 7 e Mezzo (Unknown),   198?    (Not sure about the title. Only english text inside the program rom).


    Hardware notes:

    - CPU 1x R6502P
    - 1x MC6845P
    - 1x oscillator 10.000MHz
    - ROMs: 1x AM27128 (NS3.1)
            2x SEEQ DQ2764 (1, 2)
            1x SGS M2764 (NS1)
    - PLDs: 1x PAL16R4A
    - 1x 4 DIP switches
    - 1x 30x2 edge connector
    - 1x 10 pins male connector
    - 1x trimmer (volume)


    PCB Layout:
     _________________________________________________________________
    |                                                                 |
    |                                                                 |
    |                                    _________   _________        |
    |                                   | 74LS08N | | 74LS32  |       |
    |                                   |_________| |_________|       |
    |                                    _________   _________        |
    |                                   | 74LS138 | | 74HC00  |       |
    |                                   |_________| |_________|       |
    |    ______________                     __________________        |
    |   |              |                   |                  |   ____|
    |   | MK48Z02B-20  |                   |     R6502P       |  |
    |   |______________|                   |__________________|  |
    |  ________________   _________         __________________   |
    | |                | | 74LS157 |       |                  |  |____
    | |    AM27128     | |_________|       |     MC6845P      |   ____|
    | |________________|  _________        |__________________|   ____|
    |                    | 74LS157 |      ________   _________    ____|
    |                    |_________|     | 74LS14 | | 74LS374 |   ____|
    |  ____________       _________      |________| |_________|   ____|
    | | 74LS245    |     | 74LS157 |                 _________    ____|
    | |____________|     |_________|                | 74HCZ44 |   ____|
    |  ____________       _________                 |_________|   ____|
    | | 74LS245    |     | 74LS32  |      _______                 ____|  30x2
    | |____________|     |_________|     | | | | |                ____|  connector
    |  ______________                    |4|3|2|1|                ____|
    | | HM6116       |                   |_|_|_|_|                ____|
    | | o MSM2128    |                                            ____|
    | |______________|                   DIP SW x4                ____|
    |  ______________                                             ____|
    | | HM6116       |    ________       _________                ____|
    | | o MSM2128    |   | 74LS08 |     | 74LS174 |               ____|
    | |______________|   |________|     |_________|               ____|
    |  ________________   __________                              ____|
    | |                | | PAL16R4A |                             ____|
    | |     2764       | |__________|                             ____|
    | |________________|  __________                              ____|
    |  ________________  | 74LS166  |                             ____|
    | |                | |__________|                            |
    | |     2764       |  __________                             |
    | |________________| | 74LS166  |                            |____
    |  ________________  |__________|                               __|
    | |                |  __________       _________               |  |
    | |     2764       | | 74LS166  |     | 74LS05  |              |8 |  10
    | |________________| |__________|     |_________|              |8 |  pins
    |  ________  ______   __________       _________               |8 |  male
    | | 74LS04 || osc. | | 74LS193  |     | 74LS86  |              |8 |  connector
    | |________||10 MHz| |__________|     |_________|              |8 |
    |           |______|                                           |__|
    |_________________________________________________________________|


    NOTE: Magic Fly & 7 e Mezzo PAL16R4A Fuse Maps were converted to the new bin format and added to respective sets.


    Pinouts (7mezzo):
    -----------------
    ************ Edge connector ************

    (Solder Side)               (Parts Side)

    GND                 30      GND
    +10v.               29      +10v.
    +10v.               28      +10v.
    unused              27      unused
    unused              26      unused
    GND                 25      GND
    +12v.               24      +12v.
    +12v.               23      +12v.
    unused              22      unused
    common C (3)        21      common A (1)
    common D (4)        20      common B (2)
    DEAL                19      DOUBLE
    HOLD 1              18      (unreadable)
    HOLD 2              17      HOLD 5
    HOLD 3              16      HOLD 4
    METER               15      BET
    COUPON              14
                        13      COIN 1
    (unreadable)        12      COIN 2
    TAKE                11      PAY
    SMALL (play1)       10      BIG (play3)
    unused              09      unused
    unused              08      unused
    unused              07      unused
    (unreadable)        06      (unreadable)
    sync                05      (unreadable)
    GND                 04      GND
    speaker+            03      speaker+
    speaker- (GND)      02      speaker- (GND)
    +5v.                01      +5v.

    (1) = DOUBLE, DEAL, (unreadable), BET, METER
    (2) = TAKE, SMALL, BIG, PAY
    (3) = HOLD 1, HOLD 2, HOLD 3, HOLD 4, HOLD 5
    (4) = COIN 1, COIN 2, COUPON


    **** Pins connector ****

    pin 01:
    pin 02:
    pin 03:
    pin 04:
    pin 05:  (soldered to pin 01)
    pin 06:  (unreadable)
    pin 07:  (unreadable)
    pin 08:  (unreadable)
    pin 09:  (unreadable)
    pin 10:  common (GND)



    Memory Map (magicfly)
    ------------------------

    $0000 - $00FF    RAM    // Zero page (pointers and registers)

                         ($000D)            // Incremented each time a NMI is triggered.
                         ($001D)            // in case of #$00, NMI do nothing and return.
                         ($0011)            // Store lenghts for text handling.
                         ($0012)            // Store values to be written in color RAM.
                         ($0013 - $0014)    // Pointer to text offset.
                         ($0015 - $0016)    // Pointer to video ram.
                         ($0017 - $0018)    // Pointer to color ram.
                         ($0019)            // Program loops waiting for a value to be written here through NMI. (see below at "other checks")
                         ($003A - $003D)    // Store new read values from $2800 (input port) through NMI.
                         ($003F - $0042)    // Store old read values from $2800 (input port) through NMI.
                         ($005E - $005F)    // Store values to be written in video and color ram, respectively.
                         ($0067 - $0067)    // Program compare the content with #$02, #$03, #$04, #$05 and #$E1.
                                            // If #$E1 is found here, the machine hangs showing "I/O ERROR". (see below at "some clues")
                         ($0096 - $0098)    // Store values from content of $2800 (input port) AND #$80, AND #$40, AND #$10.
                         ($009B - $00A8)    // Text scroll buffer.

    $0100 - $01FF    RAM    // 6502 Stack Pointer.

    $0800 - $0801    (mc6845?)    // At begining, write 18 bytes sequentially in $0801, and the increment (x register) in $0800.

    $1000 - $13FF    Video RAM    // Initialized in subroutine starting at $CF83, filled with value stored in $5E.
    $1800 - $1BFF    Color RAM    // Initialized in subroutine starting at $CF83, filled with value stored in $5F.
                                  // (In 7mezzo is located at $CB13 using $64 and $65 to store video and color ram values.)

                                     CF83: 48         pha
                                     CF84: 8A         txa
                                     CF85: 48         pha
                                     CF86: 98         tya
                                     CF87: 48         pha
                                     CF88: A0 00      ldy  #$00
                                     CF8A: AD 5E 00   lda  $005E
                                     CF8D: 99 00 10   sta  $1000,y
                                     CF90: 99 00 11   sta  $1100,y
                                     CF93: 99 00 12   sta  $1200,y
                                     CF96: 99 00 13   sta  $1300,y
                                     CF99: AD 5F 00   lda  $005F
                                     CF9C: 99 00 18   sta  $1800,y
                                     CF9F: 99 00 19   sta  $1900,y
                                     CFA2: 99 00 1A   sta  $1A00,y
                                     CFA5: 99 00 1B   sta  $1B00,y
                                     CFA8: 88         dey
                                     CFA9: D0 DF      bne  $CF8A
                                     CFAB: 68         pla
                                     CFAC: A8         tay
                                     CFAD: 68         pla
                                     CFAE: AA         tax
                                     CFAF: 68         pla
                                     CFB0: 60         rts

    $1C00 - $27FF    RAM

    $2800 - $2800    Input port    // Input port (code at $CE96). No writes, only reads.
                                   // NMI routine read from here and store new values to $003A - $003D and copy old ones to $003F - $0042.

                               CE96: AD 00 28   lda  $2800
                               CE99: 29 80      and  #$80
                               CE9B: 8D 96 00   sta  $0096
                               CE9E: AD 00 28   lda  $2800
                               CEA1: 29 40      and  #$40
                               CEA3: 8D 97 00   sta  $0097
                               CEA6: AD 00 28   lda  $2800
                               CEA9: 29 10      and  #$10
                               CEAB: 8D 98 00   sta  $0098

    $2801 - $2FFF    RAM

    $3000 - $3000    ???    // Something seems to be mapped here. Only writes, no reads.
                            // Code at $C152 do a complex loop with boolean operations and write #$00/#$80 to $3000. Also NMI routine write another values.
                            // (normally the program execution stuck here)

                               C152: 8A         txa
                               C153: 48         pha
                               C154: 98         tya
                               C155: 48         pha
                               C156: A9 00      lda  #$00
                               C158: 8D 1D 00   sta  $001D
                               C15B: AD 4D 00   lda  $004D
                               C15E: F0 15      beq  $C175
                               C160: AD 94 00   lda  $0094
                               C163: 49 FF      eor  #$FF
                               C165: 29 80      and  #$80
                               C167: 8D 94 00   sta  $0094
                               C16A: AD 39 00   lda  $0039
                               C16D: 29 70      and  #$70
                               C16F: 0D 94 00   ora  $0094
                               C172: 8D 00 30   sta  $3000
                               C175: 88         dey
                               C176: D0 05      bne  $C17D
                               C178: CE 4E 00   dec  $004E
                               C17B: F0 09      beq  $C186
                               C17D: CA         dex
                               C17E: D0 F5      bne  $C175
                               C180: AE 4D 00   ldx  $004D
                               C183: 4C 5B C1   jmp  $C15B
                               C186: AD 39 00   lda  $0039
                               C189: 29 70      and  #$70
                               C18B: 8D 00 30   sta  $3000
                               C18E: A9 01      lda  #$01
                               C190: 8D 1D 00   sta  $001D
                               C193: 68         pla
                               C194: A8         tay
                               C195: 68         pla
                               C196: AA         tax
                               C197: 60         rts

    $3001 - $BFFF    RAM

    $C000 - $FFFF    ROM

    -------------------------------------------------


    Some clues:

    In part of the code you can find...

    C198: A2 FF      ldx  #$FF     ; *** From Start ****
    C19A: 9A         txs
    C19B: D8         cld
    C19C: 20 70 CE   jsr  $CE70
    C19F: 20 0C DA   jsr  $DA0C
    C1A2: AD 67 00   lda  $0067    ; If $0067 = #$E1, jump to $CE3C.
    C1A5: D0 03      bne  $C1AA
    C1A7: 4C CD C1   jmp  $C1CD
    C1AA: C9 02      cmp  #$02
    C1AC: D0 03      bne  $C1B1
    C1AE: 4C 6A C3   jmp  $C36A
    C1B1: C9 03      cmp  #$03
    C1B3: D0 03      bne  $C1B8
    C1B5: 4C F6 C4   jmp  $C4F6
    C1B8: C9 04      cmp  #$04
    C1BA: D0 03      bne  $C1BF
    C1BC: 4C 3D C6   jmp  $C63D
    C1BF: C9 05      cmp  #$05
    C1C1: D0 03      bne  $C1C6
    C1C3: 4C 5D C7   jmp  $C75D
    C1C6: C9 E1      cmp  #$E1
    C1C8: D0 03      bne  $C1CD
    C1CA: 4C 3C CE   jmp  $CE3C
    ...
    ...
    CE3C: A9 00      lda  #$00     ; Clean value at $0067.
    CE3E: 8D 1D 00   sta  $001D
    CE41: 8D 67 00   sta  $0067
    CE44: A9 1F      lda  #$1F     ; Clean the screen.
    CE46: 8D 5F 00   sta  $005F
    CE49: A9 20      lda  #$20
    CE4B: 8D 5E 00   sta  $005E
    CE4E: 20 83 CF   jsr  $CF83
    CE51: A9 01      lda  #$01
    CE53: 8D 10 00   sta  $0010
    CE56: A9 0A      lda  #$0A
    CE58: 8D 11 00   sta  $0011
    CE5B: A9 1B      lda  #$1B
    CE5D: 8D 12 00   sta  $0012
    CE60: A9 D1      lda  #$D1     ; Point to $D194 in $0013/$0014,
    CE62: 8D 14 00   sta  $0014    ; text: "I/O ERROR (TURN OFF TO RESET)".
    CE65: A9 94      lda  #$94
    CE67: 8D 13 00   sta  $0013
    CE6A: 20 03 C0   jsr  $C003    ; Subroutine to write text in video ram.
    CE6D: 4C 6D CE   jmp  $CE6D    ; Stuck here!!!

    Other checks:

    CA71: A9 00      lda  #$00     ; Write #$00 at $0019.
    CA73: 8D 19 00   sta  $0019
    CA76: AD 19 00   lda  $0019    ; Waiting for #$0a at $0019... (incremented through the NMI sub at $CCF6)
    CA79: C9 0A      cmp  #$0A
    CA7B: D0 F9      bne  $CA76
    CA7D: 60         rts
    CA7E: A9 00      lda  #$00     ; Here the same, but waiting for #$04.
    CA80: 8D 19 00   sta  $0019
    CA83: AD 19 00   lda  $0019
    CA86: C9 04      cmp  #$04
    CA88: D0 F9      bne  $CA83
    CA8A: 60         rts
    CA8B: A9 00      lda  #$00     ; Here the same, but waiting for #$32.
    CA8D: 8D 19 00   sta  $0019    ; With the hack in driver init, the game stuck here
    CA90: AD 19 00   lda  $0019    ; waiting for #$32 to be written at $0019.
    CA93: C9 32      cmp  #$32
    CA95: D0 F9      bne  $CA90
    CA97: 60         rts

    DA0C: A9 15      lda  #$15     ; Fill the video RAM with spaces (#$20),
    DA0E: 8D 5F 00   sta  $005F    ; and color RAM with #$15
    DA11: A9 20      lda  #$20
    DA13: 8D 5E 00   sta  $005E
    DA16: 20 83 CF   jsr  $CF83
    DA19: AD 00 18   lda  $1800    ; Check the 1st position at color RAM
    DA1C: 29 80      and  #$80     ; boolean AND with #$80
    DA1E: D0 10      bne  $DA30    ; if not equal, jump to $DA30.

    DA20: A9 1F      lda  #$1F     ; Fill the color RAM with #$1F
    DA22: 8D 5F 00   sta  $005F
    DA25: 20 83 CF   jsr  $CF83
    DA28: AD 00 18   lda  $1800    ; Check the 1st position at color RAM
    DA2B: 29 80      and  #$80     ; boolean AND with #$80
    DA2D: F0 01      beq  $DA30    ; if equal, jump to $DA30.
    DA2F: 60         rts

    DA30: A9 00      lda  #$00     ; Fill video & color RAM
    DA32: 8D 1D 00   sta  $001D    ; with calculated values
    DA35: A9 FF      lda  #$FF     ; through $C111 subroutine
    DA37: 20 11 C1   jsr  $C111
    DA3A: 8D 5E 00   sta  $005E
    DA3D: A9 20      lda  #$20
    DA3F: 20 11 C1   jsr  $C111
    DA42: 8D 5F 00   sta  $005F
    DA45: 20 83 CF   jsr  $CF83
    DA48: 20 77 DB   jsr  $DB77
    DA4B: 4C 30 DA   jmp  $DA30    ; Jump to a loop


    TODO:

    - Map inputs & DIP switches.
    - Correct the GFX banks.
    - Hook properly the MC6845.
    - Figure out the sound.
    - Clean and sort out a lot of things.


*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"


/*************************
*     Video Hardware     *
*************************/

static tilemap *bg_tilemap;

WRITE8_HANDLER( magicfly_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( magicfly_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index];
	int color = attr & 0xff;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START(magicfly)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE(magicfly)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( magicfly_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM    // zero page (pointers and registers)
	AM_RANGE(0x0100, 0x01ff) AM_RAM    // stack pointer
	AM_RANGE(0x0200, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)    // wrong
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)    // wrong
	AM_RANGE(0x0802, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(magicfly_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1400, 0x17ff) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(magicfly_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1c00, 0x27ff) AM_RAM
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_0_r)
	AM_RANGE(0x2801, 0x2fff) AM_RAM
	AM_RANGE(0x3000, 0x3000) AM_WRITENOP    // code loops writing #$00/#$80 here. NMI write other values.
	AM_RANGE(0x3001, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( magicfly )

  PORT_START_TAG("IN0")
  PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Q)
  PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_W)
  PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_E)
  PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_R)
  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_T)
  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_Y)
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_U)
  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_I)

//  PORT_START_TAG("IN1")
//  PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_A)
//  PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_S)
//  PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_D)
//  PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
//  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_J)
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_K)
//
//  PORT_START_TAG("DSW0")    // Only 4 DIP switches
//  PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x1800, &charlayout, 0, 16 },
	{ REGION_GFX1, 0x1000, &charlayout, 0, 16 },
	{ REGION_GFX2, 0x1800, &charlayout, 0, 16 },
	{ REGION_GFX3, 0x1800, &charlayout, 0, 16 },
	{ -1 }
};


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( magicfly )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 10000000/12)	// a guess
	MDRV_CPU_PROGRAM_MAP(magicfly_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_VIDEO_START(magicfly)
	MDRV_VIDEO_UPDATE(magicfly)

MACHINE_DRIVER_END


static DRIVER_INIT( magicfly )
{

/***************************************************************
*
* Temporary patch to avoid a loop of checks for debug purposes
*
* Code at 0xc19f:
*
*    C19F: 20 0C DA   jsr $DA0C
*    (checks to $1800 and writing video & color RAM)
*
*   changed to :
*
*    C19F: EA         nop
*    C1A0: EA         nop
*    C1A1: EA         nop
*
***************************************************************/

    UINT8 *ROM = memory_region(REGION_CPU1);

//    ROM[0xc19f] = 0xea;
//    ROM[0xc1a0] = 0xea;
//    ROM[0xc1a1] = 0xea;

    ROM[0xda30] = 0x60;    // just a rts to part of subroutine to allow work some registers.

}

static DRIVER_INIT( 7mezzo )
{

/* Similar to magicfly, but another offset */

    UINT8 *ROM = memory_region(REGION_CPU1);

    ROM[0xd21a] = 0x60;    // just a rts to part of subroutine.

}


/*************************
*        Rom Load        *
*************************/

ROM_START( magicfly )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "magicfly3.3",	 0xc000, 0x4000, CRC(c29798d5) SHA1(bf92ac93d650398569b3ab79d01344e74a6d35be) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "magicfly2.bin",	 0x0000, 0x2000, CRC(3596a45b) SHA1(7ec32ec767d0883d05606beb588d8f27ba8f10a4) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "magicfly0.bin",	 0x0000, 0x2000, CRC(44e3c9d6) SHA1(677d25360d261bf2400f399b8015eeb529ad405e) )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "magicfly1.bin",	 0x0000, 0x2000, CRC(724d330c) SHA1(cce3923ce48634b27f0e7d29979cd36e7394ab37) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "pal16r4a-magicfly.bin",   0x0000, 0x0104, CRC(bd76fb53) SHA1(2d0634e8edb3289a103719466465e9777606086e) )

ROM_END

ROM_START( 7mezzo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ns3.1",	 0xc000, 0x4000, CRC(b1867b76) SHA1(eb76cffb81c865352f4767015edade54801f6155) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",	 0x0000, 0x2000, CRC(7983a41c) SHA1(68805ea960c2738d3cd2c7490ffed84f90da029b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin",	 0x0000, 0x2000, CRC(e04fb210) SHA1(81e764e296fe387daf8ca67064d5eba2a4fc3c26) )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ns1.bin",	 0x0000, 0x2000, CRC(a6ada872) SHA1(7f531a76e73d479161e485bdcf816eb8eb9fdc62) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "pal16r4a-7mezzo.bin",   0x0000, 0x0104, CRC(61ac7372) SHA1(7560506468a7409075094787182ded24e2d0c0a3) )
ROM_END


/*************************
*      Game Drivers      *
*************************/

//    YEAR  NAME      PARENT    MACHINE   INPUT     INIT            COMPANY        FULLNAME
GAME( 198?, magicfly, 0,        magicfly, magicfly, magicfly, ROT0, "P&A Games",  "Magic Fly",    GAME_NO_SOUND | GAME_NOT_WORKING )
GAME( 198?, 7mezzo  , 0,        magicfly, magicfly, 7mezzo  , ROT0, "Unknown"  ,  "7 e Mezzo", 	  GAME_NO_SOUND | GAME_NOT_WORKING )

