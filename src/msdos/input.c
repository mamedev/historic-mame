#include "driver.h"
#include <allegro.h>


void my_textout (char *buf);
void update_screen(void);

int use_mouse;
int joystick;

void msdos_init_input (void)
{
	int err;

	set_leds(0);    /* turn off all leds */

	if (joystick != JOY_TYPE_NONE)
	{
		/* Try to install Allegro's joystick handler */

		/* load calibration data (from mame.cfg) */
		/* if valid data was found, this also sets Allegro's joy_type */
		err=load_joystick_data(0);

		/* valid calibration? user has choosen another joystick type? */
		if (err || (joystick != joy_type))
		{
			if (install_joystick(joystick) != 0)
			{
				printf ("Joystick not found.\n");
				joystick = JOY_TYPE_NONE;
			}
		}
	}
	else
	{
		joystick = JOY_TYPE_NONE;
	}


	if (use_mouse && install_mouse() != -1)
		use_mouse = 1;
	else
		use_mouse = 0;
}


/* check if a key is pressed. The keycode is the standard PC keyboard code, as */
/* defined in osdepend.h. Return 0 if the key is not pressed, nonzero otherwise. */
int osd_key_pressed(int keycode)
{
	if (keycode == OSD_KEY_RCONTROL) keycode = KEY_RCONTROL;
	if (keycode == OSD_KEY_ALTGR) keycode = KEY_ALTGR;

	return key[keycode];
}



/* wait for a key press and return the keycode */
int osd_read_key(void)
{
	int k;


	/* wait for all keys to be released */
	do
	{
	      for (k = OSD_MAX_KEY;k >= 0;k--)
	      if (osd_key_pressed(k)) break;
	} while (k >= 0);

	/* wait for a key press */
	do
	{
	      for (k = OSD_MAX_KEY;k >= 0;k--)
	      if (osd_key_pressed(k)) break;
	} while (k < 0);

	return k;
}



/* Wait for a key press and return keycode.  Support repeat */
int osd_read_keyrepeat(void)
{
	int res;


	clear_keybuf();
	res = readkey() >> 8;

	if (res == KEY_RCONTROL) res = OSD_KEY_RCONTROL;
	if (res == KEY_ALTGR) res = OSD_KEY_ALTGR;

	return res;
}


int osd_debug_readkey(void)
{
	return osd_read_keyrepeat();
}


/* return the name of a key */
const char *osd_key_name(int keycode)
{
	static char *keynames[] =
	{
		"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"0", "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T",
		"Y", "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "LEFTCONTROL",     "A",
		"S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
		"TILDE", "LEFTSHIFT", "Error", "Z", "X", "C", "V", "B", "N", "M",
		"COMMA", ".", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK", "F1", "F2",
		"F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",     "SCRLOCK",
		"HOME", "UP", "PGUP", "MINUS PAD", "LEFT", "5 PAD", "RIGHT", "PLUS PAD", "END", "DOWN",
		"PGDN", "INS", "DEL", "RIGHTCONTROL", "ALTGR", "Error", "F11", "F12"
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

void joy_calibration(void)
{
	int i;
	char *msg;

	int need_save = 0; /* do we really need to save the data? */

	if (joystick == JOY_TYPE_NONE)
		return;

	/* reinitialises the joystick. */
	remove_joystick();
	install_joystick (joystick);

	for (i=0; i < num_joysticks; i++)
	{
		while (joy[i].flags & JOYFLAG_CALIBRATE)
		{
			need_save = 1; /* OK, we need to save this later */
			msg = calibrate_joystick_name(i);
			osd_clearbitmap(Machine->scrbitmap);
			my_textout(msg);
			update_screen();
			(void)osd_read_key();
			if (calibrate_joystick(i))
				continue; /* continue with next joystick on error */
		}
	}

	osd_clearbitmap(Machine->scrbitmap);
	if (need_save)
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

