/*********************************************************************

  cheat.c

*********************************************************************/

#include "driver.h"
#include <stdarg.h>

#define ram_size 65536

/*      --------- Mame Action Replay version 0.99 ------------        */

/*

Wats new in 0.99:
 -New method to search for energy bar value
 -The method that search for a number now search for the number and
   the number -1
 -Scrolling in the list when there is more than 10 value
 -Implemented a way to display memory location on the screen.


Since the DoCheat() routine can display on the screen,
the call to DoCheat() has to be done after *drv->vh_update

-   (*drv->vh_update)(Machine->scrbitmap);  * update screen *
-
+   if (nocheat == 0) DoCheat(CurrentVolume);
-
-   if (showfps || showfpstemp) * MAURY: nuove opzioni *
-   {
-     int trueorientation;



To do:
 -Edit a cheat (Name, Address, Data)
 -Erase the CHEAT ON message because it will not erase itself in vector games
 -A place to enter a new cheat by hand instead of editing the CHEAT.DAT file
   for when your friend tell you one.
 -Scan the ram of all the CPUs (maybe not?)
 -Do something for the 68000
  -We will have to detect where is the ram.
  -Allocate variable length table
  -32 bit addressing
 -Probably will have problem with a full 8086 (more than 64K)


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

static unsigned char *StartRam;
static unsigned char *BackupRam;
static unsigned char *FlagTable;
static int StartValue;
static int CurrentMethod;

static int CheatEnabled;
int he_did_cheat;


void xprintf(int x,int y,va_list arg_list,...);
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

  StartRam = NULL;
  BackupRam = NULL;
  FlagTable = NULL;

  for(i=0;i<10;i++)
    Watches[i] = 0xFFFF;
  WatchX = 0;
  WatchY = 0;
  WatchesFlag = 0;

/* Load the cheats for that game */
/* Ex.: pacman:0:4e14:6:0:Infinite Lives  */
  if ((f = fopen("CHEAT.DAT","r")) != 0){
    for(;;){

      if(fgets(str,80,f) == NULL)
        break;

			#ifdef macintosh	/* JB 971004 */
			/* remove extraneous CR on Macs if it exists */
			if( str[0] = '\r' )
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
  if(StartRam != NULL)
    free(StartRam);
  if(BackupRam != NULL)
    free(BackupRam);
  if(FlagTable != NULL)
    free(FlagTable);
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
  if(WatchesFlag != 0){
    int trueorientation;

  /* hack: force the display into standard orientation */
    trueorientation = Machine->orientation;
    Machine->orientation = ORIENTATION_DEFAULT;


    buf[0] = 0;
    for (i=0;i<10;i++){
      if(Watches[i] != 0xFFFF){
        sprintf(buf2,"%02X ",Machine->memory_region[0][Watches[i]]);
        strcat(buf,buf2);
      }
    }

    for (i = 0;i < strlen(buf);i++)
      drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,WatchX+(i*Machine->uifont->width),WatchY,0,TRANSPARENCY_NONE,0);

    Machine->orientation = trueorientation;
  }

/* Affect the memory */
  if(CheatEnabled == 1)
    for(i=0;i<CheatTotal;i++)
      if(CheatTable[i].Special == 0){
        //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
      }else{
        if(CheatTable[i].Count == 0){

/* Check special function */
          switch(CheatTable[i].Special){
            case 1:
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
/*Delete this cheat from the table*/
              for(j = i;j<CheatTotal-1;j++){
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
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              CheatTable[i].Count = 1*60;
              break;
            case 3:
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              CheatTable[i].Count = 2*60;
              break;
            case 4:
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
              CheatTable[i].Count = 5*60;
              break;

/* 5,6,7 check if the value has changed, if yes, start a timer
    when the timer end, change the location*/
            case 5:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 1*60;
                CheatTable[i].Special = 100;
              }
              break;
            case 6:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 2*60;
                CheatTable[i].Special = 101;
              }
              break;
            case 7:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 5*60;
                CheatTable[i].Special = 102;
              }
              break;

/* 8,9,10,11 do not change the location if the value change by X every frames
   This is to try to not change the value of an energy bar
   when a bonus is awarded to it at the end of a level
   See Kung Fu Master*/
            case 8:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 1;
                CheatTable[i].Special = 103;
                //CheatTable[i].Backup = Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address];
                CheatTable[i].Backup = Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address];	/* JB 971017 */
              }
              break;
            case 9:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 1;
                CheatTable[i].Special = 104;
                //CheatTable[i].Backup = Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address];
                CheatTable[i].Backup = Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address];	/* JB 971017 */
              }
              break;
            case 10:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 1;
                CheatTable[i].Special = 105;
                //CheatTable[i].Backup = Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address];
                CheatTable[i].Backup = Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address];	/* JB 971017 */
              }
              break;
            case 11:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Data){
			        if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] != CheatTable[i].Data){ /* JB 971004 */
                CheatTable[i].Count = 1;
                CheatTable[i].Special = 106;
                //CheatTable[i].Backup = Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address];
                CheatTable[i].Backup = Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address];	/* JB 971017 */
              }
              break;

