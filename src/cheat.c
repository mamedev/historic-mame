/*********************************************************************

  cheat.c

*********************************************************************/

/*|*\
|*|  Modifications by John Butler
|*|
|*|	JB 980407:	Changes to support 68K processor and 24-bit addresses.
|*|			This also provided a way to make the cheat system "aware" of
|*|			RAM areas and have it ignores ROM areas.
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
|*|			  by pressing OSD_KEY_F2
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
|*|	JCK 981209:	Fixed display in ContinueSearch function
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
|*|	JCK 981211:	Added new #define : COMMENTCHEAT 999 (type of cheat for comments)
|*|			Added new type of cheat : 999 (comment)
|*|			Comment is marked # in function AddCheckToName
|*|			Cheat structure for a comment :
|*|			  - .Address 	= 0
|*|			  - .Data 		= 0
|*|			  - .Special	= COMMENTCHEAT
|*|			  - other fields as usual
|*|			New functions :
|*|			  - void EditComment(int CheatNo, int ypos)
|*|
|*|	JCK 981212:	Added More to struct cheat_struct (more description on a cheat)
|*|			Fields Name and More of struct cheat_struct are only 40 chars long
|*|			Possibility to get a cheat comment by pressing OSD_KEY_PLUS_PAD
|*|               OSD_KEY_DEL initialises string in xedit function
|*|               Special is saved on 3 digits (%03d)
|*|			Added linked cheats (types 100-144 same as types 0-44)
|*|			Variable int fastsearch instead of #define FAST_SEARCH
|*|			  (this variable can be modified via options or command-line)
|*|			Renamed CheatTotal to ActiveCheatTotal (variable and struct)
|*|			Possibility to add codes from another filename by pressing
|*|                 (OSD_KEY_LEFT_SHIFT or OSD_KEY_RIGHT_SHIFT) + OSD_KEY_F5
|*|               15 matches instead of 10
|*|			New functions :
|*|			  - int xedit(int x,int y,char *inputs,int maxlen)
|*|			  - static char *FormatAddr(int cpu, int addtext)
|*|			  - void SearchInProgress(int yPos, int ShowMsg)
|*|			  - void DisplayWatches(int ClrScr, int *x,int *y, char *buffer)
|*|			  - void DeleteActiveCheatFromTable(int NoCheat)
|*|			  - void DeleteLoadedCheatFromTable(int NoCheat)
|*|			Removed functions :
|*|			  - void EditComment(int CheatNo, int ypos)
|*|			  - static char * pAddrTemplate (int cpu)
|*|			  - static char * pShortAddrTemplate (int cpu)
|*|			  - void ShowSearchInProgress(void)
|*|			  - void HideSearchInProgress(void)
|*|			Rewritten functions :
|*|			  - void DoCheat(void)
|*|			  - void LoadCheat(int merge, char *filename)
|*|			  - int RenameCheatFile(int DisplayFileName, char *filename)
|*|			  - void EditCheat(int CheatNo)
|*|			  - void DisplayActiveCheats(int y)
|*|
|*|	JCK 981213:	Variable int sologame to load cheats for one player only
|*|			  (this variable can be modified via options or command-line)
|*|			Fixed display in EditCheat function
|*|			Possibility to edit the More field in EditCheat function
|*|			Possibility to toggle cheats for one player only ON/OFF
|*|			  by pressing OSD_KEY_F11
|*|			Possibility to scan many banks of memory
|*|			Possibility to press OSD_KEY_F1 in SelectValue function
|*|			New functions :
|*|			  - int SkipBank(int CpuToScan, int *BankToScanTable, void (*handler)(int,int))
|*|			Rewritten functions :
|*|			  - static int build_tables (void)
|*|
|*|	JCK 981214: Integrated some of VM's changes : function SelectCheat has been rewritten
|*|			No more reference to HardRefresh in SelectCheat function (label has been deleted)
|*|			One function to save one or all cheats to the cheatfile (SaveCheat)
|*|			New #define :
|*|			  - NEW_CHEAT ((struct cheat_struct *) -1)
|*|			  - YHEAD_SELECT (FontHeight * 9)
|*|			  - YFOOT_SELECT (MachHeight - (FontHeight * 2))
|*|			  - YFOOT_MATCH  (MachHeight - (FontHeight * 8))
|*|			  - YFOOT_WATCH  (MachHeight - (FontHeight * 3))
|*|			Rewritten functions :
|*|			  - void ContinueSearchMatchFooter(int count, int idx)
|*|			  - void ChooseWatchFooter(void)
|*|			New functions :
|*|			  - void set_cheat(struct cheat_struct *dest, struct cheat_struct *src)
|*|			  - int build_cheat_list(int Index, struct DisplayText ext_dt[40],
|*|			  						char ext_str2[MAX_DISPLAYCHEATS + 1][40])
|*|			  - int SelectCheatHeader(void)
|*|			  - int SaveCheat(int NoCheat)
|*|			  - int FindFreeWatch(void)
|*|			  - void AddCpuToAddr(int cpu, int addr, int data, char *buffer)
|*|			Modified functions to use the new set_cheat function :
|*|			  - void DeleteActiveCheatFromTable(int NoCheat)
|*|			  - void DeleteLoadedCheatFromTable(int NoCheat)
|*|			  - void ContinueSearch(void)
|*|			  - void ChooseWatch(void)
|*|
|*|	JCK 981215: Fixed the -sologame option : cheats for players 2, 3 and 4 are not loaded
|*|			dt strcuture is correctly filled in SelectCheat function
|*|
|*|	JCK 981216: Partially fixed display for the watches : the last watch is now removed
|*|			Fixed message when user wants to add a cheatfile (SHIFT + OSD_KEY_F5)
|*|			Name of an active cheat changes according to the name of the loaded cheat
|*|			Added a new help for CheatListHelp and CheatListHelpEmpty
|*|			Help is now centered according to the max. length of help text
|*|			SelectCheatHeader is now correctly called
|*|			Possibility to activate more than one cheat for types 20-24 and 40-44
|*|			Possibility to change the speed of search in the "Start Search" menu
|*|			Check is made to see that a user hasn't pressed OSD_KEY_xSHIFT in combinaison
|*|			  with the function keys (OSD_KEY_Fx) in the following functions :
|*|			  - void SelectCheat(void)
|*|			  - void ContinueSearch(void)
|*|			  - void ChooseWatch(void)
|*|			New functions :
|*|			  - int SelectFastSearchHeader(void)
|*|			  - void SelectFastSearchHelp(void)
|*|			Rewritten functions :
|*|			  - void CheatListHelp (void)
|*|			  - void CheatListHelpEmpty (void)
|*|
|*|	JCK 981217: Use of osd_key_pressed_memory() instead of osd_key_pressed() to test
|*|			  OSD_KEY_CHEAT_TOGGLE, OSD_KEY_INSERT and OSD_KEY_DEL in DoCheat function
|*|			dt strcuture is correctly filled in SelectCheat function
|*|
|*|  Modifications by Felipe de Almeida Leme (FAL)
|*|
|*|	FAL 981029:	Pointer checking to avoid segmentation fault when reading an incorrect cheat file
|*|
\*|*/

/*\
|*|  TO DO list by JCK from The Ultimate Patchers
|*|
|*|	JCK 981216: Possibility to view the matches without scanning the memory again :
|*|			  - functions StartSearch and ContinueSearch should return an int
|*|			  	(stored in static int MatchFound)
|*|			  - function StartSearch and ContinueSearch should be rewritten
|*|			  - function memset_ram should be removed
|*|			Take care of orientation to correctly erase the watches
|*|			  (functions ClearTextLine and ClearArea should be rewritten)
|*|			Trap the Shift keys : correct bug when they are pressed
|*|			Merge functions DeleteActiveCheatFromTable and DeleteLoadedCheatFromTable
|*|			Rewrite function FormatAddr ?
\*|*/

#include "driver.h"
#include <stdarg.h>
#include <ctype.h>  /* for toupper() and tolower() */

/* JCK 981123 Please do not remove ! Just comment it out ! */
/* #define JCK */


/*
 The CHEAT.DAT file:
 -This file should be in the same directory of MAME.EXE .
 -This file can be edited with a text editor, but keep the same format:
    all fields are separated by a colon (:)
   * Name of the game (short name)
   * No of the CPU
   * Address in Hexadecimal
   * Data to put at this address in Hexadecimal
   * Type of cheat
   * Description of the cheat (29 caracters max)
   * More description of the cheat (29 caracters max) - this field is optional

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

JCK 981212
100 to 144-Used for linked cheats; same as 0 to 44 otherwize
999-Comment (can't be activated - marked with a # in the cheat list)
*/

extern struct GameDriver neogeo_bios;    /* JCK 981208 */
extern unsigned char *memory_find_base (int cpu, int offset);

struct cheat_struct
{
  int CpuNo;
  int Address;
  int Data;
  int Special;
  int Count;
  int Backup;
  int Minimum;		/* JCK 981016 */
  int Maximum;		/* JCK 981016 */
  char Name[40];		/* JCK 981212 */
  char More[40];		/* JCK 981212 */
};

/* macros stolen from memory.c for our nefarious purposes: */
#define MEMORY_READ(index,offset)       ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].memory_read)(offset))
#define MEMORY_WRITE(index,offset,data) ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].memory_write)(offset,data))
#define ADDRESS_BITS(index)             (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].address_bits)

#define MAX_ADDRESS(cpu)	(0xFFFFFFFF >> (32-ADDRESS_BITS(cpu)))

#define RD_GAMERAM(cpu,a)	read_gameram(cpu,a)
#define WR_GAMERAM(cpu,a,v)	write_gameram(cpu,a,v)

#define MAX_LOADEDCHEATS	150
#define MAX_ACTIVECHEATS	15
#define MAX_DISPLAYCHEATS	15
#define MAX_MATCHES		15    		/* JCK 981212 */
#define MAX_WATCHES		10

#define TOTAL_CHEAT_TYPES	144			/* JCK 981212 */
#define CHEAT_NAME_MAXLEN	29
#define CHEAT_FILENAME_MAXLEN	29                /* JCK 981212 */

#define Method_1 1
#define Method_2 2
#define Method_3 3
#define Method_4 4
#define Method_5 5

#define NOVALUE 			-0x0100		/* No value has been selected */ /* JCK 981023 */

#define FIRSTPOS			(FontHeight*3/2)	/* yPos of the 1st string displayed */ /* JCK 981026 */

#define COMMENTCHEAT          999               /* Type of cheat for comments */ /* JCK 981211 */

/* VM 981213 BEGIN */
/* Q: Would 0/NULL be better than -1? */
#define NEW_CHEAT ((struct cheat_struct *) -1)
/* VM 981213 END */

/* VM & JCK 981214 BEGIN */
#define YHEAD_SELECT (FontHeight * 9)
#define YFOOT_SELECT (MachHeight - (FontHeight * 2))
#define YFOOT_MATCH  (MachHeight - (FontHeight * 8))
#define YFOOT_WATCH  (MachHeight - (FontHeight * 3))
/* VM & JCK 981214 END */

/* This variables are not static because of extern statement */
char *cheatfile = "CHEAT.DAT";    /* JCK 980917 */
int fastsearch = 2;    /* JCK 981212 */
int sologame = 0;      /* JCK 981213 */

int he_did_cheat;

void		CheatListHelp( void );
void		CheatListHelpEmpty( void );		/* JCK 981016 */
void		EditCheatHelp( void );
void		StartSearchHelp( void );
void		ChooseWatchHelp( void );		/* JCK 981022 */
void		SelectFastSearchHelp ( void );	/* JCK 981216 */

static int	iCheatInitialized = 0;

static struct ExtMemory StartRam[MAX_EXT_MEMORY];
static struct ExtMemory BackupRam[MAX_EXT_MEMORY];
static struct ExtMemory FlagTable[MAX_EXT_MEMORY];

static int ActiveCheatTotal;
static int LoadedCheatTotal;
static struct cheat_struct ActiveCheatTable[MAX_ACTIVECHEATS+1];    /* JCK 981212 */
static struct cheat_struct LoadedCheatTable[MAX_LOADEDCHEATS+1];    /* JCK 981212 */

static unsigned int WatchesCpuNo[MAX_WATCHES];    /* JCK 981008 */
static unsigned int Watches[MAX_WATCHES];    /* JCK 980917 */
static int WatchesFlag;
static int WatchX,WatchY;

static int StartValue;
static int CurrentMethod;
static int SaveMethod;    /* JCK 981023 */

