/**************************************************************************
 * Mystic Warrior (c) 1993 Konami
 * Metamorphic Force (c) 1993 Konami
 * Violent Storm (c) 1993 Konami
 * Martial Champion (c) 1993 Konami
 * Gaiapolis (c) 1993 Konami
 * Ultimate Battler Dadandarn!! (c) 1993 Konami
 *
 * Driver by R. Belmont, Phil Stroffolino, Acho Tang, and Nicola Salmoria.
 * Assists from Olivier Galibert, Brian Troha, The Guru, and Yasuhiro Ogawa.
 *
 * These games are the "pre-GX" boards, combining features of the previous
 * line of hardware begun with Xexex and those of the future 32-bit System
 * GX (notably 5 bit per pixel graphics, the powerful K055555 mixer/priority
 * encoder, and K054338 alpha blend engine from System GX are used).
 *
 * Game status:
 * - All games are playable with sound and correct colors.
 * - Metamorphic Force's intro needs alpha blended sprites.
 * - Metamorphic Force in-game needs the 53250.  It's never
 *   important for gameplay fortunately, but it's needed for
 *   completeness.
 */

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"

VIDEO_START(gaiapols);
VIDEO_START(dadandarn);
VIDEO_START(viostorm);
VIDEO_START(metamrph);
VIDEO_START(martchmp);
VIDEO_START(mystwarr);
VIDEO_UPDATE(dadandarn);
VIDEO_UPDATE(mystwarr);
VIDEO_UPDATE(metamrph);
VIDEO_UPDATE(martchmp);

WRITE16_HANDLER(ddd_053936_enable_w);
WRITE16_HANDLER(ddd_053936_clip_w);
READ16_HANDLER(gai_053936_tilerom_0_r);
READ16_HANDLER(ddd_053936_tilerom_0_r);
READ16_HANDLER(ddd_053936_tilerom_1_r);
READ16_HANDLER(gai_053936_tilerom_2_r);
READ16_HANDLER(ddd_053936_tilerom_2_r);

static int init_eeprom_count;
static int mw_irq_control;

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

/* Gaiapolis and Polygonet Commanders use the ER5911,
   but the command formats are slightly different.  Why? */
static struct EEPROM_interface eeprom_interface_gaia =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(mystwarr)
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

static NVRAM_HANDLER(gaiapols)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_gaia);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( mweeprom_r )
{
	if (ACCESSING_LSB)
	{
		int res = readinputport(1) | EEPROM_read_bit();

		if (init_eeprom_count)
		{
			init_eeprom_count--;
			res &= ~0x04;
		}

		return res;
	}

	logerror("msb access to eeprom port\n");
	return 0;
}

static READ16_HANDLER( vseeprom_r )
{
	if (ACCESSING_LSB)
	{
		int res = readinputport(1) | EEPROM_read_bit();

		if (init_eeprom_count)
		{
			init_eeprom_count--;
			res &= ~0x08;
		}

		return res;
	}

	logerror("msb access to eeprom port\n");
	return 0;
}

static WRITE16_HANDLER( mweeprom_w )
{
	if (ACCESSING_MSB)
	{
		EEPROM_write_bit((data&0x0100) ? 1 : 0);
		EEPROM_set_cs_line((data&0x0200) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data&0x0400) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

	logerror("unknown LSB write %x to eeprom\n", data);
}

static READ16_HANDLER( dddeeprom_r )
{
	if (ACCESSING_MSB)
	{
		return (readinputport(1) | EEPROM_read_bit())<<8;
	}

	return readinputport(3);
}

static WRITE16_HANDLER( mmeeprom_w )
{
	if (ACCESSING_LSB)
	{
		EEPROM_write_bit((data&0x01) ? 1 : 0);
		EEPROM_set_cs_line((data&0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}
}

static INTERRUPT_GEN(mystwarr_interrupt)
{
	if (!(mw_irq_control & 0x01))
		return;

	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
			break;

		case 1:
			cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
			break;

		case 2:
			cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);
			break;

	}
}

static INTERRUPT_GEN(metamrph_interrupt)
{
	if (!K053246_is_IRQ_enabled())
	{
		return;
	}

	switch (cpu_getiloops())
	{
		case 224:
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
			break;

		case 240:
			cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
			break;

		case 260:
			cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
			break;

	}
}

static INTERRUPT_GEN(mchamp_interrupt) //*
{
	if (!K053246_is_IRQ_enabled())
	{
		return;
	}

	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
			break;

		case 1:
	      		cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);
			break;

	}
}

static INTERRUPT_GEN(ddd_interrupt)
{
	cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
}

static WRITE16_HANDLER( sound_cmd1_w )
{
	soundlatch_w(0, data&0xff);
}

static WRITE16_HANDLER( sound_cmd1_msb_w )
{
	soundlatch_w(0, data>>8);
}

static WRITE16_HANDLER( sound_cmd2_w )
{
	soundlatch2_w(0, data&0xff);
	return;
}

static WRITE16_HANDLER( sound_cmd2_msb_w )
{
	soundlatch2_w(0, data>>8);
	return;
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( sound_status_r )
{
	int latch = soundlatch3_r(0);

	if ((latch & 0xf) == 0xe) latch |= 1;

	return latch;
}

static READ16_HANDLER( sound_status_msb_r )
{
	int latch = soundlatch3_r(0);

	if ((latch & 0xf) == 0xe) latch |= 1;

	return latch<<8;
}

static WRITE16_HANDLER( irq_ack_w )
{
	if (ACCESSING_LSB)
	{
		mw_irq_control = data&0xff;

		if ((data &0xf0) != 0xd0)
			logerror("Unknown write to IRQ reg: %x\n", data);
	}
}

static READ16_HANDLER( player1_r )
{
	return readinputport(2) | (readinputport(3)<<8);
}

static READ16_HANDLER( player2_r )
{
	return readinputport(4) | (readinputport(5)<<8);
}

static READ16_HANDLER( mmplayer1_r )
{
	return readinputport(2) | (readinputport(4)<<8);
}

static READ16_HANDLER( mmplayer2_r )
{
	return readinputport(3) | (readinputport(5)<<8);
}

static READ16_HANDLER( mmcoins_r )
{
	int res = readinputport(0);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= ~0x10;
	}

	return res;
}

