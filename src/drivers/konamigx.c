/**************************************************************************
 *
 * konamigx.c - Konami System GX
 * Driver by R. Belmont and Phil Stroffolino.
 * ESC protection emulation and TMS57002 skipper by Olivier Galibert.
 *
 * Basic hardware consists of:
 * - MC68EC020 main CPU at 24 MHz
 * - MC68000 sound CPU with 2x K054539 PCM chips plus a TMS57002 DASP for effects
 *
 * Tilemaps are handled by the old familiar K054156 with a new partner K056832.
 * This combination is mostly compatible with the 156/157 combo but now sports
 * up to 8 bits per pixel as well as larger 128x64 tile planes.
 *
 * Sprites are handled by a K053246 combined with a K055673.  This combo was
 * actually used on some 16 bit Konami games as well.  It appears identical
 * to the standard 246/247 combo except with 5-8 bits per pixel.
 *
 * Video output is handled by a K055555 mixer/priority encoder.  This is a much
 * more capable beast than the earlier 53251, supporting a background gradient
 * and flexible priority handling among other features.  It's combined with the
 * 54338 alpha blending engine first seen in Xexex for flexible blending.
 *
 * There are actually 4 types of System GX hardware, which are differentiated
 * by their ROM board.  The most common is the "Type 2".
 *
 * 68020 memory map for Type 2:
 * 000000: BIOS ROM (128k)
 * 200000: game program ROM (1 meg, possibly up to 2)
 * 400000: data ROMs
 * c00000: Work RAM (128k)
 * cc0000: Protection chip
 * d00000: 054157 ROM readback for memory test
 * d20000: sprite RAM (4k)
 * d40000: 054157/056832 tilemap generator  (VACSET)
 * d44000: tile bank selectors		    (VSCSET)
 * d48000: 053246/055673 sprite generator   (OBJSET1)
 * d4a000: more readback for sprite generator (OBJSET2)
 * d4c000: CCU1 registers                    (CCUS1)
 * 00: HCH			  6M/288   8M/384   12M/576	       224 256
 * 02: HCL		   HCH 	    01      01        02	  VCH  01  01
 * 04: HFPH		   HCL      7f      ff        ff	  VCL  07  20
 * 06: HFPL		   HFPH	    00      00        00	  VFP  11  0c
 * 08: HBPH		   HFPL	    10      19        23	  VBP  0e  0e
 * 0A: HBPL		   HBPH	    00      00        00	  VSW  07  05
 * 10: VCH		   HBPL	    30      3f        4d
 * 12: VCL		   HSW	    03      04        09
 * 14: VFP
 * 16: VBP
 * 18: VSW/HSW
 * 1A: INT TIME
 * 1C: INT1ACK (read VCTH) INT1 = V blank
 * 1E: INT2ACK (read VCTL) INT2 = H blank
 * d4e000: CCU2 registers		    (CCUS2)
 * d50000: K055555 8-bit-per-pixel priority encoder (PCUCS)
 * d52000: shared RAM with audio 68000      (SOUNDCS)
 * d56000: EEPROM comms, bit 7 is watchdog, bit 5 is frame?? (WRPOR1)
 * d56001: IRQ acknowledge (bits 0->3 = irq 1->4), IRQ enable in hi nibble (bits 4->7 = irq 1->4)
 * d58000: control register (OBJCHA, 68000 enable/disable, probably more) (WRPOR2)
 * d5a000: dipswitch bank 1		    (RDPORT1)
 * d5a001: dipswitch bank 2
 * d5a002: input port (service switch)
 * d5a003: EEPROM data/ready in bit 0
 * d5c000: player 1 inputs		    (RDPORT2)
 * d5c001: player 2 inputs
 * d5e000: test switch			    (RDPORT3)
 * d80000: K054338 alpha blender registers
 * d90000: palette RAM for tilemaps
 * da0000: tilemap RAM (8k window, bankswitched by d40033)
 * da2000: (run n' gun 2 copies some ROMs here)
 * dc0000: LANCRAMCS
 * dd0000: LANCIOCS
 * dd2000: RS232C-1
 * dd4000: RS232C-2
 * dd6000: trackball 1
 * dd8000: trackball 2
 * dda000: ADC-WRPORT
 * ddc000: ADC-RDPORT
 * dde000: EXT-WRPORT
 * de0000: EXT_RDPORT
 * e20000: ??? (more RnG 2 additional RAM)
 * e40000: ??? (more RnG 2 additional RAM)
 * e60000: ??? ("""")
 * e80000: 32k of some kind of RAM that RnG2 and Soccer Superstars want
 * ea0000: 32k of some kind of RAM that RnG2 and Soccer Superstars want
 * f00000: 32k of RnG2 additional RAM
 *
 * IRQs:
 * 1: VBL (at 60 Hz)
 * 2: HBL
 * 3: Sprite DMA complete
 * 4: from protection device, indicates command completion for "ESC" chip
 *
 */

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"
#include "machine/konamigx.h"

VIDEO_START(konamigx_5bpp);
VIDEO_START(konamigx_6bpp);
VIDEO_START(konamigx_6bpp_2);
VIDEO_START(le2);
VIDEO_UPDATE(konamigx);
void konamigx_set_alpha(int on);

WRITE32_HANDLER( konamigx_palette_w );
WRITE32_HANDLER( konamigx_tilebank_w );
WRITE32_HANDLER( konamigx_sprbank_w );

static data8_t gx_irqregister;
static data32_t *gx_workram;	/* workram pointer for ESC protection fun */
static data8_t sndto020[16], sndto000[16];	/* read/write split mapping */
static data8_t gx_toggle = 0;
data8_t gx_control1;
static int init_eeprom_count;

static WRITE32_HANDLER(gx_w)
{
	UINT32 old = gx_workram[offset];
	UINT32 new;
	COMBINE_DATA(gx_workram+offset);
	new = gx_workram[offset];
	if(0 && old != new) {
		int adr = 0xc00000+4*offset;
		int sn, so;
#if 0
		if(adr < 0xc07230 || adr >= 0xc07230+0x172*0xc0)
			return;
		sn = (adr-0xc07230)/0xc0;
		so = (adr-0xc07230)%0xc0;
#else
		if(adr < 0xc00604 || adr >= 0xc00604+0xfc*0x100)
			return;
		sn = (adr-0xc00604)/0x100;
		so = (adr-0xc00604)%0x100;
#endif

		//		fprintf(stderr, "%06x [%03x.%02x]: %08x (%x)\n", adr, sn, so, new, activecpu_get_pc());
	}
}

static NVRAM_HANDLER(konamigx_93C46)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

/*
   Konami ESC (E Security Chip) protection chip found on:
	- Salamander 2
	- Sexy Parodius
	- Twinbee Yahhoo
	- Dragoon Might
	- Daisu-Kiss

   The ESC is a custom microcontroller with external SRAM connected
   to it.  It's microprogram is uploaded by the host game.  The ESC
   has complete DMA access to the entire 68020 address space, like
   most Konami protection devices.

   Most games use the same or a compatible microprogram.  This
   program gathers the sprite information scattered around work RAM
   and builds a sprite list in sprite RAM which is properly sorted
   in priority order.  The Japanese version of Gokujou Parodius
   contains a 68020 version of this program at 0x2a285c.  The code
   below is almost a direct translation into C of that routine.

   Salamander 2 uses a different work RAM format and a different
   ESC program, and Dragoon Might uses a third format and program.
   These games haven't been reverse-engineered yet and so have
   no sprites.

   Winning Spike contains a different security chip which lives at
   cc0000 and cc0004.  Unlike ESC it has no external SRAM and no
   program is uploaded to it.

   Run and Gun 2 may contain yet another type of security chip,
   although this is unverified.

*/

/* constant names mostly taken from sexy parodius, which includes
   a debug protection routine with asserts intact!  Cracking the ESC
   would have been much more difficult without it.
 */

#define ESC_INIT_CONSTANT	0x0108db04
#define ESC_OBJECT_MAGIC_ID	0xfef724fb
#define ESTATE_END	2
#define ESTATE_ERROR	3

//    opcode 1
// dragoonj
//  ESC Opcode 1 : 000000xx 000000xx 00000xx0 00000000
// tbyahhoo  sprites at c00000
//  ESC Opcode 1 : 0000ffff 0000ffff 0000ffff 0000000e
// sexyparo  sprites at c00604
//  ESC Opcode 1 : 00000000 00000000 00d20000 00000000
//  ESC Opcode 1 : 00000000 00000000 00d21000 00000000
// salmndr2
//  ESC Opcode 1 : 00c1f7f8 0022002c 00c00060 00010014
// tokkae
//  ESC Opcode 1 : 00c00000 0000000x 00000023 0000ffff
// puzldama
//  ESC Opcode 1 : 002xxxxx 00236102 xxxxxx01 xxxxxxxx

// Say hello to gokuparo at 0x2a285c

static struct sprite_entry {
	int pri;
	unsigned int adr;
} sprites[0x100];

static int pri_comp(const void *s1, const void *s2)
{
	return ((struct sprite_entry *)s1)->pri - ((struct sprite_entry *)s2)->pri;
}

