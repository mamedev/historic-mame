/***************************************************************************

  inptport.c

  Input ports handling

TODO:	remove the 1 analog device per port limitation
		support for inputports producing interrupts
		support for extra "real" hardware (PC throttle's, spinners etc)

***************************************************************************/

#include "driver.h"
#include <math.h>

#ifdef MAME_NET
#include "network.h"

static unsigned short input_port_defaults[MAX_INPUT_PORTS];
static int default_player;
static int analog_player_port[MAX_INPUT_PORTS];
#endif /* MAME_NET */


/* Use the MRU code for 4way joysticks */
#define MRU_JOYSTICK

/* header identifying the version of the game.cfg file */
#define MAMECFGSTRING "MAMECFG\5"
#define MAMEDEFSTRING "MAMEDEF\4"

extern void *record;
extern void *playback;

extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];
extern unsigned int lastcoin[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];

static unsigned short input_port_value[MAX_INPUT_PORTS];
static unsigned short input_vblank[MAX_INPUT_PORTS];

/* Assuming a maxium of one analog input device per port BW 101297 */
static struct InputPort *input_analog[MAX_INPUT_PORTS];
static int input_analog_current_value[MAX_INPUT_PORTS],input_analog_previous_value[MAX_INPUT_PORTS];
static int input_analog_init[MAX_INPUT_PORTS];

static int mouse_delta_x[OSD_MAX_JOY_ANALOG], mouse_delta_y[OSD_MAX_JOY_ANALOG];
static int analog_current_x[OSD_MAX_JOY_ANALOG], analog_current_y[OSD_MAX_JOY_ANALOG];
static int analog_previous_x[OSD_MAX_JOY_ANALOG], analog_previous_y[OSD_MAX_JOY_ANALOG];


/***************************************************************************

  Configuration load/save

***************************************************************************/

char ipdn_defaultstrings[][MAX_DEFSTR_LEN] =
{
	"Off",
	"On",
	"No",
	"Yes",
	"Lives",
	"Bonus Life",
	"Difficulty",
	"Demo Sounds",
	"Coinage",
	"Coin A",
	"Coin B",
	"9 Coins/1 Credit",
	"8 Coins/1 Credit",
	"7 Coins/1 Credit",
	"6 Coins/1 Credit",
	"5 Coins/1 Credit",
	"4 Coins/1 Credit",
	"3 Coins/1 Credit",
	"2 Coins/1 Credit",
	"3 Coins/2 Credits",
	"4 Coins/3 Credits",
	"1 Coin/1 Credit",
	"4 Coins/5 Credits",
	"3 Coins/4 Credits",
	"2 Coins/3 Credits",
	"1 Coin/2 Credits",
	"2 Coins/5 Credits",
	"1 Coin/3 Credits",
	"1 Coin/4 Credits",
	"1 Coin/5 Credits",
	"1 Coin/6 Credits",
	"1 Coin/7 Credits",
	"1 Coin/8 Credits",
	"1 Coin/9 Credits",
	"Free Play",
	"Cabinet",
	"Upright",
	"Cocktail",
	"Flip Screen",
	"Service Mode",
	"Unused",
	"Unknown"
};

struct ipd inputport_defaults[] =
{
	{ IPT_UI_CONFIGURE,         "Config Menu",       KEYCODE_TAB,   JOYCODE_NONE },
	{ IPT_UI_ON_SCREEN_DISPLAY, "On Screen Display", KEYCODE_TILDE, JOYCODE_NONE },
	{ IPT_UI_PAUSE,             "Pause",             KEYCODE_P,     JOYCODE_NONE },
	{ IPT_UI_RESET_MACHINE,     "Reset Game",        KEYCODE_F3,    JOYCODE_NONE },
	{ IPT_UI_SHOW_GFX,          "Show Gfx",          KEYCODE_F4,    JOYCODE_NONE },
	{ IPT_UI_FRAMESKIP_DEC,     "Frameskip Dec",     KEYCODE_F8,    JOYCODE_NONE },
	{ IPT_UI_FRAMESKIP_INC,     "Frameskip Inc",     KEYCODE_F9,    JOYCODE_NONE },
	{ IPT_UI_THROTTLE,          "Throttle",          KEYCODE_F10,   JOYCODE_NONE },
	{ IPT_UI_SHOW_FPS,          "Show FPS",          KEYCODE_F11,   JOYCODE_NONE },
	{ IPT_UI_SNAPSHOT,          "Save Snapshot",     KEYCODE_F12,   JOYCODE_NONE },
	{ IPT_UI_TOGGLE_CHEAT,      "Toggle Cheat",      KEYCODE_F5,    JOYCODE_NONE },
	{ IPT_UI_UP,                "UI Up",             KEYCODE_UP,    JOYCODE_1_UP },
	{ IPT_UI_DOWN,              "UI Down",           KEYCODE_DOWN,  JOYCODE_1_DOWN },
	{ IPT_UI_LEFT,              "UI Left",           KEYCODE_LEFT,  JOYCODE_1_LEFT },
	{ IPT_UI_RIGHT,             "UI Right",          KEYCODE_RIGHT, JOYCODE_1_RIGHT },
	{ IPT_UI_SELECT,            "UI Select",         KEYCODE_ENTER, JOYCODE_1_BUTTON1 },
	{ IPT_UI_CANCEL,            "UI Cancel",         KEYCODE_ESC,   JOYCODE_NONE },

