#ifndef KEYBOARD_H
#define KEYBOARD_H


struct KeyboardKey
{
	char *name;			/* OS dependant name; 0 terminates the list */
	int code;			/* OS dependant code (must be < KEYCODE_START) */
	int standardcode;	/* KEYCODE_xxx equivalent from list below, or KEYCODE_OTHER if n/a */
};

enum
{
	KEYCODE_START = 256,	/* marker - not a real key */

	KEYCODE_1,KEYCODE_2,KEYCODE_3,KEYCODE_4,KEYCODE_5,
	KEYCODE_6,KEYCODE_7,KEYCODE_8,KEYCODE_9,KEYCODE_0,
	KEYCODE_A,KEYCODE_B,KEYCODE_C,KEYCODE_D,KEYCODE_E,KEYCODE_F,KEYCODE_G,
	KEYCODE_H,KEYCODE_I,KEYCODE_J,KEYCODE_K,KEYCODE_L,KEYCODE_M,KEYCODE_N,
	KEYCODE_O,KEYCODE_P,KEYCODE_Q,KEYCODE_R,KEYCODE_S,KEYCODE_T,KEYCODE_U,
	KEYCODE_V,KEYCODE_W,KEYCODE_X,KEYCODE_Y,KEYCODE_Z,
	KEYCODE_F1,KEYCODE_F2,KEYCODE_F3,KEYCODE_F4,KEYCODE_F5,KEYCODE_F6,
	KEYCODE_F7,KEYCODE_F8,KEYCODE_F9,KEYCODE_F10,KEYCODE_F11,KEYCODE_F12,
	KEYCODE_UP,KEYCODE_DOWN,KEYCODE_LEFT,KEYCODE_RIGHT,
	KEYCODE_ESC,KEYCODE_TAB,KEYCODE_ENTER,KEYCODE_SPACE,
	KEYCODE_LCONTROL,KEYCODE_RCONTROL,KEYCODE_LSHIFT,KEYCODE_RSHIFT,
	KEYCODE_LALT,KEYCODE_RALT,KEYCODE_BACKSPACE,KEYCODE_TILDE,
	KEYCODE_INSERT,KEYCODE_DEL,KEYCODE_HOME,KEYCODE_END,KEYCODE_PGUP,KEYCODE_PGDN,
	KEYCODE_1_PAD,KEYCODE_2_PAD,KEYCODE_3_PAD,KEYCODE_4_PAD,KEYCODE_5_PAD,
	KEYCODE_6_PAD,KEYCODE_7_PAD,KEYCODE_8_PAD,KEYCODE_9_PAD,KEYCODE_0_PAD,

	KEYCODE_OTHER,	/* anything else */

	KEYCODE_ANY,	/* special value for keyboard_key_pressed() */
	KEYCODE_NONE	/* no key pressed */
};


const char *keyboard_key_name(int keycode);
int keyboard_key_pressed(int keycode);
int keyboard_key_pressed_memory(int keycode);
int keyboard_key_pressed_memory_repeat(int keycode,int speed);
int keyboard_read_key_immediate(void);
int keyboard_debug_readkey(void);
int keyboard_ui_key_pressed(int code);
int keyboard_ui_key_pressed_repeat(int code,int speed);

#endif
