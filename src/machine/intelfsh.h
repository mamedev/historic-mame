/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	(8)	// 8 chips

#define FLASH_INTEL_28F016S5 ( 0 )
#define FLASH_SHARP_LH28F400 ( 1 )
#define FLASH_FUJITSU_29F016A ( 2 ) /* sys 573 2Mx8 */
#define FLASH_INTEL_E28F008SA ( 3 ) /* seibu spi 1Mx8 */
#define FLASH_INTEL_TE28F160 ( 4 ) /* taito gnet 1Mx16 */

void intelflash_init(int chip, int type, void *data);

data32_t intelflash_read( int chip, data32_t address );
void intelflash_write( int chip, data32_t address, data32_t value );

void nvram_handler_intelflash_0(mame_file *file,int read_or_write);
void nvram_handler_intelflash_1(mame_file *file,int read_or_write);
void nvram_handler_intelflash_2(mame_file *file,int read_or_write);
void nvram_handler_intelflash_3(mame_file *file,int read_or_write);
void nvram_handler_intelflash_4(mame_file *file,int read_or_write);
void nvram_handler_intelflash_5(mame_file *file,int read_or_write);
void nvram_handler_intelflash_6(mame_file *file,int read_or_write);
void nvram_handler_intelflash_7(mame_file *file,int read_or_write);

void nvram_handler_intelflash_16le_0(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_1(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_2(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_3(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_4(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_5(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_6(mame_file *file,int read_or_write);
void nvram_handler_intelflash_16le_7(mame_file *file,int read_or_write);

#endif
