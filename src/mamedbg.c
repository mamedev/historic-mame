/****************************************************************************
 *	The new MAME debugger
 *	Written by:   Juergen Buchmueller (so far - help welcome! :)
 *
 *	Based on code found in the preivous version of the MAME debugger
 *	written by:   Martin Scragg, John Butler, Mirko Buffoni
 *				  Chris Moore, Aaron Giles, Ernesto Corvi
 *
 *	Online help is available by pressing F1 (context sensitive!)
 *
 *	TODO:
 *	- Verify correct endianess handling, ie. look at the UINT16/UINT32 modes
 *	  of the memory and dasm windows (selected with M)
 *	- Add missing functionality ! There's a lot left to do...
 *	- Squash thousands of bugs :-/
 *	- Test & improve find_rom_name()
 *	- Add names for more generic handlers to name_rdmem() and name_wrmem()
 *
 *  LATER:
 *	- Add stack view using cpu_get_reg(REG_SP_CONTENTS+offset)
 *	- Add more display modes for the memory windows (ASCII, binary?, decimal?)
 *	- Make the windows resizeable somehow (using win_set_w and win_set_h)
 *	- Anything you like - just contact me to avoid doubled effort.
 *
 ****************************************************************************/
#include "mamedbg.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "driver.h"
#include "window.h"
#include "vidhrdw/generic.h"

#ifndef INVALID
#define INVALID -1
#endif

/****************************************************************************
 * Externals (define in the header files)
 ****************************************************************************/
/* Long(er) function names, short macro names... */
#define ABITS	cpu_address_bits()
#define AMASK   cpu_address_mask()
#define ALIGN   cpu_align_unit()
#define INSTL   cpu_max_inst_len()
#define ENDIAN	cpu_endianess()

#define RDMEM(a)	(*cpuintf[cputype].memory_read)(a)
#define WRMEM(a,v)	(*cpuintf[cputype].memory_write)(a,v)

/****************************************************************************
 * Globals
 ****************************************************************************/
int debug_key_pressed = 0;
int debug_key_delay = 0;

/****************************************************************************
 * Limits
 ****************************************************************************/
#define MAX_DATA	256 		/* Maximum memory size in bytes of a dump window */
#define MAX_REGS	64			/* Maximum number of registers for a CPU */
#define MAX_MEM 	2			/* You can't redefine this... too easy */

#define MAX_LOOPS	32			/* Maximum loop addresses recognized by trace */

#define EDIT_CMDS	0
#define EDIT_REGS	1			/* Just the order of the windows */
#define EDIT_DASM	2
#define EDIT_MEM1	3
#define EDIT_MEM2	4

#define DBG_WINDOWS 5

/* Some convenience macros to address the cpu'th window */
#define WIN_CMDS(cpu)   (cpu*DBG_WINDOWS+EDIT_CMDS)
#define WIN_REGS(cpu)	(cpu*DBG_WINDOWS+EDIT_REGS)
#define WIN_DASM(cpu)	(cpu*DBG_WINDOWS+EDIT_DASM)
#define WIN_MEM(cpu,n)	(cpu*DBG_WINDOWS+EDIT_MEM1+n)
#define WIN_MEM1(cpu)	(cpu*DBG_WINDOWS+EDIT_MEM1)
#define WIN_MEM2(cpu)	(cpu*DBG_WINDOWS+EDIT_MEM2)
#define WIN_HELP		(MAX_WINDOWS-1)
#define WIN_MSGBOX		(MAX_WINDOWS-2)

#define MODE_HEX_UINT8	0
#define MODE_HEX_UINT16 1
#define MODE_HEX_UINT32 2

#ifdef LSB_FIRST
#define UINT16_XOR_LE(o) (((o)&~1)|(((o)&1)^1))
#define UINT32_XOR_LE(o) (((o)&~3)|(((o)&3)^3))
#define UINT16_XOR_BE(o) (o)
#define UINT32_XOR_BE(o) (o)
#else
#define UINT16_XOR_LE(o) (o)
#define UINT32_XOR_LE(o) (o)
#define UINT16_XOR_BE(o) (((o)&~1)|(((o)&1)^1))
#define UINT32_XOR_BE(o) (((o)&~3)|(((o)&3)^3))
#endif

/****************************************************************************
 * Statics
 ****************************************************************************/
static int activecpu = INVALID;
static int previous_activecpu = INVALID;
static int totalcpu = 0;
static int cputype = 0;

static int dbg_fast = 0;
static int single_step = 0;
static int dbg_trace = 0;
static int do_update = 0;
static int in_debug = 0;

/* Allow condensed display using alternating dim, bright colors */
static int mem_condensed_display = 0;

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
static unsigned get_register_id( char **parg, int *size );
static unsigned get_register_or_value( char **parg, int *size );
static void trace_init( int count, char *filename );
static void trace_done( void );
static void trace_select( void );
static void trace_output( unsigned pc );

static int hit_break( void );
static int hit_watch( void );

static const char *find_rom_name( const char *type, int region, unsigned *base, unsigned start );
static const char *name_rdmem( unsigned base );
static const char *name_wrmem( unsigned base );
static const char *name_memory( unsigned base );

static int win_create(int n, int x, int y, int w, int h,
	UINT8 co_text, UINT8 co_frame, UINT8 chr, UINT32 attributes);
static int win_msgbox( UINT8 color, const char *title, const char *fmt, ... );
static void dbg_create_windows( void );

static unsigned dasm_line( unsigned pc, int times );
static void dasm_opcodes( int state );

static void dump_regs( void );
static unsigned dump_dasm( unsigned pc, int set_title );
static void dump_mem_hex( int which, unsigned len_addr, unsigned len_data );
static void dump_mem( int which, int set_title );

static void edit_cmds_info( void );
static int edit_cmds_parse( char *cmd );
static void edit_cmds_append( const char *src );

static void edit_regs( void );
static void edit_dasm( void );
static void edit_mem( int which );
static void edit_cmds(void);

static void cmd_help( void );
static void cmd_default( int code );
static void cmd_breakpoint_clear( void );
static void cmd_watchpoint_set( void );
static void cmd_breakpoint_set( void );
static void cmd_display_memory( void );
static void cmd_edit_memory( void );
static void cmd_dasm_to_file( void );
static void cmd_dump_to_file( void );
static void cmd_fast( void );
static void cmd_go_break( void );
static void cmd_here( void );
static void cmd_jump( void );
static void cmd_replace_register( void );
static void cmd_trace_to_file( void );
static void cmd_watchpoint_clear( void );
static void cmd_switch_window( void );
static void cmd_dasm_up( void );
static void cmd_dasm_down( void );
static void cmd_dasm_page_up( void );
static void cmd_dasm_page_down( void );
static void cmd_help( void );
static void cmd_toggle_breakpoint( void );
static void cmd_toggle_watchpoint( void );
static void cmd_run_to_cursor( void );
static void cmd_view_screen( void );
static void cmd_breakpoint_cpu( void );
static void cmd_step( void );
static void cmd_animate( void );
static void cmd_step_over( void );
static void cmd_go( void );

/****************************************************************************
 * Generic structure for editing values
 * x,y are the coordinates inside a window
 * w is the width in hex digits (aka nibbles)
 * n is the distance of the hex part from the start of the output (register)
 ****************************************************************************/
typedef struct {
	UINT8	x,y,w,n;
}	s_edit;

/****************************************************************************
 * Register display and editing structure
 ****************************************************************************/
typedef struct {
    UINT32  backup[MAX_REGS];       /* backup register values */
    UINT32  newval[MAX_REGS];       /* new register values */
	s_edit	edit[MAX_REGS]; 		/* list of x,y,w triplets for the register values */
	char	name[MAX_REGS][15+1];	/* ...fifteen characters enough!? */
	UINT8	id[MAX_REGS];			/* the ID of the register (cpu_get_reg/cpu_set_reg) */
    INT32   idx;                    /* index of current register */
    INT32   count;                  /* number of registers */
    INT32   nibble;                 /* edit nibble */
    INT32   color;                  /* color of the edit window */
    INT32   changed;
}	s_regs;

/****************************************************************************
 * Memory display and editing structure
 ****************************************************************************/
typedef struct {
    UINT8   backup[MAX_DATA];   /* backup data */
    UINT8   newval[MAX_DATA];   /* newly read data */
	s_edit	edit[MAX_DATA]; 	/* list of x,y,w triplets for the memory elements */
    UINT32  base;               /* current base address */
	UINT32	address;			/* current cursor address */
    INT32   offset;             /* edit offset */
    INT32   nibble;             /* edit nibble */
	INT32	bytes;				/* number of bytes per edit line */
	INT32	width;				/* width in nibbles of the edit line */
	INT32	size;				/* number of bytes in the edit window */
    UINT8   color;              /* color of the edit window */
	UINT8	mode;				/* 0 bytes, 1 words, 2 dword */
    UINT8   changed;
}	s_mem;

/****************************************************************************
 * Disassembler structure
 ****************************************************************************/
typedef struct {
	UINT32	pc_cpu; 			/* The CPUs PC */
	UINT32	pc_top; 			/* Top of the disassembly window PC */
	UINT32	pc_cur; 			/* Cursor PC */
	UINT32	pc_end; 			/* End of the disassembly window PC */
	UINT32	opcodes;			/* Display opcodes */
	UINT32	case_mode;			/* default, lower or upper case */
}	s_dasm;

/****************************************************************************
 * Tracing structure
 ****************************************************************************/
typedef struct {
    UINT32  last_pc[MAX_LOOPS];
    FILE    *file;
    INT32   iters;
    INT32   loops;
}	s_trace;

/****************************************************************************
 * Debugger structure. There is one instance per CPU
 ****************************************************************************/
typedef struct {
	UINT32	next_pc;
	UINT32	previous_sp;

	/* Break- and Watchpoints */
	UINT32	break_point;
	UINT32	break_temp;
	UINT32	break_cpunum;
	UINT32	watch_point;
	UINT32	watch_value;

    s_regs  regs;
    s_dasm  dasm;
    s_mem   mem[MAX_MEM];
    s_trace trace;

    char    cmdline[80+1];
	UINT8	window; 	/* edit what: cmds, regs, dasm, mem1, mem2 */

}	s_dbg;

static	s_dbg	dbg[MAX_CPU];

/* Covenience macros... keep the code readable */
#define DBG 	dbg[activecpu]
#define REGS	dbg[activecpu].regs
#define DASM	dbg[activecpu].dasm
#define MEM 	dbg[activecpu].mem
#define TRACE	dbg[trace_cpu].trace

