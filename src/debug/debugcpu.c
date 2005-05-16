/*###################################################################################################
**
**
**      debugcpu.c
**      Debugger CPU/memory interface engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#include "driver.h"
#include "debugcpu.h"
#include "debugcmd.h"
#include "debugcon.h"
#include "debugexp.h"
#include "debugvw.h"
#include <ctype.h>



/*###################################################################################################
**  DEBUGGING
**#################################################################################################*/

#define DEBUG			0



/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

#define NUM_TEMP_VARIABLES	10



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/



/*###################################################################################################
**  LOCAL VARIABLES
**#################################################################################################*/

/* fixme: make this go away */
int debug_key_pressed;
/* fixme: end */

FILE *debug_source_file;

static UINT64 wpdata;
static UINT64 wpaddr;

static int execution_state;
static UINT32 execution_counter;
static int next_index = 1;
static int within_debugger_code = 0;
static int last_cpunum;
static int last_stopped_cpunum;
static int steps_until_stop;
static offs_t step_overout_breakpoint;
static int step_overout_cpunum;
static int key_check_counter;
static cycles_t last_periodic_update_time;
static int break_on_vblank;
static int break_on_interrupt;
static int break_on_interrupt_cpunum;
static int break_on_interrupt_irqline;

static struct debug_cpu_info debug_cpuinfo[MAX_CPU];

static UINT64 tempvar[NUM_TEMP_VARIABLES];



/*###################################################################################################
**  PROTOTYPES
**#################################################################################################*/

static void perform_trace(struct debug_cpu_info *info);
static void prepare_for_step_overout(void);
static UINT64 get_wpaddr(UINT32 ref);
static UINT64 get_wpdata(UINT32 ref);
static UINT64 get_cycles(UINT32 ref);
static UINT64 get_cpunum(UINT32 ref);
static UINT64 get_tempvar(UINT32 ref);
static void set_tempvar(UINT32 ref, UINT64 value);
static UINT64 get_cpu_reg(UINT32 ref);
static void set_cpu_reg(UINT32 ref, UINT64 value);



/*###################################################################################################
**  FRONTENDS FOR OLDER FUNCTIONS
**#################################################################################################*/

/*-------------------------------------------------
    mame_debug_init - start up all subsections
-------------------------------------------------*/

void mame_debug_init(void)
{
	/* initialize the various subsections */
	debug_cpu_init();
	debug_command_init();
	debug_console_init();
	debug_view_init();
}


/*-------------------------------------------------
    mame_debug_exit - shutdown all subsections
-------------------------------------------------*/

void mame_debug_exit(void)
{
	debug_view_exit();
	debug_console_exit();
	debug_command_exit();
	debug_cpu_exit();
}


/*-------------------------------------------------
    set_ea_info - hacky substitute for the old
    version in mamedbg.c
-------------------------------------------------*/

#ifdef MAME_DEBUG
const char *set_ea_info(int what, unsigned value, int size, int access)
{
	static char buffer[8][63+1];
	static int which = 0;
	const char *sign = "";
	unsigned width, result;

	which = (which + 1) % 8;

	if( access == EA_REL_PC )
		/* PC relative calls set_ea_info with value = PC and size = offset */
		result = value + size;
	else
		result = value;

	switch( access )
	{
		case EA_VALUE:	/* Immediate value */
			switch( size )
			{
				case EA_INT8:
				case EA_UINT8:
					width = 2;
					break;
				case EA_INT16:
				case EA_UINT16:
					width = 4;
					break;
				case EA_INT32:
				case EA_UINT32:
					width = 8;
					break;
				default:
					return "set_ea_info: invalid <size>!";
			}

			switch( size )
			{
				case EA_INT8:
				case EA_INT16:
				case EA_INT32:
					if( result & (1 << ((width * 4) - 1)) )
					{
						sign = "-";
						result = (unsigned)-result;
					}
					break;
			}

			if (width < 8)
				result &= (1 << (width * 4)) - 1;
			break;

		case EA_ZPG_RD:
		case EA_ZPG_WR:
		case EA_ZPG_RDWR:
			result &= 0xff;
			width = 2;
			break;

		case EA_ABS_PC: /* Absolute program counter change */
			result &= (address_space[ADDRESS_SPACE_PROGRAM].addrmask | 3);
			if( size == EA_INT8 || size == EA_UINT8 )
				width = 2;
			else
			if( size == EA_INT16 || size == EA_UINT16 )
				width = 4;
			else
			if( size == EA_INT32 || size == EA_UINT32 )
				width = 8;
			else
				width = (activecpu_addrbus_width(ADDRESS_SPACE_PROGRAM) + 3) / 4;
			break;

		case EA_REL_PC: /* Relative program counter change */
		default:
			result &= (address_space[ADDRESS_SPACE_PROGRAM].addrmask | 3);
			width = (activecpu_addrbus_width(ADDRESS_SPACE_PROGRAM) + 3) / 4;
	}
	sprintf( buffer[which], "%s$%0*X", sign, width, result );
	return buffer[which];
}
#endif



/*###################################################################################################
**  INITIALIZATION
**#################################################################################################*/

/*-------------------------------------------------
    debug_cpu_init - initialize the CPU
    information for debugging
-------------------------------------------------*/

