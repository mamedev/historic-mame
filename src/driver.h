#ifndef DRIVER_H
#define DRIVER_H


#include "Z80.h"
#include "common.h"
#include "machine.h"


/***************************************************************************

Note that the memory hooks are not passed the actual memory address where
the operation takes place, but the offset from the beginning of the block
they are assigned to. This makes handling of mirror addresses easier, and
makes the handlers a bit more "object oriented". If you handler needs to
read/write the main memory area, provide a "base" pointer: it will be
initialized by the main engine to point to the beginning of the memory block
assigned to the handler.

***************************************************************************/
struct MemoryReadAddress
{
	int start,end;
	int (*handler)(int offset);	/* see special values below */
	unsigned char **base;
};

#define MRA_NOP 0	/* don't care, return 0 */
#define MRA_RAM ((int(*)())-1)	/* plain RAM location (return its contents) */
#define MRA_ROM ((int(*)())-2)	/* plain ROM location (return its contents) */


struct MemoryWriteAddress
{
	int start,end;
	void (*handler)(int offset,int data);	/* see special values below */
	unsigned char **base;
};


#define MWA_NOP 0	/* do nothing */
#define MWA_RAM ((void(*)())-1)	/* plain RAM location (store the value) */
#define MWA_ROM ((void(*)())-2)	/* plain ROM location (do nothing) */


struct GfxDecodeInfo
{
	int start;	/* beginning of data data to decode (offset in RAM[]) */
	struct GfxLayout *gfxlayout;
	int first_color_code;	/* first and last color codes used by this */
	int last_color_code;	/* gfx elements */
};



struct MachineDriver
{
	/* basic machine hardware */
	int cpu_clock;
	int frames_per_second;
	const struct MemoryReadAddress *memory_read;
	const struct MemoryWriteAddress *memory_write;
	const struct DSW *dswsettings;
	int defaultdsw[MAX_DIP_SWITCHES];	/* default dipswitch settings */

	int (*init_machine)(const char *gamename);
	int (*interrupt)(void);
	void (*out)(byte Port,byte Value);

	/* video hardware */
	int screen_width,screen_height;
	struct GfxDecodeInfo *gfxdecodeinfo;
	int total_colors;	/* palette is 3*total_colors bytes long */
	int color_codes;	/* colortable has color_codes tuples - the length */
						/* of each tuple depends on the graphic data, for example */
						/* 2-bitplane characters use 4 bytes in each tuple. */
		/* if they are available, provide a dump of the color proms (there is no */
		/* copyright infringement in that, since you can't copyright a color scheme) */
		/* and a function to convert them to a usable palette and colortable. */
		/* Otherwise, leave these two fields null and provide palette and colortable. */
	const unsigned char *color_prom;
	void (*vh_convert_color_prom)(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
	const unsigned char *palette;
	const unsigned char *colortable;

	int	numbers_start;	/* start of numbers and letters in the character roms */
	int letters_start;	/* (used by displaytext() ) */
	int white_text,yellow_text;	/* used by the dip switch menu */
	int paused_x,paused_y,paused_color;	/* used to print PAUSED on the screen */

	int (*vh_init)(const char *gamename);
	int (*vh_start)(void);
	void (*vh_stop)(void);
	void (*vh_screenrefresh)(struct osd_bitmap *bitmap);

	/* sound hardware */
	unsigned char *samples;
	int (*sh_init)(const char *gamename);
	int (*sh_start)(void);
	void (*sh_stop)(void);
	void (*sh_out)(byte Port,byte Value);
	int (*sh_in)(byte Port);
	void (*sh_update)(void);
};



struct GameDriver
{
	const char *name;
	const struct RomModule *rom;
	unsigned (*opcode_decode)(dword A);
	const struct MachineDriver *drv;
};



extern struct GameDriver drivers[];


#endif
