/****************************************************************************

	Data East protection/IO chips


	Game						Custom chip number
	------------------------------------------------
	Edward Randy					60
	Mutant Fighter					66
	Captain America					75
	Robocop 2						75
	Lemmings						75
	Caveman Ninja					104
	Wizard Fire						104
	Pocket Gal DX					104
	Boogie Wings					104
	Rohga							104
	Diet GoGo						104
	Funky Jet						146
	Nitro Ball						146
	Super Shanghai Dragon's Eye		146
	Dragon Gun						146
	Fighter's History				? (ID scratched off)
	Tattoo Assassins				?

	This series of chips is used for I/O and copy protection.  They are all
	16 bit word based chips, with 0x400 write addresses, and 0x400 read
	addresses.  The basic idea of the protection is that read & write
	addresses don't match.  So if you write to write address 0, you might
	read that data back at read address 10, and something else will be at 0.
	In addition, the data read back may be bit shifted in various ways.
	Games can be very well protected by writing variables to the chip, and
	expecting certain values back from certain read addresses.  With care,
	it can be impossible to tell from the game code what values go where,
	and what shifting goes on.  Eg, writing 10 values to the chip, of which
	7 are dummy values, then reading back 3 particular values, and using them
	in a multiplication and add calculation to get a jump address for the
	program.  Even if you can guess one of many possible legal jump addresses
	it's hard to tell what values should be bit shifted in what way.

	It's also been found some chips contain a hardwired XOR port and hardwired
	NAND port which affects only certain read values.

	The chips also handle the interface to the sound cpu via a write address,
	and game inputs via several read addresses.  The I/O input data is also
	mirrored to several locations, some with bit shifting.

	Although several games use chip 104, each seems to be different, the
	address lines leading to the chip on each game are probably arranged
	differently, to further scramble reads/writes.  From hardware tests
	chips 60 && 66 appear to be identical.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "state.h"

#define DECO_PORT(p) (deco16_prot_ram[p/2])

data16_t *deco16_prot_ram;
data32_t *deco32_prot_ram;

/***************************************************************************/

WRITE16_HANDLER( deco16_104_prot_w ) /* Wizard Fire */
{
	if (offset==(0x150/2)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

	if (offset!=(0x150>>1) && offset!=(0x0>>1) && offset!=(0x110>>1) && offset!=(0x280>>1)
		&& offset!=(0x290>>1) && offset!=(0x2b0>>1) && offset!=(0x370>>1) && offset!=(0x3c0>>1)
		&& offset!=(0x370>>1) && offset!=(0x3c0>>1) && offset!=(0x430>>1) && offset!=(0x460>>1)
		&& offset!=(0x5a0>>1) && offset!=(0x5b0>>1) && offset!=(0x6e0>>1) && offset!=(0x7d0>>1)
		)
		logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset<<1,data);

	COMBINE_DATA(&deco16_prot_ram[offset]);
}

READ16_HANDLER( deco16_104_prot_r ) /* Wizard Fire */
{
	switch (offset<<1) {
		case 0x110: /* Player input */
			return readinputport(0);

		case 0x36c: /* Coins */
		case 0x334: /* Probably also, c6, 2c0, 2e0, 4b2, 46a, 4da, rohga is 44c */
			return readinputport(1);
		case 0x0dc:
			return readinputport(1)<<4;

		case 0x494: /* Dips */
			return readinputport(2);

		case 0x244:
			return deco16_prot_ram[0];
		case 0x7cc:
			return ((deco16_prot_ram[0]&0x000f)<<12) | ((deco16_prot_ram[0]&0x00f0)<<4) | ((deco16_prot_ram[0]&0x0f00)>>4) | ((deco16_prot_ram[0]&0xf000)>>12);
		case 0x0c0:
			return (((deco16_prot_ram[0]&0x000e)>>1) | ((deco16_prot_ram[0]&0x0001)<<3))<<12;
		case 0x188:
			return (((deco16_prot_ram[0]&0x000e)>>1) | ((deco16_prot_ram[0]&0x0001)<<3))<<12;
		case 0x65e:
			return (((deco16_prot_ram[0]&0x000c)>>2) | ((deco16_prot_ram[0]&0x0003)<<2))<<12;
		case 0x5ce:
			return ((deco16_prot_ram[0]<<8)&0xf000) | ((deco16_prot_ram[0]&0xe)<<7) | ((deco16_prot_ram[0]&0x1)<<11);
		case 0x61a:
			return (deco16_prot_ram[0]<<8)&0xff00;

		case 0x496:
			return deco16_prot_ram[0x110/2];
		case 0x40a:
			return ((deco16_prot_ram[0x110/2]&0x000f)<<12) | ((deco16_prot_ram[0x110/2]&0x00f0)>>4) | ((deco16_prot_ram[0x110/2]&0x0f00)<<0) | ((deco16_prot_ram[0x110/2]&0xf000)>>8);
		case 0x1e8:
			return ((deco16_prot_ram[0x110/2]&0x00ff)<<8) | ((deco16_prot_ram[0x110/2]&0xff00)>>8);
		case 0x4bc:
			return ((deco16_prot_ram[0x110/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x110/2]&0x0003)<<6) | ((deco16_prot_ram[0x110/2]&0x000c)<<2);
		case 0x46e:
			return ((deco16_prot_ram[0x110/2]&0xfff0)<<0) | ((deco16_prot_ram[0x110/2]&0x0007)<<1) | ((deco16_prot_ram[0x110/2]&0x0008)>>3);
		case 0x264:
			return ((deco16_prot_ram[0x110/2]&0x000f)<<8) | ((deco16_prot_ram[0x110/2]&0x00f0)>>0) | ((deco16_prot_ram[0x110/2]&0x0f00)<<4);
		case 0x172:
			return ((deco16_prot_ram[0x110/2]&0x000f)<<4) | ((deco16_prot_ram[0x110/2]&0x00f0)<<4) | ((deco16_prot_ram[0x110/2]&0xf000)<<0);

		case 0x214:
			return deco16_prot_ram[0x280/2];
		case 0x52e:
			return ((deco16_prot_ram[0x280/2]&0x000f)<<8) | ((deco16_prot_ram[0x280/2]&0x00f0)>>0) | ((deco16_prot_ram[0x280/2]&0x0f00)>>8) | ((deco16_prot_ram[0x280/2]&0xf000)>>0);
		case 0x07a:
			return ((deco16_prot_ram[0x280/2]&0x000f)<<8) | ((deco16_prot_ram[0x280/2]&0x00f0)>>0) | ((deco16_prot_ram[0x280/2]&0x0f00)>>8) | ((deco16_prot_ram[0x280/2]&0xf000)>>0);
		case 0x360:
			return ((deco16_prot_ram[0x280/2]&0x000f)<<8) | ((deco16_prot_ram[0x280/2]&0x00f0)>>0) | ((deco16_prot_ram[0x280/2]&0x0f00)>>8) | ((deco16_prot_ram[0x280/2]&0xf000)>>0);
		case 0x4dc:
			return ((deco16_prot_ram[0x280/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x280/2]&0x0007)<<5) | ((deco16_prot_ram[0x280/2]&0x0008)<<1);
		case 0x3a8:
			return ((deco16_prot_ram[0x280/2]&0x000e)<<3) | ((deco16_prot_ram[0x280/2]&0x0001)<<7) | ((deco16_prot_ram[0x280/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x280/2]&0xf000)>>12);
		case 0x2f6:
			return ((deco16_prot_ram[0x280/2]&0xff00)>>8) | ((deco16_prot_ram[0x280/2]&0x00f0)<<8) | ((deco16_prot_ram[0x280/2]&0x000c)<<6) | ((deco16_prot_ram[0x280/2]&0x0003)<<10);

		case 0x7e4:
			return (deco16_prot_ram[0x290/2]&0x00f0)<<8;

		case 0x536:
			return ((deco16_prot_ram[0x2b0/2]&0x000f)<<8) | ((deco16_prot_ram[0x2b0/2]&0x00f0)<<0) | ((deco16_prot_ram[0x2b0/2]&0x0f00)<<4) | ((deco16_prot_ram[0x2b0/2]&0xf000)>>12);

		case 0x0be:
			return ((deco16_prot_ram[0x370/2]&0x000f)<<4) | ((deco16_prot_ram[0x370/2]&0x00f0)<<4) | ((deco16_prot_ram[0x370/2]&0x0f00)>>8) | ((deco16_prot_ram[0x370/2]&0xf000)>>0);

		case 0x490:
			return (deco16_prot_ram[0x3c0/2]&0xfff0) | ((deco16_prot_ram[0x3c0/2]&0x0007)<<1) | ((deco16_prot_ram[0x3c0/2]&0x0008)>>3);

		case 0x710:
			return (deco16_prot_ram[0x430/2]&0xfff0) | ((deco16_prot_ram[0x430/2]&0x0007)<<1) | ((deco16_prot_ram[0x430/2]&0x0008)>>3);

		case 0x22a:
			return ((deco16_prot_ram[0x5a0/2]&0xff00)>>8) | ((deco16_prot_ram[0x5a0/2]&0x00f0)<<8) | ((deco16_prot_ram[0x5a0/2]&0x0001)<<11) | ((deco16_prot_ram[0x5a0/2]&0x000e)<<7);

		case 0x626:
			return ((deco16_prot_ram[0x5b0/2]&0x000f)<<8) | ((deco16_prot_ram[0x5b0/2]&0x00f0)<<8) | ((deco16_prot_ram[0x5b0/2]&0x0f00)>>4) | ((deco16_prot_ram[0x5b0/2]&0xf000)>>12);

		case 0x444:
			return deco16_prot_ram[0x604/2]; //rohga

		case 0x5ac:
			return ((deco16_prot_ram[0x6e0/2]&0xfff0)>>4) | ((deco16_prot_ram[0x6e0/2]&0x0007)<<13) | ((deco16_prot_ram[0x6e0/2]&0x0008)<<9);

		case 0x650:
			return ((deco16_prot_ram[0x7d0/2]&0xfff0)>>4) | ((deco16_prot_ram[0x7d0/2]&0x000f)<<12);

		case 0x4ac:
			return ((deco16_prot_ram[0x460/2]&0x0007)<<13) | ((deco16_prot_ram[0x460/2]&0x0008)<<9);
	}

	logerror("Deco Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
	return 0;
}

/***************************************************************************/

WRITE16_HANDLER( deco16_60_prot_w ) /* Edward Randy */
{
	if (offset==(0x64/2)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
	}

	COMBINE_DATA(&deco16_prot_ram[offset]);

// 0 4 2c 32 34 36 3c 3e 40 54 56 58 6a 76 80 84 88 8a 8c 8e 90 92 94 96 9e a0 a2 a4 a6 a8 aa ac ae b0

if (offset!=0x32 && offset!=0x36/2 && offset!=0x9e/2 && offset!=0x76/2
	&& offset!=4/2 && offset!=0x2c/2 && offset!=0x3c/2 && offset!=0x3e/2
	&& offset!=0x80/2 && offset!=0x84/2 &&offset!=0x88/2 && offset!=0x8c/2 && offset!=0x90/2 && offset!=0x94/2
	&& offset!=0xa0/2 &&offset!=0xa2/2 && offset!=0xa4/2 && offset!=0xa6/2 && offset!=0xa8/2
	&& offset!=0xaa/2 && offset!=0xac/2 && offset!=0xae/2 && offset!=0xb0/2
	&& (offset<0xd0/2 || offset>0xe0/2)
	&& (offset<4 || offset>17)
	&& offset!=0x40/2 && offset!=0x54/2 && offset!=0x56/2 && offset!=0x58/2 && offset!=0x6a/2 && offset!=0x2c/2
	&& offset!=0 && offset!=0x34 && offset!=0x8a && offset!=0x8e && offset!=0x92 && offset!=0x96
	)
logerror("Protection PC %06x: warning - write %04x to %04x\n",activecpu_get_pc(),data,offset<<1);

}

READ16_HANDLER( deco16_60_prot_r ) /* Edward Randy */
{
	switch (offset<<1) {
		/* Video registers */
		case 0x32a: /* Moved to 0x140006 on int */
			return deco16_prot_ram[0x80/2];
		case 0x380: /* Moved to 0x140008 on int */
			return deco16_prot_ram[0x84/2];
		case 0x63a: /* Moved to 0x150002 on int */
			return deco16_prot_ram[0x88/2];
		case 0x42a: /* Moved to 0x150004 on int */
			return deco16_prot_ram[0x8c/2];
		case 0x030: /* Moved to 0x150006 on int */
			return deco16_prot_ram[0x90/2];
		case 0x6b2: /* Moved to 0x150008 on int */
			return deco16_prot_ram[0x94/2];

		case 0x6fa:
			return deco16_prot_ram[0x4/2];
		case 0xe4:
			return (deco16_prot_ram[0x4/2]&0xf000)|((deco16_prot_ram[0x4/2]&0x00ff)<<4)|((deco16_prot_ram[0x4/2]&0x0f00)>>8);

		case 0x390:
			return deco16_prot_ram[0x2c/2];
		case 0x3b2:
			return deco16_prot_ram[0x3c/2];
		case 0x440:
			return deco16_prot_ram[0x3e/2];

		case 0x6fc:
			return deco16_prot_ram[0x66/2];

		case 0x15a:
			return deco16_prot_ram[0xa0/2];
		case 0x102:
			return deco16_prot_ram[0xa2/2];
		case 0x566:
			return deco16_prot_ram[0xa4/2];
		case 0xd2:
			return deco16_prot_ram[0xa6/2];
		case 0x4a6:
			return deco16_prot_ram[0xa8/2];
		case 0x3dc:
			return deco16_prot_ram[0xaa/2];
		case 0x2a0:
			return deco16_prot_ram[0xac/2];
		case 0x392:
			return deco16_prot_ram[0xae/2];
		case 0x444:
			return deco16_prot_ram[0xb0/2];

		case 0x5ea:
			return deco16_prot_ram[0xb8/2];
		case 0x358:
			return deco16_prot_ram[0xba/2];
		case 0x342:
			return deco16_prot_ram[0xbc/2];
		case 0x3c:
			return deco16_prot_ram[0xbe/2];
		case 0x656:
			return deco16_prot_ram[0xc0/2];
		case 0x18c:
			return deco16_prot_ram[0xc2/2];
		case 0x370:
			return deco16_prot_ram[0xc4/2];
		case 0x5c6:
			return deco16_prot_ram[0xc6/2];

			/* C8 written but not read */

		case 0x248:
			return deco16_prot_ram[0xd0/2];
		case 0x1ea:
			return deco16_prot_ram[0xd2/2];
		case 0x4cc:
			return deco16_prot_ram[0xd4/2];
		case 0x724:
			return deco16_prot_ram[0xd6/2];
		case 0x578:
			return deco16_prot_ram[0xd8/2];
		case 0x63e:
			return deco16_prot_ram[0xda/2];
		case 0x4ba:
			return deco16_prot_ram[0xdc/2];
		case 0x1a:
			return deco16_prot_ram[0xde/2];
		case 0x120:
			return deco16_prot_ram[0xe0/2];
		case 0x7c2: /* (Not checked for mask/xor but seems standard) */
			return deco16_prot_ram[0x50/2];

		/* memcpy selectors, transfer occurs in interrupt */
		case 0x32e: return deco16_prot_ram[4]; /* src msb */
		case 0x6d8: return deco16_prot_ram[5]; /* src lsb */
		case 0x010: return deco16_prot_ram[6]; /* dst msb */
		case 0x07a: return deco16_prot_ram[7]; /* src lsb */

		case 0x37c: return deco16_prot_ram[8]; /* src msb */
		case 0x250: return deco16_prot_ram[9];
		case 0x04e: return deco16_prot_ram[10];
		case 0x5ba: return deco16_prot_ram[11];
		case 0x5f4: return deco16_prot_ram[12]; /* length */

		case 0x38c: return deco16_prot_ram[13]; /* src msb */
		case 0x02c: return deco16_prot_ram[14];
		case 0x1e6: return deco16_prot_ram[15];
		case 0x3e4: return deco16_prot_ram[16];
		case 0x174: return deco16_prot_ram[17]; /* length */

		/* Player 1 & 2 controls, read in IRQ then written *back* to protection device */
		case 0x50: /* written to 9e byte */
			return readinputport(0);
		case 0x6f8: /* written to 76 byte */
			return (readinputport(0)>>8)|(readinputport(0)<<8); /* byte swap IN0 */

		case 0x5c: /* After coin insert, high 0x8000 bit set starts game */
			return deco16_prot_ram[0x3b];
		case 0x3a6: /* Top byte OR'd with above, masked to 7 */
			return deco16_prot_ram[0x9e/2];
		case 0xc6:
			return ((deco16_prot_ram[0x9e/2]&0xff00)>>8) | ((deco16_prot_ram[0x9e/2]&0x00ff)<<8);

		case 0xac: /* Dip switches */
			return readinputport(2);
		case 0xc2:
			return readinputport(2) ^ deco16_prot_ram[0x2c/2];

		case 0x5d4: /* The state of the dips last frame */
			return deco16_prot_ram[0x34/2];

		case 0x7bc:
			return ((deco16_prot_ram[0x76/2]&0xff00)>>8) | ((deco16_prot_ram[0x76/2]&0x00ff)<<8);

		case 0x2f6: /* Stage clear flag */
			return (((deco16_prot_ram[0]&0xfff0)>>0) | ((deco16_prot_ram[0]&0x000c)>>2) | ((deco16_prot_ram[0]&0x0003)<<2)) & (~deco16_prot_ram[0x36/2]);

		case 0x76a: /* Coins */
			return readinputport(1);

		case 0x284: /* Bit shifting with inverted mask register */
			return (((deco16_prot_ram[0x40/2]&0xfff0)>>0) | ((deco16_prot_ram[0x40/2]&0x0007)<<1) | ((deco16_prot_ram[0x40/2]&0x0008)>>3)) & (~deco16_prot_ram[0x36/2]);
		case 0x6c4: /* Bit shifting with inverted mask register */
			return (((deco16_prot_ram[0x54/2]&0xf000)>>4) | ((deco16_prot_ram[0x54/2]&0x0f00)>>4) | ((deco16_prot_ram[0x54/2]&0x00f0)>>4) | ((deco16_prot_ram[0x54/2]&0x0003)<<14) | ((deco16_prot_ram[0x54/2]&0x000c)<<10)) & (~deco16_prot_ram[0x36/2]);
		case 0x33e: /* Bit shifting with inverted mask register */
			return (((deco16_prot_ram[0x56/2]&0xff00)>>0) | ((deco16_prot_ram[0x56/2]&0x00f0)>>4) | ((deco16_prot_ram[0x56/2]&0x000f)<<4)) & (~deco16_prot_ram[0x36/2]);
		case 0x156: /* Bit shifting with inverted mask register */
			return (((deco16_prot_ram[0x58/2]&0xfff0)>>4) | ((deco16_prot_ram[0x58/2]&0x000e)<<11) | ((deco16_prot_ram[0x58/2]&0x0001)<<15)) & (~deco16_prot_ram[0x36/2]);
		case 0x286: /* Bit shifting with inverted mask register */
			return (((deco16_prot_ram[0x6a/2]&0x00f0)<<4) | ((deco16_prot_ram[0x6a/2]&0x0f00)<<4) | ((deco16_prot_ram[0x6a/2]&0x0007)<<5) | ((deco16_prot_ram[0x6a/2]&0x0008)<<1)) & (~deco16_prot_ram[0x36/2]);

		case 0x7d6: /* XOR IN0 */
			return readinputport(0) ^ deco16_prot_ram[0x2c/2];
		case 0x4b4:
			return ((deco16_prot_ram[0x32/2]&0x00f0)<<8) | ((deco16_prot_ram[0x32/2]&0x000e)<<7) | ((deco16_prot_ram[0x32/2]&0x0001)<<11);
	}

	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset*2);
	return 0;
}