static int CheatEnabled;

static int WatchEnabled;    /* JCK 980917 */

static int SearchCpuNo;    /* JCK 981006 */
static int SearchCpuNoOld; /* JCK 981206 */

static int MallocFailure;  /* JCK 981208 */

/* JCK 981022 BEGIN */
static int MachHeight;
static int MachWidth;
static int FontHeight;
static int FontWidth;
static int ManyCpus;
/* JCK 981022 END */

static int ValTmp;			/* JCK 981023 */

static int MatchFound;    /* JCK 981216 */

/* JCK 981212 BEGIN */
static char CCheck[2]	= "\x8C";
static char CNoCheck[2]	= " ";
static char CComment[2]	= "#";
static char CMore[2]	= "+";
/* JCK 981212 END */

/* START JB 980506 */
static unsigned char osd_key_chars[] =
{
/* 0    1    2    3    4    5    6    7    8    9 */
   0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8',    /* 0 */
  '9', '0', '-', '=',  0 ,  0,  'q', 'w', 'e', 'r',    /* 1 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']',  0 ,  0 ,    /* 2 */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 3 */
 '\'', '`',  0 ,  0 , 'z', 'x', 'c', 'v', 'b', 'n',    /* 4 */
  'm', ',', '.', '/',  0 , '8',  0 , ' ',  0 ,  0 ,    /* 5 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 6 */
   0 ,  0 ,  0 ,  0 , '-',  0 , '5',  0 , '+',  0 ,    /* 7 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 8 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 9 */
   0 , '1', '2', '3', '4',  0 , '6', '7', '8', '9',    /* 10 */
  '0',  0 , '=', '/', '*',  0
};

static unsigned char osd_key_caps[] =
{
/* 0    1    2    3    4    5    6    7    8    9 */
   0 ,  0 , '!', '@', '#', '$', '%', '^', '&', '*',    /* 0 */
  '(', ')', '_', '+',  0 ,  0 , 'Q', 'W', 'E', 'R',    /* 1 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  0 ,  0 ,    /* 2 */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',    /* 3 */
  '"', '~',  0 ,  0 , 'Z', 'X', 'C', 'V', 'B', 'N',    /* 4 */
  'M', '<', '>', '?',  0 , '*',  0 , ' ',  0 ,  0 ,    /* 5 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 6 */
   0 ,  0 ,  0 ,  0 , '-',  0 , '5',  0 , '+',  0 ,    /* 7 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 8 */
   0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,    /* 9 */
   0 , '1', '2', '3', '4',  0 , '6', '7', '8', '9',    /* 10 */
  '0',  0 , '=', '/', '*',  0
};
/* END JB 980506 */

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
  if (x == 0)
	dt[0].x = (MachWidth - FontWidth * strlen(s)) / 2;
  else
	dt[0].x = x;
  if (dt[0].x < 0)
	dt[0].x = 0;
  if (dt[0].x > MachWidth)
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

/* JCK 981212 BEGIN */
/* Edit a string at the position x y
 * if x = 0 then center the string in the screen */