	{ IPT_COIN1,  "Coin A",          KEYCODE_3, JOYCODE_NONE },
	{ IPT_COIN2,  "Coin B",          KEYCODE_4, JOYCODE_NONE },
	{ IPT_COIN3,  "Coin C",          KEYCODE_5, JOYCODE_NONE },
	{ IPT_COIN4,  "Coin D",          KEYCODE_6, JOYCODE_NONE },
	{ IPT_TILT,   "Tilt",            KEYCODE_T, JOYCODE_NONE },
	{ IPT_START1, "1 Player Start",  KEYCODE_1, JOYCODE_NONE },
	{ IPT_START2, "2 Players Start", KEYCODE_2, JOYCODE_NONE },
	{ IPT_START3, "3 Players Start", KEYCODE_7, JOYCODE_NONE },
	{ IPT_START4, "4 Players Start", KEYCODE_8, JOYCODE_NONE },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER1, "P1 Up",          KEYCODE_UP,      JOYCODE_1_UP    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER1, "P1 Down",        KEYCODE_DOWN,    JOYCODE_1_DOWN  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER1, "P1 Left",        KEYCODE_LEFT,    JOYCODE_1_LEFT  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER1, "P1 Right",       KEYCODE_RIGHT,   JOYCODE_1_RIGHT },
	{ IPT_BUTTON1             | IPF_PLAYER1, "P1 Button 1",    KEYCODE_LCONTROL,JOYCODE_1_BUTTON1 },
	{ IPT_BUTTON2             | IPF_PLAYER1, "P1 Button 2",    KEYCODE_LALT,    JOYCODE_1_BUTTON2 },
	{ IPT_BUTTON3             | IPF_PLAYER1, "P1 Button 3",    KEYCODE_SPACE,   JOYCODE_1_BUTTON3 },
	{ IPT_BUTTON4             | IPF_PLAYER1, "P1 Button 4",    KEYCODE_LSHIFT,  JOYCODE_1_BUTTON4 },
	{ IPT_BUTTON5             | IPF_PLAYER1, "P1 Button 5",    KEYCODE_Z,       JOYCODE_1_BUTTON5 },
	{ IPT_BUTTON6             | IPF_PLAYER1, "P1 Button 6",    KEYCODE_X,       JOYCODE_1_BUTTON6 },
	{ IPT_BUTTON7             | IPF_PLAYER1, "P1 Button 7",    KEYCODE_C,       JOYCODE_NONE },
	{ IPT_BUTTON8             | IPF_PLAYER1, "P1 Button 8",    KEYCODE_V,       JOYCODE_NONE },
	{ IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1, "P1 Right/Up",    KEYCODE_I,       JOYCODE_1_BUTTON2 },
	{ IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1, "P1 Right/Down",  KEYCODE_K,       JOYCODE_1_BUTTON3 },
	{ IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1, "P1 Right/Left",  KEYCODE_J,       JOYCODE_1_BUTTON1 },
	{ IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1, "P1 Right/Right", KEYCODE_L,       JOYCODE_1_BUTTON4 },
	{ IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1, "P1 Left/Up",     KEYCODE_E,       JOYCODE_1_UP },
	{ IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1, "P1 Left/Down",   KEYCODE_D,       JOYCODE_1_DOWN },
	{ IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1, "P1 Left/Left",   KEYCODE_S,       JOYCODE_1_LEFT },
	{ IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1, "P1 Left/Right",  KEYCODE_F,       JOYCODE_1_RIGHT },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER2, "P2 Up",          KEYCODE_R,       JOYCODE_2_UP    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER2, "P2 Down",        KEYCODE_F,       JOYCODE_2_DOWN  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER2, "P2 Left",        KEYCODE_D,       JOYCODE_2_LEFT  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER2, "P2 Right",       KEYCODE_G,       JOYCODE_2_RIGHT },
	{ IPT_BUTTON1             | IPF_PLAYER2, "P2 Button 1",    KEYCODE_A,       JOYCODE_2_BUTTON1 },
	{ IPT_BUTTON2             | IPF_PLAYER2, "P2 Button 2",    KEYCODE_S,       JOYCODE_2_BUTTON2 },
	{ IPT_BUTTON3             | IPF_PLAYER2, "P2 Button 3",    KEYCODE_Q,       JOYCODE_2_BUTTON3 },
	{ IPT_BUTTON4             | IPF_PLAYER2, "P2 Button 4",    KEYCODE_W,       JOYCODE_2_BUTTON4 },
	{ IPT_BUTTON5             | IPF_PLAYER2, "P2 Button 5",    KEYCODE_NONE,    JOYCODE_2_BUTTON5 },
	{ IPT_BUTTON6             | IPF_PLAYER2, "P2 Button 6",    KEYCODE_NONE,    JOYCODE_2_BUTTON6 },
	{ IPT_BUTTON7             | IPF_PLAYER2, "P2 Button 7",    KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_BUTTON8             | IPF_PLAYER2, "P2 Button 8",    KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER2, "P2 Right/Up",    KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER2, "P2 Right/Down",  KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER2, "P2 Right/Left",  KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2, "P2 Right/Right", KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKLEFT_UP     | IPF_PLAYER2, "P2 Left/Up",     KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER2, "P2 Left/Down",   KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER2, "P2 Left/Left",   KEYCODE_NONE,    JOYCODE_NONE },
	{ IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER2, "P2 Left/Right",  KEYCODE_NONE,    JOYCODE_NONE },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER3, "P3 Up",          KEYCODE_I,       JOYCODE_3_UP    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER3, "P3 Down",        KEYCODE_K,       JOYCODE_3_DOWN  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER3, "P3 Left",        KEYCODE_J,       JOYCODE_3_LEFT  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER3, "P3 Right",       KEYCODE_L,       JOYCODE_3_RIGHT },
	{ IPT_BUTTON1             | IPF_PLAYER3, "P3 Button 1",    KEYCODE_RCONTROL,JOYCODE_3_BUTTON1 },
	{ IPT_BUTTON2             | IPF_PLAYER3, "P3 Button 2",    KEYCODE_RSHIFT,  JOYCODE_3_BUTTON2 },
	{ IPT_BUTTON3             | IPF_PLAYER3, "P3 Button 3",    KEYCODE_ENTER,   JOYCODE_3_BUTTON3 },
	{ IPT_BUTTON4             | IPF_PLAYER3, "P3 Button 4",    KEYCODE_NONE,    JOYCODE_3_BUTTON4 },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER4, "P4 Up",          KEYCODE_NONE,    JOYCODE_4_UP    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER4, "P4 Down",        KEYCODE_NONE,    JOYCODE_4_DOWN  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER4, "P4 Left",        KEYCODE_NONE,    JOYCODE_4_LEFT  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER4, "P4 Right",       KEYCODE_NONE,    JOYCODE_4_RIGHT },
	{ IPT_BUTTON1             | IPF_PLAYER4, "P4 Button 1",    KEYCODE_NONE,    JOYCODE_4_BUTTON1 },
	{ IPT_BUTTON2             | IPF_PLAYER4, "P4 Button 2",    KEYCODE_NONE,    JOYCODE_4_BUTTON2 },
	{ IPT_BUTTON3             | IPF_PLAYER4, "P4 Button 3",    KEYCODE_NONE,    JOYCODE_4_BUTTON3 },
	{ IPT_BUTTON4             | IPF_PLAYER4, "P4 Button 4",    KEYCODE_NONE,    JOYCODE_4_BUTTON4 },

	{ IPT_PEDAL	                | IPF_PLAYER1, "Pedal 1",        KEYCODE_LCONTROL,JOYCODE_1_BUTTON1 },
	{ IPT_PEDAL	| IPT_EXTENSION | IPF_PLAYER1, "P1 Auto Release <Y/N>", KEYCODE_Y,   JOYCODE_NONE },
	{ IPT_PEDAL                 | IPF_PLAYER2, "Pedal 2",        KEYCODE_A,       JOYCODE_2_BUTTON1 },
	{ IPT_PEDAL	| IPT_EXTENSION | IPF_PLAYER2, "P2 Auto Release <Y/N>", KEYCODE_Y,   JOYCODE_NONE },
	{ IPT_PEDAL                 | IPF_PLAYER3, "Pedal 3",        KEYCODE_RCONTROL,JOYCODE_3_BUTTON1 },
	{ IPT_PEDAL	| IPT_EXTENSION | IPF_PLAYER3, "P3 Auto Release <Y/N>", KEYCODE_Y,   JOYCODE_NONE },
	{ IPT_PEDAL                 | IPF_PLAYER4, "Pedal 4",        KEYCODE_NONE,    JOYCODE_4_BUTTON1 },
	{ IPT_PEDAL	| IPT_EXTENSION | IPF_PLAYER4, "P4 Auto Release <Y/N>", KEYCODE_Y,   JOYCODE_NONE },

	{ IPT_PADDLE | IPF_PLAYER1,  "Paddle",        KEYCODE_LEFT,  JOYCODE_1_LEFT },
	{ (IPT_PADDLE | IPF_PLAYER1)+IPT_EXTENSION,             "Paddle",        KEYCODE_RIGHT, JOYCODE_1_RIGHT  },
	{ IPT_PADDLE | IPF_PLAYER2,  "Paddle 2",      KEYCODE_D,     JOYCODE_2_LEFT },
	{ (IPT_PADDLE | IPF_PLAYER2)+IPT_EXTENSION,             "Paddle 2",      KEYCODE_G,     JOYCODE_2_RIGHT },
	{ IPT_PADDLE | IPF_PLAYER3,  "Paddle 3",      KEYCODE_J,     JOYCODE_3_LEFT },
	{ (IPT_PADDLE | IPF_PLAYER3)+IPT_EXTENSION,             "Paddle 3",      KEYCODE_L,     JOYCODE_3_RIGHT },
	{ IPT_PADDLE | IPF_PLAYER4,  "Paddle 4",      KEYCODE_NONE,  JOYCODE_4_LEFT },
	{ (IPT_PADDLE | IPF_PLAYER4)+IPT_EXTENSION,             "Paddle 4",      KEYCODE_NONE,  JOYCODE_4_RIGHT },
	{ IPT_DIAL | IPF_PLAYER1,    "Dial",          KEYCODE_LEFT,  JOYCODE_1_LEFT },
	{ (IPT_DIAL | IPF_PLAYER1)+IPT_EXTENSION,               "Dial",          KEYCODE_RIGHT, JOYCODE_1_RIGHT },
	{ IPT_DIAL | IPF_PLAYER2,    "Dial 2",        KEYCODE_D,     JOYCODE_2_LEFT },
	{ (IPT_DIAL | IPF_PLAYER2)+IPT_EXTENSION,               "Dial 2",      KEYCODE_G,     JOYCODE_2_RIGHT },
	{ IPT_DIAL | IPF_PLAYER3,    "Dial 3",        KEYCODE_J,     JOYCODE_3_LEFT },
	{ (IPT_DIAL | IPF_PLAYER3)+IPT_EXTENSION,               "Dial 3",      KEYCODE_L,     JOYCODE_3_RIGHT },
	{ IPT_DIAL | IPF_PLAYER4,    "Dial 4",        KEYCODE_NONE,  JOYCODE_4_LEFT },
	{ (IPT_DIAL | IPF_PLAYER4)+IPT_EXTENSION,               "Dial 4",      KEYCODE_NONE,  JOYCODE_4_RIGHT },

	{ IPT_TRACKBALL_X | IPF_PLAYER1, "Track X",   KEYCODE_LEFT,  JOYCODE_1_LEFT },
	{ (IPT_TRACKBALL_X | IPF_PLAYER1)+IPT_EXTENSION,                 "Track X",   KEYCODE_RIGHT, JOYCODE_1_RIGHT },
	{ IPT_TRACKBALL_X | IPF_PLAYER2, "Track X 2", KEYCODE_D,     JOYCODE_2_LEFT },
	{ (IPT_TRACKBALL_X | IPF_PLAYER2)+IPT_EXTENSION,                 "Track X 2", KEYCODE_G,     JOYCODE_2_RIGHT },
	{ IPT_TRACKBALL_X | IPF_PLAYER3, "Track X 3", KEYCODE_J,     JOYCODE_3_LEFT },
	{ (IPT_TRACKBALL_X | IPF_PLAYER3)+IPT_EXTENSION,                 "Track X 3", KEYCODE_L,     JOYCODE_3_RIGHT },
	{ IPT_TRACKBALL_X | IPF_PLAYER4, "Track X 4", KEYCODE_NONE,  JOYCODE_4_LEFT },
	{ (IPT_TRACKBALL_X | IPF_PLAYER4)+IPT_EXTENSION,                 "Track X 4", KEYCODE_NONE,  JOYCODE_4_RIGHT },

	{ IPT_TRACKBALL_Y | IPF_PLAYER1, "Track Y",   KEYCODE_UP,    JOYCODE_1_UP },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER1)+IPT_EXTENSION,                 "Track Y",   KEYCODE_DOWN,  JOYCODE_1_DOWN },
	{ IPT_TRACKBALL_Y | IPF_PLAYER2, "Track Y 2", KEYCODE_R,     JOYCODE_2_UP },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER2)+IPT_EXTENSION,                 "Track Y 2", KEYCODE_F,     JOYCODE_2_DOWN },
	{ IPT_TRACKBALL_Y | IPF_PLAYER3, "Track Y 3", KEYCODE_I,     JOYCODE_3_UP },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER3)+IPT_EXTENSION,                 "Track Y 3", KEYCODE_K,     JOYCODE_3_DOWN },
	{ IPT_TRACKBALL_Y | IPF_PLAYER4, "Track Y 4", KEYCODE_NONE,  JOYCODE_4_UP },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER4)+IPT_EXTENSION,                 "Track Y 4", KEYCODE_NONE,  JOYCODE_4_DOWN },

	{ IPT_AD_STICK_X | IPF_PLAYER1, "AD Stick X",   KEYCODE_LEFT,  JOYCODE_1_LEFT },
	{ (IPT_AD_STICK_X | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick X",   KEYCODE_RIGHT, JOYCODE_1_RIGHT },
	{ IPT_AD_STICK_X | IPF_PLAYER2, "AD Stick X 2", KEYCODE_D,     JOYCODE_2_LEFT },
	{ (IPT_AD_STICK_X | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick X 2", KEYCODE_G,     JOYCODE_2_RIGHT },
	{ IPT_AD_STICK_X | IPF_PLAYER3, "AD Stick X 3", KEYCODE_J,     JOYCODE_3_LEFT },
	{ (IPT_AD_STICK_X | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick X 3", KEYCODE_L,     JOYCODE_3_RIGHT },
	{ IPT_AD_STICK_X | IPF_PLAYER4, "AD Stick X 4", KEYCODE_NONE,  JOYCODE_4_LEFT },
	{ (IPT_AD_STICK_X | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick X 4", KEYCODE_NONE,  JOYCODE_4_RIGHT },

	{ IPT_AD_STICK_Y | IPF_PLAYER1, "AD Stick Y",   KEYCODE_UP,    JOYCODE_1_UP },
	{ (IPT_AD_STICK_Y | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick Y",   KEYCODE_DOWN,  JOYCODE_1_DOWN },
	{ IPT_AD_STICK_Y | IPF_PLAYER2, "AD Stick Y 2", KEYCODE_R,     JOYCODE_2_UP },
	{ (IPT_AD_STICK_Y | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick Y 2", KEYCODE_F,     JOYCODE_2_DOWN },
	{ IPT_AD_STICK_Y | IPF_PLAYER3, "AD Stick Y 3", KEYCODE_I,     JOYCODE_3_UP },
	{ (IPT_AD_STICK_Y | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick Y 3", KEYCODE_K,     JOYCODE_3_DOWN },
	{ IPT_AD_STICK_Y | IPF_PLAYER4, "AD Stick Y 4", KEYCODE_NONE,  JOYCODE_4_UP },
	{ (IPT_AD_STICK_Y | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick Y 4", KEYCODE_NONE,  JOYCODE_4_DOWN },

	{ IPT_UNKNOWN,             "UNKNOWN",         IP_KEY_NONE,     IP_JOY_NONE },
	{ IPT_END,                 0,                 0,     0 }	/* returned when there is no match */
};

struct ipd inputport_defaults_backup[sizeof(inputport_defaults)/sizeof(struct ipd)];


static int readint(void *f,UINT32 *num)
{
	int i;


	*num = 0;
	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;


		*num <<= 8;
		if (osd_fread(f,&c,1) != 1)
			return -1;
		*num |= c;
	}

	return 0;
}

static void writeint(void *f,UINT32 num)
{
	int i;


	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT32)-1)) & 0xff;
		osd_fwrite(f,&c,1);
		num <<= 8;
	}
}

static int readword(void *f,UINT16 *num)
{
	int i,res;


	res = 0;
	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		res <<= 8;
		if (osd_fread(f,&c,1) != 1)
			return -1;
		res |= c;
	}

	*num = res;
	return 0;
}

static void writeword(void *f,UINT16 num)
{
	int i;


	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT16)-1)) & 0xff;
		osd_fwrite(f,&c,1);
		num <<= 8;
	}
}



static void load_default_keys(void)
{
	void *f;


	osd_customize_inputport_defaults(inputport_defaults);
	memcpy(inputport_defaults_backup,inputport_defaults,sizeof(inputport_defaults));

	if ((f = osd_fopen("default",0,OSD_FILETYPE_CONFIG,0)) != 0)
	{
		char buf[8];


		/* read header */
		if (osd_fread(f,buf,8) != 8)
			goto getout;
		if (memcmp(buf,MAMEDEFSTRING,8) != 0)
			goto getout;	/* header invalid */

		for (;;)
		{
			UINT32 type;
			UINT16 def_keyboard,def_joystick;
			UINT16 keyboard,joystick;
			int i;


			if (readint(f,&type) != 0)
				goto getout;
			if (readword(f,&def_keyboard) != 0)
				goto getout;
			if (readword(f,&def_joystick) != 0)
				goto getout;
			if (readword(f,&keyboard) != 0)
				goto getout;
			if (readword(f,&joystick) != 0)
				goto getout;

			i = 0;
			while (inputport_defaults[i].type != IPT_END)
			{
				if (inputport_defaults[i].type == type)
				{
					/* load stored settings only if the default hasn't changed */
					if (inputport_defaults[i].keyboard == def_keyboard)
						inputport_defaults[i].keyboard = keyboard;
					if (inputport_defaults[i].joystick == def_joystick)
						inputport_defaults[i].joystick = joystick;
				}
				i++;
			}
		}

getout:
		osd_fclose(f);
	}
}

static void save_default_keys(void)
{
	void *f;


	if ((f = osd_fopen("default",0,OSD_FILETYPE_CONFIG,1)) != 0)
	{
		int i;


		/* write header */
		osd_fwrite(f,MAMEDEFSTRING,8);

		i = 0;
		while (inputport_defaults[i].type != IPT_END)
		{
			writeint(f,inputport_defaults[i].type);
			writeword(f,inputport_defaults_backup[i].keyboard);
			writeword(f,inputport_defaults_backup[i].joystick);
			writeword(f,inputport_defaults[i].keyboard);
			writeword(f,inputport_defaults[i].joystick);
			i++;
		}

		osd_fclose(f);
	}
	memcpy(inputport_defaults,inputport_defaults_backup,sizeof(inputport_defaults_backup));
}



static int readip(void *f,struct InputPort *in)
{
	if (readint(f,&in->type) != 0)
		return -1;
	if (readword(f,&in->mask) != 0)
		return -1;
	if (readword(f,&in->default_value) != 0)
		return -1;
	if (readword(f,&in->keyboard) != 0)
		return -1;
	if (readword(f,&in->joystick) != 0)
		return -1;

	return 0;
}

static void writeip(void *f,struct InputPort *in)
{
	writeint(f,in->type);
	writeword(f,in->mask);
	writeword(f,in->default_value);
	writeword(f,in->keyboard);
	writeword(f,in->joystick);
}



int load_input_port_settings(void)
{
	void *f;
#ifdef MAME_NET
    struct InputPort *in;
    int port, player;
#endif /* MAME_NET */


	load_default_keys();

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,0)) != 0)
	{
#ifndef MAME_NET
		struct InputPort *in;
#endif
		unsigned int total,savedtotal;
		char buf[8];
		int i;


		in = Machine->gamedrv->input_ports;

		/* calculate the size of the array */
		total = 0;
		while (in->type != IPT_END)
		{
			total++;
			in++;
		}

		/* read header */
		if (osd_fread(f,buf,8) != 8)
			goto getout;
		if (memcmp(buf,MAMECFGSTRING,8) != 0)
			goto getout;	/* header invalid */

		/* read array size */
		if (readint(f,&savedtotal) != 0)
			goto getout;
		if (total != savedtotal)
			goto getout;	/* different size */

		/* read the original settings and compare them with the ones defined in the driver */
		in = Machine->gamedrv->input_ports;
		while (in->type != IPT_END)
		{
			struct InputPort saved;


			if (readip(f,&saved) != 0)
				goto getout;

			if (in->mask != saved.mask ||
				in->default_value != saved.default_value ||
				in->type != saved.type ||
				in->keyboard != saved.keyboard ||
				in->joystick != saved.joystick)
			goto getout;	/* the default values are different */

			in++;
		}

		/* read the current settings */
		in = Machine->input_ports;
		while (in->type != IPT_END)
		{
			if (readip(f,in) != 0)
				goto getout;
			in++;
		}

		/* Clear the coin & ticket counters/flags - LBO 042898 */
		for (i = 0; i < COIN_COUNTERS; i ++)
			coins[i] = lastcoin[i] = coinlockedout[i] = 0;
		dispensed_tickets = 0;

		/* read in the coin/ticket counters */
		for (i = 0; i < COIN_COUNTERS; i ++)
		{
			if (readint(f,&coins[i]) != 0)
				goto getout;
		}
		if (readint(f,&dispensed_tickets) != 0)
			goto getout;

		mixer_read_config(f);

getout:
		osd_fclose(f);
	}

	/* All analog ports need initialization */
	{
		int i;
		for (i = 0; i < MAX_INPUT_PORTS; i++)
			input_analog_init[i] = 1;
	}
#ifdef MAME_NET
	/* Find out what port is used by what player and swap regular inputs */
	in = Machine->input_ports;

//	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
//	if (in->type != IPT_PORT)
//	{
//		if (errorlog) fprintf(errorlog,"Error in InputPort definition: expecting PORT_START\n");
//		return;
//	}
//	else in++;
	in++;

	/* scan all the input ports */
	port = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		/* now check the input bits. */
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
				(in->type & ~IPF_MASK) != IPT_EXTENSION &&			/* skip analog extension fields */
				(in->type & IPF_UNUSED) == 0 &&						/* skip unused bits */
				!(!options.cheat && (in->type & IPF_CHEAT)) &&				/* skip cheats if cheats disabled */
				(in->type & ~IPF_MASK) != IPT_VBLANK &&				/* skip vblank stuff */
				((in->type & ~IPF_MASK) >= IPT_COIN1 &&				/* skip if coin input and it's locked out */
				(in->type & ~IPF_MASK) <= IPT_COIN4 &&
                 coinlockedout[(in->type & ~IPF_MASK) - IPT_COIN1]))
			{
				player = 0;
				if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2) player = 1;
				else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3) player = 2;
				else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4) player = 3;

				if (((in->type & ~IPF_MASK) > IPT_ANALOG_START)
					&& ((in->type & ~IPF_MASK) < IPT_ANALOG_END))
				{
					analog_player_port[port] = player;
				}
				if (((in->type & ~IPF_MASK) == IPT_BUTTON1) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON2) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON3) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON4) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_DOWN) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_DOWN) ||
 					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_DOWN) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_PADDLE) ||
					((in->type & ~IPF_MASK) == IPT_DIAL) ||
					((in->type & ~IPF_MASK) == IPT_TRACKBALL_X) ||
					((in->type & ~IPF_MASK) == IPT_TRACKBALL_Y) ||
					((in->type & ~IPF_MASK) == IPT_AD_STICK_X) ||
					((in->type & ~IPF_MASK) == IPT_AD_STICK_Y))
				{
					switch (default_player)
					{
						case 0:
							/* do nothing */
							break;
						case 1:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER2;
							}
							else if (player == 1)
							{
								in->type &= ~IPF_PLAYER2;
								in->type |= IPF_PLAYER1;
							}
							break;
						case 2:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER3;
							}
							else if (player == 2)
							{
								in->type &= ~IPF_PLAYER3;
								in->type |= IPF_PLAYER1;
							}
							break;
						case 3:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER4;
							}
							else if (player == 3)
							{
								in->type &= ~IPF_PLAYER4;
								in->type |= IPF_PLAYER1;
							}
							break;
					}
				}
			}
			in++;
		}
		port++;
		if (in->type == IPT_PORT) in++;
	}

	/* TODO: at this point the games should initialize peers to same as server */

