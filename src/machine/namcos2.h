/***************************************************************************

  namcos2.h

  Common functions & declarations for the Namco System 2 driver

***************************************************************************/

/* memory decode regions */

#define MEM_CPU1		0
#define MEM_CPU2		1
#define MEM_CPU_SOUND	2
#define MEM_CPU_MCU		3
#define MEM_GFX_OBJ		4
#define MEM_GFX_CHR		5
#define MEM_GFX_ROZ		6
#define MEM_DATA		7
#define MEM_VOICE		8

/* CPU reference numbers */

#define NAMCOS2_CPU1	0
#define NAMCOS2_CPU2	1
#define NAMCOS2_CPU3	2
#define NAMCOS2_CPU4	3

#define CPU_GAME	NAMCOS2_CPU1
#define CPU_RENDER	NAMCOS2_CPU2
#define CPU_SOUND	NAMCOS2_CPU3
#define CPU_MCU		NAMCOS2_CPU4

/* VIDHRDW */

#define GFX_OBJ		0
#define GFX_CHR		1
#define GFX_ROZ		2

extern void namcos2_1c0000_w( int offset, int data );
extern void namcos2_420000_w( int offset, int data );
extern void namcos2_4a0000_w( int offset, int data );
extern void namcos2_c40000_w( int offset, int data );
extern void namcos2_d00000_w( int offset, int data );

extern int namcos2_1c0000_r( int offset );
extern int namcos2_420000_r( int offset );
extern int namcos2_4a0000_r( int offset );
extern int namcos2_c40000_r( int offset );
extern int namcos2_d00000_r( int offset );

extern unsigned char namcos2_1c0000[];
extern unsigned char namcos2_420000[];
extern unsigned char namcos2_4a0000[];
extern unsigned char namcos2_c40000[];
extern unsigned char namcos2_d00000[];

int  namcos2_vh_start(void);
void namcos2_vh_stop(void);
void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh);

/* MACHINE */

void namcos2_init_machine(void);
//int  namcos2_input_r(int offset);


void namcos2_sound_interrupt(int irq);
void namcos2_sound_bankselect_w(int offset, int data);


/**************************************************************/
/* Dual port memory handlers                                  */
/**************************************************************/
int	 namcos2_dpram_byte_r(int offset);
void namcos2_dpram_byte_w(int offset, int data);
int	 namcos2_dpram_word_r(int offset);
void namcos2_dpram_word_w(int offset, int data);


/**************************************************************/
/* Sprite memory handlers                                     */
/**************************************************************/
int	namcos2_sprite_word_r(int offset);
void namcos2_sprite_word_w(int offset, int data);
extern unsigned char *namcos2_sprite_ram;


/**************************************************************/
/*  EEPROM memory function handlers                           */
/**************************************************************/
#define EEPROM_W				namcos2_eeprom_w, &namcos2_eeprom, &namcos2_eeprom_size
#define EEPROM_R				namcos2_eeprom_r
int		namcos2_hiload(void);
void	namcos2_hisave(void);
void 	namcos2_eeprom_w( int offset, int data );
int 	namcos2_eeprom_r( int offset );
int		namcos2_eeprom_stat_r( int offset );
extern unsigned char *namcos2_eeprom;
extern int namcos2_eeprom_size;

/**************************************************************/
/*  Shared video memory function handlers                     */
/**************************************************************/
#define SHAREDVIDEORAM_W		MWA_BANK1, &videoram
#define SHAREDVIDEORAM_R		MRA_BANK1


/**************************************************************/
/*                                                            */
/**************************************************************/
#define CPU1_WORKINGRAM_W		MWA_BANK2, &namcos2_cpu1_workingram
#define CPU1_WORKINGRAM_R		MRA_BANK2
extern unsigned char *namcos2_cpu1_workingram;


/**************************************************************/
/*                                                            */
/**************************************************************/
#define CPU2_WORKINGRAM_W		MWA_BANK3
#define CPU2_WORKINGRAM_R		MRA_BANK3


/**************************************************************/
/*                                                            */
/**************************************************************/
#define CPU1_RAM2_W				MWA_BANK4, &namcos2_cpu1_ram2
#define CPU1_RAM2_R				MRA_BANK4
extern unsigned char *namcos2_cpu1_ram2;


/**************************************************************/
/*  ROZ - Rotate & Zoom memory function handlers              */
/**************************************************************/
#define ROZ_RAM_W				MWA_BANK5, &namcos2_roz_ram
#define ROZ_RAM_R				MRA_BANK5
extern void namcos2_roz_ctrl_w( int offset, int data );
extern int namcos2_roz_ctrl_r( int offset );
extern unsigned char namcos2_roz_ctrl[];
extern unsigned char *namcos2_roz_ram;


/**************************************************************/
/*                                                            */
/**************************************************************/
#define SHAREDRAM1_W			MWA_BANK6, &namcos2_sharedram1
#define SHAREDRAM1_R			MRA_BANK6
extern unsigned char *namcos2_sharedram1;


/**************************************************************/
/*                                                            */
/**************************************************************/
#define BANKED_SOUND_ROM_R		MRA_BANK7
#define CPU3_ROM1				7			/* Bank number */
