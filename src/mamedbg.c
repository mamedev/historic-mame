/*		The MAME Debugger!
 *
 *		Written by:   Martin Scragg, John Butler, Mirko Buffoni
 *		              Chris Moore, Aaron Giles, Ernesto Corvi
 *
 *		Powered by the new Command-Line parser!    (MB: 980204)
 *		Many commands accept either a value or a register name.
 *		You can indeed type either  R HL = SP  or  R HL = 1fd0.
 *		In the syntax, where you see <address> you may generally
 *		use a number or a register name.
 *
 *		Commands:
 *		-  BPX  <address>           Set a breakpoint
 *		-  BC                       Clear all breakpoints
 *		-  D    <1|2> [address]     Display an address on viewport 1
 *									or 2.  Default to disassembly
 *									window cursor location.
 *		-  DASM <filename>          Disassemble a range of memory to file
 *		        <StartAddress>      Addresses may be a value or a register
 *		        <EndAddress>
 *		-  DUMP <filename>          Dump a memory range to "filename"
 *		        <StartAddress>      Addresses may be a value or a register
 *		        <EndAddress>
 *		-  TRACE <filename>|OFF		Start CPU instruction tracing to "filename"
 *									or turn CPU instruction tracing OFF
 *		-  E    <1|2> [address]     Edit an address on viewport 1 or
 *									2.  Default to not changing the
 *									location which the viewport is editing.
 *		-  F                        Run fast.  Ignore breakpoints, etc.
 *                                  Just watch for TILDE being pressed.
 *		-  G    <address>           Continue execution.  If an address is
 *		                            provided, MDB will set a breakpoint there
 *		-  HERE                     Set a temporary breakpoint to current
 *		                            cursor position
 *		-  J    <address>           Jump to address in code window.  Doesn't
 *		                            alter PC.
 *		-  R    <register> =        Edit register.  If no parameters are given
 *		        <register|value>    the first register is edited.  If only the
 *		                            first parameter is given, it will edit
 *		                            <register>.
 *		-  BPW	<address>			Set a Watchpoint for the currently active cpu
 *		-  WC						Clear Watchpoint for the currently active cpu
 *
 *		During memory editing, it is possible to search for a string by
 *		pressing S.  This doesn't work yet on 68K systems if string is not
 *		found!  By pressing 'H', you can toggle ASCII display.  By pressing
 *		J you can jump to an address.
 *
 *		Shortcuts are available:
 *		-	F2	Toggle a breakpoint at current cursor position
 *		-	F4	Run to cursor
 *		-	F5	View emulation screen
 *		-	F6	Breakpoint to the next CPU
 *		-	F8	Step
 *		-	F9	Animate (Trace at high speed)
 *		-   F10  Step Over
 *		-	F12	GO!
 *		-	ESC	^ditto^
 *
 *		ENJOY! ;)
 */

#include <stdio.h>

#ifdef MAME_DEBUG

#include "mamedbg.h"
#include "driver.h"

/* undefine the following if you don't want 'LEFT' and 'RIGHT' to do
   anything in the disassembly window.

   Nicola wrote:

   > Actually, I would like right and left to move in and out of
   > subroutines.  Press RIGHT while on a CALL/JSR/JMP instruction, and
   > you go to the pointed address. move as you like, then press LEFT to
   > return to the previous point.  Recursively, of course.
*/
#define ALLOW_LEFT_AND_RIGHT_IN_DASM_WINDOW	/* CM 980428 */

static int MEM1 = MEM1DEFAULT;	/* JB 971210 */
static int MEM2 = MEM2DEFAULT;	/* JB 971210 */

extern int debug_key_pressed;	/* JB 980505 */

/* globals */
static int              BreakPoint[5] = { -1, -1, -1, -1, -1 };
static int              TempBreakPoint = -1;	/* MB 980103 */
static int				CPUBreakPoint = -1;		/* MB 980121 */
static int              CurrentPC = -1;
static int              CursorPC = -1;		/* MB 980103 */
static int				NextPC = -1;
static int				PreviousSP = 0;
static int              StartAddress = -1;
static int              EndAddress = -1;	/* MB 980103 */
static int              activecpu, cputype, PreviousCPUType;
static int              DisplayASCII[2] = { FALSE, FALSE };	/* MB 980103 */
static int				InDebug=FALSE;
static int				Update;
static int				gCurrentTraceCPU = -1;	/* JB 980214 */
static int				DebugFast = 0; /* CM 980506 */
static int				CPUWatchpoint[5] = { -1, -1, -1, -1, -1 };	/* EHC 980506 */
static int				CPUWatchdata[5] = { -1, -1, -1, -1, -1 };	/* EHC 980506 */