#endif /* MAME_NET */

	update_input_ports();

	/* if we didn't find a saved config, return 0 so the main core knows that it */
	/* is the first time the game is run and it should diplay the disclaimer. */
	if (f) return 1;
	else return 0;
}



void save_input_port_settings(void)
{
	void *f;
#ifdef MAME_NET
	struct InputPort *in;
	int port, player;

	/* Swap input port definitions back to defaults */
	in = Machine->input_ports;

	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		if (errorlog) fprintf(errorlog,"Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else in++;

	/* scan all the input ports */
	port = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		/* now check the input bits. */
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
				(in->type & ~IPF_MASK) != IPT_EXTENSION &&			/* skip analog extension fields */
				(in->type & IPF_UNUSED) == 0 &&						/* skip unused bits */
				!(!options.cheat && (in->type & IPF_CHEAT)) &&				/* skip cheats if cheats disabled */
				(in->type & ~IPF_MASK) != IPT_VBLANK &&				/* skip vblank stuff */
				((in->type & ~IPF_MASK) >= IPT_COIN1 &&				/* skip if coin input and it's locked out */
				(in->type & ~IPF_MASK) <= IPT_COIN4 &&
                 coinlockedout[(in->type & ~IPF_MASK) - IPT_COIN1]))
			{
				player = 0;
				if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2) player = 1;
				else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3) player = 2;
				else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4) player = 3;

				if (((in->type & ~IPF_MASK) == IPT_BUTTON1) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON2) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON3) ||
					((in->type & ~IPF_MASK) == IPT_BUTTON4) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_DOWN) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICK_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_DOWN) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKRIGHT_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_UP) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_DOWN) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_LEFT) ||
					((in->type & ~IPF_MASK) == IPT_JOYSTICKLEFT_RIGHT) ||
					((in->type & ~IPF_MASK) == IPT_PADDLE) ||
					((in->type & ~IPF_MASK) == IPT_DIAL) ||
					((in->type & ~IPF_MASK) == IPT_TRACKBALL_X) ||
					((in->type & ~IPF_MASK) == IPT_TRACKBALL_Y) ||
					((in->type & ~IPF_MASK) == IPT_AD_STICK_X) ||
					((in->type & ~IPF_MASK) == IPT_AD_STICK_Y))
				{
					switch (default_player)
					{
						case 0:
							/* do nothing */
							analog_player_port[port] = player;
							break;
						case 1:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER2;
								analog_player_port[port] = 1;
							}
							else if (player == 1)
							{
								in->type &= ~IPF_PLAYER2;
								in->type |= IPF_PLAYER1;
								analog_player_port[port] = 0;
							}
							break;
						case 2:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER3;
								analog_player_port[port] = 2;
							}
							else if (player == 2)
							{
								in->type &= ~IPF_PLAYER3;
								in->type |= IPF_PLAYER1;
								analog_player_port[port] = 0;
							}
							break;
						case 3:
							if (player == 0)
							{
								in->type &= ~IPF_PLAYER1;
								in->type |= IPF_PLAYER4;
								analog_player_port[port] = 3;
							}
							else if (player == 3)
							{
								in->type &= ~IPF_PLAYER4;
								in->type |= IPF_PLAYER1;
								analog_player_port[port] = 0;
							}
							break;
					}
				}
			}
			in++;
		}
		port++;
		if (in->type == IPT_PORT) in++;
	}
