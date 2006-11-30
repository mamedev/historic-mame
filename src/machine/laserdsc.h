/*************************************************************************

    laserdsc.h

    Generic laserdisc support.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*************************************************************************/

#pragma once

#ifndef __LASERDSC_H__
#define __LASERDSC_H__

#include "mamecore.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* types of players supported */
#define LASERDISC_TYPE_PR7820		0
#define LASERDISC_TYPE_LDV1000		1
#define LASERDISC_TYPE_22VP932		2

/* laserdisc control lines */
#define LASERDISC_LINE_ENTER		0			/* "ENTER" key/line */
#define LASERDISC_INPUT_LINES		1

/* laserdisc status lines */
#define LASERDISC_LINE_READY		0			/* "READY" line */
#define LASERDISC_LINE_STATUS		1			/* "STATUS STROBE" line */
#define LASERDISC_LINE_COMMAND		2			/* "COMMAND STROBE" line */
#define LASERDISC_OUTPUT_LINES		3



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _laserdisc_state laserdisc_state;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

laserdisc_state *laserdisc_init(int type);
void laserdisc_vsync(laserdisc_state *state);
void laserdisc_reset(laserdisc_state *state);
const char *laserdisc_describe_state(laserdisc_state *state);

void laserdisc_data_w(laserdisc_state *state, UINT8 data);
void laserdisc_line_w(laserdisc_state *state, UINT8 line, UINT8 newstate);
UINT8 laserdisc_data_r(laserdisc_state *state);
UINT8 laserdisc_line_r(laserdisc_state *state, UINT8 line);

void pr7820_set_slow_speed(laserdisc_state *state, double frame_rate_scaler);

#endif 	/* __LASERDSC_H__ */
