/*********************************************************************

  cheat.c

*********************************************************************/

/*|*\
|*|  Modifications by John Butler
|*|
|*|	JB 980407:	Changes to support 68K processor and 24-bit addresses.
|*|			This also provided a way to make the cheat system "aware" of
|*|			RAM areas and have it ignore ROM areas.
|*|
|*|	JB 980424:	Many changes for more flexibility and more safeguards:
|*|			- RD_GAMERAM now calls cpu_readmemXX (after swapping in the
|*|			  correct cpu memory context) for safer memory accesses.
|*|			- References to addresses are limited to appropriate ranges
|*|			  for the cpu involved.
|*|			- Watches are still only allowed for cpu 0, but adding
|*|			  support for other cpus will be much easier now.
|*|			- I86 cpu (which uses 20-bit addresses) should be handled
|*|			  properly now.
|*|			- CPUs addressing more than 24-bits will not work as-is,
|*|			  but support wouldn't be too difficult to add (if MAME
|*|			  ever emulates a cpu like that).
|*|			- Searches for new cheats are still only allowed for cpu 0,
|*|			  but adding support for other cpus will be easier now.
|*|			  However, cheats are allowed for other cpus, and should
|*|			  always be correctly handled now.
|*|			- Optimized some code for readability and code size.
|*|
|*|			TO DO (not necessarily by me):
|*|			  - show non-RAM addresses in watches in a different color
|*|			  - support watches in cpus other than 0
|*|			  - support searches for new cheats in cpus other than 0
|*|			  - reduce code size by combining redundant code
|*|
|*|			CAVEATS:
|*|			  - Code is now heavily dependent on certain structures,
|*|			    defines, functions and assumptions in the main code,
|*|				particulary memory-related. Watch out if they change!
|*|
|*|	JB 980505:	Changes to help functions to make more readable, easier
|*|			to maintain, less redundant.
|*|
|*| JB 980506:	New tables osd_key_chars[] and osd_key_caps[], rewrite
|*|			in EditCheat() to reduce code size, improve readability.
|*|
|*|	Modifications/Thoughts By James R. Twine
|*|
|*|	JRT1:	These modifications fix the redraw problem with the watches,
|*|			a drawing bug that can sometimes cause a crash, and adjusts
|*|			the drawing location when a watch is added, if the added
|*|			watch would be drawn off screen.
|*|
|*|	JRT2:	These modifications allow checking for Hex values when
|*|			searching for cheats.  Having a decimal limit limits the
|*|			values that can be found.
|*|
|*|	JRT3:	This modification clears the screen after using <F7> to
|*|			enable and disable cheats on the fly.  This prevents the
|*|			"CHEAT ON/OFF" message from hanging around on the vector
|*|			games.  It causes a slight "blink", but does no more harm
|*|			than using "Pause" on the vector games.  It also prevents
|*|			handling <F7> if no cheats are loaded.
|*|
|*|	JRT5:	Geeze...  Where to start?  Changed some of the text/
|*|			prompts to make terms consistant.  "Special" changed to
|*|			"Type", because it IS the type of cheat.  Menu prompts
|*|			changed, and spacing altered.  In-Application help added
|*|			for the Cheat List and the Cheat Search pages.
|*|
|*|			Support for <SHIFT> modified characters in Edit Mode, name
|*|			limit reduced to 25 while editing, to prevent problems.
|*|			When an edit is started, the existing name comes in and
|*|			is the default new name.  Makes editing mistakes faster.
|*|
|*|			Changes made that display different text depending on
|*|			if a search is first continued (after initialization), or
|*|			further down the line, to prevent confusion.
|*|
|*|			Changed all UPPERCASE ONLY TEXT to Mixed Case Text, so
|*|			that the prompts are a little more friendly.
|*|
|*|			Some of the Menus that I modified could be better, if you
|*|			have a better design, please...  IMPLEMENT IT!!! :)
|*|
|*|			Most of the changes are bracketed by "JRT5", but I KNOW
|*|			that I missed a few...  Sorry.
|*|
|*|	JRT6:	Slight modifications to change display and manipulation
|*|			of Addresses so that the user cannot manipulate addresses
|*|			that are out of the CPUs memory space.
|*|
|*|			Additional Variables created for data display and
|*|			manipulation.
|*|
|*|
|*|	Thoughts:
|*|			Could the cheat system become "aware" of the RAM and ROM
|*|			area(s) of the games it is working with?  If yes, it would
|*|			prevent the cheat system from checking ROM areas (unless
|*|			specifically requested to do so).  This would prevent false
|*|			(static) locations from popping up in searches.
|*|
|*|			Or, could the user specify a specific area to limit the
|*|			search to?  This would provide the same effect, but would
|*|			require the user to know what (s)he is doing.
|*|
|*|	James R. Twine (JRT)
\*|*/

#include "driver.h"
#include <stdarg.h>
#include <ctype.h>  /* for toupper() and tolower() */

/*#define ram_size 65536    JB 980407   */

/*      --------- Mame Action Replay version 1.00 ------------        */

/*

Wats new in 1.00:
 -The modifications by James R. Twine listed at the beginning of the file
  -He also revised the Cheat.doc (Thanks!)
 -You can now edit a cheat with F3 in the cheat list
 -You can insert a new empty cheat in the list by pressing Insert


To do:
 -Scan the ram of all the CPUs (maybe not?)
 -Do something for the 68000						[DONE JB 980424]
  -We will have to detect where is the ram.			[DONE JB 980424]
  -Allocate variable length table					[DONE JB 980424]
  -32 bit addressing								[DONE JB 980424]
 -Probably will have problem with a full 8086 		[DONE JB 980424]
  (more than 64K)


I do not know if this will work with all the games because the value
 must be in Machine->memory_region[0]
 Or should I call a routine to read/write in ram instead of going
 directly to Machine->memory_region[0]


The CHEAT.DAT file:
 -This file should be in the same directory of MAME.EXE .

 -This file can be edited with a text editor, but keep the same format:
    all fields are separated by a colon (:)
  -Name of the game (short name)
  -No of the CPU
  -Address in Hexadecimal
  -Data to put at this address in Hexadecimal
  -Special codes usually 0
  -Description of the cheat (30 caracters max)


Special codes:
 0-Normal, update ram when DoCheat is Called
 1-Write in ram just one time then delete cheat from active list
 2-Wait a second between 2 writes
 3-Wait 2 second between 2 writes
 4-Wait 5 second between 2 writes
 5-When the original value is not like the cheat, wait 1 second then write it
    Needed by Tempest for the Super Zapper
 6-When the original value is not like the cheat, wait 2 second then write it
 7-When the original value is not like the cheat, wait 5 second then write it
 8-Do not change if value decrease by 1 each frames
 9-Do not change if value decrease by 2 each frames
10-Do not change if value decrease by 3 each frames
11-Do not change if value decrease by 4 each frames

*/

struct cheat_struct {
  int CpuNo;
  int Address;
  int Data;
  int Special; /* Special function usually 0 */
  int Count;
  int Backup;
  char Name[80];
};

static int CheatTotal;
static struct cheat_struct CheatTable[11];
static int LoadedCheatTotal;
static struct cheat_struct LoadedCheatTable[101];

static unsigned int Watches[10];
static int WatchesFlag;
static int WatchX,WatchY;

#if 0 /* JB 980407 */
static unsigned char *StartRam;
static unsigned char *BackupRam;
static unsigned char *FlagTable;
#endif
static int StartValue;
static int CurrentMethod;

/* JRT5 */
static int	iCheatInitialized = 0;
void		CheatListHelp( void );
void		EditCheatHelp( void );
void		StartSearchHelp( void );
/* JRT5 */

#if 0
/* JRT6 */
static int	iCPUType = 0x0; 					/* removed JB 980424 */
static int	iWatchUnusedValue = 0x0;			/* removed JB 980424 */
char		*pAddrTemplate = "Addr:    %04X\0\0";	/* removed JB 980424 */
char		*pShortAddrTemplate = "  $%04X";		/* removed JB 980424 */
/* JRT6 */
#endif

static int CheatEnabled;
int he_did_cheat;

/* START JB 980506 */
static unsigned char osd_key_chars[] =
{
/* 0    1    2    3    4    5    6    7    8    9 */
   0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8',	/* 0*/
  '9', '0', '-', '=',  0 ,  0,  'q', 'w', 'e', 'r',	/* 1*/
  't', 'y', 'u', 'i', 'o', 'p', '[', ']',  0 ,  0 ,	/* 2*/
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 3*/
 '\'', '`',  0 ,  0 , 'z', 'x', 'c', 'v', 'b', 'n', /* 4*/
  'm', ',', '.', '/',  0 , '8',  0 , ' ',  0 ,  0 ,	/* 5*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 6*/
   0 ,  0 ,  0 ,  0 , '-',  0 , '5',  0 , '+',  0 , /* 7*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 8*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 9*/
   0 , '1', '2', '3', '4',  0 , '6', '7', '8', '9',	/* 10*/
  '0',  0 , '=', '/', '*',  0
};

static unsigned char osd_key_caps[] =
{
/* 0    1    2    3    4    5    6    7    8    9 */
   0 ,  0 , '!', '@', '#', '$', '%', '^', '&', '*',	/* 0*/
  '(', ')', '_', '+',  0 ,  0 , 'Q', 'W', 'E', 'R',	/* 1*/
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  0 ,  0 ,	/* 2*/
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 3*/
  '"', '~',  0 ,  0 , 'Z', 'X', 'C', 'V', 'B', 'N',	/* 4*/
  'M', '<', '>', '?',  0 , '*',  0 , ' ',  0 ,  0 ,	/* 5*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 6*/
   0 ,  0 ,  0 ,  0 , '-',  0 , '5',  0 , '+',  0 , /* 7*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 8*/
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , /* 9*/
   0 , '1', '2', '3', '4',  0 , '6', '7', '8', '9',	/* 10*/
  '0',  0 , '=', '/', '*',  0
};
/* END JB 980506 */

/* START JB 980424 */

#define MAX_ADDRESS(cpu)	(0xFFFFFFFF >> (32-ADDRESS_BITS(cpu)))

/* macros stolen from memory.c for our nefarious purposes: */
#define MEMORY_READ(index,offset)       ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].memory_read)(offset))
#define MEMORY_WRITE(index,offset,data) ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].memory_write)(offset,data))
#define ADDRESS_BITS(index)             (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].address_bits)

/* return a format specifier for printf based on cpu address range */
static char * pAddrTemplate (int cpu)
{
	static char s[32];

	switch (ADDRESS_BITS(cpu) >> 2)
	{
		case 4:
			strcpy (s, "Addr:    $%04X");
			break;
		case 5:
			strcpy (s, "Addr:   $%05X");
			break;
		case 6:
		default:
			strcpy (s, "Addr:  $%06X");
			break;
	}
	return s;
}

/* return a format specifier for printf based on cpu address range */
static char * pShortAddrTemplate (int cpu)
{
	static char s[32];

	switch (ADDRESS_BITS(cpu) >> 2)
	{
		case 4:
			strcpy (s, "  $%04X");
			break;
		case 5:
			strcpy (s, " $%05X");
			break;
		case 6:
		default:
			strcpy (s, "$%06X");
			break;
	}
	return s;
}

/* END JB 980424 */

/* START JB 980407 */
static struct ExtMemory StartRam[MAX_EXT_MEMORY];
static struct ExtMemory BackupRam[MAX_EXT_MEMORY];
static struct ExtMemory FlagTable[MAX_EXT_MEMORY];

#define RD_GAMERAM(cpu,a)	read_gameram(cpu,a)
#define WR_GAMERAM(cpu,a,v)	write_gameram(cpu,a,v)
#define RD_STARTRAM(a)		read_ram(StartRam,a)
#define WR_STARTRAM(a,v)	write_ram(StartRam,a,v)
#define RD_BACKUPRAM(a)		read_ram(BackupRam,a)
#define WR_BACKUPRAM(a,v)	write_ram(BackupRam,a,v)
#define RD_FLAGTABLE(a)		read_ram(FlagTable,a)
#define WR_FLAGTABLE(a,v)	write_ram(FlagTable,a,v)