void debug_cpu_init(void)
{
	int cpunum, spacenum, regnum;

	/* reset globals */
	execution_state = EXECUTION_STATE_STOPPED;
	execution_counter = 0;
	next_index = 1;
	within_debugger_code = 0;
	last_cpunum = 0;
	last_stopped_cpunum = 0;
	steps_until_stop = 0;
	step_overout_breakpoint = ~0;
	step_overout_cpunum = 0;
	key_check_counter = 0;

	/* add "wpaddr", "wpdata", "cycles", "cpunum" to the global symbol table */
	debug_symtable_add_register(GLOBAL_SYMBOL_TABLE, "wpaddr", 0, get_wpaddr, NULL);
	debug_symtable_add_register(GLOBAL_SYMBOL_TABLE, "wpdata", 0, get_wpdata, NULL);
	debug_symtable_add_register(GLOBAL_SYMBOL_TABLE, "cycles", 0, get_cycles, NULL);
	debug_symtable_add_register(GLOBAL_SYMBOL_TABLE, "cpunum", 0, get_cpunum, NULL);

	/* add the temporary variables to the global symbol table */
	for (regnum = 0; regnum < NUM_TEMP_VARIABLES; regnum++)
	{
		char symname[10];
		sprintf(symname, "temp%d", regnum);
		debug_symtable_add_register(GLOBAL_SYMBOL_TABLE, symname, regnum, get_tempvar, set_tempvar);
	}

	/* reset the CPU info */
	memset(debug_cpuinfo, 0, sizeof(debug_cpuinfo));

	/* loop over CPUs and build up their info */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type;

		/* if this is a dummy, stop looking */
		if (cputype == CPU_DUMMY)
			break;

		/* reset the PC data */
		debug_cpuinfo[cpunum].valid = 1;
		debug_cpuinfo[cpunum].endianness = cpunum_endianness(cpunum);
		debug_cpuinfo[cpunum].opwidth = cpunum_min_instruction_bytes(cpunum);
		debug_cpuinfo[cpunum].ignoring = 0;
		debug_cpuinfo[cpunum].temp_breakpoint_pc = ~0;

		/* fetch the memory accessors */
		debug_cpuinfo[cpunum].read = (int (*)(int, UINT32, int, UINT64 *))cpunum_get_info_fct(cpunum, CPUINFO_PTR_READ);
		debug_cpuinfo[cpunum].write = (int (*)(int, UINT32, int, UINT64))cpunum_get_info_fct(cpunum, CPUINFO_PTR_WRITE);
		debug_cpuinfo[cpunum].readop = (int (*)(UINT32, int, UINT64 *))cpunum_get_info_fct(cpunum, CPUINFO_PTR_READOP);

		/* allocate a symbol table */
		debug_cpuinfo[cpunum].symtable = debug_symtable_alloc();

		/* add all registers into it */
		for (regnum = 0; regnum < MAX_REGS; regnum++)
		{
			const char *str = cpunum_reg_string(cpunum, regnum);
			const char *colon;
			char symname[256];
			int charnum;

			/* skip if we don't get a valid string, or one without a colon */
			if (str == NULL)
				continue;
			colon = strchr(str, ':');
			if (colon == NULL)
				continue;

			/* strip all spaces from the name and convert to lowercase */
			for (charnum = 0; charnum < sizeof(symname) - 1 && str < colon; str++)
				if (!isspace(*str))
					symname[charnum++] = tolower(*str);
			symname[charnum] = 0;

			/* add the symbol to the table */
			debug_symtable_add_register(debug_cpuinfo[cpunum].symtable, symname, regnum, get_cpu_reg, set_cpu_reg);
		}

		/* loop over address spaces and get info */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
			int datawidth = cpunum_databus_width(cpunum, spacenum);
			int addrwidth = cpunum_addrbus_width(cpunum, spacenum);
			int addrshift = cpunum_addrbus_shift(cpunum, spacenum);

			debug_cpuinfo[cpunum].space[spacenum].databytes = datawidth / 8;
			debug_cpuinfo[cpunum].space[spacenum].addr2byte_lshift = (addrshift < 0) ? -addrshift : 0;
			debug_cpuinfo[cpunum].space[spacenum].addr2byte_rshift = (addrshift > 0) ?  addrshift : 0;
			debug_cpuinfo[cpunum].space[spacenum].addrchars = (addrwidth + 3) / 4;
			debug_cpuinfo[cpunum].space[spacenum].addrmask = (0xfffffffful >> (32 - addrwidth));
		}
	}
}


/*-------------------------------------------------
    debug_cpu_exit - free all memory
-------------------------------------------------*/

void debug_cpu_exit(void)
{
	int cpunum, spacenum;

	/* loop over all watchpoints and breakpoints to free their memory */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		/* close any tracefiles */
		if (debug_cpuinfo[cpunum].trace.file)
			fclose(debug_cpuinfo[cpunum].trace.file);
		if (debug_cpuinfo[cpunum].trace.action)
			free(debug_cpuinfo[cpunum].trace.action);

		/* free the symbol table */
		if (debug_cpuinfo[cpunum].symtable)
			debug_symtable_free(debug_cpuinfo[cpunum].symtable);

		/* free all breakpoints */
		while (debug_cpuinfo[cpunum].first_bp)
			debug_breakpoint_clear(debug_cpuinfo[cpunum].first_bp->index);

		/* loop over all address spaces */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
			/* free all watchpoints */
			while (debug_cpuinfo[cpunum].space[spacenum].first_wp)
				debug_watchpoint_clear(debug_cpuinfo[cpunum].space[spacenum].first_wp->index);
		}
	}
}



/*###################################################################################################
**  EXECUTION CONTROL
**#################################################################################################*/

/*-------------------------------------------------
    debug_cpu_single_step - single step past the
    requested number of instructions
-------------------------------------------------*/

void debug_cpu_single_step(int numsteps)
{
	if (!within_debugger_code)
		return;
	steps_until_stop = numsteps;
	execution_state = EXECUTION_STATE_STEP_INTO;
}


/*-------------------------------------------------
    debug_cpu_single_step_over - single step over
    a single instruction
-------------------------------------------------*/

void debug_cpu_single_step_over(int numsteps)
{
	if (!within_debugger_code)
		return;
	steps_until_stop = numsteps;
	step_overout_cpunum = cpu_getactivecpu();
	execution_state = EXECUTION_STATE_STEP_OVER;
}


/*-------------------------------------------------
    debug_cpu_single_step_out - single step out of
    the current function
-------------------------------------------------*/

void debug_cpu_single_step_out(void)
{
	if (!within_debugger_code)
		return;
	steps_until_stop = 100;
	step_overout_cpunum = cpu_getactivecpu();
	execution_state = EXECUTION_STATE_STEP_OUT;
}


/*-------------------------------------------------
    debug_cpu_go - resume execution
-------------------------------------------------*/

void debug_cpu_go(offs_t targetpc)
{
	if (!within_debugger_code)
		return;
	execution_state = EXECUTION_STATE_RUNNING;
	debug_cpuinfo[cpu_getactivecpu()].temp_breakpoint_pc = targetpc;
}


/*-------------------------------------------------
    debug_cpu_go_vblank - run until the next
    VBLANK
-------------------------------------------------*/

