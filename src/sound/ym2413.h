
#ifndef _YM2413_H_
#define _YM2413_H_

#include "2413intf.h"

/* Total # of YM2413's that can be used at once - change as needed */
#define MAX_YM2413 MAX_2413


/* YM2413 ROM file. If you want it to be loaded, define this. */
/*#define YM2413_ROM_FILE "./ym2413.rom"*/

/* Choose *one* of this banks (in order: best->worse) */
#define PATCH_OKAZAKI
// #define PATCH_ALLEGRO
// #define PATCH_MAME
// #define PATCH_GUIJT

#define PATCH_RHYTHM_FRS
// #define PATCH_RHYTHM_OKAZAKI
// #define PATCH_RHYTHM_MAME

/* YM2413 context */
typedef struct
{
	unsigned char reg[0x40];            /* 64 registers */
	unsigned char latch;                /* Register latch */
	unsigned char rhythm;               /* Rhythm instruments loaded flag */
	unsigned char user[0x10];           /* User instrument settings */
	struct
	{
		unsigned short int frequency;   /* Channel frequency */
		unsigned char volume;           /* Channel volume */
		unsigned char instrument;       /* Channel instrument */
	unsigned char oldSUSKEY;	/* Previous SUS state */
	}channel[9];
	int DAC_stream;
}t_ym2413;

/* YM3812 context */
/* This values must be cached */
typedef struct
{
	struct
	{
		unsigned char KSLm;	/* We must write this whem the ym2413 volume  */
							/* is changed */
		unsigned char KSLc;
		unsigned char TLc;	/* Is the YM2413 volume and internal TLc */
							/* combined */
						/* YM3812 :  YM2413  */
						/* TLc    =  VOL+((63-VOL)*(63/(TLc+1))) */
						/* *** But before this VOL must be adjusted *** */
						/* VOL = VOL*4+3*(VOL&1) */
		unsigned char TLm;	/* The same idea, but for rhythm mode */
		unsigned char SLRRc;	/* Release Rate is modified when SUS=1 */
	}channel[9];
}t_ym3812;


/* Global data */
extern t_ym2413 ym2413[MAX_YM2413];

/* Function prototypes */

/* Reset a given chip in the chip context */
void ym2413_reset(int chip);

/* Write to a chip in the chip context */
void ym2413_write(int chip, int address, int data);


#endif /* _YM2413_H_ */

