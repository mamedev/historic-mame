//============================================================
//
//  input.c - Win32 implementation of MAME input routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <winioctl.h>
#include <ctype.h>

// undef WINNT for dinput.h to prevent duplicate definition
#undef WINNT
#include <dinput.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "rc.h"
#include "input.h"
#include "debugwin.h"


//============================================================
//  IMPORTS
//============================================================

extern int verbose;
extern int win_physical_width;
extern int win_physical_height;
extern int win_window_mode;


//============================================================
//  PARAMETERS
//============================================================

#define MAX_KEYBOARDS		1
#define MAX_MICE			8
#define MAX_JOYSTICKS		8
#define MAX_LIGHTGUNS		2

#define MAX_KEYS			256

#define MAX_JOY				512
#define MAX_AXES			8
#define MAX_BUTTONS			32
#define MAX_POV				4

#define HISTORY_LENGTH		16

enum
{
	ANALOG_TYPE_PADDLE = 0,
	ANALOG_TYPE_ADSTICK,
	ANALOG_TYPE_LIGHTGUN,
	ANALOG_TYPE_PEDAL,
	ANALOG_TYPE_DIAL,
	ANALOG_TYPE_TRACKBALL,
#ifdef MESS
	ANALOG_TYPE_MOUSE,
#endif // MESS
	ANALOG_TYPE_COUNT
};

enum
{
	SELECT_TYPE_KEYBOARD = 0,
	SELECT_TYPE_MOUSE,
	SELECT_TYPE_JOYSTICK,
	SELECT_TYPE_LIGHTGUN
};

enum
{
	AXIS_TYPE_INVALID = 0,
	AXIS_TYPE_DIGITAL,
	AXIS_TYPE_ANALOG
};



//============================================================
//  MACROS
//============================================================

#define STRUCTSIZE(x)		((dinput_version == 0x0300) ? sizeof(x##_DX3) : sizeof(x))

#define ELEMENTS(x)			(sizeof(x) / sizeof((x)[0]))



//============================================================
//  TYPEDEFS
//============================================================

