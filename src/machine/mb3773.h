/*
 * mb3773 - Power Supply Monitor with Watch Dog Timer
 *
 */

#if !defined( MB3773_H )
#define MB3773_H ( 1 )

#include "driver.h"

extern void mb3773_set_ck( data8_t new_ck );
extern void mb3773_init( void );

#endif