static READ16_HANDLER( dddcoins_r )
{
	int res = (readinputport(0)<<8) | readinputport(2);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= ~0x0800;
	}

	return res;
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
	{
//		printf("spr write %x to %x (PC=%x)\n", data, offset, activecpu_get_pc());
		COMBINE_DATA(spriteram16+offset);
	}
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x7f80) >> 4);

		K053247_word_w(offset,data,mem_mask);
	}
}

/* 68000 memory handlers */
/* Mystic Warriors */
static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },		// main program
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x482000, 0x48200f, K055673_rom_word_r },
	{ 0x482010, 0x48201f, MRA16_NOP },		// place holder
	{ 0x494000, 0x494001, player1_r },
	{ 0x494002, 0x494003, player2_r },
	{ 0x496000, 0x496001, mmcoins_r },
	{ 0x496002, 0x496003, mweeprom_r },
	{ 0x498014, 0x498015, sound_status_r },
	{ 0x49c000, 0x49c01f, MRA16_NOP },		// place holder
	{ 0x600000, 0x601fff, K056832_ram_word_r },
	{ 0x602000, 0x603fff, K056832_ram_word_r }, // tilemap RAM mirror read(essential)
	{ 0x680000, 0x683fff, K056832_rom_word_r },
	{ 0x700000, 0x701fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x480000, 0x4800ff, K055555_word_w },
	{ 0x482010, 0x48201f, MWA16_NOP },		// place holder
	{ 0x484000, 0x484007, K053246_word_w },
	{ 0x48a000, 0x48a01f, K054338_word_w },
	{ 0x48c000, 0x48c03f, K056832_word_w },
	{ 0x490000, 0x490001, mweeprom_w },
	{ 0x492000, 0x492001, MWA16_NOP },		// watchdog
	{ 0x49800c, 0x49800d, sound_cmd1_w },
	{ 0x49800e, 0x49800f, sound_cmd2_w },
	{ 0x49a000, 0x49a001, sound_irq_w },
	{ 0x49c000, 0x49c01f, MWA16_NOP },		// place holder
	{ 0x49e006, 0x49e007, irq_ack_w },
	{ 0x600000, 0x601fff, K056832_ram_word_w },
	{ 0x602000, 0x603fff, K056832_ram_word_w }, // tilemap RAM mirror write(essential)
	{ 0x700000, 0x701fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

/* Metamorphic Force */
static MEMORY_READ16_START( mmreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },		// main program
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_r },
	{ 0x211000, 0x21ffff, MRA16_RAM },
	{ 0x244000, 0x24400f, K055673_rom_word_r },
 	{ 0x24c000, 0x24ffff, K053250_0_ram_r },
	{ 0x250000, 0x25000f, K053250_0_r },
	{ 0x268014, 0x268015, sound_status_r },
	{ 0x274000, 0x274001, mmplayer1_r },
	{ 0x274002, 0x274003, mmplayer2_r },
	{ 0x278000, 0x278001, mmcoins_r },
	{ 0x278002, 0x278003, vseeprom_r },
	{ 0x27c000, 0x27c001, MRA16_NOP },		// watchdog lives here
	{ 0x300000, 0x301fff, K056832_ram_word_r },
	{ 0x302000, 0x303fff, K056832_ram_word_r }, // tilemap RAM mirror read(essential)
	{ 0x310000, 0x311fff, K056832_rom_word_r },
	{ 0x320000, 0x321fff, K053250_0_rom_r },
	{ 0x330000, 0x331fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( mmwritemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_w },
	{ 0x211000, 0x21ffff, MWA16_RAM },
	{ 0x240000, 0x240007, K053246_word_w },
 	{ 0x24c000, 0x24ffff, K053250_0_ram_w },// "LVC RAM" (53250_ram)
	{ 0x250000, 0x25000f, K053250_0_w },
	{ 0x254000, 0x25401f, K054338_word_w },
	{ 0x258000, 0x2580ff, K055555_word_w },
	{ 0x26001c, 0x26001d, MWA16_NOP },		// IRQ ack? for vbl
	{ 0x264000, 0x264001, sound_irq_w },
	{ 0x26800c, 0x26800d, sound_cmd1_w },
	{ 0x26800e, 0x26800f, sound_cmd2_w },
	{ 0x26C000, 0x26C007, K056832_b_word_w },
	{ 0x270000, 0x27003f, K056832_word_w },
	{ 0x27C000, 0x27C001, mmeeprom_w },
	{ 0x300000, 0x301fff, K056832_ram_word_w },
	{ 0x302000, 0x303fff, K056832_ram_word_w }, // tilemap RAM mirror write(essential)
	{ 0x330000, 0x331fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

/* Violent Storm */
static MEMORY_READ16_START( vsreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },		// main program
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_r },
	{ 0x211000, 0x21ffff, MRA16_RAM },
	{ 0x244000, 0x24400f, K055673_rom_word_r },
 	{ 0x24c000, 0x24ffff, MRA16_RAM },
	{ 0x250000, 0x25000f, K053250_0_r },
	{ 0x25c000, 0x25c03f, K055550_word_r },
	{ 0x268014, 0x268015, sound_status_r },
	{ 0x274000, 0x274001, mmplayer1_r },
	{ 0x274002, 0x274003, mmplayer2_r },
	{ 0x278000, 0x278001, mmcoins_r },
	{ 0x278002, 0x278003, vseeprom_r },
	{ 0x27c000, 0x27c001, MRA16_NOP },		// watchdog lives here
	{ 0x300000, 0x301fff, K056832_ram_word_r },
	{ 0x302000, 0x303fff, K056832_ram_word_r }, // tilemap RAM mirror read(essential)
	{ 0x310000, 0x311fff, K056832_rom_word_r },
	{ 0x330000, 0x331fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( vswritemem )
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_w },
	{ 0x211000, 0x21ffff, MWA16_RAM },
	{ 0x240000, 0x240007, K053246_word_w },
 	{ 0x24c000, 0x24ffff, MWA16_RAM },
	{ 0x250000, 0x25000f, K053250_0_w },
	{ 0x254000, 0x25401f, K054338_word_w },
	{ 0x258000, 0x2580ff, K055555_word_w },
	{ 0x25c000, 0x25c03f, K055550_word_w },
	{ 0x26001c, 0x26001d, MWA16_NOP },		// IRQ ack? for vbl
	{ 0x264000, 0x264001, sound_irq_w },
	{ 0x26800c, 0x26800d, sound_cmd1_w },
	{ 0x26800e, 0x26800f, sound_cmd2_w },
	{ 0x26C000, 0x26C007, K056832_b_word_w },
	{ 0x270000, 0x27003f, K056832_word_w },
	{ 0x27C000, 0x27C001, mmeeprom_w },
	{ 0x300000, 0x301fff, K056832_ram_word_w },
	{ 0x302000, 0x303fff, K056832_ram_word_w }, // tilemap RAM mirror write(essential)
	{ 0x330000, 0x331fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

//* Martial Champion specific K053247 interface and K053990 protection
static READ16_HANDLER( K053247_martchmp_word_r )
{
	if (offset & 0x0018)
		return spriteram16[offset];
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x1fe0) >> 2);
		return K053247_word_r(offset,mem_mask);
	}
}