struct axis_history
{
	LONG		value;
	INT32		count;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

UINT8						win_trying_to_quit;
int							win_use_mouse;



//============================================================
//  LOCAL VARIABLES
//============================================================

// this will be filled in dynamically
static struct OSCodeInfo	codelist[MAX_KEYS+MAX_JOY];
static int					total_codes;

// DirectInput variables
static LPDIRECTINPUT		dinput;
static int					dinput_version;

// global states
static int					input_paused;
static cycles_t				last_poll;

// Controller override options
static float				a2d_deadzone;
static int					use_joystick;
static int					use_lightgun;
static int					use_lightgun_dual;
static int					use_lightgun_reload;
static int					use_keyboard_leds;
static const char *			ledmode;
static int					steadykey;
static UINT8				analog_type[ANALOG_TYPE_COUNT];
static int					dummy[10];

// keyboard states
static int					keyboard_count;
static LPDIRECTINPUTDEVICE	keyboard_device[MAX_KEYBOARDS];
static LPDIRECTINPUTDEVICE2	keyboard_device2[MAX_KEYBOARDS];
static DIDEVCAPS			keyboard_caps[MAX_KEYBOARDS];
static BYTE					keyboard_state[MAX_KEYBOARDS][MAX_KEYS];

// additional key data
static INT8					oldkey[MAX_KEYS];
static INT8					currkey[MAX_KEYS];

// mouse states
static int					mouse_active;
static int					mouse_count;
static LPDIRECTINPUTDEVICE	mouse_device[MAX_MICE];
static LPDIRECTINPUTDEVICE2	mouse_device2[MAX_MICE];
static DIDEVCAPS			mouse_caps[MAX_MICE];
static DIMOUSESTATE			mouse_state[MAX_MICE];
static int					lightgun_count;
static POINT				lightgun_dual_player_pos[4];
static int					lightgun_dual_player_state[4];

// joystick states
static int					joystick_count;
static LPDIRECTINPUTDEVICE	joystick_device[MAX_JOYSTICKS];
static LPDIRECTINPUTDEVICE2	joystick_device2[MAX_JOYSTICKS];
static DIDEVCAPS			joystick_caps[MAX_JOYSTICKS];
static DIJOYSTATE			joystick_state[MAX_JOYSTICKS];
static DIPROPRANGE			joystick_range[MAX_JOYSTICKS][MAX_AXES];
static UINT8				joystick_digital[MAX_JOYSTICKS][MAX_AXES];
static char					joystick_name[MAX_JOYSTICKS][MAX_PATH];
static struct axis_history	joystick_history[MAX_JOYSTICKS][MAX_AXES][HISTORY_LENGTH];
static UINT8				joystick_type[MAX_JOYSTICKS][MAX_AXES];

// gun states
static INT32				gun_axis[MAX_LIGHTGUNS][2];

// led states
static int					original_leds;
static HANDLE				hKbdDev;
static int					ledmethod;



//============================================================
//  OPTIONS
//============================================================

// prototypes
static int decode_ledmode(struct rc_option *option, const char *arg, int priority);
static int decode_analog_select(struct rc_option *option, const char *arg, int priority);
static int decode_digital(struct rc_option *option, const char *arg, int priority);

// global input options
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input device options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "mouse", NULL, rc_bool, &win_use_mouse, "0", 0, 0, NULL, "enable mouse input" },
	{ "joystick", "joy", rc_bool, &use_joystick, "0", 0, 0, NULL, "enable joystick input" },
	{ "lightgun", "gun", rc_bool, &use_lightgun, "0", 0, 0, NULL, "enable lightgun input" },
	{ "dual_lightgun", "dual", rc_bool, &use_lightgun_dual, "0", 0, 0, NULL, "enable dual lightgun input" },
	{ "offscreen_reload", "reload", rc_bool, &use_lightgun_reload, "0", 0, 0, NULL, "offscreen shots reload" },
	{ "steadykey", "steady", rc_bool, &steadykey, "0", 0, 0, NULL, "enable steadykey support" },
	{ "keyboard_leds", "leds", rc_bool, &use_keyboard_leds, "1", 0, 0, NULL, "enable keyboard LED emulation" },
	{ "led_mode", NULL, rc_string, &ledmode, "ps/2", 0, 0, decode_ledmode, "LED mode (ps/2|usb)" },
	{ "a2d_deadzone", "a2d", rc_float, &a2d_deadzone, "0.3", 0.0, 1.0, NULL, "minimal analog value for digital input" },
	{ "ctrlr", NULL, rc_string, &options.controller, 0, 0, 0, NULL, "preconfigure for specified controller" },
	{ "paddle_device", "paddle", rc_string, &dummy[0], "keyboard", ANALOG_TYPE_PADDLE, 0, decode_analog_select, "enable (keyboard|mouse|joystick) if a paddle control is present" },
	{ "adstick_device", "adstick", rc_string, &dummy[1], "keyboard", ANALOG_TYPE_ADSTICK, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if an analog joystick control is present" },
	{ "pedal_device", "pedal", rc_string, &dummy[2], "keyboard", ANALOG_TYPE_PEDAL, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if a pedal control is present" },
	{ "dial_device", "dial", rc_string, &dummy[3], "keyboard", ANALOG_TYPE_DIAL, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if a dial control is present" },
	{ "trackball_device", "trackball", rc_string, &dummy[4], "keyboard", ANALOG_TYPE_TRACKBALL, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if a trackball control is present" },
	{ "lightgun_device", NULL, rc_string, &dummy[5], "keyboard", ANALOG_TYPE_LIGHTGUN, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if a lightgun control is present" },
#ifdef MESS
	{ "mouse_device", NULL, rc_string, &dummy[6], "mouse", ANALOG_TYPE_MOUSE, 0, decode_analog_select, "enable (keyboard|mouse|joystick|lightgun) if a mouse control is present" },
#endif // MESS
	{ "digital", NULL, rc_string, &dummy[7], "none", 1, 0, decode_digital, "mark certain joysticks or axes as digital (none|all|j<N>*|j<N>a<M>[,...])" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//  PROTOTYPES
//============================================================

static void updatekeyboard(void);
static void update_joystick_axes(void);
static void init_keycodes(void);
static void init_joycodes(void);
static void poll_lightguns(void);



//============================================================
//  KEYBOARD/JOYSTICK LIST
//============================================================

// macros for building/mapping keyboard codes
#define KEYCODE(dik, vk, ascii)		((dik) | ((vk) << 8) | ((ascii) << 16))
#define DICODE(keycode)				((keycode) & 0xff)
#define VKCODE(keycode)				(((keycode) >> 8) & 0xff)
#define ASCIICODE(keycode)			(((keycode) >> 16) & 0xff)

// macros for building/mapping joystick codes
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12) | 0x80000000)
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define CODETYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

// macros for differentiating the two
#define IS_KEYBOARD_CODE(code)		(((code) & 0x80000000) == 0)
#define IS_JOYSTICK_CODE(code)		(((code) & 0x80000000) != 0)

// joystick types
#define CODETYPE_KEYBOARD			0
#define CODETYPE_AXIS_NEG			1
#define CODETYPE_AXIS_POS			2
#define CODETYPE_POV_UP				3
#define CODETYPE_POV_DOWN			4
#define CODETYPE_POV_LEFT			5
#define CODETYPE_POV_RIGHT			6
#define CODETYPE_BUTTON				7
#define CODETYPE_JOYAXIS			8
#define CODETYPE_MOUSEAXIS			9
#define CODETYPE_MOUSEBUTTON		10
#define CODETYPE_GUNAXIS			11

// master keyboard translation table
const int win_key_trans_table[][4] =
{
	// MAME key             dinput key          virtual key     ascii
	{ KEYCODE_ESC, 			DIK_ESCAPE,			VK_ESCAPE,	 	27 },
	{ KEYCODE_1, 			DIK_1,				'1',			'1' },
	{ KEYCODE_2, 			DIK_2,				'2',			'2' },
	{ KEYCODE_3, 			DIK_3,				'3',			'3' },
	{ KEYCODE_4, 			DIK_4,				'4',			'4' },
	{ KEYCODE_5, 			DIK_5,				'5',			'5' },
	{ KEYCODE_6, 			DIK_6,				'6',			'6' },
	{ KEYCODE_7, 			DIK_7,				'7',			'7' },
	{ KEYCODE_8, 			DIK_8,				'8',			'8' },
	{ KEYCODE_9, 			DIK_9,				'9',			'9' },
	{ KEYCODE_0, 			DIK_0,				'0',			'0' },
	{ KEYCODE_MINUS, 		DIK_MINUS, 			0xbd,			'-' },
	{ KEYCODE_EQUALS, 		DIK_EQUALS,		 	0xbb,			'=' },
	{ KEYCODE_BACKSPACE,	DIK_BACK, 			VK_BACK, 		8 },
	{ KEYCODE_TAB, 			DIK_TAB, 			VK_TAB, 		9 },
	{ KEYCODE_Q, 			DIK_Q,				'Q',			'Q' },
	{ KEYCODE_W, 			DIK_W,				'W',			'W' },
	{ KEYCODE_E, 			DIK_E,				'E',			'E' },
	{ KEYCODE_R, 			DIK_R,				'R',			'R' },
	{ KEYCODE_T, 			DIK_T,				'T',			'T' },
	{ KEYCODE_Y, 			DIK_Y,				'Y',			'Y' },
	{ KEYCODE_U, 			DIK_U,				'U',			'U' },
	{ KEYCODE_I, 			DIK_I,				'I',			'I' },
	{ KEYCODE_O, 			DIK_O,				'O',			'O' },
	{ KEYCODE_P, 			DIK_P,				'P',			'P' },
	{ KEYCODE_OPENBRACE,	DIK_LBRACKET, 		0xdb,			'[' },
	{ KEYCODE_CLOSEBRACE,	DIK_RBRACKET, 		0xdd,			']' },
	{ KEYCODE_ENTER, 		DIK_RETURN, 		VK_RETURN, 		13 },
	{ KEYCODE_LCONTROL, 	DIK_LCONTROL, 		VK_CONTROL, 	0 },
	{ KEYCODE_A, 			DIK_A,				'A',			'A' },
	{ KEYCODE_S, 			DIK_S,				'S',			'S' },
	{ KEYCODE_D, 			DIK_D,				'D',			'D' },
	{ KEYCODE_F, 			DIK_F,				'F',			'F' },
	{ KEYCODE_G, 			DIK_G,				'G',			'G' },
	{ KEYCODE_H, 			DIK_H,				'H',			'H' },
	{ KEYCODE_J, 			DIK_J,				'J',			'J' },
	{ KEYCODE_K, 			DIK_K,				'K',			'K' },
	{ KEYCODE_L, 			DIK_L,				'L',			'L' },
	{ KEYCODE_COLON, 		DIK_SEMICOLON,		0xba,			';' },
	{ KEYCODE_QUOTE, 		DIK_APOSTROPHE,		0xde,			'\'' },
	{ KEYCODE_TILDE, 		DIK_GRAVE, 			0xc0,			'`' },
	{ KEYCODE_LSHIFT, 		DIK_LSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_BACKSLASH,	DIK_BACKSLASH, 		0xdc,			'\\' },
	{ KEYCODE_Z, 			DIK_Z,				'Z',			'Z' },
	{ KEYCODE_X, 			DIK_X,				'X',			'X' },
	{ KEYCODE_C, 			DIK_C,				'C',			'C' },
	{ KEYCODE_V, 			DIK_V,				'V',			'V' },
	{ KEYCODE_B, 			DIK_B,				'B',			'B' },
	{ KEYCODE_N, 			DIK_N,				'N',			'N' },
	{ KEYCODE_M, 			DIK_M,				'M',			'M' },
	{ KEYCODE_COMMA, 		DIK_COMMA,			0xbc,			',' },
	{ KEYCODE_STOP, 		DIK_PERIOD, 		0xbe,			'.' },
	{ KEYCODE_SLASH, 		DIK_SLASH, 			0xbf,			'/' },
	{ KEYCODE_RSHIFT, 		DIK_RSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_ASTERISK, 	DIK_MULTIPLY, 		VK_MULTIPLY,	'*' },
	{ KEYCODE_LALT, 		DIK_LMENU, 			VK_MENU, 		0 },
	{ KEYCODE_SPACE, 		DIK_SPACE, 			VK_SPACE,		' ' },
	{ KEYCODE_CAPSLOCK, 	DIK_CAPITAL, 		VK_CAPITAL, 	0 },
	{ KEYCODE_F1, 			DIK_F1,				VK_F1, 			0 },
	{ KEYCODE_F2, 			DIK_F2,				VK_F2, 			0 },
	{ KEYCODE_F3, 			DIK_F3,				VK_F3, 			0 },
	{ KEYCODE_F4, 			DIK_F4,				VK_F4, 			0 },
	{ KEYCODE_F5, 			DIK_F5,				VK_F5, 			0 },
	{ KEYCODE_F6, 			DIK_F6,				VK_F6, 			0 },
	{ KEYCODE_F7, 			DIK_F7,				VK_F7, 			0 },
	{ KEYCODE_F8, 			DIK_F8,				VK_F8, 			0 },
	{ KEYCODE_F9, 			DIK_F9,				VK_F9, 			0 },
	{ KEYCODE_F10, 			DIK_F10,			VK_F10, 		0 },
	{ KEYCODE_NUMLOCK, 		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ KEYCODE_SCRLOCK, 		DIK_SCROLL,			VK_SCROLL, 		0 },
	{ KEYCODE_7_PAD, 		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ KEYCODE_8_PAD, 		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ KEYCODE_9_PAD, 		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ KEYCODE_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT, 	0 },
	{ KEYCODE_4_PAD, 		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ KEYCODE_5_PAD, 		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ KEYCODE_6_PAD, 		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ KEYCODE_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ KEYCODE_1_PAD, 		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ KEYCODE_2_PAD, 		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ KEYCODE_3_PAD, 		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ KEYCODE_0_PAD, 		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ KEYCODE_DEL_PAD, 		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ KEYCODE_F11, 			DIK_F11,			VK_F11, 		0 },
	{ KEYCODE_F12, 			DIK_F12,			VK_F12, 		0 },
	{ KEYCODE_F13, 			DIK_F13,			VK_F13, 		0 },
	{ KEYCODE_F14, 			DIK_F14,			VK_F14, 		0 },
	{ KEYCODE_F15, 			DIK_F15,			VK_F15, 		0 },
	{ KEYCODE_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN, 		0 },
	{ KEYCODE_RCONTROL, 	DIK_RCONTROL,		VK_CONTROL, 	0 },
	{ KEYCODE_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE, 		0 },
	{ KEYCODE_PRTSCR, 		DIK_SYSRQ, 			0, 				0 },
	{ KEYCODE_RALT, 		DIK_RMENU,			VK_MENU, 		0 },
	{ KEYCODE_HOME, 		DIK_HOME,			VK_HOME, 		0 },
	{ KEYCODE_UP, 			DIK_UP,				VK_UP, 			0 },
	{ KEYCODE_PGUP, 		DIK_PRIOR,			VK_PRIOR, 		0 },
	{ KEYCODE_LEFT, 		DIK_LEFT,			VK_LEFT, 		0 },
	{ KEYCODE_RIGHT, 		DIK_RIGHT,			VK_RIGHT, 		0 },
	{ KEYCODE_END, 			DIK_END,			VK_END, 		0 },
	{ KEYCODE_DOWN, 		DIK_DOWN,			VK_DOWN, 		0 },
	{ KEYCODE_PGDN, 		DIK_NEXT,			VK_NEXT, 		0 },
	{ KEYCODE_INSERT, 		DIK_INSERT,			VK_INSERT, 		0 },
	{ KEYCODE_DEL, 			DIK_DELETE,			VK_DELETE, 		0 },
	{ KEYCODE_LWIN, 		DIK_LWIN,			VK_LWIN, 		0 },
	{ KEYCODE_RWIN, 		DIK_RWIN,			VK_RWIN, 		0 },
	{ KEYCODE_MENU, 		DIK_APPS,			VK_APPS, 		0 },
	{ -1 }
};


// master joystick translation table
static int joy_trans_table[][2] =
{
	// internal code                    MAME code
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 0),	JOYCODE_1_LEFT },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 0),	JOYCODE_1_RIGHT },
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 1),	JOYCODE_1_UP },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 1),	JOYCODE_1_DOWN },
	{ JOYCODE(0, CODETYPE_BUTTON, 0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_BUTTON, 1),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_BUTTON, 2),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_BUTTON, 3),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_BUTTON, 4),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_BUTTON, 5),	JOYCODE_1_BUTTON6 },
	{ JOYCODE(0, CODETYPE_BUTTON, 6),	JOYCODE_1_BUTTON7 },
	{ JOYCODE(0, CODETYPE_BUTTON, 7),	JOYCODE_1_BUTTON8 },
	{ JOYCODE(0, CODETYPE_BUTTON, 8),	JOYCODE_1_BUTTON9 },
	{ JOYCODE(0, CODETYPE_BUTTON, 9),	JOYCODE_1_BUTTON10 },
	{ JOYCODE(0, CODETYPE_BUTTON, 10),	JOYCODE_1_BUTTON11 },
	{ JOYCODE(0, CODETYPE_BUTTON, 11),	JOYCODE_1_BUTTON12 },
	{ JOYCODE(0, CODETYPE_BUTTON, 12),	JOYCODE_1_BUTTON13 },
	{ JOYCODE(0, CODETYPE_BUTTON, 13),	JOYCODE_1_BUTTON14 },
	{ JOYCODE(0, CODETYPE_BUTTON, 14),	JOYCODE_1_BUTTON15 },
	{ JOYCODE(0, CODETYPE_BUTTON, 15),	JOYCODE_1_BUTTON16 },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 0),	JOYCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 1),	JOYCODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 2),	JOYCODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_AXIS_NEG, 0),	JOYCODE_2_LEFT },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 0),	JOYCODE_2_RIGHT },
	{ JOYCODE(1, CODETYPE_AXIS_NEG, 1),	JOYCODE_2_UP },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 1),	JOYCODE_2_DOWN },
	{ JOYCODE(1, CODETYPE_BUTTON, 0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_BUTTON, 1),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_BUTTON, 2),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_BUTTON, 3),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_BUTTON, 4),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_BUTTON, 5),	JOYCODE_2_BUTTON6 },
	{ JOYCODE(1, CODETYPE_BUTTON, 6),	JOYCODE_2_BUTTON7 },
	{ JOYCODE(1, CODETYPE_BUTTON, 7),	JOYCODE_2_BUTTON8 },
	{ JOYCODE(1, CODETYPE_BUTTON, 8),	JOYCODE_2_BUTTON9 },
	{ JOYCODE(1, CODETYPE_BUTTON, 9),	JOYCODE_2_BUTTON10 },
	{ JOYCODE(1, CODETYPE_BUTTON, 10),	JOYCODE_2_BUTTON11 },
	{ JOYCODE(1, CODETYPE_BUTTON, 11),	JOYCODE_2_BUTTON12 },
	{ JOYCODE(1, CODETYPE_BUTTON, 12),	JOYCODE_2_BUTTON13 },
	{ JOYCODE(1, CODETYPE_BUTTON, 13),	JOYCODE_2_BUTTON14 },
	{ JOYCODE(1, CODETYPE_BUTTON, 14),	JOYCODE_2_BUTTON15 },
	{ JOYCODE(1, CODETYPE_BUTTON, 15),	JOYCODE_2_BUTTON16 },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 0),	JOYCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 1),	JOYCODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 2),	JOYCODE_2_ANALOG_Z },

	{ JOYCODE(2, CODETYPE_AXIS_NEG, 0),	JOYCODE_3_LEFT },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 0),	JOYCODE_3_RIGHT },
	{ JOYCODE(2, CODETYPE_AXIS_NEG, 1),	JOYCODE_3_UP },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 1),	JOYCODE_3_DOWN },
	{ JOYCODE(2, CODETYPE_BUTTON, 0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(2, CODETYPE_BUTTON, 1),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(2, CODETYPE_BUTTON, 2),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(2, CODETYPE_BUTTON, 3),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(2, CODETYPE_BUTTON, 4),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(2, CODETYPE_BUTTON, 5),	JOYCODE_3_BUTTON6 },
	{ JOYCODE(2, CODETYPE_BUTTON, 6),	JOYCODE_3_BUTTON7 },
	{ JOYCODE(2, CODETYPE_BUTTON, 7),	JOYCODE_3_BUTTON8 },
	{ JOYCODE(2, CODETYPE_BUTTON, 8),	JOYCODE_3_BUTTON9 },
	{ JOYCODE(2, CODETYPE_BUTTON, 9),	JOYCODE_3_BUTTON10 },
	{ JOYCODE(2, CODETYPE_BUTTON, 10),	JOYCODE_3_BUTTON11 },
	{ JOYCODE(2, CODETYPE_BUTTON, 11),	JOYCODE_3_BUTTON12 },
	{ JOYCODE(2, CODETYPE_BUTTON, 12),	JOYCODE_3_BUTTON13 },
	{ JOYCODE(2, CODETYPE_BUTTON, 13),	JOYCODE_3_BUTTON14 },
	{ JOYCODE(2, CODETYPE_BUTTON, 14),	JOYCODE_3_BUTTON15 },
	{ JOYCODE(2, CODETYPE_BUTTON, 15),	JOYCODE_3_BUTTON16 },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 0),	JOYCODE_3_ANALOG_X },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 1),	JOYCODE_3_ANALOG_Y },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 2),	JOYCODE_3_ANALOG_Z },

	{ JOYCODE(3, CODETYPE_AXIS_NEG, 0),	JOYCODE_4_LEFT },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 0),	JOYCODE_4_RIGHT },
	{ JOYCODE(3, CODETYPE_AXIS_NEG, 1),	JOYCODE_4_UP },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 1),	JOYCODE_4_DOWN },
	{ JOYCODE(3, CODETYPE_BUTTON, 0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(3, CODETYPE_BUTTON, 1),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(3, CODETYPE_BUTTON, 2),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(3, CODETYPE_BUTTON, 3),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(3, CODETYPE_BUTTON, 4),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(3, CODETYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 },
	{ JOYCODE(3, CODETYPE_BUTTON, 6),	JOYCODE_4_BUTTON7 },
	{ JOYCODE(3, CODETYPE_BUTTON, 7),	JOYCODE_4_BUTTON8 },
	{ JOYCODE(3, CODETYPE_BUTTON, 8),	JOYCODE_4_BUTTON9 },
	{ JOYCODE(3, CODETYPE_BUTTON, 9),	JOYCODE_4_BUTTON10 },
	{ JOYCODE(3, CODETYPE_BUTTON, 10),	JOYCODE_4_BUTTON11 },
	{ JOYCODE(3, CODETYPE_BUTTON, 11),	JOYCODE_4_BUTTON12 },
	{ JOYCODE(3, CODETYPE_BUTTON, 12),	JOYCODE_4_BUTTON13 },
	{ JOYCODE(3, CODETYPE_BUTTON, 13),	JOYCODE_4_BUTTON14 },
	{ JOYCODE(3, CODETYPE_BUTTON, 14),	JOYCODE_4_BUTTON15 },
	{ JOYCODE(3, CODETYPE_BUTTON, 15),	JOYCODE_4_BUTTON16 },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 0),	JOYCODE_4_ANALOG_X },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 1),	JOYCODE_4_ANALOG_Y },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 2),	JOYCODE_4_ANALOG_Z },

	{ JOYCODE(4, CODETYPE_AXIS_NEG, 0),	JOYCODE_5_LEFT },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 0),	JOYCODE_5_RIGHT },
	{ JOYCODE(4, CODETYPE_AXIS_NEG, 1),	JOYCODE_5_UP },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 1),	JOYCODE_5_DOWN },
	{ JOYCODE(4, CODETYPE_BUTTON, 0),	JOYCODE_5_BUTTON1 },
	{ JOYCODE(4, CODETYPE_BUTTON, 1),	JOYCODE_5_BUTTON2 },
	{ JOYCODE(4, CODETYPE_BUTTON, 2),	JOYCODE_5_BUTTON3 },
	{ JOYCODE(4, CODETYPE_BUTTON, 3),	JOYCODE_5_BUTTON4 },
	{ JOYCODE(4, CODETYPE_BUTTON, 4),	JOYCODE_5_BUTTON5 },
	{ JOYCODE(4, CODETYPE_BUTTON, 5),	JOYCODE_5_BUTTON6 },
	{ JOYCODE(4, CODETYPE_BUTTON, 6),	JOYCODE_5_BUTTON7 },
	{ JOYCODE(4, CODETYPE_BUTTON, 7),	JOYCODE_5_BUTTON8 },
	{ JOYCODE(4, CODETYPE_BUTTON, 8),	JOYCODE_5_BUTTON9 },
	{ JOYCODE(4, CODETYPE_BUTTON, 9),	JOYCODE_5_BUTTON10 },
	{ JOYCODE(4, CODETYPE_BUTTON, 10),	JOYCODE_5_BUTTON11 },
	{ JOYCODE(4, CODETYPE_BUTTON, 11),	JOYCODE_5_BUTTON12 },
	{ JOYCODE(4, CODETYPE_BUTTON, 12),	JOYCODE_5_BUTTON13 },
	{ JOYCODE(4, CODETYPE_BUTTON, 13),	JOYCODE_5_BUTTON14 },
	{ JOYCODE(4, CODETYPE_BUTTON, 14),	JOYCODE_5_BUTTON15 },
	{ JOYCODE(4, CODETYPE_BUTTON, 15),	JOYCODE_5_BUTTON16 },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 0),	JOYCODE_5_ANALOG_X },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 1), 	JOYCODE_5_ANALOG_Y },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 2),	JOYCODE_5_ANALOG_Z },

	{ JOYCODE(5, CODETYPE_AXIS_NEG, 0),	JOYCODE_6_LEFT },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 0),	JOYCODE_6_RIGHT },
	{ JOYCODE(5, CODETYPE_AXIS_NEG, 1),	JOYCODE_6_UP },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 1),	JOYCODE_6_DOWN },
	{ JOYCODE(5, CODETYPE_BUTTON, 0),	JOYCODE_6_BUTTON1 },
	{ JOYCODE(5, CODETYPE_BUTTON, 1),	JOYCODE_6_BUTTON2 },
	{ JOYCODE(5, CODETYPE_BUTTON, 2),	JOYCODE_6_BUTTON3 },
	{ JOYCODE(5, CODETYPE_BUTTON, 3),	JOYCODE_6_BUTTON4 },
	{ JOYCODE(5, CODETYPE_BUTTON, 4),	JOYCODE_6_BUTTON5 },
	{ JOYCODE(5, CODETYPE_BUTTON, 5),	JOYCODE_6_BUTTON6 },
	{ JOYCODE(5, CODETYPE_BUTTON, 6),	JOYCODE_6_BUTTON7 },
	{ JOYCODE(5, CODETYPE_BUTTON, 7),	JOYCODE_6_BUTTON8 },
	{ JOYCODE(5, CODETYPE_BUTTON, 8),	JOYCODE_6_BUTTON9 },
	{ JOYCODE(5, CODETYPE_BUTTON, 9),	JOYCODE_6_BUTTON10 },
	{ JOYCODE(5, CODETYPE_BUTTON, 10),	JOYCODE_6_BUTTON11 },
	{ JOYCODE(5, CODETYPE_BUTTON, 11),	JOYCODE_6_BUTTON12 },
	{ JOYCODE(5, CODETYPE_BUTTON, 12),	JOYCODE_6_BUTTON13 },
	{ JOYCODE(5, CODETYPE_BUTTON, 13),	JOYCODE_6_BUTTON14 },
	{ JOYCODE(5, CODETYPE_BUTTON, 14),	JOYCODE_6_BUTTON15 },
	{ JOYCODE(5, CODETYPE_BUTTON, 15),	JOYCODE_6_BUTTON16 },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 0),	JOYCODE_6_ANALOG_X },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 1),	JOYCODE_6_ANALOG_Y },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 2),	JOYCODE_6_ANALOG_Z },

	{ JOYCODE(6, CODETYPE_AXIS_NEG, 0),	JOYCODE_7_LEFT },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 0),	JOYCODE_7_RIGHT },
	{ JOYCODE(6, CODETYPE_AXIS_NEG, 1),	JOYCODE_7_UP },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 1),	JOYCODE_7_DOWN },
	{ JOYCODE(6, CODETYPE_BUTTON, 0),	JOYCODE_7_BUTTON1 },
	{ JOYCODE(6, CODETYPE_BUTTON, 1),	JOYCODE_7_BUTTON2 },
	{ JOYCODE(6, CODETYPE_BUTTON, 2),	JOYCODE_7_BUTTON3 },
	{ JOYCODE(6, CODETYPE_BUTTON, 3),	JOYCODE_7_BUTTON4 },
	{ JOYCODE(6, CODETYPE_BUTTON, 4),	JOYCODE_7_BUTTON5 },
	{ JOYCODE(6, CODETYPE_BUTTON, 5),	JOYCODE_7_BUTTON6 },
	{ JOYCODE(6, CODETYPE_BUTTON, 6),	JOYCODE_7_BUTTON7 },
	{ JOYCODE(6, CODETYPE_BUTTON, 7),	JOYCODE_7_BUTTON8 },
	{ JOYCODE(6, CODETYPE_BUTTON, 8),	JOYCODE_7_BUTTON9 },
	{ JOYCODE(6, CODETYPE_BUTTON, 9),	JOYCODE_7_BUTTON10 },
	{ JOYCODE(6, CODETYPE_BUTTON, 10),	JOYCODE_7_BUTTON11 },
	{ JOYCODE(6, CODETYPE_BUTTON, 11),	JOYCODE_7_BUTTON12 },
	{ JOYCODE(6, CODETYPE_BUTTON, 12),	JOYCODE_7_BUTTON13 },
	{ JOYCODE(6, CODETYPE_BUTTON, 13),	JOYCODE_7_BUTTON14 },
	{ JOYCODE(6, CODETYPE_BUTTON, 14),	JOYCODE_7_BUTTON15 },
	{ JOYCODE(6, CODETYPE_BUTTON, 15),	JOYCODE_7_BUTTON16 },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 0),	JOYCODE_7_ANALOG_X },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 1),	JOYCODE_7_ANALOG_Y },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 2),	JOYCODE_7_ANALOG_Z },

	{ JOYCODE(7, CODETYPE_AXIS_NEG, 0),	JOYCODE_8_LEFT },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 0),	JOYCODE_8_RIGHT },
	{ JOYCODE(7, CODETYPE_AXIS_NEG, 1),	JOYCODE_8_UP },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 1),	JOYCODE_8_DOWN },
	{ JOYCODE(7, CODETYPE_BUTTON, 0),	JOYCODE_8_BUTTON1 },
	{ JOYCODE(7, CODETYPE_BUTTON, 1),	JOYCODE_8_BUTTON2 },
	{ JOYCODE(7, CODETYPE_BUTTON, 2),	JOYCODE_8_BUTTON3 },
	{ JOYCODE(7, CODETYPE_BUTTON, 3),	JOYCODE_8_BUTTON4 },
	{ JOYCODE(7, CODETYPE_BUTTON, 4),	JOYCODE_8_BUTTON5 },
	{ JOYCODE(7, CODETYPE_BUTTON, 5),	JOYCODE_8_BUTTON6 },
	{ JOYCODE(7, CODETYPE_BUTTON, 6),	JOYCODE_8_BUTTON7 },
	{ JOYCODE(7, CODETYPE_BUTTON, 7),	JOYCODE_8_BUTTON8 },
	{ JOYCODE(7, CODETYPE_BUTTON, 8),	JOYCODE_8_BUTTON9 },
	{ JOYCODE(7, CODETYPE_BUTTON, 9),	JOYCODE_8_BUTTON10 },
	{ JOYCODE(7, CODETYPE_BUTTON, 10),	JOYCODE_8_BUTTON11 },
	{ JOYCODE(7, CODETYPE_BUTTON, 11),	JOYCODE_8_BUTTON12 },
	{ JOYCODE(7, CODETYPE_BUTTON, 12),	JOYCODE_8_BUTTON13 },
	{ JOYCODE(7, CODETYPE_BUTTON, 13),	JOYCODE_8_BUTTON14 },
	{ JOYCODE(7, CODETYPE_BUTTON, 14),	JOYCODE_8_BUTTON15 },
	{ JOYCODE(7, CODETYPE_BUTTON, 15),	JOYCODE_8_BUTTON16 },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 0),	JOYCODE_8_ANALOG_X },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 1),	JOYCODE_8_ANALOG_Y },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 2),	JOYCODE_8_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_2_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_GUNAXIS, 0),		GUNCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_GUNAXIS, 1),		GUNCODE_1_ANALOG_Y },

	{ JOYCODE(1, CODETYPE_GUNAXIS, 0),		GUNCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_GUNAXIS, 1),		GUNCODE_2_ANALOG_Y },
};



