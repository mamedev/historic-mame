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
/* mame 0.36b11 */
#define MAMECFGSTRING_V5 "MAMECFG\5"
#define MAMEDEFSTRING_V4 "MAMEDEF\4"

/* mame 0.36b12 with multi key/joy extension */
#define MAMECFGSTRING_V6 "MAMECFG\6"
#define MAMEDEFSTRING_V5 "MAMEDEF\5"

/* mame 0.36b13 with multi key/joy extension with and/or combination */
#define MAMECFGSTRING_V7 "MAMECFG\7"
#define MAMEDEFSTRING_V6 "MAMEDEF\6"


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
	"8 Coins/3 Credits",
	"4 Coins/2 Credits",
	"2 Coins/1 Credit",
	"5 Coins/3 Credits",
	"3 Coins/2 Credits",
	"4 Coins/3 Credits",
	"4 Coins/4 Credits",
	"3 Coins/3 Credits",
	"2 Coins/2 Credits",
	"1 Coin/1 Credit",
	"4 Coins/5 Credits",
	"3 Coins/4 Credits",
	"2 Coins/3 Credits",
	"4 Coins/7 Credits",
	"2 Coins/4 Credits",
	"1 Coin/2 Credits",
	"2 Coins/5 Credits",
	"2 Coins/6 Credits",
	"1 Coin/3 Credits",
	"2 Coins/7 Credits",
	"2 Coins/8 Credits",
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
	"Unknown"	/* must be the last one, mame.c relies on that */
};