static void generate_sprites(UINT32 src, UINT32 spr, int count)
{
	int i;
	int scount;
	int ecount;
	scount = 0;
	ecount = 0;

	for(i=0; i<count; i++) {
		unsigned int adr = src + 0x100*i;
		int pri;
		if(!cpu_readmem24bedw_word(adr+2))
			continue;
		pri = cpu_readmem24bedw_word(adr+28);

		if(pri < 256) {
			sprites[ecount].pri = pri;
			sprites[ecount].adr = adr;
			ecount++;
		}
	}
	qsort(sprites, ecount, sizeof(struct sprite_entry), pri_comp);

	for(i=0; i<ecount; i++) {
		unsigned int adr = sprites[i].adr;
		if(adr) {
			unsigned int set =(cpu_readmem24bedw_word(adr) << 16)|cpu_readmem24bedw_word(adr+2);
			unsigned short glob_x = cpu_readmem24bedw_word(adr+4);
			unsigned short glob_y = cpu_readmem24bedw_word(adr+8);
			unsigned short flip_x = cpu_readmem24bedw_word(adr+12) ? 0x1000 : 0x0000;
			unsigned short flip_y = cpu_readmem24bedw_word(adr+14) ? 0x2000 : 0x0000;
			unsigned short glob_f = flip_x | (flip_y ^ 0x2000);
			unsigned short zoom_x = cpu_readmem24bedw_word(adr+20);
			unsigned short zoom_y = cpu_readmem24bedw_word(adr+22);
			unsigned short color_val    = 0x0000;
			unsigned short color_mask   = 0xffff;
			unsigned short color_set    = 0x0000;
			unsigned short color_rotate = 0x0000;
			unsigned short v;

			v = cpu_readmem24bedw_word(adr+24);
			if(v & 0x8000) {
				color_mask = 0xf3ff;
				color_val |= (v & 3) << 10;
			}

			v = cpu_readmem24bedw_word(adr+26);
			if(v & 0x8000) {
				color_mask &= 0xfcff;
				color_val  |= (v & 3) << 8;
			}

			v = cpu_readmem24bedw_word(adr+18);
			if(v & 0x8000) {
				color_mask &= 0xff1f;
				color_val  |= v & 0xe0;
			}

			v = cpu_readmem24bedw_word(adr+16);
			if(v & 0x8000)
				color_set = v & 0x1f;
			if(v & 0x4000)
				color_rotate = v & 0x1f;

			if(!zoom_x)
				zoom_x = 0x40;
			if(!zoom_y)
				zoom_y = 0x40;

			if(set >= 0x200000 && set < 0xd00000)
			{
				unsigned short count2 = cpu_readmem24bedw_word(set);
				set += 2;
				while(count2) {
					unsigned short idx  = cpu_readmem24bedw_word(set);
					unsigned short flip = cpu_readmem24bedw_word(set+2);
					unsigned short col  = cpu_readmem24bedw_word(set+4);
					short y = cpu_readmem24bedw_word(set+6);
					short x = cpu_readmem24bedw_word(set+8);

					if(idx == 0xffff) {
						set = (flip<<16) | col;
						if(set >= 0x200000 && set < 0xd00000)
							continue;
						else
							break;
					}

					if(zoom_y != 0x40)
						y = y*0x40/zoom_y;
					if(zoom_x != 0x40)
						x = x*0x40/zoom_x;

					if(flip_x)
						x = glob_x - x;
					else
						x = glob_x + x;
					if(x < -256 || x > 512+32)
						goto next;

					if(flip_y)
						y = glob_y - y;
					else
						y = glob_y + y;
					if(y < -256 || y > 512)
						goto next;

					col = (col & color_mask) | color_val;
					if(color_set)
						col = (col & 0xffe0) | color_set;
					if(color_rotate)
						col = (col & 0xffe0) | ((col + color_rotate) & 0x1f);

					cpu_writemem24bedw_word(spr   , (flip ^ glob_f) | scount);
					cpu_writemem24bedw_word(spr+ 2, idx);
					cpu_writemem24bedw_word(spr+ 4, y);
					cpu_writemem24bedw_word(spr+ 6, x);
					cpu_writemem24bedw_word(spr+ 8, zoom_y);
					cpu_writemem24bedw_word(spr+10, zoom_x);
					cpu_writemem24bedw_word(spr+12, col);
					spr += 16;
					scount++;
					if(scount == 256)
						return;
				next:
					count2--;
					set += 10;
				}
			}
		}
	}
	while(scount < 256) {
		cpu_writemem24bedw_word(spr, scount);
		scount++;
		spr += 16;
	}
}

static void generate_sprites_2(UINT32 src, UINT32 spr, int count)
{
	int i;
	int scount;
	int ecount;
	scount = 0;
	ecount = 0;

	for(i=0; i<count; i++) {
		unsigned int adr = src + 0xc0*i;
		int pri;
		if(!cpu_readmem24bedw_dword(adr+0x08))
			continue;
		pri = 1; //cpu_readmem24bedw_word(adr+28);

		if(pri < 256) {
			sprites[ecount].pri = pri;
			sprites[ecount].adr = adr;
			ecount++;
		}
	}
	qsort(sprites, ecount, sizeof(struct sprite_entry), pri_comp);

	for(i=0; i<ecount; i++) {
		unsigned int adr = sprites[i].adr;
		if(adr) {
			unsigned int set;
			unsigned short glob_x = cpu_readmem24bedw_word(adr+0x14);
			unsigned short glob_y = cpu_readmem24bedw_word(adr+0x18);
			unsigned short flip_x = 0; //cpu_readmem24bedw_word(adr+12) ? 0x1000 : 0x0000;
			unsigned short flip_y = 0; //cpu_readmem24bedw_word(adr+14) ? 0x2000 : 0x0000;
			unsigned short glob_f = flip_x | flip_y;

			count = cpu_readmem24bedw_word(adr+0x1e) & 0x7fff;

#if 0
			{
				int z;
				fprintf(stderr, "%03x:", (adr-src)/0xc0);
				for(z=0; z<0x20; z+=2)
					fprintf(stderr, " %04x", cpu_readmem24bedw_word(adr+z));
				fprintf(stderr, "\n");
			}
#endif

			// Unused info, to add somewhere:
			// bits 12-15 of adr+4
			// bit 15 of adr+1e
			// adr+1c (sprite priority ?)
			if(count > 10)
				count = 10;

			for(set = adr+0x20; set < adr+0x20+0x10*count; set += 0x10) {
				unsigned short idx   = cpu_readmem24bedw_word(set+0x2);
				unsigned short flags = cpu_readmem24bedw_word(set+0x0);
				unsigned short col   = cpu_readmem24bedw_word(set+0xc);
				short y = cpu_readmem24bedw_word(set+0x4);
				short x = cpu_readmem24bedw_word(set+0x6);
				unsigned short zoom_x = cpu_readmem24bedw_word(set+0x8);
				unsigned short zoom_y = cpu_readmem24bedw_word(set+0xa);

#if 0
				{
					int z;
					fprintf(stderr, "  %x:", (set-adr)/0x10);
					for(z=0; z<0x10; z+=2)
						fprintf(stderr, " %04x", cpu_readmem24bedw_word(set+z));
					fprintf(stderr, "\n");
				}
#endif

				if(!zoom_x)
					zoom_x = 0x40;
				if(!zoom_y)
					zoom_y = 0x40;

				if(flip_x)
					x = glob_x - x;
				else
					x = glob_x + x;

				if(x < -256 || x > 512+32)
					continue;

				if(flip_y)
					y = glob_y - y;
				else
					y = glob_y + y;

				if(y < -256 || y > 512)
					continue;

				cpu_writemem24bedw_word(spr   , ((flags & 0xff00) ^ glob_f) | scount);
				cpu_writemem24bedw_word(spr+ 2, idx);
				cpu_writemem24bedw_word(spr+ 4, y);
				cpu_writemem24bedw_word(spr+ 6, x);
				cpu_writemem24bedw_word(spr+ 8, zoom_y);
				cpu_writemem24bedw_word(spr+10, zoom_x);
				cpu_writemem24bedw_word(spr+12, (col & 0x1f) | ((flags & 0xf) << 6) | (( col & 0x0f00) << 4));
				spr += 16;
				scount++;
				if(scount == 256)
					return;
			}
		}
	}
	while(scount < 256) {
		cpu_writemem24bedw_word(spr, scount);
		scount++;
		spr += 16;
	}
}

static void sal2_esc(UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4)
{
	generate_sprites_2(0xc07230, 0xd20000, 0x172);
}

static void sexyparo_esc(UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4)
{
	// The d20000 should probably be p3
	generate_sprites(0xc00604, 0xd20000, 0xfc);
}

static void tbyahhoo_esc(UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4)
{
	generate_sprites(0xc00000, 0xd20000, 0x100);
}

static unsigned char esc_program[4096];
static void (*esc_cb)(UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4);

static WRITE32_HANDLER( esc_w )
{
	unsigned int opcode;
	unsigned int params;

	/* ignore NULL writes to the ESC (these appear to be "keepalives" on the real hardware) */
	if (!data)
	{
		return;
	}

	/* ignore out-of-range addresses */
	if ((data < 0xc00000) || (data > 0xc1ffff))
	{
		return;
	}

	/* the master opcode can be at an unaligned address, so get it "safely" */
	opcode = (cpu_readmem24bedw_word(data+2))|(cpu_readmem24bedw_word(data)<<16);

	/* if there's an OBJECT_MAGIC_ID, that means
	   there is a valid ESC command packet. */
	if (opcode == ESC_OBJECT_MAGIC_ID)
	{
		int i;
		/* get the subop now */
		opcode = cpu_readmem24bedw(data+8);
		params = (cpu_readmem24bedw_word(data+12) << 16) | cpu_readmem24bedw_word(data+14);

		switch(opcode) {
		case 5: // Reset
			break;
		case 2: // Load program
			for(i=0; i<4096; i++)
				esc_program[i] = cpu_readmem24bedw(params+i);
#if 0
			{
				FILE *f;

				f = fopen("esc.bin", "wb");
				fwrite(esc_program, 4096, 1, f);
				fclose(f);

				logerror("Dumping ESC program\n");
			}
#endif
			break;
		case 1: // Run program
			if(esc_cb) {
				UINT32 p1 = (cpu_readmem24bedw_word(params+0)<<16) | cpu_readmem24bedw_word(params+2);
				UINT32 p2 = (cpu_readmem24bedw_word(params+4)<<16) | cpu_readmem24bedw_word(params+6);
				UINT32 p3 = (cpu_readmem24bedw_word(params+8)<<16) | cpu_readmem24bedw_word(params+10);
				UINT32 p4 = (cpu_readmem24bedw_word(params+12)<<16) | cpu_readmem24bedw_word(params+14);
				esc_cb(p1, p2, p3, p4);
			}
			break;
		default:
			logerror("Unknown ESC opcode %d\n", opcode);
			break;
		}
		cpu_writemem24bedw(data+9, ESTATE_END);

		if (gx_irqregister & 0x10)
		{
			cpu_set_irq_line(0, 4, HOLD_LINE);
		}
	}
	else
	{
		/* INIT_CONSTANT means just for the ESC to initialize itself,
		   there is not normal command parsing here. */
		if (opcode == ESC_INIT_CONSTANT)
		{
//			logerror("Got ESC_INIT_CONSTANT, 'booting' ESC\n");
			return;
		}

		/* unknown constant (never been seen in any game..) */
	}
}

