/*
 * X-Mame video specifics code
 *
 */
#define __KEYBOARD_C_

/*
 * Include files.
 */

#include "xmame.h"

/* translation table from X11 codes to raw keyboard scan codes */
/* the idea is make a re-definable lookup table, instead a long case switch */

#ifndef OSD_KEY_NONE
#define OSD_KEY_NONE	0
#endif

/* following code is taken from keysmdef.h */

/******************************************************************/
/* $XConsortium: keysymdef.h /main/24 1996/02/02 14:28:10 kaleb $ */
/******************************************************************/

/* XK_VoidSymbol	0xFFFFFF	*/ /* void symbol */

static int extended_code_table[] = {
/*                      0xFF00  */ OSD_KEY_NONE,
/*                      0xFF01  */ OSD_KEY_NONE,
/*                      0xFF02  */ OSD_KEY_NONE,
/*                      0xFF03  */ OSD_KEY_NONE,
/*                      0xFF04  */ OSD_KEY_NONE,
/*                      0xFF05  */ OSD_KEY_NONE,
/*                      0xFF06  */ OSD_KEY_NONE,
/*                      0xFF07  */ OSD_KEY_NONE,
/* XK_BackSpace		0xFF08	*/ OSD_KEY_BACKSPACE,
/* XK_Tab		0xFF09	*/ OSD_KEY_TAB,
/* XK_Linefeed		0xFF0A	*/ OSD_KEY_ENTER,
/* XK_Clear		0xFF0B  */ OSD_KEY_DEL,
/*                      0xFF0C  */ OSD_KEY_NONE,
/* XK_Return		0xFF0D	*/ OSD_KEY_ENTER,
/*                      0xFF0E  */ OSD_KEY_NONE,
/*                      0xFF0F  */ OSD_KEY_NONE,
/*                      0xFF10  */ OSD_KEY_NONE,
/*                      0xFF11  */ OSD_KEY_NONE,
/*                      0xFF12  */ OSD_KEY_NONE,
/* XK_Pause		0xFF13	*/ OSD_KEY_NONE,
/* XK_Scroll_Lock	0xFF14  */ OSD_KEY_SCRLOCK,
/* XK_Sys_Req		0xFF15  */ OSD_KEY_NONE,
/*                      0xFF16  */ OSD_KEY_NONE,
/*                      0xFF17  */ OSD_KEY_NONE,
/*                      0xFF18  */ OSD_KEY_NONE,
/*                      0xFF19  */ OSD_KEY_NONE,
/*                      0xFF1A  */ OSD_KEY_NONE,
/* XK_Escape		0xFF1B  */ OSD_KEY_ESC,
/*                      0xFF1C  */ OSD_KEY_NONE,
/*                      0xFF1D  */ OSD_KEY_NONE,
/*                      0xFF1E  */ OSD_KEY_NONE,
/*                      0xFF1F  */ OSD_KEY_NONE,
/* XK_Multi_key		0xFF20  */ OSD_KEY_NONE,
/* XK_Kanji		0xFF21	*/ OSD_KEY_NONE,
/* XK_Muhenkan		0xFF22  */ OSD_KEY_NONE,
/* XK_Henkan_Mode	0xFF23  */ OSD_KEY_NONE,
/* XK_Henkan		0xFF23  duplicated */
/* XK_Romaji		0xFF24  */ OSD_KEY_NONE,
/* XK_Hiragana		0xFF25  */ OSD_KEY_NONE,
/* XK_Katakana		0xFF26  */ OSD_KEY_NONE,
/* XK_Hiragana_Katakana	0xFF27  */ OSD_KEY_NONE,
/* XK_Zenkaku		0xFF28  */ OSD_KEY_NONE,
/* XK_Hankaku		0xFF29  */ OSD_KEY_NONE,
/* XK_Zenkaku_Hankaku	0xFF2A  */ OSD_KEY_NONE,
/* XK_Touroku		0xFF2B  */ OSD_KEY_NONE,
/* XK_Massyo		0xFF2C  */ OSD_KEY_NONE,
/* XK_Kana_Lock		0xFF2D  */ OSD_KEY_NONE,
/* XK_Kana_Shift	0xFF2E  */ OSD_KEY_NONE,
/* XK_Eisu_Shift	0xFF2F  */ OSD_KEY_NONE,
/* XK_Eisu_toggle	0xFF30  */ OSD_KEY_NONE,
/*                      0xFF31  */ OSD_KEY_NONE,
/*                      0xFF32  */ OSD_KEY_NONE,
/*                      0xFF33  */ OSD_KEY_NONE,
/*                      0xFF34  */ OSD_KEY_NONE,
/*                      0xFF35  */ OSD_KEY_NONE,
/*                      0xFF36  */ OSD_KEY_NONE,
/*                      0xFF37  */ OSD_KEY_NONE,
/*                      0xFF38  */ OSD_KEY_NONE,
/*                      0xFF39  */ OSD_KEY_NONE,
/*                      0xFF3A  */ OSD_KEY_NONE,
/*                      0xFF3B  */ OSD_KEY_NONE,
/*                      0xFF3C  */ OSD_KEY_NONE,
/*                      0xFF3D  */ OSD_KEY_NONE,
/*                      0xFF3E  */ OSD_KEY_NONE,
/*                      0xFF3F  */ OSD_KEY_NONE,
/*                      0xFF40  */ OSD_KEY_NONE,
/*                      0xFF41  */ OSD_KEY_NONE,
/*                      0xFF42  */ OSD_KEY_NONE,
/*                      0xFF43  */ OSD_KEY_NONE,
/*                      0xFF44  */ OSD_KEY_NONE,
/*                      0xFF45  */ OSD_KEY_NONE,
/*                      0xFF46  */ OSD_KEY_NONE,
/*                      0xFF47  */ OSD_KEY_NONE,
/*                      0xFF48  */ OSD_KEY_NONE,
/*                      0xFF49  */ OSD_KEY_NONE,
/*                      0xFF4A  */ OSD_KEY_NONE,
/*                      0xFF4B  */ OSD_KEY_NONE,
/*                      0xFF4C  */ OSD_KEY_NONE,
/*                      0xFF4D  */ OSD_KEY_NONE,
/*                      0xFF4E  */ OSD_KEY_NONE,
/*                      0xFF4F  */ OSD_KEY_NONE,
/* XK_Home		0xFF50  */ OSD_KEY_HOME,
/* XK_Left		0xFF51	*/ OSD_KEY_LEFT,
/* XK_Up		0xFF52	*/ OSD_KEY_UP,
/* XK_Right		0xFF53	*/ OSD_KEY_RIGHT,
/* XK_Down		0xFF54	*/ OSD_KEY_DOWN,
/* XK_Prior		0xFF55	*/ OSD_KEY_PGUP,
/* XK_Page_Up		0xFF55  duplicated */
/* XK_Next		0xFF56	*/ OSD_KEY_PGDN,
/* XK_Page_Down		0xFF56  duplicated */
/* XK_End		0xFF57	*/ OSD_KEY_END,
/* XK_Begin		0xFF58	*/ OSD_KEY_HOME,
/*                      0xFF59  */ OSD_KEY_NONE,
/*                      0xFF5A  */ OSD_KEY_NONE,
/*                      0xFF5B  */ OSD_KEY_NONE,
/*                      0xFF5C  */ OSD_KEY_NONE,
/*                      0xFF5D  */ OSD_KEY_NONE,
/*                      0xFF5E  */ OSD_KEY_NONE,
/*                      0xFF5F  */ OSD_KEY_NONE,
/* XK_Select		0xFF60	*/ OSD_KEY_NONE,
/* XK_Print		0xFF61  */ OSD_KEY_NONE,
/* XK_Execute		0xFF62	*/ OSD_KEY_ENTER,
/* XK_Insert		0xFF63	*/ OSD_KEY_INSERT,
/*                      0xFF64	*/ OSD_KEY_NONE,
/* XK_Undo		0xFF65	*/ OSD_KEY_NONE,
/* XK_Redo		0xFF66	*/ OSD_KEY_NONE,
/* XK_Menu		0xFF67  */ OSD_KEY_NONE,
/* XK_Find		0xFF68	*/ OSD_KEY_NONE,
/* XK_Cancel		0xFF69	*/ OSD_KEY_NONE,
/* XK_Help		0xFF6A	*/ OSD_KEY_NONE,
/* XK_Break		0xFF6B  */ OSD_KEY_NONE,
/* 			0xFF6C  */ OSD_KEY_NONE,
/* 			0xFF6D  */ OSD_KEY_NONE,
/* 			0xFF6E  */ OSD_KEY_NONE,
/* 			0xFF6F  */ OSD_KEY_NONE,
/* 			0xFF70  */ OSD_KEY_NONE,
/* 			0xFF71  */ OSD_KEY_NONE,
/* 			0xFF72  */ OSD_KEY_NONE,
/* 			0xFF73  */ OSD_KEY_NONE,
/* 			0xFF74  */ OSD_KEY_NONE,
/* 			0xFF75  */ OSD_KEY_NONE,
/* 			0xFF76  */ OSD_KEY_NONE,
/* 			0xFF77  */ OSD_KEY_NONE,
/* 			0xFF78  */ OSD_KEY_NONE,
/* 			0xFF79  */ OSD_KEY_NONE,
/* 			0xFF7A  */ OSD_KEY_NONE,
/* 			0xFF7B  */ OSD_KEY_NONE,
/* 			0xFF7C  */ OSD_KEY_NONE,
/* 			0xFF7D  */ OSD_KEY_NONE,
/* XK_Mode_switch	0xFF7E	*/ OSD_KEY_NONE,
/* XK_script_switch     0xFF7E  duplicated */
/* XK_Num_Lock		0xFF7F  */ OSD_KEY_NUMLOCK,
/* XK_KP_Space		0xFF80	*/ OSD_KEY_NONE,
/* 			0xFF81  */ OSD_KEY_NONE,
/* 			0xFF82  */ OSD_KEY_NONE,
/* 			0xFF83  */ OSD_KEY_NONE,
/* 			0xFF84  */ OSD_KEY_NONE,
/* 			0xFF85  */ OSD_KEY_NONE,
/* 			0xFF86  */ OSD_KEY_NONE,
/* 			0xFF87  */ OSD_KEY_NONE,
/* 			0xFF88  */ OSD_KEY_NONE,
/* XK_KP_Tab		0xFF89  */ OSD_KEY_TAB,
/* 			0xFF8A  */ OSD_KEY_NONE,
/* 			0xFF8B  */ OSD_KEY_NONE,
/* 			0xFF8C  */ OSD_KEY_NONE,
/* XK_KP_Enter		0xFF8D	*/ OSD_KEY_ENTER,
/* 			0xFF8E  */ OSD_KEY_NONE,
/* 			0xFF8F  */ OSD_KEY_NONE,
/* 			0xFF90  */ OSD_KEY_NONE,
/* XK_KP_F1		0xFF91	*/ OSD_KEY_F1,
/* XK_KP_F2		0xFF92  */ OSD_KEY_F2,
/* XK_KP_F3		0xFF93  */ OSD_KEY_F3,
/* XK_KP_F4		0xFF94  */ OSD_KEY_F4,
/* XK_KP_Home		0xFF95  */ OSD_KEY_HOME,
/* XK_KP_Left		0xFF96  */ OSD_KEY_LEFT,
/* XK_KP_Up		0xFF97  */ OSD_KEY_UP,
/* XK_KP_Right		0xFF98  */ OSD_KEY_RIGHT,
/* XK_KP_Down		0xFF99  */ OSD_KEY_DOWN,
/* XK_KP_Prior		0xFF9A  */ OSD_KEY_PGUP,
/* XK_KP_Page_Up	0xFF9A  duplicated */
/* XK_KP_Next		0xFF9B  */ OSD_KEY_PGDN,
/* XK_KP_Page_Down	0xFF9B  duplicated */
/* XK_KP_End		0xFF9C  */ OSD_KEY_END,
/* XK_KP_Begin		0xFF9D  */ OSD_KEY_HOME,
/* XK_KP_Insert		0xFF9E  */ OSD_KEY_INSERT,
/* XK_KP_Delete		0xFF9F  */ OSD_KEY_DEL,
/* 			0xFFA0  */ OSD_KEY_NONE,
/* 			0xFFA1  */ OSD_KEY_NONE,
/* 			0xFFA2  */ OSD_KEY_NONE,
/* 			0xFFA3  */ OSD_KEY_NONE,
/* 			0xFFA4  */ OSD_KEY_NONE,
/* 			0xFFA5  */ OSD_KEY_NONE,
/* 			0xFFA6  */ OSD_KEY_NONE,
/* 			0xFFA7  */ OSD_KEY_NONE,
/* 			0xFFA8  */ OSD_KEY_NONE,
/* 			0xFFA9  */ OSD_KEY_NONE,
/* XK_KP_Multiply	0xFFAA  */ OSD_KEY_ASTERISK,
/* XK_KP_Add		0xFFAB  */ OSD_KEY_PLUS_PAD,
/* XK_KP_Separator	0xFFAC	*/ OSD_KEY_NONE,
/* XK_KP_Subtract	0xFFAD  */ OSD_KEY_MINUS_PAD,
/* XK_KP_Decimal	0xFFAE  */ OSD_KEY_STOP,
/* XK_KP_Divide		0xFFAF  */ OSD_KEY_SLASH,
/* XK_KP_0		0xFFB0  */ OSD_KEY_0,
/* XK_KP_1		0xFFB1  */ OSD_KEY_1,
/* XK_KP_2		0xFFB2  */ OSD_KEY_2,
/* XK_KP_3		0xFFB3  */ OSD_KEY_3,
/* XK_KP_4		0xFFB4  */ OSD_KEY_4,
/* XK_KP_5		0xFFB5  */ OSD_KEY_5_PAD,
/* XK_KP_6		0xFFB6  */ OSD_KEY_6,
/* XK_KP_7		0xFFB7  */ OSD_KEY_7,
/* XK_KP_8		0xFFB8  */ OSD_KEY_8,
/* XK_KP_9		0xFFB9  */ OSD_KEY_9,
/* 			0xFFBA  */ OSD_KEY_NONE,
/* 			0xFFBB  */ OSD_KEY_NONE,
/* 			0xFFBC  */ OSD_KEY_NONE,
/* XK_KP_Equal		0xFFBD	*/ OSD_KEY_EQUALS,
/* XK_F1		0xFFBE  */ OSD_KEY_F1,
/* XK_F2		0xFFBF  */ OSD_KEY_F2,
/* XK_F3		0xFFC0  */ OSD_KEY_F3,
/* XK_F4		0xFFC1  */ OSD_KEY_F4,
/* XK_F5		0xFFC2  */ OSD_KEY_F5,
/* XK_F6		0xFFC3  */ OSD_KEY_F6,
/* XK_F7		0xFFC4  */ OSD_KEY_F7,
/* XK_F8		0xFFC5  */ OSD_KEY_F8,
/* XK_F9		0xFFC6  */ OSD_KEY_F9,
/* XK_F10		0xFFC7  */ OSD_KEY_F10,
/* XK_F11		0xFFC8  */ OSD_KEY_F11,
/* XK_L1		0xFFC8  duplicated */
/* XK_F12		0xFFC9  */ OSD_KEY_F12,
/* XK_L2		0xFFC9  duplicated */
/* XK_F13		0xFFCA  */ OSD_KEY_NONE,
/* XK_L3		0xFFCA  duplicated */
/* XK_F14		0xFFCB  */ OSD_KEY_NONE,
/* XK_L4		0xFFCB  duplicated */
/* XK_F15		0xFFCC  */ OSD_KEY_NONE,
/* XK_L5		0xFFCC  duplicated */
/* XK_F16		0xFFCD  */ OSD_KEY_NONE,
/* XK_L6		0xFFCD  duplicated */
/* XK_F17		0xFFCE  */ OSD_KEY_NONE,
/* XK_L7		0xFFCE  duplicated */
/* XK_F18		0xFFCF  */ OSD_KEY_NONE,
/* XK_L8		0xFFCF  duplicated */
/* XK_F19		0xFFD0  */ OSD_KEY_NONE,
/* XK_L9		0xFFD0  duplicated */
/* XK_F20		0xFFD1  */ OSD_KEY_NONE,
/* XK_L10		0xFFD1  duplicated */
/* XK_F21		0xFFD2  */ OSD_KEY_NONE,
/* XK_R1		0xFFD2  duplicated */
/* XK_F22		0xFFD3  */ OSD_KEY_NONE,
/* XK_R2		0xFFD3  duplicated */
/* XK_F23		0xFFD4  */ OSD_KEY_NONE,
/* XK_R3		0xFFD4  duplicated */
/* XK_F24		0xFFD5  */ OSD_KEY_NONE,
/* XK_R4		0xFFD5  duplicated */
/* XK_F25		0xFFD6  */ OSD_KEY_NONE,
/* XK_R5		0xFFD6  duplicated */
/* XK_F26		0xFFD7  */ OSD_KEY_NONE,
/* XK_R6		0xFFD7  duplicated */
/* XK_F27		0xFFD8  */ OSD_KEY_NONE,
/* XK_R7		0xFFD8  duplicated */
/* XK_F28		0xFFD9  */ OSD_KEY_NONE,
/* XK_R8		0xFFD9  duplicated */
/* XK_F29		0xFFDA  */ OSD_KEY_NONE,
/* XK_R9		0xFFDA  duplicated */
/* XK_F30		0xFFDB  */ OSD_KEY_NONE,
/* XK_R10		0xFFDB  duplicated */
/* XK_F31		0xFFDC  */ OSD_KEY_NONE,
/* XK_R11		0xFFDC  duplicated */
/* XK_F32		0xFFDD  */ OSD_KEY_NONE,
/* XK_R12		0xFFDD  duplicated */
/* XK_F33		0xFFDE  */ OSD_KEY_NONE,
/* XK_R13		0xFFDE  duplicated */
/* XK_F34		0xFFDF  */ OSD_KEY_NONE,
/* XK_R14		0xFFDF  duplicated */
/* XK_F35		0xFFE0  */ OSD_KEY_NONE,
/* XK_R15		0xFFE0  duplicated */
/* XK_Shift_L		0xFFE1	*/ OSD_KEY_LSHIFT,
/* XK_Shift_R		0xFFE2	*/ OSD_KEY_RSHIFT,
/* XK_Control_L		0xFFE3	*/ OSD_KEY_CONTROL,
/* XK_Control_R		0xFFE4	*/ OSD_KEY_CONTROL,
/* XK_Caps_Lock		0xFFE5	*/ OSD_KEY_CAPSLOCK,
/* XK_Shift_Lock	0xFFE6	*/ OSD_KEY_CAPSLOCK,
/* XK_Meta_L		0xFFE7	*/ OSD_KEY_ALT,
/* XK_Meta_R		0xFFE8	*/ OSD_KEY_ALT,
/* XK_Alt_L		0xFFE9	*/ OSD_KEY_ALT,
/* XK_Alt_R		0xFFEA	*/ OSD_KEY_ALT,
/* XK_Super_L		0xFFEB	*/ OSD_KEY_NONE,
/* XK_Super_R		0xFFEC	*/ OSD_KEY_NONE,
/* XK_Hyper_L		0xFFED	*/ OSD_KEY_NONE,
/* XK_Hyper_R		0xFFEE	*/ OSD_KEY_NONE,
/* 			0xFFEF  */ OSD_KEY_NONE,
/* 			0xFFF0  */ OSD_KEY_NONE,
/* 			0xFFF1  */ OSD_KEY_NONE,
/* 			0xFFF2  */ OSD_KEY_NONE,
/* 			0xFFF3  */ OSD_KEY_NONE,
/* 			0xFFF4  */ OSD_KEY_NONE,
/* 			0xFFF5  */ OSD_KEY_NONE,
/* 			0xFFF6  */ OSD_KEY_NONE,
/* 			0xFFF7  */ OSD_KEY_NONE,
/* 			0xFFF8  */ OSD_KEY_NONE,
/* 			0xFFF9  */ OSD_KEY_NONE,
/* 			0xFFFA  */ OSD_KEY_NONE,
/* 			0xFFFB  */ OSD_KEY_NONE,
/* 			0xFFFC  */ OSD_KEY_NONE,
/* 			0xFFFD  */ OSD_KEY_NONE,
/* 			0xFFFE  */ OSD_KEY_NONE,
/* XK_Delete		0xFFFF	*/ OSD_KEY_DEL
}; 	/* extended_code_table */

