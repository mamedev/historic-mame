/***************************************************************************

  8910intf.c

  Another lovable case of MS-DOS 8+3 name. That stands for 8910 interface.
  Many games use the AY-3-8910 to produce sound; the functions contained in
  this file make it easier to interface with it.

***************************************************************************/

#include "driver.h"
#include "psg.h"
#include "sndhrdw/8910intf.h"


#define TARGET_EMULATION_RATE 44100	/* will be adapted to be a multiple of buffer_len */
static int emulation_rate;
static int buffer_len;



static struct AY8910interface *intf;
static unsigned char *buffer[MAX_8910];



static unsigned char porthandler(int num,AY8910 *chip, int port, int iswrite, unsigned char val)
{
	if (iswrite)
	{
		if (port == 0x0e)
		{
			if (intf->portAwrite[num]) (*intf->portAwrite[num])(0,val);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - write %02x to 8910 #%d Port A\n",cpu_getpc(),val,num);
		}
		else
		{
			if (intf->portBwrite[num]) (*intf->portBwrite[num])(1,val);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - write %02x to 8910 #%d Port B\n",cpu_getpc(),val,num);
		}
	}
	else
	{
		chip->Regs[port] = 0;

		if (port == 0x0e)
		{
			if (intf->portAread[num]) chip->Regs[port] = (*intf->portAread[num])(0);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - read 8910 #%d Port A\n",cpu_getpc(),num);
		}
		else
		{
			if (intf->portBread[num]) chip->Regs[port] = (*intf->portBread[num])(1);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - read 8910 #%d Port B\n",cpu_getpc(),num);
		}
	}

	return 0;
}



static unsigned char porthandler0(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	return porthandler(0,chip,port,iswrite,val);
}
static unsigned char porthandler1(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	return porthandler(1,chip,port,iswrite,val);
}
static unsigned char porthandler2(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	return porthandler(2,chip,port,iswrite,val);
}
static unsigned char porthandler3(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	return porthandler(3,chip,port,iswrite,val);
}
static unsigned char porthandler4(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	return porthandler(4,chip,port,iswrite,val);
}



int AY8910_sh_start(struct AY8910interface *interface)
{
	int i;


	intf = interface;

	buffer_len = TARGET_EMULATION_RATE / Machine->drv->frames_per_second / intf->updates_per_frame;
	emulation_rate = buffer_len * Machine->drv->frames_per_second * intf->updates_per_frame;

	for (i = 0;i < MAX_8910;i++) buffer[i] = 0;
	for (i = 0;i < intf->num;i++)
	{
		if ((buffer[i] = malloc(buffer_len * intf->updates_per_frame)) == 0)
		{
			while (--i >= 0) free(buffer[i]);
			return 1;
		}
		memset(buffer[i],0x80,buffer_len * intf->updates_per_frame);
	}

	if (AYInit(intf->num,intf->clock,emulation_rate,buffer_len,0) == 0)
	{
		AYSetPortHandler(0,AY_PORTA,porthandler0);
		AYSetPortHandler(0,AY_PORTB,porthandler0);
		if (intf->num > 1)
		{
			AYSetPortHandler(1,AY_PORTA,porthandler1);
			AYSetPortHandler(1,AY_PORTB,porthandler1);
		}
		if (intf->num > 2)
		{
			AYSetPortHandler(2,AY_PORTA,porthandler2);
			AYSetPortHandler(2,AY_PORTB,porthandler2);
		}
		if (intf->num > 3)
		{
			AYSetPortHandler(3,AY_PORTA,porthandler3);
			AYSetPortHandler(3,AY_PORTB,porthandler3);
		}
		if (intf->num > 4)
		{
			AYSetPortHandler(4,AY_PORTA,porthandler4);
			AYSetPortHandler(4,AY_PORTB,porthandler4);
		}

		return 0;
	}
	else return 1;
}



void AY8910_sh_stop(void)
{
	int i;


	AYShutdown();
	for (i = 0;i < intf->num;i++) free(buffer[i]);
}



static int lastreg0,lastreg1,lastreg2,lastreg3,lastreg4;	/* AY-3-8910 register currently selected */


int AY8910_read_port_0_r(int offset)
{
	return AYReadReg(0,lastreg0);
}
int AY8910_read_port_1_r(int offset)
{
	return AYReadReg(1,lastreg1);
}
int AY8910_read_port_2_r(int offset)
{
	return AYReadReg(2,lastreg2);
}
int AY8910_read_port_3_r(int offset)
{
	return AYReadReg(3,lastreg3);
}
int AY8910_read_port_4_r(int offset)
{
	return AYReadReg(4,lastreg4);
}



void AY8910_control_port_0_w(int offset,int data)
{
	lastreg0 = data;
}
void AY8910_control_port_1_w(int offset,int data)
{
	lastreg1 = data;
}
void AY8910_control_port_2_w(int offset,int data)
{
	lastreg2 = data;
}
void AY8910_control_port_3_w(int offset,int data)
{
	lastreg3 = data;
}
void AY8910_control_port_4_w(int offset,int data)
{
	lastreg4 = data;
}



void AY8910_write_port_0_w(int offset,int data)
{
	AYWriteReg(0,lastreg0,data);
}
void AY8910_write_port_1_w(int offset,int data)
{
	AYWriteReg(1,lastreg1,data);
}
void AY8910_write_port_2_w(int offset,int data)
{
	AYWriteReg(2,lastreg2,data);
}
void AY8910_write_port_3_w(int offset,int data)
{
	AYWriteReg(3,lastreg3,data);
}
void AY8910_write_port_4_w(int offset,int data)
{
	AYWriteReg(4,lastreg4,data);
}



static int updatecount;


void AY8910_update(void)
{
	int i;


	if (updatecount >= intf->updates_per_frame) return;

	for (i = 0;i < intf->num;i++)
		AYSetBuffer(i,&buffer[i][updatecount * buffer_len]);
	AYUpdate();

	updatecount++;
}



void AY8910_sh_update(void)
{
	int i;


	if (play_sound == 0) return;

	if (intf->updates_per_frame == 1) AY8910_update();

if (errorlog && updatecount != intf->updates_per_frame)
	fprintf(errorlog,"Error: AY8910_update() has not been called %d times in a frame\n",intf->updates_per_frame);

	updatecount = 0;	/* must be zeroed here to keep in sync in case of a reset */

	for (i = 0;i < intf->num;i++)
		AYSetBuffer(i,&buffer[i][0]);

	/* update FM music */
    osd_ym2203_update();

	for (i = 0;i < intf->num;i++)
		osd_play_streamed_sample(i,buffer[i],buffer_len * intf->updates_per_frame,emulation_rate,intf->volume[i]);
}