static READ32_HANDLER( sound020_r )
{
	int reg=0, rv=0;

	reg = (offset*2)-8;
//	logerror("020: read %x from sound reg %d (PC=%lx, mask=%x)\n", rv, reg, activecpu_get_pc(), mem_mask);
	rv = sndto020[reg]<<24 | sndto020[reg+1]<<8;

	/* we clearly have some problem because some games require these hacks */

	/* perhaps 68000/68020 timing is skewed? */
	if (!strcmp(Machine->gamedrv->name, "le2"))
	{
		if (reg == 0) rv |= 0xff00;
	}
	if (!strcmp(Machine->gamedrv->name, "winspike"))
	{
		if (activecpu_get_pc() == 0x2026fe) rv = 0xc0c0c0c0;

	}

	if (!strcmp(Machine->gamedrv->name, "tkmmpzdm"))
	{

		if (activecpu_get_pc() == 0x20360a) rv = 0;

		if (activecpu_get_pc() == 0x203680) rv = 0x0f000000;

		if (activecpu_get_pc() == 0x20370a) rv = 0;

		if (activecpu_get_pc() == 0x203710) rv = 0;

		if (activecpu_get_pc() == 0x2037c8) rv = 0xff00;

		if (activecpu_get_pc() == 0x2037d0) rv = 0;

		if (activecpu_get_pc() == 0x203858) rv = 0xaa00;

		if (activecpu_get_pc() == 0x203860) rv = 0x55000000;

		if (activecpu_get_pc() == 0x2038e8) rv = 0x5500;

		if (activecpu_get_pc() == 0x2038f0) rv = 0xaa000000;

		if (activecpu_get_pc() == 0x203978) rv = 0;

		if (activecpu_get_pc() == 0x203980) rv = 0xff000000;

		if (activecpu_get_pc() == 0x2039ea) rv = 0;

		if (activecpu_get_pc() == 0x203a2e) rv = 0x0f000000;

		if (activecpu_get_pc() == 0x203ae4) rv = 0;

		if (activecpu_get_pc() == 0x203b7c) rv = 0xff000000;

		if (activecpu_get_pc() == 0x203b90) rv = 0xff00;

		if (activecpu_get_pc() == 0x203bb6) rv = 0;

		if (activecpu_get_pc() == 0x203c6a) rv = 0;

	}

	if (reg == 2) rv = 0;	/* hack */

//	logerror("020: read %x from sound reg %d (PC=%x, mask=%x)\n", rv, reg, activecpu_get_pc(), mem_mask);
/*	logerror("to020: %x %x %x %x %x %x %x %x\n",
			sndto020[0], sndto020[1],
			sndto020[2], sndto020[3],
			sndto020[4], sndto020[5],
			sndto020[6], sndto020[7]);*/

	return(rv);
}

static void write_snd_020(int reg, int val)
{
	sndto000[reg] = val;

//	logerror("020: write %x to %x\n", val, reg);

	if (reg == 7)
	{
//		logerror("020: sound command %x %x %x %x\n", sndto000[0], sndto000[1], sndto000[2], sndto000[3]);
		cpu_set_irq_line(1, 1, HOLD_LINE);
	}
}

static WRITE32_HANDLER( sound020_w )
{
	int reg=0, val=0;

	if (!(mem_mask & 0xff000000))
	{
		reg = offset*2;
		val = data>>24;
		write_snd_020(reg, val);
	}

	if (!(mem_mask & 0xff00))
	{
		reg = (offset*2)+1;
		val = (data>>8)&0xff;
		write_snd_020(reg, val);
	}
}

/* input handlers */
static READ32_HANDLER( service_r )
{
	int res = (readinputport(1)<<24) | (readinputport(8)<<8);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= ~0x08000000;
	}

	return res;
}

static READ32_HANDLER( players_r )
{
	return (readinputport(2)<<24) | (readinputport(3)<<16) | (readinputport(4)<<8) | (readinputport(5));
}

static READ32_HANDLER( le2_gun_H_r )
{

	int p1x = readinputport(9)*287/0xff+24;

	int p2x = readinputport(11)*287/0xff+24;

	return (p1x<<16)|p2x;

}

static READ32_HANDLER( le2_gun_V_r )
{

	int p1y = readinputport(10)*223/0xff;

	int p2y = readinputport(12)*223/0xff;

	return (p1y<<16)|p2y;

}

/* EEPROM handlers */
static READ32_HANDLER( eeprom_r )
{
	// d5a000 = DIP switches #1 (RDPORT1)
	// d5a001 = DIP switches #2
	// d5a002 = input port: service4/3/2/1 coin 4/3/2/1
	// d5a003 = objint stat, exioint stat, trackint stat, excgint stat, escint stat,
	//	    excpuint stat, objdma stat, eeprom do

	// note: racin' force expects bit 1 of the eeprom port to toggle
	gx_toggle ^= 0x02;
	return (readinputport(6) << 24) | (readinputport(7) << 16) | (readinputport(0) << 8) | EEPROM_read_bit() | gx_toggle;
}

static WRITE32_HANDLER( eeprom_w )
{
	data32_t odata;

	if (!(mem_mask & 0xff000000))
	{
		odata = data >> 24;
		/*
		  bit 7: afr
		  bit 6: objscan
		  bit 5: background color select: 0 = 338 solid color, 1 = 5^5 gradient
		  bit 4: coin counter 2
		  bit 3: coin counter 1
		  bit 2: eeprom clock
		  bit 1: eeprom chip select
		  bit 0: eeprom data
		*/

		EEPROM_write_bit((odata&0x01) ? 1 : 0);
		EEPROM_set_cs_line((odata&0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((odata&0x04) ? ASSERT_LINE : CLEAR_LINE);

		gx_control1 = odata;
	}

	if (!(mem_mask & 0xff0000))
	{
		/*
		  bit 7 = mask all IRQ
		  bit 6 = LAN IRQ enable
		  bit 5 = CCU2 IRQ enable
		  bit 4 = ESC IRQ enable
		  bit 3 = EXCPU IRQ enable
		  bit 2 = OBJ IRQ enable
		  bit 1 = CCU1-INT2 enable
		  bit 0 = CCU1-INT1 enable
		*/

		gx_irqregister = (data>>16)&0xff;
//		logerror("write %x to IRQ register (PC=%x)\n", gx_irqregister, activecpu_get_pc());
	}
}

static WRITE32_HANDLER( control_w )
{
	logerror("write %x to control register (mask=%x)\n", data, mem_mask);

	// known controls:
	// bit 23 = reset graphics chips
	// bit 22 = 0 to halt 68000, 1 to let it run (SOUNDRESET)
	// bit 21 = VRAM-CHARD 0=VRAM, 1=ROM
	// bit 20 = OBJCHA line for '246
	// bit 19 = select top 2 sprite priority bits to be 14/15 or 16/17 of the
	//          spritelist "color" word.
	// bit 18 = if 0, the top 2 sprite priority bits are "11" else use bit 19's
	//          results.
	// bit 17 = DOTSEL1 : 0 = 6M, 1=8M, 2=12M, 3=16M
	// bit 16 = DOTSEL0
	if (!(mem_mask & 0x00ff0000))
	{
		if (data & 0x400000)
		{
			// enable 68k
			// clear the halt condition and reset the 68000
			cpu_set_halt_line(1, CLEAR_LINE);
			cpu_set_reset_line(1, PULSE_LINE);
		}
		else
		{
			// disable 68k
			cpu_set_halt_line(1, ASSERT_LINE);
		}

		K053246_set_OBJCHA_line((data&0x100000) ? ASSERT_LINE : CLEAR_LINE);
	}
}

/* interrupt controller */
static INTERRUPT_GEN(konamigx_interrupt)
{
	if (!(gx_irqregister & 0x80))
	{
		return;
	}

	// IRQ 1 happens at 60 hz and is the main vbl
	if ((cpu_getiloops() == 0) && (gx_irqregister & 0x01))
	{

		cpu_set_irq_line(0, 1, HOLD_LINE);
	}

#if 0	// IRQ 2 is the scanline interrupt
	if ((cpu_getiloops() == 1)) //&& (gx_irqregister & 0x20))
	{
		cpu_set_irq_line(0, 2, HOLD_LINE);
	}
#endif

	// IRQ 3 is the "sprite list DMA end" IRQ and also happens at 60 Hz
	if ((cpu_getiloops() == 1) && (gx_irqregister & 0x04))
	{
		cpu_set_irq_line(0, 3, HOLD_LINE);
	}
}



static INTERRUPT_GEN(gxtype1_interrupt)
{
	if (!(gx_irqregister & 0x80))
	{
		return;
	}

	// IRQ 1 happens at 60 hz and is the main vbl
	if ((cpu_getiloops() == 0) && (gx_irqregister & 0x01))
	{

		cpu_set_irq_line(0, 1, HOLD_LINE);
	}

#if 0	// IRQ 2 is the scanline interrupt
	if ((cpu_getiloops() == 1)) //&& (gx_irqregister & 0x20))
	{
		cpu_set_irq_line(0, 2, HOLD_LINE);
	}
#endif

	// IRQ 3 is the "sprite list DMA end" IRQ and also happens at 60 Hz
	if ((cpu_getiloops() == 1) && (gx_irqregister & 0x04))
	{
		cpu_set_irq_line(0, 3, HOLD_LINE);
	}
}


static int analog_ctl;



static int analog_val = 0;

static int analog_latch;

static int analog_state = 0;


static READ32_HANDLER( gxanalog_r )
{

	return analog_val;
}


// selects which port? to read
static WRITE32_HANDLER( gxanalog_w )
{

	int new_ctl = (data >> 24) & 0xf;


	// guaranteed resync

	if ((new_ctl == 4) && (analog_ctl == 1))

	{

		analog_state = 1;

	}



	switch (analog_state)

	{

		case 0:	  // initial state

			break;



		case 1:	// got a 4, check next

			if ((new_ctl == 4) && (analog_ctl == 4))

			{

				analog_latch = readinputport(10);	// gas

				analog_state = 2;

			}



			if ((new_ctl == 2) && (analog_ctl == 4))

			{

				analog_latch = readinputport(9);	// steer

				analog_state = 3;

			}

			break;



		case 2:

			if ((new_ctl == 1) && (analog_ctl == 0))

			{

				analog_val = (analog_latch & 0x200)<<15;

				analog_latch <<= 1;

			}

			break;



		case 3:

			if ((new_ctl == 1) && (analog_ctl == 0))

			{

				analog_val = (analog_latch & 0x400)<<14;

				analog_latch <<= 1;

			}

			break;

	}



	analog_ctl = new_ctl;

}

/* 68020 memory handlers */
static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x01ffff, MRA32_ROM },		// bios
	{ 0x200000, 0x2fffff, MRA32_ROM },		// game program
	{ 0x400000, 0x5fffff, MRA32_ROM },		// data ROM
	{ 0xc00000, 0xc1ffff, MRA32_RAM },		// work ram
	{ 0xd00000, 0xd01fff, K056832_rom_long_r },	// tile ROM readthrough (for test menu)
	{ 0xd20000, 0xd20fff, K053247_long_r },		// sprite RAM
	{ 0xd21000, 0xd23fff, MRA32_RAM },		// additional RAM in the sprite region
	{ 0xd44000, 0xd44003, le2_gun_H_r },		// gun horizontal position
	{ 0xd44004, 0xd44007, le2_gun_V_r },		// gun vertical position
	{ 0xd4a000, 0xd4bfff, K053246_long_r },		// sprite ROM readthrough (for test menu)
	{ 0xd52000, 0xd5201f, sound020_r },		// shared RAM with sound 68000
	{ 0xd5a000, 0xd5a003, eeprom_r },		// EEPROM read
	{ 0xd5c000, 0xd5c003, players_r },		// player 1 & 2 JAMMA inputs
	{ 0xd5e000, 0xd5e003, service_r }, 		// service switch
	{ 0xd90000, 0xd97fff, MRA32_RAM },		// palette RAM
	{ 0xda0000, 0xda1fff, K056832_ram_long_r },	// tilemap RAM
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0xc00000, 0xc1ffff, gx_w, &gx_workram },
	{ 0xcc0000, 0xcc0003, esc_w },
	{ 0xd44000, 0xd4400f, konamigx_tilebank_w },
	{ 0xd4a000, 0xd4a0ff, MWA32_NOP },	// dragoonj writes here constantly
