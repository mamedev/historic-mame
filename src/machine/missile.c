/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"
#include "cpuintrf.h"
#include "sndhrdw/pokyintf.h"


/* #define TRACKBALL */

static int ctrld;
static int h_pos, v_pos;

int  missile_video_r(int address);
void missile_video_w(int address, int data);
void missile_video_mult_w(int address, int data);
void missile_video_3rd_bit_w(int address, int data);




/********************************************************************************************/
int missile_read_trackball()
{
	#define KEYMOVE 2

	int xdelta, ydelta;


/* 	mouse support */
	xdelta = readtrakport(0);
	ydelta = readtrakport(1);

	h_pos += xdelta;
	v_pos -= ydelta;

/* 	keyboard & joystick support	 */
	if(osd_key_pressed(OSD_KEY_UP) || osd_joy_pressed(OSD_JOY_UP))
		v_pos += KEYMOVE;
	if(osd_key_pressed(OSD_KEY_DOWN) || osd_joy_pressed(OSD_JOY_DOWN))
		v_pos -= KEYMOVE;
	if(osd_key_pressed(OSD_KEY_LEFT) || osd_joy_pressed(OSD_JOY_LEFT))
		h_pos -= KEYMOVE;
	if(osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_pressed(OSD_JOY_RIGHT))
		h_pos += KEYMOVE;


	return( ((v_pos << 4) & 0xF0)  |  (h_pos & 0x0F));
}



int missile_trakball_r(int data)
{
	#define MAXMOVE 7

	data = data >> 1;
	if(data > MAXMOVE)
		data = MAXMOVE;
	else if(data < -MAXMOVE)
		data = -MAXMOVE;
	return data;
}



/********************************************************************************************/
void missile_4800_w(int offset, int data)
{
	ctrld = data & 1;
}



/********************************************************************************************/
int missile_4008_r(int offset)
{
	return(readinputport(3));
}



/********************************************************************************************/
int missile_4800_r(int offset)
{
	if(ctrld)
		return(missile_read_trackball());
	else
		return (readinputport(0));
}


/********************************************************************************************/
int missile_4900_r(int offset)
{
	return (readinputport(1));
}

/********************************************************************************************/
int missile_4A00_r(int offset)
{
	return (readinputport(2));
}



/********************************************************************************************/
int missile_init_machine(const char *gamename)
{

	h_pos = v_pos = 0;
	return 0;
}


/********************************************************************************************/
void missile_w(int address, int data)
{
	int pc, opcode;


	pc = cpu_getpreviouspc();
	opcode = RAM[pc];

/* 	3 different ways to write to video ram... 		 */

	if((opcode == 0x81) && address >= 0x640){
		/* 	STA ($00,X) */
		missile_video_w(address, data);
		return;
	}

	if(address >= 0x401 && address <= 0x5FF){
		missile_video_3rd_bit_w(address, data);
		return;
	}

	if(address >= 0x640 && address <= 0x3FFF){
			missile_video_mult_w(address, data);
			return;
	}




	if(address == 0x4800){
		ctrld = data & 1;
		return;
	}

	if(address >= 0x4000 && address <= 0x400F){
		pokey1_w(address, data);
		return;
	}

	if(address >= 0x4B00 && address <= 0x4B07){
		RAM[address] = (data & 0x0E) >> 1;
		return;
	}

	RAM[address] = data;
}



/********************************************************************************************/
int missile_r(int address)
{
	int pc, opcode;


	pc = cpu_getpreviouspc();
	opcode = RAM[pc];


	if((opcode == 0xA1) && (address >=0x1900 && address <= 0xFFF9)){
	/* 		LDA ($00,X)  */
		return(missile_video_r(address));
	}

	if(address == 0x4008)
		return(missile_4008_r(0));
	if ((address >= 0x4000) && (address <= 0x400f))
		return(pokey1_r (address & 0x0f));
	if(address == 0x4800)
		return(missile_4800_r(0));
	if(address == 0x4900)
		return(missile_4900_r(0));
	if(address == 0x4A00)
		return(missile_4A00_r(0));

/* 	if(address >= 0xFFF9 && address <= 0xFFFF) */
/* 		return(ROM[address]); */

	return(RAM[address]);
}
