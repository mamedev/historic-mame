#ifndef _MAMEDBG_H
#define _MAMEDBG_H

#ifndef macintosh
#ifndef WIN32
#ifndef UNIX
	#include <conio.h>
	#define __INLINE__ static __inline__	/* keep allegro.h happy */
	#include <allegro.h>
	#undef __INLINE__
#endif
#endif
#endif

#include "driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef macintosh /* JB 980208 */
#define uclock_t clock_t
#define	uclock clock
#define UCLOCKS_PER_SEC CLOCKS_PER_SEC
#endif

#ifdef UNIX
#include "xmame.h" /* defines uclock types depending on arch */
#endif

#include "osdepend.h"
#include "osd_dbg.h"
#include "cpuintrf.h"
#include "cpu/z80/z80.h"
#include "cpu/m6502/m6502.h"
#include "cpu/h6280/h6280.h"
#include "cpu/i86/i86intrf.h"
#include "cpu/i8039/i8039.h"
#include "cpu/i8085/i8085.h"
#include "cpu/m6808/m6808.h"
#include "cpu/m6805/m6805.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/s2650/s2650.h"
#include "cpu/t11/t11.h"    /* ASG 030598 */
#include "cpu/tms9900/tms9900.h"
#include "cpu/z8000/z8000.h"
#include "cpu/tms32010/tms32010.h"


/* JB 980214 */
void asg_2650Trace(unsigned char *RAM, int PC);
void asg_TraceInit(int count, char *filename);
void asg_TraceKill(void);
void asg_TraceSelect(int indx);
void asg_Z80Trace(unsigned char *RAM, int PC);
void asg_6809Trace(unsigned char *RAM, int PC);
void asg_6800Trace(unsigned char *RAM, int PC);
void asg_6802Trace(unsigned char *RAM, int PC);
void asg_6803Trace(unsigned char *RAM, int PC);
void asg_6808Trace(unsigned char *RAM, int PC);
void asg_63701Trace(unsigned char *RAM, int PC);
void asg_6805Trace(unsigned char *RAM, int PC);
void asg_6502Trace(unsigned char *RAM, int PC);
void asg_68000Trace(unsigned char *RAM, int PC);
void asg_8085Trace(unsigned char *RAM, int PC);
void asg_8039Trace(unsigned char *RAM, int PC);
void asg_T11Trace(unsigned char *RAM, int PC);	/* ASG 030598 */
void asg_34010Trace(unsigned char *RAM, int PC); /* AJP 080298 */
void asg_I86Trace(unsigned char *RAM, int PC); /* AM 980925 */
void asg_9900Trace(unsigned char *RAM, int PC);
void asg_Z8000Trace(unsigned char *RAM, int PC);
void asg_32010Trace(unsigned char *RAM, int PC);
void asg_6280Trace(unsigned char *RAM, int PC);


extern int traceon;

extern int CurrentVolume;

#define	MEM1DEFAULT             0x0000
#define MEM2DEFAULT             0x8000
#define	HEADING_COLOUR          LIGHTGREEN
#define	LINE_COLOUR             LIGHTCYAN
#define	REGISTER_COLOUR         WHITE
#define	FLAG_COLOUR             WHITE
#define	BREAKPOINT_COLOUR       YELLOW
#define	PC_COLOUR               (WHITE+BLUE*16)	/* MB 980103 */
#ifndef UNIX
#define	CURSOR_COLOUR           (WHITE+RED*16)	/* MB 980103 */
#else
#define	CURSOR_COLOUR           MAGENTA
#endif
#define COMMANDINFO_COLOUR		(BLACK+7*16)	/* MB 980201 */
#define	CODE_COLOUR             WHITE
#define	MEM1_COLOUR             WHITE
#define	CHANGES_COLOUR          LIGHTCYAN
#define	MEM2_COLOUR             WHITE
#define	ERROR_COLOUR            RED
#define	PROMPT_COLOUR           CYAN
#define	INSTRUCTION_COLOUR      WHITE
#define	INPUT_COLOUR            WHITE

static unsigned char MemWindowBackup[0x100*2];	/* Enough to backup 2 windows */

static void DrawDebugScreen8 (int TextCol, int LineCol);
static void DrawDebugScreen16 (int TextCol, int LineCol);
static void DrawDebugScreen32 (int TextCol, int LineCol);
static void DrawDebugScreen16word (int TextCol, int LineCol);

int DummyDasm(char *S, int PC){	return (1); }
int DummyCC = 0;

void DummyTrace(int PC){;}	/* JB 980214 */

/* Temporary wrappers around these functions at the moment */
int TempDasm6800 (char *buffer, int pc) { return (Dasm6800 (buffer, pc)); }
int TempDasm6802 (char *buffer, int pc) { return (Dasm6802 (buffer, pc)); }
int TempDasm6803 (char *buffer, int pc) { return (Dasm6803 (buffer, pc)); }
int TempDasm6808 (char *buffer, int pc) { return (Dasm6808 (buffer, pc)); }
int TempDasm63701 (char *buffer, int pc) { return (Dasm63701 (buffer, pc)); }
int TempDasm6805 (char *buffer, int pc) { return (Dasm6805 (&ROM[pc], buffer, pc)); }
int TempDasm6809 (char *buffer, int pc) { return (Dasm6809 (buffer, pc)); }
int TempDasm68000 (char *buffer, int pc){ change_pc24(pc); return (Dasm68000 (buffer, pc));}
int TempDasm8085 (char *buffer, int pc)  { return (Dasm8085 (buffer, pc)); }
int TempDasm8039 (char *buffer, int pc)  { return (Dasm8039 (buffer, &ROM[pc])); }
int TempDasmT11 (char *buffer, int pc){ return (DasmT11 (&ROM[pc], buffer, pc));}	/* ASG 030598 */
int TempDasmZ80 (char *buffer, int pc)  { return (DasmZ80 (buffer, pc)); }
int TempDasm2650 (char *buffer, int pc) { return (Dasm2650 (buffer, pc)); }
int TempDasm34010 (char *buffer, int pc){ return (Dasm34010 (&OP_ROM[((unsigned int) pc)>>3], buffer, pc));}
int TempDasmI86 (char *buffer, int pc) { return (DasmI86 (&OP_ROM[pc], buffer, pc));}	/* AM 980925 */
int TempDasm9900 (char *buffer, int pc) { return (Dasm9900 (buffer, pc)); }
int TempDasmZ8000 (char *buffer, int pc){ return (DasmZ8000 (buffer, pc));}
int TempDasm32010 (char *buffer, int pc){ return (Dasm32010 (buffer, &ROM[(pc<<1)])); }

/* JB 980214 */
void TempZ80Trace (int PC) { asg_Z80Trace (ROM, PC); }
void Temp6809Trace (int PC) { asg_6809Trace (ROM, PC); }
void Temp6800Trace (int PC) { asg_6800Trace (ROM, PC); }
void Temp6802Trace (int PC) { asg_6802Trace (ROM, PC); }
void Temp6803Trace (int PC) { asg_6803Trace (ROM, PC); }
void Temp6808Trace (int PC) { asg_6808Trace (ROM, PC); }
void Temp63701Trace (int PC) { asg_63701Trace (ROM, PC); }
void Temp6805Trace (int PC) { asg_6805Trace (ROM, PC); }
void Temp6502Trace (int PC) { asg_6502Trace (ROM, PC); }
void Temp68000Trace (int PC) { asg_68000Trace (ROM, PC); }
void Temp8085Trace (int PC) { asg_8085Trace (ROM, PC); }
void Temp2650Trace (int PC) { asg_2650Trace (ROM, PC); } /* HJB 110698 */
void Temp8039Trace (int PC) { asg_8039Trace (ROM, PC); } /* AM 200698 */
void TempT11Trace (int PC) { asg_T11Trace (ROM, PC); }  /* ASG 030598 */
void Temp34010Trace (int PC) { asg_34010Trace (OP_ROM, PC); }  /* AJP 080298 */
void TempI86Trace (int PC) { asg_I86Trace (ROM, PC); }  /* AM 980925 */
void Temp9900Trace (int PC) { asg_9900Trace (ROM, PC); }
void TempZ8000Trace (int PC) { asg_Z8000Trace (ROM, PC); }  /* HJB 981115 */
void Temp32010Trace (int PC) { asg_32010Trace (ROM, (PC<<1)); }
void Temp6280Trace (int PC) { asg_6280Trace (ROM, PC); }

/* Commands functions */
static int ModifyRegisters(char *param);
static int SetBreakPoint(char *param);
static int ClearBreakPoint(char *param);
static int Here(char *param);
static int Go(char *param);
static int DumpToFile(char *param);
static int DasmToFile(char *param);
static int DisplayMemory(char *param);
static int DisplayCode(char *param);
static int EditMemory(char *param);
static int FastDebug(char *param);
static int TraceToFile(char *param);	/* JB 980214 */
static int SetWatchPoint(char *param);	/* EHC 980506 */
static int ClearWatchPoint(char *param);	/* EHC 980506 */

