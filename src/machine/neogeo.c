#include "driver.h"

static unsigned char *biosbank;
static unsigned char *memcard;
unsigned char *neogeo_ram;
unsigned char *neogeo_sram;

extern int neogeo_rom_loaded;

void neogeo_init_machine (void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    void *f;

	/* skip tests, start game - ram will not be set up properly..
	WRITE_WORD(&RAM[4],0);
	WRITE_WORD(&RAM[6],0x122); */

	/* Patch bios rom, for Calendar errors */
	RAM = Machine->memory_region[4];
    WRITE_WORD(&RAM[0x11c14],0x4e71);
    WRITE_WORD(&RAM[0x11c16],0x4e71);
    WRITE_WORD(&RAM[0x11c1c],0x4e71);
    WRITE_WORD(&RAM[0x11c1e],0x4e71);

    /* Rom internal checksum fails for now.. */
    WRITE_WORD(&RAM[0x11c62],0x4e71);
    WRITE_WORD(&RAM[0x11c64],0x4e71);

	/* Remove memory check for now */
    WRITE_WORD(&RAM[0x11b00],0x4ef9);
    WRITE_WORD(&RAM[0x11b02],0x00c1);
    WRITE_WORD(&RAM[0x11b04],0x1b6a);

	/* allocate and clear the RAM banks */
	if (!neogeo_ram)
		neogeo_ram = malloc (0x10000);
	memset (neogeo_ram, 0, 0x10000);

	if (!neogeo_sram)
		neogeo_sram = malloc (0x10000);

    /* Load the SRAM settings for this game */
	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, neogeo_sram, 0x10000);
		osd_fclose (f);
	}
    else {
		memset (neogeo_sram, 0, 0x10000);

        /* If we have no SRAM settings we need to set up machine defaults */
		WRITE_WORD(&neogeo_sram[0x3a],0x0101); /* Coin slot 1: 1 coin 1 credit */
		WRITE_WORD(&neogeo_sram[0x3c],0x0102); /* Coin slot 2: 1 coin 2 credits */
        /* Without above two settings, coin inputs won't work at all! */

		WRITE_WORD(&neogeo_sram[0x3e],0x0101); /* Service switch parameters? */

		WRITE_WORD(&neogeo_sram[0x40],0x0103); /* Unknown */

		WRITE_WORD(&neogeo_sram[0x42],0x0000); /* Game start only when credited */
		WRITE_WORD(&neogeo_sram[0x44],0x303b); /* Game start compulsion is 1st byte */
		WRITE_WORD(&neogeo_sram[0x46],0x0000); /* Attract mode sound set per game */

    }

	/* Credits are stored in the SRAM in case of power failures, we don't really
		want this behaviour in Mame, so zero credit entry on bootup.  Without
        this a game will go straight to intro screen on bootup if credits are
        in the system */
	WRITE_WORD(&neogeo_sram[0x34],0x0000);

	/* If the ROM has already been loaded, skip this code */
	if (neogeo_rom_loaded) return;

	cpu_setbank (1, neogeo_ram);
	cpu_setbank (2, neogeo_sram);

	/* Set the biosbank */
	cpu_setbank (3, Machine->memory_region[4]);

	/* Set the 2nd ROM bank */
    RAM = Machine->memory_region[0];
	cpu_setbank (4, &RAM[0x100000]);

	/* Allocate and point to the memcard - bank 5 */
	if (!memcard)
		memcard = calloc (0x1000, 1);

	cpu_setbank (5, memcard);

	/* Set this flag so that machine resets, etc. don't run this code again */
	neogeo_rom_loaded = 1;
}