/*Special case, linked with 5,6,7 */
            case 100:
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              CheatTable[i].Special = 5;
              break;
            case 101:
              //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              CheatTable[i].Special = 6;
              break;
            case 102:
               //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
			        Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
			        	[CheatTable[i].Address] = CheatTable[i].Data; /* JB 971004 */
              CheatTable[i].Special = 7;
              break;
/*Special case, linked with 8,9,10,11 */
/* Change the memory only if the memory decreased by X */
            case 103:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Backup-1)
                //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
              if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
              	[CheatTable[i].Address] != CheatTable[i].Backup-1)	/* JB 971017 */
                Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address] = CheatTable[i].Data;		/* JB 971017 */
              CheatTable[i].Special = 8;
              break;
            case 104:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Backup-2)
                //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
              if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
              	[CheatTable[i].Address] != CheatTable[i].Backup-2)	/* JB 971017 */
                Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address] = CheatTable[i].Data;		/* JB 971017 */
              CheatTable[i].Special = 9;
              break;
            case 105:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Backup-3)
                //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
              if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
              	[CheatTable[i].Address] != CheatTable[i].Backup-3)	/* JB 971017 */
                Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address] = CheatTable[i].Data;		/* JB 971017 */
              CheatTable[i].Special = 10;
              break;
            case 106:
              //if(Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] != CheatTable[i].Backup-4)
                //Machine->memory_region[CheatTable[i].CpuNo][CheatTable[i].Address] = CheatTable[i].Data;
              if(Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
              	[CheatTable[i].Address] != CheatTable[i].Backup-4)	/* JB 971017 */
                Machine->memory_region[ Machine->drv->cpu[CheatTable[i].CpuNo].memory_region ]
                	[CheatTable[i].Address] = CheatTable[i].Data;		/* JB 971017 */
              CheatTable[i].Special = 11;
              break;
          }
        }else{
          CheatTable[i].Count--;
        }
      }


/* ` continue cheat, to accelerate the search for new cheat */
  if (osd_key_pressed(OSD_KEY_TILDE)){
    osd_set_mastervolume(0);
    while (osd_key_pressed(OSD_KEY_TILDE))
      osd_update_audio(); /* give time to the sound hardware to apply the volume change */

    ContinueCheat();

    osd_set_mastervolume(CurrentVolume);
  }

/* F7 Enable/Disable the active cheats on the fly. Required for some cheat */
  if (osd_key_pressed(OSD_KEY_F7)){
    y = (Machine->scrbitmap->height - Machine->uifont->height) / 2;
    if(CheatEnabled == 0){
      CheatEnabled = 1;
      xprintf(0,y,"CHEAT ON");
    }else{
      CheatEnabled = 0;
      xprintf(0,y,"CHEAT OFF");
    }
    while (osd_key_pressed(OSD_KEY_F7));  /* wait for key release */
  }

}




/*****************
 *  Print a string at the position x y
 * if x = 0 then center the string in the screen
 */
void xprintf(int x,int y,va_list arg_list,...)
{
struct DisplayText dt[2];
char s[80];

va_list arg_ptr;
char *format;

  va_start(arg_ptr,arg_list);
  format=arg_list;
  (void) vsprintf(s,format,arg_ptr);

  dt[0].text = s;
  dt[0].color = DT_COLOR_WHITE;
  if(x == 0)
    dt[0].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(s)) / 2;
  else
    dt[0].x = x;
  if(dt[0].x < 0)
    dt[0].x = 0;
  if(dt[0].x > Machine->scrbitmap->width)
    dt[0].x = 0;
  dt[0].y = y;
  dt[1].text = 0;

  displaytext(dt,0);

}


