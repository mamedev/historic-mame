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
|*|   JB 980506:	New tables osd_key_chars[] and osd_key_caps[], rewrite
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
|*|
|*|  Modifications by JCK from The Ultimate Patchers
|*|
|*|	JCK 980917:	Possibility of "circular" values in search method 1 :
|*|			  - if you press OSD_KEY_LEFT or OSD_KEY_DOWN when value is 0,
|*|			    it turns to 0xFF
|*|			  - if you press OSD_KEY_RIGHT or OSD_KEY_UP when value is 0xFF,
|*|			    it turns to 0.
|*|			Added new #define (MAX_LOADEDCHEATS, MAX_ACTIVECHEATS, MAX_DISPLAYCHEATS,
|*|			  MAX_MATCHES and MAX_WATCHES) to improve readability and upgradibility
|*|			  (changes where it was needed)
|*|			Added the possibility of toggling the watches display
|*|			  ON (OSD_KEY_INSERT) and OFF (OSD_KEY_DEL) .
|*|			Possibility to work on another cheat file (CHEAT.DAT is just the default one)
|*|			  (have a look in confic.c)
|*|			Added new types of cheats : 20 to 24 and 40 to 44 (see below)
|*|
|*|	JCK 981006:	15 active cheats instead of 10
|*|			Possibility of searches in CPUs other than 0 :
|*|			  a question is asked to the user to choose a CPU (0 default) when he starts
|*|			  a new search and the game has more than one CPU
|*|			Possibility to copy a cheat code by pressing OSD_KEY_F4
|*|			Possibility to save all the cheat codes to disk by pressing OSD_KEY_F6
|*|			Possibility to remove all the cheat codes from the active list
|*|			  by pressing OSD_KEY_F7
|*|			In the edit cheat, on the data line :
|*|			  - OSD_KEY_HOME sets the value to 0
|*|			  - OSD_KEY_END sets the value to 0x80
|*|			Possibility to use to use OSD_KEY_HOME, OSD_KEY_END, OSD_KEY_PGDN and
|*|			  OSD_KEY_PGUP to select a cheat code in the list
|*|
|*|	JCK 981008:	Data is saved on 2 hex digits (%02X)
|*|			Possibility to view watches for CPUs other than 0
|*|			Free memory before the start of a new search
|*|
|*|	JCK 981009:	CPU is displayed with the watches
|*|			Possibility to change the CPU of a watch by pressing OSD_KEY_9 or OSD_KEY_0
|*|
|*|	JCK 981016:	Added a new help for CheatListHelp and CheatListHelpEmpty
|*|			Corrected a minor bug in SelectCheat for OSD_KEY_HOME and OSD_KEY_END
|*|			Possibility to rename the cheat filename is now by pressing OSD_KEY_F9
|*|			Possibility to reload the cheat database by pressing OSD_KEY_F8
|*|			Possibility to rename the cheat filename and reload the cheat database
|*|			  by pressing OSD_KEY_F5 (same as OSD_KEY_F9 + OSD_KEY_F8)
|*|			Moved while (osd_key_pressed(key)) after each case when there was the possibility
|*|			  to use OSD_KEY_F10 (this key wasn't removed from the buffer as well as the
|*|			  other functions keys). The modified functions are EditCheat and SelectCheat
|*|			Added while (osd_key_pressed(key)) in function ShowHelp
|*|			Possibility to invoke help on StartCheat by pressing OSD_KEY_F10
|*|			New functions :
|*|			  - int RenameCheatFile(void) : returns 1 if the cheat file has been renamed, else 0
|*|			  - int IsBCD(int ParamValue) : returns 1 if ParamValue is a BCD value, else 0
|*|			  - void LoadCheat(void) : loads the cheats for a game
|*|				(this function is called by InitCheat and SelectCheat)
|*|			Added Minimum and Maximum to struct cheat_struct (used by types 60-65 & 70-75)
|*|
|*|	JCK 981020:	Added new types of cheats : 60 to 65 and 70 to 75 (see below)
|*|			Possibility to change the maximum value when the user edits a cheat
|*|			New functions :
|*|			  - int SelectValue(int v, int BCDOnly, int ZeroPoss, int Mini, int Maxi) :
|*|			      returns the value selected
|*|
|*|	JCK 981021:	Added the corrections made by Brad Oliver :
|*|			  - fixed the bug that didn't allow users to find cheats
|*|			  - changed function build_tables : it has now int ParamCpuNo as parameter
|*|			  - use OSD_KEY_CHEAT_TOGGLE (= OSD_KEY_F5) to to toggle cheats ON/OFF
|*|
|*|	JCK 981022:	Possibility to copy any watch is the first empty space by pressing OSD_KEY_F4
|*|			Possibility to add the selected watch to the cheat list by pressing OSD_KEY_F1
|*|			Possibility to add all the watches to the cheat list by pressing OSD_KEY_F6
|*|			Possibility to reset all the watches by pressing OSD_KEY_F7
|*|			Possibility to invoke help on ChooseWatch by pressing OSD_KEY_F10
|*|			Corrected help text in function CheatListHelp (F6 and F7 keys were inverted)
|*|			Added while (osd_key_pressed(key)) to remove the function keys from the buffer
|*|			  in the edit part of functions RenameCheatFile and EditCheat
|*|			Matches and Watches are added to the cheat list with a new format if many CPUs :
|*|			  %04X (%01X) = %02X   (Address (CPU) = Data)
|*|			New variables :
|*|			  - MachHeight = ( Machine -> uiheight );
|*|			  - MachWidth  = ( Machine -> uiwidth );
|*|			  - FontHeight = ( Machine -> uifont -> height );
|*|			  - FontWidth  = ( Machine -> uifont -> width );
|*|			  - ManyCpus   = ( cpu_gettotalcpu() > 1 );
|*|			Changed function xprintf : it has now int ForEdit as first parameter
|*|			  (if ForEdit=1 then add a cursor at the end of the text)
|*|			Removed function xprintfForEdit (use now the new xprintf function)
|*|			Renamed function DisplayCheats into DisplayActiveCheats
|*|			Rewritten functions :
|*|			  - int cheat_menu(void)
|*|			  - void EditCheat(int CheatNo)
|*|			  - void ChooseWatch(void)
|*|			New functions :
|*|			  - int EditCheatHeader(void)
|*|			  - int ChooseWatchHeader(void)
|*|			  - int ChooseWatchFooter(int y)
|*|			  - void ChooseWatchHelp(void)
|*|
|*|	JCK 981023:	New define : NOVALUE -0x0100 (if no value is selected)
|*|			CPU is added to the watches only if many CPUs
|*|			Changed keys in the matches list :
|*|			  - OSD_KEY_F1 adds the selected match to the cheat list (old key was OSD_KEY_ENTER)
|*|			  - OSD_KEY_F6 adds all the matches to the cheat list    (old key was OSD_KEY_F2)
|*|			Changed function SelectValue :
|*|			  - int SelectValue(int v, int BCDOnly, int ZeroPoss, int WrapPoss, int DispTwice,
|*|						  int Mini, int Maxi, char *fmt, char *msg, int ClrScr, int yPos)
|*|				* if BCDOnly=1 it accepts only BCD values
|*|				* if ZeroPoss=1 the user can select 0
|*|				* if WrapPoss=1 the user can have wrap from Mini to Maxi and from Maxi to Mini
|*|				* if DispTwice=1 the value is displayed twice when BCDOnly=0
|*|				* fmt is the format of the number when BCDOnly=0
|*|				* msg is displayed in the function
|*|				* if ClrScr=1 it clears the screen
|*|			  - it also returns NOVALUE if OSD_KEY_ESC or OSD_KEY_TAB is pressed
|*|			Renamed functions :
|*|			  - StartCheat into StartSearch
|*|			  - ContinueCheat into ContinueSearch
|*|			Rewritten functions :
|*|			  - void StartSearch(void)
|*|			  - void ContinueSearch(void)
|*|			New functions :
|*|			  - int SelectMenu(int *s, struct DisplayText *dt,
|*|						 int ArrowsOnly, int WaitForKey,
|*|						 int Mini, int Maxi, int ClrScr, int *done) :
|*|				returns the key pressed as well as the selected item
|*|			  - void AddCpuToWatch(int NoWatch,char *buffer)
|*|			  - int StartSearchHeader(void)
|*|			  - int ContinueSearchHeader(void)
|*|			  - int ContinueSearchMatchHeader(int count)
|*|			  - int ContinueSearchMatchFooter(int count, int idx, int y)
|*|
|*|	JCK 981026:	New define : FIRSTPOS (FontHeight*3/2) (yPos of the 1st string displayed)
|*|			Display [S] instead of (Saved) to fit the screen when OSD_KEY_F1 is pressed
|*|			Display [W] instead of (Watch) to fit the screen when OSD_KEY_F2 is pressed
|*|			Corrected digit change in function ChooseWatch : based on CPU instead of 0
|*|			Matches are displayed using function pShortAddrTemplate
|*|			Matches list and watches list are centered according to variable length of text
|*|
|*|	JCK 981027:	Trapped function build_tables()
|*|			Modified functions to support 32 bits CPU :
|*|			  - static char * pAddrTemplate (int cpu)
|*|			  - static char * pShortAddrTemplate (int cpu)
|*|			  - void EditCheat(int CheatNo)
|*|			  - void ChooseWatch(void)
|*|			  - int EditCheatHeader(void)
|*|			  - int ChooseWatchHeader(void)
|*|			  - void EditCheatHelp (void)
|*|			  - void ChooseWatchHelp (void)
|*|
|*|	JCK 981030:	Fixed a bug to scroll through descriptions of cheats
|*|			The active cheats are displayed with a check
|*|			Added the possibility to add a watch when result of a search is displayed
|*|			New functions :
|*|			  - void AddCheckToName(int NoCheat,char *buffer)
|*|
|*|	JCK 981123:	Removed ParamCpuNo in function build_tables (now use the static SearchCpuNo)
|*|			Added new #define : JCK - Please do not remove ! Just comment it out !
|*|
|*|	JCK 981125:	MEMORY_WRITE instead of memory_find_base in function write_gameram
|*|			New functions :
|*|			  - int ConvertAddr(int ParamCpuNo, int ParamAddr) :
|*|				returns a converted address to fit low endianess
|*|
|*|	JCK 981126:	The region of ext tables is the one of the CPU (no more crash)
|*|
|*|	JCK 981203:	Change context before writing into memory
|*|
|*|	JCK 981204:	New text when cheats are saved to file or added as watch
|*|			Changed type of cheat (special field in cheat_struct) to match
|*|			  AddCheckToName function
|*|
|*|	JCK 981206:	Ext memory is scanned only if search in a different CPU
|*|
|*|	JCK 981207:	Possibility to get info on a cheat by pressing OSD_KEY_F12
|*|			Changed function CheatListHelp to include the new OSD_KEY_F12 key
|*|			Fixed display (not in function ChooseWatch because of rotation)
|*|			New functions :
|*|			  - void ClearTextLine(int addskip, int ypos)
|*|			  - void ClearArea(int addskip, int ystart, int yend)
|*|
|*|	JCK 981208: Integrated some of VM's changes (see below)
|*|			Added new #define : FAST_SEARCH 1 (speed-up search of cheat codes)
|*|			Speed-up search of cheat codes for TMS34010 and NEOGEO games
|*|			Added messages to show that a search is in progress
|*|			Error message in function build_tables is more user-friendly
|*|
|*|	JCK 981209:	Fixed display in function ContinueSearch
|*|
|*|	JCK 981210: Integrated some of VM's changes : scan mwa instead of mra
|*|			In searches, read memory instead of contents of ext. memories
|*|			One loop to verify memory locations and count them
|*|			New functions :
|*|			  - void ShowSearchInProgress(void)
|*|			  - void HideSearchInProgress(void)
|*|			Removed functions :
|*|			  - int ConvertAddr(int ParamCpuNo, int ParamAddr)
|*|			  - static unsigned char read_ram (struct ExtMemory *table, int offset)
|*|			Removed #define :
|*|			  - RD_STARTRAM(a)	read_ram(StartRam,a)
|*|			  - WR_STARTRAM(a,v)	write_ram(StartRam,a,v)
|*|			  - RD_BACKUPRAM(a)	read_ram(BackupRam,a)
|*|			  - WR_BACKUPRAM(a,v)	write_ram(BackupRam,a,v)
|*|			  - RD_FLAGTABLE(a)	read_ram(FlagTable,a)
|*|			  - WR_FLAGTABLE(a,v)	write_ram(FlagTable,a,v)
|*|
|*|	JCK 981211:	Added new type of cheat : 99 (comment)
|*|			Comment is marked # in function AddCheckToName
|*|			Cheat structure for a comment :
|*|			  - .Address 	= 0
|*|			  - .Data 		= 0
|*|			  - .Special	= 99
|*|			  - other fields as ususal
|*|			New functions :
|*|			  - void EditComment(int CheatNo, int ypos)
|*|
|*|  Modifications by Felipe de Almeida Leme (FAL)
|*|
|*|	FAL 981029:	pointer checking to avoid segmentation fault when
|*|			reading an incorrect cheat file
|*/

#include "driver.h"
#include <stdarg.h>
#include <ctype.h>  /* for toupper() and tolower() */

/* JCK 981123 Please do not remove ! Just comment it out ! */
/* #define JCK */

/* VM & JCK 981208 - Change to 0 to scan all memory banks */
#define FAST_SEARCH 1

/*      --------- Mame Action Replay version 1.00 ------------        */

/*

Whats new in 1.00:
 -The modifications by James R. Twine listed at the beginning of the file
 -He also revised the Cheat.doc (Thanks!)
 -You can now edit a cheat with F3 in the cheat list
 -You can insert a new empty cheat in the list by pressing Insert


To do:
 -Scan the ram of all the CPUs (maybe not?)			[DONE JCK 981009]
 -Do something for the 68000						[DONE JB 980424]
  -We will have to detect where is the ram.			[DONE JB 980424]
  -Allocate variable length table					[DONE JB 980424]
  -32 bit addressing							[DONE JB 980424]
 -Probably will have problem with a full 8086 			[DONE JB 980424]
  (more than 64K)


I do not know if this will work with all the games because the value
  must be in Machine->memory_region[0]
Or should I call a routine to read/write in ram instead of going
  directly to Machine->memory_region[0]


The CHEAT.DAT file:
 -This file should be in the same directory of MAME.EXE .
 -This file can be edited with a text editor, but keep the same format:
    all fields are separated by a colon (:)
   * Name of the game (short name)
   * No of the CPU
   * Address in Hexadecimal
   * Data to put at this address in Hexadecimal
   * Type of cheat
   * Description of the cheat (30 caracters max)


Types of cheats:
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

JCK 980917
20 to 24 set the specified bits (force bits to 1); same as 0 to 4 otherwize
40 to 44 reset the specified bits (force bits to 0); same as 0 to 4 otherwize

JCK 981020
60-Select a decimal value from 0 to maximum (0 is allowed - display 0 ... maximum)
     this value is written in memory when it changes then delete cheat from active list
61-Select a decimal value from 0 to maximum (0 isn't allowed - display 1 ... maximum+1)
     this value is written in memory when it changes then delete cheat from active list
62-Select a decimal value from 1 to maximum
     this value is written in memory when it changes then delete cheat from active list
63-Select a BCD value from 0 to maximum (0 is allowed - display 0 ... maximum)
     this value is written in memory when it changes then delete cheat from active list
64-Select a BCD value from 0 to maximum (0 isn't allowed - display 1 ... maximum+1)
     this value is written in memory when it changes then delete cheat from active list
65-Select a BCD value from 1 to maximum
     this value is written in memory when it changes then delete cheat from active list
70-Select a decimal value from 0 to maximum (0 is allowed - display 0 ... maximum)
     this value is written once in memory then delete cheat from active list
71-Select a decimal value from 0 to maximum (0 isn't allowed - display 1 ... maximum+1)
     this value is written once in memory then delete cheat from active list
72-Select a decimal value from 1 to maximum
     this value is written once in memory then delete cheat from active list
73-Select a BCD value from 0 to maximum (0 is allowed - display 0 ... maximum)
     this value is written once in memory then delete cheat from active list
74-Select a BCD value from 0 to maximum (0 isn't allowed - display 1 ... maximum+1)
     this value is written once in memory then delete cheat from active list
75-Select a BCD value from 1 to maximum
     this value is written once in memory then delete cheat from active list

JCK 981211
99-Comment (can't be activated - marked with a # in the cheat list)

*/

extern struct GameDriver neogeo_bios;    /* JCK 981208 */

struct cheat_struct {
  int CpuNo;
  int Address;
  int Data;
  int Special; /* Special function usually 0 */
  int Count;
  int Backup;
  char Name[80];
  int Minimum;    /* JCK 981016 */
  int Maximum;    /* JCK 981016 */
};

/* JCK 980917 BEGIN */
#define MAX_LOADEDCHEATS	150
#define MAX_ACTIVECHEATS	15    		/* JCK 981006 */
#define MAX_DISPLAYCHEATS	15
#define MAX_MATCHES		10
#define MAX_WATCHES		10
/* JCK 980917 END */

/* JCK 981022 BEGIN */
#define TOTAL_CHEAT_TYPES	75			/* Cheat Type Values */
#define CHEAT_NAME_MAXLEN	29			/* Cheat (Edit) Name MaxLen */
#define CHEAT_FILENAME_MAXLEN	25			/* Cheat FileName MaxLen */

#define Method_1 1
#define Method_2 2
#define Method_3 3
#define Method_4 4
#define Method_5 5
/* JCK 981022 END */

#define NOVALUE 			-0x0100		/* No value has been selected */ /* JCK 981023 */

#define FIRSTPOS			(FontHeight*3/2)	/* yPos of the 1st string displayed */ /* JCK 981026 */

static int CheatTotal;
static struct cheat_struct CheatTable[MAX_ACTIVECHEATS+1];    /* JCK 980917 */
static int LoadedCheatTotal;
static struct cheat_struct LoadedCheatTable[MAX_LOADEDCHEATS+1];    /* JCK 980917 */

static unsigned int WatchesCpuNo[MAX_WATCHES];    /* JCK 981008 */
static unsigned int Watches[MAX_WATCHES];    /* JCK 980917 */
static int WatchesFlag;
static int WatchX,WatchY;

static int StartValue;
static int CurrentMethod;
static int SaveMethod;    /* JCK 981023 */

/* JRT5 */
static int	iCheatInitialized = 0;
void		CheatListHelp( void );
void		CheatListHelpEmpty( void );		/* JCK 981016 */
void		EditCheatHelp( void );
void		StartSearchHelp( void );
void		ChooseWatchHelp( void );		/* JCK 981022 */
/* JRT5 */

static int CheatEnabled;
int he_did_cheat;

static int WatchEnabled;    /* JCK 980917 */

char *cheatfile = "CHEAT.DAT";    /* JCK 980917 */

static int SearchCpuNo;    /* JCK 981006 */
static int SearchCpuNoOld; /* JCK 981206 */

static int MallocFailure;  /* JCK 981208 */


/* JCK 981022 BEGIN */
int MachHeight;
int MachWidth;
int FontHeight;
int FontWidth;
int ManyCpus;
/* JCK 981022 END */

static int ValTmp;			/* JCK 981023 */

/* JCK 981211 BEGIN */
char CCheck[2]	= "\x8C";
char CNoCheck[2]	= "\x20";
char CComment[2]	= "\x23";
/* JCK 981211 END */

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