#endif /* MAME_NET */

	save_default_keys();

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,1)) != 0)
	{
#ifndef MAME_NET
		struct InputPort *in;
#endif /* MAME_NET */
		int total;
		int i;


		in = Machine->gamedrv->input_ports;

		/* calculate the size of the array */
		total = 0;
		while (in->type != IPT_END)
		{
			total++;
			in++;
		}

		/* write header */
		osd_fwrite(f,MAMECFGSTRING,8);
		/* write array size */
		writeint(f,total);
		/* write the original settings as defined in the driver */
		in = Machine->gamedrv->input_ports;
		while (in->type != IPT_END)
		{
			writeip(f,in);
			in++;
		}
		/* write the current settings */
		in = Machine->input_ports;
		while (in->type != IPT_END)
		{
			writeip(f,in);
			in++;
		}

		/* write out the coin/ticket counters for this machine - LBO 042898 */
		for (i = 0; i < COIN_COUNTERS; i ++)
			writeint(f,coins[i]);
		writeint(f,dispensed_tickets);

		mixer_write_config(f);

		osd_fclose(f);
	}
}



/* Note that the following 3 routines have slightly different meanings with analog ports */
const char *input_port_name(const struct InputPort *in)
{
	int i,type;


	if (in->name != IP_NAME_DEFAULT) return in->name;

	i = 0;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
	else
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return inputport_defaults[i+1].name;
	else
		return inputport_defaults[i].name;
}