/*****************
 *
 */
void DisplayCheats(int x,int y)
{
int i;

  xprintf(0,y,"Active Cheats:");
  x -= 4*Machine->uifont->width;
  y += 2*Machine->uifont->height;
  for(i=0;i<CheatTotal;i++){
    xprintf(x,y,"%s",CheatTable[i].Name);
    y += Machine->uifont->height;
  }
  if(CheatTotal == 0){
    x = (Machine->scrbitmap->width - Machine->uifont->width * 12) / 2;
    xprintf(x,y,"--- NONE ---");
  }
}




/*****************
 *
 */
void SelectCheat(void)
{
 int i,x,y,s,key,done,total;
 FILE *f;
 struct DisplayText dt[40];
 int flag;
 int Index;
 int StartY,EndY;

  osd_clearbitmap(Machine->scrbitmap);

  x = Machine->scrbitmap->width / 2;
  y = 10;
  x -= 13*Machine->uifont->width;
  y += 4*Machine->uifont->height;

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
    y += Machine->uifont->height;
  }

  dt[total].text = 0; /* terminate array */

  x += 4*Machine->uifont->width;
  y += 3*Machine->uifont->height;
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
      xprintf(0,15,"No Cheat available");
    }else{
      xprintf(0,10,"Del Delete/F1 Save");
      xprintf(0,20,"F2 Put in Memory Watch list");
      xprintf(0,30,"Select a Cheat (%d)",LoadedCheatTotal);
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
            memset(Machine->scrbitmap->line[i],0,Machine->scrbitmap->width);

        }
        break;

      case OSD_KEY_UP:
        if (s > 0)
          s--;
        else{
          s = total - 1;
          if(LoadedCheatTotal < 10)
            break;

/* Top of the list, page up */
          if(Index == 0)
            Index = (LoadedCheatTotal/10)*10;
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
            memset(Machine->scrbitmap->line[i],0,Machine->scrbitmap->width);

        }
        break;

      case OSD_KEY_F1:
        if(LoadedCheatTotal == 0)
          break;
        if ((f = fopen("CHEAT.DAT","a")) != 0){
					#ifdef macintosh /* JB 971004 */
					/* Use DOS-style line enders */
					fprintf(f,"%s:%d:%04X:%X:0:%s\n\r",Machine->gamedrv->name,LoadedCheatTable[s+Index].CpuNo,LoadedCheatTable[s+Index].Address,LoadedCheatTable[s+Index].Data,LoadedCheatTable[s+Index].Name);
					#else
          fprintf(f,"%s:%d:%04X:%X:0:%s\n",Machine->gamedrv->name,LoadedCheatTable[s+Index].CpuNo,LoadedCheatTable[s+Index].Address,LoadedCheatTable[s+Index].Data,LoadedCheatTable[s+Index].Name);
					#endif
          xprintf(dt[s].x+(strlen(LoadedCheatTable[s+Index].Name)*Machine->uifont->width)+Machine->uifont->width,dt[s].y,"(Saved)");
          fclose(f);
        }
        break;

      case OSD_KEY_F2:
        for (i=0;i<10;i++){
          if(Watches[i] == 0xFFFF){
            Watches[i] = LoadedCheatTable[s+Index].Address;
            xprintf(dt[s].x+(strlen(LoadedCheatTable[s+Index].Name)*Machine->uifont->width)+Machine->uifont->width,dt[s].y,"(Watch)");
            WatchesFlag = 1;
            break;
          }
        }
        break;


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

