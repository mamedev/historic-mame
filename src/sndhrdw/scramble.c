#include "driver.h"
#include "Z80.h"
#include "psg.h"



#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


static int command,command_pending;



extern void scramble_soundcommand_w(int offset,int data)
{
	/* actually I should trigger the interrupt when bit 3 of 8201 is set to 1 */
	command_pending = 1;
	command = data;
}



static unsigned char porthandler(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	if (iswrite)
	{
	}
	else
	{
		if (port == 0x0e)
		{
			chip->Regs[port] = command;
		}
		else if (port == 0x0f)
		{
			static int clock;	/* this is completely inaccurate!!!! */


			chip->Regs[port] = clock;
			clock ^= 0x80;
		}
	}

	return 0;
}



int scramble_sh_interrupt(void)
{
	if (command_pending == 1)
	{
		command_pending = 0;
		return 0xff;
	}
	else return Z80_IGNORE_INT;
}



int scramble_sh_start(void)
{
	if (AYInit(2,emulation_rate,buffer_len,0) == 0)
	{
		AYSetPortHandler(0,AY_PORTA,porthandler);
		AYSetPortHandler(0,AY_PORTB,porthandler);

		return 0;
	}
	else return 1;
}



void scramble_sh_stop(void)
{
	AYShutdown();
}



int static lastreg1,lastreg2;	/* AY-3-8910 register currently selected */

int scramble_sh_read_port1_r(int offset)
{
	return AYReadReg(0,lastreg1);
}



void scramble_sh_control_port1_w(int offset,int data)
{
	lastreg1 = data;
}



void scramble_sh_write_port1_w(int offset,int data)
{
	AYWriteReg(0,lastreg1,data);
}



int scramble_sh_read_port2_r(int offset)
{
	return AYReadReg(1,lastreg2);
}



void scramble_sh_control_port2_w(int offset,int data)
{
	lastreg2 = data;
}



void scramble_sh_write_port2_w(int offset,int data)
{
	AYWriteReg(1,lastreg2,data);
}



void scramble_sh_update(void)
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
