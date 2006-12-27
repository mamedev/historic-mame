#ifndef _MC68HC11_H
#define _MC68HC11_H

#include "cpuintrf.h"

#ifdef MAME_DEBUG
offs_t hc11_disasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
#endif

void mc68hc11_get_info(UINT32 state, cpuinfo *info);

#endif