int xedit(int x,int y,char *inputs,int maxlen)
{
  int i, key, length;
  int done = 0;
  char *buffer;

  if ((buffer = malloc(maxlen+1)) == NULL)
	return(2);    /* Cancel as if used pressed Esc */

  memset (buffer, '\0', sizeof(buffer));
  strncpy (buffer, inputs, maxlen+1);
  do
  {
	ClearTextLine(1, y);
	xprintf (1, x, y, "%s", buffer);
	length = strlen (buffer);
	key = osd_read_keyrepeat (0);
	switch (key)
	{
		case OSD_KEY_DEL:
			memset (buffer, '\0', sizeof(buffer));
			break;
		case OSD_KEY_BACKSPACE:
			if (length)
				buffer[length-1] = 0;
			break;
		case OSD_KEY_ENTER:
			done = 1;
			strcpy (inputs, buffer);
			break;
		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			done = 2;
			break;
		default:
			if (length < maxlen)
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
  } while (done == 0);
  ClearTextLine(1, y);
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
			*done = 3;    /* JCK 981212 */
			break;
		case OSD_KEY_UP:
			if (*s > Mini)
				(*s)--;
			else
				*s = Maxi;
			while ((*s > Mini) && (!dt[*s].text[0]))    /* For Space In Menu */
				(*s)--;
			*done = 3;    /* JCK 981212 */
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

/* Function to select a value (returns the value or NOVALUE if OSD_KEY_ESC or OSD_KEY_TAB) */ /* JCK 981023 */
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

		/* JCK 981213 BEGIN */
		case OSD_KEY_F1:
			done = 3;
			break;
		/* JCK 981213 END */

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

  /* JCK 981213 BEGIN */
  if (done == 3)
	return (2 * NOVALUE);
  /* JCK 981213 END */

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


/* return a format specifier for printf based on cpu address range */
static char *FormatAddr(int cpu, int addtext)
{
	static char bufadr[10];
	static char buffer[18];
      int i;

	memset (buffer, '\0', strlen(buffer));
	switch ((ADDRESS_BITS(cpu)+3) >> 2)
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
	/* JCK 981027 END */
}
/* JCK 981212 END */

/* JCK 981214 BEGIN */
void AddCpuToAddr(int cpu, int addr, int data, char *buffer)
{
	char fmt[32];

	strcpy (fmt, FormatAddr(cpu ,0));
	if (ManyCpus)
	{
		strcat(fmt," (%01X) = %02X");
		sprintf (buffer, fmt, addr, cpu, data);
	}
	else
	{
		strcat(fmt," = %02X");
		sprintf (buffer, fmt, addr, data);
	}
}
/* JCK 981214 END */

/* JCK 981023 BEGIN */
void AddCpuToWatch(int NoWatch,char *buffer)
{
  char bufadd[10];

  sprintf(buffer, FormatAddr(WatchesCpuNo[NoWatch],0),
			Watches[NoWatch]);    /* JCK 981212 */
  if (ManyCpus)
  {
	sprintf (bufadd, "  (%01X)", WatchesCpuNo[NoWatch]);
	strcat(buffer,bufadd);
  }
}
/* JCK 981023 END */

/* JCK 981030 BEGIN */
void AddCheckToName(int NoCheat,char *buffer)
{
  int i = 0;
  int flag = 0;
  int Special;

  if (LoadedCheatTable[NoCheat].Special == COMMENTCHEAT)
	strcpy(buffer,CComment);    /* JCK 981211 */
  else
  {
	for (i = 0;i < ActiveCheatTotal && !flag;i ++)    /* JCK 981212 */
	{
		/* JCK 981212 BEGIN */
		Special = ActiveCheatTable[i].Special;
		if (Special >= 200)
			Special -= 200;

		if (Special < 60)
		{
			if (	(ActiveCheatTable[i].Address 	== LoadedCheatTable[NoCheat].Address)	&&
				(ActiveCheatTable[i].Data	== LoadedCheatTable[NoCheat].Data	)	&&
				(Special				== LoadedCheatTable[NoCheat].Special)	)
				flag = 1;
		}
		else
		{
			if (	(ActiveCheatTable[i].Address 	== LoadedCheatTable[NoCheat].Address)	&&
				(Special				== LoadedCheatTable[NoCheat].Special)	)
				flag = 1;
		}
		/* JCK 981212 END */
	}
	strcpy(buffer,(flag ? CCheck : CNoCheck));

	/* JCK 981216 BEGIN */
	if (flag)
		strcpy(ActiveCheatTable[i-1].Name, LoadedCheatTable[NoCheat].Name);
	/* JCK 981216 END */

  }

  /* JCK 981212 BEGIN */
  if (LoadedCheatTable[NoCheat].More[0])
	strcat(buffer,CMore);
  else
	strcat(buffer," ");
  /* JCK 981212 BEGIN */

  strcat(buffer,LoadedCheatTable[NoCheat].Name);
  for (i = strlen(buffer);i < CHEAT_NAME_MAXLEN+2;i ++)
	buffer[i]=' ';

  buffer[CHEAT_NAME_MAXLEN+2]=0;
}
/* JCK 981030 END */

/* read a byte from cpu at address <add> */
static unsigned char read_gameram (int cpu, int add)
{
	int save = cpu_getactivecpu();
	unsigned char data;

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

/* JCK 981213 BEGIN */
/* Returns 1 if memory area has to be skipped */
int SkipBank(int CpuToScan, int *BankToScanTable, void (*handler)(int,int))
{
	int res = 0;

	if (fastsearch)
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
/* JCK 981213 END */

/* JCK 981212 BEGIN */
/* create tables for storing copies of all MWA_RAM areas */
static int build_tables (void)    /* JCK 981123 */
{
	/* const struct MemoryReadAddress *mra = Machine->drv->cpu[SearchCpuNo].memory_read; */
	const struct MemoryWriteAddress *mwa = Machine->drv->cpu[SearchCpuNo].memory_write;    /* VM 981210 */

	int region = Machine->drv->cpu[SearchCpuNo].memory_region;    /* JCK 981126 */

	struct ExtMemory *ext_sr = StartRam;
	struct ExtMemory *ext_br = BackupRam;
	struct ExtMemory *ext_ft = FlagTable;

	int i, yPos;    /* JCK 981213 */

      /* VM & JCK 981208 BEGIN */
      /* Trap memory allocation errors */
	int MemoryNeeded = 0;

	/* Search speedup : (the games should be dasmed to confirm this) */
      /* Games based on Exterminator driver should scan BANK1          */
      /* Games based on SmashTV driver should scan BANK2               */
      /* NEOGEO games should only scan BANK1 (0x100000 -> 0x01FFFF)    */
	int CpuToScan = -1;
      int BankToScanTable[9];    /* 0 for RAM & 1-8 for Banks 1-8 */ /* JCK 981213 */

	/* JCK 981213 BEGIN */
      for (i = 0; i < 9;i ++)
      	BankToScanTable[i] = ( fastsearch < 2 );
	/* JCK 981213 END */

	if ((Machine->drv->cpu[1].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010)
	{
		/* 2nd CPU is 34010: games based on Exterminator driver */
		CpuToScan = 0;
		BankToScanTable[1] = 1;    /* JCK 981213 */
	}
	else if ((Machine->drv->cpu[0].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010)
	{
		/* 1st CPU but not 2nd is 34010: games based on SmashTV driver */
		CpuToScan = 0;
		BankToScanTable[2] = 1;    /* JCK 981213 */
	}
	else if (Machine->gamedrv->clone_of == &neogeo_bios)
	{
		/* games based on NEOGEO driver */
		CpuToScan = 0;
		BankToScanTable[1] = 1;    /* JCK 981213 */
	}
      /* VM & JCK 981208 END */

	/* JCK 981213 BEGIN */
      /* No CPU so we scan RAM & BANKn */
      if ((CpuToScan == -1) && (fastsearch >= 2))
      	for (i = 0; i < 9;i ++)
      		BankToScanTable[i] = 1;
	/* JCK 981213 END */

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
      yPos = FIRSTPOS + 10 * FontHeight;
	xprintf(0, 0, yPos, "Allocating Memory...");
	/* JCK 981208 END */

	/* VM 981210 BEGIN */
	while (mwa->start != -1)
	{
		/* int (*handler)(int) = mra->handler; */
		void (*handler)(int,int) = mwa->handler;
		int size = (mwa->end - mwa->start) + 1;

		/* JCK 981213 BEGIN */
		if (SkipBank(CpuToScan, BankToScanTable, handler))
		{
			mwa++;
			continue;
		}

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
				ext_sr++, ext_br++, ext_ft++;
			}
		}
		else
			MemoryNeeded += (3 * size);
		/* JCK 981213 END */
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

	ClearTextLine (1, yPos);    /* JCK 981213 */

	return MallocFailure;    /* JCK 981208 */
}


/* JCK 981212 BEGIN */
/* Message to show that search is in progress */
void SearchInProgress(int ShowMsg, int yPos)
{
  if (ShowMsg)
	xprintf(0, 0, yPos, "Search in Progress...");
  else
	ClearTextLine (1, yPos);
}


/* Function to rename the cheatfile (returns 1 if the file has been renamed else 0)*/
int RenameCheatFile(int merge, int DisplayFileName, char *filename)
{
  int i, key;
  int done = 0;
  int EditYPos;
  char buffer[32];

  EditYPos = FIRSTPOS + 3 * FontHeight;    /* JCK 981026 */

  osd_clearbitmap (Machine->scrbitmap);

  if (merge < 0)
	xprintf (0, 0, EditYPos-(FontHeight*2),
			"Enter the Cheat Filename");
  else
	xprintf (0, 0, EditYPos-(FontHeight*2),
			"Enter the Filename to %s:",(merge ? "Add" : "Load"));    /* JCK 981216 */

  memset (buffer, '\0', 32);
  strncpy (buffer, filename, CHEAT_FILENAME_MAXLEN);

  done = xedit(0, EditYPos, buffer, CHEAT_FILENAME_MAXLEN);
  if (done == 1)
  {
	strcpy (filename, buffer);
      if (DisplayFileName)
      {
		osd_clearbitmap (Machine->scrbitmap);
	      xprintf (0, 0, EditYPos-(FontHeight*2), "Cheat Filename is now:");
		xprintf (0, 0, EditYPos, "%s", buffer);    /* JCK 981022 */
		EditYPos += 4*FontHeight;
		xprintf(0, 0,EditYPos,"Press A Key To Continue...");
		key = osd_read_keyrepeat(0);
		while (osd_key_pressed(key)) ; /* wait for key release */
      }
  }
  osd_clearbitmap (Machine->scrbitmap);
  return(done);
}

/* JCK 981214 BEGIN */
/* Function who loads the cheats for a game */
int SaveCheat(int NoCheat)
{
	FILE *f;
      char fmt[32];
      int i;
	int count = 0;

	if ((f = fopen(cheatfile,"a")) != 0)
	{
		for (i = 0; i < LoadedCheatTotal; i++)
		{
            	if ((NoCheat == i) || (NoCheat == -1))
                  {
	            	int addmore = (LoadedCheatTable[i].More[0]);

				/* form fmt string, adjusting length of address field for cpu address range */
				sprintf(fmt, "%%s:%%d:%s:%%02X:%%03d:%%s%s\n",
						FormatAddr(LoadedCheatTable[i].CpuNo,0),
	                              (addmore ? ":%s" : ""));

				#ifdef macintosh
				fprintf(f, "\r");     /* force DOS-style line enders */
				#endif

				if (fprintf(f, fmt, Machine->gamedrv->name,
						LoadedCheatTable[i].CpuNo,
						LoadedCheatTable[i].Address,
						LoadedCheatTable[i].Data,
						LoadedCheatTable[i].Special,
						LoadedCheatTable[i].Name,
	                              (addmore ? LoadedCheatTable[i].More : "")))
					count ++;
			}
		}
		fclose (f);
	}
      return(count);
}
/* JCK 981214 END */

/* JCK 981215 BEGIN */
/* Function who loads the cheats for a game */
void LoadCheat(int merge, char *filename)
{
  FILE *f;
  char *ptr;
  char str[90];    /* To support the add. description */ /* JCK 981213 */

  /* JCK 981212 BEGIN */
  if (!merge)
  {
	ActiveCheatTotal = 0;
	LoadedCheatTotal = 0;
  }
  /* JCK 981212 END */

  /* Load the cheats for that game */
  /* Ex.: pacman:0:4e14:6:0:Infinite Lives */
  if ((f = fopen(filename,"r")) != 0)    /* JCK 981212 */
  {
	for(;;)
	{
	      if (fgets(str,90,f) == NULL)    /* JCK 981213 */
      		break;

		#ifdef macintosh  /* JB 971004 */
		/* remove extraneous LF on Macs if it exists */
		if ( str[0] == '\r' )
			strcpy( str, &str[1] );
		#endif

		if (str[strlen(Machine->gamedrv->name)] != ':')
			continue;
		if (strncmp(str,Machine->gamedrv->name,strlen(Machine->gamedrv->name)) != 0)
			continue;
		if (str[0] == ';') /*Comments line*/
			continue;

		if (LoadedCheatTotal >= MAX_LOADEDCHEATS)    /* JCK 981212 */
		{
			break;
		}

		/* JCK 981215 BEGIN */
		if (sologame)
			if (	(strstr(str, "2UP") != NULL) || (strstr(str, "PL2") != NULL)	||
				(strstr(str, "3UP") != NULL) || (strstr(str, "PL3") != NULL)	||
				(strstr(str, "4UP") != NULL) || (strstr(str, "PL4") != NULL)	||
				(strstr(str, "2up") != NULL) || (strstr(str, "pl2") != NULL)	||
				(strstr(str, "3up") != NULL) || (strstr(str, "pl3") != NULL)	||
				(strstr(str, "4up") != NULL) || (strstr(str, "pl4") != NULL)	)
		          continue;
		/* JCK 981215 END */

		/* JCK 980917 BEGIN */
		/* Reset the counter */
		LoadedCheatTable[LoadedCheatTotal].Count=0;
		/* JCK 980917 END */

		/* Extract the fields from the string */
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

		/* JCK 981212 BEGIN */
		strcpy(LoadedCheatTable[LoadedCheatTotal].More,"\n");
		ptr = strtok(NULL, ":");
		if (ptr)
			strcpy(LoadedCheatTable[LoadedCheatTotal].More,ptr);
		if (strstr(LoadedCheatTable[LoadedCheatTotal].Name,"\n") != NULL)
			LoadedCheatTable[LoadedCheatTotal].Name[strlen(LoadedCheatTable[LoadedCheatTotal].Name)-1] = 0;
		if (strstr(LoadedCheatTable[LoadedCheatTotal].More,"\n") != NULL)
			LoadedCheatTable[LoadedCheatTotal].More[strlen(LoadedCheatTable[LoadedCheatTotal].More)-1] = 0;
		/* JCK 981212 END */

		/* JCK 981016 BEGIN */
		/* Fill the new fields : Minimum and Maximum */
		if (	(LoadedCheatTable[LoadedCheatTotal].Special==62)	||
			(LoadedCheatTable[LoadedCheatTotal].Special==65)	||
			(LoadedCheatTable[LoadedCheatTotal].Special==72)	||
			(LoadedCheatTable[LoadedCheatTotal].Special==75)	)
			LoadedCheatTable[LoadedCheatTotal].Minimum = 1;
		else
			LoadedCheatTable[LoadedCheatTotal].Minimum = 0;
		if (	(LoadedCheatTable[LoadedCheatTotal].Special>=60)	&&
			(LoadedCheatTable[LoadedCheatTotal].Special<=75)	)
		{
			LoadedCheatTable[LoadedCheatTotal].Maximum = LoadedCheatTable[LoadedCheatTotal].Data;
			LoadedCheatTable[LoadedCheatTotal].Data = 0;
		}
		else
			LoadedCheatTable[LoadedCheatTotal].Maximum = 0xFF;
		/* JCK 981016 END */

		/* JCK 981211 BEGIN */
		if (LoadedCheatTable[LoadedCheatTotal].Special == COMMENTCHEAT)
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
/* JCK 981215 END */

/* Init some variables */
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

  reset_table (StartRam);
  reset_table (BackupRam);
  reset_table (FlagTable);

  MatchFound = 0;    /* JCK 981216 */

  for (i = 0;i < MAX_WATCHES;i ++)    /* JCK 980917 */
  {
	WatchesCpuNo[ i ] = 0;    /* JCK 981008 */
	Watches[ i ] = MAX_ADDRESS(WatchesCpuNo[ i ]);    /* JCK 981009 */
  }

  WatchX = Machine->uixmin;
  WatchY = Machine->uiymin;
  WatchesFlag = 0;

  LoadCheat(0, cheatfile);   /* JCK 981212 */

}

/* JCK 981212 BEGIN */
void DisplayActiveCheats(int y)
{
  int i;

  xprintf(0, 0, y, "Active Cheats: %d", ActiveCheatTotal);
  y += ( FontHeight * 3 / 2 );

  if (ActiveCheatTotal == 0)
	xprintf(0, 0, y, "--- None ---");

  for (i = 0;i < ActiveCheatTotal;i ++)
  {
	xprintf(0, 0 ,y, "%-30s", ActiveCheatTable[i].Name);
	y += FontHeight;
  }
}

/* Display watches if there are some */
void DisplayWatches(int ClrScr, int *x,int *y,char *buffer)
{
      int i;
      char bufadr[4];

	/* JCK 981216 BEGIN */
	if (ClrScr)
		ClearArea(0, ( *y ? *y - 1 : *y ), *y + FontHeight + 1);
	/* JCK 981216 END */

	if ((WatchesFlag != 0) && (WatchEnabled != 0))
	{
		int trueorientation;

		/* hack: force the display into standard orientation */
		trueorientation = Machine->orientation;
		Machine->orientation = ORIENTATION_DEFAULT;

		buffer[0] = 0;
		for (i = 0;i < MAX_WATCHES;i ++)
		{
			if ( Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))
			{
                  	if (i)    /* If not 1st watch add a space */
                  		strcat(buffer," ");
				sprintf(bufadr,"%02X", RD_GAMERAM (WatchesCpuNo[i], Watches[i]));
				strcat(buffer,bufadr);
			}
		}

            /* Adjust x offset to fit the screen */
		while (	(*x >= (MachWidth - (FontWidth * (int)strlen(buffer)))) &&
            		(*x > Machine->uixmin)	)
            	(*x)--;

		for (i = 0;i < (int)strlen(buffer);i ++)
			drawgfx(Machine->scrbitmap,Machine->uifont,buffer[i],DT_COLOR_WHITE,0,0,
				*x+(i*FontWidth),*y,0,TRANSPARENCY_NONE,0);

		Machine->orientation = trueorientation;
	}
}

/* VM & JCK 981214 BEGIN */
/* copy one cheat structure to another */
void set_cheat(struct cheat_struct *dest, struct cheat_struct *src)
{
	/* Changed order to match structure - Added field More */
	struct cheat_struct new_cheat =
	{
		0,				/* CpuNo */
		0,				/* Address */
		0,				/* Data */
		0,				/* Special */
		0,				/* Count */
		0,				/* Backup */
		0,				/* Minimum */
		0xFF,				/* Maximum */
		"---- New Cheat ----", 	/* Name */
		""				/* More */
        };

	if (src == NEW_CHEAT)
	{
		src = &new_cheat;
	}

	dest->CpuNo 	= src->CpuNo;
	dest->Address 	= src->Address;
	dest->Data 		= src->Data;
	dest->Special 	= src->Special;
	dest->Count 	= src->Count;
	dest->Backup 	= src->Backup;
	dest->Minimum 	= src->Minimum;
	dest->Maximum 	= src->Maximum;
	strcpy(dest->Name, src->Name);
	strcpy(dest->More, src->More);
}
/* VM & JCK 981214 END */

void DeleteActiveCheatFromTable(int NoCheat)
{
  int i;
  if ((NoCheat > ActiveCheatTotal) || (NoCheat < 0))
	return;
  for (i = NoCheat; i < ActiveCheatTotal-1;i ++)
  {
	set_cheat(&ActiveCheatTable[i], &ActiveCheatTable[i + 1]);    /* JCK 981214 */
  }
  ActiveCheatTotal --;
}

void DeleteLoadedCheatFromTable(int NoCheat)
{
  int i;
  if ((NoCheat > LoadedCheatTotal) || (NoCheat < 0))
	return;
  for (i = NoCheat; i < LoadedCheatTotal-1;i ++)
  {
	set_cheat(&LoadedCheatTable[i], &LoadedCheatTable[i + 1]);    /* JCK 981214 */
  }
  LoadedCheatTotal --;
}
/* JCK 981212 END */

/* JCK 981214 BEGIN */
int FindFreeWatch(void)
{
  int i;
  for (i = 0;i < MAX_WATCHES;i ++)
  {
	if (Watches[i] == MAX_ADDRESS(WatchesCpuNo[i]))
		break;
  }
  return(i == MAX_WATCHES ? 0 : i+1);
}
/* JCK 981214 END */


/* Free allocated arrays */
void StopCheat(void)
{
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

      DisplayWatches(0, &WatchX, &WatchY, buf);    /* JCK 981212 */

	/* Affect the memory */
	for (i = 0; CheatEnabled == 1 && i < ActiveCheatTotal;i ++)    /* JCK 981212 */
	{
		if (	(ActiveCheatTable[i].Special == 0)		||
			(ActiveCheatTable[i].Special == 100)	)    /* JCK 981212 */
		{
			/* JB 980407 */
			WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                  		ActiveCheatTable[i].Address,
                  		ActiveCheatTable[i].Data);
		}
		else
		{
			if (ActiveCheatTable[i].Count == 0)
			{
				/* Check special function */
				switch(ActiveCheatTable[i].Special)
				{
					case 1:
					case 101:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
                                    DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						break;
					case 2:
					case 102:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 1*60;
						break;
					case 3:
					case 103:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 2*60;
						break;
					case 4:
					case 104:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 5*60;
						break;

					/* 5,6,7 check if the value has changed, if yes, start a timer
					    when the timer end, change the location*/
					case 5:
					case 105:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 1*60;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
						}
						break;
					case 6:
					case 106:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 2*60;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
						}
						break;
					case 7:
					case 107:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 5*60;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
						}
						break;

					/* 8,9,10,11 do not change the location if the value change by X every frames
					   This is to try to not change the value of an energy bar
					   when a bonus is awarded to it at the end of a level
					   See Kung Fu Master*/
					case 8:
					case 108:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 1;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
							ActiveCheatTable[i].Backup =
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address);
						}
						break;
					case 9:
					case 109:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 1;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
							ActiveCheatTable[i].Backup =
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address);
						}
						break;
					case 10:
					case 110:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 1;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
							ActiveCheatTable[i].Backup =
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address);
						}
						break;
					case 11:
					case 111:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Data)
						{
							ActiveCheatTable[i].Count = 1;
							ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
							ActiveCheatTable[i].Backup =
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address);
						}
						break;

					/* JCK 980917 BEGIN */
					case 20:
					case 120:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) |
                                                			ActiveCheatTable[i].Data);
						break;
					case 21:
					case 121:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) |
		                                                	ActiveCheatTable[i].Data);
                                    DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						break;
					case 22:
					case 122:    /* JCK 981212 Linked cheat */
						/* JB 980407 */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
            	                                   		ActiveCheatTable[i].Address) |
                  		                              	ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 1*60;
						break;
					case 23:
					case 123:    /* JCK 981212 Linked cheat */
						/* JB 980407 */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) |
                              		                  	ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 2*60;
						break;
					case 24:
					case 124:    /* JCK 981212 Linked cheat */
						/* JB 980407 */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) |
                                          		      	ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 5*60;
						break;
					case 40:
					case 140:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) &
                                                			~ActiveCheatTable[i].Data);
						break;
					case 41:
					case 141:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) &
		                                                	~ActiveCheatTable[i].Data);
                                    DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						break;
					case 42:
					case 142:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) &
            		                                    	~ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 1*60;
						break;
					case 43:
					case 143:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) &
                        		                        	~ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 2*60;
						break;
					case 44:
					case 144:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                                		ActiveCheatTable[i].Address) &
                                    		            	~ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Count = 5*60;
						break;
					/* JCK 980917 END */

					/* JCK 981030 BEGIN */
					case 60:
					case 61:
					case 62:
					case 63:
					case 64:
					case 65:
						ActiveCheatTable[i].Count = 1;
						ActiveCheatTable[i].Special += 200;    /* JCK 981212 */
						ActiveCheatTable[i].Backup =
							RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                               		ActiveCheatTable[i].Address);
						break;
					case 70:
					case 71:
					case 72:
					case 73:
					case 74:
					case 75:
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
                                    DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						break;
					/* JCK 981030 END */

						/*Special case, linked with 5,6,7 */
					case 205:    /* JCK 981212 */
					case 305:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 5;
						break;
					case 206:    /* JCK 981212 */
					case 306:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 6;
						break;
					case 207:    /* JCK 981212 */
					case 307:    /* JCK 981212 Linked cheat */
						WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    		ActiveCheatTable[i].Address,
								ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 7;
						break;

					/*Special case, linked with 8,9,10,11 */
					/* Change the memory only if the memory decreased by X */
					case 208:    /* JCK 981212 */
					case 308:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
	                                   			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Backup-1)
							WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address,
									ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 8;
						break;
					case 209:    /* JCK 981212 */
					case 309:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
	                                    	ActiveCheatTable[i].Backup-2)
							WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address,
									ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 9;
						break;
					case 210:    /* JCK 981212 */
					case 310:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
	                                    	ActiveCheatTable[i].Backup-3)
							WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address,
									ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 10;
						break;
					case 211:    /* JCK 981212 */
					case 311:    /* JCK 981212 Linked cheat */
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Backup-4)
							WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address,
									ActiveCheatTable[i].Data);
						ActiveCheatTable[i].Special = 11;
						break;

					/* JCK 981204 BEGIN */
					/*Special case, linked with 60 .. 65 */
					/* Change the memory only if the memory has changed since the last backup */
					case 260:
					case 261:
					case 262:
					case 263:
					case 264:
					case 265:
						if (	RD_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address) !=
                                    		ActiveCheatTable[i].Backup)
						{
							WR_GAMERAM (ActiveCheatTable[i].CpuNo,
                                    			ActiveCheatTable[i].Address,
									ActiveCheatTable[i].Data);
                                    	DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						}
						break;
					/* JCK 981204 END */

          		}	/* end switch */
        	} /* end if (ActiveCheatTable[i].Count == 0) */
        	else
		{
			ActiveCheatTable[i].Count--;
		}
		} /* end else */
	} /* end for */


  /* OSD_KEY_CHEAT_TOGGLE Enable/Disable the active cheats on the fly. Required for some cheat */
  if ( osd_key_pressed_memory( OSD_KEY_CHEAT_TOGGLE ) && ActiveCheatTotal )    /* JCK 981212 */
  {
	y = (MachHeight - FontHeight) / 2;
      /* JCK 981212 BEGIN */
      CheatEnabled ^= 1;
      xprintf(0, 0,y,"Cheats %s", (CheatEnabled ? "On" : "Off"));
      /* JCK 981212 END */

	/* JRT3 10-23-97 BEGIN */
	osd_clearbitmap( Machine -> scrbitmap );        /* Clear Screen */
	(Machine->drv->vh_update)(Machine->scrbitmap,1);  /* Make Game Redraw Screen */
	/* JRT3 10-23-97 END */
  }

  /* JCK 980917 BEGIN */
  /* OSD_KEY_INSERT toggles the Watch display ON */
  if ( osd_key_pressed_memory( OSD_KEY_INSERT ) && (WatchEnabled == 0) )
  {
	WatchEnabled = 1;
  }
  /* OSD_KEY_DEL toggles the Watch display OFF */
  if ( osd_key_pressed_memory( OSD_KEY_DEL ) && (WatchEnabled != 0) ){
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
	if (i)
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
  char str2[6][50];	/* JCK 981213 */
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
  if ((FontWidth * (int)strlen(str2[0])) > MachWidth - Machine->uixmin)    /* JCK 981213 */
	sprintf(str2[0],"%s",LoadedCheatTable[CheatNo].Name);
  sprintf(str2[1],"CPU:        %01X",LoadedCheatTable[CheatNo].CpuNo);    /* JCK 981022 */

  sprintf(str2[2], FormatAddr(LoadedCheatTable[CheatNo].CpuNo,1),
				LoadedCheatTable[CheatNo].Address);    /* JCK 981212 */

  sprintf(str2[3],"Value:    %03d  (0x%02X)",LoadedCheatTable[CheatNo].Data,LoadedCheatTable[CheatNo].Data);
  sprintf(str2[4],"Type:      %02d",LoadedCheatTable[CheatNo].Special);

  /* JCK 981213 BEGIN */
  sprintf(str2[5],"More: %s",LoadedCheatTable[CheatNo].More);
  if ((FontWidth * (int)strlen(str2[5])) > MachWidth - Machine->uixmin)    /* JCK 981213 */
	sprintf(str2[5],"%s",LoadedCheatTable[CheatNo].More);
  /* JCK 981213 END */

  for (i = 0;i < 6;i ++)     /* JCK 981213 */
  {
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
					sprintf(str2[2], FormatAddr(LoadedCheatTable[CheatNo].CpuNo,1),
						LoadedCheatTable[CheatNo].Address);    /* JCK 981212 */
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
                              ClearTextLine(1, dt[4].y);    /* JCK 981214 */
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
                                          /* JCK 981212 BEGIN */
							case 100:
								LoadedCheatTable[CheatNo].Special = 75;
								break;
							case 120:
								LoadedCheatTable[CheatNo].Special = 111;
								break;
							case 140:
								LoadedCheatTable[CheatNo].Special = 124;
								break;
                                          /* JCK 981212 END */
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
					sprintf (str2[2], FormatAddr(LoadedCheatTable[CheatNo].CpuNo,1),
						LoadedCheatTable[CheatNo].Address);    /* JCK 981212 */
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
                              ClearTextLine(1, dt[4].y);    /* JCK 981214 */
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
                                          /* JCK 981212 BEGIN */
							case 75:
								LoadedCheatTable[CheatNo].Special = 100;
								break;
							case 111:
								LoadedCheatTable[CheatNo].Special = 120;
								break;
							case 124:
								LoadedCheatTable[CheatNo].Special = 140;
								break;
                                          /* JCK 981212 END */
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
				sprintf(str2[2], FormatAddr(LoadedCheatTable[CheatNo].CpuNo,1),
					LoadedCheatTable[CheatNo].Address);    /* JCK 981212 */
			}
			break;

		case OSD_KEY_F10:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
			EditCheatHelp();								/* Show Help */
			y = EditCheatHeader();    /* JCK 981022 */
			break;

		case OSD_KEY_ENTER:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
                  /* JCK 981213 BEGIN */
			switch (s)
			{
				case 0:	/* Name */
					for (i = 0; i < total; i++)
						dt[i].color = DT_COLOR_WHITE;
					displaytext (dt, 0,1);
					xprintf (0, 0, EditYPos-2*FontHeight, "Edit Cheat Description:");
                        	xedit(0, EditYPos, LoadedCheatTable[CheatNo].Name, CHEAT_NAME_MAXLEN);
					sprintf (str2[0], "Name: %s", LoadedCheatTable[CheatNo].Name);
					if ((FontWidth * (int)strlen(str2[0])) > MachWidth - Machine->uixmin)
						sprintf(str2[0],"%s",LoadedCheatTable[CheatNo].Name);
					osd_clearbitmap (Machine->scrbitmap);
					y = EditCheatHeader();    /* JCK 981022 */
                              break;
				case 5:	/* More */
					for (i = 0; i < total; i++)
						dt[i].color = DT_COLOR_WHITE;
					displaytext (dt, 0,1);
					xprintf (0, 0, EditYPos-2*FontHeight, "Edit Cheat More Description:");
                        	xedit(0, EditYPos, LoadedCheatTable[CheatNo].More, CHEAT_NAME_MAXLEN);
					sprintf (str2[5], "More: %s", LoadedCheatTable[CheatNo].More);
					if ((FontWidth * (int)strlen(str2[5])) > MachWidth - Machine->uixmin)
						sprintf(str2[5],"%s",LoadedCheatTable[CheatNo].More);
					osd_clearbitmap (Machine->scrbitmap);
					y = EditCheatHeader();
                              break;
			}
			break;
                  /* JCK 981213 END */

		case OSD_KEY_ESC:
		case OSD_KEY_TAB:
			while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
			done = 1;
			break;
	}
  } while (done == 0);

  while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

  /* JCK 981020 BEGIN */
  if (	(LoadedCheatTable[CheatNo].Special==62) || (LoadedCheatTable[CheatNo].Special==65)	||
      	(LoadedCheatTable[CheatNo].Special==72) || (LoadedCheatTable[CheatNo].Special==75)	)
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


