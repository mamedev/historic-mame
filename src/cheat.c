/*********************************************************************

  cheat.c

  This is a massively rewritten version of the cheat engine. There
  are still quite a few features that are broken or not reimplemented.

  Busted (probably permanently) is the ability to use the UI_CONFIGURE
  key to pop the menu system on and off while retaining the current
  menu selection. The sheer number of submenus involved makes this
  a difficult task to pull off here.

  TODO:

  * comment cheats
  * memory searches
  * saving of cheats to a file
  * inserting/deleting new cheats
  * cheats as watchpoints
  * key shortcuts to the search menus

(Description from Pugsy's cheat.dat)

; all fields are separated by a colon (:)
;  -Name of the game (short name) [zip file or directory]
;  -No of the CPU usually 0, only different in multiple CPU games
;  -Address in Hexadecimal (where to poke)
;  -Data to put at this address in hexadecimal (what to poke)
;  -Special (see below) usually 000
;   -000 : the byte is poked every time and the cheat remains in active list.
;   -001 : the byte is poked once and the cheat is removed from active list.
;   -002 : the byte is poked every one second and the cheat remains in active
;          list.
;   -003 : the byte is poked every two seconds and the cheat remains in active
;          list.
;   -004 : the byte is poked every five seconds and the cheat remains in active
;          list.
;   -005 : the byte is poked one second after the original value has changed
;          and the cheat remains in active list.
;   -006 : the byte is poked two seconds after the original value has changed
;          and the cheat remains in active list.
;   -007 : the byte is poked five seconds after the original value has changed
;          and the cheat remains in active list.
;   -008 : the byte is poked unless the original value in being decremented by
;          1 each frame and the cheat remains in active list.
;   -009 : the byte is poked unless the original value in being decremented by
;          2 each frame and the cheat remains in active list.
;   -010 : the byte is poked unless the original value in being decremented by
;          3 each frame and the cheat remains in active list.
;   -011 : the byte is poked unless the original value in being decremented by
;          4 each frame and the cheat remains in active list.
;   -020 : the bits are set every time and the cheat remains in active list.
;   -021 : the bits are set once and the cheat is removed from active list.
;   -040 : the bits are reset every time and the cheat remains in active list.
;   -041 : the bits are reset once and the cheat is removed from active list.
;   -060 : the user selects a decimal value from 0 to byte
;          (display : 0 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -061 : the user selects a decimal value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once when it changes
;          and the cheat is removed from the active list.
;   -062 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -063 : the user selects a BCD value from 0 to byte
;          (display : 0 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -064 : the user selects a BCD value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once when it changes
;          and the cheat is removed from the active list.
;   -065 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -070 : the user selects a decimal value from 0 to byte
;          (display : 0 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -071 : the user selects a decimal value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once and the cheat is
;          removed from the active list.
;   -072 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -073 : the user selects a BCD value from 0 to byte
;          (display : 0 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -074 : the user selects a BCD value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once and the cheat is
;          removed from the active list.
;   -075 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -500 to 575: These cheat types are identical to types 000 to 075 except
;                they are used in linked cheats (i.e. of 1/8 type). The first
;                cheat in the link list will be the normal type (eg type 000)
;                and the remaining cheats (eg 2/8...8/8) will be of this type
;                (eg type 500).
;   -998 : this is used as a watch cheat, ideal for showing answers in quiz
;          games .
;   -999 : this is used for comments only, cannot be enabled/selected by the
;          user.
;  -Name of the cheat
;  -Description for the cheat

*********************************************************************/

#include "driver.h"
#include "ui_text.h"

#ifndef NEOFREE
#ifndef TINY_COMPILE
extern struct GameDriver driver_neogeo;
#endif
#endif

extern unsigned char *memory_find_base (int cpu, int offset);

/*****************
 *
 * Cheats
 *
 */
#define SUBCHEAT_FLAG_DONE		0x0001
#define SUBCHEAT_FLAG_TIMED		0x0002

struct subcheat_struct
{
	int cpu;
	offs_t address;
	data_t data;
	data_t backup;						/* The original value of the memory location, checked against the current */
	UINT32 code;
	UINT16 flags;
	data_t min;
	data_t max;
	UINT32 frames_til_trigger;			/* the number of frames until this cheat fires (does not change) */
	UINT32 frame_count;					/* decrementing frame counter to determine if cheat should fire */
};

#define CHEAT_FLAG_ACTIVE	0x01
#define CHEAT_FLAG_WATCH	0x02
#define CHEAT_FLAG_COMMENT	0x04

struct cheat_struct
{
	char *name;
	char *comment;
	UINT8 flags;						/* bit 0 = active, 1 = watchpoint, 2 = comment */
	int num_sub;						/* number of cheat cpu/address/data/code combos for this one cheat */
	struct subcheat_struct *subcheat;	/* a variable-number of subcheats are attached to each "master" cheat */
};

struct memory_struct
{
	int Enabled;
	char name[40];
	mem_write_handler handler;
};

enum
{
	kCheatSpecial_Poke = 0,
	kCheatSpecial_Poke1 = 2,
	kCheatSpecial_Poke2 = 3,
	kCheatSpecial_Poke5 = 4,
	kCheatSpecial_Delay1 = 5,
	kCheatSpecial_Delay2 = 6,
	kCheatSpecial_Delay5 = 7,
	kCheatSpecial_Backup1 = 8,
	kCheatSpecial_Backup4 = 11,
	kCheatSpecial_SetBit1 = 22,
	kCheatSpecial_SetBit2 = 23,
	kCheatSpecial_SetBit5 = 24,
	kCheatSpecial_ResetBit1 = 42,
	kCheatSpecial_ResetBit2 = 43,
	kCheatSpecial_ResetBit5 = 44,
	kCheatSpecial_UserFirst = 60,
	kCheatSpecial_m0d0c = 60,			/* minimum value 0, display range 0 to byte, poke when changed */
	kCheatSpecial_m0d1c = 61,			/* minimum value 0, display range 1 to byte+1, poke when changed */
	kCheatSpecial_m1d1c = 62,			/* minimum value 1, display range 1 to byte, poke when changed */
	kCheatSpecial_m0d0bcdc = 63,		/* BCD, minimum value 0, display range 0 to byte, poke when changed */
	kCheatSpecial_m0d1bcdc = 64,		/* BCD, minimum value 0, display range 1 to byte+1, poke when changed */
	kCheatSpecial_m1d1bcdc = 65,		/* BCD, minimum value 1, display range 1 to byte, poke when changed */
	kCheatSpecial_m0d0 = 70,			/* minimum value 0, display range 0 to byte */
	kCheatSpecial_m0d1 = 71,			/* minimum value 0, display range 1 to byte+1 */
	kCheatSpecial_m1d1 = 72,			/* minimum value 1, display range 1 to byte */
	kCheatSpecial_m0d0bcd = 73,			/* BCD, minimum value 0, display range 0 to byte */
	kCheatSpecial_m0d1bcd = 74,			/* BCD, minimum value 0, display range 1 to byte+1 */
	kCheatSpecial_m1d1bcd = 75,			/* BCD, minimum value 1, display range 1 to byte */
	kCheatSpecial_UserLast = 75,
	kCheatSpecial_Last = 99,
	kCheatSpecial_LinkStart = 500,		/* only used when loading the database */
	kCheatSpecial_LinkEnd = 599,		/* only used when loading the database */
	kCheatSpecial_Watch = 998,
	kCheatSpecial_Comment = 999,
	kCheatSpecial_Timed = 1000
};

/*****************
 *
 * Watchpoints
 *
 */
#define MAX_WATCHES 	20

struct watch_struct
{
	int cheat_num;		/* if this watchpoint is tied to a cheat, this is the index into the cheat array. -1 if none */
	UINT32 address;
	INT16 cpu;
	UINT8 num_bytes;	/* number of consecutive bytes to display */
	UINT8 label_type;	/* none, address, text */
	char label[255];	/* optional text label */
	UINT16 x, y;		/* position of watchpoint on screen */
};

static struct watch_struct watches[MAX_WATCHES];
static int is_watch_active; /* true if at least one watchpoint is active */
static int is_watch_visible; /* we can toggle the visibility for all on or off */

/* in hiscore.c */
int computer_readmem_byte(int cpu, int addr);
void computer_writemem_byte(int cpu, int addr, int value);

/* Some macros to simplify the code */
#define READ_CHEAT				computer_readmem_byte (subcheat->cpu, subcheat->address)
#define WRITE_CHEAT				computer_writemem_byte (subcheat->cpu, subcheat->address, subcheat->data)
#define COMPARE_CHEAT			(computer_readmem_byte (subcheat->cpu, subcheat->address) != subcheat->data)
#define CPU_AUDIO_OFF(index)	((Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU) && (Machine->sample_rate == 0))

#define MAX_LOADEDCHEATS	200
#define CHEAT_FILENAME_MAXLEN	255

/* This variables are not static because of extern statement */
char *cheatfile = "cheat.dat";
char database[CHEAT_FILENAME_MAXLEN+1];

int he_did_cheat;

//int fastsearch = 2;
//int sologame   = 0;

/* Local routines */
static INT32 DisplayHelpFile (INT32 selected);
static INT32 EditCheatMenu (INT32 selected, UINT8 cheatnum);
static INT32 CommentMenu (INT32 selected, int cheat_index);

/* Local variables */
/* static int	search_started = 0; */

#if 0
static struct ExtMemory StartRam[MAX_EXT_MEMORY];
static struct ExtMemory BackupRam[MAX_EXT_MEMORY];
static struct ExtMemory FlagTable[MAX_EXT_MEMORY];

static struct ExtMemory OldBackupRam[MAX_EXT_MEMORY];
static struct ExtMemory OldFlagTable[MAX_EXT_MEMORY];
#endif

static int ActiveCheatTotal;										/* number of cheats currently active */
static int LoadedCheatTotal;										/* total number of cheats */
static struct cheat_struct CheatTable[MAX_LOADEDCHEATS+1];

static int CheatEnabled;

