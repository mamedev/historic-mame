/***************************************************************************

  namcos2.h

  Common functions & declarations for the Namco System 2 driver

***************************************************************************/

/* memory decode regions */

#define MEM_CPU1		0
#define MEM_CPU2		1
#define MEM_CPU_SOUND	2
#define MEM_CPU_MCU		3
#define MEM_GFX_OBJ 	4
#define MEM_GFX_CHR		5
#define MEM_GFX_ROZ		6
#define MEM_DATA		7
#define MEM_VOICE		8
#define MEM_SHAPE       9


/* CPU reference numbers */

#define NAMCOS2_CPU1	0
#define NAMCOS2_CPU2	1
#define NAMCOS2_CPU3	2
#define NAMCOS2_CPU4	3

#define CPU_MASTER	NAMCOS2_CPU1
#define CPU_SLAVE	NAMCOS2_CPU2
#define CPU_SOUND	NAMCOS2_CPU3
#define CPU_MCU		NAMCOS2_CPU4

/* VIDHRDW */

#define GFX_OBJ1	0
#define GFX_OBJ2    1
#define GFX_CHR		2
#define GFX_ROZ		3

#define NAMCOS2_ASSAULT             0x1000
#define NAMCOS2_ASSAULTA            0x1001
#define NAMCOS2_ASSAULT_PLUS        0x1002
#define NAMCOS2_METAL_HAWK          0x1003
#define NAMCOS2_PHELIOS             0x1004
#define NAMCOS2_BURNING_FORCE       0x1005
#define NAMCOS2_WALKYRIE            0x1006
#define NAMCOS2_MARVEL_LAND         0x1007
#define NAMCOS2_ROLLING_THUNDER_2   0x1008
#define NAMCOS2_COSMO_GANG          0x1009
#define NAMCOS2_FOUR_TRAX           0x100a
#define NAMCOS2_ORDYNE              0x100b
#define NAMCOS2_FINEST_HOUR         0x100c
#define NAMCOS2_DRAGON_SABER        0x100d
#define NAMCOS2_MIRAI_NINJA         0x100e

extern int namcos2_gametype;

int  namcos2_vh_start(void);
void namcos2_vh_stop(void);
void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh);

/* MACHINE */

void namcos2_init_machine(void);

//int  namcos2_sound_interrupt(int irq);
int  namcos2_sound_interrupt(void);
void namcos2_sound_bankselect_w(int offset, int data);


/**************************************************************/
/* Dual port memory handlers                                  */
/**************************************************************/
int	 namcos2_dpram_byte_r(int offset);
void namcos2_dpram_byte_w(int offset, int data);
int	 namcos2_68k_dpram_word_r(int offset);
void namcos2_68k_dpram_word_w(int offset, int data);


/**************************************************************/
/* Sprite memory handlers                                     */
/**************************************************************/
void namcos2_68k_sprite_ram_w(int offset, int data);
void namcos2_68k_sprite_bank_w( int offset, int data );
int namcos2_68k_sprite_ram_r(int offset );
int namcos2_68k_sprite_bank_r( int offset );

extern unsigned char *namcos2_sprite_ram;
extern int namcos2_sprite_bank;


/**************************************************************/
/*  EEPROM memory function handlers                           */
/**************************************************************/
#define NAMCOS2_68K_EEPROM_W    namcos2_68k_eeprom_w, &namcos2_eeprom, &namcos2_eeprom_size
#define NAMCOS2_68K_EEPROM_R    namcos2_68k_eeprom_r
int		namcos2_hiload(void);
void	namcos2_hisave(void);
void 	namcos2_68k_eeprom_w( int offset, int data );
int 	namcos2_68k_eeprom_r( int offset );
extern unsigned char *namcos2_eeprom;
extern int namcos2_eeprom_size;

/**************************************************************/
/*  Shared video memory function handlers                     */
/**************************************************************/
#define NAMCOS2_68K_VIDEORAM_W  MWA_BANK1, &videoram
#define NAMCOS2_68K_VIDEORAM_R  MRA_BANK1
extern int  namcos2_68k_vram_ctrl_r( int offset );
extern void namcos2_68k_vram_ctrl_w( int offset, int data );
extern unsigned char namcos2_68k_vram_ctrl[];


