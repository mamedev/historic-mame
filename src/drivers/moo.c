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
	ROM_LOAD16_BYTE( "151b01",    0x000000,  0x40000, 0xfb2fa298 )
	ROM_LOAD16_BYTE( "151b02.ea", 0x000001,  0x40000, 0x37b30c01 )

	/* data */
	ROM_LOAD16_BYTE( "151a03", 0x100000,  0x40000, 0xc896d3ea )
	ROM_LOAD16_BYTE( "151a04", 0x100001,  0x40000, 0x3b24706a )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "151a07", 0x000000, 0x040000, 0xcde247fc )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "151a05", 0x000000, 0x100000, 0xbc616249 )
	ROM_LOAD( "151a06", 0x100000, 0x100000, 0x38dbcac1 )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "151a10", 0x000000, 0x200000, 0x376c64f1 )
	ROM_LOAD( "151a11", 0x200000, 0x200000, 0xe7f49225 )
	ROM_LOAD( "151a12", 0x400000, 0x200000, 0x4978555f )
	ROM_LOAD( "151a13", 0x600000, 0x200000, 0x4771f525 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "151a08", 0x000000, 0x200000, 0x962251d7 )
ROM_END

ROM_START( mooua )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "151b01", 0x000000,  0x40000, 0xfb2fa298 )
	ROM_LOAD16_BYTE( "151b02", 0x000001,  0x40000, 0x3d9f4d59 )

	/* data */
	ROM_LOAD16_BYTE( "151a03", 0x100000,  0x40000, 0xc896d3ea )
	ROM_LOAD16_BYTE( "151a04", 0x100001,  0x40000, 0x3b24706a )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD( "151a07", 0x000000, 0x040000, 0xcde247fc )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "151a05", 0x000000, 0x100000, 0xbc616249 )
	ROM_LOAD( "151a06", 0x100000, 0x100000, 0x38dbcac1 )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "151a10", 0x000000, 0x200000, 0x376c64f1 )
	ROM_LOAD( "151a11", 0x200000, 0x200000, 0xe7f49225 )
	ROM_LOAD( "151a12", 0x400000, 0x200000, 0x4978555f )
	ROM_LOAD( "151a13", 0x600000, 0x200000, 0x4771f525 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD( "151a08", 0x000000, 0x200000, 0x962251d7 )
ROM_END

ROM_START( bucky )
	ROM_REGION( 0x240000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "173ea.b01", 0x000000,  0x40000, 0x7785ac8a )
	ROM_LOAD16_BYTE( "173ea.b02", 0x000001,  0x40000, 0x9b45f122 )

	/* data */
	ROM_LOAD16_BYTE( "t5", 0x200000,  0x20000, 0xcd724026 )
	ROM_LOAD16_BYTE( "t6", 0x200001,  0x20000, 0x7dd54d6f )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD("173.a07", 0x000000, 0x40000, 0x4cdaee71 )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "173a05.t8",  0x000000, 0x100000, 0xd14333b4 )
	ROM_LOAD( "173a06.t10", 0x100000, 0x100000, 0x6541a34f )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "173a10.b8",  0x000000, 0x200000, 0x42fb0a0c )
	ROM_LOAD( "173a11.a8",  0x200000, 0x200000, 0xb0d747c4 )
	ROM_LOAD( "173a12.b10", 0x400000, 0x200000, 0x0fc2ad24 )
	ROM_LOAD( "173a13.a10", 0x600000, 0x200000, 0x4cf85439 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD("173a08.b6", 0x000000, 0x200000, 0xdcdded95)
	ROM_LOAD("173a09.a6", 0x200000, 0x200000, 0xc93697c4)
ROM_END

ROM_START( buckyua )
	ROM_REGION( 0x240000, REGION_CPU1, 0 )
	/* main program */
	ROM_LOAD16_BYTE( "q5", 0x000000,  0x40000, 0xdcaecca0 )
	ROM_LOAD16_BYTE( "q6", 0x000001,  0x40000, 0xe3c856a6 )

	/* data */
	ROM_LOAD16_BYTE( "t5", 0x200000,  0x20000, 0xcd724026 )
	ROM_LOAD16_BYTE( "t6", 0x200001,  0x20000, 0x7dd54d6f )

	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	/* Z80 sound program */
	ROM_LOAD("173.a07", 0x000000, 0x40000, 0x4cdaee71 )
	ROM_RELOAD(         0x010000, 0x040000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	/* tilemaps */
	ROM_LOAD( "173a05.t8",  0x000000, 0x100000, 0xd14333b4 )
	ROM_LOAD( "173a06.t10", 0x100000, 0x100000, 0x6541a34f )

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	/* sprites */
	ROM_LOAD( "173a10.b8",  0x000000, 0x200000, 0x42fb0a0c )
	ROM_LOAD( "173a11.a8",  0x200000, 0x200000, 0xb0d747c4 )
	ROM_LOAD( "173a12.b10", 0x400000, 0x200000, 0x0fc2ad24 )
	ROM_LOAD( "173a13.a10", 0x600000, 0x200000, 0x4cf85439 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	/* K054539 samples */
	ROM_LOAD("173a08.b6", 0x000000, 0x200000, 0xdcdded95)
	ROM_LOAD("173a09.a6", 0x200000, 0x200000, 0xc93697c4)
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
