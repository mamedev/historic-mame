
/* PGM System (c)1997 IGS

Based on Information from ElSemi

A flexible cartridge based platform some would say was designed to compete with
SNK's NeoGeo and Capcom's CPS Hardware systems, despite its age it only uses a
68000 for the main processor and a Z80 to drive the sound, just like the two
previously mentioned systems in that respect..

Resolution is 448x224, 15 bit colour

Sound system is ICS WaveFront 2115 Wavetable midi synthesizer, used in some
actual sound cards (Turtle Beach)

Later games are encrypted.  Latest games (kov2, ddp2) include an arm7
coprocessor with an internal rom and an encrypted external rom.

Roms Contain the Following Data

Pxxxx - 68k Program
Txxxx - TX & BG Graphics (2 formats within the same rom)
Mxxxx - Music samples (8 bit mono 11025Hz)
Axxxx - Colour Data (for sprites)
Bxxxx - Masks & A Rom Colour Indexes (for sprites)

There is no rom for the Z80, the program is uploaded by the 68k

Known Games on this Platform
----------------------------


010x  - 1997  - Oriental Legend
020x  - 1997  - Dragon World 2
030x  - 1998  - The Killing Blade
040x  - 1998  - Dragon World 3
050x  - 1999? - Oriental Legend Super
060x  - 1999  - Knights of Valor, Knights of Valor Plus, Knights of Valor Superheroes
070x  - 1999  - Photo Y2k
080x  - 1999  - Puzzle Star
090x  - 2001  - Puzzli II
100x  - 2001  - Martial Masters

120x  - 2001  - Knights of Valor 2 Plus (9 Dragons?)
130x  - 2001  - DoDonpachi II

0450x - 2002  - Demon Front

---

unknown codes:
Dragon World 2001
Photo Y2k2
The Gladiator

---


Oriental Legend
Oriental Legend Super
Sengoku Senki / Knights of Valour Series
-
Sangoku Senki (c)1999 IGS
Sangoku Senki Super Heroes (c)1999 IGS
Sangoku Senki 2 Knights of Valour (c)2000 IGS
Sangoku Senki Busyou Souha (c)2001 IGS
-
DoDonPachi II (Bee Storm)
Photo Y2K
Photo Y2K II
Martial Masters
The Killing Blade
Dragon World 2
Dragon World 3
Dragon World 2001
Demon Front (c) 2002
The Gladiator (c) 2002
Puzzli II

There is also a single board version of the PGM system used by

Demon Front
Later Cave shooters (with different bios?)


To Do / Notes:

some sprite problems
optimize?
layer enables?
sprites use dma?
verify some things
protection in many games

some dumps are suspicious (orlegend super clones are missing roms, drgw3k sets might not have the right protection rom..)


General Notes:

the current 'kov' sets were from 'sango' boards but the protection determines the region so
it makes more sense to name them kov since the roms are probably the same on the various
boards.  The current sets were taken from taiwan boards incase somebody finds
it not to be the case however due to the previous note.

dragon world 2 still has strange protection issues, we have to patch the code for now, what
should really happen, it jumps to invalid code, should the protection device cause the 68k
to see valid code there or something?

kov superheroes uses a different protection chip / different protection commands and doesn't
work, some of the gfx also need redumping to check they're the same as kov, its using invalid
codes for the ones we have (could just be protection tho)


Protection Devices / Co-processors

IGS used a variety of additional ASIC chips on the game boards, these act as protection and
also give additional power to the board to make up for the limited power of the 68000
processor.  Some protection devices use external data roms, others have internal code only.
Most of these are not emulated correctly.

ASIC 3:
	used by:
	different per region, supplies region code
	used by:
	Oriental Legend
	function:

ASIC 12 + ASIC 25
	these seem to be used together
	ASIC 25 appears to perform some kind of bitswap operations
	used by:
	Dragon World 2

ASIC 22 + ASIC 25
	these seem to be used together, ASIC25 has an external software decrypted? data rom
	ASIC 22 might be an updated version of ASIC12 ?
	used by:
	Dragon World 3
	The Killing Blade

ASIC 25 + ASIC 28
	Oriental Legend Super

ASIC 28:
	performs a variety of calculations, quite complex, different per region, supplies region code
	used by:
	Knights of Valour 1 / Plus / Superheroes (plus & superheroes seems to use extra functions, emulation issues reported in places in plus)
	Photo Y2k / Real and Fake (maybe..)

ASIC 27:
	arm9? cpu with 64kb internal rom (different per game) + external data rom
	probably used to give extra power to the system, lots of calculations are offloaded to it
	used by:
	DoDonPachi II
	Knights of Valor 2 / 2 Plus
	Martial Masters
	Demon Front
	Puzzli II

there are probably more...

PCB Layout
----------

IGS PCB NO-0133-2 (Main Board)
|-------------------------------------------------------------------------------------|
|   |----------------------------|   |----------|   |----------------------------|    |
|   |----------------------------|   |----------|   |----------------------------|    |
|                                      PGM_T01S.U29  UM61256    SRM2B61256  SRM2B61256|
| |---------|  33.8688MHz   |----------|                        SRM2B61256  SRM2B61256|
| |WAVEFRONT|               |L8A0290   |   UM6164  UM6164                             |
| |ICS2115V |               |IGS023    |                 PGM_P01S.U20              SW2|
| |(PLCC84) |               |(QFP256)  |                                              |
| |         |               |          |                                              |
| |---------|        50MHz  |----------|                                              |
|    UPD6379  PGM_M01S.U18                             |----------|                   |
|VOL                                                   |MC68HC000 |          74HC132  |
|                                                      |FN20      |   20MHz  74HC132  |
|  UPC844C    |------|                                 |(PLCC68)  |                   |
|             |Z80   |                                 |          |          V3021    |
|             |PLCC44|                  PAL            |----------|                   |
|             |------|    |--------|                                      32.768kHz   |-|
|                         |IGS026  |                                                    |
|                         |(QFP144)|           |--------|                              I|
|                         |        |           |IGS026  |                              D|
|                         |--------|           |(QFP144)|                              C|
|TDA1519A    UM61256 UM61256                   |        |                              3|
|                              TD62064         |--------|                              4|
|                                                                          3.6V_BATT    |
|                                                                                     |-|
|              |----|                                           |-----|     SW3       |
|              |    |               J  A  M  M  A               |     | SW1           |
|--------------|    |-------------------------------------------|     |---------------|


IGS PCB NO-0136 (Riser)
|-------------------------------------------------------------------------------------|
|      |---------------------------------|  |---------------------------------|       |
|      |---------------------------------|  |---------------------------------|       |
|                                                                                     |
|      |---------------------------------|  |---------------------------------|       |
|      |---------------------------------|  |---------------------------------|       |
|                                                                                     |
|   |----------------------------|   |----------|   |----------------------------|    |
|---|                            |---|          |---|                            |----|
    |----------------------------|   |----------|   |----------------------------|

Notes:
      All IC's are shown.

      CPU's
      -----
         68HC000FN20 - Motorola 68000 processor, clocked at 20.000MHz (PLCC68)
         Z80         - Zilog Z0840008VSC Z80 processor, clocked at 8.468MHz (PLCC44)

      SOUND
      -----
         ICS2115     - ICS WaveFront ICS2115V Wavetable Midi Synthesizer, clocked at 33.8688MHz (PLCC84)

      RAM
      ---
         SRM2B256 - Epson SRM2B256SLMX55 8K x8 SRAM (x4, SOP28)
         UM6164   - Unicorn Microelectronics UM6164DS-12 8K x8 SRAM (x2, SOJ28)
         UM61256  - Unicorn Microelectronics UM61256FS-15 32K x8 SRAM (x3, SOJ28)

      ROMs
      ----
         PGM_M01S.U18 - 16MBit MASKROM (TSOP48)
         PGM_P01S.U20 - 1MBit  MASKROM (DIP40, socketed, equivalent to 27C1024 EPROM)
         PGM_T01S.U29 - 16MBit MASKROM (SOP44)

      CUSTOM IC's
      -----------
         IGS023 (QFP256)
         IGS026 (x2, QFP144)

      OTHER
      -----
         3.6V_BATT - 3.6V NICad battery, connected to the V3021 RTC
         IDC34     - IDC34 way flat cable plug, doesn't appear to be used for any games. Might be for
                     re-programming some of the custom IC's or on-board surface mounted ROMs?
         PAL       - Atmel ATF16V8B PAL (DIP20)
         SW1       - Push button switch to enter Test Mode
         SW2       - 8 position DIP Switch (for configuration of PCB/game options)
         SW3       - SPDT switch (purpose unknown)
         TD62064   - Toshiba NPN 50V 1.5A Quad Darlinton Switch; for driving coin meters (DIP16)
         TDA1519A  - Philips 2x 6W Stereo Power AMP (SIL9)
         uPD6379   - NEC 2-channel 16-bit D/A converter 10mW typ. (SOIC8)
         uPC844C   - NEC Quad High Speed Wide Band Operational Amplifier (DIP14)
         V3021     - EM Microelectronic-Marin SA Ultra Low Power 32kHz CMOS Real Time Clock (DIP8)
         VOL       - Volume potentiometer

*/

