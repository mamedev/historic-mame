#ifndef DRIVER_H
#define DRIVER_H


#include "common.h"
#include "mame.h"
#include "cpuintrf.h"
#include "memory.h"
#include "inptport.h"
#include "usrintrf.h"
#include "cheat.h"



/***************************************************************************

Don't confuse this with the I/O ports in memory.h. This is used to handle game
inputs (joystick, coin slots, etc). Typically, you will read them using
input_port_[n]_r(), which you will associate to the appropriate memory
address or I/O port.

***************************************************************************/
struct InputPort
{
	int default_value;	/* default value for the input port */
	int keyboard[8];	/* keys affecting the 8 bits of the input port (0 means none) */
	int joystick[8];	/* same for joystick */
};

/* Many games poll an input bit to check for vertical blanks instead of using */
/* interrupts. This special value to put in the keyboard[] field allows you to */
/* handle that. If you set one of the input bits to this, the bit will be */
/* inverted while a vertical blank is happening. */
#define IPB_VBLANK	(-1)

struct NewInputPort
{
	unsigned char mask;	/* bits affected */
	unsigned char default_value;	/* default value for the bits affected */
							/* you can also use one of the IP_ACTIVE defines below */
	int type;	/* see defines below */
	const char *name;	/* name to display */
	int keyboard;	/* key affecting the input bits */
	int joystick;	/* joystick command affecting the input bits */
	int arg;	/* extra argument needed in some cases */
};


#define IP_ACTIVE_HIGH 0x00
#define IP_ACTIVE_LOW 0xff

enum { IPT_END=1,IPT_PORT,
	/* use IPT_JOYSTICK for panels where the player has one single joystick */
	IPT_JOYSTICK_UP, IPT_JOYSTICK_DOWN, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT,
	/* use IPT_JOYSTICKLEFT and IPT_JOYSTICKRIGHT for dual joystick panels */
	IPT_JOYSTICKRIGHT_UP, IPT_JOYSTICKRIGHT_DOWN, IPT_JOYSTICKRIGHT_LEFT, IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP, IPT_JOYSTICKLEFT_DOWN, IPT_JOYSTICKLEFT_LEFT, IPT_JOYSTICKLEFT_RIGHT,
	IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_BUTTON4,	/* action buttons */
	IPT_BUTTON5, IPT_BUTTON6, IPT_BUTTON7, IPT_BUTTON8,

	/* analog inputs */
	/* the "arg" field contains the default sensitivity expressed as a percentage */
	/* (100 = default, 50 = half, 200 = twice) */
	IPT_ANALOG_START,
	IPT_PADDLE, IPT_DIAL, IPT_TRACKBALL_X, IPT_TRACKBALL_Y, IPT_AD_STICK_X, IPT_AD_STICK_Y,
	IPT_ANALOG_END,

	IPT_COIN1, IPT_COIN2, IPT_COIN3, IPT_COIN4,	/* coin slots */
	IPT_START1, IPT_START2, IPT_START3, IPT_START4,	/* start buttons */
	IPT_SERVICE, IPT_TILT,
	IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING,
	IPT_VBLANK,
	IPT_UNKNOWN
};

#define IPT_UNUSED     IPF_UNUSED

#define IPF_MASK       0xffff0000
#define IPF_UNUSED     0x80000000	/* The bit is not used by this game, but is used */
									/* by other games running on the same hardware. */
									/* This is different from IPT_UNUSED, which marks */
									/* bits not connected to anything. */
#define IPF_COCKTAIL   IPF_PLAYER2	/* the bit is used in cocktail mode only */

#define IPF_CHEAT      0x40000000	/* Indicates that the input bit is a "cheat" key */
									/* (providing invulnerabilty, level advance, and */
									/* so on). MAME will not recognize it when the */
									/* -nocheat command line option is specified. */

#define IPF_PLAYERMASK 0x00030000	/* use IPF_PLAYERn if more than one person can */
#define IPF_PLAYER1    0         	/* play at the same time. The IPT_ should be the same */
#define IPF_PLAYER2    0x00010000	/* for all players (e.g. IPT_BUTTON1 | IPF_PLAYER2) */
#define IPF_PLAYER3    0x00020000	/* IPF_PLAYER1 is the default and can be left out to */
#define IPF_PLAYER4    0x00030000	/* increase readability. */

#define IPF_8WAY       0         	/* Joystick modes of operation. 8WAY is the default, */
#define IPF_4WAY       0x00080000	/* it prevents left/right or up/down to be pressed at */
#define IPF_2WAY       0         	/* the same time. 4WAY prevents diagonal directions. */
									/* 2WAY should be used for joysticks wich move only */
                                 	/* on one axis (e.g. Battle Zone) */

#define IPF_IMPULSE    0x00100000	/* When this is set, when the key corrisponding to */
									/* the input bit is pressed it will be reported as */
									/* pressed for a certain number of video frames and */
									/* then released, regardless of the real status of */
									/* the key. This is useful e.g. for some coin inputs. */
									/* The number of frames the signal should stay active */
									/* is specified in the "arg" field. */