/* Print a string at the position x y
 * if x = 0 then center the string in the screen
 * if ForEdit = 0 then adds a cursor at the end of the text */
void xprintf(int ForEdit,int x,int y,char *fmt,...)    /* JCK 981022 */
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
	dt[0].x = (MachWidth - FontWidth * strlen(s)) / 2;
  else
	dt[0].x = x;
  if(dt[0].x < 0)
	dt[0].x = 0;
  if(dt[0].x > MachWidth)
	dt[0].x = 0;
  dt[0].y = y;
  dt[1].text = 0;

  displaytext(dt,0,1);

  /* JCK 981022 BEGIN */
  if (ForEdit == 1)
  {
	dt[0].x += FontWidth * strlen(s);
	s[0] = '_';
	s[1] = 0;
	dt[0].color = DT_COLOR_YELLOW;
	displaytext(dt,0,1);
  }
  /* JCK 981022 END */

}

/* JCK 981207 BEGIN */
void ClearTextLine(int addskip, int ypos)
{
  int i;
  int addx = 0, addy = 0;

  if (addskip)
  {
	addx = Machine->uixmin;
	addy = Machine->uiymin;
  }

  for (i=ypos; (i< ( ypos + FontHeight + 1 )) && (i< Machine->scrbitmap->height); i++)
	memset (Machine->scrbitmap->line[i + addy], 0, MachWidth+addx);
}

void ClearArea(int addskip, int ystart, int yend)
{
  int i;
  int addx = 0, addy = 0;

  if (addskip)
  {
	addx = Machine->uixmin;
	addy = Machine->uiymin;
  }

  for (i=ystart; (i< yend) && (i< Machine->scrbitmap->height); i++)
	memset (Machine->scrbitmap->line[i + addy], 0, MachWidth+addx);
}
/* JCK 981207 END */

/* return a format specifier for printf based on cpu address range */
static char * pAddrTemplate (int cpu)
{
	static char s[32];

	switch ((ADDRESS_BITS(cpu)+3) >> 2)
	{
		/* JCK 981027 BEGIN */
		case 1:
		case 2:
		case 3:
		case 4:
			strcpy (s, "Addr:      $%04X");
			break;
		case 5:
			strcpy (s, "Addr:     $%05X");
			break;
		case 6:
			strcpy (s, "Addr:    $%06X");
			break;
		case 7:
			strcpy (s, "Addr:   $%07X");
			break;
		case 8:
		default:
			strcpy (s, "Addr:  $%08X");
			break;
		/* JCK 981027 END */
	}
	return s;
}

/* return a format specifier for printf based on cpu address range */
static char * pShortAddrTemplate (int cpu)
{
	static char s[32];

	/* JCK 981027 BEGIN */
	switch ((ADDRESS_BITS(cpu)+3) >> 2)
	{
		case 1:
		case 2:
		case 3:
		case 4:
			strcpy (s, "%04X");
			break;
		case 5:
			strcpy (s, "%05X");
			break;
		case 6:
			strcpy (s, "%06X");
			break;
		case 7:
			strcpy (s, "%07X");
			break;
		case 8:
		default:
			strcpy (s, "%08X");
			break;
	}
	return s;
	/* JCK 981027 END */
}

/* END JB 980424 */

/* START JB 980407 */
static struct ExtMemory StartRam[MAX_EXT_MEMORY];
static struct ExtMemory BackupRam[MAX_EXT_MEMORY];
static struct ExtMemory FlagTable[MAX_EXT_MEMORY];

#define RD_GAMERAM(cpu,a)	read_gameram(cpu,a)
#define WR_GAMERAM(cpu,a,v)	write_gameram(cpu,a,v)

extern unsigned char *memory_find_base (int cpu, int offset);

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

	memorycontextswap(cpu);

	data = MEMORY_READ(cpu,add);    /* JCK 981125 */
	/* data = *(unsigned char *)memory_find_base (cpu, add); */

	if (cpu != save)
		memorycontextswap(save);

	return data;
}

/* write a byte to CPU 0 ram at address <add> */
static void write_gameram (int cpu, int add, unsigned char data)
{
	int save = cpu_getactivecpu();    /* JCK 981203 */

	memorycontextswap(cpu);

	MEMORY_WRITE(cpu,add,data);    /* JCK 981125 */
	/* *(unsigned char *)memory_find_base (cpu, add) = data; */

	if (cpu != save)
		memorycontextswap(save);
}

/* make a copy of each ram area from search CPU ram to the specified table */
static void backup_ram (struct ExtMemory *table)
{
	struct ExtMemory *ext;
	unsigned char *gameram;

	for (ext = table; ext->data; ext++)
	{
		/* JCK 981210 BEGIN */
		int i;
		gameram = memory_find_base (SearchCpuNo, ext->start);
		memcpy (ext->data, gameram, ext->end - ext->start + 1);
		for (i=0; i <= ext->end - ext->start; i++)
			ext->data[i] = RD_GAMERAM(SearchCpuNo, i+ext->start);
		/* JCK 981210 END */
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

/* create tables for storing copies of all MWA_RAM areas */
static int build_tables (void)    /* JCK 981123 */
{
	/* const struct MemoryReadAddress *mra = Machine->drv->cpu[SearchCpuNo].memory_read; */
	const struct MemoryWriteAddress *mwa = Machine->drv->cpu[SearchCpuNo].memory_write;    /* VM 981210 */

	int region = Machine->drv->cpu[SearchCpuNo].memory_region;    /* JCK 981126 */

	struct ExtMemory *ext_sr = StartRam;
	struct ExtMemory *ext_br = BackupRam;
	struct ExtMemory *ext_ft = FlagTable;

	int yPos;    /* JCK 981208 */

      /* VM & JCK 981208 BEGIN */
      /* Trap memory allocation errors */
	int MemoryNeeded = 0;

	/* Search speedup : (the games should be dasmed to confirm this) */
      /* Games based on Exterminator driver should scan BANK1          */
      /* Games based on SmashTV driver should scan BANK2               */
      /* NEOGEO games should only scan BANK1 (0x100000 -> 0x01FFFF)    */
      /* We should better make reference to a driver instead of CPU    */
	int CpuToScan = -1;
	int BankToScan = 0;  /* scan RAM and BANKn */

	if ((Machine->drv->cpu[1].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010)
	{
		/* 2nd CPU is 34010: games based on Exterminator driver */
		CpuToScan = 0;
		BankToScan = 1;
	}
	else if ((Machine->drv->cpu[0].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010)
	{
		/* 1st CPU but not 2nd is 34010: games based on SmashTV driver */
		CpuToScan = 0;
		BankToScan = 2;
	}
	else if (Machine->gamedrv->clone_of == &neogeo_bios)
	{
		/* games based on NEOGEO driver */
		CpuToScan = 0;
		BankToScan = 1;
	}
      /* VM & JCK 981208 BEGIN */

	/* JCK 981208 BEGIN */
	/* free memory that was previously allocated if no error occured */
      /* it must also be there because mwa varies from one CPU to another */
      if (!MallocFailure)
      {
		reset_table (StartRam);
		reset_table (BackupRam);
		reset_table (FlagTable);
      }

      MallocFailure = 0;

      /* Message to show that something is in progress */
	osd_clearbitmap(Machine->scrbitmap);
      yPos = FIRSTPOS + 4 * FontHeight;
	xprintf(0, 0, yPos, "Allocating Memory...");
	/* JCK 981208 END */

	/* VM 981210 BEGIN */
	while (mwa->start != -1)
	{
		/* int (*handler)(int) = mwa->handler; */
		void (*handler)(int,int) = mwa->handler;
		int size = (mwa->end - mwa->start) + 1;

		switch ((FPTR)handler)
		{
			/* only track RAM areas */
        		case (FPTR)MWA_RAM:
			case (FPTR)MWA_BANK1:
			case (FPTR)MWA_BANK2:
			case (FPTR)MWA_BANK3:
			case (FPTR)MWA_BANK4:
			case (FPTR)MWA_BANK5:
			case (FPTR)MWA_BANK6:
			case (FPTR)MWA_BANK7:
			case (FPTR)MWA_BANK8:

			/* default: */

				#ifdef JCK
				if (errorlog) fprintf(errorlog,"build_tables : cpu = %d - handler = %d - start = %X - end = %X\n",
								SearchCpuNo, (FPTR)handler, mwa->start,mwa->end);
				#endif

                        /* VM & JCK 981208 BEGIN */
                        /* Change the define to scan all banks */
                        #if FAST_SEARCH
				if (	(CpuToScan == SearchCpuNo)	&&
					(	((BankToScan == 1) && (handler != MWA_BANK1))	||
						((BankToScan == 2) && (handler != MWA_BANK2))	)	)
				{
					mwa++;
					continue;
				}
                        #endif
                        /* VM & JCK 981208 END */

                        /* JCK 981208 BEGIN */
				/* time to allocate */
                        if (!MallocFailure)
                        {
					ext_sr->data = malloc (size);
					ext_br->data = malloc (size);
					ext_ft->data = malloc (size);

					if (ext_sr->data == NULL)
                              {
						MallocFailure = 1;
                        		MemoryNeeded += size;
                              }
					if (ext_br->data == NULL)
                              {
						MallocFailure = 1;
                        		MemoryNeeded += size;
                              }
					if (ext_ft->data == NULL)
                              {
						MallocFailure = 1;
                        		MemoryNeeded += size;
                              }

                              if (!MallocFailure)
                              {
						ext_sr->start = ext_br->start = ext_ft->start = mwa->start;
						ext_sr->end = ext_br->end = ext_ft->end = mwa->end;
						ext_sr->region = ext_br->region = ext_ft->region = region;    /* JCK 981126 */

						#ifdef JCK
						if (errorlog)
						{
							fprintf(errorlog,"               srs = %06X - sre = %06X - srr = %d\n",
										ext_sr->start, ext_sr->end, ext_sr->region);
							fprintf(errorlog,"               brs = %06X - bre = %06X - brr = %d\n",
										ext_br->start, ext_br->end, ext_br->region);
							fprintf(errorlog,"               srs = %06X - fte = %06X - ftr = %d\n",
										ext_ft->start, ext_ft->end, ext_ft->region);
						}
						#endif

						ext_sr++, ext_br++, ext_ft++;
                              }
                        }
                        else
                        	MemoryNeeded += (3 * size);
                        /* JCK 981208 END */
				break;
		}
		mwa++;
	}
	/* VM 981210 END */

	/* JCK 981208 BEGIN */
	/* free memory that was previously allocated if an error occured */
      if (MallocFailure)
      {
		int key;

		reset_table (StartRam);
		reset_table (BackupRam);
		reset_table (FlagTable);

		osd_clearbitmap(Machine->scrbitmap);
            yPos = FIRSTPOS;
		xprintf(0, 0, yPos, "Error while allocating memory !");
		yPos += (2 * FontHeight);
		xprintf(0, 0, yPos, "You need %d more bytes", MemoryNeeded);
		yPos += FontHeight;
		xprintf(0, 0, yPos, "(0x%X) of free memory", MemoryNeeded);
		yPos += (2 * FontHeight);
		xprintf(0, 0, yPos, "No search available for CPU %d", SearchCpuNo);
		yPos += (4 * FontHeight);
		xprintf(0, 0, yPos, "Press A Key To Continue...");
		key = osd_read_keyrepeat(0);
		while (osd_key_pressed(key)) ; /* wait for key release */
		osd_clearbitmap(Machine->scrbitmap);
      }
	/* JCK 981208 BEGIN */

	return MallocFailure;    /* JCK 981208 */
}


/* JCK 981210 BEGIN */
/* Message to show that something is in progress */
void ShowSearchInProgress(void)
{
  int yPos = FIRSTPOS + (4 * FontHeight);

  osd_clearbitmap(Machine->scrbitmap);
  xprintf(0, 0, yPos, "Search in Progress...");
}

void HideSearchInProgress(void)
{
  int yPos = FIRSTPOS + (4 * FontHeight);

  ClearTextLine (1, yPos);
}
/* JCK 981210 END */

/* JCK 981016 BEGIN */
/* Function to rename the cheatfile (returns 1 if the file has been renamed else 0)*/
int RenameCheatFile(void)
{
  int i, key;
  int done = 0;
  int EditYPos;
  char buffer[32];

  EditYPos = FIRSTPOS + 3 * FontHeight;    /* JCK 981026 */

  osd_clearbitmap (Machine->scrbitmap);
  xprintf (0, 0, EditYPos-(FontHeight*2), "Enter the Cheat Filename:");
  memset (buffer, '\0', 32);
  strncpy (buffer, cheatfile, CHEAT_FILENAME_MAXLEN);
  ClearTextLine(1, EditYPos);    /* JCK 981207 */
  xprintf (1, 0, EditYPos, "%s", buffer);
  do
  {
	int length;
	length = strlen (buffer);
	key = osd_read_keyrepeat (0);
	switch (key)
	{
		case OSD_KEY_BACKSPACE:
			if (length)
				buffer[length-1] = 0;
			break;
		case OSD_KEY_ENTER:
			done = 1;
			if (length)
				strcpy (cheatfile, buffer);
			else
				done = 2;
			break;
		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			done = 2;
			break;
		default:
			if (length < CHEAT_FILENAME_MAXLEN)
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
				else
					while (osd_key_pressed(key)) ;    /* JCK 981022 */
			}
			break;
	}
	ClearTextLine(1, EditYPos);    /* JCK 981207 */
	xprintf (1, 0, EditYPos, "%s", buffer);
  } while (done == 0);
  if (done == 1)
  {
	osd_clearbitmap (Machine->scrbitmap);
      xprintf (0, 0, EditYPos-(FontHeight*2), "Cheat Filename is now:");
	xprintf (0, 0, EditYPos, "%s", buffer);    /* JCK 981022 */
	EditYPos += 4*FontHeight;
	xprintf(0, 0,EditYPos,"Press A Key To Continue...");
	key = osd_read_keyrepeat(0);
	while (osd_key_pressed(key)) ; /* wait for key release */
  }
  return(done);
}

/* Function to test if a value is BCD (returns 1) or not (returns 0) */
int IsBCD(int ParamValue)
{
  return(ParamValue%0x10<=9?1:0);
}
/* JCK 981016 END */

/* JCK 981023 BEGIN */
/* Function to create menus (returns the number of lines) */
int CreateMenu (char **paDisplayText, struct DisplayText *dt, int yPos)
{
  int i = 0;

  while (paDisplayText[i])
  {
	if(i)
		dt[i].y = (dt[i - 1].y + FontHeight + 2);
	else
		dt[i].y = yPos;
	dt[i].color = DT_COLOR_WHITE;
	dt[i].text = paDisplayText[i];
	dt[i].x = (MachWidth - FontWidth * strlen(dt[i].text)) / 2;
	if(dt[i].x > MachWidth)
		dt[i].x = 0;
	i++;
  }
  dt[i].text = 0; /* terminate array */
  return(i);
}

/* Function to select a line from a menu (returns the key pressed) */
int SelectMenu(int *s, struct DisplayText *dt, int ArrowsOnly, int WaitForKey,
			int Mini, int Maxi, int ClrScr, int *done)
{
  int i,key;

  if (ClrScr == 1)
	osd_clearbitmap(Machine->scrbitmap);

  if (Maxi<Mini)
  {
	xprintf(0,0,0,"SM : Mini = %d - Maxi = %d",Mini,Maxi);
	key = osd_read_keyrepeat(0);
	while (osd_key_pressed(key)) ; /* wait for key release */
	*done = 2;
	*s = NOVALUE;
	return (NOVALUE);
  }

  *done = 0;
  do
  {
	for (i = 0;i < Maxi + 1;i++)
		dt[i].color = (i == *s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
	displaytext(dt,0,1);

	key = osd_read_keyrepeat(0);
	switch (key)
	{
		case OSD_KEY_DOWN:
			if (*s < Maxi)
				(*s)++;
			else
				*s = Mini;
			while ((*s < Maxi) && (!dt[*s].text[0]))    /* For Space In Menu */
				(*s)++;
			break;
		case OSD_KEY_UP:
			if (*s > Mini)
				(*s)--;
			else
				*s = Maxi;
			while ((*s > Mini) && (!dt[*s].text[0]))    /* For Space In Menu */
				(*s)--;
			break;
		case OSD_KEY_HOME:
			if (!ArrowsOnly)
				*s = Mini;
			else
				*done = 3;
			break;
		case OSD_KEY_END:
			if (!ArrowsOnly)
				*s = Maxi;
			else
				*done = 3;
			break;
		case OSD_KEY_ENTER:
			*done = 1;
			break;
		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			*done = 2;
			break;
		default:
			*done = 3;
			break;
	}
	if ((*done != 0) && (WaitForKey))
		while (osd_key_pressed(key)) ; /* wait for key release */
  } while (*done == 0);

  return (key);
}
/* JCK 981023 END */

/* Function to select a value (returns the value or -1 if OSD_KEY_ESC or OSD_KEY_TAB) */ /* JCK 981023 */
int SelectValue(int v, int BCDOnly, int ZeroPoss, int WrapPoss, int DispTwice,
			int Mini, int Maxi, char *fmt, char *msg, int ClrScr, int yPos)
{
  int done,key,w,MiniIsZero;

  if (ClrScr == 1)    /* JCK 981023 */
	osd_clearbitmap(Machine->scrbitmap);

  if (Maxi<Mini)
  {
	xprintf(0,0,0,"SV : Mini = %d - Maxi = %d",Mini,Maxi);
	key = osd_read_keyrepeat(0);
	while (osd_key_pressed(key)) ; /* wait for key release */
	return (NOVALUE);
  }

  /* align Mini and Maxi to the first BCD value */
  if (BCDOnly == 1)
  {
	while (IsBCD(Mini) == 0) Mini-- ;
	while (IsBCD(Maxi) == 0) Maxi-- ;
  }
  MiniIsZero = ((Mini == 0) && (ZeroPoss == 0));
  /* add 1 if Mini = 0 and 0 can't be selected */
  if (MiniIsZero == 1)
  {
	Mini ++;
	while (IsBCD(Mini) == 0) Mini++ ;
	Maxi ++;
	while (IsBCD(Maxi) == 0) Maxi++ ;
	w = v + 1;
	while (IsBCD(w) == 0) w++ ;
  }
  else
	w = v;

  /* JCK 981023 BEGIN */
  /* xprintf(0, 0,yPos,"Enter the new Value:"); */
  if (msg[0])
  {
  xprintf(0, 0, yPos, msg);
  yPos += FontHeight + 2;
  }
  /* JCK 981023 END */

  xprintf(0, 0, yPos, "(Arrow Keys Change Value)");
  yPos += 2*FontHeight;

  done = 0;
  do
  {
	if (BCDOnly == 0)
	{
		/* JCK 981023 BEGIN */
		if (DispTwice == 0)
			xprintf(0, 0, yPos, fmt, w);
		else
			xprintf(0, 0, yPos, fmt, w, w);
		/* JCK 981023 END */
	}
	else
	{
		if (w<=0xFF)
			xprintf(0, 0, yPos, "%02X", w);
		else
			xprintf(0, 0, yPos, "%03X", w);
	}

	key = osd_read_keyrepeat(0);
	switch (key)
	{
		case OSD_KEY_RIGHT:
		case OSD_KEY_UP:
			if (w < Maxi)
			{
				w++;
				if (BCDOnly == 1)
					while (IsBCD(w) == 0) w++ ;
			}
			/* JCK 981023 BEGIN */
			else
				if (WrapPoss == 1)
					w = Mini;
			/* JCK 981023 END */
			break;
		case OSD_KEY_LEFT:
		case OSD_KEY_DOWN:
			if (w > Mini)
			{
				w--;
				if (BCDOnly == 1)
					while (IsBCD(w) == 0) w-- ;
			}
			/* JCK 981023 BEGIN */
			else
				if (WrapPoss == 1)
					w = Maxi;
			/* JCK 981023 END */
			break;
		case OSD_KEY_HOME:
			w = Mini;
			break;
		case OSD_KEY_END:
			w = Maxi;
			break;

		/* JCK 981023 BEGIN */
		case OSD_KEY_ENTER:
			done = 1;
			break;
		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			done = 2;
			break;
		default:
			while (osd_key_pressed(key)) ; /* wait for key release */
			break;
		/* JCK 981023 END */
	}
  } while (done == 0);

  /* JCK 981023 BEGIN */
  if (ClrScr == 1)    /* JCK 981023 */
	osd_clearbitmap(Machine->scrbitmap);

  if (done == 2)
	return (NOVALUE);
  /* JCK 981023 END */

  /* sub 1 if Mini = 0 */
  if (MiniIsZero == 1)
  {
	v = w - 1;
	if (BCDOnly == 1)
		while (IsBCD(v) == 0) v-- ;
  }
  else
	v = w;
  return (v);
}

/* Function who loads the cheats for a game */
void LoadCheat(void)
{
  FILE *f;
  char *ptr;
  char str[80];

  CheatTotal = 0;
  LoadedCheatTotal = 0;

/* Load the cheats for that game */
/* Ex.: pacman:0:4e14:6:0:Infinite Lives */
  if ((f = fopen(cheatfile,"r")) != 0){    /* JCK 980917 */
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

      if(LoadedCheatTotal >= MAX_LOADEDCHEATS-1){    /* JCK 980917 */
        break;
      }

/* JCK 980917 BEGIN */
/*Reset the counter*/
	LoadedCheatTable[LoadedCheatTotal].Count=0;
/* JCK 980917 END */

/*Extract the fields from the string*/
      ptr = strtok(str, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      ptr = strtok(NULL, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      sscanf(ptr,"%d", &LoadedCheatTable[LoadedCheatTotal].CpuNo);

      ptr = strtok(NULL, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      sscanf(ptr,"%x", &LoadedCheatTable[LoadedCheatTotal].Address);

      ptr = strtok(NULL, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      sscanf(ptr,"%x", &LoadedCheatTable[LoadedCheatTotal].Data);

      ptr = strtok(NULL, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      sscanf(ptr,"%d", &LoadedCheatTable[LoadedCheatTotal].Special);

      ptr = strtok(NULL, ":");
      if ( ! ptr )	/* FAL 981029 */
	continue;
      strcpy(LoadedCheatTable[LoadedCheatTotal].Name,ptr);
/*Chop the CRLF at the end of the string */
      LoadedCheatTable[LoadedCheatTotal].Name[strlen(LoadedCheatTable[LoadedCheatTotal].Name)-1] = 0;

/* JCK 981016 BEGIN */
/*Fill the new fields : Minimum and Maximum*/
	if ((LoadedCheatTable[LoadedCheatTotal].Special==62) || (LoadedCheatTable[LoadedCheatTotal].Special==65) ||
	    (LoadedCheatTable[LoadedCheatTotal].Special==72) || (LoadedCheatTable[LoadedCheatTotal].Special==75))
		LoadedCheatTable[LoadedCheatTotal].Minimum = 1;
	else
		LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
	if ((LoadedCheatTable[LoadedCheatTotal].Special>=60) && (LoadedCheatTable[LoadedCheatTotal].Special<=75))
	{
		LoadedCheatTable[LoadedCheatTotal].Maximum = LoadedCheatTable[LoadedCheatTotal].Data;
		LoadedCheatTable[LoadedCheatTotal].Data = 0;
	}
	else
		LoadedCheatTable[LoadedCheatTotal].Maximum = 0xFF;
/* JCK 981016 END */


	/* JCK 981211 BEGIN */
	if (LoadedCheatTable[LoadedCheatTotal].Special == 99)
	{
		LoadedCheatTable[LoadedCheatTotal].Address = 0;
		LoadedCheatTable[LoadedCheatTotal].Data = 0;
	}
	/* JCK 981211 END */

      LoadedCheatTotal++;
    }
    fclose(f);
  }
}
/* JCK 981016 END */



/*****************
 * Init some variables
 */
void InitCheat(void)
{
  int i;

  he_did_cheat = 0;
  CheatEnabled = 0;

  WatchEnabled = 0;    /* JCK 980917 */

  CurrentMethod = 0;
  SaveMethod = 0;    /* JCK 981023 */

  SearchCpuNoOld	= -1;		/* JCK 981206 */
  MallocFailure	= -1;		/* JCK 981208 */

  /* JCK 981022 BEGIN */
  MachHeight = ( Machine -> uiheight );
  MachWidth  = ( Machine -> uiwidth );
  FontHeight = ( Machine -> uifont -> height );
  FontWidth  = ( Machine -> uifont -> width );
  ManyCpus   = ( cpu_gettotalcpu() > 1 );
  /* JCK 981022 END */

/* JB 980407 */
reset_table (StartRam);
reset_table (BackupRam);
reset_table (FlagTable);

/* JRT6	- modified JB 980424 */
	for(i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
	{
		/* Watches[ i ] = MAX_ADDRESS(0); */			/* Set Unused Value For Watch*/
		WatchesCpuNo[ i ] = 0;    /* JCK 981008 */
		Watches[ i ] = MAX_ADDRESS(WatchesCpuNo[ i ]);    /* JCK 981009 */
	}
/* JRT6 */

  WatchX = Machine->uixmin;
  WatchY = Machine->uiymin;
  WatchesFlag = 0;

  LoadCheat();   /* JCK 981016 */

}

/*****************
 * Free allocated arrays
 */
void StopCheat(void)
{
/* JB 980407 */
reset_table (StartRam);
reset_table (BackupRam);
reset_table (FlagTable);
}

/*****************
 * The routine called in the emulation
 * Modify some memory location
 */
void DoCheat(void)
{
	int key;
	int i,j,y;
	char buf[80];
	char buf2[10];

	/* Display watches if there is some */
	if( (WatchesFlag != 0) && (WatchEnabled != 0) )    /* JCK 980917 */
	{
		int trueorientation;

		/* hack: force the display into standard orientation */
		trueorientation = Machine->orientation;
		Machine->orientation = ORIENTATION_DEFAULT;

		buf[0] = 0;
		for (i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
		{
			/* if( Watches[ i ] != MAX_ADDRESS(0)) */ 		/* If Watch Is In Use*/
			if( Watches[ i ] != MAX_ADDRESS(WatchesCpuNo[ i ]))    /* JCK 981009 */
			{
				/*sprintf(buf2,"%02X ",Machine->memory_region[0][Watches[i]]);*/
				/* sprintf(buf2,"%02X ", RD_GAMERAM (0, Watches[i])); */ /* JB 980407 */
				sprintf(buf2,"%02X ", RD_GAMERAM (WatchesCpuNo[i], Watches[i]));    /* JCK 981008 */
				strcat(buf,buf2);
			}
		}

		for (i = 0;i < (int)strlen(buf);i++)
			drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				WatchX+(i*FontWidth),WatchY,0,TRANSPARENCY_NONE,0);

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
							CheatTable[j].Minimum = CheatTable[j+1].Minimum;    /* JCK 981016 */
							CheatTable[j].Maximum = CheatTable[j+1].Maximum;    /* JCK 981016 */
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
							CheatTable[i].Special = 105;    /* JCK 981204 */
						}
						break;
					case 6:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 2*60;
							CheatTable[i].Special = 106;    /* JCK 981204 */
						}
						break;
					case 7:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 5*60;
							CheatTable[i].Special = 107;    /* JCK 981204 */
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
							CheatTable[i].Special = 108;    /* JCK 981204 */
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 9:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 109;    /* JCK 981204 */
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 10:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 110;    /* JCK 981204 */
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;
					case 11:
						/* JB 980407 */
						if (RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Data)
						{
							CheatTable[i].Count = 1;
							CheatTable[i].Special = 111;    /* JCK 981204 */
							CheatTable[i].Backup =
								RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						}
						break;

					/* JCK 980917 BEGIN */
					case 20:
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) | CheatTable[i].Data);
						break;
					case 21:
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) | CheatTable[i].Data);

						/*Delete this cheat from the table*/
						for(j = i;j<CheatTotal-1;j++)
						{
							CheatTable[j].CpuNo = CheatTable[j+1].CpuNo;
							CheatTable[j].Address = CheatTable[j+1].Address;
							CheatTable[j].Data = CheatTable[j+1].Data;
							CheatTable[j].Special = CheatTable[j+1].Special;
							CheatTable[j].Count = CheatTable[j+1].Count;
							strcpy(CheatTable[j].Name,CheatTable[j+1].Name);
							CheatTable[j].Minimum = CheatTable[j+1].Minimum;    /* JCK 981016 */
							CheatTable[j].Maximum = CheatTable[j+1].Maximum;    /* JCK 981016 */
						}
						CheatTotal--;
						break;
					case 22:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) | CheatTable[i].Data);
						CheatTable[i].Count = 1*60;
						break;
					case 23:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) | CheatTable[i].Data);
						CheatTable[i].Count = 2*60;
						break;
					case 24:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) | CheatTable[i].Data);
						CheatTable[i].Count = 5*60;
						break;
					case 40:
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) & ~CheatTable[i].Data);
						break;
					case 41:
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) & ~CheatTable[i].Data);

						/*Delete this cheat from the table*/
						for(j = i;j<CheatTotal-1;j++)
						{
							CheatTable[j].CpuNo = CheatTable[j+1].CpuNo;
							CheatTable[j].Address = CheatTable[j+1].Address;
							CheatTable[j].Data = CheatTable[j+1].Data;
							CheatTable[j].Special = CheatTable[j+1].Special;
							CheatTable[j].Count = CheatTable[j+1].Count;
							strcpy(CheatTable[j].Name,CheatTable[j+1].Name);
							CheatTable[j].Minimum = CheatTable[j+1].Minimum;    /* JCK 981016 */
							CheatTable[j].Maximum = CheatTable[j+1].Maximum;    /* JCK 981016 */
						}
						CheatTotal--;
						break;
					case 42:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) & ~CheatTable[i].Data);
						CheatTable[i].Count = 1*60;
						break;
					case 43:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) & ~CheatTable[i].Data);
						CheatTable[i].Count = 2*60;
						break;
					case 44:
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address,
							RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address) & ~CheatTable[i].Data);
						CheatTable[i].Count = 5*60;
						break;
					/* JCK 980917 END */

					/* JCK 981030 BEGIN */
					case 60:
					case 61:
					case 62:
					case 63:
					case 64:
					case 65:
						CheatTable[i].Count = 1;
						CheatTable[i].Special += 100;    /* JCK 981204 */
						CheatTable[i].Backup = RD_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address);
						break;
					case 70:
					case 71:
					case 72:
					case 73:
					case 74:
					case 75:
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
							CheatTable[j].Minimum = CheatTable[j+1].Minimum;
							CheatTable[j].Maximum = CheatTable[j+1].Maximum;
						}
						CheatTotal--;
						break;
					/* JCK 981030 END */

						/*Special case, linked with 5,6,7 */
					case 105:    /* JCK 981204 */
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 5;
						break;
					case 106:    /* JCK 981204 */
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 6;
						break;
					case 107:    /* JCK 981204 */
						/* JB 980407 */
						WR_GAMERAM (CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 7;
						break;

					/*Special case, linked with 8,9,10,11 */
					/* Change the memory only if the memory decreased by X */
					case 108:    /* JCK 981204 */
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-1)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 8;
						break;
					case 109:    /* JCK 981204 */
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-2)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 9;
						break;
					case 110:    /* JCK 981204 */
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-3)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 10;
						break;
					case 111:    /* JCK 981204 */
						/* JB 980407 */
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup-4)
							WR_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address, CheatTable[i].Data);
						CheatTable[i].Special = 11;
						break;

					/* JCK 981204 BEGIN */
					/*Special case, linked with 60 .. 65 */
					/* Change the memory only if the memory has changed since the last backup */
					case 160:
					case 161:
					case 162:
					case 163:
					case 164:
					case 165:
						if (RD_GAMERAM(CheatTable[i].CpuNo, CheatTable[i].Address) != CheatTable[i].Backup)
						{
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
								CheatTable[j].Minimum = CheatTable[j+1].Minimum;
								CheatTable[j].Maximum = CheatTable[j+1].Maximum;
							}
							CheatTotal--;
						}
						break;
					/* JCK 981204 END */

          		}	/* end switch */
        	} /* end if(CheatTable[i].Count == 0) */
        	else
			{
				CheatTable[i].Count--;
			}
		} /* end else */
	} /* end for */


  /* ` continue cheat, to accelerate the search for new cheat */

  /* F5 Enable/Disable the active cheats on the fly. Required for some cheat */
  if( osd_key_pressed_memory( OSD_KEY_CHEAT_TOGGLE ) && CheatTotal )    /* JCK 981021 */
  {
	y = (MachHeight - FontHeight) / 2;
	if(CheatEnabled == 0)
	{
		CheatEnabled = 1;
		xprintf(0, 0,y,"Cheats On");
	}
	else
	{
	      CheatEnabled = 0;
	      xprintf(0, 0,y,"Cheats Off");
	}

	/* JRT3 10-23-97 BEGIN */
	osd_clearbitmap( Machine -> scrbitmap );        /* Clear Screen */
	(Machine->drv->vh_update)(Machine->scrbitmap,1);  /* Make Game Redraw Screen */
	/* JRT3 10-23-97 END */
  }

  /* Ins toggles the Watch display ON */
  if( osd_key_pressed_memory( OSD_KEY_INSERT ) && (WatchEnabled == 0) )
  {
	WatchEnabled = 1;
  }
  /* Del toggles the Watch display OFF */
  if( osd_key_pressed_memory( OSD_KEY_DEL ) && (WatchEnabled != 0) ){
	WatchEnabled = 0;
  }
  /* JCK 980917 END */
}


