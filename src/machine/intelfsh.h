/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	(43)

#define FLASH_INTEL_28F016S5 ( 0 )
#define FLASH_SHARP_LH28F400 ( 1 )
#define FLASH_FUJITSU_29F016A ( 2 ) /* sys 573 2Mx8 */
#define FLASH_INTEL_E28F008SA ( 3 ) /* seibu spi 1Mx8 */
#define FLASH_INTEL_TE28F160 ( 4 ) /* taito gnet 1Mx16 */

void intelflash_init(int chip, int type, void *data);

data32_t intelflash_read( int chip, data32_t address );
void intelflash_write( int chip, data32_t address, data32_t value );

extern NVRAM_HANDLER( intelflash_0 );
extern NVRAM_HANDLER( intelflash_1 );
extern NVRAM_HANDLER( intelflash_2 );
extern NVRAM_HANDLER( intelflash_3 );
extern NVRAM_HANDLER( intelflash_4 );
extern NVRAM_HANDLER( intelflash_5 );
extern NVRAM_HANDLER( intelflash_6 );
extern NVRAM_HANDLER( intelflash_7 );
extern NVRAM_HANDLER( intelflash_8 );
extern NVRAM_HANDLER( intelflash_9 );
extern NVRAM_HANDLER( intelflash_10 );
extern NVRAM_HANDLER( intelflash_11 );
extern NVRAM_HANDLER( intelflash_12 );
extern NVRAM_HANDLER( intelflash_13 );
extern NVRAM_HANDLER( intelflash_14 );
extern NVRAM_HANDLER( intelflash_15 );
extern NVRAM_HANDLER( intelflash_16 );
extern NVRAM_HANDLER( intelflash_17 );
extern NVRAM_HANDLER( intelflash_18 );
extern NVRAM_HANDLER( intelflash_19 );
extern NVRAM_HANDLER( intelflash_20 );
extern NVRAM_HANDLER( intelflash_21 );
extern NVRAM_HANDLER( intelflash_22 );
extern NVRAM_HANDLER( intelflash_23 );
extern NVRAM_HANDLER( intelflash_24 );
extern NVRAM_HANDLER( intelflash_25 );
extern NVRAM_HANDLER( intelflash_26 );
extern NVRAM_HANDLER( intelflash_27 );
extern NVRAM_HANDLER( intelflash_28 );
extern NVRAM_HANDLER( intelflash_29 );
extern NVRAM_HANDLER( intelflash_30 );
extern NVRAM_HANDLER( intelflash_31 );
extern NVRAM_HANDLER( intelflash_32 );
extern NVRAM_HANDLER( intelflash_33 );
extern NVRAM_HANDLER( intelflash_34 );
extern NVRAM_HANDLER( intelflash_35 );
extern NVRAM_HANDLER( intelflash_36 );
extern NVRAM_HANDLER( intelflash_37 );
extern NVRAM_HANDLER( intelflash_38 );
extern NVRAM_HANDLER( intelflash_39 );
extern NVRAM_HANDLER( intelflash_40 );
extern NVRAM_HANDLER( intelflash_41 );
extern NVRAM_HANDLER( intelflash_42 );

extern NVRAM_HANDLER( intelflash_16le_0 );
extern NVRAM_HANDLER( intelflash_16le_1 );
extern NVRAM_HANDLER( intelflash_16le_2 );
extern NVRAM_HANDLER( intelflash_16le_3 );
extern NVRAM_HANDLER( intelflash_16le_4 );
extern NVRAM_HANDLER( intelflash_16le_5 );
extern NVRAM_HANDLER( intelflash_16le_6 );
extern NVRAM_HANDLER( intelflash_16le_7 );
extern NVRAM_HANDLER( intelflash_16le_8 );
extern NVRAM_HANDLER( intelflash_16le_9 );
extern NVRAM_HANDLER( intelflash_16le_10 );
extern NVRAM_HANDLER( intelflash_16le_11 );
extern NVRAM_HANDLER( intelflash_16le_12 );
extern NVRAM_HANDLER( intelflash_16le_13 );
extern NVRAM_HANDLER( intelflash_16le_14 );
extern NVRAM_HANDLER( intelflash_16le_15 );
extern NVRAM_HANDLER( intelflash_16le_16 );
extern NVRAM_HANDLER( intelflash_16le_17 );
extern NVRAM_HANDLER( intelflash_16le_18 );
extern NVRAM_HANDLER( intelflash_16le_19 );
extern NVRAM_HANDLER( intelflash_16le_20 );
extern NVRAM_HANDLER( intelflash_16le_21 );
extern NVRAM_HANDLER( intelflash_16le_22 );
extern NVRAM_HANDLER( intelflash_16le_23 );
extern NVRAM_HANDLER( intelflash_16le_24 );
extern NVRAM_HANDLER( intelflash_16le_25 );
extern NVRAM_HANDLER( intelflash_16le_26 );
extern NVRAM_HANDLER( intelflash_16le_27 );
extern NVRAM_HANDLER( intelflash_16le_28 );
extern NVRAM_HANDLER( intelflash_16le_29 );
extern NVRAM_HANDLER( intelflash_16le_30 );
extern NVRAM_HANDLER( intelflash_16le_31 );
extern NVRAM_HANDLER( intelflash_16le_32 );
extern NVRAM_HANDLER( intelflash_16le_33 );
extern NVRAM_HANDLER( intelflash_16le_34 );
extern NVRAM_HANDLER( intelflash_16le_35 );
extern NVRAM_HANDLER( intelflash_16le_36 );
extern NVRAM_HANDLER( intelflash_16le_37 );
extern NVRAM_HANDLER( intelflash_16le_38 );
extern NVRAM_HANDLER( intelflash_16le_39 );
extern NVRAM_HANDLER( intelflash_16le_40 );
extern NVRAM_HANDLER( intelflash_16le_41 );
extern NVRAM_HANDLER( intelflash_16le_42 );

#endif
