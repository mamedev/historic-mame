#include "driver.h"

extern void gottlieb_video_outputs(int data);
static int test;
static int test_switch;
static int joystick2, joystick3;


static int gottlieb_buttons()
{
	int res = readinputport(1);

	if (osd_key_pressed(OSD_KEY_F1))
	{
		while (osd_key_pressed(OSD_KEY_F1));

		test ^= 1;
	}

	if (test)
		res &= ~test_switch;

	return res;
}

int qbert_IN1_r(int offset)
{
	test_switch=0x40;
	return gottlieb_buttons();
}

int mplanets_IN1_r(int offset)
{
	test_switch=0x80;
	return gottlieb_buttons();
}

int reactor_IN1_r(int offset)
{
	test_switch=0x02;
	return gottlieb_buttons();
}

int krull_IN1_r(int offset)
{
	test_switch=0x01;
	return gottlieb_buttons();
}

int stooges_IN1_r(int offset)
{
	test_switch=0x01;
	return gottlieb_buttons();
}

int stooges_joysticks(int offset)
{
	if (joystick2) return readinputport(5);
	else if (joystick3) return readinputport(6);
	else return readinputport(4);
}

void gottlieb_output(int offset, int data)
{
	joystick2=data&0x20;
	joystick3=data&0x40;
	gottlieb_video_outputs(data);
}

int mplanets_dial_r(int offset)
{
	int res = 0;
	const speed = 2;
	int trak;
	static int countdown=0;

	if (countdown==0) {
		countdown=4;
		if (osd_key_pressed(OSD_KEY_Z) || osd_joy_pressed(OSD_JOY_FIRE3))
			res = -speed;  
		else if (osd_key_pressed(OSD_KEY_X) || osd_joy_pressed(OSD_JOY_FIRE4))
			res = speed;
	}
	countdown--;

	return res;
        trak = input_trak_0_r(0);

	if(trak == NO_TRAK) {
	  return res;
	} else {
	  return trak;
	}
}

int reactor_tb_H_r(int offset)
{
	int res = 0x00;
	const speed = 2;
	int trak;

	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_pressed(OSD_JOY_LEFT))
		res = -speed;
	else if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_pressed(OSD_JOY_RIGHT))
		res = speed;

        trak = input_trak_0_r(0);

	if(trak == NO_TRAK) {
	  return res;
	} else {
	  return trak;
	}
}

int reactor_tb_V_r(int offset)
{
	int res = 0x00;
	const speed = 2;
	int trak;

	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_pressed(OSD_JOY_UP))
		res = -speed;
	else if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_pressed(OSD_JOY_DOWN))
		res = speed;

        trak = input_trak_1_r(0);

	if(trak == NO_TRAK) {
	  return res;
	} else {
	  return trak;
	}
}

int gottlieb_trakball(int data) {
  if(data<-127) {
    return(-127);
  }

  if(data>127) {
    return(127);
  }

  return(data);
}

/*
int mplanets_dial_r(int offset)
{
	int res = 0;
	const speed = 2;
	static int countdown=0;

	if (countdown==0) {
		countdown=4;
		if (osd_key_pressed(OSD_KEY_Z) || osd_joy_pressed(OSD_JOY_FIRE3))
			res = -speed;  
		else if (osd_key_pressed(OSD_KEY_X) || osd_joy_pressed(OSD_JOY_FIRE4))
			res = speed;
	}
	countdown--;
	return res;
}

int reactor_tb_H_r(int offset)
{
	int res = 0x00;
	const speed = 2;

	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_pressed(OSD_JOY_LEFT))
		res = -speed;
	else if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_pressed(OSD_JOY_RIGHT))
		res = speed;

	return res;
}

int reactor_tb_V_r(int offset)
{
	int res = 0x00;
	const speed = 2;

	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_pressed(OSD_JOY_UP))
		res = -speed;
	else if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_pressed(OSD_JOY_DOWN))
		res = speed;

	return res;
}
*/