//	{ 0xd4c000, 0xd4c0ff, MWA32_NOP },
	{ 0xd4c01c, 0xd4c01f, MWA32_NOP },
	{ 0xd50000, 0xd5007f, K055555_long_w },
	{ 0xd20000, 0xd20fff, K053247_long_w },
	{ 0xd21000, 0xd23fff, MWA32_RAM },
	{ 0xd52000, 0xd5201f, sound020_w },
	{ 0xd40000, 0xd4003f, K056832_long_w },
	{ 0xd48000, 0xd4803f, K053246_long_w },
	{ 0xd4a000, 0xd4a01f, konamigx_sprbank_w },
	{ 0xd56000, 0xd56003, eeprom_w },
	{ 0xd58000, 0xd58003, control_w },
	{ 0xd80000, 0xd800ff, K054338_long_w },
	{ 0xda0000, 0xda1fff, K056832_ram_long_w },
	{ 0xda2000, 0xda3fff, K056832_ram_long_w },	// dragoon might wants this
	{ 0xd90000, 0xd97fff, konamigx_palette_w, &paletteram32 },
MEMORY_END

static MEMORY_READ32_START( type1readmem )
	{ 0x000000, 0x01ffff, MRA32_ROM },		// bios
	{ 0x200000, 0x2fffff, MRA32_ROM },		// game program
	{ 0x400000, 0x7fffff, MRA32_ROM },		// data ROM
	{ 0xc00000, 0xc1ffff, MRA32_RAM },		// work ram
	{ 0xd00000, 0xd01fff, K056832_rom_long_r },	// tile ROM readthrough (for test menu)
	{ 0xd20000, 0xd20fff, K053247_long_r },		// sprite RAM
	{ 0xd21000, 0xd23fff, MRA32_RAM },		// additional RAM in the sprite region
	{ 0xd4a000, 0xd4bfff, K053246_long_r },		// sprite ROM readthrough (for test menu)
	{ 0xd52000, 0xd5201f, sound020_r },		// shared RAM with sound 68000
	{ 0xd5a000, 0xd5a003, eeprom_r },		// EEPROM read
	{ 0xd5c000, 0xd5c003, players_r },		// player 1 & 2 JAMMA inputs
	{ 0xd5e000, 0xd5e003, service_r }, 		// service switch
	{ 0xd90000, 0xd97fff, MRA32_RAM },		// palette RAM
	{ 0xda0000, 0xda1fff, K056832_ram_long_r },	// tilemap RAM
	{ 0xdc0000, 0xdc1fff, MRA32_RAM },
	{ 0xdd0000, 0xdd00ff, MRA32_NOP },	// LAN board
	{ 0xddc000, 0xddcfff, gxanalog_r },
	{ 0xe80000, 0xe81fff, MRA32_RAM },
	{ 0xea0000, 0xea1fff, MRA32_RAM },
	{ 0xec0000, 0xedffff, MRA32_RAM },
	{ 0xf80000, 0xf80fff, MRA32_RAM },
	{ 0xfc0000, 0xfc0fff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( type1writemem )
	{ 0xc00000, 0xc1ffff, MWA32_RAM, &gx_workram },
	{ 0xcc0000, 0xcc0003, esc_w },
	{ 0xd44000, 0xd4400f, konamigx_tilebank_w },
//	{ 0xd4c000, 0xd4c0ff, MWA32_NOP },
	{ 0xd4c01c, 0xd4c01f, MWA32_NOP },
	{ 0xd50000, 0xd5007f, K055555_long_w },
	{ 0xd20000, 0xd20fff, K053247_long_w },
	{ 0xd21000, 0xd23fff, MWA32_RAM },
	{ 0xd52000, 0xd5201f, sound020_w },
	{ 0xd40000, 0xd4003f, K056832_long_w },
	{ 0xd48000, 0xd4803f, K053246_long_w },
	{ 0xd56000, 0xd56003, eeprom_w },
	{ 0xd58000, 0xd58003, control_w },
	{ 0xd80000, 0xd800ff, K054338_long_w },
	{ 0xda0000, 0xda1fff, K056832_ram_long_w },
	{ 0xd90000, 0xd97fff, konamigx_palette_w, &paletteram32 },
	{ 0xdc0000, 0xdc1fff, MWA32_RAM },
	{ 0xdd0000, 0xdd00ff, MWA32_NOP },	// LAN board
	{ 0xdda000, 0xddafff, gxanalog_w },
	{ 0xe00000, 0xe0ffff, MWA32_NOP },
	{ 0xe20000, 0xe2ffff, MWA32_NOP },
	{ 0xe80000, 0xe81fff, MWA32_RAM },
	{ 0xea0000, 0xea1fff, MWA32_RAM },
	{ 0xec0000, 0xedffff, MWA32_RAM },
	{ 0xf80000, 0xf80fff, MWA32_RAM },
	{ 0xfc0000, 0xfc0fff, MWA32_RAM },
MEMORY_END

static MEMORY_READ32_START( type3readmem )
	{ 0x000000, 0x01ffff, MRA32_ROM },		// bios
	{ 0x200000, 0x2fffff, MRA32_ROM },		// game program
	{ 0x400000, 0x5fffff, MRA32_ROM },		// data ROM
	{ 0xc00000, 0xc1ffff, MRA32_RAM },		// work ram
	{ 0xd00000, 0xd01fff, K056832_rom_long_r },	// tile ROM readthrough (for test menu)
	{ 0xd20000, 0xd20fff, K053247_long_r },		// sprite RAM
	{ 0xd21000, 0xd23fff, MRA32_RAM },		// additional RAM in the sprite region
	{ 0xd4a000, 0xd4bfff, K053246_long_r },		// sprite ROM readthrough (for test menu)
	{ 0xd52000, 0xd5201f, sound020_r },		// shared RAM with sound 68000
	{ 0xd5a000, 0xd5a003, eeprom_r },		// EEPROM read
	{ 0xd5c000, 0xd5c003, players_r },		// player 1 & 2 JAMMA inputs
	{ 0xd5e000, 0xd5e003, service_r }, 		// service switch
	{ 0xd90000, 0xd97fff, MRA32_RAM },		// palette RAM
	{ 0xda0000, 0xda1fff, K056832_ram_long_r },	// tilemap RAM
	{ 0xe60000, 0xe60fff, MRA32_RAM },
	{ 0xe80000, 0xe87fff, MRA32_RAM },
	{ 0xea0000, 0xea7fff, MRA32_RAM },
	{ 0xf00000, 0xf07fff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( type3writemem )
	{ 0xc00000, 0xc1ffff, MWA32_RAM, &gx_workram },
	{ 0xcc0000, 0xcc0003, esc_w },
	{ 0xd44000, 0xd4400f, konamigx_tilebank_w },
//	{ 0xd4c000, 0xd4c0ff, MWA32_NOP },
	{ 0xd4c01c, 0xd4c01f, MWA32_NOP },
	{ 0xd50000, 0xd5007f, K055555_long_w },
	{ 0xd20000, 0xd20fff, K053247_long_w },
	{ 0xd21000, 0xd23fff, MWA32_RAM },
	{ 0xd52000, 0xd5201f, sound020_w },
	{ 0xd40000, 0xd4003f, K056832_long_w },
	{ 0xd48000, 0xd4803f, K053246_long_w },
	{ 0xd56000, 0xd56003, eeprom_w },
	{ 0xd58000, 0xd58003, control_w },
	{ 0xd80000, 0xd800ff, K054338_long_w },
	{ 0xda0000, 0xda1fff, K056832_ram_long_w },
	{ 0xd90000, 0xd97fff, MWA32_RAM, &paletteram32 }, //konamigx_palette_w, &paletteram32 },
	{ 0xe60000, 0xe60fff, MWA32_RAM },
	{ 0xe80000, 0xe87fff, MWA32_RAM },
	{ 0xea0000, 0xea7fff, MWA32_RAM },
	{ 0xf00000, 0xf07fff, MWA32_RAM },
MEMORY_END

static MEMORY_READ32_START( type4readmem )
	{ 0x000000, 0x01ffff, MRA32_ROM },		// bios
	{ 0x200000, 0x2fffff, MRA32_ROM },		// game program
	{ 0x400000, 0x5fffff, MRA32_ROM },		// data ROM
	{ 0xc00000, 0xc1ffff, MRA32_RAM },		// work ram
	{ 0xd00000, 0xd01fff, K056832_rom_long_r },	// tile ROM readthrough (for test menu)
	{ 0xd20000, 0xd20fff, K053247_long_r },		// sprite RAM
	{ 0xd21000, 0xd23fff, MRA32_RAM },		// additional RAM in the sprite region
	{ 0xd4a000, 0xd4bfff, K053246_long_r },		// sprite ROM readthrough (for test menu)
	{ 0xd52000, 0xd5201f, sound020_r },		// shared RAM with sound 68000
	{ 0xd5a000, 0xd5a003, eeprom_r },		// EEPROM read
	{ 0xd5c000, 0xd5c003, players_r },		// player 1 & 2 JAMMA inputs
	{ 0xd5e000, 0xd5e003, service_r }, 		// service switch
	{ 0xd90000, 0xd97fff, MRA32_RAM },		// palette RAM
	{ 0xda0000, 0xda1fff, K056832_ram_long_r },	// tilemap RAM
	{ 0xda2000, 0xda3fff, MRA32_RAM },
	{ 0xe20000, 0xe21fff, MRA32_RAM },
	{ 0xe40000, 0xe41fff, MRA32_RAM },
	{ 0xe60000, 0xe61fff, MRA32_RAM },
	{ 0xe80000, 0xe87fff, MRA32_RAM },
	{ 0xea0000, 0xea7fff, MRA32_RAM },
	{ 0xf00000, 0xf07fff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( type4writemem )
	{ 0xc00000, 0xc1ffff, MWA32_RAM, &gx_workram },
	{ 0xcc0000, 0xcc0003, esc_w },
	{ 0xd44000, 0xd4400f, konamigx_tilebank_w },
//	{ 0xd4c000, 0xd4c0ff, MWA32_NOP },
	{ 0xd4c01c, 0xd4c01f, MWA32_NOP },
	{ 0xd50000, 0xd5007f, K055555_long_w },
	{ 0xd20000, 0xd20fff, K053247_long_w },
	{ 0xd21000, 0xd23fff, MWA32_RAM },
	{ 0xd52000, 0xd5201f, sound020_w },
	{ 0xd40000, 0xd4003f, K056832_long_w },
	{ 0xd48000, 0xd4803f, K053246_long_w },
	{ 0xd56000, 0xd56003, eeprom_w },
	{ 0xd58000, 0xd58003, control_w },
	{ 0xd80000, 0xd800ff, K054338_long_w },
	{ 0xd90000, 0xd97fff, MWA32_RAM, &paletteram32 }, //konamigx_palette_w, &paletteram32 },
	{ 0xda0000, 0xda1fff, K056832_ram_long_w },
	{ 0xda2000, 0xda3fff, MWA32_RAM },
	{ 0xe20000, 0xe21fff, MWA32_RAM },
	{ 0xe40000, 0xe41fff, MWA32_RAM },
	{ 0xe60000, 0xe61fff, MWA32_RAM },
	{ 0xe80000, 0xe87fff, MWA32_RAM },
	{ 0xea0000, 0xea7fff, MWA32_RAM },
	{ 0xf00000, 0xf07fff, MWA32_RAM },
MEMORY_END

/**********************************************************************************/
/* Sound handling */

READ16_HANDLER( dual539_r )
{
	data16_t ret = 0;

	if (ACCESSING_LSB16)
		ret |= K054539_1_r(offset);
	if (ACCESSING_MSB16)
		ret |= K054539_0_r(offset)<<8;

	return ret;
}

WRITE16_HANDLER( dual539_w )
{
	if (ACCESSING_LSB16)
		K054539_1_w(offset, data);
	if (ACCESSING_MSB16)
		K054539_0_w(offset, data>>8);
}

READ16_HANDLER( sndcomm68k_r )
{
	return sndto000[offset-8];
}

WRITE16_HANDLER( sndcomm68k_w )
{
//	logerror("68K: write %x to %x\n", data, offset);
	sndto020[offset] = data;
}

/* 68000 memory handling */
static MEMORY_READ16_START( sndreadmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x200000, 0x2004ff, dual539_r },
	{ 0x300000, 0x300001, tms57002_data_word_r },
	{ 0x400000, 0x40001f, sndcomm68k_r },
	{ 0x500000, 0x500001, tms57002_status_word_r },
MEMORY_END

static MEMORY_WRITE16_START( sndwritemem )
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x2004ff, dual539_w },
	{ 0x300000, 0x300001, tms57002_data_word_w },
	{ 0x400000, 0x40001f, sndcomm68k_w },
	{ 0x500000, 0x500001, tms57002_control_word_w },
MEMORY_END

/* 68000 timer interrupt controller */
static INTERRUPT_GEN(gxaudio_interrupt)
{
	cpu_set_irq_line(1, 2, HOLD_LINE);
}

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};

/**********************************************************************************/


/* i think we could reduce the number of machine drivers with different visible areas by adjusting the sprite

   positioning on a per game basis too */



static MACHINE_DRIVER_START( konamigx )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68EC020, 24000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(konamigx_interrupt,2)

	/* note: part is a -8, crystals are 18.4 and 32.0 MHz, and
	   twinbee yahhoo will not pass POST if the 68000 isn't
	   running at least this fast.  so the higher speed is probably a HACK... */
	MDRV_CPU_ADD_TAG("sound", M68000, 9200000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sndreadmem,sndwritemem)
	MDRV_CPU_PERIODIC_INT(gxaudio_interrupt, 480)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(konamigx_93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(18, 36*8-1+18, 15, 28*8-1+15 )

	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(konamigx_5bpp)
	MDRV_VIDEO_UPDATE(konamigx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tbyahhoo )

	MDRV_IMPORT_FROM(konamigx)

	MDRV_SCREEN_SIZE(64*8, 32*8)

	MDRV_VISIBLE_AREA(0+2, 36*8-1+2, 15, 15+224-1 )

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tokkae )
 MDRV_IMPORT_FROM(konamigx)
 MDRV_VIDEO_START(konamigx_6bpp)

 MDRV_VISIBLE_AREA(22, 36*8-1+22, 16, 28*8-1+16 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( puzzle )
	MDRV_IMPORT_FROM(konamigx)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(21, 21+288-1, 15, 15+224-1 )

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( daiskiss )
	MDRV_IMPORT_FROM(konamigx)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(3, 3+288-1, 16, 16+224-1 )

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dragoonj )
	MDRV_IMPORT_FROM(konamigx)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(4*8, 54*8-1, 0*8, 32*8-1 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( le2 )
	MDRV_IMPORT_FROM(konamigx)

	MDRV_VIDEO_START(le2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( konamigx_6bpp )
	MDRV_IMPORT_FROM(konamigx)
	MDRV_VIDEO_START(konamigx_6bpp)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( konamigx_6bpp_2 )
	MDRV_IMPORT_FROM(konamigx)
	MDRV_VIDEO_START(konamigx_6bpp_2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gxtype1 )
	MDRV_IMPORT_FROM(konamigx)

	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(type1readmem, type1writemem)
	MDRV_CPU_VBLANK_INT(gxtype1_interrupt, 3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gxtype3 )
	MDRV_IMPORT_FROM(konamigx)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(type3readmem, type3writemem)
	MDRV_CPU_VBLANK_INT(konamigx_interrupt, 2)

	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gxtype4 )
	MDRV_IMPORT_FROM(konamigx)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(type4readmem, type4writemem)
	MDRV_CPU_VBLANK_INT(konamigx_interrupt, 2)

	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
MACHINE_DRIVER_END

/**********************************************************************************/

INPUT_PORTS_START( konamigx )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* for gun games */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPSETTING(    0x01, "Mono")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Foo")
	PORT_DIPSETTING(    0x00, "Foo")
	PORT_DIPSETTING(    0x01, "Bar")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( racinfrc )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* for gun games */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )		// racin force needs this set to get past the calibration screen
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPSETTING(    0x01, "Mono")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Foo")
	PORT_DIPSETTING(    0x00, "Foo")
	PORT_DIPSETTING(    0x01, "Bar")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* no gun */

	PORT_START /* mask default type                     sens delta min max */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 35, 5, 0x38, 0xc8 )

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_PEDAL, 35, 5, 0, 0x68, KEYCODE_LCONTROL, IP_JOY_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT )

INPUT_PORTS_END

INPUT_PORTS_START( le2 )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* for gun games */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Mono")
	PORT_DIPNAME( 0x02, 0x02, "Coin Mechanism")

	PORT_DIPSETTING(    0x02, "Common")

	PORT_DIPSETTING(    0x00, "Independant")

	PORT_DIPNAME( 0x04, 0x04, "Stage Select" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Mirror" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* for gun games */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* mask default type                     sens delta min max */
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X | IPF_PLAYER2, 35, 15, 0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y | IPF_PLAYER2, 35, 15, 0, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( gokuparo )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPSETTING(    0x01, "Mono")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( puzldama )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPSETTING(    0x01, "Mono")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, "Vs. cabinet" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( type3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPSETTING(    0x01, "Mono")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Screens" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

/**********************************************************************************/

/* BIOS */
#define GX_BIOS ROM_LOAD("300a01", 0x000000, 128*1024, 0xd5fa95f5)

ROM_START(konamigx)
	ROM_REGION( 0x10000, REGION_CPU1, 0)
	GX_BIOS
ROM_END

#define ROM_LOADSPR_WORD(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPSIZE(2) | ROM_SKIP(5))
#define ROM_LOADSPR_5TH(name,offset,length,crc)	 ROMX_LOAD(name, offset, length, crc, ROM_GROUPSIZE(1) | ROM_SKIP(5))

#define ROM_LOADTILE_WORD(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPDWORD | ROM_SKIP(1))
#define ROM_LOADTILE_BYTE(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPBYTE | ROM_SKIP(4))

#define ROM_LOADTILE_WORDS2(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPDWORD | ROM_SKIP(2))
#define ROM_LOADTILE_BYTES2(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(4))

#define ROM_LOAD48_WORD(name,offset,length,crc)	ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(4))
#define ROM_LOAD64_WORD(name,offset,length,crc)	ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))


/* Gokujou Parodius */
ROM_START( gokuparo )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "321jad02.31b", 0x200002, 512*1024, 0xc2e548c0 )
	ROM_LOAD32_WORD_SWAP( "321jad04.27b", 0x200000, 512*1024, 0x916a7951 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("321_b06.9c", 0x000000, 128*1024, 0xda810554 )
	ROM_LOAD16_BYTE("321_b07.7c", 0x000001, 128*1024, 0xc47634c0 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "fj-jap.17h", 0x000000, 2*1024*1024, 0x437d0057 )
	ROM_LOADTILE_BYTE( "321b12.13g", 0x000004, 512*1024, 0x5f9edfa0 )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "fj-jap.25g", 0x000000, 2*1024*1024, 0xc6e2e74d )
	ROM_LOAD32_WORD( "fj-jap.28g", 0x000002, 2*1024*1024, 0xea9f8c48 )
	ROM_LOAD( "321b09.30g", 0x400000, 1*1024*1024, 0x94add237 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "321b17.9g", 0x000000, 2*1024*1024, 0xb3e8d5d8 )
	ROM_LOAD( "321b18.7g", 0x200000, 2*1024*1024, 0x2c561ad0 )
ROM_END

/* Fantastic Journey (US version) */
ROM_START( fantjour )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "fsus31b.bin", 0x200002, 512*1024, 0xafaf9d17 )
	ROM_LOAD32_WORD_SWAP( "fsus27b.bin", 0x200000, 512*1024, 0xb2cfe225 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("321_b06.9c", 0x000000, 128*1024, 0xda810554 )
	ROM_LOAD16_BYTE("321_b07.7c", 0x000001, 128*1024, 0xc47634c0 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "fj-jap.17h", 0x000000, 2*1024*1024, 0x437d0057 )
	ROM_LOADTILE_BYTE( "321b12.13g", 0x000004, 512*1024, 0x5f9edfa0 )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "fj-jap.25g", 0x000000, 2*1024*1024, 0xc6e2e74d )
	ROM_LOAD32_WORD( "fj-jap.28g", 0x000002, 2*1024*1024, 0xea9f8c48 )
	ROM_LOAD( "321b09.30g", 0x400000, 1*1024*1024, 0x94add237 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "321b17.9g", 0x000000, 2*1024*1024, 0xb3e8d5d8 )
	ROM_LOAD( "321b18.7g", 0x200000, 2*1024*1024, 0x2c561ad0 )