extern unsigned char *memory_find_base (int cpu, int offset);

#if 0
/* removed JB 980424 */
/* read a byte from CPU 0 ram at address <add> */
static unsigned char read_gameram (int cpu, int add)
{
	return *(unsigned char *)memory_find_base (cpu, add);
}
#endif

/* modified JB 980424 */
/* read a byte from cpu at address <add> */
static unsigned char read_gameram (int cpu, int add)
{
	int save = cpu_getactivecpu();
	unsigned char data;

/* This was causing crashes, most likely because when no cpu is active,
   activecpu is -1 in cpuintrf.c, but cpu_getactivecpu() returns 0. So
   this code wasn't swapping in cpu 0's memory context because it thought
   it was already the current context. Consequently, it was probably
   calling cpu_readmem24() on a 6502 memory region -- kaboom. */
/*	if (cpu != save)*/
		memorycontextswap(cpu);

	data = MEMORY_READ(cpu,add);

	if (cpu != save)
		memorycontextswap(save);

	return data;
}

/* write a byte to CPU 0 ram at address <add> */
static void write_gameram (int cpu, int add, unsigned char data)
{
	*(unsigned char *)memory_find_base (cpu, add) = data;
}

/* read a byte representing the address <offset> from one of the tables */
static unsigned char read_ram (struct ExtMemory *table, int offset)
{
	struct ExtMemory *ext;

	for (ext = table; ext->data; ext++)
		if (ext->start <= offset && ext->end >= offset)
			return *(ext->data + (offset - ext->start));

	/* address requested is not one that we have -- shrug */
	return 0;
}

/* make a copy of each ram area from CPU 0 ram to the specified table */
static void backup_ram (struct ExtMemory *table)
{
	struct ExtMemory *ext;
	unsigned char *gameram;

	for (ext = table; ext->data; ext++)
	{
		gameram = memory_find_base (0, ext->start);
		memcpy (ext->data, gameram, ext->end - ext->start + 1);
	}
}

/* set every byte in specified table to data */
static void memset_ram (struct ExtMemory *table, unsigned char data)
{
	struct ExtMemory *ext;

	for (ext = table; ext->data; ext++)
		memset (ext->data, data, ext->end - ext->start + 1);
}

/* free all the memory and init the table */
static void reset_table (struct ExtMemory *table)
{
	struct ExtMemory *ext;

	for (ext = table; ext->data; ext++)
		free (ext->data);
	memset (table, 0, sizeof (struct ExtMemory) * MAX_EXT_MEMORY);
}

/* create tables for storing copies of all MRA_RAM areas of CPU 0 */
static int build_tables (void)
{
	const struct MemoryReadAddress *memoryread;
	const struct MemoryReadAddress *mra;
	struct ExtMemory *ext_sr = StartRam;
	struct ExtMemory *ext_br = BackupRam;
	struct ExtMemory *ext_ft = FlagTable;

	memoryread = Machine->drv->cpu[0].memory_read;

	/* memory read handler build */
	mra = memoryread;
	while (mra->start != -1) mra++;
	mra--;

	while (mra >= memoryread)
	{
		int (*handler)(int) = mra->handler;
		int size;

		switch ((FPTR)handler)
		{
			/* only track RAM areas */
			case (FPTR)MRA_RAM:
			case (FPTR)MRA_BANK1:
			case (FPTR)MRA_BANK2:
			case (FPTR)MRA_BANK3:
			case (FPTR)MRA_BANK4:
			case (FPTR)MRA_BANK5:
			case (FPTR)MRA_BANK6:
			case (FPTR)MRA_BANK7:
			case (FPTR)MRA_BANK8:
				size = mra->end - mra->start + 1;

				/* time to allocate */
				ext_sr->start = ext_br->start = ext_ft->start = mra->start;
				ext_sr->end = ext_br->end = ext_ft->end = mra->end;
				ext_sr->region = ext_br->region = ext_ft->region = 0;
				ext_sr->data = malloc (size);
				ext_br->data = malloc (size);
				ext_ft->data = malloc (size);

				/* if that fails, we're through */
				if (ext_sr->data==NULL || ext_br->data==NULL || ext_ft->data==NULL)
					return 1;

				/* reset the memory */
				memset (ext_sr->data, 0, size);
				memset (ext_br->data, 0, size);
				memset (ext_ft->data, 0, size);

				ext_sr++, ext_br++, ext_ft++;
				break;
		}
		mra--;
	}
	return 0;
}
/* END JB 980407 */

void xprintf(int x,int y,char *fmt,...);
void ContinueCheat(void);


/*****************
 * Init some variables
 */
void InitCheat(void)
{
 FILE *f;
 char *ptr;
 char str[80];
 int i;

  he_did_cheat = 0;
  CheatEnabled = 0;

  CheatTotal = 0;
  LoadedCheatTotal = 0;
  CurrentMethod = 0;

/* JB 980407 */
#if 0
  StartRam = NULL;
  BackupRam = NULL;
  FlagTable = NULL;
#else
  reset_table (StartRam);
  reset_table (BackupRam);
  reset_table (FlagTable);
#endif

/* JRT6	- modified JB 980424 */
	for(i=0;i<10;i++)
		Watches[ i ] = MAX_ADDRESS(0);			/* Set Unused Value For Watch*/
/* JRT6 */

  WatchX = Machine->uixmin;
  WatchY = Machine->uiymin;
  WatchesFlag = 0;

/* Load the cheats for that game */
/* Ex.: pacman:0:4e14:6:0:Infinite Lives  */
  if ((f = fopen("CHEAT.DAT","r")) != 0){
    for(;;){

      if(fgets(str,80,f) == NULL)
        break;

      #ifdef macintosh  /* JB 971004 */
      /* remove extraneous LF on Macs if it exists */
      if( str[0] == '\r' )
        strcpy( str, &str[1] );
      #endif

      if(str[strlen(Machine->gamedrv->name)] != ':')
        continue;
      if(strncmp(str,Machine->gamedrv->name,strlen(Machine->gamedrv->name)) != 0)
        continue;

      if(str[0] == ';') /*Comments line*/
        continue;

      if(LoadedCheatTotal >= 99){
        break;
      }

/*Extract the fields from the string*/
      ptr = strtok(str, ":");
      ptr = strtok(NULL, ":");
      sscanf(ptr,"%d", &LoadedCheatTable[LoadedCheatTotal].CpuNo);

      ptr = strtok(NULL, ":");
      sscanf(ptr,"%x", &LoadedCheatTable[LoadedCheatTotal].Address);

      ptr = strtok(NULL, ":");
      sscanf(ptr,"%x", &LoadedCheatTable[LoadedCheatTotal].Data);

      ptr = strtok(NULL, ":");
      sscanf(ptr,"%d", &LoadedCheatTable[LoadedCheatTotal].Special);

      ptr = strtok(NULL, ":");
      strcpy(LoadedCheatTable[LoadedCheatTotal].Name,ptr);
/*Chop the CRLF at the end of the string */
      LoadedCheatTable[LoadedCheatTotal].Name[strlen(LoadedCheatTable[LoadedCheatTotal].Name)-1] = 0;

      LoadedCheatTotal++;
    }
    fclose(f);
  }
}

/*****************
 * Free allocated arrays
 */
void StopCheat(void)
{

/* JB 980407 */
#if 0
  if(StartRam != NULL)
    free(StartRam);
  if(BackupRam != NULL)
    free(BackupRam);
  if(FlagTable != NULL)
    free(FlagTable);
#else
	reset_table (StartRam);
	reset_table (BackupRam);
	reset_table (FlagTable);
#endif
}

/*****************
 * The routine called in the emulation
 * Modify some memory location
 * Put some function to ` and F7
 */
void DoCheat(int CurrentVolume)
{
	int i,j,y;
	char buf[80];
	char buf2[10];

	/* Display watches if there is some */
	if(WatchesFlag != 0)
	{
		int trueorientation;

		/* hack: force the display into standard orientation */
		trueorientation = Machine->orientation;
		Machine->orientation = ORIENTATION_DEFAULT;

		buf[0] = 0;
		for (i=0;i<10;i++)
		{
			if( Watches[ i ] != MAX_ADDRESS(0)) 		/* If Watch Is In Use*/
			{
				/*sprintf(buf2,"%02X ",Machine->memory_region[0][Watches[i]]);*/
				sprintf(buf2,"%02X ", RD_GAMERAM (0, Watches[i])); /* JB 980407 */
				strcat(buf,buf2);
			}
		}

		for (i = 0;i < (int)strlen(buf);i++)
			drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				WatchX+(i*Machine->uifont->width),WatchY,0,TRANSPARENCY_NONE,0);

		Machine->orientation = trueorientation;
	}

	/* Affect the memory */
	for(i=0; CheatEnabled==1 && i<CheatTotal;i++) /* JB 980407 */
	{
		if(CheatTable[i].Special == 0)
		{
			/* JB 980407 */
			WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
		}
		else
		{
			if(CheatTable[i].Count == 0)
			{
				/* Check special function */
				switch(CheatTable[i].Special)
				{
					case 1:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							CheatTable[i].Data);

						/*Delete this cheat from the table*/
						for(j = i;j<CheatTotal-1;j++)
						{
							CheatTable[j].CpuNo = CheatTable[j+1].CpuNo;
							CheatTable[j].Address = CheatTable[j+1].Address;
							CheatTable[j].Data = CheatTable[j+1].Data;
							CheatTable[j].Special = CheatTable[j+1].Special;
							CheatTable[j].Count = CheatTable[j+1].Count;
							strcpy(CheatTable[j].Name,CheatTable[j+1].Name);
						}
						CheatTotal--;
						break;
					case 2:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							CheatTable[i].Data);
						CheatTable[i].Count = 1*60;
						break;
					case 3:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							CheatTable[i].Data);
						CheatTable[i].Count = 2*60;
						break;
					case 4:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							CheatTable[i].Data);
						CheatTable[i].Count = 5*60;
						break;

					/* 5,6,7 check if the value has changed, if yes, start a timer
					    when the timer end, change the location*/
					case 5:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1*60;
							CheatTable[i].Special = 100;
						}
						break;
					case 6:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 2*60;
							CheatTable[i].Special = 101;
						}
						break;
					case 7:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 5*60;
							CheatTable[i].Special = 102;
						}
						break;

					/* 8,9,10,11 do not change the location if the value change by X every frames
					   This is to try to not change the value of an energy bar
					   when a bonus is awarded to it at the end of a level
					   See Kung Fu Master*/
					case 8:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 103;
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 9:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 104;
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 10:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 105;
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 11:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 106;
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;

						/*Special case, linked with 5,6,7 */
					case 100:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 5;
						break;
					case 101:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 6;
						break;
					case 102:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 7;
						break;

					/*Special case, linked with 8,9,10,11 */
					/* Change the memory only if the memory decreased by X */
					case 103:
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-1)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 8;
						break;
					case 104:
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-2)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 9;
						break;
					case 105:
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-3)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 10;
						break;
					case 106:
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-4)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 11;
						break;
          		}	/* end switch */
        	} /* end if(CheatTable[i].Count == 0) */
        	else
			{
				CheatTable[i].Count--;
			}
		} /* end else */
	} /* end for */


/* ` continue cheat, to accelerate the search for new cheat */
#if 0
  if (osd_key_pressed(OSD_KEY_TILDE)){
    osd_set_mastervolume(0);
    while (osd_key_pressed(OSD_KEY_TILDE))
      osd_update_audio(); /* give time to the sound hardware to apply the volume change */

    ContinueCheat();

    osd_set_mastervolume(CurrentVolume);
    (Machine->drv->vh_update)(Machine->scrbitmap);  /* Make Game Redraw Screen */
  }
#endif

