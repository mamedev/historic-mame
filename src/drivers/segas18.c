/***************************************************************************

	Sega System 18 hardware

****************************************************************************

	Known bugs:
		* i8751 needs to be hooked up in mwalk
		* input ports need serious fixing in several games
		* coin1/2 won't work in ddcrew (they show in test mode if you force
		  them to active high but still don't change), i/o chip emulation bug?

****************************************************************************

**	MC68000 + Z80
**	2xYM3438 + Custom PCM
**
**	Alien Storm
**	Bloxeed
**	Clutch Hitter
**	D.D. Crew
**	Laser Ghost
**	Michael Jackson's Moonwalker
**	Shadow Dancer
**	Search Wally

***************************************************************************/

#include "driver.h"
#include "system16.h"



/*************************************
 *
 *	Debugging
 *
 *************************************/

#define PRINT_MAPPINGS			(0)



/*************************************
 *
 *	Constants
 *
 *************************************/

#define ROM_BOARD_171_SHADOW	(0)		/* 171-???? -- used by shadow dancer */
#define ROM_BOARD_171_5874		(1)		/* 171-5874 */
#define ROM_BOARD_171_5987		(2)		/* 171-5987 */



/*************************************
 *
 *	Types
 *
 *************************************/

struct region_info
{
	UINT8			regbase;
	offs_t			regoffs;
	offs_t			length;
	offs_t			mirror;
	offs_t			romoffset;
	read16_handler	read;
	write16_handler	write;
	data16_t **		base;
	const char *	name;
};



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 rom_board;
static UINT8 memory_control[0x20];
static UINT8 misc_io_data[0x10];

static read16_handler custom_io_r;
static write16_handler custom_io_w;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

extern void fd1094_machine_init(void);
extern void fd1094_driver_init(void);

extern READ16_HANDLER( segac2_vdp_r );
extern WRITE16_HANDLER( segac2_vdp_w );

static void update_memory_mapping(void);

static READ16_HANDLER( misc_io_r );
static WRITE16_HANDLER( misc_io_w );
static WRITE16_HANDLER( rom_shad_bank_w );
static WRITE16_HANDLER( rom_5987_bank_w );
static READ16_HANDLER( unknown_rgn2_r );
static WRITE16_HANDLER( unknown_rgn2_w );



/*************************************
 *
 *	Memory mapping tables
 *
 *************************************/

static struct region_info rom_171_shad_info[] =
{
	{ 0x3d/2, 0x00000, 0x04000, 0xffc000,      ~0, misc_io_r,             misc_io_w,             NULL,               "I/O space" },
	{ 0x39/2, 0x00000, 0x04000, 0xfff000,      ~0, paletteram16_word_r,   segaic16_paletteram_w, &paletteram16,      "color RAM" },
	{ 0x35/2, 0x00000, 0x10000, 0xfe0000,      ~0, MRA16_BANK10,          segaic16_tileram_w,    &segaic16_tileram,  "tile RAM" },
	{ 0x35/2, 0x10000, 0x01000, 0xfef000,      ~0, MRA16_BANK11,          system18_textram_w,    &segaic16_textram,  "text RAM" },
	{ 0x31/2, 0x00000, 0x00800, 0xfff800,      ~0, MRA16_BANK12,          MWA16_BANK12,          &segaic16_spriteram,"object RAM" },
	{ 0x2d/2, 0x00000, 0x04000, 0xffc000,      ~0, MRA16_BANK13,          MWA16_BANK13,          &sys16_workingram,  "work RAM" },
	{ 0x29/2, 0x00000, 0x10000, 0xff0000,      ~0, NULL,                  NULL,                  NULL,               "????" },
	{ 0x25/2, 0x00000, 0x00010, 0xfffff0,      ~0, segac2_vdp_r,          segac2_vdp_w,          NULL,               "VDP" },
	{ 0x21/2, 0x00000, 0x80000, 0xf80000, 0x00000, MRA16_BANK16,          NULL,                  NULL,               "ROM 0" },
	{ 0 }
};

static struct region_info rom_171_5874_info[] =
{
	{ 0x3d/2, 0x00000, 0x04000, 0xffc000,      ~0, misc_io_r,             misc_io_w,             NULL,               "I/O space" },
	{ 0x39/2, 0x00000, 0x04000, 0xfff000,      ~0, paletteram16_word_r,   segaic16_paletteram_w, &paletteram16,      "color RAM" },
	{ 0x35/2, 0x00000, 0x10000, 0xfe0000,      ~0, MRA16_BANK10,          segaic16_tileram_w,    &segaic16_tileram,  "tile RAM" },
	{ 0x35/2, 0x10000, 0x01000, 0xfef000,      ~0, MRA16_BANK11,          system18_textram_w,    &segaic16_textram,  "text RAM" },
	{ 0x31/2, 0x00000, 0x00800, 0xfff800,      ~0, MRA16_BANK12,          MWA16_BANK12,          &segaic16_spriteram,"object RAM" },
	{ 0x2d/2, 0x00000, 0x04000, 0xffc000,      ~0, MRA16_BANK13,          MWA16_BANK13,          &sys16_workingram,  "work RAM" },
	{ 0x29/2, 0x00000, 0x00010, 0xfffff0,      ~0, segac2_vdp_r,          segac2_vdp_w,          NULL,               "VDP" },
	{ 0x25/2, 0x00000, 0x80000, 0xf80000, 0x80000, MRA16_BANK15,          NULL,                  NULL,               "ROM 1" },
	{ 0x21/2, 0x00000, 0x80000, 0xf80000, 0x00000, MRA16_BANK16,          NULL,                  NULL,               "ROM 0" },
	{ 0 }
};

static struct region_info rom_171_5987_info[] =
{
	{ 0x3d/2, 0x00000, 0x04000, 0xffc000,      ~0, misc_io_r,             misc_io_w,             NULL,               "I/O space" },
	{ 0x39/2, 0x00000, 0x04000, 0xfff000,      ~0, paletteram16_word_r,   segaic16_paletteram_w, &paletteram16,      "color RAM" },
	{ 0x35/2, 0x00000, 0x10000, 0xfe0000,      ~0, MRA16_BANK10,          segaic16_tileram_w,    &segaic16_tileram,  "tile RAM" },
	{ 0x35/2, 0x10000, 0x01000, 0xfef000,      ~0, MRA16_BANK11,          system18_textram_w,    &segaic16_textram,  "text RAM" },
	{ 0x31/2, 0x00000, 0x00800, 0xfff800,      ~0, MRA16_BANK12,          MWA16_BANK12,          &segaic16_spriteram,"object RAM" },
	{ 0x2d/2, 0x00000, 0x04000, 0xffc000,      ~0, MRA16_BANK13,          MWA16_BANK13,          &sys16_workingram,  "work RAM" },
	{ 0x29/2, 0x00000, 0x00010, 0xfffff0,      ~0, segac2_vdp_r,          segac2_vdp_w,          NULL,               "VDP" },
	{ 0x25/2, 0x00000, 0x80000, 0xf80000, 0x80000, MRA16_BANK15,          rom_5987_bank_w,       NULL,               "ROM 1/banking" },
	{ 0x21/2, 0x00000, 0x80000, 0xf80000, 0x00000, MRA16_BANK16,          NULL,                  NULL,               "ROM 0" },
	{ 0 }
};

static struct region_info *region_info_list[] =
{
	&rom_171_shad_info[0],
	&rom_171_5874_info[0],
	&rom_171_5987_info[0]
};



/*************************************
 *
 *	Configuration
 *
 *************************************/

static void system18_generic_init(int _rom_board)
{
	int rgnum;//, soundnum;

	/* set the ROM board */
	rom_board = _rom_board;

	/* loop over the regions and allocate memory */
	for (rgnum = 0; region_info_list[rom_board][rgnum].regbase != 0; rgnum++)
	{
		const struct region_info *rgn = &region_info_list[rom_board][rgnum];
		if (rgn->base)
			*rgn->base = auto_malloc(rgn->length);
	}

	/* init the FD1094 */
	fd1094_driver_init();

	/* reset the custom handlers and other pointers */
	custom_io_r = NULL;
	custom_io_w = NULL;
}



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

MACHINE_INIT( system18 )
{
	fd1094_machine_init();
}



/*************************************
 *
 *	Dynamic memory maps
 *
 *************************************/

static WRITE16_HANDLER( unmapped_w )
{
	UINT8 oldval;

	/* only matters if accessing the LSB */
	if (!ACCESSING_LSB)
		return;

	/* wraps every 32 words */
	offset &= 0x1f;
	data &= 0xff;

	/* remember the previous value and swap in the new one */
	oldval = memory_control[offset];
	memory_control[offset] = data;

	/* switch off the offset */
	switch (offset)
	{
		case 0x05/2:
			printf("Halting main CPU! (PC=%06X)\n", activecpu_get_pc());
			cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
			break;

		case 0x07/2:
			soundlatch_w(0, data & 0xff);
			cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
			break;

		case 0x09/2:
			printf("Resetting main CPU!\n");
			cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
			break;

		case 0x21/2:	case 0x23/2:
		case 0x25/2:	case 0x27/2:
		case 0x29/2:	case 0x2b/2:
		case 0x2d/2:	case 0x2f/2:
		case 0x31/2:	case 0x33/2:
		case 0x35/2:	case 0x37/2:
		case 0x39/2:	case 0x3b/2:
		case 0x3d/2:	case 0x3f/2:
			if (oldval != data)
				update_memory_mapping();
			break;

		default:
			logerror("Unknown unmapped_w to address %02X = %02X\n", offset * 2 + 1, data);
			break;
	}
}


static READ16_HANDLER( unmapped_r )
{
	/* Unmapped memory returns the last word on the data bus, which is almost always the opcode */
	/* of the next instruction due to prefetch; however, since we may be encrypted, we actually */
	/* need to return the encrypted opcode, not the last decrypted data. */

	/* Believe it or not, this is actually important for Cotton, which has the following evil */
	/* code: btst #0,$7038f7, which tests the low bit of an unmapped address, which thus should */
	/* return the prefetched value. */
	static int recurse = 0;
	data16_t result;

	/* prevent recursion */
	if (recurse)
		return 0xffff;

	/* read original encrypted memory at that address */
	recurse = 1;
	result = program_read_word_16be(activecpu_get_pc());
	recurse = 0;
	return result;
}