//============================================================
//  decode_ledmode
//============================================================

static int decode_ledmode(struct rc_option *option, const char *arg, int priority)
{
	if( strcmp( arg, "ps/2" ) != 0 &&
		strcmp( arg, "usb" ) != 0 )
	{
		fprintf(stderr, "error: invalid value for led_mode: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}



//============================================================
//  decode_analog_select
//============================================================

static int decode_analog_select(struct rc_option *option, const char *arg, int priority)
{
	if (strcmp(arg, "keyboard") == 0)
		analog_type[(int)option->min] = SELECT_TYPE_KEYBOARD;
	else if (strcmp(arg, "mouse") == 0)
		analog_type[(int)option->min] = SELECT_TYPE_MOUSE;
	else if (strcmp(arg, "joystick") == 0)
		analog_type[(int)option->min] = SELECT_TYPE_JOYSTICK;
	else if (strcmp(arg, "lightgun") == 0)
		analog_type[(int)option->min] = SELECT_TYPE_LIGHTGUN;
	else
	{
		fprintf(stderr, "error: invalid value for %s: %s\n", option->name, arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}



//============================================================
//  decode_digital
//============================================================

static int decode_digital(struct rc_option *option, const char *arg, int priority)
{
	if (strcmp(arg, "none") == 0)
		memset(joystick_digital, 0, sizeof(joystick_digital));
	else if (strcmp(arg, "all") == 0)
		memset(joystick_digital, 1, sizeof(joystick_digital));
	else
	{
		/* scan the string */
		while (1)
		{
			int joynum = 0;
			int axisnum = 0;

			/* stop if we hit the end */
			if (arg[0] == 0)
				break;

			/* we require the next bits to be j<N> */
			if (tolower(arg[0]) != 'j' || sscanf(&arg[1], "%d", &joynum) != 1)
				goto usage;
			arg++;
			while (arg[0] != 0 && isdigit(arg[0]))
				arg++;

			/* if we are followed by a comma or an end, mark all the axes digital */
			if (arg[0] == 0 || arg[0] == ',')
			{
				if (joynum != 0 && joynum - 1 < MAX_JOYSTICKS)
					memset(&joystick_digital[joynum - 1], 1, sizeof(joystick_digital[joynum - 1]));
				if (arg[0] == 0)
					break;
				arg++;
				continue;
			}

			/* loop over axes */
			while (1)
			{
				/* stop if we hit the end */
				if (arg[0] == 0)
					break;

				/* if we hit a comma, skip it and break out */
				if (arg[0] == ',')
				{
					arg++;
					break;
				}

				/* we require the next bits to be a<N> */
				if (tolower(arg[0]) != 'a' || sscanf(&arg[1], "%d", &axisnum) != 1)
					goto usage;
				arg++;
				while (arg[0] != 0 && isdigit(arg[0]))
					arg++;

				/* set that axis to digital */
				if (joynum != 0 && joynum - 1 < MAX_JOYSTICKS && axisnum < MAX_AXES)
					joystick_digital[joynum - 1][axisnum] = 1;
			}
		}
	}
	option->priority = priority;
	return 0;

usage:
	fprintf(stderr, "error: invalid value for digital: %s -- valid values are:\n", arg);
	fprintf(stderr, "         none -- no axes on any joysticks are digital\n");
	fprintf(stderr, "         all -- all axes on all joysticks are digital\n");
	fprintf(stderr, "         j<N> -- all axes on joystick <N> are digital\n");
	fprintf(stderr, "         j<N>a<M> -- axis <M> on joystick <N> is digital\n");
	fprintf(stderr, "    Multiple axes can be specified for one joystick:\n");
	fprintf(stderr, "         j1a5a6 -- axes 5 and 6 on joystick 1 are digital\n");
	fprintf(stderr, "    Multiple joysticks can be specified separated by commas:\n");
	fprintf(stderr, "         j1,j2a2 -- all joystick 1 axes and axis 2 on joystick 2 are digital\n");
	return -1;
}



//============================================================
//  autoselect_analog_devices
//============================================================

static void autoselect_analog_devices(const struct InputPort *inp, int type1, int type2, int type3, int anatype, const char *ananame)
{
	// loop over input ports
	for ( ; inp->type != IPT_END; inp++)

		// if this port type is in use, apply the autoselect criteria
		if ((type1 != 0 && inp->type == type1) ||
			(type2 != 0 && inp->type == type2) ||
			(type3 != 0 && inp->type == type3))
		{
			// autoenable mouse devices
			if (analog_type[anatype] == SELECT_TYPE_MOUSE && !win_use_mouse)
			{
				win_use_mouse = 1;
				if (verbose)
					printf("Autoenabling mice due to presence of a %s\n", ananame);
			}

			// autoenable joystick devices
			if (analog_type[anatype] == SELECT_TYPE_JOYSTICK && !use_joystick)
			{
				use_joystick = 1;
				if (verbose)
					printf("Autoenabling joysticks due to presence of a %s\n", ananame);
			}

			// autoenable lightgun devices
			if (analog_type[anatype] == SELECT_TYPE_LIGHTGUN && !use_lightgun)
			{
				use_lightgun = 1;
				if (verbose)
					printf("Autoenabling lightguns due to presence of a %s\n", ananame);
			}

			// all done
			break;
		}
}



//============================================================
//  enum_keyboard_callback
//============================================================

static BOOL CALLBACK enum_keyboard_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	HRESULT result;

	// if we're not out of mice, log this one
	if (keyboard_count >= MAX_KEYBOARDS)
		goto out_of_keyboards;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &keyboard_device[keyboard_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(keyboard_device[keyboard_count], &IID_IDirectInputDevice2, (void **)&keyboard_device2[keyboard_count]);
	if (result != DI_OK)
		keyboard_device2[keyboard_count] = NULL;

	// get the caps
	keyboard_caps[keyboard_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(keyboard_device[keyboard_count], &keyboard_caps[keyboard_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(keyboard_device[keyboard_count], &c_dfDIKeyboard);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	result = IDirectInputDevice_SetCooperativeLevel(keyboard_device[keyboard_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	keyboard_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_get_caps:
	if (keyboard_device2[keyboard_count])
		IDirectInputDevice_Release(keyboard_device2[keyboard_count]);
	IDirectInputDevice_Release(keyboard_device[keyboard_count]);
cant_create_device:
out_of_keyboards:
	return DIENUM_CONTINUE;
}



//============================================================
//  enum_mouse_callback
//============================================================

static BOOL CALLBACK enum_mouse_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result;

	// if we're not out of mice, log this one
	if (mouse_count >= MAX_MICE)
		goto out_of_mice;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &mouse_device[mouse_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(mouse_device[mouse_count], &IID_IDirectInputDevice2, (void **)&mouse_device2[mouse_count]);
	if (result != DI_OK)
		mouse_device2[mouse_count] = NULL;

	// get the caps
	mouse_caps[mouse_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(mouse_device[mouse_count], &mouse_caps[mouse_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set relative mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_REL;
	result = IDirectInputDevice_SetProperty(mouse_device[mouse_count], DIPROP_AXISMODE, &value.diph);
	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(mouse_device[mouse_count], &c_dfDIMouse);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	if (use_lightgun)
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	else
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);

	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	if (use_lightgun)
		lightgun_count++;
	mouse_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (mouse_device2[mouse_count])
		IDirectInputDevice_Release(mouse_device2[mouse_count]);
	IDirectInputDevice_Release(mouse_device[mouse_count]);
cant_create_device:
out_of_mice:
	return DIENUM_CONTINUE;
}



//============================================================
//  enum_joystick_callback
//============================================================

static BOOL CALLBACK enum_joystick_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result = DI_OK;
	DWORD flags;

	// if we're not out of mice, log this one
	if (joystick_count >= MAX_JOYSTICKS)
		goto out_of_joysticks;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &joystick_device[joystick_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(joystick_device[joystick_count], &IID_IDirectInputDevice2, (void **)&joystick_device2[joystick_count]);
	if (result != DI_OK)
		joystick_device2[joystick_count] = NULL;

	// remember the name
	strcpy(joystick_name[joystick_count], instance->tszInstanceName);

	// get the caps
	joystick_caps[joystick_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(joystick_device[joystick_count], &joystick_caps[joystick_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set absolute mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_ABS;
	result = IDirectInputDevice_SetProperty(joystick_device[joystick_count], DIPROP_AXISMODE, &value.diph);
 	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(joystick_device[joystick_count], &c_dfDIJoystick);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
#if HAS_WINDOW_MENU
	flags = DISCL_BACKGROUND | DISCL_EXCLUSIVE;
#else
	flags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
#endif
	result = IDirectInputDevice_SetCooperativeLevel(joystick_device[joystick_count], win_video_window, flags);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	joystick_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (joystick_device2[joystick_count])
		IDirectInputDevice_Release(joystick_device2[joystick_count]);
	IDirectInputDevice_Release(joystick_device[joystick_count]);
cant_create_device:
out_of_joysticks:
	return DIENUM_CONTINUE;
}



//============================================================
//  win_init_input
//============================================================

int win_init_input(void)
{
	const struct InputPort *inp;
	HRESULT result;

	// first attempt to initialize DirectInput
	dinput_version = DIRECTINPUT_VERSION;
	result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
	if (result != DI_OK)
	{
		dinput_version = 0x0300;
		result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
		if (result != DI_OK)
			goto cant_create_dinput;
	}
	if (verbose)
		fprintf(stderr, "Using DirectInput %d\n", dinput_version >> 8);

	// enable devices based on autoselect
	if (Machine != NULL && Machine->gamedrv != NULL)
	{
		begin_resource_tracking();
		inp = input_port_allocate(Machine->gamedrv->construct_ipt);
		autoselect_analog_devices(inp, IPT_PADDLE,     IPT_PADDLE_V,   0,             ANALOG_TYPE_PADDLE,   "paddle");
		autoselect_analog_devices(inp, IPT_AD_STICK_X, IPT_AD_STICK_Y, IPT_AD_STICK_Z,ANALOG_TYPE_ADSTICK,  "analog joystick");
		autoselect_analog_devices(inp, IPT_LIGHTGUN_X, IPT_LIGHTGUN_Y, 0,             ANALOG_TYPE_LIGHTGUN, "lightgun");
		autoselect_analog_devices(inp, IPT_PEDAL,      IPT_PEDAL2,     IPT_PEDAL3,    ANALOG_TYPE_PEDAL,    "pedal");
		autoselect_analog_devices(inp, IPT_DIAL,       IPT_DIAL_V,     0,             ANALOG_TYPE_DIAL,     "dial");
		autoselect_analog_devices(inp, IPT_TRACKBALL_X,IPT_TRACKBALL_Y,0,             ANALOG_TYPE_TRACKBALL,"trackball");
#ifdef MESS
		autoselect_analog_devices(inp, IPT_MOUSE_X,    IPT_MOUSE_Y,    0,             ANALOG_TYPE_MOUSE,    "mouse");
#endif // MESS
		end_resource_tracking();
	}

	// initialize keyboard devices
	keyboard_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_KEYBOARD, enum_keyboard_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_keyboard;

	// initialize mouse devices
	lightgun_count = 0;
	mouse_count = 0;
	if (win_use_mouse || use_lightgun)
	{
		lightgun_dual_player_state[0] = lightgun_dual_player_state[1] = 0;
		lightgun_dual_player_state[2] = lightgun_dual_player_state[3] = 0;
		result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_MOUSE, enum_mouse_callback, 0, DIEDFL_ATTACHEDONLY);
		if (result != DI_OK)
			goto cant_init_mouse;

		// if we have at least one mouse, and the "Dual" option is selected,
		//  then the lightgun_count is 2 (The two guns are read as a single
		//  4-button mouse).
		if (mouse_count && use_lightgun_dual && lightgun_count < 2)
			lightgun_count = 2;
	}

	// initialize joystick devices
	joystick_count = 0;
	if (use_joystick)
	{
		result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_JOYSTICK, enum_joystick_callback, 0, DIEDFL_ATTACHEDONLY);
		if (result != DI_OK)
			goto cant_init_joystick;
	}

	total_codes = 0;

	// init the keyboard list
	init_keycodes();

	// init the joystick list
	init_joycodes();

	// terminate array
	memset(&codelist[total_codes], 0, sizeof(codelist[total_codes]));

	// print the results
	if (verbose)
	{
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d  Lightguns=%d\n", keyboard_count, mouse_count, joystick_count, lightgun_count);
		if (options.controller)
			fprintf(stderr,"\"%s\" controller support enabled\n", options.controller);
	}
	return 0;

cant_init_joystick:
cant_init_mouse:
cant_init_keyboard:
	IDirectInput_Release(dinput);
cant_create_dinput:
	dinput = NULL;
	return 1;
}



//============================================================
//  win_shutdown_input
//============================================================

void win_shutdown_input(void)
{
	int i;

	// release all our keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		IDirectInputDevice_Release(keyboard_device[i]);
		if (keyboard_device2[i])
			IDirectInputDevice_Release(keyboard_device2[i]);
		keyboard_device2[i]=0;
	}

	// release all our joysticks
	for (i = 0; i < joystick_count; i++)
	{
		IDirectInputDevice_Release(joystick_device[i]);
		if (joystick_device2[i])
			IDirectInputDevice_Release(joystick_device2[i]);
		joystick_device2[i]=0;
	}

	// release all our mice
	for (i = 0; i < mouse_count; i++)
	{
		IDirectInputDevice_Release(mouse_device[i]);
		if (mouse_device2[i])
			IDirectInputDevice_Release(mouse_device2[i]);
		mouse_device2[i]=0;
	}

	// free allocated strings
	for (i = 0; i < total_codes; i++)
	{
		free(codelist[i].name);
		codelist[i].name = NULL;
	}

	// release DirectInput
	if (dinput)
		IDirectInput_Release(dinput);
	dinput = NULL;
}



//============================================================
//  win_pause_input
//============================================================

void win_pause_input(int paused)
{
	int i;

	// if paused, unacquire all devices
	if (paused)
	{
		// unacquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Unacquire(keyboard_device[i]);

		// unacquire all our mice
		for (i = 0; i < mouse_count; i++)
			IDirectInputDevice_Unacquire(mouse_device[i]);
	}

	// otherwise, reacquire all devices
	else
	{
		// acquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Acquire(keyboard_device[i]);

		// acquire all our mice if active
		if (mouse_active && !win_has_menu())
			for (i = 0; i < mouse_count && (win_use_mouse || use_lightgun); i++)
				IDirectInputDevice_Acquire(mouse_device[i]);
	}

	// set the paused state
	input_paused = paused;
	win_update_cursor_state();
}



//============================================================
//  win_poll_input
//============================================================

void win_poll_input(void)
{
	HWND focus = GetFocus();
	HRESULT result = 1;
	int i, j;

	// remember when this happened
	last_poll = osd_cycles();

	// periodically process events, in case they're not coming through
	win_process_events_periodic();

	// if we don't have focus, turn off all keys
	if (!focus)
	{
		memset(&keyboard_state[0][0], 0, sizeof(keyboard_state[i]));
		updatekeyboard();
		return;
	}

	// poll all keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		// first poll the device
		if (keyboard_device2[i])
			IDirectInputDevice2_Poll(keyboard_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);

		// handle lost inputs here
		if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
		{
			result = IDirectInputDevice_Acquire(keyboard_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);
		}

		// convert to 0 or 1
		if (result == DI_OK)
			for (j = 0; j < sizeof(keyboard_state[i]); j++)
				keyboard_state[i][j] >>= 7;
	}

	// if we couldn't poll the keyboard that way, poll it via GetAsyncKeyState
	if (result != DI_OK)
		for (i = 0; codelist[i].oscode; i++)
			if (IS_KEYBOARD_CODE(codelist[i].oscode))
			{
				int dik = DICODE(codelist[i].oscode);
				int vk = VKCODE(codelist[i].oscode);

				// if we have a non-zero VK, query it
				if (vk)
					keyboard_state[0][dik] = (GetAsyncKeyState(vk) >> 15) & 1;
			}

	// update the lagged keyboard
	updatekeyboard();

#ifndef NEW_DEBUGGER
	// if the debugger is up and visible, don't bother with the rest
	if (win_debug_window != NULL && IsWindowVisible(win_debug_window))
		return;
#endif

	// poll all joysticks
	for (i = 0; i < joystick_count; i++)
	{
		// first poll the device
		if (joystick_device2[i])
			IDirectInputDevice2_Poll(joystick_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);

		// handle lost inputs here
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
		{
			result = IDirectInputDevice_Acquire(joystick_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);
		}
	}

	// update joystick axis history
	update_joystick_axes();

	// poll all our mice if active
	if (mouse_active && !win_has_menu())
		for (i = 0; i < mouse_count && (win_use_mouse||use_lightgun); i++)
		{
			// first poll the device
			if (mouse_device2[i])
				IDirectInputDevice2_Poll(mouse_device2[i]);

			// get the state
			result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);

			// handle lost inputs here
			if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
			{
				result = IDirectInputDevice_Acquire(mouse_device[i]);
				if (result == DI_OK)
					result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);
			}
		}

	// poll the lightguns
	poll_lightguns();
}



//============================================================
//  is_mouse_captured
//============================================================

int win_is_mouse_captured(void)
{
	return (!input_paused && mouse_active && mouse_count > 0 && win_use_mouse && !win_has_menu());
}



//============================================================
//  updatekeyboard
//============================================================

// since the keyboard controller is slow, it is not capable of reporting multiple
// key presses fast enough. We have to delay them in order not to lose special moves
// tied to simultaneous button presses.

static void updatekeyboard(void)
{
	int i, changed = 0;

	// see if any keys have changed state
	for (i = 0; i < MAX_KEYS; i++)
		if (keyboard_state[0][i] != oldkey[i])
		{
			changed = 1;

			// keypress was missed, turn it on for one frame
			if (keyboard_state[0][i] == 0 && currkey[i] == 0)
				currkey[i] = -1;
		}

	// if keyboard state is stable, copy it over
	if (!changed)
		memcpy(currkey, &keyboard_state[0][0], sizeof(currkey));

	// remember the previous state
	memcpy(oldkey, &keyboard_state[0][0], sizeof(oldkey));
}



//============================================================
//  is_key_pressed
//============================================================

static int is_key_pressed(os_code_t keycode)
{
	int dik = DICODE(keycode);

	// make sure we've polled recently
	if (osd_cycles() > last_poll + osd_cycles_per_second()/4)
		win_poll_input();

	// special case: if we're trying to quit, fake up/down/up/down
	if (dik == DIK_ESCAPE && win_trying_to_quit)
	{
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	// if the video window isn't visible, we have to get our events from the console
	if (!win_video_window || !IsWindowVisible(win_video_window))
	{
		// warning: this code relies on the assumption that when you're polling for
		// keyboard events before the system is initialized, they are all of the
		// "press any key" to continue variety
		int result = _kbhit();
		if (result)
			_getch();
		return result;
	}

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	// if the debugger is visible and we don't have focus, the key is not pressed
	if (debugwin_is_debugger_visible() && GetFocus() != win_video_window)
		return 0;
#endif

	// otherwise, just return the current keystate
	if (steadykey)
		return currkey[dik];
	else
		return keyboard_state[0][dik];
}



//============================================================
//  osd_readkey_unicode
//============================================================

int osd_readkey_unicode(int flush)
{
#if 0
	if (flush) clear_keybuf();
	if (keypressed())
		return ureadkey(NULL);
	else
		return 0;
#endif
	return 0;
}



//============================================================
//  init_keycodes
//============================================================

static void init_keycodes(void)
{
	int key;

	// iterate over all possible keys
	for (key = 0; key < MAX_KEYS; key++)
	{
		DIDEVICEOBJECTINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
		result = IDirectInputDevice_GetObjectInfo(keyboard_device[0], &instance, key, DIPH_BYOFFSET);
		if (result == DI_OK)
		{
			// if it worked, assume we have a valid key

			// copy the name
			char *namecopy = malloc(strlen(instance.tszName) + 1);
			if (namecopy)
			{
				input_code_t standardcode;
				os_code_t code;
				int entry;

				// find the table entry, if there is one
				for (entry = 0; win_key_trans_table[entry][0] >= 0; entry++)
					if (win_key_trans_table[entry][DI_KEY] == key)
						break;

				// compute the code, which encodes DirectInput, virtual, and ASCII codes
				code = KEYCODE(key, 0, 0);
				standardcode = CODE_OTHER_DIGITAL;
				if (win_key_trans_table[entry][0] >= 0)
				{
					code = KEYCODE(key, win_key_trans_table[entry][VIRTUAL_KEY], win_key_trans_table[entry][ASCII_KEY]);
					standardcode = win_key_trans_table[entry][MAME_KEY];
				}

				// fill in the key description
				codelist[total_codes].name = strcpy(namecopy, instance.tszName);
				codelist[total_codes].oscode = code;
				codelist[total_codes].inputcode = standardcode;
				total_codes++;
			}
		}
	}
}



//============================================================
//  update_joystick_axes
//============================================================

static void update_joystick_axes(void)
{
	int joynum, axis;

	for (joynum = 0; joynum < joystick_count; joynum++)
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			struct axis_history *history = &joystick_history[joynum][axis][0];
			LONG curval = ((LONG *)&joystick_state[joynum].lX)[axis];
			int newtype;

			/* if same as last time (within a small tolerance), update the count */
			if (history[0].count > 0 && (history[0].value - curval) > -4 && (history[0].value - curval) < 4)
				history[0].count++;

			/* otherwise, update the history */
			else
			{
				memmove(&history[1], &history[0], sizeof(history[0]) * (HISTORY_LENGTH - 1));
				history[0].count = 1;
				history[0].value = curval;
			}

			/* if we've only ever seen one value here, or if we've been stuck at the same value for a long */
			/* time (1 minute), mark the axis as dead or invalid */
			if (history[1].count == 0 || history[0].count > Machine->refresh_rate * 60)
				newtype = AXIS_TYPE_INVALID;

			/* scan the history and count unique values; if we get more than 3, it's analog */
			else
			{
				int bucketsize = (joystick_range[joynum][axis].lMax - joystick_range[joynum][axis].lMin) / 3;
				LONG uniqueval[3] = { 1234567890, 1234567890, 1234567890 };
				int histnum;

				/* assume digital unless we figure out otherwise */
				newtype = AXIS_TYPE_DIGITAL;

				/* loop over the whole history, bucketing the values */
				for (histnum = 0; histnum < HISTORY_LENGTH; histnum++)
					if (history[histnum].count > 0)
					{
						int bucket = (history[histnum].value - joystick_range[joynum][axis].lMin) / bucketsize;

						/* if we already have an entry in this bucket, we're analog */
						if (uniqueval[bucket] != 1234567890 && uniqueval[bucket] != history[histnum].value)
						{
							newtype = AXIS_TYPE_ANALOG;
							break;
						}

						/* remember this value */
						uniqueval[bucket] = history[histnum].value;
					}
			}

			/* if the type doesn't match, switch it */
			if (joystick_type[joynum][axis] != newtype)
			{
				static const char *axistypes[] = { "invalid", "digital", "analog" };
				joystick_type[joynum][axis] = newtype;
				if (verbose)
					fprintf(stderr, "Joystick %d axis %d is now %s\n", joynum, axis, axistypes[newtype]);
			}
		}
}



//============================================================
//  add_joylist_entry
//============================================================

static void add_joylist_entry(const char *name, os_code_t code, input_code_t standardcode)
{
	// copy the name
	char *namecopy = malloc(strlen(name) + 1);
	if (namecopy)
	{
		int entry;

		// find the table entry, if there is one
		for (entry = 0; entry < ELEMENTS(joy_trans_table); entry++)
			if (joy_trans_table[entry][0] == code)
				break;

		// fill in the joy description
		codelist[total_codes].name = strcpy(namecopy, name);
		codelist[total_codes].oscode = code;
		if (entry < ELEMENTS(joy_trans_table))
			standardcode = joy_trans_table[entry][1];
		codelist[total_codes].inputcode = standardcode;
		total_codes++;
	}
}



//============================================================
//  init_joycodes
//============================================================

static void init_joycodes(void)
{
	int mouse, gun, stick, axis, button, pov;
	char tempname[MAX_PATH];

	// map mice first
	for (mouse = 0; mouse < mouse_count; mouse++)
	{
		// add analog axes (fix me -- should enumerate these)
		sprintf(tempname, "Mouse %d X", mouse + 1);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 0), CODE_OTHER_ANALOG_RELATIVE);
		sprintf(tempname, "Mouse %d Y", mouse + 1);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 1), CODE_OTHER_ANALOG_RELATIVE);
		sprintf(tempname, "Mouse %d Z", mouse + 1);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 2), CODE_OTHER_ANALOG_RELATIVE);

		// add mouse buttons
		for (button = 0; button < 4; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(mouse_device[mouse], &instance, offsetof(DIMOUSESTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add mouse number to the name
				if (mouse_count > 1)
					sprintf(tempname, "Mouse %d %s", mouse + 1, instance.tszName);
				else
					sprintf(tempname, "Mouse %s", instance.tszName);
				add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEBUTTON, button), CODE_OTHER_DIGITAL);
			}
		}
	}

	// map lightguns second
	for (gun = 0; gun < lightgun_count; gun++)
	{
		// add lightgun axes (fix me -- should enumerate these)
		sprintf(tempname, "Lightgun %d X", gun + 1);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 0), CODE_OTHER_ANALOG_ABSOLUTE);
		sprintf(tempname, "Lightgun %d Y", gun + 1);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 1), CODE_OTHER_ANALOG_ABSOLUTE);
	}

	// now map joysticks
	for (stick = 0; stick < joystick_count; stick++)
	{
		// log the info
		if (verbose)
			fprintf(stderr, "Joystick %d: %s (%d axes, %d buttons, %d POVs)\n", stick + 1, joystick_name[stick], (int)joystick_caps[stick].dwAxes, (int)joystick_caps[stick].dwButtons, (int)joystick_caps[stick].dwPOVs);

		// loop over all axes
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// reset the type
			joystick_type[stick][axis] = AXIS_TYPE_INVALID;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				if (verbose)
					fprintf(stderr, "  Axis %d (%s)%s\n", axis, instance.tszName, joystick_digital[stick][axis] ? " - digital" : "");

				// add analog axis
				if (!joystick_digital[stick][axis])
				{
					sprintf(tempname, "J%d %s", stick + 1, instance.tszName);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_JOYAXIS, axis), CODE_OTHER_ANALOG_ABSOLUTE);
				}

				// add negative value
				sprintf(tempname, "J%d %s -", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_NEG, axis), CODE_OTHER_DIGITAL);

				// add positive value
				sprintf(tempname, "J%d %s +", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_POS, axis), CODE_OTHER_DIGITAL);

				// get the axis range while we're here
				joystick_range[stick][axis].diph.dwSize = sizeof(DIPROPRANGE);
				joystick_range[stick][axis].diph.dwHeaderSize = sizeof(joystick_range[stick][axis].diph);
				joystick_range[stick][axis].diph.dwObj = offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG);
				joystick_range[stick][axis].diph.dwHow = DIPH_BYOFFSET;
				result = IDirectInputDevice_GetProperty(joystick_device[stick], DIPROP_RANGE, &joystick_range[stick][axis].diph);
			}
		}

		// loop over all buttons
		for (button = 0; button < MAX_BUTTONS; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// make the name for this item
				sprintf(tempname, "J%d %s", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_BUTTON, button), CODE_OTHER_DIGITAL);
			}
		}

		// check POV hats
		for (pov = 0; pov < MAX_POV; pov++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgdwPOV[pov]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add up direction
				sprintf(tempname, "J%d %s U", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_UP, pov), CODE_OTHER_DIGITAL);

				// add down direction
				sprintf(tempname, "J%d %s D", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_DOWN, pov), CODE_OTHER_DIGITAL);

				// add left direction
				sprintf(tempname, "J%d %s L", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_LEFT, pov), CODE_OTHER_DIGITAL);

				// add right direction
				sprintf(tempname, "J%d %s R", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_RIGHT, pov), CODE_OTHER_DIGITAL);
			}
		}
	}
}



