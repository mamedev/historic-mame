#include "mamalleg.h"
#include "driver.h"


int use_mouse;
int joystick;
int use_hotrod;


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


struct KeyboardKey keylist[] =
{
	{ "A",			KEY_A,				KEYCODE_A },
	{ "B",			KEY_B,				KEYCODE_B },
	{ "C",			KEY_C,				KEYCODE_C },
	{ "D",			KEY_D,				KEYCODE_D },
	{ "E",			KEY_E,				KEYCODE_E },
	{ "F",			KEY_F,				KEYCODE_F },
	{ "G",			KEY_G,				KEYCODE_G },
	{ "H",			KEY_H,				KEYCODE_H },
	{ "I",			KEY_I,				KEYCODE_I },
	{ "J",			KEY_J,				KEYCODE_J },
	{ "K",			KEY_K,				KEYCODE_K },
	{ "L",			KEY_L,				KEYCODE_L },
	{ "M",			KEY_M,				KEYCODE_M },
	{ "N",			KEY_N,				KEYCODE_N },
	{ "O",			KEY_O,				KEYCODE_O },
	{ "P",			KEY_P,				KEYCODE_P },
	{ "Q",			KEY_Q,				KEYCODE_Q },
	{ "R",			KEY_R,				KEYCODE_R },
	{ "S",			KEY_S,				KEYCODE_S },
	{ "T",			KEY_T,				KEYCODE_T },
	{ "U",			KEY_U,				KEYCODE_U },
	{ "V",			KEY_V,				KEYCODE_V },
	{ "W",			KEY_W,				KEYCODE_W },
	{ "X",			KEY_X,				KEYCODE_X },
	{ "Y",			KEY_Y,				KEYCODE_Y },
	{ "Z",			KEY_Z,				KEYCODE_Z },
	{ "0",			KEY_0,				KEYCODE_0 },
	{ "1",			KEY_1,				KEYCODE_1 },
	{ "2",			KEY_2,				KEYCODE_2 },
	{ "3",			KEY_3,				KEYCODE_3 },
	{ "4",			KEY_4,				KEYCODE_4 },
	{ "5",			KEY_5,				KEYCODE_5 },
	{ "6",			KEY_6,				KEYCODE_6 },
	{ "7",			KEY_7,				KEYCODE_7 },
	{ "8",			KEY_8,				KEYCODE_8 },
	{ "9",			KEY_9,				KEYCODE_9 },
	{ "0 PAD",		KEY_0_PAD,			KEYCODE_0_PAD },
	{ "1 PAD",		KEY_1_PAD,			KEYCODE_1_PAD },
	{ "2 PAD",		KEY_2_PAD,			KEYCODE_2_PAD },
	{ "3 PAD",		KEY_3_PAD,			KEYCODE_3_PAD },
	{ "4 PAD",		KEY_4_PAD,			KEYCODE_4_PAD },
	{ "5 PAD",		KEY_5_PAD,			KEYCODE_5_PAD },
	{ "6 PAD",		KEY_6_PAD,			KEYCODE_6_PAD },
	{ "7 PAD",		KEY_7_PAD,			KEYCODE_7_PAD },
	{ "8 PAD",		KEY_8_PAD,			KEYCODE_8_PAD },
	{ "9 PAD",		KEY_9_PAD,			KEYCODE_9_PAD },
	{ "F1",			KEY_F1,				KEYCODE_F1 },
	{ "F2",			KEY_F2,				KEYCODE_F2 },
	{ "F3",			KEY_F3,				KEYCODE_F3 },
	{ "F4",			KEY_F4,				KEYCODE_F4 },
	{ "F5",			KEY_F5,				KEYCODE_F5 },
	{ "F6",			KEY_F6,				KEYCODE_F6 },
	{ "F7",			KEY_F7,				KEYCODE_F7 },
	{ "F8",			KEY_F8,				KEYCODE_F8 },
	{ "F9",			KEY_F9,				KEYCODE_F9 },
	{ "F10",		KEY_F10,			KEYCODE_F10 },
	{ "F11",		KEY_F11,			KEYCODE_F11 },
	{ "F12",		KEY_F12,			KEYCODE_F12 },
	{ "ESC",		KEY_ESC,			KEYCODE_ESC },
	{ "~",			KEY_TILDE,			KEYCODE_TILDE },
	{ "-",			KEY_MINUS,			KEYCODE_OTHER },
	{ "=",			KEY_EQUALS,			KEYCODE_OTHER },
	{ "BKSPACE",	KEY_BACKSPACE,		KEYCODE_BACKSPACE },
	{ "TAB",		KEY_TAB,			KEYCODE_TAB },
	{ "[",			KEY_OPENBRACE,		KEYCODE_OTHER },
	{ "]",			KEY_CLOSEBRACE,		KEYCODE_OTHER },
	{ "ENTER",		KEY_ENTER,			KEYCODE_ENTER },
	{ ";",			KEY_COLON,			KEYCODE_OTHER },
	{ ":",			KEY_QUOTE,			KEYCODE_OTHER },
	{ "\\",			KEY_BACKSLASH,		KEYCODE_OTHER },
	{ "<",			KEY_BACKSLASH2,		KEYCODE_OTHER },
	{ ",",			KEY_COMMA,			KEYCODE_OTHER },
	{ ".",			KEY_STOP,			KEYCODE_OTHER },
	{ "/",			KEY_SLASH,			KEYCODE_OTHER },
	{ "SPACE",		KEY_SPACE,			KEYCODE_SPACE },
	{ "INS",		KEY_INSERT,			KEYCODE_INSERT },
	{ "DEL",		KEY_DEL,			KEYCODE_DEL },
	{ "HOME",		KEY_HOME,			KEYCODE_HOME },
	{ "END",		KEY_END,			KEYCODE_END },
	{ "PGUP",		KEY_PGUP,			KEYCODE_PGUP },
	{ "PGDN",		KEY_PGDN,			KEYCODE_PGDN },
	{ "LEFT",		KEY_LEFT,			KEYCODE_LEFT },
	{ "RIGHT",		KEY_RIGHT,			KEYCODE_RIGHT },
	{ "UP",			KEY_UP,				KEYCODE_UP },
	{ "DOWN",		KEY_DOWN,			KEYCODE_DOWN },
	{ "/ PAD",		KEY_SLASH_PAD,		KEYCODE_OTHER },
	{ "* PAD",		KEY_ASTERISK,		KEYCODE_OTHER },
	{ "- PAD",		KEY_MINUS_PAD,		KEYCODE_OTHER },
	{ "+ PAD",		KEY_PLUS_PAD,		KEYCODE_OTHER },
	{ ". PAD",		KEY_DEL_PAD,		KEYCODE_OTHER },
	{ "ENTER PAD",	KEY_ENTER_PAD,		KEYCODE_OTHER },
	{ "PRTSCR",		KEY_PRTSCR,			KEYCODE_OTHER },
	{ "PAUSE",		KEY_PAUSE,			KEYCODE_OTHER },
	{ "LSHIFT",		KEY_LSHIFT,			KEYCODE_LSHIFT },
	{ "RSHIFT",		KEY_RSHIFT,			KEYCODE_RSHIFT },
	{ "LCTRL",		KEY_LCONTROL,		KEYCODE_LCONTROL },
	{ "RCTRL",		KEY_RCONTROL,		KEYCODE_RCONTROL },
	{ "ALT",		KEY_ALT,			KEYCODE_LALT },
	{ "ALTGR",		KEY_ALTGR,			KEYCODE_RALT },
	{ "LWIN",		KEY_LWIN,			KEYCODE_OTHER },
	{ "RWIN",		KEY_RWIN,			KEYCODE_OTHER },
	{ "MENU",		KEY_MENU,			KEYCODE_OTHER },
	{ "SCRLOCK",	KEY_SCRLOCK,		KEYCODE_OTHER },
	{ "NUMLOCK",	KEY_NUMLOCK,		KEYCODE_OTHER },
	{ "CAPSLOCK",	KEY_CAPSLOCK,		KEYCODE_OTHER },
	{ 0, 0, 0 }	/* end of table */
};

