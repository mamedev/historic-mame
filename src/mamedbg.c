/****************************************************************************/
//
// Instructions for use of debugger
//
// Key						Function
//
// Tilde					Enter debugger
// B						Set breakpoint
// D						Delete breakpoint
// S, Space or Enter		Single step
// 1						Edit memory window 1
// 2						Edit memory window 2
// R						Edit Registers
// T						Trace through the code at high speed in debugger
// V						View the output
// C						Continue to run program out of debugger
// Arrows, PgUp, PgDn		Move around in the edit windows and Scrolls
// 							the disassembly window
// ESC						Exit edit window and program
//

#ifdef MAME_DEBUG

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


#if defined (UNIX)
#define uclock_t clock_t
#define	uclock clock
#define UCLOCKS_PER_SEC CLOCKS_PER_SEC
#else
#if defined(WIN32)
#include "uclock.h"
#endif
#endif

#ifndef macintosh
#ifndef WIN32
	#include <conio.h>
	#include <allegro.h>
#endif
#endif

#include "osdepend.h"
#include "driver.h"
#include "osd_dbg.h"
#include "Z80.h"
#include "Z80Dasm.h"
#include "m6809.h"
#include "m6808.h"	/* JB 971018 */
#include "M6502.h"	/* JB 971019 */
#include "M68000.h"	/* JB 971207 */

extern int Dasm6809 (unsigned char *pBase, char *buffer, int pc);
extern int Dasm6808 (unsigned char *base, char *buf, int pc);	/* JB 971018 */
extern int Dasm6502 (char *buf, int pc);	/* JB 971019 */
extern int Dasm68000 (unsigned char *pBase, char *buffer, int pc);

extern int CurrentVolume;

#define	MEM1DEFAULT	0xc000
#define	MEM2DEFAULT	0xc200
#define	HEADING_COLOUR		LIGHTGREEN
#define	LINE_COLOUR			LIGHTCYAN
#define	REGISTER_COLOUR		WHITE
#define	FLAG_COLOUR			WHITE
#define	BREAKPOINT_COLOUR	YELLOW
#define	PC_COLOUR			RED
#define	CODE_COLOUR			WHITE
#define	MEM1_COLOUR			WHITE
#define	MEM2_COLOUR			WHITE
#define	ERROR_COLOUR		RED
#define	PROMPT_COLOUR		CYAN
#define	INSTRUCTION_COLOUR	WHITE
#define	INPUT_COLOUR		WHITE

static int MEM1 = MEM1DEFAULT;	/* JB 971210 */
static int MEM2 = MEM2DEFAULT;	/* JB 971210 */

struct	EditRegs {
	int		X;
	int		Y;
	int		Len;
	int		val;
};

typedef struct
{
	char 	*name;
	int		size;
} tRegdef;

/* JB 971207 */
static tRegdef m68k_reglist[] =
{
	{ "PC", 4 },
	{ "VBR", 4 },
	{ "ISP", 4 },
	{ "USP", 4 },
	{ "SFC", 4 },
	{ "DFC", 4 },
	{ "D0", 4 },
	{ "D1", 4 },
	{ "D2", 4 },
	{ "D3", 4 },
	{ "D4", 4 },
	{ "D5", 4 },
	{ "D6", 4 },
	{ "D7", 4 },
	{ "A0", 4 },
	{ "A1", 4 },
	{ "A2", 4 },
	{ "A3", 4 },
	{ "A4", 4 },
	{ "A5", 4 },
	{ "A6", 4 },
	{ "A7", 4 },
	{ "", -1 }
};

/* JB 971019 */
static tRegdef m6502_reglist[] =
{
	{ "A", 1 },
	{ "X", 1 },
	{ "Y", 1 },
	{ "S", 1 },
	{ "PC", 2 },
	{ "", -1 }
};

/* JB 971018 */
static tRegdef m6808_reglist[] =
{
	{ "A",	1 },
	{ "B",	1 },
	{ "PC",	2 },
	{ "S",	2 },
	{ "X",	2 },
	{ "",	-1 }
};

static tRegdef m6809_reglist[] =
{
	{ "A",	1 },
	{ "B",	1 },
	{ "PC",	2 },
	{ "S",	2 },
	{ "U",	2 },
	{ "X",	2 },
	{ "Y",	2 },
	{ "DP", 1 },
	{ "",	-1 }
};

static tRegdef z80_reglist[] =
{
	{ "AF",	2 },
	{ "HL",	2 },
	{ "DE",	2 },
	{ "BC",	2 },
	{ "PC",	2 },
	{ "SP",	2 },
	{ "IX",	2 },
	{ "IY", 2 },
	{ "",	-1 }
};

/* globals */
static struct EditRegs	edit[32];	/* JB 971210 */
static unsigned char	rgs[CPU_CONTEXT_SIZE];	/* ASG 971105 */
static int				BreakPoint[5] = { -1, -1, -1, -1, -1 };
static int				CurrentPC = -1;
static int				StartAddress = -1;
static int				activecpu, cputype, PreviousCPUType;

