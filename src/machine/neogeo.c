#include "driver.h"

static unsigned char *biosbank;
static unsigned char *memcard;
unsigned char *rambank;
unsigned char *rambank2;

extern int neogeo_rom_loaded;

void neogeo_init_machine (void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

#ifdef LSB_FIRST
		/* skip tests, start game */
   //     WRITE_WORD(&RAM[4],0);
   //     WRITE_WORD(&RAM[4],0x122);

   	RAM[0x4] = 0x00;
	RAM[0x5] = 0x00;
	RAM[0x6] = 0x22;
	RAM[0x7] = 0x01;
#else
   	RAM[0x04] = 0x00;
	RAM[0x05] = 0x00;
	RAM[0x06] = 0x01;
	RAM[0x07] = 0x22;
#endif

	/* allocate and clear the RAM banks */
	if (!rambank)
		rambank = malloc (0x10000);
	memset (rambank, 0, 0x10000);

	if (!rambank2)
		rambank2 = malloc (0x10000);
	memset (rambank2, 0, 0x10000);

	/* If the ROM has already been loaded, skip this code */
	if (neogeo_rom_loaded) return;

	cpu_setbank (1, rambank);
	cpu_setbank (2, rambank2);

	/* Set the biosbank */
	cpu_setbank (3, Machine->memory_region[2]);

	/* Set the 2nd ROM bank */
	cpu_setbank (4, &RAM[0x100000]);

	/* Allocate and point to the memcard - bank 5 */
	if (!memcard)
		memcard = calloc (0x1000, 1);

	cpu_setbank (5, memcard);

	/* Set this flag so that machine resets, etc. don't run this code again */
	neogeo_rom_loaded = 1;
}
