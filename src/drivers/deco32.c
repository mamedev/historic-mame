/***************************************************************************

	Data East 32 bit ARM based games:

	Captain America
	Dragon Gun
	Fighter's History
	Locked 'N Loaded
	Tattoo Assassins

	Emulation by Bryan McPhail, mish@tendril.co.uk

	Captain America & Fighter's History - reset with both start buttons
	held down for test mode.  Reset with player 1 start held in Fighter's
	History for 'Pattern Editor'.

	Tattoo Assassins is a prototype, it is thought only 25 test units
	were manufactured and distributed to test arcades before the game
	was recalled.  The current romset includes only the eproms, not
	the soldered mask roms, therefore there are no graphics.  TA is
	the only game developed by Data East Pinball in USA, rather than
	Data East Corporation in Japan.

	Here is the story of the dumped romset, from mowerman@erols.com:

	"The game was Demoed on the East Coast by State Sales in Baltimore just
	before the plug was pulled.  Next auction there was a brand new TA
	cabinet, 25" dedicated for sale, no CPUboard but did have a sound board,
	bid didn't meet reserve (about $600).  The op ran into an ex employee of
	DE that was absorbed by Sega, he knew of about 10 boards in storage for
	shipment back to Sega Japan, cash talks & we scored a boardset.  Slapped
	the boardset in the cabinet & he sold the game to a big east coast truck
	stop operator who saw the machine & had to have it.  I heard latter that
	it showed up at auction on the East coast 2 more times."

	[Nb:  Data East Pinball only was absorbed by Sega, not Data East
	Corporation, and the boards were probably going to DE, rather than Sega]

	Tattoo Assassins uses DE Pinball soundboard 520-5077-00 R

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/arm/arm.h"
#include "cpu/h6280/h6280.h"
#include "cpu/m6809/m6809.h"
#include "decocrpt.h"
#include "machine/eeprom.h"
#include "deco32.h"

static data32_t *deco32_prot_ram, *deco32_ram;

/* With thanks to PinMame for BSMT2000 interface... */
static struct {
  int bufFull;
  int soundCmd;
  int bsmtData;
} locals;