/***************************************************************************/

static int mutantf_port_0e_hack=0, mutantf_port_6a_hack=0,mutantf_port_e8_hack=0;

WRITE16_HANDLER( deco16_66_prot_w ) /* Mutant Fighter */
{
	if (offset==(0x64/2)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

	COMBINE_DATA(&deco16_prot_ram[offset]);

	/* See below */
	if (offset==(0xe/2))
		mutantf_port_0e_hack=data;
	else
		mutantf_port_0e_hack=0x800;

	if (offset==(0x6a/2))
		mutantf_port_6a_hack=data;
	else
		mutantf_port_6a_hack=0x2866;

	if (offset==(0xe8/2))
		mutantf_port_e8_hack=data;
	else
		mutantf_port_e8_hack=0x2401;

//	2 4 c e 18 1e 22 2c 2e 34 36 38 3a 42 48 58 6a 72 7a 82 88 92 a2 a4 aa b0 b6 b8 dc e4 e8 f4 fa 1c8 308 7e8 40e
	offset=offset<<1;
	if (offset!=0x02 && offset!=0xc && offset!=0xe && offset!=0x18 && offset!=0x2c && offset!=0x2e && offset!=0x34
		&& offset!=0x42 && offset!=0x58 && offset!=0x6a && offset!=0x72 && offset!=0x7a && offset!=0xb8
		&& offset!=0xdc && offset!=0xe8 && offset!=0xf4 && offset!=0x1c8 && offset!=0x7e8
		&& offset!=0xe && offset!=0x48 && offset!=0xaa && offset!=0xb0 && offset!=0x36
		&& offset!=0xa4 && offset!=0x4 && offset!=0x82 && offset!=0x88 && offset!=0x22
		&& offset!=0xb6 && offset!=0xfa && offset!=0xe4 && offset!=0x3a && offset!=0x1e
		&& offset!=0x38 && offset!=0x92 && offset!=0xa2 && offset!=0x308 && offset!=0x40e
	)
	logerror("Protection PC %06x: warning - write %04x to %04x\n",activecpu_get_pc(),data,offset);
}

READ16_HANDLER( deco16_66_prot_r ) /* Mutant Fighter */
{
	if (offset!=0xe/2)
		mutantf_port_0e_hack=0x0800;
	if (offset!=0x6a/2)
		mutantf_port_6a_hack=0x2866;

 	switch (offset*2) {
		case 0xac: /* Dip switches */
			return readinputport(2);
		case 0xc2: /* Dip switches */
			return readinputport(2) ^ deco16_prot_ram[0x2c/2];
		case 0x46: /* Coins */
			return readinputport(1) ^ deco16_prot_ram[0x2c/2];
		case 0x50: /* Player 1 & 2 input ports */
			return readinputport(0);
		case 0x63c: /* Player 1 & 2 input ports */
			return readinputport(0) ^ deco16_prot_ram[0x2c/2];

		case 0x5f4:
			return deco16_prot_ram[0x18/2];
		case 0x7e8:
			return deco16_prot_ram[0x58/2];
		case 0x1c8:
			return deco16_prot_ram[0x6a/2];
		case 0x10:
			return deco16_prot_ram[0xc/2];
		case 0x672:
			return deco16_prot_ram[0x72/2];
		case 0x5ea:
			return deco16_prot_ram[0xb8/2];
		case 0x1e8:
			return deco16_prot_ram[0x2/2];
		case 0xf6:
			return deco16_prot_ram[0x42/2];
		case 0x692:
			return deco16_prot_ram[0x2e/2];
		case 0x63a:
			return deco16_prot_ram[0x88/2];
		case 0x7a:
			return deco16_prot_ram[0xe/2];
		case 0x40e:
			return deco16_prot_ram[0x7a/2];
		case 0x602:
			return deco16_prot_ram[0x92/2];
		case 0x5d4:
			return deco16_prot_ram[0x34/2];
		case 0x6fa:
			return deco16_prot_ram[0x4/2];
		case 0x3dc:
			return deco16_prot_ram[0xaa/2];
		case 0x444:
			return deco16_prot_ram[0xb0/2];
		case 0x102:
			return deco16_prot_ram[0xa2/2];
		case 0x458:
			return deco16_prot_ram[0xb6/2];
		case 0x2a6:
			return deco16_prot_ram[0xe8/2];
		case 0x626:
			return deco16_prot_ram[0xf4/2];
		case 0x762:
			return deco16_prot_ram[0x82/2];
		case 0x308:
			return deco16_prot_ram[0x38/2];
		case 0x1e6:
			return deco16_prot_ram[0x1e/2];
		case 0x566:
			return deco16_prot_ram[0xa4/2];
		case 0x5b6:
			return deco16_prot_ram[0xe4/2];
		case 0x77c:
			return deco16_prot_ram[0xfa/2];
		case 0x4ba:
			return deco16_prot_ram[0xdc/2];

		case 0x1e:
			return deco16_prot_ram[0xf4/2] ^ deco16_prot_ram[0x2c/2];
		case 0x18e:
			return ((deco16_prot_ram[0x1e/2]&0x000f)<<12) | ((deco16_prot_ram[0x1e/2]&0x0ff0)>>0) | ((deco16_prot_ram[0x1e/2]&0xf000)>>12);
		case 0x636:
			return ((deco16_prot_ram[0x18/2]&0x00ff)<<8) | ((deco16_prot_ram[0x18/2]&0x0f00)>>4) | ((deco16_prot_ram[0x18/2]&0xf000)>>12);
		case 0x7d4:
			return ((deco16_prot_ram[0xc/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xc/2]&0x000c)<<2) | ((deco16_prot_ram[0xc/2]&0x0003)<<6);
		case 0x542:
			return ((deco16_prot_ram[0x92/2]&0x00ff)<<8) ^ deco16_prot_ram[0x2c/2];
		case 0xb0:
			return (((deco16_prot_ram[0xc/2]&0x000f)<<12) | ((deco16_prot_ram[0xc/2]&0x00f0)<<4) | ((deco16_prot_ram[0xc/2]&0xff00)>>8)) ^ deco16_prot_ram[0x2c/2];
		case 0x4:
			return (((deco16_prot_ram[0x18/2]&0x00f0)<<8) | ((deco16_prot_ram[0x18/2]&0x0003)<<10) | ((deco16_prot_ram[0x18/2]&0x000c)<<6)) & (~deco16_prot_ram[0x36/2]);

		case 0xe: /* On real hardware this value only seems to persist for 1 read or write, then reverts to 0800.  Hmm */
			{
				int ret=mutantf_port_0e_hack;
				mutantf_port_0e_hack=0x800;
				//logerror("Protection PC %06x: warning - read unknown memory address %04x\n",activecpu_get_pc(),offset<<1);
				return ret;
			}

		case 0x6a: /* On real hardware this value only seems to persist for 1 read or write, then reverts to 0x2866.  Hmm */
			{
				int ret=mutantf_port_6a_hack;
				mutantf_port_6a_hack=0x2866;
				//logerror("Protection PC %06x: warning - read unknown memory address %04x\n",activecpu_get_pc(),offset<<1);
				return ret;
			}

		case 0xe8: /* On real hardware this value only seems to persist for 1 read or write, then reverts to 0x2401.  Hmm */
			{
				int ret=mutantf_port_e8_hack;
				mutantf_port_e8_hack=0x2401;
				//logerror("Protection PC %06x: warning - read unknown memory address %04x\n",activecpu_get_pc(),offset<<1);
				return ret;
			}

		case 0xaa: /* ??? */
			//logerror("Protection PC %06x: warning - read unknown memory address %04x\n",activecpu_get_pc(),offset<<1);
			return 0xc080;

		case 0x42: /* Strange, but consistent */
			//logerror("Protection PC %06x: warning - read unknown memory address %04x\n",activecpu_get_pc(),offset<<1);
			return deco16_prot_ram[0x2c/2]^0x5302;

		case 0x48: /* Correct for test data, but I wonder if the 0x1800 is from an address, not a constant */
			//logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
			return (0x1800) & (~deco16_prot_ram[0x36/2]);

		case 0x52:
			return (0x2188) & (~deco16_prot_ram[0x36/2]);

		case 0x82:
			return ((0x0022 ^ deco16_prot_ram[0x2c/2])) & (~deco16_prot_ram[0x36/2]);

		case 0xc:
			return 0x2000;
	}

#ifdef MAME_DEBUG
	usrintf_showmessage("Deco66:  Read unmapped port %04x\n",offset*2);
#endif

	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
	return 0;
}

/***************************************************************************/

WRITE32_HANDLER( deco32_fghthist_prot_w )
{
	if ((offset<<1)==0x10a) {
		soundlatch_w(0,(data>>16)&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

	COMBINE_DATA(&deco32_prot_ram[offset]);


	if ((offset<<1)!=0 && (offset<<1)!=0x302 && (offset<<1)!=0x506 && (offset<<1)!=0x386 
		&& (offset<<1)!=0x6 && (offset<<1)!=0x186 && (offset<<1)!=0x70c)
		logerror("%08x:  Protection write %04x %08x\n",activecpu_get_pc(),offset<<1,data);
}

READ32_HANDLER( deco32_fghthist_prot_r )
{
	data16_t val;


	if (activecpu_get_pc()==0x163dc || activecpu_get_pc()==0x16390)
		logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);


	/* The 16 bit protection device is attached to the top 16 bits of the 32 bit bus */
	switch (offset<<1) {
	case 0x624: return readinputport(0) << 16; /* IN0 */
	case 0x5aa: return readinputport(1) << 16; /* IN1 */
	case 0x150: return EEPROM_read_bit() << 16;

	/* A hardware XOR with various bitshifting patterns on the input data */
	case 0x518:
		val=deco32_prot_ram[0x788>>1]>>16;
		val=((val&0x0003)<<6) | ((val&0x000c)<<2) | ((val&0x00f0)<<4) | ((val&0x0f00)<<4) | ((val&0xf000)>>12);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x33c:
		val=deco32_prot_ram[0x788>>1]>>16;
		val=((val&0x0003)<<10) | ((val&0x000c)<<6) | ((val&0x00f0)<<8) | ((val&0x0f00)>>8) | ((val&0xf000)>>8);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x6ee:
		val=((deco32_prot_ram[0]>>16)&0xffff);
		val=((val&0x000c)>>2) | ((val&0x0003)<<2) | ((val&0xfff0)<<0);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x0d0:
		val=((deco32_prot_ram[0]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)<<4) | ((val&0x0f00)>>8) | ((val&0xf000)>>8);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x30c:
		val=((deco32_prot_ram[0]>>16)&0xffff);
		val=((val&0x000e)<<3) | ((val&0x0001)<<7) | ((val&0x00f0)<<4) | ((val&0x0f00)<<4) | ((val&0xf000)>>12);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x3c4:
		val=((deco32_prot_ram[0]>>16)&0xffff);
		val=((val&0x000f)<<0) | ((val&0x00f0)<<4) | ((val&0x0f00)<<4) | ((val&0xf000)>>8);
		return (val<<16); /* No XOR */

	case 0x104:
		val=((deco32_prot_ram[0x6>>1]>>16)&0xffff);
		val=((val&0x000f)<<4) | ((val&0x00f0)>>4) | ((val&0x0f00)<<4) | ((val&0xf000)>>4);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x6c6:
		val=((deco32_prot_ram[0x6>>1]>>16)&0xffff);
		val=((val&0x0007)<<1) | ((val&0xfff0)>>0) | ((val&0x0008)>>3);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x88:
		val=deco32_prot_ram[0x70c>>1]>>16;
		val=((val&0x0007)<<13) | ((val&0x0008)<<9); /* Bottom bits are masked out before XOR */
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x428:
		val=((deco32_prot_ram[0x386>>1]>>16)&0xffff);
		val=((val&0x0007)<<1) | ((val&0xfff0)>>0) | ((val&0x0008)>>3);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x790:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000f)<<0) | ((val&0x00f0)<<8) | ((val&0x0f00)>>0) | ((val&0xf000)>>8);
		return (val<<16);

	case 0xf8:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000f)<<4) | ((val&0x00f0)>>4) | ((val&0x0f00)<<4) | ((val&0xf000)>>4);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x21a:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000e)<<3) | ((val&0x0001)<<7) | ((val&0x00f0)<<4) | ((val&0xf000)>>12) | ((val&0x0f00)<<4);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x5e8:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000f)<<0) | ((val&0x00f0)<<8) | ((val&0xff00)>>4);
		return (val<<16);

	case 0x630:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)>>4) | ((val&0x0f00)<<0) | ((val&0xf000)>>8);
		return (val<<16);

	case 0x67c:
		val=((deco32_prot_ram[0x82>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)>>4) | ((val&0x0f00)>>0) | ((val&0xf000)>>8);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x27c:
		val=((deco32_prot_ram[0x386>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)<<4);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x64e:
		val=((deco32_prot_ram[0x40a>>1]>>16)&0xffff);
		val=((val&0x000f)<<4) | ((val&0x00f0)<<4) | ((val&0x0f00)>>8) | ((val&0xf000)>>0);
		return (val<<16);

	case 0x146:
		val=((deco32_prot_ram[0x480>>1]>>16)&0xffff);
		val=((val&0x00ff)<<8) | ((val&0xff00)>>8);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

//	case 0x40a: //wrong?
//		val=((deco32_prot_ram[0x40a>>1]>>16)&0xffff);
//		return (val<<16);

	case 0x1e8: /* Bitshifted XOR, with additional inverse mask on final output */
		val=((deco32_prot_ram[0xe>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)<<4) | ((val&0x0f00)>>8) | ((val&0xf000)>>8);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x6c2: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0xe>>1]>>16)&0xffff);
		val=((val&0x0003)<<14) | ((val&0x000c)<<10) | ((val&0xfff0)>>4);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

