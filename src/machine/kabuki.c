/***************************************************************************

"Kabuki" Z80 encryption


The "Kabuki" is a custom Z80 module which runs encrypted code. The encryption
key is stored in some battery-backed RAM, therefore the chip has the annoying
habit of stopping working every few years, when the battery dies.
Check at the bottom of this text to see a list of all the known games which
use this chip. They are only a handful, and there are probably no more (though
other versions might use different keys, like Super Pang / Super Buster Bros).


How it works:
The base operation is a bit swap which affects couples of adjacent bits.
Each of the 4 couples may or may not be swapped, depending on the address of
the byte and on whether it is an opcode or data.
The decryption consists of these steps:
- bitswap
- ROL
- bitswap
- XOR with a key
- ROL
- bitswap
- ROL
- bitswap

To know how to apply the byteswap, take the address of the byte to decode and:
- if the byte is an opcode, add addr_key to the address
- if the byte is data, XOR the address with 1FC0, add 1, and then add addr_key
You'll get a 16-bit word. The first two bitswaps depend on bits 0-7 of that
word, while the second two on bits 8-15. When a bit in the word is 1, swap the
two bits, oherwise don't. The exact couple of bits affected depends on the
game and is identified in this file with two keys: swap_key1 and swap_key2
(which are just permutations of the numbers 0-7, not full 32-bit integers).


Key space size:
- swap_key1  8! = 40320
- swap_key2  8! = 40320
- addr_key   2^16 = 65536
- xor_key    2^8 = 256
- total      2.7274 * 10^16


Weaknesses:
- 0x00 and 0xff, having all the bits set to the same value, are not affected
  by bit permutations after the XOR. Therefore, their encryption is the same
  regardless of the high 8 bits of the address, and of the value of
  swap_key2. If there is a long stream of 0x00 or 0xff in the original data,
  this can be used to find by brute force all the candidates for swap_key1,
  xor_key, and for the low 8 bits of addr_key. This is a serious weakness
  which dramatically reduces the security of the encrytion.
- A 0x00 is always encrypted as a byte with as many 1s as xor_key; a 0xff is
  always encrypted as a byte with as many 0s as xor_key has 1s. So you just
  need to know one 0x00 or 0xff in the unencrypted data to know how many 1s
  there are in xor_key.
- Once you have restricted the range for swap_key1 and you know the number of
  1s in the xor_key, you can easily use known plaintext attacks and brute
  force to find the remaining keys. Long strings like THIS GAME IS FOR USE IN
  and ABCDEFGHIJKLMNOPQRSTUVWXYZ can be found by comparing the number of 1s
  in the clear and encrypted data, taking xor_key into account. When you have
  found where the string is, use brute force to reduce the key space.


Known games:
                                swap_key1  swap_key2  addr_key  xor_key
Pang / Buster Bros              32104567   76540123     6548      24
Super Pang                      76540123   45673210     5852      43
Super Buster Bros               76540123   45673210     2130      12
Block Block                     64201357   64201357     0002      01
Warriors of Fate                32104567   54162703     5151      51
Cadillacs and Dinosaurs         45673210   24607531     4343      43
Punisher                        54762103   75314206     2222      22
Slam Masters                    23451076   65437012     3131      19

***************************************************************************/

#include "driver.h"



static int bitswap(int src,int key,int select)
{
	if (select & (1 << ((key >> 0) & 7)))
		src = (src & 0xfc) | ((src & 0x01) << 1) | ((src & 0x02) >> 1);
	if (select & (1 << ((key >> 4) & 7)))
		src = (src & 0xf3) | ((src & 0x04) << 1) | ((src & 0x08) >> 1);
	if (select & (1 << ((key >> 8) & 7)))
		src = (src & 0xcf) | ((src & 0x10) << 1) | ((src & 0x20) >> 1);
	if (select & (1 << ((key >>12) & 7)))
		src = (src & 0x3f) | ((src & 0x40) << 1) | ((src & 0x80) >> 1);

	return src;
}

static int bytedecode(int src,int swap_key1,int swap_key2,int xor_key,int select)
{
	src = bitswap(src,swap_key1 & 0xffff,select & 0xff);
	src = ((src & 0x7f) << 1) | ((src & 0x80) >> 7);
	src = bitswap(src,swap_key1 >> 16,select & 0xff);
	src ^= xor_key;
	src = ((src & 0x7f) << 1) | ((src & 0x80) >> 7);
	src = bitswap(src,swap_key2 & 0xffff,select >> 8);
	src = ((src & 0x7f) << 1) | ((src & 0x80) >> 7);
	src = bitswap(src,swap_key2 >> 16,select >> 8);
	return src;
}

void kabuki_decode(unsigned char *src,unsigned char *dest_op,unsigned char *dest_data,
		int base_addr,int length,int swap_key1,int swap_key2,int addr_key,int xor_key)
{
	int A;
	int select;

	for (A = 0;A < length;A++)
	{
		/* decode opcodes */
		select = (A + base_addr) + addr_key;
		dest_op[A] = bytedecode(src[A],swap_key1,swap_key2,xor_key,select);

		/* decode data */
		select = ((A + base_addr) ^ 0x1fc0) + addr_key + 1;
		dest_data[A] = bytedecode(src[A],swap_key1,swap_key2,xor_key,select);
	}
}


extern int encrypted_cpu;

void bbros_decode(void)
{
	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x32104567,0x76540123,0x6548,0x24);
	for (i = 0x10000;i < 0x30000;i += 0x4000)
		kabuki_decode(RAM+i,ROM+i,RAM+i,0x8000,0x4000, 0x32104567,0x76540123,0x6548,0x24);
}

void spang_decode(void)
{
	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x76540123,0x45673210,0x5852,0x43);
	for (i = 0x10000;i < 0x50000;i += 0x4000)
		kabuki_decode(RAM+i,ROM+i,RAM+i,0x8000,0x4000, 0x76540123,0x45673210,0x5852,0x43);
}

void sbbros_decode(void)
{
	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x76540123,0x45673210,0x2130,0x12);
	for (i = 0x10000;i < 0x50000;i += 0x4000)
		kabuki_decode(RAM+i,ROM+i,RAM+i,0x8000,0x4000, 0x76540123,0x45673210,0x2130,0x12);
}

void block_decode(void)
{
	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x64201357,0x64201357,0x0002,0x01);
	for (i = 0x10000;i < 0x50000;i += 0x4000)
		kabuki_decode(RAM+i,ROM+i,RAM+i,0x8000,0x4000, 0x64201357,0x64201357,0x0002,0x01);
}

void wof_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	encrypted_cpu = 1;
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x32104567,0x54162703,0x5151,0x51);
}

void dino_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	encrypted_cpu = 1;
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x45673210,0x24607531,0x4343,0x43);
}

void punisher_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	encrypted_cpu = 1;
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x54762103,0x75314206,0x2222,0x22);
}

void slammast_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	encrypted_cpu = 1;
	kabuki_decode(RAM,ROM,RAM,0x0000,0x8000, 0x23451076,0x65437012,0x3131,0x19);
}