/* JCK 981022 BEGIN */
int EditCheatHeader(void)
{
  int i = 0;
  char *paDisplayText[] = {
		"To edit a Cheat name, press",
		"<ENTER> when name is selected.",
		"To edit values or to select a",
		"pre-defined Cheat Name, use :",
		"<+> and Right arrow key: +1",
		"<-> and Left  arrow key: -1",
		"<1> ... <8>: +1 digit",    /* JCK 981027 */
		"",
		"<F10>: Show Help + other keys",
		0 };

  struct DisplayText dt[20];

  while (paDisplayText[i])
  {
	if(i)
		dt[i].y = (dt[i - 1].y + FontHeight + 2);
	else
		dt[i].y = FIRSTPOS;
	dt[i].color = DT_COLOR_WHITE;
	dt[i].text = paDisplayText[i];
	dt[i].x = (MachWidth - FontWidth * strlen(dt[i].text)) / 2;
	if(dt[i].x > MachWidth)
		dt[i].x = 0;
	i++;
  }
  dt[i].text = 0; /* terminate array */
  displaytext(dt,0,1);
  return(dt[i-1].y + ( 3 * FontHeight ));
}
/* JCK 981022 END */

void EditCheat(int CheatNo)
{
  char *CheatNameList[] = {
	"Infinite Lives",
	"Infinite Lives PL1",
	"Infinite Lives PL2",
	"Invincibility",
	"Infinite Time",
	"Infinite Ammo",
	"---> <ENTER> To Edit <---",
	"\0" };

  int i,s,y,key,done;
  int total;
  struct DisplayText dt[20];
  char str2[5][50];	/* JCK 981022 */
  char buffer[50];	/* JCK 981022 */
  int CurrentName;
  int EditYPos;

  osd_clearbitmap(Machine->scrbitmap);

  y = EditCheatHeader();

  total = 0;

  /* JCK 981019 BEGIN */
  if ((LoadedCheatTable[CheatNo].Special>=60) && (LoadedCheatTable[CheatNo].Special<=75))
	LoadedCheatTable[CheatNo].Data = LoadedCheatTable[CheatNo].Maximum;
  /* JCK 981019 END */

  sprintf(str2[0],"Name: %s",LoadedCheatTable[CheatNo].Name);
  if((FontWidth * (int)strlen(str2[0])) > MachWidth)
	sprintf(str2[0],"%s",LoadedCheatTable[CheatNo].Name);
  sprintf(str2[1],"CPU:        %01X",LoadedCheatTable[CheatNo].CpuNo);    /* JCK 981022 */

  /* JRT6 - modified JB 980407 */
  sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),LoadedCheatTable[CheatNo].Address);
  /* JRT6 */

  sprintf(str2[3],"Value:    %03d  (0x%02X)",LoadedCheatTable[CheatNo].Data,LoadedCheatTable[CheatNo].Data);
  sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);

  for (i=0;i<5;i++){
	dt[total].text = str2[i];
	dt[total].x = MachWidth / 2;
	if(MachWidth < 35*FontWidth)
		dt[total].x = 0;
	else
		dt[total].x -= 15*FontWidth;
	dt[total].y = y;
	dt[total].color = DT_COLOR_WHITE;
	total++;
	y += FontHeight;
  }

  dt[total].text = 0; /* terminate array */

  EditYPos = ( y + ( 4 * FontHeight ) );

  s = 0;
  CurrentName = -1;

  done = 0;
  do
  {

	for (i = 0;i < total;i++)
		dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
	displaytext(dt,0,1);

	key = osd_read_keyrepeat(0);
	switch (key)
	{
      	case OSD_KEY_DOWN:
			if (s < total - 1)
				s++;
			else
				s = 0;
			break;

		case OSD_KEY_UP:
			if (s > 0)
				s--;
			else
				s = total - 1;
			break;

		case OSD_KEY_MINUS_PAD:
		case OSD_KEY_LEFT:	/* JB 980424 */
			switch (s)
			{
				case 0:	/* Name */

					/* JCK 981030 BEGIN */
					if (CurrentName < 0)
						CurrentName = 0;
					/* JCK 981030 END */

					if (CurrentName == 0)						/* wrap if necessary*/
						while (CheatNameList[CurrentName][0])
							CurrentName++;
					CurrentName--;
					strcpy (LoadedCheatTable[CheatNo].Name, CheatNameList[CurrentName]);
					sprintf (str2[0],"Name: %s", LoadedCheatTable[CheatNo].Name);
					ClearTextLine(1, dt[0].y);    /* JCK 981207 */
					break;
				case 1:	/* CpuNo */
					if (ManyCpus)    /* JCK 981022 */
					{
						if (LoadedCheatTable[CheatNo].CpuNo == 0)
							LoadedCheatTable[CheatNo].CpuNo = cpu_gettotalcpu() - 1;
						else
							LoadedCheatTable[CheatNo].CpuNo --;
						sprintf (str2[1], "CPU:        %01X", LoadedCheatTable[CheatNo].CpuNo);    /* JCK 981022 */
					}
					break;
				case 2:	/* Address */
					if (LoadedCheatTable[CheatNo].Address == 0)
						LoadedCheatTable[CheatNo].Address = MAX_ADDRESS(LoadedCheatTable[CheatNo].CpuNo);
					else
						LoadedCheatTable[CheatNo].Address --;
					sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
						LoadedCheatTable[CheatNo].Address);
					break;
				case 3:	/* Data */
					if (LoadedCheatTable[CheatNo].Data == 0)
						LoadedCheatTable[CheatNo].Data = 0xFF;
					else
						LoadedCheatTable[CheatNo].Data --;
					sprintf(str2[3], "Value:    %03d  (0x%02X)", LoadedCheatTable[CheatNo].Data,
						LoadedCheatTable[CheatNo].Data);
					break;
				case 4:	/* Special */
					/*
					if (LoadedCheatTable[CheatNo].Special <= 0)
						LoadedCheatTable[CheatNo].Special = TOTAL_CHEAT_TYPES;
					else
						LoadedCheatTable[CheatNo].Special --;
					*/

					/* JCK 981020 BEGIN */
					if (LoadedCheatTable[CheatNo].Special <= 0)
						LoadedCheatTable[CheatNo].Special = TOTAL_CHEAT_TYPES;
					else
      					switch (LoadedCheatTable[CheatNo].Special)
						{
							case 20:
								LoadedCheatTable[CheatNo].Special = 11;
								break;
							case 40:
								LoadedCheatTable[CheatNo].Special = 24;
								break;
							case 60:
								LoadedCheatTable[CheatNo].Special = 44;
								break;
							case 70:
								LoadedCheatTable[CheatNo].Special = 65;
								break;
							default:
								LoadedCheatTable[CheatNo].Special --;
								break;
						}
					/* JCK 981020 END */

					sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);
					break;
			}
			break;

		case OSD_KEY_PLUS_PAD:
		case OSD_KEY_RIGHT:	/* JB 980424 */
			switch (s)
			{
				case 0:	/* Name */
					CurrentName ++;
					if (CheatNameList[CurrentName][0] == 0)
						CurrentName = 0;
					strcpy (LoadedCheatTable[CheatNo].Name, CheatNameList[CurrentName]);
					sprintf (str2[0], "Name: %s", LoadedCheatTable[CheatNo].Name);
					ClearTextLine(1, dt[0].y);    /* JCK 981207 */
					break;
				case 1:	/* CpuNo */
					if (ManyCpus)    /* JCK 981022 */
					{
						LoadedCheatTable[CheatNo].CpuNo ++;
						if (LoadedCheatTable[CheatNo].CpuNo >= cpu_gettotalcpu())
							LoadedCheatTable[CheatNo].CpuNo = 0;
						sprintf(str2[1],"CPU:        %01X",LoadedCheatTable[CheatNo].CpuNo);    /* JCK 981022 */
					}
					break;
				case 2:	/* Address */
					LoadedCheatTable[CheatNo].Address ++;
					if (LoadedCheatTable[CheatNo].Address > MAX_ADDRESS(LoadedCheatTable[CheatNo].CpuNo))
						LoadedCheatTable[CheatNo].Address = 0;
					sprintf (str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
						LoadedCheatTable[CheatNo].Address);
					break;
				case 3:	/* Data */
					if(LoadedCheatTable[CheatNo].Data == 0xFF)
						LoadedCheatTable[CheatNo].Data = 0;
					else
						LoadedCheatTable[CheatNo].Data ++;
					sprintf(str2[3],"Value:    %03d  (0x%02X)",LoadedCheatTable[CheatNo].Data,
						LoadedCheatTable[CheatNo].Data);
					break;
				case 4: /* Special */
					/*
					LoadedCheatTable[CheatNo].Special ++;
					if(LoadedCheatTable[CheatNo].Special > TOTAL_CHEAT_TYPES)
						LoadedCheatTable[CheatNo].Special = 0;
					*/

					/* JCK 981020 BEGIN */
					if (LoadedCheatTable[CheatNo].Special >= TOTAL_CHEAT_TYPES)
						LoadedCheatTable[CheatNo].Special = 0;
					else
      					switch (LoadedCheatTable[CheatNo].Special)
						{
							case 11:
								LoadedCheatTable[CheatNo].Special = 20;
								break;
							case 24:
								LoadedCheatTable[CheatNo].Special = 40;
								break;
							case 44:
								LoadedCheatTable[CheatNo].Special = 60;
								break;
							case 65:
								LoadedCheatTable[CheatNo].Special = 70;
								break;
							default:
								LoadedCheatTable[CheatNo].Special ++;
								break;
						}
					/* JCK 981020 END */

					sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);
					break;
			}
			break;

		/* JCK 981006 BEGIN */

		case OSD_KEY_HOME:
			/* this key only applies to the data line */
			if (s == 3)
			{
				LoadedCheatTable[CheatNo].Data = 0;
				sprintf(str2[3], "Value:    %03d  (0x%02X)", LoadedCheatTable[CheatNo].Data,
					LoadedCheatTable[CheatNo].Data);
			}
			break;

		case OSD_KEY_END:
			/* this key only applies to the data line */
			if (s == 3)
			{
				LoadedCheatTable[CheatNo].Data = 0x80;
				sprintf(str2[3], "Value:    %03d  (0x%02X)", LoadedCheatTable[CheatNo].Data,
					LoadedCheatTable[CheatNo].Data);
			}
			break;

		/* JCK 981006 END */

		/* JCK 981027 BEGIN */
		case OSD_KEY_8:
			if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 29) break;
		case OSD_KEY_7:
			if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 25) break;
		case OSD_KEY_6:
			if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 21) break;
		case OSD_KEY_5:
			if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 17) break;
		case OSD_KEY_4:
			if (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo) < 13) break;
		/* JCK 981027 END */
		case OSD_KEY_3:
		case OSD_KEY_2:
		case OSD_KEY_1:
			/* these keys only apply to the address line */
			if (s == 2)
			{
				int addr = LoadedCheatTable[CheatNo].Address;	/* copy address*/
				int digit = (OSD_KEY_8 - key);	/* if key is OSD_KEY_8, digit = 0*/	/* JCK 981027 */
				int mask;

				/* adjust digit based on cpu address range */
				digit -= (8 - (ADDRESS_BITS(LoadedCheatTable[CheatNo].CpuNo)+3) / 4);	/* JCK 981027 */

				mask = 0xF << (digit * 4);	/* if digit is 1, mask = 0xf0*/

				/* JCK 981027 BEGIN */
				do
				{
				if ((addr & mask) == mask)
					/* wrap hex digit around to 0 if necessary */
					addr &= ~mask;
				else
					/* otherwise bump hex digit by 1 */
					addr += (0x1 << (digit * 4));
				} while (addr > MAX_ADDRESS(LoadedCheatTable[CheatNo].CpuNo));
				/* JCK 981027 END */

				LoadedCheatTable[CheatNo].Address = addr;
				sprintf(str2[2], pAddrTemplate(LoadedCheatTable[CheatNo].CpuNo),
					LoadedCheatTable[CheatNo].Address);
			}
			break;

		case OSD_KEY_F10:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
			EditCheatHelp();								/* Show Help*/
			y = EditCheatHeader();    /* JCK 981022 */
			break;

		case OSD_KEY_ENTER:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
			if (s == 0)
			{
				/* Edit Name */
				for (i = 0; i < total; i++)
				dt[i].color = DT_COLOR_WHITE;
				displaytext (dt, 0,1);
				xprintf (0, 0, EditYPos-2*FontHeight, "Edit the Cheat name:");

				/* JRT5 BEGIN  29 Char Editing Limit...*/    /* JCK 981022 */
				memset (buffer, '\0', 32);
				strncpy (buffer, LoadedCheatTable[ CheatNo ].Name, CHEAT_NAME_MAXLEN);    /* JCK 981009 */

				ClearTextLine(1, EditYPos);    /* JCK 981207 */

				xprintf (1, 0, EditYPos, "%s", buffer);    /* JCK 981022 */
				/* JRT5 END */

				do	/* JB 980506 */
				{
					int length;

					length = strlen (buffer);
					key = osd_read_keyrepeat (0);

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
								else
									while (osd_key_pressed(key)) ;    /* JCK 981022 */
							}
							break;
					}
					ClearTextLine(1, EditYPos);    /* JCK 981207 */
					xprintf (1, 0, EditYPos, "%s", buffer);
				} while (done == 0);
				done = 0;
				osd_clearbitmap (Machine->scrbitmap);
				y = EditCheatHeader();    /* JCK 981022 */
			}
			break;

		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
			done = 1;
			break;
	}
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

  /* JCK 981020 BEGIN */
  if ((LoadedCheatTable[CheatNo].Special==62) || (LoadedCheatTable[CheatNo].Special==65) ||
      (LoadedCheatTable[CheatNo].Special==72) || (LoadedCheatTable[CheatNo].Special==75))
	LoadedCheatTable[CheatNo].Minimum = 1;
  else
	LoadedCheatTable[CheatNo].Minimum = 0;
  if ((LoadedCheatTable[CheatNo].Special>=60) && (LoadedCheatTable[CheatNo].Special<=75))
  {
	LoadedCheatTable[CheatNo].Maximum = LoadedCheatTable[CheatNo].Data;
	LoadedCheatTable[CheatNo].Data = 0;
  }
  else
	LoadedCheatTable[CheatNo].Maximum = 0xFF;
  /* JCK 981020 END */

  osd_clearbitmap(Machine->scrbitmap);
}