/* return a list of all available keys */
const struct KeyboardKey *osd_get_key_list(void)
{
	return keylist;
}

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	if (use_hotrod)
	{
		while (defaults->type != IPT_END)
		{
			if (defaults->keyboard == KEYCODE_UP) defaults->keyboard = KEY_8_PAD;
			if (defaults->keyboard == KEYCODE_DOWN) defaults->keyboard = KEY_2_PAD;
			if (defaults->keyboard == KEYCODE_LEFT) defaults->keyboard = KEY_4_PAD;
			if (defaults->keyboard == KEYCODE_RIGHT) defaults->keyboard = KEY_6_PAD;
			if (defaults->type == IPT_UI_SELECT) defaults->keyboard = KEY_LCONTROL;
			if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) defaults->keyboard = KEY_R;
			if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) defaults->keyboard = KEY_F;
			if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) defaults->keyboard = KEY_D;
			if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) defaults->keyboard = KEY_G;
			if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) defaults->keyboard = KEY_8_PAD;
			if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) defaults->keyboard = KEY_2_PAD;
			if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) defaults->keyboard = KEY_4_PAD;
			if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) defaults->keyboard = KEY_6_PAD;

			defaults++;
		}
	}
}

int osd_is_key_pressed(int keycode)
{
	if (keycode >= KEY_MAX) return 0;

	if (keycode == KEY_PAUSE)
	{
		static int pressed,counter;
		int res;

		res = key[KEY_PAUSE] ^ pressed;
		if (res)
		{
			if (counter > 0)
			{
				if (--counter == 0)
					pressed = key[KEY_PAUSE];
			}
			else counter = 10;
		}

		return res;
	}

	return key[keycode];
}


void osd_wait_keypress(void)
{
	readkey();
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


void osd_poll_joysticks(void)
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
void osd_analogjoy_read(int player,int *analog_x, int *analog_y)
{
	*analog_x = *analog_y = 0;

	/* is there an analog joystick at all? */
	if (player+1 > num_joysticks || joystick == JOY_TYPE_NONE)
		return;

	*analog_x = joy[player].stick[0].axis[0].pos;
	*analog_y = joy[player].stick[0].axis[1].pos;
}


static int calibration_target;

int osd_joystick_needs_calibration (void)
{
	/* This could be improved, but unfortunately, this version of Allegro */
	/* lacks a flag which tells if a joystick is calibrationable, it only  */
	/* remembers whether calibration is yet to be done. */
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


void osd_trak_read(int player,int *deltax,int *deltay)
{
	if (player != 0 || use_mouse == 0) *deltax = *deltay = 0;
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

