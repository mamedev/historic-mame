/*
	CD-DA "Red Book" audio sound hardware handler
	Relies on the actual CD logic and reading in cdrom.c.
*/

#include "driver.h"
#include "cpuintrf.h"
#include "cdrom.h"

static struct cdrom_file *cdrom_disc[MAX_CDDA];

static void cdda_update(int num, INT16 **outputs, int length) 
{
	if (cdrom_disc[num] != (struct cdrom_file *)NULL)
	{
		cdrom_get_audio_data(cdrom_disc[num], &outputs[0][0], &outputs[1][0], length);
	}
}

int CDDA_sh_start( const struct MachineSound *msound )
{
	char buf[2][40];
	const char *name[2];
	int  vol[2];
	struct CDDAinterface *intf;
	int i;

	intf = msound->sound_interface;

	for(i=0; i<intf->num; i++)
	{
		sprintf(buf[0], "CD-DA %d L", i);
		sprintf(buf[1], "CD-DA %d R", i);
		name[0] = buf[0];
		name[1] = buf[1];
		vol[0]=intf->mixing_level[i] >> 16;
		vol[1]=intf->mixing_level[i] & 0xffff;
		stream_init_multi(2, name, vol, 44100, i, cdda_update);
	}

	return 0;
}

void CDDA_sh_stop( void )
{
}

void CDDA_set_cdrom(int num, void *file)
{
	cdrom_disc[num] = (struct cdrom_file *)file;
}