/* JCK 981211 BEGIN */
void EditComment(int CheatNo, int ypos)
{
  int i, key;
  int done = 0;
  char buffer[32];

  memset (buffer, '\0', 32);
  strncpy (buffer, LoadedCheatTable[ CheatNo ].Name, CHEAT_NAME_MAXLEN);
  ClearTextLine(1, ypos);
  xprintf (1, 0, ypos, "%s", buffer);

  do
  {
	int length;
	length = strlen (buffer);
	key = osd_read_keyrepeat (0);
	switch (key)
	{
		case OSD_KEY_BACKSPACE:
			if (length)
				buffer[length-1] = 0;
			break;
		case OSD_KEY_ENTER:
			done = 1;
			strcpy (LoadedCheatTable[CheatNo].Name, buffer);
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
				else
					while (osd_key_pressed(key)) ;
			}
			break;
	}
	ClearTextLine(1, ypos);
	xprintf (1, 0, ypos, "%s", buffer);
  } while (done == 0);

  ClearTextLine(1, ypos);
}
/* JCK 981211 END */


void DisplayActiveCheats(int x,int y)
{
  int i;

  xprintf(0, 0,y,"Active Cheats:");
  x -= 4*FontWidth;
  y += ( FontHeight + 3 );
  for(i=0;i<CheatTotal;i++)
  {
	xprintf(0, x,y,"%s",CheatTable[i].Name);
	y += FontHeight;
  }
  if (CheatTotal == 0)
  {
	x = (MachWidth - FontWidth * 12) / 2;
	xprintf(0, x,y,"--- None ---");
  }
}

/* JCK 981030 BEGIN */
void AddCheckToName(int NoCheat,char *buffer)
{
  int i = 0;
  int flag = 0;
  int Special;

  if (LoadedCheatTable[NoCheat].Special == 99)
	strcpy(buffer,CComment);    /* JCK 981211 */
  else
  {
	for (i=0;i<CheatTotal && !flag;i++)
	{
		/* JCK 981204 BEGIN */
		Special = CheatTable[i].Special;
		if (Special > 100)
			Special -= 100;

		if (Special < 60)
		{
			if (	(CheatTable[i].Address 	== LoadedCheatTable[NoCheat].Address)	&&
				(CheatTable[i].Data	== LoadedCheatTable[NoCheat].Data	)	&&
				(Special			== LoadedCheatTable[NoCheat].Special)	)
				flag = 1;
		}
		else
		{
			if (	(CheatTable[i].Address 	== LoadedCheatTable[NoCheat].Address)	&&
				(Special			== LoadedCheatTable[NoCheat].Special)	)
				flag = 1;
		}
		/* JCK 981204 END */
	  }
	  strcpy(buffer,(flag ? CCheck : CNoCheck));
  }
  strcat(buffer," ");
  strcat(buffer,LoadedCheatTable[NoCheat].Name);
  for(i=strlen(buffer);i<CHEAT_NAME_MAXLEN+2;i++)
	buffer[i]=' ';
  buffer[CHEAT_NAME_MAXLEN+2]=0;
}
/* JCK 981030 END */

void SelectCheat(void)
{
  int i,x,y,s,key,done,total;
  int		iWhereY = 0;
  FILE *f;
  struct DisplayText dt[40];
  int flag;
  int Index;
  int StartY,EndY;

  int BCDOnly, ZeroPoss;	/* JCK 981020 */

  char str2[MAX_DISPLAYCHEATS+1][40];    /* JCK 981030 */
  int j;    /* JCK 981030 */

  char fmt[32];    /* Moved by JCK 981204 */
  char buf[40];    /* JCK 981207 */

HardRefresh:
  osd_clearbitmap(Machine->scrbitmap);

  x = MachWidth / 2;
  if(MachWidth < 35*FontWidth)
	  x = 0;
  else
	  x -= 15*FontWidth;
  y = 0;    /* JCK 981207 */
  y += 9*FontHeight; /* JCK 981207 */ /* Keep space for menu */

/* No more than MAX_DISPLAYCHEATS cheat displayed */
  if(LoadedCheatTotal > MAX_DISPLAYCHEATS)    /* JCK 980917 */
    total = MAX_DISPLAYCHEATS;
  else
    total = LoadedCheatTotal;

  Index = 0;
  StartY = y;

/* Make the list */
  for (i = 0;i < total;i++)
  {

	/* JCK 981030 BEGIN */
	/* dt[i].text = LoadedCheatTable[i].Name; */
	AddCheckToName(i, str2[ i ]);
	dt[i].text = str2[i];
	/* JCK 981030 END */

	dt[i].x = x;
	dt[i].y = y;
	dt[i].color = DT_COLOR_WHITE;
	y += FontHeight;
  }

  dt[total].text = 0; /* terminate array */

  x += 4*FontWidth;
  y += (FontHeight * 2);    /* JCK 981026 */
  /* DisplayActiveCheats(x,y); */ /* Removed by JCK 981030 */

  EndY = MachHeight - 2 * FontHeight;

  s = 0;
  done = 0;
  do
  {
    for (i = 0;i < total;i++)
      dt[i].color = (i == s) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;

    if(LoadedCheatTotal == 0){
		  iWhereY = 0;
		  xprintf(0, 0, iWhereY, "<INS>: Add New Cheat" );

		  /* JCK 981016 BEGIN */
		  iWhereY += ( FontHeight + 0 );
	        xprintf(0, 0, iWhereY,"<F10>: Show Help + other keys");
		  /* JCK 981016 END */

		  iWhereY += ( FontHeight * 3 );
      	  xprintf(0, 0,iWhereY,"No Cheats Available!");
    }else{
		  iWhereY = 0;
		  xprintf(0, 0, iWhereY, "<DEL>: Delete  <INS>: Add" );
		  iWhereY += ( FontHeight + 0 );
		  xprintf(0, 0, iWhereY,"<F1>: Save  <F2>: Watch");
		  iWhereY += ( FontHeight + 0 );

		  /* JCK 981006 BEGIN */
		  xprintf(0, 0, iWhereY,"<F3>: Edit  <F6>: Save all");
		  iWhereY += ( FontHeight + 0 );
		  xprintf(0, 0, iWhereY,"<F4>: Copy  <F7>: Del all");
		  iWhereY += ( FontHeight + 0 );
	        xprintf(0, 0, iWhereY,"<ENTER>: Enable/Disable");
		  iWhereY += ( FontHeight + 0 );
	        xprintf(0, 0, iWhereY,"<F10>: Show Help + other keys");
		  iWhereY += ( FontHeight + 4 );
		  /* JCK 981006 END */

		  xprintf(0, 0, iWhereY,"Select a Cheat (%d Total)",LoadedCheatTotal);
		  iWhereY += ( FontHeight + 4 );

	}

	displaytext(dt,0,1);

	key = osd_read_keyrepeat(0);
	ClearTextLine(1, EndY);
	switch (key)
	{
      case OSD_KEY_DOWN:
        if (s < total - 1)
          s++;
        else{

          s = 0;

          if(LoadedCheatTotal <= MAX_DISPLAYCHEATS)    /* JCK 980917 */
            break;
/*
End of list
 -Increment index
 -Redo the list
*/
          if(LoadedCheatTotal > Index+MAX_DISPLAYCHEATS)    /* JCK 980917 */
            Index += MAX_DISPLAYCHEATS;
          else
            Index = 0;

/* Make the list */
          total = 0;
          for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
          }
          dt[total].text = 0; /* terminate array */

/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

        }
        break;

      case OSD_KEY_UP:
        if (s > 0)
          s--;
        else{
          s = total - 1;
/* JRT5 Fixes Blank List When <UP> Hit With Exactly 10 Entries */
/*          if(LoadedCheatTotal < 10)*/
          if(LoadedCheatTotal <= MAX_DISPLAYCHEATS)    /* JCK 980917 */
/* JRT5 */
            break;

/* Top of the list, page up */
          if(Index == 0)
/* JRT5 Fixes Blank List When <UP> Hit With Exactly 10 Entries */
/*            Index = (LoadedCheatTotal/10)*10;*/
              /* Index = ( LoadedCheatTotal - 1 ); */
		Index = ((LoadedCheatTotal-1)/MAX_DISPLAYCHEATS)*MAX_DISPLAYCHEATS;    /* JCK 980917 */
/* JRT5 */
          else if(Index > MAX_DISPLAYCHEATS)    /* JCK 980917 */
            Index -= MAX_DISPLAYCHEATS;
          else
            Index = 0;

/* Refresh the list */
          total = 0;
          for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
          }
          dt[total].text = 0; /* terminate array */
          s = total-1;

/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

        }
        break;

	/* JCK 981006 BEGIN */

      case OSD_KEY_HOME:
        Index = 0;  /* No more test */ /* JCK 981016 */
/* Make the list */
        total = 0;
        for (i = 0;i < MAX_DISPLAYCHEATS;i++)
	  {
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
        }
        dt[total].text = 0; /* terminate array */
        s = 0;
/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

	  break;

      case OSD_KEY_END:
	  Index = ((LoadedCheatTotal-1)/MAX_DISPLAYCHEATS)*MAX_DISPLAYCHEATS; /* No more test */ /* JCK 981016 */
/* Refresh the list */
        total = 0;
        for (i = 0;i < MAX_DISPLAYCHEATS;i++)
	  {
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
        }
        dt[total].text = 0; /* terminate array */
        s = total-1;
/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

	  break;

      case OSD_KEY_PGDN:
        if (s+Index >= LoadedCheatTotal - MAX_DISPLAYCHEATS)
	  {
		Index = ((LoadedCheatTotal-1)/MAX_DISPLAYCHEATS)*MAX_DISPLAYCHEATS;
		s = (LoadedCheatTotal - 1) - Index;
	  }
	  else
		Index += MAX_DISPLAYCHEATS;
/* Make the list */
        total = 0;
        for (i = 0;i < MAX_DISPLAYCHEATS;i++)
	  {
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
        }
        dt[total].text = 0; /* terminate array */
/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

        break;

      case OSD_KEY_PGUP:
        if (s+Index <= MAX_DISPLAYCHEATS)
	  {
		Index = 0;
		s = 0;
	  }
	  else
		Index -= MAX_DISPLAYCHEATS;
/* Make the list */
        total = 0;
        for (i = 0;i < MAX_DISPLAYCHEATS;i++)
	  {
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
        }
        dt[total].text = 0; /* terminate array */
/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

        break;

	/* JCK 981006 END */

      case OSD_KEY_INSERT:
/* Add a new empty cheat */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
/* JRT5 Print Message If Cheat List Is Full */
        if(LoadedCheatTotal > MAX_LOADEDCHEATS-1){    /* JCK 980917 */
					xprintf( 0, 0, EndY, "(Cheat List Is Full.)" );    /* JCK 981207 */
          break;
        }
/* JRT5 */
        LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
        LoadedCheatTable[LoadedCheatTotal].Special = 0;
        LoadedCheatTable[LoadedCheatTotal].Count = 0;
        LoadedCheatTable[LoadedCheatTotal].Address = 0;
        LoadedCheatTable[LoadedCheatTotal].Data = 0;
        strcpy(LoadedCheatTable[LoadedCheatTotal].Name,"---- New Cheat ----");
        LoadedCheatTable[LoadedCheatTotal].Minimum = 0;    /* JCK 981016 */
        LoadedCheatTable[LoadedCheatTotal].Maximum = 0;    /* JCK 981016 */
        LoadedCheatTotal++;
        goto HardRefresh; /* I know...  */
        break;

      case OSD_KEY_DEL:
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
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
		  CheatTable[i].Minimum = CheatTable[i+1].Minimum;    /* JCK 981016 */
		  CheatTable[i].Maximum = CheatTable[i+1].Maximum;    /* JCK 981016 */
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
	    LoadedCheatTable[i].Minimum = LoadedCheatTable[i+1].Minimum;    /* JCK 981016 */
	    LoadedCheatTable[i].Maximum = LoadedCheatTable[i+1].Maximum;    /* JCK 981016 */
        }
        LoadedCheatTotal--;