#include "driver.h"
#include <time.h>

data16_t *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs, *pgm_rowscrollram;
static data8_t *z80_mainram;
WRITE16_HANDLER( pgm_tx_videoram_w );
WRITE16_HANDLER( pgm_bg_videoram_w );
VIDEO_START( pgm );
VIDEO_EOF( pgm );
VIDEO_UPDATE( pgm );
void pgm_kov_decrypt(void);
void pgm_kovsh_decrypt(void);
void pgm_dw2_decrypt(void);
void pgm_djlzz_decrypt(void);
void pgm_dw3_decrypt(void);
void pgm_killbld_decrypt(void);
void pgm_pstar_decrypt(void);

READ16_HANDLER( pgm_asic3_r );
WRITE16_HANDLER( pgm_asic3_w );
WRITE16_HANDLER( pgm_asic3_reg_w );

READ16_HANDLER (sango_protram_r);
READ16_HANDLER (ASIC28_r16);
WRITE16_HANDLER (ASIC28_w16);

READ16_HANDLER (dw2_d80000_r );


static READ16_HANDLER ( z80_ram_r )
{
	return (z80_mainram[offset*2] << 8)|z80_mainram[offset*2+1];
}

static WRITE16_HANDLER ( z80_ram_w )
{
	int pc = activecpu_get_pc();
	if(ACCESSING_MSB)
		z80_mainram[offset*2] = data >> 8;
	if(ACCESSING_LSB)
		z80_mainram[offset*2+1] = data;

	if(pc != 0xf12 && pc != 0xde2 && pc != 0x100c50 && pc != 0x100b20)
		logerror("Z80: write %04x, %04x @ %04x (%06x)\n", offset*2, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER ( z80_reset_w )
{
	logerror("Z80: reset %04x @ %04x (%06x)\n", data, mem_mask, activecpu_get_pc());

	if(data == 0x5050) {
		ics2115_reset();
		cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
		cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);
		if(0) {
			FILE *out;
			out = fopen("z80ram.bin", "wb");
			fwrite(z80_mainram, 1, 65536, out);
			fclose(out);
		}
	}
}

static WRITE16_HANDLER ( z80_ctrl_w )
{
	logerror("Z80: ctrl %04x @ %04x (%06x)\n", data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER ( m68k_l1_w )
{
	if(ACCESSING_LSB) {
		logerror("SL 1 m68.w %02x (%06x) IRQ\n", data & 0xff, activecpu_get_pc());
		soundlatch_w(0, data);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE );
	}
}

static WRITE8_HANDLER( z80_l3_w )
{
	logerror("SL 3 z80.w %02x (%04x)\n", data, activecpu_get_pc());
	soundlatch3_w(0, data);
}

static void sound_irq(int level)
{
	cpunum_set_input_line(1, 0, level);
}

struct ics2115_interface pgm_ics2115_interface = {
	{ MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) },
	REGION_SOUND1,
	sound_irq
};

/* Calendar Emulation */

static unsigned char CalVal, CalMask, CalCom=0, CalCnt=0;

static unsigned char bcd(unsigned char data)
{
	return ((data / 10) << 4) | (data % 10);
}

READ16_HANDLER( pgm_calendar_r )
{
	unsigned char calr;
	calr = (CalVal & CalMask) ? 1 : 0;
	CalMask <<= 1;
	return calr;
}

WRITE16_HANDLER( pgm_calendar_w )
{
	static time_t ltime;
	static struct tm *today;

	// initialize the time, otherwise it crashes
	if( !ltime )
	{
		time(&ltime);
		today = localtime(&ltime);
	}

	CalCom <<= 1;
	CalCom |= data & 1;
	++CalCnt;
	if(CalCnt==4)
	{
		CalMask = 1;
		CalVal = 1;
		CalCnt = 0;
		switch(CalCom & 0xf)
		{
			case 1: case 3: case 5: case 7: case 9: case 0xb: case 0xd:
				CalVal++;
				break;

			case 0:
				CalVal=bcd(today->tm_wday); //??
				break;

			case 2:  //Hours
				CalVal=bcd(today->tm_hour);
				break;

			case 4:  //Seconds
				CalVal=bcd(today->tm_sec);
				break;

			case 6:  //Month
				CalVal=bcd(today->tm_mon + 1); //?? not bcd in MVS
				break;

			case 8:
				CalVal=0; //Controls blinking speed, maybe milliseconds
				break;

			case 0xa: //Day
				CalVal=bcd(today->tm_mday);
				break;

			case 0xc: //Minute
				CalVal=bcd(today->tm_min);
				break;

			case 0xe:  //Year
				CalVal=bcd(today->tm_year % 100);
				break;

			case 0xf:  //Load Date
				time(&ltime);
				today = localtime(&ltime);
				break;
		}
	}
}

/*** Memory Maps *************************************************************/

static ADDRESS_MAP_START( pgm_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x5fffff) AM_ROMBANK(1) /* Game ROM */

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&z80_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x8000, 0x8003) AM_READWRITE(ics2115_r, ics2115_w)
	AM_RANGE(0x8100, 0x81ff) AM_READWRITE(soundlatch3_r, z80_l3_w)
	AM_RANGE(0x8200, 0x82ff) AM_READWRITE(soundlatch_r, soundlatch_w)
	AM_RANGE(0x8400, 0x84ff) AM_READWRITE(soundlatch2_r, soundlatch2_w)