/* F7 Enable/Disable the active cheats on the fly. Required for some cheat */
/* JRT3 10-26-97 BEGIN */
  if( osd_key_pressed( OSD_KEY_F7 ) && CheatTotal ){
/* JRT3 10-26-97 END */
    y = (Machine->uiheight - Machine->uifont->height) / 2;
    if(CheatEnabled == 0){
      CheatEnabled = 1;
      xprintf(0,y,"Cheats On");
    }else{
      CheatEnabled = 0;
      xprintf(0,y,"Cheats Off");
    }
    while (osd_key_pressed(OSD_KEY_F7)) ;  /* wait for key release */

/* JRT3 10-23-97 BEGIN */
    osd_clearbitmap( Machine -> scrbitmap );        /* Clear Screen */
    (Machine->drv->vh_update)(Machine->scrbitmap,1);  /* Make Game Redraw Screen */
/* JRT3 10-23-97 END */
  }

}




/*****************
 *  Print a string at the position x y
 * if x = 0 then center the string in the screen
 */
void xprintf(int x,int y,char *fmt,...)
{
struct DisplayText dt[2];
char s[80];

va_list arg_ptr;
char *format;

  va_start(arg_ptr,fmt);
  format=fmt;
  (void) vsprintf(s,format,arg_ptr);

  dt[0].text = s;
  dt[0].color = DT_COLOR_WHITE;
  if(x == 0)
    dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(s)) / 2;
  else
    dt[0].x = x;
  if(dt[0].x < 0)
    dt[0].x = 0;
  if(dt[0].x > Machine->uiwidth)
    dt[0].x = 0;
  dt[0].y = y;
  dt[1].text = 0;

  displaytext(dt,0);
}

void xprintfForEdit(int x,int y,char *fmt,...)
{
struct DisplayText dt[2];
char s[80];

va_list arg_ptr;
char *format;

  va_start(arg_ptr,fmt);
  format=fmt;
  (void) vsprintf(s,format,arg_ptr);

  dt[0].text = s;
  dt[0].color = DT_COLOR_WHITE;
  if(x == 0)
    dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(s)) / 2;
  else
    dt[0].x = x;
  if(dt[0].x < 0)
    dt[0].x = 0;
  if(dt[0].x > Machine->uiwidth)
    dt[0].x = 0;
  dt[0].y = y;
  dt[1].text = 0;

  displaytext(dt,0);

  dt[0].x += Machine->uifont->width * strlen(s);
  s[0] = '_';
  s[1] = 0;
  dt[0].color = DT_COLOR_YELLOW;
  displaytext(dt,0);

}



/*****************
 *
 */
void EditCheat(int CheatNo)
{
/* JRT5 BEGIN Constants, And Menu Text Changes */
#define		EDIT_CHEAT_LINES	11		/* Lines In Blurb*/
#define		TOTAL_CHEAT_TYPES	11		/* Cheat Type Values*/
#define		CHEAT_NAME_MAXLEN	25		/* Cheat (Edit) Name MaxLen*/

 char	*paDisplayText[] = {
		"To edit a Cheat name, press",
		"<ENTER> while Cheat name",
		"is selected.",
		"To edit Cheat values,  or to",
		"select a pre-defined Cheat",
		"Name, use:",
		"<+> and Right arrow key: +1",
		"<-> and Left  arrow key: -1",
		"<1>/<2>/<3>/<4>: +1 digit",
		"",
		"<F10> To Show Help",
		"\0" };
/* int flag;*/
/* JRT5 END */

 int  iFontHeight = ( Machine -> uifont -> height );
 char *CheatNameList[] = {
  "Infinite Lives",
  "Infinite Lives PL1",
  "Infinite Lives PL2",
  "Invincibility",
  "Infinite Time",
  "Infinite Ammo",
  "---> <ENTER> To Edit <---",
  "\0"
 };
 int i,s,y,key,done;
 int total;
 struct DisplayText dt[20];
 char str2[12][50];
 char buffer[100];
 int FirstEditableItem;
 int CurrentName;
 int EditYPos;

  osd_clearbitmap(Machine->scrbitmap);

/* JRT5 BEGIN Menu Text Changes */
  for(i=0;i<EDIT_CHEAT_LINES;i++)
  {
    if(i)
      dt[i].y = (dt[i - 1].y + iFontHeight + 2);
    else
      dt[i].y = 20;
    dt[i].color = DT_COLOR_WHITE;
    dt[i].text = paDisplayText[i];
    dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
    if(dt[i].x > Machine->uiwidth)
      dt[i].x = 0;
  }
  y = ( dt[ EDIT_CHEAT_LINES - 1 ].y + ( 3 * iFontHeight ) );
  total = EDIT_CHEAT_LINES;
/* JRT5 END */

   FirstEditableItem = total;

	sprintf(str2[0],"Name: %s",LoadedCheatTable[CheatNo].Name);
	if((Machine->uifont->width * (int)strlen(str2[0])) > Machine->uiwidth)
		sprintf(str2[0],"%s",LoadedCheatTable[CheatNo].Name);
	sprintf(str2[1],"CPU:        %1d",LoadedCheatTable[CheatNo].CpuNo);

/* JRT6	- modified JB 980407 */
	sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
		LoadedCheatTable[CheatNo].Address);
/* JRT6 */

	sprintf(str2[3],"Value:    %03d  (0x%02X)",LoadedCheatTable[CheatNo].Data,LoadedCheatTable[CheatNo].Data);
	sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);

  for (i=0;i<5;i++){

    dt[total].text = str2[i];

	  dt[total].x = Machine->uiwidth / 2;
	  if(Machine->uiwidth < 35*Machine->uifont->width)
		  dt[total].x = 0;
	  else
		  dt[total].x -= 15*Machine->uifont->width;

    dt[total].y = y;
    dt[total].color = DT_COLOR_WHITE;

    total++;

    y += Machine->uifont->height;
  }
  dt[total].text = 0; /* terminate array */

  EditYPos = ( y + ( 3 * iFontHeight ) + 3 );

  s = FirstEditableItem;
  CurrentName = -1;

  done = 0;
  do
  {

    for (i = 4;i < total;i++)
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1) s++;
        else s = FirstEditableItem;
        break;

      case OSD_KEY_UP:
        if (s > FirstEditableItem) s--;
        else s = total - 1;
        break;

	  case OSD_KEY_F10:
		EditCheatHelp();								/* Show Help*/
		break;

      case OSD_KEY_MINUS_PAD:
      case OSD_KEY_LEFT:	/* JB 980424 */
      	switch (s - FirstEditableItem)
      	{
      		case 0:	/* Name*/
      			if (CurrentName == 0)						/* wrap if necessary*/
      				while (CheatNameList[CurrentName][0])
      					CurrentName++;
      			CurrentName--;
				strcpy (LoadedCheatTable[CheatNo].Name, CheatNameList[CurrentName]);
				sprintf (str2[0],"Name: %s", LoadedCheatTable[CheatNo].Name);
				for (i=dt[FirstEditableItem].y; i< ( dt[FirstEditableItem].y + iFontHeight + 1); i++)
					memset (Machine->scrbitmap->line[i], 0, Machine->uiwidth);
				break;
			case 1:	/* CpuNo*/
				if (LoadedCheatTable[CheatNo].CpuNo == 0)
					LoadedCheatTable[CheatNo].CpuNo = cpu_gettotalcpu() - 1;
				else
					LoadedCheatTable[CheatNo].CpuNo --;

				sprintf (str2[1], "CPU:        %1d", LoadedCheatTable[CheatNo].CpuNo);
				break;
			case 2:	/* Address*/
				if (LoadedCheatTable[CheatNo].Address == 0)
					LoadedCheatTable[CheatNo].Address = MAX_ADDRESS(LoadedCheatTable[CheatNo].CpuNo);
				else
					LoadedCheatTable[CheatNo].Address --;

		  		sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
		  			LoadedCheatTable[CheatNo].Address);
		  		break;
		  	case 3:	/* Data*/
				if (LoadedCheatTable[CheatNo].Data == 0)
					LoadedCheatTable[CheatNo].Data = 0xFF;
				else
					LoadedCheatTable[CheatNo].Data --;

				sprintf(str2[3], "Value:    %03d  (0x%02X)", LoadedCheatTable[CheatNo].Data, LoadedCheatTable[CheatNo].Data);
				break;
			case 4:	/* Special*/
				if (LoadedCheatTable[CheatNo].Special <= 0)
					LoadedCheatTable[CheatNo].Special = TOTAL_CHEAT_TYPES;
				else
					LoadedCheatTable[CheatNo].Special --;

				sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);
				break;
        }
        break;

      case OSD_KEY_PLUS_PAD:
      case OSD_KEY_RIGHT:	/* JB 980424 */
      	switch (s - FirstEditableItem)
      	{
      		case 0:	/* Name*/
				CurrentName ++;
				if (CheatNameList[CurrentName][0] == 0)
					CurrentName = 0;
				strcpy (LoadedCheatTable[CheatNo].Name, CheatNameList[CurrentName]);
				sprintf (str2[0], "Name: %s", LoadedCheatTable[CheatNo].Name);
				for(i=dt[FirstEditableItem].y; i< ( dt[FirstEditableItem].y + iFontHeight + 1); i++)
					memset(Machine->scrbitmap->line[i],0,Machine->uiwidth);
				break;
			case 1:	/* CpuNo*/
				LoadedCheatTable[CheatNo].CpuNo ++;
				if (LoadedCheatTable[CheatNo].CpuNo >= cpu_gettotalcpu())
					LoadedCheatTable[CheatNo].CpuNo = 0;
				sprintf(str2[1],"CPU:        %1d",LoadedCheatTable[CheatNo].CpuNo);
				break;
			case 2:	/* Address*/
            	LoadedCheatTable[CheatNo].Address ++;
				if (LoadedCheatTable[CheatNo].Address > MAX_ADDRESS(LoadedCheatTable[CheatNo].CpuNo))
					LoadedCheatTable[CheatNo].Address = 0;
				sprintf (str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
					LoadedCheatTable[CheatNo].Address);
				break;
			case 3:	/* Data*/
				if(LoadedCheatTable[CheatNo].Data == 0xFF)
					LoadedCheatTable[CheatNo].Data = 0;
				else
					LoadedCheatTable[CheatNo].Data ++;
				sprintf(str2[3],"Value:    %03d  (0x%02X)",LoadedCheatTable[CheatNo].Data,LoadedCheatTable[CheatNo].Data);
				break;
			case 4: /* Special*/
				LoadedCheatTable[CheatNo].Special ++;
				if(LoadedCheatTable[CheatNo].Special > TOTAL_CHEAT_TYPES)
					LoadedCheatTable[CheatNo].Special = 0;
				sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);
				break;
      	}
        break;

      case OSD_KEY_6:	/* JB 980424 */
      	if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 24) break;
      case OSD_KEY_5:
      	if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 20) break;
      case OSD_KEY_4:
      	if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 16) break;
      case OSD_KEY_3:
      case OSD_KEY_2:
      case OSD_KEY_1:
		  /* these keys only apply to the address line */
		  if (s == 2+FirstEditableItem)
	      {
	      		int addr = LoadedCheatTable[CheatNo].Address;	/* copy address*/
	      		int digit = (OSD_KEY_6 - key);	/* if key is OSD_KEY_6, digit = 0*/
	      		int mask;

	      		/* adjust digit based on cpu address range */
	      		digit -= (6 - ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) / 4);

	      		mask = 0xF << (digit * 4);	/* if digit is 1, mask = 0xf0*/

	      		if ((addr & mask) == mask)
	      			/* wrap hex digit around to 0 if necessary */
	      			addr &= ~mask;
	      		else
	      			/* otherwise bump hex digit by 1 */
	      			addr += (0x1 << (digit * 4));

				LoadedCheatTable[CheatNo].Address = addr;
				sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
					LoadedCheatTable[CheatNo].Address);
		  }
		  break;
      case OSD_KEY_ENTER:
		if (s == 0+FirstEditableItem)
		{
			/* Edit Name */
			for (i = 4; i < total; i++)
				dt[i].color = DT_COLOR_WHITE;
			displaytext (dt, 0);
			xprintf (0, EditYPos-iFontHeight-3, "Edit the Cheat name:");

			/* JRT5 BEGIN  25 Char Editing Limit...*/
			memset (buffer, '\0', 32);
			strncpy (buffer, LoadedCheatTable[ CheatNo ].Name, 25);
			for (i=EditYPos; i< ( EditYPos + iFontHeight + 1); i++)
				memset (Machine->scrbitmap->line[i], 0, Machine->uiwidth);
			xprintfForEdit (0, EditYPos, "%s", buffer);
			/* JRT5 END */

			do	/* JB 980506 */
			{
				int length;

				length = strlen (buffer);
				key = osd_read_keyrepeat ();

				switch (key)
				{
					case OSD_KEY_BACKSPACE:
						if (length)
							buffer[length-1] = 0;
						break;
					case OSD_KEY_ENTER:
						done = 1;
						strcpy (LoadedCheatTable[CheatNo].Name, buffer);
						sprintf (str2[0], "Name: %s", LoadedCheatTable[CheatNo].Name);
						break;
					case OSD_KEY_ESC:
					case OSD_KEY_TAB:
						done = 1;
						break;
					default:
						if (length < CHEAT_NAME_MAXLEN)
						{
							unsigned char c = 0;

							if (osd_key_pressed (OSD_KEY_LSHIFT) ||
							 osd_key_pressed (OSD_KEY_RSHIFT) ||
							 osd_key_pressed (OSD_KEY_CAPSLOCK))
								c = osd_key_caps[key];
							else
								c = osd_key_chars[key];

							if (c)
							{
								buffer[length++] = c;
								buffer[length] = 0;
							}
						}
						break;
	            }
	            for (i=EditYPos; i< ( EditYPos + iFontHeight + 1); i++)
					memset (Machine->scrbitmap->line[i], 0, Machine->uiwidth);
					xprintfForEdit (0, EditYPos, "%s", buffer);
			} while (done == 0);
			done = 0;
			osd_clearbitmap (Machine->scrbitmap);
		}
		break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */

  osd_clearbitmap(Machine->scrbitmap);
}