/* VM 981213 BEGIN */
/*
 * I don't fully understand how dt and str2 are utilized, so for now I pass them as parameters
 * until I can figure out which function they ideally belong in.
 * They should not be turned into globals; this program has too many globals as it is.
 */
int build_cheat_list(int Index, struct DisplayText ext_dt[40], char ext_str2[MAX_DISPLAYCHEATS + 1][40])
{
	int total = 0;
	while (total < MAX_DISPLAYCHEATS)
	{
		if (Index + total >= LoadedCheatTotal)
		{
			break;
		}

		AddCheckToName(total + Index, ext_str2[total]);
		ext_dt[total].text = ext_str2[total];

		total++;
	}
	ext_dt[total].text = 0; /* terminate array */

	/* Clear old list */
	ClearArea(1, YHEAD_SELECT, YFOOT_SELECT);    /* JCK 981207 */

	return total;
}
/* VM 981213 END */

/* JCK 981214 BEGIN */
int SelectCheatHeader(void)
{
  int y = 0;

  osd_clearbitmap (Machine->scrbitmap);	/* Clear Screen */ /* JCK 981216 */

  if (LoadedCheatTotal == 0)
  {
	xprintf(0, 0, y, "<INS>: Add New Cheat" );

	/* JCK 981016 BEGIN */
	y += (FontHeight);
	xprintf(0, 0, y, "<F10>: Show Help + other keys");
	/* JCK 981016 END */

	y += (FontHeight * 3);
	xprintf(0, 0, y, "No Cheats Available!");
  }
  else
  {
	xprintf(0, 0, y, "<DEL>: Delete  <INS>: Add" );
	y += (FontHeight);
	xprintf(0, 0, y, "<F1>: Save  <F2>: Watch");
	y += (FontHeight + 0);

	/* JCK 981006 BEGIN */
	xprintf(0, 0, y, "<F3>: Edit  <F6>: Save all");
	y += (FontHeight);
	xprintf(0, 0, y, "<F4>: Copy  <F7>: All off");    /* JCK 981214 */
	y += (FontHeight);
	xprintf(0, 0, y, "<ENTER>: Enable/Disable");
	y += (FontHeight);
	xprintf(0, 0, y, "<F10>: Show Help + other keys");
	y += (FontHeight + 4);
	/* JCK 981006 END */

	xprintf(0, 0, y, "Select a Cheat (%d Total)", LoadedCheatTotal);
	y += (FontHeight + 4);
  }
  return(YHEAD_SELECT);
}
/* JCK 981214 END */

