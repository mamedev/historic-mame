/*###################################################################################################
**
**
**      debugcmd.c
**      Debugger help engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#include "driver.h"



/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

#define CONSOLE_HISTORY		(10000)
#define CONSOLE_LINE_CHARS	(100)



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/

struct help_item
{
	const char *		tag;
	const char *		help;
};



/*###################################################################################################
**  LOCAL VARIABLES
**#################################################################################################*/



/*###################################################################################################
**  PROTOTYPES
**#################################################################################################*/



/*###################################################################################################
**  TABLE OF HELP
**#################################################################################################*/

static struct help_item static_help_list[] =
{
	{
		"",
		"\n"
		"MAME Debugger Help\n"
		"  help [<topic>] -- get help on a particular topic\n"
		"\n"
		"Topics:\n"
		"  General\n"
		"  Memory\n"
		"  Execution\n"
		"  Breakpoints\n"
		"  Watchpoints\n"
		"  Expressions\n"
		"\n"
	},
	{
		"general",
		"\n"
		"General Debugger Help\n"
		"Type help <command> for further details on each command\n"
		"\n"
		"  help [<topic>] -- get help on a particular topic\n"
		"  do <expression> -- evaluates the given expression\n"
		"  printf <format>[,<item>[,...]] -- prints one or more <item>s to the console using <format>\n"
		"  logerror <format>[,<item>[,...]] -- outputs one or more <item>s to the error.log\n"
		"  tracelog <format>[,<item>[,...]] -- outputs one or more <item>s to the trace file using <format>\n"
		"  snap [<filename>] -- save a screen snapshot\n"
		"  quit -- exits MAME and the debugger\n"
		"\n"
	},
	{
		"memory",
		"\n"
		"Memory Commands\n"
		"Type help <command> for further details on each command\n"
		"\n"
		"  dasm <filename>,<address>,<length>[,<opcodes>[,<cpunum>]] -- disassemble to the given file\n"
		"  f[ind] <address>,<length>[,<data>[,...]] -- search program memory for data\n"
		"  f[ind]d <address>,<length>[,<data>[,...]] -- search data memory for data\n"
		"  f[ind]i <address>,<length>[,<data>[,...]] -- search I/O memory for data\n"
		"  dump <filename>,<address>,<length>[,<size>[,<ascii>[,<cpunum>]]] -- dump program memory as text\n"
		"  dumpd <filename>,<address>,<length>[,<size>[,<ascii>[,<cpunum>]]] -- dump data memory as text\n"
		"  dumpi <filename>,<address>,<length>[,<size>[,<ascii>[,<cpunum>]]] -- dump I/O memory as text\n"
		"  save <filename>,<address>,<length>[,<cpunum>] -- save binary program memory to the given file\n"
		"  saved <filename>,<address>,<length>[,<cpunum>] -- save binary data memory to the given file\n"
		"  savei <filename>,<address>,<length>[,<cpunum>] -- save binary I/O memory to the given file\n"
		"  map <address> -- map logical program address to physical address and bank\n"
		"  mapd <address> -- map logical data address to physical address and bank\n"
		"  mapi <address> -- map logical I/O address to physical address and bank\n"
		"  memdump [<filename>] -- dump the current memory map to <filename>\n"
		"\n"
	},
	{
		"execution",
		"\n"
		"Execution Commands\n"
		"Type help <command> for further details on each command\n"
		"\n"
		"  s[tep] [<count>=1] -- single steps for <count> instructions (F12)\n"
		"  o[ver] [<count>=1] -- single steps over <count> instructions (F11)\n"
		"  out -- single steps until the current subroutine/exception handler is exited (Shift-F11)\n"
		"  g[o] [<address>] -- resumes execution, sets temp breakpoint at <address> (F5)\n"
		"  gi[nt] [<irqline>] -- resumes execution, setting temp breakpoint if <irqline> is taken (F7)\n"
		"  gv[blank] -- resumes execution, setting temp breakpoint on the next VBLANK (F8)\n"
		"  n[ext] -- executes until the next CPU switch (F6)\n"
		"  focus <cpunum> -- focuses debugger only on <cpunum>\n"
		"  ignore [<cpunum>[,<cpunum>[,...]]] -- stops debugging on <cpunum>\n"
		"  observe [<cpunum>[,<cpunum>[,...]]] -- resumes debugging on <cpunum>\n"
		"  trace {<filename>|OFF}[,<cpunum>[,<action>]] -- trace the given CPU to a file (defaults to active CPU)\n"
		"\n"
	},
	{
		"breakpoints",
		"\n"
		"Breakpoint Commands\n"
		"Type help <command> for further details on each command\n"
		"\n"
		"  bp[set] <address>[,<condition>[,<action>]] -- sets breakpoint at <address>\n"
		"  bpclear [<bpnum>] -- clears a given breakpoint or all if no <bpnum> specified\n"
		"  bpdisable [<bpnum>] -- disables a given breakpoint or all if no <bpnum> specified\n"
		"  bpenable [<bpnum>] -- enables a given breakpoint or all if no <bpnum> specified\n"
		"  bplist -- lists all the breakpoints\n"
		"\n"
	},
	{
		"watchpoints",
		"\n"
		"Watchpoint Commands\n"
		"Type help <command> for further details on each command\n"
		"\n"
		"  wp[set] <address>,<length>,<type>[,<condition>[,<action>]] -- sets program space watchpoint\n"
		"  wpd[set] <address>,<length>,<type>[,<condition>[,<action>]] -- sets data space watchpoint\n"
		"  wpi[set] <address>,<length>,<type>[,<condition>[,<action>]] -- sets I/O space watchpoint\n"
		"  wpclear [<wpnum>] -- clears a given watchpoint or all if no <wpnum> specified\n"
		"  wpdisable [<wpnum>] -- disables a given watchpoint or all if no <wpnum> specified\n"
		"  wpenable [<wpnum>] -- enables a given watchpoint or all if no <wpnum> specified\n"
		"  wplist -- lists all the watchpoints\n"
		"  hotspot [<cpunum>,[<depth>[,<hits>]]] -- attempt to find hotspots\n"
		"\n"
	},
	{
		"expressions",
		"\n"
		"Expressions can be used anywhere a numeric parameter is expected. The syntax for expressions is\n"
		"very close to standard C-style syntax with full operator ordering and parentheses. There are a\n"
		"few operators missing (notably the trinary ? : operator), and a few new ones (memory accessors).\n"
		"The table below lists all the operators in their order, highest precedence operators first.\n"
		"\n"
		"  ( ) : standard parentheses\n"
		"  ++ -- : postfix increment/decrement\n"
		"  ++ -- ~ ! - + b@ w@ d@ q@ : prefix inc/dec, binary NOT, logical NOT, unary +/-, memory access\n"
		"  * / % : multiply, divide, modulus\n"
		"  + - : add, subtract\n"
		"  << >> : shift left/right\n"
		"  < <= > >= : less than, less than or equal, greater than, greater than or equal\n"
		"  == != : equal, not equal\n"
		"  & : binary AND\n"
		"  ^ : binary XOR\n"
		"  | : binary OR\n"
		"  && : logical AND\n"
		"  || : logical OR\n"
		"  = *= /= %= += -= <<= >>= &= |= ^= : assignment\n"
		"  , : separate terms, function parameters\n"
		"\n"
		"These are the differences from C behaviors. First, All math is performed on full 64-bit unsigned\n"
		"values, so things like a < 0 won't work as expected. Second, the logical operators && and || do\n"
		"not have short-circuit properties -- both halves are always evaluated. Finally, the new memory\n"
		"operators work like this: b@<addr> refers to the byte read from <addr>. Similarly, w@ refers to\n"
		"a word in memory, d@ refers to a dword in memory, and q@ refers to a qword in memory. The memory\n"
		"operators can be used as both lvalues and rvalues, so you can write b@100 = ff to store a byte\n"
		"in memory. By default these operators read from the program memory space, but you can override\n"
		"that by prefixing them with a 'd' or an 'i'. So dw@300 refers to data memory word at address\n"
		"300 and id@400 refers to an I/O memory dword at address 400.\n"
		"\n"
	},
	{
		"do",
		"\n"
		"  do <expression>\n"
		"\n"
		"The do command simply evaluates the given <expression>. This is typically used to set or modify\n"
		"variables.\n"
		"\n"
		"Examples:\n"
		"\n"
		"do pc = 0\n"
		"  Sets the register 'pc' to 0.\n"
		"\n"
	},
	{
		"printf",
		"\n"
		"  printf <format>[,<item>[,...]]\n"
		"\n"
		"The printf command performs a C-style printf to the debugger console. Only a very limited set of\n"
		"formatting options are available:\n"
		"\n"
		"  %[0][<n>]d -- prints <item> as a decimal value with optional digit count and zero-fill\n"
		"  %[0][<n>]x -- prints <item> as a hexadecimal value with optional digit count and zero-fill\n"
		"\n"
		"All remaining formatting options are ignored. Use %% together to output a % character. Multiple\n"
		"lines can be printed by embedding a \\n in the text.\n"
		"\n"
		"Examples:\n"
		"\n"
		"printf \"PC=%04X\",pc\n"
		"  Prints PC=<pcval> where <pcval> is displayed in hexadecimal with 4 digits with zero-fill.\n"
		"\n"
		"printf \"A=%d, B=%d\\nC=%d\",a,b,a+b\n"
		"  Prints A=<aval>, B=<bval> on one line, and C=<a+bval> on a second line.\n"
		"\n"
	},
	{
		"logerror",
		"\n"
		"  logerror <format>[,<item>[,...]]\n"
		"\n"
		"The logerror command performs a C-style printf to the error log. Only a very limited set of\n"
		"formatting options are available:\n"
		"\n"
		"  %[0][<n>]d -- logs <item> as a decimal value with optional digit count and zero-fill\n"
		"  %[0][<n>]x -- logs <item> as a hexadecimal value with optional digit count and zero-fill\n"
		"\n"
		"All remaining formatting options are ignored. Use %% together to output a % character. Multiple\n"
		"lines can be printed by embedding a \\n in the text.\n"
		"\n"
		"Examples:\n"
		"\n"
		"logerror \"PC=%04X\",pc\n"
		"  Logs PC=<pcval> where <pcval> is displayed in hexadecimal with 4 digits with zero-fill.\n"
		"\n"
		"logerror \"A=%d, B=%d\\nC=%d\",a,b,a+b\n"
		"  Logs A=<aval>, B=<bval> on one line, and C=<a+bval> on a second line.\n"
		"\n"
	},
	{
		"tracelog",
		"\n"
		"  tracelog <format>[,<item>[,...]]\n"
		"\n"
		"The tracelog command performs a C-style printf and routes the output to the currently open trace\n"
		"file (see the 'trace' command for details). If no file is currently open, tracelog does nothing.\n"
		"Only a very limited set of formatting options are available. See the 'printf' help for details.\n"
		"\n"
		"Examples:\n"
		"\n"
		"tracelog \"PC=%04X\",pc\n"
		"  Outputs PC=<pcval> where <pcval> is displayed in hexadecimal with 4 digits with zero-fill.\n"
		"\n"
		"printf \"A=%d, B=%d\\nC=%d\",a,b,a+b\n"
		"  Outputs A=<aval>, B=<bval> on one line, and C=<a+bval> on a second line.\n"
		"\n"
	},
	{
		"snap",
		"\n"
		"  snap [<filename>]\n"
		"\n"
		"The snap command takes a snapshot of the current video display and saves it to the configured\n"
		"snapshot directory. If <filename> is specified explicitly, it is saved under the requested\n"
		"filename. If <filename> is omitted, it is saved using the same default rules as the \"save\n"
		"snapshot\" key in MAME proper.\n"
		"\n"
		"Examples:\n"
		"\n"
		"snap\n"
		"  Takes a snapshot of the current video screen and saves to the next non-conflicting filename\n"
		"  in the configured snapshot directory.\n"
		"\n"
		"snap shinobi\n"
		"  Takes a snapshot of the current video screen and saves it as 'shinobi.png' in the configured\n"
		"  snapshot directory.\n"
		"\n"
	},
	{
		"source",
		"\n"
		"  source <filename>\n"
		"\n"
		"The source command reads in a set of debugger commands from a file and executes them one by\n"
		"one, similar to a batch file.\n"
		"\n"
		"Examples:\n"
		"\n"
		"source break_and_trace.cmd\n"
		"  Reads in debugger commands from break_and_trace.cmd and executes them.\n"
		"\n"
	},
	{
		"quit",
		"\n"
		"  quit\n"
		"\n"
		"The quit command exits MAME immediately.\n"
		"\n"
	},
	{
		"dasm",
		"\n"
		"  dasm <filename>,<address>,<length>[,<opcodes>[,<cpunum>]]\n"
		"\n"
		"The dasm command disassembles program memory to the file specified in the <filename> parameter.\n"
		"<address> indicates the address of the start of disassembly, and <length> indicates how much\n"
		"memory to disassemble. The range <address> through <address>+<length>-1 inclusive will be\n"
		"output to the file. By default, the raw opcode data is output with each line. The optional\n"
		"<opcodes> parameter can be used to enable (1) or disable(0) this feature. Finally, you can\n"
		"disassemble code from another CPU by specifying the <cpunum> parameter.\n"
		"\n"
		"Examples:\n"
		"\n"
		"dasm venture.asm,0,10000\n"
		"  Disassembles addresses 0-ffff in the current CPU, including raw opcode data, to the\n"
		"  file 'venture.asm'.\n"
		"\n"
		"dasm harddriv.asm,3000,1000,0,2\n"
		"  Disassembles addresses 3000-3fff from CPU #2, with no raw opcode data, to the file\n"
		"  'harddriv.asm'.\n"
		"\n"
	},
	{
		"find",
		"\n"
		"  f[ind][{d|i}] <address>,<length>[,<data>[,...]]\n"
		"\n"
		"The find/findd/findi commands search through memory for the specified sequence of data.\n"
		"'find' will search program space memory, while 'findd' will search data space memory\n"
		"and 'findi' will search I/O space memory. <address> indicates the address to begin searching,\n"
		"and <length> indicates how much memory to search. <data> can either be a quoted string\n"
		"or a numeric value or expression or the wildcard character '?'. Strings by default imply a\n"
		"byte-sized search; non-string data is searched by default in the native word size of the CPU.\n"
		"To override the search size for non-strings, you can prefix the value with b. to force byte-\n"
		"sized search, w. for word-sized search, d. for dword-sized, and q. for qword-sized. Overrides\n"
		"are remembered, so if you want to search for a series of words, you need only to prefix the\n"
		"first value with a w. Note also that you can intermix sizes in order to perform more complex\n"
		"searches. The entire range <address> through <address>+<length>-1\n inclusive will be searched\n"
		"for the sequence, and all occurrences will be displayed.\n"
		"\n"
		"Examples:\n"
		"\n"
		"find 0,10000,\"HIGH SCORE\",0\n"
		"  Searches the address range 0-ffff in the current CPU for the string \"HIGH SCORE\" followed\n"
		"  by a 0 byte.\n"
		"\n"
		"findd 3000,1000,w.abcd,4567\n"
		"  Searches the data memory address range 3000-3fff for the word-sized value abcd followed by\n"
		"  the word-sized value 4567.\n"
		"\n"
		"find 0,8000,\"AAR\",d.0,\"BEN\",w.0\n"
		"  Searches the address range 0000-7fff for the string \"AAR\" followed by a dword-sized 0\n"
		"  followed by the string \"BEN\", followed by a word-sized 0.\n"
		"\n"
	},
	{
		"dump",
		"\n"
		"  dump[{d|i}] <filename>,<address>,<length>[,<size>[,<ascii>[,<cpunum>]]]\n"
		"\n"
		"The dump/dumpd/dumpi commands dump memory to the text file specified in the <filename>\n"
		"parameter. 'dump' will dump program space memory, while 'dumpd' will dump data space memory\n"
		"and 'dumpi' will dump I/O space memory. <address> indicates the address of the start of dumping,\n"
		"and <length> indicates how much memory to dump. The range <address> through <address>+<length>-1\n"
		"inclusive will be output to the file. By default, the data will be output in byte format, unless\n"
		"the underlying address space is word/dword/qword-only. You can override this by specifying the\n"
		"<size> parameter, which can be used to group the data in 1, 2, 4 or 8-byte chunks. The optional\n"
		"<ascii> parameter can be used to enable (1) or disable (0) the output of ASCII characters to the\n"
		"right of each line; by default, this is enabled. Finally, you can dump memory from another CPU\n"
		"by specifying the <cpunum> parameter.\n"
		"\n"
		"Examples:\n"
		"\n"
		"dump venture.dmp,0,10000\n"
		"  Dumps addresses 0-ffff in the current CPU in 1-byte chunks, including ASCII data, to\n"
		"  the file 'venture.dmp'.\n"
		"\n"
		"dumpd harddriv.dmp,3000,1000,4,0,3\n"
		"  Dumps data memory addresses 3000-3fff from CPU #3 in 4-byte chunks, with no ASCII data,\n"
		"  to the file 'harddriv.dmp'.\n"
		"\n"
	},
	{
		"save",
		"\n"
		"  save[{d|i}] <filename>,<address>,<length>[,<cpunum>]\n"
		"\n"
		"The save/saved/savei commands save raw memory to the binary file specified in the <filename>\n"
		"parameter. 'save' will save program space memory, while 'saved' will save data space memory\n"
		"and 'savei' will save I/O space memory. <address> indicates the address of the start of saving,\n"
		"and <length> indicates how much memory to save. The range <address> through <address>+<length>-1\n"
		"inclusive will be output to the file. You can also save memory from another CPU by specifying the\n"
		"<cpunum> parameter.\n"
		"\n"
		"Examples:\n"
		"\n"
		"save venture.bin,0,10000\n"
		"  Saves addresses 0-ffff in the current CPU to the binary file 'venture.bin'.\n"
		"\n"
		"saved harddriv.bin,3000,1000,3\n"
		"  Saves data memory addresses 3000-3fff from CPU #3 to the binary file 'harddriv.bin'.\n"
		"\n"
	},
	{
		"step",
		"\n"
		"  s[tep] [<count>=1]\n"
		"\n"
		"The step command single steps one or more instructions in the currently executing CPU. By\n"
		"default, step executes one instruction each time it is issued. You can also tell step to step\n"
		"multiple instructions by including the optional <count> parameter.\n"
		"\n"
		"Examples:\n"
		"\n"
		"s\n"
		"  Steps forward one instruction on the current CPU.\n"
		"\n"
		"step 4\n"
		"  Steps forward four instructions on the current CPU.\n"
		"\n"
	},
	{
		"over",
		"\n"
		"  o[ver] [<count>=1]\n"
		"\n"
		"The over command single steps \"over\" one or more instructions in the currently executing CPU,\n"
		"stepping over subroutine calls and exception handler traps and counting them as a single\n"
		"instruction. Note that when stepping over a subroutine call, code may execute on other CPUs\n"
		"before the subroutine call completes. By default, over executes one instruction each time it is\n"
		"issued. You can also tell step to step multiple instructions by including the optional <count> \n"
		"parameter.\n"
		"\n"
		"Note that the step over functionality may not be implemented on all CPU types. If it is not\n"
		"implemented, then 'over' will behave exactly like 'step'.\n"
		"\n"
		"Examples:\n"
		"\n"
		"o\n"
		"  Steps forward over one instruction on the current CPU.\n"
		"\n"
		"over 4\n"
		"  Steps forward over four instructions on the current CPU.\n"
		"\n"
	},
	{
		"out",
		"\n"
		"  out\n"
		"\n"
		"The out command single steps until it encounters a return from subroutine or return from\n"
		"exception instruction. Note that because it detects return from exception conditions, if you\n"
		"attempt to step out of a subroutine and an interrupt/exception occurs before you hit the end,\n"
		"then you may stop prematurely at the end of the exception handler.\n"
		"\n"
		"Note that the step out functionality may not be implemented on all CPU types. If it is not\n"
		"implemented, then 'out' will behave exactly like 'out'.\n"
		"\n"
		"Examples:\n"
		"\n"
		"out\n"
		"  Steps until the current subroutine or exception handler returns.\n"
		"\n"
	},
	{
		"go",
		"\n"
		"  g[o] [<address>]\n"
		"\n"
		"The go command resumes execution of the current code. Control will not be returned to the\n"
		"debugger until a breakpoint or watchpoint is hit, or until you manually break in using the\n"
		"assigned key. The go command takes an optional <address> parameter which is a temporary\n"
		"unconditional breakpoint that is set before executing, and automatically removed when hit.\n"
		"\n"
		"Examples:\n"
		"\n"
		"g\n"
		"  Resume execution until the next break/watchpoint or until a manual break.\n"
		"\n"
		"g 1234\n"
		"  Resume execution, stopping at address 1234 unless something else stops us first.\n"
		"\n"
	},
	{
		"gvblank",
		"\n"
		"  gv[blank]\n"
		"\n"
		"The gvblank command resumes execution of the current code. Control will not be returned to\n"
		"the debugger until a breakpoint or watchpoint is hit, or until the next VBLANK occurs in the\n"
		"emulator.\n"
		"\n"
		"Examples:\n"
		"\n"
		"gv\n"
		"  Resume execution until the next break/watchpoint or until the next VBLANK.\n"
		"\n"
	},
	{
		"gint",
		"\n"
		"  gi[nt] [<irqline>]\n"
		"\n"
		"The gint command resumes execution of the current code. Control will not be returned to the\n"
		"debugger until a breakpoint or watchpoint is hit, or until an IRQ is asserted and acknowledged\n"
		"on the current CPU. You can specify <irqline> if you wish to stop execution only on a particular\n"
		"IRQ line being asserted and acknowledged. If <irqline> is omitted, then any IRQ line will stop\n"
		"execution.\n"
		"\n"
		"Examples:\n"
		"\n"
		"gi\n"
		"  Resume execution until the next break/watchpoint or until any IRQ is asserted and acknowledged\n"
		"  on the current CPU.\n"
		"\n"
		"gint 4\n"
		"  Resume execution until the next break/watchpoint or until IRQ line 4 is asserted and\n"
		"  acknowledged on the current CPU.\n"
		"\n"
	},
	{
		"next",
		"\n"
		"  n[ext]\n"
		"\n"
		"The next command resumes execution and continues executing until the next time a different\n"
		"CPU is scheduled. Note that if you have used 'ignore' to ignore certain CPUs, you will not\n"
		"stop until a non-'ignore'd CPU is scheduled.\n"
	},
	{
		"focus",
		"\n"
		"  focus <cpunum>\n"
		"\n"
		"Sets the debugger focus exclusively to the given <cpunum>. This is equivalent to specifying\n"
		"'ignore' on all other CPUs.\n"
		"\n"
		"Examples:\n"
		"\n"
		"focus 1\n"
		"  Focus exclusively CPU #1 while ignoring all other CPUs when using the debugger.\n"
		"\n"
	},
	{
		"ignore",
		"\n"
		"  ignore [<cpunum>[,<cpunum>[,...]]]\n"
		"\n"
		"Ignores the specified <cpunum> in the debugger. This means that you won't ever see execution\n"
		"on that CPU, nor will you be able to set breakpoints on that CPU. To undo this change use\n"
		"the 'observe' command. You can specify multiple <cpunum>s in a single command. Note also that\n"
		"you are not permitted to ignore all CPUs; at least one must be active at all times.\n"
		"\n"
		"Examples:\n"
		"\n"
		"ignore 1\n"
		"  Ignore CPU #1 when using the debugger.\n"
		"\n"
		"ignore 2,3,4\n"
		"  Ignore CPU #2, #3 and #4 when using the debugger.\n"
		"\n"
		"ignore\n"
		"  List the CPUs that are currently ignored.\n"
		"\n"
	},
	{
		"observe",
		"\n"
		"  observe [<cpunum>[,<cpunum>[,...]]]\n"
		"\n"
		"Re-enables interaction with the specified <cpunum> in the debugger. This command undoes the\n"
		"effects of the 'ignore' command. You can specify multiple <cpunum>s in a single command.\n"
		"\n"
		"Examples:\n"
		"\n"
		"observe 1\n"
		"  Stop ignoring CPU #1 when using the debugger.\n"
		"\n"
		"observe 2,3,4\n"
		"  Stop ignoring CPU #2, #3 and #4 when using the debugger.\n"
		"\n"
		"observe\n"
		"  List the CPUs that are currently observed.\n"
		"\n"
	},
	{
		"trace",
		"\n"
		"  trace {<filename>|OFF}[,<cpunum>[,<action>]]\n"
		"\n"
		"Starts or stops tracing of the execution of the specified <cpunum>. If <cpunum> is omitted,\n"
		"the currently active CPU is specified. When enabling tracing, specify the filename in the\n"
		"<filename> parameter. To disable tracing, substitute the keyword 'off' for <filename>. If you\n"
		"wish to log additional information on each trace, you can append an <action> parameter which\n"
		"is a command that is executed before each trace is logged. Generally, this is used to include\n"
		"a 'tracelog' command. Note that you may need to embed the action within braces { } in order\n"
		"to prevent commas and semicolons from being interpreted as applying to the trace command\n"
		"itself.\n"
		"\n"
		"Examples:\n"
		"\n"
		"trace dribling.tr,0\n"
		"  Begin tracing the execution of CPU #0, logging output to dribling.tr.\n"
		"\n"
		"trace joust.tr\n"
		"  Begin tracing the currently active CPU, logging output to joust.tr.\n"
		"\n"
		"trace >>pigskin.tr\n"
		"  Begin tracing the currently active CPU, appending log output to pigskin.tr.\n"
		"\n"
		"trace off,0\n"
		"  Turn off tracing on CPU #0.\n"
		"\n"
		"trace asteroid.tr,0,{tracelog \"A=%02X \",a}\n"
		"  Begin tracing the execution of CPU #0, logging output to asteroid.tr. Before each line,\n"
		"  output A=<aval> to the tracelog.\n"
		"\n"
	},
	{
		"traceover",
		"\n"
		"  traceover {<filename>|OFF}[,<cpunum>[,<action>]]\n"
		"\n"
		"Starts or stops tracing of the execution of the specified <cpunum>. When tracing reaches\n"
		"a subroutine or call, tracing will skip over the subroutine. The same algorithm is used as is\n"
		"used in the step over command. This means that traceover will not work properly when calls\n"
		"are recusive or the return address is not immediately following the call instruction. If\n"
		"<cpunum> is omitted, the currently active CPU is specified. When enabling tracing, specify the\n"
		"filename in the <filename> parameter. To disable tracing, substitute the keyword 'off' for\n"
		"<filename>. If you wish to log additional information on each trace, you can append an <action>\n"
		"parameter which is a command that is executed before each trace is logged. Generally, this is\n"
		"used to include a 'tracelog' command. Note that you may need to embed the action within braces\n"
		"{ } in order to prevent commas and semicolons from being interpreted as applying to the trace\n"
		"command itself.\n"
		"\n"
		"Examples:\n"
		"\n"
		"traceover dribling.tr,0\n"
		"  Begin tracing the execution of CPU #0, logging output to dribling.tr.\n"
		"\n"
		"traceover joust.tr\n"
		"  Begin tracing the currently active CPU, logging output to joust.tr.\n"
		"\n"
		"traceover off,0\n"
		"  Turn off tracing on CPU #0.\n"
		"\n"
		"traceover asteroid.tr,0,{tracelog \"A=%02X \",a}\n"
		"  Begin tracing the execution of CPU #0, logging output to asteroid.tr. Before each line,\n"
		"  output A=<aval> to the tracelog.\n"
		"\n"
	},
	{
		"traceflush",
		"\n"
		"  traceflush\n"
		"\n"
		"Flushes all open trace files.\n"
	},
	{
		"bpset",
		"\n"
		"  bp[set] <address>[,<condition>[,<action>]]\n"
		"\n"
		"Sets a new execution breakpoint at the specified <address>. The optional <condition>\n"
		"parameter lets you specify an expression that will be evaluated each time the breakpoint is\n"
		"hit. If the result of the expression is true (non-zero), the breakpoint will actually halt\n"
		"execution; otherwise, execution will continue with no notification. The optional <action>\n"
		"parameter provides a command that is executed whenever the breakpoint is hit and the\n"
		"<condition> is true. Note that you may need to embed the action within braces { } in order\n"
		"to prevent commas and semicolons from being interpreted as applying to the bpset command\n"
		"itself. Each breakpoint that is set is assigned an index which can be used in other\n"
		"breakpoint commands to reference this breakpoint.\n"
		"\n"
		"Examples:\n"
		"\n"
		"bp 1234\n"
		"  Set a breakpoint that will halt execution whenever the PC is equal to 1234.\n"
		"\n"
		"bp 23456,a0 == 0 && a1 == 0\n"
		"  Set a breakpoint that will halt execution whenever the PC is equal to 23456 AND the\n"
		"  expression (a0 == 0 && a1 == 0) is true.\n"
		"\n"
		"bp 3456,1,{printf \"A0=%08X\\n\",a0; g}\n"
		"  Set a breakpoint that will halt execution whenever the PC is equal to 3456. When\n"
		"  this happens, print A0=<a0val> and continue executing.\n"
		"\n"
		"bp 45678,a0==100,{do a0 = ff; g}\n"
		"  Set a breakpoint that will halt execution whenever the PC is equal to 45678 AND the\n"
		"  expression (a0 == 100) is true. When that happens, set a0 to ff and resume execution.\n"
		"\n"
		"do temp0 = 0; bp 567890,++temp0 >= 10\n"
		"  Set a breakpoint that will halt execution whenever the PC is equal to 567890 AND the\n"
		"  expression (++temp0 >= 10) is true. This effectively breaks only after the breakpoint\n"
		"  has been hit 16 times.\n"
		"\n"
	},
	{
		"bpclear",
		"\n"
		"  bpclear [<bpnum>]\n"
		"\n"
		"The bpclear command clears a breakpoint. If <bpnum> is specified, only the requested\n"
		"breakpoint is cleared, otherwise all breakpoints are cleared.\n"
		"\n"
		"Examples:\n"
		"\n"
		"bpclear 3\n"
		"  Clear breakpoint index 3.\n"
		"\n"
		"bpclear\n"
		"  Clear all breakpoints.\n"
		"\n"
	},
	{
		"bpdisable",
		"\n"
		"  bpdisable [<bpnum>]\n"
		"\n"
		"The bpdisable command disables a breakpoint. If <bpnum> is specified, only the requested\n"
		"breakpoint is disabled, otherwise all breakpoints are disabled. Note that disabling a\n"
		"breakpoint does not delete it, it just temporarily marks the breakpoint as inactive.\n"
		"\n"
		"Examples:\n"
		"\n"
		"bpdisable 3\n"
		"  Disable breakpoint index 3.\n"
		"\n"
		"bpdisable\n"
		"  Disable all breakpoints.\n"
		"\n"
	},
	{
		"bpenable",
		"\n"
		"  bpenable [<bpnum>]\n"
		"\n"
		"The bpenable command enables a breakpoint. If <bpnum> is specified, only the requested\n"
		"breakpoint is enabled, otherwise all breakpoints are enabled.\n"
		"\n"
		"Examples:\n"
		"\n"
		"bpenable 3\n"
		"  Enable breakpoint index 3.\n"
		"\n"
		"bpenable\n"
		"  Enable all breakpoints.\n"
		"\n"
	},
	{
		"bplist",
		"\n"
		"  bplist\n"
		"\n"
		"The bplist command lists all the current breakpoints, along with their index and any\n"
		"conditions or actions attached to them.\n"
		"\n"
	},
	{
		"wpset",
		"\n"
		"  wp[{d|i}][set] <address>,<length>,<type>[,<condition>[,<action>]]\n"
		"\n"
		"Sets a new watchpoint starting at the specified <address> and extending for <length>. The\n"
		"inclusive range of the watchpoint is <address> through <address> + <length> - 1. The 'wpset'\n"
		"command sets a watchpoint on program memory; the 'wpdset' command sets a watchpoint on data\n"
		"memory; and the 'wpiset' sets a watchpoint on I/O memory. The <type> parameter specifies\n"
		"which sort of accesses to trap on. It can be one of three values: 'r' for a read watchpoint\n"
		"'w' for a write watchpoint, and 'rw' for a read/write watchpoint.\n"
		"\n"
		"The optional <condition> parameter lets you specify an expression that will be evaluated each\n"
		"time the watchpoint is hit. If the result of the expression is true (non-zero), the watchpoint\n"
		"will actually halt execution; otherwise, execution will continue with no notification. The\n"
		"optional <action> parameter provides a command that is executed whenever the watchpoint is hit\n"
		"and the <condition> is true. Note that you may need to embed the action within braces { } in\n"
		"order to prevent commas and semicolons from being interpreted as applying to the wpset command\n"
		"itself. Each watchpoint that is set is assigned an index which can be used in other\n"
		"watchpoint commands to reference this watchpoint.\n"
		"\n"
		"In order to help <condition> expressions, two variables are available. For all watchpoints,\n"
		"the variable 'wpaddr' is set to the address that actually triggered the watchpoint. For write\n"
		"watchpoints, the variable 'wpdata' is set to the data that is being written.\n"
		"\n"
		"Examples:\n"
		"\n"
		"wp 1234,6,rw\n"
		"  Set a watchpoint that will halt execution whenever a read or write occurs in the address\n"
		"  range 1234-1239 inclusive.\n"
		"\n"
		"wp 23456,a,w,wpdata == 1\n"
		"  Set a watchpoint that will halt execution whenever a write occurs in the address range\n"
		"  23456-2345f AND the data written is equal to 1.\n"
		"\n"
		"wp 3456,20,r,1,{printf \"Read @ %08X\\n\",wpaddr; g}\n"
		"  Set a watchpoint that will halt execution whenever a read occurs in the address range\n"
		"  3456-3475. When this happens, print Read @ <wpaddr> and continue executing.\n"
		"\n"
		"do temp0 = 0; wp 45678,1,writeval==f0,{do temp0++; g}\n"
		"  Set a watchpoint that will halt execution whenever a write occurs to the address 45678 AND\n"
		"  the value being written is equal to f0. When that happens, increment the variable temp0 and\n"
		"  resume execution.\n"
		"\n"
	},
	{
		"wpclear",
		"\n"
		"  wpclear [<wpnum>]\n"
		"\n"
		"The wpclear command clears a watchpoint. If <wpnum> is specified, only the requested\n"
		"watchpoint is cleared, otherwise all watchpoints are cleared.\n"
		"\n"
		"Examples:\n"
		"\n"
		"wpclear 3\n"
		"  Clear watchpoint index 3.\n"
		"\n"
		"wpclear\n"
		"  Clear all watchpoints.\n"
		"\n"
	},
	{
		"wpdisable",
		"\n"
		"  wpdisable [<wpnum>]\n"
		"\n"
		"The wpdisable command disables a watchpoint. If <wpnum> is specified, only the requested\n"
		"watchpoint is disabled, otherwise all watchpoints are disabled. Note that disabling a\n"
		"watchpoint does not delete it, it just temporarily marks the watchpoint as inactive.\n"
		"\n"
		"Examples:\n"
		"\n"
		"wpdisable 3\n"
		"  Disable watchpoint index 3.\n"
		"\n"
		"wpdisable\n"
		"  Disable all watchpoints.\n"
		"\n"
	},
	{
		"wpenable",
		"\n"
		"  wpenable [<wpnum>]\n"
		"\n"
		"The wpenable command enables a watchpoint. If <wpnum> is specified, only the requested\n"
		"watchpoint is enabled, otherwise all watchpoints are enabled.\n"
		"\n"
		"Examples:\n"
		"\n"
		"wpenable 3\n"
		"  Enable watchpoint index 3.\n"
		"\n"
		"wpenable\n"
		"  Enable all watchpoints.\n"
		"\n"
	},
	{
		"wplist",
		"\n"
		"  wplist\n"
		"\n"
		"The wplist command lists all the current watchpoints, along with their index and any\n"
		"conditions or actions attached to them.\n"
		"\n"
	},
	{
		"hotspot",
		"\n"
		"  hotspot [<cpunum>,[<depth>[,<hits>]]]\n"
		"\n"
		"The hotspot command attempts to help locate hotspots in the code where speedup opportunities\n"
		"might be present. <cpunum>, which defaults to the currently active CPU, specified which\n"
		"processor's memory to track. <depth>, which defaults to 64, controls the depth of the search\n"
		"buffer. The search buffer tracks the last <depth> memory reads from unique PCs. The <hits>\n"
		"parameter, which defaults to 250, specifies the minimum number of hits to report.\n"
		"\n"
		"The basic theory of operation is like this: each memory read is trapped by the debugger and\n"
		"logged in the search buffer according to the address which was read and the PC that executed\n"
		"the opcode. If the search buffer already contains a matching entry, that entry's count is\n"
		"incremented and the entry is moved to the top of the list. If the search buffer does not\n"
		"contain a matching entry, the entry from the bottom of the list is removed, and a new entry\n"
		"is created at the top with an initial count of 1. Entries which fall off the bottom are\n"
		"examined and if their count is larger than <hits>, they are reported to the debugger\n"
		"console.\n"
		"\n"
		"Examples:\n"
		"\n"
		"hotspot 0,10\n"
		"  Looks for hotspots on CPU 0 using a search buffer of 16 entries, reporting any entries which\n"
		"  end up with 250 or more hits.\n"
		"\n"
		"hotspot 1,40,#1000\n"
		"  Looks for hotspots on CPU 1 using a search buffer of 64 entries, reporting any entries which\n"
		"  end up with 1000 or more hits.\n"
	},
	{
		"map",
		"\n"
		"  map[{d|i}] <address>\n"
		"\n"
		"The map/mapd/mapi commands map a logical address in memory to the correct physical address, as\n"
		"well as specifying the bank. 'map' will map program space memory, while 'mapd' will map data space\n"
		"memory and 'mapi' will map I/O space memory.\n"
		"\n"
		"Example:\n"
		"\n"
		"map 152d0\n"
		"  Gives physical address and bank for logical address 152d0 in program memory\n"
		"\n"
	},
	{
		"memdump",
		"\n"
		"  memdump [<filename>]\n"
		"\n"
		"Dumps the current memory map to <filename>. If <filename> is omitted, then dumps\n"
		"to memdump.log"
		"\n"
		"Examples:\n"
		"\n"
		"memdump mylog.log\n"
		"  Dumps memory to mylog.log.\n"
		"\n"
		"memdump\n"
		"  Dumps memory to memdump.log.\n"
		"\n"
	},
};