/*****************
 *
 */
void DisplayCheats(int x,int y)
{
int i;

  xprintf(0,y,"Active Cheats:");
  x -= 4*Machine->uifont->width;
  y += ( Machine->uifont->height + 3 );
  for(i=0;i<CheatTotal;i++){
    xprintf(x,y,"%s",CheatTable[i].Name);
    y += Machine->uifont->height;
  }
  if(CheatTotal == 0){
    x = (Machine->uiwidth - Machine->uifont->width * 12) / 2;
    xprintf(x,y,"--- None ---");
  }
}




/*****************
 *
 */
void SelectCheat(void)
{
 int i,x,y,s,key,done,total;
/* JRT5 */
int		iFontHeight = ( Machine -> uifont -> height );
int		iWhereY = 0;
/* JRT5 */
 FILE *f;
 struct DisplayText dt[40];
 int flag;
 int Index;
 int StartY,EndY;


HardRefresh:
  osd_clearbitmap(Machine->scrbitmap);

  x = Machine->uiwidth / 2;
  if(Machine->uiwidth < 35*Machine->uifont->width)
	  x = 0;
  else
	  x -= 15*Machine->uifont->width;
  y = 10;
  y += 6*Machine->uifont->height; /* Keep space for menu */

/* No more than 10 cheat displayed */
  if(LoadedCheatTotal > 10)
    total = 10;
  else
    total = LoadedCheatTotal;

  Index = 0;
  StartY = y;

/* Make the list */
  for (i = 0;i < total;i++)
  {
    dt[i].text = LoadedCheatTable[i].Name;
    dt[i].x = x;
    dt[i].y = y;
    dt[i].color = DT_COLOR_WHITE;
    y += iFontHeight;
  }

  dt[total].text = 0; /* terminate array */

  x += 4*Machine->uifont->width;
  y += ( ( Machine->uifont->height * 2 ) + 3 );
  DisplayCheats(x,y);

  EndY = y;

  s = 0;
  done = 0;
  do
  {
    for (i = 0;i < total;i++)
    {
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
    }

    if(LoadedCheatTotal == 0){
      iWhereY = 0;
		  xprintf(0, iWhereY, "<INS>: Add New Cheat" );
		  iWhereY += ( iFontHeight * 3 );
      xprintf(0,iWhereY,"No Cheats Available!");
    }else{
/* JRT5 */
			iWhereY = 0;
		  xprintf(0, iWhereY, "<DEL>: Delete  <INS>: Add" );
		  iWhereY += ( iFontHeight + 1 );
		  xprintf(0, iWhereY,"<F1>: Save  <F2>: Watch");
		  iWhereY += ( iFontHeight + 1 );
      xprintf(0, iWhereY,"<F3>: Edit  <F10>: Show Help");
		  iWhereY += ( iFontHeight + 1 );
      xprintf(0, iWhereY,"<ENTER>: Enable/Disable");
		  iWhereY += ( iFontHeight + 4 );
      xprintf(0, iWhereY,"Select a Cheat (%d Total)",LoadedCheatTotal);
		  iWhereY += ( iFontHeight + 1 );
/* JRT5 */

    }

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1)
          s++;
        else{

          s = 0;

          if(LoadedCheatTotal <= 10)
            break;
/*
End of list
 -Increment index
 -Redo the list
*/
          if(LoadedCheatTotal > Index+10)
            Index += 10;
          else
            Index = 0;

/* Make the list */
          total = 0;
          for (i = 0;i < 10;i++){
            if(Index+i >= LoadedCheatTotal)
              break;
            dt[i].text = LoadedCheatTable[i+Index].Name;
            total++;
          }
          dt[total].text = 0; /* terminate array */

/* Clear old list */
          for (i = StartY;i < EndY;i++)
            memset(Machine->scrbitmap->line[i],0,Machine->uiwidth);

        }
        break;

      case OSD_KEY_UP:
        if (s > 0)
          s--;
        else{
          s = total - 1;
/* JRT5 Fixes Blank List When <UP> Hit With Exactly 10 Entries */
/*          if(LoadedCheatTotal < 10)*/
          if(LoadedCheatTotal <= 10)
/* JRT5 */
            break;

/* Top of the list, page up */
          if(Index == 0)
/* JRT5 Fixes Blank List When <UP> Hit With Exactly 10 Entries */
/*            Index = (LoadedCheatTotal/10)*10;*/
            Index = ( LoadedCheatTotal - 1 );
/* JRT5 */
          else if(Index > 10)
            Index -= 10;
          else
            Index = 0;

/* Refresh the list */
          total = 0;
          for (i = 0;i < 10;i++){
            if(Index+i >= LoadedCheatTotal)
              break;
            dt[i].text = LoadedCheatTable[i+Index].Name;
            total++;
          }
          dt[total].text = 0; /* terminate array */
          s = total-1;

/* Clear old list */
          for (i = StartY;i < EndY;i++)
            memset(Machine->scrbitmap->line[i],0,Machine->uiwidth);

        }
        break;

      case OSD_KEY_INSERT:
/* Add a new empty cheat */
/* JRT5 Print Message If Cheat List Is Full */
        if(LoadedCheatTotal > 99){
					xprintf( 0, ( ( EndY - Machine->uifont->height ) - 4 ),
						"(Cheat List Is Full.)" );
          break;
        }
/* JRT5 */
        LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
        LoadedCheatTable[LoadedCheatTotal].Special = 0;
        LoadedCheatTable[LoadedCheatTotal].Count = 0;
        LoadedCheatTable[LoadedCheatTotal].Address = 0;
        LoadedCheatTable[LoadedCheatTotal].Data = 0;
        strcpy(LoadedCheatTable[LoadedCheatTotal].Name,"---- New Cheat ----");
        LoadedCheatTotal++;
        goto HardRefresh; /* I know...  */
        break;


      case OSD_KEY_F1:
        if(LoadedCheatTotal == 0)
          break;
        if ((f = fopen("CHEAT.DAT","a")) != 0)
        {
			/* JRT6 - modified JB 980424 */
        	char	fmt[32];

        	/* form fmt string, adjusting length of address field for cpu address range */
        	switch (ADDRESS_BITS(LoadedCheatTable[s+Index].CpuNo) >> 2)
        	{
        		case 4:
        			strcpy (fmt, "%s:%d:%04X:%X:%d:%s\n");
        			break;
        		case 5:
        			strcpy (fmt, "%s:%d:%05X:%X:%d:%s\n");
        			break;
        		case 6:
        		default:
        			strcpy (fmt, "%s:%d:%06X:%X:%d:%s\n");
        			break;
        	}

			#ifdef macintosh
				strcat (fmt, "\r");	/* force DOS-style line enders */
			#endif

			fprintf (f, fmt, Machine->gamedrv->name, LoadedCheatTable[s+Index].CpuNo,
				LoadedCheatTable[s+Index].Address, LoadedCheatTable[s+Index].Data,
				LoadedCheatTable[s+Index].Special, LoadedCheatTable[s+Index].Name);

			xprintf (dt[s].x + (strlen(LoadedCheatTable[s+Index].Name) *
				Machine->uifont->width) + Machine->uifont->width, dt[s].y, "(Saved)");
			fclose (f);
        }
        break;

      case OSD_KEY_F2:	/* Add to watch list */
      	if (LoadedCheatTable[s+Index].CpuNo==0)	/* watches are for cpu 0 only */ /* JB 980424 */
      	{
	        for (i=0;i<10;i++)
	        {
				if (Watches[i] == MAX_ADDRESS(0))	/* JRT6/JB */
				{
            		Watches[i] = LoadedCheatTable[s+Index].Address;
            		xprintf(dt[s].x+(strlen(LoadedCheatTable[s+Index].Name) *
            			Machine->uifont->width)+Machine->uifont->width,dt[s].y,"(Watch)");
            		WatchesFlag = 1;
            		break;
          		}
        	}
        }
        break;
      case OSD_KEY_F3:
/* Edit current cheat */
/* JRT6 */
		if( LoadedCheatTotal )								/* If At Least One Cheat*/
			EditCheat(s+Index);								/* Edit It*/
        break;
/* JRT6 */


/* JRT5 Invoke Cheat List Help */
			case OSD_KEY_F10:
			{
				CheatListHelp();
				break;
			}
/* JRT5 */


      case OSD_KEY_DEL:
        if(LoadedCheatTotal == 0)
          break;

/* Erase the current cheat from the list */
/* But before, erase it from the active list if it is there */
        for(i=0;i<CheatTotal;i++){
          if(CheatTable[i].Address == LoadedCheatTable[s+Index].Address)
           if(CheatTable[i].Data == LoadedCheatTable[s+Index].Data){
/* The selected Cheat is already in the list then delete it.*/
            for(;i<CheatTotal-1;i++){
              CheatTable[i].CpuNo = CheatTable[i+1].CpuNo;
              CheatTable[i].Address = CheatTable[i+1].Address;
              CheatTable[i].Data = CheatTable[i+1].Data;
              CheatTable[i].Special = CheatTable[i+1].Special;
              CheatTable[i].Count = CheatTable[i+1].Count;
              strcpy(CheatTable[i].Name,CheatTable[i+1].Name);
            }
            CheatTotal--;
            break;
           }
        }

/* Delete entry from list */
        for(i=s+Index;i<LoadedCheatTotal-1;i++){
          LoadedCheatTable[i].CpuNo = LoadedCheatTable[i+1].CpuNo;
          LoadedCheatTable[i].Address = LoadedCheatTable[i+1].Address;
          LoadedCheatTable[i].Data = LoadedCheatTable[i+1].Data;
          LoadedCheatTable[i].Special = LoadedCheatTable[i+1].Special;
          LoadedCheatTable[i].Count = LoadedCheatTable[i+1].Count;
          strcpy(LoadedCheatTable[i].Name,LoadedCheatTable[i+1].Name);
        }
        LoadedCheatTotal--;

/* Refresh the list */
        total = 0;
        for (i = 0;i < 10;i++){
          if(Index+i >= LoadedCheatTotal)
            break;
          dt[i].text = LoadedCheatTable[i+Index].Name;
          total++;
        }
        dt[total].text = 0; /* terminate array */
        if(total <= s)
          s = total-1;

        if(total == 0){
          if(Index != 0){
/* The page is empty so backup one page */
            if(Index == 0)
              Index = (LoadedCheatTotal/10)*10;
            else if(Index > 10)
              Index -= 10;
            else
              Index = 0;

/* Make the list */
            total = 0;
            for (i = 0;i < 10;i++){
              if(Index+i >= LoadedCheatTotal)
                break;
              dt[i].text = LoadedCheatTable[i+Index].Name;
              total++;
            }
            dt[total].text = 0; /* terminate array */
            s = total-1;
          }
        }

/* Redisplay all */
        osd_clearbitmap(Machine->scrbitmap);
        DisplayCheats(x,y);

        break;


      case OSD_KEY_ENTER:
        if(total == 0)
          break;

        flag = 0;
        for(i=0;i<CheatTotal;i++){
          if(CheatTable[i].Address == LoadedCheatTable[s+Index].Address){
/* The selected Cheat is already in the list then delete it.*/
            for(;i<CheatTotal-1;i++){
              CheatTable[i].CpuNo = CheatTable[i+1].CpuNo;
              CheatTable[i].Address = CheatTable[i+1].Address;
              CheatTable[i].Data = CheatTable[i+1].Data;
              CheatTable[i].Special = CheatTable[i+1].Special;
              CheatTable[i].Count = CheatTable[i+1].Count;
              strcpy(CheatTable[i].Name,CheatTable[i+1].Name);
            }
            CheatTotal--;
            flag = 1;
            break;
          }
        }

/* No more than 10 cheat at the time */
        if(CheatTotal > 9){
					xprintf( 0, ( ( EndY - Machine->uifont->height ) - 4 ),
						"(Limit Of 10 Active Cheats)" );
          break;
        }

/* Add the selected cheat to the active cheats list if it was not already there */
        if(flag == 0){
          CheatTable[CheatTotal].CpuNo = LoadedCheatTable[s+Index].CpuNo;
          CheatTable[CheatTotal].Address = LoadedCheatTable[s+Index].Address;
          CheatTable[CheatTotal].Data = LoadedCheatTable[s+Index].Data;
          CheatTable[CheatTotal].Special = LoadedCheatTable[s+Index].Special;
          CheatTable[CheatTotal].Count = 0;
          strcpy(CheatTable[CheatTotal].Name,LoadedCheatTable[s+Index].Name);
          CheatTotal++;
          CheatEnabled = 1;
          he_did_cheat = 1;
        }

        osd_clearbitmap(Machine->scrbitmap);
        DisplayCheats(x,y);
        break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */

  /* clear the screen before returning */
  osd_clearbitmap(Machine->scrbitmap);

}