static int code_table[] = {

/* 			  0x000  */ OSD_KEY_NONE,
/* 			  0x001  */ OSD_KEY_NONE,
/* 			  0x002  */ OSD_KEY_NONE,
/* 			  0x003  */ OSD_KEY_NONE,
/* 			  0x004  */ OSD_KEY_NONE,
/* 			  0x005  */ OSD_KEY_NONE,
/* 			  0x006  */ OSD_KEY_NONE,
/* 			  0x007  */ OSD_KEY_NONE,
/* 			  0x008  */ OSD_KEY_NONE,
/* 			  0x009  */ OSD_KEY_NONE,
/* 			  0x00a  */ OSD_KEY_NONE,
/* 			  0x00b  */ OSD_KEY_NONE,
/* 			  0x00c  */ OSD_KEY_NONE,
/* 			  0x00d  */ OSD_KEY_NONE,
/* 			  0x00e  */ OSD_KEY_NONE,
/* 			  0x00f  */ OSD_KEY_NONE,
/* 			  0x010  */ OSD_KEY_NONE,
/* 			  0x011  */ OSD_KEY_NONE,
/* 			  0x012  */ OSD_KEY_NONE,
/* 			  0x013  */ OSD_KEY_NONE,
/* 			  0x014  */ OSD_KEY_NONE,
/* 			  0x015  */ OSD_KEY_NONE,
/* 			  0x016  */ OSD_KEY_NONE,
/* 			  0x017  */ OSD_KEY_NONE,
/* 			  0x018  */ OSD_KEY_NONE,
/* 			  0x019  */ OSD_KEY_NONE,
/* 			  0x01a  */ OSD_KEY_NONE,
/* 			  0x01b  */ OSD_KEY_NONE,
/* 			  0x01c  */ OSD_KEY_NONE,
/* 			  0x01d  */ OSD_KEY_NONE,
/* 			  0x01e  */ OSD_KEY_NONE,
/* 			  0x01f  */ OSD_KEY_NONE,
/* XK_space               0x020  */ OSD_KEY_SPACE,
/* XK_exclam              0x021  */ OSD_KEY_1,
/* XK_quotedbl            0x022  */ OSD_KEY_QUOTE,
/* XK_numbersign          0x023  */ OSD_KEY_3,
/* XK_dollar              0x024  */ OSD_KEY_4,
/* XK_percent             0x025  */ OSD_KEY_5,
/* XK_ampersand           0x026  */ OSD_KEY_7,
/* XK_apostrophe          0x027  */ OSD_KEY_MINUS, /* keyboard dependent */
/* XK_quoteright          0x027	 duplicated */ 
/* XK_parenleft           0x028  */ OSD_KEY_9,
/* XK_parenright          0x029  */ OSD_KEY_0,
/* XK_asterisk            0x02a  */ OSD_KEY_ASTERISK,
/* XK_plus                0x02b  */ OSD_KEY_CLOSEBRACE, /* keyboard dependent */
/* XK_comma               0x02c  */ OSD_KEY_COMMA,
/* XK_minus               0x02d  */ OSD_KEY_MINUS,
/* XK_period              0x02e  */ OSD_KEY_STOP,
/* XK_slash               0x02f  */ OSD_KEY_SLASH,
/* XK_0                   0x030  */ OSD_KEY_0,
/* XK_1                   0x031  */ OSD_KEY_1,
/* XK_2                   0x032  */ OSD_KEY_2,
/* XK_3                   0x033  */ OSD_KEY_3,
/* XK_4                   0x034  */ OSD_KEY_4,
/* XK_5                   0x035  */ OSD_KEY_5,
/* XK_6                   0x036  */ OSD_KEY_6,
/* XK_7                   0x037  */ OSD_KEY_7,
/* XK_8                   0x038  */ OSD_KEY_8,
/* XK_9                   0x039  */ OSD_KEY_9,
/* XK_colon               0x03a  */ OSD_KEY_COLON,
/* XK_semicolon           0x03b  */ OSD_KEY_COLON,
/* XK_less                0x03c  */ OSD_KEY_COMMA,
/* XK_equal               0x03d  */ OSD_KEY_EQUALS,
/* XK_greater             0x03e  */ OSD_KEY_STOP,
/* XK_question            0x03f  */ OSD_KEY_SLASH,
/* XK_at                  0x040  */ OSD_KEY_2,
/* XK_A                   0x041  */ OSD_KEY_A,
/* XK_B                   0x042  */ OSD_KEY_B,
/* XK_C                   0x043  */ OSD_KEY_C,
/* XK_D                   0x044  */ OSD_KEY_D,
/* XK_E                   0x045  */ OSD_KEY_E,
/* XK_F                   0x046  */ OSD_KEY_F,
/* XK_G                   0x047  */ OSD_KEY_G,
/* XK_H                   0x048  */ OSD_KEY_H,
/* XK_I                   0x049  */ OSD_KEY_I,
/* XK_J                   0x04a  */ OSD_KEY_J,
/* XK_K                   0x04b  */ OSD_KEY_K,
/* XK_L                   0x04c  */ OSD_KEY_L,
/* XK_M                   0x04d  */ OSD_KEY_M,
/* XK_N                   0x04e  */ OSD_KEY_N,
/* XK_O                   0x04f  */ OSD_KEY_O,
/* XK_P                   0x050  */ OSD_KEY_P,
/* XK_Q                   0x051  */ OSD_KEY_Q,
/* XK_R                   0x052  */ OSD_KEY_R,
/* XK_S                   0x053  */ OSD_KEY_S,
/* XK_T                   0x054  */ OSD_KEY_T,
/* XK_U                   0x055  */ OSD_KEY_U,
/* XK_V                   0x056  */ OSD_KEY_V,
/* XK_W                   0x057  */ OSD_KEY_W,
/* XK_X                   0x058  */ OSD_KEY_X,
/* XK_Y                   0x059  */ OSD_KEY_Y,
/* XK_Z                   0x05a  */ OSD_KEY_Z,
/* XK_bracketleft         0x05b  */ OSD_KEY_OPENBRACE,
/* XK_backslash           0x05c  */ OSD_KEY_NONE,
/* XK_bracketright        0x05d  */ OSD_KEY_CLOSEBRACE,
/* XK_asciicircum         0x05e  */ OSD_KEY_6,
/* XK_underscore          0x05f  */ OSD_KEY_MINUS,
/* XK_grave               0x060  */ OSD_KEY_OPENBRACE, /* keyboard dependent */
/* XK_quoteleft           0x060  duplicated */
/* XK_a                   0x061  */ OSD_KEY_A,
/* XK_b                   0x062  */ OSD_KEY_B,
/* XK_c                   0x063  */ OSD_KEY_C,
/* XK_d                   0x064  */ OSD_KEY_D,
/* XK_e                   0x065  */ OSD_KEY_E,
/* XK_f                   0x066  */ OSD_KEY_F,
/* XK_g                   0x067  */ OSD_KEY_G,
/* XK_h                   0x068  */ OSD_KEY_H,
/* XK_i                   0x069  */ OSD_KEY_I,
/* XK_j                   0x06a  */ OSD_KEY_J,
/* XK_k                   0x06b  */ OSD_KEY_K,
/* XK_l                   0x06c  */ OSD_KEY_L,
/* XK_m                   0x06d  */ OSD_KEY_M,
/* XK_n                   0x06e  */ OSD_KEY_N,
/* XK_o                   0x06f  */ OSD_KEY_O,
/* XK_p                   0x070  */ OSD_KEY_P,
/* XK_q                   0x071  */ OSD_KEY_Q,
/* XK_r                   0x072  */ OSD_KEY_R,
/* XK_s                   0x073  */ OSD_KEY_S,
/* XK_t                   0x074  */ OSD_KEY_T,
/* XK_u                   0x075  */ OSD_KEY_U,
/* XK_v                   0x076  */ OSD_KEY_V,
/* XK_w                   0x077  */ OSD_KEY_W,
/* XK_x                   0x078  */ OSD_KEY_X,
/* XK_y                   0x079  */ OSD_KEY_Y,
/* XK_z                   0x07a  */ OSD_KEY_Z,
/* XK_braceleft           0x07b  */ OSD_KEY_OPENBRACE,
/* XK_bar                 0x07c  */ OSD_KEY_NONE,
/* XK_braceright          0x07d  */ OSD_KEY_CLOSEBRACE,
/* XK_asciitilde          0x07e  */ OSD_KEY_TILDE,
/*                        0x07f  */ OSD_KEY_NONE,
/* 			  0x080  */ OSD_KEY_NONE,
/* 			  0x081  */ OSD_KEY_NONE,
/* 			  0x082  */ OSD_KEY_NONE,
/* 			  0x083  */ OSD_KEY_NONE,
/* 			  0x084  */ OSD_KEY_NONE,
/* 			  0x085  */ OSD_KEY_NONE,
/* 			  0x086  */ OSD_KEY_NONE,
/* 			  0x087  */ OSD_KEY_NONE,
/* 			  0x088  */ OSD_KEY_NONE,
/* 			  0x089  */ OSD_KEY_NONE,
/* 			  0x08a  */ OSD_KEY_NONE,
/* 			  0x08b  */ OSD_KEY_NONE,
/* 			  0x08c  */ OSD_KEY_NONE,
/* 			  0x08d  */ OSD_KEY_NONE,
/* 			  0x08e  */ OSD_KEY_NONE,
/* 			  0x08f  */ OSD_KEY_NONE,
/* 			  0x090  */ OSD_KEY_NONE,
/* 			  0x091  */ OSD_KEY_NONE,
/* 			  0x092  */ OSD_KEY_NONE,
/* 			  0x093  */ OSD_KEY_NONE,
/* 			  0x094  */ OSD_KEY_NONE,
/* 			  0x095  */ OSD_KEY_NONE,
/* 			  0x096  */ OSD_KEY_NONE,
/* 			  0x097  */ OSD_KEY_NONE,
/* 			  0x098  */ OSD_KEY_NONE,
/* 			  0x099  */ OSD_KEY_NONE,
/* 			  0x09a  */ OSD_KEY_NONE,
/* 			  0x09b  */ OSD_KEY_NONE,
/* 			  0x09c  */ OSD_KEY_NONE,
/* 			  0x09d  */ OSD_KEY_NONE,
/* 			  0x09e  */ OSD_KEY_NONE,
/* 			  0x09f  */ OSD_KEY_NONE,
/* XK_nobreakspace        0x0a0  */ OSD_KEY_NONE,
/* XK_exclamdown          0x0a1  */ OSD_KEY_EQUALS, /* keyboard dependent */
/* XK_cent                0x0a2  */ OSD_KEY_NONE,
/* XK_sterling            0x0a3  */ OSD_KEY_NONE,
/* XK_currency            0x0a4  */ OSD_KEY_NONE,
/* XK_yen                 0x0a5  */ OSD_KEY_NONE,
/* XK_brokenbar           0x0a6  */ OSD_KEY_NONE,
/* XK_section             0x0a7  */ OSD_KEY_NONE,
/* XK_diaeresis           0x0a8  */ OSD_KEY_NONE,
/* XK_copyright           0x0a9  */ OSD_KEY_NONE,
/* XK_ordfeminine         0x0aa  */ OSD_KEY_NONE,
/* XK_guillemotleft       0x0ab  */ OSD_KEY_NONE,
/* XK_notsign             0x0ac  */ OSD_KEY_NONE,
/* XK_hyphen              0x0ad  */ OSD_KEY_NONE,
/* XK_registered          0x0ae  */ OSD_KEY_NONE,
/* XK_macron              0x0af  */ OSD_KEY_NONE,
/* XK_degree              0x0b0  */ OSD_KEY_NONE,
/* XK_plusminus           0x0b1  */ OSD_KEY_NONE,
/* XK_twosuperior         0x0b2  */ OSD_KEY_NONE,
/* XK_treesuperior        0x0b3  */ OSD_KEY_NONE,
/* XK_acute               0x0b4  */ OSD_KEY_QUOTE, /* keyboard dependent */
/* XK_mu                  0x0b5  */ OSD_KEY_NONE,
/* XK_paragraph           0x0b6  */ OSD_KEY_NONE,
/* XK_periodcentered      0x0b7  */ OSD_KEY_NONE,
/* XK_cedilla             0x0b8  */ OSD_KEY_NONE,
/* XK_onesuperior         0x0b9  */ OSD_KEY_NONE,
/* XK_masculine           0x0ba  */ OSD_KEY_NONE,
/* XK_guillemotright      0x0bb  */ OSD_KEY_NONE,
/* XK_onequarter          0x0bc  */ OSD_KEY_NONE,
/* XK_onehalf             0x0bd  */ OSD_KEY_NONE,
/* XK_threequarters       0x0be  */ OSD_KEY_NONE,
/* XK_questiondown        0x0bf  */ OSD_KEY_NONE,
/* XK_Agrave              0x0c0  */ OSD_KEY_NONE,
/* XK_Aacute              0x0c1  */ OSD_KEY_NONE,
/* XK_Acircumflex         0x0c2  */ OSD_KEY_NONE,
/* XK_Atilde              0x0c3  */ OSD_KEY_NONE,
/* XK_Adiaeresis          0x0c4  */ OSD_KEY_NONE,
/* XK_Aring               0x0c5  */ OSD_KEY_NONE,
/* XK_AE                  0x0c6  */ OSD_KEY_NONE,
/* XK_Ccedilla            0x0c7  */ OSD_KEY_NONE,
/* XK_Egrave              0x0c8  */ OSD_KEY_NONE,
/* XK_Eacute              0x0c9  */ OSD_KEY_NONE,
/* XK_Ecircumflex         0x0ca  */ OSD_KEY_NONE,
/* XK_Ediaeresis          0x0cb  */ OSD_KEY_NONE,
/* XK_Igrave              0x0cc  */ OSD_KEY_NONE,
/* XK_Iacute              0x0cd  */ OSD_KEY_NONE,
/* XK_Icircumflex         0x0ce  */ OSD_KEY_NONE,
/* XK_Idiaeresis          0x0cf  */ OSD_KEY_NONE,
/* XK_ETH                 0x0d0  */ OSD_KEY_NONE,
/* XK_Eth                 0x0d0  duplicated */
/* XK_Ntilde              0x0d1  */ OSD_KEY_NONE,
/* XK_Ograve              0x0d2  */ OSD_KEY_NONE,
/* XK_Oacute              0x0d3  */ OSD_KEY_NONE,
/* XK_Ocircumflex         0x0d4  */ OSD_KEY_NONE,
/* XK_Otilde              0x0d5  */ OSD_KEY_NONE,
/* XK_Odiaeresis          0x0d6  */ OSD_KEY_NONE,
/* XK_multiply            0x0d7  */ OSD_KEY_NONE,
/* XK_Ooblique            0x0d8  */ OSD_KEY_NONE,
/* XK_Ugrave              0x0d9  */ OSD_KEY_NONE,
/* XK_Uacute              0x0da  */ OSD_KEY_NONE,
/* XK_Ucircumflex         0x0db  */ OSD_KEY_NONE,
/* XK_Udiaeresis          0x0dc  */ OSD_KEY_NONE,
/* XK_Yacute              0x0dd  */ OSD_KEY_NONE,
/* XK_THORN               0x0de  */ OSD_KEY_NONE,
/* XK_Thorn               0x0de  duplicated */
/* XK_ssharp              0x0df  */ OSD_KEY_NONE,
/* XK_agrave              0x0e0  */ OSD_KEY_NONE,
/* XK_aacute              0x0e1  */ OSD_KEY_NONE,
/* XK_acircumflex         0x0e2  */ OSD_KEY_NONE,
/* XK_atilde              0x0e3  */ OSD_KEY_NONE,
/* XK_adiaeresis          0x0e4  */ OSD_KEY_NONE,
/* XK_aring               0x0e5  */ OSD_KEY_NONE,
/* XK_ae                  0x0e6  */ OSD_KEY_NONE,
/* XK_ccedilla            0x0e7  */ OSD_KEY_SLASH, /* keyboard dependent */
/* XK_egrave              0x0e8  */ OSD_KEY_NONE,
/* XK_eacute              0x0e9  */ OSD_KEY_NONE,
/* XK_ecircumflex         0x0ea  */ OSD_KEY_NONE,
/* XK_ediaeresis          0x0eb  */ OSD_KEY_NONE,
/* XK_igrave              0x0ec  */ OSD_KEY_NONE,
/* XK_iacute              0x0ed  */ OSD_KEY_NONE,
/* XK_icircumflex         0x0ee  */ OSD_KEY_NONE,
/* XK_idiaeresis          0x0ef  */ OSD_KEY_NONE,
/* XK_eth                 0x0f0  */ OSD_KEY_NONE,
/* XK_ntilde              0x0f1  */ OSD_KEY_COLON, /* keyboard dependent */
/* XK_ograve              0x0f2  */ OSD_KEY_NONE,
/* XK_oacute              0x0f3  */ OSD_KEY_NONE,
/* XK_ocircumflex         0x0f4  */ OSD_KEY_NONE,
/* XK_otilde              0x0f5  */ OSD_KEY_NONE,
/* XK_odiaeresis          0x0f6  */ OSD_KEY_NONE,
/* XK_division            0x0f7  */ OSD_KEY_NONE,
/* XK_oslash              0x0f8  */ OSD_KEY_NONE,
/* XK_ugrave              0x0f9  */ OSD_KEY_NONE,
/* XK_uacute              0x0fa  */ OSD_KEY_NONE,
/* XK_ucircumflex         0x0fb  */ OSD_KEY_NONE,
/* XK_udiaeresis          0x0fc  */ OSD_KEY_NONE,
/* XK_yacute              0x0fd  */ OSD_KEY_NONE,
/* XK_thorn               0x0fe  */ OSD_KEY_NONE,
/* XK_ydiaeresis          0x0ff  */ OSD_KEY_NONE

}; /* code_table */