static WRITE32_HANDLER( deco32_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

static WRITE32_HANDLER(tattass_sound_w)
{
	locals.soundCmd = data;
	locals.bufFull = TRUE;
}

static READ32_HANDLER( deco32_71_r )
{
	/* Bit 0x80 goes high when sprite DMA is complete, and low
	while it's in progress, we don't bother to emulate it */
	return 0xffffffff;
}

static READ32_HANDLER( captaven_irq_control_r )
{
	/* IRQ controller, bottom bit Vblank, other bit signals Vblank IRQ */
	switch (offset<<2) {
	case 0xc: return 0xffffffde | cpu_getvblank();
	}
	return 0xffffffff;
}

static READ32_HANDLER( captaven_prot_r )
{
	/* Protection/IO chip 75, same as Lemmings & Robocop 2 */
	switch (offset<<2) {
	case 0x0a0: return readinputport(0); /* Player 1 & 2 controls */
	case 0x158: return readinputport(1); /* Player 3 & 4 controls */
	case 0xed4: return readinputport(2); /* Misc */
	}

	logerror("%08x: Unmapped protection read %04x\n",activecpu_get_pc(),offset<<2);
	return 0xffffffff;
}

static READ32_HANDLER( captaven_soundcpu_r )
{
	/* Top byte - top bit low == sound cpu busy, bottom word is dips */

//	logerror("%08x Read sound cpu %d\n",activecpu_get_pc(),offset);
	return 0xffff0000 | readinputport(3);
}

static READ32_HANDLER( fghthist_prot_r )
{
	logerror("%08x:  Unhandled prot read %04x\n",activecpu_get_pc(),offset<<1);

//000029e8:  Unhandled prot read 012c		- VALUE ignored
//00002a00:  Unhandled prot read 0304		- VALUE ignored
//00001110:  Unhandled prot read 0006		- VALUE ignored

	if ((offset<<1)==0x88) {
 		switch (deco32_prot_ram[0x386]) {
		case 0x00000000: 	logerror("%08x:  prot read - returning 0xf\n",activecpu_get_pc()); return 0xf0000000; //5==two player?
		case 0x00010000: 	logerror("%08x:  prot read - returning 0xe\n",activecpu_get_pc()); return 0xe0000000;

		}

		return 0xf0000000;
	}

	switch (offset<<1) {

	/* A hardware XOR */
	case 0x6ee:
	case 0x0d0:
	case 0x30c:
	case 0x3c4:
		logerror("\t%08x:  %08x ^ %08x == %08x\n",activecpu_get_pc(),deco32_prot_ram[0]&0xffff0000,deco32_prot_ram[0x302>>1]&0xffff0000,(deco32_prot_ram[0]&0xffff0000)^(deco32_prot_ram[0x302>>1]&0xffff0000));
		return ((deco32_prot_ram[0]&0xffff0000)^(deco32_prot_ram[0x302>>1]&0xffff0000))&0x00010000;
	}

	return 0;
}

static WRITE32_HANDLER( fghthist_prot_w )
{
	COMBINE_DATA(&deco32_prot_ram[offset]);
	logerror("%08x:  Protection write %04x %08x\n",activecpu_get_pc(),offset<<1,data);
}

static READ32_HANDLER( fghthist_control_r )
{
	switch (offset) {
	case 0: return 0xffff0000 | readinputport(2);
	case 1: return (readinputport(1)<<16) | readinputport(0);
	case 2: return 0xfffffffe | EEPROM_read_bit();
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( fghthist_control_w )
{
	switch (offset<<2) {
	case 0xc:
		if (ACCESSING_LSB32) {
			EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_write_bit(data & 0x10);
			EEPROM_set_cs_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
		}
		return;
	}
}

static READ32_HANDLER( lockload_control_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( dragngun_prot_r )
{
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);

	switch (offset<<1) {
	case 0x140/2: return 0xffff0000 | readinputport(0);
	case 0xadc/2: return 0xffff0000 | readinputport(1);
	case 0x6a0/2: return 0xffffffff;
	}
	return 0xffffffff;
}

static READ32_HANDLER( lockload_lightgun_r )
{
	logerror("%08x:Read lightgun %08x (%08x)\n",activecpu_get_pc(),offset,mem_mask);
	switch (offset) {
	case 0:	return 0;//readinputport(0);
	case 1: return 0;//readinputport(1);
	}
	return 0;
}

static READ32_HANDLER( dragngun_control_r )
{
//log error all these
//	if (activecpu_get_pc()!=0xfd6f8)
//		logerror("%08x:Read control %08x (%08x)\n",activecpu_get_pc(),offset,mem_mask);

	switch (offset) {
//	case 1: return 0xffff0000 | readinputport(0);
	case 0: return 0xfffffffe | EEPROM_read_bit();

	case 3: /* Irq controller

		Bit 0:  1 = Vblank active
		Bit 1:
		Bit 2:
		Bit 3:
		Bit 4:	VBL Irq
		Bit 5:	Gun 0 IRQ? - As below
		Bit 6:	Gun 1 IRQ? - reads from 128008 (case 2), then 178000/170000 area
		Bit 7:
		*/
		return 0xffffff9e | cpu_getvblank();
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( dragngun_control_w )
{
	switch (offset<<2) {
	case 0x0:
		if (ACCESSING_LSB32) {
			EEPROM_set_clock_line((data & 0x2) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_write_bit(data & 0x1);
			EEPROM_set_cs_line((data & 0x4) ? CLEAR_LINE : ASSERT_LINE);
		}
		return;
	}

	logerror("%08x:Write control 1 %08x %08x\n",activecpu_get_pc(),offset,data);

}

static READ32_HANDLER( tattass_prot_r )
{
	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);

	switch (offset<<1) {
	case 0x4c4: return 0;//0xffff0000 | readinputport(0);

	}
	return 0xffffffff;
}

/**********************************************************************************/

static MEMORY_READ32_START( captaven_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },

	{ 0x100000, 0x100007, deco32_71_r },
	{ 0x110000, 0x110fff, MRA32_RAM }, /* Sprites */
	{ 0x120000, 0x127fff, MRA32_RAM }, /* Main RAM */
	{ 0x130000, 0x131fff, MRA32_RAM }, /* Palette RAM */
	{ 0x128000, 0x128fff, captaven_prot_r },
	{ 0x148000, 0x14800f, captaven_irq_control_r },
	{ 0x160000, 0x167fff, MRA32_RAM }, /* Extra work RAM */
	{ 0x168000, 0x168003, captaven_soundcpu_r },

	{ 0x180000, 0x18001f, MRA32_RAM },
	{ 0x190000, 0x191fff, MRA32_RAM },
	{ 0x194000, 0x195fff, MRA32_RAM },
	{ 0x1a0000, 0x1a1fff, MRA32_RAM },
	{ 0x1a4000, 0x1a5fff, MRA32_RAM },

	{ 0x1c0000, 0x1c001f, MRA32_RAM },
	{ 0x1d0000, 0x1d1fff, MRA32_RAM },
	{ 0x1e0000, 0x1e1fff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( captaven_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },

	{ 0x100000, 0x100003, buffer_spriteram32_w },
	{ 0x108000, 0x108003, MWA32_NOP }, /* vbl ack? */
	{ 0x148000, 0x148003, deco32_unknown_w },
	{ 0x148004, 0x148007, MWA32_NOP }, //??
	{ 0x148008, 0x14800b, MWA32_NOP }, /* irq ack? */
	{ 0x110000, 0x110fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x120000, 0x127fff, MWA32_RAM, &deco32_ram }, /* Main RAM */
	{ 0x1280c8, 0x1280cb, deco32_sound_w },
	{ 0x130000, 0x131fff, deco32_paletteram_w, &paletteram32 },
	{ 0x160000, 0x167fff, MWA32_RAM }, /* Additional work RAM */

	{ 0x178000, 0x178003, deco32_pri_w },
	{ 0x180000, 0x18001f, MWA32_RAM, &deco32_pf12_control },
	{ 0x190000, 0x191fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x192000, 0x193fff, deco32_pf1_data_w }, /* Mirror address - bug in program code */
	{ 0x194000, 0x195fff, deco32_pf2_data_w, &deco32_pf2_data },

	{ 0x1a0000, 0x1a1fff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x1a4000, 0x1a5fff, MWA32_RAM, &deco32_pf2_rowscroll },

	{ 0x1c0000, 0x1c001f, MWA32_RAM, &deco32_pf34_control },
	{ 0x1d0000, 0x1d1fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1e0000, 0x1e1fff, MWA32_RAM, &deco32_pf3_rowscroll },
MEMORY_END

static MEMORY_READ32_START( fghthist_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x120020, 0x12002f, fghthist_control_r },

	{ 0x168000, 0x169fff, MRA32_RAM },

	{ 0x178000, 0x178fff, MRA32_RAM },
	{ 0x179000, 0x179fff, MRA32_RAM },

	{ 0x182000, 0x183fff, MRA32_RAM },
	{ 0x184000, 0x185fff, MRA32_RAM },
	{ 0x192000, 0x1923ff, MRA32_RAM },
	{ 0x192800, 0x1928ff, MRA32_RAM },
	{ 0x194000, 0x1943ff, MRA32_RAM },
	{ 0x194800, 0x1948ff, MRA32_RAM },
	{ 0x1a0000, 0x1a001f, MRA32_RAM },

	{ 0x1c2000, 0x1c3fff, MRA32_RAM },
	{ 0x1c4000, 0x1c5fff, MRA32_RAM },
	{ 0x1d2000, 0x1d23ff, MRA32_RAM },
	{ 0x1d2800, 0x1d28ff, MRA32_RAM },
	{ 0x1d4000, 0x1d43ff, MRA32_RAM },
	{ 0x1d4800, 0x1d48ff, MRA32_RAM },
	{ 0x1e0000, 0x1e001f, MRA32_RAM },

	{ 0x16c000, 0x16c01f, MRA32_NOP },
	{ 0x17c000, 0x17c03f, MRA32_NOP },

	{ 0x200000, 0x200fff, fghthist_prot_r },

MEMORY_END

static MEMORY_WRITE32_START( fghthist_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM },
	{ 0x120020, 0x12002f, fghthist_control_w },
	{ 0x1201fc, 0x1201ff, deco32_sound_w },
	{ 0x168000, 0x169fff, deco32_paletteram_w, &paletteram32 },

	{ 0x178000, 0x178fff, MWA32_RAM, &spriteram32 },
	{ 0x179000, 0x179fff, MWA32_RAM, &spriteram32_2 },

	{ 0x182000, 0x183fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x184000, 0x185fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x192000, 0x1923ff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x192800, 0x1928ff, MWA32_RAM, &deco32_pf1_colscroll },
	{ 0x194000, 0x1943ff, MWA32_RAM, &deco32_pf2_rowscroll },
	{ 0x194800, 0x1948ff, MWA32_RAM, &deco32_pf2_colscroll },
	{ 0x1a0000, 0x1a001f, MWA32_RAM, &deco32_pf12_control },

	{ 0x1c2000, 0x1c3fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1c4000, 0x1c5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1d2000, 0x1d23ff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1d2800, 0x1d28ff, MWA32_RAM, &deco32_pf3_colscroll },
	{ 0x1d4000, 0x1d43ff, MWA32_RAM, &deco32_pf4_rowscroll },
	{ 0x1d4800, 0x1d48ff, MWA32_RAM, &deco32_pf4_colscroll },
	{ 0x1e0000, 0x1e001f, MWA32_RAM, &deco32_pf34_control },

	{ 0x16c000, 0x16c01f, MWA32_NOP },
	{ 0x17c000, 0x17c03f, MWA32_NOP },

	{ 0x200000, 0x200fff, fghthist_prot_w, &deco32_prot_ram },
MEMORY_END

static MEMORY_READ32_START( dragngun_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x120000, 0x120fff, dragngun_prot_r },
	{ 0x128000, 0x12800f, dragngun_control_r },  //locked
	{ 0x130000, 0x131fff, MRA32_RAM },

	{ 0x138000, 0x138003, MRA32_RAM }, //sprite ack?  return 0 else tight loop


//	{ 0x128000, 0x12800f, dragngun_control_r },  //locked

//	{ 0x170000, 0x17000f, dragngun_control_r },  //locked

	{ 0x180000, 0x18001f, MRA32_RAM },
	{ 0x190000, 0x191fff, MRA32_RAM },
	{ 0x194000, 0x195fff, MRA32_RAM },
	{ 0x1a0000, 0x1a0fff, MRA32_RAM },
	{ 0x1a4000, 0x1a4fff, MRA32_RAM },

	{ 0x1c0000, 0x1c001f, MRA32_RAM },
	{ 0x1d0000, 0x1d1fff, MRA32_RAM },
	{ 0x1d4000, 0x1d5fff, MRA32_RAM },
	{ 0x1e0000, 0x1e0fff, MRA32_RAM },
	{ 0x1e4000, 0x1e4fff, MRA32_RAM },

	{ 0x208000, 0x2083ff, MRA32_RAM }, //0x10 byte increments only
	{ 0x210000, 0x2103ff, MRA32_RAM }, //0x10 byte increments only
	{ 0x218000, 0x218fff, MRA32_RAM }, //0x10 byte increments only

	{ 0x204800, 0x204fff, MRA32_RAM }, //0x10 byte increments only
	{ 0x220000, 0x221fff, MRA32_RAM }, //0x10 byte increments only
	{ 0x228000, 0x2283ff, MRA32_RAM }, //0x10 byte increments only

	{ 0x300000, 0x3fffff, MRA32_ROM },

	{ 0x400000, 0x400003, lockload_control_r }, //tested 0xf


	{ 0x440000, 0x440003, lockload_control_r }, //test switch in here??

	{ 0x420000, 0x42000f, dragngun_control_r },
//	{ 0x1008000, 0x100ffff, MRA32_RAM },

MEMORY_END

static MEMORY_WRITE32_START( dragngun_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },
	{ 0x1204c0, 0x1204c3, deco32_sound_w },
	{ 0x130000, 0x131fff, deco32_paletteram_w, &paletteram32 },

	{ 0x180000, 0x18001f, MWA32_RAM, &deco32_pf12_control },
	{ 0x190000, 0x191fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x194000, 0x195fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x1a0000, 0x1a0fff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x1a4000, 0x1a4fff, MWA32_RAM, &deco32_pf2_rowscroll },

	{ 0x1c0000, 0x1c001f, MWA32_RAM, &deco32_pf34_control },
	{ 0x1d0000, 0x1d1fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1d4000, 0x1d5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1e0000, 0x1e0fff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1e4000, 0x1e4fff, MWA32_RAM, &deco32_pf4_rowscroll },

	{ 0x204800, 0x204fff, MWA32_RAM }, //0x10 byte increments only  // 13f ff stuff

	{ 0x208000, 0x2083ff, MWA32_RAM }, //lower sprite bank
	{ 0x210000, 0x2103ff, MWA32_RAM }, //upper sprite bank
//376b for the select screen
	{ 0x20c000, 0x20c3ff, MWA32_RAM },
	{ 0x218000, 0x218fff, MWA32_RAM, &spriteram32_2 },

	{ 0x228000, 0x2283ff, MWA32_RAM },

	{ 0x220000, 0x221fff, MWA32_RAM, &spriteram32 },

	{ 0x300000, 0x3fffff, MWA32_ROM },
	{ 0x420000, 0x42000f, dragngun_control_w },
MEMORY_END

static MEMORY_READ32_START( tattass_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x162000, 0x162fff, MRA32_RAM }, /* 'Jack' RAM!? */
	{ 0x168000, 0x169fff, MRA32_RAM },

	{ 0x170000, 0x171fff, MRA32_RAM },
	{ 0x178000, 0x179fff, MRA32_RAM },

	{ 0x182000, 0x183fff, MRA32_RAM },
	{ 0x184000, 0x185fff, MRA32_RAM },
	{ 0x192000, 0x1927ff, MRA32_RAM },
	{ 0x192800, 0x193fff, MRA32_RAM },
	{ 0x194000, 0x1947ff, MRA32_RAM },
	{ 0x194800, 0x195fff, MRA32_RAM },
	{ 0x1a0000, 0x1a001f, MRA32_RAM },

	{ 0x1c2000, 0x1c3fff, MRA32_RAM },
	{ 0x1c4000, 0x1c5fff, MRA32_RAM },
	{ 0x1d2000, 0x1d27ff, MRA32_RAM },
	{ 0x1d2800, 0x1d3fff, MRA32_RAM },
	{ 0x1d4000, 0x1d47ff, MRA32_RAM },
	{ 0x1d4800, 0x1d5fff, MRA32_RAM },
	{ 0x1e0000, 0x1e001f, MRA32_RAM },

	{ 0x200000, 0x200fff, tattass_prot_r },
MEMORY_END

static MEMORY_WRITE32_START( tattass_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM },
	{ 0x162000, 0x162fff, MWA32_RAM }, /* 'Jack' RAM!? */
	{ 0x168000, 0x169fff, deco32_paletteram_w, &paletteram32 },
	{ 0x170000, 0x171fff, MWA32_RAM, &spriteram32 },
	{ 0x178000, 0x179fff, MWA32_RAM, &spriteram32_2 },

	{ 0x182000, 0x183fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x184000, 0x185fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x192000, 0x1927ff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x192800, 0x193fff, MWA32_RAM, &deco32_pf1_colscroll },
	{ 0x194000, 0x1947ff, MWA32_RAM, &deco32_pf2_rowscroll },
	{ 0x194800, 0x195fff, MWA32_RAM, &deco32_pf2_colscroll },
	{ 0x1a0000, 0x1a001f, MWA32_RAM, &deco32_pf12_control },

	{ 0x1c2000, 0x1c3fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1c4000, 0x1c5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1d2000, 0x1d27ff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1d2800, 0x1d3fff, MWA32_RAM, &deco32_pf3_colscroll },
	{ 0x1d4000, 0x1d47ff, MWA32_RAM, &deco32_pf4_rowscroll },
	{ 0x1d4800, 0x1d5fff, MWA32_RAM, &deco32_pf4_colscroll },
	{ 0x1e0000, 0x1e001f, MWA32_RAM, &deco32_pf34_control },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

static WRITE_HANDLER(des_bsmt0_w) { locals.bsmtData = data; }
static WRITE_HANDLER(des_bsmt1_w)
{
	int reg = offset^ 0xff;
	logerror("BSMT write to %02X (V%X R%X) = %02X%02X\n",
				reg, reg % 11, reg / 11, locals.bsmtData, data);
	BSMT2000_data_0_w(reg, ((locals.bsmtData<<8)|data), 0);

	// BSMT is ready directly
	cpu_set_irq_line(1, M6809_IRQ_LINE, HOLD_LINE);
}
static READ_HANDLER(des_2000_r) { return 0; }
static READ_HANDLER(des_2002_r) { locals.bufFull = FALSE; return locals.soundCmd; }
static READ_HANDLER(des_2004_r) { return 0; }
static READ_HANDLER(des_2006_r) { return 0x80; } // BSMT is always ready
// Writing 0x80 here resets BSMT ?
static WRITE_HANDLER(des_2000_w) {/*DBGLOG(("2000w=%2x\n",data));*/}
static WRITE_HANDLER(des_2002_w) {/*DBGLOG(("2002w=%2x\n",data));*/}
static WRITE_HANDLER(des_2004_w) {/*DBGLOG(("2004w=%2x\n",data));*/}
static WRITE_HANDLER(des_2006_w) {/*DBGLOG(("2006w=%2x\n",data));*/}

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem_tattass )
	{ 0x000000, 0x001fff, MRA_RAM },
//	{ 0x2000, 0x2001, des_2000_r },
	{ 0x2002, 0x2003, des_2002_r },
//	{ 0x2004, 0x2005, des_2004_r },
	{ 0x2006, 0x2007, des_2006_r },
	{ 0x2000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_tattass )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2001, des_2000_w },
	{ 0x6000, 0x6000, des_bsmt0_w },
	{ 0xa000, 0xa0ff, des_bsmt1_w },
	{ 0x2000, 0xffff, MWA_ROM },
MEMORY_END

/**********************************************************************************/

/* Notes (2002.02.05) :

When the "Continue Coin" Dip Switch is set to "2 Start/1 Continue",
the "Coinage" Dip Switches have no effect.

START, BUTTON1 and COIN effects :

  2 players, common coin slots

STARTn starts a game for player n. It adds 100 energy points each time it is pressed
(provided there are still some credits, and energy is <= 900).

BUTTON1n selects the character for player n.

COIN1n adds credit(s)/coin(s).

  2 players, individual coin slots

NO STARTn button !

BUTTON1n starts a game for player n. It also adds 100 energy points for each credit
inserted for the player. It then selects the character for player n.

COIN1n adds 100 energy points (based on "Coinage") for player n when ingame if energy
<= 900, else adds credit(s)/coin(s) for player n.

  4 players, common coin slots

NO STARTn button !

BUTTON1n starts a game for player n. It gives 100 energy points. It then selects the
character for player n.

  4 players, individual coin slots

NO STARTn button !

BUTTON1n starts a game for player n. It also adds 100 energy points for each credit
inserted for the player. It then selects the character for player n.

COIN1n adds 100 energy points (based on "Coinage") for player n when ingame if energy
<= 900, else adds credit(s)/coin(s) for player n.

*/

INPUT_PORTS_START( captaven )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "1 Start/1 Continue" )
	PORT_DIPSETTING(      0x0000, "2 Start/1 Continue" )

	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "Coin Slots" )
	PORT_DIPSETTING(      0x1000, "Common" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x2000, 0x2000, "Max Players" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( fghthist )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 ) //test button
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 ) //test at $a008
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( lockload )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) //unused??
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 ) //unused
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) //probably unused
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) //reset button??
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )  //service??
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3 ) //check  //test BUTTON F2
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( tattass )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "1 Start/1 Continue" )
	PORT_DIPSETTING(      0x0000, "2 Start/1 Continue" )

	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "Coin Slots" )
	PORT_DIPSETTING(      0x1000, "Common" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x2000, 0x2000, "Max Players" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout tilelayout2 =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+0, RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+0, RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+0, 8, 0,  },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout spritelayout4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{3*8,2*8,1*8,0*8,7*8,6*8,5*8,4*8,
	 11*8,10*8,9*8,8*8,15*8,14*8,13*8,12*8},
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,
	 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*16*8
};