/*****************
 * Start a cheat search
 * If the method 1 is selected, ask the user a number
 * In all cases, backup the ram.
 *
 * Ask the user to select one of the following:
 *  1 - Lives or other number (byte) (exact)       ask a start value , ask new value
 *  2 - Timers (byte) (+ or - X)                   nothing at start, ask +-X
 *  3 - Status (bit) (true or false)               nothing at start, ask same or opposite
 *  4 - Slow but sure (Same as start or different) nothing at start, ask same or different
 *
 * Another method is used in the Pro action Replay the Energy method
 *  you can tell that the value is now 25%/50%/75%/100% as the start
 *  the problem is that I probably cannot search for exactly 50%, so
 *  that do I do? search +/- 10% ?
 * If you think of other way to search for codes, let me know.
 */

#define Method_1 1
#define Method_2 2
#define Method_3 3

#define Method_4 4
#define Method_5 5

void StartCheat(void)
{
 int i,y,s,key,done,count;
 struct DisplayText dt[10];
 int total;

 osd_clearbitmap(Machine->scrbitmap);

  y = 25;
/* JRT5 */
  xprintf(0,y,"- Choose A Search Method -");

  total = 9;
  dt[0].text = "Lives, Or Some Other Value";
  dt[1].text = "Timers (+/- Some Value)";
  dt[2].text = "Energy (Greater Or Less)";
  dt[3].text = "Status (A Bit Or Flag)";
  dt[4].text = "Slow But Sure (Changed Or Not)";
  dt[5].text = "";
  dt[6].text = "Show Help";
  dt[7].text = "";
  dt[8].text = "Return To Cheat Menu";
  dt[9].text = 0;
/* JRT5 */

  y += 3*Machine->uifont->height;
  for (i = 0;i < total;i++)
  {
    dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
    if(dt[i].x > Machine->uiwidth)
      dt[i].x = 0;
    dt[i].y = i * ( Machine->uifont->height + 2 ) + y;
    if (i == total-1) dt[i].y += 2*Machine->uifont->height;
  }


  s = 0;
  done = 0;
  do
  {
    for (i = 0;i < total;i++)
    {
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
    }

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1) s++;
        else s = 0;

/* JRT5 For Space In Menu*/
        if( ( s == 5 ) || ( s == 7 ) )
					s++;
/* JRT5 */
		break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;

/* JRT5 For Space In Menu*/
        if( ( s == 5 ) || ( s == 7 ) )
					s--;
/* JRT5 */
        break;

      case OSD_KEY_ENTER:
        switch (s)
        {
          case 0:
            CurrentMethod = Method_1;
            done = 1;
            break;

          case 1:
            CurrentMethod = Method_2;
            done = 1;
            break;

          case 2:
            CurrentMethod = Method_3;
            done = 1;
            break;

          case 3:
            CurrentMethod = Method_4;
            done = 1;
            break;

          case 4:
            CurrentMethod = Method_5;
            done = 1;
            break;

          case 6:
            StartSearchHelp();								/* Show Help*/
						break;

          case 8:
            done = 2;
            break;
        }
        break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 2;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */

  osd_clearbitmap(Machine->scrbitmap);

/* User select to return to the previous menu */
  if(done == 2)
    return;




/* If the method 1 is selected, ask for a number */
if(CurrentMethod == Method_1){
  y = 25;
  xprintf(0,y,"Enter Value To Search For:");
  y += Machine->uifont->height;
  xprintf(0,y,"(Arrow Keys Change Value)");
  y += 2*Machine->uifont->height;

  s = 0;
  done = 0;
  do
  {
/* JRT2 10-23-97 BEGIN */
      xprintf(0,y,"%03d  (0x%02X)",s, s);
/* JRT2 10-23-97 END */

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_RIGHT:
      case OSD_KEY_UP:
/* JRT2 10-23-97 BEGIN */
        if(s < 0xFF)
/* JRT2 10-23-97 END */
          s++;
        break;

      case OSD_KEY_LEFT:
      case OSD_KEY_DOWN:
        if(s != 0)
          s--;
        break;
      case OSD_KEY_ENTER:
      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 1;
        break;
    }
  } while (done == 0);

  StartValue = s; /* Save initial value for when continue */
}else{
  StartValue = 0;
}

	/* JB 980407 */
	/* build ram tables to match game ram */
	if (build_tables ())
		return;
#if 0
/* Allocate array if not already allocated */
  if(StartRam == NULL)
    if ((StartRam = malloc(ram_size)) == 0)
      return ;

  if(BackupRam == NULL)
    if ((BackupRam = malloc(ram_size)) == 0)
      return ;


  if(FlagTable == NULL)
    if ((FlagTable = malloc(ram_size)) == 0)
      return ;
#endif

	/* JB 980407 */
	/* Backup the ram */
	backup_ram (StartRam);
	backup_ram (BackupRam);
	memset_ram (FlagTable, 0xFF); /* At start, all location are good */

#if 0
  memcpy (StartRam, Machine->memory_region[0], ram_size);
  memcpy (BackupRam, Machine->memory_region[0], ram_size);

  memset(FlagTable,0xFF,ram_size); /* At start, all location are good */
#endif

	/* Flag the location that match the initial value if method 1 */
	if(CurrentMethod == Method_1)
	{
		/* JB 980407 */
		struct ExtMemory *ext, *ext_ft;

		for (ext = StartRam, ext_ft = FlagTable; ext->data; ext++, ext_ft++)
		{
			for (i=0; i <= ext->end - ext->start; i++)
				if ((ext->data[i] != s) && (ext->data[i] != s-1))
					ext_ft->data[i] = 0;
		}
#if 0
    for (i=0;i<ram_size;i++)
      if((StartRam[i] != s)&&(StartRam[i] != s-1))
        FlagTable[i] = 0;
#endif

		/* JB 980407 */
		count = 0;
		for (ext = FlagTable; ext->data; ext++)
			for (i=0; i<= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					count++;
#if 0
    count = 0;
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        count++;
#endif

    y += 2*Machine->uifont->height;
    xprintf(0,y,"Matches Found: %d",count);

  }else{
    y += 4*Machine->uifont->height;
    xprintf(0,y,"Search Initialized.");
    iCheatInitialized = 1;
  }

  y += 4*Machine->uifont->height;
  xprintf(0,y,"Press A Key To Continue...");
  key = osd_read_keyrepeat();
  while (osd_key_pressed(key)) ; /* wait for key release */

  osd_clearbitmap(Machine->scrbitmap);
}


/*****************
 *
 */
void ContinueCheat(void)
{
 char *str;
 char str2[12][80];
 int i,j,x,y,count,s,key,done;
 struct DisplayText dt[20];
 int total;
 int Continue;
 struct ExtMemory *ext;		/* JB 980407 */
 struct ExtMemory *ext_br;	/* JB 980407 */
 struct ExtMemory *ext_sr;	/* JB 980407 */

  osd_clearbitmap(Machine->scrbitmap);

  if(CurrentMethod == 0){
    StartCheat();
    return;
  }

  y = 10;
  count = 0;


/******** Method 1 ***********/

/* Ask new value if method 1 */
  if(CurrentMethod == Method_1){
/* JRT5 */
		xprintf(0,y,"Enter The New Value:");
    y += Machine->uifont->height;
    xprintf(0,y,"(Arrow Keys Change Value)");
    y += Machine->uifont->height;
    xprintf(0,y,"Press <ENTER> When Done.");
    y += Machine->uifont->height;
    xprintf(0,y,"<F1> Starts A New Search.");
    y += 2*Machine->uifont->height;
/* JRT5 */

    s = StartValue;
    done = 0;
    do
    {
/* JRT2 10-23-97 BEGIN */
      xprintf(0,y,"%03d  (0x%02X)",s, s);
/* JRT2 10-23-97 END */

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_RIGHT:
        case OSD_KEY_UP:
/* JRT2 10-23-97 BEGIN */
          if(s < 0xFF)
/* JRT2 10-23-97 END */
            s++;
          break;

        case OSD_KEY_F1:
          StartCheat();
          return;
          break;

        case OSD_KEY_LEFT:
        case OSD_KEY_DOWN:
          if(s != 0)
            s--;
          break;
        case OSD_KEY_ENTER:
          done = 1;
          break;
        case OSD_KEY_ESC:
        case OSD_KEY_TAB:
          done = 2;
          break;
      }
    } while (done == 0);

    while (osd_key_pressed(key)) ; /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

    StartValue = s; /* Save the value for when continue */

	/* JB 980407 */
	for (ext = FlagTable; ext->data; ext++)
	{
		unsigned char *gameram = memory_find_base (0, ext->start);
		for (i=0; i <= ext->end - ext->start; i++)
			if ((gameram[i] != s) && (gameram[i] != s-1))
				ext->data[i] = 0;
	}
#if 0
/* Flag the value */
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        if((Machine->memory_region[0][i] != s)&&(Machine->memory_region[0][i] != s-1))
          FlagTable[i] = 0;
#endif
  }



/******** Method 2 ***********/

