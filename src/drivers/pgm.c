
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
Photo Y2K? (and its sequel?)
Material Artist
Dragon World 2+3
.. lots more ..


To Do / Notes:

sprite zooming (zoom table is contained in vidram)
sprite masking (lower priority sprites under bg layers can mask higher ones) -? no?
calender
optimize?
layer enables?
sprites use dma?
verify some things
other 2 interrupts - one is sound related *dragon world 2 uses one of them for inputs
the 'encryption' info came from unknown 3rd party
see notes at bottom of driver, protection emulation by ElSemi

Sango / Kov level 2 'bridge' is drawn with bg sprites but needs to appear OVER the bg layer,
ElSemi believes that bg sprites are drawn as fg sprites if the first sprite in the sprite
list doesn't have the bg bit set. -fixed-

the protection determines the region in both the supported games, however the 'china' board
of orlegend is the same revision one of the other sets, but of an earlier build date.

the current 'kov' sets were from 'sango' boards but the protection determines the region so
it makes more sense to name them kov since the roms are probably the same on the various
boards.  The current sets were taken from taiwan boards incase somebody finds
it not to be the case however due to the previous note.

the dragon world 2 set is china only (whats the real chinese name?) there is an english
version, however this set doesn't allow for that without hacks at least.

dragon world 2 still has strange protection issues, we have to patch the code for now, what
should really happen, it jumps to invalid code, should the protection device cause the 68k
to see valid code there or something?

kov superheroes uses a different protection chip / different protection commands and doesn't
work, some of the gfx also need redumping to check they're the same as kov, its using invalid
codes for the ones we have (could just be protection tho)

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
		cpu_set_halt_line(1, CLEAR_LINE);
		cpu_set_reset_line(1, PULSE_LINE);
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
		cpu_set_nmi_line( 1, PULSE_LINE );
	}
}

static WRITE_HANDLER( z80_l3_w )
{
	logerror("SL 3 z80.w %02x (%04x)\n", data, activecpu_get_pc());
	soundlatch3_w(0, data);
}

static void sound_irq(int level)
{
	cpu_set_irq_line(1, 0, level);
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
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 )

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
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
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
	PORT_DIPNAME( 0x0003, 0x0000, "Region" )
	PORT_DIPSETTING(      0x0000, "World" )
//	PORT_DIPSETTING(      0x0001, "World" ) // again?
	PORT_DIPSETTING(      0x0002, "Korea" )
	PORT_DIPSETTING(      0x0003, "China" )
INPUT_PORTS_END

INPUT_PORTS_START( sango )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 )
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
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
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
	PORT_DIPNAME( 0x000f, 0x0005, "Region" )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan (Alta License)" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, "World" )
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
		cpu_set_irq_line(0, 6, HOLD_LINE);
	else
		cpu_set_irq_line(0, 4, HOLD_LINE);
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

	install_mem_read16_handler (0, 0xC0400e, 0xC0400f, pgm_asic3_r);
	install_mem_write16_handler(0, 0xC04000, 0xC04001, pgm_asic3_reg_w);
	install_mem_write16_handler(0, 0xC0400e, 0xC0400f, pgm_asic3_w);
	install_mem_read16_handler (0, 0x80a70e, 0x80a70f, orlegend_speedup);
}

static DRIVER_INIT( dragwld2 )
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

	pgm_basic_init();

 	pgm_dw2_decrypt();

/* this seems to be some strange protection, its not right yet ..

<ElSemi> patch the jmp (a0) to jmp(a3) (otherwise they jump to illegal code)
<ElSemi> there are 3 (consecutive functions)
<ElSemi> 303bc
<ElSemi> 30462
<ElSemi> 304f2

*/
	mem16[0x1303bc/2]=0x4e93;
	mem16[0x130462/2]=0x4e93;
	mem16[0x1304F2/2]=0x4e93;

/*

<ElSemi> Here is how to "bypass" the dw2 hang protection, it fixes the mode
select and after failing in the 2nd stage (probably there are other checks
out there).

*/
	install_mem_read16_handler(0, 0xd80000, 0xd80003, dw2_d80000_r);

}

static DRIVER_INIT( kov )
{
	pgm_basic_init();

	install_mem_read16_handler(0, 0x500000, 0x500003, ASIC28_r16);
	install_mem_write16_handler(0, 0x500000, 0x500003, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	install_mem_read16_handler(0, 0x4f0000, 0x4fffff, sango_protram_r);

 	pgm_kov_decrypt();
}

static DRIVER_INIT( kovsh )
{
	pgm_basic_init();

	install_mem_read16_handler(0, 0x500000, 0x500003, ASIC28_r16);
	install_mem_write16_handler(0, 0x500000, 0x500003, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	install_mem_read16_handler(0, 0x4f0000, 0x4fffff, sango_protram_r);

 	pgm_kovsh_decrypt();
}

static DRIVER_INIT( djlzz )
{
	pgm_basic_init();

	install_mem_read16_handler(0, 0x500000, 0x500003, ASIC28_r16);
	install_mem_write16_handler(0, 0x500000, 0x500003, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	install_mem_read16_handler(0, 0x4f0000, 0x4fffff, sango_protram_r);

 	pgm_djlzz_decrypt();
}

static DRIVER_INIT( dw3 )
{
	pgm_basic_init();

//	install_mem_read16_handler(0, 0xda0000, 0xdaffff, dw3_prot_r);
//	install_mem_write16_handler(0, 0xda0000, 0xdaffff, dw3_prot_w);

 	pgm_dw3_decrypt();
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


ROM_START( dragwld2 )
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

ROM_START( dw3 )
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

ROM_START( dw3k )
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


/*** GAME ********************************************************************/

GAMEX( 1997, pgm,      0,          pgm, pgm,   0,          ROT0, "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAMEX( 1997, orlegend, pgm,        pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 126)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orlegnde, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orlegndc, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112, Chinese Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orld111c, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 111, Chinese Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orld105k, orlegend,   pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 105, Korean Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, dragwld2, pgm,        pgm, pgm,   dragwld2,   ROT0, "IGS", "Zhong Guo Long II (ver. 100C, China)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, kov,      pgm,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 117)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kov115,   kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 115)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kovj,     kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 100, Japanese Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kovplus,  kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour Plus / Sangoku Senki Plus (ver. 119)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, kovplusa, kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour Plus / Sangoku Senki Plus (alt ver. 119)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* not working */
GAMEX( 1998, dw3,      pgm,        pgm, sango, dw3,	       ROT0, "IGS", "Dragon World 3", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, dw3k,     dw3,        pgm, sango, dw3,	       ROT0, "IGS", "Dragon World 3 (Korean Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, photoy2k, pgm,        pgm, sango, djlzz, 	   ROT0, "IGS", "Photo Y2K", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, raf102j,  photoy2k,   pgm, sango, djlzz, 	   ROT0, "IGS", "Real and Fake / Photo Y2K (ver. 102, Japan Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, kovsh,    kov,        pgm, sango, kovsh,	   ROT0, "IGS", "Knights of Valour Superheroes / Sangoku Senki Superheroes (ver. 322)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