//============================================================
//  get_joycode_value
//============================================================

static INT32 get_joycode_value(os_code_t joycode)
{
	int joyindex = JOYINDEX(joycode);
	int codetype = CODETYPE(joycode);
	int joynum = JOYNUM(joycode);
	DWORD pov;

	// switch off the type
	switch (codetype)
	{
		case CODETYPE_MOUSEBUTTON:
			/* ActLabs lightgun - remap button 2 (shot off-screen) as button 1 */
			if (use_lightgun_dual && joyindex<4) {
				if (use_lightgun_reload && joynum==0) {
					if (joyindex==0 && lightgun_dual_player_state[1])
						return 1;
					if (joyindex==1 && lightgun_dual_player_state[1])
						return 0;
					if (joyindex==2 && lightgun_dual_player_state[3])
						return 1;
					if (joyindex==3 && lightgun_dual_player_state[3])
						return 0;
				}
				return lightgun_dual_player_state[joyindex];
			}

			if (use_lightgun) {
				if (use_lightgun_reload && joynum==0) {
					if (joyindex==0 && (mouse_state[0].rgbButtons[1]&0x80))
						return 1;
					if (joyindex==1 && (mouse_state[0].rgbButtons[1]&0x80))
						return 0;
				}
			}
			return mouse_state[joynum].rgbButtons[joyindex] >> 7;

		case CODETYPE_BUTTON:
			return joystick_state[joynum].rgbButtons[joyindex] >> 7;

		case CODETYPE_AXIS_POS:
		case CODETYPE_AXIS_NEG:
		{
			LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
			LONG top = joystick_range[joynum][joyindex].lMax;
			LONG bottom = joystick_range[joynum][joyindex].lMin;
			LONG middle = (top + bottom) / 2;

			// watch for movement greater "a2d_deadzone" along either axis
			// FIXME in the two-axis joystick case, we need to find out
			// the angle. Anything else is unprecise.
			if (codetype == CODETYPE_AXIS_POS)
				return (val > middle + ((top - middle) * a2d_deadzone));
			else
				return (val < middle - ((middle - bottom) * a2d_deadzone));
		}

		// anywhere from 0-45 (315) deg to 0+45 (45) deg
		case CODETYPE_POV_UP:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 31500 || pov <= 4500));

		// anywhere from 90-45 (45) deg to 90+45 (135) deg
		case CODETYPE_POV_RIGHT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 4500 && pov <= 13500));

		// anywhere from 180-45 (135) deg to 180+45 (225) deg
		case CODETYPE_POV_DOWN:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 13500 && pov <= 22500));

		// anywhere from 270-45 (225) deg to 270+45 (315) deg
		case CODETYPE_POV_LEFT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 22500 && pov <= 31500));

		// analog joystick axis
		case CODETYPE_JOYAXIS:
		{
			if (joystick_type[joynum][joyindex] != AXIS_TYPE_ANALOG)
				return ANALOG_VALUE_INVALID;
			else
			{
				LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
				LONG top = joystick_range[joynum][joyindex].lMax;
				LONG bottom = joystick_range[joynum][joyindex].lMin;

				if (!use_joystick)
					return 0;
				val = (INT64)(val - bottom) * (INT64)(ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) / (INT64)(top - bottom) + ANALOG_VALUE_MIN;
				if (val < ANALOG_VALUE_MIN) val = ANALOG_VALUE_MIN;
				if (val > ANALOG_VALUE_MAX) val = ANALOG_VALUE_MAX;
				return val;
			}
		}

		// analog mouse axis
		case CODETYPE_MOUSEAXIS:
			// if the mouse isn't yet active, make it so
			if (!mouse_active && win_use_mouse && !win_has_menu())
			{
				mouse_active = 1;
				win_pause_input(0);
			}

			// return the latest mouse info
			if (joyindex == 0)
				return mouse_state[joynum].lX * 512;
			if (joyindex == 1)
				return mouse_state[joynum].lY * 512;
			if (joyindex == 2)
				return mouse_state[joynum].lZ * 512;
			return 0;

		// analog gun axis
		case CODETYPE_GUNAXIS:
			// return the latest gun info
			if (joynum >= MAX_LIGHTGUNS)
				return 0;
			if (joyindex >= 2)
				return 0;
			return gun_axis[joynum][joyindex];
	}

	// keep the compiler happy
	return 0;
}