static WRITE16_HANDLER( K053247_martchmp_word_w )
{
	if (offset & 0x0018)
	{
		COMBINE_DATA(spriteram16+offset);
	}
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x1fe0) >> 2);

		K053247_word_w(offset,data,mem_mask);
	}
}

WRITE16_HANDLER( K053990_martchmp_word_w )
{
	static UINT16 mcprot_data[0x20];
	int src_addr, src_count, src_skip;
	int dst_addr, dst_count, dst_skip;
	int mod_addr, mod_count, mod_skip, mod_offs;
	int mode, i, element_size = 1;
	data16_t mod_val, mod_data;

	COMBINE_DATA(mcprot_data+offset);

	if (offset == 0x0c && ACCESSING_MSB)
	{
		mode  = (mcprot_data[0x0d]<<8 & 0xff00) | (mcprot_data[0x0f] & 0xff);

		switch (mode)
		{
			case 0xffff: // word copy
				element_size = 2;
			case 0xff00: // byte copy
				src_addr  = mcprot_data[0x0];
				src_addr |= mcprot_data[0x1]<<16 & 0xff0000;
				dst_addr  = mcprot_data[0x2];
				dst_addr |= mcprot_data[0x3]<<16 & 0xff0000;
				src_count = mcprot_data[0x8]>>8;
				src_skip  = mcprot_data[0x8] & 0xff;
				dst_count = mcprot_data[0x9]>>8;
				dst_skip  = mcprot_data[0x9] & 0xff;

				src_addr += src_skip;
				src_skip += element_size;
				dst_addr += dst_skip;
				dst_skip += element_size;

				if (element_size == 1)
				for (i=src_count; i; i--)
				{
					cpu_writemem24bew(dst_addr, cpu_readmem24bew(src_addr));
					src_addr += src_skip;
					dst_addr += dst_skip;
				}
				else
				for (i=src_count; i; i--)
				{
					cpu_writemem24bew_word(dst_addr, cpu_readmem24bew_word(src_addr));
					src_addr += src_skip;
					dst_addr += dst_skip;
				}
			break;

			case 0x00ff: // sprite list modifier
				src_addr  = mcprot_data[0x0];
				src_addr |= mcprot_data[0x1]<<16 & 0xff0000;
				src_skip  = mcprot_data[0x1]>>8;
				dst_addr  = mcprot_data[0x2];
				dst_addr |= mcprot_data[0x3]<<16 & 0xff0000;
				dst_skip  = mcprot_data[0x3]>>8;
				mod_addr  = mcprot_data[0x4];
				mod_addr |= mcprot_data[0x5]<<16 & 0xff0000;
				mod_skip  = mcprot_data[0x5]>>8;
				mod_offs  = mcprot_data[0x8] & 0xff;
				mod_offs<<= 1;
				mod_count = 0x100;

				src_addr += mod_offs;
				dst_addr += mod_offs;

				for (i=mod_count; i; i--)
				{
					mod_val  = cpu_readmem24bew_word(mod_addr);
					mod_addr += mod_skip;

					mod_data = cpu_readmem24bew_word(src_addr);
					src_addr += src_skip;

					mod_data += mod_val;

					cpu_writemem24bew_word(dst_addr, mod_data);
					dst_addr += dst_skip;
				}
			break;

			default:
			break;
		}
	}
}

/* Martial Champion */
static MEMORY_READ16_START( mcreadmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },		// main program
	{ 0x100000, 0x10ffff, MRA16_RAM },		// work RAM
	{ 0x300000, 0x3fffff, MRA16_ROM },		// data ROM
	{ 0x402000, 0x40200f, K055673_rom_word_r },	// sprite ROM readback
	{ 0x412000, 0x412001, MRA16_NOP },		// watchdog
	{ 0x414000, 0x414001, player1_r },
	{ 0x414002, 0x414003, player2_r },
	{ 0x416000, 0x416001, mmcoins_r },              // coin
	{ 0x416002, 0x416003, mweeprom_r },		// eeprom read
	{ 0x418014, 0x418015, sound_status_r },		// z80 status
	{ 0x480000, 0x483fff, K053247_martchmp_word_r },// sprite RAM
	{ 0x600000, 0x601fff, MRA16_RAM },		// palette RAM
	{ 0x680000, 0x681fff, K056832_ram_word_r },	// tilemap RAM
	{ 0x682000, 0x683fff, K056832_ram_word_r },	// tilemap RAM mirror read(essential)
	{ 0x700000, 0x703fff, K056832_rom_word_r },	// tile ROM readback
