#ifndef INPTPORT_H
#define INPTPORT_H

#include "memory.h"
#include "input.h"

#ifdef MESS
#include "unicode.h"
#endif

/* input ports handling */

/* Don't confuse this with the I/O ports in memory.h. This is used to handle game */
/* inputs (joystick, coin slots, etc). Typically, you will read them using */
/* input_port_[n]_r(), which you will associate to the appropriate memory */
/* address or I/O port. */

/***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

struct InputPort
{
	UINT16 mask;			/* bits affected */
	UINT16 default_value;	/* default value for the bits affected */
							/* you can also use one of the IP_ACTIVE defines below */
	UINT8 type;				/* see defines below */
	UINT8 unused;			/* The bit is not used by this game, but is used */
							/* by other games running on the same hardware. */
							/* This is different from IPT_UNUSED, which marks */
							/* bits not connected to anything. */
	UINT8 cocktail;			/* the bit is used in cocktail mode only */
	UINT8 cheat;			/* Indicates that the input bit is a "cheat" key */
							/* (providing invulnerabilty, level advance, and */
							/* so on). MAME will not recognize it when the */
							/* -nocheat command line option is specified. */
	UINT8 player;			/* the player associated with this port; note that
							 * player 1 is '0' */
	UINT8 toggle;			/* When this is set, the key acts as a toggle - press */
							/* it once and it goes on, press it again and it goes off. */
							/* useful e.g. for sone Test Mode dip switches. */
	UINT8 reset_cpu;		/* when the key is pressed, reset the first CPU */
	UINT8 impulse;			/* When this is set, when the key corrisponding to */
							/* the input bit is pressed it will be reported as */
							/* pressed for a certain number of video frames and */
							/* then released, regardless of the real status of */
							/* the key. This is useful e.g. for some coin inputs. */
							/* The number of frames the signal should stay active */
							/* is specified in the "arg" field. */
	UINT8 four_way;			/* Joystick modes of operation. 8WAY is the default, */
							/* it prevents left/right or up/down to be pressed at */
							/* the same time. 4WAY prevents diagonal directions. */
							/* 2WAY should be used for joysticks wich move only */
							/* on one axis (e.g. Battle Zone) */

	const char *name;		/* name to display */
	InputSeq seq[2];        /* input sequence affecting the input bits */
#ifdef MESS
	UINT16 category;
#endif /* MESS */

	union
	{
		struct
		{
			/* valid if type is between IPT_ANALOG_BEGIN and IPT_ANALOG_END */
			UINT16 min;
			UINT16 max;
			UINT8 sensitivity;
			UINT8 delta;

			UINT8 reverse;			/* By default, analog inputs like IPT_TRACKBALL increase */
									/* when going right/up. This flag inverts them. */
			UINT8 center;			/* always preload in->default, autocentering the STICK/TRACKBALL */
			UINT8 custom_update;	/* normally, analog ports are updated when they are accessed. */
									/* When this flag is set, they are never updated automatically, */
									/* it is the responsibility of the driver to call */
									/* update_analog_port(int port). */
		} analog;
		struct
		{
			const char *tag;		/* used to tag PORT_START declarations */
		} start;
#ifdef MESS
		struct
		{
			unicode_char_t chars[3];
		} keyboard;
#endif /* MESS */
	} u;
};


#define IP_ACTIVE_HIGH 0x0000
#define IP_ACTIVE_LOW 0xffff

enum { IPT_END=1,IPT_PORT,
	/* use IPT_JOYSTICK for panels where the player has one single joystick */
	IPT_JOYSTICK_UP, IPT_JOYSTICK_DOWN, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT,
	/* use IPT_JOYSTICKLEFT and IPT_JOYSTICKRIGHT for dual joystick panels */
	IPT_JOYSTICKRIGHT_UP, IPT_JOYSTICKRIGHT_DOWN, IPT_JOYSTICKRIGHT_LEFT, IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP, IPT_JOYSTICKLEFT_DOWN, IPT_JOYSTICKLEFT_LEFT, IPT_JOYSTICKLEFT_RIGHT,
	IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_BUTTON4,	/* action buttons */
	IPT_BUTTON5, IPT_BUTTON6, IPT_BUTTON7, IPT_BUTTON8, IPT_BUTTON9, IPT_BUTTON10,