int input_port_type_key(int type)
{
	int i;


	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	return inputport_defaults[i].keyboard;
}

int input_port_type_joy(int type)
{
	int i;


	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	return inputport_defaults[i].joystick;
}

int input_port_key(const struct InputPort *in)
{
	int i,type;


	while (in->keyboard == IP_KEY_PREVIOUS) in--;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
	{
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if (((in-1)->type & IPF_UNUSED) || (!options.cheat && ((in-1)->type & IPF_CHEAT)))
			return IP_KEY_NONE;
	}
	else
	{
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if ((in->type & IPF_UNUSED) || (!options.cheat && (in->type & IPF_CHEAT)))
			return IP_KEY_NONE;
	}

	if (in->keyboard != IP_KEY_DEFAULT) return in->keyboard;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return inputport_defaults[i+1].keyboard;
	else
		return inputport_defaults[i].keyboard;
}

int input_port_joy(const struct InputPort *in)
{
	int i,type;


	while (in->joystick == IP_JOY_PREVIOUS) in--;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
	{
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no joy */
		if (((in-1)->type & IPF_UNUSED) || (!options.cheat && ((in-1)->type & IPF_CHEAT)))
			return IP_JOY_NONE;
	}
	else
	{
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no joy */
		if ((in->type & IPF_UNUSED) || (!options.cheat && (in->type & IPF_CHEAT)))
			return IP_JOY_NONE;
	}

	if (in->joystick != IP_JOY_DEFAULT) return in->joystick;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return inputport_defaults[i+1].joystick;
	else
		return inputport_defaults[i].joystick;
}



