#define __INLINE__ static __inline__	/* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"


void my_textout (char *buf, int x, int y);

int use_mouse;
int joystick;

void msdos_init_input (void)
{
	int err;

	install_keyboard();
	set_leds(0);    /* turn off all leds */

	if (joystick != JOY_TYPE_NONE)
	{
		/* Try to install Allegro's joystick handler */

		/* load calibration data (from mame.cfg) */
		/* if valid data was found, this also sets Allegro's joy_type */
		err=load_joystick_data(0);

		/* valid calibration? */
		if (err)
		{
			if (errorlog)
					fprintf (errorlog, "No calibration data found\n");
			if (install_joystick (joystick) != 0)
			{
				printf ("Joystick not found.\n");
				joystick = JOY_TYPE_NONE;
			}
		}
		else if (joystick != joy_type)
		{
			if (errorlog)
				fprintf (errorlog, "Calibration data is from different joystick\n");
			remove_joystick();
			if (install_joystick (joystick) != 0)
			{
				printf ("Joystick not found.\n");
				joystick = JOY_TYPE_NONE;
			}
		}

		if (errorlog)
		{
			if (joystick == JOY_TYPE_NONE)
				fprintf (errorlog, "Joystick not found\n");
			else
				fprintf (errorlog, "Installed %s %s\n",
						joystick_driver->name, joystick_driver->desc);
		}
	}

	if (use_mouse && install_mouse() != -1)
		use_mouse = 1;
	else
		use_mouse = 0;
}


void msdos_shutdown_input(void)
{
	remove_keyboard();
}


/*
 * Check if a key is pressed. The keycode is the standard PC keyboard
 * code, as defined in osdepend.h. Return 0 if the key is not pressed,
 * nonzero otherwise. Handle pseudo keycodes.
 */
int osd_key_pressed(int keycode)
{
	if (keycode > OSD_MAX_KEY)
	{
#ifdef MESS
        switch (keycode)
        {
			case OSD_KEY_CANCEL:
                return (key[OSD_KEY_NUMLOCK] & key[KEY_LCONTROL]);

            case OSD_KEY_SHOW_GFX:
                return key[OSD_KEY_F5];

			case OSD_KEY_JOY_CALIBRATE:
				return key[OSD_KEY_F7];

            case OSD_KEY_FRAMESKIP:
                return key[OSD_KEY_F8];

            case OSD_KEY_RESET_MACHINE:
				return key[OSD_KEY_F9];

			case OSD_KEY_THROTTLE:
                return key[OSD_KEY_F10];

            case OSD_KEY_SHOW_FPS:
                return key[OSD_KEY_F11] && !(key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]);

            case OSD_KEY_SHOW_PROFILE:
                return key[OSD_KEY_F11] && (key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]);

            case OSD_KEY_CONFIGURE:
                return key[OSD_KEY_F12];

            case OSD_KEY_VOLUME_DOWN:
				return (key[OSD_KEY_MINUS_PAD] && !key[OSD_KEY_LSHIFT]);

			case OSD_KEY_VOLUME_UP:
				return (key[OSD_KEY_PLUS_PAD] && !key[OSD_KEY_LSHIFT]);

			case OSD_KEY_GAMMA_DOWN:
				return (key[OSD_KEY_MINUS_PAD] && key[OSD_KEY_LSHIFT]);

            case OSD_KEY_GAMMA_UP:
                return (key[OSD_KEY_PLUS_PAD] && key[OSD_KEY_LSHIFT]);

            case OSD_KEY_PAUSE:
            case OSD_KEY_UNPAUSE:
				return key[OSD_KEY_SCRLOCK];

			case OSD_KEY_SNAPSHOT:
				return key[KEY_PRTSCR];
			default:
				return 0;
		}
