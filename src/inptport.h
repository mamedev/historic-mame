/***************************************************************************

	inptport.h

	Handle input ports and mappings.

***************************************************************************/

#ifndef INPTPORT_H
#define INPTPORT_H

#include "memory.h"
#include "input.h"

#ifdef MESS
#include "unicode.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*************************************
 *
 *	Constants
 *
 *************************************/

#define MAX_INPUT_PORTS		30
#define MAX_PLAYERS			8

#define IP_ACTIVE_HIGH		0x00000000
#define IP_ACTIVE_LOW		0xffffffff


/* sequence types for input_port_seq() call */
enum
{
	SEQ_TYPE_STANDARD = 0,
	SEQ_TYPE_INCREMENT = 1,
	SEQ_TYPE_DECREMENT = 2
};


/* conditions for DIP switches */
enum
{
	PORTCOND_ALWAYS = 0,
	PORTCOND_EQUALS,
	PORTCOND_NOTEQUALS
};


/* groups for input ports */
enum
{
	IPG_UI = 0,
	IPG_PLAYER1,
	IPG_PLAYER2,
	IPG_PLAYER3,
	IPG_PLAYER4,
	IPG_PLAYER5,
	IPG_PLAYER6,
	IPG_PLAYER7,
	IPG_PLAYER8,
	IPG_OTHER,
	IPG_TOTAL_GROUPS,
	IPG_INVALID
};


/* various input port types */
enum
{
	/* pseudo-port types */
	IPT_INVALID = 0,
	IPT_UNUSED,
	IPT_END,
	IPT_UNKNOWN,
	IPT_PORT,
	IPT_DIPSWITCH_NAME,
	IPT_DIPSWITCH_SETTING,
	IPT_VBLANK,
	IPT_KEYBOARD,				/* MESS only */
	IPT_CONFIG_NAME,			/* MESS only */
	IPT_CONFIG_SETTING,			/* MESS only */
	IPT_START,					/* MESS only */
	IPT_SELECT,					/* MESS only */
	IPT_CATEGORY_NAME,			/* MESS only */
	IPT_CATEGORY_SETTING,		/* MESS only */

#define __ipt_digital_joystick_start IPT_JOYSTICK_UP
	/* use IPT_JOYSTICK for panels where the player has one single joystick */
	IPT_JOYSTICK_UP,
	IPT_JOYSTICK_DOWN,
	IPT_JOYSTICK_LEFT,
	IPT_JOYSTICK_RIGHT,

	/* use IPT_JOYSTICKLEFT and IPT_JOYSTICKRIGHT for dual joystick panels */
	IPT_JOYSTICKRIGHT_UP,
	IPT_JOYSTICKRIGHT_DOWN,
	IPT_JOYSTICKRIGHT_LEFT,
	IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP,
	IPT_JOYSTICKLEFT_DOWN,
	IPT_JOYSTICKLEFT_LEFT,
	IPT_JOYSTICKLEFT_RIGHT,
#define __ipt_digital_joystick_end IPT_JOYSTICKLEFT_RIGHT
	
	/* action buttons */
	IPT_BUTTON1,
	IPT_BUTTON2,
	IPT_BUTTON3,
	IPT_BUTTON4,
	IPT_BUTTON5,
	IPT_BUTTON6,
	IPT_BUTTON7,
	IPT_BUTTON8,
	IPT_BUTTON9,
	IPT_BUTTON10,

	/* start buttons */
	IPT_START1,
	IPT_START2,
	IPT_START3,
	IPT_START4,
	IPT_START5,
	IPT_START6,
	IPT_START7,
	IPT_START8,
	
	/* coin slots */
	IPT_COIN1,
	IPT_COIN2,
	IPT_COIN3,
	IPT_COIN4,
	IPT_COIN5,
	IPT_COIN6,
	IPT_COIN7,
	IPT_COIN8,
	IPT_BILL1,

	/* service coin */
	IPT_SERVICE1,
	IPT_SERVICE2,
	IPT_SERVICE3,
	IPT_SERVICE4,

	/* misc other digital inputs */
	IPT_SERVICE,
	IPT_TILT,
	IPT_INTERLOCK,
	IPT_VOLUME_UP,
	IPT_VOLUME_DOWN,
	