static struct GfxLayout spritelayout5 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{4,5,6,7},
	{3*8,2*8,1*8,0*8,7*8,6*8,5*8,4*8,
	 11*8,10*8,9*8,8*8,15*8,14*8,13*8,12*8},
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,
	 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo_captaven[] =
{
	{ REGION_GFX1, 0, &charlayout,        512, 32 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tilelayout,        512, 32 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tilelayout2,      1024,  4 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &spritelayout,        0, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_fghthist[] =
{
	{ REGION_GFX1, 0, &charlayout,          0,  16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tilelayout,        256,  16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tilelayout,        512,  32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &spritelayout,     1024, 128 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_dragngun[] =
{
	{ REGION_GFX1, 0, &charlayout,        512, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,        768, 16 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout2,      1024,  4 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout4,       0, 32 },	/* Sprites 16x16 */
	{ REGION_GFX4, 0, &spritelayout5,       0, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data >> 0)& 1) * 0x40000);
	OKIM6295_set_bank_base(1, ((data >> 1)& 1) * 0x40000);

	/* Todo:  Verify these are the right way around, or perhaps they are always
		used together */
	//logerror("YM2151 %04x %04x\n",offset,data);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 95, 60 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ 24000000 },
	{ 11 },
	{ REGION_SOUND1 },
	{ 100 }
};

/**********************************************************************************/

static INTERRUPT_GEN( deco32_vbl_interrupt )
{
	cpu_set_irq_line(0, ARM_IRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( tattass_snd_interrupt )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, HOLD_LINE);
}

static MACHINE_DRIVER_START( captaven )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/4)
	MDRV_CPU_MEMORY(captaven_readmem,captaven_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_captaven)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(captaven)
	MDRV_VIDEO_EOF(captaven)
	MDRV_VIDEO_UPDATE(captaven)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fghthist )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/4)
	MDRV_CPU_MEMORY(fghthist_readmem,fghthist_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_fghthist)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(fghthist)
	MDRV_VIDEO_UPDATE(fghthist)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dragngun )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/4)
	MDRV_CPU_MEMORY(dragngun_readmem,dragngun_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_dragngun)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(dragngun)
	MDRV_VIDEO_UPDATE(dragngun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tattass )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/4)
	MDRV_CPU_MEMORY(tattass_readmem,tattass_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem_tattass,sound_writemem_tattass)
	MDRV_CPU_PERIODIC_INT(tattass_snd_interrupt,489) /* Fixed FIRQ of 489Hz as measured on real (pinball) machine */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_fghthist)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(tattass)
	MDRV_VIDEO_UPDATE(tattass)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(BSMT2000, bsmt2000_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( captaven )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hn_00-4.1e",	0x000000, 0x20000, 0x147fb094 )
	ROM_LOAD32_BYTE( "hn_01-4.1h",	0x000001, 0x20000, 0x11ecdb95 )
	ROM_LOAD32_BYTE( "hn_02-4.1k",	0x000002, 0x20000, 0x35d2681f )
	ROM_LOAD32_BYTE( "hn_03-4.1m",	0x000003, 0x20000, 0x3b59ba05 )
	ROM_LOAD32_BYTE( "man-12.3e",	0x080000, 0x20000, 0xd6261e98 )
	ROM_LOAD32_BYTE( "man-13.3h",	0x080001, 0x20000, 0x40f0764d )
	ROM_LOAD32_BYTE( "man-14.3k",	0x080002, 0x20000, 0x7cb9a4bd )
	ROM_LOAD32_BYTE( "man-15.3m",	0x080003, 0x20000, 0xc7854fe8 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "hj_08.17k",	0x00000,  0x10000,  0x361fbd16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "man-00.8a",	0x000000,  0x80000,  0x7855a607 ) /* Encrypted tiles */

	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "man-05.16a",	0x000000,  0x40000,  0xd44d1995 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x140000,  0x40000 )
	ROM_CONTINUE( 			0x280000,  0x40000 )
	ROM_CONTINUE( 			0x3c0000,  0x40000 )
	ROM_LOAD( "man-04.14a",	0x040000,  0x40000,  0x541492a1 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x180000,  0x40000 )
	ROM_CONTINUE( 			0x2c0000,  0x40000 )
	ROM_CONTINUE( 			0x400000,  0x40000 )
	ROM_LOAD( "man-03.12a",	0x080000,  0x40000,  0x2d9c52b2 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x1c0000,  0x40000 )
	ROM_CONTINUE( 			0x300000,  0x40000 )
	ROM_CONTINUE( 			0x440000,  0x40000 )
	ROM_LOAD( "man-02.11a",	0x0c0000,  0x40000,  0x07674c05 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x200000,  0x40000 )
	ROM_CONTINUE( 			0x340000,  0x40000 )
	ROM_CONTINUE( 			0x480000,  0x40000 )
	ROM_LOAD( "man-01.10a",	0x100000,  0x40000,  0xae714ada ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x240000,  0x40000 )
	ROM_CONTINUE( 			0x380000,  0x40000 )
	ROM_CONTINUE( 			0x4c0000,  0x40000 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "man-06.17a",	0x000000,  0x100000,  0xa9a64297 )
	ROM_LOAD16_BYTE( "man-07.18a",	0x000001,  0x100000,  0xb1db200c )
	ROM_LOAD16_BYTE( "man-08.17c",	0x200000,  0x100000,  0x28e98e66 )
	ROM_LOAD16_BYTE( "man-09.21c",	0x200001,  0x100000,  0x1921245d )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "man-10.14k",	0x000000,  0x80000,  0x0132c578 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "man-11.16k",	0x000000,  0x80000,  0x0dc60a4c )