#else
		switch (keycode)
        {
            case OSD_KEY_CANCEL:
				return key[OSD_KEY_ESC];

            case OSD_KEY_SHOW_GFX:
				return key[OSD_KEY_F4];

			case OSD_KEY_RESET_MACHINE:
                return key[OSD_KEY_F3];

            case OSD_KEY_CHEAT_TOGGLE:
				return key[OSD_KEY_F5];

            case OSD_KEY_JOY_CALIBRATE:
				return key[OSD_KEY_F7];

            case OSD_KEY_FRAMESKIP:
                return key[OSD_KEY_F8];

            case OSD_KEY_THROTTLE:
                return key[OSD_KEY_F10];

            case OSD_KEY_SHOW_FPS:
                return key[OSD_KEY_F11] && !(key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]);

            case OSD_KEY_SHOW_PROFILE:
                return key[OSD_KEY_F11] && (key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]);

            case OSD_KEY_CONFIGURE:
				return key[OSD_KEY_TAB];

            case OSD_KEY_ON_SCREEN_DISPLAY:
			{
				extern int mame_debug;
				return (key[OSD_KEY_TILDE] && !mame_debug);
			}

            case OSD_KEY_DEBUGGER:
			{
				extern int mame_debug;
				return (key[OSD_KEY_TILDE] && mame_debug);
			}

            case OSD_KEY_PAUSE:
            case OSD_KEY_UNPAUSE:
				return key[OSD_KEY_P];

			case OSD_KEY_SNAPSHOT:
				return key[KEY_F12];
			default:
				return 0;
        }
#endif
    }
	if (keycode == OSD_KEY_RCONTROL) keycode = KEY_RCONTROL;
	if (keycode == OSD_KEY_ALTGR) keycode = KEY_ALTGR;
	return key[keycode];
}

static char memory[256];

/* Report a key as pressed only when the user hits it, not while it is */
/* being kept pressed. */
int osd_key_pressed_memory(int keycode)
{
	int res = 0;

	if (keycode == OSD_KEY_UNPAUSE) keycode = OSD_KEY_PAUSE;	/* we use the same key */

	if (osd_key_pressed(keycode))
	{
		if (memory[keycode] == 0) res = 1;
		memory[keycode] = 1;
	}
	else
		memory[keycode] = 0;

	return res;
}

/* report kay as pulsing while it is pressed */
int osd_key_pressed_memory_repeat(int keycode,int speed)
{
	static int counter;
	int res = 0;

	if (osd_key_pressed(keycode))
	{
		if (memory[keycode] == 0 || ++counter > speed * Machine->drv->frames_per_second / 60)
		{
			counter = 0;
			res = 1;
		}
		memory[keycode] = 1;
	}
	else
		memory[keycode] = 0;

	return res;
}


/* If the user presses a key return it, otherwise return OSD_KEY_NONE. */
/* DO NOT wait for the user to press a key */
int osd_read_key_immediate(void)
{
	int res;


	/* first of all, record keys which are NOT pressed */
	for (res = OSD_MAX_KEY;res > OSD_KEY_NONE;res--)
	{
		if (!osd_key_pressed(res))
		{
			memory[res] = 0;
			memory[key_to_pseudo_code(res)] = 0;
		}
	}

	for (res = OSD_MAX_KEY;res > OSD_KEY_NONE;res--)
	{
		if (osd_key_pressed(res))
		{
			if (memory[res] == 0)
			{
				memory[res] = 1;
				memory[key_to_pseudo_code(res)] = 1;
			}
			else res = OSD_KEY_NONE;
			break;
		}
	}

	return res;
}



/*
 * Wait for a key press and return the keycode.
 * Translate certain keys (or key combinations) back to
 * the pseudo key values defined in osdepend.h.
 */
