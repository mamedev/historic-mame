/***************************************************************************

 Wild West C.O.W.boys of Moo Mesa
 Bucky O'Hare
 (c) 1992 Konami
 Driver by R. Belmont based on xexex.c by Olivier Galibert.
 Moo Mesa protection information thanks to ElSemi and OG.

 These are the final Xexex hardware games before the pre-GX/Mystic Warriors
 hardware took over.

TODO
----
 - 54338 color blender support (is this even used in moo?)
 - Moon in ghost town stage of moo is in front of everything.  Priority
   set in the sprite is in fact in front of the bg tilemaps, so the video
   emulation is doing it's job.  Core bug?
 - Memory trashing(?) during moo causes a hang around the second boss even
   with the protection correct.  (Save a state before that and restore it
   to work around).
 - The table on ropes looks weird when it's first scrolling down.  Appears
   to be related to some '157 linescroll bugs also seen in Xexex.

CHANGELOG
---------
* March 18, 2002 (RB except as noted)
 - Visible area adjusted to make sense (384x224 is a reasonable resolution
   for a standard-resolution PCB).  There's still a minor glitch on one of
   the intro scenes but I consider that acceptable.  And sorry Stephh, we
   definitely can't show the green lines.
 - Z80 banking corrected (bucky cared, moo didn't seem to)
 - Tilemaps in moo now use the correct '251 priority registers (confirmed
   by the swinging table stage, which sends the linescroll data using plane 1's
   RAM to the far back).
 - Game infos more exactly match the title screens (stephh)
 - Removed third button for moo (stephh)
 - Made bucky world version the parent as per mame tradition (stephh)
 - Can now pass ram/rom test with sound off (reported by stephh)

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"

VIDEO_START(moo);
VIDEO_UPDATE(moo);

static int cur_control2;

static int init_eeprom_count, init_nosound_count;

static data16_t *workram;

static data16_t protram[16];

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/* read command */
	"011100",		/* write command */
	"0100100000000",	/* erase command */
	"0100000000000",	/* lock command */
	"0100110000000" 	/* unlock command */
};

static NVRAM_HANDLER( moo )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( control1_r )
{
	int res;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	/* bit 3 is service button */
	/* bits 4-7 are DIP switches */
	res = EEPROM_read_bit() | input_port_1_r(0);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7;
	}

	return res;
}

static READ16_HANDLER( control2_r )
{
	return cur_control2;
}

static WRITE16_HANDLER( control2_w )
{
	/* bit 0  is data */
	/* bit 1  is cs (active low) */
	/* bit 2  is clock (active high) */
	/* bit 8 = enable sprite ROM reading */
	/* bit 10 is watchdog */

	cur_control2 = data;

	EEPROM_write_bit(cur_control2 & 0x01);
	EEPROM_set_cs_line((cur_control2 & 0x02) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((cur_control2 & 0x04) ? ASSERT_LINE : CLEAR_LINE);

	if (data & 0x100)
	{
		K053246_set_OBJCHA_line(ASSERT_LINE);
	}
	else
	{
		K053246_set_OBJCHA_line(CLEAR_LINE);
	}
}

static INTERRUPT_GEN(moo_interrupt)
{
	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, 4, HOLD_LINE);
			break;

		case 1:
			if (K053246_is_IRQ_enabled())
				cpu_set_irq_line(0, 5, HOLD_LINE);
			break;
	}
}

static WRITE16_HANDLER( sound_cmd1_w )
{
	if((data & 0x00ff0000) == 0) {
		data &= 0xff;
		soundlatch_w(0, data);
	}
}

static WRITE16_HANDLER( sound_cmd2_w )
{
	if((data & 0x00ff0000) == 0) {
		soundlatch2_w(0, data & 0xff);
	}
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( sound_status_r )
{
	int latch = soundlatch3_r(0);

	/* make test pass with sound off.
	   these games are trickier than your usual konami stuff, they expect to
	   read 0xff (meaning the z80 booted properly) then 0x80 (z80 busy) then
	   the self-test result */
	if (!Machine->sample_rate) {
		if (init_nosound_count < 10)
		{
			if (!init_nosound_count)
				latch = 0xff;
			else
				latch = 0x80;
			init_nosound_count++;
		}
		else
		{
			latch = 0x0f;
		}
	}

	return latch;
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + (data&0xf)*0x4000);
}

