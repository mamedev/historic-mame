/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

static int earom_offset;
static int earom_data;
static char earom[64];

int atari_vg_earom_r (int offset)
{
	if (errorlog)
		fprintf (errorlog, "read earom: %d\n",offset);
	return (earom_data);
}

void atari_vg_earom_w (int offset, int data)
{
	if (errorlog)
		fprintf (errorlog, "write earom: %d:%d\n",offset, data);
	earom_offset=offset;
	earom_data=data;
}

/* 0,8 and 14 get written to this location, too.
 * Don't know what they do exactly
 */
void atari_vg_earom_ctrl (int offset, int data)
{
	if (errorlog)
		fprintf (errorlog, "earom ctrl: %d:%d\n",offset, data);

	if (data == 9)
		earom_data=earom[earom_offset];
	if (data == 12)
		earom[earom_offset]=earom_data;
}


int atari_vg_earom_load(void)
{
	/* We read the EAROM contents from disk */
        /* No check necessary */
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&earom[0],0x40);
		fclose(f);
	}
	return 1;
}

void atari_vg_earom_save(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&earom[0],0x40);
		fclose(f);
	}
}
