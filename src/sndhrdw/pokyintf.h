#ifndef pokyintf_h
#define pokyintf_h

#include "sndhrdw/pokey.h"

#define NO_CLIP		0
#define USE_CLIP	1

struct POKEYinterface
{
	int num;	/* total number of pokeys in the machine */
	int clock;
	int volume;
	int clip;				/* determines if pokey.c will clip the sample range */
	/* Handlers for reading the pot values. Some Atari games use ALLPOT to return */
	/* dipswitch settings and other things */
	int (*pot0_r[MAXPOKEYS])(int offset);
	int (*pot1_r[MAXPOKEYS])(int offset);
	int (*pot2_r[MAXPOKEYS])(int offset);
	int (*pot3_r[MAXPOKEYS])(int offset);
	int (*pot4_r[MAXPOKEYS])(int offset);
	int (*pot5_r[MAXPOKEYS])(int offset);
	int (*pot6_r[MAXPOKEYS])(int offset);
	int (*pot7_r[MAXPOKEYS])(int offset);
	int (*allpot_r[MAXPOKEYS])(int offset);
};

int pokey1_sh_start (void);
int pokey2_sh_start (void);
int pokey4_sh_start (void);

int pokey_sh_start (struct POKEYinterface *interface);
void pokey_sh_stop (void);

int pokey1_r (int offset);
int pokey2_r (int offset);
int pokey3_r (int offset);
int pokey4_r (int offset);
int quad_pokey_r (int offset);

void pokey1_w (int offset,int data);
void pokey2_w (int offset,int data);
void pokey3_w (int offset,int data);
void pokey4_w (int offset,int data);
void quad_pokey_w (int offset,int data);

void pokey_sh_update (void);

#endif