/* Function to test if a value is BCD (returns 1) or not (returns 0) */
int IsBCD(int ParamValue)
{
	return(((ParamValue % 0x10 <= 9) & (ParamValue <= 0x99)) ? 1 : 0);
}

#if 0
/* return a format specifier for printf based on cpu address range */
static char *FormatAddr(int cpu, int addtext)
{
	static char bufadr[10];
	static char buffer[18];
	int i;

	memset (buffer, '\0', strlen(buffer));
	switch (cpunum_address_bits(cpu) >> 2)
	{
		case 4:
			strcpy (bufadr, "%04X");
			break;
		case 5:
			strcpy (bufadr, "%05X");
			break;
		case 6:
			strcpy (bufadr, "%06X");
			break;
		case 7:
			strcpy (bufadr, "%07X");
			break;
		case 8:
			strcpy (bufadr, "%08X");
			break;
		default:
			strcpy (bufadr, "%X");
			break;
	}
	  if (addtext)
	  {
	strcpy (buffer, "Addr:  ");
		for (i = strlen(bufadr) + 1; i < 8; i ++)
		strcat (buffer, " ");
	  }
	  strcat (buffer,bufadr);
	return buffer;
}
#endif

#if 0
/* make a copy of a source ram table to a dest. ram table */
static void copy_ram (struct ExtMemory *dest, struct ExtMemory *src)
{
	struct ExtMemory *ext_dest, *ext_src;

	for (ext_src = src, ext_dest = dest; ext_src->data; ext_src++, ext_dest++)
	{
		memcpy (ext_dest->data, ext_src->data, ext_src->end - ext_src->start + 1);
	}
}