void debug_cpu_go_vblank(void)
{
	if (!within_debugger_code)
		return;
	execution_state = EXECUTION_STATE_RUNNING;
	debug_cpuinfo[cpu_getactivecpu()].temp_breakpoint_pc = ~0;
	break_on_vblank = 1;
}


/*-------------------------------------------------
    debug_cpu_go_interrupt - run until the
    specified interrupt fires
-------------------------------------------------*/

void debug_cpu_go_interrupt(int irqline)
{
	if (!within_debugger_code)
		return;
	execution_state = EXECUTION_STATE_RUNNING;
	debug_cpuinfo[cpu_getactivecpu()].temp_breakpoint_pc = ~0;
	break_on_interrupt = 1;
	break_on_interrupt_cpunum = cpu_getactivecpu();
	break_on_interrupt_irqline = irqline;
}


/*-------------------------------------------------
    debug_cpu_next_cpu - execute until we hit
    the next CPU
-------------------------------------------------*/

void debug_cpu_next_cpu(void)
{
	if (!within_debugger_code)
		return;
	execution_state = EXECUTION_STATE_NEXT_CPU;
}


/*-------------------------------------------------
    debug_cpu_ignore_cpu - ignore/observe a given
    CPU
-------------------------------------------------*/

void debug_cpu_ignore_cpu(int cpunum, int ignore)
{
	debug_cpuinfo[cpunum].ignoring = ignore;
	if (!within_debugger_code)
		return;
	if (cpunum == cpu_getactivecpu() && debug_cpuinfo[cpunum].ignoring)
		execution_state = EXECUTION_STATE_NEXT_CPU;
}


/*-------------------------------------------------
    debug_cpu_trace - trace execution of a given
    CPU
-------------------------------------------------*/

void debug_cpu_trace(int cpunum, FILE *file, int trace_over, const char *action)
{
	/* close existing files and delete expressions */
	if (debug_cpuinfo[cpunum].trace.file)
		fclose(debug_cpuinfo[cpunum].trace.file);
	debug_cpuinfo[cpunum].trace.file = NULL;

	if (debug_cpuinfo[cpunum].trace.action)
		free(debug_cpuinfo[cpunum].trace.action);
	debug_cpuinfo[cpunum].trace.action = NULL;

	/* open any new files */
	debug_cpuinfo[cpunum].trace.file = file;
	debug_cpuinfo[cpunum].trace.action = NULL;
	if (action)
	{
		debug_cpuinfo[cpunum].trace.action = malloc(strlen(action) + 1);
		if (debug_cpuinfo[cpunum].trace.action)
			strcpy(debug_cpuinfo[cpunum].trace.action, action);
	}

	/* specify trace over */
	debug_cpuinfo[cpunum].trace.trace_over_target = trace_over ? ~0 : 0;
}



/*###################################################################################################
**  UTILITIES
**#################################################################################################*/

/*-------------------------------------------------
    debug_get_cpu_info - returns the cpu info
    block for a given CPU
-------------------------------------------------*/

const struct debug_cpu_info *debug_get_cpu_info(int cpunum)
{
	return &debug_cpuinfo[cpunum];
}


/*-------------------------------------------------
    debug_halt_on_next_instruction - halt in
    the debugger on the next instruction
-------------------------------------------------*/

void debug_halt_on_next_instruction(void)
{
	execution_state = EXECUTION_STATE_STOPPED;
}


/*-------------------------------------------------
    debug_refresh_display - redraw the current
    video display
-------------------------------------------------*/

void debug_refresh_display(void)
{
	reset_partial_updates();
	draw_screen();	/* so we can change stuff in RAM and see the effect on screen */
	update_video_and_audio();
}


/*-------------------------------------------------
    debug_get_execution_state - return the
    current execution state
-------------------------------------------------*/

int debug_get_execution_state(void)
{
	return execution_state;
}


/*-------------------------------------------------
    debug_get_execution_counter - return the
    current execution counter
-------------------------------------------------*/

UINT32 debug_get_execution_counter(void)
{
	return execution_counter;
}


/*-------------------------------------------------
    get_wpaddr - getter callback for the
    'wpaddr' symbol
-------------------------------------------------*/

static UINT64 get_wpaddr(UINT32 ref)
{
	return wpaddr;
}


/*-------------------------------------------------
    get_wpdata - getter callback for the
    'wpdata' symbol
-------------------------------------------------*/

static UINT64 get_wpdata(UINT32 ref)
{
	return wpdata;
}


/*-------------------------------------------------
    get_cycles - getter callback for the
    'cycles' symbol
-------------------------------------------------*/

static UINT64 get_cycles(UINT32 ref)
{
	return activecpu_get_icount();
}


/*-------------------------------------------------
    get_cpunum - getter callback for the
    'cpunum' symbol
-------------------------------------------------*/

static UINT64 get_cpunum(UINT32 ref)
{
	return cpu_getactivecpu();
}


/*-------------------------------------------------
    get_tempvar - getter callback for the
    'tempX' symbols
-------------------------------------------------*/

static UINT64 get_tempvar(UINT32 ref)
{
	return tempvar[ref];
}


/*-------------------------------------------------
    set_tempvar - setter callback for the
    'tempX' symbols
-------------------------------------------------*/

static void set_tempvar(UINT32 ref, UINT64 value)
{
	tempvar[ref] = value;
}


/*-------------------------------------------------
    get_cpu_reg - getter callback for a CPU's
    register symbols
-------------------------------------------------*/

static UINT64 get_cpu_reg(UINT32 ref)
{
	return activecpu_get_reg(ref);
}


/*-------------------------------------------------
    set_cpu_reg - setter callback for a CPU's
    register symbols
-------------------------------------------------*/

static void set_cpu_reg(UINT32 ref, UINT64 value)
{
	activecpu_set_reg(ref, value);
}



/*###################################################################################################
**  MAIN CPU CALLBACK
**#################################################################################################*/

/*-------------------------------------------------
    MAME_Debug - called by the CPU cores before
    executing any instruction
-------------------------------------------------*/

