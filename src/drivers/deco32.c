/***************************************************************************

	Data East 32 bit ARM based games:

	Captain America
	Dragon Gun
	Fighter's History
	Locked 'N Loaded
	Tattoo Assassins

	Emulation by Bryan McPhail, mish@tendril.co.uk.  Thank you to Tim,
	Avedis and Stiletto for many things including work on Fighter's
	History protection and tracking down Tattoo Assassins!

	Captain America & Fighter's History - reset with both start buttons
	held down for test mode.  Reset with player 1 start held in Fighter's
	History for 'Pattern Editor'.

	Tattoo Assassins is a prototype, it is thought only 25 test units
	were manufactured and distributed to test arcades before the game
	was recalled.  TA is the only game developed by Data East Pinball 
	in USA, rather than Data East Corporation in Japan.

	Tattoo Assassins uses DE Pinball soundboard 520-5077-00 R


	Todo:

	Tattoo Assassins & Dragongun use an unemulated chip (Ace/Jack) for
	special blending effects.  It's exact effect is unclear.

	Video backgrounds in Dragongun and Lock N Load?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/arm/arm.h"
#include "cpu/h6280/h6280.h"
#include "cpu/m6809/m6809.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "machine/eeprom.h"
#include "deco32.h"

static data32_t *deco32_ram;
static int raster_enable,raster_offset;
static void *raster_irq_timer;

/**********************************************************************************/

static void interrupt_gen(int scanline)
{
	/* Save state of scroll registers before the IRQ */
	deco32_raster_display_list[deco32_raster_display_position++]=scanline;
	deco32_raster_display_list[deco32_raster_display_position++]=deco32_pf12_control[1]&0xffff;
	deco32_raster_display_list[deco32_raster_display_position++]=deco32_pf12_control[2]&0xffff;
	deco32_raster_display_list[deco32_raster_display_position++]=deco32_pf12_control[3]&0xffff;
	deco32_raster_display_list[deco32_raster_display_position++]=deco32_pf12_control[4]&0xffff;

	cpu_set_irq_line(0, ARM_IRQ_LINE, HOLD_LINE);
	timer_adjust(raster_irq_timer,TIME_NEVER,0,0);
}

static READ32_HANDLER( deco32_irq_controller_r )
{
	switch (offset) {
	case 2: /* Raster IRQ ACK - value read is not used */
		cpu_set_irq_line(0, ARM_IRQ_LINE, CLEAR_LINE);
		return 0;

	case 3: /* Irq controller

		Bit 0:  1 = Vblank active
		Bit 1:  ? (Hblank active?  Captain America raster IRQ waits for this to go low)
		Bit 2:
		Bit 3:
		Bit 4:	VBL Irq
		Bit 5:	Raster IRQ
		Bit 6:	Lightgun IRQ (on Lock N Load only)
		Bit 7:
		*/
		if (cpu_getvblank())
			return 0xffffff80 | 0x1 | 0x10; /* Assume VBL takes priority over possible raster/lightgun irq */

		return 0xffffff80 | cpu_getvblank() | (cpu_getiloops() ? 0x40 : 0x20);
//		return 0xffffff80 | cpu_getvblank() | (0x40); //test for lock load guns 
	}

	logerror("%08x: Unmapped IRQ read %08x (%08x)\n",activecpu_get_pc(),offset,mem_mask);
	return 0xffffffff;
}

static WRITE32_HANDLER( deco32_irq_controller_w )
{
	int scanline;

	switch (offset) {
	case 0: /* IRQ enable - probably an irq mask, but only values used are 0xc8 and 0xca */		
//		logerror("%08x:  IRQ write %d %08x\n",activecpu_get_pc(),offset,data);
		raster_enable=(data&0xff)==0xc8; /* 0xca seems to be off */
		break; 

	case 1: /* Raster IRQ scanline position, only valid for values between 1 & 239 (0 and 240-256 do NOT generate IRQ's) */
		scanline=(data&0xff)+raster_offset; /* Captain America seems to need (scanline-1), may be related to unemulated hblank? */
		if (raster_enable && scanline>0 && scanline<240)
			timer_adjust(raster_irq_timer,cpu_getscanlinetime(scanline),scanline,TIME_NEVER);
		else
			timer_adjust(raster_irq_timer,TIME_NEVER,0,0);
		break;
	case 2: /* VBL irq ack */
		break;
	}
}