int osd_read_key(int translate)
{
	int res;

	if (translate)
	{
		/* wait for all keys to be released (including pseudo keys) */
		do
		{
			for (res = OSD_MAX_PSEUDO; res >= 0; res--)
				if (osd_key_pressed(res))
					break;
		} while (res >= 0);

		/* wait for a key press (including pseudo keys) */
		do
		{
			for (res = OSD_MAX_PSEUDO; res >= 0; res--)
				if (osd_key_pressed(res))
					break;
		} while (res < 0);
	}
	else
	{
		/* wait for all keys to be released (standard keys) */
		do
		{
			for (res = OSD_MAX_KEY; res >= 0; res--)
				if (osd_key_pressed(res))
					break;
		} while (res >= 0);

		/* wait for a key press (standard keys) */
		do
		{
			for (res = OSD_MAX_KEY; res >= 0; res--)
				if (osd_key_pressed(res))
					break;
        } while (res < 0);
    }

    if (res == KEY_RCONTROL) res = OSD_KEY_RCONTROL;
	if (res == KEY_ALTGR) res = OSD_KEY_ALTGR;
	return res;
}

/* translate a scancode to a pseudo key code */
int key_to_pseudo_code(int k)
{
#ifdef MESS
    switch (k)
    {
        case OSD_KEY_NUMLOCK:
            if (key[OSD_KEY_LCONTROL])
                return OSD_KEY_CANCEL;
            break;

        case OSD_KEY_F5:
            return OSD_KEY_SHOW_GFX;

		case OSD_KEY_F7:
			return OSD_KEY_JOY_CALIBRATE;

        case OSD_KEY_F8:
            return OSD_KEY_FRAMESKIP;

        case OSD_KEY_F9:
            return OSD_KEY_RESET_MACHINE;

        case OSD_KEY_F10:
            return OSD_KEY_THROTTLE;

        case OSD_KEY_F11:
            if ((key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]))
                return OSD_KEY_SHOW_PROFILE;
            return OSD_KEY_SHOW_FPS;

        case OSD_KEY_F12:
            return OSD_KEY_CONFIGURE;

        case OSD_KEY_MINUS_PAD:
            if (key[OSD_KEY_LSHIFT])
                return OSD_KEY_GAMMA_DOWN;
            return OSD_KEY_VOLUME_DOWN;

        case OSD_KEY_PLUS_PAD:
            if (key[OSD_KEY_LSHIFT])
                return OSD_KEY_GAMMA_UP;
            return OSD_KEY_VOLUME_UP;

        case OSD_KEY_SCRLOCK:
            return OSD_KEY_PAUSE;
    }
#else
    switch (k)
    {
        case OSD_KEY_ESC:
            return OSD_KEY_CANCEL;

        case OSD_KEY_F4:
            return OSD_KEY_SHOW_GFX;

		case OSD_KEY_F5:
			return OSD_KEY_CHEAT_TOGGLE;

		case OSD_KEY_F7:
			return OSD_KEY_JOY_CALIBRATE;

        case OSD_KEY_F8:
            return OSD_KEY_FRAMESKIP;

        case OSD_KEY_F3:
            return OSD_KEY_RESET_MACHINE;

        case OSD_KEY_F10:
            return OSD_KEY_THROTTLE;

        case OSD_KEY_F11:
            if ((key[OSD_KEY_LSHIFT] || key[OSD_KEY_RSHIFT]))
                return OSD_KEY_SHOW_PROFILE;
            return OSD_KEY_SHOW_FPS;

        case OSD_KEY_TAB:
            return OSD_KEY_CONFIGURE;

        case OSD_KEY_TILDE:
		{
			extern int mame_debug;

			if (mame_debug) return OSD_KEY_DEBUGGER;
			else return OSD_KEY_ON_SCREEN_DISPLAY;
		}

        case OSD_KEY_P:
            return OSD_KEY_PAUSE;
    }
#endif
    return k;
}

/* Wait for a key press and return keycode.  Support repeat */
int osd_read_keyrepeat(int translate)
{
	int res;

	clear_keybuf();
	res = readkey() >> 8;

	if (res == KEY_RCONTROL) res = OSD_KEY_RCONTROL;
	if (res == KEY_ALTGR) res = OSD_KEY_ALTGR;

	if (translate)
		res = key_to_pseudo_code(res);
	return res;
}

int osd_debug_readkey(void)
{
	int res;

	clear_keybuf();
	res = readkey() >> 8;

	if (res == KEY_RCONTROL) res = OSD_KEY_RCONTROL;
	if (res == KEY_ALTGR) res = OSD_KEY_ALTGR;

	return res;
}


