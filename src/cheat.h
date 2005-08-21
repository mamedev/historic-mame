/*********************************************************************

    cheat.h

    Cheat system.

*********************************************************************/

#pragma once

#ifndef __CHEAT_H__
#define __CHEAT_H__

extern int he_did_cheat;

void cheat_init(void);
void cheat_exit(void);

int cheat_menu(int selection);
void cheat_periodic(void);

void cheat_display_watches(void);

#endif	/* __CHEAT_H__ */
