#ifndef __INFO_H
#define __INFO_H

const char *info_cpu_name(const struct MachineCPU *cpu);
const char *info_sound_name(const struct MachineSound *sound);
/* returns 0 if the sound type doesn't support multiple instances */
int info_sound_num(const struct MachineSound *sound);
/* returns 0 if the sound type doesn't support a clock frequency */
int info_sound_clock(const struct MachineSound *sound);

/* Print all the MAME info records */
void print_mame_info(FILE* out, const struct GameDriver* games[]);

#endif