ADDRESS_MAP_END


/*** Input Ports *************************************************************/

/* enough for 4 players, the basic dips mapped are listed in the test mode */

INPUT_PORTS_START( pgm )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Region */
	PORT_DIPNAME( 0x0003, 0x0000, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( World ) )
//	PORT_DIPSETTING(      0x0001, DEF_STR( World ) ) // again?
	PORT_DIPSETTING(      0x0002, "Korea" )
	PORT_DIPSETTING(      0x0003, "China" )
INPUT_PORTS_END

INPUT_PORTS_START( sango )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Region */
	PORT_DIPNAME( 0x000f, 0x0005, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan (Alta License)" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, DEF_STR( World ) )
INPUT_PORTS_END

/*** GFX Decodes *************************************************************/

/* we can't decode the sprite data like this because it isn't tile based.  the
   data decoded by pgm32_charlayout was rearranged at start-up */

static struct GfxLayout pgm8_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20,16,  28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout pgm32_charlayout =
{
	32,32,
	RGN_FRAC(1,1),
	5,
	{ 3,4,5,6,7 },
	{ 0  , 8 ,16 ,24 ,32 ,40 ,48 ,56 ,
	  64 ,72 ,80 ,88 ,96 ,104,112,120,
	  128,136,144,152,160,168,176,184,
	  192,200,208,216,224,232,240,248 },
	{ 0*256, 1*256, 2*256, 3*256, 4*256, 5*256, 6*256, 7*256,
	  8*256, 9*256,10*256,11*256,12*256,13*256,14*256,15*256,
	 16*256,17*256,18*256,19*256,20*256,21*256,22*256,23*256,
	 24*256,25*256,26*256,27*256,28*256,29*256,30*256,31*256 },
	 32*256
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pgm8_charlayout,    0x800, 32  }, /* 8x8x4 Tiles */
	{ REGION_GFX2, 0, &pgm32_charlayout,   0x400, 32  }, /* 32x32x5 Tiles */
	{ -1 } /* end of array */
};

/*** Machine Driver **********************************************************/

static INTERRUPT_GEN( pgm_interrupt ) {
	if( cpu_getiloops() == 0 )
		cpunum_set_input_line(0, 6, HOLD_LINE);
	else
		cpunum_set_input_line(0, 4, HOLD_LINE);
}

static MACHINE_DRIVER_START( pgm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000) /* 20 mhz! verified on real board */
	MDRV_CPU_PROGRAM_MAP(pgm_mem,0)
	MDRV_CPU_VBLANK_INT(pgm_interrupt,2)

	MDRV_CPU_ADD(Z80, 8468000)
	MDRV_CPU_PROGRAM_MAP(z80_mem, 0)
	MDRV_CPU_IO_MAP(z80_io, 0)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU | CPU_16BIT_PORT)

	MDRV_SOUND_ADD(ICS2115, pgm_ics2115_interface)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 56*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1200/2)

	MDRV_VIDEO_START(pgm)
	MDRV_VIDEO_EOF(pgm)
	MDRV_VIDEO_UPDATE(pgm)
MACHINE_DRIVER_END

/*** Init Stuff **************************************************************/

/* This function expands the 32x32 5-bit data into a format which is easier to
   decode in MAME */

static void expand_32x32x5bpp(void)
{
	data8_t *src    = memory_region       ( REGION_GFX1 );
	data8_t *dst    = memory_region       ( REGION_GFX2 );
	size_t  srcsize = memory_region_length( REGION_GFX1 );
	int cnt, pix;

	for (cnt = 0; cnt < srcsize/5 ; cnt ++)
	{
		pix =  ((src[0+5*cnt] >> 0)& 0x1f );							  dst[0+8*cnt]=pix;
		pix =  ((src[0+5*cnt] >> 5)& 0x07) | ((src[1+5*cnt] << 3) & 0x18);dst[1+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 2)& 0x1f );		 					  dst[2+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 7)& 0x01) | ((src[2+5*cnt] << 1) & 0x1e);dst[3+8*cnt]=pix;
		pix =  ((src[2+5*cnt] >> 4)& 0x0f) | ((src[3+5*cnt] << 4) & 0x10);dst[4+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 1)& 0x1f );							  dst[5+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 6)& 0x03) | ((src[4+5*cnt] << 2) & 0x1c);dst[6+8*cnt]=pix;
		pix =  ((src[4+5*cnt] >> 3)& 0x1f );							  dst[7+8*cnt]=pix;
	}
}

/* This function expands the sprite colour data (in the A Roms) from 3 pixels
   in each word to a byte per pixel making it easier to use */

data8_t *pgm_sprite_a_region;
size_t	pgm_sprite_a_region_allocate;

static void expand_colourdata(void)
{
	data8_t *src    = memory_region       ( REGION_GFX3 );
	size_t  srcsize = memory_region_length( REGION_GFX3 );
	int cnt;
	size_t	needed = srcsize / 2 * 3;

	/* work out how much ram we need to allocate to expand the sprites into
	   and be able to mask the offset */
	pgm_sprite_a_region_allocate = 1;
	while (pgm_sprite_a_region_allocate < needed)
		pgm_sprite_a_region_allocate = pgm_sprite_a_region_allocate <<1;

	pgm_sprite_a_region = auto_malloc (pgm_sprite_a_region_allocate);


	for (cnt = 0 ; cnt < srcsize/2 ; cnt++)
	{
		data16_t colpack;

		colpack = ((src[cnt*2]) | (src[cnt*2+1] << 8));
		pgm_sprite_a_region[cnt*3+0] = (colpack >> 0 ) & 0x1f;
		pgm_sprite_a_region[cnt*3+1] = (colpack >> 5 ) & 0x1f;
		pgm_sprite_a_region[cnt*3+2] = (colpack >> 10) & 0x1f;
	}
}

static void pgm_basic_init(void)
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();
}

/* Oriental Legend INIT */

READ16_HANDLER ( orlegend_speedup )
{
	if (activecpu_get_pc()==0x104DD2) cpu_spinuntil_int();
	return pgm_mainram[0x00a70e/2];
}

