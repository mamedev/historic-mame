/***************************************************************************

    datafile.h

    Controls execution of the core MAME system.

***************************************************************************/

#pragma once

#ifndef __DATAFILE_H__
#define __DATAFILE_H__

struct tDatafileIndex
{
	long offset;
	const struct GameDriver *driver;
};

extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);

#endif	/* __DATAFILE_H__ */