/* return the name of a key */
const char *osd_key_name(int keycode)
{
  static char *keynames[] = { 
	"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y",
        "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "CONTROL",
        "A", "S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
        "TILDE", "LEFTSHIFT", "NULL", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
        "STOP", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK",
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",
        "SCRLOCK", "HOME", "UP", "PGUP", "MINUS PAD", "LEFT", "5 PAD", "RIGHT",
        "PLUS PAD", "END", "DOWN", "PGDN", "INS", "DEL", "F11", "F12" 
	};

  if (keycode && keycode <= OSD_MAX_KEY) return (char*)keynames[keycode-1];
  else return 0;
}

/*
 *  initialize keyboard system
 */
int sysdep_keyboard_init(void)
{
	int i;
	for (i=0;i<OSD_NUMKEYS;i++) key[i]=FALSE;
	return 0;
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from; 2-> invalid to
 */

int sysdep_mapkey(int from, int to)
{
	/* perform tests */
/* fprintf(stderr,"trying to map %x to%x\n",from,to); */
	if ( (to<0) || (to>OSD_NUMKEYS) )return 2;
	if ( (from>=0) && (from<=0x00ff) ) 
			{code_table[from]=to; return 0;}
	if ( (from>=0xff00) && (from<=0xffff) ) 
			{extended_code_table[from&0x00ff]=to; return 0;}
	return 1; 
}

/*
 * Wait until a key is pressed and return its code. Real shit: should use
 *    select() call to wait for incoming requests :-( 
*/

int osd_read_key(void)
{
	static int i=1;
	while(1) {
		osd_key_pressed(-1); /* empty events queue */
		for(; i< OSD_NUMKEYS; i++) {
			if (key[i]) {
/* fprintf(stderr,"Retornada tecla %d\n",i); */
				return i;
			}
		}
		i=1;
	}
}

/*
 * Parse keyboard events
 * If request < 0 empty event queue
 * If request >=0 get events until queue empty or specified key event found 
*/

int osd_key_pressed (int request)
{
  XEvent 		E;
  int	 		keycode,code;
  int			mask;
  int 			*pt;
  while (XCheckWindowEvent (display,window, KeyPressMask | KeyReleaseMask, &E))
  {
    mask = TRUE; 	/* In case of KeyPress. */
    if ((E.type == KeyPress) || (E.type == KeyRelease))
    {
	if (E.type == KeyRelease) mask = FALSE;
	keycode = XLookupKeysym ((XKeyEvent *) &E, 0);
	/* look which table should be used */
	pt=code_table;
        if ( (keycode & 0xff00) == 0xff00 ) pt=extended_code_table;
	code=keycode&0x00ff;
	/* if unnasigned key ignore it */
	if ( *(pt+code) ) key [ *(pt+code) ] = mask;
    } /* if */
  } /* while */
/* fprintf(stderr,"%8X %d\n",keycode,*(pt+code)); */
  if (request >= 0) {
/* fprintf(stderr,"Request:%d return:%d\n",request,key[request]); */
	return key[request];
  }
  else return (FALSE);
}