/* Refresh the list */
        total = 0;
        for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
          if(Index+i >= LoadedCheatTotal)
            break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

          total++;
        }
        dt[total].text = 0; /* terminate array */
        if(total <= s)
          s = total-1;

        if(total == 0){
          if(Index != 0){
/* The page is empty so backup one page */
            if(Index == 0)
              /* Index = (LoadedCheatTotal/10)*10; */
              Index = ((LoadedCheatTotal-1)/MAX_DISPLAYCHEATS)*MAX_DISPLAYCHEATS;
            else if(Index > MAX_DISPLAYCHEATS)    /* JCK 980917 */
              Index -= MAX_DISPLAYCHEATS;
            else
              Index = 0;

/* Make the list */
            total = 0;
            for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
              if(Index+i >= LoadedCheatTotal)
                break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

              total++;
            }
            dt[total].text = 0; /* terminate array */
            s = total-1;
          }
        }

/* Redisplay all */
        osd_clearbitmap(Machine->scrbitmap);
        /* DisplayActiveCheats(x,y); */ /* Removed by JCK 981030 */

        break;

      case OSD_KEY_F1:
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        if(LoadedCheatTotal == 0)
          break;
        if ((f = fopen(cheatfile,"a")) != 0)    /* JCK 980917 */
        {
		/* JRT6 - modified JB 980424 */
        	/* form fmt string, adjusting length of address field for cpu address range */
		/* JCK 091027 BEGIN */
        	switch ((ADDRESS_BITS(LoadedCheatTable[s+Index].CpuNo)+3) >> 2)
        	{
        		case 4:
        			strcpy (fmt, "%s:%d:%04X:%02X:%d:%s\n");
        			break;
        		case 5:
        			strcpy (fmt, "%s:%d:%05X:%02X:%d:%s\n");
        			break;
        		case 6:
        			strcpy (fmt, "%s:%d:%06X:%02X:%d:%s\n");
        			break;
        		case 7:
        			strcpy (fmt, "%s:%d:%07X:%02X:%d:%s\n");
        			break;
        		case 8:
        		default:
        			strcpy (fmt, "%s:%d:%08X:%02X:%d:%s\n");
        			break;
        	}
		/* JCK 091027 END */

			#ifdef macintosh
				strcat (fmt, "\r");	/* force DOS-style line enders */
			#endif

			fprintf (f, fmt, Machine->gamedrv->name, LoadedCheatTable[s+Index].CpuNo,
				LoadedCheatTable[s+Index].Address, LoadedCheatTable[s+Index].Data,
				LoadedCheatTable[s+Index].Special, LoadedCheatTable[s+Index].Name);

			xprintf (0, 0, EndY, "Saved to File %s ....", cheatfile); /* JCK 981207 */
			fclose (f);
        }
        break;

      case OSD_KEY_F2:	/* Add to watch list */
		while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

		/* JCK 981211 BEGIN */
		if (LoadedCheatTable[s+Index].Special == 99)
		{
			xprintf (0, 0, EndY, "Comment NOT Added as Watch");
			break;
		}
		/* JCK 981211 END */

      	if (LoadedCheatTable[s+Index].CpuNo<cpu_gettotalcpu())	/* watches are for all cpus */ /* JCK 981008 */
      	{
	        for (i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
	        {
			/* if (Watches[i] == MAX_ADDRESS(0)) */	/* JRT6/JB */
			if (Watches[i] == MAX_ADDRESS(WatchesCpuNo[i]))    /* JCK 981009 */
			{
            		WatchesCpuNo[i] = LoadedCheatTable[s+Index].CpuNo;    /* JCK 981008 */
            		Watches[i] = LoadedCheatTable[s+Index].Address;

				/* JCK 981204 BEGIN */
				strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
				sprintf (buf, fmt, Watches[i]);
				xprintf (0, 0, EndY, "%s Added as Watch %d",buf,i+1);    /* JCK 981207 */
				/* JCK 981204 END */

            		WatchesFlag = 1;
				WatchEnabled = 1;    /* JCK 980917 */
            		break;
          		}
        	  }
        	}
        break;
      case OSD_KEY_F3:
/* Edit current cheat */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
/* JRT6 */

		/* JCK 981211 BEGIN */
		if( LoadedCheatTotal )								/* If At Least One Cheat*/
		{
			if (LoadedCheatTable[s+Index].Special == 99)
				EditComment(s+Index, EndY);							/* Edit It*/
			else
				EditCheat(s+Index);								/* Edit It*/
		}
		/* JCK 981211 END */

/* Make the list */
          total = 0;
          for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
            if(Index+i >= LoadedCheatTotal)
              break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

            total++;
          }
          dt[total].text = 0; /* terminate array */

/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

        break;
/* JRT6 */

/* JCK 981006 BEGIN */

      case OSD_KEY_F4:
/* Copy the current cheat */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        if(LoadedCheatTotal == 0)
          break;
        if(LoadedCheatTotal > MAX_LOADEDCHEATS-1)
	  {
	    xprintf( 0, 0, EndY, "(Cheat List Is Full.)" );    /* JCK 981207 */
          break;
        }
        LoadedCheatTable[LoadedCheatTotal].Count = 0;
        LoadedCheatTable[LoadedCheatTotal].CpuNo = LoadedCheatTable[s+Index].CpuNo;
        LoadedCheatTable[LoadedCheatTotal].Address = LoadedCheatTable[s+Index].Address;
        LoadedCheatTable[LoadedCheatTotal].Data = LoadedCheatTable[s+Index].Data;
        LoadedCheatTable[LoadedCheatTotal].Special = LoadedCheatTable[s+Index].Special;
        strcpy(LoadedCheatTable[LoadedCheatTotal].Name,LoadedCheatTable[s+Index].Name);
        LoadedCheatTable[LoadedCheatTotal].Minimum = LoadedCheatTable[s+Index].Minimum;    /* JCK 981016 */
        LoadedCheatTable[LoadedCheatTotal].Maximum = LoadedCheatTable[s+Index].Maximum;    /* JCK 981016 */
        LoadedCheatTotal++;
        goto HardRefresh; /* I know...  */
        break;

/* JCK 981006 END */

/* JCK 981016 BEGIN */

      case OSD_KEY_F5:
/* Rename the cheatfile and reload the database */
		while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
		done = RenameCheatFile();
		if (done == 1)
		{
	  		LoadCheat();
		}
		done = 0;
        	goto HardRefresh; /* I know...  */
        	break;

/* JCK 981016 END */

/* JCK 981006 BEGIN */

      case OSD_KEY_F6:
/* Save all loaded cheats do the file */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        if(LoadedCheatTotal == 0)
          break;
        if ((f = fopen(cheatfile,"a")) != 0)
        {
	      for(i=0;i<LoadedCheatTotal;i++)
		{
	        	/* form fmt string, adjusting length of address field for cpu address range */
			/* JCK 091027 BEGIN */
      	  	switch ((ADDRESS_BITS(LoadedCheatTable[i].CpuNo)+3) >> 2)
        		{
        			case 4:
        				strcpy (fmt, "%s:%d:%04X:%02X:%d:%s\n");
	        			break;
      	  		case 5:
        				strcpy (fmt, "%s:%d:%05X:%02X:%d:%s\n");
        				break;
	        		case 6:
      	  			strcpy (fmt, "%s:%d:%06X:%02X:%d:%s\n");
        				break;
        			case 7:
        				strcpy (fmt, "%s:%d:%07X:%02X:%d:%s\n");
	        			break;
      	  		case 8:
        			default:
        				strcpy (fmt, "%s:%d:%08X:%02X:%d:%s\n");
        				break;
	        	}
			/* JCK 091027 END */
			#ifdef macintosh
				strcat (fmt, "\r");	/* force DOS-style line enders */
			#endif
			fprintf (f, fmt, Machine->gamedrv->name, LoadedCheatTable[i].CpuNo,
				LoadedCheatTable[i].Address, LoadedCheatTable[i].Data,
				LoadedCheatTable[i].Special, LoadedCheatTable[i].Name);
		}
		fclose (f);
        }
        xprintf (0, 0, EndY, "%d Cheats Saved to File %s ....",	LoadedCheatTotal, cheatfile); /* JCK 981207 */
        break;

      case OSD_KEY_F7:
/* Remove all active cheats from the list */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        if(LoadedCheatTotal == 0)
          break;
	  CheatTotal=0;
        /* DisplayActiveCheats(x,y); */ /* Removed by JCK 981030 */

		/* JCK 981203 BEGIN */
/* Make the list */
          total = 0;
          for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
            if(Index+i >= LoadedCheatTotal)
              break;
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
            total++;
          }
          dt[total].text = 0; /* terminate array */
/* Clear old list */
		ClearArea(1, StartY, EndY);    /* JCK 981207 */

		/* JCK 981203 END */

        break;

/* JCK 981006 END */

/* JCK 981016 BEGIN */

      case OSD_KEY_F8:
/* Reload the database */
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
	  LoadCheat();
        goto HardRefresh; /* I know...  */
        break;

      case OSD_KEY_F9:
/* Rename the cheatfile */
		while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
		done = RenameCheatFile();
		done = 0;
		goto HardRefresh; /* I know...  */
		break;

/* JCK 981016 END */

/* JRT5 Invoke Cheat List Help */
	case OSD_KEY_F10:
		while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
		/* JCK 981016 BEGIN */
    		if(LoadedCheatTotal == 0)
			CheatListHelpEmpty();
		else
			CheatListHelp();
		/* JCK 981016 END */
		break;
/* JRT5 */

/* JCK 981207 BEGIN */

      case OSD_KEY_F12:
/* Display info about a cheat */
		while (osd_key_pressed(key)) ; /* wait for key release */

		/* JCK 981211 BEGIN */
		if (LoadedCheatTable[s+Index].Special == 99)
		{
			xprintf (0, 0, EndY, "This is a Comment");
			break;
		}
		/* JCK 981211 END */

		strcpy(fmt, pShortAddrTemplate(LoadedCheatTable[s+Index].CpuNo));
		if (ManyCpus)
		{
			strcat(fmt," (%01X) = %02X [Type %d]");
			sprintf (buf, fmt, LoadedCheatTable[s+Index].Address, LoadedCheatTable[s+Index].CpuNo,
					LoadedCheatTable[s+Index].Data,LoadedCheatTable[s+Index].Special);
		}
		else
		{
			strcat(fmt," = %02X [Type %d]");
			sprintf (buf, fmt, LoadedCheatTable[s+Index].Address,
					LoadedCheatTable[s+Index].Data,LoadedCheatTable[s+Index].Special);
		}
		xprintf (0, 0, EndY, "%s", buf);
		break;

/* JCK 981207 END */

  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
      case OSD_KEY_ENTER:
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        if(total == 0)
          break;

		/* JCK 981211 BEGIN */
		if (LoadedCheatTable[s+Index].Special == 99)
		{
			xprintf (0, 0, EndY, "Comment NOT Activated");
			break;
		}
		/* JCK 981211 END */

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
		  CheatTable[i].Minimum = CheatTable[i+1].Minimum;    /* JCK 981016 */
		  CheatTable[i].Maximum = CheatTable[i+1].Maximum;    /* JCK 981016 */
            }
            CheatTotal--;
            flag = 1;
            break;
          }
        }

/* No more than MAX_ACTIVECHEATS cheat at the time */
        if(CheatTotal > MAX_ACTIVECHEATS-1){    /* JCK 980917 */
					xprintf( 0, 0, EndY, "(Limit Of Active Cheats)" );    /* JCK 981207 */
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
          CheatTable[CheatTotal].Minimum = LoadedCheatTable[s+Index].Minimum;    /* JCK 981016 */
          CheatTable[CheatTotal].Maximum = LoadedCheatTable[s+Index].Maximum;    /* JCK 981016 */

	    /* JCK 981020 - 981023 BEGIN */
	    ValTmp = 0;
	    if ((CheatTable[CheatTotal].Special>=60) && (CheatTable[CheatTotal].Special<=75))
	    {
			CheatTable[CheatTotal].Data = RD_GAMERAM(CheatTable[CheatTotal].CpuNo, CheatTable[CheatTotal].Address);
			BCDOnly = 	((CheatTable[CheatTotal].Special==63) ||
					 (CheatTable[CheatTotal].Special==64) ||
					 (CheatTable[CheatTotal].Special==65) ||
					 (CheatTable[CheatTotal].Special==73) ||
					 (CheatTable[CheatTotal].Special==74) ||
					 (CheatTable[CheatTotal].Special==75));
			ZeroPoss =	((CheatTable[CheatTotal].Special==60) ||
					 (CheatTable[CheatTotal].Special==63) ||
					 (CheatTable[CheatTotal].Special==70) ||
					 (CheatTable[CheatTotal].Special==73));

			/* JCK 981023 BEGIN */
			ValTmp = SelectValue(CheatTable[CheatTotal].Data, BCDOnly, ZeroPoss, 0, 0,
							CheatTable[CheatTotal].Minimum, CheatTable[CheatTotal].Maximum,
							"%03d", "Enter the new Value:", 1, FIRSTPOS + 3 * FontHeight);
			if (ValTmp != NOVALUE)
				CheatTable[CheatTotal].Data = ValTmp;
			/* JCK 981023 END */
	    }
	    if (ValTmp != NOVALUE)
	    {
		/* Removed by JCK 981030 */
		/*
	    	if ((CheatTable[CheatTotal].Special>=60) && (CheatTable[CheatTotal].Special<=65))
          		CheatTable[CheatTotal].Special = 60;
	    	if ((CheatTable[CheatTotal].Special>=70) && (CheatTable[CheatTotal].Special<=75))
          		CheatTable[CheatTotal].Special = 70;
		*/
            CheatTotal++;
            CheatEnabled = 1;
            he_did_cheat = 1;
	    }
	    /* JCK 981020 - 981023 END */

        }

		osd_clearbitmap(Machine->scrbitmap);
		/* DisplayActiveCheats(x,y); */ /* Removed by JCK 981030 */
/* Make the list */
            total = 0;
            for (i = 0;i < MAX_DISPLAYCHEATS;i++){    /* JCK 980917 */
              if(Index+i >= LoadedCheatTotal)
                break;

		/* JCK 981030 BEGIN */
            /* dt[i].text = LoadedCheatTable[i+Index].Name; */
		AddCheckToName(i+Index, str2[ i ]);
		dt[i].text = str2[i];
		/* JCK 981030 END */

              total++;
            }
            dt[total].text = 0; /* terminate array */
		break;

      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
  	  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

  /* clear the screen before returning */
  osd_clearbitmap(Machine->scrbitmap);

}


/* JCK 981023 BEGIN */
int StartSearchHeader(void)
{
  int y = FIRSTPOS;

  osd_clearbitmap(Machine->scrbitmap);

  xprintf(0, 0, y, "<F10>: Show Help");
  y += 2*FontHeight;
  xprintf(0, 0,y,"Choose One Of The Following:");
  y += 2*FontHeight;
  return(y);
}

int ContinueSearchHeader(void)
{
  int y = FIRSTPOS;

  osd_clearbitmap(Machine->scrbitmap);

  xprintf(0, 0, y, "<F1>: Start A New Search");
  y += 2*FontHeight;

  switch (CurrentMethod)
  {
	case Method_1:
		xprintf(0, 0,y,"Enter The New Value:");
		break;
	case Method_2:
		xprintf(0, 0,y,"Enter How Much The Value");
		y += FontHeight;
		if ( iCheatInitialized )
		{
			xprintf(0, 0,y,"Has Changed Since You");
			y += FontHeight;
			xprintf(0, 0,y,"Started The Search:");
		}
		else
		{
			xprintf(0, 0,y,"Has Changed Since The");
			y += FontHeight;
			xprintf(0, 0,y,"Last Check:");
		}
		break;
	case Method_3:
		xprintf(0, 0,y,"Choose The Expression That");
		y += FontHeight;
		xprintf(0, 0,y,"Specifies What Occured Since");
		y += FontHeight;
		if ( iCheatInitialized )
			xprintf(0, 0,y,"You Started The Search:");
		else
			xprintf(0, 0,y,"The Last Check:");
		break;
	case Method_4:
		xprintf(0, 0,y,"Choose One Of The Following:");
		break;
	case Method_5:
		xprintf(0, 0,y,"Choose One Of The Following:");
		break;
  }

  y += 2*FontHeight;
  return(y);
}

int ContinueSearchMatchHeader(int count)
{
  int y = FIRSTPOS;
  char *str;

  xprintf(0, 0,y,"Matches Found: %d",count);
  if (count > MAX_MATCHES)    /* JCK 980917 */
	str = "Here Are some Matches:";
  else
	if (count != 0)
		str = "Here Is The List:";
	else
		str = "(No Matches Found)";
  y += 2*FontHeight;
  xprintf(0, 0,y,"%s",str);
  return(y);
}