#define IPF_TOGGLE     0x00200000	/* When this is set, the key acts as a toggle - press */
									/* it once and it goes on, press it again and it goes off. */
									/* useful e.g. for sone Test Mode dip switches. */
#define IPF_REVERSE    0x00400000	/* By default, analog inputs like IPT_TRACKBALL increase */
									/* when going right/up. This flag inverts them. */

#define IPF_CENTER     0x00800000	/* always preload in->default, autocentering the STICK/TRACKBALL */

/* LBO - These 4 byte values are packed into the arg field and are typically used with analog ports */
/* Since the sensivity is only one byte, we want to have 100% = 2^16, */
/* 50% = 2^8, 200% = 2^32 and so on. BW	*/
#define IPF_SENSITIVITY(percent)	(((percent*16)/100)&0xff)
#define IPF_CLIP(clip)			((clip&0xff) << 8  )
#define IPF_MIN(min)			((min&0xff)  << 16 )
#define IPF_MAX(max)			((max&0xff)  << 24 )

/* LBO - these fields are packed into in->keyboard & in->joystick for analog controls */
#define IPF_DEC(key)			((key&0xff)       )
#define IPF_INC(key)			((key&0xff) << 8  )
#define IPF_DELTA(val)			((val&0xff) << 16 )

#define IP_NAME_DEFAULT ((const char *)-1)

#define IP_KEY_DEFAULT -1
#define IP_KEY_NONE -2
#define IP_KEY_PREVIOUS -3	/* use the same key as the previous input bit */

#define IP_JOY_DEFAULT -1
#define IP_JOY_NONE -2
#define IP_JOY_PREVIOUS -3	/* use the same joy as the previous input bit */

/* start of table */
#define INPUT_PORTS_START(name) static struct NewInputPort name[] = {
/* end of table */
#define INPUT_PORTS_END { 0, 0, IPT_END, 0, 0 } };
/* start of a new input port */
#define PORT_START { 0, 0, IPT_PORT, 0, 0, 0, 0 },
/* input bit definition */
#define PORT_BIT(mask,default,type) { mask, default, type, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 },
/* input bit definition with extended fields */
#define PORT_BITX(mask,default,type,name,key,joy,arg) { mask, default, type, name, key, joy, arg },
/* analog input */
#define PORT_ANALOG(mask,default,type,sensitivity,clip,min,max) \
	{ mask, default, type, IP_NAME_DEFAULT, \
	IP_KEY_DEFAULT, IP_JOY_DEFAULT, \
	IPF_SENSITIVITY(sensitivity) | IPF_CLIP(clip) | IPF_MIN(min) | IPF_MAX(max) },
/* analog input with extended fields for defining default keys & sensitivities */
#define PORT_ANALOGX(mask,default,type,sensitivity,clip,min,max,keydec,keyinc,joydec,joyinc,delta) \
	{ mask, default, type, IP_NAME_DEFAULT, \
	IPF_DEC(keydec) | IPF_INC(keyinc) | IPF_DELTA(delta), IPF_DEC(joydec) | IPF_INC(joyinc) | IPF_DELTA(delta), \
	IPF_SENSITIVITY(sensitivity) | IPF_CLIP(clip) | IPF_MIN(min) | IPF_MAX(max) },

/* dip switch definition */
#define PORT_DIPNAME(mask,default,name,key) { mask, default, IPT_DIPSWITCH_NAME, name, key, IP_JOY_NONE, 0 },
#define PORT_DIPSETTING(default,name) { 0, default, IPT_DIPSWITCH_SETTING, name, IP_KEY_NONE, IP_JOY_NONE, 0 },



/* Key setting definition */
struct KEYSet
{
	int num;	      /* input port affected */
			      /* -1 terminates the array */
	int mask;	      /* bit affected */
	const char *name;     /* name of the setting */
};


/* dipswitch setting definition */
struct DSW
{
	int num;	/* input port affected */
				/* -1 terminates the array */
	int mask;	/* bits affected */
	const char *name;	/* name of the setting */
	const char *values[17];/* null terminated array of names for the values */
									/* the setting can have */
	int reverse; 	/* set to 1 to display values in reverse order */
};



struct GfxDecodeInfo
{
	int memory_region;	/* memory region where the data resides (usually 1) */
						/* -1 marks the end of the array */
	int start;	/* beginning of data to decode */
	struct GfxLayout *gfxlayout;
	int color_codes_start;	/* offset in the color lookup table where color codes start */
	int total_color_codes;	/* total number of color codes */
};



struct MachineCPU
{
	int cpu_type;	/* see #defines below. */
	int cpu_clock;	/* in Hertz */
	int memory_region;	/* number of the memory region (allocated by loadroms()) where */
						/* this CPU resides */
	const struct MemoryReadAddress *memory_read;
	const struct MemoryWriteAddress *memory_write;
	const struct IOReadPort *port_read;
	const struct IOWritePort *port_write;
	int (*interrupt)(void);
	int interrupts_per_frame;	/* usually 1 */
};

