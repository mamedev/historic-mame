#include "driver.h"
#include "Z80.h"
#include "psg.h"


#define AY8910_CLOCK (1832727040)	/* 1.832727040 MHZ?????? */

#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


static int command,command_pending;


int bombjack_soundcommand_r(int offset)
{
   if (command_pending==1)
   {
   if (errorlog) fprintf(errorlog,"received command %02x\n",command);
      command_pending=0;
      return command;
   }
   else
      return 0;
}



void bombjack_soundcommand_w(int offset,int data)
{
	command_pending = 1;
	command = data;
}




int bombjack_sh_interrupt(void)
{
          return Z80_NMI_INT;
}



static int lastreg1;
static int lastreg2;
static int lastreg3;
	/* AY-3-8910 register currently selected */

void bombjack_sh_control_port1_w(int offset,int data)
{
	lastreg1 = data;
}

void bombjack_sh_control_port2_w(int offset,int data)
{
	lastreg2 = data;
}

void bombjack_sh_control_port3_w(int offset,int data)
{
	lastreg3 = data;
}



void bombjack_sh_write_port1_w(int offset,int data)
{
	AYWriteReg(0,lastreg1,data);
}

void bombjack_sh_write_port2_w(int offset,int data)
{
	AYWriteReg(1,lastreg2,data);
}

void bombjack_sh_write_port3_w(int offset,int data)
{
	AYWriteReg(2,lastreg3,data);
}


int bombjack_sh_start(void)
{
	if (AYInit(3,AY8910_CLOCK,emulation_rate,buffer_len,0) == 0)
	{
		return 0;
	}
	else return 1;
}



void bombjack_sh_stop(void)
{
	AYShutdown();
}



void bombjack_sh_update(void)
{
	unsigned char buffer[3][buffer_len];
	static int playing;


	if (play_sound == 0) return;

	AYSetBuffer(0,buffer[0]);
	AYSetBuffer(1,buffer[1]);
	AYSetBuffer(2,buffer[2]);
	AYUpdate();
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(2,buffer[2],buffer_len,emulation_rate,255);

if (!playing)
{
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(2,buffer[2],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(2,buffer[2],buffer_len,emulation_rate,255);
	playing = 1;
}
}