MEMORY_END

static MEMORY_WRITE16_START( mcwritemem )
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x400000, 0x4000ff, K055555_word_w },
	{ 0x402000, 0x40201f, K053247_reg_word_w }, // OBJSET2
	{ 0x404000, 0x404007, K053246_word_w },
	{ 0x40a000, 0x40a01f, K054338_word_w },
	{ 0x40c000, 0x40c03f, K056832_word_w },
	{ 0x40e000, 0x40e03f, K053990_martchmp_word_w },
	{ 0x410000, 0x410001, mweeprom_w },
	{ 0x412000, 0x412001, MWA16_NOP },	   	// watchdog
	{ 0x41c000, 0x41c01f, K053252_word_w },		// CCU regs
	{ 0x41e000, 0x41e007, K056832_b_word_w },
	{ 0x41800c, 0x41800d, sound_cmd1_w },
	{ 0x41800e, 0x41800f, sound_cmd2_w },
	{ 0x41a000, 0x41a001, sound_irq_w },
	{ 0x480000, 0x483fff, K053247_martchmp_word_w, &spriteram16 },
	{ 0x600000, 0x601fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x680000, 0x681fff, K056832_ram_word_w },
	{ 0x682000, 0x683fff, K056832_ram_word_w },	// tilemap RAM mirror write(essential)
MEMORY_END

/* Ultimate Battler Dadandarn */
static MEMORY_READ16_START( dddreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },	// main program and data ROM
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x410000, 0x411fff, K056832_ram_word_r },	// tilemap RAM
	{ 0x412000, 0x413fff, K056832_ram_word_r }, // tilemap RAM mirror read(essential)
	{ 0x420000, 0x421fff, MRA16_RAM },
	{ 0x430000, 0x430007, K053246_word_r },
	{ 0x440000, 0x443fff, K056832_rom_word_r },
	{ 0x450000, 0x45000f, K055673_rom_word_r },
	{ 0x470000, 0x470fff, MRA16_RAM },
	{ 0x48a014, 0x48a015, sound_status_msb_r },
	{ 0x48e000, 0x48e001, dddcoins_r },	// bit 3 (0x8) is test switch
	{ 0x48e020, 0x48e021, dddeeprom_r },
	{ 0x600000, 0x60ffff, MRA16_RAM },
	{ 0x680000, 0x68003f, K055550_word_r },
	{ 0x800000, 0x87ffff, ddd_053936_tilerom_0_r },	// 256k tilemap readback
	{ 0xa00000, 0xa7ffff, ddd_053936_tilerom_1_r }, // 128k tilemap readback
	{ 0xc00000, 0xdfffff, ddd_053936_tilerom_2_r },	// tile character readback
MEMORY_END

static MEMORY_WRITE16_START( dddwritemem )
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x410000, 0x411fff, K056832_ram_word_w },
	{ 0x412000, 0x413fff, K056832_ram_word_w }, // tile RAM mirror write(essential)
	{ 0x420000, 0x421fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x430000, 0x430007, K053246_word_w },
	{ 0x460000, 0x46001f, MWA16_RAM, &K053936_0_ctrl },
	{ 0x470000, 0x470fff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x480000, 0x48003f, K056832_word_w },	// 832 registers
	{ 0x482000, 0x482007, K056832_b_word_w },
	{ 0x484000, 0x484003, ddd_053936_clip_w },
	{ 0x486000, 0x486007, K053246_word_w },
	{ 0x48601c, 0x48601d, MWA16_NOP },
	{ 0x488000, 0x4880ff, K055555_word_w },
	{ 0x48a00c, 0x48a00d, sound_cmd1_msb_w },
	{ 0x48a00e, 0x48a00f, sound_cmd2_msb_w },
	{ 0x48c000, 0x48c01f, K054338_word_w },
	{ 0x600000, 0x60ffff, MWA16_RAM },
	{ 0x680000, 0x68003f, K055550_word_w },
	{ 0x6a0000, 0x6a0001, mmeeprom_w },
	{ 0x6c0000, 0x6c0001, ddd_053936_enable_w },
	{ 0x6e0000, 0x6e0001, sound_irq_w },
	{ 0xe00000, 0xe00001, MWA16_NOP }, 	// watchdog
MEMORY_END

/* Gaiapolis */
// a00000 = the 128k tilemap
// 800000 = the 256k tilemap
// c00000 = 936 tiles (7fffff window)
static MEMORY_READ16_START( gaiareadmem )
	{ 0x000000, 0x2fffff, MRA16_ROM },	// main program
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x410000, 0x411fff, K056832_ram_word_r },	// tilemap RAM
	{ 0x412000, 0x413fff, K056832_ram_word_r }, // tilemap RAM mirror read(essential)
	{ 0x420000, 0x421fff, MRA16_RAM },
	{ 0x430000, 0x430007, K053246_word_r },
	{ 0x440000, 0x441fff, K056832_rom_word_r },
	{ 0x450000, 0x45000f, K055673_rom_word_r },
	{ 0x470000, 0x470fff, MRA16_RAM },
	{ 0x48a014, 0x48a015, sound_status_msb_r },
	{ 0x48e000, 0x48e001, dddcoins_r },	// bit 3 (0x8) is test switch
	{ 0x48e020, 0x48e021, dddeeprom_r },
	{ 0x600000, 0x60ffff, MRA16_RAM },
	{ 0x660000, 0x6600ff, K054000_lsb_r },
	{ 0x800000, 0x87ffff, gai_053936_tilerom_0_r },	// 256k tilemap readback
	{ 0xa00000, 0xa7ffff, ddd_053936_tilerom_1_r }, // 128k tilemap readback
	{ 0xc00000, 0xdfffff, gai_053936_tilerom_2_r },	// tile character readback