/* the interface with the 053247 is weird. The chip can address only 0x1000 bytes */
/* of RAM, but they put 0x10000 there. The CPU can access them all. */
static READ16_HANDLER( K053247_scattered_word_r )
{
	if (offset & 0x0078)
		return spriteram16[offset];
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x7f80) >> 4);
		return K053247_word_r(offset,mem_mask);
	}
}

static WRITE16_HANDLER( K053247_scattered_word_w )
{
	if (offset & 0x0078)
		COMBINE_DATA(spriteram16+offset);
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x7f80) >> 4);

		K053247_word_w(offset,data,mem_mask);
	}
}

static READ16_HANDLER( player1_r ) 	// players 1 and 3
{
	return input_port_2_r(0) | (input_port_4_r(0)<<8);
}

static READ16_HANDLER( player2_r )	// players 2 and 4
{
	return input_port_3_r(0) | (input_port_5_r(0)<<8);
}

static WRITE16_HANDLER( moo_prot_w )
{
	UINT32 src1, src2, dst, length, a, b, res;

	COMBINE_DATA(&protram[offset]);

	if (offset == 0xc)	// trigger operation
	{
		src1 = (protram[1]&0xff)<<16 | protram[0];
		src2 = (protram[3]&0xff)<<16 | protram[2];
		dst = (protram[5]&0xff)<<16 | protram[4];
		length = protram[0xf];

		while (length)
		{
			a = cpu_readmem24bew_word(src1);
			b = cpu_readmem24bew_word(src2);
			res = a+2*b;

			cpu_writemem24bew_word(dst, res);

			src1 += 2;
			src2 += 2;
			dst += 2;
			length--;
		}
	}
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x0c4000, 0x0c4001, K053246_word_r },
	{ 0x0d6014, 0x0d6015, sound_status_r },
	{ 0x0da000, 0x0da001, player1_r },
	{ 0x0da002, 0x0da003, player2_r },
	{ 0x0dc000, 0x0dc001, input_port_0_word_r },
	{ 0x0dc002, 0x0dc003, control1_r },
	{ 0x0de000, 0x0de001, control2_r },
	{ 0x100000, 0x17ffff, MRA16_ROM },
	{ 0x180000, 0x18ffff, MRA16_RAM },	      	/* Work RAM */
	{ 0x190000, 0x19ffff, K053247_scattered_word_r },
	{ 0x1a0000, 0x1a1fff, K054157_ram_word_r },	/* Graphic planes */
	{ 0x1b0000, 0x1b1fff, K054157_rom_word_r }, 	/* Passthrough to tile roms */
	{ 0x1c0000, 0x1c1fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x0c0000, 0x0c003f, K054157_word_w },
	{ 0x0c2000, 0x0c2007, K053246_word_w },
	{ 0x0ca000, 0x0ca01f, MWA16_NOP },	/* K054338 alpha blending engine, coming soon */
	{ 0x0cc000, 0x0cc01f, K053251_lsb_w },
	{ 0x0ce000, 0x0ce01f, moo_prot_w },
	{ 0x0d0000, 0x0d001f, MWA16_NOP },
	{ 0x0d4000, 0x0d4001, sound_irq_w },
	{ 0x0d600c, 0x0d600d, sound_cmd1_w },
	{ 0x0d600e, 0x0d600f, sound_cmd2_w },
	{ 0x0d8000, 0x0d8007, K054157_b_word_w },
	{ 0x0de000, 0x0de001, control2_w },
	{ 0x100000, 0x17ffff, MWA16_ROM },
	{ 0x180000, 0x18ffff, MWA16_RAM, &workram },
	{ 0x190000, 0x19ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x1a0000, 0x1a1fff, K054157_ram_word_w },
	{ 0x1c0000, 0x1c1fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( buckyreadmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },
	{ 0x090000, 0x09ffff, K053247_scattered_word_r },
	{ 0x0a0000, 0x0affff, MRA16_RAM },
	{ 0x0c4000, 0x0c4001, K053246_word_r },
	{ 0x0d2000, 0x0d20ff, K054000_lsb_r },
	{ 0x0d6014, 0x0d6015, sound_status_r },
	{ 0x0da000, 0x0da001, player1_r },
	{ 0x0da002, 0x0da003, player2_r },
	{ 0x0dc000, 0x0dc001, input_port_0_word_r },
	{ 0x0dc002, 0x0dc003, control1_r },
	{ 0x0de000, 0x0de001, control2_r },
	{ 0x180000, 0x181fff, K054157_ram_word_r },	/* Graphic planes */
	{ 0x182000, 0x187fff, MRA16_RAM },
	{ 0x190000, 0x191fff, K054157_rom_word_r }, 	/* Passthrough to tile roms */
	{ 0x1b0000, 0x1b3fff, MRA16_RAM },
	{ 0x200000, 0x23ffff, MRA16_ROM },		/* data */
MEMORY_END

static MEMORY_WRITE16_START( buckywritemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x090000, 0x09ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x0a0000, 0x0affff, MWA16_RAM },
	{ 0x0c0000, 0x0c003f, K054157_word_w },
	{ 0x0c2000, 0x0c2007, K053246_word_w },
	{ 0x0ca000, 0x0ca01f, MWA16_NOP },	/* K054338 alpha blending engine, coming soon */
	{ 0x0cc000, 0x0cc01f, K053251_lsb_w },
	{ 0x0ce000, 0x0ce01f, moo_prot_w },
	{ 0x0d0000, 0x0d001f, MWA16_NOP },
	{ 0x0d2000, 0x0d20ff, K054000_lsb_w },
	{ 0x0d4000, 0x0d4001, sound_irq_w },
	{ 0x0d600c, 0x0d600d, sound_cmd1_w },
	{ 0x0d600e, 0x0d600f, sound_cmd2_w },
	{ 0x0d8000, 0x0d8007, K054157_b_word_w },
	{ 0x0de000, 0x0de001, control2_w },
	{ 0x180000, 0x181fff, K054157_ram_word_w },
	{ 0x182000, 0x187fff, MWA16_RAM },
	{ 0x1b0000, 0x1b3fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x200000, 0x23ffff, MWA16_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xec01, 0xec01, YM2151_status_port_0_r },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xec00, 0xec00, YM2151_register_port_0_w },
	{ 0xec01, 0xec01, YM2151_data_port_0_w },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, sound_bankswitch_w },