/* return the name of a key */
const char *osd_key_name(int keycode)
{
	static char *keynames[] =
	{
		"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "MINUS", "EQUAL", "BKSPACE",
		"TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "OPBRACE", "CLBRACE", "ENTER",
		"LCTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE", "TILDE",
		"LSHIFT", "Error", "Z", "X", "C", "V", "B", "N", "M", "COMMA", ".", "SLASH", "RSHIFT",
		"*", "ALT", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
		"NUMLOCK", "SCRLOCK", "HOME", "UP", "PGUP", "MINUS PAD",
		"LEFT", "5 PAD", "RIGHT", "PLUS PAD", "END", "DOWN",
		"PGDN", "INS", "DEL", "RCTRL", "ALTGR", "Error",
		"F11", "F12", "Error", "Error",
		"Error", "Error", "Error", "Error", "Error",
		"Error", "Error", "Error", "Error", "Error",
		"1 PAD", "2 PAD", "3 PAD", "4 PAD", "Error",
		"6 PAD", "7 PAD", "8 PAD", "9 PAD", "0 PAD",
		". PAD", "= PAD", "/ PAD", "* PAD", "ENTER PAD",
		"Error", "Error", "Error", "Error", "Error",
        "Error", "Error", "PAUSE",
    };
	static char *nonedefined = "None";

	if (keycode && keycode <= OSD_MAX_KEY) return keynames[keycode-1];
	else return (char *)nonedefined;
}


/* return the name of a joystick button */
const char *osd_joy_name(int joycode)
{
	static char *joynames[] = {
		"Left", "Right", "Up", "Down", "Button A",
		"Button B", "Button C", "Button D", "Button E", "Button F",
		"Button G", "Button H", "Button I", "Button J", "Any Button",
		"J2 Left", "J2 Right", "J2 Up", "J2 Down", "J2 Button A",
		"J2 Button B", "J2 Button C", "J2 Button D", "J2 Button E", "J2 Button F",
		"J2 Button G", "J2 Button H", "J2 Button I", "J2 Button J", "J2 Any Button",
		"J3 Left", "J3 Right", "J3 Up", "J3 Down", "J3 Button A",
		"J3 Button B", "J3 Button C", "J3 Button D", "J3 Button E", "J3 Button F",
		"J3 Button G", "J3 Button H", "J3 Button I", "J3 Button J", "J3 Any Button",
		"J4 Left", "J4 Right", "J4 Up", "J4 Down", "J4 Button A",
		"J4 Button B", "J4 Button C", "J4 Button D", "J4 Button E", "J4 Button F",
		"J4 Button G", "J4 Button H", "J4 Button I", "J4 Button J", "J4 Any Button"
	};

	if (joycode == 0) return "None";
	else if (joycode <= OSD_MAX_JOY) return (char *)joynames[joycode-1];
	else return "Unknown";
}


void osd_poll_joystick(void)
{
	if (joystick > JOY_TYPE_NONE)
		poll_joystick();
}