/* No more than 10 cheat at the time */
        if(CheatTotal > 9){
          break;
        }

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

  while (osd_key_pressed(key)); /* wait for key release */

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
  xprintf(0,y,"--- Choose a method ---");

  total = 6;
  dt[0].text = "Lives or other number";
  dt[1].text = "Timers (+ or - X)";
  dt[2].text = "Energy (less or more)";
  dt[3].text = "Status (bit)";
  dt[4].text = "Slow but sure (Same or not)";
  dt[5].text = "RETURN TO CHEAT MENU";
  dt[6].text = 0;

  y += 3*Machine->uifont->height;
  for (i = 0;i < total;i++)
  {
    dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
    dt[i].y = i * 2*Machine->uifont->height + y;
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
        break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;
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

          case 5:
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

  while (osd_key_pressed(key)); /* wait for key release */

  osd_clearbitmap(Machine->scrbitmap);

/* User select to return to the previous menu */
  if(done == 2)
    return;




/* If the method 1 is selected, ask for a number */
if(CurrentMethod == Method_1){
  y = 25;
  xprintf(0,y,"Enter the number you search");
  y += Machine->uifont->height;
  xprintf(0,y,"Use arrows to select");
  y += 2*Machine->uifont->height;

  s = 0;
  done = 0;
  do
  {
    xprintf(0,y,"%02d",s);

    key = osd_read_keyrepeat();

    switch (key)
    {
      case OSD_KEY_RIGHT:
      case OSD_KEY_UP:
        if(s < 99)
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
}


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


/* Backup the ram */
  memcpy (StartRam, Machine->memory_region[0], ram_size);
  memcpy (BackupRam, Machine->memory_region[0], ram_size);

  memset(FlagTable,0xFF,ram_size); /* At start, all location are good */


/* Flag the location that match the initial value if method 1 */
  if(CurrentMethod == Method_1){
    for (i=0;i<ram_size;i++)
      if((StartRam[i] != s)&&(StartRam[i] != s-1))
        FlagTable[i] = 0;

    count = 0;
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        count++;

    y += 2*Machine->uifont->height;
    xprintf(0,y,"Found : %d",count);

  }else{
    y += 4*Machine->uifont->height;
    xprintf(0,y,"Search initialized");
  }

  y += 4*Machine->uifont->height;
  xprintf(0,y,"Press a key to continue");
  key = osd_read_keyrepeat();
  while (osd_key_pressed(key)); /* wait for key release */

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

  osd_clearbitmap(Machine->scrbitmap);

  if(CurrentMethod == 0){
    StartCheat();
/*    xprintf(0,50,"You must start a search first!");
    key = osd_read_keyrepeat();
    while (osd_key_pressed(key));
    osd_clearbitmap(Machine->scrbitmap);*/
    return;
  }

  y = 10;
  count = 0;


/******** Method 1 ***********/

/* Ask new value if method 1 */
  if(CurrentMethod == Method_1){
    xprintf(0,y,"Enter the new value");
    y += Machine->uifont->height;
    xprintf(0,y,"Use arrows to select");
    y += Machine->uifont->height;
    xprintf(0,y,"Press ENTER when done");
    y += Machine->uifont->height;
    xprintf(0,y,"F1 start a new search");
    y += 2*Machine->uifont->height;

    s = StartValue;
    done = 0;
    do
    {
      xprintf(0,y,"%02d",s);

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_RIGHT:
        case OSD_KEY_UP:
          if(s < 99)
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

    while (osd_key_pressed(key)); /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

    StartValue = s; /* Save the value for when continue */

/* Flag the value */
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        if((Machine->memory_region[0][i] != s)&&(Machine->memory_region[0][i] != s-1))
          FlagTable[i] = 0;

  }



/******** Method 2 ***********/

/* Ask new value if method 2 */
  if(CurrentMethod == Method_2){
    xprintf(0,y,"Enter by how much it changed");
    y += Machine->uifont->height;
    xprintf(0,y,"since the last time");
    y += Machine->uifont->height;
    xprintf(0,y,"Use arrows to select");
    y += Machine->uifont->height;
    xprintf(0,y,"Press ENTER when done");
    y += Machine->uifont->height;
    xprintf(0,y,"F1 start a new search");
    y += 2*Machine->uifont->height;

    s = StartValue;
    done = 0;
    do
    {
      xprintf(0,y,"%+03d",s);

      key = osd_read_keyrepeat();

      switch (key)
      {
        case OSD_KEY_RIGHT:
        case OSD_KEY_UP:
          if(s < 99)
            s++;
          break;

        case OSD_KEY_LEFT:
        case OSD_KEY_DOWN:
          if(s > -99)
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

    while (osd_key_pressed(key)); /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

    StartValue = s; /* Save the value for when continue */

/* Flag the value */
    for (i=0;i<ram_size;i++)
      if(FlagTable[i] != 0)
        if(Machine->memory_region[0][i] != (BackupRam[i] + s))
          FlagTable[i] = 0;

/* Backup the ram because we ask how much the value changed since the last
   time, not in relation of the start value */
    memcpy (BackupRam, Machine->memory_region[0], ram_size);

  }



/******** Method 3 ***********/
  if(CurrentMethod == Method_3){
    xprintf(0,y,"F1 start a new search.");
    y += Machine->uifont->height;
    xprintf(0,y,"Since the last time,");
    y += Machine->uifont->height;
    xprintf(0,y,"Choose the expression");
    y += Machine->uifont->height;
    xprintf(0,y,"that match what happen.");
    y += 2*Machine->uifont->height;

    y += 2*Machine->uifont->height;
    total = 3;
    dt[0].text = "New value is smaller";
    dt[1].text = "New value is equal";
    dt[2].text = "New value is greater";
    dt[3].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
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

    while (osd_key_pressed(key)); /* wait for key release */


/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }

    if(s == 0){
/* User select that the value is now smaller, then clear the flag
    of the locations that are equal or greater at the backup */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0)
          if(Machine->memory_region[0][i] >= BackupRam[i])
            FlagTable[i] = 0;
    }else if(s==1){
/* User select that the value is equal, then clear the flag
    of the locations that do not equal the backup */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0)
          if(Machine->memory_region[0][i] != BackupRam[i])
            FlagTable[i] = 0;
    }else{
/* User select that the value is now greater, then clear the flag
    of the locations that are equal or smaller */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0)
          if(Machine->memory_region[0][i] <= BackupRam[i])
            FlagTable[i] = 0;
    }

/* Backup the ram because we ask how much the value changed since the last
   time, not in relation of the start value */
    memcpy (BackupRam, Machine->memory_region[0], ram_size);

  }




/******** Method 4 ***********/

  if(CurrentMethod == Method_4){
/* Ask if the value is the same as when we start or the opposite */
    xprintf(0,y,"Choose one of the following");
    y += Machine->uifont->height;
    xprintf(0,y,"F1 start a new search");

    y += 2*Machine->uifont->height;
    total = 2;
    dt[0].text = "Same as start";
    dt[1].text = "Opposite from start";
    dt[2].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
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

    while (osd_key_pressed(key)); /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }


    if(s == 0){
/* User select same as start */
/* We want to keep a flag if a bit has not change */
/* So,*/
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0){
          j = Machine->memory_region[0][i] ^ (StartRam[i] ^ 0xFF);
          FlagTable[i] = j & FlagTable[i];
        }
    }else{
/* User select opposite as start */
/* We want to keep a flag if a bit has change */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0){
          j = Machine->memory_region[0][i] ^ StartRam[i];
          FlagTable[i] = j & FlagTable[i];
        }
    }
  }