/* private functions */
static void DrawMemWindow (int Base, int Col, int Offset, int DisplayASCII);	/* MB 980103 */
static int  GetNumber (int X, int Y, int Col);
static int  ScreenEdit (int XMin, int XMax, int YMin, int YMax, int Col, int BaseAdd, int *DisplayASCII);	/* MB 980103 */
static int  debug_draw_disasm (int pc);
static int  debug_dasm_line (int, char *, int);
static void debug_draw_flags (void);
static void debug_draw_registers (void);

static void EditRegisters (int);
static int	IsRegister(int cputype, char *);
static unsigned long GetAddress(int cputype, char *src);

/* globals */
static unsigned char    rgs[CPU_CONTEXT_SIZE];	/* ASG 971105 */
static unsigned char    bckrgs[CPU_CONTEXT_SIZE];	/* MB 980221 */


typedef struct
{
	char 	*Name;
	int		*Val;
	int		Size;
	int		XPos;
	int		YPos;
} tRegdef;

typedef struct
{
	char	*Name;
	int		NamePos;
	void	(*DrawDebugScreen)(int, int);
	int		(*Dasm)(char *, int);
	void	(*Trace)(int);	/* JB 980214 */
	int		DasmLineLen;
	int		DasmStartX;
	char	*Flags;
	int		*CC;
	int		FlagSize;
	char	*AddPrint;
	int		AddMask;
	int		MemWindowAddX;
	int		MemWindowDataX;
	int		MemWindowDataXEnd;
	int		MemWindowNumBytes;
	int		MemWindowDataSpace; /* 1 + data character space - ie, 3 for byte, 5 for word, 9 for dword */
	int		MaxInstLen;
	int		AlignUnit;			/* CM 980428 */
	int		*SPReg;
	int		SPSize;
	tRegdef	RegList[48];
} tDebugCpuInfo;

typedef struct
{
	tRegdef RegList[48];
} tBackupReg;



/* LEAVE cmNOMORE as last Command! */
enum { cmBPX, cmBC, cmD, cmDASM, cmDUMP, cmE, cmF, cmG, cmHERE, cmJ, cmR, cmTRACE, cmBPW, cmWC, cmNOMORE };

typedef struct
{
	int 	Cmd;
	char	*Name;
	char	*Description;
	int		(*ExecuteCommand)(char *);
} tCommands;


static tCommands CommandInfo[] =
{
	{	cmBPX,	"BPX ",		"bpx [address]", SetBreakPoint },
	{	cmBC,	"BC ",		"Breakpoint clear", ClearBreakPoint },
	{	cmD,	"D ",		"Display <1|2> [Address]", DisplayMemory },
	{	cmDASM,	"DASM ",	"Dasm <FileName> <StartAddr> <EndAddr>", DasmToFile },
	{	cmDUMP,	"DUMP ",	"Dump <FileName> <StartAddr> <EndAddr>", DumpToFile },
	{   cmE,	"E ",		"Edit <1|2> [address]", EditMemory },
	{   cmF,	"F ",		"Fast", FastDebug },
	{	cmG,	"G ", 		"Go <address>", Go },
	{	cmHERE,	"HERE ",	"Run to cursor", Here },
	{	cmJ,	"J ",		"Jump <Address>", DisplayCode },
	{	cmR,	"R ",		"r [register] = [register|value]", ModifyRegisters },
	{	cmTRACE,"TRACE ",	"Trace <FileName>|OFF", TraceToFile },	/* JB 980214 */
	{	cmBPW,	"BPW ",		"Set Watchpoint <Address>", SetWatchPoint },	/* EHC 980506 */
	{	cmWC,	"WC ",		"Clear Watchpoint", ClearWatchPoint },	/* EHC 980506 */
	{   cmNOMORE },
};