void MAME_Debug(void)
{
	int cpunum = cpu_getactivecpu();
	struct debug_cpu_info *info = &debug_cpuinfo[cpunum];

	/* quick out if we are ignoring */
	if (info->ignoring)
		return;

	/* note that we are in the debugger code */
	within_debugger_code = 1;

	/* bump the counter */
	execution_counter++;

	/* are we tracing? */
	if (info->trace.file)
		perform_trace(info);

	/* check for execution breakpoints */
	if (execution_state != EXECUTION_STATE_STOPPED)
	{
		/* see if we hit an interrupt break */
		if (break_on_interrupt == 2 && break_on_interrupt_cpunum == cpunum)
		{
			debug_console_printf("Stopped on interrupt (CPU %d, IRQ %d)\n", break_on_interrupt_cpunum, break_on_interrupt_irqline);
			break_on_interrupt = 0;
			execution_state = EXECUTION_STATE_STOPPED;
		}

		/* see if the CPU changed and break if we are waiting for that to happen */
		if (cpunum != last_cpunum)
		{
			if (execution_state == EXECUTION_STATE_NEXT_CPU)
				execution_state = EXECUTION_STATE_STOPPED;
			last_cpunum = cpunum;
		}

		/* check the temp running breakpoint and break if we hit it */
		if (info->temp_breakpoint_pc != ~0 && execution_state == EXECUTION_STATE_RUNNING && activecpu_get_pc() == info->temp_breakpoint_pc)
		{
			execution_state = EXECUTION_STATE_STOPPED;
			debug_console_printf("Stopped at temporary breakpoint %X on CPU %d\n", info->temp_breakpoint_pc, cpunum);
			info->temp_breakpoint_pc = ~0;
		}

		/* check for execution breakpoints */
		if (info->first_bp)
			debug_check_breakpoints(cpunum, activecpu_get_pc());

		/* handle single stepping */
		if (steps_until_stop > 0 && (execution_state >= EXECUTION_STATE_STEP_INTO && execution_state <= EXECUTION_STATE_STEP_OUT))
		{
			/* is this an actual step? */
			if (step_overout_breakpoint == ~0 || (cpunum == step_overout_cpunum && activecpu_get_pc() == step_overout_breakpoint))
			{
				/* decrement the count and reset the breakpoint */
				steps_until_stop--;
				step_overout_breakpoint = ~0;

				/* if we hit 0, stop; otherwise, we might want to update everything */
				if (steps_until_stop == 0)
					execution_state = EXECUTION_STATE_STOPPED;
				else if (execution_state != EXECUTION_STATE_STEP_OUT)
				{
					debug_view_update_all();
					debug_refresh_display();
				}
			}
		}

		/* check for debug keypresses */
		if (execution_state != EXECUTION_STATE_STOPPED && ++key_check_counter > 10000)
		{
			key_check_counter = 0;
			if (input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
			{
				execution_state = EXECUTION_STATE_STOPPED;
				debug_console_printf("User-initiated break\n");
			}

			/* while we're here, check for a periodic update */
			if (cpunum == last_stopped_cpunum && execution_state != EXECUTION_STATE_STOPPED && osd_cycles() > last_periodic_update_time + osd_cycles_per_second()/4)
			{
				debug_view_update_all();
				last_periodic_update_time = osd_cycles();
			}
		}
	}

	/* if we are supposed to halt, do it now */
	if (execution_state == EXECUTION_STATE_STOPPED)
	{
		/* reset the state */
		steps_until_stop = 0;
		step_overout_breakpoint = ~0;

		/* update all views */
		debug_view_update_all();
		debug_refresh_display();

		/* wait for the debugger; during this time, disable sound output */
		osd_sound_enable(0);
		while (execution_state == EXECUTION_STATE_STOPPED)
		{
			osd_wait_for_debugger();

			/* check for commands in the source file */
			while (debug_source_file && (execution_state == EXECUTION_STATE_STOPPED))
			{
				char buf[512];
				int i;

				if (feof(debug_source_file))
				{
					fclose(debug_source_file);
					debug_source_file = NULL;
				}
				else
				{
					memset(buf, 0, sizeof(buf));
					fgets(buf, sizeof(buf), debug_source_file);
					i = strlen(buf);
					while((i > 0) && (isspace(buf[i-1])))
						buf[--i] = '\0';
					if (buf[0])
						debug_console_execute_command(buf, 1);
				}
			}
		}
		osd_sound_enable(1);

		/* remember the last cpunum where we stopped */
		last_stopped_cpunum = cpunum;
	}

	/* handle step out/over on the instruction we are about to execute */
	if ((execution_state == EXECUTION_STATE_STEP_OVER || execution_state == EXECUTION_STATE_STEP_OUT) && cpunum == step_overout_cpunum && step_overout_breakpoint == ~0)
		prepare_for_step_overout();

	/* no longer in debugger code */
	within_debugger_code = 0;
}


/*-------------------------------------------------
    perform_trace - log to the tracefile the
    data for a given instruction
-------------------------------------------------*/

static void perform_trace(struct debug_cpu_info *info)
{
	offs_t pc = activecpu_get_pc();
	int offset, count, i;
	char buffer[100];
	offs_t dasmresult;

	/* are we in trace over mode and in a subroutine? */
	if (info->trace.trace_over_target && (info->trace.trace_over_target != ~0))
	{
		if (info->trace.trace_over_target != pc)
			return;
		info->trace.trace_over_target = ~0;
	}

	/* check for a loop condition */
	for (i = count = 0; i < TRACE_LOOPS; i++)
		if (info->trace.history[i] == pc)
			count++;

	/* if no more than 1 hit, process normally */
	if (count <= 1)
	{
		/* if we just finished looping, indicate as much */
		if (info->trace.loops)
			fprintf(info->trace.file, "\n   (loops for %d instructions)\n\n", info->trace.loops);
		info->trace.loops = 0;

		/* execute any trace actions first */
		if (info->trace.action)
			debug_console_execute_command(info->trace.action, 0);

		/* print the address */
		offset = sprintf(buffer, "%0*X: ", info->space[ADDRESS_SPACE_PROGRAM].addrchars, pc);

		/* print the disassembly */
		dasmresult = activecpu_dasm(&buffer[offset], pc);

		/* output the result */
		fprintf(info->trace.file, "%s\n", buffer);

		/* do we need to step the trace over this instruction? */
		if (info->trace.trace_over_target && (dasmresult & DASMFLAG_SUPPORTED)
			&& (dasmresult & DASMFLAG_STEP_OVER))
		{
			int extraskip = (dasmresult & DASMFLAG_OVERINSTMASK) >> DASMFLAG_OVERINSTSHIFT;
			offs_t trace_over_target = pc + (dasmresult & DASMFLAG_LENGTHMASK);

			/* if we need to skip additional instructions, advance as requested */
			while (extraskip-- > 0)
				trace_over_target += activecpu_dasm(buffer, trace_over_target) & DASMFLAG_LENGTHMASK;

			info->trace.trace_over_target = trace_over_target;
		}

		/* log this PC */
		info->trace.nextdex = (info->trace.nextdex + 1) % TRACE_LOOPS;
		info->trace.history[info->trace.nextdex] = pc;
	}

	/* else just count the loop */
	else
		info->trace.loops++;
}


/*-------------------------------------------------
    prepare_for_step_overout - prepare things for
    stepping over an instruction
-------------------------------------------------*/

static void prepare_for_step_overout(void)
{
	offs_t pc = activecpu_get_pc();
	char dasmbuffer[100];
	offs_t dasmresult;

	/* disassemble the current instruction and get the flags */
	dasmresult = activecpu_dasm(dasmbuffer, pc);

	/* if flags are supported and it's a call-style opcode, set a temp breakpoint after that instruction */
	if ((dasmresult & DASMFLAG_SUPPORTED) && (dasmresult & DASMFLAG_STEP_OVER))
	{
		int extraskip = (dasmresult & DASMFLAG_OVERINSTMASK) >> DASMFLAG_OVERINSTSHIFT;
		pc += dasmresult & DASMFLAG_LENGTHMASK;

		/* if we need to skip additional instructions, advance as requested */
		while (extraskip-- > 0)
			pc += activecpu_dasm(dasmbuffer, pc) & DASMFLAG_LENGTHMASK;
		step_overout_breakpoint = pc;
	}

	/* if we're stepping out and this isn't a step out instruction, reset the steps until stop to a high number */
	if (execution_state == EXECUTION_STATE_STEP_OUT)
	{
		if ((dasmresult & DASMFLAG_SUPPORTED) && !(dasmresult & DASMFLAG_STEP_OUT))
			steps_until_stop = 100;
		else
			steps_until_stop = 1;
	}
}


/*-------------------------------------------------
    debug_vblank_hook - called when the real
    VBLANK hits
-------------------------------------------------*/

void debug_vblank_hook(void)
{
	/* if we're configured to stop on VBLANK, break */
	if (break_on_vblank)
	{
		execution_state = EXECUTION_STATE_STOPPED;
		debug_console_printf("Stopped at VBLANK\n");
		break_on_vblank = 0;
	}
}


/*-------------------------------------------------
    debug_vblank_hook - called when an interrupt
    is acknowledged
-------------------------------------------------*/

void debug_interrupt_hook(int cpunum, int irqline)
{
	/* if we're configured to stop on interrupt, break */
	if (break_on_interrupt && cpunum == break_on_interrupt_cpunum && (break_on_interrupt_irqline == -1 || irqline == break_on_interrupt_irqline))
	{
		break_on_interrupt = 2;
		break_on_interrupt_irqline = irqline;
	}
}



/*###################################################################################################
**  BREAKPOINTS
**#################################################################################################*/

/*-------------------------------------------------
    debug_check_breakpoints - check the
    breakpoints for a given CPU
-------------------------------------------------*/

void debug_check_breakpoints(int cpunum, offs_t pc)
{
	struct breakpoint *bp;
	UINT64 result;

	/* see if we match */
	for (bp = debug_cpuinfo[cpunum].first_bp; bp; bp = bp->next)
		if (bp->enabled && bp->address == pc)

			/* if we do, evaluate the condition */
			if (bp->condition == NULL || (debug_expression_execute(bp->condition, &result) == EXPRERR_NONE && result))
			{
				/* halt in the debugger by default */
				execution_state = EXECUTION_STATE_STOPPED;

				/* if we hit, evaluate the action */
				if (bp->action != NULL)
					debug_console_execute_command(bp->action, 0);

				/* print a notification, unless the action made us go again */
				if (execution_state == EXECUTION_STATE_STOPPED)
					debug_console_printf("Stopped at breakpoint %d", bp->index);
				break;
			}
}


/*-------------------------------------------------
    debug_breakpoint_first - find the first
    breakpoint for a given CPU
-------------------------------------------------*/

static struct breakpoint *find_breakpoint(int bpnum)
{
	struct breakpoint *bp;
	int cpunum;

	/* loop over CPUs and find the requested breakpoint */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		for (bp = debug_cpuinfo[cpunum].first_bp; bp; bp = bp->next)
			if (bp->index == bpnum)
				return bp;

	return NULL;
}


/*-------------------------------------------------
    debug_breakpoint_first - return the first
    breakpoint for a given CPU
-------------------------------------------------*/

struct breakpoint *debug_breakpoint_first(int cpunum)
{
	return (cpunum < MAX_CPU) ? debug_cpuinfo[cpunum].first_bp : NULL;
}


/*-------------------------------------------------
    debug_breakpoint_set - set a new breakpoint
-------------------------------------------------*/

int debug_breakpoint_set(int cpunum, offs_t address, struct parsed_expression *condition, const char *action)
{
	struct breakpoint *bp = malloc(sizeof(*bp));

	/* if we can't allocate, return failure */
	if (!bp)
		return 0;

	/* fill in the structure */
	bp->index = next_index++;
	bp->enabled = 1;
	bp->address = address;
	bp->condition = condition;
	bp->action = NULL;
	if (action)
	{
		bp->action = malloc(strlen(action) + 1);
		if (bp->action)
			strcpy(bp->action, action);
	}

	/* hook us in */
	bp->next = debug_cpuinfo[cpunum].first_bp;
	debug_cpuinfo[cpunum].first_bp = bp;
	return bp->index;
}


/*-------------------------------------------------
    debug_breakpoint_clear - clear a breakpoint
-------------------------------------------------*/

int debug_breakpoint_clear(int bpnum)
{
	struct breakpoint *bp, *pbp;
	int cpunum;

	/* loop over CPUs and find the requested breakpoint */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		for (pbp = NULL, bp = debug_cpuinfo[cpunum].first_bp; bp; pbp = bp, bp = bp->next)
			if (bp->index == bpnum)
			{
				/* unlink us from the list */
				if (pbp == NULL)
					debug_cpuinfo[cpunum].first_bp = bp->next;
				else
					pbp->next = bp->next;

				/* free the memory */
				if (bp->condition)
					debug_expression_free(bp->condition);
				if (bp->action)
					free(bp->action);
				free(bp);
				return 1;
			}

	/* we didn't find it; return an error */
	return 0;
}


/*-------------------------------------------------
    debug_breakpoint_enable - enable/disable a
    breakpoint
-------------------------------------------------*/

int debug_breakpoint_enable(int bpnum, int enable)
{
	struct breakpoint *bp = find_breakpoint(bpnum);

	/* if we found it, set it */
	if (bp != NULL)
	{
		bp->enabled = (enable != 0);
		return 1;
	}
	return 0;
}



/*###################################################################################################
**  WATCHPOINTS
**#################################################################################################*/

/*-------------------------------------------------
    debug_check_watchpoints - check the
    breakpoints for a given CPU and address space
-------------------------------------------------*/

void debug_check_watchpoints(int cpunum, int spacenum, int type, offs_t address, offs_t size, UINT64 value_to_write)
{
	struct watchpoint *wp;
	UINT64 result;

	/* if we're within debugger code, don't stop */
	if (within_debugger_code)
		return;

	/* if we are a write watchpoint, stash the value that will be written */
	wpaddr = address;
	if (type & WATCHPOINT_WRITE)
		wpdata = value_to_write;

	/* see if we match */
	for (wp = debug_cpuinfo[cpunum].space[spacenum].first_wp; wp; wp = wp->next)
		if (wp->enabled && (wp->type & type) && address + size > wp->address && address < wp->address + wp->length)

			/* if we do, evaluate the condition */
			if (wp->condition == NULL || (debug_expression_execute(wp->condition, &result) == EXPRERR_NONE && result))
			{
				static const char *sizes[] =
				{
					"0bytes", "byte", "word", "3bytes", "dword", "5bytes", "6bytes", "7bytes", "qword"
				};
				char buffer[100];

				/* halt in the debugger by default */
				execution_state = EXECUTION_STATE_STOPPED;

				/* if we hit, evaluate the action */
				if (wp->action != NULL)
					debug_console_execute_command(wp->action, 0);

				/* print a notification, unless the action made us go again */
				if (execution_state == EXECUTION_STATE_STOPPED)
				{
					if (type & WATCHPOINT_WRITE)
					{
						sprintf(buffer, "Stopped at watchpoint %d writing %s to %08X", wp->index, sizes[size], address);
						if (value_to_write >> 32)
							sprintf(&buffer[strlen(buffer)], " (data=%X%08X)", (UINT32)(value_to_write >> 32), (UINT32)value_to_write);
						else
							sprintf(&buffer[strlen(buffer)], " (data=%X)", (UINT32)value_to_write);
					}
					else
						sprintf(buffer, "Stopped at watchpoint %d reading %s from %08X", wp->index, sizes[size], address);
					debug_console_write_line(buffer);
				}
				break;
			}
}


/*-------------------------------------------------
    debug_watchpoint_first - find the first
    watchpoint for a given CPU
-------------------------------------------------*/

static struct watchpoint *find_watchpoint(int wpnum)
{
	struct watchpoint *wp;
	int cpunum, spacenum;

	/* loop over CPUs and address spaces and find the requested watchpoint */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			for (wp = debug_cpuinfo[cpunum].space[spacenum].first_wp; wp; wp = wp->next)
				if (wp->index == wpnum)
					return wp;

	return NULL;
}


/*-------------------------------------------------
    debug_watchpoint_first - return the first
    watchpoint for a given CPU
-------------------------------------------------*/

struct watchpoint *debug_watchpoint_first(int cpunum, int spacenum)
{
	return (cpunum < MAX_CPU && spacenum < ADDRESS_SPACES) ? debug_cpuinfo[cpunum].space[spacenum].first_wp : NULL;
}


/*-------------------------------------------------
    debug_watchpoint_set - set a new watchpoint
-------------------------------------------------*/

int debug_watchpoint_set(int cpunum, int spacenum, int type, offs_t address, offs_t length, struct parsed_expression *condition, const char *action)
{
	struct watchpoint *wp = malloc(sizeof(*wp));

	/* if we can't allocate, return failure */
	if (!wp)
		return 0;

	/* fill in the structure */
	wp->index = next_index++;
	wp->enabled = 1;
	wp->type = type;
	wp->address = ADDR2BYTE(address, &debug_cpuinfo[cpunum], spacenum);
	wp->length = ADDR2BYTE(length, &debug_cpuinfo[cpunum], spacenum);
	wp->condition = condition;
	wp->action = NULL;
	if (action)
	{
		wp->action = malloc(strlen(action) + 1);
		if (wp->action)
			strcpy(wp->action, action);
	}

	/* hook us in */
	wp->next = debug_cpuinfo[cpunum].space[spacenum].first_wp;
	debug_cpuinfo[cpunum].space[spacenum].first_wp = wp;
	debug_cpuinfo[cpunum].total_watchpoints++;
	return wp->index;
}


/*-------------------------------------------------
    debug_watchpoint_clear - clear a watchpoint
-------------------------------------------------*/

int debug_watchpoint_clear(int wpnum)
{
	struct watchpoint *wp, *pwp;
	int cpunum, spacenum;

	/* loop over CPUs and find the requested watchpoint */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			for (pwp = NULL, wp = debug_cpuinfo[cpunum].space[spacenum].first_wp; wp; pwp = wp, wp = wp->next)
				if (wp->index == wpnum)
				{
					/* unlink us from the list */
					if (pwp == NULL)
						debug_cpuinfo[cpunum].space[spacenum].first_wp = wp->next;
					else
						pwp->next = wp->next;

					/* free the memory */
					if (wp->condition)
						debug_expression_free(wp->condition);
					if (wp->action)
						free(wp->action);
					free(wp);
					debug_cpuinfo[cpunum].total_watchpoints--;
					return 1;
				}

	/* we didn't find it; return an error */
	return 0;
}


/*-------------------------------------------------
    debug_watchpoint_enable - enable/disable a
    watchpoint
-------------------------------------------------*/

int debug_watchpoint_enable(int wpnum, int enable)
{
	struct watchpoint *wp = find_watchpoint(wpnum);

	/* if we found it, set it */
	if (wp != NULL)
	{
		wp->enabled = (enable != 0);
		return 1;
	}
	return 0;
}


/*-------------------------------------------------
    debug_watchpoint_count_ptr - return a pointer
    to the count of live watchpoints
-------------------------------------------------*/

const int *debug_watchpoint_count_ptr(int cpunum)
{
	return &debug_cpuinfo[cpunum].total_watchpoints;
}




/*###################################################################################################
**  MEMORY ACCESSORS
**#################################################################################################*/

/*-------------------------------------------------
    debug_read_byte - return a byte from the
    current cpu in the specified memory space
-------------------------------------------------*/

data8_t debug_read_byte(int spacenum, offs_t address)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	data8_t result;
	UINT64 custom;

	memory_set_debugger_access(1);
	if (info->read && (*info->read)(spacenum, address, 1, &custom))
		result = custom;
	else
		result = (*address_space[spacenum].accessors->read_byte)(address);
	memory_set_debugger_access(0);
	return result;
}