MEMORY_END

static MEMORY_WRITE16_START( gaiawritemem )
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x410000, 0x411fff, K056832_ram_word_w },
	{ 0x412000, 0x413fff, K056832_ram_word_w }, // tilemap RAM mirror write(essential)
	{ 0x412000, 0x4120ff, MWA16_RAM },
	{ 0x420000, 0x421fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x430000, 0x430007, K053246_word_w },
	{ 0x450010, 0x45001f, MWA16_NOP },
	{ 0x460000, 0x46001f, MWA16_RAM, &K053936_0_ctrl  },
	{ 0x470000, 0x470fff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x480000, 0x48003f, K056832_word_w },	// 832 registers
	{ 0x482000, 0x482007, K056832_b_word_w },
	{ 0x484000, 0x484003, ddd_053936_clip_w },
	{ 0x486000, 0x486007, K053246_word_w },
	{ 0x48601c, 0x48601d, MWA16_NOP },
	{ 0x488000, 0x4880ff, K055555_word_w },
	{ 0x48a00c, 0x48a00d, sound_cmd1_msb_w },
	{ 0x48a00e, 0x48a00f, sound_cmd2_msb_w },
	{ 0x48c000, 0x48c01f, K054338_word_w },
	{ 0x600000, 0x60ffff, MWA16_RAM },
	{ 0x660000, 0x6600ff, K054000_lsb_w },
	{ 0x6a0000, 0x6a0001, mmeeprom_w },
	{ 0x6c0000, 0x6c0001, ddd_053936_enable_w },
	{ 0x6e0000, 0x6e0001, sound_irq_w },
	{ 0xe00000, 0xe00001, MWA16_NOP }, 	// watchdog
MEMORY_END

/**********************************************************************************/

static int cur_sound_region;

static void reset_sound_region(void)
{
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + cur_sound_region*0x4000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = (data & 0xf);
	reset_sound_region();
}

static INTERRUPT_GEN(audio_interrupt)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

/* sound memory maps

   there are 2 sound boards: the martial champion single-'539 version
   and the dual-'539 version used by run and gun, violent storm, monster maulers,
   gaiapolous, metamorphic force, and mystic warriors.  Their memory maps are
   quite similar to xexex/gijoe/asterix's sound.
 */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe230, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe62f, K054539_1_r },
	{ 0xe630, 0xe7ff, MRA_RAM },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_NOP },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, sound_bankswitch_w },
	{ 0xfff1, 0xfff3, MWA_NOP },
MEMORY_END

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};

static struct K054539interface k054539_interface_mystwarr =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 20, 20 } },
	{ NULL }
};

/**********************************************************************************/

static struct GfxLayout bglayout_4bpp =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*64
};

static struct GfxLayout bglayout_8bpp =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*128
};