static WRITE32_HANDLER( deco32_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

static READ32_HANDLER( deco32_71_r )
{
	/* Bit 0x80 goes high when sprite DMA is complete, and low
	while it's in progress, we don't bother to emulate it */
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
	return 0xffff0000 | readinputport(3);
}

static READ32_HANDLER( fghthist_control_r )
{
	switch (offset) {
	case 0: return 0xffff0000 | readinputport(0);
	case 1: return 0xffff0000 | readinputport(1); //check top bits??
	case 2: return 0xfffffffe | EEPROM_read_bit();
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( fghthist_eeprom_w )
{
	if (ACCESSING_LSB32) {
		EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x10);
		EEPROM_set_cs_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}

/**********************************************************************************/

static READ32_HANDLER( dragngun_service_r )
{
//	logerror("%08x:Read service\n",activecpu_get_pc());
	return readinputport(3);
}

static READ32_HANDLER( lockload_gun_mirror_r )
{
//logerror("%08x:Read gun %d\n",activecpu_get_pc(),offset);
//return ((rand()%0xffff)<<16) | rand()%0xffff;
	if (offset) /* Mirror of player 1 and player 2 fire buttons */
		return readinputport(5) | ((rand()%0xff)<<16);
	return readinputport(4) | readinputport(6) | (readinputport(6)<<16) | (readinputport(6)<<24); //((rand()%0xff)<<16);
}

static READ32_HANDLER( dragngun_prot_r )
{
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);

	static int strobe=0;
	if (!strobe) strobe=8;
	else strobe=0;

//definitely vblank in locked load

	switch (offset<<1) {
	case 0x140/2: return 0xffff0000 | readinputport(0); /* IN0 */
	case 0xadc/2: return 0xffff0000 | readinputport(1) | strobe; /* IN1 */
	case 0x6a0/2: return 0xffff0000 | readinputport(2); /* IN2 (Dip switch) */
	}
	return 0xffffffff;
}

static int dragngun_lightgun_port;

static READ32_HANDLER( dragngun_lightgun_r )
{
	/* Ports 0-3 are read, but seem unused */
	switch (dragngun_lightgun_port) {
	case 4: return readinputport(4); break;
	case 5: return readinputport(5); break;
	case 6: return readinputport(6); break;
	case 7: return readinputport(7); break;
	}

//	logerror("Illegal lightgun port %d read \n",dragngun_lightgun_port);
	return 0;
}

static WRITE32_HANDLER( dragngun_lightgun_w )
{
//	logerror("Lightgun port %d\n",dragngun_lightgun_port);
	dragngun_lightgun_port=offset;
}

static READ32_HANDLER( dragngun_eeprom_r )
{
	return 0xfffffffe | EEPROM_read_bit();
}

static WRITE32_HANDLER( dragngun_eeprom_w )
{
	if (ACCESSING_LSB32) {
		EEPROM_set_clock_line((data & 0x2) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x1);
		EEPROM_set_cs_line((data & 0x4) ? CLEAR_LINE : ASSERT_LINE);
		return;
	}
	logerror("%08x:Write control 1 %08x %08x\n",activecpu_get_pc(),offset,data);
}

static READ32_HANDLER(dragngun_oki_2_r)
{
	return OKIM6295_status_2_r(0);
}

static WRITE32_HANDLER(dragngun_oki_2_w)
{
	OKIM6295_data_2_w(0,data&0xff);
}

/**********************************************************************************/

static int tattass_eprom_bit;

static READ32_HANDLER( tattass_prot_r )
{
	switch (offset<<1) {
	case 0x280: return readinputport(0) << 16; /* IN0 */
	case 0x4c4: return readinputport(1) << 16; /* IN1 */
	case 0x35a: return tattass_eprom_bit << 16;
	}

	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);

	return 0xffffffff;
}

static WRITE32_HANDLER( tattass_prot_w )
{
	/* Only sound port of chip is used - no protection */
	if (offset==0x700/4) {
		/* 'Swap bits 0 and 3 to correct for design error from BSMT schematic' */
		int soundcommand = (data>>16)&0xff;
		soundcommand = BITSWAP8(soundcommand,7,6,5,4,0,2,1,3);
		soundlatch_w(0,soundcommand);
	}
}

static WRITE32_HANDLER( tattass_control_w )
{
	static int lastClock=0;
	static char buffer[32];
	static int bufPtr=0;
	static int pendingCommand=0; /* 1 = read, 2 = write */
	static int readBitCount=0;
	static int byteAddr=0;
	data8_t *eeprom=EEPROM_get_data_pointer(0);

	/* Eprom in low byte */
	if (mem_mask==0xffffff00) { /* Byte write to low byte only (different from word writing including low byte) */
		/* 
			The Tattoo Assassins eprom seems strange...  It's 1024 bytes in size, and 8 bit
			in width, but offers a 'multiple read' mode where a bit stream can be read
			starting at any byte boundary.
		
			Multiple read mode:
			Write 110aa000		[Read command, top two bits of address, 4 zeroes] 
			Write 00000000		[8 zeroes]
			Write aaaaaaaa		[Bottom 8 bits of address]

			Then bits are read back per clock, for as many bits as needed (NOT limited to byte
			boundaries).

			Write mode:
			Write 000aa000		[Write command, top two bits of address, 4 zeroes]
			Write 00000000		[8 zeroes]
			Write aaaaaaaa		[Bottom 8 bits of address]
			Write dddddddd		[8 data bits]

		*/
		if ((data&0x40)==0) {
			if (bufPtr) {
				int i;
				logerror("Eprom reset (bit count %d): ",readBitCount);
				for (i=0; i<bufPtr; i++)
					logerror("%s",buffer[i] ? "1" : "0");
				logerror("\n");

			}
			bufPtr=0;
			pendingCommand=0;
			readBitCount=0;
		}

		/* Eprom has been clocked */
		if (lastClock==0 && data&0x20 && data&0x40) {
			if (bufPtr>=32) {
				logerror("Eprom overflow!");
				bufPtr=0;
			}

			/* Handle pending read */
			if (pendingCommand==1) {
				int d=readBitCount/8;
				int m=7-(readBitCount%8);
				int a=(byteAddr+d)%1024;
				int b=eeprom[a];

				tattass_eprom_bit=(b>>m)&1;

				readBitCount++;
				lastClock=data&0x20;
				return;
			}

			/* Handle pending write */
			if (pendingCommand==2) {
				buffer[bufPtr++]=(data&0x10)>>4;

				if (bufPtr==32) {
					int b=(buffer[24]<<7)|(buffer[25]<<6)|(buffer[26]<<5)|(buffer[27]<<4)
						|(buffer[28]<<3)|(buffer[29]<<2)|(buffer[30]<<1)|(buffer[31]<<0);

					eeprom[byteAddr]=b;
				}
				lastClock=data&0x20;
				return;
			}

			buffer[bufPtr++]=(data&0x10)>>4;
			if (bufPtr==24) {
				/* Decode addr */
				byteAddr=(buffer[3]<<9)|(buffer[4]<<8)
						|(buffer[16]<<7)|(buffer[17]<<6)|(buffer[18]<<5)|(buffer[19]<<4)
						|(buffer[20]<<3)|(buffer[21]<<2)|(buffer[22]<<1)|(buffer[23]<<0);

				/* Check for read command */
				if (buffer[0] && buffer[1]) {
					tattass_eprom_bit=(eeprom[byteAddr]>>7)&1;
					readBitCount=1;
					pendingCommand=1;
				}

				/* Check for write command */
				else if (buffer[0]==0x0 && buffer[1]==0x0) {
					pendingCommand=2;
				}
				else {
					logerror("Detected unknown eprom command\n");
				}
			}

		} else {
			if (!(data&0x40)) {
				logerror("Cs set low\n");
				bufPtr=0;
			}
		}
	
		lastClock=data&0x20;
		return;
	}

	/* Volume in high byte */
	if (mem_mask==0xffff00ff) {
		//TODO:  volume attenuation == ((data>>8)&0xff);
		return;
	}

	/* Playfield control - Only written in full word memory accesses */
	deco32_pri_w(0,data&0x3,0); /* Bit 0 - layer priority toggle, Bit 1 - BG2/3 Joint mode (8bpp) */

	/* Sound board reset control */
	if (data&0x80)
		cpu_set_reset_line(1, CLEAR_LINE);
	else
		cpu_set_reset_line(1, ASSERT_LINE);

	/* bit 0x4 fade cancel? */
	/* bit 0x8 ?? */
	/* Bit 0x100 ?? */
	//logerror("%08x: %08x data\n",data,mem_mask);
}

/**********************************************************************************/

static MEMORY_READ32_START( captaven_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },

	{ 0x100000, 0x100007, deco32_71_r },
	{ 0x110000, 0x110fff, MRA32_RAM }, /* Sprites */
	{ 0x120000, 0x127fff, MRA32_RAM }, /* Main RAM */
	{ 0x130000, 0x131fff, MRA32_RAM }, /* Palette RAM */
	{ 0x128000, 0x128fff, captaven_prot_r },
	{ 0x148000, 0x14800f, deco32_irq_controller_r },
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
	{ 0x108000, 0x108003, MWA32_NOP }, /* ? */
	{ 0x110000, 0x110fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x120000, 0x127fff, MWA32_RAM, &deco32_ram }, /* Main RAM */
	{ 0x1280c8, 0x1280cb, deco32_sound_w },
	{ 0x130000, 0x131fff, deco32_nonbuffered_palette_w, &paletteram32 },
	{ 0x148000, 0x14800f, deco32_irq_controller_w },

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

	{ 0x200000, 0x200fff, deco32_fghthist_prot_r },
MEMORY_END

static MEMORY_WRITE32_START( fghthist_writemem )
	{ 0x000000, 0x001fff, deco32_pf1_data_w }, /* Hardware bug?  Test mode writes here and expects to get PF1 */
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },
	{ 0x12002c, 0x12002f, fghthist_eeprom_w },
	{ 0x1201fc, 0x1201ff, deco32_sound_w },
	{ 0x140000, 0x140003, MWA32_NOP }, /* VBL irq ack */
	//148000 - IRQ mask (ca)...
	{ 0x168000, 0x169fff, deco32_buffered_palette_w, &paletteram32 },
	{ 0x16c008, 0x16c00b, deco32_palette_dma_w },

	{ 0x178000, 0x178fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x179000, 0x179fff, MWA32_RAM, &spriteram32_2 }, // ?
	{ 0x17c010, 0x17c013, buffer_spriteram32_w },

	{ 0x182000, 0x183fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x184000, 0x185fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x192000, 0x192fff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x194000, 0x194fff, MWA32_RAM, &deco32_pf2_rowscroll },
	{ 0x1a0000, 0x1a001f, MWA32_RAM, &deco32_pf12_control },

	{ 0x1c2000, 0x1c3fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1c4000, 0x1c5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1d2000, 0x1d2fff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1d4000, 0x1d4fff, MWA32_RAM, &deco32_pf4_rowscroll },
	{ 0x1e0000, 0x1e001f, MWA32_RAM, &deco32_pf34_control },

	{ 0x200000, 0x200fff, deco32_fghthist_prot_w, &deco32_prot_ram },