static tBackupReg BackupRegisters[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		{
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_Z80    1 */
	{
		{
			{ "A", (int *)&((Z80_Regs *)bckrgs)->AF.b.h, 1, 2, 1 },
			{ "HL", (int *)&((Z80_Regs *)bckrgs)->HL.w.l, 2, 8, 1 },
			{ "DE", (int *)&((Z80_Regs *)bckrgs)->DE.w.l, 2, 16, 1 },
			{ "BC", (int *)&((Z80_Regs *)bckrgs)->BC.w.l, 2, 24, 1 },
			{ "PC", (int *)&((Z80_Regs *)bckrgs)->PC.w.l, 2, 32, 1 },
			{ "SP", (int *)&((Z80_Regs *)bckrgs)->SP.w.l, 2, 40, 1 },
			{ "IX", (int *)&((Z80_Regs *)bckrgs)->IX.w.l, 2, 48, 1 },
			{ "IY", (int *)&((Z80_Regs *)bckrgs)->IY.w.l, 2, 56, 1 },
			{ "IFF1", (int *)&((Z80_Regs *)bckrgs)->IFF1, 1, 64, 1 },
			{ "IFF2", (int *)&((Z80_Regs *)bckrgs)->IFF2, 1, 72, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_8080  2 */
	{
			{
					{ "AF", (int *)&((i8085_Regs *)bckrgs)->AF.w.l, 2, 2, 1 },
					{ "HL", (int *)&((i8085_Regs *)bckrgs)->HL.w.l, 2, 10, 1 },
					{ "DE", (int *)&((i8085_Regs *)bckrgs)->DE.w.l, 2, 18, 1 },
					{ "BC", (int *)&((i8085_Regs *)bckrgs)->BC.w.l, 2, 26, 1 },
					{ "PC", (int *)&((i8085_Regs *)bckrgs)->PC.w.l, 2, 34, 1 },
					{ "SP", (int *)&((i8085_Regs *)bckrgs)->PC.w.l, 2, 42, 1 },
					{ "IM", (int *)&((i8085_Regs *)bckrgs)->IM, 1, 50, 1 },
					{ "", (int *)-1, -1, -1, -1 }
			},
    },
	/* #define CPU_8085  3 */
	{
			{
					{ "AF", (int *)&((i8085_Regs *)bckrgs)->AF.w.l, 2, 2, 1 },
					{ "HL", (int *)&((i8085_Regs *)bckrgs)->HL.w.l, 2, 10, 1 },
					{ "DE", (int *)&((i8085_Regs *)bckrgs)->DE.w.l, 2, 18, 1 },
					{ "BC", (int *)&((i8085_Regs *)bckrgs)->BC.w.l, 2, 26, 1 },
					{ "PC", (int *)&((i8085_Regs *)bckrgs)->PC.w.l, 2, 34, 1 },
					{ "SP", (int *)&((i8085_Regs *)bckrgs)->PC.w.l, 2, 42, 1 },
					{ "IM", (int *)&((i8085_Regs *)bckrgs)->IM, 1, 50, 1 },
					{ "", (int *)-1, -1, -1, -1 }
			},
	},
	/* #define CPU_M6502  4 */
	{
		{
			{ "A", (int *)&((m6502_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "X", (int *)&((m6502_Regs *)bckrgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((m6502_Regs *)bckrgs)->y, 1, 12, 1 },
			{ "S", (int *)&((m6502_Regs *)bckrgs)->sp.d, 1, 17, 1 },
			{ "PC", (int *)&((m6502_Regs *)bckrgs)->pc.w.l, 2, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M65C02 5 */
	{
		{
			{ "A", (int *)&((m6502_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "X", (int *)&((m6502_Regs *)bckrgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((m6502_Regs *)bckrgs)->y, 1, 12, 1 },
			{ "S", (int *)&((m6502_Regs *)bckrgs)->sp.d, 1, 17, 1 },
			{ "PC", (int *)&((m6502_Regs *)bckrgs)->pc.w.l, 2, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6510  6 */
	{
		{
			{ "A", (int *)&((m6502_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "X", (int *)&((m6502_Regs *)bckrgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((m6502_Regs *)bckrgs)->y, 1, 12, 1 },
			{ "S", (int *)&((m6502_Regs *)bckrgs)->sp.d, 1, 17, 1 },
			{ "PC", (int *)&((m6502_Regs *)bckrgs)->pc.w.l, 2, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_H6280  7 */
	{
		{
			{ "A", (int *)&((h6280_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "X", (int *)&((h6280_Regs *)bckrgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((h6280_Regs *)bckrgs)->y, 1, 12, 1 },
			{ "S", (int *)&((h6280_Regs *)bckrgs)->sp.b.l, 1, 17, 1 },
			{ "PC", (int *)&((h6280_Regs *)bckrgs)->pc.w.l, 2, 22, 1 },
			{ "M1", (int *)&((h6280_Regs *)rgs)->mmr[0], 1, 30, 1 },
			{ "M2", (int *)&((h6280_Regs *)rgs)->mmr[1], 1, 36, 1 },
			{ "M3", (int *)&((h6280_Regs *)rgs)->mmr[2], 1, 42, 1 },
			{ "M4", (int *)&((h6280_Regs *)rgs)->mmr[3], 1, 48, 1 },
			{ "M5", (int *)&((h6280_Regs *)rgs)->mmr[4], 1, 54, 1 },
			{ "M6", (int *)&((h6280_Regs *)rgs)->mmr[5], 1, 60, 1 },
			{ "M7", (int *)&((h6280_Regs *)rgs)->mmr[6], 1, 66, 1 },
			{ "M8", (int *)&((h6280_Regs *)rgs)->mmr[7], 1, 72, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_I86	  8 */
	{
		{
			{ "IP", (int *)&((i86_Regs *)bckrgs)->ip, 4, 2, 1 },
			{ "ES", (int *)&((i86_Regs *)bckrgs)->sregs[0], 2, 14, 1 },
			{ "CS", (int *)&((i86_Regs *)bckrgs)->sregs[1], 2, 22, 1 },
			{ "SS", (int *)&((i86_Regs *)bckrgs)->sregs[2], 2, 30, 1 },
			{ "DS", (int *)&((i86_Regs *)bckrgs)->sregs[3], 2, 38, 1 },
			{ "AX", (int *)&((i86_Regs *)bckrgs)->regs.w[0], 2, 67, 3 },
			{ "CX", (int *)&((i86_Regs *)bckrgs)->regs.w[1], 2, 67, 4 },
			{ "DX", (int *)&((i86_Regs *)bckrgs)->regs.w[2], 2, 67, 5 },
			{ "BX", (int *)&((i86_Regs *)bckrgs)->regs.w[3], 2, 67, 6 },
			{ "SP", (int *)&((i86_Regs *)bckrgs)->regs.w[4], 2, 67, 7 },
			{ "BP", (int *)&((i86_Regs *)bckrgs)->regs.w[5], 2, 67, 8 },
			{ "SI", (int *)&((i86_Regs *)bckrgs)->regs.w[6], 2, 67, 9 },
			{ "DI", (int *)&((i86_Regs *)bckrgs)->regs.w[7], 2, 67, 10 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_I8035  9 */
	{
		{
			{ "PC", (int *)&((I8039_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)bckrgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)bckrgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_I8039 10 */
	{
		{
			{ "PC", (int *)&((I8039_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)bckrgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)bckrgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_I8048 11 */
	{
		{
			{ "PC", (int *)&((I8039_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)bckrgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)bckrgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_N7751 12 */
	{
		{
			{ "PC", (int *)&((I8039_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)bckrgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)bckrgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6800 13 */
	{
		{
			{ "A", (int *)&((m6802_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6802_Regs *)bckrgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6802_Regs *)bckrgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6802_Regs *)bckrgs)->s, 2, 20, 1 },
			{ "X", (int *)&((m6802_Regs *)bckrgs)->x, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6802 14 */
	{
		{
			{ "A", (int *)&((m6802_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6802_Regs *)bckrgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6802_Regs *)bckrgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6802_Regs *)bckrgs)->s, 2, 20, 1 },
			{ "X", (int *)&((m6802_Regs *)bckrgs)->x, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6803 15 */
	{
		{
			{ "A", (int *)&((m6803_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6803_Regs *)bckrgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6803_Regs *)bckrgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6803_Regs *)bckrgs)->s, 2, 20, 1 },
			{ "X", (int *)&((m6803_Regs *)bckrgs)->x, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M6808 16 */
	{
		{
			{ "A", (int *)&((m6808_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6808_Regs *)bckrgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6808_Regs *)bckrgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6808_Regs *)bckrgs)->s, 2, 20, 1 },
			{ "X", (int *)&((m6808_Regs *)bckrgs)->x, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_HD63701 17 */
	{
		{
			{ "A", (int *)&((hd63701_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((hd63701_Regs *)bckrgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((hd63701_Regs *)bckrgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((hd63701_Regs *)bckrgs)->s, 2, 20, 1 },
			{ "X", (int *)&((hd63701_Regs *)bckrgs)->x, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_6805 18 */	 /* JB 980214 */
	{
		{
			{ "A", (int *)&((m6805_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "PC", (int *)&((m6805_Regs *)bckrgs)->pc, 2, 7, 1 },
			{ "S", (int *)&((m6805_Regs *)bckrgs)->s, 2, 15, 1 },
			{ "X", (int *)&((m6805_Regs *)bckrgs)->x, 1, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_68705 19 */	 /* JB 980214 */
	{
		{
			{ "A", (int *)&((m6805_Regs *)bckrgs)->a, 1, 2, 1 },
			{ "PC", (int *)&((m6805_Regs *)bckrgs)->pc, 2, 7, 1 },
			{ "S", (int *)&((m6805_Regs *)bckrgs)->s, 2, 15, 1 },
			{ "X", (int *)&((m6805_Regs *)bckrgs)->x, 1, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6309 20 */
	{
		{
			{ "A", (int *)&((m6809_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6809_Regs *)bckrgs)->d.b.l, 1, 8, 1 },
			{ "PC", (int *)&((m6809_Regs *)bckrgs)->pc, 2, 14, 1 },
			{ "S", (int *)&((m6809_Regs *)bckrgs)->s.w.l, 2, 23, 1 },
			{ "U", (int *)&((m6809_Regs *)bckrgs)->u.w.l, 2, 31, 1 },
			{ "X", (int *)&((m6809_Regs *)bckrgs)->x.w.l, 2, 39, 1 },
			{ "Y", (int *)&((m6809_Regs *)bckrgs)->y.w.l, 2, 47, 1 },
			{ "DP", (int *)&((m6809_Regs *)bckrgs)->dp, 1, 55, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6809 21 */
	{
		{
			{ "A", (int *)&((m6809_Regs *)bckrgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6809_Regs *)bckrgs)->d.b.l, 1, 8, 1 },
			{ "PC", (int *)&((m6809_Regs *)bckrgs)->pc, 2, 14, 1 },
			{ "S", (int *)&((m6809_Regs *)bckrgs)->s.w.l, 2, 23, 1 },
			{ "U", (int *)&((m6809_Regs *)bckrgs)->u.w.l, 2, 31, 1 },
			{ "X", (int *)&((m6809_Regs *)bckrgs)->x.w.l, 2, 39, 1 },
			{ "Y", (int *)&((m6809_Regs *)bckrgs)->y.w.l, 2, 47, 1 },
			{ "DP", (int *)&((m6809_Regs *)bckrgs)->dp, 1, 55, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M68000 22 */
	{
		{
			{ "PC", (int *)&((MC68000_Regs *)bckrgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)bckrgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)bckrgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)bckrgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)bckrgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)bckrgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)bckrgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)bckrgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)bckrgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)bckrgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)bckrgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)bckrgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)bckrgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)bckrgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)bckrgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)bckrgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)bckrgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)bckrgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)bckrgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)bckrgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)bckrgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)bckrgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M68000 23 */
	{
		{
			{ "PC", (int *)&((MC68000_Regs *)bckrgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)bckrgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)bckrgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)bckrgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)bckrgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)bckrgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)bckrgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)bckrgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)bckrgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)bckrgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)bckrgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)bckrgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)bckrgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)bckrgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)bckrgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)bckrgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)bckrgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)bckrgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)bckrgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)bckrgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)bckrgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)bckrgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M68000 24 */
	{
		{
			{ "PC", (int *)&((MC68000_Regs *)bckrgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)bckrgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)bckrgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)bckrgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)bckrgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)bckrgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)bckrgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)bckrgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)bckrgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)bckrgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)bckrgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)bckrgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)bckrgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)bckrgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)bckrgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)bckrgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)bckrgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)bckrgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)bckrgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)bckrgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)bckrgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)bckrgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_T11 25 */
	{
		{
			{ "R0", (int *)&((t11_Regs *)bckrgs)->reg[0], 2, 2, 1 },
			{ "R1", (int *)&((t11_Regs *)bckrgs)->reg[1], 2, 10, 1 },
			{ "R2", (int *)&((t11_Regs *)bckrgs)->reg[2], 2, 18, 1 },
			{ "R3", (int *)&((t11_Regs *)bckrgs)->reg[3], 2, 26, 1 },
			{ "R4", (int *)&((t11_Regs *)bckrgs)->reg[4], 2, 34, 1 },
			{ "R5", (int *)&((t11_Regs *)bckrgs)->reg[5], 2, 42, 1 },
			{ "SP", (int *)&((t11_Regs *)bckrgs)->reg[6], 2, 50, 1 },
			{ "PC", (int *)&((t11_Regs *)bckrgs)->reg[7], 2, 58, 1 },
			{ "PSW", (int *)&((t11_Regs *)bckrgs)->psw, 2, 66, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_2650 26 */
	{
		{
			{ "R0", (int *)&((s2650_Regs *)bckrgs)->reg[0], 1, 2, 1 },
			{ "R1", (int *)&((s2650_Regs *)bckrgs)->reg[1], 1, 8, 1 },
			{ "R2", (int *)&((s2650_Regs *)bckrgs)->reg[2], 1, 14, 1 },
			{ "R3", (int *)&((s2650_Regs *)bckrgs)->reg[3], 1, 20, 1 },
			{ "IAR", (int *)&((s2650_Regs *)bckrgs)->iar, 2, 26, 1 },
			{ " ", (int *)&((s2650_Regs *)bckrgs)->ir, 1, 34, 1 },
			{ "PSL", (int *)&((s2650_Regs *)bckrgs)->psl, 1, 39, 1 },
			{ "PSU", (int *)&((s2650_Regs *)bckrgs)->psu, 1, 46, 1 },
			{ "EA", (int *)&((s2650_Regs *)bckrgs)->ea, 2, 53, 1 },
			{ "r1", (int *)&((s2650_Regs *)bckrgs)->reg[4], 1, 61, 1 },
			{ "r2", (int *)&((s2650_Regs *)bckrgs)->reg[5], 1, 67, 1 },
			{ "r3", (int *)&((s2650_Regs *)bckrgs)->reg[6], 1, 73, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS34010 27 */
	{
		{
			{ "PC", (int *)&((TMS34010_Regs *)bckrgs)->pc, 4, 2, 3 },
			{ "SP", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[15], 4, 55, 1 },
			{ "A0", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[0], 4, 55, 4 },
			{ "A1", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[1], 4, 55, 5 },
			{ "A2", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[2], 4, 55, 6 },
			{ "A3", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[3], 4, 55, 7 },
			{ "A4", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[4], 4, 55, 8 },
			{ "A5", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[5], 4, 55, 9 },
			{ "A6", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[6], 4, 55, 10 },
			{ "A7", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[7], 4, 55, 11 },
			{ "A8", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[8], 4, 55, 12 },
			{ "A9", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[9], 4, 55, 13 },
			{ "A10", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[10], 4, 54, 14 },
			{ "A11", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[11], 4, 54, 15 },
			{ "A12", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[12], 4, 54, 16 },
			{ "A13", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[13], 4, 54, 17 },
			{ "A14", (int *)&((TMS34010_Regs *)bckrgs)->regs.a.Aregs[14], 4, 54, 18 },
			{ "B0", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[0<<4], 4, 68, 4 },
			{ "B1", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[1<<4], 4, 68, 5 },
			{ "B2", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[2<<4], 4, 68, 6 },
			{ "B3", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[3<<4], 4, 68, 7 },
			{ "B4", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[4<<4], 4, 68, 8 },
			{ "B5", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[5<<4], 4, 68, 9 },
			{ "B6", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[6<<4], 4, 68, 10 },
			{ "B7", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[7<<4], 4, 68, 11 },
			{ "B8", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[8<<4], 4, 68, 12 },
			{ "B9", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[9<<4], 4, 68, 13 },
			{ "B10", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[10<<4], 4, 67, 14 },
			{ "B11", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[11<<4], 4, 67, 15 },
			{ "B12", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[12<<4], 4, 67, 16 },
			{ "B13", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[13<<4], 4, 67, 17 },
			{ "B14", (int *)&((TMS34010_Regs *)bckrgs)->regs.Bregs[14<<4], 4, 67, 18 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS9900 28 */
	{
		{
			{ "PC", (int *)&((TMS9900_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "IR", (int *)&((TMS9900_Regs *)bckrgs)->IR, 2, 10, 1 },
			{ "WP", (int *)&((TMS9900_Regs *)bckrgs)->WP, 2, 18, 1 },
			{ "ST", (int *)&((TMS9900_Regs *)bckrgs)->STATUS, 2, 26, 1 },
			{ "R0", (int *)&((TMS9900_Regs *)bckrgs)->FR0, 2, 69, 4 },
			{ "R1", (int *)&((TMS9900_Regs *)bckrgs)->FR1, 2, 69, 5 },
			{ "R2", (int *)&((TMS9900_Regs *)bckrgs)->FR2, 2, 69, 6 },
			{ "R3", (int *)&((TMS9900_Regs *)bckrgs)->FR3, 2, 69, 7 },
			{ "R4", (int *)&((TMS9900_Regs *)bckrgs)->FR4, 2, 69, 7 },
			{ "R5", (int *)&((TMS9900_Regs *)bckrgs)->FR5, 2, 69, 8 },
			{ "R6", (int *)&((TMS9900_Regs *)bckrgs)->FR6, 2, 69, 9 },
			{ "R7", (int *)&((TMS9900_Regs *)bckrgs)->FR7, 2, 69, 10 },
			{ "R8", (int *)&((TMS9900_Regs *)bckrgs)->FR8, 2, 69, 11 },
			{ "R9", (int *)&((TMS9900_Regs *)bckrgs)->FR9, 2, 69, 12 },
			{ "R10", (int *)&((TMS9900_Regs *)bckrgs)->FR10, 2, 68, 13 },
			{ "R11", (int *)&((TMS9900_Regs *)bckrgs)->FR11, 2, 68, 14 },
			{ "R12", (int *)&((TMS9900_Regs *)bckrgs)->FR12, 2, 68, 15 },
			{ "R13", (int *)&((TMS9900_Regs *)bckrgs)->FR13, 2, 68, 16 },
			{ "R14", (int *)&((TMS9900_Regs *)bckrgs)->FR14, 2, 68, 17 },
			{ "R15", (int *)&((TMS9900_Regs *)bckrgs)->FR15, 2, 68, 18 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_Z8000 29 */
	{
		{
			{ "PC", (int *)&((z8000_Regs *)bckrgs)->pc, 2, 2, 1 },
			{ "PSAP", (int *)&((z8000_Regs *)bckrgs)->psap, 2, 10, 1 },
			{ "REFRESH", (int *)&((z8000_Regs *)bckrgs)->refresh, 2, 20, 1 },
			{ "NSP", (int *)&((z8000_Regs *)bckrgs)->nsp, 2, 33, 1 },
			{ "IRQ", (int *)&((z8000_Regs *)bckrgs)->irq_req, 2, 43, 1 },
			{ "OP", (int *)&((z8000_Regs *)bckrgs)->op[0], 2, 53, 1 },
			{ " ", (int *)&((z8000_Regs *)bckrgs)->op[1], 2, 60, 1 },
			{ " ", (int *)&((z8000_Regs *)bckrgs)->op[2], 2, 63, 1 },
#ifdef	LSB_FIRST
#define REGWXOR 3
#else
#define REGWXOR 0
#endif
            { "R 0", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 0^REGWXOR], 2, 67, 3 },
			{ "R 1", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 1^REGWXOR], 2, 67, 4 },
			{ "R 2", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 2^REGWXOR], 2, 67, 5 },
			{ "R 3", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 3^REGWXOR], 2, 67, 6 },
			{ "R 4", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 4^REGWXOR], 2, 67, 7 },
			{ "R 5", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 5^REGWXOR], 2, 67, 8 },
			{ "R 6", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 6^REGWXOR], 2, 67, 9 },
			{ "R 7", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 7^REGWXOR], 2, 67, 10 },
			{ "R 8", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 8^REGWXOR], 2, 67, 12 },
			{ "R 9", (int *)&((z8000_Regs *)bckrgs)->regs.W[ 9^REGWXOR], 2, 67, 13 },
			{ "R10", (int *)&((z8000_Regs *)bckrgs)->regs.W[10^REGWXOR], 2, 67, 14 },
			{ "R11", (int *)&((z8000_Regs *)bckrgs)->regs.W[11^REGWXOR], 2, 67, 15 },
			{ "R12", (int *)&((z8000_Regs *)bckrgs)->regs.W[12^REGWXOR], 2, 67, 16 },
			{ "R13", (int *)&((z8000_Regs *)bckrgs)->regs.W[13^REGWXOR], 2, 67, 17 },
			{ "R14", (int *)&((z8000_Regs *)bckrgs)->regs.W[14^REGWXOR], 2, 67, 18 },
			{ "R15", (int *)&((z8000_Regs *)bckrgs)->regs.W[15^REGWXOR], 2, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS320C10 30 */
	{
		{
			{ "PC", (int *)&((TMS320C10_Regs *)bckrgs)->PC, 2, 2, 1 },
			{ "ACC", (int *)&((TMS320C10_Regs *)bckrgs)->ACC, 4, 10, 1 },
			{ "P", (int *)&((TMS320C10_Regs *)bckrgs)->Preg, 4, 23, 1 },
			{ "T", (int *)&((TMS320C10_Regs *)bckrgs)->Treg, 2, 34, 1 },
			{ "AR0", (int *)&((TMS320C10_Regs *)bckrgs)->AR[0], 2, 41, 1 },
			{ "AR1", (int *)&((TMS320C10_Regs *)bckrgs)->AR[1], 2, 50, 1 },
			{ "STR", (int *)&((TMS320C10_Regs *)bckrgs)->STR, 2, 59, 1 },
			{ "STAK3", (int *)&((TMS320C10_Regs *)bckrgs)->STACK[3], 2, 68, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* Dummy CPU -- placeholder for CCPU */
	{
		{
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
};


static tDebugCpuInfo DebugInfo[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		"Dummy", 14,
		DrawDebugScreen8,
		DummyDasm, DummyTrace, 0, 8,
		"........", (int *)&DummyCC, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,					/* CM 980428 */
		(int *)-1, -1,
		{
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_Z80    1 */
	{
		"Z80", 14,
		DrawDebugScreen8,
		TempDasmZ80, TempZ80Trace, 15, 8,	/* JB 980103 */
		"SZ.H.PNC", (int *)&((Z80_Regs *)rgs)->AF.b.l, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((Z80_Regs *)rgs)->SP.w.l, 2,
		{
			{ "A", (int *)&((Z80_Regs *)rgs)->AF.b.h, 1, 2, 1 },
			{ "HL", (int *)&((Z80_Regs *)rgs)->HL.w.l, 2, 8, 1 },
			{ "DE", (int *)&((Z80_Regs *)rgs)->DE.w.l, 2, 16, 1 },
			{ "BC", (int *)&((Z80_Regs *)rgs)->BC.w.l, 2, 24, 1 },
			{ "PC", (int *)&((Z80_Regs *)rgs)->PC.w.l, 2, 32, 1 },
			{ "SP", (int *)&((Z80_Regs *)rgs)->SP.w.l, 2, 40, 1 },
			{ "IX", (int *)&((Z80_Regs *)rgs)->IX.w.l, 2, 48, 1 },
			{ "IY", (int *)&((Z80_Regs *)rgs)->IY.w.l, 2, 56, 1 },
			{ "IFF1", (int *)&((Z80_Regs *)rgs)->IFF1, 1, 64, 1 },
			{ "IFF2", (int *)&((Z80_Regs *)rgs)->IFF1, 1, 72, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_8080  2 */
	{
		"8080", 14,
		DrawDebugScreen8,
		TempDasm8085, Temp8085Trace, 15, 8,
		"SZ.H.PNC", (int *)&((i8085_Regs *)rgs)->AF.b.l, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
				(int *)&((i8085_Regs *)rgs)->SP.w.l, 2,
		{
			{ "AF", (int *)&((i8085_Regs *)rgs)->AF.w.l, 2, 2, 1 },
			{ "HL", (int *)&((i8085_Regs *)rgs)->HL.w.l, 2, 10, 1 },
			{ "DE", (int *)&((i8085_Regs *)rgs)->DE.w.l, 2, 18, 1 },
			{ "BC", (int *)&((i8085_Regs *)rgs)->BC.w.l, 2, 26, 1 },
			{ "PC", (int *)&((i8085_Regs *)rgs)->PC.w.l, 2, 34, 1 },
			{ "SP", (int *)&((i8085_Regs *)rgs)->SP.w.l, 2, 42, 1 },
			{ "IM", (int *)&((i8085_Regs *)rgs)->IM, 1, 50, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_8085  3 */
	{
		"8085A", 14,
		DrawDebugScreen8,
		TempDasm8085, Temp8085Trace, 15, 8,
		"SZ.H.PNC", (int *)&((i8085_Regs *)rgs)->AF.b.l, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
				(int *)&((i8085_Regs *)rgs)->SP.w.l, 2,
		{
			{ "AF", (int *)&((i8085_Regs *)rgs)->AF.w.l, 2, 2, 1 },
			{ "HL", (int *)&((i8085_Regs *)rgs)->HL.w.l, 2, 10, 1 },
			{ "DE", (int *)&((i8085_Regs *)rgs)->DE.w.l, 2, 18, 1 },
			{ "BC", (int *)&((i8085_Regs *)rgs)->BC.w.l, 2, 26, 1 },
			{ "PC", (int *)&((i8085_Regs *)rgs)->PC.w.l, 2, 34, 1 },
			{ "SP", (int *)&((i8085_Regs *)rgs)->SP.w.l, 2, 42, 1 },
			{ "IM", (int *)&((i8085_Regs *)rgs)->IM, 1, 50, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M6502  4 */
	{
		"6502", 14,
		DrawDebugScreen8,
		Dasm6502, Temp6502Trace, 15, 8,
		"NVRBDIZC", (int *)&((m6502_Regs *)rgs)->p, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		3, 1,
		(int *)&((m6502_Regs *)rgs)->sp.b.l, 1,
		{
			{ "A", (int *)&((m6502_Regs *)rgs)->a, 1, 2, 1 },
			{ "X", (int *)&((m6502_Regs *)rgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((m6502_Regs *)rgs)->y, 1, 12, 1 },
			{ "S", (int *)&((m6502_Regs *)rgs)->sp.b.l, 1, 17, 1 },
			{ "PC", (int *)&((m6502_Regs *)rgs)->pc.w.l, 2, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M6502  5 */
	{
		"65C02", 14,
		DrawDebugScreen8,
		Dasm6502, Temp6502Trace, 15, 8,
		"NVRBDIZC", (int *)&((m6502_Regs *)rgs)->p, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		3, 1,
		(int *)&((m6502_Regs *)rgs)->sp.b.l, 1,
		{
			{ "A", (int *)&((m6502_Regs *)rgs)->a, 1, 2, 1 },
			{ "X", (int *)&((m6502_Regs *)rgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((m6502_Regs *)rgs)->y, 1, 12, 1 },
			{ "S", (int *)&((m6502_Regs *)rgs)->sp.b.l, 1, 17, 1 },
			{ "PC", (int *)&((m6502_Regs *)rgs)->pc.w.l, 2, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6502  6 */
    {
        "6510", 14,
        DrawDebugScreen8,
        Dasm6502, Temp6502Trace, 15, 8,
        "NVRBDIZC", (int *)&((m6502_Regs *)rgs)->p, 8,
        "%04X:", 0xffff,
                25, 31, 77, 16, 3,
        3, 1,
        (int *)&((m6502_Regs *)rgs)->sp.b.l, 1,
        {
            { "A", (int *)&((m6502_Regs *)rgs)->a, 1, 2, 1 },
            { "X", (int *)&((m6502_Regs *)rgs)->x, 1, 7, 1 },
            { "Y", (int *)&((m6502_Regs *)rgs)->y, 1, 12, 1 },
            { "S", (int *)&((m6502_Regs *)rgs)->sp.b.l, 1, 17, 1 },
            { "PC", (int *)&((m6502_Regs *)rgs)->pc.w.l, 2, 22, 1 },
            { "", (int *)-1, -1, -1, -1 }
        },
    },
    /* #define CPU_H6280  7 */
	{
		"6280", 14,
		DrawDebugScreen8,
		Dasm6280, Temp6280Trace, 15, 8,
		"NVRBDIZC", (int *)&((h6280_Regs *)rgs)->p, 8,
		"%04X:", 0x1fffff,
		25, 31, 77, 16, 3,
		3, 1,
		(int *)&((h6280_Regs *)rgs)->sp.b.l, 1,
		{
			{ "A", (int *)&((h6280_Regs *)rgs)->a, 1, 2, 1 },
			{ "X", (int *)&((h6280_Regs *)rgs)->x, 1, 7, 1 },
			{ "Y", (int *)&((h6280_Regs *)rgs)->y, 1, 12, 1 },
			{ "S", (int *)&((h6280_Regs *)rgs)->sp.b.l, 1, 17, 1 },
			{ "PC", (int *)&((h6280_Regs *)rgs)->pc.w.l, 2, 22, 1 },
			{ "M1", (int *)&((h6280_Regs *)rgs)->mmr[0], 1, 30, 1 },
			{ "M2", (int *)&((h6280_Regs *)rgs)->mmr[1], 1, 36, 1 },
			{ "M3", (int *)&((h6280_Regs *)rgs)->mmr[2], 1, 42, 1 },
			{ "M4", (int *)&((h6280_Regs *)rgs)->mmr[3], 1, 48, 1 },
			{ "M5", (int *)&((h6280_Regs *)rgs)->mmr[4], 1, 54, 1 },
			{ "M6", (int *)&((h6280_Regs *)rgs)->mmr[5], 1, 60, 1 },
			{ "M7", (int *)&((h6280_Regs *)rgs)->mmr[6], 1, 66, 1 },
			{ "M8", (int *)&((h6280_Regs *)rgs)->mmr[7], 1, 72, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_I86	  8 */
	{
		"I86", 21,
		DrawDebugScreen16,
		TempDasmI86, TempI86Trace,  20, 10,
		"....ODITSZ.A.P.C", (int *)&((i86_Regs *)rgs)->flags, 16,
		"%06X:     ", 0xffffff,
                32, 40, 62, 8, 3,
		5, 1,					/* CM 980428 */
		(int *)&((i86_Regs *)rgs)->regs.w[4], 2,
		{
			{ "IP", (int *)&((i86_Regs *)rgs)->ip, 4, 2, 1 },
			{ "ES", (int *)&((i86_Regs *)rgs)->sregs[0], 2, 14, 1 },
			{ "CS", (int *)&((i86_Regs *)rgs)->sregs[1], 2, 22, 1 },
			{ "SS", (int *)&((i86_Regs *)rgs)->sregs[2], 2, 30, 1 },
			{ "DS", (int *)&((i86_Regs *)rgs)->sregs[3], 2, 38, 1 },
			{ "AX", (int *)&((i86_Regs *)rgs)->regs.w[0], 2, 67, 3 },
			{ "CX", (int *)&((i86_Regs *)rgs)->regs.w[1], 2, 67, 4 },
			{ "DX", (int *)&((i86_Regs *)rgs)->regs.w[2], 2, 67, 5 },
			{ "BX", (int *)&((i86_Regs *)rgs)->regs.w[3], 2, 67, 6 },
			{ "SP", (int *)&((i86_Regs *)rgs)->regs.w[4], 2, 67, 7 },
			{ "BP", (int *)&((i86_Regs *)rgs)->regs.w[5], 2, 67, 8 },
			{ "SI", (int *)&((i86_Regs *)rgs)->regs.w[6], 2, 67, 9 },
			{ "DI", (int *)&((i86_Regs *)rgs)->regs.w[7], 2, 67, 10 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_I8035  9 */
	{
		"I8035", 14,
		DrawDebugScreen8,
		TempDasm8039, Temp8039Trace, 15, 8,
		"........", (int *)&((I8039_Regs *)rgs)->PSW, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,					/* CM 980428 */
		(int *)&((I8039_Regs *)rgs)->SP, 1,
		{
			{ "PC", (int *)&((I8039_Regs *)rgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)rgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)rgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_I8039  10 */
	{
		"I8039", 14,
		DrawDebugScreen8,
		TempDasm8039, Temp8039Trace, 15, 8,
		"........", (int *)&((I8039_Regs *)rgs)->PSW, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,					/* CM 980428 */
		(int *)&((I8039_Regs *)rgs)->SP, 1,
		{
			{ "PC", (int *)&((I8039_Regs *)rgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)rgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)rgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_I8048  11 */
	{
		"I8048", 14,
		DrawDebugScreen8,
		TempDasm8039, Temp8039Trace, 15, 8,
		"........", (int *)&((I8039_Regs *)rgs)->PSW, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,					/* CM 980428 */
		(int *)&((I8039_Regs *)rgs)->SP, 1,
		{
			{ "PC", (int *)&((I8039_Regs *)rgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)rgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)rgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_N7751  12 */
	{
		"N7751", 14,
		DrawDebugScreen8,
		TempDasm8039, Temp8039Trace, 15, 8,
		"........", (int *)&((I8039_Regs *)rgs)->PSW, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,					/* CM 980428 */
		(int *)&((I8039_Regs *)rgs)->SP, 1,
		{
			{ "PC", (int *)&((I8039_Regs *)rgs)->PC, 2, 2, 1 },
			{ "A", (int *)&((I8039_Regs *)rgs)->A, 1, 10, 1 },
			{ "SP", (int *)&((I8039_Regs *)rgs)->SP, 1, 15, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6800  13 */
	{
		"6800", 14,
		DrawDebugScreen8,
		TempDasm6800, Temp6800Trace, 15, 8,
		"..HINZVC", (int *)&((m6802_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6808_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6802_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6802_Regs *)rgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6802_Regs *)rgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6802_Regs *)rgs)->s.w.l, 2, 20, 1 },
			{ "X", (int *)&((m6802_Regs *)rgs)->x.w.l, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6802  14 */
	{
		"6802", 14,
		DrawDebugScreen8,
		TempDasm6802, Temp6802Trace, 15, 8,
		"..HINZVC", (int *)&((m6802_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6808_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6802_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6802_Regs *)rgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6802_Regs *)rgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6802_Regs *)rgs)->s.w.l, 2, 20, 1 },
			{ "X", (int *)&((m6802_Regs *)rgs)->x.w.l, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M6803  15 */
	{
		"6803", 14,
		DrawDebugScreen8,
		TempDasm6803, Temp6803Trace, 15, 8,
		"..HINZVC", (int *)&((m6808_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6803_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6803_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6803_Regs *)rgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6803_Regs *)rgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6803_Regs *)rgs)->s.w.l, 2, 20, 1 },
			{ "X", (int *)&((m6803_Regs *)rgs)->x.w.l, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6808  16 */
	{
		"6808", 14,
		DrawDebugScreen8,
		TempDasm6808, Temp6808Trace, 15, 8,
		"..HINZVC", (int *)&((m6808_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6808_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6808_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6808_Regs *)rgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((m6808_Regs *)rgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((m6808_Regs *)rgs)->s.w.l, 2, 20, 1 },
			{ "X", (int *)&((m6808_Regs *)rgs)->x.w.l, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_HD63701 17 */
	{
		"63701", 14,
		DrawDebugScreen8,
		TempDasm63701, Temp63701Trace, 15, 8,
		"..HINZVC", (int *)&((hd63701_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((hd63701_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((hd63701_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((hd63701_Regs *)rgs)->d.b.l, 1, 7, 1 },
			{ "PC", (int *)&((hd63701_Regs *)rgs)->pc, 2, 12, 1 },
			{ "S", (int *)&((hd63701_Regs *)rgs)->s.w.l, 2, 20, 1 },
			{ "X", (int *)&((hd63701_Regs *)rgs)->x.w.l, 2, 27, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_6805 18 */	 /* JB 980214 */
	{
		"6805", 14,
		DrawDebugScreen8,
		TempDasm6805, Temp6805Trace, 15, 8,
		"...HINZC",  (int *)&((m6805_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6805_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6805_Regs *)rgs)->a, 1, 2, 1 },
			{ "PC", (int *)&((m6805_Regs *)rgs)->pc, 2, 7, 1 },
			{ "S", (int *)&((m6805_Regs *)rgs)->s, 2, 15, 1 },
			{ "X", (int *)&((m6805_Regs *)rgs)->x, 1, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_68705 19 */	  /* JB 980214 */
	{
		"68705", 14,
		DrawDebugScreen8,
		TempDasm6805, Temp6805Trace, 15, 8,
		"...HINZC",  (int *)&((m6805_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		4, 1,					/* CM 980428 */
		(int *)&((m6805_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6805_Regs *)rgs)->a, 1, 2, 1 },
			{ "PC", (int *)&((m6805_Regs *)rgs)->pc, 2, 7, 1 },
			{ "S", (int *)&((m6805_Regs *)rgs)->s, 2, 15, 1 },
			{ "X", (int *)&((m6805_Regs *)rgs)->x, 1, 22, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6309  20 */
	{
		"6309", 14,
		DrawDebugScreen8,
		TempDasm6809, Temp6809Trace, 15, 8,
		"..H.NZVC", (int *)&((m6809_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		5, 1,					/* CM 980428 */
		(int *)&((m6809_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6809_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6809_Regs *)rgs)->d.b.l, 1, 8, 1 },
			{ "PC", (int *)&((m6809_Regs *)rgs)->pc.w.l, 2, 14, 1 },
			{ "S", (int *)&((m6809_Regs *)rgs)->s.w.l, 2, 23, 1 },
			{ "U", (int *)&((m6809_Regs *)rgs)->u.w.l, 2, 31, 1 },
			{ "X", (int *)&((m6809_Regs *)rgs)->x.w.l, 2, 39, 1 },
			{ "Y", (int *)&((m6809_Regs *)rgs)->y.w.l, 2, 47, 1 },
			{ "DP", (int *)&((m6809_Regs *)rgs)->dp, 1, 55, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M6809  21 */
	{
		"6809", 14,
		DrawDebugScreen8,
		TempDasm6809, Temp6809Trace, 15, 8,
		"..H.NZVC", (int *)&((m6809_Regs *)rgs)->cc, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		5, 1,					/* CM 980428 */
		(int *)&((m6809_Regs *)rgs)->s, 2,
		{
			{ "A", (int *)&((m6809_Regs *)rgs)->d.b.h, 1, 2, 1 },
			{ "B", (int *)&((m6809_Regs *)rgs)->d.b.l, 1, 8, 1 },
			{ "PC", (int *)&((m6809_Regs *)rgs)->pc.w.l, 2, 14, 1 },
			{ "S", (int *)&((m6809_Regs *)rgs)->s.w.l, 2, 23, 1 },
			{ "U", (int *)&((m6809_Regs *)rgs)->u.w.l, 2, 31, 1 },
			{ "X", (int *)&((m6809_Regs *)rgs)->x.w.l, 2, 39, 1 },
			{ "Y", (int *)&((m6809_Regs *)rgs)->y.w.l, 2, 47, 1 },
			{ "DP", (int *)&((m6809_Regs *)rgs)->dp, 1, 55, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M68000 22 */
	{
		"68K", 21,
		DrawDebugScreen16,
		TempDasm68000, Temp68000Trace, 21, 10,
		"T.S..III...XNZVC", (int *)&((MC68000_Regs *)rgs)->regs.sr, 16,
		"%06.6X:     ", 0xffffff,
                33, 41, 63, 8, 3,
		10,						/* CM 980428; "MOVE.W $12345678,$87654321" is 10 bytes*/
		2,						/* CM 980428; MC68000 instructions are evenly aligned */
		(int *)&((MC68000_Regs *)rgs)->regs.isp, 4,
		{
			{ "PC", (int *)&((MC68000_Regs *)rgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)rgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)rgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)rgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)rgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)rgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)rgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)rgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)rgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)rgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)rgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)rgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)rgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)rgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)rgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)rgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)rgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)rgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)rgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)rgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)rgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)rgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_M68010 23 */
	{
		"68010", 21,
		DrawDebugScreen16,
		TempDasm68000, Temp68000Trace, 21, 10,
		"T.S..III...XNZVC", (int *)&((MC68000_Regs *)rgs)->regs.sr, 16,
		"%06.6X:     ", 0xffffff,
                33, 41, 63, 8, 3,
		10,						/* CM 980428; "MOVE.W $12345678,$87654321" is 10 bytes*/
		2,						/* CM 980428; MC68000 instructions are evenly aligned */
		(int *)&((MC68000_Regs *)rgs)->regs.isp, 4,
		{
			{ "PC", (int *)&((MC68000_Regs *)rgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)rgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)rgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)rgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)rgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)rgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)rgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)rgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)rgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)rgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)rgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)rgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)rgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)rgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)rgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)rgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)rgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)rgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)rgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)rgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)rgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)rgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_M68020 24 */
	{
		"68020", 21,
		DrawDebugScreen16,
		TempDasm68000, Temp68000Trace, 21, 10,
		"T.S..III...XNZVC", (int *)&((MC68000_Regs *)rgs)->regs.sr, 16,
		"%06.6X:     ", 0xffffff,
                33, 41, 63, 8, 3,
		10,						/* CM 980428; "MOVE.W $12345678,$87654321" is 10 bytes*/
		2,						/* CM 980428; MC68000 instructions are evenly aligned */
		(int *)&((MC68000_Regs *)rgs)->regs.isp, 4,
		{
			{ "PC", (int *)&((MC68000_Regs *)rgs)->regs.pc, 4, 2, 1 },
			{ "VBR", (int *)&((MC68000_Regs *)rgs)->regs.vbr, 4, 14, 1 },
			{ "ISP", (int *)&((MC68000_Regs *)rgs)->regs.isp, 4, 27, 1 },
			{ "USP", (int *)&((MC68000_Regs *)rgs)->regs.usp, 4, 40, 1 },
			{ "SFC", (int *)&((MC68000_Regs *)rgs)->regs.sfc, 4, 53, 1 },
			{ "DFC", (int *)&((MC68000_Regs *)rgs)->regs.dfc, 4, 66, 1 },
			{ "D0", (int *)&((MC68000_Regs *)rgs)->regs.d[0], 4, 67, 3 },
			{ "D1", (int *)&((MC68000_Regs *)rgs)->regs.d[1], 4, 67, 4 },
			{ "D2", (int *)&((MC68000_Regs *)rgs)->regs.d[2], 4, 67, 5 },
			{ "D3", (int *)&((MC68000_Regs *)rgs)->regs.d[3], 4, 67, 6 },
			{ "D4", (int *)&((MC68000_Regs *)rgs)->regs.d[4], 4, 67, 7 },
			{ "D5", (int *)&((MC68000_Regs *)rgs)->regs.d[5], 4, 67, 8 },
			{ "D6", (int *)&((MC68000_Regs *)rgs)->regs.d[6], 4, 67, 9 },
			{ "D7", (int *)&((MC68000_Regs *)rgs)->regs.d[7], 4, 67, 10 },
			{ "A0", (int *)&((MC68000_Regs *)rgs)->regs.a[0], 4, 67, 12 },
			{ "A1", (int *)&((MC68000_Regs *)rgs)->regs.a[1], 4, 67, 13 },
			{ "A2", (int *)&((MC68000_Regs *)rgs)->regs.a[2], 4, 67, 14 },
			{ "A3", (int *)&((MC68000_Regs *)rgs)->regs.a[3], 4, 67, 15 },
			{ "A4", (int *)&((MC68000_Regs *)rgs)->regs.a[4], 4, 67, 16 },
			{ "A5", (int *)&((MC68000_Regs *)rgs)->regs.a[5], 4, 67, 17 },
			{ "A6", (int *)&((MC68000_Regs *)rgs)->regs.a[6], 4, 67, 18 },
			{ "A7", (int *)&((MC68000_Regs *)rgs)->regs.a[7], 4, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
	/* #define CPU_T11	  25 */
	{
		"T11", 14,
		DrawDebugScreen8,
		TempDasmT11, TempT11Trace, 15, 8,	/* JB 980103 */
		".IITNZVC", (int *)&((t11_Regs *)rgs)->psw.b.l, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		6, 2,					/* CM 980428 */
		(int *)&((t11_Regs *)rgs)->reg[6].w.l, 2,
		{
			{ "R0", (int *)&((t11_Regs *)rgs)->reg[0].w.l, 2, 2, 1 },
			{ "R1", (int *)&((t11_Regs *)rgs)->reg[1].w.l, 2, 10, 1 },
			{ "R2", (int *)&((t11_Regs *)rgs)->reg[2].w.l, 2, 18, 1 },
			{ "R3", (int *)&((t11_Regs *)rgs)->reg[3].w.l, 2, 26, 1 },
			{ "R4", (int *)&((t11_Regs *)rgs)->reg[4].w.l, 2, 34, 1 },
			{ "R5", (int *)&((t11_Regs *)rgs)->reg[5].w.l, 2, 42, 1 },
			{ "SP", (int *)&((t11_Regs *)rgs)->reg[6].w.l, 2, 50, 1 },
			{ "PC", (int *)&((t11_Regs *)rgs)->reg[7].w.l, 2, 58, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_2650   26 */
	{
		"2650", 14,
		DrawDebugScreen8,
		TempDasm2650, Temp2650Trace, 15, 8, /* JB 980103 */
		"MPHRWV?C", (int *)&((s2650_Regs *)rgs)->psl, 8,
		"%04X:", 0x7fff,
                25, 31, 77, 16, 3,
		6, 2,					/* CM 980428 */
		(int *)&((s2650_Regs *)rgs)->psu, 1,
		{
			{ "R0", (int *)&((s2650_Regs *)rgs)->reg[0], 1, 2, 1 },
			{ "R1", (int *)&((s2650_Regs *)rgs)->reg[1], 1, 8, 1 },
			{ "R2", (int *)&((s2650_Regs *)rgs)->reg[2], 1, 14, 1 },
			{ "R3", (int *)&((s2650_Regs *)rgs)->reg[3], 1, 20, 1 },
			{ "IAR", (int *)&((s2650_Regs *)rgs)->iar, 2, 26, 1 },
			{ "PSL", (int *)&((s2650_Regs *)rgs)->psl, 1, 34, 1 },
			{ "PSU", (int *)&((s2650_Regs *)rgs)->psu, 1, 41, 1 },
			{ "EA", (int *)&((s2650_Regs *)rgs)->ea, 2, 48, 1 },
			{ "R1'", (int *)&((s2650_Regs *)rgs)->reg[4], 1, 55, 1 },
			{ "R2'", (int *)&((s2650_Regs *)rgs)->reg[5], 1, 63, 1 },
			{ "R3'", (int *)&((s2650_Regs *)rgs)->reg[6], 1, 70, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS34010 27 */
	{
		"34010", 21,
		DrawDebugScreen32,
		TempDasm34010, Temp34010Trace, 20, 11,
		"NCZV..P...I.........EFFFFFEFFFFF", (int *)&((TMS34010_Regs *)rgs)->st, 32,
		"%08.8X:     ", 0xffffffff,
                32, 41, 52, 4, 3,
		0x50,						/* max inst length */
		0x10,						/* instructions are evenly aligned */
		(int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[15], 4,
		{
			{ "PC", (int *)&((TMS34010_Regs *)rgs)->pc, 4, 2, 3 },
			{ "SP", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[15], 4, 55, 1 },
			{ "A0", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[0], 4, 55, 4 },
			{ "A1", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[1], 4, 55, 5 },
			{ "A2", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[2], 4, 55, 6 },
			{ "A3", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[3], 4, 55, 7 },
			{ "A4", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[4], 4, 55, 8 },
			{ "A5", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[5], 4, 55, 9 },
			{ "A6", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[6], 4, 55, 10 },
			{ "A7", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[7], 4, 55, 11 },
			{ "A8", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[8], 4, 55, 12 },
			{ "A9", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[9], 4, 55, 13 },
			{ "A10", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[10], 4, 54, 14 },
			{ "A11", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[11], 4, 54, 15 },
			{ "A12", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[12], 4, 54, 16 },
			{ "A13", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[13], 4, 54, 17 },
			{ "A14", (int *)&((TMS34010_Regs *)rgs)->regs.a.Aregs[14], 4, 54, 18 },
			{ "B0", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[0<<4], 4, 68, 4 },
			{ "B1", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[1<<4], 4, 68, 5 },
			{ "B2", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[2<<4], 4, 68, 6 },
			{ "B3", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[3<<4], 4, 68, 7 },
			{ "B4", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[4<<4], 4, 68, 8 },
			{ "B5", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[5<<4], 4, 68, 9 },
			{ "B6", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[6<<4], 4, 68, 10 },
			{ "B7", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[7<<4], 4, 68, 11 },
			{ "B8", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[8<<4], 4, 68, 12 },
			{ "B9", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[9<<4], 4, 68, 13 },
			{ "B10", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[10<<4], 4, 67, 14 },
			{ "B11", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[11<<4], 4, 67, 15 },
			{ "B12", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[12<<4], 4, 67, 16 },
			{ "B13", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[13<<4], 4, 67, 17 },
			{ "B14", (int *)&((TMS34010_Regs *)rgs)->regs.Bregs[14<<4], 4, 67, 18 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS9900	 28 */
	{
		"9900", 21,
		DrawDebugScreen16,
		TempDasm9900, Temp9900Trace, 22, 8, /* JB 980103 */
		"LAECVPX.....IIII", (int *)&((TMS9900_Regs *)rgs)->STATUS, 16,
		"%04X:", 0xffff,
                33, 39, 61, 8, 3,
		6, 2,					/* CM 980428 */
		(int *)&((TMS9900_Regs *)rgs)->WP, 2,
		{
			{ "PC", (int *)&((TMS9900_Regs *)rgs)->PC,     2, 2, 1 },
			{ "IR", (int *)&((TMS9900_Regs *)rgs)->IR,     2, 10, 1 },
			{ "WP", (int *)&((TMS9900_Regs *)rgs)->WP,     2, 18, 1 },
			{ "ST", (int *)&((TMS9900_Regs *)rgs)->STATUS, 2, 26, 1 },
			{ "R0", (int *)&((TMS9900_Regs *)rgs)->FR0, 2, 69, 3 },
			{ "R1", (int *)&((TMS9900_Regs *)rgs)->FR1, 2, 69, 4 },
			{ "R2", (int *)&((TMS9900_Regs *)rgs)->FR2, 2, 69, 5 },
			{ "R3", (int *)&((TMS9900_Regs *)rgs)->FR3, 2, 69, 6 },
			{ "R4", (int *)&((TMS9900_Regs *)rgs)->FR4, 2, 69, 7 },
			{ "R5", (int *)&((TMS9900_Regs *)rgs)->FR5, 2, 69, 8 },
			{ "R6", (int *)&((TMS9900_Regs *)rgs)->FR6, 2, 69, 9 },
			{ "R7", (int *)&((TMS9900_Regs *)rgs)->FR7, 2, 69, 10 },
			{ "R8", (int *)&((TMS9900_Regs *)rgs)->FR8, 2, 69, 11 },
			{ "R9", (int *)&((TMS9900_Regs *)rgs)->FR9, 2, 69, 12 },
			{ "R10", (int *)&((TMS9900_Regs *)rgs)->FR10, 2, 68, 13 },
			{ "R11", (int *)&((TMS9900_Regs *)rgs)->FR11, 2, 68, 14 },
			{ "R12", (int *)&((TMS9900_Regs *)rgs)->FR12, 2, 68, 15 },
			{ "R13", (int *)&((TMS9900_Regs *)rgs)->FR13, 2, 68, 16 },
			{ "R14", (int *)&((TMS9900_Regs *)rgs)->FR14, 2, 68, 17 },
			{ "R15", (int *)&((TMS9900_Regs *)rgs)->FR15, 2, 68, 18 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_Z8000 29 */
	{
		"Z8000", 21,
		DrawDebugScreen16,
		TempDasmZ8000, TempZ8000Trace, 23, 8,
		"sne21...CZSVDH..", (int *)&((z8000_Regs *)rgs)->fcw, 16,
		"%04X:", 0xffff,
                33, 39, 61, 8, 3,
		6,			    /* max inst length */
		1,			    /* instructions are evenly aligned */
		(int *)&((z8000_Regs *)rgs)->nsp, 2,
		{
			{ "PC", (int *)&((z8000_Regs *)rgs)->pc, 2, 2, 1 },
			{ "PSAP", (int *)&((z8000_Regs *)rgs)->psap, 2, 10, 1 },
			{ "REFRESH", (int *)&((z8000_Regs *)rgs)->refresh, 2, 20, 1 },
			{ "NSP", (int *)&((z8000_Regs *)rgs)->nsp, 2, 33, 1 },
			{ "IRQ", (int *)&((z8000_Regs *)rgs)->irq_req, 2, 43, 1 },
			{ "OP", (int *)&((z8000_Regs *)rgs)->op[0], 2, 53, 1 },
			{ " ", (int *)&((z8000_Regs *)rgs)->op[1], 2, 60, 1 },
			{ " ", (int *)&((z8000_Regs *)rgs)->op[2], 2, 66, 1 },
			{ "R 0", (int *)&((z8000_Regs *)rgs)->regs.W[ 0^REGWXOR], 2, 67, 3 },
			{ "R 1", (int *)&((z8000_Regs *)rgs)->regs.W[ 1^REGWXOR], 2, 67, 4 },
			{ "R 2", (int *)&((z8000_Regs *)rgs)->regs.W[ 2^REGWXOR], 2, 67, 5 },
			{ "R 3", (int *)&((z8000_Regs *)rgs)->regs.W[ 3^REGWXOR], 2, 67, 6 },
			{ "R 4", (int *)&((z8000_Regs *)rgs)->regs.W[ 4^REGWXOR], 2, 67, 7 },
			{ "R 5", (int *)&((z8000_Regs *)rgs)->regs.W[ 5^REGWXOR], 2, 67, 8 },
			{ "R 6", (int *)&((z8000_Regs *)rgs)->regs.W[ 6^REGWXOR], 2, 67, 9 },
			{ "R 7", (int *)&((z8000_Regs *)rgs)->regs.W[ 7^REGWXOR], 2, 67, 10 },
			{ "R 8", (int *)&((z8000_Regs *)rgs)->regs.W[ 8^REGWXOR], 2, 67, 12 },
			{ "R 9", (int *)&((z8000_Regs *)rgs)->regs.W[ 9^REGWXOR], 2, 67, 13 },
			{ "R10", (int *)&((z8000_Regs *)rgs)->regs.W[10^REGWXOR], 2, 67, 14 },
			{ "R11", (int *)&((z8000_Regs *)rgs)->regs.W[11^REGWXOR], 2, 67, 15 },
			{ "R12", (int *)&((z8000_Regs *)rgs)->regs.W[12^REGWXOR], 2, 67, 16 },
			{ "R13", (int *)&((z8000_Regs *)rgs)->regs.W[13^REGWXOR], 2, 67, 17 },
			{ "R14", (int *)&((z8000_Regs *)rgs)->regs.W[14^REGWXOR], 2, 67, 18 },
			{ "R15", (int *)&((z8000_Regs *)rgs)->regs.W[15^REGWXOR], 2, 67, 19 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* #define CPU_TMS320C10  30 */
	{
		"32010", 21,
		DrawDebugScreen16word,
		TempDasm32010, Temp32010Trace, 22, 8,
		"OMI.arp1.....dp1", (int *)&((TMS320C10_Regs *)rgs)->STR, 16,
		"%04X:", 0xffff,
		33, 39, 77, 16, 5,
		4, 1,
		(int *)&((TMS320C10_Regs *)rgs)->STACK[3], 2,
		{
			{ "PC",  (int *)&((TMS320C10_Regs *)rgs)->PC, 2, 2, 1 },
			{ "ACC", (int *)&((TMS320C10_Regs *)rgs)->ACC, 4, 10, 1 },
			{ "P",   (int *)&((TMS320C10_Regs *)rgs)->Preg, 4, 23, 1 },
			{ "T",   (int *)&((TMS320C10_Regs *)rgs)->Treg, 2, 34, 1 },
			{ "AR0", (int *)&((TMS320C10_Regs *)rgs)->AR[0], 2, 41, 1 },
			{ "AR1", (int *)&((TMS320C10_Regs *)rgs)->AR[1], 2, 50, 1 },
			{ "STR", (int *)&((TMS320C10_Regs *)rgs)->STR, 2, 59, 1 },
			{ "STAK3", (int *)&((TMS320C10_Regs *)rgs)->STACK[3], 2, 68, 1 },
			{ "", (int *)-1, -1, -1, -1 }
		},
	},
	/* Dummy CPU -- placeholder CCPU */
	{
		"CCPU", 14,
		DrawDebugScreen8,
		DummyDasm, DummyTrace, 0, 8,
		"........", (int *)&DummyCC, 8,
		"%04X:", 0xffff,
                25, 31, 77, 16, 3,
		1, 1,
		(int *)-1, -1,
		{
			{ "", (int *)-1, -1, -1, -1 }
		},
    },
};


#endif /* _MAMEDBG_H */