ROM_END

ROM_START( captavne )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hg_00-4.1e",	0x000000, 0x20000, 0x7008d43c )
	ROM_LOAD32_BYTE( "hg_01-4.1h",	0x000001, 0x20000, 0x53dc1042 )
	ROM_LOAD32_BYTE( "hg_02-4.1k",	0x000002, 0x20000, 0x9e3f9ee2 )
	ROM_LOAD32_BYTE( "hg_03-4.1m",	0x000003, 0x20000, 0xbc050740 )
	ROM_LOAD32_BYTE( "man-12.3e",	0x080000, 0x20000, 0xd6261e98 )
	ROM_LOAD32_BYTE( "man-13.3h",	0x080001, 0x20000, 0x40f0764d )
	ROM_LOAD32_BYTE( "man-14.3k",	0x080002, 0x20000, 0x7cb9a4bd )
	ROM_LOAD32_BYTE( "man-15.3m",	0x080003, 0x20000, 0xc7854fe8 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "hj_08.17k",	0x00000,  0x10000,  0x361fbd16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "man-00.8a",	0x000000,  0x80000,  0x7855a607 ) /* Encrypted tiles */

	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "man-05.16a",	0x000000,  0x40000,  0xd44d1995 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x140000,  0x40000 )
	ROM_CONTINUE( 			0x280000,  0x40000 )
	ROM_CONTINUE( 			0x3c0000,  0x40000 )
	ROM_LOAD( "man-04.14a",	0x040000,  0x40000,  0x541492a1 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x180000,  0x40000 )
	ROM_CONTINUE( 			0x2c0000,  0x40000 )
	ROM_CONTINUE( 			0x400000,  0x40000 )
	ROM_LOAD( "man-03.12a",	0x080000,  0x40000,  0x2d9c52b2 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x1c0000,  0x40000 )
	ROM_CONTINUE( 			0x300000,  0x40000 )
	ROM_CONTINUE( 			0x440000,  0x40000 )
	ROM_LOAD( "man-02.11a",	0x0c0000,  0x40000,  0x07674c05 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x200000,  0x40000 )
	ROM_CONTINUE( 			0x340000,  0x40000 )
	ROM_CONTINUE( 			0x480000,  0x40000 )
	ROM_LOAD( "man-01.10a",	0x100000,  0x40000,  0xae714ada ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x240000,  0x40000 )
	ROM_CONTINUE( 			0x380000,  0x40000 )
	ROM_CONTINUE( 			0x4c0000,  0x40000 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "man-06.17a",	0x000000,  0x100000,  0xa9a64297 )
	ROM_LOAD16_BYTE( "man-07.18a",	0x000001,  0x100000,  0xb1db200c )
	ROM_LOAD16_BYTE( "man-08.17c",	0x200000,  0x100000,  0x28e98e66 )
	ROM_LOAD16_BYTE( "man-09.21c",	0x200001,  0x100000,  0x1921245d )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "man-10.14k",	0x000000,  0x80000,  0x0132c578 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "man-11.16k",	0x000000,  0x80000,  0x0dc60a4c )