MEMORY_END


INPUT_PORTS_START( moo )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0xc0, 0x80, "Number of Players")
	PORT_DIPSETTING(    0xc0, "2")
	PORT_DIPSETTING(    0x40, "3")
	PORT_DIPSETTING(    0x80, "4")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

/* Same as 'moo', but additional "Button 3" for all players */
INPUT_PORTS_START( bucky )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0xc0, 0x80, "Number of Players")
	PORT_DIPSETTING(    0xc0, "2")
	PORT_DIPSETTING(    0x40, "3")
	PORT_DIPSETTING(    0x80, "4")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END


static struct YM2151interface ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K054539interface k054539_interface =
{
	1,			/* 1 chip */
	48000,
	{ REGION_SOUND1 },
	{ { 100, 100 } },
};

static MACHINE_INIT( moo )
{
	init_nosound_count = 0;
}


static MACHINE_DRIVER_START( moo )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(moo_interrupt, 2)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(moo)

	MDRV_NVRAM_HANDLER(moo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	/* visrgn derived by opening it all the way up, taking snapshots,
	   and measuring in the GIMP.  The character select screen demands this
	   geometry or stuff gets cut off */
	MDRV_VISIBLE_AREA(8*8, 56*8-1, 2*8, 30*8-1)

	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(moo)
	MDRV_VIDEO_UPDATE(moo)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bucky )
	MDRV_IMPORT_FROM(moo)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(buckyreadmem,buckywritemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(4096)
MACHINE_DRIVER_END



ROM_START( moo )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "151b01",    0x000000,  0x40000, CRC(fb2fa298) SHA1(f03b24681a2b329ba797fd2780ac9a3cf862ebcb) )
	ROM_LOAD16_BYTE( "151b02.ea", 0x000001,  0x40000, CRC(37b30c01) SHA1(cb91739097a4a36f8f8d92998d822ffc851e1279) )

	/* data */
	ROM_LOAD16_BYTE( "151a03", 0x100000,  0x40000, CRC(c896d3ea) SHA1(ea83c63e2c3dbc4f1e1d49f1852a78ffc1f0ea4b) )
	ROM_LOAD16_BYTE( "151a04", 0x100001,  0x40000, CRC(3b24706a) SHA1(c2a77944284e35ff57f0774fa7b67e53d3b63e1f) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "151a07", 0x000000, 0x040000, CRC(cde247fc) SHA1(cdee0228db55d53ae43d7cd2d9001dadd20c2c61) )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "151a05", 0x000000, 0x100000, CRC(bc616249) SHA1(58c1f1a03ce9bead8f79d12ce4b2d342432b24b5) )
	ROM_LOAD( "151a06", 0x100000, 0x100000, CRC(38dbcac1) SHA1(c357779733921695b20ac586db5b475f5b2b8f4c) )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "151a10", 0x000000, 0x200000, CRC(376c64f1) SHA1(eb69c5a27f9795e28f04a503955132f0a9e4de12) )
	ROM_LOAD( "151a11", 0x200000, 0x200000, CRC(e7f49225) SHA1(1255b214f29b6507540dad5892c60a7ae2aafc5c) )
	ROM_LOAD( "151a12", 0x400000, 0x200000, CRC(4978555f) SHA1(d9871f21d0c8a512b408e137e2e80e9392c2bf6f) )
	ROM_LOAD( "151a13", 0x600000, 0x200000, CRC(4771f525) SHA1(218d86b6230919b5db0304dac00513eb6b27ba9a) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "151a08", 0x000000, 0x200000, CRC(962251d7) SHA1(32dccf515d2ca8eeffb45cada3dcc60089991b77) )
