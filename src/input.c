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
	if (keycode == IP_CODE_NOT) return "not";
	if (keycode == IP_CODE_OR) return "or";

	keyinfo = osd_get_key_list();

	while (keyinfo->name)
	{
		if (keyinfo->code == keycode || keyinfo->standardcode == keycode)
			return keyinfo->name;

		keyinfo++;
	}

	return "n/a";
}

void keyboard_name_multi(InputKeySeq* keycode, char* buffer, unsigned max)
{
	int j;
	char* dest = buffer;
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
	{
		const char* name;

		if ((*keycode)[j]==IP_KEY_NONE)
			break;

		if (j && 1 + 1 <= max)
		{
			*dest = ' ';
			dest += 1;
			max -= 1;
		}

		name = keyboard_name((*keycode)[j]);
		if (!name)
			break;

		if (strlen(name) + 1 <= max)
		{
			strcpy(dest,name);
			dest += strlen(name);
			max -= strlen(name);
		}
	}

	if (dest == buffer && 4 + 1 <= max)
		strcpy(dest,"None");
	else
		*dest = 0;
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
		res = osd_is_key_pressed(keycode) != 0;
profiler_mark(PROFILER_END);

	return res;
}

int keyboard_pressed_multi(InputKeySeq* keycode)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
	{
		switch ((*keycode)[j])
		{
			case IP_KEY_NONE :
				return res && count;
			case IP_CODE_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case IP_CODE_NOT :
                               	invert = !invert;
				break;
			default:
				if (res && keyboard_pressed((*keycode)[j]) == invert)
					res = 0;
				invert = 0;
				++count;
		}
	}
	return res && count;
}

static int keymemory[MAX_KEYS];

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



const char *joystick_name(int joycode)
{
	const struct JoystickInfo *joyinfo;


	if (joycode == JOYCODE_NONE) return "None";
	if (joycode == IP_CODE_NOT) return "not";
	if (joycode == IP_CODE_OR) return "or";

	joyinfo = osd_get_joy_list();

	while (joyinfo->name)
	{
		if (joyinfo->code == joycode || joyinfo->standardcode == joycode)
			return joyinfo->name;

		joyinfo++;
	}

	return "n/a";
}