void SelectCheat(void)
{
	int i, x, y, highlighted, key, done, total;
	int iWhereY = 0;
	struct DisplayText dt[40];
	int flag;
	int Index;

	int BCDOnly, ZeroPoss;        /* JCK 981020 */

	char str2[MAX_DISPLAYCHEATS + 1][40];    /* JCK 981030 */
	int j;    /* JCK 981030 */

	char fmt[32];    /* Moved by JCK 981204 */
	char buf[40];    /* JCK 981207 */

	osd_clearbitmap(Machine->scrbitmap);

	/* VM 981213 BEGIN */
	if (MachWidth < FontWidth * 35)
	{
        	x = 0;
	}
	else
	{
        	x = (MachWidth / 2) - (FontWidth * 16);
	}
     	y = SelectCheatHeader();    /* JCK 981214 */
	/* VM 981213 END */

	/* JCK 981215 BEGIN */
	/* Make the list */
	for (i = 0; i < MAX_DISPLAYCHEATS; i++)
	{
		dt[i].x = x;
		dt[i].y = y;
		dt[i].color = DT_COLOR_WHITE;
		y += FontHeight;
	}

	Index = 0;
	total = build_cheat_list(Index, dt, str2);
	/* JCK 981215 END */

	y += (FontHeight * 2);    /* JCK 981026 */

	highlighted = 0;
	done = 0;
	do
	{
		for (i = 0; i < total; i++)
		{
			dt[i].color = (i == highlighted) ? DT_COLOR_YELLOW : DT_COLOR_WHITE;
		}

		displaytext(dt, 0, 1);

		key = osd_read_keyrepeat(0);
		ClearTextLine(1, YFOOT_SELECT);
		switch (key)
		{
			case OSD_KEY_DOWN:

				if (highlighted < total - 1)
				{
					highlighted++;
				}
				else
				{
					highlighted = 0;

					if (LoadedCheatTotal <= MAX_DISPLAYCHEATS)    /* JCK 980917 */
					{
						break;
					}

					if (LoadedCheatTotal > Index + MAX_DISPLAYCHEATS)    /* JCK 980917 */
					{
						Index += MAX_DISPLAYCHEATS;
					}
					else
					{
						Index = 0;

					}

					/* VM 981213 */
					total = build_cheat_list(Index, dt, str2);
				}
				break;

			case OSD_KEY_UP:
				if (highlighted > 0)
				{
					highlighted--;
				}
				else
				{
					highlighted = total - 1;
					if (LoadedCheatTotal <= MAX_DISPLAYCHEATS)    /* JCK 980917 */
					{
						break;
					}

					if (Index == 0)
					{
						Index = ((LoadedCheatTotal - 1) / MAX_DISPLAYCHEATS) * MAX_DISPLAYCHEATS;    /* JCK 980917 */
					}
					else if (Index > MAX_DISPLAYCHEATS)    /* JCK 980917 */
					{
						Index -= MAX_DISPLAYCHEATS;
					}
					else
					{
						Index = 0;
					}

					/* VM 981213 */
					total = build_cheat_list(Index, dt, str2);
					highlighted = total - 1;
				}
				break;

			case OSD_KEY_HOME:
				Index = 0;

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				break;

			case OSD_KEY_END:
				Index = ((LoadedCheatTotal - 1) / MAX_DISPLAYCHEATS) * MAX_DISPLAYCHEATS;

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				highlighted = total - 1;
				break;

			case OSD_KEY_PGDN:
				if (highlighted + Index >= LoadedCheatTotal - MAX_DISPLAYCHEATS)
				{
					Index = ((LoadedCheatTotal - 1) / MAX_DISPLAYCHEATS) * MAX_DISPLAYCHEATS;
					highlighted = (LoadedCheatTotal - 1) - Index;
				}
				else
				{
					Index += MAX_DISPLAYCHEATS;
				}

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				break;

			case OSD_KEY_PGUP:
				if (highlighted + Index <= MAX_DISPLAYCHEATS)
				{
					Index = 0;
					highlighted = 0;
				}
				else
				{
					Index -= MAX_DISPLAYCHEATS;
				}

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				break;

			case OSD_KEY_INSERT:    /* Add a new empty cheat */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JRT5 Print Message If Cheat List Is Full */
				if (LoadedCheatTotal > MAX_LOADEDCHEATS -1)
				{
					xprintf(0, 0, YFOOT_SELECT, "(Cheat List Is Full.)" );    /* JCK 981207 */
					break;
				}

				set_cheat(&LoadedCheatTable[LoadedCheatTotal], NEW_CHEAT);
				LoadedCheatTotal++;

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				/* JCK 981214 END */

		      	if (LoadedCheatTotal == 1)
					SelectCheatHeader();    /* JCK 981216 */

				break;

			case OSD_KEY_DEL:
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981214 BEGIN */
				/* Erase the current cheat from the list */
				/* But before, erase it from the active list if it is there */
				for (i = 0;i < ActiveCheatTotal;i ++)    /* JCK 981212 */
				{
					if (ActiveCheatTable[i].Address == LoadedCheatTable[highlighted + Index].Address)
						if (ActiveCheatTable[i].Data == LoadedCheatTable[highlighted + Index].Data)
						{
							/* The selected Cheat is already in the list then delete it.*/
							DeleteActiveCheatFromTable(i);    /* JCK 981212 */
							break;
						}
				}
				/* Delete entry from list */
				DeleteLoadedCheatFromTable(highlighted + Index);    /* JCK 981212 */
				/* JCK 981214 END */

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);

				if (total <= highlighted)
				{
					highlighted = total - 1;
				}

				if ((total == 0) && (Index != 0))    /* JCK 981214 */
				{
					/* The page is empty so backup one page */
					if (Index == 0)
					{
						Index = ((LoadedCheatTotal - 1) / MAX_DISPLAYCHEATS) * MAX_DISPLAYCHEATS;
					}
					else if (Index > MAX_DISPLAYCHEATS)    /* JCK 980917 */
					{
						Index -= MAX_DISPLAYCHEATS;
					}
					else
					{
						Index = 0;
					}

					/* VM 981213 */
					total = build_cheat_list(Index, dt, str2);
					highlighted = total - 1;
				}

		      	if (LoadedCheatTotal == 0)
					SelectCheatHeader();    /* JCK 981216 */

				break;

			/* JCK 981212 BEGIN */
			case OSD_KEY_PLUS_PAD:
				/* Display comment about a cheat */
				while (osd_key_pressed(key)) ; /* wait for key release */
				if (LoadedCheatTable[highlighted + Index].More[0])
					xprintf (0, 0, YFOOT_SELECT, "%s", LoadedCheatTable[highlighted + Index].More);
				break;
			/* JCK 981212 END */

			case OSD_KEY_F1:    /* Save cheat to file */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

                        /* JCK 981214 BEGIN */
                        j = SaveCheat(highlighted + Index);
				xprintf(0, 0, YFOOT_SELECT, "Cheat %sSaved to File %s", (j ? "" : "NOT "), cheatfile);
                        /* JCK 981214 END */

				break;

			case OSD_KEY_F2:     /* Add to watch list */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				/* JCK 981211 BEGIN */
				if (LoadedCheatTable[highlighted + Index].Special == COMMENTCHEAT)
				{
					xprintf (0, 0, YFOOT_SELECT, "Comment NOT Added as Watch");
					break;
				}
				/* JCK 981211 END */

				if (LoadedCheatTable[highlighted + Index].CpuNo < cpu_gettotalcpu())  /* watches are for all cpus */ /* JCK 981008 */
				{
					for (i = 0; i < MAX_WATCHES; i++)    /* JCK 980917 */
					{
						if (Watches[i] == MAX_ADDRESS(WatchesCpuNo[i]))    /* JCK 981009 */
						{
							WatchesCpuNo[i] = LoadedCheatTable[highlighted + Index].CpuNo;    /* JCK 981008 */
							Watches[i] = LoadedCheatTable[highlighted + Index].Address;

							/* JCK 981212 BEGIN */
							strcpy(fmt, FormatAddr(SearchCpuNo,0));
							sprintf (buf, fmt, Watches[i]);
							xprintf (0, 0, YFOOT_SELECT, "%s Added as Watch %d",buf,i+1);    /* JCK 981207 */
							/* JCK 981212 END */

							WatchesFlag = 1;
							WatchEnabled = 1;    /* JCK 980917 */
							break;
						}
					}
				}
				break;

			case OSD_KEY_F3:    /* Edit current cheat */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				/* JCK 981211 BEGIN */
				if (LoadedCheatTable[highlighted + Index].Special == COMMENTCHEAT)
					xedit(0, YFOOT_SELECT, LoadedCheatTable[highlighted + Index].Name, CHEAT_NAME_MAXLEN);    /* JCK 981212 */
				else
					EditCheat(highlighted+Index);
				/* JCK 981211 END */

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);

		      	SelectCheatHeader();    /* JCK 981216 */

				break;

			case OSD_KEY_F4:    /* Copy the current cheat */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				if (LoadedCheatTotal > MAX_LOADEDCHEATS - 1)
				{
					xprintf(0, 0, YFOOT_SELECT, "(Cheat List Is Full.)" );    /* JCK 981207 */
					break;
				}

				set_cheat(&LoadedCheatTable[LoadedCheatTotal], &LoadedCheatTable[highlighted + Index]);
				LoadedCheatTable[LoadedCheatTotal].Count = 0;
				LoadedCheatTotal++;

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				/* JCK 981214 END */

				break;

			case OSD_KEY_F5:    /* Rename the cheatfile and reload the database */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JCK 981212 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					strcpy(buf, cheatfile);
					if (RenameCheatFile(1, 0, buf) == 1)
						LoadCheat(1, buf);
				}
				else
				{
					if (RenameCheatFile(0, 1, cheatfile) == 1)
						LoadCheat(0, cheatfile);
				}
				/* JCK 981212 END */

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;

		      	SelectCheatHeader();    /* JCK 981216 */

				/* JCK 981214 END */

				break;

			case OSD_KEY_F6:    /* Save all cheats to file */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

                        /* JCK 981214 BEGIN */
                        j = SaveCheat(-1);
				xprintf(0, 0, YFOOT_SELECT, "%d Cheats Saved to File %s", j, cheatfile);
                        /* JCK 981214 END */

				break;

			case OSD_KEY_F7:    /* Remove all active cheats from the list */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				ActiveCheatTotal = 0;    /* JCK 981212 */

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				break;

			case OSD_KEY_F8:    /* Reload the database */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				LoadCheat(0, cheatfile);    /* JCK 981212 */

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				/* JCK 981214 END */

		      	SelectCheatHeader();    /* JCK 981216 */

				break;

			case OSD_KEY_F9:    /* Rename the cheatfile */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				RenameCheatFile(-1, 1, cheatfile);    /* JCK 981212 */

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				/* JCK 981214 END */

		      	SelectCheatHeader();    /* JCK 981216 */

				break;

			case OSD_KEY_F10:    /* Invoke help */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JCK 981016 BEGIN */
				if (LoadedCheatTotal == 0)
				{
					CheatListHelpEmpty();
				}
				else
				{
					CheatListHelp();
				}
				/* JCK 981016 END */

		      	SelectCheatHeader();    /* JCK 981216 */

				break;

			/* JCK 981213 BEGIN */
			case OSD_KEY_F11:    /* Toggle sologame ON/OFF then reload the database */
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				sologame ^= 1;
				LoadCheat(0, cheatfile);

				/* JCK 981214 BEGIN */
				/* goto HardRefresh; */
				Index = 0;
				total = build_cheat_list(Index, dt, str2);
				highlighted = 0;
				/* JCK 981214 END */

		      	SelectCheatHeader();    /* JCK 981216 */

				break;
			/* JCK 981213 END */

			/* JCK 981207 BEGIN */
			case OSD_KEY_F12:    /* Display info about a cheat */
				while (osd_key_pressed(key)) ; /* wait for key release */
				if (LoadedCheatTotal == 0)
				{
					break;
				}

				/* JCK 981216 BEGIN */
				if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
				{
					break;
				}
				/* JCK 981216 END */

				/* JCK 981211 BEGIN */
				if (LoadedCheatTable[highlighted + Index].Special == COMMENTCHEAT)
				{
					xprintf (0, 0, YFOOT_SELECT, "This is a Comment");
					break;
				}
				/* JCK 981211 END */

                        /* JCK 981214 BEGIN */
	                  AddCpuToAddr(LoadedCheatTable[highlighted + Index].CpuNo,
                        		 LoadedCheatTable[highlighted + Index].Address,
                                     LoadedCheatTable[highlighted + Index].Data, fmt);
				strcat(fmt," [Type %d]");
				sprintf(buf, fmt, LoadedCheatTable[highlighted + Index].Special);
                        /* JCK 981214 END */

				xprintf(0, 0, YFOOT_SELECT, "%s", buf);

				break;
			/* JCK 981207 END */

			case OSD_KEY_ENTER:
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */
				if (total == 0)
				{
					break;
				}

				/* JCK 981211 BEGIN */
				if (LoadedCheatTable[highlighted + Index].Special == COMMENTCHEAT)
				{
					xprintf (0, 0, YFOOT_SELECT, "Comment NOT Activated");    /* JCK 981214 */
					break;
				}
				/* JCK 981211 END */

				/* JCK 981214 BEGIN */
				flag = 0;
				for (i = 0;i < ActiveCheatTotal;i ++)
				{
					if (ActiveCheatTable[i].Address == LoadedCheatTable[highlighted + Index].Address)
					{
						/* JCK 981216 BEGIN */
						if (	((ActiveCheatTable[i].Special >= 20) && (ActiveCheatTable[i].Special <= 24))	||
							((ActiveCheatTable[i].Special >= 40) && (ActiveCheatTable[i].Special <= 44))	)
						{
							if (	(ActiveCheatTable[i].Special 	!= LoadedCheatTable[highlighted + Index].Special)	||
								(ActiveCheatTable[i].Data 	!= LoadedCheatTable[highlighted + Index].Data	)	)
								continue;
						}
						/* JCK 981216 END */

						/* The selected Cheat is already in the list then delete it.*/
						DeleteActiveCheatFromTable(i);    /* JCK 981212 */
						flag = 1;

						/* JCK 981212 BEGIN */
						/* Delete linked cheats */
						while (i < ActiveCheatTotal)
						{
							if ((ActiveCheatTable[i].Special < 100) ||
									(ActiveCheatTable[i].Special == COMMENTCHEAT))
								break;
							DeleteActiveCheatFromTable(i);
						}
						/* JCK 981212 END */

						break;
					}
				}
				/* JCK 981214 END */

				/* Add the selected cheat to the active cheats list if it was not already there */
				if (flag == 0)
				{
					/* No more than MAX_ACTIVECHEATS cheat at the time */
					if (ActiveCheatTotal > MAX_ACTIVECHEATS-1)    /* JCK 981212 */
					{
						xprintf( 0, 0, YFOOT_SELECT, "(Limit Of Active Cheats)" );    /* JCK 981214 */
						break;
					}

					set_cheat(&ActiveCheatTable[ActiveCheatTotal], &LoadedCheatTable[highlighted + Index]);    /* JCK 981214 */
					ActiveCheatTable[ActiveCheatTotal].Count = 0;
					ValTmp = 0;

					/* JCK 981214 BEGIN */
					if (	(ActiveCheatTable[ActiveCheatTotal].Special>=60)	&&
						(ActiveCheatTable[ActiveCheatTotal].Special<=75)	)
					{
						ActiveCheatTable[ActiveCheatTotal].Data =
							RD_GAMERAM (ActiveCheatTable[ActiveCheatTotal].CpuNo,
									ActiveCheatTable[ActiveCheatTotal].Address);
						BCDOnly = (	(ActiveCheatTable[ActiveCheatTotal].Special == 63)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 64)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 65)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 73)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 74)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 75)	);
						ZeroPoss = ((ActiveCheatTable[ActiveCheatTotal].Special == 60)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 63)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 70)	||
								(ActiveCheatTable[ActiveCheatTotal].Special == 73)	);

						ValTmp = SelectValue(ActiveCheatTable[ActiveCheatTotal].Data,
										BCDOnly, ZeroPoss, 0, 0,
										ActiveCheatTable[ActiveCheatTotal].Minimum,
										ActiveCheatTable[ActiveCheatTotal].Maximum,
										"%03d", "Enter the new Value:", 1,
										FIRSTPOS + 3 * FontHeight);
						if (ValTmp > NOVALUE)
							ActiveCheatTable[ActiveCheatTotal].Data = ValTmp;
					}
					/* JCK 981214 END */

					if (ValTmp > NOVALUE)    /* JCK 981214 */
					{
						ActiveCheatTotal ++;    /* JCK 981212 */
						CheatEnabled = 1;
						he_did_cheat = 1;

						/* JCK 981212 BEGIN */
						/* Activate linked cheats */
						for (i = highlighted + Index + 1; i < LoadedCheatTotal && ActiveCheatTotal < MAX_ACTIVECHEATS; i++)
						{
							if ((LoadedCheatTable[i].Special < 100) ||
									(LoadedCheatTable[i].Special == COMMENTCHEAT))
								break;
							set_cheat(&ActiveCheatTable[ActiveCheatTotal], &LoadedCheatTable[i]);    /* JCK 981214 */
							ActiveCheatTable[ActiveCheatTotal].Count = 0;
							ActiveCheatTotal ++;
						}
						/* JCK 981212 END */
					}
				}

				osd_clearbitmap(Machine->scrbitmap);

		      	SelectCheatHeader();    /* JCK 981216 */

				/* VM 981213 */
				total = build_cheat_list(Index, dt, str2);
				break;

			case OSD_KEY_ESC:
			case OSD_KEY_TAB:
				while (osd_key_pressed(key)) ; /* wait for key release */ /* moved by JCK 981016 */

				done = 1;
				break;
		}
	}
	while (done == 0);

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
  int y = 0;    /* JCK 981214 */
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