ROM_END

ROM_START( mooua )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "151b01", 0x000000,  0x40000, CRC(fb2fa298) SHA1(f03b24681a2b329ba797fd2780ac9a3cf862ebcb) )
	ROM_LOAD16_BYTE( "151b02", 0x000001,  0x40000, CRC(3d9f4d59) SHA1(db47044bd4935fce94ec659242c9819c30eb6d0f) )

	/* data */
	ROM_LOAD16_BYTE( "151a03", 0x100000,  0x40000, CRC(c896d3ea) SHA1(ea83c63e2c3dbc4f1e1d49f1852a78ffc1f0ea4b) )
	ROM_LOAD16_BYTE( "151a04", 0x100001,  0x40000, CRC(3b24706a) SHA1(c2a77944284e35ff57f0774fa7b67e53d3b63e1f) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "151a07", 0x000000, 0x040000, CRC(cde247fc) SHA1(cdee0228db55d53ae43d7cd2d9001dadd20c2c61) )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "151a05", 0x000000, 0x100000, CRC(bc616249) SHA1(58c1f1a03ce9bead8f79d12ce4b2d342432b24b5) )
	ROM_LOAD( "151a06", 0x100000, 0x100000, CRC(38dbcac1) SHA1(c357779733921695b20ac586db5b475f5b2b8f4c) )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "151a10", 0x000000, 0x200000, CRC(376c64f1) SHA1(eb69c5a27f9795e28f04a503955132f0a9e4de12) )
	ROM_LOAD( "151a11", 0x200000, 0x200000, CRC(e7f49225) SHA1(1255b214f29b6507540dad5892c60a7ae2aafc5c) )
	ROM_LOAD( "151a12", 0x400000, 0x200000, CRC(4978555f) SHA1(d9871f21d0c8a512b408e137e2e80e9392c2bf6f) )
	ROM_LOAD( "151a13", 0x600000, 0x200000, CRC(4771f525) SHA1(218d86b6230919b5db0304dac00513eb6b27ba9a) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "151a08", 0x000000, 0x200000, CRC(962251d7) SHA1(32dccf515d2ca8eeffb45cada3dcc60089991b77) )
ROM_END