static void update_memory_mapping(void)
{
	static const offs_t region_size_map[4] = { 0x00ffff, 0x01ffff, 0x07ffff, 0x1fffff };
	int rgnum;

	if (PRINT_MAPPINGS) printf("----\nRemapping:\n");

	/* first reset everything back to the beginning */
	memory_install_read16_handler (0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x00ffff, 0, 0, MRA16_ROM);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x00ffff, 0, 0, (write16_handler)STATIC_UNMAP);
	memory_install_read16_handler (0, ADDRESS_SPACE_PROGRAM, 0x010000, 0xffffff, 0, 0, unmapped_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x010000, 0xffffff, 0, 0, unmapped_w);

	/* loop over the regions */
	for (rgnum = 0; region_info_list[rom_board][rgnum].regbase != 0; rgnum++)
	{
		const struct region_info *rgn = &region_info_list[rom_board][rgnum];
		offs_t region_size = region_size_map[memory_control[rgn->regbase] & 3];
		offs_t region_base = (memory_control[rgn->regbase + 1] << 16) & ~region_size;

		/* only map if the base is non-zero, or if it's the base of the first ROM */
		if (region_base != 0 || rgn->romoffset == 0)
		{
			offs_t region_mirror = rgn->mirror & region_size;
			offs_t region_start = region_base + (rgn->regoffs & region_size);
			offs_t region_end = region_start + ((rgn->length - 1 < region_size) ? rgn->length - 1 : region_size);
			write16_handler write = rgn->write;
			read16_handler read = rgn->read;
			int banknum = 0;

			/* check for mapping to banks */
			if ((FPTR)read >= STATIC_BANK1 && (FPTR)read <= STATIC_BANKMAX)
				banknum = ((FPTR)read - STATIC_BANK1) + 1;
			if ((FPTR)write >= STATIC_BANK1 && (FPTR)write <= STATIC_BANKMAX)
				banknum = ((FPTR)write - STATIC_BANK1) + 1;

			/* ROM areas need extra clamping */
			if (rgn->romoffset != ~0)
			{
				offs_t romsize = memory_region_length(REGION_CPU1);
				if (region_start >= romsize)
					read = NULL;
				else if (region_start + rgn->length > romsize)
					region_end = romsize - 1;
			}

			/* map it */
			if (read)
				memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, region_start, region_end, 0, region_mirror, read);
			if (write)
				memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, region_start, region_end, 0, region_mirror, write);

			/* set the bank pointer */
			if (banknum && (read || write))
			{
				if (rgn->base)
					memory_set_bankptr(banknum, *rgn->base);
				else if (rgn->romoffset != ~0)
					memory_set_bankptr(banknum, memory_region(REGION_CPU1) + region_start);
			}

			if (PRINT_MAPPINGS) printf("  %06X-%06X (%06X) = %s\n", region_start, region_end, region_mirror, rgn->name);
		}
	}
}



/*************************************
 *
 *	I/O space
 *
 *************************************/

static READ16_HANDLER( io_chip_r )
{
	offset &= 0x1f/2;

	switch (offset)
	{
		/* I/O ports */
		case 0x00/2:
		case 0x02/2:
		case 0x04/2:
		case 0x06/2:
		case 0x08/2:
		case 0x0a/2:
		case 0x0c/2:
		case 0x0e/2:
			/* if the port is configured as an output, return the last thing written */
			if (misc_io_data[0x1e/2] & (1 << offset))
				return misc_io_data[offset];

			/* otherwise, return an input port */
			return readinputport(offset);

		/* 'SEGA' protection */
		case 0x10/2:
			return 'S';
		case 0x12/2:
			return 'E';
		case 0x14/2:
			return 'G';
		case 0x16/2:
			return 'A';

		/* CNT register & mirror */
		case 0x18/2:
		case 0x1c/2:
			return misc_io_data[0x1c/2];

		/* port direction register & mirror */
		case 0x1a/2:
		case 0x1e/2:
			return misc_io_data[0x1e/2];
	}
	return 0xffff;
}


static WRITE16_HANDLER( io_chip_w )
{
	UINT8 old;

	/* generic implementation */
	offset &= 0x1f/2;
	old = misc_io_data[offset];
	misc_io_data[offset] = data;

	switch (offset)
	{
		/* I/O ports */
		case 0x00/2:
		case 0x02/2:
		case 0x04/2:
		case 0x08/2:
		case 0x0a/2:
		case 0x0c/2:
			break;

		/* miscellaneous output */
		case 0x06/2:
			system18_set_grayscale(~data & 0x40);
			system18_set_screen_flip(data & 0x20);
			coin_lockout_w(1, data & 0x08);
			coin_lockout_w(0, data & 0x04);
			coin_counter_w(1, data & 0x02);
			coin_counter_w(0, data & 0x01);
			break;

		/* tile banking */
		case 0x0e/2:
			if (rom_board == ROM_BOARD_171_5874 || rom_board == ROM_BOARD_171_SHADOW)
			{
				int i;
				for (i = 0; i < 4; i++)
				{
					system18_set_tile_bank(0 + i, (data & 0xf) * 4 + i);
					system18_set_tile_bank(4 + i, ((data >> 4) & 0xf) * 4 + i);
				}
			}
			break;

		/* CNT register */
		case 0x1c/2:
			if ((old ^ data) & 2)
				system18_set_draw_enable(data & 2);
			if ((old ^ data) & 4)
				system18_set_vdp_enable(data & 4);
			break;
	}
}


static READ16_HANDLER( misc_io_r )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		/* I/O chip */
		case 0x0000/2:
		case 0x1000/2:
			return io_chip_r(offset, mem_mask);

		/* video control latch */
		case 0x2000/2:
			return readinputport(4 + (offset & 1));
	}
	if (custom_io_r)
		return custom_io_r(offset, mem_mask);
	logerror("%06X:misc_io_r - unknown read access to address %04X\n", activecpu_get_pc(), offset * 2);
	return unmapped_r(offset, mem_mask);
}


static WRITE16_HANDLER( misc_io_w )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		/* I/O chip */
		case 0x0000/2:
		case 0x1000/2:
			if (ACCESSING_LSB)
			{
				io_chip_w(offset, data, mem_mask);
				return;
			}
			break;

		/* video control latch */
		case 0x2000/2:
			if (ACCESSING_LSB)
			{
				system18_set_vdp_mixing(data & 0xff);
				return;
			}
			break;
	}
	if (custom_io_w)
		return custom_io_w(offset, data, mem_mask);
	logerror("%06X:misc_io_w - unknown write access to address %04X = %04X & %04X\n", activecpu_get_pc(), offset * 2, data, mem_mask ^ 0xffff);
}



static READ16_HANDLER( unknown_rgn2_r )
{
	if (PRINT_MAPPINGS) printf("Region 2: read from %04X\n", offset * 2);
	return unmapped_r(offset, mem_mask);
}


static WRITE16_HANDLER( unknown_rgn2_w )
{
	if (PRINT_MAPPINGS) printf("Region 2: write to %04X = %04X & %04X\n", offset * 2, data, mem_mask ^ 0xffff);
}

/*************************************
  Game Specific I/O
*************************************/

static READ16_HANDLER ( ddcrew_custom_io_r )
{

	switch (offset*2)
	{
		case 0x3020:
			return readinputportbytag("P3");

		case 0x3022:
			return readinputportbytag("P4");

		case 0x3024:
			return readinputportbytag("P34START");

		default:
	//		printf("unmapped %08x\n",offset*2);
			break;
	}

	return 0xffff;
}

/*************************************
 *
 *	Tile banking
 *
 *************************************/

static WRITE16_HANDLER( rom_shad_bank_w )
{
	logerror("shad_bank(%X) = %04X & %04X\n", offset, data, mem_mask ^ 0xffff);
}


static WRITE16_HANDLER( rom_5987_bank_w )
{
	if (!ACCESSING_LSB)
		return;
	offset &= 0xf;
	data &= 0xff;

	/* tile banking */
	if (offset < 8)
	{
		int maxbanks = Machine->gfx[0]->total_elements / 1024;
		if (data >= maxbanks)
			data %= maxbanks;
		system18_set_tile_bank(offset, data);
	}

	/* sprite banking */
	else
	{
		int maxbanks = memory_region_length(REGION_GFX2) / 0x40000;
		if (data >= maxbanks)
			data = 255;
		system18_set_sprite_bank(offset - 8, data);
	}
}



/*************************************
 *
 *	Sound handlers
 *
 *************************************/

static WRITE8_HANDLER( soundbank_w )
{
	memory_set_bankptr(1, memory_region(REGION_CPU2) + 0x10000 + 0x2000 * data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( system18_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x00ffff) AM_ROM
	AM_RANGE(0x010000, 0xffffff) AM_WRITE(unmapped_w)
ADDRESS_MAP_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x9fff) AM_ROM AM_REGION(REGION_CPU2, 0x10000)
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_MIRROR(0x0ff0) AM_READWRITE(RF5C68_r, RF5C68_reg_w)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(RF5C68_r, RF5C68_w)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0c) AM_READWRITE(YM2612_status_port_0_A_r, YM2612_control_port_0_A_w)
	AM_RANGE(0x81, 0x81) AM_MIRROR(0x0c) AM_WRITE(YM2612_data_port_0_A_w)
	AM_RANGE(0x82, 0x82) AM_MIRROR(0x0c) AM_WRITE(YM2612_control_port_0_B_w)
	AM_RANGE(0x83, 0x83) AM_MIRROR(0x0c) AM_WRITE(YM2612_data_port_0_B_w)
	AM_RANGE(0x90, 0x90) AM_MIRROR(0x0c) AM_READWRITE(YM2612_status_port_1_A_r, YM2612_control_port_1_A_w)
	AM_RANGE(0x91, 0x91) AM_MIRROR(0x0c) AM_WRITE(YM2612_data_port_1_A_w)
	AM_RANGE(0x92, 0x92) AM_MIRROR(0x0c) AM_WRITE(YM2612_control_port_1_B_w)
	AM_RANGE(0x93, 0x93) AM_MIRROR(0x0c) AM_WRITE(YM2612_data_port_1_B_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x1f) AM_WRITE(soundbank_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x1f) AM_READ(soundlatch_r)
ADDRESS_MAP_END



/*************************************
 *
 *	Generic port definitions
 *
 *************************************/

static INPUT_PORTS_START( system18_generic )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("PORTC")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PORTD")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SERVICE")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COINAGE")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1" )

	PORT_START_TAG("DSW")
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

	PORT_START_TAG("PORTH")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START( generic )
	PORT_INCLUDE( system18_generic )
INPUT_PORTS_END



/*************************************
 *
 *	Game-specific port definitions
 *
 *************************************/

static INPUT_PORTS_START( astorm )
	PORT_INCLUDE( system18_generic )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easiest" )
	PORT_DIPSETTING(    0x08, "Easier" )
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x14, DEF_STR( Harder ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Hardest ) )
	PORT_DIPSETTING(    0x00, "Special" )
	PORT_DIPNAME( 0x20, 0x20, "Coin Chutes" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "1" )
INPUT_PORTS_END


static INPUT_PORTS_START( shdancer )
	PORT_INCLUDE( system18_generic )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Time Adjust" )
	PORT_DIPSETTING(    0x00, "2.20" )
	PORT_DIPSETTING(    0x40, "2.40" )
	PORT_DIPSETTING(    0xc0, "3.00" )
	PORT_DIPSETTING(    0x80, "3.30" )