static DRIVER_INIT( orlegend )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0400e, 0xC0400f, 0, 0, pgm_asic3_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC04000, 0xC04001, 0, 0, pgm_asic3_reg_w);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0400e, 0xC0400f, 0, 0, pgm_asic3_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x80a70e, 0x80a70f, 0, 0, orlegend_speedup);
}

static void drgwld2_common_init(void)
{
	pgm_basic_init();
	pgm_dw2_decrypt();
	/*
	Info from Elsemi
	Here is how to "bypass" the dw2 hang protection, it fixes the mode
	select and after failing in the 2nd stage (probably there are other checks
	out there).
	*/
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd80000, 0xd80003, 0, 0, dw2_d80000_r);
}

static DRIVER_INIT( drgw2 )
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	drgwld2_common_init();
	/* patches .. i hate patches .. */
	mem16[0x131098/2]=0x4e93;
	mem16[0x13113e/2]=0x4e93;
	mem16[0x1311ce/2]=0x4e93;
}

static DRIVER_INIT( drgw2c )
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	drgwld2_common_init();
	/* this seems to be some strange protection
	ElSemi
	patch the jmp (a0) to jmp(a3) (otherwise they jump to illegal code)
	there are 3 (consecutive functions)
	303bc, 30462, 304f2
	*/
	mem16[0x1303bc/2]=0x4e93;
	mem16[0x130462/2]=0x4e93;
	mem16[0x1304F2/2]=0x4e93;
}

static DRIVER_INIT( kov )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_kov_decrypt();
}

static DRIVER_INIT( pstar )
{
	pgm_basic_init();
 	pgm_pstar_decrypt();
}

static DRIVER_INIT( kovsh )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_kovsh_decrypt();
}

static DRIVER_INIT( djlzz )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_djlzz_decrypt();
}

static DRIVER_INIT( dw3 )
{
	pgm_basic_init();

//	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xda0000, 0xdaffff, 0, 0, dw3_prot_r);
//	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xda0000, 0xdaffff, 0, 0, dw3_prot_w);

 	pgm_dw3_decrypt();
}

static DRIVER_INIT( killbld )
{
	pgm_basic_init();
 	pgm_killbld_decrypt();
}



/*** Rom Loading *************************************************************/

/* take note of REGION_GFX2 needed for expanding the 32x32x5bpp data and
   REGION_GFX4 needed for expanding the Sprite Colour Data */

/* The Bios - NOT A GAME */
ROM_START( pgm )
	ROM_REGION( 0x520000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x00000, 0x20000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* 8x8 Text Layer Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) )
ROM_END

ROM_START( orlegend )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0103.rom",    0x100000, 0x200000, CRC(d5e93543) SHA1(f081edc26514ca8354c13c7f6f89aba8e4d3e7d2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegnde )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0102.rom",    0x100000, 0x200000, CRC(4d0f6cc5) SHA1(8d41f0a712fb11a1da865f5159e5e27447b4388a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegndc )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0101.160",    0x100000, 0x200000, CRC(b24f0c1e) SHA1(a2cf75d739681f091c24ef78ed6fc13aa8cfe0c6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Oriental Legend / Xi Yo Gi Shi Re Zuang (CHINA 111 Ver.)
(c)1997 IGS

PGM system
IGS PCB NO-0134-1
IGS PCB NO-0135


OLV111CH.U11 [b80ddd3c]
OLV111CH.U6  [5fb86373]
OLV111CH.U7  [6ee79faf]
OLV111CH.U9  [83cf09c8]

T0100.U8


A0100.U5
A0101.U6
A0102.U7
A0103.U8
A0104.U11
A0105.U12

B0100.U9
B0101.U10
B0102.U15

M0100.U1

*/

