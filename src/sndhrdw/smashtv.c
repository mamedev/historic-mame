/*************************************************************************
 Preliminary driver for Williams/Midway games using the TMS34010 processor
 This is very much a work in progress.

 sound hardware

**************************************************************************/
#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/williams.h"
#include "machine/6821pia.h"

void smashtv_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	williams_cvsd_data_w(0, (data & 0xff) | ((data & 0x200) >> 1));
}
void trog_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data|0x100);
	williams_cvsd_data_w(0, (data & 0xff) | ((data & 0x200) >> 1));
}
void narc_sound_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
//	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	williams_narc_data_w(0, data);
}
void mk_sound_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
//	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	williams_adpcm_data_w(0, data);
}
void nbajam_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	williams_adpcm_data_w(0, data);
}