INPUT_PORTS_END

static INPUT_PORTS_START( ddcrew )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)

	PORT_START_TAG("PORTC")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PORTD")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SERVICE")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) // ?? game doesn't want to read these correctly
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) // ?? game doesn't want to read these correctly
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("COINAGE")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1" )

	PORT_START_TAG("DSW")
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

	PORT_START_TAG("PORTH")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)

	PORT_START_TAG("P4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)

	PORT_START_TAG("P34START")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) // individual mode
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 ) // individual mode
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct RF5C68interface rf5c68_interface =
{
	8000000,
	100
};


static struct YM2612interface ym3438_interface =
{
	2,
	8000000,
	{ YM3012_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER),
	  YM3012_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER) },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL }
};



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	0, 1024 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( system18 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(system18_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(sound_portmap,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1000000 * (262 - 224) / (262 * 60))

	MDRV_MACHINE_INIT(system18)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*3+2048)

	MDRV_VIDEO_START(system18)
	MDRV_VIDEO_UPDATE(system18)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("3438", YM3438, ym3438_interface)
	MDRV_SOUND_ADD_TAG("5c68", RF5C68, rf5c68_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Alien Storm, Sega System 18
	CPU: FD1094 (317-0146)
	ROM Board: 171-5873B
*/
ROM_START( astorm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13085.bin", 0x000000, 0x40000, CRC(15f74e2d) SHA1(30d9d099ec18907edd3d20df312565c3bd5a80de) )
	ROM_LOAD16_BYTE( "epr13084.bin", 0x000001, 0x40000, CRC(9687b38f) SHA1(cdeb5b4f06ad4ad8ca579392c1ec901487b08e76) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0146.key", 0x0000, 0x2000, CRC(e94991c5) SHA1(c9a8b56e01792654436f24b219d7a92c0852461f) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, CRC(df5d0a61) SHA1(79ad71de348f280bad847566c507b7a31f022292) )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, CRC(787afab8) SHA1(a119042bb2dad54e9733bfba4eaab0ac5fc0f9e7) )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, CRC(4e01b477) SHA1(4178ce4a87ea427c3b0195e64acef6cddfb3485f) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, CRC(a782b704) SHA1(ba15bdfbc267b8d86f03e5310ce60846ff846de3) )
	ROM_LOAD16_BYTE( "mpr13089.bin", 0x000000, 0x40000, CRC(2a4227f0) SHA1(47284dce8f896f8e8eace9c20302842cacb479c1) )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, CRC(eb510228) SHA1(4cd387b160ec7050e1300ebe708853742169e643) )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, CRC(3b6b4c55) SHA1(970495c54b3e1893ee8060f6ca1338c2cbbd1074) )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, CRC(e668eefb) SHA1(d4a087a238b4d3ac2d23fe148d6a73018e348a89) )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, CRC(2293427d) SHA1(4fd07763ff060afd594e3f64fa4750577f56c80e) )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, CRC(de9221ed) SHA1(5e2e434d1aa547be1e5652fc906d2e18c5122023) )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, CRC(8c9a71c4) SHA1(40b774765ac888792aad46b6351a24b7ef40d2dc) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13083.bin", 0x010000, 0x20000, CRC(5df3af20) SHA1(e49105fcfd5bf37d14bd760f6adca5ce2412883d) )
	ROM_LOAD( "epr13076.bin", 0x090000, 0x40000, CRC(94e6c76e) SHA1(f99e58a9bf372c41af211bd9b9ea3ac5b924c6ed) )
	ROM_LOAD( "epr13077.bin", 0x110000, 0x40000, CRC(e2ec0d8d) SHA1(225b0d223b7282cba7710300a877fb4a2c6dbabb) )
	ROM_LOAD( "epr13078.bin", 0x190000, 0x40000, CRC(15684dc5) SHA1(595051006de24f791dae937584e502ff2fa31d9c) )
ROM_END

/**************************************************************************************************************************
	Alien Storm (3 players version), Sega System 18
	CPU: FD1094 (317-0148)
	ROM Board: 171-5873B
	Game numbers: 833-7379-02 (main pcb: 834-7381-02, rom pcb: 834-7380-02)
*/
ROM_START( astorma )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ep13165.a6", 0x000000, 0x40000, CRC(6efcd381) SHA1(547c6703a34c3b9b887f5a63ec59a7055067bf3b) )
	ROM_LOAD16_BYTE( "ep13164.a5", 0x000001, 0x40000, CRC(97d693c6) SHA1(1a9aa98b32aae9367ed897e6931b2633b11b079e) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, CRC(df5d0a61) SHA1(79ad71de348f280bad847566c507b7a31f022292) )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, CRC(787afab8) SHA1(a119042bb2dad54e9733bfba4eaab0ac5fc0f9e7) )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, CRC(4e01b477) SHA1(4178ce4a87ea427c3b0195e64acef6cddfb3485f) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, CRC(a782b704) SHA1(ba15bdfbc267b8d86f03e5310ce60846ff846de3) )
	ROM_LOAD16_BYTE( "mpr13089.bin", 0x000000, 0x40000, CRC(2a4227f0) SHA1(47284dce8f896f8e8eace9c20302842cacb479c1) )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, CRC(eb510228) SHA1(4cd387b160ec7050e1300ebe708853742169e643) )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, CRC(3b6b4c55) SHA1(970495c54b3e1893ee8060f6ca1338c2cbbd1074) )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, CRC(e668eefb) SHA1(d4a087a238b4d3ac2d23fe148d6a73018e348a89) )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, CRC(2293427d) SHA1(4fd07763ff060afd594e3f64fa4750577f56c80e) )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, CRC(de9221ed) SHA1(5e2e434d1aa547be1e5652fc906d2e18c5122023) )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, CRC(8c9a71c4) SHA1(40b774765ac888792aad46b6351a24b7ef40d2dc) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13083.bin", 0x10000, 0x20000, CRC(5df3af20) SHA1(e49105fcfd5bf37d14bd760f6adca5ce2412883d) )
	ROM_LOAD( "epr13076.bin", 0x30000, 0x40000, CRC(94e6c76e) SHA1(f99e58a9bf372c41af211bd9b9ea3ac5b924c6ed) )
	ROM_LOAD( "epr13077.bin", 0x70000, 0x40000, CRC(e2ec0d8d) SHA1(225b0d223b7282cba7710300a877fb4a2c6dbabb) )
	ROM_LOAD( "epr13078.bin", 0xb0000, 0x40000, CRC(15684dc5) SHA1(595051006de24f791dae937584e502ff2fa31d9c) )
ROM_END