MEMORY_END

static MEMORY_READ32_START( fghthsta_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x168000, 0x169fff, MRA32_RAM },

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

	{ 0x200000, 0x200fff, deco32_fghthist_prot_r },
MEMORY_END

static MEMORY_WRITE32_START( fghthsta_writemem )
	{ 0x000000, 0x001fff, deco32_pf1_data_w }, /* Hardware bug?  Test mode writes here and expects to get PF1 */
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },

	{ 0x140000, 0x140003, MWA32_NOP }, /* VBL irq ack */
	{ 0x150000, 0x150003, fghthist_eeprom_w }, /* Volume port/Eprom */

	{ 0x168000, 0x169fff, deco32_buffered_palette_w, &paletteram32 },
	{ 0x16c008, 0x16c00b, deco32_palette_dma_w },

	{ 0x178000, 0x179fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x17c010, 0x17c013, buffer_spriteram32_w },

	{ 0x182000, 0x183fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x184000, 0x185fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x192000, 0x192fff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x194000, 0x194fff, MWA32_RAM, &deco32_pf2_rowscroll },
	{ 0x1a0000, 0x1a001f, MWA32_RAM, &deco32_pf12_control },

	{ 0x1c2000, 0x1c3fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1c4000, 0x1c5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1d2000, 0x1d2fff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1d4000, 0x1d4fff, MWA32_RAM, &deco32_pf4_rowscroll },
	{ 0x1e0000, 0x1e001f, MWA32_RAM, &deco32_pf34_control },

	{ 0x200000, 0x200fff, deco32_fghthist_prot_w, &deco32_prot_ram },
MEMORY_END

static MEMORY_READ32_START( dragngun_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x120000, 0x120fff, dragngun_prot_r }, 
	{ 0x128000, 0x12800f, deco32_irq_controller_r },
	{ 0x130000, 0x131fff, MRA32_RAM },
	{ 0x138000, 0x138003, MRA32_NOP }, /* Palette dma complete in bit 0x8? ack?  return 0 else tight loop */

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

	{ 0x208000, 0x208fff, MRA32_RAM },
	{ 0x20c000, 0x20cfff, MRA32_RAM },
	{ 0x210000, 0x217fff, MRA32_RAM },
	{ 0x218000, 0x21ffff, MRA32_RAM },
	{ 0x220000, 0x221fff, MRA32_RAM }, /* Main spriteram */

	{ 0x204800, 0x204fff, MRA32_RAM }, //0x10 byte increments only
	{ 0x228000, 0x2283ff, MRA32_RAM }, //0x10 byte increments only

	{ 0x300000, 0x3fffff, MRA32_ROM },

	{ 0x400000, 0x400003, dragngun_oki_2_r },
	{ 0x420000, 0x420003, dragngun_eeprom_r },
	{ 0x438000, 0x438003, dragngun_lightgun_r },
	{ 0x440000, 0x440003, dragngun_service_r },
MEMORY_END

static MEMORY_WRITE32_START( dragngun_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },
	{ 0x1204c0, 0x1204c3, deco32_sound_w },
	{ 0x128000, 0x12800f, deco32_irq_controller_w },

	{ 0x130000, 0x131fff, deco32_buffered_palette_w, &paletteram32 },
	{ 0x138000, 0x138003, MWA32_NOP }, // palette mode?  check
	{ 0x138008, 0x13800b, deco32_palette_dma_w },

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

	{ 0x204800, 0x204fff, MWA32_RAM }, // ace? 0x10 byte increments only  // 13f ff stuff

	{ 0x208000, 0x208fff, MWA32_RAM, &dragngun_sprite_layout_0_ram },
	{ 0x20c000, 0x20cfff, MWA32_RAM, &dragngun_sprite_layout_1_ram },
	{ 0x210000, 0x217fff, MWA32_RAM, &dragngun_sprite_lookup_0_ram },
	{ 0x218000, 0x21ffff, MWA32_RAM, &dragngun_sprite_lookup_1_ram },
	{ 0x220000, 0x221fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x228000, 0x2283ff, MWA32_RAM }, /* ? */
	{ 0x230000, 0x230003, dragngun_spriteram_dma_w },

	{ 0x300000, 0x3fffff, MWA32_ROM },

	{ 0x400000, 0x400003, dragngun_oki_2_w },
	{ 0x410000, 0x410003, MWA32_NOP }, /* Some kind of serial bit-stream - digital volume control? */
	{ 0x420000, 0x420003, dragngun_eeprom_w },
	{ 0x430000, 0x43001f, dragngun_lightgun_w },
	{ 0x500000, 0x500003, dragngun_sprite_control_w },
MEMORY_END

static MEMORY_READ32_START( lockload_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x120000, 0x120fff, dragngun_prot_r }, 
	{ 0x128000, 0x12800f, deco32_irq_controller_r },
	{ 0x130000, 0x131fff, MRA32_RAM },
	{ 0x138000, 0x138003, MRA32_RAM }, //palette dma complete in bit 0x8? ack?  return 0 else tight loop

	{ 0x170000, 0x170007, lockload_gun_mirror_r }, /* Not on Dragongun */

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

	{ 0x208000, 0x208fff, MRA32_RAM },
	{ 0x20c000, 0x20cfff, MRA32_RAM },
	{ 0x210000, 0x217fff, MRA32_RAM },
	{ 0x218000, 0x21ffff, MRA32_RAM },
	{ 0x220000, 0x221fff, MRA32_RAM }, /* Main spriteram */

	{ 0x204800, 0x204fff, MRA32_RAM }, //0x10 byte increments only
	{ 0x228000, 0x2283ff, MRA32_RAM }, //0x10 byte increments only

	{ 0x300000, 0x3fffff, MRA32_ROM },

	{ 0x400000, 0x400003, dragngun_oki_2_r }, 
	{ 0x420000, 0x420003, dragngun_eeprom_r },
