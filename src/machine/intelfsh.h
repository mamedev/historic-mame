/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	(4)	// 4 chips

void nvram_handler_intelflash_0(mame_file *file,int read_or_write);
void nvram_handler_intelflash_1(mame_file *file,int read_or_write);
void nvram_handler_intelflash_2(mame_file *file,int read_or_write);
void nvram_handler_intelflash_3(mame_file *file,int read_or_write);
data8_t intelflash_read_byte(int chip, data32_t address);
void intelflash_write_byte(int chip, data32_t address, data8_t value);
data16_t intelflash_read_word(int chip, data32_t address);
void intelflash_write_word(int chip, data32_t address, data16_t data);

#endif
