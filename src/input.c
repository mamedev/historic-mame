/******************************************************************************

  input.c

  Handle input from the user - keyboard, joystick, etc.

******************************************************************************/

#include "driver.h"


/* return the name of a key */
const char *keyboard_key_name(int keycode)
{
	const struct KeyboardKey *kkey;
	static char buffer[20];


	if (keycode == KEYCODE_NONE) return "None";

	kkey = osd_get_key_list();

	while (kkey->name)
	{
		if (kkey->code == keycode || kkey->standardcode == keycode)
			return kkey->name;

		kkey++;
	}

	sprintf(buffer,"Error %02x",keycode);
	return buffer;
}

/* translate a pseudo key code to to a key code */
static int pseudo_to_key_code(int keycode)
{
	if (keycode >= KEYCODE_START)
	{
		const struct KeyboardKey *kkey;

		kkey = osd_get_key_list();

		while (kkey->name)
		{
			if (kkey->standardcode == keycode)
				return kkey->code;

			kkey++;
		}
	}

    return keycode;
}


int keyboard_key_pressed(int keycode)
{
	if (keycode == KEYCODE_ANY)
		return (keyboard_read_key_immediate() != KEYCODE_NONE);

	keycode = pseudo_to_key_code(keycode);

	return osd_is_key_pressed(keycode);
}


static char memory[KEYCODE_START];

/* Report a key as pressed only when the user hits it, not while it is */
/* being kept pressed. */
int keyboard_key_pressed_memory(int keycode)
{
	int res = 0;

	keycode = pseudo_to_key_code(keycode);

	if (keyboard_key_pressed(keycode))
	{
		if (keycode == KEYCODE_ANY) return 1;

		if (keycode < KEYCODE_START)
		{
			if (memory[keycode] == 0) res = 1;
			memory[keycode] = 1;
		}
	}
	else
	{
		if (keycode < KEYCODE_START)
		{
			memory[keycode] = 0;
		}
	}

	return res;
}

/* report key as pulsing while it is pressed */
int keyboard_key_pressed_memory_repeat(int keycode,int speed)
{
	static int counter,keydelay;
	int res = 0;

	keycode = pseudo_to_key_code(keycode);
	if (keycode >= KEYCODE_START) return 0;

	if (keyboard_key_pressed(keycode))
	{
		if (memory[keycode] == 0)
		{
			keydelay = 3;
			counter = 0;
			res = 1;
		}
		else if (++counter > keydelay * speed * Machine->drv->frames_per_second / 60)
		{
			keydelay = 1;
			counter = 0;
			res = 1;
		}
		memory[keycode] = 1;
	}
	else
		memory[keycode] = 0;

	return res;
}

/* If the user presses a key return it, otherwise return KEYCODE_NONE. */
/* DO NOT wait for the user to press a key */
int keyboard_read_key_immediate(void)
{
	int res;

	/* first of all, record keys which are NOT pressed */
	for (res = 0;res < KEYCODE_START;res++)
	{
		if (!keyboard_key_pressed(res))
		{
			memory[res] = 0;
		}
	}

	for (res = 0;res < KEYCODE_START;res++)
	{
		if (keyboard_key_pressed(res))
		{
			if (memory[res] == 0)
			{
				memory[res] = 1;
				return res;
			}
		}
	}

	return KEYCODE_NONE;
}


int keyboard_debug_readkey(void)
{
	int i,res;
	const struct KeyboardKey *kkey;

	osd_wait_keypress();
	while ((res = keyboard_read_key_immediate()) == KEYCODE_NONE) ;

	kkey = osd_get_key_list();

	while (kkey->name)
	{
		if (kkey->code == res)
			return kkey->standardcode;

		kkey++;
	}

	return res;
}


int keyboard_ui_key_pressed(int code)
{
	return (keyboard_key_pressed_memory(input_port_type_key(code))
			|| osd_joy_pressed(input_port_type_joy(code)));
}


int keyboard_ui_key_pressed_repeat(int code,int speed)
{
	return (keyboard_key_pressed_memory_repeat(input_port_type_key(code),speed)
			|| osd_joy_pressed(input_port_type_joy(code)));
}