//	case 0x502: /* Bitshifted XOR, with additional inverse mask on final output */

		// VAL unknown - NOT 0xe - shift unknown

//		val=0;//((deco32_prot_ram[0xe>>1]>>16)&0xffff);
//		val=0;//((val&0x000f)<<12) | ((val&0x00f0)<<4) | ((val&0x0f00)>>8) | ((val&0xf000)>>8);
//		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x4b4: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x604>>1]>>16)&0xffff);
		val=((val&0x000f)<<4) | ((val&0x00f0)>>4) | ((val&0xff00)>>0);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x4d4: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x604>>1]>>16)&0xffff);
		val=((val&0x000e)<<7) | ((val&0x00f0)<<8) | ((val&0x0001)<<11);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x4c2: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x604>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)<<4) | ((val&0x0f00)>>4) | ((val&0xf000)>>12);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x328: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x10c>>1]>>16)&0xffff);
		val=((val&0x0fff)<<4);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x90:
		val=((deco32_prot_ram[0x8a>>1]>>16)&0xffff);
		val=((val&0xfff0)>>4) | ((val&0x0007)<<13) | ((val&0x0008)<<9);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x7ee: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x8a>>1]>>16)&0xffff);
		val=((val&0x000f)<<12) | ((val&0x00f0)<<4);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x20c: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x8a>>1]>>16)&0xffff);
		val=((val&0xff00)>>8) | ((val&0x00f0)<<8) | ((val&0x0003)<<10) | ((val&0x000c)<<6);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x59c:
		val=((deco32_prot_ram[0x186>>1]>>16)&0xffff);
		val=((val&0x0fff)<<4);
		return (val<<16);

	case 0x3e4:
		val=((deco32_prot_ram[0xc>>1]>>16)&0xffff);
		val=((val&0x0fff)<<4);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x644: /* Bitshifting with inverse mask on final output */
		val=((deco32_prot_ram[0x604>>1]>>16)&0xffff);
		val=((val&0xff00)>>8) | ((val&0x00f0)<<8) | ((val&0x0008)<<5) | ((val&0x0007)<<9);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x1c2:
		val=((deco32_prot_ram[0x20a>>1]>>16)&0xffff);
		val=((val&0x0f00)<<4) | ((val&0x00f0)<<0) | ((val&0x000f)<<8);
		return (val<<16) & (~deco32_prot_ram[0x506>>1]);

	case 0x13e:
		val=((deco32_prot_ram[0x20a>>1]>>16)&0xffff);
		val=((val&0x00ff)<<8);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0xf2:
		val=((deco32_prot_ram[0x18e>>1]>>16)&0xffff);
		val=((val&0xfff0)<<0) | ((val&0x0007)<<1) | ((val&0x0008)>>3);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x68e:
		val=((deco32_prot_ram[0x18e>>1]>>16)&0xffff);
		val=((val&0xfff0)>>4) | ((val&0x0007)<<13) | ((val&0x0008)<<9);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x7b6:
		val=((deco32_prot_ram[0x18e>>1]>>16)&0xffff);
		val=((val&0xff00)>>8) | ((val&0x000f)<<12) | ((val&0x00f0)<<4);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x6:
		val=((deco32_prot_ram[0x186>>1]>>16)&0xffff);
		val=((val&0x000f)<<8) | ((val&0x00f0)<<8) | ((val&0xf000)>>12) | ((val&0x0f00)>>4);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x4e0:
		val=((deco32_prot_ram[0x10e>>1]>>16)&0xffff);
		val=((val&0x000f)<<8) | ((val&0x00f0)>>4) | ((val&0xf000)>>0) | ((val&0x0f00)>>4);
		return (val<<16);

	case 0x12c:
		val=((deco32_prot_ram[0x506>>1]>>16)&0xffff);
		return (val<<16) ^ deco32_prot_ram[0x302>>1];

	case 0x5c:
		val=((deco32_prot_ram[0x382>>1]>>16)&0xffff);
		val=((val&0x000f)<<4) | ((val&0x00f0)<<4) | ((val&0xf000)>>0) | ((val&0x0f00)>>8);
		return ((val<<16) ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x10e:
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
//		val=((deco32_prot_ram[0x480>>1]>>16)&0xf000);
//		return (val<<16) & (~deco32_prot_ram[0x506>>1]);
		val=(deco32_prot_ram[0x10e>>1]>>16);
		return (val<<16);
	case 0x382:
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
//		val=((deco32_prot_ram[0x480>>1]>>16)&0xf000);
//		return (val<<16) & (~deco32_prot_ram[0x506>>1]);
		val=(deco32_prot_ram[0x382>>1]>>16);
		return (val<<16);

	// above also affected by writes to 0x80???

//  above twoveryinconsistent	
	
	case 0x1fc: //todo - check if mask is used
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
		val=((deco32_prot_ram[0x500>>1]>>16)&0xffff);
		val=(val<<4);
		return (val<<16); // & (~deco32_prot_ram[0x506>>1]);



	case 0x126:
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
//		val=((deco32_prot_ram[0xc>>1]>>16)&0xffff);
		val=((deco32_prot_ram[0x10e>>1]>>16)&0xffff);
		val=((val&0xf000)<<0) | ((val&0x0ff0)>>4) | ((val&0x000f)<<8);
		return (val<<16);

// CHECK ABOVE - affected by writes to both 0xc and 0x10e????



	case 0x142: /* Hmm */ // not affected by writes to 10e 480 382 18e
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
		val=((deco32_prot_ram[0x10e>>1]>>16)&0xffff);
		return (0x01000000 ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x580:
		val=((deco32_prot_ram[0x10c>>1]>>16)&0xffff);
		val=((val&0xff00)>>8) | ((val&0x00f0)<<4) | ((val&0x000f)<<12);
		return (val<<16);// ^ deco32_prot_ram[0x302>>1]) & (~deco32_prot_ram[0x506>>1]);

	case 0x640:
		val=((deco32_prot_ram[0x10c>>1]>>16)&0xffff);
		val=((val&0xf000)>>8) | ((val&0x0ff0)<<4);
		return (val<<16) ^ deco32_prot_ram[0x302>>1]; //) & (~deco32_prot_ram[0x506>>1]);

	case 0xa:
		return 0x10000000;
	case 0x80:
		return 0x00100000;
	case 0x538:
		return 0x00100000;

	}

/*
Read 126

	  00007c38:  Protection write 000c 0f830000
	cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 0000F0FF & FFFFFFFF
	00016378:  Protection write 010e 0a800000
	000163ac:  Protection write 0480 00700000

Read 80
	0000d990:  Protection write 0006 ffedffee
	cpu #0 (PC=0000A1C8): unmapped memory dword write to 0016C008 = 00000000 & FFFFFFFF
	cpu #0 (PC=0000A230): unmapped memory dword write to 00140000 = 0017C020 & FFFFFFFF
	0000cae8:  Protection write 070c 00010000
	cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 000000A0 & FFFFFFFF
	00016378:  Protection write 0080 00088027
	00016390:Read prot 00000080 (00000000)



  routine at:

  1637c
		Write 0e00 into $382
		Reads back 382


  18e1c:
	





  18e0c is collision routine...




Single Knockdown for player 1

cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 000000A0 & FFFFFFFF
00016378:  Protection write 0080 00088027
00016380: Read 0000000a
00016390:Read prot 00000080 (00000000)
00016390:Read prot 00000080 (00000000)
000163ac:  Protection write 0480 00700000
000163b4:Read prot 00000142 (00000000)
000163dc:Read prot 00000538 (00000000)
000163dc:Read prot 00000538 (00000000)
00001c58:  Protection write 0082 3e0aed6f
00001c7c:  Protection write 0082 ed6f0000
00001c58:  Protection write 0082 58e000c2
	
Strike against player 1 (jump error)

0013940:  Protection write 018e 02000002
0000f4a4:  Protection write 010c 00480000
Warning: sound latch written before being read. Previous: 08, new: 30
00007c38:  Protection write 000c 0f830000
cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 0000F0FF & FFFFFFFF
00016378:  Protection write 010e 0a800000
00016390:Read prot 0000010e (00000000)
00016390:Read prot 0000010e (00000000)
000163ac:  Protection write 0480 00700000
000163b4:Read prot 00000142 (00000000)
000163dc:Read prot 00000126 (00000000)
000163dc:Read prot 00000126 (00000000)
cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 0000F0FF & FFFFFFFF
00016378:  Protection write 010e 09000000
00016390:Read prot 0000010e (00000000)
00016390:Read prot 0000010e (00000000)
000163ac:  Protection write 0480 00700000
000163b4:Read prot 00000142 (00000000)
000163dc:Read prot 000004e0 (00000000)
cpu #0 (PC=00016374): unmapped memory dword write to 00208800 = 000007A0 & FFFFFFFF
00016378:  Protection write 0382 0e000390
00016380: Read 0000007a
00016390:Read prot 00000382 (00000000)
00016390:Read prot 00000382 (00000000)
000163ac:  Protection write 0480 00700000
000163b4:Read prot 00000142 (00000000)
000163dc:Read prot 0000005c (00000000)


  */


	if ((offset<<1)==0x506) //except soon after writes to.. 506/302??
		return 0x02000000;

	if ((offset<<1)==0x304) // not affected by writes to 10e 480 382 18e 302
		return 0;

	if (activecpu_get_pc()!=0x163c8 && activecpu_get_pc()!=0x16448)
	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);
	if ((offset<<1)==0x80)
		return 0; //strobe between 0 and 0x00100000

