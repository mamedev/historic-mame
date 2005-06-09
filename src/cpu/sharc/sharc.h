#ifndef _SHARC_H
#define _SHARC_H

#define SHARC_FLAG0		0
#define SHARC_FLAG1		1
#define SHARC_FLAG2		2
#define SHARC_FLAG3		3

typedef enum {
	BOOT_MODE_EPROM,
	BOOT_MODE_HOST,
	BOOT_MODE_LINK,
	BOOT_MODE_NOBOOT
} SHARC_BOOT_MODE;

typedef struct {
	SHARC_BOOT_MODE boot_mode;
} sharc_config;

#if (HAS_ADSP21062)
void adsp21062_get_info(UINT32 state, union cpuinfo *info);
#endif

extern void sharc_set_flag_input(int flag_num, int state);

#ifdef MAME_DEBUG
extern void sharc_dasm_one(char *buffer, offs_t pc, UINT64 opcode);
#endif

#endif /* _SHARC_H */