//	{ 0x438000, 0x438003, dragngun_lightgun_r },
	{ 0x440000, 0x440003, dragngun_service_r },
MEMORY_END


static MEMORY_WRITE32_START( lockload_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },
	{ 0x1204c0, 0x1204c3, deco32_sound_w },
	{ 0x128000, 0x12800f, deco32_irq_controller_w },

	{ 0x130000, 0x131fff, deco32_buffered_palette_w, &paletteram32 },
	{ 0x138000, 0x138003, MWA32_NOP }, // palette mode?  check
	{ 0x138008, 0x13800b, deco32_palette_dma_w },
	{ 0x178008, 0x17800f, MWA32_NOP }, /* Gun read ACK's */

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

	{ 0x204800, 0x204fff, MWA32_RAM }, // ace? 0x10 byte increments only  // 13f ff stuff

	{ 0x208000, 0x208fff, MWA32_RAM, &dragngun_sprite_layout_0_ram },
	{ 0x20c000, 0x20cfff, MWA32_RAM, &dragngun_sprite_layout_1_ram },
	{ 0x210000, 0x217fff, MWA32_RAM, &dragngun_sprite_lookup_0_ram },
	{ 0x218000, 0x21ffff, MWA32_RAM, &dragngun_sprite_lookup_1_ram },
	{ 0x220000, 0x221fff, MWA32_RAM, &spriteram32, &spriteram_size },

	{ 0x228000, 0x2283ff, MWA32_RAM },

	{ 0x230000, 0x230003, dragngun_spriteram_dma_w },

	{ 0x300000, 0x3fffff, MWA32_ROM },
	{ 0x400000, 0x400003, dragngun_oki_2_w },
	{ 0x420000, 0x420003, dragngun_eeprom_w },
//	{ 0x430000, 0x43001f, dragngun_lightgun_w },
	{ 0x500000, 0x500003, dragngun_sprite_control_w },
MEMORY_END

static MEMORY_READ32_START( tattass_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },
	{ 0x120000, 0x120003, MRA32_NOP }, /* ACIA (unused) */

	{ 0x162000, 0x162fff, MRA32_RAM }, /* 'Jack' RAM!? */
	{ 0x163000, 0x16307f, MRA32_RAM }, 
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
	{ 0x000000, 0x0f7fff, MWA32_ROM },
	{ 0x0f8000, 0x0fffff, MWA32_NOP }, /* Screen area on debug board? Cleared on startup */
	{ 0x100000, 0x11ffff, MWA32_RAM, &deco32_ram },

	{ 0x120000, 0x120003, MWA32_NOP }, /* ACIA (unused) */
	{ 0x130000, 0x130003, MWA32_NOP }, /* Coin port (unused?) */
	{ 0x140000, 0x140003, MWA32_NOP }, /* Vblank ack */
	{ 0x150000, 0x150003, tattass_control_w }, /* Volume port/Eprom/Priority */

	{ 0x162000, 0x162fff, MWA32_RAM }, /* 'Jack' RAM!? */
	{ 0x163000, 0x16307f, MWA32_RAM }, /* 'Ace' RAM!? */
	{ 0x163080, 0x16309f, MWA32_RAM }, /* 'Ace' control RAM!? */

	{ 0x164000, 0x164003, MWA32_NOP }, /* Palette control BG2/3 ($1a constant) */
	{ 0x164004, 0x164007, MWA32_NOP }, /* Palette control Obj1 ($6 constant) */
	{ 0x164008, 0x16400b, MWA32_NOP }, /* Palette control Obj2 ($5 constant) */
	{ 0x16400c, 0x16400f, MWA32_NOP }, 
	{ 0x168000, 0x169fff, deco32_buffered_palette_w, &paletteram32 },
	{ 0x16c000, 0x16c003, MWA32_NOP }, 
	{ 0x16c008, 0x16c00b, deco32_palette_dma_w },

	{ 0x170000, 0x171fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x174000, 0x174003, MWA32_NOP }, /* Sprite DMA mode (2) */
	{ 0x174010, 0x174013, buffer_spriteram32_w },
	{ 0x174018, 0x17401b, MWA32_NOP }, /* Sprite 'CPU' (unused) */

	{ 0x178000, 0x179fff, MWA32_RAM, &spriteram32_2, &spriteram_2_size },
	{ 0x17c000, 0x17c003, MWA32_NOP }, /* Sprite DMA mode (2) */
	{ 0x17c010, 0x17c013, buffer_spriteram32_2_w },
	{ 0x17c018, 0x17c01b, MWA32_NOP }, /* Sprite 'CPU' (unused) */

	{ 0x182000, 0x183fff, deco32_pf1_data_w, &deco32_pf1_data },
	{ 0x184000, 0x185fff, deco32_pf2_data_w, &deco32_pf2_data },
	{ 0x192000, 0x193fff, MWA32_RAM, &deco32_pf1_rowscroll },
	{ 0x194000, 0x195fff, MWA32_RAM, &deco32_pf2_rowscroll },
	{ 0x1a0000, 0x1a001f, MWA32_RAM, &deco32_pf12_control },

	{ 0x1c2000, 0x1c3fff, deco32_pf3_data_w, &deco32_pf3_data },
	{ 0x1c4000, 0x1c5fff, deco32_pf4_data_w, &deco32_pf4_data },
	{ 0x1d2000, 0x1d3fff, MWA32_RAM, &deco32_pf3_rowscroll },
	{ 0x1d4000, 0x1d5fff, MWA32_RAM, &deco32_pf4_rowscroll },
	{ 0x1e0000, 0x1e001f, MWA32_RAM, &deco32_pf34_control },

	{ 0x200000, 0x200fff, tattass_prot_w, &deco32_prot_ram },
MEMORY_END

/******************************************************************************/

static int bsmt_latch;

static WRITE_HANDLER(deco32_bsmt0_w) 
{ 
	bsmt_latch = data; 
}

static WRITE_HANDLER(deco32_bsmt1_w)
{
	BSMT2000_data_0_w(offset^ 0xff, ((bsmt_latch<<8)|data), 0);
	cpu_set_irq_line(1, M6809_IRQ_LINE, HOLD_LINE); /* BSMT is ready */
}

