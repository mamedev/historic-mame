/******************************************************************************

  input.c

  Handle input from the user - keyboard, joystick, etc.

******************************************************************************/

#include "driver.h"

#include <time.h>

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

void keyboard_name_multi(UINT16* keycode, char* buffer, unsigned max) {
	int j;
	char* dest = buffer;
	for(j=0;j<INPUT_KEY_CODE_MAX;++j) {
		const char* name;

		if (keycode[j]==IP_KEY_NONE)
			break;

		name = keyboard_name(keycode[j]);
		if (!name)
			break;

		if (dest != buffer && 1 + 1 <= max) {
			*dest++ = ' ';
			--max;
		}

		if (strlen(name) + 1 <= max) {
			strcpy(dest,name);
			dest += strlen(name);
			max -= strlen(name);
		}
	}

	if (dest == buffer && 4 + 1 <= max ) {
		strcpy(dest,"None");
	} else {
		*dest = 0;
	}
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
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_key_code(&keycode,&entry) == 0)
		res = osd_is_key_pressed(keycode);
profiler_mark(PROFILER_END);

	return res;
}

int keyboard_pressed_multi(UINT16* keycode)
{
	int j;
	if (!keyboard_pressed(keycode[0]))
		return 0;
	for(j=1;j<INPUT_KEY_CODE_MAX;++j) {
		if (keycode[j]==IP_KEY_NONE)
			return 1;
		if (!keyboard_pressed(keycode[j]))
			return 0;
	}
	return 1;
}

static char keymemory[MAX_KEYS];

int keyboard_pressed_memory(int keycode)
{
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_key_code(&keycode,&entry) == 0)
	{
		if (osd_is_key_pressed(keycode))
		{
			if (keymemory[entry] == 0) res = 1;
			keymemory[entry] = 1;
		}
		else
			keymemory[entry] = 0;
	}
profiler_mark(PROFILER_END);

	return res;
}

int keyboard_pressed_memory_multi(UINT16* keycode)
{
	int j;
	if (!keyboard_pressed_memory(keycode[0]))
		return 0;
	for(j=1;j<INPUT_KEY_CODE_MAX;++j) {
		if (keycode[j]==IP_KEY_NONE)
			return 1;
		if (!keyboard_pressed_memory(keycode[j]))
			return 0;
	}
	return 1;
}

int keyboard_pressed_memory_repeat(int keycode,int speed)
{
	static int counter,keydelay;
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_key_code(&keycode,&entry) == 0)
	{
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
	}
profiler_mark(PROFILER_END);

	return res;
}

int keyboard_pressed_memory_repeat_multi(UINT16* keycode,int speed)
{
	int j;
	if (!keyboard_pressed_memory_repeat(keycode[0],speed))
		return 0;
	for(j=1;j<INPUT_KEY_CODE_MAX;++j) {
		if (keycode[j]==IP_KEY_NONE)
			return 1;
		if (!keyboard_pressed_memory_repeat(keycode[j],speed))
			return 0;
	}
	return 1;
}

int keyboard_read_async(void)
{
	int res;
	int i;
	const struct KeyboardInfo *keyinfo;


profiler_mark(PROFILER_INPUT);
	/* first of all, record keys which are NOT pressed */
	keyinfo = osd_get_key_list();

	for (i = 0;keyinfo[i].name;i++)
	{
		if (!osd_is_key_pressed(keyinfo[i].code))
			keymemory[i] = 0;
	}

	res = KEYCODE_NONE;
	for (i = 0;keyinfo[i].name;i++)
	{
		if (osd_is_key_pressed(keyinfo[i].code))
		{
			if (keymemory[i] == 0)
			{
				keymemory[i] = 1;

				if (keyinfo[i].standardcode != KEYCODE_OTHER) res = keyinfo[i].standardcode;
				else res = keyinfo[i].code;

				break;
			}
		}
	}
profiler_mark(PROFILER_END);

	return res;
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



static const char *joystick_name(int joycode)
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

void joystick_name_multi(UINT16* joycode, char* buffer, unsigned max) {
	int j;
	char* dest = buffer;
	for(j=0;j<INPUT_KEY_CODE_MAX;++j) {
		const char* name;

		if (joycode[j]==IP_JOY_NONE)
			break;

		name = joystick_name(joycode[j]);
		if (!name)
			break;

		if (dest != buffer && 1 + 1 <= max) {
			*dest++ = ' ';
			--max;
		}

		if (strlen(name) + 1 <= max) {
			strcpy(dest,name);
			dest += strlen(name);
			max -= strlen(name);
		}
	}

	if (dest == buffer && 4 + 1 <= max ) {
		strcpy(dest,"None");
	} else {
		*dest = 0;
	}
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
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_joy_code(&joycode,&entry) == 0)
		res = osd_is_joy_pressed(joycode);
profiler_mark(PROFILER_END);

	return res;
}

int joystick_pressed_multi(UINT16* joycode)
{
	int j;
	if (!joystick_pressed(joycode[0]))
		return 0;
	for(j=1;j<INPUT_JOY_CODE_MAX;++j) {
		if (joycode[j]==IP_JOY_NONE)
			return 1;
		if (!joystick_pressed(joycode[j]))
			return 0;
	}
	return 1;
}

static char joymemory[MAX_JOYS];

int joystick_pressed_memory(int joycode)
{
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_joy_code(&joycode,&entry) == 0)
	{
		if (osd_is_joy_pressed(joycode))
		{
			if (joymemory[entry] == 0) res = 1;
			joymemory[entry] = 1;
		}
		else
			joymemory[entry] = 0;
	}