void update_analog_port(int port)
{
	struct InputPort *in;
	int current, delta, type, sensitivity, clip, min, max, default_value;
	int axis, is_stick, check_bounds;
	int inckey, deckey, keydelta, incjoy, decjoy;
	int player;

	/* get input definition */
	in = input_analog[port];

	/* if we're not cheating and this is a cheat-only port, bail */
	if (!options.cheat && (in->type & IPF_CHEAT)) return;
	type=(in->type & ~IPF_MASK);


	deckey = input_port_key(in);
	decjoy = input_port_joy(in);
	inckey = input_port_key(in+1);
	incjoy = input_port_joy(in+1);

	keydelta = IP_GET_DELTA(in);

	switch (type)
	{
		case IPT_PADDLE:
			axis = X_AXIS; is_stick = 0; check_bounds = 1; break;
		case IPT_DIAL:
			axis = X_AXIS; is_stick = 0; check_bounds = 0; break;
		case IPT_TRACKBALL_X:
			axis = X_AXIS; is_stick = 0; check_bounds = 0; break;
		case IPT_TRACKBALL_Y:
			axis = Y_AXIS; is_stick = 0; check_bounds = 0; break;
		case IPT_AD_STICK_X:
			axis = X_AXIS; is_stick = 1; check_bounds = 1; break;
		case IPT_AD_STICK_Y:
			axis = Y_AXIS; is_stick = 1; check_bounds = 1; break;
		case IPT_PEDAL:
			axis = Y_AXIS; is_stick = 0; check_bounds = 1; break;
		default:
			/* Use some defaults to prevent crash */
			axis = X_AXIS; is_stick = 0; check_bounds = 0;
			if (errorlog)
				fprintf (errorlog,"Oops, polling non analog device in update_analog_port()????\n");
	}


	sensitivity = IP_GET_SENSITIVITY(in);
	clip = IP_GET_CLIP(in);
	min = IP_GET_MIN(in);
	max = IP_GET_MAX(in);
	default_value = in->default_value * 100 / sensitivity;
	/* extremes can be either signed or unsigned */
	if (min > max)
	{
		if (in->mask > 0xff) min = min - 0x10000;
		else min = min - 0x100;
	}


	input_analog_previous_value[port] = input_analog_current_value[port];

	/* if IPF_CENTER go back to the default position, but without */
	/* throwing away sub-precision movements which might have been done. */
	/* sticks are handled later... */
	if ((in->type & IPF_CENTER) && (!is_stick))
		input_analog_current_value[port] -=
				(input_analog_current_value[port] * sensitivity / 100 - in->default_value) * 100 / sensitivity;

	current = input_analog_current_value[port];

	delta = 0;

	switch (in->type & IPF_PLAYERMASK)
	{
		case IPF_PLAYER2:          player = 1; break;
		case IPF_PLAYER3:          player = 2; break;
		case IPF_PLAYER4:          player = 3; break;
		case IPF_PLAYER1: default: player = 0; break;
	}

	if (axis == X_AXIS)
		delta = mouse_delta_x[player];
	else
		delta = mouse_delta_y[player];

	if (keyboard_pressed(deckey)) delta -= keydelta;
	if (joystick_pressed(decjoy)) delta -= keydelta;

	if (type != IPT_PEDAL)
	{
		if (keyboard_pressed(inckey)) delta += keydelta;
		if (joystick_pressed(incjoy)) delta += keydelta;
	}
	else
	{
		/* is this cheesy or what? */
		if (!delta && inckey == KEYCODE_Y) delta += keydelta;
		delta = -delta;
	}

	if (clip != 0)
	{
		if (delta*sensitivity/100 < -clip)
			delta = -clip*100/sensitivity;
		else if (delta*sensitivity/100 > clip)
			delta = clip*100/sensitivity;
	}

	if (in->type & IPF_REVERSE) delta = -delta;

	if (is_stick)
	{
		int new, prev;

		/* center stick */
		if ((delta == 0) && (in->type & IPF_CENTER))
		{
			if (current > default_value)
				delta = -100 / sensitivity;
			if (current < default_value)
				delta =  100 / sensitivity;
		}

		/* An analog joystick which is not at zero position (or has just */
		/* moved there) takes precedence over all other computations */
		/* analog_x/y holds values from -128 to 128 (yes, 128, not 127) */

		if (axis == X_AXIS)
		{
			new  = analog_current_x[player];
			prev = analog_previous_x[player];
		}
		else
		{
			new  = analog_current_y[player];
			prev = analog_previous_y[player];
		}

		if ((new != 0) || (new-prev != 0))
		{
			delta=0;

			if (in->type & IPF_REVERSE)
			{
				new  = -new;
				prev = -prev;
			}

			/* apply sensitivity using a logarithmic scale */
			if (in->mask > 0xff)
			{
				if (new > 0)
				{
					current = (pow(new / 32768.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 32768.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
			else
			{
				if (new > 0)
				{
					current = (pow(new / 128.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 128.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
		}
	}

	current += delta;

	if (check_bounds)
	{
		if (current*sensitivity/100 < min)
			current=min*100/sensitivity;
		if (current*sensitivity/100 > max)
			current=max*100/sensitivity;
	}

	input_analog_current_value[port]=current;
}

void scale_analog_port(int port)
{
	struct InputPort *in;
	int delta,current,sensitivity;

	in = input_analog[port];
	sensitivity = IP_GET_SENSITIVITY(in);

	delta = cpu_scalebyfcount(input_analog_current_value[port] - input_analog_previous_value[port]);

	current = input_analog_previous_value[port] + delta;

	input_port_value[port] &= ~in->mask;
	input_port_value[port] |= (current * sensitivity / 100) & in->mask;

	if (playback)
		readword(playback,&input_port_value[port]);
	if (record)
		writeword(record,input_port_value[port]);
#ifdef MAME_NET
	if ( net_active() && (default_player != NET_SPECTATOR) )
		net_analog_sync((unsigned char *) input_port_value, port, analog_player_port, default_player);
#endif /* MAME_NET */
}


void update_input_ports(void)
{
	int port,ib;
	struct InputPort *in;
#define MAX_INPUT_BITS 1024
static int impulsecount[MAX_INPUT_BITS];
static int waspressed[MAX_INPUT_BITS];
#define MAX_JOYSTICKS 3
#define MAX_PLAYERS 4
#ifdef MRU_JOYSTICK
static int update_serial_number = 1;
static int joyserial[MAX_JOYSTICKS*MAX_PLAYERS][4];
#else
int joystick[MAX_JOYSTICKS*MAX_PLAYERS][4];
#endif

#ifdef MAME_NET
int player;
#endif /* MAME_NET */

	/* clear all the values before proceeding */
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		input_port_value[port] = 0;
		input_vblank[port] = 0;
		input_analog[port] = 0;
	}

#ifndef MRU_JOYSTICK
	for (i = 0;i < 4*MAX_JOYSTICKS*MAX_PLAYERS;i++)
		joystick[i/4][i%4] = 0;
#endif

	in = Machine->input_ports;

	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		if (errorlog) fprintf(errorlog,"Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else in++;

#ifdef MRU_JOYSTICK
	/* scan all the joystick ports */
	port = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) >= IPT_JOYSTICK_UP &&
				(in->type & ~IPF_MASK) <= IPT_JOYSTICKLEFT_RIGHT)
			{
				int key,joy;

				key = input_port_key(in);
				joy = input_port_joy(in);

				if ((key != 0 && key != IP_KEY_NONE) ||
					(joy != 0 && joy != IP_JOY_NONE))
				{
					int joynum,joydir,player;

					player = 0;
					if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2)
						player = 1;
					else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3)
						player = 2;
					else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4)
						player = 3;

					joynum = player * MAX_JOYSTICKS +
							 ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) / 4;
					joydir = ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) % 4;

					if (keyboard_pressed(key) || joystick_pressed(joy))
					{
						if (joyserial[joynum][joydir] == 0)
							joyserial[joynum][joydir] = update_serial_number;
					}
					else
						joyserial[joynum][joydir] = 0;
				}
			}
			in++;
		}

		port++;
		if (in->type == IPT_PORT) in++;
	}
	update_serial_number += 1;

	in = Machine->input_ports;

	/* already made sure the InputPort definition is correct */
	in++;
