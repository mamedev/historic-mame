#include "driver.h"


int gottlieb_nvram_load(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* Try loading static RAM */
	/* no need to wait for anything: Krull doesn't touch the tables
		if the checksum is correct */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* just load in everything, the game will overwrite what it doesn't want */
		osd_fread(f,&RAM[0x0000],0x1000);
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(&RAM[0x0000],0xff,0x1000);

	return 1;
}

void gottlieb_nvram_save(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0000],0x1000);
		osd_fclose(f);
	}
}