struct ipd inputport_defaults[] =
{
	{ IPT_UI_CONFIGURE,         "Config Menu",       INPUT_KEY_SEQ_DEF_1(KEYCODE_TAB),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_ON_SCREEN_DISPLAY, "On Screen Display", INPUT_KEY_SEQ_DEF_1(KEYCODE_TILDE), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_PAUSE,             "Pause",             INPUT_KEY_SEQ_DEF_1(KEYCODE_P),     INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_RESET_MACHINE,     "Reset Game",        INPUT_KEY_SEQ_DEF_1(KEYCODE_F3),    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_SHOW_GFX,          "Show Gfx",          INPUT_KEY_SEQ_DEF_1(KEYCODE_F4),    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_FRAMESKIP_DEC,     "Frameskip Dec",     INPUT_KEY_SEQ_DEF_1(KEYCODE_F8),    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_FRAMESKIP_INC,     "Frameskip Inc",     INPUT_KEY_SEQ_DEF_1(KEYCODE_F9),    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_THROTTLE,          "Throttle",          INPUT_KEY_SEQ_DEF_1(KEYCODE_F10),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_SHOW_FPS,          "Show FPS",          INPUT_KEY_SEQ_DEF_5(KEYCODE_F11, IP_CODE_NOT, KEYCODE_LCONTROL, IP_CODE_NOT, KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_SHOW_PROFILER,     "Show Profiler",     INPUT_KEY_SEQ_DEF_2(KEYCODE_F11, KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
#ifdef MAME_DEBUG
	{ IPT_UI_SHOW_COLORS,       "Show Colors",	 INPUT_KEY_SEQ_DEF_2(KEYCODE_F11, KEYCODE_LCONTROL), INPUT_JOY_SEQ_DEF_0 },
#endif
#ifdef MESS
	{ IPT_UI_TOGGLE_UI,         "UI Toggle",         INPUT_KEY_SEQ_DEF_1(KEYCODE_SCRLOCK),INPUT_JOY_SEQ_DEF_0 },
#endif
	{ IPT_UI_SNAPSHOT,          "Save Snapshot",     INPUT_KEY_SEQ_DEF_1(KEYCODE_F12),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_TOGGLE_CHEAT,      "Toggle Cheat",      INPUT_KEY_SEQ_DEF_1(KEYCODE_F5),    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_UP,                "UI Up",             INPUT_KEY_SEQ_DEF_1(KEYCODE_UP),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_UP) },
	{ IPT_UI_DOWN,              "UI Down",           INPUT_KEY_SEQ_DEF_1(KEYCODE_DOWN),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_DOWN) },
	{ IPT_UI_LEFT,              "UI Left",           INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ IPT_UI_RIGHT,             "UI Right",          INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },
	{ IPT_UI_SELECT,            "UI Select",         INPUT_KEY_SEQ_DEF_1(KEYCODE_ENTER), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON1) },
	{ IPT_UI_CANCEL,            "UI Cancel",         INPUT_KEY_SEQ_DEF_1(KEYCODE_ESC),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_PAN_UP,            "Pan Up",            INPUT_KEY_SEQ_DEF_3(KEYCODE_PGUP, IP_CODE_NOT, KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_PAN_DOWN,          "Pan Down",          INPUT_KEY_SEQ_DEF_3(KEYCODE_PGDN, IP_CODE_NOT, KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_PAN_LEFT,          "Pan Left",          INPUT_KEY_SEQ_DEF_2(KEYCODE_PGUP,KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_UI_PAN_RIGHT,         "Pan Right",         INPUT_KEY_SEQ_DEF_2(KEYCODE_PGDN,KEYCODE_LSHIFT), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_COIN1,  "Coin A",          INPUT_KEY_SEQ_DEF_1(KEYCODE_3), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_COIN2,  "Coin B",          INPUT_KEY_SEQ_DEF_1(KEYCODE_4), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_COIN3,  "Coin C",          INPUT_KEY_SEQ_DEF_1(KEYCODE_5), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_COIN4,  "Coin D",          INPUT_KEY_SEQ_DEF_1(KEYCODE_6), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_TILT,   "Tilt",            INPUT_KEY_SEQ_DEF_1(KEYCODE_T), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_START1, "1 Player Start",  INPUT_KEY_SEQ_DEF_1(KEYCODE_1), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_START2, "2 Players Start", INPUT_KEY_SEQ_DEF_1(KEYCODE_2), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_START3, "3 Players Start", INPUT_KEY_SEQ_DEF_1(KEYCODE_7), INPUT_JOY_SEQ_DEF_0 },
	{ IPT_START4, "4 Players Start", INPUT_KEY_SEQ_DEF_1(KEYCODE_8), INPUT_JOY_SEQ_DEF_0 },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER1, "P1 Up",          INPUT_KEY_SEQ_DEF_1(KEYCODE_UP),      INPUT_JOY_SEQ_DEF_1(JOYCODE_1_UP)    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER1, "P1 Down",        INPUT_KEY_SEQ_DEF_1(KEYCODE_DOWN),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_DOWN)  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER1, "P1 Left",        INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT)  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER1, "P1 Right",       INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT),   INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },
	{ IPT_BUTTON1             | IPF_PLAYER1, "P1 Button 1",    INPUT_KEY_SEQ_DEF_1(KEYCODE_LCONTROL),INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON1) },
	{ IPT_BUTTON2             | IPF_PLAYER1, "P1 Button 2",    INPUT_KEY_SEQ_DEF_1(KEYCODE_LALT),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON2) },
	{ IPT_BUTTON3             | IPF_PLAYER1, "P1 Button 3",    INPUT_KEY_SEQ_DEF_1(KEYCODE_SPACE),   INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON3) },
	{ IPT_BUTTON4             | IPF_PLAYER1, "P1 Button 4",    INPUT_KEY_SEQ_DEF_1(KEYCODE_LSHIFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON4) },
	{ IPT_BUTTON5             | IPF_PLAYER1, "P1 Button 5",    INPUT_KEY_SEQ_DEF_1(KEYCODE_Z),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON5) },
	{ IPT_BUTTON6             | IPF_PLAYER1, "P1 Button 6",    INPUT_KEY_SEQ_DEF_1(KEYCODE_X),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON6) },
	{ IPT_BUTTON7             | IPF_PLAYER1, "P1 Button 7",    INPUT_KEY_SEQ_DEF_1(KEYCODE_C),       INPUT_JOY_SEQ_DEF_0 },
	{ IPT_BUTTON8             | IPF_PLAYER1, "P1 Button 8",    INPUT_KEY_SEQ_DEF_1(KEYCODE_V),       INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1, "P1 Right/Up",    INPUT_KEY_SEQ_DEF_1(KEYCODE_I),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON2) },
	{ IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1, "P1 Right/Down",  INPUT_KEY_SEQ_DEF_1(KEYCODE_K),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON3) },
	{ IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1, "P1 Right/Left",  INPUT_KEY_SEQ_DEF_1(KEYCODE_J),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON1) },
	{ IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1, "P1 Right/Right", INPUT_KEY_SEQ_DEF_1(KEYCODE_L),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON4) },
	{ IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1, "P1 Left/Up",     INPUT_KEY_SEQ_DEF_1(KEYCODE_E),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_UP) },
	{ IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1, "P1 Left/Down",   INPUT_KEY_SEQ_DEF_1(KEYCODE_D),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_DOWN) },
	{ IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1, "P1 Left/Left",   INPUT_KEY_SEQ_DEF_1(KEYCODE_S),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1, "P1 Left/Right",  INPUT_KEY_SEQ_DEF_1(KEYCODE_F),       INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER2, "P2 Up",          INPUT_KEY_SEQ_DEF_1(KEYCODE_R),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_UP)    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER2, "P2 Down",        INPUT_KEY_SEQ_DEF_1(KEYCODE_F),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_DOWN)  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER2, "P2 Left",        INPUT_KEY_SEQ_DEF_1(KEYCODE_D),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_LEFT)  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER2, "P2 Right",       INPUT_KEY_SEQ_DEF_1(KEYCODE_G),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_RIGHT) },
	{ IPT_BUTTON1             | IPF_PLAYER2, "P2 Button 1",    INPUT_KEY_SEQ_DEF_1(KEYCODE_A),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON1) },
	{ IPT_BUTTON2             | IPF_PLAYER2, "P2 Button 2",    INPUT_KEY_SEQ_DEF_1(KEYCODE_S),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON2) },
	{ IPT_BUTTON3             | IPF_PLAYER2, "P2 Button 3",    INPUT_KEY_SEQ_DEF_1(KEYCODE_Q),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON3) },
	{ IPT_BUTTON4             | IPF_PLAYER2, "P2 Button 4",    INPUT_KEY_SEQ_DEF_1(KEYCODE_W),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON4) },
	{ IPT_BUTTON5             | IPF_PLAYER2, "P2 Button 5",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON5) },
	{ IPT_BUTTON6             | IPF_PLAYER2, "P2 Button 6",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON6) },
	{ IPT_BUTTON7             | IPF_PLAYER2, "P2 Button 7",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_BUTTON8             | IPF_PLAYER2, "P2 Button 8",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER2, "P2 Right/Up",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER2, "P2 Right/Down",  INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER2, "P2 Right/Left",  INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2, "P2 Right/Right", INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKLEFT_UP     | IPF_PLAYER2, "P2 Left/Up",     INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER2, "P2 Left/Down",   INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER2, "P2 Left/Left",   INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },
	{ IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER2, "P2 Left/Right",  INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_0 },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER3, "P3 Up",          INPUT_KEY_SEQ_DEF_1(KEYCODE_I),       INPUT_JOY_SEQ_DEF_1(JOYCODE_3_UP)    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER3, "P3 Down",        INPUT_KEY_SEQ_DEF_1(KEYCODE_K),       INPUT_JOY_SEQ_DEF_1(JOYCODE_3_DOWN)  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER3, "P3 Left",        INPUT_KEY_SEQ_DEF_1(KEYCODE_J),       INPUT_JOY_SEQ_DEF_1(JOYCODE_3_LEFT)  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER3, "P3 Right",       INPUT_KEY_SEQ_DEF_1(KEYCODE_L),       INPUT_JOY_SEQ_DEF_1(JOYCODE_3_RIGHT) },
	{ IPT_BUTTON1             | IPF_PLAYER3, "P3 Button 1",    INPUT_KEY_SEQ_DEF_1(KEYCODE_RCONTROL),INPUT_JOY_SEQ_DEF_1(JOYCODE_3_BUTTON1) },
	{ IPT_BUTTON2             | IPF_PLAYER3, "P3 Button 2",    INPUT_KEY_SEQ_DEF_1(KEYCODE_RSHIFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_3_BUTTON2) },
	{ IPT_BUTTON3             | IPF_PLAYER3, "P3 Button 3",    INPUT_KEY_SEQ_DEF_1(KEYCODE_ENTER),   INPUT_JOY_SEQ_DEF_1(JOYCODE_3_BUTTON3) },
	{ IPT_BUTTON4             | IPF_PLAYER3, "P3 Button 4",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_3_BUTTON4) },

	{ IPT_JOYSTICK_UP         | IPF_PLAYER4, "P4 Up",          INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_UP)    },
	{ IPT_JOYSTICK_DOWN       | IPF_PLAYER4, "P4 Down",        INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_DOWN)  },
	{ IPT_JOYSTICK_LEFT       | IPF_PLAYER4, "P4 Left",        INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_LEFT)  },
	{ IPT_JOYSTICK_RIGHT      | IPF_PLAYER4, "P4 Right",       INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_RIGHT) },
	{ IPT_BUTTON1             | IPF_PLAYER4, "P4 Button 1",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_BUTTON1) },
	{ IPT_BUTTON2             | IPF_PLAYER4, "P4 Button 2",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_BUTTON2) },
	{ IPT_BUTTON3             | IPF_PLAYER4, "P4 Button 3",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_BUTTON3) },
	{ IPT_BUTTON4             | IPF_PLAYER4, "P4 Button 4",    INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_BUTTON4) },

	{ IPT_PEDAL	                | IPF_PLAYER1, "Pedal 1",        INPUT_KEY_SEQ_DEF_1(KEYCODE_LCONTROL),INPUT_JOY_SEQ_DEF_1(JOYCODE_1_BUTTON1) },
	{ (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER1, "P1 Auto Release <Y/N>", INPUT_KEY_SEQ_DEF_1(KEYCODE_Y),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_PEDAL                 | IPF_PLAYER2, "Pedal 2",        INPUT_KEY_SEQ_DEF_1(KEYCODE_A),       INPUT_JOY_SEQ_DEF_1(JOYCODE_2_BUTTON1) },
	{ (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER2, "P2 Auto Release <Y/N>", INPUT_KEY_SEQ_DEF_1(KEYCODE_Y),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_PEDAL                 | IPF_PLAYER3, "Pedal 3",        INPUT_KEY_SEQ_DEF_1(KEYCODE_RCONTROL),INPUT_JOY_SEQ_DEF_1(JOYCODE_3_BUTTON1) },
	{ (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER3, "P3 Auto Release <Y/N>", INPUT_KEY_SEQ_DEF_1(KEYCODE_Y),   INPUT_JOY_SEQ_DEF_0 },
	{ IPT_PEDAL                 | IPF_PLAYER4, "Pedal 4",        INPUT_KEY_SEQ_DEF_0,    INPUT_JOY_SEQ_DEF_1(JOYCODE_4_BUTTON1) },
	{ (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER4, "P4 Auto Release <Y/N>", INPUT_KEY_SEQ_DEF_1(KEYCODE_Y),   INPUT_JOY_SEQ_DEF_0 },

	{ IPT_PADDLE | IPF_PLAYER1,  "Paddle",        INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ (IPT_PADDLE | IPF_PLAYER1)+IPT_EXTENSION,             "Paddle",        INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT)  },
	{ IPT_PADDLE | IPF_PLAYER2,  "Paddle 2",      INPUT_KEY_SEQ_DEF_1(KEYCODE_D),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_LEFT) },
	{ (IPT_PADDLE | IPF_PLAYER2)+IPT_EXTENSION,             "Paddle 2",      INPUT_KEY_SEQ_DEF_1(KEYCODE_G),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_RIGHT) },
	{ IPT_PADDLE | IPF_PLAYER3,  "Paddle 3",      INPUT_KEY_SEQ_DEF_1(KEYCODE_J),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_LEFT) },
	{ (IPT_PADDLE | IPF_PLAYER3)+IPT_EXTENSION,             "Paddle 3",      INPUT_KEY_SEQ_DEF_1(KEYCODE_L),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_RIGHT) },
	{ IPT_PADDLE | IPF_PLAYER4,  "Paddle 4",      INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_LEFT) },
	{ (IPT_PADDLE | IPF_PLAYER4)+IPT_EXTENSION,             "Paddle 4",      INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_RIGHT) },
	{ IPT_DIAL | IPF_PLAYER1,    "Dial",          INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ (IPT_DIAL | IPF_PLAYER1)+IPT_EXTENSION,               "Dial",          INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },
	{ IPT_DIAL | IPF_PLAYER2,    "Dial 2",        INPUT_KEY_SEQ_DEF_1(KEYCODE_D),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_LEFT) },
	{ (IPT_DIAL | IPF_PLAYER2)+IPT_EXTENSION,               "Dial 2",      INPUT_KEY_SEQ_DEF_1(KEYCODE_G),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_RIGHT) },
	{ IPT_DIAL | IPF_PLAYER3,    "Dial 3",        INPUT_KEY_SEQ_DEF_1(KEYCODE_J),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_LEFT) },
	{ (IPT_DIAL | IPF_PLAYER3)+IPT_EXTENSION,               "Dial 3",      INPUT_KEY_SEQ_DEF_1(KEYCODE_L),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_RIGHT) },
	{ IPT_DIAL | IPF_PLAYER4,    "Dial 4",        INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_LEFT) },
	{ (IPT_DIAL | IPF_PLAYER4)+IPT_EXTENSION,               "Dial 4",      INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_RIGHT) },

	{ IPT_TRACKBALL_X | IPF_PLAYER1, "Track X",   INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ (IPT_TRACKBALL_X | IPF_PLAYER1)+IPT_EXTENSION,                 "Track X",   INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },
	{ IPT_TRACKBALL_X | IPF_PLAYER2, "Track X 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_D),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_LEFT) },
	{ (IPT_TRACKBALL_X | IPF_PLAYER2)+IPT_EXTENSION,                 "Track X 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_G),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_RIGHT) },
	{ IPT_TRACKBALL_X | IPF_PLAYER3, "Track X 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_J),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_LEFT) },
	{ (IPT_TRACKBALL_X | IPF_PLAYER3)+IPT_EXTENSION,                 "Track X 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_L),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_RIGHT) },
	{ IPT_TRACKBALL_X | IPF_PLAYER4, "Track X 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_LEFT) },
	{ (IPT_TRACKBALL_X | IPF_PLAYER4)+IPT_EXTENSION,                 "Track X 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_RIGHT) },

	{ IPT_TRACKBALL_Y | IPF_PLAYER1, "Track Y",   INPUT_KEY_SEQ_DEF_1(KEYCODE_UP),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_UP) },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER1)+IPT_EXTENSION,                 "Track Y",   INPUT_KEY_SEQ_DEF_1(KEYCODE_DOWN),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_DOWN) },
	{ IPT_TRACKBALL_Y | IPF_PLAYER2, "Track Y 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_R),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_UP) },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER2)+IPT_EXTENSION,                 "Track Y 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_F),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_DOWN) },
	{ IPT_TRACKBALL_Y | IPF_PLAYER3, "Track Y 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_I),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_UP) },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER3)+IPT_EXTENSION,                 "Track Y 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_K),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_DOWN) },
	{ IPT_TRACKBALL_Y | IPF_PLAYER4, "Track Y 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_UP) },
	{ (IPT_TRACKBALL_Y | IPF_PLAYER4)+IPT_EXTENSION,                 "Track Y 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_DOWN) },

	{ IPT_AD_STICK_X | IPF_PLAYER1, "AD Stick X",   INPUT_KEY_SEQ_DEF_1(KEYCODE_LEFT),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_LEFT) },
	{ (IPT_AD_STICK_X | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick X",   INPUT_KEY_SEQ_DEF_1(KEYCODE_RIGHT), INPUT_JOY_SEQ_DEF_1(JOYCODE_1_RIGHT) },
	{ IPT_AD_STICK_X | IPF_PLAYER2, "AD Stick X 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_D),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_LEFT) },
	{ (IPT_AD_STICK_X | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick X 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_G),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_RIGHT) },
	{ IPT_AD_STICK_X | IPF_PLAYER3, "AD Stick X 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_J),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_LEFT) },
	{ (IPT_AD_STICK_X | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick X 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_L),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_RIGHT) },
	{ IPT_AD_STICK_X | IPF_PLAYER4, "AD Stick X 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_LEFT) },
	{ (IPT_AD_STICK_X | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick X 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_RIGHT) },

	{ IPT_AD_STICK_Y | IPF_PLAYER1, "AD Stick Y",   INPUT_KEY_SEQ_DEF_1(KEYCODE_UP),    INPUT_JOY_SEQ_DEF_1(JOYCODE_1_UP) },
	{ (IPT_AD_STICK_Y | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick Y",   INPUT_KEY_SEQ_DEF_1(KEYCODE_DOWN),  INPUT_JOY_SEQ_DEF_1(JOYCODE_1_DOWN) },
	{ IPT_AD_STICK_Y | IPF_PLAYER2, "AD Stick Y 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_R),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_UP) },
	{ (IPT_AD_STICK_Y | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick Y 2", INPUT_KEY_SEQ_DEF_1(KEYCODE_F),     INPUT_JOY_SEQ_DEF_1(JOYCODE_2_DOWN) },
	{ IPT_AD_STICK_Y | IPF_PLAYER3, "AD Stick Y 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_I),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_UP) },
	{ (IPT_AD_STICK_Y | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick Y 3", INPUT_KEY_SEQ_DEF_1(KEYCODE_K),     INPUT_JOY_SEQ_DEF_1(JOYCODE_3_DOWN) },
	{ IPT_AD_STICK_Y | IPF_PLAYER4, "AD Stick Y 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_UP) },
	{ (IPT_AD_STICK_Y | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick Y 4", INPUT_KEY_SEQ_DEF_0,  INPUT_JOY_SEQ_DEF_1(JOYCODE_4_DOWN) },

	{ IPT_UNKNOWN,             "UNKNOWN",         INPUT_KEY_SEQ_DEF_0,     INPUT_JOY_SEQ_DEF_0 },
	{ IPT_END,                 0,                 INPUT_KEY_SEQ_DEF_0,     INPUT_JOY_SEQ_DEF_0 }	/* returned when there is no match */
};

struct ipd inputport_defaults_backup[sizeof(inputport_defaults)/sizeof(struct ipd)];

/***************************************************************************/
/* Generic IO */

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

/***************************************************************************/
/* Sequences */

void input_key_seq_set_1(InputKeySeq* a, InputCode code)
{
	int j;
	(*a)[0] = code;
	for(j=1;j<INPUT_KEY_SEQ_MAX;++j)
		(*a)[j] = IP_KEY_NONE;
}

void input_joy_seq_set_1(InputJoySeq* a, InputCode code)
{
	int j;
	(*a)[0] = code;
	for(j=1;j<INPUT_JOY_SEQ_MAX;++j)
		(*a)[j] = IP_JOY_NONE;
}

void input_key_seq_copy(InputKeySeq* a, InputKeySeq* b)
{
	int j;
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
		(*a)[j] = (*b)[j];
}

void input_joy_seq_copy(InputJoySeq* a, InputJoySeq* b)
{
	int j;
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
		(*a)[j] = (*b)[j];
}

int input_key_seq_cmp(InputKeySeq* a, InputKeySeq* b)
{
	int j;
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
		if ((*a)[j] != (*b)[j])
			return -1;
	return 0;
}

int input_joy_seq_cmp(InputJoySeq* a, InputJoySeq* b)
{
	int j;
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
		if ((*a)[j] != (*b)[j])
			return -1;
	return 0;
}

static int input_key_seq_read(void* f, InputKeySeq* code, int multikey, int andorkey)
{
	int j;
	if (multikey)
	{
		if (andorkey)
		{
			for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
				if (readword(f,&(*code)[j]) != 0)
					return -1;
		}
		else
		{
			for(j=0;j<2;++j)
				if (readword(f,&(*code)[j]) != 0)
					return -1;
			for(j=2;j<INPUT_KEY_SEQ_MAX;++j)
				(*code)[j] = IP_KEY_NONE;
		}
	}
	else
	{
		if (readword(f,&(*code)[0]) != 0)
			return -1;
		for(j=1;j<INPUT_KEY_SEQ_MAX;++j)
			(*code)[j] = IP_KEY_NONE;
	}

	/* convert any 0 and KEYCODE_NONE to IP_KEY_NONE */
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
		if ((*code)[j]==0 || (*code)[j]==KEYCODE_NONE)
			(*code)[j] = IP_KEY_NONE;

	return 0;
}

static int input_joy_seq_read(void* f, InputJoySeq* code, int multikey, int andorkey)
{
	int j;
	if (multikey)
	{
		if (andorkey)
		{
			for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
				if (readword(f,&(*code)[j]) != 0)
					return -1;
		}
		else
		{
			for(j=0;j<2;++j)
				if (readword(f,&(*code)[j]) != 0)
					return -1;
			for(j=2;j<INPUT_JOY_SEQ_MAX;++j)
				(*code)[j] = IP_JOY_NONE;
		}
	}
	else
	{
		if (readword(f,&(*code)[0]) != 0)
			return -1;
		for(j=1;j<INPUT_JOY_SEQ_MAX;++j)
			(*code)[j] = IP_JOY_NONE;
	}

	/* convert any 0 and JOYCODE_NONE to IP_JOY_NONE */
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
		if ((*code)[j]==0 || (*code)[j]==JOYCODE_NONE)
			(*code)[j] = IP_JOY_NONE;

	return 0;
}

static void input_key_seq_write(void* f, InputKeySeq* code)
{
	int j;
	for(j=0;j<INPUT_KEY_SEQ_MAX;++j)
		writeword(f,(*code)[j]);
}

static void input_joy_seq_write(void* f, InputJoySeq* code)
{
	int j;
	for(j=0;j<INPUT_JOY_SEQ_MAX;++j)
		writeword(f,(*code)[j]);
}

/***************************************************************************/
/* Load */

static void load_default_keys(void)
{
	void *f;


	osd_customize_inputport_defaults(inputport_defaults);
	memcpy(inputport_defaults_backup,inputport_defaults,sizeof(inputport_defaults));

	if ((f = osd_fopen("default",0,OSD_FILETYPE_CONFIG,0)) != 0)
	{
		char buf[8];

		int multikey;
		int andorkey;

		/* read header */
		if (osd_fread(f,buf,8) != 8)
			goto getout;

		if (memcmp(buf,MAMEDEFSTRING_V4,8) == 0)
		{
			multikey = 0;
			andorkey = 0;
		}
		else if (memcmp(buf,MAMEDEFSTRING_V5,8) == 0)
		{
			multikey = 1;
			andorkey = 0;
		}
		else if (memcmp(buf,MAMEDEFSTRING_V6,8) == 0)
		{
			multikey = 1;
			andorkey = 1;
		} else
			goto getout;	/* header invalid */

		for (;;)
		{
			UINT32 type;
			InputKeySeq def_keyboard;
			InputJoySeq def_joystick;
			InputKeySeq keyboard;
			InputJoySeq joystick;
			int i;

			if (readint(f,&type) != 0)
				goto getout;

			if (input_key_seq_read(f,&def_keyboard,multikey,andorkey)!=0)
				goto getout;
			if (input_joy_seq_read(f,&def_joystick,multikey,andorkey)!=0)
				goto getout;
			if (input_key_seq_read(f,&keyboard,multikey,andorkey)!=0)
				goto getout;
			if (input_joy_seq_read(f,&joystick,multikey,andorkey)!=0)
				goto getout;

			i = 0;
			while (inputport_defaults[i].type != IPT_END)
			{
				if (inputport_defaults[i].type == type)
				{
					/* load stored settings only if the default hasn't changed */
					if (input_key_seq_cmp(&inputport_defaults[i].keyboard,&def_keyboard)==0)
						input_key_seq_copy(&inputport_defaults[i].keyboard,&keyboard);
					if (input_joy_seq_cmp(&inputport_defaults[i].joystick,&def_joystick)==0)
						input_joy_seq_copy(&inputport_defaults[i].joystick,&joystick);
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
		osd_fwrite(f,MAMEDEFSTRING_V6,8);

		i = 0;
		while (inputport_defaults[i].type != IPT_END)
		{
			writeint(f,inputport_defaults[i].type);

			input_key_seq_write(f,&inputport_defaults_backup[i].keyboard);
			input_joy_seq_write(f,&inputport_defaults_backup[i].joystick);
			input_key_seq_write(f,&inputport_defaults[i].keyboard);
			input_joy_seq_write(f,&inputport_defaults[i].joystick);

			i++;
		}

		osd_fclose(f);
	}
	memcpy(inputport_defaults,inputport_defaults_backup,sizeof(inputport_defaults_backup));
}



static int readip(void *f,struct InputPort *in, int multikey, int andorkey)
{
	if (readint(f,&in->type) != 0)
		return -1;
	if (readword(f,&in->mask) != 0)
		return -1;
	if (readword(f,&in->default_value) != 0)
		return -1;
	if (input_key_seq_read(f,&in->keyboard,multikey,andorkey) != 0)
		return -1;
	if (input_joy_seq_read(f,&in->joystick,multikey,andorkey) != 0)
		return -1;

	return 0;
}

static void writeip(void *f,struct InputPort *in)
{
	writeint(f,in->type);
	writeword(f,in->mask);
	writeword(f,in->default_value);
	input_key_seq_write(f,&in->keyboard);
	input_joy_seq_write(f,&in->joystick);
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
		int multikey;
		int andorkey;

		in = Machine->input_ports_default;

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
		if (memcmp(buf,MAMECFGSTRING_V5,8) == 0)
		{
			multikey = 0;
			andorkey = 0;
		}
		else if (memcmp(buf,MAMECFGSTRING_V6,8) == 0)
		{
			multikey = 1;
			andorkey = 0;
		}
		else if (memcmp(buf,MAMECFGSTRING_V7,8) == 0)
		{
			multikey = 1;
			andorkey = 1;
		} else
			goto getout;	/* header invalid */

		/* read array size */
		if (readint(f,&savedtotal) != 0)
			goto getout;
		if (total != savedtotal)
			goto getout;	/* different size */

		/* read the original settings and compare them with the ones defined in the driver */
		in = Machine->input_ports_default;
		while (in->type != IPT_END)
		{
			struct InputPort saved;

			if (readip(f,&saved,multikey,andorkey) != 0)
				goto getout;

			if (in->mask != saved.mask ||
				in->default_value != saved.default_value ||
				in->type != saved.type ||
				input_key_seq_cmp(&in->keyboard,&saved.keyboard) !=0 ||
				input_joy_seq_cmp(&in->joystick,&saved.joystick) !=0 )
			goto getout;	/* the default values are different */

			in++;
		}

		/* read the current settings */
		in = Machine->input_ports;
		while (in->type != IPT_END)
		{
			if (readip(f,in,multikey,andorkey) != 0)
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

/***************************************************************************/
/* Save */

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


		in = Machine->input_ports_default;

		/* calculate the size of the array */
		total = 0;
		while (in->type != IPT_END)
		{
			total++;
			in++;
		}

		/* write header */
		osd_fwrite(f,MAMECFGSTRING_V7,8);
		/* write array size */
		writeint(f,total);
		/* write the original settings as defined in the driver */
		in = Machine->input_ports_default;
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

InputKeySeq* input_port_type_key_multi(int type)
{
	int i;


	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	return &inputport_defaults[i].keyboard;
}

InputJoySeq* input_port_type_joy_multi(int type)
{
	int i;


	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	return &inputport_defaults[i].joystick;
}

InputKeySeq* input_port_key_multi(const struct InputPort *in)
{
	int i,type;

	static InputKeySeq ip_key_none = INPUT_KEY_SEQ_DEF_1(IP_KEY_NONE);

	while (input_key_seq_get_1((InputKeySeq*)&in->keyboard) == IP_KEY_PREVIOUS) in--;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
	{
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if (((in-1)->type & IPF_UNUSED) || (!options.cheat && ((in-1)->type & IPF_CHEAT)))
			return &ip_key_none;
	}
	else
	{
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if ((in->type & IPF_UNUSED) || (!options.cheat && (in->type & IPF_CHEAT)))
			return &ip_key_none;
	}

	if (input_key_seq_get_1((InputKeySeq*)&in->keyboard) != IP_KEY_DEFAULT)
		return (InputKeySeq*)&in->keyboard;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return &inputport_defaults[i+1].keyboard;
	else
		return &inputport_defaults[i].keyboard;
}

InputJoySeq* input_port_joy_multi(const struct InputPort *in)
{
	int i,type;

	static InputJoySeq ip_joy_none = INPUT_JOY_SEQ_DEF_1(IP_JOY_NONE);

	while (input_joy_seq_get_1((InputJoySeq*)&in->joystick) == IP_JOY_PREVIOUS) in--;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
	{
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no joy */
		if (((in-1)->type & IPF_UNUSED) || (!options.cheat && ((in-1)->type & IPF_CHEAT)))
			return &ip_joy_none;
	}
	else
	{
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no joy */
		if ((in->type & IPF_UNUSED) || (!options.cheat && (in->type & IPF_CHEAT)))
			return &ip_joy_none;
	}

	if (input_joy_seq_get_1((InputJoySeq*)&in->joystick) != IP_JOY_DEFAULT)
		return (InputJoySeq*)&in->joystick;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return &inputport_defaults[i+1].joystick;
	else
		return &inputport_defaults[i].joystick;
}



void update_analog_port(int port)
{
	struct InputPort *in;
	int current, delta, type, sensitivity, clip, min, max, default_value;
	int axis, is_stick, check_bounds;
	InputKeySeq* inckey;
	InputKeySeq* deckey;
	InputJoySeq* incjoy;
	InputJoySeq* decjoy;
	int keydelta;
	int player;

	/* get input definition */
	in = input_analog[port];

	/* if we're not cheating and this is a cheat-only port, bail */
	if (!options.cheat && (in->type & IPF_CHEAT)) return;
	type=(in->type & ~IPF_MASK);

	deckey = input_port_key_multi(in);
	decjoy = input_port_joy_multi(in);
	inckey = input_port_key_multi(in+1);
	incjoy = input_port_joy_multi(in+1);

	keydelta = IP_GET_DELTA(in);

	switch (type)
	{
		case IPT_PADDLE:
			axis = X_AXIS; is_stick = 0; check_bounds = 1; break;
		case IPT_PADDLE_V:
			axis = Y_AXIS; is_stick = 0; check_bounds = 1; break;
		case IPT_DIAL:
			axis = X_AXIS; is_stick = 0; check_bounds = 0; break;
		case IPT_DIAL_V:
			axis = Y_AXIS; is_stick = 0; check_bounds = 0; break;
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

	if (keyboard_pressed_multi(deckey)) delta -= keydelta;
	if (joystick_pressed_multi(decjoy)) delta -= keydelta;

	if (type != IPT_PEDAL)
	{
		if (keyboard_pressed_multi(inckey)) delta += keydelta;
		if (joystick_pressed_multi(incjoy)) delta += keydelta;
	}
	else
	{
		/* is this cheesy or what? */
		if (!delta && input_key_seq_get_1(inckey) == KEYCODE_Y) delta += keydelta;
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
			delta = 100 / sensitivity;
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

static void scale_analog_port(int port)
{
	struct InputPort *in;
	int delta,current,sensitivity;

profiler_mark(PROFILER_INPUT);
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
profiler_mark(PROFILER_END);
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


profiler_mark(PROFILER_INPUT);

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
				InputKeySeq* key;
				InputJoySeq* joy;

				key = input_port_key_multi(in);
				joy = input_port_joy_multi(in);

				if ((input_key_seq_get_1(key) != 0 && input_key_seq_get_1(key) != IP_KEY_NONE) ||
					(input_joy_seq_get_1(joy) != 0 && input_joy_seq_get_1(joy) != IP_JOY_NONE))
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

					if (keyboard_pressed_multi(key) || joystick_pressed_multi(joy))
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
					InputKeySeq* key;
					InputJoySeq* joy;

					key = input_port_key_multi(in);
					joy = input_port_joy_multi(in);

					if (keyboard_pressed_multi(key) || joystick_pressed_multi(joy))
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
							cpu_set_reset_line(0,PULSE_LINE);

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

profiler_mark(PROFILER_END);
}



/* used the the CPU interface to notify that VBlank has ended, so we can update */
/* IPT_VBLANK input ports. */
void inputport_vblank_end(void)
{
	int port;
	int i;


profiler_mark(PROFILER_INPUT);
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
profiler_mark(PROFILER_END);
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

/***************************************************************************/
/* InputPort conversion */

static unsigned input_port_count(const struct InputPortTiny *src)
{
	unsigned total;

	total = 0;
	while (src->type != IPT_END)
	{
		int type = src->type & ~IPF_MASK;
		if (type > IPT_ANALOG_START && type < IPT_ANALOG_END)
			total += 2;
		else if (type != IPT_EXTENSION)
			++total;
		++src;
	}

	++total; /* for IPT_END */

	return total;
}

struct InputPort* input_port_allocate(const struct InputPortTiny *src)
{
	struct InputPort* dst;
	struct InputPort* base;
	unsigned total;

	total = input_port_count(src);

	base = (struct InputPort*)malloc(total * sizeof(struct InputPort));
	dst = base;

	while (src->type != IPT_END)
	{
		int type = src->type & ~IPF_MASK;
		const struct InputPortTiny *ext;
		const struct InputPortTiny *src_end;
		InputCode key_default;
		InputCode joy_default;

		if (type > IPT_ANALOG_START && type < IPT_ANALOG_END)
			src_end = src + 2;
		else
			src_end = src + 1;

		switch (type)
		{
			case IPT_END :
			case IPT_PORT :
			case IPT_DIPSWITCH_NAME :
			case IPT_DIPSWITCH_SETTING :
				key_default = IP_KEY_NONE;
				joy_default = IP_JOY_NONE;
			break;
			default:
				key_default = IP_KEY_DEFAULT;
				joy_default = IP_JOY_DEFAULT;
		}

		ext = src_end;
		while (src != src_end)
		{
			dst->type = src->type;
			dst->mask = src->mask;
			dst->default_value = src->default_value;
			dst->name = src->name;

			if (ext->type == IPT_EXTENSION)
			{
				input_key_seq_set_1(&dst->keyboard,IP_GET_KEY(ext));
				input_joy_seq_set_1(&dst->joystick,IP_GET_JOY(ext));
				++ext;
			} else
			{
				input_key_seq_set_1(&dst->keyboard,key_default);
				input_joy_seq_set_1(&dst->joystick,joy_default);
			}

			++src;
			++dst;
		}

		src = ext;
	}

	dst->type = IPT_END;

	return base;
}

void input_port_free(struct InputPort* dst)
{
	free(dst);
}