ROM_END

ROM_START( captavnu )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hh_00-19.1e",	0x000000, 0x20000, 0x08b870e0 )
	ROM_LOAD32_BYTE( "hh_01-19.1h",	0x000001, 0x20000, 0x0dc0feca )
	ROM_LOAD32_BYTE( "hh_02-19.1k",	0x000002, 0x20000, 0x26ef94c0 )
	ROM_LOAD32_BYTE( "hn_03-4.1m",	0x000003, 0x20000, 0x3b59ba05 )
	ROM_LOAD32_BYTE( "man-12.3e",	0x080000, 0x20000, 0xd6261e98 )
	ROM_LOAD32_BYTE( "man-13.3h",	0x080001, 0x20000, 0x40f0764d )
	ROM_LOAD32_BYTE( "man-14.3k",	0x080002, 0x20000, 0x7cb9a4bd )
	ROM_LOAD32_BYTE( "man-15.3m",	0x080003, 0x20000, 0xc7854fe8 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "hj_08.17k",	0x00000,  0x10000,  0x361fbd16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "man-00.8a",	0x000000,  0x80000,  0x7855a607 ) /* Encrypted tiles */

	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "man-05.16a",	0x000000,  0x40000,  0xd44d1995 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x140000,  0x40000 )
	ROM_CONTINUE( 			0x280000,  0x40000 )
	ROM_CONTINUE( 			0x3c0000,  0x40000 )
	ROM_LOAD( "man-04.14a",	0x040000,  0x40000,  0x541492a1 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x180000,  0x40000 )
	ROM_CONTINUE( 			0x2c0000,  0x40000 )
	ROM_CONTINUE( 			0x400000,  0x40000 )
	ROM_LOAD( "man-03.12a",	0x080000,  0x40000,  0x2d9c52b2 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x1c0000,  0x40000 )
	ROM_CONTINUE( 			0x300000,  0x40000 )
	ROM_CONTINUE( 			0x440000,  0x40000 )
	ROM_LOAD( "man-02.11a",	0x0c0000,  0x40000,  0x07674c05 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x200000,  0x40000 )
	ROM_CONTINUE( 			0x340000,  0x40000 )
	ROM_CONTINUE( 			0x480000,  0x40000 )
	ROM_LOAD( "man-01.10a",	0x100000,  0x40000,  0xae714ada ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x240000,  0x40000 )
	ROM_CONTINUE( 			0x380000,  0x40000 )
	ROM_CONTINUE( 			0x4c0000,  0x40000 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "man-06.17a",	0x000000,  0x100000,  0xa9a64297 )
	ROM_LOAD16_BYTE( "man-07.18a",	0x000001,  0x100000,  0xb1db200c )
	ROM_LOAD16_BYTE( "man-08.17c",	0x200000,  0x100000,  0x28e98e66 )
	ROM_LOAD16_BYTE( "man-09.21c",	0x200001,  0x100000,  0x1921245d )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "man-10.14k",	0x000000,  0x80000,  0x0132c578 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "man-11.16k",	0x000000,  0x80000,  0x0dc60a4c )