	/* analog inputs */
	/* the "arg" field contains the default sensitivity expressed as a percentage */
	/* (100 = default, 50 = half, 200 = twice) */
	IPT_ANALOG_START,
	IPT_PADDLE, IPT_PADDLE_V,
	IPT_DIAL, IPT_DIAL_V,
	IPT_TRACKBALL_X, IPT_TRACKBALL_Y,
	IPT_AD_STICK_X, IPT_AD_STICK_Y, IPT_AD_STICK_Z,
	IPT_LIGHTGUN_X, IPT_LIGHTGUN_Y,
	IPT_PEDAL, IPT_PEDAL2,
#ifdef MESS
	IPT_MOUSE_X, IPT_MOUSE_Y,
#endif /* MESS */
	IPT_ANALOG_END,

	IPT_START1, IPT_START2, IPT_START3, IPT_START4,	/* start buttons */
	IPT_COIN1, IPT_COIN2, IPT_COIN3, IPT_COIN4,	/* coin slots */
	IPT_SERVICE1, IPT_SERVICE2, IPT_SERVICE3, IPT_SERVICE4,	/* service coin */
	IPT_SERVICE, IPT_TILT,
	IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING,
#ifdef MESS
	IPT_KEYBOARD,
	IPT_CONFIG_NAME, IPT_CONFIG_SETTING,
	IPT_START, IPT_SELECT,
	IPT_CATEGORY_NAME, IPT_CATEGORY_SETTING,
#endif /* MESS */
/* Many games poll an input bit to check for vertical blanks instead of using */
/* interrupts. This special value allows you to handle that. If you set one of the */
/* input bits to this, the bit will be inverted while a vertical blank is happening. */
	IPT_VBLANK,
	IPT_UNKNOWN,
	IPT_OSD_RESERVED,
	IPT_OSD_1,
	IPT_OSD_2,
	IPT_OSD_3,
	IPT_OSD_4,

	IPT_EXTENSION,

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
	IPT_UI_PAN_UP, IPT_UI_PAN_DOWN, IPT_UI_PAN_LEFT, IPT_UI_PAN_RIGHT,
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

	/* 8 player support */
	IPT_START5, IPT_START6, IPT_START7, IPT_START8,
	IPT_COIN5, IPT_COIN6, IPT_COIN7, IPT_COIN8,

	/* Analog adjuster support */
	IPT_ADJUSTER,
	__ipt_max
};

#define IPT_UNUSED     IPF_UNUSED
#define IPT_SPECIAL    IPT_UNUSED	/* special meaning handled by custom functions */


/* accursed legacy macros */
#define IPF_MASK       0xffffff00
#define IPF_UNUSED     0x80000000
#define IPF_COCKTAIL   IPF_PLAYER2
#define IPF_CHEAT      0x40000000
#define IPF_PLAYERMASK 0x00070000
#define IPF_PLAYER1    0         
#define IPF_PLAYER2    0x00010000
#define IPF_PLAYER3    0x00020000
#define IPF_PLAYER4    0x00030000
#define IPF_PLAYER5    0x00040000
#define IPF_PLAYER6    0x00050000
#define IPF_PLAYER7    0x00060000
#define IPF_PLAYER8    0x00070000
#define IPF_8WAY       0         
#define IPF_4WAY       0x00080000
#define IPF_2WAY       0         
#define IPF_IMPULSE    0x00100000
#define IPF_TOGGLE     0x00200000
#define IPF_REVERSE    0x00400000
#define IPF_CENTER     0x00800000
#define IPF_CUSTOM_UPDATE 0x01000000
#define IPF_RESETCPU   0x02000000
#define IPF_SENSITIVITY(percent)	((percent & 0xff) << 8)
#define IPF_DELTA(val)				((val & 0xff) << 16)

#define IP_NAME_DEFAULT ((const char *)-1)

/* Wrapper for compatibility */
#define IP_KEY_DEFAULT CODE_DEFAULT
#define IP_JOY_DEFAULT CODE_DEFAULT
#define IP_KEY_PREVIOUS CODE_PREVIOUS
#define IP_JOY_PREVIOUS CODE_PREVIOUS
#define IP_KEY_NONE CODE_NONE
#define IP_JOY_NONE CODE_NONE

