/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#ifndef _INTELFLASH_H_
#define _INTELFLASH_H_

#define FLASH_CHIPS_MAX	(8)	// 8 chips

void intelflash_init(int chip);

void nvram_handler_intelflash_0(mame_file *file,int read_or_write);
void nvram_handler_intelflash_1(mame_file *file,int read_or_write);
void nvram_handler_intelflash_2(mame_file *file,int read_or_write);
void nvram_handler_intelflash_3(mame_file *file,int read_or_write);
void nvram_handler_intelflash_4(mame_file *file,int read_or_write);
void nvram_handler_intelflash_5(mame_file *file,int read_or_write);
void nvram_handler_intelflash_6(mame_file *file,int read_or_write);
void nvram_handler_intelflash_7(mame_file *file,int read_or_write);

void intelflash_set_ids_0(int device, int maker);
void intelflash_set_ids_1(int device, int maker);
void intelflash_set_ids_2(int device, int maker);
void intelflash_set_ids_3(int device, int maker);
void intelflash_set_ids_4(int device, int maker);
void intelflash_set_ids_5(int device, int maker);
void intelflash_set_ids_6(int device, int maker);
void intelflash_set_ids_7(int device, int maker);
data8_t intelflash_read_byte(int chip, data32_t address);
void intelflash_write_byte(int chip, data32_t address, data8_t value);
data16_t intelflash_read_word(int chip, data32_t address);
void intelflash_write_word(int chip, data32_t address, data16_t data);

#endif