ROM_END

ROM_START( captavnj )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hj_00-2.1e",	0x000000, 0x20000, 0x10b1faaf )
	ROM_LOAD32_BYTE( "hj_01-2.1h",	0x000001, 0x20000, 0x62c59f27 )
	ROM_LOAD32_BYTE( "hj_02-2.1k",	0x000002, 0x20000, 0xce946cad )
	ROM_LOAD32_BYTE( "hj_03-2.1m",	0x000003, 0x20000, 0x140cf9ce )
	ROM_LOAD32_BYTE( "man-12.3e",	0x080000, 0x20000, 0xd6261e98 )
	ROM_LOAD32_BYTE( "man-13.3h",	0x080001, 0x20000, 0x40f0764d )
	ROM_LOAD32_BYTE( "man-14.3k",	0x080002, 0x20000, 0x7cb9a4bd )
	ROM_LOAD32_BYTE( "man-15.3m",	0x080003, 0x20000, 0xc7854fe8 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "hj_08.17k",	0x00000,  0x10000,  0x361fbd16 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "man-00.8a",	0x000000,  0x80000,  0x7855a607 ) /* Encrypted tiles */

	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "man-05.16a",	0x000000,  0x40000,  0xd44d1995 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x140000,  0x40000 )
	ROM_CONTINUE( 			0x280000,  0x40000 )
	ROM_CONTINUE( 			0x3c0000,  0x40000 )
	ROM_LOAD( "man-04.14a",	0x040000,  0x40000,  0x541492a1 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x180000,  0x40000 )
	ROM_CONTINUE( 			0x2c0000,  0x40000 )
	ROM_CONTINUE( 			0x400000,  0x40000 )
	ROM_LOAD( "man-03.12a",	0x080000,  0x40000,  0x2d9c52b2 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x1c0000,  0x40000 )
	ROM_CONTINUE( 			0x300000,  0x40000 )
	ROM_CONTINUE( 			0x440000,  0x40000 )
	ROM_LOAD( "man-02.11a",	0x0c0000,  0x40000,  0x07674c05 ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x200000,  0x40000 )
	ROM_CONTINUE( 			0x340000,  0x40000 )
	ROM_CONTINUE( 			0x480000,  0x40000 )
	ROM_LOAD( "man-01.10a",	0x100000,  0x40000,  0xae714ada ) /* Encrypted tiles */
	ROM_CONTINUE( 			0x240000,  0x40000 )
	ROM_CONTINUE( 			0x380000,  0x40000 )
	ROM_CONTINUE( 			0x4c0000,  0x40000 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "man-06.17a",	0x000000,  0x100000,  0xa9a64297 )
	ROM_LOAD16_BYTE( "man-07.18a",	0x000001,  0x100000,  0xb1db200c )
	ROM_LOAD16_BYTE( "man-08.17c",	0x200000,  0x100000,  0x28e98e66 )
	ROM_LOAD16_BYTE( "man-09.21c",	0x200001,  0x100000,  0x1921245d )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "man-10.14k",	0x000000,  0x80000,  0x0132c578 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "man-11.16k",	0x000000,  0x80000,  0x0dc60a4c )
ROM_END

ROM_START( dragngun )
	ROM_REGION(0x400000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "kb02.bin", 0x000000, 0x40000, 0x4fb9cfea )
	ROM_LOAD32_BYTE( "kb06.bin", 0x000001, 0x40000, 0x2395efec )
	ROM_LOAD32_BYTE( "kb00.bin", 0x000002, 0x40000, 0x1539ff35 )
	ROM_LOAD32_BYTE( "kb04.bin", 0x000003, 0x40000, 0x5b5c1ec2 )
	ROM_LOAD32_BYTE( "kb03.bin", 0x300000, 0x40000, 0x6c6a4f42 )
	ROM_LOAD32_BYTE( "kb07.bin", 0x300001, 0x40000, 0x2637e8a1 )
	ROM_LOAD32_BYTE( "kb01.bin", 0x300002, 0x40000, 0xd780ba8d )
	ROM_LOAD32_BYTE( "kb05.bin", 0x300003, 0x40000, 0xfbad737b )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "kb10snd.bin",  0x00000,  0x10000,  0xec56f560 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "kb08.bin",  0x00000,  0x10000,  0x8fe4e5f5 ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "kb09.bin",  0x00001,  0x10000,  0xe9dcac3f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dgma0.bin",  0x00000,  0x80000,  0xd0491a37 ) /* Encrypted tiles */
	ROM_LOAD( "dgma1.bin",  0x80000,  0x80000,  0xd5970365 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dgma2.bin",   0x000000, 0x40000,  0xc6cd4baf ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x100000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x200000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x300000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "dgma3.bin",   0x040000, 0x40000,  0x793006d7 ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x140000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x240000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x340000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "dgma4.bin",   0x080000, 0x40000,  0x56631a2b ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x180000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x280000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x380000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "dgma5.bin",   0x0c0000, 0x40000,  0xac16e7ae ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x1c0000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x2c0000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x3c0000, 0x40000 ) /* 3/4 */

	ROM_REGION( 0x800000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "dgma9.bin",  0x000000, 0x100000,  0x18fec9e1 )
	ROM_LOAD32_BYTE( "dgma10.bin", 0x400000, 0x100000,  0x73126fbc )
	ROM_LOAD32_BYTE( "dgma11.bin", 0x000001, 0x100000,  0x1fc638a4 )
	ROM_LOAD32_BYTE( "dgma12.bin", 0x400001, 0x100000,  0x4c412512 )
	ROM_LOAD32_BYTE( "dgma13.bin", 0x000002, 0x100000,  0xd675821c )
	ROM_LOAD32_BYTE( "dgma14.bin", 0x400002, 0x100000,  0x22d38c71 )
	ROM_LOAD32_BYTE( "dgma15.bin", 0x000003, 0x100000,  0xec976b20 )
	ROM_LOAD32_BYTE( "dgma16.bin", 0x400003, 0x100000,  0x8b329bc8 )

	ROM_REGION( 0x100000, REGION_GFX5, 0 ) /* Video data - unused for now */
	ROM_LOAD( "dgma17.bin",  0x00000,  0x100000,  0x7799ed23 )
	ROM_LOAD( "dgma18.bin",  0x00000,  0x100000,  0xded66da9 )
	ROM_LOAD( "dgma19.bin",  0x00000,  0x100000,  0xbdd1ed20 )
	ROM_LOAD( "dgma20.bin",  0x00000,  0x100000,  0xfa0462f0 )
	ROM_LOAD( "dgma21.bin",  0x00000,  0x100000,  0x2d0a28ae )
	ROM_LOAD( "dgma22.bin",  0x00000,  0x100000,  0xc85f3559 )
	ROM_LOAD( "dgma23.bin",  0x00000,  0x100000,  0xba907d6a )
	ROM_LOAD( "dgma24.bin",  0x00000,  0x100000,  0x5cec45c8 )
	ROM_LOAD( "dgma25.bin",  0x00000,  0x100000,  0xd65d895c )
	ROM_LOAD( "dgma26.bin",  0x00000,  0x100000,  0x246a06c5 )
	ROM_LOAD( "dgma27.bin",  0x00000,  0x100000,  0x3fcbd10f )
	ROM_LOAD( "dgma28.bin",  0x00000,  0x100000,  0x5a2ec71d )

	ROM_REGION(0x80000, REGION_SOUND3, 0 )
	ROM_LOAD( "dgadpcm1.bin", 0x000000, 0x80000,  0xb9281dfd )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "dgadpcm2.bin", 0x000000, 0x80000,  0x3e006c6e )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "dgadpcm3.bin", 0x000000, 0x80000,  0x40287d62 )