/* start of table */
#define INPUT_PORTS_START(name) \
 	void construct_ipt_##name(void *param)				\
	{													\
 		struct InputPort *port;							\
		int seq = 0;									\
		int seq_code = 0;								\
 		(void) port; (void) seq; (void) seq_code;		\

/* end of table */
#define INPUT_PORTS_END \
 		port = input_port_initialize(param, IPT_END);	\
	}													\

/* aliasing */
#define INPUT_PORTS_ALIAS(name, base)					\
 	void construct_ipt_##name(void *param)				\
	{													\
 		construct_ipt_##base(param);					\
	}													\

#define INPUT_PORTS_EXTERN(name)						\
	extern void construct_ipt_##name(void *param)		\

/* including */
#define PORT_INCLUDE(name)								\
 		construct_ipt_##name(param);					\

/* start of a new input port */
#define PORT_START \
		port = input_port_initialize(param, IPT_PORT);	\

/* start of a new input port */
#define PORT_START_TAG(tag_) \
		port = input_port_initialize(param, IPT_PORT);	\
		port->u.start.tag = (tag_);						\

/* input bit definition */
#define PORT_BIT_NAME(mask_,default_,type_,name_) 		\
		port = input_port_initialize(param, (type_));	\
		port->mask = (mask_);							\
		port->default_value = (default_);				\
		port->name = (name_);							\
		seq = 0;										\
		seq_code = 0;									\

#define PORT_BIT(mask,default,type) \
	PORT_BIT_NAME(mask, default, type, IP_NAME_DEFAULT)

/* impulse input bit definition */
#define PORT_BIT_IMPULSE_NAME(mask,default,type,duration,name) \
	PORT_BIT_NAME(mask, default, type, name)	port->impulse = (duration);
#define PORT_BIT_IMPULSE(mask,default,type,duration) \
	PORT_BIT_IMPULSE_NAME(mask, default, type, duration, IP_NAME_DEFAULT)

/* new technique to append a code */
#define PORT_CODE(code)												\
		if ((code) < __code_max)									\
		{															\
			if (seq_code > 0)										\
				port->seq[seq][seq_code++] = CODE_OR;				\
			port->seq[seq][seq_code++] = (code);					\
		}															\

#define PORT_NEXTSEQ												\
		seq++;														\
		seq_code = 0;												\

#define PORT_4WAY													\
		port->four_way = 1;											\

#define PORT_PLAYER(player_)										\
		port->player = (player_) - 1;								\

#define PORT_COCKTAIL												\
		port->cocktail = 1;											\
		port->player = 1;											\

#define PORT_TOGGLE													\
		port->toggle = 1;											\

/* key/joy code specification */
#define PORT_CODELEGACY(key,joy) \
	{																		\
		InputCode or3;														\
		PORT_CODE(key);														\
		PORT_CODE(joy);														\
		switch(joy)															\
		{																	\
			case JOYCODE_1_BUTTON1:	or3 = JOYCODE_MOUSE_1_BUTTON1;	break;	\
			case JOYCODE_1_BUTTON2:	or3 = JOYCODE_MOUSE_1_BUTTON2;	break;	\
			case JOYCODE_1_BUTTON3:	or3 = JOYCODE_MOUSE_1_BUTTON3;	break;	\
			case JOYCODE_2_BUTTON1:	or3 = JOYCODE_MOUSE_2_BUTTON1;	break;	\
			case JOYCODE_2_BUTTON2:	or3 = JOYCODE_MOUSE_2_BUTTON2;	break;	\
			case JOYCODE_2_BUTTON3:	or3 = JOYCODE_MOUSE_2_BUTTON3;	break;	\
			case JOYCODE_3_BUTTON1:	or3 = JOYCODE_MOUSE_3_BUTTON1;	break;	\
			case JOYCODE_3_BUTTON2:	or3 = JOYCODE_MOUSE_3_BUTTON2;	break;	\
			case JOYCODE_3_BUTTON3:	or3 = JOYCODE_MOUSE_3_BUTTON3;	break;	\
			case JOYCODE_4_BUTTON1:	or3 = JOYCODE_MOUSE_4_BUTTON1;	break;	\
			case JOYCODE_4_BUTTON2:	or3 = JOYCODE_MOUSE_4_BUTTON2;	break;	\
			case JOYCODE_4_BUTTON3:	or3 = JOYCODE_MOUSE_4_BUTTON3;	break;	\
			default:				or3 = CODE_NONE;				break;	\
		}																	\
		PORT_CODE(or3);														\
		PORT_NEXTSEQ														\
	}																		\
			