/**************************************************************/
/*  Shared video palette function handlers                    */
/**************************************************************/
//#define NAMCOS2_PALETTE_W     MWA_BANK2, &namcos2_68k_video_palette
//#define NAMCOS2_PALETTE_Rq    MRA_BANK2
extern unsigned char *namcos2_68k_video_palette;
extern int  namcos2_68k_video_palette_r( int offset );
extern void namcos2_68k_video_palette_w( int offset, int data );


/**************************************************************/
/*  Shared data ROM memory handlerhandlers                    */
/**************************************************************/
extern int  namcos2_68k_data_rom_r(int offset);


/**************************************************************/
/* Shared serial communications processory (CPU5 ????)        */
/**************************************************************/
extern int  namcos2_68k_serial_comms_ram_r(int offset);
extern void namcos2_68k_serial_comms_ram_w(int offset, int data);
extern int  namcos2_68k_serial_comms_ctrl_r(int offset);
extern void namcos2_68k_serial_comms_ctrl_w(int offset,int data);


/**************************************************************/
/* Shared protection/random number generator                  */
/**************************************************************/
extern int  namcos2_68k_key_r( int offset );
extern void namcos2_68k_key_w( int offset, int data );
extern unsigned char namcos2_68k_key[];


/**************************************************************/
/* Non-shared memory custom IO device - IRQ/Inputs/Outputs   */
/**************************************************************/

#define NAMCOS2_C148_0          0       /* 0x1c0000 */
#define NAMCOS2_C148_1          1
#define NAMCOS2_C148_2          2
#define NAMCOS2_C148_CPUIRQ     3
#define NAMCOS2_C148_EXIRQ      4       /* 0x1c8000 */
#define NAMCOS2_C148_POSIRQ     5
#define NAMCOS2_C148_SERIRQ     6
#define NAMCOS2_C148_VBLANKIRQ  7


extern int  namcos2_68k_master_C148[32];
extern void namcos2_68k_master_C148_w( int offset, int data );
extern int  namcos2_68k_master_C148_r( int offset );
extern int  namcos2_68k_master_vblank( void );
extern void namcos2_68k_master_posirq( int moog );

extern int  namcos2_68k_slave_C148[32];
extern void namcos2_68k_slave_C148_w( int offset, int data );
extern int  namcos2_68k_slave_C148_r( int offset );
extern int  namcos2_68k_slave_vblank(void);
extern void namcos2_68k_slave_posirq( int moog );


/**************************************************************/
/* MASTER CPU RAM MEMORY                                      */
/**************************************************************/

extern unsigned char *namcos2_68k_master_ram;

#define NAMCOS2_68K_MASTER_RAM_W    MWA_BANK3, &namcos2_68k_master_ram
#define NAMCOS2_68K_MASTER_RAM_R    MRA_BANK3


/**************************************************************/
/* SLAVE CPU RAM MEMORY                                       */
/**************************************************************/

extern unsigned char *namcos2_68k_slave_ram;

#define NAMCOS2_68K_SLAVE_RAM_W     MWA_BANK4, &namcos2_68k_slave_ram
#define NAMCOS2_68K_SLAVE_RAM_R     MRA_BANK4


/**************************************************************/
/*  ROZ - Rotate & Zoom memory function handlers              */
/**************************************************************/
#define NAMCOS2_68K_ROZ_RAM_W       MWA_BANK5, &namcos2_roz_ram
#define NAMCOS2_68K_ROZ_RAM_R       MRA_BANK5

extern void namcos2_68k_roz_ctrl_w( int offset, int data );
extern int  namcos2_68k_roz_ctrl_r( int offset );
extern unsigned char namcos2_roz_ctrl[];
extern unsigned char *namcos2_roz_ram;


/**************************************************************/
/*                                                            */
/**************************************************************/
#define BANKED_SOUND_ROM_R		MRA_BANK6
#define CPU3_ROM1				6			/* Bank number */