ROM_END

ROM_START( fghthist )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "kz00-1.1f", 0x000000, 0x80000, 0x3a3dd15c )
	ROM_LOAD32_WORD( "kz01-1.2f", 0x000002, 0x80000, 0x86796cd6 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "kz02.18k",  0x00000,  0x10000,  0x5fd2309c )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbf00-8.bin",  0x000000,  0x100000,  0xd3e9b580 ) /* Encrypted tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbf01-8.bin",  0x000000,  0x100000,  0x0c6ed2eb ) /* Encrypted tiles */

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "mbf02-16.bin",  0x000001,  0x200000,  0xc19c5953 )
	ROM_LOAD16_BYTE( "mbf04-16.bin",  0x000000,  0x200000,  0xf6a23fd7 )
	ROM_LOAD16_BYTE( "mbf03-16.bin",  0x400001,  0x200000,  0x37d25c75 )
	ROM_LOAD16_BYTE( "mbf05-16.bin",  0x400000,  0x200000,  0x137be66d )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "mbf06.bin",  0x000000,  0x80000,  0xfb513903 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "mbf07.bin",  0x000000,  0x80000,  0x51d4adc7 )

	ROM_REGION(512, REGION_PROMS, 0 )
	ROM_LOAD( "mb7124h.8j",  0,  512,  0x7294354b )
ROM_END

ROM_START( fghthstw )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "fhist00.bin", 0x000000, 0x80000, 0xfe5eaba1 ) /* Rom kx */
	ROM_LOAD32_WORD( "fhist01.bin", 0x000002, 0x80000, 0x3fb8d738 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "kz02.18k",  0x00000,  0x10000,  0x5fd2309c )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbf00-8.bin",  0x000000,  0x100000,  0xd3e9b580 ) /* Encrypted tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbf01-8.bin",  0x000000,  0x100000,  0x0c6ed2eb ) /* Encrypted tiles */

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "mbf02-16.bin",  0x000001,  0x200000,  0xc19c5953 )
	ROM_LOAD16_BYTE( "mbf04-16.bin",  0x000000,  0x200000,  0xf6a23fd7 )
	ROM_LOAD16_BYTE( "mbf03-16.bin",  0x400001,  0x200000,  0x37d25c75 )
	ROM_LOAD16_BYTE( "mbf05-16.bin",  0x400000,  0x200000,  0x137be66d )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "mbf06.bin",  0x000000,  0x80000,  0xfb513903 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "mbf07.bin",  0x000000,  0x80000,  0x51d4adc7 )

	ROM_REGION(512, REGION_PROMS, 0 )
	ROM_LOAD( "mb7124h.8j",  0,  512,  0x7294354b )
ROM_END

ROM_START( lockload )
	ROM_REGION(0x400000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "nh-00-0.b5", 0x000002, 0x80000, 0xb8a57164 )
	ROM_LOAD32_BYTE( "nh-01-0.b8", 0x000000, 0x80000, 0xe371ac50 )
	ROM_LOAD32_BYTE( "nh-02-0.d5", 0x000003, 0x80000, 0x3e361e82 )
	ROM_LOAD32_BYTE( "nh-03-0.d8", 0x000001, 0x80000, 0xd08ee9c3 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "nh-06-0.n25",  0x00000,  0x10000,  0x7a1af51d )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nh-04-0.b15",  0x00000,  0x10000,  0xf097b3d9 ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "nh-05-0.b17",  0x00001,  0x10000,  0x448fec1e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbm-00.d15",  0x00000, 0x80000,  0xb97de8ff ) /* Encrypted tiles */
	ROM_LOAD( "mbm-01.d17",  0x80000, 0x80000,  0x6d4b8fa0 )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mbm-02.b23",  0x000000, 0x40000,  0xe723019f ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x200000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x400000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x600000, 0x40000 ) /* 3/4 */
	ROM_CONTINUE(            0x040000, 0x40000 ) /* Next block 2bpp 0/4 */
	ROM_CONTINUE(            0x240000, 0x40000 ) /* 1/4 */
	ROM_CONTINUE(            0x440000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x640000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "mbm-03.b26",  0x080000, 0x40000,  0xe0d09894 ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x280000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x480000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x680000, 0x40000 ) /* 3/4 */
	ROM_CONTINUE(            0x0c0000, 0x40000 ) /* Next block 2bpp 0/4 */
	ROM_CONTINUE(            0x2c0000, 0x40000 ) /* 1/4 */
	ROM_CONTINUE(            0x4c0000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x6c0000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "mbm-04.e23",  0x100000, 0x40000,  0x9e12466f ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x300000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x500000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x700000, 0x40000 ) /* 3/4 */
	ROM_CONTINUE(            0x140000, 0x40000 ) /* Next block 2bpp 0/4 */
	ROM_CONTINUE(            0x340000, 0x40000 ) /* 1/4 */
	ROM_CONTINUE(            0x540000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x740000, 0x40000 ) /* 3/4 */
	ROM_LOAD( "mbm-05.e26",  0x180000, 0x40000,  0x6ff02dc0 ) /* Encrypted tiles 0/4 */
	ROM_CONTINUE(            0x380000, 0x40000 ) /* 2 bpp per 0x40000 chunk, 1/4 */
	ROM_CONTINUE(            0x580000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x780000, 0x40000 ) /* 3/4 */
	ROM_CONTINUE(            0x1c0000, 0x40000 ) /* Next block 2bpp 0/4 */
	ROM_CONTINUE(            0x3c0000, 0x40000 ) /* 1/4 */
	ROM_CONTINUE(            0x5c0000, 0x40000 ) /* 2/4 */
	ROM_CONTINUE(            0x7c0000, 0x40000 ) /* 3/4 */

	ROM_REGION( 0x800000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "mbm-08.a14",  0x000000, 0x100000,  0x5358a43b )
	ROM_LOAD32_BYTE( "mbm-09.a16",  0x400000, 0x100000,  0x2cce162f )
	ROM_LOAD32_BYTE( "mbm-10.a19",  0x000001, 0x100000,  0x232e1c91 )
	ROM_LOAD32_BYTE( "mbm-11.a20",  0x400001, 0x100000,  0x8a2a2a9f )
	ROM_LOAD32_BYTE( "mbm-12.a21",  0x000002, 0x100000,  0x7d221d66 )
	ROM_LOAD32_BYTE( "mbm-13.a22",  0x400002, 0x100000,  0x678b9052 )
	ROM_LOAD32_BYTE( "mbm-14.a23",  0x000003, 0x100000,  0x5aaaf929 )
	ROM_LOAD32_BYTE( "mbm-15.a25",  0x400003, 0x100000,  0x789ce7b1 )

	ROM_REGION( 0x100000, REGION_GFX5, 0 ) /* Video data - unused for now, same as Dragongun */