/* Ask new value if method 2 */
  if(CurrentMethod == Method_2){
/* JRT5 */
    xprintf(0,y,"Enter How Much The Value");
    y += Machine->uifont->height;

/* JRT5 For Different Text Depending On Start/Continue Search */
    if( iCheatInitialized )
		{
			xprintf(0,y,"Has Changed Since You");
			y += Machine->uifont->height;
			xprintf(0,y,"Started The Search:");
		}
		else
		{
			xprintf(0,y,"Has Changed Since The");
			y += Machine->uifont->height;
			xprintf(0,y,"Last Check:");
		}
/* JRT5 */

    y += Machine->uifont->height;
    xprintf(0,y,"(Arrow Keys Change Value)");
    y += Machine->uifont->height;
    xprintf(0,y,"Press <ENTER> When Done.");
    y += Machine->uifont->height;
    xprintf(0,y,"<F1> Starts A New Search.");
    y += 2*Machine->uifont->height;
/* JRT5 */

    s = StartValue;
    done = 0;
    do
    {
/* JRT2 10-23-97 BEGIN */
      xprintf(0,y,"%+04d  (0x%02X)",s, (unsigned char)s);
/* JRT2 10-23-97 END */

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_RIGHT:
        case OSD_KEY_UP:
/* JRT2 10-23-97 BEGIN */
          if(s < 128)
/* JRT2 10-23-97 END */
            s++;
          break;

        case OSD_KEY_LEFT:
        case OSD_KEY_DOWN:
/* JRT2 10-23-97 BEGIN */
          if(s > -127)
/* JRT2 10-23-97 END */
            s--;
          break;

        case OSD_KEY_F1:
          StartCheat();
          return;
          break;

        case OSD_KEY_ENTER:
          done = 1;
          break;
        case OSD_KEY_ESC:
        case OSD_KEY_TAB:
          done = 2;
          break;
      }
    } while (done == 0);

    while (osd_key_pressed(key)) ; /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }
/* JRT5 For Different Text Depending On Start/Continue Search */
		iCheatInitialized = 0;
/* JRT5 */

    StartValue = s; /* Save the value for when continue */

	/* JB 980407 */
	for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
	{
		unsigned char *gameram = memory_find_base (0, ext->start);
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
				if (gameram[i] != (ext_br->data[i] + s))
					ext->data[i] = 0;
	}
#if 0
/* Flag the value */
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        if(Machine->memory_region[0][i] != (BackupRam[i] + s))
          FlagTable[i] = 0;
#endif

/* Backup the ram because we ask how much the value changed since the last
   time, not in relation of the start value */
    /*memcpy (BackupRam, Machine->memory_region[0], ram_size);*/
	backup_ram (BackupRam);	/* JB 980407 */
  }



/******** Method 3 ***********/
  if(CurrentMethod == Method_3){
/* JRT5 */
    xprintf(0,y,"Choose The Expression That");
    y += Machine->uifont->height;
    xprintf(0,y,"Specifies What Occured Since");
    y += Machine->uifont->height;

/* JRT5 For Different Text Depending On Start/Continue Search */
    if( iCheatInitialized )
			xprintf(0,y,"You Started The Search:");
		else
			xprintf(0,y,"The Last Check:");
/* JRT5 */

    y += 2*Machine->uifont->height;
    xprintf(0,y,"<F1> Starts A New Search.");
    y += Machine->uifont->height;
/* JRT5 */

    y += 2*Machine->uifont->height;
    total = 3;
    dt[0].text = "New Value is Less";
    dt[1].text = "New Value is Equal";
    dt[2].text = "New value is Greater";
    dt[3].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
      if(dt[i].x > Machine->uiwidth)
        dt[i].x = 0;
      dt[i].y = y;
      y += Machine->uifont->height;
    }

    s = 0;
    done = 0;
    do
    {
      for (i = 0;i < total;i++)
        dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

      displaytext(dt,0);

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_DOWN:
          if (s < total - 1) s++;
          else s = 0;
          break;

        case OSD_KEY_UP:
          if (s > 0) s--;
          else s = total - 1;
          break;

        case OSD_KEY_F1:
          StartCheat();
          return;
          break;

        case OSD_KEY_ENTER:
          done = 1;
          break;

        case OSD_KEY_ESC:
        case OSD_KEY_TAB:
          done = 2;
          break;
      }
    } while (done == 0);

    while (osd_key_pressed(key)) ; /* wait for key release */


/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

/* JRT5 For Different Text Depending On Start/Continue Search */
		iCheatInitialized = 0;
/* JRT5 */

    if (s == 0)
    {
		/* User select that the value is now smaller, then clear the flag
	    	of the locations that are equal or greater at the backup */

		/* JB 980407 */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					if (gameram[i] >= ext_br->data[i])
						ext->data[i] = 0;
		}
		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0)
	          if(Machine->memory_region[0][i] >= BackupRam[i])
	            FlagTable[i] = 0;
		#endif
    }
    else if (s==1)
    {
		/* User select that the value is equal, then clear the flag
		    of the locations that do not equal the backup */

		/* JB 980407 */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					if (gameram[i] != ext_br->data[i])
						ext->data[i] = 0;
		}

		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0)
	          if(Machine->memory_region[0][i] != BackupRam[i])
	            FlagTable[i] = 0;
		#endif
    }
    else
    {
		/* User select that the value is now greater, then clear the flag
		    of the locations that are equal or smaller */

		/* JB 980407 */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					if (gameram[i] <= ext_br->data[i])
						ext->data[i] = 0;
		}

		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0)
	          if(Machine->memory_region[0][i] <= BackupRam[i])
	            FlagTable[i] = 0;
		#endif
    }

	/* Backup the ram because we ask how much the value changed since the last
	   time, not in relation of the start value */
	/*memcpy (BackupRam, Machine->memory_region[0], ram_size);*/
	backup_ram (BackupRam);	/* JB 980407 */

  }




/******** Method 4 ***********/

  if(CurrentMethod == Method_4){
/* Ask if the value is the same as when we start or the opposite */
    xprintf(0,y,"Choose One Of The Following:");
    y += Machine->uifont->height;
    xprintf(0,y,"<F1> Starts A New Search.");

    y += 2*Machine->uifont->height;
    total = 2;
    dt[0].text = "Bit is Same as Start";
    dt[1].text = "Bit is Opposite from Start";
    dt[2].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
      if(dt[i].x > Machine->uiwidth)
        dt[i].x = 0;
      dt[i].y = y;
      y += Machine->uifont->height;
    }

    s = 0;
    done = 0;
    do
    {
      for (i = 0;i < total;i++)
        dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

      displaytext(dt,0);

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_DOWN:
          if (s < total - 1) s++;
          else s = 0;
          break;

        case OSD_KEY_UP:
          if (s > 0) s--;
          else s = total - 1;
          break;

        case OSD_KEY_F1:
          StartCheat();
          return;
          break;

        case OSD_KEY_ENTER:
          done = 1;
          break;

        case OSD_KEY_ESC:
        case OSD_KEY_TAB:
          done = 2;
          break;
      }
    } while (done == 0);

    while (osd_key_pressed(key)) ; /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

/* JRT5 For Different Text Depending On Start/Continue Search */
		iCheatInitialized = 0;
/* JRT5 */

    if (s == 0)
    {
		/* User select same as start */
		/* We want to keep a flag if a bit has not change */
		/* So,*/

		/* JB 980407 */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
				{
					j = gameram[i] ^ (ext_sr->data[i] ^ 0xFF);
					ext->data[i] = j & ext->data[i];
				}
		}

		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0){
	          j = Machine->memory_region[0][i] ^ (StartRam[i] ^ 0xFF);
	          FlagTable[i] = j & FlagTable[i];
	        }
		#endif
    }
    else
    {
		/* User select opposite as start */
		/* We want to keep a flag if a bit has change */

		/* JB 980407 */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
				{
					j = gameram[i] ^ ext_sr->data[i];
					ext->data[i] = j & ext->data[i];
				}
		}

		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0){
	          j = Machine->memory_region[0][i] ^ StartRam[i];
	          FlagTable[i] = j & FlagTable[i];
	        }
		#endif
    }
  }





/******** Method 5 ***********/

  if(CurrentMethod == Method_5){
/* Ask if the value is the same as when we start or different */
    xprintf(0,y,"Choose One Of The Following:");
    y += Machine->uifont->height;
    xprintf(0,y,"<F1> Starts A New Search.");

    y += 2*Machine->uifont->height;
    total = 2;
    dt[0].text = "Memory is Same as Start";
    dt[1].text = "Memory is Different from Start";
    dt[2].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
      if(dt[i].x > Machine->uiwidth)
        dt[i].x = 0;
      dt[i].y = y;
      y += Machine->uifont->height;
    }

    s = 0;
    done = 0;
    do
    {
      for (i = 0;i < total;i++)
        dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

      displaytext(dt,0);

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_DOWN:
          if (s < total - 1) s++;
          else s = 0;
          break;

        case OSD_KEY_UP:
          if (s > 0) s--;
          else s = total - 1;
          break;

        case OSD_KEY_F1:
          StartCheat();
          return;
          break;

        case OSD_KEY_ENTER:
          done = 1;
          break;

        case OSD_KEY_ESC:
        case OSD_KEY_TAB:
          done = 2;
          break;
      }
    } while (done == 0);

    while (osd_key_pressed(key)) ; /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

/* JRT5 For Different Text Depending On Start/Continue Search */
		iCheatInitialized = 0;
/* JRT5 */

    if (s == 0)
    {
		/* Discard the locations that are different from when we started */
		/* The value must be the same as when we start to keep it */

		/* JB 980407 */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					if (gameram[i] != ext_sr->data[i])
						ext->data[i] = 0;
		}
		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0)
	          if(Machine->memory_region[0][i] != StartRam[i])
	            FlagTable[i] = 0;
		#endif
    }
    else
    {
		/* Discard the locations that are the same from when we started */
		/* The value must be different as when we start to keep it */

		/* JB 980407 */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			unsigned char *gameram = memory_find_base (0, ext->start);
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
					if (gameram[i] == ext_sr->data[i])
						ext->data[i] = 0;
		}

		#if 0
	      for (i=0;i<ram_size;i++)
	        if(FlagTable[i] != 0)
	          if(Machine->memory_region[0][i] == StartRam[i])
	            FlagTable[i] = 0;
		#endif
    }
  }





/* For all method:
    -display how much location the search
    -Display them
    -The user can press ENTER to add one to the cheat list*/

/* Count how much we have flagged */

	/* JB 980407 */
	count = 0;
	for (ext = FlagTable; ext->data; ext++)
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
				count++;
#if 0
  count = 0;
  for (i=0;i<ram_size;i++)
    if(FlagTable[i] != 0)
      count++;
#endif

  y += 2*Machine->uifont->height;
  xprintf(0,y,"Matches Found: %d",count);

  if(count > 10)
    str = "Here Are 10 Matches:";
  else if(count != 0)
    str = "Here Is The List:";
  else
    str = "(No Matches Found)";
  y += 2*Machine->uifont->height;
  xprintf(0,y,"%s",str);

  x = (Machine->uiwidth - Machine->uifont->width * 9) / 2;
  y += 2*Machine->uifont->height;

  total = 0;
  Continue = 0;

	/* JB 980407 */
	for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
	{
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
			{
				sprintf (str2[total], "%04X = %02X", i+ext->start, ext_sr->data[i]);
				dt[total].text = str2[total];
				dt[total].x = x;
				dt[total].y = y;
				dt[total].color = DT_COLOR_WHITE;

				total++;

				y += Machine->uifont->height;
				if (total >= 10)
				{
					Continue = i+ext->start;
					break;
				}
			}
	}

#if 0
  for (i=0;i<ram_size;i++){
    if(FlagTable[i] != 0){
      sprintf(str2[total],"%04X = %02X",i,StartRam[i]);
      dt[total].text = str2[total];
      dt[total].x = x;
      dt[total].y = y;
      dt[total].color = DT_COLOR_WHITE;

      total++;

      y += Machine->uifont->height;
      if(total >= 10){
        Continue = i;
        break;
      }
    }
  }
