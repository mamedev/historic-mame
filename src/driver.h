#ifndef DRIVER_H
#define DRIVER_H


#include "common.h"
#include "mame.h"
#include "cpuintrf.h"
#include "usrintrf.h"


/***************************************************************************

Note that the memory hooks are not passed the actual memory address where
the operation takes place, but the offset from the beginning of the block
they are assigned to. This makes handling of mirror addresses easier, and
makes the handlers a bit more "object oriented". If you handler needs to
read/write the main memory area, provide a "base" pointer: it will be
initialized by the main engine to point to the beginning of the memory block
assigned to the handler. You may also provided a pointer to "size": it
will be set to the length of the memory area processed by the handler.

***************************************************************************/
struct MemoryReadAddress
{
	int start,end;
	int (*handler)(int offset);	/* see special values below */
	unsigned char **base;	/* optional (see explanation above) */
	int *size;	/* optional (see explanation above) */
};

#define MRA_NOP 0	/* don't care, return 0 */
#define MRA_RAM ((int(*)())-1)	/* plain RAM location (return its contents) */
#define MRA_ROM ((int(*)())-2)	/* plain ROM location (return its contents) */


struct MemoryWriteAddress
{
	int start,end;
	void (*handler)(int offset,int data);	/* see special values below */
	unsigned char **base;	/* optional (see explanation above) */
	int *size;	/* optional (see explanation above) */
};

#define MWA_NOP 0	/* do nothing */
#define MWA_RAM ((void(*)())-1)	/* plain RAM location (store the value) */
#define MWA_ROM ((void(*)())-2)	/* plain ROM location (do nothing) */
/* RAM[] and ROM[] are usually the same, but they aren't if the CPU opcodes are */
/* encrypted. In such a case, opcodes are fetched from ROM[], and arguments from */
/* RAM[]. If the program dynamically creates code in RAM and executes it, it */
/* won't work unless writes to RAM affects both RAM[] and ROM[]. */
#define MWA_RAMROM ((void(*)())-3)	/* write to both the RAM[] and ROM[] array. */


/***************************************************************************

IN and OUT ports are handled like memory accesses, the hook template is the
same so you can interchange them. Of course there is no 'base' pointer for
IO ports.

***************************************************************************/
struct IOReadPort
{
	int start,end;
	int (*handler)(int offset);	/* see special values below */
};

#define IORP_NOP 0	/* don't care, return 0 */


struct IOWritePort
{
	int start,end;
	void (*handler)(int offset,int data);	/* see special values below */
};

#define IOWP_NOP 0	/* do nothing */



/***************************************************************************

Don't confuse this with the I/O ports above. This is used to handle game
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



/*
 *  The idea behing the trak-ball support is to have the system dependent
 *  handler return a positive or negative number representing the distance
 *  from the mouse region origin.  Then depending on what the emulation needs,
 *  it will do a conversion on the distance from the origin into the desired
 *  format.  Why was it implemented this way?  Take Reactor and Crystal Castles
 *  as an example.  Reactor's inputs range from -127 to 127 where < 0 is
 *  left/down and >0 is right/up.  However, Crystal Castles just looks at the
 *  difference between successive unsigned char values.  So their inputs are
 *  quite different.
 */

struct TrakPort {
  int axis;                     /* The axis of the trak-ball */
  int centered;                 /* Does it return to the "zero" position after a read? */
  float scale;                  /* How much do we scale the value by? */
  int (*conversion)(int data);  /* Function to do the conversion to what the game needs */
};


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
#define CPU_M6809  4

/* set this if the CPU is used as a slave for audio. It will not be emulated if */
/* play_sound == 0, therefore speeding up a lot the emulation. */
#define CPU_AUDIO_CPU 0x8000

#define CPU_FLAGS_MASK 0xff00


#define MAX_CPU 4


struct MachineDriver
{
	/* basic machine hardware */
	struct MachineCPU cpu[MAX_CPU];
	int frames_per_second;
	int (*init_machine)(const char *gamename);

	/* video hardware */
	int screen_width,screen_height;
	struct rectangle visible_area;
	struct GfxDecodeInfo *gfxdecodeinfo;
	int total_colors;	/* palette is 3*total_colors bytes long */
	int color_table_len;	/* length in bytes of the color lookup table */
	void (*vh_convert_color_prom)(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

	int (*vh_init)(const char *gamename);
	int (*vh_start)(void);
	void (*vh_stop)(void);
	void (*vh_update)(struct osd_bitmap *bitmap);

	/* sound hardware */
	unsigned char *samples;
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

	struct InputPort *input_ports;
        struct TrakPort *trak_ports;
	const struct DSW *dswsettings;
	const struct KEYSet *keysettings;

		/* if they are available, provide a dump of the color proms (there is no */
		/* copyright infringement in that, since you can't copyright a color scheme) */
		/* and a function to convert them to a usable palette and colortable (the */
		/* function pointer is in the MachineDriver, not here) */
		/* Otherwise, leave this field null and provide palette and colortable. */
	const unsigned char *color_prom;
	const unsigned char *palette;
	const unsigned char *colortable;

	int paused_x,paused_y;	/* used to print PAUSED on the screen */

	int (*hiscore_load)(const char *name);	/* will be called every vblank until it */
						/* returns nonzero */
	void (*hiscore_save)(const char *name);	/* will not be loaded if hiscore_load() hasn't yet */
						/* returned nonzero, to avoid saving an invalid table */
};



extern const struct GameDriver *drivers[];

#ifdef WIN32
#pragma warning(disable:4113)
#define PI 3.1415926535
#define inline __inline
#endif

#endif