/*-------------------------------------------------
    debug_read_word - return a word from the
    current cpu in the specified memory space
-------------------------------------------------*/

data16_t debug_read_word(int spacenum, offs_t address)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	data16_t result;

	if (info->read)
	{
		UINT64 custom;
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->read)(spacenum, address, 2, &custom);
		memory_set_debugger_access(0);
		if (handled)
			return custom;
	}
	if (address_space[spacenum].accessors->read_word)
	{
		memory_set_debugger_access(1);
		result = (*address_space[spacenum].accessors->read_word)(address);
		memory_set_debugger_access(0);
	}
	else
	{
		data8_t byte0 = debug_read_byte(spacenum, address + 0);
		data8_t byte1 = debug_read_byte(spacenum, address + 1);
		if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
			result = byte0 | (byte1 << 8);
		else
			result = byte1 | (byte0 << 8);
	}
	return result;
}


/*-------------------------------------------------
    debug_read_dword - return a dword from the
    current cpu in the specified memory space
-------------------------------------------------*/

data32_t debug_read_dword(int spacenum, offs_t address)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	data32_t result;

	if (info->read)
	{
		UINT64 custom;
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->read)(spacenum, address, 4, &custom);
		memory_set_debugger_access(0);
		if (handled)
			return custom;
	}
	if (address_space[spacenum].accessors->read_dword)
	{
		memory_set_debugger_access(1);
		result = (*address_space[spacenum].accessors->read_dword)(address);
		memory_set_debugger_access(0);
	}
	else
	{
		data16_t word0 = debug_read_word(spacenum, address + 0);
		data16_t word1 = debug_read_word(spacenum, address + 2);
		if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
			result = word0 | (word1 << 16);
		else
			result = word1 | (word0 << 16);
	}
	return result;
}


