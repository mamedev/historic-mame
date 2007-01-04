/***************************************************************************

    restrack.h

    Core MAME resource tracking.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __RESTRACK_H__
#define __RESTRACK_H__

#include "mamecore.h"


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* add a callback to be invoked on end_resource_tracking() */
void add_free_resources_callback(void (*callback)(void));

/* initialize the resource tracking system */
void init_resource_tracking(void);

/* tear down the resource tracking system */
void exit_resource_tracking(void);

/* begin tracking resources */
void begin_resource_tracking(void);

/* stop tracking resources and free everything since the last begin */
void end_resource_tracking(void);

/* return the current resource tag */
INLINE int get_resource_tag(void)
{
	extern int resource_tracking_tag;
	return resource_tracking_tag;
}



/* allocate memory and fatalerror if there's a problem */
#define malloc_or_die(s)	_malloc_or_die(s, __FILE__, __LINE__)
void *_malloc_or_die(size_t size, const char *file, int line) ATTR_MALLOC;

/* allocate memory that will be freed at the next end_resource_tracking */
#define auto_malloc(s)		_auto_malloc(s, __FILE__, __LINE__)
void *_auto_malloc(size_t size, const char *file, int line) ATTR_MALLOC;

/* allocate memory and duplicate a string that will be freed at the next end_resource_tracking */
char *auto_strdup(const char *str) ATTR_MALLOC;

/* auto_strdup() variant that tolerates NULL */
char *auto_strdup_allow_null(const char *str) ATTR_MALLOC;



#endif /* __RESTRACK_H__ */