ROM_END

/* Salamander 2 */
ROM_START( salmndr2 )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "521-a02.31b", 0x200002, 512*1024, 0xf6c3a95b )
	ROM_LOAD32_WORD_SWAP( "521-a03.31b", 0x200000, 512*1024, 0xc3be5e0a )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("521-a04.9c", 0x000000, 64*1024, 0xefddca7a )
	ROM_LOAD16_BYTE("521-a05.7c", 0x000001, 64*1024, 0x51a3af2c )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORDS2("521-a09.17h", 0x000000, 2*1024*1024, 0xfb9e2f5e )
	ROM_LOADTILE_WORDS2("521-a11.15h", 0x300000, 1*1024*1024, 0x25e0a6e5 )
	ROM_LOADTILE_BYTES2("521-a13.13c", 0x000004, 2*1024*1024, 0x3ed7441b )

	/* sprites */
	ROM_REGION( 0x600000, REGION_GFX2, 0)
	ROM_LOAD48_WORD( "521-a08.25g", 0x000000, 2*1024*1024, 0xf24f76bd )
	ROM_LOAD48_WORD( "521-a07.28g", 0x000002, 2*1024*1024, 0x50ef9b7a )
	ROM_LOAD48_WORD( "521-a06.30g", 0x000004, 2*1024*1024, 0xcba5db2c )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "521-a12.9g", 0x000000, 2*1024*1024, 0x66614d3b )
	ROM_LOAD( "521-a13.7g", 0x200000, 1*1024*1024, 0xc3322475 )
ROM_END

/* Twinbee Yahoo! */
ROM_START( tbyahhoo )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "426jaa02.31b", 0x200002, 512*1024, 0x0416ad78 )
	ROM_LOAD32_WORD_SWAP( "424jaa04.27b", 0x200000, 512*1024, 0xbcbe0e40 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("424a06.9c", 0x000000, 128*1024, 0xa4760e14 )
	ROM_LOAD16_BYTE("424a07.7c", 0x000001, 128*1024, 0xfa90d7e2 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "424a14.17h", 0x000000, 2*1024*1024, 0xb1d9fce8 )
	ROM_LOADTILE_BYTE( "424a12.13g", 0x000004, 512*1024, 0x7f9cb8b1 )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "424a11.25g", 0x000000, 2*1024*1024, 0x29592688 )
	ROM_LOAD32_WORD( "424a10.28g", 0x000002, 2*1024*1024, 0xcf24e5e3 )
	ROM_LOAD( "424a09.30g", 0x400000, 1*1024*1024, 0xdaa07224 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "424a17.9g", 0x000000, 2*1024*1024, 0xe9dd9692 )
	ROM_LOAD( "424a18.7g", 0x200000, 2*1024*1024, 0x0f0d9f3a )
ROM_END

/* Daisu-Kiss */
ROM_START( daiskiss )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "535jaa02.31b", 0x200002, 512*1024, 0xe5b3e0e5 )
	ROM_LOAD32_WORD_SWAP( "535jaa03.27b", 0x200000, 512*1024, 0x9dc10140 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("535a08.9c", 0x000000, 128*1024, 0x449416a7 )
	ROM_LOAD16_BYTE("535a09.7c", 0x000001, 128*1024, 0x8ec57ab4 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "535a19.17h", 0x000000, 2*1024*1024, 0xfa1c59d1 )
	ROM_LOADTILE_BYTE( "535a18.13g", 0x000004, 512*1024,    0xd02e5103 )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "535a17.25g", 0x000000, 1*1024*1024, 0xb12070e2 )
	ROM_LOAD32_WORD( "535a13.28g", 0x000002, 1*1024*1024, 0x10cf9d05 )
	ROM_LOAD( "535a11.30g", 0x400000, 512*1024, 0x2b176b0f )

	/* sound data */
	ROM_REGION( 0x200000, REGION_SOUND1, 0)
	ROM_LOAD( "535a22.9g", 0x000000, 2*1024*1024, 0x7ee59acb )
ROM_END


/* Sexy Parodius */
ROM_START( sexyparo )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "533a02.31b", 0x200002, 512*1024, 0xb8030abc )
	ROM_LOAD32_WORD_SWAP( "533a03.27b", 0x200000, 512*1024, 0x4a95e80d )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("533a08.9c", 0x000000, 128*1024, 0x06d14cff )
	ROM_LOAD16_BYTE("533a09.7c", 0x000001, 128*1024, 0xa93c6f0b )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "533a19.17h", 0x000000, 2*1024*1024, 0x3ec1843e )
	ROM_LOADTILE_BYTE( "533-ja.13g", 0x000004, 512*1024,    0xd3e0d058  )

	/* sprites */
	ROM_REGION( 0x600000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "533a17.25g", 0x000000, 2*1024*1024, 0x9947af57 )
	ROM_LOAD32_WORD( "533a13.28g", 0x000002, 2*1024*1024, 0x58f1fc38 )
	ROM_LOAD( "533a11.30g", 0x400000, 2*1024*1024, 0x983105e1 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "533a22.9g", 0x000000, 2*1024*1024, 0x97233814 )
	ROM_LOAD( "533a23.7g", 0x200000, 2*1024*1024, 0x1bb7552b )
ROM_END

