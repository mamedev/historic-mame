#include "driver.h"

static int test;
static int test_switch;

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