/* make a copy of each ram area from search CPU ram to the specified table */
static void backup_ram (struct ExtMemory *table)
{
	struct ExtMemory *ext;
	unsigned char *gameram;

	for (ext = table; ext->data; ext++)
	{
		int i;
		gameram = memory_find_base (Searchcpu, ext->start);
		memcpy (ext->data, gameram, ext->end - ext->start + 1);
		for (i=0; i <= ext->end - ext->start; i++)
			ext->data[i] = computer_readmem_byte(Searchcpu, i+ext->start);
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

/* Returns 1 if memory area has to be skipped */
int SkipBank(int CpuToScan, int *BankToScanTable, mem_write_handler handler)
{
	int res = 0;

	if ((fastsearch == 1) || (fastsearch == 2))
	{
		switch ((FPTR)handler)
		{
			case (FPTR)MWA_RAM:
				res = !BankToScanTable[0];
				break;
			case (FPTR)MWA_BANK1:
				res = !BankToScanTable[1];
				break;
			case (FPTR)MWA_BANK2:
				res = !BankToScanTable[2];
				break;
			case (FPTR)MWA_BANK3:
				res = !BankToScanTable[3];
				break;
			case (FPTR)MWA_BANK4:
				res = !BankToScanTable[4];
				break;
			case (FPTR)MWA_BANK5:
				res = !BankToScanTable[5];
				break;
			case (FPTR)MWA_BANK6:
				res = !BankToScanTable[6];
				break;
			case (FPTR)MWA_BANK7:
				res = !BankToScanTable[7];
				break;
			case (FPTR)MWA_BANK8:
				res = !BankToScanTable[8];
				break;
			default:
				res = 1;
				break;
		}
	}
	return(res);
}
#endif

/* Function to rename the cheatfile (returns 1 if the file has been renamed else 0)*/
int RenameCheatFile(int merge, int DisplayFileName, char *filename)
{
	return 0;
}

/* Function who loads the cheats for a game */
int SaveCheat(int NoCheat)
{
	return 0;
}

/***************************************************************************

  cheat_set_code

  Given a cheat code, sets the various attribues of the cheat structure.
  This is to aid in making the cheat engine more flexible in the event that
  someday the codes are restructured or the case statement in DoCheat is
  simplified from its current form.

***************************************************************************/
void cheat_set_code (struct subcheat_struct *subcheat, int code, int cheat_num)
{
	switch (code)
	{
		case kCheatSpecial_Poke1:
		case kCheatSpecial_Delay1:
		case kCheatSpecial_SetBit1:
		case kCheatSpecial_ResetBit1:
			subcheat->frames_til_trigger = 1 * Machine->drv->frames_per_second; /* was 60 */
			break;
		case kCheatSpecial_Poke2:
		case kCheatSpecial_Delay2:
		case kCheatSpecial_SetBit2:
		case kCheatSpecial_ResetBit2:
			subcheat->frames_til_trigger = 2 * Machine->drv->frames_per_second; /* was 60 */
			break;
		case kCheatSpecial_Poke5:
		case kCheatSpecial_Delay5:
		case kCheatSpecial_SetBit5:
		case kCheatSpecial_ResetBit5:
			subcheat->frames_til_trigger = 5 * Machine->drv->frames_per_second; /* was 60 */
			break;
		case kCheatSpecial_Comment:
			subcheat->frames_til_trigger = 0;
			subcheat->address = 0;
			subcheat->data = 0;
			CheatTable[cheat_num].flags |= CHEAT_FLAG_COMMENT;
			break;
		case kCheatSpecial_Watch:
			subcheat->frames_til_trigger = 0;
			subcheat->data = 0;
			CheatTable[cheat_num].flags |= CHEAT_FLAG_WATCH;
			break;
		default:
			subcheat->frames_til_trigger = 0;
			break;
	}

	/* Set the minimum value */
	if ((code == kCheatSpecial_m1d1c) ||
		(code == kCheatSpecial_m1d1bcdc) ||
		(code == kCheatSpecial_m1d1) ||
		(code == kCheatSpecial_m1d1bcd))
		subcheat->min = 1;
	else
		subcheat->min = 0;

	/* Set the maximum value */
	if ((code >= kCheatSpecial_UserFirst) &&
		(code <= kCheatSpecial_UserLast))
	{
		subcheat->max = subcheat->data;
		subcheat->data = 0;
	}
	else
		subcheat->max = 0xff;

	subcheat->code = code;
}

/***************************************************************************

  cheat_set_status

  Given an index into the cheat table array, make the selected cheat
  either active or inactive.

  TODO: possibly support converting to a watchpoint in here.

***************************************************************************/
void cheat_set_status (int cheat_num, int active)
{
	int i;

	if (active) /* enable the cheat */
	{
		for (i = 0; i <= CheatTable[cheat_num].num_sub; i ++)
		{
			/* Reset the active variables */
			CheatTable[cheat_num].subcheat[i].frame_count = 0;
			CheatTable[cheat_num].subcheat[i].backup = 0;
		}

		/* only add if there's a cheat active already */
		if ((CheatTable[cheat_num].flags & CHEAT_FLAG_ACTIVE) == 0)
		{
			CheatTable[cheat_num].flags |= CHEAT_FLAG_ACTIVE;
			ActiveCheatTotal++;
		}

		/* tell the MAME core that we're cheaters! */
		he_did_cheat = 1;
	}
	else /* disable the cheat (case 0, 2) */
	{
		for (i = 0; i <= CheatTable[cheat_num].num_sub; i ++)
		{
			/* Reset the active variables */
			CheatTable[cheat_num].subcheat[i].frame_count = 0;
			CheatTable[cheat_num].subcheat[i].backup = 0;
		}

		/* only add if there's a cheat active already */
		if (CheatTable[cheat_num].flags & CHEAT_FLAG_ACTIVE)
		{
			CheatTable[cheat_num].flags &= ~CHEAT_FLAG_ACTIVE;
			ActiveCheatTotal--;
		}
	}
}

void cheat_insert_new (int cheat_num)
{
	/* if list is full, bail */
	if (LoadedCheatTotal == MAX_LOADEDCHEATS) return;

	/* if the index is off the end of the list, fix it */
	if (cheat_num > LoadedCheatTotal) cheat_num = LoadedCheatTotal;

	/* clear space in the middle of the table if needed */
	if (cheat_num < LoadedCheatTotal)
		memmove (&CheatTable[cheat_num+1], &CheatTable[cheat_num], sizeof (struct cheat_struct) * (LoadedCheatTotal - cheat_num));

	/* clear the new entry */
	memset (&CheatTable[cheat_num], 0, sizeof (struct cheat_struct));

	CheatTable[cheat_num].name = malloc (strlen (ui_getstring(UI_none)) + 1);
	strcpy (CheatTable[cheat_num].name, ui_getstring(UI_none));

	CheatTable[cheat_num].subcheat = calloc (1, sizeof (struct subcheat_struct));

	/*add one to the total */
	LoadedCheatTotal ++;
}

void cheat_delete (int cheat_num)
{
	/* if the index is off the end, make it the last one */
	if (cheat_num >= LoadedCheatTotal) cheat_num = LoadedCheatTotal - 1;

	/* deallocate storage for the cheat */
	free (CheatTable[cheat_num].name);
	free (CheatTable[cheat_num].comment);
	free (CheatTable[cheat_num].subcheat);

	/* If it's active, decrease the count */
	if (CheatTable[cheat_num].flags & CHEAT_FLAG_ACTIVE)
		ActiveCheatTotal --;

	/* move all the elements after this one up one slot if there are more than 1 and it's not the last */
	if ((LoadedCheatTotal > 1) && (cheat_num < LoadedCheatTotal - 1))
		memmove (&CheatTable[cheat_num], &CheatTable[cheat_num+1], sizeof (struct cheat_struct) * (LoadedCheatTotal - (cheat_num + 1)));

	/* knock one off the total */
	LoadedCheatTotal --;
}

void subcheat_insert_new (int cheat_num, int subcheat_num)
{
	/* if the index is off the end of the list, fix it */
	if (subcheat_num > CheatTable[cheat_num].num_sub) subcheat_num = CheatTable[cheat_num].num_sub + 1;

	/* grow the subcheat table allocation */
	CheatTable[cheat_num].subcheat = realloc (CheatTable[cheat_num].subcheat, sizeof (struct subcheat_struct) * (CheatTable[cheat_num].num_sub + 2));
	if (CheatTable[cheat_num].subcheat == NULL) return;

	/* insert space in the middle of the table if needed */
	if ((subcheat_num < CheatTable[cheat_num].num_sub) || (subcheat_num == 0))
		memmove (&CheatTable[cheat_num].subcheat[subcheat_num+1], &CheatTable[cheat_num].subcheat[subcheat_num],
			sizeof (struct subcheat_struct) * (CheatTable[cheat_num].num_sub + 1 - subcheat_num));

	/* clear the new entry */
	memset (&CheatTable[cheat_num].subcheat[subcheat_num], 0, sizeof (struct subcheat_struct));

	/*add one to the total */
	CheatTable[cheat_num].num_sub ++;
}

void subcheat_delete (int cheat_num, int subcheat_num)
{
	if (CheatTable[cheat_num].num_sub < 1) return;
	/* if the index is off the end, make it the last one */
	if (subcheat_num > CheatTable[cheat_num].num_sub) subcheat_num = CheatTable[cheat_num].num_sub;

	/* remove the element in the middle if it's not the last */
	if (subcheat_num < CheatTable[cheat_num].num_sub)
		memmove (&CheatTable[cheat_num].subcheat[subcheat_num], &CheatTable[cheat_num].subcheat[subcheat_num+1],
			sizeof (struct subcheat_struct) * (CheatTable[cheat_num].num_sub - subcheat_num));

	/* shrink the subcheat table allocation */
	CheatTable[cheat_num].subcheat = realloc (CheatTable[cheat_num].subcheat, sizeof (struct subcheat_struct) * (CheatTable[cheat_num].num_sub));
	if (CheatTable[cheat_num].subcheat == NULL) return;

	/* knock one off the total */
	CheatTable[cheat_num].num_sub --;
}

/* Function to load the cheats for a game from a single database */
void LoadCheatFile (int merge, char *filename)
{
	void *f = osd_fopen (NULL, filename, OSD_FILETYPE_CHEAT, 0);
	char curline[2048];
	int name_length;
	struct subcheat_struct *subcheat;
	int sub = 0;

	if (!merge)
	{
		ActiveCheatTotal = 0;
		LoadedCheatTotal = 0;
	}

	if (!f) return;

	name_length = strlen (Machine->gamedrv->name);

	/* Load the cheats for that game */
	/* Ex.: pacman:0:4E14:06:000:1UP Unlimited lives:Coded on 1 byte */
	while ((osd_fgets (curline, 2048, f) != NULL) && (LoadedCheatTotal < MAX_LOADEDCHEATS))
	{
		char *ptr;
		int temp_cpu;
		offs_t temp_address;
		data_t temp_data;
		INT32 temp_code;


		/* Take a few easy-out cases to speed things up */
		if (curline[name_length] != ':') continue;
		if (strncmp (curline, Machine->gamedrv->name, name_length) != 0) continue;
		if (curline[0] == ';') continue;

#if 0
		if (sologame)
			if ((strstr(str, "2UP") != NULL) || (strstr(str, "PL2") != NULL) ||
				(strstr(str, "3UP") != NULL) || (strstr(str, "PL3") != NULL) ||
				(strstr(str, "4UP") != NULL) || (strstr(str, "PL4") != NULL) ||
				(strstr(str, "2up") != NULL) || (strstr(str, "pl2") != NULL) ||
				(strstr(str, "3up") != NULL) || (strstr(str, "pl3") != NULL) ||
				(strstr(str, "4up") != NULL) || (strstr(str, "pl4") != NULL))
		  continue;
#endif

		/* Extract the fields from the line */
		/* Skip the driver name */
		ptr = strtok(curline, ":");
		if (!ptr) continue;

		/* CPU number */
		ptr = strtok(NULL, ":");
		if (!ptr) continue;
		sscanf(ptr,"%d", &temp_cpu);
		/* skip if it's a sound cpu and the audio is off */
		if (CPU_AUDIO_OFF(temp_cpu)) continue;
		/* skip if this is a bogus CPU */
		if (temp_cpu >= cpu_gettotalcpu()) continue;

		/* Address */
		ptr = strtok(NULL, ":");
		if (!ptr) continue;
		sscanf(ptr,"%X", &temp_address);
		temp_address &= cpunum_address_mask(temp_cpu);

		/* data byte */
		ptr = strtok(NULL, ":");
		if (!ptr) continue;
		sscanf(ptr,"%x", &temp_data);
		temp_data &= 0xff;

		/* special code */
		ptr = strtok(NULL, ":");
		if (!ptr) continue;
		sscanf(ptr,"%d", &temp_code);

		/* Is this a subcheat? */
		if ((temp_code >= kCheatSpecial_LinkStart) &&
			(temp_code <= kCheatSpecial_LinkEnd))
		{
			sub ++;

			/* Adjust the special flag */
			temp_code -= kCheatSpecial_LinkStart;

			/* point to the last valid main cheat entry */
			LoadedCheatTotal --;
		}
		else
		{
			/* no, make this the first cheat in the series */
			sub = 0;
		}

		/* Allocate (or resize) storage for the subcheat array */
		CheatTable[LoadedCheatTotal].subcheat = realloc (CheatTable[LoadedCheatTotal].subcheat, sizeof (struct subcheat_struct) * (sub + 1));
		if (CheatTable[LoadedCheatTotal].subcheat == NULL) continue;

		/* Store the current number of subcheats embodied by this code */
		CheatTable[LoadedCheatTotal].num_sub = sub;

		subcheat = &CheatTable[LoadedCheatTotal].subcheat[sub];

		/* Reset the cheat */
		subcheat->frames_til_trigger = 0;
		subcheat->frame_count = 0;
		subcheat->backup = 0;
		subcheat->flags = 0;

		/* Copy the cheat data */
		subcheat->cpu = temp_cpu;
		subcheat->address = temp_address;
		subcheat->data = temp_data;
		subcheat->code = temp_code;

		cheat_set_code (subcheat, temp_code, LoadedCheatTotal);

		/* don't bother with the names & comments for subcheats */
		if (sub) goto next;

		/* Disable the cheat */
		CheatTable[LoadedCheatTotal].flags &= ~CHEAT_FLAG_ACTIVE;

		/* cheat name */
		CheatTable[LoadedCheatTotal].name = NULL;
		ptr = strtok(NULL, ":");
		if (!ptr) continue;

		/* Allocate storage and copy the name */
		CheatTable[LoadedCheatTotal].name = malloc (strlen (ptr) + 1);
		strcpy (CheatTable[LoadedCheatTotal].name,ptr);

		/* Strip line-ending if needed */
		if (strstr(CheatTable[LoadedCheatTotal].name,"\n") != NULL)
			CheatTable[LoadedCheatTotal].name[strlen(CheatTable[LoadedCheatTotal].name)-1] = 0;

		/* read the "comment" field if there */
		ptr = strtok(NULL, ":");
		if (ptr)
		{
			/* Allocate storage and copy the comment */
			CheatTable[LoadedCheatTotal].comment = malloc (strlen (ptr) + 1);
			strcpy(CheatTable[LoadedCheatTotal].comment,ptr);

			/* Strip line-ending if needed */
			if (strstr(CheatTable[LoadedCheatTotal].comment,"\n") != NULL)
				CheatTable[LoadedCheatTotal].comment[strlen(CheatTable[LoadedCheatTotal].comment)-1] = 0;
		}
		else
			CheatTable[LoadedCheatTotal].comment = NULL;

next:
		LoadedCheatTotal ++;
	}

	osd_fclose (f);
}

/* Function who loads the cheats for a game from many databases */
void LoadCheatFiles (void)
{
	char *ptr;
	char str[CHEAT_FILENAME_MAXLEN+1];
	char filename[CHEAT_FILENAME_MAXLEN+1];

	int pos1, pos2;

	ActiveCheatTotal = 0;
	LoadedCheatTotal = 0;

	/* start off with the default cheat file, cheat.dat */
	strcpy (str, cheatfile);
	ptr = strtok (str, ";");

	/* append any additional cheat files */
	strcpy (database, ptr);
	strcpy (str, cheatfile);
	str[strlen (str) + 1] = 0;
	pos1 = 0;
	while (str[pos1])
	{
		pos2 = pos1;
		while ((str[pos2]) && (str[pos2] != ';'))
			pos2++;
		if (pos1 != pos2)
		{
			memset (filename, '\0', sizeof(filename));
			strncpy (filename, &str[pos1], (pos2 - pos1));
			LoadCheatFile (1, filename);
			pos1 = pos2 + 1;
		}
	}
}

#if 0
void InitMemoryAreas(void)
{
	const struct MemoryWriteAddress *mwa = Machine->drv->cpu[Searchcpu].memory_write;
	char buffer[40];

	MemoryAreasSelected = 0;
	MemoryAreasTotal = 0;

	while (mwa->start != -1)
	{
		sprintf (buffer, FormatAddr(Searchcpu,0), mwa->start);
		strcpy (MemToScanTable[MemoryAreasTotal].Name, buffer);
		strcat (MemToScanTable[MemoryAreasTotal].Name," -> ");
		sprintf (buffer, FormatAddr(Searchcpu,0), mwa->end);
		strcat (MemToScanTable[MemoryAreasTotal].Name, buffer);
		MemToScanTable[MemoryAreasTotal].handler = mwa->handler;
		MemToScanTable[MemoryAreasTotal].Enabled = 0;
		MemoryAreasTotal++;
		mwa++;
	}
}
#endif

/* Init some variables */
void InitCheat(void)
{
	int i;

	he_did_cheat = 0;
	CheatEnabled = 1;

#if 0
  reset_table (StartRam);
  reset_table (BackupRam);
  reset_table (FlagTable);

  reset_table (OldBackupRam);
  reset_table (OldFlagTable);
#endif

	/* Reset the watchpoints to their defaults */
	is_watch_active = 0;
	is_watch_visible = 1;

	for (i = 0;i < MAX_WATCHES;i ++)
	{
		/* disable this watchpoint */
		watches[i].num_bytes = 0;

		watches[i].cpu = 0;
		watches[i].label[0] = 0x00;
		watches[i].label_type = 0;
		watches[i].address = 0;

		/* set the screen position */
		watches[i].x = 0;
		watches[i].y = i * Machine->uifontheight;
	}

	LoadCheatFiles ();
/*	InitMemoryAreas(); */
}

/* copy one cheat structure to another */
void set_cheat(struct cheat_struct *dest, struct cheat_struct *src)
{
// TODO: fix!
#if 0
	/* Changed order to match structure - Added field More */
	struct cheat_struct new_cheat =
	{
		0,				/* cpu */
		0,				/* Address */
		0,				/* Data */
		0,				/* Special */
		0,				/* Count */
		0,				/* Backup */
		0,				/* Minimum */
		0xFF,				/* Maximum */
		"---- New Cheat ----",	/* Name */
		"",				/* More */
		0,				/* active */
	};

	if (src == NEW_CHEAT)
	{
		src = &new_cheat;
	}

	dest->cpu = src->cpu;
	dest->Address	= src->Address;
	dest->Data		= src->Data;
	dest->Special	= src->Special;
	dest->Count = src->Count;
	dest->Backup	= src->Backup;
	dest->Minimum	= src->Minimum;
	dest->Maximum	= src->Maximum;
	dest->active = src->active;
	strcpy(dest->Name, src->Name);
	strcpy(dest->comment, src->comment);
#endif
}

void DeleteLoadedCheatFromTable(int NoCheat)
{
  int i;
  if ((NoCheat > LoadedCheatTotal) || (NoCheat < 0))
	return;
  for (i = NoCheat; i < LoadedCheatTotal-1;i ++)
  {
	set_cheat(&CheatTable[i], &CheatTable[i + 1]);
  }
  LoadedCheatTotal --;
}

int EditCheatHeader(void)
{
#if 0
  int i = 0;
  char *paDisplayText[] = {
		"To edit a Cheat name, press",
		"<ENTER> when name is selected.",
		"To edit values or to select a",
		"pre-defined Cheat Name, use :",
		"<+> and Right arrow key: +1",
		"<-> and Left  arrow key: -1",
		"<1> ... <8>: +1 digit",
		"",
		"<F10>: Show Help + other keys",
		0 };

  struct DisplayText dt[20];

  while (paDisplayText[i])
  {
	if (i)
		dt[i].y = (dt[i - 1].y + FontHeight + 2);
	else
		dt[i].y = FIRSTPOS;
	dt[i].color = UI_COLOR_WHITE;
	dt[i].text = paDisplayText[i];
	dt[i].x = (MachWidth - FontWidth * strlen(dt[i].text)) / 2;
	if(dt[i].x > MachWidth)
		dt[i].x = 0;
	i++;
  }
  dt[i].text = 0; /* terminate array */
  displaytext(dt,0,1);
  return(dt[i-1].y + ( 3 * FontHeight ));
#else
	return 0;
#endif
}

void EditCheat(int CheatNo)
{
#if 0
  char *CheatNameList[] = {
	"Infinite Lives PL1",
	"Infinite Lives PL2",
	"Infinite Time",
	"Infinite Time PL1",
	"Infinite Time PL2",
	"Invincibility",
	"Invincibility PL1",
	"Invincibility PL2",
	"Infinite Energy",
	"Infinite Energy PL1",
	"Infinite Energy PL2",
	"Select Next Level",
	"Select Current level",
	"Infinite Ammo",
	"Infinite Ammo PL1",
	"Infinite Ammo PL2",
	"Infinite Bombs",
	"Infinite Bombs PL1",
	"Infinite Bombs PL2",
	"Select Score PL1",
	"Select Score PL2",
	"Drain all Energy Now! PL1",
	"Drain all Energy Now! PL2",
	"Infinite",
	"Always have",
	"Get",
	"Lose",
	"[                           ]",
	"---> <ENTER> To Edit <---",
	"\0" };

  int i,s,y,key,done;
  int total;
  struct DisplayText dt[20];
  char str2[6][40];
  int CurrentName;
  int EditYPos;

  char buffer[10];

  cheat_clearbitmap();

  y = EditCheatHeader();

  total = 0;

  if ((CheatTable[CheatNo].special>= kCheatSpecialUserFirst) && (CheatTable[CheatNo].special<=kCheatSpecialUserLast))
	CheatTable[CheatNo].data = CheatTable[CheatNo].Maximum;

  sprintf(str2[0],"Name: %s",CheatTable[CheatNo].Name);
  if ((FontWidth * (int)strlen(str2[0])) > MachWidth - Machine->uixmin)
	sprintf(str2[0],"%s",CheatTable[CheatNo].Name);
  sprintf(str2[1],"CPU:        %01X",CheatTable[CheatNo].cpu);

  sprintf(str2[2], FormatAddr(CheatTable[CheatNo].cpu,1),
				CheatTable[CheatNo].address);

  sprintf(str2[3],"Value:    %03d  (0x%02X)",CheatTable[CheatNo].data,CheatTable[CheatNo].data);
  sprintf(str2[4],"Type:     %03d",CheatTable[CheatNo].special);

  sprintf(str2[5],"comment: %s",CheatTable[CheatNo].comment);
  if ((FontWidth * (int)strlen(str2[5])) > MachWidth - Machine->uixmin)
	sprintf(str2[5],"%s",CheatTable[CheatNo].comment);

  for (i = 0;i < 6;i ++)
  {
	dt[total].text = str2[i];
	dt[total].x = MachWidth / 2;
	if(MachWidth < 35*FontWidth)
		dt[total].x = 0;
	else
		dt[total].x -= 15*FontWidth;
	dt[total].y = y;
	dt[total].color = UI_COLOR_WHITE;
	total++;
	y += FontHeight;
  }

  dt[total].text = 0; /* terminate array */

  EditYPos = ( y + ( 4 * FontHeight ) );

  s = 0;
  CurrentName = -1;

  oldkey = 0;

  done = 0;
  do
  {

	for (i = 0;i < total;i++)
		dt[i].color = (i == s) ? UI_COLOR_YELLOW : UI_COLOR_WHITE;
	displaytext(dt,0,1);

	/* key = keyboard_read_sync(); */
	key = cheat_readkey();	  /* MSH 990217 */
	switch (key)
	{
		case KEYCODE_DOWN:
		case KEYCODE_2_PAD:
			if (s < total - 1)
				s++;
			else
				s = 0;
			break;

		case KEYCODE_UP:
		case KEYCODE_8_PAD:
			if (s > 0)
				s--;
			else
				s = total - 1;
			break;

		case KEYCODE_LEFT:
		case KEYCODE_4_PAD:
			switch (s)
			{
				case 0:    /* Name */

					if (CurrentName < 0)
						CurrentName = 0;

					if (CurrentName == 0)						/* wrap if necessary*/
						while (CheatNameList[CurrentName][0])
							CurrentName++;
					CurrentName--;
					strcpy (CheatTable[CheatNo].Name, CheatNameList[CurrentName]);
					sprintf (str2[0],"Name: %s", CheatTable[CheatNo].Name);
					ClearTextLine(1, dt[0].y);
					break;
				case 1:    /* cpu */
					if (ManyCpus)
					{
						if (CheatTable[CheatNo].cpu == 0)
							CheatTable[CheatNo].cpu = cpu_gettotalcpu() - 1;
						else
							CheatTable[CheatNo].cpu --;
						sprintf (str2[1], "CPU:        %01X", CheatTable[CheatNo].cpu);
					}
					break;
				case 2:    /* Address */
					if (CheatTable[CheatNo].address == 0)
						CheatTable[CheatNo].address = MAX_ADDRESS(CheatTable[CheatNo].cpu);
					else
						CheatTable[CheatNo].address --;
					sprintf(str2[2], FormatAddr(CheatTable[CheatNo].cpu,1),
						CheatTable[CheatNo].address);
					break;
				case 3:    /* Data */
					if (CheatTable[CheatNo].data == 0)
						CheatTable[CheatNo].data = 0xFF;
					else
						CheatTable[CheatNo].data --;
					sprintf(str2[3], "Value:    %03d  (0x%02X)", CheatTable[CheatNo].data,
						CheatTable[CheatNo].data);
					break;
				case 4:    /* Special */
					if (CheatTable[CheatNo].special <= 0)
						CheatTable[CheatNo].special = TOTAL_CHEAT_TYPES + OFFSET_LINK_CHEAT;
					else
					switch (CheatTable[CheatNo].special)
						{
							case 20:
								CheatTable[CheatNo].special = 11;
								break;
							case 40:
								CheatTable[CheatNo].special = 24;
								break;
							case kCheatSpecialUserFirst:
								CheatTable[CheatNo].special = 44;
								break;
							case 70:
								CheatTable[CheatNo].special = 65;
								break;
							case OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = kCheatSpecialUserLast;
								break;
							case 20 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 11 + OFFSET_LINK_CHEAT;
								break;
							case 40 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 24 + OFFSET_LINK_CHEAT;
								break;
							case kCheatSpecialUserFirst + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 44 + OFFSET_LINK_CHEAT;
								break;
							case 70 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 65 + OFFSET_LINK_CHEAT;
								break;
							default:
								CheatTable[CheatNo].special --;
								break;
						}

					sprintf(str2[4],"Type:     %03d",CheatTable[CheatNo].special);
					break;
			}
			break;

		case KEYCODE_RIGHT:
		case KEYCODE_6_PAD:
			switch (s)
			{
				case 0:    /* Name */
					CurrentName ++;
					if (CheatNameList[CurrentName][0] == 0)
						CurrentName = 0;
					strcpy (CheatTable[CheatNo].Name, CheatNameList[CurrentName]);
					sprintf (str2[0], "Name: %s", CheatTable[CheatNo].Name);
					ClearTextLine(1, dt[0].y);
					break;
				case 1:    /* cpu */
					if (ManyCpus)
					{
						CheatTable[CheatNo].cpu ++;
						if (CheatTable[CheatNo].cpu >= cpu_gettotalcpu())
							CheatTable[CheatNo].cpu = 0;
						sprintf(str2[1],"CPU:        %01X",CheatTable[CheatNo].cpu);
					}
					break;
				case 2:    /* Address */
					CheatTable[CheatNo].address ++;
					if (CheatTable[CheatNo].address > MAX_ADDRESS(CheatTable[CheatNo].cpu))
						CheatTable[CheatNo].address = 0;
					sprintf (str2[2], FormatAddr(CheatTable[CheatNo].cpu,1),
						CheatTable[CheatNo].address);
					break;
				case 3:    /* Data */
					if(CheatTable[CheatNo].data == 0xFF)
						CheatTable[CheatNo].data = 0;
					else
						CheatTable[CheatNo].data ++;
					sprintf(str2[3],"Value:    %03d  (0x%02X)",CheatTable[CheatNo].data,
						CheatTable[CheatNo].data);
					break;
				case 4:    /* Special */
					if (CheatTable[CheatNo].special >= TOTAL_CHEAT_TYPES + OFFSET_LINK_CHEAT)
						CheatTable[CheatNo].special = 0;
					else
					switch (CheatTable[CheatNo].special)
						{
							case 11:
								CheatTable[CheatNo].special = 20;
								break;
							case 24:
								CheatTable[CheatNo].special = 40;
								break;
							case 44:
								CheatTable[CheatNo].special = kCheatSpecialUserFirst;
								break;
							case 65:
								CheatTable[CheatNo].special = 70;
								break;
							case kCheatSpecialUserLast:
								CheatTable[CheatNo].special = OFFSET_LINK_CHEAT;
								break;
							case 11 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 20 + OFFSET_LINK_CHEAT;
								break;
							case 24 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 40 + OFFSET_LINK_CHEAT;
								break;
							case 44 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = kCheatSpecialUserFirst + OFFSET_LINK_CHEAT;
								break;
							case 65 + OFFSET_LINK_CHEAT:
								CheatTable[CheatNo].special = 70 + OFFSET_LINK_CHEAT;
								break;
							default:
								CheatTable[CheatNo].special ++;
								break;
						}

					sprintf(str2[4],"Type:     %03d",CheatTable[CheatNo].special);
					break;
			}
			break;

		case KEYCODE_HOME:
		case KEYCODE_7_PAD:
			switch (s)
			{
				case 3: /* Data */
					if (CheatTable[CheatNo].data >= 0x80)
						CheatTable[CheatNo].data -= 0x80;
					sprintf(str2[3], "Value:    %03d  (0x%02X)", CheatTable[CheatNo].data,
						CheatTable[CheatNo].data);
					break;
				case 4: /* Special */
					if (CheatTable[CheatNo].special >= OFFSET_LINK_CHEAT)
						CheatTable[CheatNo].special -= OFFSET_LINK_CHEAT;
					sprintf(str2[4],"Type:     %03d",CheatTable[CheatNo].special);
					break;
			}
			break;

		case KEYCODE_END:
		case KEYCODE_1_PAD:
			switch (s)
			{
				case 3: /* Data */
					if (CheatTable[CheatNo].data < 0x80)
						CheatTable[CheatNo].data += 0x80;
					sprintf(str2[3], "Value:    %03d  (0x%02X)", CheatTable[CheatNo].data,
						CheatTable[CheatNo].data);
					break;
				case 4: /* Special */
					if (CheatTable[CheatNo].special < OFFSET_LINK_CHEAT)
						CheatTable[CheatNo].special += OFFSET_LINK_CHEAT;
					sprintf(str2[4],"Type:     %03d",CheatTable[CheatNo].special);
					break;
			}
			break;

		case KEYCODE_8:
			if (ADDRESS_BITS(CheatTable[CheatNo].cpu) < 29) break;
		case KEYCODE_7:
			if (ADDRESS_BITS(CheatTable[CheatNo].cpu) < 25) break;
		case KEYCODE_6:
			if (ADDRESS_BITS(CheatTable[CheatNo].cpu) < 21) break;
		case KEYCODE_5:
			if (ADDRESS_BITS(CheatTable[CheatNo].cpu) < 17) break;
		case KEYCODE_4:
			if (ADDRESS_BITS(CheatTable[CheatNo].cpu) < 13) break;
		case KEYCODE_3:
		case KEYCODE_2:
		case KEYCODE_1:
			/* these keys only apply to the address line */
			if (s == 2)
			{
				int addr = CheatTable[CheatNo].address;	/* copy address*/
				int digit = (KEYCODE_8 - key);	/* if key is KEYCODE_8, digit = 0 */
				int mask;

				/* adjust digit based on cpu address range */
				digit -= (8 - (ADDRESS_BITS(CheatTable[CheatNo].cpu)+3) / 4);

				mask = 0xF << (digit * 4);	/* if digit is 1, mask = 0xf0 */

				do
				{
				if ((addr & mask) == mask)
					/* wrap hex digit around to 0 if necessary */
					addr &= ~mask;
				else
					/* otherwise bump hex digit by 1 */
					addr += (0x1 << (digit * 4));
				} while (addr > MAX_ADDRESS(CheatTable[CheatNo].cpu));

				CheatTable[CheatNo].address = addr;
				sprintf(str2[2], FormatAddr(CheatTable[CheatNo].cpu,1),
					CheatTable[CheatNo].address);
			}
			break;

		case KEYCODE_F3:
			while (keyboard_pressed(key)) {}
		  oldkey = 0;
			switch (s)
			{
				case 2:    /* Address */
					for (i = 0; i < total; i++)
						dt[i].color = UI_COLOR_WHITE;
					displaytext (dt, 0,1);
					xprintf (0, 0, EditYPos-2*FontHeight, "Edit Cheat Address:");
					sprintf(buffer, FormatAddr(CheatTable[CheatNo].cpu,0),
						CheatTable[CheatNo].address);
				xedit(0, EditYPos, buffer, strlen(buffer), 1);
					sscanf(buffer,"%X", &CheatTable[CheatNo].address);
					if (CheatTable[CheatNo].address > MAX_ADDRESS(CheatTable[CheatNo].cpu))
						CheatTable[CheatNo].address = MAX_ADDRESS(CheatTable[CheatNo].cpu);
					sprintf(str2[2], FormatAddr(CheatTable[CheatNo].cpu,1),
						CheatTable[CheatNo].address);
					cheat_clearbitmap();
					y = EditCheatHeader();
				  break;
			}
			break;

		case KEYCODE_F10:
			while (keyboard_pressed(key)) {}
		  oldkey = 0;
			EditCheatHelp();
			y = EditCheatHeader();
			break;

		case KEYCODE_ENTER:
		case KEYCODE_ENTER_PAD:
			while (keyboard_pressed(key)) {}
		  oldkey = 0;
			switch (s)
			{
				case 0:    /* Name */
					for (i = 0; i < total; i++)
						dt[i].color = UI_COLOR_WHITE;
					displaytext (dt, 0,1);
					xprintf (0, 0, EditYPos-2*FontHeight, "Edit Cheat Description:");
				xedit(0, EditYPos, CheatTable[CheatNo].Name, CHEAT_NAME_MAXLEN, 0);
					sprintf (str2[0], "Name: %s", CheatTable[CheatNo].Name);
					if ((FontWidth * (int)strlen(str2[0])) > MachWidth - Machine->uixmin)
						sprintf(str2[0],"%s",CheatTable[CheatNo].Name);
					cheat_clearbitmap();
					y = EditCheatHeader();
				  break;
				case 5:    /* Comment */
					for (i = 0; i < total; i++)
						dt[i].color = UI_COLOR_WHITE;
					displaytext (dt, 0,1);
					xprintf (0, 0, EditYPos-2*FontHeight, "Edit Cheat Comment Description:");
				xedit(0, EditYPos, CheatTable[CheatNo].comment, CHEAT_NAME_MAXLEN, 0);
					sprintf (str2[5], "Comment: %s", CheatTable[CheatNo].comment);
					if ((FontWidth * (int)strlen(str2[5])) > MachWidth - Machine->uixmin)
						sprintf(str2[5],"%s",CheatTable[CheatNo].comment);
					cheat_clearbitmap();
					y = EditCheatHeader();
				  break;
			}
			break;

		case KEYCODE_ESC:
		case KEYCODE_TAB:
			while (keyboard_pressed(key)) {}
			done = 1;
			break;
	}
  } while (done == 0);

  while (keyboard_pressed(key)) {}

  if (	(CheatTable[CheatNo].special==62) || (CheatTable[CheatNo].special==65)	||
	(CheatTable[CheatNo].special==72) || (CheatTable[CheatNo].special==kCheatSpecialUserLast)	)
	CheatTable[CheatNo].Minimum = 1;
  else
	CheatTable[CheatNo].Minimum = 0;
  if ((CheatTable[CheatNo].special>= kCheatSpecialUserFirst) && (CheatTable[CheatNo].special<=kCheatSpecialUserLast))
  {
	CheatTable[CheatNo].Maximum = CheatTable[CheatNo].data;
	CheatTable[CheatNo].data = 0;
  }
  else
	CheatTable[CheatNo].Maximum = 0xFF;

  cheat_clearbitmap();
#endif
}


INT32 EnableDisableCheatMenu (INT32 selected)
{
	int sel;
	static INT8 submenu_choice;
	const char *menu_item[MAX_LOADEDCHEATS + 2];
	const char *menu_subitem[MAX_LOADEDCHEATS];
	char buf[MAX_LOADEDCHEATS][80];
	char buf2[MAX_LOADEDCHEATS][10];
	static int tag[MAX_LOADEDCHEATS];
	int i, total = 0;


	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		submenu_choice = CommentMenu (submenu_choice, tag[sel]);
		if (submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	for (i = 0; i < LoadedCheatTotal; i ++)
	{
		int string_num;

		if (CheatTable[i].comment && (CheatTable[i].comment[0] != 0x00))
		{
			sprintf (buf[total], "%s (%s...)", CheatTable[i].name, ui_getstring (UI_moreinfo));
		}
		else
			sprintf (buf[total], "%s", CheatTable[i].name);

		tag[total] = i;
		menu_item[total] = buf[total];

		/* add submenu options for all cheats that are not comments */
		if ((CheatTable[i].flags & CHEAT_FLAG_COMMENT) == 0)
		{
			if (CheatTable[i].flags & CHEAT_FLAG_ACTIVE)
				string_num = UI_on;
			else
				string_num = UI_off;
			sprintf (buf2[total], "%s", ui_getstring (string_num));
			menu_subitem[total] = buf2[total];
		}
		else
			menu_subitem[total] = NULL;
		total ++;
	}

	menu_item[total] = ui_getstring (UI_returntoprior);
	menu_subitem[total++] = NULL;
	menu_item[total] = 0;	/* terminate array */

	ui_displaymenu(menu_item,menu_subitem,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if ((input_ui_pressed_repeat(IPT_UI_LEFT,8)) || (input_ui_pressed_repeat(IPT_UI_RIGHT,8)))
	{
		if ((CheatTable[tag[sel]].flags & CHEAT_FLAG_COMMENT) == 0)
		{
			int active = CheatTable[tag[sel]].flags & CHEAT_FLAG_ACTIVE;

			active ^= 0x01;

			cheat_set_status (tag[sel], active);
			CheatEnabled = 1;
		}
	}


	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == (total - 1))
		{
			/* return to prior menu */
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			if (CheatTable[tag[sel]].comment && (CheatTable[tag[sel]].comment[0] != 0x00))
			{
				submenu_choice = 1;
				/* tell updatescreen() to clean after us */
				need_to_clear_bitmap = 1;
			}
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

static INT32 CommentMenu (INT32 selected, int cheat_index)
{
	char buf[2048];
	char buf2[256];
	int sel;


	sel = selected - 1;

	buf[0] = 0;

	if (CheatTable[cheat_index].comment[0] == 0x00)
	{
		sel = -1;
		buf[0] = 0;
	}
	else
	{
		sprintf (buf2, "\t%s\n\t%s\n\n", ui_getstring (UI_moreinfoheader), CheatTable[cheat_index].name);
		strcpy (buf, buf2);
		strcat (buf, CheatTable[cheat_index].comment);
	}

	/* menu system, use the normal menu keys */
	strcat(buf,"\n\n\t");
	strcat(buf,ui_getstring (UI_lefthilight));
	strcat(buf," ");
	strcat(buf,ui_getstring (UI_returntoprior));
	strcat(buf," ");
	strcat(buf,ui_getstring (UI_righthilight));

	ui_displaymessagewindow(buf);

	if (input_ui_pressed(IPT_UI_SELECT))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

INT32 AddEditCheatMenu (INT32 selected)
{
	int sel;
	static INT8 submenu_choice;
	const char *menu_item[MAX_LOADEDCHEATS + 4];
//	char buf[MAX_LOADEDCHEATS][80];
//	char buf2[MAX_LOADEDCHEATS][10];
	int tag[MAX_LOADEDCHEATS];
	int i, total = 0;


	sel = selected - 1;

	/* Set up the "tag" table so we know which cheats belong in the menu */
	for (i = 0; i < LoadedCheatTotal; i ++)
	{
		/* add menu listings for all cheats that are not comments */
		if ((CheatTable[i].flags & CHEAT_FLAG_COMMENT) == 0)
		{
			tag[total] = i;
			menu_item[total++] = CheatTable[i].name;
		}
	}

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		submenu_choice = EditCheatMenu (submenu_choice, tag[sel]);
		if (submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	menu_item[total++] = ui_getstring (UI_returntoprior);
	menu_item[total] = NULL; /* TODO: add help string */
	menu_item[total+1] = 0;	/* terminate array */

	ui_displaymenu(menu_item,0,0,sel,0);

	if (code_pressed_memory_repeat (KEYCODE_INSERT, 8))
	{
		/* add a new cheat at the current position (or the end) */
		if (sel < total - 1)
			cheat_insert_new (tag[sel]);
		else
			cheat_insert_new (LoadedCheatTotal);
	}

	if (code_pressed_memory_repeat (KEYCODE_DEL, 8))
	{
		if (LoadedCheatTotal)
		{
			/* delete the selected cheat (or the last one) */
			if (sel < total - 1)
				cheat_delete (tag[sel]);
			else
			{
				cheat_delete (LoadedCheatTotal - 1);
				sel = total - 2;
			}
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == (total - 1))
		{
			/* return to prior menu */
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

static INT32 EditCheatMenu (INT32 selected, UINT8 cheat_num)
{
	int sel;
	int total, total2;
	struct subcheat_struct *subcheat;
	static INT8 submenu_choice;
	static UINT8 textedit_active;
	const char *menu_item[40];
	const char *menu_subitem[40];
	char setting[40][30];
	char flag[40];
	int arrowize;
	int subcheat_num;
	int i;


	sel = selected - 1;

	total = 0;
	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring (UI_cheatname);
	menu_item[total++] = ui_getstring (UI_cheatdescription);
	for (i = 0; i <= CheatTable[cheat_num].num_sub; i ++)
	{
		menu_item[total++] = ui_getstring (UI_cpu);
		menu_item[total++] = ui_getstring (UI_address);
		menu_item[total++] = ui_getstring (UI_value);
		menu_item[total++] = ui_getstring (UI_code);
	}
	menu_item[total++] = ui_getstring (UI_returntoprior);
	menu_item[total] = 0;

	arrowize = 0;

	/* set up the submenu selections */
	total2 = 0;
	for (i = 0; i < 40; i ++)
		flag[i] = 0;

	/* if we're editing the label, make it inverse */
	if (textedit_active)
		flag[sel] = 1;

	/* name */
	if (CheatTable[cheat_num].name != 0x00)
		sprintf (setting[total2], "%s", CheatTable[cheat_num].name);
	else
		strcpy (setting[total2], ui_getstring (UI_none));
	menu_subitem[total2] = setting[total2]; total2++;

	/* comment */
	if (CheatTable[cheat_num].comment && CheatTable[cheat_num].comment != 0x00)
		sprintf (setting[total2], "%s...", ui_getstring (UI_moreinfo));
	else
		strcpy (setting[total2], ui_getstring (UI_none));
	menu_subitem[total2] = setting[total2]; total2++;

	/* Subcheats */
	for (i = 0; i <= CheatTable[cheat_num].num_sub; i ++)
	{
		subcheat = &CheatTable[cheat_num].subcheat[i];

		/* cpu number */
		sprintf (setting[total2], "%d", subcheat->cpu);
		menu_subitem[total2] = setting[total2]; total2++;

		/* address */
		if (cpunum_address_bits(subcheat->cpu) <= 16)
		{
			sprintf (setting[total2], "%04X", subcheat->address);
		}
		else
		{
			sprintf (setting[total2], "%08X", subcheat->address);
		}
		menu_subitem[total2] = setting[total2]; total2++;

		/* value */
		sprintf (setting[total2], "%d", subcheat->data);
		menu_subitem[total2] = setting[total2]; total2++;

		/* code */
		sprintf (setting[total2], "%d", subcheat->code);
		menu_subitem[total2] = setting[total2]; total2++;

		menu_subitem[total2] = NULL;
	}

	ui_displaymenu(menu_item,menu_subitem,flag,sel,arrowize);

	if (code_pressed_memory_repeat (KEYCODE_INSERT, 8))
	{
		if ((sel >= 2) && (sel <= ((CheatTable[cheat_num].num_sub + 1) * 4) + 1))
		{
			subcheat_num = (sel - 2) % 4;
		}
		else
		{
			subcheat_num = CheatTable[cheat_num].num_sub + 1;
		}

		/* add a new subcheat at the current position (or the end) */
		subcheat_insert_new (cheat_num, subcheat_num);
	}

	if (code_pressed_memory_repeat (KEYCODE_DEL, 8))
	{
		if ((sel >= 2) && (sel <= ((CheatTable[cheat_num].num_sub + 1) * 4) + 1))
		{
			subcheat_num = (sel - 2) % 4;
		}
		else
		{
			subcheat_num = CheatTable[cheat_num].num_sub;
		}

		if (CheatTable[cheat_num].num_sub != 0)
			subcheat_delete (cheat_num, subcheat_num);
	}

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
	{
		textedit_active = 0;
		sel = (sel + 1) % total;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		textedit_active = 0;
		sel = (sel + total - 1) % total;
	}

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if ((sel >= 2) && (sel <= ((CheatTable[cheat_num].num_sub + 1) * 4) + 1))
		{
			int newsel;

			subcheat = &CheatTable[cheat_num].subcheat[(sel - 2) / 4];
			newsel = (sel - 2) % 4;

			switch (newsel)
			{
				case 0: /* CPU */
					subcheat->cpu --;
					/* skip audio CPUs when the sound is off */
					if (CPU_AUDIO_OFF(subcheat->cpu))
						subcheat->cpu --;
					if (subcheat->cpu < 0)
						subcheat->cpu = cpu_gettotalcpu() - 1;
					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;
				case 1: /* address */
					textedit_active = 0;
					subcheat->address --;
					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;
				case 2: /* value */
					textedit_active = 0;
					subcheat->data --;
					subcheat->data &= 0xff;
					break;
				case 3: /* code */
					textedit_active = 0;
					subcheat->code --;
					break;
			}
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if ((sel >= 2) && (sel <= ((CheatTable[cheat_num].num_sub+1) * 4) + 1))
		{
			int newsel;

			subcheat = &CheatTable[cheat_num].subcheat[(sel - 2) / 4];
			newsel = (sel - 2) % 4;

			switch (newsel)
			{
				case 0: /* CPU */
					subcheat->cpu ++;
					/* skip audio CPUs when the sound is off */
					if (CPU_AUDIO_OFF(subcheat->cpu))
						subcheat->cpu ++;
					if (subcheat->cpu >= cpu_gettotalcpu())
						subcheat->cpu = 0;
					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;
				case 1: /* address */
					textedit_active = 0;
					subcheat->address ++;
					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;
				case 2: /* value */
					textedit_active = 0;
					subcheat->data ++;
					subcheat->data &= 0xff;
					break;
				case 3: /* code */
					textedit_active = 0;
					subcheat->code ++;
					break;
			}
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == ((CheatTable[cheat_num].num_sub+1) * 4) + 2)
		{
			/* return to main menu */
			submenu_choice = 0;
			sel = -1;
		}
		else if (/*(sel == 4) ||*/ (sel == 0))
		{
			/* wait for key up */
			while (input_ui_pressed(IPT_UI_SELECT)) {};

			/* flush the text buffer */
			osd_readkey_unicode (1);
			textedit_active ^= 1;
		}
		else
		{
			submenu_choice = 1;
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		textedit_active = 0;
		/* flush the text buffer */
		osd_readkey_unicode (1);
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	/* After we've weeded out any control characters, look for text */
	if (textedit_active)
	{
		int code;

#if 0
		/* is this the address field? */
		if (sel == 1)
		{
			INT8 hex_val;

			/* see if a hex digit was typed */
			hex_val = code_read_hex_async();
			if (hex_val != -1)
			{
				/* shift over one digit, add in the new value and clip */
				subcheat->address <<= 4;
				subcheat->address |= hex_val;
				subcheat->address &= cpunum_address_mask(subcheat->cpu);
			}
		}
		else
#endif
		if (CheatTable[cheat_num].name)
		{
			int length = strlen(CheatTable[cheat_num].name);

			code = osd_readkey_unicode(0) & 0xff; /* no 16-bit support */

			if (code)
			{
				if (code == 0x08) /* backspace */
				{
					/* clear the buffer */
					CheatTable[cheat_num].name[0] = 0x00;
				}
				else
				{
					/* append the character */
					CheatTable[cheat_num].name = realloc (CheatTable[cheat_num].name, length + 2);
					if (CheatTable[cheat_num].name != NULL)
					{
						CheatTable[cheat_num].name[length] = code;
						CheatTable[cheat_num].name[length+1] = 0x00;
					}
				}
			}
		}
	}

	return sel + 1;
}


#ifdef macintosh
#pragma mark -
#endif

INT32 StartSearch (INT32 selected)
{
	return -1;
}

INT32 ContinueSearch (INT32 selected, int ViewLast)
{
	return -1;
}

INT32 RestoreSearch (INT32 selected)
{
	return -1;
}

#ifdef macintosh
#pragma mark -
#endif

int FindFreeWatch (void)
{
	int i;
	for (i = 0; i < MAX_WATCHES; i ++)
	{
		if (watches[i].num_bytes == 0)
			return i;
	}

	/* indicate no free watch found */
	return -1;
}

void DisplayWatches (void)
{
	int i;
	char buf[256];

	if ((!is_watch_active) || (!is_watch_visible)) return;

	for (i = 0; i < MAX_WATCHES; i++)
	{
		/* Is this watchpoint active? */
		if (watches[i].num_bytes != 0)
		{
			char buf2[80];

			/* Display the first byte */
			sprintf (buf, "%02x", computer_readmem_byte (watches[i].cpu, watches[i].address));

			/* If this is for more than one byte, display the rest */
			if (watches[i].num_bytes > 1)
			{
				int j;

				for (j = 1; j < watches[i].num_bytes; j ++)
				{
					sprintf (buf2, " %02x", computer_readmem_byte (watches[i].cpu, watches[i].address + j));
					strcat (buf, buf2);
				}
			}

			/* Handle any labels */
			switch (watches[i].label_type)
			{
				case 0:
				default:
					break;
				case 1:
					if (cpunum_address_bits(watches[i].cpu) <= 16)
					{
						sprintf (buf2, " (%04x)", watches[i].address);
						strcat (buf, buf2);
					}
					else
					{
						sprintf (buf2, " (%08x)", watches[i].address);
						strcat (buf, buf2);
					}
					break;
				case 2:
					{
						sprintf (buf2, " (%s)", watches[i].label);
						strcat (buf, buf2);
					}
					break;
			}

			ui_text (buf, watches[i].x, watches[i].y);
		}
	}
}

INT32 ConfigureWatch (INT32 selected, UINT8 watchnum)
{
#ifdef NUM_ENTRIES
#undef NUM_ENTRIES
#endif
#define NUM_ENTRIES 9

	int sel;
	int total, total2;
	static INT8 submenu_choice;
	static UINT8 textedit_active;
	const char *menu_item[NUM_ENTRIES];
	const char *menu_subitem[NUM_ENTRIES];
	char setting[NUM_ENTRIES][30];
	char flag[NUM_ENTRIES];
	int arrowize;
	int i;


	sel = selected - 1;

	total = 0;
	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring (UI_cpu);
	menu_item[total++] = ui_getstring (UI_address);
	menu_item[total++] = ui_getstring (UI_watchlength);
	menu_item[total++] = ui_getstring (UI_watchlabeltype);
	menu_item[total++] = ui_getstring (UI_watchlabel);
	menu_item[total++] = ui_getstring (UI_watchx);
	menu_item[total++] = ui_getstring (UI_watchy);
	menu_item[total++] = ui_getstring (UI_returntoprior);
	menu_item[total] = 0;

	arrowize = 0;

	/* set up the submenu selections */
	total2 = 0;
	for (i = 0; i < NUM_ENTRIES; i ++)
		flag[i] = 0;

	/* if we're editing the label, make it inverse */
	if (textedit_active)
		flag[sel] = 1;

	/* cpu number */
	sprintf (setting[total2], "%d", watches[watchnum].cpu);
	menu_subitem[total2] = setting[total2]; total2++;

	/* address */
	if (cpunum_address_bits(watches[watchnum].cpu) <= 16)
	{
		sprintf (setting[total2], "%04x", watches[watchnum].address);
	}
	else
	{
		sprintf (setting[total2], "%08x", watches[watchnum].address);
	}
	menu_subitem[total2] = setting[total2]; total2++;

	/* length */
	sprintf (setting[total2], "%d", watches[watchnum].num_bytes);
	menu_subitem[total2] = setting[total2]; total2++;

	/* label type */
	switch (watches[watchnum].label_type)
	{
		case 0:
			strcpy (setting[total2], ui_getstring (UI_none));
			break;
		case 1:
			strcpy (setting[total2], ui_getstring (UI_address));
			break;
		case 2:
			strcpy (setting[total2], ui_getstring (UI_text));
			break;
	}
	menu_subitem[total2] = setting[total2]; total2++;

	/* label */
	if (watches[watchnum].label[0] != 0x00)
		sprintf (setting[total2], "%s", watches[watchnum].label);
	else
		strcpy (setting[total2], ui_getstring (UI_none));
	menu_subitem[total2] = setting[total2]; total2++;

	/* x */
	sprintf (setting[total2], "%d", watches[watchnum].x);
	menu_subitem[total2] = setting[total2]; total2++;

	/* y */
	sprintf (setting[total2], "%d", watches[watchnum].y);
	menu_subitem[total2] = setting[total2]; total2++;

	menu_subitem[total2] = NULL;

	ui_displaymenu(menu_item,menu_subitem,flag,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
	{
		textedit_active = 0;
		sel = (sel + 1) % total;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		textedit_active = 0;
		sel = (sel + total - 1) % total;
	}

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		switch (sel)
		{
			case 0: /* CPU */
				watches[watchnum].cpu --;
				/* skip audio CPUs when the sound is off */
				if (CPU_AUDIO_OFF(watches[watchnum].cpu))
					watches[watchnum].cpu --;
				if (watches[watchnum].cpu < 0)
					watches[watchnum].cpu = cpu_gettotalcpu() - 1;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;
			case 1: /* address */
				textedit_active = 0;
				watches[watchnum].address --;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;
			case 2: /* number of bytes */
				watches[watchnum].num_bytes --;
				if (watches[watchnum].num_bytes == (UINT8) -1)
					watches[watchnum].num_bytes = 16;
				break;
			case 3: /* label type */
				watches[watchnum].label_type --;
				if (watches[watchnum].label_type == (UINT8) -1)
					watches[watchnum].label_type = 2;
				break;
			case 4: /* label string */
				textedit_active = 0;
				break;
			case 5: /* x */
				watches[watchnum].x --;
				if (watches[watchnum].x == (UINT16) -1)
					watches[watchnum].x = Machine->uiwidth - 1;
				break;
			case 6: /* y */
				watches[watchnum].y --;
				if (watches[watchnum].y == (UINT16) -1)
					watches[watchnum].y = Machine->uiheight - 1;
				break;
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		switch (sel)
		{
			case 0:
				watches[watchnum].cpu ++;
				/* skip audio CPUs when the sound is off */
				if (CPU_AUDIO_OFF(watches[watchnum].cpu))
					watches[watchnum].cpu ++;
				if (watches[watchnum].cpu >= cpu_gettotalcpu())
					watches[watchnum].cpu = 0;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;
			case 1:
				textedit_active = 0;
				watches[watchnum].address ++;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;
			case 2:
				watches[watchnum].num_bytes ++;
				if (watches[watchnum].num_bytes > 16)
					watches[watchnum].num_bytes = 0;
				break;
			case 3:
				watches[watchnum].label_type ++;
				if (watches[watchnum].label_type > 2)
					watches[watchnum].label_type = 0;
				break;
			case 4:
				textedit_active = 0;
				break;
			case 5:
				watches[watchnum].x ++;
				if (watches[watchnum].x >= Machine->uiwidth)
					watches[watchnum].x = 0;
				break;
			case 6:
				watches[watchnum].y ++;
				if (watches[watchnum].y >= Machine->uiheight)
					watches[watchnum].y = 0;
				break;
		}
	}

	/* see if any watchpoints are active and set the flag if so */
	is_watch_active = 0;
	for (i = 0; i < MAX_WATCHES; i ++)
	{
		if (watches[i].num_bytes != 0)
		{
			is_watch_active = 1;
			break;
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == 7)
		{
			/* return to main menu */
			submenu_choice = 0;
			sel = -1;
		}
		else if ((sel == 4) || (sel == 1))
		{
			/* wait for key up */
			while (input_ui_pressed(IPT_UI_SELECT)) {};

			/* flush the text buffer */
			osd_readkey_unicode (1);
			textedit_active ^= 1;
		}
		else
		{
			submenu_choice = 1;
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		textedit_active = 0;
		/* flush the text buffer */
		osd_readkey_unicode (1);
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	/* After we've weeded out any control characters, look for text */
	if (textedit_active)
	{
		int code;

		/* is this the address field? */
		if (sel == 1)
		{
			INT8 hex_val;

			/* see if a hex digit was typed */
			hex_val = code_read_hex_async();
			if (hex_val != -1)
			{
				/* shift over one digit, add in the new value and clip */
				watches[watchnum].address <<= 4;
				watches[watchnum].address |= hex_val;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
			}
		}
		else
		{
			int length = strlen(watches[watchnum].label);

			if (length < 254)
			{
				code = osd_readkey_unicode(0) & 0xff; /* no 16-bit support */

				if (code)
				{
					if (code == 0x08) /* backspace */
					{
						/* clear the buffer */
						watches[watchnum].label[0] = 0x00;
					}
					else
					{
						/* append the character */
						watches[watchnum].label[length] = code;
						watches[watchnum].label[length+1] = 0x00;
					}
				}
			}
		}
	}

	return sel + 1;
}

INT32 ChooseWatch (INT32 selected)
{
	int sel;
	static INT8 submenu_choice;
	const char *menu_item[MAX_WATCHES + 2];
	char buf[MAX_WATCHES][80];
	const char *watchpoint_str = ui_getstring (UI_watchpoint);
	const char *disabled_str = ui_getstring (UI_disabled);
	int i, total = 0;


	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		submenu_choice = ConfigureWatch (submenu_choice, sel);

		if (submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	for (i = 0; i < MAX_WATCHES; i ++)
	{
		sprintf (buf[i], "%s %d: ", watchpoint_str, i);
		/* If the watchpoint is active (1 or more bytes long), show it */
		if (watches[i].num_bytes)
		{
			char buf2[80];

			if (cpunum_address_bits(watches[i].cpu) <= 16)
			{
				sprintf (buf2, "%04x", watches[i].address);
				strcat (buf[i], buf2);
			}
			else
			{
				sprintf (buf2, "%08x", watches[i].address);
				strcat (buf[i], buf2);
			}
		}
		else
			strcat (buf[i], disabled_str);

		menu_item[total++] = buf[i];
	}

	menu_item[total++] = ui_getstring (UI_returntoprior);
	menu_item[total] = 0;	/* terminate array */

	ui_displaymenu(menu_item,0,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == MAX_WATCHES)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}


#ifdef macintosh
#pragma mark -
#endif

static INT32 DisplayHelpFile (INT32 selected)
{
	return -1;
}

#ifdef macintosh
#pragma mark -
#endif

INT32 cheat_menu(INT32 selected)
{
#ifdef MENU_return
#undef MENU_return
#endif
#define MENU_return 8

	const char *menu_item[10];
	INT32 sel;
	UINT8 total = 0;
	static INT8 submenu_choice;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		switch (sel)
		{
			case 0:
				submenu_choice = EnableDisableCheatMenu (submenu_choice);
				break;
			case 1:
				submenu_choice = AddEditCheatMenu (submenu_choice);
				break;
			case 2:
				submenu_choice = StartSearch (submenu_choice);
				break;
			case 3:
				submenu_choice = ContinueSearch (submenu_choice, 0);
				break;
			case 4:
				submenu_choice = ContinueSearch (submenu_choice, 1);
				break;
			case 5:
				submenu_choice = RestoreSearch (submenu_choice);
				break;
			case 6:
				submenu_choice = ChooseWatch (submenu_choice);
				break;
			case 7:
				submenu_choice = DisplayHelpFile (submenu_choice);
				break;
			case MENU_return:
				submenu_choice = 0;
				sel = -1;
				break;
		}

		if (submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring (UI_enablecheat);
	menu_item[total++] = ui_getstring (UI_addeditcheat);
	menu_item[total++] = ui_getstring (UI_startcheat);
	menu_item[total++] = ui_getstring (UI_continuesearch);
	menu_item[total++] = ui_getstring (UI_viewresults);
	menu_item[total++] = ui_getstring (UI_restoreresults);
	menu_item[total++] = ui_getstring (UI_memorywatch);
	menu_item[total++] = ui_getstring (UI_generalhelp);
	menu_item[total++] = ui_getstring (UI_returntomain);
	menu_item[total] = 0;

	ui_displaymenu(menu_item,0,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == MENU_return)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			/* tell updatescreen() to clean after us */
			need_to_clear_bitmap = 1;
		}
	}

	/* Cancel pops us up a menu level */
	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		need_to_clear_bitmap = 1;
	}

	return sel + 1;
}

/* Free allocated arrays */
void StopCheat(void)
{
	int i;

	for (i = 0; i < LoadedCheatTotal; i ++)
	{
		/* free storage for the strings */
		if (CheatTable[i].name)
		{
			free (CheatTable[i].name);
			CheatTable[i].name = NULL;
		}
		if (CheatTable[i].comment)
		{
			free (CheatTable[i].comment);
			CheatTable[i].comment = NULL;
		}
	}

#if 0
  reset_table (StartRam);
  reset_table (BackupRam);
  reset_table (FlagTable);

  reset_table (OldBackupRam);
  reset_table (OldFlagTable);
#endif
}

void DoCheat(void)
{
	DisplayWatches ();

	if ((CheatEnabled) && (ActiveCheatTotal))
	{
		int i, j;

		/* At least one cheat is active, handle them */
		for (i = 0; i < LoadedCheatTotal; i ++)
		{
			/* skip if this isn't an active cheat */
			if ((CheatTable[i].flags & CHEAT_FLAG_ACTIVE) == 0) continue;

			/* loop through all subcheats */
			for (j = 0; j <= CheatTable[i].num_sub; j ++)
			{
				struct subcheat_struct *subcheat = &CheatTable[i].subcheat[j];

				if (subcheat->flags & SUBCHEAT_FLAG_DONE) continue;

				/* most common case: 0 */
				if (subcheat->code == kCheatSpecial_Poke)
				{
					WRITE_CHEAT;
				}
				/* Check special function if cheat counter is ready */
				else if (subcheat->frame_count == 0)
				{
					switch (subcheat->code)
					{
						case 1:
							WRITE_CHEAT;
							subcheat->flags |= SUBCHEAT_FLAG_DONE;
							break;
						case 2:
						case 3:
						case 4:
							WRITE_CHEAT;
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 5,6,7 check if the value has changed, if yes, start a timer. */
						/* When the timer ends, change the location */
						case 5:
						case 6:
						case 7:
							if (subcheat->flags & SUBCHEAT_FLAG_TIMED)
							{
								WRITE_CHEAT;
								subcheat->flags &= ~SUBCHEAT_FLAG_TIMED;
							}
							else if (COMPARE_CHEAT)
							{
								subcheat->frame_count = subcheat->frames_til_trigger;
								subcheat->flags |= SUBCHEAT_FLAG_TIMED;
							}
							break;

						/* 8,9,10,11 do not change the location if the value change by X every frames
						  This is to try to not change the value of an energy bar
				 		  when a bonus is awarded to it at the end of a level
				 		  See Kung Fu Master */
						case 8:
						case 9:
						case 10:
						case 11:
							if (subcheat->flags & SUBCHEAT_FLAG_TIMED)
							{
								/* Check the value to see if it has increased over the original value by 1 or more */
								if (READ_CHEAT != subcheat->backup - (kCheatSpecial_Backup1 - subcheat->code + 1))
									WRITE_CHEAT;
								subcheat->flags &= ~SUBCHEAT_FLAG_TIMED;
							}
							else
							{
								subcheat->backup = READ_CHEAT;
								subcheat->frame_count = 1;
								subcheat->flags |= SUBCHEAT_FLAG_TIMED;
							}
							break;

						/* 20-24: set bits */
						case 20:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							break;
						case 21:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							subcheat->flags |= SUBCHEAT_FLAG_DONE;
							break;
						case 22:
						case 23:
						case 24:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 40-44: reset bits */
						case 40:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							break;
						case 41:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							subcheat->flags |= SUBCHEAT_FLAG_DONE;
							break;
						case 42:
						case 43:
						case 44:
							computer_writemem_byte (subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 60-65: user select, poke when changes */
						case 60: case 61: case 62: case 63: case 64: case 65:
							if (subcheat->flags & SUBCHEAT_FLAG_TIMED)
							{
								if (READ_CHEAT != subcheat->backup)
								{
									WRITE_CHEAT;
									subcheat->flags |= SUBCHEAT_FLAG_DONE;
								}
							}
							else
							{
								subcheat->backup = READ_CHEAT;
								subcheat->frame_count = 1;
								subcheat->flags |= SUBCHEAT_FLAG_TIMED;
							}
							break;

						/* 70-75: user select, poke once */
						case 70: case 71: case 72: case 73: case 74: case 75:
							WRITE_CHEAT;
							subcheat->flags |= SUBCHEAT_FLAG_DONE;
						break;
					}
				}
				else
				{
					subcheat->frame_count--;
				}
			}
		} /* end for */
	}

	/* IPT_UI_TOGGLE_CHEAT Enable/Disable the active cheats on the fly. Required for some cheats. */
	if (input_ui_pressed(IPT_UI_TOGGLE_CHEAT))
	{
		/* Hold down shift to toggle the watchpoints */
		if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		{
			is_watch_visible ^= 1;
			usrintf_showmessage("%s %s", ui_getstring (UI_watchpoints), (is_watch_visible ? ui_getstring (UI_on) : ui_getstring (UI_off)));
		}
		else if (ActiveCheatTotal)
		{
			CheatEnabled ^= 1;
			usrintf_showmessage("%s %s", ui_getstring (UI_cheats), (CheatEnabled ? ui_getstring (UI_on) : ui_getstring (UI_off)));
		}
	}

#if 0
  /* KEYCODE_END loads the "Continue Search" sub-menu of the cheat engine */
  if ( keyboard_pressed_memory( KEYCODE_END ) )
  {
	osd_sound_enable(0);
	osd_pause(1);
	ContinueSearch(0, 0);
	osd_pause(0);
	osd_sound_enable(1);
  }
#endif

}