/* Run and Gun 2 */
ROM_START( rungun2 )
	/* main program */
	ROM_REGION( 0x600000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "505a02", 0x200002, 512*1024, 0xcfca23f7 )
	ROM_LOAD32_WORD_SWAP( "505a03", 0x200000, 512*1024, 0xad7f9ded )

	/* data roms */
	ROM_LOAD32_WORD_SWAP( "505a04", 0x400000, 1024*1024, 0x11a73f01 )
	ROM_LOAD32_WORD_SWAP( "505a05", 0x400002, 1024*1024, 0x5da5d695 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("505a06", 0x000000, 128*1024, 0x920013f1 )
	ROM_LOAD16_BYTE("505a07", 0x000001, 128*1024, 0x5641c603 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "505a20", 0x000000, 2*1024*1024, 0x2bae0fb6 )
	ROM_LOADTILE_BYTE( "505a21", 0x000004, 1024*1024,   0x03fda175 )

	/* sprites */
	ROM_REGION( 0x600000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "505a19", 0x000000, 2*1024*1024, 0xffde4f17 )
	ROM_LOAD32_WORD( "505a15", 0x000002, 2*1024*1024, 0xd9ab1e6c )
	ROM_LOAD( "505a11", 0x400000, 2*1024*1024, 0x75c13df0 )

	/* PSAC2 tiles */
	ROM_REGION( 0x200000, REGION_GFX3, 0)
	ROM_LOAD("505a24", 0x000000, 2*1024*1024, 0x70e906da )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "505a22", 0x000000, 2*1024*1024, 0xc2b67a9d )
	ROM_LOAD( "505a23", 0x200000, 2*1024*1024, 0x67f03445 )
ROM_END

/* Taisen Tokkae-dama */
ROM_START( tokkae )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "615jaa02.31b", 0x200002, 512*1024, 0xf66d6dbf )
	ROM_LOAD32_WORD_SWAP( "615jaa03.27b", 0x200000, 512*1024, 0xb7760e2b )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("615a08.9c", 0x000000, 128*1024, 0xa5340de4 )
	ROM_LOAD16_BYTE("615a09.7c", 0x000001, 128*1024, 0xc61f954c )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORDS2( "615a19.17h", 0x000000, 1*1024*1024, 0x07749e1e )
	ROM_LOADTILE_BYTES2( "615a20.13c", 0x000004, 512*1024,    0x9911b5a1 )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD32_WORD( "615a17.25g", 0x000000, 2*1024*1024, 0xb864654b )
	ROM_LOAD32_WORD( "615a13.28g", 0x000002, 2*1024*1024, 0x4e8afa1a )
	ROM_LOAD32_WORD( "615a16.18h", 0x400000, 2*1024*1024, 0xdfa0f0fe )
	ROM_LOAD32_WORD( "615a12.27g", 0x400002, 2*1024*1024, 0xfbc563fd )

	ROM_LOAD( "615a11.30g", 0x800000, 2*1024*1024, 0xf25946e4 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "615a22.9g", 0x000000, 2*1024*1024, 0xea7e47dd )
	ROM_LOAD( "615a23.7g", 0x200000, 2*1024*1024, 0x22d71f36 )
ROM_END

/* Tokimeki Memorial Taisen Puzzle-dama */
ROM_START( tkmmpzdm )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "515jab02.31b", 0x200002, 512*1024, 0x60d4d577 )
	ROM_LOAD32_WORD_SWAP( "515jab03.27b", 0x200000, 512*1024, 0xc383413d )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("515a04.9c", 0x000000, 128*1024, 0xa9b7bb45 )
	ROM_LOAD16_BYTE("515a05.7c", 0x000001, 128*1024, 0xdea4ca2f )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORDS2( "515a11.17h", 0x000000, 1024*1024, 0x8689852d )
	ROM_LOADTILE_BYTES2( "515a12.13c", 0x000004, 512*1024, 0x6936f94a )

	/* sprites */
	ROM_REGION( 0x600000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "515a10.25g", 0x000000, 2*1024*1024, 0xe6e7ab7e )
	ROM_LOAD32_WORD( "515a08.28g", 0x000002, 2*1024*1024, 0x5613daea )
	ROM_LOAD( "515a06.30g", 0x400000, 2*1024*1024, 0x13b7b953 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "515a13.9g", 0x000000, 2*1024*1024, 0x4b066b00 )
	ROM_LOAD( "515a14.7g", 0x200000, 2*1024*1024, 0x128cc944 )
ROM_END

/* Winning Spike */
ROM_START( winspike )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "705jaa02.31b", 0x200002, 512*1024, 0x85f11b03 )
	ROM_LOAD32_WORD_SWAP( "705jaa03.27b", 0x200000, 512*1024, 0x1d5e3922 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("705a08.9c", 0x000000, 128*1024, 0x0d531639 )
	ROM_LOAD16_BYTE("705a09.7c", 0x000001, 128*1024, 0x24e58845 )

	/* tiles */
	ROM_REGION( 0xc00000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "705a19.17h", 0x000000, 4*1024*1024, 0x16986d64 )
//	ROM_LOADTILE_BYTE( "705a18.22h", 0x000004, 4*1024*1024, 0x5055ae43 )

	/* sprites */
	ROM_REGION( 0xc00000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "705a17.25g", 0x000000, 4*1024*1024, 0x971d2812 )
	ROM_LOAD32_WORD( "705a13.28g", 0x000002, 4*1024*1024, 0x3b62584b )
	ROM_LOAD( "705a11.30g", 0x400000, 4*1024*1024, 0x68542ce9 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "705a22.9g", 0x000000, 4*1024*1024, 0x1a9246f6 )
ROM_END

/* Soccer Superstars */
ROM_START( soccerss )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "427jaa03.30m", 0x200002, 512*1024, 0xf76add04 )
	ROM_LOAD32_WORD_SWAP( "427jaa02.28m", 0x200000, 512*1024, 0x210f9ba7 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("427a07.6m", 0x000000, 128*1024, 0x8dbaf4c7 )
	ROM_LOAD16_BYTE("427a06.9m", 0x000001, 128*1024, 0x979df65d )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
//	ROM_LOAD( "427a09.137", 0x000000, 2*1024*1024, 0x56bdd480 )
//	ROM_LOAD( "427a12.21r", 0x000000, 2*1024*1024, 0x97d6fd38 )
//	ROM_LOAD( "427a13.21r", 0x000000, 2*1024*1024, 0x97d6fd38 )
	ROM_LOADTILE_WORD( "427a14.143", 0x000000, 512*1024, 0x7575a0ed )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "427a10.25r", 0x000000, 2*1024*1024, 0x6b3ccb41 )
	ROM_LOAD32_WORD( "427a11.23r", 0x000002, 2*1024*1024, 0xc1ca74c1  )
//	ROM_LOAD( "427a18.145", 0x400000, 1*1024*1024, 0xbb6e6ec6 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "427a16.9r", 0x000000, 2*1024*1024,  0x39547265 )
	ROM_LOAD( "427a15.11r", 0x000000, 1*1024*1024, 0x33ce2b8e )
ROM_END

/* Taisen Puzzle-dama */
ROM_START( puzldama )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "315jaa02.31b", 0x200002, 512*1024, 0xe0a35c7d )
	ROM_LOAD32_WORD_SWAP( "315jaa04.27b", 0x200000, 512*1024, 0xabe4f0e7 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("315a06.9c", 0x000000, 128*1024, 0x06580a9f )
	ROM_LOAD16_BYTE("315a07.7c", 0x000001, 128*1024, 0x431c58f3 )

	/* tiles */
	ROM_REGION( 0xf00000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "315a14.17h", 0x000000, 512*1024, 0x0ab731e0)
	ROM_LOADTILE_BYTE( "315a12.13g", 0x000004, 2*1024*1024, 0x3047b8d2 )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "315a11.25g", 0x000000, 2*1024*1024, 0xb8a99c29  )
	ROM_LOAD32_WORD( "315a10.28g", 0x000002, 2*1024*1024, 0x77d175dc )
	ROM_LOAD( "315a09.30g", 0x400000, 1*1024*1024, 0x82580329 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "315a17.9g", 0x000000, 2*1024*1024, 0xea763d61 )
	ROM_LOAD( "315a18.7g", 0x200000, 2*1024*1024, 0x6e416cee )
ROM_END

/* Dragoon Might */
ROM_START( dragoonj )
	/* main program */
	ROM_REGION( 0x600000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "417a02.31b", 0x200002, 512*1024, 0x533cbbd5 )
	ROM_LOAD32_WORD_SWAP( "417a03.27b", 0x200000, 512*1024, 0x8e1f883f )

	/* data roms */
	ROM_LOAD32_WORD_SWAP( "417a04.26c", 0x400002, 1024*1024, 0xdc574747 )
	ROM_LOAD32_WORD_SWAP( "417a05.23c", 0x400000, 1024*1024, 0x2ee2c587 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("417a06.9c", 0x000000, 128*1024, 0x8addbbee )
	ROM_LOAD16_BYTE("417a07.7c", 0x000001, 128*1024, 0xc1fd7584 )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "417a16.17h", 0x000000, 2*1024*1024, 0x88b2213b )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD32_WORD( "417a08.33g", 0x000000, 2*1024*1024, 0x801e9d93 )
	ROM_LOAD32_WORD( "417a12.23h", 0x000002, 2*1024*1024, 0x25496115 )
	ROM_LOAD32_WORD( "417a15.25g", 0x400000, 2*1024*1024, 0x83bccd01 )
	ROM_LOAD32_WORD( "417a11.28g", 0x400002, 2*1024*1024, 0x624a7c4c )
//	ROM_LOAD( "417a13.18h", 0x800000, 2*1024*1024, 0xfbf551f1 )
//	ROM_LOAD( "417a09.30g", 0xa00000, 2*1024*1024, 0xb5653e24 )

	/* sound data */
	ROM_REGION( 0x200000, REGION_SOUND1, 0)
	ROM_LOAD( "417a17.9g", 0x000000, 2*1024*1024, 0x88d47dfd )