/*-------------------------------------------------
    debug_read_qword - return a qword from the
    current cpu in the specified memory space
-------------------------------------------------*/

data64_t debug_read_qword(int spacenum, offs_t address)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	data64_t result;

	if (info->read)
	{
		UINT64 custom;
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->read)(spacenum, address, 8, &custom);
		memory_set_debugger_access(0);
		if (handled)
			return custom;
	}
	if (address_space[spacenum].accessors->read_qword)
	{
		memory_set_debugger_access(1);
		result = (*address_space[spacenum].accessors->read_qword)(address);
		memory_set_debugger_access(0);
	}
	else
	{
		data32_t dword0 = debug_read_dword(spacenum, address + 0);
		data32_t dword1 = debug_read_dword(spacenum, address + 4);
		if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
			result = dword0 | ((data64_t)dword1 << 32);
		else
			result = dword1 | ((data64_t)dword0 << 32);
	}
	return result;
}


/*-------------------------------------------------
    debug_write_byte - write a byte to the
    current cpu in the specified memory space
-------------------------------------------------*/

void debug_write_byte(int spacenum, offs_t address, data8_t data)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	memory_set_debugger_access(1);
	if (info->write && (*info->write)(spacenum, address, 1, data))
		;
	else
		(*address_space[spacenum].accessors->write_byte)(address, data);
	memory_set_debugger_access(0);
}


