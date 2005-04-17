/*
	Intel Flash ROM emulation
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	( 56 )

#define FLASH_INTEL_28F016S5 ( 0 )
#define FLASH_SHARP_LH28F400 ( 1 )
#define FLASH_FUJITSU_29F016A ( 2 )
#define FLASH_INTEL_E28F008SA ( 3 )
#define FLASH_INTEL_TE28F160 ( 4 )

extern void intelflash_init( int chip, int type, void *data );
extern data32_t intelflash_read( int chip, data32_t address );
extern void intelflash_write( int chip, data32_t address, data32_t value );
extern void nvram_handler_intelflash( int chip, mame_file *file, int read_or_write );

#endif