//	if (! ((offset<<1)>0x500 && (offset<<1)<0x540))
//	logerror("%08x:Read prot %08x (%08x)\n",activecpu_get_pc(),offset<<1,mem_mask);

	if ((offset<<1)==0x40a)
		return 0x10000000;

	if ((offset<<1)==0x382) // not affected by writes to 10e 480 382 18e 302
		return 0;

	return 0; //xffffffff;
}

/***************************************************************************/

WRITE16_HANDLER( deco16_104_cninja_prot_w )
{
	if (offset==(0xa8/2)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

	COMBINE_DATA(&deco16_prot_ram[offset]);
}

READ16_HANDLER( deco16_104_cninja_prot_r )
{
 	switch (offset<<1) {
		case 0x80: /* Master level control */
			return deco16_prot_ram[0];

		case 0xde: /* Restart position control */
			return deco16_prot_ram[1];

		case 0xe6: /* The number of credits in the system. */
			return deco16_prot_ram[2];

		case 0x86: /* End of game check.  See 0x1814 */
			return deco16_prot_ram[3];

		/* Video registers */
		case 0x5a: /* Moved to 0x140000 on int */
			return deco16_prot_ram[8];
		case 0x84: /* Moved to 0x14000a on int */
			return deco16_prot_ram[9];
		case 0x20: /* Moved to 0x14000c on int */
			return deco16_prot_ram[10];
		case 0x72: /* Moved to 0x14000e on int */
			return deco16_prot_ram[11];
		case 0xdc: /* Moved to 0x150000 on int */
			return deco16_prot_ram[12];
		case 0x6e: /* Moved to 0x15000a on int */
			return deco16_prot_ram[13]; /* Not used on bootleg */
		case 0x6c: /* Moved to 0x15000c on int */
			return deco16_prot_ram[14];
		case 0x08: /* Moved to 0x15000e on int */
			return deco16_prot_ram[15];

		case 0x36: /* Dip switches */
			return readinputport(2);

		case 0x1c8: /* Coins */
			return readinputport(1);

		case 0x22c: /* Player 1 & 2 input ports */
			return readinputport(0);
	}

	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset);
	return 0;
}

/***************************************************************************/

WRITE16_HANDLER( deco16_146_funkyjet_prot_w )
{
	COMBINE_DATA(&deco16_prot_ram[offset]);

	if (offset == (0x10a >> 1)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}
}

READ16_HANDLER( deco16_146_funkyjet_prot_r )
{
	switch (offset)
	{
		case 0x0be >> 1:
			return deco16_prot_ram[0x106>>1];
		case 0x11e >> 1:
			return deco16_prot_ram[0x500>>1];

		case 0x148 >> 1: /* EOR mask for joysticks */
			return deco16_prot_ram[0x70e>>1];

		case 0x1da >> 1:
			return deco16_prot_ram[0x100>>1];
		case 0x21c >> 1:
			return deco16_prot_ram[0x504>>1];
		case 0x226 >> 1:
			return deco16_prot_ram[0x58c>>1];
		case 0x24c >> 1:
			return deco16_prot_ram[0x78e>>1];
		case 0x250 >> 1:
			return deco16_prot_ram[0x304>>1];
		case 0x2d4 >> 1:
			return deco16_prot_ram[0x102>>1];
		case 0x2d8 >> 1: /* EOR mask for credits */
			return deco16_prot_ram[0x502>>1];

		case 0x3a6 >> 1:
			return deco16_prot_ram[0x104>>1];
		case 0x3a8 >> 1: /* See 93e4/9376 */
			return deco16_prot_ram[0x500>>1];
		case 0x3e8 >> 1:
			return deco16_prot_ram[0x50c>>1];

		case 0x4e4 >> 1:
			return deco16_prot_ram[0x702>>1];
		case 0x562 >> 1:
			return deco16_prot_ram[0x18e>>1];
		case 0x56c >> 1:
			return deco16_prot_ram[0x50c>>1];

		case 0x688 >> 1:
			return deco16_prot_ram[0x300>>1];
		case 0x788 >> 1:
			return deco16_prot_ram[0x700>>1];

		case 0x7d4 >> 1: /* !? On the bootleg these address checks are NOP'd, so a BEQ is never taken */
			return 0x10; //deco16_prot_ram[0x7da>>1];

		case 0x27c >>1: /* From bootleg code at 0x400 */
			return ((deco16_prot_ram[0x70e>>1]>>4)&0x0fff) | ((deco16_prot_ram[0x70e>>1]&0x0001)<<15) | ((deco16_prot_ram[0x70e>>1]&0x000e)<<11);
		case 0x192 >>1: /* From bootleg code at 0x400 */
			return ((deco16_prot_ram[0x78e>>1]<<0)&0xf000);

		case 0x5be >> 1: /* Confirmed from bootleg code at 0xc07c */
			return ((deco16_prot_ram[0x70e>>1]<<4)&0xff00) | (deco16_prot_ram[0x70e>>1]&0x000f);
		case 0x5ca >> 1: /* Confirmed from bootleg code at 0xc05e */
			return ((deco16_prot_ram[0x78e>>1]>>4)&0xff00) | (deco16_prot_ram[0x78e>>1]&0x000f) | ((deco16_prot_ram[0x78e>>1]<<8)&0xf000);

		case 0x00c >> 1: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 0x778 >> 1: /* Credits */
			return readinputport(2);
		case 0x382 >> 1: /* DIPS */
			return (readinputport(3) + (readinputport(4) << 8));
	}

	if (activecpu_get_pc()!=0xc0ea)	logerror("CPU #0 PC %06x: warning - read unmapped control address %06x\n",activecpu_get_pc(),offset<<1);

	return 0;
}

