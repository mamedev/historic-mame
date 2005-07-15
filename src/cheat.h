/*********************************************************************

    cheat.h

    Cheat system.

*********************************************************************/

#pragma once

#ifndef __CHEAT_H__
#define __CHEAT_H__

extern int he_did_cheat;

void InitCheat(void);
void StopCheat(void);

int cheat_menu(struct mame_bitmap *bitmap, int selection);
void DoCheat(struct mame_bitmap *bitmap);

void DisplayWatches(struct mame_bitmap * bitmap);

#endif	/* __CHEAT_H__ */