#define CMD 	dbg[activecpu].cmdline

/****************************************************************************
 * Tracing
 ****************************************************************************/
static int trace_cpu = 0;
static int trace_on = 0;

/****************************************************************************
 * Commands structure
 ****************************************************************************/
typedef struct {
	int valid;				/* command is valid for which windows (0 all) */
    const char *name;       /* command (NULL none) */
	int key;				/* key code (0 none) */
	const char *info;		/* description of expected arguments */
	void (*function)(void); /* function handling the key/command */
}	s_command;

#define ALL 	((1<<EDIT_CMDS)|(1<<EDIT_REGS)|(1<<EDIT_DASM)|(1<<EDIT_MEM1)|(1<<EDIT_MEM2))
#define BROKEN	0x80000000

static s_command commands[] = {
{	(1<<EDIT_CMDS),
		"BC",       0,
		"Breakpoint Clear",
		cmd_breakpoint_clear },
{	(1<<EDIT_CMDS) | BROKEN,
		"BPW",      0,
		"Set Watchpoint <Address>",
		cmd_watchpoint_set },
{	(1<<EDIT_CMDS) | BROKEN,
		"BPX",      0,
		"Set Breakpoint <Address>",
		cmd_breakpoint_set },
{	(1<<EDIT_CMDS),
		"D",        0,
		"Display <1|2> <Address>",
		cmd_display_memory },
{	(1<<EDIT_CMDS) | BROKEN,
		"DASM",     0,
		"Disassemble to <FileName> <StartAddr> <EndAddr>",
		cmd_dasm_to_file },
{	(1<<EDIT_CMDS) | BROKEN,
		"DUMP",     0,
		"Dump to <FileName> <StartAddr> <EndAddr>",
		cmd_dump_to_file },
{	(1<<EDIT_CMDS),
		"E",        0,
		"Edit <1|2> [address]",
		cmd_edit_memory },
{	(1<<EDIT_CMDS),
		"F",        0,
		"Fast",
		cmd_fast },
{	(1<<EDIT_CMDS),
		"G",        0,
		"Go [Break at <Address>]",
		cmd_go_break },
{	(1<<EDIT_CMDS),
		"HERE",     0,
		"Run to cursor",
		cmd_here },
{	(1<<EDIT_CMDS),
		"J",        0,
		"Jump to <Address>",
		cmd_jump },
{	(1<<EDIT_CMDS),
		"R",        0,
		"Replace [Register] = [<Register>|<Value>]",
		cmd_replace_register },
{	(1<<EDIT_CMDS),
		"TRACE",    0,
		"Trace to <FileName>|OFF",
		cmd_trace_to_file },
{	(1<<EDIT_CMDS),
		"WC",       0,
		"Watchpoint Clear",
		cmd_watchpoint_clear },
{	(1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_UP,
		"Move cursor up in disassembler window",
		cmd_dasm_up },
{	(1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_DOWN,
		"Move cursor down in disassembler window",
		cmd_dasm_down },
{	(1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_PGUP,
		"Move cursor up one page in disassembler window",
		cmd_dasm_page_up },
{	(1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_PGDN,
		"Move cursor down one page in disassembler window",
		cmd_dasm_page_down },
{	ALL,
		0,			OSD_KEY_TAB,
		"Switch between windows (backwards SHIFT+TAB)",
		cmd_switch_window },
{	ALL,
		0,			OSD_KEY_F1,
		"Help - maybe you realized this ;)",
		cmd_help },
{   (1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_F2,
		"Toggle breakpoint at current cursor position",
		cmd_toggle_breakpoint },
{	(1<<EDIT_CMDS)|(1<<EDIT_DASM),
		0,			OSD_KEY_F4,
		"Run to cursor",
		cmd_run_to_cursor },
{	(1<<EDIT_MEM1)|(1<<EDIT_MEM2),
		0,			OSD_KEY_F4,
		"Set watchpoint at current memory location",
		cmd_toggle_watchpoint },
{   ALL,
		0,			OSD_KEY_F5,
		"View emulation screen",
		cmd_view_screen },
{	ALL,
		0,			OSD_KEY_F6,
		"Breakpoint to the next CPU",
		cmd_breakpoint_cpu },
{	ALL,
		0,			OSD_KEY_F8,
		"Step",
		cmd_step },
{	ALL,
		0,			OSD_KEY_F9,
		"Animate (Trace at high speed)",
		cmd_animate },
{	ALL,
		0,			OSD_KEY_F10,
		"Step over",
		cmd_step_over },
{	ALL,
		0,			OSD_KEY_F12,
		"Go!",
		cmd_go },
{	ALL,
		0,			OSD_KEY_ESC,
		"Go!",
        cmd_go },
/* This is the end of the list! */
{ 0,    },      
};

/**************************************************************************
 * xtou
 * Hex to unsigned.
 * The pointer to the char* is placed after all consecutive hex digits
 * and trailing space. The pointer to int size (if given) contains the
 * number of digits found.
 **************************************************************************/
INLINE unsigned xtou( char **parg, int *size)
{
	unsigned val = 0, digit;

    if (size) *size = 0;
	while( isxdigit( *(*parg) ) )
	{
		digit = toupper(*(*parg)) - '0';
		if( digit > 9 ) digit -= 7;
		val = (val << 4) | digit;
		if( size ) (*size)++;
		(*parg) += 1;
	}
	while( isspace(*(*parg)) ) *parg += 1;
	return val;
}

/**************************************************************************
 * utox
 * Unsigned to hex.
 * The unsigned val is converted to a size digits hex string
 * (with leading zeroes).
 **************************************************************************/
INLINE char *utox( unsigned val, unsigned size )
{
	static char buffer[32+1];
	static char digit[] = "0123456789ABCDEF";
	char *dst = &buffer[size];
	*dst-- = '\0';
	while( size-- > 0 )
	{
		*dst-- = digit[val & 15];
		val >>= 4;
	}
	return buffer;
}

/**************************************************************************
 * get_register_id
 * Return the ID for a register if the string at *parg matches one
 * of the register names for the active cpu.
 **************************************************************************/
static unsigned get_register_id( char **parg, int *size )
{
	int i, l;
	for( i = 0; i < REGS.count; i++ )
	{
		l = strlen( REGS.name[i] );
		if( l > 0 && !strncmp( *parg, REGS.name[i], l ) )
		{
			if( !isalnum( (*parg)[l] ) )
			{
				if( size ) *size = l;
				*parg += l;
				while( isspace(*(*parg)) ) *parg += 1;
				return REGS.id[i];
			}
		}
    }
	if( size ) *size = 0;
	return 0;
}

/**************************************************************************
 * get_register_or_value
 * Return the value for a register if the string at *parg matches one
 * of the register names for the active cpu. Otherwise get a hex
 * value from *parg. In both cases set the pointer int size to the
 * length of the name or digits found (if size is not NULL)
 **************************************************************************/
static unsigned get_register_or_value( char **parg, int *size )
{
	int regnum, l;

	regnum = get_register_id( parg, &l );
	if( regnum > 0 )
	{
		if( size ) *size = l;
		return cpu_get_reg( regnum );
	}
	/* default to hex value */
	return xtou( parg, size );
}

/**************************************************************************
 * trace_init
 * Creates trace output files for all CPUs
 * Resets the loop and interation counters and the last PC array
 **************************************************************************/
static void trace_init( int count, char *filename )
{
	char name[100];
	for( trace_cpu = 0; trace_cpu < totalcpu; trace_cpu++ )
	{
		sprintf( name, "%s.%d", filename, trace_cpu );
		TRACE.file = fopen(name,"w");
		TRACE.iters = 0;
		TRACE.loops = 0;
		memset(TRACE.last_pc, 0xff, sizeof(TRACE.last_pc));
	}
	trace_cpu = 0;
}

/**************************************************************************
 * trace_done
 * Closes the trace output files
 **************************************************************************/
void trace_done(void)
{
	for( trace_cpu = 0; trace_cpu < totalcpu; trace_cpu++ )
	{
		if( TRACE.file )
			fclose( TRACE.file );
		TRACE.file = NULL;
	}
}

/**************************************************************************
 * trace_select
 * Switches tracing to the active CPU
 **************************************************************************/
static void trace_select( void )
{
	if( trace_on && TRACE.file )
	{
		if( TRACE.loops )
		{
			fprintf( TRACE.file,
				"\n   (loops for %d instructions)\n\n",
				TRACE.loops );
			TRACE.loops = 0;
		}
		fprintf(TRACE.file,"\n=============== End of iteration #%d ===============\n\n",TRACE.iters++);
		fflush(TRACE.file);
	}
	if( activecpu < totalcpu )
		trace_cpu = activecpu;
}

/**************************************************************************
 * trace_output
 * Outputs the next disassembled instruction to the trace file
 * Loops are detected and a loop count is output after the
 * first repetition instead of disassembling the loop over and over
 **************************************************************************/
static void trace_output( unsigned pc )
{
	if( trace_on && TRACE.file )
	{
		int count, i;

		// check for trace_loops
		for( i = count = 0; i < MAX_LOOPS; i++ )
			if( TRACE.last_pc[i] == pc )
				count++;
		if( count > 1 )
		{
			TRACE.loops++;
		}
		else
		{
			if( TRACE.loops )
			{
				fprintf( TRACE.file,
					"\n   (trace_loops for %d instructions)\n\n",
					TRACE.loops );
				TRACE.loops = 0;
			}
			fprintf( TRACE.file, "%s", cpu_pc() );
			fprintf( TRACE.file, "%s\n", cpu_dasm() );
			memmove(
				&TRACE.last_pc[0],
				&TRACE.last_pc[1],
				(MAX_LOOPS-1)*sizeof(TRACE.last_pc[0]) );
			TRACE.last_pc[MAX_LOOPS-1] = pc;
		}
    }
}

/**************************************************************************
 * hit_break
 * Return non zero if the breakpoint for the activecpu,
 * the temporary breakpoint or the 'activecpu' break was hit
 **************************************************************************/
static int hit_break(void)
{
	UINT32 pc;

	if( DBG.break_point == INVALID &&
		DBG.break_temp == INVALID &&
		DBG.break_cpunum == INVALID )
		return 0;

	pc = cpu_get_pc();

	if( DBG.break_point == pc )
		return 1;
	if( DBG.break_temp == pc )
	{
		DBG.break_temp = INVALID;
		return 1;
	}
	if( DBG.break_cpunum == activecpu )
	{
		DBG.break_cpunum = INVALID;
		return 1;
	}

	return 0;
}

/**************************************************************************
 * hit_watch
 * Return non zero if the watchpoint for the activecpu
 * was hit (ie. monitored data changed)
 **************************************************************************/
static int hit_watch(void)
{
	UINT32 data;

	if( DBG.watch_point == INVALID ) return 0;

	data = cpuintf[cputype].memory_read(DBG.watch_point);

	if( DBG.watch_value == data )
	{
		DBG.watch_value = data;
		return 1;
	}
	return 0;
}


/**************************************************************************
 * find_rom_name
 * Find the name for a rom from the drivers list
 **************************************************************************/
static const char *find_rom_name( const char *type, int region, unsigned *base, unsigned start )
{
    static char buffer[16][15+1];
    static int which = 0;
    const struct RomModule *romp = Machine->gamedrv->rom;
	unsigned offset = *base;

    which = ++which % 16;

    while( romp->name || romp->offset || romp->length )
    {
		romp++; /* skip memory region definition */

        while( romp->length )
        {
            const char *name;
            int length;

            name = romp->name;
            length = 0;

            do
            {
                /* ROM_RELOAD */
                if (romp->name == (char *)-1)
                    length = 0; /* restart */

                length += romp->length & ~ROMFLAG_MASK;

                romp++;

            } while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

			/* region found already ? */
			if( region == 0 )
			{
				/* address inside that range ? */
				if( offset < length )
				{
					/* put back that offset */
					*base = offset;
					sprintf(buffer[which], "%s", name);
					return buffer[which];
				}
				/* subtract length of that ROM */
				offset -= length;
			}

        }
		--region;
    }

    /* default to ROM + xxxx (base - start) */
	*base -= start;
	return type;
}

/**************************************************************************
 * name_rdmem
 * Find a descriptive name for the given memory read region of activecpu
 **************************************************************************/
static const char *name_rdmem( unsigned base )
{
	static char buffer[16][79+1];
    static int which = 0;
	const struct MachineCPU *cpu = Machine->drv->cpu;
	const struct MemoryReadAddress *mr = cpu->memory_read;
	int ram_cnt = 1, nop_cnt = 1;
    const char *name;
	char *dst;

	which = ++which % 16;
	dst = buffer[which];
	*dst = '\0';

	while( *dst == '\0' && mr->start != -1 )
    {
        if( base >= mr->start && base <= mr->end )
        {
			unsigned offset = base - mr->start;

			if( mr->description )
				sprintf(dst, "%s+%04X", mr->description, offset );
			else
			if( mr->base && *mr->base == videoram )
				sprintf(dst, "video+%04X", offset );
			else
			if( mr->base && *mr->base == colorram )
				sprintf(dst, "color+%04X", offset );
			else
			if( mr->base && *mr->base == spriteram )
				sprintf(dst, "sprite+%04X", offset );
			else
			switch( (FPTR)mr->handler )
            {
			case (FPTR)MRA_RAM:
				sprintf(dst, "RAM%d+%04X", ram_cnt, offset );
				break;
			case (FPTR)MRA_ROM:
				name = find_rom_name("ROM", cpu->memory_region, &base, mr->start );
				sprintf(dst, "%s+%04X", name, base );
				break;
			case (FPTR)MRA_BANK1:
				sprintf(dst, "BANK1+%04X", offset );
				break;
			case (FPTR)MRA_BANK2:
				sprintf(dst, "BANK2+%04X", offset );
				break;
			case (FPTR)MRA_BANK3:
				sprintf(dst, "BANK3+%04X", offset );
				break;
			case (FPTR)MRA_BANK4:
				sprintf(dst, "BANK4+%04X", offset );
				break;
			case (FPTR)MRA_BANK5:
				sprintf(dst, "BANK5+%04X", offset );
				break;
			case (FPTR)MRA_BANK6:
				sprintf(dst, "BANK6+%04X", offset );
				break;
			case (FPTR)MRA_BANK7:
				sprintf(dst, "BANK7+%04X", offset );
				break;
			case (FPTR)MRA_BANK8:
				sprintf(dst, "BANK8+%04X", offset );
				break;
			case (FPTR)MRA_NOP:
				sprintf(dst, "NOP%d+%04X", nop_cnt, offset );
				break;
			default:
				if( (FPTR)mr->handler == (FPTR)input_port_0_r )
					sprintf(dst, "input_port_0+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_1_r )
					sprintf(dst, "input_port_1+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_2_r )
					sprintf(dst, "input_port_2+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_3_r )
					sprintf(dst, "input_port_3+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_4_r )
					sprintf(dst, "input_port_4+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_5_r )
					sprintf(dst, "input_port_5+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_6_r )
					sprintf(dst, "input_port_6+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_7_r )
					sprintf(dst, "input_port_7+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_8_r )
					sprintf(dst, "input_port_8+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_9_r )
					sprintf(dst, "input_port_9+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_10_r )
					sprintf(dst, "input_port_10+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_11_r )
					sprintf(dst, "input_port_11+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_12_r )
					sprintf(dst, "input_port_12+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_13_r )
					sprintf(dst, "input_port_13+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_14_r )
					sprintf(dst, "input_port_14+%04X", offset );
				else
				if( (FPTR)mr->handler == (FPTR)input_port_15_r )
					sprintf(dst, "input_port_15+%04X", offset );
            }
        }
        switch( (FPTR)mr->handler )
        {
			case (FPTR)MRA_RAM: ram_cnt++; break;
			case (FPTR)MRA_NOP: nop_cnt++; break;
        }
        mr++;
    }

	return dst;
}

/**************************************************************************
 * name_wrmem
 * Find a descriptive name for the given memory write region of activecpu
 **************************************************************************/
static const char *name_wrmem( unsigned base )
{
    static char buffer[16][79+1];
    static int which = 0;
	const struct MachineCPU *cpu = Machine->drv->cpu;
    const struct MemoryWriteAddress *mw = cpu->memory_write;
	int ram_cnt = 1, nop_cnt = 1;
	const char *name;
    char *dst;

    which = ++which % 16;
    dst = buffer[which];
	*dst = '\0';

    ram_cnt = nop_cnt = 1;
	while( *dst == '\0' && mw->start != -1 )
    {
        if( base >= mw->start && base <= mw->end )
        {
			if( mw->description )
				sprintf(dst, "%s+%04X", mw->description, base - mw->start );
			else
			if( mw->base && *mw->base == videoram )
				sprintf(dst, "video+%04X", base - mw->start );
			else
			if( mw->base && *mw->base == colorram )
				sprintf(dst, "color+%04X", base - mw->start );
			else
			if( mw->base && *mw->base == spriteram )
				sprintf(dst, "sprite+%04X", base - mw->start );
            else
            switch( (FPTR)mw->handler )
            {
			case (FPTR)MWA_RAM:
				sprintf(dst, "RAM%d+%04X", ram_cnt, base - mw->start );
				break;
			case (FPTR)MWA_ROM:
				name = find_rom_name("ROM", cpu->memory_region, &base, mw->start );
				sprintf(dst, "%s+%04X", name, base );
				break;
			case (FPTR)MWA_RAMROM:
				name = find_rom_name("RAMROM", cpu->memory_region, &base, mw->start);
				sprintf(dst, "%s+%04X", name, base );
				break;
			case (FPTR)MWA_BANK1:
				sprintf(dst, "BANK1+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK2:
				sprintf(dst, "BANK2+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK3:
				sprintf(dst, "BANK3+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK4:
				sprintf(dst, "BANK4+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK5:
				sprintf(dst, "BANK5+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK6:
				sprintf(dst, "BANK6+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK7:
				sprintf(dst, "BANK7+%04X", base - mw->start );
				break;
			case (FPTR)MWA_BANK8:
				sprintf(dst, "BANK8+%04X", base - mw->start );
				break;
			case (FPTR)MWA_NOP:
				sprintf(dst, "NOP%d+%04X", nop_cnt, base - mw->start );
				break;
			}
        }
        switch( (FPTR)mw->handler )
        {
            case (FPTR)MRA_RAM: ram_cnt++; break;
            case (FPTR)MRA_NOP: nop_cnt++; break;
        }
        mw++;
    }

    return dst;
}

/**************************************************************************
 * name_memory
 * Find a descriptive name for the given memory region of activecpu
 **************************************************************************/
static const char *name_memory( unsigned base )
{
	static char buffer[8][79+1];
    static int which = 0;
	const char *rd, *wr;

	/* search readmem and writemem names */
    rd = name_rdmem( base );
	wr = name_wrmem( base );

	/* both empty, so it's no specific region */
    if( *rd == '\0' && *wr == '\0' )
	{
		which = ++which % 8;
		sprintf(buffer[which], "N/A:%04X", base);
        return buffer[which];
    }

	which = ++which % 8;

    /* both names differ? */
    if( strcmp(rd,wr) )
		/* well, return one of the names... */
        sprintf(buffer[which], "%s\t%s", rd, wr);
	else
		/* return the name for readmem... */
		sprintf(buffer[which], "%s", rd);

    return buffer[which];
}

/**************************************************************************
 * win_create
 * Wrapper function to fill a struct sWindow and call win_open()
 **************************************************************************/
static int win_create(int n, int x, int y, int w, int h,
	UINT8 co_text, UINT8 co_frame, UINT8 chr, UINT32 attributes)
{
    struct sWindow win;
	/* fill in the default values for window creation */
	memset( &win, 0, sizeof(struct sWindow) );
	win.filler = chr;
	win.x = x;
	win.y = y;
	win.w = w;
	win.h = h;
	win.flags = NO_SCROLL | NO_WRAP | BORDER_TOP | ((n)? HIDDEN : 0) | attributes;
	win.co_text = co_text;
	win.co_frame = co_frame;
	win.co_title = COLOR_TITLE;
	win.saved_text = ' ';
	win.saved_attr = WIN_WHITE;
	return win_open(n, &win);
}

static int win_msgbox( UINT8 color, const char *title, const char *fmt, ... )
{
	UINT32 win = WIN_MSGBOX;
    va_list arg;
	int i;

	win_create( win,
		4,10,40,5, color, COLOR_LINE, ' ',
		BORDER_TOP | BORDER_LEFT | BORDER_RIGHT | BORDER_BOTTOM );
	win_set_title( win, title );

    va_start( arg, fmt );
	win_vprintf( win, fmt, arg );
	va_end( arg );

    win_show( win );
    i = osd_debug_readkey();
    win_close( win );

    return i;
}

/**************************************************************************
 * dbg_create_windows
 * Depending on the CPU type, create a window layout specified
 * by the CPU core - returned by function cputype_win_layout()
 **************************************************************************/
#define REGS_X	win_layout[0*4+0]
#define REGS_Y  win_layout[0*4+1]
#define REGS_W  win_layout[0*4+2]
#define REGS_H  win_layout[0*4+3]
#define DASM_X	win_layout[1*4+0]
#define DASM_Y	win_layout[1*4+1]
#define DASM_W	win_layout[1*4+2]
#define DASM_H	win_layout[1*4+3]
#define MEM1_X	win_layout[2*4+0]
#define MEM1_Y	win_layout[2*4+1]
#define MEM1_W	win_layout[2*4+2]
#define MEM1_H	win_layout[2*4+3]
#define MEM2_X	win_layout[3*4+0]
#define MEM2_Y	win_layout[3*4+1]
#define MEM2_W	win_layout[3*4+2]
#define MEM2_H	win_layout[3*4+3]
#define CMDS_X	win_layout[4*4+0]
#define CMDS_Y	win_layout[4*4+1]
#define CMDS_W	win_layout[4*4+2]
#define CMDS_H	win_layout[4*4+3]
static void dbg_create_windows( void )
{
    UINT32 flags;
    int i;

    /* Initialize windowing engine */
	win_init_engine(80,25);

	for( i = 0; i < totalcpu; i++ )
	{
		const UINT8 *win_layout = (UINT8*)cpunum_win_layout(i);

		flags = BORDER_TOP;
		if( REGS_X + REGS_W < 80) flags |= BORDER_RIGHT;
		win_create(WIN_REGS(i), REGS_X,REGS_Y,REGS_W,REGS_H,
			COLOR_REGS, COLOR_LINE, ' ', flags );

		flags = BORDER_TOP | BORDER_RIGHT;
		win_create(WIN_DASM(i), DASM_X,DASM_Y,DASM_W,DASM_H,
			COLOR_DASM, COLOR_LINE, ' ', flags );

		flags = BORDER_TOP;
		if( MEM1_X + MEM1_W < 80) flags |= BORDER_RIGHT;
		win_create(WIN_MEM1(i), MEM1_X,MEM1_Y,MEM1_W,MEM1_H,
			COLOR_MEM1, COLOR_LINE, ' ', flags );

		flags = BORDER_TOP;
		if( MEM2_X + MEM2_W < 80) flags |= BORDER_RIGHT;
		win_create(WIN_MEM2(i), MEM2_X,MEM2_Y,MEM2_W,MEM2_H,
			COLOR_MEM2, COLOR_LINE, ' ', flags );

		flags = BORDER_TOP;
		win_create(WIN_CMDS(i), CMDS_X,CMDS_Y,CMDS_W,CMDS_H,
			COLOR_CMDS, COLOR_LINE, ' ', flags );
		win_set_title(WIN_CMDS(i), "Command");
	}
}

/**************************************************************************
 * dasm_line
 * disassemble <times> instructions from pc and return the final pc
 **************************************************************************/
static unsigned dasm_line( unsigned pc, int times )
{
    unsigned pc_saved = cpu_get_pc();

    cpu_set_pc( pc );
    while( times-- > 0 )
        cpu_dasm();
    pc = cpu_get_pc();
    cpu_set_pc( pc_saved );

    return pc;
}

/**************************************************************************
 * dasm_opcodes
 * Set opcodes dump on disassembly on/off
 * dw is the maximum width of the opcode dump section
 **************************************************************************/
static void dasm_opcodes( int state )
{
    UINT32 win = WIN_DASM(activecpu);
    UINT32 w = win_get_w( win );
	UINT32 dw = ((INSTL / ALIGN) * ALIGN * 2) + 1;

    if( DASM.opcodes != state )
    {
        DASM.opcodes = state;
        if( state )
            win_set_w( win, w + dw  );
        else
            win_set_w( win, w - dw );
        DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
    }
}

/**************************************************************************
 * dump_regs
 * Update the register display
 * Compare register values against the ones stored in reg->backup[]
 * Store new values in reg->newval[] which is copied to reg->backup[]
 * before the next instruction is executed (at the end of MAME_Debug).
 **************************************************************************/
static void dump_regs( void )
{
	char title[80+1];
	UINT32 win = WIN_REGS(activecpu);
	s_regs *regs = &REGS;
	s_edit *pedit = regs->edit;
	UINT32 *old = regs->backup;
	UINT32 *val = regs->newval;
	const char *name = cpu_name(), *flags = cpu_flags();
    int w = win_get_w(win);
	int h = win_get_h(win);
	const INT8 *reg = (INT8*)cpu_reg_layout();
	int i, j, l;

	win_set_curpos( win, 0, 0 );
	sprintf( title, "CPU #%d %-8s Flags:%s  Cycles:%6u", activecpu, name, flags, cpu_geticount() );
	l = strlen(title);
	if( l + 2 < w )
	{
        /* Everything should fit into the caption */
		if( l + 4 < w )
			/* We can even separate the cycles to the right corner */
			sprintf( title, "CPU #%d %-8s Flags:%s\tCycles:%6u", activecpu, name, flags, cpu_geticount() );
		win_set_title( win, title );
	}
	else
	{
		/* At least CPU # and flags should fit into the caption */
		sprintf( title, "CPU #%d %-8s Flags:%s", activecpu, name, flags );
		if( strlen(title) + 2 < w )
		{
			win_set_title( win, title );
			win_printf( win, "Cycles:%6u\n", cpu_geticount() );
			if( --h <= 0) return;
		}
		else
		{
			/* Only CPU # and name fit into the caption */
			sprintf( title, "CPU #%d %-8s", activecpu, name );
			win_set_title( win, title );
			if( strlen(cpu_flags()) + 8 < w )
				win_printf( win, "Flags: %s\n", flags );
			else
				win_printf( win, "F:%s\n", flags );
			if( --h <= 0) return;
			win_printf( win, "Cycles:%6u\n", cpu_geticount() );
			if( --h <= 0) return;
		}
	}

	for( i = 0, j = 0; *reg; i++, reg++ )
	{
		if( *reg == -1 )
		{
			win_putc( win, '\n');
			if( --h <= 0 ) return;
		}
		else
		{
			regs->id[j] = *reg;
			*val = cpu_get_reg(regs->id[j]);
			if( *val != *old )
			{
				regs->changed = 1;
				win_set_color( win, COLOR_CHANGES );
			}
			else
			{
				win_set_color( win, COLOR_REGS );
			}
			name = cpu_dump_reg(*reg);

			/* edit structure not yet initialized? */
			if( regs->count == 0 )
			{
				char *p;
				/* get the cursor position */
				pedit->x = win_get_cx(win);
				pedit->y = win_get_cy(win);
				strncpy( regs->name[j], name, sizeof(regs->name[j]) - 1 );
				/* find a colon */
				p = strchr( regs->name[j], ':');
				if( p )
				{
                    pedit->w = strlen( p + 1 );
				}
				else
				{
					p = strchr( regs->name[j], '\'' );
					if( p )
					{
                        ++p;    /* Include the apostrophe in the name! */
						pedit->w = strlen( p );
                    }
				}
				/* TODO: other characters to delimit a register name from it's value? */
				if( p )
				{
					/* length of the hex string (number of nibbles) */
					pedit->n = strlen( name ) - pedit->w;
					/* terminate name at (or after) the delimiting character */
					*p = '\0';
					/* eventually strip trailing spaces */
					while( *--p == ' ' ) *p = '\0';
				}
				else
				{
					/* this is certainly wrong :( */
					pedit->w = strlen(regs->name[j]);
					pedit->n = 0;
				}
			}
			pedit++;

			win_printf( win, "%s", name );

			/* If no row break follows, advance to the next tab stop */
			if( reg[1] != -1 )
				win_putc( win, '\t' );
			val++;
			old++;
			j++;
		}
	}

    regs->count = i;
}

/**************************************************************************
 * dump_dasm
 * Update the disassembler display
 **************************************************************************/
static unsigned dump_dasm( unsigned pc, int set_title )
{
    UINT32 win = WIN_DASM(activecpu);
    int w = win_get_w(win);
    int h = win_get_h(win);
	int x, y, l, line_pc_cpu = INVALID, line_pc_cur = INVALID;
    UINT8 color;
	const char *dasm;
	unsigned pc_first = pc;
	unsigned pc_saved = cpu_get_pc();

	if( set_title )
		win_set_title( win, name_rdmem(pc) );

	while( line_pc_cpu == INVALID )
	{
		pc = pc_first;
		cpu_set_pc( pc );

		for( y = 0; y < h; y++ )
		{
			if( pc == DBG.break_point )
				color = COLOR_BREAKPOINT;
			else
				color = COLOR_CODE;
			if( pc == DASM.pc_cpu )
			{
				color = (color & 0x0f) | (COLOR_PC & 0xf0);
				line_pc_cpu = y;
			}
			if( pc == DASM.pc_cur )
			{
				color = (color & 0x0f) | (COLOR_CURSOR & 0xf0);
				line_pc_cur = y;
			}
			win_set_curpos( win, 0, y );
            win_set_color( win, color );
			l = win_printf( win, "%s ", cpu_pc() );
			dasm = cpu_dasm();
			if( DBG.window == EDIT_DASM && DASM.opcodes )
			{
				switch( ALIGN )
				{
					case 1:
						for( x = 0; x < INSTL; x++ )
						{
							if ( cpu_get_pc() > pc )
							{
								l += win_printf( win, "%02X ", RDMEM(pc) );
								pc++;
							}
							else l += win_printf( win, "   ");
						}
						break;
                    case 2:
						for( x = 0; x < INSTL; x += 2 )
                        {
                            if ( cpu_get_pc() > pc )
							{
                                if( ENDIAN == CPU_IS_LE )
									l += win_printf( win, "%02X%02X ",
										RDMEM(UINT16_XOR_LE(pc+0)),
										RDMEM(UINT16_XOR_LE(pc+1)) );
								else
									l += win_printf( win, "%02X%02X ",
										RDMEM(UINT16_XOR_BE(pc+0)),
										RDMEM(UINT16_XOR_BE(pc+1)) );
                                pc += 2;
							}
							else l += win_printf( win, "     ");
                        }
                        break;
					case 4:
						for( x = 0; x < INSTL; x += 4 )
                        {
                            if ( cpu_get_pc() > pc )
							{
                                if( ENDIAN == CPU_IS_LE )
									l += win_printf( win, "%02X%02X%02X%02X ",
										RDMEM(UINT32_XOR_LE(pc+0)),
										RDMEM(UINT32_XOR_LE(pc+1)),
										RDMEM(UINT32_XOR_LE(pc+2)),
										RDMEM(UINT32_XOR_LE(pc+3)) );
								else
									l += win_printf( win, "%02X%02X%02X%02X ",
										RDMEM(UINT32_XOR_BE(pc+0)),
										RDMEM(UINT32_XOR_BE(pc+1)),
										RDMEM(UINT32_XOR_BE(pc+2)),
                                        RDMEM(UINT32_XOR_BE(pc+3)) );
								pc += 2;
							}
							else l += win_printf( win, "     ");
                        }
                        break;
                }
			}
			else
			{
				pc = cpu_get_pc();
			}
            win_printf( win, "%-*.*s", w-l, w-l, dasm );
		}
		if( line_pc_cpu == INVALID )
		{
			/*
			 * We didn't find the exact instruction of the CPU PC.
			 * This has to be caused by a jump into the midst of
			 * another instruction down from the top. If the CPU PC
			 * is between pc_first and pc (end), try again on next
			 * instruction size boundary, else bail out...
			 */
			if( DASM.pc_cpu > pc_first && DASM.pc_cpu < pc )
				pc_first += ALIGN;
			else
				line_pc_cpu = 0;
		}
	}

	win_set_curpos( win, 0, line_pc_cur );

	cpu_set_pc( pc_saved );

    return pc;
}

/**************************************************************************
 * dump_mem_hex
 * Update a memory window using the cpu_readmemXXX function
 * Changed values are displayed using foreground color COLOR_CHANGES
 * The new values are stored into mem->newval[] of the activecpu
 **************************************************************************/
static void dump_mem_hex( int which, unsigned len_addr, unsigned len_data )
{
	UINT32 win = WIN_MEM(activecpu,which);
	s_mem *mem = &MEM[which];
	s_edit *pedit = mem->edit;
	int w = win_get_w(win);
	int h = win_get_h(win);
	UINT8 *val = mem->newval;
	UINT8 *old = mem->backup;
	UINT8 color, dim_bright = 0;
	UINT8 spc_addr = 0; 	/* assume no space after address */
	UINT8 spc_data = 1; 	/* assume one space between adjacent data elements */
	UINT8 spc_hyphen = 0;	/* assume no space around center hyphen */
	unsigned offs, column;

	/* how many elements (bytes,words,dwords) will fit in a line? */
	mem->width = (w - len_addr - 1) / (len_data + spc_data);

	/* display multiples of eight bytes per line only */
	if( mem->width > ((16/len_data)-1) )
		mem->width &= ~((16/len_data)-1);

	/* Is bytes per line not divideable by eight? */
	if( mem_condensed_display && (mem->width & 7) )
	{
		/* We try an alternating dim,bright layout w/o data spacing */
		spc_data = 0;
		/* how many bytes will fit in a line? */
		mem->width = (w - len_addr - 1) / len_data;
		/* display multiples of eight data elements per line only */
		if( mem->width > ((16/len_data)-1) )
			mem->width &= ~((16/len_data)-1);
		dim_bright = 0x08;
	}

	/* calculate number of bytes per line */
	mem->bytes = mem->width * len_data / 2;
	/* calculate the mem->size using that data width */
	mem->size = mem->bytes * h;

	/* will a space after the address fit into the line? */
	if( ( len_addr + spc_addr + mem->width * (len_data + spc_data) + 1 ) < w )
		spc_addr = 1;

	/* will two spaces around the center hyphen fit into the line ? */
	if( ( len_addr + spc_addr + mem->width * (len_data + spc_data) + 2 ) < w )
		spc_hyphen = 1;

	win_set_curpos( win, 0, 0 );

	for( offs = 0, column = 0; offs < mem->size; offs++, old++, val++ )
	{
		color = mem->color;
		switch( len_data )
		{
			case 2:
				*val = RDMEM( (mem->base+offs) & AMASK );
				break;
			case 4:
				if( ENDIAN == CPU_IS_LE )
					*val = RDMEM( (mem->base+UINT16_XOR_LE(offs)) & AMASK );
				else
					*val = RDMEM( (mem->base+UINT16_XOR_BE(offs)) & AMASK );
                break;
			case 8:
				if( ENDIAN == CPU_IS_LE )
					*val = RDMEM( (mem->base+UINT32_XOR_LE(offs)) & AMASK );
				else
					*val = RDMEM( (mem->base+UINT32_XOR_BE(offs)) & AMASK );
                break;
		}

		if( column == 0 )
		{
			win_set_color( win, color );
			win_printf( win, "%0*X:%*s",
				len_addr, (mem->base + offs) & AMASK, spc_addr, "" );
		}

		if( *val != *old )
		{
			mem->changed = 1;
			color = COLOR_CHANGES;
		}

		if( (column * 2 / len_data) & 1 )
			color ^= dim_bright;

		/* edit structure not yet initialized? */
		if( pedit->w == 0 )
		{
			/* store memory edit x,y */
			pedit->x = win_get_cx( win );
			pedit->y = win_get_cy( win );
			pedit->w = 2;
			if( ENDIAN == CPU_IS_BE )
				pedit->n = column % (len_data / 2);
			else
				pedit->n = (column % (len_data / 2)) ^ ((len_data / 2) - 1);
		}
		pedit++;

		win_set_color( win, color );
		win_printf( win, "%02X", *val );

		if( ++column < mem->bytes )
		{
			if( column == mem->bytes / 2 )
				win_printf( win, "%*s-%*s", spc_hyphen, "", spc_hyphen, "" );
			else
			if( spc_data && (column * 2 % len_data) == 0 )
				win_putc( win, ' ' );
		}
		else
		{
			win_putc( win, '\n');
			column = 0;
		}
	}
}

/**************************************************************************
 * dump_mem
 * Update a memory window
 * Dispatch to one of the memory handler specific output functions
 **************************************************************************/
static void dump_mem( int which, int set_title )
{
	s_mem *mem = &MEM[which];
	unsigned len_addr = (ABITS + 3) / 4;

    if( set_title )
		win_set_title( WIN_MEM(activecpu,which), name_memory(mem->base) );

	switch( mem->mode )
	{
		case MODE_HEX_UINT8:  dump_mem_hex( which, len_addr, 2 ); break;
		case MODE_HEX_UINT16: dump_mem_hex( which, len_addr, 4 ); break;
		case MODE_HEX_UINT32: dump_mem_hex( which, len_addr, 8 ); break;
	}
}

/**************************************************************************
 * edit_regs
 * Edit the registers
 **************************************************************************/
static void edit_regs( void )
{
	UINT32 win = WIN_REGS(activecpu);
	s_regs *regs = &REGS;
	s_edit *pedit = regs->edit;
	unsigned shift, mask, val;
    const char *k;
	int i, x, y;

	win_set_curpos( win, pedit[regs->idx].x + pedit[regs->idx].n + regs->nibble, pedit[regs->idx].y );
	ScreenSetCursor( win_get_cy_abs(win), win_get_cx_abs(win) );

    i = osd_debug_readkey();
	k = osd_key_name(i);

	shift = (pedit->w - 1 - regs->nibble) * 4;
	mask = ~(0x0000000f << shift);

	if( strlen(k) == 1 )
	{
		switch( k[0] )
		{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9': case 'A': case 'B':
			case 'C': case 'D': case 'E': case 'F':
				val = k[0] - '0';
				if( val > 9 ) val -= 7;
				val <<= shift;
				/* now modify the register */
				cpu_set_reg( regs->id[regs->idx],
					( cpu_get_reg( regs->id[regs->idx] ) & mask ) | val );
				dump_regs();
				i = OSD_KEY_RIGHT;	/* advance to next nibble */
		}
	}

    switch( i )
    {
		case OSD_KEY_LEFT:
			if( --regs->nibble < 0 )
			{
				if( --regs->idx < 0 )
				{
					regs->idx = regs->count - 1;
				}
				regs->nibble = pedit[regs->idx].w - 1;
			}
			break;

		case OSD_KEY_RIGHT:
			if( ++regs->nibble >= pedit[regs->idx].w )
			{
				regs->nibble = 0;
				if( ++regs->idx >= regs->count )
				{
					regs->idx = 0;
                }
            }
            break;

		case OSD_KEY_UP:
			i = regs->idx;
			x = pedit[regs->idx].x;
			y = pedit[regs->idx].y;
			while( x != pedit[i].x || pedit[i].y == y )
			{
				if( --i < 0 )
                {
                    if( pedit[i].y == y )
                    {
                        i = regs->idx;
                        break;
                    }
					i = regs->count - 1;
                }
			}
			regs->idx = i;
            break;

		case OSD_KEY_DOWN:
			i = regs->idx;
			x = pedit[regs->idx].x;
			y = pedit[regs->idx].y;
			while( x != pedit[i].x || pedit[i].y == y )
			{
				if( ++i >= regs->count )
				{
					i = 0;
					if( pedit[i].y == y )
					{
						i = regs->idx;
						break;
					}
                }
			}
            regs->idx = i;
            break;

		case OSD_KEY_ENTER:
			DBG.window = EDIT_CMDS;
			break;

        default:
			cmd_default( i );
    }
}


/**************************************************************************
 * edit_dasm
 * Edit the disassembler output
 **************************************************************************/
static void edit_dasm(void)
{
	UINT32 win = WIN_DASM(activecpu);
	int h = win_get_h( win );
    const char *k;
    int i;

	win_set_title( win, name_rdmem(DASM.pc_cur) );
    ScreenSetCursor( win_get_cy_abs(win), win_get_cx_abs(win) );

    i = osd_debug_readkey();
	k = osd_key_name(i);

    switch( i )
    {
		case OSD_KEY_M: /* Toggle mode (opcode display) */
			dasm_opcodes( DASM.opcodes ^ 1 );
			break;

		case OSD_KEY_ENTER:
			DBG.window = EDIT_CMDS;
			break;

        default:
			cmd_default( i );
    }
}


/**************************************************************************
 * edit_mem
 * Edit the memory dumps output
 **************************************************************************/
static void edit_mem( int which )
{
	UINT32 win = WIN_MEM(activecpu,which);
	s_mem *mem = &MEM[which];
	s_edit *pedit = mem->edit;
    const char *k;
	unsigned shift, mask, val;
	int i, update_window = 0;

    win_set_curpos( win, pedit[mem->offset].x + mem->nibble, pedit[mem->offset].y );
    ScreenSetCursor( win_get_cy_abs(win), win_get_cx_abs(win) );

    switch( mem->mode )
	{
		case MODE_HEX_UINT8:
			mem->address = (mem->base + mem->offset) & AMASK;
			break;
		case MODE_HEX_UINT16:
			mem->address = (mem->base + (mem->offset & ~1) + pedit[mem->offset].n ) & AMASK;
            break;
		case MODE_HEX_UINT32:
			mem->address = (mem->base + (mem->offset & ~3) + pedit[mem->offset].n ) & AMASK;
            break;
	}
	win_set_title( win, name_memory( mem->address ) );

    i = osd_debug_readkey();
	k = osd_key_name(i);

	shift = (pedit[mem->offset].w - 1 - mem->nibble) * 4;
	mask = ~(0x0f << shift);

	if( strlen(k) == 1 )
	{
		switch( k[0] )
		{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9': case 'A': case 'B':
			case 'C': case 'D': case 'E': case 'F':
				val = k[0] - '0';
				if( val > 9 ) val -= 7;
				val <<= shift;
				/* now modify the register */
				WRMEM( mem->address, ( RDMEM( mem->address ) & mask ) | val );
				update_window = 1;
				i = OSD_KEY_RIGHT;	/* advance to next nibble */
		}
	}

    switch( i )
    {
        case OSD_KEY_LEFT:
			if( --mem->nibble < 0 )
			{
				if( --mem->offset < 0 )
				{
					mem->base = (mem->base - mem->bytes) & AMASK;
					mem->offset += mem->bytes;
					update_window = 1;
				}
				mem->nibble = pedit[mem->offset].w - 1;
			}
			break;

		case OSD_KEY_RIGHT:
			if( ++mem->nibble >= pedit[mem->offset].w )
			{
				mem->nibble = 0;
				if( ++mem->offset >= mem->size )
				{
					mem->base = (mem->base + mem->bytes) & AMASK;
					mem->offset -= mem->bytes;
					update_window = 1;
                }
            }
            break;

		case OSD_KEY_UP:
			mem->offset -= mem->bytes;
			if( mem->offset < 0 )
			{
				mem->base = (mem->base - mem->bytes) & AMASK;
				mem->offset += mem->bytes;
				update_window = 1;
            }
			break;

		case OSD_KEY_DOWN:
			mem->offset += mem->bytes;
			if( mem->offset >= mem->size )
            {
				mem->base = (mem->base + mem->bytes) & AMASK;
				mem->offset -= mem->bytes;
				update_window = 1;
            }
            break;

		case OSD_KEY_PGUP:
			mem->base = (mem->base - mem->size) & AMASK;
			update_window = 1;
            break;

		case OSD_KEY_PGDN:
			mem->base = (mem->base + mem->size) & AMASK;
            update_window = 1;
            break;

		case OSD_KEY_HOME:
			mem->offset = 0;
			mem->base = 0x00000000;
            update_window = 1;
            break;

		case OSD_KEY_END:
			mem->offset = mem->size - 1;
			mem->base = (0xffffffff - mem->offset) & AMASK;
            update_window = 1;
            break;

		case OSD_KEY_M:
			mem->mode = ++(mem->mode) % 3;
			/* Reset cursor coordinates and sizes of the edit info */
            memset( mem->edit, 0, sizeof(mem->edit) );
			update_window = 1;
			break;

		case OSD_KEY_ENTER:
			DBG.window = EDIT_CMDS;
			break;

        default:
			cmd_default( i );
    }

	if( update_window )
	{
		memcpy( mem->backup, mem->newval, mem->size );
		dump_mem( which, 0 );
	}
}

/**************************************************************************
 * edit_cmds_info
 * Search the cmdline for the beginning of a known command and
 * display some information in the caption of the command line input
 **************************************************************************/
static void edit_cmds_info( void )
{
    char *cmdline = DBG.cmdline;
    int i, j;

    if( strlen(cmdline) )
    {
        for( i = 0; commands[i].name; i++ )
        {
            j = strlen(cmdline);
            if( strlen(commands[i].name) < j )
                j = strlen(commands[i].name);
            if( strncmp( cmdline, commands[i].name, j ) == 0 )
            {
				win_set_title( WIN_CMDS(activecpu), "Command: %s %s", commands[i].name, commands[i].info );
                return;
            }
        }
    }
    win_set_title( WIN_CMDS(activecpu), "Command" );
}

/**************************************************************************
 * edit_cmds_parse
 * Search the cmdline for a known command and if found,
 * strip it from cmdline and return it's index
 **************************************************************************/
static int edit_cmds_parse( char *cmd )
{
    int i, l;

    for( i = 0; commands[i].valid; i++ )
    {
        if( !commands[i].name )
            continue;
        l = strlen( commands[i].name );
        if( strncmp( cmd, commands[i].name, l) == 0 && !isalnum( cmd[l] ) )
        {
            while( cmd[l] && isspace( cmd[l] ) ) l++;
            strcpy( cmd, cmd + l );
            return i;
        }
    }
    return 0;
}


/**************************************************************************
 * edit_cmds_append
 * Append a character (string) to the command line
 **************************************************************************/
static void edit_cmds_append( const char *src )
{
	char *cmdline = DBG.cmdline;
	UINT32 win = WIN_CMDS(activecpu);
    if( strlen(cmdline) < 80 )
	{
		strcat(cmdline, src);
		win_printf( win, "%s", src );
	}
}

void edit_cmds_reset( void )
{
	DBG.cmdline[0] = '\0';
	win_set_curpos( WIN_CMDS(activecpu), 0, 0 );
	win_erase_eol( WIN_CMDS(activecpu), ' ' );
}

/**************************************************************************
 * edit_cmds
 * Edit the command line input
 **************************************************************************/
static void edit_cmds(void)
{
	char *cmdline = DBG.cmdline;
	UINT32 win = WIN_CMDS(activecpu);
    int w = win_get_w(win);
	const char *k;
	int i, cmd;

	ScreenSetCursor( win_get_cy_abs(win), win_get_cx_abs(win) );

	edit_cmds_info();

	i = osd_debug_readkey();
	k = osd_key_name(i);

	if( strlen(k) == 1 )
		edit_cmds_append(k);

    switch( i )
    {
		case OSD_KEY_SPACE:  edit_cmds_append(" "); break;
		case OSD_KEY_EQUALS: edit_cmds_append("="); break;
		case OSD_KEY_STOP:	 edit_cmds_append("."); break;

        case OSD_KEY_BACKSPACE:
            if( strlen(cmdline) > 0 )
            {
                cmdline[strlen(cmdline)-1] = '\0';
				win_printf( win, "\b \b" );
            }
            break;

		case OSD_KEY_ENTER:
			if( strlen(cmdline) )
            {
				cmd = edit_cmds_parse( cmdline );
                if( commands[cmd].function )
					(*commands[cmd].function)();
				break;
            }
			else
			{
				/* ENTER in an empty line: do single step... */
                i = OSD_KEY_F8;
			}
			break;

        default:
			cmd_default( i );
    }
}

/**************************************************************************
 **************************************************************************
 *
 *		Command functions
 *
 **************************************************************************
 **************************************************************************/

/**************************************************************************
 * cmd_help
 * Display a help window containing a list of the (currently)
 * available commands and keystrokes.
 **************************************************************************/
static void cmd_help( void )
{
    UINT32 win = WIN_HELP;
    int i, h, k, l, top, lines;
    char *help = malloc(4096+1), *dst, *src;

    if( !help )
    {
        win_msgbox( COLOR_ERROR, "Memory problem!", "Couldn't allocate help text buffer" );
        return;
    }
    dst = help;
    switch( DBG.window )
    {
        case EDIT_CMDS:
            dst += sprintf( dst, " Welcome to the new MAME debugger!") + 1;
            dst += sprintf( dst, " ") + 1;
            dst += sprintf( dst, " Many commands accept either a value or a register name.") + 1;
            dst += sprintf( dst, " You can indeed type either  R HL = SP  or  R HL = 1fd0.") + 1;
            dst += sprintf( dst, " In the syntax, where you see <Address> you may generally") + 1;
            dst += sprintf( dst, " use a number or a register name.") + 1;
            dst += sprintf( dst, " ") + 1;
            break;
        case EDIT_REGS:
            dst += sprintf( dst, " %s [%s] Version %s", cpu_name(), cpu_core_family(), cpu_core_version() ) + 1;
            dst += sprintf( dst, " Address bits   : %d [%X]", cpu_address_bits(), cpu_address_mask() ) + 1;
            dst += sprintf( dst, " Code align unit: %d bytes", cpu_align_unit() ) + 1;
            dst += sprintf( dst, " This CPU is    : %s endian", (cpu_endianess() == CPU_IS_LE) ? "little" : "big" ) + 1;
            dst += sprintf( dst, " Source file    : %s", cpu_core_file() ) + 1;
            dst += sprintf( dst, " %s", cpu_core_credits() ) + 1;
            dst += sprintf( dst, " ") + 1;
            break;
        case EDIT_DASM:
            dst += sprintf( dst, " %s [%s]", cpu_name(), cpu_core_family() ) + 1;
            dst += sprintf( dst, " Press M to toggle the mode 'opcode display' on/off"  ) + 1;
            dst += sprintf( dst, " ") + 1;
            break;
        case EDIT_MEM1:
        case EDIT_MEM2:
            dst += sprintf( dst, " Press M to switch throught display modes"  ) + 1;
            break;
    }
    dst += sprintf( dst, " Valid commands are:" ) + 1;

    for( i = 0; commands[i].valid; i++ )
    {
        if( commands[i].valid & ( 1 << DBG.window ) )
        {
            if( commands[i].name )
                dst += sprintf( dst, " %s\t%s", commands[i].name, commands[i].info ) + 1;
            else
                dst += sprintf( dst, " [%s]\t%s", osd_key_name(commands[i].key), commands[i].info ) + 1;
        }
    }

    /* Terminate *help with a second NULL byte */
    *dst++ = '\0';

    /* Count lines */
    for( lines = 0, src = help; *src && i > 0; src += strlen(src) + 1 )
        lines++;

    win_create( win,
        4,2,68,16, COLOR_COMMANDINFO, COLOR_LINE, ' ',
        BORDER_TOP | BORDER_LEFT | BORDER_RIGHT | BORDER_BOTTOM | SHADOW );
    win_show( win );
    h = win_get_h( win );

    top = 0;
    win_set_title( win, "Debug command help" );
    do
    {
        for( src = help, i = top; *src && i > 0; src += strlen(src) + 1 )
            i--;
        win_set_curpos( win, 0, 0 );
        l = 0;
        do
        {
            if( *src )
            {
                win_printf( win, src );
                src += strlen(src) + 1;
            }
            win_putc( win, '\n');
            l++;
        } while( l < win_get_h(win) );

        k = osd_debug_readkey();
        switch( k )
        {
            case OSD_KEY_UP:
                if( top > 0 )
                    top--;
                break;
            case OSD_KEY_ENTER:
            case OSD_KEY_DOWN:
                if( top < lines - h )
                    top++;
                break;
            case OSD_KEY_PGUP:
                if( top - h > 0 )
                    top -= h;
                else
                    top = 0;
                break;
            case OSD_KEY_PGDN:
                if( top + h < lines - h )
                    top += h;
                else
                    top = lines - h;
                break;
        }
    } while( k != OSD_KEY_ESC );

    free( help );

    win_close( win );
}

/**************************************************************************
 * cmd_default
 * Handle a key stroke with no special meaning inside the active window
 **************************************************************************/
static void cmd_default( int code )
{
    int i;
    for( i = 0; commands[i].valid; i++ )
    {
        if( (commands[i].valid & (1<<DBG.window)) && commands[i].key == code )
        {
            if( commands[i].function )
                (*commands[i].function)();
            return;
        }
    }
}


/**************************************************************************
 * cmd_breakpoint_set
 * Set the breakpoint for the current CPU to the specified address
 **************************************************************************/
static void cmd_breakpoint_set( void )
{
	char *cmd = CMD;

	DBG.break_point = get_register_or_value( &cmd, NULL );
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_breakpoint_clear
 * Reset the breakpoint for the current CPU to INVALID
 **************************************************************************/
static void cmd_breakpoint_clear( void )
{
	DBG.break_point = INVALID;
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_watchpoint_set
 * Set the watchpoint for the current CPU to the specified address
 * The monitored data is one byte at the given address
 **************************************************************************/
static void cmd_watchpoint_set( void )
{
	char *cmd = CMD;
    unsigned data;

	DBG.watch_point = get_register_or_value( &cmd, NULL );
	data = RDMEM(DBG.watch_point);
	DBG.watch_value = data;

    edit_cmds_reset();
}

/**************************************************************************
 * cmd_watchpoint_clear
 * Reset the watchpoint for the current CPU to INVALID
 **************************************************************************/
static void cmd_watchpoint_clear( void )
{
	DBG.break_point = INVALID;
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_display_memory
 * Set one of the memory display window's start address
 **************************************************************************/
static void cmd_display_memory( void )
{
	char *cmd = CMD;
    unsigned which, address;
    int length;

	which = xtou( &cmd, &length );
	if( length )
	{
		address = get_register_or_value( &cmd, &length );
		if( length )
		{
			which = (which - 1) % MAX_MEM;
		}
		else
		{
			address = which;
			which = 0;
		}
		MEM[which].base = address;
		dump_mem( which, 1 );
    }
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_to_file
 * Disassemble a range of code and output it to a file
 **************************************************************************/
static void cmd_dasm_to_file( void )
{
}

/**************************************************************************
 * cmd_dump_to_file
 * (Hex-)Dump a range of code and output it to a file
 **************************************************************************/
static void cmd_dump_to_file( void )
{
}

/**************************************************************************
 * cmd_edit_memory
 * Set one of the memory display window's start address and
 * switch to that window
 **************************************************************************/
static void cmd_edit_memory( void )
{
    char *cmd = CMD;
    unsigned which, address;
    int length;

    which = xtou( &cmd, &length );
    if( length )
    {
		address = get_register_or_value( &cmd, &length );
        if( length )
        {
            which = (which - 1) % MAX_MEM;
        }
        else
        {
            address = which;
            which = 0;
        }
        MEM[which].base = address;
        dump_mem( which, 0 );
        switch( which )
        {
            case 0: DBG.window = EDIT_MEM1; break;
            case 1: DBG.window = EDIT_MEM2; break;
        }
    }
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_fast
 * Hmm... no idea ;)
 **************************************************************************/
static void cmd_fast( void )
{
	dbg_fast = 1;
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_go_break
 * Let the emulation run and optionally set a breakpoint
 **************************************************************************/
static void cmd_go_break( void )
{
	char *cmd = CMD;
	unsigned brk;
	int length;

	brk = get_register_or_value( &cmd, &length );
	if( length )
		DBG.break_temp = brk;

    do_update = 0;
	in_debug = 0;
	dbg_fast = 1;

    osd_sound_enable(1);
	osd_set_display(
		Machine->scrbitmap->width,
		Machine->scrbitmap->height,
		Machine->drv->video_attributes);
}

/**************************************************************************
 * cmd_here
 * Set a temporary breakpoint at the cursor PC and let the emulation run
 **************************************************************************/
static void cmd_here( void )
{
	DBG.break_temp = DASM.pc_cur;
	do_update = 0;
	in_debug = 0;
	dbg_fast = 1;

    osd_sound_enable(1);
	osd_set_display(Machine->scrbitmap->width,
					Machine->scrbitmap->height,
					Machine->drv->video_attributes);

    edit_cmds_reset();
}

/**************************************************************************
 * cmd_jump
 * Jump to the specified address in the disassembler window
 **************************************************************************/
static void cmd_jump( void )
{
	char *cmd = CMD;
	unsigned address;
	int length;

	address = get_register_or_value( &cmd, &length );
	if( length > 0 )
	{
		DASM.pc_top = address;
		DASM.pc_cur = DASM.pc_top;
		DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
	}
    edit_cmds_reset();
}

/**************************************************************************
 * cmd_replace_register
 * Either change a register to a specified value, change to the
 * registers window to edit a specified or (if none given) the
 * first register
 **************************************************************************/
static void cmd_replace_register( void )
{
    char *cmd = CMD;
	unsigned regnum, address;
    int length;

	regnum = get_register_id( &cmd, &length );
	if( regnum > 0 )
	{
		address = get_register_or_value( &cmd, &length );
		if( length )
		{
			cpu_set_reg( regnum, address );
			dump_regs();
		}
		else
		{
			/* Edit the first register */
			for( REGS.idx = 0; REGS.idx < REGS.count; REGS.idx++ )
				if( REGS.id[REGS.idx] == regnum ) break;
			DBG.window = EDIT_REGS;
        }
    }
	else
	{
		/* Edit the first register */
        REGS.idx = 0;
		DBG.window = EDIT_REGS;
    }
    edit_cmds_reset();
}

static void cmd_trace_to_file( void )
{
	edit_cmds_reset();
}

static void cmd_dasm_up( void )
{
	if ( (DASM.pc_cur >= dasm_line( DASM.pc_top, 1 ) ) &&
		 (DASM.pc_cur < DASM.pc_end) )
	{
		unsigned dasm_pc_1st = DASM.pc_top;
		unsigned dasm_pc_2nd = DASM.pc_top;
		while( dasm_pc_2nd < DASM.pc_end )
		{
			dasm_pc_2nd = dasm_line( dasm_pc_1st, 1 );

			if( dasm_pc_2nd == DASM.pc_cur )
			{
				DASM.pc_cur = dasm_pc_1st;
				dasm_pc_2nd = DASM.pc_end;
			}
			else
			{
				dasm_pc_1st = dasm_pc_2nd;
			}
		}
	}
	else
	{
		/*
		 * Try to find the previous instruction by searching
		 * from the longest instruction length towards the
		 * current address.  If we can't find one then
		 * just go back one byte, which means that a
		 * previous guess was wrong.
		 */
		unsigned dasm_pc_tmp = (DASM.pc_top - 2 * INSTL) & AMASK;
		if (dasm_pc_tmp > DASM.pc_top )
			dasm_pc_tmp = 0;
		while( dasm_pc_tmp < DASM.pc_top )
		{
			if( dasm_line( dasm_pc_tmp, 1 ) == DASM.pc_top )
				break;
			dasm_pc_tmp += ALIGN;
		}
		if( dasm_pc_tmp == DASM.pc_top )
			dasm_pc_tmp -= ALIGN;
		DASM.pc_cur = dasm_pc_tmp;
		if( DASM.pc_cur < DASM.pc_top )
			DASM.pc_top = DASM.pc_cur;
	}
	DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
	win_set_title( WIN_DASM(activecpu), name_rdmem(DASM.pc_cur) );
    edit_cmds_reset();
}

static void cmd_dasm_down( void )
{
	DASM.pc_cur = dasm_line( DASM.pc_cur, 1 );
	if( DASM.pc_cur >= DASM.pc_end )
		  DASM.pc_top = dasm_line( DASM.pc_top, 1 );
	DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
	win_set_title( WIN_DASM(activecpu), name_rdmem(DASM.pc_cur) );
    edit_cmds_reset();
}

static void cmd_dasm_page_up( void )
{
	UINT32 i;
    /*
     * This uses a 'rolling window' of start
     *  addresses to work out the best address to
     *  use to generate the previous pagefull of
     *  disassembly - CM 980428
     */
    if( DASM.pc_top > 0 )
    {
        unsigned dasm_pc_tmp[50];   /* needs to be > max windows height */
        unsigned h = win_get_h(WIN_DASM(activecpu));
        unsigned dasm_pc_1st = (DASM.pc_top - INSTL * h) & AMASK;

        if( dasm_pc_1st > DASM.pc_top )
            dasm_pc_1st = 0;

        for( i= 0; dasm_pc_1st < DASM.pc_top; i++ )
        {
            dasm_pc_tmp[i % h] = dasm_pc_1st;
			dasm_pc_1st = dasm_line( dasm_pc_1st, 1 );
        }

        /*
         * If this ever happens, it's because our
         * max_inst_len member is too small for the CPU
         */
        if( i < h )
        {
            dasm_pc_1st = dasm_pc_tmp[0];
            win_msgbox(COLOR_ERROR, "DASM page up", "Increase MaxInstLen? Line = %d\n", i);
        }
        else
        {
            DASM.pc_top = dasm_pc_tmp[(i+ 1) % h];
        }
    }
    DASM.pc_cur = DASM.pc_top;
    DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
    win_set_title( WIN_DASM(activecpu), name_rdmem(DASM.pc_cur) );
    edit_cmds_reset();
}

static void cmd_dasm_page_down( void )
{
    unsigned h = win_get_h(WIN_DASM(activecpu));

	DASM.pc_top = dasm_line( DASM.pc_top, h );
	DASM.pc_cur = dasm_line( DASM.pc_cur, h );
	DASM.pc_end = dump_dasm( DASM.pc_top, 0 );
	win_set_title( WIN_DASM(activecpu), name_rdmem(DASM.pc_cur) );
    edit_cmds_reset();
}

static void cmd_toggle_breakpoint( void )
{
	if( DBG.break_point == INVALID )
		DBG.break_point = DASM.pc_cur;
	else
	if( DBG.break_point == DASM.pc_cur )
		DBG.break_point = INVALID;
	else
	{
		win_msgbox( COLOR_PROMPT, "Breakpoint", "Cleared break point at %X", DBG.break_point );
		DBG.break_point = INVALID;
	}
}

static void cmd_toggle_watchpoint( void )
{
	int which = DBG.window == EDIT_MEM1 ? 0 : 1;

	if( DBG.watch_point == INVALID )
	{
		unsigned data;
		DBG.watch_point = MEM[which].address;
		data = RDMEM(DBG.watch_point);
		DBG.watch_value = data;
    }
	else
	if( DBG.watch_point == DASM.pc_cur )
	{
		DBG.watch_point = INVALID;
	}
	else
	{
		win_msgbox( COLOR_PROMPT, "Watchpoint", "Cleared watch point at %X", DBG.watch_point );
		DBG.watch_point = INVALID;
	}
}

static void cmd_run_to_cursor( void )
{
	DBG.break_temp = DASM.pc_cur;

    edit_cmds_reset();

    cmd_go();
}

static void cmd_view_screen( void )
{
	int i;

    osd_set_display(
		Machine->scrbitmap->width,
		Machine->scrbitmap->height,
        Machine->drv->video_attributes);

	/* Let memory changes eventually do something to the video */
	do
	{
		(*Machine->drv->vh_update)(Machine->scrbitmap,1);
		osd_update_video_and_audio();
	} while( !osd_key_pressed_memory(OSD_KEY_ANY) );

	set_gfx_mode (GFX_TEXT,80,25,0,0);
	win_invalidate_video();
	win_show( WIN_REGS(activecpu) );
	win_show( WIN_DASM(activecpu) );
	win_show( WIN_MEM1(activecpu) );
	win_show( WIN_MEM2(activecpu) );
	win_show( WIN_CMDS(activecpu) );
}

static void cmd_breakpoint_cpu( void )
{
    char *cmd = CMD;

    if( totalcpu > 1 )
    {
		DBG.break_cpunum = xtou( &cmd, NULL) % totalcpu;
        cmd_go();
    }

    edit_cmds_reset();
}

static void cmd_step( void )
{
	single_step = 1;
    edit_cmds_reset();
}

static void cmd_animate( void )
{
	dbg_trace = 1;
	single_step = 1;
    edit_cmds_reset();
}

static void cmd_step_over( void )
{
	/* Set temporary breakpoint on next instruction after cursor */
	DBG.break_temp = dasm_line( DASM.pc_cur, 1 );
	cmd_go();
}

static void cmd_switch_window( void )
{
	if( osd_key_pressed(OSD_KEY_LSHIFT) || osd_key_pressed(OSD_KEY_RSHIFT) )
		DBG.window = --DBG.window % DBG_WINDOWS;
	else
		DBG.window = ++DBG.window % DBG_WINDOWS;
}

static void cmd_go( void )
{
	debug_key_pressed = 0;
	do_update = 0;
	in_debug = 0;
	dbg_fast = 1;

    edit_cmds_reset();

    osd_sound_enable(1);
	osd_set_display(
		Machine->scrbitmap->width,
		Machine->scrbitmap->height,
		Machine->drv->video_attributes);
}

/**************************************************************************
 **************************************************************************
 *      MAME_Debug
 *      This function is called from within an execution loop of a
 *      CPU core whenever mame_debug is non zero
 **************************************************************************
 **************************************************************************/
void MAME_Debug(void)
{
    static int first_time = 1;

    if( ++debug_key_delay == 0x7fff )
    {
        debug_key_delay = 0;
        debug_key_pressed = osd_key_pressed (OSD_KEY_DEBUGGER);
    }

	if( dbg_fast )
    {
        if( !debug_key_pressed ) return;
		dbg_fast = 0;
    }

    activecpu = cpu_getactivecpu();
    cputype = Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK;

    if( trace_on )
    {
        trace_select();
        trace_output( cpu_get_pc() );
    }

    if( DBG.previous_sp )
    {
        /* assume we're in debug */
        in_debug = 1;
        /* See if we went into a function.
           A 'return' will cause the CPU's stack pointer to be
           greater than the previous stack pointer */
        if( cpu_get_pc() != DBG.next_pc &&
            cpu_get_sp() < DBG.previous_sp )
        {
			/* if so, set a breakpoint on the return PC */
			DBG.break_point = DBG.next_pc;
            do_update = 0;
            in_debug = 0;
            osd_sound_enable(1);
            osd_set_display(
                Machine->scrbitmap->width,
                Machine->scrbitmap->height,
                Machine->drv->video_attributes);
        }
        DBG.previous_sp = 0;
    }

    if( first_time )
    {
        int i;

        set_gfx_mode (GFX_TEXT,80,25,0,0);

        memset( &dbg, 0, sizeof(dbg) );
        for( i = 0; i < MAX_CPU; i++ )
        {
            dbg[i].window = EDIT_CMDS;
			dbg[i].break_point = INVALID;
			dbg[i].break_temp = INVALID;
			dbg[i].break_cpunum = INVALID;
			dbg[i].watch_point = INVALID;
            dbg[i].mem[0].base = MEM1DEFAULT;
            dbg[i].mem[0].color = COLOR_MEM1;
            dbg[i].mem[1].base = MEM2DEFAULT;
            dbg[i].mem[1].color = COLOR_MEM2;
            switch( ALIGN )
            {
                case 1: dbg[i].mem[0].mode = dbg[i].mem[1].mode = MODE_HEX_UINT8;  break;
                case 2: dbg[i].mem[0].mode = dbg[i].mem[1].mode = MODE_HEX_UINT16; break;
                case 4: dbg[i].mem[0].mode = dbg[i].mem[1].mode = MODE_HEX_UINT32; break;
            }
            dbg[i].regs.color = COLOR_REGS;
        }

        totalcpu = cpu_gettotalcpu();

        /* create windows for the active CPU */
        dbg_create_windows();

        debug_key_pressed = 1;
    }

    if ( (hit_break || hit_watch || debug_key_pressed) && !in_debug )
    {
        uclock_t curr = uclock();
        debug_key_pressed = 0;

		if( !first_time )
        {
            osd_sound_enable(0);
            do
            {
                osd_update_video_and_audio();   /* give time to the sound hardware to apply the volume change */
            } while( (uclock_t)(uclock() - curr) < (UCLOCKS_PER_SEC / 15) );
        }

        first_time = 0;

        set_gfx_mode (GFX_TEXT,80,25,0,0);
        win_invalidate_video();

		edit_cmds_reset();

		DBG.break_temp = INVALID;
		DBG.break_cpunum = INVALID;
        in_debug = 1;
        do_update = 1;
        dbg_trace = 0;
        DASM.pc_top = cpu_get_pc();
        DASM.pc_cur = DASM.pc_top;
    }

    if( single_step )
    {
        DASM.pc_cur = cpu_get_pc();
        single_step = 0;
    }

    /* Assume we need to update the windows */
    do_update = 1;

    while( in_debug && !single_step )
    {
        static uclock_t last_time = 0;

        if( dbg_trace )
        {
            uclock_t this_time = uclock();

            /*
             * If less than 25000 microseconds are gone,
             * do not update or check for dbg_trace stop
             */
            if( (this_time - last_time) < 25000 )
            {
                do_update = 0;
            }
            else
            {
                last_time = this_time;
                if( osd_key_pressed(OSD_KEY_SPACE) )
                    dbg_trace = 0;
            }

            single_step = 1;
        }

        if( do_update )
        {
			if( activecpu != previous_activecpu )
			{
				if( previous_activecpu != INVALID )
				{
					win_hide( WIN_REGS(previous_activecpu) );
					win_hide( WIN_DASM(previous_activecpu) );
					win_hide( WIN_MEM1(previous_activecpu) );
					win_hide( WIN_MEM2(previous_activecpu) );
					win_hide( WIN_CMDS(previous_activecpu) );
				}
				win_show( WIN_REGS(activecpu) );
				win_show( WIN_DASM(activecpu) );
				win_show( WIN_MEM1(activecpu) );
				win_show( WIN_MEM2(activecpu) );
				win_show( WIN_CMDS(activecpu) );
			}

            dump_regs();
            dump_mem( 0, DBG.window != EDIT_MEM1 );
            dump_mem( 1, DBG.window != EDIT_MEM2 );
            DASM.pc_cpu = cpu_get_pc();
            /* Check if pc_cpu is outside of our disassembly window */
            if( DASM.pc_cpu < DASM.pc_top || DASM.pc_cpu >= DASM.pc_end )
                DASM.pc_top = DASM.pc_cpu;
            DASM.pc_end = dump_dasm( DASM.pc_top, 1 );
            do_update = 0;
        }

        if( !dbg_trace )
        {
            switch( DBG.window )
            {
                case EDIT_REGS: edit_regs(); break;
                case EDIT_DASM: edit_dasm(); break;
                case EDIT_MEM1: edit_mem(0); break;
                case EDIT_MEM2: edit_mem(1); break;
                case EDIT_CMDS: edit_cmds(); break;
            }
        }
    }

    /* update backup copies of memory and registers */
    if( MEM[0].changed )
    {
        MEM[0].changed = 0;
        memcpy( MEM[0].backup,
                MEM[0].newval, MEM[0].size );
    }
    if( MEM[1].changed )
    {
        MEM[1].changed = 0;
        memcpy( MEM[1].backup,
                MEM[1].newval, MEM[0].size );
    }
    if( REGS.changed )
    {
        REGS.changed = 0;
        memcpy( REGS.backup,
                REGS.newval, REGS.count * sizeof(UINT32) );
    }
}


