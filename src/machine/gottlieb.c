#include "driver.h"

void gottlieb_video_outputs(int data);
static int test;
static int test_switch;
static int joystick2, joystick3;


static int gottlieb_buttons()
{
	int res = readinputport(1);

	if (osd_key_pressed(OSD_KEY_F1))
	{
		while (osd_key_pressed(OSD_KEY_F1)){};

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