/**************************************************************************************************************************
	Alien Storm (2 players version), Sega System 18
	CPU: FD1094 (317-????)
	ROM Board: 171-5873B
*/
ROM_START( astorm2p )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13182.bin", 0x000000, 0x40000, CRC(e31f2a1c) SHA1(690ee10c36e5bb6175470fb5564527e0e4a94d2c) )
	ROM_LOAD16_BYTE( "epr13181.bin", 0x000001, 0x40000, CRC(78cd3b26) SHA1(a81b807c5da625d8e4648ae80c41e4ca3870c0fa) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, CRC(df5d0a61) SHA1(79ad71de348f280bad847566c507b7a31f022292) )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, CRC(787afab8) SHA1(a119042bb2dad54e9733bfba4eaab0ac5fc0f9e7) )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, CRC(4e01b477) SHA1(4178ce4a87ea427c3b0195e64acef6cddfb3485f) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, CRC(a782b704) SHA1(ba15bdfbc267b8d86f03e5310ce60846ff846de3) )
	ROM_LOAD16_BYTE( "mpr13089.bin", 0x000000, 0x40000, CRC(2a4227f0) SHA1(47284dce8f896f8e8eace9c20302842cacb479c1) )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, CRC(eb510228) SHA1(4cd387b160ec7050e1300ebe708853742169e643) )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, CRC(3b6b4c55) SHA1(970495c54b3e1893ee8060f6ca1338c2cbbd1074) )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, CRC(e668eefb) SHA1(d4a087a238b4d3ac2d23fe148d6a73018e348a89) )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, CRC(2293427d) SHA1(4fd07763ff060afd594e3f64fa4750577f56c80e) )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, CRC(de9221ed) SHA1(5e2e434d1aa547be1e5652fc906d2e18c5122023) )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, CRC(8c9a71c4) SHA1(40b774765ac888792aad46b6351a24b7ef40d2dc) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "ep13083a.bin", 0x10000, 0x20000, CRC(e7528e06) SHA1(1f4e618692c20aeb316d578c5ded12440eb072ab) )
	ROM_LOAD( "epr13076.bin", 0x30000, 0x40000, CRC(94e6c76e) SHA1(f99e58a9bf372c41af211bd9b9ea3ac5b924c6ed) )
	ROM_LOAD( "epr13077.bin", 0x70000, 0x40000, CRC(e2ec0d8d) SHA1(225b0d223b7282cba7710300a877fb4a2c6dbabb) )
	ROM_LOAD( "epr13078.bin", 0xb0000, 0x40000, CRC(15684dc5) SHA1(595051006de24f791dae937584e502ff2fa31d9c) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Bloxeed, Sega System 18
	CPU: FD1094 (317-????)
	ROM Board: 171-????
*/
ROM_START( bloxeed )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "rom-e.rom", 0x000000, 0x20000, CRC(a481581a) SHA1(5ce5a0a082622919d2fe0e7d52ec807b2e2c25a2) )
	ROM_LOAD16_BYTE( "rom-o.rom", 0x000001, 0x20000, CRC(dd1bc3bf) SHA1(c0d79862a349ea4dac103c17325633c5dd4a93d1) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr0.rom", 0x00000, 0x10000, CRC(e024aa33) SHA1(d734be240cd05031aaadf9735c0b1b00e8e6d4cb) )
	ROM_LOAD( "scr1.rom", 0x10000, 0x10000, CRC(8041b814) SHA1(29fa49ba9a73eed07865a86ea774e2c6a60aed5b) )
	ROM_LOAD( "scr2.rom", 0x20000, 0x10000, CRC(de32285e) SHA1(8994dc128d6a23763e5fcfca1868b336d4aa0a21) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x00001, 0x10000, CRC(90d31a8c) SHA1(1747652a5109ce65add197cf06535f2463a99fdc) )
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x00000, 0x10000, CRC(f0c0f49d) SHA1(7ecd591265165f3149241e2ceb5059faab88360f) )

	ROM_REGION( 0x20000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "sound0.rom",	 0x00000, 0x20000, CRC(6f2fc63c) SHA1(3cce22c8f80013f05b5a2d36c42a61a81e4d6cbd) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Clutch Hitter, Sega System 18
	CPU: FD1094 (317-0176)
	ROM Board: 171-5873B
*/
ROM_START( cltchitr )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13751.4a", 0x000000, 0x40000, CRC(c8d80233) SHA1(69cdbb989f580abbb006820347becf8d233fa773) )
	ROM_LOAD16_BYTE( "epr13795.6a", 0x000001, 0x40000, CRC(b0b60b67) SHA1(ee3325ddb7461008f556c1696157a766225b9ef5) )
	ROM_LOAD16_BYTE( "epr13784.5a", 0x200000, 0x40000, CRC(80c8180d) SHA1(80e72ab7d97714009fd31b3d50176af84b0dcdb7) )
	ROM_LOAD16_BYTE( "epr13786.7a", 0x200001, 0x40000, CRC(3095dac0) SHA1(20edce74b6f2a82a3865613e601a0e212615d0e4) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0176.key", 0x0000, 0x2000, CRC(9b072430) SHA1(3bc1c7a6d71b4351a42d85e68e70715a7659c096) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13773.1c",  0x000000, 0x80000, CRC(3fc600e5) SHA1(8bec4ecf6a4e4b38d13133960036a5a4800a668e) )
	ROM_LOAD( "mpr13774.2c",  0x080000, 0x80000, CRC(2411a824) SHA1(0e383ccc4e0830ffb395d5102e2950610c147007) )
	ROM_LOAD( "mpr13775.3c",  0x100000, 0x80000, CRC(cf527bf6) SHA1(1f9cf1f0e92709f0465dc97ebbdaa715a419f7a7) )

	ROM_REGION16_BE( 0x600000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13779.10c", 0x000001, 0x80000, CRC(c707f416) SHA1(e6a9d89849f7f1c303a3ca29a629f81397945a2d) )
	ROM_LOAD16_BYTE( "mpr13787.10a", 0x000000, 0x80000, CRC(f05c68c6) SHA1(b6a0535b6c734a0c89fdb6506c32ffe6ab3aa8cd) )
	ROM_LOAD16_BYTE( "mpr13780.11c", 0x200001, 0x80000, CRC(a4c341e0) SHA1(15a0b5a42b56465a7b7df98968cc2ed177ce6f59) )
	ROM_LOAD16_BYTE( "mpr13788.11a", 0x200000, 0x80000, CRC(0106fea6) SHA1(e16e2a469ecbbc704021dee6264db30aa0898368) )
	ROM_LOAD16_BYTE( "mpr13781.12c", 0x400001, 0x80000, CRC(f33b13af) SHA1(d3eb64dcf12d532454bf3cd6c86528c0f11ee316) )
	ROM_LOAD16_BYTE( "mpr13789.12a", 0x400000, 0x80000, CRC(09ba8835) SHA1(72e83dd1793a7f4b2b881e71f262493e3d4992b3) )

	ROM_REGION( 0x190000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13793.7c",    0x010000, 0x80000, CRC(a3d31944) SHA1(44d17aa0598eacfac4b12c8955fd1067ca09abbd) )
	ROM_LOAD( "epr13792.6c",    0x090000, 0x80000, CRC(808f9695) SHA1(d50677d20083ad56dbf0864db05f76f93a4e9eba) )
	ROM_LOAD( "epr13791.5c",	0x110000, 0x80000, CRC(35c16d80) SHA1(7ed04600748c5efb63e25f066daa163e9c0d8038) )
ROM_END

/**************************************************************************************************************************
	Clutch Hitter, Sega System 18
	CPU: FD1094 (317-0175)
	ROM Board: 171-????
*/
ROM_START( cltchtrj )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13783.a4", 0x000000, 0x40000, CRC(e2a1d5af) SHA1(cb6710fe2093889d5d159eaf55a3bd95c6f2ef87) )
	ROM_LOAD16_BYTE( "epr13796.a6", 0x000001, 0x40000, CRC(06001c67) SHA1(3aa48631013e6dc55e4c1d222b465e6b41ece36b) )
	ROM_LOAD16_BYTE( "epr13785.a5", 0x200000, 0x40000, CRC(09714762) SHA1(c75c88b1c313e172fdb7f9a570d57be38f959b2b) )
	ROM_LOAD16_BYTE( "epr13797.a7", 0x200001, 0x40000,  CRC(361ade9f) SHA1(a7fd48c55695fd322d0456ff7dc2d2b2bc3e561b) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0175.key", 0x0000, 0x2000, CRC(70d9d283) SHA1(ff309b2a221d9a03ccf301a208c76a7c2eaea790) )

	ROM_REGION( 0x180000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr13773.1c",  0x000000, 0x80000, CRC(3fc600e5) SHA1(8bec4ecf6a4e4b38d13133960036a5a4800a668e) )
	ROM_LOAD( "mpr13774.2c",  0x080000, 0x80000, CRC(2411a824) SHA1(0e383ccc4e0830ffb395d5102e2950610c147007) )
	ROM_LOAD( "mpr13775.3c",  0x100000, 0x80000, CRC(cf527bf6) SHA1(1f9cf1f0e92709f0465dc97ebbdaa715a419f7a7) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13779.10c", 0x000001, 0x80000, CRC(c707f416) SHA1(e6a9d89849f7f1c303a3ca29a629f81397945a2d) )
	ROM_LOAD16_BYTE( "mpr13787.10a", 0x000000, 0x80000, CRC(f05c68c6) SHA1(b6a0535b6c734a0c89fdb6506c32ffe6ab3aa8cd) )
	ROM_LOAD16_BYTE( "mpr13780.11c", 0x200001, 0x80000, CRC(a4c341e0) SHA1(15a0b5a42b56465a7b7df98968cc2ed177ce6f59) )
	ROM_LOAD16_BYTE( "mpr13788.11a", 0x200000, 0x80000, CRC(0106fea6) SHA1(e16e2a469ecbbc704021dee6264db30aa0898368) )
	ROM_LOAD16_BYTE( "mpr13781.12c", 0x400001, 0x80000, CRC(f33b13af) SHA1(d3eb64dcf12d532454bf3cd6c86528c0f11ee316) )
	ROM_LOAD16_BYTE( "mpr13789.12a", 0x400000, 0x80000, CRC(09ba8835) SHA1(72e83dd1793a7f4b2b881e71f262493e3d4992b3) )
	/* extra gfx roms??*/
	ROM_LOAD16_BYTE( "epr13782.c13", 0x600001, 0x40000, CRC(73790852) SHA1(891345cb8ce4b04bd293ee9bac9b1b9940d6bcb2) )
	ROM_LOAD16_BYTE( "epr13790.a13", 0x600000, 0x40000, CRC(23849101) SHA1(1aeb0fefb6688dfd841bd7d0b17ffdfce69f1dd9) )

	ROM_REGION( 0x190000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	/* another copy in different set is epr-13778.b7 - 9c54c038 */
	ROM_LOAD( "epr13778.c7",    0x010000, 0x20000, CRC(35e86146) SHA1(9be82165dc12d5f32ef26f37ea02b29e3621893f) )
	ROM_LOAD( "epr13777.c6",    0x090000, 0x80000, CRC(d1524782) SHA1(121c5804927ed686ea50d5d81d0226e041f50f6f) )
	ROM_LOAD( "epr13776.c5",	0x110000, 0x80000, CRC(282ac9fe) SHA1(4f54d93779c35c036d7c85fce6736df80f3bbe33) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	D.D. Crew, Sega System 18
	CPU: FD1094 (317-0186)
	ROM Board: 171-5873B
*/
ROM_START( ddcrew )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "14152.4a", 0x000000, 0x40000, CRC(69c7b571) SHA1(9fe4815a1cff0a46a754a6bdee12abaf7beb6501) )
	ROM_LOAD16_BYTE( "14153.6a", 0x000001, 0x40000, CRC(e01fae0c) SHA1(7166f955324f73e94d8ae6d2a5b2f4b497e62933) )
	ROM_LOAD16_BYTE( "14139.5a", 0x200000, 0x40000, CRC(06c31531) SHA1(d084cb72bf83578b34e959bb60a0695faf4161f8) )
	ROM_LOAD16_BYTE( "14141.7a", 0x200001, 0x40000, CRC(080a494b) SHA1(64522dccbf6ed856ab80aa185454183df87d7ae9) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0186.key", 0x0000, 0x2000, CRC(7acaf1fd) SHA1(236d6382072adda8f7907d98d428fcca36700fa0) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14127.1c", 0x00000, 0x40000, CRC(2228cd88) SHA1(5774bb6a401c3da05c5f3c9d3996b20bb3713cb2) )
	ROM_LOAD( "14128.2c", 0x40000, 0x40000, CRC(edba8e10) SHA1(25a2833ead4ca363802ddc2eb97c40976502921a) )
	ROM_LOAD( "14129.3c", 0x80000, 0x40000, CRC(e8ecc305) SHA1(a26d0c5c7826cd315f8b2c27e5a503a2a7b535c4) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14134.10c", 0x000001, 0x80000, CRC(4fda6a4b) SHA1(a9e582e494ab967e8f3ccf4d5844bb8ef889928c) )
	ROM_LOAD16_BYTE( "14142.10a", 0x000000, 0x80000, CRC(3cbf1f2a) SHA1(80b6b006936740087786acd538e28aca85fa6894) )
	ROM_LOAD16_BYTE( "14135.11c", 0x200001, 0x80000, CRC(e9c74876) SHA1(aff9d071e77f01c6937188bf67be38fa898343e6) )
	ROM_LOAD16_BYTE( "14143.11a", 0x200000, 0x80000, CRC(59022c31) SHA1(5e1409fe0f29284dc6a3ffacf69b761aae09f132) )
	ROM_LOAD16_BYTE( "14136.12c", 0x400001, 0x80000, CRC(720d9858) SHA1(8ebcb8b3e9555ca48b28908d47dcbbd654398b6f) )
	ROM_LOAD16_BYTE( "14144.12a", 0x400000, 0x80000, CRC(7775fdd4) SHA1(a03cac039b400b651a4bf2167a8f2338f488ce26) )
	ROM_LOAD16_BYTE( "14137.13c", 0x600001, 0x80000, CRC(846c4265) SHA1(58d0c213d085fb4dee18b7aefb05087d9d522950) )
	ROM_LOAD16_BYTE( "14145.13a", 0x600000, 0x80000, CRC(0e76c797) SHA1(9a44dc948e84e5acac36e80105c2349ee78e6cfa) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "14133.7c",	 0x010000, 0x20000, CRC(cff96665) SHA1(b4dc7f1a03415ebebdb99a82ae89328c345e7678) )
	ROM_LOAD( "14132.6c",    0x090000, 0x80000, CRC(1fae0220) SHA1(8414c74318ea915816c6b67801ac7c8c3fc905f9) )
	ROM_LOAD( "14131.5c",    0x110000, 0x80000, CRC(be5a7d0b) SHA1(c2c598b0cf711273fdd568f3401375e9772c1d61) )
	ROM_LOAD( "14130.4c",    0x190000, 0x80000, CRC(948f34a1) SHA1(d4c6728d5eea06cee6ac15a34ec8cccb4cc4b982) )
ROM_END

/**************************************************************************************************************************
	D.D. Crew, Sega System 18
	CPU: FD1094 (317-????)
	ROM Board: 171-5873B
*/
ROM_START( ddcrewa )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14138.a4", 0x000000, 0x40000, CRC(9a0dadf0) SHA1(b55c8cdd3158607ec8203bfebc9e7aba24d6d565) )
	ROM_LOAD16_BYTE( "epr14140.a6", 0x000001, 0x40000, CRC(e74362f4) SHA1(a6f96d714baeb826221b712b996e99831cf25abf) )
	ROM_LOAD16_BYTE( "14139.5a", 0x200000, 0x40000, CRC(06c31531) SHA1(d084cb72bf83578b34e959bb60a0695faf4161f8) )
	ROM_LOAD16_BYTE( "14141.7a", 0x200001, 0x40000, CRC(080a494b) SHA1(64522dccbf6ed856ab80aa185454183df87d7ae9) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14127.1c", 0x00000, 0x40000, CRC(2228cd88) SHA1(5774bb6a401c3da05c5f3c9d3996b20bb3713cb2) )
	ROM_LOAD( "14128.2c", 0x40000, 0x40000, CRC(edba8e10) SHA1(25a2833ead4ca363802ddc2eb97c40976502921a) )
	ROM_LOAD( "14129.3c", 0x80000, 0x40000, CRC(e8ecc305) SHA1(a26d0c5c7826cd315f8b2c27e5a503a2a7b535c4) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14134.10c", 0x000001, 0x80000, CRC(4fda6a4b) SHA1(a9e582e494ab967e8f3ccf4d5844bb8ef889928c) )
	ROM_LOAD16_BYTE( "14142.10a", 0x000000, 0x80000, CRC(3cbf1f2a) SHA1(80b6b006936740087786acd538e28aca85fa6894) )
	ROM_LOAD16_BYTE( "14135.11c", 0x200001, 0x80000, CRC(e9c74876) SHA1(aff9d071e77f01c6937188bf67be38fa898343e6) )
	ROM_LOAD16_BYTE( "14143.11a", 0x200000, 0x80000, CRC(59022c31) SHA1(5e1409fe0f29284dc6a3ffacf69b761aae09f132) )
	ROM_LOAD16_BYTE( "14136.12c", 0x400001, 0x80000, CRC(720d9858) SHA1(8ebcb8b3e9555ca48b28908d47dcbbd654398b6f) )
	ROM_LOAD16_BYTE( "14144.12a", 0x400000, 0x80000, CRC(7775fdd4) SHA1(a03cac039b400b651a4bf2167a8f2338f488ce26) )
	ROM_LOAD16_BYTE( "14137.13c", 0x600001, 0x80000, CRC(846c4265) SHA1(58d0c213d085fb4dee18b7aefb05087d9d522950) )
	ROM_LOAD16_BYTE( "14145.13a", 0x600000, 0x80000, CRC(0e76c797) SHA1(9a44dc948e84e5acac36e80105c2349ee78e6cfa) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "14133.7c",	 0x010000, 0x20000, CRC(cff96665) SHA1(b4dc7f1a03415ebebdb99a82ae89328c345e7678) )
	ROM_LOAD( "14130.4c",    0x090000, 0x80000, CRC(948f34a1) SHA1(d4c6728d5eea06cee6ac15a34ec8cccb4cc4b982) )
	ROM_LOAD( "14131.5c",    0x110000, 0x80000, CRC(be5a7d0b) SHA1(c2c598b0cf711273fdd568f3401375e9772c1d61) )
	ROM_LOAD( "14132.6c",    0x190000, 0x80000, CRC(1fae0220) SHA1(8414c74318ea915816c6b67801ac7c8c3fc905f9) )
ROM_END

/**************************************************************************************************************************
	D.D. Crew, Sega System 18
	CPU: FD1094 (317-0184)
	ROM Board: 171-5873B
*/
ROM_START( ddcrewb )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14148.a4", 0x000000, 0x40000, CRC(df4cb0cf) SHA1(153993997e9ceb06a9d4c73794ea66d0c585a291) )
	ROM_LOAD16_BYTE( "epr14149.a6", 0x000001, 0x40000, CRC(380ff818) SHA1(7c7dd7aa825665f1a9aaebb5af4ecf5dd109b0ca) )
	ROM_LOAD16_BYTE( "14139.5a", 0x200000, 0x40000, CRC(06c31531) SHA1(d084cb72bf83578b34e959bb60a0695faf4161f8) )
	ROM_LOAD16_BYTE( "14141.7a", 0x200001, 0x40000, CRC(080a494b) SHA1(64522dccbf6ed856ab80aa185454183df87d7ae9) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14127.1c", 0x00000, 0x40000, CRC(2228cd88) SHA1(5774bb6a401c3da05c5f3c9d3996b20bb3713cb2) )
	ROM_LOAD( "14128.2c", 0x40000, 0x40000, CRC(edba8e10) SHA1(25a2833ead4ca363802ddc2eb97c40976502921a) )
	ROM_LOAD( "14129.3c", 0x80000, 0x40000, CRC(e8ecc305) SHA1(a26d0c5c7826cd315f8b2c27e5a503a2a7b535c4) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14134.10c", 0x000001, 0x80000, CRC(4fda6a4b) SHA1(a9e582e494ab967e8f3ccf4d5844bb8ef889928c) )
	ROM_LOAD16_BYTE( "14142.10a", 0x000000, 0x80000, CRC(3cbf1f2a) SHA1(80b6b006936740087786acd538e28aca85fa6894) )
	ROM_LOAD16_BYTE( "14135.11c", 0x200001, 0x80000, CRC(e9c74876) SHA1(aff9d071e77f01c6937188bf67be38fa898343e6) )
	ROM_LOAD16_BYTE( "14143.11a", 0x200000, 0x80000, CRC(59022c31) SHA1(5e1409fe0f29284dc6a3ffacf69b761aae09f132) )
	ROM_LOAD16_BYTE( "14136.12c", 0x400001, 0x80000, CRC(720d9858) SHA1(8ebcb8b3e9555ca48b28908d47dcbbd654398b6f) )
	ROM_LOAD16_BYTE( "14144.12a", 0x400000, 0x80000, CRC(7775fdd4) SHA1(a03cac039b400b651a4bf2167a8f2338f488ce26) )
	ROM_LOAD16_BYTE( "14137.13c", 0x600001, 0x80000, CRC(846c4265) SHA1(58d0c213d085fb4dee18b7aefb05087d9d522950) )
	ROM_LOAD16_BYTE( "14145.13a", 0x600000, 0x80000, CRC(0e76c797) SHA1(9a44dc948e84e5acac36e80105c2349ee78e6cfa) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "14133.7c",	 0x010000, 0x20000, CRC(cff96665) SHA1(b4dc7f1a03415ebebdb99a82ae89328c345e7678) )
	ROM_LOAD( "14130.4c",    0x090000, 0x80000, CRC(948f34a1) SHA1(d4c6728d5eea06cee6ac15a34ec8cccb4cc4b982) )
	ROM_LOAD( "14131.5c",    0x110000, 0x80000, CRC(be5a7d0b) SHA1(c2c598b0cf711273fdd568f3401375e9772c1d61) )
	ROM_LOAD( "14132.6c",    0x190000, 0x80000, CRC(1fae0220) SHA1(8414c74318ea915816c6b67801ac7c8c3fc905f9) )
ROM_END

/**************************************************************************************************************************
	D.D. Crew, Sega System 18
	CPU: FD1094 (317-0187)
	ROM Board: 171-5873B
*/
ROM_START( ddcrewc )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14160.a4", 0x000000, 0x40000, CRC(b9f897b7) SHA1(65cee6c8006f328eee648e144e11fa60b1433ff5) )
	ROM_LOAD16_BYTE( "epr14161.a6", 0x000001, 0x40000, CRC(bb03c1f0) SHA1(9e7fbd2cda448992c6cbf4b96078b57305def097) )
	ROM_LOAD16_BYTE( "14139.5a", 0x200000, 0x40000, CRC(06c31531) SHA1(d084cb72bf83578b34e959bb60a0695faf4161f8) )
	ROM_LOAD16_BYTE( "14141.7a", 0x200001, 0x40000, CRC(080a494b) SHA1(64522dccbf6ed856ab80aa185454183df87d7ae9) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14127.1c", 0x00000, 0x40000, CRC(2228cd88) SHA1(5774bb6a401c3da05c5f3c9d3996b20bb3713cb2) )
	ROM_LOAD( "14128.2c", 0x40000, 0x40000, CRC(edba8e10) SHA1(25a2833ead4ca363802ddc2eb97c40976502921a) )
	ROM_LOAD( "14129.3c", 0x80000, 0x40000, CRC(e8ecc305) SHA1(a26d0c5c7826cd315f8b2c27e5a503a2a7b535c4) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14134.10c", 0x000001, 0x80000, CRC(4fda6a4b) SHA1(a9e582e494ab967e8f3ccf4d5844bb8ef889928c) )
	ROM_LOAD16_BYTE( "14142.10a", 0x000000, 0x80000, CRC(3cbf1f2a) SHA1(80b6b006936740087786acd538e28aca85fa6894) )
	ROM_LOAD16_BYTE( "14135.11c", 0x200001, 0x80000, CRC(e9c74876) SHA1(aff9d071e77f01c6937188bf67be38fa898343e6) )
	ROM_LOAD16_BYTE( "14143.11a", 0x200000, 0x80000, CRC(59022c31) SHA1(5e1409fe0f29284dc6a3ffacf69b761aae09f132) )
	ROM_LOAD16_BYTE( "14136.12c", 0x400001, 0x80000, CRC(720d9858) SHA1(8ebcb8b3e9555ca48b28908d47dcbbd654398b6f) )
	ROM_LOAD16_BYTE( "14144.12a", 0x400000, 0x80000, CRC(7775fdd4) SHA1(a03cac039b400b651a4bf2167a8f2338f488ce26) )
	ROM_LOAD16_BYTE( "14137.13c", 0x600001, 0x80000, CRC(846c4265) SHA1(58d0c213d085fb4dee18b7aefb05087d9d522950) )
	ROM_LOAD16_BYTE( "14145.13a", 0x600000, 0x80000, CRC(0e76c797) SHA1(9a44dc948e84e5acac36e80105c2349ee78e6cfa) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "14133.7c",	 0x010000, 0x20000, CRC(cff96665) SHA1(b4dc7f1a03415ebebdb99a82ae89328c345e7678) )
	ROM_LOAD( "14130.4c",    0x090000, 0x80000, CRC(948f34a1) SHA1(d4c6728d5eea06cee6ac15a34ec8cccb4cc4b982) )
	ROM_LOAD( "14131.5c",    0x110000, 0x80000, CRC(be5a7d0b) SHA1(c2c598b0cf711273fdd568f3401375e9772c1d61) )
	ROM_LOAD( "14132.6c",    0x190000, 0x80000, CRC(1fae0220) SHA1(8414c74318ea915816c6b67801ac7c8c3fc905f9) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Laser Ghost, Sega System 18
	CPU: FD1094 (317-0165)
	ROM Board: 171-5873B
*/
ROM_START( lghost )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13427.bin", 0x000000, 0x40000, CRC(5bf8fb6b) SHA1(587bbf45b5e470da7d9166b2cbf4ac58a1f2a825) )
	ROM_LOAD16_BYTE( "epr13428.bin", 0x000001, 0x40000, CRC(276775f5) SHA1(5dd5dabe7e9311b3520d0405dda0c983e9bc4ba2) )
	ROM_LOAD16_BYTE( "epr13411.5a",  0x200000, 0x40000, CRC(5160167b) SHA1(3d176a18c7527b1e485f10b144bb4db1b945e709) )
	ROM_LOAD16_BYTE( "epr13413.7a",  0x200001, 0x40000, CRC(656b3bd8) SHA1(db81d4ae3138308dce1e3db7a859f1d63c4ff815) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0165.key", 0x0000, 0x2000,  CRC(a04267ab) SHA1(688ee59dfaaf240e23de4cada648689d1717ab04) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13414.1c", 0x00000, 0x40000, CRC(dada2419) SHA1(f6ffd02d75232a09ea83fd199e5e30b2773b0cf5) )
	ROM_LOAD( "epr13415.2c", 0x40000, 0x40000, CRC(bbb62c48) SHA1(7a4c5bd11b73a92deece72b55627f48ac167acd6) )
	ROM_LOAD( "epr13416.3c", 0x80000, 0x40000, CRC(1d11dbae) SHA1(331aa49c6b38d32ec33184dbd0851888463ddbc7) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr13603.10a", 0x000000, 0x80000, CRC(5350a94e) SHA1(47e99803cab4b508feb51069c940d6c824d6961d) )
	ROM_LOAD16_BYTE( "epr13604.10c", 0x000001, 0x80000, CRC(4009c8e5) SHA1(97f513d312f4c90f8bffdf797afa3749779989a5) )
	ROM_LOAD16_BYTE( "epr13421.11a", 0x200000, 0x80000, CRC(2fc75890) SHA1(9f97f07dba3b978df8eb357894168ad74f151d30) )
	ROM_LOAD16_BYTE( "mpr13424.11c", 0x200001, 0x80000, CRC(fb98d920) SHA1(cebdebe88902e96c631df6053ac2589f794da155) )
	ROM_LOAD16_BYTE( "mpr13422.12a", 0x400000, 0x80000, CRC(48a0754d) SHA1(9fead9f8319593adb4bddaaa4d053b21ca726009) )
	ROM_LOAD16_BYTE( "mpr13425.12c", 0x400001, 0x80000, CRC(f8252589) SHA1(5a1ed24296d0609393e53df3ee585a366da4ee46) )
	ROM_LOAD16_BYTE( "mpr13423.13a", 0x600000, 0x80000, CRC(335bbc9d) SHA1(78793335b2f8a1bb05809259521db193c17c9b98) )
	ROM_LOAD16_BYTE( "mpr13426.13c", 0x600001, 0x80000, CRC(5cfb1e25) SHA1(1dd57475604f339e58bf946e17ae0dc5cf4a3dba) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13417.7c",   0x010000, 0x20000, CRC(cd7beb49) SHA1(2435ce000f1eefdd06b27ea93e22fd82c0e999d2) )
	/* these seem best 3 from the different sized dumps */
	ROM_LOAD( "mpr13420.6c",   0x090000, 0x40000, CRC(3de0dee4) SHA1(31833684df5a34d5e9ef04f2ab42355b8e9cbb45) )
	ROM_LOAD( "mpr13419.5c",   0x110000, 0x40000, CRC(e7021b0a) SHA1(82e390fac63965d4f80ae01758c19ae951c39475) )
	ROM_LOAD( "mpr13418.4c",   0x190000, 0x40000, CRC(0732594d) SHA1(9fbeae29f1a31d136ddc9a49c786b2a08a523e0d) )
ROM_END

/**************************************************************************************************************************
	Laser Ghost, Sega System 18
	CPU: FD1094 (317-0166)
	ROM Board: 171-5873B
*/
ROM_START( lghosta )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13429.4a", 0x000000, 0x40000, CRC(09bd65c0) SHA1(f2b332a86d52af3c5270f668bdd43a0d44eca346) )
	ROM_LOAD16_BYTE( "epr13430.6a", 0x000001, 0x40000, CRC(51009fe0) SHA1(f1e6e3714c01c15c0e932470a9e1a17abb59e958) )
	ROM_LOAD16_BYTE( "epr13411.5a", 0x200000, 0x40000, CRC(5160167b) SHA1(3d176a18c7527b1e485f10b144bb4db1b945e709) )
	ROM_LOAD16_BYTE( "epr13413.7a", 0x200001, 0x40000, CRC(656b3bd8) SHA1(db81d4ae3138308dce1e3db7a859f1d63c4ff815) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0166.key", 0x0000, 0x2000, CRC(8379961f) SHA1(44e0662e92ece65ad2049b2cd804f516e631166e) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13414.1c", 0x00000, 0x40000, CRC(dada2419) SHA1(f6ffd02d75232a09ea83fd199e5e30b2773b0cf5) )
	ROM_LOAD( "epr13415.2c", 0x40000, 0x40000, CRC(bbb62c48) SHA1(7a4c5bd11b73a92deece72b55627f48ac167acd6) )
	ROM_LOAD( "epr13416.3c", 0x80000, 0x40000, CRC(1d11dbae) SHA1(331aa49c6b38d32ec33184dbd0851888463ddbc7) )

	ROM_REGION16_BE( 0x800000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr13603.10a", 0x000000, 0x80000, CRC(5350a94e) SHA1(47e99803cab4b508feb51069c940d6c824d6961d) )
	ROM_LOAD16_BYTE( "epr13604.10c", 0x000001, 0x80000, CRC(4009c8e5) SHA1(97f513d312f4c90f8bffdf797afa3749779989a5) )
	ROM_LOAD16_BYTE( "epr13421.11a", 0x200000, 0x80000, CRC(2fc75890) SHA1(9f97f07dba3b978df8eb357894168ad74f151d30) )
	ROM_LOAD16_BYTE( "mpr13424.11c", 0x200001, 0x80000, CRC(fb98d920) SHA1(cebdebe88902e96c631df6053ac2589f794da155) )
	ROM_LOAD16_BYTE( "mpr13422.12a", 0x400000, 0x80000, CRC(48a0754d) SHA1(9fead9f8319593adb4bddaaa4d053b21ca726009) )
	ROM_LOAD16_BYTE( "mpr13425.12c", 0x400001, 0x80000, CRC(f8252589) SHA1(5a1ed24296d0609393e53df3ee585a366da4ee46) )
	ROM_LOAD16_BYTE( "mpr13423.13a", 0x600000, 0x80000, CRC(335bbc9d) SHA1(78793335b2f8a1bb05809259521db193c17c9b98) )
	ROM_LOAD16_BYTE( "mpr13426.13c", 0x600001, 0x80000, CRC(5cfb1e25) SHA1(1dd57475604f339e58bf946e17ae0dc5cf4a3dba) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13417.7c",   0x010000, 0x20000, CRC(cd7beb49) SHA1(2435ce000f1eefdd06b27ea93e22fd82c0e999d2) )
	/* these seem best 3 from the different sized dumps */
	ROM_LOAD( "mpr13420.6c",   0x090000, 0x40000, CRC(3de0dee4) SHA1(31833684df5a34d5e9ef04f2ab42355b8e9cbb45) )
	ROM_LOAD( "mpr13419.5c",   0x110000, 0x40000, CRC(e7021b0a) SHA1(82e390fac63965d4f80ae01758c19ae951c39475) )
	ROM_LOAD( "mpr13418.4c",   0x190000, 0x40000, CRC(0732594d) SHA1(9fbeae29f1a31d136ddc9a49c786b2a08a523e0d) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Moonwalker, Sega System 18
	CPU: FD1094 (317-0159)
	ROM Board: 171-5873B
*/
ROM_START( mwalk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom cpu 317-0159 */
	ROM_LOAD16_BYTE( "epr13235.a6", 0x000000, 0x40000, CRC(6983e129) SHA1(a8dd430620ab8ce11df46aa208d762d47f510464) )
	ROM_LOAD16_BYTE( "epr13234.a5", 0x000001, 0x40000, CRC(c9fd20f2) SHA1(9476e6481e6d8f223acd52f543fa04f408d48dc3) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0159.key", 0x0000, 0x2000, CRC(507838f0) SHA1(0c92d313da40b5dec7398c05b57698de6153b4b0) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, CRC(862d2c03) SHA1(3c5446d702a639b62a602c6d687f9875d8450218) )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, CRC(7d1ac3ec) SHA1(8495357304f1df135bba77ef3b96e79a883b8ff0) )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, CRC(56d3393c) SHA1(50a2d065060692c9ecaa56046a781cb21d93e554) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, CRC(c59f107b) SHA1(10fa60fca6e34eda277c483bb1c0e81bb88c8a47) )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, CRC(a5e96346) SHA1(a854f4dd5dc16975373255110fdb8ab3d121b1af) )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, CRC(364f60ff) SHA1(9ac887ec0b2e32b504b7c6a5f3bb1ce3fe41a15a) )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, CRC(9550091f) SHA1(bb6e898f7b540e130fd338c10f74609a7604cef4) )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, CRC(523df3ed) SHA1(2e496125e75decd674c3a08404fbdb53791a965d) )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, CRC(f40dc45d) SHA1(e9468cef428f52ecdf6837c6d9a9fea934e7676c) )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, CRC(9ae7546a) SHA1(5413b0131881b0b32bac8de51da9a299835014bb) )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, CRC(de3786be) SHA1(2279bb390aa3efab9aeee0a643e5cb6a4f5933b6) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, CRC(56c2e82b) SHA1(d5755a1bb6e889d274dc60e883d4d65f12fdc877) )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, CRC(19e2061f) SHA1(2dcf1718a43dab4da53b4f67722664e70ddd2169) )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, CRC(58d4d9ce) SHA1(725e73a656845b02702ef131b4c0aa2a73cdd02e) )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, CRC(623edc5d) SHA1(c32d9f818d40f311877fbe6532d9e95b6045c3c4) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* protection MCU */
	/* extra byte on the end is security info put there by the eprom reader software.... */
	/* not verified if mcu is the same on the other sets.. */
	ROM_LOAD( "315-5437.bin", 0x00000, 0x1001,  CRC(3e2aeb90) SHA1(f889a42cef9a8ddbfa888692b45b74c3cb8fa054) )
ROM_END

/**************************************************************************************************************************
	Moonwalker, Sega System 18
	CPU: FD1094 (317-0158)
	ROM Board: 171-5873B
*/
ROM_START( mwalka )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom cpu 317-0158 */
	ROM_LOAD16_BYTE( "epr13233", 0x000000, 0x40000, CRC(f3dac671) SHA1(cd9d372c7e272d2371bc1f9fb0167831c804423f) )
	ROM_LOAD16_BYTE( "epr13232", 0x000001, 0x40000, CRC(541d8bdf) SHA1(6a99153fddca246ba070e93c4bacd145f15f76bf) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0158.key", 0x0000, 0x2000, CRC(db6059e2) SHA1(f93aaa63e38381be8efc2a2dd00d1781abf788f4) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, CRC(862d2c03) SHA1(3c5446d702a639b62a602c6d687f9875d8450218) )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, CRC(7d1ac3ec) SHA1(8495357304f1df135bba77ef3b96e79a883b8ff0) )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, CRC(56d3393c) SHA1(50a2d065060692c9ecaa56046a781cb21d93e554) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, CRC(c59f107b) SHA1(10fa60fca6e34eda277c483bb1c0e81bb88c8a47) )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, CRC(a5e96346) SHA1(a854f4dd5dc16975373255110fdb8ab3d121b1af) )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, CRC(364f60ff) SHA1(9ac887ec0b2e32b504b7c6a5f3bb1ce3fe41a15a) )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, CRC(9550091f) SHA1(bb6e898f7b540e130fd338c10f74609a7604cef4) )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, CRC(523df3ed) SHA1(2e496125e75decd674c3a08404fbdb53791a965d) )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, CRC(f40dc45d) SHA1(e9468cef428f52ecdf6837c6d9a9fea934e7676c) )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, CRC(9ae7546a) SHA1(5413b0131881b0b32bac8de51da9a299835014bb) )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, CRC(de3786be) SHA1(2279bb390aa3efab9aeee0a643e5cb6a4f5933b6) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, CRC(56c2e82b) SHA1(d5755a1bb6e889d274dc60e883d4d65f12fdc877) )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, CRC(19e2061f) SHA1(2dcf1718a43dab4da53b4f67722664e70ddd2169) )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, CRC(58d4d9ce) SHA1(725e73a656845b02702ef131b4c0aa2a73cdd02e) )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, CRC(623edc5d) SHA1(c32d9f818d40f311877fbe6532d9e95b6045c3c4) )
ROM_END

/**************************************************************************************************************************
	Moonwalker, Sega System 18
	CPU: FD1094 (317-0157)
	ROM Board: 171-5873B
*/
ROM_START( mwalkb )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom cpu 317-0157 */
	ROM_LOAD16_BYTE( "ep13227.a6", 0x000000, 0x40000, CRC(6c0534b3) SHA1(23f35d1a15275cbc4b6d2f81f5634abac3832282) )
	ROM_LOAD16_BYTE( "ep13226.a5", 0x000001, 0x40000, CRC(99765854) SHA1(c00776c676b77fed4e94bb02f52f905c845ee73c) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0157.key", 0x0000, 0x2000, CRC(324d6931) SHA1(f8f4530a75aeeace1c8456da37118975c5c43316) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, CRC(862d2c03) SHA1(3c5446d702a639b62a602c6d687f9875d8450218) )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, CRC(7d1ac3ec) SHA1(8495357304f1df135bba77ef3b96e79a883b8ff0) )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, CRC(56d3393c) SHA1(50a2d065060692c9ecaa56046a781cb21d93e554) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, CRC(c59f107b) SHA1(10fa60fca6e34eda277c483bb1c0e81bb88c8a47) )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, CRC(a5e96346) SHA1(a854f4dd5dc16975373255110fdb8ab3d121b1af) )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, CRC(364f60ff) SHA1(9ac887ec0b2e32b504b7c6a5f3bb1ce3fe41a15a) )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, CRC(9550091f) SHA1(bb6e898f7b540e130fd338c10f74609a7604cef4) )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, CRC(523df3ed) SHA1(2e496125e75decd674c3a08404fbdb53791a965d) )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, CRC(f40dc45d) SHA1(e9468cef428f52ecdf6837c6d9a9fea934e7676c) )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, CRC(9ae7546a) SHA1(5413b0131881b0b32bac8de51da9a299835014bb) )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, CRC(de3786be) SHA1(2279bb390aa3efab9aeee0a643e5cb6a4f5933b6) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, CRC(56c2e82b) SHA1(d5755a1bb6e889d274dc60e883d4d65f12fdc877) )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, CRC(19e2061f) SHA1(2dcf1718a43dab4da53b4f67722664e70ddd2169) )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, CRC(58d4d9ce) SHA1(725e73a656845b02702ef131b4c0aa2a73cdd02e) )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, CRC(623edc5d) SHA1(c32d9f818d40f311877fbe6532d9e95b6045c3c4) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Shadow Dancer, Sega System 18
	CPU: 68000
	ROM Board: 171-5873B
*/
ROM_START( shdancer )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "shdancer.a6", 0x000000, 0x40000, CRC(3d5b3fa9) SHA1(370dd6e8ab9fb9e77eee9262d13fbdb4cf575abc) )
	ROM_LOAD16_BYTE( "shdancer.a5", 0x000001, 0x40000, CRC(2596004e) SHA1(1b993aa74e7559f7e99253fd2144db9449c04cce) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sd12712.bin", 0x00000, 0x40000, CRC(9bdabe3d) SHA1(4bb30fa2d4cdefe4a864cef7153b516bc5b02c42) )
	ROM_LOAD( "sd12713.bin", 0x40000, 0x40000, CRC(852d2b1c) SHA1(8e5bc83d45e48b621ea3016207f2028fe41701e6) )
	ROM_LOAD( "sd12714.bin", 0x80000, 0x40000, CRC(448226ce) SHA1(3060e4a43311069e2691d659c1e0c1a48edfeedb) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "sd12719.bin",  0x000001, 0x40000, CRC(d6888534) SHA1(2201f1921a68cf39e5a94b487c90e48d032d630f) )
	ROM_LOAD16_BYTE( "sd12726.bin",  0x000000, 0x40000, CRC(ff344945) SHA1(2743778c42f53321f9691d60bbf94ea8baf1382f) )
	ROM_LOAD16_BYTE( "sd12718.bin",  0x080001, 0x40000, CRC(ba2efc0c) SHA1(459a1a280f870c94aefb70127ed007cb090ed203) )
	ROM_LOAD16_BYTE( "sd12725.bin",  0x080000, 0x40000, CRC(268a0c17) SHA1(2756054fa3c3aed30a1fce5e41acb0ceaebe90b5) )
	ROM_LOAD16_BYTE( "sd12717.bin",  0x100001, 0x40000, CRC(c81cc4f8) SHA1(22f364e85057ceef533e051c8d0755b9691c5ec4) )
	ROM_LOAD16_BYTE( "sd12724.bin",  0x100000, 0x40000, CRC(0f4903dc) SHA1(851bd60e877c9e39be23dc1fde91efc9da513c29) )
	ROM_LOAD16_BYTE( "sd12716.bin",  0x180001, 0x40000, CRC(a870e629) SHA1(29f6633240f9737ec19e16100decc7aa045b2060) )
	ROM_LOAD16_BYTE( "sd12723.bin",  0x180000, 0x40000, CRC(c606cf90) SHA1(cb53ae9a6da1eb31c584173d1fbbd1c8539fb54c) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "sd12720.bin", 0x10000, 0x20000, CRC(7a0d8de1) SHA1(eca5e2104e5b3e772d083a718171234f06ea8a55) )
	ROM_LOAD( "sd12715.bin", 0x90000, 0x40000, CRC(07051a52) SHA1(d48658497f4a34665d3e051f893ff057c38925ae) )
ROM_END

/**************************************************************************************************************************
	Shadow Dancer, Sega System 18
	CPU: 68000
	ROM Board: 171-5873B
*/
ROM_START( shdancrj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sd12722b.bin", 0x000000, 0x40000, CRC(c00552a2) SHA1(74fddfe596bc00bc11c4a06e2103417e8fd334f6) )
	ROM_LOAD16_BYTE( "sd12721b.bin", 0x000001, 0x40000, CRC(653d351a) SHA1(1a03a154cb81a5a2f28c38aecdd6b5d107ea7ffa) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sd12712.bin",  0x00000, 0x40000, CRC(9bdabe3d) SHA1(4bb30fa2d4cdefe4a864cef7153b516bc5b02c42) )
	ROM_LOAD( "sd12713.bin",  0x40000, 0x40000, CRC(852d2b1c) SHA1(8e5bc83d45e48b621ea3016207f2028fe41701e6) )
	ROM_LOAD( "sd12714.bin",  0x80000, 0x40000, CRC(448226ce) SHA1(3060e4a43311069e2691d659c1e0c1a48edfeedb) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "sd12719.bin",  0x000001, 0x40000, CRC(d6888534) SHA1(2201f1921a68cf39e5a94b487c90e48d032d630f) )
	ROM_LOAD16_BYTE( "sd12726.bin",  0x000000, 0x40000, CRC(ff344945) SHA1(2743778c42f53321f9691d60bbf94ea8baf1382f) )
	ROM_LOAD16_BYTE( "sd12718.bin",  0x080001, 0x40000, CRC(ba2efc0c) SHA1(459a1a280f870c94aefb70127ed007cb090ed203) )
	ROM_LOAD16_BYTE( "sd12725.bin",  0x080000, 0x40000, CRC(268a0c17) SHA1(2756054fa3c3aed30a1fce5e41acb0ceaebe90b5) )
	ROM_LOAD16_BYTE( "sd12717.bin",  0x100001, 0x40000, CRC(c81cc4f8) SHA1(22f364e85057ceef533e051c8d0755b9691c5ec4) )
	ROM_LOAD16_BYTE( "sd12724.bin",  0x100000, 0x40000, CRC(0f4903dc) SHA1(851bd60e877c9e39be23dc1fde91efc9da513c29) )
	ROM_LOAD16_BYTE( "sd12716.bin",  0x180001, 0x40000, CRC(a870e629) SHA1(29f6633240f9737ec19e16100decc7aa045b2060) )
	ROM_LOAD16_BYTE( "sd12723.bin",  0x180000, 0x40000, CRC(c606cf90) SHA1(cb53ae9a6da1eb31c584173d1fbbd1c8539fb54c) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "sd12720.bin", 0x10000, 0x20000, CRC(7a0d8de1) SHA1(eca5e2104e5b3e772d083a718171234f06ea8a55) )
	ROM_LOAD( "sd12715.bin", 0x90000, 0x40000, CRC(07051a52) SHA1(d48658497f4a34665d3e051f893ff057c38925ae) )
ROM_END

/**************************************************************************************************************************
	Shadow Dancer, Sega System 18
	CPU: 68000
	ROM Board: 171-5873B
*/
ROM_START( shdancrb )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12772b.bin", 0x000000, 0x40000, CRC(6868a4d4) SHA1(f0d142c81fe1eba4f5c59a0163e25c80ccfe85d7) )
	ROM_LOAD16_BYTE( "epr12771b.bin", 0x000001, 0x40000, CRC(04e30c84) SHA1(6c5705f7de6ee1bd754b51988cc7d1008f817c78) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sd12712.bin",  0x00000, 0x40000, CRC(9bdabe3d) SHA1(4bb30fa2d4cdefe4a864cef7153b516bc5b02c42) )
	ROM_LOAD( "sd12713.bin",  0x40000, 0x40000, CRC(852d2b1c) SHA1(8e5bc83d45e48b621ea3016207f2028fe41701e6) )
	ROM_LOAD( "sd12714.bin",  0x80000, 0x40000, CRC(448226ce) SHA1(3060e4a43311069e2691d659c1e0c1a48edfeedb) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "sd12719.bin",  0x000001, 0x40000, CRC(d6888534) SHA1(2201f1921a68cf39e5a94b487c90e48d032d630f) )
	ROM_LOAD16_BYTE( "sd12726.bin",  0x000000, 0x40000, CRC(ff344945) SHA1(2743778c42f53321f9691d60bbf94ea8baf1382f) )
	ROM_LOAD16_BYTE( "sd12718.bin",  0x080001, 0x40000, CRC(ba2efc0c) SHA1(459a1a280f870c94aefb70127ed007cb090ed203) )
	ROM_LOAD16_BYTE( "sd12725.bin",  0x080000, 0x40000, CRC(268a0c17) SHA1(2756054fa3c3aed30a1fce5e41acb0ceaebe90b5) )
	ROM_LOAD16_BYTE( "sd12717.bin",  0x100001, 0x40000, CRC(c81cc4f8) SHA1(22f364e85057ceef533e051c8d0755b9691c5ec4) )
	ROM_LOAD16_BYTE( "sd12724.bin",  0x100000, 0x40000, CRC(0f4903dc) SHA1(851bd60e877c9e39be23dc1fde91efc9da513c29) )
	ROM_LOAD16_BYTE( "sd12716.bin",  0x180001, 0x40000, CRC(a870e629) SHA1(29f6633240f9737ec19e16100decc7aa045b2060) )
	ROM_LOAD16_BYTE( "sd12723.bin",  0x180000, 0x40000, CRC(c606cf90) SHA1(cb53ae9a6da1eb31c584173d1fbbd1c8539fb54c) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "sd12720.bin", 0x10000, 0x20000, CRC(7a0d8de1) SHA1(eca5e2104e5b3e772d083a718171234f06ea8a55) )
	ROM_LOAD( "sd12715.bin", 0x90000, 0x40000, CRC(07051a52) SHA1(d48658497f4a34665d3e051f893ff057c38925ae) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
	Where's Wally?, Sega System 18
	CPU: FD1094 317-0197
	ROM Board: 171-5873B
*/
ROM_START( wwally )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code - custom CPU 317-0197 */
	ROM_LOAD16_BYTE( "14731", 0x000000, 0x40000, CRC(6e3235b9) SHA1(11d5628644e8301550c36c93e5f137c67c11e735) )
	ROM_LOAD16_BYTE( "14730", 0x000001, 0x40000, CRC(e72bc17a) SHA1(ac3b7d86571a6f510c202735134c1bc4809aa26e) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	/* not dumped */

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14719", 0x000000, 0x80000, CRC(55202d05) SHA1(bf2f488f4ce8fa82a3c94fe165d371bc5d94d69c) )
	ROM_LOAD( "14720", 0x080000, 0x80000, CRC(5b86eee0) SHA1(9a03ad8833c8b436ab3d82939a39b46993fdcf9a) )
	ROM_LOAD( "14721", 0x100000, 0x80000, CRC(f185fe12) SHA1(e5a8d0a88bde97f9f23c114c9bd854341cd26e94) )

	ROM_REGION16_BE( 0x900000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14726", 0x000001, 0x100000, CRC(7213d1d3) SHA1(b70346a1dd3aa112bc696a7f4dd9ca908c3e5afa) )
	ROM_LOAD16_BYTE( "14732", 0x000000, 0x100000, CRC(04ced549) SHA1(59fd6510f0e14613d830ac6527f12ccc0b9351a5) )
	ROM_LOAD16_BYTE( "14727", 0x400001, 0x100000, CRC(3b74e0f0) SHA1(40432dbbbf75dae1e5e32b7cc2c4884f5e9e3bf5) )
	ROM_LOAD16_BYTE( "14733", 0x400000, 0x100000, CRC(6da0444f) SHA1(80c32895af19bda3277376fdbd1c163f0ed25665) )
	ROM_LOAD16_BYTE( "14728", 0x800001, 0x080000, CRC(5b921587) SHA1(2779dc658bb7a51f2399af5a6463ccb2dd388e88) )
	ROM_LOAD16_BYTE( "14734", 0x800000, 0x080000, CRC(6f3f5ed9) SHA1(01972c8bd5bfde58715bc0adc7dea73bf6350a26) )

	ROM_REGION( 0x210000, REGION_CPU2, ROMREGION_ERASEFF ) /* sound CPU */
	ROM_LOAD( "14725",	 0x010000, 0x20000, CRC(2b29684f) SHA1(b83962a4f475f9b3e79d4f7ff577b170c4043156) )
	ROM_LOAD( "14724",   0x090000, 0x80000, CRC(47cbea86) SHA1(c70d1fed2912c7c05223ce0bb0941706f957295f) )
	ROM_LOAD( "14723",   0x110000, 0x80000, CRC(bc5adc27) SHA1(09405002b940a3d3eb0f1258f37af51e0b7581b9) )
	ROM_LOAD( "14722",   0x190000, 0x80000, CRC(1bd081f8) SHA1(e5b0b5d8334486f813d7c430bb7fce3f69605a21) )
ROM_END



/*************************************
 *
 *	Generic driver initialization
 *
 *************************************/

static DRIVER_INIT( generic_shad )
{
	system18_generic_init(ROM_BOARD_171_SHADOW);
}

static DRIVER_INIT( generic_5874 )
{
	system18_generic_init(ROM_BOARD_171_5874);
}

static DRIVER_INIT( generic_5987 )
{
	system18_generic_init(ROM_BOARD_171_5987);
}



/*************************************
 *
 *	Game-specific driver inits
 *
 *************************************/

static DRIVER_INIT( mwalk )
{
	static UINT8 default_map[] = { 0x02,0x00,0x02,0x08,0x00,0xc0,0x00,0xff, 0x00,0x44,0x01,0x40,0x00,0x44,0x00,0xe4 };
	init_generic_5874();
	memcpy(&memory_control[0x20/2], default_map, 16);
	update_memory_mapping();
}

static DRIVER_INIT( ddcrew )
{
	init_generic_5987();
	custom_io_r = ddcrew_custom_io_r;
}

/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1990, astorm,   0,        system18, astorm,   generic_5874, ROT0, "Sega",    "Alien Storm (Japan, 2 Players set 1, FD1094 317-0146)" )
GAMEX(1990, astorma,  astorm,   system18, astorm,   generic_5874, ROT0, "Sega",    "Alien Storm (317-0148)", GAME_NOT_WORKING )
GAMEX(1990, astorm2p, astorm,   system18, astorm,   generic_5874, ROT0, "Sega",    "Alien Storm (2 Players set 2, 317-?)", GAME_NOT_WORKING )
GAMEX(1990, bloxeed,  0,        system18, generic,  generic_5874, ROT0, "Sega",    "Bloxeed", GAME_NOT_WORKING )
GAME( 1991, cltchitr, 0,        system18, generic,  generic_5987, ROT0, "Sega",    "Clutch Hitter (US, FD1094 317-176)" ) // decrypted
GAME( 1991, cltchtrj, cltchitr, system18, generic,  generic_5987, ROT0, "Sega",    "Clutch Hitter (Japan, FD1094 317-0175)" ) // decrypted
GAMEX(1991, ddcrew,   0,        system18, ddcrew,   ddcrew,       ROT0, "Sega",    "D. D. Crew (US, 4 Player, FD1094 317-0186)", GAME_NOT_WORKING ) // decrypted
GAMEX(1991, ddcrewa,  ddcrew,   system18, ddcrew,   ddcrew,       ROT0, "Sega",    "D. D. Crew (Europe, 4 Player, FD1094 317-?)", GAME_NOT_WORKING ) // not decrypted
GAMEX(1991, ddcrewb,  ddcrew,   system18, ddcrew,   ddcrew,       ROT0, "Sega",    "D. D. Crew (Europe, 2 Player, FD1094 317-0184)", GAME_NOT_WORKING ) // not decrypted
GAMEX(1991, ddcrewc,  ddcrew,   system18, ddcrew,   ddcrew,       ROT0, "Sega",    "D. D. Crew (Europe, 3 Player, FD1094 317-0187)", GAME_NOT_WORKING ) // not decrypted
GAMEX(1990, lghost,   0,        system18, generic,  generic_5987, ROT0, "Sega",    "Laser Ghost (US, 317-0165)", GAME_NOT_WORKING ) // decrypted
GAMEX(1990, lghosta,  lghost,   system18, generic,  generic_5987, ROT0, "Sega",    "Laser Ghost (317-0166)", GAME_NOT_WORKING ) // decrypted
GAMEX(1990, mwalk,    0,        system18, astorm,   mwalk,        ROT0, "Sega",    "Michael Jackson's Moonwalker (Set 1, 317-0159)", GAME_UNEMULATED_PROTECTION|GAME_NOT_WORKING ) // decrypted, but protected
GAMEX(1990, mwalka,   mwalk,    system18, astorm,   mwalk,        ROT0, "Sega",    "Michael Jackson's Moonwalker (Set 2, US, 317-0158)", GAME_UNEMULATED_PROTECTION|GAME_NOT_WORKING ) // decrypted, but protected
GAMEX(1990, mwalkb,   mwalk,    system18, astorm,   mwalk,        ROT0, "Sega",    "Michael Jackson's Moonwalker (Set 3, Japan, 317-0157)", GAME_UNEMULATED_PROTECTION|GAME_NOT_WORKING ) // decrypted, but protected
GAME( 1989, shdancer, 0,        system18, shdancer, generic_shad, ROT0, "Sega",    "Shadow Dancer (US)"  )
GAME( 1989, shdancrj, shdancer, system18, shdancer, generic_shad, ROT0, "Sega",    "Shadow Dancer (Japan)" )
GAME( 1989, shdancrb, shdancer, system18, shdancer, generic_shad, ROT0, "Sega",    "Shadow Dancer (Rev.B)" )
GAMEX(1992, wwally,   0,        system18, shdancer, generic_5987, ROT0, "Sega",    "Where's Wally?", GAME_NOT_WORKING )