static READ_HANDLER(deco32_bsmt_status_r) 
{
	return 0x80; 
}

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
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem_tattass )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2002, 0x2003, soundlatch_r },
	{ 0x2006, 0x2007, deco32_bsmt_status_r },
	{ 0x2000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_tattass )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2001, MWA_NOP },	/* Reset BSMT? */
	{ 0x6000, 0x6000, deco32_bsmt0_w },
	{ 0xa000, 0xa0ff, deco32_bsmt1_w },
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
	
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( dragngun )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL ) //check  //test BUTTON F2
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

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED ) /* Would be a dipswitch, but only 1 present on board */
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Stage Select" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x0004, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN ) //check  //test BUTTON F2
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 20, 25, 0, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( lockload )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) //reset button??
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )  //service??
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL ) //check  //test BUTTON F2
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
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED ) /* Would be a dipswitch, but only 1 present on board */
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 ) //IPT_VBLANK )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x0004, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
//	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) //check  //test BUTTON F2
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* mirror of fire buttons */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 20, 25, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 20, 25, 0, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( tattass )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED ) /* 'soundmask' */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
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
//	{ 24, 16, 8, 0 },
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

static struct GfxLayout spritelayout2 =
{
	16,16,
	RGN_FRAC(1,5),
	5,
	{ 0x800000*8, 0x600000*8, 0x400000*8, 0x200000*8, 0 },
	{ //7,6,5,4,3,2,1,0,16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
0,1,2,3,4,5,6,7

	  },

	
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
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

static struct GfxDecodeInfo gfxdecodeinfo_tattass[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tilelayout,        256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tilelayout,        512, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &spritelayout2,    1536, 16 },	/* Sprites 16x16 */
	{ REGION_GFX4, 0, &spritelayout,     1024+256, 32 },	/* Sprites 16x16 */
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
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(42,MIXER_PAN_LEFT,42,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 35 }
};

static struct OKIM6295interface okim6295_3_interface =
{
	3,              /* 3 chips */
	{ 32220000/32/132, 32220000/16/132, 32220000/32/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2, REGION_SOUND3 },
	{ 35, 15, 35 }
};

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ 24000000 },
	{ 11 },
	{ REGION_SOUND1 },
	{ 100 }
};

static const UINT8 tattass_default_eprom[0x160] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x4a,0x45,0x4b,0x19,
	0x4e,0x4c,0x4b,0x14,0x4b,0x4a,0x4d,0x0f, 0x42,0x4c,0x53,0x0c,0x4a,0x57,0x43,0x0a,
	0x41,0x44,0x51,0x0a,0x4a,0x41,0x4b,0x09, 0x4b,0x52,0x54,0x08,0x4c,0x4f,0x4e,0x08,
	0x4c,0x46,0x53,0x07,0x53,0x4c,0x53,0x05, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x02,0x01,0x01,0x01, 0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,
	0x01,0x02,0x02,0xff,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x63,0x00,0x00,0x00, 0x02,0x03,0x00,0x03,0x00,0x00,0x00,0x02,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static struct EEPROM_interface eeprom_interface_tattass =
{
	10,				// address bits	10  ==> } 1024 byte eprom
	8,				// data bits	8
};

static NVRAM_HANDLER(tattass)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		int len;
		EEPROM_init(&eeprom_interface_tattass);	
		if (file) EEPROM_load(file);
		else memcpy(EEPROM_get_data_pointer(&len),tattass_default_eprom,0x160);
	}
}

/**********************************************************************************/

static MACHINE_INIT( deco32 )
{
	raster_irq_timer = timer_alloc(interrupt_gen);
}

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
	MDRV_CPU_ADD(ARM, 28000000/3)
	MDRV_CPU_MEMORY(captaven_readmem,captaven_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(deco32)
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
	MDRV_CPU_ADD(ARM, 28000000/3)
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

	MDRV_VIDEO_START(tattass)
	MDRV_VIDEO_UPDATE(fghthist)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fghthsta )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/3)
	MDRV_CPU_MEMORY(fghthsta_readmem,fghthsta_writemem)
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

	MDRV_VIDEO_START(tattass)
	MDRV_VIDEO_UPDATE(fghthist)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dragngun )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/2)
	MDRV_CPU_MEMORY(dragngun_readmem,dragngun_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(deco32)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_dragngun)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(dragngun)
	MDRV_VIDEO_UPDATE(dragngun)
	MDRV_VIDEO_EOF(dragngun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_3_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( lockload )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/2)
	MDRV_CPU_MEMORY(lockload_readmem,lockload_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,2) // From 2

	MDRV_CPU_ADD(H6280, 32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(deco32)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_dragngun)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(lockload)
	MDRV_VIDEO_UPDATE(dragngun)
	MDRV_VIDEO_EOF(dragngun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_3_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tattass )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM, 28000000/2) /* Unconfirmed */
	MDRV_CPU_MEMORY(tattass_readmem,tattass_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt,1)

	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem_tattass,sound_writemem_tattass)
	MDRV_CPU_PERIODIC_INT(tattass_snd_interrupt,489) /* Fixed FIRQ of 489Hz as measured on real (pinball) machine */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_NVRAM_HANDLER(tattass)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_tattass)
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

ROM_START( captavna )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hn_00.e1",	0x000000, 0x20000, 0x12dd0c71 )
	ROM_LOAD32_BYTE( "hn_01.h1",	0x000001, 0x20000, 0xac5ea492 )
	ROM_LOAD32_BYTE( "hn_02.k1",	0x000002, 0x20000, 0x0c5e13f6 )
	ROM_LOAD32_BYTE( "hn_03.l1",	0x000003, 0x20000, 0xbc050740 )
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

ROM_START( captavuu )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_BYTE( "hh-00.1e",	0x000000, 0x20000, 0xc34da654 )
	ROM_LOAD32_BYTE( "hh-01.1h",	0x000001, 0x20000, 0x55abe63f )
	ROM_LOAD32_BYTE( "hh-02.1k",	0x000002, 0x20000, 0x6096a9fb )
	ROM_LOAD32_BYTE( "hh-03.1m",	0x000003, 0x20000, 0x93631ded )
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

	ROM_REGION( 0x120000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dgma0.bin",  0x00000,  0x80000,  0xd0491a37 ) /* Encrypted tiles */
	ROM_LOAD( "dgma1.bin",  0x90000,  0x80000,  0xd5970365 )

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

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "dgadpcm2.bin", 0x000000, 0x80000,  0x3e006c6e )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "dgadpcm1.bin", 0x000000, 0x80000,  0xb9281dfd )

	ROM_REGION(0x80000, REGION_SOUND3, 0 )
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

ROM_START( fghthsta )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "le-00.1f", 0x000000, 0x80000, 0xa5c410eb )
	ROM_LOAD32_WORD( "le-01.2f", 0x000002, 0x80000, 0x7e148aa2 )
//	ROM_LOAD32_WORD( "kz00.out", 0x000000, 0x80000, 0x3a3dd5c )
//	ROM_LOAD32_WORD( "kz01.out", 0x000002, 0x80000, 0x86796d6 )

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

	ROM_REGION( 0x100000, REGION_GFX5, 0 ) /* Video data - same as Dragongun, probably leftover from a conversion */
