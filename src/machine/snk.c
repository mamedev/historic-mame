#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


int bg_video_offs=0;
int p1ctr_count=0, p2ctr_count=0;
int p1_daial=0, p2_daial=0, daial_max=0;

int DAIAL_8[8]   = { 0xF0,0x30,0x10,0x50,0x40,0xC0,0x80,0xA0 };
int DAIAL_12[12] = { 0xB0,0xA0,0x90,0x80,0x70,0x60,0x50,0x40,0x30,0x20,0x10,0x00 };

int mf800=0xf;
int snk_soundcommand=0;

unsigned char *snk_hrdwmem;
unsigned char *snk_sharedram;

extern unsigned char *snk_bg_dirtybuffer;

int snk_read_port1(void)
{
	p1ctr_count++;

	if (p1ctr_count > 10)
	{
		p1ctr_count = 0;
		if (osd_key_pressed(OSD_KEY_X)) p1_daial++;
		if (osd_key_pressed(OSD_KEY_Z)) p1_daial--;
		if (p1_daial < 0) p1_daial = daial_max;
		if (p1_daial > daial_max) p1_daial = 0;
	}

	if (daial_max > 8)
		return ((input_port_1_r(0) & 15) | DAIAL_12[p1_daial]);
	else
		return ((input_port_1_r(0) & 15) | DAIAL_8[p1_daial]);
}

int snk_read_port2(void)
{
	p2ctr_count++;

	if (p2ctr_count > 10)
	{
		p2ctr_count = 0;
		if (osd_key_pressed(OSD_KEY_M)) p2_daial++;
		if (osd_key_pressed(OSD_KEY_N)) p2_daial--;
		if (p2_daial < 0) p2_daial = daial_max;
		if (p2_daial > daial_max) p2_daial = 0;
	}

	if (daial_max > 8)
		return ((input_port_2_r(0) & 15) | DAIAL_12[p2_daial]);
	else
		return ((input_port_2_r(0) & 15) | DAIAL_8[p2_daial]);
}


int snk_hrdwmem_r(int offset)
{

	if (offset==0x000)			/* COINS, START */
		return input_port_0_r(0);
	if (offset==0x100)			/* PLAYER 1 CONTROLS */
		return snk_read_port1();
	if (offset==0x200)			/* PLAYER 2 CONTROLS */
		return snk_read_port2();
	if (offset==0x300)			/* PLAYERS BUTTONS */
		return input_port_3_r(0);
	if (offset==0x500)			/* DIP SW1 */
		return input_port_4_r(0);
	if (offset==0x600)			/* DIP SW2 */
		return input_port_5_r(0);

	if (offset==0x700)
	{
		cpu_cause_interrupt(1, Z80_NMI_INT);
		return 0;
	}

	/* SKIP HARD FRONT CHECK */
	if (offset==0xe00 && cpu_getactivecpu()==0)
	{
		Z80_Regs _regs;

		Z80_GetRegs(&_regs);
		_regs.SP.D += 2;
		_regs.PC.D = cpu_getreturnpc();
		Z80_SetRegs(&_regs);
		return 0xff;
	}

	/* HARD FLAGS */
	if (offset > 0xe00 && offset < 0xee0) return 0xff;
	if (offset == 0xee0) return 0x3;

	return 0xff;
}

void snk_hrdwmem_w(int offset, int data)
{
	snk_hrdwmem[offset] = data;
	if (offset==0x400) snk_soundcommand = data;
}

int snk_sharedram_r(int offset)
{
	return snk_sharedram[offset];
}

void snk_sharedram_w(int offset, int data)
{
	if (offset >= bg_video_offs && offset < bg_video_offs+0x800)
	{
		snk_bg_dirtybuffer[(offset - bg_video_offs) >> 1] = 1;
	}
	snk_sharedram[offset] = data;
}

void snk_init_machine(void)
{
	p1ctr_count=0;
	p2ctr_count=0;
	p1_daial=0;
	p2_daial=0;
}

void ikarius_init_data(void)
{
	daial_max=11;
	bg_video_offs = 0x800;
}

void ikarijp_init_data(void)
{
	daial_max=11;
	bg_video_offs = 0x000;
}

void ikarijpb_init_data(void)
{
	daial_max=7;
	bg_video_offs = 0x000;
}


/* this is wrong of course */

int snk_read_f800(int offset)
{
	return mf800;
}

void snk_write_f800(int offset, int data)
{
	mf800 = data;
}

int  snk_soundcommand_r(int offset)
{
	int val = snk_soundcommand;

	snk_soundcommand = 0;
	return val;
}