/*-------------------------------------------------
    debug_write_word - write a word to the
    current cpu in the specified memory space
-------------------------------------------------*/

void debug_write_word(int spacenum, offs_t address, data16_t data)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	if (info->write)
	{
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->write)(spacenum, address, 2, data);
		memory_set_debugger_access(0);
		if (handled)
			return;
	}
	if (address_space[spacenum].accessors->write_word)
	{
		memory_set_debugger_access(1);
		(*address_space[spacenum].accessors->write_word)(address, data);
		memory_set_debugger_access(0);
	}
	else if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
	{
		debug_write_byte(spacenum, address + 0, data >> 0);
		debug_write_byte(spacenum, address + 1, data >> 8);
	}
	else
	{
		debug_write_byte(spacenum, address + 0, data >> 8);
		debug_write_byte(spacenum, address + 1, data >> 0);
	}
}


/*-------------------------------------------------
    debug_write_dword - write a dword to the
    current cpu in the specified memory space
-------------------------------------------------*/

void debug_write_dword(int spacenum, offs_t address, data32_t data)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	if (info->write)
	{
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->write)(spacenum, address, 4, data);
		memory_set_debugger_access(0);
		if (handled)
			return;
	}
	if (address_space[spacenum].accessors->write_dword)
	{
		memory_set_debugger_access(1);
		(*address_space[spacenum].accessors->write_dword)(address, data);
		memory_set_debugger_access(0);
	}
	else if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
	{
		debug_write_word(spacenum, address + 0, data >> 0);
		debug_write_word(spacenum, address + 2, data >> 16);
	}
	else
	{
		debug_write_word(spacenum, address + 0, data >> 16);
		debug_write_word(spacenum, address + 2, data >> 0);
	}
}


/*-------------------------------------------------
    debug_write_qword - write a qword to the
    current cpu in the specified memory space
-------------------------------------------------*/