profiler_mark(PROFILER_END);

	return res;
}

int joystick_pressed_memory_multi(UINT16* joycode)
{
	int j;
	if (!joystick_pressed_memory(joycode[0]))
		return 0;
	for(j=1;j<INPUT_JOY_CODE_MAX;++j) {
		if (joycode[j]==IP_JOY_NONE)
			return 1;
		if (!joystick_pressed_memory(joycode[j]))
			return 0;
	}
	return 1;
}

int joystick_pressed_memory_repeat(int joycode,int speed)
{
	static int counter,joydelay;
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_joy_code(&joycode,&entry) == 0)
	{
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
	}
profiler_mark(PROFILER_END);

	return res;
}

int joystick_pressed_memory_repeat_multi(UINT16* joycode, int speed)
{
	int j;
	if (!joystick_pressed_memory_repeat(joycode[0],speed))
		return 0;
	for(j=1;j<INPUT_JOY_CODE_MAX;++j) {
		if (joycode[j]==IP_JOY_NONE)
			return 1;
		if (!joystick_pressed_memory_repeat(joycode[j],speed))
			return 0;
	}
	return 1;
}

int joystick_read_async(void)
{
	int res;
	int i;
	const struct JoystickInfo *joyinfo;


profiler_mark(PROFILER_INPUT);
	/* first of all, record joys which are NOT pressed */
	joyinfo = osd_get_joy_list();

	for (i = 0;joyinfo[i].name;i++)
	{
		if (!osd_is_joy_pressed(joyinfo[i].code))
			joymemory[i] = 0;
	}

	res = JOYCODE_NONE;
	for (i = 0;joyinfo[i].name;i++)
	{
		if (osd_is_joy_pressed(joyinfo[i].code))
		{
			if (joymemory[i] == 0)
			{
				joymemory[i] = 1;

				if (joyinfo[i].standardcode != JOYCODE_OTHER) res = joyinfo[i].standardcode;
				else res = joyinfo[i].code;

				break;
			}
		}
	}
profiler_mark(PROFILER_END);

	return res;
}


int input_ui_pressed(int code)
{
	return (keyboard_pressed_memory_multi(input_port_type_key_multi(code))
			|| joystick_pressed_memory_multi(input_port_type_joy_multi(code)));
}


int input_ui_pressed_repeat(int code,int speed)
{
	return (keyboard_pressed_memory_repeat_multi(input_port_type_key_multi(code),speed)
			|| joystick_pressed_memory_repeat_multi(input_port_type_joy_multi(code),speed));
}


/* Static Information used in key/joy recording */
static UINT16 record_keycode[INPUT_KEY_CODE_MAX]; /* buffer for key recording */
static UINT16 record_joycode[INPUT_KEY_CODE_MAX]; /* buffer for joy recording */
static unsigned record_count; /* number of key/joy press recorded */
static clock_t record_last; /* time of last key/joy press */

#define RECORD_TIME (CLOCKS_PER_SEC/2) /* max time between key press */

/* Start a key/joy recording */
void record_start(void) {
	record_count = 0;
	record_last = clock();
}

/* Record a keystroke
	return <0 if no key pressed
	return ==0 if key recorded
	return >0 if aborted
*/
int keyboard_record(UINT16* keycode) {
	int newkey;

	if (keyboard_pressed_memory_multi(input_port_type_key_multi(IPT_UI_CANCEL)))
		return 1;

	if (record_count > 0 && clock() > record_last + RECORD_TIME)
	{
		/* fill to end */
		while (record_count < INPUT_KEY_CODE_MAX) {
			record_keycode[record_count++] = IP_KEY_NONE;
		}

		INPUT_KEY_CODE_COPY(keycode,record_keycode);
		return 0;
	}

	newkey = keyboard_read_async();
	if (newkey != KEYCODE_NONE)
	{
		int j;

		/* don't insert duplicate */
		for(j=0;j<record_count && j<INPUT_KEY_CODE_MAX;++j)
			if (record_keycode[j] == newkey)
				return -1;

		record_keycode[record_count++] = newkey;
		record_last = clock();

		if (record_count == INPUT_KEY_CODE_MAX)
		{
			INPUT_KEY_CODE_COPY(keycode,record_keycode);
			return 0;
		}
	}

	return -1;
}

int joystick_record(UINT16* joycode) {
	int newjoy;

	if (keyboard_pressed_memory_multi(input_port_type_key_multi(IPT_UI_CANCEL)))
		return 1;

	if (keyboard_pressed_memory(KEYCODE_N))
	{
		INPUT_JOY_CODE_SET_1(joycode, JOYCODE_NONE);
		return 0;
	}

	if (record_count > 0 && clock() > record_last + RECORD_TIME)
	{
		/* fill to end */
		while (record_count < INPUT_JOY_CODE_MAX) {
			record_joycode[record_count++] = IP_JOY_NONE;
		}

		INPUT_JOY_CODE_COPY(joycode,record_joycode);
		return 0;
	}

	newjoy = joystick_read_async();
	if (newjoy != JOYCODE_NONE)
	{
		int j;

		/* don't insert duplicate */
		for(j=0;j<record_count && j<INPUT_JOY_CODE_MAX;++j)
			if (record_joycode[j] == newjoy)
				return -1;

		record_joycode[record_count++] = newjoy;
		record_last = clock();

		if (record_count == INPUT_JOY_CODE_MAX)
		{
			INPUT_JOY_CODE_COPY(joycode,record_joycode);
			return 0;
		}
	}

	return -1;
}