#endif
  dt[total].text = 0; /* terminate array */

  y += 2*Machine->uifont->height;
  xprintf(0,y,"<ENTER> To Add Entry To List",str);
  y += Machine->uifont->height;
  xprintf(0,y,"<F2> To Add All To List",str);
  y += Machine->uifont->height;
  xprintf(0,y,"<PAGE DOWN> To Scroll",str);
  y += 2*Machine->uifont->height;

  s = 0;
  done = 0;
  do
  {
    	int Begin = 0;	/* JB 980407 */

    for (i = 0;i < total;i++)
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1)
          s++;
        else{
          s = 0;

/* Scroll down in the list */
          if(total < 10)
            break;
          total = 0;

			/* JB 980407 */
			Begin = Continue+1;
			Continue = 0;
			for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
			{
				if (ext->start <= Begin && ext->end >= Begin)
					for (i = Begin - ext->start; i <= ext->end - ext->start; i++)
						if (ext->data[i] != 0)
						{
							sprintf (str2[total],"%04X = %02X", i+ext->start, ext_sr->data[i]);
							dt[total].text = str2[total];

							total++;

							if (total >= 10)
							{
								Continue = i+ext->start;
								break;
							}
						}
			}

			#if 0
	          for (i=Continue+1;i<ram_size;i++){
	            if(FlagTable[i] != 0){
	              sprintf(str2[total],"%04X = %02X",i,StartRam[i]);
	              dt[total].text = str2[total];

	              total++;

	              if(total >= 10){
	                Continue = i;
	                break;
	              }
	            }
	          }
			#endif
          dt[total].text = 0; /* terminate array */

        }
        break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;
        break;

      case OSD_KEY_HOME:
        total = 0;

		/* JB 980407 */
		Continue = 0;
		for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
		{
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
				{
					sprintf (str2[total], "%04X = %02X", i+ext->start, ext_sr->data[i]);
					dt[total].text = str2[total];

					total++;
					if (total >= 10)
					{
						Continue = i+ext->start;
						break;
					}
				}
		}
		#if 0
	       for (i=0;i<ram_size;i++){
	          if(FlagTable[i] != 0){
	            sprintf(str2[total],"%04X = %02X",i,StartRam[i]);
	            dt[total].text = str2[total];

	            total++;

	            if(total >= 10){
	              Continue = i;
	              break;
	            }
	          }
	        }
        #endif
        dt[total].text = 0; /* terminate array */
        break;

      case OSD_KEY_PGDN:
        if(total == 0)
          break;
        if(total < 10)
          Continue = -1;
        total = 0;

		/* JB 980407 */
		Begin = Continue+1;
		Continue = 0;
		for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
		{
			if (ext->start <= Begin && ext->end >= Begin)
				for (i = Begin - ext->start; i <= ext->end - ext->start; i++)
					if (ext->data[i] != 0)
					{
						sprintf (str2[total],"%04X = %02X", i+ext->start, ext_sr->data[i]);
						dt[total].text = str2[total];

						total++;

						if (total >= 10)
						{
							Continue = i+ext->start;
							break;
						}
					}
		}

		#if 0
	        for (i=Continue+1;i<ram_size;i++){
	          if(FlagTable[i] != 0){
	            sprintf(str2[total],"%04X = %02X",i,StartRam[i]);
	            dt[total].text = str2[total];

	            total++;

	            if(total >= 10){
	              Continue = i;
	              break;
	            }
	          }
	        }
        #endif
        dt[total].text = 0; /* terminate array */
        break;

      case OSD_KEY_F2:
        if(total == 0)
          break;
/* Add all the list to the LoadedCheatTable */
        if(LoadedCheatTotal > 99){
          xprintf(0,y,"Not Added: Cheat List Is Full.");
          break;
        }
        count = 0;

		/* JB 980407 */
		for (ext = FlagTable, ext_sr = StartRam; ext->data && count<100; ext++, ext_sr++)
		{
			for (i = 0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
				{
					count++;
					LoadedCheatTable[LoadedCheatTotal].Special = 0;
					LoadedCheatTable[LoadedCheatTotal].Count = 0;
					LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
					LoadedCheatTable[LoadedCheatTotal].Address = i+ext->start;
					LoadedCheatTable[LoadedCheatTotal].Data = ext_sr->data[i];
					sprintf (str2[11],"%04X = %02X", i+ext->start, ext_sr->data[i]);
					strcpy(LoadedCheatTable[LoadedCheatTotal].Name,str2[11]);
					LoadedCheatTotal++;
				}
				if(LoadedCheatTotal > 99)
				break;
		}

		#if 0
	        for (i=0;i<ram_size;i++){
	          if(FlagTable[i] != 0){
	            count++;
	            LoadedCheatTable[LoadedCheatTotal].Special = 0;
	            LoadedCheatTable[LoadedCheatTotal].Count = 0;
	            LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
	            LoadedCheatTable[LoadedCheatTotal].Address = i;
	            LoadedCheatTable[LoadedCheatTotal].Data = StartRam[i];
	            sprintf(str2[11],"%04X = %02X",i,StartRam[i]);
	            strcpy(LoadedCheatTable[LoadedCheatTotal].Name,str2[11]);
	            LoadedCheatTotal++;
	          }
	          if(LoadedCheatTotal > 99)
	            break;
	        }
        #endif
        xprintf(0,y,"%d Added",count);

/* JRT5 Print Message If Cheat List Is Full */
        if(LoadedCheatTotal > 99){
					y += Machine->uifont->height;
					xprintf(0,y,"(Cheat List Is Full.)");
				}
/* JRT5 */
        break;


      case OSD_KEY_ENTER:
        if(total == 0)
          break;

/* Add the selected address to the LoadedCheatTable */
        if(LoadedCheatTotal > 99){
          xprintf(0,y,"Not Added: Cheat List Is Full.");
          break;
        }
        LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
        LoadedCheatTable[LoadedCheatTotal].Special = 0;
        LoadedCheatTable[LoadedCheatTotal].Count = 0;
        sscanf(dt[s].text,"%X",&i);
        LoadedCheatTable[LoadedCheatTotal].Address = i;
        /*LoadedCheatTable[LoadedCheatTotal].Data = StartRam[i];*/
        LoadedCheatTable[LoadedCheatTotal].Data = RD_STARTRAM(i); /* JB 980407 */
        strcpy(LoadedCheatTable[LoadedCheatTotal].Name,dt[s].text);
        LoadedCheatTotal++;
        xprintf(0,y,"%s Added",dt[s].text);

        break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
      default:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */


  osd_clearbitmap(Machine->scrbitmap);
}


/*****************
 *
 */
void ChooseWatch(void)
{
/* JRT1 10-23-97 BEGIN */
 int  iFontHeight = ( Machine -> uifont -> height );
 int  iFontWidth = ( Machine -> uifont -> width );
 int  iSelectedWatch = 0;
 char *paDisplayText[] = {
                "<+> and Right arrow key: +1",
                "<-> and Left  arrow key: -1",
                "<1/2/3/4/5/6>: +1 digit",			/* JB 980407 */
                "<Delete> to disable a watch",
                "<Enter> to copy previous watch",
                "<I>, <J>, <K>, and <L> move",
                "the watch positions",
/* JRT6 */
                "(All \"F\"s means watch disabled)",
/* JRT6 */
                "\0" };
 /* JRT1 10-23-97 END */
 int i,s,y,key,done;
 int total;
 struct DisplayText dt[20];
 char str2[12][10];
 char buf[40];
 char buf2[10];
 int trueorientation;

  /* hack: force the display into standard orientation to avoid */
  /* rotating the user interface */
  trueorientation = Machine->orientation;
  Machine->orientation = ORIENTATION_DEFAULT;

  osd_clearbitmap(Machine->scrbitmap);

/* JRT1 10-23-97 BEGIN */
  /**/
  /* Add Menu Text To DisplayText Array*/
  /**/
  for( i = 0; i < 8; i++ )
  {
    if( i )                       /* If Not First One*/
      dt[ i ].y = ( dt[ i - 1 ].y + iFontHeight + 2 );  /* Increment From Last One*/
    else                          /* If First One*/
      dt[ i ].y = 35;                 /* Set To Static Value*/
    dt[ i ].color = DT_COLOR_WHITE;           /* Draw As White*/
    dt[ i ].text = paDisplayText[ i ];          /* Assign String*/
    dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
    if(dt[i].x > Machine->uiwidth)
      dt[i].x = 0;
  }
  y = ( dt[ 7 ].y + ( 3 * iFontHeight ) );          /* Calculate Watch Start*/
  total = 8;                        /* Start Of Watch DT Locations*/
/* JRT1 10-23-97 END */

  for (i=0;i<10;i++){

/* JRT1 10-23-97 BEGIN */
/* JRT6/JB - modified JB 980424 */
	sprintf(str2[ i ], pShortAddrTemplate(0), Watches[i]);		/* Use Loop Index, Not "total"*/
/* JRT6/JB */
	dt[total].text = str2[ i ];								/* Use Loop Index, Not "total"*/
/* JRT1 10-23-97 END */

    dt[total].x = (Machine->uiwidth - Machine->uifont->width * 5) / 2;
    dt[total].y = y;
    dt[total].color = DT_COLOR_WHITE;

    total++;

    y += Machine->uifont->height;
  }
  dt[total].text = 0; /* terminate array */

/* JRT1 10-23-97 BEGIN */
  s = 8;                          /* Start Of Watch DT Locations*/
  iSelectedWatch = ( s - 8 );               /* Init First Selected Watch*/
/* JRT1 10-23-97 END */

  done = 0;
  do
  {
/* Display a test to see where the watches are */
    buf[0] = 0;
    for(i=0;i<10;i++)
/* JRT6/JB - modified JB 980424 */
      if (Watches[i] != MAX_ADDRESS(0)){
/* JRT6/JB */
        /*sprintf(buf2,"%02X ",Machine->memory_region[0][Watches[i]]);*/
        sprintf (buf2, "%02X ", RD_GAMERAM(0,Watches[i]));	/* JB 980407 */
        strcat(buf,buf2);
      }

/* JRT1 10-23-97 BEGIN */
  /**/
  /* Clear Old Watch Space*/
  /**/
  for( i = ( WatchY ? WatchY - 1 : WatchY );
     i < ( WatchY + iFontHeight + 1 ); i++ )      /* For Watch Area*/
  {
    memset( Machine -> scrbitmap -> line[ i ], 0,
    Machine -> scrbitmap -> width );      /* Clear Old Drawing*/
  }
    while( WatchX >= ( Machine->uiwidth -
        ( iFontWidth * (int)strlen( buf ) ) ) )   /* If Going To Draw OffScreen*/
  {
    if( !WatchX-- )                   /* Adjust Drawing X Offset*/
      break;                      /* Stop If Too Small*/
  }
/* JRT1 10-23-97 END */

  for (i = 0;i < (int)strlen(buf);i++)
    drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,WatchX+(i*Machine->uifont->width),WatchY,0,TRANSPARENCY_NONE,0);

/* JRT1 10-23-97 BEGIN */
    for (i = 8;i < total;i++)               /* New Watch Start Location*/
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
/* JRT1 10-23-97 END */

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1) s++;
        else s = 8;
/* JRT1 10-23-97 BEGIN */
		    iSelectedWatch = ( s - 8 );             /* Calculate Selected Watch*/
/* JRT1 10-23-97 END */
        break;

      case OSD_KEY_UP:
        if (s > 8) s--;
        else s = total - 1;
/* JRT1 10-23-97 BEGIN */
		    iSelectedWatch = ( s - 8 );             /* Calculate Selected Watch*/
/* JRT1 10-23-97 END */
        break;

      case OSD_KEY_J:
        if(WatchX != 0)
          WatchX--;
        break;
      case OSD_KEY_L:
/* JRT1 10-23-97 BEGIN */
        if(WatchX <= ( Machine->uiwidth -
          ( iFontWidth * (int)strlen( buf ) ) ) )
/* JRT1 10-23-97 END */
          WatchX++;
        break;
      case OSD_KEY_K:                   /* (Minus Extra One)*/
