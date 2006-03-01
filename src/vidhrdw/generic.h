/*********************************************************************

    generic.h

    Generic simple machine functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __VIDEO_GENERIC_H__
#define __VIDEO_GENERIC_H__

#include "mamecore.h"



/***************************************************************************

    Global variables

***************************************************************************/

extern UINT8 *videoram;
extern UINT16 *videoram16;
extern UINT32 *videoram32;
extern size_t videoram_size;

extern UINT8 *colorram;
extern UINT16 *colorram16;
extern UINT32 *colorram32;

extern UINT8 *spriteram;
extern UINT16 *spriteram16;
extern UINT32 *spriteram32;

extern UINT8 *spriteram_2;
extern UINT16 *spriteram16_2;
extern UINT32 *spriteram32_2;

extern UINT8 *spriteram_3;
extern UINT16 *spriteram16_3;
extern UINT32 *spriteram32_3;

extern UINT8 *buffered_spriteram;
extern UINT16 *buffered_spriteram16;
extern UINT32 *buffered_spriteram32;

extern UINT8 *buffered_spriteram_2;
extern UINT16 *buffered_spriteram16_2;
extern UINT32 *buffered_spriteram32_2;

extern size_t spriteram_size;
extern size_t spriteram_2_size;
extern size_t spriteram_3_size;

extern UINT8 *dirtybuffer;
extern UINT16 *dirtybuffer16;
extern UINT32 *dirtybuffer32;

extern mame_bitmap *tmpbitmap;
extern int flip_screen_x, flip_screen_y;



/***************************************************************************

    Function prototypes

***************************************************************************/

void generic_video_init(void);

/* Generic video start/update callbacks */
VIDEO_START( generic );
VIDEO_START( generic_bitmapped );
VIDEO_UPDATE( generic_bitmapped );

/* Generic read/write handlers */
READ8_HANDLER( videoram_r );
WRITE8_HANDLER( videoram_w );

READ8_HANDLER( colorram_r );
WRITE8_HANDLER( colorram_w );

READ8_HANDLER( spriteram_r );
WRITE8_HANDLER( spriteram_w );

READ16_HANDLER( spriteram16_r );
WRITE16_HANDLER( spriteram16_w );

READ8_HANDLER( spriteram_2_r );
WRITE8_HANDLER( spriteram_2_w );

/* Generic sprite buffering */
WRITE8_HANDLER( buffer_spriteram_w );
WRITE16_HANDLER( buffer_spriteram16_w );
WRITE32_HANDLER( buffer_spriteram32_w );

WRITE8_HANDLER( buffer_spriteram_2_w );
WRITE16_HANDLER( buffer_spriteram16_2_w );
WRITE32_HANDLER( buffer_spriteram32_2_w );

void buffer_spriteram(UINT8 *ptr,int length);
void buffer_spriteram_2(UINT8 *ptr,int length);

/* Global video attribute handling code */
void flip_screen_set(int on);
void flip_screen_x_set(int on);
void flip_screen_y_set(int on);
#define flip_screen flip_screen_x

/* sets a variable and schedules a full screen refresh if it changed */
void set_vh_global_attribute(int *addr, int data);
int get_vh_global_attribute_changed(void);


#endif	/* __VIDEO_GENERIC_H__ */