//	ROM_LOAD( "dgma17.bin",  0x00000,  0x100000,  0x7799ed23 ) /* Todo - fix filenames */
//	ROM_LOAD( "dgma18.bin",  0x00000,  0x100000,  0xded66da9 )
//	ROM_LOAD( "dgma19.bin",  0x00000,  0x100000,  0xbdd1ed20 )
//	ROM_LOAD( "dgma20.bin",  0x00000,  0x100000,  0xfa0462f0 )
//	ROM_LOAD( "dgma21.bin",  0x00000,  0x100000,  0x2d0a28ae )
//	ROM_LOAD( "dgma22.bin",  0x00000,  0x100000,  0xc85f3559 )
//	ROM_LOAD( "dgma23.bin",  0x00000,  0x100000,  0xba907d6a )
//	ROM_LOAD( "dgma24.bin",  0x00000,  0x100000,  0x5cec45c8 )
//	ROM_LOAD( "dgma25.bin",  0x00000,  0x100000,  0xd65d895c )
//	ROM_LOAD( "dgma26.bin",  0x00000,  0x100000,  0x246a06c5 )
//	ROM_LOAD( "dgma27.bin",  0x00000,  0x100000,  0x3fcbd10f )
//	ROM_LOAD( "dgma28.bin",  0x00000,  0x100000,  0x5a2ec71d )

	ROM_REGION(0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "mbm-06.n17",  0x00000, 0x100000,  0xf34d5999 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "mbm-07.n21",  0x00000, 0x80000,  0x414f3793 )

	ROM_REGION(0x80000, REGION_SOUND3, 0 )
	ROM_LOAD( "mar-07.n19",  0x00000, 0x80000,  0x40287d62 )	// same as dragngun, unused?
ROM_END