/******** Method 5 ***********/

  if(CurrentMethod == Method_5){
/* Ask if the value is the same as when we start or different */
    xprintf(0,y,"Choose one of the following");
    y += Machine->uifont->height;
    xprintf(0,y,"F1 start a new search");

    y += 2*Machine->uifont->height;
    total = 2;
    dt[0].text = "Same as start";
    dt[1].text = "Different from start";
    dt[2].text = 0; /* terminate array */
    for (i = 0;i < total;i++)
    {
      dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
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

    while (osd_key_pressed(key)); /* wait for key release */

/* the user abort the selection so, change nothing */
    if(done == 2){
      osd_clearbitmap(Machine->scrbitmap);
      return;
    }


    if(s == 0){
/* Discard the locations that are different from when we started */
/* The value must be the same as when we start to keep it */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0)
          if(Machine->memory_region[0][i] != StartRam[i])
            FlagTable[i] = 0;
    }else{
/* Discard the locations that are the same from when we started */
/* The value must be different as when we start to keep it */
      for (i=0;i<ram_size;i++)
        if(FlagTable[i] != 0)
          if(Machine->memory_region[0][i] == StartRam[i])
            FlagTable[i] = 0;
    }
  }





/* For all method:
    -display how much location the search
    -Display them
    -The user can press ENTER to add one to the cheat list*/

