/******************************************************************************

  input.c

  Handle input from the user - keyboard, joystick, etc.

******************************************************************************/

#include "driver.h"

#define MAX_KEYS 256	/* allow up to 256 entries in the key list */
#define MAX_JOYS 256	/* allow up to 256 entries in the joy list */


const char *keyboard_name(int keycode)
{
	const struct KeyboardInfo *keyinfo;


	if (keycode == KEYCODE_NONE) return "None";

	keyinfo = osd_get_key_list();

	while (keyinfo->name)
	{
		if (keyinfo->code == keycode || keyinfo->standardcode == keycode)
			return keyinfo->name;

		keyinfo++;
	}

	return "n/a";
}


static int pseudo_to_key_code(int *keycode,int *entry)
{
	const struct KeyboardInfo *keyinfo;
	int i;

	keyinfo = osd_get_key_list();

	for (i = 0;keyinfo[i].name;i++)
	{
		if (keyinfo[i].standardcode == *keycode || keyinfo[i].code == *keycode)
		{
			*keycode = keyinfo[i].code;
			*entry = i;
			return 0;
		}
	}

	return 1;
}


int keyboard_pressed(int keycode)
{
	int entry;

	if (pseudo_to_key_code(&keycode,&entry) != 0) return 0;

	return osd_is_key_pressed(keycode);
}


static char keymemory[MAX_KEYS];

int keyboard_pressed_memory(int keycode)
{
	int res = 0;
	int entry;

	if (pseudo_to_key_code(&keycode,&entry) != 0) return 0;

	if (osd_is_key_pressed(keycode))
	{
		if (keymemory[entry] == 0) res = 1;
		keymemory[entry] = 1;
	}
	else
		keymemory[entry] = 0;

	return res;
}


int keyboard_pressed_memory_repeat(int keycode,int speed)
{
	static int counter,keydelay;
	int res = 0;
	int entry;

	if (pseudo_to_key_code(&keycode,&entry) != 0) return 0;

	if (osd_is_key_pressed(keycode))
	{
		if (keymemory[entry] == 0)
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
		keymemory[entry] = 1;
	}
	else
		keymemory[entry] = 0;

	return res;
}


int keyboard_read_async(void)
{
	int i;
	const struct KeyboardInfo *keyinfo;


	/* first of all, record keys which are NOT pressed */
	keyinfo = osd_get_key_list();

	for (i = 0;keyinfo[i].name;i++)
	{
		if (!osd_is_key_pressed(keyinfo[i].code))
			keymemory[i] = 0;
	}

	for (i = 0;keyinfo[i].name;i++)
	{
		if (osd_is_key_pressed(keyinfo[i].code))
		{
			if (keymemory[i] == 0)
			{
				keymemory[i] = 1;

				if (keyinfo[i].standardcode != KEYCODE_OTHER) return keyinfo[i].standardcode;
				else return keyinfo[i].code;
			}
		}
	}

	return KEYCODE_NONE;
}


int keyboard_read_sync(void)
{
	int res;


	/* now let the OS process it */
	res = osd_wait_keypress();
	if (res != KEYCODE_NONE)
	{
		int i;
		const struct KeyboardInfo *keyinfo;


		keyinfo = osd_get_key_list();

		for (i = 0;keyinfo[i].name;i++)
		{
			if (res == keyinfo[i].code || res == keyinfo[i].standardcode)
			{
				keymemory[i] = 1;

				if (keyinfo[i].standardcode != KEYCODE_OTHER) return keyinfo[i].standardcode;
				else return keyinfo[i].code;
			}
		}

		res = KEYCODE_NONE;
	}

	while (res == KEYCODE_NONE)
		res = keyboard_read_async();

	return res;
}



const char *joystick_name(int joycode)
{
	const struct JoystickInfo *joyinfo;


	if (joycode == JOYCODE_NONE) return "None";

	joyinfo = osd_get_joy_list();

	while (joyinfo->name)
	{
		if (joyinfo->code == joycode || joyinfo->standardcode == joycode)
			return joyinfo->name;

		joyinfo++;
	}

	return "n/a";
}


static int pseudo_to_joy_code(int *joycode,int *entry)
{
	const struct JoystickInfo *joyinfo;
	int i;

	joyinfo = osd_get_joy_list();

	for (i = 0;joyinfo[i].name;i++)
	{
		if (joyinfo[i].standardcode == *joycode || joyinfo[i].code == *joycode)
		{
			*joycode = joyinfo[i].code;
			*entry = i;
			return 0;
		}
	}

	return 1;
}


int joystick_pressed(int joycode)
{
	int entry;

	if (pseudo_to_joy_code(&joycode,&entry) != 0) return 0;

	return osd_is_joy_pressed(joycode);
}


static char joymemory[MAX_JOYS];

int joystick_pressed_memory(int joycode)
{
	int res = 0;
	int entry;

	if (pseudo_to_joy_code(&joycode,&entry) != 0) return 0;

	if (osd_is_joy_pressed(joycode))
	{
		if (joymemory[entry] == 0) res = 1;
		joymemory[entry] = 1;
	}
	else
		joymemory[entry] = 0;

	return res;
}


int joystick_pressed_memory_repeat(int joycode,int speed)
{
	static int counter,joydelay;
	int res = 0;
	int entry;

	if (pseudo_to_joy_code(&joycode,&entry) != 0) return 0;

	if (osd_is_joy_pressed(joycode))
	{
		if (joymemory[entry] == 0)
		{
			joydelay = 3;
			counter = 0;
			res = 1;
		}
		else if (++counter > joydelay * speed * Machine->drv->frames_per_second / 60)
		{
			joydelay = 1;
			counter = 0;
			res = 1;
		}
		joymemory[entry] = 1;
	}
	else
		joymemory[entry] = 0;

	return res;
}


int joystick_read_async(void)
{
	int i;
	const struct JoystickInfo *joyinfo;


	/* first of all, record joys which are NOT pressed */
	joyinfo = osd_get_joy_list();

	for (i = 0;joyinfo[i].name;i++)
	{
		if (!osd_is_joy_pressed(joyinfo[i].code))
			joymemory[i] = 0;
	}

	for (i = 0;joyinfo[i].name;i++)
	{
		if (osd_is_joy_pressed(joyinfo[i].code))
		{
			if (joymemory[i] == 0)
			{
				joymemory[i] = 1;

				if (joyinfo[i].standardcode != JOYCODE_OTHER) return joyinfo[i].standardcode;
				else return joyinfo[i].code;
			}
		}
	}

	return JOYCODE_NONE;
}


int input_ui_pressed(int code)
{
	return (keyboard_pressed_memory(input_port_type_key(code))
			|| joystick_pressed_memory(input_port_type_joy(code)));
}


int input_ui_pressed_repeat(int code,int speed)
{
	return (keyboard_pressed_memory_repeat(input_port_type_key(code),speed)
			|| joystick_pressed_memory_repeat(input_port_type_joy(code),speed));
}