static struct GfxDecodeInfo gfxdecodeinfo_gaiapols[] =
{
	{ REGION_GFX3, 0, &bglayout_4bpp, 0x0000, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_ultbttlr[] =
{
	{ REGION_GFX3, 0, &bglayout_8bpp, 0x0000, 8 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( mystwarr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)	/* 16 MHz (confirmed) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(mystwarr_interrupt,3)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(audio_interrupt, 480)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(mystwarr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(25, 312, 16, 240-1 )	
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(mystwarr)
	MDRV_VIDEO_UPDATE(mystwarr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("539", K054539, k054539_interface_mystwarr)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( viostorm )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(vsreadmem,vswritemem)
	MDRV_CPU_VBLANK_INT(metamrph_interrupt,262)

	/* video hardware */
	MDRV_VIDEO_START(viostorm)
	MDRV_VIDEO_UPDATE(metamrph)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(39, 39+384-1, 16, 16+224-1 )

	MDRV_SOUND_REPLACE("539", K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( metamrph )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mmreadmem,mmwritemem)
	MDRV_CPU_VBLANK_INT(metamrph_interrupt,262)

	MDRV_VIDEO_START(metamrph)
	MDRV_VIDEO_UPDATE(metamrph)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(16+6, 16+294-1, 18, 18+224-1 )

	MDRV_SOUND_REPLACE("539", K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ultbttlr )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(dddreadmem, dddwritemem)
	MDRV_CPU_VBLANK_INT(ddd_interrupt,1)

	MDRV_GFXDECODE(gfxdecodeinfo_ultbttlr)

	MDRV_VIDEO_START(dadandarn)
	MDRV_VIDEO_UPDATE(dadandarn)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(24, 24+288-1, 17, 17+224-1)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)

	MDRV_SOUND_REPLACE("539", K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( martchmp ) //*
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mcreadmem,mcwritemem)
	MDRV_CPU_VBLANK_INT(mchamp_interrupt,2)

	MDRV_VIDEO_START(martchmp)
	MDRV_VIDEO_UPDATE(martchmp)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(34, 34+384-1, 16, 16+224-1)

	MDRV_SOUND_REPLACE("539", K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gaiapols )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(gaiareadmem,gaiawritemem)
	MDRV_CPU_VBLANK_INT(ddd_interrupt,1)

	MDRV_GFXDECODE(gfxdecodeinfo_gaiapols)

	MDRV_NVRAM_HANDLER(gaiapols)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_VIDEO_START(gaiapols)
	MDRV_VIDEO_UPDATE(dadandarn)
	MDRV_SCREEN_SIZE(64*8, 32*8)
			
	MDRV_VISIBLE_AREA(35, 35+380-1 -1, 16, 16+224-1)

	MDRV_SOUND_REPLACE("539", K054539, k054539_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

INPUT_PORTS_START( mystwarr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* game loops if this is set */
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x40, 0x40, "Number of Players")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPSETTING(    0x40, "2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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

INPUT_PORTS_START( metamrph )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x40, 0x40, "Number of Players")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPSETTING(    0x40, "2")
	PORT_DIPNAME( 0x80, 0x80, "Continuous Energy Increment")
	PORT_DIPSETTING(    0x80, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))

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

INPUT_PORTS_START( viostorm )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Coin Mechanism")
	PORT_DIPSETTING(    0x40, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x80, 0x80, "Number of Players")
	PORT_DIPSETTING(    0x00, "3")
	PORT_DIPSETTING(    0x80, "2")

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

INPUT_PORTS_START( dadandarn )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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

INPUT_PORTS_START( martchmp )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* game loops if this is set */
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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

/**********************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

#define ROM_LOADTILE_WORD(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(3) | ROM_REVERSE)
#define ROM_LOADTILE_BYTE(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPBYTE | ROM_SKIP(4))

ROM_START( mystwarr )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "128eaa01.20f", 0x000000, 0x40000, 0x508f249c )
	ROM_LOAD16_BYTE( "128eaa02.20g", 0x000001, 0x40000, 0xf8ffa352 )
	ROM_LOAD16_BYTE( "128a03.19f", 0x100000, 0x80000, 0xe98094f3 )
	ROM_LOAD16_BYTE( "128a04.19g", 0x100001, 0x80000, 0x88c6a3e4 )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("128a05.6b", 0x000000, 0x020000, 0x0e5194e0 )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "128a08.1h", 0x000000, 1*1024*1024, 0x63d6cfa0 )
	ROM_LOADTILE_WORD( "128a09.1k", 0x000002, 1*1024*1024, 0x573a7725 )
	ROM_LOADTILE_BYTE( "128a10.3h", 0x000004, 512*1024, 0x558e545a )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "128a16.22k", 0x000000, 1*1024*1024, 0x459b6407 )
	ROM_LOAD64_WORD( "128a15.20k", 0x000002, 1*1024*1024, 0x6bbfedf4 )
	ROM_LOAD64_WORD( "128a14.19k", 0x000004, 1*1024*1024, 0xf7bd89dd )
	ROM_LOAD64_WORD( "128a13.17k", 0x000006, 1*1024*1024, 0xe89b66a2 )
	ROM_LOAD16_BYTE( "128a12.12k", 0x400000, 512*1024, 0x63de93e2 )
	ROM_LOAD16_BYTE( "128a11.10k", 0x400001, 512*1024, 0x4eac941a )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "128a06.2d", 0x000000, 2*1024*1024, 0x88ed598c )
	ROM_LOAD( "128a07.1d", 0x200000, 2*1024*1024, 0xdb79a66e )
ROM_END

ROM_START( mystwaru )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "128uaa01.20f", 0x000000, 0x40000, 0x3a89aafd )
	ROM_LOAD16_BYTE( "128uaa02.20g", 0x000001, 0x40000, 0xde07410f )
	ROM_LOAD16_BYTE( "128a03.19f", 0x100000, 0x80000, 0xe98094f3 )
	ROM_LOAD16_BYTE( "128a04.19g", 0x100001, 0x80000, 0x88c6a3e4 )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("128a05.6b", 0x000000, 0x020000, 0x0e5194e0 )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "128a08.1h", 0x000000, 1*1024*1024, 0x63d6cfa0 )
	ROM_LOADTILE_WORD( "128a09.1k", 0x000002, 1*1024*1024, 0x573a7725 )
	ROM_LOADTILE_BYTE( "128a10.3h", 0x000004, 512*1024, 0x558e545a )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "128a16.22k", 0x000000, 1*1024*1024, 0x459b6407 )
	ROM_LOAD64_WORD( "128a15.20k", 0x000002, 1*1024*1024, 0x6bbfedf4 )
	ROM_LOAD64_WORD( "128a14.19k", 0x000004, 1*1024*1024, 0xf7bd89dd )
	ROM_LOAD64_WORD( "128a13.17k", 0x000006, 1*1024*1024, 0xe89b66a2 )
	ROM_LOAD16_BYTE( "128a12.12k", 0x400000, 512*1024, 0x63de93e2 )
	ROM_LOAD16_BYTE( "128a11.10k", 0x400001, 512*1024, 0x4eac941a )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "128a06.2d", 0x000000, 2*1024*1024, 0x88ed598c )
	ROM_LOAD( "128a07.1d", 0x200000, 2*1024*1024, 0xdb79a66e )
ROM_END


ROM_START( viostorm )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "168uab01.15h", 0x000001, 0x80000, 0x2d6a9fa3 )
	ROM_LOAD16_BYTE( "168uab02.15f", 0x000000, 0x80000, 0x0e75f7cc )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, 0x507fb3eb )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, 0x1b34a881 )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, 0xdb0ce743 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, 0xbd2bbdea )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, 0x7a57c9e7 )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, 0xb6b1c4ef )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, 0xcdec3650 )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, 0x25404fd7 )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, 0xfdbbf8cc )
ROM_END

ROM_START( viostrma )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "168aac01.15h", 0x000001, 0x80000, 0x3620635c )
	ROM_LOAD16_BYTE( "168aac02.15f", 0x000000, 0x80000, 0xdb679aec )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, 0x507fb3eb )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, 0x1b34a881 )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, 0xdb0ce743 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, 0xbd2bbdea )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, 0x7a57c9e7 )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, 0xb6b1c4ef )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, 0xcdec3650 )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, 0x25404fd7 )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, 0xfdbbf8cc )
ROM_END

ROM_START( viostrmj )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "168jac01.b01", 0x000001, 0x80000, 0xf8be1225 )
	ROM_LOAD16_BYTE( "168jac02.b02", 0x000000, 0x80000, 0xf42fd1e5 )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, 0x507fb3eb )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, 0x1b34a881 )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, 0xdb0ce743 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, 0xbd2bbdea )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, 0x7a57c9e7 )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, 0xb6b1c4ef )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, 0xcdec3650 )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, 0x25404fd7 )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, 0xfdbbf8cc )
ROM_END

ROM_START( metamrph )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "224a01", 0x000001, 0x40000, 0xe1d9b516 )
	ROM_LOAD16_BYTE( "224a02", 0x000000, 0x40000, 0x289c926b )
	ROM_LOAD16_BYTE( "224a03", 0x100001, 0x80000, 0xa5bedb01 )
	ROM_LOAD16_BYTE( "224a04", 0x100000, 0x80000, 0xada53ba4 )

	/* sound program */
	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	ROM_LOAD("224a05", 0x000000, 0x040000, 0x4b4c985c )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "224a09", 0x000000, 1*1024*1024, 0x1931afce )
	ROM_LOADTILE_WORD( "224a08", 0x000002, 1*1024*1024, 0xdc94d53a )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "224a10", 0x000000, 2*1024*1024, 0x161287f0 )
	ROM_LOAD64_WORD( "224a11", 0x000002, 2*1024*1024, 0xdf5960e1 )
	ROM_LOAD64_WORD( "224a12", 0x000004, 2*1024*1024, 0xca72a4b3 )
	ROM_LOAD64_WORD( "224a13", 0x000006, 2*1024*1024, 0x86b58feb )