/* Count how much we have flagged */
  count = 0;
  for (i=0;i<ram_size;i++)
    if(FlagTable[i] != 0)
      count++;

  y += 2*Machine->uifont->height;
  xprintf(0,y,"Found : %d",count);

  if(count > 10)
    str = "Here is 10:";
  else if(count != 0)
    str = "Here is the list:";
  else
    str = "No list";
  y += 2*Machine->uifont->height;
  xprintf(0,y,"%s",str);

  x = (Machine->scrbitmap->width - Machine->uifont->width * 9) / 2;
  y += 2*Machine->uifont->height;

  total = 0;
  Continue = 0;
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
  dt[total].text = 0; /* terminate array */

  y += 2*Machine->uifont->height;
  xprintf(0,y,"Press ENTER to add to list",str);
  y += Machine->uifont->height;
  xprintf(0,y,"Press Page Down to scroll",str);
  y += Machine->uifont->height;
  xprintf(0,y,"Press F2 to add all to list",str);
  y += 2*Machine->uifont->height;

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
        if (s < total - 1)
          s++;
        else{
          s = 0;

/* Scroll down in the list */
          if(total < 10)
            break;
          total = 0;
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
          dt[total].text = 0; /* terminate array */

        }
        break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;
        break;

      case OSD_KEY_HOME:
        total = 0;
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
        dt[total].text = 0; /* terminate array */
        break;

      case OSD_KEY_PGDN:
        if(total == 0)
          break;
        if(total < 10)
          Continue = -1;
        total = 0;
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
        dt[total].text = 0; /* terminate array */
        break;

      case OSD_KEY_F2:
        if(total == 0)
          break;
/* Add all the list to the LoadedCheatTable */
        if(LoadedCheatTotal > 99){
          xprintf(0,y,"NOT Added");
          break;
        }
        count = 0;
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
        xprintf(0,y,"%d Added",count);
        break;


      case OSD_KEY_ENTER:
        if(total == 0)
          break;

/* Add the selected address to the LoadedCheatTable */
        if(LoadedCheatTotal > 99){
          xprintf(0,y,"NOT Added");
          break;
        }
        LoadedCheatTable[LoadedCheatTotal].CpuNo = 0;
        LoadedCheatTable[LoadedCheatTotal].Special = 0;
        LoadedCheatTable[LoadedCheatTotal].Count = 0;
        sscanf(dt[s].text,"%X",&i);
        LoadedCheatTable[LoadedCheatTotal].Address = i;
        LoadedCheatTable[LoadedCheatTotal].Data = StartRam[i];
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

  while (osd_key_pressed(key)); /* wait for key release */


  osd_clearbitmap(Machine->scrbitmap);
}


/*****************
 *
 */
void ChooseWatch(void)
{
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

  y = 40;
  xprintf(0,y,"+/- and arrows = +/- 1");
  y += Machine->uifont->height;
  xprintf(0,y,"1/2/3/4 = +1 digit X");
  y += Machine->uifont->height;
  xprintf(0,y,"Delete to disable");
  y += Machine->uifont->height;
  xprintf(0,y,"Enter copy previous");
  y += Machine->uifont->height;
  xprintf(0,y,"I J K L move position");
  y += Machine->uifont->height;
  xprintf(0,y,"FFFF mean disabled");

  y += 2*Machine->uifont->height;

  total = 0;
  for (i=0;i<10;i++){
    sprintf(str2[total],"$%04X",Watches[i]);
    dt[total].text = str2[total];
    dt[total].x = (Machine->scrbitmap->width - Machine->uifont->width * 5) / 2;
    dt[total].y = y;
    dt[total].color = DT_COLOR_WHITE;

    total++;

    y += Machine->uifont->height;
  }
  dt[total].text = 0; /* terminate array */

  s = 0;
  done = 0;
  do
  {
/* Display a test to see where the watches are */
    buf[0] = 0;
    for(i=0;i<10;i++)
      if(Watches[i] != 0xFFFF){
        sprintf(buf2,"%02X ",Machine->memory_region[0][Watches[i]]);
        strcat(buf,buf2);
      }

    strcat(buf,"  ");
    for (i = 0;i < strlen(buf);i++)
      drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,WatchX+(i*Machine->uifont->width),WatchY,0,TRANSPARENCY_NONE,0);


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

      case OSD_KEY_J:
        if(WatchX != 0)
          WatchX--;
        break;
      case OSD_KEY_L:
        if(WatchX <= (Machine->scrbitmap->width-Machine->uifont->width))
          WatchX++;
        break;
      case OSD_KEY_K:
        if(WatchY <= (Machine->scrbitmap->height-Machine->uifont->height))
          WatchY++;
        break;
      case OSD_KEY_I:
        if(WatchY != 0)
          WatchY--;
        break;

      case OSD_KEY_MINUS_PAD:
      case OSD_KEY_LEFT:
        if(Watches[s] == 0)
          Watches[s] = 0xFFFF;
        else
          Watches[s] --;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_PLUS_PAD:
      case OSD_KEY_RIGHT:
        if(Watches[s] == 0xFFFF)
          Watches[s] = 0;
        else
          Watches[s] ++;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_PGDN:
        if(Watches[s] <= 0x100)
          Watches[s] |= 0xFF00;
        else
          Watches[s] -= 0x100;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_PGUP:
        if(Watches[s] >= 0xFF00)
          Watches[s] &= 0x00FF;
        else
          Watches[s] += 0x100;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;


      case OSD_KEY_4:
        if((Watches[s]&0x000F) >= 0x000F)
          Watches[s] &= 0xFFF0;
        else
          Watches[s] ++;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_3:
        if((Watches[s]&0x00FF) >= 0x00F0)
          Watches[s] &= 0xFF0F;
        else
          Watches[s] += 0x10;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_2:
        if((Watches[s]&0x0FFF) >= 0x0F00)
          Watches[s] &= 0xF0FF;
        else
          Watches[s] += 0x100;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_1:
        if(Watches[s] >= 0xF000)
          Watches[s] &= 0x0FFF;
        else
          Watches[s] += 0x1000;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;


      case OSD_KEY_DEL:
        Watches[s] = 0xFFFF;
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;

      case OSD_KEY_ENTER:
        if(s == 0)
          break;
        Watches[s] = Watches[s-1];
        sprintf(str2[s],"$%04X",Watches[s]);
        dt[s].text = str2[s];
        break;


      case OSD_KEY_ESC:
      case OSD_KEY_TAB:
      default:
        done = 1;
        break;
    }
  } while (done == 0);

  while (osd_key_pressed(key)); /* wait for key release */

