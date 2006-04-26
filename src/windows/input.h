//============================================================
//
//  input.h - Win32 implementation of MAME input routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H

//============================================================
//  MACROS
//============================================================

// Define the keyboard indicators.
// (Definitions borrowed from ntddkbd.h)
//

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)


//============================================================
//  PARAMETERS
//============================================================

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;		// Unit identifier.
    USHORT LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3


//============================================================
//  PROTOTYPES
//============================================================

extern const int win_key_trans_table[][4];
extern int win_use_mouse;

int osd_get_leds(void);
void osd_set_leds(int state);

void start_led(void);
void stop_led(void);
void input_mouse_button_down(int button, int x, int y);
void input_mouse_button_up(int button);

#endif /* ifndef __INPUTD_H */