	/* analog inputs */
#define __ipt_analog_start IPT_PADDLE
	IPT_PADDLE,			/* absolute */
	IPT_PADDLE_V,		/* absolute */
	IPT_AD_STICK_X,		/* absolute */
	IPT_AD_STICK_Y,		/* absolute */
	IPT_AD_STICK_Z,		/* absolute */
	IPT_LIGHTGUN_X,		/* absolute */
	IPT_LIGHTGUN_Y,		/* absolute */
	IPT_PEDAL,			/* absolute */
	IPT_PEDAL2,			/* absolute */
	IPT_PEDAL3,			/* absolute */
	IPT_DIAL,			/* relative */
	IPT_DIAL_V,			/* relative */
	IPT_TRACKBALL_X,	/* relative */
	IPT_TRACKBALL_Y,	/* relative */
	IPT_MOUSE_X,		/* relative */
	IPT_MOUSE_Y,		/* relative */
#define __ipt_analog_end IPT_MOUSE_Y

	/* Analog adjuster support */
	IPT_ADJUSTER,

	/* the following are special codes for user interface handling - not to be used by drivers! */
	IPT_UI_CONFIGURE,
	IPT_UI_ON_SCREEN_DISPLAY,
	IPT_UI_PAUSE,
	IPT_UI_RESET_MACHINE,
	IPT_UI_SHOW_GFX,
	IPT_UI_FRAMESKIP_DEC,
	IPT_UI_FRAMESKIP_INC,
	IPT_UI_THROTTLE,
	IPT_UI_SHOW_FPS,
	IPT_UI_SNAPSHOT,
	IPT_UI_TOGGLE_CHEAT,
	IPT_UI_UP,
	IPT_UI_DOWN,
	IPT_UI_LEFT,
	IPT_UI_RIGHT,
	IPT_UI_SELECT,
	IPT_UI_CANCEL,
	IPT_UI_PAN_UP,
	IPT_UI_PAN_DOWN,
	IPT_UI_PAN_LEFT,
	IPT_UI_PAN_RIGHT,
	IPT_UI_SHOW_PROFILER,
	IPT_UI_TOGGLE_UI,
	IPT_UI_TOGGLE_DEBUG,
	IPT_UI_SAVE_STATE,
	IPT_UI_LOAD_STATE,
	IPT_UI_ADD_CHEAT,
	IPT_UI_DELETE_CHEAT,
	IPT_UI_SAVE_CHEAT,
	IPT_UI_WATCH_VALUE,
	IPT_UI_EDIT_CHEAT,
	IPT_UI_TOGGLE_CROSSHAIR,

	/* additional OSD-specified UI port types (up to 16) */
	IPT_OSD_1,
	IPT_OSD_2,
	IPT_OSD_3,
	IPT_OSD_4,
	IPT_OSD_5,
	IPT_OSD_6,
	IPT_OSD_7,
	IPT_OSD_8,
	IPT_OSD_9,
	IPT_OSD_10,
	IPT_OSD_11,
	IPT_OSD_12,
	IPT_OSD_13,
	IPT_OSD_14,
	IPT_OSD_15,
	IPT_OSD_16,

	/* other meaning not mapped to standard defaults */
	IPT_OTHER,

	/* special meaning handled by custom code */
	IPT_SPECIAL,

	__ipt_max
};


