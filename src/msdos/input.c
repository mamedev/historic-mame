#include "driver.h"
#include <allegro.h>


void my_textout (char *buf);
void update_screen(void);

int use_mouse;
int joy_type;

void msdos_init_input (void)
{
	set_leds(0);    /* turn off all leds */

	if (joy_type != -1)
	{
		int tmp,err;

		/* Try to install Allegro's joystick handler */
		/* load_joystick_data overwrites joy_type... */
		tmp=joy_type;
		err=load_joystick_data(0);

		if (err || (tmp != joy_type))
		/* no valid cfg file or the joy_type doesn't match */
		{
			joy_type=tmp;
			if (initialise_joystick() != 0)
			{
				printf ("Joystick not found.\n");
				joy_type=-1;
			}
		}
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
	int key;


	/* wait for all keys to be released */
	do
	{
	      for (key = OSD_MAX_KEY;key >= 0;key--)
	      if (osd_key_pressed(key)) break;
	} while (key >= 0);

	/* wait for a key press */
	do
	{
	      for (key = OSD_MAX_KEY;key >= 0;key--)
	      if (osd_key_pressed(key)) break;
	} while (key < 0);

	return key;
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
	if (joy_type != -1)
		poll_joystick();
}


/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
	/* compiler bug? If I don't declare these variables as volatile, */
	/* joystick right is not detected */
#if 0
	extern volatile joy_left, joy_right, joy_up, joy_down;
	extern volatile joy2_left, joy2_right, joy2_up, joy2_down;
	extern volatile joy_b1, joy_b2, joy_b3, joy_b4, joy_b5, joy_b6;
	extern volatile joy2_b1, joy2_b2;
	extern volatile joy_hat;
#endif

	switch (joycode)
	{
		case OSD_JOY_LEFT:
			return joy_left;
			break;
		case OSD_JOY_RIGHT:
			return joy_right;
			break;
		case OSD_JOY_UP:
			return joy_up;
			break;
		case OSD_JOY_DOWN:
			return joy_down;
			break;
		case OSD_JOY_FIRE1:
			return (joy_b1 || (mouse_b & 1));
			break;
		case OSD_JOY_FIRE2:
			return (joy_b2 || (mouse_b & 2));
			break;
		case OSD_JOY_FIRE3:
			return (joy_b3 || (mouse_b & 4));
			break;
		case OSD_JOY_FIRE4:
			return joy_b4;
			break;
		case OSD_JOY_FIRE5:
			return joy_b5;
			break;
		case OSD_JOY_FIRE6:
			return joy_b6;
			break;
		case OSD_JOY_FIRE:
			return (joy_b1 || joy_b2 || joy_b3 || joy_b4 || joy_b5 || joy_b6 || mouse_b);

		/* The second joystick (only 2 fire buttons!)*/
		case OSD_JOY2_LEFT:
			return joy2_left;
			break;
		case OSD_JOY2_RIGHT:
			return joy2_right;
			break;
		case OSD_JOY2_UP:
			return joy2_up;
			break;
		case OSD_JOY2_DOWN:
			return joy2_down;
			break;
		case OSD_JOY2_FIRE1:
			return (joy2_b1);
			break;
		case OSD_JOY2_FIRE2:
			return (joy2_b2);
			break;
		case OSD_JOY2_FIRE:
			return (joy2_b1 || joy2_b2);
			break;

		/* The third joystick (coolie hat) */
		case OSD_JOY3_LEFT:
			return (joy_hat==JOY_HAT_LEFT);
			break;
		case OSD_JOY3_RIGHT:
			return (joy_hat==JOY_HAT_RIGHT);
			break;
		case OSD_JOY3_UP:
			return (joy_hat==JOY_HAT_UP);
			break;
		case OSD_JOY3_DOWN:
			return (joy_hat==JOY_HAT_DOWN);
			break;
		default:
//                      if (errorlog)
//                              fprintf (errorlog,"Error: osd_joy_pressed called with no valid joyname\n");
			break;
	}
	return 0;
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
int osd_analogjoy_read(int axis)
{
	if (joy_type==-1)
		return (NO_ANALOGJOY);

	switch(axis) {
		case X_AXIS:
			return(joy_x);
			break;
		case Y_AXIS:
			return(joy_y);
			break;
	}
	fprintf (errorlog, "osd_analogjoy_read(): no axis selected\n");
	return 0;
}

void joy_calibration(void)
{
	char buf[60];
	int i;
	char *hat_posname[4] = { "up", "down", "left", "right" };
	int hat_pos[4] = { JOY_HAT_UP, JOY_HAT_DOWN, JOY_HAT_LEFT, JOY_HAT_RIGHT };

	if (joy_type==-1)
	{
		return;
	}

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Center joystick(s)");
	my_textout(buf);
	update_screen();
	(void)osd_read_key();
	initialise_joystick();

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Move to upper left");
	my_textout(buf);
	update_screen();
	(void)osd_read_key();
	calibrate_joystick_tl();

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Move to lower right");
	my_textout(buf);
	update_screen();
	(void)osd_read_key();
	calibrate_joystick_br();

	if (joy_type != JOY_TYPE_WINGEX ) goto end_calibration;

	calibrate_joystick_hat(JOY_HAT_CENTRE);
	for (i=0; i<4; i++)
	{
		osd_clearbitmap(Machine->scrbitmap);
		sprintf (buf,"Move hat %s",hat_posname[i]);
		my_textout(buf);
		update_screen();
		(void)osd_read_key();
		calibrate_joystick_hat(hat_pos[i]);
	}

end_calibration:

	osd_clearbitmap(Machine->scrbitmap);
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