ROM_START( tattass )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "pp44.cpu", 0x000000, 0x80000, 0xc3ca5b49 )
	ROM_LOAD32_WORD( "pp45.cpu", 0x000002, 0x80000, 0xd3f30de0 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "u7.snd",  0x00000, 0x10000,  0x6947be8a )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "abak_b01.s02",  0x000000, 0x80000,  0xbc805680 )
	ROM_LOAD16_BYTE( "abak_b01.s13",  0x000001, 0x80000,  0x350effcd )
	ROM_LOAD16_BYTE( "abak_b23.s02",  0x100000, 0x80000,  0x91abdc21 )
	ROM_LOAD16_BYTE( "abak_b23.s13",  0x100001, 0x80000,  0x80eb50fe )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bbak_b01.s02",  0x000000, 0x80000,  0x611be9a6 )
	ROM_LOAD16_BYTE( "bbak_b01.s13",  0x000001, 0x80000,  0x097e0604 )
	ROM_LOAD16_BYTE( "bbak_b23.s02",  0x100000, 0x80000,  0x3836531a )
	ROM_LOAD16_BYTE( "bbak_b23.s13",  0x100001, 0x80000,  0x1210485a )

	ROM_REGION( 0xa00000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ob1_c0.b0",  0x000000, 0x80000,  0x053fecca )
	ROM_LOAD( "ob1_c1.b0",  0x200000, 0x80000,  0xe183e6bc )
	ROM_LOAD( "ob1_c2.b0",  0x400000, 0x80000,  0x1314f828 )
	ROM_LOAD( "ob1_c3.b0",  0x600000, 0x80000,  0xc63866df )
	ROM_LOAD( "ob1_c4.b0",  0x800000, 0x80000,  0xf71cdd1b )

	ROM_LOAD( "ob1_c0.b1",  0x080000, 0x80000,  0x385434b0 )
	ROM_LOAD( "ob1_c1.b1",  0x280000, 0x80000,  0x0a3ec489 )
	ROM_LOAD( "ob1_c2.b1",  0x480000, 0x80000,  0x52f06081 )
	ROM_LOAD( "ob1_c3.b1",  0x680000, 0x80000,  0xa8a5cfbe )
	ROM_LOAD( "ob1_c4.b1",  0x880000, 0x80000,  0x09d0acd6 )

	ROM_LOAD( "ob1_c0.b2",  0x100000, 0x80000,  0x946e9f59 )
	ROM_LOAD( "ob1_c1.b2",  0x300000, 0x80000,  0x9f66ad54 )
	ROM_LOAD( "ob1_c2.b2",  0x500000, 0x80000,  0xa8df60eb )
	ROM_LOAD( "ob1_c3.b2",  0x700000, 0x80000,  0xa1a753be )
	ROM_LOAD( "ob1_c4.b2",  0x900000, 0x80000,  0xb65b3c4b )

	ROM_LOAD( "ob1_c0.b3",  0x180000, 0x80000,  0xcbbbc696 )
	ROM_LOAD( "ob1_c1.b3",  0x380000, 0x80000,  0xf7b1bdee )
	ROM_LOAD( "ob1_c2.b3",  0x580000, 0x80000,  0x97815619 )
	ROM_LOAD( "ob1_c3.b3",  0x780000, 0x80000,  0xfc3ccb7a )
	ROM_LOAD( "ob1_c4.b3",  0x980000, 0x80000,  0xdfdfd0ff )

	ROM_REGION( 0x800000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ob2_c0.b0",  0x000001, 0x80000,  0x9080ebe4 )
	ROM_LOAD32_BYTE( "ob2_c1.b0",  0x000003, 0x80000,  0xc0464970 )
	ROM_LOAD32_BYTE( "ob2_c2.b0",  0x000000, 0x80000,  0x35a2e621 )
	ROM_LOAD32_BYTE( "ob2_c3.b0",  0x000002, 0x80000,  0x99c7cc2d )
	ROM_LOAD32_BYTE( "ob2_c0.b1",  0x200001, 0x80000,  0x2c2c15c9 )
	ROM_LOAD32_BYTE( "ob2_c1.b1",  0x200003, 0x80000,  0xd2c49a14 )
	ROM_LOAD32_BYTE( "ob2_c2.b1",  0x200000, 0x80000,  0xfbe957e8 )
	ROM_LOAD32_BYTE( "ob2_c3.b1",  0x200002, 0x80000,  0xd7238829 )
	ROM_LOAD32_BYTE( "ob2_c0.b2",  0x400001, 0x80000,  0xaefa1b01 )
	ROM_LOAD32_BYTE( "ob2_c1.b2",  0x400003, 0x80000,  0x4af620ca )
	ROM_LOAD32_BYTE( "ob2_c2.b2",  0x400000, 0x80000,  0x8e58be07 )
	ROM_LOAD32_BYTE( "ob2_c3.b2",  0x400002, 0x80000,  0x1b5188c5 )
	ROM_LOAD32_BYTE( "ob2_c0.b3",  0x600001, 0x80000,  0xa2a5dafd )
	ROM_LOAD32_BYTE( "ob2_c1.b3",  0x600003, 0x80000,  0x6f0afd05 )
	ROM_LOAD32_BYTE( "ob2_c2.b3",  0x600000, 0x80000,  0x90fe5f4f )
	ROM_LOAD32_BYTE( "ob2_c3.b3",  0x600002, 0x80000,  0xe3517e6e )

	ROM_REGION(0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "u17.snd",  0x000000, 0x80000,  0xb945c18d )
	ROM_LOAD( "u21.snd",  0x080000, 0x80000,  0x10b2110c )
	ROM_LOAD( "u36.snd",  0x100000, 0x80000,  0x3b73abe2 )
	ROM_LOAD( "u37.snd",  0x180000, 0x80000,  0x986066b5 )
ROM_END

ROM_START( tattassa )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* ARM 32 bit code */
	ROM_LOAD32_WORD( "rev232a.000", 0x000000, 0x80000, 0x1a357112 )
	ROM_LOAD32_WORD( "rev232a.001", 0x000002, 0x80000, 0x550245d4 )
 
	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "u7.snd",  0x00000, 0x10000,  0x6947be8a )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "abak_b01.s02",  0x000000, 0x80000,  0xbc805680 )
	ROM_LOAD16_BYTE( "abak_b01.s13",  0x000001, 0x80000,  0x350effcd )
	ROM_LOAD16_BYTE( "abak_b23.s02",  0x100000, 0x80000,  0x91abdc21 )
	ROM_LOAD16_BYTE( "abak_b23.s13",  0x100001, 0x80000,  0x80eb50fe )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bbak_b01.s02",  0x000000, 0x80000,  0x611be9a6 )
	ROM_LOAD16_BYTE( "bbak_b01.s13",  0x000001, 0x80000,  0x097e0604 )
	ROM_LOAD16_BYTE( "bbak_b23.s02",  0x100000, 0x80000,  0x3836531a )
	ROM_LOAD16_BYTE( "bbak_b23.s13",  0x100001, 0x80000,  0x1210485a )

	ROM_REGION( 0xa00000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ob1_c0.b0",  0x000000, 0x80000,  0x053fecca )
	ROM_LOAD( "ob1_c1.b0",  0x200000, 0x80000,  0xe183e6bc )
	ROM_LOAD( "ob1_c2.b0",  0x400000, 0x80000,  0x1314f828 )
	ROM_LOAD( "ob1_c3.b0",  0x600000, 0x80000,  0xc63866df )
	ROM_LOAD( "ob1_c4.b0",  0x800000, 0x80000,  0xf71cdd1b )

	ROM_LOAD( "ob1_c0.b1",  0x080000, 0x80000,  0x385434b0 )
	ROM_LOAD( "ob1_c1.b1",  0x280000, 0x80000,  0x0a3ec489 )
	ROM_LOAD( "ob1_c2.b1",  0x480000, 0x80000,  0x52f06081 )
	ROM_LOAD( "ob1_c3.b1",  0x680000, 0x80000,  0xa8a5cfbe )
	ROM_LOAD( "ob1_c4.b1",  0x880000, 0x80000,  0x09d0acd6 )

	ROM_LOAD( "ob1_c0.b2",  0x100000, 0x80000,  0x946e9f59 )
	ROM_LOAD( "ob1_c1.b2",  0x300000, 0x80000,  0x9f66ad54 )
	ROM_LOAD( "ob1_c2.b2",  0x500000, 0x80000,  0xa8df60eb )
	ROM_LOAD( "ob1_c3.b2",  0x700000, 0x80000,  0xa1a753be )
	ROM_LOAD( "ob1_c4.b2",  0x900000, 0x80000,  0xb65b3c4b )

	ROM_LOAD( "ob1_c0.b3",  0x180000, 0x80000,  0xcbbbc696 )
	ROM_LOAD( "ob1_c1.b3",  0x380000, 0x80000,  0xf7b1bdee )
	ROM_LOAD( "ob1_c2.b3",  0x580000, 0x80000,  0x97815619 )
	ROM_LOAD( "ob1_c3.b3",  0x780000, 0x80000,  0xfc3ccb7a )
	ROM_LOAD( "ob1_c4.b3",  0x980000, 0x80000,  0xdfdfd0ff )

	ROM_REGION( 0x800000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ob2_c0.b0",  0x000001, 0x80000,  0x9080ebe4 )
	ROM_LOAD32_BYTE( "ob2_c1.b0",  0x000003, 0x80000,  0xc0464970 )
	ROM_LOAD32_BYTE( "ob2_c2.b0",  0x000000, 0x80000,  0x35a2e621 )
	ROM_LOAD32_BYTE( "ob2_c3.b0",  0x000002, 0x80000,  0x99c7cc2d )
	ROM_LOAD32_BYTE( "ob2_c0.b1",  0x200001, 0x80000,  0x2c2c15c9 )
	ROM_LOAD32_BYTE( "ob2_c1.b1",  0x200003, 0x80000,  0xd2c49a14 )
	ROM_LOAD32_BYTE( "ob2_c2.b1",  0x200000, 0x80000,  0xfbe957e8 )
	ROM_LOAD32_BYTE( "ob2_c3.b1",  0x200002, 0x80000,  0xd7238829 )
	ROM_LOAD32_BYTE( "ob2_c0.b2",  0x400001, 0x80000,  0xaefa1b01 )
	ROM_LOAD32_BYTE( "ob2_c1.b2",  0x400003, 0x80000,  0x4af620ca )
	ROM_LOAD32_BYTE( "ob2_c2.b2",  0x400000, 0x80000,  0x8e58be07 )
	ROM_LOAD32_BYTE( "ob2_c3.b2",  0x400002, 0x80000,  0x1b5188c5 )
	ROM_LOAD32_BYTE( "ob2_c0.b3",  0x600001, 0x80000,  0xa2a5dafd )
	ROM_LOAD32_BYTE( "ob2_c1.b3",  0x600003, 0x80000,  0x6f0afd05 )
	ROM_LOAD32_BYTE( "ob2_c2.b3",  0x600000, 0x80000,  0x90fe5f4f )
	ROM_LOAD32_BYTE( "ob2_c3.b3",  0x600002, 0x80000,  0xe3517e6e )

	ROM_REGION(0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "u17.snd",  0x000000, 0x80000,  0xb945c18d )
	ROM_LOAD( "u21.snd",  0x080000, 0x80000,  0x10b2110c )
	ROM_LOAD( "u36.snd",  0x100000, 0x80000,  0x3b73abe2 )
	ROM_LOAD( "u37.snd",  0x180000, 0x80000,  0x986066b5 )
ROM_END

ROM_START( nslasher )
	ROM_REGION(0x100000, REGION_CPU1, 0 ) /* Encrypted ARM 32 bit code */
	ROM_LOAD32_WORD( "mainprg.1f", 0x000000, 0x80000, 0x507acbae )
	ROM_LOAD32_WORD( "mainprg.2f", 0x000002, 0x80000, 0x931fc7ee )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "sndprg.17l",  0x00000,  0x10000,  0x18939e92 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbh-00.8c",  0x000000,  0x200000,  0xa877f8a3 ) /* Encrypted tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbh-01.9c",  0x000000,  0x200000,  0x1853dafc ) /* Encrypted tiles */

	ROM_REGION( 0xc00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "mbh-02.14c",  0x000001,  0x200000,  0xb2f158a1 )
	ROM_LOAD16_BYTE( "mbh-04.16c",  0x000000,  0x200000,  0xeecfe06d )
	ROM_LOAD16_BYTE( "mbh-03.15c",  0x400001,  0x80000,  0x787787e3 )
	ROM_LOAD16_BYTE( "mbh-05.17c",  0x400000,  0x80000,  0x1d2b7c17 )
	ROM_LOAD16_BYTE( "mbh-06.18c",  0xa00000,  0x100000,  0x038c2127 )
	ROM_LOAD16_BYTE( "mbh-07.19c",  0xb00000,  0x40000,  0xbbd22323 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "mbh-08.16e",  0x000001,  0x80000,  0xcdd7f8cb )
	ROM_LOAD16_BYTE( "mbh-09.18e",  0x000000,  0x80000,  0x33fa2121 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "mbh-10.14l", 0x000000,  0x80000,  0xc4d6b116 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "mbh-11.16l", 0x000000,  0x80000,  0x0ec40b6b )
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
		//logerror("%08x (%08x): CPU Spin - %d cycles left this frame ran %d (%d)\n",activecpu_get_pc(),ret,cycles_left_to_run(),cycles_currently_ran(),cycles_left_to_run()+cycles_currently_ran());
		cpu_spinuntil_int();
	}

	return ret;
}