/* default strings used in port definitions */
enum
{
	STR_Off = 0,
	STR_On,
	STR_No,
	STR_Yes,
	STR_Lives,
	STR_Bonus_Life,
	STR_Difficulty,
	STR_Demo_Sounds,
	STR_Coinage,
	STR_Coin_A,
	STR_Coin_B,
	STR_9C_1C,
	STR_8C_1C,
	STR_7C_1C,
	STR_6C_1C,
	STR_5C_1C,
	STR_4C_1C,
	STR_3C_1C,
	STR_8C_3C,
	STR_4C_2C,
	STR_2C_1C,
	STR_5C_3C,
	STR_3C_2C,
	STR_4C_3C,
	STR_4C_4C,
	STR_3C_3C,
	STR_2C_2C,
	STR_1C_1C,
	STR_4C_5C,
	STR_3C_4C,
	STR_2C_3C,
	STR_4C_7C,
	STR_2C_4C,
	STR_1C_2C,
	STR_2C_5C,
	STR_2C_6C,
	STR_1C_3C,
	STR_2C_7C,
	STR_2C_8C,
	STR_1C_4C,
	STR_1C_5C,
	STR_1C_6C,
	STR_1C_7C,
	STR_1C_8C,
	STR_1C_9C,
	STR_Free_Play,
	STR_Cabinet,
	STR_Upright,
	STR_Cocktail,
	STR_Flip_Screen,
	STR_Service_Mode,
	STR_Pause,
	STR_Test,
	STR_Tilt,
	STR_Version,
	STR_Region,
	STR_International,
	STR_Japan,
	STR_USA,
	STR_Europe,
	STR_Asia,
	STR_World,
	STR_Hispanic,
	STR_Language,
	STR_English,
	STR_Japanese,
	STR_German,
	STR_French,
	STR_Italian,
	STR_Spanish,
	STR_Very_Easy,
	STR_Easy,
	STR_Normal,
	STR_Medium,
	STR_Hard,
	STR_Harder,
	STR_Hardest,
	STR_Very_Hard,
	STR_Very_Low,
	STR_Low,
	STR_High,
	STR_Higher,
	STR_Highest,
	STR_Very_High,
	STR_Players,
	STR_Controls,
	STR_Dual,
	STR_Single,
	STR_Game_Time,
	STR_Continue_Price,
	STR_Controller,
	STR_Light_Gun,
	STR_Joystick,
	STR_Trackball,
	STR_Continues,
	STR_Allow_Continue,
	STR_Level_Select,
	STR_Infinite,
	STR_Stereo,
	STR_Mono,
	STR_Unused,
	STR_Unknown,
	STR_Standard,
	STR_Reverse,
	STR_Alternate,
	STR_None,
	STR_TOTAL
};



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct IptInitParams;

struct InputPortDefinition
{
	UINT32		type;			/* type of port; see enum above */
	UINT8		group;			/* which group the port belongs to */
	UINT8		player;			/* player number (0 is player 1) */
	const char *token;			/* token used to store settings */
	const char *name;			/* user-friendly name */
	input_seq_t	defaultseq;		/* default input sequence */
	input_seq_t	defaultincseq;	/* default input sequence to increment (analog ports only) */
	input_seq_t	defaultdecseq;	/* default input sequence to decrement (analog ports only) */
};


struct InputPort
{
	UINT32		mask;			/* bits affected */
	UINT32		default_value;	/* default value for the bits affected */
								/* you can also use one of the IP_ACTIVE defines above */
	UINT32		type;			/* see enum above */
	UINT8		unused;			/* The bit is not used by this game, but is used */
								/* by other games running on the same hardware. */
								/* This is different from IPT_UNUSED, which marks */
								/* bits not connected to anything. */
	UINT8		cocktail;		/* the bit is used in cocktail mode only */
	UINT8		player;			/* the player associated with this port; note that */
								/* player 1 is '0' */
	UINT8		toggle;			/* When this is set, the key acts as a toggle - press */
								/* it once and it goes on, press it again and it goes off. */
								/* useful e.g. for some Test Mode dip switches. */
	UINT8		impulse;		/* When this is set, when the key corresponding to */
								/* the input bit is pressed it will be reported as */
								/* pressed for a certain number of video frames and */
								/* then released, regardless of the real status of */
								/* the key. This is useful e.g. for some coin inputs. */
								/* The number of frames the signal should stay active */
								/* is specified in the "arg" field. */
	UINT8		four_way;		/* Joystick modes of operation. 8WAY is the default, */
								/* it prevents left/right or up/down to be pressed at */
								/* the same time. 4WAY prevents diagonal directions. */
								/* 2WAY should be used for joysticks wich move only */
								/* on one axis (e.g. Battle Zone) */
	const char *name;			/* user-friendly name to display */
	input_seq_t	seq;			/* input sequence affecting the input bits */
	UINT16		category;		/* (MESS-specific) category */