#endif


	/* scan all the input ports */
	port = 0;
	ib = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		struct InputPort *start;


		/* first of all, scan the whole input port definition and build the */
		/* default value. I must do it before checking for input because otherwise */
		/* multiple keys associated with the same input bit wouldn't work (the bit */
		/* would be reset to its default value by the second entry, regardless if */
		/* the key associated with the first entry was pressed) */
		start = in;
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
				(in->type & ~IPF_MASK) != IPT_EXTENSION)			/* skip analog extension fields */
			{
				input_port_value[port] =
						(input_port_value[port] & ~in->mask) | (in->default_value & in->mask);
#ifdef MAME_NET
				if ( net_active() )
					input_port_defaults[port] = input_port_value[port];
#endif /* MAME_NET */
			}

			in++;
		}

		/* now get back to the beginning of the input port and check the input bits. */
		for (in = start;
			 in->type != IPT_END && in->type != IPT_PORT;
			 in++, ib++)
		{
#ifdef MAME_NET
			player = 0;
			if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2) player = 1;
			else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3) player = 2;
			else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4) player = 3;
#endif /* MAME_NET */
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
					(in->type & ~IPF_MASK) != IPT_EXTENSION)		/* skip analog extension fields */
			{
				if ((in->type & ~IPF_MASK) == IPT_VBLANK)
				{
					input_vblank[port] ^= in->mask;
					input_port_value[port] ^= in->mask;
if (errorlog && Machine->drv->vblank_duration == 0)
	fprintf(errorlog,"Warning: you are using IPT_VBLANK with vblank_duration = 0. You need to increase vblank_duration for IPT_VBLANK to work.\n");
				}
				/* If it's an analog control, handle it appropriately */
				else if (((in->type & ~IPF_MASK) > IPT_ANALOG_START)
					  && ((in->type & ~IPF_MASK) < IPT_ANALOG_END  )) /* LBO 120897 */
				{
					input_analog[port]=in;
					/* reset the analog port on first access */
					if (input_analog_init[port])
					{
						input_analog_init[port] = 0;
						input_analog_current_value[port] = input_analog_previous_value[port]
							= in->default_value * 100 / IP_GET_SENSITIVITY(in);
					}
				}
				else
				{
					int key,joy;


					key = input_port_key(in);
					joy = input_port_joy(in);

					if (keyboard_pressed(key) || joystick_pressed(joy))
					{
						/* skip if coin input and it's locked out */
						if ((in->type & ~IPF_MASK) >= IPT_COIN1 &&
							(in->type & ~IPF_MASK) <= IPT_COIN4 &&
                            coinlockedout[(in->type & ~IPF_MASK) - IPT_COIN1])
						{
							continue;
						}

						/* if IPF_RESET set, reset the first CPU */
						if ((in->type & IPF_RESETCPU) && waspressed[ib] == 0)
							cpu_reset(0);

						if (in->type & IPF_IMPULSE)
						{
if (errorlog && IP_GET_IMPULSE(in) == 0)
	fprintf(errorlog,"error in input port definition: IPF_IMPULSE with length = 0\n");
							if (waspressed[ib] == 0)
								impulsecount[ib] = IP_GET_IMPULSE(in);
								/* the input bit will be toggled later */
						}
						else if (in->type & IPF_TOGGLE)
						{
							if (waspressed[ib] == 0)
							{
								in->default_value ^= in->mask;
								input_port_value[port] ^= in->mask;
							}
						}
						else if ((in->type & ~IPF_MASK) >= IPT_JOYSTICK_UP &&
								(in->type & ~IPF_MASK) <= IPT_JOYSTICKLEFT_RIGHT)
						{
#ifndef MAME_NET
							int joynum,joydir,mask,player;


							player = 0;
							if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2) player = 1;
							else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3) player = 2;
							else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4) player = 3;
#else
							int joynum,joydir,mask;
#endif /* !MAME_NET */
							joynum = player * MAX_JOYSTICKS +
									((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) / 4;
							joydir = ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) % 4;

							mask = in->mask;

#ifndef MRU_JOYSTICK
							/* avoid movement in two opposite directions */
							if (joystick[joynum][joydir ^ 1] != 0)
								mask = 0;
							else if (in->type & IPF_4WAY)
							{
								int dir;


								/* avoid diagonal movements */
								for (dir = 0;dir < 4;dir++)
								{
									if (joystick[joynum][dir] != 0)
										mask = 0;
								}
							}

							joystick[joynum][joydir] = 1;
#else
							/* avoid movement in two opposite directions */
							if (joyserial[joynum][joydir ^ 1] != 0)
								mask = 0;
							else if (in->type & IPF_4WAY)
							{
								int mru_dir = joydir;
								int mru_serial = 0;
								int dir;


								/* avoid diagonal movements, use mru button */
								for (dir = 0;dir < 4;dir++)
								{
									if (joyserial[joynum][dir] > mru_serial)
									{
										mru_serial = joyserial[joynum][dir];
										mru_dir = dir;
									}
								}

								if (mru_dir != joydir)
									mask = 0;
							}
#endif

							input_port_value[port] ^= mask;
						}
						else
							input_port_value[port] ^= in->mask;

						waspressed[ib] = 1;
					}
					else
						waspressed[ib] = 0;

					if ((in->type & IPF_IMPULSE) && impulsecount[ib] > 0)
					{
						impulsecount[ib]--;
						waspressed[ib] = 1;
						input_port_value[port] ^= in->mask;
					}
				}
			}
		}

		port++;
		if (in->type == IPT_PORT) in++;
	}

	if (playback)
	{
		int i;

		for (i = 0; i < MAX_INPUT_PORTS; i ++)
			readword(playback,&input_port_value[i]);
	}
	if (record)
	{
		int i;

		for (i = 0; i < MAX_INPUT_PORTS; i ++)
			writeword(record,input_port_value[i]);
	}