/* Set Watch Flag */
  WatchesFlag = 0;
  for(i=0;i<10;i++)
    if(Watches[i] != 0xFFFF)
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
  int i,y,s,key,done;
  int total;

/* Exit if the Cpu is a 68000 */
  if(Machine->drv->cpu[0].cpu_type == CPU_M68000){
    y = 50;
    osd_clearbitmap(Machine->scrbitmap);
    xprintf(0,y,"The cheat do not work with 68000 games yet.");
    y += 4*Machine->uifont->height;
    xprintf(0,y,"Press a key to continue");
    key = osd_read_keyrepeat();
    while (osd_key_pressed(key)); /* wait for key release */
    return 0;
  }

/* Cheat menu */
  total = 5;
  dt[0].text = "LOAD / ENABLE CHEAT";
  dt[1].text = "START A NEW SEARCH";
  dt[2].text = "CONTINUE SEARCH";
  dt[3].text = "MEMORY WATCH";
  dt[4].text = "RETURN TO MAIN MENU";
  dt[5].text = 0; /* terminate array */
  for (i = 0;i < total;i++)
  {
    dt[i].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[i].text)) / 2;
    dt[i].y = i * 2*Machine->uifont->height + 4*Machine->uifont->height;
    if (i == total-1) dt[i].y += Machine->uifont->height;
  }

  osd_clearbitmap(Machine->scrbitmap);

  DisplayCheats(dt[total - 1].x,dt[total - 1].y+4*Machine->uifont->height);

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
        break;

      case OSD_KEY_UP:
        if (s > 0) s--;
        else s = total - 1;
        break;

      case OSD_KEY_ENTER:
        switch (s)
        {
          case 0:

            SelectCheat();

            DisplayCheats(dt[total - 1].x,dt[total - 1].y+4*Machine->uifont->height);
            break;

          case 1:
            StartCheat();
            DisplayCheats(dt[total - 1].x,dt[total - 1].y+4*Machine->uifont->height);
            break;

          case 2:
            ContinueCheat();
            DisplayCheats(dt[total - 1].x,dt[total - 1].y+4*Machine->uifont->height);
            break;

          case 3:
            ChooseWatch();
            break;

          case 4:
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

  while (osd_key_pressed(key)); /* wait for key release */

  /* clear the screen before returning */
  osd_clearbitmap(Machine->scrbitmap);


  if (done == 2) return 1;
  else return 0;
}
