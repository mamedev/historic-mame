/*##########################################################################

	atarigen.h
	
	General functions for Atari raster games.

##########################################################################*/

#include "driver.h"
#include "vidhrdw/ataripf.h"
#include "vidhrdw/atarimo.h"
#include "vidhrdw/atarian.h"
#include "vidhrdw/atarirle.h"

#ifndef __MACHINE_ATARIGEN__
#define __MACHINE_ATARIGEN__


/*##########################################################################
	CONSTANTS
##########################################################################*/

#define ATARI_CLOCK_14MHz	14318180
#define ATARI_CLOCK_20MHz	20000000
#define ATARI_CLOCK_32MHz	32000000
#define ATARI_CLOCK_50MHz	50000000



/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

typedef void (*atarigen_int_callback)(void);

typedef void (*atarigen_scanline_callback)(int scanline);

struct atarivc_state_desc
{
	int latch1;								/* latch #1 value (-1 means disabled) */
	int latch2;								/* latch #2 value (-1 means disabled) */
	int rowscroll_enable;					/* true if row-scrolling is enabled */
	int palette_bank;						/* which palette bank is enabled */
	int pf0_xscroll;						/* playfield 1 xscroll */
	int pf0_xscroll_raw;					/* playfield 1 xscroll raw value */
	int pf0_yscroll;						/* playfield 1 yscroll */
	int pf1_xscroll;						/* playfield 2 xscroll */
	int pf1_xscroll_raw;					/* playfield 2 xscroll raw value */
	int pf1_yscroll;						/* playfield 2 yscroll */
	int mo_xscroll;							/* sprite xscroll */
	int mo_yscroll;							/* sprite xscroll */
};



/*##########################################################################
	GLOBALS
##########################################################################*/

extern int 				atarigen_scanline_int_state;
extern int 				atarigen_sound_int_state;
extern int 				atarigen_video_int_state;

extern const data16_t *	atarigen_eeprom_default;
extern data16_t *		atarigen_eeprom;
extern size_t 			atarigen_eeprom_size;

extern int 				atarigen_cpu_to_sound_ready;
extern int 				atarigen_sound_to_cpu_ready;

extern data16_t *		atarivc_data;
extern data16_t *		atarivc_eof_data;
extern struct atarivc_state_desc atarivc_state;



/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

/*---------------------------------------------------------------
	INTERRUPT HANDLING
---------------------------------------------------------------*/

void atarigen_interrupt_reset(atarigen_int_callback update_int);
void atarigen_update_interrupts(void);

void atarigen_scanline_int_set(int scanline);
int atarigen_scanline_int_gen(void);
WRITE16_HANDLER( atarigen_scanline_int_ack_w );

int atarigen_sound_int_gen(void);
WRITE16_HANDLER( atarigen_sound_int_ack_w );

int atarigen_video_int_gen(void);
WRITE16_HANDLER( atarigen_video_int_ack_w );


/*---------------------------------------------------------------
	EEPROM HANDLING
---------------------------------------------------------------*/

void atarigen_eeprom_reset(void);

WRITE16_HANDLER( atarigen_eeprom_enable_w );
WRITE16_HANDLER( atarigen_eeprom_w );
READ16_HANDLER( atarigen_eeprom_r );
READ16_HANDLER( atarigen_eeprom_upper_r );

void atarigen_nvram_handler(void *file,int read_or_write);
void atarigen_hisave(void);


/*---------------------------------------------------------------
	SLAPSTIC HANDLING
---------------------------------------------------------------*/

void atarigen_slapstic_init(int cpunum, int base, int chipnum);
void atarigen_slapstic_reset(void);

WRITE16_HANDLER( atarigen_slapstic_w );
READ16_HANDLER( atarigen_slapstic_r );

void slapstic_init(int chip);
void slapstic_reset(void);
int slapstic_bank(void);
int slapstic_tweak(offs_t offset);


/*---------------------------------------------------------------
	SOUND I/O
---------------------------------------------------------------*/

void atarigen_sound_io_reset(int cpu_num);

int atarigen_6502_irq_gen(void);
READ_HANDLER( atarigen_6502_irq_ack_r );
WRITE_HANDLER( atarigen_6502_irq_ack_w );

void atarigen_ym2151_irq_gen(int irq);

WRITE16_HANDLER( atarigen_sound_w );
READ16_HANDLER( atarigen_sound_r );
WRITE16_HANDLER( atarigen_sound_upper_w );
READ16_HANDLER( atarigen_sound_upper_r );

void atarigen_sound_reset(void);
WRITE16_HANDLER( atarigen_sound_reset_w );
WRITE_HANDLER( atarigen_6502_sound_w );
READ_HANDLER( atarigen_6502_sound_r );


/*---------------------------------------------------------------
	SOUND HELPERS
---------------------------------------------------------------*/

void atarigen_init_6502_speedup(int cpunum, int compare_pc1, int compare_pc2);
void atarigen_set_ym2151_vol(int volume);
void atarigen_set_ym2413_vol(int volume);
void atarigen_set_pokey_vol(int volume);
void atarigen_set_tms5220_vol(int volume);
void atarigen_set_oki6295_vol(int volume);


/*---------------------------------------------------------------
	VIDEO CONTROLLER
---------------------------------------------------------------*/

void atarivc_reset(data16_t *eof_data);
void atarivc_update(const data16_t *data);

WRITE16_HANDLER( atarivc_w );
READ16_HANDLER( atarivc_r );

INLINE void atarivc_update_pf_xscrolls(void)
{
	atarivc_state.pf0_xscroll = atarivc_state.pf0_xscroll_raw + ((atarivc_state.pf1_xscroll_raw) & 7);
	atarivc_state.pf1_xscroll = atarivc_state.pf1_xscroll_raw + 4;
}


/*---------------------------------------------------------------
	VIDEO HELPERS
---------------------------------------------------------------*/

void atarigen_scanline_timer_reset(atarigen_scanline_callback update_graphics, int frequency);
int atarigen_get_hblank(void);
WRITE16_HANDLER( atarigen_halt_until_hblank_0_w );
WRITE16_HANDLER( atarigen_666_paletteram_w );
WRITE16_HANDLER( atarigen_expanded_666_paletteram_w );


/*---------------------------------------------------------------
	MISC HELPERS
---------------------------------------------------------------*/

void atarigen_invert_region(int region);
void atarigen_swap_mem(void *ptr1, void *ptr2, int bytes);

#endif