int ContinueSearchMatchFooter(int count, int idx, int y)
{
  int i = 0;
  int FreeWatch = 0;

  y += FontHeight;
  if (LoadedCheatTotal > MAX_LOADEDCHEATS-1)
	xprintf(0, 0,y,"(Cheat List Is Full.)");
  y += 2 * FontHeight;

  if ((count > MAX_MATCHES) && (idx != 0))
	xprintf(0, 0,y,"<HOME>: First page");
  else
	ClearTextLine(1, y);    /* JCK 981207 */
  y += FontHeight;

  if (count > idx+MAX_MATCHES)
	xprintf(0, 0,y,"<PAGE DOWN>: Scroll");
  else
	ClearTextLine(1, y);    /* JCK 981207 */
  y += FontHeight;

  /* JCK 981030 BEGIN */
  for (i=0;i<MAX_WATCHES;i++)
	if (Watches[i] == MAX_ADDRESS(WatchesCpuNo[i]))
		FreeWatch = 1;
  if (FreeWatch)
	xprintf(0, 0,y,"<F2>: Add Entry To Watches");
  else
	ClearTextLine(1, y);    /* JCK 981207 */
  y += FontHeight;
  /* JCK 981030 END */

  if (LoadedCheatTotal < MAX_LOADEDCHEATS)
  {
	xprintf(0, 0,y,"<F1>: Add Entry To List");
	y += FontHeight;
	xprintf(0, 0,y,"<F6>: Add All To List");
	y += 2*FontHeight;
  }
  else
  {
	ClearArea(1, y, y + 3 * FontHeight);    /* JCK 981207 */
	/* for (i=y; i< ( y + 3 * FontHeight + 1 ); i++) memset (Machine->scrbitmap->line[i], 0, MachWidth); */
  }
  return(y);
}
/* JCK 981023 END */

/*****************
 * Start a cheat search
 * If the method 1 is selected, ask the user a number
 * In all cases, backup the ram.
 *
 * Ask the user to select one of the following:
 *  1 - Lives or other number (byte) (exact)       ask a start value , ask new value
 *  2 - Timers (byte) (+ or - X)                   nothing at start, ask +-X
 *  3 - Energy (byte) (less, equal or greater)     nothing at start, ask less, equal or greater
 *  4 - Status (bit)  (true or false)              nothing at start, ask same or opposite
 *  5 - Slow but sure (Same as start or different) nothing at start, ask same or different
 *
 * Another method is used in the Pro action Replay the Energy method
 *  you can tell that the value is now 25%/50%/75%/100% as the start
 *  the problem is that I probably cannot search for exactly 50%, so
 *  that do I do? search +/- 10% ?
 * If you think of other way to search for codes, let me know.
 */

void StartSearch(void)
{
  /* JCK 981023 BEGIN */
  int i = 0;
  int y;
  int s,key,done,count;
  int total;

  char *paDisplayText[] = {
		"Lives, Or Some Other Value",
		"Timers (+/- Some Value)",
		"Energy (Greater Or Less)",
		"Status (A Bit Or Flag)",
		"Slow But Sure (Changed Or Not)",
		"",
		"Return To Cheat Menu",
		0 };

  struct DisplayText dt[10];

  y = StartSearchHeader();
  /* JCK 981023 END */

  /* JCK 981023 BEGIN */
  total = CreateMenu(paDisplayText, dt, y);
  y = dt[total-1].y + ( 3 * FontHeight );

  count = 1;

  s = 0;
  done = 0;
  do
  {
	key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
	switch (key)
	{
		case OSD_KEY_F10:
			StartSearchHelp();								/* Show Help*/
			done = 0;
			y = StartSearchHeader();    /* JCK 981023 */
			break;

		case OSD_KEY_ENTER:
			switch (s)
			{
				case 0:
					SaveMethod = CurrentMethod;
					CurrentMethod = Method_1;
					done = 1;
					break;

				case 1:
					SaveMethod = CurrentMethod;
					CurrentMethod = Method_2;
					done = 1;
					break;

				case 2:
					SaveMethod = CurrentMethod;
					CurrentMethod = Method_3;
					done = 1;
					break;

				case 3:
					SaveMethod = CurrentMethod;
					CurrentMethod = Method_4;
					done = 1;
					break;

				case 4:
					SaveMethod = CurrentMethod;
					CurrentMethod = Method_5;
					done = 1;
					break;

				case 6:
					done = 2;
					break;
			}
			break;
	}
  } while (done == 0);
  /* JCK 981023 END */

  osd_clearbitmap(Machine->scrbitmap);

  /* User select to return to the previous menu */
  if (done == 2)
	return;

  /* JCK 981006 BEGIN */
  /* if(cpu_gettotalcpu() > 1) */
  if (ManyCpus)    /* JCK 981022 */
  {
	/* JCK 981023 BEGIN */
	ValTmp = SelectValue(0, 0, 1, 1, 0, 0, cpu_gettotalcpu()-1,
					"%01X", "Enter CPU To Search In:", 1, FIRSTPOS + 3 * FontHeight);
	osd_clearbitmap(Machine->scrbitmap);
	if (ValTmp == NOVALUE)
	{
		CurrentMethod = SaveMethod;
		return;
	}
	s = ValTmp;
	/* JCK 981023 END */
  }
  else
  {
	s = 0;
  }
  SearchCpuNo = s;
/* JCK 981006 END */

  osd_clearbitmap(Machine->scrbitmap);

  /* If the method 1 is selected, ask for a number */
  if(CurrentMethod == Method_1)
  {
	/* JCK 981023 BEGIN */
	ValTmp = SelectValue(0, 0, 1, 1, 1, 0, 0xFF,
					"%03d  (0x%02X)", "Enter Value To Search For:", 1, FIRSTPOS + 3 * FontHeight);
	osd_clearbitmap(Machine->scrbitmap);
	if (ValTmp == NOVALUE)
	{
		return;
		CurrentMethod = SaveMethod;
	}
	s = ValTmp;
	/* JCK 981023 END */
  }
  else
  {
	s = 0;    /* JCK 981023 */
  }
  StartValue = s;    /* JCK 981023 */

  /* JB 980407 */
  /* build ram tables to match game ram */
  /* JCK 981206 BEGIN */
  if (SearchCpuNoOld != SearchCpuNo)
  {
	if (!build_tables ())    /* JCK 981208 */
        	SearchCpuNoOld = SearchCpuNo;
      else
	{
		CurrentMethod = 0;
		return;
	}
  }
  /* JCK 981206 END */

  y = FIRSTPOS + (4 * FontHeight);

  ShowSearchInProgress();    /* JCK 981210 */

  /* Backup the ram */
  backup_ram (StartRam);
  backup_ram (BackupRam);
  memset_ram (FlagTable, 0xFF); /* At start, all location are good */

  /* Flag the location that match the initial value if method 1 */
  if (CurrentMethod == Method_1)
  {
	/* JCK 981210 BEGIN */
	struct ExtMemory *ext;
      count = 0;
	for (ext = FlagTable; ext->data; ext++)
	{
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
                  {
				if (	(RD_GAMERAM(SearchCpuNo, i+ext->start) != s) 	&&
					(RD_GAMERAM(SearchCpuNo, i+ext->start) != s-1)	)
					ext->data[i] = 0;
                        else
					count ++;
                  }
	}
	/* JCK 981210 END */

	HideSearchInProgress();    /* JCK 981210 */
	xprintf(0, 0,y,"Matches Found: %d",count);
  }
  else
  {
	HideSearchInProgress();    /* JCK 981210 */
	xprintf(0, 0,y,"Search Initialized.");
	iCheatInitialized = 1;
  }

  /* JCK 981023 BEGIN */
  if (count == 0)
	CurrentMethod = 0;
  /* JCK 981023 END */

  y += 4*FontHeight;
  xprintf(0, 0,y,"Press A Key To Continue...");
  key = osd_read_keyrepeat(0);
  while (osd_key_pressed(key)) ; /* wait for key release */
  osd_clearbitmap(Machine->scrbitmap);
}

void ContinueSearch(void)
{
  char *str;
  char str2[12][80];
  int i,j,x,y,count,s,key,done;
  struct DisplayText dt[20];
  int total;
  int Continue;
  struct ExtMemory *ext;	/* JB 980407 */
  struct ExtMemory *ext_br;	/* JB 980407 */
  struct ExtMemory *ext_sr;	/* JB 980407 */

  /* JCK 981023 BEGIN */
  int Index = 0;
  int yFoot,yTemp,countAdded;
  /* JCK 981023 END */

  char fmt[20];    /* JCK 981022 */

  /* JCK 981125 BEGIN */
  char buf[20];
  char *ptr;
  int TrueAddr, TrueData;
  /* JCK 981125 END */

  osd_clearbitmap(Machine->scrbitmap);

  if (CurrentMethod == 0)
  {
	StartSearch();
	return;
  }

  count = 0;
  y = ContinueSearchHeader();    /* JCK 981023 */

  /******** Method 1 ***********/
  /* Ask new value if method 1 */
  if (CurrentMethod == Method_1)
  {
	/* JCK 981023 BEGIN */
	ValTmp = SelectValue(StartValue, 0, 1, 1, 1, 0, 0xFF, "%03d  (0x%02X)", "", 0, y);
	osd_clearbitmap(Machine->scrbitmap);
	if (ValTmp == NOVALUE)
	        return;
	s = ValTmp;
	/* JCK 981023 END */

	StartValue = s; /* Save the value for when continue */

	ShowSearchInProgress();    /* JCK 981210 */

	for (ext = FlagTable; ext->data; ext++)
	{
		/* JCK 981210 BEGIN */
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
                  {
				if (	(RD_GAMERAM(SearchCpuNo, i+ext->start) != s) 	&&
					(RD_GAMERAM(SearchCpuNo, i+ext->start) != s-1)	)
					ext->data[i] = 0;
                        else
					count ++;
                  }
		/* JCK 981210 END */
	}
  }


  /******** Method 2 ***********/
  /* Ask new value if method 2 */
  if (CurrentMethod == Method_2)
  {
	/* JCK 981023 BEGIN */
	ValTmp = SelectValue(StartValue, 0, 1, 0, 0, -127, 128, "%+04d", "", 0, y);
	osd_clearbitmap(Machine->scrbitmap);
	if (ValTmp == NOVALUE)
	        return;
	s = ValTmp;
	/* JCK 981023 END */

	/* JRT5 For Different Text Depending On Start/Continue Search */
	iCheatInitialized = 0;
	/* JRT5 */

	StartValue = s; /* Save the value for when continue */

	ShowSearchInProgress();    /* JCK 981210 */

	for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
	{
		/* JCK 981210 BEGIN */
		for (i=0; i <= ext->end - ext->start; i++)
			if (ext->data[i] != 0)
                  {
				if (RD_GAMERAM(SearchCpuNo, i+ext->start) != (ext_br->data[i] + s))
					ext->data[i] = 0;
                        else
					count ++;
                  }
		/* JCK 981210 END */
	}

	/* Backup the ram because we ask how much the value changed */
      /* since the last time,	not in relation of the start value. */
	backup_ram (BackupRam);	/* JB 980407 */
  }


  /******** Method 3 ***********/
  if (CurrentMethod == Method_3)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] = {
		"New Value is Less",
		"New Value is Equal",
		"New Value is Greater",
		"",
		"Return To Cheat Menu",
		0 };

	total = CreateMenu(paDisplayText, dt, y);
	y = dt[total-1].y + ( 3 * FontHeight );

	s = 0;
	done = 0;
	do
	{
		key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
		switch (key)
		{
			case OSD_KEY_F1:
				StartSearch();
				return;
				break;

			case OSD_KEY_ENTER:
				if (s == total-1)
					done = 2;
				break;
		}
	} while (done == 0);
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;

	ShowSearchInProgress();    /* JCK 981210 */

	if (s == 0)
	{
		/* User select that the value is now smaller, then clear the flag
			of the locations that are equal or greater at the backup */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
                  	{
					if (RD_GAMERAM(SearchCpuNo, i+ext->start) >= ext_br->data[i])
						ext->data[i] = 0;
                        	else
						count ++;
                  	}
			/* JCK 981210 END */
		}
	}
	else if (s==1)
	{
		/* User select that the value is equal, then clear the flag
			of the locations that do not equal the backup */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
                  	{
					if (RD_GAMERAM(SearchCpuNo, i+ext->start) != ext_br->data[i])
						ext->data[i] = 0;
                        	else
						count ++;
                  	}
			/* JCK 981210 END */
		}
	}
	else
	{
		/* User select that the value is now greater, then clear the flag
			of the locations that are equal or smaller */
		for (ext = FlagTable, ext_br = BackupRam; ext->data; ext++, ext_br++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
                  	{
					if (RD_GAMERAM(SearchCpuNo, i+ext->start) <= ext_br->data[i])
						ext->data[i] = 0;
                        	else
						count ++;
                  	}
			/* JCK 981210 END */
		}
	}

	/* Backup the ram because we ask how much the value changed */
      /* since the last	time, not in relation of the start value. */
	backup_ram (BackupRam);	/* JB 980407 */
  }


  /******** Method 4 ***********/
  /* Ask if the value is the same as when we start or the opposite */
  if (CurrentMethod == Method_4)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] = {
		"Bit is Same as Start",
		"Bit is Opposite from Start",
		"",
		"Return To Cheat Menu",
		0 };

	total = CreateMenu(paDisplayText, dt, y);
	y = dt[total-1].y + ( 3 * FontHeight );

	s = 0;
	done = 0;
	do
	{
		key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
		switch (key)
		{
			case OSD_KEY_F1:
				StartSearch();
				return;
				break;

			case OSD_KEY_ENTER:
				if (s == total-1)
					done = 2;
		}
	} while (done == 0);
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;

	ShowSearchInProgress();    /* JCK 981210 */

	if (s == 0)
	{
		/* User select same as start */
		/* We want to keep a flag if a bit has not changed */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
                 	{
				if (ext->data[i] != 0)
				{
					j = RD_GAMERAM(SearchCpuNo, i+ext->start) ^ (ext_sr->data[i] ^ 0xFF);
					ext->data[i] = j & ext->data[i];
				}
				if (ext->data[i] != 0)
						count ++;
                 	}
			/* JCK 981210 END */
		}
	}
	else
	{
		/* User select opposite as start */
		/* We want to keep a flag if a bit has changed */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
                 	{
				if (ext->data[i] != 0)
				{
					j = RD_GAMERAM(SearchCpuNo, i+ext->start) ^ ext_sr->data[i];
					ext->data[i] = j & ext->data[i];
				}
				if (ext->data[i] != 0)
						count ++;
                 	}
			/* JCK 981210 END */
		}
	}
  }


  /******** Method 5 ***********/
  /* Ask if the value is the same as when we start or different */
  if(CurrentMethod == Method_5)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] = {
		"Memory is Same as Start",
		"Memory is Different from Start",
		"",
		"Return To Cheat Menu",
		0 };

	total = CreateMenu(paDisplayText, dt, y);
	y = dt[total-1].y + ( 3 * FontHeight );

	s = 0;
	done = 0;
	do
	{
		key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
		switch (key)
		{
			case OSD_KEY_F1:
				StartSearch();
				return;
				break;

			case OSD_KEY_ENTER:
				if (s == total-1)
					done = 2;
		}
	} while (done == 0);
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;

	ShowSearchInProgress();    /* JCK 981210 */

	if (s == 0)
	{
		/* Discard the locations that are different from when we started */
		/* The value must be the same as when we start to keep it */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
                  	{
					if (RD_GAMERAM(SearchCpuNo, i+ext->start) != ext_sr->data[i])
						ext->data[i] = 0;
                        	else
						count ++;
                  	}
			/* JCK 981210 END */
		}
	}
	else
	{
		/* Discard the locations that are the same from when we started */
		/* The value must be different as when we start to keep it */
		for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
		{
			/* JCK 981210 BEGIN */
			for (i=0; i <= ext->end - ext->start; i++)
				if (ext->data[i] != 0)
                  	{
					if (RD_GAMERAM(SearchCpuNo, i+ext->start) == ext_sr->data[i])
						ext->data[i] = 0;
                        	else
						count ++;
                  	}
			/* JCK 981210 END */
		}
	}
  }


  /* For all method:
    - Display how much location the search
    - Display them
    - The user can press F2 to add one to the watches
    - The user can press F1 to add one to the cheat list
    - The user can press F6 to add all to the cheat list */

  osd_clearbitmap(Machine->scrbitmap);

  countAdded = 0;    /* JCK 981023 */

  /* JCK 981023 BEGIN */
  y = ContinueSearchMatchHeader(count);

  if (count == 0)
  {
	y += 4*FontHeight;
	xprintf(0, 0,y,"Press A Key To Continue...");
	key = osd_read_keyrepeat(0);
	while (osd_key_pressed(key)) ; /* wait for key release */
	osd_clearbitmap(Machine->scrbitmap);
	CurrentMethod = 0;
	return;
  }
  /* JCK 981023 END */

  /* yFoot = y + FontHeight * (MAX_MATCHES + 3); */ /* JCK 981023 */
  yFoot = MachHeight - 8 * FontHeight;    /* JCK 981207 */

  y += 2*FontHeight;

  yTemp = y;

  total = 0;
  Continue = 0;

  /* JB 980407 */
  for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
  {
	for (i=0; i <= ext->end - ext->start; i++)
		if (ext->data[i] != 0)
		{
			/* JCK 981026 BEGIN */
			strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
			strcat(fmt," = %02X");
			/* JCK 981026 END */

			/* JCK 981210 BEGIN */
			TrueAddr = i+ext->start;
			TrueData = ext_sr->data[i];
			sprintf (str2[total], fmt, TrueAddr, TrueData);
			/* JCK 981210 END */

			dt[total].text = str2[total];
			dt[total].x = (MachWidth - FontWidth * strlen(dt[total].text)) / 2;    /* JCK 981026 */
			dt[total].y = y;
			dt[total].color = DT_COLOR_WHITE;
			total++;
			y += FontHeight;
			if (total >= MAX_MATCHES)    /* JCK 980917 */
			{
				Continue = i+ext->start;
				break;
			}
		}
  }

  dt[total].text = 0; /* terminate array */

  /* JCK 981023 BEGIN */
  yTemp = ContinueSearchMatchHeader(count);

  s = 0;
  done = 0;
  do
  {
	int Begin = 0;

	yTemp = ContinueSearchMatchFooter(count, Index, yFoot);

	key = SelectMenu (&s, dt, 1, 1, 0, total-1, 0, &done);

	ClearTextLine(1, yFoot);    /* JCK 981207 */

	switch (key)
	{
		case OSD_KEY_HOME:
			/* JCK 981023 BEGIN */
			if (count == 0)
				break;
			if (count <= MAX_MATCHES)
				break;
			if (Index == 0)
				break;

			osd_clearbitmap(Machine->scrbitmap);
			yTemp = ContinueSearchMatchHeader(count);

			s = 0;
			Index = 0;
			/* JCK 981023 END */

			total = 0;

			/* JB 980407 */
			Continue = 0;
			for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
			{
				for (i=0; i <= ext->end - ext->start; i++)
					if ((ext->data[i] != 0) && (total < MAX_MATCHES))    /* JCK 981023 */
					{
						/* JCK 981026 BEGIN */
						strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
						strcat(fmt," = %02X");
						/* JCK 981026 END */

						/* JCK 981210 BEGIN */
						TrueAddr = i+ext->start;
						TrueData = ext_sr->data[i];
						sprintf (str2[total], fmt, TrueAddr, TrueData);
						/* JCK 981210 END */

						dt[total].text = str2[total];
						dt[total].x = (MachWidth - FontWidth * strlen(dt[total].text)) / 2;    /* JCK 981026 */
						total++;
						if (total >= MAX_MATCHES)    /* JCK 980917 */
						{
							Continue = i+ext->start;
							break;
						}
					}
			}

			dt[total].text = 0; /* terminate array */
			break;

		case OSD_KEY_PGDN:
			/* JCK 981023 BEGIN */
			if (count == 0)
				break;
			if (count <= Index+MAX_MATCHES)
				break;

			osd_clearbitmap(Machine->scrbitmap);
			yTemp = ContinueSearchMatchHeader(count);

			s = 0;
			Index += MAX_MATCHES;
			/* JCK 981023 END */

			total = 0;

			/* JB 980407 */
			Begin = Continue+1;
			Continue = 0;

			for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
			{
				if (ext->start <= Begin && ext->end >= Begin)    /* JCK 981209 */
				{
					for (i = Begin - ext->start; i <= ext->end - ext->start; i++)
						if ((ext->data[i] != 0) && (total < MAX_MATCHES))    /* JCK 981023 */
						{
							/* JCK 981026 BEGIN */
							strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
							strcat(fmt," = %02X");
							/* JCK 981026 END */

							/* JCK 981210 BEGIN */
							TrueAddr = i+ext->start;
							TrueData = ext_sr->data[i];
							sprintf (str2[total], fmt, TrueAddr, TrueData);
							/* JCK 981210 END */

							dt[total].text = str2[total];
							dt[total].x = (MachWidth - FontWidth * strlen(dt[total].text)) / 2;    /* JCK 981026 */
							total++;
							if (total >= MAX_MATCHES)    /* JCK 980917 */
							{
								Continue = i+ext->start;
								break;
							}
						}
				}
                        /* JCK 981209 BEGIN */
				else if (ext->start > Begin)
				{
					for (i=0; i <= ext->end - ext->start; i++)
						if ((ext->data[i] != 0) && (total < MAX_MATCHES))    /* JCK 981023 */
						{
							/* JCK 981026 BEGIN */
							strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
							strcat(fmt," = %02X");
							/* JCK 981026 END */

							/* JCK 981210 BEGIN */
							TrueAddr = i+ext->start;
							TrueData = ext_sr->data[i];
							sprintf (str2[total], fmt, TrueAddr, TrueData);
							/* JCK 981210 END */

							dt[total].text = str2[total];
							dt[total].x = (MachWidth - FontWidth * strlen(dt[total].text)) / 2;    /* JCK 981026 */
							total++;
							if (total >= MAX_MATCHES)    /* JCK 980917 */
							{
								Continue = i+ext->start;
								break;
							}
						}
				}
                        /* JCK 981209 END */
			}

			dt[total].text = 0; /* terminate array */
			break;

		case OSD_KEY_F1:    /* JCK 981023 */
			while (osd_key_pressed(key)) ;

			if (count == 0)    /* JCK 981023 */
				break;
			/* Add the selected address to the LoadedCheatTable */
			if (LoadedCheatTotal < MAX_LOADEDCHEATS-1)    /* JCK 980917 */
			{
				LoadedCheatTable[LoadedCheatTotal].CpuNo = SearchCpuNo;    /* JCK 981008 */
				LoadedCheatTable[LoadedCheatTotal].Special = 0;
				LoadedCheatTable[LoadedCheatTotal].Count = 0;

				/* JCK 981125 BEGIN */
				strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
				strcpy(buf, dt[s].text);
				ptr = strtok(buf, "=");
				sscanf(ptr,"%X", &TrueAddr);
				ptr = strtok(NULL, "=");
				sscanf(ptr,"%02X", &TrueData);
				LoadedCheatTable[LoadedCheatTotal].Address = TrueAddr;
				LoadedCheatTable[LoadedCheatTotal].Data = TrueData;
				/* JCK 981125 END */

				/* JCK 981026 BEGIN */
				if (ManyCpus)
				{
					strcat(fmt," (%01X) = %02X");
					sprintf (str2[11], fmt, TrueAddr, SearchCpuNo, TrueData);    /* JCK 981125 */
				}
				else
				{
					strcat(fmt," = %02X");
					sprintf (str2[11], fmt, TrueAddr, TrueData);    /* JCK 981125 */
				}
				/* JCK 981026 END */
				strcpy(LoadedCheatTable[LoadedCheatTotal].Name,str2[11]);
				/* JCK 981023 BEGIN */
				LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
				LoadedCheatTable[LoadedCheatTotal].Maximum = 0xFF;
				/* JCK 981023 END */
				LoadedCheatTotal++;
				xprintf(0, 0,yFoot,"%s Added",dt[s].text);    /* JCK 981023 */
			}
			else
				xprintf(0, 0,yFoot,"%s Not Added",dt[s].text);    /* JCK 981023 */

			break;

		case OSD_KEY_F2:
			/* JCK 981030 BEGIN */
			while (osd_key_pressed(key)) ;

			for (j=0;j<MAX_WATCHES;j++)
			{
				if (Watches[j] == MAX_ADDRESS(WatchesCpuNo[j]))
				{
					/* JCK 981125 BEGIN */
					strcpy(buf, dt[s].text);
					ptr = strtok(buf, "=");
					sscanf(ptr,"%X", &TrueAddr);
					/* JCK 981125 END */

					WatchesCpuNo[j] = SearchCpuNo;
					Watches[j] = TrueAddr;    /* JCK 981125 */
					WatchesFlag = 1;
					WatchEnabled = 1;
					strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
					sprintf (str2[11], fmt, Watches[j]);
					xprintf(0, 0,yFoot,"%s Added as Watch %d",str2[11],j+1);
					break;
				}
			}

			break;
			/* JCK 981030 END */

		case OSD_KEY_F6:    /* JCK 981023 */
			while (osd_key_pressed(key)) ;

			if (count == 0)    /* JCK 981023 */
				break;
			countAdded = 0;    /* JCK 981023 */

			/* JB 980407 and JCK 980917 */
			for (ext = FlagTable, ext_sr = StartRam; ext->data && count<MAX_LOADEDCHEATS; ext++, ext_sr++)
			{
				for (i = 0; i <= ext->end - ext->start; i++)
				{
					if(LoadedCheatTotal > MAX_LOADEDCHEATS-1)    /* JCK 980917 */
						break;
					if (ext->data[i] != 0)
					{
						countAdded++;    /* JCK 981023 */
						LoadedCheatTable[LoadedCheatTotal].Special = 0;
						LoadedCheatTable[LoadedCheatTotal].Count = 0;
						LoadedCheatTable[LoadedCheatTotal].CpuNo = SearchCpuNo;    /* JCK 981008 */

						/* JCK 981210 BEGIN */
						TrueAddr = i+ext->start;
						TrueData = ext_sr->data[i];
						LoadedCheatTable[LoadedCheatTotal].Address = TrueAddr;
						LoadedCheatTable[LoadedCheatTotal].Data = TrueData;
						/* JCK 981210 END */

						/* JCK 981026 BEGIN */
						strcpy(fmt, pShortAddrTemplate(SearchCpuNo));
						if (ManyCpus)
						{
							strcat(fmt," (%01X) = %02X");
							sprintf (str2[11], fmt, TrueAddr, SearchCpuNo, TrueData);    /* JCK 981125 */
						}
						else
						{
							strcat(fmt," = %02X");
							sprintf (str2[11], fmt, TrueAddr, TrueData);    /* JCK 981125 */
						}
						/* JCK 981026 END */
						strcpy(LoadedCheatTable[LoadedCheatTotal].Name,str2[11]);
						/* JCK 981023 BEGIN */
						LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
						LoadedCheatTable[LoadedCheatTotal].Maximum = 0xFF;
						/* JCK 981023 END */
						LoadedCheatTotal++;
					}
				}
			}

			xprintf(0, 0,yFoot,"%d Added",countAdded);    /* JCK 981023 */

			break;

	}
  } while (done != 2);
  /* JCK 981023 END */

  osd_clearbitmap(Machine->scrbitmap);
}