void ContinueSearchMatchFooter(int count, int idx)
{
  int i = 0;
  int FreeWatch = 0;

  int y = YFOOT_MATCH;

  y += FontHeight;
  if (LoadedCheatTotal >= MAX_LOADEDCHEATS)
	xprintf(0, 0,y,"(Cheat List Is Full.)");
  else
	ClearTextLine(1, y);    /* JCK 981212 */
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

  if (FindFreeWatch())    /* JCK 981214 */
	xprintf(0, 0,y,"<F2>: Add Entry To Watches");
  else
	ClearTextLine(1, y);    /* JCK 981207 */
  y += FontHeight;

  if (LoadedCheatTotal < MAX_LOADEDCHEATS)
  {
	xprintf(0, 0,y,"<F1>: Add Entry To List");
	y += FontHeight;
	xprintf(0, 0,y,"<F6>: Add All To List");
	y += 2*FontHeight;
  }
  else
  {
	/* for (i=y; i< ( y + 3 * FontHeight + 1 ); i++) memset (Machine->scrbitmap->line[i], 0, MachWidth); */
	ClearArea(1, y, y + 2 * FontHeight);    /* JCK 981212 */
  }
}
/* JCK 981023 END */

/* JCK 981216 BEGIN */
int SelectFastSearchHeader(void)
{
  int y = FIRSTPOS;

  osd_clearbitmap(Machine->scrbitmap);

  xprintf(0, 0, y, "<F10>: Show Help");
  y += 2*FontHeight;
  xprintf(0, 0,y,"Choose One Of The Following:");
  y += 2*FontHeight;
  return(y);
}

void SelectFastSearch(void)
{
  int y;
  int s,key,done;
  int total;

  char *paDisplayText[] = {
		"Scan All Memory (Slow But Sure)",
		"Scan all RAM and BANKS (Normal)",
		"Scan one BANK (Fastest Search)",
		"",
		"Return To Start Search Menu",
		0 };

  struct DisplayText dt[10];

  y = SelectFastSearchHeader();

  total = CreateMenu(paDisplayText, dt, y);

  s = fastsearch;
  done = 0;
  do
  {
	key = SelectMenu (&s, dt, 0, 1, 0, total-1, 0, &done);
	switch (key)
	{
		case OSD_KEY_F10:
			SelectFastSearchHelp();
			done = 0;
			StartSearchHeader();    /* JCK 981023 */
			break;

		case OSD_KEY_ENTER:
			switch (s)
			{
				case 0:
					if (fastsearch != 0)
						SearchCpuNoOld = -1;    /* Force tables to be built */
					fastsearch = 0;
					done = 1;
					break;

				case 1:
					if (fastsearch != 1)
						SearchCpuNoOld = -1;    /* Force tables to be built */
					fastsearch = 1;
					done = 1;
					break;

				case 2:
					if (fastsearch != 2)
						SearchCpuNoOld = -1;    /* Force tables to be built */
					fastsearch = 2;
					done = 1;
					break;

				case 4:
					done = 2;
					break;
			}
			break;
	}
  } while ((done != 1) && (done != 2));

  osd_clearbitmap(Machine->scrbitmap);
}
/* JCK 981216 END */

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
		/* JCK 981216 BEGIN */
		"",
		"Change Search Speed",
		/* JCK 981216 END */
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
			StartSearchHelp();								/* Show Help */
			done = 0;
			StartSearchHeader();    /* JCK 981023 */
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

				/* JCK 981216 BEGIN */
				case 6:
					SelectFastSearch();
					done = 0;
					StartSearchHeader();
					break;
				/* JCK 981216 END */

				case 8:    /* JCK 981216 */
					done = 2;
					break;
			}
			break;
	}
  } while ((done != 1) && (done != 2));    /* JCK 981212 */
  /* JCK 981023 END */

  osd_clearbitmap(Machine->scrbitmap);

  /* User select to return to the previous menu */
  if (done == 2)
	return;

  /* JCK 981006 BEGIN */
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
	/* JCK 981023 END */

	/* JCK 981213 END */
	if (ValTmp == 2 * NOVALUE)
	{
		CurrentMethod = SaveMethod;
		StartSearch();
		return;
	}
	/* JCK 981213 END */

	s = ValTmp;
  }
  else
  {
	s = 0;
  }
  SearchCpuNo = s;
