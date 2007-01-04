/***************************************************************************

    restrack.h

    Core MAME resource tracking.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "restrack.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	void			(*free_resources)(void);
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* malloc tracking */
static void **malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* free resource callbacks */
static callback_item *free_resources_callback_list;



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    _malloc_or_die - allocate memory or die
    trying
-------------------------------------------------*/

void *_malloc_or_die(size_t size, const char *file, int line)
{
	void *result;

	/* fail on attempted allocations of 0 */
	if (size == 0)
		fatalerror("Attempted to malloc zero bytes (%s:%d)", file, line);

	/* allocate and return if we succeeded */
#ifdef MALLOC_DEBUG
	result = malloc_file_line(size, file, line);
#else
	result = malloc(size);
#endif
	if (result != NULL)
	{
#ifdef MAME_DEBUG
		int i;
		for (i = 0; i < size; i++)
			((UINT8 *)result)[i] = rand();
#endif
		return result;
	}

	/* otherwise, die horribly */
	fatalerror("Failed to allocate %d bytes (%s:%d)", (int)size, file, line);
}


/*-------------------------------------------------
    add_free_resources_callback - add a callback to
    be invoked on end_resource_tracking()
-------------------------------------------------*/

void add_free_resources_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	cb = malloc_or_die(sizeof(*cb));
	cb->free_resources = callback;
	cb->next = NULL;

	for (cur = &free_resources_callback_list; *cur != NULL; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    free_callback_list - free a list of callbacks
-------------------------------------------------*/

static void free_callback_list(callback_item **cb)
{
	while (*cb != NULL)
	{
		callback_item *temp = *cb;
		*cb = (*cb)->next;
		free(temp);
	}
}


/*-------------------------------------------------
    auto_malloc_add - add pointer to malloc list
-------------------------------------------------*/

INLINE void auto_malloc_add(void *result)
{
	/* make sure we have tracking space */
	if (malloc_list_index == malloc_list_size)
	{
		void **list;

		/* if this is the first time, allocate 256 entries, otherwise double the slots */
		if (malloc_list_size == 0)
			malloc_list_size = 256;
		else
			malloc_list_size *= 2;

		/* realloc the list */
		list = realloc(malloc_list, malloc_list_size * sizeof(list[0]));
		if (list == NULL)
			fatalerror("Unable to extend malloc tracking array to %d slots", malloc_list_size);
		malloc_list = list;
	}
	malloc_list[malloc_list_index++] = result;
}


/*-------------------------------------------------
    auto_malloc_free - release auto_malloc'd memory
-------------------------------------------------*/

static void auto_malloc_free(void)
{
	/* start at the end and free everything till you reach the sentinel */
	while (malloc_list_index > 0 && malloc_list[--malloc_list_index] != NULL)
		free(malloc_list[malloc_list_index]);

	/* if we free everything, free the list */
	if (malloc_list_index == 0)
	{
		free(malloc_list);
		malloc_list = NULL;
		malloc_list_size = 0;
	}
}


/*-------------------------------------------------
    init_resource_tracking - initialize the
    resource tracking system
-------------------------------------------------*/

void init_resource_tracking(void)
{
	resource_tracking_tag = 0;
	add_free_resources_callback(auto_malloc_free);
}



/*-------------------------------------------------
    exit_resource_tracking - tear down the
    resource tracking system
-------------------------------------------------*/

void exit_resource_tracking(void)
{
	while (resource_tracking_tag != 0)
		end_resource_tracking();
	free_callback_list(&free_resources_callback_list);
}



/*-------------------------------------------------
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* add a NULL as a sentinel */
	auto_malloc_add(NULL);

	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
    end_resource_tracking - stop tracking
    resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	callback_item *cb;

	/* call everyone who tracks resources to let them know */
	for (cb = free_resources_callback_list; cb; cb = cb->next)
		(*cb->free_resources)();

	/* decrement the tag counter */
	resource_tracking_tag--;
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *_auto_malloc(size_t size, const char *file, int line)
{
	void *result;

	/* fail horribly if it doesn't work */
	result = _malloc_or_die(size, file, line);

	/* track this item in our list */
	auto_malloc_add(result);
	return result;
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *auto_strdup(const char *str)
{
	assert_always(str != NULL, "auto_strdup() requires non-NULL str");
	return strcpy(auto_malloc(strlen(str) + 1), str);
}


/*-------------------------------------------------
    auto_strdup_allow_null - allocate auto-freeing
    string if str is null
-------------------------------------------------*/

char *auto_strdup_allow_null(const char *str)
{
	return (str != NULL) ? auto_strdup(str) : NULL;
}