//============================================================
//  osd_is_code_pressed
//============================================================

INT32 osd_get_code_value(os_code_t code)
{
	if (IS_KEYBOARD_CODE(code))
		return is_key_pressed(code);
	else
		return get_joycode_value(code);
}



//============================================================
//  osd_get_code_list
//============================================================

const struct OSCodeInfo *osd_get_code_list(void)
{
	return codelist;
}



//============================================================
//  osd_lightgun_read
//============================================================

void input_mouse_button_down(int button, int x, int y)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=1;
	lightgun_dual_player_pos[button].x=x;
	lightgun_dual_player_pos[button].y=y;

	//logerror("mouse %d at %d %d\n",button,x,y);
}

void input_mouse_button_up(int button)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=0;
}

static void poll_lightguns(void)
{
	POINT point;
	int player;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && (win_use_mouse || use_lightgun) && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// if out of range, skip it
	if (!use_lightgun || !win_physical_width || !win_physical_height)
		return;

	// Warning message to users - design wise this probably isn't the best function to put this in...
	if (win_window_mode)
		usrintf_showmessage("Lightgun not supported in windowed mode");

	// loop over players
	for (player = 0; player < MAX_LIGHTGUNS; player++)
	{
		// Hack - if button 2 is pressed on lightgun, then return 0,0 (off-screen) to simulate reload
		if (use_lightgun_reload)
		{
			int return_offscreen=0;

			// In dualmode we need to use the buttons returned from Windows messages
			if (use_lightgun_dual)
			{
				if (player==0 && lightgun_dual_player_state[1])
					return_offscreen=1;

				if (player==1 && lightgun_dual_player_state[3])
					return_offscreen=1;
			}
			else
			{
				if (mouse_state[0].rgbButtons[1]&0x80)
					return_offscreen=1;
			}

			if (return_offscreen)
			{
				gun_axis[player][0] = ANALOG_VALUE_MIN;
				gun_axis[player][1] = ANALOG_VALUE_MIN;
				continue;
			}
		}

		// Act-Labs dual lightgun - _only_ works with Windows messages for input location
		if (use_lightgun_dual)
		{
			if (player==0)
			{
				point.x=lightgun_dual_player_pos[0].x; // Button 0 is player 1
				point.y=lightgun_dual_player_pos[0].y; // Button 0 is player 1
			}
			else if (player==1)
			{
				point.x=lightgun_dual_player_pos[2].x; // Button 2 is player 2
				point.y=lightgun_dual_player_pos[2].y; // Button 2 is player 2
			}
			else
			{
				point.x=point.y=0;
			}

			// Map absolute pixel values into -128 -> 128 range
			gun_axis[player][0] = (point.x * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_width/2) / (win_physical_width-1) + ANALOG_VALUE_MIN;
			gun_axis[player][1] = (point.y * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_height/2) / (win_physical_height-1) + ANALOG_VALUE_MIN;
		}
		else
		{
			// I would much prefer to use DirectInput to read the gun values but there seem to be
			// some problems...  DirectInput (8.0 tested) on Win98 returns garbage for both buffered
			// and immediate, absolute and relative axis modes.  Win2k (DX 8.1) returns good data
			// for buffered absolute reads, but WinXP (8.1) returns garbage on all modes.  DX9 betas
			// seem to exhibit the same behaviour.  I have no idea of the cause of this, the only
			// consistent way to read the location seems to be the Windows system call GetCursorPos
			// which requires the application have non-exclusive access to the mouse device
			//
			GetCursorPos(&point);

			// Map absolute pixel values into ANALOG_VALUE_MIN -> ANALOG_VALUE_MAX range
			gun_axis[player][0] = (point.x * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_width/2) / (win_physical_width-1) + ANALOG_VALUE_MIN;
			gun_axis[player][1] = (point.y * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_height/2) / (win_physical_height-1) + ANALOG_VALUE_MIN;
		}

		if (gun_axis[player][0] < ANALOG_VALUE_MIN) gun_axis[player][0] = ANALOG_VALUE_MIN;
		if (gun_axis[player][0] > ANALOG_VALUE_MAX) gun_axis[player][0] = ANALOG_VALUE_MAX;
		if (gun_axis[player][1] < ANALOG_VALUE_MIN) gun_axis[player][1] = ANALOG_VALUE_MIN;
		if (gun_axis[player][1] > ANALOG_VALUE_MAX) gun_axis[player][1] = ANALOG_VALUE_MAX;
	}
}