/*###################################################################################################
**  CODE
**#################################################################################################*/

const char *debug_get_help(const char *tag)
{
	static char ambig_message[1024];
	struct help_item *found = NULL;
	int i, msglen, foundcount = 0;
	int taglen = strlen(tag);
	char tagcopy[256];

	/* make a lowercase copy of the tag */
	for (i = 0; i <= taglen; i++)
		tagcopy[i] = tolower(tag[i]);

	/* find a match */
	for (i = 0; i < sizeof(static_help_list) / sizeof(static_help_list[0]); i++)
		if (!strncmp(static_help_list[i].tag, tagcopy, taglen))
		{
			foundcount++;
			found = &static_help_list[i];
			if (strlen(found->tag) == taglen)
			{
				foundcount = 1;
				break;
			}
		}

	/* only a single match makes sense */
	if (foundcount == 1)
		return found->help;

	/* if not found, return the first entry */
	if (foundcount == 0)
		return static_help_list[0].help;

	/* otherwise, indicate ambiguous help */
	msglen = sprintf(ambig_message, "Ambiguous help request, did you mean:\n");
	for (i = 0; i < sizeof(static_help_list) / sizeof(static_help_list[0]); i++)
		if (!strncmp(static_help_list[i].tag, tagcopy, taglen))
			msglen += sprintf(&ambig_message[msglen], "  help %s?\n", static_help_list[i].tag);
	return ambig_message;
}

