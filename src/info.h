#ifndef __INFO_H
#define __INFO_H

/* CPU information table */
extern struct cpu_desc {
	int cpu_type;
	const char* desc;
} CPU_DESC[];

/* SOUND information table */
typedef unsigned (*SOUND_clock)(void* interface);
typedef unsigned (*SOUND_num)(void* interface);

extern struct sound_desc {
	int sound_type;
	SOUND_num num;
	SOUND_clock clock;
	const char* desc;
} SOUND_DESC[];

/* Print all the MAME info records */
void print_mame_info(FILE* out, const struct GameDriver* games[]);

#endif