	/* valid if type is between __ipt_analog_start and __ipt_analog_end */
	struct
	{
		INT32	min;			/* minimum value for absolute axes */
		INT32	max;			/* maximum value for absolute axes */
		INT32	sensitivity;	/* sensitivity (100=normal) */
		INT32	delta;			/* delta to apply each frame a digital inc/dec key is pressed */
		INT32	centerdelta;	/* delta to apply each frame no digital inputs are pressed */
		UINT8	reverse;		/* reverse the sense of the analog axis */
		UINT8	reset;			/* always preload in->default for relative axes, returning only deltas */
		input_seq_t incseq;		/* increment sequence */
		input_seq_t decseq;		/* decrement sequence */
	} analog;
	
	/* valid if type is IPT_PORT */
	struct
	{
		const char *tag;		/* used to tag PORT_START declarations */
	} start;

	/* valid if type is IPT_DIPSWITCH_SETTING */
	struct
	{
		UINT8	portnum;		/* portnumber to use for condition */
		UINT8	condition;		/* condition to use */
		UINT32	mask;			/* mask to apply to the portnum */
		UINT32	value;			/* value to compare against */
	} dipsetting;

	/* valid if type is IPT_KEYBOARD */
#ifdef MESS
	struct
	{
		unicode_char_t chars[3];/* (MESS-specific) unicode key data */
	} keyboard;
#endif
};



/*************************************
 *
 *	Macros for building input ports
 *
 *************************************/

#define IP_NAME_DEFAULT 	NULL

/* start of table */
#define INPUT_PORTS_START(name)										\
 	void construct_ipt_##name(struct IptInitParams *param)			\
	{																\
 		struct InputPort *port;										\
		int seq_index[3];											\
		int key;													\
 		(void) port; (void) seq_index; (void) key;					\

/* end of table */
#define INPUT_PORTS_END												\
	}																\

/* aliasing */
#define INPUT_PORTS_ALIAS(name, base)								\
 	void construct_ipt_##name(struct IptInitParams *param)			\
	{																\
 		construct_ipt_##base(param);								\
	}																\

#define INPUT_PORTS_EXTERN(name)									\
	extern void construct_ipt_##name(struct IptInitParams *param)	\

/* including */
#define PORT_INCLUDE(name)											\
 	construct_ipt_##name(param);									\

/* start of a new input port */
#define PORT_START													\
	port = input_port_initialize(param, IPT_PORT);					\

/* start of a new input port */
#define PORT_START_TAG(tag_)										\
	port = input_port_initialize(param, IPT_PORT);					\
	port->start.tag = (tag_);										\

/* input bit definition */
#define PORT_BIT(mask_,default_,type_) 								\
	port = input_port_initialize(param, (type_));					\
	port->mask = (mask_);											\
	port->default_value = (default_);								\
	seq_index[0] = seq_index[1] = seq_index[2] = key = 0;			\

/* new technique to append a code */
#define PORT_CODE_SEQ(code_,seq_,si_)								\
	if ((code_) < __code_max)										\
	{																\
		if (seq_index[si_] > 0)										\
			port->seq_.code[seq_index[si_]++] = CODE_OR;			\
		port->seq_.code[seq_index[si_]++] = (code_);				\
	}																\

#define PORT_CODE(code) PORT_CODE_SEQ(code,seq,0)
#define PORT_CODE_DEC(code)	PORT_CODE_SEQ(code,analog.decseq,1)
#define PORT_CODE_INC(code)	PORT_CODE_SEQ(code,analog.incseq,2)

#define PORT_2WAY													\
	port->four_way = 0;												\

#define PORT_4WAY													\
	port->four_way = 1;												\

#define PORT_8WAY													\
	port->four_way = 0;												\

#define PORT_PLAYER(player_)										\
	port->player = (player_) - 1;									\

#define PORT_COCKTAIL												\
	port->cocktail = 1;												\
	port->player = 1;												\

#define PORT_TOGGLE													\
	port->toggle = 1;												\

#define PORT_NAME(name_)											\
	port->name = (name_);											\

#define PORT_IMPULSE(duration_)										\
	port->impulse = (duration_);									\

#define PORT_REVERSE												\
	port->analog.reverse = 1;										\