/***************************************************************************/

static data8_t deco16_prot_backbuffer_selected=0;
static data16_t deco16_prot_backbuffer[0x100];

void deco16_104_rohga_reset(void)
{
	deco16_prot_backbuffer_selected=0;
	state_save_register_UINT8("rohga", 0, "protstate", &deco16_prot_backbuffer_selected, 1);
	state_save_register_UINT16("rohga", 0, "prot", deco16_prot_backbuffer, 0x100);
}

WRITE16_HANDLER( deco16_104_rohga_prot_w )
{
	/* 
		Schmeizr uses a feature of this chip that other games don't seem to exploit.  There
		appear to be two banks of data ports (referred to here as front and back buffers).
		These banks are switched when certain addresses are read - also very good protection
		against trojans as it is non-obvious when data ports are switched.

		It appears to work as follows..  When the front buffer is active (the state upon
		reset) any writes to data ports push the existing front buffer word into the back
		buffer (ie, a FIFO latch type of system).  When the back buffer is active, any writes
		to data ports affect the back buffer only - the front buffer data is not affected, and
		there is no FIFO action.

		By default the read ports return a scrambled address and scrambled data from a 
		front buffer port.  When the back buffer is active most read ports return scrambled
		data from the back buffer, however some ports work exclusively on front buffer data
		even if the back buffer is selected.  

		There doesn't appear to be any way to detect what bank is currently selected - it
		seems game code must maintain this state (if it matters).
	*/

	if (deco16_prot_backbuffer_selected) {
		COMBINE_DATA(&deco16_prot_backbuffer[offset]);

//		if (offset==0x7e/2)
//			logerror("Wrote %04x into back only\n",data);
	} else {
		/* FIFO */
//		if (offset==0x7e/2)
//			logerror("Wrote %04x into front, %04x to back\n",data,deco16_prot_ram[offset]);
		deco16_prot_backbuffer[offset]=deco16_prot_ram[offset];
		COMBINE_DATA(&deco16_prot_ram[offset]);
	}

	if (offset==(0xa8/2)) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

	
	offset=offset*2;

//	if (offset==0x60)
//	logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset,data);
	
	//logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset,data);
	if (offset==0xee || offset==0x42 || offset==0xa8)
		return;

//	logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset,data);

#if 1
// 66 7c 7e 28 58 4a 9e
	if (offset==0x66 || offset==0x7c || offset==0x7e || offset==0x28 || offset==0x58 || offset==0x4a)
		return;
	if (offset==0x9e || offset==0x7c || offset==0x7e || offset==0x28 || offset==0x58 || offset==0x4a)
		return;
#endif

#if 1
	if (offset>=0x80 && offset<0xa0)
		return;
	if (offset>=0xc0 && offset<0xd0)
		return;

//	if (offset==0x3c)
//		logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset,data);
// Actually read:
//	22 24 26 2c 2e 30 32 3c 40 44 46 48 60 62 66 6a 6e 76 7e 80 82 84 86 88 8a 8c 90 94 96 98 9a 9c a0 c0 c4 c6 c8 cc ce d6 dc de 

// check 74 b0 b4 d4

//  24 2a 2c 2e 30 3c 44 48 66 6a 74 a0 b0 b4 d4 d6 
//  32 60 7e
//  22 26 a4 a6
//  62 6e d0 dc de d2 d8 da 46
	if (offset==0x24 || offset==0x2a || offset==0x2c || offset==0x2e || offset==0x30 || offset==0x3c)
		return;
	if (offset==0x44 || offset==0x48 || offset==0x66 || offset==0x6a || offset==0x74 || offset==0xa0)
		return;
	if (offset==0xb0 || offset==0xb4 || offset==0xd4 || offset==0xd6)
		return;
	if (offset==0x32 || offset==0x60 || offset==0x7e || offset==0xa4 || offset==0xa6)
		return;
	if (offset==0x22 || offset==0x26 || offset==0x62 || offset==0xd0 || offset==0xdc)
		return;
	if (offset==0xde || offset==0xd2 || offset==0xd8 || offset==0xda || offset==0x46)
		return;
	if (offset==0x20 || offset==0x40)
		return;
	if (offset==0x6e || offset==0xaa || offset==0xac || offset==0xa2)
		return;
#endif

	logerror("CONTROL PC %06x: warning - write unmapped protection memory address %04x %04x\n",activecpu_get_pc(),offset,data);
}

