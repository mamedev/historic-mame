#include "driver.h"
#include "M6502.h"
#include "psg.h"


#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


static int command,command_pending;
static int interrupt_enable;


int btime_soundcommand_r(int offset)
{
if (errorlog) fprintf(errorlog,"received command %02x\n",command);
	return command;
}



void btime_soundcommand_w(int offset,int data)
{
	command_pending = 1;
	command = data;
}



void btime_sh_interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;
}



int btime_sh_interrupt(void)
{
	if (command_pending == 1)
	{
if (errorlog) fprintf(errorlog,"triggering interrupt\n");
		command_pending = 0;
		return INT_IRQ;
	}
	else if (interrupt_enable != 0) return INT_NMI;
	else return INT_NONE;
}



static int lastreg1,lastreg2;	/* AY-3-8910 register currently selected */

void btime_sh_control_port1_w(int offset,int data)
{
	lastreg1 = data;
}

void btime_sh_control_port2_w(int offset,int data)
{
	lastreg2 = data;
}



void btime_sh_write_port1_w(int offset,int data)
{
	AYWriteReg(0,lastreg1,data);
}

void btime_sh_write_port2_w(int offset,int data)
{
	AYWriteReg(1,lastreg2,data);
}



int btime_sh_start(void)
{
	if (AYInit(2,emulation_rate,buffer_len,0) == 0)
	{
		return 0;
	}
	else return 1;
}



void btime_sh_stop(void)
{
	AYShutdown();
}



void btime_sh_update(void)
{
	unsigned char buffer[2][buffer_len];
	static int playing;


	if (play_sound == 0) return;

	AYSetBuffer(0,buffer[0]);
	AYSetBuffer(1,buffer[1]);
	AYUpdate();
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
if (!playing)
{
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	playing = 1;
}
}