//	ROM_LOAD( "dgma17.bin",  0x00000,  0x100000,  0x7799ed23 ) /* Todo - fix filenames */
	ROM_LOAD( "dgma18.bin",  0x00000,  0x100000,  0xded66da9 )
	ROM_LOAD( "dgma19.bin",  0x00000,  0x100000,  0xbdd1ed20 )
	ROM_LOAD( "dgma20.bin",  0x00000,  0x100000,  0xfa0462f0 )
	ROM_LOAD( "dgma21.bin",  0x00000,  0x100000,  0x2d0a28ae )
	ROM_LOAD( "dgma22.bin",  0x00000,  0x100000,  0xc85f3559 )
	ROM_LOAD( "dgma23.bin",  0x00000,  0x100000,  0xba907d6a )
	ROM_LOAD( "dgma24.bin",  0x00000,  0x100000,  0x5cec45c8 )
	ROM_LOAD( "dgma25.bin",  0x00000,  0x100000,  0xd65d895c )
	ROM_LOAD( "dgma26.bin",  0x00000,  0x100000,  0x246a06c5 )
	ROM_LOAD( "dgma27.bin",  0x00000,  0x100000,  0x3fcbd10f )
	ROM_LOAD( "dgma28.bin",  0x00000,  0x100000,  0x5a2ec71d )

	ROM_REGION(0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "mbm-06.n17",  0x00000, 0x100000,  0xf34d5999 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "mbm-07.n21",  0x00000, 0x80000,  0x414f3793 )

	ROM_REGION(0x80000, REGION_SOUND3, 0 )
	ROM_LOAD( "mar-07.n19",  0x00000, 0x80000,  0x40287d62 )
ROM_END

ROM_START( tattass )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "pp44.cpu", 0x000000, 0x80000, 0xc3ca5b49 )
	ROM_LOAD32_WORD( "pp45.cpu", 0x000002, 0x80000, 0xd3f30de0 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "u7.snd",  0x00000, 0x10000,  0x6947be8a )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfx1",  0x00000, 0x10000,  0 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gfx2",  0x00000, 0x10000,  0 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gfx3",  0x00000, 0x10000,  0 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gfx4",  0x00000, 0x10000,  0 )

	ROM_REGION(0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "u17.snd",  0x00000, 0x80000,  0xb945c18d )
	ROM_LOAD( "u36.snd",  0x00000, 0x80000,  0x3b73abe2 )
	ROM_LOAD( "u21.snd",  0x00000, 0x80000,  0x10b2110c )
	ROM_LOAD( "u37.snd",  0x00000, 0x80000,  0x986066b5 )
ROM_END

/**********************************************************************************/

static READ32_HANDLER( captaven_skip )
{
	data32_t ret=deco32_ram[0x748c/4];

	if (activecpu_get_pc()==0x39e8 && (ret&0xff)!=0) {
//		logerror("CPU Spin - %d cycles left this frame ran %d (%d)\n",cycles_left_to_run(),cycles_currently_ran(),cycles_left_to_run()+cycles_currently_ran());
		cpu_spinuntil_int();
	}

	return ret;
}

static READ32_HANDLER( dragngun_skip )
{
	data32_t ret=deco32_ram[0x1f15c/4];

	if (activecpu_get_pc()==0x628c && (ret&0xff)!=0) {
		logerror("%08x (%08x): CPU Spin - %d cycles left this frame ran %d (%d)\n",activecpu_get_pc(),ret,cycles_left_to_run(),cycles_currently_ran(),cycles_left_to_run()+cycles_currently_ran());
		cpu_spinuntil_int();
	}

	return ret;
}

static void init_captaven(void)
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);

	install_mem_read32_handler(0, 0x12748c, 0x12748f, captaven_skip);
}

static void init_dragngun(void)
{
	data32_t *ROM = (UINT32 *)memory_region(REGION_CPU1);

	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);

	ROM[0x1b32c/4]=0xe1a00000;//  NOP test switch lock

	install_mem_read32_handler(0, 0x11f15c, 0x11f15f, dragngun_skip);
}

static void init_fghthist(void)
{
	data32_t *ROM = (UINT32 *)memory_region(REGION_CPU1);

//	RAM[0x8d360/4]=RAM[0x8d35c/4];

//	RAM[]=0xe1a00000;  NOP
	ROM[0x10ec/4]=0xe1a00000;

	deco56_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
}

static void init_lockload(void)
{
	data8_t *RAM = memory_region(REGION_CPU1);
//	data32_t *ROM = (UINT32 *)memory_region(REGION_CPU1);

	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);

	memcpy(RAM+0x300000,RAM+0x100000,0x100000);
	memset(RAM+0x100000,0,0x100000);

//	ROM[0x3fe3c0/4]=0xe1a00000;//  NOP test switch lock
//	ROM[0x3fe3cc/4]=0xe1a00000;//  NOP test switch lock
//	ROM[0x3fe40c/4]=0xe1a00000;//  NOP test switch lock
}

/**********************************************************************************/

GAME( 1991, captaven, 0,        captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America (Asia Rev 4)" )
GAME( 1991, captavne, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America (UK Rev 4)" )
GAME( 1991, captavnu, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America (US Rev 1.9)" )
GAME( 1991, captavnj, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America (Japan Rev 2)" )
GAMEX(1993, dragngun, 0,        dragngun, lockload, dragngun, ROT0, "Data East Corporation", "Dragon Gun (US)", GAME_IMPERFECT_GRAPHICS  )
GAMEX(1993, fghthist, 0,        fghthist, fghthist, fghthist, ROT0, "Data East Corporation", "Fighter's History (US)", GAME_UNEMULATED_PROTECTION )
GAMEX(1993, fghthstw, fghthist, fghthist, fghthist, fghthist, ROT0, "Data East Corporation", "Fighter's History (World)", GAME_UNEMULATED_PROTECTION )
GAMEX(1994, lockload, 0,        dragngun, lockload, lockload, ROT0, "Data East Corporation", "Locked 'N Loaded (US)", GAME_IMPERFECT_GRAPHICS  )
GAMEX(1994, tattass,  0,        tattass,  tattass,  0,        ROT0, "Data East Pinball",     "Tattoo Assassins (Prototype)", GAME_IMPERFECT_GRAPHICS )