void joystick_name_multi(InputJoySeq* joycode, char* buffer, unsigned max)
{
	int j;
	char* dest = buffer;
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
	{
		const char* name;

		if ((*joycode)[j]==IP_JOY_NONE)
			break;

		if (j && 1 + 1 <= max)
		{
			*dest = ' ';
			dest += 1;
			max -= 1;
		}

		name = joystick_name((*joycode)[j]);
		if (!name)
			break;

		if (strlen(name) + 1 <= max)
		{
			strcpy(dest,name);
			dest += strlen(name);
			max -= strlen(name);
		}
	}

	if (dest == buffer && 4 + 1 <= max)
		strcpy(dest,"None");
	else
		*dest = 0;
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


static int joystick_pressed(int joycode)
{
	int res = 0;
	int entry;

profiler_mark(PROFILER_INPUT);
	if (pseudo_to_joy_code(&joycode,&entry) == 0)
		res = osd_is_joy_pressed(joycode) != 0;
profiler_mark(PROFILER_END);

	return res;
}

int joystick_pressed_multi(InputJoySeq* joycode)
{
	int j;
	int res = 1;
	int count = 0;
	int invert = 0;

	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
	{
		switch ((*joycode)[j])
		{
			case IP_JOY_NONE :
				return res && count;
			case IP_CODE_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case IP_CODE_NOT :
                               	invert = 1;
				break;
			default:
				if (res && joystick_pressed((*joycode)[j]) == invert)
					res = 0;
				invert = 0;
				++count;
		}
	}
	return res && count;
}


static char joymemory[MAX_JOYS];

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

/* Static buffer for memory input */
/* Works like keymemory and joymemory but at higher level */
#define MAX_INPUTS 256
static int inputmemory[MAX_INPUTS];

int input_ui_pressed(int code)
{
	int res = 0;

	/* assert( code < MAX_INPUTS ); */

profiler_mark(PROFILER_INPUT);
	if (keyboard_pressed_multi(input_port_type_key_multi(code)) || joystick_pressed_multi(input_port_type_joy_multi(code)))
	{
		if (inputmemory[code] == 0)
			res = 1;
		inputmemory[code] = 1;
	}
	else
		inputmemory[code] = 0;
profiler_mark(PROFILER_END);

	return res;
}

int input_ui_pressed_repeat(int code,int speed)
{
	static int counter,inputdelay;
	int res = 0;

	/* assert( code < MAX_INPUTS ); */

profiler_mark(PROFILER_INPUT);
	if (keyboard_pressed_multi(input_port_type_key_multi(code)) || joystick_pressed_multi(input_port_type_joy_multi(code)))
	{
		if (inputmemory[code] == 0)
		{
			inputdelay = 3;
			counter = 0;
			res = 1;
		}
		else if (++counter > inputdelay * speed * Machine->drv->frames_per_second / 60)
		{
			inputdelay = 1;
			counter = 0;
			res = 1;
		}
		inputmemory[code] = 1;
	}
	else
		inputmemory[code] = 0;
profiler_mark(PROFILER_END);

	return res;
}

/* Static informations used in key/joy recording */
static InputCode record_keycode[INPUT_KEY_SEQ_MAX]; /* buffer for key recording */
static InputCode record_joycode[INPUT_JOY_SEQ_MAX]; /* buffer for joy recording */
static int record_count; /* number of key/joy press recorded */
static clock_t record_last; /* time of last key/joy press */

#define RECORD_TIME (CLOCKS_PER_SEC*2/3) /* max time between key press */

/* Start a key/joy recording */
void record_start(void)
{
	int i;

	record_count = 0;
	record_last = clock();

	/* set key/joy memory, otherwise key/joy memory interferes with input memory */
	for(i=0;i<MAX_KEYS;++i)
		keymemory[i] = 1;
	for(i=0;i<MAX_JOYS;++i)
		joymemory[i] = 1;
}

/* Check that almost one key/joy must be pressed */
static int input_key_code_valid(InputKeySeq* keycode)
{
	int j;
	int positive = 0;
	int pred_not = 0;
	int operand = 0;
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
	{
		switch ((*keycode)[j])
		{
			case IP_KEY_NONE :
				break;
			case IP_CODE_OR :
				if (!operand || !positive)
					return 0;
				pred_not = 0;
				positive = 0;
				operand = 0;
				break;
			case IP_CODE_NOT :
				if (pred_not)
					return 0;
				pred_not = !pred_not;
				operand = 0;
				break;
			default:
				if (!pred_not)
					positive = 1;
				pred_not = 0;
				operand = 1;
				break;
		}
	}
	return positive && operand;
}

static int input_joy_code_valid(InputJoySeq* joycode)
{
	int j;
	int positive = 0;
	int pred_not = 0;
	int operand = 0;
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
	{
		switch ((*joycode)[j])
		{
			case IP_JOY_NONE :
				break;
			case IP_CODE_OR :
				if (!operand || !positive)
					return 0;
				pred_not = 0;
				positive = 0;
				operand = 0;
				break;
			case IP_CODE_NOT :
				if (pred_not)
					return 0;
				pred_not = !pred_not;
				operand = 0;
				break;
			default:
				if (!pred_not)
					positive = 1;
				pred_not = 0;
				operand = 1;
				break;
		}
	}
	return positive && operand;
}


/* Record a key/joy sequence
	return <0 if more input is needed
	return ==0 if sequence succesfully recorded
	return >0 if aborted
*/
int keyboard_record(InputKeySeq* keycode, int first)
{
	int newkey;

	if (input_ui_pressed(IPT_UI_CANCEL))
		return 1;

	if (record_count == INPUT_KEY_SEQ_MAX || (record_count > 0 && clock() > record_last + RECORD_TIME))
	{
		int k = 0;
		if (!first)
		{
			/* search the first space free */
			while (k < INPUT_KEY_SEQ_MAX && (*keycode)[k] != IP_KEY_NONE)
				++k;
		}

		/* if no space restart */
		if (k + record_count + (k!=0) > INPUT_KEY_SEQ_MAX)
			k = 0;

		/* insert */
		if (k + record_count + (k!=0) <= INPUT_KEY_SEQ_MAX)
		{
			int j;
			if (k!=0)
				(*keycode)[k++] = IP_CODE_OR;
			for(j=0;j<record_count;++j,++k)
				(*keycode)[k] = record_keycode[j];
		}
		/* fill to end */
		while (k < INPUT_KEY_SEQ_MAX)
		{
			(*keycode)[k] = IP_KEY_NONE;
			++k;
		}

		if (!input_key_code_valid(keycode))
			input_key_seq_set_1(keycode,IP_KEY_NONE);

		return 0;
	}

	newkey = keyboard_read_async();
	if (newkey != KEYCODE_NONE)
	{
		/* if code is duplicate negate the code */
		if (record_count && newkey == record_keycode[record_count-1])
			record_keycode[record_count-1] = IP_CODE_NOT;

		record_keycode[record_count++] = newkey;
		record_last = clock();
	}

	return -1;
}

int joystick_record(InputJoySeq* joycode, int first)
{
	int newjoy;

	if (input_ui_pressed(IPT_UI_CANCEL))
		return 1;

	if (keyboard_pressed_memory(KEYCODE_N))
	{
		input_joy_seq_set_1(joycode, IP_JOY_NONE);
		return 0;
	}

	if (record_count == INPUT_JOY_SEQ_MAX || (record_count > 0 && clock() > record_last + RECORD_TIME))
	{
		int k = 0;
		if (!first)
		{
			/* search the first space free */
			while (k < INPUT_JOY_SEQ_MAX && (*joycode)[k] != IP_JOY_NONE)
				++k;
		}

		/* if no space restart */
		if (k + record_count + (k!=0) > INPUT_JOY_SEQ_MAX)
			k = 0;

		/* insert */
		if (k + record_count + (k!=0) <= INPUT_JOY_SEQ_MAX)
		{
			int j;
			if (k!=0)
				(*joycode)[k++] = IP_CODE_OR;
			for(j=0;j<record_count;++j,++k)
				(*joycode)[k] = record_joycode[j];
		}
		/* fill to end */
		while (k < INPUT_JOY_SEQ_MAX)
		{
			(*joycode)[k] = IP_JOY_NONE;
			++k;
		}

		if (!input_joy_code_valid(joycode))
			input_joy_seq_set_1(joycode,IP_JOY_NONE);

		return 0;
	}

	newjoy = joystick_read_async();
	if (newjoy != JOYCODE_NONE)
	{
		/* if code is duplicate negate the code */
		if (record_count && newjoy == record_joycode[record_count-1])
			record_joycode[record_count-1] = IP_CODE_NOT;

		record_joycode[record_count++] = newjoy;
		record_last = clock();
	}

	return -1;
}