/* JCK 981006 END */

  osd_clearbitmap(Machine->scrbitmap);

  /* If the method 1 is selected, ask for a number */
  if (CurrentMethod == Method_1)
  {
	/* JCK 981023 BEGIN */
	ValTmp = SelectValue(0, 0, 1, 1, 1, 0, 0xFF,
					"%03d  (0x%02X)", "Enter Value To Search For:", 1, FIRSTPOS + 3 * FontHeight);
	osd_clearbitmap(Machine->scrbitmap);
	if (ValTmp == NOVALUE)
	{
		CurrentMethod = SaveMethod;
		return;
	}
	/* JCK 981023 END */

	/* JCK 981213 END */
	if (ValTmp == 2 * NOVALUE)
	{
		CurrentMethod = SaveMethod;
		StartSearch();
		return;
	}
	/* JCK 981213 END */

	s = ValTmp;
  }
  else
  {
	s = 0;    /* JCK 981023 */
  }
  StartValue = s;    /* JCK 981023 */

  osd_clearbitmap(Machine->scrbitmap);

  /* JCK 981206 BEGIN */
  /* build ram tables to match game ram */
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

  y = FIRSTPOS + (10 * FontHeight);
  SearchInProgress(1,y);    /* JCK 981212 */

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

	SearchInProgress(0,y);    /* JCK 981212 */
	xprintf(0, 0,y,"Matches Found: %d",count);
  }
  else
  {
	SearchInProgress(0,y);    /* JCK 981212 */
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
  char str2[MAX_MATCHES+2][80];
  int i,j,x,y,count,s,key,done;
  struct DisplayText dt[20];
  int total;
  int Continue;
  struct ExtMemory *ext;	/* JB 980407 */
  struct ExtMemory *ext_br;	/* JB 980407 */
  struct ExtMemory *ext_sr;	/* JB 980407 */

  /* JCK 981023 BEGIN */
  int Index = 0;
  int countAdded;
  /* JCK 981023 END */

  char fmt[40];    /* JCK 981022 */

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
	/* JCK 981023 END */

	/* JCK 981213 END */
	if (ValTmp == 2 * NOVALUE)
	{
		CurrentMethod = SaveMethod;
		StartSearch();
		return;
	}
	/* JCK 981213 END */

	s = ValTmp;    /* JCK 981023 */

	StartValue = s; /* Save the value for when continue */
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
	/* JCK 981023 END */

	/* JCK 981213 END */
	if (ValTmp == 2 * NOVALUE)
	{
		CurrentMethod = SaveMethod;
		StartSearch();
		return;
	}
	/* JCK 981213 END */

	s = ValTmp;    /* JCK 981023 */

	/* JRT5 For Different Text Depending On Start/Continue Search */
	iCheatInitialized = 0;
	/* JRT5 */

	StartValue = s; /* Save the value for when continue */
  }


  /******** Method 3 ***********/
  if (CurrentMethod == Method_3)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] =
      {
		"New Value is Less",
		"New Value is Equal",
		"New Value is Greater",
		"",
		"Return To Cheat Menu",
		0
      };

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
	} while ((done != 1) && (done != 2));    /* JCK 981212 */
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;
  }


  /******** Method 4 ***********/
  /* Ask if the value is the same as when we start or the opposite */
  if (CurrentMethod == Method_4)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] =
      {
		"Bit is Same as Start",
		"Bit is Opposite from Start",
		"",
		"Return To Cheat Menu",
		0
      };

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
	} while ((done != 1) && (done != 2));    /* JCK 981212 */
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;
  }


  /******** Method 5 ***********/
  /* Ask if the value is the same as when we start or different */
  if (CurrentMethod == Method_5)
  {
	/* JCK 981023 BEGIN */
	char *paDisplayText[] =
      {
		"Memory is Same as Start",
		"Memory is Different from Start",
		"",
		"Return To Cheat Menu",
		0
      };

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
	} while ((done != 1) && (done != 2));    /* JCK 981212 */
	/* JCK 981023 END */

	osd_clearbitmap(Machine->scrbitmap);

	/* User select to return to the previous menu */
	if (done == 2)
		return;

	iCheatInitialized = 0;
  }

  /* JCK 981212 BEGIN */
  y = FIRSTPOS + (10 * FontHeight);
  SearchInProgress(1,y);    /* JCK 981212 */
  count = 0;
  for (ext = FlagTable, ext_sr = StartRam, ext_br = BackupRam; ext->data; ext++, ext_sr++, ext_br++)
  {
	for (i=0; i <= ext->end - ext->start; i++)
      {
		if (ext->data[i] != 0)
		{
            	int ValRead = RD_GAMERAM(SearchCpuNo, i+ext->start);
			switch (CurrentMethod)
			{
				case Method_1:    /* Value */
					if ((ValRead != s) && (ValRead != s-1))
						ext->data[i] = 0;
					break;
				case Method_2:    /* Timer */
					if (ValRead != (ext_br->data[i] + s))
						ext->data[i] = 0;
					break;
				case Method_3:    /* Energy */
                        	switch (s)
                        	{
                        		case 0:    /* Less */
							if (ValRead >= ext_br->data[i])
								ext->data[i] = 0;
                                    	break;
                        		case 1:    /* Equal */
							if (ValRead != ext_br->data[i])
								ext->data[i] = 0;
                                    	break;
                        		case 2:    /* Greater */
							if (ValRead <= ext_br->data[i])
								ext->data[i] = 0;
                                    	break;
                        	}
					break;
				case Method_4:    /* Bit */
                        	switch (s)
                        	{
                        		case 0:    /* Same */
							j = ValRead ^ (ext_sr->data[i] ^ 0xFF);
							ext->data[i] = j & ext->data[i];
                                    	break;
                        		case 1:    /* Opposite */
							j = ValRead ^ ext_sr->data[i];
							ext->data[i] = j & ext->data[i];
                                    	break;
                        	}
					break;
				case Method_5:    /* Byte */
                        	switch (s)
                        	{
                        		case 0:    /* Same */
							if (ValRead != ext_sr->data[i])
								ext->data[i] = 0;
                                    	break;
                        		case 1:    /* Different */
							if (ValRead == ext_sr->data[i])
								ext->data[i] = 0;
                                    	break;
                        	}
					break;
				default:
					ext->data[i] = 0;
					break;
			}
		}
		if (ext->data[i] != 0)
            	count ++;
      }
  }
  if ((CurrentMethod == Method_2) || (CurrentMethod == Method_3))
	backup_ram (BackupRam);
  SearchInProgress(0,y);    /* JCK 981212 */
  /* JCK 981212 END */


  /* For all methods:
    - Display how much locations we have found
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

  y += 2*FontHeight;

  total = 0;
  Continue = 0;

  /* JB 980407 */
  for (ext = FlagTable, ext_sr = StartRam; ext->data && Continue==0; ext++, ext_sr++)
  {
	for (i=0; i <= ext->end - ext->start; i++)
		if (ext->data[i] != 0)
		{
			/* JCK 981212 BEGIN */
			strcpy(fmt, FormatAddr(SearchCpuNo,0));
			strcat(fmt," = %02X");
			/* JCK 981212 END */

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
  ContinueSearchMatchHeader(count);

  s = 0;
  done = 0;
  do
  {
	int Begin = 0;

	ContinueSearchMatchFooter(count, Index);    /* JCK 981214 */

	key = SelectMenu (&s, dt, 1, 0, 0, total-1, 0, &done);

	ClearTextLine(1, YFOOT_MATCH);    /* JCK 981214 */

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
			ContinueSearchMatchHeader(count);

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
						/* JCK 981212 BEGIN */
						strcpy(fmt, FormatAddr(SearchCpuNo,0));
						strcat(fmt," = %02X");
						/* JCK 981212 END */

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
			ContinueSearchMatchHeader(count);

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
							/* JCK 981212 BEGIN */
							strcpy(fmt, FormatAddr(SearchCpuNo,0));
							strcat(fmt," = %02X");
							/* JCK 981212 END */

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
							/* JCK 981212 BEGIN */
							strcpy(fmt, FormatAddr(SearchCpuNo,0));
							strcat(fmt," = %02X");
							/* JCK 981212 END */

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

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			/* JCK 981212 BEGIN */
			strcpy(buf, dt[s].text);
			ptr = strtok(buf, "=");
			sscanf(ptr,"%X", &TrueAddr);
			ptr = strtok(NULL, "=");
			sscanf(ptr,"%02X", &TrueData);

                  AddCpuToAddr(SearchCpuNo, TrueAddr, TrueData, str2[MAX_MATCHES+1]);    /* JCK 981214 */

			/* Add the selected address to the LoadedCheatTable */
			if (LoadedCheatTotal < MAX_LOADEDCHEATS)    /* JCK 980917 */
			{
				set_cheat(&LoadedCheatTable[LoadedCheatTotal], NEW_CHEAT);   /* JCK 981214 */
				LoadedCheatTable[LoadedCheatTotal].CpuNo   = SearchCpuNo;    /* JCK 981008 */
				LoadedCheatTable[LoadedCheatTotal].Address = TrueAddr;
				LoadedCheatTable[LoadedCheatTotal].Data    = TrueData;
				strcpy(LoadedCheatTable[LoadedCheatTotal].Name, str2[MAX_MATCHES+1]);
				LoadedCheatTotal++;
				xprintf(0, 0,YFOOT_MATCH,"%s Added",str2[MAX_MATCHES+1]);    /* JCK 981023 */
			}
			else
				xprintf(0, 0,YFOOT_MATCH,"%s Not Added",str2[MAX_MATCHES+1]);    /* JCK 981023 */
			/* JCK 981212 END */

			break;

		case OSD_KEY_F2:
			while (osd_key_pressed(key)) ;

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			/* JCK 981214 BEGIN */
                  j = FindFreeWatch();
			if (j)
			{
				/* JCK 981125 BEGIN */
				strcpy(buf, dt[s].text);
				ptr = strtok(buf, "=");
				sscanf(ptr,"%X", &TrueAddr);
				/* JCK 981125 END */

				WatchesCpuNo[j-1] = SearchCpuNo;
				Watches[j-1] = TrueAddr;    /* JCK 981125 */
				WatchesFlag = 1;
				WatchEnabled = 1;
				strcpy(fmt, FormatAddr(SearchCpuNo,0));    /* JCK 981212 */
				sprintf (str2[MAX_MATCHES+1], fmt, Watches[j-1]);
				xprintf(0, 0,YFOOT_MATCH,"%s Added as Watch %d",str2[MAX_MATCHES+1],j);
			}
			/* JCK 981214 END */

			break;

		case OSD_KEY_F6:    /* JCK 981023 */
			while (osd_key_pressed(key)) ;

			if (count == 0)    /* JCK 981023 */
				break;

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			countAdded = 0;    /* JCK 981023 */

			/* JB 980407 and JCK 980917 */
			for (ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
			{
				for (i = 0; i <= ext->end - ext->start; i++)
				{
					if (LoadedCheatTotal > MAX_LOADEDCHEATS-1)
						break;
					if (ext->data[i] != 0)
					{
						countAdded++;    /* JCK 981023 */

						TrueAddr = i+ext->start;
						TrueData = ext_sr->data[i];

                  			AddCpuToAddr(SearchCpuNo, TrueAddr, TrueData, str2[MAX_MATCHES+1]);    /* JCK 981214 */

						set_cheat(&LoadedCheatTable[LoadedCheatTotal], NEW_CHEAT);   /* JCK 981214 */
						LoadedCheatTable[LoadedCheatTotal].CpuNo   = SearchCpuNo;    /* JCK 981008 */
						LoadedCheatTable[LoadedCheatTotal].Address = TrueAddr;
						LoadedCheatTable[LoadedCheatTotal].Data    = TrueData;
						strcpy(LoadedCheatTable[LoadedCheatTotal].Name,str2[MAX_MATCHES+1]);
						LoadedCheatTotal++;
					}
				}
				if (LoadedCheatTotal > MAX_LOADEDCHEATS)
					break;
			}

			xprintf(0, 0,YFOOT_MATCH,"%d Added",countAdded);    /* JCK 981023 */

			break;

	}
  } while (done != 2);
  /* JCK 981023 END */

  osd_clearbitmap(Machine->scrbitmap);
}

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

void ChooseWatchFooter(void)
{
  int i = 0;

  int y = YFOOT_WATCH;

  y += FontHeight;
  if (LoadedCheatTotal > MAX_LOADEDCHEATS-1)
	xprintf(0, 0,y,"(Cheat List Is Full.)");
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

  int countAdded;    /* JCK 981022 */

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
  /* JCK 981022 END */

  total = 0;

  for (i=0;i<MAX_WATCHES;i++)    /* JCK 980917 */
  {
	AddCpuToWatch(i, str2[ i ]);    /* JCK 981023 */

	dt[total].text = str2[ i ];

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
      DisplayWatches(1, &WatchX, &WatchY, buf);    /* JCK 981212 */
	ChooseWatchFooter();    /* JCK 981214 */
	countAdded = 0;    /* JCK 981022 */
	key = SelectMenu (&s, dt, 1, 0, 0, total-1, 0, &done);    /* JCK 981023 */
	ClearTextLine(1, YFOOT_WATCH);    /* JCK 981214 */
	switch (key)
	{
		case OSD_KEY_J:
			if (WatchX > Machine->uixmin)    /* JCK 981212 */
				WatchX--;
			break;

		case OSD_KEY_L:
			/* JRT1 10-23-97 BEGIN */
			if (WatchX <= ( MachWidth - ( FontWidth * (int)strlen( buf ) ) ) )
			/* JRT1 10-23-97 END */
				WatchX++;
			break;

		case OSD_KEY_K:                   /* (Minus Extra One)*/
			/* JRT1 10-23-97 BEGIN */
			if (WatchY <= (MachHeight-FontHeight) - 1 )
			/* JRT1 10-23-97 END */
				WatchY++;
			break;

		case OSD_KEY_I:
			if (WatchY > Machine->uiymin)    /* JCK 981212 */
				WatchY--;
			break;

		case OSD_KEY_MINUS_PAD:
		case OSD_KEY_LEFT:
			if (Watches[ s ] <= 0)
				Watches[ s ] = MAX_ADDRESS(WatchesCpuNo[ s ]);    /* JCK 981009 */
			else
				Watches[ s ]--;
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */
			break;

		case OSD_KEY_PLUS_PAD:
		case OSD_KEY_RIGHT:
			if (Watches[ s ] >= MAX_ADDRESS(WatchesCpuNo[ s ]))    /* JCK 981009 */
				Watches[ s ] = 0;
			else
				Watches[ s ]++;
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */
			break;

		case OSD_KEY_PGDN:
			if (Watches[ s ] <= 0x100)
				Watches[ s ] |= (0xFFFFFF00 & MAX_ADDRESS(WatchesCpuNo[ s ]));    /* JCK 981009 */
			else
				Watches[ s ] -= 0x100;
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */
			break;

		case OSD_KEY_PGUP:
			if (Watches[ s ] >= 0xFF00)
				Watches[ s ] |= (0xFFFF00FF & MAX_ADDRESS(WatchesCpuNo[ s ]));    /* JCK 981009 */
			else
				Watches[ s ] += 0x100;
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */
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
				AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */

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

				AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */

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

				AddCpuToWatch(s, str2[ s ]);     /* JCK 981023 */

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
			while (osd_key_pressed(key)) ; /* wait for key release */
			OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
			WatchesCpuNo[ s ] = 0;    /* JCK 981008 */
			Watches[ s ] = MAX_ADDRESS(WatchesCpuNo[ s ]);    /* JCK 981009 */
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */

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

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			AddCpuToAddr(WatchesCpuNo[s], Watches[s],
                  			RD_GAMERAM(WatchesCpuNo[s], Watches[s]), buf);    /* JCK 981214 */

			if (LoadedCheatTotal < MAX_LOADEDCHEATS-1)
			{
				set_cheat(&LoadedCheatTable[LoadedCheatTotal], NEW_CHEAT);   /* JCK 981214 */
				LoadedCheatTable[LoadedCheatTotal].CpuNo   = WatchesCpuNo[s];
				LoadedCheatTable[LoadedCheatTotal].Address = Watches[s];
				LoadedCheatTable[LoadedCheatTotal].Data    = RD_GAMERAM(WatchesCpuNo[s], Watches[s]);
				strcpy(LoadedCheatTable[LoadedCheatTotal].Name, buf);
				LoadedCheatTotal++;
				xprintf(0, 0,YFOOT_WATCH,"%s Added",buf);    /* JCK 981022 */
			}
			else
				xprintf(0, 0,YFOOT_WATCH,"%s Not Added",buf);    /* JCK 981022 */
			break;

		case OSD_KEY_F4:
			while (osd_key_pressed(key)) ; /* wait for key release */

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

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

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			for (i=0;i<MAX_WATCHES;i++)
			{
				if(LoadedCheatTotal > MAX_LOADEDCHEATS-1)
					break;
				if (Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))
				{
					countAdded++;

					AddCpuToAddr(WatchesCpuNo[i], Watches[i],
		                  			RD_GAMERAM(WatchesCpuNo[i], Watches[i]), buf);    /* JCK 981214 */

					set_cheat(&LoadedCheatTable[LoadedCheatTotal], NEW_CHEAT);   /* JCK 981214 */
					LoadedCheatTable[LoadedCheatTotal].CpuNo   = WatchesCpuNo[i];
					LoadedCheatTable[LoadedCheatTotal].Address = Watches[i];
					LoadedCheatTable[LoadedCheatTotal].Data    = RD_GAMERAM(WatchesCpuNo[i], Watches[i]);
					strcpy(LoadedCheatTable[LoadedCheatTotal].Name,buf);
					LoadedCheatTotal++;
				}

          		}
			xprintf(0, 0,YFOOT_WATCH,"%d Added",countAdded);    /* JCK 981022 */

			break;

		case OSD_KEY_F7:
			while (osd_key_pressed(key)) ; /* wait for key release */

			/* JCK 981216 BEGIN */
			if (osd_key_pressed (OSD_KEY_LSHIFT) || osd_key_pressed (OSD_KEY_RSHIFT))
			{
				break;
			}
			/* JCK 981216 END */

			for (i=0;i<MAX_WATCHES;i++)
			{
				WatchesCpuNo[ i ] = 0;
				Watches[ i ] = MAX_ADDRESS(WatchesCpuNo[ i ]);
				AddCpuToWatch(i, str2[ i ]);    /* JCK 981023 */
          		}
			break;

		case OSD_KEY_F10:
			while (osd_key_pressed(key)) ; /* wait for key release */
			ChooseWatchHelp();								/* Show Help */
			y = ChooseWatchHeader();
			break;

		/* JCK 981022 END */

		case OSD_KEY_ENTER:
			if (s == 0)    /* JCK 981022 */ /* Start Of Watch DT Locations*/
				break;
			OldCpuNo = WatchesCpuNo[ s ];    /* JCK 981026 */
			WatchesCpuNo[ s ] = WatchesCpuNo[ s - 1];    /* JCK 981008 */
			Watches[ s ] = Watches[ s - 1];
			AddCpuToWatch(s, str2[ s ]);    /* JCK 981023 */

			/* JCK 981026 BEGIN */
			if (MAX_ADDRESS(WatchesCpuNo[s]) != MAX_ADDRESS(OldCpuNo))
			{
				ClearTextLine(0, dt[s].y);    /* JCK 981207 */
				dt[s].x = (MachWidth - FontWidth * strlen(str2[ s ])) / 2;
			}
			/* JCK 981026 END */

			break;
	}

	/* Set Watch Flag */
	WatchesFlag = 0;
	for(i = 0;i < MAX_WATCHES;i ++)    /* JCK 980917 */
		if(Watches[i] != MAX_ADDRESS(WatchesCpuNo[i]))    /* JCK 981009 */
		{
			WatchesFlag = 1;
			WatchEnabled = 1;    /* JCK 980917 */
		}

  } while (done != 2);    /* JCK 981023 */

  while (osd_key_pressed(key)) ; /* wait for key release */

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

  DisplayActiveCheats(y);    /* JCK 981212 */

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
					DisplayActiveCheats(y);    /* JCK 981212 */
					break;

				case 1:
					StartSearch();
					done = 0;
					DisplayActiveCheats(y);    /* JCK 981212 */
					break;

				case 2:
					ContinueSearch();
					done = 0;
					DisplayActiveCheats(y);    /* JCK 981212 */
					break;

				case 3:
					ChooseWatch();
					done = 0;
					DisplayActiveCheats(y);    /* JCK 981212 */
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
  int maxlen = 0;    /* JCK 981216 */
  int key;    /* JCK 981016 */
  int iCounter = 0;
  struct DisplayText *text = dt;

  /* JCK 981216 BEGIN */
  while (dt->text)
  {
	if (strlen(dt->text) > maxlen)
		maxlen = strlen(dt->text);
	dt++;
  }
  dt = text;
  /* JCK 981216 END */

  while (dt->text)
  {
	dt->x = (MachWidth - maxlen * FontWidth) / 2;    /* JCK 981216 */
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
		{ "" },
		{ "@Save All (F6):" },						/* Save All Cheat Info */	/* JCK 981022 */
		{ "  Save all the Cheats in" },
		{ "  the Cheat File." },
		{ "" },
		{ "" },
		{ "@Del All (F7):" },						/* Del All Cheat Info */	/* JCK 981022 */
		{ "  Remove all the active Cheats" },
		{ "" },
		{ "" },
		/* JCK 981207 BEGIN */
		{ "@Info (F12):" },						/* Info Info */
		{ "  Display Info on a Cheat" },
		{ "" },
		{ "" },
		/* JCK 981207 END */
		/* JCK 981216 BEGIN */
		{ "@More info (+):" },						/* More Info */
		{ "  Display the Extra Description" },
		{ "  of a Cheat if any." },
		{ "" },
		/* JCK 981216 END */
		{ "" },
		{ "@Press any key to continue..." },			/* Continue Prompt */		/* JCK 981216 */
		{ 0 }									/* End Of Text */
	};
	/* JCK 981016 END */

	/* JCK 981216 BEGIN */
	struct DisplayText	dtHelpText3[] =
	{
		{ "@       Cheat List Help 3" },				/* Header */
		{ "" },
		/* JCK 981216 BEGIN */
		{ "@Sologame ON/OFF (F11):" },				/* Sologame Info */
		{ "  Toggles this option ON/OFF." },
		{ "  When Sologame is ON, only" },
		{ "  Cheats for Player 1 are" },
		{ "  Loaded from the Cheat File."},
		{ "" },
		{ "@Load (F5):" },						/* Load Info */
		{ "  Load a Cheat Database" },
		{ "" },
		{ "@Add from file (Shift+F5):" },				/* Merge Info */
		{ "  Add the Cheats from a Cheat" },
		{ "  Database to the current" },
		{ "  Cheat Database." },
		{ "  (Only In Memory !)" },
		{ "" },
		{ "@Reload (F8):" },						/* Reload Database Info */
		{ "  Reload the Cheat Database" },
		{ "" },
		/* JCK 981216 END */
		{ "@Rename (F9):" },						/* Rename Database Info */
		{ "  Rename the Cheat Database" },
		{ "" },
		{ "@Help (F10):" },						/* Help Info */
		{ "  Display this help" },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};
	/* JCK 981216 END */

	ShowHelp (dtHelpText);
	ShowHelp (dtHelpText2);    /* JCK 981016 */
	ShowHelp (dtHelpText3);    /* JCK 981216 */
}