/* private functions */
static void DrawDebugScreen (int TextCol, int LineCol);
static int debug_readbyte (int a);
static void debug_writebyte (int a, int v);
static void DrawMemWindow (int Base, int Col, int Offset);	/* JB 971210 */
static int GetNumber (int X, int Y, int Col);
static int ScreenEdit (int XMin, int XMax, int YMin, int YMax, int Col, int BaseAdd);	/* JB 971210 */
static void debug_draw_disasm (int pc);
static int debug_dasm_line (int, char *);
static void debug_draw_cpu (void);
static void debug_draw_flags (void);
static void debug_draw_registers (void);
static void debug_get_regs (void);
static void debug_store_editregs (void);
static int /* numregs */ debug_setup_editregs (void);

static void EditRegisters (struct EditRegs *Edit, int Size, int Col);
static int debug_key_pressed (void);

/* Draw the screen outline */
static void DrawDebugScreen (int TextCol, int LineCol)
{
	int		y;

	ScreenClear();
	switch (cputype)
	{
		case CPU_M68000:	/* JB 971210 */
	ScreenPutString("ษออ           อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป", LineCol, 0, 0);
	ScreenPutString("Registers", TextCol, 4, 0);
	ScreenPutString("บ", LineCol, 0, 1);
	ScreenPutString("บ", LineCol, 79, 1);
	ScreenPutString("ฬออ       อออออออออหออ     อออหออ          อออออออออออออออออออออป              บ", LineCol, 0, 2);
	ScreenPutString("Flags", TextCol, 4, 2);
	ScreenPutString("CPU", TextCol, 23, 2);
	ScreenPutString("Memory 1", TextCol, 34, 2);
	ScreenPutString("บ", LineCol, 0, 3);
	ScreenPutString("บ", LineCol, 19, 3);
	ScreenPutString("บ", LineCol, 30, 3);
	ScreenPutString("บ", LineCol, 64, 3);
	ScreenPutString("บ", LineCol, 79, 3);
	ScreenPutString("ฬออ      ออออออออออสออออออออออน", LineCol, 0, 4);
	ScreenPutString("บ", LineCol, 64, 4);
	ScreenPutString("Code", TextCol, 4, 4);
	ScreenPutString("บ", LineCol, 79, 4);
	for (y = 5; y < 11; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 30, y);
		ScreenPutString("บ", LineCol, 64, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("บ", LineCol, 0, 11);
	ScreenPutString("ฬออ          อออออออออออออออออออออน", LineCol, 30, 11);
	ScreenPutString("บ", LineCol, 79, 11);
	ScreenPutString("Memory 2", TextCol, 34, 11);
	for (y = 12; y < 20; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 30, y);
		ScreenPutString("บ", LineCol, 64, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ฬออ         ออออออออออออออออออสอออออออออออออออออออออออออออออออออสออออออออออออออน", LineCol, 0, 20);
	ScreenPutString("Command", TextCol, 4, 20);
	for (y = 21; y < 24; y++)
	{
		ScreenPutString("บ", LineCol, 0, y);
		ScreenPutString("บ", LineCol, 79, y);
	}
	ScreenPutString("ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ", LineCol, 0, 24);
			break;

		case CPU_Z80:
		case CPU_M6502:
		case CPU_I86:
		case CPU_M6808:
		case CPU_M6809:

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
		break;
	}
}

static int debug_readbyte (int a)
{
	int b = 0;

	switch (cputype)
	{
		case CPU_Z80:
			b = Z80_RDMEM (a);
			break;
		case CPU_M6809:
			b = M6809_RDMEM (a);
			break;
		case CPU_M6808:		/* JB 971018 */
			b = M6808_RDMEM (a);
			break;
		case CPU_M6502:		/* JB 971019 */
			b = Rd6502 (a);
			break;
#if 0
		case CPU_I86:
			b = ReadByte (a);
			break;
#endif
		case CPU_M68000:	/* JB 971207 */
			b = cpu_readmem24 (a & 0xffffff);
			/* get_byte (a); */
			break;
	}
	return b;
}

static void debug_writebyte (int a, int v)
{
	switch (cputype)
	{
		case CPU_Z80:
			Z80_WRMEM (a, v);
			break;
		case CPU_M6809:
			M6809_WRMEM (a, v);
			break;
		case CPU_M6808:		/* JB 971018 */
			M6808_WRMEM (a, v);
			break;
		case CPU_M6502:		/* JB 971019 */
			Wr6502 (a, v);
			break;
#if 0
		case CPU_I86:
			WriteByte (a, v);
			break;
#endif
		case CPU_M68000:	/* JB 971207 */
			cpu_writemem24 (a & 0xffffff, v);
			/* put_byte (a, v); */
			break;
	}
}

/* Draw the Memory dump windows */
static void DrawMemWindow (int Base, int Col, int Offset)	/* JB 971210 */
{
	int		X;
	int		Y;
	char	S[8];

	switch (cputype)	/* JB 971210 */
	{
		case CPU_M68000:
			for (Y = 0; Y < 8; Y++)
			{
				sprintf(S, "%06X:", Base);
				ScreenPutString(S, Col, 32, Y + Offset);
				for (X = 0; X < 8; X++)
				{
					sprintf (S, "%02X", debug_readbyte (Base++));
					ScreenPutString(S, Col, 40 + (X * 3), Y + Offset);
					if (Base > 0xffffff || Base < 0)
						Base = 0x000000;
				}
			}
			break;
		default:
			for (Y = 0; Y < 8; Y++)
			{
				sprintf(S, "%04X:", Base);
				ScreenPutString(S, Col, 25, Y + Offset);
				for (X = 0; X < 16; X++)
				{
					sprintf (S, "%02X", debug_readbyte (Base++));
					ScreenPutString(S, Col, 31 + (X * 3), Y + Offset);
					if (Base > 0xffff || Base < 0)
						Base = 0x0000;	/* JB 971210 */
				}
			}
			break;
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
	while ((Key = (readkey()) & 0xff) != '\r')
	{
		Key = toupper(Key);
		switch(Key)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				if (Pos<15)		/* JB 971207 */
				{
					Num[Pos++] = Key;
					Num[Pos] = '\0';
				}
				break;
			case '\b':
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

/* Edit the memory window */
static int ScreenEdit (int XMin, int XMax, int YMin, int YMax, int Col, int BaseAdd)	/* JB 971210 */
{
	int		Editing = TRUE;
	int		High = TRUE;
	int		Scroll = FALSE;
	char	Num[2];
	char	Digits[] = {"0123456789ABCDEF"};
	int		Key;
	int		X = XMin;
	int		Y = YMin;
	int		row_bytes;		/* JB 971210 */
	int		Add = BaseAdd;	/* JB 971210 */
	int		Base = BaseAdd;	/* JB 971210 */

	if ( (XMax - XMin) < 46 )	/* JB 971210 */
		row_bytes = 8;
	else
		row_bytes = 16;

	while (Editing)
	{
		ScreenSetCursor(Y, X);
		Key = readkey();
		switch((Key & 0xff00) >> 8)
		{
			case 0x48:			/* Up */
				Y--;
				Add -= row_bytes;		/* JB 971210 */
				break;
			case 0x49:			/* Page Up */
				Add -= row_bytes * 8;	/* JB 971210 */
				Base -= row_bytes * 8;	/* JB 971210 */
				Scroll = TRUE;
				break;
			case 0x4B:			/* Left */
				if (High)
				{
					X -= 2;
					Add--;
				}
				else
					X -= 1;
				High = !High;
				break;
			case 0x4D:			/* Right */
				if (High)
					X += 1;
				else
				{
					X += 2;
					Add++;
				}
				High = !High;
				break;
			case 0x50:			/* Down */
				Y++;
				Add += row_bytes;	/* JB 971210 */
				break;
			case 0x51:			/* Page Down */
				Add += row_bytes * 8;	/* JB 971210 */
				Base += row_bytes * 8;	/* JB 971210 */
				Scroll = TRUE;
				break;
			default:
				Key = toupper (Key & 0xff);
				if ((Key >= '0' && Key <= '9') || (Key >='A' && Key <= 'F'))
				{
					byte b = debug_readbyte (Add);

					sprintf (Num, "%c", Key);
					ScreenPutString (Num, Col, X, Y);

					if (High)
					{
						debug_writebyte (Add, (b & 0x0f) | ((strchr (Digits, Key) - Digits) << 4));
						X++;
					}
					else
					{
						debug_writebyte (Add, (b & 0xf0) | (strchr (Digits, Key) - Digits));
						X += 2;
						Add++;
					}
					High = !High;
				}
				else if (Key==0x1b)
				{
					Editing = FALSE;
				}
				break;
		}
		if (X < XMin)
		{
			X = XMax;
			Y--;
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
		if (cputype==CPU_M68000) 	/* JB 971210 */
		{
			if ( (Base<0) || Base > (0xffffff - 16*8) )
			{
				Base = Add = 0;
				Scroll = TRUE;
			}
		}
		else if ( (Base<0) || Base > (0xffff - 16*16) )
		{
			Base = Add = 0x0000;
			Scroll = TRUE;
		}
		if (Scroll)
		{
			DrawMemWindow (Base, Col, YMin);
			Scroll = FALSE;
		}
	}
	return (Base);
}

static void debug_draw_disasm (int pc)
{
	char 	s[100];
	int		i, Colour;

	for (i=0; i<15; i++)
	{
		if (pc==BreakPoint[activecpu])
			Colour = BREAKPOINT_COLOUR;
		else
			if (pc == CurrentPC)
				Colour = PC_COLOUR;
			else
				Colour = CODE_COLOUR;

		if (cputype==CPU_M68000)	/* JB 971207 */
		{
			sprintf(s, "%06X:                     ", pc);
			ScreenPutString (s, Colour, 2, i + 5);
			pc += debug_dasm_line (pc, s);
			ScreenPutString (s, Colour, 10, i + 5);
		}
		else
		{
			sprintf(s, "%04X:                ", pc);
			ScreenPutString (s, Colour, 2, i + 5);
			pc += debug_dasm_line (pc, s);
			ScreenPutString (s, Colour, 8, i + 5);
		}
	}
}

static int debug_dasm_line (int pc, char *s)
{
	int		offset = 0;

	s[0] = '\0';
	switch (cputype)
	{
		case CPU_M6809:
			offset = Dasm6809 (&ROM[pc], s, pc);
			break;
		case CPU_Z80:
			offset = DasmZ80 (s, pc);
			break;
		case CPU_M6808:		/* JB 971018 */
			offset = Dasm6808 (&ROM[pc], s, pc);
			break;
		case CPU_M6502:		/* JB 971019 */
			offset = Dasm6502 (s, pc);
			break;
		case CPU_M68000:	/* JB 971207 */
			offset = Dasm68000 (&ROM[pc], s, pc);
			s[20] = '\0';
			break;
		default:
			break;
	}
	if (cputype != CPU_M68000)	/* JB 971210 */
		s[15] = '\0';
	return (offset);
}

static void debug_draw_cpu (void)
{
	char s[20], name[10];
	int	x = 14;		/* JB 971210 */

	switch (cputype)
	{
		case CPU_Z80:
			strcpy (name, "Z80");
			break;
		case CPU_M6809:
			strcpy (name, "6809");
			break;
		case CPU_M6808:		/* JB 971018 */
			strcpy (name, "6808");
			break;
		case CPU_M6502:		/* JB 971019 */
			strcpy (name, "6502");
			break;
		case CPU_M68000:	/* JB 971207 */
			strcpy (name, "68K");
			x = 21;
			break;
		default:
			name[0] = '\0';
			break;
	}

	ScreenPutString ("        ", FLAG_COLOUR, x, 3);	/* JB 971210 */
	sprintf (s, "%d (%s)", activecpu, name);
	ScreenPutString (s, FLAG_COLOUR, x, 3);				/* JB 971210 */
}

static void debug_draw_flags (void)
{
	char	Flags[17], s[17];
	int		i, cc;
	int		flag_size = 8;

	switch (cputype)
	{
		case CPU_M6809:
			strcpy (Flags, "..H.NZVC");
			cc = ((m6809_Regs *)rgs)->cc;
			break;
		case CPU_Z80:
			strcpy (Flags, "SZ.H.PNC");
			cc = ((Z80_Regs *)rgs)->AF.B.l;
			break;
		case CPU_M6808:	/* JB 971018 */
			strcpy (Flags, "..HINZVC");
			cc = ((m6808_Regs *)rgs)->cc;
			break;
		case CPU_M6502:	/* JB 971019 */
			strcpy (Flags, "NVRBDIZC");
			cc = ((M6502 *)rgs)->P;
			break;
		case CPU_M68000:	/* JB 971207 */
			flag_size = 16;
			strcpy (Flags, "T.S..III...XNZVC");
			cc = ((MC68000_Regs *)rgs)->regs.sr;
			break;
		default:		/* JB 971018 */
			strcpy (Flags, "........");
			cc = 0;
			break;
	}

	if (flag_size==8)		/* JB 971210 */
	{
		for (i=0; i<8; i++, cc <<= 1)
			s[i] = cc & 0x80 ? Flags[i] : '.';
	}
	else
	{
		/* this is probably wrong for little endian machines */
		for (i=0; i<16; i++, cc <<= 1)
			s[i] = cc & 0x8000 ? Flags[i] : '.';
	}
	s[flag_size]='\0';	/* JB 971210 */
	ScreenPutString (s, FLAG_COLOUR, 2, 3);
}

static void debug_draw_registers (void)
{
	m6809_Regs				*r6809;
	m6808_Regs				*r6808;		/* JB 971018 */
	Z80_Regs				*rZ80;
	M6502					*r6502;		/* JB 971019 */
	MC68000_Regs			*r68k;	/* JB 971207 */
	char					s[100];

	switch (cputype)
	{
		case CPU_M6809:
			r6809 = (m6809_Regs *)rgs;
			sprintf (s, "A:%02X B:%02X PC:%04X S:%04X U:%04X X:%04X Y:%04X DP:%02X",
				r6809->a, r6809->b, r6809->pc, r6809->s, r6809->u, r6809->x, r6809->y, r6809->dp );
			break;
		case CPU_M6808:		/* JB 971018 */
			r6808 = (m6808_Regs *)rgs;
			sprintf (s, "A:%02X B:%02X PC:%04X S:%04X X:%04X",
				r6808->a, r6808->b, r6808->pc, r6808->s, r6808->x );
			break;
		case CPU_Z80:
			rZ80 = (Z80_Regs *)rgs;
			sprintf (s, "AF:%04X HL:%04X DE:%04X BC:%04X PC:%04X SP:%04X IX:%04X IY:%04X I:%02X",
				rZ80->AF.W.l, rZ80->HL.W.l, rZ80->DE.W.l, rZ80->BC.W.l, rZ80->PC.W.l,
				rZ80->SP.W.l, rZ80->IX.W.l, rZ80->IY.W.l, rZ80->I);
			break;
		case CPU_M6502:		/* JB 971019 */
			r6502 = (M6502 *)rgs;
			sprintf (s, "A:%02X X:%02X Y:%02X S:%02X PC:%04X", r6502->A, r6502->X, r6502->Y,
				r6502->S, r6502->PC.W);
			break;
		case CPU_M68000:	/* JB 971207 */
			{
				int i;

				r68k = (MC68000_Regs *)rgs;

				for (i=0; i<8; i++)
				{
					sprintf (s, "D%d:%08X", i, r68k->regs.d[i] );
					ScreenPutString (s, REGISTER_COLOUR, 67, 3+i);

					sprintf (s, "A%d:%08X", i, r68k->regs.a[i] );
					ScreenPutString (s, REGISTER_COLOUR, 67, 12+i);
				}

				sprintf (s, "PC:%08X VBR:%08X ISP:%08X USP:%08X SFC:%08X DFC:%08X",
					r68k->regs.pc, r68k->regs.vbr, r68k->regs.isp, r68k->regs.usp,
					r68k->regs.sfc, r68k->regs.dfc);
			}
			break;
		default:
			s[0] = '\0';
			break;
	}
	ScreenPutString ("                                                                       ",
		REGISTER_COLOUR, 2, 1);
	ScreenPutString (s, REGISTER_COLOUR, 2, 1);
}

static void debug_get_regs (void)
{
	switch (cputype)
	{
		case CPU_M6809:
			m6809_GetRegs ((m6809_Regs *)rgs);
			break;
		case CPU_M6808:	/* JB 971018 */
			m6808_GetRegs ((m6808_Regs *)rgs);
			break;
		case CPU_Z80:
			Z80_GetRegs ((Z80_Regs *)rgs);
			break;
		case CPU_M6502:	/* JB 971019 */
			cpu_getcpucontext (activecpu, rgs);
			break;
		case CPU_M68000:/* JB 971207 */
			MC68000_GetRegs ((MC68000_Regs *)rgs);
			break;
		default:
			break;
	}
}

static void debug_store_editregs (void)
{
	m6809_Regs	*r6809;
	m6808_Regs	*r6808;	/* JB 971018 */
	Z80_Regs	*rZ80;
	M6502		*r6502;	/* JB 971019 */
	MC68000_Regs *r68k;	/* JB 971207 */

	switch (cputype)
	{
		case CPU_M68000:	/* JB 971207 */
			r68k = (MC68000_Regs *)rgs;
			r68k->regs.pc = edit[0].val;
			r68k->regs.vbr = edit[1].val;
			r68k->regs.isp = edit[2].val;
			r68k->regs.usp = edit[3].val;
			r68k->regs.sfc = edit[4].val;
			r68k->regs.dfc = edit[5].val;

			r68k->regs.d[0] = edit[6].val;
			r68k->regs.d[1] = edit[7].val;
			r68k->regs.d[2] = edit[8].val;
			r68k->regs.d[3] = edit[9].val;
			r68k->regs.d[4] = edit[10].val;
			r68k->regs.d[5] = edit[11].val;
			r68k->regs.d[6] = edit[12].val;
			r68k->regs.d[7] = edit[13].val;

			r68k->regs.a[0] = edit[14].val;
			r68k->regs.a[1] = edit[15].val;
			r68k->regs.a[2] = edit[16].val;
			r68k->regs.a[3] = edit[17].val;
			r68k->regs.a[4] = edit[18].val;
			r68k->regs.a[5] = edit[19].val;
			r68k->regs.a[6] = edit[20].val;
			r68k->regs.a[7] = edit[21].val;
			MC68000_SetRegs (r68k);
			break;
		case CPU_M6809:
			r6809 = (m6809_Regs *)rgs;
			r6809->a = edit[0].val;
			r6809->b = edit[1].val;
			r6809->pc = edit[2].val;
			r6809->s = edit[3].val;
			r6809->u = edit[4].val;
			r6809->x = edit[5].val;
			r6809->y = edit[6].val;
			r6809->dp = edit[7].val;
			m6809_SetRegs (r6809);
			break;
		case CPU_M6808:		/* JB 971018 */
			r6808 = (m6808_Regs *)rgs;
			r6808->a = edit[0].val;
			r6808->b = edit[1].val;
			r6808->pc = edit[2].val;
			r6808->s = edit[3].val;
			r6808->x = edit[4].val;
			m6808_SetRegs (r6808);
			break;
		case CPU_Z80:		/* JB 971019 */
			rZ80 = (Z80_Regs *)rgs;
			rZ80->AF.W.l = edit[0].val;
			rZ80->HL.W.l = edit[1].val;
			rZ80->DE.W.l = edit[2].val;
			rZ80->BC.W.l = edit[3].val;
			rZ80->PC.W.l = edit[4].val;
			rZ80->SP.W.l = edit[5].val;
			rZ80->IX.W.l = edit[6].val;
			rZ80->IY.W.l = edit[7].val;
			Z80_SetRegs (rZ80);
			break;
		case CPU_M6502:		/* JB 971019 */
			r6502 = (M6502 *)rgs;
			r6502->A = edit[0].val;
			r6502->X = edit[1].val;
			r6502->Y = edit[2].val;
			r6502->S = edit[3].val;
			r6502->PC.W = edit[4].val;
			cpu_setcpucontext (activecpu, rgs);
			break;
		default:
			break;
	}
}

static int /* numregs */ debug_setup_editregs (void)
{
	static unsigned char 	rgs[CPU_CONTEXT_SIZE];	/* ASG 971105 */
	m6809_Regs				*r6809;
	m6808_Regs				*r6808;		/* JB 971018 */
	Z80_Regs				*rZ80;
	M6502					*r6502;		/* JB 971019 */
	MC68000_Regs			*r68k;		/* JB 971207 */
	tRegdef					*rd;
	int						num = 0;

	switch (cputype)
	{
		case CPU_M6809:
			rd = m6809_reglist;
			break;
		case CPU_M6808:		/* JB 971018 */
			rd = m6808_reglist;
			break;
		case CPU_Z80:
			rd = z80_reglist;
			break;
		case CPU_M6502:		/* JB 971019 */
			rd = m6502_reglist;
			break;
		case CPU_M68000:	/* JB 971207 */
			rd = m68k_reglist;
			break;
		default:
			rd = 0;
			break;
	}

	/* build EditRegs table from register list */
	if (rd)
	{
		int x = 2;

		for (num=0; rd->size != -1; num++)
		{
			x += strlen (rd->name) + 1;
			edit[num].X = x;
			edit[num].Y = 1;
			edit[num].Len = 2 * rd->size;
			edit[num].val = 0;
			x += 1 + edit[num].Len;
			rd++;
		}
	}

	switch (cputype)
	{
		case CPU_M6809:
			r6809 = (m6809_Regs *)rgs;
			m6809_GetRegs (r6809);
			edit[0].val = r6809->a;
			edit[1].val = r6809->b;
			edit[2].val = r6809->pc;
			edit[3].val = r6809->s;
			edit[4].val = r6809->u;
			edit[5].val = r6809->x;
			edit[6].val = r6809->y;
			edit[7].val = r6809->dp;
			break;
		case CPU_M6808:		/* JB 971018 */
			r6808 = (m6808_Regs *)rgs;
			m6808_GetRegs (r6808);
			edit[0].val = r6808->a;
			edit[1].val = r6808->b;
			edit[2].val = r6808->pc;
			edit[3].val = r6808->s;
			edit[4].val = r6808->x;
			break;
		case CPU_Z80:
			rZ80 = (Z80_Regs *)rgs;
			Z80_GetRegs (rZ80);
			edit[0].val = rZ80->AF.W.l;
			edit[1].val = rZ80->HL.W.l;
			edit[2].val = rZ80->DE.W.l;
			edit[3].val = rZ80->BC.W.l;
			edit[4].val = rZ80->PC.W.l;
			edit[5].val = rZ80->SP.W.l;
			edit[6].val = rZ80->IX.W.l;
			edit[7].val = rZ80->IY.W.l;
			break;
		case CPU_M6502:		/* JB 971019 */
			r6502 = (M6502 *)rgs;
			cpu_getcpucontext (activecpu, rgs);
			edit[0].val = r6502->A;
			edit[1].val = r6502->X;
			edit[2].val = r6502->Y;
			edit[3].val = r6502->S;
			edit[4].val = r6502->PC.W;
			break;
		case CPU_M68000:	/* JB 971207 */

			/* adjust because some of the registers are displayed down the right side */
			{
				int i;

				for (i=0; i<8; i++)
				{
					/* d0-d7 */
					edit[6+i].X = 70;
					edit[6+i].Y = 3+i;
					/* a0-a7 */
					edit[14+i].X = 70;
					edit[14+i].Y = 12+i;
				}
			}

			r68k = (MC68000_Regs *)rgs;
			MC68000_GetRegs (r68k);
			edit[0].val = r68k->regs.pc;
			edit[1].val = r68k->regs.vbr;
			edit[2].val = r68k->regs.isp;
			edit[3].val = r68k->regs.usp;
			edit[4].val = r68k->regs.sfc;
			edit[5].val = r68k->regs.dfc;

			edit[6].val = r68k->regs.d[0];
			edit[7].val = r68k->regs.d[1];
			edit[8].val = r68k->regs.d[2];
			edit[9].val = r68k->regs.d[3];
			edit[10].val = r68k->regs.d[4];
			edit[11].val = r68k->regs.d[5];
			edit[12].val = r68k->regs.d[6];
			edit[13].val = r68k->regs.d[7];

			edit[14].val = r68k->regs.a[0];
			edit[15].val = r68k->regs.a[1];
			edit[16].val = r68k->regs.a[2];
			edit[17].val = r68k->regs.a[3];
			edit[18].val = r68k->regs.a[4];
			edit[19].val = r68k->regs.a[5];
			edit[20].val = r68k->regs.a[6];
			edit[21].val = r68k->regs.a[7];
			break;
		default:
			break;
	}

	return num;
}

/* Edit registers window */
static void EditRegisters (struct EditRegs *Edit, int Size, int Col)
{
	int		Editing = TRUE;
	int		Pos = 0;
	int		Which = 0;
	char	Num[2];
	char	Digits[] = {"0123456789ABCDEF"};
	int		Key;
	int		X = 0;
	int		Y = 0;

	if (Size<=0) return;

	X = Edit[Which].X;
	Y = Edit[Which].Y;
	Pos = Edit[Which].Len;

	while (Editing)
	{
		ScreenSetCursor (Y, X);
		Key = readkey();
		switch ((Key & 0xff00) >> 8)
		{
			case 0x4B:			/* Left */
				X--;
				Pos++;
				break;
			case 0x4D:			/* Right */
				X++;
				Pos--;
				break;
			case 0x48:			/* Up */	/* JB 971215 */
				if (--Which < 0)
					Which = Size - 1;

				X = Edit[Which].X + Edit[Which].Len - 1;
				Y = Edit[Which].Y;
				Pos = Edit[Which].Len;
				break;
			case 0x50:			/* Down */	/* JB 971215 */
				if (++Which >=  Size)
					Which = 0;

				X = Edit[Which].X;
				Y = Edit[Which].Y;
				Pos = Edit[Which].Len;
				break;
			default:
				Key = toupper (Key & 0xff);
				if ((Key >= '0' && Key <= '9') || (Key >= 'A' && Key <= 'F'))
				{
					switch (Pos - 1)
					{
						case 0:
							Edit[Which].val &= 0xfffffff0;	/* JB 971207 */
							break;
						case 1:
							Edit[Which].val &= 0xffffff0f;	/* JB 971207 */
							break;
						case 2:
							Edit[Which].val &= 0xfffff0ff;	/* JB 971207 */
							break;
						case 3:
							Edit[Which].val &= 0xffff0fff;	/* JB 971207 */
							break;
						case 4:	/* JB 971207 */
							Edit[Which].val &= 0xfff0ffff;
							break;
						case 5:	/* JB 971207 */
							Edit[Which].val &= 0xff0fffff;
							break;
						case 6:	/* JB 971207 */
							Edit[Which].val &= 0xf0ffffff;
							break;
						case 7:	/* JB 971207 */
							Edit[Which].val &= 0x0fffffff;
							break;
					}
					Edit[Which].val |=
						((strchr(Digits,Key) - Digits) << (4 * (Pos - 1)));
					sprintf (Num, "%c", Key);
					ScreenPutString (Num, Col, X, Y);
					X++;
					Pos--;
				}
				else if (Key==0x1b)
				{
					Editing = FALSE;
				}
				break;
		}
		if (Pos > Edit[Which].Len)
		{
			if (--Which < 0)
				Which = Size - 1;

			X = Edit[Which].X + Edit[Which].Len - 1;
			Y = Edit[Which].Y;
			Pos = 1;
		}
		else if (Pos < 1)
		{
			if (++Which >=  Size)
				Which = 0;

			X = Edit[Which].X;
			Y = Edit[Which].Y;
			Pos = Edit[Which].Len;
		}
	}
}

static int debug_key_pressed (void)
{
	static int	delay = 0;

	if (++delay==0xffff)
	{
		delay = 0;
		return osd_key_pressed (OSD_KEY_TILDE);
	}
	else
		return FALSE;
}

/*** Single-step debugger ****************************/
/*** This function should exist if DEBUG is        ***/
/*** #defined. Call from the CPU's Execute function***/
/*****************************************************/
void MAME_Debug (void)
{
	static int				InDebug=FALSE;
	static int				Update;
	static int				Step;
	static int				DebugTrace = FALSE;
	static int				FirstTime = TRUE;
	int						Key;
	int						InvalidKey;

	activecpu = cpu_getactivecpu ();
	cputype = Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK;
	debug_get_regs ();

	/* Check for breakpoint or tilde key to enter debugger */
	if (cpu_getpc()==BreakPoint[activecpu] || debug_key_pressed () ||
			FirstTime)
	{
		uclock_t	curr = uclock();

		osd_set_mastervolume(0);
		do
		{
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */
		} while (uclock() < (curr + (UCLOCKS_PER_SEC / 15)));

		clear_keybuf ();
		set_gfx_mode (GFX_TEXT,80,25,0,0);
		PreviousCPUType = !cputype;
		InDebug = TRUE;
		Update = TRUE;
		DebugTrace = FALSE;
		FirstTime = FALSE;
		StartAddress = cpu_getpc ();
	}
	if (Step)
		StartAddress = cpu_getpc ();
	Step = FALSE;		/* So that we will get back into debugger if needed */
	Update = TRUE;		/* Cause a screen refresh */
	while ((InDebug) && !Step)
	{
		if (Update)
		{
			if (cputype != PreviousCPUType)
			{
				DrawDebugScreen (HEADING_COLOUR, LINE_COLOUR);
				PreviousCPUType = cputype;
			}
			debug_draw_flags ();
			debug_draw_registers ();
			debug_draw_cpu ();
			CurrentPC = cpu_getpc ();
			if (DebugTrace)
				StartAddress = CurrentPC;
			debug_draw_disasm (StartAddress);
			DrawMemWindow (MEM1, MEM1_COLOUR, 3);
			DrawMemWindow (MEM2, MEM2_COLOUR, 12);
			ScreenPutString ("Command>", PROMPT_COLOUR, 2, 21);
			ScreenSetCursor (21, 10);
			Update = FALSE;
		}

		if (!DebugTrace)
		{
			char	S[128];
			int		TempAddress;
			int		MaxInstLen;
			int		Line;

			Key = readkey ();
			InvalidKey = FALSE;		/* Assume a valid key */
			sprintf (S, "%c", Key);
			ScreenPutString(S, INPUT_COLOUR, 10, 21);
			ScreenPutString("                                              ", INSTRUCTION_COLOUR, 2, 22);

			switch (toupper (Key & 0xff))
			{
				case '1':
					if (cputype==CPU_M68000)	/* JB 971210 */
						MEM1 = ScreenEdit (40, 62, 3, 10, MEM1_COLOUR, MEM1);
					else
						MEM1 = ScreenEdit (31, 77, 3, 10, MEM1_COLOUR, MEM1);
					Update = TRUE;
					break;
				case '2':
					if (cputype==CPU_M68000)	/* JB 971210 */
						MEM2 = ScreenEdit (40, 62, 12, 19, MEM2_COLOUR, MEM2);
					else
						MEM2 = ScreenEdit (31, 77, 12, 19, MEM2_COLOUR, MEM2);
					Update = TRUE;
					break;
				case 'B':
					ScreenPutString ("Enter new breakpoint address", INSTRUCTION_COLOUR, 2, 22);
					BreakPoint[activecpu] = GetNumber (10, 21, INPUT_COLOUR);
					Update = TRUE;
					break;
				case 'C':
				case 27:	/* ESC */
					Update = FALSE;
					InDebug = FALSE;
					osd_set_mastervolume(CurrentVolume);
					osd_set_display(Machine->scrbitmap->width,
							Machine->scrbitmap->height,
							Machine->drv->video_attributes);
					break;
				case 'D':
					BreakPoint[activecpu] = -1;
					Update = TRUE;
					break;
				case 'R':
					EditRegisters (edit, debug_setup_editregs (), REGISTER_COLOUR);
					debug_store_editregs ();
					Update = TRUE;
					break;
				case 'S':
				case ' ':
				case '\r':
					Step = TRUE;
					break;
				case 'T':
					DebugTrace = TRUE;
					Step = TRUE;
					break;
				case 'V':
					osd_set_display(Machine->scrbitmap->width,
							Machine->scrbitmap->height,
							Machine->drv->video_attributes);
					updatescreen();
					while (!readkey ())
						continue;		/* do nothing */
					set_gfx_mode (GFX_TEXT,80,25,0,0);
					DrawDebugScreen (HEADING_COLOUR, LINE_COLOUR);
					Update = TRUE;
					break;
				default:
					switch (cputype)
					{
						case CPU_M6809:
							MaxInstLen = 5;	/* JB 971207 */
							break;
						case CPU_M6808:		/* JB 971018 */
							MaxInstLen = 4;
							break;
						case CPU_Z80:
							MaxInstLen = 4;
							break;
						case CPU_M6502:		/* JB 971019 */
							MaxInstLen = 3;
							break;
						case CPU_M68000:	/* JB 971207 */
							MaxInstLen = 5;
							break;
						default:
							MaxInstLen = 1;
							break;
					}
					switch ((Key & 0xff00) >> 8)
					{
						case 0x48:		/* Scroll disassembly up a line */
							/*
							 * Try to find the previous instruction by searching
							 * from the longest instruction length towards the
							 * current address.  If we can't find one then
							 * just go back one byte, which means that a
							 * previous guess was wrong.
							 */
							for (TempAddress = StartAddress - MaxInstLen;
									TempAddress < StartAddress; TempAddress++)
							{
								if ((TempAddress +
									debug_dasm_line (TempAddress, S)) ==
										StartAddress)
									break;
							}
							if (TempAddress == StartAddress)
								TempAddress--;
							StartAddress = TempAddress;
							Update = TRUE;
							break;

						case 0x49:			/* Page Up */
							for (Line = 0; Line < 15; Line++)
							{
								for (TempAddress = StartAddress - MaxInstLen;
										TempAddress <StartAddress;TempAddress++)
								{
									if ((TempAddress +
										debug_dasm_line (TempAddress, S)) ==
											StartAddress)
										break;
								}
								if (TempAddress == StartAddress)
									TempAddress--;
								StartAddress = TempAddress;
							}
							Update = TRUE;
							break;

						case 0x50:		/* Scroll disassembly down a line */
							StartAddress += debug_dasm_line (StartAddress, S);
							Update = TRUE;
							break;

						case 0x51:			/* Page Down */
							for (Line = 0; Line < 15; Line++)
								StartAddress += debug_dasm_line(StartAddress,S);
							Update = TRUE;
							break;

						default:
							InvalidKey = TRUE;
							break;
					}
			}
			if (InvalidKey)
			{
				ScreenPutString ("Unknown Command", ERROR_COLOUR, 2, 22);
			}
			else
			{
				ScreenPutString (" ", INPUT_COLOUR, 10, 21);
				ScreenPutString ("                                             ",
					INSTRUCTION_COLOUR, 2, 22);
			}
		}
		else
		{
			Step = TRUE;
			if (keypressed ())
			{
				clear_keybuf ();
				DebugTrace = FALSE;
			}
		}
	}
}

#endif
