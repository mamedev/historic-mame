/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	(4)	// 4 chips

void intelflash_init(void);
void intelflash_save(int chip,mame_file *f);
void intelflash_load(int chip,mame_file *f);
data8_t intelflash_read_byte(int chip, data32_t address);
void intelflash_write_byte(int chip, data32_t address, data8_t value);

#endif
