/*
 * ATMEL AT28C16
 *
 * 16K ( 2K x 8 ) Parallel EEPROM
 *
 */

#if !defined( AT28C16_H )
#define AT28C16_H ( 1 )

#include "driver.h"

extern void at28c16_a9_12v( int chip, int a9_12v );

/* nvram handlers */

extern void nvram_handler_at28c16_0( mame_file *file, int read_or_write );
extern void nvram_handler_at28c16_1( mame_file *file, int read_or_write );
extern void nvram_handler_at28c16_2( mame_file *file, int read_or_write );
extern void nvram_handler_at28c16_3( mame_file *file, int read_or_write );

/* 32bit memory handlers */

extern READ32_HANDLER( at28c16_0_32_r );
extern READ32_HANDLER( at28c16_1_32_r );
extern READ32_HANDLER( at28c16_2_32_r );
extern READ32_HANDLER( at28c16_3_32_r );
extern WRITE32_HANDLER( at28c16_0_32_w );
extern WRITE32_HANDLER( at28c16_1_32_w );
extern WRITE32_HANDLER( at28c16_2_32_w );
extern WRITE32_HANDLER( at28c16_3_32_w );

extern READ32_HANDLER( at28c16_0_32_lsb_r );
extern READ32_HANDLER( at28c16_1_32_lsb_r );
extern READ32_HANDLER( at28c16_2_32_lsb_r );
extern READ32_HANDLER( at28c16_3_32_lsb_r );
extern WRITE32_HANDLER( at28c16_0_32_lsb_w );
extern WRITE32_HANDLER( at28c16_1_32_lsb_w );
extern WRITE32_HANDLER( at28c16_2_32_lsb_w );
extern WRITE32_HANDLER( at28c16_3_32_lsb_w );

#endif