/* JCK 981023 BEGIN */
void AddCpuToWatch(int NoWatch,char *buffer)
{
  char bufadd[10];

  sprintf(buffer, pShortAddrTemplate(WatchesCpuNo[NoWatch]), Watches[NoWatch]);
  if (ManyCpus)
  {
	sprintf (bufadd, "  (%01X)", WatchesCpuNo[NoWatch]);
	strcat(buffer,bufadd);
  }
}

/* JCK 981023 END */

/* JCK 981022 BEGIN */
int ChooseWatchHeader(void)
{
  int i = 0;
  char *paDisplayText[] = {
 		"<+>: +1 byte    <->: -1 byte",    /* JCK 981210 */
		"<1> ... <8>: +1 digit",    /* JCK 981027 */
		"<9>: Prev CPU  <0>: Next CPU",
		"<Delete>: disable a watch",
		"<Enter>: copy previous watch",
		"<I><J><K><L>: move watch pos.",
		"(All \"F\"s = watch disabled)",
		"",
		"<F10>: Show Help + other keys",
		0 };

  struct DisplayText dt[20];

  while (paDisplayText[i])
  {
	if(i)
		dt[i].y = (dt[i - 1].y + FontHeight + 2);
	else
		dt[i].y = FIRSTPOS;    /* JCK 981207 */
	dt[i].color = DT_COLOR_WHITE;
	dt[i].text = paDisplayText[i];
	dt[i].x = (MachWidth - FontWidth * strlen(dt[i].text)) / 2;
	if(dt[i].x > MachWidth)
		dt[i].x = 0;
	i++;
  }
  dt[i].text = 0; /* terminate array */
  displaytext(dt,0,1);
  return(dt[i-1].y + ( 3 * FontHeight ));
}

int ChooseWatchFooter(int y)
{
  int i = 0;

  y += FontHeight;
  if (LoadedCheatTotal > MAX_LOADEDCHEATS-1)
	xprintf(0, 0,y,"(Cheat List Is Full.)");
  return(y);
}
/* JCK 981022 END */

void ChooseWatch(void)
{
  int i,s,y,key,done;
  int total;
  struct DisplayText dt[20];
  char str2[MAX_WATCHES+1][15];    /* JCK 981027 */
  char buf[40];
  char buf2[20];    /* JCK 981030 */

  int yFoot,yTemp,countAdded;    /* JCK 981022 */

  char fmt[20];    /* JCK 981022 */

  int j,OldCpuNo = 0;    /* JCK 981026 */

  int trueorientation;
  /* hack: force the display into standard orientation to avoid */
  /* rotating the user interface */
  trueorientation = Machine->orientation;
  Machine->orientation = ORIENTATION_DEFAULT;

  osd_clearbitmap(Machine->scrbitmap);

  /* JCK 981022 BEGIN */
  y = ChooseWatchHeader();
  /* yFoot = y + FontHeight * (MAX_WATCHES + 3); */
  yFoot = MachHeight - 2 * FontHeight;    /* JCK 981207 */
  /* JCK 981022 END */

  total = 0;

  for (i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
  {
	/* JRT1 10-23-97 BEGIN */
	/* JRT6/JB - modified JB 980424 */
	/* sprintf(str2[ i ], pShortAddrTemplate(0), Watches[i]); */

	/* JCK 981023 BEGIN */
	/* sprintf(str2[ i ], pShortAddrTemplate(WatchesCpuNo[i]), Watches[i]);
	sprintf (buf2, " %01X", WatchesCpuNo[i]);
	strcat(str2[ i ],buf2); */
	AddCpuToWatch(i, str2[ i ]);
	/* JCK 981023 END */

	/* JRT6/JB */
	dt[total].text = str2[ i ];
	/* JRT1 10-23-97 END */

	/* dt[total].x = (MachWidth - FontWidth * 5) / 2; */
	dt[total].x = (MachWidth - FontWidth * strlen(dt[total].text)) / 2;    /* JCK 981026 */
	dt[total].y = y;
	dt[total].color = DT_COLOR_WHITE;
	total++;
	y += FontHeight;
  }
  dt[total].text = 0; /* terminate array */

  s = 0;
  done = 0;
  do
  {
	/* Display a test to see where the watches are */
	buf[0] = 0;
	for(i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
	{
		/* JCK 981026 BEGIN */
		if (Watches[i] > MAX_ADDRESS(WatchesCpuNo[i]))
		{
			Watches[i] = MAX_ADDRESS(WatchesCpuNo[i]);
			AddCpuToWatch(i, str2[ i ]);
		}
		/* JCK 981026 END */
		if (Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))    /* JCK 981009 */
		{
			/* sprintf (buf2, "%02X ", RD_GAMERAM(0,Watches[i])); */	/* JB 980407 */
			sprintf (buf2, "%02X ", RD_GAMERAM(WatchesCpuNo[i],Watches[i]));    /* JCK 981008 */
			strcat(buf,buf2);
		}
	}

	#if 0
	/* JRT1 10-23-97 BEGIN */
	/**/
	/* Clear Old Watch Space*/
	/**/
	for( i = ( WatchY ? WatchY - 1 : WatchY ); i < ( WatchY + FontHeight + 1 ); i++ )      /* For Watch Area*/
	{
		memset( Machine -> scrbitmap -> line[ i ], 0, Machine -> scrbitmap -> width );      /* Clear Old Drawing*/
	}
	#endif

	ClearArea(0, ( WatchY ? WatchY - 1 : WatchY ), WatchY + FontHeight + 1);    /* JCK 981207 */

	while( WatchX >= ( MachWidth - ( FontWidth * (int)strlen( buf ) ) ) )   /* If Going To Draw OffScreen*/
	{
		if( !WatchX-- )                   /* Adjust Drawing X Offset*/
			break;                      /* Stop If Too Small*/
	}
	/* JRT1 10-23-97 END */

	for (i = 0;i < (int)strlen(buf);i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
			WatchX+(i*FontWidth),WatchY,0,TRANSPARENCY_NONE,0);

	yTemp = ChooseWatchFooter(yFoot);    /* JCK 981022 */

	countAdded = 0;    /* JCK 981022 */

	key = SelectMenu (&s, dt, 1, 0, 0, total-1, 0, &done);    /* JCK 981023 */

	ClearTextLine(1, yFoot);    /* JCK 981207 */

	switch (key)
	{
		case OSD_KEY_J:
			if(WatchX != 0)
				WatchX--;
			break;

		case OSD_KEY_L:
			/* JRT1 10-23-97 BEGIN */
			if(WatchX <= ( MachWidth - ( FontWidth * (int)strlen( buf ) ) ) )
			/* JRT1 10-23-97 END */
				WatchX++;
			break;

		case OSD_KEY_K:                   /* (Minus Extra One)*/
			/* JRT1 10-23-97 BEGIN */
			if(WatchY <= (MachHeight-FontHeight) - 1 )
			/* JRT1 10-23-97 END */
				WatchY++;
			break;

		case OSD_KEY_I:
			if(WatchY != 0)
				WatchY--;
			break;

		case OSD_KEY_MINUS_PAD:
		case OSD_KEY_LEFT:
			if(Watches[ s ] <= 0)
				/* Watches[ s ] = MAX_ADDRESS(0); */
				Watches[ s ] = MAX_ADDRESS(WatchesCpuNo[ s ]);    /* JCK 981009 */
			else
				Watches[ s ]--;

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			break;

		case OSD_KEY_PLUS_PAD:
		case OSD_KEY_RIGHT:
			/* if(Watches[ s ] >= MAX_ADDRESS(0)) */
			if(Watches[ s ] >= MAX_ADDRESS(WatchesCpuNo[ s ]))    /* JCK 981009 */
				Watches[ s ] = 0;
			else
				Watches[ s ]++;
			/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			break;

		case OSD_KEY_PGDN:
			if(Watches[ s ] <= 0x100)
				/* Watches[ s ] |= (0xFFFFFF00 & MAX_ADDRESS(0)); */
				Watches[ s ] |= (0xFFFFFF00 & MAX_ADDRESS(WatchesCpuNo[ s ]));    /* JCK 981009 */
			else
				Watches[ s ] -= 0x100;
			/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			break;

		case OSD_KEY_PGUP:
			if(Watches[ s ] >= 0xFF00)
				/* Watches[ s ] |= (0xFFFF00FF & MAX_ADDRESS(0)); */
				Watches[ s ] |= (0xFFFF00FF & MAX_ADDRESS(WatchesCpuNo[ s ]));    /* JCK 981009 */
			else
				Watches[ s ] += 0x100;
			/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			break;

		/* JCK 981027 BEGIN */
		case OSD_KEY_8:
			if (ADDRESS_BITS(WatchesCpuNo[s]) < 29) break;
		case OSD_KEY_7:
			if (ADDRESS_BITS(WatchesCpuNo[s]) < 25) break;
		case OSD_KEY_6:
			if (ADDRESS_BITS(WatchesCpuNo[s]) < 21) break;
		case OSD_KEY_5:
			if (ADDRESS_BITS(WatchesCpuNo[s]) < 17) break;
		case OSD_KEY_4:
			if (ADDRESS_BITS(WatchesCpuNo[s]) < 13) break;
		/* JCK 981027 END */
		case OSD_KEY_3:
		case OSD_KEY_2:
		case OSD_KEY_1:
			{
				int addr = Watches[ s ];	/* copy address*/
				int digit = (OSD_KEY_8 - key);	/* if key is OSD_KEY_8, digit = 0*/	/* JCK 091027 */
				int mask;

				/* adjust digit based on cpu address range */
				/* digit -= (6 - ADDRESS_BITS(0) / 4); */
				digit -= (8 - (ADDRESS_BITS(WatchesCpuNo[s])+3) / 4);				/* JCK 091027 */

				mask = 0xF << (digit * 4);	/* if digit is 1, mask = 0xf0*/

				/* JCK 981027 BEGIN */
				do
				{
				if ((addr & mask) == mask)
					/* wrap hex digit around to 0 if necessary */
					addr &= ~mask;
				else
					/* otherwise bump hex digit by 1 */
					addr += (0x1 << (digit * 4));
				} while (addr > MAX_ADDRESS(WatchesCpuNo[s]));
				/* JCK 981027 END */

				Watches[ s ] = addr;

				/* JCK 981023 BEGIN */
				/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */
				/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
				sprintf (buf2, " %01X", WatchesCpuNo[s]);
				strcat(str2[ s ],buf2); */
				AddCpuToWatch(s, str2[ s ]);
				/* JCK 981023 END */

			}
			break;

		/* JCK 981009 BEGIN */

		case OSD_KEY_9:
			if (ManyCpus)    /* JCK 981022 */
			{
				OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
				if(WatchesCpuNo[ s ] == 0)
					WatchesCpuNo[ s ] = cpu_gettotalcpu()-1;
				else
					WatchesCpuNo[ s ]--;

				/* JCK 981026 BEGIN */
				if (Watches[s] == MAX_ADDRESS(OldCpuNo))
					Watches[s] = MAX_ADDRESS(WatchesCpuNo[s]);
				/* JCK 981026 END */

				/* JCK 981023 BEGIN */
				/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
				sprintf (buf2, " %01X", WatchesCpuNo[s]);
				strcat(str2[ s ],buf2); */
				AddCpuToWatch(s, str2[ s ]);
				/* JCK 981023 END */

				/* JCK 981026 BEGIN */
				if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
				{
					ClearTextLine(0, dt[s].y);    /* JCK 981207 */
					dt[s].x = (MachWidth - FontWidth * strlen(str2[ s ])) / 2;
				}
				/* JCK 981026 END */
			}
			break;

		case OSD_KEY_0:
			if (ManyCpus)    /* JCK 981022 */
			{
				OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
				if(WatchesCpuNo[ s ] >= cpu_gettotalcpu()-1)
					WatchesCpuNo[ s ] = 0;
				else
					WatchesCpuNo[ s ]++;

				/* JCK 981026 BEGIN */
				if (Watches[s] == MAX_ADDRESS(OldCpuNo))
					Watches[s] = MAX_ADDRESS(WatchesCpuNo[s]);
				/* JCK 981026 END */

				/* JCK 981023 BEGIN */
				/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
				sprintf (buf2, " %01X", WatchesCpuNo[s]);
				strcat(str2[ s ],buf2); */
				AddCpuToWatch(s, str2[ s ]);
				/* JCK 981023 END */

				/* JCK 981026 BEGIN */
				if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
				{
					ClearTextLine(0, dt[s].y);    /* JCK 981207 */
					dt[s].x = (MachWidth - FontWidth * strlen(str2[ s ])) / 2;
				}
				/* JCK 981026 END */
			}
			break;

		/* JCK 981009 END */

		case OSD_KEY_DEL:
			OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
			WatchesCpuNo[ s ] = 0;    /* JCK 981008 */
			/* Watches[ s ] = MAX_ADDRESS(0); */
			Watches[ s ] = MAX_ADDRESS(WatchesCpuNo[ s ]);    /* JCK 981009 */

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			/* JCK 981026 BEGIN */
			if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
			{
				ClearTextLine(0, dt[s].y);    /* JCK 981207 */
				dt[s].x = (MachWidth - FontWidth * strlen(str2[ s ])) / 2;
			}
			/* JCK 981026 END */

			break;

		/* JCK 981022 BEGIN */

		case OSD_KEY_F1:
			while (osd_key_pressed(key)) ; /* wait for key release */
			if (Watches[s] == MAX_ADDRESS(WatchesCpuNo[s]))
				break;

			/* JCK 981026 BEGIN */
			strcpy(fmt, pShortAddrTemplate(WatchesCpuNo[s]));    /* JCK 981207 */
			if (ManyCpus)
			{
				strcat(fmt," (%01X) = %02X");
				sprintf (buf, fmt, Watches[s], WatchesCpuNo[s],
						RD_GAMERAM(WatchesCpuNo[s], Watches[s]));
			}
			else
			{
				strcat(fmt," = %02X");
				sprintf (buf, fmt, Watches[s], RD_GAMERAM(WatchesCpuNo[s], Watches[s]));
			}
			/* JCK 981026 END */

			if (LoadedCheatTotal < MAX_LOADEDCHEATS-1)
			{
				LoadedCheatTable[LoadedCheatTotal].Count = 0;
				LoadedCheatTable[LoadedCheatTotal].CpuNo = WatchesCpuNo[s];
				LoadedCheatTable[LoadedCheatTotal].Address = Watches[s];
				LoadedCheatTable[LoadedCheatTotal].Special = 0;
				LoadedCheatTable[LoadedCheatTotal].Data = RD_GAMERAM(WatchesCpuNo[s], Watches[s]);
				strcpy(LoadedCheatTable[LoadedCheatTotal].Name,buf);
				LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
				LoadedCheatTable[LoadedCheatTotal].Maximum = 0;
				LoadedCheatTotal++;
				xprintf(0, 0,yFoot,"%s Added",buf);    /* JCK 981022 */
			}
			else
				xprintf(0, 0,yFoot,"%s Not Added",buf);    /* JCK 981022 */
			break;

		case OSD_KEY_F4:
			while (osd_key_pressed(key)) ; /* wait for key release */
			for (i=0;i<MAX_WATCHES;i++)
			{
				if (Watches[i] == MAX_ADDRESS(WatchesCpuNo[i]))
				{
					OldCpuNo = WatchesCpuNo[ i ];    /* JCK 981026 */
					WatchesCpuNo[i] = WatchesCpuNo[s];
					Watches[i] = Watches[s];

					AddCpuToWatch(i, str2[ i ]);    /* JCK 981023 */

					/* JCK 981026 BEGIN */
					if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
					{
						ClearTextLine(0, dt[i].y);    /* JCK 981207 */
						dt[i].x = (MachWidth - FontWidth * strlen(str2[ i ])) / 2;
					}
					/* JCK 981026 END */

					break;
				}
          		}
			break;

		case OSD_KEY_F6:
			while (osd_key_pressed(key)) ; /* wait for key release */
			for (i=0;i<MAX_WATCHES;i++)
			{
				if(LoadedCheatTotal > MAX_LOADEDCHEATS-1)
					break;
				if (Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))
				{
					countAdded++;
					LoadedCheatTable[LoadedCheatTotal].Count = 0;
					LoadedCheatTable[LoadedCheatTotal].CpuNo = WatchesCpuNo[i];
					LoadedCheatTable[LoadedCheatTotal].Address = Watches[i];
					LoadedCheatTable[LoadedCheatTotal].Special = 0;
					LoadedCheatTable[LoadedCheatTotal].Data = RD_GAMERAM(WatchesCpuNo[i], Watches[i]);

					/* JCK 981026 BEGIN */
					strcpy(fmt, pShortAddrTemplate(WatchesCpuNo[i]));
					if (ManyCpus)
					{
						strcat(fmt," (%01X) = %02X");
						sprintf (buf, fmt, Watches[i], WatchesCpuNo[i],
								RD_GAMERAM(WatchesCpuNo[i], Watches[i]));
					}
					else
					{
						strcat(fmt," = %02X");
						sprintf (buf, fmt, Watches[i], RD_GAMERAM(WatchesCpuNo[i], Watches[i]));
					}
					/* JCK 981026 END */

					strcpy(LoadedCheatTable[LoadedCheatTotal].Name,buf);
					LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
					LoadedCheatTable[LoadedCheatTotal].Maximum = 0;
					LoadedCheatTotal++;
				}

          		}
			xprintf(0, 0,yFoot,"%d Added",countAdded);    /* JCK 981022 */

			break;

		case OSD_KEY_F7:
			while (osd_key_pressed(key)) ; /* wait for key release */
			for (i=0;i<MAX_WATCHES;i++)
			{
				WatchesCpuNo[ i ] = 0;
				Watches[ i ] = MAX_ADDRESS(WatchesCpuNo[ i ]);
				AddCpuToWatch(i, str2[ i ]);    /* JCK 981023 */
          		}
			break;

		case OSD_KEY_F10:
			while (osd_key_pressed(key)) ; /* wait for key release */
			ChooseWatchHelp();								/* Show Help*/
			y = ChooseWatchHeader();
			break;

		/* JCK 981022 END */

		case OSD_KEY_ENTER:
			if (s == 0)    /* JCK 981022 */ /* Start Of Watch DT Locations*/
				break;
			OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
			WatchesCpuNo[ s ] = WatchesCpuNo[ s - 1];    /* JCK 981008 */
			Watches[ s ] = Watches[ s - 1];

			/* JCK 981023 BEGIN */
			/* sprintf (str2[ s ], pShortAddrTemplate(0), Watches[ s ]); */
			/* sprintf (str2[ s ], pShortAddrTemplate(WatchesCpuNo[ s ]), Watches[ s ]);
			sprintf (buf2, " %01X", WatchesCpuNo[s]);
			strcat(str2[ s ],buf2); */
			AddCpuToWatch(s, str2[ s ]);
			/* JCK 981023 END */

			/* JCK 981026 BEGIN */
			if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
			{
				ClearTextLine(0, dt[s].y);    /* JCK 981207 */
				dt[s].x = (MachWidth - FontWidth * strlen(str2[ s ])) / 2;
			}
			/* JCK 981026 END */

			break;
	}
  } while (done != 2);    /* JCK 981023 */

  while (osd_key_pressed(key)) ; /* wait for key release */

  /* Set Watch Flag */
  WatchesFlag = 0;
  for(i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
	/* if(Watches[i] != MAX_ADDRESS(0)) */	/* JB 980424 */
	if(Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))    /* JCK 981009 */
	{
		WatchesFlag = 1;
		WatchEnabled = 1;    /* JCK 980917 */
	}

  osd_clearbitmap(Machine->scrbitmap);

  Machine->orientation = trueorientation;
}