static READ32_HANDLER( tattass_skip )
{
	int left=cycles_left_to_run();
	data32_t ret=deco32_ram[0];

	if (activecpu_get_pc()==0x1c5ec && left>32) {
		//logerror("%08x (%08x): CPU Spin - %d cycles left this frame ran %d (%d)\n",activecpu_get_pc(),ret,cycles_left_to_run(),cycles_currently_ran(),cycles_left_to_run()+cycles_currently_ran());
		cpu_spinuntil_int();
	}

	return ret;
}

/**********************************************************************************/

static DRIVER_INIT( captaven )
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);

	raster_offset=-1;
	install_mem_read32_handler(0, 0x12748c, 0x12748f, captaven_skip);
}

static DRIVER_INIT( dragngun )
{
	data32_t *ROM = (UINT32 *)memory_region(REGION_CPU1);
	const data8_t *SRC_RAM = memory_region(REGION_GFX1);
	data8_t *DST_RAM = memory_region(REGION_GFX2);

	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);

	memcpy(DST_RAM+0x80000,SRC_RAM,0x10000);
	memcpy(DST_RAM+0x110000,SRC_RAM+0x10000,0x10000);

	ROM[0x1b32c/4]=0xe1a00000;//  NOP test switch lock

	raster_offset=0;
	install_mem_read32_handler(0, 0x11f15c, 0x11f15f, dragngun_skip);
}

static DRIVER_INIT( fghthist )
{
	deco56_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
}

static DRIVER_INIT( lockload )
{
	data8_t *RAM = memory_region(REGION_CPU1);
//	data32_t *ROM = (UINT32 *)memory_region(REGION_CPU1);

	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);

	raster_offset=0;
	memcpy(RAM+0x300000,RAM+0x100000,0x100000);
	memset(RAM+0x100000,0,0x100000);

//	ROM[0x3fe3c0/4]=0xe1a00000;//  NOP test switch lock
//	ROM[0x3fe3cc/4]=0xe1a00000;//  NOP test switch lock
//	ROM[0x3fe40c/4]=0xe1a00000;//  NOP test switch lock
}

static DRIVER_INIT( tattass )
{
	data8_t *RAM = memory_region(REGION_GFX1);
	data8_t *tmp = (data8_t *)malloc(0x80000);

	/* Reorder bitplanes to make decoding easier */
	memcpy(tmp,RAM+0x80000,0x80000);
	memcpy(RAM+0x80000,RAM+0x100000,0x80000);
	memcpy(RAM+0x100000,tmp,0x80000);
	
	RAM = memory_region(REGION_GFX2);
	memcpy(tmp,RAM+0x80000,0x80000);
	memcpy(RAM+0x80000,RAM+0x100000,0x80000);
	memcpy(RAM+0x100000,tmp,0x80000);

	free(tmp);

	deco56_decrypt(REGION_GFX1); /* 141 */
	deco56_decrypt(REGION_GFX2); /* 141 */

	install_mem_read32_handler(0, 0x100000, 0x100003, tattass_skip);
}

static DRIVER_INIT( nslasher )
{
	data8_t *RAM = memory_region(REGION_GFX1);
	data8_t *tmp = (data8_t *)malloc(0x80000);

	/* Reorder bitplanes to make decoding easier */
	memcpy(tmp,RAM+0x80000,0x80000);
	memcpy(RAM+0x80000,RAM+0x100000,0x80000);
	memcpy(RAM+0x100000,tmp,0x80000);
	
	RAM = memory_region(REGION_GFX2);
	memcpy(tmp,RAM+0x80000,0x80000);
	memcpy(RAM+0x80000,RAM+0x100000,0x80000);
	memcpy(RAM+0x100000,tmp,0x80000);

	free(tmp);

	deco56_decrypt(REGION_GFX1); /* 141 */
	deco74_decrypt(REGION_GFX2);

	/* The board for Night Slashers is very close to the Fighter's History and
	Tattoo Assassins boards, but has an encrypted ARM cpu. */
}

/**********************************************************************************/

GAME( 1991, captaven, 0,        captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (Asia Rev 1.9)" )
GAME( 1991, captavna, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (Asia Rev 1.0)" )
GAME( 1991, captavne, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (UK Rev 1.4)" )
GAME( 1991, captavnu, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (US Rev 1.9)" )
GAME( 1991, captavuu, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (US Rev 1.6)" )
GAME( 1991, captavnj, captaven, captaven, captaven, captaven, ROT0, "Data East Corporation", "Captain America and The Avengers (Japan Rev 0.2)" )
GAMEX(1993, dragngun, 0,        dragngun, dragngun, dragngun, ROT0, "Data East Corporation", "Dragon Gun (US)", GAME_IMPERFECT_GRAPHICS  )
GAMEX(1993, fghthist, 0,        fghthist, fghthist, fghthist, ROT0, "Data East Corporation", "Fighter's History (US)", GAME_UNEMULATED_PROTECTION )
GAMEX(1993, fghthstw, fghthist, fghthist, fghthist, fghthist, ROT0, "Data East Corporation", "Fighter's History (World)", GAME_UNEMULATED_PROTECTION )
GAMEX(1993, fghthsta, fghthist, fghthsta, fghthist, fghthist, ROT0, "Data East Corporation", "Fighter's History (US Alternate Hardware)", GAME_UNEMULATED_PROTECTION )
GAMEX(1994, lockload, 0,        lockload, lockload, lockload, ROT0, "Data East Corporation", "Locked 'n Loaded (US)", GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )
GAMEX(1994, tattass,  0,        tattass,  tattass,  tattass,  ROT0, "Data East Pinball",     "Tattoo Assassins (US Prototype)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1994, tattassa, tattass,  tattass,  tattass,  tattass,  ROT0, "Data East Pinball",     "Tattoo Assassins (Asia Prototype)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1994, nslasher, 0,        tattass,  tattass,  nslasher, ROT0, "Data East Corporation", "Night Slashers", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)