//============================================================
//  osd_joystick_needs_calibration
//============================================================

int osd_joystick_needs_calibration(void)
{
	return 0;
}



//============================================================
//  osd_joystick_start_calibration
//============================================================

void osd_joystick_start_calibration(void)
{
}



//============================================================
//  osd_joystick_calibrate_next
//============================================================

const char *osd_joystick_calibrate_next(void)
{
	return 0;
}



//============================================================
//  osd_joystick_calibrate
//============================================================

void osd_joystick_calibrate(void)
{
}



//============================================================
//  osd_joystick_end_calibration
//============================================================

void osd_joystick_end_calibration(void)
{
}



//============================================================
//  osd_customize_inputport_list
//============================================================

void osd_customize_inputport_list(struct InputPortDefinition *defaults)
{
	static input_seq_t no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	struct InputPortDefinition *idef = defaults;

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// map in some OSD-specific keys
		switch (idef->type)
		{
			// alt-enter for fullscreen
			case IPT_OSD_1:
				idef->token = "TOGGLE_FULLSCREEN";
				idef->name = "Toggle fullscreen";
				seq_set_2(&idef->defaultseq, KEYCODE_LALT, KEYCODE_ENTER);
				break;

#ifdef MESS
			case IPT_OSD_2:
				if (options.disable_normal_ui)
				{
					idef->token = "TOGGLE_MENUBAR";
					idef->name = "Toggle menubar";
					seq_set_1 (&idef->defaultseq, KEYCODE_SCRLOCK);
				}
				break;
#endif /* MESS */
		}

		// disable the config menu if the ALT key is down
		// (allows ALT-TAB to switch between windows apps)
		if (idef->type == IPT_UI_CONFIGURE)
			seq_copy(&idef->defaultseq, &no_alt_tab_seq);

#ifdef MESS
		if (idef->type == IPT_UI_THROTTLE)
			seq_set_0(&idef->defaultseq);
#endif /* MESS */

		// Dual lightguns - remap default buttons to suit
		if (use_lightgun && use_lightgun_dual)
		{
			static input_seq_t p1b2 = SEQ_DEF_3(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2);
			static input_seq_t p1b3 = SEQ_DEF_3(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3);
			static input_seq_t p2b1 = SEQ_DEF_5(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1, CODE_OR, MOUSECODE_1_BUTTON3);
			static input_seq_t p2b2 = SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2);

			if (idef->type == IPT_BUTTON2 && idef->player == 1)
				seq_copy(&idef->defaultseq, &p1b2);
			if (idef->type == IPT_BUTTON3 && idef->player == 1)
				seq_copy(&idef->defaultseq, &p1b3);
			if (idef->type == IPT_BUTTON1 && idef->player == 2)
				seq_copy(&idef->defaultseq, &p2b1);
			if (idef->type == IPT_BUTTON2 && idef->player == 2)
				seq_copy(&idef->defaultseq, &p2b2);
		}

		// find the next one
		idef++;
	}
}