int cheat_menu(void)
{
  /* JCK 981022 BEGIN */
  int x,y;
  int s,key,done;
  int total;

  char *paDisplayText[] = {
		"Load And/Or Enable A Cheat",
		"Start A New Cheat Search",
		"Continue Search",
		"Memory Watch",
		"",
		"Return To Main Menu",
		0 };

  struct DisplayText dt[10];
  /* JCK 981022 END */

  /* JCK 981023 BEGIN */
  total = CreateMenu(paDisplayText, dt, FIRSTPOS);
  x = dt[total-1].x;
  y = dt[total-1].y + 2 * FontHeight;
  /* JCK 981023 END */

  osd_clearbitmap(Machine->scrbitmap);

  DisplayActiveCheats(x,y);

  s = 0;
  done = 0;
  do
  {
	key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
	switch (key)
	{
		case OSD_KEY_ENTER:
			switch (s)
			{
				case 0:
					SelectCheat();
					done = 0;
					DisplayActiveCheats(x,y);    /* JCK 981022 */
					break;

				case 1:
					StartSearch();
					done = 0;
					DisplayActiveCheats(x,y);    /* JCK 981022 */
					break;

				case 2:
					ContinueSearch();
					done = 0;
					DisplayActiveCheats(x,y);    /* JCK 981022 */
					break;

				case 3:
					ChooseWatch();
					done = 0;
					DisplayActiveCheats(x,y);    /* JCK 981022 */
					break;

				case 5:
					done = 1;
					break;
			}
			break;
	}
  } while ((done != 1) && (done != 2));    /* JCK 981030 */

  /* clear the screen before returning */
  osd_clearbitmap(Machine->scrbitmap);

  if (done == 2)
	return 1;
  else
	return 0;
}

/* JRT5 Help System Functions */

/* JB 980505 */
/* display help text and wait for key press */
static void ShowHelp (struct DisplayText *dt)
{
  int key;    /* JCK 981016 */
  int iCounter = 0;
  struct DisplayText *text = dt;

  while (dt->text)
  {
	dt->x = 0;
	dt->y = FontHeight * iCounter++;

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
  displaytext (text, 0,1);					/* Draw Help Text*/

  /* JCK 981016 BEGIN */
  /* osd_read_key (0); */						/* Wait For A Key*/
  key = osd_read_keyrepeat (0);
  while (osd_key_pressed(key));
  /* JCK 981016 END */

  osd_clearbitmap (Machine->scrbitmap);	/* Clear Screen Again*/
}

/* JB 980505 */
void CheatListHelp (void)
{
	/* JCK 981016 BEGIN */
	struct DisplayText	dtHelpText[] =
	{
		{ "@       Cheat List Help 1" },				/* Header */			/* JCK 981016 */
		{ "" },
		{ "" },
		{ "@Delete:" },							/* Delete Cheat Info*/
		{ "  Delete the selected Cheat" },
		{ "  from the Cheat List." },
		{ "  (Not from the Cheat File!)" },
		{ "" },
		{ "@Add:" },							/* Add Cheat Info*/
		{ "  Add a new (blank) Cheat to" },
		{ "  the Cheat List." },
		{ "" },
		{ "@Save (F1):" },						/* Save Cheat Info*/
		{ "  Save the selected Cheat in" },
		{ "  the Cheat File." },
		{ "" },
		{ "@Watch (F2):" },						/* Address Watcher Info*/
		{ "  Activate a Memory Watcher" },
		{ "  at the address that the" },
		{ "  selected Cheat modifies." },
		{ "" },
		{ "@Edit (F3):" },						/* Edit Cheat Info*/
		{ "  Edit the Properties of the" },
		{ "  selected Cheat." },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },			/* Continue Prompt */		/* JCK 981016 */
		{ 0 }									/* End Of Text */
	};

	struct DisplayText	dtHelpText2[] =
	{
		{ "@       Cheat List Help 2" },				/* Header */
		{ "" },
		{ "@Copy (F4):" },						/* Copy Cheat Info*/
		{ "  Copy the selected Cheat" },
		{ "  to the Cheat List." },
		{ "" },
		{ "@Save All (F6):" },						/* Save All Cheat Info */	/* JCK 981022 */
		{ "  Save all the Cheats in" },
		{ "  the Cheat File." },
		{ "" },
		{ "@Del All (F7):" },						/* Del All Cheat Info */	/* JCK 981022 */
		{ "  Remove all the active Cheats" },
		{ "" },
		{ "@Reload (F8):" },						/* Reload Database Info */
		{ "  Reload the Cheat Database" },
		{ "" },
		{ "@Rename (F9):" },						/* Rename Database Info */
		{ "  Rename the Cheat Database" },
		{ "" },
		{ "@Help (F10):" },						/* Help Info */
		{ "  Display this help" },
		{ "" },
		/* JCK 981207 BEGIN */
		{ "@Info (F12):" },						/* Info Info */
		{ "  Display Info on a Cheat" },
		/* JCK 981207 END */
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};
	/* JCK 981016 END */

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);    /* JCK 981016 */
}

/* JCK 981016 BEGIN */
void CheatListHelpEmpty (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@       Cheat List Help" },				/* Header */
		{ "" },
		{ "" },
		{ "" },
		{ "@Add:" },							/* Add Cheat Info */
		{ "  Add a new (blank) Cheat to" },
		{ "  the Cheat List." },
		{ "" },
		{ "" },
		{ "" },
		{ "@Reload (F8):" },						/* Reload Database Info */
		{ "  Reload the Cheat Database" },
		{ "" },
		{ "" },
		{ "" },
		{ "@Rename (F9):" },						/* Rename Database Info */
		{ "  Rename the Cheat Database" },
		{ "" },
		{ "" },
		{ "" },
		{ "@Help (F10):" },						/* Help Info */
		{ "  Display this help" },
		{ "" },
		{ "" },
		{ "" },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};

	ShowHelp (dtHelpText);
}
/* JCK 981016 END */

/* JB 980505 */
void StartSearchHelp (void)
{
	struct DisplayText dtHelpText[] =
	{
		{ "@   Cheat Search Help 1" },				/* Header */
		{ "" },
		{ "" },
		{ "@Lives Or Some Other Value:" },				/* Lives/# Search Info */
		{ " Searches for a specific" },
		{ " value that you specify." },
		{ "" },
		{ "@Timers:" },							/* Timers Search Info */
		{ " Starts by storing all of" },
		{ " the game's memory, and then" },
		{ " looking for values that" },
		{ " have changed by a specific" },
		{ " amount from the value that" },
		{ " was stored when the search" },
		{ " was started or continued." },
		{ "" },
		{ "@Energy:" },							/* Energy Search Info */
		{ " Similar to Timers. Searches" },
		{ " for values that are Greater" },
		{ " than, Less than, or Equal" },
		{ " to the values stored when" },
		{ " the search was started or" },
		{ " continued." },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },			/* Continue Prompt */
		{ 0 }									/* End Of Text */
	};

	struct DisplayText dtHelpText2[] =
	{
		{ "@   Cheat Search Help 2" },				/* Header */
		{ "" },
		{ "" },
		{ "@Status:" },							/* Status Search Info */
		{ "  Searches for a Bit or Flag" },
		{ "  that may or may not have" },
		{ "  toggled its value since" },
		{ "  the search was started." },
		{ "" },
		{ "@Slow But Sure:" },						/* SBS Search Info */
		{ "  This search stores all of" },
		{ "  the game's memory, and then" },
		{ "  looks for values that are" },
		{ "  the Same As, or Different" },
		{ "  from the values stored when" },
		{ "  the search was started." },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);
}

/* JB 980505 */
void EditCheatHelp (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@     Edit Cheat Help 1" },				/* Header */
		{ "" },
		{ "" },
		{ "@Name:" },							/* Cheat Name Info */
		{ "  Displays the Name of this" },
		{ "  Cheat. It can be edited by" },
		{ "  hitting <ENTER> while it is" },
		{ "  selected.  Cheat Names are" },
		{ "  limited to 25 characters." },
		{ "  You can use <SHIFT> to" },
		{ "  uppercase a character, but" },
		{ "  only one character at a" },
		{ "  time!" },
		{ "" },
		{ "@CPU:" },							/* Cheat CPU Info */
		{ "  Specifies the CPU (memory" },
		{ "  region) that gets affected." },
		{ "" },
		{ "@Address:" },							/* Cheat Address Info */
		{ "  The Address of the location" },
		{ "  in memory that gets set to" },
		{ "  the new value." },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },			/* Continue Prompt */
		{ 0 }									/* End Of Text */
	};

	struct DisplayText	dtHelpText2[] =
	{
		{ "@     Edit Cheat Help 2" },				/* Header */
		{ "" },
		{ "" },
		{ "@Value:" },							/* Cheat Value Info */
		{ "  The new value that gets" },
		{ "  placed into the specified" },
		{ "  Address while the Cheat is" },
		{ "  active." },
		{ "" },
		{ "@Type:" },							/* Cheat Type Info */
		{ "  Specifies how the Cheat" },
		{ "  will actually work. See the" },
		{ "  CHEAT.DOC file for details." },
		{ "" },
		{ "@Notes:" },							/* Extra Notes */
		{ "  Use the Right and Left" },
		{ "  arrow keys to increment and" },
		{ "  decrement values, or to" },
		{ "  select from pre-defined" },
		{ "  Cheat Names." },											/* JCK 981022 */
		{ "  The <1> ... <8> keys are used" },								/* JCK 981027 */
		{ "  to increment the number in" },
		{ "  that specific column of a" },
		{ "  value." },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);
}
/* JRT5 */

/* JCK 981022 BEGIN */
void ChooseWatchHelp (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@     Choose Watch Help 1" },				/* Header */
		{ "" },
		{ "" },
		{ "" },
		{ "@Delete:" },							/* Delete Watch Info */
		{ "  Delete the selected Watch" },
		{ "" },
		{ "" },
		{ "@Copy (Enter):" },						/* Copy Watch Info */
		{ "  Copy the previous Watch" },
		{ "" },
		{ "" },
		{ "@Notes:" },							/* Extra Notes */
		{ "  Use the Right and Left" },
		{ "  arrow keys to increment and" },
		{ "  decrement values."},
		{ "  The <1> ... <8> keys are used" },								/* JCK 981027 */
		{ "  to increment the number in" },
		{ "  that specific column of a" },
		{ "  value. The <9> and <0> keys" },
		{ "  are used to decrement/increment"},
		{ "  the number of the CPU." },
		{ "  The <I><J><K><L> keys are used" },
		{ "  to move the watches up, left," },
		{ "  down and right." },
		{ "" },
		{ "" },
		{ "" },
		{ "@Press any key to continue..." },			/* Continue Prompt */
		{ 0 }									/* End Of Text */
	};

	struct DisplayText	dtHelpText2[] =
	{
		{ "@     Choose Watch Help 2" },				/* Header */
		{ "" },
		{ "" },
		{ "" },
		{ "@Save (F1):" },						/* Save Watch Info */
		{ "  Saves the selected Watch" },
		{ "  as a Cheat in the Cheat List" },
		{ "" },
		{ "" },
		{ "@Far Copy (F4):" },						/* Far Copy Watch Info */
		{ "  Copy the selected Watch" },
		{ "" },
		{ "" },
		{ "@Save All (F6):" },						/* Save All Watches Info */
		{ "  Saves all the Watches" },
		{ "  as Cheats in the Cheat List" },
		{ "" },
		{ "" },
		{ "@Del All (F7):" },						/* Del All Watches Info */
		{ "  Remove all the Watches" },
		{ "" },
		{ "" },
		{ "@Help (F10):" },						/* Help Info */
		{ "  Display this help" },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);
}
/* JCK 981022 END */