READ16_HANDLER( deco16_104_rohga_prot_r )
{
//	if (offset!=0x88/2 && offset!=0x44c/2 && offset!=0x36c/2 && offset!=0x292/2)
//logerror("Protection PC %06x: warning - read prot address %04x\n",activecpu_get_pc(),offset<<1);

	switch (offset) {
		case 0x88/2: /* Player 1 & 2 input ports */
			return readinputport(0);

		case 0x44c/2: /* VBL */
		case 0x36c/2: /* Coins */
			return readinputport(1);

		case 0x292/2: /* Dips */
			return readinputport(2);
#if 1
		case 0x420/2:
			return ((deco16_prot_ram[0x2e/2]&0xf000)>>4) | ((deco16_prot_ram[0x2e/2]&0x0f00)<<4) | ((deco16_prot_ram[0x2e/2]&0x00f0)>>4) | ((deco16_prot_ram[0x2e/2]&0x000f)<<4);

		case 0x390/2:
			return deco16_prot_ram[0x2c/2];

//		case 0x754/2: //fails verify
//			return (((deco16_prot_ram[0x76/2]&0x000f)<<12) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);

		case 0x756/2:
			return ((deco16_prot_ram[0x60/2]&0xfff0)>>4) | ((deco16_prot_ram[0x60/2]&0x0007)<<13) | ((deco16_prot_ram[0x60/2]&0x0008)<<9);
		case 0x424/2:
			return ((deco16_prot_ram[0x60/2]&0xf000)>>4) | ((deco16_prot_ram[0x60/2]&0x0f00)<<4) | ((deco16_prot_ram[0x60/2]&0x00f0)>>0) | ((deco16_prot_ram[0x60/2]&0x000f)<<0);

		case 0x156/2:
			return (((deco16_prot_ram[0xde/2]&0xff00)<<0) | ((deco16_prot_ram[0xde/2]&0x000f)<<4) | ((deco16_prot_ram[0xde/2]&0x00f0)>>4)) & (~deco16_prot_ram[0xee/2]);
		case 0xa8/2:
			return (((deco16_prot_ram[0xde/2]&0xff00)>>4) | ((deco16_prot_ram[0xde/2]&0x000f)<<0) | ((deco16_prot_ram[0xde/2]&0x00f0)<<8)) & (~deco16_prot_ram[0xee/2]);
		case 0x64a/2:
			return (((deco16_prot_ram[0xde/2]&0xfff0)>>4) | ((deco16_prot_ram[0xde/2]&0x000c)<<10) | ((deco16_prot_ram[0xde/2]&0x0003)<<14)) & (~deco16_prot_ram[0xee/2]);

		case 0x16e/2:
			return deco16_prot_ram[0x6a/2];
		case 0x39c/2:
			return (deco16_prot_ram[0x6a/2]&0x00ff) | ((deco16_prot_ram[0x6a/2]&0xf000)>>4) | ((deco16_prot_ram[0x6a/2]&0x0f00)<<4);

		case 0x212/2: /* XOR register used */
			return (((deco16_prot_ram[0x6e/2]&0xff00)>>4) | ((deco16_prot_ram[0x6e/2]&0x00f0)<<8) | ((deco16_prot_ram[0x6e/2]&0x000f)<<0)) ^ deco16_prot_ram[0x42/2];

		case 0x70a/2: /* XOR register used */
			return (((deco16_prot_ram[0xde/2]&0x00f0)<<8) | ((deco16_prot_ram[0xde/2]&0x0007)<<9) | ((deco16_prot_ram[0xde/2]&0x0008)<<5)) ^ deco16_prot_ram[0x42/2];

		case 0x7a0/2:
			return (deco16_prot_ram[0x6e/2]&0x00ff) | ((deco16_prot_ram[0x6e/2]&0xf000)>>4) | ((deco16_prot_ram[0x6e/2]&0x0f00)<<4);
		case 0x162/2:
			return deco16_prot_ram[0x6e/2];


		case 0x384/2:
			return ((deco16_prot_ram[0xdc/2]&0xf000)>>12) | ((deco16_prot_ram[0xdc/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xdc/2]&0x000c)<<2) | ((deco16_prot_ram[0xdc/2]&0x0003)<<6);

		case 0x302/2:
			return deco16_prot_ram[0x24/2];
		case 0x334/2:
			return deco16_prot_ram[0x30/2];
		case 0x34c/2:
			return deco16_prot_ram[0x3c/2];

		case 0x514/2:
			return (((deco16_prot_ram[0x32/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x32/2]&0x000c)<<2) | ((deco16_prot_ram[0x32/2]&0x0003)<<6)) & (~deco16_prot_ram[0xee/2]);
		case 0x444/2:
			return ((deco16_prot_ram[0x66/2]&0x00f0)<<8) | ((deco16_prot_ram[0x66/2]&0x0007)<<9)  | ((deco16_prot_ram[0x66/2]&0x0008)<<5);

		case 0x34e/2:
			return ((deco16_prot_ram[0xde/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xde/2]&0xf000)>>8) | ((deco16_prot_ram[0xde/2]&0x000f)<<0);
		case 0x722/2: /* XOR applied before MASK */
			return (((deco16_prot_ram[0xdc/2]&0x0fff)<<4) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x574/2: /* XOR applied before MASK */
			return ((((deco16_prot_ram[0xdc/2]&0xfff0)>>0) | ((deco16_prot_ram[0xdc/2]&0x0003)<<2) | ((deco16_prot_ram[0xdc/2]&0x000c)>>2)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);


		case 0x5ae/2:
			return deco16_prot_ram[0xdc/2];
		case 0x410/2:
			return deco16_prot_ram[0xde/2];
		case 0x340/2:
			return ((deco16_prot_ram[0x90/2]&0xfff0) | ((deco16_prot_ram[0x90/2]&0x7)<<1) | ((deco16_prot_ram[0x90/2]&0x8)>>3)) ^ deco16_prot_ram[0x42/2];
		case 0x4a4/2:
			return (((deco16_prot_ram[0xce/2]&0x0ff0) | ((deco16_prot_ram[0xce/2]&0xf000)>>12) | ((deco16_prot_ram[0xce/2]&0x000f)<<12)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x256/2:
			return ((((deco16_prot_ram[0xce/2]&0xf000)>>12) | ((deco16_prot_ram[0xce/2]&0x0fff)<<4))) & (~deco16_prot_ram[0xee/2]);
		case 0x79a/2:
			return (((deco16_prot_ram[0xc8/2]&0xfff0)>>4) | ((deco16_prot_ram[0xc8/2]&0x0008)<<9) | ((deco16_prot_ram[0xc8/2]&0x0007)<<13)) ^ deco16_prot_ram[0x42/2];
		case 0x65e/2:
			return deco16_prot_ram[0x9c/2];
		case 0x79c/2:
			return ((deco16_prot_ram[0xc6/2]&0xf000) | ((deco16_prot_ram[0xc6/2]&0x00ff)<<4) | ((deco16_prot_ram[0xc6/2]&0x0f00)>>8)) & (~deco16_prot_ram[0xee/2]);
		case 0x15e/2:
			return (((deco16_prot_ram[0x98/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x98/2]&0xf000)>>12) | ((deco16_prot_ram[0x98/2]&0x0003)<<6) | ((deco16_prot_ram[0x98/2]&0x000c)<<2)) ^ deco16_prot_ram[0x42/2];
		case 0x6e4/2:
			return deco16_prot_ram[0x98/2];
		case 0x1e/2:
			return ((((deco16_prot_ram[0xc4/2]&0xf000)>>4) | ((deco16_prot_ram[0xc4/2]&0x0f00)<<4) | ((deco16_prot_ram[0xc4/2]&0x00ff)<<0)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x23a/2:
			return ((((deco16_prot_ram[0x86/2]&0xfff0)>>0) | ((deco16_prot_ram[0x86/2]&0x0003)<<2) | ((deco16_prot_ram[0x86/2]&0x000c)>>2)) ^ deco16_prot_ram[0x42/2]);
		case 0x6e/2:
			return ((((deco16_prot_ram[0x96/2]&0xf000)>>8) | ((deco16_prot_ram[0x96/2]&0x0f0f)<<0) | ((deco16_prot_ram[0x96/2]&0x00f0)<<8)) ^ deco16_prot_ram[0x42/2]);
		case 0x3a2/2:
			return ((((deco16_prot_ram[0x94/2]&0xf000)>>8) | ((deco16_prot_ram[0x94/2]&0x0f00)>>8) | ((deco16_prot_ram[0x94/2]&0x00f0)<<8) | ((deco16_prot_ram[0x94/2]&0x000e)<<7) | ((deco16_prot_ram[0x94/2]&0x0001)<<11)) ^ deco16_prot_ram[0x42/2]);// & (~deco16_prot_ram[0xee/2]);
		case 0x4a6/2:
			return ((deco16_prot_ram[0x8c/2]&0xff00)>>0) | ((deco16_prot_ram[0x8c/2]&0x00f0)>>4) | ((deco16_prot_ram[0x8c/2]&0x000f)<<4);
		case 0x7b0/2:
			return deco16_prot_ram[0x80/2];
		case 0x5aa/2: // need to verify (added mask as test)
			return ((((deco16_prot_ram[0x98/2]&0x0f00)>>8) | ((deco16_prot_ram[0x98/2]&0xf000)>>8) | ((deco16_prot_ram[0x98/2]&0x00f0)<<8) | ((deco16_prot_ram[0x98/2]&0x000e)<<7) | ((deco16_prot_ram[0x98/2]&0x0001)<<11)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x662/2:
			return deco16_prot_ram[0x8c/2];
		case 0x624/2:
			return deco16_prot_ram[0x9a/2];
		case 0x2c/2:
			return (((deco16_prot_ram[0x82/2]&0x0f0f)>>0) | ((deco16_prot_ram[0x82/2]&0xf000)>>8) | ((deco16_prot_ram[0x82/2]&0x00f0)<<8)) & (~deco16_prot_ram[0xee/2]);


		case 0x41a/2:
			return ((((deco16_prot_ram[0x84/2]&0x00f0)<<8) | ((deco16_prot_ram[0x84/2]&0xf000)>>8) | ((deco16_prot_ram[0x84/2]&0x0f00)>>8) | ((deco16_prot_ram[0x84/2]&0x0003)<<10) | ((deco16_prot_ram[0x84/2]&0x000c)<<6)) ^ deco16_prot_ram[0x42/2]);
		case 0x168/2:
			return ((((deco16_prot_ram[0x84/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x84/2]&0x000e)<<3) | ((deco16_prot_ram[0x84/2]&0x0001)<<5))) & (~deco16_prot_ram[0xee/2]);
		case 0x314/2:
			return ((((deco16_prot_ram[0x84/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x84/2]&0x000e)<<3) | ((deco16_prot_ram[0x84/2]&0x0001)<<5)));
		case 0x1b4/2: 
			return ((deco16_prot_ram[0xcc/2]&0x00f0)<<4) | ((deco16_prot_ram[0xcc/2]&0x000f)<<12);
		case 0x5e2/2: 
			return ((((deco16_prot_ram[0x84/2]&0x00f0)<<8) | ((deco16_prot_ram[0x84/2]&0x000e)<<7) | ((deco16_prot_ram[0x84/2]&0x0001)<<9)));
		case 0x22c/2: 
			return ((deco16_prot_ram[0xc4/2]&0x00f0)<<8);
		case 0x27e/2:
			return ((((deco16_prot_ram[0x94/2]&0x00f0)<<8)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x77e/2:
			return ((deco16_prot_ram[0x62/2]&0xf000)>>12) | ((deco16_prot_ram[0x62/2]&0x0ff0)<<0) | ((deco16_prot_ram[0x62/2]&0x000f)<<12);
		case 0xc/2:
			return ((deco16_prot_ram[0xd6/2]&0xf000)>>12) | ((deco16_prot_ram[0xd6/2]&0x0fff)<<4);


		case 0x90/2:
			return deco16_prot_ram[0x44/2];
		case 0x246/2:
			return ((((deco16_prot_ram[0x48/2]&0xff00)>>8) | ((deco16_prot_ram[0x48/2]&0x00f0)<<8) | ((deco16_prot_ram[0x48/2]&0x0f00)>>8) | ((deco16_prot_ram[0x48/2]&0x0003)<<10) | ((deco16_prot_ram[0x48/2]&0x000c)<<6)) ^ deco16_prot_ram[0x42/2]);
		case 0x546/2:
			return (((deco16_prot_ram[0x62/2]&0xf0f0)>>0) | ((deco16_prot_ram[0x62/2]&0x000f)<<8)) & (~deco16_prot_ram[0xee/2]);
		case 0x7ce/2:
			return ((deco16_prot_ram[0x80/2]&0x000e)<<11) | ((deco16_prot_ram[0x80/2]&0x0001)<<15);
		case 0x2e2/2:
			return ((deco16_prot_ram[0xc6/2]&0x000e)<<11) | ((deco16_prot_ram[0xc6/2]&0x0001)<<15);
		case 0x40e/2:
			return ((((deco16_prot_ram[0x8a/2]&0xf000)>>0) | ((deco16_prot_ram[0x8a/2]&0x00ff)<<4)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x248/2:
			return ((((deco16_prot_ram[0x8c/2]&0xff00)>>8) | ((deco16_prot_ram[0x8c/2]&0x00f0)<<4) | ((deco16_prot_ram[0x8c/2]&0x000f)<<12)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x3c0/2:
			return deco16_prot_ram[0x22/2];
		case 0x4b8/2:
			return (((deco16_prot_ram[0x46/2]&0xf000)>>12) | ((deco16_prot_ram[0x46/2]&0x0f00)>>4) | ((deco16_prot_ram[0x46/2]&0x00ff)<<8)) ^ deco16_prot_ram[0x42/2];
		case 0x65c/2:
			return ((((deco16_prot_ram[0x44/2]&0xf000)>>12) | ((deco16_prot_ram[0x44/2]&0x0fff)<<4)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);


		case 0x32a/2:
			return ((((deco16_prot_ram[0xc0/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xc0/2]&0x000e)<<3) | ((deco16_prot_ram[0xc0/2]&0x0001)<<7))) & (~deco16_prot_ram[0xee/2]);// ^ deco16_prot_ram[0x42/2];
		case 0x8/2:
			return ((((deco16_prot_ram[0x94/2]&0xfff0)<<0) | ((deco16_prot_ram[0x94/2]&0x000e)>>1) | ((deco16_prot_ram[0x94/2]&0x0001)<<3))) & (~deco16_prot_ram[0xee/2]);// ^ deco16_prot_ram[0x42/2];
		case 0x456/2:
			return (((deco16_prot_ram[0x26/2]&0xfff0)<<0) | ((deco16_prot_ram[0x26/2]&0x0007)<<1) | ((deco16_prot_ram[0x26/2]&0x0008)>>3));// ^ deco16_prot_ram[0x42/2];
		case 0x190/2:
			return ((((deco16_prot_ram[0x44/2]&0xf000)<<0) | ((deco16_prot_ram[0x44/2]&0x00ff)<<4))) & (~deco16_prot_ram[0xee/2]);// ^ deco16_prot_ram[0x42/2];
		case 0x3f2/2:
			return ((((deco16_prot_ram[0x48/2]&0x000f)<<12) | ((deco16_prot_ram[0x48/2]&0x00f0)<<4))) & (~deco16_prot_ram[0xee/2]);// ^ deco16_prot_ram[0x42/2];
		case 0x2be/2:
			return ((deco16_prot_ram[0x40/2]&0x00ff)<<8);

		case 0x178/2:
			return (((deco16_prot_ram[0x88/2]&0x00ff)<<8) | ((deco16_prot_ram[0x88/2]&0xff00)>>8)) & (~deco16_prot_ram[0xee/2]);
		case 0x19e/2:
			return ((((deco16_prot_ram[0x3c/2]&0xf000)>>12) | ((deco16_prot_ram[0x3c/2]&0x0f00)<<4) | ((deco16_prot_ram[0x3c/2]&0x00f0)>>0) | ((deco16_prot_ram[0x3c/2]&0x000f)<<8)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);

		case 0x2a2/2:
			return ((((deco16_prot_ram[0x44/2]&0xff00)>>8) | ((deco16_prot_ram[0x44/2]&0x00f0)<<8) | ((deco16_prot_ram[0x44/2]&0x000e)<<7) | ((deco16_prot_ram[0x44/2]&0x0001)<<11)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);

		case 0x748/2:
			return (((deco16_prot_ram[0x44/2]&0xfff0)<<0) | ((deco16_prot_ram[0x44/2]&0x000e)>>1) | ((deco16_prot_ram[0x44/2]&0x0001)<<3));// & (~deco16_prot_ram[0xee/2]);
		case 0x686/2:
			return (((deco16_prot_ram[0x46/2]&0xf000)>>4) | ((deco16_prot_ram[0x46/2]&0x0f00)>>8) | ((deco16_prot_ram[0x46/2]&0x00f0)<<8) | ((deco16_prot_ram[0x46/2]&0x000f)<<4));// & (~deco16_prot_ram[0xee/2]);

		case 0x4c4/2:
			return ((deco16_prot_ram[0x3c/2]&0x000f)<<12) & (~deco16_prot_ram[0xee/2]);
		case 0x538/2:
			return ((deco16_prot_ram[0x3c/2]&0x000f)<<12);
		case 0x63a/2:
			return ((deco16_prot_ram[0x3c/2]&0x000f)<<12);
		// 63a as above?? more tests needed.


		case 0x348/2:
			return ((((deco16_prot_ram[0x44/2]&0xf000)>>12) | ((deco16_prot_ram[0x44/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x44/2]&0x000e)<<3) | ((deco16_prot_ram[0x44/2]&0x0001)<<7))) ^ deco16_prot_ram[0x42/2];// & (~deco16_prot_ram[0xee/2]);
		case 0x200/2:
			return (((deco16_prot_ram[0xa0/2]&0xfff0)>>4) | ((deco16_prot_ram[0xa0/2]&0x0007)<<13) | ((deco16_prot_ram[0xa0/2]&0x0008)<<9));// & (~deco16_prot_ram[0xee/2]);
		case 0x254/2:
			return ((((deco16_prot_ram[0x7e/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x7e/2]&0x000c)<<2) | ((deco16_prot_ram[0x7e/2]&0x0003)<<6))) ^ deco16_prot_ram[0x42/2];// & (~deco16_prot_ram[0xee/2]);
		case 0x182/2:
			return ((deco16_prot_ram[0x46/2]&0xf000)<<0) | ((deco16_prot_ram[0x46/2]&0x0f00)>>8) | ((deco16_prot_ram[0x46/2]&0x00f0)>>0) | ((deco16_prot_ram[0x46/2]&0x000f)<<8);
		case 0x58/2:
			return deco16_prot_ram[0x46/2];
		case 0x48e/2: // fails verify (remove xor as test)
			return ((((deco16_prot_ram[0x46/2]&0xf000)>>12) | ((deco16_prot_ram[0x46/2]&0x0f00)>>4) | ((deco16_prot_ram[0x46/2]&0x00f0)<<4) | ((deco16_prot_ram[0x46/2]&0x000f)<<12)));// /*^ deco16_prot_ram[0x42/2]*/) & (~deco16_prot_ram[0xee/2]);

		case 0x72a/2:
			return ((((deco16_prot_ram[0x86/2]&0xfff0)>>4) | ((deco16_prot_ram[0x86/2]&0x0003)<<14) | ((deco16_prot_ram[0x86/2]&0x000c)<<10)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);

		case 0x4ba/2:
			return (((deco16_prot_ram[0x24/2]&0xf000)>>12) | ((deco16_prot_ram[0x24/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x24/2]&0x000c)<<2) | ((deco16_prot_ram[0x24/2]&0x0003)<<6)) & (~deco16_prot_ram[0xee/2]);
		case 0x92/2:
			return (((deco16_prot_ram[0x3c/2]&0xfff0)>>0) | ((deco16_prot_ram[0x3c/2]&0x0007)<<1) | ((deco16_prot_ram[0x3c/2]&0x0008)>>3));
		case 0x1f0/2:
			return ((((deco16_prot_ram[0xa2/2]&0xf000)>>12) | ((deco16_prot_ram[0xa2/2]&0x0f00)>>4) | ((deco16_prot_ram[0xa2/2]&0x00ff)<<8)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x24e/2:
			return ((((deco16_prot_ram[0x46/2]&0xf000)>>8) | ((deco16_prot_ram[0x46/2]&0x0f00)>>0) | ((deco16_prot_ram[0x46/2]&0x00f0)>>4) | ((deco16_prot_ram[0x46/2]&0x000f)<<12)) ^ deco16_prot_ram[0x42/2]);// & (~deco16_prot_ram[0xee/2]);
		case 0x594/2:
			return ((((deco16_prot_ram[0x40/2]&0x00f0)<<8) | ((deco16_prot_ram[0x40/2]&0x000c)<<6) | ((deco16_prot_ram[0x40/2]&0x0003)<<10)) ^ deco16_prot_ram[0x42/2]);// & (~deco16_prot_ram[0xee/2]);

		case 0x7e2/2:
			return ((((deco16_prot_ram[0x96/2]&0xf000)<<0) | ((deco16_prot_ram[0x96/2]&0x00f0)<<4) | ((deco16_prot_ram[0x96/2]&0x000f)<<4))) ^ deco16_prot_ram[0x42/2];// | ((deco16_prot_ram[0x96/2]&0x0001)<<7));// ^ deco16_prot_ram[0x42/2]);// & (~deco16_prot_ram[0xee/2]);
		case 0x18c/2:
			return (((deco16_prot_ram[0x22/2]&0xfff0)>>4) | ((deco16_prot_ram[0x22/2]&0x000e)<<11) | ((deco16_prot_ram[0x22/2]&0x0001)<<15));// ^ deco16_prot_ram[0x42/2]);// & (~deco16_prot_ram[0xee/2]);
		case 0x1fa/2:
			return ((((deco16_prot_ram[0x26/2]&0xf000)>>8) | ((deco16_prot_ram[0x26/2]&0x0f00)<<0) | ((deco16_prot_ram[0x26/2]&0x00f0)>>4) | ((deco16_prot_ram[0x26/2]&0x000f)<<12))) ^ deco16_prot_ram[0x42/2];// & (~deco16_prot_ram[0xee/2]);
		case 0x70e/2:
			return ((((deco16_prot_ram[0x26/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x26/2]&0x000c)<<2) | ((deco16_prot_ram[0x26/2]&0x0003)<<6))) ^ deco16_prot_ram[0x42/2];// & (~deco16_prot_ram[0xee/2]);
		case 0x33a/2:
			return deco16_prot_ram[0x60/2] & (~deco16_prot_ram[0xee/2]);
		case 0x1e2/2:
			return ((deco16_prot_ram[0xd0/2]&0xf000)>>12) | ((deco16_prot_ram[0xd0/2]&0x0f00)>>4) | ((deco16_prot_ram[0xd0/2]&0x00ff)<<8);
		case 0x3f4/2: // fails verify
			return deco16_prot_ram[0x6e/2]<<4;// & (~deco16_prot_ram[0xee/2]);

		case 0x2ae/2:
			return ((deco16_prot_ram[0x9c/2]&0xf000)<<0) | ((deco16_prot_ram[0x9c/2]&0x0ff0)>>4) | ((deco16_prot_ram[0x9c/2]&0x000f)<<8);// & (~deco16_prot_ram[0xee/2]);
		case 0x96/2:
			return ((((deco16_prot_ram[0x22/2]&0xff00)>>8) | ((deco16_prot_ram[0x22/2]&0x00f0)<<8) | ((deco16_prot_ram[0x22/2]&0x000e)<<7) | ((deco16_prot_ram[0x22/2]&0x0001)<<11)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		// 53e changes to 1000 from d000 (write 1234 to ce 9c 9e)

// 444 suspected as flip address..

// 53e changes on ce 9c or 9e - use special test
// 780 is stuck at 2300 on standard test - no xor or mask

//		case 0x3c:
// 3c when written returns 3c for 1 read only.. (then returns to zero)
		
		case 0x2d0/2:
			return (((deco16_prot_ram[0x66/2]&0xf000)>>4) | ((deco16_prot_ram[0x66/2]&0x00f0)<<8)) ^ deco16_prot_ram[0x42/2];
		case 0x33e/2:
			return (((deco16_prot_ram[0x0/2]&0xf000)>>12) | ((deco16_prot_ram[0x0/2]&0x0f00)>>4) | ((deco16_prot_ram[0x0/2]&0x00f0)<<4) | ((deco16_prot_ram[0x0/2]&0x000f)<<12)) & (~deco16_prot_ram[0xee/2]);
		case 0x2b8/2: 
			return ((deco16_prot_ram[0x66/2]&0x00f0)<<8) ^ deco16_prot_ram[0x42/2];

			
			// $2d0 goes to 7500 when 5678 written to $66 (no xor, no mask), goes to 3100 when 1234 written
// $33e goes to 8765 when 5678 written to 0		- no xor, yes mask  (4321)


// these not affected by any known data write
// $174 return 4 hardwired (no xor, yes mask)
// $3ca return 40 hardwired (xor, no mask)
// $2ac return 400 hardwired (no xor, no mask)
// $12e hardwired 9000 (xor & mask)
// $21a hardwired a860 (no xor no mask)  a020 on different boot?

		// From 0x3d5cc - writes to ee, a4, a6 don't seem to affect read
		case 0x44e/2:
			return 0x1000 ^ deco16_prot_ram[0x42/2];

		// From 0x609ca - writes to 0xfe have no effect
		case 0x3ca/2: // also returns 0x1040??
			return 0x40 ^ deco16_prot_ram[0x42/2];
		case 0x2ac/2:
			return 0x400;

		// From 0x60c3e - writes to 0xfe have no effect
		case 0x3c/2:
			return 0;
		case 0x174/2: // also 0x404?
			return 0x4 & (~deco16_prot_ram[0xee/2]);

// TODO LATEST
/*
Protection PC 04907c: warning - read unmapped protection address 053e
Protection PC 0032fc: warning - read unmapped protection address 0780
Protection PC 045854: warning - read unmapped protection address 0454
Protection PC 0032fc: warning - read unmapped protection address 0700

Protection PC 066c9a: warning - read unmapped protection address 0294
CONTROL PC 066c9c: warning - write protection memory address 0010 0000

CONTROL PC 0609c4: warning - write protection memory address 00fe 0000
cpu #0 (PC=00058A38): unmapped memory word read from 00A60004 & 00FF
*/

//		case 0x294/2:
//			return 0x2000;
//		case 0x21a/2: // sometimes a020... hmm - flip address?
//			return 0xa020;

//		case 0x44e/2:
//			return ((((deco16_prot_ram[0x44/2]&0x00f0)<<4) | ((deco16_prot_ram[0x44/2]&0x000f)<<12))) ^ deco16_prot_ram[0x42/2];// & (~deco16_prot_ram[0xee/2]);

//		case 0x12e/2:
//			return ((((deco16_prot_ram[0x2/2]&0xf000)>>0) | ((deco16_prot_ram[0x2/2]&0x0ff0)>>4) | ((deco16_prot_ram[0x2/2]&0x000f)<<8)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);

		case 0x34a/2:
			return (((deco16_prot_ram[0x4/2]&0xff00)>>0) | ((deco16_prot_ram[0x4/2]&0x00f0)>>4) | ((deco16_prot_ram[0x4/2]&0x000f)<<4)) & (~deco16_prot_ram[0xee/2]);
		case 0x324/2:
			return (((deco16_prot_ram[0x6/2]&0xf000)>>12) | ((deco16_prot_ram[0x6/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x6/2]&0x0007)<<5) | ((deco16_prot_ram[0x6/2]&0x0008)<<1));
		case 0x344/2:
			return (((deco16_prot_ram[0x8/2]&0xf000)>>8) | ((deco16_prot_ram[0x8/2]&0x0f00)>>8) | ((deco16_prot_ram[0x8/2]&0x00f0)<<4) | ((deco16_prot_ram[0x8/2]&0x000f)<<12));
		case 0x72/2:
			return ((((deco16_prot_ram[0xa/2]&0xf000)>>8) | ((deco16_prot_ram[0xa/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xa/2]&0x000f)>>0))) & (~deco16_prot_ram[0xee/2]);
		case 0x36e/2:
			return ((((deco16_prot_ram[0xc/2]&0xf000)>>0) | ((deco16_prot_ram[0xc/2]&0x0ff0)>>4) | ((deco16_prot_ram[0xc/2]&0x000f)<<8))) & (~deco16_prot_ram[0xee/2]);

		case 0x590/2:
			return ((((deco16_prot_ram[0xe/2]&0xfff0)>>4) | ((deco16_prot_ram[0xe/2]&0x000e)<<11) | ((deco16_prot_ram[0xe/2]&0x0001)<<15))) ^ deco16_prot_ram[0x42/2];
		case 0x7b6/2:
			return ((((deco16_prot_ram[0x2/2]&0xf000)>>8) | ((deco16_prot_ram[0x2/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x2/2]&0x000f)<<0)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x588/2:
			return ((((deco16_prot_ram[0x4/2]&0xff00)>>4) | ((deco16_prot_ram[0x4/2]&0x00f0)<<8) | ((deco16_prot_ram[0x4/2]&0x000f)<<0)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x1f6/2:
			return (((deco16_prot_ram[0x6/2]&0xf000)>>12) | ((deco16_prot_ram[0x6/2]&0x0ff0)<<4) | ((deco16_prot_ram[0x6/2]&0x0007)<<5) | ((deco16_prot_ram[0x6/2]&0x0008)<<1)) ^ deco16_prot_ram[0x42/2];
		case 0x4c0/2:
			return (((deco16_prot_ram[0x8/2]&0xf000)>>4) | ((deco16_prot_ram[0x8/2]&0x0f00)<<4) | ((deco16_prot_ram[0x8/2]&0x00f0)>>4) | ((deco16_prot_ram[0x8/2]&0x000f)<<4)) & (~deco16_prot_ram[0xee/2]);
		case 0x63e/2:
			return ((((deco16_prot_ram[0xa/2]&0x0ff0)<<4) | ((deco16_prot_ram[0xa/2]&0xf000)>>12) | ((deco16_prot_ram[0xa/2]&0x0003)<<6) | ((deco16_prot_ram[0xa/2]&0x000c)<<2)));
		case 0x7cc/2:
			return ((((deco16_prot_ram[0xc/2]&0xfff0)>>4) | ((deco16_prot_ram[0xc/2]&0x000e)<<11) | ((deco16_prot_ram[0xc/2]&0x0001)<<15)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x1bc/2:
			return (((deco16_prot_ram[0xe/2]&0xf000)>>12) | ((deco16_prot_ram[0xe/2]&0x0f00)>>4) | ((deco16_prot_ram[0xe/2]&0x00ff)<<8)) & (~deco16_prot_ram[0xee/2]);

// 44e returns 8700 for 5678 on 44			XOR

// $12e returns 5867 on  2					XOR	MASK
// $34a 5687 on 4								MASK
// $324 returns 6715 on 6
// $344 8756   on 8 
// $72 6758   on a				MASK
// $36e 5867   on e				MASK

#endif

		case 0x150/2: // shared#
if (deco16_prot_backbuffer_selected)
		logerror("Protection PC %06x: warning - read %04x for $7e\n",activecpu_get_pc(),deco16_prot_backbuffer[0x7e/2]);
else
		logerror("Protection PC %06x: warning - read %04x for $7e\n",activecpu_get_pc(),deco16_prot_ram[0x7e/2]);

			if (deco16_prot_backbuffer_selected)
				return deco16_prot_backbuffer[0x7e/2];
			return deco16_prot_ram[0x7e/2];

		case 0x6c4/2: /* Schmeizr Robo only - reads from here flip buffers */
			deco16_prot_backbuffer_selected^=1;

			logerror("Protection PC %06x: warning - read flip address %04x - backbuffer now %d\n",activecpu_get_pc(),offset<<1,deco16_prot_backbuffer_selected);


			return ((deco16_prot_ram[0x66/2]&0xf0f0) | ((deco16_prot_ram[0x66/2]&0x000f)<<8)) & (~deco16_prot_ram[0xee/2]);


		case 0x10e/2: /* Schmeizr Robo only - front buffer only read port */
			if (deco16_prot_backbuffer_selected)
				return deco16_prot_backbuffer[0x7c/2];
			return deco16_prot_ram[0x7c/2];
		case 0x56a/2: /* Schmeizr Robo only - front buffer only read port */
			if (deco16_prot_backbuffer_selected)
				return (((deco16_prot_backbuffer[0x7c/2]&0xfff0)>>4) | ((deco16_prot_backbuffer[0x7c/2]&0x000e)<<11) | ((deco16_prot_backbuffer[0x7c/2]&0x0001)<<15)) & (~deco16_prot_backbuffer[0xee/2]);
			return (((deco16_prot_ram[0x7c/2]&0xfff0)>>4) | ((deco16_prot_ram[0x7c/2]&0x000e)<<11) | ((deco16_prot_ram[0x7c/2]&0x0001)<<15)) & (~deco16_prot_ram[0xee/2]);

		case 0x39a/2: /* Schmeizr Robo only */
if (deco16_prot_backbuffer_selected)
		logerror("Protection PC %06x: warning - 39a read %04x for $7e\n",activecpu_get_pc(),deco16_prot_backbuffer[0x7e/2]);
else
		logerror("Protection PC %06x: warning - 39a read %04x for $7e\n",activecpu_get_pc(),deco16_prot_ram[0x7e/2]);


			if (deco16_prot_backbuffer_selected)
				return ((((deco16_prot_backbuffer[0x7e/2]&0xfff0)>>4) | ((deco16_prot_backbuffer[0x7e/2]&0x000e)<<11) | ((deco16_prot_backbuffer[0x7e/2]&0x0001)<<15)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_backbuffer[0xee/2]);
			return ((((deco16_prot_ram[0x7e/2]&0xfff0)>>4) | ((deco16_prot_ram[0x7e/2]&0x000e)<<11) | ((deco16_prot_ram[0x7e/2]&0x0001)<<15)) ^ deco16_prot_ram[0x42/2]) & (~deco16_prot_ram[0xee/2]);
		case 0x188/2: /* Schmeizr Robo only */
	//		logerror("Protection PC %06x: warning - read prot address %04x\n",activecpu_get_pc(),offset<<1);
			return 0;
		case 0x3cc/2: /* Schmeizr Robo only */
	//			logerror("Protection PC %06x: warning - read prot address %04x\n",activecpu_get_pc(),offset<<1);
		return deco16_prot_ram[0xee/2];
		case 0x4a/2: /* Schmeizr Robo only */
			if (deco16_prot_backbuffer_selected)
				return (deco16_prot_backbuffer[0x9e/2]&0xffff) & (~deco16_prot_backbuffer[0xee/2]);
			return (deco16_prot_ram[0x9e/2]&0xffff) & (~deco16_prot_ram[0xee/2]);
		case 0x7e8/2: /* Schmeizr Robo only */
			if (deco16_prot_backbuffer_selected)
				return (deco16_prot_backbuffer[0x4a/2]&0xffff) ^ deco16_prot_ram[0x42/2];
			return (deco16_prot_ram[0x4a/2]&0xffff) ^ deco16_prot_ram[0x42/2];
		case 0xfc/2: /* Schmeizr Robo only */
			if (deco16_prot_backbuffer_selected)
				return deco16_prot_backbuffer[0x4a/2];
			return deco16_prot_ram[0x4a/2];
		case 0x38c/2: /* Schmeizr Robo only */
			if (deco16_prot_backbuffer_selected)
				return deco16_prot_backbuffer[0x28/2];
			return deco16_prot_ram[0x28/2];
		case 0x28/2: /* Schmeizr Robo only  */
			if (deco16_prot_backbuffer_selected)
				return deco16_prot_backbuffer[0x58/2];
			return deco16_prot_ram[0x58/2];
	}

/*
	Sprite routine:

	Writes data into 7c
	puts $0 into $66

	tsts 66
	if 0
	puts data into 7c
	puts ffff into 66


*/

	logerror("Protection PC %06x: warning - read unmapped protection address %04x\n",activecpu_get_pc(),offset<<1);

	return 0;
}

/**********************************************************************************/

WRITE16_HANDLER( deco16_146_nitroball_prot_w )
{
	const int writeport=offset*2;

	if (writeport==0x260) {
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1,0,HOLD_LINE);
		return;
	}

/*		20 30 130 150 170 190 1b0 1c0 1d0 1f0 240 260 290 2b0 2f0 310 340 370
 used	20	  130 150 170     1b0     1d0     240 SND 290 2b0 2f0         370
*/
	if (writeport!=0x20 && writeport!=0x130 && writeport!=0x150 && writeport!=0x170 && writeport!=0x1b0 
		&& writeport!=0x1d0 && writeport!=0x240 && writeport!=0x290 && writeport!=0x2b0
		&& writeport!=0x2f0 && writeport!=0x370)
		logerror("CONTROL PC %06x: warning - write protection memory address %04x %04x\n",activecpu_get_pc(),offset<<1,data);

	COMBINE_DATA(&deco16_prot_ram[offset]);
}

READ16_HANDLER( deco16_146_nitroball_prot_r )
{
	const int readport=offset*2;
	static int a;
	a++;

	/* Nitroball's use of the protection device is quite weak - a range of values are
	written to the chip at boot-time and there are many checks throughout the game
	for certain values at certain locations.  Most checks are quite obvious - a
	jump to an illegal address if the data isn't correct.  This, coupled with the 
	fact there are no extra data writes at game-time, means it is quite easy to
	spot the relationship between read & write addresses.  Even in the 'hard' cases
	it is easy to guess the corret values.
	*/

// 0 a 4c 6a 6c ea 12e 13a 1de 316 3c6 452 4d0 53a 552 54c 582 5da 672 6be 70a 7e0

/*

Protection PC 05abc4: warning - read unmapped memory address 00ea
Warning: sound latch written before being read. Previous: 3f, new: 3c
Protection PC 05ac1a: warning - read unmapped memory address 006c
Protection PC 05abfe: warning - read unmapped memory address 053c
Protection PC 02251c: warning - read unmapped memory address 012e
Protection PC 022680: warning - read unmapped memory address 00ea
Protection PC 02251c: warning - read unmapped memory address 012e
Protection PC 0597c4: warning - read unmapped memory address 053c
Protection PC 04a7e8: warning - read unmapped memory address 006c

  */

	switch (readport) {
	case 0: return a; //Todo

	case 0x582: /* Player 1 & Player 2 */
		return readinputport(0);
	case 0x4c: /* Coins/VBL */
		return readinputport(1);
	case 0x672: /* Dip switches */
		return readinputport(2);

/*
CONTROL PC 078176: warning - write protection memory address 01f0 1ffc
CONTROL PC 078186: warning - write protection memory address 0310 0024
CONTROL PC 07819e: warning - write protection memory address 01c0 0061

  QPRINT	#4, #2, #$0
	QPRINT	#14,#2, #$a
	QPRINT	#24,#2, #$ea
	QPRINT	#34,#2, #$6a
	QPRINT	#44,#2, #$6c
	QPRINT	#54,#2, #$12e
	QPRINT	#64,#2, #$13a
	
		*/

	case 0xa:
//		logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
//may strobe??
		return ((DECO_PORT(0x310)&0x0fff)<<4);

	case 0xea:
		//logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
		return ((DECO_PORT(0x1c0)&0xf000)<<0) | ((DECO_PORT(0x1c0)&0x00ff)<<4);

	case 0x12e:
		//logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
		return ((DECO_PORT(0x1f0)&0xf000)>>4) | ((DECO_PORT(0x1f0)&0x0f00)<<4) | ((DECO_PORT(0x1f0)&0x00f0)>>4) | ((DECO_PORT(0x1f0)&0x000f)<<4);

//untested

	case 0x13a:
// Correct FOR Protection PC 06b694: warning - read unmapped memory address 013a
//		Protection PC 00846a: warning - read unmapped memory address 013a
// Correct FOR Protection PC 0083b4: warning - read unmapped memory address 013a

		//logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
		return 0x2600; //hmm

	case 0x4f6: //todo
		return 0x3800;

	case 0x452: //todo
		return 0x0044;


	case 0x316:
		return ((DECO_PORT(0x290)&0xf000)>>4) | ((DECO_PORT(0x290)&0x0f00)<<4) | ((DECO_PORT(0x290)&0x00ff)<<0);

	case 0x3c6:
		return ((DECO_PORT(0x170)&0xfff0)<<0) | ((DECO_PORT(0x170)&0x000e)>>1) | ((DECO_PORT(0x170)&0x0001)<<3);

	case 0x4d0:
		return ((DECO_PORT(0x20)&0x00f0)<<8) | ((DECO_PORT(0x20)&0x0007)<<9) | ((DECO_PORT(0x20)&0x0008)<<5);

	case 0x53a:
		return ((DECO_PORT(0x370)&0xffff)<<0);

	case 0x552:
		return ((DECO_PORT(0x240)&0xfff0)<<0) | ((DECO_PORT(0x240)&0x0007)<<1) | ((DECO_PORT(0x240)&0x0008)>>3);

	case 0x54c:
		return ((DECO_PORT(0x2f0)&0x00ff)<<8);

	case 0x5da:
		return ((DECO_PORT(0x130)&0x00f0)<<8) | ((DECO_PORT(0x130)&0x000e)<<7) | ((DECO_PORT(0x130)&0x0001)<<11);

	case 0x6be:
		return ((DECO_PORT(0x150)&0xf000)>>12) | ((DECO_PORT(0x150)&0x0ff0)<<0) | ((DECO_PORT(0x150)&0x000f)<<12);

	case 0x70a:
		return ((DECO_PORT(0x1d0)&0x0ff0)<<4) | ((DECO_PORT(0x1d0)&0x0003)<<6) | ((DECO_PORT(0x1d0)&0x000c)<<2);

	case 0x7e0:
		return ((DECO_PORT(0x2b0)&0xfff0)<<0) | ((DECO_PORT(0x2b0)&0x0003)<<2) | ((DECO_PORT(0x2b0)&0x000c)>>2);

	case 0x1de:
		return ((DECO_PORT(0x1b0)&0x0ff0)<<4) | ((DECO_PORT(0x1b0)&0x000e)<<3) | ((DECO_PORT(0x1b0)&0x0001)<<7);
	}

	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset<<1);
	return 0;
}
