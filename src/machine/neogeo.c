#include "driver.h"

static unsigned char *biosbank;
static unsigned char *memcard;
static unsigned char *rambank;
static int    onlyonce=1;

void neogeo_init_machine(void) {
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (onlyonce) {
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

		/* allocate the biosbank */
		if (!biosbank)
			biosbank = calloc(0x20000, 1);

		bcopy(&(RAM[0x100000]),biosbank,0x20000);
		cpu_setbank(1,biosbank);

		/* Clear RAM */
		memset(&(RAM[0x100000]),0,0x10000);

		if (!memcard)
			memcard = calloc (0x1000, 1);

		/* point to the memcard bank */
		cpu_setbank (2, memcard);

		if (!rambank)
			rambank = calloc (0x10000, 1);

		/* point to the second rambank bank */
		cpu_setbank (3, rambank);
		onlyonce = 0;

        /* If we have a large rom-set, the 2nd half is mapped here */
        // if (big_romset??) How do I check if the area is used??
        cpu_setbank(4,&RAM[0x120000]);


	}
}