#define CPU_Z80    1
#define CPU_M6502  2
#define CPU_I86    3
#define CPU_I8039  4
#define CPU_M6803  5
#define CPU_M6802  CPU_M6803
#define CPU_M6808  CPU_M6803
#define CPU_M6809  6
#define CPU_M68000 7

/* set this if the CPU is used as a slave for audio. It will not be emulated if */
/* sound is disabled, therefore speeding up a lot the emulation. */
#define CPU_AUDIO_CPU 0x8000

#define CPU_FLAGS_MASK 0xff00


#define MAX_CPU 4	/* MAX_CPU is the maximum number of CPUs which cpuintrf.c */
					/* can run at the same time. Currently, 4 is enough. */


/* ASG 081897 -- added these flags for the video hardware */

/* bit 0 of the video attributes indicates raster or vector video hardware */
#define	VIDEO_TYPE_RASTER			0x0000
#define	VIDEO_TYPE_VECTOR			0x0001

/* bit 1 of the video attributes indicates whether or not dirty rectangles will work */
#define	VIDEO_SUPPORTS_DIRTY		0x0002

/* bit 2 of the video attributes indicates whether or not the driver modifies the palette */
#define	VIDEO_MODIFIES_PALETTE	0x0004



struct MachineDriver
{
	/* basic machine hardware */
	struct MachineCPU cpu[MAX_CPU];
	int frames_per_second;
	int cpu_slices_per_frame;	/* for multicpu games. 1 is the minimum, meaning */
								/* that each CPU runs for the whole video frame */
								/* before giving control to the others. The higher */
								/* this setting, the more closely CPUs are interleaved */
								/* and therefore the more accurate the emulation is. */
								/* However, an higher setting also means slower */
								/* performance. */
	void (*init_machine)(void);

	/* video hardware */
	int screen_width,screen_height;
	struct rectangle visible_area;
	struct GfxDecodeInfo *gfxdecodeinfo;
	int total_colors;	/* palette is 3*total_colors bytes long */
	int color_table_len;	/* length in bytes of the color lookup table */
	void (*vh_convert_color_prom)(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

	int video_attributes;	/* ASG 081897 */
	int (*vh_init)(const char *gamename);
	int (*vh_start)(void);
	void (*vh_stop)(void);
	void (*vh_update)(struct osd_bitmap *bitmap);

	/* sound hardware */
	int (*sh_init)(const char *gamename);
	int (*sh_start)(void);
	void (*sh_stop)(void);
	void (*sh_update)(void);
};



struct GameDriver
{
	const char *description;
	const char *name;
	const char *credits;
	const struct MachineDriver *drv;

	const struct RomModule *rom;
	void (*rom_decode)(void);		/* used to decrypt the ROMs after loading them */
	void (*opcode_decode)(void);	/* used to decrypt the CPU opcodes in the ROMs, */
									/* if the encryption is different from the above. */
	const char **samplenames;	/* optional array of names of samples to load. */
						/* drivers can retrieve them in Machine->samples */
	const unsigned char *sound_prom;

struct InputPort *input_ports;	/* obsolete */
	struct NewInputPort *new_input_ports;
int trak;	/* filler for obsolete struct - will be removed */
const struct DSW *dswsettings;	/* obsolete */
const struct KEYSet *keysettings;	/* obsolete */

		/* if they are available, provide a dump of the color proms (there is no */
		/* copyright infringement in that, since you can't copyright a color scheme) */
		/* and a function to convert them to a usable palette and colortable (the */
		/* function pointer is in the MachineDriver, not here) */
		/* Otherwise, leave this field null and provide palette and colortable. */
	const unsigned char *color_prom;
	const unsigned char *palette;
	const unsigned char *colortable;
	int orientation;	/* orientation of the monitor; see defines below */

	int (*hiscore_load)(void);	/* will be called every vblank until it */
						/* returns nonzero */
	void (*hiscore_save)(void);	/* will not be called if hiscore_load() hasn't yet */
						/* returned nonzero, to avoid saving an invalid table */
};


#define	ORIENTATION_DEFAULT		0x00
#define	ORIENTATION_FLIP_X		0x01	/* mirror everything in the X direction */
#define	ORIENTATION_FLIP_Y		0x02	/* mirror everything in the Y direction */
#define ORIENTATION_SWAP_XY		0x04	/* mirror along the top-left/bottom-rigth diagonal */
#define	ORIENTATION_ROTATE_90	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_X)	/* rotate clockwise 90 degrees */
#define	ORIENTATION_ROTATE_180	(ORIENTATION_FLIP_X|ORIENTATION_FLIP_Y)	/* rotate 180 degrees */
#define	ORIENTATION_ROTATE_270	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_Y)	/* rotate counter-clockwise 90 degrees */
/* IMPORTANT: to perform more than one transformation, DO NOT USE |, use ^ instead. */
/* For example, to rotate 90 degrees counterclockwise and flip horizontally, use: */
/* ORIENTATION_ROTATE_270 ^ ORIENTATION_FLIP_X */
/* FLIP is performed *after* SWAP_XY. */



extern const struct GameDriver *drivers[];

#endif