/* JRT1 10-23-97 BEGIN */
        if(WatchY <= (Machine->uiheight-Machine->uifont->height) - 1 )
/* JRT1 10-23-97 END */
          WatchY++;
        break;
      case OSD_KEY_I:
        if(WatchY != 0)
          WatchY--;
        break;

/* JRT1 10-23-97 BEGIN */                 /* Use iSelectedWatch As Watch Indexes*/
/* JRT6 : Begin Mass Changes By JB and JRT - modified JB 980424 */
      case OSD_KEY_MINUS_PAD:
      case OSD_KEY_LEFT:
        if(Watches[ iSelectedWatch ] <= 0)
          Watches[ iSelectedWatch ] = MAX_ADDRESS(0);
        else
          Watches[ iSelectedWatch ]--;
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;

      case OSD_KEY_PLUS_PAD:
      case OSD_KEY_RIGHT:
        if(Watches[ iSelectedWatch ] >= MAX_ADDRESS(0))
          Watches[ iSelectedWatch ] = 0;
        else
          Watches[ iSelectedWatch ]++;
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;

      case OSD_KEY_PGDN:
		if(Watches[ iSelectedWatch ] <= 0x100)
        	Watches[ iSelectedWatch ] |= (0xFFFFFF00 & MAX_ADDRESS(0));
        else
			Watches[ iSelectedWatch ] -= 0x100;
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;

      case OSD_KEY_PGUP:
		if(Watches[ iSelectedWatch ] >= 0xFF00)
        	Watches[ iSelectedWatch ] |= (0xFFFF00FF & MAX_ADDRESS(0));
		else
			Watches[ iSelectedWatch ] += 0x100;
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;
      case OSD_KEY_6:	/* JB 980424 */
      	if (ADDRESS_BITS(0) < 24) break;
      case OSD_KEY_5:
      	if (ADDRESS_BITS(0) < 20) break;
      case OSD_KEY_4:
      	if (ADDRESS_BITS(0) < 16) break;
      case OSD_KEY_3:
      case OSD_KEY_2:
      case OSD_KEY_1:
	      {
	      		int addr = Watches[ iSelectedWatch ];	/* copy address*/
	      		int digit = (OSD_KEY_6 - key);	/* if key is OSD_KEY_6, digit = 0*/
	      		int mask;

	      		/* adjust digit based on cpu address range */
	      		digit -= (6 - ADDRESS_BITS(0) / 4);

	      		mask = 0xF << (digit * 4);	/* if digit is 1, mask = 0xf0*/

	      		if ((addr & mask) == mask)
	      			/* wrap hex digit around to 0 if necessary */
	      			addr &= ~mask;
	      		else
	      			/* otherwise bump hex digit by 1 */
	      			addr += (0x1 << (digit * 4));

				Watches[ iSelectedWatch ] = addr;
				sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
		  }
		  break;

      case OSD_KEY_DEL:
        Watches[ iSelectedWatch ] = MAX_ADDRESS(0);
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;

      case OSD_KEY_ENTER:
        if(s == 8)                      /* Start Of Watch DT Locations*/
          break;
        Watches[ iSelectedWatch ] = Watches[ iSelectedWatch - 1];
        sprintf (str2[ iSelectedWatch ], pShortAddrTemplate(0), Watches[ iSelectedWatch ]);
        break;
/* JRT6/JB */
/* JRT1 10-23-97 END */

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */

/* Set Watch Flag */
  WatchesFlag = 0;
  for(i=0;i<10;i++)
    if(Watches[i] != MAX_ADDRESS(0))	/* JB 980424 */
      WatchesFlag = 1;

  osd_clearbitmap(Machine->scrbitmap);

  Machine->orientation = trueorientation;
}


/*****************
 *
 */
int cheat_menu(void)
{
  struct DisplayText dt[10];
  int i,s,key,done;
  int total;

/* Cheat menu */
  total = 6;
  dt[0].text = "Load And/Or Enable A Cheat";
  dt[1].text = "Start A New Cheat Search";
  dt[2].text = "Continue Search";
  dt[3].text = "Memory Watch";
  dt[4].text = "";
  dt[5].text = "Return To Main Menu";
  dt[6].text = 0; /* terminate array */
  for (i = 0;i < total;i++)
  {
    dt[i].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[i].text)) / 2;
    if(dt[i].x > Machine->uiwidth)
      dt[i].x = 0;
    dt[i].y = i * ( Machine->uifont->height + 2 );/*4*Machine->uifont->height;*/
    if (i == total-1) dt[i].y += Machine->uifont->height;
  }

  osd_clearbitmap(Machine->scrbitmap);

  DisplayCheats(dt[total - 1].x,dt[total - 1].y+total*Machine->uifont->height);

  s = 0;
  done = 0;
  do
  {
    for (i = 0;i < total;i++)
    {
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
    }

    displaytext(dt,0);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_DOWN:
        if (s < total - 1) s++;
        else s = 0;

/* JRT5 For Space In Menu */
		if( s == 4 )
			s++;
/* JRT5 */
        break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;

/* JRT5 For Space In Menu */
		if( s == 4 )
			s--;
/* JRT5 */
        break;

      case OSD_KEY_ENTER:
        switch (s)
        {
          case 0:

            SelectCheat();


/* JRT5 For Space In Menu */
            DisplayCheats(dt[total - 1].x,dt[total - 1].y+2*Machine->uifont->height);
            break;

          case 1:
            StartCheat();
            DisplayCheats(dt[total - 1].x,dt[total - 1].y+2*Machine->uifont->height);
            break;

          case 2:
            ContinueCheat();
            DisplayCheats(dt[total - 1].x,dt[total - 1].y+2*Machine->uifont->height);
            break;
/* JRT5 END */

          case 3:
            ChooseWatch();
            break;

          case 5:
            done = 1;
            break;
        }
        break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */

  /* clear the screen before returning */
  osd_clearbitmap(Machine->scrbitmap);


  if (done == 2) return 1;
  else return 0;
}

/* JRT5 Help System Functions */

/* JB 980505 */
/* display help text and wait for key press */
static void ShowHelp (struct DisplayText *dt)
{
	int	iFontHeight = Machine->uifont->height;
	int	iCounter = 0;
	struct DisplayText *text = dt;

	while (dt->text)
	{
		dt->x = 0;
		dt->y = iFontHeight * iCounter++;

		/* @ at beginning of string indicates special text */
		if (dt->text[0] == '@')
		{
			dt->color = DT_COLOR_YELLOW;
			dt->text++;						/* skip '@'*/
		}
		else
			dt->color = DT_COLOR_WHITE;
		dt++;
	}
	osd_clearbitmap (Machine->scrbitmap);	/* Clear Screen*/
	displaytext (text, 0);					/* Draw Help Text*/
	osd_read_key ();						/* Wait For A Key*/
	osd_clearbitmap (Machine->scrbitmap);	/* Clear Screen Again*/
}

/* JB 980505 */
void CheatListHelp (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@       Cheat List Help" },		/* Header*/
		{ "" },								/* Space*/
		{ "" },								/* Space*/
		{ "@Delete:" },						/* Delete Cheat Info*/
		{ "  Delete the selected Cheat" },
		{ "  from the Cheat List." },
		{ "  (Not from the Cheat File!)" },
		{ "" },
		{ "@Add:" }, 						/* Add Cheat Info*/
		{ "  Add a new (blank) Cheat to" },
		{ "  the Cheat List." },
		{ "" },
		{ "@Save:" },						/* Save Cheat Info*/
		{ "  Save the selected Cheat in" },
		{ "  the Cheat File." },
		{ "" },
		{ "@Watch:" },						/* Address Watcher Info*/
		{ "  Activate a Memory Watcher" },
		{ "  at the address that the" },
		{ "  selected Cheat modifies." },
		{ "" },
		{ "@Edit:" },						/* Edit Cheat Info*/
		{ "  Edit the Properties of the" },
		{ "  selected Cheat." },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },	/* Return Prompt*/
		{ 0 }								/* End Of Text*/
	};

	ShowHelp (dtHelpText);
}

/* JB 980505 */
void StartSearchHelp (void)
{
	struct DisplayText dtHelpText[] =
	{
		{ "@   Cheat Search Help 1" },		/* Header*/
		{ "" },								/* Space*/
		{ "" },								/* Space*/
		{ "@Lives Or Some Other Value:" },	/* Lives/# Search Info*/
		{ " Searches for a specific" },
		{ " value that you specify." },
		{ "" },
		{ "@Timers:" }, 					/* Timers Search Info*/
		{ " Starts by storing all of" },
		{ " the game's memory, and then" },
		{ " looking for values that" },
		{ " have changed by a specific" },
		{ " amount from the value that" },
		{ " was stored when the search" },
		{ " was started or continued." },
		{ "" },
		{ "@Energy:" },						/* Energy Search Info*/
		{ " Similar to Timers. Searches" },
		{ " for values that are Greater" },
		{ " than, Less than, or Equal" },
		{ " to the values stored when" },
		{ " the search was started or" },
		{ " continued." },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },/* Continue Prompt*/
		{ 0 }								/* End Of Text*/
	};

	struct DisplayText dtHelpText2[] =
	{
		{ "@   Cheat Search Help 2" },		/* Header*/
		{ "" },								/* Space*/
		{ "" },								/* Space*/
		{ "@Status:" },						/* Status Search Info*/
		{ " Searches for a Bit or Flag" },
		{ " that may or may not have" },
		{ " toggled its value since" },
		{ " the search was started." },
		{ "" },
		{ "@Slow But Sure:" },				/* SBS Search Info*/
		{ " This search stores all of" },
		{ " the game's memory, and then" },
		{ " looks for values that are" },
		{ " the Same As, or Different" },
		{ " from the values stored when" },
		{ " the search was started." },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },	/* Return Prompt*/
		{ 0 }
	};

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);
}

/* JB 980505 */
void EditCheatHelp (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@     Edit Cheat Help 1" },		/* Header*/
		{ "" },								/* Space*/
		{ "" },								/* Space*/
		{ "@Name:" },						/* Cheat Name Info*/
		{ " Displays the Name of this" },
		{ " Cheat. It can be edited by" },
		{ " hitting <ENTER> while it is" },
		{ " selected.  Cheat Names are" },
		{ " limited to 25 characters." },
		{ " You can use <SHIFT> to" },
		{ " uppercase a character, but" },
		{ " only one character at a" },
		{ " time!" },
		{ "" },
		{ "@CPU:" }, 						/* Cheat CPU Info*/
		{ " Specifies the CPU (memory" },
		{ " region) that gets affected." },
		{ "" },
		{ "@Address:" },					/* Cheat Address Info*/
		{ " The Address of the location" },
		{ " in memory that gets set to" },
		{ " the new value." },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },/* Continue Prompt*/
		{ 0 }								/* End Of Text*/
	};

	struct DisplayText	dtHelpText2[] =
	{
		{ "@     Edit Cheat Help 2" },		/* Header*/
		{ "" },								/* Space*/
		{ "" },								/* Space*/
		{ "@Value:" },						/* Cheat Value Info*/
		{ " The new value that gets" },
		{ " placed into the specified" },
		{ " Address while the Cheat is" },
		{ " active." },
		{ "" },
		{ "@Type:" },						/* Cheat Type Info*/
		{ " Specifies how the Cheat" },
		{ " will actually work. See the" },
		{ " CHEAT.DOC file for details." },
		{ "" },
		{ "@Notes:" },						/* Extra Notes*/
		{ " Use the Right and Left" },
		{ " arrow keys to increment and" },
		{ " decrement values, or to" },
		{ " select from pre-defined" },
		{ " Cheat Names.  The <1>, <2>," },	/* JB 980407 */
		{ " <3>, <4> etc. keys are used" },	/* JB 980407 */
		{ " to increment the number in" },
		{ " that specific column of a" },
		{ " value." },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },	/* Return Prompt*/
		{ 0 }
	};

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);
}
/* JRT5 */