void debug_write_qword(int spacenum, offs_t address, data64_t data)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	if (info->write)
	{
		int handled;
		memory_set_debugger_access(1);
		handled = (*info->write)(spacenum, address, 8, data);
		memory_set_debugger_access(0);
		if (handled)
			return;
	}
	if (address_space[spacenum].accessors->write_qword)
	{
		memory_set_debugger_access(1);
		(*address_space[spacenum].accessors->write_qword)(address, data);
		memory_set_debugger_access(0);
	}
	else if (debug_cpuinfo[cpu_getactivecpu()].endianness == CPU_IS_LE)
	{
		debug_write_dword(spacenum, address + 0, data >> 0);
		debug_write_dword(spacenum, address + 4, data >> 32);
	}
	else
	{
		debug_write_dword(spacenum, address + 0, data >> 32);
		debug_write_dword(spacenum, address + 4, data >> 0);
	}
}


/*-------------------------------------------------
    debug_read_opcode - read 1,2,4 or 8 bytes at
    the given offset from opcode space
-------------------------------------------------*/

UINT64 debug_read_opcode(UINT32 offset, int size)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	static const UINT64 dummy_data = ~0;
	const void *ptr;

	/* adjust the offset */
	memory_set_opbase(offset);

	/* shortcut if we have a custom routine */
	if (info->readop)
	{
		UINT64 result;
		if ((*info->readop)(offset, size, &result))
			return result;
	}

	switch (info->space[ADDRESS_SPACE_PROGRAM].databytes * 10 + size)
	{
		/* dump opcodes in bytes from a byte-sized bus */
		case 11:
			break;

		/* dump opcodes in bytes from a word-sized bus */
		case 21:
			offset ^= (info->endianness == CPU_IS_LE) ? BYTE_XOR_LE(0) : BYTE_XOR_BE(0);
			break;

		/* dump opcodes in words from a word-sized bus */
		case 22:
			break;

		/* dump opcodes in bytes from a dword-sized bus */
		case 41:
			offset ^= (info->endianness == CPU_IS_LE) ? BYTE4_XOR_LE(0) : BYTE4_XOR_BE(0);
			break;

		/* dump opcodes in words from a dword-sized bus */
		case 42:
			offset ^= (info->endianness == CPU_IS_LE) ? WORD_XOR_LE(0) : WORD_XOR_BE(0);
			break;

		/* dump opcodes in dwords from a dword-sized bus */
		case 44:
			break;

		/* dump opcodes in bytes from a qword-sized bus */
		case 81:
			offset ^= (info->endianness == CPU_IS_LE) ? BYTE8_XOR_LE(0) : BYTE8_XOR_BE(0);
			break;

		/* dump opcodes in words from a qword-sized bus */
		case 82:
			offset ^= (info->endianness == CPU_IS_LE) ? WORD2_XOR_LE(0) : WORD2_XOR_BE(0);
			break;

		/* dump opcodes in dwords from a qword-sized bus */
		case 84:
			offset ^= (info->endianness == CPU_IS_LE) ? DWORD_XOR_LE(0) : DWORD_XOR_BE(0);
			break;

		/* dump opcodes in qwords from a qword-sized bus */
		case 88:
			break;

		default:
			osd_die("debug_read_opcode: unknown type = %d\n", info->space[ADDRESS_SPACE_PROGRAM].databytes * 10 + size);
			break;
	}

	/* get pointer to data */
	ptr = memory_get_op_ptr(cpu_getactivecpu(), offset);
	if (!ptr)
		ptr = &dummy_data;	/* if we're not mapped, just return all F's */
	else if (osd_is_bad_read_ptr(ptr, size))
		osd_die("debug_read_opcode: offset %x mapped to invalid memory %p\n", offset, ptr);

	switch(size)
	{
		case 1:	return *((data8_t *) ptr);
		case 2:	return *((data16_t *) ptr);
		case 4:	return *((data32_t *) ptr);
		case 8:	return *((data64_t *) ptr);
	}

	return 0;	/* appease compiler */
}


/*-------------------------------------------------
    debug_read_memory - read 1,2,4 or 8 bytes at
    the given offset in the given address space
-------------------------------------------------*/

UINT64 debug_read_memory(int space, UINT32 offset, int size)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	if (info->space[space].databytes == 0)
		return ~0;

	/* adjust the address into a byte address */
	offset = ADDR2BYTE(offset, info, space);
	switch (size)
	{
		case 1:		return debug_read_byte(space, offset);
		case 2:		return debug_read_word(space, offset);
		case 4:		return debug_read_dword(space, offset);
		case 8:		return debug_read_qword(space, offset);
	}
	return ~0;
}


/*-------------------------------------------------
    debug_write_memory - write 1,2,4 or 8 bytes to
    the given offset in the given address space
-------------------------------------------------*/

void debug_write_memory(int space, UINT32 offset, int size, UINT64 value)
{
	const struct debug_cpu_info *info = &debug_cpuinfo[cpu_getactivecpu()];
	if (info->space[space].databytes == 0)
		return;

	/* adjust the address into a byte address */
	offset = ADDR2BYTE(offset, info, space);
	switch (size)
	{
		case 1:		debug_write_byte(space, offset, value); break;
		case 2:		debug_write_word(space, offset, value); break;
		case 4:		debug_write_dword(space, offset, value); break;
		case 8:		debug_write_qword(space, offset, value); break;
	}
}


/*-------------------------------------------------
    debug_trace_printf - writes text to a given
    CPU's trace file
-------------------------------------------------*/

void debug_trace_printf(int cpunum, const char *fmt, ...)
{
	va_list va;

	struct debug_cpu_info *info = &debug_cpuinfo[cpunum];

	if (info->trace.file)
	{
		va_start(va, fmt);
		vfprintf(info->trace.file, fmt, va);
		va_end(va);
	}
}


/*-------------------------------------------------
    debug_source_script - specifies a debug command
    script to use
-------------------------------------------------*/

void debug_source_script(const char *file)
{
	if (debug_source_file)
	{
		fclose(debug_source_file);
		debug_source_file = NULL;
	}

	if (file)
	{
		debug_source_file = fopen(file, "r");
		if (!debug_source_file)
			debug_console_printf("Cannot open command file '%s'\n", file);
	}
}
