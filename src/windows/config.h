//============================================================
//
//  config.h - Win32 configuration routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef _WIN_CONFIG__
#define _WIN_CONFIG__

#include "rc.h"

// Initializes and exits the configuration system
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

// Creates an RC object
struct rc_struct *cli_rc_create(void);

// Accessor for the current RC object
struct rc_struct *cli_rc_access(void);

// Flushes the log file
void win_flush_logfile(void);

#endif // _WIN_CONFIG__
