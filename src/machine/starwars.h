#ifndef m_starwars_h
#define m_starwars_h

/***************************************************************************
machine\starwars.h

This file is Copyright 1997, Steve Baines.

Release 2.0 (6 August 1997)

See drivers\starwars.c for notes

***************************************************************************/

int banked_rom_r(int);
int input_bank_1_r(int);
void evggo(int, int);
void evgres(int, int);
void wdclr(int, int);
void irqclr(int, int);
void led3(int, int);
void led2(int, int);
void mpage(int, int);
void led1(int,int);
void recall(int, int);
void nstore(int, int);

int control_r (int offset);
void control_w (int offset, int data);
int starwars_interrupt (void);

#endif