#ifdef MAME_NET
	if ( net_active() && (default_player != NET_SPECTATOR) )
		net_input_sync((unsigned char *) input_port_value, (unsigned char *) input_port_defaults, MAX_INPUT_PORTS);
#endif /* MAME_NET */
}



/* used the the CPU interface to notify that VBlank has ended, so we can update */
/* IPT_VBLANK input ports. */
void inputport_vblank_end(void)
{
	int port;
	int i;


	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		if (input_vblank[port])
		{
			input_port_value[port] ^= input_vblank[port];
			input_vblank[port] = 0;
		}
	}

	/* poll all the analog joysticks */
	osd_poll_joysticks();

	/* update the analog devices */
	for (i = 0;i < OSD_MAX_JOY_ANALOG;i++)
	{
		/* update the analog joystick position */
		analog_previous_x[i] = analog_current_x[i];
		analog_previous_y[i] = analog_current_y[i];
		osd_analogjoy_read (i, &(analog_current_x[i]), &(analog_current_y[i]));

		/* update mouse/trackball position */
		osd_trak_read (i, &mouse_delta_x[i], &mouse_delta_y[i]);
	}

	for (i = 0;i < MAX_INPUT_PORTS;i++)
	{
		struct InputPort *in;

		in=input_analog[i];
		if (in)
		{
			update_analog_port(i);
		}
	}
}



int readinputport(int port)
{
	struct InputPort *in;

	/* Update analog ports on demand */
	in=input_analog[port];
	if (in)
	{
		scale_analog_port(port);
	}

	return input_port_value[port];
}

int input_port_0_r(int offset) { return readinputport(0); }
int input_port_1_r(int offset) { return readinputport(1); }
int input_port_2_r(int offset) { return readinputport(2); }
int input_port_3_r(int offset) { return readinputport(3); }
int input_port_4_r(int offset) { return readinputport(4); }
int input_port_5_r(int offset) { return readinputport(5); }
int input_port_6_r(int offset) { return readinputport(6); }
int input_port_7_r(int offset) { return readinputport(7); }
int input_port_8_r(int offset) { return readinputport(8); }
int input_port_9_r(int offset) { return readinputport(9); }
int input_port_10_r(int offset) { return readinputport(10); }
int input_port_11_r(int offset) { return readinputport(11); }
int input_port_12_r(int offset) { return readinputport(12); }
int input_port_13_r(int offset) { return readinputport(13); }
int input_port_14_r(int offset) { return readinputport(14); }
int input_port_15_r(int offset) { return readinputport(15); }

#ifdef MAME_NET
void set_default_player_controls(int player)
{
	if (player == NET_SPECTATOR)
		default_player = NET_SPECTATOR;
	else
		default_player = player - 1;
}
#endif /* MAME_NET */