//============================================================
//  osd_get_leds
//============================================================

int osd_get_leds(void)
{
	int result = 0;

	if (!use_keyboard_leds)
		return 0;

	// if we're on Win9x, use GetKeyboardState
	if( ledmethod == 0 )
	{
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// set the numlock bit
		result |= (key_states[VK_NUMLOCK] & 1);
		result |= (key_states[VK_CAPITAL] & 1) << 1;
		result |= (key_states[VK_SCROLL] & 1) << 2;
	}
	else if( ledmethod == 1 ) // WinNT/2K/XP, use GetKeyboardState
	{
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// set the numlock bit
		result |= (key_states[VK_NUMLOCK] & 1);
		result |= (key_states[VK_CAPITAL] & 1) << 1;
		result |= (key_states[VK_SCROLL] & 1) << 2;
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer

		// Address first keyboard
		OutputBuffer.UnitId = 0;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
						NULL, 0,
						&OutputBuffer, DataLength,
						&ReturnedLength, NULL);

		// Demangle lights to match 95/98
		if (OutputBuffer.LedFlags & KEYBOARD_NUM_LOCK_ON) result |= 0x1;
		if (OutputBuffer.LedFlags & KEYBOARD_CAPS_LOCK_ON) result |= 0x2;
		if (OutputBuffer.LedFlags & KEYBOARD_SCROLL_LOCK_ON) result |= 0x4;
	}

	return result;
}