/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
	int joy_num;

	/* which joystick are we polling? */
	if (joycode < OSD_JOY_LEFT)
		return 0;
	else if (joycode < OSD_JOY2_LEFT)
	{
		joy_num = 0;
	}
	else if (joycode < OSD_JOY3_LEFT)
	{
		joy_num = 1;
		joycode = joycode - OSD_JOY2_LEFT + OSD_JOY_LEFT;
	}
	else if (joycode < OSD_JOY4_LEFT)
	{
		joy_num = 2;
		joycode = joycode - OSD_JOY3_LEFT + OSD_JOY_LEFT;
	}
	else if (joycode < OSD_MAX_JOY)
	{
		joy_num = 3;
		joycode = joycode - OSD_JOY4_LEFT + OSD_JOY_LEFT;
	}
	else
		return 0;

	if (joy_num == 0)
	{
		/* special case for mouse buttons */
		switch (joycode)
		{
			case OSD_JOY_FIRE1:
				if (mouse_b & 1) return 1; break;
			case OSD_JOY_FIRE2:
				if (mouse_b & 2) return 1; break;
			case OSD_JOY_FIRE3:
				if (mouse_b & 4) return 1; break;
			case OSD_JOY_FIRE: /* any mouse button */
				if (mouse_b) return 1; break;
		}
	}

	/* do we have as many sticks? */
	if (joy_num+1 > num_joysticks)
		return 0;

	switch (joycode)
	{
		case OSD_JOY_LEFT:
			return joy[joy_num].stick[0].axis[0].d1;
			break;
		case OSD_JOY_RIGHT:
			return joy[joy_num].stick[0].axis[0].d2;
			break;
		case OSD_JOY_UP:
			return joy[joy_num].stick[0].axis[1].d1;
			break;
		case OSD_JOY_DOWN:
			return joy[joy_num].stick[0].axis[1].d2;
			break;
		case OSD_JOY_FIRE1:
			return joy[joy_num].button[0].b;
			break;
		case OSD_JOY_FIRE2:
			return joy[joy_num].button[1].b;
			break;
		case OSD_JOY_FIRE3:
			return joy[joy_num].button[2].b;
			break;
		case OSD_JOY_FIRE4:
			return joy[joy_num].button[3].b;
			break;
		case OSD_JOY_FIRE5:
			return joy[joy_num].button[4].b;
			break;
		case OSD_JOY_FIRE6:
			return joy[joy_num].button[5].b;
			break;
		case OSD_JOY_FIRE7:
			return joy[joy_num].button[6].b;
			break;
		case OSD_JOY_FIRE8:
			return joy[joy_num].button[7].b;
			break;
		case OSD_JOY_FIRE9:
			return joy[joy_num].button[8].b;
			break;
		case OSD_JOY_FIRE10:
			return joy[joy_num].button[9].b;
			break;
		case OSD_JOY_FIRE:
			{
				int i;
				for (i = 0; i < 10; i++)
					if (joy[joy_num].button[i].b)
						return 1;
			}
			break;
	}
	return 0;
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int *analog_x, int *analog_y)
{
	*analog_x = *analog_y = 0;

	/* is there an analog joystick at all? */
	if (joystick == JOY_TYPE_NONE)
		return;

	*analog_x = joy[0].stick[0].axis[0].pos;
	*analog_y = joy[0].stick[0].axis[1].pos;
}


static int calibration_target;

int osd_joystick_needs_calibration (void)
{
	/* This could be improved, but unfortunately, this version of Allgegro */
	/* lacks a flag which tells if a joystick is calibrationable, it only  */
	/* remembers wether calibration is yet to be done. */
	if (joystick == JOY_TYPE_NONE)
		return 0;
	else
		return 1;
}


void osd_joystick_start_calibration (void)
{
	/* reinitialises the joystick. */
	remove_joystick();
	install_joystick (joystick);
	calibration_target = 0;
}

char *osd_joystick_calibrate_next (void)
{
	while (calibration_target < num_joysticks)
	{
		if (joy[calibration_target].flags & JOYFLAG_CALIBRATE)
			return (calibrate_joystick_name (calibration_target));
		else
			calibration_target++;
	}

	return 0;
}

void osd_joystick_calibrate (void)
{
	calibrate_joystick (calibration_target);
}

void osd_joystick_end_calibration (void)
{
	save_joystick_data(0);
}


void osd_trak_read(int *deltax,int *deltay)
{
	if (use_mouse == 0) *deltax = *deltay = 0;
	else get_mouse_mickeys(deltax,deltay);
}




static int leds=0;
static const int led_flags[3] = {
  KB_NUMLOCK_FLAG,
  KB_CAPSLOCK_FLAG,
  KB_SCROLOCK_FLAG
};
void osd_led_w(int led,int on) {
  int temp=leds;
  if (led<3) {
    if (on&1)
	temp |=  led_flags[led];
    else
	temp &= ~led_flags[led];
    if (temp!=leds) {
	leds=temp;
	set_leds (leds);
    }
  }
}