/* Draw the screen outline */
static void DrawDebugScreen8 (int TextCol, int LineCol)
{
	int		y;

	ScreenClear();
	ScreenPutString("ษออ           อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป", LineCol, 0, 0);
	ScreenPutString("Registers", TextCol, 4, 0);
	ScreenPutString("บ", LineCol, 0, 1);
	ScreenPutString("บ", LineCol, 79, 1);
	ScreenPutString("ฬออ       ออหออ     อออหออ          อออออออออออออออออออออออออออออออออออออออออออน", LineCol, 0, 2);
	ScreenPutString("Flags", TextCol, 4, 2);
	ScreenPutString("CPU", TextCol, 16, 2);
	ScreenPutString("Memory 1", TextCol, 27, 2);
	ScreenPutString("บ", LineCol, 0, 3);
	ScreenPutString("บ", LineCol, 12, 3);
	ScreenPutString("บ", LineCol, 23, 3);
	ScreenPutString("บ", LineCol, 79, 3);
	ScreenPutString("ฬออ      อออสออออออออออน", LineCol, 0, 4);
	ScreenPutString("Code", TextCol, 4, 4);
	ScreenPutString("บ", LineCol, 79, 4);
	for (y = 5; y < 11; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 23, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("บ", LineCol, 0, 11);
	ScreenPutString("ฬออ          อออออออออออออออออออออออออออออออออออออออออออน", LineCol, 23, 11);
	ScreenPutString("Memory 2", TextCol, 27, 11);
	for (y = 12; y < 20; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 23, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ฬออ         อออออออออออสอออออออออออออออออออออออออออออออออออออออออออออออออออออออน", LineCol, 0, 20);
	ScreenPutString("Command", TextCol, 4, 20);
	for (y = 21; y < 24; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ", LineCol, 0, 24);
}

static void DrawDebugScreen16 (int TextCol, int LineCol)	/* MB 980103 */
{
	int		y;

	ScreenClear();
	ScreenPutString("ษออ           อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป", LineCol, 0, 0);
	ScreenPutString("Registers", TextCol, 4, 0);
	ScreenPutString("บ", LineCol, 0, 1);
	ScreenPutString("บ", LineCol, 79, 1);
	ScreenPutString("ฬออ       อออออออออหออ     ออออหออ          อออออออออออออออออออออป             บ", LineCol, 0, 2);
	ScreenPutString("Flags", TextCol, 4, 2);
	ScreenPutString("CPU", TextCol, 23, 2);
	ScreenPutString("Memory 1", TextCol, 34, 2);
	ScreenPutString("บ", LineCol, 0, 3);
	ScreenPutString("บ", LineCol, 19, 3);
	ScreenPutString("บ", LineCol, 31, 3);
	ScreenPutString("บ", LineCol, 65, 3);
	ScreenPutString("บ", LineCol, 79, 3);
	ScreenPutString("ฬออ      ออออออออออสอออออออออออน", LineCol, 0, 4);
	ScreenPutString("บ", LineCol, 65, 4);
	ScreenPutString("Code", TextCol, 4, 4);
	ScreenPutString("บ", LineCol, 79, 4);
	for (y = 5; y < 11; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 31, y);
		ScreenPutString("บ", LineCol, 65, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("บ", LineCol, 0, 11);
	ScreenPutString("ฬออ          อออออออออออออออออออออน", LineCol, 31, 11);
	ScreenPutString("บ", LineCol, 79, 11);
	ScreenPutString("Memory 2", TextCol, 34, 11);
	for (y = 12; y < 20; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 31, y);
		ScreenPutString("บ", LineCol, 65, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ฬออ         อออออออออออออออออออสอออออออออออออออออออออออออออออออออสอออออออออออออน", LineCol, 0, 20);
	ScreenPutString("Command", TextCol, 4, 20);
	for (y = 21; y < 24; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ", LineCol, 0, 24);
}

/* Draw the Memory dump windows */
static void DrawMemWindow (int Base, int Col, int Offset, int _DisplayASCII)	/* JB 971210 */
{
	int     X;
	int     Y;
	int		which = ((Offset==12)?0x100:0);		/* I hate to do this */
	char    FMT[32];
	char    S[32];
	unsigned char	value, oldValue;

	for (Y = 0; Y < 8; Y++)
	{
		sprintf(FMT, "%s", DebugInfo[cputype].AddPrint);
		sprintf(S, FMT, Base);
		ScreenPutString(S, Col, DebugInfo[cputype].MemWindowAddX,
				Y + Offset);
		for (X = 0; X < DebugInfo[cputype].MemWindowNumBytes; X++)
		{
			value = cpuintf[cputype].memory_read(Base++ & DebugInfo[cputype].AddMask);
			oldValue = MemWindowBackup[Y * DebugInfo[cputype].MemWindowNumBytes + X + which];
			if (_DisplayASCII)	/* MB 980103 */
			{
				sprintf (S, "  ");
				if (S[0] != 0) S[0] = value; else S[0] = 32;
			}
			else
			  sprintf (S, "%02X", value);
			ScreenPutString(S, ((value == oldValue)?Col:CHANGES_COLOUR), DebugInfo[cputype].MemWindowDataX +
					(X * 3), Y + Offset);
			MemWindowBackup[Y * DebugInfo[cputype].MemWindowNumBytes + X + which] = value;
			if (Base > DebugInfo[cputype].AddMask || Base < 0)
				Base = 0x000000;
		}
	}
}


static int IsRegister(int _cputype, char *src)
{
	int i = 0;
	while ((i < 32) && (DebugInfo[_cputype].RegList[i].Name != NULL) &&
			(strcmp(DebugInfo[_cputype].RegList[i].Name,src) != 0))
		i++;

	if ((i < 32) && (DebugInfo[_cputype].RegList[i].Name != NULL))
		return i;
	return -1;
}

static unsigned long GetAddress(int _cputype, char *src)
{
	static unsigned char _rgs[CPU_CONTEXT_SIZE];	/* ASG 971105 */
	int SrcREG = IsRegister(_cputype, src);
	unsigned long rv = 0;

	if (SrcREG != -1)
	{
		cpuintf[_cputype].get_regs(_rgs);
		switch (DebugInfo[_cputype].RegList[SrcREG].Size)
		{
			case 1:
				rv = *(byte *)DebugInfo[_cputype].RegList[SrcREG].Val;
				break;
			case 2:
				rv = *(word *)DebugInfo[_cputype].RegList[SrcREG].Val;
				break;
			case 4:
				rv = *(dword *)DebugInfo[_cputype].RegList[SrcREG].Val;
				break;
		}
	}
	else
	{
		int value;
		int n = sscanf(src, "%x", &value);
		if (n > 0)
			rv = value;
	}
	return rv;
}


static int ModifyRegisters(char *param)
{
	int	 i,nCorrectParams = 0, rv = 0;
	char s1[20],s2[20];
	char *pr = param;
	char *pr2;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	if (strlen(pr) > 0)
	{
		pr2 = strtok(pr,"=");
		if (pr2)
		{
			pr2 += strlen(pr2);
			pr2[0] = ' ';
			if (pr2[1] == ' ')
			{
				i = 0;
				while ((pr2[i] == ' ') && (pr2[i] != '\0')) i++;
				while (pr2[0] != '\0')
				{
				  pr2[0] = pr2[i];
				  pr2++;
				}
			}
		}
		nCorrectParams = sscanf(pr, "%s %s", s1, s2);
		if (nCorrectParams > 1)
		{
			int TrgREG = IsRegister(cputype, s1);
			unsigned long addr = GetAddress(cputype, s2);
			byte b = addr & 0xff;
			word w = addr & 0xffff;

			if (TrgREG != -1)
			{
				cpuintf[cputype].get_regs(rgs);
				switch (DebugInfo[cputype].RegList[TrgREG].Size)
				{
					case 1:
						*(byte *)DebugInfo[cputype].RegList[TrgREG].Val = b;
						break;
					case 2:
						*(word *)DebugInfo[cputype].RegList[TrgREG].Val = w;
						break;
					case 4:
						*(dword *)DebugInfo[cputype].RegList[TrgREG].Val = addr;
						break;
				}
				cpuintf[cputype].set_regs(rgs);
			}
			else
				EditRegisters(0);
		}
		else if (nCorrectParams > 0)
		{
			int TrgREG = IsRegister(cputype, s1);
			if (TrgREG != -1)
				EditRegisters(TrgREG);
			else
				EditRegisters(0);
		}
		else
			EditRegisters(0);
	}
	else
		EditRegisters(0);
	return rv;
}

/* JB 980214 */
/* TRACE <FileName>|OFF */
static int TraceToFile(char *param)
{
	int	i,nCorrectParams = 0, rv = 0;
	char s1[128];
	char *pr = param;

	/* strip leading and trailing spaces */
	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s", s1);
	if (nCorrectParams == 1)
	{
		if (!stricmp (s1, "OFF"))
		{
			/* turn trace off */
			if (!traceon)
			{
				if (errorlog)
					fprintf (errorlog, "Trace not on when TRACE OFF encountered.\n");
				rv = -1;
			}
			else
			{
				asg_TraceKill ();
				traceon = 0;
			}
		}
		else
		{
			/* turn trace on */
			if (traceon)
			{
				if (errorlog)
					fprintf (errorlog, "Trace already on when TRACE %s encountered.\n", s1);
				rv = -1;
			}
			else
			{
				asg_TraceInit (cpu_gettotalcpu (), s1);
				traceon = 1;
				gCurrentTraceCPU = activecpu;
				asg_TraceSelect (gCurrentTraceCPU);
			}
		}
	}
	else rv = -1;
	return rv;
}

static int DumpToFile(char *param)
{
	FILE *f;
	int	i,nCorrectParams = 0, rv = 0, first;
	char s1[128], s2[20], s3[20];
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s %s %s", s1, s2, s3);
	if (nCorrectParams >= 3)
	{
		unsigned long StartAd = GetAddress(cputype,s2);
		unsigned long EndAd = GetAddress(cputype,s3);

		if ((f = fopen(s1, "w")) == NULL)
		{
			if (errorlog) fprintf(errorlog, "Error while creating DUMP file:  %s\n", s1);
			rv = -1;
		}

		first = 1;
		while (StartAd <= EndAd)
		{
			if (first || (StartAd % 16 == 0))
			{
				int start = StartAd & (DebugInfo[cputype].AddMask ^ 0xf);
				sprintf(s1, DebugInfo[cputype].AddPrint, start);
				s1[7] = 0;
				fprintf(f, "%s", s1);
				if (first)
				{
					fprintf(f, "%*s", 3 * (int)(StartAd-start), "");
					first = 0;
				}
			}

			fprintf(f, " %02x",	cpuintf[cputype].memory_read(StartAd & DebugInfo[cputype].AddMask));

			StartAd++;
			if (StartAd % 16 == 0)
				fprintf(f, "\n");
		}

		fprintf(f, "\n");
		if (fclose(f) != 0)
		{
			if (errorlog) fprintf(errorlog, "Error while closing DUMP file.\n");
			rv = -1;
		}
	}
	else rv = -1;
	return rv;
}
static int DasmToFile(char *param)
{
	FILE *f;
	int	i,nCorrectParams = 0, rv = 0;
	char s1[128], s2[20], s3[20];
	char *pr = param;
	int  pc, old_pc, tmp;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s %s %s", s1, s2, s3);
	if (nCorrectParams >= 3)
	{
		unsigned long StartAd = GetAddress(cputype,s2);
		unsigned long EndAd = GetAddress(cputype,s3);

		if ((f = fopen(s1, "w")) == NULL)
		{
			if (errorlog) fprintf(errorlog, "Error while creating DASM file:  %s\n", s1);
			rv = -1;
		}

		pc = StartAd;
		while (pc < EndAd)
		{
			sprintf(s1, DebugInfo[cputype].AddPrint, pc);
			s1[7] = 0;
			fprintf(f, "%s", s1);
			old_pc = pc;
			pc += debug_dasm_line (pc, s1);

			for (tmp = old_pc; tmp < pc; tmp++)
				fprintf(f, " %02x",	cpuintf[cputype].memory_read(tmp & DebugInfo[cputype].AddMask));

			fprintf(f, " %*s ",	3 * (old_pc + DebugInfo[cputype].MaxInstLen - pc), "");

			fprintf(f, "%s\n", s1);
		}

		if (fclose(f) != 0)
		{
			if (errorlog) fprintf(errorlog, "Error while closing DASM file.\n");
			rv = -1;
		}
	}
	else rv = -1;
	return rv;
}

static int SetBreakPoint(char *param)
{
	int	i,nCorrectParams = 0;
	char s1[20];
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s", s1);
	if (nCorrectParams > 0)
	{
		int addr = GetAddress(cputype, s1);
		BreakPoint[activecpu] = addr & DebugInfo[cputype].AddMask;
	}
	else
		BreakPoint[activecpu] = CursorPC;

	return 0;
}

static int ClearBreakPoint(char *param)
{
	BreakPoint[activecpu] = -1;
	return 0;
}

/* EHC 980506 */
/* BPW <addr> */
static int SetWatchPoint(char *param)
{
	int	i,nCorrectParams = 0;
	char s1[20];
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s", s1);
	if (nCorrectParams > 0)
	{
		int addr = GetAddress(cputype, s1);
		CPUWatchpoint[activecpu] = addr & DebugInfo[cputype].AddMask;
		CPUWatchdata[activecpu] = cpuintf[cputype].memory_read( CPUWatchpoint[activecpu] );
		return 0;
	}

	return -1;
}

/* EHC 980506 */
/* WPC */
static int ClearWatchPoint(char *param)
{
	CPUWatchpoint[activecpu] = -1;
	return 0;
}

static int Here(char *param)
{
	TempBreakPoint = CursorPC;
	Update = FALSE;
	InDebug = FALSE;
	osd_set_mastervolume(CurrentVolume);
	osd_set_display(Machine->scrbitmap->width,
					Machine->scrbitmap->height,
					Machine->drv->video_attributes);
	return 0;
}

static int Go(char *param)
{
	int	i,nCorrectParams = 0;
	char s1[20];
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	nCorrectParams = sscanf(param, "%s", s1);
	if (nCorrectParams > 0)
	{
		int addr = GetAddress(cputype, s1);
		TempBreakPoint = addr & DebugInfo[cputype].AddMask;
	}
	Update = FALSE;
	InDebug = FALSE;
	osd_set_mastervolume(CurrentVolume);
	osd_set_display(Machine->scrbitmap->width,
					Machine->scrbitmap->height,
					Machine->drv->video_attributes);
	return 0;
}

static int EditMemory(char *param)
{
	int	i,nCorrectParams = 0, rv = 0;
	char s1[20];
	int addr, view;
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	if (strlen(pr) > 0)
	{
		nCorrectParams = sscanf(pr, "%d %s", &view, s1);	/* CM 980428 */
		if (nCorrectParams > 0)
		{
			if (nCorrectParams == 2)
				addr = GetAddress(cputype, s1);
			else
				/* if no address was specified, use current viewport address */
				addr = (view == 1 ? MEM1 : MEM2);			/* CM 980428 */

			if (view == 1)
				MEM1 = ScreenEdit(DebugInfo[cputype].MemWindowDataX,
								  DebugInfo[cputype].MemWindowDataXEnd,
								  3, 10, MEM1_COLOUR, addr, &DisplayASCII[0]);
			else if (view == 2)
				MEM2 = ScreenEdit(DebugInfo[cputype].MemWindowDataX,
								  DebugInfo[cputype].MemWindowDataXEnd,
								  12, 19, MEM2_COLOUR, addr, &DisplayASCII[1]);
			else rv = -1;
		}
	}
	else
	{
		MEM1 = ScreenEdit(DebugInfo[cputype].MemWindowDataX,
						  DebugInfo[cputype].MemWindowDataXEnd,
						  3, 10, MEM1_COLOUR, MEM1, &DisplayASCII[0]);
	}
	return rv;
}

static int FastDebug(char *param)
{
	DebugFast = 1;
	return Go("");
}

static int DisplayMemory(char *param)
{
	int	i,nCorrectParams = 0, rv = 0;
	char s1[20];
	int addr, view;
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	if (strlen(pr) > 0)
	{
		nCorrectParams = sscanf(pr, "%d %s", &view, s1); /* CM 980428 */
		if (nCorrectParams > 0)
		{
			if (nCorrectParams == 2)
				addr = GetAddress(cputype, s1);
			else
				/* if no address is specified, use current disassembly
				 * window cursor position */
				addr = CursorPC; /* CM 980428 */

			if (view == 1)
			{
				DrawMemWindow (addr, MEM1_COLOUR, 3, DisplayASCII[0]);	/* MB 980103 */
				MEM1 = addr;
			}
			else if (view == 2)
			{
				DrawMemWindow (addr, MEM2_COLOUR, 12, DisplayASCII[1]);	/* MB 980103 */
				MEM2 = addr;
			}
			else rv = -1;
		}
	}
	else
		rv = -1;
	return rv;
}


static int DisplayCode(char *param)
{
	int	i,nCorrectParams = 0, rv = 0;
	char s1[20];
	char *pr = param;

	while ((pr[0] == ' ') && (pr[0] != '\0')) pr++;
	i = strlen(pr);
	while ((i >= 0) && ((pr[i] == ' ') || (pr[i] == '\0'))) pr[i--] = '\0';

	if (strlen(pr) > 0)
	{
		nCorrectParams = sscanf(pr, "%s", s1);
		if (nCorrectParams > 0)
		{
			StartAddress = GetAddress(cputype, s1);
			CursorPC = StartAddress;
			EndAddress = debug_draw_disasm (StartAddress);
		}
	}
	return rv;
}


/* Get a String of hex digits */
static void GetHexString (int X, int Y, int Col, int *String)
{
	char		Num[16], *Num_Ptr;
	int			Pos = 0;
	int			Key;
	int			Index;
	char		Temp[3];

	Num[Pos] = '\0';
	ScreenSetCursor(Y, X);
	ScreenPutString("        ", Col, X, Y);
	while ((Key = osd_debug_readkey ()) != OSD_KEY_ENTER)	/* JB 980208 */
	{
		switch(Key)
		{
			case OSD_KEY_0: case OSD_KEY_1: case OSD_KEY_2: case OSD_KEY_3:
			case OSD_KEY_4: case OSD_KEY_5: case OSD_KEY_6: case OSD_KEY_7:
			case OSD_KEY_8: case OSD_KEY_9: case OSD_KEY_A: case OSD_KEY_B:
			case OSD_KEY_C: case OSD_KEY_D: case OSD_KEY_E: case OSD_KEY_F:
				if (Pos<15)		/* JB 971207 */
				{
					strcat(Num, osd_key_name(Key));
					Pos++;
				}
				break;
			case OSD_KEY_BACKSPACE:
				if (Pos > 0)
				{
					Pos--;
					Num[Pos] = '\0';
				}
				break;
			default:
				Key = 0;
				break;
		}
		if (Key)
		{
			ScreenPutString("        ", Col, X + Pos, Y);
			ScreenPutString(Num, Col, X, Y);
			ScreenSetCursor(Y, X + Pos);
		}
	}
	ScreenPutString("        ", Col, X, Y);

	Index = 0;
	Num_Ptr = &Num[0];
	if ((Pos % 2) == 1)
	{
		Temp[0] = *(Num_Ptr++);
		Temp[1] = '\0';
		String[Index++] = strtol(Temp, NULL, 16);
	}

	while (*Num_Ptr)
	{
		Temp[0] = *(Num_Ptr++);
		Temp[1] = *(Num_Ptr++);
		Temp[2] = '\0';
		String[Index++] = strtol(Temp, NULL, 16);
	}

	String[Index] = -1;

	if (errorlog)
	{
		int Loop;

		fprintf(errorlog, "got a string:\n");
		for (Loop = 0; String[Loop] != -1; Loop++)
			fprintf(errorlog, "%3d %02x\n", String[Loop], String[Loop]);
		fprintf(errorlog, "end of string:\n");
	}
}

/* Get a Number */
static int GetNumber (int X, int Y, int Col)
{
	char	Num[16];
	int		Pos = 0;
	int		Key;

	Num[Pos] = '\0';
	ScreenSetCursor(Y, X);
	ScreenPutString("        ", Col, X, Y);
	while ((Key = osd_debug_readkey ()) != OSD_KEY_ENTER)	/* JB 980208 */
	{
		switch(Key)
		{
			case OSD_KEY_0:
			case OSD_KEY_1:
			case OSD_KEY_2:
			case OSD_KEY_3:
			case OSD_KEY_4:
			case OSD_KEY_5:
			case OSD_KEY_6:
			case OSD_KEY_7:
			case OSD_KEY_8:
			case OSD_KEY_9:
			case OSD_KEY_A:
			case OSD_KEY_B:
			case OSD_KEY_C:
			case OSD_KEY_D:
			case OSD_KEY_E:
			case OSD_KEY_F:
				if (Pos<15)		/* JB 971207 */
				{
					strcat(Num, osd_key_name(Key));
					Pos++;
				}
				break;
			case OSD_KEY_BACKSPACE:
				if (Pos > 0)
				{
					Pos--;
					Num[Pos] = '\0';
				}
				break;
			default:
				Key = 0;
				break;
		}
		if (Key)
		{
			ScreenPutString("        ", Col, X + Pos, Y);
			ScreenPutString(Num, Col, X, Y);
			ScreenSetCursor(Y, X + Pos);
		}
	}
	ScreenPutString("        ", Col, X, Y);
	return(strtol(Num, NULL, 16));
}

/* Search for a string of numbers in memory; target is a -1 terminated array */
int FindString (int Add, int *Target)
{
	int Start_Address;
	int Offset;

	ScreenPutString ("Searching...       ", INSTRUCTION_COLOUR, 2, 23);
	for (Start_Address = Add + 1; Start_Address != Add;
		 Start_Address = (Start_Address + 1) % DebugInfo[cputype].AddMask)
	{
		if (Start_Address % ((DebugInfo[cputype].AddMask >> 8) + 1) == 0)
		{
			char tmp[70];
			sprintf(tmp, "[%02x%%] %x/%x",
					Start_Address / ((DebugInfo[cputype].AddMask >> 8) + 1),
					Start_Address, ((DebugInfo[cputype].AddMask >> 8) + 1));
			ScreenPutString (tmp, INSTRUCTION_COLOUR, 15, 23);
		}
		for (Offset = 0;
			 Target[Offset] != -1 &&
				 cpuintf[cputype].memory_read(Start_Address + Offset) ==
				 Target[Offset];
			 Offset++);

		if (Target[Offset] == -1)
		{
			ScreenPutString ("Found the string   ", INSTRUCTION_COLOUR, 2, 23);
			return Start_Address;
		}
	}

	ScreenPutString ("Not found          ",
					 INSTRUCTION_COLOUR, 2, 23);
	return Add;
}

/* Edit the memory window */
static int ScreenEdit (int XMin, int XMax, int YMin, int YMax, int Col, int BaseAdd, int *_DisplayASCII)	/* JB 971210 */
{
	int		Editing = TRUE;
	int		High = TRUE;
	int		Scroll = FALSE;
	int		Key;
	char	Num[2];
	int		X = XMin;
	int		Y = YMin;
	int		row_bytes;		/* JB 971210 */
	int		Add = BaseAdd;	/* JB 971210 */
	int		Base = BaseAdd;	/* JB 971210 */
	int		Target[16];
	static int	Last_Target[16] = {-1};
	int		Loop;
	char	info[80];
	byte	b;

	DrawMemWindow (Base, Col, YMin, *_DisplayASCII);

	row_bytes = DebugInfo[cputype].MemWindowNumBytes;

	while (Editing)
	{
		ScreenSetCursor(Y, X);
		//Key = osd_read_keyrepeat();
		Key = osd_debug_readkey();	/* JB 980103 */
		switch(Key)
		{
			case OSD_KEY_HOME:			/* Home */	/* MB 980103 */
				Add &= 0xFF0000;
				Base &= 0xFF0000;
				X = XMin;
				Y = YMin;
				High = TRUE;
				Scroll = TRUE;
				break;
			case OSD_KEY_END:			/* End */	/* MB 980103 */
				Base = (Add & 0xff0000) | (0x010000-(YMax-YMin+1)*row_bytes);
				Add |= 0x00FFFF;
				X = XMax-1;
				Y = YMax;
				High = TRUE;
				Scroll = TRUE;
				break;
			case OSD_KEY_UP:			/* Up */
				Y--;
				Add -= row_bytes;		/* JB 971210 */
				break;
			case OSD_KEY_PGUP:			/* Page Up */
				Add -= row_bytes * 8;	/* JB 971210 */
				Base -= row_bytes * 8;	/* JB 971210 */
				Scroll = TRUE;
				break;
			case OSD_KEY_LEFT:			/* Left */
				if (*_DisplayASCII)	/* MB 980103 */
				   X-=3;
				else
				{
				   if (High)
				   {
					   X -= 2;
					   Add--;
				   }
				   else
					   X -= 1;
				   High = !High;
				}
				break;
			case OSD_KEY_RIGHT:			/* Right */
				if (*_DisplayASCII)	/* MB 980103 */
				   X+=3;
				else
				{
				   if (High)
					   X += 1;
				   else
				   {
					   X += 2;
					   Add++;
				   }
				   High = !High;
				}
				break;
			case OSD_KEY_DOWN:			/* Down */
				Y++;
				Add += row_bytes;	/* JB 971210 */
				break;
			case OSD_KEY_PGDN:			/* Page Down */
				Add += row_bytes * 8;	/* JB 971210 */
				Base += row_bytes * 8;	/* JB 971210 */
				Scroll = TRUE;
				break;
			case OSD_KEY_H:
				*_DisplayASCII = !(*_DisplayASCII);	/* MB 980103 */
				High = TRUE;
				Scroll = TRUE;
				break;
			case OSD_KEY_J:	/* MB 980103 */
				sprintf(info, "%-76s", " ");
				ScreenPutString(info, INSTRUCTION_COLOUR, 2, 23);
				ScreenPutString ("Jump to new address",	INSTRUCTION_COLOUR, 2, 23);
				Add = GetNumber (10, 21, INPUT_COLOUR);
				Base = Add;
				X = XMin + (High ? 0 : 1);			/* CM 980118 */
				Y = YMin;
				Scroll = TRUE;
				break;

			case OSD_KEY_S:
				sprintf(info, "%-76s", " ");
				ScreenPutString(info, INSTRUCTION_COLOUR, 2, 23);
				ScreenPutString ("Search for string  ",	INSTRUCTION_COLOUR, 2, 23);
				GetHexString (10, 21, INPUT_COLOUR, Target);
				if (Target[0] == -1)
				{
					Base = Add = FindString (Add, Last_Target);
				}
				else
				{
					Base = Add = FindString (Add, Target);
					for (Loop = 0; Target[Loop] != -1; Loop++)
						Last_Target[Loop] = Target[Loop];
					Last_Target[Loop] = -1;
				}
				X = XMin + (High ? 0 : 1);
				Y = YMin;
				Scroll = TRUE;
				break;

			case OSD_KEY_0:
			case OSD_KEY_1:
			case OSD_KEY_2:
			case OSD_KEY_3:
			case OSD_KEY_4:
			case OSD_KEY_5:
			case OSD_KEY_6:
			case OSD_KEY_7:
			case OSD_KEY_8:
			case OSD_KEY_9:
			case OSD_KEY_A:
			case OSD_KEY_B:
			case OSD_KEY_C:
			case OSD_KEY_D:
			case OSD_KEY_E:
			case OSD_KEY_F:
				if (!(*_DisplayASCII))	/* MB 980103 */
				{
					b = cpuintf[cputype].memory_read(Add & DebugInfo[cputype].AddMask);

					strcpy(Num, osd_key_name(Key));
					ScreenPutString (Num, Col, X, Y);
					Num[0] -= '0';
					if (Num[0] > 9)
						Num[0] -= 7;

					if (High)
					{
						cpuintf[cputype].memory_write(Add &
							DebugInfo[cputype].AddMask, (b & 0x0f) | (Num[0] << 4));
						X++;
					}
					else
					{
						cpuintf[cputype].memory_write(Add &
							DebugInfo[cputype].AddMask, (b & 0xf0) | Num[0]);
						X += 2;
						Add++;
					}
					High = !High;
				}
				break;
			case OSD_KEY_ESC:
			case OSD_KEY_ENTER:
				Editing = FALSE;
				break;
		}
		if (X < XMin)
		{
			X = XMax;
			Y--;
			if (*_DisplayASCII) X--;
		}
		else if (X > XMax)
		{
			X = XMin;
			Y++;
		}
		if (Y > YMax)
		{
			Y = YMax;
			Base += row_bytes;	/* JB 971210 */
			Scroll = TRUE;
		}
		else if (Y < YMin)
		{
			Y = YMin;
			Base -= row_bytes;	/* JB 971210 */
			Scroll = TRUE;
		}
		if ((Base < 0) || Base > (DebugInfo[cputype].AddMask -
			 (YMax-YMin) * DebugInfo[cputype].MemWindowNumBytes))
		{
			Base = Add = 0;
			Scroll = TRUE;
		}
		if (Scroll)
		{
			DrawMemWindow (Base, Col, YMin, *_DisplayASCII);
			Scroll = FALSE;
		}
	}
	return (Base);
}

static int debug_draw_disasm (int pc)
{
	char 	fmt[100];
	char 	s[100];
	int		i, Colour;

	for (i=0; i<15; i++)
	{
		if (pc==BreakPoint[activecpu])
		{
			Colour = BREAKPOINT_COLOUR;	/* MB 980103 */
			if (pc == CurrentPC) Colour += (PC_COLOUR & 0xf0);
			else if (pc == CursorPC) Colour += (CURSOR_COLOUR & 0xf0);
		}
		else if (pc==CursorPC)	/* MB 980103 */
			Colour = CURSOR_COLOUR;
		else
			if (pc == CurrentPC)
				Colour = PC_COLOUR;
			else
				Colour = CODE_COLOUR;

		/* MB:  I don't like current implementation.  We must think each window as
			an object with its property, so we must clear its content depending on
			its size, and everything inside is another set of objects.  So we can
			move and resize the window without become crazy each time! */

		sprintf(fmt, " %s                          ",	/* MB 980103 */
				DebugInfo[cputype].AddPrint);
		sprintf(s, fmt, pc);
				s[DebugInfo[cputype].DasmStartX+DebugInfo[cputype].DasmLineLen-1] = '\0';	/* MB 980103 */
		ScreenPutString (s, Colour, 1, i + 5);
		pc += debug_dasm_line (pc, s);
		ScreenPutString (s, Colour, DebugInfo[cputype].DasmStartX, i + 5);
	}
		return pc;
}

static int debug_dasm_line (int pc, char *s)
{
	int		offset = 0;

	s[0] = '\0';
	offset = DebugInfo[cputype].Dasm(s, pc);
	s[DebugInfo[cputype].DasmLineLen] = '\0';
	return (offset);	/* MB 980103 */
}


static void debug_draw_flags (void)
{
	char	Flags[17], s[17];
	int		i, cc;
	int		flag_size = 8;

	strcpy (Flags, DebugInfo[cputype].Flags);
//	cc = *DebugInfo[cputype].CC;	/* removed /* JB 980103 */
	flag_size = DebugInfo[cputype].FlagSize;

	if (flag_size==8)		/* JB 971210 */
	{
		cc = *(byte *)DebugInfo[cputype].CC;	/* JB 980103 */
		for (i=0; i<8; i++, cc <<= 1)
			s[i] = cc & 0x80 ? Flags[i] : '.';
	}
	else
	{
		cc = *(word *)DebugInfo[cputype].CC;	/* JB 980103 */
		/* this is probably wrong for little endian machines */
		for (i=0; i<16; i++, cc <<= 1)
			s[i] = cc & 0x8000 ? Flags[i] : '.';
	}
	s[flag_size]='\0';	/* JB 971210 */
	ScreenPutString (s, FLAG_COLOUR, 2, 3);
}

static void debug_draw_registers (void)
{
	char	s[32];
	int		Num = 0;
	int 	Value = 0, oldValue = 0;

	while (DebugInfo[cputype].RegList[Num].Size > 0)
	{
		switch (DebugInfo[cputype].RegList[Num].Size)
		{
			case 1:
				Value = *(byte *)DebugInfo[cputype].RegList[Num].Val;	/* MB 980221 */
				oldValue = *(byte *)BackupRegisters[cputype].RegList[Num].Val;	/* MB 980221 */
				sprintf(s, "%s:%02X", DebugInfo[cputype].RegList[Num].Name, Value);
//						*(byte *)DebugInfo[cputype].RegList[Num].Val);	/* JB 980103 */
				break;
			case 2:
				Value = *(word *)DebugInfo[cputype].RegList[Num].Val;	/* MB 980221 */
				oldValue = *(word *)BackupRegisters[cputype].RegList[Num].Val;	/* MB 980221 */
				sprintf(s, "%s:%04X", DebugInfo[cputype].RegList[Num].Name, Value);
//						*(word *)DebugInfo[cputype].RegList[Num].Val);	/* JB 980103 */
				break;
			case 4:
				Value = *(dword *)DebugInfo[cputype].RegList[Num].Val;	/* MB 980221 */
				oldValue = *(dword *)BackupRegisters[cputype].RegList[Num].Val;	/* MB 980221 */
				sprintf(s, "%s:%08X", DebugInfo[cputype].RegList[Num].Name, Value);
//						*(dword *)DebugInfo[cputype].RegList[Num].Val);	/* JB 980103 */
				break;
		}
		ScreenPutString (s, ((Value == oldValue)?REGISTER_COLOUR:CHANGES_COLOUR),
			DebugInfo[cputype].RegList[Num].XPos,
			DebugInfo[cputype].RegList[Num].YPos);
		Num++;
	}
	cpuintf[cputype].get_regs(bckrgs);
}

/* Edit registers window */
static void EditRegisters (int Which)
{
	int		Editing = TRUE;
	int		High = TRUE;
	int		Key;
	char	Num[2];
	int		X = 0;
	int		Y = 0;
	int		NumRegs = 0;
	unsigned char	*Val;
#ifdef	LSB_FIRST
	int		Msw = TRUE;
	int		Msb = TRUE;
#endif

	X = 0;
	while (DebugInfo[cputype].RegList[X++].Size > 0)
		NumRegs++;

	if (NumRegs == 0)
		return;

	cpuintf[cputype].get_regs(rgs);

	X = DebugInfo[cputype].RegList[Which].XPos +
		strlen(DebugInfo[cputype].RegList[Which].Name) + 1;
	Y = DebugInfo[cputype].RegList[Which].YPos;
#ifdef	LSB_FIRST
	Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val +
		DebugInfo[cputype].RegList[Which].Size - 1;
#else
	Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
#endif

	while (Editing)
	{
		ScreenSetCursor (Y, X);
//		Key = osd_read_keyrepeat();
		Key = osd_debug_readkey();	/* JB 980103 */
		switch (Key)
		{
			case OSD_KEY_LEFT:			/* Left */
				X--;
				High = !High;
				if (!High)
				{
#ifdef	LSB_FIRST
					if (Msb)
					{
						if (Msw)
							Val -= 4;
						else
							if (DebugInfo[cputype].RegList[Which].Size > 2)
								Val++;
							else
								Val -= 2;
						Msw = !Msw;
					}
					else
					{
						if (DebugInfo[cputype].RegList[Which].Size > 1)
							Val++;
						else
							Val--;
					}
					Msb = !Msb;
#else
					Val--;
#endif
				}
				break;
			case OSD_KEY_RIGHT:			/* Right */
				X++;
				High = !High;
				if (High)
				{
#ifdef	LSB_FIRST
					if (!Msb)
					{
						if (!Msw)
							Val += 4;
						else
							if (DebugInfo[cputype].RegList[Which].Size > 2)
								Val--;
							else
								Val += 2;
						Msw = !Msw;
					}
					else
					{
						if (DebugInfo[cputype].RegList[Which].Size > 1)
							Val--;
						else
							Val++;
					}
					Msb = !Msb;
#else
					Val++;
#endif
				}
				break;
			case OSD_KEY_UP:			/* Up */	/* JB 971215 */
				if (--Which < 0)
					Which = NumRegs - 1;

				X = DebugInfo[cputype].RegList[Which].XPos +
					strlen(DebugInfo[cputype].RegList[Which].Name) + 1 +
					(DebugInfo[cputype].RegList[Which].Size * 2) - 1;
				Y = DebugInfo[cputype].RegList[Which].YPos;
#ifdef	LSB_FIRST
				Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
				Msw = FALSE;
				Msb = FALSE;
#else
				Val = ((unsigned char *)DebugInfo[cputype].RegList[Which].Val +
					(DebugInfo[cputype].RegList[Which].Size - 1));
#endif
				High = FALSE;
				break;
			case OSD_KEY_DOWN:			/* Down */	/* JB 971215 */
				if (++Which >=  NumRegs)
					Which = 0;

				X = DebugInfo[cputype].RegList[Which].XPos +
					strlen(DebugInfo[cputype].RegList[Which].Name) + 1;
				Y = DebugInfo[cputype].RegList[Which].YPos;
				Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
#ifdef	LSB_FIRST
				Val = ((unsigned char *)DebugInfo[cputype].RegList[Which].Val +
					(DebugInfo[cputype].RegList[Which].Size - 1));
				Msw = TRUE;
				Msb = TRUE;
#else
				Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
#endif
				High = TRUE;
				break;
			case OSD_KEY_0:
			case OSD_KEY_1:
			case OSD_KEY_2:
			case OSD_KEY_3:
			case OSD_KEY_4:
			case OSD_KEY_5:
			case OSD_KEY_6:
			case OSD_KEY_7:
			case OSD_KEY_8:
			case OSD_KEY_9:
			case OSD_KEY_A:
			case OSD_KEY_B:
			case OSD_KEY_C:
			case OSD_KEY_D:
			case OSD_KEY_E:
			case OSD_KEY_F:
				strcpy(Num, osd_key_name(Key));
				ScreenPutString (Num, REGISTER_COLOUR, X, Y);
				Num[0] -= '0';
				if (Num[0] > 9)
					Num[0] -= 7;
				if (High)
				{
					*Val &= 0x0f;
					*Val |= (Num[0] << 4);
				}
				else
				{
					*Val &= 0xf0;
					*Val |= Num[0];
				}
				X++;
				High = !High;
				if (High)
				{
#ifdef	LSB_FIRST
					if (!Msb)
					{
						if (!Msw)
							Val += 4;
						else
							if (DebugInfo[cputype].RegList[Which].Size > 2)
								Val--;
							else
								Val += 2;
						Msw = !Msw;
					}
					else
					{
						if (DebugInfo[cputype].RegList[Which].Size > 1)
							Val--;
						else
							Val++;
					}
					Msb = !Msb;
#else
					Val++;
#endif
				}
				break;
			case OSD_KEY_ENTER:
				Editing = FALSE;
				break;
		}
		if (Val >
				((unsigned char *)DebugInfo[cputype].RegList[Which].Val +
				(DebugInfo[cputype].RegList[Which].Size - 1)))
		{
			if (++Which >=  NumRegs)
				Which = 0;

			X = DebugInfo[cputype].RegList[Which].XPos +
				strlen(DebugInfo[cputype].RegList[Which].Name) + 1;
			Y = DebugInfo[cputype].RegList[Which].YPos;
#ifdef	LSB_FIRST
			Val = ((unsigned char *)DebugInfo[cputype].RegList[Which].Val +
				(DebugInfo[cputype].RegList[Which].Size - 1));
			Msw = TRUE;
			Msb = TRUE;
#else
			Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
#endif
			High = TRUE;
		}
		else if (Val < (unsigned char *)DebugInfo[cputype].RegList[Which].Val)
		{
			if (--Which < 0)
				Which = NumRegs - 1;

			X = DebugInfo[cputype].RegList[Which].XPos +
				strlen(DebugInfo[cputype].RegList[Which].Name) + 1 +
				(DebugInfo[cputype].RegList[Which].Size * 2) - 1;
			Y = DebugInfo[cputype].RegList[Which].YPos;
#ifdef	LSB_FIRST
			Val = (unsigned char *)DebugInfo[cputype].RegList[Which].Val;
			Msw = FALSE;
			Msb = FALSE;
#else
			Val = ((unsigned char *)DebugInfo[cputype].RegList[Which].Val +
				(DebugInfo[cputype].RegList[Which].Size - 1));
#endif
			High = FALSE;
		}
	}
	cpuintf[cputype].set_regs(rgs);
}

static int DisplayCommandInfo(char * commandline, char * command)
{
	int i,found,offst, rv = -1;
	char info[90] = "\0";

	sprintf(info, "%-67s", " ");
	ScreenPutString(info, INSTRUCTION_COLOUR, 10, 21);
	sprintf(info, "%-76s", " ");
	ScreenPutString(info, COMMANDINFO_COLOUR, 2, 23);
	sprintf(info, "Found:  ");

	i = 0;
	offst = 0;
	found = 0;
	while (CommandInfo[i].Cmd != cmNOMORE)
	{
		if (memcmp(CommandInfo[i].Name,command,strlen(command)) == 0)
		{
			if (found) strcat(info,", ");
			strcat(info, CommandInfo[i].Name);
			found++;
			offst = i;
		}
		i++;
	}

	if ((found == 1) && (strcmp(CommandInfo[offst].Name,command) == 0))
	{
		sprintf(info, "%-76s", " ");
		sprintf(info, CommandInfo[offst].Description);
		rv = offst;
	}
	else if (!found)
	{
		sprintf(info, "Found:  Nothing!%-60s", " ");
	}

	if (strlen(info) >= 75) { info[75] = 16; info[76] = '\0'; }
	ScreenSetCursor(21,10+strlen(commandline));
	ScreenPutString(commandline, INSTRUCTION_COLOUR, 10, 21);
	ScreenPutString(info, COMMANDINFO_COLOUR, 2, 23);

	return ((rv==-1) ? cmNOMORE : CommandInfo[rv].Cmd);
}


static int GetSPValue(int _cputype)
{
	int rv = 0;
	switch (DebugInfo[_cputype].SPSize)
	{
		case 1: rv = *(byte *)DebugInfo[_cputype].SPReg;
				break;
		case 2: rv = *(word *)DebugInfo[_cputype].SPReg;
				break;
		case 4: rv = *(dword *)DebugInfo[_cputype].SPReg;
				break;
	}
	return rv;
}


/*** Single-step debugger ****************************/
/*** This function should exist if DEBUG is        ***/
/*** #defined. Call from the CPU's Execute function***/
/*****************************************************/
void MAME_Debug (void)
{
	static char		CommandLine[80];
	static char		Command[80];
	static int		cmd = cmNOMORE;
	static int		ResetCommandLine = 0;
	static int		Step;
	static int		DebugTrace = FALSE;
	static int		FirstTime = TRUE;
	int				Key;

	if (DebugFast)
	{
		if (debug_key_pressed)
			DebugFast = 0;
		else
			return;
	}

	*CommandLine = '\0';
	*Command = '\0';

	activecpu = cpu_getactivecpu ();
	cputype = Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK;
	cpuintf[cputype].get_regs(rgs);

	if (traceon)				/* CM 980505 - skip if trace is off */
	{
		/* JB 980214 -- Call CPU trace function */
		if (activecpu != gCurrentTraceCPU)
		{
			gCurrentTraceCPU = activecpu;
			asg_TraceSelect (gCurrentTraceCPU);
		}
		DebugInfo[cputype].Trace(cpu_getpc ());
	}

	if (PreviousSP)
	{
		/* See if we went into a function.  A RET will cause SP to be > than previousSP */
		if ((cpu_getpc() != NextPC) && (GetSPValue(cputype) < PreviousSP))
		{
			/* if so, set a breakpoint on the return PC */
			BreakPoint[activecpu] = NextPC;
			Update = FALSE;
			InDebug = FALSE;
			osd_set_mastervolume(CurrentVolume);
			osd_set_display(Machine->scrbitmap->width,
							Machine->scrbitmap->height,
							Machine->drv->video_attributes);
		}
		else
		{
			/* if not, set trace on so that we break immediately */
			InDebug = TRUE;
		}
		PreviousSP = 0;
	}

	if (FirstTime)
	{
		cpuintf[cputype].get_regs(bckrgs);
		memset(MemWindowBackup,0,0x200);
	}

	/* Check for breakpoint or tilde key to enter debugger */
	if ((BreakPoint[activecpu] != -1 && cpu_getpc()==BreakPoint[activecpu]) ||
		(TempBreakPoint != -1 && cpu_getpc() == TempBreakPoint) ||/*MB 980103*/
		( ( CPUWatchpoint[activecpu] >= 0 ) && ( cpuintf[cputype].memory_read( CPUWatchpoint[activecpu] ) != CPUWatchdata[activecpu] ) ) || /* EHC 980506 */
		debug_key_pressed /* JB 980505 */ || FirstTime || ((CPUBreakPoint >= 0) && (activecpu != CPUBreakPoint)))    /* MB 980121 */
	{
		uclock_t	curr = uclock();

		debug_key_pressed = 0;	/* JB 980505 */

		/* EHC 980506 - Watchpoint support */
		if ( ( ( CPUWatchpoint[activecpu] >= 0 ) && ( cpuintf[cputype].memory_read( CPUWatchpoint[activecpu] ) != CPUWatchdata[activecpu] ) ) ) {
			/* if we dropped in because of a watchpoint, update it with the new value */
			CPUWatchdata[activecpu] = cpuintf[cputype].memory_read( CPUWatchpoint[activecpu] );
		}

		osd_set_mastervolume(0);
		do
		{
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */
		} while (uclock() < (curr + (UCLOCKS_PER_SEC / 15)));

		set_gfx_mode (GFX_TEXT,80,25,0,0);
		TempBreakPoint = -1;
		CPUBreakPoint = -1;	/* MB 980121 */
		PreviousCPUType = !cputype;
		InDebug = TRUE;
		Update = TRUE;
		DebugTrace = FALSE;
		FirstTime = FALSE;
		StartAddress = cpu_getpc ();
		CursorPC = StartAddress;	/* MB 980103 */
	}
	if (Step)
		CursorPC = cpu_getpc ();	/* MB 980103 */
	Step = FALSE;		/* So that we will get back into debugger if needed */
	Update = TRUE;		/* Cause a screen refresh */
	while ((InDebug) && !Step)
	{
		if (Update)
		{
			char s[21];

			if (cputype != PreviousCPUType)
			{
				DebugInfo[cputype].DrawDebugScreen(HEADING_COLOUR, LINE_COLOUR);
				PreviousCPUType = cputype;
			}
			debug_draw_flags ();
			debug_draw_registers ();
			ScreenPutString ("        ", FLAG_COLOUR, DebugInfo[cputype].NamePos, 3);
			sprintf (s, "%d (%s)", activecpu, DebugInfo[cputype].Name);
			ScreenPutString (s, FLAG_COLOUR, DebugInfo[cputype].NamePos, 3);
			CurrentPC = cpu_getpc ();
			if (DebugTrace)
			   StartAddress = CurrentPC;
			if (CurrentPC < StartAddress || CurrentPC >= EndAddress)	/* MB 980103 */
			   StartAddress = CurrentPC;
			EndAddress = debug_draw_disasm (StartAddress);
			DrawMemWindow (MEM1, MEM1_COLOUR, 3, DisplayASCII[0]);	/* MB 980103 */
			DrawMemWindow (MEM2, MEM2_COLOUR, 12, DisplayASCII[1]);	/* MB 980103 */
			ScreenPutString ("Command>", PROMPT_COLOUR, 2, 21);
			ScreenSetCursor (21, 10);
			Update = FALSE;
		}

		if (!DebugTrace)
		{
			char	S[128];
			int		TempAddress;
			int		Line;

			//Key = osd_read_keyrepeat ();
			Key = osd_debug_readkey ();	/* JB 980103 */
			if ((strlen(osd_key_name(Key)) == 1) && (strlen(CommandLine) < 80))
				strcat(CommandLine,osd_key_name(Key));
			else if (((Key == OSD_KEY_SPACE) || (Key == OSD_KEY_ENTER)) && (strlen(CommandLine) < 80))
				strcat(CommandLine," ");
			else if ((Key == OSD_KEY_EQUALS) && (strlen(CommandLine) < 80))
				strcat(CommandLine,"=");
			else if ((Key == OSD_KEY_STOP) && (strlen(CommandLine) < 80))
				strcat(CommandLine,".");
#ifdef macintosh
			else if ((Key == OSD_KEY_SLASH) && (strlen(CommandLine) < 80))
				strcat(CommandLine,"/");
#else
			else if ((Key == OSD_KEY_SLASH) && (strlen(CommandLine) < 80))
				strcat(CommandLine,"\\");
#endif
			else if ((Key == OSD_KEY_BACKSPACE) && (strlen(CommandLine) > 0))
				CommandLine[strlen(CommandLine)-1] = '\0';
			{
				int i;
				Command[0] = CommandLine[0];
				for (i=0; i<=strlen(CommandLine); i++)
				{
					if (CommandLine[i] == ' ')
					{
						memcpy(Command,CommandLine,i);
						Command[i] = ' ';
						Command[i+1] = '\0';
						break;
					}
					else
						Command[i] = CommandLine[i];
				}
			}
			cmd = DisplayCommandInfo(CommandLine,Command);
			if (cmd != cmNOMORE && Key == OSD_KEY_ENTER)
			{
				CommandInfo[cmd].ExecuteCommand(&CommandLine[strlen(CommandInfo[cmd].Name)]);
				if (cmd != cmJ)
					Update = TRUE;
				ResetCommandLine = 1;
			}
			else
			{
				switch (Key)
				{
					case OSD_KEY_F12:
					case OSD_KEY_ESC:
						ResetCommandLine = 1;
						Update = FALSE;
						InDebug = FALSE;
						osd_set_mastervolume(CurrentVolume);
						osd_set_display(Machine->scrbitmap->width,
										Machine->scrbitmap->height,
										Machine->drv->video_attributes);
						break;

					case OSD_KEY_F2:	/* MB 980103 */
						ResetCommandLine = 1;
						if (BreakPoint[activecpu] == -1)
						   BreakPoint[activecpu] = CursorPC;
						else
						   BreakPoint[activecpu] = -1;
						Update = TRUE;
						break;

					case OSD_KEY_F4:	/* MB 980103 */
						ResetCommandLine = 1;
						TempBreakPoint = CursorPC;
						Update = FALSE;
						InDebug = FALSE;
						osd_set_mastervolume(CurrentVolume);
						osd_set_display(Machine->scrbitmap->width,
										Machine->scrbitmap->height,
										Machine->drv->video_attributes);
						break;

					case OSD_KEY_F6:
						CPUBreakPoint = activecpu;

					case OSD_KEY_F8:	/* MB 980118 */
					case OSD_KEY_ENTER:
						ResetCommandLine = 1;
						Step = TRUE;
						break;

					case OSD_KEY_F9:	/* Was T */
						ResetCommandLine = 1;
						DebugTrace = TRUE;
						Step = TRUE;
						break;

					case OSD_KEY_F10:	/* MB 980207 */
						ResetCommandLine = 1;
						NextPC = CurrentPC + debug_dasm_line(CurrentPC,S);
						PreviousSP = GetSPValue(cputype);
						Step = TRUE;
						break;

					case OSD_KEY_F5:	/* MB 980103 */
						ResetCommandLine = 1;
						osd_set_display(Machine->scrbitmap->width,
										Machine->scrbitmap->height,
										Machine->drv->video_attributes);
						updatescreen();
						while (!osd_read_key ()) continue;		/* do nothing */
						set_gfx_mode (GFX_TEXT,80,25,0,0);
						DebugInfo[cputype].DrawDebugScreen(HEADING_COLOUR, LINE_COLOUR);
						Update = TRUE;
						break;

#ifdef ALLOW_LEFT_AND_RIGHT_IN_DASM_WINDOW
					case OSD_KEY_LEFT:		/* Move Cursor back one alignment unit */
											/* CM 980428 */
						ResetCommandLine = 1;
						StartAddress = CursorPC -= DebugInfo[cputype].AlignUnit;
						if (StartAddress < 0) StartAddress = 0;
						EndAddress = debug_draw_disasm (StartAddress);
						break;
#endif /* ALLOW_LEFT_AND_RIGHT_IN_DASM_WINDOW */

					case OSD_KEY_UP:		/* Scroll disassembly up a line */
						ResetCommandLine = 1;
						if ((CursorPC >= StartAddress+debug_dasm_line(StartAddress,S)) &&
							(CursorPC < EndAddress))	/* MB 980103 */
						{
							int previous = StartAddress;
							int tmp = StartAddress;
							while (tmp < EndAddress)
							{
								tmp += debug_dasm_line(previous,S);
								if (tmp == CursorPC)
								{
									CursorPC = previous;
									tmp = EndAddress;
								}
								else
									previous = tmp;
							}
							EndAddress = debug_draw_disasm (StartAddress);
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
							TempAddress = StartAddress - DebugInfo[cputype].MaxInstLen;
							if (TempAddress < 0) TempAddress = 0; /* CM 980428 */

							for (TempAddress = (TempAddress > 0) ? TempAddress : 0;
								 TempAddress < StartAddress; /* CM 980428 */
								 TempAddress += DebugInfo[cputype].AlignUnit)
							  {
								  if ((TempAddress +
									  debug_dasm_line (TempAddress, S)) ==
										  StartAddress)
									  break;
							  }
							  if (TempAddress == StartAddress) /* CM 980428 */
								  TempAddress -= DebugInfo[cputype].AlignUnit;
							  CursorPC = TempAddress;
												  if (CursorPC < StartAddress) StartAddress = CursorPC;
												  EndAddress = debug_draw_disasm (StartAddress);
						}
						break;

					case OSD_KEY_PGUP:			/* Page Up */
					{
						/* This uses a 'rolling window' of start
						   addresses to work out the best address to
						   use to generate the previous pagefull of
						   disassembly - CM 980428 */
						int TempAddresses[16];

						ResetCommandLine = 1;

						TempAddress = StartAddress -
							DebugInfo[cputype].MaxInstLen * 16;

						if (TempAddress < 0) TempAddress = 0;

						for (Line = 0; TempAddress < StartAddress; Line++)
						{
							TempAddresses[Line % 16] = TempAddress;
							TempAddress += debug_dasm_line(TempAddress, S);
						}

						/* If this ever happens, it's because our
						   'MaxInstLen' member is too small for the
						   CPU */
						if (Line < 16)
						{
							StartAddress = TempAddresses[0];
							if (errorlog)
								fprintf(errorlog, "Increase MaxInstLen? Line = %d\n",
										Line);
						}
						else
						{
							StartAddress = TempAddresses[(Line + 1) % 16];
						}

						CursorPC = StartAddress;
						EndAddress = debug_draw_disasm (StartAddress);
						break;
					}

#ifdef ALLOW_LEFT_AND_RIGHT_IN_DASM_WINDOW
					case OSD_KEY_RIGHT:		/* Move Cursor forward one alignment unit */
											/* CM 980428 */
						ResetCommandLine = 1;
						StartAddress = CursorPC += DebugInfo[cputype].AlignUnit;
						EndAddress = debug_draw_disasm (StartAddress);
						break;
#endif /* ALLOW_LEFT_AND_RIGHT_IN_DASM_WINDOW */

					case OSD_KEY_DOWN:		/* Scroll disassembly down a line */
						ResetCommandLine = 1;
						CursorPC += debug_dasm_line (CursorPC, S);	/* MB 980103 */
						if (CursorPC >= EndAddress)
							  StartAddress += debug_dasm_line(StartAddress, S);
						EndAddress = debug_draw_disasm (StartAddress);
						break;

					case OSD_KEY_PGDN:			/* Page Down */
						ResetCommandLine = 1;
						for (Line = 0; Line < 15; Line++)	/* MB 980103 */
						{
							StartAddress += debug_dasm_line(StartAddress,S);
							CursorPC += debug_dasm_line(CursorPC,S);
						}
						EndAddress = debug_draw_disasm (StartAddress);
						break;

				}
			}
			if (ResetCommandLine)
			{
				ResetCommandLine = 0;
				memset(CommandLine,0,80);
				memset(Command,0,80);
				DisplayCommandInfo(CommandLine,Command);
			}
		}
		else
		{
			Step = TRUE;
			if (osd_key_pressed (OSD_KEY_SPACE))
			{
				DebugTrace = FALSE;
			}
		}
	}
}

#endif