/* input bit definition with extended fields */
#define PORT_BITX(mask,default,type,name,key,joy) \
	PORT_BIT_NAME(mask, default, type, name) \
	PORT_CODELEGACY(key,joy)

/* analog input */
#define PORT_ANALOG(mask,default,type,sensitivity_,delta_,min_,max_) \
	PORT_BIT(mask, default, type) \
	port->u.analog.min = (min_);								\
	port->u.analog.max = (max_);								\
	port->u.analog.sensitivity = (sensitivity_);				\
	port->u.analog.delta = (delta_);							\

#define PORT_ANALOGX(mask,default,type,sensitivity_,delta_,min_,max_,keydec,keyinc,joydec,joyinc) \
	PORT_BIT(mask, default, type) \
	port->u.analog.min = (min_);								\
	port->u.analog.max = (max_);								\
	port->u.analog.sensitivity = (sensitivity_);				\
	port->u.analog.delta = (delta_);							\
	PORT_CODELEGACY(keydec,joydec)									\
	PORT_CODELEGACY(keyinc,joyinc)

/* dip switch definition */
#define PORT_DIPNAME(mask,default,name) \
	PORT_BIT_NAME(mask, default, IPT_DIPSWITCH_NAME, name)

#define PORT_DIPSETTING(default,name) \
	PORT_BIT_NAME(0, default, IPT_DIPSWITCH_SETTING, name)

/* analog adjuster definition */
#define PORT_ADJUSTER(default,name) \
	PORT_BIT_NAME(0x00ff, (default & 0xff) | (default << 8), IPT_ADJUSTER, name)


#define PORT_SERVICE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	\
	PORT_DIPSETTING(    mask & default, DEF_STR( Off ) )	\
	PORT_DIPSETTING(    mask &~default, DEF_STR( On ) )

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

#define MAX_DEFSTR_LEN 20
extern const char ipdn_defaultstrings[][MAX_DEFSTR_LEN];

/* this must match the ipdn_defaultstrings list in inptport.c */
enum {
	STR_Off,
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
	/*STR_Pause,
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
	STR_Mono,*/
	STR_Unused,
	STR_Unknown,
	STR_TOTAL
};

enum { IKT_STD, IKT_IPT, IKT_IPT_EXT, IKT_OSD_KEY, IKT_OSD_JOY };

#define DEF_STR(str_num) (ipdn_defaultstrings[STR_##str_num])

#define MAX_INPUT_PORTS 30


int load_input_port_settings(void);
void save_input_port_settings(void);

const char *input_port_name(const struct InputPort *in);
int input_port_active(const struct InputPort *in);
InputSeq* input_port_type_seq(int type);
InputSeq* input_port_seq(struct InputPort *in, int seq);
int input_port_seq_count(const struct InputPort *in);

struct InputPort *input_port_initialize(void *param, UINT32 type);
struct InputPort *input_port_allocate(void construct_ipt(void *param));

void init_analog_seq(void);

void update_analog_port(int port);
void update_input_ports(void);	/* called by cpuintrf.c - not for external use */
void inputport_vblank_end(void);	/* called by cpuintrf.c - not for external use */

int readinputport(int port);
int readinputportbytag(const char *tag);
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

struct ipd
{
	UINT32 type;
	const char *name;
	InputSeq seq;
};

struct ik
{
	const char *name;
	UINT32 type;
	UINT32 val;
};
extern struct ik input_keywords[];
extern struct ik *osd_input_keywords;
extern int num_ik;

void seq_set_string(InputSeq* a, const char *buf);

#ifdef __cplusplus
}
#endif


#endif