	/* K053250 linescroll/zoom thingy */
	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "224a14", 0x000000, 256*1024, 0x3c79b404 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "224a06", 0x000000, 2*1024*1024, 0x972f6abe )
	ROM_LOAD( "224a07", 0x200000, 1*1024*1024, 0x61b2f97a )
ROM_END

ROM_START( mtlchmpj ) //*
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "234jaa01.20f", 0x000000, 0x40000, 0x76c3c568 )
	ROM_LOAD16_BYTE( "234jaa02.20g", 0x000001, 0x40000, 0x95eec0aa )
	ROM_LOAD16_BYTE( "234_d03.19f", 0x300000, 0x80000, 0xabb577c6 )
	ROM_LOAD16_BYTE( "234_d04.19g", 0x300001, 0x80000, 0x030a1925 )

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("234_d05.6b", 0x000000, 0x020000, 0xefb6bcaa )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "234a08.1h", 0x000000, 1*1024*1024, 0x27e94288 )
	ROM_LOADTILE_WORD( "234a09.1k", 0x000002, 1*1024*1024, 0x03aad28f )
	ROM_LOADTILE_BYTE( "234a10.3h", 0x000004, 512*1024, 0x51f50fe2 )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "234a16.22k", 0x000000, 2*1024*1024, 0x14d909a5 )
	ROM_LOAD64_WORD( "234a15.20k", 0x000002, 2*1024*1024, 0xa5028418 )
	ROM_LOAD64_WORD( "234a14.19k", 0x000004, 2*1024*1024, 0xd7921f47 )
	ROM_LOAD64_WORD( "234a13.17k", 0x000006, 2*1024*1024, 0x5974392e )
	ROM_LOAD16_BYTE( "234a12.12k", 0x800000, 1024*1024, 0xc7f2b099 )
	ROM_LOAD16_BYTE( "234a11.10k", 0x800001, 1024*1024, 0x82923713 )

	/* K053250 road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "234a06.2d", 0x000000, 2*1024*1024, 0x12d32384 )
	ROM_LOAD( "234a07.1d", 0x200000, 2*1024*1024, 0x05ee239f )
ROM_END

ROM_START( gaiapols )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "123e07.24m", 0x000000, 1*1024*1024, 0xf1a1db0f )
	ROM_LOAD16_BYTE( "123e09.19l", 0x000001, 1*1024*1024, 0x4b3b57e7 )

	/* 68k data */
	ROM_LOAD16_BYTE( "123jaf11.19p", 0x200000, 256*1024, 0x19919571 )
	ROM_LOAD16_BYTE( "123jaf12.17p", 0x200001, 256*1024, 0x4246e595 )

	/* sound program */
	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	ROM_LOAD("123e13.9c", 0x000000, 0x040000, 0xe772f822 )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "123e16.2t", 0x000000, 1*1024*1024, 0xa3238200 )
	ROM_LOADTILE_WORD( "123e17.2x", 0x000002, 1*1024*1024, 0xbd0b9fb9 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "123e19.34u", 0x000000, 2*1024*1024, 0x219a7c26 )
	ROM_LOAD64_WORD( "123e21.34y", 0x000002, 2*1024*1024, 0x1888947b )
	ROM_LOAD64_WORD( "123e18.36u", 0x000004, 2*1024*1024, 0x3719b6d4 )
	ROM_LOAD64_WORD( "123e20.36y", 0x000006, 2*1024*1024, 0x490a6f64 )

	/* K053536 roz tiles */
	ROM_REGION( 0x180000, REGION_GFX3, 0)
	ROM_LOAD( "123e04.32n", 0x000000, 0x080000, 0x0d4d5b8b )
	ROM_LOAD( "123e05.29n", 0x080000, 0x080000, 0x7d123f3e )
	ROM_LOAD( "123e06.26n", 0x100000, 0x080000, 0xfa50121e )

	/* K053936 map data */
	ROM_REGION( 0xa0000, REGION_GFX4, 0)
	ROM_LOAD( "123e01.36j", 0x000000, 0x20000, 0x9dbc9678 )
	ROM_LOAD( "123e02.34j", 0x020000, 0x40000, 0xb8e3f500 )
	ROM_LOAD( "123e03.36m", 0x060000, 0x40000, 0xfde4749f )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "123e14.2g", 0x000000, 2*1024*1024, 0x65dfd3ff )
	ROM_LOAD( "123e15.2m", 0x200000, 2*1024*1024, 0x7017ff07 )
ROM_END