#define PORT_RESET													\
	port->analog.reset = 1;											\

#define PORT_MINMAX(min_,max_)										\
	port->analog.min = (min_);										\
	port->analog.max = (max_);										\

#define PORT_SENSITIVITY(sensitivity_)								\
	port->analog.sensitivity = (sensitivity_);						\

#define PORT_KEYDELTA(delta_)										\
	port->analog.delta = port->analog.centerdelta = (delta_);		\

/* note that PORT_CENTERDELTA must appear after PORT_KEYDELTA */
#define PORT_CENTERDELTA(delta_)									\
	port->analog.centerdelta = (delta_);							\
	
#define PORT_UNUSED													\
	port->unused = 1;												\


/* dip switch definition */
#define PORT_DIPNAME(mask,default,name)								\
	PORT_BIT(mask, default, IPT_DIPSWITCH_NAME) PORT_NAME(name)		\

#define PORT_DIPSETTING(default,name)								\
	PORT_BIT(0, default, IPT_DIPSWITCH_SETTING) PORT_NAME(name)		\

/* conditionals for dip switch settings */
#define PORT_DIPCONDITION(port_,mask_,condition_,value_)			\
	port->dipsetting.portnum = (port_);								\
	port->dipsetting.mask = (mask_);								\
	port->dipsetting.condition = (condition_);						\
	port->dipsetting.value = (value_);								\

/* analog adjuster definition */
#define PORT_ADJUSTER(default,name)									\
	PORT_BIT(0x00ff, (default & 0xff) | (default << 8), IPT_ADJUSTER) PORT_NAME(name)



/*************************************
 *
 *	Helper macros
 *
 *************************************/

#define PORT_SERVICE(mask,default)	\
	PORT_BIT(    mask, mask & default, IPT_DIPSWITCH_NAME ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) PORT_TOGGLE	\
	PORT_DIPSETTING(    mask & default, DEF_STR( Off ) )	\
	PORT_DIPSETTING(    mask &~default, DEF_STR( On ) )

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BIT(    mask, mask & default, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode ))



/*************************************
 *
 *	Global variables
 *
 *************************************/

extern const char *inptport_default_strings[];

#define DEF_STR(str_num) (inptport_default_strings[STR_##str_num])



/*************************************
 *
 *	Function prototypes
 *
 *************************************/

int load_input_port_settings(void);
void save_input_port_settings(void);

struct InputPort *input_port_initialize(struct IptInitParams *params, UINT32 type);
struct InputPort *input_port_allocate(void construct_ipt(struct IptInitParams *params));

struct InputPortDefinition *get_input_port_list(void);
struct InputPortDefinition *get_input_port_list_backup(void);

int input_port_active(const struct InputPort *in);
int port_type_is_analog(int type);
int port_type_in_use(int type);
int port_type_to_group(int type, int player);
int port_tag_to_index(const char *tag);
const char *input_port_name(const struct InputPort *in);
input_seq_t *input_port_seq(struct InputPort *in, int seqtype);
input_seq_t *input_port_default_seq(int type, int player, int seqtype);
int input_port_condition(const struct InputPort *in);

const char *port_type_to_token(int type, int player);
int token_to_port_type(const char *string, int *player);

int input_port_type_pressed(int type, int player);
int input_ui_pressed(int code);
int input_ui_pressed_repeat(int code, int speed);

void inputport_vblank_start(void);	/* called by cpuintrf.c - not for external use */
void inputport_vblank_end(void);	/* called by cpuintrf.c - not for external use */

UINT32 readinputport(int port);
UINT32 readinputportbytag(const char *tag);