ROM_START( bucky )
	ROM_REGION( 0x240000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "173ea.b01", 0x000000,  0x40000, CRC(7785ac8a) SHA1(ef78d14f54d3a0b724b9702a18c67891e2d366a7) )
	ROM_LOAD16_BYTE( "173ea.b02", 0x000001,  0x40000, CRC(9b45f122) SHA1(325af1612e6f90ef9ae9353c43dc645be1f3465c) )

	/* data */
	ROM_LOAD16_BYTE( "t5", 0x200000,  0x20000, CRC(cd724026) SHA1(525445499604b713da4d8bc0a88e428654ceab95) )
	ROM_LOAD16_BYTE( "t6", 0x200001,  0x20000, CRC(7dd54d6f) SHA1(b0ee8ec445b92254bca881eefd4449972fed506a) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD("173.a07", 0x000000, 0x40000, CRC(4cdaee71) SHA1(bdc05d4475415f6fac65d7cdbc48df398e57845e) )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "173a05.t8",  0x000000, 0x100000, CRC(d14333b4) SHA1(d1a15ead2d156e1fceca0bf202ab3962411caf11) )
	ROM_LOAD( "173a06.t10", 0x100000, 0x100000, CRC(6541a34f) SHA1(15cf481498e3b7e0b2f7bfe5434121cc3bd65662) )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "173a10.b8",  0x000000, 0x200000, CRC(42fb0a0c) SHA1(d68c932cfabdec7896698b433525fe47ef4698d0) )
	ROM_LOAD( "173a11.a8",  0x200000, 0x200000, CRC(b0d747c4) SHA1(0cf1ee1b9a35ded31a81c321df2a076f7b588971) )
	ROM_LOAD( "173a12.b10", 0x400000, 0x200000, CRC(0fc2ad24) SHA1(6eda1043ee1266b8ba938a03a90bc7787210a936) )
	ROM_LOAD( "173a13.a10", 0x600000, 0x200000, CRC(4cf85439) SHA1(8c298bf0e659a830a1830a1180f4ce71215ade45) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD("173a08.b6", 0x000000, 0x200000, CRC(dcdded95) SHA1(8eeb546a0b60a35a6dce36c5ee872e6c93c577c9) )
	ROM_LOAD("173a09.a6", 0x200000, 0x200000, CRC(c93697c4) SHA1(0528a604868267a30d281b822c187df118566691) )
ROM_END

ROM_START( buckyua )
	ROM_REGION( 0x240000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "q5", 0x000000,  0x40000, CRC(dcaecca0) SHA1(c41847c9d89cdaf7cfa81ad9cc018c32592a882f) )
	ROM_LOAD16_BYTE( "q6", 0x000001,  0x40000, CRC(e3c856a6) SHA1(33cc8a29643e44b31ee280015c0c994bed72a0e3) )

	/* data */
	ROM_LOAD16_BYTE( "t5", 0x200000,  0x20000, CRC(cd724026) SHA1(525445499604b713da4d8bc0a88e428654ceab95) )
	ROM_LOAD16_BYTE( "t6", 0x200001,  0x20000, CRC(7dd54d6f) SHA1(b0ee8ec445b92254bca881eefd4449972fed506a) )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD("173.a07", 0x000000, 0x40000, CRC(4cdaee71) SHA1(bdc05d4475415f6fac65d7cdbc48df398e57845e) )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "173a05.t8",  0x000000, 0x100000, CRC(d14333b4) SHA1(d1a15ead2d156e1fceca0bf202ab3962411caf11) )
	ROM_LOAD( "173a06.t10", 0x100000, 0x100000, CRC(6541a34f) SHA1(15cf481498e3b7e0b2f7bfe5434121cc3bd65662) )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "173a10.b8",  0x000000, 0x200000, CRC(42fb0a0c) SHA1(d68c932cfabdec7896698b433525fe47ef4698d0) )
	ROM_LOAD( "173a11.a8",  0x200000, 0x200000, CRC(b0d747c4) SHA1(0cf1ee1b9a35ded31a81c321df2a076f7b588971) )
	ROM_LOAD( "173a12.b10", 0x400000, 0x200000, CRC(0fc2ad24) SHA1(6eda1043ee1266b8ba938a03a90bc7787210a936) )
	ROM_LOAD( "173a13.a10", 0x600000, 0x200000, CRC(4cf85439) SHA1(8c298bf0e659a830a1830a1180f4ce71215ade45) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD("173a08.b6", 0x000000, 0x200000, CRC(dcdded95) SHA1(8eeb546a0b60a35a6dce36c5ee872e6c93c577c9) )
	ROM_LOAD("173a09.a6", 0x200000, 0x200000, CRC(c93697c4) SHA1(0528a604868267a30d281b822c187df118566691) )
ROM_END


static DRIVER_INIT( moo )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);

	state_save_register_INT32("Moo", 0, "control2", (INT32 *)&cur_control2, 1);
	state_save_register_UINT16("Moo", 0, "protram", (UINT16 *)protram, 1);
}


GAME( 1992, moo,     0,       moo,     moo,     moo,      ROT0, "Konami", "Wild West C.O.W.-Boys of Moo Mesa (World version EA)")
GAME( 1992, mooua,   moo,     moo,     moo,     moo,      ROT0, "Konami", "Wild West C.O.W.-Boys of Moo Mesa (US version UA)")
GAME( 1992, bucky,   0,       bucky,   bucky,   moo,      ROT0, "Konami", "Bucky O'Hare (World version EA)")
GAME( 1992, buckyua, bucky,   bucky,   bucky,   moo,      ROT0, "Konami", "Bucky O'Hare (US version UA)")