ROM_START( dadandrn )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "170a07.24m", 0x000000, 0x80000, 0x6a55e828 )
	ROM_LOAD16_BYTE( "170a09.19l", 0x000001, 0x80000, 0x9e821cd8 )
	ROM_LOAD16_BYTE( "170a08.21m", 0x100000, 0x40000, 0x03c59ba2 )
	ROM_LOAD16_BYTE( "170a10.17l", 0x100001, 0x40000, 0x8a340909 )

	/* sound program */
	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD("170a13.9c", 0x000000, 0x40000, 0x2ebf4d1c)
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "170a16.2t", 0x000000, 1*1024*1024, 0x41fee912 )
	ROM_LOADTILE_WORD( "170a17.2x", 0x000002, 1*1024*1024, 0x96957c91 )
	ROM_LOADTILE_BYTE( "170a24.5r", 0x000004, 512*1024, 0x562ad4bd )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "170a19.34u", 0x000000, 2*1024*1024, 0xbe835141 )
	ROM_LOAD64_WORD( "170a21.34y", 0x000002, 2*1024*1024, 0xbcb68136 )
	ROM_LOAD64_WORD( "170a18.36u", 0x000004, 2*1024*1024, 0xe1e3c8d2 )
	ROM_LOAD64_WORD( "170a20.36y", 0x000006, 2*1024*1024, 0xccb4d88c )
	ROM_LOAD16_BYTE( "170a23.29y", 0x800000, 1024*1024, 0x6b5390e4 )
	ROM_LOAD16_BYTE( "170a22.32y", 0x800001, 1024*1024, 0x21628106 )

	/* K053536 roz plane */
	ROM_REGION( 0x180000, REGION_GFX3, 0)
	ROM_LOAD( "170a04.33n", 0x000000, 0x80000, 0x64b9a73b )
	ROM_LOAD( "170a05.30n", 0x080000, 0x80000, 0xf2c101d0 )
	ROM_LOAD( "170a06.27n", 0x100000, 0x80000, 0xb032e59b )

	/* K053936 tilemap data */
	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_ERASE00)
	ROM_LOAD( "170a02.34j", 0x000000, 0x40000, 0xb040cebf )
	ROM_LOAD( "170a03.36m", 0x040000, 0x40000, 0x7fb412b2 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD("170a14.2g", 0x000000, 2*1024*1024, 0x83317cda)
	ROM_LOAD("170a15.2m", 0x200000, 2*1024*1024, 0xd4113ae9)
ROM_END

static DRIVER_INIT(mystwarr)
{
	unsigned char *s = memory_region(REGION_GFX1);
	unsigned char *pFinish = s+memory_region_length(REGION_GFX1)-3;

	while( s<pFinish )
	{
		/* convert the whole mess to 5bpp planar in System GX's format
		   (p3 p1 p2 p0 p5)
		   (the original ROMs are stored as chunky for the first 4 bits
		   and the 5th bit is planar, which is undecodable as-is) */
		int d0 = ((s[0]&0x80)   )|((s[0]&0x08)<<3)|((s[1]&0x80)>>2)|((s[1]&0x08)<<1)|
		         ((s[2]&0x80)>>4)|((s[2]&0x08)>>1)|((s[3]&0x80)>>6)|((s[3]&0x08)>>3);
		int d1 = ((s[0]&0x40)<<1)|((s[0]&0x04)<<4)|((s[1]&0x40)>>1)|((s[1]&0x04)<<2)|
		         ((s[2]&0x40)>>3)|((s[2]&0x04)   )|((s[3]&0x40)>>5)|((s[3]&0x04)>>2);
		int d2 = ((s[0]&0x20)<<2)|((s[0]&0x02)<<5)|((s[1]&0x20)   )|((s[1]&0x02)<<3)|
		         ((s[2]&0x20)>>2)|((s[2]&0x02)<<1)|((s[3]&0x20)>>4)|((s[3]&0x02)>>1);
		int d3 = ((s[0]&0x10)<<3)|((s[0]&0x01)<<6)|((s[1]&0x10)<<1)|((s[1]&0x01)<<4)|
		         ((s[2]&0x10)>>1)|((s[2]&0x01)<<2)|((s[3]&0x10)>>3)|((s[3]&0x01)   );

		s[0] = d3;
		s[1] = d1;
		s[2] = d2;
		s[3] = d0;

		s += 5;
	}

	/* set default bankswitch */
	cur_sound_region = 2;
	reset_sound_region();

	mw_irq_control = 0;

	state_save_register_int("Mystwarr", 0, "IRQ control", &mw_irq_control);
	state_save_register_int("Mystwarr", 0, "sound region", &cur_sound_region);
	state_save_register_func_postload(reset_sound_region);
}

/*           ROM       parent    machine   inp        init */
GAMEX( 1993, mystwarr, 0,        mystwarr, mystwarr,  mystwarr, ROT0,  "Konami", "Mystic Warriors (World ver EAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, mystwaru, mystwarr, mystwarr, mystwarr,  mystwarr, ROT0,  "Konami", "Mystic Warriors (US ver UAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, dadandrn, 0,        ultbttlr, dadandarn, mystwarr, ROT0,  "Konami", "Kyukyoku Sentai Dadandarn (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, viostorm, 0,        viostorm, viostorm,  mystwarr, ROT0,  "Konami", "Violent Storm (US ver UAB)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, viostrmj, viostorm, viostorm, viostorm,  mystwarr, ROT0,  "Konami", "Violent Storm (Japan ver JAC)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, viostrma, viostorm, viostorm, viostorm,  mystwarr, ROT0,  "Konami", "Violent Storm (Asia ver AAC)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, metamrph, 0,        metamrph, metamrph,  mystwarr, ROT0,  "Konami", "Metamorphic Force (US ver UAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, mtlchmpj, 0,        martchmp, martchmp,  mystwarr, ROT0,  "Konami", "Martial Champion (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, gaiapols, 0,        gaiapols, dadandarn, mystwarr, ROT90, "Konami", "Gaiapolis (Japan ver JAF)", GAME_IMPERFECT_GRAPHICS )