//============================================================
//  osd_set_leds
//============================================================

void osd_set_leds(int state)
{
	if (!use_keyboard_leds)
		return;

	// if we're on Win9x, use SetKeyboardState
	if( ledmethod == 0 )
	{
		// thanks to Lee Taylor for the original version of this code
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// mask states and set new states
		key_states[VK_NUMLOCK] = (key_states[VK_NUMLOCK] & ~1) | ((state >> 0) & 1);
		key_states[VK_CAPITAL] = (key_states[VK_CAPITAL] & ~1) | ((state >> 1) & 1);
		key_states[VK_SCROLL] = (key_states[VK_SCROLL] & ~1) | ((state >> 2) & 1);

		SetKeyboardState(&key_states[0]);
	}
	else if( ledmethod == 1 ) // WinNT/2K/XP, use keybd_event()
	{
		int k;
		BYTE keyState[ 256 ];
		const BYTE vk[ 3 ] = { VK_NUMLOCK, VK_CAPITAL, VK_SCROLL };

		GetKeyboardState( (LPBYTE)&keyState );
		for( k = 0; k < 3; k++ )
		{
			if( (  ( ( state >> k ) & 1 ) && !( keyState[ vk[ k ] ] & 1 ) ) ||
				( !( ( state >> k ) & 1 ) &&  ( keyState[ vk[ k ] ] & 1 ) ) )
			{
				// Simulate a key press
				keybd_event( vk[ k ], 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0 );

				// Simulate a key release
				keybd_event( vk[ k ], 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0 );
			}
		}
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer
		UINT				LedFlags=0;

		// Demangle lights to match 95/98
		if (state & 0x1) LedFlags |= KEYBOARD_NUM_LOCK_ON;
		if (state & 0x2) LedFlags |= KEYBOARD_CAPS_LOCK_ON;
		if (state & 0x4) LedFlags |= KEYBOARD_SCROLL_LOCK_ON;

		// Address first keyboard
		InputBuffer.UnitId = 0;
		InputBuffer.LedFlags = LedFlags;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
						&InputBuffer, DataLength,
						NULL, 0,
						&ReturnedLength, NULL);
	}

	return;
}



//============================================================
//  start_led
//============================================================

void start_led(void)
{
	OSVERSIONINFO osinfo = { sizeof(OSVERSIONINFO) };

	if (!use_keyboard_leds)
		return;

	// retrive windows version
	GetVersionEx(&osinfo);

	if ( osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		// 98
		ledmethod = 0;
	}
	else if( strcmp( ledmode, "usb" ) == 0 )
	{
		// nt/2k/xp
		ledmethod = 1;
	}
	else
	{
		// nt/2k/xp
		int error_number;

		ledmethod = 2;

		if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
					"\\Device\\KeyboardClass0"))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}

		hKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
					NULL,	OPEN_EXISTING,	0,	NULL);

		if (hKbdDev == INVALID_HANDLE_VALUE)
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	// remember the initial LED states
	original_leds = osd_get_leds();

	return;
}



//============================================================
//  stop_led
//============================================================

void stop_led(void)
{
	int error_number = 0;

	if (!use_keyboard_leds)
		return;

	// restore the initial LED states
	osd_set_leds(original_leds);

	if( ledmethod == 0 )
	{
	}
	else if( ledmethod == 1 )
	{
	}
	else
	{
		// nt/2k/xp
		if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}

		if (!CloseHandle(hKbdDev))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	return;
}