READ8_HANDLER( input_port_0_r );
READ8_HANDLER( input_port_1_r );
READ8_HANDLER( input_port_2_r );
READ8_HANDLER( input_port_3_r );
READ8_HANDLER( input_port_4_r );
READ8_HANDLER( input_port_5_r );
READ8_HANDLER( input_port_6_r );
READ8_HANDLER( input_port_7_r );
READ8_HANDLER( input_port_8_r );
READ8_HANDLER( input_port_9_r );
READ8_HANDLER( input_port_10_r );
READ8_HANDLER( input_port_11_r );
READ8_HANDLER( input_port_12_r );
READ8_HANDLER( input_port_13_r );
READ8_HANDLER( input_port_14_r );
READ8_HANDLER( input_port_15_r );
READ8_HANDLER( input_port_16_r );
READ8_HANDLER( input_port_17_r );
READ8_HANDLER( input_port_18_r );
READ8_HANDLER( input_port_19_r );
READ8_HANDLER( input_port_20_r );
READ8_HANDLER( input_port_21_r );
READ8_HANDLER( input_port_22_r );
READ8_HANDLER( input_port_23_r );
READ8_HANDLER( input_port_24_r );
READ8_HANDLER( input_port_25_r );
READ8_HANDLER( input_port_26_r );
READ8_HANDLER( input_port_27_r );
READ8_HANDLER( input_port_28_r );
READ8_HANDLER( input_port_29_r );

READ16_HANDLER( input_port_0_word_r );
READ16_HANDLER( input_port_1_word_r );
READ16_HANDLER( input_port_2_word_r );
READ16_HANDLER( input_port_3_word_r );
READ16_HANDLER( input_port_4_word_r );
READ16_HANDLER( input_port_5_word_r );
READ16_HANDLER( input_port_6_word_r );
READ16_HANDLER( input_port_7_word_r );
READ16_HANDLER( input_port_8_word_r );
READ16_HANDLER( input_port_9_word_r );
READ16_HANDLER( input_port_10_word_r );
READ16_HANDLER( input_port_11_word_r );
READ16_HANDLER( input_port_12_word_r );
READ16_HANDLER( input_port_13_word_r );
READ16_HANDLER( input_port_14_word_r );
READ16_HANDLER( input_port_15_word_r );
READ16_HANDLER( input_port_16_word_r );
READ16_HANDLER( input_port_17_word_r );
READ16_HANDLER( input_port_18_word_r );
READ16_HANDLER( input_port_19_word_r );
READ16_HANDLER( input_port_20_word_r );
READ16_HANDLER( input_port_21_word_r );
READ16_HANDLER( input_port_22_word_r );
READ16_HANDLER( input_port_23_word_r );
READ16_HANDLER( input_port_24_word_r );
READ16_HANDLER( input_port_25_word_r );
READ16_HANDLER( input_port_26_word_r );
READ16_HANDLER( input_port_27_word_r );
READ16_HANDLER( input_port_28_word_r );
READ16_HANDLER( input_port_29_word_r );

READ32_HANDLER( input_port_0_dword_r );
READ32_HANDLER( input_port_1_dword_r );
READ32_HANDLER( input_port_2_dword_r );
READ32_HANDLER( input_port_3_dword_r );
READ32_HANDLER( input_port_4_dword_r );
READ32_HANDLER( input_port_5_dword_r );
READ32_HANDLER( input_port_6_dword_r );
READ32_HANDLER( input_port_7_dword_r );
READ32_HANDLER( input_port_8_dword_r );
READ32_HANDLER( input_port_9_dword_r );
READ32_HANDLER( input_port_10_dword_r );
READ32_HANDLER( input_port_11_dword_r );
READ32_HANDLER( input_port_12_dword_r );
READ32_HANDLER( input_port_13_dword_r );
READ32_HANDLER( input_port_14_dword_r );
READ32_HANDLER( input_port_15_dword_r );
READ32_HANDLER( input_port_16_dword_r );
READ32_HANDLER( input_port_17_dword_r );
READ32_HANDLER( input_port_18_dword_r );
READ32_HANDLER( input_port_19_dword_r );
READ32_HANDLER( input_port_20_dword_r );
READ32_HANDLER( input_port_21_dword_r );
READ32_HANDLER( input_port_22_dword_r );
READ32_HANDLER( input_port_23_dword_r );
READ32_HANDLER( input_port_24_dword_r );
READ32_HANDLER( input_port_25_dword_r );
READ32_HANDLER( input_port_26_dword_r );
READ32_HANDLER( input_port_27_dword_r );
READ32_HANDLER( input_port_28_dword_r );
READ32_HANDLER( input_port_29_dword_r );


#ifdef __cplusplus
}
#endif


#endif