/* JCK 981016 BEGIN */
void CheatListHelpEmpty (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@       Cheat List Help" },				/* Header */
		{ "" },
		{ "" },
		{ "@Add:" },							/* Add Cheat Info */
		{ "  Add a new (blank) Cheat to" },
		{ "  the Cheat List." },
		{ "" },
		{ "" },
		/* JCK 981216 BEGIN */
		{ "@Sologame ON/OFF (F11):" },				/* Sologame Info */
		{ "  Toggles this option ON/OFF." },
		{ "  When Sologame is ON, only" },
		{ "  Cheats for Player 1 are" },
		{ "  Loaded from the Cheat File."},
		/* JCK 981216 END */
		{ "" },
		{ "" },
		{ "@Load (F5):" },						/* Load Database Info */
		{ "  Load a Cheat Database" },
		{ "" },
		{ "@Reload (F8):" },						/* Reload Database Info */
		{ "  Reload the Cheat Database" },
		{ "" },
		{ "@Rename (F9):" },						/* Rename Database Info */
		{ "  Rename the Cheat Database" },
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
		/* JCK 981216 BEGIN */
		{ "@Select Search Speed:" },					/* Fastsearch Info */
		{ "  This allow you scan all" },
		{ "  or part of memory areas" },
		{ "" },
		/* JCK 981216 END */
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
		{ "  limited to 29 characters." },									/* JCK 981216 */
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
		{ "@More:" },							/* Cheat More Info */
		{ "  Same as Name. This is" },
		{ "  the extra description." },
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

/* JCK 981216 BEGIN */
void SelectFastSearchHelp (void)
{
	struct DisplayText	dtHelpText[] =
	{
		{ "@   Select Search Speed Help" },				/* Header */
		{ "" },
		{ "" },
		{ "" },
		{ "@Slow search:" },						/* All Memory Info */
		{ "  Scan all memory to find" },
		{ "  cheats. Large amount of" },
		{ "  memory might be needed." },
		{ "" },
		{ "" },
		{ "@Normal search:" },						/* RAM & BANK Info */
		{ "  Scan all memory areas" },
		{ "  which are labelled RAM" },
		{ "  or BANK1 to BANK8." },
		{ "" },
		{ "" },
		{ "@Fastest search:" },						/* One BANK Info */
		{ "  Scan the useful memory" },
		{ "  area. Used to scan NEOGEO" },
		{ "  games and the ones with" },
		{ "  TM34010 CPU(s)." },
		{ "" },
		{ "" },
		{ "@Help (F10):" },						/* Help Info */
		{ "  Display this help" },
		{ "" },
		{ "" },
		{ "" },
		{ "" },
		{ "@ Press any key to return..." },				/* Return Prompt */
		{ 0 }									/* End Of Text */
	};

	ShowHelp (dtHelpText);
}
/* JCK 981216 END */