ROM_END

/* Vs. Net Soccer */
ROM_START(vsnetscr)
	/* main program */
	ROM_REGION( 0x600000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "627uab02.31m", 0x200002, 512*1024, 0xc352cc6f )
	ROM_LOAD32_WORD_SWAP( "627uab03.29m", 0x200000, 512*1024, 0x53ca7eec )

	/* data roms */
	ROM_LOAD32_WORD_SWAP( "627a04", 0x400002, 1024*1024, 0x17334e9a )
	ROM_LOAD32_WORD_SWAP( "627a05", 0x400000, 1024*1024, 0xbe4e7b3c )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("627b06.9m", 0x000000, 128*1024, 0xc8337b9d )
	ROM_LOAD16_BYTE("627b07.7m", 0x000001, 128*1024, 0xd7d92579 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, 0)
	ROM_LOADTILE_WORD( "627a20", 0x000000, 2*1024*1024, 0x6465f684 )
	ROM_LOADTILE_BYTE( "627a21", 0x000004, 1024*1024,   0xd0755fb8 )

	/* sprites */
	ROM_REGION( 0x600000, REGION_GFX2, 0)
	ROM_LOAD32_WORD( "627a13", 0x000000, 2*1024*1024, 0xaf48849d )
	ROM_LOAD32_WORD( "627a11", 0x000002, 2*1024*1024, 0xa2e507f2 )
	ROM_LOAD( "627a09", 0x400000, 2*1024*1024, 0x312cf8a4 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "627a23", 0x000000, 2*1024*1024, 0xb0266eb6 )
	ROM_LOAD( "627a24", 0x200000, 2*1024*1024, 0x2cd73305 )
ROM_END


/* Racin' Force */
ROM_START( racinfrc )
	/* main program */
	ROM_REGION( 0x800000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_WORD_SWAP( "250uab02.34n", 0x200000, 512*1024, 0x315040c6 )
	ROM_LOAD32_WORD_SWAP( "250uab03.31n", 0x200002, 512*1024, 0x171134ab )

	/* data roms */
	ROM_LOAD32_WORD_SWAP( "250a04.34s", 0x400000, 2*1024*1024, 0x45e4d43c )
	ROM_LOAD32_WORD_SWAP( "250a05.31s", 0x400002, 2*1024*1024, 0xa235af3e )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("250a06.8p", 0x000000, 128*1024, 0x2d0a3ff1 )
	ROM_LOAD16_BYTE("250a07.6p", 0x000001, 128*1024, 0x612b670a )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "250a15.19y", 0x000000, 2*1024*1024, 0xb46ae7c4 )
//	ROM_LOADTILE_BYTE( "250a14.21y", 0x000004, 512*1024,    0xd14abf98 )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD32_WORD( "250a08.36y", 0x000000, 2*1024*1024, 0x25ff6414 )
	ROM_LOAD32_WORD( "250a10.31y", 0x000000, 2*1024*1024, 0x75c02d12 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "250a17.14y", 0x000000, 2*1024*1024, 0xadefa079 )
	ROM_LOAD( "250a18.12y", 0x200000, 2*1024*1024, 0x8014a2eb )
ROM_END

/* Lethal Enforcers II */
ROM_START( le2 )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	GX_BIOS
	ROM_LOAD32_BYTE( "312eaa05.26b", 0x200000, 128*1024, 0x875f6561 )
	ROM_LOAD32_BYTE( "312eaa04.28b", 0x200001, 128*1024, 0xd5fb8d30 )
	ROM_LOAD32_BYTE( "312eaa03.30b", 0x200002, 128*1024, 0xcfe07036 )
	ROM_LOAD32_BYTE( "312eaa02.33b", 0x200003, 128*1024, 0x5094b965 )

	/* sound program */
	ROM_REGION( 0x40000, REGION_CPU2, 0)
	ROM_LOAD16_BYTE("312b06.9c", 0x000000, 128*1024, 0xa6f62539 )
	ROM_LOAD16_BYTE("312b07.7c", 0x000001, 128*1024, 0x1aa19c41 )

	/* tiles */
	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOAD16_BYTE( "312a14.17h", 0x000000, 2*1024*1024, 0xdc862f19 )
	ROM_LOAD16_BYTE( "312a12.22h", 0x000001, 2*1024*1024, 0x98c04ddd )
	ROM_LOAD16_BYTE( "312a15.15h", 0x400000, 2*1024*1024, 0x516f2941 )
	ROM_LOAD16_BYTE( "312a13.20h", 0x400001, 2*1024*1024, 0x16e5fdaa )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "312a08.33g", 0x000000, 2*1024*1024, 0x29015d56 )
	ROM_LOAD64_WORD( "312a09.30g", 0x000002, 2*1024*1024, 0xb2c5d6d5 )
	ROM_LOAD64_WORD( "312a10.28g", 0x000004, 2*1024*1024, 0x3c570d04 )
	ROM_LOAD64_WORD( "312a11.25g", 0x000006, 2*1024*1024, 0x5f474357 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "312a17.9g", 0x000000, 2*1024*1024, 0xed101448 )
	ROM_LOAD( "312a18.7g", 0x200000, 1*1024*1024, 0x5717abd7 )
ROM_END

static DRIVER_INIT(konamigx)
{
	tms57002_init();
	esc_cb = 0;

	gx_irqregister = 0;

	state_save_register_UINT8("KonamiGX", 0, "IRQ enable", &gx_irqregister, 1);
	state_save_register_UINT8("KonamiGX", 0, "Sound comms 1", sndto020, 16);
	state_save_register_UINT8("KonamiGX", 0, "Sound comms 2", sndto000, 16);
}

static DRIVER_INIT(gokuparo)
{
	init_konamigx();
}

static DRIVER_INIT(sexyparo)
{
	init_konamigx();
	esc_cb = sexyparo_esc;
}

static DRIVER_INIT(sal2)
{
	init_konamigx();
	esc_cb = sal2_esc;
}

static DRIVER_INIT(tbyahhoo)
{
	init_konamigx();
	gx_irqregister = 0x81;
	esc_cb = tbyahhoo_esc;
}

/*          ROM   parent  machine   inp    	init */
/* dummy parent for the BIOS */
GAMEX(1994, konamigx, 0, konamigx, konamigx, konamigx, ROT0, "Konami", "System GX", NOT_A_DRIVER )

/* Type 1: standard with an add-on 53936 on the ROM board and analog inputs */
/* Racin' Force boots but needs a lot of work, including analog inputs */
/* Golfing Greats 2 is on this type of board as well */
GAMEX( 1994, racinfrc, konamigx, gxtype1,  racinfrc, konamigx, ROT0, "Konami", "Racin' Force (version UAB)", GAME_NOT_WORKING )

/* Type 2: totally stock, sometimes with funny protection chips on the ROM board */

/* these games work and are playable with minor graphics glitches */
GAMEX( 1994, le2,      konamigx, le2,      le2,      konamigx, ROT0, "Konami", "Lethal Enforcers II: Gun Fighters (Ver EAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, gokuparo, konamigx, konamigx, gokuparo, gokuparo, ROT0, "Konami", "Gokujyou Parodius (Ver JAD)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, puzldama, konamigx, puzzle,   puzldama, konamigx, ROT0, "Konami", "Taisen Puzzle-dama (Ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, sexyparo, konamigx, konamigx, gokuparo, sexyparo, ROT0, "Konami", "Sexy Parodius (Ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, daiskiss, konamigx, daiskiss, gokuparo, tbyahhoo, ROT0, "Konami", "Daisu-Kiss (Ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, tokkae,   konamigx, tokkae,   puzldama, konamigx, ROT0, "Konami", "Tokkae Puzzle-dama (Ver JAA)", GAME_IMPERFECT_GRAPHICS )

/* these games work and are playable but have major gfx problems */
GAMEX( 1995, tbyahhoo, konamigx, tbyahhoo, gokuparo, tbyahhoo, ROT0, "Konami", "Twin Bee Yahhoo! (Ver JAA)", GAME_IMPERFECT_GRAPHICS )

/* these games run but have missing sprites due to protection */
GAMEX( 1995, dragoonj, konamigx, dragoonj, puzldama, konamigx, ROT0, "Konami", "Dragoon Might (Ver JAA)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1995, tkmmpzdm, konamigx, konamigx_6bpp, puzldama, konamigx, ROT0, "Konami", "Tokimeki Memorial Taisen Puzzle-dama (version JAB)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1996, salmndr2, konamigx, konamigx_6bpp_2, gokuparo, sal2, ROT0, "Konami", "Salamander 2", GAME_UNEMULATED_PROTECTION )

/* these games are unplayable due to protection */
GAMEX( 1994, fantjour, gokuparo, konamigx, konamigx, gokuparo, ROT0, "Konami", "Fantastic Journey", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION )
GAMEX( 1997, winspike, konamigx, konamigx_6bpp, konamigx, konamigx, ROT0, "Konami", "Winning Spike (Ver JAA)", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION )

/* Type 3: 53936 on the ROM board */
GAMEX( 1994, soccerss, konamigx, gxtype3,  type3, konamigx, ROT0, "Konami", "Soccer Superstars (Ver JAA)", GAME_NOT_WORKING )
GAMEX( 1996, vsnetscr, konamigx, gxtype3,  type3, konamigx, ROT0, "Konami", "Versus Net Soccer (Ver UAB)", GAME_NOT_WORKING )

/* Type 4: dual monitor output and 53936 on the ROM board.  Doesn't use the GX's own
   palette RAM or color mixer.  Additionally, every other frame (!) of the video is
   directed to a different monitor to create the dual-mon effect.  Probably should be a
   separate driver. */
GAMEX( 1996, rungun2,  konamigx, gxtype4,  type3,    sexyparo, ROT0, "Konami", "Run and Gun 2 (Ver UAA)", GAME_NOT_WORKING )