ROM_START( orld111c )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "olv111ch.u6",     0x100001, 0x080000, CRC(5fb86373) SHA1(2fc58eff1f38754c75819fde666244b867ca4f05) )
	ROM_LOAD16_BYTE( "olv111ch.u9",     0x100000, 0x080000, CRC(83cf09c8) SHA1(959780b45326059517f3008a356657f4f3d2908f) )
	ROM_LOAD16_BYTE( "olv111ch.u7",     0x200001, 0x080000, CRC(6ee79faf) SHA1(039b4b07b8577f0d3022ae01210c00375624cb3c) )
	ROM_LOAD16_BYTE( "olv111ch.u11",    0x200000, 0x080000, CRC(b80ddd3c) SHA1(55c700ce71ffdee392e03fd9d4719542c3527132) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Oriental Legend / Xi Yo Gi Shi Re Zuang (KOREA 105 Ver.)
(c)1997 IGS

PGM system
IGS PCB NO-0134-2
IGS PCB NO-0135


OLV105KO.U11 [40ae4d9e]
OLV105KO.U6  [b86703fe]
OLV105KO.U7  [5712facc]
OLV105KO.U9  [5a108e39]

T0100.U8


A0100.U5
A0101.U6
A0102.U7
A0103.U8
A0104.U11
A0105.U12

B0100.U9
B0101.U10
B0102.U15

M0100.U1

*/

ROM_START( orld105k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "olv105ko.u6",     0x100001, 0x080000, CRC(b86703fe) SHA1(a3529b45efd400ecd5e76f764b528ebce46e24ab) )
	ROM_LOAD16_BYTE( "olv105ko.u9",     0x100000, 0x080000, CRC(5a108e39) SHA1(2033f4fe3f2dfd725dac535324f58348b9ac3914) )
	ROM_LOAD16_BYTE( "olv105ko.u7",     0x200001, 0x080000, CRC(5712facc) SHA1(2d95ebd1703874e89ac3a206f8c1f0ece6e833e0) )
	ROM_LOAD16_BYTE( "olv105ko.u11",    0x200000, 0x080000, CRC(40ae4d9e) SHA1(62d7a96438b7fe93f74753333f50e077d417971e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Dragon World 2 (English / World Version)
IGS, 1997

This is a cart for the IGS PGM system.


PCB Layout
----------

IGS PCB NO-0162
|-----------------------------------------------|
| |------|                                      |
| |IGS012|       *1                    T0200.U7 |
| |      |                                      |
| |------|                                      |
|              |--------|                       |
|              |        |                       |
|              | IGS025 |  *2   V-110X.U2       |
| PAL    PAL   |        |                  PAL  |
|              |--------|                       |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS012       - Custom IGS IC (QFP80)

      -- on english version
      IGS025       - Custom IGS IC (PLCC68, labelled "DRAGON II 0006")
      -- on china version
      IGS025       - Custom IGS IC (PLCC68, labelled "DRAGON II 0005")


      T0200.U7     - 32MBit MaskROM (SOP44)

  	  -- on english version
  	  V-110X.U2    - AM27C4096 4MBit EPROM (DIP42, labelled "DRAGON II V-110X")
      -- on china version
      V-110X.U2    - AM27C4096 4MBit EPROM (DIP42, labelled "DRAGON II V-100C")

      PALs         - x3, labelled "CZ U3", "CZ U4", "CZ U6"
      *1           - Unpopulated position for MX23C4100 SOP40 MASKROM
      *2           - Unpopulated position for MX23C4100 DIP40 EPROM/MASKROM


IGS PCB NO-0135
|-----------------------------------------------|
| U11    U12     U13      U14       U15      U16|
|                                               |
|                                               |
|A0200.U5                       B0200.U9        |
|        U6      U7       U8                 U10|
|                                               |
|                                               |
|74LS138         U1       U2             74LS139|
|                                               |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      This PCB contains only SOP44 MASKROMS and 2 logic IC's
      Only U5 and U9 are populated

      glitch on select screen exists on real board.

*/

ROM_START( drgw2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-110x.u2",    0x100000, 0x080000, CRC(1978106b) SHA1(af8a13d7783b755a58762c98bdc32cab845b2251) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END


ROM_START( drgw2c )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-100c.u2",    0x100000, 0x080000, CRC(67467981) SHA1(58af01a3871b6179fe42ff471cc39a2161940043) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END

/*

Dragon World 3
Alta Co./IGS, 1998

Cart for IGS PGM system

Top board of cart contains.....
8MHz Xtal
32.768kHz Xtal
UM6164 (RAM x 2)
MACH211 CPLD
IGS022 ASIC
IGS025 ASIC
1x PAL
2x 27C040 EPROMs (main 68k program)
1x 27C512 EPROM (protection code?)
1x 32MBit smt MASKROM (T0400)

Bottom board contains.....
4x 32MBit smt MASKROMs (A0400, A0401, B0400, M0400)

*/

ROM_START( drgw3 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "dw3_v100.u12",     0x100001, 0x080000,  CRC(47243906) SHA1(9cd46e3cba97f049bcb238ceb6edf27a760ef831) )
	ROM_LOAD16_BYTE( "dw3_v100.u13",     0x100000, 0x080000,  CRC(b7cded21) SHA1(c1ae2af2e42227503c81bbcd2bd6862aa416bd78) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data */
	ROM_LOAD( "dw3_v100.u15", 0x000000, 0x010000, CRC(03dc4fdf) SHA1(b329b04325d4f725231b1bb7862eedef2319b652) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "dw3t0400.u18",   0x400000, 0x400000, CRC(b70f3357) SHA1(8733969d7d21f540f295a9f747a4bb8f0d325cf0) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "dw3a0400.u9",     0x0000000, 0x400000, CRC(dd7bfd40) SHA1(fb7ec5bf89a413c5208716083762a725ff63f5db) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "dw3a0401.u10",    0x0400000, 0x400000, CRC(cab6557f) SHA1(1904dd86645eea27ac1ab8a2462b20f6531356f8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "dw3b0400.u13",    0x0000000, 0x400000,  CRC(4bb87cc0) SHA1(71b2dc43fd11f7a6dffaba501e4e344b843583d8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "dw3m0400.u1",  0x400000, 0x400000, CRC(031eb9ce) SHA1(0673ec194732becc6648c2ae1396e894aa269f9a) )
ROM_END

/*

Dragon World 3 (KOREA 106 Ver.)
(c)1998 IGS

PGM system
IGS PCB NO-0189
IGS PCB NO-0178


DW3_V106.U12 [c3f6838b]
DW3_V106.U13 [28284e22]


*/

ROM_START( drgw3k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "dw3_v106.u12",     0x100001, 0x080000,  CRC(c3f6838b) SHA1(c135b1d4dd62af308139d40d03c29be7508fb1e7) )
	ROM_LOAD16_BYTE( "dw3_v106.u13",     0x100000, 0x080000,  CRC(28284e22) SHA1(4643a69881ddb7383ca10f3eb2aa2cf41be39e9f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data - is it correct for this set? */
	ROM_LOAD( "dw3_v100.u15", 0x000000, 0x010000, CRC(03dc4fdf) SHA1(b329b04325d4f725231b1bb7862eedef2319b652) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "dw3t0400.u18",   0x400000, 0x400000, CRC(b70f3357) SHA1(8733969d7d21f540f295a9f747a4bb8f0d325cf0) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "dw3a0400.u9",     0x0000000, 0x400000, CRC(dd7bfd40) SHA1(fb7ec5bf89a413c5208716083762a725ff63f5db) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "dw3a0401.u10",    0x0400000, 0x400000, CRC(cab6557f) SHA1(1904dd86645eea27ac1ab8a2462b20f6531356f8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "dw3b0400.u13",    0x0000000, 0x400000,  CRC(4bb87cc0) SHA1(71b2dc43fd11f7a6dffaba501e4e344b843583d8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "dw3m0400.u1",  0x400000, 0x400000, CRC(031eb9ce) SHA1(0673ec194732becc6648c2ae1396e894aa269f9a) )
ROM_END

ROM_START( kov )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.117",    0x100000, 0x400000, CRC(c4d19fe6) SHA1(14ef31539bfbc665e76c9703ee01b12228344052) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kov115 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.115",    0x100000, 0x400000, CRC(527a2924) SHA1(7e3b166dddc5245d7b408e78437c16fd2986d1d9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/*

Sangoku Senki / Knights of Valour (JPN 100 Ver.)
(c)1999 ALTA / IGS

PGM system
IGS PCB NO-0212-1
IGS PCB NO-0213T


SAV111.U10   [d5536107]
SAV111.U4    [ae2f1b4e]
SAV111.U5    [5fdd4aa8]
SAV111.U7    [95eedf0e]
SAV111.U8    [003cbf49]

T0600.U11


A0600.U2
A0601.U4
A0602.U6
A0603.U9

M0600.U3

B0600.U5
B0601.U7

*/

ROM_START( kovj )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "sav111.u4",      0x100001, 0x080000, CRC(ae2f1b4e) SHA1(2ac9d84f5dee52f374941cfd68e2b98ecad436a8) )
	ROM_LOAD16_BYTE( "sav111.u7",      0x100000, 0x080000, CRC(95eedf0e) SHA1(582a54e9a1eda7ff73e20f0e69d2d50141772378) )
	ROM_LOAD16_BYTE( "sav111.u5",      0x200001, 0x080000, CRC(5fdd4aa8) SHA1(43c96e21ad4f11148e1e94a59c53780b2edd43ba) )
	ROM_LOAD16_BYTE( "sav111.u8",      0x200000, 0x080000, CRC(003cbf49) SHA1(fb5bea47ecae025b1b425af52cd05e061f45e377) )
	ROM_LOAD16_WORD_SWAP( "sav111.u10",0x300000, 0x080000, CRC(d5536107) SHA1(f963e015d99c1621323eecf63e773c0b9f4b6a43) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kovplus )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.119",    0x100000, 0x400000, CRC(e4b0875d) SHA1(e8382e131b0e431406dc2a05cc1ef128302d987c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/*

Sangoku Senki Plus / Knights of Valour Plus (Alt 119 Ver.)
(c)1999 IGS

PGM system
IGS PCB NO-0222
IGS PCB NO-0213


V119.U2      [29588ef2]
V119.U3      [6750388f]
V119.U4      [8200ece6]
V119.U5      [d4101ffd]
V119.U6      [71e28f27]

T0600.U11


A0600.U2
A0601.U4
A0602.U6
A0603.U9

M0600.U3

B0600.U5
B0601.U7

*/

ROM_START( kovplusa )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_BYTE( "v119.u3",     0x100001, 0x080000, CRC(6750388f) SHA1(869f4ad27f2992cc62baa9a78bf7984a43ec4cc5) )
	ROM_LOAD16_BYTE( "v119.u5",     0x100000, 0x080000, CRC(d4101ffd) SHA1(a327fd56eec65b07df9305cd93ef2c46bf8e40f3) )
	ROM_LOAD16_BYTE( "v119.u4",     0x200001, 0x080000, CRC(8200ece6) SHA1(97081d2e8aed2ac6fbe5951890aecea18af5ce2e) )
	ROM_LOAD16_BYTE( "v119.u6",     0x200000, 0x080000, CRC(71e28f27) SHA1(db382807e9185f0dc17124f210165fa1b36ca6ac) )
	ROM_LOAD16_WORD_SWAP( "v119.u2",0x300000, 0x080000, CRC(29588ef2) SHA1(17d1a308d44434cf65224a24360cf4b6e32d28f3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kovsh )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.322",    0x100000, 0x400000, CRC(7c78e5f3) SHA1(9b1e4bd63fb1294ebeb539966842273c8dc7683b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.320",    0x400000, 0x400000, CRC(164b3c94) SHA1(f00ea66886ca6bff74bbeaa49e7f5c75c275d5d7) ) // bad? its half the size of the kov one

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( photoy2k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v104.16m",     0x100000, 0x200000, CRC(e051070f) SHA1(a5a1a8dd7542a30632501af8d02fda07475fd9aa) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x480000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0700.rom",    0x400000, 0x080000, CRC(93943b4d) SHA1(3b439903853727d45d62c781af6073024eb3c5a3) )

	ROM_REGION( 0x480000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1080000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0700.l",    0x0000000, 0x0400000, CRC(26a9ae9c) SHA1(c977c89db6fdf47ee260ff687b80375caeab975c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0700.h",    0x0400000, 0x0400000, CRC(79bc1fc1) SHA1(a09472a9b75704c1d31ab828f92c2a5007b2b4ed) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.l",    0x0800000, 0x0400000, CRC(23607f81) SHA1(8b6dbcdce9b131370693847ed9771aa04b62711c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.h",    0x0c00000, 0x0400000, CRC(5f2efd37) SHA1(9a5bd9751691bc085b0751b9fa8ede9eb97b1248) )
	ROM_LOAD( "a0702.rom",  0x1000000, 0x0080000, CRC(42239e1b) SHA1(2b6d20958abf8a67ce525d5c8964b6d225ccaeda) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0700.l",    0x0000000, 0x0400000, CRC(af096904) SHA1(8e86b36cc44720ece68022e409279bf9144341ba) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "b0700.h",    0x0400000, 0x0400000, CRC(6d53de26) SHA1(f3f93fd2f87adb815834ba0242b94073fbb5e333) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "cgv101.rom", 0x0800000, 0x0020000, CRC(da02ec3e) SHA1(7ee21d748c9b932f53e790a9040167f904fecefc) )

	ROM_REGION( 0x480000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0700.rom",    0x400000, 0x080000, CRC(acc7afce) SHA1(ac2d344ebac336f0f363bb045dd8ea4e83d1fb50) )
ROM_END

/*

Real and Fake / Photo Y2K (JPN 102 Ver.)
(c)1999 ALTA / IGS

PGM system
IGS PCB NO-0220
IGS PCB NO-0221


V102.U4      [a65eda9f]
V102.U5      [9201621b]
V102.U6      [b9ca5504]
V102.U8      [3be22b8f]

T0700.U11


A0700.U2
A0701.U4

SP_V102.U5

B0700.U7

CG_V101.U3
CG_V101.U6

*/

ROM_START( raf102j )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "v102.u4",     0x100001, 0x080000, CRC(a65eda9f) SHA1(6307cacf4a262e781753eff14700a0455837780c) )
	ROM_LOAD16_BYTE( "v102.u6",     0x100000, 0x080000, CRC(b9ca5504) SHA1(058cf01316f233236ca9861349f515935283b75e) )
	ROM_LOAD16_BYTE( "v102.u5",     0x200001, 0x080000, CRC(9201621b) SHA1(1ca3ebe7eec40614bfa8b911657fa2b51f2c51a4) )
	ROM_LOAD16_BYTE( "v102.u8",     0x200000, 0x080000, CRC(3be22b8f) SHA1(03634fbd6a8a8369c6cb1fd6694a3784dac5bf59) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x480000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0700.rom",    0x400000, 0x080000, CRC(93943b4d) SHA1(3b439903853727d45d62c781af6073024eb3c5a3) )

	ROM_REGION( 0x480000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1080000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0700.l",    0x0000000, 0x0400000, CRC(26a9ae9c) SHA1(c977c89db6fdf47ee260ff687b80375caeab975c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0700.h",    0x0400000, 0x0400000, CRC(79bc1fc1) SHA1(a09472a9b75704c1d31ab828f92c2a5007b2b4ed) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.l",    0x0800000, 0x0400000, CRC(23607f81) SHA1(8b6dbcdce9b131370693847ed9771aa04b62711c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.h",    0x0c00000, 0x0400000, CRC(5f2efd37) SHA1(9a5bd9751691bc085b0751b9fa8ede9eb97b1248) )
	ROM_LOAD( "a0702.rom",  0x1000000, 0x0080000, CRC(42239e1b) SHA1(2b6d20958abf8a67ce525d5c8964b6d225ccaeda) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0700.l",    0x0000000, 0x0400000, CRC(af096904) SHA1(8e86b36cc44720ece68022e409279bf9144341ba) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "b0700.h",    0x0400000, 0x0400000, CRC(6d53de26) SHA1(f3f93fd2f87adb815834ba0242b94073fbb5e333) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "cgv101.rom", 0x0800000, 0x0020000, CRC(da02ec3e) SHA1(7ee21d748c9b932f53e790a9040167f904fecefc) )

	ROM_REGION( 0x480000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0700.rom",    0x400000, 0x080000, CRC(acc7afce) SHA1(ac2d344ebac336f0f363bb045dd8ea4e83d1fb50) )
ROM_END

/*

The Killing Blade (English / World Version)
IGS, 1998

This is a cart for the IGS PGM system.


PCB Layout
----------

IGS PCB NO-0179
|-----------------------------------------------|
|                      8MHz  |--------|         |
|            32.768kHz|----| |        |T0300.U14|
|6164  6164           |IGS | | IGS025 |         |
|                     |022 | |        |         |
|*                    |----| |--------|         |
|                                               |
|           U2     U3     U4     U5     U6      |
| PAL   PAL                                PAL  |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS022       - Custom IGS IC (QFP100)
      IGS025       - Custom IGS IC (PLCC68, labelled "ENGLISH")
      T0300.U14    - 32MBit MaskROM (SOP44, labelled "T0300")
      6164         - x2, 8K x8 SRAM (SOJ28)
      U2           - 27C512 512KBit EPROM (DIP28, labelled "KB U2 V104")
      U3           - 27C4000 4MBit EPROM (DIP32, labelled "KB U3 V104")
      U4           - 27C4000 4MBit EPROM (DIP32, labelled "KB U4 V104")
      U5           - 27C4000 4MBit EPROM (DIP32, labelled "KB U5 V104")
      U6           - 27C4000 4MBit EPROM (DIP32, labelled "KB U6 V104")
      PALs         - x3, labelled "DH U8", "DH U1", "DH U7"
      *            - Unpopulated position for DIP42 EPROM/MASKROM (labelled "P0300")


IGS PCB NO-0178
|-----------------------------------------------|
| U9    U10   U11    U12     U13     U14     U15|
|                                               |
|                                               |
|                                               |
| U1    U2                         74LS138      |
|                                  74LS139      |
|             U3     U4      U5              U8 |
|                                               |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      U1           - 32MBit MASKROM (SOP44, labelled "M0300")
      U2           - 32MBit MASKROM (SOP44, labelled "A0307")
      U3           - 16MBit MASKROM (DIP42, labelled "A0302")
      U4           - 16MBit MASKROM (DIP42, labelled "A0304")
      U5           - 16MBit MASKROM (DIP42, labelled "A0305")
      U8           - 16MBit MASKROM (DIP42, labelled "B0301")
      U9           - 32MBit MASKROM (SOP44, labelled "A0300")
      U10          - 32MBit MASKROM (SOP44, labelled "A0301")
      U11          - 32MBit MASKROM (SOP44, labelled "A0303")
      U12          - 32MBit MASKROM (SOP44, labelled "A0306")
      U13          - 32MBit MASKROM (SOP44, labelled "B0300")
      U14          - 32MBit MASKROM (SOP44, labelled "B0302")
      U15          - 32MBit MASKROM (SOP44, labelled "B0303")

*/

ROM_START( killbld )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "kb_u3_v104.u3",     0x100001, 0x080000, CRC(6db1d719) SHA1(804002f014d275aaf0368fb7f904938fe4ac07ee) )
	ROM_LOAD16_BYTE( "kb_u6_v104.u6",     0x100000, 0x080000, CRC(31ecc978) SHA1(82666d534e4151775063af6d39f575faba0f1047) )
	ROM_LOAD16_BYTE( "kb_u4_v104.u4",     0x200001, 0x080000, CRC(1ed8b2e7) SHA1(331c037640cfc1fe743cd0e65a1156c470b3303e) ) // order?
	ROM_LOAD16_BYTE( "kb_u5_v104.u5",     0x200000, 0x080000, CRC(a0bafc29) SHA1(b20db7c16353c6f87ed3c08c9d037b07336711f1) ) // order?

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */


	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data */
	ROM_LOAD( "kb_u2_v104.u2", 0x000000, 0x010000,  CRC(c970f6d5) SHA1(399fc6f80262784c566363c847dc3fdc4fb37494) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0300.u14",    0x400000, 0x400000, CRC(0922f7d9) SHA1(4302b4b7369e13f315fad14f7d6cad1321101d24) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x2000000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0300.u9",   0x0000000, 0x0400000,  CRC(3f9455d3) SHA1(3718ce00ad93975383aafc14e5a74dc297b011a1) )
	ROM_LOAD( "a0301.u10",  0x0400000, 0x0400000,  CRC(92776889) SHA1(6d677837fefff47bfd1c6166322f69f89989a5e2) )
	ROM_LOAD( "a0302.u3",   0x0800000, 0x0200000,  CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) )
	ROM_LOAD( "a0303.u11",  0x0c00000, 0x0400000,  CRC(33f5cc69) SHA1(9cacd5058d4bb25b77f71658bbbbd4b38d0a6b6a) )
	ROM_LOAD( "a0304.u4",   0x1000000, 0x0200000,  CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) )
	ROM_LOAD( "a0305.u5",   0x1400000, 0x0200000,  CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) )
	ROM_LOAD( "a0306.u12",  0x1800000, 0x0400000,  CRC(cc018a8e) SHA1(37752d46f238fb57c0ab5a4f96b1e013f2077347) )
	ROM_LOAD( "a0307.u2",   0x1c00000, 0x0400000,  CRC(bc772e39) SHA1(079cc42a190cb916f02b59bca8fa90e524acefe9) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0300.u13",    0x0000000, 0x0400000, CRC(7f876981) SHA1(43555a200929ad5ecc42137fc9aeb42dc4f50d20) )
	ROM_LOAD( "b0301.u8",     0x0400000, 0x0200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) )
	ROM_LOAD( "b0302.u14",    0x0800000, 0x0400000, CRC(eea9c502) SHA1(04b3972c7111ea59a3cceab6ad124080c4ce3520) )
	ROM_LOAD( "b0303.u15",    0x0c00000, 0x0200000, CRC(77a9652e) SHA1(2342f643d37945fbda224a5034c013796e5134ca) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0300.u1",     0x400000, 0x400000, CRC(93159695) SHA1(50c5976c9b681bd3d1ebefa3bfa9fe6e72dcb96f) )
ROM_END

ROM_START( puzlstar )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_BYTE( "v100mg.u1",     0x100001, 0x080000, CRC(5788b77d) SHA1(7770aae6e686da92b2623c977d1bc8f019f48267) )
	ROM_LOAD16_BYTE( "v100mg.u2",     0x100000, 0x080000, CRC(4c79d979) SHA1(3b92052a35994f2b3dd164930154184c45d5e2d0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0800.u5",    0x400000, 0x200000, CRC(f9d84e59) SHA1(80ec77025ac5bf355b1a60f2a678dd4c56071f6b) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0800.u1",    0x0000000, 0x0400000, CRC(e1e6ec40) SHA1(390432431f144ef63424a426582b311765a61771) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0800.u3",    0x0000000, 0x0200000, CRC(52e7bef5) SHA1(a678251b7e46a1016d0afc1d8d5c9928008ad5b1) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0800.u2",    0x400000, 0x400000,  CRC(e1a46541) SHA1(6fe9de5700d8638374734d80551dcedb62975140) )
ROM_END


/*

Oriental Legend Super
IGS, 1998

This is a cart for the IGS PGM system.

PCB Layout
----------
IGS PCB NO-0191-1
|-----------------------------------------------|
|6264                 8MHz|--------|            |
|6264                     |        |   T0500.U18|
|                         | IGS025 |            |
|                 |-----| |        |   T0501.U19|
|                 | IGS | |--------|            |
|                 | 028 |                       |
|        *1       |-----|           V101.U1     |
|              V101.U2   V101.U4  PAL      PAL  |
|  V101.U6          V101.U3   V101.U5           |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS028       - Custom IGS IC (QFP100)
      IGS025       - Custom IGS IC (PLCC68, labelled "KOREA")
      T0500.U18    - 32MBit MaskROM (SOP44)
      T0501.U19    - 16MBit MaskROM (SOP44)
      V101.U1      - MX27C4096 4MBit EPROM (DIP40)
      V101.U2/3/4/5- MX27C4000 4MBit EPROM (DIP32)
      PALs         - x2, labelled "CW-2 U8", "CW-2 U7"
      6264         - 8K x8 SRAM
      *1           - Unpopulated position for SOP44 MASKROM labelled "P0500"


IGS PCB NO-0135
|-----------------------------------------------|
|A0504.U11        A0506.U13     B0502.U15       |
|         A0505.U12         U14        B0503.U16|
|                                               |
|A0500.U5                       B0500.U9        |
|         A0501.U6       A0503.U8      B0501.U10|
|                 A0502.U7                      |
|                                               |
|74LS138          M0500.U1               74LS139|
|                           U2                  |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      This PCB contains only SOP44 MaskROMS and 2 logic IC's
      U2 and U14 are not populated.
      All are 32MBit except M0500 which is 16MBit.

*/

ROM_START( olds )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "sp_v101.u2",      0x100001, 0x080000,   CRC(08eb9661) SHA1(105946e72e562adb1a9fd794ca0fd2c91967eb56) )
	ROM_LOAD16_BYTE( "sp_v101.u3",      0x100000, 0x080000,   CRC(0a358c1e) SHA1(95c7c3f069c5d05001e22535750f6b3cd7de105f) )
	ROM_LOAD16_BYTE( "sp_v101.u4",      0x200001, 0x080000,   CRC(766570e0) SHA1(e7c3f5664ec69b662b82c2e1375555db7305390c) )
	ROM_LOAD16_BYTE( "sp_v101.u5",      0x200000, 0x080000,   CRC(58662e12) SHA1(2b39bd847e9c4968a8e77a2f3cec77cf323ceee3) )
	ROM_LOAD16_WORD_SWAP( "sp_v101.u1",0x300000, 0x080000,    CRC(2b2f4f1e) SHA1(67b97cf8cc7f517d67cd45588addd2ad8e24612a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* ASIC25? Protection Data */
	ROM_LOAD( "sp_v101.u6", 0x000000, 0x010000,  CRC(097046bc) SHA1(6d75db85cf4c79b63e837897785c253014b2126d) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )


	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

ROM_START( olds100 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "v100-u2.040",      0x100001, 0x080000,  CRC(517c2a06) SHA1(bbf5b311fac9b0bb4d4129c0561e5e24f6963fa2) )
	ROM_LOAD16_BYTE( "v100-u3.040",      0x100000, 0x080000,  CRC(d0e2b741) SHA1(2e671dbb4320d1f0c059b35efd33cdea26f12131) )
	ROM_LOAD16_BYTE( "v100-u4.040",      0x200001, 0x080000,  CRC(32a6bdbd) SHA1(a93d7f4eae722a58eca9ec351ad5890cefda56f0) )
	ROM_LOAD16_BYTE( "v100-u5.040",      0x200000, 0x080000,  CRC(b4a1cafb) SHA1(b2fccd480ede93f58ad043387b18b898152f06ef) )
	/* u1 is missing from this set? */
//	ROM_LOAD16_WORD_SWAP( "v100-u1.040",0x300000, 0x080000,    CRC(1) SHA1(1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* ASIC25? Protection Data */
	ROM_LOAD( "kd-u6.512", 0x000000, 0x010000,  CRC(e7613dda) SHA1(0d7c043b90e2f9a36a45066f22e3e305dc716676) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

ROM_START( olds100a )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	/* this rom had a lame hack applied to it by the dumper, this was removed, hopefully it is correct now */
	ROM_LOAD16_WORD_SWAP( "p0500.v10",    0x100000, 0x400000, CRC(8981fc87) SHA1(678d6705d06b99bca5951ff77708adadc4c4396b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 - romless */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* ASIC25? Protection Data */
	/* missing from this set .. */
//	ROM_LOAD( "kd-u6.512", 0x000000, 0x010000,  CRC(1) SHA1(1) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

/*** GAME ********************************************************************/

GAMEX( 1997, pgm,      0,          pgm, pgm,   0,          ROT0, "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAMEX( 1997, orlegend, pgm,        pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 126)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1997, orlegnde, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1997, orlegndc, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112, Chinese Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1997, orld111c, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 111, Chinese Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1997, orld105k, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 105, Korean Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1997, drgw2,    pgm,        pgm, pgm,   drgw2,      ROT0, "IGS", "Dragon World II (ver. 110X, Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1997, drgw2c,   drgw2,      pgm, pgm,   drgw2c,     ROT0, "IGS", "Zhong Guo Long II (ver. 100C, China)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, kov,      pgm,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 117)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kov115,   kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 115)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kovj,     kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 100, Japanese Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kovplus,  kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour Plus / Sangoku Senki Plus (ver. 119)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, kovplusa, kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour Plus / Sangoku Senki Plus (alt ver. 119)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* not working */
GAMEX( 1998, drgw3,    pgm,        pgm, sango, dw3,	       ROT0, "IGS", "Dragon World 3", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, drgw3k,   drgw3,      pgm, sango, dw3,	       ROT0, "IGS", "Dragon World 3 (Korean Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, photoy2k, pgm,        pgm, sango, djlzz, 	   ROT0, "IGS", "Photo Y2K", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, raf102j,  photoy2k,   pgm, sango, djlzz, 	   ROT0, "IGS", "Real and Fake / Photo Y2K (ver. 102, Japan Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, kovsh,    kov,        pgm, sango, kovsh,	   ROT0, "IGS", "Knights of Valour Superheroes / Sangoku Senki Superheroes (ver. 322)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, killbld,  pgm,        pgm, sango, killbld,	   ROT0, "IGS", "The Killing Blade", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, puzlstar, pgm,        pgm, sango, pstar,	   ROT0, "IGS", "Puzzle Star", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, olds,     pgm,        pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend Super (Korea 101)", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION|GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1999, olds100,  olds,       pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend Super (100)", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION|GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
GAMEX( 1999, olds100a, olds,       pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend Super (100 alt)", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION|GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND  )
